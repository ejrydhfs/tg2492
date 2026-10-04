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

include $(TARGET_HOME)/TI_Build.make

TARGET_DIR := fw_env
TARGET := $(TARGET_DIR)/printenv

all: build install

install_rule: install

clean_rule: clean

config_rule: config

uninstall_rule: uninstall

build: $(TARGET)


obj-y := $(TARGET_DIR)/crc32.o $(TARGET_DIR)/fw_env.o $(TARGET_DIR)/fw_env_main.o


$(TARGET): $(obj-y)
	@rm -f $@
	@echo "  LD	 	$@"
	@$(CC) $(LDFLAGS) -o $@ $(filter $(obj-y), $^)

install:
	@install -m 0755 $(TARGET) $(TI_filesystem_path)$(TI_sbindir)/$(notdir $(TARGET))
	@cd $(TI_filesystem_path)$(TI_sbindir) && ln -sf $(notdir $(TARGET)) ./setenv && ln -sf $(notdir $(TARGET)) ./getenv
ifdef TI_ENVUTILS_USE_CONFIG_FILE
	@install $(TARGET_DIR)/fw_env.config $(TI_filesystem_path)$(TI_etcdir)
endif

clean:
	@rm -f $(TARGET) $(obj-y)

config:

uninstall:
	@rm -f $(TI_filesystem_path)$(TI_sbindir)/$(notdir $(TARGET))
	@rm -f $(TI_filesystem_path)$(TI_etcdir)/fw_env.config
	@rm -f $(TI_filesystem_path)$(TI_sbindir)/setenv
	@rm -f $(TI_filesystem_path)$(TI_sbindir)/getenv

distclean: clean uninstall

%.o : %.c
	@echo "  CC [C] 	$@"
	@$(CC) $(CFLAGS) -c $< -o $@


