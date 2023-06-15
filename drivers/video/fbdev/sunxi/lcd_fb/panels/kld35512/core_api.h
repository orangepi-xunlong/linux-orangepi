/*
 * core_api/core_api.h
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
#ifndef _CORE_API_H
#define _CORE_API_H

#include "../panels.h"

#define RESET(s, v) sunxi_lcd_gpio_set_value(s, 0, v)
#define DC(s, v) sunxi_lcd_gpio_set_value(s, 1, v)

void reset_panel(unsigned int sel);

void init_panel(unsigned int sel);

void display_standardb(unsigned int sel);

void exit_panel(unsigned int sel);
int panel_dma_transfer(unsigned int sel, void *buf, unsigned int len);
int panel_blank(unsigned int sel, unsigned int en);
int panel_set_var(unsigned int sel, struct fb_info *p_info);
void address(unsigned int sel, int x, int y, int width, int height);

#endif /*End of file*/
