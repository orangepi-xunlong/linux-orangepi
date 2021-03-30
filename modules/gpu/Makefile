#
# Copyright (C) 2015-2016 Allwinner Technology Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
#
# Author: Albert Yu <yuxyun@allwinnertech.com>
#

ifneq ($(LICHEE_KDIR),)
export KERNEL_DIR = $(LICHEE_KDIR)
else
$(error LICHEE_KDIR is empty)
endif

export KDIR = $(KERNEL_DIR)
export KERNELDIR = $(KERNEL_DIR)
export ARCH
export CROSS_COMPILER

#GPU_BUILD_TYPE = release
GPU_BUILD_TYPE = debug

include $(KERNEL_DIR)/.config

ifdef CONFIG_OF
export USING_DT = 1
endif

##############################################################
##############     Set WINDOW_SYSTEM_TYPE      ###############
##############################################################
WINDOW_SYSTEM_TYPE = linux

##############################################################
##################     Set GPU_TYPE         ##################
##############################################################
GPU_TYPE = $(CONFIG_SUNXI_GPU_TYPE)

##############################################################
#############  Set GPU_BUILD_DIR and GPU_KO_NAME #############
##############################################################
ifeq ($(GPU_TYPE), "mali400")
GPU_BUILD_DIR = mali400/kernel_mode/driver/src/devicedrv/mali
GPU_KO_NAME   = $(GPU_BUILD_DIR)/mali.ko
else ifeq ($(GPU_TYPE), "mali450")
GPU_BUILD_DIR = mali450/kernel_mode/driver/src/devicedrv/mali
UMP_BUILD_DIR = mali450/kernel_mode/driver/src/devicedrv/ump
UMPLOCK_BUILD_DIR = mali450/kernel_mode/driver/src/devicedrv/umplock
GPU_KO_NAME   = $(GPU_BUILD_DIR)/mali.ko
UMP_KO_NAME   = $(UMP_BUILD_DIR)/ump.ko
UMPLOCK_KO_NAME = $(UMPLOCK_BUILD_DIR)/umplock.ko
else ifeq ($(GPU_TYPE), "sgx544")
GPU_BUILD_DIR = sgx544/$(WINDOW_SYSTEM_TYPE)/kernel_mode/eurasia_km/eurasiacon/build/linux2/sunxi_android
GPU_KO_NAME   = sgx544/$(WINDOW_SYSTEM_TYPE)/kernel_mode/eurasia_km/eurasiacon/binary2_sunxi_$(WINDOW_SYSTEM_TYPE)_$(GPU_BUILD_TYPE)/target/pvrsrvkm.ko
else ifeq ($(GPU_TYPE), "rgx6230")
GPU_BUILD_DIR = rgx6230/$(WINDOW_SYSTEM_TYPE)/kernel_mode/rogue_km/build/linux/sunxi_$(WINDOW_SYSTEM_TYPE)
GPU_KO_NAME   = rgx6230/$(WINDOW_SYSTEM_TYPE)/kernel_mode/rogue_km/binary_sunxi_$(WINDOW_SYSTEM_TYPE)_$(GPU_BUILD_TYPE)/target_armv7-a/pvrsrvkm.ko
else
$(error The GPU type $(GPU_TYPE) is invalid)
endif

##############################################################

.PHONY: all build install clean

all: build install

build:
	$(MAKE) -j -C $(GPU_BUILD_DIR) BUILD=$(GPU_BUILD_TYPE)
	$(MAKE) -j -C $(UMPLOCK_BUILD_DIR) BUILD=$(GPU_BUILD_TYPE) 
	$(MAKE) -j -C $(UMP_BUILD_DIR) BUILD=$(GPU_BUILD_TYPE) 

install:
	@cp -uv $(GPU_KO_NAME) $(LICHEE_MOD_DIR);
	@cp -uv $(UMP_KO_NAME) $(LICHEE_MOD_DIR);
	@cp -uv $(UMPLOCK_KO_NAME) $(LICHEE_MOD_DIR);

clean:
	$(MAKE) -C $(GPU_BUILD_DIR) clean
