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

include ../config/preconfig.mk
include ../config/window_system.mk

ifeq ($(TC_DISPLAY_MEM_SIZE),)
 # Remember if TC_DISPLAY_MEM_SIZE was unset before ../tc_linux/tc_common.mk is
 # included. We need to override it in some cases but only when it was not set
 # from the command line.
 TC_DISPLAY_MEM_SIZE_WAS_UNSET := 1
endif

# Inherit build config from tc_linux
include ../tc_linux/tc_common.mk

# Override the heap size for the Display Class (DC)
ifneq ($(SUPPORT_KMS),1)
 ifeq ($(SUPPORT_DISPLAY_CLASS),1)
  # Set only when TC_DISPLAY_MEM_SIZE was not set from the command line.
  ifeq ($(TC_DISPLAY_MEM_SIZE_WAS_UNSET),1)
   TC_DISPLAY_MEM_SIZE := 383
  endif
 endif
endif

# if defined to 1, core.mk will skip setting SUPPORT_WRAP_EXTMEM
# allowing us to instead set it later in this Makefile based on
# TC_MEMORY_CONFIG
PLATFORM_SUPPORT_WRAP_EXTMEM := 1

VMM_TYPE := vzfpga

ifdef SUPPORT_TRUSTED_DEVICE
 # the TEE_DDK is currently tested only with premapped firmware heaps
 RGX_PREMAP_FW_HEAPS := 1
 KERNEL_COMPONENTS += tee_ddk
endif

ifdef RGX_NUM_DRIVERS_SUPPORTED
 ifeq ($(RGX_NUM_DRIVERS_SUPPORTED),1)
  # native build, no vz options
 else
  KERNEL_COMPONENTS += vm_migration_test
  DEFAULT_FPGA_CARD_RAM ?= 0x100000000

  # round the number of drivers supported to a power of two so the Odin card memory
  # carveouts won't create problems with the DDK's heap alignment requirements
  ENV_NUM_DRIVERS := RGX_NUM_DRIVERS_SUPPORTED
  RGX_NUM_DRIVERS_SUPPORTED := $(shell awk 'BEGIN{p=1; while(p<$(RGX_NUM_DRIVERS_SUPPORTED)){p*=2}; print p}')

ifdef SUPPORT_TRUSTED_DEVICE
  # leave some OSIDs for security
  RGX_NUM_DRIVERS_SUPPORTED := $(shell [ $(RGX_NUM_DRIVERS_SUPPORTED) -le 4 ] && echo $(RGX_NUM_DRIVERS_SUPPORTED) || echo 4)

  # VZ + DRM security uses shared memory for AutoVz
  SUPPORT_AUTOVZ_HW_REGS ?= 0
endif

  # use a tested vz configuration by default
  SUPPORT_AUTOVZ ?= 1
  PVRSRV_DEVICE_INIT_MODE ?= PVRSRV_LINUX_DEV_INIT_ON_PROBE
  PVR_AUTOVZ_WDG_PERIOD_MS ?= 1000
  RGX_HCS_DEFAULT_DEADLINE_MS ?= 200
  PVRSRV_APPHINT_GUESTFWHEAPSTRIDE ?= ($(DEFAULT_FPGA_CARD_RAM)/$(RGX_NUM_DRIVERS_SUPPORTED))
  PVRSRV_APPHINT_AUTOVZGPUPOWERDOWN ?= 1

 ifeq ($(PVR_ARCH),rogue)
  HW_BRANCH := $(shell echo $(RGX_BVNC) | cut -d'.' -f1)

  ifeq ($(shell test $(HW_BRANCH) -lt 36; echo $$?), 0)
   # likely older rogue cores with more limited vz features
   SUPPORT_AUTOVZ_HW_REGS ?= 0
  else
   # modern rogue cores
   SUPPORT_AUTOVZ_HW_REGS ?= 1
  endif
   # avoid MIPS bug on OSID7's heap mapping with 32MB Fw heaps
   RGX_FW_HEAP_SHIFT ?= 24
 else
  SUPPORT_AUTOVZ_HW_REGS ?= 1
  RGX_FW_HEAP_SHIFT ?= 25
  # Support for volcanic cores with 32 OSIDs
  PVRSRV_MAX_DEVICES ?= 32
 endif

 endif
endif

# Assume a frequency of 10Mhz
VULKAN_TIMESTAMP_FREQUENCY ?= 10000000

# Should be last
include ../config/core.mk
$(eval $(call TunableBothConfigC,FPGA,1))

-include ../common/lws.mk
include ../common/3rdparty.mk
include ../common/testchip.mk

# Platform-specific definition of SUPPORT_WRAP_EXTMEM
ifeq ($(PLATFORM_SUPPORT_WRAP_EXTMEM),1)
 ifeq ($(TC_MEMORY_CONFIG),)
  $(error TC_MEMORY_CONFIG must be defined)
 endif
 ifeq ($(TC_MEMORY_CONFIG),TC_MEMORY_HYBRID)
  $(eval $(call TunableBothConfigC,SUPPORT_WRAP_EXTMEM,SUPPORT_WRAP_EXTMEM))
 else
  ifeq ($(TC_MEMORY_CONFIG),TC_MEMORY_HOST)
   $(eval $(call TunableBothConfigC,SUPPORT_WRAP_EXTMEM,SUPPORT_WRAP_EXTMEM))
  endif
 endif
endif
