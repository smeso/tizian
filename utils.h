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

#ifndef _TIZIAN_UTILS_H
#define _TIZIAN_UTILS_H

#include <errno.h>
#include <stdio.h>

enum verbosity {QUIET, NORMAL, VERBOSE};

extern enum verbosity global_verbosity;

#define LONG_PROGNAME "tizian"
#define PROGNAME "tznct"

static inline void print_error(const char *s) {
	if (global_verbosity == VERBOSE)
		perror(s);
}

#endif /* _TIZIAN_UTILS_H */
