# SPDX-License-Identifier: GPL-2.0-or-later
# Makefile for the video capture/playback device drivers.
# Copyright 2010 Steven King <sfking@fdwdc.com>
#Copyright(c) 2022-2027 Allwinnertech Co., Ltd
# Each configuration option enables a list of files.
#longan system
#CC := ../../../../../out/toolchain/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc
#tina system
#CC:= not support

#longan system
CC := ../../../../../out/toolchain/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc
ARCH_DIR := ../../../../../kernel/linux-5.15/arch/arm64/include/
CFLAGS := -I $(ARCH_DIR)
TARGET := csi_test_mplane

.PHONY: all clean

all: $(TARGET)

csi_test_mplane: csi_test_mplane.c
	$(CC) $(CFLAGS) -static  $^  -o  $@  -lpthread -lc -lgcc_eh

	#mv csi_test_mplane ~/to_server/camera_test/

clean:
	rm -rf $(TARGET)
