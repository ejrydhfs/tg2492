# Copyright (C) 2006, 2019 Intel Corporation
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# SPDX-License-Identifier: BSD-3-Clause

# udhcp makefile
-include $(TARGET_HOME)/.config
-include $(TARGET_HOME)/TI_Build.make

prefix=$(TI_filesystem_path)
SBINDIR=/sbin
USRSBINDIR=${prefix}/sbin
USRBINDIR=${prefix}/bin
USRSHAREDIR=${prefix}/share
VARTMP=${prefix}/var/tmp

# Uncomment this to get a shared binary. Call as udhcpd for the server,
# and udhcpc for the client

ifeq ($(CONFIG_TI_UDHCP_CLIENT),y) 
ifeq ($(CONFIG_TI_UDHCP_SERVER),y)
COMBINED_BINARY=1
endif
endif

# Uncomment this for extra output and to compile with debugging symbols
#DEBUG=1

# Uncomment this to output messages to syslog, otherwise, messages go to stdout
CFLAGS += -Os

#CROSS_COMPILE=arm-uclibc-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
INSTALL = install

VER := 0.9.7


OBJS_SHARED = options.o socket.o packet.o pidfile.o
DHCPD_OBJS = dhcpd.o arpping.o files.o leases.o serverpacket.o
DHCPC_OBJS = dhcpc.o clientpacket.o script.o

# UNIHAN ADD START
# To support static lease
DHCPD_OBJS += static_leases.o
# UNIHAN ADD END

ifdef COMBINED_BINARY
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(DHCPC_OBJS) $(OBJS_SHARED) frontend.o
CFLAGS += -DCOMBINED_BINARY
else
EXEC1 = udhcpd
OBJS1 = $(DHCPD_OBJS) $(OBJS_SHARED)

EXEC2 = udhcpc
OBJS2 = $(DHCPC_OBJS) $(OBJS_SHARED)
endif

EXEC3 = dumpleases
OBJS3 = dumpleases.o

BOOT_PROGRAMS = udhcpc
DAEMONS = udhcpd
COMMANDS = dumpleases

ifdef SYSLOG
CFLAGS += -DSYSLOG
endif

CFLAGS += -W -Wall -Wstrict-prototypes -DVERSION='"$(VER)"' 

ifdef DEBUG
CFLAGS += -g -DDEBUG
else
CFLAGS += -Os -fomit-frame-pointer
#STRIP=-s
STRIP=
endif

LDFLAGS += -lresolv

# ARRIS ADD
ifeq ($(CONFIG_TI_DMALLOC_UTIL),y)
#un comment the below lines to enable dmalloc for this process
#CFLAGS += -DCONFIG_DMALLOC_ARRIS
#LDFLAGS +=   -ldmallocth
endif
# END ARRIS ADD


all: $(EXEC1) $(EXEC2) $(EXEC3)

$(OBJS1) $(OBJS2) $(OBJS3): *.h Makefile
$(EXEC1) $(EXEC2) $(EXEC3): Makefile

.c.o:
	$(CC) -c $(CFLAGS) $<

$(EXEC1): $(OBJS1)
	$(LD) $(LDFLAGS) $(OBJS1) -o $(EXEC1)

$(EXEC2): $(OBJS2)
	$(LD) $(LDFLAGS) $(OBJS2) -o $(EXEC2)

$(EXEC3): $(OBJS3)
	$(LD) $(LDFLAGS) $(OBJS3) -o $(EXEC3)


install: all

	$(INSTALL) $(STRIP) $(DAEMONS) $(USRSBINDIR)
	$(INSTALL) dhcp_fingerprints.conf $(VARTMP)
ifdef COMBINED_BINARY
	(cd $(USRSBINDIR) ; ln -sf ./$(DAEMONS) $(BOOT_PROGRAMS))
else
ifeq ($(CONFIG_TI_UDHCP_CLIENT),y) 
	$(INSTALL) $(STRIP) $(BOOT_PROGRAMS) $(USRSBINDIR)
endif
endif

ifeq ($(CONFIG_TI_UDHCP_CLIENT),y) 
	mkdir -p $(USRSHAREDIR)/udhcpc
	for name in bound deconfig renew script altbound; do \
		$(INSTALL) samples/sample.$$name \
			$(USRSHAREDIR)/udhcpc/default.$$name ; \
	done
endif

clean:
	-rm -f udhcpd udhcpc dumpleases *.o core
	-rm -f $(USRSBINDIR)/udhcpd
	-rm -f $(USRSBINDIR)/udhcpc
ifeq ($(CONFIG_TI_UDHCP_CLIENT),y)
	for name in bound deconfig renew script altbound; do \
	$(RM) $(USRSHAREDIR)/udhcpc/default.$$name ; \
    done
endif

