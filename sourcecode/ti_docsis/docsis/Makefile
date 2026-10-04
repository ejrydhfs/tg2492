# Copyright (C) 1998-2007, 2019 Intel Corporation
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

.PHONY: dummy all config build install uninstall clean distclean package

#
# Include the configuration and definition files
#
-include $(TARGET_HOME)/.config
include $(TARGET_HOME)/TI_Build.make

#
# Build directories
#
ifneq ($(BUILD_POST),y)
#build-y += $(CURDIR)/src/common/management/snmp/src/mibgen
endif
build-y += $(filter $(CURDIR)/%,$(shell find $(CURDIR) -maxdepth 1 -type d|grep src|sort))

#
# General definitions
#
BUILD_DIRS-$(CONFIG_TI_DOCSIS) := $(build-y)
CLEAN_DIRS := $(shell set t=""; for i in "$(BUILD_DIRS-y)"; do t="$$i $$t"; done; echo "$$t")
export DOCSIS_PREFIX  := $(shell pwd)
export CC_PREFIX := $(TARGET_HOME)/ti/common_components
export NETDK_PREFIX := $(TARGET_HOME)/ti/netdk
export DOCSIS_SHARED_LIBS_PREFIX := $(DOCSIS_PREFIX)/build
export TI_INC_PREFIX := $(TARGET_HOME)/ti/include
#
# Compile time macros
#
config-list := $(patsubst %, _config_%, $(BUILD_DIRS-y))
build-list := $(patsubst %, _build_%, $(build-y))
install-list := $(patsubst %, _install_%, $(BUILD_DIRS-y))
uninstall-list := $(patsubst %, _uninstall_%, $(CLEAN_DIRS))
clean-list := $(patsubst %, _clean_%, $(CLEAN_DIRS))
distclean-list := $(patsubst %, _distclean_%, $(CLEAN_DIRS))

#
# DOCSIS Targets that are supported (According to UA spec)
#


all: build install

dummy:

build: dummy $(build-list)

install: dummy $(install-list)

uninstall: dummy $(uninstall-list)

clean: dummy $(clean-list)
	@find ./build \( -name '*.[oas]' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' -o -name '*.so' \) -follow -type f -print | xargs rm -f
	@rm -f $(TI_include)/docsis_pcd.h

distclean: dummy $(distclean-list)
	@find ./build \( -name '*.[oas]' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' -o -name '*.so' \) -follow -type f -print | xargs rm -f
	@rm -f $(TI_include)/docsis_pcd.h

package:
	@ $(MAKE) -C $(filter-out $(CURDIR)/s%,$(patsubst _build_%,%,$(build-list))) package
	$(TARGET_HOME)/tools/scripts/mkpkg -d $(DOCSIS_PREFIX) -o $(DOCSIS_PREFIX)

config: config_pcd $(config-list)

#
# Generic internal targets
#
ifdef BUILD_DIRS-y
$(config-list) : dummy
	@ $(MAKE) -C $(patsubst _config_%,%,$@) config

$(build-list) : dummy
	@ $(MAKE) -C $(patsubst _build_%,%,$@) build

$(install-list) : dummy
	@ $(MAKE) -C $(patsubst _install_%,%,$@) install
else
$(build-list) : dummy
endif

ifdef CLEAN_DIRS
$(uninstall-list) : dummy
	@ $(MAKE) -C $(patsubst _uninstall_%,%,$@) uninstall

$(clean-list) : dummy
	@ $(MAKE) -C $(patsubst _clean_%,%,$@) clean

$(distclean-list) : dummy
	@ $(MAKE) -C $(patsubst _distclean_%,%,$@) distclean
endif

#
# Target for PCD header file generation
#
ifdef CONFIG_TI_PCD
$(TI_include)/docsis_pcd.h: $(TARGET_HOME)/ti/docsis/src/startup/src/docsis.pcd
	@if [ -f $(TARGET_HOME)/ti/common_components/bin/pcdparser ]; then $(TARGET_HOME)/ti/common_components/bin/pcdparser -f $(TARGET_HOME)/ti/docsis/src/startup/src/docsis.pcd -o $(TI_include)/docsis_pcd.h; fi

# ARRIS CHANGE - Create docsis_pcd.h in arris_libs config and not here since arris_libs needs this file. You'd
# think this would be built in ti docsis during initial config but pcdparser isn't built yet so it doesn't run
# till the second config right before the ti docsis build.  So moving it to arris_libs instead.
config_pcd:

#config_pcd: $(TI_include)/docsis_pcd.h
# END ARRIS CHANGE
else
config_pcd:
endif

