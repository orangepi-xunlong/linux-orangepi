# SPDX-License-Identifier: GPL-2.0
#
#  Makefile for the Linux Controller Area Network drivers.
#

#ccflags-y += -O0 -Wno-error
ifeq ($(CC),clang)
	obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink_asm.o
else
ifeq ($(CONFIG_AWLINK_SUPPORT_KERNEL_6_6), y)
	obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink_asm_linux6.6.o
else
	obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink_asm_linux.o
endif
endif

obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink.o
