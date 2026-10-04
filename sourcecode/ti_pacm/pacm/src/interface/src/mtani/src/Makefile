################################################################################
#
#  This file is provided under a dual BSD/GPLv2 license.  When using or
#  redistributing this file, you may do so under either license.
#
#  GPL LICENSE SUMMARY
#
#  Copyright(c) 2008-2019 Intel Corporation.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#  The full GNU General Public License is included in this distribution
#  in the file called LICENSE.GPL.
#
#
#  Contact Information:
#  Intel Corporation
#  2200 Mission College Blvd.
#  Santa Clara, CA  97052
#
#
#  BSD LICENSE
#
#  Copyright(c) 2008-2019 Intel Corporation. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
################################################################################

# Target module name
TARGET := mtani

# Required definitions
ifndef TARGET_HOME
TARGET_HOME := /vobs/ua_sdk
endif

ifndef KERNELRELEASE
# Include TI build definition file (optional) only in non-kernel compilation mode
-include $(TARGET_HOME)/TI_Build.make
endif

# Kernel directory
ifndef KERNEL_DIR
KERNEL_DIR := $(TARGET_HOME)/kernel/ti/linux-2.6.39.3/src
endif

# File system path, where to install the module
ifndef TI_filesystem_path
TI_filesystem_path := $(TARGET_HOME)/build/vsdk/fs
endif

# TI include export directory
ifndef TI_include
TI_include := $(TARGET_HOME)/ti/include
endif

# Include directories
INCLUDES := \
	-I$(PWD)/../include \
	-I$(KERNEL_DIR)/include/asm-arm/arch-avalanche/generic \
	-I$(KERNEL_DIR)/include/asm-arm/arch-avalanche/puma5 \
	-I$(TI_include)

export EXTRA_CFLAGS += $(INCLUDES)

# Module extra compilation flags
EXTRA_CFLAGS +=

# Installation module directory
export INSTALL_MOD_DIR := /drivers/net

# Extra symbol versions (External symbols used by this module)
export KBUILD_EXTSYMVERS := $(PWD)/../../../../../../docsis/src/common/data_path/docsis_bridge/dbridge_dlm/src/

# Filesystem path
export INSTALL_MOD_PATH=$(TI_filesystem_path)

# Kernel module compilation
ifneq ($(KERNELRELEASE),)

obj-m += $(TARGET).o

# Target objects
$(TARGET)-objs := \
	mta_ni.o

else

# Makefile targets
build: 		build_rule

config: 	config_rule

install: 	install_rule

uninstall: 	uninstall_rule

clean:		clean_rule

distclean:	uninstall clean

all: 		config build install

# UMF Rules
config_rule:

build_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm modules

install_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm modules_install
	@#ARRIS CHANGE - preserve timestamps
	@install -p -m 0755 ../include/mta_ni.h $(TI_include)
	@#END ARRIS CHANGE

uninstall_rule:
	@find $(TI_filesystem_path)/lib/modules/ -name $(TARGET).ko | xargs rm -rf
	@rm -rf $(TI_include)/mta_ni.h

clean_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm clean
	@rm -f Module.symvers

endif

