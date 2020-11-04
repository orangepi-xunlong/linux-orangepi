obj-$(CONFIG_SUNXI_DI) += deinterlace.o

ifeq ($(CONFIG_ARCH_SUN50IW6P1),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN8IW12P1),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN8IW17P1),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN50IW2),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN8IW10),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN8IW11),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN3IW1),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN50IW1),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN8IW7P1),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_ARCH_SUN50IW9P1),y)
sub_dir = drv_div3x
endif

ifneq ($(sub_dir),)
-include $(obj)/$(sub_dir)/Makefile
deinterlace-y += $(sunxi_di)
endif

