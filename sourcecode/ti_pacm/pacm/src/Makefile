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

.PHONY:  build clean distclean install uninstall

.NOTPARALLEL:

#
# Include the configuration file and build file
#
include $(PACM_PREFIX)/build/pacm_config.make 
include $(TARGET_HOME)/TI_Build.make

# ARRIS ADD
SIP_STACK_BASE = $(TARGET_HOME)/../SIP

FS_LIB_PATH= $(TI_FS_PATH)$(TI_FS_LIB_PATH)

RV_LIB_PATH= $(SIP_STACK_BASE)/binaries/linux_MontaVista_4.0/Debug/lib
# END ARRIS ADD

#
# Modules directories to be built
#
subdir-y += common/src
subdir-y += interface/src
subdir-y += mtacontrol/src
subdir-$(CONFIG_TI_PACM_SERVICES)   += services/src
subdir-$(CONFIG_TI_PACM_SNMP)       += snmp/src
subdir-$(CONFIG_TI_PACM_SECURITY)   += security/src
subdir-$(CONFIG_TI_PACM_VENDOR_APP) += vendor/src
subdir-$(CONFIG_TI_PACM_BUILD_TEST) += test/src
subdir-y += init/src
subdir-$(CONFIG_TI_PACM_DOIM_DOCSIS_IF) += interface/src/doim/src
subdir-y += cli_menus/src
subdir-y += cli_menus/cli_plugin/src
subdir-y += logger/src
subdir-y += logger/voice_plugin/src

#
# Targets that are supported according to UA spec,
# Please do not change anything below.
#
#pacm-headers-list := $(shell ls ./include/*.h 2> /dev/null)

all: build install

# ARRIS CHANGE - ADD SIP Stack to build
build: sipstack dep first_rule
# END ARRIS CHANGE

export COMPONENT_INCLUDES := $(patsubst %,-I%, $(shell find $(PACM_PREFIX) -name include -type d)) -I$(TARGET_HOME)/ti/arris_libs/src/llist
export COMPONENT_PREFIX := $(PACM_PREFIX)

include $(PACM_PREFIX)/build/rules.make

config: config-sub_dirs
#	@install --preserve-timestamps -m 0644 $(pacm-headers-list) $(TI_include)

install: install-sub_dirs

clean: clean_sipstack uninstall clean-sub_dirs
	@find . \( -name '*.[oas]' -o -name core -o -name '*~' -o -name '.*~' -o -name '.#*' -o -name '*.so' \) -follow -type f -print | xargs rm -f

distclean: clean
	@find . \( -name '.depend' \) -type f -print -follow | xargs rm -f

uninstall: uninstall-sub_dirs
	@rm -f $(patsubst ./include/%,$(TI_include)/%,$(pacm-headers-list))

build_pre:

build_post:

# ARRIS ADD - Add SIP Stack Build
ifdef CONFIG_SIP
sipstack:
	@echo --- BUILDING SIP STACK ---
	@$(MAKE) -C $(TARGET_HOME)/../SIP libs nosamples=on MVISTA=on VER=4.0
	@find $(RV_LIB_PATH) -type f -name librv* | xargs -i install -m 0755 {} $(FS_LIB_PATH)

clean_sipstack:
	@echo --- CLEANING SIP STACK ---
	@$(MAKE) -C $(TARGET_HOME)/../SIP clean
	@find $(FS_LIB_PATH) -type f -name librv* | xargs -i rm -f {}

else
sipstack:

clean_sipstack:
endif

# END ARRIS ADD
