// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for panels based on Himax HX8394 controller, such as:
 *
 * - HannStar hx8394 5.99" MIPI-DSI panel
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

static u8 cmd_hx8394_0[] = { 0xb9, 0xff, 0x83, 0x94 };
static u8 cmd_hx8394_1[] = {
	0xb1, 0x50, 0x15, 0x75, 0x09, 0x32, 0x44, 0x71, 0x31, 0x4d,
	0x2f, 0x56, 0x73, 0x02, 0x02
};
static u8 cmd_hx8394_2[] = { 0xba, 0x63, 0x03, 0x68, 0x6b, 0xb2, 0xc0 };
static u8 cmd_hx8394_3[] = { 0xd2, 0x88 };
static u8 cmd_hx8394_4[] = { 0xb2, 0x00, 0x80, 0x64, 0x10, 0x07 };
static u8 cmd_hx8394_5[] = {
	0xb4, 0x01, 0x75, 0x01, 0x75, 0x01, 0x75, 0x01, 0x0c, 0x86,
	0x75, 0x00, 0x3f, 0x01, 0x75, 0x01, 0x75, 0x01, 0x75, 0x01,
	0x0c, 0x86
};
static u8 cmd_hx8394_6[] = {
	0xd3, 0x00, 0x00, 0x06, 0x06, 0x40, 0x1a, 0x08, 0x00, 0x32,
	0x10, 0x08, 0x00, 0x08, 0x54, 0x15, 0x10, 0x05, 0x04, 0x02,
	0x12, 0x10, 0x05, 0x07, 0x23, 0x23, 0x0c, 0x0c, 0x27, 0x10,
	0x07, 0x07, 0x10, 0x40
};
static u8 cmd_hx8394_7[] = {
	0xd5, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0x02, 0x03, 0x20,
	0x21, 0x22, 0x23, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x19, 0x19, 0x18, 0x18, 0x18, 0x18, 0x1b, 0x1b, 0x1a,
	0x1a, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18
};
static u8 cmd_hx8394_8[] = {
	0xd6, 0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04, 0x23,
	0x22, 0x21, 0x20, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x58,
	0x58, 0x18, 0x18, 0x19, 0x19, 0x18, 0x18, 0x1b, 0x1b, 0x1a,
	0x1a, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18
};
static u8 cmd_hx8394_9[] = {
	0xe0, 0x00, 0x1a, 0x24, 0x2c, 0x2e, 0x32, 0x34, 0x32, 0x66,
	0x73, 0x82, 0x7f, 0x85, 0x95, 0x97, 0x99, 0xa4, 0xa5, 0xa0,
	0xab, 0xba, 0x5a, 0x59, 0x5d, 0x61, 0x63, 0x6c, 0x72, 0x7f,
	0x00, 0x19, 0x24, 0x2c, 0x2e, 0x32, 0x34, 0x32, 0x66, 0x73,
	0x82, 0x7f, 0x85, 0x95, 0x97, 0x99, 0xa4, 0xa5, 0xa0, 0xab,
	0xba, 0x5a, 0x59, 0x5d, 0x61, 0x63, 0x6c, 0x72, 0x7f
};
static u8 cmd_hx8394_10[] = { 0xcc, 0x03 };
static u8 cmd_hx8394_11[] = { 0xc0, 0x1f, 0x73 };
static u8 cmd_hx8394_12[] = { 0xb6, 0x42, 0x42 };
static u8 cmd_hx8394_13[] = { 0xd4, 0x02 };
static u8 cmd_hx8394_14[] = { 0xbd, 0x01 };
static u8 cmd_hx8394_15[] = { 0xb1, 0x00 };
static u8 cmd_hx8394_16[] = { 0xbd, 0x00 };
static u8 cmd_hx8394_17[] = { 0xbf, 0x40, 0x81, 0x50, 0x00, 0x1a, 0xfc, 0x01 };
static u8 cmd_hx8394_18[] = { 0xc6, 0xef };
static u8 cmd_hx8394_19[] = { 0x36, 0x02 };// h-flip
// static u8 cmd_hx8394_20[] = { 0x11 };
// static u8 cmd_hx8394_21[] = { 0x29 };

const struct dcs_cmd init_cmds_hx8394_720x1280[] = {
	{.delay = 0,   .size = 4,   .cmd = cmd_hx8394_0 },
	{.delay = 0,   .size = 15,  .cmd = cmd_hx8394_1 },
	{.delay = 0,   .size = 7,   .cmd = cmd_hx8394_2 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_3 },
	{.delay = 0,   .size = 6,   .cmd = cmd_hx8394_4 },
	{.delay = 0,   .size = 22,  .cmd = cmd_hx8394_5 },
	{.delay = 0,   .size = 34,  .cmd = cmd_hx8394_6 },
	{.delay = 0,   .size = 45,  .cmd = cmd_hx8394_7 },
	{.delay = 0,   .size = 45,  .cmd = cmd_hx8394_8 },
	{.delay = 0,   .size = 59,  .cmd = cmd_hx8394_9 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_10 },
	{.delay = 0,   .size = 3,   .cmd = cmd_hx8394_11 },
	{.delay = 0,   .size = 3,   .cmd = cmd_hx8394_12 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_13 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_14 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_15 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_16 },
	{.delay = 0,   .size = 8,   .cmd = cmd_hx8394_17 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_18 },
	{.delay = 0,   .size = 2,   .cmd = cmd_hx8394_19 },
	// {.delay = 120, .size = 1,   .cmd = cmd_hx8394_20 },
	// {.delay = 20,  .size = 1,   .cmd = cmd_hx8394_21 }
};

#define DRV_NAME "panel-himax-hx8394"

struct hx8394 {
	struct device *dev;
	struct drm_panel panel;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *pwm_gpio;
	struct gpio_desc *power_gpio;
	bool enabled;
	bool prepared;

	const struct hx8394_panel_desc *desc;
};

struct hx8394_panel_desc {
	const struct drm_display_mode *mode;
	unsigned int lanes;
	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	int (*init_sequence)(struct hx8394 *ctx);
};

static inline struct hx8394 *panel_to_hx8394(struct drm_panel *panel)
{
	return container_of(panel, struct hx8394, panel);
}

static int hx8394_init_sequence(struct hx8394 *ctx)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(init_cmds_hx8394_720x1280); i++) {
		ret = mipi_dsi_dcs_write_buffer(dsi, init_cmds_hx8394_720x1280[i].cmd, init_cmds_hx8394_720x1280[i].size);
		if (ret < 0)
			return ret;
		msleep(init_cmds_hx8394_720x1280[i].delay);
	}

	return 0;
}

static const struct drm_display_mode hx8394_mode = {
	.hdisplay    = 720,
	.hsync_start = 720 + 128,
	.hsync_end   = 720 + 128 + 64,
	.htotal	     = 720 + 128 + 64 + 36,
	.vdisplay    = 1280,
	.vsync_start = 1280 + 6,
	.vsync_end   = 1280 + 6 + 16,
	.vtotal	     = 1280 + 6 + 16 + 4,
	.clock	     = 74250,
	.flags	     = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC,
	.width_mm    = 68,
	.height_mm   = 136,
};

static const struct hx8394_panel_desc hx8394_desc = {
	.mode = &hx8394_mode,
	.lanes = 4,
	.mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST,
	.format = MIPI_DSI_FMT_RGB888,
	.init_sequence = hx8394_init_sequence,
};

static int hx8394_enable(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);
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
	if (ret || data != 0x94)
		dev_err(ctx->dev, "Panel id read failed: %d\n", ret);

	msleep(20);

	cmd = 0xdc;
	ret = mipi_dsi_dcs_read(dsi, cmd, &data, len);
	if (ret || data != 0xf)
		dev_err(ctx->dev, "Panel id read failed: %d\n", ret);

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

static int hx8394_disable(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);
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

static int hx8394_unprepare(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);

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

static int hx8394_prepare(struct drm_panel *panel)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);

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

static int hx8394_get_modes(struct drm_panel *panel,
			    struct drm_connector *connector)
{
	struct hx8394 *ctx = panel_to_hx8394(panel);
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

static const struct drm_panel_funcs hx8394_drm_funcs = {
	.disable   = hx8394_disable,
	.unprepare = hx8394_unprepare,
	.prepare   = hx8394_prepare,
	.enable	   = hx8394_enable,
	.get_modes = hx8394_get_modes,
};

static int hx8394_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct hx8394 *ctx;
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

	drm_panel_init(&ctx->panel, dev, &hx8394_drm_funcs,
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

static void hx8394_shutdown(struct mipi_dsi_device *dsi)
{
	struct hx8394 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = drm_panel_disable(&ctx->panel);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to disable panel: %d\n", ret);

	ret = drm_panel_unprepare(&ctx->panel);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to unprepare panel: %d\n", ret);
}

static int hx8394_remove(struct mipi_dsi_device *dsi)
{
	struct hx8394 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	hx8394_shutdown(dsi);

	ret = mipi_dsi_detach(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);
		return ret;
	}

	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id hx8394_of_match[] = {
	{ .compatible = "cvitek,hx8394", .data = &hx8394_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hx8394_of_match);

static struct mipi_dsi_driver hx8394_driver = {
	.probe	= hx8394_probe,
	.remove = hx8394_remove,
	.shutdown = hx8394_shutdown,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = hx8394_of_match,
	},
};
module_mipi_dsi_driver(hx8394_driver);

MODULE_AUTHOR("Bowen Du <bowen.du@sophgo.com>");
MODULE_DESCRIPTION("DRM driver for Himax HX8394 based MIPI DSI panels");
MODULE_LICENSE("GPL");
