
/*
 ******************************************************************************
 *
 * sunxi_cci.h
 *
 * Hawkview ISP - sunxi_cci.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/23	      Second Version
 *
 ******************************************************************************
 */

#ifndef _SUNXI_CCI_H_
#define _SUNXI_CCI_H_

#include "../platform/platform_cfg.h"
struct cci_platform_data {
	unsigned int cci_sel;
};
struct cci_dev {
	unsigned int cci_sel;
	struct platform_device *pdev;
	unsigned int id;
	spinlock_t slock;
	int irq;
	wait_queue_head_t wait;

	void __iomem *base;
};

#endif /*_SUNXI_CCI_H_*/
