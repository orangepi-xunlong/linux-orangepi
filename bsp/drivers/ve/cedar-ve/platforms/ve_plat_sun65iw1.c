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
#include <linux/of_address.h>
#include <linux/debugfs.h>
#include <sunxi-sid.h>
#include "cedar_ve.h"

#define VE_REG_DEC_TOP_BASE	0X0000
#define VE_REG_ENC_TOP_BASE	0X0000
#define VE_REG_VCUENC_BASE	0x0900
#define VE_REG_VCUDEC_BASE	0x0600
#define VE_REG_ENCPP_BASE	0x0a00
#define VE_REG_ENC_BASE		0x0b00
#define VE_REG_H265_EXT_BASE	0x0c00
#define VE_REG_DECJPEG_BASE	0x0e00

#define VE_REG_ENC_TOP_VCU_CFG	(VE_REG_ENC_TOP_BASE + 0x5c)
#define VE_REG_DEC_TOP_VCU_CFG	(VE_REG_DEC_TOP_BASE + 0x5c)
#define VE_REG_VCUENC_INT_STA	(VE_REG_VCUENC_BASE + 0x28)
#define VE_REG_VCUDEC_INT_STA	(VE_REG_VCUDEC_BASE + 0x28)

/* dvfs v0.31 */
struct ve_dvfs_pair {
	unsigned int dvfs_index;
	const char dvfs_name[64];
	const char dvfs_freq_name[64];
	const char dvfs_volt_name[64];
};

static struct ve_dvfs_pair ve_dvfs[] = {
	/* dvfs-index   dvfs_name   dvfs_freq_name   dvfs_volt_name */
	{0x00, "opp-vf0000", "opp-hz-0", "opp-microvolt-0"},
	{0x01, "opp-vf0100", "opp-hz-0", "opp-microvolt-0"},
	{0x03, "opp-vf0300", "opp-hz-0", "opp-microvolt-0"},
	{0x24, "opp-vf0402", "opp-hz-0", "opp-microvolt-0"},
};

enum VE_DVFS_LEVEL {
	VE_DVFS_L1,
	VE_DVFS_L2,
	VE_DVFS_L3,
	VE_DVFS_L4,
};

#define L1_START_PIXELS	(0)
#define L1_END_PIXELS	(1280*736*60)
#define L2_START_PIXELS	(1280*736*60+1)
#define L2_END_PIXELS	(1920*1088*30)
#define L3_START_PIXELS	(1920*1088*30+1)
#define L3_END_PIXELS	(1920*1088*60)
#define L4_START_PIXELS	(1920*1088*60+1)
#define L4_END_PIXELS	(3840*2176*30)
#define L_MAX_END_PIXELS	(7680*4320*60)
#define VE_DVFS_MAX_FREQ	(560)

#define MAX_VE_LOAD_PARAM_CHANNEL	(16)

/* max performance:
 * h264     -- 4k30fps
 * h265/vp9 -- 4k60fps
 */
static struct ve_performat_info dvfs_performat[] = {
	/* codec_format          level       start_pixels     end_pixels        ve_freq */
	{VE_DECODER_FORMAT_H264, VE_DVFS_L1, L1_START_PIXELS, L1_END_PIXELS,    100},
	{VE_DECODER_FORMAT_H264, VE_DVFS_L2, L2_START_PIXELS, L2_END_PIXELS,    200},
	{VE_DECODER_FORMAT_H264, VE_DVFS_L3, L3_START_PIXELS, L3_END_PIXELS,    350},
	{VE_DECODER_FORMAT_H264, VE_DVFS_L4, L4_START_PIXELS, L_MAX_END_PIXELS, VE_DVFS_MAX_FREQ},

	{VE_DECODER_FORMAT_H265, VE_DVFS_L1, L1_START_PIXELS, L1_END_PIXELS,    100},
	{VE_DECODER_FORMAT_H265, VE_DVFS_L2, L2_START_PIXELS, L2_END_PIXELS,    150},
	{VE_DECODER_FORMAT_H265, VE_DVFS_L3, L3_START_PIXELS, L3_END_PIXELS,    350},
	{VE_DECODER_FORMAT_H265, VE_DVFS_L4, L4_START_PIXELS, L_MAX_END_PIXELS, VE_DVFS_MAX_FREQ},

	{VE_DECODER_FORMAT_VP9,  VE_DVFS_L1, L1_START_PIXELS, L1_END_PIXELS,    150},
	{VE_DECODER_FORMAT_VP9,  VE_DVFS_L2, L2_START_PIXELS, L2_END_PIXELS,    200},
	{VE_DECODER_FORMAT_VP9,  VE_DVFS_L3, L3_START_PIXELS, L3_END_PIXELS,    350},
	{VE_DECODER_FORMAT_VP9,  VE_DVFS_L4, L4_START_PIXELS, L_MAX_END_PIXELS, VE_DVFS_MAX_FREQ},

	{VE_DECODER_FORMAT_OTHER, VE_DVFS_L1, L1_START_PIXELS, L_MAX_END_PIXELS, VE_DVFS_MAX_FREQ},
};

/* debug */
#define VE_DEBUGFS_MAX_CHANNEL	16
#define VE_DEBUGFS_BUF_SIZE	1024

struct ve_debugfs_proc {
	unsigned int len;
	char data[VE_DEBUGFS_BUF_SIZE * VE_DEBUGFS_MAX_CHANNEL];
};

struct ve_debugfs_buffer {
	unsigned char  cur_channel_id;
	unsigned int proc_len[VE_DEBUGFS_MAX_CHANNEL];
	char *proc_buf[VE_DEBUGFS_MAX_CHANNEL];
	char *data;
	struct mutex lock_proc;
};

struct ve_dbgfs_h {
	struct dentry *dbg_root;
	struct ve_debugfs_buffer dbg_info;
};

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

static void ve_interupt_vcu_enc(struct cedar_dev *cedar_devp, wait_queue_head_t *wait_ve)
{
	struct iomap_para *addrs = &cedar_devp->iomap_addrs;
	volatile char *base_reg = addrs->regs_ve;
	unsigned int vcu_config_value = readl((void *)(base_reg + VE_REG_ENC_TOP_VCU_CFG));

	if (vcu_config_value & 0x1) {
		volatile char *vcu_status1_reg = base_reg + VE_REG_VCUENC_INT_STA;
		unsigned int vcu_status1_value = readl((void *)vcu_status1_reg);
		/* clear the interrupt flag */
		if (vcu_status1_value & 0x1) {
			/* just clear interrupt_flag, not clear other unnormal flag */
			vcu_status1_value &= 0x1;
			writel(vcu_status1_value, (void *)vcu_status1_reg);
			vcu_status1_value = readl((void *)vcu_status1_reg);
		}

		cedar_devp->vcuenc_irq_flag = 1;
		wake_up(wait_ve);
	}
}

static void ve_interupt_vcu_dec(struct cedar_dev *cedar_devp, wait_queue_head_t *wait_ve)
{
	struct iomap_para *addrs = &cedar_devp->iomap_addrs;
	volatile char *base_reg = addrs->regs_ve;
	unsigned int vcu_config_value = readl((void *)(base_reg + VE_REG_DEC_TOP_VCU_CFG));

	if (vcu_config_value & 0x4) {
		volatile char *vcu_status1_reg = base_reg + VE_REG_VCUDEC_INT_STA;
		unsigned int vcu_status1_value = readl((void *)vcu_status1_reg);
		/* clear the interrupt flag */
		if (vcu_status1_value & 0x1) {
			/* just clear interrupt_flag, not clear other unnormal flag */
			vcu_status1_value &= 0x1;
			writel(vcu_status1_value, (void *)vcu_status1_reg);
			vcu_status1_value = readl((void *)vcu_status1_reg);
			cedar_devp->vcudec_irq_flag = 1;
			wake_up(wait_ve);
		}
	}
}

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

	if (cedar_devp->ve_mode == VE_MODE_VCUENC) {
		ve_interupt_vcu_enc(cedar_devp, wait_ve);
		return;
	}

	if (cedar_devp->ve_mode == VE_MODE_VCUDEC) {
		ve_interupt_vcu_dec(cedar_devp, wait_ve);
		return;
	}

	if (cedar_devp->quirks->ve_mod == VE_MODULE_2)
		modual_sel = readl((void *)(addrs.regs_ve + VE_REG_ENC_TOP_BASE + 0x0));
	else
		modual_sel = readl((void *)(addrs.regs_ve + VE_REG_DEC_TOP_BASE + 0x0));

	if (modual_sel&(3<<6)) {
		if (modual_sel&(1<<7)) {
			/* avc enc */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + VE_REG_ENC_BASE + 0x1c);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + VE_REG_ENC_BASE + 0x14);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x7);
			status = readl((void *)ve_int_status_reg);
			status &= 0xf;
		} else {
			/* isp */
			ve_int_status_reg = (unsigned long)(addrs.regs_ve + VE_REG_ENCPP_BASE + 0x10);
			ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + VE_REG_ENCPP_BASE + 0x08);
			interrupt_enable = readl((void *)ve_int_ctrl_reg) & (0x1);
			status = readl((void *)ve_int_status_reg);
			status &= 0x1;
		}

		/* modify by fangning 2013-05-22 */
		if (status && interrupt_enable) {
			/* disable interrupt */
			/* avc enc */
			if (modual_sel&(1<<7)) {
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + VE_REG_ENC_BASE + 0x14);
				val = readl((void *)ve_int_ctrl_reg);
				writel(val & (~0x7), (void *)ve_int_ctrl_reg);
			} else {
				/* isp */
				ve_int_ctrl_reg = (unsigned long)(addrs.regs_ve + VE_REG_ENCPP_BASE + 0x08);
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

/* dvfs */
int ve_dvfs_get_attr(struct cedar_dev *cedar_devp)
{
	/* enable VF, DF */
	struct ve_dvfs_attr *attr = &cedar_devp->dvfs_attr;
	struct device_node *node = cedar_devp->plat_dev->of_node, *opp_table_node, *opp_node;
	unsigned int i, j;
	long long opp_hz;
	u32 dvfs = 0;
	unsigned int dvfs_arry_size = 0, opp_volt = 0;
	const char *dvfs_name = NULL;

	/* enable VF */
	opp_table_node = of_parse_phandle(node, "operating-points-v2", 0);
	if (!opp_table_node) {
		VE_LOGE("no or unsupport operating-points-v2 node\n");
		return -ENODEV;
	}

	/*if (sunxi_get_soc_dvfs(&dvfs))
		VE_LOGE("failed to get soc dvfs, use default vf table\n");*/

	dvfs_arry_size = sizeof(ve_dvfs) / sizeof(struct ve_dvfs_pair);
	for (i = 0; i < dvfs_arry_size; ++i) {
		if (ve_dvfs[i].dvfs_index == dvfs) {
			attr->dvfs_array_num += 1;
			dvfs_name = ve_dvfs[i].dvfs_name;
		}
	}
	if (attr->dvfs_array_num == 0) {
		VE_LOGE("not match VF index\n");
		return -1;
	}
	opp_node = of_find_node_by_name(opp_table_node, dvfs_name);
	if (!opp_node) {
		VE_LOGE("Failed to get %s node\n", ve_dvfs[i].dvfs_name);
		of_node_put(opp_table_node);
		return -ENODEV;
	}
	VE_LOGV("dvfs_name %s dvfs_array_num %d", dvfs_name, attr->dvfs_array_num);
	attr->dvfs_array = kcalloc(attr->dvfs_array_num,
				   sizeof(struct ve_dvfs_info), GFP_KERNEL);
	attr->default_freq = 0;
	for (i = 0, j = 0; i < dvfs_arry_size && j < attr->dvfs_array_num; ++i) {
		if (strcmp(dvfs_name, ve_dvfs[i].dvfs_name)) {
			continue;
		}
		attr->dvfs_array[j].dvfs_index = ve_dvfs[i].dvfs_index;
		of_property_read_u64(opp_node, ve_dvfs[i].dvfs_freq_name, &opp_hz);
		attr->dvfs_array[j].ve_freq = (unsigned int)opp_hz / 1000000;
		of_property_read_u32(opp_node, ve_dvfs[i].dvfs_volt_name, &opp_volt);
		attr->dvfs_array[j].voltage = (unsigned int)opp_volt / 1000;
		if (attr->dvfs_array[j].voltage == cedar_devp->voltage)
			attr->default_freq = attr->dvfs_array[j].ve_freq;
		j += 1;
	}
	if (!attr->default_freq) {
		attr->default_freq = VE_DVFS_MAX_FREQ;
		VE_LOGW("get vf table failed, default %uMHz\n", attr->default_freq);
	}
	VE_LOGV("voltage %d ve_freq_default %d", cedar_devp->voltage, attr->default_freq);

	/* enable DF */
	cedar_devp->case_load_channels = MAX_VE_LOAD_PARAM_CHANNEL;
	cedar_devp->load_infos = kcalloc(cedar_devp->case_load_channels,
					 sizeof(struct ve_case_load_info), GFP_KERNEL);
	if (!cedar_devp->load_infos) {
		cedar_devp->case_load_channels = 0;
		VE_LOGE("no memory\n");
		return -ENOMEM;
	}
	cedar_devp->performat_max_freq = VE_DVFS_MAX_FREQ;
	cedar_devp->performat_num = sizeof(dvfs_performat) / sizeof(struct ve_performat_info);
	cedar_devp->performat_infos = dvfs_performat;

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
	char *dfs_name = NULL;
	char *dfs_name_nor = "ve";
	char *dfs_name_2 = "ve2";

	dbgfs_h = kzalloc(sizeof(*dbgfs_h), GFP_KERNEL);
	if (!dbgfs_h) {
		VE_LOGE("kzalloc fail\n");
		return NULL;
	}

	if (cedar_devp->quirks->ve_mod == VE_MODULE_2)
		dfs_name = dfs_name_2;
	else
		dfs_name = dfs_name_nor;

	dbgfs_h->dbg_root = debugfs_lookup("ve", NULL);
	if (IS_ERR_OR_NULL(dbgfs_h->dbg_root))
		dbgfs_h->dbg_root = debugfs_create_dir("ve", NULL);
	if (IS_ERR_OR_NULL(dbgfs_h->dbg_root)) {
		VE_LOGE("debugfs root is null please check!\n");
		return NULL;
	}
	dent = debugfs_create_file(dfs_name, 0444, dbgfs_h->dbg_root, dbgfs_h, &ve_debugfs_fops);
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

extern void cedar_dma_flush_range(const void *, const void *);
int ioctl_flush_cache_range(unsigned long arg,  uint8_t user, struct cedar_dev *cedar_devp)
{
	struct cache_range data;

	(void)cedar_devp;

	if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
		return -EFAULT;

	if (IS_ERR_OR_NULL((void *)data.start) || IS_ERR_OR_NULL((void *)data.end)) {
		VE_LOGE("flush 0x%x, end 0x%x fault user virt address!\n",
			(u32)data.start, (u32)data.end);
		return -EFAULT;
	}

	/*
	 * VE_LOGD("ion flush_range start:%lx end:%lx size:%lx\n",
	 * data.start, data.end, data.end - data.start);
	 */
	cedar_dma_flush_range((void *)data.start, (void *)data.end);
	if (copy_to_user((void __user *)arg, &data, sizeof(data)))
		return -EFAULT;

	return 0;
}

/* ioctl -> IOCTL_GET_CSI_ONLINE_INFO */
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