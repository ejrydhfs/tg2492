################################################################################
##+--------------------------------------------------------------------------+##
##| BSD LICENSE                                                              |##
##|                                                                          |##
##| Copyright(c) 1998-2019 Intel Corporation. All rights reserved.           |##
##|                                                                          |##
##| Redistribution and use in source and binary forms, with or without       |##
##| modification, are permitted provided that the following conditions       |##
##| are met:                                                                 |##
##|                                                                          |##
##|   * Redistributions of source code must retain the above copyright       |##
##|     notice, this list of conditions and the following disclaimer.        |##
##|                                                                          |##
##|   * Redistributions in binary form must reproduce the above copyright    |##
##|     notice, this list of conditions and the following disclaimer in      |##
##|     the documentation and/or other materials provided with the           |##
##|     distribution.                                                        |##
##|                                                                          |##
##|   * Neither the name of Intel Corporation nor the names of its           |##
##|     contributors may be used to endorse or promote products derived      |##
##|     from this software without specific prior written permission.        |##
##|                                                                          |##
##| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS      |##
##| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT        |##
##| LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR    |##
##| A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     |##
##| OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    |##
##| SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT         |##
##| LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,    |##
##| DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    |##
##| THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      |##
##| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    |##
##| OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     |##
##+--------------------------------------------------------------------------+##
################################################################################

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
#build-y += $(CURDIR)/src/snmp/src/mibgen
endif
build-y += $(filter $(CURDIR)/%,$(shell find $(CURDIR) -maxdepth 1 -type d|grep src|sort))

#
# General definitions
#
BUILD_DIRS-$(CONFIG_TI_PACM) := $(build-y)
CLEAN_DIRS := $(shell set t=""; for i in "$(BUILD_DIRS-y)"; do t="$$i $$t"; done; echo "$$t")
export PACM_PREFIX  := $(shell pwd)

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
# PACM Targets that are supported (According to UA spec)
#
all: build install

dummy:

build: dummy $(build-list)

install: dummy $(install-list)

uninstall: dummy $(uninstall-list)

clean: dummy $(clean-list)
	@find ./build \( -name '*.[oas]' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' -o -name '*.so' \) -follow -type f -print | xargs rm -f

distclean: dummy $(distclean-list)
	@find ./build \( -name '*.[oas]' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' -o -name '*.so' \) -follow -type f -print | xargs rm -f
	@rm -f $(TI_include)/pacm_pcd.h

package:
	@ $(MAKE) -C $(filter-out $(CURDIR)/s%,$(patsubst _build_%,%,$(build-list))) package
	$(TARGET_HOME)/tools/scripts/mkpkg -d $(PACM_PREFIX) -o $(PACM_PREFIX)

config: $(config-list)

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

