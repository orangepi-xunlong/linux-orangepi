# SPDX-License-Identifier: GPL-2.0-or-later
obj-$(CONFIG_AW_HDMI_DISP2) += hdmi.o

ifdef LICHEE_BSP_DIR
HDMI_LIB_PATH=$(LICHEE_BSP_DIR)/drivers/video/sunxi/disp2/hdmi
else
HDMI_LIB_PATH=$(obj)
endif

obj_file = $(shell if [ -d "$(obj)" ]; then echo "exist"; else echo "noexist"; fi)
ifeq ("$(obj_file)", "noexist")
$(shell mkdir -p $(obj))
endif

ifeq ($(CONFIG_ARM64),y)
$(shell cp $(HDMI_LIB_PATH)/libhdmi_sun50iw1 $(obj)/libhdmi_sun50iw1.a)
endif
ifeq ($(CONFIG_ARM),y)
$(shell cp $(HDMI_LIB_PATH)/libhdmi_sun8iw11 $(obj)/libhdmi_sun8iw11.a)
endif

hdmi-y := drv_hdmi.o hdmi_core.o hdmi_edid.o
ifeq ($(CONFIG_ARM64),y)
hdmi-$(CONFIG_ARCH_SUN50IW1P1) += libhdmi_sun50iw1.a
hdmi-$(CONFIG_ARCH_SUN50IW2P1) += libhdmi_sun50iw1.a
endif

ifeq ($(CONFIG_ARM),y)
hdmi-$(CONFIG_ARCH_SUN50IW1P1) += libhdmi_sun8iw11.a
hdmi-$(CONFIG_ARCH_SUN50IW2P1) += libhdmi_sun8iw11.a
hdmi-$(CONFIG_ARCH_SUN8IW11)   += libhdmi_sun8iw11.a
hdmi-$(CONFIG_ARCH_SUN8IW12)   += hdmi_bsp_sun8iw12.o
hdmi-$(CONFIG_ARCH_SUN8IW7)    += hdmi_bsp_sun8iw7.o
hdmi-$(CONFIG_ARCH_SUN8IW6)    += hdmi_bsp_sun8iw6.o
endif
