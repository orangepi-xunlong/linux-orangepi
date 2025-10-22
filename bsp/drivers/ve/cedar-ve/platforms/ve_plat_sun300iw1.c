/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2024, Dby <dby@allwinnertech.com>
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
#include <linux/dma-noncoherent.h>

#include "cedar_ve.h"

#define VE_REG_TOP_BASE		0x0000
#define VE_REG_VCUENC_BASE	0x0900
#define VE_REG_ENCPP_BASE	0x0a00
#define VE_REG_ENC_BASE		0x0b00
#define VE_REG_H265_EXT_BASE	0x0c00
#define VE_REG_DECJPEG_BASE	0x0e00

#define VE_REG_TOP_DECODER		(VE_REG_TOP_BASE + 0x2c)
#define VE_REG_TOP_ENCODER		(VE_REG_TOP_BASE + 0x44)
#define VE_REG_TOP_VCU_CFG		(VE_REG_TOP_BASE + 0x5c)

#define VE_REG_DECJPEG_INT_CTRL		(VE_REG_DECJPEG_BASE + 0x14)
#define VE_REG_DECJPEG_INT_STA		(VE_REG_DECJPEG_BASE + 0x1c)

#define VE_REG_ENCPP_INT_CTRL		(VE_REG_ENCPP_BASE + 0x08)
#define VE_REG_ENCPP_INT_STA		(VE_REG_ENCPP_BASE + 0x10)

#define VE_REG_ENC_INT_CTRL		(VE_REG_ENC_BASE + 0x14)
#define VE_REG_ENC_INT_STA		(VE_REG_ENC_BASE + 0x1c)

#define VE_REG_VCUENC_INT_CTRL		(VE_REG_VCUENC_BASE + 0x20)
#define VE_REG_VCUENC_INT_STA		(VE_REG_VCUENC_BASE + 0x28)
#define VE_REG_VCUENC_CSIINT_CTRL	(VE_REG_VCUENC_BASE + 0x70)
#define VE_REG_VCUENC_CSIERR_STA	(VE_REG_VCUENC_BASE + 0x78)
#define VE_REG_VCUENC_CSIWARN_STA	(VE_REG_VCUENC_BASE + 0x74)

#define VE_MARK_ONLINE_CSI_INT_EN         (0x003303FF)
#define VE_MARK_ONLINE_CSI_INT_ERROR_STA  (0x03330033)
#define VE_MARK_ONLINE_CSI_INT_WARING_STA (0x00230033)

/* IOCTL_GET_CSI_ONLINE_INFO variable */
typedef struct CsiOnlineRelatedInfo {
	/* target */
	unsigned int sensor_id;
	unsigned int bk_id;

	/* info */
	unsigned int csi_frame_start_cnt;
	unsigned int csi_sensor_id;
	unsigned int csi_bk_done;
	unsigned int csi_bk_error;
	unsigned int csi_cur_frame_addr;
	unsigned int csi_last_frame_addr;
} CsiOnlineRelatedInfo;

/* dvfs */
#define VE_DEBUGFS_MAX_CHANNEL	16
#define VE_DEBUGFS_BUF_SIZE	1024

struct ve_channel_proc_manager {
	struct ve_channel_proc_info proc_info;
	int channel_id;
	unsigned int active_flag;
};

struct ve_debugfs_proc_info_manager {
	struct mutex lock_proc;
	/* 0:default, 1:view debugging info after app finish, other:TODO*/
	int flag;
	/*when cat ve_advance, 0: just show advance info, 1: show base and advance info*/
	int advance_flag;
	/* to clear the previous channel proc info*/
	int ref_cnt;
	struct ve_channel_proc_manager ch_proc_mgr[VE_DEBUGFS_MAX_CHANNEL];
};

struct ve_dbgfs_h {
	struct dentry *dbg_root;
	struct ve_debugfs_proc_info_manager dbg_info;
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

	/* map for csi dma0&dma1 io space */
	iomap_addrs->regs_csi0 = of_iomap(node, 2);
	if (!iomap_addrs->regs_csi0)
		VE_LOGW("ve Can't map regs_csi0 registers\n");

	iomap_addrs->regs_csi1 = of_iomap(node, 3);
	if (!iomap_addrs->regs_csi1)
		VE_LOGW("ve Can't map regs_csi1 registers\n");

	return 0;
}

void reset_vcu(struct cedar_dev *cedar_devp, unsigned int irq_sta)
{
	volatile char *base_reg = cedar_devp->iomap_addrs.regs_ve;
	volatile char		 *reg_addr_top_44  = base_reg + 0x44;
	volatile unsigned int ori_val_top_44   = 0;
	volatile unsigned int reset_val_top_44 = 0;
	ori_val_top_44 = readl((void *)(reg_addr_top_44));

	VE_LOGI("reset vcu: irq_sta = 0x%x, irq_cnt = %d",
		irq_sta, cedar_devp->irq_cnt);

	//*step1: write 1 to vcu_reset
	reset_val_top_44 = ori_val_top_44 | 1<<4;
	writel(reset_val_top_44, (void *)(reg_addr_top_44));

	//*step2: write 1 to enc_reset
	reset_val_top_44 = ori_val_top_44 | 1<<0;
	writel(reset_val_top_44, (void *)(reg_addr_top_44));

	//*step3: write 0 to enc_reset
	reset_val_top_44 = ori_val_top_44 & (~(1<<0));
	writel(reset_val_top_44, (void *)(reg_addr_top_44));

	//*step4: write 0 to vcu_reset
	reset_val_top_44 = ori_val_top_44 & (~(1<<4));
	writel(reset_val_top_44, (void *)(reg_addr_top_44));

}

/* irq */
void ve_irq_work(struct cedar_dev *cedar_devp)
{
	wait_queue_head_t *wait_ve = &cedar_devp->wait_ve;
	volatile char *base_reg = cedar_devp->iomap_addrs.regs_ve;

	volatile unsigned int irq_en;
	volatile unsigned int irq_sta;
	volatile unsigned int csi_irq_en;
	volatile unsigned int csi_irq_sta_err;
	volatile unsigned int csi_irq_sta_warn;
	volatile unsigned int reg_val;
	int  vcu_enable = 0;

	/* decodec */
	reg_val = readl((void *)(base_reg + VE_REG_TOP_DECODER));
	if (reg_val & (1 << 3)) {
		irq_en = readl((void *)(base_reg + VE_REG_DECJPEG_INT_CTRL)) & (0x78);
		irq_sta = readl((void *)(base_reg + VE_REG_DECJPEG_INT_STA)) & (0xf);
		if (irq_sta && irq_en) {
			reg_val = readl((void *)(base_reg + VE_REG_DECJPEG_INT_CTRL));
			writel(reg_val & (~0x78), (void *)(base_reg + VE_REG_DECJPEG_INT_CTRL));
			irq_sta = readl((void *)(base_reg + VE_REG_DECJPEG_INT_STA));
			writel(irq_sta, (void *)(base_reg + VE_REG_DECJPEG_INT_STA));
			cedar_devp->jpeg_irq_value = 1;
			cedar_devp->jpeg_irq_flag = 1;
			wake_up(wait_ve);
		}
	}

	/* encodec */
	if (cedar_devp->ve_mode == VE_MODE_VCUENC) {
		goto VE_MODE_VCUENC_IRQ;
	}

	reg_val = readl((void *)(base_reg + VE_REG_TOP_ENCODER));
	if (reg_val & (1 << 6)) {
		irq_en = readl((void *)(base_reg + VE_REG_ENCPP_INT_CTRL)) & (0x21);
		irq_sta = readl((void *)(base_reg + VE_REG_ENCPP_INT_STA)) & (0x21);
		if (irq_sta && irq_en) {
			if (cedar_devp->ve_mode != VE_MODE_ENCPP) {
				VE_LOGI("irq encpp, but mode mismatch(%d)\n", cedar_devp->ve_mode);
			} else {
				VE_LOGD("irq encpp\n");
			}
			reg_val = readl((void *)(base_reg + VE_REG_ENCPP_INT_CTRL));
			writel(reg_val & (~0x1), (void *)(base_reg + VE_REG_ENCPP_INT_CTRL));
			irq_sta = readl((void *)(base_reg + VE_REG_ENCPP_INT_STA));
			writel(irq_sta, (void *)(base_reg + VE_REG_ENCPP_INT_STA));
			cedar_devp->en_irq_value = 1;
			cedar_devp->en_irq_flag = 1;
			wake_up(wait_ve);
		}
	}

	reg_val = readl((void *)(base_reg + VE_REG_TOP_ENCODER));
	if (reg_val & (1 << 7)) {
		irq_en = readl((void *)(base_reg + VE_REG_ENC_INT_CTRL)) & (0x1ff);
		irq_sta = readl((void *)(base_reg + VE_REG_ENC_INT_STA)) & (0x1ff);
		if (irq_sta && irq_en) {
			if (cedar_devp->ve_mode != VE_MODE_ENC) {
				VE_LOGI("irq enc, but mode mismatch(%d)\n", cedar_devp->ve_mode);
			} else {
				VE_LOGD("irq enc\n");
			}
			reg_val = readl((void *)(base_reg + VE_REG_ENC_INT_CTRL));
			writel(reg_val & (~0x1ff), (void *)(base_reg + VE_REG_ENC_INT_CTRL));
			irq_sta = readl((void *)(base_reg + VE_REG_ENC_INT_STA));
			writel(irq_sta, (void *)(base_reg + VE_REG_ENC_INT_STA));
			cedar_devp->en_irq_value = 1;
			cedar_devp->en_irq_flag = 1;
			wake_up(wait_ve);
		}
	}
	return;

VE_MODE_VCUENC_IRQ:

	/* mabye it is not ture int */
	reg_val = readl((void *)(base_reg + VE_REG_TOP_VCU_CFG));
	vcu_enable = reg_val & (1 << 0);
	if (vcu_enable == 0)
		return;

	cedar_devp->irq_cnt++;
	csi_irq_en = readl((void *)(base_reg + VE_REG_VCUENC_CSIINT_CTRL));
	csi_irq_sta_err = readl((void *)(base_reg + VE_REG_VCUENC_CSIERR_STA));
	csi_irq_sta_warn = readl((void *)(base_reg + VE_REG_VCUENC_CSIWARN_STA));
	VE_LOGD("vcu: irq-0x%x, err-0x%x, warn-0x%x\n",
		csi_irq_en, csi_irq_sta_err, csi_irq_sta_warn);

	/* clear interrupt from csi of online-mode */
	if (csi_irq_en & VE_MARK_ONLINE_CSI_INT_EN) {
		/*disable interrupt*/
		csi_irq_en &= (~VE_MARK_ONLINE_CSI_INT_EN);
		writel(csi_irq_en, (void *)(base_reg + VE_REG_VCUENC_CSIINT_CTRL));

		/*clear error and waring interrupt */
		csi_irq_sta_err &= VE_MARK_ONLINE_CSI_INT_ERROR_STA;
		if (csi_irq_sta_err)
			writel(csi_irq_sta_err, (void *)(base_reg + VE_REG_VCUENC_CSIERR_STA));

		csi_irq_sta_warn &= VE_MARK_ONLINE_CSI_INT_WARING_STA;
		if (csi_irq_sta_warn)
			writel(csi_irq_sta_warn, (void *)(base_reg + VE_REG_VCUENC_CSIWARN_STA));

		if (csi_irq_sta_err || csi_irq_sta_warn)
			VE_LOGI("csi_irq_sta_err = 0x%x, csi_irq_sta_warn = 0x%x, irq_cnt = %d", csi_irq_sta_err, csi_irq_sta_warn, cedar_devp->irq_cnt);
	}

	/* clear interrupt of vcu*/
	irq_en = readl((void *)(base_reg + VE_REG_VCUENC_INT_CTRL)) & (0xf);
	irq_sta = readl((void *)(base_reg + VE_REG_VCUENC_INT_STA)) & (0xf);
	if (irq_sta && irq_en) {
		if (cedar_devp->ve_mode != VE_MODE_VCUENC) {
			VE_LOGI("irq vcuenc, but mode mismatch(%d)\n", cedar_devp->ve_mode);
		} else {
			VE_LOGD("irq vcuenc\n");
		}
		reg_val = readl((void *)(base_reg + VE_REG_VCUENC_INT_CTRL));
		writel(reg_val & (~0xf), (void *)(base_reg + VE_REG_VCUENC_INT_CTRL));
		irq_sta = readl((void *)(base_reg + VE_REG_VCUENC_INT_STA));
		writel(irq_sta, (void *)(base_reg + VE_REG_VCUENC_INT_STA));

		/* reset vcu when had error interrupt flag */
		if (irq_sta & 0xE)
			reset_vcu(cedar_devp, (unsigned int)irq_sta);

		cedar_devp->vcuenc_irq_flag = 1;
		wake_up(wait_ve);
	}

	irq_sta = readl((void *)(base_reg + VE_REG_VCUENC_INT_STA)) & (0xf);
	csi_irq_sta_err = readl((void *)(base_reg + VE_REG_VCUENC_CSIERR_STA));
	csi_irq_sta_warn = readl((void *)(base_reg + VE_REG_VCUENC_CSIWARN_STA));
	csi_irq_sta_err  &= VE_MARK_ONLINE_CSI_INT_ERROR_STA;
	csi_irq_sta_warn &= VE_MARK_ONLINE_CSI_INT_WARING_STA;
	if (irq_sta || csi_irq_sta_err || csi_irq_sta_warn) {
		/* it is error: if irq-sta is not 0 after clear it*/
		VE_LOGI("error: irq_sta = 0x%x, csi_irq_sta_err = 0x%x, csi_irq_sta_warn = 0x%x, irq_cnt = %d",
			irq_sta, csi_irq_sta_err, csi_irq_sta_warn, cedar_devp->irq_cnt);
	}

}

/* dvfs */
int ve_dvfs_get_attr(struct cedar_dev *cedar_devp)
{
	struct ve_dvfs_attr *attr = &cedar_devp->dvfs_attr;

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

	return 0;
}

/* debug */
static int ve_debugfs_open(struct inode *inode, struct file *file)
{
	struct ve_dbgfs_h *dbgfs_h = (struct ve_dbgfs_h *)(inode->i_private);
	file->private_data = &dbgfs_h->dbg_info;
	return 0;
}

static ssize_t ve_debugfs_read(struct file *file, char __user *user_buf,
			       size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc_info_manager *dbg_info = file->private_data;
	int i = 0;
	int read_size = 0;
	unsigned char *src_data = NULL;
	unsigned int src_size = 0;
	unsigned int had_proc_data = 0;

	VE_LOGD("***** nbytes = %zd, ppos = %lld\n", nbytes, *ppos);

	if ((*ppos) > 0) {
		VE_LOGD("**had read once, ppos = %lld\n", *ppos);
		return 0;
	}

	mutex_lock(&dbg_info->lock_proc);
	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++) {
		if (dbg_info->ch_proc_mgr[i].proc_info.base_info_data) {
			src_data = dbg_info->ch_proc_mgr[i].proc_info.base_info_data;
			src_size = dbg_info->ch_proc_mgr[i].proc_info.base_info_size;
			if ((read_size + src_size) > nbytes) {
				/* had no enought buffer to read proc_info data*/
				VE_LOGW("read proc info: no enought buffer, "
					"max_size = %d, cur_total_size = %d\n",
					(int)nbytes, (int)(read_size + src_size));
				break;
			}

			*ppos = 0;
			read_size += simple_read_from_buffer(user_buf + read_size, nbytes, ppos,
							     src_data, src_size);
			had_proc_data = 1;
		}
	}
	*ppos = read_size;
	VE_LOGD("max_size = %d, read_size = %d\n", nbytes, read_size);

	if (had_proc_data == 0) {
		VE_LOGD("there is no any codec working currently.\n");

		if (dbg_info->flag == 0) {
			VE_LOGD("Usage:\n"
				"[1] If you want to restore defaults, please type this cmd:\n"
				"	 echo 0 > /sys/kernel/debug/mpp/ve\n"
				"[2] If you want to view debugging info after app finish, "
				"please type this cmd before app start:\n"
				"	 echo 1 > /sys/kernel/debug/mpp/ve\n"
				"[3] TODO.\n");
		} else if (dbg_info->flag == 1) {
			VE_LOGD("Please run app first.\n");
		} else {
			VE_LOGD("Invalid flag: %d, Future support.\n", dbg_info->flag);
		}
		mutex_unlock(&dbg_info->lock_proc);
		return 0;
	}
	mutex_unlock(&dbg_info->lock_proc);

	return read_size;
}


static ssize_t ve_debugfs_write(struct file *file, const char __user *user_buf,
				size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc_info_manager *dbg_info = file->private_data;
	int val;
	int ret;
	char info[32];

	if (32 <= nbytes) {
		VE_LOGE("invalid params, nbytes=%zu(>=32)\n", nbytes);
		return 0;
	}

	memset(info, 0, 32);
	if (copy_from_user(info, user_buf, nbytes)) {
		VE_LOGE("copy_from_user fail\n");
		return 0;
	}
/*//defined CONFIG_VIDEO_RT_MEDIA
	int i = 0;
	char *s_loglevel[LOGLEVEL_NUM] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
	char *c_loglevel = NULL;
	if (strstr(info, "log") != NULL) {
		for (i = 0; i < LOGLEVEL_NUM; i++) {
			c_loglevel = strstr(info, s_loglevel[i]);
			if (c_loglevel != NULL) {
				ret = kstrtoint(c_loglevel, 10, &debug_fs_set_log_level);
				if (ret) {
					debug_fs_set_log_level = 0;
					VE_LOGE("kstrtoint fail, ret=%d\n", ret);
					return 0;
				}
				VE_LOGD("set debug_fs_set_log_level %d\n", debug_fs_set_log_level);
				break;
			}
		}
		if (i >= LOGLEVEL_NUM)
			VE_LOGD("not find loglevel\n");
	} else
*/
	{
		ret = kstrtoint(info, 10, &val);
		if (ret) {
			VE_LOGE("kstrtoint fail, ret=%d\n", ret);
			return 0;
		}

		mutex_lock(&dbg_info->lock_proc);
		dbg_info->flag = val;
		VE_LOGD("debugfs write flag:%d (0:default, 1:view debugging info after app "
			"finish, other:TODO)\n", dbg_info->flag);
		mutex_unlock(&dbg_info->lock_proc);
	}

	return nbytes;
}

static int ve_debugfs_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static const struct file_operations ve_debugfs_fops = {
	.owner   = THIS_MODULE,
	.open    = ve_debugfs_open,
	.llseek  = no_llseek,
	.read    = ve_debugfs_read,
	.write   = ve_debugfs_write,
	.release = ve_debugfs_release,
};

static int ve_debugfs_advance_open(struct inode *inode, struct file *file)
{
	struct ve_dbgfs_h *dbgfs_h = (struct ve_dbgfs_h *)(inode->i_private);
	file->private_data = &dbgfs_h->dbg_info;
	return 0;
}

static ssize_t ve_debugfs_advance_read(struct file *file, char __user *user_buf,
				       size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc_info_manager *dbg_info = file->private_data;
	int i = 0;
	int read_size = 0;
	unsigned char *src_data = NULL;
	unsigned int   src_size = 0;
	unsigned int   had_proc_data = 0;

	VE_LOGD("***** nbytes = %zd, ppos = %lld\n", nbytes, *ppos);

	if ((*ppos) > 0) {
		VE_LOGD("**had read once, ppos = %lld\n", *ppos);
		return 0;
	}

	mutex_lock(&dbg_info->lock_proc);

	for (i = 0; i < VE_DEBUGFS_MAX_CHANNEL; i++) {
		if (dbg_info->ch_proc_mgr[i].proc_info.base_info_data) {

			/*show base and advance proc info when advance_flag is 1*/
			if (dbg_info->advance_flag == 1) {
				src_data = dbg_info->ch_proc_mgr[i].proc_info.base_info_data;
				src_size = dbg_info->ch_proc_mgr[i].proc_info.base_info_size;
				if ((read_size + src_size) > nbytes) {
					/* had no enought buffer to read proc_info data*/
					VE_LOGW("read proc info: no enought buffer, "
						"max_size = %d, cur_total_size = %d\n",
						(int)nbytes, (int)(read_size + src_size));
					break;
				}

				*ppos = 0;
				read_size += simple_read_from_buffer(user_buf + read_size, nbytes,
								     ppos, src_data, src_size);
			}

			src_data = dbg_info->ch_proc_mgr[i].proc_info.advance_info_data;
			src_size = dbg_info->ch_proc_mgr[i].proc_info.advance_info_size;
			if ((read_size + src_size) > nbytes) {
				/* had no enought buffer to read proc_info data*/
				VE_LOGW("read proc info: no enought buffer, "
					"max_size = %d, cur_total_size = %d\n",
					(int)nbytes, (int)(read_size + src_size));
				break;
			}

			*ppos = 0;
			read_size += simple_read_from_buffer(user_buf + read_size, nbytes, ppos,
							     src_data, src_size);
			had_proc_data = 1;
		}
	}
	*ppos = read_size;
	VE_LOGD("max_size = %d, read_size = %d\n", nbytes, read_size);

	if (had_proc_data == 0) {
		VE_LOGD("there is no any codec working currently.\n");
		if (dbg_info->flag == 0) {
			VE_LOGD("Usage:\n"
				"[1] If you want to restore defaults, please type this cmd:\n"
				"	 echo 0 > /sys/kernel/debug/mpp/ve\n"
				"[2] If you want to view debugging info after app finish, "
				"please type this cmd before app start:\n"
				"	 echo 1 > /sys/kernel/debug/mpp/ve\n"
				"[3] TODO.\n");
		} else if (dbg_info->flag == 1)
			VE_LOGD("Please run app first.\n");
		else
			VE_LOGD("Invalid flag: %d, Future support.\n", dbg_info->flag);

		mutex_unlock(&dbg_info->lock_proc);
		return 0;
	}
	mutex_unlock(&dbg_info->lock_proc);

	return read_size;
}

static ssize_t ve_debugfs_advance_write(struct file *file, const char __user *user_buf,
					size_t nbytes, loff_t *ppos)
{
	struct ve_debugfs_proc_info_manager *dbg_info = file->private_data;
	int val;
	int ret;
	char info[32];

	if (nbytes >= 32) {
		VE_LOGE("invalid params, nbytes=%zu(>=32)\n", nbytes);
		return 0;
	}

	memset(info, 0, 32);
	if (copy_from_user(info, user_buf, nbytes)) {
		VE_LOGE("copy_from_user fail\n");
		return 0;
	}

	ret = kstrtoint(info, 10, &val);
	if (ret) {
		VE_LOGE("kstrtoint fail, ret=%d\n", ret);
		return 0;
	}

	mutex_lock(&dbg_info->lock_proc);
	dbg_info->advance_flag = val;
	VE_LOGD("debugfs write advance flag:%d (when cat ve_advance, "
		"0: just show advance info, 1: show base and advance info)\n",
		dbg_info->advance_flag);
	mutex_unlock(&dbg_info->lock_proc);

	return nbytes;
}

static int ve_debugfs_advance_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static const struct file_operations ve_debugfs_advance_fops = {
	.owner   = THIS_MODULE,
	.open    = ve_debugfs_advance_open,
	.llseek  = no_llseek,
	.read    = ve_debugfs_advance_read,
	.write   = ve_debugfs_advance_write,
	.release = ve_debugfs_advance_release,
};

ve_dbgfs_t *ve_debug_register_driver(struct cedar_dev *cedar_devp)
{
	struct ve_dbgfs_h *dbgfs_h = NULL;
	struct ve_debugfs_proc_info_manager *dbg_info = NULL;
	struct dentry *dent;
	(void)cedar_devp;

	dbgfs_h = kzalloc(sizeof(*dbgfs_h), GFP_KERNEL);
	if (!dbgfs_h) {
		VE_LOGE("kzalloc fail\n");
		return NULL;
	}
	dbg_info = &dbgfs_h->dbg_info;

#if defined(CONFIG_SUNXI_MPP)
	dbgfs_h->dbg_root = debugfs_mpp_root;
#else
	dbgfs_h->dbg_root = debugfs_lookup("mpp", NULL);
	if (IS_ERR_OR_NULL(dbgfs_h->dbg_root))
		dbgfs_h->dbg_root = debugfs_create_dir("mpp", NULL);
#endif
	if (dbgfs_h->dbg_root == NULL) {
		VE_LOGE("get debugfs_mpp_root is NULL, please check mpp\n");
		return NULL;
	}

	dent = debugfs_create_file("ve_base", 0644, dbgfs_h->dbg_root,
				   dbgfs_h, &ve_debugfs_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(dbgfs_h->dbg_root);
		dbgfs_h->dbg_root = NULL;
		return NULL;
	}

	dent = debugfs_create_file("ve_advance", 0644, dbgfs_h->dbg_root,
				   dbgfs_h, &ve_debugfs_advance_fops);
	if (IS_ERR_OR_NULL(dent)) {
		VE_LOGE("Unable to create debugfs status file.\n");
		debugfs_remove_recursive(dbgfs_h->dbg_root);
		dbgfs_h->dbg_root = NULL;
		return NULL;
	}

	memset(dbg_info, 0, sizeof(struct ve_debugfs_proc_info_manager));
	mutex_init(&dbg_info->lock_proc);
	dbg_info->flag = 1; /* default: view debugging info after app finish. */
	VE_LOGD("dbg_info: flag = %d\n", dbg_info->flag);

	return dbgfs_h;
}

void ve_debug_unregister_driver(ve_dbgfs_t *dbgfs)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	struct ve_debugfs_proc_info_manager *dbg_info = NULL;
	int i;

	if (dbgfs_h == NULL) {
		VE_LOGW("dbgfs invalid\n");
		return;
	}
	if (dbgfs_h->dbg_root == NULL) {
		VE_LOGW("note: debug root already is null\n");
		return;
	}
	dbg_info = &dbgfs_h->dbg_info;

	debugfs_remove_recursive(dbgfs_h->dbg_root);
	dbgfs_h->dbg_root = NULL;

	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (dbg_info->ch_proc_mgr[i].proc_info.base_info_data != NULL)
			vfree(dbg_info->ch_proc_mgr[i].proc_info.base_info_data);

		if (dbg_info->ch_proc_mgr[i].proc_info.advance_info_data != NULL)
			vfree(dbg_info->ch_proc_mgr[i].proc_info.advance_info_data);
	}
	mutex_destroy(&dbg_info->lock_proc);
}

static void reset_proc_info(struct ve_dbgfs_h *dbgfs_h)
{
	struct ve_debugfs_proc_info_manager *dbg_info = &dbgfs_h->dbg_info;
	struct ve_channel_proc_manager *cur_ch_proc_mgr = NULL;
	int i = 0;

	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		cur_ch_proc_mgr = &dbg_info->ch_proc_mgr[i];

		if (cur_ch_proc_mgr->proc_info.base_info_data)
			vfree(cur_ch_proc_mgr->proc_info.base_info_data);
		if (cur_ch_proc_mgr->proc_info.advance_info_data)
			vfree(cur_ch_proc_mgr->proc_info.advance_info_data);

		memset(cur_ch_proc_mgr, 0, sizeof(struct ve_channel_proc_manager));
	}
}

void ve_debug_open(ve_dbgfs_t *dbgfs, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	struct ve_debugfs_proc_info_manager *dbg_info = NULL;
	(void)vi;

	if (dbgfs_h == NULL) {
		VE_LOGW("dbgfs invalid\n");
		return;
	}
	dbg_info = &dbgfs_h->dbg_info;

	mutex_lock(&dbg_info->lock_proc);
	if (dbg_info->ref_cnt == 0)
		reset_proc_info(dbgfs_h);

	dbg_info->ref_cnt++;
	mutex_unlock(&dbg_info->lock_proc);
}

void ve_debug_release(ve_dbgfs_t *dbgfs, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	struct ve_debugfs_proc_info_manager *dbg_info = NULL;
	(void)vi;

	if (dbgfs_h == NULL) {
		VE_LOGW("dbgfs invalid\n");
		return;
	}
	dbg_info = &dbgfs_h->dbg_info;

	mutex_lock(&dbg_info->lock_proc);
	dbg_info->ref_cnt--;
	mutex_unlock(&dbg_info->lock_proc);
}

/* ioctl -> IOCTL_FLUSH_CACHE_RANGE */
int ioctl_flush_cache_range(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp)
{
	(void)arg;
	(void)user;
	(void)cedar_devp;
	return 0;
}

int ioctl_get_csi_online_related_info(unsigned long arg, uint8_t from_kernel, struct cedar_dev *cedar_devp)
{
	CsiOnlineRelatedInfo mCsiInfo[16];
	unsigned long flags;
	unsigned int i = 0;
	volatile char *regs_csi = NULL;
	volatile unsigned int reg_value = 0;
	unsigned int reg_csi_hist_id, reg_csi_hist_addr;
	CsiOnlineRelatedInfo *pcsi_info = NULL;
	CsiOnlineRelatedInfo *pcsi_infos = mCsiInfo;
    memset(mCsiInfo, 0, sizeof(CsiOnlineRelatedInfo) * 16);

	data_to_kernel((void *)mCsiInfo, (void *)arg, sizeof(CsiOnlineRelatedInfo) * 16, from_kernel);
	spin_lock_irqsave(&cedar_devp->lock, flags);
	for (i = 0; i < 16; ++i) {
		pcsi_info = &pcsi_infos[i];
		if (i < 8) {
			regs_csi = cedar_devp->iomap_addrs.regs_csi0;
			reg_csi_hist_id = 0x120 + (i << 3);
			reg_csi_hist_addr = 0x124 + (i << 3);
		} else {
			regs_csi = cedar_devp->iomap_addrs.regs_csi1;
			reg_csi_hist_id = 0x120 + ((i - 8) << 3);
			reg_csi_hist_addr = 0x124 + ((i - 8) << 3);
		}

		reg_value = readl(regs_csi + reg_csi_hist_id);
		pcsi_info->csi_frame_start_cnt = reg_value & 0xFF;
		pcsi_info->csi_sensor_id = (reg_value & 0x300) >> 8;
		pcsi_info->csi_bk_done = (reg_value & 0x1000) >> 12;
		pcsi_info->csi_bk_error = (reg_value & 0x2000) >> 13;
		reg_value = readl(regs_csi + reg_csi_hist_addr);
		pcsi_info->csi_cur_frame_addr = reg_value;
		/* TODO: why << 2 */
		pcsi_info->csi_cur_frame_addr = pcsi_info->csi_cur_frame_addr << 2;
	}
	spin_unlock_irqrestore(&cedar_devp->lock, flags);

	data_to_other((void *)arg, (void *)mCsiInfo, sizeof(CsiOnlineRelatedInfo) * 16, from_kernel);
	return 0;
}

/* ioctl -> IOCTL_SET_PROC_INFO
 *       -> IOCTL_STOP_PROC_INFO
 *       -> IOCTL_COPY_PROC_INFO
 */
static long setup_proc_info(struct ve_dbgfs_h *dbgfs_h, unsigned long usr_arg, int b_from_user)
{
	struct ve_debugfs_proc_info_manager *dbg_info = &dbgfs_h->dbg_info;
	int i = 0;
	struct ve_channel_proc_info ch_proc_info_user;
	struct ve_channel_proc_manager *cur_ch_proc_mgr = NULL;
	unsigned int cur_channel_id = 0;

	memset(&ch_proc_info_user, 0, sizeof(struct ve_channel_proc_info));
	if (dbgfs_h->dbg_root == NULL)
		return 0;
	if (b_from_user) {
		if (copy_from_user(&ch_proc_info_user, (void __user *)usr_arg,
				   sizeof(struct ve_channel_proc_info))) {
			VE_LOGW("IOCTL_SET_PROC_INFO copy_from_user fail\n");
			return -EFAULT;
		}
	} else {
		memcpy(&ch_proc_info_user, (ve_channel_proc_info *)usr_arg,
		       sizeof(ve_channel_proc_info));
	}
	cur_channel_id = ch_proc_info_user.channel_id;

	VE_LOGD("*base_size = %d, advance_size = %d, struct_size = %d\n",
		ch_proc_info_user.base_info_size, ch_proc_info_user.advance_info_size,
		sizeof(struct ve_channel_proc_info));

	if (ch_proc_info_user.base_info_size == 0 || ch_proc_info_user.base_info_data == NULL) {
		VE_LOGW("invalid base info, size = %d, data = %p\n",
			ch_proc_info_user.base_info_size, ch_proc_info_user.base_info_data);
		return 0;
	}

	/* check whether had the-match channel*/
	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (dbg_info->ch_proc_mgr[i].channel_id == cur_channel_id
			&& dbg_info->ch_proc_mgr[i].active_flag == 1) {
			break;
		}
	}
	VE_LOGD("channel_id = %d, i = %d\n", cur_channel_id, i);

	if (i >= MAX_VE_DEBUG_INFO_NUM) {
		for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
			VE_LOGD("find channel, active_flag = %d, i = %d\n",
				dbg_info->ch_proc_mgr[i].active_flag, i);
			if (dbg_info->ch_proc_mgr[i].active_flag == 0)
				break;
		}
		if (i >= MAX_VE_DEBUG_INFO_NUM) {
			VE_LOGE("cannot find empty channel proc, max_ch = %d\n",
				MAX_VE_DEBUG_INFO_NUM);
			return 0;
		}
		cur_ch_proc_mgr = &dbg_info->ch_proc_mgr[i];
		if (cur_ch_proc_mgr->proc_info.base_info_data)
			vfree(cur_ch_proc_mgr->proc_info.base_info_data);
		if (cur_ch_proc_mgr->proc_info.advance_info_data)
			vfree(cur_ch_proc_mgr->proc_info.advance_info_data);

		cur_ch_proc_mgr->proc_info.base_info_data =
			vmalloc(ch_proc_info_user.base_info_size);
		if (cur_ch_proc_mgr->proc_info.base_info_data == NULL) {
			VE_LOGE("vmalloc failed, size = %d\n", ch_proc_info_user.base_info_size);
			return 0;
		}
		cur_ch_proc_mgr->proc_info.base_info_size = ch_proc_info_user.base_info_size;
		memset(cur_ch_proc_mgr->proc_info.base_info_data, 0,
		       ch_proc_info_user.base_info_size);

		if (ch_proc_info_user.advance_info_size > 0) {
			cur_ch_proc_mgr->proc_info.advance_info_data =
				vmalloc(ch_proc_info_user.advance_info_size);
			if (cur_ch_proc_mgr->proc_info.advance_info_data == NULL) {
				VE_LOGE("vmalloc failed, size = %d\n",
					ch_proc_info_user.advance_info_size);
				return 0;
			}
			cur_ch_proc_mgr->proc_info.advance_info_size =
				ch_proc_info_user.advance_info_size;
			memset(cur_ch_proc_mgr->proc_info.advance_info_data, 0,
				ch_proc_info_user.advance_info_size);
		} else {
			cur_ch_proc_mgr->proc_info.advance_info_data = NULL;
			cur_ch_proc_mgr->proc_info.advance_info_size = 0;
		}

		cur_ch_proc_mgr->active_flag = 1;
		cur_ch_proc_mgr->channel_id  = cur_channel_id;
	} else {
		cur_ch_proc_mgr = &dbg_info->ch_proc_mgr[i];

		/* re-vmalloc buffer if not enought*/
		if (cur_ch_proc_mgr->proc_info.base_info_size != ch_proc_info_user.base_info_size) {
			vfree(cur_ch_proc_mgr->proc_info.base_info_data);

			cur_ch_proc_mgr->proc_info.base_info_data =
				vmalloc(ch_proc_info_user.base_info_size);
			if (cur_ch_proc_mgr->proc_info.base_info_data == NULL) {
				VE_LOGE("vmalloc failed, size = %d\n",
					ch_proc_info_user.base_info_size);
				return 0;
			}
			cur_ch_proc_mgr->proc_info.base_info_size =
				ch_proc_info_user.base_info_size;

		}
		memset(cur_ch_proc_mgr->proc_info.base_info_data, 0,
			ch_proc_info_user.base_info_size);

		if (cur_ch_proc_mgr->proc_info.advance_info_size !=
		    ch_proc_info_user.advance_info_size) {
			vfree(cur_ch_proc_mgr->proc_info.advance_info_data);
			cur_ch_proc_mgr->proc_info.advance_info_data = NULL;
			cur_ch_proc_mgr->proc_info.advance_info_size = 0;

			if (ch_proc_info_user.advance_info_data &&
			    ch_proc_info_user.advance_info_size > 0) {
				cur_ch_proc_mgr->proc_info.advance_info_data =
					vmalloc(ch_proc_info_user.advance_info_size);
				if (cur_ch_proc_mgr->proc_info.advance_info_data == NULL) {
					VE_LOGE("vmalloc failed, size = %d\n",
						ch_proc_info_user.advance_info_size);
					return 0;
				}
				cur_ch_proc_mgr->proc_info.advance_info_size =
					ch_proc_info_user.advance_info_size;
			}

		}
		if (cur_ch_proc_mgr->proc_info.advance_info_data)
			memset(cur_ch_proc_mgr->proc_info.advance_info_data, 0,
				ch_proc_info_user.advance_info_size);
	}

	/*copy proc info data*/
	if (b_from_user) {
		if (copy_from_user(cur_ch_proc_mgr->proc_info.base_info_data,
				   (void __user *)ch_proc_info_user.base_info_data,
				   ch_proc_info_user.base_info_size)) {
			VE_LOGW("IOCTL_SET_PROC_INFO copy_from_user fail\n");
			return -EFAULT;
		}

		if (ch_proc_info_user.advance_info_data &&
		    ch_proc_info_user.advance_info_size > 0) {
			if (copy_from_user(cur_ch_proc_mgr->proc_info.advance_info_data,
					   (void __user *)ch_proc_info_user.advance_info_data,
					   ch_proc_info_user.advance_info_size)) {
				VE_LOGW("IOCTL_SET_PROC_INFO copy_from_user fail\n");
				return -EFAULT;
			}
		}
	} else {
		memcpy(cur_ch_proc_mgr->proc_info.base_info_data, ch_proc_info_user.base_info_data,
		       ch_proc_info_user.base_info_size);

		if (ch_proc_info_user.advance_info_data && ch_proc_info_user.advance_info_size > 0)
			memcpy(cur_ch_proc_mgr->proc_info.advance_info_data,
			       ch_proc_info_user.advance_info_data,
			       ch_proc_info_user.advance_info_size);
	}

	return 0;
}

static long stop_proc_info(struct ve_dbgfs_h *dbgfs_h, unsigned int channel_id)
{
	struct ve_debugfs_proc_info_manager *dbg_info = &dbgfs_h->dbg_info;
	struct ve_channel_proc_manager *cur_ch_proc_mgr = NULL;
	int i = 0;

	for (i = 0; i < MAX_VE_DEBUG_INFO_NUM; i++) {
		if (dbg_info->ch_proc_mgr[i].channel_id == channel_id
			&& dbg_info->ch_proc_mgr[i].active_flag == 1) {
			break;
		}
	}

	if (i >= MAX_VE_DEBUG_INFO_NUM) {
		VE_LOGI("can not find match channel, id = %d\n", channel_id);
		return 0;
	}

	cur_ch_proc_mgr = &dbg_info->ch_proc_mgr[i];

	if (dbg_info->flag == 1) {
		cur_ch_proc_mgr->active_flag = 0;
	} else {
		if (cur_ch_proc_mgr->proc_info.base_info_data)
			vfree(cur_ch_proc_mgr->proc_info.base_info_data);
		if (cur_ch_proc_mgr->proc_info.advance_info_data)
			vfree(cur_ch_proc_mgr->proc_info.advance_info_data);
		cur_ch_proc_mgr->proc_info.base_info_data = NULL;
		cur_ch_proc_mgr->proc_info.base_info_size = 0;
		cur_ch_proc_mgr->proc_info.advance_info_data = NULL;
		cur_ch_proc_mgr->proc_info.advance_info_size = 0;
		cur_ch_proc_mgr->active_flag = 0;
		cur_ch_proc_mgr->channel_id  = -1;
	}

	return 0;
}

int ioctl_set_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	struct ve_debugfs_proc_info_manager *dbg_info = NULL;
	int ret;
	(void)vi;

	if (dbgfs_h == NULL) {
		VE_LOGW("dbgfs invalid\n");
		return -1;
	}
	if (dbgfs_h->dbg_root == NULL) {
		VE_LOGW("note: debug root already is null\n");
		return -1;
	}
	dbg_info = &dbgfs_h->dbg_info;

	mutex_lock(&dbg_info->lock_proc);
	ret = setup_proc_info(dbgfs_h, arg, 1);
	mutex_unlock(&dbg_info->lock_proc);
	return ret;
}

int ioctl_copy_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi)
{
	(void)dbgfs;
	(void)arg;
	(void)vi;
	/* NULL */
	return 0;
}

int ioctl_stop_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi)
{
	struct ve_dbgfs_h *dbgfs_h = (ve_dbgfs_t *)dbgfs;
	struct ve_debugfs_proc_info_manager *dbg_info = NULL;
	unsigned int channel_id = (unsigned int)arg;
	int ret;
	(void)vi;

	if (dbgfs_h == NULL) {
		VE_LOGW("dbgfs invalid\n");
		return -1;
	}
	if (dbgfs_h->dbg_root == NULL) {
		VE_LOGW("note: debug root already is null\n");
		return -1;
	}
	dbg_info = &dbgfs_h->dbg_info;

	mutex_lock(&dbg_info->lock_proc);
	ret = stop_proc_info(dbgfs_h, channel_id);
	mutex_unlock(&dbg_info->lock_proc);

	return ret;
}
