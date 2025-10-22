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

#include <linux/component.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/gpio.h>
#include <linux/phy/phy.h>
#include <linux/phy/phy-mipi-dphy.h>
#include <linux/media-bus-format.h>
#include <linux/version.h>
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
#include "panel/panels.h"
#define PHY_ENABLE 1

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2) \
	|| IS_ENABLED(CONFIG_ARCH_SUN65IW1) || IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define LVDS_DISPLL_CLK
#endif
struct lvds_data {
	int id;
};
struct sunxi_drm_lvds {
	struct sunxi_drm_device sdrm;
	struct drm_display_mode mode;
	struct disp_lvds_para lvds_para;
	bool bound;
	bool sw_enable;
	struct device *dev;
	struct phy *phy0;
	struct phy *phy1;
	union phy_configure_opts phy_opts;

	const struct lvds_data *lvds_data;

	struct reset_control *rst_bus;
	struct clk *pclk;
	unsigned long mode_flags;
	unsigned long pclk_clk_rate;

};
static const struct lvds_data lvds0_data = {
	.id = 0,
};

static const struct lvds_data lvds1_data = {
	.id = 1,
};

static const struct of_device_id sunxi_drm_lvds_match[] = {
	{ .compatible = "allwinner,lvds0", .data = &lvds0_data },
	{ .compatible = "allwinner,lvds1", .data = &lvds1_data },
	{},
};

static struct device *drm_lvds_of_get_tcon(struct device *lvds_dev)
{
	struct device_node *node = lvds_dev->of_node;
	struct device_node *tcon_lcd_node;
	struct device_node *lvds_in_tcon;
	struct platform_device *pdev = NULL;
	struct device *tcon_lcd_dev = NULL;;

	lvds_in_tcon = of_graph_get_endpoint_by_regs(node, 0, 0);
	if (!lvds_in_tcon) {
		DRM_ERROR("endpoint lvds_in_tcon not fount\n");
		return NULL;
	}

	tcon_lcd_node = of_graph_get_remote_port_parent(lvds_in_tcon);
	if (!tcon_lcd_node) {
		DRM_ERROR("node tcon_lcd not fount\n");
		tcon_lcd_dev = NULL;
		goto LVDS_PUT;
	}

	pdev = of_find_device_by_node(tcon_lcd_node);
	if (!pdev) {
		DRM_ERROR("tcon_lcd platform device not fount\n");
		tcon_lcd_dev = NULL;
		goto TCON_LVDS_PUT;
	}

	tcon_lcd_dev = &pdev->dev;
	platform_device_put(pdev);

TCON_LVDS_PUT:
	of_node_put(tcon_lcd_node);
LVDS_PUT:
	of_node_put(lvds_in_tcon);

	return tcon_lcd_dev;
}

static inline struct sunxi_drm_lvds *encoder_to_sunxi_drm_lvds(struct drm_encoder *encoder)
{
	struct sunxi_drm_device *sdrm = container_of(encoder, struct sunxi_drm_device, encoder);

	return container_of(sdrm, struct sunxi_drm_lvds, sdrm);
}

static inline struct sunxi_drm_lvds *connector_to_sunxi_drm_lvds(struct drm_connector *connector)
{
	struct sunxi_drm_device *sdrm = container_of(connector, struct sunxi_drm_device, connector);

	return container_of(sdrm, struct sunxi_drm_lvds, sdrm);
}

static int sunxi_lvds_clk_config_enable(struct sunxi_drm_lvds *lvds)
{
	int ret = 0;

	if (lvds->rst_bus) {
		ret = reset_control_deassert(lvds->rst_bus);
		if (ret) {
			DRM_ERROR("reset_control_deassert for rst_bus_lvds failed\n");
			return ret;
		}
	}

	return ret;
}

static int sunxi_lvds_clk_config_disable(struct sunxi_drm_lvds *lvds)
{
	int ret = 0;

	if (lvds->rst_bus)
		ret = reset_control_assert(lvds->rst_bus);

	return ret;
}

static int sunxi_lvds_displl_enable(struct sunxi_drm_lvds *lvds)
{
	if (lvds->pclk) {
		clk_set_rate(lvds->pclk, lvds->pclk_clk_rate);
		clk_prepare_enable(lvds->pclk);
	}

	return 0;
}

static int sunxi_lvds_displl_disable(struct sunxi_drm_lvds *lvds)
{
	if (lvds->pclk) {
		clk_set_rate(lvds->pclk, 24000000);
		clk_disable_unprepare(lvds->pclk);
	}

	return 0;
}

static int sunxi_lcd_pin_set_state(struct device *dev, char *name)
{
	int ret;
	struct pinctrl *pctl;
	struct pinctrl_state *state;

	DRM_INFO("[LVDS] %s start\n", __FUNCTION__);
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

void sunxi_drm_lvds_encoder_atomic_enable(struct drm_encoder *encoder,
					struct drm_atomic_state *state)
{
	int ret;
	struct drm_crtc *crtc = encoder->crtc;
	int de_hw_id = sunxi_drm_crtc_get_hw_id(crtc);
	struct drm_crtc_state *crtc_state = crtc->state;
	struct sunxi_drm_lvds *lvds = encoder_to_sunxi_drm_lvds(encoder);
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct disp_output_config disp_cfg;

	drm_mode_to_sunxi_video_timings(&lvds->mode, &lvds->lvds_para.timings);

	memset(&disp_cfg, 0, sizeof(struct disp_output_config));
	memcpy(&disp_cfg.lvds_para, &lvds->lvds_para,
		sizeof(lvds->lvds_para));
	disp_cfg.type = INTERFACE_LVDS;
	disp_cfg.de_id = de_hw_id;
	disp_cfg.irq_handler = sunxi_crtc_event_proc;
	disp_cfg.irq_data = scrtc_state->base.crtc;
	disp_cfg.sw_enable = lvds->sw_enable;
	disp_cfg.tcon_lcd_div = 7;
#ifdef LVDS_DISPLL_CLK
	disp_cfg.displl_clk = true;
#else
	disp_cfg.displl_clk = false;
#endif
	if (lvds->lvds_data->id == 1)
		disp_cfg.displl_clk = false;
	sunxi_tcon_mode_init(lvds->sdrm.tcon_dev, &disp_cfg);

	lvds->pclk_clk_rate = lvds->lvds_para.timings.pixel_clk * disp_cfg.tcon_lcd_div;
	ret = sunxi_lvds_clk_config_enable(lvds);
	if (ret) {
		DRM_ERROR("lvds clk enable failed\n");
		return;
	}

	sunxi_lcd_pin_set_state(lvds->dev, "active");

	if (lvds->sw_enable) {
		if (lvds->phy0) {
			phy_power_on(lvds->phy0);
			if (lvds->pclk)
				clk_prepare_enable(lvds->pclk);
		}
		if (lvds->phy1)
			phy_power_on(lvds->phy1);

		panel_lvds_regulator_enable(lvds->sdrm.panel);
	} else {
		if (lvds->phy0) {
			phy_power_on(lvds->phy0);
			phy_set_mode_ext(lvds->phy0, PHY_MODE_LVDS, PHY_ENABLE);
		}
		if (lvds->phy1) {
			phy_power_on(lvds->phy1);
			phy_set_mode_ext(lvds->phy1, PHY_MODE_LVDS, PHY_ENABLE);
		}
		drm_panel_prepare(lvds->sdrm.panel);

		sunxi_lvds_displl_enable(lvds);

		ret = sunxi_lvds_enable_output(lvds->sdrm.tcon_dev);
		if (ret < 0)
			DRM_DEV_INFO(lvds->dev, "failed to enable lvds ouput\n");
	}
	drm_panel_enable(lvds->sdrm.panel);

	DRM_INFO("[LVDS] %s finish\n", __FUNCTION__);

	return;
}

void sunxi_drm_lvds_encoder_atomic_disable(struct drm_encoder *encoder,
					struct drm_atomic_state *state)
{
	struct sunxi_drm_lvds *lvds = encoder_to_sunxi_drm_lvds(encoder);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	lvds->sdrm.panel->prepare_prev_first = false;
#endif
	drm_panel_disable(lvds->sdrm.panel);
	sunxi_lvds_displl_disable(lvds);
	drm_panel_unprepare(lvds->sdrm.panel);

	sunxi_lvds_clk_config_disable(lvds);
	if (lvds->phy0)
		phy_power_off(lvds->phy0);

	if (lvds->phy1)
		phy_power_off(lvds->phy1);

	sunxi_lcd_pin_set_state(lvds->dev, "sleep");
	sunxi_lvds_disable_output(lvds->sdrm.tcon_dev);
	sunxi_tcon_mode_exit(lvds->sdrm.tcon_dev);
	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
}

static bool sunxi_lvds_fifo_check(void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	return sunxi_tcon_check_fifo_status(lvds->sdrm.tcon_dev);
}

int sunxi_lvds_get_current_line(void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	return sunxi_tcon_get_current_line(lvds->sdrm.tcon_dev);
}

static bool sunxi_lvds_is_sync_time_enough(void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	return sunxi_tcon_is_sync_time_enough(lvds->sdrm.tcon_dev);
}

static void sunxi_lvds_enable_vblank(bool enable, void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;

	sunxi_tcon_enable_vblank(lvds->sdrm.tcon_dev, enable);
}

static bool sunxi_lvds_is_support_backlight(void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	return panel_lvds_is_support_backlight(lvds->sdrm.panel);
}

static int sunxi_lvds_get_backlight_value(void *data)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	return panel_lvds_get_backlight_value(lvds->sdrm.panel);
}

static void sunxi_lvds_set_backlight_value(void *data, int brightness)
{
	struct sunxi_drm_lvds *lvds = (struct sunxi_drm_lvds *)data;
	panel_lvds_set_backlight_value(lvds->sdrm.panel, brightness);
}

int sunxi_drm_lvds_encoder_atomic_check(struct drm_encoder *encoder,
				struct drm_crtc_state *crtc_state,
				struct drm_connector_state *conn_state)
{
	struct sunxi_crtc_state *scrtc_state = to_sunxi_crtc_state(crtc_state);
	struct sunxi_drm_lvds *lvds = encoder_to_sunxi_drm_lvds(encoder);

	/* FIXME:TODO: color_fmt/clolor_depth update by actual configuration */
//	scrtc_state->color_fmt = DISP_CSC_TYPE_RGB;
//	scrtc_state->color_depth = DISP_DATA_8BITS;
	scrtc_state->tcon_id = lvds->sdrm.tcon_id;
	scrtc_state->enable_vblank = sunxi_lvds_enable_vblank;
	scrtc_state->is_sync_time_enough = sunxi_lvds_is_sync_time_enough;
	scrtc_state->get_cur_line = sunxi_lvds_get_current_line;
	scrtc_state->is_support_backlight = sunxi_lvds_is_support_backlight;
	scrtc_state->get_backlight_value = sunxi_lvds_get_backlight_value;
	scrtc_state->set_backlight_value = sunxi_lvds_set_backlight_value;
	scrtc_state->check_status = sunxi_lvds_fifo_check;
	scrtc_state->output_dev_data = lvds;

	if (conn_state->crtc) {
		lvds->sw_enable = sunxi_drm_check_if_need_sw_enable(conn_state->connector);
		scrtc_state->sw_enable = lvds->sw_enable;
	}
	DRM_DEBUG_DRIVER("%s finish\n", __FUNCTION__);
	return 0;
}

static void sunxi_drm_lvds_encoder_mode_set(struct drm_encoder *encoder,
					struct drm_display_mode *mode,
					struct drm_display_mode *adj_mode)
{
	struct sunxi_drm_lvds *lvds = encoder_to_sunxi_drm_lvds(encoder);
	struct disp_lvds_para *lvds_para = &lvds->lvds_para;
	struct drm_connector *connector = &lvds->sdrm.connector;
	struct drm_display_info *info = &connector->display_info;
	u32 bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;

	DRM_INFO("[LVDS]%s start\n", __FUNCTION__);
	if (info->num_bus_formats)
		bus_format = info->bus_formats[0];
	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA:  // jeida-24
		lvds_para->lvds_data_mode = 1;
		lvds_para->lvds_colordepth = 0;
		break;
	case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG:   // vesa-18
		lvds_para->lvds_data_mode = 0;
		lvds_para->lvds_colordepth = 1;
		break;
	case MEDIA_BUS_FMT_RGB888_1X7X4_SPWG:   // vesa-24
		lvds_para->lvds_data_mode = 0;
		lvds_para->lvds_colordepth = 0;
		break;
	default:
		;
	}

	drm_mode_copy(&lvds->mode, adj_mode);
	DRM_INFO("[LVDS]%s finish\n", __FUNCTION__);

}

static const struct drm_encoder_helper_funcs sunxi_lvds_encoder_helper_funcs = {
	.atomic_enable = sunxi_drm_lvds_encoder_atomic_enable,
	.atomic_disable = sunxi_drm_lvds_encoder_atomic_disable,
	.atomic_check = sunxi_drm_lvds_encoder_atomic_check,
	.mode_set = sunxi_drm_lvds_encoder_mode_set,
//	.loader_protect = sunxi_drm_lvds_encoder_loader_protect,
};

static int drm_lvds_connector_set_property(struct drm_connector *connector,
		struct drm_connector_state *state,
		struct drm_property *property,
		uint64_t val)
{
	return 0;

}
static int drm_lvds_connector_get_property(struct drm_connector *connector,
		const struct drm_connector_state *state,
		struct drm_property *property,
		uint64_t *val)
{
	return 0;

}

static void drm_lvds_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static const struct drm_connector_funcs sunxi_lvds_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = drm_lvds_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
	.atomic_set_property = drm_lvds_connector_set_property,
	.atomic_get_property = drm_lvds_connector_get_property,
};

static int sunxi_lvds_connector_get_modes(struct drm_connector *connector)
{
	struct sunxi_drm_lvds *lvds = connector_to_sunxi_drm_lvds(connector);

	DRM_INFO("[LVDS]%s start\n", __FUNCTION__);
	return drm_panel_get_modes(lvds->sdrm.panel, connector);
}

static const struct drm_connector_helper_funcs sunxi_lvds_connector_helper_funcs = {
	.get_modes = sunxi_lvds_connector_get_modes,
};

s32 sunxi_lvds_parse_dt(struct device *dev)
{
	s32 ret = -1;
	s32 value = 0;
	struct sunxi_drm_lvds *lvds = dev_get_drvdata(dev);
	struct sunxi_drm_device *sdrm = &lvds->sdrm;
	struct disp_lvds_para *lvds_para = &lvds->lvds_para;

	if (!sdrm->hw_id) {
		lvds->phy0 = devm_phy_get(dev, "combophy0");
		if (IS_ERR_OR_NULL(lvds->phy0)) {
			DRM_INFO("lvds%d's combophy0 not setting, maybe not used!\n", sdrm->hw_id);
			lvds->phy0 = NULL;
		}
	}

	ret = of_property_read_u32(dev->of_node, "dual-channel", &value);
	if (!ret) {
		lvds_para->dual_lvds = value;
	}

	if (lvds_para->dual_lvds && !sdrm->hw_id) {
		lvds->phy1 = devm_phy_get(dev, "combophy1");
		if (IS_ERR_OR_NULL(lvds->phy1)) {
			DRM_INFO("lvds%d's combophy1 not setting, maybe not used!\n", sdrm->hw_id);
			lvds->phy1 = NULL;
		}
	}
	lvds->pclk = devm_clk_get_optional(dev, "lvds_pclk");
	if (IS_ERR(lvds->pclk)) {
		DRM_ERROR("fail to get lvds_pclk\n");
	}

	lvds->rst_bus = devm_reset_control_get_shared(dev, "rst_bus_lvds");
	if (IS_ERR(lvds->rst_bus)) {
		DRM_ERROR("fail to get reset rst_bus_lvds\n");
		return -EINVAL;
	}

	return 0;
}
static int sunxi_drm_lvds_bind(struct device *dev, struct device *master, void *data)
{
	struct drm_device *drm = (struct drm_device *)data;
	struct device *tcon_lcd_dev = NULL;
	struct sunxi_drm_lvds *lvds = dev_get_drvdata(dev);
	struct sunxi_drm_device *sdrm = &lvds->sdrm;
	int ret, tcon_id;

	DRM_INFO("[LVDS]%s start\n", __FUNCTION__);
	ret = sunxi_lvds_parse_dt(dev);
	if (ret) {
		DRM_ERROR("sunxi_tcon_parse_dts failed\n");
	}
	ret = drm_of_find_panel_or_bridge(dev->of_node, 1, -1,
			&lvds->sdrm.panel, &lvds->sdrm.bridge);
	if (ret) {
		DRM_DEV_ERROR(dev, "Failed to find panel or bridge: %d\n", ret);
		return ret;
	}

	tcon_lcd_dev = drm_lvds_of_get_tcon(lvds->dev);
	if (tcon_lcd_dev == NULL) {
		DRM_ERROR("tcon_lcd for lvds not found!\n");
		ret = -1;
	}
	tcon_id = sunxi_tcon_of_get_id(tcon_lcd_dev);

	sdrm->tcon_dev = tcon_lcd_dev;
	sdrm->tcon_id = tcon_id;
	sdrm->drm_dev = drm;

	drm_encoder_helper_add(&sdrm->encoder, &sunxi_lvds_encoder_helper_funcs);
	ret = drm_simple_encoder_init(drm, &sdrm->encoder, DRM_MODE_ENCODER_LVDS);
	if (ret) {
		DRM_ERROR("Couldn't initialise the encoder for tcon %d\n", tcon_id);
		return ret;
	}

	sdrm->encoder.possible_crtcs =
			drm_of_find_possible_crtcs(drm, tcon_lcd_dev->of_node);
	if (sdrm->panel) {
		drm_connector_helper_add(&sdrm->connector,
				&sunxi_lvds_connector_helper_funcs);

		ret = drm_connector_init(drm, &sdrm->connector,
				&sunxi_lvds_connector_funcs,
				DRM_MODE_CONNECTOR_LVDS);
		if (ret) {
			drm_encoder_cleanup(&sdrm->encoder);
			DRM_ERROR("[LVDS]Couldn't initialise the connector for tcon %d\n", tcon_id);
			return ret;
		}

		drm_connector_attach_encoder(&sdrm->connector, &sdrm->encoder);
	//	tcon_dev->cfg.private_data = lvds;
	} else {
		ret = drm_bridge_attach(&sdrm->encoder, sdrm->bridge, NULL, 0);
		if (ret) {
			drm_encoder_cleanup(&sdrm->encoder);
			DRM_ERROR("[LVDS]failed to attach bridge %d\n", ret);
			return ret;
		}
	}

	lvds->bound = true;
	DRM_INFO("[LVDS]%s ok\n", __FUNCTION__);

	return 0;
}

static void sunxi_drm_lvds_unbind(struct device *dev, struct device *master,
				void *data)
{
	struct sunxi_drm_lvds *lvds = dev_get_drvdata(dev);

	drm_connector_cleanup(&lvds->sdrm.connector);
	drm_encoder_cleanup(&lvds->sdrm.encoder);
	lvds->bound = false;
}

static const struct component_ops sunxi_drm_lvds_component_ops = {
	.bind = sunxi_drm_lvds_bind,
	.unbind = sunxi_drm_lvds_unbind,
};


static int sunxi_drm_lvds_probe(struct platform_device *pdev)
{
	struct sunxi_drm_lvds *lvds;
	struct device *dev = &pdev->dev;

	DRM_INFO("[LVDS] sunxi_drm_lvds_probe start\n");
	lvds = devm_kzalloc(dev, sizeof(*lvds), GFP_KERNEL);
	if (!lvds)
		return -ENOMEM;

	lvds->lvds_data = of_device_get_match_data(dev);
	if (!lvds->lvds_data) {
		DRM_ERROR("sunxi_drm_lvds fail to get match data\n");
		return -ENODEV;
	}
	lvds->sdrm.hw_id = lvds->lvds_data->id;
	lvds->dev = dev;

	dev_set_drvdata(dev, lvds);
	platform_set_drvdata(pdev, lvds);

	DRM_INFO("[LVDS]%s ok\n", __FUNCTION__);

	return component_add(&pdev->dev, &sunxi_drm_lvds_component_ops);
}

static int sunxi_drm_lvds_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &sunxi_drm_lvds_component_ops);
	return 0;
}

struct platform_driver sunxi_lvds_platform_driver = {
	.probe = sunxi_drm_lvds_probe,
	.remove = sunxi_drm_lvds_remove,
	.driver = {
		.name = "drm-lvds",
		.owner = THIS_MODULE,
		.of_match_table = sunxi_drm_lvds_match,
	},
};
