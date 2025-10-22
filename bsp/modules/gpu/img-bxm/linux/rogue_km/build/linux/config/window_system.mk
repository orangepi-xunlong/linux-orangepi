########################################################################### ###
#@File
#@Title         Select a window system
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

# Set the default window system.
# If you want to override the default, create a default_window_system.mk file
# that sets WINDOW_SYSTEM appropriately.  (There is a suitable example in
# ../config/default_window_system_xorg.mk)
-include ../config/default_window_system.mk

ifeq ($(SUPPORT_NEUTRINO_PLATFORM),)
WINDOW_SYSTEM ?= ews
_supported_window_systems := ews lws-generic nulldrmws nullws surfaceless tizen wayland xorg
else
WINDOW_SYSTEM ?= nullws
_supported_window_systems := nullws screen
endif

_window_system_mk_path := ../common/window_systems
_window_systems := \
 $(sort $(patsubst $(_window_system_mk_path)/%.mk,%,$(wildcard $(_window_system_mk_path)/*.mk)))
_window_systems := $(filter $(_supported_window_systems),$(_window_systems))

_unrecognised_window_system := $(strip $(filter-out $(_window_systems),$(WINDOW_SYSTEM)))
ifneq ($(_unrecognised_window_system),)
$(warning *** Unrecognised WINDOW_SYSTEM: $(_unrecognised_window_system))
$(warning *** WINDOW_SYSTEM was set via: $(origin WINDOW_SYSTEM))
$(error Supported Window Systems are: $(_window_systems))
endif

ifeq ($(MESA_EGL),1)
 override SUPPORT_EGL_KHR_NATIVE_PIXMAP := 0
 ifeq ($(WINDOW_SYSTEM),nulldrmws)
  ifeq ($(SUPPORT_FALLBACK_FENCE_SYNC),1)
   # The Mesa EGL version of nulldrmws requires a display driver that
   # supports atomic modesetting, and so it also requires native fence sync.
   $(warning Fallback fence sync selected. Using IMG EGL for nulldrmws.)
   override undefine MESA_EGL
  endif
 endif
endif

# Use this to mark config options that are user-tunable for certain window
# systems but not others. Warn if an attempt is made to change it.
#
# $(1): config option
# $(2): window system(s) for which the config option is user-tunable
#
define WindowSystemTunableOption
$(if $(filter $(2),$(WINDOW_SYSTEM)),,\
	$(if $(filter command line environment,$(origin $(1))),\
		$(error Changing '$(1)' for '$(WINDOW_SYSTEM)' is not supported)))
endef

$(call WindowSystemTunableOption,EGL_EXTENSION_ANDROID_NATIVE_FENCE_SYNC,)
$(call WindowSystemTunableOption,GBM_BACKEND,$(if $(MESA_EGL),,nulldrmws))
$(call WindowSystemTunableOption,MESA_EGL,nulldrmws)
$(call WindowSystemTunableOption,MESA_WSI,nulldrmws)
$(call WindowSystemTunableOption,OPK_DEFAULT,)
$(call WindowSystemTunableOption,OPK_FALLBACK,)
$(call WindowSystemTunableOption,SUPPORT_ACTIVE_FLUSH,\
	ews lws-generic nullws nulldrmws wayland screen)
$(call WindowSystemTunableOption,SUPPORT_DISPLAY_CLASS,ews nullws screen)
$(call WindowSystemTunableOption,SUPPORT_FALLBACK_FENCE_SYNC,\
		lws-generic nulldrmws surfaceless wayland xorg)
$(call WindowSystemTunableOption,SUPPORT_INSECURE_EXPORT,ews)
$(call WindowSystemTunableOption,SUPPORT_KMS,ews)
$(call WindowSystemTunableOption,SUPPORT_NATIVE_FENCE_SYNC,\
	$(if $(SUPPORT_KMS),,ews) nullws)
$(call WindowSystemTunableOption,SUPPORT_SECURE_EXPORT,nullws ews)
$(call WindowSystemTunableOption,SUPPORT_VK_PLATFORMS,lws-generic)
$(call WindowSystemTunableOption,PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE,\
	$(if $(SUPPORT_KMS),,ews) nullws)
$(call WindowSystemTunableOption,SUPPORT_XWAYLAND,wayland)

ifneq ($(MESA_EGL),1)
 # Tests of MESA_EGL are against the value 1 and the empty string, so values
 # other than 1, such as 0, should be mapped to the empty string.
 override undefine MESA_EGL
else
 # MESA_WSI defaults to the same value as MESA_EGL.
 MESA_WSI ?= 1
endif

ifneq ($(MESA_WSI),1)
 # Tests of MESA_WSI are against the value 1 and the empty string, so values
 # other than 1, such as 0, should be mapped to the empty string.
 override undefine MESA_WSI
endif

ifneq ($(MESA_ZINK),1)
 override undefine MESA_ZINK
endif

MESA_WSI_NO_VK_EXT_HEADLESS_SURFACE ?= $(MESA_ZINK)
ifneq ($(MESA_WSI_NO_VK_EXT_HEADLESS_SURFACE),1)
 override undefine MESA_WSI_NO_VK_EXT_HEADLESS_SURFACE
endif

ifneq ($(filter lws-generic,$(WINDOW_SYSTEM)),)
 ifeq ($(MESA_WSI_NO_VK_EXT_HEADLESS_SURFACE),1)
  ifeq ($(SUPPORT_VK_PLATFORMS),)
   # There is no point building Mesa WSI if no platforms have been selected.
   override undefine MESA_WSI
  else
   override MESA_WSI := 1
  endif
 else
   override MESA_WSI := 1
 endif
endif

ifeq ($(WINDOW_SYSTEM),xorg)
 override MESA_EGL := 1
 override MESA_WSI := 1
 SUPPORT_VK_PLATFORMS := x11
 SUPPORT_DISPLAY_CLASS := 0
 SUPPORT_NATIVE_FENCE_SYNC := 1
 SUPPORT_KMS := 1
 override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
 SUPPORT_ACTIVE_FLUSH := 1
else ifeq ($(WINDOW_SYSTEM),wayland)
 override MESA_EGL := 1
 override MESA_WSI := 1
 SUPPORT_VK_PLATFORMS := wayland
 SUPPORT_DISPLAY_CLASS := 0
 SUPPORT_NATIVE_FENCE_SYNC := 1
 SUPPORT_KMS := 1
 override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
 ifeq ($(SUPPORT_XWAYLAND),1)
  SUPPORT_VK_PLATFORMS += x11
  SUPPORT_ACTIVE_FLUSH := 1
 endif
else ifeq ($(WINDOW_SYSTEM),tizen)
 override MESA_EGL := 1
 override undefine MESA_WSI
 SUPPORT_DISPLAY_CLASS := 0
 SUPPORT_NATIVE_FENCE_SYNC := 1
 SUPPORT_KMS := 1
 override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
else ifeq ($(WINDOW_SYSTEM),surfaceless)
 override MESA_EGL := 1
 ifeq ($(MESA_WSI_NO_VK_EXT_HEADLESS_SURFACE),1)
  override undefine MESA_WSI
 else
  override MESA_WSI := 1
 endif
 SUPPORT_ACTIVE_FLUSH := 1
 SUPPORT_DISPLAY_CLASS := 0
 SUPPORT_NATIVE_FENCE_SYNC := 1
 SUPPORT_KMS := 1
 override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
else ifeq ($(WINDOW_SYSTEM),lws-generic)
 override MESA_EGL := 1
 SUPPORT_DISPLAY_CLASS := 0
 SUPPORT_NATIVE_FENCE_SYNC := 1
 SUPPORT_KMS := 1
 override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
else ifeq ($(WINDOW_SYSTEM),ews) # Linux builds only
 SUPPORT_VK_PLATFORMS := null
 SUPPORT_SECURE_EXPORT ?= 1
 SUPPORT_DISPLAY_CLASS ?= 1
 SUPPORT_EGL_KHR_NATIVE_PIXMAP := 1
 OPK_DEFAULT := libpvrEWS_WSEGL.so
 ifeq ($(SUPPORT_DISPLAY_CLASS),1)
  PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE ?= 1
  OPK_FALLBACK := libpvrNULL_WSEGL.so
  PVR_HANDLE_BACKEND ?= generic
  ifeq ($(SUPPORT_NATIVE_FENCE_SYNC),) # Set default if no override
   SUPPORT_FALLBACK_FENCE_SYNC := 1
  endif
 else ifeq ($(SUPPORT_KMS),1)
  override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
  OPK_FALLBACK := libpvrNULLDRM_WSEGL.so
  SUPPORT_NATIVE_FENCE_SYNC := 1
 else
  $(error either SUPPORT_DISPLAY_CLASS or SUPPORT_KMS must be enabled)
 endif
 ifeq ($(SUPPORT_NATIVE_FENCE_SYNC)$(SUPPORT_FALLBACK_FENCE_SYNC),1)
  EGL_EXTENSION_ANDROID_NATIVE_FENCE_SYNC := 1
 endif
else ifeq ($(WINDOW_SYSTEM),nullws) # Linux and Neutrino builds
 PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE ?= 1
 OPK_DEFAULT  := libpvrNULL_WSEGL.so
 OPK_FALLBACK := libpvrNULL_WSEGL.so
 PVR_HANDLE_BACKEND ?= generic
 SUPPORT_VK_PLATFORMS := null
 SUPPORT_DISPLAY_CLASS ?= 1
 ifeq ($(SUPPORT_NATIVE_FENCE_SYNC),) # Set default if no override
  SUPPORT_FALLBACK_FENCE_SYNC := 1
 endif
else ifeq ($(WINDOW_SYSTEM),nulldrmws)
 ifeq ($(MESA_EGL),)
  OPK_DEFAULT  := libpvrNULLDRM_WSEGL.so
  OPK_FALLBACK := libpvrNULLDRM_WSEGL.so
  _supported_gbm_backends := dbm
  GBM_BACKEND ?= dbm
 endif
 SUPPORT_VK_PLATFORMS := null
 SUPPORT_DISPLAY_CLASS := 0
 SUPPORT_NATIVE_FENCE_SYNC := 1
 SUPPORT_KMS := 1
 override PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE := 0
else ifeq ($(WINDOW_SYSTEM),screen) # Neutrino builds
 OPK_DEFAULT  := libpvrSCREEN_WSEGL.so
 OPK_FALLBACK := libpvrSCREEN_WSEGL.so
 PVR_HANDLE_BACKEND := generic
 SUPPORT_VK_PLATFORMS := null
 SUPPORT_DISPLAY_CLASS ?= 1
 SUPPORT_FALLBACK_FENCE_SYNC := 1
 PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE ?= 1
endif

MESA_WSI_NO_VK_KHR_PRESENT_ID ?= $(or $(MESA_ZINK),$(if $(filter wayland,$(SUPPORT_VK_PLATFORMS)),1))
ifneq ($(MESA_WSI_NO_VK_KHR_PRESENT_ID),1)
 override undefine MESA_WSI_NO_VK_KHR_PRESENT_ID
endif

MESA_WSI_NO_VK_KHR_PRESENT_WAIT ?= $(MESA_WSI_NO_VK_KHR_PRESENT_ID)
ifneq ($(MESA_WSI_NO_VK_KHR_PRESENT_WAIT),1)
 override undefine MESA_WSI_NO_VK_KHR_PRESENT_WAIT
endif

ifeq ($(SUPPORT_FALLBACK_FENCE_SYNC),1)
 ifneq ($(filter lws-generic nulldrmws surfaceless wayland xorg,\
		 $(WINDOW_SYSTEM)),)
  $(warning Fallback fence sync selected for window system $(WINDOW_SYSTEM).)
  undefine SUPPORT_NATIVE_FENCE_SYNC
 endif
endif

ifeq ($(MESA_EGL),1)
 EGL_BASENAME_SUFFIX := _PVR_MESA
 SUPPORT_OPENGLES1_V1_ONLY := 1
 ifeq ($(SUPPORT_NATIVE_FENCE_SYNC),1)
  EGL_EXTENSION_ANDROID_NATIVE_FENCE_SYNC := 1
 endif
endif

ifeq ($(call is-not-target-os,neutrino),true)
 SUPPORT_VKEXT_IMAGE_FORMAT_MOD := 1
endif
