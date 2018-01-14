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

#ifndef _TIZIAN_NAMESPACES_H
#define _TIZIAN_NAMESPACES_H

#include "utils.h"

#define NETNS_PREFIX PROGNAME "_veth"

int setup_netns(unsigned char *id, const char *ip_prefix, int enable_net);
int delete_netns(const unsigned char id, const char *ip_prefix, int enable_net);
int set_netns(const unsigned char id);
int get_id_maps(unsigned long *new_uid_root, unsigned long *interval, pid_t init);
int setup_id_maps(unsigned long new_uid_root, unsigned long interval, pid_t child);

#endif /* _TIZIAN_NAMESPACES_H */
