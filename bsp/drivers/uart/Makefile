# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the uart drivers.

CFLAGS_sunxi-uart-trace.o	:= -I$(src)

sunxi-uart-ng-objs := sunxi-uart-ng-core.o sunxi-uart-ng-dma.o sunxi-uart-trace.o

obj-$(CONFIG_AW_UART_NG)		+= sunxi-uart-ng.o
obj-$(CONFIG_AW_UART)			+= sunxi-uart.o
obj-$(CONFIG_AW_EARLY_PRINTK)		+= early_printk.o
obj-$(CONFIG_AW_IOSERIAL)		+= sunxi-ioserial.o
obj-$(CONFIG_AW_SOFT_UART)		+= sunxi-soft-uart/

ccflags-$(CONFIG_AW_UART_DYNAMIC_DEBUG) := -DDYNAMIC_DEBUG_MODULE
