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

# ******************************************************************************
# * FILE PURPOSE:    PSP Linux package  Makefile
# ******************************************************************************
# * FILE NAME:       Makefile
# *
# * DESCRIPTION:     PSP Linux package Makefile
# *
# * (C) Copyright 2008, Texas Instruments, Inc
# *****************************************************************************/

-include $(TI_BUILD_ROOT)/TI_Build.make
-include $(TI_BUILD_ROOT)/.config

TI_IMAGES_ROOT:=$(TI_images_path)
# ARRIS ADD
UBOOT_EMBED_DIR:=$(TARGET_HOME)/ti/arris_libs/src/boot

.PHONY: build config install uninstall clean distclean package

PSPUBOOTPREFIX  := $(shell pwd)
#CROSS_TOOLS := $(CROSS)
CP=/bin/cp
RM=/bin/rm

#export PSPUBOOTPREFIX CROSS_TOOLS DEBUG
export PSPUBOOTPREFIX DEBUG CP RM



# Endian Selection - this variable comes from kernel .config file
ifeq ($(CONFIG_CPU_BIG_ENDIAN), y)
CROSS?=armeb-linux-uclibceabi-
else
CROSS?=armeb-linux-uclibceabi-
endif

#Create "Board name" from CONFIG_TI_SYSTEM_BOARD_NAME   (Remove '"')
BOARD_NAME:=$(strip $(subst ",, $(CONFIG_TI_SYSTEM_BOARD_NAME)))

# Identify config option
export CONFIG_TI_BBU
export CONFIG_TI_PSPUBOOT_DDR_ASYNC_MODE
export CONFIG_TI_PSPUBOOT_EXTENDED_FLASH
export CONFIG_UBOOT_SDRAM_ADDRESS
export CONFIG_ARM_AVALANCHE_SDRAM_ADDRESS
export CONFIG_ARM_AVALANCHE_KERNEL_PARAMS_ADDRESS
export CONFIG_ARM_AVALANCHE_KERNEL_PARAMS_OFFSET
export BOARD_NAME
export CONFIG_GENERIC_MMC
export CONFIG_SYSTEM_PUMA6_SOC
export CONFIG_MACH_PUMA6_FPGA
# ARRIS ADD
export CONFIG_TI_PSPUBOOT_ARRIS_GATEWAY
export CONFIG_VENDOR_ARRIS_P6MG
export CONFIG_VENDOR_ARRIS_TG1682
export CONFIG_SYSTEM_MOCA
export CONFIG_VENDOR_ARRIS_NO_ATOM

CROSS_FLAGS=CROSS_COMPILE=$(CROSS)

ifdef CONFIG_TI_PSPUBOOT
install:build
	-$(CP) -f src/u-boot-1.2.0/u-boot.bin $(TI_IMAGES_ROOT)/u-boot.bin
	#-$(CP) -f src/u-boot-1.2.0/u-boot.bin $(TI_IMAGES_ROOT)/$(BOARD_NAME)_u-boot.bin
# ARRIS
ifdef CONFIG_MACH_PUMA5
	-$(CP) -f src/u-boot-1.2.0/u-boot.srec $(TI_IMAGES_ROOT)/u-boot.srec
	cd $(TARGET_HOME)/tools/bin/ && \
	$(TARGET_HOME)/tools/bin/bin2data.linux $(TI_IMAGES_ROOT)/u-boot.bin ubootArray > $(UBOOT_EMBED_DIR)/u-boot.c
else
# can't do the install from here since the filesystem doesn't exist yet; copy over and install from arris_libs
	-$(CP) -f src/u-boot-1.2.0/u-boot.bin $(UBOOT_EMBED_DIR)/u-boot.bin
endif
# END ARRIS


build:config
	$(MAKE) -C src/u-boot-1.2.0 CROSS_COMPILE=$(CROSS)

config: $(PSPUBOOTPREFIX)/src/u-boot-1.2.0/include/config.mk
	#-$(CP)  -uf $(KERNEL_DIR)/include/asm-arm/arch-avalanche/puma6/docsis_ip_boot_params.h src/u-boot-1.2.0/include

$(PSPUBOOTPREFIX)/src/u-boot-1.2.0/include/config.mk: $(TARGET_HOME)/.config
	$(MAKE) -C src/u-boot-1.2.0 CROSS_COMPILE=$(CROSS) $(BOARD_NAME)_config

uninstall:
	-$(RM) -f $(TI_IMAGES_ROOT)/u-boot.bin
	-$(RM) -f $(TI_IMAGES_ROOT)/$(BOARD_NAME)_u-boot.bin
	-$(RM) -f $(TI_IMAGES_ROOT)/u-boot.srec
# ARRIS ADD
ifdef CONFIG_MACH_PUMA6
	-$(RM) -f $(TI_FS_PATH)/usr/sbin/u-boot.bin
	-$(RM) -f $(UBOOT_EMBED_DIR)/u-boot.bin
	-$(RM) -f src/u-boot-1.2.0/u-boot.bin
endif
# END ARRIS

clean:uninstall
	$(MAKE) -C src/u-boot-1.2.0 clean


distclean:clean
	$(MAKE) -C src/u-boot-1.2.0 distclean
# ARRIS
ifdef CONFIG_MACH_PUMA5
	cd $(UBOOT_EMBED_DIR) && rm u-boot.*
endif
# END ARRIS
else
install:

build:

config:

uninstall:

clean:

distclean:

endif

package:
	-$(CP)  -f $(KERNEL_DIR)/include/asm-arm/arch-avalanche/puma6/docsis_ip_boot_params.h src/u-boot-1.2.0/include
	$(TI_BUILD_ROOT)/tools/scripts/mkpkg -s -d $(PSPUBOOTPREFIX) -o $(PSPUBOOTPREFIX)

