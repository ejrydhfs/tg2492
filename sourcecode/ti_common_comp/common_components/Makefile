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

.PHONY: dummy all config build install uninstall clean distclean package

.NOTPARALLEL:

#
# Include the configuration and definition files
#
-include $(TARGET_HOME)/.config
include $(TARGET_HOME)/TI_Build.make
export BUILD_POST

#
# Build directories
#
#ifneq ($(BUILD_POST),y)
#build-y += $(TARGET_HOME)/ti/docsis/src/common/management/snmp/src/mibgen
# ARRIS ADD BEGIN - userspace backtrace needs to be generated before pcd
#build-y += src/libunwind/src
# ARRIS ADD END
#endif

#build-y += src # ARRIS DELETE - Delete duplicate src tree walk by Make
build-y += $(filter $(CURDIR)/%,$(shell find $(CURDIR) -maxdepth 1 -type d|grep src|sort))

#
# General definitions
#
BUILD_DIRS-$(CONFIG_TI_COMMON_COMPONENTS) := $(build-y)
CLEAN_DIRS := $(build-y)
ADV_DIRS := adv_src
export COMCOMP_PREFIX  := $(shell pwd)
export COMPONENT_PREFIX := $(COMCOMP_PREFIX)
export COMPONENT_INCLUDES := $(patsubst %,-I%, $(shell find $(COMPONENT_PREFIX) -name include -type d -follow 2> /dev/null))

#
# Compile time macros
#
config-list := $(patsubst %, _config_%, $(BUILD_DIRS-y))
build-list := $(patsubst %, _build_%, $(BUILD_DIRS-y))
#install-list := $(patsubst %, _install_%, $(ADV_DIRS))
install-list := $(patsubst %, _install_%, $(BUILD_DIRS-y))
uninstall-list := $(patsubst %, _uninstall_%, $(CLEAN_DIRS))
clean-list := $(patsubst %, _clean_%, $(CLEAN_DIRS))
distclean-list := $(patsubst %, _distclean_%, $(CLEAN_DIRS))
package-list := $(patsubst %, _package_%, $(ADV_DIRS))

#
# COMCOMP Targets that are supported (According to UA spec)
#
all: build install

dummy:

build: dummy $(build-list)
	# ARRIS MOD BEGIN : Don't delete the .a archive files - why does common components do this?
	#find ./build \( -name '*.o' -o -name '*.so' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' \) -follow -type f -print 2> /dev/null | xargs rm -f
	# ARRIS MOD END

install: dummy $(install-list) install_cli_scripts
#	@install -m 555 system/check/z_arris_check.sh $(TI_filesystem_path)/$(TI_scriptsdir)

uninstall: dummy $(uninstall-list) uninstall_cli_scripts

clean: dummy $(clean-list)
	@find . \( -name core -o -name '*~' -o -name '.*~' -o -name '.#*' \) -follow -type f -print 2> /dev/null | xargs rm -f

distclean: dummy $(distclean-list)
	@find . \( -name core -o -name '*~' -o -name '.*~' -o -name '.#*' \) -follow -type f -print 2> /dev/null | xargs rm -f

package: dummy $(package-list)
	@rm -rf /tmp/$(shell whoami)_temp_package
	@mkdir -p /tmp/$(shell whoami)_temp_package/common_components

	@mkdir -p $(TI_include)
	#$(MAKE) -C $(OPENSSL_SRC_DIR) -f Makefile.flat Makefile
	$(MAKE) -C $(OPENSSL_SRC_DIR) -f Makefile.flat package
	@rm -rf $(TI_include)/openssl
	@ln  -sf $(OPENSSL_SRC_DIR)/include/openssl/ $(TI_include)

	@$(MAKE) -C src/envoy_linux/src
	@find ./src/envoy_linux \( -name '*.d' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' \) -follow -type f -print | xargs rm -f
	@find ./src/envoy_linux/src -name \*.c -type f -follow > /tmp/$(shell whoami)_temp_package/exclude.tmp
	@find ./build \( -name '*.[ao]' -o -name '*.so' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' \) -follow -type f -print | xargs rm -f
	@tar --ignore-failed-read --exclude lost+found --exclude host --exclude-from /tmp/$(shell whoami)_temp_package/exclude.tmp -h  -cf - . | tar -C /tmp/$(shell whoami)_temp_package/common_components -xf -
	@find ./src/envoy_linux \( -name '*.o' \) -follow -type f -print | xargs rm -f
	@$(MAKE) -C /tmp/$(shell whoami)_temp_package/common_components/src/mmc/src/phison_util/src
	@$(TARGET_HOME)/tools/scripts/mkpkg -d /tmp/$(shell whoami)_temp_package/common_components -o $(COMCOMP_PREFIX)
	@rm -rf /tmp/$(shell whoami)_temp_package

	@rm -rf $(TI_include)
	@$(MAKE) -C $(OPENSSL_SRC_DIR) -f Makefile.flat distclean
	$(MAKE) -C $(TARGET_HOME)/ti/system/src/uClibc -f Makefile.flat distclean
	-$(MAKE) -C $(KERNEL_DIR) distclean
	-$rm -f  $(KERNEL_DIR)/include/linux/version.h

config: dummy $(config-list)

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
endif

ifdef CLEAN_DIRS
$(uninstall-list) : dummy
	@ $(MAKE) -C $(patsubst _uninstall_%,%,$@) uninstall

$(clean-list) : dummy
	@ $(MAKE) -C $(patsubst _clean_%,%,$@) clean

$(distclean-list) : dummy
	@ $(MAKE) -C $(patsubst _distclean_%,%,$@) distclean
endif

$(package-list) : dummy
	@ $(MAKE) -C $(patsubst _package_%,%,$@) package

install_cli_scripts:

uninstall_cli_scripts:
ifeq ($(CONFIG_TI_CLI),y)
	@rm -f $(TI_filesystem_path)$(TI_sbindir)/cli*.sh
	@rm -f $(TI_filesystem_path)$(TI_sbindir)/arris*.sh
endif
