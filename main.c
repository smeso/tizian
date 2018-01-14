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

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "container.h"
#include "utils.h"

static int go_to_background = 0;

static void exit_usage(int code)
{
	FILE *f = stderr;
	if (code == 0)
		f = stdout;
	fprintf(f, "usage: %s [-q|--quiet] create [-i|--ip-prefix IP_PREFIX] [-c|--cpu-shares CPU_SHARES]\n", LONG_PROGNAME);
	fprintf(f, "                                   [-C|--cpu-perc CPU_PERC] [-b|--blkio-weight BWEIGHT]\n");
	fprintf(f, "                                   [-f|--max-fds MAX_FDS] [-p|--max-pids MAX_PIDS]\n");
	fprintf(f, "                                   [-m|--max-mem MAX_MEM] [-n|--disable-net]\n");
	fprintf(f, "                                   [-s|--disable-sara] [-I|--init-only]\n");
	fprintf(f, "                                   [-M|--no-proc-mount] [-u|--uid UID] [-e|--cmd CMD]\n");
	fprintf(f, "                                   [-E|--init-cmd INIT_CMD] [-P|--init-profile APPARMOR_PROFILE]\n");
	fprintf(f, "                                   [-d|--intermediate-profile APPARMOR_PROFILE]\n");
	fprintf(f, "                                   [-a|--cmd-profile APPARMOR_PROFILE]\n");
	fprintf(f, "                                   [-U|--userns-slot SLOT] [-B|--background] <PATH>\n");
	fprintf(f, "       %s [-q|--quiet] attach [-s|--disable-sara] [-u|--uid UID] [-e|--cmd CMD]\n", LONG_PROGNAME);
	fprintf(f, "                                   [-a|--cmd-profile CMD_APPARMOR_PROFILE] [-B|--background] <ID>\n");
	fprintf(f, "       %s [-q|--quiet] list\n", LONG_PROGNAME);
	fprintf(f, "       %s [-q|--quiet] delete <ID>\n", LONG_PROGNAME);
	fprintf(f, "       %s [-q|--quiet] ps <ID>\n", LONG_PROGNAME);
	fprintf(f, "       %s [-q|--quiet] wipe\n", LONG_PROGNAME);
	fprintf(f, "       %s [-h|--help]\n", LONG_PROGNAME);
	fprintf(f, "\n");
	fprintf(f, "%s: better chrooting with containers\n", LONG_PROGNAME);
	fprintf(f, "\n");
	fprintf(f, "optional arguments:\n");
	fprintf(f, "-h, --help					Show this help message and exit\n");
	fprintf(f, "-q, --quiet					Suppress any output.\n");
	fprintf(f, "-B, --background				run in background\n");
	fprintf(f, "-i, --ip-prefix IP_PREFIX			Set an IPv4 prefix in the format 'xxx.xxx.xxx.',\n");
	fprintf(f, "						it doesn't need to be unique across containers\n");
	fprintf(f, "						(default: '10.0.0.').\n");
	fprintf(f, "-c, --cpu-shares CPU_SHARES			Minimum numbers of cpu shares (1024 == 1 cpu)\n");
	fprintf(f, "						for this container when system is busy (default:\n");
	fprintf(f, "						256).\n");
	fprintf(f, "-C, --cpu-perc CPU_PERC				Maximum cpu usage percentage even when system is\n");
	fprintf(f, "						idle (default: 50).\n");
	fprintf(f, "-b, --blkio-weight BWEIGHT			blkio CFQ weight (default: 0 - disabled).\n");
	fprintf(f, "-f, --max-fds MAX_FDS				Maximum, per-process, number of open fds (default:\n");
	fprintf(f, "						4096).\n");
	fprintf(f, "-p, --max-pids MAX_PIDS				Maximum number of pids (default: 1024).\n");
	fprintf(f, "-m, --max-mem MAX_MEM				Max memory usage allowed, with swap, it gets doubled\n");
	fprintf(f, "						(default: 1073741824).\n");
	fprintf(f, "-n, --disable-net				No network access.\n");
	fprintf(f, "-s, --disable-sara				Do not enforce S.A.R.A. LSM memory protections.\n");
	fprintf(f, "-I, --init-only					Only run init. Do not execute any command.\n");
	fprintf(f, "-M, --no-proc-mount				Do not mount /proc and /dev/pts.\n");
	fprintf(f, "-u, --uid UID					UID (default: 0).\n");
	fprintf(f, "-e, --cmd CMD					The command to execute inside the container\n");
	fprintf(f, "						(default: the user shell according to /etc/passwd).\n");
	fprintf(f, "-E, --init-cmd INIT_CMD				The init program to run (default: a minimal built-in\n");
	fprintf(f, "						init process).\n");
	fprintf(f, "-P, --init-profile APPARMOR_PROFILE		AppArmor profile for init (default: none).\n");
	fprintf(f, "-d, --intermediate-profile APPARMOR_PROFILE	AppArmor profile for intermediate process (default: none).\n");
	fprintf(f, "-a, --cmd-profile APPARMOR_PROFILE		AppArmor profile for the command to execute (default: none).\n");
	fprintf(f, "-U, --userns-slot SLOT				UID and GID map slot. Available slots are form 0 to 65536.\n");
	fprintf(f, "						The slot is multiplied by 65535 to get the UID/GID map\n");
	fprintf(f, "						for root (default: 0).\n");
	fprintf(f, "\n");
	fprintf(f, "Example:\n\t%s create ~/chroot\n", LONG_PROGNAME);
	exit(code);
}

static const char short_simple[] = "hq";

static const struct option long_simple[] = {
	{ "help", 0, NULL, 'h' },
	{ "quiet", 0, NULL, 'q' },
	{ NULL, 0, NULL, 0 }
};

static const char short_attach[] = "hqsu:e:a:B";

static const struct option long_attach[] = {
	{ "help", 0, NULL, 'h' },
	{ "quiet", 0, NULL, 'q' },
	{ "disable-sara", 0, NULL, 's' },
	{ "uid", 1, NULL, 'u' },
	{ "cmd", 1, NULL, 'e' },
	{ "cmd-profile", 1, NULL, 'a' },
	{ "background", 0, NULL, 'B' },
	{ NULL, 0, NULL, 0 }
};

static const char short_create[] = "hqi:c:C:b:f:p:m:nsIMu:e:E:P:d:a:U:B";

static const struct option long_create[] = {
	{ "help", 0, NULL, 'h' },
	{ "quiet", 0, NULL, 'q' },
	{ "ip-prefix", 1, NULL, 'i' },
	{ "cpu-shares", 1, NULL, 'c' },
	{ "cpu-perc", 1, NULL, 'C' },
	{ "blkio-weight", 1, NULL, 'b' },
	{ "max-fds", 1, NULL, 'f' },
	{ "max-pids", 1, NULL, 'p' },
	{ "max-mem", 1, NULL, 'm' },
	{ "disable-net", 0, NULL, 'n' },
	{ "disable-sara", 0, NULL, 's' },
	{ "init-only", 0, NULL, 'I' },
	{ "no-proc-mount", 0, NULL, 'M' },
	{ "uid", 1, NULL, 'u' },
	{ "cmd", 1, NULL, 'e' },
	{ "init-cmd", 1, NULL, 'E' },
	{ "init-profile", 1, NULL, 'P' },
	{ "intermediate-profile", 1, NULL, 'd' },
	{ "cmd-profile", 1, NULL, 'a' },
	{ "userns-slot", 1, NULL, 'U' },
	{ "background", 0, NULL, 'B' },
	{ NULL, 0, NULL, 0 }
};

static char *NULL_CMD[] = {NULL};

static struct container_config config = {
	.chroot_path = {0},
	.ip_prefix = "10.0.0.",
	.cpu_shares = 256,
	.blkio_weight = 0,
	.max_pids = 1024,
	.max_fds = 4096,
	.max_mem = (1024lu*1024*1024),
	.cpu_perc = 50,
	.enable_net = 1,
	.use_sara = 1,
	.init_only = 0,
	.proc_mount = 1,
	.cont_id = NEW_CONT_ID,
	.uid = 0,
	.cmd = NULL_CMD,
	.init_cmd = NULL_CMD,
	.intermediate_profile = NULL,
	.init_profile = NULL,
	.exec_profile = NULL,
	.new_root_uid = 0,
	.interval = 65535,
	.argc = 0,
	.argv = NULL,
};

#define CHECK_CONVERT(FIELD) do {						\
	errno = 0;								\
	tmp = strtoul(optarg, &t, 10);						\
	if (tmp != (typeof(config.FIELD)) tmp ||				\
	    tmp == ULONG_MAX ||							\
	    errno ||								\
	    *t != '\0') {							\
		fprintf(stderr, "Wrong argument for %s.\n", argv[optind-2]);	\
		exit_usage(1);							\
	}									\
	config.FIELD = tmp;							\
} while (0)

static int is_ip_prefix_valid(const char *ip_prefix)
{
	struct in_addr dst;
	char src[14];

	snprintf(src, sizeof(src), "%s0", ip_prefix);
	if (inet_pton(AF_INET, src, &dst) != 1)
		return 0;
	return 1;
}

static char **split_cmd(char *cmd)
{
	char **ret;
	char *tmp, *tmp2;
	int i;
	int c = 0;

	if (strlen(cmd) == 0)
		return NULL_CMD;

	tmp = strdup(cmd);
	for (i = 0; tmp[i] != '\0'; ++i)
		if (tmp[i] == ' ')
			++c;
	++c;
	ret = calloc(c+1, sizeof(char *));
	tmp2 = strtok(tmp, " ");
	for (i = 0; i < c && tmp2; ++i) {
		ret[i] = strdup(tmp2);
		tmp2 = strtok(NULL, " ");
	}
	free(tmp);
	return ret;
}

static void set_config(int argc, char *argv[],
		       const char *const shortops, const struct option *const longops)
{
	char c, *t;
	unsigned long tmp;

	do {
		c = getopt_long(argc, argv, shortops, longops, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			exit_usage(0);
		case 'q':
			global_verbosity = QUIET;
			break;
		case 'i':
			if (strlen(optarg) > sizeof(config.ip_prefix)-1 ||
			    !is_ip_prefix_valid(optarg)) {
				fprintf(stderr, "Wrong argument for %s.\n", argv[optind-2]);
				exit_usage(1);
			}
			snprintf(config.ip_prefix, sizeof(config.ip_prefix), "%s", optarg);
			break;
		case 'c':
			CHECK_CONVERT(cpu_shares);
			break;
		case 'C':
			CHECK_CONVERT(cpu_perc);
			break;
		case 'b':
			CHECK_CONVERT(blkio_weight);
			break;
		case 'f':
			CHECK_CONVERT(max_fds);
			break;
		case 'p':
			CHECK_CONVERT(max_pids);
			break;
		case 'm':
			CHECK_CONVERT(max_mem);
			break;
		case 'n':
			config.enable_net = 0;
			break;
		case 's':
			config.use_sara = 0;
			break;
		case 'I':
			config.init_only = 1;
			break;
		case 'M':
			config.proc_mount = 0;
			break;
		case 'u':
			CHECK_CONVERT(uid);
			if (config.uid >= 65535 || config.uid < 0) {
				fprintf(stderr, "Wrong argument for %s.\n", argv[optind-2]);
				exit_usage(1);
			}
			break;
		case 'e':
			config.cmd = split_cmd(optarg);
			break;
		case 'E':
			config.init_cmd = split_cmd(optarg);
			break;
		case 'P':
			config.init_profile = strdup(optarg);
			break;
		case 'd':
			config.intermediate_profile = strdup(optarg);
			break;
		case 'a':
			config.exec_profile = strdup(optarg);
			break;
		case 'U':
			CHECK_CONVERT(new_root_uid);
			if (config.new_root_uid > 65536) {
				fprintf(stderr, "Wrong argument for %s.\n", argv[optind-2]);
				exit_usage(1);
			}
			config.new_root_uid *= 65535;
			break;
		case 'B':
			go_to_background = 1;
			break;
		case '?':
		default:
			exit_usage(1);
		}
	} while (1);
}

static int cmpids(const void *a, const void *b)
{
	return (*((unsigned char *)a))-(*((unsigned char *)b));
}

#define CHK_CMD(CMD) (strcmp(argv[1], CMD) == 0)

int main(int argc, char *argv[])
{
	char *t;
	ssize_t i;
	
	if (geteuid() != 0) {
		fprintf(stderr, "You must be root to run %s.\n", LONG_PROGNAME);
		return 1;
	}

	if (argc < 2)
		exit_usage(1);

	config.argc = argc;
	config.argv = argv;
	global_verbosity = VERBOSE;
	srand(time(NULL));

	if (CHK_CMD("create")) {
		set_config(argc-1, argv+1, short_create, long_create);
		if (argc != optind+2)
			exit_usage(1);
		snprintf(config.chroot_path,
			sizeof(config.chroot_path),
			"%s", argv[optind+1]);
		if (go_to_background) {
			pid_t child;

			if ((child = fork()) == -1) {
				perror("fork impossible");
				return 1;
			} else if (child == 0) {
				setsid();
				return container(&config);
			} else
				return 0;
		} else
			return container(&config);
	} else if (CHK_CMD("attach")) {
		set_config(argc-1, argv+1, short_attach, long_attach);
		if (argc != optind+2)
			exit_usage(1);
		errno = 0;
		config.cont_id = strtoul(argv[optind+1], &t, 10);
		if (config.cont_id != (unsigned char) config.cont_id ||
		    config.cont_id & 1 ||
		    errno ||
		    *t != '\0') {
			fprintf(stderr, "Wrong id.\n");
			exit(1);
		}
		if (!container_exists(config.cont_id)) {
			fprintf(stderr, "Container %d does not exists.\n", config.cont_id);
			exit(1);
		}
		return container(&config);
	} else if (CHK_CMD("list")) {
		unsigned char ids[129];
		ssize_t size;

		set_config(argc-1, argv+1, short_simple, long_simple);
		if (argc != optind+1)
			exit_usage(1);
		size = list_containers(ids, sizeof(ids));
		if (size == -1) {
			perror("Too many containers.");
			size = sizeof(ids);
		}
		qsort(ids, size, sizeof(*ids), cmpids);
		for (i = 0; i < size; ++i)
			printf("%u\n", ids[i]);
		return 0;
	} else if (CHK_CMD("delete")) {
		unsigned long id;

		set_config(argc-1, argv+1, short_simple, long_simple);
		if (argc != optind+2)
			exit_usage(1);
		errno = 0;
		id = strtoul(argv[optind+1], &t, 10);
		if (id != (unsigned char) id || id & 1 || errno || *t != '\0') {
			fprintf(stderr, "Wrong id.\n");
			exit_usage(1);
		}
		delete_container(id);
		return 0;
	} else if (CHK_CMD("ps")) {
		unsigned long id;
		pid_t pids[1024];
		pid_t nspids[sizeof(pids)];
		ssize_t size;

		set_config(argc-1, argv+1, short_simple, long_simple);
		if (argc != optind+2)
			exit_usage(1);
		errno = 0;
		id = strtoul(argv[optind+1], &t, 10);
		if (id != (unsigned char) id || id & 1 || errno || *t != '\0') {
			fprintf(stderr, "Wrong id.\n");
			exit_usage(1);
		}
		size = ps_container(id, pids, nspids, sizeof(pids));
		if (size == -1) {
			perror("Too many processes.");
			size = sizeof(pids);
		}
		printf("PID\tNSPID\n");
		for (i = 0; i < size; ++i)
			printf("%d\t%d\n", pids[i], nspids[i]);
		return 0;
	} else if (CHK_CMD("wipe")) {
		unsigned char ids[129];
		ssize_t size;

		set_config(argc-1, argv+1, short_simple, long_simple);
		if (argc != optind+1)
			exit_usage(1);
		size = list_containers(ids, sizeof(ids));
		if (size == -1) {
			perror("Too many containers.");
			size = sizeof(ids);
		}
		for (i = 0; i < size; ++i)
			delete_container(ids[i]);
		return 0;
	} else if (CHK_CMD("--help") || CHK_CMD("-h") ) {
		exit_usage(0);
	} else
		exit_usage(1);
	return 0;
}
