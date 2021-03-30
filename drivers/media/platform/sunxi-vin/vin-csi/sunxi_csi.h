
/*
 ******************************************************************************
 *
 * sunxi_csi.h
 *
 * Hawkview ISP - sunxi_csi.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/02/27	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _SUNXI_CSI_H_
#define _SUNXI_CSI_H_

#include "../platform/platform_cfg.h"

#define VIDIOC_SUNXI_CSI_SET_CORE_CLK 			1
#define VIDIOC_SUNXI_CSI_SET_M_CLK 			2

#define CSI_CORE_CLK_RATE (300*1000*1000)

enum csi_pad {
	CSI_PAD_SINK,
	CSI_PAD_SOURCE,
	CSI_PAD_NUM,
};

enum {
	CSI_CORE_CLK = 0,
	CSI_MASTER_CLK,
	CSI_MISC_CLK,
	CSI_CORE_CLK_SRC,
	CSI_MASTER_CLK_24M_SRC,
	CSI_MASTER_CLK_PLL_SRC,
	CSI_CLK_NUM,
};

#define NOCLK 			0xff

struct csi_pix_format {
	unsigned int pix_width_alignment;
	enum v4l2_mbus_pixelcode code;
	u32 fmt_reg;
	u8 data_alignment;
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
	struct clk *clock[CSI_CLK_NUM];
	struct v4l2_mbus_framefmt format;
	struct csi_pix_format *csi_fmt;

};

void sunxi_csi_set_output_fmt(struct v4l2_subdev *sd, __u32 pixelformat);
void sunxi_csi_dump_regs(struct v4l2_subdev *sd);
struct v4l2_subdev *sunxi_csi_get_subdev(int id);
int sunxi_csi_platform_register(void);
void sunxi_csi_platform_unregister(void);

#endif /*_SUNXI_CSI_H_*/
