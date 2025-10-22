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
#include <video/videomode.h>
#include "panels.h"

#define POWER_MAX 3
#define GPIO_MAX 3
struct panel_lvds {
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

static void panel_lvds_sleep(unsigned int msec)
{
	if (msec > 20)
		msleep(msec);
	else
		usleep_range(msec * 1000, (msec + 1) * 1000);
}
static inline struct panel_lvds *to_panel_lvds(struct drm_panel *panel)
{
	return container_of(panel, struct panel_lvds, panel);
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
	struct panel_lvds *lvds = to_panel_lvds(panel);
	int i;


	for (i = GPIO_MAX; i > 0; i--) {
		if (lvds->enable_gpio[i - 1]) {
			gpiod_set_value_cansleep(lvds->enable_gpio[i - 1], 0);
			if (lvds->delay.enable)
				panel_lvds_sleep(lvds->delay.enable);
		}
	}

	if (lvds->reset_gpio)
		gpiod_set_value_cansleep(lvds->reset_gpio, 0);
	if (lvds->delay.reset)
		panel_lvds_sleep(lvds->delay.reset);

	for (i = POWER_MAX; i > 0; i--) {
		if (lvds->supply[i - 1]) {
			regulator_disable(lvds->supply[i - 1]);
			if (lvds->delay.power)
				panel_lvds_sleep(lvds->delay.power);
		}
	}

	return 0;
}
int panel_lvds_regulator_enable(struct drm_panel *panel)
{
	struct panel_lvds *lvds = to_panel_lvds(panel);
	int err, i;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	panel->prepared = true;
#endif
	for (i = 0; i < POWER_MAX; i++) {
		if (lvds->supply[i]) {
			err = regulator_enable(lvds->supply[i]);
			if (err < 0) {
				dev_err(lvds->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
			if (lvds->delay.power)
				panel_lvds_sleep(lvds->delay.power);
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
	struct panel_lvds *lvds = to_panel_lvds(panel);
	int i;

	panel_lvds_regulator_enable(panel);
	for (i = 0; i < GPIO_MAX; i++) {
		if (lvds->enable_gpio[i]) {
			gpiod_set_value_cansleep(lvds->enable_gpio[i], 1);

			if (lvds->delay.enable)
				panel_lvds_sleep(lvds->delay.enable);
		}
	}

	if (lvds->reset_gpio)
		gpiod_set_value_cansleep(lvds->reset_gpio, 1);
	if (lvds->delay.reset)
		panel_lvds_sleep(lvds->delay.reset);

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
	struct panel_lvds *lvds = to_panel_lvds(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_create(connector->dev);
	if (!mode)
		return 0;

	drm_display_mode_from_videomode(&lvds->video_mode, mode);
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
	drm_display_info_set_bus_formats(&connector->display_info,
					&lvds->bus_format, 1);
/*
	connector->display_info.width_mm = lvds->width;
	connector->display_info.height_mm = lvds->height;
	drm_display_info_set_bus_formats(&connector->display_info,
					&lvds->bus_format, 1);
	connector->display_info.bus_flags = lvds->data_mirror
						? DRM_BUS_FLAG_DATA_LSB_TO_MSB
						: DRM_BUS_FLAG_DATA_MSB_TO_LSB;
*/
	drm_connector_set_panel_orientation(connector, lvds->orientation);

	return 1;
}

static const struct drm_panel_funcs panel_lvds_funcs = {
	.disable = panel_lvds_disable,
	.unprepare = panel_lvds_unprepare,
	.prepare = panel_lvds_prepare,
	.enable = panel_lvds_enable,
	.get_modes = panel_lvds_get_modes,
};

static int panel_lvds_parse_dt(struct panel_lvds *lvds)
{
	struct device_node *np = lvds->dev->of_node;
	struct display_timings *disp = NULL;
	struct display_timing *timing = NULL;
	char *power_name = NULL;
	char *gpio_name = NULL;
	int ret, i;

	ret = of_drm_get_panel_orientation(np, &lvds->orientation);
	if (ret < 0) {
		lvds->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	}

	disp = of_get_display_timings(np);
	if (!disp) {
		dev_err(lvds->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	timing = display_timings_get(disp, disp->native_mode);
	if (!timing) {
		dev_err(lvds->dev, "%pOF: problems parsing panel-timin\n",
			np);
		return -ENODEV;
	}
	videomode_from_timing(timing, &lvds->video_mode);

	for (i = 0; i < POWER_MAX; i++) {
		power_name = kasprintf(GFP_KERNEL, "power%d", i);
		lvds->supply[i] = devm_regulator_get_optional(lvds->dev, power_name);
		if (IS_ERR(lvds->supply[i])) {
			ret = PTR_ERR(lvds->supply[i]);

			if (ret != -ENODEV) {
				if (ret != -EPROBE_DEFER)
					dev_err(lvds->dev,
						"failed to request regulator(%s): %d\n",
						power_name, ret);
				return ret;
			}

			lvds->supply[i] = NULL;
		}
	}

	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		gpio_name = kasprintf(GFP_KERNEL, "enable%d", i);
		lvds->enable_gpio[i] =
			devm_gpiod_get_optional(lvds->dev, gpio_name, GPIOD_OUT_HIGH);
		if (IS_ERR(lvds->enable_gpio[i])) {
			ret = PTR_ERR(lvds->enable_gpio[i]);
			dev_err(lvds->dev, "failed to request %s GPIO: %d\n", gpio_name,
				ret);
			return ret;
		}
	}

	lvds->reset_gpio =
		devm_gpiod_get_optional(lvds->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(lvds->reset_gpio)) {
		ret = PTR_ERR(lvds->reset_gpio);
		dev_err(lvds->dev, "failed to request %s GPIO: %d\n", "reset",
			ret);
		return ret;
	}
/*
	ret = of_property_read_u32(np, "width-mm", &lvds->width);
	if (ret < 0) {
		dev_err(lvds->dev, "%pOF: invalid or missing %s DT property\n",
			np, "width-mm");
		return -ENODEV;
	}
	ret = of_property_read_u32(np, "height-mm", &lvds->height);
	if (ret < 0) {
		dev_err(lvds->dev, "%pOF: invalid or missing %s DT property\n",
			np, "height-mm");
		return -ENODEV;
	}

	of_property_read_string(np, "label", &lvds->label);
*/
	of_property_read_u32(np, "power-delay-ms", &lvds->delay.power);
	of_property_read_u32(np, "enable-delay-ms", &lvds->delay.enable);
	of_property_read_u32(np, "reset-delay-ms", &lvds->delay.reset);

	ret = of_property_read_u32(np, "bus-format", &lvds->bus_format);
	if (ret < 0) {
		dev_err(lvds->dev, "%pOF: invalid or missing %s DT property\n",
			np, "bus-format");
		return -ENODEV;
	}

	lvds->data_mirror = of_property_read_bool(np, "data-mirror");

	return 0;
}

static int panel_lvds_probe(struct platform_device *pdev)
{
	struct panel_lvds *lvds;
	int ret;

	DRM_WARN("[LVDS-PANEL] panel_lvds_probe start\n");
	lvds = devm_kzalloc(&pdev->dev, sizeof(*lvds), GFP_KERNEL);
	if (!lvds)
		return -ENOMEM;

	lvds->dev = &pdev->dev;

	ret = panel_lvds_parse_dt(lvds);
	if (ret < 0)
		return ret;


	drm_panel_init(&lvds->panel, lvds->dev, &panel_lvds_funcs,
					DRM_MODE_CONNECTOR_LVDS);

	ret = drm_panel_of_backlight(&lvds->panel);
	if (ret)
		return ret;

	drm_panel_add(&lvds->panel);
	if (ret < 0)
		return ret;

	dev_set_drvdata(lvds->dev, lvds);
	DRM_WARN("[LVDS-PANEL] panel_lvds_probe finish\n");
	return 0;
}

static int panel_lvds_remove(struct platform_device *pdev)
{
	struct panel_lvds *lvds = dev_get_drvdata(&pdev->dev);

	drm_panel_remove(&lvds->panel);

				panel_lvds_disable(&lvds->panel);
				panel_lvds_unprepare(&lvds->panel);

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
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
