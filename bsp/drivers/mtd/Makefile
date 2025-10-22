# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the memory technology device drivers.
#

# Core functionality.

ccflags-y += -I $(srctree)/include/linux/mtd

obj-$(CONFIG_AW_MTD)           		+= parsers/
obj-$(CONFIG_AW_MTD_SPI_NOR_6_1)	+= spi-nor-6.1/
obj-$(CONFIG_AW_MTD_SPI_NOR_5_15)	+= spi-nor-5.15/
obj-$(CONFIG_AW_MTD)			+= awnand/
obj-$(CONFIG_AW_MTD_PSTORE)		+= mtdpstore.o
