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
# * Makefile Wrapper file for building libnfnetlink
# *************************************************************************

#include $(TARGET_HOME)/TI_Build.make
ifndef CURDIR
CURDIR=$(shell pwd)
endif

PACKAGE_VERSION := 1.0.0
PACKAGE_DIR := libnfnetlink-$(PACKAGE_VERSION)

PACKAGE_CONF := \
        --host=armeb-buildroot-linux-uclibcgnueabi --prefix=$(TI_filesystem_path) \
        --includedir=$(TI_include) \
        CFLAGS="$(CFLAGS_basic)" \
        --with-gnu-ld \
        --exec-prefix=$(TI_filesystem_path) --datarootdir=$(CURDIR)

first_rule: all

all: build

$(CURDIR)/$(PACKAGE_DIR)/Makefile:
	# For some reason these files must not be CI, so for working with VersionControl, we create new ones
	@cd $(CURDIR)/$(PACKAGE_DIR) && cp Makefile.in.orig Makefile.in
	@cd $(CURDIR)/$(PACKAGE_DIR) && cp aclocal.m4.orig aclocal.m4
	@cd $(CURDIR)/$(PACKAGE_DIR) && cp configure.orig configure
	# Now configure
	cd $(CURDIR)/$(PACKAGE_DIR) && CC=$(CROSS)gcc ./configure $(PACKAGE_CONF) 

build_lib : $(CURDIR)/$(PACKAGE_DIR)/Makefile
	$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) all 

build: install_rule

config_rule: $(CURDIR)/$(PACKAGE_DIR)/Makefile

$(CURDIR)/$(PACKAGE_DIR)/src/.libs/libnfnetlink.so: build_lib

install_rule install: $(CURDIR)/$(PACKAGE_DIR)/src/.libs/libnfnetlink.so
	$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) install 
	if [ -f $(TI_filesystem_path)/lib/libnfnetlink.so ]; then cp -f $(TI_filesystem_path)/lib/libnfnetlink.* $(TARGET_HOME)/ti/lib/ ; fi; 
	# ARRIS ADD BEGIN : arris_util.so should static link
	if [ -f $(TI_filesystem_path)/lib/libnfnetlink.a ]; then install --preserve-timestamps -m 0644 $(TI_filesystem_path)/lib/libnfnetlink.a $(COMPONENT_PREFIX)/build ; fi; 
	# ARRIS ADD END

clean_rule distclean clean:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) distclean -i; fi
	@rm -f $(TARGET_HOME)/ti/lib/libnfnetlink.*  
	@rm -f $(CURDIR)/$(PACKAGE_DIR)/Makefile.in  
	@rm -f $(CURDIR)/$(PACKAGE_DIR)/aclocal.m4  
	@rm -f $(CURDIR)/$(PACKAGE_DIR)/configure 
	@rm -rf $(CURDIR)/man

uninstall_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) uninstall -i; fi

