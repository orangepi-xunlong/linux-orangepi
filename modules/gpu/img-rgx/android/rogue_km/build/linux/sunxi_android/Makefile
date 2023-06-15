########################################################################### ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
#
# The contents of this file are subject to the MIT license as set out below.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
#
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
#
# This License is also included in this distribution in the file called
# "MIT-COPYING".
#
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

export MULTIARCH=64only
export EXCLUDED_APIS=opencl opengl vulkan cldnn imgdnn

PLATFORM_RELEASE := 11.0

# Google removed GCC from Android R prebuilts. Use clang for kbuild.
# Using set of LLVM tools by default. But they could be commented out
# depending on platforms.
#

$(info path:$(ANDROID_CLANG_PATH))

ifneq ($(ANDROID_CLANG_PATH),)
export USE_CLANG:=1
KERNEL_CC := $(ANDROID_CLANG_PATH)/clang
KERNEL_LD := $(ANDROID_CLANG_PATH)/ld.lld
KERNEL_NM := $(ANDROID_CLANG_PATH)/llvm-nm
KERNEL_OBJCOPY := $(ANDROID_CLANG_PATH)/llvm-objcopy

export HOST_CC:=$(ANDROID_CLANG_PATH)/clang
export HOST_CXX:=$(ANDROID_CLANG_PATH)/clang++
else
export USE_CLANG:=0

ifeq ($(LICHEE_ARCH),arm)
KERNEL_CC := $(LICHEE_TOOLCHAIN_PATH)/bin/arm-linux-gnueabi-gcc
KERNEL_LD := $(LICHEE_TOOLCHAIN_PATH)/bin/arm-linux-gnueabi-ld
KERNEL_NM := $(LICHEE_TOOLCHAIN_PATH)/bin/arm-linux-gnueabi-gcc-nm
KERNEL_OBJCOPY := $(LICHEE_TOOLCHAIN_PATH)/bin/arm-linux-gnueabi-objcopy

export HOST_CC:=$(LICHEE_TOOLCHAIN_PATH)/bin/arm-linux-gnueabi-gcc
export HOST_CXX:=$(LICHEE_TOOLCHAIN_PATH)/bin/arm-linux-gnueabi-g++
endif

ifeq ($(LICHEE_ARCH),arm64)
KERNEL_CC := $(LICHEE_TOOLCHAIN_PATH)/bin/aarch64-linux-gnu-gcc
KERNEL_LD := $(LICHEE_TOOLCHAIN_PATH)/bin/aarch64-linux-gnu-ld
KERNEL_NM := $(LICHEE_TOOLCHAIN_PATH)/bin/aarch64-linux-gnu-gcc-nm
KERNEL_OBJCOPY := $(LICHEE_TOOLCHAIN_PATH)/bin/aarch64-linux-gnu-objcopy

export HOST_CC:=$(LICHEE_TOOLCHAIN_PATH)/aarch64-linux-gnu-gcc
export HOST_CXX:=$(LICHEE_TOOLCHAIN_PATH)/bin/aarch64-linux-gnu-g++
endif

endif


ifeq ($(ARCH_SUPPORTS_INT128), y)
DEVICE_MEMSETCPY_ALIGN_IN_BYTES := 16
else
DEVICE_MEMSETCPY_ALIGN_IN_BYTES := 8
endif

# If a TARGET_PRODUCT is specified but not a TARGET_DEVICE, try to
# derive the TARGET_DEVICE from TARGET_PRODUCT.
#
ifeq ($(TARGET_DEVICE),)
override TARGET_DEVICE := \
 $(patsubst mini_%,%,$(patsubst full_%,%,$(TARGET_PRODUCT)))
ifneq ($(filter kylin_perf kylin_p1,$(TARGET_DEVICE)),)
override TARGET_DEVICE := sun9i
endif
endif

ifeq ($(TARGET_DEVICE),)
override TARGET_DEVICE := sun9i
endif

PVR_SYSTEM := rgx_sunxi
RGX_BVNC = 1.75.2.30
include ../config/preconfig.mk



ifeq ($(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND),1)
PVR_DVFS := 1
endif

include ../common/android/arch.mk
include ../common/android/features.mk

PVR_ANDROID_COMPOSERHAL := null

PDUMP_STREAMBUF_SIZE_MB := 10

RGX_TIMECORR_CLOCK := sched

# Don't use dma_alloc_coherent because it falls back to allocate
# highmem pages and maps them to vmalloc space which exhausts quickly
PVR_LINUX_PHYSMEM_SUPPRESS_DMA_AC := 1

include ../config/core.mk
#include ../common/android/extra_config.mk

