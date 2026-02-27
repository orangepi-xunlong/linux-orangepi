# SPDX-License-Identifier: GPL-2.0-only
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_lcd/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_tcon/
ccflags-y += -I$(srctree)/$(obj)/phy/
ccflags-y += -I$(srctree)/$(obj)/lib/inc/

# base
sunxidrm-y += sunxi_drm_drv.o sunxi_drm_crtc.o sunxi_drm_intf.o sunxi_drm_notify.o
sunxidrm-y += sunxi_drm_gem.o
sunxidrm-y += sunxi_drm_debug.o kernel_panic_printf.o

#fbdev
ifdef CONFIG_FB
sunxidrm-y += sunxi_fbdev_core.o
else
sunxidrm-y += sunxi_drmfb_core.o
endif
sunxidrm-y += sunxi_fbdev_platform.o

# de
include $(srctree)/$(obj)/sunxi_device/hardware/lowlevel_de/Makefile
sunxidrm-$(CONFIG_AW_DRM_DE) += $(de_obj)

# tcon
include $(srctree)/$(obj)/sunxi_device/hardware/lowlevel_tcon/Makefile
sunxidrm-$(CONFIG_AW_DRM_TCON) += $(tcon_obj) \
				  sunxi_device/sunxi_tcon.o \
				  sunxi_device/hardware/lowlevel_lcd/tcon_lcd.o

# tcon_top
sunxidrm-$(CONFIG_AW_DRM_TCON_TOP) += sunxi_device/sunxi_tcon_top.o

# dsi
sunxidrm-$(CONFIG_AW_DRM_DSI) += sunxi_drm_dsi.o \
				 sunxi_device/hardware/lowlevel_lcd/dsi_v1.o

# lvds
sunxidrm-$(CONFIG_AW_DRM_LVDS) += sunxi_drm_lvds.o

# rgb
sunxidrm-$(CONFIG_AW_DRM_RGB) += sunxi_drm_rgb.o

# HDMI
sunxidrm-$(CONFIG_AW_DRM_HDMI_TX) += sunxi_drm_hdmi.o
sunxidrm-$(CONFIG_AW_DRM_HDMI_TX) += sunxi_device/sunxi_hdmi.o
ccflags-$(CONFIG_AW_DRM_HDMI20) += -I$(src)/sunxi_device/hardware/lowlevel_hdmi20/
include $(srctree)/$(obj)/sunxi_device/hardware/lowlevel_hdmi20/Makefile
sunxidrm-$(CONFIG_AW_DRM_HDMI20) += $(obj_low_hdmi)

# edp
sunxidrm-$(CONFIG_AW_DRM_EDP) += sunxi_drm_edp.o
sunxidrm-$(CONFIG_AW_DRM_EDP) += sunxi_device/sunxi_edp.o
sunxidrm-$(CONFIG_AW_DRM_DP_HDCP) += sunxi_device/sunxi_edp_hdcp.o
sunxidrm-$(CONFIG_AW_DRM_DP_HDCP) += lib/src/sha1.o
sunxidrm-$(CONFIG_AW_DRM_INNO_EDP13) += sunxi_device/hardware/lowlevel_edp/inno_edp13/inno_edp13.o
sunxidrm-$(CONFIG_AW_DRM_TRILINEAR_EDP14) += sunxi_device/hardware/lowlevel_edp/trilinear_dp14/trilinear_dp14.o

# phy
obj-$(CONFIG_AW_DRM_PHY) += phy/

# panel
obj-y += panel/

# tracepoints
# define_trace.h needs to know how to find our header
CFLAGS_sunxi_drm_trace.o := -I$(src)
sunxidrm-y += sunxi_drm_trace.o

obj-$(CONFIG_AW_DRM)        += sunxidrm.o
