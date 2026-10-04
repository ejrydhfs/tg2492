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
# * Makefile Wrapper file for building Libxml2 library
# *************************************************************************

include $(TARGET_HOME)/TI_Build.make
include $(TARGET_HOME)/.config

export CFLAGS := -Os -mthumb -mthumb-interwork -I$(TARGET_HOME)/ti/include

PACKAGE_VERSION := 2.7.7
PACKAGE_DIR := libxml2-$(PACKAGE_VERSION)
PACKAGE_CONF := \
	--host=$(patsubst %-gcc,%,$(CROSS)gcc) \
	--exec-prefix=$(TI_filesystem_path) \
	--prefix=$(TARGET_HOME)/ti \
	--includedir=$(TARGET_HOME)/ti/include \
	--without-iconv \
	--without-python \
	--with-zlib=$(TI_filesystem_path)/lib \
	--with-pic --without-debug \
	--without-docbook --without-ftp --without-history --without-legacy


#        --with-pic --without-c14n --without-catalog --without-debug \
#        --without-docbook --without-ftp --without-history --without-legacy \
#        --with-minimum --disable-static
	
# UNIHAN ADD START
PACKAGE_CONF += --without-threads
# UNIHAN ADD END
	
export CFLAGS  := $(CFLAGS_optimize) -g $(TI_cflags_ARMThumb)
export LDFLAGS := $(LDLAGS_optimize)

# -mthumb -mthumb-interwork -I$(TARGET_HOME)/ti/include
#unexport CFLAGS
unexport LDFLAGS
unexport CPPFLAGS 
	
first_rule: all

all: build

$(CURDIR)/$(PACKAGE_DIR)/Makefile:
	@# ARRIS CHANGE
	@cd $(CURDIR)/$(PACKAGE_DIR) && autoreconf -fi
	@cd $(CURDIR)/$(PACKAGE_DIR) && CC=$(CROSS)gcc ./configure INSTALL="/usr/bin/install -p -c" $(PACKAGE_CONF)
	@# END ARRIS CHANGE

build_lib: $(CURDIR)/$(PACKAGE_DIR)/Makefile
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) all

# Need to install the library now because it is required by the next applications
build: install_rule

config_rule:

$(CURDIR)/$(PACKAGE_DIR)/.libs/libxml2.so: build_lib

install_rule: $(CURDIR)/$(PACKAGE_DIR)/.libs/libxml2.so
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) install
	@mkdir -p $(TARGET_HOME)/ti/lib
	@if [ -f $(TI_filesystem_path)/lib/libxml2.a ]; then mv -f $(TI_filesystem_path)/lib/libxml2.a $(TARGET_HOME)/ti/lib/libxml2_pic.a; fi; 
	@mkdir -p $(TARGET_HOME)/ti/lib/bin/
	@if [ -f $(TI_filesystem_path)/bin/xml2-config ]; then mv -f $(TI_filesystem_path)/bin/xml2-config $(TARGET_HOME)/ti/lib/bin/xml2-config; fi;
	@if [ -f $(TI_filesystem_path)/bin/xmlcatalog ]; then rm -f $(TI_filesystem_path)/bin/xmlcatalog; fi;
	@if [ -f $(TI_filesystem_path)/bin/xmllint ]; then rm -f $(TI_filesystem_path)/bin/xmllint; fi;
	@if [ -f $(TI_filesystem_path)/bin/xmlwf ]; then rm -f $(TI_filesystem_path)/bin/xmlwf; fi;

clean_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) distclean -i; fi

uninstall_rule:
	@if [ -f $(CURDIR)/$(PACKAGE_DIR)/Makefile ]; then $(MAKE) -C $(CURDIR)/$(PACKAGE_DIR) uninstall -i; fi

