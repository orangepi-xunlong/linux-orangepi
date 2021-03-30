/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/kref.h>
#include "../sunxi_drm_core.h"
#include "../sunxi_drm_gem.h"
#include "../drm_de/drm_al.h"
#include "sunxi_common.h"
#include "transform.h"
#include "sunxi_rotate.h"
#include "drm/sunxi_drm.h"
#include "sunxi_drm_drv.h"

static irqreturn_t sunxi_rotate_interrupt(int irq, void *dev_id);
void sunxi_rotate_daemon(struct work_struct *work);
void sunxi_schedule_process(struct sunxi_rotate_private *rotate_private);

int sunxi_drm_rotate_init(void *dev_private)
{
	int ret;
	void __iomem *io_base;
	int irq;
	struct device_node *node;
	struct sunxi_drm_private *private;
	struct sunxi_rotate_private *rotate_private;

return 0;
	node = sunxi_drm_get_name_node("sun50i-tr");
	if(node == NULL) {
		DRM_ERROR("rotate get device node error.\n");
		return -EINVAL;
	}

	io_base = of_iomap(node, 0);
	if(io_base == NULL) {
		DRM_ERROR("rotate get addr error.\n");
		return -EINVAL;
	}

	irq = irq_of_parse_and_map(node, 0);
	if(irq == 0) {
		DRM_ERROR("rotate get irq error.\n");
		return -EINVAL;
	}

	private = (struct sunxi_drm_private *)dev_private;
	rotate_private = kzalloc(sizeof(struct sunxi_rotate_private), GFP_KERNEL);
	if(!rotate_private) {
		DRM_ERROR("rotate kzalloc private data error.\n");
		return -EINVAL;
	}
	rotate_private->clk = of_clk_get(node, 0);
	if (IS_ERR(rotate_private->clk)) {
		DRM_ERROR("rotate get clk error.\n");
		return -EINVAL;
	}
	rotate_private->irq = irq;
	INIT_LIST_HEAD(&rotate_private->rotate_work);
	INIT_LIST_HEAD(&rotate_private->qurey_head);
	mutex_init(&rotate_private->qurey_mlock);
	mutex_init(&rotate_private->user_mlock);
	spin_lock_init(&rotate_private->head_lock);
	spin_lock_init(&rotate_private->process_lock);
	init_waitqueue_head(&rotate_private->task_wq);
	rotate_private->used_cnt = 0;
	idr_init(&rotate_private->user_idr);
	rotate_private->current_task = NULL;

	de_tr_set_base((uintptr_t)io_base);
	ret = request_irq(irq, sunxi_rotate_interrupt, IRQF_SHARED,
	"drm_rotate", rotate_private);
	//tasklet_init(&rotate_private->rotate, sunxi_drm_soft_irq_handle, (unsigned long)(rotate_private));
	INIT_DELAYED_WORK(&rotate_private->daemon_work, sunxi_rotate_daemon);
	rotate_private->active =  ROTATE_IN_IDLE;
	private->rotate_private = rotate_private;

	return 0;
}

int sunxi_drm_rotate_destroy(struct sunxi_rotate_private *rotate_private)
{
	mutex_lock(&rotate_private->user_mlock);
	if(rotate_private->irq != 0)
		free_irq(rotate_private->irq, sunxi_rotate_interrupt);
	if(rotate_private->clk)
		clk_disable(rotate_private->clk);
	mutex_unlock(&rotate_private->qurey_mlock);
	kfree(rotate_private);
	return 0;
}

void rotate_task_free(struct kref *kref)
{
	struct sunxi_rotate_task *task;
	task = container_of(kref, struct sunxi_rotate_task, refcount);
	if(task)
		kfree(task);
}

void sunxi_rotate_daemon(struct work_struct *work)
{
	struct delayed_work *delay_work;
	struct sunxi_rotate_private *rotate_private;

	delay_work = to_delayed_work(work);
	rotate_private = to_sunxi_rotate_priv(delay_work);
	if (!rotate_private->current_task
	|| rotate_private->used_cnt == 0)
	return;

	spin_lock(&rotate_private->process_lock);
	if (rotate_private->active == ROTAE_IN_BAD) {
		de_tr_reset();
		/* becareful for  */
		rotate_private->active = ROTATE_IN_IDLE;
		if (rotate_private->current_task)
			rotate_private->current_task->status = ROTATE_ERROR;
		spin_unlock(&rotate_private->process_lock);
		sunxi_schedule_process(rotate_private);
		return;
	}

	if (rotate_private->active == ROTATE_IN_WORK) {
		rotate_private->active = ROTAE_IN_BAD;
		spin_unlock(&rotate_private->process_lock);

		schedule_delayed_work(delay_work, ROTATE_DAEMON_HZ);
		return;
	}
	spin_unlock(&rotate_private->process_lock);

}

void sunxi_schedule_process(struct sunxi_rotate_private *rotate_private)
{
	struct sunxi_rotate_task *task_work = NULL;

	spin_lock(&rotate_private->process_lock);
	if (rotate_private->active > ROTATE_IN_IDLE) {
		spin_unlock(&rotate_private->process_lock);
		return;
	}
	rotate_private->active = ROTATE_IN_WORK;
	spin_unlock(&rotate_private->process_lock);

	cancel_delayed_work(&rotate_private->daemon_work);
	spin_lock(&rotate_private->head_lock);
	if (rotate_private->rotate_work.next != &rotate_private->rotate_work) {
		task_work = to_sunxi_rotate_w(rotate_private->rotate_work.next);
		list_del(&task_work->w_head);
		kref_get(&task_work->refcount);
		rotate_private->current_task = task_work;
	}
	spin_unlock(&rotate_private->head_lock);

	if(task_work) {
		task_work->status = ROTATE_BUSY;
		de_tr_set_cfg(&task_work->info);
	}else{
		rotate_private->active = ROTATE_IN_IDLE;
	}
}

static irqreturn_t sunxi_rotate_interrupt(int irq, void *dev_id)
{
	int ret = 0;
	struct sunxi_rotate_private *rotate_private;
	rotate_private = (struct sunxi_rotate_private *)dev_id;

	ret = de_tr_irq_query();
	if(0 == ret) {
		rotate_private->active = ROTATE_IN_FINISH;
		if (rotate_private->current_task) {
			rotate_private->current_task->status = ROTATE_OK;
			DRM_DEBUG_KMS("int rotate irq  %p\n", rotate_private->current_task);
			if(rotate_private->current_task->sleep_mode) {
				wake_up_all(&rotate_private->task_wq);
			}
			kref_put(&rotate_private->current_task->refcount, rotate_task_free);
			rotate_private->current_task = NULL;
		}
		rotate_private->active = ROTATE_IN_IDLE;
		sunxi_schedule_process(rotate_private);
	}

	return IRQ_HANDLED;
}

static inline int sunxi_check_access(struct sunxi_rotate_private *rotate_private,
        struct sunxi_rotate_cmd *rotate_cmd, struct drm_file *file_priv)
{
	void *handle;
	mutex_lock(&rotate_private->user_mlock);
	handle = idr_find(&rotate_private->user_idr, rotate_cmd->handle);
	mutex_unlock(&rotate_private->user_mlock);
	if(handle != (void*)file_priv) {
		DRM_ERROR("Gived id is not your's or not a rotate id\n");
		return -EINVAL;
	}
	return 0;
}

static bool sunxi_check_rotate_info_ok(struct drm_device *dev,
	struct sunxi_rotate_info  *rotate_info, 
	struct sunxi_rotate_task *task_work, struct drm_file *file_priv)
{
	struct sunxi_drm_gem_buf *gem_buf;
	struct drm_gem_object *sunxi_gem;
	tr_info *tr_info;
	tr_info = &task_work->info;

	sunxi_gem = drm_gem_object_lookup(dev, file_priv, rotate_info->dst_gem_handle);
	if (!sunxi_gem) {
		return false;
	}

	drm_gem_object_unreference(sunxi_gem);

	if (!sunxi_check_gem_memory_type(sunxi_gem, SUNXI_BO_CONTIG)) {
		DRM_ERROR("not a contig  mem 1\n");
		return false;
	}

	gem_buf = (struct sunxi_drm_gem_buf *)sunxi_gem->driver_private;
	tr_info->dst_frame.laddr[0] = gem_buf->dma_addr;
	tr_info->dst_frame.haddr[0] = (gem_buf->dma_addr>>32);
	sunxi_sync_buf(gem_buf);

	sunxi_gem = drm_gem_object_lookup(dev, file_priv, rotate_info->src_gem_handle);
	if (!sunxi_gem) {
		return false;
	}

	drm_gem_object_unreference(sunxi_gem);

	if (!sunxi_check_gem_memory_type(sunxi_gem, SUNXI_BO_CONTIG)) {
		DRM_ERROR("not a contig  mem 2\n");
		return false;
	}

	gem_buf = (struct sunxi_drm_gem_buf *)sunxi_gem->driver_private;
	sunxi_sync_buf(gem_buf);
	tr_info->src_frame.laddr[0] = gem_buf->dma_addr;
	tr_info->src_frame.haddr[0] = (gem_buf->dma_addr>>32);
	DRM_DEBUG_KMS("from addr:  %02x%08x  size%lu\n", tr_info->src_frame.haddr[0], tr_info->src_frame.laddr[0], gem_buf->size);
	DRM_DEBUG_KMS("to   addr:  %02x%08x\n", tr_info->dst_frame.haddr[0], tr_info->dst_frame.laddr[0]);

	if((rotate_info->pitch * 8)/rotate_info->bpp%16) {
		DRM_ERROR("1 pitch: %d bpp:%d\n", rotate_info->pitch, rotate_info->bpp);
		return false;
	}
	if((rotate_info->pitch * 8)%rotate_info->bpp) {
		DRM_ERROR("2 pitch: %d bpp:%d\n", rotate_info->pitch, rotate_info->bpp);
		return false;
	}

	return true;
}

int sunxi_rotate_transform(struct drm_device *dev, struct sunxi_rotate_info *rotate_info,
	struct sunxi_rotate_task *task_work, struct drm_file *file_priv)
{
	tr_info *tr_info;

	if (!sunxi_check_rotate_info_ok(dev, rotate_info, task_work, file_priv))
		goto out; 

	tr_info = &task_work->info;
	switch(rotate_info->depth) {
	case 32:
		tr_info->dst_frame.fmt = TR_FORMAT_ARGB_8888;
		tr_info->src_frame.fmt = TR_FORMAT_ARGB_8888;
		break;
	case 15:
		if (rotate_info->bpp == 16) {
			tr_info->dst_frame.fmt = TR_FORMAT_ABGR_1555;
			tr_info->src_frame.fmt = TR_FORMAT_ABGR_1555; 
		}else{
			goto out;
		}
		break;
	case 24:
		if (rotate_info->bpp == 32) {
			tr_info->dst_frame.fmt = TR_FORMAT_XRGB_8888;
			tr_info->src_frame.fmt = TR_FORMAT_XRGB_8888; 
		}else{
			goto out;
		}
		break;
	case 16:
		tr_info->dst_frame.fmt = TR_FORMAT_RGB_565;
		tr_info->src_frame.fmt = TR_FORMAT_RGB_565; 
		break;
	default:
		goto out;

	}
	/* tmp just support memory copy for drm,
	* hardware not support crop
	*/
	tr_info->mode = rotate_info->mode;
	tr_info->src_frame.pitch[0] = rotate_info->pitch/rotate_info->bpp*8;
	tr_info->dst_frame.pitch[0] = rotate_info->pitch/rotate_info->bpp*8;
	tr_info->src_frame.height[0] = rotate_info->height;
	tr_info->dst_frame.height[0] = rotate_info->height;
	tr_info->src_rect.x = 0;
	tr_info->src_rect.y = 0;
	tr_info->src_rect.w = rotate_info->width;
	tr_info->src_rect.h = rotate_info->height;
	tr_info->dst_rect.x = 0;
	tr_info->dst_rect.y = 0;
	tr_info->dst_rect.w = rotate_info->width;
	tr_info->dst_rect.h = rotate_info->height;
	DRM_DEBUG_KMS("3 wh: %dx%d  format:%d  mode:%d\n", rotate_info->width,
			rotate_info->height,tr_info->dst_frame.fmt,tr_info->mode);

	task_work->sleep_mode = rotate_info->sleep_mode;
	task_work->timeout = rotate_info->set_time;
	return 0;
out:

	return -EINVAL;   
}

void sunxi_add_process_head(struct sunxi_rotate_private *rotate_private,
	struct sunxi_rotate_task *work)
{
	struct sunxi_rotate_task *del_work;

	work->status = ROTAE_NO_START;
	kref_init(&work->refcount);
	spin_lock(&rotate_private->head_lock);
	list_add_tail(&work->w_head, &rotate_private->rotate_work);
	spin_unlock(&rotate_private->head_lock);

	if (!work->sleep_mode) {
		work->timeout += jiffies;
		mutex_lock(&rotate_private->qurey_mlock);
		list_add_tail(&work->q_head, &rotate_private->qurey_head);
		rotate_private->query_cnt++;
		while(rotate_private->query_cnt-- > 10) {
			list_del(rotate_private->qurey_head.prev);
			del_work = to_sunxi_rotate_q(rotate_private->qurey_head.next);
			DRM_INFO("query head has upflow. del the old work id:%lu\n", (unsigned long)del_work);

			spin_lock(&rotate_private->head_lock);
			if (del_work->w_head.next != LIST_POISON1)
			list_del(&del_work->w_head);
			spin_unlock(&rotate_private->head_lock);
			kref_put(&del_work->refcount, rotate_task_free);
		}
		mutex_unlock(&rotate_private->qurey_mlock);
	}
	sunxi_schedule_process(rotate_private);
}

int sunxi_rotate_commit(struct drm_device *dev, struct sunxi_rotate_private *rotate_private,
	struct sunxi_rotate_cmd *rotate_cmd, struct drm_file *file_priv)
{
	int ret;
	struct sunxi_rotate_task *task_work;
	struct sunxi_rotate_info rotate_info;

	if (sunxi_check_access(rotate_private, rotate_cmd, file_priv)) {
		goto out;
	}

	if (copy_from_user(&rotate_info, (void __user *)rotate_cmd->private,
	sizeof(struct sunxi_rotate_info)) != 0) {
		DRM_ERROR("copy_from_user rotate_info error.\n");
		goto out;
	}

	task_work = kzalloc(sizeof(struct sunxi_rotate_task), GFP_KERNEL);
	if (!task_work) {
		DRM_ERROR("kzalloc rotate task_work error.\n");
		goto out;
	}
	ret = sunxi_rotate_transform(dev, &rotate_info, task_work, file_priv);
	if(ret) {
		DRM_ERROR("transform rotate data error.\n");
		goto out;
	}
	sunxi_add_process_head(rotate_private, task_work);
	if (task_work->sleep_mode) {
		wait_event_interruptible_timeout(rotate_private->task_wq,
		task_work->status == ROTATE_OK, msecs_to_jiffies(task_work->timeout));

		DRM_DEBUG_KMS("staus:%d  %p  %p\n", task_work->status, task_work, rotate_private->current_task);
		put_user(task_work->status, (int *)rotate_cmd->private);

		spin_lock(&rotate_private->head_lock);
		if (task_work->w_head.next != LIST_POISON1)
			list_del(&task_work->w_head);
		spin_unlock(&rotate_private->head_lock);
		kref_put(&task_work->refcount, rotate_task_free);

	}
	return 0;
out:
	if(task_work)
		kfree(task_work);
	return -EINVAL; 
}

struct sunxi_rotate_task *sunxi_rotate_find_id(struct sunxi_rotate_private *rotate_private,
        struct sunxi_rotate_task *need_id)
{
	struct list_head *pos;
	struct list_head *n;
	struct sunxi_rotate_task *find = NULL;

	list_for_each_safe(pos, n, &rotate_private->qurey_head) {
		find = to_sunxi_rotate_q(pos);
		if(find == need_id)
			break;
			find = NULL;
	}

	if (find == NULL) {
		DRM_ERROR("no-query type work or a bad work or has queryed\n");
	}
	return find;
}

int sunxi_rotate_query(struct sunxi_rotate_private *rotate_private,
    struct sunxi_rotate_cmd *rotate_cmd, struct drm_file *file_priv)
{
	int ret = -1;
	struct sunxi_rotate_task *query_hanle = NULL;

	if (sunxi_check_access(rotate_private, rotate_cmd, file_priv)) {
		goto out;
	}
	mutex_lock(&rotate_private->qurey_mlock);

	query_hanle = sunxi_rotate_find_id(rotate_private, rotate_cmd->private);
	if (query_hanle) {
		ret = query_hanle->status;

		if (time_after_eq(jiffies, query_hanle->timeout)) {
			spin_lock(&rotate_private->head_lock);
			if(query_hanle->w_head.next != LIST_POISON1){
				list_del(&query_hanle->w_head);
			}
			spin_unlock(&rotate_private->head_lock);

			list_del(&query_hanle->q_head);
			kref_put(&query_hanle->refcount, rotate_task_free);
		}
	}

	mutex_unlock(&rotate_private->qurey_mlock);

	put_user(ret, (int *)rotate_cmd->private);

	return 0;
out:
	return -EINVAL;
}

int sunxi_rotate_aquire(struct sunxi_rotate_private *rotate_private,
        struct sunxi_rotate_cmd *rotate_cmd, struct drm_file *file_priv)
{
	int ret;

	mutex_lock(&rotate_private->user_mlock);
	ret = idr_alloc(&rotate_private->user_idr, file_priv, 1, 0, GFP_KERNEL);
	rotate_private->used_cnt++;
	if(ret < 0) {
		DRM_ERROR("rotate aquire error %d.\n", ret);
		return -EINVAL;
	}

	if (rotate_private->used_cnt == 1) {
		if (rotate_private->clk)
			clk_prepare_enable(rotate_private->clk);
		de_tr_init();
	}

	mutex_unlock(&rotate_private->user_mlock);
	put_user(ret, (int *)rotate_cmd->private);

	return 0;
}

int sunxi_rotate_release(struct sunxi_rotate_private *rotate_private,
        int id, struct drm_file *file_priv)
{
	int ret = 0;
	void *handle;

	mutex_lock(&rotate_private->user_mlock);
	handle = idr_find(&rotate_private->user_idr, id);
	if (handle != (void*)file_priv) {
		DRM_ERROR("you release the handle is not your's\n");
		ret = -EINVAL;
		goto out;
	}
	rotate_private->used_cnt--;
	idr_remove(&rotate_private->user_idr, id);
out:
	if (rotate_private->used_cnt == 0) {
		de_tr_exit();
		if (rotate_private->clk)
			clk_disable(rotate_private->clk);
	}
	mutex_unlock(&rotate_private->user_mlock);

	return ret;
}

int sunxi_drm_rotate_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	int ret = -EINVAL;
	struct sunxi_rotate_cmd *rotate_cmd;
	struct sunxi_drm_private *drm_private;
	struct sunxi_rotate_private *rotate_private;

	rotate_cmd = (struct sunxi_rotate_cmd *)data;
	drm_private = (struct sunxi_drm_private *)dev->dev_private;
	rotate_private = drm_private->rotate_private;
	if (!rotate_cmd || !rotate_private) {
		DRM_ERROR("you must give us rotate date\n");
		goto out;
	}

	switch(rotate_cmd->cmd) {
	case TR_CMD_AQUIRE:
		ret = sunxi_rotate_aquire(rotate_private, rotate_cmd, file_priv);
		break;
	case TR_CMD_COMMIT:
		ret = sunxi_rotate_commit(dev, rotate_private, rotate_cmd, file_priv);
		break;
	case TR_CMD_QUERY:
		ret = sunxi_rotate_query(rotate_private, rotate_cmd, file_priv);
		break;
	case TR_CMD_RELEASE:
		ret = sunxi_rotate_release(rotate_private, rotate_cmd->handle, file_priv);
		break;
	default:
		DRM_ERROR("rotate give us a error cmd.\n");
		return -EINVAL;
	}
out:
	return ret;
}

