/*************************************************************************/ /*!
@File           physmem_ramem.c
@Title          Resource allocator managed PMR Factory common definitions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of Services memory management.  This file defines the
                RA managed memory PMR factory API that is shared between local
                physheap implementations (LMA & IMA)
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

#include "img_types.h"
#include "pvr_debug.h"
#include "pvrsrv_error.h"
#include "pvrsrv_memallocflags.h"
#include "physmem_ramem.h"
#include "physheap.h"
#include "allocmem.h"
#include "ra.h"
#include "connection_server.h"
#include "device.h"
#include "devicemem_server_utils.h"
#include "osfunc.h"
#include "pmr.h"
#include "pmr_impl.h"
#include "rgx_pdump_panics.h"
#include "pdump_km.h"

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#endif

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "rgxutils.h"
#endif

#if defined(INTEGRITY_OS)
#include "mm.h"
#include "integrity_memobject.h"
#endif

#include "physmem_dlm.h"


#if defined(PVRSRV_PHYSHEAP_DISABLE_OOM_DEMOTION)
#define PHYSHEAP_DPF_LVL PVR_DBG_ERROR
#else
#define PHYSHEAP_DPF_LVL PVR_DBG_WARNING
#endif

/* Common Physheap Callback implementations */

IMG_UINT32
RAMemGetPageShift(void)
{
	return PVRSRV_4K_PAGE_SIZE_ALIGNSHIFT;
}

PVRSRV_ERROR
RAMemDoPhyContigPagesAlloc(RA_ARENA *pArena,
                           size_t uiSize,
                           PVRSRV_DEVICE_NODE *psDevNode,
                           PG_HANDLE *psMemHandle,
                           IMG_DEV_PHYADDR *psDevPAddr,
                           IMG_PID uiPid)
{
	RA_BASE_T uiCardAddr = 0;
	RA_LENGTH_T uiActualSize;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32Log2NumPages;

#if defined(DEBUG)
	static IMG_UINT32 ui32MaxLog2NumPagesHistory = 0;
#if defined(RGX_NUM_DRIVERS_SUPPORTED) && (RGX_NUM_DRIVERS_SUPPORTED > 1)
	/* Firmware heaps on VZ are usually LMA, and several 64kb allocations need to
	 * be made on Guest drivers: FwGuardPage, FwSysInit, FwConnectionCtl, FwOsInit.
	 * Increase the maximum number to avoid driver warnings. */
	IMG_UINT32	ui32MaxLog2NumPages = 7;	/* 128 pages => 512KB */
#else
	IMG_UINT32	ui32MaxLog2NumPages = 4;	/*  16 pages =>  64KB */
#endif
	ui32MaxLog2NumPages = MAX(ui32MaxLog2NumPages, OSGetOrder(1 << psDevNode->ui32Non4KPageSizeLog2));
#else /* defined(DEBUG) */
	PVR_UNREFERENCED_PARAMETER(psDevNode);
#endif /* defined(DEBUG) */

	PVR_ASSERT(uiSize != 0);
	ui32Log2NumPages = OSGetOrder(uiSize);
	uiSize = IMG_PAGES2BYTES64(OSGetPageSize(),ui32Log2NumPages);

	eError = RA_Alloc(pArena,
	                  uiSize,
	                  RA_NO_IMPORT_MULTIPLIER,
	                  0,                         /* No flags */
	                  uiSize,
	                  "RAMemPhyContigPagesAlloc",
	                  &uiCardAddr,
	                  &uiActualSize,
	                  NULL);                     /* No private handle */

	if (eError != PVRSRV_OK)
	{
		RA_USAGE_STATS sRAStats;
		RA_Get_Usage_Stats(pArena, &sRAStats);

		PVR_DPF((PVR_DBG_ERROR,
				"Failed to Allocate size = 0x"IMG_SIZE_FMTSPECX", align = 0x"
				IMG_SIZE_FMTSPECX" Arena Free Space 0x%"IMG_UINT64_FMTSPECX,
				uiSize, uiSize,	sRAStats.ui64FreeArenaSize));
		return eError;
	}

	PVR_ASSERT(uiSize == uiActualSize);

	psMemHandle->u.ui64Handle = uiCardAddr;
	psDevPAddr->uiAddr = (IMG_UINT64) uiCardAddr;

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStatAndTrack(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_LMA,
	                                    uiSize,
	                                    uiCardAddr,
	                                    uiPid);
#else
	{
		IMG_CPU_PHYADDR sCpuPAddr;
		sCpuPAddr.uiAddr = psDevPAddr->uiAddr;

		PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_LMA,
									 NULL,
									 sCpuPAddr,
									 uiSize,
									 uiPid
									 DEBUG_MEMSTATS_VALUES);
	}
#endif
#else /* PVRSRV_ENABLE_PROCESS_STATS */
	PVR_UNREFERENCED_PARAMETER(uiPid);
#endif /* PVRSRV_ENABLE_PROCESS_STATS */
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	PVR_DPF((PVR_DBG_MESSAGE,
	        "%s: (GPU Virtualisation) Allocated 0x" IMG_SIZE_FMTSPECX " at 0x%"
	        IMG_UINT64_FMTSPECX ", Arena ID %u",
	        __func__, uiSize, psDevPAddr->uiAddr, psMemHandle->uiOSid));
#endif

#if defined(DEBUG)
	if (ui32Log2NumPages > ui32MaxLog2NumPages && ui32Log2NumPages > ui32MaxLog2NumPagesHistory)
	{
		PVR_ASSERT((ui32Log2NumPages <= ui32MaxLog2NumPages));
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: ui32MaxLog2NumPages = %u, increasing to %u", __func__,
		        ui32MaxLog2NumPages, ui32Log2NumPages ));
		ui32MaxLog2NumPagesHistory = ui32Log2NumPages;
	}
#endif /* defined(DEBUG) */
	psMemHandle->uiOrder = ui32Log2NumPages;

	return eError;
}

void
RAMemDoPhyContigPagesFree(RA_ARENA *pArena,
                          PG_HANDLE *psMemHandle)
{
	RA_BASE_T uiCardAddr = (RA_BASE_T) psMemHandle->u.ui64Handle;

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsDecrMemAllocStatAndUntrack(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_LMA,
	                                      (IMG_UINT64)uiCardAddr);
#else
	PVRSRVStatsRemoveMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_PAGES_PT_LMA,
									(IMG_UINT64)uiCardAddr,
									OSGetCurrentClientProcessIDKM());
#endif
#endif

	RA_Free(pArena, uiCardAddr);
	psMemHandle->uiOrder = 0;
}

PVRSRV_ERROR
RAMemPhyContigPagesMap(PHYS_HEAP *psPhysHeap,
                       PG_HANDLE *psMemHandle,
                       size_t uiSize, IMG_DEV_PHYADDR *psDevPAddr,
                       void **pvPtr)
{
	IMG_CPU_PHYADDR sCpuPAddr;
	IMG_UINT32 ui32NumPages = (1 << psMemHandle->uiOrder);
	PVR_UNREFERENCED_PARAMETER(uiSize);

	PhysHeapDevPAddrToCpuPAddr(psPhysHeap, 1, &sCpuPAddr, psDevPAddr);
	*pvPtr = OSMapPhysToLin(sCpuPAddr,
							ui32NumPages * OSGetPageSize(),
							PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC);
	PVR_RETURN_IF_NOMEM(*pvPtr);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_IOREMAP_PT_LMA,
	                            ui32NumPages * OSGetPageSize(),
	                            OSGetCurrentClientProcessIDKM());
#else
	{
		PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_IOREMAP_PT_LMA,
									 *pvPtr,
									 sCpuPAddr,
									 ui32NumPages * OSGetPageSize(),
									 OSGetCurrentClientProcessIDKM()
									 DEBUG_MEMSTATS_VALUES);
	}
#endif
#endif
	return PVRSRV_OK;
}

void
RAMemPhyContigPagesUnmap(PHYS_HEAP *psPhysHeap,
                         PG_HANDLE *psMemHandle,
                         void *pvPtr)
{
	IMG_UINT32 ui32NumPages = (1 << psMemHandle->uiOrder);
	PVR_UNREFERENCED_PARAMETER(psPhysHeap);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_IOREMAP_PT_LMA,
		                            ui32NumPages * OSGetPageSize(),
		                            OSGetCurrentClientProcessIDKM());
#else
	PVRSRVStatsRemoveMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_IOREMAP_PT_LMA,
	                                (IMG_UINT64)(uintptr_t)pvPtr,
	                                OSGetCurrentClientProcessIDKM());
#endif
#endif

	OSUnMapPhysToLin(pvPtr, ui32NumPages * OSGetPageSize());
}

PVRSRV_ERROR
RAMemPhyContigPagesClean(PHYS_HEAP *psPhysHeap,
                         PG_HANDLE *psMemHandle,
                         IMG_UINT32 uiOffset,
                         IMG_UINT32 uiLength)
{
	/* No need to flush because we map as uncached */
	PVR_UNREFERENCED_PARAMETER(psPhysHeap);
	PVR_UNREFERENCED_PARAMETER(psMemHandle);
	PVR_UNREFERENCED_PARAMETER(uiOffset);
	PVR_UNREFERENCED_PARAMETER(uiLength);

	return PVRSRV_OK;
}

/* Local memory allocation routines */

/* Assert that the conversions between the RA base type and the device
 * physical address are safe.
 */
static_assert(sizeof(IMG_DEV_PHYADDR) == sizeof(RA_BASE_T),
              "Size IMG_DEV_PHYADDR != RA_BASE_T");

/* Since 0x0 is a valid DevPAddr, we rely on max 64-bit value to be an invalid
 * page address */
#define INVALID_PAGE_ADDR ~((IMG_UINT64)0x0)
#define ZERO_PAGE_VALUE 0

typedef struct _PMR_KERNEL_MAP_HANDLE_ {
	void *vma;
	void *pvKernelAddress;
	/* uiSize has 2 uses:
	 * In Physically contiguous case it is used to track size of the mapping
	 * for free.
	 * In Physically sparse case it is used to determine free path to use, single page
	 * sparse mapping or multi page
	 */
	size_t uiSize;
} PMR_KERNEL_MAPPING;

typedef struct _PMR_LMALLOCARRAY_DATA_ {

#define FLAG_ZERO              (0U)
#define FLAG_POISON_ON_FREE    (1U)
#define FLAG_POISON_ON_ALLOC   (2U)
#define FLAG_ONDEMAND          (3U)
#define FLAG_SPARSE            (4U)
#define FLAG_PHYS_CONTIG       (5U)
#define FLAG_ZOMBIE            (6U)

	IMG_PID uiPid;

	/*
	 * N.B Chunks referenced in this struct commonly are
	 * to OS page sized. But in reality it is dependent on
	 * the uiLog2ChunkSize.
	 * Chunks will always be one 1 << uiLog2ChunkSize in size.
	 * */

	/*
	 * The number of chunks currently allocated in the PMR.
	 */
	IMG_INT32 iNumChunksAllocated;

	/*
	 * Total number of (Virtual) chunks supported by this PMR.
	 */
	IMG_UINT32 uiTotalNumChunks;

	/* The number of chunks to next be allocated for the PMR.
	 * This will initially be the number allocated at first alloc
	 * but may be changed in later calls to change sparse.
	 * It represents the number of chunks to next be allocated.
	 * This is used to store this value because we have the ability to
	 * defer allocation.
	 */
	IMG_UINT32 uiChunksToAlloc;

	/*
	 * Log2 representation of the chunksize.
	 */
	IMG_UINT32 uiLog2ChunkSize;

	/* Physical heap and arena pointers for this allocation */
	PHYS_HEAP* psPhysHeap;
	RA_ARENA* psArena;
	PVRSRV_MEMALLOCFLAGS_T uiAllocFlags;

	/*
	   Connection data for this requests' originating process. NULL for
	   direct-bridge originating calls
	 */
	CONNECTION_DATA *psConnection;

	/*
	 * Allocation flags related to the pages:
	 * Zero              - Should we Zero memory on alloc
	 * Poison on free    - Should we Poison the memory on free.
	 * Poison on alloc   - Should we Poison the memory on alloc.
	 * On demand         - Is the allocation on Demand i.e Do we defer allocation to time of use.
	 * Sparse            - Is the PMR sparse.
	 * Phys Contig       - Is the alloc Physically contiguous
	 * Zombie            - Is zombie
	 * */
	IMG_UINT32 ui32Flags;

	RA_BASE_ARRAY_T aBaseArray; /* Array of RA Bases */

} PMR_LMALLOCARRAY_DATA;

static PVRSRV_ERROR
_FreeLMPages(PMR_LMALLOCARRAY_DATA *psPageArrayData,
             IMG_UINT32 *pui32FreeIndices,
             IMG_UINT32 ui32FreeChunkCount);

static PVRSRV_ERROR _MapPhysicalContigAlloc(PHYS_HEAP *psPhysHeap,
                                            RA_BASE_ARRAY_T paBaseArray,
                                            size_t uiSize,
                                            PMR_FLAGS_T ulFlags,
                                            PMR_KERNEL_MAPPING *psMapping)
{
	IMG_UINT32 ui32CPUCacheFlags;
	PVRSRV_ERROR eError;
	IMG_CPU_PHYADDR sCpuPAddr;
	IMG_DEV_PHYADDR sDevPAddr;
	sDevPAddr.uiAddr = RA_BASE_STRIP_GHOST_BIT(*paBaseArray);

	eError = DevmemCPUCacheMode(ulFlags, &ui32CPUCacheFlags);
	PVR_RETURN_IF_ERROR(eError);

	PhysHeapDevPAddrToCpuPAddr(psPhysHeap,
	                           1,
	                           &sCpuPAddr,
	                           &sDevPAddr);

	psMapping->pvKernelAddress = OSMapPhysToLin(sCpuPAddr, uiSize, ui32CPUCacheFlags);
	PVR_LOG_RETURN_IF_FALSE(psMapping->pvKernelAddress,
	                        "OSMapPhyToLin: out of VM Mem",
	                        PVRSRV_ERROR_PMR_NO_KERNEL_MAPPING);
	psMapping->vma = NULL;
	psMapping->uiSize = uiSize;

	return PVRSRV_OK;
}

static PVRSRV_ERROR _MapPhysicalSparseAlloc(PMR_LMALLOCARRAY_DATA *psLMAllocArrayData,
                                            RA_BASE_ARRAY_T paBaseArray,
                                            size_t uiSize,
                                            PMR_KERNEL_MAPPING *psMapping)
{
	IMG_UINT32 uiChunkCount = uiSize >> psLMAllocArrayData->uiLog2ChunkSize;
	IMG_CPU_PHYADDR uiPages[PMR_MAX_TRANSLATION_STACK_ALLOC], *puiPages;
	PVRSRV_ERROR eError;
	size_t uiPageShift = OSGetPageShift();
	IMG_UINT32 uiOSPagesPerChunkShift = psLMAllocArrayData->uiLog2ChunkSize - uiPageShift;
	IMG_UINT32 uiOSPageCount = uiChunkCount << uiOSPagesPerChunkShift;

	if (uiOSPageCount > PMR_MAX_TRANSLATION_STACK_ALLOC)
	{
		puiPages = OSAllocZMem(sizeof(IMG_CPU_PHYADDR) * uiOSPageCount);
		PVR_RETURN_IF_NOMEM(puiPages);
	}
	else
	{
		puiPages = &uiPages[0];
	}

	if (uiOSPagesPerChunkShift == 0)
	{
		IMG_UINT32 i;
		PhysHeapDevPAddrToCpuPAddr(psLMAllocArrayData->psPhysHeap,
								   uiChunkCount,
								   puiPages,
								   (IMG_DEV_PHYADDR *)paBaseArray);

		/* If the ghost bit is present then the addrs returned will be off by 1
		 * Strip the ghost bit to correct to real page aligned addresses.
		 * */
		for (i = 0; i < uiChunkCount; i++)
		{
			puiPages[i].uiAddr = RA_BASE_STRIP_GHOST_BIT(puiPages[i].uiAddr);
		}
	}
	else
	{
		IMG_UINT32 i = 0, j = 0, index = 0;
		for (i = 0; i < uiChunkCount; i++)
		{
			IMG_UINT32 ui32OSPagesPerDeviceChunk = (1 << uiOSPagesPerChunkShift);
			IMG_DEV_PHYADDR uiDevAddr;
			uiDevAddr.uiAddr = RA_BASE_STRIP_GHOST_BIT(paBaseArray[i]);
			for (j = 0; j < ui32OSPagesPerDeviceChunk; j++)
			{
				PhysHeapDevPAddrToCpuPAddr(psLMAllocArrayData->psPhysHeap,
										   1,
										   &puiPages[index],
										   &uiDevAddr);
				uiDevAddr.uiAddr += (1ULL << uiPageShift);
				index++;
			}
		}
	}

	eError = OSMapPhysArrayToLin(puiPages,
	                             uiOSPageCount,
	                             &psMapping->pvKernelAddress,
	                             &psMapping->vma);
	if (eError == PVRSRV_OK)
	{
		psMapping->uiSize = uiSize;
	}

	if (puiPages != &uiPages[0])
	{
		OSFreeMem(puiPages);
	}

	return eError;
}

static PVRSRV_ERROR _MapPMRKernel(PMR_LMALLOCARRAY_DATA *psLMAllocArrayData,
                                  RA_BASE_ARRAY_T paBaseArray,
                                  size_t uiSize,
                                  PMR_FLAGS_T ulFlags,
                                  PMR_KERNEL_MAPPING *psMapping)
{
	PVRSRV_ERROR eError;
	PHYS_HEAP *psPhysHeap = psLMAllocArrayData->psPhysHeap;
	if (!BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_SPARSE))
	{
		/* Physically Contig */
		if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_PHYS_CONTIG))
		{
			eError = _MapPhysicalContigAlloc(psPhysHeap,
			                                 paBaseArray,
			                                 uiSize,
			                                 ulFlags,
			                                 psMapping);
		}
		/* Physically Sparse */
		else
		{
			eError = _MapPhysicalSparseAlloc(psLMAllocArrayData,
			                                 paBaseArray,
			                                 uiSize,
			                                 psMapping);
		}
	}
	else
	{
		/* Sparse Alloc Single Chunk */
		if (uiSize == IMG_PAGE2BYTES64(psLMAllocArrayData->uiLog2ChunkSize))
		{
			eError = _MapPhysicalContigAlloc(psPhysHeap,
			                                 paBaseArray,
			                                 uiSize,
			                                 ulFlags,
			                                 psMapping);
		}
		/* Sparse Alloc Multi Chunk */
		else
		{
			eError = _MapPhysicalSparseAlloc(psLMAllocArrayData,
			                                 paBaseArray,
			                                 uiSize,
			                                 psMapping);
		}
	}

	return eError;
}

static void _UnMapPhysicalContigAlloc(PMR_KERNEL_MAPPING *psKernelMapping)
{
	OSUnMapPhysToLin(psKernelMapping->pvKernelAddress, psKernelMapping->uiSize);
}

static void _UnMapPhysicalSparseAlloc(PMR_KERNEL_MAPPING *psKernelMapping)
{
	OSUnMapPhysArrayToLin(psKernelMapping->pvKernelAddress,
	                   psKernelMapping->vma);
}

static void _UnMapPMRKernel(PMR_LMALLOCARRAY_DATA *psLMAllocArrayData,
                            PMR_KERNEL_MAPPING *psKernelMapping)
{
	if (!BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_SPARSE))
	{
		/* Physically Contig */
		if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_PHYS_CONTIG))
		{
			_UnMapPhysicalContigAlloc(psKernelMapping);
		}
		/* Physically Sparse */
		else
		{
			_UnMapPhysicalSparseAlloc(psKernelMapping);
		}
	}
	else
	{
		/* Sparse Alloc Single Chunk */
		if (psKernelMapping->uiSize == IMG_PAGE2BYTES64(psLMAllocArrayData->uiLog2ChunkSize))
		{
			_UnMapPhysicalContigAlloc(psKernelMapping);
		}
		/* Sparse Alloc Multi Chunk */
		else
		{
			_UnMapPhysicalSparseAlloc(psKernelMapping);
		}
	}
}

static PVRSRV_ERROR
_PhysPgMemSet(PMR_LMALLOCARRAY_DATA *psLMAllocArrayData,
              RA_BASE_ARRAY_T paBaseArray,
              size_t uiSize,
              IMG_BYTE ui8SetValue)
{
	PVRSRV_ERROR eError;
	PMR_KERNEL_MAPPING sKernelMapping;

	eError = _MapPMRKernel(psLMAllocArrayData,
	                       paBaseArray,
	                       uiSize,
	                       PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC,
	                       &sKernelMapping);
	PVR_GOTO_IF_ERROR(eError, map_failed);

	OSCachedMemSetWMB(sKernelMapping.pvKernelAddress, ui8SetValue, uiSize);

	_UnMapPMRKernel(psLMAllocArrayData, &sKernelMapping);

	return PVRSRV_OK;

map_failed:
	PVR_DPF((PVR_DBG_ERROR, "Failed to poison/zero allocation"));
	return eError;
}

static PVRSRV_ERROR
_AllocLMPageArray(PMR_SIZE_T uiSize,
                  IMG_UINT32 ui32NumPhysChunks,
                  IMG_UINT32 uiLog2AllocPageSize,
                  IMG_UINT32 ui32Flags,
                  PHYS_HEAP* psPhysHeap,
                  RA_ARENA* pArena,
                  PVRSRV_MEMALLOCFLAGS_T uiAllocFlags,
                  IMG_PID uiPid,
                  PMR_LMALLOCARRAY_DATA **ppsPageArrayDataPtr,
                  CONNECTION_DATA *psConnection)
{
	PMR_LMALLOCARRAY_DATA *psPageArrayData = NULL;
	PVRSRV_ERROR eError;
	IMG_UINT32 uiNumPages;

	PVR_ASSERT(!BIT_ISSET(ui32Flags, FLAG_ZERO) || !BIT_ISSET(ui32Flags, FLAG_POISON_ON_ALLOC));
	PVR_ASSERT(OSGetPageShift() <= uiLog2AllocPageSize);

	/* Use of cast below is justified by the assertion that follows to
	prove that no significant bits have been truncated */
	uiNumPages = (IMG_UINT32)(((uiSize - 1) >> uiLog2AllocPageSize) + 1);
	PVR_ASSERT(((PMR_SIZE_T)uiNumPages << uiLog2AllocPageSize) == uiSize);

	psPageArrayData = OSAllocMem(sizeof(PMR_LMALLOCARRAY_DATA) + IMG_FLEX_ARRAY_SIZE(sizeof(RA_BASE_T), uiNumPages));
	PVR_GOTO_IF_NOMEM(psPageArrayData, eError, errorOnAllocArray);

	if (BIT_ISSET(ui32Flags, FLAG_SPARSE))
	{
		/* Since no pages are allocated yet, initialise page addresses to INVALID_PAGE_ADDR */
		OSCachedMemSet(psPageArrayData->aBaseArray,
					   0xFF,
					   sizeof(RA_BASE_T) *
					   uiNumPages);
	}
	else
	{
		/* Base pointers have been allocated for the full PMR in case we require a non
		 * physically contiguous backing for the virtually contiguous allocation but the most
		 * common case will be contiguous and so only require the first Base to be present
		 */
		psPageArrayData->aBaseArray[0] = INVALID_BASE_ADDR;
	}

	psPageArrayData->uiTotalNumChunks = uiNumPages;
	psPageArrayData->uiChunksToAlloc = BIT_ISSET(ui32Flags, FLAG_SPARSE) ? ui32NumPhysChunks : uiNumPages;
	psPageArrayData->uiLog2ChunkSize = uiLog2AllocPageSize;

	psPageArrayData->psConnection = psConnection;
	psPageArrayData->uiPid = uiPid;
	psPageArrayData->iNumChunksAllocated = 0;
	psPageArrayData->ui32Flags = ui32Flags;
	psPageArrayData->psPhysHeap = psPhysHeap;
	psPageArrayData->psArena = pArena;
	psPageArrayData->uiAllocFlags = uiAllocFlags;

	*ppsPageArrayDataPtr = psPageArrayData;

	return PVRSRV_OK;

/*
  error exit path follows:
*/

errorOnAllocArray:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static PVRSRV_ERROR
_AllocLMPagesContig(PMR_LMALLOCARRAY_DATA *psPageArrayData)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 uiLog2ChunkSize = psPageArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64PhysSize = IMG_PAGES2BYTES64(psPageArrayData->uiChunksToAlloc, uiLog2ChunkSize);
	IMG_BOOL bPhysContig;
	IMG_UINT32 ui32Flags = psPageArrayData->ui32Flags;


	eError = RA_AllocMulti(psPageArrayData->psArena,
	                  ui64PhysSize,
	                  uiLog2ChunkSize,
	                  RA_NO_IMPORT_MULTIPLIER,
	                  0,                       /* No flags */
	                  "LMA_Page_Alloc",
	                  psPageArrayData->aBaseArray,
	                  psPageArrayData->uiTotalNumChunks,
	                  &bPhysContig);

	if (PVRSRV_OK != eError)
	{
		RA_USAGE_STATS sRAStats;
		RA_Get_Usage_Stats(psPageArrayData->psArena, &sRAStats);

		PVR_DPF((PHYSHEAP_DPF_LVL,
				"Contig: Failed to Allocate size = 0x%llx, align = 0x%llx"
				" Arena Free Space 0x%"IMG_UINT64_FMTSPECX""
				" Arena Name: '%s'",
				(unsigned long long)ui64PhysSize,
				1ULL << uiLog2ChunkSize,
				sRAStats.ui64FreeArenaSize,
				RA_GetArenaName(psPageArrayData->psArena)));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES, errorOnRAAlloc);
	}

	if (bPhysContig)
	{
		BIT_SET(psPageArrayData->ui32Flags, FLAG_PHYS_CONTIG);
	}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
{
	PVR_DPF((PVR_DBG_MESSAGE,
			"(GPU Virtualization Validation): First RealBase: %"IMG_UINT64_FMTSPECX,
			psPageArrayData->aBaseArray[0]));
}
#endif

	if (BIT_ISSET(ui32Flags, FLAG_POISON_ON_ALLOC))
	{
		eError = _PhysPgMemSet(psPageArrayData,
		                       psPageArrayData->aBaseArray,
		                       ui64PhysSize,
		                       PVRSRV_POISON_ON_ALLOC_VALUE);
		PVR_LOG_GOTO_IF_ERROR(eError, "_PhysPgMemSet", errorOnPoison);
	}

	if (BIT_ISSET(ui32Flags, FLAG_ZERO))
	{
		eError = _PhysPgMemSet(psPageArrayData,
		                       psPageArrayData->aBaseArray,
		                       ui64PhysSize,
		                       ZERO_PAGE_VALUE);
		PVR_LOG_GOTO_IF_ERROR(eError, "_PhysPgMemSet", errorOnZero);
	}

	psPageArrayData->iNumChunksAllocated += psPageArrayData->uiChunksToAlloc;

	/* We have alloc'd the previous request, set 0 for book keeping */
	psPageArrayData->uiChunksToAlloc = 0;


#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES, ui64PhysSize, psPageArrayData->uiPid);
#else
	if (bPhysContig)
	{
		IMG_CPU_PHYADDR sLocalCpuPAddr;
		sLocalCpuPAddr.uiAddr = (IMG_UINT64) psPageArrayData->aBaseArray[0];
		PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,
								 NULL,
								 sLocalCpuPAddr,
								 IMG_PAGES2BYTES64(psPageArrayData->uiTotalNumChunks, uiLog2ChunkSize),
								 psPageArrayData->uiPid
								 DEBUG_MEMSTATS_VALUES);
	}
	else
	{
		IMG_UINT32 i, j;
		IMG_CPU_PHYADDR sLocalCpuPAddr;

		for (i = 0; i < psPageArrayData->uiTotalNumChunks;)
		{
			IMG_UINT32 ui32AllocSizeInChunks = 1;

			for (j = i;
			     j + 1 != psPageArrayData->uiTotalNumChunks &&
			     RA_BASE_IS_GHOST(psPageArrayData->aBaseArray[j + 1]);
			     j++)
			{
				ui32AllocSizeInChunks++;
			}

			sLocalCpuPAddr.uiAddr = (IMG_UINT64) psPageArrayData->aBaseArray[i];
			PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,
									 NULL,
									 sLocalCpuPAddr,
									 IMG_PAGES2BYTES64(ui32AllocSizeInChunks, uiLog2ChunkSize),
									 psPageArrayData->uiPid
									 DEBUG_MEMSTATS_VALUES);

			i += ui32AllocSizeInChunks;
		}
	}
#endif
#endif

	return PVRSRV_OK;

	/*
	  error exit paths follow:
	*/
errorOnZero:
errorOnPoison:
	eError = PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES;

	RA_FreeMulti(psPageArrayData->psArena,
	              psPageArrayData->aBaseArray,
	              psPageArrayData->uiTotalNumChunks);

errorOnRAAlloc:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

/*
 * Fully allocated variant of sparse allocation does not take in as argument an
 * array of indices. It is used in cases where the amount of chunks to allocate is
 * the same as the total the PMR can represent. I.E when we want to fully populate
 * a sparse PMR.
 */
static PVRSRV_ERROR
_AllocLMPagesSparseFull(PMR_LMALLOCARRAY_DATA *psPageArrayData)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 uiLog2ChunkSize = psPageArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64PhysSize = IMG_PAGES2BYTES64(psPageArrayData->uiChunksToAlloc, uiLog2ChunkSize);
	IMG_UINT32 ui32Flags = psPageArrayData->ui32Flags;


	eError = RA_AllocMultiSparse(psPageArrayData->psArena,
	                  uiLog2ChunkSize,
	                  RA_NO_IMPORT_MULTIPLIER,
	                  0,                       /* No flags */
	                  "LMA_Page_Alloc",
	                  psPageArrayData->aBaseArray,
	                  psPageArrayData->uiTotalNumChunks,
	                  NULL, /* No indices given meaning allocate full base array using chunk count below */
	                  psPageArrayData->uiChunksToAlloc);
	if (PVRSRV_OK != eError)
	{
		RA_USAGE_STATS sRAStats;
		RA_Get_Usage_Stats(psPageArrayData->psArena, &sRAStats);

		PVR_DPF((PHYSHEAP_DPF_LVL,
				"SparseFull: Failed to Allocate size = 0x%llx, align = 0x%llx"
				" Arena Free Space 0x%"IMG_UINT64_FMTSPECX""
				" Arena Name: '%s'",
				(unsigned long long)ui64PhysSize,
				1ULL << uiLog2ChunkSize,
				sRAStats.ui64FreeArenaSize,
				RA_GetArenaName(psPageArrayData->psArena)));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES, errorOnRAAlloc);
	}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
{
	PVR_DPF((PVR_DBG_MESSAGE,
		"(GPU Virtualization Validation): First RealBase: %"IMG_UINT64_FMTSPECX,
		psPageArrayData->aBaseArray[0]));
}
#endif

	if (BIT_ISSET(ui32Flags, FLAG_POISON_ON_ALLOC))
	{
		eError = _PhysPgMemSet(psPageArrayData,
		                       psPageArrayData->aBaseArray,
		                       ui64PhysSize,
		                       PVRSRV_POISON_ON_ALLOC_VALUE);
		PVR_LOG_GOTO_IF_ERROR(eError, "_PhysPgMemSet", errorOnPoison);
	}

	if (BIT_ISSET(ui32Flags, FLAG_ZERO))
	{
		eError = _PhysPgMemSet(psPageArrayData,
		                       psPageArrayData->aBaseArray,
		                       ui64PhysSize,
		                       ZERO_PAGE_VALUE);
		PVR_LOG_GOTO_IF_ERROR(eError, "_PhysPgMemSet", errorOnZero);
	}

	psPageArrayData->iNumChunksAllocated += psPageArrayData->uiChunksToAlloc;

	/* We have alloc'd the previous request, set 0 for book keeping */
	psPageArrayData->uiChunksToAlloc = 0;

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES, ui64PhysSize, psPageArrayData->uiPid);
#else
	{
		IMG_UINT32 i;

		for (i = 0; i < psPageArrayData->uiTotalNumChunks; i++)
		{
			IMG_CPU_PHYADDR sLocalCpuPAddr;
			sLocalCpuPAddr.uiAddr =
				(IMG_UINT64) RA_BASE_STRIP_GHOST_BIT(psPageArrayData->aBaseArray[i]);
			PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,
									 NULL,
									 sLocalCpuPAddr,
									 1 << uiLog2ChunkSize,
									 psPageArrayData->uiPid
									 DEBUG_MEMSTATS_VALUES);
		}
	}
#endif
#endif

	return PVRSRV_OK;

	/*
	  error exit paths follow:
	*/
errorOnZero:
errorOnPoison:
	eError = PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES;

	RA_FreeMulti(psPageArrayData->psArena,
	              psPageArrayData->aBaseArray,
	              psPageArrayData->uiTotalNumChunks);

errorOnRAAlloc:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}

static PVRSRV_ERROR
_AllocLMPagesSparse(PMR_LMALLOCARRAY_DATA *psPageArrayData, IMG_UINT32 *pui32MapTable)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 uiLog2ChunkSize = psPageArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64ChunkSize = IMG_PAGE2BYTES64(uiLog2ChunkSize);
	IMG_UINT32 uiChunksToAlloc = psPageArrayData->uiChunksToAlloc;
	IMG_UINT32 ui32Flags = psPageArrayData->ui32Flags;

	if (!pui32MapTable)
	{
		PVR_LOG_GOTO_WITH_ERROR("pui32MapTable", eError, PVRSRV_ERROR_PMR_INVALID_MAP_INDEX_ARRAY, errorOnRAAlloc);
	}

#if defined(DEBUG)
	/*
	 * This block performs validation of the mapping table input in the following ways:
	 * Check that each index in the mapping table does not exceed the number of the chunks
	 * the whole PMR supports.
	 * Check that each index given by the mapping table is not already allocated.
	 * Check that there are no duplicated indices given in the mapping table.
	 */
	{
		IMG_UINT32 i;
		IMG_BOOL bIssueDetected = IMG_FALSE;
		PVRSRV_ERROR eMapCheckError;

		for (i = 0; i < uiChunksToAlloc; i++)
		{
			if (pui32MapTable[i] >= psPageArrayData->uiTotalNumChunks)
			{
				PVR_DPF((PVR_DBG_ERROR,
						"%s: Page alloc request Index out of bounds for PMR @0x%p",
						__func__,
						psPageArrayData));
				eMapCheckError = PVRSRV_ERROR_DEVICEMEM_OUT_OF_RANGE;
				bIssueDetected = IMG_TRUE;
				break;
			}

			if (!RA_BASE_IS_INVALID(psPageArrayData->aBaseArray[pui32MapTable[i]]))
			{
				PVR_DPF((PVR_DBG_ERROR,
						"%s: Mapping already exists Index %u Mapping index %u",
						__func__,
						i,
						pui32MapTable[i]));
				eMapCheckError = PVRSRV_ERROR_PMR_MAPPING_ALREADY_EXISTS;
				bIssueDetected = IMG_TRUE;
				break;
			}

			if (RA_BASE_IS_SPARSE_PREP(psPageArrayData->aBaseArray[pui32MapTable[i]]))
			{
				PVR_DPF((PVR_DBG_ERROR,
						"%s: Mapping already exists in mapping table given Index %u Mapping index %u",
						__func__,
						i,
						pui32MapTable[i]));
				eMapCheckError = PVRSRV_ERROR_PMR_MAPPING_ALREADY_EXISTS;
				bIssueDetected = IMG_TRUE;
				break;
			}
			else
			{
				/* Set the To Prep value so we can detect duplicated map indices */
				psPageArrayData->aBaseArray[pui32MapTable[i]] = RA_BASE_SPARSE_PREP_ALLOC_ADDR;
			}
		}
		/* Unwind the Alloc Prep Values */
		if (bIssueDetected)
		{
			/* We don't want to affect the index of the issue seen
			 * as it could be a valid mapping. If it is a duplicated
			 * mapping in the given table then we will clean-up the
			 * previous instance anyway.
			 */
			IMG_UINT32 uiUnwind = i;

			for (i = 0; i < uiUnwind; i++)
			{
				psPageArrayData->aBaseArray[pui32MapTable[i]] = INVALID_BASE_ADDR;
			}

			PVR_GOTO_WITH_ERROR(eError, eMapCheckError, errorOnRAAlloc);
		}
	}
#endif

	eError = RA_AllocMultiSparse(psPageArrayData->psArena,
	                              psPageArrayData->uiLog2ChunkSize,
	                              RA_NO_IMPORT_MULTIPLIER,
	                              0,
	                              "LMA_Page_Alloc",
	                              psPageArrayData->aBaseArray,
	                              psPageArrayData->uiTotalNumChunks,
	                              pui32MapTable,
	                              uiChunksToAlloc);
	if (PVRSRV_OK != eError)
	{
		RA_USAGE_STATS sRAStats;
		RA_Get_Usage_Stats(psPageArrayData->psArena, &sRAStats);

		PVR_DPF((PHYSHEAP_DPF_LVL,
				"Sparse: Failed to Allocate size = 0x%llx, align = 0x%llx"
				" Arena Free Space 0x%"IMG_UINT64_FMTSPECX""
				" Arena Name: '%s'",
				(unsigned long long) uiChunksToAlloc << uiLog2ChunkSize,
				1ULL << uiLog2ChunkSize,
				sRAStats.ui64FreeArenaSize,
				RA_GetArenaName(psPageArrayData->psArena)));
		PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES, errorOnRAAlloc);
	}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
{
	PVR_DPF((PVR_DBG_MESSAGE,
	        "(GPU Virtualization Validation): First RealBase: %"IMG_UINT64_FMTSPECX,
	        psPageArrayData->aBaseArray[pui32MapTable[0]]));
}
#endif

	if (BIT_ISSET(ui32Flags, FLAG_POISON_ON_ALLOC) || BIT_ISSET(ui32Flags, FLAG_ZERO))
	{
		IMG_UINT32 i, ui32Index = 0;
		for (i = 0; i < uiChunksToAlloc; i++)
		{
			ui32Index = pui32MapTable[i];

			eError = _PhysPgMemSet(psPageArrayData,
			                       &psPageArrayData->aBaseArray[ui32Index],
			                       ui64ChunkSize,
			                       BIT_ISSET(ui32Flags, FLAG_POISON_ON_ALLOC) ? PVRSRV_POISON_ON_ALLOC_VALUE :
			                                                                    ZERO_PAGE_VALUE);
			PVR_LOG_GOTO_IF_ERROR(eError, "_PhysPgMemSet", errorOnPoisonZero);
		}
	}

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,
	                            uiChunksToAlloc << uiLog2ChunkSize,
	                            psPageArrayData->uiPid);
#else
	{
		IMG_UINT32 i;

		for (i = 0; i < psPageArrayData->uiChunksToAlloc; i++)
		{
			IMG_UINT32 ui32Index = pui32MapTable[i];
			IMG_CPU_PHYADDR sLocalCpuPAddr;
			sLocalCpuPAddr.uiAddr =
				(IMG_UINT64) RA_BASE_STRIP_GHOST_BIT(psPageArrayData->aBaseArray[ui32Index]);
			PVRSRVStatsAddMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,
									 NULL,
									 sLocalCpuPAddr,
									 ui64ChunkSize,
									 psPageArrayData->uiPid
									 DEBUG_MEMSTATS_VALUES);
		}
	}
#endif
#endif

	psPageArrayData->iNumChunksAllocated += uiChunksToAlloc;

	/* We have alloc'd the previous request, set 0 for book keeping */
	psPageArrayData->uiChunksToAlloc = 0;

	return PVRSRV_OK;

	/*
	  error exit paths follow:
	*/
errorOnPoisonZero:
	eError = PVRSRV_ERROR_PMR_FAILED_TO_ALLOC_PAGES;

	RA_FreeMultiSparse(psPageArrayData->psArena,
	                    psPageArrayData->aBaseArray,
	                    psPageArrayData->uiTotalNumChunks,
	                    psPageArrayData->uiLog2ChunkSize,
	                    pui32MapTable,
	                    &uiChunksToAlloc);

errorOnRAAlloc:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;

}

static PVRSRV_ERROR
_AllocLMPages(PMR_LMALLOCARRAY_DATA *psPageArrayData, IMG_UINT32 *pui32MapTable)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(NULL != psPageArrayData);
	PVR_ASSERT(0 <= psPageArrayData->iNumChunksAllocated);

	if (psPageArrayData->uiTotalNumChunks <
			(psPageArrayData->iNumChunksAllocated + psPageArrayData->uiChunksToAlloc))
	{
		PVR_DPF((PVR_DBG_ERROR, "Pages requested to allocate don't fit PMR alloc Size. "
				"Allocated: %u + Requested: %u > Total Allowed: %u",
				psPageArrayData->iNumChunksAllocated,
				psPageArrayData->uiChunksToAlloc,
				psPageArrayData->uiTotalNumChunks));
		return PVRSRV_ERROR_PMR_BAD_MAPPINGTABLE_SIZE;
	}

	/* If we have a non-backed sparse PMR then we can just return */
	if (psPageArrayData->uiChunksToAlloc == 0)
	{
		PVR_DPF((PVR_DBG_MESSAGE,
							"%s: Non-Backed Sparse PMR Created: %p.",
							__func__,
							psPageArrayData));
		return PVRSRV_OK;
	}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
	{
		IMG_UINT32 ui32OSid=0;
		PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPageArrayData->psPhysHeap);

		/* Obtain the OSid specific data from our connection handle */
		if (psPageArrayData->psConnection != NULL)
		{
			ui32OSid = psPageArrayData->psConnection->ui32OSid;
		}

		/* Replace the given RA Arena with OS Arena */
		if (PVRSRV_CHECK_SHARED_BUFFER(psPageArrayData->uiAllocFlags))
		{
			psPageArrayData->psArena = psDevNode->psOSSharedArena;
			PVR_DPF((PVR_DBG_MESSAGE,
					 "(GPU Virtualization Validation): Giving from shared mem"));
		}
		else
		{
			psPageArrayData->psArena = psDevNode->psOSidSubArena[ui32OSid];
			PVR_DPF((PVR_DBG_MESSAGE,
					 "(GPU Virtualization Validation): Giving from OS slot %d",
					 ui32OSid));
		}
	}
#endif

	/*
	 * 3 cases:
	 * Sparse allocation populating the whole PMR.
	 * [**********]
	 * Sparse allocation partially populating the PMR at given indices.
	 * [*** *** **]
	 * Contiguous allocation.
	 * [**********]
	 *
	 * Note: Separate cases are required for 1 and 3 due to memstats tracking.
	 * In Contiguous case we can track the block as a single memstat record as we know
	 * we will also free in that size record.
	 * Sparse allocations require a memstat record per chunk as they can be arbitrarily
	 * free'd.
	 */
	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_SPARSE))
	{
		if (psPageArrayData->uiTotalNumChunks == psPageArrayData->uiChunksToAlloc &&
		    !pui32MapTable)
		{
			eError = _AllocLMPagesSparseFull(psPageArrayData);
		}
		else
		{
			eError = _AllocLMPagesSparse(psPageArrayData, pui32MapTable);
		}
	}
	else
	{
		eError = _AllocLMPagesContig(psPageArrayData);
	}

	return eError;
}

static void
_FreeLMPageArray(PMR_LMALLOCARRAY_DATA *psPageArrayData)
{
	PVR_DPF((PVR_DBG_MESSAGE,
			"physmem_lma.c: freed local memory array structure for PMR @0x%p",
			psPageArrayData));

	OSFreeMem(psPageArrayData);
}

static PVRSRV_ERROR
_FreeLMPagesContig(PMR_LMALLOCARRAY_DATA *psPageArrayData)
{
	RA_ARENA *pArena = psPageArrayData->psArena;
	IMG_UINT64 ui64PhysSize = IMG_PAGES2BYTES64(psPageArrayData->uiTotalNumChunks, psPageArrayData->uiLog2ChunkSize);
	PVRSRV_ERROR eError;
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	IMG_UINT32 uiStat = PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES;
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_ZOMBIE))
	{
		uiStat = PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_LMA_PAGES;
	}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */
#endif /* defined(PVRSRV_ENABLE_PROCESS_STATS) */

	PVR_ASSERT(psPageArrayData->iNumChunksAllocated != 0);
	PVR_ASSERT(psPageArrayData->iNumChunksAllocated ==
	           psPageArrayData->uiTotalNumChunks);

	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_POISON_ON_FREE))
	{
		eError = _PhysPgMemSet(psPageArrayData,
							   psPageArrayData->aBaseArray,
							   ui64PhysSize,
							   PVRSRV_POISON_ON_FREE_VALUE);
		PVR_LOG_IF_ERROR(eError, "_PhysPgMemSet");
	}

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsDecrMemAllocStat(uiStat,
	                            ui64PhysSize,
	                            psPageArrayData->uiPid);
#else
	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_PHYS_CONTIG))
	{
		PVRSRVStatsRemoveMemAllocRecord(uiStat,
		                                (IMG_UINT64) psPageArrayData->aBaseArray[0],
		                                psPageArrayData->uiPid);
	}
	else
	{
		IMG_UINT32 i;

		for (i = 0; i < psPageArrayData->uiTotalNumChunks; i++)
		{
			if (RA_BASE_IS_REAL(psPageArrayData->aBaseArray[i]))
			{
				PVRSRVStatsRemoveMemAllocRecord(uiStat,
				                                (IMG_UINT64) psPageArrayData->aBaseArray[i],
				                                psPageArrayData->uiPid);
			}
		}
	}
#endif
#endif

	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_PHYS_CONTIG))
	{
		eError = RA_FreeMulti(pArena,
							  psPageArrayData->aBaseArray,
							  1);
		PVR_LOG_RETURN_IF_ERROR(eError, "RA_FreeMulti");
	}
	else
	{
		eError = RA_FreeMulti(pArena,
							  psPageArrayData->aBaseArray,
							  psPageArrayData->iNumChunksAllocated);
		PVR_LOG_RETURN_IF_ERROR(eError, "RA_FreeMulti");
	}

	psPageArrayData->iNumChunksAllocated = 0;

	PVR_ASSERT(0 <= psPageArrayData->iNumChunksAllocated);

	PVR_DPF((PVR_DBG_MESSAGE,
			"%s: freed %"IMG_UINT64_FMTSPEC" local memory for PMR @0x%p",
			__func__,
			ui64PhysSize,
			psPageArrayData));

	return eError;
}

static PVRSRV_ERROR
_FreeLMPagesRemainingSparse(PMR_LMALLOCARRAY_DATA *psPageArrayData)
{
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	IMG_UINT64 ui64ChunkSize = IMG_PAGE2BYTES64(psPageArrayData->uiLog2ChunkSize);
	IMG_UINT32 ui32Flags = psPageArrayData->ui32Flags;
	IMG_BOOL bPoisonOnFree = (BIT_ISSET(ui32Flags, FLAG_POISON_ON_FREE));
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	IMG_UINT32 uiStat = PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES;
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_ZOMBIE))
	{
		uiStat = PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_LMA_PAGES;
	}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */
#endif /* defined(PVRSRV_ENABLE_PROCESS_STATS) */

#if defined(PVRSRV_ENABLE_PROCESS_STATS) && !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsDecrMemAllocStat(uiStat,
	                            psPageArrayData->iNumChunksAllocated << psPageArrayData->uiLog2ChunkSize,
	                            psPageArrayData->uiPid);
#endif

	for (i = 0; i < psPageArrayData->uiTotalNumChunks;)
	{
		if (RA_BASE_IS_REAL(psPageArrayData->aBaseArray[i]))
		{
			IMG_UINT32 j;
			IMG_UINT32 ui32AccumulatedChunks = 1;

			for (j = i;
				 j + 1 != psPageArrayData->uiTotalNumChunks &&
				 RA_BASE_IS_GHOST(psPageArrayData->aBaseArray[j + 1]);
				 j++)
			{
				ui32AccumulatedChunks++;
			}

#if defined(PVRSRV_ENABLE_PROCESS_STATS) && defined(PVRSRV_ENABLE_MEMORY_STATS)
			for (j = i; j < (i + ui32AccumulatedChunks); j++)
			{
				PVRSRVStatsRemoveMemAllocRecord(uiStat,
				                                RA_BASE_STRIP_GHOST_BIT(psPageArrayData->aBaseArray[j]),
				                                psPageArrayData->uiPid);
				if (bPoisonOnFree)
#else
			for (j = i; j < (i + ui32AccumulatedChunks) && bPoisonOnFree; j++)
			{
#endif
				{
					eError = _PhysPgMemSet(psPageArrayData,
										   &psPageArrayData->aBaseArray[j],
										   ui64ChunkSize,
										   PVRSRV_POISON_ON_FREE_VALUE);
					PVR_LOG_IF_ERROR(eError, "_PhysPgMemSet");
				}
			}

			eError = RA_FreeMulti(psPageArrayData->psArena,
			                       &psPageArrayData->aBaseArray[i],
			                       ui32AccumulatedChunks);
			PVR_LOG_RETURN_IF_ERROR(eError, "RA_FreeMulti");

			psPageArrayData->iNumChunksAllocated -= ui32AccumulatedChunks;
			i += ui32AccumulatedChunks;
		}
		else if (RA_BASE_IS_INVALID(psPageArrayData->aBaseArray[i]))
		{
			i++;
		}
	}

	/* We have freed all allocations in the previous loop */
	PVR_ASSERT(0 <= psPageArrayData->iNumChunksAllocated);

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_FreeLMPagesSparse(PMR_LMALLOCARRAY_DATA *psPageArrayData,
                   IMG_UINT32 *pui32FreeIndices,
                   IMG_UINT32 ui32FreeChunkCount)
{
	RA_ARENA *pArena = psPageArrayData->psArena;
	IMG_UINT32 uiLog2ChunkSize = psPageArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64ChunkSize = IMG_PAGE2BYTES64(uiLog2ChunkSize);
	IMG_UINT32 ui32Flags = psPageArrayData->ui32Flags;
	IMG_UINT32 uiActualFreeCount = ui32FreeChunkCount;
	PVRSRV_ERROR eError;
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	IMG_UINT32 uiStat = PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES;
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_ZOMBIE))
	{
		uiStat = PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_LMA_PAGES;
	}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */
#endif /* defined(PVRSRV_ENABLE_PROCESS_STATS) */

	PVR_ASSERT(psPageArrayData->iNumChunksAllocated != 0);

#if defined(PVRSRV_ENABLE_PROCESS_STATS) && defined(PVRSRV_ENABLE_MEMORY_STATS)
	{
		IMG_UINT32 i;

		for (i = 0; i < ui32FreeChunkCount; i++)
		{
			IMG_UINT32 ui32Index = pui32FreeIndices[i];

			PVRSRVStatsRemoveMemAllocRecord(uiStat,
			                                (IMG_UINT64) RA_BASE_STRIP_GHOST_BIT(
			                                psPageArrayData->aBaseArray[ui32Index]),
			                                psPageArrayData->uiPid);
		}
	}
#endif

	if (BIT_ISSET(ui32Flags, FLAG_POISON_ON_FREE))
	{
		IMG_UINT32 i, ui32Index = 0;
		for (i = 0; i < ui32FreeChunkCount; i++)
		{
			ui32Index = pui32FreeIndices[i];

			eError = _PhysPgMemSet(psPageArrayData,
								   &psPageArrayData->aBaseArray[ui32Index],
								   ui64ChunkSize,
								   PVRSRV_POISON_ON_FREE_VALUE);
			PVR_LOG_IF_ERROR(eError, "_PhysPgMemSet");
		}
	}

	eError = RA_FreeMultiSparse(pArena,
	                             psPageArrayData->aBaseArray,
	                             psPageArrayData->uiTotalNumChunks,
	                             uiLog2ChunkSize,
	                             pui32FreeIndices,
	                             &uiActualFreeCount);
	psPageArrayData->iNumChunksAllocated -= uiActualFreeCount;
#if defined(PVRSRV_ENABLE_PROCESS_STATS) && !defined(PVRSRV_ENABLE_MEMORY_STATS)
	PVRSRVStatsDecrMemAllocStat(uiStat,
	                            uiActualFreeCount << psPageArrayData->uiLog2ChunkSize,
	                            psPageArrayData->uiPid);
#endif
	if (eError == PVRSRV_ERROR_RA_FREE_INVALID_CHUNK)
	{
		/* Log the RA error but convert it to PMR level to match the interface,
		 * this is important because other PMR factories may not use the RA but
		 * still return error, returning a PMR based error
		 * keeps the interface agnostic to implementation behaviour.
		 */
		PVR_LOG_IF_ERROR(eError, "RA_FreeMultiSparse");
		return PVRSRV_ERROR_PMR_FREE_INVALID_CHUNK;
	}
	PVR_LOG_RETURN_IF_ERROR(eError, "RA_FreeMultiSparse");

	PVR_ASSERT(0 <= psPageArrayData->iNumChunksAllocated);


	PVR_DPF((PVR_DBG_MESSAGE,
			"%s: freed %" IMG_UINT64_FMTSPEC " local memory for PMR @0x%p",
			__func__,
			(uiActualFreeCount * ui64ChunkSize),
			psPageArrayData));

	return PVRSRV_OK;
}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
static PVRSRV_ERROR PMRFreeZombiePagesRAMem(PMR_IMPL_ZOMBIEPAGES pvPriv)
{
	PVRSRV_ERROR eError;
	PMR_LMALLOCARRAY_DATA *psZombiePageArray = pvPriv;

	eError = _FreeLMPages(psZombiePageArray, NULL, 0);
	PVR_GOTO_IF_ERROR(eError, e0);

	_FreeLMPageArray(psZombiePageArray);
	return PVRSRV_OK;
e0:
	return eError;
}

/* Allocates a new PMR_LMALLOCARRAY_DATA object and fills it with
 * pages to be extracted from psSrcPageArrayData.
 */
static PVRSRV_ERROR
_ExtractPages(PMR_LMALLOCARRAY_DATA *psSrcPageArrayData,
			  IMG_UINT32 *pai32ExtractIndices,
			  IMG_UINT32 ui32ExtractPageCount,
			  PMR_LMALLOCARRAY_DATA **psOutPageArrayData)
{
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32ExtractPageCountSaved;
	PMR_LMALLOCARRAY_DATA* psDstPageArrayData;

	/* Alloc PMR_LMALLOCARRAY_DATA for the extracted pages */
	eError = _AllocLMPageArray(ui32ExtractPageCount << psSrcPageArrayData->uiLog2ChunkSize,
	                           ui32ExtractPageCount,
	                           psSrcPageArrayData->uiLog2ChunkSize,
	                           psSrcPageArrayData->ui32Flags,
	                           psSrcPageArrayData->psPhysHeap,
	                           psSrcPageArrayData->psArena,
	                           psSrcPageArrayData->uiAllocFlags,
	                           psSrcPageArrayData->uiPid,
	                           &psDstPageArrayData,
	                           psSrcPageArrayData->psConnection);
	PVR_LOG_GOTO_IF_ERROR(eError, "_AllocLMPageArray", alloc_error);

	ui32ExtractPageCountSaved = ui32ExtractPageCount;
	/* Transfer pages from source base array to newly allocated page array */
	eError = RA_TransferMultiSparseIndices(psSrcPageArrayData->psArena,
	                                       psSrcPageArrayData->aBaseArray,
	                                       psSrcPageArrayData->uiTotalNumChunks,
	                                       psDstPageArrayData->aBaseArray,
	                                       psDstPageArrayData->uiTotalNumChunks,
	                                       psSrcPageArrayData->uiLog2ChunkSize,
	                                       pai32ExtractIndices,
	                                       &ui32ExtractPageCountSaved);
	PVR_LOG_GOTO_IF_FALSE((eError == PVRSRV_OK) && (ui32ExtractPageCountSaved == ui32ExtractPageCount),
	                      "RA_TransferMultiSparseIndices failed",
	                      transfer_error);


	/* Update page counts */
	psSrcPageArrayData->iNumChunksAllocated -= ui32ExtractPageCount;
	psDstPageArrayData->iNumChunksAllocated += ui32ExtractPageCount;

	*psOutPageArrayData = psDstPageArrayData;

	return PVRSRV_OK;
transfer_error:
	_FreeLMPageArray(psDstPageArrayData);
alloc_error:
	return eError;
}

/* Extracts all allocated pages referenced psSrcPageArrayData
 * Allocates a new PMR_OSPAGEARRAY_DATA object and fills it with the extracted
 * pages information.
 */
static PVRSRV_ERROR
_ExtractAllPages(PMR_LMALLOCARRAY_DATA *psSrcPageArrayData,
				 PMR_LMALLOCARRAY_DATA **psOutPageArrayData)
{
	PVRSRV_ERROR eError;
	PMR_LMALLOCARRAY_DATA* psDstPageArrayData;
	IMG_UINT32 ui32IdxSrc, ui32IdxDst;

	if (psSrcPageArrayData->iNumChunksAllocated == 0)
	{
		/* Do nothing if psSrcPageArrayData contains no allocated pages */
		return PVRSRV_OK;
	}

	/* Alloc PMR_LMALLOCARRAY_DATA for the extracted pages */
	eError = _AllocLMPageArray(psSrcPageArrayData->iNumChunksAllocated << psSrcPageArrayData->uiLog2ChunkSize,
	                           psSrcPageArrayData->iNumChunksAllocated,
	                           psSrcPageArrayData->uiLog2ChunkSize,
	                           psSrcPageArrayData->ui32Flags,
	                           psSrcPageArrayData->psPhysHeap,
	                           psSrcPageArrayData->psArena,
	                           psSrcPageArrayData->uiAllocFlags,
	                           psSrcPageArrayData->uiPid,
	                           &psDstPageArrayData,
	                           psSrcPageArrayData->psConnection);
	PVR_LOG_RETURN_IF_ERROR_VA(eError, "_AllocLMPageArray failed in %s", __func__);

	/* Now do the transfer */
	ui32IdxDst=0;
	for (ui32IdxSrc=0; ((ui32IdxDst<psSrcPageArrayData->iNumChunksAllocated) &&
	                    (psDstPageArrayData->iNumChunksAllocated<psSrcPageArrayData->iNumChunksAllocated)); ui32IdxSrc++)
	{
		if (psSrcPageArrayData->aBaseArray[ui32IdxSrc] != INVALID_BASE_ADDR)
		{
			psDstPageArrayData->aBaseArray[ui32IdxDst++] = psSrcPageArrayData->aBaseArray[ui32IdxSrc];
			psSrcPageArrayData->aBaseArray[ui32IdxSrc] = INVALID_BASE_ADDR;
			psDstPageArrayData->iNumChunksAllocated++;
		}
	}

	/* Update src page count */
	psSrcPageArrayData->iNumChunksAllocated = 0;

	*psOutPageArrayData = psDstPageArrayData;

	return PVRSRV_OK;
}
#endif /* defined(SUPPORT_PMR_PAGES_DEFERRED_FREE) */

static PVRSRV_ERROR
_FreeLMPages(PMR_LMALLOCARRAY_DATA *psPageArrayData,
             IMG_UINT32 *pui32FreeIndices,
             IMG_UINT32 ui32FreeChunkCount)
{
	PVRSRV_ERROR eError;

	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_SPARSE))
	{
		if (!pui32FreeIndices)
		{
			eError =  _FreeLMPagesRemainingSparse(psPageArrayData);
		}
		else
		{
			eError = _FreeLMPagesSparse(psPageArrayData, pui32FreeIndices, ui32FreeChunkCount);
		}
	}
	else
	{
		eError = _FreeLMPagesContig(psPageArrayData);
	}

	return eError;
}

/*
 *
 * Implementation of PMR callback functions
 *
 */

/* destructor func is called after last reference disappears, but
   before PMR itself is freed. */
static void
PMRFinalizeLocalMem(PMR_IMPL_PRIVDATA pvPriv)
{
	PVRSRV_ERROR eError;
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData = pvPriv;

	/* We can't free pages until now. */
	if (psLMAllocArrayData->iNumChunksAllocated != 0)
	{
		eError = _FreeLMPages(psLMAllocArrayData, NULL, 0);
		PVR_LOG_IF_ERROR(eError, "_FreeLMPages");
		PVR_ASSERT (eError == PVRSRV_OK);
	}

	_FreeLMPageArray(psLMAllocArrayData);
}

#if defined(SUPPORT_PMR_DEFERRED_FREE)
#if defined(PVRSRV_ENABLE_MEMORY_STATS)
static inline void _TransferToMemZombieRecord_LmaPages(PMR_LMALLOCARRAY_DATA *psPageArrayData,
                                                       RA_BASE_T uiBase)
{
	IMG_CPU_PHYADDR sCPUPhysAddr = {
		.uiAddr = RA_BASE_STRIP_GHOST_BIT(uiBase)
	};

	PVRSRVStatsTransferMemAllocRecord(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES,
	                                  PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_LMA_PAGES,
	                                  sCPUPhysAddr.uiAddr,
	                                  psPageArrayData->uiPid
	                                  DEBUG_MEMSTATS_VALUES);
}
#endif

static PVRSRV_ERROR PMRZombifyLocalMem(PMR_IMPL_PRIVDATA pvPriv, PMR *psPMR)
{
	PMR_LMALLOCARRAY_DATA *psPageArrayData = pvPriv;

	BIT_SET(psPageArrayData->ui32Flags, FLAG_ZOMBIE);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#if !defined(PVRSRV_ENABLE_MEMORY_STATS)
	{
		IMG_PID uiPid = psPageArrayData->uiPid;
		IMG_UINT32 uiLog2ChunkSize = psPageArrayData->uiLog2ChunkSize;
		IMG_UINT64 uiSize = BIT_ISSET(psPageArrayData->ui32Flags, FLAG_SPARSE) ?
			(IMG_UINT64) psPageArrayData->iNumChunksAllocated << uiLog2ChunkSize :
			(IMG_UINT64) psPageArrayData->uiTotalNumChunks << uiLog2ChunkSize;

		PVRSRVStatsDecrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_ALLOC_LMA_PAGES, uiSize, uiPid);
		PVRSRVStatsIncrMemAllocStat(PVRSRV_MEM_ALLOC_TYPE_ZOMBIE_LMA_PAGES, uiSize, uiPid);
	}
#else /* !defined(PVRSRV_ENABLE_MEMORY_STATS) */
	if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_SPARSE))
	{
		/* _FreeLMPagesRemainingSparse path */

		IMG_UINT32 i;

		for (i = 0; i < psPageArrayData->uiTotalNumChunks; i++)
		{
			if (RA_BASE_IS_REAL(psPageArrayData->aBaseArray[i]))
			{
				IMG_UINT32 j;
				IMG_UINT32 ui32AccumulatedChunks = 1;

				for (j = i;
				     j + 1 != psPageArrayData->uiTotalNumChunks &&
				         RA_BASE_IS_GHOST(psPageArrayData->aBaseArray[j + 1]);
				     j++)
				{
					ui32AccumulatedChunks++;
				}

				for (j = i; j < (i + ui32AccumulatedChunks); j++)
				{
					_TransferToMemZombieRecord_LmaPages(psPageArrayData,
					                                    psPageArrayData->aBaseArray[j]);
				}
			}
		}
	}
	else
	{
		/* _FreeLMPagesContig path */

		if (BIT_ISSET(psPageArrayData->ui32Flags, FLAG_PHYS_CONTIG))
		{
			_TransferToMemZombieRecord_LmaPages(psPageArrayData,
			                                    psPageArrayData->aBaseArray[0]);
		}
		else
		{
			IMG_UINT32 i;

			for (i = 0; i < psPageArrayData->uiTotalNumChunks; i++)
			{
				if (RA_BASE_IS_REAL(psPageArrayData->aBaseArray[i]))
				{
					_TransferToMemZombieRecord_LmaPages(psPageArrayData,
					                                    psPageArrayData->aBaseArray[i]);
				}
			}

		}
	}
#endif /* !defined(PVRSRV_ENABLE_MEMORY_STATS) */
#endif /* defined(PVRSRV_ENABLE_PROCESS_STATS) */

	PVR_UNREFERENCED_PARAMETER(psPMR);

	return PVRSRV_OK;
}
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

/* callback function for locking the system physical page addresses.
   As we are LMA there is nothing to do as we control physical memory. */
static PVRSRV_ERROR
PMRLockSysPhysAddressesLocalMem(PMR_IMPL_PRIVDATA pvPriv)
{

	PVRSRV_ERROR eError;
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData;

	psLMAllocArrayData = pvPriv;

	if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_ONDEMAND))
	{
		/* Allocate Memory for deferred allocation */
		eError = _AllocLMPages(psLMAllocArrayData, NULL);
		PVR_RETURN_IF_ERROR(eError);
	}

	return PVRSRV_OK;
}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
static PVRSRV_ERROR
PMRUnlockSysPhysAddressesLocalMem(PMR_IMPL_PRIVDATA pvPriv,
                                  PMR_IMPL_ZOMBIEPAGES *ppvZombiePages)
#else
static PVRSRV_ERROR
PMRUnlockSysPhysAddressesLocalMem(PMR_IMPL_PRIVDATA pvPriv)
#endif
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData = pvPriv;
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	PMR_LMALLOCARRAY_DATA *psExtractedPagesPageArray = NULL;

	*ppvZombiePages = NULL;
#endif

	if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_ONDEMAND))
	{
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
		if (psLMAllocArrayData->iNumChunksAllocated == 0)
		{
			*ppvZombiePages = NULL;
			return PVRSRV_OK;
		}

		eError = _ExtractAllPages(psLMAllocArrayData,
		                          &psExtractedPagesPageArray);
		PVR_LOG_GOTO_IF_ERROR(eError, "_ExtractAllPages", e0);

		if (psExtractedPagesPageArray)
		{
			/* Zombify pages to get proper stats */
			eError = PMRZombifyLocalMem(psExtractedPagesPageArray, NULL);
			PVR_WARN_IF_ERROR(eError, "PMRZombifyLocalMem");
		}
		*ppvZombiePages = psExtractedPagesPageArray;
#else
		/* Free Memory for deferred allocation */
		eError = _FreeLMPages(psLMAllocArrayData, NULL, 0);
		PVR_RETURN_IF_ERROR(eError);
#endif
	}

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
e0:
#endif
	PVR_ASSERT(eError == PVRSRV_OK);
	return eError;
}

/* N.B. It is assumed that PMRLockSysPhysAddressesLocalMem() is called _before_ this function! */
static PVRSRV_ERROR
PMRSysPhysAddrLocalMem(PMR_IMPL_PRIVDATA pvPriv,
					   IMG_UINT32 ui32Log2PageSize,
					   IMG_UINT32 ui32NumOfPages,
					   IMG_DEVMEM_OFFSET_T *puiOffset,
#if defined(SUPPORT_STATIC_IPA)
					   IMG_UINT64 ui64IPAPolicyValue,
					   IMG_UINT64 ui64IPAClearMask,
#endif
					   IMG_BOOL *pbValid,
					   IMG_DEV_PHYADDR *psDevPAddr)
{
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData = pvPriv;
	IMG_UINT32 idx;
	IMG_UINT32 uiLog2AllocSize;
	IMG_UINT64 uiAllocIndex;
	IMG_DEVMEM_OFFSET_T uiInAllocOffset;
	IMG_UINT32 uiNumAllocs = psLMAllocArrayData->uiTotalNumChunks;

#if defined(SUPPORT_STATIC_IPA)
	PVR_UNREFERENCED_PARAMETER(ui64IPAPolicyValue);
	PVR_UNREFERENCED_PARAMETER(ui64IPAClearMask);
#endif

	if (psLMAllocArrayData->uiLog2ChunkSize < ui32Log2PageSize)
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: Requested physical addresses from PMR "
		         "for incompatible contiguity %u!",
		         __func__,
		         ui32Log2PageSize));
		return PVRSRV_ERROR_PMR_INCOMPATIBLE_CONTIGUITY;
	}

	PVR_ASSERT(psLMAllocArrayData->uiLog2ChunkSize != 0);
	PVR_ASSERT(ui32Log2PageSize >= RA_BASE_FLAGS_LOG2);

	if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_PHYS_CONTIG))
	{
		for (idx=0; idx < ui32NumOfPages; idx++)
		{
			if (pbValid[idx])
			{
				psDevPAddr[idx].uiAddr = psLMAllocArrayData->aBaseArray[0] + puiOffset[idx];
#if defined(SUPPORT_STATIC_IPA)
				/* Modify the physical address with the associated IPA values */
				psDevPAddr[idx].uiAddr &= ~ui64IPAClearMask;
				psDevPAddr[idx].uiAddr |= ui64IPAPolicyValue;
#endif
			}
		}
	}
	else
	{
		uiLog2AllocSize = psLMAllocArrayData->uiLog2ChunkSize;

		for (idx=0; idx < ui32NumOfPages; idx++)
		{
			if (pbValid[idx])
			{
				uiAllocIndex = puiOffset[idx] >> uiLog2AllocSize;
				uiInAllocOffset = puiOffset[idx] - (uiAllocIndex << uiLog2AllocSize);

				PVR_LOG_RETURN_IF_FALSE(uiAllocIndex < uiNumAllocs,
										"puiOffset out of range", PVRSRV_ERROR_OUT_OF_RANGE);

				PVR_ASSERT(uiInAllocOffset < (1ULL << uiLog2AllocSize));

				/* The base may or may not be a ghost base, but we don't care,
				 * we just need the real representation of the base.
				 */
				psDevPAddr[idx].uiAddr = RA_BASE_STRIP_GHOST_BIT(
					psLMAllocArrayData->aBaseArray[uiAllocIndex]) + uiInAllocOffset;
#if defined(SUPPORT_STATIC_IPA)
				/* Modify the physical address with the associated IPA values */
				psDevPAddr[idx].uiAddr &= ~ui64IPAClearMask;
				psDevPAddr[idx].uiAddr |= ui64IPAPolicyValue;
#endif
			}
		}
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR
PMRAcquireKernelMappingDataLocalMem(PMR_IMPL_PRIVDATA pvPriv,
								 size_t uiOffset,
								 size_t uiSize,
								 void **ppvKernelAddressOut,
								 IMG_HANDLE *phHandleOut,
								 PMR_FLAGS_T ulFlags)
{
	PVRSRV_ERROR eError;
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData = pvPriv;
	PMR_KERNEL_MAPPING *psKernelMapping;
	RA_BASE_T *paBaseArray;
	IMG_UINT32 ui32ChunkIndex = 0;
	size_t uiOffsetMask = uiOffset;

	IMG_UINT32 uiLog2ChunkSize = psLMAllocArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64ChunkSize = IMG_PAGE2BYTES64(uiLog2ChunkSize);
	IMG_UINT64 ui64PhysSize;

	PVR_ASSERT(psLMAllocArrayData);
	PVR_ASSERT(ppvKernelAddressOut);
	PVR_ASSERT(phHandleOut);

	if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_SPARSE))
	{
		IMG_UINT32 i;
		/* Locate the desired physical chunk to map in */
		ui32ChunkIndex = uiOffset >> psLMAllocArrayData->uiLog2ChunkSize;

		if (OSIsMapPhysNonContigSupported())
		{
			/* If a size hasn't been supplied assume we are mapping a single page */
			IMG_UINT32 uiNumChunksToMap;

			/* This is to support OSMapPMR originated parameters */
			if (uiOffset == 0 && uiSize == 0)
			{
				uiNumChunksToMap = psLMAllocArrayData->iNumChunksAllocated;
			}
			else
			{
				uiNumChunksToMap = uiSize >> psLMAllocArrayData->uiLog2ChunkSize;
			}

			/* Check we are attempting to map at least a chunk in size */
			if (uiNumChunksToMap < 1)
			{
				PVR_LOG_RETURN_IF_ERROR(PVRSRV_ERROR_INVALID_PARAMS, "uiNumChunksToMap < 1");
			}

			/* Check contiguous region doesn't exceed size of PMR */
			if (ui32ChunkIndex + (uiNumChunksToMap - 1) > psLMAllocArrayData->uiTotalNumChunks)
			{
				PVR_LOG_RETURN_IF_ERROR(PVRSRV_ERROR_INVALID_PARAMS,
				                        "Mapping range exceeds total num chunks in PMR");
			}

			/* Check the virtually contiguous region given is physically backed */
			for (i = ui32ChunkIndex; i < ui32ChunkIndex + uiNumChunksToMap; i++)
			{
				if (RA_BASE_IS_INVALID(psLMAllocArrayData->aBaseArray[i]))
				{
					PVR_LOG_RETURN_IF_ERROR(PVRSRV_ERROR_PMR_INCOMPATIBLE_CONTIGUITY, "Sparse contiguity check");
				}
			}
			/* Size of virtually contiguous sparse alloc */
			ui64PhysSize = IMG_PAGES2BYTES64(uiNumChunksToMap, psLMAllocArrayData->uiLog2ChunkSize);
		}
		else
		{
			size_t uiStart = uiOffset;
			size_t uiEnd = uiOffset + uiSize - 1;
			size_t uiChunkMask = ~(IMG_PAGE2BYTES64(psLMAllocArrayData->uiLog2ChunkSize) - 1);

			/* We can still map if only one chunk is required */
			if ((uiStart & uiChunkMask) != (uiEnd & uiChunkMask))
			{
				PVR_LOG_RETURN_IF_ERROR(PVRSRV_ERROR_PMR_INCOMPATIBLE_CONTIGUITY, "Sparse contiguity check");
			}
			/* Map a single chunk */
			ui64PhysSize = ui64ChunkSize;
		}

		paBaseArray = &psLMAllocArrayData->aBaseArray[ui32ChunkIndex];

		/* Offset mask to be used for address offsets within a chunk */
		uiOffsetMask = (1U << psLMAllocArrayData->uiLog2ChunkSize) - 1;
	}
	else
	{
		paBaseArray = psLMAllocArrayData->aBaseArray;
		ui64PhysSize = IMG_PAGES2BYTES64(psLMAllocArrayData->uiTotalNumChunks, uiLog2ChunkSize);
	}

	PVR_ASSERT(ui32ChunkIndex < psLMAllocArrayData->uiTotalNumChunks);

	psKernelMapping = OSAllocMem(sizeof(*psKernelMapping));
	PVR_RETURN_IF_NOMEM(psKernelMapping);

	eError = _MapPMRKernel(psLMAllocArrayData,
	                       paBaseArray,
	                       ui64PhysSize,
	                       ulFlags,
	                       psKernelMapping);
	if (eError == PVRSRV_OK)
	{
		/* uiOffset & uiOffsetMask is used to get the kernel addr within the page */
		*ppvKernelAddressOut = ((IMG_CHAR *) psKernelMapping->pvKernelAddress) + (uiOffset & uiOffsetMask);
		*phHandleOut = psKernelMapping;
	}
	else
	{
		OSFreeMem(psKernelMapping);
		PVR_LOG_ERROR(eError, "_MapPMRKernel");
	}

	return eError;
}

static void PMRReleaseKernelMappingDataLocalMem(PMR_IMPL_PRIVDATA pvPriv,
                                                IMG_HANDLE hHandle)
{
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData = (PMR_LMALLOCARRAY_DATA *) pvPriv;
	PMR_KERNEL_MAPPING *psKernelMapping = (PMR_KERNEL_MAPPING *) hHandle;

	PVR_ASSERT(psLMAllocArrayData);
	PVR_ASSERT(psKernelMapping);

	_UnMapPMRKernel(psLMAllocArrayData,
	                psKernelMapping);

	OSFreeMem(psKernelMapping);
}

static PVRSRV_ERROR
CopyBytesLocalMem(PMR_IMPL_PRIVDATA pvPriv,
				  IMG_DEVMEM_OFFSET_T uiOffset,
				  IMG_UINT8 *pcBuffer,
				  size_t uiBufSz,
				  size_t *puiNumBytes,
				  void (*pfnCopyBytes)(IMG_UINT8 *pcBuffer,
									   IMG_UINT8 *pcPMR,
									   size_t uiSize))
{
	PMR_LMALLOCARRAY_DATA *psLMAllocArrayData = pvPriv;
	size_t uiBytesCopied;
	size_t uiBytesToCopy;
	size_t uiBytesCopyableFromAlloc;
	PMR_KERNEL_MAPPING sMapping;
	IMG_UINT8 *pcKernelPointer = NULL;
	size_t uiBufferOffset;
	IMG_UINT64 uiAllocIndex;
	IMG_DEVMEM_OFFSET_T uiInAllocOffset;
	IMG_UINT32 uiLog2ChunkSize = psLMAllocArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64ChunkSize = IMG_PAGE2BYTES64(uiLog2ChunkSize);
	IMG_UINT64 ui64PhysSize;
	PVRSRV_ERROR eError;

	uiBytesCopied = 0;
	uiBytesToCopy = uiBufSz;
	uiBufferOffset = 0;

	if (BIT_ISSET(psLMAllocArrayData->ui32Flags, FLAG_SPARSE))
	{
		while (uiBytesToCopy > 0)
		{
			/* we have to map one alloc in at a time */
			PVR_ASSERT(psLMAllocArrayData->uiLog2ChunkSize != 0);
			uiAllocIndex = uiOffset >> psLMAllocArrayData->uiLog2ChunkSize;
			uiInAllocOffset = uiOffset - (uiAllocIndex << psLMAllocArrayData->uiLog2ChunkSize);
			uiBytesCopyableFromAlloc = uiBytesToCopy;
			if (uiBytesCopyableFromAlloc + uiInAllocOffset > (1ULL << psLMAllocArrayData->uiLog2ChunkSize))
			{
				uiBytesCopyableFromAlloc = TRUNCATE_64BITS_TO_SIZE_T((1ULL << psLMAllocArrayData->uiLog2ChunkSize)-uiInAllocOffset);
			}
			/* Mapping a single chunk at a time */
			ui64PhysSize = ui64ChunkSize;

			PVR_ASSERT(uiBytesCopyableFromAlloc != 0);
			PVR_ASSERT(uiAllocIndex < psLMAllocArrayData->uiTotalNumChunks);
			PVR_ASSERT(uiInAllocOffset < (1ULL << uiLog2ChunkSize));

			eError = _MapPMRKernel(psLMAllocArrayData,
			                       &psLMAllocArrayData->aBaseArray[uiAllocIndex],
			                       ui64PhysSize,
			                       PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC,
			                       &sMapping);
			PVR_GOTO_IF_ERROR(eError, e0);
			pcKernelPointer = sMapping.pvKernelAddress;
			pfnCopyBytes(&pcBuffer[uiBufferOffset], &pcKernelPointer[uiInAllocOffset], uiBytesCopyableFromAlloc);

			_UnMapPMRKernel(psLMAllocArrayData,
			                &sMapping);

			uiBufferOffset += uiBytesCopyableFromAlloc;
			uiBytesToCopy -= uiBytesCopyableFromAlloc;
			uiOffset += uiBytesCopyableFromAlloc;
			uiBytesCopied += uiBytesCopyableFromAlloc;
		}
	}
	else
	{
		ui64PhysSize = IMG_PAGES2BYTES64(psLMAllocArrayData->uiTotalNumChunks, uiLog2ChunkSize);
		PVR_ASSERT((uiOffset + uiBufSz) <= ui64PhysSize);
		PVR_ASSERT(ui64ChunkSize != 0);
		eError = _MapPMRKernel(psLMAllocArrayData,
		                       psLMAllocArrayData->aBaseArray,
		                       ui64PhysSize,
		                       PVRSRV_MEMALLOCFLAG_CPU_UNCACHED_WC,
		                       &sMapping);
		PVR_GOTO_IF_ERROR(eError, e0);
		pcKernelPointer = sMapping.pvKernelAddress;
		pfnCopyBytes(pcBuffer, &pcKernelPointer[uiOffset], uiBufSz);

		_UnMapPMRKernel(psLMAllocArrayData,
		                &sMapping);

		uiBytesCopied = uiBufSz;
	}
	*puiNumBytes = uiBytesCopied;
	return PVRSRV_OK;
e0:
	*puiNumBytes = uiBytesCopied;
	return eError;
}

static void ReadLocalMem(IMG_UINT8 *pcBuffer,
						 IMG_UINT8 *pcPMR,
						 size_t uiSize)
{
	/* the memory is mapped as WC (and also aligned to page size) so we can
	 * safely call "Cached" memcpy */
	OSCachedMemCopy(pcBuffer, pcPMR, uiSize);
}

static PVRSRV_ERROR
PMRReadBytesLocalMem(PMR_IMPL_PRIVDATA pvPriv,
				  IMG_DEVMEM_OFFSET_T uiOffset,
				  IMG_UINT8 *pcBuffer,
				  size_t uiBufSz,
				  size_t *puiNumBytes)
{
	return CopyBytesLocalMem(pvPriv,
							 uiOffset,
							 pcBuffer,
							 uiBufSz,
							 puiNumBytes,
							 ReadLocalMem);
}

static void WriteLocalMem(IMG_UINT8 *pcBuffer,
						  IMG_UINT8 *pcPMR,
						  size_t uiSize)
{
	/* the memory is mapped as WC (and also aligned to page size) so we can
	 * safely call "Cached" memcpy but need to issue a write memory barrier
	 * to flush the write buffers after */
	OSCachedMemCopyWMB(pcPMR, pcBuffer, uiSize);
}

static PVRSRV_ERROR
PMRWriteBytesLocalMem(PMR_IMPL_PRIVDATA pvPriv,
					  IMG_DEVMEM_OFFSET_T uiOffset,
					  IMG_UINT8 *pcBuffer,
					  size_t uiBufSz,
					  size_t *puiNumBytes)
{
	return CopyBytesLocalMem(pvPriv,
							 uiOffset,
							 pcBuffer,
							 uiBufSz,
							 puiNumBytes,
							 WriteLocalMem);
}

/*************************************************************************/ /*!
@Function       PMRChangeSparseMemLocalMem
@Description    This function Changes the sparse mapping by allocating and
                freeing of pages. It also changes the GPU maps accordingly.
@Return         PVRSRV_ERROR failure code
*/ /**************************************************************************/
static PVRSRV_ERROR
PMRChangeSparseMemLocalMem(PMR_IMPL_PRIVDATA pPriv,
                           const PMR *psPMR,
                           IMG_UINT32 ui32AllocPageCount,
                           IMG_UINT32 *pai32AllocIndices,
                           IMG_UINT32 ui32FreePageCount,
                           IMG_UINT32 *pai32FreeIndices,
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
                           PMR_IMPL_ZOMBIEPAGES *ppvZombiePages,
#endif
                           IMG_UINT32 uiFlags)
{
	PVRSRV_ERROR eError = PVRSRV_ERROR_INVALID_PARAMS;

	IMG_UINT32 ui32AdtnlAllocPages = 0;
	IMG_UINT32 ui32AdtnlFreePages = 0;
	IMG_UINT32 ui32CommonRequstCount = 0;
	IMG_UINT32 ui32Loop = 0;
	IMG_UINT32 ui32Index = 0;
	IMG_UINT32 uiAllocpgidx;
	IMG_UINT32 uiFreepgidx;

	PMR_LMALLOCARRAY_DATA *psPMRPageArrayData = (PMR_LMALLOCARRAY_DATA *)pPriv;
	IMG_UINT32 uiLog2ChunkSize = psPMRPageArrayData->uiLog2ChunkSize;
	IMG_UINT64 ui64ChunkSize = IMG_PAGE2BYTES64(uiLog2ChunkSize);

#if defined(DEBUG)
	IMG_BOOL bPoisonFail = IMG_FALSE;
	IMG_BOOL bZeroFail = IMG_FALSE;
#endif

	/* Fetch the Page table array represented by the PMR */
	RA_BASE_T *paBaseArray = psPMRPageArrayData->aBaseArray;
	PMR_MAPPING_TABLE *psPMRMapTable = PMR_GetMappingTable(psPMR);

#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	*ppvZombiePages = NULL;
#endif
	/* The incoming request is classified into two operations independent of
	 * each other: alloc & free chunks.
	 * These operations can be combined with two mapping operations as well
	 * which are GPU & CPU space mappings.
	 *
	 * From the alloc and free chunk requests, the net amount of chunks to be
	 * allocated or freed is computed. Chunks that were requested to be freed
	 * will be reused to fulfil alloc requests.
	 *
	 * The order of operations is:
	 * 1. Allocate new Chunks.
	 * 2. Move the free chunks from free request to alloc positions.
	 * 3. Free the rest of the chunks not used for alloc
	 *
	 * Alloc parameters are validated at the time of allocation
	 * and any error will be handled then. */

	if (SPARSE_RESIZE_BOTH == (uiFlags & SPARSE_RESIZE_BOTH))
	{
		ui32CommonRequstCount = (ui32AllocPageCount > ui32FreePageCount) ?
				ui32FreePageCount : ui32AllocPageCount;

		PDUMP_PANIC(PMR_DeviceNode(psPMR), SPARSEMEM_SWAP, "Request to swap alloc & free chunks not supported");
	}

	if (SPARSE_RESIZE_ALLOC == (uiFlags & SPARSE_RESIZE_ALLOC))
	{
		ui32AdtnlAllocPages = ui32AllocPageCount - ui32CommonRequstCount;
	}
	else
	{
		ui32AllocPageCount = 0;
	}

	if (SPARSE_RESIZE_FREE == (uiFlags & SPARSE_RESIZE_FREE))
	{
		ui32AdtnlFreePages = ui32FreePageCount - ui32CommonRequstCount;
	}
	else
	{
		ui32FreePageCount = 0;
	}

	PVR_LOG_RETURN_IF_FALSE(
	    (ui32CommonRequstCount | ui32AdtnlAllocPages | ui32AdtnlFreePages) != 0,
	    "Invalid combination of parameters: ui32CommonRequstCount,"
	    " ui32AdtnlAllocPages and ui32AdtnlFreePages.",
	    PVRSRV_ERROR_INVALID_PARAMS
	);

	{
		/* Validate the free page indices */
		if (ui32FreePageCount)
		{
			if (pai32FreeIndices != NULL)
			{
				for (ui32Loop = 0; ui32Loop < ui32FreePageCount; ui32Loop++)
				{
					uiFreepgidx = pai32FreeIndices[ui32Loop];

					if (uiFreepgidx >= psPMRPageArrayData->uiTotalNumChunks)
					{
						PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_OUT_OF_RANGE, e0);
					}

					if (RA_BASE_IS_INVALID(paBaseArray[uiFreepgidx]))
					{
						PVR_LOG_GOTO_WITH_ERROR("paBaseArray[uiFreepgidx]", eError, PVRSRV_ERROR_INVALID_PARAMS, e0);
					}
				}
			}
			else
			{
				PVR_DPF((PVR_DBG_ERROR,
				         "%s: Given non-zero free count but missing indices array",
				         __func__));
				return PVRSRV_ERROR_INVALID_PARAMS;
			}
		}

		/* The following block of code verifies any issues with common alloc chunk indices */
		for (ui32Loop = ui32AdtnlAllocPages; ui32Loop < ui32AllocPageCount; ui32Loop++)
		{
			uiAllocpgidx = pai32AllocIndices[ui32Loop];
			if (uiAllocpgidx >= psPMRPageArrayData->uiTotalNumChunks)
			{
				PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_DEVICEMEM_OUT_OF_RANGE, e0);
			}

			if ((!RA_BASE_IS_INVALID(paBaseArray[uiAllocpgidx])) ||
					(psPMRMapTable->aui32Translation[uiAllocpgidx] != TRANSLATION_INVALID))
			{
				PVR_LOG_GOTO_WITH_ERROR("Trying to allocate already allocated page again", eError, PVRSRV_ERROR_INVALID_PARAMS, e0);
			}
		}

		ui32Loop = 0;

		/* Allocate new chunks */
		if (0 != ui32AdtnlAllocPages)
		{
			/* Say how many chunks to allocate */
			psPMRPageArrayData->uiChunksToAlloc = ui32AdtnlAllocPages;

			eError = _AllocLMPages(psPMRPageArrayData, pai32AllocIndices);
			PVR_LOG_GOTO_IF_ERROR(eError, "_AllocLMPages", e0);

			/* Mark the corresponding chunks of translation table as valid */
			for (ui32Loop = 0; ui32Loop < ui32AdtnlAllocPages; ui32Loop++)
			{
				psPMRMapTable->aui32Translation[pai32AllocIndices[ui32Loop]] = pai32AllocIndices[ui32Loop];
			}

			psPMRMapTable->ui32NumPhysChunks += ui32AdtnlAllocPages;
		}

		ui32Index = ui32Loop;
		ui32Loop = 0;

		/* Move the corresponding free chunks to alloc request */
		eError = RA_SwapSparseMem(psPMRPageArrayData->psArena,
		                           paBaseArray,
		                           psPMRPageArrayData->uiTotalNumChunks,
		                           psPMRPageArrayData->uiLog2ChunkSize,
		                           &pai32AllocIndices[ui32Index],
		                           &pai32FreeIndices[ui32Loop],
		                           ui32CommonRequstCount);
		PVR_LOG_GOTO_IF_ERROR(eError, "RA_SwapSparseMem", unwind_alloc);

		for (ui32Loop = 0; ui32Loop < ui32CommonRequstCount; ui32Loop++, ui32Index++)
		{
			uiAllocpgidx = pai32AllocIndices[ui32Index];
			uiFreepgidx  = pai32FreeIndices[ui32Loop];

			psPMRMapTable->aui32Translation[uiFreepgidx] = TRANSLATION_INVALID;
			psPMRMapTable->aui32Translation[uiAllocpgidx] = uiAllocpgidx;

			/* Be sure to honour the attributes associated with the allocation
			 * such as zeroing, poisoning etc. */
			if (BIT_ISSET(psPMRPageArrayData->ui32Flags, FLAG_POISON_ON_ALLOC))
			{
				eError = _PhysPgMemSet(psPMRPageArrayData,
				                       &psPMRPageArrayData->aBaseArray[uiAllocpgidx],
				                       ui64ChunkSize,
				                       PVRSRV_POISON_ON_ALLOC_VALUE);

				/* Consider this as a soft failure and go ahead but log error to kernel log */
				if (eError != PVRSRV_OK)
				{
#if defined(DEBUG)
					bPoisonFail = IMG_TRUE;
#endif
				}
			}

			if (BIT_ISSET(psPMRPageArrayData->ui32Flags, FLAG_ZERO))
			{
				eError = _PhysPgMemSet(psPMRPageArrayData,
									   &psPMRPageArrayData->aBaseArray[uiAllocpgidx],
									   ui64ChunkSize,
									   ZERO_PAGE_VALUE);
				/* Consider this as a soft failure and go ahead but log error to kernel log */
				if (eError != PVRSRV_OK)
				{
#if defined(DEBUG)
					/* Don't think we need to zero any chunks further */
					bZeroFail = IMG_TRUE;
#endif
				}
			}
		}

		/* Free or zombie the additional free chunks */
		if (0 != ui32AdtnlFreePages)
		{
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
			PMR_LMALLOCARRAY_DATA *psExtractedPagesPageArray = NULL;

			eError = _ExtractPages(psPMRPageArrayData, &pai32FreeIndices[ui32Loop], ui32AdtnlFreePages, &psExtractedPagesPageArray);
			PVR_LOG_GOTO_IF_ERROR(eError, "_ExtractPages", e0);

			/* Zombify pages to get proper stats */
			eError = PMRZombifyLocalMem(psExtractedPagesPageArray, NULL);
			PVR_LOG_IF_ERROR(eError, "PMRZombifyLocalMem");

			*ppvZombiePages = psExtractedPagesPageArray;
#else
			eError = _FreeLMPages(psPMRPageArrayData, &pai32FreeIndices[ui32Loop], ui32AdtnlFreePages);
			PVR_LOG_GOTO_IF_ERROR(eError, "_FreeLMPages", e0);
#endif /* SUPPORT_PMR_PAGES_DEFERRED_FREE */
			ui32Index = ui32Loop;
			ui32Loop = 0;

			while (ui32Loop++ < ui32AdtnlFreePages)
			{
				/* Set the corresponding mapping table entry to invalid address */
				psPMRMapTable->aui32Translation[pai32FreeIndices[ui32Index++]] = TRANSLATION_INVALID;
			}

			psPMRMapTable->ui32NumPhysChunks -= ui32AdtnlFreePages;
		}
	}

#if defined(DEBUG)
	if (IMG_TRUE == bPoisonFail)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Error in poisoning the chunk", __func__));
	}

	if (IMG_TRUE == bZeroFail)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Error in zeroing the chunk", __func__));
	}
#endif

	return PVRSRV_OK;

unwind_alloc:
	_FreeLMPages(psPMRPageArrayData, pai32AllocIndices, ui32Index);

	for (ui32Loop = 0; ui32Loop < ui32Index; ui32Loop++)
	{
		psPMRMapTable->aui32Translation[pai32AllocIndices[ui32Loop]] = TRANSLATION_INVALID;
	}

e0:
	return eError;
}

static PMR_IMPL_FUNCTAB _sPMRLMAFuncTab = {
	.pfnLockPhysAddresses = &PMRLockSysPhysAddressesLocalMem,
	.pfnUnlockPhysAddresses = &PMRUnlockSysPhysAddressesLocalMem,
	.pfnDevPhysAddr = &PMRSysPhysAddrLocalMem,
	.pfnAcquireKernelMappingData = &PMRAcquireKernelMappingDataLocalMem,
	.pfnReleaseKernelMappingData = &PMRReleaseKernelMappingDataLocalMem,
	.pfnReadBytes = &PMRReadBytesLocalMem,
	.pfnWriteBytes = &PMRWriteBytesLocalMem,
	.pfnChangeSparseMem = &PMRChangeSparseMemLocalMem,
	.pfnMMap = NULL,
	.pfnFinalize = &PMRFinalizeLocalMem,
#if defined(SUPPORT_PMR_PAGES_DEFERRED_FREE)
	.pfnFreeZombiePages = &PMRFreeZombiePagesRAMem,
#endif
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	.pfnZombify = &PMRZombifyLocalMem,
#endif
};

PVRSRV_ERROR
PhysmemNewRAMemRamBackedPMR(PHYS_HEAP *psPhysHeap,
                            RA_ARENA *pArena,
                            CONNECTION_DATA *psConnection,
                            IMG_DEVMEM_SIZE_T uiSize,
                            IMG_UINT32 ui32NumPhysChunks,
                            IMG_UINT32 ui32NumVirtChunks,
                            IMG_UINT32 *pui32MappingTable,
                            IMG_UINT32 uiLog2AllocPageSize,
                            PVRSRV_MEMALLOCFLAGS_T uiFlags,
                            const IMG_CHAR *pszAnnotation,
                            IMG_PID uiPid,
                            PMR **ppsPMRPtr,
                            IMG_UINT32 ui32PDumpFlags)
{
	PVRSRV_ERROR eError;
	PVRSRV_ERROR eError2;
	PMR *psPMR = NULL;
	PMR_LMALLOCARRAY_DATA *psPrivData = NULL;
	PMR_FLAGS_T uiPMRFlags;
	IMG_UINT32 ui32LMAllocFlags = 0;

	/* This path is checking for the type of PMR to create, if sparse we
	 * have to perform additional validation as we can only map sparse ranges
	 * if the os functionality to do so is present. We can also only map virtually
	 * contiguous sparse regions. Non backed gaps in a range cannot be mapped.
	 */
	if (ui32NumPhysChunks != ui32NumVirtChunks || ui32NumVirtChunks > 1)
	{
		if (PVRSRV_CHECK_KERNEL_CPU_MAPPABLE(uiFlags) &&
		   !OSIsMapPhysNonContigSupported())
		{
			PVR_DPF((PVR_DBG_ERROR,
					"%s: LMA kernel mapping functions not available "
					"for physically discontiguous memory.",
					__func__));
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_INVALID_PARAMS, errorOnParam);
		}
		BIT_SET(ui32LMAllocFlags, FLAG_SPARSE);
	}

	if (PVRSRV_CHECK_ON_DEMAND(uiFlags))
	{
		BIT_SET(ui32LMAllocFlags, FLAG_ONDEMAND);
	}

	if (PVRSRV_CHECK_ZERO_ON_ALLOC(uiFlags))
	{
		BIT_SET(ui32LMAllocFlags, FLAG_ZERO);
	}

	if (PVRSRV_CHECK_POISON_ON_ALLOC(uiFlags))
	{
		BIT_SET(ui32LMAllocFlags, FLAG_POISON_ON_ALLOC);
	}

#if defined(DEBUG)
	if (PVRSRV_CHECK_POISON_ON_FREE(uiFlags))
	{
		BIT_SET(ui32LMAllocFlags, FLAG_POISON_ON_FREE);
	}
#endif

	/* Create Array structure that holds the physical pages */
	eError = _AllocLMPageArray(uiSize,
	                           ui32NumPhysChunks,
	                           uiLog2AllocPageSize,
	                           ui32LMAllocFlags,
	                           psPhysHeap,
	                           pArena,
	                           uiFlags,
	                           uiPid,
	                           &psPrivData,
	                           psConnection);
	PVR_GOTO_IF_ERROR(eError, errorOnAllocPageArray);

	if (!BIT_ISSET(ui32LMAllocFlags, FLAG_ONDEMAND))
	{
		/* Allocate the physical pages */
		eError = _AllocLMPages(psPrivData, pui32MappingTable);
		PVR_GOTO_IF_ERROR(eError, errorOnAllocPages);
	}

	/* In this instance, we simply pass flags straight through.

	   Generically, uiFlags can include things that control the PMR
	   factory, but we don't need any such thing (at the time of
	   writing!), and our caller specifies all PMR flags so we don't
	   need to meddle with what was given to us.
	*/
	uiPMRFlags = (PMR_FLAGS_T)(uiFlags & PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK);
	/* check no significant bits were lost in cast due to different
	   bit widths for flags */
	PVR_ASSERT(uiPMRFlags == (uiFlags & PVRSRV_MEMALLOCFLAGS_PMRFLAGSMASK));

	if (BIT_ISSET(ui32LMAllocFlags, FLAG_ONDEMAND))
	{
		PDUMPCOMMENT(PhysHeapDeviceNode(psPhysHeap), "Deferred Allocation PMR (LMA)");
	}

	eError = PMRCreatePMR(psPhysHeap,
						  uiSize,
						  ui32NumPhysChunks,
						  ui32NumVirtChunks,
						  pui32MappingTable,
						  uiLog2AllocPageSize,
						  uiPMRFlags,
						  pszAnnotation,
						  &_sPMRLMAFuncTab,
						  psPrivData,
						  PMR_TYPE_LMA,
						  &psPMR,
						  ui32PDumpFlags);
	PVR_LOG_GOTO_IF_ERROR(eError, "PMRCreatePMR", errorOnCreate);

	*ppsPMRPtr = psPMR;
	return PVRSRV_OK;

errorOnCreate:
	if (!BIT_ISSET(ui32LMAllocFlags, FLAG_ONDEMAND) && psPrivData->iNumChunksAllocated)
	{
		eError2 = _FreeLMPages(psPrivData, NULL, 0);
		PVR_ASSERT(eError2 == PVRSRV_OK);
	}

errorOnAllocPages:
	_FreeLMPageArray(psPrivData);

errorOnAllocPageArray:
errorOnParam:
	PVR_ASSERT(eError != PVRSRV_OK);
	return eError;
}
