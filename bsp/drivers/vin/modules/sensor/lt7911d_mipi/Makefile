# SPDX-License-Identifier: GPL-2.0-or-later
# Makefile for the video capture/playback device drivers.
# Copyright 2010 Steven King <sfking@fdwdc.com>
#Copyright(c) 2022-2027 Allwinnertech Co., Ltd
# Each configuration option enables a list of files.
#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

obj-$(CONFIG_SENSOR_LT7911D) := lt7911d_mipi.o
lt7911d_mipi-objs := lt7911d_drv_mipi.o lt7911d_write_fireware.o

clean:
	@make -C $(LINUX_ROOT) M=$(PWD) clean
