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

include ../common/android/platform_version.mk

# Basic support option tuning for Android
#
SUPPORT_ANDROID_PLATFORM := 1
SUPPORT_OPENGLES1_V1_ONLY := 1

# Meminfo IDs are required for buffer stamps
#
SUPPORT_MEMINFO_IDS := 1

# Enable services ion support by default
#
SUPPORT_ION ?= 1

# Need multi-process support in PDUMP
#
SUPPORT_PDUMP_MULTI_PROCESS := 1

# Always print debugging after 5 seconds of no activity
#
CLIENT_DRIVER_DEFAULT_WAIT_RETRIES := 50

# Android WSEGL is always the same
#
OPK_DEFAULT := libpvrANDROID_WSEGL.so

# srvkm is always built, but bufferclass_example is only built
# before EGL_image_external was generally available.
#
KERNEL_COMPONENTS := srvkm

# Kernel modules are always installed here under Android
#
PVRSRV_MODULE_BASEDIR := /system/modules/

# Use the new PVR_DPF implementation to allow lower message levels
# to be stripped from production drivers
#
PVRSRV_NEW_PVR_DPF := 1

# Production Android builds don't want PVRSRVGetDCSystemBuffer
#
SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER := 0

# Prefer to limit the 3D parameters heap to <16MB and move the
# extra 48MB to the general heap. This only affects cores with
# 28bit MMUs (520, 530, 531, 540).
#
SUPPORT_LARGE_GENERAL_HEAP := 1

# Enable a page pool for uncached memory allocations. This improves
# the performance of such allocations because the pages are temporarily
# not returned to Linux and therefore do not have to be re-invalidated
# (fewer cache invalidates are needed).
#
# Default the cache size to a maximum of 5400 pages (~21MB). If using
# newer Linux kernels (>=3.0) the cache may be reclaimed and become
# smaller than this maximum during runtime.
#
PVR_LINUX_MEM_AREA_POOL_MAX_PAGES ?= 5400

##############################################################################
# Framebuffer target extension is used to find configs compatible with
# the framebuffer (added in JB MR1).
#
EGL_EXTENSION_ANDROID_FRAMEBUFFER_TARGET := 1

##############################################################################
# Handle various platform includes for unittests
#
UNITTEST_INCLUDES := \
 eurasiacon/android \
 $(ANDROID_ROOT)/frameworks/base/native/include

ifeq ($(is_at_least_jellybean),1)
UNITTEST_INCLUDES += \
 $(ANDROID_ROOT)/frameworks/native/include \
 $(ANDROID_ROOT)/frameworks/native/opengl/include \
 $(ANDROID_ROOT)/libnativehelper/include/nativehelper
else
UNITTEST_INCLUDES += \
 $(ANDROID_ROOT)/frameworks/base/opengl/include \
 $(ANDROID_ROOT)/dalvik/libnativehelper/include/nativehelper
endif

# But it doesn't have OpenVG headers
#
UNITTEST_INCLUDES += eurasiacon/unittests/include

##############################################################################
# Future versions moved proprietary libraries to a vendor directory
#
SHLIB_DESTDIR := /system/vendor/lib
DEMO_DESTDIR := /system/vendor/bin

# EGL libraries go in a special place
#
EGL_DESTDIR := $(SHLIB_DESTDIR)/egl

##############################################################################
# We can support OpenCL in the build since Froyo (stlport was added in 2.2)
#
SYS_CXXFLAGS := -fuse-cxa-atexit $(SYS_CFLAGS)
SYS_INCLUDES += \
 -isystem $(ANDROID_ROOT)/bionic \
 -isystem $(ANDROID_ROOT)/external/stlport/stlport

##############################################################################
# Support the OES_EGL_image_external extensions in the client drivers.
#
GLES1_EXTENSION_EGL_IMAGE_EXTERNAL := 1
GLES2_EXTENSION_EGL_IMAGE_EXTERNAL := 1

##############################################################################
# ICS requires that at least one driver EGLConfig advertises the
# EGL_RECORDABLE_ANDROID attribute. The platform requires that surfaces
# rendered with this config can be consumed by an OMX video encoder.
#
EGL_EXTENSION_ANDROID_RECORDABLE := 1

##############################################################################
# ICS added the EGL_ANDROID_blob_cache extension. Enable support for this
# extension in EGL/GLESv2.
#
EGL_EXTENSION_ANDROID_BLOB_CACHE := 1

##############################################################################
# ICS and earlier should rate-limit composition by waiting for 3D renders
# to complete in the compositor's eglSwapBuffers().
#
ifeq ($(is_at_least_jellybean),0)
PVR_ANDROID_COMPOSITOR_WAIT_FOR_RENDER := 1
endif

##############################################################################
# JB added a new corkscrew API for userland backtracing.
#
ifeq ($(is_at_least_jellybean),1)
PVR_ANDROID_HAS_CORKSCREW_API := 1
endif

##############################################################################
# JB MR1 makes the framebuffer HAL obsolete.
#
# We also need to support IMPLEMENTATION_DEFINED so gralloc allocates
# framebuffers and GPU buffers in a 'preferred' format.
#
ifeq ($(is_at_least_jellybean_mr1),0)
SUPPORT_ANDROID_FRAMEBUFFER_HAL := 1
else
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED := 1
endif

##############################################################################
# JB MR1 introduces cross-process syncs associated with a fd.
# This requires a new enough kernel version to have the base/sync driver.
#
ifeq ($(is_at_least_jellybean_mr1),1)
EGL_EXTENSION_ANDROID_NATIVE_FENCE_SYNC := 1
PVR_ANDROID_NATIVE_WINDOW_HAS_SYNC := 1
endif

##############################################################################
# JB MR1 introduces new usage bits for the camera HAL and some new formats.
#
ifeq ($(is_at_least_jellybean_mr1),1)
PVR_ANDROID_HAS_GRALLOC_USAGE_HW_CAMERA := 1
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_RAW_SENSOR := 1
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_BLOB := 1
endif

##############################################################################
# JB MR2 adds a new graphics HAL (gralloc) API function, lock_ycbcr(), and
# a so-called "flexible" YUV format enum.
#
ifeq ($(is_at_least_jellybean_mr2),1)
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_YCbCr_420_888 := 1
PVR_ANDROID_GRALLOC_HAS_0_2_FEATURES := 1
endif

##############################################################################
# In JB MR2 we can use a native helper library for the unittest wrapper.
# In earlier versions, we must use a less ideal approach.
#
ifeq ($(is_at_least_jellybean_mr2),0)
PVR_ANDROID_SURFACE_FIELD_NAME := \"mNativeSurface\"
endif

##############################################################################
# JB MR2 introduces two new camera HAL formats (Y8, Y16)
#
ifeq ($(is_at_least_jellybean_mr2),1)
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_Y8 := 1
PVR_ANDROID_HAS_HAL_PIXEL_FORMAT_Y16 := 1
endif

##############################################################################
# KK's EGL wrapper remaps EGLConfigs in the BGRA and BGRX formats to RGBA and
# RGBX respectively, for CpuConsumer compatibility. It does this because the
# usage bits for the gralloc allocation are not available to EGL.
#
# In this newer platform version, gralloc has been redefined to allow the
# 'format' parameter to gralloc->alloc() to be ignored for non-USAGE_SW
# allocations, so long as the bits per channel and sRGB-ness are preserved.
#
ifeq ($(is_at_least_kitkat),1)
PVR_ANDROID_REMAP_HW_ONLY_PIXEL_FORMATS := 1
endif

##############################################################################
# Support newer HWC features in KK
#
ifeq ($(is_at_least_kitkat),1)
PVR_ANDROID_HWC_HAS_1_3_FEATURES := 1
endif

##############################################################################
# KK eliminated egl.cfg. Only create for older versions.
#
ifeq ($(is_at_least_kitkat),0)
PVR_ANDROID_HAS_EGL_CFG := 1
endif

##############################################################################
# KK has a bug in its browser that we need to work around.
#
ifeq ($(is_at_least_kitkat),1)
PVR_ANDROID_RELAX_GRALLOC_MODULE_MAP_CHECKS := 1
endif

##############################################################################
# KK's Camera HAL requires that ACTIVE_ARRAY_SIZE specify xmin/ymin first
#
ifeq ($(is_at_least_kitkat),1)
PVR_ANDROID_CAMERA_ACTIVE_ARRAY_SIZE_HAS_XMIN_YMIN := 1
endif

##############################################################################
# KitKat added a new memory tracking HAL. This enables gralloc support for
# the GRAPHICS/GL memtrack types.
#
ifeq ($(is_at_least_kitkat),1)
SUPPORT_ANDROID_MEMTRACK_HAL := 1
endif

# Placeholder for future version handling
#
ifeq ($(is_future_version),1)
-include ../common/android/future_version.mk
endif
