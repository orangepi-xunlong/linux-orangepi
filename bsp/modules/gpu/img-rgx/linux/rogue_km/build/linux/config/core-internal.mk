########################################################################### ###
#@File
#@Title         Internal only build configuration.
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@Description   Contains all config options that are for internal use only and
#               should not be changed in a production environment.
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


ifeq ($(SUPPORT_VALIDATION),1)
# Enable custom register configuration feature for testing and HW validation
SUPPORT_USER_REGISTER_CONFIGURATION ?=1
# Enable per-PID debugfs interface for summary stats.
PVRSRV_ENABLE_PERPID_STATS ?= 1
endif


ifneq ($(SUPPORT_KMS),)

 ifneq ($(PVR_REMVIEW),)

 endif
endif

$(eval $(call TunableBothConfigMake,SUPPORT_USER_REGISTER_CONFIGURATION,,\
Internal use only._\
))
$(eval $(call TunableBothConfigC,SUPPORT_USER_REGISTER_CONFIGURATION,,\
Internal use only._\
))

$(eval $(call TunableBothConfigC,PVRSRV_WRAP_EXTMEM_WRITE_ATTRIB_ENABLE,,\
Setting this option enables the write attribute for all the device mappings acquired_\
through the PVRSRVWrapExtMem interface. Otherwise the option is disabled by default._\
))


$(eval $(call TunableKernelConfigMake,SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK,))
$(eval $(call TunableKernelConfigC,SUPPORT_LINUX_WRAP_EXTMEM_PAGE_TABLE_WALK,,\
This allows the kernel wrap memory handler to determine the pages_\
associated with a given virtual address by performing a walk-through of the corresponding_\
page tables. This method is only used with virtual address regions that belong to device_\
or with virtual memory regions that have VM_IO set._\
This setting is for Linux platforms only ._\
))

# Set to 1 to create 'using_pvrtld' file to identify PDUMP backend

# Set to 0 to not to create out2.pending.txt/prm file

$(eval $(call TunableBothConfigC,SUPPORT_PERCONTEXT_FREELIST,1,Internal use only))

$(eval $(call TunableBothConfigC,SUPPORT_GPIO_VALIDATION,,\
Internal use only._\
))

ifeq ($(NO_HARDWARE),1)
 SUPPORT_SHADOW_FREELISTS := 1
endif

$(eval $(call TunableBothConfigMake,SUPPORT_SHADOW_FREELISTS,,\
Enabling this feature enables shadow freelists and hardware recovery in the firmware._\
))

$(eval $(call TunableBothConfigC,SUPPORT_SHADOW_FREELISTS,,\
Enabling this feature enables shadow freelists and hardware recovery in the firmware._\
))

$(eval $(call TunableKernelConfigC,PVRSRV_ENABLE_CACHEOP_STATS,,\
Enable cache maintenance operations to be recorded and published via _\
DebugFS on Linux._\
))

$(eval $(call TunableBothConfigC,PVRSRV_ENABLE_PERPID_STATS,,\
Enable the presentation of process statistics in the kernel Server module._\
Feature off by default. \
))

$(eval $(call TunableBothConfigC,PVRSRV_ENABLE_LINUX_MMAP_STATS,,\
Linux only. Enable recording user-space memory mapping stats (mmap system calls) in DDK which are_\
presented via mmap_stats debugfs file._\
))

$(eval $(call TunableKernelConfigC,PVRSRV_HWPERF_HOST_DEBUG_DEFERRED_EVENTS,,\
Enable debugging (logs and alerts) of HWPerfHost events generated from atomic contexts._\
))




ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
endif

#
# Firmware statistics framework support.
#
$(eval $(call TunableBothConfigMake,SUPPORT_RGXFW_STATS_FRAMEWORK,,\
Enabling this feature enables an internal framework for statistics_\
monitoring within the firmware. _\
))
$(eval $(call TunableBothConfigC,SUPPORT_RGXFW_STATS_FRAMEWORK,))

#
# GPU virtualization validation
#
$(eval $(call TunableBothConfigC,SUPPORT_GPUVIRT_VALIDATION,,\
Enable validation mode for GPU Virtualisation in which processes inside_\
an OS are given independent OSIDs._\
))
$(eval $(call TunableBothConfigC,GPUVIRT_VALIDATION_NUM_OS,8))
$(eval $(call TunableBothConfigC,GPUVIRT_VALIDATION_NUM_REGIONS,2))

# GPUVIRT_VALIDATION default region values used _in the emulator_.
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_OSIDREGION0MIN,$\
\"0x00000000 0x04000000 0x10000000 0x18000000 0x20000000 0x28000000 0x30000000 0x38000000\",\
Array of comma/space separated strings that define the start addresses for all 8 OSids on Region 0.))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_OSIDREGION0MAX,$\
\"0x3FFFFFFF 0x0FFFFFFF 0x17FFFFFF 0x1FFFFFFF 0x27FFFFFF 0x2FFFFFFF 0x37FFFFFF 0x3FFFFFFF\",\
Array of comma/space separated strings that define the end addresses for all 8 OSids on Region 0.))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_OSIDREGION1MIN,$\
\"0x3F000000 0x3F000000 0x3F000000 0x3F000000 0x3F000000 0x3F000000 0x3F000000 0x3F000000\",\
Array of comma/space separated strings that define the start addresses for all 8 OSids on Region 1.))
$(eval $(call AppHintConfigC,PVRSRV_APPHINT_OSIDREGION1MAX,$\
\"0x3FFFFFFF 0x3FFFFFFF 0x3FFFFFFF 0x3FFFFFFF 0x3FFFFFFF 0x3FFFFFFF 0x3FFFFFFF 0x3FFFFFFF\",\
Array of comma/space separated strings that define the end addresses for all 8 OSids on Region 1.))


$(eval $(call AppHintConfigC,PVRSRV_APPHINT_VALIDATEIRQ,0,\
Used to validate the interrupt integration. \
Enables extra code in the FW to assert all interrupt lines \
at boot and polls on the host side. The code is only built when \
generating pdumps for nohw targets.))

