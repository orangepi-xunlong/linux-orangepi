# Makefile for the Linux video drivers on sunxi platform.
# 16 Aug 2013, Tyle, <mailto:tyle@allwinnertech.com>
# Rewritten to use lists instead of if-statements.


obj-$(CONFIG_FB_SUNXI)     += disp/
obj-$(CONFIG_TV_GM7121_DISP1)    += tv/
obj-$(CONFIG_LCD_SUNXI)    += lcd/
obj-$(CONFIG_HDMI_SUNXI)   += hdmi/

obj-$(CONFIG_FB_SUNXI_LEGACY)     += legacy/disp/
obj-$(CONFIG_LCD_SUNXI_LEGACY)    += legacy/lcd/
obj-$(CONFIG_HDMI_SUNXI_LEGACY)   += legacy/hdmi/

obj-$(CONFIG_DISP2_SUNXI) += disp2/disp/
obj-$(CONFIG_HDMI_DISP2_SUNXI) += disp2/hdmi/
obj-$(CONFIG_DISP2_SUNXI) += disp2/tv/
obj-$(CONFIG_HDMI_EP952) += disp2/hdmi_ep952/
