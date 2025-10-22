# SPDX-License-Identifier: GPL-2.0-or-later */
# Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved.
codec_dir = ${src}/codec
codec_lib_dir = codec/lib
codec_mem_dir = codec/memory
codec_abs_dir = $(PWD)/${src}/codec
EXTRA_CFLAGS += -I$(codec_abs_dir)/lib/include
# EXTRA_CFLAGS += -I$(codec_abs_dir)/memory/kernelMemory -I$(codec_abs_dir)/memory/ionMemory \
	#   -I$(codec_abs_dir)/memory/kernelMemory/ion_mem
# #include ${codec_dir}/memory/Makefile
# $(info EXTRA_CFLAGS: $(EXTRA_CFLAGS) codec_dir $(codec_dir))
# codec_obj := $(codec_mem_dir)/ionMemory/ionAlloc.o \
	#       $(codec_mem_dir)/memoryAdapter.o \
	#       $(codec_mem_dir)/kernelMemory/mem_interface.o \
	#       $(codec_mem_dir)/kernelMemory/ion_mem/ion_mem.o
codec_obj += $(codec_lib_dir)/libbase $(codec_lib_dir)/libve $(codec_lib_dir)/libvencoder $(codec_lib_dir)/libmem

