# SPDX-License-Identifier: GPL-2.0-only
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

ccflags-y += -I$(mkfile_dir)/../sunxi_device/hardware

# Although now only support v35x, new platform support should not add new dir
ccflags-y += -I$(mkfile_dir)/../sunxi_device/hardware/lowlevel_v35x/tcon
ccflags-y += -I$(mkfile_dir)/../sunxi_device/hardware/lowlevel_v35x/

obj-$(CONFIG_PANEL_EDP_GENERAL) += edp_general_panel.o
obj-$(CONFIG_PANEL_DSI_GENERAL) += panel-dsi.o
obj-$(CONFIG_PANEL_LVDS_GENERAL) += panel-lvds.o
obj-$(CONFIG_PANEL_RGB_GENERAL) += panel-rgb.o
