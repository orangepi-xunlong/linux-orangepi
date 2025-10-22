# SPDX-License-Identifier: GPL-2.0-or-later
# Makefile for the video capture/playback device drivers.
# Copyright 2010 Steven King <sfking@fdwdc.com>
#Copyright(c) 2022-2027 Allwinnertech Co., Ltd
# Each configuration option enables a list of files.
#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

obj-$(CONFIG_SENSOR_RK628F) := rk628f.o
rk628f-objs := rk628f_mipi.o \
				rk628_twi_helper.o \
				rk628.o \
				rk628_combtxphy.o \
				rk628_combrxphy.o \
				rk628_cru.o \
				rk628_hdmirx.o \
				rk628_post_process.o \
				rk628_mipi_dphy.o
