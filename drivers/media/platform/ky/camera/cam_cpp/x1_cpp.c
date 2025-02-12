// SPDX-License-Identifier: GPL-2.0
/*
 * x1_cpp.c - Driver for KY X1 Camera Post Process
 * lizhirong <zhirong.li@ky.com>
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#include <linux/completion.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/workqueue.h>
#include <linux/timekeeping.h>
#include <linux/pm_qos.h>
#include <media/v4l2-event.h>
#include <media/x1/x1_plat_cam.h>
#include <media/x1/x1_cpp_uapi.h>
#include "cam_dbg.h"
//#include "cpp_compat_ioctl32.h"
#include "cpp_dmabuf.h"
#include "cpp_iommu.h"
#include "x1_cpp.h"

#ifdef CONFIG_ARCH_KY
//#include <soc/spm/plat.h>
#endif

#undef CAM_MODULE_TAG
#define CAM_MODULE_TAG CAM_MDL_CPP

#define CPP_DRV_NAME "mars-cpp"

//#define CPP_FNC_DEFAULT_FREQ (416000000)
#define CPP_FNC_DEFAULT_FREQ (307200000)
#define ISP_BUS_DEFAULT_FREQ (307200000)

#ifdef CONFIG_KY_FPGA
#define CPP_FRMCMD_TIMEOUT_MS (800)
#else
#define CPP_FRMCMD_TIMEOUT_MS (300)
#endif

#ifdef CONFIG_KY_DEBUG
struct dev_running_info {
	bool b_dev_running;
	bool (*is_dev_running)(struct dev_running_info *p_devinfo);
	struct notifier_block nb;
} cpp_running_info;

static bool check_dev_running_status(struct dev_running_info *p_devinfo)
{
	return p_devinfo->b_dev_running;
}

#define to_devinfo(_nb) container_of(_nb, struct dev_running_info, nb)

static int dev_clkoffdet_notifier_handler(struct notifier_block *nb,
					  unsigned long msg, void *data)
{
	struct clk_notifier_data *cnd = data;
	struct dev_running_info *p_devinfo = to_devinfo(nb);

	if ((__clk_is_enabled(cnd->clk)) && (msg & PRE_RATE_CHANGE) &&
	    (cnd->new_rate == 0) && (cnd->old_rate != 0)) {
		if (p_devinfo->is_dev_running(p_devinfo))
			return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}
#endif

//#if IS_ENABLED(CONFIG_KY_DDR_FC) && defined(CONFIG_PM)
#if 0				// FIXME
static struct spm_bw_con *ddr_qos_cons;
static int cpp_init_bandwidth(void)
{
	ddr_qos_cons =
	    register_spm_ddr_bw_cons(CPP_DRV_NAME,
				     PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE,
				     PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);

	if (IS_ERR_OR_NULL(ddr_qos_cons)) {
		cam_err("freq qos regsiter failed\n");
		return -EINVAL;
	}
	return 0;
}

static int cpp_deinit_bandwidth(void)
{
	rm_spm_ddr_bw_cons(ddr_qos_cons);
	return 0;
}

static int cpp_update_bandwidth(int32_t rsum, int32_t wsum)
{
	update_spm_ddr_bw_read_req(ddr_qos_cons, rsum);
	update_spm_ddr_bw_write_req(ddr_qos_cons, wsum);

	return 0;
}
#else
static int cpp_init_bandwidth(void)
{
	return 0;
}

static int cpp_deinit_bandwidth(void)
{
	return 0;
}

static int cpp_update_bandwidth(s32 rsum, s32 wsum)
{
	return 0;
}
#endif

static int cpp_hw_reg_config(struct cpp_device *cpp_dev, void *arg)
{
	struct x1_cpp_reg_cfg *reg_cfg = arg;

	/* validate argument */
	if (reg_cfg->u.rw_info.reg_offset > resource_size(cpp_dev->mem)) {
		cam_err("%s: reg offset 0x%08x res len 0x%llx", __func__,
			reg_cfg->u.rw_info.reg_offset, resource_size(cpp_dev->mem));
		return -EINVAL;
	}

	switch (reg_cfg->cmd_type) {
	case CPP_WRITE32:
		cpp_reg_write_mask(cpp_dev, reg_cfg->u.rw_info.reg_offset,
				   reg_cfg->u.rw_info.val, reg_cfg->u.rw_info.mask);
		break;
	case CPP_READ32:
		reg_cfg->u.rw_info.val =
		    cpp_reg_read(cpp_dev, reg_cfg->u.rw_info.reg_offset);
		break;
	default:
		break;
	}

	return 0;
}

static void cmd_queue_init(struct device_queue *queue, const char *name)
{
	unsigned long flags;

	cam_dbg("%s E", __func__);

	spin_lock_init(&queue->lock);
	spin_lock_irqsave(&queue->lock, flags);
	INIT_LIST_HEAD(&queue->list);
	queue->len = 0;
	queue->max = 0;
	queue->name = name;
	spin_unlock_irqrestore(&queue->lock, flags);
}

static void cmd_enqueue(struct device_queue *queue, struct list_head *entry)
{
	unsigned long flags;

	spin_lock_irqsave(&queue->lock, flags);
	queue->len++;
	if (queue->len > queue->max) {
		queue->max = queue->len;
		cam_dbg("%s new max is %d", queue->name, queue->max);
	}
	list_add_tail(entry, &queue->list);
	spin_unlock_irqrestore(&queue->lock, flags);
}

static struct cpp_queue_cmd *cmd_dequeue(struct device_queue *queue)
{
	unsigned long flags;
	struct cpp_queue_cmd *qcmd;

	spin_lock_irqsave(&queue->lock, flags);
	qcmd = list_first_entry_or_null(&queue->list, struct cpp_queue_cmd, list_frame);
	if (!qcmd) {
		spin_unlock_irqrestore(&queue->lock, flags);
		return NULL;
	}
	list_del(&qcmd->list_frame);
	queue->len--;
	spin_unlock_irqrestore(&queue->lock, flags);

	return qcmd;
}

static struct cpp_queue_cmd *queue_cmd_alloc(void)
{
	struct cpp_queue_cmd *qcmd;
	int i;

	qcmd = kzalloc(sizeof(struct cpp_queue_cmd), GFP_KERNEL);
	if (!qcmd) {
		cam_err("failed to allocate memory for cpp_queue_cmd");
		goto err_qcmd_alloc;
	}

	for (i = 0; i < MAX_REG_CMDS; i++) {
		qcmd->hw_cmds[i].reg_data =
		    kzalloc(sizeof(struct reg_val_mask_info) * MAX_REG_DATA,
			    GFP_KERNEL);
		if (!qcmd->hw_cmds[i].reg_data) {
			cam_err("failed to allocate memory for reg cmd %d", i);
			goto err_cmds_alloc;
		}
	}

	return qcmd;

err_cmds_alloc:
	for (i = 0; i < MAX_REG_CMDS; i++)
		kfree(qcmd->hw_cmds[i].reg_data);

	kfree(qcmd);

err_qcmd_alloc:
	return NULL;
}

static int queue_cmd_free(struct cpp_queue_cmd *qcmd)
{
	int i;

	if (!qcmd) {
		cam_err("queue cmd is NULL");
		return -EINVAL;
	}

	for (i = 0; i < MAX_REG_CMDS; i++)
		kfree(qcmd->hw_cmds[i].reg_data);

	kfree(qcmd);

	return 0;
}

static void cmd_queue_empty(struct device_queue *queue)
{
	unsigned long flags;
	struct cpp_queue_cmd *qcmd = NULL;

	if (queue) {
		cam_dbg("%s len %d, is empty", queue->name, queue->len);

		spin_lock_irqsave(&queue->lock, flags);
		while (!list_empty(&queue->list)) {
			queue->len--;
			qcmd =
			    list_first_entry(&queue->list, struct cpp_queue_cmd,
					     list_frame);
			list_del_init(&qcmd->list_frame);
			queue_cmd_free(qcmd);	/* release frame qcmd */
			qcmd = NULL;
		}
		queue->len = 0;
		queue->max = 0;
		spin_unlock_irqrestore(&queue->lock, flags);
	}
}

static int cmd_queue_request(struct device_queue *queue, int len)
{
	int i;
	static struct cpp_queue_cmd *qcmd;

	if (!queue) {
		cam_err("device queue is NULL");
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		qcmd = queue_cmd_alloc();
		if (qcmd)
			cmd_enqueue(queue, &qcmd->list_frame);
		else
			goto err_queue_alloc;
	}

	return 0;

err_queue_alloc:
	cmd_queue_empty(queue);

	return -ENOMEM;
}

/*
 * cpp_low_power_mode_set - enable auto clock gating to save power
 * @cpp_dev: cpp device
 * @en: switch on/off
 *
 * Return 0 on success or a negative error code otherwise
 */
static int cpp_low_power_mode_set(struct cpp_device *cpp_dev, int en)
{
	if (cpp_dev->hw_info.cpp_hw_version == CPP_HW_VERSION_1_0) {
		cam_err("CPP_HW_VERSION_1_0 not support low power mode");
		return -ENOTSUPP;
	}

	if (en) {
		cpp_dev->ops->enable_clk_gating(cpp_dev, 1);
		cpp_dev->hw_info.low_pwr_mode = 1;
	} else {
		cpp_dev->ops->enable_clk_gating(cpp_dev, 0);
		cpp_dev->hw_info.low_pwr_mode = 0;
	}

	return 0;
}

static int x1_cpp_send_reg_cmd(struct cpp_device *cpp_dev,
				struct cpp_reg_cfg_cmd *reg_cmd)
{
	int i;

	if (!cpp_dev || !reg_cmd) {
		cam_err("invalid args cpp_dev %p reg_cmd %p\n", cpp_dev, reg_cmd);
		return -EINVAL;
	}

	/* reg cmd skip */
	if (!reg_cmd->reg_data || !reg_cmd->reg_len)
		return 0;

	switch (reg_cmd->reg_type) {
	case CPP_WRITE32:
		for (i = 0; i < reg_cmd->reg_len; i++) {
			if (reg_cmd->reg_data[i].mask)	/* mask 0x0 skip */
				cpp_reg_write_mask(cpp_dev,
						   reg_cmd->reg_data[i].
						   reg_offset,
						   reg_cmd->reg_data[i].val,
						   reg_cmd->reg_data[i].mask);
		}
		break;
	case CPP_WRITE32_RLX:
		for (i = 0; i < reg_cmd->reg_len; i++) {
			cpp_reg_write_relaxed(cpp_dev,
					      reg_cmd->reg_data[i].reg_offset,
					      reg_cmd->reg_data[i].val);
		}
		break;
	case CPP_WRITE32_NOP:
		cam_dbg("cpp write32 %d nops", reg_cmd->reg_len);
		return 0;
	default:
		cam_err("invalid reg cmd type %d", reg_cmd->reg_type);
		return -EINVAL;
	}

	return 0;
}

static void cpp_send_frame_to_hardware(struct cpp_device *cpp_dev,
				       struct cpp_queue_cmd *qcmd)
{
	int i;

	if (atomic_read(&qcmd->in_processing)) {
		cam_err("frame_cmd has been processed");
		return;
	}

	atomic_set(&qcmd->in_processing, 1);
	qcmd->ts_reg_config = ktime_get_ns();

	for (i = 0; i < MAX_REG_CMDS; i++)
		x1_cpp_send_reg_cmd(cpp_dev, &qcmd->hw_cmds[i]);
	qcmd->ts_frm_trigger = ktime_get_ns();
}

static void cpp_device_run(struct cpp_device *cdev)
{
	struct cpp_run_work *run_work = &cdev->run_work;

	queue_work(run_work->run_wq, &run_work->work);
}

static void x1_cpp_try_run(struct cpp_device *cdev)
{
	unsigned long flags_job;

	spin_lock_irqsave(&cdev->job_spinlock, flags_job);
	if (NULL != cdev->curr_ctx) {
		spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
		cam_dbg("cpp_ctx: %p is running, won't run now", cdev->curr_ctx);
		return;
	}

	if (list_empty(&cdev->job_queue)) {
		spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
		cam_dbg("No job pending");
		return;
	}

	cdev->curr_ctx = list_first_entry(&cdev->job_queue, struct cpp_ctx, queue);
	cdev->curr_ctx->job_flags |= TRANS_RUNNING;
	spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);

	cam_dbg("Running job on cpp_ctx: %p", cdev->curr_ctx);
	cpp_device_run(cdev);
}

static void x1_cpp_try_queue(struct cpp_device *cdev, struct cpp_ctx *ctx)
{
	unsigned long flags_job, flags_frmq;

	spin_lock_irqsave(&cdev->job_spinlock, flags_job);

	if (ctx->job_flags & TRANS_ABORT) {
		spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
		cam_dbg("Abort context");
		return;
	}

	if (ctx->job_flags & TRANS_QUEUED) {
		spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
		cam_dbg("On job queue already");
		return;
	}

	spin_lock_irqsave(&ctx->frmq.lock, flags_frmq);
	if (list_empty(&ctx->frmq.list)) {
		spin_unlock_irqrestore(&ctx->frmq.lock, flags_frmq);
		spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
		cam_dbg("no frame cmd available");
		return;
	}
	spin_unlock_irqrestore(&ctx->frmq.lock, flags_frmq);

	list_add_tail(&ctx->queue, &cdev->job_queue);
	ctx->job_flags |= TRANS_QUEUED;

	spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
}

static void x1_cpp_try_schedule(struct cpp_ctx *ctx)
{
	struct cpp_device *cdev = ctx->cpp_dev;

	x1_cpp_try_queue(cdev, ctx);
	x1_cpp_try_run(cdev);
}

static void x1_cpp_job_finish(struct cpp_device *cdev, struct cpp_ctx *ctx)
{
	unsigned long flags_job;

	spin_lock_irqsave(&cdev->job_spinlock, flags_job);
	if (!cdev->curr_ctx || cdev->curr_ctx != ctx) {
		spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
		cam_err("Called by an instance not currently running\n");
		return;
	}

	list_del(&cdev->curr_ctx->queue);
	cdev->curr_ctx->job_flags &= ~(TRANS_QUEUED | TRANS_RUNNING);
	cdev->curr_ctx = NULL;

	spin_unlock_irqrestore(&cdev->job_spinlock, flags_job);
}

static int x1_cpp_get_frame(struct cpp_device *cdev,
			     struct cpp_queue_cmd *qcmd,
			     struct cpp_frame_info *frame_info)
{
	int ret, i;

	for (i = 0; i < MAX_REG_CMDS; i++) {
		if (frame_info->regs[i].reg_len > MAX_REG_DATA) {
			cam_err("insufficient to copy reg cmd %d with %d entries",
				i, frame_info->regs[i].reg_len);
			return -EINVAL;
		}

		if (frame_info->regs[i].reg_len) {
			if (copy_from_user(qcmd->hw_cmds[i].reg_data,
					   (void __user *)frame_info->regs[i].reg_data,
					   sizeof(struct reg_val_mask_info) *
					   frame_info->regs[i].reg_len)) {
				cam_err("failed to copy reg cmd %d from user", i);
				return -EFAULT;
			}
		}
		qcmd->hw_cmds[i].reg_len = frame_info->regs[i].reg_len;
		qcmd->hw_cmds[i].reg_type = frame_info->regs[i].reg_type;
	}

	qcmd->dma_ports[MAC_DMA_PORT_R0] =
	    cpp_dmabuf_prepare(cdev, &frame_info->src_buf_info, MAC_DMA_PORT_R0);
	if (IS_ERR_OR_NULL(qcmd->dma_ports[MAC_DMA_PORT_R0])) {
		cam_err("failed to prepare dmabuf R0");
		return -EINVAL;
	}

	qcmd->dma_ports[MAC_DMA_PORT_R1] =
	    cpp_dmabuf_prepare(cdev, &frame_info->pre_buf_info, MAC_DMA_PORT_R1);
	if (IS_ERR_OR_NULL(qcmd->dma_ports[MAC_DMA_PORT_R1])) {
		cam_err("failed to prepare dmabuf R1");
		ret = -EINVAL;
		goto err_r1;
	}

	qcmd->dma_ports[MAC_DMA_PORT_W0] =
	    cpp_dmabuf_prepare(cdev, &frame_info->dst_buf_info, MAC_DMA_PORT_W0);
	if (IS_ERR_OR_NULL(qcmd->dma_ports[MAC_DMA_PORT_W0])) {
		cam_err("failed to prepare dmabuf W0");
		ret = -EINVAL;
		goto err_r2;
	}

	qcmd->frame_id = frame_info->frame_id;
	qcmd->client_id = frame_info->client_id;

	return 0;

err_r2:
	cpp_dmabuf_cleanup(cdev, qcmd->dma_ports[MAC_DMA_PORT_R1]);
err_r1:
	cpp_dmabuf_cleanup(cdev, qcmd->dma_ports[MAC_DMA_PORT_R0]);

	return ret;
}

static int x1_cpp_process_frame(struct cpp_ctx *ctx, struct cpp_frame_info *info)
{
	int ret;
	struct cpp_queue_cmd *qcmd;

	qcmd = cmd_dequeue(&ctx->idleq);
	if (!qcmd) {
		cam_err("%s: %s is not enough", __func__, ctx->idleq.name);
		return -EAGAIN;
	}

	ret = x1_cpp_get_frame(ctx->cpp_dev, qcmd, info);
	if (ret) {
		cmd_enqueue(&ctx->idleq, &qcmd->list_frame);
	} else {
		cmd_enqueue(&ctx->frmq, &qcmd->list_frame);
		x1_cpp_try_schedule(ctx);
	}

	return ret;
}

int x1_cpp_send_event(struct cpp_device *cpp_dev, u32 event_type,
		       struct x1_cpp_event_data *event_data)
{
	struct v4l2_event cpp_event;

	memset(&cpp_event, 0, sizeof(struct v4l2_event));
	cpp_event.id = 0;
	cpp_event.type = event_type;
	memcpy(&cpp_event.u.data[0], event_data, sizeof(struct x1_cpp_event_data));
	v4l2_event_queue(cpp_dev->csd.sd.devnode, &cpp_event);

	return 0;
}

static void cpp_update_axi_cfg(struct cpp_device *cpp_dev,
			       struct cpp_dma_port_info **dma_info)
{
	cpp_dev->ops->cfg_port_dmad(cpp_dev, dma_info[MAC_DMA_PORT_R0],
				    MAC_DMA_PORT_R0);
	cpp_dev->ops->cfg_port_dmad(cpp_dev, dma_info[MAC_DMA_PORT_R1],
				    MAC_DMA_PORT_R1);
	cpp_dev->ops->cfg_port_dmad(cpp_dev, dma_info[MAC_DMA_PORT_W0],
				    MAC_DMA_PORT_W0);
}

static void x1_cpp_device_run_work(struct work_struct *work)
{
	struct cpp_run_work *run_work = container_of(work, struct cpp_run_work, work);
	struct cpp_device *cdev = container_of(run_work, struct cpp_device, run_work);
	struct cpp_ctx *curr_ctx = cdev->curr_ctx;
	struct cpp_queue_cmd *frm_cmd = NULL;
	u32 evt_type;
	struct x1_cpp_event_data data;

	int ret, port_id;
	int iommu_state = cdev->mmu_dev->state;

	if (!curr_ctx) {
		cam_err("current ctx is Null when device running");
		return;
	}

	frm_cmd = cmd_dequeue(&curr_ctx->frmq);
	if (!frm_cmd) {
		cam_err("cpp_ctx: %p %s is Null when device running", curr_ctx,
			curr_ctx->frmq.name);
		goto exit;
	}

	if (iommu_state == CPP_IOMMU_ATTACHED) {
		for (port_id = 0; port_id < MAX_DMA_PORT; ++port_id) {
			ret = cpp_dma_alloc_iommu_channels(cdev,
							   frm_cmd->dma_ports[port_id]);
			if (ret) {
				pr_err
				    ("%s: dma port%d failed to alloc iommu channels\n",
				     __func__, port_id);
				goto done;
			}
			cpp_dma_fill_iommu_channels(cdev, frm_cmd->dma_ports[port_id]);
		}
	}

	cpp_update_axi_cfg(cdev, frm_cmd->dma_ports);

	reinit_completion(&run_work->run_complete);
	cpp_send_frame_to_hardware(cdev, frm_cmd);

	if (!wait_for_completion_timeout(&run_work->run_complete,
					 msecs_to_jiffies(CPP_FRMCMD_TIMEOUT_MS))) {
		evt_type = V4L2_EVENT_CPP_FRAME_ERR;
		data.u.err_info.err_type = 0;
		data.u.err_info.frame_id = frm_cmd->frame_id;
		data.u.err_info.client_id = frm_cmd->client_id;
		cam_err("c%dframe%d run timeout", frm_cmd->client_id,
			frm_cmd->frame_id);
		cdev->ops->debug_dump(cdev);
	} else {
		if (cdev->state == CPP_STATE_ERR) {
			evt_type = V4L2_EVENT_CPP_FRAME_ERR;
			data.u.err_info.err_type = 1;
			data.u.err_info.frame_id = frm_cmd->frame_id;
			data.u.err_info.client_id = frm_cmd->client_id;
			cam_err("c%dframe%d run error", frm_cmd->client_id,
				frm_cmd->frame_id);
			cdev->ops->debug_dump(cdev);
		} else {
			frm_cmd->ts_frm_finish = ktime_get_ns();

			evt_type = V4L2_EVENT_CPP_FRAME_DONE;
			data.u.done_info.success = 1;
			data.u.done_info.frame_id = frm_cmd->frame_id;
			data.u.done_info.client_id = frm_cmd->client_id;
			data.u.done_info.seg_reg_cfg = frm_cmd->ts_frm_trigger -
			    frm_cmd->ts_reg_config;
			data.u.done_info.seg_stream = frm_cmd->ts_frm_finish -
			    frm_cmd->ts_frm_trigger;
			cam_dbg("c%dframe%d run finish", frm_cmd->client_id,
				frm_cmd->frame_id);
		}
	}

done:
	if (iommu_state == CPP_IOMMU_ATTACHED) {
		for (port_id = 0; port_id < MAX_DMA_PORT; ++port_id)
			cpp_dma_free_iommu_channels(cdev, frm_cmd->dma_ports[port_id]);
	}

	cpp_dmabuf_cleanup(cdev, frm_cmd->dma_ports[MAC_DMA_PORT_W0]);
	cpp_dmabuf_cleanup(cdev, frm_cmd->dma_ports[MAC_DMA_PORT_R1]);
	cpp_dmabuf_cleanup(cdev, frm_cmd->dma_ports[MAC_DMA_PORT_R0]);
	x1_cpp_send_event(cdev, evt_type, &data);

	atomic_set(&frm_cmd->in_processing, 0);
	cmd_enqueue(&curr_ctx->idleq, &frm_cmd->list_frame);

exit:
	x1_cpp_job_finish(cdev, curr_ctx);

	/* process next frame */
	x1_cpp_try_schedule(curr_ctx);
}

static int cpp_setup_run_work(struct cpp_device *cdev)
{
	struct cpp_run_work *run_work = &cdev->run_work;

	cam_dbg("Installing cpp run work");

	run_work->run_wq = create_singlethread_workqueue(CPP_DRV_NAME);
	if (!run_work->run_wq) {
		cam_err("Can't create %s run wq", CPP_DRV_NAME);
		return -ENOMEM;
	}
	INIT_WORK(&run_work->work, x1_cpp_device_run_work);
	init_completion(&run_work->run_complete);

	return 0;
}

static void cpp_cancel_run_work(struct cpp_device *cdev)
{
	struct cpp_ctx *ctx = &cdev->priv;
	struct cpp_run_work *run_work = &cdev->run_work;
	unsigned long flags;

	cam_dbg("Canceling cpp run work");

	spin_lock_irqsave(&cdev->job_spinlock, flags);

	ctx->job_flags |= TRANS_ABORT;
	if (ctx->job_flags & TRANS_RUNNING) {
		spin_unlock_irqrestore(&cdev->job_spinlock, flags);
		cam_dbg("cpp_ctx %p running, will wait to complete", ctx);
		if (run_work->run_wq)
			flush_workqueue(run_work->run_wq);
	} else if (ctx->job_flags & TRANS_QUEUED) {
		list_del(&ctx->queue);
		ctx->job_flags &= ~(TRANS_QUEUED | TRANS_RUNNING);
		spin_unlock_irqrestore(&cdev->job_spinlock, flags);
	} else {
		/* Do nothing, was not on queue/running */
		spin_unlock_irqrestore(&cdev->job_spinlock, flags);
	}
}

static void cpp_destroy_run_work(struct cpp_device *cdev)
{
	struct cpp_run_work *run_work = &cdev->run_work;

	cam_dbg("Destroying cpp run work");

	if (run_work->run_wq) {
		flush_workqueue(run_work->run_wq);
		destroy_workqueue(run_work->run_wq);
	}
}

static int cpp_update_clock_rate(struct cpp_device *cpp_dev,
				 unsigned long func_rate, unsigned long bus_rate);

static long x1_cpp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct cpp_device *cpp_dev;
	int ret = 0;

	cpp_dev = v4l2_get_subdevdata(sd);
	if (!cpp_dev) {
		cam_err("cpp_dev is null");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&cpp_dev->mutex))
		return -ERESTARTSYS;

	switch (cmd) {
	case VIDIOC_X1_CPP_HW_INFO: {
		struct cpp_hw_info *hw_info = arg;

		cam_dbg("VIDIOC_X1_CPP_HW_INFO");
		memset(hw_info, 0, sizeof(*hw_info));
		hw_info->cpp_hw_version = cpp_dev->hw_info.cpp_hw_version;
		hw_info->low_pwr_mode = cpp_dev->hw_info.low_pwr_mode;
		break;
	}
	case VIDIOC_X1_CPP_REG_CFG: {
		cam_dbg("VIDIOC_X1_CPP_REG_CFG");
		if (cpp_dev->state != CPP_STATE_IDLE) {
			cam_err("check cpp state %d when reg cfg",
				cpp_dev->state);
			return -EIO;
		}
		ret = cpp_hw_reg_config(cpp_dev, arg);
		break;
	}
	case VIDIOC_X1_CPP_HW_RST: {
		cam_dbg("VIDIOC_X1_CPP_HW_RST");
		if (cpp_dev->state == CPP_STATE_OFF) {
			cam_err("check cpp state %d when hw reset",
				cpp_dev->state);
			return -EIO;
		}

		cam_dbg("cpp state %d when hw reset", cpp_dev->state);
		ret = cpp_dev->ops->global_reset(cpp_dev);
		/* recover submodule registers */
		cpp_dev->ops->enable_irqs_common(cpp_dev, 1);
		cpp_dev->ops->set_burst_len(cpp_dev);
		cpp_dev->state = CPP_STATE_IDLE;
		break;
	}
	case VIDIOC_X1_CPP_LOW_PWR: {
		cam_dbg("VIDIOC_X1_CPP_LOW_PWR");
		ret = cpp_low_power_mode_set(cpp_dev, *((int *)arg));
		break;
	}
	case VIDIOC_X1_CPP_PROCESS_FRAME: {
		struct cpp_frame_info *proc_info = arg;

		cam_dbg("VIDIOC_X1_CPP_PROCESS_FRAME");
		ret = x1_cpp_process_frame(&cpp_dev->priv, proc_info);
		break;
	}
	case VIDIOC_X1_CPP_FLUSH_QUEUE: {
		cam_dbg("VIDIOC_X1_CPP_FLUSH_QUEUE");
		if (cpp_dev->state == CPP_STATE_OFF) {
			cam_err("check cpp state %d when flush",
				cpp_dev->state);
			return -EIO;
		}
		flush_workqueue(cpp_dev->run_work.run_wq);
		break;
	}
	case VIDIOC_X1_CPP_IOMMU_ATTACH: {
		cam_dbg("VIDIOC_X1_CPP_IOMMU_ATTACH");

		cpp_dev->mmu_dev->state = CPP_IOMMU_ATTACHED;
		break;
	}
	case VIDIOC_X1_CPP_IOMMU_DETACH: {
		cam_dbg("VIDIOC_X1_CPP_IOMMU_DETACH");

		cpp_dev->mmu_dev->state = CPP_IOMMU_DETACHED;
		break;
	}
	case VIDIOC_X1_CPP_UPDATE_BANDWIDTH: {
		struct cpp_bandwidth_info *bw_info =
			(struct cpp_bandwidth_info *)arg;

		cam_dbg("VIDIOC_X1_CPP_UPDATE_BANDWIDTH");
		ret = cpp_update_bandwidth(bw_info->rsum, bw_info->wsum);
		break;
	}
	case VIDIOC_X1_CPP_UPDATE_CLOCKRATE: {
		struct cpp_clock_info *clk_info = (struct cpp_clock_info *)arg;

		cam_dbg("VIDIOC_X1_CPP_UPDATE_CLOCKRATE");
		ret = cpp_update_clock_rate(cpp_dev, clk_info->func_rate, -1);
		break;
	}
	default:
		ret = -ENOTTY;
		break;
	}
	mutex_unlock(&cpp_dev->mutex);

	return ret;
}

static int x1_cpp_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				   struct v4l2_event_subscription *sub)
{
	cam_dbg("%s E", __func__);
	return v4l2_event_subscribe(fh, sub, MAX_CPP_V4L2_EVENTS, NULL);
}

static int x1_cpp_unsubscribe_event(struct v4l2_subdev *sd,
				     struct v4l2_fh *fh,
				     struct v4l2_event_subscription *sub)
{
	cam_dbg("%s E", __func__);
	return v4l2_event_unsubscribe(fh, sub);
}

static struct v4l2_subdev_core_ops x1_cpp_subdev_core_ops = {
	.ioctl = x1_cpp_subdev_ioctl,
	.subscribe_event = x1_cpp_subscribe_event,
	.unsubscribe_event = x1_cpp_unsubscribe_event,
};

static struct v4l2_subdev_ops cpp_subdev_ops = {
	.core = &x1_cpp_subdev_core_ops,
};

static int cpp_update_clock_rate(struct cpp_device *cpp_dev,
				 unsigned long func_rate, unsigned long bus_rate)
{
	long clk_val;
	int ret;

	if (func_rate > 0) {
		clk_val = clk_round_rate(cpp_dev->fnc_clk, func_rate);
		if (clk_val < 0) {
			cam_err("fnc clk round rate failed: %ld", clk_val);
			return -EINVAL;
		}
		ret = clk_set_rate(cpp_dev->fnc_clk, clk_val);
		if (ret < 0) {
			cam_err("fnc clk set rate failed: %d", ret);
			return ret;
		}

	}

	if (bus_rate > 0) {
		clk_val = clk_round_rate(cpp_dev->bus_clk, bus_rate);
		if (clk_val < 0) {
			cam_err("bus clk round rate failed: %ld", clk_val);
			return -EINVAL;
		}
		ret = clk_set_rate(cpp_dev->bus_clk, clk_val);
		if (ret < 0) {
			cam_err("bus clk set rate failed: %d", ret);
			return ret;
		}
	}

	cam_dbg("func clock rate: %ld, bus clock rate: %ld",
		 clk_get_rate(cpp_dev->fnc_clk), clk_get_rate(cpp_dev->bus_clk));

	return 0;
}

static int cpp_enable_clocks(struct cpp_device *cpp_dev)
{
	int ret;

	reset_control_deassert(cpp_dev->ahb_reset);

//	ret = clk_prepare_enable(cpp_dev->ahb_clk);
//	if (ret)
//		return ret;

	ret = clk_prepare_enable(cpp_dev->fnc_clk);
	if (ret)
		goto err_clks_ahb;
	reset_control_deassert(cpp_dev->isp_cpp_reset);

	ret = clk_prepare_enable(cpp_dev->bus_clk);
	if (ret)
		goto err_clks_fnc;
	reset_control_deassert(cpp_dev->isp_ci_reset);

	ret = clk_prepare_enable(cpp_dev->dpu_clk);
	if (ret)
		goto err_clks_bus;
	reset_control_deassert(cpp_dev->lcd_mclk_reset);

	return 0;

err_clks_bus:
	reset_control_assert(cpp_dev->isp_ci_reset);
	clk_disable_unprepare(cpp_dev->bus_clk);
err_clks_fnc:
	reset_control_assert(cpp_dev->isp_cpp_reset);
	clk_disable_unprepare(cpp_dev->fnc_clk);
err_clks_ahb:
//	clk_disable_unprepare(cpp_dev->ahb_clk);
	reset_control_assert(cpp_dev->ahb_reset);

	return ret;
}

static void cpp_disable_clocks(struct cpp_device *cpp_dev)
{
	reset_control_assert(cpp_dev->isp_ci_reset);
	clk_disable_unprepare(cpp_dev->bus_clk);

	reset_control_assert(cpp_dev->isp_cpp_reset);
	clk_disable_unprepare(cpp_dev->fnc_clk);
//	clk_disable_unprepare(cpp_dev->ahb_clk);
	reset_control_assert(cpp_dev->ahb_reset);

	reset_control_assert(cpp_dev->lcd_mclk_reset);
	clk_disable_unprepare(cpp_dev->dpu_clk);
}

/**
 * cpp_init_hardware - pd, clock on
 *
 * @cpp_dev:
 *
 * Return: 0 on success, error code otherwise.
 */
static int cpp_init_hardware(struct cpp_device *cpp_dev)
{
	int ret;

	/* get runtime pm */
	ret = pm_runtime_get_sync(&cpp_dev->pdev->dev);
	if (ret < 0) {
		cam_err("rpm get failed: %d", ret);
		return ret;
	}


	ret = cpp_enable_clocks(cpp_dev);
	if (ret) {
		pm_runtime_put_sync(&cpp_dev->pdev->dev);
		return ret;
	}

	ret = cpp_update_clock_rate(cpp_dev, CPP_FNC_DEFAULT_FREQ,
				    ISP_BUS_DEFAULT_FREQ);
	if (ret) {
		pm_runtime_put_sync(&cpp_dev->pdev->dev);
		return ret;
	}

	/* Do HW Reset, checking cpp function properly */
	ret = cpp_dev->ops->global_reset(cpp_dev);
	if (ret) {
		cpp_disable_clocks(cpp_dev);
		pm_runtime_put_sync(&cpp_dev->pdev->dev);
		return ret;
	}

	cpp_dev->ops->enable_irqs_common(cpp_dev, 1);
	cpp_dev->ops->set_burst_len(cpp_dev);
	cpp_dev->hw_info.cpp_hw_version = cpp_dev->ops->hw_version(cpp_dev);
	cpp_dev->hw_info.low_pwr_mode = 1;

	return 0;
}

/**
 * cpp_release_hardware - pd, clock off
 *
 * @cpp_dev:
 *
 */
static void cpp_release_hardware(struct cpp_device *cpp_dev)
{
	/* reset bandwidth */
	cpp_update_bandwidth(0, 0);

	/* hang workaround */
	cpp_dev->ops->global_reset(cpp_dev);

	/* disable all irqs */
	cpp_dev->ops->enable_irqs_common(cpp_dev, 0);

	/* disable clock(s) */
	cpp_disable_clocks(cpp_dev);

	/* put runtime pm */
	pm_runtime_put_sync(&cpp_dev->pdev->dev);
}

static int cpp_open_node(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct cpp_device *cpp_dev;
	unsigned long flags;
	int ret;

	cam_dbg("%s E", __func__);

	cpp_dev = v4l2_get_subdevdata(sd);
	if (!cpp_dev) {
		cam_err("cpp_dev is null\n");
		return -EINVAL;
	}

	if (mutex_lock_interruptible(&cpp_dev->mutex))
		return -ERESTARTSYS;

	if (cpp_dev->open_cnt == MAX_ACTIVE_CPP_INSTANCE) {
		cam_err("no available cpp instance");
		mutex_unlock(&cpp_dev->mutex);
		return -ENODEV;
	}

	cpp_dev->open_cnt++;
	if (cpp_dev->open_cnt == 1) {
		ret = cpp_init_hardware(cpp_dev);
		if (ret < 0) {
			cam_err("cpp init hardware failed!");
			cpp_dev->open_cnt--;
			mutex_unlock(&cpp_dev->mutex);
			return ret;
		}
		cpp_dev->state = CPP_STATE_IDLE;

		ret = cmd_queue_request(&cpp_dev->priv.idleq, 6);
		if (ret) {
			cpp_release_hardware(cpp_dev);
			cpp_dev->open_cnt--;
			mutex_unlock(&cpp_dev->mutex);
			return ret;
		}

#ifdef CONFIG_KY_DEBUG
		cpp_running_info.b_dev_running = true;
#endif
		spin_lock_irqsave(&cpp_dev->job_spinlock, flags);
		cpp_dev->priv.job_flags = 0;	/* stream on */
		spin_unlock_irqrestore(&cpp_dev->job_spinlock, flags);
	}

	if (cpp_dev->mapped) {
		vfree(cpp_dev->shared_mem);
		cpp_dev->shared_mem = NULL;
		cpp_dev->shared_size = 0;
		cpp_dev->mapped = 0;
	}

	mutex_unlock(&cpp_dev->mutex);
	cam_dbg("%s X", __func__);

	return 0;
}

static int cpp_close_node(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct cpp_device *cpp_dev;
	int ret = 0;

	cam_dbg("%s E", __func__);

	cpp_dev = v4l2_get_subdevdata(sd);
	if (!cpp_dev) {
		pr_err("cpp_dev is null\n");
		return -EINVAL;
	}

	mutex_lock(&cpp_dev->mutex);
	if (cpp_dev->open_cnt == 0) {
		cam_err("no existing cpp instance");
		mutex_unlock(&cpp_dev->mutex);
		return -ENODEV;
	}

	cpp_dev->open_cnt--;
	if (cpp_dev->open_cnt == 0) {
#ifdef CONFIG_KY_DEBUG
		cpp_running_info.b_dev_running = false;
#endif
		cpp_cancel_run_work(cpp_dev);
		cmd_queue_empty(&cpp_dev->priv.frmq);
		cmd_queue_empty(&cpp_dev->priv.idleq);
		cpp_release_hardware(cpp_dev);
		cpp_dev->state = CPP_STATE_OFF;
		cpp_dev->mmu_dev->ops->dump_status(cpp_dev->mmu_dev);
		pm_relax(&cpp_dev->pdev->dev);
	}

	mutex_unlock(&cpp_dev->mutex);
	cam_dbg("%s X", __func__);

	return ret;
}

static int cpp_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *vdev = video_devdata(file);
	struct v4l2_subdev *sd = vdev_to_v4l2_subdev(vdev);
	struct cpp_device *cpp_dev = v4l2_get_subdevdata(sd);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return vm_iomap_memory(vma, cpp_dev->mem->start, resource_size(cpp_dev->mem));
}

static int cpp_subdev_registered(struct v4l2_subdev *sd)
{
	int ret = 0;
	struct v4l2_file_operations *fops;

	fops = kzalloc(sizeof(*fops), GFP_KERNEL);
	if (!fops)
		return -ENOMEM;

	*fops = *sd->devnode->fops;
	fops->mmap = cpp_mmap;
#ifdef CONFIG_COMPAT
	//fops->compat_ioctl32 = x1_cpp_compat_ioctl32;
#endif

	sd->devnode->fops = fops;

	return ret;
}

static void cpp_subdev_unregistered(struct v4l2_subdev *sd)
{
	const struct v4l2_file_operations *fops;

	fops = sd->devnode->fops;
	kfree(fops);
	sd->devnode->fops = NULL;
}

static const struct v4l2_subdev_internal_ops x1_cpp_internal_ops = {
	.open = cpp_open_node,
	.close = cpp_close_node,
};

static const struct spm_v4l2_subdev_ops cpp_spm_subdev_ops = {
	.registered = cpp_subdev_registered,
	.unregistered = cpp_subdev_unregistered,
};

static int cpp_init_subdev(struct cpp_device *cpp_dev)
{
	int ret = 0;

	cpp_dev->csd.internal_ops = &x1_cpp_internal_ops;
	cpp_dev->csd.ops = &cpp_subdev_ops;
	cpp_dev->csd.spm_ops = &cpp_spm_subdev_ops;
	cpp_dev->csd.name = CPP_DRV_NAME;
	cpp_dev->csd.sd_flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	cpp_dev->csd.sd_flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	cpp_dev->csd.ent_function = MEDIA_ENT_F_X1_CPP;
	cpp_dev->csd.pads_cnt = 0;
	cpp_dev->csd.token = cpp_dev;

	ret = plat_cam_register_subdev(&cpp_dev->csd);
	if (ret)
		pr_err("Fail to create platform camera subdev ");

	return ret;
}

static const struct of_device_id x1cpp_dt_match[] = {
	{
		.compatible = "ky,x1cpp",
		.data = &cpp_ops_2_0,
	},
	{}
};

MODULE_DEVICE_TABLE(of, x1cpp_dt_match);

static int cpp_probe(struct platform_device *pdev)
{
	struct cpp_device *cpp_dev;
	const struct of_device_id *match_dev;
	int ret = 0;
	int irq = 0;

	cam_dbg("enter cpp_probe\n");
	match_dev = of_match_device(x1cpp_dt_match, &pdev->dev);
	if (!match_dev || !match_dev->data) {
		dev_err(&pdev->dev, "no match data\n");
		return -EINVAL;
	}

	cpp_dev = devm_kzalloc(&pdev->dev, sizeof(struct cpp_device), GFP_KERNEL);
	if (!cpp_dev)
		return -ENOMEM;
	cpp_dev->ops = (struct cpp_hw_ops *)match_dev->data;

	/* get mem */
	cpp_dev->mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "cpp");
	if (!cpp_dev->mem) {
		dev_err(&pdev->dev, "no mem resource");
		return -ENODEV;
	}
	cpp_dev->regs_base = devm_ioremap_resource(&pdev->dev, cpp_dev->mem);
	if (IS_ERR(cpp_dev->regs_base)) {
		dev_err(&pdev->dev, "fail to remap iomem\n");
		return PTR_ERR(cpp_dev->regs_base);
	}

	irq = platform_get_irq_byname(pdev, "cpp");
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource");
		return -ENODEV;
	}
	dev_dbg(&pdev->dev, "cpp irq: %d\n", irq);
	ret = devm_request_irq(&pdev->dev, irq,
			       cpp_dev->ops->isr, 0, CPP_DRV_NAME, cpp_dev);
	if (ret) {
		dev_err(&pdev->dev, "fail to request irq\n");
		return ret;
	}

	/* get clock(s) */
#ifdef CONFIG_ARCH_KY
/*
	cpp_dev->ahb_clk = devm_clk_get(&pdev->dev, "isp_ahb");
	if (IS_ERR(cpp_dev->ahb_clk)) {
		ret = PTR_ERR(cpp_dev->ahb_clk);
		dev_err(&pdev->dev, "failed to get ahb clock: %d\n", ret);
		return ret;
	}
*/
	cpp_dev->ahb_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ahb_reset");
	if (IS_ERR_OR_NULL(cpp_dev->ahb_reset)) {
		dev_err(&pdev->dev, "not found core isp_ahb_reset\n");
		return PTR_ERR(cpp_dev->ahb_reset);
	}

	cpp_dev->isp_cpp_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_cpp_reset");
	if (IS_ERR_OR_NULL(cpp_dev->isp_cpp_reset)) {
		dev_err(&pdev->dev, "not found core isp_cpp_reset\n");
		return PTR_ERR(cpp_dev->isp_cpp_reset);
	}

	cpp_dev->isp_ci_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ci_reset");
	if (IS_ERR_OR_NULL(cpp_dev->isp_ci_reset)) {
		dev_err(&pdev->dev, "not found core isp_ci_reset\n");
		return PTR_ERR(cpp_dev->isp_ci_reset);
	}

	cpp_dev->lcd_mclk_reset = devm_reset_control_get_optional_shared(&pdev->dev, "lcd_mclk_reset");
	if (IS_ERR_OR_NULL(cpp_dev->lcd_mclk_reset)) {
		dev_err(&pdev->dev, "not found core lcd_mclk_reset\n");
		return PTR_ERR(cpp_dev->lcd_mclk_reset);
	}

	cpp_dev->fnc_clk = devm_clk_get(&pdev->dev, "cpp_func");
	if (IS_ERR(cpp_dev->fnc_clk)) {
		ret = PTR_ERR(cpp_dev->fnc_clk);
		dev_err(&pdev->dev, "failed to get function clock: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_KY_DEBUG
	cpp_running_info.is_dev_running = check_dev_running_status;
	cpp_running_info.nb.notifier_call = dev_clkoffdet_notifier_handler;
	clk_notifier_register(cpp_dev->fnc_clk, &cpp_running_info.nb);
	clk_notifier_register(cpp_dev->bus_clk, &cpp_running_info.nb);
//	clk_notifier_register(cpp_dev->ahb_clk, &cpp_running_info.nb);
#endif

	cpp_dev->bus_clk = devm_clk_get(&pdev->dev, "isp_axi");
	if (IS_ERR(cpp_dev->bus_clk)) {
		ret = PTR_ERR(cpp_dev->bus_clk);
		dev_err(&pdev->dev, "failed to get bus clock: %d\n", ret);
		return ret;
	}

	cpp_dev->dpu_clk = devm_clk_get(&pdev->dev, "dpu_mclk");
	if (IS_ERR(cpp_dev->dpu_clk)) {
		ret = PTR_ERR(cpp_dev->dpu_clk);
		dev_err(&pdev->dev, "failed to get dpu clock: %d\n", ret);
		return ret;
	}
#endif

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(33));
	if (ret)
		return ret;

	cpp_dev->pdev = pdev;
	platform_set_drvdata(pdev, cpp_dev);
	ret = cpp_iommu_register(cpp_dev);
	if (ret)
		return ret;
	cpp_dev->mmu_dev->state = CPP_IOMMU_ATTACHED;

	ret = cpp_init_subdev(cpp_dev);
	if (ret)
		return ret;

	mutex_init(&cpp_dev->mutex);
	init_completion(&cpp_dev->reset_complete);
	spin_lock_init(&cpp_dev->job_spinlock);
	INIT_LIST_HEAD(&cpp_dev->job_queue);
	cmd_queue_init(&cpp_dev->priv.idleq, "cpp idle queue");
	cmd_queue_init(&cpp_dev->priv.frmq, "cpp frm queue");
	cpp_dev->priv.cpp_dev = cpp_dev;
	cpp_dev->priv.job_flags = 0;
	cpp_dev->state = CPP_STATE_OFF;

	ret = cpp_setup_run_work(cpp_dev);
	if (ret)
		goto err_work;

	ret = cpp_init_bandwidth();
	if (ret)
		goto err_work;

	/* enable runtime pm */
	pm_runtime_enable(&pdev->dev);
	device_init_wakeup(&pdev->dev, true);

	cam_dbg("%s probed", dev_name(&pdev->dev));
	return ret;

err_work:
	mutex_destroy(&cpp_dev->mutex);
	plat_cam_unregister_subdev(&cpp_dev->csd);

	return ret;
}

static int cpp_remove(struct platform_device *pdev)
{
	struct cpp_device *cpp_dev;

	cpp_dev = platform_get_drvdata(pdev);
	if (!cpp_dev) {
		dev_err(&pdev->dev, "cpp device is NULL");
		return 0;
	}
	device_init_wakeup(&pdev->dev, false);
	pm_runtime_disable(&pdev->dev);
	cpp_deinit_bandwidth();

	cpp_destroy_run_work(cpp_dev);
	plat_cam_unregister_subdev(&cpp_dev->csd);
	cpp_iommu_unregister(cpp_dev);
	mutex_destroy(&cpp_dev->mutex);
	devm_kfree(&pdev->dev, cpp_dev);
	cam_dbg("%s removed", dev_name(&pdev->dev));

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int x1cpp_suspend(struct device *dev)
{
	/* TODO: */
	return 0;
}

static int x1cpp_resume(struct device *dev)
{
	/* TODO: */
	return 0;
}
#endif

#ifdef CONFIG_PM
static int x1cpp_runtime_suspend(struct device *dev)
{
	/* TODO: */
	return 0;
}

static int x1cpp_runtime_resume(struct device *dev)
{
	/* TODO: */
	return 0;
}
#endif

static const struct dev_pm_ops x1cpp_pm_ops = { SET_RUNTIME_PM_OPS(
	x1cpp_runtime_suspend, x1cpp_runtime_resume,
	NULL) SET_SYSTEM_SLEEP_PM_OPS(x1cpp_suspend, x1cpp_resume)
};
static struct platform_driver cpp_driver = {
	.driver = {
		.name = CPP_DRV_NAME,
		.of_match_table = x1cpp_dt_match,
		.pm		= &x1cpp_pm_ops,
	},
	.probe = cpp_probe,
	.remove = cpp_remove,
};

module_platform_driver(cpp_driver);

MODULE_DESCRIPTION("KY Camera Post Process Driver");
MODULE_LICENSE("GPL");
