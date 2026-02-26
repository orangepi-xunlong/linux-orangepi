/*************************************************************************/ /*!
@File
@Title          Process and driver statistic definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#ifndef PROC_STATS_H
#define PROC_STATS_H

#define DKP_HIDDEN "hidden"

#define PROCESS_STAT_KMALLOC                   X(PVRSRV_PROCESS_STAT_TYPE_KMALLOC,                "MemoryUsageKMalloc",                "host_kmalloc")
#define PROCESS_STAT_KMALLOC_MAX               X(PVRSRV_PROCESS_STAT_TYPE_KMALLOC_MAX,            "MemoryUsageKMallocMax",             DKP_HIDDEN)

#define PROCESS_STAT_VMALLOC                   X(PVRSRV_PROCESS_STAT_TYPE_VMALLOC,                "MemoryUsageVMalloc",                "host_vmalloc")
#define PROCESS_STAT_VMALLOC_MAX               X(PVRSRV_PROCESS_STAT_TYPE_VMALLOC_MAX,            "MemoryUsageVMallocMax",             DKP_HIDDEN)

#define PROCESS_STAT_ALLOC_PAGES_PT_UMA        X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_PAGES_PT_UMA,     "MemoryUsageAllocPTMemoryUMA",       "host_mem_dev_pt")
#define PROCESS_STAT_ALLOC_PAGES_PT_UMA_MAX    X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_PAGES_PT_UMA_MAX, "MemoryUsageAllocPTMemoryUMAMax",    DKP_HIDDEN)

#define PROCESS_STAT_VMAP_PT_UMA               X(PVRSRV_PROCESS_STAT_TYPE_VMAP_PT_UMA,            "MemoryUsageVMapPTUMA",              DKP_HIDDEN)
#define PROCESS_STAT_VMAP_PT_UMA_MAX           X(PVRSRV_PROCESS_STAT_TYPE_VMAP_PT_UMA_MAX,        "MemoryUsageVMapPTUMAMax",           DKP_HIDDEN)

#define PROCESS_STAT_ALLOC_PAGES_PT_LMA        X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_PAGES_PT_LMA,     "MemoryUsageAllocPTMemoryLMA",       "local_mem_dev_pt")
#define PROCESS_STAT_ALLOC_PAGES_PT_LMA_MAX    X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_PAGES_PT_LMA_MAX, "MemoryUsageAllocPTMemoryLMAMax",    DKP_HIDDEN)

#define PROCESS_STAT_IOREMAP_PT_LMA            X(PVRSRV_PROCESS_STAT_TYPE_IOREMAP_PT_LMA,         "MemoryUsageIORemapPTLMA",           DKP_HIDDEN)
#define PROCESS_STAT_IOREMAP_PT_LMA_MAX        X(PVRSRV_PROCESS_STAT_TYPE_IOREMAP_PT_LMA_MAX,     "MemoryUsageIORemapPTLMAMax",        DKP_HIDDEN)

#define PROCESS_STAT_ALLOC_LMA_PAGES           X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_LMA_PAGES,        "MemoryUsageAllocGPUMemLMA",         "local_mem_dev_buf")
#define PROCESS_STAT_ALLOC_LMA_PAGES_MAX       X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_LMA_PAGES_MAX,    "MemoryUsageAllocGPUMemLMAMax",      DKP_HIDDEN)

#if defined(SUPPORT_PMR_DEFERRED_FREE)
#define PROCESS_STAT_ZOMBIE_LMA_PAGES          X(PVRSRV_PROCESS_STAT_TYPE_ZOMBIE_LMA_PAGES,       "MemoryUsageZombieGPUMemLMA",        "local_mem_dev_buf_purgeable")
#define PROCESS_STAT_ZOMBIE_LMA_PAGES_MAX      X(PVRSRV_PROCESS_STAT_TYPE_ZOMBIE_LMA_PAGES_MAX,   "MemoryUsageZombieGPUMemLMAMax",     DKP_HIDDEN)
#else
#define PROCESS_STAT_ZOMBIE_LMA_PAGES
#define PROCESS_STAT_ZOMBIE_LMA_PAGES_MAX
#endif

#define PROCESS_STAT_ALLOC_UMA_PAGES           X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_UMA_PAGES,        "MemoryUsageAllocGPUMemUMA",         "host_mem_dev_buf")
#define PROCESS_STAT_ALLOC_UMA_PAGES_MAX       X(PVRSRV_PROCESS_STAT_TYPE_ALLOC_UMA_PAGES_MAX,    "MemoryUsageAllocGPUMemUMAMax",      DKP_HIDDEN)

#if defined(SUPPORT_PMR_DEFERRED_FREE)
#define PROCESS_STAT_ZOMBIE_UMA_PAGES          X(PVRSRV_PROCESS_STAT_TYPE_ZOMBIE_UMA_PAGES,       "MemoryUsageZombieGPUMemUMA",        "host_mem_dev_buf_purgeable")
#define PROCESS_STAT_ZOMBIE_UMA_PAGES_MAX      X(PVRSRV_PROCESS_STAT_TYPE_ZOMBIE_UMA_PAGES_MAX,   "MemoryUsageZombieGPUMemUMAMax",     DKP_HIDDEN)
#else
#define PROCESS_STAT_ZOMBIE_UMA_PAGES
#define PROCESS_STAT_ZOMBIE_UMA_PAGES_MAX
#endif

#define PROCESS_STAT_MAP_UMA_LMA_PAGES         X(PVRSRV_PROCESS_STAT_TYPE_MAP_UMA_LMA_PAGES,      "MemoryUsageMappedGPUMemUMA/LMA",    DKP_HIDDEN)
#define PROCESS_STAT_MAP_UMA_LMA_PAGES_MAX     X(PVRSRV_PROCESS_STAT_TYPE_MAP_UMA_LMA_PAGES_MAX,  "MemoryUsageMappedGPUMemUMA/LMAMax", DKP_HIDDEN)

#define PROCESS_STAT_DMA_BUF_IMPORT            X(PVRSRV_PROCESS_STAT_TYPE_DMA_BUF_IMPORT,         "MemoryUsageDmaBufImport",           "dma_buf_import")
#define PROCESS_STAT_DMA_BUF_IMPORT_MAX        X(PVRSRV_PROCESS_STAT_TYPE_DMA_BUF_IMPORT_MAX,     "MemoryUsageDmaBufImportMax",        DKP_HIDDEN)

#if defined(SUPPORT_PMR_DEFERRED_FREE)
#define PROCESS_STAT_DMA_BUF_ZOMBIE            X(PVRSRV_PROCESS_STAT_TYPE_DMA_BUF_ZOMBIE,         "MemoryUsageDmaBufZombie",           "dma_buf_purgeable")
#define PROCESS_STAT_DMA_BUF_ZOMBIE_MAX        X(PVRSRV_PROCESS_STAT_TYPE_DMA_BUF_ZOMBIE_MAX,     "MemoryUsageDmaBufZombieMax",        DKP_HIDDEN)
#else
#define PROCESS_STAT_DMA_BUF_ZOMBIE
#define PROCESS_STAT_DMA_BUF_ZOMBIE_MAX
#endif

#define PROCESS_STAT_TOTAL                     X(PVRSRV_PROCESS_STAT_TYPE_TOTAL,                  "MemoryUsageTotal",                  DKP_HIDDEN)
#define PROCESS_STAT_TOTAL_MAX                 X(PVRSRV_PROCESS_STAT_TYPE_TOTAL_MAX,              "MemoryUsageTotalMax",               DKP_HIDDEN)

/* Process stat keys */
#define PVRSRV_PROCESS_STAT_KEY \
	PROCESS_STAT_KMALLOC \
	PROCESS_STAT_KMALLOC_MAX \
	PROCESS_STAT_VMALLOC \
	PROCESS_STAT_VMALLOC_MAX \
	PROCESS_STAT_ALLOC_PAGES_PT_UMA \
	PROCESS_STAT_ALLOC_PAGES_PT_UMA_MAX \
	PROCESS_STAT_VMAP_PT_UMA \
	PROCESS_STAT_VMAP_PT_UMA_MAX \
	PROCESS_STAT_ALLOC_PAGES_PT_LMA \
	PROCESS_STAT_ALLOC_PAGES_PT_LMA_MAX \
	PROCESS_STAT_IOREMAP_PT_LMA \
	PROCESS_STAT_IOREMAP_PT_LMA_MAX \
	PROCESS_STAT_ALLOC_LMA_PAGES \
	PROCESS_STAT_ALLOC_LMA_PAGES_MAX \
	PROCESS_STAT_ZOMBIE_LMA_PAGES \
	PROCESS_STAT_ZOMBIE_LMA_PAGES_MAX \
	PROCESS_STAT_ALLOC_UMA_PAGES \
	PROCESS_STAT_ALLOC_UMA_PAGES_MAX \
	PROCESS_STAT_ZOMBIE_UMA_PAGES \
	PROCESS_STAT_ZOMBIE_UMA_PAGES_MAX \
	PROCESS_STAT_MAP_UMA_LMA_PAGES \
	PROCESS_STAT_MAP_UMA_LMA_PAGES_MAX \
	PROCESS_STAT_DMA_BUF_IMPORT \
	PROCESS_STAT_DMA_BUF_IMPORT_MAX \
	PROCESS_STAT_DMA_BUF_ZOMBIE \
	PROCESS_STAT_DMA_BUF_ZOMBIE_MAX \
	PROCESS_STAT_TOTAL \
	PROCESS_STAT_TOTAL_MAX

#if defined(SUPPORT_LINUX_FDINFO)
/* DKP process stats within the drm-memory-<region> key */
#define PVRSRV_DKP_MEM_STAT_GROUP_MEMORY \
	PROCESS_STAT_KMALLOC \
	PROCESS_STAT_VMALLOC \
	PROCESS_STAT_ALLOC_PAGES_PT_UMA \
	PROCESS_STAT_ALLOC_UMA_PAGES \
	PROCESS_STAT_ZOMBIE_UMA_PAGES

/* DKP process stats within the drm-shared-<region> key */
#define PVRSRV_DKP_MEM_STAT_GROUP_SHARED \
	PROCESS_STAT_DMA_BUF_IMPORT \
	PROCESS_STAT_DMA_BUF_ZOMBIE

/* DKP process stats within the drm-total-<region> key */
#define PVRSRV_DKP_MEM_STAT_GROUP_TOTAL \
	PROCESS_STAT_KMALLOC \
	PROCESS_STAT_VMALLOC \
	PROCESS_STAT_ALLOC_PAGES_PT_UMA \
	PROCESS_STAT_ALLOC_UMA_PAGES \
	PROCESS_STAT_ALLOC_PAGES_PT_LMA \
	PROCESS_STAT_ALLOC_LMA_PAGES

/* DKP process stats within the drm-resident-<region> key */
#define PVRSRV_DKP_MEM_STAT_GROUP_RESIDENT \
	PROCESS_STAT_KMALLOC \
	PROCESS_STAT_VMALLOC \
	PROCESS_STAT_ALLOC_PAGES_PT_UMA \
	PROCESS_STAT_ALLOC_UMA_PAGES \
	PROCESS_STAT_ZOMBIE_UMA_PAGES \
	PROCESS_STAT_ALLOC_PAGES_PT_LMA \
	PROCESS_STAT_ALLOC_LMA_PAGES \
	PROCESS_STAT_ZOMBIE_LMA_PAGES \

/* DKP process stats within the drm-purgeable-<region> key */
#define PVRSRV_DKP_MEM_STAT_GROUP_PURGEABLE \
	PROCESS_STAT_ZOMBIE_LMA_PAGES \
	PROCESS_STAT_ZOMBIE_UMA_PAGES \
	PROCESS_STAT_DMA_BUF_ZOMBIE

/* DKP process stats within the drm-active-<region> key */
#define PVRSRV_DKP_MEM_STAT_GROUP_ACTIVE

#endif

/* X-Macro for Device stat keys */
#define PVRSRV_DEVICE_STAT_KEY \
	X(PVRSRV_DEVICE_STAT_TYPE_CONNECTIONS, "Connections") \
	X(PVRSRV_DEVICE_STAT_TYPE_MAX_CONNECTIONS, "ConnectionsMax") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_OOMS, "RenderContextOutOfMemoryEvents") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_PRS, "RenderContextPartialRenders") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_GROWS, "RenderContextGrows") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_PUSH_GROWS, "RenderContextPushGrows") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_TA_STORES, "RenderContextTAStores") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_3D_STORES, "RenderContext3DStores") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_CDM_STORES, "RenderContextCDMStores") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_TDM_STORES, "RenderContextTDMStores") \
	X(PVRSRV_DEVICE_STAT_TYPE_RC_RAY_STORES, "RenderContextRayStores") \
	X(PVRSRV_DEVICE_STAT_TYPE_ZSBUFFER_REQS_BY_APP, "ZSBufferRequestsByApp") \
	X(PVRSRV_DEVICE_STAT_TYPE_ZSBUFFER_REQS_BY_FW, "ZSBufferRequestsByFirmware") \
	X(PVRSRV_DEVICE_STAT_TYPE_FREELIST_GROW_REQS_BY_APP, "FreeListGrowRequestsByApp") \
	X(PVRSRV_DEVICE_STAT_TYPE_FREELIST_GROW_REQS_BY_FW, "FreeListGrowRequestsByFirmware") \
	X(PVRSRV_DEVICE_STAT_TYPE_FREELIST_PAGES_INIT, "FreeListInitialPages") \
	X(PVRSRV_DEVICE_STAT_TYPE_FREELIST_MAX_PAGES, "FreeListMaxPages") \
	X(PVRSRV_DEVICE_STAT_TYPE_OOM_VIRTMEM_COUNT, "MemoryOOMCountDeviceVirtual") \
	X(PVRSRV_DEVICE_STAT_TYPE_OOM_PHYSMEM_COUNT, "MemoryOOMCountPhysicalHeap") \
	X(PVRSRV_DEVICE_STAT_TYPE_INVALID_VIRTMEM, "MemoryOOMCountDeviceVirtualAtAddr")

/* X-Macro for Driver stat keys */
#define PVRSRV_DRIVER_STAT_KEY \
	X(PVRSRV_DRIVER_STAT_TYPE_KMALLOC, "MemoryUsageKMalloc") \
	X(PVRSRV_DRIVER_STAT_TYPE_KMALLOC_MAX, "MemoryUsageKMallocMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_VMALLOC, "MemoryUsageVMalloc") \
	X(PVRSRV_DRIVER_STAT_TYPE_VMALLOC_MAX, "MemoryUsageVMallocMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_PT_MEMORY_UMA, "MemoryUsageAllocPTMemoryUMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_PT_MEMORY_UMA_MAX, "MemoryUsageAllocPTMemoryUMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_VMAP_PT_UMA, "MemoryUsageVMapPTUMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_VMAP_PT_UMA_MAX, "MemoryUsageVMapPTUMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_PT_MEMORY_LMA, "MemoryUsageAllocPTMemoryLMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_PT_MEMORY_LMA_MAX, "MemoryUsageAllocPTMemoryLMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_IOREMAP_PT_LMA, "MemoryUsageIORemapPTLMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_IOREMAP_PT_LMA_MAX, "MemoryUsageIORemapPTLMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_GPUMEM_LMA, "MemoryUsageAllocGPUMemLMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_GPUMEM_LMA_MAX, "MemoryUsageAllocGPUMemLMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ZOMBIE_GPUMEM_LMA, "MemoryUsageZombieGPUMemLMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_ZOMBIE_GPUMEM_LMA_MAX, "MemoryUsageZombieGPUMemLMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_GPUMEM_UMA, "MemoryUsageAllocGPUMemUMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_GPUMEM_UMA_MAX, "MemoryUsageAllocGPUMemUMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ZOMBIE_GPUMEM_UMA, "MemoryUsageZombieGPUMemUMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_ZOMBIE_GPUMEM_UMA_MAX, "MemoryUsageZombieGPUMemUMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_GPUMEM_UMA_POOL, "MemoryUsageAllocGPUMemUMAPool") \
	X(PVRSRV_DRIVER_STAT_TYPE_ALLOC_GPUMEM_UMA_POOL_MAX, "MemoryUsageAllocGPUMemUMAPoolMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_MAPPED_GPUMEM_UMA_LMA, "MemoryUsageMappedGPUMemUMA/LMA") \
	X(PVRSRV_DRIVER_STAT_TYPE_MAPPED_GPUMEM_UMA_LMA_MAX, "MemoryUsageMappedGPUMemUMA/LMAMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_DMA_BUF_IMPORT, "MemoryUsageDmaBufImport") \
	X(PVRSRV_DRIVER_STAT_TYPE_DMA_BUF_IMPORT_MAX, "MemoryUsageDmaBufImportMax") \
	X(PVRSRV_DRIVER_STAT_TYPE_DMA_BUF_ZOMBIE, "MemoryUsageDmaBufZombie") \
	X(PVRSRV_DRIVER_STAT_TYPE_DMA_BUF_ZOMBIE_MAX, "MemoryUsageDmaBufZombieMax")


typedef enum {
#define X(stat_type, stat_str, drm_str) stat_type,
	PVRSRV_PROCESS_STAT_KEY
#undef X
	PVRSRV_PROCESS_STAT_TYPE_COUNT
}PVRSRV_PROCESS_STAT_TYPE;

typedef enum {
#define X(stat_type, stat_str) stat_type,
	PVRSRV_DEVICE_STAT_KEY
#undef X
	PVRSRV_DEVICE_STAT_TYPE_COUNT
}PVRSRV_DEVICE_STAT_TYPE;

typedef enum {
#define X(stat_type, stat_str) stat_type,
	PVRSRV_DRIVER_STAT_KEY
#undef X
	PVRSRV_DRIVER_STAT_TYPE_COUNT
}PVRSRV_DRIVER_STAT_TYPE;

#endif // PROC_STATS_H
