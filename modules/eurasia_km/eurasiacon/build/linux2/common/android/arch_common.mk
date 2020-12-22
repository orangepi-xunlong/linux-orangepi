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

ifeq ($(USE_CLANG),1)
export CC  := $(OUT_DIR)/host/$(HOST_OS)-$(HOST_ARCH)/bin/clang
export CXX := $(OUT_DIR)/host/$(HOST_OS)-$(HOST_ARCH)/bin/clang++
endif

# FIXME: We need to run this early because config/core.mk hasn't been
# included yet. Use the same variable names as in that makefile.
#
_CC		:= $(if $(filter default,$(origin CC)),gcc,$(CC))
_CLANG	:= $(shell ../tools/cc-check.sh --clang --cc $(_CC))

SYS_CFLAGS := \
 -fno-short-enums \
 -funwind-tables \
 -D__linux__
SYS_INCLUDES := \
 -isystem $(ANDROID_ROOT)/bionic/libc/arch-$(ANDROID_ARCH)/include \
 -isystem $(ANDROID_ROOT)/bionic/libc/include \
 -isystem $(ANDROID_ROOT)/bionic/libc/kernel/common \
 -isystem $(ANDROID_ROOT)/bionic/libc/kernel/arch-$(ANDROID_ARCH) \
 -isystem $(ANDROID_ROOT)/bionic/libm/include \
 -isystem $(ANDROID_ROOT)/bionic/libm/include/$(ANDROID_ARCH) \
 -isystem $(ANDROID_ROOT)/bionic/libthread_db/include \
 -isystem $(ANDROID_ROOT)/frameworks/base/include \
 -isystem $(ANDROID_ROOT)/system/core/include \
 -isystem $(ANDROID_ROOT)/hardware/libhardware/include \
 -isystem $(ANDROID_ROOT)/external/openssl/include \
 -isystem $(ANDROID_ROOT)/system/media/camera/include \
 -isystem $(ANDROID_ROOT)/hardware/libhardware_legacy/include

# This is comparing PVR_BUILD_DIR to see if it is omap and adding 
# includes required for it's HWC
ifeq ($(notdir $(abspath .)),omap_android)
SYS_INCLUDES += \
 -isystem $(ANDROID_ROOT)/hardware/ti/omap4xxx/kernel-headers
endif

ifeq ($(_CLANG),true)
SYS_INCLUDES := \
 -nostdinc $(SYS_INCLUDES) \
 -isystem $(ANDROID_ROOT)/external/clang/lib/include
endif

SYS_EXE_LDFLAGS := \
 -Bdynamic -nostdlib -Wl,-dynamic-linker,/system/bin/linker \
 -lc -ldl -lcutils

SYS_LIB_LDFLAGS := $(SYS_EXE_LDFLAGS)

SYS_EXE_LDFLAGS_CXX := -lstdc++

SYS_LIB_LDFLAGS_CXX := $(SYS_EXE_LDFLAGS_CXX)
