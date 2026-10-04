# *************************************************************************
# * Makefile Wrapper file for building TACACS+
# *************************************************************************

#include $(TARGET_HOME)/TI_Build.make
include $(TARGET_HOME)/.config

PACKAGE_VERSION := 2010
PACKAGE_DIR := ntpclient-$(PACKAGE_VERSION)

first_rule: all

all: build_lib

build_lib: 
	@$(MAKE) -C $(CURDIR)/$(PACKAGE_DIR)

config_rule:

install_rule:
	@install -m 0755 $(CURDIR)/$(PACKAGE_DIR)/ntpclient $(TI_filesystem_path)/sbin/ntpclient

clean_rule:

uninstall_rule:
		
