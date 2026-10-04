###################################################################################
## Copyright 2013-2020 CommScope, Inc. 
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

# Target module name
TARGET := arris

ifndef KERNELRELEASE
# Include TI build definition file (optional) only in non-kernel compilation mode
-include $(TARGET_HOME)/TI_Build.make
endif

# Kernel directory
ifndef KERNEL_DIR
KERNEL_DIR := $(TARGET_HOME)/kernel/ti/linux-2.6.39.3/src
endif

# TI include export directory
ifndef TI_include
TI_include := $(TARGET_HOME)/ti/include
endif

# Include directories
INCLUDES := \
        -I$(KERNEL_DIR)/include \
        -I$(KERNEL_DIR)/net/bridge \
        -I$(KERNEL_DIR)/asm/arch-avalanche \
        -I$(KERNEL_DIR)/include/asm/arch-avalanche/generic \
        -I$(KERNEL_DIR)/include/asm/arch-avalanche/generic/puma6 \
	-I$(KERNEL_DIR)/include/asm-arm/arch-avalanche/generic \
        -I$(TARGET_HOME)/ti/include \
	-I$(TARGET_HOME)/ti/arris_libs/src/arris_kernel/include

export EXTRA_CFLAGS += $(INCLUDES)

# Installation module directory
export INSTALL_MOD_DIR := /drivers

# Extra symbol versions (External symbols used by this module)
export KBUILD_EXTSYMVERS :=

# Filesystem path
export INSTALL_MOD_PATH=$(TI_filesystem_path)

# Kernel module compilation
ifneq ($(KERNELRELEASE),)

	obj-m += $(TARGET).o

	# Target objects
	$(TARGET)-objs := arris_mod.o arris_rip.o arris_bridge.o

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
	@install --preserve-timestamps -D arris_mod_api.h $(KERNEL_DIR)/include/arris/arris_mod_api.h

build_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm modules

install_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm modules_install

uninstall_rule:
	@find $(TI_filesystem_path)/lib/modules/ -name $(TARGET).ko | xargs rm -rf

clean_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm clean
	@rm -f Module.symvers

endif
