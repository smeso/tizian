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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

#define TMP_DIR_MOUNT "/tmp/mnttmp.XXXXXXX"
#define TMP_DIR_OLD_ROOT "/old_root.XXXXXX"

static int pivot_root(const char *new_root, const char *put_old)
{
	return syscall(SYS_pivot_root, new_root, put_old);
}

int pre_mount_proc(const char *new_root, int proc_mount, uid_t new_root_uid)
{
	char tmp_new_root[] = TMP_DIR_MOUNT;
	char old_root[] = TMP_DIR_MOUNT TMP_DIR_OLD_ROOT;
	char new_in_old_root[] = TMP_DIR_OLD_ROOT TMP_DIR_MOUNT;
	struct stat s;
	char mode_buf[128];
	int ignored __attribute__((unused));

	if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
		print_error("Can't remount / as private");
		return -1;
	}

	if (!mkdtemp(tmp_new_root)) {
		print_error("Can't make new root tmpdir");
		return -1;
	}
	memcpy(old_root, tmp_new_root, strlen(tmp_new_root));

	if (mount(new_root, tmp_new_root, NULL, MS_REC | MS_BIND | MS_PRIVATE, NULL)) {
		print_error("Can't mount new root");
		return -1;
	}

	if (!mkdtemp(old_root)) {
		print_error("Can't make old root tmpdir");
		return -1;
	}
	memcpy(new_in_old_root,
	       old_root+sizeof(TMP_DIR_MOUNT)-1,
	       sizeof(TMP_DIR_OLD_ROOT));
	memcpy(new_in_old_root+sizeof(TMP_DIR_OLD_ROOT)-1,
	       tmp_new_root,
	       strlen(tmp_new_root));

	if (pivot_root(tmp_new_root, old_root)) {
		print_error("Can't pivot root");
		return -1;
	}

	if (chdir("/")) {
		print_error("Can't chdir in new root");
		return -1;
	}

	if (rmdir(new_in_old_root)) {
		print_error("Can't rmdir new root tmpdir");
		return -1;
	}

	if (umount2(old_root+sizeof(TMP_DIR_MOUNT)-1, MNT_DETACH)) {
		print_error("Can't umount old root");
		return -1;
	}

	if (rmdir(old_root+sizeof(TMP_DIR_MOUNT)-1)) {
		print_error("Can't rmdir old root tmpdir");
		return -1;
	}

	if (!proc_mount)
		return 0;

	mkdir("/dev", 0755);
	mkdir("/dev/pts", 0755);
	mkdir("/proc", 0555);
	mkdir("/etc", 0755);
	unlink("/dev/ptmx");
	ignored = symlink("/dev/pts/ptmx", "/dev/ptmx");
	mknod("/dev/null", 0666, makedev(1, 3));
	mknod("/dev/zero", 0666, makedev(1, 5));
	mknod("/dev/full", 0666, makedev(1, 7));
	mknod("/dev/random", 0666, makedev(1, 8));
	mknod("/dev/urandom", 0666, makedev(1, 9));
	mknod("/dev/tty", 0666, makedev(5, 0));

	if (stat("/dev/tty", &s)) {
		print_error("Can't stat /dev/tty");
		return -1;
	}
	snprintf(mode_buf, sizeof(mode_buf),
		 "uid=%d,gid=%d,mode=0620,ptmxmode=0666,newinstance",
		  new_root_uid,
		  s.st_gid);
	if (mount("devpts", "/dev/pts", "devpts", MS_NOSUID|MS_NOEXEC, mode_buf)) {
		print_error("devpts impossible");
		return -1;
	}
	return 0;
}

int mount_proc(uid_t new_root_uid)
{
	int fd1, fd2, ret = 0;
	char mode_buf[128];

	snprintf(mode_buf, sizeof(mode_buf),
		 "gid=%d,hidepid=2",
		  new_root_uid);
	if (mount("proc", "/proc", "proc", MS_NOSUID|MS_NOEXEC|MS_NODEV, mode_buf)) {
		print_error("proc mount impossible");
		return -1;
	}

	fd1 = open("/proc/mounts", O_RDONLY);
	if (fd1 == -1) {
		print_error("Can't open /proc/mounts");
		return -1;
	}
	unlink("/etc/mtab");
	fd2 = open("/etc/mtab", O_TRUNC|O_CREAT|O_WRONLY, 0644);
	if (fd2 == -1) {
		print_error("Can't open /etc/mtab");
		ret = -1;
		goto out_fd1;
	}

	if (sendfile(fd2, fd1, NULL, UINT_MAX) == -1) {
		print_error("Can't sendfile /etc/mtab");
		ret = -1;
		goto out;
	}
out:
	close(fd2);
out_fd1:
	close(fd1);
	return ret;
}
