###################################################################################
## Copyright 2020 CommScope, Inc. 
## Copyright (c) 2015-2016, ARRIS Enterprises, LLC. 
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
TARGET := tibat

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
        -I$(TARGET_HOME)/ti/pacm/src/interface/src/mtani/include \
        -I$(KERNEL_DIR)/net/bridge/ti_docsis_bridge \
        -I$(KERNEL_DIR)/include \
        -I$(KERNEL_DIR)/include/asm/arch-avalanche \
        -I$(KERNEL_DIR)/include/asm/arch-avalanche/generic \
        -I$(KERNEL_DIR)/include/asm/arch-avalanche/generic/puma5 \
        -I$(TARGET_HOME)/ti/include

export EXTRA_CFLAGS += $(INCLUDES)

#export EXTRA_CFLAGS += -Werror

# Module extra compilation flags
EXTRA_CFLAGS += -DINIT_EXT_DMA

# Installation module directory
export INSTALL_MOD_DIR := /drivers/tlm

# Extra symbol versions (External symbols used by this module)
export KBUILD_EXTSYMVERS := 

# Filesystem path
export INSTALL_MOD_PATH=$(TI_filesystem_path)

# Kernel module compilation
ifneq ($(KERNELRELEASE),)

	obj-m += $(TARGET).o

	# Target objects
	$(TARGET)-objs := tibat_char.o jtagfunc430.o lowlevelfunc430.o

else

unexport LDFLAGS
unexport CFLAGS


STEC2BIN=$(TARGET_HOME)/tools/bin/srec2bin
INSTALL_BIN_DIR=/lib/modules/2.6.39.3/drivers/tlm
SREC_CAT=$(TARGET_HOME)/tools/bin/srec_cat
SREC_DIR=$(TARGET_HOME)/ti/arris_pcable/src/tlm/tibat_ko/srec
SREC_BIN_DIR=$(TI_filesystem_path)/$(INSTALL_BIN_DIR)


# Add new TI based charger models to the list below
# NOTE: THIS IS THE ONLY CHANGE NEEDED IN THIS MAKEFILE FOR NEW MODELS
modemTypes=TM3202G TG1682GP2 TG1652G TG2472GP2

define intel2bin
if [ -f  $(SREC_DIR)/$1_$2.hex ]; then $(SREC_CAT) $(SREC_DIR)/$1_$2.hex  -Intel -o  $(SREC_BIN_DIR)/$1_$2_TI.bin_temp -Binary; \
dd if=$(SREC_BIN_DIR)/$1_$2_TI.bin_temp of=$(SREC_BIN_DIR)/$1_$2_TI.bin bs=1 skip=50176;\
rm $(SREC_BIN_DIR)/$1_$2_TI.bin_temp;\
fi
endef

define removebin
if [ -f  $(SREC_BIN_DIR)/$1_$2_TI.bin ]; then rm -rf $(SREC_BIN_DIR)/$1_$2_TI.bin; fi
endef

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
	@for modem in $(modemTypes); do $(call intel2bin,$${modem},AP); done
	@for modem in $(modemTypes); do $(call intel2bin,$${modem},ST); done

uninstall_rule:
	@find $(SREC_BIN_DIR) -name $(TARGET).ko | xargs rm -rf
	
clean_rule:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm clean
	@rm -f Module.symvers
	@for modem in $(modemTypes); do $(call removebin,$${modem},AP); done
	@for modem in $(modemTypes); do $(call removebin,$${modem},ST); done
endif

