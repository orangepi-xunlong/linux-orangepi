/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * drv_edp2.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __DRV_EDP_H__
#define __DRV_EDP_H__

#include "edp_core/edp_core.h"
#include "edp_core/edp_edid.h"
#include "edp_configs.h"
#include "../include/disp_edid.h"
#include "panels/edp_panels.h"
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include <asm-generic/int-ll64.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/compat.h>
#include <uapi/video/sunxi_display2.h>
#include <video/sunxi_edp.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>

#if IS_ENABLED(CONFIG_EXTCON)
#include <linux/extcon.h>
#endif

#define EDP_NUM_MAX 2
#define EDP_POWER_STR_LEN 32

struct edp_debug {
	unsigned long aux_i2c_addr;
	unsigned long aux_i2c_len;
	unsigned long aux_read_start;
	unsigned long aux_read_end;
	u32 aux_write_start;
	u32 aux_write_len;
	u32 aux_write_val[16];
	u32 aux_write_val_before[16];
	u32 lane_debug_en;
	u32 hpd_mask;
	u32 hpd_mask_pre;

	/* resource lock, power&clk won't release when edp enable fail if not 0 */
	u32 edp_res_lock;

	/* bypass training for some signal test case */
	u32 bypass_training;
};

/**
 * save some info here for every edp module
 */
struct edp_info_t {
	u32 enable;
	u32 irq;
	uintptr_t base_addr;
	struct device *dev;
	struct clk *clk_bus;
	struct clk *clk;
	struct clk *clk_24m;
	struct regulator *vdd_regulator;
	struct regulator *vcc_regulator;
	struct reset_control *rst_bus;
	s32 rst_gpio;

	char dpcd_rx_buf[576];
	char dpcd_ext_rx_buf[32];
	bool hpd_state;
	bool hpd_state_now;
	bool suspend;
	bool training_done;
	bool timings_fixed;
	bool edid_timings_prefer;
	bool fps_limit_60;
	bool sink_capacity_prefer;
	bool use_def_timings;
	bool use_edid_timings;

	bool use_debug_para;

	struct mutex mlock;
	struct edp_tx_core edp_core;
	struct edp_tx_cap source_cap;
	struct edp_rx_cap sink_cap;
	struct edp_debug edp_debug;
	struct sunxi_edp_output_desc *desc;
};

enum sunxi_edp_output_type {
	EDP_OUTPUT = 0,
	DP_OUTPUT  = 1,
};
typedef enum sunxi_edp_output_type sunxi_edp_output_type;

struct sunxi_edp_output_desc {
	enum sunxi_edp_output_type type;
	s32 (*init)(struct platform_device *pdev);
	s32 (*exit)(struct platform_device *pdev);
	s32 (*sw_enable_early)(u32 sel);
	s32 (*sw_enable)(u32 sel);
	s32 (*enable_early)(u32 sel);
	s32 (*enable)(u32 sel);
	s32 (*disable)(u32 sel);
	s32 (*plugin)(u32 sel);
	s32 (*plugout)(u32 sel);
	s32 (*runtime_suspend)(u32 sel);
	s32 (*runtime_resume)(u32 sel);
	s32 (*suspend)(u32 sel);
	s32 (*resume)(u32 sel);
};


extern s32 disp_set_edp_func(struct disp_tv_func *func);
extern s32 disp_deliver_edp_dev(struct device *dev);
extern s32 disp_edp_drv_init(int sel);
extern s32 disp_edp_drv_deinit(int sel);
extern unsigned int disp_boot_para_parse(const char *name);
extern bool disp_init_ready(void);

#endif /*End of file*/
