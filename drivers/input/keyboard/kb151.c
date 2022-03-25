// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (C) 2021 Samuel Holland <samuel@sholland.org>
// Copyright (C) 2022 Ondřej Jirman <megi@xff.cz>

#include <linux/crc8.h>
#include <linux/i2c.h>
#include <linux/input/matrix_keypad.h>
#include <dt-bindings/input/input.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/pm_wakeirq.h>
#include <linux/regulator/consumer.h>

#define KB151_CRC8_POLYNOMIAL		0x07

#define KB151_DEVICE_ID_HI		0x00
#define KB151_DEVICE_ID_HI_VALUE	0x4b
#define KB151_DEVICE_ID_LO		0x01
#define KB151_DEVICE_ID_LO_VALUE	0x42
#define KB151_FW_REVISION		0x02
#define KB151_FW_FEATURES		0x03
#define KB151_MATRIX_SIZE		0x06
#define KB151_SCAN_CRC			0x07
#define KB151_SCAN_DATA			0x08
#define KB151_SYS_CONFIG		0x20
#define KB151_SYS_CONFIG_DISABLE_SCAN	BIT(0)

/* imported from firmware/registers.h */

#define REG_DEVID_K		0x00
#define REG_DEVID_B		0x01
#define REG_FW_REVISION		0x02
#define REG_FW_FEATURES		0x03
#define REG_FW_FEATURES_USB_DEBUGGER	BIT(0)
#define REG_FW_FEATURES_FLASHING_MODE	BIT(1)
#define REG_FW_FEATURES_SELF_TEST	BIT(2)
#define REG_FW_FEATURES_STOCK_FW	BIT(3)
#define REG_FW_FEATURES_I2CA		BIT(4)

#define REG_KEYMATRIX_SIZE	0x06
#define REG_KEYMATRIX_STATE_CRC8 0x07
#define REG_KEYMATRIX_STATE	0x08
#define REG_KEYMATRIX_STATE_END	0x13

#define REG_SYS_CONFIG		0x20
#define REG_SYS_CONFIG_SCAN_BLOCK	BIT(0)

#define REG_SYS_CHG_ADDR	0x21
#define REG_SYS_CHG_DATA	0x22

#define REG_SYS_COMMAND		0x23
#define REG_SYS_COMMAND_MCU_RESET	'r'
#define REG_SYS_COMMAND_USB_IAP		'i'
#define REG_SYS_COMMAND_SELFTEST	't'
#define REG_SYS_COMMAND_CHG_READ	0x91
#define REG_SYS_COMMAND_CHG_WRITE	0xA1

#define REG_SYS_USER_APP_BLOCK	0x24
#define REG_SYS_USER_APP_BLOCK_MAGIC	0x53

#define REG_FLASH_DATA_START	0x70
#define REG_FLASH_DATA_END	0xef
#define REG_FLASH_ADDR_L	0xf0
#define REG_FLASH_ADDR_H	0xf1
#define REG_FLASH_CRC8		0xf2

#define REG_FLASH_UNLOCK	0xf3
#define REG_FLASH_UNLOCK_MAGIC	0x46

#define REG_FLASH_CMD		0xf4
#define REG_FLASH_CMD_READ_ROM	0x52
#define REG_FLASH_CMD_WRITE_ROM	0x57
#define REG_FLASH_CMD_ERASE_ROM	0x45
#define REG_FLASH_CMD_COMMIT	0x43

#define REG_DEBUG_LOG		0xff

/* default regmap from the factory */

#define DEFAULT_MAP_ROWS 12
#define DEFAULT_MAP_COLS 12

static bool disable_input;
module_param(disable_input, bool, S_IRUGO);
MODULE_PARM_DESC(disable_input, "Disable the keyboard part of the driver");

static bool disable_fn_layer;
module_param(disable_fn_layer, bool, 0644);
MODULE_PARM_DESC(disable_fn_layer, "Disable the keyboard's Fn key layer map");

static const u32 kb151_default_keymap[] = {
	MATRIX_KEY(0,  0, KEY_ESC),
	MATRIX_KEY(0,  1, KEY_1),
	MATRIX_KEY(0,  2, KEY_2),
	MATRIX_KEY(0,  3, KEY_3),
	MATRIX_KEY(0,  4, KEY_4),
	MATRIX_KEY(0,  5, KEY_5),
	MATRIX_KEY(0,  6, KEY_6),
	MATRIX_KEY(0,  7, KEY_7),
	MATRIX_KEY(0,  8, KEY_8),
	MATRIX_KEY(0,  9, KEY_9),
	MATRIX_KEY(0, 10, KEY_0),
	MATRIX_KEY(0, 11, KEY_BACKSPACE),
	MATRIX_KEY(1,  0, KEY_TAB),
	MATRIX_KEY(1,  1, KEY_Q),
	MATRIX_KEY(1,  2, KEY_W),
	MATRIX_KEY(1,  3, KEY_E),
	MATRIX_KEY(1,  4, KEY_R),
	MATRIX_KEY(1,  5, KEY_T),
	MATRIX_KEY(1,  6, KEY_Y),
	MATRIX_KEY(1,  7, KEY_U),
	MATRIX_KEY(1,  8, KEY_I),
	MATRIX_KEY(1,  9, KEY_O),
	MATRIX_KEY(1, 10, KEY_P),
	MATRIX_KEY(1, 11, KEY_ENTER),
	MATRIX_KEY(2,  0, KEY_LEFTMETA),
	MATRIX_KEY(2,  1, KEY_A),
	MATRIX_KEY(2,  2, KEY_S),
	MATRIX_KEY(2,  3, KEY_D),
	MATRIX_KEY(2,  4, KEY_F),
	MATRIX_KEY(2,  5, KEY_G),
	MATRIX_KEY(2,  6, KEY_H),
	MATRIX_KEY(2,  7, KEY_J),
	MATRIX_KEY(2,  8, KEY_K),
	MATRIX_KEY(2,  9, KEY_L),
	MATRIX_KEY(2, 10, KEY_SEMICOLON),
	MATRIX_KEY(3,  0, KEY_LEFTSHIFT),
	MATRIX_KEY(3,  1, KEY_Z),
	MATRIX_KEY(3,  2, KEY_X),
	MATRIX_KEY(3,  3, KEY_C),
	MATRIX_KEY(3,  4, KEY_V),
	MATRIX_KEY(3,  5, KEY_B),
	MATRIX_KEY(3,  6, KEY_N),
	MATRIX_KEY(3,  7, KEY_M),
	MATRIX_KEY(3,  8, KEY_COMMA),
	MATRIX_KEY(3,  9, KEY_DOT),
	MATRIX_KEY(3, 10, KEY_SLASH),
	MATRIX_KEY(4,  1, KEY_LEFTCTRL),
	MATRIX_KEY(4,  4, KEY_SPACE),
	MATRIX_KEY(4,  6, KEY_APOSTROPHE),
	MATRIX_KEY(4,  8, KEY_RIGHTBRACE),
	MATRIX_KEY(4,  9, KEY_LEFTBRACE),
	MATRIX_KEY(5,  2, KEY_FN),
	MATRIX_KEY(5,  3, KEY_LEFTALT),
	MATRIX_KEY(5,  5, KEY_RIGHTALT),

	/* FN layer */
	MATRIX_KEY(6,  1, KEY_BACKSLASH), // |
	MATRIX_KEY(6,  2, KEY_BACKSLASH),
	MATRIX_KEY(6,  3, KEY_DOLLAR),
	MATRIX_KEY(6,  4, KEY_EURO),
	MATRIX_KEY(6,  5, KEY_GRAVE), // ~
	MATRIX_KEY(6,  6, KEY_GRAVE),
	MATRIX_KEY(6,  7, KEY_MINUS), // _
	MATRIX_KEY(6,  8, KEY_EQUAL),
	MATRIX_KEY(6,  9, KEY_MINUS),
	MATRIX_KEY(6, 10, KEY_EQUAL),
	MATRIX_KEY(6, 11, KEY_DELETE),

	MATRIX_KEY(8,  0, KEY_SYSRQ),
	MATRIX_KEY(8, 10, KEY_INSERT),

	MATRIX_KEY(9,  0, KEY_LEFTSHIFT),
	MATRIX_KEY(9,  8, KEY_HOME),
	MATRIX_KEY(9,  9, KEY_UP),
	MATRIX_KEY(9, 10, KEY_END),

	MATRIX_KEY(10, 1, KEY_LEFTCTRL),
	MATRIX_KEY(10, 6, KEY_LEFT),
	MATRIX_KEY(10, 8, KEY_RIGHT),
	MATRIX_KEY(10, 9, KEY_DOWN),

	MATRIX_KEY(11, 2, KEY_FN),
	MATRIX_KEY(11, 3, KEY_LEFTALT),
	MATRIX_KEY(11, 5, KEY_RIGHTALT),
};

static const struct matrix_keymap_data kb151_default_keymap_data = {
	.keymap = kb151_default_keymap,
	.keymap_size = ARRAY_SIZE(kb151_default_keymap),
};

struct kb151 {
	struct input_dev *input;
	struct regulator *vbat_supply;
	u8 crc_table[CRC8_TABLE_SIZE];
	u8 row_shift;
	u8 rows;
	u8 cols;
	u8 fn_state;
	u8 buf_swap;
	u8 buf[];
};

static void kb151_update(struct i2c_client *client)
{
	struct kb151 *kb151 = i2c_get_clientdata(client);
	unsigned short *keymap = kb151->input->keycode;
	struct device *dev = &client->dev;
	size_t buf_len = kb151->cols + 1;
	u8 *old_buf = kb151->buf;
	u8 *new_buf = kb151->buf;
	int col, crc, ret, row;

	if (kb151->buf_swap)
		old_buf += buf_len;
	else
		new_buf += buf_len;

	ret = i2c_smbus_read_i2c_block_data(client, KB151_SCAN_CRC,
					    buf_len, new_buf);
	if (ret != buf_len) {
		dev_err(dev, "Failed to read scan data: %d\n", ret);
		return;
	}

	dev_dbg(dev, "%02x | %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
		new_buf[0], new_buf[1], new_buf[2], new_buf[3], new_buf[4], new_buf[5],
		new_buf[6], new_buf[7], new_buf[8], new_buf[9], new_buf[10], new_buf[11],
		new_buf[12]);
	crc = crc8(kb151->crc_table, new_buf + 1, kb151->cols, CRC8_INIT_VALUE);
	if (crc != new_buf[0]) {
		dev_err(dev, "Bad scan data (%02x != %02x)\n",
			crc, new_buf[0]);
		return;
	}

	for (col = 0; col < kb151->cols; ++col) {
		u8 old = *(++old_buf);
		u8 new = *(++new_buf);
		u8 changed = old ^ new;

		for (row = 0; row < kb151->rows; ++row) {
			u8 pressed = new & BIT(row);
			u8 map_row = row + (kb151->fn_state ? kb151->rows : 0);
			int code = MATRIX_SCAN_CODE(map_row, col, kb151->row_shift);

			if (!(changed & BIT(row)))
				continue;

			dev_dbg(&client->dev, "row %u col %u %sed\n",
				map_row, col, pressed ? "press" : "releas");
			if (keymap[code] == KEY_FN && !disable_fn_layer) {
				dev_dbg(&client->dev, "FN is now %s\n",
					pressed ? "pressed" : "released");
				kb151->fn_state = pressed;
			} else {
				input_report_key(kb151->input, keymap[code], pressed);
			}
		}
	}
	input_sync(kb151->input);

	kb151->buf_swap = !kb151->buf_swap;
}

static int kb151_open(struct input_dev *input)
{
	struct i2c_client *client = input_get_drvdata(input);
	struct device *dev = &client->dev;
	int ret, val;

	ret = i2c_smbus_read_byte_data(client, KB151_SYS_CONFIG);
	if (ret < 0) {
		dev_err(dev, "Failed to read config: %d\n", ret);
		return ret;
	}

	val = ret & ~KB151_SYS_CONFIG_DISABLE_SCAN;
	ret = i2c_smbus_write_byte_data(client, KB151_SYS_CONFIG, val);
	if (ret) {
		dev_err(dev, "Failed to write config: %d\n", ret);
		return ret;
	}

	kb151_update(client);

	enable_irq(client->irq);

	return 0;
}

static void kb151_close(struct input_dev *input)
{
	struct i2c_client *client = input_get_drvdata(input);
	struct device *dev = &client->dev;
	int ret, val;

	disable_irq(client->irq);

	ret = i2c_smbus_read_byte_data(client, KB151_SYS_CONFIG);
	if (ret < 0) {
		dev_err(dev, "Failed to read config: %d\n", ret);
		return;
	}

	val = ret | KB151_SYS_CONFIG_DISABLE_SCAN;
	ret = i2c_smbus_write_byte_data(client, KB151_SYS_CONFIG, val);
	if (ret) {
		dev_err(dev, "Failed to write config: %d\n", ret);
	}
}

static irqreturn_t kb151_irq_thread(int irq, void *data)
{
	struct i2c_client *client = data;

	kb151_update(client);

	return IRQ_HANDLED;
}

#ifdef CONFIG_IP5XXX_POWER

static int kb151_charger_regmap_read(void *context,
				     unsigned int reg, unsigned int *val)
{
	struct i2c_client *client = context;
	int ret, i;

	// initiate read of data from the charger
	uint8_t buf[4] = { REG_SYS_CHG_ADDR, reg, 0xAA, REG_SYS_COMMAND_CHG_READ };
	struct i2c_msg msgs[] = {
		{ client->addr, 0, 4, buf },
	};

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(&client->dev, "Charger read failed (%d)\n", ret);
		return ret;
	}

	for (i = 0; i < 5; i++) {
		// read the result
		uint8_t buf2[1] = { REG_SYS_CHG_DATA, };
		uint8_t buf3[2] = { };
		struct i2c_msg msgs2[] = {
			{ client->addr, 0, 1, buf2 },
			{ client->addr, I2C_M_RD, sizeof(buf3), buf3 },
		};

		usleep_range(700, 1200);

		ret = i2c_transfer(client->adapter, msgs2, ARRAY_SIZE(msgs2));
		if (ret < 0) {
			dev_err(&client->dev, "Charger read failed (%d)\n", ret);
			return ret;
		}

		dev_dbg(&client->dev, "rd %02x %02x\n", buf3[0], buf3[1]);

		if (buf3[1] == REG_SYS_COMMAND_CHG_READ)
			continue;

		if (buf3[1] == 0) {
			*val = buf3[0];
			return 0;
		}

		if (buf3[1] == 0xff) {
			dev_err(&client->dev,
				"Charger read failed - MCU returned 0x%hhx\n",
				buf3[1]);
			return -EXDEV;
		}
	}

	dev_err(&client->dev, "Charger read failed - timeout (%d)\n", ret);
	return -ETIMEDOUT;
}

static int kb151_charger_regmap_write(void *context, unsigned int reg, unsigned int val)
{
	struct i2c_client *client = context;
	int ret, i;

	uint8_t buf[4] = { REG_SYS_CHG_ADDR, reg, val, REG_SYS_COMMAND_CHG_WRITE };
	struct i2c_msg msgs[] = {
		{ client->addr, 0, 4, buf },
	};

	dev_dbg(&client->dev, "wr 0x%02hhx: %02hhx\n", reg, val);

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		dev_err(&client->dev, "Charger write failed (%d)\n", ret);
		return ret;
	}

	for (i = 0; i < 5; i++) {
		// read the result
		uint8_t buf2[1] = { REG_SYS_COMMAND, };
		uint8_t buf3[1] = { };
		struct i2c_msg msgs2[] = {
			{ client->addr, 0, 1, buf2 },
			{ client->addr, I2C_M_RD, sizeof(buf3), buf3 },
		};

		usleep_range(700, 1200);

		ret = i2c_transfer(client->adapter, msgs2, ARRAY_SIZE(msgs2));
		if (ret < 0) {
			dev_err(&client->dev, "Charger write failed (%d)\n", ret);
			return ret;
		}

		if (buf3[0] == REG_SYS_COMMAND_CHG_WRITE)
			continue;

		if (buf3[0] == 0)
			return 0;

		if (buf3[0] == 0xff) {
			dev_err(&client->dev,
				"Charger write failed - MCU returned 0x%hhx\n",
				buf2[1]);
			return -EXDEV;
		}
	}

	dev_err(&client->dev, "Charger write failed - timeout (%d)\n", ret);
	return -ETIMEDOUT;
}

static struct regmap_bus kb151_charger_regmap_bus = {
	.reg_write = kb151_charger_regmap_write,
	.reg_read = kb151_charger_regmap_read,
};

static struct regmap_config kb151_charger_regmap_config = {
        .reg_bits       = 8,
        .reg_stride     = 1,
        .val_bits       = 8,
        .max_register   = 0xff,
        .cache_type     = REGCACHE_NONE,
};

extern int ip5xxx_power_probe_with_regmap(struct device* dev, struct regmap *regmap);

#endif

static int kb151_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	u8 info[KB151_MATRIX_SIZE + 1];
	unsigned int kb_rows, kb_cols;
	unsigned int map_rows = DEFAULT_MAP_ROWS, map_cols = DEFAULT_MAP_COLS;
	struct kb151 *kb151;
#ifdef CONFIG_IP5XXX_POWER
	struct regmap* regmap;
#endif
	bool has_of_keymap;
	int ret;

	has_of_keymap = of_property_read_bool(dev->of_node, "linux,keymap");

	ret = i2c_smbus_read_i2c_block_data(client, 0, sizeof(info), info);
	if (ret != sizeof(info)) {
		dev_err(dev, "KB151 was not detected on the bus (%d)\n", ret);
		return ret;
	}

	if (info[KB151_DEVICE_ID_HI] != KB151_DEVICE_ID_HI_VALUE ||
	    info[KB151_DEVICE_ID_LO] != KB151_DEVICE_ID_LO_VALUE) {
		dev_warn(dev, "Device on address %hu doesn't look like KB151\n",
			 client->addr);
		return -ENODEV;
	}

	dev_info(dev, "Found KB151 with firmware %d.%d (features=%#x)\n",
		 info[KB151_FW_REVISION] >> 4,
		 info[KB151_FW_REVISION] & 0xf,
		 info[KB151_FW_FEATURES]);

	if (has_of_keymap) {
		ret = matrix_keypad_parse_properties(dev, &map_rows, &map_cols);
		if (ret)
			return ret;
	}

	kb_rows = info[KB151_MATRIX_SIZE] & 0xf;
	kb_cols = info[KB151_MATRIX_SIZE] >> 4;
	if (map_rows != 2 * kb_rows || map_cols != kb_cols) {
		dev_err(dev, "Keyboard matrix is %ux%u, but key map is %ux%u\n",
			kb_rows, kb_cols, map_rows, map_cols);
		return -EINVAL;
	}

	/* Allocate two buffers, and include space for the CRC. */
	kb151 = devm_kzalloc(dev, struct_size(kb151, buf, 2 * (kb_cols + 1)), GFP_KERNEL);
	if (!kb151)
		return -ENOMEM;

	kb151->vbat_supply = devm_regulator_get(dev, "vbat");
	if (IS_ERR(kb151->vbat_supply))
		return dev_err_probe(dev, PTR_ERR(kb151->vbat_supply),
				     "Failed to get vbat_supply\n");

	i2c_set_clientdata(client, kb151);

	crc8_populate_msb(kb151->crc_table, KB151_CRC8_POLYNOMIAL);

	kb151->row_shift = get_count_order(kb_cols);
	kb151->rows = kb_rows;
	kb151->cols = kb_cols;

	if (disable_input)
		goto charger;

	kb151->input = devm_input_allocate_device(dev);
	if (!kb151->input)
		return -ENOMEM;

	input_set_drvdata(kb151->input, client);

	kb151->input->name = client->name;
	kb151->input->phys = "kb151/input0";
	kb151->input->id.bustype = BUS_I2C;
	kb151->input->open = kb151_open;
	kb151->input->close = kb151_close;

	__set_bit(EV_REP, kb151->input->evbit);

	ret = matrix_keypad_build_keymap(has_of_keymap ? NULL : &kb151_default_keymap_data,
					 NULL, map_rows, map_cols,
					 NULL, kb151->input);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to build keymap\n");

	ret = devm_request_threaded_irq(dev, client->irq,
					NULL, kb151_irq_thread,
					IRQF_ONESHOT | IRQF_NO_AUTOEN,
					client->name, client);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to request IRQ\n");

	ret = input_register_device(kb151->input);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register input\n");

charger:
#ifdef CONFIG_IP5XXX_POWER
	// we need to create a custom regmap_bus that will proxy
	// charger register reads/writes via a keyboard MCU
	regmap = __regmap_lockdep_wrapper(__devm_regmap_init,
		 "kb151-charger-regmap", dev,
		  &kb151_charger_regmap_bus, client,
		  &kb151_charger_regmap_config);
	if (IS_ERR(regmap)) {
		dev_err(dev, "Failed to create charger regmap (%ld)\n", PTR_ERR(regmap));
		return PTR_ERR(regmap);
	}

	// initialize the charger
	ret = ip5xxx_power_probe_with_regmap(dev, regmap);
	if (ret) {
		dev_err(dev, "Failed to initialize keyboard charger (%d)\n", ret);
		return ret;
	}
#else
	dev_warn(dev, "Your kernel doesn't have CONFIG_IP5XXX_POWER enabled, keyboard charger support is disabled.\n");
#endif

	ret = regulator_enable(kb151->vbat_supply);
	if (ret) {
		dev_err(dev, "Failed to enable keyboard vbat supply (%d)\n", ret);
		return ret;
	}

	return 0;
}

static int kb151_remove(struct i2c_client *client)
{
	struct kb151 *kb151 = i2c_get_clientdata(client);

	regulator_disable(kb151->vbat_supply);

	return 0;
}

static const struct of_device_id kb151_of_match[] = {
	{ .compatible = "pine64,kb151" },
	{ }
};
MODULE_DEVICE_TABLE(of, kb151_of_match);

static struct i2c_driver kb151_driver = {
	.probe_new	= kb151_probe,
	.remove		= kb151_remove,
	.driver		= {
		.name		= "kb151",
		.of_match_table = kb151_of_match,
	},
};
module_i2c_driver(kb151_driver);

MODULE_AUTHOR("Samuel Holland <samuel@sholland.org>");
MODULE_DESCRIPTION("Pine64 KB151 keyboard driver");
MODULE_LICENSE("GPL");
