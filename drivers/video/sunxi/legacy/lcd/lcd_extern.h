/* linux/drivers/video/sunxi/lcd/video_source_interface.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Ulti interface for LCD driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_lcd.h"

extern struct sunxi_lcd_drv g_lcd_drv;

__s32 sunxi_lcd_parse_panel_para(struct sunxi_panel *panel, __panel_para_t * info);

void sunxi_lcd_parse_sys_config(struct sunxi_panel *panel, __disp_lcd_cfg_t *lcd_cfg);

__s32 LCD_POWER_EN(__u32 sel, __bool b_en);

__s32 LCD_GPIO_set_attr(disp_gpio_set_t  gpio_info[1], __bool b_output);

__s32 LCD_GPIO_read(disp_gpio_set_t  gpio_info[1]);

__s32 LCD_GPIO_write(disp_gpio_set_t  gpio_info[1], __u32 data);

__s32 LCD_GPIO_init(disp_gpio_set_t  gpio_info[6], __bool lcd_gpio_used[6]);

__s32 LCD_GPIO_exit(disp_gpio_set_t  gpio_info[6]);

__s32 LCD_GPIO_cfg(disp_gpio_set_t gpio_info[], __bool used[], __u32 num, __u32 bon);




