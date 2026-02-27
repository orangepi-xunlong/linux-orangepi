/*************************************************************************/ /*!
@File           physmem_lma.c
@Title          Local card memory allocator
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management. This module is responsible for
                implementing the function callbacks for local card memory.
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

#include "physmem_lma.h"
#include "pvr_debug.h"
#include "pvrsrv_memalloc_physheap.h"
#include "physheap.h"
#include "physheap_config.h"
#include "allocmem.h"
#include "ra.h"
#include "device.h"
#include "osfunc.h"
#include "physmem_ramem.h"
#include "pvrsrv.h"

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "rgxutils.h"
#endif

typedef struct PHYSMEM_LMA_DATA_TAG {
	RA_ARENA           *psRA;
	IMG_CPU_PHYADDR    sStartAddr;
	IMG_DEV_PHYADDR    sCardBase;
	IMG_UINT64         uiSize;
} PHYSMEM_LMA_DATA;

/*
 * This function will set the psDevPAddr to whatever the system layer
 * has set it for the referenced heap.
 * It will not fail if the psDevPAddr is invalid.
 */
static PVRSRV_ERROR
_GetDevPAddr(PHEAP_IMPL_DATA pvImplData,
			 IMG_DEV_PHYADDR *psDevPAddr)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;

	*psDevPAddr = psLMAData->sCardBase;

	return PVRSRV_OK;
}

/*
 * This function will set the psCpuPAddr to whatever the system layer
 * has set it for the referenced heap.
 * It will not fail if the psCpuPAddr is invalid.
 */
static PVRSRV_ERROR
_GetCPUPAddr(PHEAP_IMPL_DATA pvImplData,
			 IMG_CPU_PHYADDR *psCpuPAddr)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;

	*psCpuPAddr = psLMAData->sStartAddr;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_GetSize(PHEAP_IMPL_DATA pvImplData,
		 IMG_UINT64 *puiSize)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;

	*puiSize = psLMAData->uiSize;

	return PVRSRV_OK;
}

static void PhysmemGetRAMemRamMemStats(PHEAP_IMPL_DATA pvImplData,
		 IMG_UINT64 *pui64TotalSize,
		 IMG_UINT64 *pui64FreeSize)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;
	RA_USAGE_STATS sRAUsageStats;

	RA_Get_Usage_Stats(psLMAData->psRA, &sRAUsageStats);

	*pui64TotalSize = sRAUsageStats.ui64TotalArenaSize;
	*pui64FreeSize = sRAUsageStats.ui64FreeArenaSize;
}

static PVRSRV_ERROR
PhysmemGetArenaLMA(PHYS_HEAP *psPhysHeap,
				   RA_ARENA **ppsArena)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)PhysHeapGetImplData(psPhysHeap);

	PVR_LOG_RETURN_IF_FALSE(psLMAData != NULL, "psLMAData", PVRSRV_ERROR_NOT_IMPLEMENTED);

	*ppsArena = psLMAData->psRA;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
_CreateArenas(PHEAP_IMPL_DATA pvImplData, IMG_CHAR *pszLabel, PHYS_HEAP_POLICY uiPolicy)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;

	IMG_UINT32 ui32RAPolicy =
	    ((uiPolicy & PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG_MASK) == PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG)
	    ? RA_POLICY_ALLOC_ALLOW_NONCONTIG : RA_POLICY_DEFAULT;

	psLMAData->psRA = RA_Create_With_Span(pszLabel,
	                             OSGetPageShift(),
	                             psLMAData->sStartAddr.uiAddr,
	                             psLMAData->sCardBase.uiAddr,
	                             psLMAData->uiSize,
	                             ui32RAPolicy);
	PVR_LOG_RETURN_IF_NOMEM(psLMAData->psRA, "RA_Create_With_Span");

	return PVRSRV_OK;
}

static void
_DestroyArenas(PHEAP_IMPL_DATA pvImplData)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;

	/* Remove RAs and RA names for local card memory */
	if (psLMAData->psRA)
	{
		RA_Delete(psLMAData->psRA);
		psLMAData->psRA = NULL;
	}
}

static void
_DestroyImplData(PHEAP_IMPL_DATA pvImplData)
{
	PHYSMEM_LMA_DATA *psLMAData = (PHYSMEM_LMA_DATA*)pvImplData;

	_DestroyArenas(pvImplData);

	OSFreeMem(psLMAData);
}

struct _PHYS_HEAP_ITERATOR_ {
	PHYS_HEAP *psPhysHeap;
	RA_ARENA_ITERATOR *psRAIter;

	IMG_UINT64 uiTotalSize;
	IMG_UINT64 uiInUseSize;
};

PVRSRV_ERROR LMA_HeapIteratorCreate(PVRSRV_DEVICE_NODE *psDevNode,
                                    PVRSRV_PHYS_HEAP ePhysHeap,
                                    PHYS_HEAP_ITERATOR **ppsIter)
{
	PVRSRV_ERROR eError;
	PHYSMEM_LMA_DATA *psLMAData;
	PHYS_HEAP_ITERATOR *psHeapIter;
	PHYS_HEAP *psPhysHeap = NULL;
	RA_USAGE_STATS sStats;

	PVR_LOG_RETURN_IF_INVALID_PARAM(ppsIter != NULL, "ppsIter");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode != NULL, "psDevNode");

	eError = PhysHeapAcquireByID(ePhysHeap, psDevNode, &psPhysHeap);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysHeapAcquireByID");

	PVR_LOG_GOTO_IF_FALSE(PhysHeapGetType(psPhysHeap) == PHYS_HEAP_TYPE_LMA,
	                      "PhysHeap must be of LMA type", release_heap);

	psLMAData = (PHYSMEM_LMA_DATA *) PhysHeapGetImplData(psPhysHeap);

	psHeapIter = OSAllocMem(sizeof(*psHeapIter));
	PVR_LOG_GOTO_IF_NOMEM(psHeapIter, eError, release_heap);

	psHeapIter->psPhysHeap = psPhysHeap;
	psHeapIter->psRAIter = RA_IteratorAcquire(psLMAData->psRA, IMG_FALSE);
	PVR_LOG_GOTO_IF_NOMEM(psHeapIter->psRAIter, eError, free_heap_iter);

	/* get heap usage */
	RA_Get_Usage_Stats(psLMAData->psRA, &sStats);

	psHeapIter->uiTotalSize = sStats.ui64TotalArenaSize;
	psHeapIter->uiInUseSize = sStats.ui64TotalArenaSize - sStats.ui64FreeArenaSize;

	*ppsIter = psHeapIter;

	return PVRSRV_OK;

free_heap_iter:
	OSFreeMem(psHeapIter);

release_heap:
	PhysHeapRelease(psPhysHeap);

	return eError;
}

void LMA_HeapIteratorDestroy(PHYS_HEAP_ITERATOR *psIter)
{
	PHYS_HEAP_ITERATOR *psHeapIter = psIter;

	PVR_LOG_RETURN_VOID_IF_FALSE(psHeapIter != NULL, "psHeapIter is NULL");

	PhysHeapRelease(psHeapIter->psPhysHeap);
	RA_IteratorRelease(psHeapIter->psRAIter);
	OSFreeMem(psHeapIter);
}

PVRSRV_ERROR LMA_HeapIteratorReset(PHYS_HEAP_ITERATOR *psIter)
{
	PHYS_HEAP_ITERATOR *psHeapIter = psIter;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psHeapIter != NULL, "ppsIter");

	RA_IteratorReset(psHeapIter->psRAIter);

	return PVRSRV_OK;
}

IMG_BOOL LMA_HeapIteratorNext(PHYS_HEAP_ITERATOR *psIter,
                              IMG_DEV_PHYADDR *psDevPAddr,
                              IMG_UINT64 *puiSize)
{
	PHYS_HEAP_ITERATOR *psHeapIter = psIter;
	RA_ITERATOR_DATA sData = {0};

	if (psHeapIter == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "psHeapIter in %s() is NULL", __func__));
		return IMG_FALSE;
	}

	if (!RA_IteratorNext(psHeapIter->psRAIter, &sData))
	{
		return IMG_FALSE;
	}

	PVR_ASSERT(sData.uiSize != 0);

	psDevPAddr->uiAddr = sData.uiAddr;
	*puiSize = sData.uiSize;

	return IMG_TRUE;
}

PVRSRV_ERROR LMA_HeapIteratorGetHeapStats(PHYS_HEAP_ITERATOR *psIter,
                                          IMG_UINT64 *puiTotalSize,
                                          IMG_UINT64 *puiInUseSize)
{
	PHYS_HEAP_ITERATOR *psHeapIter = psIter;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psHeapIter != NULL, "psHeapIter");

	*puiTotalSize = psHeapIter->uiTotalSize;
	*puiInUseSize = psHeapIter->uiInUseSize;

	return PVRSRV_OK;
}

#if defined(SUPPORT_GPUVIRT_VALIDATION)
static PVRSRV_ERROR
LMA_PhyContigPagesAllocGPV(PHYS_HEAP *psPhysHeap,
                           size_t uiSize,
                           PG_HANDLE *psMemHandle,
                           IMG_DEV_PHYADDR *psDevPAddr,
                           IMG_UINT32 ui32OSid,
                           IMG_PID uiPid)
{
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPhysHeap);
	RA_ARENA *pArena;
	IMG_UINT32 ui32Log2NumPages = 0;
	PVRSRV_ERROR eError;

	PVR_ASSERT(uiSize != 0);
	ui32Log2NumPages = OSGetOrder(uiSize);
	uiSize = IMG_PAGES2BYTES64(OSGetPageSize(),ui32Log2NumPages);

	PVR_ASSERT(ui32OSid < GPUVIRT_VALIDATION_NUM_OS);
	if (ui32OSid >= GPUVIRT_VALIDATION_NUM_OS)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid Arena index %u defaulting to 0",
		        __func__, ui32OSid));
		ui32OSid = 0;
	}

	pArena = psDevNode->psOSidSubArena[ui32OSid];

	if (psMemHandle->uiOSid != ui32OSid)
	{
		PVR_LOG(("%s: Unexpected OSid value %u - expecting %u", __func__,
		        psMemHandle->uiOSid, ui32OSid));
	}

	psMemHandle->uiOSid = ui32OSid;		/* For Free() use */

	eError =  RAMemDoPhyContigPagesAlloc(pArena, uiSize, psDevNode, psMemHandle,
	                                     psDevPAddr, uiPid);
	PVR_LOG_IF_ERROR(eError, "RAMemDoPhyContigPagesAlloc");

	return eError;
}
#endif

static PVRSRV_ERROR
LMA_PhyContigPagesAlloc(PHYS_HEAP *psPhysHeap,
                         size_t uiSize,
                         PG_HANDLE *psMemHandle,
                         IMG_DEV_PHYADDR *psDevPAddr,
                         IMG_PID uiPid)
{
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	IMG_UINT32 ui32OSid = 0;
	return LMA_PhyContigPagesAllocGPV(psPhysHeap, uiSize, psMemHandle, psDevPAddr,
									  ui32OSid, uiPid);
#else
	PVRSRV_ERROR eError;

	RA_ARENA *pArena;
	IMG_UINT32 ui32Log2NumPages = 0;
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPhysHeap);

	eError = PhysmemGetArenaLMA(psPhysHeap, &pArena);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemGetArenaLMA");

	PVR_ASSERT(uiSize != 0);
	ui32Log2NumPages = OSGetOrder(uiSize);
	uiSize = IMG_PAGES2BYTES64(OSGetPageSize(),ui32Log2NumPages);

	eError = RAMemDoPhyContigPagesAlloc(pArena, uiSize, psDevNode, psMemHandle,
	                                    psDevPAddr, uiPid);
	PVR_LOG_IF_ERROR(eError, "RAMemDoPhyContigPagesAlloc");

	return eError;
#endif
}

static void
LMA_PhyContigPagesFree(PHYS_HEAP *psPhysHeap,
                       PG_HANDLE *psMemHandle)
{
	RA_ARENA	*pArena;

#if defined(SUPPORT_GPUVIRT_VALIDATION)
	RA_BASE_T uiCardAddr = (RA_BASE_T) psMemHandle->u.ui64Handle;
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPhysHeap);
	IMG_UINT32	ui32OSid = psMemHandle->uiOSid;

	/*
	 * The Arena ID is set by the originating allocation, and maintained via
	 * the call stacks into this function. We have a limited range of IDs
	 * and if the passed value falls outside this we simply treat it as a
	 * 'global' arena ID of 0. This is where all default OS-specific allocations
	 * are created.
	 */
	PVR_ASSERT(ui32OSid < GPUVIRT_VALIDATION_NUM_OS);
	if (ui32OSid >= GPUVIRT_VALIDATION_NUM_OS)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid Arena index %u PhysAddr 0x%"
		         IMG_UINT64_FMTSPECx " Reverting to Arena 0", __func__,
		         ui32OSid, uiCardAddr));
		/*
		 * No way of determining what we're trying to free so default to the
		 * global default arena index 0.
		 */
		ui32OSid = 0;
	}

	pArena = psDevNode->psOSidSubArena[ui32OSid];

	PVR_DPF((PVR_DBG_MESSAGE, "%s: (GPU Virtualisation) Freeing 0x%"
	        IMG_UINT64_FMTSPECx ", Arena %u", __func__,
	        uiCardAddr, ui32OSid));

#else
	PhysmemGetArenaLMA(psPhysHeap, &pArena);
#endif

	RAMemDoPhyContigPagesFree(pArena,
	                          psMemHandle);
}

static PVRSRV_ERROR
LMAPhysmemNewRAMemRamBackedPMR(PHYS_HEAP *psPhysHeap,
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
	RA_ARENA *pArena;

	eError = PhysmemGetArenaLMA(psPhysHeap, &pArena);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemGetArenaLMA");

	eError = PhysmemNewRAMemRamBackedPMR(psPhysHeap,
	                                     pArena,
	                                     psConnection,
	                                     uiSize,
	                                     ui32NumPhysChunks,
	                                     ui32NumVirtChunks,
	                                     pui32MappingTable,
	                                     uiLog2AllocPageSize,
	                                     uiFlags,
	                                     pszAnnotation,
	                                     uiPid,
	                                     ppsPMRPtr,
	                                     ui32PDumpFlags);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemNewLocalRamBackedPMR");

	return PVRSRV_OK;
}

static PHEAP_IMPL_FUNCS _sPHEAPImplFuncs =
{
	.pfnDestroyData = &_DestroyImplData,
	.pfnGetDevPAddr = &_GetDevPAddr,
	.pfnGetCPUPAddr = &_GetCPUPAddr,
	.pfnGetSize = &_GetSize,
	.pfnGetPageShift = &RAMemGetPageShift,
	.pfnGetFactoryMemStats = &PhysmemGetRAMemRamMemStats,
	.pfnCreatePMR = &LMAPhysmemNewRAMemRamBackedPMR,
#if defined(SUPPORT_GPUVIRT_VALIDATION)
	.pfnPagesAllocGPV = &LMA_PhyContigPagesAllocGPV,
#endif
	.pfnPagesAlloc = &LMA_PhyContigPagesAlloc,
	.pfnPagesFree = &LMA_PhyContigPagesFree,
	.pfnPagesMap = &RAMemPhyContigPagesMap,
	.pfnPagesUnMap = &RAMemPhyContigPagesUnmap,
	.pfnPagesClean = &RAMemPhyContigPagesClean,
};

PVRSRV_ERROR
PhysmemCreateHeapLMA(PVRSRV_DEVICE_NODE *psDevNode,
                     PHYS_HEAP_POLICY uiPolicy,
                     PHYS_HEAP_CONFIG *psConfig,
                     IMG_CHAR *pszLabel,
                     PHYS_HEAP **ppsPhysHeap)
{
	PHYSMEM_LMA_DATA *psLMAData;
	PHYS_HEAP *psPhysHeap;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(pszLabel != NULL, "pszLabel");

	PVR_ASSERT(psConfig->eType == PHYS_HEAP_TYPE_LMA ||
	           psConfig->eType == PHYS_HEAP_TYPE_DMA);

	psLMAData = OSAllocMem(sizeof(*psLMAData));
	PVR_LOG_RETURN_IF_NOMEM(psLMAData, "OSAllocMem");

	psLMAData->sStartAddr = PhysHeapConfigGetStartAddr(psConfig);
	psLMAData->sCardBase = PhysHeapConfigGetCardBase(psConfig);
	psLMAData->uiSize = PhysHeapConfigGetSize(psConfig);

	eError = PhysHeapCreate(psDevNode,
							psConfig,
							uiPolicy,
							(PHEAP_IMPL_DATA)psLMAData,
							&_sPHEAPImplFuncs,
							&psPhysHeap);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapCreate", err_free_lma_data);

	eError = _CreateArenas(psLMAData, pszLabel, uiPolicy);
	PVR_LOG_GOTO_IF_ERROR(eError, "_CreateArenas", err_free_physheap);

	if (ppsPhysHeap != NULL)
	{
		*ppsPhysHeap = psPhysHeap;
	}

	return PVRSRV_OK;

err_free_physheap:
	PhysHeapDestroy(psPhysHeap);
err_free_lma_data:
	OSFreeMem(psLMAData);
	return eError;

}
