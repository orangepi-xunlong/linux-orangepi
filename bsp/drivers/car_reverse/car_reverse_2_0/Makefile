# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the car_reverse drivers.
#

obj-$(CONFIG_VIDEO_SUNXI_CAR_REVERSE2) = sunxi_car_reverse.o

sunxi_car_reverse-y = buffer_pool.o \
	video_source.o \
	preview_display.o \
	car_reverse.o

#ifeq ($(CONFIG_SUPPORT_AUXILIARY_LINE),y)
ifneq ($(findstring $(CONFIG_SUPPORT_AUXILIARY_LINE), y m),)
sunxi_car_reverse-y += auxiliary_line.o
endif

#ifeq ($(CONFIG_SUPPORT_PREVIEW_ENHANCER),y)
ifneq ($(findstring $(CONFIG_SUPPORT_PREVIEW_ENHANCER), y m),)
sunxi_car_reverse-y += preview_enhancer.o
endif

#ifeq ($(CONFIG_SUPPORT_PREVIEW_ROTATOR),y)
ifneq ($(findstring $(CONFIG_SUPPORT_PREVIEW_ROTATOR), y m),)
sunxi_car_reverse-y += preview_rotator.o
endif

obj-$(CONFIG_CAR_REVERSE_SUPPORT_VIRTUAL_SOURCE) += virtual_source.o

ccflags-y := -DDYNAMIC_DEBUG_MODULE
