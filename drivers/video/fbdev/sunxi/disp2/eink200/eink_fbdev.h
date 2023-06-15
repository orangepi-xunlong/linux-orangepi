/*
 * eink_fbdev.h
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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
#ifndef _EINK_FBDEV_H
#define _EINK_FBDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include "include/eink_driver.h"

int eink_fbdev_init(struct device *p_dev);
int eink_fb_exit(void);


#ifdef __cplusplus
}
#endif

#endif /*End of file*/
