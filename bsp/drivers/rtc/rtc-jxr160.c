// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Real Time Clock (RTC) Driver for jxr160
 * Copyright (C) 2018 Zoro Li
 * Copyright (C) 2023 Allwinnertech Ltd.
 */

#include <linux/bcd.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/rtc.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/pm_wakeirq.h>

/* register layout */
#define JXR160_REG_SECOND		(0x00)
#define JXR160_REG_MINUTE		(0x01)
#define JXR160_REG_HOUR			(0x02)
#define JXR160_REG_WEEK			(0x03)
#define JXR160_REG_DAY			(0x04)
#define JXR160_REG_MONTH		(0x05)
#define JXR160_REG_YEAR			(0x06)
#define JXR160_REG_RAM			(0x07)
#define JXR160_REG_ALM_MINUTE		(0x08)
#define JXR160_REG_ALM_HOUR		(0x09)
#define JXR160_REG_ALM_WEEK		(0x0A)
#define JXR160_REG_ALM_DAY		(0x0A)
#define JXR160_REG_TC0			(0x0B)
#define JXR160_REG_TC1			(0x0C)
#define JXR160_REG_EXTENSION		(0x0D)
#define JXR160_REG_FLAG			(0x0E)
#define JXR160_REG_CONTROL		(0x0F)

#define JXR160_WADA			BIT(6)
#define JXR160_AIE			BIT(3)
#define JXR160_AF			BIT(3)

#define NUM_TIME_REGS			(JXR160_REG_YEAR - JXR160_REG_SECOND + 1)
#define NUM_ALARM_REGS			(JXR160_REG_ALM_DAY - JXR160_REG_ALM_MINUTE + 1)

#define	SEC_MASK			0x7F
#define	MIN_MASK			0x7F
#define	HOUR_MASK			0x3F
#define	DAY_MASK			0x3F
#define	MON_MASK			0x1F
#define MON_DEVIATION			1	/* the month representation in the kernel is 1 less than the actual value */
#define YEAR_DEVIATION			100	/* the year in the kernel represents starting from 1900, which is 100 less than the actual value */

struct jxr160 {
	struct rtc_device	*rtc;
	struct regmap		*regmap;
};

static int jxr160_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char rtc_data[NUM_TIME_REGS] = {0};
	struct i2c_client *client = to_i2c_client(dev);
	struct jxr160 *chip = i2c_get_clientdata(client);
	int ret;

	ret = regmap_bulk_read(chip->regmap, JXR160_REG_SECOND, rtc_data,
			       NUM_TIME_REGS);
	if (ret < 0) {
		dev_err(dev, "use I2C reading time from RTC failed with err:%d\n", ret);
		return ret;
	}

	/* Date and time data are stored in registers in BCD format, and conversion is required when reading them */
	tm->tm_sec  = bcd2bin(rtc_data[JXR160_REG_SECOND] & SEC_MASK);
	tm->tm_min  = bcd2bin(rtc_data[JXR160_REG_MINUTE] & MIN_MASK);
	tm->tm_hour = bcd2bin(rtc_data[JXR160_REG_HOUR] & HOUR_MASK);
	tm->tm_mday = bcd2bin(rtc_data[JXR160_REG_DAY] & DAY_MASK);
	tm->tm_mon  = bcd2bin(rtc_data[JXR160_REG_MONTH] & MON_MASK) - MON_DEVIATION;
	tm->tm_year = bcd2bin(rtc_data[JXR160_REG_YEAR]) + YEAR_DEVIATION;

	return 0;
}

static int jxr160_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char rtc_data[NUM_TIME_REGS];
	struct i2c_client *client = to_i2c_client(dev);
	struct jxr160 *chip = i2c_get_clientdata(client);
	int ret;

	if ((tm->tm_year < 100) || (tm->tm_year > 199)) {
		dev_err(dev, "the actual setting year should be between 2000 and 2099!\n");
		return -EINVAL;
	}

	rtc_data[JXR160_REG_SECOND] = bin2bcd(tm->tm_sec);
	rtc_data[JXR160_REG_MINUTE] = bin2bcd(tm->tm_min);
	rtc_data[JXR160_REG_HOUR]   = bin2bcd(tm->tm_hour);
	rtc_data[JXR160_REG_WEEK]  = 1 << (tm->tm_wday);
	rtc_data[JXR160_REG_DAY]   = bin2bcd(tm->tm_mday);
	rtc_data[JXR160_REG_MONTH] = bin2bcd(tm->tm_mon + MON_DEVIATION);
	rtc_data[JXR160_REG_YEAR]  = bin2bcd(tm->tm_year - YEAR_DEVIATION);

	ret = regmap_bulk_write(chip->regmap, JXR160_REG_SECOND, rtc_data,
				NUM_TIME_REGS);
	if (ret < 0) {
		dev_err(dev, "use I2C writing to RTC failed with err:%d\n", ret);
		return ret;
	}

	return 0;
}

static int jxr160_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned char rtc_data[NUM_ALARM_REGS] = {0};
	struct i2c_client *client = to_i2c_client(dev);
	struct jxr160 *chip = i2c_get_clientdata(client);
	int ret;

	ret = regmap_bulk_read(chip->regmap, JXR160_REG_ALM_MINUTE, rtc_data,
			       NUM_ALARM_REGS);
	if (ret < 0) {
		dev_err(dev, "use I2C reading alarm from RTC failed with err:%d\n", ret);
		return ret;
	}

	alm->time.tm_min  = bcd2bin(rtc_data[JXR160_REG_ALM_MINUTE - JXR160_REG_ALM_MINUTE] & MIN_MASK);
	alm->time.tm_hour = bcd2bin(rtc_data[JXR160_REG_ALM_HOUR - JXR160_REG_ALM_MINUTE] & HOUR_MASK);
	alm->time.tm_mday = bcd2bin(rtc_data[JXR160_REG_ALM_DAY - JXR160_REG_ALM_MINUTE] & DAY_MASK);

	/* check whether alarm interruption is allowed */
	if (((rtc_data[JXR160_REG_ALM_MINUTE - JXR160_REG_ALM_MINUTE] & (0x80)) != 0) || (rtc_data[JXR160_REG_ALM_HOUR - JXR160_REG_ALM_MINUTE] & (0x80)) != 0 || (rtc_data[JXR160_REG_ALM_DAY - JXR160_REG_ALM_MINUTE] & (0x80)) != 0)
		alm->enabled = 1;

	/* check if there is an alarm interrupt */
	ret = regmap_bulk_read(chip->regmap, JXR160_REG_FLAG, rtc_data, 1);
	if (ret < 0) {
		dev_err(dev, "use I2C reading JXR160_REG_FLAG from RTC failed with err:%d\n", ret);
		return ret;
	}
	if ((rtc_data[0] & JXR160_AF) != 0)
		alm->pending = 1;
	else
		alm->pending = 0;

	return 0;
}

static int jxr160_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned char rtc_data[NUM_ALARM_REGS];
	struct i2c_client *client = to_i2c_client(dev);
	struct jxr160 *chip = i2c_get_clientdata(client);
	int ret;

	rtc_data[JXR160_REG_ALM_MINUTE - JXR160_REG_ALM_MINUTE] = bin2bcd(alm->time.tm_min);
	rtc_data[JXR160_REG_ALM_HOUR - JXR160_REG_ALM_MINUTE] = bin2bcd(alm->time.tm_hour);
	rtc_data[JXR160_REG_ALM_DAY - JXR160_REG_ALM_MINUTE] = bin2bcd(alm->time.tm_mday);

	if (alm->enabled == 1) {
		ret = regmap_bulk_write(chip->regmap, JXR160_REG_ALM_MINUTE, rtc_data, NUM_ALARM_REGS);
		if (ret < 0) {
			dev_err(dev, "use I2C writing alarm from RTC failed with err:%d\n", ret);
			return ret;
		}
	}

	/* set to daily alarm mode */
	ret = regmap_bulk_read(chip->regmap, JXR160_REG_EXTENSION, rtc_data, 1);
	rtc_data[0] &= JXR160_WADA;
	ret = regmap_bulk_write(chip->regmap, JXR160_REG_EXTENSION, rtc_data, 1);

	/* set enable interrupt, set alarm interrupt output, set single event alarm */
	ret = regmap_bulk_read(chip->regmap, JXR160_REG_CONTROL, rtc_data, 1);
	rtc_data[0] &= JXR160_AIE;
	ret = regmap_bulk_write(chip->regmap, JXR160_REG_CONTROL, rtc_data, 1);

	return 0;
}

static irqreturn_t rtc_handle_irq(int irq, void *dev_id)
{
	struct i2c_client *client = dev_id;
	struct jxr160 *chip = i2c_get_clientdata(client);
	unsigned char rtc_data[1];

	/* clear rtc interrupt flag bit */
	regmap_bulk_read(chip->regmap, JXR160_REG_FLAG, rtc_data, 1);
	rtc_data[0] &= ~JXR160_AF;
	regmap_bulk_write(chip->regmap, JXR160_REG_FLAG, rtc_data, 1);

	/* update rtc interrupt status */
	rtc_update_irq(chip->rtc, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}

static const struct rtc_class_ops jxr160_rtc_ops = {
	.read_time	= jxr160_rtc_read_time,
	.set_time	= jxr160_rtc_set_time,
	.read_alarm     = jxr160_rtc_read_alarm,
	.set_alarm      = jxr160_rtc_set_alarm,
};

/*
 * Configure parameters and attributes for regmap
 * reg_bits: register address bits
 * val_bits: register value bits
 * max_register: register mapping maximum address
*/
static const struct regmap_config regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0x11,
};

static int jxr160_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	struct jxr160 *chip;
	struct gpio_desc *gpiod;
	struct device_node *np = client->dev.of_node;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2c is not available or there are issues with its functionality\n");
		return -ENODEV;
	}

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&client->dev, "device memory request failed\n");
		return -ENOMEM;
	}

	chip->regmap = devm_regmap_init_i2c(client, &regmap_config);
	if (IS_ERR(chip->regmap)) {
		dev_err(&client->dev, "I2C initialization regmap failed\n");
		return PTR_ERR(chip->regmap);
	}

	i2c_set_clientdata(client, chip);

	chip->rtc = devm_rtc_allocate_device(&client->dev);
	if (IS_ERR(chip->rtc)) {
		dev_err(&client->dev, "alloc rtc device failed\n");
		return PTR_ERR(chip->rtc);
	}

	chip->rtc->ops = &jxr160_rtc_ops;
	chip->rtc->range_min = RTC_TIMESTAMP_BEGIN_2000;
	chip->rtc->range_max = RTC_TIMESTAMP_END_2099;

	ret = devm_rtc_register_device(chip->rtc);
	if (ret) {
		dev_err(&client->dev, "rtc device registration failed\n");
		return ret;
	}

	gpiod = devm_gpiod_get(&client->dev, "irq", GPIOD_IN);
	client->irq = gpiod_to_irq(gpiod);
	if (client->irq < 0) {
		dev_err(&client->dev, "interrupt number conversion failed\n");
		return client->irq;
	}

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(&client->dev, client->irq, NULL, rtc_handle_irq,
						IRQF_TRIGGER_LOW | IRQF_ONESHOT,
						"jxr160", client);
		if (ret)
			dev_err(&client->dev, "unable to request IQR, client->irq = %d\n", client->irq);
	}

	if (of_property_read_bool(np, "wakeup-source")) {
		device_init_wakeup(&client->dev, true);
		dev_pm_set_wake_irq(&client->dev, client->irq);
	}

	return 0;
}

static const struct i2c_device_id jxr160_id[] = {
	{"jxr160", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, jxr160_id);

static const __maybe_unused struct of_device_id rtc_dt_ids[] = {
	{ .compatible = "juxuan,jxr160-rtc" },
	{},
};
MODULE_DEVICE_TABLE(of, rtc_dt_ids);

static struct i2c_driver jxr160_driver = {
	.driver     = {
		.name   = "jxr160",
		.of_match_table = of_match_ptr(rtc_dt_ids),
	},
	.probe      = jxr160_probe,
	.id_table   = jxr160_id,
};

module_i2c_driver(jxr160_driver);

MODULE_AUTHOR("Dang Hao <danghao@allwinnertech.com>");
MODULE_DESCRIPTION("JXR160 RTC driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
