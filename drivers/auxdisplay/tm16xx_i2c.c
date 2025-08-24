// SPDX-License-Identifier: GPL-2.0
/*
 * TM16xx and compatible LED display/keypad controller driver
 * Supports TM16xx, FD6xx, PT6964, HBS658, AIP16xx and related chips.
 *
 * Copyright (C) 2025 Jean-François Lessard
 */

#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>

#include "tm16xx.h"

static int tm16xx_i2c_probe(struct i2c_client *client)
{
	const struct tm16xx_controller *controller;
	struct tm16xx_display *display;
	int ret;

	controller = i2c_get_match_data(client);
	if (!controller)
		return -EINVAL;

	display = devm_kzalloc(&client->dev, sizeof(*display), GFP_KERNEL);
	if (!display)
		return -ENOMEM;

	display->dev = &client->dev;
	display->controller = controller;

	i2c_set_clientdata(client, display);

	ret = tm16xx_probe(display);
	if (ret)
		return ret;

	return 0;
}

static void tm16xx_i2c_remove(struct i2c_client *client)
{
	struct tm16xx_display *display = i2c_get_clientdata(client);

	tm16xx_remove(display);
}

/**
 * tm16xx_i2c_write() - I2C write helper for controller
 * @display: pointer to tm16xx_display structure
 * @data: command and data bytes to send
 * @len: number of bytes in @data
 *
 * Return: 0 on success, negative error code on failure
 */
static int tm16xx_i2c_write(struct tm16xx_display *display, u8 *data, size_t len)
{
	struct i2c_client *i2c = to_i2c_client(display->dev);

	/* expected sequence: S Command [A] Data [A] P */
	struct i2c_msg msg = {
		.addr = data[0] >> 1,
		.flags = 0,
		.len = len - 1,
		.buf = &data[1],
	};
	int ret;

	ret = i2c_transfer(i2c->adapter, &msg, 1);
	if (ret < 0)
		return ret;

	return (ret == 1) ? 0 : -EIO;
}

/**
 * tm16xx_i2c_read() - I2C read helper for controller
 * @display: pointer to tm16xx_display structure
 * @cmd: command/address byte to send before reading
 * @data: buffer to receive data
 * @len: number of bytes to read into @data
 *
 * Return: 0 on success, negative error code on failure
 */
static int tm16xx_i2c_read(struct tm16xx_display *display, u8 cmd, u8 *data, size_t len)
{
	struct i2c_client *i2c = to_i2c_client(display->dev);

	/* expected sequence: S Command [A] [Data] [A] P */
	struct i2c_msg msgs[1] = {{
		.addr = cmd >> 1,
		.flags = I2C_M_RD | I2C_M_NO_RD_ACK,
		.len = len,
		.buf = data,
	}};
	int ret;

	ret = i2c_transfer(i2c->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0)
		return ret;

	return (ret == ARRAY_SIZE(msgs)) ? 0 : -EIO;
}

/* I2C controller-specific functions */
static int tm1650_init(struct tm16xx_display *display)
{
	const enum led_brightness brightness = display->main_led.brightness;
	u8 cmds[2];

	cmds[0] = TM1650_CMD_CTRL;
	cmds[1] = TM16XX_CTRL_BRIGHTNESS(brightness, brightness, TM1650) |
		  TM1650_CTRL_SEG8_MODE;

	return tm16xx_i2c_write(display, cmds, ARRAY_SIZE(cmds));
}

static int tm1650_data(struct tm16xx_display *display, u8 index,
		       unsigned int grid)
{
	u8 cmds[2];

	cmds[0] = TM1650_CMD_ADDR + index * 2;
	cmds[1] = grid; /* SEG 1 to 8 */

	return tm16xx_i2c_write(display, cmds, ARRAY_SIZE(cmds));
}

static int tm1650_keys(struct tm16xx_display *display)
{
	int row, col;
	bool pressed;
	u8 keycode;
	int ret;

	ret = tm16xx_i2c_read(display, TM1650_CMD_READ, &keycode, 1);
	if (ret)
		return ret;

	if (keycode == 0x00 || keycode == 0xFF)
		return -EINVAL;

	row = FIELD_GET(TM1650_KEY_ROW_MASK, keycode);
	pressed = FIELD_GET(TM1650_KEY_DOWN_MASK, keycode) != 0;
	if ((keycode & TM1650_KEY_COMBINED) == TM1650_KEY_COMBINED) {
		tm16xx_set_key(display, row, 0, pressed);
		tm16xx_set_key(display, row, 1, pressed);
	} else {
		col = FIELD_GET(TM1650_KEY_COL_MASK, keycode);
		tm16xx_set_key(display, row, col, pressed);
	}

	return 0;
}

static int fd655_init(struct tm16xx_display *display)
{
	const enum led_brightness brightness = display->main_led.brightness;
	u8 cmds[2];

	cmds[0] = FD655_CMD_CTRL;
	cmds[1] = TM16XX_CTRL_BRIGHTNESS(brightness, brightness % 3, FD655);

	return tm16xx_i2c_write(display, cmds, ARRAY_SIZE(cmds));
}

static int fd655_data(struct tm16xx_display *display, u8 index,
		      unsigned int grid)
{
	u8 cmds[2];

	cmds[0] = FD655_CMD_ADDR + index * 2;
	cmds[1] = grid; /* SEG 1 to 8 */

	return tm16xx_i2c_write(display, cmds, ARRAY_SIZE(cmds));
}

static int fd6551_init(struct tm16xx_display *display)
{
	const enum led_brightness brightness = display->main_led.brightness;
	u8 cmds[2];

	cmds[0] = FD6551_CMD_CTRL;
	cmds[1] = TM16XX_CTRL_BRIGHTNESS(brightness, ~(brightness - 1), FD6551);

	return tm16xx_i2c_write(display, cmds, ARRAY_SIZE(cmds));
}

static void hbs658_swap_nibbles(u8 *data, size_t len)
{
	for (size_t i = 0; i < len; i++)
		data[i] = (data[i] << 4) | (data[i] >> 4);
}

static int hbs658_init(struct tm16xx_display *display)
{
	const enum led_brightness brightness = display->main_led.brightness;
	u8 cmd;
	int ret;

	/* Set data command */
	cmd = TM16XX_CMD_WRITE | TM16XX_DATA_ADDR_AUTO;
	hbs658_swap_nibbles(&cmd, 1);
	ret = tm16xx_i2c_write(display, &cmd, 1);
	if (ret)
		return ret;

	/* Set control command with brightness */
	cmd = TM16XX_CMD_CTRL |
	      TM16XX_CTRL_BRIGHTNESS(brightness, brightness - 1, TM16XX);
	hbs658_swap_nibbles(&cmd, 1);
	ret = tm16xx_i2c_write(display, &cmd, 1);
	if (ret)
		return ret;

	return 0;
}

static int hbs658_data(struct tm16xx_display *display, u8 index,
		       unsigned int grid)
{
	u8 cmds[2];

	cmds[0] = TM16XX_CMD_ADDR + index * 2;
	cmds[1] = grid;

	hbs658_swap_nibbles(cmds, ARRAY_SIZE(cmds));
	return tm16xx_i2c_write(display, cmds, ARRAY_SIZE(cmds));
}

static int hbs658_keys(struct tm16xx_display *display)
{
	u8 cmd, keycode;
	int col;
	int ret;

	cmd = TM16XX_CMD_READ;
	hbs658_swap_nibbles(&cmd, 1);
	ret = tm16xx_i2c_read(display, cmd, &keycode, 1);
	if (ret)
		return ret;

	hbs658_swap_nibbles(&keycode, 1);

	if (keycode != 0xFF) {
		col = FIELD_GET(HBS658_KEY_COL_MASK, keycode);
		tm16xx_set_key(display, 0, col, true);
	}

	return 0;
}

/* I2C controller definitions */
static const struct tm16xx_controller tm1650_controller = {
	.max_grids = 4,
	.max_segments = 8,
	.max_brightness = 8,
	.max_key_rows = 4,
	.max_key_cols = 7,
	.init = tm1650_init,
	.data = tm1650_data,
	.keys = tm1650_keys,
};

static const struct tm16xx_controller fd655_controller = {
	.max_grids = 5,
	.max_segments = 7,
	.max_brightness = 3,
	.max_key_rows = 5,
	.max_key_cols = 7,
	.init = fd655_init,
	.data = fd655_data,
	.keys = tm1650_keys,
};

static const struct tm16xx_controller fd6551_controller = {
	.max_grids = 5,
	.max_segments = 7,
	.max_brightness = 8,
	.max_key_rows = 0,
	.max_key_cols = 0,
	.init = fd6551_init,
	.data = fd655_data,
};

static const struct tm16xx_controller hbs658_controller = {
	.max_grids = 5,
	.max_segments = 8,
	.max_brightness = 8,
	.max_key_rows = 1,
	.max_key_cols = 8,
	.init = hbs658_init,
	.data = hbs658_data,
	.keys = hbs658_keys,
};

static const struct of_device_id tm16xx_i2c_of_match[] = {
	{ .compatible = "titanmec,tm1650", .data = &tm1650_controller },
	{ .compatible = "fdhisi,fd6551",   .data = &fd6551_controller },
	{ .compatible = "fdhisi,fd655",    .data = &fd655_controller  },
	{ .compatible = "winrise,hbs658",  .data = &hbs658_controller },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, tm16xx_i2c_of_match);

static const struct i2c_device_id tm16xx_i2c_id[] = {
	{ "tm1650", (kernel_ulong_t)&tm1650_controller },
	{ "fd6551", (kernel_ulong_t)&fd6551_controller },
	{ "fd655",  (kernel_ulong_t)&fd655_controller  },
	{ "hbs658", (kernel_ulong_t)&hbs658_controller },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, tm16xx_i2c_id);

static struct i2c_driver tm16xx_i2c_driver = {
	.driver = {
		.name = "tm16xx-i2c",
		.of_match_table = tm16xx_i2c_of_match,
	},
	.probe = tm16xx_i2c_probe,
	.remove = tm16xx_i2c_remove,
	.shutdown = tm16xx_i2c_remove,
	.id_table = tm16xx_i2c_id,
};
module_i2c_driver(tm16xx_i2c_driver);

MODULE_AUTHOR("Jean-François Lessard");
MODULE_DESCRIPTION("TM16xx-i2c LED Display Controllers");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("TM16XX");
