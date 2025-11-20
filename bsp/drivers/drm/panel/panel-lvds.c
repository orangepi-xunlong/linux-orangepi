// SPDX-License-Identifier: GPL-2.0+
/*
 * Generic LVDS panel driver
 *
 * Copyright (C) 2023 Allwinner.
 *
 */

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/version.h>

#include <drm/drm_crtc.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include "panel-lvds.h"

static void panel_lvds_sleep(unsigned int msec)
{
	if (msec > 20)
		msleep(msec);
	else
		usleep_range(msec * 1000, (msec + 1) * 1000);
}

static int panel_lvds_disable(struct drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);

	if (lvds->backlight)
		backlight_disable(lvds->backlight);

	return 0;
}

static int panel_lvds_unprepare(struct drm_panel *panel)
{
	struct panel_lvds *lvds_panel = to_panel_lvds(panel);
	int i;


	for (i = GPIO_MAX; i > 0; i--) {
		if (lvds_panel->enable_gpio[i - 1]) {
			gpiod_set_value_cansleep(lvds_panel->enable_gpio[i - 1], 0);
			if (lvds_panel->delay.enable)
				panel_lvds_sleep(lvds_panel->delay.enable);
		}
	}

	if (lvds_panel->reset_num) {
		if (lvds_panel->reset_gpio)
			gpiod_set_value_cansleep(lvds_panel->reset_gpio, 0);
		if (lvds_panel->delay.reset)
			panel_lvds_sleep(lvds_panel->delay.reset);
	}

	for (i = POWER_MAX; i > 0; i--) {
		if (lvds_panel->supply[i - 1]) {
			regulator_disable(lvds_panel->supply[i - 1]);
			if (lvds_panel->delay.power)
				panel_lvds_sleep(lvds_panel->delay.power);
		}
	}

	return 0;
}
int panel_lvds_regulator_enable(struct drm_panel *panel)
{
	struct panel_lvds *lvds_panel = to_panel_lvds(panel);
	int err, i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	panel->prepared = true;
#endif
	for (i = 0; i < POWER_MAX; i++) {
		if (lvds_panel->supply[i]) {
			err = regulator_enable(lvds_panel->supply[i]);
			if (err < 0) {
				dev_err(lvds_panel->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (lvds_panel->delay.power)
				panel_lvds_sleep(lvds_panel->delay.power);
		}
	}
	return 0;
}
EXPORT_SYMBOL(panel_lvds_regulator_enable);

bool panel_lvds_is_support_backlight(struct drm_panel *panel)
{
	return panel->backlight;
}
EXPORT_SYMBOL(panel_lvds_is_support_backlight);

int panel_lvds_get_backlight_value(struct drm_panel *panel)
{
	if (panel->backlight)
		return backlight_get_brightness(panel->backlight);

	return 0;
}
EXPORT_SYMBOL(panel_lvds_get_backlight_value);

void panel_lvds_set_backlight_value(struct drm_panel *panel, int brightness)
{
	if (!panel->backlight || backlight_is_blank(panel->backlight) || brightness <= 0)
		return ;

	// TODO: support backlight mapping
	panel->backlight->props.brightness = brightness;
	backlight_update_status(panel->backlight);
}
EXPORT_SYMBOL(panel_lvds_set_backlight_value);

static int panel_lvds_prepare(struct drm_panel *panel)
{
	struct panel_lvds *lvds_panel = to_panel_lvds(panel);
	int i;

	panel_lvds_regulator_enable(panel);
	for (i = 0; i < GPIO_MAX; i++) {
		if (lvds_panel->enable_gpio[i]) {
			gpiod_set_value_cansleep(lvds_panel->enable_gpio[i], 1);

			if (lvds_panel->delay.enable)
				panel_lvds_sleep(lvds_panel->delay.enable);
		}
	}

	for (i = 0; i < lvds_panel->reset_num; i++) {
		if (lvds_panel->reset_gpio)
			gpiod_set_value_cansleep(lvds_panel->reset_gpio, 0);
		if (lvds_panel->delay.reset)
			panel_lvds_sleep(lvds_panel->delay.reset);
		if (lvds_panel->reset_gpio)
			gpiod_set_value_cansleep(lvds_panel->reset_gpio, 1);
		if (lvds_panel->delay.reset)
			panel_lvds_sleep(lvds_panel->delay.reset);
	}

	return 0;
}

static int panel_lvds_enable(struct drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);

	if (lvds->backlight) {
		lvds->backlight->props.state &= ~BL_CORE_FBBLANK;
		lvds->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(lvds->backlight);
	}

	return 0;
}

static int panel_lvds_get_modes(struct drm_panel *panel,
			struct drm_connector *connector)
{
	struct panel_lvds *lvds_panel = to_panel_lvds(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode)
		return 0;

	drm_display_mode_from_videomode(&lvds_panel->video_mode, mode);
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
	drm_display_info_set_bus_formats(&connector->display_info,
					&lvds_panel->bus_format, 1);
/*
	connector->display_info.width_mm = lvds_panel->width;
	connector->display_info.height_mm = lvds_panel->height;
	drm_display_info_set_bus_formats(&connector->display_info,
					&lvds_panel->bus_format, 1);
	connector->display_info.bus_flags = lvds_panel->data_mirror
					? DRM_BUS_FLAG_DATA_LSB_TO_MSB
					: DRM_BUS_FLAG_DATA_MSB_TO_LSB;
*/
	drm_connector_set_panel_orientation(connector, lvds_panel->orientation);

	return 1;
}

static const struct drm_panel_funcs panel_lvds_funcs = {
	.disable = panel_lvds_disable,
	.unprepare = panel_lvds_unprepare,
	.prepare = panel_lvds_prepare,
	.enable = panel_lvds_enable,
	.get_modes = panel_lvds_get_modes,
};

static int panel_lvds_parse_dt(struct panel_lvds *lvds_panel)
{
	struct device_node *np = lvds_panel->dev->of_node;
	struct display_timings *disp = NULL;
	struct display_timing *timing = NULL;
	char *power_name = NULL;
	char *gpio_name = NULL;
	int ret, i;

	ret = of_drm_get_panel_orientation(np, &lvds_panel->orientation);
	if (ret < 0) {
		lvds_panel->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	}

	disp = of_get_display_timings(np);
	if (!disp) {
		dev_err(lvds_panel->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	timing = display_timings_get(disp, disp->native_mode);
	if (!timing) {
		dev_err(lvds_panel->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	videomode_from_timing(timing, &lvds_panel->video_mode);

	for (i = 0; i < POWER_MAX; i++) {
		power_name = kasprintf(GFP_KERNEL, "power%d", i);
		lvds_panel->supply[i] = devm_regulator_get_optional(lvds_panel->dev, power_name);
		if (IS_ERR(lvds_panel->supply[i])) {
			ret = PTR_ERR(lvds_panel->supply[i]);

			if (ret != -ENODEV) {
				if (ret != -EPROBE_DEFER)
					dev_err(lvds_panel->dev,
						"failed to request regulator(%s): %d\n",
						power_name, ret);
				return ret;
			}

			lvds_panel->supply[i] = NULL;
		}
	}

	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		gpio_name = kasprintf(GFP_KERNEL, "enable%d", i);
		lvds_panel->enable_gpio[i] =
			devm_gpiod_get_optional(lvds_panel->dev, gpio_name, GPIOD_OUT_HIGH);
		if (IS_ERR(lvds_panel->enable_gpio[i])) {
			ret = PTR_ERR(lvds_panel->enable_gpio[i]);
			dev_err(lvds_panel->dev, "failed to request %s GPIO: %d\n", gpio_name,
				ret);
			return ret;
		}
	}

	lvds_panel->reset_gpio =
		devm_gpiod_get_optional(lvds_panel->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(lvds_panel->reset_gpio)) {
		ret = PTR_ERR(lvds_panel->reset_gpio);
		dev_err(lvds_panel->dev, "failed to request %s GPIO: %d\n", "reset",
			ret);
		return ret;
	}
/*
	ret = of_property_read_u32(np, "width-mm", &lvds_panel->width);
	if (ret < 0) {
		dev_err(lvds_panel->dev, "%pOF: invalid or missing %s DT property\n",
			np, "width-mm");
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "height-mm", &lvds_panel->height);
	if (ret < 0) {
		dev_err(lvds_panel->dev, "%pOF: invalid or missing %s DT property\n",
			np, "height-mm");
		return -ENODEV;
	}

	of_property_read_string(np, "label", &lvds_panel->label);
*/
	of_property_read_u32(np, "power-delay-ms", &lvds_panel->delay.power);
	of_property_read_u32(np, "enable-delay-ms", &lvds_panel->delay.enable);
	of_property_read_u32(np, "reset-delay-ms", &lvds_panel->delay.reset);
	of_property_read_u32(np, "reset-num", &lvds_panel->reset_num);

	ret = of_property_read_u32(np, "bus-format", &lvds_panel->bus_format);
	if (ret < 0) {
		dev_err(lvds_panel->dev, "%pOF: invalid or missing %s DT property\n",
			np, "bus-format");
		return -ENODEV;
	}

	lvds_panel->data_mirror = of_property_read_bool(np, "data-mirror");

	return 0;
}

static int panel_lvds_probe(struct platform_device *pdev)
{
	struct panel_lvds *lvds_panel;
	int ret;

	DRM_WARN("[LVDS-PANEL] panel_lvds_probe start\n");
	lvds_panel = devm_kzalloc(&pdev->dev, sizeof(*lvds_panel), GFP_KERNEL);
	if (!lvds_panel)
		return -ENOMEM;

	lvds_panel->funcs = devm_kzalloc(&pdev->dev, sizeof(*lvds_panel->funcs), GFP_KERNEL);
	if (!lvds_panel->funcs)
		return -ENOMEM;

	lvds_panel->dev = &pdev->dev;

	ret = panel_lvds_parse_dt(lvds_panel);
	if (ret < 0)
		return ret;


	drm_panel_init(&lvds_panel->panel, lvds_panel->dev, &panel_lvds_funcs,
					DRM_MODE_CONNECTOR_LVDS);

	ret = drm_panel_of_backlight(&lvds_panel->panel);
	if (ret)
		return ret;

	drm_panel_add(&lvds_panel->panel);

	dev_set_drvdata(lvds_panel->dev, lvds_panel);
	DRM_WARN("[LVDS-PANEL] panel_lvds_probe finish\n");
	return 0;
}

static int panel_lvds_remove(struct platform_device *pdev)
{
	struct panel_lvds *lvds_panel = dev_get_drvdata(&pdev->dev);

	drm_panel_remove(&lvds_panel->panel);

				panel_lvds_disable(&lvds_panel->panel);
				panel_lvds_unprepare(&lvds_panel->panel);

	return 0;
}

static const struct of_device_id panel_lvds_of_table[] = {
	{ .compatible = "sunxi-lvds", },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, panel_lvds_of_table);

static struct platform_driver panel_lvds_driver = {
	.probe = panel_lvds_probe,
	.remove = panel_lvds_remove,
	.driver = {
		.name = "sunxi-lvds",
		.of_match_table = panel_lvds_of_table,
	},
};

module_platform_driver(panel_lvds_driver);

MODULE_AUTHOR("xiaozhineng <xiaozhineng@allwinnertech.com>");
MODULE_DESCRIPTION("LVDS Panel Driver");
MODULE_VERSION("1.0.3");
MODULE_LICENSE("GPL");
