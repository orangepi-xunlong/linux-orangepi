#
# Copyright (C) 2010-2012, 2014-2015 ARM Limited. All rights reserved.
# 
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
# 
# A copy of the licence is included with the program, and can also be obtained from Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

# For each arch check: CROSS_COMPILE , KDIR , CFLAGS += -DARCH

BUILD ?= debug

check_cc2 = \
	$(shell if $(1) -S -o /dev/null -xc /dev/null > /dev/null 2>&1; \
	then \
		echo "$(2)"; \
	else \
		echo "$(3)"; \
	fi ;)

# Check that required parameters are supplied.
ifeq ($(CONFIG),)
CONFIG := default
endif
ifeq ($(CPU)$(KDIR),)
$(error "KDIR or CPU must be specified.")
endif

# Get any user defined KDIR-<names> or maybe even a hardcoded KDIR
-include KDIR_CONFIGURATION

export CONFIG

export CONFIG_UMP := m
ifeq ($(BUILD),debug)
export CONFIG_UMP_DEBUG := y
else
export CONFIG_UMP_DEBUG := n
endif

ifeq ($(KDIR),)
$(error No KDIR found for platform $(CPU))
endif

all:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) modules

kernelrelease:
	$(MAKE) -C $(KDIR) kernelrelease

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	$(MAKE) -C $(KDIR) M=$(CURDIR)/../mali clean
