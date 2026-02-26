// SPDX-License-Identifier: GPL-2.0+
/*
 * Generic RGB panel driver
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
#include "panel-rgb.h"

static void panel_rgb_sleep(unsigned int msec)
{
	if (msec > 20)
		msleep(msec);
	else
		usleep_range(msec * 1000, (msec + 1) * 1000);
}

static int panel_rgb_disable(struct drm_panel *panel)
{
	struct panel_rgb *rgb_panel = to_panel_rgb(panel);

	if (rgb_panel->backlight)
		backlight_disable(rgb_panel->backlight);

	return 0;
}

static int panel_rgb_unprepare(struct drm_panel *panel)
{
	struct panel_rgb *rgb_panel = to_panel_rgb(panel);
	int i;

	for (i = GPIO_MAX; i > 0; i--) {
		if (rgb_panel->enable_gpio[i - 1]) {
			gpiod_set_value_cansleep(rgb_panel->enable_gpio[i - 1], 0);
			if (rgb_panel->delay.enable)
				panel_rgb_sleep(rgb_panel->delay.enable);
		}
	}

	if (rgb_panel->reset_num) {
		if (rgb_panel->reset_gpio)
			gpiod_set_value_cansleep(rgb_panel->reset_gpio, 0);
		if (rgb_panel->delay.reset)
			panel_rgb_sleep(rgb_panel->delay.reset);
	}

	for (i = POWER_MAX; i > 0; i--) {
		if (rgb_panel->supply[i - 1]) {
			regulator_disable(rgb_panel->supply[i - 1]);
			if (rgb_panel->delay.power)
				panel_rgb_sleep(rgb_panel->delay.power);
		}
	}

	return 0;
}
int panel_rgb_regulator_enable(struct drm_panel *panel)
{
	struct panel_rgb *rgb_panel = to_panel_rgb(panel);
	int err, i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	panel->prepared = true;
#endif
	for (i = 0; i < POWER_MAX; i++) {
		if (rgb_panel->supply[i]) {
			err = regulator_enable(rgb_panel->supply[i]);
			if (err < 0) {
				dev_err(rgb_panel->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (rgb_panel->delay.power)
				panel_rgb_sleep(rgb_panel->delay.power);
		}
	}
	return 0;
}
EXPORT_SYMBOL(panel_rgb_regulator_enable);

static int panel_rgb_prepare(struct drm_panel *panel)
{
	struct panel_rgb *rgb_panel = to_panel_rgb(panel);
	int i;

	panel_rgb_regulator_enable(panel);
	for (i = 0; i < GPIO_MAX; i++) {
		if (rgb_panel->enable_gpio[i]) {
			gpiod_set_value_cansleep(rgb_panel->enable_gpio[i], 1);

			if (rgb_panel->delay.enable)
				panel_rgb_sleep(rgb_panel->delay.enable);
		}
	}

	for (i = 0; i < rgb_panel->reset_num; i++) {
		if (rgb_panel->reset_gpio)
				gpiod_set_value_cansleep(rgb_panel->reset_gpio, 0);
		if (rgb_panel->delay.reset)
				panel_rgb_sleep(rgb_panel->delay.reset);
		if (rgb_panel->reset_gpio)
				gpiod_set_value_cansleep(rgb_panel->reset_gpio, 1);
		if (rgb_panel->delay.reset)
				panel_rgb_sleep(rgb_panel->delay.reset);
	}

	return 0;
}

bool panel_rgb_is_support_backlight(struct drm_panel *panel)
{
	return panel->backlight;
}
EXPORT_SYMBOL(panel_rgb_is_support_backlight);

int panel_rgb_get_backlight_value(struct drm_panel *panel)
{
	if (panel->backlight)
		return backlight_get_brightness(panel->backlight);

	return 0;
}
EXPORT_SYMBOL(panel_rgb_get_backlight_value);

void panel_rgb_set_backlight_value(struct drm_panel *panel, int brightness)
{
	if (!panel->backlight || backlight_is_blank(panel->backlight) || brightness <= 0)
		return ;

	// TODO: support backlight mapping
	panel->backlight->props.brightness = brightness;
	backlight_update_status(panel->backlight);
}
EXPORT_SYMBOL(panel_rgb_set_backlight_value);

static int panel_rgb_enable(struct drm_panel *panel)
{
	struct panel_rgb *rgb_panel = to_panel_rgb(panel);

	if (rgb_panel->backlight) {
		rgb_panel->backlight->props.state &= ~BL_CORE_FBBLANK;
		rgb_panel->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(rgb_panel->backlight);
	}

	return 0;
}

static int panel_rgb_get_modes(struct drm_panel *panel,
			struct drm_connector *connector)
{
	struct panel_rgb *rgb_panel = to_panel_rgb(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode)
		return 0;

	drm_display_mode_from_videomode(&rgb_panel->video_mode, mode);
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
	drm_display_info_set_bus_formats(&connector->display_info,
					&rgb_panel->bus_format, 1);
/*
	connector->display_info.width_mm = rgb_panel->width;
	connector->display_info.height_mm = rgb_panel->height;
	drm_display_info_set_bus_formats(&connector->display_info,
					&rgb_panel->bus_format, 1);
	connector->display_info.bus_flags = rgb_panel->data_mirror
					? DRM_BUS_FLAG_DATA_LSB_TO_MSB
					: DRM_BUS_FLAG_DATA_MSB_TO_LSB;
*/
	drm_connector_set_panel_orientation(connector, rgb_panel->orientation);

	return 1;
}

static const struct drm_panel_funcs panel_rgb_funcs = {
	.disable = panel_rgb_disable,
	.unprepare = panel_rgb_unprepare,
	.prepare = panel_rgb_prepare,
	.enable = panel_rgb_enable,
	.get_modes = panel_rgb_get_modes,
};

static int panel_rgb_parse_dt(struct panel_rgb *rgb_panel)
{
	struct device_node *np = rgb_panel->dev->of_node;
	struct display_timings *disp = NULL;
	struct display_timing *timing = NULL;
	char *power_name = NULL;
	char *gpio_name = NULL;
//	const char *mapping;
	int ret, i;

	ret = of_drm_get_panel_orientation(np, &rgb_panel->orientation);
	if (ret < 0) {
		rgb_panel->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	}

	disp = of_get_display_timings(np);
	if (!disp) {
		dev_err(rgb_panel->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	timing = display_timings_get(disp, disp->native_mode);
	if (!timing) {
		dev_err(rgb_panel->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	videomode_from_timing(timing, &rgb_panel->video_mode);

	for (i = 0; i < POWER_MAX; i++) {
		power_name = kasprintf(GFP_KERNEL, "power%d", i);
		rgb_panel->supply[i] = devm_regulator_get_optional(rgb_panel->dev, power_name);
		if (IS_ERR(rgb_panel->supply[i])) {
			ret = PTR_ERR(rgb_panel->supply[i]);

			if (ret != -ENODEV) {
				if (ret != -EPROBE_DEFER)
					dev_err(rgb_panel->dev,
						"failed to request regulator(%s): %d\n",
						power_name, ret);
				return ret;
			}

			rgb_panel->supply[i] = NULL;
		}
	}

	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		gpio_name = kasprintf(GFP_KERNEL, "enable%d", i);
		rgb_panel->enable_gpio[i] =
			devm_gpiod_get_optional(rgb_panel->dev, gpio_name, GPIOD_OUT_HIGH);
		if (IS_ERR(rgb_panel->enable_gpio[i])) {
			ret = PTR_ERR(rgb_panel->enable_gpio[i]);
			dev_err(rgb_panel->dev, "failed to request %s GPIO: %d\n", gpio_name,
				ret);
			return ret;
		}
	}

	rgb_panel->reset_gpio =
		devm_gpiod_get_optional(rgb_panel->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(rgb_panel->reset_gpio)) {
		ret = PTR_ERR(rgb_panel->reset_gpio);
		dev_err(rgb_panel->dev, "failed to request %s GPIO: %d\n", "reset",
			ret);
		return ret;
	}
	of_property_read_u32(np, "power-delay-ms", &rgb_panel->delay.power);
	of_property_read_u32(np, "enable-delay-ms", &rgb_panel->delay.enable);
	of_property_read_u32(np, "reset-delay-ms", &rgb_panel->delay.reset);
	of_property_read_u32(np, "reset-num", &rgb_panel->reset_num);
	of_property_read_u32(np, "bus-format", &rgb_panel->bus_format);
/*
	ret = of_property_read_u32(np, "width-mm", &rgb->width);
	if (ret < 0) {
		dev_err(rgb->dev, "%pOF: invalid or missing %s DT property\n",
			np, "width-mm");
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "height-mm", &rgb->height);
	if (ret < 0) {
		dev_err(rgb->dev, "%pOF: invalid or missing %s DT property\n",
			np, "height-mm");
		return -ENODEV;
	}

	of_property_read_string(np, "label", &rgb->label);

	ret = of_property_read_string(np, "data-mapping", &mapping);
	if (ret < 0) {
		dev_err(rgb->dev, "%pOF: invalid or missing %s DT property\n",
			np, "data-mapping");
		return -ENODEV;
	}

	if (!strcmp(mapping, "jeida-18")) {
		rgb->bus_format = MEDIA_BUS_FMT_RGB666_1X7X3_SPWG;
	} else if (!strcmp(mapping, "jeida-24")) {
		rgb->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA;
	} else if (!strcmp(mapping, "vesa-24")) {
		rgb->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;
	} else {
		dev_err(rgb->dev, "%pOF: invalid or missing %s DT property\n",
			np, "data-mapping");
		return -EINVAL;
	}

	rgb->data_mirror = of_property_read_bool(np, "data-mirror");
*/
	return 0;
}

static int panel_rgb_probe(struct platform_device *pdev)
{
	struct panel_rgb *rgb_panel;
	int ret;

	DRM_WARN("[RGB-PANEL] panel_rgb_probe start\n");
	rgb_panel = devm_kzalloc(&pdev->dev, sizeof(*rgb_panel), GFP_KERNEL);
	if (!rgb_panel)
		return -ENOMEM;

	rgb_panel->funcs = devm_kzalloc(&pdev->dev, sizeof(*rgb_panel->funcs), GFP_KERNEL);
	if (!rgb_panel->funcs)
		return -ENOMEM;

	rgb_panel->dev = &pdev->dev;

	ret = panel_rgb_parse_dt(rgb_panel);
	if (ret < 0)
		return ret;


	drm_panel_init(&rgb_panel->panel, rgb_panel->dev, &panel_rgb_funcs,
					DRM_MODE_CONNECTOR_DPI);

	ret = drm_panel_of_backlight(&rgb_panel->panel);
	if (ret)
		return ret;

	drm_panel_add(&rgb_panel->panel);

	dev_set_drvdata(rgb_panel->dev, rgb_panel);
	DRM_WARN("[RGB-PANEL] panel_rgb_probe finish\n");
	return 0;
}

static int panel_rgb_remove(struct platform_device *pdev)
{
	struct panel_rgb *rgb_panel = dev_get_drvdata(&pdev->dev);

	drm_panel_remove(&rgb_panel->panel);

				panel_rgb_disable(&rgb_panel->panel);
				panel_rgb_unprepare(&rgb_panel->panel);

	return 0;
}

static const struct of_device_id panel_rgb_of_table[] = {
	{ .compatible = "sunxi-rgb", },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, panel_rgb_of_table);

static struct platform_driver panel_rgb_driver = {
	.probe = panel_rgb_probe,
	.remove = panel_rgb_remove,
	.driver = {
		.name = "sunxi-rgb",
		.of_match_table = panel_rgb_of_table,
	},
};

module_platform_driver(panel_rgb_driver);

MODULE_AUTHOR("xiaozhineng <xiaozhineng@allwinnertech.com>");
MODULE_DESCRIPTION("RGB Panel Driver");
MODULE_VERSION("1.0.3");
MODULE_LICENSE("GPL");
