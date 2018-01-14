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

#ifndef _TIZIAN_CGROUPS_H
#define _TIZIAN_CGROUPS_H

#include "utils.h"

#define CG_PREFIX PROGNAME "_cg_"

int setup_cgroups(unsigned char id,
		  unsigned long max_mem,
		  unsigned int max_pids,
		  unsigned char cpu_perc,
		  unsigned short cpu_shares,
		  unsigned short blkio_weight);
int set_cgroups(const unsigned char id);
int delete_cgroups(const unsigned char id);

#endif /* _TIZIAN_CGROUPS_H */
