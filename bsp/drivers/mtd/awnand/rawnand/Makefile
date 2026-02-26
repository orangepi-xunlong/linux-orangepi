#
# SPDX-License-Identifier: GPL-2.0+
# (C) Copyright 2020 - 2021
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# cuizhikui <cuizhikui@allwinnertech.com>
#
# Makefile for the RAWNAND(SLC) MTD
#

obj-$(CONFIG_AW_MTD_RAWNAND) += aw-rawnand-slc.o

ifdef CONFIG_ARCH_SUN8IW11
aw-rawnand-slc-objs += aw_rawnand_nfc_v0.o
else
aw-rawnand-slc-objs += aw_rawnand_nfc.o
endif

aw-rawnand-slc-objs += aw_rawnand_base.o
aw-rawnand-slc-objs += aw_rawnand_ids.o aw_rawnand_bbt.o
aw-rawnand-slc-objs += aw_rawnand_securestorage.o aw_rawnand_spl.o
