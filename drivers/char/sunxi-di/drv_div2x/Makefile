sunxi_di_dir := drv_div2x
sunxi_di := $(sunxi_di_dir)/sunxi-di.o

ifeq ($(CONFIG_ARCH_SUN50IW6P1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v2x/di_v2x.o
endif

ifeq ($(CONFIG_ARCH_SUN8IW12P1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v2x/di_v2x.o
endif

ifeq ($(CONFIG_ARCH_SUN8IW17P1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v2x/di_v2x.o
endif

ifeq ($(CONFIG_ARCH_SUN50IW2),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v1x/di_ebios.o \
			$(sunxi_di_dir)/lowlevel_v1x/di.o
endif

ifeq ($(CONFIG_ARCH_SUN8IW10),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v1x/di_ebios.o \
			$(sunxi_di_dir)/lowlevel_v1x/di.o
endif

ifeq ($(CONFIG_ARCH_SUN8IW11),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v1x/di_ebios.o \
			$(sunxi_di_dir)/lowlevel_v1x/di.o
endif

ifeq ($(CONFIG_ARCH_SUN3IW1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v1x/di_ebios_sun3i.o.o \
			$(sunxi_di_dir)/lowlevel_v1x/di.o
endif

ifeq ($(CONFIG_ARCH_SUN50IW1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v1x/di_ebios.o \
			$(sunxi_di_dir)/lowlevel_v1x/di.o
endif

ifeq ($(CONFIG_ARCH_SUN8IW7P1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v1x/di_ebios.o \
			$(sunxi_di_dir)/lowlevel_v1x/di.o
endif