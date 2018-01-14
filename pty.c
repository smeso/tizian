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

#include <signal.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "utils.h"

pid_t global_child = -1;
int global_pt = 1;

int prepare_term(struct termios *terms)
{
	struct termios terms2;
	int ret;

	ret = tcgetattr(0, terms);
	if (ret)
		goto error;
	terms2 = *terms;
	cfmakeraw(&terms2);
	ret = tcsetattr(0, TCSAFLUSH, &terms2);
	if (ret)
		goto error;
	return 0;
error:
	print_error("error preparing terminal");
	return ret;
}

static void sync_winsize(int src, int dst)
{
	struct winsize wsize;

	if (ioctl(src, TIOCGWINSZ, &wsize) == 0) {
		ioctl(dst, TIOCSWINSZ, &wsize);
		if (global_child != -1)
			kill(global_child, SIGWINCH);
	}
}

void sigwinch(int s)
{
	int serrno = errno;

	sync_winsize(1, global_pt);
	errno = serrno;
}

int pty_manager(struct termios *terms, int pt)
{
	int ret = 0;
	char input[512];
	fd_set fd_in;

	sync_winsize(1, pt);
	global_pt = pt;

	while (1) {
		FD_ZERO(&fd_in);
		FD_SET(1, &fd_in);
		FD_SET(pt, &fd_in);

		ret = select(pt + 1, &fd_in, NULL, NULL, NULL);
		if (ret == -1 && errno != EINTR) {
			print_error("Can't select");
			break;
		}

		if (FD_ISSET(1, &fd_in)) {
			ret = read(1, input, sizeof(input));
			if (ret > 0) {
				if (write(pt, input, ret) != ret) {
					print_error("error on stdin");
					break;
				}
			} else if (errno == EIO)
				break;
			else if (ret < 0) {
				print_error("error on stdin");
				break;
			}
		}

		if (FD_ISSET(pt, &fd_in)) {
			ret = read(pt, input, sizeof(input));
			if (ret > 0) {
				if (write(0, input, ret) != ret) {
					print_error("error on stdin");
					break;
				}
			} else if (errno == EIO)
				break;
			else if (ret < 0) {
				print_error("error on stdout");
				break;
			}
		}
	}
	if (ret >= 0)
		ret = 0;
	return ret;
}
