/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : cdc_log.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2015/04/13
*   Comment :
*
*
*/

#ifndef LOG_H
#define LOG_H

#include <linux/kernel.h>
#include <linux/mutex.h>
#ifndef LOG_TAG
#define LOG_TAG "cedarc"
#endif

enum CDC_LOG_LEVEL_TYPE {
    LOG_LEVEL_VERBOSE = 2,
    LOG_LEVEL_DEBUG = 3,
    LOG_LEVEL_INFO = 4,
    LOG_LEVEL_WARNING = 5,
    LOG_LEVEL_ERROR = 6,
};

extern enum CDC_LOG_LEVEL_TYPE CDC_GLOBAL_LOG_LEVEL;
extern const char *CDC_LOG_LEVEL_NAME[7];

#define CDCLOG(level, fmt, arg...)  \
	do { \
		if (level >= CDC_GLOBAL_LOG_LEVEL) \
			printk("%s: %s <%s:%u>: " fmt "\n", \
				CDC_LOG_LEVEL_NAME[level], LOG_TAG, __FUNCTION__, __LINE__, ##arg); \
	} while (0)

#define loge(fmt, arg...) CDCLOG(LOG_LEVEL_ERROR, "\033[40;31m" fmt "\033[0m", ##arg)
#define logw(fmt, arg...) CDCLOG(LOG_LEVEL_WARNING, fmt, ##arg)
#define logi(fmt, arg...) CDCLOG(LOG_LEVEL_INFO, fmt, ##arg)
#define logd(fmt, arg...) CDCLOG(LOG_LEVEL_DEBUG, fmt, ##arg)
#define logv(fmt, arg...) CDCLOG(LOG_LEVEL_VERBOSE, fmt, ##arg)

#define CEDARC_PRINTF_LINE logd("Run this line")

#define CEDARC_UNUSE(param) ((void)param) //just for remove compile warning

#define CEDARC_DEBUG (0)

#endif
