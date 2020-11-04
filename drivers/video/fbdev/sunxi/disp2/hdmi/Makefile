obj-$(CONFIG_HDMI_DISP2_SUNXI) += hdmi.o

hdmi-y := drv_hdmi.o hdmi_core.o hdmi_edid.o
ifeq ($(CONFIG_ARM64),y)
$(shell cp $(obj)/libhdmi_sun50iw1 $(obj)/libhdmi_sun50iw1.a)
hdmi-$(CONFIG_ARCH_SUN50IW1P1) += libhdmi_sun50iw1.a
hdmi-$(CONFIG_ARCH_SUN50IW2P1) += libhdmi_sun50iw1.a
endif

ifeq ($(CONFIG_ARM),y)
$(shell cp $(obj)/libhdmi_sun8iw11 $(obj)/libhdmi_sun8iw11.a)
hdmi-$(CONFIG_ARCH_SUN50IW1P1) += libhdmi_sun8iw11.a
hdmi-$(CONFIG_ARCH_SUN50IW2P1) += libhdmi_sun8iw11.a
hdmi-$(CONFIG_ARCH_SUN8IW11)   += libhdmi_sun8iw11.a
hdmi-$(CONFIG_ARCH_SUN8IW12) += hdmi_bsp_sun8iw12.o
hdmi-$(CONFIG_ARCH_SUN8IW7) += hdmi_bsp_sun8iw7.o
hdmi-$(CONFIG_ARCH_SUN8IW6) += hdmi_bsp_sun8iw6.o
endif

