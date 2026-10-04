###################################################################################
## Copyright 1998-2019 Intel Corporation
##
## Redistribution and use in source and binary forms, with or without modification,
## are permitted provided that the following conditions are met:
## 1. Redistributions of source code must retain the above copyright notice,
##    this list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
## 3. Neither the name of the copyright holder nor the names of its contributors
##    may be used to endorse or promote products derived from this software
##    without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
## THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
## BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
## OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
## OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
## OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
## WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
## OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
## EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
##
## SPDX-License-Identifier: BSD-3-Clause
###################################################################################

# *************************************************************************
# * Makefile Wrapper file for building ethtool application
# *************************************************************************

include $(TARGET_HOME)/TI_Build.make
include $(TARGET_HOME)/.config

PACKAGE_VERSION := 6
PACKAGE_DIR := ethtool-$(PACKAGE_VERSION)
PACKAGE_CONF := \
        --host=$(patsubst %-gcc,%,$(CROSS)gcc) \
	--prefix=$(CURDIR)/../ \
	--exec-prefix=$(TI_filesystem_path) \
	--includedir=$(TARGET_HOME)/ti/include 

export CFLAGS += -Os

first_rule: all

all: build

$(CURDIR)/$(PACKAGE_DIR)/Makefile:
	@cd $(CURDIR)/$(PACKAGE_DIR) && CC=$(CROSS)gcc ./configure $(PACKAGE_CONF)

build: $(CURDIR)/$(PACKAGE_DIR)/Makefile
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) all

config_rule:

install_rule:
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) install

clean_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) distclean -i; rm -rf $(CURDIR)/$(PACKAGE_DIR)/Makefile; fi
	@rm -rf $(CURDIR)/../man

uninstall_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) uninstall -i; fi

