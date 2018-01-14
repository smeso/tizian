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

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cgroups.h"
#include "utils.h"

#define MAJOR_NETCLS_HANDLE 0x12340000

#define WRITE_CG_CONF(RESTYPE, S) do {					\
	f = fopen(cgf, "w");						\
	if (f == NULL) {						\
		print_error("Can't configure " RESTYPE " cgroup");	\
		return -1;						\
	}								\
	fwrite(S, strlen(S), 1, f);					\
	fclose(f);							\
} while (0)

#define CREATE_CGGROUP(RESTYPE) do {						\
	snprintf(cg, sizeof(cg), "/sys/fs/cgroup/" RESTYPE "/" CG_PREFIX "%hhu", id);	\
	if (mkdir(cg, 0755)) {							\
		print_error("Can't create " RESTYPE " cgroup");			\
		return -1;							\
	}									\
} while (0)

int setup_cgroups(unsigned char id,
		  unsigned long max_mem,
		  unsigned int max_pids,
		  unsigned char cpu_perc,
		  unsigned short cpu_shares,
		  unsigned short blkio_weight)
{
	char cg[64];
	char cgf[96];
	char tmp[64];
	FILE *f;

	CREATE_CGGROUP("devices");
	snprintf(cgf, sizeof(cgf), "%s/devices.deny", cg);
	WRITE_CG_CONF("devices", "a\n");
	snprintf(cgf, sizeof(cgf), "%s/devices.allow", cg);
	WRITE_CG_CONF("devices", "c 1:3 rwm\n");  /* /dev/null */
	WRITE_CG_CONF("devices", "c 1:5 rwm\n");  /* /dev/zero */
	WRITE_CG_CONF("devices", "c 1:7 rwm\n");  /* /dev/full */
	WRITE_CG_CONF("devices", "c 1:8 rwm\n");  /* /dev/random */
	WRITE_CG_CONF("devices", "c 1:9 rwm\n");  /* /dev/urandom */
// 	WRITE_CG_CONF("devices", "c 1:11 rwm\n"); /* /dev/kmsg */
	WRITE_CG_CONF("devices", "c 5:0 rwm\n");  /* /dev/tty */
	WRITE_CG_CONF("devices", "c 5:2 rw\n");  /* /dev/ptmx */
	WRITE_CG_CONF("devices", "c 136:* rw\n"); /* /dev/pts/ */
	CREATE_CGGROUP("cpu");
	snprintf(tmp, sizeof(tmp), "%lu", cpu_perc*1000ul);
	snprintf(cgf, sizeof(cgf), "%s/cpu.cfs_period_us", cg);
	WRITE_CG_CONF("cpu.cfs_period_us", "100000");
	snprintf(cgf, sizeof(cgf), "%s/cpu.cfs_quota_us", cg);
	WRITE_CG_CONF("cpu.cfs_quota_us", tmp);
	snprintf(cgf, sizeof(cgf), "%s/cpu.rt_runtime_us", cg);
	WRITE_CG_CONF("cpu.rt_runtime_us", "0");
	snprintf(tmp, sizeof(tmp), "%uh", cpu_shares);
	snprintf(cgf, sizeof(cgf), "%s/cpu.shares", cg);
	WRITE_CG_CONF("cpu.shares", tmp);
	CREATE_CGGROUP("memory");
	snprintf(tmp, sizeof(tmp), "%lu", max_mem);
	snprintf(cgf, sizeof(cgf), "%s/memory.limit_in_bytes", cg);
	WRITE_CG_CONF("memory.limit_in_bytes", tmp);
	snprintf(cgf, sizeof(cgf), "%s/memory.soft_limit_in_bytes", cg);
	WRITE_CG_CONF("memory.soft_limit_in_bytes", tmp);
	snprintf(cgf, sizeof(cgf), "%s/memory.kmem.limit_in_bytes", cg);
	WRITE_CG_CONF("memory.kmem.limit_in_bytes", tmp);
	snprintf(cgf, sizeof(cgf), "%s/memory.kmem.tcp.limit_in_bytes", cg);
	WRITE_CG_CONF("memory.kmem.tcp.limit_in_bytes", tmp);
	snprintf(tmp, sizeof(tmp), "%lu", max_mem*2);
	snprintf(cgf, sizeof(cgf), "%s/memory.memsw.limit_in_bytes", cg);
	WRITE_CG_CONF("memory.memsw.limit_in_bytes", tmp);
	CREATE_CGGROUP("net_cls");
	snprintf(tmp, sizeof(tmp), "0x%x", MAJOR_NETCLS_HANDLE | id);
	snprintf(cgf, sizeof(cgf), "%s/net_cls.classid", cg);
	WRITE_CG_CONF("net_cls", tmp);
	CREATE_CGGROUP("pids");
	snprintf(tmp, sizeof(tmp), "%u", max_pids);
	snprintf(cgf, sizeof(cgf), "%s/pids.max", cg);
	WRITE_CG_CONF("pids", tmp);
	CREATE_CGGROUP("blkio");
	if (blkio_weight != 0) {
		snprintf(tmp, sizeof(tmp), "%uh", blkio_weight);
		snprintf(cgf, sizeof(cgf), "%s/weight", cg);
		WRITE_CG_CONF("blkio", tmp);
	}
	CREATE_CGGROUP("freezer");
	CREATE_CGGROUP("perf_event");
	CREATE_CGGROUP("net_prio");
//	CREATE_CGGROUP("hugetlb");
//	CREATE_CGGROUP("rdma");

	return 0;
}

#define RM_CGROUP(RESTYPE) do {							\
	snprintf(cg, sizeof(cg), "/sys/fs/cgroup/" RESTYPE "/" CG_PREFIX "%hhu", id);	\
	if (rmdir(cg)) {							\
		print_error("Can't delete " RESTYPE " cgroup");			\
		return -1;							\
	}									\
} while (0)

int delete_cgroups(const unsigned char id)
{
	char cg[64];
	char cgf[64];
	FILE *f;

	RM_CGROUP("devices");
	RM_CGROUP("cpu");
	snprintf(cgf, sizeof(cgf), "/sys/fs/cgroup/memory/" CG_PREFIX "%hhu/memory.force_empty", id);
	WRITE_CG_CONF("memory", "0");
	RM_CGROUP("memory");
	RM_CGROUP("freezer");
	RM_CGROUP("net_cls");
	RM_CGROUP("blkio");
	RM_CGROUP("net_prio");
	RM_CGROUP("pids");
	RM_CGROUP("perf_event");
//	RM_CGROUP("hugetlb");
//	RM_CGROUP("rdma");

	return 0;
}

#define SET_CGROUP(RESTYPE) do {								\
	snprintf(cg, sizeof(cg), "/sys/fs/cgroup/" RESTYPE "/" CG_PREFIX "%hhu/cgroup.procs", id);	\
	f = fopen(cg, "w");									\
	if (f == NULL) {									\
		print_error("Can't set " RESTYPE " cgroup");					\
		return -1;									\
	}											\
	fwrite("0\n", 2, 1, f);									\
	fclose(f);										\
} while (0)

int set_cgroups(const unsigned char id)
{
	char cg[64];
	FILE *f;

	SET_CGROUP("devices");
	SET_CGROUP("cpu");
	SET_CGROUP("memory");
	SET_CGROUP("freezer");
	SET_CGROUP("net_cls");
	SET_CGROUP("blkio");
	SET_CGROUP("net_prio");
	SET_CGROUP("pids");
	SET_CGROUP("perf_event");
//	SET_CGROUP("hugetlb");
//	SET_CGROUP("rdma");

	return 0;
}
