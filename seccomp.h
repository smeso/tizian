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

#ifndef _TIZIAN_SECCOMP_H
#define _TIZIAN_SECCOMP_H

#include <seccomp.h>
#include <sys/socket.h>

#ifndef __NR_copy_file_range
#define __NR_copy_file_range __NR_sendfile
#endif

#ifndef __NR_preadv2
#define __NR_preadv2 __NR_preadv
#endif

#ifndef __NR_pwritev2
#define __NR_pwritev2 __NR_pwritev
#endif

int seccomp_syscalls[] = {SCMP_SYS(_llseek), SCMP_SYS(_newselect), SCMP_SYS(accept),
			  SCMP_SYS(accept4), SCMP_SYS(access), SCMP_SYS(adjtimex),
			  SCMP_SYS(alarm), SCMP_SYS(arch_prctl), SCMP_SYS(arm_fadvise64_64),
			  SCMP_SYS(arm_sync_file_range), SCMP_SYS(bind), SCMP_SYS(bpf),
			  SCMP_SYS(brk), SCMP_SYS(cacheflush), SCMP_SYS(capget),
			  SCMP_SYS(capset), SCMP_SYS(chdir), SCMP_SYS(chmod),
			  SCMP_SYS(chown), SCMP_SYS(chown32), SCMP_SYS(chroot),
			  SCMP_SYS(clock_getres), SCMP_SYS(clock_gettime), SCMP_SYS(clock_nanosleep),
			  SCMP_SYS(close), SCMP_SYS(connect), SCMP_SYS(copy_file_range),
			  SCMP_SYS(creat), SCMP_SYS(dup), SCMP_SYS(dup2),
			  SCMP_SYS(dup3), SCMP_SYS(epoll_create), SCMP_SYS(epoll_create1),
			  SCMP_SYS(epoll_ctl), SCMP_SYS(epoll_ctl_old), SCMP_SYS(epoll_pwait),
			  SCMP_SYS(epoll_wait), SCMP_SYS(epoll_wait_old), SCMP_SYS(eventfd),
			  SCMP_SYS(eventfd2), SCMP_SYS(execve), SCMP_SYS(execveat),
			  SCMP_SYS(exit), SCMP_SYS(exit_group), SCMP_SYS(faccessat),
			  SCMP_SYS(fadvise64), SCMP_SYS(fadvise64_64), SCMP_SYS(fallocate),
			  SCMP_SYS(fanotify_init), SCMP_SYS(fanotify_mark), SCMP_SYS(fchdir),
			  SCMP_SYS(fchmod), SCMP_SYS(fchmodat), SCMP_SYS(fchown),
			  SCMP_SYS(fchown32), SCMP_SYS(fchownat), SCMP_SYS(fcntl),
			  SCMP_SYS(fcntl64), SCMP_SYS(fdatasync), SCMP_SYS(fgetxattr),
			  SCMP_SYS(flistxattr), SCMP_SYS(flock), SCMP_SYS(fork),
			  SCMP_SYS(fremovexattr), SCMP_SYS(fsetxattr), SCMP_SYS(fstat),
			  SCMP_SYS(fstat64), SCMP_SYS(fstatat64), SCMP_SYS(fstatfs),
			  SCMP_SYS(fstatfs64), SCMP_SYS(fsync), SCMP_SYS(ftruncate),
			  SCMP_SYS(ftruncate64), SCMP_SYS(futex), SCMP_SYS(futimesat),
			  SCMP_SYS(get_robust_list), SCMP_SYS(get_thread_area), SCMP_SYS(getcpu),
			  SCMP_SYS(getcwd), SCMP_SYS(getdents), SCMP_SYS(getdents64),
			  SCMP_SYS(getegid), SCMP_SYS(getegid32), SCMP_SYS(geteuid),
			  SCMP_SYS(geteuid32), SCMP_SYS(getgid), SCMP_SYS(getgid32),
			  SCMP_SYS(getgroups), SCMP_SYS(getgroups32), SCMP_SYS(getitimer),
			  SCMP_SYS(getpeername), SCMP_SYS(getpgid), SCMP_SYS(getpgrp),
			  SCMP_SYS(getpid), SCMP_SYS(getppid), SCMP_SYS(getpriority),
			  SCMP_SYS(getrandom), SCMP_SYS(getresgid), SCMP_SYS(getresgid32),
			  SCMP_SYS(getresuid), SCMP_SYS(getresuid32), SCMP_SYS(getrlimit),
			  SCMP_SYS(getrusage), SCMP_SYS(getsid), SCMP_SYS(getsockname),
			  SCMP_SYS(getsockopt), SCMP_SYS(gettid), SCMP_SYS(gettimeofday),
			  SCMP_SYS(getuid), SCMP_SYS(getuid32), SCMP_SYS(getxattr),
			  SCMP_SYS(inotify_add_watch), SCMP_SYS(inotify_init), SCMP_SYS(inotify_init1),
			  SCMP_SYS(inotify_rm_watch), SCMP_SYS(io_cancel), SCMP_SYS(io_destroy),
			  SCMP_SYS(io_getevents), SCMP_SYS(io_setup), SCMP_SYS(io_submit),
			  SCMP_SYS(ioctl), SCMP_SYS(ioperm), SCMP_SYS(iopl),
			  SCMP_SYS(ioprio_get), SCMP_SYS(ioprio_set), SCMP_SYS(ipc),
			  SCMP_SYS(kcmp), SCMP_SYS(kill), SCMP_SYS(lchown),
			  SCMP_SYS(lchown32), SCMP_SYS(lgetxattr), SCMP_SYS(link),
			  SCMP_SYS(linkat), SCMP_SYS(listen), SCMP_SYS(listxattr),
			  SCMP_SYS(llistxattr), SCMP_SYS(lookup_dcookie), SCMP_SYS(lremovexattr),
			  SCMP_SYS(lseek), SCMP_SYS(lsetxattr), SCMP_SYS(lstat),
			  SCMP_SYS(lstat64), SCMP_SYS(madvise), SCMP_SYS(memfd_create),
			  SCMP_SYS(mincore), SCMP_SYS(mkdir), SCMP_SYS(mkdirat),
			  SCMP_SYS(mknod), SCMP_SYS(mknodat), SCMP_SYS(mlock),
			  SCMP_SYS(mlock2), SCMP_SYS(mlockall), SCMP_SYS(mmap),
			  SCMP_SYS(mmap2), SCMP_SYS(mprotect), SCMP_SYS(mq_getsetattr),
			  SCMP_SYS(mq_notify), SCMP_SYS(mq_open), SCMP_SYS(mq_timedreceive),
			  SCMP_SYS(mq_timedsend), SCMP_SYS(mq_unlink), SCMP_SYS(mremap),
			  SCMP_SYS(msgctl), SCMP_SYS(msgget), SCMP_SYS(msgrcv),
			  SCMP_SYS(msgsnd), SCMP_SYS(msync), SCMP_SYS(munlock),
			  SCMP_SYS(munlockall), SCMP_SYS(munmap), SCMP_SYS(name_to_handle_at),
			  SCMP_SYS(nanosleep), SCMP_SYS(newfstatat), SCMP_SYS(open),
			  SCMP_SYS(openat), SCMP_SYS(pause), SCMP_SYS(pipe),
			  SCMP_SYS(pipe2), SCMP_SYS(poll), SCMP_SYS(ppoll),
			  SCMP_SYS(prctl), SCMP_SYS(pread64), SCMP_SYS(preadv),
			  SCMP_SYS(preadv2), SCMP_SYS(prlimit64), SCMP_SYS(process_vm_readv),
			  SCMP_SYS(process_vm_writev), SCMP_SYS(pselect6),
			  SCMP_SYS(pwrite64), SCMP_SYS(pwritev), SCMP_SYS(pwritev2),
			  SCMP_SYS(read), SCMP_SYS(readahead), SCMP_SYS(readlink),
			  SCMP_SYS(readlinkat), SCMP_SYS(readv), SCMP_SYS(recv),
			  SCMP_SYS(recvfrom), SCMP_SYS(recvmmsg), SCMP_SYS(recvmsg),
			  SCMP_SYS(remap_file_pages), SCMP_SYS(removexattr), SCMP_SYS(rename),
			  SCMP_SYS(renameat), SCMP_SYS(renameat2), SCMP_SYS(restart_syscall),
			  SCMP_SYS(rmdir), SCMP_SYS(rt_sigaction), SCMP_SYS(rt_sigpending),
			  SCMP_SYS(rt_sigprocmask), SCMP_SYS(rt_sigqueueinfo), SCMP_SYS(rt_sigreturn),
			  SCMP_SYS(rt_sigsuspend), SCMP_SYS(rt_sigtimedwait), SCMP_SYS(rt_tgsigqueueinfo),
			  SCMP_SYS(sched_get_priority_max), SCMP_SYS(sched_get_priority_min),
			  SCMP_SYS(sched_getaffinity), SCMP_SYS(sched_getattr), SCMP_SYS(sched_getparam),
			  SCMP_SYS(sched_getscheduler), SCMP_SYS(sched_rr_get_interval),
			  SCMP_SYS(sched_setaffinity), SCMP_SYS(sched_setattr), SCMP_SYS(sched_setparam),
			  SCMP_SYS(sched_setscheduler), SCMP_SYS(sched_yield),
			  SCMP_SYS(seccomp), SCMP_SYS(select), SCMP_SYS(semctl),
			  SCMP_SYS(semget), SCMP_SYS(semop), SCMP_SYS(semtimedop),
			  SCMP_SYS(send), SCMP_SYS(sendfile), SCMP_SYS(sendfile64),
			  SCMP_SYS(sendmmsg), SCMP_SYS(sendmsg), SCMP_SYS(sendto),
			  SCMP_SYS(set_robust_list), SCMP_SYS(set_thread_area), SCMP_SYS(set_tid_address),
			  SCMP_SYS(setdomainname), SCMP_SYS(setfsgid), SCMP_SYS(setfsgid32),
			  SCMP_SYS(setfsuid), SCMP_SYS(setfsuid32), SCMP_SYS(setgid),
			  SCMP_SYS(setgid32), SCMP_SYS(setgroups), SCMP_SYS(setgroups32),
			  SCMP_SYS(sethostname), SCMP_SYS(setitimer), SCMP_SYS(setns),
			  SCMP_SYS(setpgid), SCMP_SYS(setpriority), SCMP_SYS(setregid),
			  SCMP_SYS(setregid32), SCMP_SYS(setresgid), SCMP_SYS(setresgid32),
			  SCMP_SYS(setresuid), SCMP_SYS(setresuid32), SCMP_SYS(setreuid),
			  SCMP_SYS(setreuid32), SCMP_SYS(setrlimit), SCMP_SYS(setsid),
			  SCMP_SYS(setsockopt), SCMP_SYS(setuid), SCMP_SYS(setuid32),
			  SCMP_SYS(setxattr), SCMP_SYS(shmat), SCMP_SYS(shmctl),
			  SCMP_SYS(shmdt), SCMP_SYS(shmget), SCMP_SYS(shutdown),
			  SCMP_SYS(sigaltstack), SCMP_SYS(signalfd), SCMP_SYS(signalfd4),
			  SCMP_SYS(sigreturn), SCMP_SYS(socketcall), SCMP_SYS(socketpair),
			  SCMP_SYS(splice), SCMP_SYS(stat), SCMP_SYS(stat64),
			  SCMP_SYS(statfs), SCMP_SYS(statfs64), SCMP_SYS(symlink),
			  SCMP_SYS(symlinkat), SCMP_SYS(sync), SCMP_SYS(sync_file_range),
			  SCMP_SYS(sync_file_range2), SCMP_SYS(syncfs), SCMP_SYS(sysinfo),
			  SCMP_SYS(tee), SCMP_SYS(tgkill), SCMP_SYS(time),
			  SCMP_SYS(timer_create), SCMP_SYS(timer_delete), SCMP_SYS(timer_getoverrun),
			  SCMP_SYS(timer_gettime), SCMP_SYS(timer_settime), SCMP_SYS(timerfd_create),
			  SCMP_SYS(timerfd_gettime), SCMP_SYS(timerfd_settime), SCMP_SYS(times),
			  SCMP_SYS(tkill), SCMP_SYS(truncate), SCMP_SYS(truncate64),
			  SCMP_SYS(ugetrlimit), SCMP_SYS(umask), SCMP_SYS(umount),
			  SCMP_SYS(umount2), SCMP_SYS(uname), SCMP_SYS(unlink),
			  SCMP_SYS(unlinkat), SCMP_SYS(utime), SCMP_SYS(utimensat),
			  SCMP_SYS(utimes), SCMP_SYS(vfork), SCMP_SYS(vmsplice),
			  SCMP_SYS(wait4), SCMP_SYS(waitid), SCMP_SYS(waitpid),
			  SCMP_SYS(write), SCMP_SYS(writev)};

int seccomp_syscalls_short[] = {SCMP_SYS(wait4), SCMP_SYS(waitid), SCMP_SYS(waitpid),
				SCMP_SYS(exit), SCMP_SYS(exit_group), SCMP_SYS(rt_sigreturn),
				SCMP_SYS(sigreturn)};

int socket_whitelist[] = {AF_NETLINK,
			  AF_UNIX,
			  AF_INET,
			  AF_INET6};

#endif /* _TIZIAN_SECCOMP_H */
