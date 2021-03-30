 /*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*LCD panel ops*/


#include <drm/drmP.h>

#include "../sunxi_drm_drv.h"
#include "../sunxi_drm_core.h"
#include "../sunxi_drm_crtc.h"
#include "../sunxi_drm_encoder.h"
#include "../sunxi_drm_connector.h"
#include "../sunxi_drm_plane.h"
#include "../drm_de/drm_al.h"
#include "../subdev/sunxi_common.h"
#include "../sunxi_drm_panel.h"
#include "sunxi_lcd.h"
#include "de/disp_lcd.h"
#include "de/include.h"

#define DISP_PIN_STATE_ACTIVE "active"
#define DISP_PIN_STATE_SLEEP "sleep"

static struct lcd_clk_info clk_tbl[] = {
	{LCD_IF_HV,     6, 1, 1, 0},
	{LCD_IF_CPU,   12, 1, 1, 0},
	{LCD_IF_LVDS,   7, 1, 1, 0},
	{LCD_IF_DSI,    4, 1, 4, 148500000},
};

/* LCD panel control extern*/

static int sunxi_lcd_extern_backlight_enable(struct sunxi_lcd_private *sunxi_lcd)
{
	disp_gpio_set_t  gpio_info[1];
	disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	if (lcd_cfg->lcd_bl_en_used && sunxi_lcd->bl_enalbe == 0) {

		if (!((!strcmp(lcd_cfg->lcd_bl_en_power, "")) ||
		(!strcmp(lcd_cfg->lcd_bl_en_power, "none")))) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_bl_en_power);
		}
		memcpy(gpio_info, &(lcd_cfg->lcd_bl_en), sizeof(disp_gpio_set_t));
		DRM_DEBUG_KMS("[%d] %s", __LINE__, __func__);

		//lcd_cfg->lcd_bl_gpio_hdl = sunxi_drm_sys_gpio_request(gpio_info);
		sunxi_lcd->bl_enalbe = 1;
	}

	return 0;
}

static int sunxi_lcd_extern_backlight_disable(struct sunxi_lcd_private *sunxi_lcd)
{
	disp_gpio_set_t  gpio_info[1];
	disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	if (lcd_cfg->lcd_bl_en_used && sunxi_lcd->bl_enalbe == 1) {
		memcpy(gpio_info, &(lcd_cfg->lcd_bl_en), sizeof(disp_gpio_set_t));
		gpio_info->data = (gpio_info->data==0)?1:0;
		gpio_info->mul_sel = 7;
		//sunxi_drm_sys_gpio_release(lcd_cfg->lcd_bl_gpio_hdl);
		DRM_DEBUG_KMS("[%d] %s", __LINE__, __func__);

		if (!((!strcmp(lcd_cfg->lcd_bl_en_power, "")) ||
		(!strcmp(lcd_cfg->lcd_bl_en_power, "none")))) {
			sunxi_drm_sys_power_disable(lcd_cfg->lcd_bl_en_power);
		}
		sunxi_lcd->bl_enalbe = 0;
	}

	return 0;
}

void sunxi_lcd_extrn_power_on(struct sunxi_lcd_private *sunxi_lcd)
{
	int i;
	disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;

	if(sunxi_lcd->ex_power_on)
		return;

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (1 == lcd_cfg->lcd_power_used[i]) {
			/* regulator type */
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_power[i]);
		}
	}
	sunxi_lcd->ex_power_on = 1;
}

void sunxi_lcd_extrn_power_off(struct sunxi_lcd_private *sunxi_lcd)
{
	int i;
	disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;
	if(sunxi_lcd->ex_power_on == 0)
		return;

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (1 == lcd_cfg->lcd_power_used[i]) {
			/* regulator type */
			sunxi_drm_sys_power_disable(lcd_cfg->lcd_power[i]);
		}
	}
	sunxi_lcd->ex_power_on = 0;
}

struct lcd_clk_info *sunxi_lcd_clk_info_init(disp_panel_para * panel)
{
	int tcon_div = 6;//tcon inner div
	int lcd_div = 1;//lcd clk div
	int dsi_div = 4;//dsi clk div
	int dsi_rate = 0;
	int i;
	int find = 0;
	struct lcd_clk_info *info;

	if (NULL == panel) {
		DRM_ERROR("give a err sunxi_panel.\n");
		return NULL;
	}

	info = kzalloc(sizeof(struct lcd_clk_info), GFP_KERNEL);
	if (!info) {
		DRM_ERROR("failed to allocate disp_panel_para.\n");
		return NULL;
	}

	for (i = 0; i < sizeof(clk_tbl) / sizeof(struct lcd_clk_info); i++) {
		if (clk_tbl[i].lcd_if == panel->lcd_if) {
			tcon_div = clk_tbl[i].tcon_div;
			lcd_div = clk_tbl[i].lcd_div;
			dsi_div = clk_tbl[i].dsi_div;
			dsi_rate = clk_tbl[i].dsi_rate;
			find = 1;
			break;
		}
	}

	if (panel->lcd_if == LCD_IF_DSI) {
		u32 lane = panel->lcd_dsi_lane;
		u32 bitwidth = 0;

		switch (panel->lcd_dsi_format) {
		case LCD_DSI_FORMAT_RGB888:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB666:
			bitwidth = 24;
			break;
		case LCD_DSI_FORMAT_RGB565:
			bitwidth = 16;
			break;
		case LCD_DSI_FORMAT_RGB666P:
			bitwidth = 18;
			break;
		}

		dsi_div = bitwidth / lane;
	}

	if (0 == find) {
		DRM_ERROR("can not find the lcd_clk_info, use default.\n");
	}

	info->tcon_div = tcon_div;
	info->lcd_div = lcd_div;
	info->dsi_div = dsi_div;
	info->dsi_rate = dsi_rate;

	return info;
}

static bool default_panel_init(struct sunxi_panel *sunxi_panel)
{
	u32 i = 0, j = 0;
	u32 items;

	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;
	disp_lcd_cfg *lcd_cfg = sunxi_lcd->lcd_cfg;
	panel_extend_para *info = sunxi_lcd->extend_panel;
	disp_panel_para *panel = sunxi_lcd->panel;
	u32 lcd_bright_curve_tbl[101][2];
	/*init gamma*/

	u8 lcd_gamma_tbl[][2] = {
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};
	u32 lcd_cmap_tbl[2][3][4] = {
		{
			{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
			{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
			{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
			{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
			{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
			{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};
	lcd_cfg->backlight_dimming = 0;
	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}

	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) +
				(lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];
	/* init cmap */

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

	/*init the bright */
	for (i = 0, items = 0; i < 101; i++) {
		if (lcd_cfg->backlight_curve_adjust[i] == 0) {
			if (i == 0) {
				lcd_bright_curve_tbl[items][0] = 0;
				lcd_bright_curve_tbl[items][1] = 0;
				items++;
			}
		} else {
			lcd_bright_curve_tbl[items][0] = 255 * i / 100;
			lcd_bright_curve_tbl[items][1] = lcd_cfg->backlight_curve_adjust[i];
			items++;
		}
	}

	for (i = 0; i < items-1; i++) {
		u32 num = lcd_bright_curve_tbl[i+1][0] - lcd_bright_curve_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value = lcd_bright_curve_tbl[i][1] + ((lcd_bright_curve_tbl[i+1][1] -
			lcd_bright_curve_tbl[i][1]) * j)/num;
			info->lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] + j] = value;
		}
	}

	info->lcd_bright_curve_tbl[255] = lcd_bright_curve_tbl[items-1][1];

	sunxi_lcd->clk_info = sunxi_lcd_clk_info_init(panel);
	if (!sunxi_lcd->clk_info) {
		DRM_ERROR("failed to init clk_info.\n");
		return false;
	}
	sunxi_panel->clk_rate = panel->lcd_dclk_freq * 1000000;
	return true;
}

bool default_panel_open(struct sunxi_panel *sunxi_panel)
{
	sunxi_lcd_extrn_power_on(sunxi_panel->private);
	sunxi_drm_delayed_ms(80);
	sunxi_chain_enable(sunxi_panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(100);
	return true;
}

bool default_panel_close(struct sunxi_panel *panel)
{
	sunxi_chain_disable(panel->drm_connector, CHAIN_BIT_ENCODER);
	sunxi_drm_delayed_ms(200);
	sunxi_lcd_extrn_power_off(panel->private);
	sunxi_drm_delayed_ms(500);
	return true;
}

bool sunxi_lcd_default_set_bright(struct sunxi_panel *sunxi_panel, unsigned int bright)
{
	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;

	if (bright > 0) {
		sunxi_drm_sys_pwm_enable(sunxi_lcd->pwm_info);
	} else {
		sunxi_drm_sys_pwm_disable(sunxi_lcd->pwm_info);
	}
	sunxi_common_pwm_set_bl(sunxi_lcd, bright);
	/* to add spec bright for spec lcd*/
	if (bright > 0) {
		sunxi_lcd_extern_backlight_enable(sunxi_lcd);
	} else {
		sunxi_lcd_extern_backlight_disable(sunxi_lcd);
	}
	return true;
}

bool cmp_panel_with_display_mode(struct drm_display_mode *mode,
			disp_panel_para       *panel)
{
	if((mode->clock == panel->lcd_dclk_freq * 1000) &&
			(mode->hdisplay == panel->lcd_x) &&
		(mode->hsync_start == (panel->lcd_ht - panel->lcd_hbp)) &&
		(mode->hsync_end == (panel->lcd_ht - panel->lcd_hbp + panel->lcd_hspw)) &&
		(mode->htotal == panel->lcd_ht) &&
		(mode->vdisplay == panel->lcd_y) &&
		(mode->vsync_start == (panel->lcd_vt - panel->lcd_vbp)) &&
		(mode->vsync_end == (panel->lcd_vt - panel->lcd_vbp + panel->lcd_vspw)) &&
		(mode->vtotal == panel->lcd_vt) &&
		(mode->vscan == 0) &&
		(mode->flags == 0) &&
		(mode->width_mm == panel->lcd_width) &&
		(mode->height_mm == panel->lcd_height) &&
		(mode->vrefresh == (mode->clock * 1000 / mode->vtotal / mode->htotal)))
		return true;
	return false;
}

enum drm_mode_status
	sunxi_lcd_valid_mode(struct sunxi_panel *panel, struct drm_display_mode *mode)
{
	enum drm_mode_status status = MODE_BAD;
	struct sunxi_lcd_private *sunxi_lcd_p = (struct sunxi_lcd_private *)panel->private;

	if (cmp_panel_with_display_mode(mode, sunxi_lcd_p->panel))
		status = MODE_OK;

	return status;
}

static inline void convert_panel_to_display_mode(struct drm_display_mode *mode,
			disp_panel_para  *panel)
{
	mode->clock       = panel->lcd_dclk_freq * 1000;

	mode->hdisplay    = panel->lcd_x;
	mode->hsync_start = panel->lcd_ht - panel->lcd_hbp;
	mode->hsync_end   = panel->lcd_ht - panel->lcd_hbp + panel->lcd_hspw;
	mode->htotal      = panel->lcd_ht;

	mode->vdisplay    = panel->lcd_y;
	mode->vsync_start = panel->lcd_vt - panel->lcd_vbp;
	mode->vsync_end   = panel->lcd_vt - panel->lcd_vbp + panel->lcd_vspw;
	mode->vtotal      = panel->lcd_vt;
	mode->vscan       = 0;
	mode->flags       = 0;
	mode->width_mm    = panel->lcd_width;
	mode->height_mm   = panel->lcd_height;
	mode->vrefresh    = mode->clock * 1000 / mode->vtotal / mode->htotal;
	DRM_DEBUG_KMS("Modeline %d: %d %d %d %d %d %d %d %d %d %d "
		"0x%x 0x%x\n",
		mode->base.id, mode->vrefresh, mode->clock,
		mode->hdisplay, mode->hsync_start,
		mode->hsync_end, mode->htotal,
		mode->vdisplay, mode->vsync_start,
		mode->vsync_end, mode->vtotal, mode->type, mode->flags);
	DRM_DEBUG_KMS("panel: clk[%d] [x %d, ht %d, hbp %d, hspw %d] [y%d, vt%d, bp %d, pw %d,] "
		"%d x %d\n",
		panel->lcd_dclk_freq * 1000,
		panel->lcd_x, panel->lcd_ht,
		panel->lcd_hbp,panel->lcd_hspw,
		panel->lcd_y, panel->lcd_vt,
		panel->lcd_vbp, panel->lcd_vspw,panel->lcd_width, panel->lcd_height);
}

unsigned int sunxi_lcd_get_modes(struct sunxi_panel *sunxi_panel)
{
	disp_panel_para   *panel;
	struct drm_display_mode *mode;
	struct sunxi_lcd_private *sunxi_lcd_p = (struct sunxi_lcd_private *)sunxi_panel->private;
	struct drm_connector *connector = sunxi_panel->drm_connector;
	panel = sunxi_lcd_p->panel;;

	mode = drm_mode_create(connector->dev);
	if (!mode) {
		DRM_ERROR("failed to create a new display mode.\n");
		return 0;
	}

	convert_panel_to_display_mode(mode, panel);
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	return 1;
}

enum drm_connector_status
	sunxi_common_detect(struct sunxi_panel *panel)
{
	return connector_status_connected;
}

static struct  panel_ops  default_panel = {
	.init = default_panel_init,
	.open = default_panel_open,
	.close = default_panel_close,
	.reset = NULL,
	.bright_light = sunxi_lcd_default_set_bright,
	.detect  = sunxi_common_detect,
	.change_mode_to_timming = NULL,
	.check_valid_mode = sunxi_lcd_valid_mode,
	.get_modes = sunxi_lcd_get_modes,
};

int sunxi_get_lcd_sys_info(struct sunxi_lcd_private *sunxi_lcd)
{
	disp_gpio_set_t  *gpio_info;
	char primary_key[20], sub_name[25];
	int i = 0, ret = -EINVAL, value = 1;
	disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;
	disp_panel_para  *info = sunxi_lcd->panel;
	char compat[32];
	struct device_node *node;

	sprintf(primary_key, "sunxi-lcd%d", sunxi_lcd->lcd_id);
	node = sunxi_drm_get_name_node(primary_key);
	if (!node) {
		DRM_ERROR("of_find_compatible_node %s fail\n", compat);
		return -EINVAL;
	}
	/* lcd_used */
	ret = sunxi_drm_get_sys_item_int(node, "lcd_used", &value);
	if (ret == 0) {
		lcd_cfg->lcd_used = value;
	}

	if (lcd_cfg->lcd_used == 0) {
		return -EINVAL;
	}

	/* lcd_bl_en */
	lcd_cfg->lcd_bl_en_used = 0;
	gpio_info = &(lcd_cfg->lcd_bl_en);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_bl_en", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_bl_en_used = 1;
	}

	sprintf(sub_name, "lcd_bl_en_power");
	ret = sunxi_drm_get_sys_item_char(node, sub_name, lcd_cfg->lcd_bl_en_power);

	/* lcd fix power */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0) {
			sprintf(sub_name, "lcd_fix_power");
		}else {
			sprintf(sub_name, "lcd_fix_power%d", i);
		}

		lcd_cfg->lcd_power_used[i] = 0;
		ret = sunxi_drm_get_sys_item_char(node, sub_name, lcd_cfg->lcd_fix_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_fix_power_used[i] = 1;
		}
	}

	/* lcd_power */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0) {
			sprintf(sub_name, "lcd_power");
		} else {
			sprintf(sub_name, "lcd_power%d", i);
		}
		lcd_cfg->lcd_power_used[i] = 0;
		ret = sunxi_drm_get_sys_item_char(node, sub_name,lcd_cfg->lcd_power[i]);
		if (ret == 0) {
			lcd_cfg->lcd_power_used[i] = 1;
		}
	}

	/* lcd_gpio */
	for (i = 0; i < LCD_GPIO_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_%d", i);
		gpio_info = &(lcd_cfg->lcd_gpio[i]);
		ret = sunxi_drm_get_sys_item_gpio(node, sub_name, gpio_info);
		if (ret == 0) {
			lcd_cfg->lcd_gpio_used[i] = 1;
		}
	}

	/*lcd_gpio_scl,lcd_gpio_sda*/
	gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_gpio_scl", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL] = 1;
	}

	gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
	ret = sunxi_drm_get_sys_item_gpio(node, "lcd_gpio_sda", gpio_info);
	if (ret == 0) {
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA] = 1;
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_power%d", i);
		ret = sunxi_drm_get_sys_item_char(node, sub_name, lcd_cfg->lcd_gpio_power[i]);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (0 == i) {
			sprintf(sub_name, "lcd_pin_power");
		} else {
			sprintf(sub_name, "lcd_pin_power%d", i);
		}
		ret = sunxi_drm_get_sys_item_char(node, sub_name, lcd_cfg->lcd_pin_power[i]);
	}

	/* backlight adjust */
	for (i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if (i == 100) {
			lcd_cfg->backlight_curve_adjust[i] = 255;
		}

		ret = sunxi_drm_get_sys_item_int(node, sub_name, &value);
		if (ret == 0) {
			value = (value > 100)? 100:value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}

	sprintf(sub_name, "lcd_backlight");
	ret = sunxi_drm_get_sys_item_int(node, sub_name, &value);
	if (ret == 0) {
		value = (value > 256)? 256:value;
		lcd_cfg->backlight_bright = value;
	} else {
		lcd_cfg->backlight_bright = 197;
	}
	lcd_cfg->lcd_bright = lcd_cfg->backlight_bright;
	/* display mode */
	ret = sunxi_drm_get_sys_item_int(node, "lcd_x", &value);
	if (ret == 0) {
		info->lcd_x = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_y", &value);
	if (ret == 0) {
		info->lcd_y = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_width", &value);
	if (ret == 0) {
		info->lcd_width = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_height", &value);
	if (ret == 0) {
		info->lcd_height = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dclk_freq", &value);
	if (ret == 0) {
		info->lcd_dclk_freq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_used", &value);
	if (ret == 0) {
		info->lcd_pwm_used = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_ch", &value);
	if (ret == 0) {
		info->lcd_pwm_ch = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_freq", &value);
	if (ret == 0) {
		info->lcd_pwm_freq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_pwm_pol", &value);
	if (ret == 0) {
		info->lcd_pwm_pol = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_if", &value);
	if (ret == 0) {
	info->lcd_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hbp", &value);
	if (ret == 0) {
		info->lcd_hbp = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_ht", &value);
	if (ret == 0) {
		info->lcd_ht = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_vbp", &value);
	if (ret == 0) {
		info->lcd_vbp = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_vt", &value);
	if (ret == 0) {
		info->lcd_vt = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_if", &value);
	if (ret == 0) {
		info->lcd_hv_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_vspw", &value);
	if (ret == 0) {
		info->lcd_vspw = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hspw", &value);
	if (ret == 0) {
		info->lcd_hspw = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_if", &value);
	if (ret == 0) {
		info->lcd_lvds_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_mode", &value);
	if (ret == 0) {
		info->lcd_lvds_mode = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_colordepth", &value);
	if (ret == 0) {
		info->lcd_lvds_colordepth= value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_lvds_io_polarity", &value);
	if (ret == 0) {
		info->lcd_lvds_io_polarity = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_cpu_if", &value);
	if (ret == 0) {
		info->lcd_cpu_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_cpu_te", &value);
	if (ret == 0) {
		info->lcd_cpu_te = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_frm", &value);
	if (ret == 0) {
		info->lcd_frm = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_if", &value);
	if (ret == 0) {
		info->lcd_dsi_if = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_lane", &value);
	if (ret == 0) {
		info->lcd_dsi_lane = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_format", &value);
	if (ret == 0) {
		info->lcd_dsi_format = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_eotp", &value);
	if (ret == 0) {
		info->lcd_dsi_eotp = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_dsi_te", &value);
	if (ret == 0) {
		info->lcd_dsi_te = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_rate", &value);
	if (ret == 0) {
		info->lcd_edp_rate = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_lane", &value);
	if (ret == 0) {
		info->lcd_edp_lane= value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_colordepth", &value);
	if (ret == 0) {
		info->lcd_edp_colordepth = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_edp_fps", &value);
	if (ret == 0) {
		info->lcd_edp_fps = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_clk_phase", &value);
	if (ret == 0) {
		info->lcd_hv_clk_phase = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_sync_polarity", &value);
	if (ret == 0) {
		info->lcd_hv_sync_polarity = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_srgb_seq", &value);
	if (ret == 0) {
		info->lcd_hv_srgb_seq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_rb_swap", &value);
	if (ret == 0) {
		info->lcd_rb_swap = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_syuv_seq", &value);
	if (ret == 0) {
		info->lcd_hv_syuv_seq = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_hv_syuv_fdly", &value);
	if (ret == 0) {
		info->lcd_hv_syuv_fdly = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_gamma_en", &value);
	if (ret == 0) {
		info->lcd_gamma_en = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_cmap_en", &value);
	if (ret == 0) {
		info->lcd_cmap_en = value;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_xtal_freq", &value);
	if (ret == 0) {
		info->lcd_xtal_freq = value;
	}

	ret = sunxi_drm_get_sys_item_char(node, "lcd_size", (void*)info->lcd_size);
	ret = sunxi_drm_get_sys_item_char(node, "lcd_model_name", (void*)info->lcd_model_name);

	return 0;
}

int sunxi_lcd_gpio_init(disp_lcd_cfg *lcd_cfg)
{
	int i;
	/* for use the gpio ctl the lcd panel,
	* don't contain the lcd IO pin. 
	*/
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcd_cfg->lcd_gpio_power[i], "")) ||
		(!strcmp(lcd_cfg->lcd_gpio_power[i], "none")))) {

			sunxi_drm_sys_power_enable(lcd_cfg->lcd_gpio_power[i]);
		}
	}

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		lcd_cfg->gpio_hdl[i] = 0;

		if (lcd_cfg->lcd_gpio_used[i]) {
			disp_gpio_set_t  gpio_info[1];

			memcpy(gpio_info, &(lcd_cfg->lcd_gpio[i]), sizeof(disp_gpio_set_t));
			lcd_cfg->gpio_hdl[i] = sunxi_drm_sys_gpio_request(gpio_info);
		}
	}

	return 0;
}

static int sunxi_lcd_pin_enalbe(struct sunxi_lcd_private *sunxi_lcd)
{
	int  i;
	char dev_name[25];
	disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcd_cfg->lcd_pin_power[i], "")) ||
		(!strcmp(lcd_cfg->lcd_pin_power[i], "none")))) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_pin_power[i]);
		}
	}

	sprintf(dev_name, "lcd%d", sunxi_lcd->lcd_id);
	sunxi_drm_sys_pin_set_state(dev_name, DISP_PIN_STATE_ACTIVE);

	return 0;
}

static int sunxi_lcd_pin_disable(struct sunxi_lcd_private *sunxi_lcd)
{
	int  i;
	char dev_name[25];
	disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	sprintf(dev_name, "lcd%d", sunxi_lcd->lcd_id);
	sunxi_drm_sys_pin_set_state(dev_name, DISP_PIN_STATE_SLEEP);

	if (LCD_IF_DSI ==  sunxi_lcd->panel->lcd_if) {
#ifdef SUPPORT_DSI
		dsi_io_close(sunxi_lcd->lcd_id);
#endif
	}

	for (i = LCD_GPIO_REGU_NUM-1; i >= 0; i--) {
		if (!((!strcmp(lcd_cfg->lcd_pin_power[i], "")) ||
			(!strcmp(lcd_cfg->lcd_pin_power[i], "none")))){

			sunxi_drm_sys_power_disable(lcd_cfg->lcd_pin_power[i]);
		}
	}

	return 0;
}

int sunxi_pwm_dev_init(struct sunxi_lcd_private *sunxi_lcd)
{
	__u64 backlight_bright;
	__u64 period_ns, duty_ns;
	disp_panel_para  *panel_info = sunxi_lcd->panel;
	struct pwm_info_t  *pwm_info;

	if (panel_info->lcd_pwm_used) {
		pwm_info = kzalloc(sizeof(struct pwm_info_t), GFP_KERNEL);
		if (!pwm_info) {
			DRM_ERROR("failed to alloc pwm_info.\n");
			return -ENOMEM;
		}

		pwm_info->channel = panel_info->lcd_pwm_ch;
		pwm_info->polarity = panel_info->lcd_pwm_pol;
		pwm_info->pwm_dev = pwm_request(panel_info->lcd_pwm_ch, "lcd");

		if (!pwm_info->pwm_dev) {
			DRM_ERROR("failed to pwm_request pwm_dev.\n");
			kfree(pwm_info);
			return -ENOMEM;
		}

		if (panel_info->lcd_pwm_freq != 0) {
			period_ns = 1000*1000*1000 / panel_info->lcd_pwm_freq;
		} else {
			DRM_INFO("lcd pwm use default Hz.\n");
			period_ns = 1000*1000*1000 / 1000;  //default 1khz
		}

		backlight_bright = sunxi_lcd->lcd_cfg->backlight_bright;

		duty_ns = (backlight_bright * period_ns) / 256;
		pwm_set_polarity(pwm_info->pwm_dev, pwm_info->polarity);
		pwm_info->duty_ns = duty_ns;
		pwm_info->period_ns = period_ns;
		sunxi_lcd->pwm_info = pwm_info;
		DRM_DEBUG_KMS("[%d] %s %d", __LINE__, __func__,pwm_info->polarity);
		DRM_DEBUG_KMS("duty_ns:%lld period_ns:%lld\n", duty_ns, period_ns);
	}

	return 0;
}

void sunxi_pwm_dev_destroy(struct pwm_info_t  *pwm_info)
{
	if (pwm_info) {
		if (pwm_info->pwm_dev)
			pwm_free(pwm_info->pwm_dev);
		kfree(pwm_info);
	}
}

int sunxi_common_pwm_set_bl(struct sunxi_lcd_private *sunxi_lcd, unsigned int bright)
{
	u32 duty_ns;
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	panel_extend_para *extend_panel;
	disp_lcd_cfg  *lcd_cfg = sunxi_lcd->lcd_cfg;

	extend_panel = sunxi_lcd->extend_panel;

	if (sunxi_lcd->pwm_info->pwm_dev && extend_panel != NULL) {
		if (backlight_bright != 0) {
			backlight_bright += 1;
		}
		DRM_DEBUG_KMS("[%d] %s light:%d", __LINE__, __func__, bright);
		bright = (bright > 255)? 255:bright;
		backlight_bright = extend_panel->lcd_bright_curve_tbl[bright];

		sunxi_lcd->lcd_cfg->backlight_dimming =
			(0 == sunxi_lcd->lcd_cfg->backlight_dimming)? 256:sunxi_lcd->lcd_cfg->backlight_dimming;
		backlight_dimming = sunxi_lcd->lcd_cfg->backlight_dimming;
		period_ns = sunxi_lcd->pwm_info->period_ns;
		duty_ns = (backlight_bright * backlight_dimming *  period_ns/256 + 128) / 256;
		sunxi_lcd->pwm_info->duty_ns = duty_ns;
		lcd_cfg->backlight_bright = bright;
		DRM_DEBUG_KMS("bright:%d duty_ns:%d period_ns:%llu\n", bright, duty_ns, period_ns);

		sunxi_drm_sys_pwm_config(sunxi_lcd->pwm_info->pwm_dev, duty_ns, period_ns);
	}
	return 0;
}

int sunxi_lcd_panel_ops_init(struct sunxi_panel *sunxi_panel)
{
	char primary_key[20];
	int  ret;
	char drv_name[20];
	struct device_node *node;
	struct sunxi_lcd_private *sunxi_lcd = sunxi_panel->private;

	sprintf(primary_key, "sunxi-lcd%d", sunxi_lcd->lcd_id);
	node = sunxi_drm_get_name_node(primary_key);

	ret = sunxi_drm_get_sys_item_char(node, "lcd_driver_name", drv_name);
	if (!strcmp(drv_name, "default_lcd")) {
		sunxi_panel->panel_ops = &default_panel;
		sunxi_panel->panel_ops->init(sunxi_panel);
		return 0;
	}

	DRM_ERROR("failed to init sunxi_panel.\n");
	return -EINVAL;
}

void sunxi_lcd_panel_ops_destroy(struct panel_ops  *panel_ops)
{
	return;
}

bool sunxi_fix_power_enable(disp_lcd_cfg  *lcd_cfg)
{
	int i;
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (1 == lcd_cfg->lcd_fix_power_used[i]) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_fix_power[i]);
		}
	}
	return true;
}

bool sunxi_fix_power_disable(disp_lcd_cfg  *lcd_cfg)
{
	int i;
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (1 == lcd_cfg->lcd_fix_power_used[i]) {
			sunxi_drm_sys_power_disable(lcd_cfg->lcd_fix_power[i]);
		}
	}
	return true;
}
static int sunxi_gpio_request(disp_lcd_cfg  *lcd_cfg)
{
	int i;
	disp_gpio_set_t  gpio_info[1];
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (!((!strcmp(lcd_cfg->lcd_gpio_power[i], "")) ||
			(!strcmp(lcd_cfg->lcd_gpio_power[i], "none")))) {
			sunxi_drm_sys_power_enable(lcd_cfg->lcd_gpio_power[i]);
		}
	}

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		lcd_cfg->gpio_hdl[i] = 0;

		if (lcd_cfg->lcd_gpio_used[i]) {
			memcpy(gpio_info, &(lcd_cfg->lcd_gpio[i]), sizeof(disp_gpio_set_t));
			lcd_cfg->gpio_hdl[i] = sunxi_drm_sys_gpio_request(gpio_info);
		}
	}

	return 0;
}

static int sunxi_gpio_release(disp_lcd_cfg  *lcd_cfg)
{
	int i;
	for (i = 0; i < LCD_GPIO_NUM; i++) {
		if (lcd_cfg->gpio_hdl[i]) {
			disp_gpio_set_t  gpio_info[1];

			sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);

			memcpy(gpio_info, &(lcd_cfg->lcd_gpio[i]), sizeof(disp_gpio_set_t));
			gpio_info->mul_sel = 7;
			lcd_cfg->gpio_hdl[i] = sunxi_drm_sys_gpio_request(gpio_info);
			sunxi_drm_sys_gpio_release(lcd_cfg->gpio_hdl[i]);
			lcd_cfg->gpio_hdl[i] = 0;
		}
	}

	for (i = LCD_GPIO_REGU_NUM-1; i >= 0; i--) {
		if (!((!strcmp(lcd_cfg->lcd_gpio_power[i], "")) ||
			(!strcmp(lcd_cfg->lcd_gpio_power[i], "none")))) {
			sunxi_drm_sys_power_disable(lcd_cfg->lcd_gpio_power[i]);
		}
	}

	return 0;
}

bool sunxi_common_enable(void *data)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_panel *sunxi_panel;
	struct panel_ops *panel_ops;
	disp_lcd_cfg   *lcd_cfg;
	struct sunxi_lcd_private *sunxi_lcd;

	sunxi_connector = to_sunxi_connector(data);
	hw_res = sunxi_connector->hw_res;
	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = sunxi_panel->private;
	panel_ops = sunxi_panel->panel_ops;
	lcd_cfg = sunxi_lcd->lcd_cfg;

	sunxi_fix_power_enable(lcd_cfg);
	sunxi_gpio_request(lcd_cfg);
	sunxi_lcd_pin_enalbe(sunxi_lcd);
	panel_ops->open(sunxi_panel);

	panel_ops->bright_light(sunxi_panel, lcd_cfg->lcd_bright);
	return true;
}

bool sunxi_common_disable(void *data)
{
	struct sunxi_drm_connector *sunxi_connector;
	struct sunxi_hardware_res *hw_res;
	struct sunxi_panel *sunxi_panel;
	struct panel_ops *panel_ops;
	disp_lcd_cfg      *lcd_cfg;
	struct sunxi_lcd_private *sunxi_lcd;

	sunxi_connector = to_sunxi_connector(data);
	hw_res = sunxi_connector->hw_res;
	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = sunxi_panel->private;
	panel_ops = sunxi_panel->panel_ops;
	lcd_cfg = sunxi_lcd->lcd_cfg;

	sunxi_irq_free(hw_res);

	sunxi_lcd_extern_backlight_disable(sunxi_lcd);
	sunxi_drm_sys_pwm_disable(sunxi_lcd->pwm_info);
	panel_ops->close(sunxi_panel);
	sunxi_lcd_pin_disable(sunxi_lcd);
	sunxi_clk_disable(hw_res);
	sunxi_gpio_release(lcd_cfg);
	sunxi_fix_power_disable(lcd_cfg);
	return true;
}

#ifdef SUPPORT_DSI
bool sunxi_dsi_irq_query(void *data, int need_irq)
{
#ifdef CONFIG_ARCH_SUN8IW11
	enum __dsi_irq_id_t id;
#endif
#ifdef CONFIG_ARCH_SUN50IW1P1
	__dsi_irq_id_t id;
#endif

	switch (need_irq) {
	case QUERY_VSYNC_IRQ:
		id = DSI_IRQ_VIDEO_VBLK;
		break;
	default:
		return false;
	}
	/* bug for sunxi_connector->connector_id */
	if(dsi_irq_query(0, id))
		return true;

	return false;
}

bool sunxi_dsi_init(void *data)
{
	struct sunxi_panel *sunxi_panel;
	struct sunxi_lcd_private *sunxi_lcd;
	struct sunxi_drm_connector *sunxi_connector =
	to_sunxi_connector(data);

	sunxi_panel = sunxi_connector->panel;
	sunxi_lcd = (struct sunxi_lcd_private *)sunxi_panel->private;
	sunxi_connector->hw_res->clk_rate = sunxi_panel->clk_rate 
				* sunxi_lcd->clk_info->lcd_div;

	return true;
}

#endif

bool sunxi_lvds_init(void *data)
{
	return true;
}

bool sunxi_lvds_reset(void *data)
{
	struct drm_connector *connector = (struct drm_connector *)data;
	struct sunxi_drm_connector *sunxi_connector =
			to_sunxi_connector(data);
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_crtc *sunxi_crtc;
	if (!connector || !connector->encoder ||
		!connector->encoder->crtc) {
		return false;
	}
	sunxi_encoder = to_sunxi_encoder(connector->encoder);
	sunxi_crtc = to_sunxi_crtc(connector->encoder->crtc);
	sunxi_clk_enable(sunxi_connector->hw_res);

	return true;
}


#ifdef SUPPORT_DSI
bool sunxi_dsi_reset(void *data)
{
	struct drm_connector *connector = (struct drm_connector *)data;
	struct sunxi_drm_connector *sunxi_connector =
	to_sunxi_connector(data);
	struct sunxi_drm_encoder *sunxi_encoder;
	struct sunxi_drm_crtc *sunxi_crtc;
	struct sunxi_lcd_private *sunxi_lcd;

	if (!connector || !connector->encoder ||
		!connector->encoder->crtc) {
		return false;
	}
	sunxi_lcd = (struct sunxi_lcd_private *)sunxi_connector->panel->private;
	sunxi_encoder = to_sunxi_encoder(connector->encoder);
	sunxi_crtc = to_sunxi_crtc(connector->encoder->crtc);
	sunxi_clk_set(sunxi_connector->hw_res);
	sunxi_irq_free(sunxi_crtc->hw_res);
	sunxi_irq_free(sunxi_encoder->hw_res);
	sunxi_irq_request(sunxi_connector->hw_res);
	/* after enable the tcon ,THK ok?*/
	sunxi_clk_enable(sunxi_connector->hw_res);
	dsi_cfg(sunxi_lcd->lcd_id, sunxi_lcd->panel);
	return true;
}

static struct sunxi_hardware_ops dsi_con_ops = {
	.init = sunxi_dsi_init,
	.reset = sunxi_dsi_reset,
	.enable = sunxi_common_enable,
	.disable = sunxi_common_disable,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.irq_query = sunxi_dsi_irq_query,
	.vsync_delayed_do = NULL,
	.set_timming = NULL,
};
#endif

static struct sunxi_hardware_ops edp_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops lvds_con_ops = {
	.init = sunxi_lvds_init,
	.reset = sunxi_lvds_reset,
	.enable = sunxi_common_enable,
	.disable = sunxi_common_disable,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.irq_query = NULL, 
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops hv_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops cpu_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static struct sunxi_hardware_ops edsi_con_ops = {
	.reset = NULL,
	.enable = NULL,
	.disable = NULL,
	.updata_reg = NULL,
	.vsync_proc = NULL,
	.vsync_delayed_do = NULL,
};

static int sunxi_lcd_hwres_ops_init(struct sunxi_hardware_res *hw_res, int type)
{
	switch (type) {
	case LCD_IF_HV:
		hw_res->ops = &hv_con_ops;
		break;
	case LCD_IF_CPU:
		hw_res->ops = &cpu_con_ops;
		break;
	case LCD_IF_LVDS:
		hw_res->ops = &lvds_con_ops;
		break;
#ifdef SUPPORT_DSI
	case LCD_IF_DSI:
		hw_res->ops = &dsi_con_ops;
		break;
#endif
	case LCD_IF_EDP:
		hw_res->ops = &edp_con_ops;
		break;
	case LCD_IF_EXT_DSI:
		hw_res->ops = &edsi_con_ops;
		break;
	default:
		DRM_ERROR("give us an err hw_res.\n");
		return -EINVAL;
	}
	return 0;
}

static void sunxi_lcd_hwres_ops_destroy(struct sunxi_hardware_res *hw_res)
{
	hw_res->ops = NULL;
}

bool sunxi_cmp_panel_type(disp_mod_id res_id, int type)
{
	/* TODO for  DISP_MOD_EINK HDMI */
	/* 0:hv(sync+de); 1:8080; 2:ttl; 3:lvds; 4:dsi; 5:edp */
	switch ((int)(res_id)) {
	case DISP_MOD_DSI0:
		if (type == 4)
			return true;
		break;
	case DISP_MOD_DSI1:
		if (type == 4)
			return true;
		break;
		/* for :0:hv(sync+de); 1:8080; 2:ttl; */
	case DISP_MOD_DSI2:
		if (type < 3)
			return true;
		break;
	case DISP_MOD_HDMI:
		if (type == 7)
			return true;
		break;
	case DISP_MOD_LVDS:
		if (type == 3)
			return true;
		break;

	default:
		DRM_ERROR("[%s][%s]get a err lcd type.\n", __FILE__,__func__);
	}

	return false;
}

int sunxi_lcd_private_init(struct sunxi_panel *sunxi_panel, int lcd_id)
{
	struct sunxi_lcd_private *sunxi_lcd_p;
	sunxi_lcd_p = kzalloc(sizeof( struct sunxi_lcd_private), GFP_KERNEL);
	if (!sunxi_lcd_p) {
		DRM_ERROR("failed to allocate sunxi_lcd_p.\n");
		return -EINVAL;
	}
	sunxi_panel->private = sunxi_lcd_p;

	sunxi_lcd_p->lcd_cfg = kzalloc(sizeof(disp_lcd_cfg), GFP_KERNEL);
	if(!sunxi_lcd_p->lcd_cfg) {
		DRM_ERROR("failed to allocate lcd_cfg.\n"); 
		goto lcd_err;
	}

	sunxi_lcd_p->extend_panel = kzalloc(sizeof(panel_extend_para), GFP_KERNEL);
	if (!sunxi_lcd_p->extend_panel) {
		DRM_ERROR("failed to allocate extend_panel.\n"); 
		goto lcd_err;
	}

	sunxi_lcd_p->panel = kzalloc(sizeof(disp_panel_para), GFP_KERNEL);
	if (!sunxi_lcd_p->panel) {
		DRM_ERROR("failed to allocate panel.\n"); 
		goto lcd_err;
	}

	sunxi_lcd_p->lcd_id = lcd_id;
	sunxi_get_lcd_sys_info(sunxi_lcd_p);
	if(sunxi_pwm_dev_init(sunxi_lcd_p))
		goto lcd_err;

	return 0;

lcd_err:
	if (sunxi_lcd_p->panel)
		kfree(sunxi_lcd_p->panel);
	if (sunxi_lcd_p->extend_panel)
		kfree(sunxi_lcd_p->extend_panel);
	if (sunxi_lcd_p->lcd_cfg)
		kfree(sunxi_lcd_p->lcd_cfg);
	return -EINVAL;
}

void sunxi_lcd_private_destroy(struct sunxi_lcd_private *sunxi_lcd_p)
{
	if (sunxi_lcd_p->panel)
		kfree(sunxi_lcd_p->panel);
	if (sunxi_lcd_p->extend_panel);
		kfree(sunxi_lcd_p->extend_panel);
	if (sunxi_lcd_p->lcd_cfg)
		kfree(sunxi_lcd_p->lcd_cfg);
	sunxi_pwm_dev_destroy(sunxi_lcd_p->pwm_info);
	kfree(sunxi_lcd_p);
}

struct sunxi_panel *sunxi_lcd_init(struct sunxi_hardware_res *hw_res, int panel_id, int lcd_id)
{
	char primary_key[20];
	int value,ret;
	struct sunxi_panel *sunxi_panel = NULL;
	struct device_node *node;

	sprintf(primary_key, "sunxi-lcd%d", lcd_id);
	node = sunxi_drm_get_name_node(primary_key);
	if (!node) {
		DRM_ERROR("get device [%s] node fail.\n", primary_key);
		return NULL;
	}

	ret = sunxi_drm_get_sys_item_int(node, "lcd_used", &value);
	if (ret == 0) {
		if (value == 1) {
			ret = sunxi_drm_get_sys_item_int(node, "lcd_if", &value);
			if (ret == 0) {
				if (!sunxi_cmp_panel_type(hw_res->res_id, value)) {
					goto err_false;
				}

				sunxi_panel = sunxi_panel_creat(DISP_OUTPUT_TYPE_LCD, panel_id);
				if (!sunxi_panel) {
					DRM_ERROR("creat sunxi panel fail.\n");
					goto err_false;
				}

				ret = sunxi_lcd_private_init(sunxi_panel, lcd_id);
				if (ret) {
					DRM_ERROR("creat lcd_private fail.\n");
					goto err_panel;
				}

				ret = sunxi_lcd_hwres_ops_init(hw_res, value);
				if (ret) {
					DRM_ERROR("creat hwres_ops fail.\n");
					goto err_pravate;
				} 

				ret = sunxi_lcd_panel_ops_init(sunxi_panel);
				if (ret) {
					DRM_ERROR("creat panel_ops fail.\n");
					goto err_ops;
				}
			}
		}
	}

	return sunxi_panel;

err_ops:
	sunxi_lcd_hwres_ops_destroy(hw_res);
err_pravate:
	sunxi_lcd_private_destroy(sunxi_panel->private);
err_panel:
	if(sunxi_panel)
	sunxi_panel_destroy(sunxi_panel);
err_false:
	return NULL;
}

void sunxi_lcd_destroy(struct sunxi_panel *sunxi_panel,
	struct sunxi_hardware_res *hw_res)
{
	if(sunxi_panel) {
		sunxi_lcd_hwres_ops_destroy(hw_res);
		sunxi_lcd_private_destroy(sunxi_panel->private);
	}
	return ;
}
