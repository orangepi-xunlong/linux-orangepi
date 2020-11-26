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

GPU_BUILD_TYPE = release

include $(KERNEL_DIR)/.config

ifdef CONFIG_OF
export USING_DT = 1
endif

##############################################################
##############     Set WINDOW_SYSTEM_TYPE      ###############
##############################################################
ifeq ($(LICHEE_PLATFORM), android)
WINDOW_SYSTEM_TYPE = android
else ifeq ($(LICHEE_PLATFORM), secureandroid)
WINDOW_SYSTEM_TYPE = android
else ifeq ($(LICHEE_PLATFORM), camdroid)
WINDOW_SYSTEM_TYPE = android
else ifeq ($(LICHEE_PLATFORM), linux)
WINDOW_SYSTEM_TYPE = linux
else ifeq ($(LICHEE_PLATFORM), dragonboard)
WINDOW_SYSTEM_TYPE = linux
else
$(error LICHEE_PLATFORM is invalid)
endif

##############################################################
##################     Set GPU_TYPE         ##################
##############################################################
GPU_TYPE = $(CONFIG_SUNXI_GPU_TYPE)

##############################################################
#############  Set GPU_BUILD_DIR and GPU_KO_NAME #############
##############################################################
ifeq ($(strip $(GPU_TYPE)), $(filter $(GPU_TYPE), "mali400" "mali450"))
GPU_BUILD_DIR = mali-utgard/kernel_mode/driver/src/devicedrv/mali
GPU_KO_NAME   = $(GPU_BUILD_DIR)/mali.ko
else ifeq ($(strip $(GPU_TYPE)), $(filter $(GPU_TYPE), "mali-t720" "mali-t760"))
GPU_BUILD_DIR = mali-midgard/kernel_mode/driver
GPU_KO_NAME   = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
GPU_KO_NAME  += $(GPU_BUILD_DIR)/drivers/base/kds/kds.ko
GPU_KO_NAME  += $(GPU_BUILD_DIR)/drivers/base/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifeq ($(GPU_TYPE), "sgx544")
GPU_BUILD_DIR = sgx544/$(WINDOW_SYSTEM_TYPE)/kernel_mode/eurasia_km/eurasiacon/build/linux2/sunxi_$(WINDOW_SYSTEM_TYPE)
GPU_KO_NAME   = sgx544/$(WINDOW_SYSTEM_TYPE)/kernel_mode/eurasia_km/eurasiacon/binary2_sunxi_$(WINDOW_SYSTEM_TYPE)_$(GPU_BUILD_TYPE)/target/pvrsrvkm.ko
ifeq ($(WINDOW_SYSTEM_TYPE), linux)
GPU_KO_NAME   += sgx544/$(WINDOW_SYSTEM_TYPE)/kernel_mode/eurasia_km/eurasiacon/binary2_sunxi_$(WINDOW_SYSTEM_TYPE)_$(GPU_BUILD_TYPE)/target/dc_sunxi.ko
endif
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

install:
	@cp -uv $(GPU_KO_NAME) $(LICHEE_MOD_DIR);

clean:
	$(MAKE) -C $(GPU_BUILD_DIR) clean
