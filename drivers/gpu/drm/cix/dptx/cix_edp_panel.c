/*
 * Copyright (C) 2013, NVIDIA Corporation.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>

#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_panel.h>
#include "cix_edp_panel.h"

/**
 * @modes: Pointer to array of fixed modes appropriate for this panel.  If
 *         only one mode then this can just be the address of this the mode.
 *         NOTE: cannot be used with "timings" and also if this is specified
 *         then you cannot override the mode in the device tree.
 * @num_modes: Number of elements in modes array.
 * @timings: Pointer to array of display timings.  NOTE: cannot be used with
 *           "modes" and also these will be used to validate a device tree
 *           override if one is present.
 * @num_timings: Number of elements in timings array.
 * @bpc: Bits per color.
 * @size: Structure containing the physical size of this panel.
 * @delay: Structure containing various delay values for this panel.
 * @bus_format: See MEDIA_BUS_FMT_... defines.
 * @bus_flags: See DRM_BUS_FLAG_... defines.
 */
struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;
	const struct display_timing *timings;
	unsigned int num_timings;

	unsigned int bpc;

	/**
	 * @width: width (in millimeters) of the panel's active display area
	 * @height: height (in millimeters) of the panel's active display area
	 */
	struct {
		unsigned int width;
		unsigned int height;
	} size;

	/**
	 * @prepare: the time (in milliseconds) that it takes for the panel to
	 *           become ready and start receiving video data
	 * @hpd_absent_delay: Add this to the prepare delay if we know Hot
	 *                    Plug Detect isn't used.
	 * @enable: the time (in milliseconds) that it takes for the panel to
	 *          display the first valid frame after starting to receive
	 *          video data
	 * @disable: the time (in milliseconds) that it takes for the panel to
	 *           turn the display off (no content is visible)
	 * @unprepare: the time (in milliseconds) that it takes for the panel
	 *             to power itself down completely
	 * @reset: the time (in milliseconds) that it takes for the panel
	 *         to reset itself completely
	 * @init: the time (in milliseconds) that it takes for the panel to
	 *	  send init command sequence after reset deassert
	 */
	struct {
		unsigned int prepare;
		unsigned int hpd_absent_delay;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
		unsigned int reset;
		unsigned int init;
	} delay;

	u32 bus_format;
	u32 bus_flags;
	int connector_type;
};

struct cix_edp_panel {
	struct drm_panel base;
	bool prepared;
	bool enabled;
	bool power_invert;
	bool no_hpd;

	const struct panel_desc *desc;

	struct regulator *supply;

	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *hpd_gpio;

	struct drm_display_mode override_mode;

	enum drm_panel_orientation orientation;
};

static inline struct cix_edp_panel *to_cix_edp_panel(struct drm_panel *panel)
{
	return container_of(panel, struct cix_edp_panel, base);
}


static unsigned int cix_edp_panel_get_timings_modes(struct cix_edp_panel *panel,
						   struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	unsigned int i, num = 0;

	for (i = 0; i < panel->desc->num_timings; i++) {
		const struct display_timing *dt = &panel->desc->timings[i];
		struct videomode vm;

		videomode_from_timing(dt, &vm);
		mode = drm_mode_create(connector->dev);
		if (!mode) {
			dev_err(panel->base.dev, "failed to add mode %ux%u\n",
				dt->hactive.typ, dt->vactive.typ);
			continue;
		}

		drm_display_mode_from_videomode(&vm, mode);

		mode->type |= DRM_MODE_TYPE_DRIVER;

		if (panel->desc->num_timings == 1)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_probed_add(connector, mode);
		num++;
	}

	return num;
}

static unsigned int cix_edp_panel_get_display_modes(struct cix_edp_panel *panel,
						   struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	unsigned int i, num = 0;

	for (i = 0; i < panel->desc->num_modes; i++) {
		const struct drm_display_mode *m = &panel->desc->modes[i];

		mode = drm_mode_duplicate(connector->dev, m);
		if (!mode) {
			dev_err(panel->base.dev, "failed to add mode %ux%u@%u\n",
				m->hdisplay, m->vdisplay,
				drm_mode_vrefresh(m));
			continue;
		}

		mode->type |= DRM_MODE_TYPE_DRIVER;

		if (panel->desc->num_modes == 1)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_set_name(mode);

		drm_mode_probed_add(connector, mode);
		num++;
	}

	return num;
}

static int cix_edp_panel_get_non_edid_modes(struct cix_edp_panel *panel,
					   struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	bool has_override = panel->override_mode.type;
	unsigned int num = 0;

	if (!panel->desc)
		return 0;

	if (has_override) {
		mode = drm_mode_duplicate(connector->dev,
					  &panel->override_mode);
		if (mode) {
			drm_mode_probed_add(connector, mode);
			num = 1;
		} else {
			dev_err(panel->base.dev, "failed to add override mode\n");
		}
	}

	/* Only add timings if override was not there or failed to validate */
	if (num == 0 && panel->desc->num_timings)
		num = cix_edp_panel_get_timings_modes(panel, connector);

	/*
	 * Only add fixed modes if timings/override added no mode.
	 *
	 * We should only ever have either the display timings specified
	 * or a fixed mode. Anything else is rather bogus.
	 */
	WARN_ON(panel->desc->num_timings && panel->desc->num_modes);
	if (num == 0)
		num = cix_edp_panel_get_display_modes(panel, connector);

	if (panel->desc->bpc)
		connector->display_info.bpc = panel->desc->bpc;
	if (panel->desc->size.width)
		connector->display_info.width_mm = panel->desc->size.width;
	if (panel->desc->size.height)
		connector->display_info.height_mm = panel->desc->size.height;
	if (panel->desc->bus_format)
		drm_display_info_set_bus_formats(&connector->display_info,
						 &panel->desc->bus_format, 1);
	if (panel->desc->bus_flags)
		connector->display_info.bus_flags = panel->desc->bus_flags;

	return num;
}

static int cix_edp_panel_regulator_enable(struct cix_edp_panel *p)
{
	int err;

	if (p->power_invert) {
		if (regulator_is_enabled(p->supply) > 0)
			regulator_disable(p->supply);
	} else {
		if (!p->supply)
			return -ENODEV;

		err = regulator_enable(p->supply);
		if (err < 0)
			return err;
	}

	return 0;
}

static int cix_edp_panel_regulator_disable(struct cix_edp_panel *p)
{
	int err;

	if (p->power_invert) {
		if (!regulator_is_enabled(p->supply)) {
			err = regulator_enable(p->supply);
			if (err < 0)
				return err;
		}
	} else {
		regulator_disable(p->supply);
	}

	return 0;
}

static int cix_edp_panel_disable(struct drm_panel *panel)
{
	struct cix_edp_panel *p = to_cix_edp_panel(panel);

	if (!p->enabled) {
		dev_info(panel->dev, "%s, panel has been disabled\n", __func__);
		return 0;
	}

	dev_info(panel->dev, "%s, begin\n", __func__);

	if (p->desc->delay.disable)
		msleep(p->desc->delay.disable);

	p->enabled = false;

	dev_info(panel->dev, "%s, end\n", __func__);

	return 0;
}

static int cix_edp_panel_unprepare(struct drm_panel *panel)
{
	struct cix_edp_panel *p = to_cix_edp_panel(panel);

	if (!p->prepared) {
		dev_info(panel->dev, "%s, panel has been unprepared\n", __func__);
		return 0;
	}

	dev_info(panel->dev, "%s, begin\n", __func__);

	gpiod_set_value_cansleep(p->reset_gpio, 1);
	gpiod_set_value_cansleep(p->enable_gpio, 0);

	cix_edp_panel_regulator_disable(p);

	if (p->desc->delay.unprepare)
		msleep(p->desc->delay.unprepare);

	p->prepared = false;

	dev_info(panel->dev, "%s, end\n", __func__);

	return 0;
}

static int cix_edp_panel_get_hpd_gpio(struct device *dev,
				     struct cix_edp_panel *p, bool from_probe)
{
	int err;

	p->hpd_gpio = devm_gpiod_get_optional(dev, "hpd", GPIOD_IN);
	if (IS_ERR(p->hpd_gpio)) {
		err = PTR_ERR(p->hpd_gpio);

		/*
		 * If we're called from probe we won't consider '-EPROBE_DEFER'
		 * to be an error--we'll leave the error code in "hpd_gpio".
		 * When we try to use it we'll try again.  This allows for
		 * circular dependencies where the component providing the
		 * hpd gpio needs the panel to init before probing.
		 */
		if (err != -EPROBE_DEFER || !from_probe) {
			dev_err(dev, "failed to get 'hpd' GPIO: %d\n", err);
			return err;
		}
	}

	return 0;
}

static int cix_edp_panel_prepare(struct drm_panel *panel)
{
	struct cix_edp_panel *p = to_cix_edp_panel(panel);
	unsigned int delay;
	int err;
	int hpd_asserted;

	if (p->prepared) {
		dev_info(panel->dev, "%s, panel has been prepared\n", __func__);
		return 0;
	}

	dev_info(panel->dev, "%s, begin\n", __func__);

	err = cix_edp_panel_regulator_enable(p);
	if (err < 0) {
		dev_err(panel->dev, "failed to enable supply: %d\n", err);
		return err;
	}

	gpiod_set_value_cansleep(p->enable_gpio, 1);

	delay = p->desc->delay.prepare;
	if (p->no_hpd)
		delay += p->desc->delay.hpd_absent_delay;
	if (delay)
		msleep(delay);

	if (p->hpd_gpio) {
		if (IS_ERR(p->hpd_gpio)) {
			err = cix_edp_panel_get_hpd_gpio(panel->dev, p, false);
			if (err)
				return err;
		}

		err = readx_poll_timeout(gpiod_get_value_cansleep, p->hpd_gpio,
					 hpd_asserted, hpd_asserted,
					 1000, 2000000);
		if (hpd_asserted < 0)
			err = hpd_asserted;

		if (err) {
			dev_err(panel->dev,
				"error waiting for hpd GPIO: %d\n", err);
			return err;
		}
	}

	gpiod_set_value_cansleep(p->reset_gpio, 1);

	if (p->desc->delay.reset)
		msleep(p->desc->delay.reset);

	gpiod_set_value_cansleep(p->reset_gpio, 0);


	if (p->desc->delay.init)
		msleep(p->desc->delay.init);

	p->prepared = true;

	dev_info(panel->dev, "%s, end\n", __func__);

	return 0;
}

static int cix_edp_panel_enable(struct drm_panel *panel)
{
	struct cix_edp_panel *p = to_cix_edp_panel(panel);

	if (p->enabled) {
		dev_info(panel->dev, "%s, panel has been enabled\n", __func__);
		return 0;
	}

	dev_info(panel->dev, "%s, begin\n", __func__);

	if (p->desc->delay.enable)
		msleep(p->desc->delay.enable);

	p->enabled = true;

	dev_info(panel->dev, "%s, end\n", __func__);

	return 0;
}

static int cix_edp_panel_get_modes(struct drm_panel *panel,
				  struct drm_connector *connector)
{
	struct cix_edp_panel *p = to_cix_edp_panel(panel);
	int num = 0;

	/* add hard-coded panel modes */
	num += cix_edp_panel_get_non_edid_modes(p, connector);

	/* set up connector's "panel orientation" property */
	drm_connector_set_panel_orientation(connector, p->orientation);

	return num;
}

static int cix_edp_panel_get_timings(struct drm_panel *panel,
				    unsigned int num_timings,
				    struct display_timing *timings)
{
	struct cix_edp_panel *p = to_cix_edp_panel(panel);
	unsigned int i;

	if (p->desc->num_timings < num_timings)
		num_timings = p->desc->num_timings;

	if (timings)
		for (i = 0; i < num_timings; i++)
			timings[i] = p->desc->timings[i];

	return p->desc->num_timings;
}

static const struct drm_panel_funcs cix_edp_panel_funcs = {
	.disable = cix_edp_panel_disable,
	.unprepare = cix_edp_panel_unprepare,
	.prepare = cix_edp_panel_prepare,
	.enable = cix_edp_panel_enable,
	.get_modes = cix_edp_panel_get_modes,
	.get_timings = cix_edp_panel_get_timings,
};

#define CIX_EDP_PANEL_BOUNDS_CHECK(to_check, bounds, field) \
	(to_check->field.typ >= bounds->field.min && \
	 to_check->field.typ <= bounds->field.max)
static void cix_edp_panel_parse_panel_timing_node(struct device *dev,
						 struct cix_edp_panel *panel,
						 const struct display_timing *ot)
{
	const struct panel_desc *desc = panel->desc;
	struct videomode vm;
	unsigned int i;

	if (WARN_ON(desc->num_modes)) {
		dev_err(dev, "Reject override mode: panel has a fixed mode\n");
		return;
	}
	if (WARN_ON(!desc->num_timings)) {
		dev_err(dev, "Reject override mode: no timings specified\n");
		return;
	}

	for (i = 0; i < panel->desc->num_timings; i++) {
		const struct display_timing *dt = &panel->desc->timings[i];

		if (!CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, hactive) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, hfront_porch) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, hback_porch) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, hsync_len) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, vactive) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, vfront_porch) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, vback_porch) ||
		    !CIX_EDP_PANEL_BOUNDS_CHECK(ot, dt, vsync_len))
			continue;

		if (ot->flags != dt->flags)
			continue;

		videomode_from_timing(ot, &vm);
		drm_display_mode_from_videomode(&vm, &panel->override_mode);
		panel->override_mode.type |= DRM_MODE_TYPE_DRIVER |
					     DRM_MODE_TYPE_PREFERRED;
		break;
	}

	if (WARN_ON(!panel->override_mode.type))
		dev_err(dev, "Reject override mode: No display_timing found\n");
}


static int cix_edp_panel_probe(struct device *dev, const struct panel_desc *desc)
{
	struct cix_edp_panel *panel;
	struct display_timing dt;
	int err;

	dev_info(dev, "%s, begin\n", __func__);

	panel = devm_kzalloc(dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	panel->enabled = false;
	panel->prepared = false;
	panel->desc = desc;

	panel->no_hpd = of_property_read_bool(dev->of_node, "no-hpd");
	if (!panel->no_hpd) {
		err = cix_edp_panel_get_hpd_gpio(dev, panel, true);
		if (err)
			return err;
	}

	panel->supply = devm_regulator_get(dev, "power");
	if (IS_ERR(panel->supply)) {
		err = PTR_ERR(panel->supply);
		dev_err(dev, "failed to get power regulator: %d\n", err);
		return err;
	}

	panel->enable_gpio = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(panel->enable_gpio)) {
		err = PTR_ERR(panel->enable_gpio);
		if (err != -EPROBE_DEFER)
			dev_err(dev, "failed to get enable GPIO: %d\n", err);
		return err;
	}

	panel->reset_gpio = devm_gpiod_get_optional(dev, "reset", GPIOD_ASIS);
	if (IS_ERR(panel->reset_gpio)) {
		err = PTR_ERR(panel->reset_gpio);
		if (err != -EPROBE_DEFER)
			dev_err(dev, "failed to get reset GPIO: %d\n", err);
		return err;
	}

	err = of_drm_get_panel_orientation(dev->of_node, &panel->orientation);
	if (err) {
		dev_err(dev, "%pOF: failed to get orientation %d\n", dev->of_node, err);
		return err;
	}

	panel->power_invert = of_property_read_bool(dev->of_node, "power-invert");

	if (!of_get_display_timing(dev->of_node, "panel-timing", &dt))
		cix_edp_panel_parse_panel_timing_node(dev, panel, &dt);


	if (desc->bus_format == 0)
		dev_warn(dev, "Specify missing bus_format\n");

	if (desc->bpc != 6 && desc->bpc != 8)
		dev_warn(dev, "Expected bpc in {6,8} but got: %u\n", desc->bpc);

	drm_panel_init(&panel->base, dev, &cix_edp_panel_funcs, DRM_MODE_CONNECTOR_eDP);

	err = drm_panel_of_backlight(&panel->base);
	if (err) {
		dev_err(dev, "failed to find backlight: %d\n", err);
		goto exit;
	}

	drm_panel_add(&panel->base);

	dev_set_drvdata(dev, panel);

	dev_info(dev, "%s, end\n", __func__);

	return 0;

exit:
	return err;
}

static int cix_edp_panel_remove(struct device *dev)
{
	struct cix_edp_panel *panel = dev_get_drvdata(dev);

	drm_panel_remove(&panel->base);
	drm_panel_disable(&panel->base);
	drm_panel_unprepare(&panel->base);

	return 0;
}

static void cix_edp_panel_shutdown(struct device *dev)
{
	struct cix_edp_panel *panel = dev_get_drvdata(dev);

	drm_panel_disable(&panel->base);
	drm_panel_unprepare(&panel->base);
}

static const struct of_device_id platform_of_match[] = {
	{
		.compatible = "cix-edp-panel",
		.data = NULL,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, platform_of_match);

static bool of_child_node_is_present(const struct device_node *node,
				     const char *name)
{
	struct device_node *child;

	child = of_get_child_by_name(node, name);
	of_node_put(child);

	return !!child;
}

static int cix_edp_panel_of_get_desc_data(struct device *dev,
					 struct panel_desc *desc)
{
	struct device_node *np = dev->of_node;
	u32 bus_flags;

	if (of_child_node_is_present(np, "display-timings")) {
		struct drm_display_mode *mode;

		mode = devm_kzalloc(dev, sizeof(*mode), GFP_KERNEL);
		if (!mode)
			return -ENOMEM;

		if (!of_get_drm_display_mode(np, mode, &bus_flags,
					     OF_USE_NATIVE_MODE)) {
			desc->modes = mode;
			desc->num_modes = 1;
			desc->bus_flags = bus_flags;
		}
	} else if (of_child_node_is_present(np, "panel-timing")) {
		struct display_timing *timing;
		struct videomode vm;

		timing = devm_kzalloc(dev, sizeof(*timing), GFP_KERNEL);
		if (!timing)
			return -ENOMEM;

		if (!of_get_display_timing(np, "panel-timing", timing)) {
			desc->timings = timing;
			desc->num_timings = 1;

			bus_flags = 0;
			vm.flags = timing->flags;
			drm_bus_flags_from_videomode(&vm, &bus_flags);
			desc->bus_flags = bus_flags;
		}
	}

	if (desc->num_modes || desc->num_timings) {
		of_property_read_u32(np, "bpc", &desc->bpc);
		of_property_read_u32(np, "bus-format", &desc->bus_format);
		of_property_read_u32(np, "width-mm", &desc->size.width);
		of_property_read_u32(np, "height-mm", &desc->size.height);
	}

	of_property_read_u32(np, "prepare-delay-ms", &desc->delay.prepare);
	of_property_read_u32(np, "enable-delay-ms", &desc->delay.enable);
	of_property_read_u32(np, "disable-delay-ms", &desc->delay.disable);
	of_property_read_u32(np, "unprepare-delay-ms", &desc->delay.unprepare);
	of_property_read_u32(np, "reset-delay-ms", &desc->delay.reset);
	of_property_read_u32(np, "init-delay-ms", &desc->delay.init);

	return 0;
}

static int cix_edp_panel_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *id;
	const struct panel_desc *desc;
	struct panel_desc *d;
	int err;

	id = of_match_node(platform_of_match, pdev->dev.of_node);
	if (!id)
		return -ENODEV;

	if (!id->data) {
		d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);
		if (!d)
			return -ENOMEM;

		err = cix_edp_panel_of_get_desc_data(dev, d);
		if (err) {
			dev_err(dev, "failed to get desc data: %d\n", err);
			return err;
		}
	}

	desc = id->data ? id->data : d;

	return cix_edp_panel_probe(&pdev->dev, desc);
}

static int cix_edp_panel_platform_remove(struct platform_device *pdev)
{
	return cix_edp_panel_remove(&pdev->dev);
}

static void cix_edp_panel_platform_shutdown(struct platform_device *pdev)
{
	cix_edp_panel_shutdown(&pdev->dev);
}

static struct platform_driver cix_edp_panel_platform_driver = {
	.driver = {
		.name = "cix-edp-panel",
		.of_match_table = platform_of_match,
	},
	.probe = cix_edp_panel_platform_probe,
	.remove = cix_edp_panel_platform_remove,
	.shutdown = cix_edp_panel_platform_shutdown,
};

int cix_edp_panel_init(void)
{
	int err;

	err = platform_driver_register(&cix_edp_panel_platform_driver);
	if (err < 0)
		return err;

	return 0;
}

void cix_edp_panel_exit(void)
{
	platform_driver_unregister(&cix_edp_panel_platform_driver);
}
