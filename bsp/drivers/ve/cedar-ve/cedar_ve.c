/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * drivers\media\cedar_ve
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.allwinnertech.com>
 * fangning<fangning@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#define SUNXI_MODNAME	"VE"
#include "cedar_ve.h"
#include <asm/dma.h>
#include <asm/siginfo.h>
#include <asm/signal.h>
#include <asm/cacheflush.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/rmap.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/sched/signal.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#include <linux/pm_runtime.h>
#include <linux/devfreq.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/reset.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#if IS_ENABLED(CONFIG_SUNXI_MPP)
#include <linux/mpp.h>
#endif
#include <sunxi-clk.h>
#include <sunxi-sid.h>

#define DRV_VERSION "1.0.0"

#define VE_CLK_HIGH_WATER  (900)
#define VE_CLK_LOW_WATER   (50)

#define PRINTK_IOMMU_ADDR 0

struct __cedarv_task {
	int task_prio;
	int ID;
	unsigned long timeout;
	unsigned int frametime;
	unsigned int block_mode;
};

struct cedarv_engine_task {
	struct __cedarv_task t;
	struct list_head list;
	struct task_struct *task_handle;
	unsigned int status;
	unsigned int running;
	unsigned int is_first_task;
};

struct cedarv_regop {
	unsigned long addr;
	unsigned int value;
};

struct __cedarv_task_compat {
	int task_prio;
	int ID;
	u32 timeout;
	unsigned int frametime;
	unsigned int block_mode;
};

struct cedarv_regop_compat {
	u32 addr;
	unsigned int value;
};

struct dma_buf_info {
	struct aw_mem_list_head i_list;
	int fd;
	unsigned long addr;
	struct dma_buf *dma_buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int p_id;
	struct cedar_dev *cedar_devp;
};

static struct cedar_ve_quirks cedar_ve_quirk;
static struct cedar_ve_quirks cedar_ve1_quirk;
static struct cedar_ve_quirks cedar_ve2_quirk;
static void set_system_register(struct cedar_dev *cedar_devp);
static int map_dma_buf_addr(struct cedar_dev *cedar_devp,
			    int fd, unsigned int *addr);
static void unmap_dma_buf_addr(struct cedar_dev *cedar_devp,
			       int unmap_all, int fd);

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
extern int SetVeSuspend(void);
extern int SetVeResume(void);
#endif
#endif

/* note: clk control sequence, must be obeyed!!!
 * enable  : deassert->[set rate]->mbus->bus->module
 * disable : module->bus->mbus->assert
 * set rate: disable module -> [set rate] -> enable module
 */
static int enable_cedar_hw_clk_reset(struct cedar_dev *cedar_devp);
static void disable_cedar_hw_clk_reset(struct cedar_dev *cedar_devp);
static int enable_cedar_hw_clk_module(struct cedar_dev *cedar_devp);
static void disable_cedar_hw_clk_module(struct cedar_dev *cedar_devp);
static long setup_ve_freq(struct cedar_dev *cedar_devp, int ve_freq);

/* note: Video engine interrupt service routine to wake up ve wait queue. */
static irqreturn_t VideoEngineInterupt(int irq, void *dev)
{
	struct cedar_dev *cedar_devp = (struct cedar_dev *)dev;
	(void)irq;

	ve_irq_work(cedar_devp);

	return IRQ_HANDLED;
}

static void vdfs_remove_case_load_param(struct cedar_dev *cedar_devp, u32 channel_id)
{
	int i = 0;
	unsigned long flags;
	struct ve_case_load_info *load_info = NULL;

	if (cedar_devp->case_load_channels == 0)
		return;

	spin_lock_irqsave(&cedar_devp->lock, flags);

	for (i = 0; i < cedar_devp->case_load_channels; ++i) {
		load_info = &cedar_devp->load_infos[i];
		if (load_info->is_used && load_info->process_channel_id == channel_id) {
			VE_LOGD("remove case load when call cedardev_release, "
				"i = %d, process_id = %d\n", i, channel_id);
			memset(load_info, 0, sizeof(struct ve_case_load_info));
			break;
		}
	}

	spin_unlock_irqrestore(&cedar_devp->lock, flags);
}

static int vdfs_update_case_load_param(struct cedar_dev *cedar_devp,
				       struct ve_case_load_param *cur_load_param, u32 channel_id)
{
	int i = 0;
	unsigned long flags;
	struct ve_case_load_info *match_load_info = NULL;
	struct ve_case_load_info *load_info = NULL;

	if (cedar_devp->case_load_channels == 0)
		return 0;

	VE_LOGV("update load info: w = %d, h = %d, f = %d, codec_format = %d, remove = %d, "
		"thread_id = %d, process_id = %d\n",
		cur_load_param->width, cur_load_param->height, cur_load_param->frame_rate,
		cur_load_param->codec_format, cur_load_param->is_remove_cur_param,
		cur_load_param->thread_channel_id, channel_id);

	spin_lock_irqsave(&cedar_devp->lock, flags);

	/* match the exist load_info channel */
	for (i = 0; i < cedar_devp->case_load_channels; ++i) {
		load_info = &cedar_devp->load_infos[i];
		if (load_info->is_used &&
		    load_info->load_param.thread_channel_id == cur_load_param->thread_channel_id &&
		    load_info->process_channel_id == channel_id)
			break;
	}

	if (i < cedar_devp->case_load_channels) {
		match_load_info = &cedar_devp->load_infos[i];
	} else {
		/* find the empty */
		for (i = 0; i < cedar_devp->case_load_channels; ++i) {
			load_info = &cedar_devp->load_infos[i];
			if (load_info->is_used == 0) {
				break;
			}
		}
		if (i < cedar_devp->case_load_channels) {
			match_load_info = &cedar_devp->load_infos[i];
		} else {
			VE_LOGW("cannot find empty load_info, may be channels overflow %d\n",
				cedar_devp->case_load_channels);
			spin_unlock_irqrestore(&cedar_devp->lock, flags);
			return -1;
		}
	}

	if (cur_load_param->is_remove_cur_param == 1) {
		memset(match_load_info, 0, sizeof(struct ve_case_load_info));
	} else {
		memcpy(&match_load_info->load_param, cur_load_param,
		       sizeof(struct ve_case_load_param));
		match_load_info->process_channel_id = channel_id;
		match_load_info->is_used = 1;
	}

	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	return 0;
}

static int vdfs_adjust_ve_freq_by_case_load(struct cedar_dev *cedar_devp)
{
	int i = 0;
	u32 total_load_pixels = 0;
	struct ve_case_load_info *cur_load_info = NULL;
	struct ve_performat_info *performat_infos = cedar_devp->performat_infos;
	int adjust_new_ve_freq = cedar_devp->user_setting_ve_freq;
	int decoder_channel_num = 0;
	int encoder_channel_num = 0;
	int had_different_codec_channel = 0;
	int previous_codec_format = -1;
	int cur_codec_format = -1;
	int array_size = 0;

	if (cedar_devp->case_load_channels == 0 || cedar_devp->performat_num == 0)
		return 0;

	for (i = 0; i < cedar_devp->case_load_channels; ++i) {
		cur_load_info = &cedar_devp->load_infos[i];
		if (!cur_load_info->is_used)
			continue;

		VE_LOGV("i[%d]: w = %d, h = %d, f = %d, codec_format = %d, remove = %d, "
			"thread_id = %d, process_id = %d\n", i,
			cur_load_info->load_param.width, cur_load_info->load_param.height,
			cur_load_info->load_param.frame_rate,
			cur_load_info->load_param.codec_format,
			cur_load_info->load_param.is_remove_cur_param,
			cur_load_info->load_param.thread_channel_id,
			cur_load_info->process_channel_id);

		if (cur_load_info->load_param.frame_rate <= 0)
			cur_load_info->load_param.frame_rate = 60;

		if (previous_codec_format == -1)
			previous_codec_format = cur_load_info->load_param.codec_format;

		if (previous_codec_format != cur_load_info->load_param.codec_format)
			had_different_codec_channel = 1;

		if (cur_load_info->load_param.codec_format < VE_DECODER_FORMAT_MAX)
			decoder_channel_num++;
		else
			encoder_channel_num++;

		cur_codec_format = cur_load_info->load_param.codec_format;
		total_load_pixels += cur_load_info->load_param.width
				   * cur_load_info->load_param.height
				   * cur_load_info->load_param.frame_rate;
	}

	VE_LOGV("dec_ch_num = %d, enc_ch_num = %d, had_diff_codec_ch = %d, cur_format = %d, "
		"total_p = %u\n",
		decoder_channel_num, encoder_channel_num, had_different_codec_channel,
		cur_codec_format, total_load_pixels);

	if ((decoder_channel_num + encoder_channel_num) == 0) {
		return 0;
	}

	/* not adjust ve freq */
	if (encoder_channel_num > 0 || decoder_channel_num > 2
	 || had_different_codec_channel == 1) {
		setup_ve_freq(cedar_devp, cedar_devp->user_setting_ve_freq);
		return 0;
	}

	/* match the performance */
	array_size = cedar_devp->performat_num;
	for (i = 0; i < array_size; ++i) {
		VE_LOGV("i = %d, format = %d, start_p = %d, end_p = %d, ve_freq = %d\n", i,
			performat_infos[i].codec_formmat,
			performat_infos[i].start_pixels,
			performat_infos[i].end_pixels,
			performat_infos[i].ve_freq);
		if (cur_codec_format == performat_infos[i].codec_formmat
		 && total_load_pixels >= performat_infos[i].start_pixels
		 && total_load_pixels <= performat_infos[i].end_pixels)
			break;
	}
	/* can not match perform, not adjust ve freq */
	if (i >= array_size) {
		VE_LOGW("cannot match performance info, it maybe wrong, i = %d, array_size = %d\n",
			i, array_size);
		return 0;
	}
	VE_LOGV("i = %d, total_p = %u, format = %d, start_p = %d, end_p = %d, ve_freq = %d\n",
		i, total_load_pixels, cur_codec_format,
		performat_infos[i].start_pixels,
		performat_infos[i].end_pixels, performat_infos[i].ve_freq);

	adjust_new_ve_freq = performat_infos[i].ve_freq;
	setup_ve_freq(cedar_devp, adjust_new_ve_freq);
	VE_LOGV("VE adjust_ve_freq=%d M\n", adjust_new_ve_freq);

	return 0;
}

/* note: setup_ve_freq() can only be called by ve is not working. */
static long setup_ve_freq(struct cedar_dev *cedar_devp, int ve_freq)
{
	long ret = 0;

	disable_cedar_hw_clk_module(cedar_devp);
	if (ve_freq >= VE_CLK_LOW_WATER && ve_freq <= VE_CLK_HIGH_WATER) {
		if (clk_set_rate(cedar_devp->ve_clk, ve_freq * 1000000)) {
			VE_LOGW("set clock failed\n");
		}
	}
	ret = clk_get_rate(cedar_devp->ve_clk);
	VE_LOGD("VE set ve_freq %d real_freq = %ld MHz\n", ve_freq, ret / 1000000);
	enable_cedar_hw_clk_module(cedar_devp);

	return ret;
}

static int enable_cedar_hw_clk_reset(struct cedar_dev *cedar_devp)
{
	int ret = 0;

	#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	if (reset_control_deassert(cedar_devp->reset_ve)) {
		VE_LOGW("reset_control_deassert reset_ve failed\n");
		ret = -EINVAL;
		goto out;
	}
	#endif
	if (reset_control_deassert(cedar_devp->reset)) {
		VE_LOGW("reset_control_deassert reset failed\n");
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
}

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
static void set_vcu_en_regmode_to1_to0(struct cedar_dev *cedar_devp)
{
	#define SUN60IW2_SUN55IW6_TOP_RESET_REG	(0x800 + 0x04)
	volatile char *ve_top_reset_reg = cedar_devp->iomap_addrs.regs_ve + SUN60IW2_SUN55IW6_TOP_RESET_REG;
	uint32_t ve_top_reset_reg_value = 0;
	//VE_LOGI("regs_ve 0x%x ve_top_reset_reg : 0x%x", cedar_devp->iomap_addrs.regs_ve, ve_top_reset_reg);
	//write bit0/bit4 to 1
	ve_top_reset_reg_value = readl((void *)ve_top_reset_reg);
	ve_top_reset_reg_value |= 0x1;
	ve_top_reset_reg_value |= (1 << 4);
	writel(ve_top_reset_reg_value, (void *)ve_top_reset_reg);
	//write bit0/bit4 to 0
	ve_top_reset_reg_value = readl((void *)ve_top_reset_reg);
	ve_top_reset_reg_value &= ~0x1;
	ve_top_reset_reg_value &= ~(1 << 4);
	writel(ve_top_reset_reg_value, (void *)ve_top_reset_reg);
}
#endif

static void disable_cedar_hw_clk_reset(struct cedar_dev *cedar_devp)
{
	reset_control_assert(cedar_devp->reset);
	#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	reset_control_assert(cedar_devp->reset_ve);
	#endif
}

static int enable_cedar_hw_clk_module(struct cedar_dev *cedar_devp)
{
	int ret = 0;

	#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	if (clk_prepare_enable(cedar_devp->bus_ve_clk)) {
		VE_LOGW("enable bus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	if (clk_prepare_enable(cedar_devp->bus_ve3_clk)) {
		VE_LOGW("enable bus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	if (clk_prepare_enable(cedar_devp->mbus_ve3_clk)) {
		VE_LOGW("enable mbus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	#else
	#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
	if (clk_prepare_enable(cedar_devp->sbus_clk)) {
		VE_LOGW("enable sbus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	if (clk_prepare_enable(cedar_devp->hbus_clk)) {
		VE_LOGW("enable hbus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	#endif
	#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	if (clk_prepare_enable(cedar_devp->ahb_gate)) {
		VE_LOGW("enable ahb_gate failed\n");
		ret = -EINVAL;
		goto out;
	}
	if (clk_prepare_enable(cedar_devp->mbus_gate)) {
		VE_LOGW("enable mbus_gate failed\n");
		ret = -EINVAL;
		goto out;
	}
	#endif
	if (clk_prepare_enable(cedar_devp->mbus_clk)) {
		VE_LOGW("enable mbus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	if (clk_prepare_enable(cedar_devp->bus_clk)) {
		VE_LOGW("enable bus clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
	#endif
	if (clk_prepare_enable(cedar_devp->ve_clk)) {
		VE_LOGW("enable ve clk gating failed\n");
		ret = -EINVAL;
		goto out;
	}
out:
	return ret;
}

static void disable_cedar_hw_clk_module(struct cedar_dev *cedar_devp)
{
	clk_disable_unprepare(cedar_devp->ve_clk);
	#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	clk_disable_unprepare(cedar_devp->mbus_ve3_clk);
	clk_disable_unprepare(cedar_devp->bus_ve3_clk);
	clk_disable_unprepare(cedar_devp->bus_ve_clk);
	#else
	clk_disable_unprepare(cedar_devp->bus_clk);
	clk_disable_unprepare(cedar_devp->mbus_clk);
	#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
	clk_disable_unprepare(cedar_devp->sbus_clk);
	clk_disable_unprepare(cedar_devp->hbus_clk);
	#endif
	#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	clk_disable_unprepare(cedar_devp->mbus_gate);
	clk_disable_unprepare(cedar_devp->ahb_gate);
	#endif
	#endif
}

static int enable_cedar_hw_clk(struct cedar_dev *cedar_devp)
{
	unsigned long flags;
	int ret = 0;

	VE_LOGD("enable hw clock start\n");
	if (cedar_devp->bInitEndFlag != 1) {
		VE_LOGW("VE Init not complete, forbid enable hw clk!\n");
		ret = -EINVAL;
		goto out;
	}
	spin_lock_irqsave(&cedar_devp->lock, flags);
	if (cedar_devp->clk_status == 1) {
		spin_unlock_irqrestore(&cedar_devp->lock, flags);
		goto out;
	}
	cedar_devp->clk_status = 1;
	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	pm_runtime_get_sync(cedar_devp->plat_dev);

	if (enable_cedar_hw_clk_reset(cedar_devp)) {
		ret = -EINVAL;
		goto out;
	}
	if (enable_cedar_hw_clk_module(cedar_devp)) {
		ret = -EINVAL;
		goto out;
	}
out:
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	if (ret == 0) {
		VE_LOGI("execute set_vcu_en_regmode_to1_to0");
		set_vcu_en_regmode_to1_to0(cedar_devp);
	}
#endif
	return ret;
}

static int disable_cedar_hw_clk(struct cedar_dev *cedar_devp)
{
	unsigned long flags;
	int ret = 0;

	VE_LOGD("disable hw clock\n");
	spin_lock_irqsave(&cedar_devp->lock, flags);
	if (cedar_devp->clk_status == 0) {
		spin_unlock_irqrestore(&cedar_devp->lock, flags);
		goto out;
	}
	cedar_devp->clk_status = 0;
	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	disable_cedar_hw_clk_module(cedar_devp);
	disable_cedar_hw_clk_reset(cedar_devp);

	pm_runtime_put_sync(cedar_devp->plat_dev);
	/* unmap_dma_buf_addr(1, 0, filp); */
out:
	return ret;
}

static int map_dma_buf_addr(struct cedar_dev *cedar_devp,
			    int fd, unsigned int *addr)
{
	struct sg_table *sgt;
	struct dma_buf_info *buf_info;

	buf_info = (struct dma_buf_info *)kmalloc(sizeof(*buf_info), GFP_KERNEL);
	if (buf_info == NULL) {
		VE_LOGE("malloc dma_buf_info error\n");
		return -1;
	}

	memset(buf_info, 0, sizeof(*buf_info));
	buf_info->dma_buf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(buf_info->dma_buf)) {
		VE_LOGE("ve get dma_buf error\n");
		goto BUF_FREE;
	}

	buf_info->attachment = dma_buf_attach(buf_info->dma_buf, cedar_devp->plat_dev);
	if (IS_ERR_OR_NULL(buf_info->attachment)) {
		VE_LOGE("ve get dma_buf_attachment error\n");
		goto BUF_PUT;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	sgt = dma_buf_map_attachment_unlocked(buf_info->attachment, DMA_BIDIRECTIONAL);
#else
	sgt = dma_buf_map_attachment(buf_info->attachment, DMA_BIDIRECTIONAL);
#endif
	buf_info->sgt = sgt;
	if (IS_ERR_OR_NULL(buf_info->sgt)) {
		VE_LOGE("ve get sg_table error\n");
		goto BUF_DETATCH;
	}

	buf_info->addr = sg_dma_address(buf_info->sgt->sgl);
	buf_info->fd = fd;
	buf_info->p_id = current->tgid;
	buf_info->cedar_devp = cedar_devp;
	#if PRINTK_IOMMU_ADDR
	VE_LOGD("fd:%d, buf_info:%p addr:%lx, dma_buf:%p," \
		"dma_buf_attach:%p, sg_table:%p, nents:%d, pid:%d\n",
		buf_info->fd,
		buf_info,
		buf_info->addr,
		buf_info->dma_buf,
		buf_info->attachment,
		buf_info->sgt,
		buf_info->sgt->nents,
		buf_info->p_id);
	#endif

	mutex_lock(&cedar_devp->lock_mem);
	aw_mem_list_add_tail(&buf_info->i_list, &cedar_devp->list);
	mutex_unlock(&cedar_devp->lock_mem);

	*addr = buf_info->addr;
	return 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	dma_buf_unmap_attachment_unlocked(buf_info->attachment, buf_info->sgt,
							DMA_BIDIRECTIONAL);
#else
	dma_buf_unmap_attachment(buf_info->attachment, buf_info->sgt,
							DMA_BIDIRECTIONAL);
#endif
BUF_DETATCH:
	dma_buf_detach(buf_info->dma_buf, buf_info->attachment);
BUF_PUT:
	dma_buf_put(buf_info->dma_buf);
BUF_FREE:
	kfree(buf_info);
	return -1;
}

static void unmap_dma_buf_addr(struct cedar_dev *cedar_devp,
			       int unmap_all, int fd)
{
	struct dma_buf_info *buf_info;
	struct aw_mem_list_head *pos;
	struct aw_mem_list_head *next;
	int tmp_fd;

	mutex_lock(&cedar_devp->lock_mem);
	/*
	 * aw_mem_list_for_each_entry(buf_info, &cedar_devp->list, i_list) {
	 * #if PRINTK_IOMMU_ADDR
	 *	VE_LOGD("list2 fd:%d, buf_info:%p addr:%lx, dma_buf:%p,"
	 *		"dma_buf_attach:%p, sg_table:%p, nents:%d, pid:%d\n",
	 *		buf_info->fd,
	 *		buf_info,
	 *		buf_info->addr,
	 *		buf_info->dma_buf,
	 *		buf_info->attachment,
	 *		buf_info->sgt,
	 *		buf_info->sgt->nents,
	 *		buf_info->p_id);
	 *	#endif
	 * }
	 */
	if (cedar_devp->bMemDevAttachFlag) {
		aw_mem_list_for_each_safe(pos, next, &cedar_devp->list) {
			buf_info = (struct dma_buf_info *)pos;
			if (unmap_all) {
				tmp_fd = buf_info->fd;
			} else {
				tmp_fd = fd;
			}

			if (buf_info->fd == tmp_fd && buf_info->p_id == current->tgid && cedar_devp == buf_info->cedar_devp) {
				#if PRINTK_IOMMU_ADDR
				VE_LOGD("free: fd:%d, buf_info:%p " \
					"iommu_addr:%lx, dma_buf:%p, dma_buf_attach:%p, " \
					"sg_table:%p nets:%d, pid:%d unmapall:%d cedar_devp:%p\n",
					buf_info->fd,
					buf_info,
					buf_info->addr,
					buf_info->dma_buf,
					buf_info->attachment,
					buf_info->sgt,
					buf_info->sgt->nents,
					buf_info->p_id,
					unmap_all,
					cedar_devp);
				#endif
				if (buf_info->dma_buf != NULL) {
					if (buf_info->attachment != NULL) {
						if (buf_info->sgt != NULL) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
							dma_buf_unmap_attachment_unlocked(buf_info->attachment,
										 buf_info->sgt,
										 DMA_BIDIRECTIONAL);
#else
							dma_buf_unmap_attachment(buf_info->attachment,
										 buf_info->sgt,
										 DMA_BIDIRECTIONAL);
#endif
						}
						dma_buf_detach(buf_info->dma_buf, buf_info->attachment);
					}
					dma_buf_put(buf_info->dma_buf);
				}
				aw_mem_list_del(&buf_info->i_list);
				kfree(buf_info);
				if (unmap_all == 0)
					break;
			}
		}
	}
	mutex_unlock(&cedar_devp->lock_mem);
}

static long _compat_cedardev_ioctl(struct ve_info *info, unsigned int cmd, unsigned long arg,
				   unsigned long from_kernel)
{
	long ret = 0;
	int ve_timeout = 0;
	/* struct cedar_dev *devp; */
	unsigned long flags;
	struct cedar_dev *cedar_devp;

	cedar_devp = info->cedar_devp;

	switch (cmd) {
	case IOCTL_GET_VE_TOP_REG_OFFSET: {
		return cedar_devp->ve_top_reg_offset;
	}
	case IOCTL_UPDATE_CASE_LOAD_PARAM: {
		struct ve_case_load_param load_param;

		if (cedar_devp->case_load_channels == 0) {
			VE_LOGW("unsupport IOCTL_UPDATE_CASE_LOAD_PARAM\n");
			return 0;
		}
		if (copy_from_user(&load_param, (void __user *)arg,
				   sizeof(struct ve_case_load_param))) {
			VE_LOGW("IOCTL_GET_VE_DEFAULT_FREQ copy_from_user fail\n");
			return -EFAULT;
		}
		vdfs_update_case_load_param(cedar_devp, &load_param, info->process_channel_id);
		vdfs_adjust_ve_freq_by_case_load(cedar_devp);
		return 0;
	}
	case IOCTL_GET_VE_DEFAULT_FREQ: {
		if (cedar_devp->dvfs_attr.default_freq == 0) {
			VE_LOGW("unsupport IOCTL_GET_VE_DEFAULT_FREQ\n");
		}
		return cedar_devp->dvfs_attr.default_freq;
	}
	case IOCTL_PROC_INFO_COPY: {
		int i = 0;
		struct ve_debug_info *debug_info = NULL;
		struct debug_head_info head_info;

		/* parser head info */
		cedar_devp->debug_info_cur_index = -1;
		if (copy_from_user(&head_info, (void __user *)arg, sizeof(head_info))) {
			VE_LOGW("SET_DEBUG_INFO copy_from_user fail\n");
			return -EFAULT;
		}

		/* VE_LOGD("size of debug_head_info: %d\n", (int)sizeof(&head_info));
		 * VE_LOGD("head_info: %d, %d, %d\n",
		 *	   head_info.pid, head_info.tid,head_info.length);
		 */
		mutex_lock(&cedar_devp->lock_debug_info);
		/* match debug info by pid and tid */
		for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
			if (cedar_devp->debug_info[i].head_info.pid == head_info.pid
			&& cedar_devp->debug_info[i].head_info.tid == head_info.tid)
				break;
		}

		if (i >= MAX_VE_DEBUG_INFO_NUM) {
			for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
				if (cedar_devp->debug_info[i].head_info.pid == 0
				&& cedar_devp->debug_info[i].head_info.tid == 0)
					break;
			}

			if (i >= MAX_VE_DEBUG_INFO_NUM) {
				VE_LOGW("SET_DEBUG_INFO  overflow  MAX_VE_DEBUG_INFO_NUM[%d]\n",
					MAX_VE_DEBUG_INFO_NUM);
			} else {
				cedar_devp->debug_info_cur_index = i;
				cedar_devp->debug_info[i].head_info.pid    = head_info.pid;
				cedar_devp->debug_info[i].head_info.tid    = head_info.tid;
				cedar_devp->debug_info[i].head_info.length = head_info.length;
			}
		} else {
			cedar_devp->debug_info_cur_index = i;
			cedar_devp->debug_info[i].head_info.length = head_info.length;
		}

		if (cedar_devp->debug_info_cur_index < 0) {
			VE_LOGE("COPY_DEBUG_INFO, cur_index[%d] is invalided\n",
				cedar_devp->debug_info_cur_index);
			mutex_unlock(&cedar_devp->lock_debug_info);
			return -EFAULT;
		}

		/* copy data of debug info */
		debug_info = &cedar_devp->debug_info[cedar_devp->debug_info_cur_index];

		/* VE_LOGD("COPY_DEBUG_INFO, index = %d, pid = %d, tid = %d, len = %d, data = %p\n", */
		/*    cedar_devp->debug_info_cur_index, debug_info->head_info.pid, */
		/*    debug_info->head_info.tid, debug_info->head_info.length, debug_info->data); */
		if (debug_info->data)
			vfree(debug_info->data);

		debug_info->data = vmalloc(debug_info->head_info.length);

		/* VE_LOGD("vmalloc: data = %p, len = %d\n", */
		/*    debug_info->data, debug_info->head_info.length); */
		if (debug_info->data == NULL) {
			mutex_unlock(&cedar_devp->lock_debug_info);
			VE_LOGE("vmalloc for debug_info->data failed\n");
			return -EFAULT;
		}

		if (copy_from_user(debug_info->data, (void __user *)(arg + sizeof(head_info)),
				   debug_info->head_info.length)) {
			vfree(debug_info->data);
			mutex_unlock(&cedar_devp->lock_debug_info);
			VE_LOGE("COPY_DEBUG_INFO copy_from_user fail\n");
			return -EFAULT;
		}
		mutex_unlock(&cedar_devp->lock_debug_info);
		break;
	}
	case IOCTL_PROC_INFO_STOP: {
		int i = 0;
		struct debug_head_info head_info;

		if (copy_from_user(&head_info, (void __user *)arg, sizeof(head_info))) {
			VE_LOGW("SET_DEBUG_INFO copy_from_user fail\n");
			return -EFAULT;
		}

		/* VE_LOGD("STOP_DEBUG_INFO, pid = %d, tid = %d\n", */
		/*		head_info.pid, head_info.tid); */
		mutex_lock(&cedar_devp->lock_debug_info);
		for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
			if (cedar_devp->debug_info[i].head_info.pid == head_info.pid
				&& cedar_devp->debug_info[i].head_info.tid == head_info.tid)
				break;
		}

		VE_LOGD("STOP_DEBUG_INFO, i = %d\n", i);
		if (i >= MAX_VE_DEBUG_INFO_NUM) {
			VE_LOGE("STOP_DEBUG_INFO: can not find the match pid[%d] and tid[%d]\n",
				head_info.pid, head_info.tid);
		} else {
			if (cedar_devp->debug_info[i].data)
				vfree(cedar_devp->debug_info[i].data);

			cedar_devp->debug_info[i].data = NULL;
			cedar_devp->debug_info[i].head_info.pid = 0;
			cedar_devp->debug_info[i].head_info.tid = 0;
			cedar_devp->debug_info[i].head_info.length = 0;
		}

		mutex_unlock(&cedar_devp->lock_debug_info);
		break;
	}
	case IOCTL_ENGINE_REQ: {
		if (down_interruptible(&cedar_devp->sem))
			return -ERESTARTSYS;

		if (cedar_devp->ref_count < 0) {
			VE_LOGW("request ve ref count error!\n");
			up(&cedar_devp->sem);
			return -EFAULT;
		}

		cedar_devp->ref_count++;
		if (cedar_devp->ref_count == 1) {
			cedar_devp->last_min_freq = 0;
			ret = enable_cedar_hw_clk(cedar_devp);
			if (ret < 0) {
				cedar_devp->ref_count--;
				up(&cedar_devp->sem);
				VE_LOGW("IOCTL_ENGINE_REQ clk enable error!\n");
				return -EFAULT;
			}
		}
		up(&cedar_devp->sem);
		break;
	}
	case IOCTL_ENGINE_REL: {
		if (down_interruptible(&cedar_devp->sem))
			return -ERESTARTSYS;
		cedar_devp->ref_count--;

		if (cedar_devp->ref_count < 0) {
			VE_LOGW("relsese ve ref count error!\n");
			up(&cedar_devp->sem);
			return -EFAULT;
		}

		if (cedar_devp->ref_count == 0) {
			ret = disable_cedar_hw_clk(cedar_devp);
			if (ret < 0) {
				VE_LOGW("IOCTL_ENGINE_REL clk disable error!\n");
				up(&cedar_devp->sem);
				return -EFAULT;
			}
		}
		up(&cedar_devp->sem);
		return ret;
	}
	case IOCTL_WAIT_VE_DE: {
		ve_timeout = (int)arg;
		cedar_devp->de_irq_value = 0;

		spin_lock_irqsave(&cedar_devp->lock, flags);
		if (cedar_devp->de_irq_flag)
			cedar_devp->de_irq_value = 1;
		spin_unlock_irqrestore(&cedar_devp->lock, flags);
		wait_event_timeout(cedar_devp->wait_ve, cedar_devp->de_irq_flag, ve_timeout*HZ);
		cedar_devp->de_irq_flag = 0;
		return cedar_devp->de_irq_value;
	}
	case IOCTL_WAIT_VE_EN: {
		ve_timeout = (int)arg;
		cedar_devp->en_irq_value = 0;

		spin_lock_irqsave(&cedar_devp->lock, flags);
		if (cedar_devp->en_irq_flag)
			cedar_devp->en_irq_value = 1;
		spin_unlock_irqrestore(&cedar_devp->lock, flags);

		wait_event_timeout(cedar_devp->wait_ve, cedar_devp->en_irq_flag, ve_timeout*HZ);
		cedar_devp->en_irq_flag = 0;

		return cedar_devp->en_irq_value;
	}
#if ((defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN50I) || \
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1) || \
	(defined CONFIG_ARCH_SUN8IW16P1) || (defined CONFIG_ARCH_SUN8IW19P1) || \
	(defined CONFIG_ARCH_SUN300IW1) || (defined CONFIG_ARCH_SUN55IW6))
	case IOCTL_WAIT_JPEG_DEC: {
		ve_timeout = (int)arg;
		cedar_devp->jpeg_irq_value = 0;

		spin_lock_irqsave(&cedar_devp->lock, flags);
		if (cedar_devp->jpeg_irq_flag)
			cedar_devp->jpeg_irq_value = 1;
		spin_unlock_irqrestore(&cedar_devp->lock, flags);

		wait_event_timeout(cedar_devp->wait_ve, cedar_devp->jpeg_irq_flag, ve_timeout*HZ);
		cedar_devp->jpeg_irq_flag = 0;
		return cedar_devp->jpeg_irq_value;
	}
#endif
	case IOCTL_ENABLE_VE: {
		VE_LOGW("IOCTL_ENABLE_VE nused, please check\n");
		break;
	}
	case IOCTL_DISABLE_VE: {
		VE_LOGW("IOCTL_DISABLE_VE nused, please check\n");
		break;
	}
	case IOCTL_RESET_VE: {
		reset_control_reset(cedar_devp->reset);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN300IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		set_vcu_en_regmode_to1_to0(cedar_devp);
	#endif
		break;
	}
	case IOCTL_SET_DRAM_HIGH_CHANNAL: {
		/*
		int flag = (int)arg;

		if (flag == 1)
			mbus_port_setpri(26, 1);
		else
			mbus_port_setpri(26, 0);

		VE_LOGW("this kernel version not supproti!\n");
		*/
		break;
	}
	case IOCTL_SET_VE_FREQ: {
		int arg_rate = (int)arg;

		VE_LOGD("VE setup_freq = %d MHz\n", arg_rate);
		ret = setup_ve_freq(cedar_devp, arg_rate);
		cedar_devp->user_setting_ve_freq = (int)ret / 1000000;
		break;
	}
	case IOCTL_GETVALUE_AVS2:
	case IOCTL_ADJUST_AVS2:
	case IOCTL_ADJUST_AVS2_ABS:
	case IOCTL_CONFIG_AVS2:
	case IOCTL_RESET_AVS2:
	case IOCTL_PAUSE_AVS2:
	case IOCTL_START_AVS2: {
		VE_LOGW("ioctrl(%u) nused, please check\n", cmd);
		break;
	}
	case IOCTL_GET_ENV_INFO: {
		struct cedarv_env_infomation_compat env_info;

		env_info.phymem_start = 0;
		env_info.phymem_total_size = 0;
		env_info.address_macc = 0;
		if (copy_to_user((char *)arg, &env_info, sizeof(env_info)))
			return -EFAULT;
		break;
	}
	case IOCTL_GET_IC_VER: {
		VE_LOGW("IOCTL_GET_IC_VER nused, please check\n");
		return 0;
	}
	case IOCTL_SET_REFCOUNT: {
		cedar_devp->ref_count = (int)arg;
		break;
	}
	case IOCTL_SET_VOL: {
		VE_LOGW("IOCTL_SET_VOL nused, please check\n");
		break;
	}
	case IOCTL_GET_LOCK: {
		int lock_ctl_ret = 0;
		u32 lock_type = arg;

		if (lock_type == VE_LOCK_VDEC)
			mutex_lock(&cedar_devp->lock_vdec);
		else if (lock_type == VE_LOCK_VENC)
			mutex_lock(&cedar_devp->lock_venc);
		else if (lock_type == VE_LOCK_JDEC)
			mutex_lock(&cedar_devp->lock_jdec);
		else if (lock_type == VE_LOCK_00_REG)
			mutex_lock(&cedar_devp->lock_00_reg);
		else if (lock_type == VE_LOCK_04_REG)
			mutex_lock(&cedar_devp->lock_04_reg);
		else
			VE_LOGE("invalid lock type '%d'\n", lock_type);

		if ((info->lock_flags&lock_type) != 0)
			VE_LOGE("when get lock, this should be 0!!!\n");

		mutex_lock(&info->lock_flag_io);
		info->lock_flags |= lock_type;
		mutex_unlock(&info->lock_flag_io);

		return lock_ctl_ret;
	}
	case IOCTL_SET_PROC_INFO: {
#if IS_ENABLED(CONFIG_DEBUG_FS)
		int ret;
		ret = ioctl_set_proc_info(cedar_devp->dbgfs,
					  arg, info);
		if (ret) {
			VE_LOGE("ioctl_set_proc_info failed\n");
			return ret;
		}
#else
		ret = 0;
#endif
		break;
	}
	case IOCTL_COPY_PROC_INFO: {
#if IS_ENABLED(CONFIG_DEBUG_FS)
		int ret;
		ret = ioctl_copy_proc_info(cedar_devp->dbgfs,
					   arg, info);
		if (ret) {
			VE_LOGE("ioctl_copy_proc_info failed\n");
			return ret;
		}
#else
		ret = 0;
#endif
		break;
	}
	case IOCTL_STOP_PROC_INFO: {
#if IS_ENABLED(CONFIG_DEBUG_FS)
		int ret;
		ret = ioctl_stop_proc_info(cedar_devp->dbgfs,
					   arg, info);
		if (ret) {
			VE_LOGE("ioctl_stop_proc_info failed\n");
			return ret;
		}
#else
		ret = 0;
#endif
		break;
	}
	case IOCTL_RELEASE_LOCK: {
		int lock_ctl_ret = 0;

		do {
			u32 lock_type = arg;

			if (!(info->lock_flags & lock_type)) {
				VE_LOGE("Not lock? flags: '%x/%x'.\n", info->lock_flags, lock_type);
				lock_ctl_ret = -1;
				break; /* break 'do...while' */
			}

			mutex_lock(&info->lock_flag_io);
			info->lock_flags &= (~lock_type);
			mutex_unlock(&info->lock_flag_io);

			if (lock_type == VE_LOCK_VDEC)
				mutex_unlock(&cedar_devp->lock_vdec);
			else if (lock_type == VE_LOCK_VENC)
				mutex_unlock(&cedar_devp->lock_venc);
			else if (lock_type == VE_LOCK_JDEC)
				mutex_unlock(&cedar_devp->lock_jdec);
			else if (lock_type == VE_LOCK_00_REG)
				mutex_unlock(&cedar_devp->lock_00_reg);
			else if (lock_type == VE_LOCK_04_REG)
				mutex_unlock(&cedar_devp->lock_04_reg);
			else
				VE_LOGE("invalid lock type '%d'\n", lock_type);
		} while (0);
		return lock_ctl_ret;
	}
	case IOCTL_GET_IOMMU_ADDR: {
		/* just for compatible, kernel 5.4 should not use it */
		struct user_iommu_param parm;
		cedar_devp->bMemDevAttachFlag = 1;

		data_to_kernel((void *)&parm, (void *)arg, sizeof(parm), from_kernel);
		if (map_dma_buf_addr(cedar_devp, parm.fd, &parm.iommu_addr) != 0) {
			VE_LOGE("IOCTL_GET_IOMMU_ADDR map dma buf fail\n");
			return -EFAULT;
		}

		data_to_other((void *)arg, &parm, sizeof(parm), from_kernel);
		break;
	}
	case IOCTL_FREE_IOMMU_ADDR: {
		/* just for compatible, kernel 5.4 should not use it */
		struct user_iommu_param parm;

		data_to_kernel((void *)&parm, (void *)arg, sizeof(parm), from_kernel);
		unmap_dma_buf_addr(cedar_devp, 0, parm.fd);
		break;
	}
	case IOCTL_MAP_DMA_BUF: {
		struct dma_buf_param parm;
		cedar_devp->bMemDevAttachFlag = 1;

		data_to_kernel((void *)&parm, (void *)arg, sizeof(parm), from_kernel);
		if (map_dma_buf_addr(cedar_devp, parm.fd, &parm.phy_addr) != 0) {
			VE_LOGE("IOCTL_GET_IOMMU_ADDR map dma buf fail\n");
			return -EFAULT;
		}
		data_to_other((void *)arg, &parm, sizeof(parm), from_kernel);
		break;
	}
	case IOCTL_UNMAP_DMA_BUF: {
		struct dma_buf_param parm;

		data_to_kernel((void *)&parm, (void *)arg, sizeof(parm), from_kernel);
		unmap_dma_buf_addr(cedar_devp, 0, parm.fd);
		break;
	}
	case IOCTL_FLUSH_CACHE_RANGE: {
		if (ioctl_flush_cache_range(arg, 1, cedar_devp)) {
			VE_LOGE("ioctl_flush_cache_range failed\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_POWER_SETUP: {
		VE_LOGW("IOCTL_POWER_SETUP nused, please check\n");
		break;
	}
	case IOCTL_POWER_SHUTDOWN: {
		VE_LOGW("IOCTL_POWER_SHUTDOWN nused, please check\n");
		break;
	}
	case IOCTL_VE_MODE: {
		enum VE_MODE ve_mode;
		ve_mode = (enum VE_MODE)arg;
		if (ve_mode >= VE_MODE_CNT || ve_mode <= VE_MODE_NULL) {
			ve_mode = VE_MODE_NULL;
			VE_LOGE("unsupport ve_mode: %d\n", ve_mode);
			return -1;
		}
		cedar_devp->ve_mode = ve_mode;
		VE_LOGD("ve_mode: %d\n", cedar_devp->ve_mode);
		return 0;
	}
	case IOCTL_WAIT_VCU_ENC: {
		u32 cur_irq_flag = 0;
		ve_timeout = (int)arg; /* unit: s */

		wait_event_timeout(cedar_devp->wait_ve,
				   cedar_devp->vcuenc_irq_flag, ve_timeout * HZ);
		cur_irq_flag = cedar_devp->vcuenc_irq_flag;
		cedar_devp->vcuenc_irq_flag = 0;

		return cur_irq_flag;
	}
	case IOCTL_WAIT_VCU_DEC: {
		u32 cur_irq_flag = 0;
		ve_timeout = (int)arg; /* unit: s */
		wait_event_timeout(cedar_devp->wait_ve,
				   cedar_devp->vcudec_irq_flag, ve_timeout * HZ);
		cur_irq_flag = cedar_devp->vcudec_irq_flag;
		cedar_devp->vcudec_irq_flag = 0;

		return cur_irq_flag;
	}
	case IOCTL_GET_CSI_ONLINE_INFO: {
		if (ioctl_get_csi_online_related_info(arg, from_kernel, cedar_devp)) {
			VE_LOGE("ioctl_get_csi_online_related_info failed\n");
			return -EFAULT;
		}
		break;
	}
	case IOCTL_CLEAR_EN_INT_FLAG: {
		spin_lock_irqsave(&cedar_devp->lock, flags);
		cedar_devp->en_irq_flag  = 0;
		cedar_devp->en_irq_value = 0;
		spin_unlock_irqrestore(&cedar_devp->lock, flags);
		break;
	}
	default:
		VE_LOGW("not support the ioctl cmd = 0x%x\n", cmd);
		return -1;
	}
	return ret;
}

static long compat_cedardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* struct cedar_dev *devp; */
	struct ve_info *info;

	info = filp->private_data;

	return _compat_cedardev_ioctl(info, cmd, arg, 0);
}

struct device *cedar_get_device(const char *dev_name)
{
	struct cedar_dev *cedar_devp = NULL;

	if (!dev_name) {
		VE_LOGE("dev name invalid\n");
		return NULL;
	}

	if (!strcmp(dev_name, cedar_ve_quirk.dev_name)) {
		cedar_devp = (struct cedar_dev *)cedar_ve_quirk.priv;
	} else if (!strcmp(dev_name, cedar_ve2_quirk.dev_name)) {
		cedar_devp = (struct cedar_dev *)cedar_ve2_quirk.priv;
	} else {
		VE_LOGE("dev name unmatch %s\n", dev_name);
		return NULL;
	}
	if (!cedar_devp) {
		VE_LOGE("dev %s maybe uinit\n", dev_name);
		return NULL;
	}
	return cedar_devp->plat_dev;
}
EXPORT_SYMBOL(cedar_get_device);

int rt_cedardev_ioctl(void *info_handle, unsigned int cmd, unsigned long arg)
{//todo, use for kernel ve code.
	struct ve_info *info = (struct ve_info *)info_handle;
	return _compat_cedardev_ioctl(info, cmd, arg, 1);
}

static struct ve_info *_cedardev_open(struct cedar_dev *cedar_devp)
{
	struct ve_info *info;
	unsigned long flags;

	if (cedar_devp->bInitEndFlag != 1) {
		VE_LOGW("VE Init not complete, forbid cedardev open!\n");
		return NULL;
	}

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return NULL;

	info->set_vol_flag = 0;

	spin_lock_irqsave(&cedar_devp->lock, flags);

	cedar_devp->process_channel_id_cnt++;
	if (cedar_devp->process_channel_id_cnt > 0xefffffff)
		cedar_devp->process_channel_id_cnt = 0;

	info->process_channel_id = cedar_devp->process_channel_id_cnt;

	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	if (down_interruptible(&cedar_devp->sem)) {
		kfree(info);
		return NULL;
	}

	/* init other resource here */
	if (cedar_devp->ref_count == 0) {
		cedar_devp->de_irq_flag = 0;
		cedar_devp->en_irq_flag = 0;
		cedar_devp->jpeg_irq_flag = 0;
	}

	up(&cedar_devp->sem);

	mutex_init(&info->lock_flag_io);
	info->lock_flags = 0;

	return info;
}

static int cedardev_open(struct inode *inode, struct file *filp)
{
	struct ve_info *info = NULL;
	struct cedar_dev *cedar_devp = container_of(inode->i_cdev, struct cedar_dev, cdev);

	info = _cedardev_open(cedar_devp);
	if (!info) {
		VE_LOGE("_cedardev_open failed\n");
		return -1;
	}
	info->cedar_devp = cedar_devp;

	filp->private_data = info;
	nonseekable_open(inode, filp);

#if IS_ENABLED(CONFIG_DEBUG_FS)
	ve_debug_open(cedar_devp->dbgfs, info);
#endif
	return 0;
}

void *rt_cedardev_open(const char *dev_name)
{
	struct ve_info *info = NULL;
	struct cedar_dev *cedar_devp = NULL;

	if (!dev_name) {
		VE_LOGE("dev name invalid\n");
		return NULL;
	}

	if (!strcmp(dev_name, cedar_ve_quirk.dev_name)) {
		cedar_devp = (struct cedar_dev *)cedar_ve_quirk.priv;
	} else if (!strcmp(dev_name, cedar_ve2_quirk.dev_name)) {
		cedar_devp = (struct cedar_dev *)cedar_ve2_quirk.priv;
	} else {
		VE_LOGE("dev name unmatch %s\n", dev_name);
		return NULL;
	}
	if (!cedar_devp) {
		VE_LOGE("dev %s maybe uinit\n", dev_name);
		return NULL;
	}

	info = _cedardev_open(cedar_devp);
	if (!info) {
		VE_LOGE("_cedardev_open failed\n");
		return NULL;
	}
	info->cedar_devp = cedar_devp;

#if IS_ENABLED(CONFIG_DEBUG_FS)
	ve_debug_open(cedar_devp->dbgfs, info);
#endif
	return info;
}

static int _cedardev_release(struct ve_info *info)
{
	struct cedar_dev *cedar_devp = info->cedar_devp;

	mutex_lock(&info->lock_flag_io);
	/*
	 * if the process abort, this will free iommu_buffer
	 * unmap_dma_buf_addr(1, 0, filp);
	 */

#if IS_ENABLED(CONFIG_DEBUG_FS)
	ve_debug_release(cedar_devp->dbgfs, info);
#endif

	/* lock status */
	if (info->lock_flags) {
		VE_LOGW("release lost-lock...\n");
		if (info->lock_flags & VE_LOCK_VDEC)
			mutex_unlock(&cedar_devp->lock_vdec);

		if (info->lock_flags & VE_LOCK_VENC)
			mutex_unlock(&cedar_devp->lock_venc);

		if (info->lock_flags & VE_LOCK_JDEC)
			mutex_unlock(&cedar_devp->lock_jdec);

		if (info->lock_flags & VE_LOCK_00_REG)
			mutex_unlock(&cedar_devp->lock_00_reg);

		if (info->lock_flags & VE_LOCK_04_REG)
			mutex_unlock(&cedar_devp->lock_04_reg);
		info->lock_flags = 0;
	}

	mutex_unlock(&info->lock_flag_io);
	mutex_destroy(&info->lock_flag_io);

	if (down_interruptible(&cedar_devp->sem)) {
		return -ERESTARTSYS;
	}

	/* release other resource here */
	if (cedar_devp->ref_count == 0) {
		cedar_devp->de_irq_flag = 1;
		cedar_devp->en_irq_flag = 1;
		cedar_devp->jpeg_irq_flag = 1;
	}
	up(&cedar_devp->sem);

	/* if the process abort, this will remove case load by process id */
	vdfs_remove_case_load_param(cedar_devp, info->process_channel_id);
	kfree(info);
	return 0;
}

static int cedardev_release(struct inode *inode, struct file *filp)
{
	return _cedardev_release((struct ve_info *)(filp->private_data));
}

int rt_cedardev_release(void *info_handle)
{
	return _cedardev_release((struct ve_info *)info_handle);
}

static void cedardev_vma_open(struct vm_area_struct *vma)
{
}

static void cedardev_vma_close(struct vm_area_struct *vma)
{
}

static struct vm_operations_struct cedardev_remap_vm_ops = {
	.open  = cedardev_vma_open,
	.close = cedardev_vma_close,
};

static int cedardev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct ve_info *info = filp->private_data;
	struct cedar_dev *cedar_devp = info->cedar_devp;
	unsigned long temp_pfn;

	if (vma->vm_end - vma->vm_start == 0) {
		VE_LOGW("vma->vm_end is equal vma->vm_start : %lx\n", vma->vm_start);
		return 0;
	}
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT)) {
		VE_LOGW("the vma->vm_pgoff is %lx,it is large than the largest page number\n",
			vma->vm_pgoff);
		return -EINVAL;
	}

	temp_pfn = cedar_devp->iomap_addrs.ve_reg_start >> 12;

	/* Set reserved and I/O flag for the area. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 25))
	vm_flags_set(vma, VM_IO);
#else
	vma->vm_flags |= /* VM_RESERVED | */VM_IO;
#endif
	/* Select uncached access. */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
				vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}


	vma->vm_ops = &cedardev_remap_vm_ops;
	cedardev_vma_open(vma);

	return 0;
}

void *rt_cedardev_mmap(void *info_handle)
{
	struct ve_info *info = (struct ve_info *)info_handle;
	struct cedar_dev *cedar_devp = info->cedar_devp;

	return (void *)cedar_devp->iomap_addrs.regs_ve;
}

static int sunxi_cedar_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(&pdev->dev);
	int ret = 0;

	VE_LOGD("standby suspend\n");
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
	SetVeSuspend();
#endif
#endif
	ret = disable_cedar_hw_clk(cedar_devp);
	if (ret < 0) {
		VE_LOGW("cedar clk disable somewhere error!\n");
		return -EFAULT;
	}

	return 0;
}

static int sunxi_cedar_resume(struct platform_device *pdev)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(&pdev->dev);
	int ret = 0;

	VE_LOGD("standby resume\n");

	if (cedar_devp->ref_count == 0) {
		return 0;
	}

	set_system_register(cedar_devp);
	ret = enable_cedar_hw_clk(cedar_devp);
	if (ret < 0) {
		VE_LOGW("cedar clk enable somewhere error!\n");
		return -EFAULT;
	}
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
	SetVeResume();
#endif
#endif

	return 0;
}
#if IS_ENABLED(CONFIG_DEBUG_FS)
static int vefs_open(struct inode *inode, struct file *file)
{
	struct cedar_dev *cedar_devp = (struct cedar_dev *)(inode->i_private);
	file->private_data = cedar_devp;
	return 0;
}

static ssize_t vefs_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
	struct cedar_dev *cedar_devp = file->private_data;
	struct ve_dvfs_attr *attr = &cedar_devp->dvfs_attr;
	struct ve_dvfs_info *dvfs_array = attr->dvfs_array;
	char buf[512] = {0};
	char *buf_tmp;
	int i = 0, len = 0, total = 0;

	buf_tmp = buf;
	while (i < attr->dvfs_array_num) {
		total += scnprintf(buf_tmp + total, sizeof(buf) - total,
				   "ve_dvfs idx %02u %03umv %03uMhz\n",
				   dvfs_array[i].dvfs_index,
				   dvfs_array[i].voltage,
				   dvfs_array[i].ve_freq);
		i += 1;
	}

	len = strlen(buf);
	if (*ppos != 0) {
		return 0;
	}
	if (copy_to_user(user_buf, buf, len) != 0) {
		return -EFAULT;
	}
	*ppos = len;

	return len;
}

static const struct file_operations vedvfs_fops = {
	.open = vefs_open,
	.read = vefs_read,
};

static int vedvfs_init(struct cedar_dev *cedar_devp)
{
	struct dentry *dent;
	char *dfs_name = NULL;
	char *dfs_name_nor = "ve_dvfs";
	char *dfs_name_2 = "ve_dvfs2";

	if (cedar_devp->quirks->ve_mod == VE_MODULE_2)
		dfs_name = dfs_name_2;
	else
		dfs_name = dfs_name_nor;

	cedar_devp->dvfs_root = debugfs_lookup("ve", NULL);
	if (IS_ERR_OR_NULL(cedar_devp->dvfs_root)) {
		cedar_devp->dvfs_root = debugfs_create_dir("ve", NULL);
		if (IS_ERR_OR_NULL(cedar_devp->dvfs_root)) {
			VE_LOGE("debugfs root is null please check!\n");
			return -ENOENT;
		}
	}
	dent = debugfs_create_file(dfs_name, 0444, cedar_devp->dvfs_root,
				   cedar_devp, &vedvfs_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(cedar_devp->dvfs_root);
		cedar_devp->dvfs_root = NULL;
		return -ENODEV;
	}
	return 0;
}
#endif

static void set_sys_cfg_by_self(struct cedar_dev *cedar_devp)
{
	unsigned int val;
	if (cedar_devp->iomap_addrs.regs_sys_cfg == NULL) {
		VE_LOGW("sys config regs addr is null\n");
		return;
	}
#if IS_ENABLED(CONFIG_ARCH_SUN8I)
	val = readl(cedar_devp->iomap_addrs.regs_sys_cfg);
	val &= 0x80000000;
	writel(val, cedar_devp->iomap_addrs.regs_sys_cfg);
	/* remapping SRAM to MACC for codec test */
	val = readl(cedar_devp->iomap_addrs.regs_sys_cfg);
	val |= 0x7fffffff;
	writel(val, cedar_devp->iomap_addrs.regs_sys_cfg);
	/* clear bootmode bit for give sram to ve */
	val = readl((cedar_devp->iomap_addrs.regs_sys_cfg + 0x1));
	val &= 0xfeffffff;
	writel(val, (cedar_devp->iomap_addrs.regs_sys_cfg + 0x1));
#elif IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
	/* clear bootmode bit for give sram to ve */
	val = readl((cedar_devp->iomap_addrs.regs_sys_cfg + 0x4));
	val &= 0xfeffffff;
	writel(val, (cedar_devp->iomap_addrs.regs_sys_cfg + 0x4));
#else
	/* remapping SRAM to MACC for codec test */
	val = readl(cedar_devp->iomap_addrs.regs_sys_cfg);
	val &= 0xfffffffe;
	writel(val, cedar_devp->iomap_addrs.regs_sys_cfg);
	/* clear bootmode bit for give sram to ve */
	val = readl((cedar_devp->iomap_addrs.regs_sys_cfg + 0x4));
	val &= 0xfeffffff;
	writel(val, (cedar_devp->iomap_addrs.regs_sys_cfg + 0x4));
#endif
}

static void set_system_register(struct cedar_dev *cedar_devp)
{
	/* modify sys_config register config */
	set_sys_cfg_by_self(cedar_devp);
}

static int deal_with_resouce(struct platform_device *pdev)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(&pdev->dev);
	const struct cedar_ve_quirks *quirks = cedar_devp->quirks;
	struct device_node *node = pdev->dev.of_node;
	unsigned int temp_val = 0;
	int ret = 0;

	/* 4.register irq function */
	cedar_devp->irq = irq_of_parse_and_map(node, 0);
	VE_LOGD("cedar-ve the get irq is %d\n", cedar_devp->irq);
	ret = request_irq(cedar_devp->irq, VideoEngineInterupt, 0, quirks->dev_name, cedar_devp);
	if (ret < 0) {
		VE_LOGW("request irq err\n");
		return -1;
	}

	/* get regs */
	memset(&cedar_devp->iomap_addrs, 0, sizeof(cedar_devp->iomap_addrs));
	if (resource_iomap_init(node, &cedar_devp->iomap_addrs)) {
		VE_LOGE("resource_iomap_init fail\n");
		return -1;
	}

	/* get clk */
	cedar_devp->ve_clk = devm_clk_get(cedar_devp->plat_dev, "ve");
	if (IS_ERR(cedar_devp->ve_clk)) {
		VE_LOGW("try to get ve clk fail\n");
		return -1;
	}

#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	cedar_devp->bus_ve_clk = devm_clk_get(cedar_devp->plat_dev, "bus_ve");
	if (IS_ERR(cedar_devp->bus_ve_clk)) {
		VE_LOGW("try to get ve clk fail\n");
		return -1;
	}

	cedar_devp->bus_ve3_clk = devm_clk_get(cedar_devp->plat_dev, "bus_ve3");
	if (IS_ERR(cedar_devp->bus_ve3_clk)) {
		VE_LOGW("try to get bus clk fail\n");
		return -1;
	}

	cedar_devp->mbus_ve3_clk = devm_clk_get(cedar_devp->plat_dev, "mbus_ve3");
	if (IS_ERR(cedar_devp->mbus_ve3_clk)) {
		VE_LOGW("try to get mbus clk fail\n");
		return -1;
	}
#else

	cedar_devp->bus_clk = devm_clk_get(cedar_devp->plat_dev, "bus_ve");
	if (IS_ERR(cedar_devp->bus_clk)) {
		VE_LOGW("try to get bus clk fail\n");
		return -1;
	}

	cedar_devp->mbus_clk = devm_clk_get(cedar_devp->plat_dev, "mbus_ve");
	if (IS_ERR(cedar_devp->mbus_clk)) {
		VE_LOGW("try to get mbus clk fail\n");
		return -1;
	}
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
	cedar_devp->sbus_clk = devm_clk_get(cedar_devp->plat_dev, "sbus_ve");
	if (IS_ERR(cedar_devp->sbus_clk)) {
		VE_LOGW("try to get sbus_clk clk fail\n");
		return -1;
	}
	cedar_devp->hbus_clk = devm_clk_get(cedar_devp->plat_dev, "hbus_ve");
	if (IS_ERR(cedar_devp->hbus_clk)) {
		VE_LOGW("try to get hbus_clk clk fail\n");
		return -1;
	}
	cedar_devp->pll_clk = devm_clk_get(cedar_devp->plat_dev, "pll_ve");
	if (IS_ERR(cedar_devp->pll_clk)) {
		VE_LOGW("try to get hbus_clk clk fail\n");
		return -1;
	}
	if (clk_set_parent(cedar_devp->ve_clk, cedar_devp->pll_clk)) {
		VE_LOGW("set ve parent clk failed\n");
		return -1;
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	cedar_devp->pll_ve = devm_clk_get(cedar_devp->plat_dev, "pll_ve");
	if (IS_ERR(cedar_devp->pll_ve)) {
		VE_LOGW("try to get pll_ve clk failed\n");
		return -1;
	}
	cedar_devp->ahb_gate = devm_clk_get(cedar_devp->plat_dev, "ahb_gate");
	if (IS_ERR(cedar_devp->ahb_gate)) {
		VE_LOGW("try to get ahb_gate clk failed\n");
		return -1;
	}
	cedar_devp->mbus_gate = devm_clk_get(cedar_devp->plat_dev, "mbus_gate");
	if (IS_ERR(cedar_devp->mbus_gate)) {
		VE_LOGW("try to get mbus_gate clk failed\n");
		return -1;
	}
	if (clk_set_parent(cedar_devp->ve_clk, cedar_devp->pll_ve)) {
		VE_LOGW("set ve parent clk failed\n");
		return -1;
	}
#endif
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	cedar_devp->reset = devm_reset_control_get(cedar_devp->plat_dev, "reset_ve3");
	if (IS_ERR(cedar_devp->reset)) {
		VE_LOGE("get reset ve3 fail\n");
		return -1;
	}

	cedar_devp->reset_ve = devm_reset_control_get_shared(cedar_devp->plat_dev, "reset_ve");
	if (IS_ERR(cedar_devp->reset_ve)) {
		VE_LOGE("get reset ve fail\n");
		return -1;
	}
#else
	cedar_devp->reset = devm_reset_control_get(cedar_devp->plat_dev, NULL);
	if (IS_ERR(cedar_devp->reset)) {
		VE_LOGE("get reset fail\n");
		return -1;
	}
#endif

	temp_val = 0;
	cedar_devp->ve_freq_setup_by_dts_config = 0;

	ret = of_property_read_u32(node, "enable_setup_ve_freq", &temp_val);
	if (ret < 0) {
		VE_LOGV("enable_setup_ve_freq_by_dts get failed\n");
	} else {
		VE_LOGD("enable_setup_ve_freq_by_dts = %d", temp_val);
		if (temp_val == 1) {
			temp_val = 0;
			ret = of_property_read_u32(node, "ve_freq_value", &temp_val);
			VE_LOGD("ve_freq_value = %d", temp_val);
			if (ret < 0) {
				VE_LOGV("ve_freq_value get failed\n");
			} else {
				cedar_devp->ve_freq_setup_by_dts_config = temp_val;
			}
		}
	}

	ret = of_property_read_u32(node, "ve_top_reg_offset", &temp_val);
	if (ret < 0) {
			VE_LOGV("ve_top_reg_offset get failed\n");
	} else {
		cedar_devp->ve_top_reg_offset = temp_val;
	}
	VE_LOGV("ret = %d, ve_top_reg_offset = 0x%x", ret, cedar_devp->ve_top_reg_offset);

	set_system_register(cedar_devp);
	ve_dvfs_get_attr(cedar_devp);
	return 0;
}

static const struct file_operations cedardev_fops = {
	.owner	 = THIS_MODULE,
	.mmap	 = cedardev_mmap,
	.open	 = cedardev_open,
	.release = cedardev_release,
	.llseek	 = no_llseek,
	.unlocked_ioctl	= compat_cedardev_ioctl,
	.compat_ioctl   = compat_cedardev_ioctl,
};

static int create_char_device(struct cedar_dev *cedar_devp)
{
	const struct cedar_ve_quirks *quirks = cedar_devp->quirks;
	int ret = 0;

	/* 1.register or alloc the device number. */
	ret = alloc_chrdev_region(&cedar_devp->ve_dev, 0, 1, quirks->dev_name);
	if (ret) {
		VE_LOGE("%s: can't get major %d\n", quirks->dev_name, MAJOR(cedar_devp->ve_dev));
		return ret;
	}

	/* 2.create char device */
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	ret = cdev_add(&cedar_devp->cdev, cedar_devp->ve_dev, 1);
	if (ret) {
		VE_LOGE("Err:%d add cedar-dev fail\n", ret);
		goto region_del;
	}
	/* 3.create class and new device for auto device node */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
	cedar_devp->class = class_create(quirks->class_name);
#else
	cedar_devp->class = class_create(THIS_MODULE, quirks->class_name);
#endif
	if (IS_ERR_OR_NULL(cedar_devp->class)) {
		VE_LOGE("class create fail\n");
		ret = -EINVAL;
		goto dev_del;
	}
	cedar_devp->dev = device_create(cedar_devp->class, NULL,
					cedar_devp->ve_dev, NULL,
					quirks->dev_name);
	if (IS_ERR_OR_NULL(cedar_devp->dev)) {
		VE_LOGE("device create fail\n");
		ret = -EINVAL;
		goto class_del;
	}
	return ret;

class_del:
	class_destroy(cedar_devp->class);
dev_del:
	cdev_del(&cedar_devp->cdev);
region_del:
	unregister_chrdev_region(cedar_devp->ve_dev, 1);
	return ret;
}

static ssize_t ve_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(dev);
	ssize_t count = 0;
	int i = 0;
	int nTotalSize = 0;

	/* VE_LOGD("show: buf = %p\n",buf); */

	count += sprintf(buf + count, "********* ve proc info **********\n");

	mutex_lock(&cedar_devp->lock_debug_info);
	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (cedar_devp->debug_info[i].head_info.pid != 0
		 && cedar_devp->debug_info[i].head_info.tid != 0) {
			nTotalSize += cedar_devp->debug_info[i].head_info.length;

			/*
			 * VE_LOGD("cpy-show: i = %d, size = %d, total size = %d, data = %p, cout = %d, id = %d, %d\n",
			 *		i,
			 *		cedar_devp->debug_info[i].head_info.length,
			 *		nTotalSize,
			 *		cedar_devp->debug_info[i].data, (int)count,
			 *		cedar_devp->debug_info[i].head_info.pid,
			 *		cedar_devp->debug_info[i].head_info.tid);
			 */

			if (nTotalSize > PAGE_SIZE)
				break;

			memcpy(buf + count, cedar_devp->debug_info[i].data,
			       cedar_devp->debug_info[i].head_info.length);

			count += cedar_devp->debug_info[i].head_info.length;
		}
	}

	mutex_unlock(&cedar_devp->lock_debug_info);
	/* VE_LOGD("return count = %d\n",(int)count); */
	return count;
}

/* sysfs create file, just support show */
static DEVICE_ATTR(ve_info, 0664, ve_info_show, NULL);

static struct attribute *ve_attributes[] = {
	&dev_attr_ve_info.attr,
	NULL
};

static struct attribute_group ve_attribute_group = {
	.name = "attr",
	.attrs = ve_attributes
};

static int cedardev_init(struct platform_device *pdev)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(&pdev->dev);
	int ret = 0;

	if (!cedar_devp) {
		VE_LOGE("cedar_devp invalid\n");
		return -1;
	}

	VE_LOGD("install start!!!\n");
	/* VE_LOGD("LINUX_VERSION_CODE:%d\n", LINUX_VERSION_CODE); */

	cedar_devp->plat_dev = &pdev->dev;
	cedar_devp->process_channel_id_cnt = 0;

	/* 1.register cdev */
	if (create_char_device(cedar_devp) != 0) {
		ret = -EINVAL;
		goto err;
	}
	/* 2.init some data */
	spin_lock_init(&cedar_devp->lock);
	sema_init(&cedar_devp->sem, 1);
	init_waitqueue_head(&cedar_devp->wq);
	init_waitqueue_head(&cedar_devp->wait_ve);

	mutex_init(&cedar_devp->lock_vdec);
	mutex_init(&cedar_devp->lock_venc);
	mutex_init(&cedar_devp->lock_jdec);
	mutex_init(&cedar_devp->lock_00_reg);
	mutex_init(&cedar_devp->lock_04_reg);
	mutex_init(&cedar_devp->lock_mem);
	mutex_init(&cedar_devp->lock_debug_info);

	AW_MEM_INIT_LIST_HEAD(&cedar_devp->list);

	cedar_devp->regulator = regulator_get(cedar_devp->plat_dev, "ve");
	if (!IS_ERR(cedar_devp->regulator)) {
		cedar_devp->voltage = regulator_get_voltage(cedar_devp->regulator)/1000;
		VE_LOGD("ve vol = %d mv\n", cedar_devp->voltage);
	} else {
		VE_LOGW("get ve regulator error\n");
		cedar_devp->regulator = NULL;
	};

	/* 3.config some register */
	if (deal_with_resouce(pdev)) {
		ret = -EINVAL;
		goto err;
	}

	/* 4.create sysfs file on new device */
	if (sysfs_create_group(&cedar_devp->dev->kobj, &ve_attribute_group)) {
		VE_LOGW("sysfs create group failed, maybe ok!\n");
	}

	/* 5.create debugfs file */
#if IS_ENABLED(CONFIG_DEBUG_FS)
	cedar_devp->dbgfs = ve_debug_register_driver(cedar_devp);
	if (!cedar_devp->dbgfs) {
		VE_LOGW("ve_debug_register_driver failed\n");
	}
	if (cedar_devp->dvfs_attr.dvfs_array_num) {
		ret = vedvfs_init(cedar_devp);
		if (ret) {
			VE_LOGW("sunxi ve dvfsinit debugfs fail\n");
		}
	}
#endif
	/* 6.runtime pm, maybe problem */
	pm_runtime_enable(&pdev->dev);

	cedar_devp->bInitEndFlag = 1;
	VE_LOGD("install end!!!\n");
	return 0;

err:
	return ret;
}

static void cedardev_exit(struct platform_device *pdev)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(&pdev->dev);
	int i = 0;

	if (!cedar_devp) {
		VE_LOGE("cedar_devp invalid\n");
		return;
	}

	free_irq(cedar_devp->irq, cedar_devp);
	iounmap(cedar_devp->iomap_addrs.regs_ve);

	/* Destroy char device */
	cdev_del(&cedar_devp->cdev);
	device_destroy(cedar_devp->class, cedar_devp->ve_dev);
	class_destroy(cedar_devp->class);
	unregister_chrdev_region(cedar_devp->ve_dev, 1);

	/* runtime pm */
	pm_runtime_disable(cedar_devp->plat_dev);

	/* check whether free debug buffer */
	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (cedar_devp->debug_info[i].data != NULL) {
			VE_LOGW("vfree : debug_info[%d].data = %p\n",
				i, cedar_devp->debug_info[i].data);
			vfree(cedar_devp->debug_info[i].data);
		}
	}

#if IS_ENABLED(CONFIG_DEBUG_FS)
	ve_debug_unregister_driver(cedar_devp->dbgfs);
	if (cedar_devp->dvfs_root) {
		debugfs_remove_recursive(cedar_devp->dvfs_root);
		cedar_devp->dvfs_root = NULL;
	}
#endif
	if (cedar_devp->regulator) {
		regulator_put(cedar_devp->regulator);
		cedar_devp->regulator = NULL;
	}
	if (cedar_devp->dvfs_attr.dvfs_array)
		kfree(cedar_devp->dvfs_attr.dvfs_array);

	cedar_devp->bInitEndFlag = 0;
	VE_LOGD("cedar-ve exit\n");
}

static int sunxi_cedar_remove(struct platform_device *pdev)
{
	struct cedar_dev *cedar_devp = dev_get_drvdata(&pdev->dev);
	const struct cedar_ve_quirks *quirks = NULL;
	struct device_node *np = pdev->dev.of_node;

	VE_LOGD("sunxi_cedar_remove\n");

	if (!cedar_devp) {
		VE_LOGE("cedar_devp invalid\n");
		return 0;
	}
	quirks = cedar_devp->quirks;

	if (quirks->ve_mod == VE_MODULE_NORMAL) {
		/* note: this method only compatible with existed platforms, no longer use.
		 * and will be switched to ve_mod to judge dual iommu.
		 */
		pdev->id = of_alias_get_id(np, "ve");
		if (pdev->id == 1) {
			VE_LOGI("remove ve just use to del iommu master\n");
			kfree(cedar_devp);
			return 0;
		} else {
			VE_LOGI("remove ve\n");
		}
	} else if (quirks->ve_mod == VE_MODULE_1) {
		VE_LOGI("remove ve1\n");
		kfree(cedar_devp);
		return 0;
	} else if (quirks->ve_mod == VE_MODULE_2) {
		VE_LOGI("remove ve2\n");
	} else {
		VE_LOGE("unsupport ve module\n");
	}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW20)
	pm_runtime_disable(&pdev->dev);
#endif
	cedardev_exit(pdev);
	kfree(cedar_devp);
	return 0;
}

static int sunxi_cedar_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cedar_dev *cedar_devp = NULL;
	struct cedar_ve_quirks *quirks = NULL;
	struct device_node *np = pdev->dev.of_node;

	quirks = (struct cedar_ve_quirks *)of_device_get_match_data(&pdev->dev);
	if (quirks == NULL) {
		VE_LOGE("quirks get failed\n");
		return -1;
	}
	cedar_devp = kzalloc(sizeof(*cedar_devp), GFP_KERNEL);
	if (cedar_devp == NULL) {
		VE_LOGE("malloc mem for cedar device err\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, cedar_devp);
	cedar_devp->quirks = quirks;
	quirks->priv = cedar_devp;

	if (quirks->ve_mod == VE_MODULE_NORMAL) {
		/* note: this method only compatible with existed platforms, no longer use.
		 * and will be switched to ve_mod to judge dual iommu.
		 */
		pdev->id = of_alias_get_id(np, "ve");
		if (pdev->id == 1) {
			VE_LOGI("probe ve just use to add iommu master\n");
			return 0;
		} else {
			VE_LOGI("probe ve\n");
		}
	} else if (quirks->ve_mod == VE_MODULE_1) {
		VE_LOGI("probe ve1\n");
		return 0;
	} else if (quirks->ve_mod == VE_MODULE_2) {
		VE_LOGI("probe ve2\n");
	} else {
		VE_LOGE("unsupport ve module\n");
		return -1;
	}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW20)
	VE_LOGD("power-domain init!!!\n");
	/* add for R528 sleep and awaken */
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
#endif
	if (cedardev_init(pdev)) {
		VE_LOGE("cedardev_init failed\n");
		return -1;
	}

	return 0;
}

static struct cedar_ve_quirks cedar_ve_quirk = {
	.ve_mod = VE_MODULE_NORMAL,
	.class_name = "cedar_ve",
	.dev_name = "cedar_dev",
};

static struct cedar_ve_quirks cedar_ve1_quirk = {
	.ve_mod = VE_MODULE_1,
	.class_name = NULL,
	.dev_name = NULL,
};

static struct cedar_ve_quirks cedar_ve2_quirk = {
	.ve_mod = VE_MODULE_2,
	.class_name = "cedar_ve2",
	.dev_name = "cedar_dev_ve2",
};

static struct of_device_id sunxi_cedar_match[] = {
	{
		.compatible = "allwinner,sunxi-cedar-ve",
		.data = &cedar_ve_quirk,
	},
	{
		.compatible = "allwinner,sunxi-cedar-ve1",
		.data = &cedar_ve1_quirk,
	},
	{
		.compatible = "allwinner,sunxi-cedar-ve2",
		.data = &cedar_ve2_quirk,
	},
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_cedar_match);

static struct platform_driver sunxi_cedar_driver = {
	.probe		= sunxi_cedar_probe,
	.remove		= sunxi_cedar_remove,
	.suspend	= sunxi_cedar_suspend,
	.resume		= sunxi_cedar_resume,
	.driver		= {
		.name	= "sunxi-cedar",
		.owner	= THIS_MODULE,
		.of_match_table = sunxi_cedar_match,
	},
};

static int __init sunxi_cedar_init(void)
{
	/* we don't register device which is registered by device tree */
	/* platform_device_register(&sunxi_device_cedar);              */
	VE_LOGI("sunxi cedar version 1.1\n");
	return platform_driver_register(&sunxi_cedar_driver);
}

static void __exit sunxi_cedar_exit(void)
{
	platform_driver_unregister(&sunxi_cedar_driver);
}
#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
postcore_initcall_sync(sunxi_cedar_init);
#else
late_initcall(sunxi_cedar_init);
#endif

module_exit(sunxi_cedar_exit);

MODULE_AUTHOR("Soft-Reuuimlla");
MODULE_DESCRIPTION("User mode CEDAR device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:cedarc-sunxi");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 25))
MODULE_IMPORT_NS(DMA_BUF);
#endif
