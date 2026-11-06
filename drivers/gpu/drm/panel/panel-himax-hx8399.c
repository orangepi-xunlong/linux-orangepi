// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for panels based on Himax hx8399 controller, such as:
 *
 * - HannStar hx8399 5.99" MIPI-DSI panel
 *
 * Copyright (C) 2021 Kamil Trzciński
 *
 * Based on drivers/gpu/drm/panel/panel-sitronix-st7703.c
 * Copyright (C) Purism SPC 2019
 */

#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/media-bus-format.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

struct dcs_cmd {
	u8	delay;
	u8	size;
	u8	*cmd;
};

static u8 data_hx8399_0[] = {
	0xB9, 0xFF, 0x83, 0x99,
};
static u8 data_hx8399_1[] = {
	0xD2, 0x77,
};
static u8 data_hx8399_2[] = {
	0xB1, 0x02, 0x04, 0x74,
	0x94, 0x01, 0x32, 0x33,
	0x11, 0x11, 0xAB, 0x4D,
	0x56, 0x73, 0x02, 0x02,
};
static u8 data_hx8399_3[] = {
	0xB2, 0x00, 0x80, 0x80,
	0xAE, 0x05, 0x07, 0x5A,
	0x11, 0x00, 0x00, 0x10,
	0x1E, 0x70, 0x03, 0xD4,
};
static u8 data_hx8399_4[] = {
	0xB4, 0x00, 0xFF, 0x02,
	0xC0, 0x02, 0xC0, 0x00,
	0x00, 0x08, 0x00, 0x04,
	0x06, 0x00, 0x32, 0x04,
	0x0A, 0x08, 0x21, 0x03,
	0x01, 0x00, 0x0F, 0xB8,
	0x8B, 0x02, 0xC0, 0x02,
	0xC0, 0x00, 0x00, 0x08,
	0x00, 0x04, 0x06, 0x00,
	0x32, 0x04, 0x0A, 0x08,
	0x01, 0x00, 0x0F, 0xB8,
	0x01,
};
static u8 data_hx8399_5[] = {
	0xD3, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x06,
	0x00, 0x00, 0x10, 0x04,
	0x00, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x05, 0x05,
	0x07, 0x00, 0x00, 0x00,
	0x05, 0x40,
};
static u8 data_hx8399_6[] = {
	0xD5, 0x18, 0x18, 0x19,
	0x19, 0x18, 0x18, 0x21,
	0x20, 0x01, 0x00, 0x07,
	0x06, 0x05, 0x04, 0x03,
	0x02, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x2F,
	0x2F, 0x30, 0x30, 0x31,
	0x31, 0x18, 0x18, 0x18,
	0x18,
};
static u8 data_hx8399_7[] = {
	0xD6, 0x18, 0x18, 0x19,
	0x19, 0x40, 0x40, 0x20,
	0x21, 0x06, 0x07, 0x00,
	0x01, 0x02, 0x03, 0x04,
	0x05, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x2F,
	0x2F, 0x30, 0x30, 0x31,
	0x31, 0x40, 0x40, 0x40,
	0x40,
};
static u8 data_hx8399_8[] = {
	0xD8, 0xA2, 0xAA, 0x02,
	0xA0, 0xA2, 0xA8, 0x02,
	0xA0, 0xB0, 0x00, 0x00,
	0x00, 0xB0, 0x00, 0x00,
	0x00,
};
static u8 data_hx8399_9[] = {
	0xBD, 0x01,
};
static u8 data_hx8399_10[] = {
	0xD8, 0xB0, 0x00, 0x00,
	0x00, 0xB0, 0x00, 0x00,
	0x00, 0xE2, 0xAA, 0x03,
	0xF0, 0xE2, 0xAA, 0x03,
	0xF0,
};
static u8 data_hx8399_11[] = {
	0xBD, 0x02,
};
static u8 data_hx8399_12[] = {
	0xD8, 0xE2, 0xAA, 0x03,
	0xF0, 0xE2, 0xAA, 0x03,
	0xF0,
};
static u8 data_hx8399_13[] = {
	0xBD, 0x00,
};
static u8 data_hx8399_14[] = {
	0xB6, 0x8D, 0x8D
};
static u8 data_hx8399_15[] = {
	0xE0, 0x00, 0x0E, 0x19,
	0x13, 0x2E, 0x39, 0x48,
	0x44, 0x4D, 0x57, 0x5F,
	0x66, 0x6C, 0x76, 0x7F,
	0x85, 0x8A, 0x95, 0x9A,
	0xA4, 0x9B, 0xAB, 0xB0,
	0x5C, 0x58, 0x64, 0x77,
	0x00, 0x0E, 0x19, 0x13,
	0x2E, 0x39, 0x48, 0x44,
	0x4D, 0x57, 0x5F, 0x66,
	0x6C, 0x76, 0x7F, 0x85,
	0x8A, 0x95, 0x9A, 0xA4,
	0x9B, 0xAB, 0xB0, 0x5C,
	0x58, 0x64, 0x77,
};
static u8 data_hx8399_16[] = {
	0xcc, 0x08,
};
// static u8 data_hx8399_17[] = {
//	0x21
// };
// static u8 data_hx8399_18[] = {
//	0x11
// };
// static u8 data_hx8399_19[] = {
//	0x29
// };

const struct dcs_cmd init_cmds_hx8399_1080x1920[] = {
	{.delay = 0, .size = 4, .cmd = data_hx8399_0 },
	{.delay = 0, .size = 2, .cmd = data_hx8399_1 },
	{.delay = 0, .size = 16, .cmd = data_hx8399_2 },
	{.delay = 0, .size = 16, .cmd = data_hx8399_3 },
	{.delay = 0, .size = 45, .cmd = data_hx8399_4 },
	{.delay = 10, .size = 34, .cmd = data_hx8399_5 },
	{.delay = 10, .size = 33, .cmd = data_hx8399_6 },
	{.delay = 10, .size = 33, .cmd = data_hx8399_7 },
	{.delay = 0, .size = 17, .cmd = data_hx8399_8 },
	{.delay = 0, .size = 2, .cmd = data_hx8399_9 },
	{.delay = 0, .size = 17, .cmd = data_hx8399_10 },
	{.delay = 0, .size = 2, .cmd = data_hx8399_11 },
	{.delay = 0, .size = 9, .cmd = data_hx8399_12 },
	{.delay = 0, .size = 2, .cmd = data_hx8399_13 },
	{.delay = 0, .size = 3, .cmd = data_hx8399_14 },
	{.delay = 10, .size = 55, .cmd = data_hx8399_15 },
	{.delay = 0, .size = 2, .cmd = data_hx8399_16 },
	// {.delay = 0, .size = 1, .cmd = data_hx8399_17 },
	// {.delay = 120, .size = 1, .cmd = data_hx8399_18 },
	// {.delay = 20, .size = 1, .cmd = data_hx8399_19 }
};

#define DRV_NAME "panel-himax-hx8399"

struct hx8399 {
	struct device *dev;
	struct drm_panel panel;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *pwm_gpio;
	struct gpio_desc *power_gpio;
	bool enabled;
	bool prepared;

	const struct hx8399_panel_desc *desc;
};

struct hx8399_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	int (*init_sequence)(struct hx8399 *ctx);
};

static inline struct hx8399 *panel_to_hx8399(struct drm_panel *panel)
{
	return container_of(panel, struct hx8399, panel);
}

static int hx8399_init_sequence(struct hx8399 *ctx)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(init_cmds_hx8399_1080x1920); i++) {
		ret = mipi_dsi_dcs_write_buffer(dsi, init_cmds_hx8399_1080x1920[i].cmd, init_cmds_hx8399_1080x1920[i].size);
		if (ret < 0)
			return ret;
		msleep(init_cmds_hx8399_1080x1920[i].delay);
	}

	return 0;
}

static const struct drm_display_mode hx8399_mode = {
	.hdisplay    = 1080,
	.hsync_start = 1080 + 119,
	.hsync_end   = 1080 + 119 + 5,
	.htotal	     = 1080 + 119 + 5 + 148,
	.vdisplay    = 1920,
	.vsync_start = 1920 + 2,
	.vsync_end   = 1920 + 2 + 6,
	.vtotal	     = 1920 + 2 + 6 + 2,
	.clock	     = 156561,
	.flags	     = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC,
	.width_mm    = 68,
	.height_mm   = 120,
};

static const struct hx8399_panel_desc hx8399_desc = {
	.mode = &hx8399_mode,
	.lanes = 4,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST,
	.format = MIPI_DSI_FMT_RGB888,
	.init_sequence = hx8399_init_sequence,
};

static int hx8399_enable(struct drm_panel *panel)
{
	struct hx8399 *ctx = panel_to_hx8399(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;
	u8 cmd = 0xda;
	u8 data;
	u8 len = 1;

	if (ctx->enabled)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, &data, len);
	if (ret || data != 0x83)
		dev_err(ctx->dev, "Panel id read failed: %d\n", ret);

	msleep(20);

	cmd = 0xdb;
	ret = mipi_dsi_dcs_read(dsi, cmd, &data, len);
	if (ret || data != 0x99)
		dev_err(ctx->dev, "Panel id read failed: %d\n", ret);

	msleep(20);

	cmd = 0xdc;
	ret = mipi_dsi_dcs_read(dsi, cmd, &data, len);
	if (ret || data != 0xf)
		dev_err(ctx->dev, "Panel id read failed: %d\n", ret);

	/* Panel is operational 180 msec before init_sequence  */
	msleep(180);

	ret = ctx->desc->init_sequence(ctx);
	if (ret) {
		dev_err(ctx->dev, "Panel init sequence failed: %d\n", ret);
		return ret;
	}

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret) {
		dev_err(ctx->dev, "Failed to exit sleep mode: %d\n", ret);
		return ret;
	}

	/* Panel is operational 120 msec after reset */
	msleep(120);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret) {
		dev_err(ctx->dev, "Failed to turn on the display: %d\n", ret);
		goto sleep_in;
	}

	ctx->enabled = true;

	return 0;

sleep_in:
	/* This will probably fail, but let's try orderly power off anyway. */
	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (!ret)
		msleep(50);

	return ret;
}

static int hx8399_disable(struct drm_panel *panel)
{
	struct hx8399 *ctx = panel_to_hx8399(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	if (!ctx->enabled)
		return 0;

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret) {
		dev_err(ctx->dev, "Failed to enter sleep mode: %d\n", ret);
		return ret;
	}

	msleep(50); /* about 3 frames */

	ctx->enabled = false;

	return 0;
}

static int hx8399_unprepare(struct drm_panel *panel)
{
	struct hx8399 *ctx = panel_to_hx8399(panel);

	if (!ctx->prepared)
		return 0;

	if (ctx->reset_gpio)
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);

	if (ctx->power_gpio)
		gpiod_set_value_cansleep(ctx->power_gpio, 0);

	if (ctx->pwm_gpio)
		gpiod_set_value_cansleep(ctx->pwm_gpio, 0);

	ctx->prepared = false;

	return 0;
}

static int hx8399_prepare(struct drm_panel *panel)
{
	struct hx8399 *ctx = panel_to_hx8399(panel);

	if (ctx->prepared)
		return 0;

	if (ctx->power_gpio)
		gpiod_set_value_cansleep(ctx->power_gpio, 1);

	if (ctx->pwm_gpio)
		gpiod_set_value_cansleep(ctx->pwm_gpio, 1);

	if (ctx->reset_gpio) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		msleep(180);
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	}

	ctx->prepared = true;

	return 0;
}

static int hx8399_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	struct hx8399 *ctx = panel_to_hx8399(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, ctx->desc->mode);
	if (!mode) {
		dev_err(ctx->dev, "Failed to add mode %ux%u@%u\n",
			ctx->desc->mode->hdisplay, ctx->desc->mode->vdisplay,
			drm_mode_vrefresh(ctx->desc->mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs hx8399_drm_funcs = {
	.disable   = hx8399_disable,
	.unprepare = hx8399_unprepare,
	.prepare   = hx8399_prepare,
	.enable	   = hx8399_enable,
	.get_modes = hx8399_get_modes,
};

static int hx8399_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hx8399 *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				    "Failed to get reset gpio\n");

	ctx->pwm_gpio = devm_gpiod_get(dev, "pwm", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->pwm_gpio))
		dev_err_probe(dev, PTR_ERR(ctx->pwm_gpio),
				    "Failed to get pwm gpio\n");

	ctx->power_gpio = devm_gpiod_get(dev, "power", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->power_gpio))
		dev_err_probe(dev, PTR_ERR(ctx->power_gpio),
				    "Failed to get power gpio\n");

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	ctx->desc = of_device_get_match_data(dev);

	dsi->mode_flags = ctx->desc->mode_flags;
	dsi->format = ctx->desc->format;
	dsi->lanes = ctx->desc->lanes;

	drm_panel_init(&ctx->panel, dev, &hx8399_drm_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err_probe(dev, ret, "mipi_dsi_attach failed\n");
		drm_panel_remove(&ctx->panel);
		return ret;
	}

	dev_dbg(dev, "%ux%u@%u %ubpp dsi %udl - ready\n",
		ctx->desc->mode->hdisplay, ctx->desc->mode->vdisplay,
		drm_mode_vrefresh(ctx->desc->mode),
		mipi_dsi_pixel_format_to_bpp(dsi->format), dsi->lanes);

	return 0;
}

static void hx8399_shutdown(struct mipi_dsi_device *dsi)
{
	struct hx8399 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = drm_panel_disable(&ctx->panel);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to disable panel: %d\n", ret);

	ret = drm_panel_unprepare(&ctx->panel);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to unprepare panel: %d\n", ret);
}

static int hx8399_remove(struct mipi_dsi_device *dsi)
{
	struct hx8399 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	hx8399_shutdown(dsi);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);
		return ret;
	}

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id hx8399_of_match[] = {
	{ .compatible = "cvitek,hx8399", .data = &hx8399_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hx8399_of_match);

static struct mipi_dsi_driver hx8399_driver = {
	.probe	= hx8399_probe,
	.remove = hx8399_remove,
	.shutdown = hx8399_shutdown,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = hx8399_of_match,
	},
};
module_mipi_dsi_driver(hx8399_driver);

MODULE_AUTHOR("Bowen Du <bowen.du@sophgo.com>");
MODULE_DESCRIPTION("DRM driver for Himax hx8399 based MIPI DSI panels");
MODULE_LICENSE("GPL");
