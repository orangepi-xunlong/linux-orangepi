/*************************************************************************/ /*!
@File
@Title          core services functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Main APIs for core services functions
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

#define CLEANUP_TYPE_STRINGS

#include "img_defs.h"
#include "rgxdebug_common.h"
#include "handle.h"

#include "pmr.h"
#include "connection_server.h"
#include "osconnection_server.h"
#include "pdump_km.h"
#include "ra.h"
#include "allocmem.h"
#include "pvrsrv.h"
#include "srvcore.h"
#include "services_km.h"
#include "pvrsrv_device.h"
#include "pvr_debug.h"
#include "debug_common.h"
#include "pvr_notifier.h"
#include "sync.h"
#include "sync_server.h"
#include "sync_checkpoint.h"
#include "sync_fallback_server.h"
#include "sync_checkpoint_init.h"
#include "devicemem.h"
#include "cache_km.h"
#include "info_page.h"
#include "info_page_defs.h"
#include "pvrsrv_bridge_init.h"
#include "devicemem_server.h"
#include "km_apphint_defs.h"
#include "di_server.h"
#include "di_impl_brg.h"
#include "htb_debug.h"
#include "dma_km.h"
#include "pmr.h"

#include "log2.h"

#include "lists.h"
#include "dllist.h"
#include "syscommon.h"
#include "sysvalidation.h"

#include "physmem_lma.h"
#include "physmem_osmem.h"
#include "physmem_hostmem.h"
#if defined(PVRSRV_PHYSMEM_CPUMAP_HISTORY)
#include "physmem_cpumap_history.h"
#endif

#include "tlintern.h"
#include "htbserver.h"
#include "rgxmulticore.h"


//#define MULTI_DEVICE_BRINGUP

#if defined(MULTI_DEVICE_BRINGUP)
#define MULTI_DEVICE_BRINGUP_DPF(msg, ...) PVR_DPF((PVR_DBG_MESSAGE, msg, __VA_ARGS__))
#else
#define MULTI_DEVICE_BRINGUP_DPF(msg, ...)
#endif

#if defined(SUPPORT_RGX)
#include "rgxinit.h"
#include "rgxhwperf.h"
#include "rgxfwutils.h"
#include "rgx_bridge_init.h"
#endif

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
#include "ri_server.h"
#endif

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
#include "process_stats.h"
#endif

#include "vz_vmm_pvz.h"

#include "devicemem_history_server.h"

#if defined(SUPPORT_LINUX_DVFS)
#include "pvr_dvfs_device.h"
#elif defined(SUPPORT_PDVFS)
#include "pvr_dvfs_common.h"
#endif

#if defined(SUPPORT_DISPLAY_CLASS)
#include "dc_server.h"
#endif

#include "rgx_options.h"
#include "srvinit.h"
#include "rgxutils.h"

#include "os_apphint.h"
#include "pvrsrv_apphint.h"

#include "pvrsrv_tlstreams.h"
#include "tlstream.h"

#if defined(SUPPORT_PHYSMEM_TEST) && !defined(INTEGRITY_OS) && !defined(__QNXNTO__)
#include "physmem_test.h"
#endif

#if defined(SUPPORT_GPUVIRT_VALIDATION)
#include "virt_validation_defs.h"
#endif

#if defined(__linux__)
#include "km_apphint.h"
#endif /* defined(__linux__) */

#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)
#define INFINITE_SLEEP_TIMEOUT 0ULL
#endif

/*! Wait 100ms before retrying deferred clean-up again */
#define CLEANUP_THREAD_WAIT_RETRY_TIMEOUT 100000ULL

/*! Wait 8hrs when no deferred clean-up required. Allows a poll several times
 * a day to check for any missed clean-up. */
#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)
#define CLEANUP_THREAD_WAIT_SLEEP_TIMEOUT INFINITE_SLEEP_TIMEOUT
#else
#define CLEANUP_THREAD_WAIT_SLEEP_TIMEOUT 28800000000ULL
#endif

/*! When unloading try a few times to free everything remaining on the list */
#define CLEANUP_THREAD_UNLOAD_RETRY 4

#define PVRSRV_TL_CTRL_STREAM_SIZE 4096

static PVRSRV_DATA	*gpsPVRSRVData;
static IMG_UINT32 g_ui32InitFlags;

/* mark which parts of Services were initialised */
#define		INIT_DATA_ENABLE_PDUMPINIT	0x1U

#define CLEANUP_QUEUE_TOTAL_DPF "QUEUED: %1.5d "
#define CLEANUP_QUEUE_TOTAL_DPF_MAX_SIZE sizeof(CLEANUP_QUEUE_TOTAL_DPF)+1

#define CLEANUP_STRING_SUMMARY_MAX_LEN (CLEANUP_QUEUE_TOTAL_DPF_MAX_SIZE+(PVRSRV_CLEANUP_TYPE_LAST * CLEANUP_TYPE_ITEM_DPF_MAX_SIZE))

static IMG_CHAR *_ConcatCleanupString(IMG_CHAR *cleanupString)
{
	IMG_UINT32 uiLoop;
	IMG_UINT32 strSize = CLEANUP_STRING_SUMMARY_MAX_LEN;

	PVRSRV_DATA *psPVRSRVData =  PVRSRVGetPVRSRVData();

	OSSNPrintf(cleanupString, CLEANUP_QUEUE_TOTAL_DPF_MAX_SIZE, CLEANUP_QUEUE_TOTAL_DPF,
				OSAtomicRead(&psPVRSRVData->i32NumCleanupItemsQueued));

	PVR_ASSERT(PVRSRV_CLEANUP_TYPE_CONNECTION == 1);

	for (uiLoop=PVRSRV_CLEANUP_TYPE_CONNECTION; (strSize > CLEANUP_TYPE_ITEM_DPF_MAX_SIZE) && (uiLoop < PVRSRV_CLEANUP_TYPE_LAST); uiLoop++)
	{
		IMG_CHAR acTempQueued[CLEANUP_TYPE_ITEM_DPF_MAX_SIZE];
		OSSNPrintf(acTempQueued, CLEANUP_TYPE_ITEM_DPF_MAX_SIZE, CLEANUP_TYPE_ITEM_DPF,
				PVRSRVGetCleanupName(uiLoop),
				OSAtomicRead(&psPVRSRVData->i32CleanupItemTypes[uiLoop]));
		OSStringLCat(cleanupString, acTempQueued, strSize);
	}

	return cleanupString;
}

/* Callback to dump info of cleanup thread in debug_dump */
static void CleanupThreadDumpInfo(DUMPDEBUG_PRINTF_FUNC* pfnDumpDebugPrintf,
                                  void *pvDumpDebugFile)
{
	IMG_CHAR acCleanupString[CLEANUP_STRING_SUMMARY_MAX_LEN];

	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();

	PVR_DUMPDEBUG_LOG("    Number of deferred cleanup items: %s", _ConcatCleanupString(acCleanupString));

	PVR_DUMPDEBUG_LOG("    Number of deferred cleanup items dropped after "
					  "retry limit reached : %d",
					  OSAtomicRead(&psPVRSRVData->i32NumCleanupItemsNotCompleted));
}

static void _CleanupThreadDecrementStats(PVRSRV_DATA *psPVRSRVData,
                                         PVRSRV_CLEANUP_TYPE eCleanupType)
{
#if defined(DEBUG)
	IMG_CHAR acCleanupString[CLEANUP_STRING_SUMMARY_MAX_LEN];
#endif

	PVR_ASSERT(OSAtomicRead(&psPVRSRVData->i32NumCleanupItemsQueued) >= 0);

	PVR_DPF((PVR_DBG_MESSAGE, "BEFORE REMOVING ----- %s", _ConcatCleanupString(acCleanupString)));

	OSAtomicDecrement(&psPVRSRVData->i32NumCleanupItemsQueued);

	if ((eCleanupType <= PVRSRV_CLEANUP_TYPE_UNDEF) || ((eCleanupType >= PVRSRV_CLEANUP_TYPE_LAST)))
	{
		PVR_DPF((PVR_DBG_ERROR, "Incorrect cleanup item passed: %d", eCleanupType));
	}
	else
	{
		PVR_ASSERT(OSAtomicRead(&psPVRSRVData->i32CleanupItemTypes[eCleanupType]) >= 0);
		OSAtomicDecrement(&psPVRSRVData->i32CleanupItemTypes[eCleanupType]);
	}

	PVR_DPF((PVR_DBG_MESSAGE, "AFTER REMOVING ----- %s", _ConcatCleanupString(acCleanupString)));
}

#if defined(DEBUG)
static void _CleanupThreadWorkListDump(PVRSRV_DATA *psPVRSRVData)
{
	DLLIST_NODE *psNode;
	DLLIST_NODE *psNextNode;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	OS_SPINLOCK_FLAGS uiFlags;
	char pszCleanupLog[128];

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	OSSpinLockAcquire(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);

	/* Iterate over all devices. */
	for (psDeviceNode = psPVRSRVData->psDeviceNodeList;
	     psDeviceNode != NULL;
	     psDeviceNode = psDeviceNode->psNext)
	{
		if (dllist_is_empty(&psDeviceNode->sCleanupThreadWorkList))
		{
			OSSNPrintf(pszCleanupLog, 128, "Dev_%p: CLEAN", psDeviceNode);
			PVR_LOG(("%s", pszCleanupLog));
			continue;
		}

		OSSNPrintf(pszCleanupLog, 128, "Dev_%p: TASKS", psDeviceNode);
		PVR_LOG(("%s", pszCleanupLog));

		/* Iterate over all cleanup items. */
		dllist_foreach_node(&psDeviceNode->sCleanupThreadWorkList, psNode, psNextNode)
		{
			PVRSRV_CLEANUP_THREAD_WORK *psData;

			psData = IMG_CONTAINER_OF(psNode, PVRSRV_CLEANUP_THREAD_WORK, sNode);

			PVR_ASSERT(psData != NULL);

			if ((psData->eCleanupType <= PVRSRV_CLEANUP_TYPE_UNDEF) ||
				((psData->eCleanupType >= PVRSRV_CLEANUP_TYPE_LAST)))
			{
				PVR_DPF((PVR_DBG_ERROR, "Incorrect cleanup item found: %d.", psData->eCleanupType));
				continue;
			}

			OSSNPrintf(pszCleanupLog, 128, "+ %p: type %u, depends-HW %s (%s:%d)",
					   psData,
					   psData->eCleanupType,
					   (psData->bDependsOnHW) ? "Yes" : " No",
					   psData->pszFun, psData->ui32LineNum);
			PVR_LOG(("%s", pszCleanupLog));
		}
	}

	OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);
}
#endif

/* Add work to the cleanup thread work list.
 * The work item will be executed by the cleanup thread
 */
#if defined(DEBUG)
void PVRSRVCleanupThreadAddWork_Debug(PVRSRV_DEVICE_NODE *psDeviceNode, PVRSRV_CLEANUP_THREAD_WORK *psData,
	const char *pszFun, const unsigned int ui32LineNum)
#else
void PVRSRVCleanupThreadAddWork_Int(PVRSRV_DEVICE_NODE *psDeviceNode, PVRSRV_CLEANUP_THREAD_WORK *psData)
#endif
{
#if defined(DEBUG)
	IMG_CHAR acCleanupString[CLEANUP_STRING_SUMMARY_MAX_LEN];
#endif

	PVRSRV_DATA *psPVRSRVData;
	PVRSRV_ERROR eError;
	OS_SPINLOCK_FLAGS uiFlags;

	PVR_ASSERT(psData != NULL);

	if ((psData->eCleanupType <= PVRSRV_CLEANUP_TYPE_UNDEF) ||
		((psData->eCleanupType >= PVRSRV_CLEANUP_TYPE_LAST)))
	{
		PVR_DPF((PVR_DBG_ERROR, "Incorrect cleanup item passed: %d.", psData->eCleanupType));
	}

	psPVRSRVData = PVRSRVGetPVRSRVData();

	PVR_DPF((PVR_DBG_MESSAGE, "AFTER QUEUEING ----- %s", _ConcatCleanupString(acCleanupString)));


	/* Cleanup thread work item added during device destructing:
	   1. bDependsOnHW: Item may use device resources. Not safe to clean.
	   2. !bDependsOnHW: Clean item immediately to prevent memory leak. */
	if (psPVRSRVData->bUnload
#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
		|| psPVRSRVData->eServicesState != PVRSRV_SERVICES_STATE_OK
#endif
		|| (psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING && !psData->bDependsOnHW))
	{
		CLEANUP_THREAD_FN pfnFree = psData->pfnFree;

		PVR_DPF((PVR_DBG_MESSAGE, "Cleanup thread has already quit: doing work immediately"));

		eError = pfnFree(psData->pvData);

		PVR_DPF((PVR_DBG_MESSAGE, "BEFORE ADDING ----- %s", _ConcatCleanupString(acCleanupString)));

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "Failed to free resource "
						"(callback " IMG_PFN_FMTSPEC "). "
						"Immediate free will not be retried.",
						pfnFree));
		}
	}
	else
	{
		/*
		 * Access psData before putting it in the work list.
		 * Cleanup thread will free psData after it is done with the cleanup.
		 * We should consider psData potentially freed after it was put into the worklist.
		 */

		OSAtomicIncrement(&psPVRSRVData->i32NumCleanupItemsQueued);
		OSAtomicIncrement(&psPVRSRVData->i32CleanupItemTypes[psData->eCleanupType]);
#if defined(DEBUG)
		psData->pszFun = pszFun;
		psData->ui32LineNum = ui32LineNum;
#endif

		/* add this work item to the list */
		OSSpinLockAcquire(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
		OSAtomicIncrement(&psDeviceNode->i32NumCleanupItems);
		dllist_add_to_tail(&psDeviceNode->sCleanupThreadWorkList, &psData->sNode);
		OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);

		/* signal the cleanup thread to ensure this item gets processed */
		eError = OSEventObjectSignal(psPVRSRVData->hCleanupEventObject);

		PVR_DPF((PVR_DBG_MESSAGE, "AFTER ADDING ----- %s", _ConcatCleanupString(acCleanupString)));
		PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
	}
}

void PVRSRVCleanupThreadWaitForDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(DEBUG)
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
#endif
	IMG_INT32 i32NumCleanupItems;

	PVR_ASSERT(psDeviceNode != NULL);

	if (gpsPVRSRVData->hCleanupThread == NULL)
	{
		return;
	}

	LOOP_UNTIL_TIMEOUT_US(OS_THREAD_DESTROY_TIMEOUT_US)
	{
		PVRSRV_ERROR eError;

		if (gpsPVRSRVData->hCleanupEventObject)
		{
			eError = OSEventObjectSignal(gpsPVRSRVData->hCleanupEventObject);
			PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
		}

		i32NumCleanupItems = OSAtomicRead(&psDeviceNode->i32NumCleanupItems);

		if (i32NumCleanupItems == 0)
		{
#if defined(DEBUG)
			OS_SPINLOCK_FLAGS uiFlags;

			OSSpinLockAcquire(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
			PVR_LOG_IF_FALSE(dllist_is_empty(&psDeviceNode->sCleanupThreadWorkList),
							 "Cleanup thread work list is not empty");

			OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
#endif
			break;
		}

		OSWaitus(OS_THREAD_DESTROY_TIMEOUT_US / OS_THREAD_DESTROY_RETRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT_US();

	if (i32NumCleanupItems != 0)
	{
		PVR_LOG(("Failed to flush device cleanup queue. %d items remain.", i32NumCleanupItems));
	}

#if defined(DEBUG)
	_CleanupThreadWorkListDump(psPVRSRVData);
#endif
}

static INLINE DLLIST_NODE *_CleanupThreadWorkListLast(PVRSRV_DATA *psPVRSRVData)
{
	DLLIST_NODE *psNode = NULL;
	PVRSRV_DEVICE_NODE *psDeviceNode;
	OS_SPINLOCK_FLAGS uiFlags;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	OSSpinLockAcquire(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);

	/* We treat the Host device node as the last node in the list. */
	psNode = dllist_get_prev_node(&psPVRSRVData->psHostMemDeviceNode->sCleanupThreadWorkList);
	if (psNode != NULL)
	{
		OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
		OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);
		return psNode;
	}

	/* Iterate over all devices and find the last node of the last device. */
	for (psDeviceNode = psPVRSRVData->psDeviceNodeList;
	     psDeviceNode != NULL;
	     psDeviceNode = psDeviceNode->psNext)
	{
		DLLIST_NODE *psCurrNode = dllist_get_prev_node(&psDeviceNode->sCleanupThreadWorkList);
		if (psCurrNode != NULL)
		{
			psNode = psCurrNode;
		}
	}

	OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	return psNode;
}

/* Pop an item from the head of the cleanup thread work lists.
 *
 * This pops an item in a round robin manner:
 *
 * 1. It starts from the first device and moves to next device if no cleanup
 *    item found.
 * 2. If no items were found in the regular devices bound lists it moves to the
 *    Host device list.
 * 3. In case there was nothing in the Host device items list but there was
 *    something in the devices lists, it returns first found device bound item
 *
 * If `*ppsDeviceNode` is not NULL the lookup of the next cleanup item will
 * start from the next devices after `*ppsDeviceNode`. This makes sure that
 * we always pop a first item from each device and move to the next device.
 * We prevent "starvation" of some devices this way.
 */
static INLINE DLLIST_NODE *_CleanupThreadWorkListPop(PVRSRV_DATA *psPVRSRVData,
                                                     PVRSRV_DEVICE_NODE **ppsDeviceNode)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = NULL, *psFirstDevice = NULL;
	DLLIST_NODE *psNode = NULL, *psFirstNode = NULL;
	OS_SPINLOCK_FLAGS uiFlags;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	OSSpinLockAcquire(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);

	/* always need to start from the beginning of the list in case some of the
	 * devices were already destroyed */
	for (psDeviceNode = psPVRSRVData->psDeviceNodeList;
	     psDeviceNode != NULL;
	     psDeviceNode = psDeviceNode->psNext)
	{
		/* not safe to run cleanup during deinit */
		if (psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_DESTRUCTING)
		{
			continue;
		}

		psNode = dllist_get_next_node(&psDeviceNode->sCleanupThreadWorkList);

		/* remember first non-NULL node and device in case no items are found
		 * on later devices */
		if (psNode != NULL && psFirstNode == NULL)
		{
			psFirstNode = psNode;
			psFirstDevice = psDeviceNode;
		}

		/* to ensure that further device don't get starved skip the last device
		 * until all devices are processed and then move to non-device bound
		 * items */
		if (*ppsDeviceNode != NULL) {
			if (psDeviceNode == *ppsDeviceNode)
			{
				*ppsDeviceNode = NULL;
			}

			/* in case this is the last device and next iteration exits the
			 * loop */
			psNode = NULL;

			continue;
		}

		if (psNode != NULL)
		{
			*ppsDeviceNode = psDeviceNode;
			dllist_remove_node(psNode);

			break;
		}
	}

	/* if no item found in the regular devices check also the Host device */
	if (psNode == NULL)
	{
		psNode = dllist_get_next_node(&psPVRSRVData->psHostMemDeviceNode->sCleanupThreadWorkList);
		if (psNode != NULL)
		{
			dllist_remove_node(psNode);
		}
		*ppsDeviceNode = psPVRSRVData->psHostMemDeviceNode;
	}

	/* if no item found for the Host device check if there was a cleanup item
	 * in one of the previous devices, this starts processing items from the
	 * beginning without one extra empty call to "wrap around" */
	if (psNode == NULL && psFirstNode != NULL)
	{
		psNode = psFirstNode;
		*ppsDeviceNode = psFirstDevice;
		dllist_remove_node(psNode);
	}

	OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	return psNode;
}

/* Process the cleanup thread work list */
static IMG_BOOL _CleanupThreadProcessWorkList(PVRSRV_DATA *psPVRSRVData,
                                              IMG_BOOL *pbUseHWTimeout)
{
	DLLIST_NODE *psNodeIter, *psNodeLast;
	PVRSRV_ERROR eError;
	IMG_BOOL bNeedRetry = IMG_FALSE;
	OS_SPINLOCK_FLAGS uiFlags;
	PVRSRV_CLEANUP_TYPE eCleanupType;
	PVRSRV_DEVICE_NODE *psDeviceNode = NULL;

	/* Reset HWTimeout Flag */
	*pbUseHWTimeout = IMG_FALSE;

	psNodeLast = _CleanupThreadWorkListLast(psPVRSRVData);
	if (psNodeLast == NULL)
	{
		/* no elements to clean up */
		return IMG_FALSE;
	}

	/* any callback functions which return error will be
	 * moved to the back of the list, and additional items can be added
	 * to the list at any time so we ensure we only iterate from the
	 * head of the list to the current tail (since the tail may always
	 * be changing)
	 */
	do
	{
		PVRSRV_CLEANUP_THREAD_WORK *psData;
		CLEANUP_THREAD_FN pfnFree;
		IMG_BOOL bRetry = IMG_FALSE;

		psNodeIter = _CleanupThreadWorkListPop(psPVRSRVData, &psDeviceNode);
		if (psNodeIter == NULL)
		{
			return IMG_FALSE;
		}

		psData = IMG_CONTAINER_OF(psNodeIter, PVRSRV_CLEANUP_THREAD_WORK, sNode);

		/* get the function pointer address here so we have access to it
		 * in order to report the error in case of failure, without having
		 * to depend on psData not having been freed
		 */
		pfnFree = psData->pfnFree;
		eCleanupType = psData->eCleanupType;
		eError = pfnFree(psData->pvData);

		if (eError != PVRSRV_OK)
		{
			/* Move to back of the list, if this item's
			 * retry count hasn't hit zero.
			 */
			if (CLEANUP_THREAD_IS_RETRY_TIMEOUT(psData))
			{
				if (CLEANUP_THREAD_RETRY_TIMEOUT_NOT_REACHED(psData))
				{
					bNeedRetry = IMG_TRUE;
					bRetry = IMG_TRUE;
					/* If any items require retry and are HW dependent
					 * use the HW timeout
					 */
					if (psData->bDependsOnHW)
					{
						*pbUseHWTimeout = psData->bDependsOnHW;
					}
				}
			}
			else
			{
				if (psData->ui32RetryCount > 0)
				{
					psData->ui32RetryCount--;
					bNeedRetry = IMG_TRUE;
					bRetry = IMG_TRUE;
					/* If any items require retry and are HW dependent
					 * use the HW timeout
					 */
					if (psData->bDependsOnHW)
					{
						*pbUseHWTimeout = psData->bDependsOnHW;
					}
				}
			}

			/* If the work depends on HW then we should add it to the back of the list,
			 * the cleanup thread will sleep for longer if required and the next MISR
			 * from the device will wake the task again in which it might be ready.
			 */
			if (bRetry || psData->bDependsOnHW)
			{
				/* If any items on the work list depend on HW
				 * and didn't get cleaned up.
				 */
				OSSpinLockAcquire(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
				dllist_add_to_tail(&psDeviceNode->sCleanupThreadWorkList, psNodeIter);
				OSSpinLockRelease(psPVRSRVData->hCleanupThreadWorkListLock, uiFlags);
			}
			else
			{
				PVR_DPF((PVR_DBG_ERROR, "Failed to free resource (callback " IMG_PFN_FMTSPEC "). "
				        "Retry limit reached", pfnFree));
				OSAtomicIncrement(&psPVRSRVData->i32NumCleanupItemsNotCompleted);
				OSAtomicDecrement(&psDeviceNode->i32NumCleanupItems);
				/* Dropping item */
				_CleanupThreadDecrementStats(psPVRSRVData, eCleanupType);
			}
		}
		else
		{
			/* Ok returned */
			OSAtomicDecrement(&psDeviceNode->i32NumCleanupItems);
			_CleanupThreadDecrementStats(psPVRSRVData, eCleanupType);
		}
	} while (psNodeIter != NULL && psNodeIter != psNodeLast);

	return bNeedRetry;
}

// #define CLEANUP_DPFL PVR_DBG_WARNING
#define CLEANUP_DPFL    PVR_DBG_MESSAGE

/* Create/initialise data required by the cleanup thread,
 * before the cleanup thread is started
 */
static PVRSRV_ERROR _CleanupThreadPrepare(PVRSRV_DATA *psPVRSRVData)
{
	PVRSRV_ERROR eError;

	/* Create the clean up event object */

	eError = OSEventObjectCreate("PVRSRV_CLEANUP_EVENTOBJECT", &gpsPVRSRVData->hCleanupEventObject);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", Exit);

	/* initialise the mutex and linked list required for the cleanup thread work list */

	eError = OSSpinLockCreate(&psPVRSRVData->hCleanupThreadWorkListLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", Exit);

Exit:
	return eError;
}

static void CleanupThread(void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = pvData;
	IMG_BOOL     bRetryWorkList = IMG_FALSE;
	IMG_BOOL     bUseHWTimeout = IMG_FALSE;
	IMG_HANDLE	 hOSEvent;
	PVRSRV_ERROR eRc;
	IMG_UINT32 uiUnloadRetry = 0;
	DLLIST_NODE *psNodeIter, *psNodeLast;

	/* Store the process id (pid) of the clean-up thread */
	psPVRSRVData->cleanupThreadPid = OSGetCurrentProcessID();
	psPVRSRVData->cleanupThreadTid = OSGetCurrentThreadID();

	PVR_DPF((CLEANUP_DPFL, "CleanupThread: thread starting... "));

	/* Open an event on the clean up event object so we can listen on it,
	 * abort the clean up thread and driver if this fails.
	 */
	eRc = OSEventObjectOpen(psPVRSRVData->hCleanupEventObject, &hOSEvent);
	PVR_ASSERT(eRc == PVRSRV_OK);

	/* While the driver is in a good state and is not being unloaded
	 * try to free any deferred items when signalled
	 */
	while (psPVRSRVData->eServicesState == PVRSRV_SERVICES_STATE_OK)
	{
		IMG_UINT64 ui64Timeoutus;
		if (psPVRSRVData->bUnload)
		{
			if (dllist_is_empty(&psPVRSRVData->psHostMemDeviceNode->sCleanupThreadWorkList) ||
			    uiUnloadRetry > CLEANUP_THREAD_UNLOAD_RETRY)
			{
				break;
			}
			uiUnloadRetry++;
		}

		/* Wait until signalled for deferred clean up OR wait for a
		 * short period if the previous deferred clean up was not able
		 * to release all the resources before trying again.
		 * Bridge lock re-acquired on our behalf before the wait call returns.
		 */

		if (bRetryWorkList && bUseHWTimeout)
		{
			/* If item depends on HW we are
			 * waiting for GPU work to finish, so
			 * use MAX_HW_TIME_US as timeout (this
			 * will be set appropriately when
			 * running on systems with emulated
			 * hardware, etc).
			 */
			ui64Timeoutus = MAX_HW_TIME_US;
		}
		else
		{
			/* Use the default retry timeout. */
			ui64Timeoutus = CLEANUP_THREAD_WAIT_RETRY_TIMEOUT;
		}

		eRc = OSEventObjectWaitKernel(hOSEvent,
				bRetryWorkList ?
				ui64Timeoutus :
				CLEANUP_THREAD_WAIT_SLEEP_TIMEOUT);
		if (eRc == PVRSRV_ERROR_TIMEOUT)
		{
			PVR_DPF((CLEANUP_DPFL, "CleanupThread: wait timeout"));
		}
		else if (eRc == PVRSRV_OK)
		{
			PVR_DPF((CLEANUP_DPFL, "CleanupThread: wait OK, signal received"));
		}
		else
		{
			PVR_LOG_ERROR(eRc, "OSEventObjectWaitKernel");
		}

		bRetryWorkList = _CleanupThreadProcessWorkList(psPVRSRVData,
		                                               &bUseHWTimeout);
	}

	psNodeLast = _CleanupThreadWorkListLast(psPVRSRVData);
	if (psNodeLast != NULL)
	{
		do
		{
			PVRSRV_DEVICE_NODE *psDeviceNode = NULL;
			PVRSRV_CLEANUP_THREAD_WORK *psData;
			psNodeIter = _CleanupThreadWorkListPop(psPVRSRVData, &psDeviceNode);
			if (psNodeIter == NULL)
			{
				break;
			}

			psData = IMG_CONTAINER_OF(psNodeIter, PVRSRV_CLEANUP_THREAD_WORK, sNode);
			OSAtomicIncrement(&psPVRSRVData->i32NumCleanupItemsNotCompleted);
			OSAtomicDecrement(&psDeviceNode->i32NumCleanupItems);
			/* Dropping item */
			_CleanupThreadDecrementStats(psPVRSRVData, psData->eCleanupType);
		}
		while (psNodeIter != NULL && psNodeIter != psNodeLast);

		PVR_DPF((PVR_DBG_ERROR, "Cleanup Thread Failed to free %d resources", OSAtomicRead(&psPVRSRVData->i32NumCleanupItemsNotCompleted)));
	}

	OSSpinLockDestroy(psPVRSRVData->hCleanupThreadWorkListLock);

	eRc = OSEventObjectClose(hOSEvent);
	PVR_LOG_IF_ERROR(eRc, "OSEventObjectClose");

	PVR_DPF((CLEANUP_DPFL, "CleanupThread: thread ending... "));
}

IMG_PID PVRSRVCleanupThreadGetPid(void)
{
	return gpsPVRSRVData->cleanupThreadPid;
}

uintptr_t PVRSRVCleanupThreadGetTid(void)
{
	return gpsPVRSRVData->cleanupThreadTid;
}

#if defined(SUPPORT_FW_HOST_SIDE_RECOVERY)
/*
 * Firmware is unresponsive.
 * The Host will initiate a recovery process during which the
 * Firmware and GPU are reset and returned to a working state.
 */
static PVRSRV_ERROR HandleFwHostSideRecovery(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *) psDeviceNode->pvDevice;
	DLLIST_NODE *psNode, *psNext;
	IMG_UINT32 ui32CtxIdx = 0U;
	IMG_UINT32 ui32Nodes  = 0U;

	OSWRLockAcquireRead(psDevInfo->hCommonCtxtListLock);
	/* Get the number of nodes in a linked list */
	dllist_foreach_node(&psDevInfo->sCommonCtxtListHead, psNode, psNext)
	{
		++ui32Nodes;
	}

	/* Any client contexts active at the moment? */
	if (ui32Nodes > 0U)
	{
		/* Free the active context buffer previously allocated */
		if (psDevInfo->psRGXFWIfActiveContextBufDesc)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfActiveContextBufDesc);
			DevmemFwUnmapAndFree(psDevInfo, psDevInfo->psRGXFWIfActiveContextBufDesc);
			psDevInfo->psRGXFWIfActiveContextBufDesc = NULL;
		}

		/* Setup allocations to store the active contexts */
		eError = RGXSetupFwAllocation(psDevInfo,
				RGX_FWSHAREDMEM_MAIN_ALLOCFLAGS,
				(ui32Nodes + 1) * sizeof(RGXFWIF_ACTIVE_CONTEXT_BUF_DATA),
				"FwSysActiveContextBufData",
				&psDevInfo->psRGXFWIfActiveContextBufDesc,
				(void *)  &psDevInfo->psRGXFWIfSysInit->sActiveContextBufBase.ui32Addr,
				(void **) &psDevInfo->psRGXFWIfActiveContextBuf,
				RFW_FWADDR_NOREF_FLAG);
		PVR_LOG_GOTO_IF_ERROR(eError, "RGXSetupFwAllocation",Error);

		/* List of contexts to be rekicked by FW powering up the device */
		dllist_foreach_node_backwards(&psDevInfo->sCommonCtxtListHead, psNode, psNext)
		{
			psDevInfo->psRGXFWIfActiveContextBuf[ui32CtxIdx].psContext =
					RGXGetFWCommonContextAddrFromServerCommonCtx(psDevInfo, psNode);
			++ui32CtxIdx;
		}
		/* Null context as the terminator marker */
		psDevInfo->psRGXFWIfActiveContextBuf[ui32CtxIdx].psContext.ui32Addr = 0;
	}
	OSWRLockReleaseRead(psDevInfo->hCommonCtxtListLock);

	/* Host can't expect a response on power-down request as FW is in BAD state */
	eError = PVRSRVSetDeviceCurrentPowerState(psDeviceNode->psPowerDev, PVRSRV_DEV_POWER_STATE_OFF);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVSetDeviceCurrentPowerState OFF", Error);

	/* Flag to be set to notify FW while recovering from crash */
	psDevInfo->psRGXFWIfSysInit->bFwHostRecoveryMode = IMG_TRUE;

	/* Flush here because we have setup a fw alloc addr in the structure earlier */
	RGXFwSharedMemCacheOpPtr(psDevInfo->psRGXFWIfSysInit, FLUSH);

	/* Power-on the device resetting GPU & FW */
	OSLockAcquire(psDeviceNode->hPowerLock);
	eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
			PVRSRV_DEV_POWER_STATE_ON,
			PVRSRV_POWER_FLAGS_NONE);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVSetDevicePowerStateKM ON", Error);
	OSLockRelease(psDeviceNode->hPowerLock);

Error:
	return eError;
}
#endif


void PVRSRVDeviceSetState(PVRSRV_DEVICE_NODE *psDeviceNode, PVRSRV_DEVICE_STATE eNewDevState)
{
	if (eNewDevState == psDeviceNode->eDevState)
	{
		return;
	}

	switch (eNewDevState)
	{
		case PVRSRV_DEVICE_STATE_PCI_ERROR:
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: %s", __func__, PVRSRVGetDebugDevStateString(eNewDevState)));
			psDeviceNode->eDevState = eNewDevState;
#if defined(SUPPORT_PMR_DEFERRED_FREE)
			PMRFreeZombies(psDeviceNode);
#endif
			break;
		}
		case PVRSRV_DEVICE_STATE_CREATING:
		case PVRSRV_DEVICE_STATE_CREATED:
		case PVRSRV_DEVICE_STATE_ACTIVE:
		case PVRSRV_DEVICE_STATE_FROZEN:
		case PVRSRV_DEVICE_STATE_DEINIT:
		case PVRSRV_DEVICE_STATE_DESTRUCTING:
		case PVRSRV_DEVICE_STATE_BAD:
		{
			/* PCI_ERROR is a terminal state. Reload driver to recover. */
			if (eNewDevState != PVRSRV_DEVICE_STATE_PCI_ERROR)
			{
				psDeviceNode->eDevState = eNewDevState;
			}
			break;
		}
		case PVRSRV_DEVICE_STATE_UNDEFINED:
		default:
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Unknown state (%d)", __func__, eNewDevState));
			break;
		}
	}
}

static void DevicesWatchdogThread_ForEachVaCb(PVRSRV_DEVICE_NODE *psDeviceNode,
											  va_list va)
{
#if defined(SUPPORT_RGX)
	PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *) psDeviceNode->pvDevice;
#endif
	PVRSRV_DEVICE_HEALTH_STATUS *pePreviousHealthStatus, eHealthStatus;
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_DEBUG_DUMP_STATUS eDebugDumpState;
	IMG_BOOL bCheckAfterTimePassed;

	pePreviousHealthStatus = va_arg(va, PVRSRV_DEVICE_HEALTH_STATUS *);
	/* IMG_BOOL (i.e. bool) is converted to int during default argument promotion
	 * in variadic argument list. Thus, fetch an int to get IMG_BOOL */
	bCheckAfterTimePassed = (IMG_BOOL) va_arg(va, int);

	if (psDeviceNode->eDevState != PVRSRV_DEVICE_STATE_ACTIVE)
	{
		return;
	}

	/* Block here if needed */
	PVRSRVBlockIfFrozen(psDeviceNode);

	if (psDeviceNode->pfnUpdateHealthStatus != NULL)
	{
		eError = psDeviceNode->pfnUpdateHealthStatus(psDeviceNode, bCheckAfterTimePassed);
		PVR_WARN_IF_ERROR(eError, "pfnUpdateHealthStatus");
	}
	eHealthStatus = OSAtomicRead(&psDeviceNode->eHealthStatus);

	if (eHealthStatus != PVRSRV_DEVICE_HEALTH_STATUS_OK)
	{
		if (eHealthStatus != *pePreviousHealthStatus)
		{
#if defined(SUPPORT_RGX)
			if (!(psDevInfo->ui32DeviceFlags &
				  RGXKM_DEVICE_STATE_DISABLE_DW_LOGGING_EN))
#else
			/* In this case we don't have an RGX device */
			if (eHealthStatus != PVRSRV_DEVICE_HEALTH_STATUS_UNDEFINED)
#endif
			{
				PVR_DPF((PVR_DBG_ERROR, "DevicesWatchdogThread: "
						 "Device status not OK!!!"));
				PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX,
								   NULL, NULL);
#if defined(SUPPORT_FW_HOST_SIDE_RECOVERY)
				HandleFwHostSideRecovery(psDeviceNode);
#endif
			}
		}
	}

	*pePreviousHealthStatus = eHealthStatus;

	/* Have we received request from FW to capture debug dump(could be due to HWR) */
	eDebugDumpState = (PVRSRV_DEVICE_DEBUG_DUMP_STATUS)OSAtomicCompareExchange(
						&psDeviceNode->eDebugDumpRequested,
						PVRSRV_DEVICE_DEBUG_DUMP_CAPTURE,
						PVRSRV_DEVICE_DEBUG_DUMP_NONE);
	if (PVRSRV_DEVICE_DEBUG_DUMP_CAPTURE == eDebugDumpState)
	{
		PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
	}

}

#if defined(SUPPORT_RGX)
static void HWPerfPeriodicHostEventsThread(void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = pvData;
	IMG_HANDLE hOSEvent;
	PVRSRV_ERROR eError;

	eError = OSEventObjectOpen(psPVRSRVData->hHWPerfHostPeriodicEvObj, &hOSEvent);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "OSEventObjectOpen");

#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	while ((psPVRSRVData->eServicesState == PVRSRV_SERVICES_STATE_OK) &&
			!psPVRSRVData->bUnload && !psPVRSRVData->bHWPerfHostThreadStop)
#else
	while (!psPVRSRVData->bUnload && !psPVRSRVData->bHWPerfHostThreadStop)
#endif
	{
		PVRSRV_DEVICE_NODE *psDeviceNode;
		IMG_BOOL bInfiniteSleep = IMG_TRUE;

		eError = OSEventObjectWaitKernel(hOSEvent, (IMG_UINT64)psPVRSRVData->ui32HWPerfHostThreadTimeout * 1000);
		if (eError == PVRSRV_OK && (psPVRSRVData->bUnload || psPVRSRVData->bHWPerfHostThreadStop))
		{
			PVR_DPF((PVR_DBG_MESSAGE, "HWPerfPeriodicHostEventsThread: Shutdown event received."));
			break;
		}

		for (psDeviceNode = psPVRSRVData->psDeviceNodeList;
		     psDeviceNode != NULL;
		     psDeviceNode = psDeviceNode->psNext)
		{
			PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

			/* If the psDevInfo or hHWPerfHostStream are NULL it most
			 * likely means that this device or stream has not been
			 * initialised yet, so just skip */
			if (psDevInfo == NULL || psDevInfo->hHWPerfHostStream == NULL)
			{
				continue;
			}

			/* Check if the HWPerf host stream is open for reading before writing
			 * a packet, this covers cases where the event filter is not zeroed
			 * before a reader disconnects. */
			if (TLStreamIsOpenForReading(psDevInfo->hHWPerfHostStream))
			{
				/* As long as any of the streams is opened don't go into
				 * indefinite sleep. */
				bInfiniteSleep = IMG_FALSE;
#if defined(SUPPORT_RGX)
				RGXSRV_HWPERF_HOST_INFO(psDevInfo, RGX_HWPERF_INFO_EV_MEM64_USAGE);
#endif
			}
		}

		if (bInfiniteSleep)
		{
#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)
			psPVRSRVData->ui32HWPerfHostThreadTimeout = INFINITE_SLEEP_TIMEOUT;
#else
			/* Use an 8 hour timeout if indefinite sleep is not supported. */
			psPVRSRVData->ui32HWPerfHostThreadTimeout = 60 * 60 * 8 * 1000;
#endif
		}
	}

	eError = OSEventObjectClose(hOSEvent);
	PVR_LOG_IF_ERROR(eError, "OSEventObjectClose");
}
#endif

#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)

typedef enum
{
	DWT_ST_INIT,
	DWT_ST_SLEEP_POWERON,
	DWT_ST_SLEEP_POWEROFF,
	DWT_ST_SLEEP_DEFERRED,
	DWT_ST_FINAL
} DWT_STATE;

typedef enum
{
	DWT_SIG_POWERON,
	DWT_SIG_POWEROFF,
	DWT_SIG_TIMEOUT,
	DWT_SIG_UNLOAD,
	DWT_SIG_ERROR
} DWT_SIGNAL;

static inline IMG_BOOL _DwtIsPowerOn(PVRSRV_DATA *psPVRSRVData)
{
	return List_PVRSRV_DEVICE_NODE_IMG_BOOL_Any(psPVRSRVData->psDeviceNodeList,
	                                            PVRSRVIsDevicePowered);
}

static inline void _DwtCheckHealthStatus(PVRSRV_DATA *psPVRSRVData,
                                         PVRSRV_DEVICE_HEALTH_STATUS *peStatus,
                                         IMG_BOOL bTimeOut)
{
	List_PVRSRV_DEVICE_NODE_ForEach_va(psPVRSRVData->psDeviceNodeList,
	                                   DevicesWatchdogThread_ForEachVaCb,
	                                   peStatus,
	                                   bTimeOut);
}

static DWT_SIGNAL _DwtWait(PVRSRV_DATA *psPVRSRVData, IMG_HANDLE hOSEvent,
                           IMG_UINT32 ui32Timeout)
{
	PVRSRV_ERROR eError;

	eError = OSEventObjectWaitKernel(hOSEvent, (IMG_UINT64) ui32Timeout * 1000);


	if (eError == PVRSRV_OK)
	{
		if (psPVRSRVData->bUnload)
		{
			PVR_DPF((PVR_DBG_MESSAGE, "DevicesWatchdogThread: Shutdown event"
			        " received."));
			return DWT_SIG_UNLOAD;
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE, "DevicesWatchdogThread: Power state "
			        "change event received."));

			if (_DwtIsPowerOn(psPVRSRVData))
			{
				return DWT_SIG_POWERON;
			}
			else
			{
				return DWT_SIG_POWEROFF;
			}
		}
	}
	else if (eError == PVRSRV_ERROR_TIMEOUT)
	{
		return DWT_SIG_TIMEOUT;
	}

	PVR_DPF((PVR_DBG_ERROR, "DevicesWatchdogThread: Error (%d) when"
	        " waiting for event!", eError));
	return DWT_SIG_ERROR;
}

#endif /* defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP) */

static void DevicesWatchdogThread(void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = pvData;
	PVRSRV_DEVICE_HEALTH_STATUS ePreviousHealthStatus = PVRSRV_DEVICE_HEALTH_STATUS_OK;
	IMG_HANDLE hOSEvent;
	PVRSRV_ERROR eError;
#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)
	DWT_STATE eState = DWT_ST_INIT;
	const IMG_UINT32 ui32OnTimeout = DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT;
	const IMG_UINT32 ui32OffTimeout = INFINITE_SLEEP_TIMEOUT;
#else
	IMG_UINT32 ui32Timeout = DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT;
	/* Flag used to defer the sleep timeout change by 1 loop iteration.
	 * This helps to ensure at least two health checks are performed before a long sleep.
	 */
	IMG_BOOL bDoDeferredTimeoutChange = IMG_FALSE;
#endif

	PVR_DPF((PVR_DBG_MESSAGE, "DevicesWatchdogThread: Power off sleep time: %d.",
			DEVICES_WATCHDOG_POWER_OFF_SLEEP_TIMEOUT));

	/* Open an event on the devices watchdog event object so we can listen on it
	   and abort the devices watchdog thread. */
	eError = OSEventObjectOpen(psPVRSRVData->hDevicesWatchdogEvObj, &hOSEvent);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "OSEventObjectOpen");

	/* Loop continuously checking the device status every few seconds. */
#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	while ((psPVRSRVData->eServicesState == PVRSRV_SERVICES_STATE_OK) &&
			!psPVRSRVData->bUnload)
#else
	while (!psPVRSRVData->bUnload)
#endif
	{
#if defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP)

		switch (eState)
		{
			case DWT_ST_INIT:
			{
				if (_DwtIsPowerOn(psPVRSRVData))
				{
					eState = DWT_ST_SLEEP_POWERON;
				}
				else
				{
					eState = DWT_ST_SLEEP_POWEROFF;
				}

				break;
			}
			case DWT_ST_SLEEP_POWERON:
			{
				DWT_SIGNAL eSignal = _DwtWait(psPVRSRVData, hOSEvent,
				                                    ui32OnTimeout);

				switch (eSignal) {
					case DWT_SIG_POWERON:
						/* self-transition, nothing to do */
						break;
					case DWT_SIG_POWEROFF:
						eState = DWT_ST_SLEEP_DEFERRED;
						break;
					case DWT_SIG_TIMEOUT:
						_DwtCheckHealthStatus(psPVRSRVData,
						                      &ePreviousHealthStatus,
						                      IMG_TRUE);
						/* self-transition */
						break;
					case DWT_SIG_UNLOAD:
						eState = DWT_ST_FINAL;
						break;
					case DWT_SIG_ERROR:
						/* deliberately ignored */
						break;
				}

				break;
			}
			case DWT_ST_SLEEP_POWEROFF:
			{
				DWT_SIGNAL eSignal = _DwtWait(psPVRSRVData, hOSEvent,
				                                    ui32OffTimeout);

				switch (eSignal) {
					case DWT_SIG_POWERON:
						eState = DWT_ST_SLEEP_POWERON;
						_DwtCheckHealthStatus(psPVRSRVData,
						                      &ePreviousHealthStatus,
						                      IMG_FALSE);
						break;
					case DWT_SIG_POWEROFF:
						/* self-transition, nothing to do */
						break;
					case DWT_SIG_TIMEOUT:
						/* self-transition */
						_DwtCheckHealthStatus(psPVRSRVData,
						                      &ePreviousHealthStatus,
						                      IMG_TRUE);
						break;
					case DWT_SIG_UNLOAD:
						eState = DWT_ST_FINAL;
						break;
					case DWT_SIG_ERROR:
						/* deliberately ignored */
						break;
				}

				break;
			}
			case DWT_ST_SLEEP_DEFERRED:
			{
				DWT_SIGNAL eSignal =_DwtWait(psPVRSRVData, hOSEvent,
				                                   ui32OnTimeout);

				switch (eSignal) {
					case DWT_SIG_POWERON:
						eState = DWT_ST_SLEEP_POWERON;
						_DwtCheckHealthStatus(psPVRSRVData,
						                      &ePreviousHealthStatus,
						                      IMG_FALSE);
						break;
					case DWT_SIG_POWEROFF:
						/* self-transition, nothing to do */
						break;
					case DWT_SIG_TIMEOUT:
						eState = DWT_ST_SLEEP_POWEROFF;
						_DwtCheckHealthStatus(psPVRSRVData,
						                      &ePreviousHealthStatus,
						                      IMG_FALSE);
						break;
					case DWT_SIG_UNLOAD:
						eState = DWT_ST_FINAL;
						break;
					case DWT_SIG_ERROR:
						/* deliberately ignored */
						break;
				}

				break;
			}
			case DWT_ST_FINAL:
				/* the loop should terminate on next spin if this state is
				 * reached so nothing to do here. */
				break;
		}

#else /* defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP) */
		IMG_BOOL bPwrIsOn = IMG_FALSE;
		IMG_BOOL bTimeOut = IMG_FALSE;

		/* Wait time between polls (done at the start of the loop to allow devices
		   to initialise) or for the event signal (shutdown or power on). */
		eError = OSEventObjectWaitKernel(hOSEvent, (IMG_UINT64)ui32Timeout * 1000);

		if (eError == PVRSRV_OK)
		{
			if (psPVRSRVData->bUnload)
			{
				PVR_DPF((PVR_DBG_MESSAGE, "DevicesWatchdogThread: Shutdown event received."));
				break;
			}
			else
			{
				PVR_DPF((PVR_DBG_MESSAGE, "DevicesWatchdogThread: Power state change event received."));
			}
		}
		else if (eError != PVRSRV_ERROR_TIMEOUT)
		{
			/* If timeout do nothing otherwise print warning message. */
			PVR_DPF((PVR_DBG_ERROR, "DevicesWatchdogThread: "
					"Error (%d) when waiting for event!", eError));
		}
		else
		{
			bTimeOut = IMG_TRUE;
		}

		OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
		List_PVRSRV_DEVICE_NODE_ForEach_va(psPVRSRVData->psDeviceNodeList,
		                                   DevicesWatchdogThread_ForEachVaCb,
		                                   &ePreviousHealthStatus,
		                                   bTimeOut);
		bPwrIsOn = List_PVRSRV_DEVICE_NODE_IMG_BOOL_Any(psPVRSRVData->psDeviceNodeList,
														PVRSRVIsDevicePowered);
		OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

		if (bPwrIsOn || psPVRSRVData->ui32DevicesWatchdogPwrTrans)
		{
			psPVRSRVData->ui32DevicesWatchdogPwrTrans = 0;
			ui32Timeout = psPVRSRVData->ui32DevicesWatchdogTimeout = DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT;
			bDoDeferredTimeoutChange = IMG_FALSE;
		}
		else
		{
			/* First, check if the previous loop iteration signalled a need to change the timeout period */
			if (bDoDeferredTimeoutChange)
			{
				ui32Timeout = psPVRSRVData->ui32DevicesWatchdogTimeout = DEVICES_WATCHDOG_POWER_OFF_SLEEP_TIMEOUT;
				bDoDeferredTimeoutChange = IMG_FALSE;
			}
			else
			{
				/* Signal that we need to change the sleep timeout in the next loop iteration
				 * to allow the device health check code a further iteration at the current
				 * sleep timeout in order to determine bad health (e.g. stalled cCCB) by
				 * comparing past and current state snapshots */
				bDoDeferredTimeoutChange = IMG_TRUE;
			}
		}

#endif /* defined(PVRSRV_SERVER_THREADS_INDEFINITE_SLEEP) */
	}

	eError = OSEventObjectClose(hOSEvent);
	PVR_LOG_IF_ERROR(eError, "OSEventObjectClose");
}

#if defined(SUPPORT_AUTOVZ)
static void AutoVzWatchdogThread_ForEachCb(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if (psDeviceNode->pfnUpdateAutoVzWatchdog != NULL)
	{
		psDeviceNode->pfnUpdateAutoVzWatchdog(psDeviceNode);
	}
}

static void AutoVzWatchdogThread(void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = pvData;
	IMG_HANDLE hOSEvent;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32Timeout = PVR_AUTOVZ_WDG_PERIOD_MS / 3;

	/* Open an event on the devices watchdog event object so we can listen on it
	   and abort the devices watchdog thread. */
	eError = OSEventObjectOpen(psPVRSRVData->hAutoVzWatchdogEvObj, &hOSEvent);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "OSEventObjectOpen");

#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	while ((psPVRSRVData->eServicesState == PVRSRV_SERVICES_STATE_OK) &&
			!psPVRSRVData->bUnload)
#else
	while (!psPVRSRVData->bUnload)
#endif
	{
		/* Wait time between polls (done at the start of the loop to allow devices
		   to initialise) or for the event signal (shutdown or power on). */
		eError = OSEventObjectWaitKernel(hOSEvent, (IMG_UINT64)ui32Timeout * 1000);

		List_PVRSRV_DEVICE_NODE_ForEach(psPVRSRVData->psDeviceNodeList,
		                                AutoVzWatchdogThread_ForEachCb);
	}

	eError = OSEventObjectClose(hOSEvent);
	PVR_LOG_IF_ERROR(eError, "OSEventObjectClose");
}
#endif /* SUPPORT_AUTOVZ */

PVRSRV_DATA *PVRSRVGetPVRSRVData(void)
{
	return gpsPVRSRVData;
}

static PVRSRV_ERROR InitialiseInfoPageTimeouts(PVRSRV_DATA *psPVRSRVData)
{
	if (NULL == psPVRSRVData)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psPVRSRVData->pui32InfoPage[TIMEOUT_INFO_VALUE_RETRIES] = WAIT_TRY_COUNT;
	psPVRSRVData->pui32InfoPage[TIMEOUT_INFO_VALUE_TIMEOUT_MS] =
		((MAX_HW_TIME_US / 10000) + 1000);
		/* TIMEOUT_INFO_VALUE_TIMEOUT_MS resolves to...
			vp       : 2000  + 1000
			emu      : 2000  + 1000
			rgx_nohw : 50    + 1000
			plato    : 30000 + 1000 (VIRTUAL_PLATFORM or EMULATOR)
			           50    + 1000 (otherwise)
		*/

	psPVRSRVData->pui32InfoPage[TIMEOUT_INFO_CONDITION_RETRIES] = 5;
	psPVRSRVData->pui32InfoPage[TIMEOUT_INFO_CONDITION_TIMEOUT_MS] =
		((MAX_HW_TIME_US / 10000) + 100);
		/* TIMEOUT_INFO_CONDITION_TIMEOUT_MS resolves to...
			vp       : 2000  + 100
			emu      : 2000  + 100
			rgx_nohw : 50    + 100
			plato    : 30000 + 100 (VIRTUAL_PLATFORM or EMULATOR)
			           50    + 100 (otherwise)
		*/

	psPVRSRVData->pui32InfoPage[TIMEOUT_INFO_TASK_QUEUE_RETRIES] = 10;

	psPVRSRVData->pui32InfoPage[TIMEOUT_INFO_TASK_QUEUE_FLUSH_TIMEOUT_MS] =
		MAX_HW_TIME_US / 1000U;

	return PVRSRV_OK;
}

static PVRSRV_ERROR PopulateInfoPageBridges(PVRSRV_DATA *psPVRSRVData)
{
	PVR_RETURN_IF_INVALID_PARAM(psPVRSRVData);

	psPVRSRVData->pui32InfoPage[BRIDGE_INFO_PVR_BRIDGES] = gui32PVRBridges;

#if defined(SUPPORT_RGX)
	psPVRSRVData->pui32InfoPage[BRIDGE_INFO_RGX_BRIDGES] = gui32RGXBridges;
#else
	psPVRSRVData->pui32InfoPage[BRIDGE_INFO_RGX_BRIDGES] = 0;
#endif

	return PVRSRV_OK;
}

static void _ThreadsDebugRequestNotify(PVRSRV_DBGREQ_HANDLE hDbgRequestHandle,
                                       IMG_UINT32 ui32VerbLevel,
                                       DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
                                       void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(hDbgRequestHandle);

	if (DD_VERB_LVL_ENABLED(ui32VerbLevel, DEBUG_REQUEST_VERBOSITY_HIGH))
	{
		PVR_DUMPDEBUG_LOG("------[ Server Thread Summary ]------");
		OSThreadDumpInfo(pfnDumpDebugPrintf, pvDumpDebugFile);
	}
}

#define PVRVZMODE_STR_SIZE_MAX (10U)
#define PVRVZMODE_BUFFER_SIZE (((PVRSRV_MAX_DEVICES)*(PVRVZMODE_STR_SIZE_MAX))+1)

static const IMG_CHAR *const asModeStrings[] =
{
	"native",
	"host",
	"guest",
	"default"
};

static void _InitDefaultVzDriverMode(PVRSRV_DATA *psPVRSRVData, void *pvAppHintState)
{
	char *pszMode, pszModeAppHint[PVRVZMODE_BUFFER_SIZE];
	static const IMG_CHAR *pszDefault = "";
	PVRSRV_DRIVER_MODE aeInitModes[PVRSRV_MAX_DEVICES];
	IMG_BOOL bRet;
	IMG_UINT ui32Dev;

	for (ui32Dev = 0; ui32Dev < PVRSRV_MAX_DEVICES; ui32Dev++)
	{
		aeInitModes[ui32Dev] = DRIVER_MODE_DEFAULT;
	}

	pszMode = pszModeAppHint;
	pszModeAppHint[0] = '\0';

	bRet = (IMG_BOOL) OSGetAppHintSTRING(APPHINT_NO_DEVICE,
						pvAppHintState,
						DriverMode,
						pszDefault,
						pszModeAppHint,
						PVRVZMODE_BUFFER_SIZE);
	PVR_GOTO_IF_FALSE(bRet, uninit_default);

	ui32Dev = 0;

	while (*pszMode && (ui32Dev < PVRSRV_MAX_DEVICES))
	{
		PVRSRV_DRIVER_MODE eMode;

		while (*pszMode == ',')
		{
			/* skip commas */
			pszMode++;
		}

		for (eMode = DRIVER_MODE_NATIVE; eMode <= DRIVER_MODE_DEFAULT; eMode++)
		{
			/* if no matching strings found, device's mode remains DEFAULT */
			if (OSStringNCompare(pszMode,
								 asModeStrings[eMode],
								 OSStringLength(asModeStrings[eMode])) == 0)
			{
				aeInitModes[ui32Dev] = eMode;
			}
		}

		while ((*pszMode != '\0') && (*pszMode != ','))
		{
			/* advance until the next command or string end */
			pszMode++;
		}

		ui32Dev++;
	}

uninit_default:
	for (ui32Dev = 0; ui32Dev < PVRSRV_MAX_DEVICES; ui32Dev++)
	{
		psPVRSRVData->aeModuleParamDriverMode[ui32Dev] = aeInitModes[ui32Dev];
	}
}

PVRSRV_ERROR
PVRSRVCommonDriverInit(void)
{
	PVRSRV_ERROR eError;

	PVRSRV_DATA	*psPVRSRVData = NULL;

	IMG_UINT32 ui32AppHintCleanupThreadPriority;
	IMG_UINT32 ui32AppHintWatchdogThreadPriority;
	IMG_BOOL bEnablePageFaultDebug;
	IMG_BOOL bEnableFullSyncTracking;

	void *pvAppHintState = NULL;
	IMG_UINT32 ui32AppHintDefault;
	IMG_BOOL bAppHintDefault;

	/*
	 * As this function performs one time driver initialisation, use the
	 * Services global device-independent data to determine whether or not
	 * this function has already been called.
	 */
	if (gpsPVRSRVData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Driver already initialised", __func__));
		return PVRSRV_ERROR_ALREADY_EXISTS;
	}

#if defined(SUPPORT_RGX)
	RGXBridgeDriverInit();
#endif

	eError = DIInit();
	PVR_GOTO_IF_ERROR(eError, Error);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	eError = PVRSRVStatsInitialise();
	PVR_GOTO_IF_ERROR(eError, Error);
#endif /* PVRSRV_ENABLE_PROCESS_STATS */

#if defined(SUPPORT_DI_BRG_IMPL)
	eError = PVRDIImplBrgRegister();
	PVR_GOTO_IF_ERROR(eError, Error);
#endif

	eError = HTB_CreateDIEntry();
	PVR_GOTO_IF_ERROR(eError, Error);

	/*
	 * Allocate the device-independent data. Do NOT track this allocation.
	 */
	psPVRSRVData = OSAllocZMemNoStats(sizeof(*gpsPVRSRVData));
	PVR_GOTO_IF_NOMEM(psPVRSRVData, eError, Error);

	/* Now it is set up, point gpsPVRSRVData to the actual data */
	gpsPVRSRVData = psPVRSRVData;

	eError = OSWRLockCreate(&gpsPVRSRVData->hDeviceNodeListLock);
	PVR_GOTO_IF_ERROR(eError, Error);

	/* Register the driver context debug table */
	eError = PVRSRVRegisterDriverDbgTable();
	PVR_GOTO_IF_ERROR(eError, Error);

	/* Register the Server Thread Debug notifier */
	eError = PVRSRVRegisterDriverDbgRequestNotify(&gpsPVRSRVData->hThreadsDbgReqNotify,
		                                          _ThreadsDebugRequestNotify,
		                                          DEBUG_REQUEST_SRV,
		                                          NULL);
	PVR_GOTO_IF_ERROR(eError, Error);

	/*
	 * Initialise the server bridges
	 */
	eError = ServerBridgeInit();
	PVR_GOTO_IF_ERROR(eError, Error);

	eError = DevmemIntInit();
	PVR_GOTO_IF_ERROR(eError, Error);

	eError = DebugCommonInitDriver();
	PVR_GOTO_IF_ERROR(eError, Error);

	eError = BridgeDispatcherInit();
	PVR_GOTO_IF_ERROR(eError, Error);

	/* Init any OS specific's */
	eError = OSInitEnvData();
	PVR_GOTO_IF_ERROR(eError, Error);

	/* Early init. server cache maintenance */
	eError = CacheOpInit();
	PVR_GOTO_IF_ERROR(eError, Error);

#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
	RIInitKM();
#endif

	bAppHintDefault = PVRSRV_APPHINT_ENABLEPAGEFAULTDEBUG;
	OSCreateAppHintState(&pvAppHintState);
	OSGetAppHintBOOL(APPHINT_NO_DEVICE, pvAppHintState, EnablePageFaultDebug,
			&bAppHintDefault, &bEnablePageFaultDebug);
	OSFreeAppHintState(pvAppHintState);

	if (bEnablePageFaultDebug)
	{
		eError = DevicememHistoryInitKM();
		PVR_LOG_GOTO_IF_ERROR(eError, "DevicememHistoryInitKM", Error);
	}

#if defined(PVRSRV_PHYSMEM_CPUMAP_HISTORY)
	eError = CPUMappingHistoryInit();
	PVR_GOTO_IF_ERROR(eError, Error);
#endif

	eError = PMRInit();
	PVR_GOTO_IF_ERROR(eError, Error);

#if defined(SUPPORT_DISPLAY_CLASS)
	eError = DCInit();
	PVR_GOTO_IF_ERROR(eError, Error);
#endif

	/* Initialise overall system state */
	gpsPVRSRVData->eServicesState = PVRSRV_SERVICES_STATE_OK;

	/* Create an event object */
	eError = OSEventObjectCreate("PVRSRV_GLOBAL_EVENTOBJECT", &gpsPVRSRVData->hGlobalEventObject);
	PVR_GOTO_IF_ERROR(eError, Error);
	gpsPVRSRVData->ui32GEOConsecutiveTimeouts = 0;

	eError = PVRSRVCmdCompleteInit();
	PVR_GOTO_IF_ERROR(eError, Error);

	eError = PVRSRVHandleInit();
	PVR_GOTO_IF_ERROR(eError, Error);

	OSCreateAppHintState(&pvAppHintState);
	ui32AppHintDefault = PVRSRV_APPHINT_CLEANUPTHREADPRIORITY;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, CleanupThreadPriority,
	                     &ui32AppHintDefault, &ui32AppHintCleanupThreadPriority);

	ui32AppHintDefault = PVRSRV_APPHINT_WATCHDOGTHREADPRIORITY;
	OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, WatchdogThreadPriority,
	                     &ui32AppHintDefault, &ui32AppHintWatchdogThreadPriority);

	bAppHintDefault = PVRSRV_APPHINT_ENABLEFULLSYNCTRACKING;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE, pvAppHintState, EnableFullSyncTracking,
			&bAppHintDefault, &bEnableFullSyncTracking);

	_InitDefaultVzDriverMode(gpsPVRSRVData, pvAppHintState);


	OSFreeAppHintState(pvAppHintState);
	pvAppHintState = NULL;

	eError = HostMemDeviceCreate(&gpsPVRSRVData->psHostMemDeviceNode);
	PVR_GOTO_IF_ERROR(eError, Error);

	eError = _CleanupThreadPrepare(gpsPVRSRVData);
	PVR_LOG_GOTO_IF_ERROR(eError, "_CleanupThreadPrepare", Error);

	/* Create a thread which is used to do the deferred cleanup */
	eError = OSThreadCreatePriority(&gpsPVRSRVData->hCleanupThread,
	                                "pvr_defer_free",
	                                CleanupThread,
	                                CleanupThreadDumpInfo,
	                                IMG_TRUE,
	                                gpsPVRSRVData,
	                                ui32AppHintCleanupThreadPriority);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSThreadCreatePriority:1", Error);

	/* Create the devices watchdog event object */
	eError = OSEventObjectCreate("PVRSRV_DEVICESWATCHDOG_EVENTOBJECT", &gpsPVRSRVData->hDevicesWatchdogEvObj);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", Error);

	/* Create a thread which is used to detect fatal errors */
	eError = OSThreadCreatePriority(&gpsPVRSRVData->hDevicesWatchdogThread,
	                                "pvr_device_wdg",
	                                DevicesWatchdogThread,
	                                NULL,
	                                IMG_TRUE,
	                                gpsPVRSRVData,
	                                ui32AppHintWatchdogThreadPriority);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSThreadCreatePriority:2", Error);

#if defined(SUPPORT_AUTOVZ)
	/* Create the devices watchdog event object */
	eError = OSEventObjectCreate("PVRSRV_AUTOVZ_WATCHDOG_EVENTOBJECT", &gpsPVRSRVData->hAutoVzWatchdogEvObj);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", Error);

	/* Create a thread that maintains the FW-KM connection by regularly updating the virtualization watchdog */
	eError = OSThreadCreatePriority(&gpsPVRSRVData->hAutoVzWatchdogThread,
	                                "pvr_autovz_wdg",
	                                AutoVzWatchdogThread,
	                                NULL,
	                                IMG_TRUE,
	                                gpsPVRSRVData,
	                                OS_THREAD_HIGHEST_PRIORITY);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSThreadCreatePriority:3", Error);
#endif /* SUPPORT_AUTOVZ */

#if defined(SUPPORT_RGX)
	eError = OSLockCreate(&gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", Error);
#endif

	/* Initialise the Transport Layer */
	eError = TLInit();
	PVR_GOTO_IF_ERROR(eError, Error);

	/* Initialise pdump */
	eError = PDUMPINIT();
	PVR_GOTO_IF_ERROR(eError, Error);

	g_ui32InitFlags |= INIT_DATA_ENABLE_PDUMPINIT;

	/* Initialise TL control stream */
	eError = TLStreamCreate(&psPVRSRVData->hTLCtrlStream,
	                        PVRSRV_TL_CTRL_STREAM,
	                        PVRSRV_TL_CTRL_STREAM_SIZE,
	                        TL_OPMODE_DROP_OLDEST,
	                        NULL,
	                        NULL,
	                        NULL,
	                        NULL,
	                        NULL,
                            NULL);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_ERROR(eError, "TLStreamCreate");
		psPVRSRVData->hTLCtrlStream = NULL;
	}

	eError = InfoPageCreate(psPVRSRVData);
	PVR_LOG_GOTO_IF_ERROR(eError, "InfoPageCreate", Error);


	/* Initialise the Timeout Info */
	eError = InitialiseInfoPageTimeouts(psPVRSRVData);
	PVR_GOTO_IF_ERROR(eError, Error);

	eError = PopulateInfoPageBridges(psPVRSRVData);

	PVR_GOTO_IF_ERROR(eError, Error);

	if (bEnableFullSyncTracking)
	{
		psPVRSRVData->pui32InfoPage[DEBUG_FEATURE_FLAGS] |= DEBUG_FEATURE_FULL_SYNC_TRACKING_ENABLED;
	}
	if (bEnablePageFaultDebug)
	{
		psPVRSRVData->pui32InfoPage[DEBUG_FEATURE_FLAGS] |= DEBUG_FEATURE_PAGE_FAULT_DEBUG_ENABLED;
	}

	{
		IMG_UINT64 *pui64InfoPage;

		pui64InfoPage = IMG_OFFSET_ADDR_DW(psPVRSRVData->pui32InfoPage,
		                                   DEVMEM_INFO_PHYS_BUF_MAX_SIZE);
		*pui64InfoPage = PMR_MAX_SUPPORTED_SIZE;
	}

	/* Initialise the Host Trace Buffer */
	eError = HTBInit();
	PVR_GOTO_IF_ERROR(eError, Error);

#if defined(SUPPORT_RGX)
	RGXHWPerfClientInitAppHintCallbacks();
#endif

	/* Late init. client cache maintenance via info. page */
	eError = CacheOpInit2();
	PVR_LOG_GOTO_IF_ERROR(eError, "CacheOpInit2", Error);

#if defined(SUPPORT_FALLBACK_FENCE_SYNC)
	eError = SyncFbRegisterSyncFunctions();
	PVR_LOG_GOTO_IF_ERROR(eError, "SyncFbRegisterSyncFunctions", Error);
#endif

#if defined(PDUMP)
#if (PVRSRV_DEVICE_INIT_MODE == PVRSRV_LINUX_DEV_INIT_ON_CONNECT)
	/* If initialising the device on first connection, we will
	 * bind PDump capture to the first device we connect to later.
	 */
	psPVRSRVData->ui32PDumpBoundDevice = PVRSRV_MAX_DEVICES;
#else
	/* If not initialising the device on first connection, bind PDump
	 * capture to device 0. This is because we need to capture PDump
	 * during device initialisation but only want to capture PDump for
	 * a single device (by default, device 0).
	 */
	psPVRSRVData->ui32PDumpBoundDevice = 0;
#endif
#endif

	return 0;

Error:
	PVRSRVCommonDriverDeInit();
	return eError;
}

void
PVRSRVCommonDriverDeInit(void)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_BOOL bEnablePageFaultDebug = IMG_FALSE;

	if (gpsPVRSRVData == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: missing device-independent data",
				 __func__));
		return;
	}

	if (gpsPVRSRVData->pui32InfoPage != NULL)
	{
		bEnablePageFaultDebug = GetInfoPageDebugFlagsKM() & DEBUG_FEATURE_PAGE_FAULT_DEBUG_ENABLED;
	}

	gpsPVRSRVData->bUnload = IMG_TRUE;

#if defined(SUPPORT_RGX)
	PVRSRVDestroyHWPerfHostThread();
	if (gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock)
	{
		OSLockDestroy(gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock);
		gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock = NULL;
	}
#endif

	if (gpsPVRSRVData->hGlobalEventObject)
	{
		OSEventObjectSignal(gpsPVRSRVData->hGlobalEventObject);
	}

#if defined(SUPPORT_AUTOVZ)
	/* Stop and cleanup the devices watchdog thread */
	if (gpsPVRSRVData->hAutoVzWatchdogThread)
	{
		LOOP_UNTIL_TIMEOUT_US(OS_THREAD_DESTROY_TIMEOUT_US)
		{
			if (gpsPVRSRVData->hAutoVzWatchdogEvObj)
			{
				eError = OSEventObjectSignal(gpsPVRSRVData->hAutoVzWatchdogEvObj);
				PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
			}

			eError = OSThreadDestroy(gpsPVRSRVData->hAutoVzWatchdogThread);
			if (PVRSRV_OK == eError)
			{
				gpsPVRSRVData->hAutoVzWatchdogThread = NULL;
				break;
			}
			OSWaitus(OS_THREAD_DESTROY_TIMEOUT_US/OS_THREAD_DESTROY_RETRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();
		PVR_LOG_IF_ERROR(eError, "OSThreadDestroy");
	}

	if (gpsPVRSRVData->hAutoVzWatchdogEvObj)
	{
		eError = OSEventObjectDestroy(gpsPVRSRVData->hAutoVzWatchdogEvObj);
		gpsPVRSRVData->hAutoVzWatchdogEvObj = NULL;
		PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
	}
#endif /* SUPPORT_AUTOVZ */

	/* Stop and cleanup the devices watchdog thread */
	if (gpsPVRSRVData->hDevicesWatchdogThread)
	{
		LOOP_UNTIL_TIMEOUT_US(OS_THREAD_DESTROY_TIMEOUT_US)
		{
			if (gpsPVRSRVData->hDevicesWatchdogEvObj)
			{
				eError = OSEventObjectSignal(gpsPVRSRVData->hDevicesWatchdogEvObj);
				PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
			}

			eError = OSThreadDestroy(gpsPVRSRVData->hDevicesWatchdogThread);
			if (PVRSRV_OK == eError)
			{
				gpsPVRSRVData->hDevicesWatchdogThread = NULL;
				break;
			}
			OSWaitus(OS_THREAD_DESTROY_TIMEOUT_US/OS_THREAD_DESTROY_RETRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();
		PVR_LOG_IF_ERROR(eError, "OSThreadDestroy");
	}

	if (gpsPVRSRVData->hDevicesWatchdogEvObj)
	{
		eError = OSEventObjectDestroy(gpsPVRSRVData->hDevicesWatchdogEvObj);
		gpsPVRSRVData->hDevicesWatchdogEvObj = NULL;
		PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
	}

	/* Stop and cleanup the deferred clean up thread, event object and
	 * deferred context list.
	 */
	if (gpsPVRSRVData->hCleanupThread)
	{
		LOOP_UNTIL_TIMEOUT_US(OS_THREAD_DESTROY_TIMEOUT_US)
		{
			if (gpsPVRSRVData->hCleanupEventObject)
			{
				eError = OSEventObjectSignal(gpsPVRSRVData->hCleanupEventObject);
				PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
			}

			eError = OSThreadDestroy(gpsPVRSRVData->hCleanupThread);
			if (PVRSRV_OK == eError)
			{
				gpsPVRSRVData->hCleanupThread = NULL;
				break;
			}
			OSWaitus(OS_THREAD_DESTROY_TIMEOUT_US/OS_THREAD_DESTROY_RETRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();
		PVR_LOG_IF_ERROR(eError, "OSThreadDestroy");
	}

	if (gpsPVRSRVData->hCleanupEventObject)
	{
		eError = OSEventObjectDestroy(gpsPVRSRVData->hCleanupEventObject);
		gpsPVRSRVData->hCleanupEventObject = NULL;
		PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
	}

	/* Tear down the HTB before PVRSRVHandleDeInit() removes its TL handle */
	/* HTB De-init happens in device de-registration currently */
	eError = HTBDeInit();
	PVR_LOG_IF_ERROR(eError, "HTBDeInit");

	/* Tear down CacheOp framework information page first */
	CacheOpDeInit2();

	/* Clean up information page */
	InfoPageDestroy(gpsPVRSRVData);

	/* Close the TL control plane stream. */
	if (gpsPVRSRVData->hTLCtrlStream != NULL)
	{
		TLStreamClose(gpsPVRSRVData->hTLCtrlStream);
	}

	/* deinitialise pdump */
	if ((g_ui32InitFlags & INIT_DATA_ENABLE_PDUMPINIT) > 0)
	{
		PDUMPDEINIT();
	}

	/* Clean up Transport Layer resources that remain */
	TLDeInit();

	HostMemDeviceDestroy(gpsPVRSRVData->psHostMemDeviceNode);
	gpsPVRSRVData->psHostMemDeviceNode = NULL;

	eError = PVRSRVHandleDeInit();
	PVR_LOG_IF_ERROR(eError, "PVRSRVHandleDeInit");

	/* destroy event object */
	if (gpsPVRSRVData->hGlobalEventObject)
	{
		OSEventObjectDestroy(gpsPVRSRVData->hGlobalEventObject);
		gpsPVRSRVData->hGlobalEventObject = NULL;
	}

	PVRSRVCmdCompleteDeinit();

#if defined(SUPPORT_DISPLAY_CLASS)
	eError = DCDeInit();
	PVR_LOG_IF_ERROR(eError, "DCDeInit");
#endif

	eError = PMRDeInit();
	PVR_LOG_IF_ERROR(eError, "PMRDeInit");

	BridgeDispatcherDeinit();

#if defined(PVRSRV_PHYSMEM_CPUMAP_HISTORY)
	CPUMappingHistoryDeInit();
#endif

	if (bEnablePageFaultDebug)
	{
		/* Clear all allocated history tracking data */
		DevicememHistoryDeInitKM();
	}

	CacheOpDeInit();

	OSDeInitEnvData();

	(void) DevmemIntDeInit();

	ServerBridgeDeInit();

	HTB_DestroyDIEntry();

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	PVRSRVStatsDestroyDI();		/* Stage 1 freeing */
#endif /* PVRSRV_ENABLE_PROCESS_STATS */

	DebugCommonDeInitDriver();

	DIDeInit();

	if (gpsPVRSRVData->hThreadsDbgReqNotify)
	{
		PVRSRVUnregisterDriverDbgRequestNotify(gpsPVRSRVData->hThreadsDbgReqNotify);
	}

	PVRSRVUnregisterDriverDbgTable();

	OSWRLockDestroy(gpsPVRSRVData->hDeviceNodeListLock);

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	PVRSRVStatsDestroy();		/* Stage 2 freeing */
#if defined(PVRSRV_ENABLE_GPU_MEMORY_INFO)
	RIDeInitKM();
#endif
#endif /* PVRSRV_ENABLE_PROCESS_STATS */

	OSFreeMemNoStats(gpsPVRSRVData);		/* Not trackable */

	gpsPVRSRVData = NULL;
}

static void _SysDebugRequestNotify(PVRSRV_DBGREQ_HANDLE hDebugRequestHandle,
					IMG_UINT32 ui32VerbLevel,
					DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
					void *pvDumpDebugFile)
{
	/* Only dump info once */
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE*) hDebugRequestHandle;

	PVR_UNREFERENCED_PARAMETER(ui32VerbLevel);

	PVR_DUMPDEBUG_LOG("------[ System Summary Device ID:%d ]------", psDeviceNode->sDevId.ui32InternalID);

	switch (psDeviceNode->eCurrentSysPowerState)
	{
		case PVRSRV_SYS_POWER_STATE_OFF:
			PVR_DUMPDEBUG_LOG("Device System Power State: OFF");
			break;
		case PVRSRV_SYS_POWER_STATE_ON:
			PVR_DUMPDEBUG_LOG("Device System Power State: ON");
			break;
		default:
			PVR_DUMPDEBUG_LOG("Device System Power State: UNKNOWN (%d)",
							   psDeviceNode->eCurrentSysPowerState);
			break;
	}

	PVR_DUMPDEBUG_LOG("MaxHWTOut: %dus, WtTryCt: %d, WDGTOut(on,off): (%dms,%dms)",
	                  MAX_HW_TIME_US, WAIT_TRY_COUNT, DEVICES_WATCHDOG_POWER_ON_SLEEP_TIMEOUT, DEVICES_WATCHDOG_POWER_OFF_SLEEP_TIMEOUT);

	SysDebugInfo(psDeviceNode->psDevConfig, pfnDumpDebugPrintf, pvDumpDebugFile);
}

PHYS_HEAP_CONFIG* PVRSRVFindPhysHeapConfig(PVRSRV_DEVICE_CONFIG *psDevConfig,
										   PHYS_HEAP_USAGE_FLAGS ui32Flags)
{
	IMG_UINT32 i;

	for (i = 0; i < psDevConfig->ui32PhysHeapCount; i++)
	{
		if (BITMASK_HAS(psDevConfig->pasPhysHeaps[i].ui32UsageFlags, ui32Flags))
		{
			return &psDevConfig->pasPhysHeaps[i];
		}
	}

	return NULL;
}

PVRSRV_ERROR PVRSRVAcquireInternalID(IMG_UINT32 *pui32InternalID)
{
	IMG_UINT32 ui32InternalID = 0;
	IMG_BOOL bFound = IMG_FALSE;

	for (ui32InternalID = 0;
		 ui32InternalID < PVRSRV_MAX_DEVICES;
		 ui32InternalID++)
	{
		if (PVRSRVGetDeviceInstance(ui32InternalID) == NULL)
		{
			bFound = IMG_TRUE;
			break;
		}
	}

	if (bFound)
	{
		*pui32InternalID = ui32InternalID;
		return PVRSRV_OK;
	}
	else
	{
		return PVRSRV_ERROR_NO_FREE_DEVICEIDS_AVAILABLE;
	}
}

PVRSRV_ERROR PVRSRVCommonDeviceCreate(void *pvOSDevice,
											 IMG_INT32 i32KernelDeviceID,
											 PVRSRV_DEVICE_NODE **ppsDeviceNode)
{
	PVRSRV_DATA				*psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_ERROR			eError;
	PVRSRV_DEVICE_CONFIG	*psDevConfig;
	PVRSRV_DEVICE_NODE		*psDeviceNode;
	IMG_UINT32				ui32InternalID;

#if defined(SUPPORT_PHYSMEM_TEST) && !defined(INTEGRITY_OS) && !defined(__QNXNTO__)
	IMG_UINT32				ui32AppHintPhysMemTestPasses;
#endif
	void *pvAppHintState    = NULL;
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	IMG_HANDLE				hProcessStats;
#endif
	IMG_BOOL				bAppHintDefault;
	IMG_BOOL				bEnablePageFaultDebug = IMG_FALSE;
#if defined(SUPPORT_AUTOVZ)
	IMG_BOOL				bAutoVzGPUPowerdown   = IMG_FALSE;
#endif

	MULTI_DEVICE_BRINGUP_DPF("PVRSRVCommonDeviceCreate: DevId %d", i32KernelDeviceID);

	/* Determine if we've got EnablePageFaultDebug set or not */
	bAppHintDefault = PVRSRV_APPHINT_ENABLEPAGEFAULTDEBUG;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE, pvAppHintState, EnablePageFaultDebug,
			&bAppHintDefault, &bEnablePageFaultDebug);

#if defined(SUPPORT_AUTOVZ)
	bAppHintDefault = PVRSRV_APPHINT_AUTOVZGPUPOWERDOWN;
	OSGetAppHintBOOL(APPHINT_NO_DEVICE, pvAppHintState, AutoVzGPUPowerdown,
			&bAppHintDefault, &bAutoVzGPUPowerdown);
#endif
	OSFreeAppHintState(pvAppHintState);
	pvAppHintState = NULL;

	psDeviceNode = OSAllocZMemNoStats(sizeof(*psDeviceNode));
	PVR_LOG_RETURN_IF_NOMEM(psDeviceNode, "psDeviceNode");

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	/* Allocate process statistics */
	eError = PVRSRVStatsRegisterProcess(&hProcessStats);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVStatsRegisterProcess", ErrorFreeDeviceNode);
#endif

	dllist_init(&psDeviceNode->sCleanupThreadWorkList);
	OSAtomicWrite(&psDeviceNode->i32NumCleanupItems, 0);

	/* Record setting of EnablePageFaultDebug in device-node */
	psDeviceNode->bEnablePFDebug = bEnablePageFaultDebug;
#if defined(SUPPORT_AUTOVZ)
	psDeviceNode->bAutoVzAllowGPUPowerdown = bAutoVzGPUPowerdown;
#endif
	psDeviceNode->sDevId.i32KernelDeviceID = i32KernelDeviceID;
	eError = PVRSRVAcquireInternalID(&ui32InternalID);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVAcquireInternalID", ErrorDeregisterStats);

	eError = SysDevInit(pvOSDevice, &psDevConfig);
	PVR_LOG_GOTO_IF_ERROR(eError, "SysDevInit", ErrorDeregisterStats);

	/* Lock down the InternalID for this device by saving the value into
	 * its device node. This is done after SysDevInit to allow the system
	 * layer to identify the current device's the internal ID the same way. */
	psDeviceNode->sDevId.ui32InternalID = ui32InternalID;

	PVR_ASSERT(psDevConfig);
	PVR_ASSERT(psDevConfig->pvOSDevice == pvOSDevice);
	PVR_ASSERT(!psDevConfig->psDevNode);

	if ((psDevConfig->eDefaultHeap != PVRSRV_PHYS_HEAP_GPU_LOCAL) &&
	    (psDevConfig->eDefaultHeap != PVRSRV_PHYS_HEAP_CPU_LOCAL))
	{
		PVR_LOG_MSG(PVR_DBG_ERROR, "DEFAULT Heap is invalid, "
		                           "it must be GPU_LOCAL or CPU_LOCAL");
		PVR_LOG_GOTO_IF_ERROR(eError, "SysDevInit", ErrorDeregisterStats);
	}
	PVR_DPF((PVR_DBG_MESSAGE, "Device PhysHeap Default: %s",
		(psDevConfig->eDefaultHeap == PVRSRV_PHYS_HEAP_CPU_LOCAL)
		? "CPU_LOCAL" : "GPU_LOCAL"));

	PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_CREATING);

	psDeviceNode->psDevConfig = psDevConfig;
	psDevConfig->psDevNode = psDeviceNode;

	if (psDevConfig->pfnGpuDomainPower)
	{
		psDeviceNode->eCurrentSysPowerState = psDevConfig->pfnGpuDomainPower(psDeviceNode);
	}
	else
	{
		/* If the System Layer doesn't provide a function to query the power state
		 * of the system hardware, assume the system starting state below. */

#if defined(SUPPORT_AUTOVZ)
		psDeviceNode->eCurrentSysPowerState = PVRSRV_SYS_POWER_STATE_ON;
#else
		psDeviceNode->eCurrentSysPowerState = PVRSRV_SYS_POWER_STATE_OFF;
#endif
	}

#if defined(SUPPORT_PHYSMEM_TEST) && !defined(INTEGRITY_OS) && !defined(__QNXNTO__)
	if (PVRSRV_VZ_MODE_IS(NATIVE, DEVCFG, psDevConfig))
	{
		IMG_UINT32 ui32AppHintDefault = 0;

		/* Read AppHint - Configurable memory test pass count */
		OSCreateAppHintState(&pvAppHintState);
		OSGetAppHintUINT32(APPHINT_NO_DEVICE, pvAppHintState, PhysMemTestPasses,
				&ui32AppHintDefault, &ui32AppHintPhysMemTestPasses);
		OSFreeAppHintState(pvAppHintState);
		pvAppHintState = NULL;

		if (ui32AppHintPhysMemTestPasses > 0)
		{
			eError = PhysMemTest(psDevConfig, ui32AppHintPhysMemTestPasses);
			PVR_LOG_GOTO_IF_ERROR(eError, "PhysMemTest", ErrorSysDevDeInit);
		}
	}
#endif

	/* Next update value will be 0xFFFFFFF7 since sync prim starts with 0xFFFFFFF6.
	 * Has to be set before call to PMRInitDevice(). */
	psDeviceNode->ui32NextMMUInvalidateUpdate = 0xFFFFFFF7U;
#if defined(SUPPORT_PMR_DEFERRED_FREE) || defined(SUPPORT_MMU_DEFERRED_FREE)
	psDeviceNode->uiPowerOffCounter = 0;
	psDeviceNode->uiPowerOffCounterNext = 1;
#endif

	eError = PVRSRVRegisterDeviceDbgTable(psDeviceNode);
	PVR_GOTO_IF_ERROR(eError, ErrorPvzConnectionDeInit);

	eError = PVRSRVPowerLockInit(psDeviceNode);
	PVR_GOTO_IF_ERROR(eError, ErrorUnregisterDbgTable);

	eError = PhysHeapInitDeviceHeaps(psDeviceNode, psDevConfig);
	PVR_GOTO_IF_ERROR(eError, ErrorPowerLockDeInit);

#if defined(SUPPORT_PMR_DEFERRED_FREE)
	eError = PMRInitDevice(psDeviceNode);
	PVR_GOTO_IF_ERROR(eError, ErrorPhysHeapDeInitDeviceHeaps);
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

#if defined(SUPPORT_RGX)
	/* Requirements:
	 *  registered GPU and FW local heaps */
	/*  debug table */
	eError = RGXRegisterDevice(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_LOG_ERROR(eError, "RGXRegisterDevice");
		eError = PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
		goto ErrorPMRDeInitDevice;
	}
#endif

	/* Inform the device layer PhysHeaps are now initialised so that device
	 * specific heaps can be obtained along with carrying out any Vz setup. */
	if (psDeviceNode->pfnPhysMemDeviceHeapsInit != NULL)
	{
		eError = psDeviceNode->pfnPhysMemDeviceHeapsInit(psDeviceNode);
		PVR_GOTO_IF_ERROR(eError, ErrorPMRDeInitDevice);
	}

	/* Carry out initialisation of a dedicated FW MMU data, if the FW CPU has
	 * an MMU separate to the GPU MMU e.g. MIPS based FW. */
	if (psDeviceNode->pfnFwMMUInit != NULL)
	{
		eError = psDeviceNode->pfnFwMMUInit(psDeviceNode);
		PVR_GOTO_IF_ERROR(eError, ErrorFwMMUDeinit);
	}

	eError = SyncServerInit(psDeviceNode);
	PVR_GOTO_IF_ERROR(eError, ErrorDeInitRgx);

	eError = SyncCheckpointInit(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "SyncCheckpointInit", ErrorSyncCheckpointInit);

	/*
	 * This is registered before doing device specific initialisation to ensure
	 * generic device information is dumped first during a debug request.
	 */
	eError = PVRSRVRegisterDeviceDbgRequestNotify(&psDeviceNode->hDbgReqNotify,
		                                          psDeviceNode,
		                                          _SysDebugRequestNotify,
		                                          DEBUG_REQUEST_SYS,
		                                          psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVRegisterDeviceDbgRequestNotify", ErrorRegDbgReqNotify);

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
	eError = InitDVFS(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "InitDVFS", ErrorDVFSInitFail);
#elif defined(SUPPORT_PDVFS) && !defined(NO_HARDWARE)
	eError = InitPDVFS(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "InitPDVFS", ErrorPDVFSInitFail);
#endif

	OSAtomicWrite(&psDeviceNode->iNumClockSpeedChanges, 0);

	OSWRLockCreate(&psDeviceNode->hPageFaultNotifyLock);
	if (psDeviceNode->hPageFaultNotifyLock == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create lock for PF notify list",
		        __func__));
		goto ErrorPageFaultLockFailCreate;
	}

	dllist_init(&psDeviceNode->sMemoryContextPageFaultNotifyListHead);

	PVR_DPF((PVR_DBG_MESSAGE, "Registered device %p", psDeviceNode));
	PVR_DPF((PVR_DBG_MESSAGE, "Register bank address = 0x%08lx",
			 (unsigned long)psDevConfig->sRegsCpuPBase.uiAddr));
	PVR_DPF((PVR_DBG_MESSAGE, "IRQ = %d", psDevConfig->ui32IRQ));

/* SUPPORT_ALT_REGBASE is defined for rogue cores only */
#if defined(SUPPORT_RGX) && defined(SUPPORT_ALT_REGBASE)
	{
		IMG_DEV_PHYADDR sRegsGpuPBase;

		PhysHeapCpuPAddrToDevPAddr(psDeviceNode->apsPhysHeap[PVRSRV_PHYS_HEAP_GPU_LOCAL],
		                           1,
		                           &sRegsGpuPBase,
		                           &(psDeviceNode->psDevConfig->sRegsCpuPBase));

		PVR_LOG(("%s: Using alternate Register bank GPU address: 0x%08lx (orig: 0x%08lx)", __func__,
		         (unsigned long)psDevConfig->sAltRegsGpuPBase.uiAddr,
		         (unsigned long)sRegsGpuPBase.uiAddr));
	}
#endif

	eError = DebugCommonInitDevice(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "DebugCommonInitDevice",
	                      ErrorDestroyMemoryContextPageFaultNotifyListLock);

	/* Create the devicemem_history hook for the device. We need to
	 * have the debug-info instantiated before calling this.
	 */
	if (psDeviceNode->bEnablePFDebug)
	{
		eError = DevicememHistoryDeviceCreate(psDeviceNode);
		PVR_LOG_GOTO_IF_ERROR(eError, "DevicememHistoryDeviceCreate", ErrorDebugCommonDeInitDevice);
	}

#if defined(__linux__)
	/* Register the device specific AppHints so individual AppHints can be
	 * configured before the FW is initialised. This must be called after
	 * DebugCommonInitDevice() above as it depends on the created gpuXX/apphint
	 * DI Group.
	 */
	{
		int iError = pvr_apphint_device_register(psDeviceNode);
		PVR_LOG_IF_FALSE(iError == 0, "pvr_apphint_device_register() failed");
	}
#endif /* defined(__linux__) */

#if defined(SUPPORT_RGX)
	RGXHWPerfInitAppHintCallbacks(psDeviceNode);
#endif

	/* Finally insert the device into the dev-list and set it as active */
	OSWRLockAcquireWrite(psPVRSRVData->hDeviceNodeListLock);
	List_PVRSRV_DEVICE_NODE_InsertTail(&psPVRSRVData->psDeviceNodeList,
									   psDeviceNode);
	psPVRSRVData->ui32RegisteredDevices++;
	OSWRLockReleaseWrite(psPVRSRVData->hDeviceNodeListLock);

	/* Initialise the paravirtualised connection */
	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVCFG, psDevConfig))
	{
		PvzConnectionInit(psDevConfig);
		PVR_GOTO_IF_ERROR(eError, ErrorSysDevDeInit);
	}

	BIT_SET(psDevConfig->psDevNode->ui32VmState, RGXFW_HOST_DRIVER_ID);

	*ppsDeviceNode = psDeviceNode;

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
	/* Register the DVFS device now the device node is present in the dev-list */
	eError = RegisterDVFSDevice(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RegisterDVFSDevice", ErrorRegisterDVFSDeviceFail);
#endif

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	/* Close the process statistics */
	PVRSRVStatsDeregisterProcess(hProcessStats);
#endif


	PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_CREATED);

	return PVRSRV_OK;

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
ErrorRegisterDVFSDeviceFail:
	/* Remove the device from the list */
	OSWRLockAcquireWrite(psPVRSRVData->hDeviceNodeListLock);
	List_PVRSRV_DEVICE_NODE_Remove(psDeviceNode);
	psPVRSRVData->ui32RegisteredDevices--;
	OSWRLockReleaseWrite(psPVRSRVData->hDeviceNodeListLock);

#if defined(__linux__)
	pvr_apphint_device_unregister(psDeviceNode);
#endif /* defined(__linux__) */

	/* Remove the devicemem_history hook if we created it */
	if (psDeviceNode->bEnablePFDebug)
	{
		DevicememHistoryDeviceDestroy(psDeviceNode);
	}
#endif

ErrorDebugCommonDeInitDevice:
	DebugCommonDeInitDevice(psDeviceNode);

ErrorDestroyMemoryContextPageFaultNotifyListLock:
	OSWRLockDestroy(psDeviceNode->hPageFaultNotifyLock);
	psDeviceNode->hPageFaultNotifyLock = NULL;

ErrorPageFaultLockFailCreate:

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
ErrorDVFSInitFail:
#elif defined(SUPPORT_PDVFS) && !defined(NO_HARDWARE)
ErrorPDVFSInitFail:
#endif

	if (psDeviceNode->hDbgReqNotify)
	{
		PVRSRVUnregisterDeviceDbgRequestNotify(psDeviceNode->hDbgReqNotify);
	}

ErrorRegDbgReqNotify:
	SyncCheckpointDeinit(psDeviceNode);

ErrorSyncCheckpointInit:
	SyncServerDeinit(psDeviceNode);

ErrorDeInitRgx:
#if defined(SUPPORT_RGX)
	DevDeInitRGX(psDeviceNode);
#endif
ErrorFwMMUDeinit:
ErrorPMRDeInitDevice:
#if defined(SUPPORT_PMR_DEFERRED_FREE)
	PMRDeInitDevice(psDeviceNode);
ErrorPhysHeapDeInitDeviceHeaps:
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */
	PhysHeapDeInitDeviceHeaps(psDeviceNode);
ErrorPowerLockDeInit:
	PVRSRVPowerLockDeInit(psDeviceNode);
ErrorUnregisterDbgTable:
	PVRSRVUnregisterDeviceDbgTable(psDeviceNode);
ErrorPvzConnectionDeInit:
	psDevConfig->psDevNode = NULL;
	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVCFG, psDevConfig))
	{
		PvzConnectionDeInit(psDevConfig);
	}
ErrorSysDevDeInit:
	SysDevDeInit(psDevConfig);
ErrorDeregisterStats:
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	/* Close the process statistics */
	PVRSRVStatsDeregisterProcess(hProcessStats);
ErrorFreeDeviceNode:
#endif
	OSFreeMemNoStats(psDeviceNode);

	return eError;
}

#if defined(SUPPORT_RGX)
static PVRSRV_ERROR _SetDeviceFlag(const PVRSRV_DEVICE_NODE *psDevice,
                                   const void *psPrivate, IMG_BOOL bValue)
{
	PVRSRV_ERROR eResult = PVRSRV_OK;
	IMG_UINT32 ui32Flag = (IMG_UINT32)((uintptr_t)psPrivate);

	PVR_RETURN_IF_INVALID_PARAM(ui32Flag);
	PVR_RETURN_IF_FALSE(psDevice != APPHINT_OF_DRIVER_NO_DEVICE,
	                    PVRSRV_ERROR_INVALID_PARAMS);

	eResult = RGXSetDeviceFlags((PVRSRV_RGXDEV_INFO *)psDevice->pvDevice,
	                            ui32Flag, bValue);

	return eResult;
}

static PVRSRV_ERROR _ReadDeviceFlag(const PVRSRV_DEVICE_NODE *psDevice,
                                   const void *psPrivate, IMG_BOOL *pbValue)
{
	PVRSRV_ERROR eResult = PVRSRV_OK;
	IMG_UINT32 ui32Flag = (IMG_UINT32)((uintptr_t)psPrivate);
	IMG_UINT32 ui32State;

	PVR_RETURN_IF_INVALID_PARAM(ui32Flag);
	PVR_RETURN_IF_FALSE(psDevice != APPHINT_OF_DRIVER_NO_DEVICE,
	                    PVRSRV_ERROR_INVALID_PARAMS);

	eResult = RGXGetDeviceFlags((PVRSRV_RGXDEV_INFO *)psDevice->pvDevice,
	                            &ui32State);

	if (PVRSRV_OK == eResult)
	{
		*pbValue = (ui32State & ui32Flag)? IMG_TRUE: IMG_FALSE;
	}

	return eResult;
}
static PVRSRV_ERROR _SetStateFlag(const PVRSRV_DEVICE_NODE *psDevice,
                                  const void *psPrivate, IMG_BOOL bValue)
{
	PVRSRV_ERROR eResult = PVRSRV_OK;
	IMG_UINT32 ui32Flag = (IMG_UINT32)((uintptr_t)psPrivate);

	PVR_RETURN_IF_INVALID_PARAM(ui32Flag);
	PVR_RETURN_IF_FALSE(psDevice != APPHINT_OF_DRIVER_NO_DEVICE,
	                    PVRSRV_ERROR_INVALID_PARAMS);

	eResult = RGXStateFlagCtrl((PVRSRV_RGXDEV_INFO *)psDevice->pvDevice,
	                           ui32Flag, NULL, bValue);

	return eResult;
}

static PVRSRV_ERROR _ReadStateFlag(const PVRSRV_DEVICE_NODE *psDevice,
                                   const void *psPrivate, IMG_BOOL *pbValue)
{
	IMG_UINT32 ui32Flag = (IMG_UINT32)((uintptr_t)psPrivate);
	IMG_UINT32 ui32State;
	PVRSRV_RGXDEV_INFO *psDevInfo;

	PVR_RETURN_IF_INVALID_PARAM(ui32Flag);
	PVR_RETURN_IF_FALSE(psDevice != APPHINT_OF_DRIVER_NO_DEVICE,
	                    PVRSRV_ERROR_INVALID_PARAMS);

	psDevInfo = (PVRSRV_RGXDEV_INFO *)psDevice->pvDevice;

	RGXFwSharedMemCacheOpValue(psDevInfo->psRGXFWIfFwSysData->ui32ConfigFlags,
	                           INVALIDATE);
	ui32State = psDevInfo->psRGXFWIfFwSysData->ui32ConfigFlags;

	if (pbValue)
	{
		*pbValue = (ui32State & ui32Flag)? IMG_TRUE: IMG_FALSE;
	}

	return PVRSRV_OK;
}
#endif

PVRSRV_ERROR PVRSRVCommonDeviceInitialise(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_BOOL bInitSuccessful = IMG_FALSE;
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	IMG_HANDLE hProcessStats;
#endif
	PVRSRV_ERROR eError;

	PDUMPCOMMENT(psDeviceNode, "Common Device Initialisation");

	MULTI_DEVICE_BRINGUP_DPF("PVRSRVCommonDeviceInitialise: DevId %d", psDeviceNode->sDevId.i32KernelDeviceID);

	if (psDeviceNode->eDevState != PVRSRV_DEVICE_STATE_CREATED)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Device already initialised", __func__));
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	/* Allocate an OSEventObject to use in the Freeze / Thaw transitioning to
	 * allow LBIST requests to be serviced. This object is held for the life of
	 * this device-node.
	 */
	eError = OSEventObjectCreate("PVRSRV_DEVICE_THREAD_EVENT_OBJECT",
	                             &psDeviceNode->hDeviceThreadEvObj);
	PVR_LOG_RETURN_IF_ERROR(eError, "OSEventObjectCreate");

	eError = OSEventObjectOpen(psDeviceNode->hDeviceThreadEvObj,
	                           &psDeviceNode->hDeviceFreezeThaw);

	if (PVRSRV_OK != eError)
	{
		OSEventObjectDestroy(psDeviceNode->hDeviceThreadEvObj);

		PVR_LOG_RETURN_IF_ERROR(eError, "OSEventObjectOpen");
	}

	/* Initial zero-set for the number of Frozen threads */
	OSAtomicWrite(&psDeviceNode->iFreezeCount, 0);
	OSAtomicWrite(&psDeviceNode->iTotalFreezes, 0);

	/* Initial zero-set for number of active threads on this device */
	OSAtomicWrite(&psDeviceNode->iThreadsActive, 0);

	/* Allocate devmem_history backing store for the device if we have
	 * EnablePageFaultDebug set
	 */
	if (psDeviceNode->bEnablePFDebug)
	{
		eError = DevicememHistoryDeviceInit(psDeviceNode);
		PVR_LOG_RETURN_IF_ERROR(eError, "DevicememHistoryDeviceInit");
	}

#if defined(PDUMP)
#if (PVRSRV_DEVICE_INIT_MODE == PVRSRV_LINUX_DEV_INIT_ON_CONNECT)
	{
		PVRSRV_DATA *psSRVData = PVRSRVGetPVRSRVData();

		/* If first connection, bind this and future PDump clients to use this device */
		if (psSRVData->ui32PDumpBoundDevice == PVRSRV_MAX_DEVICES)
		{
			psSRVData->ui32PDumpBoundDevice = psDeviceNode->sDevId.ui32InternalID;
		}
	}
#endif
#endif

	/* Initialise Connection_Data access mechanism */
	dllist_init(&psDeviceNode->sConnections);
	eError = OSLockCreate(&psDeviceNode->hConnectionsLock);
	PVR_LOG_RETURN_IF_ERROR(eError, "OSLockCreate");

	/* Allocate process statistics */
#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	eError = PVRSRVStatsRegisterProcess(&hProcessStats);
	PVR_LOG_RETURN_IF_ERROR(eError, "PVRSRVStatsRegisterProcess");
#endif

	eError = MMU_InitDevice(psDeviceNode);
	PVR_LOG_RETURN_IF_ERROR(eError, "MMU_InitDevice");

#if defined(SUPPORT_RGX)
	eError = RGXInit(psDeviceNode);
	PVR_LOG_GOTO_IF_ERROR(eError, "RGXInit", Exit);
#endif

	bInitSuccessful = IMG_TRUE;

#if defined(SUPPORT_RGX)
Exit:
#endif
	eError = PVRSRVDeviceFinalise(psDeviceNode, bInitSuccessful);
	PVR_LOG_IF_ERROR(eError, "PVRSRVDeviceFinalise");

#if defined(SUPPORT_RGX)
	if (bInitSuccessful && (eError == PVRSRV_OK))
	{
		RGXInitMultiCoreInfo(psDeviceNode);

		if (!PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_DisableClockGating,
			                                  _ReadStateFlag, _SetStateFlag,
			                                  APPHINT_OF_DRIVER_NO_DEVICE,
			                                  (void*)((uintptr_t)RGXFWIF_INICFG_DISABLE_CLKGATING_EN));
			PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_DisableDMOverlap,
			                                  _ReadStateFlag, _SetStateFlag,
			                                  APPHINT_OF_DRIVER_NO_DEVICE,
			                                  (void*)((uintptr_t)RGXFWIF_INICFG_DISABLE_DM_OVERLAP));
			PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_AssertOnHWRTrigger,
			                                  _ReadStateFlag, _SetStateFlag,
			                                  psDeviceNode,
			                                  (void*)((uintptr_t)RGXFWIF_INICFG_ASSERT_ON_HWR_TRIGGER));
			PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_AssertOutOfMemory,
			                                  _ReadStateFlag, _SetStateFlag,
			                                  psDeviceNode,
			                                  (void*)((uintptr_t)RGXFWIF_INICFG_ASSERT_ON_OUTOFMEMORY));
			PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_CheckMList,
			                                  _ReadStateFlag, _SetStateFlag,
			                                  psDeviceNode,
			                                  (void*)((uintptr_t)RGXFWIF_INICFG_CHECK_MLIST_EN));
		}

		PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_DisableFEDLogging,
		                                  _ReadDeviceFlag, _SetDeviceFlag,
		                                  psDeviceNode,
		                                  (void*)((uintptr_t)RGXKM_DEVICE_STATE_DISABLE_DW_LOGGING_EN));
		PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_ZeroFreelist,
		                                  _ReadDeviceFlag, _SetDeviceFlag,
		                                  psDeviceNode,
		                                  (void*)((uintptr_t)RGXKM_DEVICE_STATE_ZERO_FREELIST));
		PVRSRVAppHintRegisterHandlersBOOL(APPHINT_ID_DisablePDumpPanic,
		                                  RGXQueryPdumpPanicDisable, RGXSetPdumpPanicDisable,
		                                  psDeviceNode,
		                                  NULL);
	}
#endif

#if defined(PVRSRV_ENABLE_PROCESS_STATS)
	/* Close the process statistics */
	PVRSRVStatsDeregisterProcess(hProcessStats);
#endif

	return eError;
}

void PVRSRVCommonDeviceDestroy(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_DATA				*psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_ERROR			eError;
#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
	IMG_BOOL				bForceUnload = IMG_FALSE;
	PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus = OSAtomicRead(&psDeviceNode->eHealthStatus);

	if ((PVRSRVGetPVRSRVData()->eServicesState != PVRSRV_SERVICES_STATE_OK) ||
		(eHealthStatus == PVRSRV_DEVICE_HEALTH_STATUS_DEAD) ||
		(eHealthStatus == PVRSRV_DEVICE_HEALTH_STATUS_FAULT))
	{
		bForceUnload = IMG_TRUE;
	}
#endif

	/* Remove DI hook for the devicemem_history for this device (if any).
	 * The associated devicemem_history buffers are freed by the final
	 * call to DevicememHistoryDeInitKM() as they are used asynchronously
	 * by other parts of the DDK.
	 */
	if (psDeviceNode->bEnablePFDebug)
	{
		DevicememHistoryDeviceDestroy(psDeviceNode);
	}

	MULTI_DEVICE_BRINGUP_DPF("PVRSRVCommonDeviceDestroy: DevId %d", psDeviceNode->sDevId.i32KernelDeviceID);

	PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_DEINIT);

	PVRSRVCleanupThreadWaitForDevice(psDeviceNode);

	OSWRLockAcquireWrite(psPVRSRVData->hDeviceNodeListLock);
	List_PVRSRV_DEVICE_NODE_Remove(psDeviceNode);
	psPVRSRVData->ui32RegisteredDevices--;
	OSWRLockReleaseWrite(psPVRSRVData->hDeviceNodeListLock);

	PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_DESTRUCTING);

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
	UnregisterDVFSDevice(psDeviceNode);
#endif

#if defined(__linux__)
	pvr_apphint_device_unregister(psDeviceNode);
#endif /* defined(__linux__) */

	DebugCommonDeInitDevice(psDeviceNode);


#if defined(SUPPORT_FALLBACK_FENCE_SYNC)
	SyncFbDeregisterDevice(psDeviceNode);
#endif
	/* Counter part to what gets done in PVRSRVDeviceFinalise */
	if (psDeviceNode->hSyncCheckpointContext)
	{
		SyncCheckpointContextDestroy(psDeviceNode->hSyncCheckpointContext);
		psDeviceNode->hSyncCheckpointContext = NULL;
	}
	if (psDeviceNode->hSyncPrimContext)
	{
		if (psDeviceNode->psMMUCacheSyncPrim)
		{
			PVRSRV_CLIENT_SYNC_PRIM *psSync = psDeviceNode->psMMUCacheSyncPrim;

			/* Ensure there are no pending MMU Cache Ops in progress before freeing this sync. */
			eError = PVRSRVPollForValueKM(psDeviceNode,
			                              psSync->pui32LinAddr,
			                              psDeviceNode->ui32NextMMUInvalidateUpdate-1,
			                              0xFFFFFFFF,
			                              POLL_FLAG_LOG_ERROR,
			                              NULL);
			PVR_LOG_IF_ERROR(eError, "PVRSRVPollForValueKM");

			/* Important to set the device node pointer to NULL
			 * before we free the sync-prim to make sure we don't
			 * defer the freeing of the sync-prim's page tables itself.
			 * The sync is used to defer the MMU page table
			 * freeing. */
			psDeviceNode->psMMUCacheSyncPrim = NULL;

			/* Free general purpose sync primitive */
			SyncPrimFree(psSync);
		}

		SyncPrimContextDestroy(psDeviceNode->hSyncPrimContext);
		psDeviceNode->hSyncPrimContext = NULL;
	}

	eError = PVRSRVPowerLock(psDeviceNode);
	if (eError == PVRSRV_OK)
	{
		IMG_BOOL bHasPowerLock = IMG_TRUE;

#if defined(PVRSRV_FORCE_UNLOAD_IF_BAD_STATE)
		/*
		 * Firmware probably not responding if bForceUnload is set, but we still want to unload the
		 * driver.
		 */
		if (!bForceUnload)
#endif
		{
			/* Force idle device */
			eError = PVRSRVDeviceIdleRequestKM(psDeviceNode, NULL, IMG_TRUE);
			PVR_LOG_IF_ERROR(eError, "PVRSRVDeviceIdleRequestKM");
			if (eError == PVRSRV_ERROR_PWLOCK_RELEASED_REACQ_FAILED)
			{
				bHasPowerLock = IMG_FALSE;
			}
		}

		if (bHasPowerLock)
		{
			/* Power down the device if necessary */
			eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
												 PVRSRV_DEV_POWER_STATE_OFF,
												 PVRSRV_POWER_FLAGS_FORCED);
			PVRSRVPowerUnlock(psDeviceNode);

			if (eError != PVRSRV_OK)
			{
				PVR_LOG_ERROR(eError, "PVRSRVSetDevicePowerStateKM");

				PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
			}
		}
	}

#if defined(SUPPORT_LINUX_DVFS) && !defined(NO_HARDWARE)
	DeinitDVFS(psDeviceNode);
#elif defined(SUPPORT_PDVFS) && !defined(NO_HARDWARE)
	DeinitPDVFS(psDeviceNode);
#endif

	if (psDeviceNode->hDbgReqNotify)
	{
		PVRSRVUnregisterDeviceDbgRequestNotify(psDeviceNode->hDbgReqNotify);
	}

	SyncCheckpointDeinit(psDeviceNode);

	SyncServerDeinit(psDeviceNode);

	MMU_DeInitDevice(psDeviceNode);

#if defined(SUPPORT_RGX)
	DevDeInitRGX(psDeviceNode);
#endif

#if defined(SUPPORT_PMR_DEFERRED_FREE)
	/* must be called before PhysHeapDeInitDeviceHeaps() */
	PMRDeInitDevice(psDeviceNode);
#endif /* defined(SUPPORT_PMR_DEFERRED_FREE) */

	PhysHeapDeInitDeviceHeaps(psDeviceNode);
	PVRSRVPowerLockDeInit(psDeviceNode);

	PVRSRVUnregisterDeviceDbgTable(psDeviceNode);

	if (psDeviceNode->hPageFaultNotifyLock != NULL)
	{
		OSWRLockDestroy(psDeviceNode->hPageFaultNotifyLock);
	}

	/* Release the Connection-Data lock as late as possible. */
	if (psDeviceNode->hConnectionsLock)
	{
		OSLockDestroy(psDeviceNode->hConnectionsLock);
	}

	/* Release the hDeviceThreadEvObj as late as possible. */
	if (psDeviceNode->hDeviceThreadEvObj)
	{
		OSEventObjectClose(psDeviceNode->hDeviceFreezeThaw);

		OSEventObjectDestroy(psDeviceNode->hDeviceThreadEvObj);
	}

	PVR_ASSERT(OSAtomicRead(&psDeviceNode->iFreezeCount) == 0);

	psDeviceNode->psDevConfig->psDevNode = NULL;

	if (!PVRSRV_VZ_MODE_IS(NATIVE, DEVNODE, psDeviceNode))
	{
		PvzConnectionDeInit(psDeviceNode->psDevConfig);
	}
	SysDevDeInit(psDeviceNode->psDevConfig);

	OSFreeMemNoStats(psDeviceNode);
}

/**************************************************************************/ /*!
@Function     PVRSRVDeviceFinalise
@Description  Performs the final parts of device initialisation.
@Input        psDeviceNode            Device node of the device to finish
                                      initialising
@Input        bInitSuccessful         Whether or not device specific
                                      initialisation was successful
@Return       PVRSRV_ERROR     PVRSRV_OK on success and an error otherwise
*/ /***************************************************************************/
PVRSRV_ERROR PVRSRVDeviceFinalise(PVRSRV_DEVICE_NODE *psDeviceNode,
											   IMG_BOOL bInitSuccessful)
{
	PVRSRV_ERROR eError;
	__maybe_unused PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO *)(psDeviceNode->pvDevice);

	if (bInitSuccessful)
	{
		eError = SyncCheckpointContextCreate(psDeviceNode,
											 &psDeviceNode->hSyncCheckpointContext);
		PVR_LOG_GOTO_IF_ERROR(eError, "SyncCheckpointContextCreate", ErrorExit);
#if defined(SUPPORT_FALLBACK_FENCE_SYNC)
		eError = SyncFbRegisterDevice(psDeviceNode);
		PVR_GOTO_IF_ERROR(eError, ErrorExit);
#endif
		eError = SyncPrimContextCreate(psDeviceNode,
									   &psDeviceNode->hSyncPrimContext);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "SyncPrimContextCreate");
			SyncCheckpointContextDestroy(psDeviceNode->hSyncCheckpointContext);
			goto ErrorExit;
		}

		/* Allocate MMU cache invalidate sync */
		eError = SyncPrimAlloc(psDeviceNode->hSyncPrimContext,
							   &psDeviceNode->psMMUCacheSyncPrim,
							   "pvrsrv dev MMU cache");
		PVR_LOG_GOTO_IF_ERROR(eError, "SyncPrimAlloc", ErrorExit);

		/* Set the sync prim value to a much higher value near the
		 * wrapping range. This is so any wrapping bugs would be
		 * seen early in the driver start-up.
		 */
		SyncPrimSet(psDeviceNode->psMMUCacheSyncPrim, 0xFFFFFFF6UL);

		eError = PVRSRVPowerLock(psDeviceNode);
		PVR_LOG_GOTO_IF_ERROR(eError, "PVRSRVPowerLock", ErrorExit);

		/*
		 * Always ensure a single power on command appears in the pdump. This
		 * should be the only power related call outside of PDUMPPOWCMDSTART
		 * and PDUMPPOWCMDEND.
		 */
		eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
											 PVRSRV_DEV_POWER_STATE_ON,
											 PVRSRV_POWER_FLAGS_FORCED);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed to set device %p power state to 'on' (%s)",
					 __func__, psDeviceNode, PVRSRVGetErrorString(eError)));
			PVRSRVPowerUnlock(psDeviceNode);
			goto ErrorExit;
		}

#if defined(SUPPORT_FW_VIEW_EXTRA_DEBUG)
		eError = ValidateFWOnLoad(psDeviceNode->pvDevice);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "ValidateFWOnLoad");
			PVRSRVPowerUnlock(psDeviceNode);
			return eError;
		}
#endif

		/*
		 * Guest driver must do a runtime compatibility check against the
		 * data provided by the Firmware.
		 */
		if (PVRSRV_VZ_MODE_IS(GUEST, DEVNODE, psDeviceNode))
		{
			eError = PVRSRVDevInitCompatCheck(psDeviceNode);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,
						 "%s: Failed compatibility check for device %p (%s)",
						 __func__, psDeviceNode, PVRSRVGetErrorString(eError)));
				PVRSRVPowerUnlock(psDeviceNode);
				PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
				goto ErrorExit;
			}
		}

		PDUMPPOWCMDSTART(psDeviceNode);

		/* Force the device to idle if its default power state is off */
		eError = PVRSRVDeviceIdleRequestKM(psDeviceNode,
										   &PVRSRVDeviceIsDefaultStateOFF,
										   IMG_TRUE);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "PVRSRVDeviceIdleRequestKM");
			if (eError != PVRSRV_ERROR_PWLOCK_RELEASED_REACQ_FAILED)
			{
				PVRSRVPowerUnlock(psDeviceNode);
			}
			goto ErrorExit;
		}

		/* Place device into its default power state. */
		eError = PVRSRVSetDevicePowerStateKM(psDeviceNode,
											 PVRSRV_DEV_POWER_STATE_DEFAULT,
											 PVRSRV_POWER_FLAGS_FORCED);
		PDUMPPOWCMDEND(psDeviceNode);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
					 "%s: Failed to set device %p into its default power state (%s)",
					 __func__, psDeviceNode, PVRSRVGetErrorString(eError)));

			PVRSRVPowerUnlock(psDeviceNode);
			PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
			goto ErrorExit;
		}

		PVRSRVPowerUnlock(psDeviceNode);

		/*
		 * If PDUMP is enabled and RGX device is supported, then initialise the
		 * performance counters that can be further modified in PDUMP. Then,
		 * before ending the init phase of the pdump, drain the commands put in
		 * the kCCB during the init phase.
		 */
#if defined(SUPPORT_RGX)
#if defined(PDUMP)
		{
			eError = RGXInitHWPerfCounters(psDeviceNode);
			PVR_LOG_GOTO_IF_ERROR(eError, "RGXInitHWPerfCounters", ErrorExit);

			eError = RGXPdumpDrainKCCB(psDevInfo,
									   psDevInfo->psKernelCCBCtlLocal->ui32WriteOffset);
			PVR_LOG_GOTO_IF_ERROR(eError, "RGXPdumpDrainKCCB", ErrorExit);
		}
#endif
#endif /* defined(SUPPORT_RGX) */
		/* Now that the device(s) are fully initialised set them as active */
		PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_ACTIVE);
		eError = PVRSRV_OK;
	}
	else
	{
		/* Initialisation failed so set the device(s) into a bad state */
		PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_BAD);
		eError = PVRSRV_ERROR_NOT_INITIALISED;
	}

	/* Give PDump control a chance to end the init phase, depends on OS */
	PDUMPENDINITPHASE(psDeviceNode);
	return eError;

ErrorExit:
	/* Initialisation failed so set the device(s) into a bad state */
	PVRSRVDeviceSetState(psDeviceNode, PVRSRV_DEVICE_STATE_BAD);

	return eError;
}

PVRSRV_ERROR PVRSRVDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/* Only check devices which specify a compatibility check callback */
	if (psDeviceNode->pfnInitDeviceCompatCheck)
		return psDeviceNode->pfnInitDeviceCompatCheck(psDeviceNode);
	else
		return PVRSRV_OK;
}

/*
	PollForValueKM
*/
static
PVRSRV_ERROR PollForValueKM (volatile IMG_UINT32 __iomem           *pui32LinMemAddr,
										  IMG_UINT32                ui32Value,
										  IMG_UINT32                ui32Mask,
										  IMG_UINT32                ui32Timeoutus,
										  IMG_UINT32                ui32PollPeriodus,
										  POLL_FLAGS                ePollFlags,
										  PFN_INVALIDATE_CACHEFUNC  pfnFwInvalidate)
{
#if defined(NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(pui32LinMemAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Value);
	PVR_UNREFERENCED_PARAMETER(ui32Mask);
	PVR_UNREFERENCED_PARAMETER(ui32Timeoutus);
	PVR_UNREFERENCED_PARAMETER(ui32PollPeriodus);
	PVR_UNREFERENCED_PARAMETER(ePollFlags);
	return PVRSRV_OK;
#else
	IMG_UINT32 ui32ActualValue = 0xFFFFFFFFU; /* Initialiser only required to prevent incorrect warning */

	LOOP_UNTIL_TIMEOUT_US(ui32Timeoutus)
	{
		if (pfnFwInvalidate)
		{
			pfnFwInvalidate((const volatile void __force *)pui32LinMemAddr,
			                sizeof(*pui32LinMemAddr),
			                PVRSRV_CACHE_OP_INVALIDATE);
		}

		ui32ActualValue = OSReadHWReg32((void __iomem *)pui32LinMemAddr, 0) & ui32Mask;

		if (ui32ActualValue == ui32Value)
		{
			return PVRSRV_OK;
		}

		if (gpsPVRSRVData->eServicesState != PVRSRV_SERVICES_STATE_OK)
		{
			return PVRSRV_ERROR_TIMEOUT;
		}

#define ONE_MSEC_IN_USECS	(1000U)

		if (ui32PollPeriodus <= ONE_MSEC_IN_USECS)
		{
			OSWaitus(ui32PollPeriodus);
		}
		else
		{
			OSSleepms(ui32PollPeriodus / ONE_MSEC_IN_USECS);
		}
	} END_LOOP_UNTIL_TIMEOUT_US();

	if (BITMASK_HAS(ePollFlags, POLL_FLAG_LOG_ERROR))
	{
		PVR_DPF((PVR_DBG_ERROR,
		         "PollForValueKM: Timeout. Expected 0x%x but found 0x%x (mask 0x%x).",
		         ui32Value, ui32ActualValue, ui32Mask));
	}

	return PVRSRV_ERROR_TIMEOUT;
#endif /* NO_HARDWARE */
}


/*
	PVRSRVPollForValueKM
*/
PVRSRV_ERROR PVRSRVPollForValueKM (PVRSRV_DEVICE_NODE  *psDevNode,
												volatile IMG_UINT32	__iomem   *pui32LinMemAddr,
												IMG_UINT32                     ui32Value,
												IMG_UINT32                     ui32Mask,
												POLL_FLAGS                     ePollFlags,
												PFN_INVALIDATE_CACHEFUNC       pfnFwInvalidate)
{
	PVRSRV_ERROR eError;

	eError = PollForValueKM(pui32LinMemAddr, ui32Value, ui32Mask,
						  MAX_HW_TIME_US,
						  MAX_HW_TIME_US/WAIT_TRY_COUNT,
						  ePollFlags,
						  pfnFwInvalidate);
	if (eError != PVRSRV_OK && BITMASK_HAS(ePollFlags, POLL_FLAG_DEBUG_DUMP))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed! Error(%s) CPU linear address(%p) Expected value(%u)",
		                        __func__, PVRSRVGetErrorString(eError),
								pui32LinMemAddr, ui32Value));
		PVRSRVDebugRequest(psDevNode, DEBUG_REQUEST_VERBOSITY_MAX, NULL, NULL);
	}

	return eError;
}

PVRSRV_ERROR
PVRSRVWaitForValueKM(volatile IMG_UINT32 __iomem   *pui32LinMemAddr,
                     IMG_UINT32                    ui32Value,
                     IMG_UINT32                    ui32Mask,
                     PFN_INVALIDATE_CACHEFUNC      pfnFwInvalidate)
{
#if defined(NO_HARDWARE)
	PVR_UNREFERENCED_PARAMETER(pui32LinMemAddr);
	PVR_UNREFERENCED_PARAMETER(ui32Value);
	PVR_UNREFERENCED_PARAMETER(ui32Mask);
	return PVRSRV_OK;
#else

	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	IMG_HANDLE hOSEvent;
	PVRSRV_ERROR eError;
	PVRSRV_ERROR eErrorWait;
	IMG_UINT32 ui32ActualValue;

	eError = OSEventObjectOpen(psPVRSRVData->hGlobalEventObject, &hOSEvent);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectOpen", EventObjectOpenError);

	eError = PVRSRV_ERROR_TIMEOUT; /* Initialiser for following loop */
	LOOP_UNTIL_TIMEOUT_US(MAX_HW_TIME_US)
	{
		if (pfnFwInvalidate)
		{
			pfnFwInvalidate((const volatile void __force *)pui32LinMemAddr,
			                sizeof(*pui32LinMemAddr),
			                PVRSRV_CACHE_OP_INVALIDATE);
		}

		ui32ActualValue = (OSReadDeviceMem32(pui32LinMemAddr) & ui32Mask);

		if (ui32ActualValue == ui32Value)
		{
			/* Expected value has been found */
			eError = PVRSRV_OK;
			break;
		}
		else if (psPVRSRVData->eServicesState != PVRSRV_SERVICES_STATE_OK)
		{
			/* Services in bad state, don't wait any more */
			eError = PVRSRV_ERROR_NOT_READY;
			break;
		}
		else
		{
			/* wait for event and retry */
			eErrorWait = OSEventObjectWait(hOSEvent);
			if (eErrorWait != PVRSRV_OK  &&  eErrorWait != PVRSRV_ERROR_TIMEOUT)
			{
				PVR_DPF((PVR_DBG_WARNING, "%s: Failed with error %d. Found value 0x%x but was expected "
				         "to be 0x%x (Mask 0x%08x). Retrying",
						 __func__,
						 eErrorWait,
						 ui32ActualValue,
						 ui32Value,
						 ui32Mask));
			}
		}
	} END_LOOP_UNTIL_TIMEOUT_US();

	OSEventObjectClose(hOSEvent);

	/* One last check in case the object wait ended after the loop timeout... */
	if (eError != PVRSRV_OK &&
	    (OSReadDeviceMem32(pui32LinMemAddr) & ui32Mask) == ui32Value)
	{
		eError = PVRSRV_OK;
	}

	/* Provide event timeout information to aid the Device Watchdog Thread... */
	if (eError == PVRSRV_OK)
	{
		psPVRSRVData->ui32GEOConsecutiveTimeouts = 0;
	}
	else if (eError == PVRSRV_ERROR_TIMEOUT)
	{
		psPVRSRVData->ui32GEOConsecutiveTimeouts++;
	}

EventObjectOpenError:

	return eError;

#endif /* NO_HARDWARE */
}

int PVRSRVGetDriverStatus(void)
{
	return PVRSRVGetPVRSRVData()->eServicesState;
}

/*
	PVRSRVSystemHasCacheSnooping
*/
IMG_BOOL PVRSRVSystemHasCacheSnooping(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	if ((psDevConfig->eCacheSnoopingMode != PVRSRV_DEVICE_SNOOP_NONE) &&
		(psDevConfig->eCacheSnoopingMode != PVRSRV_DEVICE_SNOOP_EMULATED))
	{
		return IMG_TRUE;
	}
	return IMG_FALSE;
}

IMG_BOOL PVRSRVSystemSnoopingIsEmulated(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	if (psDevConfig->eCacheSnoopingMode == PVRSRV_DEVICE_SNOOP_EMULATED)
	{
		return IMG_TRUE;
	}
	return IMG_FALSE;
}

IMG_BOOL PVRSRVSystemSnoopingOfCPUCache(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	if ((psDevConfig->eCacheSnoopingMode == PVRSRV_DEVICE_SNOOP_CPU_ONLY) ||
		(psDevConfig->eCacheSnoopingMode == PVRSRV_DEVICE_SNOOP_CROSS))
	{
		return IMG_TRUE;
	}
	return IMG_FALSE;
}

IMG_BOOL PVRSRVSystemHasNonMappableLocalMemory(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	return psDevConfig->bHasNonMappableLocalMemory;
}

/*
	PVRSRVSystemWaitCycles
*/
void PVRSRVSystemWaitCycles(PVRSRV_DEVICE_CONFIG *psDevConfig, IMG_UINT32 ui32Cycles)
{
	/* Delay in us */
	IMG_UINT32 ui32Delayus = 1;

	/* obtain the device freq */
	if (psDevConfig->pfnClockFreqGet != NULL)
	{
		IMG_UINT32 ui32DeviceFreq;

		ui32DeviceFreq = psDevConfig->pfnClockFreqGet(psDevConfig->hSysData);

		ui32Delayus = (ui32Cycles*1000000)/ui32DeviceFreq;

		if (ui32Delayus == 0)
		{
			ui32Delayus = 1;
		}
	}

	OSWaitus(ui32Delayus);
}

static void *
PVRSRVSystemInstallDeviceLISR_Match_AnyVaCb(PVRSRV_DEVICE_NODE *psDeviceNode,
											va_list va)
{
	void *pvOSDevice = va_arg(va, void *);

	if (psDeviceNode->psDevConfig->pvOSDevice == pvOSDevice)
	{
		return psDeviceNode;
	}

	return NULL;
}

PVRSRV_ERROR PVRSRVSystemInstallDeviceLISR(void *pvOSDevice,
										   IMG_UINT32 ui32IRQ,
										   const IMG_CHAR *pszName,
										   PFN_LISR pfnLISR,
										   void *pvData,
										   IMG_HANDLE *phLISRData)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DEVICE_NODE *psDeviceNode;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);
	psDeviceNode =
		List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
									   &PVRSRVSystemInstallDeviceLISR_Match_AnyVaCb,
									   pvOSDevice);
	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);

	if (!psDeviceNode)
	{
		/* Device can't be found in the list so it isn't in the system */
		PVR_DPF((PVR_DBG_ERROR, "%s: device %p with irq %d is not present",
				 __func__, pvOSDevice, ui32IRQ));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	return SysInstallDeviceLISR(psDeviceNode->psDevConfig->hSysData, ui32IRQ,
								pszName, pfnLISR, pvData, phLISRData);
}

PVRSRV_ERROR PVRSRVSystemUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return SysUninstallDeviceLISR(hLISRData);
}

#if defined(SUPPORT_GPUVIRT_VALIDATION) && defined(EMULATOR)
/* functions only used on rogue, but header defining them is common */
void SetAxiProtOSid(IMG_HANDLE hSysData, IMG_UINT32 ui32OSid, IMG_BOOL bState)
{
	SysSetAxiProtOSid(hSysData, ui32OSid, bState);
}

void SetTrustedDeviceAceEnabled(IMG_HANDLE hSysData)
{
	SysSetTrustedDeviceAceEnabled(hSysData);
}
#endif

#if defined(SUPPORT_RGX)
PVRSRV_ERROR PVRSRVCreateHWPerfHostThread(IMG_UINT32 ui32Timeout)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (!ui32Timeout)
		return PVRSRV_ERROR_INVALID_PARAMS;

	OSLockAcquire(gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock);

	/* Create only once */
	if (gpsPVRSRVData->hHWPerfHostPeriodicThread == NULL)
	{
		/* Create the HWPerf event object */
		eError = OSEventObjectCreate("PVRSRV_HWPERFHOSTPERIODIC_EVENTOBJECT", &gpsPVRSRVData->hHWPerfHostPeriodicEvObj);
		PVR_LOG_IF_ERROR(eError, "OSEventObjectCreate");

		if (eError == PVRSRV_OK)
		{
			gpsPVRSRVData->bHWPerfHostThreadStop = IMG_FALSE;
			gpsPVRSRVData->ui32HWPerfHostThreadTimeout = ui32Timeout;
			/* Create a thread which is used to periodically emit host stream packets */
			eError = OSThreadCreate(&gpsPVRSRVData->hHWPerfHostPeriodicThread,
				"pvr_hwperf_host",
				HWPerfPeriodicHostEventsThread,
				NULL, IMG_TRUE, gpsPVRSRVData);
			PVR_LOG_IF_ERROR(eError, "OSThreadCreate");
		}
	}
	/* If the thread has already been created then just update the timeout and wake up thread */
	else
	{
		gpsPVRSRVData->ui32HWPerfHostThreadTimeout = ui32Timeout;
		eError = OSEventObjectSignal(gpsPVRSRVData->hHWPerfHostPeriodicEvObj);
		PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
	}

	OSLockRelease(gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock);
	return eError;
}

PVRSRV_ERROR PVRSRVDestroyHWPerfHostThread(void)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	OSLockAcquire(gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock);

	/* Stop and cleanup the HWPerf periodic thread */
	if (gpsPVRSRVData->hHWPerfHostPeriodicThread)
	{
		if (gpsPVRSRVData->hHWPerfHostPeriodicEvObj)
		{
			gpsPVRSRVData->bHWPerfHostThreadStop = IMG_TRUE;
			eError = OSEventObjectSignal(gpsPVRSRVData->hHWPerfHostPeriodicEvObj);
			PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
		}
		LOOP_UNTIL_TIMEOUT_US(OS_THREAD_DESTROY_TIMEOUT_US)
		{
			eError = OSThreadDestroy(gpsPVRSRVData->hHWPerfHostPeriodicThread);
			if (PVRSRV_OK == eError)
			{
				gpsPVRSRVData->hHWPerfHostPeriodicThread = NULL;
				break;
			}
			OSWaitus(OS_THREAD_DESTROY_TIMEOUT_US/OS_THREAD_DESTROY_RETRY_COUNT);
		} END_LOOP_UNTIL_TIMEOUT_US();
		PVR_LOG_IF_ERROR(eError, "OSThreadDestroy");

		if (gpsPVRSRVData->hHWPerfHostPeriodicEvObj)
		{
			eError = OSEventObjectDestroy(gpsPVRSRVData->hHWPerfHostPeriodicEvObj);
			gpsPVRSRVData->hHWPerfHostPeriodicEvObj = NULL;
			PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
		}
	}

	OSLockRelease(gpsPVRSRVData->hHWPerfHostPeriodicThread_Lock);
	return eError;
}
#endif

/*
 * Scan the list of known devices until we find the specific instance or
 * exhaust the list
 */
PVRSRV_DEVICE_NODE *PVRSRVGetDeviceInstance(IMG_UINT32 uiInstance)
{
	PVRSRV_DEVICE_NODE *psDevNode;

	if (uiInstance >= gpsPVRSRVData->ui32RegisteredDevices)
	{
		return NULL;
	}
	OSWRLockAcquireRead(gpsPVRSRVData->hDeviceNodeListLock);
	for (psDevNode = gpsPVRSRVData->psDeviceNodeList;
	     psDevNode != NULL; psDevNode = psDevNode->psNext)
	{
		if (uiInstance == psDevNode->sDevId.ui32InternalID)
		{
			break;
		}
	}
	OSWRLockReleaseRead(gpsPVRSRVData->hDeviceNodeListLock);

	return psDevNode;
}

PVRSRV_DEVICE_NODE *PVRSRVGetDeviceInstanceByKernelDevID(IMG_INT32 i32OSInstance)
{
	PVRSRV_DEVICE_NODE *psDevNode;

	OSWRLockAcquireRead(gpsPVRSRVData->hDeviceNodeListLock);
	for (psDevNode = gpsPVRSRVData->psDeviceNodeList;
	     psDevNode != NULL; psDevNode = psDevNode->psNext)
	{
		if (i32OSInstance == psDevNode->sDevId.i32KernelDeviceID)
		{
			MULTI_DEVICE_BRINGUP_DPF("%s: Found DevId %d. Retrieving node.", __func__, i32OSInstance);
			break;
		}
		else
		{
			MULTI_DEVICE_BRINGUP_DPF("%s: Searching for DevId %d: Id %d not matching", __func__, i32OSInstance, psDevNode->sDevId.i32KernelDeviceID);
		}
	}
	OSWRLockReleaseRead(gpsPVRSRVData->hDeviceNodeListLock);

	if (psDevNode == NULL)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: DevId %d not found.", __func__, i32OSInstance));
	}
	return psDevNode;
}

#define _FROZEN 1		/* Device is already frozen */
#define _NOT_FROZEN 0	/* Device is not frozen */

/* Freeze the specified device if not already frozen */
PVRSRV_ERROR PVRSRVDeviceFreeze(PVRSRV_DEVICE_NODE *psDevNode)
{
	PVRSRV_ERROR eError;
	IMG_BOOL     bHasPowerLock = IMG_FALSE;

	/* Verify that given argument *IS* a recognised device node */
	PVRSRV_DEVICE_NODE *lpsDevNode;

	OSWRLockAcquireRead(gpsPVRSRVData->hDeviceNodeListLock);
	for (lpsDevNode = gpsPVRSRVData->psDeviceNodeList;
	     lpsDevNode != NULL && lpsDevNode != psDevNode;
	     lpsDevNode = lpsDevNode->psNext)
		;
	OSWRLockReleaseRead(gpsPVRSRVData->hDeviceNodeListLock);

	PVR_LOG_RETURN_IF_FALSE((lpsDevNode == psDevNode), "Device node not known",
	                        PVRSRV_ERROR_INVALID_PARAMS);

	/* Mark the device as 'frozen' if not already marked */
	if (OSAtomicCompareExchange(&psDevNode->eFrozen, _NOT_FROZEN,
	                            _FROZEN) == _FROZEN)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Device %p already frozen", __func__,
		        psDevNode));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Ensure there are no other threads active within this device. */
	while (OSAtomicRead(&psDevNode->iThreadsActive) > 0)
	{
		OSReleaseThreadQuanta();	/* Let other threads execute */
	}

	/* Attempt to idle the GPU without power-down. If this fails, we try with
	 * a potential power-down.
	 */
	eError = PVRSRVPowerLock(psDevNode);
	if (eError != PVRSRV_OK)
	{
		/* Device is powered down, we can continue
		 * as there are no in-flight requests in the GPU.
		 */
		PVR_DPF((PVR_DBG_WARNING, "%s: Unable to Idle Device (%p) [%u/%d]"
		        " Device powered down",
		        __func__, psDevNode, psDevNode->sDevId.ui32InternalID,
		        psDevNode->sDevId.i32KernelDeviceID));
		eError = PVRSRV_OK;
	}
	else
	{
		bHasPowerLock = IMG_TRUE;

		eError = PVRSRVDeviceIdleRequestKM(psDevNode,
		                                   &PVRSRVDeviceIsDefaultStateOFF,
		                                   IMG_TRUE);
	}

	if (eError != PVRSRV_OK)
	{
		/* Failed to Idle. Need to remove the pending _FROZEN state. */
		(void) OSAtomicExchange(&psDevNode->eFrozen, _NOT_FROZEN);
		PVR_DPF((PVR_DBG_ERROR, "%s: Unable to Idle Device (%p) [%u/%d]",
		        __func__, psDevNode, psDevNode->sDevId.ui32InternalID,
		        psDevNode->sDevId.i32KernelDeviceID));

		if (bHasPowerLock)
		{
			PVRSRVPowerUnlock(psDevNode);
		}
		return eError;
	}

	/* Now change the device-state to not be ACTIVE until we unfreeze */
	if (psDevNode->eDevState != PVRSRV_DEVICE_STATE_ACTIVE)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Unexpected Device State %u for device %p",
		        __func__, psDevNode->eDevState, psDevNode));

		/* Reset device to UNFROZEN */
		(void) OSAtomicExchange(&psDevNode->eFrozen, _NOT_FROZEN);

		if (bHasPowerLock)
		{
			(void) PVRSRVDeviceIdleCancelRequestKM(psDevNode);
			PVRSRVPowerUnlock(psDevNode);
		}
		else
		{
			(void) PVRSRVSetDeviceSystemPowerState(psDevNode,
	                                               PVRSRV_SYS_POWER_STATE_ON,
	                                               PVRSRV_POWER_FLAGS_NONE);
		}

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVRSRVDeviceSetState(psDevNode, PVRSRV_DEVICE_STATE_FROZEN);

	PVR_DPF((PVR_DBG_VERBOSE, "%s: Device (%p) [%u/%d] FROZEN", __func__,
	        psDevNode,
	        psDevNode->sDevId.ui32InternalID,
	        psDevNode->sDevId.i32KernelDeviceID));

	OSAtomicIncrement(&psDevNode->iTotalFreezes);

	if (bHasPowerLock)
	{
		PVRSRVPowerUnlock(psDevNode);
	}

	return PVRSRV_OK;
}

/* Unfreeze / Thaw the specified device if frozen */
PVRSRV_ERROR PVRSRVDeviceThaw(PVRSRV_DEVICE_NODE *psDevNode)
{
	PVRSRV_ERROR eError;
	PVRSRV_DEVICE_NODE *lpsDevNode;
	IMG_BOOL           bHasPowerLock = IMG_FALSE;

	/* Verify that the given argument *IS* a recognised device node */
	OSWRLockAcquireRead(gpsPVRSRVData->hDeviceNodeListLock);
	for (lpsDevNode = gpsPVRSRVData->psDeviceNodeList;
	     lpsDevNode != NULL && lpsDevNode != psDevNode;
	     lpsDevNode = lpsDevNode->psNext)
		;
	OSWRLockReleaseRead(gpsPVRSRVData->hDeviceNodeListLock);

	PVR_LOG_RETURN_IF_FALSE((lpsDevNode == psDevNode), "Device node not known",
	                        PVRSRV_ERROR_INVALID_PARAMS);

	/* Unfreeze the device */
	if (OSAtomicCompareExchange(&psDevNode->eFrozen, _FROZEN,
	                            _NOT_FROZEN) == _NOT_FROZEN)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Device (%p) [%u/%d] already unfrozen",
		        __func__,
		        psDevNode, psDevNode->sDevId.ui32InternalID,
		        psDevNode->sDevId.i32KernelDeviceID));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (psDevNode->eDevState != PVRSRV_DEVICE_STATE_FROZEN)
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "%s: Unexpected Device state %u for device (%p) [%u/%d]",
		        __func__, psDevNode->eDevState, psDevNode,
		        psDevNode->sDevId.ui32InternalID,
		        psDevNode->sDevId.i32KernelDeviceID));
	}
	else
	{
		PVRSRVDeviceSetState(psDevNode, PVRSRV_DEVICE_STATE_ACTIVE);

		PVR_DPF((PVR_DBG_VERBOSE, "%s: Device (%p) [%u/%d] UNFROZEN", __func__,
		        psDevNode,
		        psDevNode->sDevId.ui32InternalID,
		        psDevNode->sDevId.i32KernelDeviceID));
	}

	/* Now unblock the device by clearing any ForcedIdle state and/or
	 * powering-on the device if needed.
	 */
	eError = PVRSRVPowerLock(psDevNode);

	bHasPowerLock = (eError == PVRSRV_OK) ? IMG_TRUE : IMG_FALSE;

	if (bHasPowerLock)
	{
		eError = PVRSRVDeviceIdleCancelRequestKM(psDevNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,
			        "%s: Could not cancel Idle state: Device (%p) [%u/%d] '%s'",
			        __func__,
			        psDevNode,
			        psDevNode->sDevId.ui32InternalID,
			        psDevNode->sDevId.i32KernelDeviceID,
			        PVRSRVGetErrorString(eError)));
		}

		PVRSRVPowerUnlock(psDevNode);
	}
	else
	{
		/* Force device back into POWER_STATE_ON as it must already be OFF. */
		(void) PVRSRVSetDeviceSystemPowerState(psDevNode,
	                                           PVRSRV_SYS_POWER_STATE_ON,
	                                           PVRSRV_POWER_FLAGS_NONE);
	}

	/* Unblock any waiting threads */
	while (OSAtomicRead(&psDevNode->iFreezeCount) > 0)
	{
		eError = OSEventObjectSignal(psDevNode->hDeviceThreadEvObj);
		if (OSAtomicRead(&psDevNode->iFreezeCount) > 0)
		{
			/* Sleep for 1ms to allow waiter to receive signal */
			OSSleepms(1U);
		}
	}

	/* Ensure that any blocked queues get rescheduled if we've woken up a
	 * waiter.
	 */
	PVRSRVCheckStatus(NULL);

	return PVRSRV_OK;
}

PVRSRV_DRIVER_MODE PVRSRVGetVzModeByDevNum(IMG_UINT32 ui32DevNum)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	PVRSRV_DRIVER_MODE eRetMode = DRIVER_MODE_NATIVE;
	PVRSRV_DEVICE_NODE *psDevNode;

	OSWRLockAcquireRead(psPVRSRVData->hDeviceNodeListLock);

	/* Iterate over all devices. */
	for (psDevNode = psPVRSRVData->psDeviceNodeList;
		 psDevNode != NULL;
		 psDevNode = psDevNode->psNext)
	{
		if (psDevNode->sDevId.ui32InternalID == ui32DevNum)
		{
			eRetMode = psDevNode->psDevConfig->eDriverMode;
			break;
		}
	}

	OSWRLockReleaseRead(psPVRSRVData->hDeviceNodeListLock);
	return eRetMode;
}

/*****************************************************************************
 End of file (pvrsrv.c)
*****************************************************************************/
