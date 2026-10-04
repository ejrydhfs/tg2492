# Copyright (C) 1998-2019 Intel Corporation
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyight notice,
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

include $(DOCSIS_PREFIX)/build/docsis_config.make

#
# Final output target
#
# Select one (or more) targets to be generated from this Makefile:
#
# TARGET	- Binary executable file
# A_TARGET	- Static library file (.a file)
# SO_TARGET	- Shared library file (.so file)
# O_TARGET	- Object file (.o file)
#
A_TARGET :=

#
# Compilation options to add to the compiler (optional)
#
cflags-y :=

# Export .h files in these dirs to all sub dirs

# ARRIS ADD puma6 kernel dir for gpio
export CFLAGS += \
	-I$(CURDIR)/hal_infrastructure/hal_mac_regs \
	-I$(CURDIR)/hal_infrastructure/hal_upstream_mbx/src \
	-I$(CURDIR)/hal_infrastructure/hal_isr/mlx/mask_lib \
	-I$(CURDIR)/hal_infrastructure/hal_soc_interface_driver \
	-I$(CURDIR)/hal_infrastructure/hal_soc_interface_driver/mpeg_out \
	-I$(CURDIR)/hal_infrastructure/hal_soc_interface_driver/mpeg_encap \
	-I$(CURDIR)/hal_infrastructure/hal_soc_interface_driver/docsis_global \
        -I$(KERNEL_DIR)/include/asm-arm/arch-avalanche/puma6 \

#
# Link options to add to the linker (optional)
# e.g. ldflags-y := -lfoo
#
ldflags-y :=

#
# List of sub-directories to compile (if required)
# e.g. subdir-y := mymodule1/src mymodule2/src
#
# HAL INFRASTRUCTURE
subdir-y := \
	hal_infrastructure/hal_reg_access/src/reg_access_lib \
	hal_infrastructure/hal_isr/mlx/mask_lib \
	hal_infrastructure/hal_phy/src \
	hal_vendor/hal_calibration/hal_ffs_calibration/src \
	hal_vendor/hal_calibration/hal_ds_calibration/src \
	hal_vendor/hal_calibration/hal_us_calibration/src \
	hal_infrastructure/hal_upstream_mbx/src/cmd_mbox_lib \
	hal_infrastructure/hal_upstream_mbx/src/event_mbox_lib \
	hal_infrastructure/hal_ds_fw_if/src \
	hal_infrastructure/hal_fw_download \
	hal_infrastructure/hal_i2c_if/src \
	hal_vendor/hal_tuners/hal_tuner_manager_api/src \
	hal_infrastructure/hal_db/src \


subdir-$(BUILD_POST) +=	hal_infrastructure/hal_isr \
	hal_services/hal_mng_q \
	hal_infrastructure/hal_soc_interface_driver


# HAL SERVICES
subdir-y +=		hal_services/hal_sync/src \
			hal_services/hal_mdd/src \
			hal_services/hal_global/src \
			hal_services/hal_rng/src \
			hal_services/hal_ucd/src \
			hal_services/hal_ucd_forwarding/src \
			hal_services/hal_filter/src \
			hal_services/hal_ds_channel_control/src \
			hal_services/hal_us_channel_control/src \
			hal_services/hal_qos/src \
			hal_services/hal_bpi/src \
			hal_services/hal_counter/src \
			hal_services/hal_phs/src \
			hal_services/hal_mpeg/src \
			hal_services/hal_dsid/src \
			hal_services/hal_oob/src \
			hal_services/hal_phy_ds_lock_monitor/src \

# Executables
subdir-y += \
	hal_infrastructure/hal_reg_access/src/reg_access_app \
	hal_infrastructure/hal_upstream_mbx/src/cmd_mbox_app \
	hal_infrastructure/hal_upstream_mbx/src/event_mbox_app \
	hal_vendor/hal_tuners/hal_tuner_manager_app/src \
	hal_infrastructure/hal_isr/mlx \
	hal_infrastructure/hal_isr/mlx/mpt_int \
#	hal_vendor/hal_tuners/test/src \
#	hal_services/hal_rng/test \
#	hal_services/hal_ucd/test \
#	hal_services/hal_qos/test \
#	hal_services/hal_bpi/test \
#	hal_services/hal_global/test \
#	hal_services/hal_phs/test \



# List of objects to compile
#
# The list could be either automatically generated from the C files in the
# current directory by the following syntax:
#
# obj-y := $(AUTO_GENERATE_C_FILES)
#
# or manually generated by explicitly specifing each C file:
#
# obj-y := obj1.o obj2.o ....
#
obj-y := $(AUTO_GENERATE_C_FILES)

#
# List of header files to install in the system include directory
#
# The list could be either automatically generated from the H files in the
# include directory by the following syntax:
#
# install_headers-y := $(AUTO_GENERATE_H_FILES)
#
# or manually generated by explicitly specifing each H file:
#
# install_headers-y := header1.h header2.h ....
#
install_headers-y :=

#
# List of additional files to clean (excluding object files and targets)
#
clean-y :=

#
# Make rules include file (Do not remove!)
#
include $(DOCSIS_PREFIX)/build/rules.make

#
# Additional module targets:
#

#
# Actions which require control here, before starting to build this module.
# If there is nothing to be done, this target should be left empty.
#
build_pre:

#
# Actions which require the control here, after this module has been built.
# If there is nothing to be done, this target should be left empty.
#
build_post:

#
# Target to install additional files
#
# Use the install command to copy additional files by the following syntax:
#
# @install -m 0755 <list of files> <destination directory>
#
install_extra:


