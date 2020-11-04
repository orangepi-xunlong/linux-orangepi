sunxi_di_dir := drv_div3x
sunxi_di := $(sunxi_di_dir)/di_driver.o  \
			$(sunxi_di_dir)/di_client.o \
			$(sunxi_di_dir)/di_fops.o \
			$(sunxi_di_dir)/di_utils.o

ifeq ($(CONFIG_ARCH_SUN50IW9P1),y)
sunxi_di += $(sunxi_di_dir)/lowlevel_v3x/di300.o \
			$(sunxi_di_dir)/lowlevel_v3x/di300_alg.o
endif