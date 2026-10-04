# Copyright (C) 1998-2010, 2014, 2019 Intel Corporation
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
# * Makefile Wrapper file for building IP Route2 Package
# *************************************************************************

#include $(TARGET_HOME)/TI_Build.make
include $(TARGET_HOME)/.config

PACKAGE_VERSION := 2.6.39
PACKAGE_DIR := iproute2-$(PACKAGE_VERSION)
ifndef CURDIR
CURDIR=$(shell pwd)
endif
# IPT_LIB_DIR and LDFLAGS parameters are required for configure script
export IPT_LIB_DIR=$(TI_filesystem_path)/lib/xtables/
export CFLAGS += $(CFLAGS_optimize) 
export LDFLAGS="-L$(IPT_LIB_DIR) -L$(TI_filesystem_path)/lib/ $(LDFLAGS_optimize)"


first_rule: all

all: build install_rule

build: 
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR)
	# ARRIS ADD BEGIN : arris_util.so should static link
	if [ -f $(CURDIR)/$(PACKAGE_DIR)/lib/libnetlink.a ]; then install --preserve-timestamps -m 0644 $(CURDIR)/$(PACKAGE_DIR)/lib/libnetlink.a $(COMPONENT_PREFIX)/build ; fi; 
	if [ -f $(CURDIR)/$(PACKAGE_DIR)/lib/libutil.a ]; then install --preserve-timestamps -m 0644 $(CURDIR)/$(PACKAGE_DIR)/lib/libutil.a $(COMPONENT_PREFIX)/build ; fi; 
	# ARRIS ADD END

config_rule : # To avoid package iptables and iproute build order depencency, just create config file
	@echo  "Configure iproute2 : using xtables, iptables libs are in $$IPT_LIB_DIR"
	@echo  'TC_CONFIG_XT:=y' >$(CURDIR)/$(PACKAGE_DIR)/Config
	@echo  "IPT_LIB_DIR:=$$IPT_LIB_DIR" >>$(CURDIR)/$(PACKAGE_DIR)/Config

install_rule : build
	@install -m 0755 $(CURDIR)/$(PACKAGE_DIR)/ip/ip $(TI_filesystem_path)$(TI_sbindir)/ip
	@$(CROSS)strip $(TI_filesystem_path)$(TI_sbindir)/ip
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) install
ifdef CONFIG_TI_DUAL_STACK_LITE
	install -m 0755 $(CURDIR)/$(PACKAGE_DIR)/ip6tnl1.sh $(TI_filesystem_path)$(TI_sbindir)/ip6tnl1
endif

clean_rule :
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) clean -i; fi
	@rm -rf $(CURDIR)/man
ifdef CONFIG_TI_DUAL_STACK_LITE
	@rm -f $(TI_filesystem_path)/usr/sbin/ip6tnl1
	@rm -f $(CURDIR)/$(PACKAGE_DIR)/Config
endif

uninstall_rule:
	rm -f $(TI_filesystem_path)$(TI_sbindir)/ip

