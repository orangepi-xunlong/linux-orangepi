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

pvrsrvkm-y += \
	services4/srvkm/env/linux/osfunc.o \
	services4/srvkm/env/linux/mutils.o \
	services4/srvkm/env/linux/mmap.o \
	services4/srvkm/env/linux/module.o \
	services4/srvkm/env/linux/pdump.o \
	services4/srvkm/env/linux/proc.o \
	services4/srvkm/env/linux/pvr_bridge_k.o \
	services4/srvkm/env/linux/pvr_debug.o \
	services4/srvkm/env/linux/mm.o \
	services4/srvkm/env/linux/mutex.o \
	services4/srvkm/env/linux/event.o \
	services4/srvkm/env/linux/osperproc.o \
	services4/srvkm/common/buffer_manager.o \
	services4/srvkm/common/devicemem.o \
	services4/srvkm/common/handle.o \
	services4/srvkm/common/hash.o \
	services4/srvkm/common/lists.o \
	services4/srvkm/common/mem.o \
	services4/srvkm/common/mem_debug.o \
	services4/srvkm/common/metrics.o \
	services4/srvkm/common/osfunc_common.o \
	services4/srvkm/common/pdump_common.o \
	services4/srvkm/common/perproc.o \
	services4/srvkm/common/power.o \
	services4/srvkm/common/pvrsrv.o \
	services4/srvkm/common/ra.o \
	services4/srvkm/common/refcount.o \
	services4/srvkm/common/resman.o \
	services4/srvkm/bridged/bridged_support.o \
	services4/srvkm/bridged/bridged_pvr_bridge.o \
	services4/system/$(PVR_SYSTEM)/sysconfig.o \
	services4/system/$(PVR_SYSTEM)/sysutils.o

ifeq ($(SUPPORT_PVRSRV_DEVICE_CLASS),1)
pvrsrvkm-y += \
	services4/srvkm/common/deviceclass.o \
	services4/srvkm/common/queue.o
endif

ifeq ($(SUPPORT_ION),1)
pvrsrvkm-y += \
	services4/srvkm/env/linux/ion.o
endif

ifeq ($(PVR_ANDROID_NATIVE_WINDOW_HAS_SYNC),1)
pvrsrvkm-y += \
	services4/srvkm/env/linux/pvr_sync.o
endif

ifeq ($(TTRACE),1)
pvrsrvkm-y += \
	services4/srvkm/common/ttrace.o
endif

ifeq ($(SUPPORT_PVRSRV_ANDROID_SYSTRACE),1)
pvrsrvkm-y += \
	services4/srvkm/env/linux/systrace.o
endif

ifneq ($(W),1)
CFLAGS_osfunc.o := -Werror
CFLAGS_mutils.o := -Werror
CFLAGS_mmap.o := -Werror
CFLAGS_module.o := -Werror
CFLAGS_pdump.o := -Werror
CFLAGS_proc.o := -Werror
CFLAGS_pvr_bridge_k.o := -Werror
CFLAGS_pvr_debug.o := -Werror
CFLAGS_mm.o := -Werror
CFLAGS_mutex.o := -Werror
CFLAGS_event.o := -Werror
CFLAGS_osperproc.o := -Werror
CFLAGS_buffer_manager.o := -Werror
CFLAGS_devicemem.o := -Werror
CFLAGS_deviceclass.o := -Werror
CFLAGS_handle.o := -Werror
CFLAGS_hash.o := -Werror
CFLAGS_metrics.o := -Werror
CFLAGS_pvrsrv.o := -Werror
CFLAGS_queue.o := -Werror
CFLAGS_ra.o := -Werror
CFLAGS_resman.o := -Werror
CFLAGS_power.o := -Werror
CFLAGS_mem.o := -Werror
CFLAGS_pdump_common.o := -Werror
CFLAGS_bridged_support.o := -Werror
CFLAGS_bridged_pvr_bridge.o := -Werror
CFLAGS_perproc.o := -Werror
CFLAGS_lists.o := -Werror
CFLAGS_mem_debug.o := -Werror
CFLAGS_osfunc_common.o := -Werror
CFLAGS_refcount.o := -Werror
endif

# SUPPORT_SGX==1 only

pvrsrvkm-y += \
	services4/srvkm/bridged/sgx/bridged_sgx_bridge.o \
	services4/srvkm/devices/sgx/sgxinit.o \
	services4/srvkm/devices/sgx/sgxpower.o \
	services4/srvkm/devices/sgx/sgxreset.o \
	services4/srvkm/devices/sgx/sgxutils.o \
	services4/srvkm/devices/sgx/sgxkick.o \
	services4/srvkm/devices/sgx/sgxtransfer.o \
	services4/srvkm/devices/sgx/mmu.o \
	services4/srvkm/devices/sgx/pb.o

ifneq ($(W),1)
CFLAGS_bridged_sgx_bridge.o := -Werror
CFLAGS_sgxinit.o := -Werror
CFLAGS_sgxpower.o := -Werror
CFLAGS_sgxreset.o := -Werror
CFLAGS_sgxutils.o := -Werror
CFLAGS_sgxkick.o := -Werror
CFLAGS_sgxtransfer.o := -Werror
CFLAGS_mmu.o := -Werror
CFLAGS_pb.o := -Werror
endif

ifeq ($(SUPPORT_DRI_DRM),1)

pvrsrvkm-y += \
 services4/srvkm/env/linux/pvr_drm.o

ccflags-y += \
 -Iinclude/drm \
 -I$(TOP)/services4/include/env/linux \

ifeq ($(PVR_DRI_DRM_NOT_PCI),1)
ccflags-y += -I$(TOP)/services4/3rdparty/linux_drm
endif

endif # SUPPORT_DRI_DRM
