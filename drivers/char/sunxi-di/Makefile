obj-$(CONFIG_SUNXI_DI) += deinterlace.o

ifeq ($(CONFIG_SUNXI_DI_V1XX),y)
sub_dir = drv_div1xx
endif

ifeq ($(CONFIG_SUNXI_DI_V2X),y)
sub_dir = drv_div2x
endif

ifeq ($(CONFIG_SUNXI_DI_V3X),y)
sub_dir = drv_div3x
endif

ifneq ($(sub_dir),)
-include $(obj)/$(sub_dir)/Makefile

#deinterlace-y += common/di_utils.o
deinterlace-y += $(sunxi_di)

endif

