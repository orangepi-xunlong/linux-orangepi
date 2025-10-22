##############################################################################
#
#    The MIT License (MIT)
#
#    Copyright (c) 2014 - 2024 Vivante Corporation
#
#    Permission is hereby granted, free of charge, to any person obtaining a
#    copy of this software and associated documentation files (the "Software"),
#    to deal in the Software without restriction, including without limitation
#    the rights to use, copy, modify, merge, publish, distribute, sublicense,
#    and/or sell copies of the Software, and to permit persons to whom the
#    Software is furnished to do so, subject to the following conditions:
#
#    The above copyright notice and this permission notice shall be included in
#    all copies or substantial portions of the Software.
#
#    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#    DEALINGS IN THE SOFTWARE.
#
##############################################################################
#
#    The GPL License (GPL)
#
#    Copyright (C) 2014 - 2024 Vivante Corporation
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version 2
#    of the License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software Foundation,
#    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
##############################################################################
#
#    Note: This software is released under dual MIT and GPL licenses. A
#    recipient may use this file under the terms of either the MIT license or
#    GPL License. If you wish to use only one license not the other, you can
#    indicate your decision by deleting one of the above license notices in your
#    version of this file.
#
##############################################################################

LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../Android.mk.def

ifeq ($(VIVANTE_ENABLE_VSIMULATOR),0)


#
# galcore.ko
#
include $(CLEAR_VARS)

GALCORE_OUT := $(TARGET_OUT_INTERMEDIATES)/GALCORE_OBJ
GALCORE := \
	$(GALCORE_OUT)/galcore.ko

KERNELENVSH := $(GALCORE_OUT)/kernelenv.sh
$(KERNELENVSH):
	rm -rf $(KERNELENVSH)
	echo 'export KERNEL_DIR=$(KERNEL_DIR)' > $(KERNELENVSH)
	echo 'export CROSS_COMPILE=$(CROSS_COMPILE)' >> $(KERNELENVSH)
	echo 'export ARCH_TYPE=$(ARCH_TYPE)' >> $(KERNELENVSH)

GALCORE_LOCAL_PATH := $(LOCAL_PATH)

$(GALCORE): $(KERNELENVSH)
	@cd $(AQROOT)
	source $(KERNELENVSH); $(MAKE) -f Kbuild -C $(AQROOT) \
		AQROOT=$(abspath $(AQROOT)) \
		AQARCH=$(abspath $(AQARCH)) \
		AQVGARCH=$(abspath $(AQVGARCH)) \
		ARCH_TYPE=$(ARCH_TYPE) \
		DEBUG=$(DEBUG) \
		VIVANTE_ENABLE_DRM=$(DRM_GRALLOC) \
		VIVANTE_ENABLE_2D=$(VIVANTE_ENABLE_2D) \
		VIVANTE_ENABLE_3D=$(VIVANTE_ENABLE_3D) \
		VIVANTE_ENABLE_VG=$(VIVANTE_ENABLE_VG) \
        USE_LINUX_PCIE=$(USE_LINUX_PCIE) \
        ENABLE_VM_PASSTHROUGH=$(ENABLE_VM_PASSTHROUGH); \
		cp $(GALCORE_LOCAL_PATH)/../../galcore.ko $(GALCORE)

LOCAL_PREBUILT_MODULE_FILE := \
	$(GALCORE)

LOCAL_GENERATED_SOURCES := \
	$(AQREG) \
	$(VG_AQREG)

LOCAL_GENERATED_SOURCES += \
	$(GALCORE)

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 21),1)
  LOCAL_MODULE_RELATIVE_PATH := modules
else
  LOCAL_MODULE_PATH          := $(TARGET_OUT_SHARED_LIBRARIES)/modules
endif

LOCAL_MODULE        := galcore
LOCAL_MODULE_SUFFIX := .ko
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := SHARED_LIBRARIES
LOCAL_STRIP_MODULE  := false
include $(BUILD_PREBUILT)

else

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
          gc_hal_kernel_command.c \
          gc_hal_kernel_db.c \
          gc_hal_kernel_event.c \
          gc_hal_kernel_heap.c \
          gc_hal_kernel.c \
          gc_hal_kernel_mmu.c \
          gc_hal_kernel_video_memory.c

LOCAL_SRC_FILES += \
          gc_hal_kernel_security.c \
          gc_hal_kernel_security_v1.c \

LOCAL_CFLAGS := \
    $(CFLAGS) \
    -DNO_VDT_PROXY

LOCAL_C_INCLUDES := \
	$(AQROOT)/vsimulator/os/linux/inc \
	$(AQROOT)/vsimulator/os/linux/emulator \
    $(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/kernel/arch \
	$(AQROOT)/hal/kernel \

LOCAL_MODULE         := libhalkernel

LOCAL_MODULE_TAGS    := optional

LOCAL_PRELINK_MODULE := false

ifeq ($(PLATFORM_VENDOR),1)
LOCAL_VENDOR_MODULE  := true
endif
include $(BUILD_STATIC_LIBRARY)

endif

