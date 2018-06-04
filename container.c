/*
 *    tizian - better chrooting with containers
 *    Copyright (C) 2017  Salvatore Mesoraca <s.mesoraca16@gmail.com>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE 1

#include <dirent.h>
#include <fcntl.h>
#include <linux/sched.h>
#include <pty.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#ifndef NO_APPARMOR
#include <sys/apparmor.h>
#endif

#ifndef NO_SARA
#include <sara.h>
#endif

#include "cgroups.h"
#include "container.h"
#include "mount.h"
#include "namespaces.h"
#include "pty.h"
#include "user_setup.h"
#include "utils.h"

#define BUILD_BUG_ON(condition) static char __BUILDBUG[1 - 2*!!(condition)] __attribute__((unused))
BUILD_BUG_ON(sizeof(PROGNAME) > 6);

enum verbosity global_verbosity;
static int global_cont_id;
static struct container_config *global_config;

static int wait_all(void)
{
	int ret;

	while (wait(&ret) != -1);
	if (WIFEXITED(ret))
		return  WEXITSTATUS(ret);
	return -1;
}

#define MEMSET_FIELD(FIELD) memset(config->FIELD, 0, strlen(config->FIELD))

static int custom_init(struct container_config *config)
{
	unsigned char init_only;
	size_t s;

	init_only = config->init_only;
	setsid();
	if (prctl(PR_SET_NAME, "init\0", 0, 0, 0))
		print_error("Can't set name");
	if (prctl(PR_SET_CHILD_SUBREAPER, 1)) {
		print_error("Can't set subreaper");
		return -1;
	}
	if (set_user(0, 1))
		return -1;
#ifndef NO_SARA
	if (config->use_sara && add_wxprot_self_flags(SARA_FULL)) {
		perror("Can't set sara flags in custom_init");
		return -1;
	}
#endif
#ifndef NO_APPARMOR
	if (config->init_profile && aa_change_profile(config->init_profile)) {
		print_error("Can't set apparmor profile");
		return -1;
	}
#endif
	if (config->argc) {
		s = strlen(config->argv[0]);
		memset(config->argv[0], 0, s);
		snprintf(config->argv[0], s+1, "init");
	}
	for (s = 1; s < config->argc; ++s)
		MEMSET_FIELD(argv[s]);
	if (config->intermediate_profile)
		MEMSET_FIELD(intermediate_profile);
	if (config->init_profile)
		MEMSET_FIELD(init_profile);
	if (config->exec_profile)
		MEMSET_FIELD(exec_profile);
	for (s = 0; config->cmd[s] != NULL; ++s)
		MEMSET_FIELD(cmd[s]);
	for (s = 0; config->init_cmd[s] != NULL; ++s)
		MEMSET_FIELD(init_cmd[s]);
	memset(config, 0, sizeof(*config));
	close(0);
	close(1);
	close(2);
	if (init_only) {
		while (1) {
			sleep(10);
			wait_all();
		}
		return 0;
	} else
		return wait_all();
}

static int run_init(struct container_config *config)
{
	if (config->init_cmd[0] == NULL)
		return custom_init(config);

#ifndef NO_APPARMOR
	if (config->init_profile && aa_change_onexec(config->init_profile)) {
		print_error("Can't set apparmor profile");
		return -1;
	}
#endif
	execvp(config->init_cmd[0], config->init_cmd);
	print_error("Can't run init");
	return -1;
}

static pid_t find_process(const unsigned char cont_id, pid_t nspid)
{
	char buf[64];
	char cgp[64];
	FILE *f, *f2;
	pid_t p;

	snprintf(cgp, sizeof(cgp), "/sys/fs/cgroup/devices/" CG_PREFIX "%hhu/tasks", cont_id);
	f = fopen(cgp, "r");
	if (f != NULL) {
		while (fgets(buf, sizeof(buf), f)) {
			p = strtol(buf, NULL, 10);
			snprintf(cgp, sizeof(cgp), "/proc/%d/status", p);
			f2 = fopen(cgp, "r");
			if (f2 != NULL) {
				snprintf(cgp, sizeof(cgp), "NSpid:\t%d\t%d\n", p, nspid);
				while (fgets(buf, sizeof(buf), f2))
					if (strncmp(buf, cgp, strlen(cgp)) == 0)
						goto found;
				fclose(f2);
			}
		}
		fclose(f);
	}
	return -1;
found:
	fclose(f);
	fclose(f2);
	return p;
}

static inline pid_t find_init(const unsigned char cont_id)
{
	return find_process(cont_id, 1);
}

static void kill_init(const unsigned char cont_id)
{
	pid_t p;

	p = find_init(cont_id);
	if (p <= 0)
		return;
	kill(p, SIGTERM);
	usleep(10000);
	kill(p, SIGKILL);
}

ssize_t ps_container(const unsigned char cont_id,
		 pid_t *pids,
		 pid_t *nspids,
		 size_t size)
{
	char buf[64];
	char cgp[64];
	FILE *f, *f2;
	ssize_t i = 0;
	pid_t p;

	snprintf(cgp, sizeof(cgp), "/sys/fs/cgroup/devices/" CG_PREFIX "%hhu/tasks", cont_id);
	f = fopen(cgp, "r");
	if (f != NULL) {
		while (fgets(buf, sizeof(buf), f) && i < size) {
			p = strtol(buf, NULL, 10);
			pids[i] = p;
			if (nspids) {
				nspids[i] = -1;
				snprintf(cgp, sizeof(cgp), "/proc/%d/status", p);
				f2 = fopen(cgp, "r");
				if (f2 != NULL) {
					snprintf(cgp, sizeof(cgp), "NSpid:\t%d\t", p);
					while (fgets(buf, sizeof(buf), f2)) {
						if (strncmp(buf, cgp, strlen(cgp)) == 0) {
							p = strtol(buf+strlen(cgp), NULL, 10);
							nspids[i] = p;
							break;
						}
					}
					fclose(f2);
				}
			}
			++i;
		}
		fclose(f);
	}
	if (i >= size)
		i = -1;
	return i;
}

void delete_container(const unsigned char cont_id)
{
	pid_t pids[1024];
	ssize_t size;
	size_t i;

	size = ps_container(cont_id, pids, NULL, sizeof(pids));
	if (size == -1)
		size = sizeof(pids);
	for (i = 0; i < size; ++i)
		kill(pids[i], SIGTERM);
	kill_init(cont_id);
}

ssize_t list_containers(unsigned char *ids, size_t size)
{
	ssize_t i = 0;
	DIR *dir;
	struct dirent *e;

	dir = opendir("/sys/fs/cgroup/devices/");
	if (dir != NULL) {
		while ((e = readdir(dir)) != NULL && i < size) {
			if (strncmp(e->d_name, CG_PREFIX, sizeof(CG_PREFIX)-1) == 0) {
				ids[i] = strtoul(e->d_name + sizeof(CG_PREFIX)-1,
						 NULL,
						 10);
				++i;
			}
		}
		closedir(dir);
	} else {
		print_error("Can't find cgroups");
		return -1;
	}
	if (i >= size)
		i = -1;
	return i;
}

int container_exists(unsigned char cont_id)
{
	char cgp[64];

	snprintf(cgp, sizeof(cgp), "/sys/fs/cgroup/devices/" CG_PREFIX "%hhu/", cont_id);
	if (access(cgp, F_OK) != -1)
		return 1;
	return 0;
}

void delete_all_containers(void)
{
	ssize_t s, i;
	unsigned char ids[129];

	s = list_containers(ids, 129);
	for (i = 0; i < s; ++i)
		delete_container(ids[i]);
}

static int set_container(const unsigned char cont_id,
			int skip_cgroups,
			int cont_created,
			const char *const nsl[])
{
	char cgp[64];
	int i, ret;
	int *nsfd;
	pid_t p;

	p = find_init(cont_id);
	if (p < 0) {
		print_error("Can't find container's init process");
		return -1;
	}
	for (i = 0; nsl[i] != NULL; ++i);
	nsfd = calloc(i, sizeof(*nsfd));
	for (i = 0; nsl[i] != NULL; ++i) {
		snprintf(cgp, sizeof(cgp), "/proc/%d/ns/%s", p, nsl[i]);
		nsfd[i] = open(cgp, O_RDONLY);
		if (nsfd[i] < 0) {
			print_error("Can't open namespace");
			ret = -1;
			goto fd_cleanup;
		}
	}
	if (!skip_cgroups) {
		ret = set_cgroups(cont_id);
		if (ret)
			goto fd_cleanup;
	}
	for (i = 0; nsl[i] != NULL; ++i) {
		if (setns(nsfd[i], 0)) {
			if (cont_created || strcmp("user", nsl[i]) != 0) {
				print_error("Can't set namespace");
				ret = -1;
				goto fd_cleanup;
			}
		}
	}
	ret = 0;
fd_cleanup:
	for (i = 0; nsl[i] != NULL && nsfd[i] != -1; ++i)
		close(nsfd[i]);
	free(nsfd);
	return ret;
}

static inline void delete_child_container(void)
{
	if (global_cont_id != NEW_CONT_ID) {
		global_verbosity = QUIET;
		delete_container(global_cont_id);
		delete_netns(global_cont_id, global_config->ip_prefix, global_config->enable_net);
		delete_cgroups(global_cont_id);
	}
}

void sigterm(int s)
{
	delete_child_container();
	exit(0);
}

int send_fd(int fd, int sock)
{
	struct msghdr msg = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(fd))];

	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
	msg.msg_controllen = cmsg->cmsg_len;
	if((sendmsg(sock, &msg, 0)) < 0) {
		print_error("Can't send fd");
		return -1;
	}
	return 0;
}

int recv_fd(int sock)
{
	int fd;
	struct msghdr msg = {0};
	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(fd))];

	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	recvmsg(sock, &msg, 0);
	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL || cmsg -> cmsg_type != SCM_RIGHTS) {
		print_error("Can't recv fd");
		return -1;
	}
	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
	return fd;
}

int container(struct container_config *config)
{
	unsigned char cont_id = config->cont_id;
	unsigned char cont_created = 0;
	int ret = -1;
	pid_t child, ptmc = 0;
	int pt;
	int socks[2] = {0};
	pid_t wait = 'w';
	struct termios terms;

#ifndef NO_SARA
	if (config->use_sara && add_wxprot_self_flags(SARA_MPROTECT)) {
		perror("Can't set sara flags.");
		return -1;
	}
#endif

	global_cont_id = NEW_CONT_ID;
	signal(SIGWINCH, sigwinch);
	signal(SIGTERM, sigterm);

	if (config->cont_id == NEW_CONT_ID) {
		if (setup_netns(&cont_id, config->ip_prefix, config->enable_net))
			return -1;
		if (setup_cgroups(cont_id,
				  config->max_mem,
				  config->max_pids,
				  config->cpu_perc,
				  config->cpu_shares,
				  config->blkio_weight)) {
			delete_netns(cont_id, config->ip_prefix, config->enable_net);
			return -1;
		}
		cont_created = 1;
		global_cont_id = cont_id;
		global_config = config;
	}

	if (global_verbosity > QUIET) {
		if (cont_created)
			fprintf(stderr, "Container ID: %d\n", cont_id);
		else
			fprintf(stderr, "Attached to container ID: %d\n", cont_id);
	}

	if (socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, socks)) {
		print_error("Can't create socketpair");
		goto cleanup_preterm;
	}

	if (fcntl(socks[0], F_SETFD, FD_CLOEXEC) || fcntl(socks[1], F_SETFD, FD_CLOEXEC)) {
		print_error("Can't set cloexec on socketpair");
		goto cleanup_preterm;
	}

	if (prepare_term(&terms))
		goto cleanup;

	if ((child = fork()) == -1) {
		print_error("fork impossible");
		goto cleanup;
	} else if (child == 0) {
		signal(SIGWINCH, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		close(socks[0]);
		socks[0] = 0;

		if (cont_created) {
			if (set_netns(cont_id))
				return -1;
			if (set_cgroups(cont_id))
				return -1;
			if (unshare(CLONE_NEWUTS|CLONE_NEWIPC|CLONE_NEWCGROUP|CLONE_NEWNS|CLONE_NEWPID)) {
				print_error("unshare1 impossible");
				return -1;
			}
			if (config->proc_mount)
				if (pre_mount_proc(config->chroot_path, config->proc_mount, config->new_root_uid))
					return -1;
			if (setrlimit(RLIMIT_NOFILE,
				      &(struct rlimit) { .rlim_max = config->max_fds, .rlim_cur = config->max_fds})) {
				print_error("Can't set rlimit");
				return -1;
			}
		} else {
			struct rlimit o;
			pid_t init = find_init(cont_id);

			if (prlimit(init, RLIMIT_NOFILE, NULL, &o)) {
				print_error("Can't get init rlimit");
				return -1;
			}
			if (setrlimit(RLIMIT_NOFILE, &o)) {
				print_error("Can't set rlimit");
				return -1;
			}
			if (get_id_maps(&config->new_root_uid, &config->interval, init)) {
				print_error("Can't read uid_map");
				return -1;
			}
			if (set_container(cont_id, 0, cont_created,
					  (const char *const []) {"cgroup",
								"ipc",
								"mnt",
								"net",
								"pid",
								"uts",
								"user",
								NULL}))
				return -1;
		}
		if ((child = forkpty(&pt, NULL, NULL, NULL)) == -1) {
			print_error("fork2 impossible");
			return -1;
		} else if (child == 0) {
			if (!cont_created) {
				close(socks[1]);
				socks[1] = 0;
				if (fchown(0, config->uid, -1)) {
					print_error("fchown impossible");
					return -1;
				}
				if (set_user(config->uid, 0))
					return -1;
				exec(config->cmd, config->exec_profile);
				print_error("exec impossibile");
				return -1;
			}
			if (fchown(0, config->uid+config->new_root_uid, -1)) {
				print_error("fchown impossible");
				return -1;
			}
			if (config->proc_mount) {
				if (mount_proc(config->new_root_uid))
					return -1;
			}
			set_environment(0, 1);
			if (config->new_root_uid != 0) {
				if (unshare(CLONE_NEWUSER)) {
					print_error("unshare impossible");
					return -1;
				}
				if (read(socks[1], &wait, 1) != 1) {
					print_error("broken socket1");
					return -1;
				}
				if (write(socks[1], &wait, 1) != 1) {
					print_error("broken socket2");
					return -1;
				}
				if (read(socks[1], &wait, 1) != 1) {
					print_error("broken socket3");
					return -1;
				}
			}
			close(socks[1]);
			socks[1] = 0;
			set_user(0, 0);
			if (config->init_only)
				return run_init(config);
			if ((child = fork()) == -1) {
				print_error("fork2 impossible");
				return -1;
			} else if (child == 0) {
				if (set_user(config->uid, 0))
					return -1;
				exec(config->cmd, config->exec_profile);
				print_error("exec impossibile");
				return -1;
			} else
				return run_init(config);
		} else {
			if (write(socks[1], &child, sizeof(child)) != sizeof(child)) {
				print_error("broken socket4");
				return -1;
			}
			if (send_fd(pt, socks[1]))
				return -1;
			return 0;
		}
	} else {
#ifndef NO_SARA
		if (config->use_sara && add_wxprot_self_flags(SARA_FULL)) {
			perror("Can't set sara flags in outer process");
			goto cleanup;
		}
#endif
		close(socks[1]);
		socks[1] = 0;
		if (read(socks[0], &child, sizeof(child)) != sizeof(child)) {
			print_error("broken socket5");
			goto cleanup;
		}
		pt = recv_fd(socks[0]);
		if (pt < 0)
			goto cleanup;
		if ((ptmc = fork()) == -1) {
			print_error("pty_manager fork impossible");
			goto cleanup;
		} else if (ptmc == 0) {
			signal(SIGTERM, SIG_DFL);
			close(socks[0]);
			socks[0] = 0;
			setsid();
#ifndef NO_APPARMOR
			if (config->intermediate_profile &&
			    aa_change_profile(config->intermediate_profile)) {
				print_error("Can't set apparmor profile in pty_manager");
				return 1;
			}
#endif
			if (set_user(0, 0))
				return 1;
			if (set_user(0, 1))
				return 1;
			if (pty_manager(&terms, pt))
				return 1;
			return 0;
		}
		global_child = ptmc;
		if (cont_created && config->new_root_uid != 0) {
			if (write(socks[0], &wait, 1) != 1) {
				print_error("broken socket6");
				goto cleanup;
			}
			if (read(socks[0], &wait, 1) != 1) {
				print_error("broken socket7");
				goto cleanup;
			}

			if (setup_id_maps(config->new_root_uid, config->interval, child))
				goto cleanup;

			if (write(socks[0], &wait, 1) != 1) {
				print_error("broken socket8");
				goto cleanup;
			}
		}
		close(socks[0]);
		socks[0] = 0;
		if (prctl(PR_SET_CHILD_SUBREAPER, 1)) {
			print_error("Can't set subreaper");
			goto cleanup;
		}
		ret = wait_all();
		goto cleanup;
	}

	return 0;

cleanup:
	if (ptmc)
		kill(ptmc, SIGKILL);
	tcsetattr(0, TCSANOW, &terms);
cleanup_preterm:
	if (cont_created)
		delete_child_container();
	return ret;
}
