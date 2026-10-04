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

-include $(TARGET_HOME)/.config


# Debugging
ifeq ($(CONFIG_TI_DOCSIS_DEBUG),y)

# Enable debug mode
CFLAGS-private += -g -ggdb -O1 -fno-omit-frame-pointer
COMPONENT_DEBUG := y
else
# Default optimization for speed
CFLAGS-private += -O2
endif

# DSG Compile Flag Definition
ifeq ($(CONFIG_DSG),y)
export CFLAGS += -DDSG
ifeq ($(CONFIG_HTX_DSG_RPC),y)
export CFLAGS += -DHTX_DSG_RPC
endif
ifeq ($(CONFIG_DSG_CCIF_PROXY),y)
export CFLAGS += -DDSG_CCIF_PROXY
endif
ifeq ($(CONFIG_DSG_INTEL_PARKER),y)
export CFLAGS += -DDSG_INTEL_PARKER
endif
endif
AUTO_GENERATE_H_FILES := $(shell ls ../include/*.h 2> /dev/null)
AUTO_GENERATE_C_FILES := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))

# The following 3 defines are used for debug purposes only, in order to print events as strings
# instead of numbers on SME messages.
# The first 2 defines are used for log messages in SME_CreateSM_ex() and the third is the name of
# the components array of structs that contain for each module (events enum) upper bound\lowwer bound
# and module events names array.

CFLAGS-private += -DSME_DEFAULT_LOG_COMPONENT=TI_COMPONENT_DOCSIS
CFLAGS-private += -DSME_DEFAULT_LOG_MODULE=DOCSIS_MOD_SME
CFLAGS-private += -DSME_DEFAULT_EVENT_NAMES_TABLE=DocsisEventIdNamesModulePtr___

# The following creates unique CLI command and sub-menu names per component
CFLAGS-private += -DCLI_COMPONENT_NAME=DOCSIS

# ARRIS CHANGE L_ALL_DOCSIS
L_ALL_DOCSIS := -lcrypto -lti_sme -lsme -lticc -lall_docsis -lm
ifeq ($(CONFIG_DSG),y)
L_ALL_DOCSIS += -lpthread
endif
ifeq ($(CONFIG_SYSTEM_MOCA2_0),y)
L_ALL_DOCSIS += -lmoca_api
endif
USO_POSTFIX := _all_subso
