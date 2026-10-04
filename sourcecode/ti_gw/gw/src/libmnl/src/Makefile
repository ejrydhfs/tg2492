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
# * Makefile Wrapper file for building libmnl
# *************************************************************************

.NOTPARALLEL:

#include $(TARGET_HOME)/TI_Build.make
ifndef CURDIR
CURDIR=$(shell pwd)
endif

PACKAGE_VERSION := 1.0.3
PACKAGE_DIR := libmnl-$(PACKAGE_VERSION)
PACKAGE_CONF := \
        --host=armeb-buildroot-linux-uclibcgnueabi --prefix=$(TI_filesystem_path) \
         --includedir=$(TARGET_HOME)/ti/include \
        --with-gnu-ld \
        --exec-prefix=$(TI_filesystem_path) --datarootdir=$(CURDIR)

export CFLAGS += $(CFLAGS_optimize)
export LDFLAGS += $(LDFLAGS_optimize)

first_rule: all

all: build

$(CURDIR)/$(PACKAGE_DIR)/Makefile:
	# For some reason these files must not be CI, so for working with VersionControl, we create new ones
	#@cd $(CURDIR)/$(PACKAGE_DIR) && cp Makefile.in.orig Makefile.in
	#@cd $(CURDIR)/$(PACKAGE_DIR) && cp aclocal.m4.orig aclocal.m4
	#@cd $(CURDIR)/$(PACKAGE_DIR) && cp configure.orig configure
	# Now configure
	rm -rf $(CURDIR)/$(PACKAGE_DIR)
	tar -xjf libmnl-$(PACKAGE_VERSION).tar.bz2
	@cd $(CURDIR)/$(PACKAGE_DIR) && CC=$(CROSS)gcc ./configure $(PACKAGE_CONF)

build_lib : $(CURDIR)/$(PACKAGE_DIR)/Makefile
	$(MAKE) -j1 -C $(CURDIR)/$(PACKAGE_DIR) all

build: build_lib install_rule
	@$(MAKE) -j1 -C $(CURDIR)/$(PACKAGE_DIR) all check
	#install -D $(CURDIR)/$(PACKAGE_DIR)/include/libmnl/libmnl.h $(TI_include)/libmnl/libmnl.h
	#cp -f $(CURDIR)/$(PACKAGE_DIR)/src/.libs/libmnl.* $(TARGET_HOME)/ti/lib/
	#cp -f $(CURDIR)/$(PACKAGE_DIR)/src/.libs/libmnl.* $(TI_filesystem_path)/lib/

config_rule: $(CURDIR)/$(PACKAGE_DIR)/Makefile

install_rule install:
	$(MAKE) -j1 -C $(CURDIR)/$(PACKAGE_DIR) install
	if [ -f $(TI_filesystem_path)/lib/libmnl.so ]; then cp -f $(TI_filesystem_path)/lib/libmnl.* $(TARGET_HOME)/ti/lib/ ; fi; 

clean_rule distclean clean:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) distclean -i; fi
	rm -rf $(CURDIR)/$(PACKAGE_DIR)
	rm -f $(TARGET_HOME)/ti/lib/libmnl.* 

uninstall_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) uninstall -i; fi

