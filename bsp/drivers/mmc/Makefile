# SPDX-License-Identifier: GPL-2.0
#
# Makefile for MMC/SD host controller drivers
#
subdir-ccflags-$(CONFIG_MMC_DEBUG) := -DDEBUG
ccflags-y += -I $(srctree)/include/linux/reset
ccflags-y += -I $(srctree)/drivers/pinctrl/sunxi
ccflags-y +=  -I $(srctree)/drivers/mmc/core/
ccflags-y +=  -I $(srctree)/drivers/mmc/host
ccflags-y += -DDYNAMIC_DEBUG_MODULE
obj-$(CONFIG_AW_MMC)		+= sunxi_mmc_host.o
obj-$(CONFIG_AW_MMC_HSQ)               += mmc_hsq.o
sunxi_mmc_host-$(CONFIG_ARCH_SUN8IW10P1)         += sunxi-smhc.o sunxi-mmc-v5px.o
sunxi_mmc_host-$(CONFIG_ARCH_SUN50IW1P1)          += sunxi-mmc-sun50iw1p1-2.o sunxi-mmc-sun50iw1p1-1.o sunxi-mmc-sun50iw1p1-0.o
sunxi_mmc_host-$(CONFIG_AW_MMC_V4P1X)          += sunxi-mmc-v4p1x.o
sunxi_mmc_host-$(CONFIG_AW_MMC_V4P00X)          += sunxi-mmc-v4p00x.o
sunxi_mmc_host-$(CONFIG_AW_MMC_V4P10X)          += sunxi-mmc-v4p10x.o
sunxi_mmc_host-$(CONFIG_AW_MMC_V4P5X)          += sunxi-mmc-v4p5x.o
sunxi_mmc_host-$(CONFIG_AW_MMC_V5P3X)          += sunxi-mmc-v5p3x.o
sunxi_mmc_host-y                += sunxi-mmc.o sunxi-mmc-debug.o sunxi-mmc-export.o sunxi-mmc-panic.o
obj-$(CONFIG_AW_MMC_CQHCI)          += cqhci.o
cqhci-y				+=cqhci-core.o
cqhci-$(CONFIG_MMC_CRYPTO)              += cqhci-crypto.o

obj-$(CONFIG_AW_MMC_PSTORE_PANIC) += sunxi-mmc-pstore-panic.o
