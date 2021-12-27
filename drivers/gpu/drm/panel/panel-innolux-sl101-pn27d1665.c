// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019-2020 Kali Prasad <kprasadvnsi@protonmail.com>
 */

#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>

#define _INIT_CMD(...) { \
	.len = sizeof((char[]){__VA_ARGS__}), \
	.data = (char[]){__VA_ARGS__} }

struct panel_init_cmd {
	u8 dtype;
	u8 wait;
	u8 dlen;
	const char *data;
};

static const char * const regulator_names[] = {
	"dvdd",
	"avdd",
	"cvdd"
};

struct sl101_pn27d1665 {
	struct drm_panel	panel;
	struct mipi_dsi_device	*dsi;

	struct regulator_bulk_data supplies[ARRAY_SIZE(regulator_names)];
	struct gpio_desc	*reset;
	struct gpio_desc	*enable;
};

static inline struct sl101_pn27d1665 *panel_to_sl101_pn27d1665(struct drm_panel *panel)
{
	printk(KERN_DEBUG "================ panel_to_sl101_pn27d1665\n");
	return container_of(panel, struct sl101_pn27d1665, panel);
}

/*
 * Display manufacturer failed to provide init sequencing according to
 * https://chromium-review.googlesource.com/c/chromiumos/third_party/coreboot/+/892065/
 * so the init sequence stems from a register dump of a working panel.
 */
static const struct panel_init_cmd sl101_pn27d1665_init_cmds[] = {
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb1, 0x68, 0x01 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xb6, 0x08 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x04, .data = (char[]){ 0xb8, 0x01, 0x02, 0x08 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbb, 0x44, 0x44 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbc, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xbd, 0x02, 0x68, 0x10, 0x10, 0x00 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xc8, 0x80 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x01 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb3, 0x4f, 0x4f }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb4, 0x10, 0x10 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb5, 0x05, 0x05 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb9, 0x35, 0x35 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x04, .data = (char[]){ 0xba, 0xba, 0x25, 0x25 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbc, 0x68, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbd, 0x68, 0x00 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xbe, 0x30 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xc0, 0x0c }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xca, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x02 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xee, 0x01 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x11, .data = (char[]){ 0xb0, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x2a, 0x00, 0x40, 0x00, 0x54, 0x00, 0x76, 0x00, 0x93, 0x00, 0xc5 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x11, .data = (char[]){ 0xb1, 0x00, 0xf0, 0x01, 0x32, 0x01, 0x66, 0x01, 0xbb, 0x01, 0xff, 0x02, 0x01, 0x02, 0x42, 0x02, 0x85 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x11, .data = (char[]){ 0xb2, 0x02, 0xaf, 0x02, 0xe0, 0x03, 0x05, 0x03, 0x35, 0x03, 0x54, 0x03, 0x84, 0x03, 0xa0, 0x03, 0xc4 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x05, .data = (char[]){ 0xb3, 0x03, 0xf2, 0x03, 0xff }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x03 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb0, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb1, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xb2, 0x08, 0x00, 0x17, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xb6, 0x05, 0x00, 0x00, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xba, 0x53, 0x00, 0xa0, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xbb, 0x53, 0x00, 0xa0, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x05, .data = (char[]){ 0xc0, 0x00, 0x00, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x05, .data = (char[]){ 0xc1, 0x00, 0x00, 0x00, 0x00 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xc4, 0x60 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xc5, 0xc0 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x05 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb0, 0x17, 0x06 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb1, 0x17, 0x06 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb2, 0x17, 0x06 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb3, 0x17, 0x06 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb4, 0x17, 0x06 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb5, 0x17, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xb8, 0x0c }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xb9, 0x00 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xba, 0x00 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xbb, 0x0a }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xbc, 0x02 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xbd, 0x03, 0x01, 0x01, 0x03, 0x03 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xc0, 0x07 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xc4, 0xa2 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc8, 0x03, 0x20 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc9, 0x01, 0x21 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x04, .data = (char[]){ 0xcc, 0x00, 0x00, 0x01 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x04, .data = (char[]){ 0xcd, 0x00, 0x00, 0x01 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xd1, 0x00, 0x04, 0xfc, 0x07, 0x14 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xd2, 0x10, 0x05, 0x00, 0x03, 0x16 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xe5, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xe6, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xe7, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xe8, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xe9, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xea, 0x06 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xed, 0x30 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x06 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb0, 0x17, 0x11 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb1, 0x16, 0x10 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb2, 0x12, 0x18 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb3, 0x13, 0x19 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb4, 0x00, 0x31 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb5, 0x31, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb6, 0x34, 0x29 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb7, 0x2a, 0x33 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb8, 0x2e, 0x2d }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xb9, 0x08, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xba, 0x34, 0x08 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbb, 0x2d, 0x2e }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbc, 0x34, 0x2a }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbd, 0x29, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbe, 0x13, 0x03 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xbf, 0x31, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc0, 0x19, 0x13 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc1, 0x18, 0x12 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc2, 0x10, 0x16 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc3, 0x11, 0x17 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xe5, 0x34, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc4, 0x12, 0x18 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc5, 0x13, 0x19 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc6, 0x17, 0x11 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc7, 0x16, 0x10 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc8, 0x08, 0x31 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xc9, 0x31, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xca, 0x34, 0x29 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xcb, 0x2a, 0x33 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xcc, 0x2d, 0x2e }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xcd, 0x00, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xce, 0x34, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xcf, 0x2e, 0x2d }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd0, 0x34, 0x2a }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd1, 0x29, 0x2a }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd2, 0x34, 0x31 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd3, 0x31, 0x08 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd4, 0x10, 0x16 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd5, 0x11, 0x17 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd6, 0x19, 0x13 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xd7, 0x18, 0x12 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x03, .data = (char[]){ 0xe6, 0x34, 0x34 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00 }},
	{ .dtype = 0x39, .wait = 0x00, .dlen = 0x06, .data = (char[]){ 0xd9, 0x00, 0x00, 0x00, 0x00, 0x00 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0xe7, 0x00 }},
	{ .dtype = 0x05, .wait = 0x78, .dlen = 0x01, .data = (char[]){ 0x11 }},
	{ .dtype = 0x05, .wait = 0x14, .dlen = 0x01, .data = (char[]){ 0x29 }},
	{ .dtype = 0x15, .wait = 0x00, .dlen = 0x02, .data = (char[]){ 0x35, 0x00 }},
	{ .dtype = 0x39, .wait = 0x14, .dlen = 0x06, .data = (char[]){ 0xf0, 0x55, 0xaa, 0x52, 0x08, 0x01 }},
};


static int sl101_pn27d1665_prepare(struct drm_panel *panel)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_prepare\n");
	struct sl101_pn27d1665 *ctx = panel_to_sl101_pn27d1665(panel);
	struct mipi_dsi_device *dsi = ctx->dsi;
	unsigned int i;
	int ret;

	gpiod_set_value(ctx->enable, 1);
	msleep(25);

	gpiod_set_value(ctx->reset, 1);
	msleep(25);

	gpiod_set_value(ctx->reset, 0);
	msleep(200);

	for (i = 0; i < ARRAY_SIZE(sl101_pn27d1665_init_cmds); i++) {
		const struct panel_init_cmd *cmd = &sl101_pn27d1665_init_cmds[i];

		switch (cmd->dtype) {
			case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
			case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
			case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
			case MIPI_DSI_GENERIC_LONG_WRITE:
				ret = mipi_dsi_generic_write(dsi, cmd->data,
								cmd->dlen);
				break;
			case MIPI_DSI_DCS_SHORT_WRITE:
			case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
			case MIPI_DSI_DCS_LONG_WRITE:
				ret = mipi_dsi_dcs_write_buffer(dsi, cmd->data,
								cmd->dlen);
				break;
			default:
				return -EINVAL;
		}

		if (ret < 0)
			goto powerdown;

		if (cmd->wait)
				msleep(cmd->wait);
	}

	return 0;

powerdown:
	gpiod_set_value(ctx->reset, 1);
	msleep(50);

	// return regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	return 0;
}

static int sl101_pn27d1665_enable(struct drm_panel *panel)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_enable\n");
	struct sl101_pn27d1665 *ctx = panel_to_sl101_pn27d1665(panel);
	int ret;

	msleep(150);

	ret = mipi_dsi_dcs_set_display_on(ctx->dsi);
	if (ret < 0)
		return ret;

	msleep(50);

	return 0;
}

static int sl101_pn27d1665_disable(struct drm_panel *panel)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_disable\n");
	struct sl101_pn27d1665 *ctx = panel_to_sl101_pn27d1665(panel);

	return mipi_dsi_dcs_set_display_off(ctx->dsi);
}

static int sl101_pn27d1665_unprepare(struct drm_panel *panel)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_unprepare\n");
	struct sl101_pn27d1665 *ctx = panel_to_sl101_pn27d1665(panel);
	int ret;

	ret = mipi_dsi_dcs_set_display_off(ctx->dsi);
	if (ret < 0)
		dev_err(panel->dev, "failed to set display off: %d\n", ret);

	ret = mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
	if (ret < 0)
		dev_err(panel->dev, "failed to enter sleep mode: %d\n", ret);

	msleep(200);

	gpiod_set_value(ctx->reset, 1);
	msleep(20);

	gpiod_set_value(ctx->enable, 0);
	msleep(20);

	// return regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	return 0;
}

static const struct drm_display_mode sl101_pn27d1665_default_mode = {
	.clock = 66000,

	.hdisplay    = 800,
	.hsync_start = 800 + 16,
	.hsync_end   = 800 + 16 + 5,
	.htotal      = 800 + 16 + 5 + 59,

	.vdisplay    = 1280,
	.vsync_start = 1280 + 8,
	.vsync_end   = 1280 + 8 + 5,
	.vtotal      = 1280 + 8 + 5 + 3,

	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	.width_mm	= 136,
	.height_mm	= 217,
};

static int sl101_pn27d1665_get_modes(struct drm_panel *panel,
				  struct drm_connector *connector)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_get_modes\n");
	struct sl101_pn27d1665 *ctx = panel_to_sl101_pn27d1665(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &sl101_pn27d1665_default_mode);
	if (!mode) {
		dev_err(&ctx->dsi->dev, "failed to add mode %ux%u@%u\n",
			sl101_pn27d1665_default_mode.hdisplay,
			sl101_pn27d1665_default_mode.vdisplay,
			drm_mode_vrefresh(&sl101_pn27d1665_default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);

	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs sl101_pn27d1665_funcs = {
	.disable = sl101_pn27d1665_disable,
	.unprepare = sl101_pn27d1665_unprepare,
	.prepare = sl101_pn27d1665_prepare,
	.enable = sl101_pn27d1665_enable,
	.get_modes = sl101_pn27d1665_get_modes,
};

static int sl101_pn27d1665_dsi_probe(struct mipi_dsi_device *dsi)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_dsi_probe\n");
	struct sl101_pn27d1665 *ctx;
	unsigned int i;
	int ret;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	printk(KERN_DEBUG "================ sl101_pn27d1665_dsi_probe -> 1\n");
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);
	ctx->dsi = dsi;

	ctx->enable = devm_gpiod_get(&dsi->dev, "enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->enable)) {
		dev_err(&dsi->dev, "Couldn't get our enable GPIO\n");
		return PTR_ERR(ctx->enable);
	}

	ctx->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset)) {
		dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(ctx->reset);
	}

	printk(KERN_DEBUG "================ sl101_pn27d1665_dsi_probe -> 2\n");
	drm_panel_init(&ctx->panel, &dsi->dev, &sl101_pn27d1665_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret){
		return ret;
	}
	printk(KERN_DEBUG "================ sl101_pn27d1665_dsi_probe -> 3\n");

	drm_panel_add(&ctx->panel);

	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return ret;
	}
	printk(KERN_DEBUG "================ sl101_pn27d1665_dsi_probe -> 4\n");
	return 0;
}

static int sl101_pn27d1665_dsi_remove(struct mipi_dsi_device *dsi)
{
	printk(KERN_DEBUG "================ sl101_pn27d1665_dsi_remove\n");
	struct sl101_pn27d1665 *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id sl101_pn27d1665_of_match[] = {
	{ .compatible = "innolux,sl101-pn27d1665", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sl101_pn27d1665_of_match);

static struct mipi_dsi_driver sl101_pn27d1665_driver = {
	.probe = sl101_pn27d1665_dsi_probe,
	.remove = sl101_pn27d1665_dsi_remove,
	.driver = {
		.name = "innolux-sl101-pn27d1665",
		.of_match_table = sl101_pn27d1665_of_match,
	},
};
module_mipi_dsi_driver(sl101_pn27d1665_driver);

MODULE_AUTHOR("Kali Prasad <kprasadvnsi@protonmail.com>");
MODULE_DESCRIPTION("Innolux SL101 PN27D1665 MIPI-DSI LCD panel");
MODULE_LICENSE("GPL");
