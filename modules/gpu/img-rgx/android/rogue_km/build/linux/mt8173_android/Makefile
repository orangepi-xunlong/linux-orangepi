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

# If a TARGET_PRODUCT is specified but not a TARGET_DEVICE, try to
# derive the TARGET_DEVICE from TARGET_PRODUCT.
#
ifeq ($(TARGET_DEVICE),)
override TARGET_DEVICE := \
 $(patsubst mini_%,%,$(patsubst full_%,%,$(TARGET_PRODUCT)))
endif

ifeq ($(TARGET_DEVICE),)
override TARGET_DEVICE := elm
endif

#E2/E3:
RGX_BVNC ?= 4.40.2.51

include ../config/preconfig.mk

PVR_SYSTEM := mt8173
PVR_LDM_PLATFORM_PRE_REGISTERED := 1

include ../common/android/arch.mk
include ../common/android/features.mk

# Kernel for Chromebook does not support llvm tools.
ifeq ($(KERNEL_CC),clang)
override KERNEL_LD :=
override KERNEL_NM :=
override KERNEL_OBJCOPY :=
endif

ifeq ($(TARGET_DEVICE),elm)
 SUPPORT_LINUX_DVFS ?= 1
 PVR_ANDROID_COMPOSERHAL ?= drm
 PVR_DRM_MODESET_MODULE_NAME := dumb
 PVR_DRM_MODESET_DRIVER_NAME := mediatek
 ION_DEFAULT_HEAP_NAME := \"ion_system_heap\"
 override CHROMIUMOS_KERNEL := 1
else
 # Heap names should be defined as per the labels used in the kernel.
 # Kernels >=4.12, support heap query so we use heap names
 # to get the correct heap id.
 # For kernels <4.12, we use heap_id_mask defined in this Makefile.
 # Heap names defined here are for reference only.
 # MTK's 'MULTIMEDIA' heap
 ION_DEFAULT_HEAP_NAME := \"ion_system_heap\"
 ION_DEFAULT_HEAP_ID_MASK := (1 << 10)
 ifeq ($(filter composerhal,$(MAKECMDGOALS)),)
  override EXCLUDED_APIS := $(EXCLUDED_APIS) composerhal
 else
  $(warning *** You explicitly enabled the IMG composerhal on the command line.)
  $(warning *** This will enable the adf_fbdev driver and support in gralloc.)
  $(warning *** This is **not** compatible with the MTK hwcomposer HAL.)
  DISPLAY_CONTROLLER ?= adf_fbdev
  KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER)
 endif
endif

# WA for power controller issue
FIX_DUSTS_POW_ON_INIT := 1

# Always print fatal and error logs, especially in kernel mode.
PVRSRV_NEED_PVR_DPF := 1

RGXFW_POWER_EVENT_DELAY_TICKS := 10

include ../config/core.mk
include ../common/android/extra_config.mk
include ../common/3rdparty.mk
