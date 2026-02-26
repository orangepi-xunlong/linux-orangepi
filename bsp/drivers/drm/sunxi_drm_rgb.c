/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/of_device.h>
#include <linux/component.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mipi-dphy.h>
#include <linux/version.h>
#include <linux/media-bus-format.h>
#include <linux/delay.h>
#include <drm/drm_panel.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_property.h>
#include "sunxi_drm_drv.h"
#include "sunxi_device/sunxi_tcon.h"
#include "sunxi_drm_intf.h"
#include "sunxi_drm_crtc.h"
#include "panel/panel-rgb.h"
#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
#define RGB_DISPLL_CLK
#endif

enum sunxi_tiger_lcd_rgb_param {
	SUNXI_TIGER_LCD_FLAG_HEADER = 0,
	SUNXI_TIGER_LCD_BACKLIGHT,
	SUNXI_TIGER_LCD_MODE_PIXELCLOCK,
	SUNXI_TIGER_LCD_MODE_HACTIVE,
	SUNXI_TIGER_LCD_MODE_HFRONT_PORCH,
	SUNXI_TIGER_LCD_MODE_HBACK_PORCH,
	SUNXI_TIGER_LCD_MODE_HSYNC_LEN,
	SUNXI_TIGER_LCD_MODE_VACTIVE,
	SUNXI_TIGER_LCD_MODE_VFRONT_PORCH,
	SUNXI_TIGER_LCD_MODE_VBACK_PORCH,
	SUNXI_TIGER_LCD_MODE_VSYNC_LEN,
	SUNXI_TIGER_LCD_RESERVED0,
	SUNXI_TIGER_LCD_RESERVED1,
	SUNXI_TIGER_LCD_TIMING_RESET_NUM,
	SUNXI_TIGER_LCD_TIMING_DELAY_POWER,
	SUNXI_TIGER_LCD_TIMING_DELAY_ENABLE,
	SUNXI_TIGER_LCD_TIMING_DELAY_RESET,
	SUNXI_TIGER_LCD_RESERVED2,
	SUNXI_TIGER_LCD_RESERVED3,
	SUNXI_TIGER_LCD_RESERVED4,
	SUNXI_TIGER_LCD_RESERVED5,
	SUNXI_TIGER_LCD_RESERVED6,
	SUNXI_TIGER_LCD_RESERVED7,
	SUNXI_TIGER_LCD_RESERVED8,
	SUNXI_TIGER_LCD_RESERVED9,
	SUNXI_TIGER_LCD_RESERVED10,
	SUNXI_TIGER_LCD_FLAG_FOOTER,
};

struct rgb_data {
	int id;
};
struct sunxi_drm_rgb {
	struct sunxi_drm_device sdrm;
	struct drm_display_mode mode;
	struct drm_display_mode *adjusted_mode;
	struct disp_rgb_para rgb_para;
	bool bound;
	bool enable;
	bool sw_enable;
	bool displl_clk;
	struct device *dev;
	struct device *sunxi_rgb_sysfs_dev;

	struct phy *phy;
	union phy_configure_opts phy_opts;
	const struct rgb_data *rgb_data;

	struct clk *pclk;
	unsigned long mode_flags;
	unsigned long pclk_clk_rate;
	struct class *rgb_class;
};
static const struct rgb_data rgb0_data = {
	.id = 0,
};

static const struct rgb_data rgb1_data = {
	.id = 1,
};

static const struct of_device_id sunxi_drm_rgb_match[] = {
	{ .compatible = "allwinner,rgb0", .data = &rgb0_data },
	{ .compatible = "allwinner,rgb1", .data = &rgb1_data },
	{},
};

static struct device *drm_rgb_of_get_tcon(struct device *rgb_dev)
{
	struct device_node *node = rgb_dev->of_node;
	struct device_node *tcon_lcd_node;
	struct device_node *rgb_in_tcon;
	struct platform_device *pdev = NULL;
	struct device *tcon_lcd_dev = NULL;;

	rgb_in_tcon = of_graph_get_endpoint_by_regs(node, 0, 0);
	if (!rgb_in_tcon) {
		DRM_ERROR("endpoint rgb_in_tcon not fount\n");
		return NULL;
	}

	tcon_lcd_node = of_graph_get_remote_port_parent(rgb_in_tcon);
	if (!tcon_lcd_node) {
		DRM_ERROR("node tcon_lcd not fount\n");
		tcon_lcd_dev = NULL;
		goto RGB_PUT;
	}

	pdev = of_find_device_by_node(tcon_lcd_node);
	if (!pdev) {
		DRM_ERROR("tcon_lcd platform device not fount\n");
		tcon_lcd_dev = NULL;
		goto TCON_RGB_PUT;
	}

	tcon_lcd_dev = &pdev->dev;
	platform_device_put(pdev);

TCON_RGB_PUT:
	of_node_put(tcon_lcd_node);
RGB_PUT:
	of_node_put(rgb_in_tcon);

	return tcon_lcd_dev;
}

static inline struct sunxi_drm_rgb *encoder_to_sunxi_drm_rgb(struct drm_encoder *encoder)
{
	struct sunxi_drm_device *sdrm = container_of(encoder, struct sunxi_drm_device, encoder);

	return container_of(sdrm, struct sunxi_drm_rgb, sdrm);
}

static inline struct sunxi_drm_rgb *connector_to_sunxi_drm_rgb(struct drm_connector *connector)
{
	struct sunxi_drm_device *sdrm = container_of(connector, struct sunxi_drm_device, connector);

	return container_of(sdrm, struct sunxi_drm_rgb, sdrm);
}

static int sunxi_lcd_pin_set_state(struct device *dev, char *name)
{
	int ret;
	struct pinctrl *pctl;
	struct pinctrl_state *state;

	DRM_INFO("[RGB] %s start\n", __FUNCTION__);
	pctl = pinctrl_get(dev);
	if (IS_ERR(pctl)) {
		DRM_INFO("[WARN]can NOT get pinctrl for %lx \n",
			(unsigned long)dev);
		ret = 0;
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		DRM_ERROR("pinctrl_lookup_state for %lx fail\n",
			(unsigned long)dev);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		DRM_ERROR("pinctrl_select_state(%s) for %lx fail\n", name,
			(unsigned long)dev);
		goto exit;
	}

exit:
	return ret;
}
static int sunxi_rgb_displl_enable(struct sunxi_drm_rgb *rgb)
{
	if (rgb->pclk) {
		clk_set_rate(rgb->pclk, rgb->pclk_clk_rate);
		clk_prepare_enable(rgb->pclk);
	}

	return 0;
}

static int sunxi_rgb_displl_disable(struct sunxi_drm_rgb *rgb)
{
	if (rgb->pclk) {
		clk_set_rate(rgb->pclk, 24000000);
		clk_disable_unprepare(rgb->pclk);
	}

	return 0;
}
void sunxi_drm_rgb_encoder_atomic_enable(struct drm_encoder *encoder,
					struct drm_atomic_state *state)
{
	int ret;
	struct drm_crtc *crtc = encoder->crtc;
	int de_hw_id = sunxi_drm_crtc_get_hw_id(crtc);
	struct drm_crtc_state *crtc_state = crtc->state;
	struct sunxi_drm_rgb *rgb = encoder_to_sunxi_drm_rgb(encoder);
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct disp_output_config disp_cfg;

	DRM_INFO("[RGB] %s start\n", __FUNCTION__);
	drm_mode_to_sunxi_video_timings(&rgb->mode, &rgb->rgb_para.timings);

	memset(&disp_cfg, 0, sizeof(struct disp_output_config));
	memcpy(&disp_cfg.rgb_para, &rgb->rgb_para,
		sizeof(rgb->rgb_para));
	disp_cfg.type = INTERFACE_RGB;
	disp_cfg.de_id = de_hw_id;
	disp_cfg.irq_handler = sunxi_crtc_event_proc;
	disp_cfg.irq_data = scrtc_state->base.crtc;
	disp_cfg.sw_enable = rgb->sw_enable;
	disp_cfg.tcon_lcd_div = 7;
#ifdef RGB_DISPLL_CLK
	disp_cfg.displl_clk = true;
#else
	disp_cfg.displl_clk = false;
#endif
	rgb->displl_clk = disp_cfg.displl_clk;
	if (rgb->rgb_data->id == 1)
		disp_cfg.displl_clk = false;
	sunxi_tcon_mode_init(rgb->sdrm.tcon_dev, &disp_cfg);

	rgb->pclk_clk_rate = rgb->rgb_para.timings.pixel_clk * disp_cfg.tcon_lcd_div;

	sunxi_lcd_pin_set_state(rgb->dev, "active");

	if (rgb->sw_enable) {
		if (rgb->phy) {
			phy_power_on(rgb->phy);
			if (rgb->pclk)
				clk_prepare_enable(rgb->pclk);
		}
		panel_rgb_regulator_enable(rgb->sdrm.panel);
	} else {
		if (rgb->phy)
			phy_power_on(rgb->phy);
		drm_panel_prepare(rgb->sdrm.panel);
		sunxi_rgb_displl_enable(rgb);
		ret = sunxi_rgb_enable_output(rgb->sdrm.tcon_dev);
		if (ret < 0)
			DRM_DEV_INFO(rgb->dev, "failed to enable rgb ouput\n");
	}
	if (rgb->sdrm.panel)
		drm_panel_enable(rgb->sdrm.panel);
	rgb->enable = true;

	DRM_INFO("[RGB] %s finish\n", __FUNCTION__);

	return;
}

void sunxi_drm_rgb_encoder_atomic_disable(struct drm_encoder *encoder,
					struct drm_atomic_state *state)
{
	struct sunxi_drm_rgb *rgb = encoder_to_sunxi_drm_rgb(encoder);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	rgb->sdrm.panel->prepare_prev_first = false;
#endif
	drm_panel_disable(rgb->sdrm.panel);
	sunxi_rgb_displl_disable(rgb);
	drm_panel_unprepare(rgb->sdrm.panel);

	if (rgb->phy) {
		phy_power_off(rgb->phy);
	}

	sunxi_lcd_pin_set_state(rgb->dev, "sleep");
	sunxi_rgb_disable_output(rgb->sdrm.tcon_dev);
	sunxi_tcon_mode_exit(rgb->sdrm.tcon_dev);
	rgb->enable = false;

	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
}

static bool sunxi_rgb_fifo_check(void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	return sunxi_tcon_check_fifo_status(rgb->sdrm.tcon_dev);
}

int sunxi_rgb_get_current_line(void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	return sunxi_tcon_get_current_line(rgb->sdrm.tcon_dev);
}

static bool sunxi_rgb_is_sync_time_enough(void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	return sunxi_tcon_is_sync_time_enough(rgb->sdrm.tcon_dev);
}

static void sunxi_rgb_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;

	sunxi_tcon_enable_vblank(rgb->sdrm.tcon_dev, enable);
}

static bool sunxi_rgb_is_support_backlight(void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	return panel_rgb_is_support_backlight(rgb->sdrm.panel);
}

static int sunxi_rgb_get_backlight_value(void *data)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	return panel_rgb_get_backlight_value(rgb->sdrm.panel);
}

static void sunxi_rgb_set_backlight_value(void *data, int brightness)
{
	struct sunxi_drm_rgb *rgb = (struct sunxi_drm_rgb *)data;
	panel_rgb_set_backlight_value(rgb->sdrm.panel, brightness);
}

int sunxi_drm_rgb_encoder_atomic_check(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct sunxi_drm_rgb *rgb = encoder_to_sunxi_drm_rgb(encoder);

	/* FIXME:TODO: color_fmt/clolor_depth update by actual configuration */
/* 	scrtc_state->color_fmt = DISP_CSC_TYPE_RGB;
	scrtc_state->color_depth = DISP_DATA_8BITS; */
	scrtc_state->tcon_id = rgb->sdrm.tcon_id;
	scrtc_state->get_cur_line = sunxi_rgb_get_current_line;
	scrtc_state->is_sync_time_enough = sunxi_rgb_is_sync_time_enough;
	scrtc_state->is_support_backlight = sunxi_rgb_is_support_backlight;
	scrtc_state->get_backlight_value = sunxi_rgb_get_backlight_value;
	scrtc_state->set_backlight_value = sunxi_rgb_set_backlight_value;
	scrtc_state->enable_vblank = sunxi_rgb_enable_vblank;
	scrtc_state->check_status = sunxi_rgb_fifo_check;
	scrtc_state->output_dev_data = rgb;
	if (conn_state->crtc) {
		rgb->sw_enable = sunxi_drm_check_if_need_sw_enable(conn_state->connector);
		scrtc_state->sw_enable = rgb->sw_enable;
	}
	if (rgb->adjusted_mode)
		drm_mode_copy(&crtc_state->adjusted_mode, rgb->adjusted_mode);

	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
	return 0;
}

static void sunxi_drm_rgb_encoder_atomic_mode_set(struct drm_encoder *encoder,
					struct drm_crtc_state *crtc_state,
					struct drm_connector_state *conn_state)
{
	struct sunxi_drm_rgb *rgb = encoder_to_sunxi_drm_rgb(encoder);
	struct disp_rgb_para *rgb_para = &rgb->rgb_para;
	struct drm_connector *connector = &rgb->sdrm.connector;
	struct drm_display_info *info = &connector->display_info;
	u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;

	DRM_INFO("[RGB]%s start\n", __FUNCTION__);
	if (info->num_bus_formats)
		bus_format = info->bus_formats[0];
	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB565_1X16:
		rgb_para->lcd_frm = LCD_FRM_RGB565;
		break;
	case MEDIA_BUS_FMT_RGB666_1X18:
		rgb_para->lcd_frm = LCD_FRM_RGB666;
		break;
	case MEDIA_BUS_FMT_RGB888_1X24:
		rgb_para->lcd_frm = LCD_FRM_BYPASS;
		break;
	default:
		DRM_ERROR("Unsupport bus_format, pls check the conf.");
		break;
	}

	drm_mode_copy(&rgb->mode,  &crtc_state->adjusted_mode);
	DRM_INFO("[RGB]%s finish\n", __FUNCTION__);

}

struct sunxi_drm_rgb *drm_device_to_rgb(struct drm_device *dev)
{
	struct drm_connector *connector;
	struct sunxi_drm_rgb *rgb = NULL;

	connector = drm_device_to_connector(dev, DRM_MODE_CONNECTOR_DPI);
	if (!connector) {
		DRM_ERROR("No DRM_MODE_CONNECTOR_DSI found!\n");
		return NULL;
	}

	rgb = connector_to_sunxi_drm_rgb(connector);
	if (!rgb)
		DRM_ERROR("Can't get rgb from connector.\n");

	return rgb;
}

static int sunxi_set_rgb_timing(struct drm_device *dev, struct lcd_timing *reg)
{
	struct sunxi_drm_rgb *rgb;
	struct panel_rgb *rgb_panel;

	rgb = drm_device_to_rgb(dev);

	rgb_panel = dev_get_drvdata(rgb->sdrm.panel->dev);
	if (!rgb_panel) {
		DRM_ERROR("Can't get rgb_panel.\n");
		return -ENODATA;
	}

	rgb_panel->delay.power  = reg->value[SUNXI_TIGER_LCD_TIMING_DELAY_POWER];
	rgb_panel->delay.enable = reg->value[SUNXI_TIGER_LCD_TIMING_DELAY_ENABLE];
	rgb_panel->delay.reset  = reg->value[SUNXI_TIGER_LCD_TIMING_DELAY_RESET];

	return 0;
}

static int sunxi_set_rgb_mode(struct drm_device *dev, struct lcd_timing *reg)
{
	struct videomode *vm;
	struct sunxi_drm_rgb *rgb;
	struct drm_crtc *crtc;
	struct drm_display_mode *old_mode;
	struct drm_display_mode *new_mode;

	rgb = drm_device_to_rgb(dev);

	crtc = rgb->sdrm.encoder.crtc;
	old_mode = &crtc->state->adjusted_mode;
	if (!old_mode) {
		DRM_ERROR("old_mode is NULL\n");
		return -ENODATA;
	}

	vm = kzalloc(sizeof(struct videomode), GFP_KERNEL);
	if (!vm) {
		DRM_ERROR("videomode malloc failed\n");
		return -ENOMEM;
	}

	new_mode = drm_mode_duplicate(dev, old_mode);
	if (!new_mode) {
		DRM_ERROR("new_mode is NULL\n");
		return -ENOMEM;
	}

	vm->pixelclock   = reg->value[SUNXI_TIGER_LCD_MODE_PIXELCLOCK];
	vm->hactive      = reg->value[SUNXI_TIGER_LCD_MODE_HACTIVE];
	vm->hfront_porch = reg->value[SUNXI_TIGER_LCD_MODE_HFRONT_PORCH];
	vm->hback_porch  = reg->value[SUNXI_TIGER_LCD_MODE_HBACK_PORCH];
	vm->hsync_len    = reg->value[SUNXI_TIGER_LCD_MODE_HSYNC_LEN];
	vm->vactive      = reg->value[SUNXI_TIGER_LCD_MODE_VACTIVE];
	vm->vfront_porch = reg->value[SUNXI_TIGER_LCD_MODE_HFRONT_PORCH];
	vm->vback_porch  = reg->value[SUNXI_TIGER_LCD_MODE_VBACK_PORCH];
	vm->vsync_len    = reg->value[SUNXI_TIGER_LCD_MODE_VSYNC_LEN];

	drm_display_mode_from_videomode(vm, new_mode);
	drm_mode_set_name(new_mode);
	rgb->adjusted_mode = new_mode;

	kfree(vm);
	return 0;
}

void sunxi_set_disp_rgb_para(struct drm_device *dev, unsigned long *arg)
{
	int i, ret;
	struct lcd_timing *reg;
	struct sunxi_drm_rgb *rgb = drm_device_to_rgb(dev);

	reg = kzalloc(sizeof(struct lcd_timing), GFP_KERNEL);
	if (!reg) {
		DRM_ERROR("pq get malloc failed\n");
		return;
	}

	memcpy(reg, arg, sizeof(struct lcd_timing));

	for (i = 0; i < LCD_REG_COUNT; i++)
		DRM_ERROR("======= value[%d] = %d =========\n", i, reg->value[i]);

	sunxi_rgb_set_backlight_value(rgb, (int)reg->value[SUNXI_TIGER_LCD_BACKLIGHT]);
	ret = sunxi_set_rgb_mode(dev, reg);
	if (ret) {
		DRM_ERROR("Set new mode failed.\n");
		return;
	}
	sunxi_set_rgb_timing(dev, reg);

	drm_mode_config_helper_suspend(dev);
	mdelay(10);
	drm_mode_config_helper_resume(dev);

	kfree(reg);
}

void sunxi_get_disp_rgb_para(struct drm_device *dev, unsigned long *arg)
{
	struct lcd_timing *reg;
	struct sunxi_drm_rgb *rgb;
	struct panel_rgb *rgb_panel;
	struct videomode *vm;
	int i;

	rgb = drm_device_to_rgb(dev);
	rgb_panel = dev_get_drvdata(rgb->sdrm.panel->dev);
	if (!rgb_panel) {
		DRM_ERROR("Can't get rgb_panel.\n");
		return;
	}

	reg = kzalloc(sizeof(struct lcd_timing), GFP_KERNEL);
	if (!reg) {
		DRM_ERROR("pq get malloc failed.\n");
		return;
	}

	vm = kzalloc(sizeof(struct videomode), GFP_KERNEL);
	if (!reg) {
		DRM_ERROR("videomode malloc failed.\n");
		return;
	}

	drm_display_mode_to_videomode(&rgb->mode, vm);

	reg->id = 27;
	reg->lcd_node = 0;
	reg->value[SUNXI_TIGER_LCD_FLAG_HEADER]         = 1;
	reg->value[SUNXI_TIGER_LCD_BACKLIGHT]           = (unsigned long)sunxi_rgb_get_backlight_value(rgb);
	/* MODE */
	reg->value[SUNXI_TIGER_LCD_MODE_PIXELCLOCK]     = vm->pixelclock;
	reg->value[SUNXI_TIGER_LCD_MODE_HACTIVE]        = vm->hactive;
	reg->value[SUNXI_TIGER_LCD_MODE_HFRONT_PORCH]   = vm->hfront_porch;
	reg->value[SUNXI_TIGER_LCD_MODE_HBACK_PORCH]    = vm->hback_porch;
	reg->value[SUNXI_TIGER_LCD_MODE_HSYNC_LEN]      = vm->hsync_len;
	reg->value[SUNXI_TIGER_LCD_MODE_VACTIVE]        = vm->vactive;
	reg->value[SUNXI_TIGER_LCD_MODE_VFRONT_PORCH]   = vm->vfront_porch;
	reg->value[SUNXI_TIGER_LCD_MODE_VBACK_PORCH]    = vm->vback_porch;
	reg->value[SUNXI_TIGER_LCD_MODE_VSYNC_LEN]      = vm->vsync_len;
	reg->value[SUNXI_TIGER_LCD_RESERVED0]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED1]           = 99999;
	/* TIMING */
	reg->value[SUNXI_TIGER_LCD_TIMING_RESET_NUM]    = 99999;
	reg->value[SUNXI_TIGER_LCD_TIMING_DELAY_POWER]  = rgb_panel->delay.power;
	reg->value[SUNXI_TIGER_LCD_TIMING_DELAY_ENABLE] = rgb_panel->delay.enable;
	reg->value[SUNXI_TIGER_LCD_TIMING_DELAY_RESET]  = rgb_panel->delay.reset;
	reg->value[SUNXI_TIGER_LCD_RESERVED2]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED3]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED4]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED5]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED6]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED7]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED8]           = 99999;
	reg->value[SUNXI_TIGER_LCD_RESERVED9]           = 99999;
	/* RGB */
	reg->value[SUNXI_TIGER_LCD_RESERVED10]          = 99999;
	reg->value[SUNXI_TIGER_LCD_FLAG_FOOTER]         = 1;

	memcpy(arg, reg, sizeof(struct lcd_timing));

	for (i = 0; i < LCD_REG_COUNT; i++)
		DRM_ERROR("======= value[%d] = %d =======\n", i, reg->value[i]);

	kfree(reg);
	kfree(vm);
}

static const struct drm_encoder_helper_funcs sunxi_rgb_encoder_helper_funcs = {
	.atomic_enable = sunxi_drm_rgb_encoder_atomic_enable,
	.atomic_disable = sunxi_drm_rgb_encoder_atomic_disable,
	.atomic_check = sunxi_drm_rgb_encoder_atomic_check,
	.atomic_mode_set = sunxi_drm_rgb_encoder_atomic_mode_set,
	/* .loader_protect = sunxi_drm_rgb_encoder_loader_protect, */
};

static int drm_rgb_connector_set_property(struct drm_connector *connector,
		struct drm_connector_state *state,
		struct drm_property *property,
		uint64_t val)
{
	return 0;

}
static int drm_rgb_connector_get_property(struct drm_connector *connector,
		const struct drm_connector_state *state,
		struct drm_property *property,
		uint64_t *val)
{
	return 0;

}

static void drm_rgb_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

void drm_rgb_atomic_print_state(struct drm_printer *p,
				   const struct drm_connector_state *state)
{
	struct disp_video_timings timings;
	struct resource *res = NULL;
	unsigned long pclk = 0;
	struct sunxi_drm_rgb *rgb = connector_to_sunxi_drm_rgb(state->connector);
	u64 refresh_rate_raw, irqcnt, vblank_cnt, refresh_rate_integer, refresh_rate_fractional;
	struct sunxi_tcon *tcon;

	tcon = dev_get_drvdata(rgb->sdrm.tcon_dev);
	tcon_lcd_get_timing(&tcon->tcon_lcd, &timings);

	refresh_rate_raw = sunxi_tcon_get_refreshraw_and_vblankcnt(rgb->sdrm.tcon_dev);
	irqcnt = atomic64_read(&tcon->cnt.irqcnt);
	vblank_cnt = atomic64_read(&tcon->cnt.vblank_cnt);

	refresh_rate_integer = div64_u64_rem(refresh_rate_raw, 1000, &refresh_rate_fractional);

	drm_printf(p, "\t%s\n", rgb->enable ? "on:" : "off");
	if (rgb->enable) {
		drm_printf(p, "\tirqcnt:%llu\tvblank_cnt:%llu\tRefresh Rate:%llu.%llu\n", irqcnt, vblank_cnt, refresh_rate_integer, refresh_rate_fractional);
		drm_printf(p, "\tclk source: %s\n", rgb->displl_clk ? "displl" : "ccmu");
		if (rgb->pclk) {
			pclk = clk_get_rate(rgb->pclk);
			drm_printf(p, "\t\tpixel_clk rate to be set:%luKHz, real pixel_clk rate:%luKHz\n",
					rgb->pclk_clk_rate / 1000, pclk / 1000);
		}

		drm_printf(p, "\t hsync-len | hback-porch |  hactive  | hfront-porch | vsync-len | vback-porch | vactive | vfront-porch \n");
		drm_printf(p, "\t-----------+-------------+-----------+--------------+-----------+-------------+---------+--------------\n");
		drm_printf(p, "\t    %3d    |    %4d     |   %4d    |     %4d     |    %3d    |    %4d     |   %4d  |     %4d\n",
				timings.hor_sync_time, timings.hor_back_porch, timings.x_res, timings.hor_front_porch,
				timings.ver_sync_time, timings.ver_back_porch, timings.y_res, timings.ver_front_porch);

		res = sunxi_tcon_get_res(rgb->sdrm.tcon_dev);
		drm_printf(p, "\v******* tcon reg dump ********\n");
		if (res) {
			print_physical_memory(p, res, 0, 0x17c);
			print_physical_memory(p, res, 0x220, 0x30);
		}
	}
}

static const struct drm_connector_funcs sunxi_rgb_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_rgb_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.atomic_set_property = drm_rgb_connector_set_property,
	.atomic_get_property = drm_rgb_connector_get_property,
	.atomic_print_state = drm_rgb_atomic_print_state,
};

static int sunxi_rgb_connector_get_modes(struct drm_connector *connector)
{
	struct sunxi_drm_rgb *rgb = connector_to_sunxi_drm_rgb(connector);

	DRM_INFO("[RGB]%s start\n", __FUNCTION__);
	return drm_panel_get_modes(rgb->sdrm.panel, connector);
}

static const struct drm_connector_helper_funcs sunxi_rgb_connector_helper_funcs = {
	.get_modes = sunxi_rgb_connector_get_modes,
};

s32 sunxi_rgb_parse_dt(struct device *dev)
{
	struct sunxi_drm_rgb *rgb = dev_get_drvdata(dev);

	rgb->phy = devm_phy_get(dev, "combophy0");
	if (IS_ERR_OR_NULL(rgb->phy)) {
		DRM_INFO("rgb%d's combophy0 not setting, maybe not used!\n", rgb->sdrm.hw_id);
		rgb->phy = NULL;
	}

	rgb->pclk = devm_clk_get_optional(dev, "rgb_pclk");
	if (IS_ERR(rgb->pclk)) {
		DRM_ERROR("fail to get rgb_pclk\n");
	}

	return 0;
}

static ssize_t colorbar_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sunxi_drm_rgb *rgb = dev_get_drvdata(dev);
	int colorbar_mode = sunxi_tcon_pattern_get(rgb->sdrm.tcon_dev);

	return sysfs_emit(buf,
		"\nNow the colorbar has been set %d\n"
		"\nUsage:\n"
		"\techo [Source] > colorbar\n"
		"\nSource:\n"
		"\t0:DE\t1:Color\t2:Grayscale\t3:Black by White\n"
		"\t4:All 0\t5:All 1\t6:Reserved\t7:Gridding\n",
		colorbar_mode);
}

static ssize_t colorbar_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_drm_rgb *rgb = dev_get_drvdata(dev);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret || val > 8)
		return -EINVAL;

	sunxi_tcon_show_pattern(rgb->sdrm.tcon_dev, val);

	return count;
}

static ssize_t reset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf,
		"\nPanel Reset Control\n"
		"\nUsage:\n"
		"\techo [Count] > reset\n"
		"\nParameters:\n"
		"\tCount: Number of reset cycles (1-9999)\n"
		"\nExample:\n"
		"\techo 3 > reset  # Reset panel 3 times\n");
}

static ssize_t reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_drm_rgb *rgb = dev_get_drvdata(dev);
	unsigned int val, i;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val >= 10000)
		return -EINVAL;

	for (i = 0; i < val; i++) {
		sunxi_drm_mode_config_reset(rgb->sdrm.drm_dev);
		msleep(500);
		DRM_INFO("%s:reset time:%u\n", __func__, i);
	}

	return count;
}

static DEVICE_ATTR_RW(colorbar);
static DEVICE_ATTR_RW(reset);

static struct attribute *rgb_attrs[] = {
	&dev_attr_colorbar.attr,
	&dev_attr_reset.attr,
	NULL
};

static const struct attribute_group rgb_attr_group = {
	.name  = "attr",
	.attrs = rgb_attrs,
};

int sunxi_rgb_init_sysfs(struct sunxi_drm_rgb *drm_rgb)
{
	int ret;
	/* Create a path: sys/class/rgb */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	drm_rgb->rgb_class = class_create(THIS_MODULE, "rgb");
#else
	drm_rgb->rgb_class = class_create("rgb");
#endif
	if (IS_ERR(drm_rgb->rgb_class)) {
		DRM_ERROR("rgb class_create fail\n");
		return PTR_ERR(drm_rgb->rgb_class);
	}

	/* Create a path "sys/class/rgb/rgb" */
	drm_rgb->sunxi_rgb_sysfs_dev = device_create(drm_rgb->rgb_class, NULL, MKDEV(0, 0), drm_rgb, "rgb");
	if (IS_ERR(drm_rgb->sunxi_rgb_sysfs_dev)) {
		DRM_ERROR("rgb device_create fail\n");
		ret = PTR_ERR(drm_rgb->sunxi_rgb_sysfs_dev);
		goto DESTROY_CLASS;
	}

	/* Create a path: sys/class/rgb/rgb/attr */
	ret = sysfs_create_group(&drm_rgb->sunxi_rgb_sysfs_dev->kobj, &rgb_attr_group);
	if (ret) {
		DRM_ERROR("rgb sysfs_create_group failed!\n");
		goto DESTROY_DEVICE;
	}

	return 0;

DESTROY_DEVICE:
	device_destroy(drm_rgb->rgb_class, MKDEV(0, 0));
DESTROY_CLASS:
	class_destroy(drm_rgb->rgb_class);

	return ret;
}

static int sunxi_drm_rgb_bind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = (struct drm_device *)data;
	struct device *tcon_lcd_dev = NULL;
	struct sunxi_drm_rgb *rgb = dev_get_drvdata(dev);
	struct sunxi_drm_device *sdrm = &rgb->sdrm;
	int ret, tcon_id;

	DRM_INFO("[RGB]%s start\n", __FUNCTION__);
	ret = sunxi_rgb_parse_dt(dev);
	if (ret) {
		DRM_ERROR("sunxi_tcon_parse_dts failed\n");
	}

	ret = drm_of_find_panel_or_bridge(dev->of_node, 1, -1,
			&rgb->sdrm.panel, &rgb->sdrm.bridge);
	if (ret) {
		DRM_DEV_ERROR(dev, "Failed to find panel or bridge: %d\n", ret);
		return ret;
	}

	tcon_lcd_dev = drm_rgb_of_get_tcon(rgb->dev);
	if (tcon_lcd_dev == NULL) {
		DRM_ERROR("tcon_lcd for rgb not found!\n");
		return -ENODEV;
	}
	tcon_id = sunxi_tcon_of_get_id(tcon_lcd_dev);

	sdrm->tcon_dev = tcon_lcd_dev;
	sdrm->tcon_id = tcon_id;
	sdrm->drm_dev = drm;
	drm_encoder_helper_add(&sdrm->encoder, &sunxi_rgb_encoder_helper_funcs);
	ret = drm_simple_encoder_init(drm, &sdrm->encoder, DRM_MODE_ENCODER_DPI);
	if (ret) {
		DRM_ERROR("Couldn't initialise the encoder for tcon %d\n", tcon_id);
		return ret;
	}

	sdrm->encoder.possible_crtcs =
			drm_of_find_possible_crtcs(drm, tcon_lcd_dev->of_node);

	if (rgb->sdrm.panel) {
		drm_connector_helper_add(&sdrm->connector,
				&sunxi_rgb_connector_helper_funcs);

		ret = drm_connector_init(drm, &sdrm->connector,
				&sunxi_rgb_connector_funcs,
				DRM_MODE_CONNECTOR_DPI);
		if (ret) {
			drm_encoder_cleanup(&sdrm->encoder);
			DRM_ERROR("[RGB]Couldn't initialise the connector for tcon %d\n", tcon_id);
			return ret;
		}

		drm_connector_attach_encoder(&sdrm->connector, &sdrm->encoder);
	} else if (rgb->sdrm.bridge) {
		ret = drm_bridge_attach(&sdrm->encoder, rgb->sdrm.bridge, NULL, 0);
		if (ret) {
			drm_encoder_cleanup(&sdrm->encoder);
			DRM_ERROR("[RGB]failed to attach bridge %d\n", ret);
			return ret;
		}
	}

	ret = sunxi_rgb_init_sysfs(rgb);
	if (ret)
		DRM_ERROR("rgb init sysfs fail!\n");

	rgb->bound = true;
	DRM_INFO("[RGB]%s ok\n", __FUNCTION__);

	return 0;
}

static void sunxi_drm_rgb_unbind(struct device *dev, struct device *master,
				void *data)
{
	struct sunxi_drm_rgb *rgb = dev_get_drvdata(dev);

	sysfs_remove_group(&rgb->sunxi_rgb_sysfs_dev->kobj, &rgb_attr_group);
	device_destroy(rgb->rgb_class, MKDEV(0, 0));

	drm_connector_cleanup(&rgb->sdrm.connector);
	drm_encoder_cleanup(&rgb->sdrm.encoder);
	rgb->bound = false;
}

static const struct component_ops sunxi_drm_rgb_component_ops = {
	.bind = sunxi_drm_rgb_bind,
	.unbind = sunxi_drm_rgb_unbind,
};


static int sunxi_drm_rgb_probe(struct platform_device *pdev)
{
	struct sunxi_drm_rgb *rgb;
	struct device *dev = &pdev->dev;

	DRM_INFO("[RGB]%s start\n", __FUNCTION__);
	rgb = devm_kzalloc(dev, sizeof(*rgb), GFP_KERNEL);
	if (!rgb)
		return -ENOMEM;

	rgb->rgb_data = of_device_get_match_data(dev);
	if (!rgb->rgb_data) {
		DRM_ERROR("sunxi_drm_rgb fail to get match data\n");
		return -ENODEV;
	}
	rgb->sdrm.hw_id = rgb->rgb_data->id;
	rgb->dev = dev;

	dev_set_drvdata(dev, rgb);
	platform_set_drvdata(pdev, rgb);

	rgb->sdrm.get_disp_para = sunxi_get_disp_rgb_para;
	rgb->sdrm.set_disp_para = sunxi_set_disp_rgb_para;

	DRM_INFO("[RGB]%s ok\n", __FUNCTION__);

	return component_add(&pdev->dev, &sunxi_drm_rgb_component_ops);
}

static int sunxi_drm_rgb_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sunxi_drm_rgb_component_ops);
	return 0;
}

struct platform_driver sunxi_rgb_platform_driver = {
	.probe = sunxi_drm_rgb_probe,
	.remove = sunxi_drm_rgb_remove,
	.driver = {
		.name = "drm-rgb",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_drm_rgb_match,
	},
};
