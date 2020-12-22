/*************************************************************************/ /*!
@File           pvr_sync.c
@Title          Kernel driver for Android's sync mechanism
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

#include "pvr_sync.h"

#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/anon_inodes.h>
#include <linux/seq_file.h>

#include "services_headers.h"
#include "sgxutils.h"
#include "ttrace.h"
#include "mutex.h"
#include "lock.h"

//#define DEBUG_PRINT

#if defined(DEBUG_PRINT)
#define DPF(fmt, ...) PVR_DPF((PVR_DBG_BUFFERED, fmt, __VA_ARGS__))
#else
#define DPF(fmt, ...) do {} while(0)
#endif

/* We can't support this code when the MISR runs in atomic context because
 * PVRSyncFreeSync() may be called by sync_timeline_signal() which may be
 * scheduled by the MISR. PVRSyncFreeSync() needs to protect the handle
 * tables against modification and so uses the Linux bridge mutex.
 *
 * You can't lock a mutex in atomic context.
 */
#if !defined(PVR_LINUX_MISR_USING_WORKQUEUE) && \
    !defined(PVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE)
#error The Android sync driver requires that the SGX MISR runs in wq context
#endif

/* Local wrapper around PVRSRV_KERNEL_SYNC_INFO to add a list head */

struct PVR_SYNC_KERNEL_SYNC_INFO
{
	/* Base services sync info structure */
	PVRSRV_KERNEL_SYNC_INFO	*psBase;

	/* Sync points can go away when there are deferred hardware
	 * operations still outstanding. We must not free the SYNC_INFO
	 * until the hardware is finished, so we add it to a defer list
	 * which is processed periodically ("defer-free").
	 *
	 * This is also used for "defer-free" of a timeline -- the process
	 * may destroy its timeline or terminate abnormally but the HW could
	 * still be using the sync object hanging off of the timeline.
	 *
	 * Note that the defer-free list is global, not per-timeline.
	 */
	struct list_head		sHead;
};

/* This is the IMG extension of a sync_timeline */

struct PVR_SYNC_TIMELINE
{
	struct sync_timeline				obj;

	/* Needed to keep a global list of all timelines for MISR checks. */
	struct list_head					sTimelineList;

	/* True if a sync point on the timeline has signaled */
	IMG_BOOL							bSyncHasSignaled;

	/* A mutex, as we want to ensure that the comparison (and possible
	 * reset) of the highest SW fence value is atomic with the takeop,
	 * so both the SW fence value and the WOP snapshot should both have
	 * the same order for all SW syncs.
	 *
	 * This mutex also protects modifications to the fence stamp counter.
	 */
	struct mutex						sTimelineLock;

	/* Every timeline has a services sync object. This object must not
	 * be used by the hardware to enforce ordering -- that's what the
	 * per sync-point objects are for. This object is attached to every
	 * TQ scheduled on the timeline and is primarily useful for debugging.
	 */
	struct PVR_SYNC_KERNEL_SYNC_INFO	*psSyncInfo;
};

/* A PVR_SYNC_DATA is the basic guts of a sync point. It's kept separate
 * because sync points can be dup'ed, and we don't want to duplicate all
 * of the shared metadata.
 *
 * This is also used to back an allocated sync info, which can be passed to
 * the CREATE ioctl to insert the fence and add it to the timeline. This is
 * used as an intermediate step as a PVRSRV_KERNEL_SYNC_INFO is needed to
 * attach to the transfer task used as a fence in the hardware.
 */

struct PVR_SYNC_DATA
{
	/* Every sync point has a services sync object. This object is used
	 * by the hardware to enforce ordering -- it is attached as a source
	 * dependency to various commands.
	 */
	struct PVR_SYNC_KERNEL_SYNC_INFO	*psSyncInfo;

	/* This refcount is incremented at create and dup time, and decremented
	 * at free time. It ensures the object doesn't start the defer-free
	 * process until it is no longer referenced.
	 */
	atomic_t							sRefcount;

	/* This is purely a debug feature. Record the WOP snapshot from the
	 * timeline synchronization object when a new fence is created.
	 */
	IMG_UINT32							ui32WOPSnapshot;

	/* This is a globally unique ID for the sync point. If a sync point is
	 * dupped, its stamp is copied over (seems counter-intuitive, but in
	 * nearly all cases a sync point is merged with another, the original
	 * is freed).
	 */
	IMG_UINT64							ui64Stamp;
};

/* A PVR_ALLOC_SYNC_DATA is used to back an allocated, but not yet created
 * and inserted into a timeline, sync data. This is required as we must
 * allocate the syncinfo to be passed down with the transfer task used to
 * implement fences in the hardware.
 */
struct PVR_ALLOC_SYNC_DATA
{
	struct PVR_SYNC_KERNEL_SYNC_INFO	*psSyncInfo;

	/* A link to the timeline is required to add a per-timeline sync
	 * to the fence transfer task.
	 */
	struct PVR_SYNC_TIMELINE			*psTimeline;
	struct file							*psFile;
};

/* This is the IMG extension of a sync_pt */

struct PVR_SYNC
{
	struct sync_pt			pt;
	struct PVR_SYNC_DATA	*psSyncData;
};

struct PVR_SYNC_FENCE
{
	/* Base sync_fence structure */
	struct sync_fence	*psBase;

	/* To ensure callbacks are always received for fences / sync_pts, even
	 * after the fence has been 'put' (freed), we must take a reference to
	 * the fence. We still need to 'put' the fence ourselves, but this might
	 * happen in irq context, where fput() is not allowed (in kernels <3.6).
	 * We must add the fence to a list which is processed in WQ context.
	 */
	struct list_head	sHead;
};

/* Any sync point from a foreign (non-PVR) timeline needs to have a "shadow"
 * syncinfo. This is modelled as a software operation. The foreign driver
 * completes the operation by calling a callback we registered with it.
 *
 * Because we are allocating SYNCINFOs for each sync_pt, rather than each
 * fence, we need to extend the waiter struct slightly to include the
 * necessary metadata.
 */
struct PVR_SYNC_FENCE_WAITER
{
	/* Base sync driver waiter structure */
	struct sync_fence_waiter			sWaiter;

	/* "Shadow" syncinfo backing the foreign driver's sync_pt */
	struct PVR_SYNC_KERNEL_SYNC_INFO	*psSyncInfo;

	/* Optimizes lookup of fence for defer-put operation */
	struct PVR_SYNC_FENCE				*psSyncFence;
};

/* Global data relating to PVR services connection */

static struct
{
	/* Process that initialized the sync driver. House-keep this so
	 * the correct per-proc data is used during shutdown. This PID is
	 * conventionally whatever `pvrsrvctl' was when it was alive.
	 */
	IMG_UINT32	ui32Pid;

	/* Device cookie for services allocation functions. The device would
	 * ordinarily be SGX, and the first/only device in the system.
	 */
	IMG_HANDLE	hDevCookie;

	/* Device memory context that all SYNC_INFOs allocated by this driver
	 * will be created in. Because SYNC_INFOs are placed in a shared heap,
	 * it does not matter from which process the create ioctl originates.
	 */
	IMG_HANDLE	hDevMemContext;
}
gsSyncServicesConnection;

/* Multi-purpose workqueue. Various functions in the Google sync driver
 * may call down to us in atomic context. However, sometimes we may need
 * to lock a mutex. To work around this conflict, use the workqueue to
 * defer whatever the operation was.
 */
static struct workqueue_struct *gpsWorkQueue;

/* Linux work struct for workqueue. */
static struct work_struct gsWork;

/* List of timelines, used by MISR callback to find signalled sync points
 * and also to kick the hardware if signalling may allow progress to be
 * made.
 */
static LIST_HEAD(gTimelineList);
static DEFINE_MUTEX(gTimelineListLock);

/* The "defer-free" object list. Driver global. */
static LIST_HEAD(gSyncInfoFreeList);
static DEFINE_SPINLOCK(gSyncInfoFreeListLock);

/* The "defer-put" object list. Driver global. */
static LIST_HEAD(gFencePutList);
static DEFINE_SPINLOCK(gFencePutListLock);

/* Sync point stamp counter -- incremented on creation of a new sync point */
static IMG_UINT64 gui64SyncPointStamp;

/* Forward declare due to cyclic dependency on gsSyncFenceAllocFOps */
static struct PVR_ALLOC_SYNC_DATA *PVRSyncAllocFDGet(int fd);

/* NOTE: Must only be called with services bridge mutex held */
static void PVRSyncSWTakeOp(PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo)
{
	psKernelSyncInfo->psSyncData->ui32WriteOpsPending = 1;
}

static void PVRSyncSWCompleteOp(PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo)
{
	psKernelSyncInfo->psSyncData->ui32WriteOpsComplete = 1;
}

static struct PVR_SYNC *
PVRSyncCreateSync(struct PVR_SYNC_TIMELINE *obj, 
				  struct PVR_SYNC_KERNEL_SYNC_INFO *psSyncInfo)
{
	struct PVR_SYNC *psPt = NULL;

	psPt = (struct PVR_SYNC *)
		sync_pt_create(&obj->obj, sizeof(struct PVR_SYNC));
	if(!psPt)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: sync_pt_create failed", __func__));
		goto err_out;
	}

	psPt->psSyncData = kmalloc(sizeof(struct PVR_SYNC_DATA), GFP_KERNEL);
	if(!psPt->psSyncData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate PVR_SYNC_DATA",
				 __func__));
		goto err_free_pt;
	}

	atomic_set(&psPt->psSyncData->sRefcount, 1);

	psPt->psSyncData->ui32WOPSnapshot =
		obj->psSyncInfo->psBase->psSyncData->ui32WriteOpsPending;

	psPt->psSyncData->psSyncInfo = psSyncInfo;

	/* Stamp the point and update the global counter under lock */
	mutex_lock(&obj->sTimelineLock);
	psPt->psSyncData->ui64Stamp = gui64SyncPointStamp++;
	mutex_unlock(&obj->sTimelineLock);

err_out:
	return psPt;
err_free_pt:
	sync_pt_free((struct sync_pt *)psPt);
	psPt = NULL;	
	goto err_out;
}

static IMG_BOOL PVRSyncIsSyncInfoInUse(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo)
{
	return !(psSyncInfo->psSyncData->ui32WriteOpsPending ==
			 psSyncInfo->psSyncData->ui32WriteOpsComplete &&
			 psSyncInfo->psSyncData->ui32ReadOpsPending ==
			 psSyncInfo->psSyncData->ui32ReadOpsComplete &&
			 psSyncInfo->psSyncData->ui32ReadOps2Pending ==
			 psSyncInfo->psSyncData->ui32ReadOps2Complete);
}

/* Releases a sync info by adding it to a deferred list to be freed later. */
static void
PVRSyncReleaseSyncInfo(struct PVR_SYNC_KERNEL_SYNC_INFO *psSyncInfo)
{
	unsigned long flags;

	spin_lock_irqsave(&gSyncInfoFreeListLock, flags);
	list_add_tail(&psSyncInfo->sHead, &gSyncInfoFreeList);
	spin_unlock_irqrestore(&gSyncInfoFreeListLock, flags);

	queue_work(gpsWorkQueue, &gsWork);
}

static void PVRSyncFreeSyncData(struct PVR_SYNC_DATA *psSyncData)
{
	PVR_ASSERT(atomic_read(&psSyncData->sRefcount) == 0);
	PVRSyncReleaseSyncInfo(psSyncData->psSyncInfo);
	psSyncData->psSyncInfo = NULL;
	kfree(psSyncData);
}

static void PVRSyncFreeSync(struct sync_pt *psPt)
{
	struct PVR_SYNC *psSync = (struct PVR_SYNC *)psPt;
#if defined(DEBUG_PRINT)
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo =
		psSync->psSyncData->psSyncInfo->psBase;
#endif

	PVR_ASSERT(atomic_read(&psSync->psSyncData->sRefcount) > 0);

	/* Only free on the last reference */
	if (atomic_dec_return(&psSync->psSyncData->sRefcount) != 0)
		return;

	DPF("R( ): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X "
		"WOP/C=0x%x/0x%x ROP/C=0x%x/0x%x RO2P/C=0x%x/0x%x "
		"ID=%llu, S=0x%x, F=%p",
		psSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr,
		psSyncInfo->sReadOpsCompleteDevVAddr.uiAddr,
		psSyncInfo->sReadOps2CompleteDevVAddr.uiAddr,
		psSyncInfo->psSyncData->ui32WriteOpsPending,
		psSyncInfo->psSyncData->ui32WriteOpsComplete,
		psSyncInfo->psSyncData->ui32ReadOpsPending,
		psSyncInfo->psSyncData->ui32ReadOpsComplete,
		psSyncInfo->psSyncData->ui32ReadOps2Pending,
		psSyncInfo->psSyncData->ui32ReadOps2Complete,
		psSync->psSyncData->ui64Stamp,
		psSync->psSyncData->ui32WOPSnapshot,
		psSync->pt.fence);

	PVRSyncFreeSyncData(psSync->psSyncData);
	psSync->psSyncData = NULL;
}

static struct sync_pt *PVRSyncDup(struct sync_pt *sync_pt)
{
	struct PVR_SYNC *psPt, *psParentPt = (struct PVR_SYNC *)sync_pt;

	psPt = (struct PVR_SYNC *)
		sync_pt_create(sync_pt->parent, sizeof(struct PVR_SYNC));
	if(!psPt)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: sync_pt_create failed", __func__));
		goto err_out;
	}

	psPt->psSyncData = psParentPt->psSyncData;
	atomic_inc(&psPt->psSyncData->sRefcount);

	PVR_ASSERT(atomic_read(&psPt->psSyncData->sRefcount) > 1);

err_out:
	return (struct sync_pt*)psPt;
}

static int PVRSyncHasSignaled(struct sync_pt *sync_pt)
{
	struct PVR_SYNC *psPt = (struct PVR_SYNC *)sync_pt;
	struct PVR_SYNC_TIMELINE *psTimeline =
		(struct PVR_SYNC_TIMELINE *) sync_pt->parent;
	PVRSRV_SYNC_DATA *psSyncData =
		psPt->psSyncData->psSyncInfo->psBase->psSyncData;

	if (psSyncData->ui32WriteOpsComplete >= psSyncData->ui32WriteOpsPending)
	{
		psTimeline->bSyncHasSignaled = IMG_TRUE;
		return 1;
	}

	return 0;
}

static int PVRSyncCompare(struct sync_pt *a, struct sync_pt *b)
{
	IMG_UINT64 ui64StampA = ((struct PVR_SYNC *)a)->psSyncData->ui64Stamp;
	IMG_UINT64 ui64StampB = ((struct PVR_SYNC *)b)->psSyncData->ui64Stamp;

	if (ui64StampA == ui64StampB)
		return 0;
	else if (ui64StampA > ui64StampB)
		return 1;
	else
		return -1;
}

static void PVRSyncPrintTimeline(struct seq_file *s,
								 struct sync_timeline *psObj)
{
	struct PVR_SYNC_TIMELINE *psTimeline = (struct PVR_SYNC_TIMELINE *)psObj;

	seq_printf(s, "WOP/WOC=0x%x/0x%x",
	           psTimeline->psSyncInfo->psBase->psSyncData->ui32WriteOpsPending,
	           psTimeline->psSyncInfo->psBase->psSyncData->ui32WriteOpsComplete);
}

static void PVRSyncPrint(struct seq_file *s, struct sync_pt *psPt)
{
	struct PVR_SYNC *psSync = (struct PVR_SYNC *)psPt;
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo =
		psSync->psSyncData->psSyncInfo->psBase;

	seq_printf(s, "ID=%llu, refs=%u, WOPSnapshot=0x%x, parent=%p",
				  psSync->psSyncData->ui64Stamp,
				  atomic_read(&psSync->psSyncData->sRefcount),
				  psSync->psSyncData->ui32WOPSnapshot,
				  psSync->pt.parent);
	seq_printf(s, "\n   WOP/WOC=0x%x/0x%x, "
	              "ROP/ROC=0x%x/0x%x, ROP2/ROC2=0x%x/0x%x, "
	              "WOC DevVA=0x%.8x, ROC DevVA=0x%.8x, "
	              "ROC2 DevVA=0x%.8x",
	              psSyncInfo->psSyncData->ui32WriteOpsPending,
	              psSyncInfo->psSyncData->ui32WriteOpsComplete,
	              psSyncInfo->psSyncData->ui32ReadOpsPending,
	              psSyncInfo->psSyncData->ui32ReadOpsComplete,
	              psSyncInfo->psSyncData->ui32ReadOps2Pending,
	              psSyncInfo->psSyncData->ui32ReadOps2Complete,
	              psSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr,
	              psSyncInfo->sReadOpsCompleteDevVAddr.uiAddr,
	              psSyncInfo->sReadOps2CompleteDevVAddr.uiAddr);
}

static void PVRSyncReleaseTimeline(struct sync_timeline *psObj)
{
	struct PVR_SYNC_TIMELINE *psTimeline = (struct PVR_SYNC_TIMELINE *)psObj;

	mutex_lock(&gTimelineListLock);
	list_del(&psTimeline->sTimelineList);
	mutex_unlock(&gTimelineListLock);

	DPF("R(t): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X "
		"WOP/C=0x%x/0x%x ROP/C=0x%x/0x%x RO2P/C=0x%x/0x%x T=%p",
		psTimeline->psSyncInfo->psBase->sWriteOpsCompleteDevVAddr.uiAddr,
		psTimeline->psSyncInfo->psBase->sReadOpsCompleteDevVAddr.uiAddr,
		psTimeline->psSyncInfo->psBase->sReadOps2CompleteDevVAddr.uiAddr,
		psTimeline->psSyncInfo->psBase->psSyncData->ui32WriteOpsPending,
		psTimeline->psSyncInfo->psBase->psSyncData->ui32WriteOpsComplete,
		psTimeline->psSyncInfo->psBase->psSyncData->ui32ReadOpsPending,
		psTimeline->psSyncInfo->psBase->psSyncData->ui32ReadOpsComplete,
		psTimeline->psSyncInfo->psBase->psSyncData->ui32ReadOps2Pending,
		psTimeline->psSyncInfo->psBase->psSyncData->ui32ReadOps2Complete,
		psTimeline);

	PVRSyncReleaseSyncInfo(psTimeline->psSyncInfo);
	psTimeline->psSyncInfo = NULL;
}

static PVRSRV_ERROR PVRSyncInitServices(void)
{
	IMG_BOOL bCreated, bShared[PVRSRV_MAX_CLIENT_HEAPS];
	PVRSRV_HEAP_INFO sHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
	IMG_UINT32 ui32ClientHeapCount = 0;
	PVRSRV_PER_PROCESS_DATA	*psPerProc;
	PVRSRV_ERROR eError;

	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	gsSyncServicesConnection.ui32Pid = OSGetCurrentProcessIDKM();

	eError = PVRSRVProcessConnect(gsSyncServicesConnection.ui32Pid, 0);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVProcessConnect failed",
								__func__));
		goto err_unlock;
	}

	psPerProc = PVRSRVFindPerProcessData();
	if (!psPerProc)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVFindPerProcessData failed",
								__func__));
		goto err_disconnect;
	}

	eError = PVRSRVAcquireDeviceDataKM(0, PVRSRV_DEVICE_TYPE_SGX,
									   &gsSyncServicesConnection.hDevCookie);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVAcquireDeviceDataKM failed",
								__func__));
		goto err_disconnect;
	}

	if (!gsSyncServicesConnection.hDevCookie)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: hDevCookie is NULL", __func__));
		goto err_disconnect;
	}

	eError = PVRSRVCreateDeviceMemContextKM(gsSyncServicesConnection.hDevCookie,
	                                        psPerProc,
											&gsSyncServicesConnection.hDevMemContext,
											&ui32ClientHeapCount,
											&sHeapInfo[0],
											&bCreated,
											&bShared[0]);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSRVCreateDeviceMemContextKM failed",
								__func__));
		goto err_disconnect;
	}

	if (!gsSyncServicesConnection.hDevMemContext)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: hDevMemContext is NULL", __func__));
		goto err_disconnect;
	}

err_unlock:
	LinuxUnLockMutex(&gPVRSRVLock);
	return eError;

err_disconnect:
	PVRSRVProcessDisconnect(gsSyncServicesConnection.ui32Pid);
	goto err_unlock;
}

static void PVRSyncCloseServices(void)
{
	IMG_BOOL bDummy;

	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	PVRSRVDestroyDeviceMemContextKM(gsSyncServicesConnection.hDevCookie,
									gsSyncServicesConnection.hDevMemContext,
									&bDummy);
	gsSyncServicesConnection.hDevMemContext = NULL;
	gsSyncServicesConnection.hDevCookie = NULL;

	PVRSRVProcessDisconnect(gsSyncServicesConnection.ui32Pid);
	gsSyncServicesConnection.ui32Pid = 0;

	LinuxUnLockMutex(&gPVRSRVLock);
}

static struct sync_timeline_ops gsTimelineOps =
{
	.driver_name		= "pvr_sync",
	.dup				= PVRSyncDup,
	.has_signaled		= PVRSyncHasSignaled,
	.compare			= PVRSyncCompare,
	.release_obj		= PVRSyncReleaseTimeline,
	.print_obj			= PVRSyncPrintTimeline,
	.print_pt			= PVRSyncPrint,
	.free_pt			= PVRSyncFreeSync,
};

static struct PVR_SYNC_TIMELINE *PVRSyncCreateTimeline(const IMG_CHAR *pszName)
{
	struct PVR_SYNC_TIMELINE *psTimeline;
	PVRSRV_ERROR eError;

	psTimeline = (struct PVR_SYNC_TIMELINE *)
		sync_timeline_create(&gsTimelineOps, sizeof(struct PVR_SYNC_TIMELINE),
							 pszName);
	if (!psTimeline)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: sync_timeline_create failed", __func__));
		goto err_out;
	}

	psTimeline->psSyncInfo =
		kmalloc(sizeof(struct PVR_SYNC_KERNEL_SYNC_INFO), GFP_KERNEL);
	if(!psTimeline->psSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate "
								"PVR_SYNC_KERNEL_SYNC_INFO", __func__));
		goto err_free_timeline;
	}

	psTimeline->bSyncHasSignaled = IMG_FALSE;

	mutex_init(&psTimeline->sTimelineLock);

	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);
	eError = PVRSRVAllocSyncInfoKM(gsSyncServicesConnection.hDevCookie,
								   gsSyncServicesConnection.hDevMemContext,
								   &psTimeline->psSyncInfo->psBase);
	LinuxUnLockMutex(&gPVRSRVLock);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate timeline syncinfo",
								__func__));
		goto err_free_syncinfo;
	}

	DPF("A(t): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X T=%p %s",
		psTimeline->psSyncInfo->psBase->sWriteOpsCompleteDevVAddr.uiAddr,
		psTimeline->psSyncInfo->psBase->sReadOpsCompleteDevVAddr.uiAddr,
		psTimeline->psSyncInfo->psBase->sReadOps2CompleteDevVAddr.uiAddr,
		psTimeline, pszName);

err_out:
	return psTimeline;
err_free_syncinfo:
	kfree(psTimeline->psSyncInfo);
err_free_timeline:
	sync_timeline_destroy((struct sync_timeline *)psTimeline);
	psTimeline = NULL;
	goto err_out;
}

static int PVRSyncOpen(struct inode *inode, struct file *file)
{
	struct PVR_SYNC_TIMELINE *psTimeline;
	IMG_CHAR task_comm[TASK_COMM_LEN+1];

	get_task_comm(task_comm, current);

	psTimeline = PVRSyncCreateTimeline(task_comm);
	if (!psTimeline)
		return -ENOMEM;

	mutex_lock(&gTimelineListLock);
	list_add_tail(&psTimeline->sTimelineList, &gTimelineList);
	mutex_unlock(&gTimelineListLock);

	file->private_data = psTimeline;
	return 0;
}

static int PVRSyncRelease(struct inode *inode, struct file *file)
{
	struct PVR_SYNC_TIMELINE *psTimeline = file->private_data;
	sync_timeline_destroy(&psTimeline->obj);
	return 0;
}

static long
PVRSyncIOCTLCreate(struct PVR_SYNC_TIMELINE *psObj, void __user *pvData)
{
	struct PVR_SYNC_KERNEL_SYNC_INFO *psProvidedSyncInfo = NULL;
	struct PVR_ALLOC_SYNC_DATA *psAllocSyncData;
	struct PVR_SYNC_CREATE_IOCTL_DATA sData;
	int err = -EFAULT, iFd = get_unused_fd();
	struct sync_fence *psFence;
	struct sync_pt *psPt;

	if (iFd < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to find unused fd (%d)",
								__func__, iFd));
		goto err_out;
	}

	if (!access_ok(VERIFY_READ, pvData, sizeof(sData)))
		goto err_put_fd;

	if (copy_from_user(&sData, pvData, sizeof(sData)))
		goto err_put_fd;

	if (sData.allocdSyncInfo < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Requested to create a fence from "
								" an invalid alloc'd fd (%d)", __func__,
								sData.allocdSyncInfo));
		goto err_put_fd;
	}

	psAllocSyncData = PVRSyncAllocFDGet(sData.allocdSyncInfo);
	if (!psAllocSyncData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: PVRSyncAllocFDGet returned NULL, "
								"possibly fd passed to CREATE is not an "
								"ALLOC'd sync?", __func__));
		goto err_put_fd;
	}

	/* Move the psSyncInfo to the newly created sync, to avoid attempting
	 * to create multiple syncs from the same allocation.
	 */
	psProvidedSyncInfo = psAllocSyncData->psSyncInfo;
	psAllocSyncData->psSyncInfo = NULL;

	if (psProvidedSyncInfo == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Alloc'd sync info is null - "
								"possibly already CREATEd?", __func__));
		fput(psAllocSyncData->psFile);
		goto err_put_fd;
	}

	fput(psAllocSyncData->psFile);

	psPt = (struct sync_pt *)PVRSyncCreateSync(psObj, psProvidedSyncInfo);
	if (!psPt)
	{
		err = -ENOMEM;
		goto err_put_fd;
	}

	sData.name[sizeof(sData.name) - 1] = '\0';
	psFence = sync_fence_create(sData.name, psPt);
	if (!psFence)
	{
		sync_pt_free(psPt);
		err = -ENOMEM;
		goto err_put_fd;
	}

	sData.fence = iFd;

	if (!access_ok(VERIFY_WRITE, pvData, sizeof(sData)))
	{
		sync_fence_put(psFence);
		goto err_put_fd;
	}

	if (copy_to_user(pvData, &sData, sizeof(sData)))
	{
		sync_fence_put(psFence);
		goto err_put_fd;
	}

	/* If the fence is a 'real' one, its signal status will be updated by
	 * the MISR calling PVRSyncUpdateAllSyncs(). However, if we created
	 * a 'fake' fence (for power optimization reasons) it has already
	 * completed, and needs to be marked signalled (as the MISR will
	 * never run for 'fake' fences).
	 */
	if(psProvidedSyncInfo->psBase->psSyncData->ui32WriteOpsPending == 0)
		sync_timeline_signal((struct sync_timeline *)psObj);

	DPF("C( ): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X F=%p %s",
		psProvidedSyncInfo->psBase->sWriteOpsCompleteDevVAddr.uiAddr,
		psProvidedSyncInfo->psBase->sReadOpsCompleteDevVAddr.uiAddr,
		psProvidedSyncInfo->psBase->sReadOps2CompleteDevVAddr.uiAddr,
		psFence, sData.name);

	sync_fence_install(psFence, iFd);
	err = 0;
err_out:
	return err;

err_put_fd:
	put_unused_fd(iFd);
	goto err_out;
}

static long
PVRSyncIOCTLDebug(struct PVR_SYNC_TIMELINE *psObj, void __user *pvData)
{
	struct PVR_SYNC_DEBUG_IOCTL_DATA sData;
	struct sync_fence *psFence;
	struct list_head *psEntry;
	int i = 0, err = -EFAULT;

	if(!access_ok(VERIFY_READ, pvData, sizeof(sData)))
		goto err_out;

	if(copy_from_user(&sData, pvData, sizeof(sData)))
		goto err_out;

	psFence = sync_fence_fdget(sData.iFenceFD);
	if(!psFence)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get fence from fd", __func__));
		goto err_out;
	}

	list_for_each(psEntry, &psFence->pt_list_head)
	{
		PVR_SYNC_DEBUG *psMetaData = &sData.sSync[i].sMetaData;
		PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;
		struct PVR_SYNC_TIMELINE *psTimeline;
		struct PVR_SYNC *psPt;

		if(i == PVR_SYNC_DEBUG_MAX_POINTS)
		{
			PVR_DPF((PVR_DBG_WARNING, "%s: Fence merged with more than %d "
									  "points", __func__,
									  PVR_SYNC_DEBUG_MAX_POINTS));
			break;
		}

		psPt = (struct PVR_SYNC *)
			container_of(psEntry, struct sync_pt, pt_list);

		/* Don't dump foreign points */
		if(psPt->pt.parent->ops != &gsTimelineOps)
			continue;

		psTimeline = (struct PVR_SYNC_TIMELINE *)psPt->pt.parent;
		psKernelSyncInfo = psPt->psSyncData->psSyncInfo->psBase;
		PVR_ASSERT(psKernelSyncInfo != NULL);

		/* The sync refcount is valid as long as the FenceFD stays open,
		 * so we can access it directly without worrying about it being
		 * freed.
		 */
		sData.sSync[i].sSyncData = *psKernelSyncInfo->psSyncData;

		psMetaData->ui64Stamp                   = psPt->psSyncData->ui64Stamp;
		psMetaData->ui32WriteOpsPendingSnapshot = psPt->psSyncData->ui32WOPSnapshot;
		i++;
	}

	sync_fence_put(psFence);

	sData.ui32NumPoints = i;

	if(!access_ok(VERIFY_WRITE, pvData, sizeof(sData)))
		goto err_out;

	if(copy_to_user(pvData, &sData, sizeof(sData)))
		goto err_out;

	err = 0;
err_out:
	return err;
}

static int PVRSyncFenceAllocRelease(struct inode *inode, struct file *file)
{
	struct PVR_ALLOC_SYNC_DATA *psAllocSyncData = file->private_data;

	if(psAllocSyncData->psSyncInfo)
	{
#if defined(DEBUG_PRINT)
		PVRSRV_KERNEL_SYNC_INFO *psSyncInfo =
			psAllocSyncData->psSyncInfo->psBase;
#endif

		DPF("R(a): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X",
			psSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr,
			psSyncInfo->sReadOpsCompleteDevVAddr.uiAddr,
			psSyncInfo->sReadOps2CompleteDevVAddr.uiAddr);

		PVRSyncReleaseSyncInfo(psAllocSyncData->psSyncInfo);
		psAllocSyncData->psSyncInfo = NULL;
	}

	kfree(psAllocSyncData);
	return 0;
}

static const struct file_operations gsSyncFenceAllocFOps =
{
	.release = PVRSyncFenceAllocRelease,
};

static struct PVR_ALLOC_SYNC_DATA *PVRSyncAllocFDGet(int fd)
{
	struct file *file = fget(fd);
	if (!file)
		return NULL;
	if (file->f_op != &gsSyncFenceAllocFOps)
		goto err;
	return file->private_data;
err:
	fput(file);
	return NULL;
}

static long
PVRSyncIOCTLAlloc(struct PVR_SYNC_TIMELINE *psTimeline, void __user *pvData)
{
	struct PVR_ALLOC_SYNC_DATA *psAllocSyncData;
	int err = -EFAULT, iFd = get_unused_fd();
	struct PVR_SYNC_ALLOC_IOCTL_DATA sData;
	PVRSRV_SYNC_DATA *psSyncData;
	struct file *psFile;
	PVRSRV_ERROR eError;

	if (iFd < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to find unused fd (%d)",
		                        __func__, iFd));
		goto err_out;
	}

	if (!access_ok(VERIFY_READ, pvData, sizeof(sData)))
		goto err_put_fd;

	if (copy_from_user(&sData, pvData, sizeof(sData)))
		goto err_put_fd;

	psAllocSyncData = kmalloc(sizeof(struct PVR_ALLOC_SYNC_DATA), GFP_KERNEL);
	if (!psAllocSyncData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate PVR_ALLOC_SYNC_DATA",
								__func__));
		err = -ENOMEM;
		goto err_put_fd;
	}

	psAllocSyncData->psSyncInfo =
		kmalloc(sizeof(struct PVR_SYNC_KERNEL_SYNC_INFO), GFP_KERNEL);
	if (!psAllocSyncData->psSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate "
								"PVR_SYNC_KERNEL_SYNC_INFO", __func__));
		err = -ENOMEM;
		goto err_free_alloc_sync_data;
	}

	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);
	eError = PVRSRVAllocSyncInfoKM(gsSyncServicesConnection.hDevCookie,
	                               gsSyncServicesConnection.hDevMemContext,
								   &psAllocSyncData->psSyncInfo->psBase);
	LinuxUnLockMutex(&gPVRSRVLock);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to alloc syncinfo (%d)",
								__func__, eError));
		err = -ENOMEM;
		goto err_free_sync_info;
	}

	psFile = anon_inode_getfile("pvr_sync_alloc",
								&gsSyncFenceAllocFOps, psAllocSyncData, 0);
	if (!psFile)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create anon inode",
								__func__));
		err = -ENOMEM;
		goto err_release_sync_info;
	}

	sData.fence = iFd;

	/* Check if this timeline looks idle. If there are still TQs running
	 * on it, userspace shouldn't attempt any kind of power optimization
	 * (e.g. it must not dummy-process GPU fences).
	 *
	 * Determining idleness here is safe because the ALLOC and CREATE
	 * pvr_sync ioctls must be called under the gralloc module lock, so
	 * we can't be creating another new fence op while we are still
	 * processing this one.
	 *
 	 * Take the bridge lock anyway so we can be sure that we read the
	 * timeline sync's pending value coherently. The complete value may
	 * be modified by the GPU, but worse-case we will decide we can't do
	 * the power optimization and will still be correct.
	 */
	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	psSyncData = psTimeline->psSyncInfo->psBase->psSyncData;
	if(psSyncData->ui32WriteOpsPending == psSyncData->ui32WriteOpsComplete)
		sData.bTimelineIdle = IMG_TRUE;
	else
		sData.bTimelineIdle = IMG_FALSE;

	LinuxUnLockMutex(&gPVRSRVLock);

	if (!access_ok(VERIFY_WRITE, pvData, sizeof(sData)))
		goto err_release_file;

	if (copy_to_user(pvData, &sData, sizeof(sData)))
		goto err_release_file;

	psAllocSyncData->psTimeline = psTimeline;
	psAllocSyncData->psFile = psFile;

	DPF("A( ): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X",
		psAllocSyncData->psSyncInfo->psBase->sWriteOpsCompleteDevVAddr.uiAddr,
		psAllocSyncData->psSyncInfo->psBase->sReadOpsCompleteDevVAddr.uiAddr,
		psAllocSyncData->psSyncInfo->psBase->sReadOps2CompleteDevVAddr.uiAddr);

	fd_install(iFd, psFile);
	err = 0;
err_out:
	return err;
err_release_sync_info:
	PVRSRVReleaseSyncInfoKM(psAllocSyncData->psSyncInfo->psBase);
err_free_sync_info:
	kfree(psAllocSyncData->psSyncInfo);
err_free_alloc_sync_data:
	kfree(psAllocSyncData);
err_put_fd:
	put_unused_fd(iFd);
	goto err_out;
err_release_file:
	fput(psFile);
	put_unused_fd(iFd);
	goto err_out;
}

static long
PVRSyncIOCTL(struct file *file, unsigned int cmd, unsigned long __user arg)
{
	struct PVR_SYNC_TIMELINE *psTimeline = file->private_data;
	void __user *pvData = (void __user *)arg;

	switch (cmd)
	{
		case PVR_SYNC_IOC_CREATE_FENCE:
			return PVRSyncIOCTLCreate(psTimeline, pvData);
		case PVR_SYNC_IOC_DEBUG_FENCE:
			return PVRSyncIOCTLDebug(psTimeline, pvData);
		case PVR_SYNC_IOC_ALLOC_FENCE:
			return PVRSyncIOCTLAlloc(psTimeline, pvData);
		default:
			return -ENOTTY;
	}
}

static void PVRSyncWorkQueueFunction(struct work_struct *data)
{
	PVRSRV_DEVICE_NODE *psDevNode =
		(PVRSRV_DEVICE_NODE*)gsSyncServicesConnection.hDevCookie;
	struct list_head sFreeList, *psEntry, *n;
	unsigned long flags;

	/* We lock the bridge mutex here for two reasons.
	 *
	 * Firstly, the SGXScheduleProcessQueuesKM and PVRSRVReleaseSyncInfoKM
	 * functions require that they are called under lock. Multiple threads
	 * into services are not allowed.
	 *
	 * Secondly, we need to ensure that when processing the defer-free list,
	 * the PVRSyncIsSyncInfoInUse() function is called *after* any freed
	 * sync was attached as a HW dependency (had ROP/ROP2 taken). This is
	 * because for 'foreign' sync timelines we allocate a new object and
	 * mark it for deletion immediately. If the 'foreign' sync_pt signals
	 * before the kick ioctl has completed, we can block it from being
	 * prematurely freed by holding the bridge mutex.
	 *
	 * NOTE: This code relies on the assumption that we can acquire a
	 * spinlock while a mutex is held and that other users of the spinlock
	 * do not need to hold the bridge mutex.
	 */
	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	/* A completed SW operation may un-block the GPU */
	SGXScheduleProcessQueuesKM(psDevNode);

	/* We can't call PVRSRVReleaseSyncInfoKM directly in this loop because
	 * that will take the mmap mutex. We can't take mutexes while we have
	 * this list locked with a spinlock. So move all the items we want to
	 * free to another, local list (no locking required) and process it
	 * in a second loop.
	 */

	INIT_LIST_HEAD(&sFreeList);
	spin_lock_irqsave(&gSyncInfoFreeListLock, flags);
	list_for_each_safe(psEntry, n, &gSyncInfoFreeList)
	{
		struct PVR_SYNC_KERNEL_SYNC_INFO *psSyncInfo =
			container_of(psEntry, struct PVR_SYNC_KERNEL_SYNC_INFO, sHead);

		if(!PVRSyncIsSyncInfoInUse(psSyncInfo->psBase))
			list_move_tail(psEntry, &sFreeList);

	}
	spin_unlock_irqrestore(&gSyncInfoFreeListLock, flags);

	list_for_each_safe(psEntry, n, &sFreeList)
	{
		struct PVR_SYNC_KERNEL_SYNC_INFO *psSyncInfo =
			container_of(psEntry, struct PVR_SYNC_KERNEL_SYNC_INFO, sHead);

		list_del(psEntry);

		DPF("F(d): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X",
			psSyncInfo->psBase->sWriteOpsCompleteDevVAddr.uiAddr,
			psSyncInfo->psBase->sReadOpsCompleteDevVAddr.uiAddr,
			psSyncInfo->psBase->sReadOps2CompleteDevVAddr.uiAddr);

		PVRSRVReleaseSyncInfoKM(psSyncInfo->psBase);
		psSyncInfo->psBase = NULL;

		kfree(psSyncInfo);
	}

	LinuxUnLockMutex(&gPVRSRVLock);

	/* Copying from one list to another (so a spinlock isn't held) used to
	 * work around the problem that PVRSyncReleaseSyncInfo() would hold the
	 * services mutex. However, we no longer do this, so this code could
	 * potentially be simplified.
	 *
	 * Note however that sync_fence_put must be called from process/WQ
	 * context because it uses fput(), which is not allowed to be called
	 * from interrupt context in kernels <3.6.
	 */
	INIT_LIST_HEAD(&sFreeList);
	spin_lock_irqsave(&gFencePutListLock, flags);
	list_for_each_safe(psEntry, n, &gFencePutList)
	{
		list_move_tail(psEntry, &sFreeList);
	}
	spin_unlock_irqrestore(&gFencePutListLock, flags);

	list_for_each_safe(psEntry, n, &sFreeList)
	{
		struct PVR_SYNC_FENCE *psSyncFence =
			container_of(psEntry, struct PVR_SYNC_FENCE, sHead);

		list_del(psEntry);

		sync_fence_put(psSyncFence->psBase);
		psSyncFence->psBase = NULL;

		kfree(psSyncFence);
	}
}

static const struct file_operations gsPVRSyncFOps =
{
	.owner			= THIS_MODULE,
	.open			= PVRSyncOpen,
	.release		= PVRSyncRelease,
	.unlocked_ioctl	= PVRSyncIOCTL,
};

static struct miscdevice gsPVRSyncDev =
{
	.minor			= MISC_DYNAMIC_MINOR,
	.name			= "pvr_sync",
	.fops			= &gsPVRSyncFOps,
};

IMG_INTERNAL
int PVRSyncDeviceInit(void)
{
	int err = -1;

	if(PVRSyncInitServices() != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to initialise services",
								__func__));
		goto err_out;
	}

	gpsWorkQueue = create_freezable_workqueue("pvr_sync_workqueue");
	if(!gpsWorkQueue)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to create pvr_sync workqueue",
								__func__));
		goto err_deinit_services;
	}

	INIT_WORK(&gsWork, PVRSyncWorkQueueFunction);

	err = misc_register(&gsPVRSyncDev);
	if(err)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to register pvr_sync misc "
								"device (err=%d)", __func__, err));
		goto err_deinit_services;
	}

	err = 0;
err_out:
	return err;
err_deinit_services:
	PVRSyncCloseServices();
	goto err_out;
}

IMG_INTERNAL
void PVRSyncDeviceDeInit(void)
{
	misc_deregister(&gsPVRSyncDev);
	destroy_workqueue(gpsWorkQueue);
	PVRSyncCloseServices();
}

IMG_INTERNAL
void PVRSyncUpdateAllSyncs(void)
{
	IMG_BOOL bNeedToProcessQueues = IMG_FALSE;
	struct list_head *psEntry;

	/* Check to see if any syncs have signalled. If they have, it may unblock
	 * the GPU. Decide what is needed and optionally schedule queue
	 * processing.
	 */
	mutex_lock(&gTimelineListLock);
	list_for_each(psEntry, &gTimelineList)
	{
		struct PVR_SYNC_TIMELINE *psTimeline =
			container_of(psEntry, struct PVR_SYNC_TIMELINE, sTimelineList);

		sync_timeline_signal((struct sync_timeline *)psTimeline);

		if(psTimeline->bSyncHasSignaled)
		{
			psTimeline->bSyncHasSignaled = IMG_FALSE;
			bNeedToProcessQueues = IMG_TRUE;
		}
	}
	mutex_unlock(&gTimelineListLock);

	if(bNeedToProcessQueues)
		queue_work(gpsWorkQueue, &gsWork);
}

static IMG_BOOL
PVRSyncIsDuplicate(PVRSRV_KERNEL_SYNC_INFO *psA, PVRSRV_KERNEL_SYNC_INFO *psB)
{
	return psA->sWriteOpsCompleteDevVAddr.uiAddr ==
		   psB->sWriteOpsCompleteDevVAddr.uiAddr ? IMG_TRUE : IMG_FALSE;
}

static void ForeignSyncPtSignaled(struct sync_fence *fence,
								  struct sync_fence_waiter *waiter)
{
	struct PVR_SYNC_FENCE_WAITER *psWaiter =
		(struct PVR_SYNC_FENCE_WAITER *)waiter;
	unsigned long flags;

	PVRSyncSWCompleteOp(psWaiter->psSyncInfo->psBase);

	DPF("R(f): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X "
		"WOP/C=0x%x/0x%x ROP/C=0x%x/0x%x RO2P/C=0x%x/0x%x",
		psWaiter->psSyncInfo->psBase->sWriteOpsCompleteDevVAddr.uiAddr,
		psWaiter->psSyncInfo->psBase->sReadOpsCompleteDevVAddr.uiAddr,
		psWaiter->psSyncInfo->psBase->sReadOps2CompleteDevVAddr.uiAddr,
		psWaiter->psSyncInfo->psBase->psSyncData->ui32WriteOpsPending,
		psWaiter->psSyncInfo->psBase->psSyncData->ui32WriteOpsComplete,
		psWaiter->psSyncInfo->psBase->psSyncData->ui32ReadOpsPending,
		psWaiter->psSyncInfo->psBase->psSyncData->ui32ReadOpsComplete,
		psWaiter->psSyncInfo->psBase->psSyncData->ui32ReadOps2Pending,
		psWaiter->psSyncInfo->psBase->psSyncData->ui32ReadOps2Complete);

	PVRSyncReleaseSyncInfo(psWaiter->psSyncInfo);
	psWaiter->psSyncInfo = NULL;

	/* We can 'put' the fence now, but this function might be called in irq
	 * context so we must defer to WQ.
	 */
	spin_lock_irqsave(&gFencePutListLock, flags);
	list_add_tail(&psWaiter->psSyncFence->sHead, &gFencePutList);
	psWaiter->psSyncFence = NULL;
	spin_unlock_irqrestore(&gFencePutListLock, flags);

	/* The PVRSyncReleaseSyncInfo() call above already queued work */
	/*queue_work(gpsWorkQueue, &gsWork);*/

	kfree(psWaiter);
}

static PVRSRV_KERNEL_SYNC_INFO *ForeignSyncPointToSyncInfo(int iFenceFd)
{
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;
	struct PVR_SYNC_FENCE_WAITER *psWaiter;
	struct PVR_SYNC_FENCE *psSyncFence;
	struct sync_fence *psFence;
	PVRSRV_ERROR eError;
	int err;

	/* FIXME: Could optimize this function by pre-testing sync_wait(.., 0)
	 *        to determine if it has already signalled. We must avoid this
	 *        for now because the sync driver was broken in earlier kernels.
	 */

	/* The custom waiter structure is freed in the waiter callback */
	psWaiter = kmalloc(sizeof(struct PVR_SYNC_FENCE_WAITER), GFP_KERNEL);
	if(!psWaiter)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate waiter", __func__));
		goto err_out;
	}

	psWaiter->psSyncInfo =
		kmalloc(sizeof(struct PVR_SYNC_KERNEL_SYNC_INFO), GFP_KERNEL);
	if(!psWaiter->psSyncInfo)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate "
								"PVR_SYNC_KERNEL_SYNC_INFO", __func__));
		goto err_free_waiter;
	}

	/* We take another reference on the parent fence, each time we see a
	 * 'foreign' sync_pt. This is to ensure the timeline, fence and sync_pts
	 * from the foreign timeline cannot go away until the sync_pt signals.
	 * In practice this also means they will not go away until the entire
	 * fence signals. It means that we will always get a
	 * sync_fence_wait_async() callback for these points.
	 */
	psFence = sync_fence_fdget(iFenceFd);
	if(!psFence)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to take reference on fence",
								__func__));
		goto err_free_syncinfo;
	}

	/* Allocate packet we can store this fence on (with a list head) so we
	 * can add it to the defer-put list without allocating memory in irq
	 * context.
	 *
	 * NOTE: At the moment we allocate one of these per sync_pts, but it
	 * might be possible to optimize this to one per fence.
	 */
	psSyncFence = kmalloc(sizeof(struct PVR_SYNC_FENCE), GFP_KERNEL);
	if(!psSyncFence)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate "
								"PVR_SYNC_FENCE", __func__));
		goto err_sync_fence_put;
	}

	psSyncFence->psBase = psFence;
	psWaiter->psSyncFence = psSyncFence;

	/* Allocate a "shadow" SYNCINFO for this sync_pt and set it up to be
	 * completed by the callback.
	 */
	eError = PVRSRVAllocSyncInfoKM(gsSyncServicesConnection.hDevCookie,
								   gsSyncServicesConnection.hDevMemContext,
								   &psKernelSyncInfo);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to allocate syncinfo", __func__));
		goto err_free_sync_fence;
	}

	/* Make sure we take the SW operation before adding the waiter, to avoid
	 * racing with parallel completes.
	 */
	PVRSyncSWTakeOp(psKernelSyncInfo);

	sync_fence_waiter_init(&psWaiter->sWaiter, ForeignSyncPtSignaled);
	psWaiter->psSyncInfo->psBase = psKernelSyncInfo;

	err = sync_fence_wait_async(psFence, &psWaiter->sWaiter);
	if(err)
	{
		if(err < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Fence was in error state", __func__));
			/* Fall-thru */
		}

		/* -1 means the fence was broken, 1 means the fence already
		 * signalled. In either case, roll back what we've done and
		 * skip using this sync_pt for synchronization.
		 */
		goto err_release_sync_info;
	}

	DPF("A(f): WOCVA=0x%.8X ROCVA=0x%.8X RO2CVA=0x%.8X F=%p",
		psKernelSyncInfo->sWriteOpsCompleteDevVAddr.uiAddr,
		psKernelSyncInfo->sReadOpsCompleteDevVAddr.uiAddr,
		psKernelSyncInfo->sReadOps2CompleteDevVAddr.uiAddr,
		psFence);

	/* NOTE: Don't use psWaiter after this point as it may asynchronously
	 * signal before this function completes (and be freed already).
	 */

	/* Even if the fence signals while we're hanging on to this, the sync
	 * can't be freed until the bridge mutex is taken in the callback. The
	 * bridge mutex won't be released by the caller of this function until
	 * the GPU operation has been scheduled, which increments ROP,
	 * preventing the sync from being freed when still in use by the GPU.
	 */
	return psKernelSyncInfo;

err_release_sync_info:
	PVRSyncSWCompleteOp(psKernelSyncInfo);
	PVRSRVReleaseSyncInfoKM(psKernelSyncInfo);
err_free_sync_fence:
	kfree(psSyncFence);
err_sync_fence_put:
	sync_fence_put(psFence);
err_free_syncinfo:
	kfree(psWaiter->psSyncInfo);
err_free_waiter:
	kfree(psWaiter);
err_out:
	return NULL;
}

static void
CopyKernelSyncInfoToDeviceSyncObject(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
                                     PVRSRV_DEVICE_SYNC_OBJECT *psSyncObject)
{
	psSyncObject->sReadOpsCompleteDevVAddr  = psSyncInfo->sReadOpsCompleteDevVAddr;
	psSyncObject->sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
	psSyncObject->sReadOps2CompleteDevVAddr = psSyncInfo->sReadOps2CompleteDevVAddr;
	psSyncObject->ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
	psSyncObject->ui32ReadOpsPendingVal  = psSyncInfo->psSyncData->ui32ReadOpsPending;
	psSyncObject->ui32ReadOps2PendingVal = psSyncInfo->psSyncData->ui32ReadOps2Pending;
}

static IMG_BOOL FenceHasForeignPoints(struct sync_fence *psFence)
{
	struct list_head *psEntry;

	list_for_each(psEntry, &psFence->pt_list_head)
	{
		struct sync_pt *psPt =
			container_of(psEntry, struct sync_pt, pt_list);

		if(psPt->parent->ops != &gsTimelineOps)
			return IMG_TRUE;
	}

	return IMG_FALSE;
}

static IMG_BOOL
AddSyncInfoToArray(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
				   IMG_UINT32 ui32SyncPointLimit,
				   IMG_UINT32 *pui32NumRealSyncs,
				   PVRSRV_KERNEL_SYNC_INFO *apsSyncInfo[])
{
	/* Ran out of syncs. Not much userspace can do about this, since it
	 * could have been passed multiple merged syncs and doesn't know they
	 * were merged. Allow this through, but print a warning and stop
	 * synchronizing.
	 */
	if(*pui32NumRealSyncs == ui32SyncPointLimit)
	{
		PVR_DPF((PVR_DBG_WARNING, "%s: Ran out of source syncs %d == %d",
								  __func__, *pui32NumRealSyncs,
								  ui32SyncPointLimit));
		return IMG_FALSE;
	}

	apsSyncInfo[*pui32NumRealSyncs] = psSyncInfo;
	(*pui32NumRealSyncs)++;
	return IMG_TRUE;
}

static IMG_BOOL
ExpandAndDeDuplicateFenceSyncs(IMG_UINT32 ui32NumSyncs,
							   int aiFenceFds[],
							   IMG_UINT32 ui32SyncPointLimit,
							   struct sync_fence *apsFence[],
							   IMG_UINT32 *pui32NumRealSyncs,
							   PVRSRV_KERNEL_SYNC_INFO *apsSyncInfo[])
{
	IMG_UINT32 i, j, ui32FenceIndex = 0;
	IMG_BOOL bRet = IMG_TRUE;

	*pui32NumRealSyncs = 0;

	for(i = 0; i < ui32NumSyncs; i++)
	{
		PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
		struct list_head *psEntry;

		/* Skip any invalid fence file descriptors without error */
		if(aiFenceFds[i] < 0)
			continue;

		/* By converting a file descriptor to a struct sync_fence, we are
		 * taking a reference on the fence. We don't want the fence to go
		 * away until we have submitted the command, even if it signals
		 * before we dispatch the command, or the timeline(s) are destroyed.
		 *
		 * This reference should be released by the caller of this function
		 * once hardware operations have been scheduled on the GPU sync_pts
		 * participating in this fence. When our MISR is scheduled, the
		 * defer-free list will be processed, cleaning up the SYNCINFO.
		 *
		 * Note that this reference *isn't* enough for non-GPU sync_pts.
		 * We'll take another reference on the fence for those operations
		 * later (the life-cycle requirements there are totally different).
		 *
		 * Fence lookup may fail here if the fd became invalid since it was
		 * patched in userspace. That's really a userspace driver bug, so
		 * just fail here instead of not synchronizing.
		 */
		apsFence[ui32FenceIndex] = sync_fence_fdget(aiFenceFds[i]);
		if(!apsFence[ui32FenceIndex])
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get fence from fd=%d",
									__func__, aiFenceFds[i]));
			bRet = IMG_FALSE;
			goto err_out;
		}

		/* If this fence has any points from foreign timelines, we need to
		 * allocate a 'shadow' SYNCINFO and update it in software ourselves,
		 * so the ukernel can test the readiness of the dependency.
		 *
		 * It's tempting to just handle all fences like this (since most of
		 * the time they *will* be merged with sw_sync) but such 'shadow'
		 * syncs are slower. This is because we need to wait for the MISR to
		 * schedule to update the GPU part of the fence (normally the ukernel
		 * would be able to make the update directly).
		 */
		if(FenceHasForeignPoints(apsFence[ui32FenceIndex]))
		{
			psSyncInfo = ForeignSyncPointToSyncInfo(aiFenceFds[i]);
			if(psSyncInfo)
			{
				if(!AddSyncInfoToArray(psSyncInfo, ui32SyncPointLimit,
									   pui32NumRealSyncs, apsSyncInfo))
				{
					/* Soft-fail. Stop synchronizing. */
					goto err_out;
				}
			}
			ui32FenceIndex++;
			continue;
		}

		/* FIXME: The ForeignSyncPointToSyncInfo() path optimizes away already
		 *        signalled fences. Consider optimizing this path too.
		 */
		list_for_each(psEntry, &apsFence[ui32FenceIndex]->pt_list_head)
		{
			struct sync_pt *psPt =
				container_of(psEntry, struct sync_pt, pt_list);

			psSyncInfo =
				((struct PVR_SYNC *)psPt)->psSyncData->psSyncInfo->psBase;

			/* Walk the current list of points and make sure this isn't a
			 * duplicate. Duplicates will deadlock.
			 */
			for(j = 0; j < *pui32NumRealSyncs; j++)
			{
				/* The point is from a different timeline so we must use it */
				if(!PVRSyncIsDuplicate(apsSyncInfo[j], psSyncInfo))
					continue;

				/* There's no need to bump the real sync count as we either
				 * ignored the duplicate or replaced an previously counted
				 * entry.
			 	 */
				break;
			}

			if(j == *pui32NumRealSyncs)
			{
				/* It's not a duplicate; moving on.. */
				if(!AddSyncInfoToArray(psSyncInfo, ui32SyncPointLimit,
									   pui32NumRealSyncs, apsSyncInfo))
					goto err_out;
			}
		}

		ui32FenceIndex++;
	}

err_out:
	return bRet;
}

IMG_INTERNAL PVRSRV_ERROR
PVRSyncPatchCCBKickSyncInfos(IMG_HANDLE    ahSyncs[SGX_MAX_SRC_SYNCS_TA],
		      PVRSRV_DEVICE_SYNC_OBJECT asDevSyncs[SGX_MAX_SRC_SYNCS_TA],
							 IMG_UINT32 *pui32NumSrcSyncs)
{
	PVRSRV_KERNEL_SYNC_INFO *apsSyncInfo[SGX_MAX_SRC_SYNCS_TA];
	struct sync_fence *apsFence[SGX_MAX_SRC_SYNCS_TA] = {};
	IMG_UINT32 i, ui32NumRealSrcSyncs;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if(!ExpandAndDeDuplicateFenceSyncs(*pui32NumSrcSyncs,
									   (int *)ahSyncs,
									   SGX_MAX_SRC_SYNCS_TA,
									   apsFence,
									   &ui32NumRealSrcSyncs,
									   apsSyncInfo))
	{
		eError = PVRSRV_ERROR_HANDLE_NOT_FOUND;
		goto err_put_fence;
	}

	/* There should only be one destination sync for a transfer.
	 * Ultimately this will be patched to two (the sync_pt SYNCINFO,
	 * and the timeline's SYNCINFO for debugging).
	 */
	for(i = 0; i < ui32NumRealSrcSyncs; i++)
	{
		PVRSRV_KERNEL_SYNC_INFO *psSyncInfo = apsSyncInfo[i];

		/* The following code is mostly the same as the texture dependencies
		 * handling in SGXDoKickKM, but we have to copy it here because it
		 * must be run while the fence is 'locked' by sync_fence_fdget.
		 */

		PVR_TTRACE_SYNC_OBJECT(PVRSRV_TRACE_GROUP_KICK, KICK_TOKEN_SRC_SYNC,
		                       psSyncInfo, PVRSRV_SYNCOP_SAMPLE);

		CopyKernelSyncInfoToDeviceSyncObject(psSyncInfo, &asDevSyncs[i]);

		/* Texture dependencies are read operations */
		psSyncInfo->psSyncData->ui32ReadOpsPending++;

		/* Finally, patch the sync back into the input array.
		 * NOTE: The syncs are protected here by the defer-free worker.
		 */
		ahSyncs[i] = psSyncInfo;
	}

	/* Updating this allows the PDUMP handling and ROP rollbacks to work
	 * correctly in SGXDoKickKM.
	 */
	*pui32NumSrcSyncs = ui32NumRealSrcSyncs;

err_put_fence:
	for(i = 0; i < SGX_MAX_SRC_SYNCS_TA && apsFence[i]; i++)
		sync_fence_put(apsFence[i]);
	return eError;
}

IMG_INTERNAL PVRSRV_ERROR
PVRSyncPatchTransferSyncInfos(IMG_HANDLE    ahSyncs[SGX_MAX_SRC_SYNCS_TA],
			      PVRSRV_DEVICE_SYNC_OBJECT asDevSyncs[SGX_MAX_SRC_SYNCS_TA],
							     IMG_UINT32 *pui32NumSrcSyncs)
{
	struct PVR_ALLOC_SYNC_DATA *psTransferSyncData;
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (*pui32NumSrcSyncs != 1)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Invalid number of syncs (%d), clamping "
								"to 1", __func__, *pui32NumSrcSyncs));
	}
	
	psTransferSyncData = PVRSyncAllocFDGet((int)ahSyncs[0]);

	if (!psTransferSyncData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to get PVR_SYNC_DATA from "
								"supplied fd", __func__));
		eError = PVRSRV_ERROR_HANDLE_NOT_FOUND;
		goto err_out;
	}

	/* There should only be one destination sync for a transfer.
	 * Ultimately this will be patched to two (the sync_pt SYNCINFO,
	 * and the timeline's SYNCINFO for debugging).
	 */
	psSyncInfo = psTransferSyncData->psSyncInfo->psBase;

	/* The following code is mostly the same as the texture dependencies
	 * handling in SGXDoKickKM, but we have to copy it here because it
	 * must be run while the fence is 'locked' by sync_fence_fdget.
	 */

	PVR_TTRACE_SYNC_OBJECT(PVRSRV_TRACE_GROUP_TRANSFER, TRANSFER_TOKEN_SRC_SYNC,
	                       psSyncInfo, PVRSRV_SYNCOP_SAMPLE);

	CopyKernelSyncInfoToDeviceSyncObject(psSyncInfo, &asDevSyncs[0]);
	CopyKernelSyncInfoToDeviceSyncObject(psTransferSyncData->psTimeline->psSyncInfo->psBase,
	                                     &asDevSyncs[1]);

	/* Treat fence TQs as write operations */
	psSyncInfo->psSyncData->ui32WriteOpsPending++;
	psTransferSyncData->psTimeline->psSyncInfo->psBase->psSyncData->ui32WriteOpsPending++;

	/* Finally, patch the sync back into the input array.
	 * NOTE: The syncs are protected here by the defer-free worker.
	 */
	ahSyncs[0] = psSyncInfo;
	ahSyncs[1] = psTransferSyncData->psTimeline->psSyncInfo->psBase;

	/* Updating this allows the PDUMP handling and ROP rollbacks to work
	 * correctly in SGXDoKickKM.
	 */
	*pui32NumSrcSyncs = 2;

	fput(psTransferSyncData->psFile);
err_out:
	return eError;
}

/* NOTE: This returns an array of sync_fences which need to be 'put'
 *       or they will leak.
 */

IMG_INTERNAL PVRSRV_ERROR
PVRSyncFencesToSyncInfos(PVRSRV_KERNEL_SYNC_INFO *apsSyncs[],
						 IMG_UINT32 *pui32NumSyncs,
						 struct sync_fence *apsFence[SGX_MAX_SRC_SYNCS_TA])
{
	PVRSRV_KERNEL_SYNC_INFO *apsSyncInfo[SGX_MAX_SRC_SYNCS_TA];
	IMG_UINT32 i, ui32NumRealSrcSyncs;
	PVRSRV_ERROR eError = PVRSRV_OK;

	memset(apsFence, 0, sizeof(struct sync_fence *) * SGX_MAX_SRC_SYNCS_TA);

	if(!ExpandAndDeDuplicateFenceSyncs(*pui32NumSyncs,
									   (int *)apsSyncs,
									   *pui32NumSyncs,
									   apsFence,
									   &ui32NumRealSrcSyncs,
									   apsSyncInfo))
	{
		for(i = 0; i < SGX_MAX_SRC_SYNCS_TA && apsFence[i]; i++)
			sync_fence_put(apsFence[i]);
		return PVRSRV_ERROR_HANDLE_NOT_FOUND;
	}

	/* We don't expect to see merged syncs here. Abort if that happens.
	 * Allow through cases where the same fence was specified more than
	 * once -- we can handle that without reallocation of memory.
	 */
	PVR_ASSERT(ui32NumRealSrcSyncs <= *pui32NumSyncs);

	for(i = 0; i < ui32NumRealSrcSyncs; i++)
		apsSyncs[i] = apsSyncInfo[i];

	*pui32NumSyncs = ui32NumRealSrcSyncs;
	return eError;
}
