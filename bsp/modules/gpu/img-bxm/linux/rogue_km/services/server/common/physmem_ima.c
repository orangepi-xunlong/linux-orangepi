/*************************************************************************/ /*!
@File           physmem_ima.c
@Title          Import memory allocator
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of memory management. This module is responsible for
                implementing the function callbacks for local card memory when
                used under a shared heap system.
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
#include "pvrsrv_error.h"
#include "pvrsrv_memalloc_physheap.h"
#include "physheap.h"
#include "physheap_config.h"
#include "allocmem.h"
#include "ra.h"
#include "device.h"
#include "osfunc.h"
#include "physmem_ima.h"
#include "physmem_dlm.h"
#include "physmem_ramem.h"

typedef struct PHYSMEM_IMA_DATA_TAG {
	PHYS_HEAP          *psPhysHeap;
	RA_ARENA           *psRA;
	PHYS_HEAP          *pDLMHeap;
	IMG_UINT32          uiLog2PMBSize;
	IMG_UINT32          uiReservedPMBs;
	PMB               **ppsReservedPMBs;
} PHYSMEM_IMA_DATA;

static PVRSRV_ERROR
IMAGetDevPAddr(PHEAP_IMPL_DATA pvImplData,
               IMG_DEV_PHYADDR *psDevPAddr)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;
	RA_ARENA_ITERATOR *psRAIter = RA_IteratorAcquire(psIMAData->psRA, IMG_FALSE);
	RA_ITERATOR_DATA sData = {0};
	PVRSRV_ERROR eError = PVRSRV_OK;

	PVR_LOG_RETURN_IF_NOMEM(psRAIter, "RA_IteratorAcquire");

	if (!RA_IteratorNext(psRAIter, &sData))
	{
		PVR_LOG_GOTO_WITH_ERROR("RA_IteratorNext",
		                        eError,
		                        PVRSRV_ERROR_FAILED_TO_GET_PHYS_ADDR,
		                        err_free_iter);
	}

	psDevPAddr->uiAddr = sData.uiAddr;

err_free_iter:
	RA_IteratorRelease(psRAIter);
	return eError;
}

static PVRSRV_ERROR
IMAGetCPUPAddr(PHEAP_IMPL_DATA pvImplData,
               IMG_CPU_PHYADDR *psCpuPAddr)
{
	IMG_DEV_PHYADDR sDevPAddr;
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;
	PVRSRV_ERROR eError = IMAGetDevPAddr(pvImplData, &sDevPAddr);
	PVR_LOG_RETURN_IF_ERROR(eError, "IMAGetDevPAddr");

	PhysHeapDevPAddrToCpuPAddr(
		psIMAData->pDLMHeap,
		1,
		psCpuPAddr,
		&sDevPAddr
	);

	return PVRSRV_OK;
}

static PVRSRV_ERROR
IMAGetSize(PHEAP_IMPL_DATA pvImplData,
           IMG_UINT64 *puiSize)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;
	RA_USAGE_STATS sRAUsageStats;

	RA_Get_Usage_Stats(psIMAData->psRA, &sRAUsageStats);

	*puiSize = sRAUsageStats.ui64TotalArenaSize;

	return PVRSRV_OK;
}

static void IMAPhysmemGetRAMemRamMemStats(PHEAP_IMPL_DATA pvImplData,
                                          IMG_UINT64 *pui64TotalSize,
                                          IMG_UINT64 *pui64FreeSize)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;
	RA_USAGE_STATS sRAUsageStats;

	RA_Get_Usage_Stats(psIMAData->psRA, &sRAUsageStats);

	*pui64TotalSize = sRAUsageStats.ui64TotalArenaSize;
	*pui64FreeSize = sRAUsageStats.ui64FreeArenaSize;
}

/*
 * We iterate on this function multiple times using the IterHandle.
 * First iteration will create the IterHandle.
 * We return IMG_TRUE when there are iterations remaining.
 * We return IMG_FALSE when there are no iterations remaining, this
 * will also free the IterHandle.
 * Callers must always iterate until false to ensure IterHandle is
 * freed.
 * */
static IMG_BOOL IMAGetHeapSpansStringIter(PHEAP_IMPL_DATA pvImplData,
                                          IMG_CHAR *ppszStrBuf,
                                          IMG_UINT32 uiStrBufSize,
                                          void **ppvIterHandle)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;
	RA_ITERATOR_DATA sData = {0};
	PVRSRV_ERROR eError;
	IMG_BOOL bIter = IMG_FALSE;

	/* If we haven't been given an IterHandle */
	if (!*ppvIterHandle)
	{
		*ppvIterHandle = RA_IteratorAcquire(psIMAData->psRA, IMG_TRUE);
		PVR_LOG_GOTO_IF_NOMEM(*ppvIterHandle, eError, return_false);
	}

	bIter = RA_IteratorNextSpan(*ppvIterHandle, &sData);
	if (bIter)
	{
		IMG_DEV_PHYADDR sRangeBase = {sData.uiAddr};
		IMG_CHAR aszCpuAddr[18] = "Non-Addressable"; /* length of CPUPHYADDR_UINT_FMTSPEC */
		IMG_UINT64 uiRangeSize = sData.uiSize;
		IMG_INT32 iCount;

		/* Cannot get the CPU addr for private heaps. Display "Non-Addressable" instead. */
		if ((PhysHeapGetFlags(psIMAData->psPhysHeap) & PHYS_HEAP_USAGE_GPU_PRIVATE) != PHYS_HEAP_USAGE_GPU_PRIVATE)
		{
			IMG_CPU_PHYADDR sCPURangeBase;
			PhysHeapDevPAddrToCpuPAddr(psIMAData->pDLMHeap,
			                           1,
			                           &sCPURangeBase,
			                           &sRangeBase);
			OSSNPrintf(aszCpuAddr, ARRAY_SIZE(aszCpuAddr), CPUPHYADDR_UINT_FMTSPEC, CPUPHYADDR_FMTARG(sCPURangeBase.uiAddr));
		}

		iCount = OSSNPrintf(ppszStrBuf,
							uiStrBufSize,
							"                            " /* padding */
							"CPU PA Base: %s, "
							"GPU PA Base: 0x%08"IMG_UINT64_FMTSPECx", "
							"Size: %"IMG_UINT64_FMTSPEC"B",
							aszCpuAddr,
							sRangeBase.uiAddr,
							uiRangeSize);
		if (!(0 < iCount && iCount < (IMG_INT32)uiStrBufSize))
		{
			PVR_DPF((PVR_DBG_ERROR, "OSSNPrintf in %s(), "
			                        "Heap Span print may be corrupt!", __func__));
		}
		return IMG_TRUE;
	}
	/* else end iteration and free the iter handle */

	RA_IteratorRelease(*ppvIterHandle);

return_false:
	return IMG_FALSE;
}

static void
IMAGetHeapDLMBacking(PHEAP_IMPL_DATA pvImplData,
                     PHYS_HEAP **psDLMPhysHeap)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;
	*psDLMPhysHeap = psIMAData->pDLMHeap;
}

/* When uiSize > DLM PMB size, allocates a single physically contiguous Huge PMB to satisfy the import.
 * A huge PMB is a PMB with a size larger than the DLM PMB size (1 << psIMAData->uiLog2PMBSize).
 * The requested import size will be rounded up to a multiple of the DLM PMB size. */
static PVRSRV_ERROR IMAImportDLMAllocHuge(RA_PERARENA_HANDLE hArenaHandle,
                                          RA_LENGTH_T uiSize,
                                          RA_FLAGS_T uiFlags,
                                          RA_LENGTH_T uBaseAlignment,
                                          const IMG_CHAR *pszAnnotation,
                                          RA_IMPORT *psImport)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*) hArenaHandle;
	PHYS_HEAP *psPhysHeap = psIMAData->pDLMHeap;
	PMB *pPMB;
	IMG_UINT64 uiPMBSizeBytes;
	PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(uiFlags);
	PVR_UNREFERENCED_PARAMETER(uBaseAlignment);
	PVR_ASSERT(psIMAData->uiLog2PMBSize != 0);
	/* Round up the allocation size to the nearest multiple of the
	 * DLM PMB size (1 << psIMAData->uiLog2PMBSize).
	 * If this is greater than DLM PMB size, a huge PMB will be imported. */
	uiPMBSizeBytes = PVR_ALIGN(uiSize, IMG_UINT64_C(1) << psIMAData->uiLog2PMBSize);

	eError = PhysHeapCreatePMB(psPhysHeap,
	                           uiPMBSizeBytes,
	                           pszAnnotation,
	                           &pPMB,
	                           &psImport->base,
	                           &psImport->uSize);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysHeapCreatePMB");

	psImport->hPriv = pPMB;

	return PVRSRV_OK;
}

/* When uiSize <= DLM PMB size, this returns a single PMB, otherwise
 * it requests multiple PMBs from the connected DLM heap.
 * The PMBs will be RA_Free'd once there are no busy segments inside of the span.
 * This will trigger the normal imp_free callback.
 * This strategy relies on RA_POLICY_ALLOC_ALLOW_NONCONTIG
 * and that all spans in the IMA heap are all the same size of 1 PMB, otherwise
 * we might add a span that is not used, invalidating the invariants of the RA. */
static PVRSRV_ERROR IMAImportDLMAllocMulti(RA_PERARENA_HANDLE hArenaHandle,
                                           RA_LENGTH_T uiSize,
                                           RA_FLAGS_T uiFlags,
                                           RA_LENGTH_T uBaseAlignment,
                                           const IMG_CHAR *pszAnnotation,
                                           IMG_UINT32 *puiImportsCount,
                                           RA_IMPORT **ppsImports)
{
	PVRSRV_ERROR eError;
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*) hArenaHandle;
	PHYS_HEAP *psPhysHeap = psIMAData->pDLMHeap;

	IMG_UINT32 i;
	IMG_UINT32 uiPMBCount;

	PVR_UNREFERENCED_PARAMETER(uiFlags);
	PVR_UNREFERENCED_PARAMETER(uBaseAlignment);
	PVR_ASSERT(psIMAData->uiLog2PMBSize != 0);

	/* Round up the allocation size to the nearest multiple of the
	 * DLM PMB size (1 << psIMAData->uiLog2PMBSize). */
	uiSize = PVR_ALIGN(uiSize, IMG_UINT64_C(1) << psIMAData->uiLog2PMBSize);
	/* Get the number of PMBs required to make the allocation. */
	uiPMBCount = uiSize >> psIMAData->uiLog2PMBSize;

	/* Try use the provided ppsImports array instead of allocating a new array
	 * if the array is large enough to hold all the new PMBs. */
	if (uiPMBCount > *puiImportsCount)
	{
		*ppsImports = OSAllocMem(uiPMBCount * sizeof(**ppsImports));
		PVR_LOG_RETURN_IF_NOMEM(ppsImports, "ppsImports");
	}
	*puiImportsCount = uiPMBCount;

	/* Create the PMB that will be become the spans. */
	for (i = 0; i < uiPMBCount; i++)
	{
		RA_IMPORT *psImport = &(*ppsImports)[i];
		eError = PhysHeapCreatePMB(psPhysHeap,
		                           IMG_UINT64_C(1) << psIMAData->uiLog2PMBSize,
		                           pszAnnotation,
		                           (PMB**) &psImport->hPriv,
		                           &psImport->base,
		                           &psImport->uSize);
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapCreatePMB", err_FreePMBs);
	}

	return PVRSRV_OK;

/* Free i PMBs from the array. */
err_FreePMBs:
	uiPMBCount = i;
	for (i = 0; i < uiPMBCount; i++)
	{
		PMBDestroy((*ppsImports)[i].hPriv);
	}
	return eError;
}

static void IMAImportDLMFree(RA_PERARENA_HANDLE hArenaHandle,
                             RA_BASE_T uiBase,
                             RA_PERISPAN_HANDLE hPriv)
{
	PMB *pPMB = (PMB*) hPriv;
	PVR_ASSERT(pPMB != NULL);

	PVR_UNREFERENCED_PARAMETER(hArenaHandle);
	PVR_UNREFERENCED_PARAMETER(uiBase);

	PMBDestroy(pPMB);
}

static void
FreeReservedMemory(PHYSMEM_IMA_DATA *psIMAData)
{
	IMG_UINT32 i;
	if (psIMAData->ppsReservedPMBs)
	{
		for (i = 0; i < psIMAData->uiReservedPMBs; i++)
		{
			/* The PMBs will be RA_Free'd by RA_Delete,
			 * so it doesn't need to be done manually */
			PMBDestroy(psIMAData->ppsReservedPMBs[i]);
		}
		OSFreeMem(psIMAData->ppsReservedPMBs);
	}
}

static PVRSRV_ERROR
AllocateReservedMemory(PHYSMEM_IMA_DATA *psIMAData,
                       PHYS_HEAP_POLICY uiPolicy,
                       IMG_UINT32 uiReservedPMBs)
{
	PVRSRV_ERROR eError;
	IMG_BOOL bSuccess;
	IMG_UINT32 i;

	IMG_UINT64 uiPMBSize = IMG_UINT64_C(1) << psIMAData->uiLog2PMBSize;
	IMG_UINT32 uiPMBCount = uiReservedPMBs;

	RA_BASE_T uiBase;
	RA_LENGTH_T uiReserveActSize;

	if (uiReservedPMBs == 0)
	{
		psIMAData->uiReservedPMBs = 0;
		psIMAData->ppsReservedPMBs = NULL;
		return PVRSRV_OK;
	}

	/* If we do not support non-contiguous, we can allocate a single huge PMB.
	 * We cannot do this for non-contiguous as IMAImportDLMAllocMulti requires all spans in the IMA-RA
	 * to be exactly the same: 1 << psIMAData->uiLog2PMBSize. */
	if ((uiPolicy & PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG_MASK) != PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG)
	{
		uiPMBSize *= uiPMBCount;
		uiPMBCount = 1;
	}

	/* Count the number of PMBs that have been successfully created. */
	psIMAData->uiReservedPMBs = 0;
	psIMAData->ppsReservedPMBs = OSAllocMem(uiPMBCount * sizeof(*psIMAData->ppsReservedPMBs));
	PVR_LOG_RETURN_IF_NOMEM(psIMAData->ppsReservedPMBs, "OSAllocMem");

	for (i = 0; i < uiPMBCount; i++)
	{
		eError = PhysHeapCreatePMB(psIMAData->pDLMHeap,
		                           uiPMBSize,
		                           "PMB Reserved",
		                           &psIMAData->ppsReservedPMBs[i],
		                           &uiBase,
		                           &uiReserveActSize);
		PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapCreatePMB", err_FreeReserved);

		psIMAData->uiReservedPMBs++;

		bSuccess = RA_Add(psIMAData->psRA,
		                  uiBase,
		                  uiReserveActSize,
		                  0,
		                  (RA_PERISPAN_HANDLE) psIMAData->ppsReservedPMBs[i]);
		PVR_LOG_GOTO_IF_FALSE(bSuccess, "RA_Add", err_AddFailed);
	}

	return PVRSRV_OK;

err_AddFailed:
    eError = PVRSRV_ERROR_OUT_OF_MEMORY;

err_FreeReserved:
	FreeReservedMemory(psIMAData);
	return eError;
}

static PVRSRV_ERROR
CreateIMAArena(PHYSMEM_IMA_DATA *psIMAData,
               IMG_CHAR *pszLabel,
               PHYS_HEAP_POLICY uiPolicy,
               IMG_UINT32 uiReservedPMBs)
{
	/* In practice an IMA heap only differs from LMA in the fact it can import more memory
	 * when it has expended its current extent. */

	/* If non contiguous mapping is available then we should allow that for an IMA heap.*/
	IMG_UINT32 ui32RAPolicy =
	    ((uiPolicy & PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG_MASK) == PHYS_HEAP_POLICY_ALLOC_ALLOW_NONCONTIG)
	    ? RA_POLICY_ALLOC_ALLOW_NONCONTIG : RA_POLICY_DEFAULT;
	PVRSRV_ERROR eError;

	PVR_ASSERT(psIMAData != NULL);

	if ((ui32RAPolicy & RA_POLICY_ALLOC_ALLOW_NONCONTIG_MASK) == RA_POLICY_ALLOC_ALLOW_NONCONTIG)
	{
		psIMAData->psRA = RA_CreateMulti(pszLabel,
		                                 OSGetPageShift(),
		                                 RA_LOCKCLASS_1,
		                                 IMAImportDLMAllocMulti,
		                                 IMAImportDLMFree,
		                                 psIMAData,
		                                 ui32RAPolicy);
	}
	else
	{
		psIMAData->psRA = RA_Create(pszLabel,
		                            OSGetPageShift(),
		                            RA_LOCKCLASS_1,
		                            IMAImportDLMAllocHuge,
		                            IMAImportDLMFree,
		                            psIMAData,
		                            ui32RAPolicy);
	}
	PVR_LOG_RETURN_IF_NOMEM(psIMAData->psRA, "RA_Create");

	eError = AllocateReservedMemory(psIMAData, uiPolicy, uiReservedPMBs);
	PVR_LOG_GOTO_IF_ERROR(eError, "AllocateReservedMemory", err_ra_free);

	return PVRSRV_OK;

err_ra_free:
	RA_Delete(psIMAData->psRA);
	psIMAData->psRA = NULL;
	return eError;
}

static void
DestroyIMAArena(PHYSMEM_IMA_DATA *psIMAData)
{
	PVR_ASSERT(psIMAData != NULL);

	/* Locked imports should be implicitly freed has a part of
	 * the RA_Delete assuming there are no allocations remaining on
	 * the import
	 */
	FreeReservedMemory(psIMAData);

	/* Remove RAs and RA names for local card memory */
	if (psIMAData->psRA)
	{
		RA_Delete(psIMAData->psRA);
		psIMAData->psRA = NULL;
	}
}

static void
IMADestroyImplData(PHEAP_IMPL_DATA pvImplData)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)pvImplData;

	DestroyIMAArena(pvImplData);

	OSFreeMem(psIMAData);
}

static PVRSRV_ERROR
PhysmemGetArenaIMA(PHYS_HEAP *psPhysHeap,
                   RA_ARENA **ppsArena)
{
	PHYSMEM_IMA_DATA *psIMAData = (PHYSMEM_IMA_DATA*)PhysHeapGetImplData(psPhysHeap);

	PVR_LOG_RETURN_IF_FALSE(psIMAData != NULL, "psIMAData", PVRSRV_ERROR_NOT_IMPLEMENTED);

	*ppsArena = psIMAData->psRA;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
IMAPhyContigPagesAlloc(PHYS_HEAP *psPhysHeap,
                       size_t uiSize,
                       PG_HANDLE *psMemHandle,
                       IMG_DEV_PHYADDR *psDevPAddr,
                       IMG_PID uiPid)
{
	PVRSRV_ERROR eError;

	RA_ARENA *pArena;
	IMG_UINT32 ui32Log2NumPages = 0;
	PVRSRV_DEVICE_NODE *psDevNode = PhysHeapDeviceNode(psPhysHeap);

	eError = PhysmemGetArenaIMA(psPhysHeap, &pArena);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemGetArenaLocalMem");

	PVR_ASSERT(uiSize != 0);
	ui32Log2NumPages = OSGetOrder(uiSize);
	uiSize = (1 << ui32Log2NumPages) * OSGetPageSize();

	eError = RAMemDoPhyContigPagesAlloc(pArena, uiSize, psDevNode, psMemHandle,
	                                    psDevPAddr, uiPid);
	PVR_LOG_IF_ERROR(eError, "LocalDoPhyContigPagesAlloc");

	return eError;
}

static void
IMAPhyContigPagesFree(PHYS_HEAP *psPhysHeap,
                      PG_HANDLE *psMemHandle)
{
	RA_ARENA	*pArena;

	PhysmemGetArenaIMA(psPhysHeap, &pArena);

	RAMemDoPhyContigPagesFree(pArena,
	                          psMemHandle);
}

static PVRSRV_ERROR
IMAPhysmemNewRAMemRamBackedPMR(PHYS_HEAP *psPhysHeap,
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

	eError = PhysmemGetArenaIMA(psPhysHeap, &pArena);
	PVR_LOG_RETURN_IF_ERROR(eError, "PhysmemGetArenaIMA");

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

static PHEAP_IMPL_FUNCS _sPHEAPImplFuncsIMA =
{
	.pfnDestroyData = &IMADestroyImplData,
	.pfnGetDevPAddr = &IMAGetDevPAddr,
	.pfnGetCPUPAddr = &IMAGetCPUPAddr,
	.pfnGetSize = &IMAGetSize,
	.pfnGetPageShift = &RAMemGetPageShift,
	.pfnGetFactoryMemStats = &IMAPhysmemGetRAMemRamMemStats,
	.pfnGetHeapSpansStringIter = &IMAGetHeapSpansStringIter,
	.pfnGetHeapDLMBacking = &IMAGetHeapDLMBacking,
	.pfnCreatePMR = &IMAPhysmemNewRAMemRamBackedPMR,
	.pfnPagesAlloc = &IMAPhyContigPagesAlloc,
	.pfnPagesFree = &IMAPhyContigPagesFree,
	.pfnPagesMap = &RAMemPhyContigPagesMap,
	.pfnPagesUnMap = &RAMemPhyContigPagesUnmap,
	.pfnPagesClean = &RAMemPhyContigPagesClean,
};

PVRSRV_ERROR
PhysmemCreateHeapIMA(PVRSRV_DEVICE_NODE *psDevNode,
                     PHYS_HEAP_POLICY uiPolicy,
                     PHYS_HEAP_CONFIG *psConfig,
                     IMG_CHAR *pszLabel,
                     PHYS_HEAP *psDLMHeap,
                     IMG_UINT32 uiLog2PMBSize,
                     PHYS_HEAP **ppsPhysHeap)
{
	PHYSMEM_IMA_DATA *psIMAData;
	PHYS_HEAP *psPhysHeap;
	IMG_UINT32 uiPMBStartingMultiple = psConfig->uConfig.sIMA.ui32PMBStartingMultiple;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode != NULL, "psDevNode");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psConfig != NULL, "psConfig");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pszLabel != NULL, "pszLabel");

	PVR_ASSERT(psConfig->eType == PHYS_HEAP_TYPE_IMA);

	psIMAData = OSAllocMem(sizeof(*psIMAData));
	PVR_LOG_RETURN_IF_NOMEM(psIMAData, "OSAllocMem");

	eError = PhysHeapCreate(psDevNode,
							psConfig,
							uiPolicy,
							(PHEAP_IMPL_DATA)psIMAData,
							&_sPHEAPImplFuncsIMA,
							&psPhysHeap);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapCreate", err_free_ima_data);

	psIMAData->psPhysHeap = psPhysHeap;
	psIMAData->pDLMHeap = psDLMHeap;
	psIMAData->uiLog2PMBSize = uiLog2PMBSize;

	eError = CreateIMAArena(psIMAData,
	                        pszLabel,
	                        uiPolicy,
	                        uiPMBStartingMultiple);
	PVR_LOG_GOTO_IF_ERROR(eError, "CreateIMAArena", err_free_physheap);

	if (ppsPhysHeap != NULL)
	{
		*ppsPhysHeap = psPhysHeap;
	}

	return PVRSRV_OK;

err_free_physheap:
	PhysHeapDestroy(psPhysHeap);
err_free_ima_data:
	OSFreeMem(psIMAData);
	return eError;
}
