======
tizian
======

tizian - better chrooting with containers

Install
*******

To install it run::

	make
	sudo make install

It's possible to disable `AppArmor <http://www.apparmor.net/>`_ and
`S.A.R.A. <https://smeso.it/sara>`_ by passing respectively
**NO_APPARMOR=1** and **NO_SARA=1** to make.

To uninstall it run::

	sudo make uninstall

If you are on Debian/Ubuntu, you can also build a .deb package::

	make deb
	sudo dpkg -i pkgs/tizian_*.deb

Usage
*****

::

	usage: tizian [-q|--quiet] create [-i|--ip-prefix IP_PREFIX] [-c|--cpu-shares CPU_SHARES]
					  [-C|--cpu-perc CPU_PERC] [-b|--blkio-weight BWEIGHT]
					  [-f|--max-fds MAX_FDS] [-p|--max-pids MAX_PIDS]
					  [-m|--max-mem MAX_MEM] [-n|--disable-net]
					  [-s|--disable-sara] [-I|--init-only]
					  [-M|--no-proc-mount] [-u|--uid UID] [-e|--cmd CMD]
					  [-E|--init-cmd INIT_CMD] [-P|--init-profile APPARMOR_PROFILE]
					  [-d|--intermediate-profile APPARMOR_PROFILE]
					  [-a|--cmd-profile APPARMOR_PROFILE]
					  [-U|--userns-slot SLOT] [-B|--background] <PATH>
	tizian [-q|--quiet] attach [-s|--disable-sara] [-u|--uid UID] [-e|--cmd CMD]
				   [-a|--cmd-profile CMD_APPARMOR_PROFILE] <ID>
	tizian [-q|--quiet] list
	tizian [-q|--quiet] delete <ID>
	tizian [-q|--quiet] ps <ID>
	tizian [-q|--quiet] wipe
	tizian [-h|--help]

	tizian: better chrooting with containers

	optional arguments:
	-h, --help					Show this help message and exit
	-q, --quiet					Suppress any output.
	-B, --background				run in background (implies -I and -q)
	-i, --ip-prefix IP_PREFIX			Set an IPv4 prefix in the format 'xxx.xxx.xxx.',
							it doesn't need to be unique across containers
							(default: '10.0.0.').
	-c, --cpu-shares CPU_SHARES			Minimum numbers of cpu shares (1024 == 1 cpu)
							for this container when system is busy (default:
							256).
	-C, --cpu-perc CPU_PERC				Maximum cpu usage percentage even when system is
							idle (default: 50).
	-b, --blkio-weight BWEIGHT			blkio CFQ weight (default: 0 - disabled).
	-f, --max-fds MAX_FDS				Maximum, per-process, number of open fds (default:
							4096).
	-p, --max-pids MAX_PIDS                         Maximum number of pids (default: 1024).
	-m, --max-mem MAX_MEM                           Max memory usage allowed, with swap, it gets doubled
							(default: 1073741824).
	-n, --disable-net				No network access.
	-s, --disable-sara				Do not enforce S.A.R.A. LSM memory protections.
	-I, --init-only					Only run init. Do not execute any command.
	-M, --no-proc-mount				Do not mount /proc and /dev/pts.
	-u, --uid UID					UID (default: 0).
	-e, --cmd CMD                                   The command to execute inside the container
							(default: the user shell according to /etc/passwd).
	-E, --init-cmd INIT_CMD				The init program to run (default: a minimal built-in
							init process).
	-P, --init-profile APPARMOR_PROFILE		AppArmor profile for init (default: 'tizian_init', use 'none' to
							disable).
	-d, --intermediate-profile APPARMOR_PROFILE	AppArmor profile for intermediate process (default:
							'tizian_intermediate', use 'none' to disable).
	-a, --cmd-profile APPARMOR_PROFILE		AppArmor profile for the command to execute (default: none).
	-U, --userns-slot SLOT				UID and GID map slot. Available slots are form 0 to 65536.
							The slot is multiplied by 65535 to get the UID/GID map
							for root (default: 0).

	Example:
		tizian create ~/chroot


::

	Usage:	tizian-chown-tree <SLOT> <PATH>
		tizian-chown-tree [-u|--undo] [SLOT] <PATH>
		tizian-chown-tree [-h|--help]

Copyright
*********

Salvatore Mesoraca - https://smeso.it

License
*******
This code is released under **GPL-3**.
