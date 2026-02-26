# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel multimedia device drivers.
#

# ccflags-y += -I $(srctree)/drivers/media/cedar-ve
ccflags-$(CONFIG_AW_VIDEO_DYNAMIC_DEBUG) += -DDYNAMIC_DEBUG_MODULE
ccflags-y += -I $(srctree)/bsp/drivers/ve/cedar-ve

obj-$(CONFIG_AW_VIDEO_ENCODER_DECODER) += sunxi-ve.o
sunxi-ve-objs := cedar_ve.o
ifneq (, $(filter y m, $(CONFIG_ARCH_SUN300IW1)))
sunxi-ve-objs += platforms/ve_plat_sun300iw1.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW6)))
sunxi-ve-objs += flush_cache.o
sunxi-ve-objs += platforms/ve_plat_sun55iw6.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN60IW2)))
sunxi-ve-objs += flush_cache.o
sunxi-ve-objs += platforms/ve_plat_sun60iw2.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN65IW1)))
sunxi-ve-objs += flush_cache.o
sunxi-ve-objs += platforms/ve_plat_sun65iw1.o
else
sunxi-ve-objs += flush_cache.o
sunxi-ve-objs += platforms/ve_plat_common.o
endif

# build video codec to sunxi-ve.
ifeq ($(or $(CONFIG_AW_VIDEO_KERNEL_ENC), $(CONFIG_AW_VIDEO_KERNEL_DEC)), y)
include $(src)/codec/Makefile
endif
