# Copyright (C) 1998-2010, 2019 Intel Corporation
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

# *************************************************************************
# * Makefile Wrapper file for building quagga package
# *************************************************************************

#include $(TARGET_HOME)/TI_Build.make
include $(TARGET_HOME)/.config

PACKAGE_VERSION := 0.99.16
PACKAGE_DIR := quagga-$(PACKAGE_VERSION)
PACKAGE_CONF := \
	--host=$(patsubst %-gcc,%,$(CROSS)gcc) \
	--with-cflags="-Drpl_malloc=malloc -Drpl_realloc=realloc $(CFLAGS_basic) " \
	--enable-user=root --enable-group=root --enable-vty-group=root \
	--disable-largefile --disable-pie \
	--localstatedir=/var --exec-prefix=$(TI_filesystem_path) --prefix=/ \
	--disable-doc --enable-ipv6 --enable-rtadv

ifndef CONFIG_TI_QUAGGA_BGP
PACKAGE_CONF += --disable-bgpd
endif

ifndef CONFIG_TI_QUAGGA_RIP
PACKAGE_CONF += --disable-ripd
endif

ifndef CONFIG_TI_QUAGGA_RIPNG
PACKAGE_CONF += --disable-ripngd
endif

ifndef CONFIG_TI_QUAGGA_OSPF
PACKAGE_CONF += --disable-ospfd
endif

ifndef CONFIG_TI_QUAGGA_OSPF6
PACKAGE_CONF += --disable-ospf6d
endif

ifndef CONFIG_TI_QUAGGA_WATCH
PACKAGE_CONF += --disable-watchquagga
endif

export LDFLAGS += $(LDLAGS_optimize)

first_rule: all

all: build

$(CURDIR)/$(PACKAGE_DIR)/Makefile:
	@# ARRIS CHANGE
	@cd $(CURDIR)/$(PACKAGE_DIR) && autoreconf -fi
	@cd $(CURDIR)/$(PACKAGE_DIR) && CC=$(CROSS)gcc ./configure INSTALL="/usr/bin/install -p -c" $(PACKAGE_CONF)
	@# END ARRIS CHANGE

build: $(CURDIR)/$(PACKAGE_DIR)/Makefile
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) all

config_rule:

install_rule:
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) install prefix=$(TARGET_HOME)/ti
	@if [ -f $(TI_filesystem_path)/lib/libzebra.a ]; then mv -f $(TI_filesystem_path)/lib/libzebra.a $(TARGET_HOME)/ti/lib/libzebra_pic.a; fi;	 
	mkdir -p $(TI_filesystem_path)$(TI_etcdir)/default 
	@install -m 0644 $(CURDIR)/zebra.conf $(TI_filesystem_path)/etc/default/
ifdef CONFIG_TI_QUAGGA_RIP
	@install -m 0644 $(CURDIR)/ripd.conf $(TI_filesystem_path)/etc/default/
endif
ifdef CONFIG_TI_QUAGGA_RIPNG
	@install -m 0644 $(CURDIR)/ripngd.conf $(TI_filesystem_path)/etc/default/
endif

clean_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) distclean -i; fi
	@rm -f $(TARGET_HOME)/ti/lib/libzebra_pic.a

uninstall_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) uninstall -i; fi
	@rm -rf $(TI_filesystem_path)/etc/zebra.conf $(TI_filesystem_path)/etc/ripd.conf $(TI_filesystem_path)/etc/ripngd.conf 

