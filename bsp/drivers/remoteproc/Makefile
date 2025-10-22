# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the remoteproc drivers.
#
ccflags-y += -I $(srctree)/drivers/remoteproc
obj-$(CONFIG_AW_REMOTEPROC_E906_BOOT) += sunxi_rproc_e906_boot.o
obj-$(CONFIG_AW_REMOTEPROC_E907_BOOT) += sunxi_rproc_e907_boot.o
obj-$(CONFIG_AW_REMOTEPROC_HIFI4_BOOT) += sunxi_rproc_hifi4_boot.o
obj-$(CONFIG_AW_REMOTEPROC)		+= sunxi_remoteproc.o
sunxi_remoteproc-y			+= sunxi_rproc.o
sunxi_remoteproc-y			+= sunxi_rproc_boot.o
sunxi_remoteproc-y			+= sunxi_rproc_standby.o
obj-$(CONFIG_AW_RPROC_FAST_BOOT) += sunxi_rproc_firmware.o
obj-$(CONFIG_AW_RPROC_ENHANCED_TRACE) += sunxi_rproc_trace.o
obj-$(CONFIG_SUNXI_RPROC_SHARE_IRQ)	+= sunxi_share_interrupt/sunxi_share_interrupt.o
