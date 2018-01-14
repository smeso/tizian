#!/usr/bin/make
#
#     tizian - better chrooting with containers
#     Copyright (C) 2017  Salvatore Mesoraca <s.mesoraca16@gmail.com>
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

ifndef DESTDIR
DESTDIR := /
endif
ifndef BINDIR
BINDIR := usr/sbin/
endif

PROGNAME := tizian
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
CFLAGS := -Wall -pedantic -std=gnu99 -O2 -fstack-protector -fPIE $(CFLAGS)
LDFLAGS := -s -Wl,-z,relro -Wl,-z,now -Wl,-Bsymbolic-functions -pie -lseccomp -lcap -lutil $(LDFLAGS)
ifndef NO_APPARMOR
LDFLAGS := $(LDFLAGS) -lapparmor
else
CFLAGS := $(CFLAGS) -DNO_APPARMOR
endif
ifndef NO_SARA
LDFLAGS := $(LDFLAGS) -lsara
else
CFLAGS := $(CFLAGS) -DNO_SARA
endif
BIN := ./
SOURCE := ./

all: $(BIN)$(PROGNAME)

$(SOURCE)%.o: $(SOURCE)%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN)$(PROGNAME):	$(SOURCE)cgroups.o \
			$(SOURCE)container.o \
			$(SOURCE)mount.o \
			$(SOURCE)namespaces.o \
			$(SOURCE)pty.o \
			$(SOURCE)user_setup.o \
			$(SOURCE)main.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	-rm -f $(SOURCE)*.o $(SOURCE)*~
	-rm -f *.o *~
	-rm -f debian/*~

distclean: clean
	-rm -f $(BIN)$(PROGNAME)
	-rm -rf pkgs/

install: all
	mkdir -p $(DESTDIR)/$(BINDIR)
ifndef NO_APPARMOR
	mkdir -p $(DESTDIR)/etc/apparmor.d/tunables
endif
	cp $(BIN)$(PROGNAME) $(DESTDIR)/$(BINDIR)
	cp $(SRC)$(PROGNAME)-chown-tree $(DESTDIR)/$(BINDIR)
ifndef NO_APPARMOR
	cp $(SRC)apparmor $(DESTDIR)/etc/apparmor.d/$(PROGNAME)
	cp $(SRC)apparmor.tunables $(DESTDIR)/etc/apparmor.d/tunables/$(PROGNAME)
endif
	chmod 755 $(DESTDIR)/$(BINDIR)/$(PROGNAME)
	chmod 755 $(DESTDIR)/$(BINDIR)/$(PROGNAME)-chown-tree

uninstall:
	-rm -f $(DESTDIR)/$(BINDIR)/$(PROGNAME)
	-rm -f $(DESTDIR)/$(BINDIR)/$(PROGNAME)-chown-tree

deb: distclean
	dpkg-buildpackage -b -uc -tc
	mkdir -p pkgs
	mv ../$(PROGNAME)_*.deb pkgs/
	mv ../$(PROGNAME)_*.changes pkgs/
	lintian pkgs/$(PROGNAME)_* || true

tar: distclean
	mkdir -p pkgs
	git archive --format=tar.gz --prefix=$(PROGNAME)/ master -o pkgs/$(PROGNAME).tar.gz

.PHONY: all install uninstall clean distclean deb tar
