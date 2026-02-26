/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *    Filename: cedarv_ve.h
 *     Version: 0.01alpha
 * Description: Video engine driver API, Don't modify it in user space.
 *     License: GPLv2
 *
 *		Author  : xyliu <xyliu@allwinnertech.com>
 *		Date    : 2016/04/13
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
 /* Notice: It's video engine driver API, Don't modify it in user space. */
#ifndef _CEDAR_VE_H_
#define _CEDAR_VE_H_
#include <sunxi-log.h>
#include <asm/io.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <uapi/linux/cedar_ve_uapi.h>
#include <asm/uaccess.h>
#include "ve_mem_list.h"

#if IS_ENABLED(CONFIG_AW_LOG_VERBOSE)
#define VE_LOGK(fmt, arg...) pr_info(fmt, ## arg)
#define VE_LOGV(fmt, arg...)
#define VE_LOGD(fmt, arg...) sunxi_debug(NULL, fmt, ## arg)
#define VE_LOGI(fmt, arg...) sunxi_info(NULL,  fmt, ## arg)
#define VE_LOGW(fmt, arg...) sunxi_warn(NULL,  fmt, ## arg)
#define VE_LOGE(fmt, arg...) sunxi_err(NULL,   fmt, ## arg)
#else
#define VE_LOGK(fmt, arg...) pr_info(fmt, ##arg)
#define VE_LOGV(fmt, arg...)
#define VE_LOGD(fmt, arg...) sunxi_debug(NULL, "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define VE_LOGI(fmt, arg...) sunxi_info(NULL,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define VE_LOGW(fmt, arg...) sunxi_warn(NULL,  "%d %s(): "fmt, __LINE__, __func__, ## arg)
#define VE_LOGE(fmt, arg...) sunxi_err(NULL,   "%d %s(): "fmt, __LINE__, __func__, ## arg)
#endif

#define MAX_VE_DEBUG_INFO_NUM	(16)

#define VE_LOCK_VDEC		0x01
#define VE_LOCK_VENC		0x02
#define VE_LOCK_JDEC		0x04
#define VE_LOCK_00_REG		0x08
#define VE_LOCK_04_REG		0x10
#define VE_LOCK_ERR		0x80
#define VE_LOCK_PROC_INFO	0x1000

typedef void ve_dbgfs_t;

enum VE_MODULE {
	/* ve normal: use to scene1 or scene2
	 * scene1. enc&dec share iummu (when dual iommu must be use with VE_MODULE_1);
	 * scene2. dec independ iommu (when dual iommu must be use with VE_MODULE_1);
	 */
	VE_MODULE_NORMAL = 0,
	/* ve 1: cooperate VE_MODULE_NORMAL to enable second iommu when dual iommu. */
	VE_MODULE_1,
	/* ve 2: enc independ iommu. */
	VE_MODULE_2,
};

struct debug_head_info {
	unsigned int pid;
	unsigned int tid;
	unsigned int length;
};

struct ve_debug_info {
	struct debug_head_info head_info;
	char *data;
};

struct iomap_para {
	volatile char *regs_ve;
	volatile char *regs_sys_cfg;
	resource_size_t ve_reg_start;
	volatile char *regs_csi0;
	volatile char *regs_csi1;
};

enum VE_MODE {
	VE_MODE_NULL = -1,
	VE_MODE_ENCPP = 0,
	VE_MODE_ENC,
	VE_MODE_DE,
	VE_MODE_VCUENC,
	VE_MODE_VCUDEC,
	VE_MODE_CNT,
};

struct ve_dvfs_info {
	unsigned int dvfs_index;
	unsigned int voltage; /* mv */
	unsigned int ve_freq; /* MHz */
};

struct ve_dvfs_attr {
	unsigned int default_freq;
	unsigned int dvfs_array_num;
	struct ve_dvfs_info *dvfs_array;
};

struct ve_case_load_info {
	struct ve_case_load_param load_param;
	int is_used;
	u32 process_channel_id;
};

struct ve_performat_info {
	unsigned int codec_formmat;
	unsigned int level_performance;
	unsigned int start_pixels;
	unsigned int end_pixels;
	unsigned int ve_freq;
};

struct cedar_ve_quirks {
	enum VE_MODULE ve_mod;
	char *class_name;
	char *dev_name;

	void *priv;	/* struct cedar_dev */
};

struct cedar_dev {
	/* device */
	dev_t ve_dev;
	struct cdev cdev;		/* char device struct */
	struct device *dev;		/* ptr to class device struct */
	struct class *class;		/* class for auto create device node */
	struct device *plat_dev;	/* ptr to class device struct */
	const struct cedar_ve_quirks *quirks;

	struct semaphore sem;		/* mutual exclusion semaphore */
	spinlock_t lock;
	wait_queue_head_t wq;		/* wait queue for poll ops */

	struct iomap_para iomap_addrs;	/* io remap addrs */

	u32 irq;			/* cedar video engine irq number */
	u32 de_irq_flag;		/* flag of video decoder engine irq generated */
	u32 de_irq_value;		/* value of video decoder engine irq */
	u32 en_irq_flag;		/* flag of video encoder engine irq generated */
	u32 en_irq_value;		/* value of video encoder engine irq */
	u32 irq_has_enable;
	int ref_count;
	int last_min_freq;

	u32 jpeg_irq_flag;		/* flag of video jpeg dec irq generated */
	u32 jpeg_irq_value;		/* value of video jpeg dec  irq */

	struct mutex lock_vdec;
	struct mutex lock_jdec;
	struct mutex lock_venc;
	struct mutex lock_00_reg;
	struct mutex lock_04_reg;
	struct aw_mem_list_head list;	/* buffer list */
	struct mutex lock_mem;
	unsigned char bMemDevAttachFlag;
	struct ve_debug_info debug_info[MAX_VE_DEBUG_INFO_NUM];
	int debug_info_cur_index;
	struct mutex lock_debug_info;

	/* clk */
	struct reset_control *reset;
	struct clk *ve_clk;
#if IS_ENABLED(CONFIG_ARCH_SUN50IW12P1)
	struct reset_control *reset_ve;
	struct clk *bus_ve3_clk;
	struct clk *bus_ve_clk;
	struct clk *mbus_ve3_clk;
#else
	struct clk *mbus_clk;
	struct clk *bus_clk;
#if IS_ENABLED(CONFIG_ARCH_SUN300IW1P1)
	struct clk *sbus_clk;
	struct clk *hbus_clk;
	struct clk *pll_clk;
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	/* enable : dereset->ahb_gate->mbus_gate->mbus_clk->bus_clk->ve_clk
	 * disable: ve_clk->bus_clk->mbus_clk->mbus_gate->ahb_gate->reset
	 * setrate: {disable}ve_clk->bus_clk->mbus_clk->mbus_gate->ahb_gate->
	 *          set ve_clk rate ->
	 *          {enable }ahb_gate->mbus_gate->mbus_clk->bus_clk->ve_clk
	 */
	struct clk *pll_ve;
	struct clk *ahb_gate;
	struct clk *mbus_gate;
#endif
#endif
	int clk_status;
	/* clk end */

	/* dvfs */
	struct ve_dvfs_attr dvfs_attr;
	unsigned int case_load_channels;
	struct ve_case_load_info *load_infos;
	unsigned int performat_max_freq;
	unsigned int performat_num;
	struct ve_performat_info *performat_infos;

	struct dentry *dvfs_root;

	struct regulator *regulator;
	int voltage;			/* mv */

	int user_setting_ve_freq;	/* MHz */
	int ve_freq_setup_by_dts_config;
	/* dvfs ned */

	int bInitEndFlag;

	enum VE_MODE ve_mode;
	u32 vcuenc_irq_flag;			/* flag of vcu of encoder engine irq generated */
	u32 vcudec_irq_flag;
	u32 vcuenc_csi_irq_flag;
	u32 irq_cnt;

	u32 ve_top_reg_offset;

	u32 process_channel_id_cnt;
	wait_queue_head_t wait_ve;
	struct regulator *regu;
	ve_dbgfs_t *dbgfs;
};

struct ve_info { /* each object will bind a new file handler */
	struct cedar_dev *cedar_devp;

	unsigned int set_vol_flag;

	struct mutex lock_flag_io;
	u32 lock_flags; /* if flags is 0, means unlock status */
	u32 process_channel_id;
};

static inline long data_to_kernel(void *dst, void *src, unsigned int size, unsigned int from_kernel)
{
	if (from_kernel) {
		memcpy(dst, src, size);
	} else {
		if (copy_from_user(dst, (void __user *)src, size)) {
			VE_LOGW("IOCTL_GET_VE_DEFAULT_FREQ copy_from_user fail\n");
			return -EFAULT;
		}
	}
	return 0;
}

static inline long data_to_other(void *dst, void *src, unsigned int size, unsigned int from_kernel)
{
	if (from_kernel) {
		memcpy(dst, src, size);
	} else {
		if (copy_to_user((void __user *)dst, src, size)) {
			VE_LOGE("ve get iommu copy_to_user error\n");
			return -EFAULT;
		}
	}
	return 0;
}

/*** for rt media only begain ***/
void *rt_cedardev_open(const char *dev_name);	/* e.g. "cedar_dev", "cedar_dev_ve2" */
void *rt_cedardev_mmap(void *info_handle);
int rt_cedardev_release(void *info_handle);
int rt_cedardev_ioctl(void *info_handle, unsigned int cmd, unsigned long arg);
/*** for rt media only end    ***/

/*** pltform code new struct definition begain ***/
int resource_iomap_init(struct device_node *node, struct iomap_para *iomap_addrs);
void ve_irq_work(struct cedar_dev *cedar_devp);
int ve_dvfs_get_attr(struct cedar_dev *cedar_devp);
int ioctl_flush_cache_range(unsigned long arg, uint8_t user, struct cedar_dev *cedar_devp);
int ioctl_get_csi_online_related_info(unsigned long arg, uint8_t from_kernel, struct cedar_dev *cedar_devp);

/* debug */
ve_dbgfs_t *ve_debug_register_driver(struct cedar_dev *cedar_devp);
void ve_debug_unregister_driver(ve_dbgfs_t *dbgfs);
void ve_debug_open(ve_dbgfs_t *dbgfs, struct ve_info *vi);
void ve_debug_release(ve_dbgfs_t *dbgfs, struct ve_info *vi);
int ioctl_set_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi);
int ioctl_copy_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi);
int ioctl_stop_proc_info(ve_dbgfs_t *dbgfs, unsigned long arg, struct ve_info *vi);
/*** pltform code new struct definition end    ***/

#endif
