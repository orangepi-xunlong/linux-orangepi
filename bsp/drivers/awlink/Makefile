# SPDX-License-Identifier: GPL-2.0
#
#  Makefile for the Linux Controller Area Network drivers.
#

#ccflags-y += -O0 -Wno-error
ifeq ($(CC),clang)
	obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink_asm.o
else
	obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink_asm_linux.o
endif

obj-$(CONFIG_AW_AWLINK_SUN55I)		+= sunxi_awlink.o
