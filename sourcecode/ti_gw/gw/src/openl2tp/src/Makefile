###################################################################################
## Copyright 2020 CommScope, Inc. 
## Copyright 2016 ARRIS Enterprises, LLC.
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

include $(TARGET_HOME)/.config

PACKAGE_VERSION := 1.8
PACKAGE_DIR := openl2tp-$(PACKAGE_VERSION)


first_rule: all

all: build


build:
	@echo "CURDIR=$(CURDIR)"
	@echo "CROSS=$(CROSS)"
	@$(MAKE) -j1 -C $(CURDIR)/$(PACKAGE_DIR) -f Makefile CROSS_COMPILE=$(CROSS) all


config_rule:


install_rule:
	@$(MAKE) -j1 -C $(CURDIR)/$(PACKAGE_DIR) -f Makefile DESTDIR=$(TI_filesystem_basefs_path)  install-daemon

clean_rule:
	@(MAKE) -j1 -C $(CURDIR)/$(PACKAGE_DIR) -f Makefile clean

uninstall_rule:


