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

# Google removed GCC from Android R prebuilts. Use clang for kbuild.
# Using set of LLVM tools by default. But they could be commented out
# depending on platforms.
#

ifneq ($(ANDROID_CLANG_PATH),)
export USE_CLANG:=1
KERNEL_CC := $(ANDROID_CLANG_PATH)/clang
KERNEL_LD := $(ANDROID_CLANG_PATH)/ld.lld
KERNEL_NM := $(ANDROID_CLANG_PATH)/llvm-nm
KERNEL_OBJCOPY := $(ANDROID_CLANG_PATH)/llvm-objcopy

export HOST_CC:=/usr/bin/clang
export HOST_CXX:=/usr/bin/clang++
else
export USE_CLANG:=0

KERNEL_CC := $(LICHEE_TOOLCHAIN_PATH)/bin/$(LICHEE_CROSS_COMPILER)-gcc
KERNEL_LD := $(LICHEE_TOOLCHAIN_PATH)/bin/$(LICHEE_CROSS_COMPILER)-ld
KERNEL_NM := $(LICHEE_TOOLCHAIN_PATH)/bin/$(LICHEE_CROSS_COMPILER)-gcc-nm
KERNEL_OBJCOPY := $(LICHEE_TOOLCHAIN_PATH)/bin/$(LICHEE_CROSS_COMPILER)-objcopy

export HOST_CC:=/usr/bin/gcc
export HOST_CXX:=/usr/bin/g++

endif

WINDOW_SYSTEM := nulldrmws
RGX_BVNC := 36.56.104.183
SUPPORT_DMA_FENCE := 1

BUILD := $(GPU_BUILD_TYPE)

include ../config/preconfig.mk
include ../config/window_system.mk


ifeq ($(ARCH_SUPPORTS_INT128), y)
DEVICE_MEMSETCPY_ALIGN_IN_BYTES := 16
else
DEVICE_MEMSETCPY_ALIGN_IN_BYTES := 8
endif

PVR_SYSTEM := rgx_sunxi

KERNEL_COMPONENTS := srvkm

#DISPLAY_CONTROLLER := dc_sunxi
#KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER)

NO_HARDWARE := 0
HWR_DEFAULT_ENABLED := 1


RGX_FW_SIGNED := 0
CACHEFLUSH_NO_KMRBF_USING_UMVA := 1

PVR_LDM_PLATFORM_PRE_REGISTERED := 1

PVRSRV_NEED_PVR_DPF := 1

ifeq ($(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND),1)
SUPPORT_LINUX_DVFS ?= 1
endif

# Should be last
include ../config/core.mk
include ../common/lws.mk
include ../common/3rdparty.mk

$(eval $(call TunableUserConfigC,SGLTRACE,))
