/*************************************************************************/ /*!
@File           pvr_gpuwork.c
@Title          PVR GPU Work Period implementation
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

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0))
#include <linux/trace_events.h>
#else
#include <linux/ftrace_event.h>
#endif
#include <linux/pid.h>
#include <linux/sched.h>
#define CREATE_TRACE_POINTS
#include "gpu_work.h"
#undef CREATE_TRACE_POINTS
#include "pvr_ricommon.h" /* for PVR_SYS_ALLOC_PID */
#include "pvr_gpuwork.h"
#include "pvr_debug.h"
#include "pvrsrv.h"
#include "hash.h"

#define MS_PER_SEC                              (1000UL)
#define THREAD_DESTROY_TIMEOUT                  (100000ULL)
#define THREAD_DESTROY_RETRIES                  (10U)

#define PVR_GPU_TRACE_WORK_PERIOD_UID_HASH_SIZE (32)

#define to_work_period(event)     (event->sPeriod)
#define to_work_period_ptr(event) (&event->sPeriod)

#define work_period_start(p)  (p.ui64StartTimeInNs)
#define work_period_end(p)    (p.ui64EndTimeInNs)

#define work_period_update_start(p, start) \
	do { \
		p->ui64StartTimeInNs = start; \
	} while (0)

#define work_period_update_end(p, end) \
	do { \
		p->ui64EndTimeInNs = end; \
	} while (0)

#define work_period_set(p, start, end) \
	work_period_update_start(p, start); \
	work_period_update_end(p, end)

#define work_period_equal(a, b) \
	(work_period_start(a) == work_period_start(b)) && \
	(work_period_end(a) == work_period_end(b)) \
	? IMG_TRUE : IMG_FALSE

#define PVR_GPU_WORK_PERIOD_UNDEFINED ((IMG_UINT64)-1)

typedef enum THREAD_STATE
{
	THREAD_STATE_NULL,
	THREAD_STATE_ALIVE,
	THREAD_STATE_TERMINATED,
} THREAD_STATE;

typedef struct _PVR_GPU_WORK_PERIOD_ {
	/* The start time of this period. */
	IMG_UINT64 ui64StartTimeInNs;
	/* The end time of this period. */
	IMG_UINT64 ui64EndTimeInNs;
} PVR_GPU_WORK_PERIOD;

typedef struct _PVR_GPU_WORK_PERIOD_EVENT_ {
	/* The is a unique value for each GPU jobs. It's possible to have
	 * distinct tasks with the same job ID, eg. TA and 3D.
	 */
	IMG_UINT32                         ui32JobId;
	/* ui32GpuId indicates which GPU files this event. */
	IMG_UINT32                         ui32GpuId;
	/* The time periods of this job. */
	PVR_GPU_WORK_PERIOD                sPeriod;
	/* List node */
	struct _PVR_GPU_WORK_PERIOD_EVENT_ *psPrev;
	struct _PVR_GPU_WORK_PERIOD_EVENT_ *psNext;
} PVR_GPU_WORK_PERIOD_EVENT;

typedef struct _PVR_GPU_WORK_PERIOD_EVENT_STATS_ {
	/* Indicates which UID own this work period event stats. */
	IMG_UINT32                         ui32Uid;
	/* Having multiple connections in a process is allowed and it's possible
	 * to have multiple processes in one app. The counting stores how many
	 * connections are connecting. The stats is removed from the uid hash
	 * table when there aren't connections in the given app.
	 */
	IMG_UINT32                         ui32RefCount;
	/* A List of work period event. */
	PVR_GPU_WORK_PERIOD_EVENT          *psList;
	/* The last node of work period event list. */
	PVR_GPU_WORK_PERIOD_EVENT          *psLastItemInList;
	/* The previous active_end_time. */
	IMG_UINT64                         ui64LastActiveTimeInNs;
	/* Protects access to work period event list. */
	POS_LOCK                           hListLock;
} PVR_GPU_WORK_PERIOD_EVENT_STATS;

typedef struct _PVR_GPU_WORK_PERIOD_EVENT_DATA_ {
	/* The work period context is initialized when Android OS supports GPU
	 * metrics to allow driver to provide hardware usages. This feature is
	 * implemented on the basis of switch trace event.
	 */
	IMG_BOOL                           bInitialized;
	/* hTraceEnabled is used to indicate if the event is enabled or not. */
	ATOMIC_T                           hTraceEnabled;
	/* The hash table stores work period stats for each apps. */
	HASH_TABLE                         *psUidHashTable;
	/* A thread to process switch events to meet the work period event
	 * specification and writes ftrace events.
	 */
	void                               *hProcessThread;
	/* hProcessEventObject is used to indicate if any pending work period
	 * events to emit.
	 */
	IMG_HANDLE                         hProcessEventObject;
	/* The state of the process thread. */
	ATOMIC_T                           hProcessThreadState;
	/* Protects access to UID hashtable. */
	POS_LOCK                           hHashTableLock;
} PVR_GPU_WORK_PERIOD_EVENT_DATA;

static PVR_GPU_WORK_PERIOD_EVENT_DATA gGpuWorkPeriodEventData;

#define getGpuWorkPeriodEventData() (&gGpuWorkPeriodEventData)

static IMG_UINT32 _GetUID(IMG_PID pid)
{
	struct task_struct *psTask;
	struct pid *psPid;

	psPid = find_get_pid((pid_t)pid);
	if (!psPid)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to lookup PID %u.",
		                        __func__, pid));
		return 0;
	}

	psTask = get_pid_task(psPid, PIDTYPE_PID);
	if (!psTask)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get pid task for PID %u.",
		                        __func__, pid));
	}
	put_pid(psPid);

	return psTask ? from_kuid(&init_user_ns, psTask->cred->uid) : 0;
}

static IMG_BOOL
_IsGpuWorkPerioidEventInitialized()
{
	return getGpuWorkPeriodEventData()->bInitialized;
}

/* ---------- Functions to operate the work period event list -------------- */

static IMG_BOOL
_IsHasPeriodEvent(PVR_GPU_WORK_PERIOD_EVENT_STATS
		*psGpuWorkPeriodEventStats)
{
	if (!psGpuWorkPeriodEventStats)
		return IMG_FALSE;

	return psGpuWorkPeriodEventStats->psList ? IMG_TRUE : IMG_FALSE;
}

static PVR_GPU_WORK_PERIOD_EVENT*
_FindGpuWorkPeriodEvent(PVR_GPU_WORK_PERIOD_EVENT *head, IMG_UINT32 ui32JobId)
{
	while (head)
	{
		if ((head->ui32JobId == ui32JobId) && work_period_end(
					to_work_period(head)) == PVR_GPU_WORK_PERIOD_UNDEFINED)
			return head;

		head = head->psPrev;
	}

	return NULL;
}

static PVR_GPU_WORK_PERIOD_EVENT*
_PeekGpuWorkPeriodEvent(PVR_GPU_WORK_PERIOD_EVENT_STATS
		*psGpuWorkPeriodEventStats)
{
	if (!psGpuWorkPeriodEventStats)
		return NULL;

	return psGpuWorkPeriodEventStats->psList;
}

static PVR_GPU_WORK_PERIOD_EVENT *
_PopGpuWorkPeriodEvent(PVR_GPU_WORK_PERIOD_EVENT_STATS
		*psGpuWorkPeriodEventStats)
{
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;

	if (!psGpuWorkPeriodEventStats)
		return NULL;

	psGpuWorkPeriodEvent = psGpuWorkPeriodEventStats->psList;
	if (!psGpuWorkPeriodEvent)
		return NULL;

	psGpuWorkPeriodEventStats->psList = psGpuWorkPeriodEvent->psNext;
	if (psGpuWorkPeriodEventStats->psList)
		psGpuWorkPeriodEventStats->psList->psPrev = NULL;

	if (psGpuWorkPeriodEvent == psGpuWorkPeriodEventStats->psLastItemInList)
		psGpuWorkPeriodEventStats->psLastItemInList = NULL;

	psGpuWorkPeriodEvent->psPrev = NULL;
	psGpuWorkPeriodEvent->psNext = NULL;

	return psGpuWorkPeriodEvent;
}

static PVRSRV_ERROR
_InsertGpuWorkPeriodEvent(
		PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats,
		IMG_UINT32 ui32GpuId, IMG_UINT32 ui32JobId, IMG_UINT64 ui64TimeInNs,
		PVR_GPU_WORK_EVENT_TYPE eEventType)
{
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;
	PVRSRV_ERROR eError;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	if (eEventType == PVR_GPU_WORK_EVENT_START) {
		psGpuWorkPeriodEvent = OSAllocZMem(sizeof(PVR_GPU_WORK_PERIOD_EVENT));
		PVR_LOG_GOTO_IF_NOMEM(psGpuWorkPeriodEvent, eError, err_release);

		if (!psGpuWorkPeriodEventStats->psLastItemInList)
			psGpuWorkPeriodEventStats->psList = psGpuWorkPeriodEvent;
		else
		{
			psGpuWorkPeriodEventStats->psLastItemInList->psNext =
				psGpuWorkPeriodEvent;
			psGpuWorkPeriodEvent->psPrev =
				psGpuWorkPeriodEventStats->psLastItemInList;
		}

		psGpuWorkPeriodEventStats->psLastItemInList = psGpuWorkPeriodEvent;

		psGpuWorkPeriodEvent->ui32GpuId = ui32GpuId;
		psGpuWorkPeriodEvent->ui32JobId = ui32JobId;

		work_period_set(to_work_period_ptr(psGpuWorkPeriodEvent), ui64TimeInNs,
				PVR_GPU_WORK_PERIOD_UNDEFINED);
	}
	else
	{
		psGpuWorkPeriodEvent = _FindGpuWorkPeriodEvent(
				psGpuWorkPeriodEventStats->psLastItemInList, ui32JobId);
		PVR_LOG_GOTO_IF_NOMEM(psGpuWorkPeriodEvent, eError, err_release);

		/* Update the end time. */
		work_period_update_end(to_work_period_ptr(psGpuWorkPeriodEvent),
				ui64TimeInNs);
	}

	eError = PVRSRV_OK;

err_release:
	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);
	return eError;
}

static void
_CleanupGpuWorkPeriodStats(
		PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats)
{
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;

	if (!_IsHasPeriodEvent(psGpuWorkPeriodEventStats))
		return;

	/* Delete elements from the work period list. */
	psGpuWorkPeriodEvent = _PopGpuWorkPeriodEvent(psGpuWorkPeriodEventStats);
	while (psGpuWorkPeriodEvent)
	{
		OSFreeMem(psGpuWorkPeriodEvent);
		psGpuWorkPeriodEvent =
			_PopGpuWorkPeriodEvent(psGpuWorkPeriodEventStats);
	}
}

static PVRSRV_ERROR
_DeleteGpuWorkPeriodStatsCallback(uintptr_t k, uintptr_t v, void *argv)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)v;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		(PVR_GPU_WORK_PERIOD_EVENT_DATA *)argv;
	IMG_UINT32 ui32Uid = (IMG_UINT32)k;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	_CleanupGpuWorkPeriodStats(psGpuWorkPeriodEventStats);

	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

	HASH_Remove(psGpuWorkPeriodEventData->psUidHashTable, ui32Uid);

	OSLockDestroy(psGpuWorkPeriodEventStats->hListLock);
	OSFreeMem(psGpuWorkPeriodEventStats);

	return PVRSRV_OK;
}

/* ----------- The entry which binds to the PVR trace feature -------------- */

void GpuTraceWorkPeriod(IMG_PID pid, IMG_UINT32 u32GpuId,
		IMG_UINT64 ui64HWTimestampInOSTime,
		IMG_UINT32 ui32IntJobRef,
		PVR_GPU_WORK_EVENT_TYPE eEventType)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	IMG_UINT32 ui32Uid = _GetUID(pid);

	if (!_IsGpuWorkPerioidEventInitialized())
		return;

	if (OSAtomicRead(&psGpuWorkPeriodEventData->hTraceEnabled) == IMG_FALSE)
		return;

	OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)HASH_Retrieve(
				psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid);

	OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);

	if (!psGpuWorkPeriodEventStats) {
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to lookup PID %u in hash table",
					__func__, pid));
		PVR_LOG_RETURN_VOID_IF_ERROR(eError, "HASH_Retrieve");
	}

	eError = _InsertGpuWorkPeriodEvent(psGpuWorkPeriodEventStats,
			u32GpuId, ui32IntJobRef, ui64HWTimestampInOSTime,
			eEventType);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "_InsertGpuWorkPeriodEvent");

	PVR_ASSERT(psGpuWorkPeriodEventData->hProcessEventObject);

	eError = OSEventObjectSignal(
			psGpuWorkPeriodEventData->hProcessEventObject);
	PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
}

static IMG_BOOL
_GpuWorkPeriodIntersect(PVR_GPU_WORK_PERIOD period_a,
		PVR_GPU_WORK_PERIOD period_b, PVR_GPU_WORK_PERIOD *psInterSectPeriod)
{
	if (!(work_period_end(period_a) < work_period_start(period_b) ||
			work_period_end(period_b) < work_period_start(period_a)))
	{
		IMG_UINT64 ui64StartTimeInNs, ui64EndTimeInNs;

		ui64StartTimeInNs = max(
				work_period_start(period_a), work_period_start(period_b));
		ui64EndTimeInNs = min(
				work_period_end(period_a), work_period_end(period_b));

		work_period_set(psInterSectPeriod, ui64StartTimeInNs, ui64EndTimeInNs);

		return IMG_TRUE;
	}

	return IMG_FALSE;
}

static PVRSRV_ERROR
_EmitCallback(uintptr_t k, uintptr_t v, void *argv)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)v;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVR_GPU_WORK_PERIOD_EVENT *psPrevEmitEvent = NULL;
	PVR_GPU_WORK_PERIOD_EVENT *psGpuWorkPeriodEvent;
	PVR_GPU_WORK_PERIOD sInterSectPeriod = {0, 0};
	PVR_GPU_WORK_PERIOD_EVENT *psEmitListHead;
	PVR_GPU_WORK_PERIOD_EVENT sEmitList = {};
	IMG_UINT32 ui32Uid = (IMG_UINT32)k;

	PVR_UNREFERENCED_PARAMETER(argv);

	psEmitListHead = &sEmitList;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	if (!_IsHasPeriodEvent(psGpuWorkPeriodEventStats))
	{
		OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

		return PVRSRV_OK;
	}

	psGpuWorkPeriodEvent = _PeekGpuWorkPeriodEvent(psGpuWorkPeriodEventStats);
	while (psGpuWorkPeriodEvent &&
			work_period_end(to_work_period(psGpuWorkPeriodEvent)) !=
			PVR_GPU_WORK_PERIOD_UNDEFINED)
	{
		IMG_BOOL bOverlappedEvent = IMG_FALSE;

		psGpuWorkPeriodEvent =
			_PopGpuWorkPeriodEvent(psGpuWorkPeriodEventStats);
		/* Fixup the overlapping. The spec says the periods should not be
		 * detailed enough to capture the various different, overlapping types
		 * of work performed by the GPU. The event just requires a summary that
		 * covers the time where any work is occurring. If a GPU work period
		 * duration is covered by another event, it can be uncalculated.
		 */
		if (psPrevEmitEvent)
		{
			if (_GpuWorkPeriodIntersect(to_work_period(psPrevEmitEvent),
						to_work_period(psGpuWorkPeriodEvent),
						&sInterSectPeriod))
			{
				/* The period duration is covered. Skip this period. */
				if (work_period_equal(sInterSectPeriod,
							to_work_period(psGpuWorkPeriodEvent)))
				{
					bOverlappedEvent = IMG_TRUE;
				}
				else if (work_period_start(sInterSectPeriod) ==
						work_period_start(to_work_period(psGpuWorkPeriodEvent)))
				{
					/* Fixup the start time from the end of intersected
					 * period.
					*/
					work_period_update_start(
							to_work_period_ptr(psGpuWorkPeriodEvent),
							work_period_end(sInterSectPeriod));
				}
				else
				{
					/* Should never touch here. */
					PVR_ASSERT(0);
				}
			}
		}

		if (!bOverlappedEvent)
		{
			psEmitListHead->psNext = psGpuWorkPeriodEvent;
			psEmitListHead = psEmitListHead->psNext;
			psPrevEmitEvent = psGpuWorkPeriodEvent;
		}
		psGpuWorkPeriodEvent =
			_PeekGpuWorkPeriodEvent(psGpuWorkPeriodEventStats);
	}

	psEmitListHead = sEmitList.psNext;

	while (psEmitListHead)
	{
		PVR_GPU_WORK_PERIOD_EVENT *psNext = psEmitListHead->psNext;
		IMG_UINT64 ui64StartTimeInNs =
			work_period_start(to_work_period(psEmitListHead));
		IMG_UINT64 ui64EndTimeInNs =
			work_period_end(to_work_period(psEmitListHead));

		/* The events must be emitted in strictly increasing order of
		 * start time.
		 */
		PVR_ASSERT(ui64StartTimeInNs >=
				psGpuWorkPeriodEventStats->ui64LastActiveTimeInNs);

		trace_gpu_work_period(psEmitListHead->ui32GpuId,
				(IMG_UINT64)ui32Uid, ui64StartTimeInNs,
				ui64EndTimeInNs, ui64EndTimeInNs - ui64StartTimeInNs);

		psGpuWorkPeriodEventStats->ui64LastActiveTimeInNs = ui64EndTimeInNs;

		OSFreeMem(psEmitListHead);
		psEmitListHead = psNext;
	}

	if (!psGpuWorkPeriodEventStats->ui32RefCount)
	{
		_CleanupGpuWorkPeriodStats(psGpuWorkPeriodEventStats);
		OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

		/* Remove the bucket from the hash table. */
		HASH_Remove(psGpuWorkPeriodEventData->psUidHashTable, ui32Uid);

		OSLockDestroy(psGpuWorkPeriodEventStats->hListLock);
		OSFreeMem(psGpuWorkPeriodEventStats);

		return PVRSRV_OK;
	}

	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);

	return PVRSRV_OK;
}

static void _ProcessGpuWorkPeriodEvents(void *pvData)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		(PVR_GPU_WORK_PERIOD_EVENT_DATA *)pvData;
	IMG_HANDLE hProcessEvent;
	PVRSRV_ERROR eError;

	eError = OSEventObjectOpen(psGpuWorkPeriodEventData->hProcessEventObject,
			&hProcessEvent);
	PVR_LOG_RETURN_VOID_IF_ERROR(eError, "OSEventObjectOpen");

	while (OSAtomicRead(&psGpuWorkPeriodEventData->hProcessThreadState) ==
			THREAD_STATE_ALIVE)
	{
		eError = OSEventObjectWaitKernel(hProcessEvent, (IMG_UINT64)-1);
		if (eError != PVRSRV_OK)
		{
			PVR_LOG_ERROR(eError, "OSEventObjectWaitKernel");
			continue;
		}

		OSSleepms(MS_PER_SEC);

		OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

		eError = HASH_Iterate(psGpuWorkPeriodEventData->psUidHashTable,
				_EmitCallback, NULL);
		PVR_LOG_IF_ERROR(eError, "HASH_Iterate");

		OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);
	}

	OSAtomicWrite(&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_TERMINATED);
}

/* -------- Initialize/Deinitialize for Android GPU metrics ---------------- */

PVRSRV_ERROR GpuTraceWorkPeriodInitialize(void)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVRSRV_ERROR eError;

	if (psGpuWorkPeriodEventData->bInitialized == IMG_FALSE)
	{
		eError = OSLockCreate(
				&psGpuWorkPeriodEventData->hHashTableLock);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSLockCreate", err_out);

		psGpuWorkPeriodEventData->psUidHashTable =
			HASH_Create(PVR_GPU_TRACE_WORK_PERIOD_UID_HASH_SIZE);
		if (!psGpuWorkPeriodEventData->psUidHashTable)
		{
			PVR_LOG_GOTO_WITH_ERROR("HASH_Create", eError,
					PVRSRV_ERROR_OUT_OF_MEMORY, err_deInitialize);
		}

		eError = OSEventObjectCreate("process event object",
				&psGpuWorkPeriodEventData->hProcessEventObject);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", err_deInitialize);

		OSAtomicWrite(&psGpuWorkPeriodEventData->hProcessThreadState,
				THREAD_STATE_NULL);

		OSAtomicWrite(&psGpuWorkPeriodEventData->hTraceEnabled, IMG_FALSE);

		psGpuWorkPeriodEventData->bInitialized = IMG_TRUE;
	}

	eError = PVRSRV_OK;

err_out:
	return eError;
err_deInitialize:
	GpuTraceSupportDeInitialize();
	goto err_out;
}

void GpuTraceSupportDeInitialize(void)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	THREAD_STATE hProcessThreadState;
	PVRSRV_ERROR eError;

	if (!_IsGpuWorkPerioidEventInitialized())
		return;

	hProcessThreadState = OSAtomicCompareExchange(
			&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_ALIVE,
			THREAD_STATE_TERMINATED);
	if (hProcessThreadState == THREAD_STATE_ALIVE)
	{
		if (psGpuWorkPeriodEventData->hProcessEventObject)
		{
			eError = OSEventObjectSignal(
					psGpuWorkPeriodEventData->hProcessEventObject);
			PVR_LOG_IF_ERROR(eError, "OSEventObjectSignal");
		}

		LOOP_UNTIL_TIMEOUT(THREAD_DESTROY_TIMEOUT)
		{
			eError = OSThreadDestroy(psGpuWorkPeriodEventData->hProcessThread);
			if (eError == PVRSRV_OK)
			{
				break;
			}
			OSWaitus(THREAD_DESTROY_TIMEOUT/THREAD_DESTROY_RETRIES);
		} END_LOOP_UNTIL_TIMEOUT();

		PVR_LOG_IF_ERROR(eError, "OSThreadDestroy");
	}

	if (psGpuWorkPeriodEventData->hProcessEventObject)
	{
		eError = OSEventObjectDestroy(
				psGpuWorkPeriodEventData->hProcessEventObject);
		PVR_LOG_IF_ERROR(eError, "OSEventObjectDestroy");
	}

	if (psGpuWorkPeriodEventData->psUidHashTable)
	{
		OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

		eError = HASH_Iterate(psGpuWorkPeriodEventData->psUidHashTable,
				_DeleteGpuWorkPeriodStatsCallback, psGpuWorkPeriodEventData);

		OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);

		PVR_LOG_IF_ERROR(eError, "HASH_Iterate");

		HASH_Delete(psGpuWorkPeriodEventData->psUidHashTable);
		psGpuWorkPeriodEventData->psUidHashTable = NULL;
	}

	if (psGpuWorkPeriodEventData->hHashTableLock)
	{
		OSLockDestroy(psGpuWorkPeriodEventData->hHashTableLock);
		psGpuWorkPeriodEventData->hHashTableLock = NULL;
	}

	psGpuWorkPeriodEventData->bInitialized = IMG_FALSE;
}

static PVRSRV_ERROR
_RegisterProcess(IMG_HANDLE* phGpuWorkPeriodEventStats, IMG_PID ownerPid)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32Uid;

	PVR_ASSERT(phGpuWorkPeriodEventStats);

	if (!_IsGpuWorkPerioidEventInitialized())
		return PVRSRV_ERROR_NOT_INITIALISED;

	PVR_DPF((PVR_DBG_MESSAGE, "%s: Register process PID %d [%s]",
			__func__, ownerPid, (ownerPid == PVR_SYS_ALLOC_PID)
			? "system" : OSGetCurrentClientProcessNameKM()));

	OSLockAcquire(psGpuWorkPeriodEventData->hHashTableLock);

	ui32Uid = _GetUID(ownerPid);
	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)HASH_Retrieve(
			psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid);
	if (psGpuWorkPeriodEventStats)
	{
		psGpuWorkPeriodEventStats->ui32RefCount++;
		*phGpuWorkPeriodEventStats = (IMG_HANDLE)psGpuWorkPeriodEventStats;
		/* The work period event stats was created for this PID.
		 * Take a reference and return the instance immediately.
		 */
		eError = PVRSRV_OK;
		goto err_release;
	}

	psGpuWorkPeriodEventStats =
		OSAllocZMem(sizeof(PVR_GPU_WORK_PERIOD_EVENT_STATS));
	PVR_LOG_GOTO_IF_NOMEM(psGpuWorkPeriodEventStats, eError, err_release);

	psGpuWorkPeriodEventStats->ui32Uid = ui32Uid;
	psGpuWorkPeriodEventStats->ui32RefCount++;

	eError = OSLockCreate(
			&psGpuWorkPeriodEventStats->hListLock);
	PVR_LOG_GOTO_IF_ERROR(eError, "OSEventObjectCreate", err_release);

	HASH_Insert(psGpuWorkPeriodEventData->psUidHashTable, (uintptr_t)ui32Uid,
			(uintptr_t)psGpuWorkPeriodEventStats);

	*phGpuWorkPeriodEventStats = (IMG_HANDLE)psGpuWorkPeriodEventStats;
	eError = PVRSRV_OK;

err_release:
	OSLockRelease(psGpuWorkPeriodEventData->hHashTableLock);
	return eError;
}

static void _UnregisterProcess(IMG_HANDLE hProcessStats)
{
	PVR_GPU_WORK_PERIOD_EVENT_STATS *psGpuWorkPeriodEventStats;
	IMG_UINT32 ui32Uid;

	if (!hProcessStats || !_IsGpuWorkPerioidEventInitialized())
		return;

	psGpuWorkPeriodEventStats =
		(PVR_GPU_WORK_PERIOD_EVENT_STATS *)hProcessStats;

	OSLockAcquire(psGpuWorkPeriodEventStats->hListLock);

	ui32Uid = psGpuWorkPeriodEventStats->ui32Uid;
	psGpuWorkPeriodEventStats->ui32RefCount--;

	OSLockRelease(psGpuWorkPeriodEventStats->hListLock);
}

PVRSRV_ERROR
GpuTraceWorkPeriodEventStatsRegister(IMG_HANDLE
		*phGpuWorkPeriodEventStats)
{
	return _RegisterProcess(phGpuWorkPeriodEventStats,
			OSGetCurrentClientProcessIDKM());
}

void
GpuTraceWorkPeriodEventStatsUnregister(
		IMG_HANDLE hGpuWorkPeriodEventStats)
{
	_UnregisterProcess(hGpuWorkPeriodEventStats);
}

/* ----- FTrace event callbacks -------------------------------------------- */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
int PVRGpuTraceEnableWorkPeriodCallback(void)
#else
void PVRGpuTraceEnableWorkPeriodCallback(void)
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) */
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();
	THREAD_STATE hProcessThreadState;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (!_IsGpuWorkPerioidEventInitialized())
	{
		PVR_LOG_GOTO_WITH_ERROR("_IsGpuWorkPerioidEventInitialized", eError,
					PVRSRV_ERROR_NOT_INITIALISED, err_out);
	}

	hProcessThreadState = OSAtomicCompareExchange(
			&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_NULL,
			THREAD_STATE_ALIVE);

	/* if the thread has not been started yet do it */
	if (hProcessThreadState == THREAD_STATE_NULL)
	{
		PVR_ASSERT(psGpuWorkPeriodEventData->hProcessThread == NULL);

		eError = OSThreadCreate(&psGpuWorkPeriodEventData->hProcessThread,
				"gpu_work_period_process", _ProcessGpuWorkPeriodEvents,
				NULL, IMG_FALSE, psGpuWorkPeriodEventData);
		PVR_LOG_GOTO_IF_ERROR(eError, "OSThreadCreate", err_terminate);
	}

	OSAtomicCompareExchange(&psGpuWorkPeriodEventData->hTraceEnabled,
			IMG_FALSE, IMG_TRUE);

err_out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
	if (eError != PVRSRV_OK)
		return  -ENODEV;
	return 0;
#else
	return;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)) */
err_terminate:
	OSAtomicWrite(&psGpuWorkPeriodEventData->hProcessThreadState,
			THREAD_STATE_TERMINATED);
	goto err_out;
}

void PVRGpuTraceDisableWorkPeriodCallback(void)
{
	PVR_GPU_WORK_PERIOD_EVENT_DATA *psGpuWorkPeriodEventData =
		getGpuWorkPeriodEventData();

	if (!_IsGpuWorkPerioidEventInitialized())
		return;

	OSAtomicCompareExchange(&psGpuWorkPeriodEventData->hTraceEnabled,
			IMG_TRUE, IMG_FALSE);
}
