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

KERNELDIR = $(LICHEE_KDIR)
BUILD := release

# If a TARGET_PRODUCT is specified but not a TARGET_DEVICE, try to
# derive the TARGET_DEVICE from TARGET_PRODUCT.
#
ifeq ($(TARGET_DEVICE),)
override TARGET_DEVICE := \
 $(patsubst mini_%,%,$(patsubst full_%,%,$(TARGET_PRODUCT)))
ifneq ($(filter fiber_w02 v972v1 v972v2 novo9spark,$(TARGET_DEVICE)),)
override TARGET_DEVICE := sun6i
endif
endif

ifeq ($(TARGET_DEVICE),)
override TARGET_DEVICE := sun8i
endif

SGXCORE := 544
SGX_CORE_REV := 115
SUPPORT_SLC := 1
SGX_FEATURE_MP := 1
SGX_FEATURE_MP_CORE_COUNT := 1

PVR_SYSTEM := sunxi
HAL_VARIANT := sun6i
#DISPLAY_CONTROLLER := dc_sunxi

LDM_PLATFORM := 1

SUPPORT_LINUX_USING_WORKQUEUES := 1
SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 1

PVR_ANDROID_USE_WINDOW_TRANSFORM_HINT ?= 1
PVR_ANDROID_PLATFORM_HAS_LINUX_FBDEV := 1

PVRSRV_USSE_EDM_STATUS_DEBUG ?= 1
PVRSRV_DUMP_MK_TRACE ?= 1

SUPPORT_ANDROID_SUNXI ?= 1

ION_DEFAULT_HEAP_MASK := ION_HEAP_CARVEOUT_MASK

PVRSRV_NEED_PVR_DPF =: 1
PVRSRV_NEW_PVR_DPF =: 1

SUPPORT_PVRSRV_DEVICE_CLASS = 0

# We have more memory on the sun6i platform, so we can spare to make
# the pool larger, and have higher resolutions which benefit from it.
#
PVR_LINUX_MEM_AREA_POOL_MAX_PAGES ?= 10800

SGX_DYNAMIC_TIMING_INFO := 1

SUPPORT_PVRSRV_ANDROID_SYSTRACE := 0

#KERNEL_CROSS_COMPILE ?= arm-eabi-

include ../common/android/paths.mk
include ../common/android/armv7-a.mk
include ../common/android/features.mk

ifneq ($(strip $(KERNELDIR)),)
 include ../kernel_version.mk
 #KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER)
endif

ifeq ($(NO_HARDWARE),1)
ifeq ($(SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED),1)
$(info WARNING: SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED=1 is incompatible with NO_HARDWARE=1)
$(info WARNING: Setting SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED=0 and switching to dc_nohw)
override SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED := 0
override DISPLAY_CONTROLLER := dcnohw
KERNEL_COMPONENTS += dc_nohw
endif
endif

include ../config/core.mk
include ../common/android/extra_config.mk
include ../common/opencl.mk

# If set, services allows two flips to enter the processing queue,
# and does not add read dependencies to the set of buffers last
# flipped to.
# 
$(eval $(call TunableKernelConfigC,SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED,))

# On sun6i, A31-specific NV12 and NV21 formats are supported.
#
$(eval $(call TunableUserConfigMake,SUPPORT_ANDROID_SUNXI,))
