# SPDX-License-Identifier: GPL-2.0-only
soph_drm-y := cvitek_drm.o cvitek_vo_sys_reg.o cvitek_disp.o cvitek_mipipll_cfg.o

soph_drm-$(CONFIG_CVITEK_MIPI_DSI) += cvitek_dsi.o
soph_drm-$(CONFIG_CVITEK_HDMI) += cvitek_dw_hdmi.o
soph_drm-$(CONFIG_CVITEK_LVDS) += cvitek_lvds.o

obj-$(CONFIG_DRM_CVITEK) += soph_drm.o
