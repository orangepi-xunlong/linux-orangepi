/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec.c
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>

#include "dec.h"
#include "dma_buf.h"
#include "dec_fence.h"
#include <asm-generic/poll.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <../drivers/misc/sunxi-tvutils/afbd_hwspinlock.h>

#include "dec_trace.h"
#include "dec_video_info.h"
#include "afbd_wb.h"

#if FPGA_DEBUG_ENABLE == 1
#include <linux/moduleparam.h>
static int sb_layout = 1;
module_param(sb_layout, int, 0660);
#endif

/*
 * invert output filed squence of bff interlace video stream.
 */
static int invert_field = 1;
module_param(invert_field, int, 0644);

/*
 * trigger a iommu invalid address bug
 */
static int iommu_debug;
module_param(iommu_debug, int, 0644);

static struct frame_dma_buf_t dummy;

static void vsync_checkin(struct dec_device *p_dec)
{
	u32 index = p_dec->p_info->sync_time_index;

	p_dec->p_info->sync_time[index] = jiffies;
	index++;
	index = (index >= 100) ? 0 : index;
	p_dec->p_info->sync_time_index = index;
}

u32 dec_get_fps(struct dec_device *p_dec)
{
	u32 pre_time_index, cur_time_index;
	u32 pre_time, cur_time;
	u32 fps = 0;

	pre_time_index = p_dec->p_info->sync_time_index;
	cur_time_index =
		(pre_time_index == 0) ?
		(100 - 1) : (pre_time_index - 1);

	pre_time = p_dec->p_info->sync_time[pre_time_index];
	cur_time = p_dec->p_info->sync_time[cur_time_index];


	 /* HZ:timer interrupt times in 1 sec */
	 /* jiffies:increase 1 when timer interrupt occur. */
	 /* jiffies/HZ:current kernel boot time(second). */
	 /* 1000MS / ((cur_time_jiffies/HZ - pre_time_jiffes/HZ)*1000) */

	if (pre_time != cur_time)
		fps = MSEC_PER_SEC * HZ / (cur_time - pre_time);

	return fps;
}

static ssize_t dec_show_info(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	unsigned int i = 0;
	void *reg_base;
	int locked;
	unsigned long flags;

	struct dec_device *p_dec =
		container_of(dev, struct dec_device, attr_device);

	if (!p_dec) {
		dev_err(dev, "Null dec_device\n");
		goto OUT;
	}

	count += sprintf(buf + count, "Enabled:%u irq:%d skip:%d irq_no:%u fps:%u\n",
			 p_dec->enabled, p_dec->p_info->irq_cnt,
			 p_dec->p_info->skip_cnt, p_dec->dec_irq_no, dec_get_fps(p_dec));

	count += sprintf(buf + count, "tvdisp top reg value:\n");
	/*print reg value*/
	reg_base = (void * __force)p_dec->p_reg->p_tvdisp_top;
	for (i = 0; i < 40; i += 4) {
		count += sprintf(buf + count, "0x%.8lx: 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n",
				 (unsigned long)reg_base, readl(reg_base), readl(reg_base + 4),
				 readl(reg_base + 8), readl(reg_base + 12));
		reg_base += 0x10;
	}

	locked = afbd_read_lock(&flags);

	count += sprintf(buf + count, "Afbd reg value:\n");
	reg_base = (void * __force)p_dec->p_reg->p_top_reg;
	for (i = 0; i < 100; i += 4) {
		count += sprintf(buf + count, "0x%.8lx: 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n",
				 (unsigned long)reg_base, readl(reg_base), readl(reg_base + 4),
				 readl(reg_base + 8), readl(reg_base + 12));
		reg_base += 0x10;
	}

	count += sprintf(buf + count, "Afbd wb reg value:\n");
	reg_base = (void * __force)p_dec->p_reg->p_wb_reg;
	for (i = 0; i < 30; i += 4) {
		count += sprintf(buf + count, "0x%.8lx: 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n",
				 (unsigned long)reg_base, readl(reg_base), readl(reg_base + 4),
				 readl(reg_base + 8), readl(reg_base + 12));
		reg_base += 0x10;
	}

	if (locked == 0)
		afbd_read_unlock(&flags);

	count += sprintf(buf + count, "Workaround read: 0x%08x\n",
			dec_workaround_read(p_dec->p_reg, 0x00));

	count += sprintf(buf + count, "Afbd clk info\n");

	count += sprintf(buf + count, "Dec_clk clk rate:%lu\n", clk_get_rate(p_dec->afbd_clk));

	count += sprintf(buf + count, "Reset state:%d\n", reset_control_status(p_dec->rst_bus_disp));

OUT:
	return count;
}

static ssize_t dec_show_frame_manager(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	struct dec_device *p_dec = container_of(dev, struct dec_device, attr_device);

	if (!p_dec) {
		dev_err(dev, "Null dec_device\n");
		goto OUT;
	}

	count += dump_video_info_memory_list(buf + count);

	if (p_dec->fmgr)
		count += dec_frame_manager_dump(p_dec->fmgr, buf + count);

OUT:
	return count;
}

static ssize_t  dec_write_repeat_count(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned int val;

	err = kstrtou32(buf, 10, &val);
	if (err) {
		pr_warn("Invalid size\n");
		return err;
	}

	set_repeat_count_for_signal_changed(val);

	return count;
}

static ssize_t  dec_show_repeat_count(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", get_repeat_count_for_signal_changed());
}

static DEVICE_ATTR(info, 0660, dec_show_info, NULL);
static DEVICE_ATTR(frame_manager, 0660, dec_show_frame_manager, NULL);
static DEVICE_ATTR(repeat_count, 0660, dec_show_repeat_count, dec_write_repeat_count);

static struct attribute *dec_attributes[] = {
	&dev_attr_info.attr,
	&dev_attr_frame_manager.attr,
	&dev_attr_repeat_count.attr,
	NULL,
};

static void dec_vsync_timestamp_record(struct dec_device *p_dec)
{
	unsigned long flags;
	ktime_t now;
	unsigned int next;
	struct dec_vsync_t *vsync = p_dec->p_vsync_info;

	now = ktime_get();

	spin_lock_irqsave(&vsync->slock, flags);

	vsync->vsync_timestamp[vsync->vsync_timestamp_tail] = now;

	next = (vsync->vsync_timestamp_tail + 1) % VSYNC_NUM;
	vsync->vsync_timestamp_tail = next;
	if (next == vsync->vsync_timestamp_head) {
		vsync->vsync_timestamp_head = (vsync->vsync_timestamp_head + 1) % VSYNC_NUM;
	}

	spin_unlock_irqrestore(&vsync->slock, flags);

	DEC_TRACE_VSYNC("record", now);

	wake_up_interruptible(&p_dec->event_wait);
}

static _Bool dec_has_available_vsync_timestamp(struct dec_device *p_dec)
{
	_Bool has = 0;
	unsigned long flags;
	struct dec_vsync_t *vsync = p_dec->p_vsync_info;

	spin_lock_irqsave(&vsync->slock, flags);
	has = vsync->vsync_timestamp_head != vsync->vsync_timestamp_tail;
	spin_unlock_irqrestore(&vsync->slock, flags);

	return has;
}

static int dec_vsync_timestamp_get(struct dec_device *p_dec, struct dec_vsync_timestamp *out)
{
	ktime_t ts;
	unsigned long flags;
	int ret = -EAGAIN;
	struct dec_vsync_t *vsync = p_dec->p_vsync_info;

	spin_lock_irqsave(&vsync->slock, flags);
	if (vsync->vsync_timestamp_head != vsync->vsync_timestamp_tail) {
		ts = vsync->vsync_timestamp[vsync->vsync_timestamp_head];
		vsync->vsync_timestamp_head = (vsync->vsync_timestamp_head + 1) % VSYNC_NUM;
		out->timestamp = ktime_to_ns(ts);
		ret = 0;
	}
	spin_unlock_irqrestore(&vsync->slock, flags);

	return ret;
}

static unsigned int dec_event_poll(struct dec_device *p_dec, struct file *filp, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &p_dec->event_wait, wait);
	if (dec_has_available_vsync_timestamp(p_dec))
		mask = POLLIN;

	return	mask;
}

static int dec_set_overscan(struct dec_device *p_dec, struct overscan_info *info)
{
	dec_frame_manager_set_overscan(p_dec->fmgr, info);
	return 0;
}

static int dec_vsync_counter;

static irqreturn_t dec_vsync_handler(int irq, void *parg)
{
	int ret = 0;
	struct dec_device *p_dev = (struct dec_device *)parg;
    struct afbd_wb_dev_t *pdev = NULL;

	ret = dec_irq_query(p_dev->p_reg);
	if (ret) {
	    pdev = get_afbd_wb_inst();
	    if (pdev) {
		    if (pdev->get_wb_state(pdev) == AFBD_WB_SYNC) {
			    if (pdev->afbd_wait_fb_finish(pdev, pdev->wb_info.ch_id)) {
				    pdev->afbd_wb_en(pdev, pdev->wb_info.ch_id, 0);
				    pdev->afbd_set_wb_state(pdev, AFBD_WB_IDLE);
			    }
		    }
		    if (pdev->get_wb_state(pdev) == AFBD_WB_COMMIT) {
			    if (pdev->afbd_is_ch_en(pdev, pdev->wb_info.ch_id)) {
				    if (pdev->buf) {
					    pdev->afbd_wb_en(pdev, pdev->wb_info.ch_id, 1);
					    pdev->afbd_set_wb_state(pdev, AFBD_WB_SYNC);
				    }
			    } else {
				    pr_err("ch:%d is not enabled!\n", pdev->wb_info.ch_id);
				    pdev->afbd_set_wb_state(pdev, AFBD_WB_IDLE);
			    }

		    }
	    }
		dec_vsync_timestamp_record(p_dev);
		vsync_checkin(p_dev);

		++p_dev->p_info->irq_cnt;

		if (p_dev->fmgr)
			dec_vsync_process(p_dev->fmgr);
	}

	dec_vsync_counter++;
	DEC_TRACE_INT_F("dec-vsync", (dec_vsync_counter & 1));

	return IRQ_HANDLED;
}

static void dec_sync_frame_to_hardware(struct afbd_reg_t *base, void *private, int skip_info_addr, unsigned int id, unsigned int field)
{
	video_frame_setting_t *setting = (video_frame_setting_t *)private;
	struct frame_dma_buf_t *item = (struct frame_dma_buf_t *)setting->frame;
	unsigned long long *frame_addr;
	unsigned long long info_addr = 0;
	unsigned int repeat_en = setting->hardware_repeat_enable;
#if FPGA_DEBUG_ENABLE == 1
	struct dec_buffer_attr inattr;
#endif

	if (base == NULL)
		return;

	// When frame item is null,
	// It means that we are exiting from stream play, So clear repeat mode flag here !
	if (!item) {
		item = &dummy;
		repeat_en = 0;
	}

	info_addr = skip_info_addr ? 0 : item->info_addr;
	frame_addr = (field == 0) ? item->frame_addr : item->second_field_addr;

	if (unlikely(iommu_debug)) {
		frame_addr[0] |= 0x80000000;
		frame_addr[1] |= 0x80000000;
	}

#if FPGA_DEBUG_ENABLE == 1
	memset(&inattr, 0, sizeof(struct dec_buffer_attr));
	inattr.is_compressed = item->is_compressed;
	inattr.format = item->format;
	inattr.width = item->width;
	inattr.height = item->height;
	inattr.sb_layout = sb_layout;
	dec_reg_video_channel_attr_config(base, &inattr);
#endif

	dec_reg_set_address(base, frame_addr, info_addr, id);
	DEC_TRACE_VINFO("vinfo", id, (uint32_t)frame_addr[0], (uint32_t)(info_addr|field));
	dec_debug_trace_frame(frame_addr[0], item->interlace);

	if (id == ADDR_OUT_MUX_ID) {
		if (invert_field && item->interlace && !item->top_field_first)
			field = (field == 0) ? 1 : 0;
		dec_reg_set_filed_mode(base,  field);
		DEC_TRACE_FRAME("field_mode", field);
	} else if (id == 3) {
		dec_reg_set_filed_repeat(base, repeat_en);
		dec_reg_set_dirty(base, true);
		DEC_TRACE_FRAME("repeat-mode", repeat_en);
	}
#if FPGA_DEBUG_ENABLE == 1
	dec_reg_set_dirty(base, true);
#endif
}

void dec_sync_videoinfo_to_hardware(struct afbd_reg_t *base, void *private, unsigned int id, unsigned int field)
{
	struct frame_dma_buf_t *item = (struct frame_dma_buf_t *)private;
	unsigned long long info_addr = item ? item->info_addr : 0;

	dec_reg_set_videoinfo(base, info_addr, id);
	DEC_TRACE_VINFO("vinfo", id, (uint32_t)0xFFFFFFFF, (uint32_t)(info_addr|field));
}

void dec_sync_interlace_cfg_to_hardware(struct afbd_reg_t *base, void *private)
{
	video_frame_setting_t *setting = (video_frame_setting_t *)private;
	struct frame_dma_buf_t *item = (struct frame_dma_buf_t *)setting->frame;
	unsigned int field = setting->field_mode;

	if (invert_field && item->interlace && !item->top_field_first)
		field = (field == 0) ? 1 : 0;

	dec_reg_set_filed_mode(base, field);
	dec_reg_set_dirty(base, true);
	DEC_TRACE_FRAME("field_mode", field);
}

void dec_decoder_display_init(struct afbd_reg_t *p_reg)
{
	// set addy_y_out_mux and addr_c_out_mux to 1,
	// requirq by svp!!!
	dec_reg_mux_select(p_reg, ADDR_OUT_MUX_ID);
}

#if FPGA_DEBUG_ENABLE == 1
static int vsync_run_thread(void *parg)
{
	struct dec_device *p_dev = (struct dec_device *)parg;

	while (1) {
		if (kthread_should_stop())
			break;
		dec_vsync_handler(0, (void *)p_dev);
		msleep_interruptible(16);
	}

	return 0;
}
#endif

static int dec_enable(struct dec_device *p_dev)
{
	int ret = -1;

	if (!p_dev) {
		pr_err("[dec] NULL pointer!\n");
		goto OUT;
	}

	mutex_lock(&p_dev->dev_lock);

	if (p_dev->enabled) {
		ret = 0;
		goto UNLOCK;
	}

	ret = reset_control_deassert(p_dev->rst_bus_disp);
	if (ret) {
		dev_err(p_dev->p_device, "reset fail:%d!\n", ret);
		goto UNLOCK;
	}

	ret = clk_prepare_enable(p_dev->disp_gate);
	if (ret) {
		dev_err(p_dev->p_device, "disp gate enable fail:%d!\n", ret);
		goto RST_ASSERT;
	}

	ret = clk_prepare_enable(p_dev->afbd_clk);
	if (ret) {
		dev_err(p_dev->p_device, "clk enable fail:%d!\n", ret);
		goto CLK_DISABLE;
	}
	clk_set_rate(p_dev->afbd_clk, p_dev->afbd_clk_frequency);

	ret = request_irq(p_dev->dec_irq_no, (irq_handler_t)dec_vsync_handler, 0x0,
			"dec", (void *)p_dev);
	if (ret) {
		dev_err(p_dev->p_device, "request irq fail:%d!\n", ret);
		goto CLK_DISABLE2;
	}

	p_dev->fmgr = dec_frame_manager_init();
	if (IS_ERR_OR_NULL(p_dev->fmgr)) {
		dev_err(p_dev->p_device, "video frame manager init failed!\n");
		ret = -EINVAL;
		goto FREE_IRQ;
	}

	dec_frame_manager_register_sync_cb(p_dev->fmgr,
			p_dev->p_reg, dec_sync_frame_to_hardware);
	dec_frame_manager_register_repeat_ctrl_pfn(p_dev->fmgr,
			p_dev->p_reg, dec_reg_set_filed_repeat);

	dec_reg_top_enable(p_dev->p_reg, 1);
	dec_decoder_display_init(p_dev->p_reg);

	afbd_wb_init(p_dev->p_reg, p_dev->p_device, AFBD_OFFLINE_MODE);


#if FPGA_DEBUG_ENABLE == 1
	p_dev->vsync_task = kthread_create(vsync_run_thread,
							   (void *)p_dev, "decd vsync");
	if (IS_ERR(p_dev->vsync_task)) {
		dev_err(p_dev->p_device, "Create decd vsync thread fail!\n");
	} else
		wake_up_process(p_dev->vsync_task);
#endif

	p_dev->enabled = true;

UNLOCK:
	mutex_unlock(&p_dev->dev_lock);
OUT:
	return ret;
FREE_IRQ:
	free_irq(p_dev->dec_irq_no, (void *)p_dev);
CLK_DISABLE2:
	clk_disable_unprepare(p_dev->disp_gate);
CLK_DISABLE:
	clk_disable_unprepare(p_dev->afbd_clk);
RST_ASSERT:
	reset_control_assert(p_dev->rst_bus_disp);
	mutex_unlock(&p_dev->dev_lock);
	return ret;
}

static int dec_disable(struct dec_device *p_dev)
{
	int ret = -1;

	mutex_lock(&p_dev->dev_lock);

	if (!p_dev->is_enabled(p_dev)) {
		ret = 0;
		goto OUT;
	}


	dec_frame_manager_stop(p_dev->fmgr);
	disable_irq(p_dev->dec_irq_no);
#if FPGA_DEBUG_ENABLE == 1
	if (p_dev->vsync_task) {
		kthread_stop(p_dev->vsync_task);
		p_dev->vsync_task = NULL;
	}
#endif
	dec_frame_manager_exit(p_dev->fmgr);
	p_dev->fmgr = NULL;

	dec_reg_int_to_display(p_dev->p_reg);

	dec_reg_enable(p_dev->p_reg, 0);

	ret = reset_control_assert(p_dev->rst_bus_disp);
	if (ret) {
		dev_err(p_dev->p_device, "reset fail:%d!\n", ret);
	}

	clk_disable_unprepare(p_dev->disp_gate);
	clk_disable_unprepare(p_dev->afbd_clk);

	free_irq(p_dev->dec_irq_no, (void *)p_dev);
	afbd_wb_deinit();

	p_dev->enabled = false;

OUT:
	mutex_unlock(&p_dev->dev_lock);
	return ret;
}

_Bool dec_is_enabled(struct dec_device *p_dev)
{
	return p_dev->enabled;
}

static struct dec_format_attr fmt_attr_tbl[] = {
	/* format							 bits */
	/* hor_rsample(u,v) */
	/* ver_rsample(u,v) */
	/* uvc */
	/* interleave */
	/* factor */
	/* div */
	{ DEC_FORMAT_RGB888, 8,  1, 1, 1, 1, 0, 1, 3, 1},
	{ DEC_FORMAT_YUV444P, 8,  1, 1, 1, 1, 0, 1, 1, 1},
	{ DEC_FORMAT_YUV422P, 8,  2, 2, 1, 1, 1, 0, 2, 1},
	{ DEC_FORMAT_YUV420P, 8,  2, 2, 2, 2, 1, 0, 3, 2},
	{ DEC_FORMAT_YUV444P_10BIT, 10, 1, 1, 1, 1, 0, 0, 6, 1},
	{ DEC_FORMAT_YUV422P_10BIT, 10, 2, 2, 1, 1, 1, 0, 4, 1},
	{ DEC_FORMAT_YUV420P_10BIT, 10, 2, 2, 2, 2, 0, 0, 3, 1},
	{ DEC_FORMAT_YUV420P_10BIT_AV1, 10, 2, 2, 2, 2, 0, 0, 3, 1},

};

static void frame_item_free(struct kref *kref)
{
	struct frame_dma_buf_t *item = container_of(kref, struct frame_dma_buf_t, refcount);

	if (item->fence) {
		dec_fence_signal(item->fence);
		dec_fence_free(item->fence);
		item->fence = NULL;
	}
	dec_debug_trace_dma_buf_unmap(item->image_item, item->image_item->dma_addr);

	dec_dma_unmap(item->image_item);
	dec_dma_unmap(item->info_item);
	video_info_buffer_deinit(item);

	kfree(item);
}

static void frame_item_release(void *private)
{
	struct frame_dma_buf_t *item = (struct frame_dma_buf_t *)private;
	kref_put(&item->refcount, frame_item_free);
}

static inline void swap_frame_address(unsigned long long *a, unsigned long long *b)
{
	unsigned long long tmp;
	tmp = *a; *a = *b; *b = tmp;
	++a; ++b;
	tmp = *a; *a = *b; *b = tmp;
}

static struct frame_dma_buf_t *frame_item_create(struct dec_device *p_dev, struct dec_frame_config *p_cfg)
{
	struct dec_dmabuf_t *image_item = NULL, *info_item = NULL;
	u32 i = 0;
	u32 len = ARRAY_SIZE(fmt_attr_tbl);
	u32 y_width, y_height, u_width, u_height;
	u32 y_pitch, u_pitch;
	u32 y_size, u_size;
	struct frame_dma_buf_t *frame_item = NULL;
	unsigned long long frame_addr[2] = {0}, info_addr = {0};

	y_width = p_cfg->image_size[0].width;
	y_height = p_cfg->image_size[0].height;
	u_width = p_cfg->image_size[1].width;
	u_height = p_cfg->image_size[1].height;
	u_width = (u_width == 0) ? y_width : u_width;
	u_height = (u_height == 0) ? y_height : u_height;

	if (p_cfg->use_phy_addr) {
		frame_addr[0] = p_cfg->image_addr[0];
		frame_addr[1] = p_cfg->image_addr[1];
		info_addr = video_info_buffer_init(p_cfg->metadata_addr);
	} else {
		if (p_cfg->format > DEC_FORMAT_MAX)
			goto OUT;

		frame_item = kmalloc(sizeof(struct frame_dma_buf_t),
					 GFP_KERNEL | __GFP_ZERO);
		if (!frame_item) {
			dev_err(p_dev->p_device, "Kmalloc frame_item fail!\n");
			goto OUT;
		}

		kref_init(&frame_item->refcount);

		image_item = dec_dma_map(p_cfg->image_fd, p_dev->p_device);
		if (!image_item) {
			dev_err(p_dev->p_device, "image dma map fail!\n");
			goto FREE_FRAME;
		}
		info_item = dec_dma_map(p_cfg->metadata_fd, p_dev->p_device);
		if (!info_item) {
			dev_err(p_dev->p_device, "image dma map fail!\n");
			goto UNMAP_IMAGE;
		}

		frame_item->image_item = image_item;
		frame_item->info_item = info_item;
		frame_item->is_compressed = p_cfg->fbd_en;
		frame_item->format = p_cfg->format;
		frame_item->width = p_cfg->image_size[0].width;
		frame_item->height = p_cfg->image_size[0].height;

		info_addr = video_info_buffer_init_dmabuf(frame_item);
		frame_addr[0] = image_item->dma_addr;

		if (p_cfg->fbd_en == 0) {
			for (i = 0; i < len; ++i) {

				if (fmt_attr_tbl[i].format == p_cfg->format) {
					y_pitch = y_width *
						((fmt_attr_tbl[i].bits == 8) ? 1 : 2);
					u_pitch = u_width *
						((fmt_attr_tbl[i].bits == 8) ? 1 : 2) *
						(fmt_attr_tbl[i].uvc + 1);

					y_pitch = DEC_ALIGN(y_pitch, p_cfg->image_align[0]);
					u_pitch = DEC_ALIGN(u_pitch, p_cfg->image_align[1]);
					y_size = y_pitch * y_height;
					u_size = u_pitch * u_height;

					frame_addr[1] = image_item->dma_addr + y_size; /* u */

					break;
				}
			}
		}

		memcpy(frame_item->frame_addr, frame_addr, 2 * sizeof(unsigned long long));
		memcpy(&frame_item->info_addr, &info_addr, sizeof(unsigned long long));
	}

	if (p_cfg->interlace_en) {
		frame_item->interlace = 1;
		frame_item->top_field_first = 1;
		frame_item->second_field_addr[0] = frame_item->frame_addr[0] + y_pitch;
		frame_item->second_field_addr[1] = frame_item->frame_addr[1] + y_pitch;

		// always make the top filed in the first address slot
		if (p_cfg->field_mode == BOTTOM_FIELD_FIRST) {
			frame_item->top_field_first = 0;
			swap_frame_address(frame_item->frame_addr, frame_item->second_field_addr);
		}

		pr_debug("top: 0x%016llx 0x%016llx bottom: 0x%016llx 0x%016llx pitch: 0x%08x\n",
				frame_item->frame_addr[0], frame_item->frame_addr[1],
				frame_item->second_field_addr[0], frame_item->second_field_addr[1],
				y_pitch);
	} else {
		// just for test !!!
		frame_item->second_field_addr[0] = frame_item->frame_addr[0];
		frame_item->second_field_addr[1] = frame_item->frame_addr[1];
	}

	// create a fence for sync
	if (p_dev->fence_context)
		frame_item->fence = dec_fence_alloc(p_dev->fence_context);

	dec_debug_trace_dma_buf_map(frame_item->image_item,
			frame_item->image_item->dma_addr,
			dec_dma_buf_size(frame_item->image_item),
			frame_item->vinfo_block);

OUT:
	return frame_item;
UNMAP_IMAGE:
	dec_dma_unmap(image_item);
FREE_FRAME:
	kfree(frame_item);
	return NULL;
}

static int dec_frame_submit(struct dec_device *p_dev,
				struct dec_frame_config *p_cfg, int *fence, int repeat_count)
{
	int ret = 0;
	struct frame_dma_buf_t *item;

	DEC_TRACE_FUNC();

	if (!p_dev || !p_cfg) {
		pr_err("[dec] Null pointer!\n");
		goto OUT;
	}
	mutex_lock(&p_dev->dev_lock);

	if (!p_dev->is_enabled(p_dev)) {
		dev_err(p_dev->p_device, "dec is not enabled!\n");
		goto OUT;
	}

	if (p_cfg->blue_en)
		goto BLUE_SCREEN;

	item = frame_item_create(p_dev, p_cfg);

	if (item) {
		do {
			DEC_TRACE_BEGIN("enqueue-frame");
			kref_get(&item->refcount);
			dec_frame_manager_enqueue_frame(p_dev->fmgr, item, frame_item_release);
			DEC_TRACE_END("");
		} while (repeat_count-- > 1);

		*fence = dec_fence_fd_create(item->fence);
		frame_item_release(item);
	}

	// TODO: handle filed on vsync callback
	// dec_reg_set_filed_mode(p_dev->p_reg, p_cfg->field_mode);

BLUE_SCREEN:
	dec_reg_blue_en(p_dev->p_reg, p_cfg->blue_en);
	dec_reg_enable(p_dev->p_reg, 1);

OUT:
	mutex_unlock(&p_dev->dev_lock);

	DEC_TRACE_END("");
	return ret;
}

static int dec_interlace_setup(struct dec_device *p_dev,
				struct dec_frame_config *p_cfg)
{
	int ret = 0;
	struct frame_dma_buf_t *items[2];

	if (!p_dev || !p_cfg) {
		pr_err("[dec] Null pointer!\n");
		goto OUT;
	}
	mutex_lock(&p_dev->dev_lock);

	if (!p_dev->is_enabled(p_dev)) {
		dev_err(p_dev->p_device, "dec is not enabled!\n");
		goto OUT;
	}

	if (p_cfg->blue_en)
		goto BLUE_SCREEN;

	dec_frame_manager_set_stream_type(p_dev->fmgr, STREAM_INTERLACED);

	items[0] = frame_item_create(p_dev, p_cfg);
	items[1] = frame_item_create(p_dev, p_cfg+1);

	if (items[0] && items[1])
		dec_frame_manager_enqueue_interlace_frame(p_dev->fmgr, items[0], items[1], frame_item_release);

BLUE_SCREEN:
	dec_reg_blue_en(p_dev->p_reg, p_cfg->blue_en);
	dec_reg_enable(p_dev->p_reg, 1);

OUT:
	mutex_unlock(&p_dev->dev_lock);
	return ret;
}

static int dec_bypass_config(struct dec_device *p_dev, int enable)
{
	mutex_lock(&p_dev->dev_lock);
	dec_reg_bypass_config(p_dev->p_reg, enable);
	mutex_unlock(&p_dev->dev_lock);
	dev_info(p_dev->p_device, "bypass config: %d\n", enable);
	return 0;
}

static int dec_stop_video_stream(struct dec_device *p_dev)
{
	int ret = 0;
	if (!p_dev) {
		pr_err("[dec] Null pointer!\n");
		goto OUT;
	}
	mutex_lock(&p_dev->dev_lock);

	if (!p_dev->is_enabled(p_dev)) {
		dev_err(p_dev->p_device, "dec is not enabled!\n");
		goto OUT;
	}

	ret = dec_frame_manager_stop(p_dev->fmgr);
OUT:
	mutex_unlock(&p_dev->dev_lock);
	return ret;
}

int dec_suspend(struct dec_device *p_dev)
{
	int ret = -1;

	if (p_dev->sync_finish)
		p_dev->sync_finish(p_dev);
	ret = p_dev->disable(p_dev);

	return ret;
}

int dec_resume(struct dec_device *p_dev)
{
	int ret = -1;

	ret = p_dev->enable(p_dev);

	return ret;
}

struct dec_device *dec_init(struct device *p_device)
{
	struct dec_device *pdec = NULL;
	int counter = 0;
	char id[32];

	if (!p_device || !p_device->parent) {
		pr_err("Null pointer!\n");
		goto OUT;
	}


	pdec = kmalloc(sizeof(struct dec_device), GFP_KERNEL | __GFP_ZERO);
	if (!pdec) {
		dev_err(pdec->p_device->parent, "Malloc dec_device fail!\n");
		goto OUT;
	}

	memcpy(&pdec->attr_device, p_device, sizeof(struct device));
	pdec->p_device = p_device->parent;


	pdec->p_reg =
		kmalloc(sizeof(struct afbd_dec_reg), GFP_KERNEL | __GFP_ZERO);
	if (!pdec->p_reg) {
		dev_err(pdec->p_device, "Malloc afbd_dec_reg fail!\n");
		goto FREE_PDEC;
	}

	counter = 0;

	pdec->p_reg->p_top_reg =
		(struct afbd_top_reg *) of_iomap(pdec->p_device->of_node, counter);
	if (!pdec->p_reg->p_top_reg) {
		dev_err(pdec->p_device, "unable to map afbd registers\n");
		goto FREE_REG;
	}

	++counter;

	pdec->p_reg->p_tvdisp_top =
		(struct tvdisp_top_reg *) of_iomap(pdec->p_device->of_node, counter);
	if (!pdec->p_reg->p_tvdisp_top) {
		dev_err(pdec->p_device, "unable to map tvdisp top registers\n");
		goto ERR_IOMAP1;
	}

	pdec->p_reg->p_dec_reg = (struct afbd_dec_reg *)((void *)pdec->p_reg->p_top_reg + DEC_REG_OFFSET);
	pdec->p_reg->p_wb_reg = (struct afbd_wb_reg *)((void *)pdec->p_reg->p_top_reg + WB_REG_OFFSET);

	counter = 0;

	pdec->dec_irq_no =
		irq_of_parse_and_map(pdec->p_device->of_node, counter);
	if (!pdec->dec_irq_no) {
		dev_err(pdec->p_device, "irq_of_parse_and_map dec irq fail\n");
		goto ERR_IOMAP2;
	}

	if (of_property_read_u32(pdec->p_device->of_node, "clock-frequency",
				&pdec->afbd_clk_frequency) != 0) {
		dev_err(pdec->p_device, "Get clock-frequency property failed\n");
		pdec->afbd_clk_frequency = 300000000;
	}

	sprintf(id, "clk_afbd");
	pdec->afbd_clk = devm_clk_get(pdec->p_device, id);
	if (IS_ERR(pdec->afbd_clk)) {
		pdec->afbd_clk = NULL;
		dev_err(pdec->p_device, "failed to get clk for %s\n", id);
		goto ERR_IOMAP2;
	}

	sprintf(id, "clk_bus_disp");
	pdec->disp_gate = devm_clk_get(pdec->p_device, id);
	if (IS_ERR(pdec->disp_gate)) {
		pdec->disp_gate = NULL;
		dev_err(pdec->p_device, "failed to get disp gate for %s\n", id);
		goto ERR_IOMAP2;
	}

	sprintf(id, "rst_bus_disp");
	pdec->rst_bus_disp = devm_reset_control_get_shared(pdec->p_device, id);
	if (IS_ERR(pdec->rst_bus_disp)) {
		pdec->rst_bus_disp = NULL;
		dev_err(pdec->p_device, "failed to get reset for %s\n", id);
		goto ERR_IOMAP2;
	}

	pdec->enable = dec_enable;
	pdec->is_enabled = dec_is_enabled;
	pdec->disable = dec_disable;
	pdec->frame_submit = dec_frame_submit;
	pdec->interlace_setup = dec_interlace_setup;
	pdec->stop_video_stream = dec_stop_video_stream;
	pdec->bypass_config = dec_bypass_config;
	pdec->suspend = dec_suspend;
	pdec->resume = dec_resume;

	pdec->vsync_timestamp_get = dec_vsync_timestamp_get;
	pdec->poll = dec_event_poll;
	pdec->set_overscan = dec_set_overscan;

	pdec->p_attr_grp = kmalloc(sizeof(struct attribute_group), GFP_KERNEL | __GFP_ZERO);

	if (!pdec->p_attr_grp) {
		dev_err(pdec->p_device, "Faile to kmalloc attribute_group\n");
		goto ERR_IOMAP2;
	}

	pdec->p_info = kmalloc(sizeof(struct dec_dump_info), GFP_KERNEL | __GFP_ZERO);
	if (!pdec->p_info) {
		dev_err(pdec->p_device, "Faile to kmalloc dec_dump_info\n");
		goto FREE_ATTR;
	}

	pdec->fence_context = dec_fence_context_alloc("dec");
	if (!pdec->fence_context) {
		dev_err(pdec->p_device, "fali to init dec fence context!\n");
		goto FREE_DUMP;
	}

	pdec->p_vsync_info =
		kmalloc(sizeof(struct dec_vsync_t), GFP_KERNEL | __GFP_ZERO);
	if (!pdec->p_vsync_info) {
		dev_err(pdec->p_device, "Fali to kmalloc vsync info!\n");
		goto FREE_FENCE;
	}

	spin_lock_init(&pdec->p_vsync_info->slock);

	pdec->p_attr_grp->name = kmalloc(10, GFP_KERNEL | __GFP_ZERO);

	sprintf((char *)pdec->p_attr_grp->name, "attr");
	pdec->p_attr_grp->attrs = dec_attributes;
	if (sysfs_create_group(&pdec->attr_device.kobj, pdec->p_attr_grp))
		dev_err(pdec->p_device, "sysfs_create_group fail\n");

	mutex_init(&pdec->dev_lock);
	init_waitqueue_head(&pdec->event_wait);

	video_info_memory_block_init(pdec->p_device);
	dec_debug_init();
	dec_debug_set_register_base(pdec->p_reg->p_top_reg);

	dummy.frame_addr[0] = 0x00;
	dummy.frame_addr[1] = 0x00;
	dummy.info_addr = 0x00;

OUT:
	return pdec;

FREE_FENCE:
	dec_fence_context_free(pdec->fence_context);
	pdec->fence_context = NULL;
FREE_DUMP:
	kfree(pdec->p_info);
	pdec->p_info = NULL;
FREE_ATTR:
	kfree(pdec->p_attr_grp);
	pdec->p_attr_grp = NULL;
ERR_IOMAP2:
	iounmap((char __iomem *)pdec->p_reg->p_tvdisp_top);
ERR_IOMAP1:
	iounmap((char __iomem *)pdec->p_reg->p_top_reg);
FREE_REG:
	kfree(pdec->p_reg);
	pdec->p_reg = NULL;
FREE_PDEC:
	kfree(pdec);
	pdec = NULL;
	return NULL;
}

int dec_exit(struct dec_device *p_dec)
{
	int ret = -1;

	if (!p_dec) {
		pr_err("[dec] NULL pointer!\n");
		goto OUT;
	}

	ret = p_dec->disable(p_dec);
	if (ret)
		dev_err(p_dec->p_device, "dec disable fail!\n");

	video_info_memory_block_exit();

	sysfs_remove_group(&p_dec->attr_device.kobj, p_dec->p_attr_grp);

	iounmap((char __iomem *)p_dec->p_reg->p_tvdisp_top);
	iounmap((char __iomem *)p_dec->p_reg->p_top_reg);
	kfree(p_dec->p_reg);
	kfree(p_dec->p_attr_grp);
	kfree(p_dec->p_info);
	kfree(p_dec->p_vsync_info);
	dec_fence_context_free(p_dec->fence_context);
	kfree(p_dec);
OUT:
	return ret;

}

// helper function for dec_frame_manager
int dec_is_interlace_frame(const void *data)
{
	const struct frame_dma_buf_t *frame = data;
	return frame->interlace;
}
