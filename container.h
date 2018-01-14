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

#ifndef _TIZIAN_CONTAINER_H
#define _TIZIAN_CONTAINER_H

#include "utils.h"

#define NEW_CONT_ID 255

struct container_config {
	unsigned short cpu_shares;
	unsigned short blkio_weight;
	unsigned int max_pids;
	unsigned int max_fds;
	unsigned long max_mem;
	unsigned char cpu_perc;
	unsigned char enable_net;
	unsigned char init_only;
	unsigned char proc_mount;
	unsigned char use_sara;
	unsigned char cont_id;
	char ip_prefix[17];
	uid_t uid;
	char **cmd;
	char **init_cmd;
	char *intermediate_profile;
	char *init_profile;
	char *exec_profile;
	unsigned long new_root_uid;
	unsigned long interval;
	int argc;
	char **argv;
	char chroot_path[4096];
};

int container(struct container_config *config) __attribute__((warn_unused_result));
void delete_container(const unsigned char cont_id);
ssize_t ps_container(const unsigned char cont_id, pid_t *pids, pid_t *nspids, size_t size) __attribute__((warn_unused_result));
ssize_t list_containers(unsigned char *ids, size_t size) __attribute__((warn_unused_result));
int container_exists(unsigned char cont_id);
void delete_all_containers(void);

#endif /* _TIZIAN_CONTAINER_H */
