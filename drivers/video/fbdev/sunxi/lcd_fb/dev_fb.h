/*
 * /home/zhengxiaobin/workspace/tina/lichee/linux-4.9/drivers/video/fbdev/sunxi/lcd_fb/dev_fb/dev_fb.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _DEV_FB_H
#define _DEV_FB_H

#include "dev_lcd_fb.h"

int fb_init(struct dev_lcd_fb_t *p_info);
int fb_exit(void);

#endif /*End of file*/
