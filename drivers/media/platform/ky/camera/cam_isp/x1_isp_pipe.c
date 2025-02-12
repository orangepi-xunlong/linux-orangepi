// SPDX-License-Identifier: GPL-2.0
/*
 * Description on this file
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#include "x1_isp_drv.h"
#include "x1_isp_statistic.h"
#include "x1_isp_pipe.h"
#include <cam_plat.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/dma-buf.h>

#define PIPE_DEVID_TO_HW_PIPELINE_ID(dev_id, hw_pipe_id) ({ \
		if (dev_id <= ISP_PIPE_DEV_ID_1)		\
			hw_pipe_id = dev_id;			\
		else						\
			hw_pipe_id = ISP_HW_PIPELINE_ID_0;	\
	})

#define PIPE_WORK_TYPE_TO_MEM_INDEX(type, mem_index) {	\
		mem_index = type - ISP_PIPE_WORK_TYPE_PREVIEW;	\
	}

int isp_pipe_sof_irq_handler(struct isp_irq_func_params *param);
int isp_pipe_irq_err_print_handler(struct isp_irq_func_params *param);
int isp_pipe_sde_sof_irq_handler(struct isp_irq_func_params *param);
int isp_pipe_sde_eof_irq_handler(struct isp_irq_func_params *param);
int isp_pipe_reset_done_irq_handler(struct isp_irq_func_params *param);
int isp_pipe_afc_eof_irq_handler(struct isp_irq_func_params *param);
int isp_pipe_aem_eof_irq_handler(struct isp_irq_func_params *param);

int isp_pipe_task_job_init(struct isp_pipe_task *pipe_task);
int _isp_pipe_job_clear(struct x1isp_pipe_dev *pipe_dev);
int isp_pipe_task_vote_handler(struct x1isp_pipe_dev *pipe_dev,
			       struct isp_pipe_task *pipe_task, u32 frame_num,
			       u32 voter_type);

struct isp_irq_handler_info {
	u32 irq_bit;
	x1isp_irq_handler irq_handler;
};

static struct isp_irq_handler_info g_host_irq_handler_infos[] = {
	{ ISP_IRQ_BIT_PIPE_SOF, isp_pipe_sof_irq_handler },
	{ ISP_IRQ_BIT_STAT_ERR, isp_pipe_irq_err_print_handler },
	{ ISP_IRQ_BIT_SDE_SOF, isp_pipe_sde_sof_irq_handler },
	{ ISP_IRQ_BIT_SDE_EOF, isp_pipe_sde_eof_irq_handler },
	// {ISP_IRQ_BIT_RESET_DONE, isp_pipe_reset_done_irq_handler}, //always pipe0, vi do it
	{ ISP_IRQ_BIT_AEM_EOF, isp_pipe_aem_eof_irq_handler },
	{ ISP_IRQ_BIT_AFC_EOF, isp_pipe_afc_eof_irq_handler },
	// {ISP_IRQ_BIT_ISP_ERR, isp_pipe_irq_err_print_handler},
};

struct isp_task_stat_map_info {
	u8 count;
	char stat_ids[3];	//max 3 stat at once
};

static struct isp_task_stat_map_info g_task_stat_map_infos[ISP_PIPE_TASK_TYPE_MAX] = {
	{ 3, { ISP_STAT_ID_AWB, ISP_STAT_ID_LTM, ISP_STAT_ID_EIS} },	//sof firmware calc task
	{ 1, { ISP_STAT_ID_AE, -1, -1} },	//eof firmware calc task
	{ 1, { ISP_STAT_ID_AF, -1, -1} },	// af
};

__maybe_unused static int isp_pipe_get_task_type_by_stat_id(u32 stat_id)
{
	int task_type = -1;

	if (stat_id == ISP_STAT_ID_AE || stat_id == ISP_STAT_ID_AWB)
		task_type = ISP_PIPE_TASK_TYPE_SOF;
	else if (stat_id == ISP_STAT_ID_AF)
		task_type = ISP_PIPE_TASK_TYPE_AF;
	else
		isp_log_err("%s:unsupported stat id(%d) for task type!", __func__, stat_id);

	return task_type;
}

/**
 * the functions prefixed with x1isp are all exposured to external.
 */

static int isp_pipe_config_irqmask(struct x1isp_pipe_dev *pipe_dev)
{
	int ret = 0;
	u32 reg_addr = 0, reg_value = 0, reg_mask = 0;
	u32 hw_pipe_id;

	/*
	 * need irq:
	 * bit[0]:host_isp_pipe_sof_irq_mask
	 * bit[11]:host_isp_statistics_err_irq_mask
	 * bit[12]:host_isp_sde_sof_irq_mask
	 * bit[26]:host_isp_aem_eof_irq
	 * bit[29]:host_isp_afc_eof_irq_mask
	 */

	hw_pipe_id = pipe_dev->isp_irq_ctx.hw_pipe_id;
	if (pipe_dev->pipedev_id <= ISP_PIPE_DEV_ID_1) {
		//1. clear irq raw status
		reg_addr = ISP_REG_OFFSET_TOP_PIPE(hw_pipe_id) + ISP_REG_IRQ_STATUS;
		x1isp_reg_writel(reg_addr, 0xffffffff, 0xffffffff);

		if (pipe_dev->frameinfo_get_by_eof || !pipe_dev->eof_task_hd_by_sof)
			reg_value = BIT(0) | BIT(11) | BIT(12) | BIT(26) | BIT(29);
		else
			reg_value = BIT(0) | BIT(11) | BIT(12) | BIT(29);

		if (pipe_dev->eof_task_hd_by_sof)
			bitmap_set(&pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_SOF].
				   stat_bitmap, ISP_STAT_ID_AE, 1);

		reg_mask = reg_value;
		reg_addr = ISP_REG_OFFSET_TOP_PIPE(hw_pipe_id) + ISP_REG_IRQ_MASK;
		x1isp_reg_writel(reg_addr, reg_value, reg_mask);
	} else {
		ret = -1;
		isp_log_err("unsupport x1isp pipe dev%d!", pipe_dev->pipedev_id);
	}

	return ret;
}

static int isp_pipe_clear_irqmask(struct x1isp_pipe_dev *pipe_dev)
{
	u32 reg_addr = 0, reg_value = 0, reg_mask = 0;
	u32 hw_pipe_id;

	hw_pipe_id = pipe_dev->isp_irq_ctx.hw_pipe_id;
	if (pipe_dev->pipedev_id <= ISP_PIPE_DEV_ID_1) {
		reg_value = 0;
		reg_mask = BIT(0) | BIT(11) | BIT(12) | BIT(26) | BIT(29);
		reg_addr = ISP_REG_OFFSET_TOP_PIPE(hw_pipe_id) + ISP_REG_IRQ_MASK;
		x1isp_reg_writel(reg_addr, reg_value, reg_mask);
	}

	if (pipe_dev->eof_task_hd_by_sof)
		bitmap_clear(&pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_SOF].stat_bitmap,
			     ISP_STAT_ID_AE, 1);

	return 0;
}

static int _isp_pipe_prepare_capture_memory(struct x1isp_pipe_dev *pipe_dev)
{
	pipe_dev->slice_reg_mem =
	    kzalloc(X1ISP_SLICE_REG_MAX_NUM * sizeof(struct isp_reg_unit), GFP_KERNEL);
	if (!pipe_dev->slice_reg_mem) {
		isp_log_err("%s: alloc memory for slice regs failed!", __func__);
		return -ENOMEM;
	}

	return 0;
}

static int _isp_pipe_free_capture_memory(struct x1isp_pipe_dev *pipe_dev)
{
	if (pipe_dev->slice_reg_mem) {
		kfree(pipe_dev->slice_reg_mem);
		pipe_dev->slice_reg_mem = NULL;
	}

	return 0;
}

static int x1isp_pipe_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	struct isp_cdev_info *cdev_info =
	    container_of(inode->i_cdev, struct isp_cdev_info, isp_cdev);
	struct x1isp_pipe_dev *pipe_dev = NULL;

	pipe_dev = (struct x1isp_pipe_dev *)(cdev_info->p_dev);
	ISP_DRV_CHECK_POINTER(pipe_dev);
	mutex_lock(&pipe_dev->isp_pipedev_mutex);
	pipe_dev->open_cnt++;
	if (1 == pipe_dev->open_cnt) {
		x1isp_dev_open();
		ret = x1isp_dev_clock_set(1);
		if (ret)
			goto fail_close_dev;;
		ret = _isp_pipe_prepare_capture_memory(pipe_dev);
		if (ret)
			goto fail_clock;
	}

	filp->private_data = pipe_dev;
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);
	isp_log_dbg("open x1isp pipe dev%d!", pipe_dev->pipedev_id);
	return ret;

fail_clock:
	x1isp_dev_clock_set(0);
fail_close_dev:
	x1isp_dev_release();
	pipe_dev->open_cnt--;
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);
	return ret;
}

static int _isp_pipe_undeploy_driver(struct x1isp_pipe_dev *pipe_dev)
{
	int mem_index = 0;

	for (mem_index = 0; mem_index < ISP_PIPE_WORK_TYPE_CAPTURE + 1; mem_index++) {
		if (pipe_dev->isp_reg_mem[mem_index].config) {
			if (pipe_dev->fd_buffer) {
				x1isp_dev_put_viraddr_to_dma_buf(pipe_dev->isp_reg_mem[mem_index].dma_buffer,
								  pipe_dev->isp_reg_mem[mem_index].kvir_addr);
				dma_buf_put(pipe_dev->isp_reg_mem[mem_index].dma_buffer);
			} else {
				//phy addr to viraddr in kernel.
				isp_log_err("%s: need to realize!", __func__);
				return -EPERM;
			}

			pipe_dev->isp_reg_mem[mem_index].config = false;
			memset(&pipe_dev->isp_reg_mem[mem_index], 0, sizeof(struct isp_kmem_info));
		}
	}

	return 0;
}

static int _isp_pipe_close_with_exception(struct x1isp_pipe_dev *pipe_dev)
{
	int i, cur_work_type = 0;

	if (ISP_WORK_STATUS_START == pipe_dev->work_status) {
		//stop and clear job first.
		cur_work_type = pipe_dev->work_type;
		_isp_pipe_job_clear(pipe_dev);
	}
	//try flush buffers when exit by unusual.
	for (i = 0; i < pipe_dev->stats_node_cnt; i++)
		x1isp_stat_try_flush_buffer(&pipe_dev->stats_nodes[i]);

	if (atomic_read(&pipe_dev->isp_irq_ctx.cur_frame_num) > 0)
		atomic_set(&pipe_dev->isp_irq_ctx.cur_frame_num, 0);

	return 0;
}

static int x1isp_pipe_release(struct inode *inode, struct file *filp)
{
	int ret = 0, i;
	struct isp_cdev_info *cdev_info =
	    container_of(inode->i_cdev, struct isp_cdev_info, isp_cdev);
	struct x1isp_pipe_dev *pipe_dev = NULL;

	pipe_dev = (struct x1isp_pipe_dev *)(cdev_info->p_dev);
	ISP_DRV_CHECK_POINTER(pipe_dev);
	mutex_lock(&pipe_dev->isp_pipedev_mutex);
	pipe_dev->open_cnt--;
	if (0 == pipe_dev->open_cnt) {
		_isp_pipe_close_with_exception(pipe_dev);
		for (i = ISP_PIPE_TASK_TYPE_SOF; i <= ISP_PIPE_TASK_TYPE_AF; i++)
			isp_pipe_task_job_init(&pipe_dev->pipe_tasks[i]);
		_isp_pipe_undeploy_driver(pipe_dev);
		x1isp_dev_clock_set(0);
		x1isp_dev_release();
		_isp_pipe_free_capture_memory(pipe_dev);
		pipe_dev->capture_client_num = 0;
	}

	filp->private_data = NULL;
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);
	isp_log_dbg("close x1isp pipe dev%d!", pipe_dev->pipedev_id);
	return ret;
}

static void isp_pipe_fill_user_task_stat_result(struct isp_ubuf_uint *ubuf_uint,
					 struct isp_kbuffer_info *kbuf_info)
{
	int plane_size = 0;

	if (ubuf_uint) {
		if (kbuf_info) {
			ubuf_uint->plane_count = kbuf_info->plane_count;
			ubuf_uint->buf_index = kbuf_info->buf_index;
			plane_size = kbuf_info->plane_count * sizeof(struct isp_buffer_plane);
			memcpy(ubuf_uint->buf_planes, kbuf_info->buf_planes, plane_size);
		} else {
			memset(ubuf_uint, 0, sizeof(struct isp_ubuf_uint));
		}
	}
}

static void isp_pipe_fill_user_task_data(struct isp_user_task_info *user_task, struct isp_kbuffer_info *kbuf_info, u32 stat_id)
{
	if (ISP_PIPE_TASK_TYPE_AF == user_task->task_type) {
		if (ISP_STAT_ID_AF == stat_id)
			isp_pipe_fill_user_task_stat_result(&user_task->stats_result.af_task.af_result, kbuf_info);
		else if (ISP_STAT_ID_PDC == stat_id)
			isp_pipe_fill_user_task_stat_result(&user_task->stats_result.af_task.pdc_result, kbuf_info);
	} else if (ISP_PIPE_TASK_TYPE_SOF == user_task->task_type) {
		if (ISP_STAT_ID_AWB == stat_id)
			isp_pipe_fill_user_task_stat_result(&user_task->stats_result.sof_task.awb_result, kbuf_info);
		else if (ISP_STAT_ID_EIS == stat_id)
			isp_pipe_fill_user_task_stat_result(&user_task->stats_result.sof_task.eis_result, kbuf_info);
		else if (ISP_STAT_ID_LTM == stat_id)
			isp_pipe_fill_user_task_stat_result(&user_task->stats_result.sof_task.ltm_result, kbuf_info);
		else if (ISP_STAT_ID_AE == stat_id)
			isp_pipe_fill_user_task_stat_result(&user_task->stats_result.sof_task.ae_result, kbuf_info);
	} else if (ISP_PIPE_TASK_TYPE_EOF == user_task->task_type) {
		isp_pipe_fill_user_task_stat_result(&user_task->stats_result.eof_task.ae_result, kbuf_info);
	}
}

static void isp_pipe_task_get_stats_result(struct x1isp_pipe_dev *pipe_dev,
				    struct isp_pipe_task *pipe_task,
				    struct isp_user_task_info *user_task, u32 frame_num,
				    u32 discard)
{
	int set_bit = -1, node_index = 0, stat_id = 0;
	struct isp_kbuffer_info *kbuf_info = NULL;
	u32 find_frame = 0;
	struct x1isp_stats_node *stats_node = NULL;

	for_each_set_bit(set_bit, &pipe_task->stat_bitmap, ISP_STAT_ID_MAX) {
		if (set_bit > ISP_STAT_ID_MAX) {
			//slave pipe stat
			if (pipe_dev->stats_node_cnt < 2) {
				isp_log_err
				    ("fatal error:stat bitmap use slave pipe, but stat node count is %d!",
				     pipe_dev->stats_node_cnt);
				continue;
			}
			node_index = 1;
		} else {
			node_index = 0;
		}

		stat_id = set_bit - (node_index * ISP_SLAVE_STAT_ID_AE);
		stats_node = &pipe_dev->stats_nodes[node_index];
		if (stat_id == ISP_STAT_ID_EIS)
			find_frame = frame_num - 1;	//EIS must find the previous frame.
		else
			find_frame = frame_num;
		if (discard) {
			x1isp_stat_get_donebuf_by_frameid(stats_node, stat_id, find_frame, true);
		} else {
			kbuf_info = x1isp_stat_get_donebuf_by_frameid(stats_node, stat_id,
								       find_frame, false);
			isp_pipe_fill_user_task_data(user_task, kbuf_info, stat_id);
		}
	}
}

int isp_pipe_task_job_init(struct isp_pipe_task *pipe_task)
{
	int i;

	atomic_set(&pipe_task->frame_num, 0);
	pipe_task->complete_cnt = 0;
	pipe_task->complete_frame_num = 0;
	reinit_completion(&pipe_task->wait_complete);

	pipe_task->task_trigger = false;

	pipe_task->vote_system.cur_ticket_cnt = 0;
	pipe_task->use_vote_sys = false;
	for (i = 0; i < pipe_task->vote_system.sys_trigger_num; i++)
		pipe_task->vote_system.voter_validity[i] = false;

	return 0;
}

static int x1isp_pipe_wait_interrupts(struct x1isp_pipe_dev *pipe_dev,
				struct isp_user_task_info *user_task)
{
	int ret = 0, hw_pipe_id = 0, task_type = -1, work_cnt = 0;
	struct completion *wait_complete = NULL;
	struct isp_pipe_task *cur_pipe_task = NULL;
	unsigned long timeout = 0;
	int frame_num = 0;

	PIPE_DEVID_TO_HW_PIPELINE_ID(pipe_dev->pipedev_id, hw_pipe_id);
	if (hw_pipe_id >= ISP_HW_PIPELINE_ID_MAX) {
		isp_log_err("%s: Please check the pipeline id(%d)!", __func__,
			    hw_pipe_id);
		return -EINVAL;
	}

	task_type = user_task->task_type;
	if (task_type >= ISP_PIPE_TASK_TYPE_MAX) {
		isp_log_err("%s: Invalid task type(%d)!", __func__, task_type);
		return -EINVAL;
	}

	cur_pipe_task = &pipe_dev->pipe_tasks[task_type];
	wait_complete = &cur_pipe_task->wait_complete;

	timeout = msecs_to_jiffies(3000);
	ret = wait_for_completion_timeout(wait_complete, timeout);
	if (0 == ret) {
		if (ISP_PIPE_WORK_TYPE_PREVIEW == pipe_dev->work_type)
			isp_log_warn
			    ("wait for isp(p%d) interrupt irq timeout for task(%d), state=%ld,%ld!",
			     hw_pipe_id, task_type,
			     pipe_dev->isp_irq_ctx.isp_irq_tasklet.state,
			     pipe_dev->isp_irq_ctx.isp_dma_irq_tasklet.state);
		//wakeup user
		return -EAGAIN;
	}
	//lock used only between thread and soft irq.
	spin_lock_bh(&cur_pipe_task->complete_lock);
	work_cnt = cur_pipe_task->complete_cnt--;
	frame_num = cur_pipe_task->complete_frame_num;
	spin_unlock_bh(&cur_pipe_task->complete_lock);
	//get stat result.
	if (work_cnt > 0) {
		user_task->result_valid = true;
		user_task->frame_number = frame_num;
		user_task->work_status = pipe_dev->work_status;
		if (ISP_WORK_STATUS_START == pipe_dev->work_status) {
			isp_pipe_task_get_stats_result(pipe_dev, cur_pipe_task,
						       user_task, frame_num, false);
		} else if (ISP_WORK_STATUS_STOP == pipe_dev->work_status) {
			//clear some flag during job.
		}
		ret = 0;
	} else {
		isp_log_err("%s: fatal error! the complete count is %d!", __func__,
			    work_cnt);
		ret = -EPERM;
	}

	return ret;
}

static int isp_pipe_start_task_vote(struct x1isp_pipe_dev *pipe_dev, unsigned int enable)
{
	int ret = 0;
	struct isp_pipe_task *af_task = NULL;

	af_task = &pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_AF];
	if (enable) {
		//atomic bitops
		set_bit(ISP_STAT_ID_PDC, &af_task->stat_bitmap);
		af_task->use_vote_sys = true;
		af_task->stat_bits_cnt++;
		isp_log_dbg("enable pdc voter,bitmap=0x%lx!", af_task->stat_bitmap);
	} else {
		clear_bit(ISP_STAT_ID_PDC, &af_task->stat_bitmap);
		af_task->stat_bits_cnt--;
		af_task->use_vote_sys = false;
		isp_log_dbg("disable pdc voter,bitmap=0x%lx!", af_task->stat_bitmap);
	}

	return ret;
}

//tasklet context
static int x1isp_pipe_notify_event(void *pdev, u32 event, void *payload, u32 load_len)
{
	int ret = 0;
	struct x1isp_pipe_dev *pipe_dev = (struct x1isp_pipe_dev *)pdev;
	struct stats_notify_params *event_params = NULL;

	ISP_DRV_CHECK_POINTER(pipe_dev);
	ISP_DRV_CHECK_POINTER(payload);

	event_params = (struct stats_notify_params *)payload;
	if (event_params->stat_id != ISP_STAT_ID_PDC) {
		isp_log_err("only support pdc dma trigger vote system!");
		return -EPERM;
	}

	switch (event) {
	case PIPE_EVENT_TRIGGER_VOTE_SYS:
		ret = isp_pipe_start_task_vote(pipe_dev, event_params->event_enable);
		break;
	case PIPE_EVENT_CAST_VOTE:
		ret = isp_pipe_task_vote_handler(pipe_dev,
						 &pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_AF],
						 event_params->frame_id,
						 TASK_VOTER_PDC_EOF);
		break;
	default:
		isp_log_err("unknown this envent:%d!", event);
		return -EINVAL;
	}
	return ret;
}

static int x1isp_pipe_enable_pdc_af(struct x1isp_pipe_dev *pipe_dev, u32 *enable)
{
	int ret = 0, i;
	u32 pdc_enable = *enable;

	for (i = 0; i < pipe_dev->stats_node_cnt; i++) {
		ret = x1isp_stat_dma_dynamic_enable(&pipe_dev->stats_nodes[i],
						     ISP_STAT_ID_PDC, pdc_enable);
	}

	return ret;
}

static int __isp_pipe_start_preview_job(struct x1isp_pipe_dev *pipe_dev, u32 switch_stream)
{
	int i, ret = 0;

	//preview need all stats while capture don't
	for (i = 0; i < pipe_dev->stats_node_cnt; i++) {
		if (!switch_stream) {
			ret = x1isp_stat_node_streamon_dma_port(&pipe_dev->stats_nodes[i]);
			if (ret)
				return ret;
		}

		x1isp_stat_node_cfg_dma_irqmask(&pipe_dev->stats_nodes[i]);
	}

	isp_pipe_config_irqmask(pipe_dev);

	return 0;
}

static int isp_pipe_start_job(struct x1isp_pipe_dev *pipe_dev, u32 work_type)
{
	int ret = 0;

	mutex_lock(&pipe_dev->isp_pipedev_mutex);
	if (ISP_WORK_STATUS_START == pipe_dev->work_status) {
		isp_log_err("the isp pipedev(%d) is already start work at type:%d!",
			    pipe_dev->pipedev_id, pipe_dev->work_type);
		ret = -EPERM;
		goto Safe_Exit;
	}

	if (ISP_PIPE_WORK_TYPE_PREVIEW == work_type) {
		ret = __isp_pipe_start_preview_job(pipe_dev, false);
		if (ret)
			goto Safe_Exit;
	}

	pipe_dev->work_type = work_type;
	pipe_dev->work_status = ISP_WORK_STATUS_START;

Safe_Exit:
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);

	return ret;
}

static int __isp_pipe_clear_preview_job(struct x1isp_pipe_dev *pipe_dev, u32 switch_stream)
{
	int i = 0;

	//wait tasklet run and disable it.
	tasklet_disable(&pipe_dev->isp_irq_ctx.isp_irq_tasklet);
	tasklet_disable(&pipe_dev->isp_irq_ctx.isp_dma_irq_tasklet);

	for (i = 0; i < pipe_dev->stats_node_cnt; i++) {
		x1isp_stat_node_clear_dma_irqmask(&pipe_dev->stats_nodes[i]);
		x1isp_stat_reset_dma_busybuf_frameid(&pipe_dev->stats_nodes[i]);
		if (!switch_stream) {
			x1isp_stat_node_streamoff_dma_port(&pipe_dev->stats_nodes[i]);
			x1isp_stat_job_flags_init(&pipe_dev->stats_nodes[i]);
		}
	}

	isp_pipe_clear_irqmask(pipe_dev);
	//enable tasklet, make sure enable/disable tasklet operation is symmetric.
	tasklet_enable(&pipe_dev->isp_irq_ctx.isp_irq_tasklet);
	tasklet_enable(&pipe_dev->isp_irq_ctx.isp_dma_irq_tasklet);
	atomic_set(&pipe_dev->isp_irq_ctx.cur_frame_num, 0);

	return 0;
}

int _isp_pipe_job_clear(struct x1isp_pipe_dev *pipe_dev)
{
	//declare stop work flag first.
	pipe_dev->work_status = ISP_WORK_STATUS_STOP;

	if (ISP_PIPE_WORK_TYPE_PREVIEW == pipe_dev->work_type)
		__isp_pipe_clear_preview_job(pipe_dev, false);

	pipe_dev->work_type = ISP_PIPE_WORK_TYPE_INIT;
	return 0;
}

static int isp_pipe_switch_stream_job(struct x1isp_pipe_dev *pipe_dev, u32 work_type)
{
	int ret = 0;

	if (pipe_dev->open_cnt == 1)
		return ret;	//needn't switch

	mutex_lock(&pipe_dev->isp_pipedev_mutex);
	//clear last work
	if (work_type != pipe_dev->work_type)
		if (ISP_PIPE_WORK_TYPE_PREVIEW == pipe_dev->work_type)
			__isp_pipe_clear_preview_job(pipe_dev, true);
	//start next work
	if (ISP_PIPE_WORK_TYPE_CAPTURE == work_type) {
		pipe_dev->capture_client_num++;
		if (1 == pipe_dev->capture_client_num) {
			pipe_dev->work_type = work_type;
			pipe_dev->work_status = ISP_WORK_STATUS_START;
		}
	} else {
		//start preview wait all capture client done
		pipe_dev->capture_client_num--;
		if (0 == pipe_dev->capture_client_num) {
			__isp_pipe_start_preview_job(pipe_dev, true);
			pipe_dev->work_type = work_type;
			pipe_dev->work_status = ISP_WORK_STATUS_START;
		}
	}
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);

	return ret;
}

static int isp_pipe_stop_job(struct x1isp_pipe_dev *pipe_dev, u32 work_type)
{
	int ret = 0, i;
	struct isp_pipe_task *pipe_task = NULL;

	mutex_lock(&pipe_dev->isp_pipedev_mutex);
	if (pipe_dev->work_status != ISP_WORK_STATUS_START) {
		isp_log_err("the isp pipedev(%d) isn't start work!", pipe_dev->pipedev_id);
		ret = -EPERM;
		goto Safe_Exit;
	}

	if (pipe_dev->work_type != work_type) {
		isp_log_warn("the isp pipedev(%d) worktype(%d) isn't match your's(%d)!",
			     pipe_dev->pipedev_id, pipe_dev->work_type, work_type);
	}

	_isp_pipe_job_clear(pipe_dev);

	//wakeup user to stop work
	if (ISP_PIPE_WORK_TYPE_PREVIEW == work_type) {
		for (i = ISP_PIPE_TASK_TYPE_SOF; i <= ISP_PIPE_TASK_TYPE_AF; i++) {
			pipe_task = &pipe_dev->pipe_tasks[i];
			//lock used only between thread and soft irq.
			spin_lock_bh(&pipe_task->complete_lock);
			pipe_task->complete_cnt++;
			complete(&pipe_task->wait_complete);
			spin_unlock_bh(&pipe_task->complete_lock);
		}
	}

Safe_Exit:
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);

	return ret;
}

static int x1isp_pipe_notify_jobs(struct x1isp_pipe_dev *pipe_dev,
			    struct isp_job_describer *job_action)
{
	int ret = 0;

	ISP_DRV_CHECK_POINTER(job_action);
	ISP_DRV_CHECK_PARAMETERS(job_action->work_type, ISP_PIPE_WORK_TYPE_PREVIEW,
				 ISP_PIPE_WORK_TYPE_CAPTURE, "job type");

	if (ISP_JOB_ACTION_START == job_action->action) {
		ret = isp_pipe_start_job(pipe_dev, job_action->work_type);
	} else if (ISP_JOB_ACTION_STOP == job_action->action) {
		ret = isp_pipe_stop_job(pipe_dev, job_action->work_type);
	} else if (ISP_JOB_ACTION_RESTART == job_action->action) {

	} else if (ISP_JOB_ACTION_SWITCH == job_action->action) {
		ret = isp_pipe_switch_stream_job(pipe_dev, job_action->work_type);
	}

	return ret;
}

static int x1isp_pipe_deploy_driver(struct x1isp_pipe_dev *pipe_dev, struct isp_drv_deployment *drv_deploy)
{
	int ret = 0, mem_index = 0;
	struct dma_buf *dma_buffer = NULL;

	//such as reg mem,we should map kvir_addr?
	ISP_DRV_CHECK_POINTER(drv_deploy);
	ISP_DRV_CHECK_PARAMETERS(drv_deploy->work_type, ISP_PIPE_WORK_TYPE_PREVIEW,
				 ISP_PIPE_WORK_TYPE_CAPTURE, "pipe_work_type");
	PIPE_WORK_TYPE_TO_MEM_INDEX(drv_deploy->work_type, mem_index);
	mutex_lock(&pipe_dev->isp_pipedev_mutex);
	if (ISP_PIPE_WORK_TYPE_PREVIEW == drv_deploy->work_type) {
		if (pipe_dev->isp_reg_mem[mem_index].config) {
			isp_log_err("%s:redeploy isp pipe(%d), index:%d!", __func__, pipe_dev->pipedev_id, mem_index);
			ret = -EPERM;
			goto Safe_Exit;
		}
	} else {
		if (pipe_dev->isp_reg_mem[mem_index].config
			&& pipe_dev->isp_reg_mem[mem_index + 1].config) {
			isp_log_err("%s:redeploy isp pipe(%d), index:%d!", __func__, pipe_dev->pipedev_id, mem_index);
			ret = -EPERM;
			goto Safe_Exit;
		}

		if (pipe_dev->isp_reg_mem[mem_index].config)
			mem_index += 1;
	}

	pipe_dev->fd_buffer = drv_deploy->fd_buffer;
	if (pipe_dev->fd_buffer) {
		dma_buffer = dma_buf_get(drv_deploy->reg_mem.fd);
		if (IS_ERR(dma_buffer)) {
			isp_log_err("%s: get dma buffer failed!", __func__);
			ret = -EBADF;
			goto Safe_Exit;
		}
		pipe_dev->isp_reg_mem[mem_index].mem.fd = drv_deploy->reg_mem.fd;
		pipe_dev->isp_reg_mem[mem_index].dma_buffer = dma_buffer;
		pipe_dev->isp_reg_mem[mem_index].mem_size = drv_deploy->reg_mem_size;
		ret = x1isp_dev_get_viraddr_from_dma_buf(dma_buffer, &pipe_dev->isp_reg_mem[mem_index].kvir_addr);
		if (ret)
			goto Safe_Exit;
	} else {
		//phy addr to viraddr in kernel.
		isp_log_err("%s: need to realize!", __func__);
		ret = -EPERM;
		goto Safe_Exit;
	}

	pipe_dev->isp_reg_mem[mem_index].config = true;
	drv_deploy->reg_mem_index = mem_index;

Safe_Exit:
	mutex_unlock(&pipe_dev->isp_pipedev_mutex);
	return ret;
}

static int x1isp_pipe_undeploy_driver(struct x1isp_pipe_dev *pipe_dev, u32 mem_index)
{
	ISP_DRV_CHECK_MAX_PARAMETERS(mem_index, ISP_PIPE_WORK_TYPE_CAPTURE, "pipe mem index");
	if (pipe_dev->isp_reg_mem[mem_index].config) {
		if (pipe_dev->fd_buffer) {
			x1isp_dev_put_viraddr_to_dma_buf(pipe_dev->isp_reg_mem[mem_index].dma_buffer,
							  pipe_dev->isp_reg_mem[mem_index].kvir_addr);
			dma_buf_put(pipe_dev->isp_reg_mem[mem_index].dma_buffer);
		} else {
			//phy addr to viraddr in kernel.
			isp_log_err("%s: need to realize!", __func__);
			return -EPERM;
		}

		pipe_dev->isp_reg_mem[mem_index].config = false;
		memset(&pipe_dev->isp_reg_mem[mem_index], 0, sizeof(struct isp_kmem_info));
	}

	return 0;
}

static int x1isp_pipe_request_stats_buffer(struct x1isp_pipe_dev *pipe_dev,
				     struct isp_buffer_request_info *request_info)
{
	int ret = 0, i;

	for (i = 0; i < pipe_dev->stats_node_cnt; i++)	//normal 1
		ret = x1isp_stat_reqbuffer(&pipe_dev->stats_nodes[i], request_info);

	return ret;
}

static int x1isp_pipe_enqueue_stats_buffer(struct x1isp_pipe_dev *pipe_dev,
				     struct isp_buffer_enqueue_info *enqueue_info)
{
	int ret = 0, i;

	for (i = 0; i < pipe_dev->stats_node_cnt; i++)	//normal 1
		ret = x1isp_stat_qbuffer(&pipe_dev->stats_nodes[i], enqueue_info);

	return ret;
}

static int x1isp_pipe_flush_stats_buffer(struct x1isp_pipe_dev *pipe_dev)
{
	int ret = 0, i;

	for (i = 0; i < pipe_dev->stats_node_cnt; i++)	//normal 1
		ret = x1isp_stat_flush_buffer(&pipe_dev->stats_nodes[i]);

	return ret;
}

static int _isp_pipe_set_slice_regs(void *kreg_mem, struct isp_slice_regs *slice_reg,
			     int slice_index)
{
	int ret = 0, reg_count;

	reg_count = slice_reg->reg_count;
	if (reg_count > X1ISP_SLICE_REG_MAX_NUM) {
		isp_log_err("%s:slice reg count:%d is greater than maxnum(%d)!",
			    __func__, reg_count, X1ISP_SLICE_REG_MAX_NUM);
		return -EINVAL;
	}

	if (copy_from_user(kreg_mem, (void __user *)slice_reg->data,
			   sizeof(struct isp_reg_unit) * reg_count)) {
		isp_log_err("failed to copy slice(%d) reg from user", slice_index);
		return -EFAULT;
	}

	ret = x1isp_reg_write_brust(kreg_mem, reg_count, false, NULL);
	memset(kreg_mem, 0, sizeof(struct isp_reg_unit) * reg_count);

	return ret;
}

static int x1isp_pipe_trigger_capture(struct x1isp_pipe_dev *pipe_dev,
				struct isp_capture_package *capture_package)
{
	int ret = 0, slice_count, i, stop_job = 0, switch_job = 0;
	struct camera_capture_slice_info camera_slice_info;
	struct spm_camera_vi_ops *vi_ops = NULL;
	u32 wait_time = 5000, vi_exit = 0;

	ISP_DRV_CHECK_POINTER(capture_package);
	slice_count = capture_package->slice_count;
	if (slice_count > X1ISP_SLICE_MAX_NUM) {
		isp_log_err("%s:slice count:%d is greater than maxnum(%d)!", __func__,
			    slice_count, X1ISP_SLICE_MAX_NUM);
		return -EINVAL;
	}

	if (ISP_WORK_STATUS_START == pipe_dev->work_status
	    && ISP_PIPE_WORK_TYPE_PREVIEW == pipe_dev->work_type) {
		isp_log_err("%s:isp pipedev%d is working on preview!", __func__,
			    pipe_dev->pipedev_id);
		return -EPERM;
	}

	x1isp_dev_get_vi_ops(&vi_ops);
	if (vi_ops == NULL) {
		isp_log_err("%s:isp pipedev%d get vi operation failed!", __func__,
			    pipe_dev->pipedev_id);
		// return -EPERM;
	}

	mutex_lock(&pipe_dev->pipedev_capture_mutex);
	if (pipe_dev->work_status != ISP_WORK_STATUS_START) {
		isp_pipe_start_job(pipe_dev, ISP_PIPE_WORK_TYPE_CAPTURE);
		stop_job = true;
	} else {
		switch_job = true;
	}

	memset(&camera_slice_info, 0, sizeof(struct camera_capture_slice_info));
	camera_slice_info.total_slice_cnt = slice_count;
	for (i = 0; i < slice_count; i++) {
		//1. set slice regs
		ret = _isp_pipe_set_slice_regs(pipe_dev->slice_reg_mem,
					       &capture_package->capture_slice_packs[i].slice_reg,
					       i);
		if (ret)
			goto vi_fail;
		//2. notify vi start and wait done
		camera_slice_info.hw_pipe_id = pipe_dev->pipedev_id;
		camera_slice_info.slice_width =
		    capture_package->capture_slice_packs[i].slice_width;
		camera_slice_info.raw_read_offset =
		    capture_package->capture_slice_packs[i].raw_read_offset;
		camera_slice_info.yuv_out_offset =
		    capture_package->capture_slice_packs[i].yuv_out_offset;
		camera_slice_info.dwt_offset[0] =
		    capture_package->capture_slice_packs[i].dwt_offset[0];
		camera_slice_info.dwt_offset[1] =
		    capture_package->capture_slice_packs[i].dwt_offset[1];
		camera_slice_info.dwt_offset[2] =
		    capture_package->capture_slice_packs[i].dwt_offset[2];
		camera_slice_info.dwt_offset[3] =
		    capture_package->capture_slice_packs[i].dwt_offset[3];
		if (vi_ops) {
			ret = vi_ops->notify_caputre_until_done(i, &camera_slice_info, wait_time);	//ms
			if (ret) {
				isp_log_err("isp pipedev%d capture slice%d failed!",
					    pipe_dev->pipedev_id, i);
				goto job_exit;
			}

			if (0 == vi_exit)
				vi_exit = 1;
		}
	}

	isp_log_dbg("isp pipe(%d) capture done!", pipe_dev->pipedev_id);
	if (stop_job)
		isp_pipe_stop_job(pipe_dev, ISP_PIPE_WORK_TYPE_CAPTURE);

	if (switch_job)
		isp_pipe_switch_stream_job(pipe_dev, ISP_PIPE_WORK_TYPE_PREVIEW);

	mutex_unlock(&pipe_dev->pipedev_capture_mutex);
	return ret;

vi_fail:
	if (vi_exit) {
		camera_slice_info.exception_exit = 1;
		vi_ops->notify_caputre_until_done(0, &camera_slice_info, wait_time);
	}
job_exit:
	if (stop_job)
		isp_pipe_stop_job(pipe_dev, ISP_PIPE_WORK_TYPE_CAPTURE);

	if (switch_job)
		isp_pipe_switch_stream_job(pipe_dev, ISP_PIPE_WORK_TYPE_PREVIEW);

	mutex_unlock(&pipe_dev->pipedev_capture_mutex);
	return ret;
}

static int x1isp_pipe_set_endframe_work(struct x1isp_pipe_dev *pipe_dev,
				  struct isp_endframe_work_info *end_info)
{
	int ret = 0;

	pipe_dev->eof_task_hd_by_sof = end_info->process_ae_by_sof;
	pipe_dev->frameinfo_get_by_eof = end_info->get_frameinfo_by_eof;
	return ret;
}

static long x1isp_pipe_ioctl_core(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct x1isp_pipe_dev *pipe_dev = file->private_data;
	struct isp_regs_info *user_regs;

	ISP_DRV_CHECK_POINTER(pipe_dev);
	switch (cmd) {
	case ISP_IOC_DEPLOY_DRV:
		ret = x1isp_pipe_deploy_driver(pipe_dev, (struct isp_drv_deployment*)arg);
		break;
	case ISP_IOC_UNDEPLOY_DRV:
		ret = x1isp_pipe_undeploy_driver(pipe_dev, *((u32*)arg));
		break;
	case ISP_IOC_SET_REG:
		user_regs = (struct isp_regs_info*)arg;

		if (pipe_dev->isp_reg_mem[user_regs->mem_index].config) {
			if (pipe_dev->isp_reg_mem[user_regs->mem_index].mem.fd != user_regs->mem.fd) {
				isp_log_err("please use deploy isp driver reg memory(fd:%d), your's %d!",
					pipe_dev->isp_reg_mem[user_regs->mem_index].mem.fd, user_regs->mem.fd);
				ret = -EINVAL;
			} else {
				ret = x1isp_reg_write_brust(user_regs->data, user_regs->size, true,
						pipe_dev->isp_reg_mem[user_regs->mem_index].kvir_addr);
			}
		} else {
			isp_log_err("please deploy isp driver first, index:%d!", user_regs->mem_index);
			ret = -EPERM;
		}
		break;
	case ISP_IOC_GET_REG:
		ret =  x1isp_reg_read_brust((struct isp_regs_info*)arg);
		break;
	case ISP_IOC_SET_PDC:
		ret = x1isp_pipe_enable_pdc_af(pipe_dev, (u32*)arg);
		break;
	case ISP_IOC_SET_JOB:
		ret = x1isp_pipe_notify_jobs(pipe_dev, (struct isp_job_describer*)arg);
		break;
	case ISP_IOC_GET_INTERRUPT:
		ret = x1isp_pipe_wait_interrupts(pipe_dev, (struct isp_user_task_info*)arg);
		break;
	case ISP_IOC_REQUEST_BUFFER:
		ret = x1isp_pipe_request_stats_buffer(pipe_dev, (struct isp_buffer_request_info*)arg);
		break;
	case ISP_IOC_ENQUEUE_BUFFER:
		ret = x1isp_pipe_enqueue_stats_buffer(pipe_dev, (struct isp_buffer_enqueue_info*)arg);
		break;
	case ISP_IOC_FLUSH_BUFFER:
		ret = x1isp_pipe_flush_stats_buffer(pipe_dev);
		break;
	case ISP_IOC_TRIGGER_CAPTURE:
		ret = x1isp_pipe_trigger_capture(pipe_dev, (struct isp_capture_package*)arg);
		break;
	case ISP_IOC_SET_SINGLE_REG:
		ret = x1isp_reg_write_single((struct isp_reg_unit*)arg);
		break;
	case ISP_IOC_SET_END_FRAME_WORK:
		ret = x1isp_pipe_set_endframe_work(pipe_dev, ((struct isp_endframe_work_info*)arg));
		break;
	default:
		isp_log_err("unsupport the cmd: %d!", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	if (ret)
		isp_log_dbg("the cmd: %d! ioctl failed, ret=%d!", cmd, ret);

	return ret;
}

static long x1isp_pipe_unlocked_ioctl(struct file *file, unsigned int cmd,
				       unsigned long arg)
{
	long ret = 0;

	ret = x1isp_dev_copy_user(file, cmd, (void *)arg, x1isp_pipe_ioctl_core);
	return ret;
}

//fixme: add compat in the future
#if 0
//#ifdef CONFIG_COMPAT

struct isp_regs_info32 {
	union {
		__u64 phy_addr;
		__s32 fd;
	} mem;
	__u32 size;
	compat_caddr_t data;	/* contains some isp_reg_unit */
	__u32 mem_index;
};

struct isp_slice_regs32 {
	__u32 reg_count;
	compat_caddr_t data;	/* contains some isp_reg_unit */
};

struct isp_capture_slice_pack32 {
	__s32 slice_width;
	__s32 raw_read_offset;
	__s32 yuv_out_offset;
	__s32 dwt_offset[4];
	struct isp_slice_regs32 slice_reg;
};

struct isp_capture_package32 {
	__u32 slice_count;
	struct isp_capture_slice_pack32 capture_slice_packs[X1ISP_SLICE_MAX_NUM];
};

#define ISP_IOC_SET_REG32 _IOW(IOC_X1_ISP_TYPE, ISP_IOC_NR_SET_REG, struct isp_regs_info32)
#define ISP_IOC_GET_REG32 _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_GET_REG, struct isp_regs_info32)
#define ISP_IOC_TRIGGER_CAPTURE32 _IOWR(IOC_X1_ISP_TYPE, ISP_IOC_NR_TRIGGER_CAPTURE, struct isp_capture_package32)

/* Use the same argument order as copy_in_user */
#define assign_in_user(to, from)					\
({									\
	typeof(*from) __assign_tmp;					\
									\
	get_user(__assign_tmp, from) || put_user(__assign_tmp, to);	\
})

static int x1isp_alloc_userspace(unsigned int size, u32 aux_space,
				  void __user **up_native)
{
	*up_native = compat_alloc_user_space(size + aux_space);
	if (!*up_native)
		return -ENOMEM;
	if (clear_user(*up_native, size))
		return -EFAULT;
	return 0;
}

static int x1isp_get_user_regs(struct isp_regs_info __user *kp,
				struct isp_regs_info32 __user *up)
{
	compat_uptr_t tmp;

	if (!access_ok(up, sizeof(struct isp_regs_info32)) ||
	    assign_in_user(&kp->size, &up->size) ||
	    assign_in_user(&kp->mem_index, &up->mem_index) ||
	    get_user(tmp, &up->data) ||
	    put_user(compat_ptr(tmp), &kp->data) ||
	    copy_in_user(&kp->mem, &up->mem, sizeof(kp->mem)))
		return -EFAULT;

	return 0;
}

static int x1isp_put_user_regs(struct isp_regs_info __user *kp,
				struct isp_regs_info32 __user *up)
{
	void *edid;

	if (!access_ok(up, sizeof(*up)) ||
	    assign_in_user(&up->size, &kp->size) ||
	    assign_in_user(&up->mem_index, &kp->mem_index) ||
	    get_user(edid, &kp->data) ||
	    put_user(ptr_to_compat(edid), &up->data) ||
	    copy_in_user(&up->mem, &kp->mem, sizeof(kp->mem)))
		return -EFAULT;

	return 0;
}

static int x1isp_get_user_capture_package(struct isp_capture_package __user *kp,
					   struct isp_capture_package32 __user *up)
{
	compat_uptr_t tmp;
	__u32 i, count;

	if (!access_ok(up, sizeof(struct isp_capture_package32)) ||
	    assign_in_user(&kp->slice_count, &up->slice_count) ||
	    get_user(count, &up->slice_count))
		return -EFAULT;

	for (i = 0; i < count; i++) {
		if (assign_in_user(&kp->capture_slice_packs[i].slice_width, &up->capture_slice_packs[i].slice_width) ||
			assign_in_user(&kp->capture_slice_packs[i].raw_read_offset, &up->capture_slice_packs[i].raw_read_offset) ||
			assign_in_user(&kp->capture_slice_packs[i].yuv_out_offset, &up->capture_slice_packs[i].yuv_out_offset) ||
			copy_in_user(&kp->capture_slice_packs[i].dwt_offset, &up->capture_slice_packs[i].dwt_offset,
				sizeof(kp->capture_slice_packs[i].dwt_offset)))
			return -EFAULT;

		if (assign_in_user(&kp->capture_slice_packs[i].slice_reg.reg_count, &up->capture_slice_packs[i].slice_reg.reg_count) ||
			get_user(tmp, &up->capture_slice_packs[i].slice_reg.data) ||
			put_user(compat_ptr(tmp), &kp->capture_slice_packs[i].slice_reg.data))
			return -EFAULT;
	}

	return 0;
}

static long x1isp_pipe_compat_ioctl(struct file *file, unsigned int cmd,
				     unsigned long arg)
{
	long ret = 0;
	void __user *up_native = NULL;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;

	/* maybe do some convention, like user space memeory or 64bit to 32bit */

	switch (cmd) {
	case ISP_IOC_SET_REG32:
		cmd = ISP_IOC_SET_REG;
		ret =
		    x1isp_alloc_userspace(sizeof(struct isp_regs_info), 0, &up_native);
		if (!ret)
			x1isp_get_user_regs(up_native, up);
		else
			isp_log_err("alloc userspace for ISP_IOC_SET_REG failed!");
		compatible_arg = 0;
		break;
	case ISP_IOC_GET_REG32:
		cmd = ISP_IOC_GET_REG;
		ret =
		    x1isp_alloc_userspace(sizeof(struct isp_regs_info), 0, &up_native);
		if (!ret)
			x1isp_get_user_regs(up_native, up);
		else
			isp_log_err("alloc userspace for ISP_IOC_GET_REG failed!");
		compatible_arg = 0;
		break;
	case ISP_IOC_TRIGGER_CAPTURE32:
		cmd = ISP_IOC_TRIGGER_CAPTURE;
		ret =
		    x1isp_alloc_userspace(sizeof(struct isp_capture_package), 0,
					   &up_native);
		if (!ret)
			x1isp_get_user_capture_package(up_native, up);
		else
			isp_log_err
			    ("alloc userspace for ISP_IOC_TRIGGER_CAPTURE failed!");
		compatible_arg = 0;
		break;
	case ISP_IOC_DEPLOY_DRV:
	case ISP_IOC_UNDEPLOY_DRV:
	case ISP_IOC_SET_PDC:
	case ISP_IOC_SET_JOB:
	case ISP_IOC_GET_INTERRUPT:
	case ISP_IOC_REQUEST_BUFFER:
	case ISP_IOC_ENQUEUE_BUFFER:
	case ISP_IOC_FLUSH_BUFFER:
	case ISP_IOC_SET_SINGLE_REG:
	case ISP_IOC_SET_END_FRAME_WORK:
		break;
	default:
		isp_log_err("unsupport the cmd: %d!", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	if (ret)
		return ret;

	if (compatible_arg)
		ret = x1isp_dev_copy_user(file, cmd, up, x1isp_pipe_ioctl_core);
	else
		ret = x1isp_dev_copy_user(file, cmd, up_native, x1isp_pipe_ioctl_core);

	if (ret) {
		isp_log_err("x1_isp_copy_user for cmd 0x%x failed!", cmd);
		goto ERROR_EXIT;
	}
	// TODO:if need to copy to user space pointer, should do more things.
	switch (cmd) {
	case ISP_IOC_GET_REG:
		ret = x1isp_put_user_regs(up_native, up);
		if (ret)
			isp_log_err("x1_isp_put_user_regs for cmd 0x%x failed!", cmd);
		break;
	}

ERROR_EXIT:
	return ret;
}

#endif

static struct file_operations g_isp_pipe_fops = {
	.owner = THIS_MODULE,
	.open = x1isp_pipe_open,
	.release = x1isp_pipe_release,
	.unlocked_ioctl = x1isp_pipe_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	//fixme: add compat in the future
	//.compat_ioctl   = x1isp_pipe_compat_ioctl,
#endif
};

struct file_operations *x1isp_pipe_get_fops(void)
{
	return &g_isp_pipe_fops;
}

static int isp_pipe_task_wakeup_user(struct x1isp_pipe_dev *pipe_dev,
			      struct isp_pipe_task *pipe_task, u32 frame_num)
{
	static DEFINE_RATELIMIT_STATE(rs, 5 * HZ, 6);
	u32 last_num = 0;

	spin_lock(&pipe_task->complete_lock);	//lock used only between thread and soft irq.
	pipe_task->complete_cnt++;
	last_num = pipe_task->complete_frame_num;
	pipe_task->complete_frame_num = frame_num;
	if (pipe_task->complete_cnt >= 2) {
		//user task may delay, needn't wakeup this time.
		if (__ratelimit(&rs))
			isp_log_warn("%s:task(%d) delay at frame%d,now frame%d !",
				     __func__, pipe_task->task_type, last_num,
				     frame_num);

		pipe_task->complete_cnt--;
		spin_unlock(&pipe_task->complete_lock);
		//return the previous buffer because they have no chance to deal.
		isp_pipe_task_get_stats_result(pipe_dev, pipe_task, NULL, frame_num - 1, true);
	} else {
		complete(&pipe_task->wait_complete);
		spin_unlock(&pipe_task->complete_lock);
	}

	return 0;
}

static void isp_pipe_task_ignore_a_vote(struct x1isp_stats_node *stats_node, u32 voter_type,
				 u32 frame_num)
{
	u32 stat_id = 0;

	if (TASK_VOTER_AF_EOF == voter_type)
		stat_id = ISP_STAT_ID_AF;
	else if (TASK_VOTER_PDC_EOF == voter_type)
		stat_id = ISP_STAT_ID_PDC;

	x1isp_stat_get_donebuf_by_frameid(stats_node, stat_id, frame_num - 1, true);
}

int isp_pipe_task_vote_handler(struct x1isp_pipe_dev *pipe_dev,
			       struct isp_pipe_task *pipe_task, u32 frame_num,
			       u32 voter_type)
{
	int ret = 0, vote_index, system_trigger = 0, i, j;
	struct task_voting_system *voting_sys = NULL;
	static DEFINE_RATELIMIT_STATE(limt_print, 5 * HZ, 3);
	u32 ticket_frame = 0, fail_vote = 0;

	voting_sys = &pipe_task->vote_system;
	ISP_DRV_CHECK_POINTER(voting_sys);
	ISP_DRV_CHECK_MAX_PARAMETERS(voter_type, TASK_VOTER_PDC_EOF, "voter_type");

	if (false == pipe_task->use_vote_sys) {
		//user disable vote system.
		pr_err_ratelimited("user disable vote system, ignore this result!");
		//frame_num+1:discard the current frame.
		isp_pipe_task_ignore_a_vote(&pipe_dev->stats_nodes[0], voter_type,
					    frame_num + 1);
		//clear flag
		spin_lock(&voting_sys->vote_lock);
		voting_sys->cur_ticket_cnt = 0;
		for (i = 0; i < voting_sys->sys_trigger_num; i++)
			voting_sys->voter_validity[i] = false;
		spin_unlock(&voting_sys->vote_lock);
		return ret;
	}

	vote_index = voting_sys->voter_index[voter_type];
	if (vote_index < 0) {
		isp_log_err("valide index(%d) for task voter(%d)!", vote_index, voter_type);
		return -EPERM;
	}

	if (true == voting_sys->voter_validity[vote_index]) {
		if (__ratelimit(&limt_print)) {
			isp_log_warn
			    ("voter(%d)'s previous isn't clear at frame%d, pdc's status:0x%lx!",
			     voter_type, frame_num,
			     x1isp_reg_readl(REG_ISP_PDC_BASE(pipe_dev->pipedev_id)));
		}
		spin_lock(&voting_sys->vote_lock);
		voting_sys->voter_frameID[vote_index] = frame_num;
		spin_unlock(&voting_sys->vote_lock);
		isp_pipe_task_ignore_a_vote(&pipe_dev->stats_nodes[0], voter_type, frame_num);
	} else {
		spin_lock(&voting_sys->vote_lock);
		voting_sys->voter_validity[vote_index] = true;
		voting_sys->voter_frameID[vote_index] = frame_num;
		voting_sys->cur_ticket_cnt++;
		if (voting_sys->cur_ticket_cnt == voting_sys->sys_trigger_num) {
			ticket_frame = voting_sys->voter_frameID[0];
			for (i = 1; i < voting_sys->sys_trigger_num; i++) {
				if (ticket_frame < voting_sys->voter_frameID[i]) {
					voting_sys->voter_validity[i - 1] = false;
					fail_vote = true;
				} else if (ticket_frame > voting_sys->voter_frameID[i]) {
					voting_sys->voter_validity[i] = false;
					fail_vote = true;
				}
				ticket_frame = voting_sys->voter_frameID[i];
			}

			//trigger system
			if (fail_vote)
				voting_sys->cur_ticket_cnt--;
			else {
				system_trigger = 1;
				//clear flag
				voting_sys->cur_ticket_cnt = 0;
				for (i = 0; i < voting_sys->sys_trigger_num; i++)
					voting_sys->voter_validity[i] = false;
			}
		}
		spin_unlock(&voting_sys->vote_lock);
	}

	if (system_trigger)
		isp_pipe_task_wakeup_user(pipe_dev, pipe_task, frame_num);

	if (fail_vote) {
		for (i = 0; i < voting_sys->sys_trigger_num; i++) {
			if (voting_sys->voter_validity[i] == false) {
				for (j = TASK_VOTER_SDE_SOF; j < TASK_VOTER_TYP_MAX; j++) {
					if (i == voting_sys->voter_index[j])
						break;
				}
				//discard current frame
				isp_pipe_task_ignore_a_vote(&pipe_dev->stats_nodes[0],
							    j,
							    voting_sys->voter_frameID[i] + 1);
			}
		}
	}

	return ret;
}

//tasklet context
static int isp_pipe_task_mem_stat_handler(struct x1isp_pipe_dev *pipe_dev, u32 task_type, u32 frame_num)
{
	int i = 0, ret = 0;

	ISP_DRV_CHECK_POINTER(pipe_dev);

	for (i = 0; i < pipe_dev->stats_node_cnt; i++) {
		if (task_type == ISP_PIPE_TASK_TYPE_SOF) {
			x1isp_stat_mem_lower_half_irq(&pipe_dev->stats_nodes[i], ISP_STAT_ID_AWB, frame_num);
			x1isp_stat_mem_lower_half_irq(&pipe_dev->stats_nodes[i], ISP_STAT_ID_LTM, frame_num);
			if (pipe_dev->eof_task_hd_by_sof)
				x1isp_stat_mem_lower_half_irq(&pipe_dev->stats_nodes[i], ISP_STAT_ID_AE, frame_num);
		} else if (task_type == ISP_PIPE_TASK_TYPE_EOF) {
			x1isp_stat_mem_lower_half_irq(&pipe_dev->stats_nodes[i], ISP_STAT_ID_AE, frame_num);
		} else if (task_type == ISP_PIPE_TASK_TYPE_AF) {
			x1isp_stat_mem_lower_half_irq(&pipe_dev->stats_nodes[i], ISP_STAT_ID_AF, frame_num);
		}
	}

	return ret;
}

static void isp_pipe_tasklet_handler(unsigned long data)
{
	struct x1isp_pipe_dev *pipe_dev = (struct x1isp_pipe_dev *)data;
	u8 trigger_task = false;
	int type_index = 0, frame_num = 0, i;
	struct x1isp_irq_context *isp_irq_ctx = NULL;
	struct isp_pipe_task *pipe_task = NULL;
	unsigned long flags = 0;

	if (!pipe_dev) {
		isp_log_err("%s: invalid pointer!", __func__);
		return;
	}

	if (ISP_PIPE_WORK_TYPE_PREVIEW != pipe_dev->work_type) {
		pr_err_ratelimited("isp pipe tasklet triggered by none-preview work!");
		return;
	}

	if (pipe_dev->stream_restart) {
		for (i = 0; i < pipe_dev->stats_node_cnt; i++)
			x1isp_stat_reset_dma_busybuf_frameid(&pipe_dev->stats_nodes[i]);
		pipe_dev->stream_restart = false;
	}
	isp_irq_ctx = &pipe_dev->isp_irq_ctx;

	//handle af and ae first duing to their done may come together with next sof
	for (type_index = ISP_PIPE_TASK_TYPE_AF; type_index >= 0; type_index--) {
		pipe_task = &pipe_dev->pipe_tasks[type_index];
		frame_num = atomic_read(&pipe_task->frame_num);
		spin_lock_irqsave(&pipe_task->task_lock, flags);
		trigger_task = pipe_task->task_trigger;
		if (trigger_task == true)
			pipe_task->task_trigger = false;
		spin_unlock_irqrestore(&pipe_task->task_lock, flags);
		if (trigger_task) {
			isp_pipe_task_mem_stat_handler(pipe_dev, type_index, frame_num);
			//wakeup user

			if (pipe_task->use_vote_sys) {
				if (ISP_PIPE_TASK_TYPE_AF != type_index) {
					isp_log_err("%s: Only AF task use voting system", __func__);
					return;
				}
				isp_pipe_task_vote_handler(pipe_dev, pipe_task, frame_num, TASK_VOTER_AF_EOF);
			} else {
				isp_pipe_task_wakeup_user(pipe_dev, pipe_task, frame_num);
			}
		}
	}
}

//the upper half irq context
static void isp_pipe_dev_call_each_irqbit_handler(u32 irq_status, u32 hw_pipe_id, u32 frame_id)
{
	struct x1isp_pipe_dev *pipe_dev = NULL;
	struct x1isp_irq_context *isp_irq_ctx = NULL;
	int set_bit = -1, schedule_lower_irq = 0;
	struct isp_irq_func_params irq_func_param;

	x1isp_dev_get_pipedev(hw_pipe_id, &pipe_dev);
	if (!pipe_dev) {
		isp_log_err("Can't find work pipe device for hw pipeline%d!", hw_pipe_id);
		return;
	}

	if (pipe_dev->work_status != ISP_WORK_STATUS_START) {
		pr_err_ratelimited("irq(fn%d) with isp pipedev(%d) doesn't start work!",
				   frame_id, hw_pipe_id);
		return;
	}

	isp_irq_ctx = &pipe_dev->isp_irq_ctx;
	irq_func_param.pipe_dev = pipe_dev;
	irq_func_param.frame_num = frame_id;
	irq_func_param.irq_status = irq_status;
	irq_func_param.hw_pipe_id = hw_pipe_id;
	for_each_set_bit(set_bit, &isp_irq_ctx->isp_irq_bitmap, ISP_IRQ_BIT_MAX_NUM) {
		if (irq_status & BIT(set_bit)) {
			if (isp_irq_ctx->isp_irq_handler[set_bit])
				schedule_lower_irq |= isp_irq_ctx->isp_irq_handler[set_bit](&irq_func_param);
		}
	}

	if (schedule_lower_irq)
		tasklet_hi_schedule(&isp_irq_ctx->isp_irq_tasklet);
}

//the upper half irq context
void x1isp_pipe_dev_irq_handler(void *irq_data)
{
	struct isp_irq_data *isp_irq = (struct isp_irq_data *)irq_data;

	if (isp_irq->pipe0_irq_status)
		isp_pipe_dev_call_each_irqbit_handler(isp_irq->pipe0_irq_status,
						      ISP_HW_PIPELINE_ID_0,
						      isp_irq->pipe0_frame_id);

	if (isp_irq->pipe1_irq_status)
		isp_pipe_dev_call_each_irqbit_handler(isp_irq->pipe1_irq_status,
						      ISP_HW_PIPELINE_ID_1,
						      isp_irq->pipe1_frame_id);
}

//the upper half irq context
void x1isp_pipe_dma_irq_handler(struct x1isp_pipe_dev *pipe_dev, void *irq_data)
{
	int i = 0;
	u32 schedule_lower_irq = 0;
	struct x1isp_irq_context *isp_irq_ctx = NULL;

	if (pipe_dev) {
		if (ISP_WORK_STATUS_START != pipe_dev->work_status)
			return;

		for (i = 0; i < pipe_dev->stats_node_cnt; i++) // max is 2.
			schedule_lower_irq |= x1isp_stat_dma_irq_handler(&pipe_dev->stats_nodes[i], irq_data);

		if (schedule_lower_irq) {
			isp_irq_ctx = &pipe_dev->isp_irq_ctx;
			tasklet_hi_schedule(&isp_irq_ctx->isp_dma_irq_tasklet);
		}
	}
}

static void isp_dma_tasklet_handler(unsigned long data)
{
	int i = 0;
	struct x1isp_pipe_dev *pipe_dev = (struct x1isp_pipe_dev *)data;
	u32 frame_id = 0;

	if (!pipe_dev) {
		isp_log_err("Invalid pipe dev pointer for isp dma tasklet!");
		return;
	}

	frame_id = atomic_read(&pipe_dev->isp_irq_ctx.cur_frame_num);
	for (i = 0; i < pipe_dev->stats_node_cnt; i++)
		x1isp_stat_dma_lower_half_irq(&pipe_dev->stats_nodes[i], frame_id);
}

//pipe sof irq handler:get frame id from vi
int isp_pipe_sof_irq_handler(struct isp_irq_func_params *param)
{
	int schedule_lower_irq = 0;
	struct x1isp_pipe_dev *pipe_dev = NULL;
	struct x1isp_irq_context *irq_ctx = NULL;
	u32 hw_pipe_id = 0, last_frame_num = 0;

	ISP_DRV_CHECK_POINTER(param);
	pipe_dev = param->pipe_dev;
	if (ISP_PIPE_WORK_TYPE_PREVIEW != pipe_dev->work_type)
		return 0;
	irq_ctx = &pipe_dev->isp_irq_ctx;
	last_frame_num = atomic_read(&irq_ctx->cur_frame_num);
	atomic_set(&irq_ctx->cur_frame_num, param->frame_num);

	if (last_frame_num != 0 && (last_frame_num != param->frame_num - 1)) {
		hw_pipe_id = param->hw_pipe_id;
		if (0 == param->frame_num) {	//stream restart and frameid comes from zero.
			pipe_dev->stream_restart = true;
			schedule_lower_irq = 1;
		} else {
			pr_err_ratelimited ("the frameID is not serial on pipe%d,%d to %d!",
					    hw_pipe_id, last_frame_num, param->frame_num);
		}
	}

	return schedule_lower_irq;
}

//just print err log.
int isp_pipe_irq_err_print_handler(struct isp_irq_func_params *param)
{
	int schedule_lower_irq = 0;
	u32 irq_value = 0, hw_pipe_id = 0;

	ISP_DRV_CHECK_POINTER(param);
	irq_value = param->irq_status;
	hw_pipe_id = param->hw_pipe_id;

	// if (irq_value & BIT(ISP_IRQ_BIT_STAT_ERR))
	//      isp_log_err("host_isp_statistics_err in pipe:%d!", hw_pipe_id);

	if (irq_value & BIT(ISP_IRQ_BIT_ISP_ERR))
		isp_log_err("host_isp_err_irq in pipe%d!", hw_pipe_id);

	return schedule_lower_irq;
}

int isp_pipe_sde_sof_irq_handler(struct isp_irq_func_params *param)
{
	int schedule_lower_irq = 1, frame_num = 0;
	struct x1isp_pipe_dev *pipe_dev = NULL;
	struct isp_pipe_task *pipe_task = NULL;
	int i = 0;

	ISP_DRV_CHECK_POINTER(param);
	pipe_dev = param->pipe_dev;
	frame_num = atomic_read(&pipe_dev->isp_irq_ctx.cur_frame_num);
	pipe_task = &pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_SOF];
	atomic_set(&pipe_task->frame_num, frame_num);
	spin_lock(&pipe_task->task_lock);
	// use_frame_id = pipe_task->user_stat_cfg.frame_num;
	pipe_task->task_trigger = true;
	spin_unlock(&pipe_task->task_lock);

	//get awb ltm read sof
	for (i = 0; i < pipe_dev->stats_node_cnt; i++) {
		x1isp_stat_mem_set_irq_flag(&pipe_dev->stats_nodes[i], ISP_STAT_ID_AWB,
					     param->hw_pipe_id);
		x1isp_stat_mem_set_irq_flag(&pipe_dev->stats_nodes[i], ISP_STAT_ID_LTM,
					     param->hw_pipe_id);
		if (pipe_dev->eof_task_hd_by_sof)
			x1isp_stat_mem_set_irq_flag(&pipe_dev->stats_nodes[i],
						     ISP_STAT_ID_AE, param->hw_pipe_id);
	}

	return schedule_lower_irq;
}

int isp_pipe_sde_eof_irq_handler(struct isp_irq_func_params *param)
{
	int schedule_lower_irq = 0;

	return schedule_lower_irq;
}

int isp_pipe_aem_eof_irq_handler(struct isp_irq_func_params *param)
{
	int schedule_lower_irq = 1, frame_num = 0;
	struct x1isp_pipe_dev *pipe_dev = NULL;
	struct isp_pipe_task *pipe_task = NULL;

	ISP_DRV_CHECK_POINTER(param);
	pipe_dev = param->pipe_dev;
	frame_num = atomic_read(&pipe_dev->isp_irq_ctx.cur_frame_num);
	pipe_task = &pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_EOF];
	atomic_set(&pipe_task->frame_num, frame_num);

	if (pipe_dev->stats_node_cnt == 1)
		x1isp_stat_mem_set_irq_flag(pipe_dev->stats_nodes, ISP_STAT_ID_AE, param->hw_pipe_id);
	// else {
	//      //combine pipe
	//      x1isp_stat_mem_set_irq_flag(&pipe_dev->stats_nodes[param->hw_pipe_id], ISP_STAT_ID_AF, param->hw_pipe_id);
	// }

	spin_lock(&pipe_task->task_lock);
	pipe_task->task_trigger = true;
	spin_unlock(&pipe_task->task_lock);

	return schedule_lower_irq;
}

int isp_pipe_afc_eof_irq_handler(struct isp_irq_func_params *param)
{
	int schedule_lower_irq = 1, frame_num = 0;
	struct x1isp_pipe_dev *pipe_dev = NULL;
	struct isp_pipe_task *pipe_task = NULL;

	ISP_DRV_CHECK_POINTER(param);
	pipe_dev = param->pipe_dev;
	frame_num = atomic_read(&pipe_dev->isp_irq_ctx.cur_frame_num);
	pipe_task = &pipe_dev->pipe_tasks[ISP_PIPE_TASK_TYPE_AF];
	atomic_set(&pipe_task->frame_num, frame_num);

	if (pipe_dev->stats_node_cnt == 1)
		x1isp_stat_mem_set_irq_flag(pipe_dev->stats_nodes, ISP_STAT_ID_AF, param->hw_pipe_id);
	// else {
	//      //combine pipe
	//      x1isp_stat_mem_set_irq_flag(&pipe_dev->stats_nodes[param->hw_pipe_id], ISP_STAT_ID_AF, param->hw_pipe_id);
	// }

	spin_lock(&pipe_task->task_lock);
	pipe_task->task_trigger = true;
	spin_unlock(&pipe_task->task_lock);

	return schedule_lower_irq;
}

static int isp_pipe_irq_ctx_constructed(struct x1isp_pipe_dev *pipe_dev, u32 pipedev_id)
{
	int ret = 0, i = 0, irq_bit = 0;
	u32 hw_pipe_id = 0;
	struct x1isp_irq_context *irq_ctx = NULL;

	PIPE_DEVID_TO_HW_PIPELINE_ID(pipedev_id, hw_pipe_id);
	irq_ctx = &pipe_dev->isp_irq_ctx;
	irq_ctx->hw_pipe_id = hw_pipe_id;

	//define isp irq we need.
	bitmap_zero(&irq_ctx->isp_irq_bitmap, ISP_IRQ_BIT_MAX_NUM);
	memset(irq_ctx->isp_irq_handler, 0, sizeof(irq_ctx->isp_irq_handler));

	for (i = 0; i < ISP_DRV_ARRAY_LENGTH(g_host_irq_handler_infos); i++) {
		irq_bit = g_host_irq_handler_infos[i].irq_bit;
		bitmap_set(&irq_ctx->isp_irq_bitmap, irq_bit, 1);
		irq_ctx->isp_irq_handler[irq_bit] =
		    g_host_irq_handler_infos[i].irq_handler;
	}

	//init tasklet for the bottom of interrupt handler.
	tasklet_init(&irq_ctx->isp_irq_tasklet, isp_pipe_tasklet_handler,
		     (unsigned long)pipe_dev);
	tasklet_init(&irq_ctx->isp_dma_irq_tasklet, isp_dma_tasklet_handler,
		     (unsigned long)pipe_dev);

	atomic_set(&irq_ctx->cur_frame_num, 0);
	isp_log_dbg("construct pipe irq_ctx, bitmap=0x%lx", irq_ctx->isp_irq_bitmap);
	return ret;
}

static int isp_pipe_dev_stats_node_create(struct x1isp_pipe_dev *pipe_dev)
{
	int ret = -1, count = 1;

	if (pipe_dev) {
		// if (pipe_dev->pipedev_id > ISP_PIPE_DEV_ID_1 && pipe_dev->pipedev_id < ISP_PIPE_DEV_ID_MAX) {
		//      count = ISP_HW_PIPELINE_ID_MAX;
		// }

		pipe_dev->stats_nodes =
		    kzalloc(sizeof(struct x1isp_stats_node) * count, GFP_KERNEL);
		if (unlikely(pipe_dev->stats_nodes == NULL)) {
			isp_log_err("could not allocate memory for isp stats node!");
			return -ENOMEM;
		}

		pipe_dev->stats_nodes->private_dev = (void *)pipe_dev;
		pipe_dev->stats_nodes->notify_event = x1isp_pipe_notify_event;
		pipe_dev->stats_node_cnt = count;
		ret = 0;
	}

	return ret;
}

static int isp_pipe_task_constructed(struct isp_pipe_task *pipe_tasks)
{
	int ret = 0, task_type, i = 0;

	for (task_type = ISP_PIPE_TASK_TYPE_SOF; task_type < ISP_PIPE_TASK_TYPE_MAX; task_type++) {
		pipe_tasks[task_type].task_type = task_type;

		memset(&pipe_tasks[task_type].vote_system, 0, sizeof(struct task_voting_system));
		for (i = 0; i < TASK_VOTER_TYP_MAX; i++)
			pipe_tasks[task_type].vote_system.voter_index[i] = -1;

		if (ISP_PIPE_TASK_TYPE_AF == task_type) {
			pipe_tasks[task_type].vote_system.sys_trigger_num = 2; //af eof and pdc eof
			pipe_tasks[task_type].vote_system.voter_index[TASK_VOTER_AF_EOF] = 0;
			pipe_tasks[task_type].vote_system.voter_index[TASK_VOTER_PDC_EOF] = 1;
			spin_lock_init(&pipe_tasks[task_type].vote_system.vote_lock);
		}
		//define stat bitmap for task.
		bitmap_zero(&pipe_tasks[task_type].stat_bitmap, ISP_STAT_ID_MAX);
		for (i = 0; i < g_task_stat_map_infos[task_type].count; i++) {
			bitmap_set(&pipe_tasks[task_type].stat_bitmap,
				   g_task_stat_map_infos[task_type].stat_ids[i], 1);
		}
		pipe_tasks[task_type].stat_bits_cnt = g_task_stat_map_infos[task_type].count;

		// memset(&pipe_tasks[task_type].user_stat_cfg, 0, sizeof(struct pipe_task_stat_config));

		init_completion(&pipe_tasks[task_type].wait_complete);
		spin_lock_init(&pipe_tasks[task_type].task_lock);
		spin_lock_init(&pipe_tasks[task_type].complete_lock);
	}

	return ret;
}

static void isp_pipe_task_exit(struct isp_pipe_task *pipe_tasks)
{
	if (pipe_tasks)
		memset(pipe_tasks, 0, sizeof(struct isp_pipe_task) * ISP_PIPE_TASK_TYPE_MAX);
}

int x1isp_pipe_dev_init(struct platform_device *pdev,
			 struct x1isp_pipe_dev *isp_pipe_dev[])
{
	int ret = 0, i = 0, j = 0;
	struct x1isp_pipe_dev *pipe_dev = NULL;
	u32 hw_pipe_id = 0;

	for (i = ISP_PIPE_DEV_ID_0; i < ISP_PIPE_DEV_ID_MAX; i++) {
		pipe_dev =
		    devm_kzalloc(&pdev->dev, sizeof(struct x1isp_pipe_dev), GFP_KERNEL);
		if (unlikely(pipe_dev == NULL)) {
			dev_err(&pdev->dev, "could not allocate memory");
			ret = -ENOMEM;
			return ret;
		}

		pipe_dev->dev_num = 0;
		pipe_dev->pipedev_id = i;
		isp_pipe_task_constructed(pipe_dev->pipe_tasks);
		for (j = ISP_PIPE_TASK_TYPE_SOF; j < ISP_PIPE_TASK_TYPE_MAX; j++)
			isp_pipe_task_job_init(&pipe_dev->pipe_tasks[j]);
		isp_pipe_irq_ctx_constructed(pipe_dev, i);

		//isp stats node create and init.
		ret = isp_pipe_dev_stats_node_create(pipe_dev);
		if (ret)
			return ret;

		PIPE_DEVID_TO_HW_PIPELINE_ID(i, hw_pipe_id);
		for (j = 0; j < pipe_dev->stats_node_cnt; j++) {
			if (j > 0)
				hw_pipe_id = ISP_HW_PIPELINE_ID_1;	//combine pipe, need stats of another pipeline
			x1isp_stat_node_init(&pipe_dev->stats_nodes[j], hw_pipe_id);
			x1isp_stat_job_flags_init(&pipe_dev->stats_nodes[j]);
		}

		mutex_init(&pipe_dev->isp_pipedev_mutex);
		mutex_init(&pipe_dev->pipedev_capture_mutex);
		isp_pipe_dev[i] = pipe_dev;
	}

	return ret;
}

static int isp_pipe_irq_ctx_exit(struct x1isp_irq_context *isp_irq_ctx)
{
	int ret = 0;

	bitmap_zero(&isp_irq_ctx->isp_irq_bitmap, ISP_IRQ_BIT_MAX_NUM);
	memset(isp_irq_ctx->isp_irq_handler, 0, sizeof(isp_irq_ctx->isp_irq_handler));
	tasklet_kill(&isp_irq_ctx->isp_irq_tasklet);
	tasklet_kill(&isp_irq_ctx->isp_dma_irq_tasklet);

	return ret;
}

int x1isp_pipe_dev_exit(struct platform_device *pdev,
			 struct x1isp_pipe_dev *isp_pipe_dev[])
{
	int ret = 0, i = 0;
	struct x1isp_pipe_dev *pipe_dev = NULL;

	for (i = ISP_PIPE_DEV_ID_0; i < ISP_PIPE_DEV_ID_MAX; i++) {
		pipe_dev = isp_pipe_dev[i];
		if (pipe_dev) {
			kfree((void *)pipe_dev->stats_nodes);
			pipe_dev->stats_nodes = NULL;
			isp_pipe_irq_ctx_exit(&pipe_dev->isp_irq_ctx);
			isp_pipe_task_exit(pipe_dev->pipe_tasks);
			devm_kfree(&pdev->dev, (void *)pipe_dev);
			mutex_destroy(&pipe_dev->isp_pipedev_mutex);
			mutex_destroy(&pipe_dev->pipedev_capture_mutex);
			isp_pipe_dev[i] = NULL;
		}
	}

	return ret;
}
