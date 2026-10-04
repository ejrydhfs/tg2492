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

#ifndef PATCHLEVEL
#include ../../Rules.mak
#endif 

CROSS_COMPILE = $(CROSS)

include $(TARGET_HOME)/.config

# Kernel directory
ifndef KERNEL_DIR
KERNELDIR := $(TARGET_HOME)/kernel/ti/linux-2.6.39.3/src
endif

# File system path, where to install the module
ifndef TI_filesystem_path
$(error TI_filesystem_path is not defined in gw/src/sc_hooks/src/sc_hooks-0.1/Makefile)
endif

ifeq ($(CONFIG_VENDOR_COMCAST),y)
MODULES = sc_nf_bridge_forward_hook.ko sc_hotspot_vlan_pri_hook.ko sc_gre_tx_filter_hook.ko sc_BrHttp.ko sc_daylightsaving.ko

obj-m:= sc_nf_bridge_forward_hook.o sc_hotspot_vlan_pri_hook.o sc_gre_tx_filter_hook.o sc_BrHttp.o sc_daylightsaving.o
else
MODULES = sc_nf_bridge_forward_hook.ko sc_hotspot_vlan_pri_hook.ko sc_gre_tx_filter_hook.ko sc_daylightsaving.ko

obj-m:= sc_nf_bridge_forward_hook.o sc_hotspot_vlan_pri_hook.o sc_gre_tx_filter_hook.o sc_daylightsaving.o
endif

all:modules

modules:
	@$(MAKE) -C $(KERNEL_DIR) M=`pwd` ARCH=arm modules

clean:
	rm -rf $(MODULES) *.o *.ko *.cmd .*.cmd

install:
	@if [ ! -d $(TI_filesystem_path)/lib/modules ]; then mkdir $(TI_filesystem_path)/lib/modules; fi
	@if [ ! -d $(TI_filesystem_path)/lib/modules/sc_hooks ]; then mkdir $(TI_filesystem_path)/lib/modules/sc_hooks; fi
	@for module in $(MODULES); do \
	  cp -f $$module $(TI_filesystem_path)/lib/modules/sc_hooks; \
	done

uninstall:
	@for module in $(MODULES); do\
	  find $(TI_filesystem_path)/lib/modules/sc_hooks -name $$module | xargs rm -f; \
	done


