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

OPTIM := -O2

ANDROID_ARCH := arm
include ../common/android/arch_common.mk

SYS_CFLAGS += -march=armv7-a

ifneq ($(BUILD),debug)
SYS_CFLAGS += -mthumb
endif

SYS_EXE_CRTBEGIN := $(TOOLCHAIN)/lib/crtbegin_dynamic.o
SYS_EXE_CRTEND := $(TOOLCHAIN)/lib/crtend_android.o

SYS_LIB_CRTBEGIN := $(TOOLCHAIN)/lib/crtbegin_so.o
SYS_LIB_CRTEND := $(TOOLCHAIN)/lib/crtend_so.o

# Handle the removal of the armelf.x and armelf.xsc linker scripts.
ifeq ($(strip $(wildcard $(ANDROID_ROOT)/build/core/armelf.x)),)
# The linker scripts have been removed. We need to use these options
# instead.
SYS_EXE_LDFLAGS += -Wl,-z,relro -Wl,-z,now
SYS_LIB_LDFLAGS += -Wl,-z,relro -Wl,-z,now
else
# The linker scripts are still present in the Android tree, so we need to
# use them.
SYS_EXE_LDFLAGS += -Wl,-T$(ANDROID_ROOT)/build/core/armelf.x
SYS_LIB_LDFLAGS += -Wl,-T$(ANDROID_ROOT)/build/core/armelf.xsc
endif

JNI_CPU_ABI := armeabi

# Android builds are usually GPL
#
LDM_PLATFORM ?= 1
