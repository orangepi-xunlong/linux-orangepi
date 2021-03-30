
/*
 ******************************************************************************
 *
 * sunxi_mipi.h
 *
 * Hawkview ISP - sunxi_mipi.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Zhao Wei   	2015/02/27	ISP Tuning Tools Support
 *
 ******************************************************************************
 */
#ifndef _SUNXI_MIPI__H_
#define _SUNXI_MIPI__H_

#include "../platform/platform_cfg.h"

enum mipi_pad {
	MIPI_PAD_SINK,
	MIPI_PAD_SOURCE,
	MIPI_PAD_NUM,
};

enum {
	MIPI_CSI_CLK = 0,
	MIPI_DPHY_CLK,
	MIPI_CSI_CLK_SRC,
	MIPI_DPHY_CLK_SRC,
	MIPI_CLK_NUM,
};

#define NOCLK 			0xff

struct mipi_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct media_pad mipi_pads[MIPI_PAD_NUM];
	struct platform_device *pdev;
	struct list_head mipi_list;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	wait_queue_head_t wait;
	void __iomem *base;
	struct mipi_para mipi_para;
	struct mipi_fmt_cfg *mipi_fmt;
	struct v4l2_mbus_framefmt format;
	struct clk *clock[MIPI_CLK_NUM];
};

struct v4l2_subdev *sunxi_mipi_get_subdev(int id);
int sunxi_mipi_platform_register(void);
void sunxi_mipi_platform_unregister(void);

#endif /*_SUNXI_MIPI__H_*/
