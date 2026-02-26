#
# Copyright (C) 2015-2019 Allwinner Technology Limited. All rights reserved.
#
# This program is free software and is provided to you under the terms of the GNU
# General Public License version 2 as published by the Free Software Foundation, and
# any use by you of this program is subject to the terms of such GNU licence.
#
# Author: Albert Yu <yuxyun@allwinnertech.com>
#

# Do not use allwinner private variables. But for compatible solution of old build
# scritp, load it as default values. Please remove it in the furture.
# All need setup:
#
# KERNEL_SRC_DIR  (KERNEL_SRC)
# KERNEL_OUT_DIR  (O)
# MODULE_INS_DIR  (INSTALL_MOD_PATH)

# Init all variables
KERNEL_SRC_DIR := $(LICHEE_KERN_DIR)
CONFIG_OS_TYPE := $(LICHEE_PLATFORM)
MODULE_INS_DIR := $(LICHEE_MOD_DIR)
KERNEL_OUT_DIR := $(O)

# Re-setup it
KERNEL_SRC_DIR := $(word 1, $(KERNEL_SRC_DIR) $(KERNEL_SRC))
KERNEL_OUT_DIR := $(word 1, $(KERNEL_OUT_DIR) $(KERNEL_SRC_DIR))
CONFIG_OS_TYPE := $(word 1, $(CONFIG_OS_TYPE) android)

ifeq ($(KERNEL_SRC_DIR),)
  $(error KERNEL_SRC_DIR not setup)
endif

KERNEL_VERSION := $(shell awk -F\" '/UTS_RELEASE/{print $$2}' $(KERNEL_OUT_DIR)/include/generated/utsrelease.h 2>/dev/null)

ifneq ($(INSTALL_MOD_PATH),)
  MODULE_INS_DIR := $(INSTALL_MOD_PATH)/lib/modules/$(KERNEL_VERSION)
  MODULE_INS_DIR := $(patsubst %/, %, $(MODULE_INS_DIR))
endif

# OUT/TOP maybe cause gpu build out not right, unexport it.
unexport OUT
unexport TOP

export KDIR = $(KERNEL_SRC_DIR)
export KERNELDIR = $(LICHEE_OUT_DIR)/kernel/build

ifneq ($(KERNEL_OUT_DIR),)
  -include $(KERNEL_OUT_DIR)/.config
else
  -include .buildconfig
  -include $(LICHEE_PLAT_OUT)/.config
endif

ifdef CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND
  export CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND = 1
endif


ifeq ($(CONFIG_ARM), y)
  CPU_ARCH := arm
else
ifeq ($(CONFIG_ARM64), y)
  CPU_ARCH := arm64
else
ifeq ($(LICHEE_ARCH), arm)
  CPU_ARCH := arm
else
ifeq ($(LICHEE_ARCH), arm64)
  CPU_ARCH := arm64
else
  $(warning neither CONFIG_ARM nor CONFIG_ARM64 is found in .config, unsupport)
  GPU_TYPE := "unsupport"
endif # LICHEE_ARCH=arm64
endif # LICHEE_ARCH=arm
endif # CONFIG_ARM64
endif # CONFIG_ARM

GPU_TYPE       := $(word 1, $(GPU_TYPE) $(CONFIG_AW_GPU_TYPE))
GPU_BUILD_TYPE ?= release
##############################################################
##########     Set GPU_BUILD_DIR and GPU_KO_NAME     #########
##############################################################
GPU_BUILD_DIR :=
GPU_KO_NAME   :=
KO_DIR :=
ifneq ($(findstring $(GPU_TYPE), "mali400" "mali450"),)
  GPU_BUILD_DIR = mali-utgard/driver/src/devicedrv/mali
  GPU_KO_NAME = $(GPU_BUILD_DIR)/mali.ko
else ifneq ($(findstring $(GPU_TYPE), "mali-t720" "mali-t760"),)
  GPU_BUILD_DIR = mali-midgard/driver
  GPU_KO_NAME    = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
  GPU_KO_NAME   += $(GPU_BUILD_DIR)/drivers/base/kds/kds.ko
  GPU_KO_NAME   += $(GPU_BUILD_DIR)/drivers/base/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifneq ($(findstring $(GPU_TYPE), "mali-g31"),)
  GPU_BUILD_DIR = mali-bifrost/driver
  GPU_KO_NAME    = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
  GPU_KO_NAME   += $(GPU_BUILD_DIR)/drivers/base/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifneq ($(findstring $(GPU_TYPE), "mali-g57"),)
  GPU_BUILD_DIR = mali-valhall/driver
  GPU_KO_NAME    = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
  GPU_KO_NAME   += $(GPU_BUILD_DIR)/drivers/base/arm/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifneq ($(findstring $(GPU_TYPE), "mali-g57-r32p1"),)
  GPU_BUILD_DIR = mali-valhall/driver_r32p1
  GPU_KO_NAME    = $(GPU_BUILD_DIR)/drivers/gpu/arm/midgard/mali_kbase.ko
  GPU_KO_NAME   += $(GPU_BUILD_DIR)/drivers/base/arm/dma_buf_test_exporter/dma-buf-test-exporter.ko
else ifneq ($(findstring $(GPU_TYPE), "sgx544"),)
  ifeq ($(CPU_ARCH), arm)
    GPU_BUILD_DIR = img-sgx/$(CONFIG_OS_TYPE)/eurasia_km/eurasiacon/build/linux2/sunxi_$(CONFIG_OS_TYPE)
    KO_DIR   := img-sgx/$(CONFIG_OS_TYPE)/eurasia_km/eurasiacon/binary2_sunxi_$(CONFIG_OS_TYPE)_$(GPU_BUILD_TYPE)/target
    ifeq ($(CONFIG_OS_TYPE), linux)
      GPU_KO_NAME = $(KO_DIR)/dc_sunxi.ko
    else
      GPU_KO_NAME = $(KO_DIR)/pvrsrvkm.ko
    endif
  endif
else ifneq ($(findstring $(GPU_TYPE), "rgx6230" "ge8300"),)
  GPU_BUILD_DIR = img-rgx/$(CONFIG_OS_TYPE)/rogue_km/build/linux/sunxi_$(CONFIG_OS_TYPE)
  ifeq ($(CONFIG_OS_TYPE), android)
    WINDOW_SYSTEM = _
  else
    WINDOW_SYSTEM = _nullws_
  endif
  ifeq ($(CPU_ARCH), arm)
    KO_DIR := img-rgx/$(CONFIG_OS_TYPE)/rogue_km/binary_sunxi_$(CONFIG_OS_TYPE)$(WINDOW_SYSTEM)$(GPU_BUILD_TYPE)/target_armv7-a
  else
    KO_DIR := img-rgx/$(CONFIG_OS_TYPE)/rogue_km/binary_sunxi_$(CONFIG_OS_TYPE)$(WINDOW_SYSTEM)$(GPU_BUILD_TYPE)/target_aarch64
  endif

  GPU_KO_NAME = $(KO_DIR)/pvrsrvkm.ko
  ifeq ($(CONFIG_OS_TYPE), linux)
    GPU_KO_NAME += $(KO_DIR)/dc_sunxi.ko
  endif
else ifneq ($(findstring $(GPU_TYPE), "bxm"),)
  GPU_BUILD_DIR = img-bxm/$(CONFIG_OS_TYPE)/rogue_km/build/linux/sunxi_$(CONFIG_OS_TYPE)

  ifeq ($(CONFIG_OS_TYPE), linux)
    ifeq ($(CPU_ARCH), arm)
      KO_DIR := img-bxm/$(CONFIG_OS_TYPE)/rogue_km/binary_sunxi_$(CONFIG_OS_TYPE)_nulldrmws_$(GPU_BUILD_TYPE)/target_armv7-a/kbuild
    else
      KO_DIR := img-bxm/$(CONFIG_OS_TYPE)/rogue_km/binary_sunxi_$(CONFIG_OS_TYPE)_nulldrmws_$(GPU_BUILD_TYPE)/target_aarch64/kbuild
    endif
  endif

  GPU_KO_NAME = $(KO_DIR)/pvrsrvkm.ko
endif

##############################################################
.PHONY: all build modules_install clean

ifneq ($(GPU_BUILD_DIR),)
build:
	$(MAKE) -C $(GPU_BUILD_DIR) BUILD=$(GPU_BUILD_TYPE)

modules_install: $(GPU_KO_NAME)
	@if [ -z "$(MODULE_INS_DIR)" ]; then \
		echo "MODULE_INS_DIR not configured, nothing to do"; \
	else \
		mkdir -p $(MODULE_INS_DIR); \
		mv -v $(GPU_KO_NAME) $(MODULE_INS_DIR); \
	fi

clean:
	$(MAKE) -C $(GPU_BUILD_DIR) clean
else
all:
	@echo "Nothing to do for unsupport configuration"

modules_install: all

clean: all
endif
