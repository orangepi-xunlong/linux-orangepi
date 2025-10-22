/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "disp_lcd.h"
#include "../dev_disp.h"
#include "disp_display.h"
#include "../lcd/panels.h"
#include "fb_top.h"
extern u32 suspend_status;
#include <linux/reset.h>

struct lcd_notify_tp screen_driver_work;
extern void lcd_set_panel_funs(void);
static s32 disp_lcd_disable(struct disp_device *lcd);
static s32 disp_lcd_enable(struct disp_device *lcd);
#define DISP_LCD_MODE_DIRTY_MASK 0x80000000
static struct lcd_timing *lcd_param;
struct disp_lcd_private_data {
	struct disp_lcd_flow open_flow;
	struct disp_lcd_flow close_flow;
	struct disp_panel_para panel_info;
	struct panel_extend_para panel_extend_info;
	struct panel_extend_para panel_extend_info_set;
	u32    panel_extend_dirty;
	struct disp_lcd_cfg lcd_cfg;
	struct disp_lcd_panel_fun lcd_panel_fun;
	bool enabling;
	bool disabling;
	bool bl_enabled;
	u32 irq_no;
	u32 irq_no_dsi;
	u32 irq_no_edp;
	u32 enabled;
	u32 power_enabled;
	u32 bl_need_enabled;
	u32 frame_per_sec;
	u32 usec_per_line;
	u32 judge_line;
	u32 tri_finish_fail;
	s32 color_temperature;
	u32 color_inverse;
	/* 0:reset all module, 1:reset panel only */
	u32 esd_reset_level;
	struct {
		uintptr_t dev;
		u32 channel;
		u32 polarity;
		u32 period_ns;
		u32 duty_ns;
		u32 enabled;
	} pwm_info;

	struct clk *clk_tcon_lcd;
	struct clk *clk_bus_tcon_lcd;
	struct clk *clk_lvds;
	struct clk *clk_edp;
	struct clk *clk_parent;

	struct reset_control *rst;
	struct reset_control *rst_bus_lvds;
	struct reset_control *rst_bus_tcon_lcd;
#if defined(SUPPORT_DSI)
	struct clk *clk_mipi_dsi[CLK_NUM_PER_DSI * DEVICE_DSI_NUM];  // 1 * 2
	struct clk *clk_bus_mipi_dsi[CLK_NUM_PER_DSI * DEVICE_DSI_NUM];
	struct reset_control *rst_bus_mipi_dsi[CLK_NUM_PER_DSI * DEVICE_DSI_NUM];

#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	/* In v35x, TCON can control all DSIS. */
	struct clk *clk_mipi_dsi_combphy[COMBPHY_CLK_NUM_IN_SYS];
#endif  /* DE_VERSION_V35X */
#endif  /* SUPPORT_DSI */

	/* 0:no reset process;1:reset request;2:resetting */
	atomic_t lcd_resetting;
	struct work_struct reflush_work;
	struct disp_lcd_esd_info esd_inf;
	struct disp_device_config config;
};
static spinlock_t lcd_data_lock;

static u32 lcd_device_num;
static struct disp_device *lcds;
static struct disp_lcd_private_data *lcd_private;
static unsigned long curr_irq_count;

static int disp_check_timing_param(struct disp_panel_para *panel)
{
	u32 x = panel->lcd_x;
	u32 y = panel->lcd_y;
	u32 vt = panel->lcd_vt;
	u32 vbp = panel->lcd_vbp;
	u32 ht = panel->lcd_ht;
	u32 hbp = panel->lcd_hbp;
	u32 hspw = panel->lcd_hspw;
	u32 vspw = panel->lcd_vspw;

	if (ht <= x + hbp) {
		panel->lcd_ht = x + hbp + (hbp - panel->lcd_hspw);
		DE_WARN("[ERROR]timing param error\n");
		DE_WARN("lcd_ht need to be equal to lcd_x + lcd_hbp + lcd_hfp\n");
		DE_WARN("lcd_x = %d, lcd_hbp = %d, lcd_ht = %d, lcd_hspw = %d\n", x, hbp, ht, hspw);
		DE_WARN("Now, reset lcd_ht = %u\n\n", panel->lcd_ht);
	}

	if (vt <= y + vbp) {
		panel->lcd_vt = y + vbp + (vbp - panel->lcd_vspw);
		DE_WARN("[ERROR]timing param error\n");
		DE_WARN("lcd_vt need to be equal to lcd_y + lcd_vbp + lcd_vfp\n");
		DE_WARN("lcd_y = %d, lcd_vbp = %d, lcd_vt = %d, lcd_vspw = %d\n", y, vbp, vt, vspw);
		DE_WARN("Now, reset lcd_vt = %u. \n\n", panel->lcd_vt);
	}

	return 0;
}

int disp_lcd_get_device_count(void)
{
	return lcd_device_num;
}

struct disp_device *disp_get_lcd(u32 dev_index)
{
	if (dev_index >= lcd_device_num)
		return NULL;

	return &lcds[dev_index];
}

static u32 disp_lcd_get_real_fps(struct disp_device *dispdev)
{
	u32 curr_irq;
	unsigned long curr_jiffies;
	static unsigned long old_jiffies;
	static unsigned long last_irq_count;
	unsigned int old_time_msec;
	unsigned int curr_time_msec;
	u32 fps;

	/* Sync data from system. */
	curr_jiffies = jiffies;
	curr_irq = curr_irq_count;

	DISP_DO_ONCE({
		old_jiffies = curr_jiffies;
		last_irq_count = curr_irq;
	});

	old_time_msec = jiffies_to_msecs(old_jiffies);
	curr_time_msec = jiffies_to_msecs(curr_jiffies);
	// DE_INFO("fps: %lf \n", ((double)curr_time_msec - old_time_msec) / (double)(curr_irq - last_irq_count));
	// DE_INFO("fps: %u \n", (curr_time_msec - old_time_msec));
	// DE_INFO("fps: %lu \n", (curr_irq - last_irq_count));
	if ((curr_time_msec - old_time_msec) == 0)
		fps =  255;
	else
		fps = (u32)(curr_irq - last_irq_count) * 1000 / (curr_time_msec - old_time_msec);

	old_jiffies = curr_jiffies;
	last_irq_count = curr_irq;

	return fps;
}

static struct disp_lcd_private_data *disp_lcd_get_priv(struct disp_device *lcd)
{
	if (lcd == NULL) {
		DE_WARN("param is NULL\n");
		return NULL;
	}

	return (struct disp_lcd_private_data *)lcd->priv_data;
}

static s32 disp_lcd_rcq_protect(struct disp_device *lcd, bool protect)
{
	struct disp_manager *mgr;

	if (lcd == NULL) {
		DE_INFO("NULL hdl\n");
		return -1;
	}

	mgr = lcd->manager;
	if (mgr && mgr->reg_protect)
		return mgr->reg_protect(mgr, protect);

	return 0;
}


static s32 disp_lcd_is_used(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	s32 ret = 0;

	if ((lcd == NULL) || (lcdp == NULL))
		ret = 0;
	else
		ret = (s32)lcdp->lcd_cfg.lcd_used;

	return ret;
}

static s32 lcd_parse_panel_para(u32 disp, struct disp_panel_para *info)
{
	s32 ret = 0;
	char primary_key[25];
	s32 value = 0;

	sprintf(primary_key, "lcd%d", disp);
	memset(info, 0, sizeof(struct disp_panel_para));

	ret = disp_sys_script_get_item(primary_key, "lcd_x", &value, 1);
	if (ret == 1)
		info->lcd_x = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_y", &value, 1);
	if (ret == 1)
		info->lcd_y = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_width", &value, 1);
	if (ret == 1)
		info->lcd_width = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_height", &value, 1);
	if (ret == 1)
		info->lcd_height = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dclk_freq", &value, 1);
	if (ret == 1)
		info->lcd_dclk_freq = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_pwm_used", &value, 1);
	if (ret == 1)
		info->lcd_pwm_used = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_pwm_ch", &value, 1);
	if (ret == 1)
		info->lcd_pwm_ch = value;

	ret =
	    disp_sys_script_get_item2(primary_key, "lcd_pwm_name",
				     &info->lcd_pwm_name);
	ret = disp_sys_script_get_item(primary_key, "lcd_pwm_freq", &value, 1);
	if (ret == 1)
		info->lcd_pwm_freq = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_pwm_pol", &value, 1);
	if (ret == 1)
		info->lcd_pwm_pol = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_if", &value, 1);
	if (ret == 1)
		info->lcd_if = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_hbp", &value, 1);
	if (ret == 1)
		info->lcd_hbp = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_ht", &value, 1);
	if (ret == 1)
		info->lcd_ht = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_vbp", &value, 1);
	if (ret == 1)
		info->lcd_vbp = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_vt", &value, 1);
	if (ret == 1)
		info->lcd_vt = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_gsensor_detect", &value, 1);
	if (ret == 1)
		info->lcd_gsensor_detect = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_hv_if", &value, 1);
	if (ret == 1)
		info->lcd_hv_if = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_vspw", &value, 1);
	if (ret == 1)
		info->lcd_vspw = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_hspw", &value, 1);
	if (ret == 1)
		info->lcd_hspw = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_lvds_if", &value, 1);
	if (ret == 1)
		info->lcd_lvds_if = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_convert_if", &value, 1);
	if (ret == 1) {
		switch (value) {
		case 0:
			info->convert_info.is_enabled = 0;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_NONE;
			break;
		case 1:
			info->convert_info.is_enabled = 0;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_LCD;
			break;
		case 2:
			info->convert_info.is_enabled = 1;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_TV;
			break;
		case 3:
			info->convert_info.is_enabled = 1;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_HDMI;
			break;
		case 4:
			info->convert_info.is_enabled = 1;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_VGA;
			break;
		case 5:
			info->convert_info.is_enabled = 1;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_VDPO;
			break;
		case 6:
			info->convert_info.is_enabled = 1;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_EDP;
			break;
		case 7:
			info->convert_info.is_enabled = 1;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_RTWB;
			break;
		default:
			info->convert_info.is_enabled = 0;
			info->convert_info.convert_type = DISP_OUTPUT_TYPE_NONE;
			break;
		}
	}

	ret = disp_sys_script_get_item(primary_key, "lcd_lvds_mode", &value, 1);
	if (ret == 1)
		info->lcd_lvds_mode = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_lvds_colordepth", &value,
				     1);
	if (ret == 1)
		info->lcd_lvds_colordepth = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_lvds_io_polarity",
				     &value, 1);
	if (ret == 1)
		info->lcd_lvds_io_polarity = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_lvds_clk_polarity",
				     &value, 1);
	if (ret == 1)
		info->lcd_lvds_clk_polarity = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_cpu_if", &value, 1);
	if (ret == 1)
		info->lcd_cpu_if = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_cpu_te", &value, 1);
	if (ret == 1)
		info->lcd_cpu_te = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_cpu_mode", &value, 1);
	if (ret == 1)
		info->lcd_cpu_mode = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_frm", &value, 1);
	if (ret == 1)
		info->lcd_frm = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_start_delay", &value, 1);
	if (ret == 1)
		info->lcd_start_delay = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_if", &value, 1);
	if (ret == 1)
		info->lcd_dsi_if = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_lane", &value, 1);
	if (ret == 1)
		info->lcd_dsi_lane = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_dsi_format", &value, 1);
	if (ret == 1)
		info->lcd_dsi_format = value;


	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_eotp", &value, 1);
	if (ret == 1)
		info->lcd_dsi_eotp = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_te", &value, 1);
	if (ret == 1)
		info->lcd_dsi_te = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_port_num",
				       &value, 1);
	if (ret == 1)
		info->lcd_dsi_port_num = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_clk_mode", &value, 1);
	if (ret == 1)
		info->lcd_dsi_clk_mode = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_clk_if", &value, 1);
	if (ret == 1)
		info->lcd_dsi_clk_if = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_dsi_data_mode", &value, 1);
	if (ret == 1)
		info->lcd_dsi_data_mode = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_tcon_mode", &value, 1);
	if (ret == 1)
		info->lcd_tcon_mode = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_slave_tcon_num",
				       &value, 1);
	if (ret == 1)
		info->lcd_slave_tcon_num = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_slave_stop_pos",
				       &value, 1);
	if (ret == 1)
		info->lcd_slave_stop_pos = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_sync_pixel_num",
				       &value, 1);
	if (ret == 1)
		info->lcd_sync_pixel_num = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_sync_line_num",
				       &value, 1);
	if (ret == 1)
		info->lcd_sync_line_num = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_tcon_en_odd_even_div",
				       &value, 1);
	if (ret == 1)
		info->lcd_tcon_en_odd_even = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_fsync_en", &value, 1);
	if (ret == 1)
		info->lcd_fsync_en = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_fsync_act_time", &value, 1);
	if (ret == 1)
		info->lcd_fsync_act_time = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_fsync_dis_time", &value, 1);
	if (ret == 1)
		info->lcd_fsync_dis_time = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_fsync_pol", &value,
				     1);
	if (ret == 1)
		info->lcd_fsync_pol = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_hv_clk_phase", &value,
				     1);
	if (ret == 1)
		info->lcd_hv_clk_phase = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_hv_sync_polarity",
				     &value, 1);
	if (ret == 1)
		info->lcd_hv_sync_polarity = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_hv_data_polarity",
				     &value, 1);
	if (ret == 1)
		info->lcd_hv_data_polarity = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_hv_srgb_seq", &value, 1);
	if (ret == 1)
		info->lcd_hv_srgb_seq = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_rb_swap", &value, 1);
	if (ret == 1)
		info->lcd_rb_swap = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_hv_syuv_seq", &value, 1);
	if (ret == 1)
		info->lcd_hv_syuv_seq = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_hv_syuv_fdly", &value,
				     1);
	if (ret == 1)
		info->lcd_hv_syuv_fdly = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_gamma_en", &value, 1);
	if (ret == 1)
		info->lcd_gamma_en = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_cmap_en", &value, 1);
	if (ret == 1)
		info->lcd_cmap_en = value;

	ret = disp_sys_script_get_item(primary_key, "lcd_xtal_freq", &value, 1);
	if (ret == 1)
		info->lcd_xtal_freq = value;

	ret =
	    disp_sys_script_get_item(primary_key, "lcd_size",
				     (int *)info->lcd_size, 2);
	ret =
	    disp_sys_script_get_item(primary_key, "lcd_model_name",
				     (int *)info->lcd_model_name, 2);
	ret =
	    disp_sys_script_get_item(primary_key, "lcd_driver_name",
				     (int *)info->lcd_driver_name, 2);

	return 0;
}

static void lcd_get_sys_config(u32 disp, struct disp_lcd_cfg *lcd_cfg)
{
	struct disp_gpio_info *gpio_info;
	int value = 1;
	char primary_key[20], sub_name[25];
	int i = 0;
	int ret;

	sprintf(primary_key, "lcd%d", disp);
	/* lcd_used */
	ret = disp_sys_script_get_item(primary_key, "lcd_used", &value, 1);
	if (ret == 1)
		lcd_cfg->lcd_used = value;

	if (lcd_cfg->lcd_used == 0)
		return;

	/* lcd_bl_en */
	lcd_cfg->lcd_bl_en_used = 0;
	gpio_info = &(lcd_cfg->lcd_bl_en);
	ret = disp_sys_script_get_item(primary_key, "lcd_bl_en", (int *)gpio_info, 3);
	if (!ret)
		lcd_cfg->lcd_bl_en_used = 1;

	sprintf(sub_name, "lcd_bl_en_power");
	ret = disp_sys_script_get_item(primary_key, "lcd_bl_en_power", (int *)lcd_cfg->lcd_bl_en_power, 2);
	if (ret)
		return;

	/* lcd_power */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (i == 0)
			sprintf(sub_name, "lcd_power");
		else
			sprintf(sub_name, "lcd_power%d", i);
		lcd_cfg->lcd_power_used[i] = 0;
		ret =
		    disp_sys_script_get_item(primary_key, sub_name,
					     (int *)(lcd_cfg->lcd_power[i]), 2);
		if (ret == 2)
			/* str */
			lcd_cfg->lcd_power_used[i] = 1;
	}

	/* lcd_gpio */
	for (i = 0; i < 6; i++) {
		sprintf(sub_name, "lcd_gpio_%d", i);

		gpio_info = &(lcd_cfg->lcd_gpio[i]);
		ret = disp_sys_script_get_item(primary_key, sub_name, (int *)gpio_info, 3);
		if (!ret)
			lcd_cfg->lcd_gpio_used[i] = 1;
	}

	/* lcd_gpio_scl,lcd_gpio_sda */
	gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SCL]);
	ret = disp_sys_script_get_item(primary_key, "lcd_gpio_scl", (int *)gpio_info, 3);
	if (ret == 3)
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SCL] = 1;

	gpio_info = &(lcd_cfg->lcd_gpio[LCD_GPIO_SDA]);
	ret = disp_sys_script_get_item(primary_key, "lcd_gpio_sda", (int *)gpio_info, 3);
	if (ret == 3)
		lcd_cfg->lcd_gpio_used[LCD_GPIO_SDA] = 1;

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		sprintf(sub_name, "lcd_gpio_power%d", i);

		ret =
		    disp_sys_script_get_item(primary_key, sub_name,
					     (int *)lcd_cfg->lcd_gpio_power[i],
					     2);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (i == 0)
			sprintf(sub_name, "lcd_pin_power");
		else
			sprintf(sub_name, "lcd_pin_power%d", i);
		ret =
		    disp_sys_script_get_item(primary_key, sub_name,
					     (int *)lcd_cfg->lcd_pin_power[i],
					     2);
	}

/* backlight adjust */
	for (i = 0; i < 101; i++) {
		sprintf(sub_name, "lcd_bl_%d_percent", i);
		lcd_cfg->backlight_curve_adjust[i] = 0;

		if (i == 100)
			lcd_cfg->backlight_curve_adjust[i] = 255;

		ret =
		    disp_sys_script_get_item(primary_key, sub_name, &value, 1);
		if (ret == 1) {
			value = (value > 100) ? 100 : value;
			value = value * 255 / 100;
			lcd_cfg->backlight_curve_adjust[i] = value;
		}
	}

	sprintf(sub_name, "lcd_backlight");
	ret = disp_sys_script_get_item(primary_key, sub_name, &value, 1);
	if (ret == 1) {
		value = (value > 256) ? 256 : value;
		lcd_cfg->backlight_bright = value;
	} else {
		lcd_cfg->backlight_bright = 197;
	}

}

static s32 lcd_clk_init(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	DE_INFO("lcd %d clk init\n", lcd->disp);

	lcdp->clk_parent = NULL;
	lcdp->clk_parent = clk_get_parent(lcdp->clk_tcon_lcd);

	return DIS_SUCCESS;
}

static s32 lcd_clk_exit(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	if (lcdp->clk_parent)
		clk_put(lcdp->clk_parent);

	return DIS_SUCCESS;
}

static int lcd_regulator_get_wrap(struct disp_lcd_cfg *lcd_cfg)
{
	int i;
	struct device *dev = g_disp_drv.dev;
	struct regulator *regulator;

	if (strlen(lcd_cfg->lcd_bl_en_power)) {
		regulator = regulator_get(dev, lcd_cfg->lcd_bl_en_power);
		if (!regulator)
			DE_WARN("regulator_get for %s failed\n", lcd_cfg->lcd_bl_en_power);
		else
			lcd_cfg->bl_regulator = regulator;
	}

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (strlen(lcd_cfg->lcd_power[i])) {
			regulator = regulator_get(dev, lcd_cfg->lcd_power[i]);
			if (!regulator) {
				DE_WARN("regulator_get for %s failed\n", lcd_cfg->lcd_power[i]);
				continue;
			}
			lcd_cfg->regulator[i] = regulator;
		}
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (strlen(lcd_cfg->lcd_gpio_power[i])) {
			regulator = regulator_get(dev, lcd_cfg->lcd_gpio_power[i]);
			if (!regulator) {
				DE_WARN("regulator_get for %s failed\n", lcd_cfg->lcd_gpio_power[i]);
				continue;
			}
			lcd_cfg->gpio_regulator[i] = regulator;
		}
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (strlen(lcd_cfg->lcd_pin_power[i])) {
			regulator = regulator_get(dev, lcd_cfg->lcd_pin_power[i]);
			if (!regulator) {
				DE_WARN("regulator_get for %s failed\n", lcd_cfg->lcd_pin_power[i]);
				continue;
			}
			lcd_cfg->pin_regulator[i] = regulator;
		}
	}

	return 0;
}

static void lcd_regulator_put_wrap(struct disp_lcd_cfg *lcd_cfg)
{
	int i;

	if (lcd_cfg->bl_regulator)
		regulator_put(lcd_cfg->bl_regulator);

	for (i = 0; i < LCD_POWER_NUM; i++) {
		if (lcd_cfg->regulator[i])
			regulator_put(lcd_cfg->regulator[i]);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (lcd_cfg->gpio_regulator[i])
			regulator_put(lcd_cfg->gpio_regulator[i]);
	}

	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		if (lcd_cfg->pin_regulator[i])
			regulator_put(lcd_cfg->pin_regulator[i]);
	}
}

/**
 * @name   :cal_real_frame_period
 * @brief  :set lcd->timings.frame_period (nsec)
 *              ,lcd->timings.start_delay (line)
 *              and lcd->timings.dclk_rate_set(hz)
 * @param  :lcd:pointer of disp_device
 * @return :0 if success, other fail
 */
static s32 cal_real_frame_period(struct disp_device *lcd)
{
	s32 ret = -1;
	struct disp_lcd_private_data *lcdp;
	struct lcd_clk_info clk_info;
	unsigned long long temp = 0;

	if (!lcd) {
		DE_WARN("NULL hdl\n");
		goto OUT;
	}

	lcdp = disp_lcd_get_priv(lcd);

	if (!lcdp) {
		DE_WARN("NULL hdl\n");
		goto OUT;
	}

	memset(&clk_info, 0, sizeof(struct lcd_clk_info));
	disp_al_lcd_get_clk_info(lcd->hwdev_index, &clk_info,
				 &lcdp->panel_info);

	if (!lcdp->clk_tcon_lcd) {
		DE_WARN("clk is NULL\n");
		goto OUT;
	}

#if defined(SUPPORT_DSI)
	if (lcdp->panel_info.lcd_if == LCD_IF_DSI)
		lcd->timings.dclk_rate_set = clk_get_rate(lcdp->clk_mipi_dsi[0]) / clk_info.dsi_div;
#endif

	if (lcdp->panel_info.lcd_if != LCD_IF_DSI)
		lcd->timings.dclk_rate_set = clk_get_rate(lcdp->clk_tcon_lcd) / clk_info.tcon_div;

	if (lcd->timings.dclk_rate_set == 0) {
		DE_WARN("lcd dclk_rate_set is 0\n");
		ret = -2;
		goto OUT;
	}

	temp = ONE_SEC * lcdp->panel_info.lcd_ht * lcdp->panel_info.lcd_vt;

	do_div(temp, lcd->timings.dclk_rate_set);

	lcd->timings.frame_period = temp;

	lcd->timings.start_delay =
	    disp_al_lcd_get_start_delay(lcd->hwdev_index, &lcdp->panel_info);

	DE_INFO("lcd frame period:%llu\n", lcd->timings.frame_period);

	ret = 0;
OUT:
	return ret;
}

#if defined(DE_VERSION_V35X) && defined(SUPPORT_DSI) && 0
static unsigned long combphy_clk_config(struct clk *combphy_clk, struct disp_panel_para *panel)
{
	unsigned long combphy_rate = 0;
	unsigned long combphy_rate_set = 0;
	unsigned char bits_per_pixel;

	switch (panel->lcd_dsi_format) {
	case LCD_DSI_FORMAT_RGB888:
		bits_per_pixel = 24;
		break;
	case LCD_DSI_FORMAT_RGB666:
		bits_per_pixel = 18;
		break;
	case LCD_DSI_FORMAT_RGB666P:
		bits_per_pixel = 18;
		break;
	case LCD_DSI_FORMAT_RGB565:
		bits_per_pixel = 16;
		break;
	}

	combphy_rate = panel->lcd_dclk_freq * bits_per_pixel / panel->lcd_dsi_lane * 1000000;

	clk_set_rate(combphy_clk, combphy_rate);
	combphy_rate_set = clk_get_rate(combphy_clk);

	if (combphy_rate != combphy_rate_set)
		DE_WARN("combphy clk was setted rate  %lu, but real rate is %lu", combphy_rate, combphy_rate_set);

	DE_WARN("combphy clk was setted rate  %lu, but real rate is %lu", combphy_rate, combphy_rate_set);
	return combphy_rate_set;
}
#endif  /* DE_VERSION_V35X */

static s32 lcd_clk_config(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_panel_para *panel;
	struct lcd_clk_info clk_info;
#ifndef CONFIG_ARCH_SUN55IW3
	unsigned long pll_rate = 297000000, pll_rate_set = 297000000;
#endif
	unsigned long lcd_rate = 33000000, dclk_rate = 33000000, dsi_rate = 0;	/* hz */
	unsigned long lcd_rate_set = 33000000, dclk_rate_set = 33000000, dsi_rate_set = 0;	/* hz */
#if defined(SUPPORT_DSI)
	u32 i = 0, j = 0;
	u32 dsi_num = 0;
#endif
	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}
	memset(&clk_info, 0, sizeof(struct lcd_clk_info));
	disp_al_lcd_get_clk_info(lcd->hwdev_index, &clk_info,
				 &lcdp->panel_info);
	dclk_rate = lcdp->panel_info.lcd_dclk_freq * 1000000;	/* Mhz -> hz */
	if (lcdp->panel_info.lcd_if == LCD_IF_DSI) {
		lcd_rate = dclk_rate * clk_info.dsi_div;
#ifndef CONFIG_ARCH_SUN55IW3
		pll_rate = lcd_rate * clk_info.lcd_div;
	} else {
		lcd_rate = dclk_rate * clk_info.tcon_div;
		pll_rate = lcd_rate * clk_info.lcd_div;
	}
	dsi_rate = pll_rate / clk_info.dsi_div;

	if (lcdp->clk_parent) {
		clk_set_rate(lcdp->clk_parent, pll_rate);
		pll_rate_set = clk_get_rate(lcdp->clk_parent);
	}

	if (clk_info.lcd_div)
		lcd_rate_set = pll_rate_set / clk_info.lcd_div;
	else
		lcd_rate_set = pll_rate_set;

	clk_set_rate(lcdp->clk_tcon_lcd, lcd_rate_set);
#else
	} else
		lcd_rate = dclk_rate * clk_info.tcon_div;

	dsi_rate = dclk_rate * clk_info.lcd_div;

	clk_set_rate(lcdp->clk_tcon_lcd, lcd_rate);
#endif
	lcd_rate_set = clk_get_rate(lcdp->clk_tcon_lcd);
#if defined(SUPPORT_DSI)
	if (lcdp->panel_info.lcd_if == LCD_IF_DSI) {
		if (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE)
#ifdef CONFIG_ARCH_SUN55IW3
			dsi_rate_set = lcd_rate_set;
		else
			dsi_rate_set = lcd_rate_set / clk_info.dsi_div;
#else
			dsi_rate_set = pll_rate_set;
		else
			dsi_rate_set = pll_rate_set / clk_info.dsi_div;
#endif
		dsi_rate_set =
		    (clk_info.dsi_rate == 0) ? dsi_rate_set : clk_info.dsi_rate;
		dsi_num = (lcdp->panel_info.lcd_tcon_mode == DISP_TCON_DUAL_DSI)
			      ? 2
			      : 1;
		dsi_rate_set /= dsi_num;
		/* total number of dsi clk for current disp device */
		dsi_num *= CLK_NUM_PER_DSI;
		/* In the case of CLK_NUM_PER_DSI equal to 2 */
		/* even index mean hs clk which need to be seted */
		/* odd index mean lp clk which no need to be seted */
		for (i = 0, j = 0; i < CLK_DSI_NUM;
				i += CLK_NUM_PER_DSI) {
			if (lcdp->clk_mipi_dsi[i]) {
#if defined(DSI_VERSION_40)
				dsi_rate_set = 150000000;
#endif
				clk_set_rate(lcdp->clk_mipi_dsi[i], dsi_rate_set);
				DE_INFO(
				    "clk_set_rate:dsi's %d th clk with %ld\n",
				    i, dsi_rate_set);
				j += CLK_NUM_PER_DSI;
			}

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
			if (lcdp->clk_mipi_dsi_combphy[i]) {
				// unsigned long comb_rate;
				clk_set_rate(lcdp->clk_mipi_dsi_combphy[i], lcd_rate_set);
			}
#endif
			if (j == dsi_num)
				break;
		}
	}
#endif
	dclk_rate_set = lcd_rate_set / clk_info.tcon_div;
	panel = &lcdp->panel_info;
	if ((lcd_rate_set != lcd_rate) || (dclk_rate_set != dclk_rate)) {
#ifndef CONFIG_ARCH_SUN55IW3
		if (pll_rate_set > pll_rate) {
			panel->tcon_clk_div_ajust.clk_div_increase_or_decrease = INCREASE;
			panel->tcon_clk_div_ajust.div_multiple = pll_rate_set / pll_rate;
			dclk_rate_set /= panel->tcon_clk_div_ajust.div_multiple;
		} else {
			panel->tcon_clk_div_ajust.clk_div_increase_or_decrease = DECREASE;
			panel->tcon_clk_div_ajust.div_multiple = pll_rate / pll_rate_set;
			dclk_rate_set *= panel->tcon_clk_div_ajust.div_multiple;
		}

		DE_WARN("lcd %d, clk: pll(%ld),clk(%ld),dclk(%ld) dsi_rate(%ld)\n ",
			lcd->disp, pll_rate, lcd_rate, dclk_rate, dsi_rate);
		DE_WARN("clk real:pll(%ld),clk(%ld),dclk(%ld) dsi_rate(%ld)\n",
			pll_rate_set, lcd_rate_set, dclk_rate_set, dsi_rate_set);
#else
		DE_WARN("lcd %d, clk:tcon_clk(%ld),dclk(%ld) dsi_rate(%ld)\n ",
			lcd->disp, lcd_rate, dclk_rate, dsi_rate);
		DE_WARN("clk real:tcon_clk(%ld),dclk(%ld) dsi_rate(%ld)\n",
			lcd_rate_set, dclk_rate_set, dsi_rate_set);
#endif
	}
	return 0;
}

static s32 lcd_clk_enable(struct disp_device *lcd, bool sw_enable)
{
#if (defined SUPPORT_COMBO_DPHY) || (defined SUPPORT_DSI)
	int enable_mipi = 0;
	u32 i = 0;
#endif
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int ret = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}
	if (!sw_enable)
		lcd_clk_config(lcd);

	ret = reset_control_deassert(lcdp->rst_bus_tcon_lcd);
	if (ret) {
		DE_WARN("reset_control_deassert for rst_bus_tcon_lcd failed\n");
		return ret;
	}

	ret = clk_prepare_enable(lcdp->clk_tcon_lcd);
	if (ret) {
		DE_WARN("clk_prepare_enable for clk_tcon_lcd failed\n");
		return ret;
	}

	ret = clk_prepare_enable(lcdp->clk_bus_tcon_lcd);
	if (ret) {
		DE_WARN("clk_prepare_enable for clk_bus_tcon_lcd failed\n");
		return ret;
	}

	if (lcdp->panel_info.lcd_if == LCD_IF_LVDS) {
		ret = reset_control_deassert(lcdp->rst_bus_lvds);
		if (ret) {
			DE_WARN("reset_control_deassert for rst_lvds failed\n");
			return ret;
		}
		ret = clk_prepare_enable(lcdp->clk_lvds);
		if (ret) {
			DE_WARN("clk_prepare_enable for clk_lvds failed\n");
			return ret;
		}
#ifdef SUPPORT_COMBO_DPHY
		enable_mipi = 1;
#endif
	} else if (lcdp->panel_info.lcd_if == LCD_IF_DSI) {
#if defined(SUPPORT_DSI)
		enable_mipi = 1;
#endif
	} else if (lcdp->panel_info.lcd_if == LCD_IF_EDP) {
		ret = clk_prepare_enable(lcdp->clk_edp);
		if (ret) {
			DE_WARN("clk_prepare_enable for clk_edp failed\n");
			return ret;
		}
	}

#if defined(SUPPORT_DSI)
	if (enable_mipi) {
		int dsi_num;

		if (lcdp->panel_info.lcd_tcon_mode == 4 ||  /* One tcon drive two dsi. */
		   /* aw1890 lvds0 uses combophy, so we need to enable
		    * combophy bus gating and combophy clk, combophy is
		    * integrated in dsi, and the combophy bus gating
		    * is dsi bus gating */
		   ((lcdp->panel_info.lcd_lvds_if == LCD_LVDS_IF_DUAL_LINK ||
		     lcdp->panel_info.lcd_lvds_if == LCD_LVDS_IF_DUAL_LINK_SAME_SRC) && lcd->hwdev_index == 0))
			dsi_num = CLK_DSI_NUM;
		else
			dsi_num = 1;

		for (i = 0; i < dsi_num; i++) {
			ret = reset_control_deassert(lcdp->rst_bus_mipi_dsi[i]);
			if (ret) {
				DE_WARN("reset_control_deassert for rst_bus_mipi_dsi[%d] failed\n", i);
				return ret;
			}

			ret = clk_prepare_enable(lcdp->clk_mipi_dsi[i]);
			if (ret) {
				DE_WARN("clk_prepare_enable for clk_mipi_dsi[%d] failed\n", i);
				return ret;
			}

			ret = clk_prepare_enable(lcdp->clk_bus_mipi_dsi[i]);
			if (ret) {
				DE_WARN("clk_prepare_enable for clk_bus_mipi_dsi[%d] failed\n", i);
				return ret;
			}
	/* This clock cannot be used at FPGA stage */
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
			ret = clk_prepare_enable(lcdp->clk_mipi_dsi_combphy[i]);
			if (ret) {
				DE_WARN("clk_prepare_enable for clk_mipi_dsi_combphy[%d] failed\n", i);
				return ret;
			}
#endif
		}
	}
#endif
	if (sw_enable)
		lcd_clk_config(lcd);

	return ret;
}

static s32 lcd_clk_disable(struct disp_device *lcd)
{
	int ret;
#if defined(SUPPORT_DSI)
	int disable_mipi = 0;
	u32 i = 0;
#endif

	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	if (lcdp->panel_info.lcd_if == LCD_IF_LVDS) {
		clk_disable(lcdp->clk_lvds);
#ifdef SUPPORT_COMBO_DPHY
		ret = reset_control_assert(lcdp->rst_bus_lvds);
		if (ret) {
			DE_WARN("reset_control_assert for rst_bus_lvds failed\n");
			return ret;
		}
#if defined(SUPPORT_DSI)
		disable_mipi = 1;
#endif
#endif
#if defined(SUPPORT_DSI)
	} else if (lcdp->panel_info.lcd_if == LCD_IF_DSI) {
		disable_mipi = 1;
#endif
	} else if (lcdp->panel_info.lcd_if == LCD_IF_EDP) {
		clk_disable_unprepare(lcdp->clk_edp);
	}

#if defined(SUPPORT_DSI)
	if (disable_mipi) {
		int dsi_num;

		if (lcdp->panel_info.lcd_tcon_mode == 4) {  /* One tcon drive two dsi. */
			dsi_num = CLK_DSI_NUM;
		} else
			dsi_num = 1;

		for (i = 0; i < dsi_num; i++) {
			clk_disable_unprepare(lcdp->clk_bus_mipi_dsi[i]);
			clk_disable_unprepare(lcdp->clk_mipi_dsi[i]);
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
			clk_disable_unprepare(lcdp->clk_mipi_dsi_combphy[i]);
#endif
			ret = reset_control_assert(lcdp->rst_bus_mipi_dsi[i]);
			if (ret) {
				DE_WARN("reset_control_assert for rst_bus_mipi_dsi failed\n");
				return ret;
			}
		}

	}
#endif

	clk_disable_unprepare(lcdp->clk_bus_tcon_lcd);
	clk_disable_unprepare(lcdp->clk_tcon_lcd);

	ret = reset_control_assert(lcdp->rst);
	if (ret) {
		DE_WARN("reset_control_assert for rst failed\n");
		return ret;
	}

	return 0;
}

static int lcd_calc_judge_line(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	if (lcdp->usec_per_line == 0) {
		struct disp_panel_para *panel_info = &lcdp->panel_info;
		/*
		 * usec_per_line = 1 / fps / vt * 1000000
		 *               = 1 / (dclk * 1000000 / vt / ht) / vt * 1000000
		 *               = ht / dclk(Mhz)
		 */
		lcdp->usec_per_line = panel_info->lcd_ht
		    / panel_info->lcd_dclk_freq;
	}

	if (lcdp->judge_line == 0) {
		int start_delay = disp_al_lcd_get_start_delay(lcd->hwdev_index,
							      &lcdp->panel_info);
		int usec_start_delay = start_delay * lcdp->usec_per_line;
		int usec_judge_point;

		if (usec_start_delay <= 200)
			usec_judge_point = usec_start_delay * 3 / 7;
		else if (usec_start_delay <= 400)
			usec_judge_point = usec_start_delay / 2;
		else
			usec_judge_point = 200;
		lcdp->judge_line = usec_judge_point / lcdp->usec_per_line;
	}

	return 0;
}

#ifdef EINK_FLUSH_TIME_TEST
struct timeval lcd_start, lcd_mid, lcd_mid1, lcd_mid2, lcd_end, t5_b, t5_e;
struct timeval pin_b, pin_e, po_b, po_e, tocn_b, tcon_e;
unsigned int lcd_t1 = 0, lcd_t2 = 0, lcd_t3 = 0, lcd_t4 = 0, lcd_t5 = 0;
unsigned int lcd_pin, lcd_po, lcd_tcon;
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
struct qareg_iomap_data {
	int initialized;
	void *qa_addr;
	void *disp_cfg_addr;
};
static struct qareg_iomap_data iomap_data;
#endif

extern int sunxi_get_soc_chipid_str(char *serial);

static s32 disp_lcd_speed_limit(struct disp_panel_para *panel, u32 *min_dclk,
				u32 *max_dclk)
{
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	char id[17] = "";
	unsigned long value = 0;
	unsigned int qa_val = 0;
	unsigned int ic_ver = 0, display_cfg_flag = 0;

	if (!iomap_data.initialized) {
		iomap_data.qa_addr = ioremap(0x0300621c, 4);
		iomap_data.disp_cfg_addr = ioremap(0x03006218, 4);
		iomap_data.initialized = 1;
	}
#endif

	/* init unlimit */
	*min_dclk = 0;
	*max_dclk = 9999;

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	if (!iomap_data.qa_addr || !iomap_data.disp_cfg_addr) {
		DE_WARN("ioremap fail!%p %p \n", iomap_data.qa_addr,
		       iomap_data.disp_cfg_addr);
		goto OUT;
	}
	qa_val = readl(iomap_data.qa_addr);
	qa_val = (qa_val >> 28) & 0x00000003;
	ic_ver = sunxi_get_soc_ver();
	display_cfg_flag = (readl(iomap_data.disp_cfg_addr) >> 12) & 0x00000001;
	/* FIXME */
	/* sunxi_get_soc_chipid_str(id); */
	strcpy(id, "0x1400");

	if (qa_val >= 2 && panel->lcd_if == LCD_IF_DSI) {
		/* bad IC  not support DSI */
		*min_dclk = 9999;
		*max_dclk = 0;
		goto OUT;
	}

	if (!kstrtoul(id, 16, &value)) {
		switch (value) {
		case 0x3c00: /* A530 */
			{
				/* ic version e */
				if ((ic_ver >= 4) && (!display_cfg_flag) &&
				    (panel->lcd_if != LCD_IF_DSI)) {
					/* only support DSI */
					*min_dclk = 9999;
					*max_dclk = 0;
					goto OUT;
				}

				if ((ic_ver == 0) && (panel->lcd_if == LCD_IF_DSI)) {
					/* not support DSI */
					*min_dclk = 9999;
					*max_dclk = 0;
					goto OUT;
				}

				if (panel->lcd_if == LCD_IF_DSI) {
					if (qa_val == 0)
						*min_dclk = 40; /* 1024*600@60 */
					/* normal qa and  B/C ic */
					if (qa_val == 1 && ic_ver == 0) {
						*min_dclk = 64; /* 1280*720@60 */
					}
				} else {
					/* LVDS or RGB */
					*min_dclk = 40; /* 1024*600@60 */
				}

				*max_dclk = 85; /* 1280*720@60 */
			} break;
		case 0x0400: /* A100 */
		{
			if (panel->lcd_if == LCD_IF_DSI) {
				if (qa_val == 0)
					*min_dclk = 40; /* 1024*600@60 */
				/* normal qa and  B/C ic */
				if (qa_val == 1 && ic_ver == 0) {
					*min_dclk = 64; /* 1280*720@60 */
				}
			} else {
				/* LVDS or RGB */
				*min_dclk = 40; /* 1024*600@60 */
			}

			*max_dclk = 85; /* 1280*720@60 */
		} break;
		case 0x1400: /* A133 */
		{
			if (panel->lcd_if == LCD_IF_DSI) {
				if ((qa_val == 0 && ic_ver == 0) ||
				    (qa_val == 1 && ic_ver == 3)) {
					/* D 01 or B/C 00 */
					*min_dclk = 40;
				} else if (qa_val == 1 && ic_ver == 0) {
					/* B/C 01 */
					*min_dclk = 64; /* 1280*720@60 */
				}
			}
			*max_dclk = 200; /* 1920*1200@60 */
		} break;
		case 0x1000: /* R818/MR813 */
		case 0x2000: {
			/* not bad ic */
			*max_dclk = 200; /* 1920*1200@60 */
		} break;
		case 0x0800: { /* T509 */
			if (panel->lcd_if == LCD_IF_DSI) {
				if (qa_val == 0 && ic_ver == 0)
					*min_dclk = 40;/* 1024*600@60 */
				else
					*min_dclk = 0;
			}
			*max_dclk = 200; /* 1920*1200@60 */
		} break;
		case 0x4000: { /* not support */
			*min_dclk = 9999;
			*max_dclk = 0;
		}
		break;
		default:
			break;
		}
	}
OUT:
#endif

	/* unlimit */
	return 0;
}

static s32 disp_lcd_tcon_enable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 min_dclk = 0, max_dclk = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	disp_lcd_speed_limit(&lcdp->panel_info, &min_dclk, &max_dclk);

	if (lcdp->panel_info.lcd_dclk_freq < min_dclk ||
	    lcdp->panel_info.lcd_dclk_freq > max_dclk) {
		return 0;
	}

	return disp_al_lcd_enable(lcd->hwdev_index, &lcdp->panel_info);
}

s32 disp_lcd_tcon_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	return disp_al_lcd_disable(lcd->hwdev_index, &lcdp->panel_info);
}

static s32 disp_lcd_pin_cfg(struct disp_device *lcd, u32 bon)
{
	int i, ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char dev_name[25];

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	DE_INFO("lcd %d pin config, state %s, %d\n", lcd->disp,
	       (bon) ? "on" : "off", bon);

	/* io-pad */
	if (bon == 1) {
		for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
			ret = disp_sys_power_enable(lcdp->lcd_cfg.pin_regulator[i]);
			if (ret)
				return DIS_FAIL;
		}
	}

	sprintf(dev_name, "lcd%d", lcd->disp);
	disp_sys_pin_set_state(dev_name,
			       (bon == 1) ?
			       DISP_PIN_STATE_ACTIVE : DISP_PIN_STATE_SLEEP);

	disp_al_lcd_io_cfg(lcd->hwdev_index, bon, &lcdp->panel_info);

	if (bon)
		return DIS_SUCCESS;

	for (i = LCD_GPIO_REGU_NUM - 1; i >= 0; i--) {
		ret = disp_sys_power_disable(lcdp->lcd_cfg.pin_regulator[i]);
		if (ret)
			return DIS_FAIL;
	}

	return DIS_SUCCESS;
}

#if IS_ENABLED(CONFIG_AW_FPGA_S4) && defined(SUPPORT_EINK)
static s32 disp_lcd_pwm_enable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	unsigned long reg = 0;
	unsigned long val = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		reg = ((unsigned long)0xf1c20878);
		val = readl(reg);
		val = val & 0xfff0ffff;
		val = val | 0x10000;
		writel(val, reg);
		reg = ((unsigned long)0xf1c2087C);
		val = readl(reg);
		val = val & 0xefffffff;
		val = val | 0x10000000;
		writel(val, reg);
		return 0;
	}
	DE_WARN("pwm device hdl is NULL\n");
	return DIS_FAIL;
}
static s32 disp_lcd_pwm_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long reg = 0;
	unsigned long val = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		reg = ((unsigned long)0xf1c20878);
		val = readl(reg);
		val = val & 0xfff0ffff;
		val = val | 0x10000;
		writel(val, reg);
		reg = ((unsigned long)0xf1c2087C);
		val = readl(reg);
		val = val & 0xefffffff;
		val = val | 0x00000000;
		writel(val, reg);
		return 0;
	}
	DE_WARN("pwm device hdl is NULL\n");
	return DIS_FAIL;
}
#else
static s32 disp_lcd_pwm_enable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	if (lcdp->panel_info.lcd_pwm_used && !lcdp->pwm_info.dev) {
		if (!lcdp->panel_info.lcd_pwm_name)
			lcdp->pwm_info.dev =
				disp_sys_pwm_request(lcdp->panel_info.lcd_pwm_ch);
		else
			lcdp->pwm_info.dev =
				disp_sys_pwm_get(lcdp->panel_info.lcd_pwm_name);
	}

	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		disp_sys_pwm_set_polarity(lcdp->pwm_info.dev,
				  lcdp->pwm_info.polarity);
		return disp_sys_pwm_enable(lcdp->pwm_info.dev);
	}
	DE_WARN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}

static s32 disp_lcd_pwm_disable(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	s32 ret = -1;
	struct pwm_device *pwm_dev;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	if (disp_lcd_is_used(lcd) && lcdp->pwm_info.dev) {
		ret = disp_sys_pwm_disable(lcdp->pwm_info.dev);
		pwm_dev = (struct pwm_device *)lcdp->pwm_info.dev;
		/* following is for reset pwm state purpose */
		disp_sys_pwm_config(lcdp->pwm_info.dev,
				    pwm_dev->state.duty_cycle - 1,
				    pwm_dev->state.period);
		disp_sys_pwm_set_polarity(lcdp->pwm_info.dev,
					  !lcdp->pwm_info.polarity);

		disp_sys_pwm_free(lcdp->pwm_info.dev);
		lcdp->pwm_info.dev = 0;

		return ret;
	}
	DE_WARN("pwm device hdl is NULL\n");

	return DIS_FAIL;
}
#endif

static s32 disp_lcd_backlight_enable(struct disp_device *lcd)
{
	int ret;
	struct disp_gpio_info gpio_info;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (lcdp->bl_enabled) {
		spin_unlock_irqrestore(&lcd_data_lock, flags);
		return -EBUSY;
	}

	lcdp->bl_need_enabled = 1;
	lcdp->bl_enabled = true;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	if (disp_lcd_is_used(lcd)) {
		unsigned bl;

		if (lcdp->lcd_cfg.lcd_bl_en_used) {
			/* io-pad */
			ret = disp_sys_power_enable(lcdp->lcd_cfg.bl_regulator);
			if (ret)
				return DIS_FAIL;

			memcpy(&gpio_info, &(lcdp->lcd_cfg.lcd_bl_en),
			       sizeof(struct disp_gpio_info));

			disp_sys_gpio_request(&gpio_info);
		}
		bl = disp_lcd_get_bright(lcd);
		disp_lcd_set_bright(lcd, bl);
	}

	return 0;
}

static s32 disp_lcd_backlight_disable(struct disp_device *lcd)
{
	int ret;
	struct disp_gpio_info gpio_info;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (!lcdp->bl_enabled) {
		spin_unlock_irqrestore(&lcd_data_lock, flags);
		return -EBUSY;
	}

	lcdp->bl_enabled = false;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	if (!disp_lcd_is_used(lcd))
		return DIS_SUCCESS;

	if (!lcdp->lcd_cfg.lcd_bl_en_used)
		return DIS_SUCCESS;

	memcpy(&gpio_info, &(lcdp->lcd_cfg.lcd_bl_en),
			sizeof(struct disp_gpio_info));
	gpio_info.value = 0;
	disp_sys_gpio_release(&gpio_info);

	ret = disp_sys_power_disable(lcdp->lcd_cfg.bl_regulator);
	if (ret)
		return DIS_FAIL;

	return DIS_SUCCESS;
}

#if IS_ENABLED(CONFIG_AW_FPGA_S4) && defined(SUPPORT_EINK)
static s32 disp_lcd_power_enable(struct disp_device *lcd, u32 power_id)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long reg = 0;
	unsigned long val = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd)) {
		reg = ((unsigned long)0xf1c20878);
		val = readl(reg);
		val = val & 0x000fffff;
		val = val | 0x11100000;
		writel(val, reg);
		reg = ((unsigned long)0xf1c2087C);
		val = readl(reg);
		val = val & 0x1fffffff;
		val = val | 0xe0000000;
		writel(val, reg);
		return 0;
	}
	return DIS_FAIL;
}
static s32 disp_lcd_power_disable(struct disp_device *lcd, u32 power_id)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long reg = 0;
	unsigned long val = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_used(lcd)) {
		reg = ((unsigned long)0xf1c20878);
		val = readl(reg);
		val = val & 0x000fffff;
		val = val | 0x77700000;
		writel(val, reg);
		return 0;
	}
	return DIS_FAIL;
}
#else
static s32 disp_lcd_power_enable(struct disp_device *lcd, u32 power_id)
{
	int ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if (!disp_lcd_is_used(lcd))
		return DIS_SUCCESS;

	if (!lcdp->lcd_cfg.regulator[power_id])
		return DIS_SUCCESS;

	ret = disp_sys_power_enable(lcdp->lcd_cfg.regulator[power_id]);
	if (ret)
		return DIS_FAIL;

	return DIS_SUCCESS;
}

static s32 disp_lcd_power_disable(struct disp_device *lcd, u32 power_id)
{
	int ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	ret = disp_sys_power_disable(lcdp->lcd_cfg.regulator[power_id]);
	if (ret)
		return DIS_FAIL;

	return DIS_SUCCESS;
}
#endif

static s32 disp_lcd_bright_get_adjust_value(struct disp_device *lcd, u32 bright)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	bright = (bright > 255) ? 255 : bright;
	return lcdp->panel_extend_info.lcd_bright_curve_tbl[bright];
}

static s32 disp_lcd_bright_curve_init(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 i = 0, j = 0;
	u32 items = 0;
	u32 lcd_bright_curve_tbl[101][2];

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	for (i = 0; i < 101; i++) {
		if (lcdp->lcd_cfg.backlight_curve_adjust[i] == 0) {
			if (i == 0) {
				lcd_bright_curve_tbl[items][0] = 0;
				lcd_bright_curve_tbl[items][1] = 0;
				items++;
			}
		} else {
			lcd_bright_curve_tbl[items][0] = 255 * i / 100;
			lcd_bright_curve_tbl[items][1] =
			    lcdp->lcd_cfg.backlight_curve_adjust[i];
			items++;
		}
	}

	for (i = 0; i < items - 1; i++) {
		u32 num =
		    lcd_bright_curve_tbl[i + 1][0] - lcd_bright_curve_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_bright_curve_tbl[i][1] +
			    ((lcd_bright_curve_tbl[i + 1][1] -
			      lcd_bright_curve_tbl[i][1]) * j) / num;
			lcdp->panel_extend_info.
			    lcd_bright_curve_tbl[lcd_bright_curve_tbl[i][0] +
						 j] = value;
		}
	}
	lcdp->panel_extend_info.lcd_bright_curve_tbl[255] =
	    lcd_bright_curve_tbl[items - 1][1];

	return 0;
}

s32 disp_lcd_set_bright(struct disp_device *lcd, u32 bright)
{
	__u64 backlight_bright = bright;
	__u64 backlight_dimming;
	__u64 period_ns;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;
	bool bright_update = false;
	struct disp_manager *mgr = NULL;
	struct disp_smbl *smbl = NULL;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	smbl = mgr->smbl;

	spin_lock_irqsave(&lcd_data_lock, flags);
	backlight_bright = (backlight_bright > 255) ? 255 : backlight_bright;
	if (lcdp->lcd_cfg.backlight_bright != backlight_bright) {
		bright_update = true;
		lcdp->lcd_cfg.backlight_bright = backlight_bright;
	}
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	if (bright_update && smbl)
		smbl->update_backlight(smbl, backlight_bright);

	if (lcdp->pwm_info.dev) {
		if (backlight_bright != 0)
			backlight_bright += 1;
		backlight_bright =
		    disp_lcd_bright_get_adjust_value(lcd, backlight_bright);

		lcdp->lcd_cfg.backlight_dimming =
		    (lcdp->lcd_cfg.backlight_dimming ==
		     0) ? 256 : lcdp->lcd_cfg.backlight_dimming;
		backlight_dimming = lcdp->lcd_cfg.backlight_dimming;
		period_ns = lcdp->pwm_info.period_ns;
		lcdp->pwm_info.duty_ns = bright == 0 ? 0 :
		    (backlight_bright * backlight_dimming * period_ns / 256 +
		     128) / 256;
		disp_sys_pwm_config(lcdp->pwm_info.dev, lcdp->pwm_info.duty_ns, period_ns);
	}

	if (lcdp->lcd_panel_fun.set_bright && lcdp->enabled) {
		lcdp->lcd_panel_fun.set_bright(lcd->disp,
					       disp_lcd_bright_get_adjust_value
					       (lcd, bright));
	}

	return DIS_SUCCESS;
}

s32 disp_lcd_get_bright(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	return lcdp->lcd_cfg.backlight_bright;
}

static s32 disp_lcd_set_bright_dimming(struct disp_device *lcd, u32 dimming)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	u32 bl = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	dimming = dimming > 256 ? 256 : dimming;
	lcdp->lcd_cfg.backlight_dimming = dimming;
	bl = disp_lcd_get_bright(lcd);
	disp_lcd_set_bright(lcd, bl);

	return DIS_SUCCESS;
}

static s32 disp_lcd_get_panel_info(struct disp_device *lcd,
				   struct disp_panel_para *info)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	memcpy(info, (struct disp_panel_para *) (&(lcdp->panel_info)),
	       sizeof(struct disp_panel_para));
	return 0;
}


static s32 disp_lcd_get_mode(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	if (lcdp->panel_info.convert_info.is_enabled)
		return lcdp->panel_info.convert_info.cur_mode;
	else
		return 0;
}
#if defined(__LINUX_PLAT__)
static irqreturn_t disp_lcd_event_proc(int irq, void *parg)
#else
static irqreturn_t disp_lcd_event_proc(void *parg)
#endif
{
	struct disp_device *lcd = (struct disp_device *)parg;
	struct disp_lcd_private_data *lcdp = NULL;
	struct disp_manager *mgr = NULL;
#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	struct disp_eink_manager *eink_manager = NULL;
#endif
	u32 hwdev_index;
	u32 irq_flag = 0;
	unsigned int panel_extend_dirty;
	unsigned long flags;

	curr_irq_count++;

	if (lcd == NULL)
		return DISP_IRQ_RETURN;

	/* Init real fps data. */
	DISP_DO_ONCE(disp_lcd_get_real_fps(lcd));

	hwdev_index = lcd->hwdev_index;
	lcdp = disp_lcd_get_priv(lcd);

	if (lcdp == NULL)
		return DISP_IRQ_RETURN;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
	eink_manager = disp_get_eink_manager(0);
	if (eink_manager == NULL)
		return DISP_IRQ_RETURN;
#endif

	if (disp_al_lcd_query_irq
	    (hwdev_index, LCD_IRQ_TCON0_VBLK, &lcdp->panel_info)) {
#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)

		eink_display_one_frame(eink_manager);
#else
		int cur_line =
		    disp_al_lcd_get_cur_line(hwdev_index, &lcdp->panel_info);
		int start_delay =
		    disp_al_lcd_get_start_delay(hwdev_index, &lcdp->panel_info);
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
		if (lcdp->lcd_panel_fun.esd_check &&
		    lcdp->lcd_panel_fun.reset_panel) {
			++lcdp->esd_inf.cnt;
			if (cur_line < 2 &&
			    !atomic_read(&lcdp->lcd_resetting) &&
			    lcdp->esd_inf.cnt >= lcdp->esd_inf.freq) {
				if (!lcdp->esd_inf.esd_check_func_pos ||
				    lcdp->lcd_panel_fun.esd_check(lcd->disp)) {
					/* request reset */
					atomic_set(&lcdp->lcd_resetting, 1);
					schedule_work(&lcdp->reflush_work);
				}
				lcdp->esd_inf.cnt = 0;
			}
		}
#endif

		mgr = lcd->manager;
		if (mgr == NULL)
			return DISP_IRQ_RETURN;

		if (cur_line <= (start_delay - lcdp->judge_line))
			sync_event_proc(mgr->disp, false);
		else
			sync_event_proc(mgr->disp, true);
#endif
	} else {
		irq_flag = disp_al_lcd_query_irq(hwdev_index, LCD_IRQ_TCON0_CNTR,
						&lcdp->panel_info);
		irq_flag |=
		    disp_al_lcd_query_irq(hwdev_index, LCD_IRQ_TCON0_TRIF,
					  &lcdp->panel_info);

		if (irq_flag == 0)
			goto exit;

		if (disp_al_lcd_tri_busy(hwdev_index, &lcdp->panel_info)) {
			/* if lcd is still busy when tri/cnt irq coming,
			 * take it as failture, record failture times,
			 * when it reach 2 times, clear counter
			 */
			lcdp->tri_finish_fail++;
			lcdp->tri_finish_fail = (lcdp->tri_finish_fail == 2) ?
			    0 : lcdp->tri_finish_fail;
		} else
			lcdp->tri_finish_fail = 0;

		mgr = lcd->manager;
		if (mgr == NULL)
			return DISP_IRQ_RETURN;

#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
		if (lcdp->lcd_panel_fun.esd_check &&
		    lcdp->lcd_panel_fun.reset_panel) {
			++lcdp->esd_inf.cnt;
			if (!atomic_read(&lcdp->lcd_resetting) &&
			    lcdp->esd_inf.cnt >= lcdp->esd_inf.freq) {
				if (!lcdp->esd_inf.esd_check_func_pos ||
				    lcdp->lcd_panel_fun.esd_check(lcd->disp)) {
					/* request reset */
					atomic_set(&lcdp->lcd_resetting, 1);
					schedule_work(&lcdp->reflush_work);
				}
				lcdp->esd_inf.cnt = 0;
			}
		}
#endif

		if (lcdp->tri_finish_fail == 0) {
			sync_event_proc(mgr->disp, false);
			disp_al_lcd_tri_start(hwdev_index, &lcdp->panel_info);
		} else
			sync_event_proc(mgr->disp, true);
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	panel_extend_dirty = lcdp->panel_extend_dirty;
	lcdp->panel_extend_dirty = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	if (panel_extend_dirty == 1)
		disp_al_lcd_cfg_ext(lcd->disp, &lcdp->panel_extend_info_set);

exit:
	return DISP_IRQ_RETURN;
}

#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
static void disp_lcd_reflush_work(struct work_struct *work)
{
	struct disp_lcd_private_data *lcdp =
	    container_of(work, struct disp_lcd_private_data, reflush_work);
	struct disp_device *lcd = disp_device_get_from_priv((void *)lcdp);

	if (!lcdp || !lcd) {
		DE_WARN("lcdp is null\n");
		return;
	}

	/* lcd is not enabled or is enabling */
	if (disp_lcd_is_enabled(lcd) == 0 || lcdp->enabling == 1)
		return;

	/* lcd is resetting */
	if (atomic_read(&lcdp->lcd_resetting) == 2)
		return;

	if (!lcdp->esd_inf.esd_check_func_pos)
		if (lcdp->lcd_panel_fun.esd_check)
			if (!lcdp->lcd_panel_fun.esd_check(lcd->disp)) {
				atomic_set(&lcdp->lcd_resetting, 0);
				return; /* everything is just fine */
			}

	if (lcdp->esd_inf.level == 2) {
		atomic_set(&lcdp->lcd_resetting, 2);
		disp_check_timing_param(&lcdp->panel_info);
		if (!(suspend_status & DISPLAY_BLANK)) {
			bsp_disp_device_switch(0, DISP_OUTPUT_TYPE_NONE,
				(enum disp_output_type)DISP_TV_MOD_1080P_60HZ);
			disp_delay_ms(300);
			bsp_disp_device_switch(0, DISP_OUTPUT_TYPE_LCD,
					(enum disp_output_type)DISP_TV_MOD_1080P_60HZ);

			++lcdp->esd_inf.rst_cnt;
		}
	} else {
		if (lcdp->esd_inf.level == 1) {
			atomic_set(&lcdp->lcd_resetting, 2);
			disp_lcd_disable(lcd);
		} else
			atomic_set(&lcdp->lcd_resetting, 2);
		++lcdp->esd_inf.rst_cnt;
		if (lcdp->lcd_panel_fun.reset_panel)
			lcdp->lcd_panel_fun.reset_panel(lcd->disp);
		if (lcdp->esd_inf.level == 1) {
			disp_lcd_enable(lcd);
		}
	}
	/* lcd reset finish */
	atomic_set(&lcdp->lcd_resetting, 0);
}
#endif

static s32 disp_lcd_cal_fps(struct disp_device *lcd)
{
	s32 ret = -1;
	struct disp_lcd_private_data *lcdp = NULL;
	struct disp_panel_para *panel_info = NULL;

	if (!lcd)
		 goto OUT;
	lcdp = disp_lcd_get_priv(lcd);
	if (!lcdp)
		goto OUT;
	panel_info = &lcdp->panel_info;


	lcdp->frame_per_sec =
	    DIV_ROUND_CLOSEST(panel_info->lcd_dclk_freq * 1000000 *
				  (panel_info->lcd_interlace + 1),
			      panel_info->lcd_ht * panel_info->lcd_vt);

	ret = 0;
OUT:
	return ret;
}

/* lcd enable except for backlight */
static s32 disp_lcd_fake_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i, ret;
	struct disp_manager *mgr = NULL;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	DE_INFO("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("mgr is NULL\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_enabled(lcd) == 1)
		return 0;

	disp_lcd_cal_fps(lcd);
	if (mgr->enable)
		mgr->enable(mgr);

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if ((lcdp->panel_info.lcd_if == LCD_IF_DSI) &&
	    (lcdp->irq_no_dsi != 0)) {
		if (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE
			|| lcdp->panel_info.lcd_tcon_mode == DISP_TCON_DUAL_DSI) {
			disp_sys_register_irq(lcdp->irq_no, 0,
					      disp_lcd_event_proc, (void *)lcd,
					      0, 0);
			disp_sys_enable_irq(lcdp->irq_no);
		} else {
			disp_sys_register_irq(lcdp->irq_no_dsi, 0,
					      disp_lcd_event_proc, (void *)lcd,
					      0, 0);
			disp_sys_enable_irq(lcdp->irq_no_dsi);
		}
	} else
#endif
	{
		disp_sys_register_irq(lcdp->irq_no, 0, disp_lcd_event_proc,
				      (void *)lcd, 0, 0);
		disp_sys_enable_irq(lcdp->irq_no);
	}
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabling = 1;
	lcdp->bl_need_enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	disp_lcd_gpio_init(lcd);
	lcd_clk_enable(lcd, false);
	ret = cal_real_frame_period(lcd);
	if (ret)
		DE_WARN("cal_real_frame_period fail:%d\n", ret);
	if (lcdp->panel_info.lcd_pwm_used && !lcdp->pwm_info.dev) {
		if (!lcdp->panel_info.lcd_pwm_name)
			lcdp->pwm_info.dev =
				disp_sys_pwm_request(lcdp->panel_info.lcd_pwm_ch);
		else
			lcdp->pwm_info.dev =
				disp_sys_pwm_get(lcdp->panel_info.lcd_pwm_name);
	}
	disp_sys_pwm_config(lcdp->pwm_info.dev, lcdp->pwm_info.duty_ns,
			    lcdp->pwm_info.period_ns);
	disp_sys_pwm_set_polarity(lcdp->pwm_info.dev, lcdp->pwm_info.polarity);
	disp_check_timing_param(&lcdp->panel_info);
	disp_al_lcd_cfg(lcd->hwdev_index, &lcdp->panel_info,
		&lcdp->panel_extend_info_set);
	lcdp->open_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_open_flow)
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->disp);
	else
		DE_WARN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->disp);

	for (i = 0; i < lcdp->open_flow.func_num - 1; i++) {
		if (lcdp->open_flow.func[i].func) {
			lcdp->open_flow.func[i].func(lcd->disp);
			DE_INFO("open flow:step %d finish, to delay %d\n", i,
			       lcdp->open_flow.func[i].delay);
			if (lcdp->open_flow.func[i].delay != 0)
				disp_delay_ms(lcdp->open_flow.func[i].delay);
		}
	}
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	atomic_set(&lcdp->lcd_resetting, 0);
#endif

	return 0;
}

static int skip_open_backlight;
#ifndef MODULE
static int __init early_bootreason(char *str)
{
	skip_open_backlight = 0;
	if (!str) {
		skip_open_backlight = 0;
		goto OUT;
	}

	if (parse_option_str("irq", str) == true) {
		skip_open_backlight = 1;
		DE_WARN("get irq bootreason:%s\n", str);
	} else {
		skip_open_backlight = 0;
	}

OUT:
	return 0;
}
early_param("bootreason", early_bootreason);
#endif

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
static s32 disp_lcd_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_manager *mgr = NULL;
	int ret = 0;
	int i;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	if (!disp_lcd_is_used(lcd)) {
		return ret;
	}

	flush_work(&lcd->close_eink_panel_work);
	DE_INFO("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("mgr is NULL\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_enabled(lcd) == 1)
		return 0;

	disp_sys_register_irq(lcdp->irq_no, 0, disp_lcd_event_proc, (void *)lcd,
			      0, 0);
	disp_sys_enable_irq(lcdp->irq_no);

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabling = 1;
	lcdp->bl_need_enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	if (lcdp->lcd_panel_fun.cfg_panel_info)
		lcdp->lcd_panel_fun.cfg_panel_info(&lcdp->panel_extend_info);
	else
		DE_WARN("lcd_panel_fun[%d].cfg_panel_info is NULL\n", lcd->disp);

	disp_lcd_gpio_init(lcd);
	ret = lcd_clk_enable(lcd, false);
	if (ret != 0)
		return DIS_FAIL;
	ret = cal_real_frame_period(lcd);
	if (ret)
		DE_WARN("cal_real_frame_period fail:%d\n", ret);
	disp_check_timing_param(&lcdp->panel_info);
	disp_al_lcd_cfg(lcd->hwdev_index, &lcdp->panel_info,
			&lcdp->panel_extend_info);/* init tcon_lcd regs */
	lcdp->open_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_open_flow)
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->disp);
	else
		DE_WARN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->disp);

	for (i = 0; i < lcdp->open_flow.func_num; i++) {
		if (lcdp->open_flow.func[i].func) {
			lcdp->open_flow.func[i].func(lcd->disp);
			DE_INFO("open flow:step %d finish, to delay %d\n", i,
			       lcdp->open_flow.func[i].delay);
			if (lcdp->open_flow.func[i].delay != 0)
				disp_delay_ms(lcdp->open_flow.func[i].delay);
		}
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}

static s32 disp_lcd_disable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_manager *mgr = NULL;
	int i;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	DE_INFO("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("mgr is NULL\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_enabled(lcd) == 0)
		return 0;

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	lcdp->bl_need_enabled = 0;
	lcdp->close_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_close_flow)
		lcdp->lcd_panel_fun.cfg_close_flow(lcd->disp);
	else
		DE_WARN("lcd_panel_fun[%d].cfg_close_flow is NULL\n", lcd->disp);

	for (i = 0; i < lcdp->close_flow.func_num; i++) {
		if (lcdp->close_flow.func[i].func) {
			lcdp->close_flow.func[i].func(lcd->disp);
			DE_INFO("close flow:step %d finish, to delay %d\n", i,
			       lcdp->close_flow.func[i].delay);
			if (lcdp->close_flow.func[i].delay != 0)
				disp_delay_ms(lcdp->close_flow.func[i].delay);
		}
	}

	lcd_clk_disable(lcd);
	disp_lcd_gpio_exit(lcd);

	disp_sys_disable_irq(lcdp->irq_no);
	disp_sys_unregister_irq(lcdp->irq_no, disp_lcd_event_proc, (void *)lcd);

	return 0;
}

#else
static s32 disp_lcd_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i;
	struct disp_manager *mgr = NULL;
	unsigned bl;
	int ret = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	if (!disp_lcd_is_used(lcd)) {
		return ret;
	}

	DE_INFO("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("mgr is NULL\n");
		return DIS_FAIL;
	}

	if (disp_lcd_is_enabled(lcd) == 1)
		return 0;

	disp_lcd_cal_fps(lcd);
	if (mgr->enable)
		mgr->enable(mgr);

#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if ((lcdp->panel_info.lcd_if == LCD_IF_DSI)
	    && (lcdp->irq_no_dsi != 0)) {
		if (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE
			|| lcdp->panel_info.lcd_tcon_mode == DISP_TCON_DUAL_DSI) {
			disp_sys_register_irq(lcdp->irq_no, 0, disp_lcd_event_proc,
					      (void *)lcd, 0, 0);
			disp_sys_enable_irq(lcdp->irq_no);
		} else {
			disp_sys_register_irq(lcdp->irq_no_dsi, 0, disp_lcd_event_proc,
					      (void *)lcd, 0, 0);
			disp_sys_enable_irq(lcdp->irq_no_dsi);
		}
	} else
#endif
	{
		disp_sys_register_irq(lcdp->irq_no, 0, disp_lcd_event_proc,
				      (void *)lcd, 0, 0);
		disp_sys_enable_irq(lcdp->irq_no);
	}
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabling = 1;
	lcdp->bl_need_enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	lcdp->panel_extend_info.lcd_gamma_en = lcdp->panel_info.lcd_gamma_en;
	disp_lcd_gpio_init(lcd);
	ret = lcd_clk_enable(lcd, false);
	if (ret != 0)
		return DIS_FAIL;
	ret = cal_real_frame_period(lcd);
	if (ret)
		DE_WARN("cal_real_frame_period fail:%d\n", ret);

	if (lcdp->panel_info.lcd_pwm_used && !lcdp->pwm_info.dev) {
		if (!lcdp->panel_info.lcd_pwm_name)
			lcdp->pwm_info.dev =
				disp_sys_pwm_request(lcdp->panel_info.lcd_pwm_ch);
		else
			lcdp->pwm_info.dev =
				disp_sys_pwm_get(lcdp->panel_info.lcd_pwm_name);
	}
	disp_sys_pwm_config(lcdp->pwm_info.dev, lcdp->pwm_info.duty_ns,
			    lcdp->pwm_info.period_ns);
	disp_sys_pwm_set_polarity(lcdp->pwm_info.dev, lcdp->pwm_info.polarity);
	disp_check_timing_param(&lcdp->panel_info);
	disp_al_lcd_cfg(lcd->hwdev_index, &lcdp->panel_info,
		&lcdp->panel_extend_info_set);
	lcd_calc_judge_line(lcd);
	lcdp->open_flow.func_num = 0;
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	if (lcdp->lcd_panel_fun.set_esd_info) {
		lcdp->lcd_panel_fun.set_esd_info(&lcdp->esd_inf);
	} else {
		/* default value */
		lcdp->esd_inf.level = 0;
		lcdp->esd_inf.freq = 60;
		lcdp->esd_inf.esd_check_func_pos = 0;
		lcdp->esd_inf.cnt = 0;
	}
#endif
	if (lcdp->lcd_panel_fun.cfg_open_flow)
		lcdp->lcd_panel_fun.cfg_open_flow(lcd->disp);
	else
		DE_WARN("lcd_panel_fun[%d].cfg_open_flow is NULL\n", lcd->disp);

	if (lcdp->panel_info.lcd_gsensor_detect == 0)
		skip_open_backlight = 0;

	for (i = 0; i < lcdp->open_flow.func_num - skip_open_backlight; i++) {
		if (lcdp->open_flow.func[i].func) {
			lcdp->open_flow.func[i].func(lcd->disp);
			DE_INFO("open flow:step %d finish, to delay %d\n", i,
			       lcdp->open_flow.func[i].delay);
			if (lcdp->open_flow.func[i].delay != 0)
				disp_delay_ms(lcdp->open_flow.func[i].delay);
		}
	}
	skip_open_backlight = 0; /* only skip one time */

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);
	bl = disp_lcd_get_bright(lcd);
	disp_lcd_set_bright(lcd, bl);
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	atomic_set(&lcdp->lcd_resetting, 0);
#endif

	return 0;
}

static s32 disp_lcd_disable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_manager *mgr = NULL;
	int i;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}
	DE_INFO("lcd %d\n", lcd->disp);
	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("mgr is NULL\n");
		return DIS_FAIL;
	}
	if (disp_lcd_is_enabled(lcd) == 0)
		return 0;

#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	atomic_set(&lcdp->lcd_resetting, 2);
#endif
	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 0;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	lcdp->bl_need_enabled = 0;
	lcdp->close_flow.func_num = 0;
	if (lcdp->lcd_panel_fun.cfg_close_flow)
		lcdp->lcd_panel_fun.cfg_close_flow(lcd->disp);
	else
		DE_WARN("lcd_panel_fun[%d].cfg_close_flow is NULL\n", lcd->disp);

	for (i = 0; i < lcdp->close_flow.func_num; i++) {
		if (lcdp->close_flow.func[i].func) {
			lcdp->close_flow.func[i].func(lcd->disp);
			DE_INFO("close flow:step %d finish, to delay %d\n", i,
			       lcdp->close_flow.func[i].delay);
			if (lcdp->close_flow.func[i].delay != 0)
				disp_delay_ms(lcdp->close_flow.func[i].delay);
		}
	}

	disp_lcd_gpio_exit(lcd);

	lcd_clk_disable(lcd);
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if ((lcdp->panel_info.lcd_if == LCD_IF_DSI) &&
	    (lcdp->irq_no_dsi != 0)) {
		if (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE
			|| lcdp->panel_info.lcd_tcon_mode == DISP_TCON_DUAL_DSI) {
			disp_sys_disable_irq(lcdp->irq_no);
			disp_sys_unregister_irq(
			    lcdp->irq_no, disp_lcd_event_proc, (void *)lcd);
		} else {
			disp_sys_disable_irq(lcdp->irq_no_dsi);
			disp_sys_unregister_irq(
			    lcdp->irq_no_dsi, disp_lcd_event_proc, (void *)lcd);
		}
	} else
#endif
	{
		disp_sys_disable_irq(lcdp->irq_no);
		disp_sys_unregister_irq(lcdp->irq_no, disp_lcd_event_proc,
					(void *)lcd);
	}

	if (mgr->disable)
		mgr->disable(mgr);

	return 0;
}
#endif
static s32 disp_lcd_sw_enable(struct disp_device *lcd)
{
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i, ret;
	struct disp_manager *mgr = NULL;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	mgr = lcd->manager;
	if (mgr == NULL) {
		DE_WARN("mgr is NULL\n");
		return DIS_FAIL;
	}
	disp_lcd_cal_fps(lcd);
	lcd_calc_judge_line(lcd);
	if (mgr->sw_enable)
		mgr->sw_enable(mgr);

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (lcd_clk_enable(lcd, true) != 0)
		return DIS_FAIL;
#endif
	ret = cal_real_frame_period(lcd);
	if (ret)
		DE_WARN("cal_real_frame_period fail:%d\n", ret);

	/* init lcd power */
	for (i = 0; i < LCD_POWER_NUM; i++) {
		ret = disp_sys_power_enable(lcdp->lcd_cfg.regulator[i]);
		if (ret)
			return DIS_FAIL;
	}

	/* init gpio */
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		ret = disp_sys_power_enable(lcdp->lcd_cfg.gpio_regulator[i]);
		if (ret)
			return DIS_FAIL;
	}

	/* init lcd pin */
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		ret = disp_sys_power_enable(lcdp->lcd_cfg.pin_regulator[i]);
		if (ret)
			return DIS_FAIL;
	}

	/* init bl */
	if (lcdp->lcd_cfg.lcd_bl_en_used) {
		ret = disp_sys_power_enable(lcdp->lcd_cfg.bl_regulator);
		if (ret)
			return DIS_FAIL;
		disp_sys_gpio_request(&lcdp->lcd_cfg.lcd_bl_en);
	}

	if (lcdp->panel_info.lcd_pwm_used) {
		if (!lcdp->pwm_info.dev) {
			if (!lcdp->panel_info.lcd_pwm_name)
				lcdp->pwm_info.dev =
					disp_sys_pwm_request(lcdp->panel_info.lcd_pwm_ch);
			else
				lcdp->pwm_info.dev =
					disp_sys_pwm_get(lcdp->panel_info.lcd_pwm_name);
		}
		if (lcdp->pwm_info.dev) {
			disp_sys_pwm_config(lcdp->pwm_info.dev,
					    lcdp->pwm_info.duty_ns,
					    lcdp->pwm_info.period_ns);
			disp_sys_pwm_set_polarity(lcdp->pwm_info.dev,
						  lcdp->pwm_info.polarity);
			disp_sys_pwm_enable(lcdp->pwm_info.dev);
		}
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->enabled = 1;
	lcdp->enabling = 0;
	lcdp->bl_need_enabled = 1;
	lcdp->bl_enabled = true;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
	if (lcdp->lcd_panel_fun.set_esd_info) {
		lcdp->lcd_panel_fun.set_esd_info(&lcdp->esd_inf);
	} else {
		/* default value */
		lcdp->esd_inf.level = 0;
		lcdp->esd_inf.freq = 60;
		lcdp->esd_inf.esd_check_func_pos = 0;
		lcdp->esd_inf.cnt = 0;
	}
#endif
	disp_al_lcd_disable_irq(lcd->hwdev_index, LCD_IRQ_TCON0_VBLK,
				&lcdp->panel_info);
#if defined(SUPPORT_DSI) && defined(DSI_VERSION_40)
	if ((lcdp->panel_info.lcd_if == LCD_IF_DSI) &&
	    (lcdp->irq_no_dsi != 0)) {
		if (lcdp->panel_info.lcd_dsi_if == LCD_DSI_IF_COMMAND_MODE
			|| lcdp->panel_info.lcd_tcon_mode == DISP_TCON_DUAL_DSI) {
			disp_sys_register_irq(lcdp->irq_no, 0,
					      disp_lcd_event_proc, (void *)lcd,
					      0, 0);
			disp_sys_enable_irq(lcdp->irq_no);
		} else {
			disp_sys_register_irq(lcdp->irq_no_dsi, 0,
					      disp_lcd_event_proc, (void *)lcd,
					      0, 0);
			disp_sys_enable_irq(lcdp->irq_no_dsi);
		}
	} else
#endif
	{
		disp_sys_register_irq(lcdp->irq_no, 0, disp_lcd_event_proc,
				      (void *)lcd, 0, 0);
		disp_sys_enable_irq(lcdp->irq_no);
	}
	disp_al_lcd_enable_irq(lcd->hwdev_index, LCD_IRQ_TCON0_VBLK,
			       &lcdp->panel_info);

	return 0;
}

s32 disp_lcd_is_enabled(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	return (s32)lcdp->enabled;
}

/**
 * disp_lcd_check_if_enabled - check lcd if be enabled status
 *
 * this function only be used by bsp_disp_sync_with_hw to check
 * the device enabled status when driver init
 */
s32 disp_lcd_check_if_enabled(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int ret = 1;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

#if !defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT)
	if (lcdp->clk_tcon_lcd &&
	   (__clk_is_enabled(lcdp->clk_tcon_lcd) == 0))
		ret = 0;
#endif

	return ret;
}

static s32 disp_lcd_set_open_func(struct disp_device *lcd, LCD_FUNC func,
				  u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	if (func) {
		lcdp->open_flow.func[lcdp->open_flow.func_num].func = func;
		lcdp->open_flow.func[lcdp->open_flow.func_num].delay = delay;
		lcdp->open_flow.func_num++;
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_set_close_func(struct disp_device *lcd, LCD_FUNC func,
				   u32 delay)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	if (func) {
		lcdp->close_flow.func[lcdp->close_flow.func_num].func = func;
		lcdp->close_flow.func[lcdp->close_flow.func_num].delay = delay;
		lcdp->close_flow.func_num++;
	}

	return DIS_SUCCESS;
}

static s32 disp_lcd_set_panel_funs(struct disp_device *lcd, char *name,
				   struct disp_lcd_panel_fun *lcd_cfg)
{
	s32 ret = -1;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}


	DE_INFO("lcd %d, driver_name %s,  panel_name %s\n", lcd->disp, lcdp->panel_info.lcd_driver_name,
	       name);
	if (!strcmp(lcdp->panel_info.lcd_driver_name, name)) {
		memset(&lcdp->lcd_panel_fun,
		       0,
		       sizeof(struct disp_lcd_panel_fun));
		lcdp->lcd_panel_fun.cfg_panel_info = lcd_cfg->cfg_panel_info;
		lcdp->lcd_panel_fun.cfg_open_flow = lcd_cfg->cfg_open_flow;
		lcdp->lcd_panel_fun.cfg_close_flow = lcd_cfg->cfg_close_flow;
		lcdp->lcd_panel_fun.esd_check = lcd_cfg->esd_check;
		lcdp->lcd_panel_fun.reset_panel = lcd_cfg->reset_panel;
		lcdp->lcd_panel_fun.set_esd_info = lcd_cfg->set_esd_info;
		lcdp->lcd_panel_fun.lcd_user_defined_func =
		    lcd_cfg->lcd_user_defined_func;
		lcdp->lcd_panel_fun.set_bright = lcd_cfg->set_bright;
		lcdp->lcd_panel_fun.get_panel_para_mapping = lcd_cfg->get_panel_para_mapping;
		if (lcdp->lcd_panel_fun.cfg_panel_info) {
			lcdp->lcd_panel_fun.cfg_panel_info(&lcdp->panel_extend_info);
			memcpy(&lcdp->panel_extend_info_set,
				&lcdp->panel_extend_info, sizeof(struct panel_extend_para));
		} else {
			DE_WARN("lcd_panel_fun[%d].cfg_panel_info is NULL\n", lcd->disp);
		}

		ret = 0;
	}

	return ret;
}

s32 disp_lcd_gpio_init(struct disp_device *lcd)
{
	int ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	/* io-pad */
	for (i = 0; i < LCD_GPIO_REGU_NUM; i++) {
		ret = disp_sys_power_enable(lcdp->lcd_cfg.gpio_regulator[i]);
		if (ret)
			return DIS_FAIL;
	}


	return 0;
}

s32 disp_lcd_gpio_exit(struct disp_device *lcd)
{
	int ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}


	/* io-pad */
	for (i = LCD_GPIO_REGU_NUM - 1; i >= 0; i--) {
		ret = disp_sys_power_disable(lcdp->lcd_cfg.gpio_regulator[i]);
		if (ret)
			return DIS_FAIL;
	}

	return DIS_SUCCESS;
}

/* direction: input(0), output(1) */
s32 disp_lcd_gpio_set_direction(struct disp_device *lcd, u32 io_index,
				u32 direction)
{
	int gpio, value, ret;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];
	struct device_node *node = g_disp_drv.node;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);

	gpio = of_get_named_gpio(node, gpio_name, 0);
	if (!gpio_is_valid(gpio)) {
		DE_WARN("of_get_named_gpio_flags for %s failed\n", gpio_name);
		return DIS_FAIL;
	}

	if (direction) {
		value = __gpio_get_value(gpio);
		ret = gpio_direction_output(gpio, value);
		if (ret) {
			DE_WARN("gpio_direction_output for %s failed\n", gpio_name);
			return DIS_FAIL;
		}
	} else {
		ret = gpio_direction_input(gpio);
		if (ret) {
			DE_WARN("gpio_direction_input for %s failed\n", gpio_name);
			return DIS_FAIL;
		}
	}

	return DIS_SUCCESS;
}

s32 disp_lcd_gpio_get_value(struct disp_device *lcd, u32 io_index)
{
	int gpio;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];
	struct device_node *node = g_disp_drv.node;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	sprintf(gpio_name, "lcd_gpio_%d", io_index);

	gpio = of_get_named_gpio(node, gpio_name, 0);
	if (!gpio_is_valid(gpio)) {
		DE_WARN("of_get_named_gpio_flags for %s failed\n", gpio_name);
		return DIS_FAIL;
	}

	return __gpio_get_value(gpio);
}

s32 disp_lcd_gpio_set_value(struct disp_device *lcd, u32 io_index, u32 data)
{
	int gpio;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	char gpio_name[20];

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	if (io_index >= LCD_GPIO_NUM) {
		DE_WARN("gpio num out of range\n");
		return DIS_FAIL;
	}
	sprintf(gpio_name, "lcd_gpio_%d", io_index);

	gpio = lcdp->lcd_cfg.lcd_gpio[io_index].gpio;
	if (!gpio_is_valid(gpio)) {
		DE_WARN("of_get_named_gpio_flags for %s failed\n", gpio_name);
		return DIS_FAIL;
	}

	__gpio_set_value(gpio, data);

	return DIS_SUCCESS;
}

static s32 disp_lcd_get_dimensions(struct disp_device *lcd, u32 *width,
				   u32 *height)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	*width = lcdp->panel_info.lcd_width;
	*height = lcdp->panel_info.lcd_height;
	return 0;
}

static s32 disp_lcd_get_status(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	return disp_al_lcd_get_status(lcd->hwdev_index, &lcdp->panel_info);
}

static bool disp_lcd_is_in_safe_period(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int start_delay;
	int cur_line;
	bool ret = true;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		goto exit;
	}

	start_delay =
	    disp_al_lcd_get_start_delay(lcd->hwdev_index, &lcdp->panel_info);
	cur_line =
	    disp_al_lcd_get_cur_line(lcd->hwdev_index, &lcdp->panel_info);

	if (cur_line >= start_delay)
		ret = false;

exit:
	return ret;
}

static const unsigned int WARM_COLOR_8B[151][2] = {
    {197, 124}, {198, 125}, {198, 126}, {199, 127}, {199, 128}, {200, 128},
    {200, 129}, {201, 130}, {201, 131}, {202, 132}, {202, 133}, {203, 134},
    {203, 135}, {204, 136}, {204, 137}, {205, 138}, {205, 138}, {206, 139},
    {206, 140}, {207, 141}, {207, 142}, {208, 143}, {208, 144}, {209, 145},
    {209, 146}, {209, 147}, {210, 148}, {210, 148}, {211, 149}, {211, 150},
    {212, 151}, {212, 152}, {213, 153}, {213, 154}, {214, 155}, {214, 156},
    {214, 157}, {215, 157}, {215, 158}, {216, 159}, {216, 160}, {217, 161},
    {217, 162}, {218, 163}, {218, 164}, {218, 165}, {219, 166}, {219, 167},
    {220, 167}, {220, 168}, {221, 169}, {221, 170}, {221, 171}, {222, 172},
    {222, 173}, {223, 174}, {223, 175}, {223, 176}, {224, 176}, {224, 177},
    {225, 178}, {225, 179}, {225, 180}, {226, 181}, {226, 182}, {227, 183},
    {227, 184}, {228, 185}, {228, 185}, {228, 186}, {229, 187}, {229, 188},
    {229, 189}, {230, 190}, {230, 191}, {231, 192}, {231, 193}, {231, 193},
    {232, 194}, {232, 195}, {233, 196}, {233, 197}, {233, 198}, {234, 199},
    {234, 200}, {234, 201}, {235, 201}, {235, 202}, {236, 203}, {236, 204},
    {236, 205}, {237, 206}, {237, 207}, {237, 208}, {238, 208}, {238, 209},
    {238, 210}, {239, 211}, {239, 212}, {240, 213}, {240, 214}, {240, 215},
    {241, 215}, {241, 216}, {241, 217}, {242, 218}, {242, 219}, {242, 220},
    {243, 221}, {243, 221}, {243, 222}, {244, 223}, {244, 224}, {244, 225},
    {245, 226}, {245, 227}, {245, 227}, {246, 228}, {246, 229}, {246, 230},
    {247, 231}, {247, 232}, {247, 233}, {248, 233}, {248, 234}, {248, 235},
    {249, 236}, {249, 237}, {249, 238}, {250, 238}, {250, 239}, {250, 240},
    {251, 241}, {251, 242}, {251, 243}, {252, 243}, {252, 244}, {252, 245},
    {252, 246}, {253, 247}, {253, 248}, {253, 248}, {254, 249}, {254, 250},
    {254, 251}, {255, 252}, {255, 253}, {255, 253}, {256, 254}, {256, 255},
    {256, 256},
};

static const unsigned int COOL_COLOR_8B[51][2] = {
    {256, 256}, {256, 256}, {255, 256}, {253, 255}, {251, 254}, {250, 253},
    {248, 252}, {246, 251}, {245, 250}, {243, 249}, {242, 248}, {240, 248},
    {239, 247}, {237, 246}, {236, 245}, {234, 244}, {233, 243}, {232, 242},
    {230, 242}, {229, 241}, {228, 240}, {226, 239}, {225, 238}, {224, 238},
    {223, 237}, {221, 236}, {220, 235}, {219, 235}, {218, 234}, {217, 233},
    {216, 233}, {215, 232}, {213, 231}, {212, 231}, {211, 230}, {210, 229},
    {209, 229}, {208, 228}, {207, 227}, {206, 227}, {205, 226}, {204, 225},
    {203, 225}, {202, 224}, {201, 224}, {200, 223}, {200, 222}, {199, 222},
    {198, 221}, {197, 221}, {196, 220},
};

static const unsigned int WARM_COLOR_10B[151][2] = {
    {788, 496}, {790, 500}, {792, 503}, {794, 507}, {796, 510}, {798, 514},
    {800, 518}, {802, 521}, {804, 525}, {806, 528}, {808, 532}, {810, 536},
    {812, 539}, {814, 543}, {816, 547}, {818, 550}, {820, 554}, {822, 557},
    {824, 561}, {826, 565}, {827, 568}, {829, 572}, {831, 576}, {833, 579},
    {835, 583}, {837, 586}, {839, 590}, {840, 594}, {842, 597}, {844, 601},
    {846, 605}, {848, 608}, {850, 612}, {851, 615}, {853, 619}, {855, 623},
    {857, 626}, {859, 630}, {860, 634}, {862, 637}, {864, 641}, {866, 644},
    {867, 648}, {869, 652}, {871, 655}, {873, 659}, {874, 663}, {876, 666},
    {878, 670}, {879, 673}, {881, 677}, {883, 681}, {884, 684}, {886, 688},
    {888, 691}, {889, 695}, {891, 699}, {893, 702}, {894, 706}, {896, 709},
    {898, 713}, {899, 717}, {901, 720}, {903, 724}, {904, 727}, {906, 731},
    {907, 735}, {909, 738}, {911, 742}, {912, 745}, {914, 749}, {915, 752},
    {917, 756}, {918, 760}, {920, 763}, {922, 767}, {923, 770}, {925, 774},
    {926, 777}, {928, 781}, {929, 784}, {931, 788}, {932, 792}, {934, 795},
    {935, 799}, {937, 802}, {938, 806}, {940, 809}, {941, 813}, {943, 816},
    {944, 820}, {946, 823}, {947, 827}, {949, 830}, {950, 834}, {951, 837},
    {953, 841}, {954, 844}, {956, 848}, {957, 851}, {959, 855}, {960, 858},
    {961, 862}, {963, 865}, {964, 869}, {966, 872}, {967, 875}, {968, 879},
    {970, 882}, {971, 886}, {973, 889}, {974, 893}, {975, 896}, {977, 900},
    {978, 903}, {979, 906}, {981, 910}, {982, 913}, {983, 917}, {985, 920},
    {986, 923}, {987, 927}, {989, 930}, {990, 934}, {991, 937}, {993, 940},
    {994, 944}, {995, 947}, {996, 951}, {998, 954}, {999, 957}, {1000, 961},
    {1001, 964}, {1003, 967}, {1004, 971}, {1005, 974}, {1007, 977}, {1008, 981},
    {1009, 984}, {1010, 987}, {1011, 991}, {1013, 994}, {1014, 997}, {1015, 1001},
    {1016, 1004}, {1018, 1007}, {1019, 1010}, {1020, 1014}, {1021, 1017}, {1022, 1020},
    {1024, 1024}
};

static const unsigned int COOL_COLOR_10B[51][2] = {
    {1024, 1024}, {1024, 1024}, {1019, 1024}, {1012, 1020}, {1005, 1017}, {998, 1013},
    {992, 1009}, {986, 1005}, {979, 1001}, {973, 998}, {967, 994}, {961, 990},
    {955, 987}, {949, 983}, {943, 980}, {938, 977}, {932, 973}, {927, 970},
    {921, 967}, {916, 963}, {911, 960}, {906, 957}, {901, 954}, {896, 951},
    {891, 948}, {886, 945}, {881, 942}, {876, 939}, {872, 936}, {867, 933},
    {863, 930}, {858, 928}, {854, 925}, {849, 922}, {845, 919}, {841, 917},
    {837, 914}, {833, 912}, {829, 909}, {825, 906}, {821, 904}, {817, 901},
    {813, 899}, {809, 897}, {806, 894}, {802, 892}, {798, 889}, {795, 887},
    {791, 885}, {788, 883}, {784, 880}
};

static s32 disp_lcd_update_gamma_tbl_set(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int i;
	unsigned int *gamma, *gamma_set;
	const unsigned int (*cur_warm)[2], (*cur_cool)[2];
	unsigned int r, g, b;
	s32 color_temperature;
	u32 color_inverse;
	u32 size;
	u32 *cur_mask;
	u32 *cur_shift;
	u32 mask[2][3] = {{0xff0000, 0xff00, 0xff}, /* R G B */
			  {0x3ff00000, 0xffc00, 0x3ff}};
	u32 shift[2][3] = {{16, 8, 0},
			   {20, 10, 0}};

	color_temperature = lcdp->color_temperature;
	color_inverse = lcdp->color_inverse;
	memcpy(&lcdp->panel_extend_info_set, &lcdp->panel_extend_info,
		sizeof(struct panel_extend_para));
	gamma = lcdp->panel_extend_info.lcd_gamma_tbl;
	gamma_set = lcdp->panel_extend_info_set.lcd_gamma_tbl;

	if (color_temperature < -50 || color_temperature > 150) {
		DE_WARN("color_temperature out of bound =%d", color_temperature);
		return -1;
	}

	if (!lcdp->panel_extend_info_set.is_lcd_gamma_tbl_10bit) {
		size = 256;
		cur_mask = &mask[0][0];
		cur_shift = &shift[0][0];
		cur_warm = WARM_COLOR_8B;
		cur_cool = COOL_COLOR_8B;
	} else {
		size = 1024;
		cur_mask = &mask[1][0];
		cur_shift = &shift[1][0];
		cur_warm = WARM_COLOR_10B;
		cur_cool = COOL_COLOR_10B;
	}

	if (color_temperature > 0) {
		/* warm color */
		for (i = 0; i < size; i++) {
			r = (gamma[i] & cur_mask[0]) >> cur_shift[0];
			g = (gamma[i] & cur_mask[1]) >> cur_shift[1];
			b = (gamma[i] & cur_mask[2]) >> cur_shift[2];

			g = (g * cur_warm[151 - color_temperature - 1][0]) / size;
			b = (b * cur_warm[151 - color_temperature - 1][1]) / size;

			r = (r << cur_shift[0]) & cur_mask[0];
			g = (g << cur_shift[1]) & cur_mask[1];
			b = (b << cur_shift[2]) & cur_mask[2];
			gamma_set[i] = r | g | b;
		}
	} else if (color_temperature < 0) {
		/* cool color */
		for (i = 0; i < size; i++) {
			r = (gamma[i] & cur_mask[0]) >> cur_shift[0];
			g = (gamma[i] & cur_mask[1]) >> cur_shift[1];
			b = (gamma[i] & cur_mask[2]) >> cur_shift[2];

			color_temperature = abs(color_temperature);
			r = (r * cur_cool[color_temperature][0]) / size;
			g = (g * cur_cool[color_temperature][1]) / size;

			r = (r << cur_shift[0]) & cur_mask[0];
			g = (g << cur_shift[1]) & cur_mask[1];
			b = (b << cur_shift[2]) & cur_mask[2];
			gamma_set[i] = r | g | b;
		}
	}

	if (color_inverse == 1) {
		for (i = 0; i < size; i++)
			gamma_set[i] = 0xffffffff -  gamma_set[i];
	}
	if (color_inverse != 0)
		lcdp->panel_extend_info_set.lcd_gamma_en = 1;
	if (color_temperature != 0)
		lcdp->panel_extend_info_set.lcd_gamma_en = 1;

	return 0;
}


static s32 disp_lcd_set_gamma_tbl(struct disp_device *lcd,
			unsigned int *gamma_table, unsigned int size)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((lcd == NULL) || (lcdp == NULL) || (lcd->manager == NULL)
	    || (gamma_table == NULL)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	if (size <= 256 * sizeof(unsigned int)) {
		size = (size > LCD_GAMMA_TABLE_SIZE) ?
			LCD_GAMMA_TABLE_SIZE : size;
	} else {
		size = (size > LCD_GAMMA_TABLE_SIZE_10BIT) ?
			LCD_GAMMA_TABLE_SIZE_10BIT : size;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (size <= 256 * sizeof(unsigned int)) {
		lcdp->panel_extend_info.is_lcd_gamma_tbl_10bit = false;
		memcpy(lcdp->panel_extend_info.lcd_gamma_tbl, gamma_table, size);
	} else {
		lcdp->panel_extend_info.is_lcd_gamma_tbl_10bit = true;
		memcpy(lcdp->panel_extend_info.lcd_gamma_tbl_10bit, gamma_table, size);
	}
	disp_lcd_update_gamma_tbl_set(lcd);
	if (disp_feat_is_using_rcq(lcd->manager->disp)) {
		disp_lcd_rcq_protect(lcd, true);
		disp_al_lcd_cfg_ext(lcd->disp, &lcdp->panel_extend_info_set);
		disp_lcd_rcq_protect(lcd, false);
	} else {
		lcdp->panel_extend_dirty = 1;
	}
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}

static s32 disp_lcd_get_gamma_tbl(struct disp_device *lcd,
			unsigned int *gamma_table, unsigned int size)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((lcd == NULL) || (lcdp == NULL)
	    || (gamma_table == NULL)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	if (size <= 256 * sizeof(unsigned int)) {
		size = (size > LCD_GAMMA_TABLE_SIZE) ?
			LCD_GAMMA_TABLE_SIZE : size;
	} else {
		size = (size > LCD_GAMMA_TABLE_SIZE_10BIT) ?
			LCD_GAMMA_TABLE_SIZE_10BIT : size;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (size <= 256 * sizeof(unsigned int)) {
		memcpy(gamma_table, lcdp->panel_extend_info.lcd_gamma_tbl, size);
	} else {
		memcpy(gamma_table, lcdp->panel_extend_info.lcd_gamma_tbl_10bit, size);
	}
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}
static s32 disp_lcd_enable_gamma(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((lcd == NULL) || (lcdp == NULL) || (lcd->manager == NULL)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	if (lcdp->panel_extend_info.lcd_gamma_en == 0) {
		lcdp->panel_extend_info.lcd_gamma_en = 1;
		disp_lcd_update_gamma_tbl_set(lcd);
		if (disp_feat_is_using_rcq(lcd->manager->disp)) {
			disp_lcd_rcq_protect(lcd, true);
			disp_al_lcd_cfg_ext(lcd->disp, &lcdp->panel_extend_info_set);
			disp_lcd_rcq_protect(lcd, false);
		} else {
			lcdp->panel_extend_dirty = 1;
		}
	}
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}

static s32 disp_lcd_disable_gamma(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	int ret;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	if (lcdp->panel_extend_info.lcd_gamma_en == 1) {
		lcdp->panel_extend_info.lcd_gamma_en = 0;
		ret = disp_al_lcd_cfg_ext(lcd->disp,
					  &lcdp->panel_extend_info);
	} else {
		ret = 0;
	}

	return ret;
}

static s32 disp_lcd_get_fps(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	return lcdp->frame_per_sec;
}

static s32 disp_lcd_set_color_temperature(struct disp_device *lcd,
	s32 color_temperature)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;

	if ((NULL == lcd) || (NULL == lcdp) || (NULL == lcd->manager)) {
		DE_WARN("NULL hdl\n");
		return -1;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	lcdp->color_temperature = color_temperature;
	disp_lcd_update_gamma_tbl_set(lcd);
	if (disp_feat_is_using_rcq(lcd->manager->disp)) {
		disp_lcd_rcq_protect(lcd, true);
		disp_al_lcd_cfg_ext(lcd->disp, &lcdp->panel_extend_info_set);
		disp_lcd_rcq_protect(lcd, false);
	} else {
		lcdp->panel_extend_dirty = 1;
	}
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return 0;
}

static s32 disp_lcd_get_color_temperature(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	unsigned long flags;
	s32 color_temperature = 0;

	if ((NULL == lcd) || (NULL == lcdp)) {
		DE_WARN("NULL hdl\n");
		return 0;
	}

	spin_lock_irqsave(&lcd_data_lock, flags);
	color_temperature = lcdp->color_temperature;
	spin_unlock_irqrestore(&lcd_data_lock, flags);

	return color_temperature;
}
static s32 pq_lcd_parse_panel_para(struct disp_lcd_private_data *lcdp)
{
	lcdp->lcd_cfg.backlight_bright = lcd_param->value[1];
	lcdp->panel_info.lcd_x = lcd_param->value[2];
	lcdp->panel_info.lcd_y = lcd_param->value[3];
	lcdp->panel_info.lcd_dclk_freq = lcd_param->value[4];
	lcdp->panel_info.lcd_ht = lcd_param->value[5];
	lcdp->panel_info.lcd_hbp = lcd_param->value[6];
	lcdp->panel_info.lcd_hspw = lcd_param->value[7];
	lcdp->panel_info.lcd_vt = lcd_param->value[8];
	lcdp->panel_info.lcd_vbp = lcd_param->value[9];
	lcdp->panel_info.lcd_vspw = lcd_param->value[10];
	lcdp->panel_info.lcd_start_delay = lcd_param->value[11];
	lcdp->panel_info.lcd_pwm_ch = lcd_param->value[12];
	lcdp->panel_info.lcd_pwm_pol = lcd_param->value[13];
	lcdp->panel_info.lcd_hv_clk_phase = lcd_param->value[14];
	lcdp->panel_info.lcd_hv_sync_polarity = lcd_param->value[15];
	lcdp->panel_info.lcd_lvds_if = lcd_param->value[16];
	lcdp->panel_info.lcd_lvds_colordepth = lcd_param->value[17];
	lcdp->panel_info.lcd_dsi_if = lcd_param->value[18];
	lcdp->panel_info.lcd_dsi_lane = lcd_param->value[19];
	lcdp->panel_info.lcd_dsi_format = lcd_param->value[20];
	lcdp->panel_info.lcd_dsi_port_num = lcd_param->value[21];
	lcdp->panel_info.lcd_tcon_mode = lcd_param->value[22];
	lcdp->panel_info.lcd_tcon_en_odd_even = lcd_param->value[23];
	lcdp->panel_info.lcd_sync_pixel_num = lcd_param->value[24];
	lcdp->panel_info.lcd_sync_line_num = lcd_param->value[25];

	return 0;
}
static void lcd_notify_tp_work(struct work_struct *work)
{
	struct fb_event event;
	struct lcd_notify_tp *tmp_work = container_of(work, struct lcd_notify_tp, tp_work);

	event.info = fb_infos[0];
	event.data = &tmp_work->blank;
	fb_notifier_call_chain(FB_EVENT_BLANK, &event);
}

static s32 disp_lcd_timing_config(struct disp_video_timings *timings,
				  struct disp_panel_para *panel_info)
{
	if (!timings || !panel_info) {
		DE_WARN("Null handle\n");
		return -1;
	}

	if (panel_info->lcd_if == LCD_IF_DSI &&
	    panel_info->lcd_tcon_mode == DISP_TCON_DUAL_DSI &&
	    panel_info->lcd_dsi_port_num ==
		DISP_LCD_DSI_SINGLE_PORT) {
		panel_info->lcd_ht *= 2;
		panel_info->lcd_hspw *= 2;
		panel_info->lcd_x *= 2;
		panel_info->lcd_hbp *= 2;
		panel_info->lcd_dclk_freq *= 2;
	}
	timings->pixel_clk = panel_info->lcd_dclk_freq * 1000;
	timings->x_res = panel_info->lcd_x;
	timings->y_res = panel_info->lcd_y;
	timings->hor_total_time = panel_info->lcd_ht;
	timings->hor_sync_time = panel_info->lcd_hspw;
	timings->hor_back_porch = panel_info->lcd_hbp - panel_info->lcd_hspw;
	timings->hor_front_porch = panel_info->lcd_ht - panel_info->lcd_hbp - panel_info->lcd_x;
	timings->ver_total_time = panel_info->lcd_vt;
	timings->ver_sync_time = panel_info->lcd_vspw;
	timings->ver_back_porch = panel_info->lcd_vbp - panel_info->lcd_vspw;
	timings->ver_front_porch = panel_info->lcd_vt - panel_info->lcd_vbp - panel_info->lcd_y;

	return 0;
}

static s32 disp_lcd_init(struct disp_device *lcd, int lcd_index)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_gpio_info gpio_info;
	int i;

	INIT_WORK(&screen_driver_work.tp_work, lcd_notify_tp_work);
	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	lcd_get_sys_config(lcd_index, &lcdp->lcd_cfg);

	lcd_regulator_get_wrap(&lcdp->lcd_cfg);

	if (disp_lcd_is_used(lcd)) {
		if (lcd_param == NULL)
			lcd_parse_panel_para(lcd_index, &lcdp->panel_info);
		else
			pq_lcd_parse_panel_para(lcdp);
		lcdp->panel_extend_info.lcd_cmap_en = lcdp->panel_info.lcd_cmap_en;
		lcdp->panel_extend_info.lcd_gamma_en = lcdp->panel_info.lcd_gamma_en;
		if (lcdp->panel_extend_info.lcd_gamma_en ||
		    lcdp->panel_extend_info.lcd_cmap_en)
			lcdp->panel_extend_dirty = 1;

		disp_lcd_timing_config(&lcd->timings, &lcdp->panel_info);
	}
	disp_lcd_bright_curve_init(lcd);

	if (disp_lcd_is_used(lcd)) {
		__u64 backlight_bright;
		__u64 period_ns, duty_ns;

		if (lcdp->panel_info.lcd_pwm_used) {
			lcdp->pwm_info.channel = lcdp->panel_info.lcd_pwm_ch;
			lcdp->pwm_info.polarity = lcdp->panel_info.lcd_pwm_pol;
			if (!lcdp->panel_info.lcd_pwm_name)
				lcdp->pwm_info.dev =
					disp_sys_pwm_request(lcdp->panel_info.lcd_pwm_ch);
			else
				lcdp->pwm_info.dev =
					disp_sys_pwm_get(lcdp->panel_info.lcd_pwm_name);

			if (lcdp->panel_info.lcd_pwm_freq != 0) {
				period_ns =
				    1000 * 1000 * 1000 /
				    lcdp->panel_info.lcd_pwm_freq;
			} else {
				DE_WARN("lcd%d.lcd_pwm_freq is ZERO\n",
				       lcd->disp);
				/* default 1khz */
				period_ns = 1000 * 1000 * 1000 / 1000;
			}

			backlight_bright = lcdp->lcd_cfg.backlight_bright;

			duty_ns = (backlight_bright * period_ns) / 256;
			lcdp->pwm_info.duty_ns = duty_ns;
			lcdp->pwm_info.period_ns = period_ns;
			disp_sys_pwm_config(lcdp->pwm_info.dev, duty_ns, period_ns);
		}
		lcd_clk_init(lcd);
		for (i = 0; i < 256; i++) {
			lcdp->panel_extend_info.lcd_gamma_tbl[i] =
				(i << 24) | (i << 16) | (i << 8) | (i);
		}
		for (i = 0; i < LCD_GPIO_NUM; i++) {
			if (!lcdp->lcd_cfg.lcd_gpio_used[i])
				continue;

			memcpy(&gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]),
			       sizeof(struct disp_gpio_info));
			disp_sys_gpio_request(&gpio_info);
		}
	}

#if IS_ENABLED(CONFIG_BACKLIGHT_CLASS_DEVICE)
	if (lcdp->panel_info.lcd_pwm_name && lcdp->pwm_info.dev)
		pwm_backlight_probe(g_disp_drv.dev, lcd, lcdp->panel_info.lcd_pwm_name,
				lcdp->pwm_info.dev);
#endif
	/* lcd_panel_parameter_check(lcd->disp, lcd); */
	return 0;
}

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
static void disp_close_eink_panel_task(struct work_struct *work)
{				/* (unsigned long parg) */
	struct disp_device *plcd = NULL;

	plcd = disp_device_find(0, DISP_OUTPUT_TYPE_LCD);
	plcd->disable(plcd);
	display_finish_flag = 1;
}
#endif

static disp_config_update_t disp_lcd_check_config_dirty(struct disp_device *lcd,
					struct disp_device_config *config)
{
	disp_config_update_t ret = DISP_NOT_UPDATE;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		goto exit;
	}

	if (lcdp->enabled == 0 ||
	    ((config->reserve1 & DISP_LCD_MODE_DIRTY_MASK) &&
	     config->reserve1 != lcdp->config.reserve1) ||
	    lcdp->panel_info.convert_info.is_enabled)
		ret = DISP_NORMAL_UPDATE;

exit:
	return ret;
}

static s32 disp_lcd_set_static_config(struct disp_device *lcd,
			       struct disp_device_config *config)
{
	int ret = -1, i = 0;
	unsigned long flags;
	struct disp_lcd_private_data *lcdp = NULL;

	if (!lcd || !config) {
		DE_WARN("NULL hdl\n");
		goto OUT;
	}

	lcdp = disp_lcd_get_priv(lcd);
	if (!lcdp) {
		DE_WARN("NULL lcdp\n");
		goto OUT;
	}

	if ((config->reserve1 & DISP_LCD_MODE_DIRTY_MASK) &&
	    config->reserve1 != lcdp->config.reserve1) {
		ret = disp_lcd_init(lcd, config->reserve1 & 0xf);
		lcd_set_panel_funs();
		lcdp->config.reserve1 = config->reserve1;
	} else
		ret = 0;

	if (lcdp->panel_info.convert_info.is_enabled &&
	    lcdp->lcd_panel_fun.get_panel_para_mapping &&
	    lcdp->panel_info.convert_info.cur_mode != config->mode) {
		const struct disp_panel_para_mapping *mapping = NULL;

		lcdp->lcd_panel_fun.get_panel_para_mapping(&mapping);
		while (mapping[i].tv_mode != DISP_TV_MODE_NUM) { /* list end */
			if (mapping[i].tv_mode != config->mode) {
				i++;
				continue;
			}
			spin_lock_irqsave(&lcd_data_lock, flags);
			lcdp->panel_info.convert_info.cur_mode = config->mode;
			lcdp->panel_info.lcd_dclk_freq = mapping[i].panel_info.lcd_dclk_freq;
			lcdp->panel_info.lcd_x = mapping[i].panel_info.lcd_x;
			lcdp->panel_info.lcd_y = mapping[i].panel_info.lcd_y;
			lcdp->panel_info.lcd_ht = mapping[i].panel_info.lcd_ht;
			lcdp->panel_info.lcd_hbp = mapping[i].panel_info.lcd_hbp;
			lcdp->panel_info.lcd_hspw = mapping[i].panel_info.lcd_hspw;
			lcdp->panel_info.lcd_vt = mapping[i].panel_info.lcd_vt;
			lcdp->panel_info.lcd_vbp = mapping[i].panel_info.lcd_vbp;
			lcdp->panel_info.lcd_vspw = mapping[i].panel_info.lcd_vspw;
			disp_lcd_timing_config(&lcd->timings, &lcdp->panel_info);
			spin_unlock_irqrestore(&lcd_data_lock, flags);
			break;
		}
	}

OUT:
	return ret;
}

static s32 disp_lcd_get_static_config(struct disp_device *lcd,
					struct disp_device_config *config)
{
	int ret = 0;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		ret = -1;
		goto exit;
	}

	config->type = lcd->type;
	config->format = DISP_CSC_TYPE_RGB;
	if (lcdp->panel_info.convert_info.is_enabled)
		config->mode = lcdp->panel_info.convert_info.cur_mode;
exit:
	return ret;
}

static s32 disp_lcd_exit(struct disp_device *lcd)
{
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(lcd);
	struct disp_gpio_info gpio_info;
	s32 i = 0;

	if ((lcd == NULL) || (lcdp == NULL)) {
		DE_WARN("NULL hdl\n");
		return DIS_FAIL;
	}

	if (lcdp->panel_info.lcd_pwm_name) {
		kfree(lcdp->panel_info.lcd_pwm_name);
		lcdp->panel_info.lcd_pwm_name = NULL;
	}
	lcd_regulator_put_wrap(&lcdp->lcd_cfg);

	for (i = 0; i < LCD_GPIO_NUM; i++) {
		memcpy(&gpio_info, &(lcdp->lcd_cfg.lcd_gpio[i]), sizeof(struct disp_gpio_info));
		gpio_info.value = 0;
		disp_sys_gpio_release(&gpio_info);
	}
	lcd_clk_exit(lcd);

	return 0;
}

#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
static s32 disp_lcd_get_esd_info(struct disp_device *dispdev,
		 struct disp_lcd_esd_info *p_esd_info)
{
	s32 ret = -1;
	struct disp_lcd_private_data *lcdp = NULL;

	if (!dispdev || !p_esd_info)
		goto OUT;
	 lcdp = disp_lcd_get_priv(dispdev);
	if (!lcdp)
		goto OUT;

	memcpy(p_esd_info, &lcdp->esd_inf, sizeof(struct disp_lcd_esd_info));
	ret = 0;
OUT:
	return ret;
}
#endif

static u32 disp_lcd_usec_before_vblank(struct disp_device *dispdev)
{
	int cur_line;
	int start_delay;
	u32 usec = 0;
	struct disp_lcd_private_data *lcdp = disp_lcd_get_priv(dispdev);

	if (!dispdev || !lcdp) {
		DE_WARN("NULL hdl\n");
		goto exit;
	}

	start_delay = disp_al_lcd_get_start_delay(dispdev->hwdev_index,
						  &lcdp->panel_info);
	cur_line =
	    disp_al_lcd_get_cur_line(dispdev->hwdev_index, &lcdp->panel_info);
	if (cur_line > (start_delay - lcdp->judge_line)) {
		usec =
		    (lcdp->panel_info.lcd_vt - cur_line + 1) * lcdp->usec_per_line;
	}

exit:
	return usec;
}

s32 disp_init_lcd(struct disp_bsp_init_para *para)
{
	u32 num_devices;
	u32 disp = 0;
	struct disp_device *lcd;
	struct disp_lcd_private_data *lcdp;
	u32 hwdev_index = 0;
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
	u32 lvds_clk_index = 0;
#endif

#if defined(SUPPORT_DSI)
	u32 i = 0, index_base;
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN8IW17P1)
	char primary_key[20];
	int ret = 0, value = 1;
	s32 use_dsi_flag = 0;
#endif

	DE_INFO("disp_init_lcd\n");

	spin_lock_init(&lcd_data_lock);
	num_devices = bsp_disp_feat_get_num_devices();
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (bsp_disp_feat_is_supported_output_types
		    (hwdev_index, DISP_OUTPUT_TYPE_LCD))
			lcd_device_num++;
	}
	if (lcds) {
		kfree(lcds);
		lcds = NULL;
	}
	lcds =
	    kmalloc_array(lcd_device_num, sizeof(struct disp_device),
			  GFP_KERNEL | __GFP_ZERO);
	if (lcds == NULL) {
		DE_WARN("malloc memory(%d bytes) fail\n",
		       (unsigned int)sizeof(struct disp_device) *
		       lcd_device_num);
		goto malloc_err;
	}
	if (lcd_private) {
		kfree(lcd_private);
		lcd_private = NULL;
	}
	lcd_private =
	    (struct disp_lcd_private_data *)
	    kmalloc(sizeof(struct disp_lcd_private_data)
		    * lcd_device_num, GFP_KERNEL | __GFP_ZERO);
	if (lcd_private == NULL) {
		DE_WARN("malloc memory(%d bytes) fail\n",
		       (unsigned int)sizeof(struct disp_lcd_private_data) *
		       lcd_device_num);
		goto malloc_err;
	}

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types
		    (hwdev_index, DISP_OUTPUT_TYPE_LCD)) {
			continue;
		}

		lcd = &lcds[disp];
		lcdp = &lcd_private[disp];
		lcd->priv_data = (void *)lcdp;

		sprintf(lcd->name, "lcd%d", disp);
		lcd->disp = disp;
#if IS_ENABLED(CONFIG_ARCH_SUN8IW17P1)
		value = 0;
		ret = disp_sys_script_get_item(primary_key, "lcd_if", &value,
					       1);
		if (value == 4) {
			lcd->hwdev_index = 1;
			use_dsi_flag = 1;
		} else
			lcd->hwdev_index = (use_dsi_flag == 1) ? 0 : hwdev_index;
#else
		lcd->hwdev_index = hwdev_index;
#endif
		lcd->type = DISP_OUTPUT_TYPE_LCD;
		lcdp->irq_no = para->irq_no[DISP_MOD_LCD0 + lcd->hwdev_index];
		lcdp->clk_tcon_lcd = para->clk_tcon[lcd->hwdev_index];
		lcdp->clk_bus_tcon_lcd = para->clk_bus_tcon[lcd->hwdev_index];
		lcdp->rst_bus_tcon_lcd = para->rst_bus_tcon[lcd->hwdev_index];

		/* LVDS CLOCK. */
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
		if (is_tcon_support_output_types(lcd->hwdev_index, TCON_TO_LVDS)) {
			if (lvds_clk_index >= DEVICE_LVDS_NUM)
				DE_WARN("lvds_clk_index >= DEVICE_LVDS_NUM");
			else {
				lcdp->clk_lvds = para->clk_tcon[lvds_clk_index];
				lcdp->rst_bus_lvds = para->rst_bus_lvds[lvds_clk_index];
				lvds_clk_index++;
			}
		}
#else
		if (lcd->hwdev_index < DEVICE_LVDS_NUM) {
			lcdp->clk_lvds = para->clk_tcon[lcd->hwdev_index];
			lcdp->rst_bus_lvds = para->rst_bus_lvds[lcd->hwdev_index];
		}
#endif
#if defined(SUPPORT_DSI)
		lcdp->irq_no_dsi = para->irq_no[DISP_MOD_DSI0 + disp];

		index_base = CLK_NUM_PER_DSI * lcd->hwdev_index;

		/* TCON0->DSI0 & DSI1, TCON1-> DSI1  */
		for (i = 0; i < CLK_NUM_PER_DSI * DEVICE_DSI_NUM && (index_base + i < CLK_DSI_NUM); i++) {
			lcdp->clk_mipi_dsi[i] = para->clk_mipi_dsi[index_base + i];
			lcdp->clk_bus_mipi_dsi[i] = para->clk_bus_mipi_dsi[index_base + i];
#if defined(DE_VERSION_V35X) || defined(DE_VERSION_V21X)
			lcdp->clk_mipi_dsi_combphy[i] = para->clk_mipi_dsi_combphy[index_base + i];
#endif  /* DE_VERSION_V35X */
			lcdp->rst_bus_mipi_dsi[i] = para->rst_bus_mipi_dsi[index_base + i];
		}
		DE_INFO("total number of clk in dsi:%d\n", CLK_DSI_NUM);

#endif
		DE_INFO("lcd %d, irq_no=%d, irq_no_dsi=%d\n", disp, lcdp->irq_no,
		       lcdp->irq_no_dsi);

		lcd->set_manager = disp_device_set_manager;
		lcd->unset_manager = disp_device_unset_manager;
		lcd->get_resolution = disp_device_get_resolution;
		lcd->get_timings = disp_device_get_timings;
		lcd->enable = disp_lcd_enable;
		lcd->sw_enable = disp_lcd_sw_enable;
		lcd->fake_enable = disp_lcd_fake_enable;
		lcd->disable = disp_lcd_disable;
		lcd->is_enabled = disp_lcd_is_enabled;
		lcd->check_if_enabled = disp_lcd_check_if_enabled;
		lcd->set_bright = disp_lcd_set_bright;
		lcd->get_bright = disp_lcd_get_bright;
		lcd->set_bright_dimming = disp_lcd_set_bright_dimming;
		lcd->get_panel_info = disp_lcd_get_panel_info;
		lcd->get_mode = disp_lcd_get_mode;
		lcd->set_static_config = disp_lcd_set_static_config;
		lcd->get_static_config = disp_lcd_get_static_config;
		lcd->check_config_dirty = disp_lcd_check_config_dirty;

		lcd->set_panel_func = disp_lcd_set_panel_funs;
		lcd->set_open_func = disp_lcd_set_open_func;
		lcd->set_close_func = disp_lcd_set_close_func;
		lcd->backlight_enable = disp_lcd_backlight_enable;
		lcd->backlight_disable = disp_lcd_backlight_disable;
		lcd->pwm_enable = disp_lcd_pwm_enable;
		lcd->pwm_disable = disp_lcd_pwm_disable;
		lcd->power_enable = disp_lcd_power_enable;
		lcd->power_disable = disp_lcd_power_disable;
		lcd->pin_cfg = disp_lcd_pin_cfg;
		lcd->tcon_enable = disp_lcd_tcon_enable;
		lcd->tcon_disable = disp_lcd_tcon_disable;
		lcd->gpio_set_value = disp_lcd_gpio_set_value;
		lcd->gpio_get_value = disp_lcd_gpio_get_value;
		lcd->gpio_set_direction = disp_lcd_gpio_set_direction;
		lcd->get_dimensions = disp_lcd_get_dimensions;
		lcd->get_status = disp_lcd_get_status;
		lcd->is_in_safe_period = disp_lcd_is_in_safe_period;
		lcd->usec_before_vblank = disp_lcd_usec_before_vblank;
		lcd->set_gamma_tbl = disp_lcd_set_gamma_tbl;
		lcd->get_gamma_tbl = disp_lcd_get_gamma_tbl;
		lcd->enable_gamma = disp_lcd_enable_gamma;
		lcd->disable_gamma = disp_lcd_disable_gamma;
		lcd->get_fps = disp_lcd_get_fps;
		lcd->set_color_temperature = disp_lcd_set_color_temperature;
		lcd->get_color_temperature = disp_lcd_get_color_temperature;
		lcd->show_builtin_patten = disp_device_show_builtin_patten;
		lcd->get_real_fps = disp_lcd_get_real_fps;
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
		lcd->get_esd_info = disp_lcd_get_esd_info;
#endif

		/* lcd->init = disp_lcd_init; */
		lcd->exit = disp_lcd_exit;

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
		INIT_WORK(&lcd->close_eink_panel_work,
			  disp_close_eink_panel_task);
#endif
		disp_lcd_init(lcd, lcd->disp);
		disp_device_register(lcd);
#if IS_ENABLED(CONFIG_DISP2_LCD_ESD_DETECT)
		INIT_WORK(&lcdp->reflush_work, disp_lcd_reflush_work);
		atomic_set(&lcdp->lcd_resetting, 0);
#endif

		disp++;
	}

	return 0;

malloc_err:
	kfree(lcds);
	kfree(lcd_private);
	lcds = NULL;
	lcd_private = NULL;

	return -1;
}

s32 disp_exit_lcd(void)
{
	u32 num_devices;
	u32 disp = 0;
	struct disp_device *lcd;
	u32 hwdev_index = 0;

	if (!lcds)
		return 0;

	num_devices = bsp_disp_feat_get_num_devices();

	disp = 0;
	for (hwdev_index = 0; hwdev_index < num_devices; hwdev_index++) {
		if (!bsp_disp_feat_is_supported_output_types
		    (hwdev_index, DISP_OUTPUT_TYPE_LCD)) {
			continue;
		}
		lcd = &lcds[disp];
		disp_device_unregister(lcd);
		lcd->exit(lcd);
		disp++;
	}

	kfree(lcds);
	kfree(lcd_private);
	lcds = NULL;
	lcd_private = NULL;

#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	if (iomap_data.initialized) {
		iounmap(iomap_data.qa_addr);
		iounmap(iomap_data.disp_cfg_addr);

		iomap_data.initialized = 0;
		iomap_data.qa_addr = NULL;
		iomap_data.disp_cfg_addr = NULL;
	}
#endif

	return 0;
}

int get_lcd_timing(int lcd_node, unsigned long arg)
{
	struct lcd_timing *reg;
	unsigned long *ubuffer;
	int disp, i;
	struct disp_lcd_private_data *lcdp;

	ubuffer = (unsigned long *)arg;
	reg = (struct lcd_timing *)kzalloc(sizeof(struct lcd_timing), GFP_KERNEL);
	if (reg == NULL) {
		DE_WARN("pq get malloc failed\n");
		return -1;
	}
	if (sizeof(struct lcd_timing) != ubuffer[3]) {
		DE_WARN("Transfer data size mismatch\n");
		return -1;
	}

	disp = lcd_node;
	lcdp = disp_lcd_get_priv(&lcds[disp]);

	reg->id = 27;
	reg->lcd_node = disp;
	reg->value[0] = 1;
	reg->value[1] = lcdp->lcd_cfg.backlight_bright;
	reg->value[2] = lcdp->panel_info.lcd_x;
	reg->value[3] = lcdp->panel_info.lcd_y;
	reg->value[4] = lcdp->panel_info.lcd_dclk_freq;
	reg->value[5] = lcdp->panel_info.lcd_ht;
	reg->value[6] = lcdp->panel_info.lcd_hbp;
	reg->value[7] = lcdp->panel_info.lcd_hspw;
	reg->value[8] = lcdp->panel_info.lcd_vt;
	reg->value[9] = lcdp->panel_info.lcd_vbp;
	reg->value[10] = lcdp->panel_info.lcd_vspw;
	reg->value[11] = lcdp->panel_info.lcd_start_delay;
	reg->value[12] = lcdp->panel_info.lcd_pwm_ch;
	reg->value[13] = lcdp->panel_info.lcd_pwm_pol;
	reg->value[14] = lcdp->panel_info.lcd_hv_clk_phase;
	reg->value[15] = lcdp->panel_info.lcd_hv_sync_polarity;
	reg->value[16] = lcdp->panel_info.lcd_lvds_if;
	reg->value[17] = lcdp->panel_info.lcd_lvds_colordepth;
	reg->value[18] = lcdp->panel_info.lcd_dsi_if;
	reg->value[19] = lcdp->panel_info.lcd_dsi_lane;
	reg->value[20] = lcdp->panel_info.lcd_dsi_format;
	reg->value[21] = lcdp->panel_info.lcd_dsi_port_num;
	reg->value[22] = lcdp->panel_info.lcd_tcon_mode;
	reg->value[23] = lcdp->panel_info.lcd_tcon_en_odd_even;
	reg->value[24] = lcdp->panel_info.lcd_sync_pixel_num;
	reg->value[25] = lcdp->panel_info.lcd_sync_line_num;
	reg->value[26] = 1;

	for (i = 0; i < LCD_REG_COUNT; i++) {
		DE_INFO("======= value[%d] = %d =========\n", i, reg->value[i]);
	}

	if (copy_to_user((void __user *)ubuffer[2], reg, ubuffer[3])) {
		DE_WARN("copy to user REG failed\n");
		return -1;
	}

	kfree(reg);
	return 0;
}

int set_lcd_timing(int lcd_node, unsigned long arg)
{
	unsigned long *ubuffer;
	int i;
	ubuffer = (unsigned long *)arg;

	lcd_param = (struct lcd_timing *)kmalloc(sizeof(struct lcd_timing), GFP_KERNEL);

	if (copy_from_user(lcd_param, (void __user *)ubuffer[2], ubuffer[3])) {
		DE_WARN("regs copy from user failed\n");
		return -1;
	}

	for (i = 0; i < LCD_REG_COUNT; i++) {
		DE_INFO("======= value[%d] = %d =========\n", i, lcd_param->value[i]);
	}
	reload_lcd(lcd_node);

	kfree(lcd_param);
	return 0;
}
extern struct disp_drv_info g_disp_drv;
extern s32 disp_exit(void);
extern struct __lcd_panel *panel_array[];
void reload_lcd(int lcd_node)
{
	int disp, ret, i;
	struct disp_lcd_private_data *lcdp;
	char name[20], primary_key[10];

	disp = lcd_node;
	DE_INFO("lcd%d reloading\n", disp);
	disp_lcd_disable(&lcds[disp]);
	msleep(5000);
	lcdp = disp_lcd_get_priv(&lcds[disp]);
	memset(&lcdp->lcd_cfg, 0, sizeof(lcdp->lcd_cfg));
	disp_lcd_init(&lcds[disp], disp);

	sprintf(primary_key, "lcd%d", disp);
	ret = disp_sys_script_get_item(primary_key, "lcd_driver_name",
			(int *)name, 2);
	DE_INFO("driver_name: %s\n", name);

	for (i = 0; panel_array[i] != NULL; i++) {
		if (!strcmp(panel_array[i]->name, name)) {
			disp_lcd_set_panel_funs(&lcds[disp], panel_array[i]->name,
					&panel_array[i]->func);
			break;
		}

	}
	disp_lcd_enable(&lcds[disp]);
	DE_INFO("lcd%d reload finish\n", disp);
}
