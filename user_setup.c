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

#include <errno.h>
#include <grp.h>
#include <linux/sched.h>
#include <locale.h>
#include <pwd.h>
#include <sched.h>
#include <seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef NO_APPARMOR
#include <sys/apparmor.h>
#endif

#define ARRAY_LEN(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

#include "container.h"
#include "seccomp.h"
#include "utils.h"

struct passwd *mygetpwuid(uid_t uid)
{
	static struct passwd u2;
	struct passwd *u;

	u = getpwuid(uid);
	if (u == NULL)
	{
		FILE *f;
		char buf[128];
		char uu[10];
		char *tmp;
		char found = 0;

		f = fopen("/etc/passwd", "r");
		if (f != NULL) {
			snprintf(uu, sizeof(uu), "%d", uid);
			while(fgets(buf, sizeof(buf), f) && !found) {
				tmp = strchr(buf, '\n');
				if (tmp != NULL)
					buf[(int)(tmp-buf)] = '\0';
				tmp = strchr(buf, '\r');
				if (tmp != NULL)
					buf[(int)(tmp-buf)] = '\0';
				tmp = strtok(buf, ":");
				if (u2.pw_name != NULL)
					free(u2.pw_name);
				u2.pw_name = strdup(tmp);
				tmp = strtok(NULL, ":");
				tmp = strtok(NULL, ":");
				if (strcmp(uu, tmp) == 0)
					found = 1;
				tmp = strtok(NULL, ":");
				u2.pw_gid = strtol(tmp, NULL, 10);
				tmp = strtok(NULL, ":");
				tmp = strtok(NULL, ":");
				if (u2.pw_dir != NULL)
					free(u2.pw_dir);
				u2.pw_dir = strdup(tmp);
				tmp = strtok(NULL, ":");
				if (u2.pw_shell != NULL)
					free(u2.pw_shell);
				u2.pw_shell = strdup(tmp);
			}
			if (found)
				return &u2;
		}
	}
	return u;
}

int set_environment(uid_t uid, int set_host)
{
	FILE *f;
	char buf[4096];
	char *tmp;
	struct passwd *u;

	buf[0] = '\0';
	tmp = getenv("TERM");
	if (tmp != NULL) {
		strncpy(buf, tmp, sizeof(buf));
		buf[sizeof(buf)-1] = '\0';
	}

	if (clearenv()) {
		print_error("Can't reset environ");
		return -1;
	}
	if (setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1)) {
		print_error("Can't set env PATH");
		return -1;
	}
	if (strlen(buf) > 0) {
		if (setenv("TERM", buf, 1)) {
			print_error("Can't set env");
			return -1;
		}
	}
	if (setenv("IFS", " \t\n", 1)) {
		print_error("Can't set env");
		return -1;
	}
	u = mygetpwuid(uid);
	if (u != NULL) {
		if (setenv("HOME", u->pw_dir, 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("USER", u->pw_name, 1)) {
			print_error("Can't set env");
			return -1;
		}
		snprintf(buf, sizeof(buf), "%d", u->pw_gid);
		if (setenv("GROUPS", buf, 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("LOGNAME", u->pw_name, 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("SHELL", u->pw_shell, 1)) {
			print_error("Can't set env");
			return -1;
		}
	} else {
		if (setenv("HOME", "/root", 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("USER", "root", 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("GROUPS", "0", 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("LOGNAME", "root", 1)) {
			print_error("Can't set env");
			return -1;
		}
		if (setenv("SHELL", "/bin/bash", 1)) {
			print_error("Can't set env");
			return -1;
		}
	}

	snprintf(buf, sizeof(buf), "C");
	f = fopen("/etc/locale.gen", "r");
	if (f != NULL) {
		while (fgets(buf, sizeof(buf), f) != NULL) {
			tmp = strchr(buf, '\n');
			if (tmp != NULL)
				buf[(int)(tmp-buf)] = '\0';
			tmp = strchr(buf, '\r');
			if (tmp != NULL)
				buf[(int)(tmp-buf)] = '\0';
			strtok(buf, " ");
			if (strlen(buf) != 0 && buf[0] != '#')
				break;
			snprintf(buf, sizeof(buf), "C");
		}
		fclose(f);
	} else
		snprintf(buf, sizeof(buf), "C");
	if (setlocale(LC_ALL, buf) == NULL) {
		if (setlocale(LC_ALL, "C") == NULL) {
			print_error("Can't set locale");
			return -1;
		}
	}
	if (setenv("LANG", buf, 1)) {
		print_error("Can't set env LANG");
		return -1;
	}
	if (setenv("LC_ALL", buf, 1)) {
		print_error("Can't set env LC_ALL");
		return -1;
	}

	f = fopen("/etc/hostname", "r");
	if (f != NULL) {
		if (fgets(buf, sizeof(buf), f) == NULL) {;
			snprintf(buf, sizeof(buf), PROGNAME);
		} else {
			tmp = strchr(buf, '\n');
			if (tmp != NULL)
				buf[(int)(tmp-buf)] = '\0';
			tmp = strchr(buf, '\r');
			if (tmp != NULL)
				buf[(int)(tmp-buf)] = '\0';
		}
		fclose(f);
	} else
		snprintf(buf, sizeof(buf), PROGNAME);

	if (set_host && sethostname(buf, strlen(buf))) {
		print_error("Can't set hostname");
		return -1;
	}
	if (set_host && setdomainname(buf, strlen(buf))) {
		print_error("Can't set domainname");
		return -1;
	}
	if (setenv("HOSTNAME", buf, 1)) {
		print_error("Can't set env HOSTNAME");
		return -1;
	}

	f = fopen("/etc/timezone", "r");
	if (f != NULL) {
		if (fgets(buf, sizeof(buf), f)) {
			tmp = strchr(buf, '\n');
			if (tmp != NULL)
				buf[(int)(tmp-buf)] = '\0';
			tmp = strchr(buf, '\r');
			if (tmp != NULL)
				buf[(int)(tmp-buf)] = '\0';
			if (setenv("TZ", buf, 1)) {
				print_error("Can't set env TZ");
				return -1;
			}
		}
		fclose(f);
	}

	if (chdir(getenv("HOME")) != 0) {
		if (chdir("/") != 0) {
			print_error("Can't chdir");
			return -1;
		}
	}
	return 0;
}

static int drop_capabilities(int all)
{
	int i;
	cap_t caps;
	cap_value_t cap_list[] = {CAP_CHOWN, CAP_DAC_OVERRIDE, CAP_FOWNER,
				  CAP_FSETID, CAP_IPC_OWNER, CAP_KILL,
				  CAP_MKNOD, CAP_NET_ADMIN, CAP_NET_BIND_SERVICE,
				  CAP_NET_RAW, CAP_SETFCAP, CAP_SETGID,
				  CAP_SETPCAP, CAP_SETUID, CAP_SYS_CHROOT};
	cap_flag_value_t ret;


	caps = cap_init();
	if (caps == NULL) {
		print_error("Can't init caps");
		return -1;
	}

	if (!all) {
		if (cap_set_flag(caps,
				CAP_EFFECTIVE,
				ARRAY_LEN(cap_list),
				cap_list, CAP_SET) == -1) {
			print_error("Can't set effective caps");
			return -1;
		}

		if (cap_set_flag(caps,
				CAP_PERMITTED,
				ARRAY_LEN(cap_list),
				cap_list, CAP_SET) == -1) {
			print_error("Can't set effective caps");
			return -1;
		}

		if (cap_set_flag(caps,
				CAP_INHERITABLE,
				ARRAY_LEN(cap_list),
				cap_list, CAP_SET) == -1) {
			print_error("Can't set effective caps");
			return -1;
		}

		for (i = 0; i <= CAP_LAST_CAP; ++i) {
			if (CAP_IS_SUPPORTED(i)) {
				if (cap_get_flag(caps, i, CAP_EFFECTIVE, &ret))
					ret = CAP_CLEAR;
				if (ret == CAP_CLEAR) {
					if (cap_drop_bound(i)) {
						print_error("Can't drop cap");
						return -1;
					}
				}
			}
		}
	}

	if (cap_set_proc(caps) == -1) {
		print_error("Can't set caps");
		return -1;
	}

	if (cap_free(caps) == -1)
		print_error("Can't free caps");

	return 0;
}

static int apply_seccomp(int all)
{
	int ret = -1;
	unsigned int i;
	scmp_filter_ctx ctx;

	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
		print_error("no new privs impossible");
		return -1;
	}
	if (prctl(PR_SET_DUMPABLE, 0)) {
		print_error("set dumpable impossible");
		return -1;
	}
	ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM));
	if (ctx == NULL) {
		print_error("seccomp_init impossible");
		return -1;
	}
	if (!all) {
		for (i = 0; i < ARRAY_LEN(seccomp_syscalls); ++i) {
			if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, seccomp_syscalls[i], 0)) {
				print_error("seccomp rule add impossible");
				goto exit;
			}
		}
		for (i = 0; i < ARRAY_LEN (socket_whitelist); i++) {
			if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(socket), 1, SCMP_A0(SCMP_CMP_EQ, socket_whitelist[i]))) {
				print_error("seccomp socket rule add impossible");
				goto exit;
			}
		}
		if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(unshare), 1,
				     SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, 0))) {
			print_error("seccomp unshare rule add impossible");
			goto exit;
		}
		if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(clone), 1,
				     SCMP_A0(SCMP_CMP_MASKED_EQ, CLONE_NEWUSER, 0))) {
			print_error("seccomp clone rule add impossible");
			goto exit;
		}
	} else {
		for (i = 0; i < ARRAY_LEN(seccomp_syscalls_short); ++i) {
			if (seccomp_rule_add(ctx, SCMP_ACT_ALLOW, seccomp_syscalls_short[i], 0)) {
				print_error("rule add impossible");
				goto exit;
			}
		}
	}

#if defined(__i386__) || defined(__x86_64__)
	i = seccomp_arch_add (ctx, SCMP_ARCH_X86);
	if (i < 0 && i != -EEXIST) {
		print_error("Can't add seccomp arch x86");
		goto exit;
	}
	i = seccomp_arch_add (ctx, SCMP_ARCH_X86_64);
	if (i < 0 && i != -EEXIST) {
		print_error("Can't add seccomp arch x86_64");
		goto exit;
	}
	i = seccomp_arch_add (ctx, SCMP_ARCH_X32);
	if (i < 0 && i != -EEXIST) {
		print_error("Can't add seccomp arch x32");
		goto exit;
	}
#endif

	if (seccomp_load(ctx)) {
		print_error("seccomp_load impossible");
		goto exit;
	}
	ret = 0;
exit:
	seccomp_release(ctx);
	return ret;
}

int exec(char *const argv[], char *const profile)
{
#ifndef NO_APPARMOR
	if (profile && aa_change_onexec(profile)) {
		print_error("Can't set apparmor profile");
		return -1;
	}
#endif
	if (argv[0] == NULL) {
		char name[1024];

		snprintf(name, sizeof(name), "-%s", getenv("SHELL"));
		execlp(getenv("SHELL"), name, (char *) NULL);
	} else
		execvp(argv[0], argv);
	return -1;
}

int set_user(uid_t uid, int drop)
{
	gid_t groups[100];
	int ngroups = ARRAY_LEN(groups);
	struct passwd *u;

	u = mygetpwuid(uid);
	if (u == NULL) {
		fprintf(stderr, "Can't find user %d.\n", uid);
// 		return -1;
	} else {
		getgrouplist(u->pw_name, u->pw_gid, groups, &ngroups);
		if (setgroups(ngroups, groups)) {
			print_error("Can't setgroups");
			return -1;
		}
		if (setgid(u->pw_gid)) {
			print_error("Can't setgid");
			return -1;
		}
	}
	if (drop_capabilities(0))
		return -1;
	if (drop && drop_capabilities(drop))
		return -1;
	if (apply_seccomp(0))
		return -1;
	if (set_environment(uid, 0))
		return -1;
	if (setuid(uid)) {
		print_error("Can't setuid");
		return -1;
	}
	return 0;
}
