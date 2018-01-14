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

#define _GNU_SOURCE

#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "container.h"
#include "namespaces.h"
#include "utils.h"

#define MAX_RETRIES ((NEW_CONT_ID+1)/2)

static int run_cmd(char *const argv[])
{
	int ret;
	pid_t child;

	if ((child = fork()) == -1) {
		print_error("run_cmd fork impossible");
		return -1;
	} else if (child == 0) {
		execvp(argv[0], argv);
		return -1;
	} else {
		waitpid(child, &ret, 0);
		return WEXITSTATUS(ret);
	}
}

static unsigned char random_id(void)
{
	unsigned char nid;

	nid = (unsigned char) rand();
	return nid & 0xFE;
}

int setup_netns(unsigned char *id, const char *ip_prefix, int enable_net)
{
	int i;
	unsigned char _id = NEW_CONT_ID;
	char vh[16], vg[16];
	char vhn[19], vgn[19];
	char vhs[19], vgs[19], vhs2[16];
	FILE *f;

	for (i = 0; i < MAX_RETRIES; ++i) {
		_id = random_id();
		if (_id == NEW_CONT_ID)
			continue;
		snprintf(vh, sizeof(vh), NETNS_PREFIX"0_%hhu", _id);
		snprintf(vg, sizeof(vg), NETNS_PREFIX"1_%hhu", _id);
		if (run_cmd((char *const []){"ip", "link", "add",
						"dev", vh, "type",
						"veth", "peer",
						"name", vg, NULL}) == 0)
			break;
		_id = NEW_CONT_ID;
	}
	if (_id == NEW_CONT_ID) {
		print_error("Can't find a free id");
		return -1;
	}
	*id = _id;
	if (run_cmd((char *const []){"ip", "link", "set",
					"dev", vh, "up", NULL})) {
		print_error("Can't bring vh up");
		return -1;
	}
	snprintf(vhn, sizeof(vhn), "%s%hhu/31", ip_prefix, _id);
	snprintf(vgn, sizeof(vgn), "%s%hhu/31", ip_prefix, _id+1);
	snprintf(vhs, sizeof(vhs), "%s%hhu/32", ip_prefix, _id);
	snprintf(vhs2, sizeof(vhs2), "%s%hhu", ip_prefix, _id);
	snprintf(vgs, sizeof(vgs), "%s%hhu/32", ip_prefix, _id+1);
	if (run_cmd((char *const []){"ip", "addr", "add",
					vhn, "dev", vh, NULL})) {
		print_error("Can't set addr for vh");
		return -1;
	}
	if (run_cmd((char *const []){"ip", "netns", "add",
					vg, NULL})) {
		print_error("Can't create ns");
		return -1;
	}
	if (run_cmd((char *const []){"ip", "link", "set", vg,
					"netns", vg, NULL})) {
		print_error("Can't add vg to ns");
		return -1;
	}
	if (run_cmd((char *const []){"ip", "netns", "exec", vg,
					"ip", "link", "set", "dev",
					"lo", "up", NULL})) {
		print_error("Can't bring up lo in ns");
		return -1;
	}
	if (run_cmd((char *const []){"ip", "netns", "exec", vg,
					"ip", "addr", "add",
					vgn, "dev", vg, NULL})) {
		print_error("Can't set addr on vg");
		return -1;
	}
	if (enable_net) {
		if (run_cmd((char *const []){"ip", "netns", "exec", vg,
						"ip", "link", "set", "dev",
						vg, "up", NULL})) {
			print_error("Can't bring up vg");
			return -1;
		}
		if (run_cmd((char *const []){"ip", "netns", "exec", vg,
						"ip", "route", "add",
						"default", "via", vhs2, NULL})) {
			print_error("Can't add default route in ns");
			return -1;
		}
	}
	if (run_cmd((char *const []){"iptables", "-N", vg, NULL})) {
		print_error("Can't create iptables chain");
		return -1;
	}
	if (run_cmd((char *const []){"iptables", "-A", "FORWARD",
					"-i", vh, "-j", vg, NULL})) {
		print_error("Can't create iptables rule 1");
		return -1;
	}
	if (enable_net) {
		if (run_cmd((char *const []){"iptables", "-A", vg,
						"-d", vhs, "-j", "ACCEPT", NULL})) {
			print_error("Can't create iptables rule 2");
			return -1;
		}
	}
	if (run_cmd((char *const []){"iptables", "-A", vg,
					"-d", "192.168.0.0/16", "-j", "DROP", NULL})) {
		print_error("Can't create iptables rule 3");
		return -1;
	}
	if (run_cmd((char *const []){"iptables", "-A", vg,
					"-d", "172.16.0.0/20", "-j", "DROP", NULL})) {
		print_error("Can't create iptables rule 4");
		return -1;
	}
	if (run_cmd((char *const []){"iptables", "-A", vg, "-d",
					"10.0.0.0/10", "-j", "DROP", NULL})) {
		print_error("Can't create iptables rule 5");
		return -1;
	}
	if (enable_net) {
		if (run_cmd((char *const []){"iptables", "-A", vg,
						"-j", "ACCEPT", NULL})) {
			print_error("Can't create iptables rule 6");
			return -1;
		}
		if (run_cmd((char *const []){"iptables", "-A", "FORWARD",
						"-o", vh, "-j", "ACCEPT", NULL})) {
			print_error("Can't create iptables rule 7");
			return -1;
		}
		if (run_cmd((char *const []){"iptables", "-t", "nat", "-A",
						"POSTROUTING", "-s", vgs, "-j",
						"MASQUERADE", NULL})) {
			print_error("Can't create iptables rule 8");
			return -1;
		}
		f = fopen("/proc/sys/net/ipv4/ip_forward", "w");
		if (f == NULL) {
			print_error("Can't open /proc/sys/net/ipv4/ip_forward");
			return -1;
		}
		fwrite("1", 1, 1, f);
		fclose(f);
	}
	return 0;
}

int delete_netns(const unsigned char id, const char *ip_prefix, int enable_net)
{
	char vh[16], vg[16], vgs[19];

	snprintf(vh, sizeof(vh), NETNS_PREFIX"0_%hhu", id);
	snprintf(vg, sizeof(vg), NETNS_PREFIX"1_%hhu", id);
	snprintf(vgs, sizeof(vgs), "%s%hhu/32", ip_prefix, id+1);
	if (run_cmd((char *const []){"ip", "netns", "delete", vg, NULL})) {
		print_error("Can't delete ns");
		return -1;
	}
	if (run_cmd((char *const []){"iptables", "-D", "FORWARD", "-i", vh,
				     "-j", vg, NULL})) {
		print_error("Can't delete forward 1");
		return -1;
	}
	if (enable_net) {
		if (run_cmd((char *const []){"iptables", "-D", "FORWARD", "-o", vh,
					"-j", "ACCEPT", NULL})) {
			print_error("Can't delete forward 2");
			return -1;
		}
		if (run_cmd((char *const []){"iptables", "-t", "nat", "-D",
					"POSTROUTING", "-s", vgs, "-j",
					"MASQUERADE", NULL})) {
			print_error("Can't delete masq");
			return -1;
		}
	}
	if (run_cmd((char *const []){"iptables", "-F", vg, NULL})) {
		print_error("Can't flush iptables chain");
		return -1;
	}
	if (run_cmd((char *const []){"iptables", "-X", vg, NULL})) {
		print_error("Can't delete iptables chain");
		return -1;
	}
	return 0;
}

int set_netns(const unsigned char id)
{
	char path[32];
	int fd, ret;

	snprintf(path, sizeof(path), "/var/run/netns/" NETNS_PREFIX "1_%hhu", id);
	fd = open(path, O_RDONLY|O_CLOEXEC);
	if (fd == -1) {
		print_error("netns open impossible");
		return -1;
	}
	ret = setns(fd, CLONE_NEWNET);
	if (ret == -1)
		print_error("net setns impossible");
	close(fd);
	return ret;
}

int get_id_maps(unsigned long *new_uid_root,
		unsigned long *interval,
		pid_t init)
{
	FILE *f = NULL;
	int ret = -1;
	char *next;
	char buf[64];
	char vh[64];
	unsigned long uid;

	snprintf(vh, sizeof(vh), "/proc/%d/uid_map", init);
	f = fopen(vh, "r");
	if (f == NULL) {
		print_error("Can't open uid_map to get ids");
		goto exit;
	}
	while (fgets(buf, sizeof(buf), f)) {
		uid = strtoul(buf, &next, 10);
		if (uid == 0) {
			*new_uid_root = strtoul(next, &next, 10);
			*interval = strtoul(next, NULL, 10);
			break;
		}
	}
	ret = 0;
exit:
	if (f != NULL)
		fclose(f);
	return ret;
}

int setup_id_maps(unsigned long new_uid_root,
		  unsigned long interval,
		  pid_t child)
{
	int fd = -1;
	int ret = -1;
	char vh[100];

	snprintf(vh, sizeof(vh), "/proc/%d/uid_map", child);
	fd = open(vh, O_RDWR);
	if (fd == -1) {
		print_error("Can't open uid_map");
		goto exit;
	}
	if (dprintf(fd, "0 %lu %lu\n", new_uid_root, interval) == -1) {
		print_error("Can't dprintf uid_map");
		goto exit;
	}
	close(fd);
	fd = -1;

	snprintf(vh, sizeof(vh), "/proc/%d/gid_map", child);
	fd = open(vh, O_RDWR);
	if (fd == -1) {
		print_error("Can't open gid_map");
		goto exit;
	}
	if (dprintf(fd, "0 %lu %lu\n", new_uid_root, interval) == -1) {
		print_error("Can't dprintf gid_map");
		goto exit;
	}
	ret = 0;
exit:
	if (fd > 0)
		close(fd);
	return ret;
}
