
/*
 ******************************************************************************
 *
 * isp_platform_drv.c
 *
 * Hawkview ISP - isp_platform_drv.c module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/06/20	      Second Version
 *
 ******************************************************************************
 */
#include <linux/kernel.h>
#include "isp_platform_drv.h"
#include "sun8iw6p1/sun8iw6p1_isp_reg_cfg.h"
#include "sun8iw12p1/sun8iw12p1_isp_reg_cfg.h"
struct isp_platform_drv *isp_platform_curr;

int isp_platform_register(struct isp_platform_drv *isp_drv)
{
	int platform_id;
	platform_id = isp_drv->platform_id;
	isp_platform_curr = isp_drv;
	return 0;
}

int isp_platform_init(unsigned int platform_id)
{
	printk("[ISP] isp platform_id = %d!\n", platform_id);
	switch (platform_id) {
	case ISP_PLATFORM_SUN8IW6P1:
		sun8iw6p1_isp_platform_register();
		break;

	case ISP_PLATFORM_SUN8IW12P1:
		sun8iw12p1_isp_platform_register();
		break;
	default:
		printk("ISP platform init ERR! No that Platform ID!\n");
		sun8iw12p1_isp_platform_register();
		break;
	}
	return 0;
}

struct isp_platform_drv *isp_get_driver(void)
{
	if (NULL == isp_platform_curr)
		printk("[ISP ERR] isp platform curr have not init!\n");
	return isp_platform_curr;
}


