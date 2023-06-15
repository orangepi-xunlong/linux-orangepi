/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "panels.h"

struct sunxi_lcd_drv g_lcd_drv;

struct __lcd_panel *panel_array[] = {
#ifdef CONFIG_LCD_SUPPORT_KLD35512
	&kld35512_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_KLD39501
	&kld39501_panel,
#endif
#ifdef CONFIG_LCD_SUPPORT_KLD2844B
	&kld2844b_panel,
#endif
	NULL,
};

static void lcd_set_panel_funs(void)
{
	int i;

	for (i = 0; panel_array[i] != NULL; i++) {
		sunxi_lcd_set_panel_funs(panel_array[i]->name,
					 &panel_array[i]->func);
	}
}

int lcd_init(void)
{
	sunxi_disp_get_source_ops(&g_lcd_drv.src_ops);
	lcd_set_panel_funs();

	return 0;
}

