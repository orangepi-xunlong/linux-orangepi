// SPDX-License-Identifier: GPL-2.0+
/*
 * Generic LVDS panel driver
 *
 * Copyright (C) 2016 Laurent Pinchart
 * Copyright (C) 2016 Renesas Electronics Corporation
 *
 * Contact: Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>

#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <drm/drm_connector.h>
#include <drm/drm_modes.h>
#include <drm/drm_edid.h>
#include <drm/drm_property.h>
#include <linux/backlight.h>

#define POWER_MAX 3
#define GPIO_MAX  3

struct general_panel {
	struct drm_panel panel;
	struct device *dev;
	struct videomode video_mode;
	struct regulator *supply[POWER_MAX];
	struct gpio_desc *enable_gpio[GPIO_MAX];
	struct gpio_desc *reset_gpio;
	enum drm_panel_orientation orientation;
	unsigned int power_delay_ms;
	unsigned int disable_delay_ms;
};

bool general_panel_edp_is_support_backlight(struct drm_panel *panel)
{
	return panel->backlight;
}
EXPORT_SYMBOL(general_panel_edp_is_support_backlight);

int general_panel_edp_get_backlight_value(struct drm_panel *panel)
{
	if (panel->backlight)
		return backlight_get_brightness(panel->backlight);

	return 0;
}
EXPORT_SYMBOL(general_panel_edp_get_backlight_value);

void general_panel_edp_set_backlight_value(struct drm_panel *panel, int brightness)
{
	if (!panel->backlight || backlight_is_blank(panel->backlight) || brightness <= 0)
		return ;

	// TODO: support backlight mapping
	panel->backlight->props.brightness = brightness;
	backlight_update_status(panel->backlight);
}
EXPORT_SYMBOL(general_panel_edp_set_backlight_value);

static void general_panel_sleep(int msec)
{
	mdelay(msec);
}

static inline struct general_panel *to_general_panel(struct drm_panel *panel)
{
	return container_of(panel, struct general_panel, panel);
}

static int general_panel_unprepare(struct drm_panel *panel)
{
	struct general_panel *edp_panel = to_general_panel(panel);
	int i;

	for (i = GPIO_MAX; i > 0; i--) {
		if (edp_panel->enable_gpio[i - 1])
			gpiod_set_value_cansleep(edp_panel->enable_gpio[i - 1], 0);
	}

	for (i = POWER_MAX; i > 0; i--) {
		if (edp_panel->supply[i - 1]) {
			regulator_disable(edp_panel->supply[i - 1]);
			msleep(10);
		}
	}
	general_panel_sleep(edp_panel->disable_delay_ms);

	return 0;
}

static int general_panel_prepare(struct drm_panel *panel)
{
	struct general_panel *edp_panel = to_general_panel(panel);
	int i;
	int err = 0;

	for (i = 0; i < POWER_MAX; i++) {
		if (edp_panel->supply[i]) {
			err = regulator_enable(edp_panel->supply[i]);
			if (err < 0) {
				dev_err(edp_panel->dev, "failed to enable supply%d: %d\n",
					i, err);
				return err;
			}
		}
	}

	for (i = 0; i < GPIO_MAX; i++) {
		if (edp_panel->enable_gpio[i])
			gpiod_set_value_cansleep(edp_panel->enable_gpio[i], 1);
	}
	general_panel_sleep(edp_panel->power_delay_ms);

	return 0;
}

static int general_panel_disable(struct drm_panel *panel)
{
	return 0;

}

static int general_panel_enable(struct drm_panel *panel)
{
	return 0;

}

static int general_panel_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct general_panel *edp_panel = to_general_panel(panel);
	struct drm_display_mode *mode = NULL;
	struct edid *edid = NULL;
	int mode_num = 0;

	if ((edp_panel->video_mode.hactive != 0) &&
	    (edp_panel->video_mode.vactive != 0) &&
	    (edp_panel->video_mode.pixelclock != 0)) {
		mode = drm_mode_create(connector->dev);
		if (!mode)
			return 0;

		drm_display_mode_from_videomode(&edp_panel->video_mode, mode);
		mode->type |= DRM_MODE_TYPE_USERDEF;
		drm_mode_probed_add(connector, mode);
		mode_num++;
	}

	if (connector->edid_blob_ptr)
		edid = drm_edid_duplicate(connector->edid_blob_ptr->data);
	else
		DRM_ERROR("edid for connector not update yet!\n");

	mode_num += drm_add_edid_modes(connector, edid);

	drm_connector_set_panel_orientation(connector, edp_panel->orientation);

	return mode_num;
}

static const struct drm_panel_funcs general_panel_funcs = {
	.unprepare = general_panel_unprepare,
	.prepare = general_panel_prepare,
	.disable = general_panel_disable,
	.enable = general_panel_enable,
	.get_modes = general_panel_get_modes,
};

static int general_panel_parse_dt(struct general_panel *edp_panel)
{
	struct device_node *np = edp_panel->dev->of_node;
	struct display_timing timing;
	int ret;

	ret = of_drm_get_panel_orientation(np, &edp_panel->orientation);
	if (ret < 0) {
		edp_panel->orientation = DRM_MODE_PANEL_ORIENTATION_NORMAL;
	}

	ret = of_get_display_timing(np, "panel-timing", &timing);
	if (ret < 0) {
		dev_err(edp_panel->dev, "%pOF: problems parsing panel-timing (%d)\n",
			np, ret);
		return ret;
	}

	videomode_from_timing(&timing, &edp_panel->video_mode);

	return 0;
}

static int general_panel_probe(struct platform_device *pdev)
{
	struct general_panel *edp_panel;
	char *power_name = NULL;
	char *gpio_name = NULL;
	int ret;
	int i = 0;

	edp_panel = devm_kzalloc(&pdev->dev, sizeof(*edp_panel), GFP_KERNEL);
	if (!edp_panel)
		return -ENOMEM;

	edp_panel->dev = &pdev->dev;

	ret = general_panel_parse_dt(edp_panel);
	if (ret < 0) {
		DRM_WARN("general edp panel timings parse from dts fail!\n");
	}

	for (i = 0; i < POWER_MAX; i++) {
		power_name = kasprintf(GFP_KERNEL, "power%d", i);
		edp_panel->supply[i] = devm_regulator_get_optional(edp_panel->dev, power_name);
		if (IS_ERR(edp_panel->supply[i])) {
			ret = PTR_ERR(edp_panel->supply[i]);

			if (ret != -ENODEV) {
				if (ret != -EPROBE_DEFER)
					dev_err(edp_panel->dev,
						"failed to request regulator(%s): %d\n",
						power_name, ret);
				return ret;
			}

			edp_panel->supply[i] = NULL;
		}
	}

	if (of_property_read_u32(edp_panel->dev->of_node, "power-delay-ms", &edp_panel->power_delay_ms) < 0)
		edp_panel->power_delay_ms = 10;

	if (of_property_read_u32(edp_panel->dev->of_node, "disable-delay-ms", &edp_panel->disable_delay_ms) < 0)
		edp_panel->disable_delay_ms = 10;

	/* Get GPIOs and backlight controller. */
	for (i = 0; i < GPIO_MAX; i++) {
		gpio_name = kasprintf(GFP_KERNEL, "enable%d", i);
		edp_panel->enable_gpio[i] =
			devm_gpiod_get_optional(edp_panel->dev, gpio_name, GPIOD_OUT_HIGH);
		if (IS_ERR(edp_panel->enable_gpio[i])) {
			ret = PTR_ERR(edp_panel->enable_gpio[i]);
			dev_err(edp_panel->dev, "failed to request %s GPIO: %d\n", gpio_name,
				ret);
			return ret;
		}
	}

	edp_panel->reset_gpio =
		devm_gpiod_get_optional(edp_panel->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(edp_panel->reset_gpio)) {
		ret = PTR_ERR(edp_panel->reset_gpio);
		dev_err(edp_panel->dev, "failed to request %s GPIO: %d\n", "reset",
			ret);
		return ret;
	}

	/*
	 * TODO: Handle all power supplies specified in the DT node in a generic
	 * way for panels that don't care about power supply ordering. LVDS
	 * panels that require a specific power sequence will need a dedicated
	 * driver.
	 */

	/* Register the panel. */
	drm_panel_init(&edp_panel->panel, edp_panel->dev, &general_panel_funcs,
		       DRM_MODE_CONNECTOR_eDP);

	ret = drm_panel_of_backlight(&edp_panel->panel);
	if (ret)
		DRM_INFO("backlight for general panel missing, maybe not need!\n");


	drm_panel_add(&edp_panel->panel);

	dev_set_drvdata(edp_panel->dev, edp_panel);
	return 0;
}

static int general_panel_remove(struct platform_device *pdev)
{
	struct general_panel *edp_panel = platform_get_drvdata(pdev);

	drm_panel_remove(&edp_panel->panel);

	drm_panel_disable(&edp_panel->panel);

	return 0;
}

static const struct of_device_id general_panel_of_table[] = {
	{
		.compatible = "edp-general-panel",
	},
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, general_panel_of_table);

static struct platform_driver general_panel_driver = {
	.probe		= general_panel_probe,
	.remove		= general_panel_remove,
	.driver		= {
		.name	= "edp-general-panel",
		.of_match_table = general_panel_of_table,
	},
};

module_platform_driver(general_panel_driver);

MODULE_AUTHOR("huangyongxing <huangyongxing@allwinnertech.com>");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("eDP General Panel Driver");
MODULE_LICENSE("GPL");
