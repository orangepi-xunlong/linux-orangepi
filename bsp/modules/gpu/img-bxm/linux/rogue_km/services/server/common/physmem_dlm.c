/*************************************************************************/ /*!
@File           physmem_dlm.c
@Title          Dedicated Local Memory allocator for Physical Memory Blocks
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Part of the memory management. This module is responsible for
                implementing the function callbacks for Dedicated local memory.
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
#include "osfunc.h"
#include "physheap.h"
#include "physheap_config.h"
#include "device.h"
#include "physmem_dlm.h"

typedef struct PHYSMEM_DLM_DATA_TAG {
	RA_ARENA           *psRA;
	IMG_CPU_PHYADDR    sStartAddr;
	IMG_DEV_PHYADDR    sCardBase;
	IMG_UINT64         uiSize;
	IMG_UINT32         uiLog2PMBSize;
} PHYSMEM_DLM_DATA;

/* PMB (Physical Memory Block) */
struct _PMB_
{
	RA_ARENA *pArena;
	RA_BASE_T uiBase;
	RA_LENGTH_T uiSize;
	const IMG_CHAR *pszAnnotation;
};

/* PMBCreatePMB
 *
 * Creates a new PMB used to represent a block of memory
 * obtained from a DLM heap.
 */
static
PVRSRV_ERROR PMBCreatePMB(RA_ARENA *pArena,
                          RA_LENGTH_T uiSize,
                          RA_LENGTH_T uiAlignment,
                          const IMG_CHAR *pszAnnotation,
                          PMB **ppsPMB)
{
	PVRSRV_ERROR eError;
	PMB* psPMB = OSAllocMem(sizeof(*psPMB));
	PVR_LOG_GOTO_IF_NOMEM(psPMB, eError, error_Return);

	eError = RA_Alloc(pArena,
	                  uiSize,
	                  RA_NO_IMPORT_MULTIPLIER,
	                  0,                         /* No flags */
	                  uiAlignment,
	                  pszAnnotation,
	                  &psPMB->uiBase,
	                  &psPMB->uiSize,
	                  NULL);                     /* No private handle */
	PVR_LOG_GOTO_IF_ERROR(eError, "RA_Alloc", error_FreePMB);

	psPMB->pArena = pArena;
	psPMB->pszAnnotation = pszAnnotation;

	*ppsPMB = psPMB;

	return PVRSRV_OK;

error_FreePMB:
	OSFreeMem(psPMB);
error_Return:
	return eError;
}

void
PMBDestroy(PMB *psPMB)
{
	PVR_LOG_RETURN_VOID_IF_FALSE(psPMB, "psPMB NULL");

	RA_Free(psPMB->pArena, psPMB->uiBase);
	OSFreeMem(psPMB);
}


const IMG_CHAR *
PMBGetAnnotation(PMB *psPMB)
{
	if (psPMB == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"psPMB in %s",__func__));
		return "";
	}

	return psPMB->pszAnnotation;
}

/* DLM */

static void PFNGetLocalRamMemStats(PHEAP_IMPL_DATA pvImplData,
                                 IMG_UINT64 *pui64TotalSize,
                                 IMG_UINT64 *pui64FreeSize)
{
	RA_USAGE_STATS sRAUsageStats;
	PHYSMEM_DLM_DATA *psDLMData = (PHYSMEM_DLM_DATA*)pvImplData;
	PVR_LOG_RETURN_VOID_IF_FALSE(pvImplData, "pvImplData NULL");

	RA_Get_Usage_Stats(psDLMData->psRA, &sRAUsageStats);

	*pui64TotalSize = sRAUsageStats.ui64TotalArenaSize;
	*pui64FreeSize = sRAUsageStats.ui64FreeArenaSize;
}

/*
* This function will set the psDevPAddr to whatever the system layer
* has set it for the referenced heap.
* It will not fail if the psDevPAddr is invalid.
*/
static PVRSRV_ERROR
PFNGetDevPAddr(PHEAP_IMPL_DATA pvImplData,
			 IMG_DEV_PHYADDR *psDevPAddr)
{
	PHYSMEM_DLM_DATA *psDLMData = (PHYSMEM_DLM_DATA*)pvImplData;
	PVR_LOG_RETURN_IF_INVALID_PARAM(pvImplData != NULL, "pvImplData");

	*psDevPAddr = psDLMData->sCardBase;

	return PVRSRV_OK;
}

/*
* This function will set the psCpuPAddr to whatever the system layer
* has set it for the referenced heap.
* It will not fail if the psCpuPAddr is invalid.
*/
static PVRSRV_ERROR
PFNGetCPUPAddr(PHEAP_IMPL_DATA pvImplData,
			 IMG_CPU_PHYADDR *psCpuPAddr)
{
	PHYSMEM_DLM_DATA *psDLMData = (PHYSMEM_DLM_DATA*)pvImplData;
	PVR_LOG_RETURN_IF_INVALID_PARAM(pvImplData != NULL, "pvImplData");

	*psCpuPAddr = psDLMData->sStartAddr;

	return PVRSRV_OK;
}

static PVRSRV_ERROR
PFNGetSize(PHEAP_IMPL_DATA pvImplData,
		 IMG_UINT64 *puiSize)
{
	PHYSMEM_DLM_DATA *psDLMData = (PHYSMEM_DLM_DATA*)pvImplData;
	PVR_LOG_RETURN_IF_INVALID_PARAM(pvImplData != NULL, "pvImplData");

	*puiSize = psDLMData->uiSize;

	return PVRSRV_OK;
}

static IMG_UINT32
PFNGetPageShift(void)
{
	return PVRSRV_4K_PAGE_SIZE_ALIGNSHIFT;
}

static PVRSRV_ERROR
CreateArenas(PHYSMEM_DLM_DATA *psDLMData, IMG_CHAR *pszLabel, PHYS_HEAP_POLICY uiPolicy)
{
	psDLMData->psRA = RA_Create_With_Span(pszLabel,
	                             OSGetPageShift(),
	                             psDLMData->sStartAddr.uiAddr,
	                             psDLMData->sCardBase.uiAddr,
	                             psDLMData->uiSize,
	                             RA_POLICY_DEFAULT);
	PVR_LOG_RETURN_IF_NOMEM(psDLMData->psRA, "RA_Create_With_Span");

	return PVRSRV_OK;
}

static void
DestroyArenas(PHYSMEM_DLM_DATA *psDLMData)
{
	/* Remove RAs and RA names for dedicated local memory */
	if (psDLMData->psRA)
	{
		RA_Delete(psDLMData->psRA);
		psDLMData->psRA = NULL;
	}
}

static void
PFNDestroyImplData(PHEAP_IMPL_DATA pvImplData)
{
	PHYSMEM_DLM_DATA *psDLMData = (PHYSMEM_DLM_DATA*)pvImplData;
	PVR_LOG_RETURN_VOID_IF_FALSE(pvImplData, "pvImplData NULL");

	DestroyArenas(pvImplData);

	OSFreeMem(psDLMData);
}

static PVRSRV_ERROR
PFNPhysmemNewLocalRamBackedPMB(PHYS_HEAP *psPhysHeap,
                            IMG_DEVMEM_SIZE_T uiSize,
                            const IMG_CHAR *pszAnnotation,
                            PMB **ppPMBPtr,
                            RA_BASE_T *puiBase,
                            RA_LENGTH_T *puiSize)
{
	PHYSMEM_DLM_DATA *psDLMData;
	PVRSRV_ERROR eError;
	PMB* pPMB;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psPhysHeap != NULL, "psPhysHeap");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pszAnnotation != NULL, "pszAnnotation");
	PVR_LOG_RETURN_IF_INVALID_PARAM(ppPMBPtr != NULL, "ppPMBPtr");

	/* Check size is aligned to page size */
	if (uiSize & ((1 << PFNGetPageShift()) - 1ULL))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "%s: uiSize %" IMG_UINT64_FMTSPEC " is not aligned to page size: %u",
		         __func__,
		         uiSize,
		         1 << PFNGetPageShift()));
		return PVRSRV_ERROR_PMB_NOT_PAGE_MULTIPLE;
	}

	PVR_ASSERT(PhysHeapGetType(psPhysHeap) == PHYS_HEAP_TYPE_DLM);

	psDLMData = (PHYSMEM_DLM_DATA*)PhysHeapGetImplData(psPhysHeap);

	eError = PMBCreatePMB(psDLMData->psRA, uiSize, IMG_UINT64_C(1) << psDLMData->uiLog2PMBSize, pszAnnotation, &pPMB);
	PVR_LOG_GOTO_IF_ERROR(eError, "PMBCreatePMB", error_Return);

	*ppPMBPtr = pPMB;
	*puiBase = pPMB->uiBase;
	*puiSize = pPMB->uiSize;

	return PVRSRV_OK;

error_Return:
	return eError;
}

static PHEAP_IMPL_FUNCS _sPHEAPImplFuncs =
{
	.pfnDestroyData = &PFNDestroyImplData,
	.pfnGetDevPAddr = &PFNGetDevPAddr,
	.pfnGetCPUPAddr = &PFNGetCPUPAddr,
	.pfnGetSize = &PFNGetSize,
	.pfnGetPageShift = &PFNGetPageShift,
	.pfnGetFactoryMemStats = &PFNGetLocalRamMemStats,
	.pfnCreatePMB = &PFNPhysmemNewLocalRamBackedPMB,
};

PVRSRV_ERROR
PhysmemCreateHeapDLM(PVRSRV_DEVICE_NODE *psDevNode,
                     PHYS_HEAP_POLICY uiPolicy,
                     PHYS_HEAP_CONFIG *psConfig,
                     IMG_CHAR *pszLabel,
                     PHYS_HEAP **ppsPhysHeap)
{
	PHYSMEM_DLM_DATA *psDLMData;
	PHYS_HEAP *psPhysHeap;
	PVRSRV_ERROR eError;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevNode != NULL, "psDevNode");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psConfig != NULL, "psConfig");
	PVR_LOG_RETURN_IF_INVALID_PARAM(pszLabel != NULL, "pszLabel");

	PVR_ASSERT(psConfig->eType == PHYS_HEAP_TYPE_DLM);

	psDLMData = OSAllocMem(sizeof(*psDLMData));
	PVR_LOG_RETURN_IF_NOMEM(psDLMData, "OSAllocMem");

	psDLMData->sStartAddr = PhysHeapConfigGetStartAddr(psConfig);
	psDLMData->sCardBase = PhysHeapConfigGetCardBase(psConfig);
	psDLMData->uiSize = PhysHeapConfigGetSize(psConfig);
	psDLMData->uiLog2PMBSize = psConfig->uConfig.sDLM.ui32Log2PMBSize;

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDLMData->uiLog2PMBSize >= OSGetPageShift(), "ui32Log2PMBSize must be greater than or equal to OSPageSize");

	eError = PhysHeapCreate(psDevNode,
							psConfig,
							uiPolicy,
							(PHEAP_IMPL_DATA)psDLMData,
							&_sPHEAPImplFuncs,
							&psPhysHeap);
	PVR_LOG_GOTO_IF_ERROR(eError, "PhysHeapCreate", error_FreeDlmData);

	eError = CreateArenas(psDLMData, pszLabel, uiPolicy);
	PVR_LOG_GOTO_IF_ERROR(eError, "CreateArenas", error_FreePhysHeap);

	if (ppsPhysHeap != NULL)
	{
		*ppsPhysHeap = psPhysHeap;
	}

	return PVRSRV_OK;

error_FreePhysHeap:
	PhysHeapDestroy(psPhysHeap);
error_FreeDlmData:
	OSFreeMem(psDLMData);
	return eError;
}
