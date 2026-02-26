/*************************************************************************/ /*!
@File           dma_km.c
@Title          kernel side of dma transfer scheduling
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements server side code for allowing DMA transfers between
                cpu and device memory.
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
#if defined(__linux__)
#include <linux/version.h>
#include <linux/uaccess.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#endif

#include "pmr.h"
#include "log2.h"
#include "device.h"
#include "pvrsrv.h"
#include "osfunc.h"
#include "dma_km.h"
#include "pvr_debug.h"
#include "lock_types.h"
#include "allocmem.h"
#include "process_stats.h"
#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO) && defined(DEBUG)
#include "ri_server.h"
#endif
#include "devicemem.h"
#include "pvrsrv_apphint.h"
#include "pvrsrv_sync_server.h"
#include "km_apphint_defs.h"
#include "di_server.h"
#include "dma_flags.h"

/* This header must always be included last */
#if defined(__linux__)
#include "kernel_compatibility.h"
#endif

typedef struct _SERVER_CLEANUP_DATA_
{
	PVRSRV_DEVICE_NODE *psDevNode;
	CONNECTION_DATA *psConnection;
	IMG_UINT32 uiNumDMA;
	IMG_UINT32 uiCount;
	SYNC_TIMELINE_OBJ sTimelineObject;
	void* pvChan;
	PMR** ppsPMR;
} SERVER_CLEANUP_DATA;

#if !defined(NO_HARDWARE)
static void Cleanup(void* pvCleanupData, IMG_BOOL bAdvanceTimeline)
{
	IMG_UINT i;
	PVRSRV_ERROR eError;
	SERVER_CLEANUP_DATA* psCleanupData = (SERVER_CLEANUP_DATA*) pvCleanupData;

#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR, "Server Cleanup thread entry (%p)", pvCleanupData));
#endif

	for (i=0; i<psCleanupData->uiCount; i++)
	{
		eError = PMRUnlockSysPhysAddresses(psCleanupData->ppsPMR[i]);
		PVR_LOG_IF_ERROR(eError, "PMRUnlockSysPhysAddresses");
	}

	/* Advance timeline */
	if (psCleanupData->sTimelineObject.pvTlObj && bAdvanceTimeline)
	{
		eError = SyncSWTimelineAdvanceKM(psCleanupData->psDevNode, &psCleanupData->sTimelineObject);
		PVR_LOG_IF_ERROR(eError, "SyncSWTimelineAdvanceKM");
		eError = SyncSWTimelineReleaseKM(&psCleanupData->sTimelineObject);
		PVR_LOG_IF_ERROR(eError, "SyncSWTimelineReleaseKM");
	}

	OSAtomicDecrement(&psCleanupData->psConnection->ui32NumDmaTransfersInFlight);
#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR, "Decremented to %d", OSAtomicRead(&psCleanupData->psConnection->ui32NumDmaTransfersInFlight)));
#endif
	eError = OSEventObjectSignal(psCleanupData->psConnection->hDmaEventObject);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: OSEventObjectSignal failed: %s",
		        __func__, PVRSRVGetErrorString(eError)));
	}


	OSFreeMem(psCleanupData->ppsPMR);
	OSFreeMem(psCleanupData);
}
#endif /* !defined(NO_HARDWARE) */

IMG_EXPORT PVRSRV_ERROR
PVRSRVInitialiseDMA(PVRSRV_DEVICE_NODE *psDeviceNode, CONNECTION_DATA *psConnectionData)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDeviceNode->psDevConfig;
	PVRSRV_ERROR eError;

	if (!psDevConfig->bHasDma)
	{
		return PVRSRV_OK;
	}

	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevConfig->pfnSlaveDMAGetChan, "pfnSlaveDMAGetChan");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevConfig->pfnSlaveDMAFreeChan, "pfnSlaveDMAFreeChan");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevConfig->pszDmaTxChanName, "pszDmaTxChanName");
	PVR_LOG_RETURN_IF_INVALID_PARAM(psDevConfig->pszDmaRxChanName, "pszDmaRxChanName");

	eError = OSEventObjectCreate("Dma transfer cleanup event object",
								 &psConnectionData->hDmaEventObject);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", dma_init_error1);

	OSLockAcquire(psDeviceNode->hConnectionsLock);

	if (psDeviceNode->ui32RefCountDMA == 0)
	{
		psDeviceNode->hDmaTxChan =
			psDevConfig->pfnSlaveDMAGetChan(psDevConfig,
											psDevConfig->pszDmaTxChanName);
		if (!psDeviceNode->hDmaTxChan)
		{
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_RESOURCE_UNAVAILABLE, dma_init_error2);
		}
		psDeviceNode->hDmaRxChan =
			psDevConfig->pfnSlaveDMAGetChan(psDevConfig,
											psDevConfig->pszDmaRxChanName);
		if (!psDeviceNode->hDmaRxChan)
		{
			PVR_GOTO_WITH_ERROR(eError, PVRSRV_ERROR_RESOURCE_UNAVAILABLE, dma_init_error3);
		}

		eError = OSLockCreate(&psDeviceNode->hDmaTxLock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(hDmaTxLock)", dma_init_error_txlock);

		eError = OSLockCreate(&psDeviceNode->hDmaRxLock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate(hDmaRxLock)", dma_init_error_rxlock);
	}

	psDeviceNode->ui32RefCountDMA++;

	OSLockRelease(psDeviceNode->hConnectionsLock);

	OSAtomicWrite(&psConnectionData->ui32NumDmaTransfersInFlight, 0);
	psConnectionData->bAcceptDmaRequests = IMG_TRUE;

	return PVRSRV_OK;

dma_init_error_rxlock:
	OSLockDestroy(psDeviceNode->hDmaTxLock);
dma_init_error_txlock:
	psDevConfig->pfnSlaveDMAFreeChan(psDevConfig, psDeviceNode->hDmaRxChan);
dma_init_error3:
	psDevConfig->pfnSlaveDMAFreeChan(psDevConfig, psDeviceNode->hDmaTxChan);
dma_init_error2:
	OSLockRelease(psDeviceNode->hConnectionsLock);
	OSEventObjectDestroy(psConnectionData->hDmaEventObject);
dma_init_error1:
	return eError;
}

static void WaitForOutstandingDma(CONNECTION_DATA *psConnectionData)
{

	PVRSRV_ERROR eError;
	IMG_HANDLE hEvent;
	IMG_UINT32 ui32Tries = 100;

#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR,
					"Waiting on %d DMA transfers in flight...", OSAtomicRead(&psConnectionData->ui32NumDmaTransfersInFlight)));
#endif

	eError = OSEventObjectOpen(psConnectionData->hDmaEventObject, &hEvent);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to open event object", __func__));
		return;
	}

	while (OSAtomicRead(&psConnectionData->ui32NumDmaTransfersInFlight) != 0)
	{
		/*
		#define DMA_TRANSFER_TIMEOUT_US (5000000ULL)

		This currently doesn't work properly. Wait time is not as requested.
		Using OSSleepms instead

		OSEventObjectWaitKernel(hEvent, DMA_TRANSFER_TIMEOUT_US);
		*/
		OSSleepms(50);
		if (!ui32Tries)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Timeout while waiting on outstanding DMA transfers!", __func__));
			break;
		}

		ui32Tries--;
	}

	OSEventObjectClose(hEvent);
}

IMG_EXPORT void
PVRSRVDeInitialiseDMA(PVRSRV_DEVICE_NODE *psDeviceNode, CONNECTION_DATA *psConnectionData)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDeviceNode->psDevConfig;

	if (!psDevConfig->bHasDma)
	{
		return;
	}

	OSLockAcquire(psDeviceNode->hDmaTxLock);
	OSLockAcquire(psDeviceNode->hDmaRxLock);

	psConnectionData->bAcceptDmaRequests = IMG_FALSE;

	OSLockRelease(psDeviceNode->hDmaRxLock);
	OSLockRelease(psDeviceNode->hDmaTxLock);

	WaitForOutstandingDma(psConnectionData);

	OSLockAcquire(psDeviceNode->hConnectionsLock);

	if (psDeviceNode->ui32RefCountDMA == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid ref count (%d)", __func__, psDeviceNode->ui32RefCountDMA));
		OSLockRelease(psDeviceNode->hConnectionsLock);
		return;
	}

	if (--psDeviceNode->ui32RefCountDMA == 0)
	{
		if (psDeviceNode->hDmaRxLock != NULL)
		{
			OSLockDestroy(psDeviceNode->hDmaRxLock);
		}

		if (psDeviceNode->hDmaTxLock != NULL)
		{
			OSLockDestroy(psDeviceNode->hDmaTxLock);
		}

		psDevConfig->pfnSlaveDMAFreeChan(psDevConfig, psDeviceNode->hDmaRxChan);
		psDevConfig->pfnSlaveDMAFreeChan(psDevConfig, psDeviceNode->hDmaTxChan);
	}

	OSLockRelease(psDeviceNode->hConnectionsLock);

	OSEventObjectDestroy(psConnectionData->hDmaEventObject);
}

IMG_EXPORT PVRSRV_ERROR
DmaDeviceParams(CONNECTION_DATA *psConnection,
				PVRSRV_DEVICE_NODE *psDevNode,
				IMG_UINT32 *ui32DmaBuffAlign,
				IMG_UINT32 *ui32DmaTransferMult)
{
	PVRSRV_DEVICE_CONFIG *psDevConfig = psDevNode->psDevConfig;

	*ui32DmaBuffAlign = psDevConfig->ui32DmaAlignment;
	*ui32DmaTransferMult = psDevConfig->ui32DmaTransferUnit;

	 return PVRSRV_OK;
}

IMG_EXPORT PVRSRV_ERROR
DmaSparseMappingTable(PMR *psPMR,
					  IMG_DEVMEM_OFFSET_T uiOffset,
					  IMG_UINT32 ui32SizeInPages,
					  IMG_BOOL *pbTable)
{
		PVRSRV_ERROR eError = PVRSRV_OK;
		IMG_DEV_PHYADDR *psDevPhyAddr;
		IMG_BOOL *pbValid;

		psDevPhyAddr = OSAllocZMem(ui32SizeInPages * sizeof(IMG_CPU_PHYADDR));
		PVR_LOG_GOTO_IF_NOMEM(psDevPhyAddr, eError, err1);

		pbValid = OSAllocZMem(ui32SizeInPages * sizeof(IMG_BOOL));
		PVR_LOG_GOTO_IF_NOMEM(pbValid, eError, err2);

		eError = PMRLockSysPhysAddresses(psPMR);
		PVR_LOG_GOTO_IF_ERROR(eError, "PMRLockSysPhysAddresses", err3);

		eError = PMR_DevPhysAddr(psPMR,
								 OSGetPageShift(),
								 ui32SizeInPages,
								 uiOffset,
								 psDevPhyAddr,
								 pbValid,
								 CPU_USE);
		PVR_LOG_GOTO_IF_ERROR(eError, "PMR_DevPhysAddr", err3);

		PMRUnlockSysPhysAddresses(psPMR);

		memcpy(pbTable, pbValid, ui32SizeInPages * sizeof(IMG_BOOL));

err3:
		OSFreeMem(pbValid);
err2:
		OSFreeMem(psDevPhyAddr);
err1:
		return eError;
}

IMG_EXPORT PVRSRV_ERROR
DmaTransfer(CONNECTION_DATA *psConnection,
		    PVRSRV_DEVICE_NODE *psDevNode,
			IMG_UINT32 uiNumDMAs,
			PMR** ppsPMR,
			IMG_UINT64 *puiAddress,
			IMG_DEVMEM_OFFSET_T *puiOffset,
			IMG_DEVMEM_SIZE_T *puiSize,
			IMG_UINT32 uiFlags,
			PVRSRV_TIMELINE iUpdateFenceTimeline)
{

	PVRSRV_ERROR eError = PVRSRV_OK;
#if defined(NO_HARDWARE)
	/* On nohw the kernel call just advances the timeline to signal completion */

	SYNC_TIMELINE_OBJ sSwTimeline = {NULL, PVRSRV_NO_TIMELINE};

	if (iUpdateFenceTimeline != PVRSRV_NO_TIMELINE)
	{
		eError = SyncSWGetTimelineObj(iUpdateFenceTimeline, &sSwTimeline);
		PVR_LOG_RETURN_IF_ERROR(eError, "SyncSWGetTimelineObj");

		eError = SyncSWTimelineAdvanceKM(psDevNode, &sSwTimeline);
		PVR_LOG_RETURN_IF_ERROR(eError, "SyncSWTimelineAdvanceKM");

		eError = SyncSWTimelineReleaseKM(&sSwTimeline);
		PVR_LOG_RETURN_IF_ERROR(eError, "SyncSWTimelineReleaseKM");
	}

	return PVRSRV_OK;

#else
	IMG_DEV_PHYADDR *psDevPhyAddr;
	IMG_DMA_ADDR *psDmaAddr;
	IMG_BOOL *pbValid;
	IMG_UINT32 i;
	PVRSRV_DEVICE_CONFIG* psDevConfig = psDevNode->psDevConfig;
	void* pvChan = NULL;
	SERVER_CLEANUP_DATA* psServerData;
	void*  pvOSData;
	POS_LOCK hChanLock = uiFlags & (DMA_FLAG_MEM_TO_DEV) ? psDevNode->hDmaTxLock : psDevNode->hDmaRxLock;

	for (i=0; i<uiNumDMAs; i++)
	{
		PMR* psPMR = ppsPMR[i];
		PMR_FLAGS_T uiPMRFlags = PMR_Flags(psPMR);
		IMG_BOOL bIsPrivate = PhysHeapGetFlags(PMR_PhysHeap(psPMR)) & PHYS_HEAP_USAGE_GPU_PRIVATE;
		IMG_BOOL bWrite = uiFlags & DMA_FLAG_MEM_TO_DEV;

		if (PMR_GetType(psPMR) == PMR_TYPE_OSMEM)
		{
			PVR_LOG_GOTO_WITH_ERROR("PMR is type OSMEM, expected device memory.", eError, PVRSRV_ERROR_NOT_SUPPORTED, error_return);
		}

		if (!bIsPrivate)
		{
			if (( bWrite && !PVRSRV_CHECK_CPU_WRITEABLE(uiPMRFlags)) ||
			    (!bWrite && !PVRSRV_CHECK_CPU_READABLE(uiPMRFlags)))
			{
				PVR_LOG_GOTO_WITH_ERROR("Incorrect CPU access parameters on PMR", eError, PVRSRV_ERROR_INVALID_PARAMS, error_return);
			}
		}
		else
		{
			/* Treat GPU private PMRs differently since they can't be mapped by the client.
			   Check access permissions based on GPU flags. */
			if (( bWrite && !PVRSRV_CHECK_GPU_WRITEABLE(uiPMRFlags)) ||
			    (!bWrite && !PVRSRV_CHECK_GPU_READABLE(uiPMRFlags))  ||
			     uiPMRFlags & PVRSRV_MEMALLOCFLAG_DEVICE_FLAG(PMMETA_PROTECT))
			{
				PVR_LOG_GOTO_WITH_ERROR("Incorrect GPU access parameters on PMR", eError, PVRSRV_ERROR_INVALID_PARAMS, error_return);
			}
		}
	}

	OSLockAcquire(hChanLock);

	if (!psConnection->bAcceptDmaRequests)
	{
		OSLockRelease(hChanLock);
		return PVRSRV_OK;
	}

	OSAtomicIncrement(&psConnection->ui32NumDmaTransfersInFlight);
#if defined(DMA_VERBOSE)
	PVR_DPF((PVR_DBG_ERROR, "Incremented to %d", OSAtomicRead(&psConnection->ui32NumDmaTransfersInFlight)));
#endif
	psServerData = OSAllocZMem(sizeof(SERVER_CLEANUP_DATA));
	PVR_LOG_GOTO_IF_NOMEM(psServerData, eError, e0);

	pvChan = uiFlags & (DMA_FLAG_MEM_TO_DEV) ? psDevNode->hDmaTxChan : psDevNode->hDmaRxChan;
	if (!pvChan)
	{
		eError = PVRSRV_ERROR_RESOURCE_UNAVAILABLE;
		PVR_LOG_GOTO_IF_ERROR(eError, "Error acquiring DMA channel", e1);
	}

	if (iUpdateFenceTimeline != PVRSRV_NO_TIMELINE)
	{
		eError = SyncSWGetTimelineObj(iUpdateFenceTimeline, &psServerData->sTimelineObject);
		PVR_LOG_GOTO_IF_ERROR(eError, "SyncSWGetTimelineObj", e1);
	}

	psServerData->uiCount = 0;
	psServerData->psDevNode = psDevNode;
	psServerData->psConnection = psConnection;
	psServerData->pvChan = pvChan;
	psServerData->ppsPMR = OSAllocZMem(sizeof(PMR*) * uiNumDMAs);
	PVR_LOG_GOTO_IF_NOMEM(psServerData->ppsPMR, eError, e2);

	eError = OSDmaAllocData(psDevNode, uiNumDMAs, &pvOSData);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSDmaAllocData failed", e3);

	for (i=0; i<uiNumDMAs; i++)
	{
		IMG_UINT32 ui32SizeInPages;
		IMG_UINT32 uiOffsetInPage = puiOffset[i] & (OSGetPageSize() - 1);
		ui32SizeInPages = (puiSize[i] + uiOffsetInPage + OSGetPageSize() - 1) >> OSGetPageShift();

		psDmaAddr = OSAllocZMem(ui32SizeInPages * sizeof(IMG_DMA_ADDR));
		PVR_LOG_GOTO_IF_NOMEM(psDmaAddr, eError, loop_e0);

		psDevPhyAddr = OSAllocZMem(ui32SizeInPages * sizeof(IMG_CPU_PHYADDR));
		PVR_LOG_GOTO_IF_NOMEM(psDevPhyAddr, eError, loop_e1);

		pbValid = OSAllocZMem(ui32SizeInPages * sizeof(IMG_BOOL));
		PVR_LOG_GOTO_IF_NOMEM(pbValid, eError, loop_e2);

		eError = PMRLockSysPhysAddresses(ppsPMR[i]);
		PVR_LOG_GOTO_IF_ERROR(eError, "PMRLockSysPhysAddresses", loop_e3);

		psServerData->ppsPMR[i] = ppsPMR[i];

		eError = PMR_DevPhysAddr(ppsPMR[i],
								 OSGetPageShift(),
								 ui32SizeInPages,
								 puiOffset[i],
								 psDevPhyAddr,
								 pbValid,
								 CPU_USE);
		PVR_LOG_GOTO_IF_ERROR(eError, "PMR_DevPhysAddr", loop_e4);

		psDevConfig->pfnDevPhysAddr2DmaAddr(psDevConfig,
											psDmaAddr,
											psDevPhyAddr,
											pbValid,
											ui32SizeInPages,
											PMR_IsSparse(ppsPMR[i]));

		if (!PMR_IsSparse(ppsPMR[i]))
		{
			eError = OSDmaPrepareTransfer(psDevNode,
										  pvChan,
										  &psDmaAddr[0], (IMG_UINT64*)puiAddress[i],
										  puiSize[i], (uiFlags & DMA_FLAG_MEM_TO_DEV), pvOSData,
										  psServerData, Cleanup, (i == 0));
			PVR_LOG_GOTO_IF_ERROR(eError, "OSDmaPrepareTransfer", loop_e4);
			psServerData->uiCount++;

		}
		else
		{
			eError = OSDmaPrepareTransferSparse(psDevNode, pvChan,
												psDmaAddr, pbValid,
												(IMG_UINT64*)puiAddress[i], puiSize[i],
												uiOffsetInPage, ui32SizeInPages,
												(uiFlags & DMA_FLAG_MEM_TO_DEV),
												pvOSData, psServerData,
												Cleanup, (i == 0));
			PVR_LOG_GOTO_IF_ERROR(eError, "OSDmaPrepareTransferSparse", loop_e4);
			psServerData->uiCount++;
		}

		OSFreeMem(pbValid);
		OSFreeMem(psDevPhyAddr);
		OSFreeMem(psDmaAddr);

		continue;

loop_e4:
		PMRUnlockSysPhysAddresses(ppsPMR[i]);
loop_e3:
		OSFreeMem(pbValid);
loop_e2:
		OSFreeMem(psDevPhyAddr);
loop_e1:
		OSFreeMem(psDmaAddr);
loop_e0:
		break;
	}

	if (psServerData->uiCount == uiNumDMAs)
	{
		OSDmaSubmitTransfer(psDevNode, pvOSData, pvChan, (uiFlags & DMA_FLAG_SYNCHRONOUS));
	}
	else
	{
		/* One of the transfers could not be programmed, roll back */
		OSDmaForceCleanup(psDevNode, pvChan, pvOSData, psServerData, Cleanup);
	}
	OSLockRelease(hChanLock);
	return eError;

e3:
	OSFreeMem(psServerData->ppsPMR);
e2:
	if (iUpdateFenceTimeline != PVRSRV_NO_TIMELINE)
	{
		SyncSWTimelineReleaseKM(&psServerData->sTimelineObject);
	}
e1:
	OSFreeMem(psServerData);
e0:
	OSLockRelease(hChanLock);
error_return:
	return eError;
#endif
}
