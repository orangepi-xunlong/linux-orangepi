##############################################################################
#
#    Copyright (c) 2005 - 2024 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################

LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../../../Android.mk.def

include $(CLEAR_VARS)

platform_option := \
    platform/$(soc_vendor)/gc_hal_user_platform_$(soc_board).c

ifeq ($(wildcard $(LOCAL_PATH)/$(platform_option)),)
  platform_option := platform/default/gc_hal_user_platform_default.c
endif

LOCAL_SRC_FILES := \
    gc_hal_user_debug.c \
    gc_hal_user_os.c \
    gc_hal_user_math.c \
    $(platform_option)

LOCAL_CFLAGS := \
    $(CFLAGS) \
	-Werror

LOCAL_C_INCLUDES := \
    $(AQROOT)/hal/inc \
    $(AQROOT)/hal/user \
    $(AQROOT)/hal/os/linux/user

ifeq ($(VIVANTE_ENABLE_3D),1)
LOCAL_C_INCLUDES += \
    $(AQROOT)/compiler/libVSC/include
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 20),1)
LOCAL_C_INCLUDES += \
	system/core/libsync/include
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 28),1)
LOCAL_C_INCLUDES += \
   system/core/include
endif

ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 31),1)
LOCAL_C_INCLUDES += \
    system/logging/liblog/include
endif

LOCAL_MODULE         := libhalosuser
LOCAL_MODULE_TAGS    := optional
ifeq ($(PLATFORM_VENDOR),1)
LOCAL_VENDOR_MODULE  := true
endif
include $(BUILD_STATIC_LIBRARY)

