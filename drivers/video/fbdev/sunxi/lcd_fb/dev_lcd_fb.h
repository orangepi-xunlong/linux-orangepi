/*
 * drivers/video/fbdev/sunxi/lcd_fb/dev_lcd_fb/dev_lcd_fb.h
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
#ifndef _DEV_LCD_FB_H
#define _DEV_LCD_FB_H
#include "include.h"


struct dev_lcd_fb_t {
	struct device *device;
	struct work_struct start_work;
	unsigned char lcd_fb_num;
};

#endif /*End of file*/
