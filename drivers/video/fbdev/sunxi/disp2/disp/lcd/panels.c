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
#ifdef CONFIG_EINK_PANEL_USED
	&default_eink,
#endif
	&default_panel,
	&lt070me05000_panel,
	&wtq05027d01_panel,
	&t27p06_panel,
	&dx0960be40a1_panel,
	&tft720x1280_panel,
	&S6D7AA0X01_panel,
	&gg1p4062utsw_panel,
	&ls029b3sx02_panel,
	&he0801a068_panel,
	&inet_dsi_panel,
	&lq101r1sx03_panel,
	/* add new panel below */
	&WilliamLcd_panel,
	&fx070_panel,
	&FX070_DHM18BOEL2_1024X600_mipi_panel,
	&bp101wx1_panel,
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

