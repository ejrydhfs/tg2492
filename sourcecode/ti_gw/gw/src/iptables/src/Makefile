# Copyright (C) 1998-2010, 2014-2019 Intel Corporation
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
# * Makefile Wrapper file for building IP Tables
# *************************************************************************

#include $(TARGET_HOME)/TI_Build.make
ifndef CURDIR
CURDIR=$(shell pwd)
endif

PACKAGE_VERSION := 1.4.12.1
PACKAGE_DIR := iptables-$(PACKAGE_VERSION)
PACKAGE_CONF := --host=armeb-buildroot-linux-uclibcgnueabi --prefix=$(TI_filesystem_path) \
                --with-kernel=$(KERNEL_DIR) \
                --enable-static --with-ksource=$(KERNEL_DIR) \
        CFLAGS="$(CFLAGS_basic)" \
        LDFLAGS="-L$(TARGET_HOME)/ti/lib -L$(TI_filesystem_path)/lib $(LDFLAGS_optimize)" \
        --with-gnu-ld \
        --exec-prefix=$(TI_filesystem_path) --datarootdir=$(CURDIR)
first_rule: all

all: build

$(CURDIR)/$(PACKAGE_DIR)/Makefile:
	#tar jxf iptables-$(PACKAGE_VERSION).tar.bz2
	@# ARRIS CHANGE
	@cd $(CURDIR)/$(PACKAGE_DIR) && autoreconf -fi
	@cd $(CURDIR)/$(PACKAGE_DIR) && CC=$(CROSS)gcc ./configure INSTALL="/usr/bin/install -p -c" $(PACKAGE_CONF)
	@# END ARRIS CHANGE

build: $(CURDIR)/$(PACKAGE_DIR)/Makefile
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) all

config_rule: $(CURDIR)/$(PACKAGE_DIR)/Makefile

install_rule install:
	$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) install 
	@echo "***"
	@echo "***"
	@echo "*** Iptables prefix : $(TI_filesystem_path)"
	@echo "***"
	@echo "***"

clean_rule distclean clean:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) distclean -i; fi
	@rm -rf $(CURDIR)/man

uninstall_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) uninstall -i; fi

