########################################################################### ###
#@Title         Root build configuration.
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

# Configuration wrapper for new build system. This file deals with
# configuration of the build. Add to this file anything that deals
# with switching driver options on/off and altering the defines or
# objects the build uses.
#
# At the end of this file is an exhaustive list of all variables
# that are passed between the platform/config stage and the generic
# build. PLEASE refrain from adding more variables than necessary
# to this stage -- almost all options can go through config.h.
#

################################# MACROS ####################################

# Write out a kernel GNU make option.
#
define KernelConfigMake
$$(shell echo "override $(1) := $(2)" >>$(CONFIG_KERNEL_MK).new)
$(if $(filter config,$(D)),$(info KernelConfigMake $(1) := $(2) 	# $(if $($(1)),$(origin $(1)),default)))
endef

# Write out a GNU make option for both user & kernel
#
define BothConfigMake
$$(eval $$(call KernelConfigMake,$(1),$(2)))
endef

# Conditionally write out a kernel GNU make option
#
define TunableKernelConfigMake
ifneq ($$($(1)),)
ifneq ($$($(1)),0)
$$(eval $$(call KernelConfigMake,$(1),$$($(1))))
endif
else
ifneq ($(2),)
$$(eval $$(call KernelConfigMake,$(1),$(2)))
endif
endif
endef

# Conditionally write out a GNU make option for both user & kernel
#
define TunableBothConfigMake
$$(eval $$(call TunableKernelConfigMake,$(1),$(2)))
endef

# Write out a kernel-only option
#
define KernelConfigC
$$(shell echo "#define $(1) $(2)" >>$(CONFIG_KERNEL_H).new)
$(if $(filter config,$(D)),$(info KernelConfigC    #define $(1) $(2) 	/* $(if $($(1)),$(origin $(1)),default) */),)
endef

# Write out an option for both user & kernel
#
define BothConfigC
$$(eval $$(call KernelConfigC,$(1),$(2)))
endef

# Conditionally write out a kernel-only option
#
define TunableKernelConfigC
ifneq ($$($(1)),)
ifneq ($$($(1)),0)
ifeq ($$($(1)),1)
$$(eval $$(call KernelConfigC,$(1),))
else
$$(eval $$(call KernelConfigC,$(1),$$($(1))))
endif
endif
else
ifneq ($(2),)
ifeq ($(2),1)
$$(eval $$(call KernelConfigC,$(1),))
else
$$(eval $$(call KernelConfigC,$(1),$(2)))
endif
endif
endif
endef

# Conditionally write out an option for both user & kernel
#
define TunableBothConfigC
$$(eval $$(call TunableKernelConfigC,$(1),$(2)))
endef

############################### END MACROS ##################################

# Check we have a new enough version of GNU make.
#
need := 3.81
ifeq ($(filter $(need),$(firstword $(sort $(MAKE_VERSION) $(need)))),)
$(error A version of GNU make >= $(need) is required - this is version $(MAKE_VERSION))
endif

# Try to guess EURASIAROOT if it wasn't set. Check this location.
#
_GUESSED_EURASIAROOT := $(abspath ../../../..)
ifneq ($(strip $(EURASIAROOT)),)
# We don't want to warn about EURASIAROOT if it's empty: this might mean that
# it's not set at all anywhere, but it could also mean that it's set like
# "export EURASIAROOT=" or "make EURASIAROOT= sometarget". If it is set but
# empty, we'll act as if it's unset and not warn.
ifneq ($(strip $(EURASIAROOT)),$(_GUESSED_EURASIAROOT))
nothing :=
space := $(nothing) $(nothing)
$(warning EURASIAROOT is set (via: $(origin EURASIAROOT)), but its value does not)
$(warning match the root of this source tree, so it is being ignored)
$(warning EURASIAROOT is set to: $(EURASIAROOT))
$(warning $(space)The detected root is: $(_GUESSED_EURASIAROOT))
$(warning To suppress this message, unset EURASIAROOT or set it empty)
endif
# else, EURASIAROOT matched the actual root of the source tree: don't warn
endif
override EURASIAROOT := $(_GUESSED_EURASIAROOT)
TOP := $(EURASIAROOT)

ifneq ($(words $(TOP)),1)
$(warning This source tree is located in a path which contains whitespace,)
$(warning which is not supported.)
$(warning $(space)The root is: $(TOP))
$(error Whitespace found in $$(TOP))
endif

$(call directory-must-exist,$(TOP))

include ../defs.mk

# Infer PVR_BUILD_DIR from the directory configuration is launched from.
# Check anyway that such a directory exists.
#
PVR_BUILD_DIR := $(notdir $(abspath .))
$(call directory-must-exist,$(TOP)/eurasiacon/build/linux2/$(PVR_BUILD_DIR))

# Output directory for configuration, object code,
# final programs/libraries, and install/rc scripts.
#
BUILD	?= release
OUT		?= $(TOP)/eurasiacon/binary2_$(PVR_BUILD_DIR)_$(BUILD)
override OUT := $(if $(filter /%,$(OUT)),$(OUT),$(TOP)/$(OUT))

CONFIG_MK			:= $(OUT)/config.mk
CONFIG_H			:= $(OUT)/config.h
CONFIG_KERNEL_MK	:= $(OUT)/config_kernel.mk
CONFIG_KERNEL_H		:= $(OUT)/config_kernel.h

# Convert commas to spaces in $(D). This is so you can say "make
# D=config-changes,freeze-config" and have $(filter config-changes,$(D))
# still work.
comma := ,
empty :=
space := $(empty) $(empty)
override D := $(subst $(comma),$(space),$(D))

# Create the OUT directory and delete any previous intermediary files
#
$(shell mkdir -p $(OUT))
$(shell \
	for file in $(CONFIG_MK).new $(CONFIG_H).new \
				$(CONFIG_KERNEL_MK).new $(CONFIG_KERNEL_H).new; do \
		rm -f $$file; \
	done)

# Some targets don't need information about any modules. If we only specify
# these targets on the make command line, set INTERNAL_CLOBBER_ONLY to
# indicate that toplevel.mk shouldn't read any makefiles
CLOBBER_ONLY_TARGETS := clean clobber help install
INTERNAL_CLOBBER_ONLY :=
ifneq ($(strip $(MAKECMDGOALS)),)
INTERNAL_CLOBBER_ONLY := \
$(if \
 $(strip $(foreach _cmdgoal,$(MAKECMDGOALS),\
          $(if $(filter $(_cmdgoal),$(CLOBBER_ONLY_TARGETS)),,x))),,true)
endif

# For a clobber-only build, we shouldn't regenerate any config files, or
# require things like SGXCORE to be set
ifneq ($(INTERNAL_CLOBBER_ONLY),true)

# These are defined by the core build system, but we might need them
# earlier to feature-check the compilers
#
_CC		:= $(if $(filter default,$(origin CC)),gcc,$(CC))
_CLANG	:= \
 $(shell $(TOP)/eurasiacon/build/linux2/tools/cc-check.sh --clang --cc $(_CC))
ifeq ($(_CLANG),true)
_CC		:= $(_CC) -target $(patsubst %-,%,$(CROSS_COMPILE))
else
_CC		:= $(CROSS_COMPILE)$(_CC)
endif
HOST_CC	?= gcc

-include ../config/user-defs.mk

# FIXME: Backwards compatibility remaps.
#
ifeq ($(SUPPORT_SLC),1)
SGX_FEATURE_SYSTEM_CACHE := 1
endif
ifeq ($(BYPASS_SLC),1)
SGX_BYPASS_SYSTEM_CACHE := 1
endif
ifeq ($(BYPASS_DCU),1)
SGX_BYPASS_DCU := 1
endif
ifneq ($(SGXCOREREV),)
SGX_CORE_REV := $(SGXCOREREV)
endif

# Core handling
#
ifeq ($(SGXCORE),)
$(error Must specify SGXCORE)
endif
ifeq ($(SGX_CORE_REV),)
override USE_SGX_CORE_REV_HEAD := 1
else ifeq ($(SGX_CORE_REV),000)
override USE_SGX_CORE_REV_HEAD := 1
override SGX_CORE_REV :=
else
override USE_SGX_CORE_REV_HEAD := 0
endif

# Enforced dependencies. Move this to an include.
#
ifeq ($(SUPPORT_LINUX_USING_WORKQUEUES),1)
override PVR_LINUX_USING_WORKQUEUES := 1
override PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE := 1
override PVR_LINUX_TIMERS_USING_WORKQUEUES := 1
override SYS_CUSTOM_POWERLOCK_WRAP := 1
else ifeq ($(SUPPORT_LINUX_USING_SHARED_WORKQUEUES),1)
override PVR_LINUX_USING_WORKQUEUES := 1
override PVR_LINUX_MISR_USING_WORKQUEUE := 1
override PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE := 1
override SYS_CUSTOM_POWERLOCK_WRAP := 1
endif

ifneq ($(PDUMP),1)
override SUPPORT_PDUMP_MULTI_PROCESS := 0
endif

ifeq ($(SUPPORT_HYBRID_PB),1)
override SUPPORT_SHARED_PB := 1
override SUPPORT_PERCONTEXT_PB := 1
else ifeq ($(SUPPORT_PERCONTEXT_PB),1)
override SUPPORT_SHARED_PB := 0
endif

ifeq ($(NO_HARDWARE),1)
override SYS_USING_INTERRUPTS := 0
override SUPPORT_HW_RECOVERY := 0
override SUPPORT_ACTIVE_POWER_MANAGEMENT := 0
endif

ifeq ($(SGX_FEATURE_36BIT_MMU),1)
override IMG_ADDRSPACE_PHYSADDR_BITS := 64
else
override IMG_ADDRSPACE_PHYSADDR_BITS := 32
endif

ifeq ($(SGXCORE),535)
ifeq ($(PVRSRV_USSE_EDM_STATUS_DEBUG),1)
SUPPORT_SGX_HWPERF ?= not-overridden
ifeq ($(SUPPORT_SGX_HWPERF),not-overridden)
$(warning Setting SUPPORT_SGX_HWPERF=0 because PVRSRV_USSE_EDM_STATUS_DEBUG=1)
SUPPORT_SGX_HWPERF := 0
endif
endif
PVR2D_ALT_2DHW ?= 0
endif

# Multi-core handling must be done separately to other options
# Also do some sanity checks
#
ifeq ($(SGX_FEATURE_MP),1)
ifeq ($(SGX_FEATURE_MP_CORE_COUNT),)
ifeq ($(SGX_FEATURE_MP_CORE_COUNT_TA),)
$(error Must specify SGX_FEATURE_MP_CORE_COUNT or both SGX_FEATURE_MP_CORE_COUNT_TA and SGX_FEATURE_MP_CORE_COUNT_3D with SGX_FEATURE_MP)
else
$(eval $(call BothConfigC,SGX_FEATURE_MP_CORE_COUNT_TA,$(SGX_FEATURE_MP_CORE_COUNT_TA)))
endif
ifeq ($(SGX_FEATURE_MP_CORE_COUNT_3D),)
$(error Must specify SGX_FEATURE_MP_CORE_COUNT or both SGX_FEATURE_MP_CORE_COUNT_TA and SGX_FEATURE_MP_CORE_COUNT_3D with SGX_FEATURE_MP)
else
$(eval $(call BothConfigC,SGX_FEATURE_MP_CORE_COUNT_3D,$(SGX_FEATURE_MP_CORE_COUNT_3D)))
endif
else
$(eval $(call BothConfigC,SGX_FEATURE_MP_CORE_COUNT,$(SGX_FEATURE_MP_CORE_COUNT)))
endif
endif

# Rather than requiring the user to have to define two variables (one quoted,
# one not), make PVRSRV_MODNAME a non-tunable and give it an overridable
# default here.
#
PVRSRV_MODNAME ?= pvrsrvkm

# Normally builds don't touch this, but we use it to influence the components
# list. Make sure it is defined early enough to make this possible.
#
SUPPORT_PVRSRV_DEVICE_CLASS ?= 1

# The user didn't set CROSS_COMPILE. There's probably nothing wrong
# with that, but we'll let them know anyway.
#
ifeq ($(CROSS_COMPILE),)
$(warning CROSS_COMPILE is not set. Target components will be built with the host compiler)
endif

# The user is trying to set one of the old SUPPORT_ options on the
# command line or in the environment. This isn't supported any more
# and will often break the build. The user is generally only trying
# to remove a component from the list of targets to build, so we'll
# point them at the new way of doing this.
define sanity-check-support-option-origin
ifeq ($$(filter undefined file,$$(origin $(1))),)
$$(warning *** Setting $(1) via $$(origin $(1)) is deprecated)
$$(error If you are trying to disable a component, use e.g. EXCLUDED_APIS="opengles1 opengl")
endif
endef
$(foreach _o,SYS_CFLAGS SYS_CXXFLAGS SYS_INCLUDES SYS_EXE_LDFLAGS SYS_LIB_LDFLAGS SYS_EXE_LDFLAGS_CXX SYS_LIB_LDFLAGS_CXX SUPPORT_EWS SUPPORT_OPENGLES1 SUPPORT_OPENGLES2 SUPPORT_OPENCL SUPPORT_RSCOMPUTE SUPPORT_OPENGL SUPPORT_UNITTESTS SUPPORT_XORG,$(eval $(call sanity-check-support-option-origin,$(_o))))

# Check for words in EXCLUDED_APIS that aren't understood by the
# common/apis/*.mk files. This should be kept in sync with all the tests on
# EXCLUDED_APIS in those files
_excludable_apis := rscompute opencl opengl opengles1 opengles2 openvg ews unittests xorg xorg_unittests scripts composerhal camerahal memtrackhal
_excluded_apis := $(subst $(comma),$(space),$(EXCLUDED_APIS))
_unrecognised := $(strip $(filter-out $(_excludable_apis),$(_excluded_apis)))
ifneq ($(_unrecognised),)
$(warning *** Unrecognised entries in EXCLUDED_APIS: $(_unrecognised))
$(warning *** EXCLUDED_APIS was set via: $(origin EXCLUDED_APIS))
$(error Excludable APIs are: $(_excludable_apis))
endif

override EXCLUDED_APIS := $(filter $(_excludable_apis), $(_excluded_apis))

# Build's selected list of components
#
-include components.mk

# PDUMP needs extra components
#
ifeq ($(PDUMP),1)
ifneq ($(COMPONENTS),)
COMPONENTS += pdump
endif
ifeq ($(SUPPORT_DRI_DRM),1)
EXTRA_PVRSRVKM_COMPONENTS += dbgdrv
else
KERNEL_COMPONENTS += dbgdrv
endif
endif

ifeq ($(SUPPORT_PVR_REMOTE),1)
ifneq ($(filter pvr2d,$(COMPONENTS)),)
COMPONENTS += null_pvr2d_remote
endif
COMPONENTS += pvrvncsrv
COMPONENTS += pvrvncinput
endif

$(if $(filter config,$(D)),$(info Build configuration:))

################################# CONFIG ####################################

# If KERNELDIR is set, write it out to the config.mk, with
# KERNEL_COMPONENTS and KERNEL_ID
#
ifneq ($(strip $(KERNELDIR)),)
include ../kernel_version.mk
PVRSRV_MODULE_BASEDIR ?= /lib/modules/$(KERNEL_ID)/extra/
$(eval $(call KernelConfigMake,KERNELDIR,$(KERNELDIR)))
# Needed only by install script
$(eval $(call KernelConfigMake,KERNEL_COMPONENTS,$(KERNEL_COMPONENTS)))
$(eval $(call TunableKernelConfigMake,EXTRA_PVRSRVKM_COMPONENTS,))
$(eval $(call TunableKernelConfigMake,EXTRA_KBUILD_SOURCE,))

# If KERNEL_CROSS_COMPILE is set to "undef", this is magically
# equivalent to being unset. If it is unset, we use CROSS_COMPILE
# (which might also be unset). If it is set, use it directly.
ifneq ($(KERNEL_CROSS_COMPILE),undef)
KERNEL_CROSS_COMPILE ?= $(CROSS_COMPILE)
$(eval $(call TunableBothConfigMake,KERNEL_CROSS_COMPILE,))
endif

# Check the KERNELDIR has a kernel built.
VMLINUX := $(strip $(wildcard $(KERNELDIR)/vmlinux))
LINUXCFG := $(strip $(wildcard $(KERNELDIR)/.config))

ifneq ($(VMLINUX),)
ifneq ($(shell file $(KERNELDIR)/vmlinux | grep 64-bit >/dev/null && echo 1),$(shell $(_CC) -dM -E - </dev/null | grep __x86_64__ >/dev/null && echo 1))
$(error Attempting to build 64-bit DDK against 32-bit kernel, or 32-bit DDK against 64-bit kernel. This is not allowed.)
endif
VMLINUX_IS_64BIT := $(shell file $(VMLINUX) | grep 64-bit >/dev/null || echo false)
VMLINUX_HAS_PAE36 := $(shell cat $(LINUXCFG) | grep CONFIG_X86_PAE=y >/dev/null || echo false)
VMLINUX_HAS_PAE40 := $(shell cat $(LINUXCFG) | grep CONFIG_ARM_LPAE=y >/dev/null || echo false)
VMLINUX_HAS_DMA32 := $(shell cat $(LINUXCFG) | grep CONFIG_ZONE_DMA32=y >/dev/null || echo false)

# $(error 64BIT=$(VMLINUX_IS_64BIT) PAE36=$(VMLINUX_HAS_PAE36) PAE40=$(VMLINUX_HAS_PAE40) DMA32=$(VMLINUX_HAS_DMA32) MMU36=$(SGX_FEATURE_36BIT_MMU))

ifneq ($(VMLINUX_IS_64BIT),false)
$(warning $$(KERNELDIR)/vmlinux: Note: vmlinux is 64-bit, which is supported but currently experimental.)
endif
else
$(warning $$(KERNELDIR)/vmlinux does not exist. Kbuild may fail.)
endif
endif

ifneq ($(VMLINUX_HAS_PAE40),false)
ifeq ($(VMLINUX_HAS_DMA32),false)
$(warning SGX MMUs are currently supported up to only 36 bits max. Your Kernel is built with 40-bit PAE but does not have CONFIG_ZONE_DMA32.)
$(warning This means you must ensure the runtime system has <= 4GB of RAM, or there will be BIG problems...)
endif 
endif

ifneq ($(SGX_FEATURE_36BIT_MMU),1)
ifneq ($(VMLINUX_IS_64BIT),false)
# Kernel is 64-bit
ifeq ($(VMLINUX_HAS_DMA32),false)
$(warning SGX is configured with 32-bit MMU. Your Kernel is 64-bit but does not have CONFIG_ZONE_DMA32.)
$(warning This means you must ensure the runtime system has <= 4GB of RAM, or there will be BIG problems...)
endif
else
 # Kernel is 32-bit
ifneq ($(VMLINUX_HAS_PAE36),false)
ifeq ($(VMLINUX_HAS_DMA32),false)
$(warning SGX is configured with 32-bit MMU. Your Kernel is 32-bit PAE, but does not have CONFIG_ZONE_DMA32. )
$(warning This means you must ensure the runtime system has <= 4GB of RAM, or there will be BIG problems...)
endif
endif
endif
endif


# Ideally configured by platform Makefiles, as necessary
#

# Invariant options for Linux
#
$(eval $(call BothConfigC,LINUX,))

$(eval $(call BothConfigC,PVR_BUILD_DIR,"\"$(PVR_BUILD_DIR)\""))
$(eval $(call BothConfigC,PVR_BUILD_TYPE,"\"$(BUILD)\""))
$(eval $(call BothConfigC,PVRSRV_MODNAME,"\"$(PVRSRV_MODNAME)\""))

$(eval $(call TunableBothConfigC,SGXCORE,))
$(eval $(call BothConfigC,SGX$(SGXCORE),))
$(eval $(call BothConfigC,SUPPORT_SGX$(SGXCORE),))

$(eval $(call TunableBothConfigC,SUPPORT_SGX,1))
$(eval $(call TunableBothConfigC,SGX_CORE_REV,))
$(eval $(call TunableBothConfigC,USE_SGX_CORE_REV_HEAD,))

$(eval $(call BothConfigC,TRANSFER_QUEUE,))
$(eval $(call BothConfigC,PVR_SECURE_HANDLES,))

# Support syncing LISR & MISR. This is required for OS's where
# on SPM platforms the LISR and MISR can run at the same time and
# thus during powerdown we need to drain all pending LISRs before
# proceeding to do the actual powerdown
$(eval $(call KernelConfigC,SUPPORT_LISR_MISR_SYNC))

ifneq ($(DISPLAY_CONTROLLER),)
$(eval $(call BothConfigC,DISPLAY_CONTROLLER,$(DISPLAY_CONTROLLER)))
endif

ifneq ($(strip $(KERNELDIR)),)
PVR_LINUX_MEM_AREA_POOL_MAX_PAGES ?= 0
ifneq ($(PVR_LINUX_MEM_AREA_POOL_MAX_PAGES),0)
PVR_LINUX_MEM_AREA_USE_VMAP ?= 1
include ../kernel_version.mk
ifeq ($(call kernel-version-at-least,3,0),true)
PVR_LINUX_MEM_AREA_POOL_ALLOW_SHRINK ?= 1
endif
endif
$(eval $(call KernelConfigC,PVR_LINUX_MEM_AREA_POOL_MAX_PAGES,$(PVR_LINUX_MEM_AREA_POOL_MAX_PAGES)))
$(eval $(call TunableKernelConfigC,PVR_LINUX_MEM_AREA_USE_VMAP,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_MEM_AREA_POOL_ALLOW_SHRINK,))
endif


$(eval $(call BothConfigMake,PVR_SYSTEM,$(PVR_SYSTEM)))


# Build-type dependent options
#
$(eval $(call BothConfigMake,BUILD,$(BUILD)))

ifeq ($(BUILD),debug)
$(eval $(call BothConfigC,DEBUG,))
$(eval $(call KernelConfigC,DEBUG_LINUX_MEMORY_ALLOCATIONS,))
$(eval $(call KernelConfigC,DEBUG_LINUX_MEM_AREAS,))
$(eval $(call KernelConfigC,DEBUG_LINUX_MMAP_AREAS,))
$(eval $(call KernelConfigC,DEBUG_BRIDGE_KM,))
else ifeq ($(BUILD),release)
$(eval $(call BothConfigC,RELEASE,))
$(eval $(call TunableBothConfigMake,DEBUGLINK,1))
else ifeq ($(BUILD),timing)
$(eval $(call BothConfigC,TIMING,))
$(eval $(call TunableBothConfigMake,DEBUGLINK,1))
else
$(error BUILD= must be either debug, release or timing)
endif

# User-configurable options
#
$(eval $(call TunableBothConfigC,SUPPORT_PERCONTEXT_PB,1))
$(eval $(call TunableBothConfigC,SUPPORT_SHARED_PB,))
$(eval $(call TunableBothConfigC,SUPPORT_HYBRID_PB,))
$(eval $(call TunableBothConfigC,SUPPORT_HW_RECOVERY,1))
$(eval $(call TunableBothConfigC,SUPPORT_ACTIVE_POWER_MANAGEMENT,1))
$(eval $(call TunableBothConfigC,SUPPORT_SGX_HWPERF,1))
$(eval $(call TunableBothConfigC,SUPPORT_SGX_LOW_LATENCY_SCHEDULING,1))
$(eval $(call TunableBothConfigC,SUPPORT_MEMINFO_IDS,))
$(eval $(call TunableBothConfigC,SUPPORT_SGX_NEW_STATUS_VALS,1))
$(eval $(call TunableBothConfigC,SUPPORT_PDUMP_MULTI_PROCESS,))
$(eval $(call TunableBothConfigC,SUPPORT_DBGDRV_EVENT_OBJECTS,1))
$(eval $(call TunableBothConfigC,SGX_FEATURE_SYSTEM_CACHE,))
$(eval $(call TunableBothConfigC,SGX_BYPASS_SYSTEM_CACHE,))
$(eval $(call TunableBothConfigC,SGX_BYPASS_DCU,))
$(eval $(call TunableBothConfigC,SGX_FAST_DPM_INIT,))
$(eval $(call TunableBothConfigC,SGX_FEATURE_MP,))
$(eval $(call TunableBothConfigC,SGX_FEATURE_MP_PLUS,))
$(eval $(call TunableBothConfigC,FPGA,))
$(eval $(call TunableBothConfigC,PDUMP,))
$(eval $(call TunableBothConfigC,MEM_TRACK_INFO_DEBUG,))
$(eval $(call TunableBothConfigC,NO_HARDWARE,))
$(eval $(call TunableBothConfigC,PDUMP_DEBUG_OUTFILES,))
$(eval $(call TunableBothConfigC,PVRSRV_USSE_EDM_STATUS_DEBUG,))
$(eval $(call TunableBothConfigC,PVRSRV_RESET_ON_HWTIMEOUT,))
$(eval $(call TunableBothConfigC,SYS_USING_INTERRUPTS,1))
$(eval $(call TunableBothConfigC,SUPPORT_EXTERNAL_SYSTEM_CACHE,))
$(eval $(call TunableBothConfigC,PVRSRV_NEW_PVR_DPF,))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_DPF,))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_ASSERT,))
$(eval $(call TunableBothConfigC,PVRSRV_NEED_PVR_TRACE,))
$(eval $(call TunableBothConfigC,SUPPORT_SECURE_33657_FIX,))
$(eval $(call TunableBothConfigC,SUPPORT_ION,))
$(eval $(call TunableBothConfigC,SUPPORT_HWRECOVERY_TRACE_LIMIT,))
$(eval $(call TunableBothConfigC,SUPPORT_PVRSRV_DEVICE_CLASS,))
$(eval $(call TunableBothConfigC,SUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER,1))
$(eval $(call TunableBothConfigC,SUPPORT_NV12_FROM_2_HWADDRS,))
$(eval $(call TunableBothConfigC,SGX_FEATURE_36BIT_MMU,))
$(eval $(call TunableBothConfigC,IMG_ADDRSPACE_PHYSADDR_BITS,))
$(eval $(call TunableBothConfigC,PVRSRV_EXTRA_PB_DEBUG,))
$(eval $(call TunableBothConfigC,PVRSRV_DEBUG_CCB_MAX,))

$(eval $(call TunableKernelConfigC,SUPPORT_LINUX_X86_WRITECOMBINE,1))
$(eval $(call TunableKernelConfigC,SUPPORT_LINUX_X86_PAT,1))
$(eval $(call TunableKernelConfigC,SGX_DYNAMIC_TIMING_INFO,))
$(eval $(call TunableKernelConfigC,SYS_SGX_ACTIVE_POWER_LATENCY_MS,))
$(eval $(call TunableKernelConfigC,SYS_CUSTOM_POWERLOCK_WRAP,))
$(eval $(call TunableKernelConfigC,SYS_SUPPORTS_SGX_IDLE_CALLBACK,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_USING_WORKQUEUES,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_MISR_USING_WORKQUEUE,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_TIMERS_USING_WORKQUEUES,))
$(eval $(call TunableKernelConfigC,PVR_LINUX_TIMERS_USING_SHARED_WORKQUEUE,))
$(eval $(call TunableKernelConfigC,LDM_PLATFORM,))
$(eval $(call TunableKernelConfigC,PVR_LDM_PLATFORM_PRE_REGISTERED,))
$(eval $(call TunableKernelConfigC,PVR_LDM_PLATFORM_PRE_REGISTERED_DEV,))
$(eval $(call TunableKernelConfigC,PVR_LDM_DRIVER_REGISTRATION_NAME,"\"$(PVRSRV_MODNAME)\""))
$(eval $(call TunableKernelConfigC,LDM_PCI,))
$(eval $(call TunableKernelConfigC,PVRSRV_DUMP_MK_TRACE,))
$(eval $(call TunableKernelConfigC,PVRSRV_DUMP_KERNEL_CCB,))
$(eval $(call TunableKernelConfigC,PVRSRV_REFCOUNT_DEBUG,))
$(eval $(call TunableKernelConfigC,PVRSRV_MMU_MAKE_READWRITE_ON_DEMAND,))
$(eval $(call TunableKernelConfigC,HYBRID_SHARED_PB_SIZE,))
$(eval $(call TunableKernelConfigC,SUPPORT_LARGE_GENERAL_HEAP,))
$(eval $(call TunableKernelConfigC,SUPPORT_OLD_ION_API,))
$(eval $(call TunableKernelConfigC,TTRACE,))
$(eval $(call TunableKernelConfigC,TTRACE_LARGE_BUFFER,))
$(eval $(call TunableKernelConfigC,SUPPORT_PDUMP_SYNC_DEBUG,))
$(eval $(call TunableKernelConfigC,SUPPORT_PER_SYNC_DEBUG,))
$(eval $(call TunableKernelConfigC,SUPPORT_FORCE_SYNC_DUMP,))



$(eval $(call TunableBothConfigMake,OPTIM,))
$(eval $(call TunableBothConfigMake,SUPPORT_ION,))
$(eval $(call TunableBothConfigMake,SUPPORT_PVRSRV_DEVICE_CLASS,))


$(eval $(call TunableKernelConfigMake,TTRACE,))

endif # INTERNAL_CLOBBER_ONLY

export INTERNAL_CLOBBER_ONLY
export TOP
export OUT

MAKE_ETC := -Rr --no-print-directory -C $(TOP) TOP=$(TOP) OUT=$(OUT) \
	        -f eurasiacon/build/linux2/toplevel.mk

# This must match the default value of MAKECMDGOALS below, and the default
# goal in toplevel.mk
.DEFAULT_GOAL := build

ifeq ($(MAKECMDGOALS),)
MAKECMDGOALS := build
else
# We can't pass autogen to toplevel.mk
MAKECMDGOALS := $(filter-out autogen,$(MAKECMDGOALS))
endif

.PHONY: autogen
autogen:
ifeq ($(INTERNAL_CLOBBER_ONLY),)
	@$(MAKE) -s --no-print-directory -C $(EURASIAROOT) \
		-f eurasiacon/build/linux2/prepare_tree.mk \
		LDM_PCI=$(LDM_PCI) LDM_PLATFORM=$(LDM_PLATFORM)
else
	@:
endif

# This deletes built-in suffix rules. Otherwise the submake isn't run when
# saying e.g. "make thingy.a"
.SUFFIXES:

# Because we have a match-anything rule below, we'll run the main build when
# we're actually trying to remake various makefiles after they're read in.
# These rules try to prevent that
%.mk: ;
Makefile%: ;
Makefile: ;

.PHONY: build kbuild install
build kbuild install: autogen
	@$(if $(MAKECMDGOALS),$(MAKE) $(MAKE_ETC) $(MAKECMDGOALS) $(eval MAKECMDGOALS :=),:)

%: autogen
	@$(if $(MAKECMDGOALS),$(MAKE) $(MAKE_ETC) $(MAKECMDGOALS) $(eval MAKECMDGOALS :=),:)
