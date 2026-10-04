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

-include $(TARGET_HOME)/.config

# Debugging
ifeq ($(CONFIG_TI_COMMON_COMPONENTS_DEBUG),y)
# ARRIS CHANGE - Change O0 to O1
CFLAGS-private := -g -O1
# END ARRIS CHANGE
COMPONENT_DEBUG := y
else
ifeq ($(CONFIG_TI_COMMON_COMPONENTS_SIZE_OPTIMIZATION),y)
CFLAGS-private := -Os
else
CFLAGS-private := -O2
endif
endif

# DSG Compile Flag Definition
ifeq ($(CONFIG_DSG),y)
export CFLAGS += -DDSG
endif
AUTO_GENERATE_H_FILES := $(shell ls ../include/*.h 2> /dev/null)
AUTO_GENERATE_C_FILES := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))

# The following creates unique CLI command and sub-menu names per component
CFLAGS-private += -DCLI_COMPONENT_NAME=CC


# ARRIS ADD - Makefile debug stuff
#UMF_DEBUG_LEVEL := 3
#UMF_LINK_ANALYSIS := y

