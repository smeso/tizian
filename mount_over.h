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

#ifndef _TIZIAN_MOUNT_OVER_H
#define _TIZIAN_MOUNT_OVER_H


const char *mount_over_dirs[] = {"/proc/asound",
				 "/proc/bus",
				 "/proc/driver",
				 "/proc/driver",
				 "/proc/fs",
				 "/proc/irq",
				 "/proc/scsi",
				 "/proc/sys",
				 "/proc/sysvipc",
				 "/proc/tty",
				 NULL};


const char *mount_over_files[] = {"/proc/config.gz",
				  "/proc/cmdline",
				  "/proc/iomem",
				  "/proc/ioports",
				  "/proc/kallsyms",
				  "/proc/kcore",
				  "/proc/latency_stats",
				  "/proc/mtrr",
				  "/proc/sched_debug",
				  "/proc/slabinfo",
				  "/proc/softirqs",
				  "/proc/sysrq-trigger",
				  "/proc/timer_list",
				  "/proc/timer_stats",
				  "/proc/version",
				  "/proc/version_signature",
				  NULL};


#endif /* _TIZIAN_MOUNT_OVER_H */
