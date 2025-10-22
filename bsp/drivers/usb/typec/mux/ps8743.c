// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Parade PS8743 Type-C cross switch / mux driver
 *
 * Copyright (C) 2023 Allwinner Technology Co., Ltd.
 */

#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/usb/typec_dp.h>
#include <linux/usb/typec_mux.h>
#include <linux/of_gpio.h>

#define PS8743_CONF			0x00

#define PS8743_CONF_CE_DP		BIT(7)
#define PS8743_CONF_DP_MODE		BIT(6)
#define PS8743_CONF_CE_USB		BIT(5)
#define PS8743_CONF_USB_MODE		BIT(4)
#define PS8743_CONF_FLIP		BIT(3)
#define PS8743_CONF_FLIP_MODE		BIT(2)
#define PS8743_CONF_OPEN		(PS8743_CONF_CE_DP | PS8743_CONF_CE_USB | PS8743_CONF_FLIP)
#define PS8743_CONF_SWAP		PS8743_CONF_FLIP_MODE
#define PS8743_CONF_4LANE_DP		PS8743_CONF_DP_MODE
#define PS8743_CONF_USB3		PS8743_CONF_USB_MODE
#define PS8743_CONF_USB3_AND_2LANE_DP	(PS8743_CONF_DP_MODE | PS8743_CONF_USB_MODE)

/* USB Type-C connector facing receiver equalization setting */
#define PS8743_USB_RX_EQ_SETTING	0x3b
#define PS8743_USB_RX_EQ_DEFAULT	0x00 /* 2.4dB */

#define PS8743_REVISION_ID		0xf0
#define PS8743_CHIP_ID			0xf2
struct ps8743 {
	struct i2c_client *client;
	struct mutex lock; /* protects the cached conf register */
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
	struct typec_switch *sw;
	struct typec_mux *mux;
#else
	struct typec_switch_dev *sw;
	struct typec_mux_dev *mux;
#endif
	u16 revision;
	u16 chip;
	u8 conf;
	u8 rx_eq_emphasis;
	int hpd_gpio;
	bool hpd_status;
	bool hpd_status_pre;
	bool rx_eq_emphasis_quirk;
};

enum ps8743_hpd_state {
	PS8743_HOUTPLUG_OUT = 0,
	PS8743_HOUTPLUG_IN,
};

static ssize_t
dump_all_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ps8743 *pi = dev_get_drvdata(dev);
	s32 ret;
	u32 i = 0, show_cnt = 0;
	s32 data[256];

	for (i = 0 ; i < 0x100; i++) {
		ret = i2c_smbus_read_byte_data(pi->client, i);
			if (ret < 0) {
				dev_err(dev, "Error reading register 0x%02x\n", i);
				data[i] = 0;
			} else
				data[i] = ret;
	}

	for (i = 0; i < 0x100; i++) {
		if ((i % 0x8) == 0)
			show_cnt += sprintf(buf + show_cnt, "\n0x%02x:", i);
		show_cnt += sprintf(buf + show_cnt, "  0x%02x", data[i]);
	}

	return show_cnt;
}


static ssize_t
i2c_write_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct ps8743 *pi = dev_get_drvdata(dev);
	u8 value = 0, i = 0;
	u8 *separator = NULL;
	u8 reg_addr = 0;
	u8 reg_val[16];
	u32 reg_lenth = 0;
	s32 ret = 0;

	separator = strchr(buf, ' ');
	if (separator == NULL) {
		dev_err(dev, "%s,%d err, syntax error!\n", __func__, __LINE__);
	} else {
		reg_addr = simple_strtoul(buf, NULL, 0);

		for (i = 0; i < 16; i++) {
			value = simple_strtoul(separator + 1, NULL, 0);

			reg_val[i] = value;
			reg_lenth++;

			separator = strchr(separator + 1, ' ');
			if (separator == NULL)
				break;
		}

		for (i = 0; i < reg_lenth; i++) {
			ret = i2c_smbus_write_byte_data(pi->client, reg_addr + i,
							reg_val[i]);
			if (ret) {
				dev_err(dev, "Error writing [reg:0x%0x2] by [value:0x%02x]\n",
					 reg_addr + i, reg_val[i]);
				continue;
			}
		}
	}

	return count;
}


static DEVICE_ATTR_RO(dump_all_reg);
static DEVICE_ATTR_WO(i2c_write);


static struct attribute *ps8743_attrs[] = {
	&dev_attr_dump_all_reg.attr,
	&dev_attr_i2c_write.attr,
	NULL,
};

static const struct attribute_group ps8743_attr_group = {
	.name = "ps8743_attr",
	.attrs = ps8743_attrs,
};


static const struct attribute_group *ps8743_groups[] = {
	&ps8743_attr_group,
	NULL,
};


static void ps8743_notify_hotplug_in(int hpd_gpio)
{
	if (gpio_is_valid(hpd_gpio))
		gpio_set_value(hpd_gpio, PS8743_HOUTPLUG_IN);
	else
		pr_warn("hotplug gpio for ps8743 not config yet\n");
}

static void ps8743_notify_hotplug_out(int hpd_gpio)
{
	if (gpio_is_valid(hpd_gpio))
		gpio_set_value(hpd_gpio, PS8743_HOUTPLUG_OUT);
	else
		pr_warn("hotplug gpio for ps8743 not config yet\n");
}

static int ps8743_check_id(struct i2c_client *i2c)
{
	struct ps8743 *pi = i2c_get_clientdata(i2c);
	int ret;

	ret = i2c_smbus_read_word_data(i2c, PS8743_REVISION_ID);
	if (ret < 0) {
		dev_err(&i2c->dev, "fail to read revision id(%d)\n", ret);
		return ret;
	}
	pi->revision = ret;

	ret = i2c_smbus_read_word_data(i2c, PS8743_CHIP_ID);
	if (ret < 0) {
		dev_err(&i2c->dev, "fail to read chip id(%d)\n", ret);
		return ret;
	}
	pi->chip = ret;

	return 0;
}

static int ps8743_init_params(struct ps8743 *pi)
{
	int ret = 0;

	if (pi->rx_eq_emphasis_quirk) {
		ret = i2c_smbus_write_byte_data(pi->client, PS8743_USB_RX_EQ_SETTING,
						pi->rx_eq_emphasis);
		if (ret) {
			dev_err(&pi->client->dev, "Error writing rx eq setting: %d\n", ret);
			return ret;
		}
		dev_info(&pi->client->dev, "setting rx_eq_emphasis: 0x%02x\n", pi->rx_eq_emphasis);
	}

	return 0;
}

static int ps8743_set_conf(struct ps8743 *pi, u8 new_conf)
{
	int ret = 0;

	if (pi->conf == new_conf)
		return 0;

	ret = i2c_smbus_write_byte_data(pi->client, PS8743_CONF, new_conf);
	if (ret) {
		dev_err(&pi->client->dev, "Error writing conf: %d\n", ret);
		return ret;
	}

	pi->conf = new_conf;
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
static int ps8743_sw_set(struct typec_switch *sw,
			      enum typec_orientation orientation)
#else
static int ps8743_sw_set(struct typec_switch_dev *sw,
			      enum typec_orientation orientation)
#endif
{
	struct ps8743 *pi = typec_switch_get_drvdata(sw);
	u8 new_conf;
	int ret;

	mutex_lock(&pi->lock);
	new_conf = pi->conf;

	switch (orientation) {
	case TYPEC_ORIENTATION_NONE:
		new_conf = PS8743_CONF_OPEN;
		break;
	case TYPEC_ORIENTATION_NORMAL:
		new_conf &= ~PS8743_CONF_SWAP;
		break;
	case TYPEC_ORIENTATION_REVERSE:
		new_conf |= PS8743_CONF_SWAP;
		break;
	}

	dev_info(&pi->client->dev, "orientation:%d, set sw conf 0x%02x -> 0x%02x\n",
		 orientation, pi->conf, new_conf);

	/* Set Default Mode: U3 */
	new_conf |= PS8743_CONF_USB3;

	ret = ps8743_set_conf(pi, new_conf);
	mutex_unlock(&pi->lock);

	return ret;
}

static int
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 18, 0)
ps8743_mux_set(struct typec_mux *mux, struct typec_mux_state *state)
#else
ps8743_mux_set(struct typec_mux_dev *mux, struct typec_mux_state *state)
#endif
{
	struct ps8743 *pi = typec_mux_get_drvdata(mux);
	u8 new_conf;
	int ret;

	mutex_lock(&pi->lock);
	new_conf = pi->conf;

	/* Clear Default Mode: U3 */
	new_conf &= ~PS8743_CONF_USB3;

	switch (state->mode) {
	case TYPEC_STATE_SAFE:
		new_conf = (new_conf & PS8743_CONF_SWAP) |
			   PS8743_CONF_OPEN;
		pi->hpd_status = false;
		break;
	case TYPEC_STATE_USB:
		new_conf = (new_conf & PS8743_CONF_SWAP) |
			   PS8743_CONF_OPEN | PS8743_CONF_USB3;
		pi->hpd_status = false;
		break;
	case TYPEC_DP_STATE_C:
	case TYPEC_DP_STATE_E:
		new_conf = (new_conf & PS8743_CONF_SWAP) |
			   PS8743_CONF_OPEN | PS8743_CONF_4LANE_DP;
		pi->hpd_status = true;
		break;
	case TYPEC_DP_STATE_D:
	case TYPEC_DP_STATE_F:
		new_conf = (new_conf & PS8743_CONF_SWAP) |
			   PS8743_CONF_OPEN | PS8743_CONF_USB3_AND_2LANE_DP;
		pi->hpd_status = true;
		break;
	default:
		break;
	}

	dev_info(&pi->client->dev, "mode:%ld, set mux conf 0x%02x -> 0x%02x\n",
		 state->mode, pi->conf, new_conf);

	ret = ps8743_set_conf(pi, new_conf);

	if (pi->hpd_status != pi->hpd_status_pre) {
		if (pi->hpd_status)
			ps8743_notify_hotplug_in(pi->hpd_gpio);
		else
			ps8743_notify_hotplug_out(pi->hpd_gpio);
	}
	pi->hpd_status_pre = pi->hpd_status;

	mutex_unlock(&pi->lock);

	return ret;
}

static int ps8743_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct typec_switch_desc sw_desc = { };
	struct typec_mux_desc mux_desc = { };
	struct ps8743 *pi;
	struct device_node *node = dev->of_node;
	int ret;
	char hpd_gpio_name[20];
	u32 rx_eq_emphasis = 0;

	pi = devm_kzalloc(dev, sizeof(*pi), GFP_KERNEL);
	if (!pi)
		return -ENOMEM;

	pi->client = client;
	mutex_init(&pi->lock);

	ret = i2c_smbus_read_byte_data(client, PS8743_CONF);
	if (ret < 0) {
		dev_err(dev, "Error reading config register %d\n", ret);
		return ret;
	}
	pi->conf = ret;
	pi->rx_eq_emphasis_quirk = device_property_read_bool(dev, "aw,rx_eq_emphasis_quirk");
	device_property_read_u32(dev, "aw,rx_eq_emphasis", &rx_eq_emphasis);
	pi->rx_eq_emphasis = rx_eq_emphasis;

	sw_desc.drvdata = pi;
	sw_desc.fwnode = dev->fwnode;
	sw_desc.set = ps8743_sw_set;

	pi->sw = typec_switch_register(dev, &sw_desc);
	if (IS_ERR(pi->sw)) {
		dev_err(dev, "Error registering typec switch: %ld\n",
			PTR_ERR(pi->sw));
		return PTR_ERR(pi->sw);
	}

	mux_desc.drvdata = pi;
	mux_desc.fwnode = dev->fwnode;
	mux_desc.set = ps8743_mux_set;

	pi->mux = typec_mux_register(dev, &mux_desc);
	if (IS_ERR(pi->mux)) {
		typec_switch_unregister(pi->sw);
		dev_err(dev, "Error registering typec mux: %ld\n",
			PTR_ERR(pi->mux));
		return PTR_ERR(pi->mux);
	}

	i2c_set_clientdata(client, pi);
	dev_set_drvdata(dev, pi);

	ret = ps8743_check_id(client);
	if (ret < 0)
		return ret;

	/* Here are some compatible issues for PCB trace or board. */
	ps8743_init_params(pi);

	sprintf(hpd_gpio_name, "hotplug");
	pi->hpd_gpio = of_get_named_gpio(node, hpd_gpio_name, 0);

	if (!gpio_is_valid(pi->hpd_gpio)) {
		pr_warn("get hotplug pin for ps8743 failed, hotplug may be useless!\n");
	} else {
		/* init hotplug state to plugout */
		devm_gpio_request(dev, pi->hpd_gpio, hpd_gpio_name);
		gpio_direction_output(pi->hpd_gpio, PS8743_HOUTPLUG_OUT);
	}
	pi->hpd_status = false;
	pi->hpd_status_pre = false;

	dev_info(dev, "Revision ID:0x%04x, Chip ID:0x%4x, probe success\n", pi->revision, pi->chip);

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
static int ps8743_remove(struct i2c_client *client)
{
	struct ps8743 *pi = i2c_get_clientdata(client);

	typec_mux_unregister(pi->mux);
	typec_switch_unregister(pi->sw);
	return 0;
}
#else
static void ps8743_remove(struct i2c_client *client)
{
	struct ps8743 *pi = i2c_get_clientdata(client);

	typec_mux_unregister(pi->mux);
	typec_switch_unregister(pi->sw);
}
#endif

static const struct i2c_device_id ps8743_table[] = {
	{ "ps8743", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ps8743_table);

static const struct of_device_id ps8743_of_match[] = {
	{ .compatible = "parade,ps8743" },
	{},
};
MODULE_DEVICE_TABLE(of, ps8743_of_match);

static struct i2c_driver ps8743_driver = {
	.driver = {
		.name = "ps8743",
		.of_match_table = ps8743_of_match,
		.dev_groups = ps8743_groups,
	},
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 3, 0)
	.probe_new	= ps8743_probe,
#else
	.probe		= ps8743_probe,
#endif
	.remove		= ps8743_remove,
	.id_table	= ps8743_table,
};

module_i2c_driver(ps8743_driver);

MODULE_ALIAS("platform:ps8743-i2c-driver");
MODULE_DESCRIPTION("Parade PS8743 Type-C mux driver");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.4");
