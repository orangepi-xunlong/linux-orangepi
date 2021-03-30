
#
# Makefile for wifi bluetooth power controller drivers
#

#obj-m += virtual_source.o
#obj-y += virtual_tvd.o

sunxi_car_reverse-y := buffer_pool.o \
	video_source.o \
	preview.o \
	car_reverse.o

ifeq ($(CONFIG_SUPPORT_AUXILIARY_LINE),y)
	sunxi_car_reverse-y += auxiliary_line.o
endif

obj-$(CONFIG_VIDEO_SUNXI_CAR_REVERSE) += sunxi_car_reverse.o
