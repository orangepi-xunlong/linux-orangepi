/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#define SUNXI_MODNAME	"VE"
#include <sunxi-log.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <asm/cacheflush.h>
#include <sunxi-sid.h>

#include "cedar_ve.h"

/* debugfs param */
#define VE_DEBUGFS_MAX_CHANNEL	16
#define VE_DEBUGFS_BUF_SIZE	1024

struct ve_debugfs_proc {
	unsigned int len;
	char data[VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL];
};

struct ve_debugfs_buffer {
	unsigned char cur_channel_id;
	unsigned int proc_len[VE_DEBUGFS_MAX_CHANNEL];
	char *proc_buf[VE_DEBUGFS_MAX_CHANNEL];
	char *data;
	struct mutex lock_proc;
};

struct ve_dbgfs_h {
	struct dentry *dbg_root;
	struct ve_debugfs_buffer dbg_info;
};

/* resource */
int resource_iomap_init(struct device_node *node, struct iomap_para *iomap_addrs)
{
	int ret = 0;
	struct resource res;

	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		VE_LOGE("parse device node resource failed\n");
		return -1;
	}
	iomap_addrs->ve_reg_start = res.start;

	/* map for macc io space */
	iomap_addrs->regs_ve = of_iomap(node, 0);
	if (!iomap_addrs->regs_ve) {
		VE_LOGW("ve Can't map registers\n");
		return -1;
	}

	/* map for sys_config io space */
	iomap_addrs->regs_sys_cfg = (char *)of_iomap(node, 1);
	if (!iomap_addrs->regs_sys_cfg) {
		VE_LOGW("ve Can't map regs_sys_cfg registers, maybe no use\n");
	}

	return 0;
}

/* irq */
void ve_irq_work(struct cedar_dev *cedar_devp)
{
	wait_queue_head_t *wait_ve = &cedar_devp->wait_ve;

	unsigned long ve_int_status_reg;
	unsigned long ve_int_ctrl_reg;
	unsigned int status;
	volatile int val;
	int modual_sel;
	unsigned int interrupt_enable;
	struct iomap_para addrs = cedar_devp->iomap_addrs;

	modual_sel = readl(addrs.regs_ve + 0x0);
#if IS_ENABLED(CONFIG_ARCH_SUN3IW1P1)
	if (modual_sel&0xa) {
		if ((modual_sel&0xb) == 0xb) {
			/* jpg enc */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/* isp */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		if (status && interrupt_enable) {
			/* disable interrupt */
			if ((modual_sel & 0xb) == 0xb) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			}

			cedar_devp->en_irq_value = 1;	/* hx modify 2011-8-1 16:08:47 */
			cedar_devp->en_irq_flag = 1;
			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);		  /* ioctl */
		}
	}
#else
	if (modual_sel&(3<<6)) {
		if (modual_sel&(1<<7)) {
			/* avc enc */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/* isp */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		/* modify by fangning 2013-05-22 */
		if (status && interrupt_enable) {
			/* disable interrupt */
			/* avc enc */
			if (modual_sel&(1<<7)) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xb00 + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				/* isp */
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xa00 + 0x08);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			}
			/* hx modify 2011-8-1 16:08:47 */
			cedar_devp->en_irq_value = 1;
			cedar_devp->en_irq_flag = 1;
			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);
		}
	}
#endif

#if ((defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN50I) || \
	(defined CONFIG_ARCH_SUN8IW12P1) || (defined CONFIG_ARCH_SUN8IW17P1) \
	|| (defined CONFIG_ARCH_SUN8IW16P1) || (defined CONFIG_ARCH_SUN8IW19P1))
	if (modual_sel&(0x20)) {
		ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xe00 + 0x1c);
		ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xe00 + 0x14);
		interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x38);

		status = readl((void *)ve_int_status_reg);

		if ((status&0x7) && interrupt_enable) {
			/* disable interrupt */
			val = readl((void *)ve_int_ctrl_reg);
			writel(val & (~0x38), (void *)ve_int_ctrl_reg);

			cedar_devp->jpeg_irq_value = 1;
			cedar_devp->jpeg_irq_flag = 1;

			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);
		}
	}
#endif

	modual_sel &= 0xf;
	if (modual_sel <= 6) {
		/* estimate Which video format */
		switch (modual_sel) {
		case 0: /* mpeg124 */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7c);
			break;
		case 1: /* h264 */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x200 + 0x28);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x200 + 0x20);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 2: /* vc1 */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve +
				0x300 + 0x2c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x300 + 0x24);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 3: /* rv */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x400 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x400 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 4: /* hevc */
			ve_int_status_reg = (unsigned long)
				(addrs.regs_ve + 0x500 + 0x38);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x500 + 0x30);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			break;
		case 6: /* scaledown */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0xF00 +0x08);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0xF00 + 0x04);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			break;
		default:
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + 0x100 + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0xf);
			VE_LOGW("ve mode :%x " "not defined!\n", modual_sel);
			break;
		}

		status = readl((void *)ve_int_status_reg);

		/* modify by fangning 2013-05-22 */
		if ((status&0xf) && interrupt_enable) {
			/* disable interrupt */
			if (modual_sel == 0) {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7c), (void *)ve_int_ctrl_reg);
			} else if (modual_sel == 6) {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x1), (void *)ve_int_ctrl_reg);
			} else {
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0xf), (void *)ve_int_ctrl_reg);
			}
			cedar_devp->de_irq_value = 1;
			cedar_devp->de_irq_flag = 1;
			/* any interrupt will wake up wait queue */
			wake_up(wait_ve);
		}
	}
}

/* SUN55IW3 DVFS */
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define SUN55IW3_CHIP_ID_A523M		(0x5200)

#define SUN55IW3_CHIPID_EFUSE_OFF	(0x0)
#define SUN55IW3_DVFS_EFUSE_OFF		(0x48)

#define SUN55IW3_DVFS_VE_VF0	(0x00)
#define SUN55IW3_DVFS_VE_VF1	(0x01)
#define SUN55IW3_DVFS_VE_VF1_2	(0x21)
#define SUN55IW3_DVFS_VE_VF2	(0x02)
#define SUN55IW3_DVFS_VE_VF2_1	(0x12)
#define SUN55IW3_DVFS_VE_VF3	(0x04)
#define SUN55IW3_DVFS_VE_VF3_1	(0x14)
#define SUN55IW3_DVFS_VE_VF3_2	(0x24)
#define SUN55IW3_DVFS_VE_VF4	(0x05)
#define SUN55IW3_DVFS_VE_VF5	(0x06)
#define SUN55IW3_DVFS_VE_VF5_2	(0x26)

struct ve_dvfs_info ve_dvfs_sun55iw3[] = {
	/*      dvfs-index      vol-mv  freq-MHz */
	{0x00,                      0,   498}, /* default */
	{SUN55IW3_DVFS_VE_VF0,    900,   498}, /* VF0 */
	{SUN55IW3_DVFS_VE_VF0,    1120,  672}, /* VF0 */
	{SUN55IW3_DVFS_VE_VF1,    920,   520}, /* VF1 */
	{SUN55IW3_DVFS_VE_VF1,    1050,  624}, /* VF1 */
	{SUN55IW3_DVFS_VE_VF1,    1120,  672}, /* VF1 */
	{SUN55IW3_DVFS_VE_VF1_2,  920,   520}, /* VF1_2 */
	{SUN55IW3_DVFS_VE_VF1_2,  1050,  624}, /* VF1_2 */
	{SUN55IW3_DVFS_VE_VF1_2,  1120,  672}, /* VF1_2 */
	{SUN55IW3_DVFS_VE_VF2,    920,   520}, /* VF2 */
	{SUN55IW3_DVFS_VE_VF2,    1050,  624}, /* VF2 */
	{SUN55IW3_DVFS_VE_VF2,    1120,  672}, /* VF2 */
	{SUN55IW3_DVFS_VE_VF2_1,  920,   520}, /* VF2_1 */
	{SUN55IW3_DVFS_VE_VF2_1,  1050,  624}, /* VF2_1 */
	{SUN55IW3_DVFS_VE_VF2_1,  1120,  672}, /* VF2_1 */
	{SUN55IW3_DVFS_VE_VF3,	  920,   520}, /* VF3 */
	{SUN55IW3_DVFS_VE_VF3,    1000,  624}, /* VF3 */
	{SUN55IW3_DVFS_VE_VF3,    1080,  672}, /* VF3 */
	{SUN55IW3_DVFS_VE_VF3_1,  920,   520}, /* VF3_1 */
	{SUN55IW3_DVFS_VE_VF3_1,  1000,  624}, /* VF3_1 */
	{SUN55IW3_DVFS_VE_VF3_1,  1080,  672}, /* VF3_1 */
	{SUN55IW3_DVFS_VE_VF3_2,  920,   520}, /* VF3_2 */
	{SUN55IW3_DVFS_VE_VF3_2,  1000,  624}, /* VF3_2 */
	{SUN55IW3_DVFS_VE_VF3_2,  1080,  672}, /* VF3_2 */
	{SUN55IW3_DVFS_VE_VF4,	  920,   576}, /* VF4 */
	{SUN55IW3_DVFS_VE_VF4,    1000,  624}, /* VF4 */
	{SUN55IW3_DVFS_VE_VF5,	  920,   432}, /* VF5 */
	{SUN55IW3_DVFS_VE_VF5_2,  920,   432}, /* VF5_2 */
};

enum VE_PERFORMANCE_LEVEL {
	VE_PERFORMANCE_LEVEL_1,
	VE_PERFORMANCE_LEVEL_2,
	VE_PERFORMANCE_LEVEL_3,
	VE_PERFORMANCE_LEVEL_4,
};

#define LEVELE_1_START_PIXELS	(0)
#define LEVELE_1_END_PIXELS	(1280*736*60)
#define LEVELE_2_START_PIXELS	(1280*736*60+1)
#define LEVELE_2_END_PIXELS	(1920*1088*30)
#define LEVELE_3_START_PIXELS	(1920*1088*30+1)
#define LEVELE_3_END_PIXELS	(1920*1088*60)
#define LEVELE_4_START_PIXELS	(1920*1088*60+1)
#define LEVELE_4_END_PIXELS	(3840*2176*30)
#define LEVELE_MAX_END_PIXELS	(7680*4320*60)
#define VE_PERFORMANCE_MAX_FREQ	(900)

#define MAX_VE_LOAD_PARAM_CHANNEL	(16)

/* max performance about sun55iw3(aw1890/a523):
* h264     -- 4k30fps
* h265/vp9 -- 4k60fps
*/
struct ve_performat_info decoder_perform_sun55iw3[] = {
	/* codec_format          level                   start_pixels           end_pixels             ve_freq */
	{VE_DECODER_FORMAT_H264, VE_PERFORMANCE_LEVEL_1, LEVELE_1_START_PIXELS, LEVELE_1_END_PIXELS,   100},
	{VE_DECODER_FORMAT_H264, VE_PERFORMANCE_LEVEL_2, LEVELE_2_START_PIXELS, LEVELE_2_END_PIXELS,   200},
	{VE_DECODER_FORMAT_H264, VE_PERFORMANCE_LEVEL_3, LEVELE_3_START_PIXELS, LEVELE_3_END_PIXELS,   350},
	{VE_DECODER_FORMAT_H264, VE_PERFORMANCE_LEVEL_4, LEVELE_4_START_PIXELS, LEVELE_MAX_END_PIXELS, VE_PERFORMANCE_MAX_FREQ },

	{VE_DECODER_FORMAT_H265, VE_PERFORMANCE_LEVEL_1, LEVELE_1_START_PIXELS, LEVELE_1_END_PIXELS,   100},
	{VE_DECODER_FORMAT_H265, VE_PERFORMANCE_LEVEL_2, LEVELE_2_START_PIXELS, LEVELE_2_END_PIXELS,   150},
	{VE_DECODER_FORMAT_H265, VE_PERFORMANCE_LEVEL_3, LEVELE_3_START_PIXELS, LEVELE_3_END_PIXELS,   350},
	{VE_DECODER_FORMAT_H265, VE_PERFORMANCE_LEVEL_4, LEVELE_4_START_PIXELS, LEVELE_MAX_END_PIXELS, VE_PERFORMANCE_MAX_FREQ },

	{VE_DECODER_FORMAT_VP9,  VE_PERFORMANCE_LEVEL_1, LEVELE_1_START_PIXELS, LEVELE_1_END_PIXELS,   150},
	{VE_DECODER_FORMAT_VP9,  VE_PERFORMANCE_LEVEL_2, LEVELE_2_START_PIXELS, LEVELE_2_END_PIXELS,   200},
	{VE_DECODER_FORMAT_VP9,  VE_PERFORMANCE_LEVEL_3, LEVELE_3_START_PIXELS, LEVELE_3_END_PIXELS,   350},
	{VE_DECODER_FORMAT_VP9,  VE_PERFORMANCE_LEVEL_4, LEVELE_4_START_PIXELS, LEVELE_MAX_END_PIXELS, VE_PERFORMANCE_MAX_FREQ },

	{VE_DECODER_FORMAT_OTHER, VE_PERFORMANCE_LEVEL_1, LEVELE_1_START_PIXELS, LEVELE_MAX_END_PIXELS, VE_PERFORMANCE_MAX_FREQ },
};

static unsigned int map_max_ve_freq_by_vf_sun55iw3(struct cedar_dev *cedar_devp)
{
	unsigned int ve_freq = 0;
	unsigned int bak_dvfs, dvfs, combi_dvfs;
	unsigned int dvfs_arry_size = 0;
	unsigned int chip_id = 0;
	int i = 0;

	/* first use ve_freq setup by dts config*/
	if (cedar_devp->ve_freq_setup_by_dts_config > 0) {
		ve_freq = cedar_devp->ve_freq_setup_by_dts_config;
		return ve_freq;
	}

	sunxi_get_module_param_from_sid(&dvfs, SUN55IW3_DVFS_EFUSE_OFF, 4);
	bak_dvfs = (dvfs >> 12) & 0xff;
	if (bak_dvfs)
		combi_dvfs = bak_dvfs;
	else
		combi_dvfs = (dvfs >> 4) & 0xff;

	sunxi_get_module_param_from_sid(&chip_id, SUN55IW3_CHIPID_EFUSE_OFF, 4);
	chip_id &= 0xffff;

	if (combi_dvfs == 0) {
		if (chip_id == SUN55IW3_CHIP_ID_A523M)
			combi_dvfs = SUN55IW3_DVFS_VE_VF0;
		else
			combi_dvfs = SUN55IW3_DVFS_VE_VF1;
	}

	dvfs_arry_size = sizeof(ve_dvfs_sun55iw3)/sizeof(struct ve_dvfs_info);
	for (i = 0; i < dvfs_arry_size; i++) {
		if (ve_dvfs_sun55iw3[i].dvfs_index == combi_dvfs
			&& ve_dvfs_sun55iw3[i].voltage == cedar_devp->voltage)
			break;
	}

	/* use VF1 when can not match VF */
	if (i >= dvfs_arry_size) {
		combi_dvfs = SUN55IW3_DVFS_VE_VF1;

		/* match ve_freq again */
		for (i = 0; i < dvfs_arry_size; i++) {
			if (ve_dvfs_sun55iw3[i].dvfs_index == combi_dvfs
				&& ve_dvfs_sun55iw3[i].voltage == cedar_devp->voltage)
				break;
		}
	}

	/* use default when can not match the vf and vol at the some time */
	if (i < dvfs_arry_size)
		ve_freq = ve_dvfs_sun55iw3[i].ve_freq;
	else {
		VE_LOGE("can not match dvfs[0x%x] and vol[%d mv], use default ve_freq\n",
			combi_dvfs, cedar_devp->voltage);
		ve_freq = ve_dvfs_sun55iw3[0].ve_freq;
	}

	VE_LOGV("chip_id = 0x%x, dvfs_values = 0x%x, vol = %d mv, ve_freq = %d\n",
		chip_id, combi_dvfs, cedar_devp->voltage, ve_freq);

	return ve_freq;
}
#endif

/* dvfs */
int ve_dvfs_get_attr(struct cedar_dev *cedar_devp)
{
	struct ve_dvfs_attr *attr = &cedar_devp->dvfs_attr;

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	/* enable VF, DF */
	attr->default_freq = map_max_ve_freq_by_vf_sun55iw3(cedar_devp);
	attr->dvfs_array_num = sizeof(ve_dvfs_sun55iw3) / sizeof(struct ve_dvfs_info);
	attr->dvfs_array = ve_dvfs_sun55iw3;
	cedar_devp->case_load_channels = MAX_VE_LOAD_PARAM_CHANNEL;
	cedar_devp->load_infos = kcalloc(cedar_devp->case_load_channels,
					 sizeof(struct ve_case_load_info), GFP_KERNEL);
	if (!cedar_devp->load_infos) {
		cedar_devp->case_load_channels = 0;
		VE_LOGE("no memory\n");
		return -ENOMEM;
	}
	cedar_devp->performat_max_freq = VE_PERFORMANCE_MAX_FREQ;
	cedar_devp->performat_num = sizeof(decoder_perform_sun55iw3)
				  / sizeof(struct ve_performat_info);
	cedar_devp->performat_infos = decoder_perform_sun55iw3;
#else
	/* disable VF */
	attr->default_freq = 0;
	attr->dvfs_array_num = 0;
	attr->dvfs_array = NULL;
	/* disable DF */
	cedar_devp->case_load_channels = 0;
	cedar_devp->load_infos = NULL;
	cedar_devp->performat_max_freq = 0;
	cedar_devp->performat_num = 0;
	cedar_devp->performat_infos = NULL;
#endif
	return 0;
}

/* debug */
static int ve_debugfs_open(struct inode *inode, struct file *file)
{
	struct ve_dbgfs_h *dbgfs_h = (struct ve_dbgfs_h *)(inode->i_private);
	int i = 0;
	char *pData;
	struct ve_debugfs_proc *pVeProc;

	pVeProc = kmalloc(sizeof(*pVeProc), GFP_KERNEL);
	if (pVeProc == NULL) {
		VE_LOGE("kmalloc pVeProc fail\n");
		return -ENOMEM;
	}
	pVeProc->len = 0;
	memset(pVeProc->data, 0, VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL);

	pData = pVeProc->data;
	mutex_lock(&dbgfs_h->dbg_info.lock_proc);
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++) {
		if (dbgfs_h->dbg_info.proc_buf[i] != NULL) {
			memcpy(pData, dbgfs_h->dbg_info.proc_buf[i], dbgfs_h->dbg_info.proc_len[i]);
			pData += dbgfs_h->dbg_info.proc_len[i];
			pVeProc->len += dbgfs_h->dbg_info.proc_len[i];
		}
	}
	mutex_unlock(&dbgfs_h->dbg_info.lock_proc);

	file->private_data = pVeProc;
	return 0;
}

static ssize_t ve_debugfs_read(struct file *file, char __user *user_buf,
			       size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc *pVeProc = file->private_data;

	if (pVeProc->len == 0) {
		VE_LOGD("there is no any codec working currently\n");
		return 0;
	}

	return simple_read_from_buffer(user_buf, nbytes, ppos, pVeProc->data, pVeProc->len);
}

static int ve_debugfs_release(struct inode *inode, struct file *file)
{
	struct ve_debugfs_proc *pVeProc = file->private_data;

	kfree(pVeProc);
	pVeProc = NULL;
	file->private_data = NULL;

	return 0;
}

static const struct file_operations ve_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = ve_debugfs_open,
	.llseek = no_llseek,
	.read = ve_debugfs_read,
	.release = ve_debugfs_release,
};

ve_dbgfs_t *ve_debug_register_driver(struct cedar_dev *cedar_devp)
{
	struct ve_dbgfs_h *dbgfs_h = NULL;
	struct dentry *dent;
	int i;
	(void)cedar_devp;

	dbgfs_h = kzalloc(sizeof(*dbgfs_h), GFP_KERNEL);
	if (!dbgfs_h) {
		VE_LOGE("kzalloc fail\n");
		return NULL;
	}

	/* debugfs_mpp_root defined out of this module */
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	dbgfs_h->dbg_root = debugfs_mpp_root;
#else
	dbgfs_h->dbg_root = debugfs_lookup("ve", NULL);
	if (IS_ERR_OR_NULL(dbgfs_h->dbg_root))
		dbgfs_h->dbg_root = debugfs_create_dir("ve", NULL);
#endif
	if (IS_ERR_OR_NULL(dbgfs_h->dbg_root)) {
		VE_LOGE("debugfs root is null please check!\n");
		return NULL;
	}
	dent = debugfs_create_file("ve", 0444, dbgfs_h->dbg_root, dbgfs_h, &ve_debugfs_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(dbgfs_h->dbg_root);
		dbgfs_h->dbg_root = NULL;
		return NULL;
	}

	memset(&dbgfs_h->dbg_info, 0, sizeof(dbgfs_h->dbg_info));
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++)
		dbgfs_h->dbg_info.proc_buf[i] = NULL;

	dbgfs_h->dbg_info.data = kmalloc(VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL, GFP_KERNEL);
	if (!dbgfs_h->dbg_info.data) {
		VE_LOGE("kmalloc proc buffer failed!\n");
		return NULL;
	}
	mutex_init(&dbgfs_h->dbg_info.lock_proc);
	VE_LOGD("dbgfs_h->dbg_info:%p, data:%p, lock:%p\n",
		&dbgfs_h->dbg_info,
		dbgfs_h->dbg_info.data,
		&dbgfs_h->dbg_info.lock_proc);

	return dbgfs_h;
}

void ve_debug_unregister_driver(ve_dbgfs_t *dbgfs)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;

	if (dbgfs_h == NULL) {
		VE_LOGW("dbgfs invalid\n");
		return;
	}
	if (dbgfs_h->dbg_root == NULL) {
		VE_LOGW("note: debug root already is null\n");
		return;
	}
	debugfs_remove_recursive(dbgfs_h->dbg_root);
	dbgfs_h->dbg_root = NULL;

	mutex_destroy(&dbgfs_h->dbg_info.lock_proc);
	kfree(dbgfs_h->dbg_info.data);
	kfree(dbgfs_h);
}

void ve_debug_open(ve_dbgfs_t *dbgfs, struct ve_info *vi)
{
	(void)dbgfs;
	(void)vi;
	/* NULL */
}

void ve_debug_release(ve_dbgfs_t *dbgfs, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	if (vi->lock_flags) {
		if (vi->lock_flags & VE_LOCK_PROC_INFO) {
			mutex_unlock(&dbgfs_h->dbg_info.lock_proc);
		}
	}
}

/* ioctl -> IOCTL_FLUSH_CACHE_RANGE */
#if IS_ENABLED(CONFIG_ARM64)
#if (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW9)) \
	&& (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
extern void cedar_dma_flush_range(const void *, const void *);
#else
extern void cedar_dma_flush_range(const void *, size_t);
#endif
#else
extern void cedar_dma_flush_range(const void *, const void *);
#endif
#if IS_ENABLED(CONFIG_64BIT) && IS_ENABLED(CONFIG_RISCV)
extern void dma_usr_va_wb_range(void *user_addr, unsigned long len);
#endif
int ioctl_flush_cache_range(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp)
{
	struct cache_range data;
	#if IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_64BIT)
	#else
	u32 addr_start = 0;
	u32 addr_end = 0;
	#endif

	(void)cedar_devp;

	if (user) {
		if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
			return -EFAULT;
	} else {
		memcpy(&data, (void *)arg, sizeof(data));
	}

	#if IS_ENABLED(CONFIG_ARM64) || IS_ENABLED(CONFIG_64BIT)
	if (IS_ERR_OR_NULL((void *)data.start) || IS_ERR_OR_NULL((void *)data.end)) {
		VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
			(u32)data.start, (u32)data.end);
		return -EFAULT;
	}
	#else
	addr_start = (u32)(data.start & 0xffffffff);
	addr_end = (u32)(data.end & 0xffffffff);

	if (IS_ERR_OR_NULL((void *)addr_start) || IS_ERR_OR_NULL((void *)addr_end)) {
		VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
			(u32)data.start, (u32)data.end);
		return -EFAULT;
	}
	#endif

	/*
	 * VE_LOGD("ion flush_range start:%lx end:%lx size:%lx\n",
	 * data.start, data.end, data.end - data.start);
	 */
	#if IS_ENABLED(CONFIG_ARM64)
	/*
	 * VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
	 * (u32)data.start, (u32)data.end);
	 */
	#if (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW9)) \
		&& (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	cedar_dma_flush_range((void *)data.start, (void *)data.end);
	#else
	cedar_dma_flush_range((void *)data.start, (unsigned long)(data.end - data.start));
	#endif
	/* dma_sync_single_for_cpu(cedar_devp->plat_dev,
	 * (u32)data.start,data.end - data.start,DMA_BIDIRECTIONAL);
	 */
	#else
	#if IS_ENABLED(CONFIG_64BIT) && IS_ENABLED(CONFIG_RISCV)
	dma_usr_va_wb_range((void *)data.start, (unsigned long)(data.end - data.start));
	#else
	/* dmac_flush_range((void *)addr_start, (void *)addr_end); */
	cedar_dma_flush_range((void *)addr_start, (void *)addr_end);
	#endif
	#endif

	if (copy_to_user((void __user *)arg, &data, sizeof(data)))
		return -EFAULT;

	return 0;
}

int ioctl_get_csi_online_related_info(unsigned long arg, uint8_t from_kernel, struct cedar_dev *cedar_devp)
{
	(void)arg;
	(void)from_kernel;
	(void)cedar_devp;

	VE_LOGW("unsupport api\n");

	return 0;
}

/* ioctl -> IOCTL_SET_PROC_INFO
 *       -> IOCTL_STOP_PROC_INFO
 *       -> IOCTL_COPY_PROC_INFO
 */
int ioctl_set_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	struct VE_PROC_INFO ve_info;
	unsigned char channel_id = 0;
	u32 lock_type = VE_LOCK_PROC_INFO;

	if (dbgfs_h->dbg_root == NULL)
		return 0;

	mutex_lock(&dbgfs_h->dbg_info.lock_proc);
	if (copy_from_user(&ve_info, (void __user *)arg, sizeof(ve_info))) {
		VE_LOGW("IOCTL_SET_PROC_INFO copy_from_user fail\n");
		mutex_unlock(&dbgfs_h->dbg_info.lock_proc);
		return -EFAULT;
	}

	channel_id = ve_info.channel_id;
	if (channel_id >= VE_DEBUGFS_MAX_CHANNEL) {
		VE_LOGW("set channel[%c] is bigger than max channel[%d]\n",
			channel_id, VE_DEBUGFS_MAX_CHANNEL);
		mutex_unlock(&dbgfs_h->dbg_info.lock_proc);
		return -EFAULT;
	}

	mutex_lock(&vi->lock_flag_io);
	vi->lock_flags |= lock_type;
	mutex_unlock(&vi->lock_flag_io);

	dbgfs_h->dbg_info.cur_channel_id = ve_info.channel_id;
	dbgfs_h->dbg_info.proc_len[channel_id] = ve_info.proc_info_len;
	dbgfs_h->dbg_info.proc_buf[channel_id] = dbgfs_h->dbg_info.data
					       + channel_id * VE_DEBUGFS_BUF_SIZE;

	return 0;
}

int ioctl_copy_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	unsigned char channel_id;
	u32 lock_type = VE_LOCK_PROC_INFO;

	if (dbgfs_h->dbg_root == NULL)
		return 0;

	mutex_lock(&vi->lock_flag_io);
	vi->lock_flags &= (~lock_type);
	mutex_unlock(&vi->lock_flag_io);

	channel_id = dbgfs_h->dbg_info.cur_channel_id;
	if (copy_from_user(dbgfs_h->dbg_info.proc_buf[channel_id], (void __user *)arg,
			   dbgfs_h->dbg_info.proc_len[channel_id])) {
		VE_LOGW("IOCTL_COPY_PROC_INFO copy_from_user fail\n");
		mutex_unlock(&dbgfs_h->dbg_info.lock_proc);
		return -EFAULT;
	}
	mutex_unlock(&dbgfs_h->dbg_info.lock_proc);
	return 0;
}

int ioctl_stop_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	unsigned char channel_id;
	(void)vi;

	if (dbgfs_h->dbg_root == NULL)
		return 0;

	channel_id = arg;
	dbgfs_h->dbg_info.proc_buf[channel_id] = NULL;

	return 0;
}
