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
#include <video/videomode.h>
#include "panels.h"

#define POWER_MAX 3
#define GPIO_MAX 3
struct panel_rgb {
	struct drm_panel panel;
	struct device *dev;

	const char *label;
	unsigned int width;
	unsigned int height;
	struct videomode video_mode;
	unsigned int bus_format;
	bool data_mirror;
	struct {
		unsigned int power;
		unsigned int enable;
		unsigned int reset;
	} delay;

	struct backlight_device *backlight;
	struct regulator *supply[POWER_MAX];

	struct gpio_desc *enable_gpio[GPIO_MAX];
	struct gpio_desc *reset_gpio;
	enum drm_panel_orientation orientation;
};

static void panel_rgb_sleep(unsigned int msec)
{
	if (msec > 20)
		msleep(msec);
	else
		usleep_range(msec * 1000, (msec + 1) * 1000);
}
static inline struct panel_rgb *to_panel_rgb(struct drm_panel *panel)
{
	return container_of(panel, struct panel_rgb, panel);
}

static int panel_rgb_disable(struct drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);

	if (rgb->backlight)
		backlight_disable(rgb->backlight);

	return 0;
}

static int panel_rgb_unprepare(struct drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);
	int i;

	for (i = GPIO_MAX; i > 0; i--) {
		if (rgb->enable_gpio[i - 1]) {
			gpiod_set_value_cansleep(rgb->enable_gpio[i - 1], 0);
			if (rgb->delay.enable)
				panel_rgb_sleep(rgb->delay.enable);
		}
	}

	if (rgb->reset_gpio)
		gpiod_set_value_cansleep(rgb->reset_gpio, 0);
	if (rgb->delay.reset)
		panel_rgb_sleep(rgb->delay.reset);

	for (i = POWER_MAX; i > 0; i--) {
		if (rgb->supply[i - 1]) {
			regulator_disable(rgb->supply[i - 1]);
			if (rgb->delay.power)
				panel_rgb_sleep(rgb->delay.power);
		}
	}

	return 0;
}
int panel_rgb_regulator_enable(struct drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);
	int err, i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	panel->prepared = true;
#endif
	for (i = 0; i < POWER_MAX; i++) {
		if (rgb->supply[i]) {
			err = regulator_enable(rgb->supply[i]);
			if (err < 0) {
				dev_err(rgb->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (rgb->delay.power)
				panel_rgb_sleep(rgb->delay.power);
		}
	}
	return 0;
}
EXPORT_SYMBOL(panel_rgb_regulator_enable);

static int panel_rgb_prepare(struct drm_panel *panel)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);
	int i;

	panel_rgb_regulator_enable(panel);
	for (i = 0; i < GPIO_MAX; i++) {
		if (rgb->enable_gpio[i]) {
			gpiod_set_value_cansleep(rgb->enable_gpio[i], 1);

			if (rgb->delay.enable)
				panel_rgb_sleep(rgb->delay.enable);
		}
	}

	if (rgb->reset_gpio)
		gpiod_set_value_cansleep(rgb->reset_gpio, 1);
	if (rgb->delay.reset)
		panel_rgb_sleep(rgb->delay.reset);

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
	struct panel_rgb *rgb = to_panel_rgb(panel);

	if (rgb->backlight) {
		rgb->backlight->props.state &= ~BL_CORE_FBBLANK;
		rgb->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(rgb->backlight);
	}

	return 0;
}

static int panel_rgb_get_modes(struct drm_panel *panel,
			struct drm_connector *connector)
{
	struct panel_rgb *rgb = to_panel_rgb(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode)
		return 0;

	drm_display_mode_from_videomode(&rgb->video_mode, mode);
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
/*
	connector->display_info.width_mm = rgb->width;
	connector->display_info.height_mm = rgb->height;
	drm_display_info_set_bus_formats(&connector->display_info,
					&rgb->bus_format, 1);
	connector->display_info.bus_flags = rgb->data_mirror
						? DRM_BUS_FLAG_DATA_LSB_TO_MSB
						: DRM_BUS_FLAG_DATA_MSB_TO_LSB;
*/
	drm_connector_set_panel_orientation(connector, rgb->orientation);

	return 1;
}

static const struct drm_panel_funcs panel_rgb_funcs = {
	.disable = panel_rgb_disable,
	.unprepare = panel_rgb_unprepare,
	.prepare = panel_rgb_prepare,
	.enable = panel_rgb_enable,
	.get_modes = panel_rgb_get_modes,
};

static int panel_rgb_parse_dt(struct panel_rgb *rgb)
{
	struct device_node *np = rgb->dev->of_node;
	struct display_timings *disp = NULL;
	struct display_timing *timing = NULL;
	char *power_name = NULL;
	char *gpio_name = NULL;
//	const char *mapping;
	int ret, i;

	ret = of_drm_get_panel_orientation(np, &rgb->orientation);
	if (ret < 0) {
		rgb->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	}

	disp = of_get_display_timings(np);
	if (!disp) {
		dev_err(rgb->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	timing = display_timings_get(disp, disp->native_mode);
	if (!timing) {
		dev_err(rgb->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	videomode_from_timing(timing, &rgb->video_mode);

	for (i = 0; i < POWER_MAX; i++) {
		power_name = kasprintf(GFP_KERNEL, "power%d", i);
		rgb->supply[i] = devm_regulator_get_optional(rgb->dev, power_name);
		if (IS_ERR(rgb->supply[i])) {
			ret = PTR_ERR(rgb->supply[i]);

			if (ret != -ENODEV) {
				if (ret != -EPROBE_DEFER)
					dev_err(rgb->dev,
						"failed to request regulator(%s): %d\n",
						power_name, ret);
				return ret;
			}

			rgb->supply[i] = NULL;
		}
	}

	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		gpio_name = kasprintf(GFP_KERNEL, "enable%d", i);
		rgb->enable_gpio[i] =
			devm_gpiod_get_optional(rgb->dev, gpio_name, GPIOD_OUT_HIGH);
		if (IS_ERR(rgb->enable_gpio[i])) {
			ret = PTR_ERR(rgb->enable_gpio[i]);
			dev_err(rgb->dev, "failed to request %s GPIO: %d\n", gpio_name,
				ret);
			return ret;
		}
	}

	rgb->reset_gpio =
		devm_gpiod_get_optional(rgb->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(rgb->reset_gpio)) {
		ret = PTR_ERR(rgb->reset_gpio);
		dev_err(rgb->dev, "failed to request %s GPIO: %d\n", "reset",
			ret);
		return ret;
	}
	of_property_read_u32(np, "power-delay-ms", &rgb->delay.power);
	of_property_read_u32(np, "enable-delay-ms", &rgb->delay.enable);
	of_property_read_u32(np, "reset-delay-ms", &rgb->delay.reset);
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
	struct panel_rgb *rgb;
	int ret;

	DRM_WARN("[RGB-PANEL] panel_rgb_probe start\n");
	rgb = devm_kzalloc(&pdev->dev, sizeof(*rgb), GFP_KERNEL);
	if (!rgb)
		return -ENOMEM;

	rgb->dev = &pdev->dev;

	ret = panel_rgb_parse_dt(rgb);
	if (ret < 0)
		return ret;


	drm_panel_init(&rgb->panel, rgb->dev, &panel_rgb_funcs,
					DRM_MODE_CONNECTOR_DPI);

	ret = drm_panel_of_backlight(&rgb->panel);
	if (ret)
		return ret;

	drm_panel_add(&rgb->panel);
	if (ret < 0)
		return ret;

	dev_set_drvdata(rgb->dev, rgb);
	DRM_WARN("[RGB-PANEL] panel_rgb_probe finish\n");
	return 0;
}

static int panel_rgb_remove(struct platform_device *pdev)
{
	struct panel_rgb *rgb = dev_get_drvdata(&pdev->dev);

	drm_panel_remove(&rgb->panel);

				panel_rgb_disable(&rgb->panel);
				panel_rgb_unprepare(&rgb->panel);

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
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
