# SPDX-License-Identifier: GPL-2.0-or-later
# Makefile for the video capture/playback device drivers.
# Copyright 2010 Steven King <sfking@fdwdc.com>
#Copyright(c) 2022-2027 Allwinnertech Co., Ltd
# Each configuration option enables a list of files.
obj-$(CONFIG_SENSOR_NVP6324) := nvp6324_mipi.o
nvp6324_mipi-objs := nvp6324_mipi_driver.o mipi_dev_nvp6324.o jaguar1_video.o jaguar1_coax_protocol.o jaguar1_motion.o jaguar1_video_eq.o jaguar1_mipi.o
#nvp6324_mipi-objs := nvp6324_mipi.o mipi_dev_nvp6324.o jaguar1_i2c.o jaguar1_video.o jaguar1_coax_protocol.o jaguar1_motion.o jaguar1_video_eq.o jaguar1_mipi.o

clean:
	@make -C $(KERNEL_PATH) M=$(PWD) clean


