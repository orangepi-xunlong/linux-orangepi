

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/media-bus-format.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_crtc.h>


struct lvds_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int en;
	unsigned int out_bit;
	unsigned int vesa_mode;
	unsigned int dual_ch;
	unsigned int vs_out_en;
	unsigned int hs_out_en;
	unsigned int hs_blk_en;
	unsigned int ml_swap;
	unsigned int ctrl_rev;
	unsigned int oe_swap;
};

static const struct drm_display_mode lcm18x56_mode = {
	.hdisplay    = 1366,
	.hsync_start = 1366 + 88,
	.hsync_end   = 1366 + 88 + 20,
	.htotal	     = 1366 + 88 + 20 + 100,
	.vdisplay    = 768,
	.vsync_start = 768 + 10,
	.vsync_end   = 768 + 10 + 2,
	.vtotal	     = 768 + 10 + 2 + 20,
	.clock	     = 75552, //ht*vt*fps
	.flags	     = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
	.width_mm    = 341,
	.height_mm   = 192,
};

static const struct lvds_panel_desc lcm18x56_desc = {
	.mode = &lcm18x56_mode,
	.en = 1,
	.out_bit = 1,
	.vesa_mode = 1,
	.dual_ch = 0,
	.vs_out_en = 1,
	.hs_out_en = 1,
	.hs_blk_en = 1,
	.ml_swap = 1,
	.ctrl_rev = 0,
	.oe_swap = 0,
};

struct panel_lvds {
    struct drm_panel panel;
	struct device *dev;
	struct gpio_desc *power_gpio;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
	bool enabled;
	bool prepared;
	bool data_mirror;
	const struct lvds_panel_desc *desc;
	unsigned int width;
	unsigned int height;
	struct videomode video_mode;
	unsigned int bus_format;
	enum drm_panel_orientation orientation;
};

static inline struct panel_lvds *to_lcm18x56_lvds(struct drm_panel *panel)
{
	return container_of(panel, struct panel_lvds, panel);
}

static int lcm18x56_disable(struct drm_panel *panel)
{
	return 0;
}

static int lcm18x56_enable(struct drm_panel *panel)
{
	return 0;
}

static int lcm18x56_unprepare(struct drm_panel *panel)
{
	struct panel_lvds *lvds = to_lcm18x56_lvds(panel);

	if (!lvds->prepared)
		return 0;

	if (lvds->power_gpio)
		gpiod_set_value_cansleep(lvds->power_gpio, 0);

	lvds->prepared = false;

	return 0;
}

static int lcm18x56_prepare(struct drm_panel *panel)
{
	struct panel_lvds *lvds = to_lcm18x56_lvds(panel);

	if (lvds->prepared)
		return 0;

	if (lvds->power_gpio)
		gpiod_set_value_cansleep(lvds->power_gpio, 1);

	lvds->prepared = true;

	return 0;
}

static int lcm18x56_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct panel_lvds *lvds = to_lcm18x56_lvds(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, lvds->desc->mode);
	if (!mode) {
		dev_err(lvds->dev, "Failed to add mode %ux%u@%u\n",
			lvds->desc->mode->hdisplay, lvds->desc->mode->vdisplay,
			drm_mode_vrefresh(lvds->desc->mode));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type |= DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_display_info_set_bus_formats(&connector->display_info,
					 &lvds->bus_format, 1);
	connector->display_info.bus_flags = lvds->data_mirror
					  ? DRM_BUS_FLAG_DATA_LSB_TO_MSB
					  : DRM_BUS_FLAG_DATA_MSB_TO_LSB;
	drm_connector_set_panel_orientation(connector, lvds->orientation);

	return 1;
}

static const struct drm_panel_funcs lcm18x56_lvds_funcs = {
	.disable = lcm18x56_disable,
	.unprepare = lcm18x56_unprepare,
	.prepare = lcm18x56_prepare,
	.enable = lcm18x56_enable,
	.get_modes = lcm18x56_get_modes,
};

static int lcm18x56_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct panel_lvds *lvds;
	int ret = 0;

	lvds = devm_kzalloc(&pdev->dev, sizeof(*lvds), GFP_KERNEL);
	if (!lvds)
		return -ENOMEM;

	lvds->dev = dev;
	lvds->desc = of_device_get_match_data(dev);

	lvds->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;
	lvds->data_mirror = 0;
	lvds->orientation = DRM_MODE_PANEL_ORIENTATION_UNKNOWN;
	
	lvds->power_gpio = devm_gpiod_get(dev, "power", GPIOD_OUT_LOW);
	if (IS_ERR(lvds->power_gpio))
		dev_err_probe(dev, PTR_ERR(lvds->power_gpio),
				    "Failed to get power gpio\n");

	drm_panel_init(&lvds->panel, dev, &lcm18x56_lvds_funcs,
		       DRM_MODE_CONNECTOR_LVDS);

	ret = drm_panel_of_backlight(&lvds->panel);
	if (ret)
		return ret;

	drm_panel_add(&lvds->panel);

	dev_set_drvdata(lvds->dev, lvds);

	return 0;
}

static int lcm18x56_remove(struct platform_device *pdev)
{
	struct panel_lvds *lvds = dev_get_drvdata(&pdev->dev);

	drm_panel_remove(&lvds->panel);

	drm_panel_disable(&lvds->panel);

	return 0;
}

static const struct of_device_id lcm18x56_of_match[] = {
	{ .compatible = "cvitek,lcm18x56", .data = &lcm18x56_desc },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, lcm18x56_of_match);

static struct platform_driver lcm18x56_driver = {
	.probe	= lcm18x56_probe,
	.remove = lcm18x56_remove,
	.driver = {
		.name = "panel-lcm18x56",
		.of_match_table = lcm18x56_of_match,
	},
};
module_platform_driver(lcm18x56_driver);

MODULE_AUTHOR("Linfeng Tang <linfeng.tang@sophgo.com>");
MODULE_DESCRIPTION("DRM driver for LCM18X56 based LVDS panels");
MODULE_LICENSE("GPL");
