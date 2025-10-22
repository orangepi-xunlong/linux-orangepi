# SPDX-License-Identifier: GPL-2.0-or-later
#
# Makefile for the Linux edp2 driver
#
# Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
# Author: huangyongxing <huangyongxing@allwinnertech.com>
#
#
obj-$(CONFIG_EDP2_AW_DISP2) += edp2.o
edp2-y := drv_edp2.o
edp2-y += edp_core/edp_core.o
edp2-y += edp_core/edp_edid.o

edp2-$(CONFIG_DISP_PHY_INNO_EDP_1_3) += lowlevel/inno_edp13_lowlevel.o

edp2-y += panels/edp_panels.o
edp2-$(CONFIG_EDP_SUPPORT_VVX10T025J00_2560X1600) += panels/VVX10T025J00_2560X1600.o
edp2-$(CONFIG_EDP_SUPPORT_LM116X001A40_1920X1080) += panels/LM116X001A40_1920X1080.o
edp2-$(CONFIG_EDP_SUPPORT_GENERAL_PANEL) += panels/general_panel.o

ccflags-y := -DDYNAMIC_DEBUG_MODULE
