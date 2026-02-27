/*
 * @File
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/kernel.h>
#include <linux/spinlock_types.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/bug.h>

#include "pvr_export_fence.h"
#include "osfunc_common.h"

struct pvr_exp_fence_context {
	struct kref kref;
	unsigned int context;
	char context_name[32];
	char driver_name[32];
	atomic_t seqno;
	atomic_t fence_count;

	void *cmd_complete_handle;

	/* lock for signal and fence lists */
	spinlock_t list_lock;
	struct list_head signal_list;
	struct list_head fence_list;
};

struct pvr_exp_fence {
	struct dma_fence base;
	struct pvr_exp_fence_context *fence_context;
	PSYNC_CHECKPOINT checkpoint_handle;
	/* dma_fence fd (needed for hwperf) */
	int fd;
	/* Lock for the dma fence */
	spinlock_t lock;
	/* fence will point to the dma_fence in base */
	struct dma_fence *fence;
	struct list_head fence_head;
	struct list_head signal_head;
};

#define PVR_DUMPDEBUG_LOG(pfnDumpDebugPrintf, pvDumpDebugFile, fmt, ...) \
	do {                                                             \
		if (pfnDumpDebugPrintf)                                  \
			pfnDumpDebugPrintf(pvDumpDebugFile, fmt,         \
					   ## __VA_ARGS__);              \
		else                                                     \
			pr_err(fmt "\n", ## __VA_ARGS__);                \
	} while (0)

static inline bool
pvr_exp_fence_sync_is_signaled(struct pvr_exp_fence *exp_fence, u32 fence_sync_flags)
{
	if (exp_fence->checkpoint_handle) {
		return SyncCheckpointIsSignalled(exp_fence->checkpoint_handle,
						 fence_sync_flags);
	}
	return false;
}

const char *pvr_exp_fence_context_name(struct pvr_exp_fence_context *fctx)
{
	return fctx->context_name;
}

void pvr_exp_fence_context_value_str(struct pvr_exp_fence_context *fctx,
				    char *str, int size)
{
	snprintf(str, size, "%d", atomic_read(&fctx->seqno));
}

static void
pvr_exp_fence_context_fences_dump(struct pvr_exp_fence_context *fctx,
				  DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
				  void *pvDumpDebugFile)
{
	unsigned long flags;
	char value[128];

	spin_lock_irqsave(&fctx->list_lock, flags);
	pvr_exp_fence_context_value_str(fctx, value, sizeof(value));
	PVR_DUMPDEBUG_LOG(pfnDumpDebugPrintf, pvDumpDebugFile,
			 "exp_fence_ctx: @%s", value);
	spin_unlock_irqrestore(&fctx->list_lock, flags);
}

static inline unsigned
pvr_exp_fence_context_seqno_next(struct pvr_exp_fence_context *fence_context)
{
	if (fence_context)
		return atomic_inc_return(&fence_context->seqno) - 1;
	else
		return 0xfeedface;
}

static void
pvr_exp_fence_context_signal_fences(void *data)
{
	struct pvr_exp_fence_context *fctx = (struct pvr_exp_fence_context *)data;
	struct pvr_exp_fence *pvr_exp_fence, *tmp;
	unsigned long flags1;
	int chkpt_ct = 0;
	int chkpt_sig_ct = 0;

	LIST_HEAD(signal_list);

	/*
	 * We can't call fence_signal while holding the lock as we can end up
	 * in a situation whereby pvr_fence_foreign_signal_sync, which also
	 * takes the list lock, ends up being called as a result of the
	 * fence_signal below, i.e. fence_signal(fence) -> fence->callback()
	 *  -> fence_signal(foreign_fence) -> foreign_fence->callback() where
	 * the foreign_fence callback is pvr_fence_foreign_signal_sync.
	 *
	 * So extract the items we intend to signal and add them to their own
	 * queue.
	 */
	spin_lock_irqsave(&fctx->list_lock, flags1);
	list_for_each_entry_safe(pvr_exp_fence, tmp, &fctx->signal_list, signal_head) {
		chkpt_ct++;
		if (pvr_exp_fence_sync_is_signaled(pvr_exp_fence, PVRSRV_FENCE_FLAG_SUPPRESS_HWP_PKT)) {
			chkpt_sig_ct++;
			list_move_tail(&pvr_exp_fence->signal_head, &signal_list);
		}
	}
	spin_unlock_irqrestore(&fctx->list_lock, flags1);

	list_for_each_entry_safe(pvr_exp_fence, tmp, &signal_list, signal_head) {
		spin_lock_irqsave(&pvr_exp_fence->fence_context->list_lock, flags1);
		list_del(&pvr_exp_fence->signal_head);
		spin_unlock_irqrestore(&pvr_exp_fence->fence_context->list_lock, flags1);
		dma_fence_signal(pvr_exp_fence->fence);
		dma_fence_put(pvr_exp_fence->fence);
	}
}

static const char *pvr_exp_fence_get_driver_name(struct dma_fence *fence)
{
	struct pvr_exp_fence *pvr_exp_fence = to_pvr_exp_fence(fence);

	if (pvr_exp_fence && pvr_exp_fence->fence_context)
		return pvr_exp_fence->fence_context->driver_name;
	else
		return "***NO_DRIVER***";
}

static const char *pvr_exp_fence_get_timeline_name(struct dma_fence *fence)
{
	struct pvr_exp_fence *pvr_exp_fence = to_pvr_exp_fence(fence);

	if (pvr_exp_fence && pvr_exp_fence->fence_context)
		return pvr_exp_fence_context_name(pvr_exp_fence->fence_context);
	else
		return "***NO_TIMELINE***";
}

static void pvr_exp_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	snprintf(str, size, "%llu", (u64) fence->seqno);
}

static void pvr_exp_fence_timeline_value_str(struct dma_fence *fence,
					    char *str, int size)
{
	struct pvr_exp_fence *pvr_exp_fence = to_pvr_exp_fence(fence);

	if (pvr_exp_fence && pvr_exp_fence->fence_context)
		pvr_exp_fence_context_value_str(pvr_exp_fence->fence_context, str, size);
}

static bool pvr_exp_fence_enable_signaling(struct dma_fence *fence)
{
	struct pvr_exp_fence *exp_fence = to_pvr_exp_fence(fence);
	unsigned long flags;

	if (!exp_fence)
		return false;

	if (pvr_exp_fence_sync_is_signaled(exp_fence, PVRSRV_FENCE_FLAG_SUPPRESS_HWP_PKT))
		return false;

	dma_fence_get(&exp_fence->base);

	spin_lock_irqsave(&exp_fence->fence_context->list_lock, flags);
	list_add_tail(&exp_fence->signal_head, &exp_fence->fence_context->signal_list);
	spin_unlock_irqrestore(&exp_fence->fence_context->list_lock, flags);

	return true;
}

static void pvr_exp_fence_context_destroy_kref(struct kref *kref)
{
	struct pvr_exp_fence_context *fence_context =
		container_of(kref, struct pvr_exp_fence_context, kref);
	unsigned int fence_count;

	if (WARN_ON(!list_empty_careful(&fence_context->fence_list)))
		pvr_exp_fence_context_fences_dump(fence_context, NULL, NULL);

	PVRSRVUnregisterCmdCompleteNotify(fence_context->cmd_complete_handle);

	fence_count = atomic_read(&fence_context->fence_count);
	if (WARN_ON(fence_count))
		pr_debug("%s context has %u fence(s) remaining\n",
			 fence_context->context_name, fence_count);

	kfree(fence_context);
}

static void pvr_exp_fence_release(struct dma_fence *fence)
{
	struct pvr_exp_fence *pvr_exp_fence = to_pvr_exp_fence(fence);
	unsigned long flags;

	if (pvr_exp_fence) {
		if (pvr_exp_fence->fence_context) {
			spin_lock_irqsave(&pvr_exp_fence->fence_context->list_lock, flags);
			list_del(&pvr_exp_fence->fence_head);
			atomic_dec(&pvr_exp_fence->fence_context->fence_count);
			spin_unlock_irqrestore(&pvr_exp_fence->fence_context->list_lock, flags);

			kref_put(&pvr_exp_fence->fence_context->kref,
				 pvr_exp_fence_context_destroy_kref);
		}

		if (pvr_exp_fence->checkpoint_handle) {
			SyncCheckpointFree(pvr_exp_fence->checkpoint_handle);
			pvr_exp_fence->checkpoint_handle = NULL;
		}

		kfree(pvr_exp_fence);
	}
}

static const struct dma_fence_ops pvr_exp_fence_ops = {
	.get_driver_name = pvr_exp_fence_get_driver_name,
	.get_timeline_name = pvr_exp_fence_get_timeline_name,
	.fence_value_str = pvr_exp_fence_value_str,
	.timeline_value_str = pvr_exp_fence_timeline_value_str,
	.enable_signaling = pvr_exp_fence_enable_signaling,
	.wait = dma_fence_default_wait,
	.release = pvr_exp_fence_release,
};

struct pvr_exp_fence_context *
pvr_exp_fence_context_create(const char *context_name, const char *driver_name)
{
	struct pvr_exp_fence_context *fence_context;
	PVRSRV_ERROR srv_err;

	fence_context = kzalloc(sizeof(*fence_context), GFP_KERNEL);
	if (!fence_context)
		return NULL;

	fence_context->context = dma_fence_context_alloc(1);
	OSStringSafeCopy(fence_context->context_name, context_name,
		sizeof(fence_context->context_name));
	OSStringSafeCopy(fence_context->driver_name, driver_name,
		sizeof(fence_context->driver_name));
	atomic_set(&fence_context->seqno, 0);
	atomic_set(&fence_context->fence_count, 0);
	kref_init(&fence_context->kref);

	spin_lock_init(&fence_context->list_lock);
	INIT_LIST_HEAD(&fence_context->signal_list);
	INIT_LIST_HEAD(&fence_context->fence_list);

	srv_err = PVRSRVRegisterCmdCompleteNotify(&fence_context->cmd_complete_handle,
				pvr_exp_fence_context_signal_fences,
				fence_context);
	if (srv_err != PVRSRV_OK) {
		pr_err("%s: failed to register command complete callback (%s)\n",
		       __func__, PVRSRVGetErrorString(srv_err));
		kfree(fence_context);
		return NULL;
	}

	return fence_context;
}

void pvr_exp_fence_context_destroy(struct pvr_exp_fence_context *fence_context)
{
	if (fence_context) {
		kref_put(&fence_context->kref, pvr_exp_fence_context_destroy_kref);
	}
}

struct dma_fence *
pvr_exp_fence_create(struct pvr_exp_fence_context *fence_context, int fd, u64 *sync_pt_idx)
{
	struct pvr_exp_fence *pvr_exp_fence;
	unsigned long flags;
	unsigned int seqno;
	struct pvr_exp_fence_context *pfence_context = fence_context;

	if (WARN_ON(!fence_context))
		return NULL;

	pvr_exp_fence = kzalloc(sizeof(*pvr_exp_fence), GFP_KERNEL);
	if (WARN_ON(!pvr_exp_fence))
		return NULL;

	spin_lock_init(&pvr_exp_fence->lock);

	INIT_LIST_HEAD(&pvr_exp_fence->fence_head);
	INIT_LIST_HEAD(&pvr_exp_fence->signal_head);

	pvr_exp_fence->fence_context = pfence_context;

	/* No sync checkpoint is assigned until attached to a kick */
	pvr_exp_fence->checkpoint_handle = NULL;

	seqno = pvr_exp_fence_context_seqno_next(pfence_context);

	pvr_exp_fence->fence = &pvr_exp_fence->base;

	dma_fence_init(&pvr_exp_fence->base, &pvr_exp_fence_ops,
		       &pvr_exp_fence->lock, pfence_context->context, seqno);

	atomic_inc(&pfence_context->fence_count);

	spin_lock_irqsave(&fence_context->list_lock, flags);
	list_add_tail(&pvr_exp_fence->fence_head, &fence_context->fence_list);
	spin_unlock_irqrestore(&fence_context->list_lock, flags);

	kref_get(&pfence_context->kref);

	pvr_exp_fence->fd = fd;

	*sync_pt_idx = pvr_exp_fence_context_seqno_next(pfence_context);

	return &pvr_exp_fence->base;
}

enum PVRSRV_ERROR_TAG
pvr_exp_fence_assign_checkpoint(PVRSRV_FENCE fence_to_resolve,
				struct dma_fence *fence,
				PSYNC_CHECKPOINT_CONTEXT checkpoint_context,
				PSYNC_CHECKPOINT *assigned_checkpoint)
{
	struct SYNC_CHECKPOINT_TAG *new_sync_checkpoint;
	struct pvr_exp_fence *pvr_exp_fence;
	PVRSRV_FENCE export_fence_fd = fence_to_resolve;
	PVRSRV_ERROR err;

	pvr_exp_fence = to_pvr_exp_fence(fence);
	if (!pvr_exp_fence)	{
		pr_err("%s: Invalid fence_to_resolve\n", __func__);
		err = PVRSRV_ERROR_INVALID_PARAMS;
		goto err_out;
	}

	if (pvr_exp_fence->checkpoint_handle) {
		/* export fence already has a sync checkpoint assigned */
		*assigned_checkpoint = pvr_exp_fence->checkpoint_handle;
		return PVRSRV_OK;
	}

	/* Ensure fd assigned to export fence when it was created is passed
	 * to SyncCheckpointAlloc() (so correct fd is used in HWPerf event)
	 * as the export fence may only be part of the check fence.
	 */
	if (pvr_exp_fence->fd != PVRSRV_NO_FENCE) {
		export_fence_fd = pvr_exp_fence->fd;
	}

	err = SyncCheckpointAlloc(checkpoint_context,
				  PVRSRV_NO_TIMELINE, export_fence_fd,
				  fence->ops->get_timeline_name(fence), &new_sync_checkpoint);
	if (unlikely(err != PVRSRV_OK)) {
		pr_err("%s: SyncCheckpointAlloc() failed (err%d)\n",
		       __func__, err);
		*assigned_checkpoint = NULL;
		goto err_out;
	}

	pvr_exp_fence->checkpoint_handle = new_sync_checkpoint;
	*assigned_checkpoint = new_sync_checkpoint;
	return PVRSRV_OK;

err_out:
	return err;
}

enum PVRSRV_ERROR_TAG
pvr_exp_fence_rollback(struct dma_fence *fence)
{
	struct pvr_exp_fence *pvr_exp_fence;
	PVRSRV_ERROR err;

	pvr_exp_fence = to_pvr_exp_fence(fence);
	if (!pvr_exp_fence) {
		pr_err("%s: Invalid fence\n", __func__);
		err = PVRSRV_ERROR_INVALID_PARAMS;
		goto err_out;
	}

	if (pvr_exp_fence->checkpoint_handle) {
		/* Free the assigned sync checkpoint */
		SyncCheckpointFree(pvr_exp_fence->checkpoint_handle);
		pvr_exp_fence->checkpoint_handle = NULL;
	}

	return PVRSRV_OK;

err_out:
	return err;
}

bool pvr_is_exp_fence(struct dma_fence *fence)
{
	return (fence->ops == &pvr_exp_fence_ops);
}

struct pvr_exp_fence *to_pvr_exp_fence(struct dma_fence *fence)
{
	if (pvr_is_exp_fence(fence))
		return container_of(fence, struct pvr_exp_fence, base);

	return NULL;
}

struct SYNC_CHECKPOINT_TAG *
pvr_exp_fence_get_checkpoint(struct pvr_exp_fence *export_fence)
{
	if (export_fence) {
		return export_fence->checkpoint_handle;
	}
	else
		return NULL;
}
