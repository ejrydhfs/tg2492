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

include $(TI_BUILD_ROOT)/TI_Build.make
-include $(TI_BUILD_ROOT)/.config

#TI_IMAGES_ROOT:=$(TI_images_path)
TI_IMAGES_ROOT:=$(TI_BUILD_ROOT)/build/$(TI_PRODUCT)/images

.PHONY: build config install uninstall clean distclean package

LINUXPREFIX  := $(shell pwd)
CP=/bin/cp
RM=/bin/rm

export LINUXPREFIX DEBUG CP RM

CROSS_FLAGS=CROSS_COMPILE=$(CROSS)

src/.config: $(TI_BUILD_ROOT)/.config
	@$(CP) -f $(TI_BUILD_ROOT)/.config src/
	@echo $(CROSS) > src/.mvl_cross_compile
	$(MAKE) -C src oldconfig ARCH=arm CROSS_COMPILE=$(CROSS)
install: .compiled
	@$(CP) -f src/arch/arm/boot/zImage $(TI_IMAGES_ROOT)/Image.bin
	@$(MAKE) -C src modules_install INSTALL_MOD_PATH=$(TI_filesystem_path) ARCH=arm


build:config
	$(MAKE) -C src ARCH=arm zImage modules SEVENZIP=$(TI_tools_path)/bin/7zip $(CROSS_FLAGS) V=1 
	touch .compiled

config: src/.config

uninstall:
	@$(RM) -f $(TI_IMAGES_ROOT)/Image.bin
	@$(RM) -f .compiled

clean:uninstall
	-@$(CP) -fv $(TI_BUILD_ROOT)/.config src/
	@$(MAKE) -C src clean 

distclean:clean
	@$(MAKE) -C src distclean

package:
	@$(TI_BUILD_ROOT)/tools/scripts/mkpkg -s -d $(LINUXPREFIX) -o $(LINUXPREFIX)

