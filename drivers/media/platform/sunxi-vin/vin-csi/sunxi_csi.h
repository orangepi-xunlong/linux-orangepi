/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-csi/sunxi_csi.h
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

/*
 ******************************************************************************
 *
 * sunxi_csi.h
 *
 * Hawkview ISP - sunxi_csi.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version         Author         Date        Description
 *
 *   3.0         Yang Feng     2015/02/27  ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _SUNXI_CSI_H_
#define _SUNXI_CSI_H_

#include "../platform/platform_cfg.h"
#include "parser_reg.h"

enum csi_pad {
	CSI_PAD_SINK,
	CSI_PAD_SOURCE,
	CSI_PAD_NUM,
};

struct csi_format {
	unsigned int wd_align;
	u32 code;
	enum input_seq seq;
	enum prs_input_fmt infmt;
	unsigned int data_width;
};

struct csi_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct media_pad csi_pads[CSI_PAD_NUM];
	struct platform_device *pdev;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	int irq;
	wait_queue_head_t wait;
	void __iomem *base;
	struct bus_info bus_info;
	struct frame_info frame_info;
	struct frame_arrange arrange;
	unsigned int cur_ch;
	unsigned int capture_mode;
	struct list_head csi_list;
	struct pinctrl *pctrl;
	struct v4l2_mbus_framefmt mf;
	struct prs_output_size out_size;
	struct csi_format *csi_fmt;
	struct prs_ncsi_if_cfg ncsi_if;
};

void sunxi_csi_dump_regs(struct v4l2_subdev *sd);
void sunxi_csi_get_input_wh(int id, int *w, int *h);
struct v4l2_subdev *sunxi_csi_get_subdev(int id);
int sunxi_csi_platform_register(void);
void sunxi_csi_platform_unregister(void);

#endif /*_SUNXI_CSI_H_*/
