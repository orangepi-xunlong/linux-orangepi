// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Real Time Clock (RTC) Driver for sd2059
 * Copyright (C) 2022 shenzhen wave
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

/* time reg */
#define SD2059_REG_SC			0x00
#define SD2059_REG_MN			0x01
#define SD2059_REG_HR			0x02
#define SD2059_REG_DW			0x03
#define SD2059_REG_DM			0x04
#define SD2059_REG_MO			0x05
#define SD2059_REG_YR			0x06

/* alarm reg */
#define SD2059_REG_ALARM_SC		0x07
#define SD2059_REG_ALARM_MN		0x08
#define SD2059_REG_ALARM_HR		0x09
#define SD2059_REG_ALARM_DW		0x0A
#define SD2059_REG_ALARM_DM		0x0B
#define SD2059_REG_ALARM_MO		0x0C
#define SD2059_REG_ALARM_YR		0x0D
#define SD2059_ALARM_ALLOED		0x0E

/* control reg */
#define SD2059_REG_CTRL1		0x0f
#define SD2059_REG_CTRL2		0x10
#define SD2059_REG_CTRL3		0x11

#define SD2059_INT_AF			BIT(5)

#define SD2059_INTAE			BIT(1)
#define SD2059_INTS0			BIT(4)
#define SD2059_INTS1			BIT(5)
#define SD2059_IM			BIT(6)

#define KEY_WRITE1			0x80
#define KEY_WRITE2			0x04
#define KEY_WRITE3			0x80

#define NUM_TIME_REGS	(SD2059_REG_YR - SD2059_REG_SC + 1)
#define NUM_ALARM_REGS	(SD2059_ALARM_ALLOED - SD2059_REG_ALARM_SC + 1)

#define SEC_MASK			0x7F	/* second register mask */
#define MIN_MASK			0x7F	/* minute register mask */
#define HOUR_24H_MASK			0x3F	/* register mask under 24-hour system */
#define HOUR_12H_MASK			0x1F	/* register Mask under 12-Hour System */
#define HOUR_12H_DEVIATION		12	/* in the 12 hour system, the afternoon time should be added with 12 */
#define WEEK_MASK			0x07	/* week register mask */
#define DAY_MASK			0x3F	/* day register mask */
#define MON_MASK			0X1F	/* month register mask */
#define MON_DEVIATION			1	/* the month representation in the kernel is 1 less than the actual value */
#define YEAR_DEVIATION			100	/* the year in the kernel represents starting from 1900, which is 100 less than the actual value */

/*
 * The sd2059 has write protection
 * and we can choose whether or not to use it.
 * Write protection is turned off by default.
 */
#define WRITE_PROTECT_EN	0

struct sd2059 {
	struct rtc_device	*rtc;
	struct regmap		*regmap;
};

/*
 * In order to prevent arbitrary modification of the time register,
 * when modification of the register,
 * the "write" bit needs to be written in a certain order.
 * 1. set WRITE1 bit
 * 2. set WRITE2 bit
 * 3. set WRITE3 bit
 */
static void sd2059_enable_reg_write(struct sd2059 *sd2059)
{
	regmap_update_bits(sd2059->regmap, SD2059_REG_CTRL2,
			   KEY_WRITE1, KEY_WRITE1);
	regmap_update_bits(sd2059->regmap, SD2059_REG_CTRL1,
			   KEY_WRITE2, KEY_WRITE2);
	regmap_update_bits(sd2059->regmap, SD2059_REG_CTRL1,
			   KEY_WRITE3, KEY_WRITE3);
}

#if WRITE_PROTECT_EN
/*
 * In order to prevent arbitrary modification of the time register,
 * we should disable the write function.
 * when disable write,
 * the "write" bit needs to be clear in a certain order.
 * 1. clear WRITE2 bit
 * 2. clear WRITE3 bit
 * 3. clear WRITE1 bit
 */
static void sd2059_disable_reg_write(struct sd2059 *sd2059)
{
	regmap_update_bits(sd2059->regmap, SD2059_REG_CTRL1,
			   KEY_WRITE2, 0);
	regmap_update_bits(sd2059->regmap, SD2059_REG_CTRL1,
			   KEY_WRITE3, 0);
	regmap_update_bits(sd2059->regmap, SD2059_REG_CTRL2,
			   KEY_WRITE1, 0);
}
#endif

static int sd2059_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char hour;
	unsigned char rtc_data[NUM_TIME_REGS] = {0};
	struct i2c_client *client = to_i2c_client(dev);
	struct sd2059 *sd2059 = i2c_get_clientdata(client);
	int ret;

	ret = regmap_bulk_read(sd2059->regmap, SD2059_REG_SC, rtc_data,
			       NUM_TIME_REGS);
	if (ret < 0) {
		dev_err(dev, "use I2C reading time from RTC failed with err:%d\n", ret);
		return ret;
	}

	/* Date and time data are stored in registers in BCD format, and conversion is required when reading them */
	tm->tm_sec	= bcd2bin(rtc_data[SD2059_REG_SC] & SEC_MASK);
	tm->tm_min	= bcd2bin(rtc_data[SD2059_REG_MN] & MIN_MASK);

	/*
	 * The sd2059 supports 12/24 hour mode.
	 * When getting time,
	 * we need to convert the 12 hour mode to the 24 hour mode.
	 */
	hour = rtc_data[SD2059_REG_HR];
	if (hour & 0x80) /* 24H MODE */
		tm->tm_hour = bcd2bin(rtc_data[SD2059_REG_HR] & HOUR_24H_MASK);
	else if (hour & 0x20) /* 12H MODE PM */
		tm->tm_hour = bcd2bin(rtc_data[SD2059_REG_HR] & HOUR_12H_MASK) + HOUR_12H_DEVIATION;
	else /* 12H MODE AM */
		tm->tm_hour = bcd2bin(rtc_data[SD2059_REG_HR] & HOUR_12H_MASK);

	tm->tm_wday = rtc_data[SD2059_REG_DW] & WEEK_MASK;
	tm->tm_mday = bcd2bin(rtc_data[SD2059_REG_DM] & DAY_MASK);
	tm->tm_mon  = bcd2bin(rtc_data[SD2059_REG_MO] & MON_MASK) - MON_DEVIATION;
	tm->tm_year = bcd2bin(rtc_data[SD2059_REG_YR]) + YEAR_DEVIATION;

	return 0;
}

static int sd2059_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char rtc_data[NUM_TIME_REGS];
	struct i2c_client *client = to_i2c_client(dev);
	struct sd2059 *sd2059 = i2c_get_clientdata(client);
	int ret;

	rtc_data[SD2059_REG_SC] = bin2bcd(tm->tm_sec);
	rtc_data[SD2059_REG_MN] = bin2bcd(tm->tm_min);
	rtc_data[SD2059_REG_HR] = bin2bcd(tm->tm_hour) | 0x80;
	rtc_data[SD2059_REG_DW] = tm->tm_wday & WEEK_MASK;
	rtc_data[SD2059_REG_DM] = bin2bcd(tm->tm_mday);
	rtc_data[SD2059_REG_MO] = bin2bcd(tm->tm_mon + MON_DEVIATION);
	rtc_data[SD2059_REG_YR] = bin2bcd(tm->tm_year - YEAR_DEVIATION);

#if WRITE_PROTECT_EN
	sd2059_enable_reg_write(sd2059);
#endif

	ret = regmap_bulk_write(sd2059->regmap, SD2059_REG_SC, rtc_data,
				NUM_TIME_REGS);
	if (ret < 0) {
		dev_err(dev, "use I2C writing to RTC failed with err:%d\n", ret);
		return ret;
	}

#if WRITE_PROTECT_EN
	sd2059_disable_reg_write(sd2059);
#endif

	return 0;
}

static int sd2059_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned char rtc_data[NUM_ALARM_REGS] = {0};
	struct i2c_client *client = to_i2c_client(dev);
	struct sd2059 *sd2059 = i2c_get_clientdata(client);
	int ret;

	ret = regmap_bulk_read(sd2059->regmap, SD2059_REG_ALARM_SC, rtc_data,
			       NUM_ALARM_REGS);
	if (ret < 0) {
		dev_err(dev, "use I2C reading alarm from RTC failed with err:%d\n", ret);
		return ret;
	}

	alm->time.tm_sec  = bcd2bin(rtc_data[SD2059_REG_ALARM_SC - SD2059_REG_ALARM_SC]);
	alm->time.tm_min  = bcd2bin(rtc_data[SD2059_REG_ALARM_MN - SD2059_REG_ALARM_SC]);
	alm->time.tm_hour = bcd2bin(rtc_data[SD2059_REG_ALARM_HR - SD2059_REG_ALARM_SC]);
	alm->time.tm_mday = bcd2bin(rtc_data[SD2059_REG_ALARM_DW - SD2059_REG_ALARM_SC]);
	alm->time.tm_wday = bcd2bin(rtc_data[SD2059_REG_ALARM_DM - SD2059_REG_ALARM_SC]);
	alm->time.tm_mon  = bcd2bin(rtc_data[SD2059_REG_ALARM_MO - SD2059_REG_ALARM_SC]);
	alm->time.tm_year = bcd2bin(rtc_data[SD2059_REG_ALARM_YR - SD2059_REG_ALARM_SC]);

	/* check whether alarm interruption is allowed */
	if ((rtc_data[SD2059_ALARM_ALLOED - SD2059_REG_ALARM_SC] & 0x7f) != 0)	/* SD2059_ALARM_ALLOED register are only valid for the lower seven bits */
		alm->enabled = 1;

	/* check if there is an alarm interrupt */
	ret = regmap_bulk_read(sd2059->regmap, SD2059_REG_CTRL1, rtc_data, 1);
	if (ret < 0) {
		dev_err(dev, "use I2C reading SD2059_REG_CTRL1 from RTC failed with err:%d\n", ret);
		return ret;
	}
	if ((rtc_data[0] & SD2059_INT_AF) != 0)
		alm->pending = 1;
	else
		alm->pending = 0;

	return 0;
}

static int sd2059_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned char rtc_data[NUM_ALARM_REGS];
	struct i2c_client *client = to_i2c_client(dev);
	struct sd2059 *sd2059 = i2c_get_clientdata(client);
	int ret;

	rtc_data[SD2059_REG_ALARM_SC - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_sec);
	rtc_data[SD2059_REG_ALARM_MN - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_min);
	rtc_data[SD2059_REG_ALARM_HR - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_hour);
	rtc_data[SD2059_REG_ALARM_DW - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_wday);
	rtc_data[SD2059_REG_ALARM_DM - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_mday);
	rtc_data[SD2059_REG_ALARM_MO - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_mon + MON_DEVIATION);
	rtc_data[SD2059_REG_ALARM_YR - SD2059_REG_ALARM_SC] = bin2bcd(alm->time.tm_year - YEAR_DEVIATION);

	if (alm->enabled == 1)
		rtc_data[SD2059_ALARM_ALLOED - SD2059_REG_ALARM_SC] = 0x7f;	/* SD2059_ALARM_ALLOED register are only valid for the lower seven bits */
	else
		rtc_data[SD2059_ALARM_ALLOED - SD2059_REG_ALARM_SC] = 0;

#if WRITE_PROTECT_EN
	sd2059_enable_reg_write(sd2059);
#endif

	ret = regmap_bulk_write(sd2059->regmap, SD2059_REG_ALARM_SC, rtc_data,
				NUM_ALARM_REGS);
	if (ret < 0) {
		dev_err(dev, "writing alarm from RTC failed with err:%d\n", ret);
		return ret;
	}

	ret = regmap_bulk_read(sd2059->regmap, SD2059_REG_CTRL2, rtc_data, 1);
	rtc_data[0] &= ~SD2059_INTS1;
	rtc_data[0] |= SD2059_INTS0;
	rtc_data[0] |= SD2059_INTAE;
	rtc_data[0] &= ~SD2059_IM;

	/* set enable interrupt, set alarm interrupt output, set single event alarm */
	ret = regmap_bulk_write(sd2059->regmap, SD2059_REG_CTRL2, rtc_data, 1);
	if (ret < 0) {
		dev_err(dev, "writing alarm from RTC failed with err:%d\n", ret);
		return ret;
	}

#if WRITE_PROTECT_EN
	sd2059_disable_reg_write(sd2059);
#endif

	return 0;
}

static irqreturn_t rtc_handle_irq(int irq, void *dev_id)
{
	struct i2c_client *client = dev_id;
	struct sd2059 *sd2059 = i2c_get_clientdata(client);
	unsigned char rtc_data[1];

	/* clear rtc interrupt flag bit */
	regmap_bulk_read(sd2059->regmap, SD2059_REG_CTRL1, rtc_data, 1);
	rtc_data[0] &= ~SD2059_INT_AF;
	regmap_bulk_write(sd2059->regmap, SD2059_REG_CTRL1, rtc_data, 1);

	rtc_update_irq(sd2059->rtc, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}

static const struct rtc_class_ops sd2059_rtc_ops = {
	.read_time	= sd2059_rtc_read_time,
	.set_time	= sd2059_rtc_set_time,
	.read_alarm     = sd2059_rtc_read_alarm,
	.set_alarm      = sd2059_rtc_set_alarm,
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

static int sd2059_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	struct sd2059 *sd2059;
	struct gpio_desc *gpiod;
	struct device_node *np = client->dev.of_node;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2c is not available or there are issues with its functionality\n");
		return -ENODEV;
	}

	sd2059 = devm_kzalloc(&client->dev, sizeof(*sd2059), GFP_KERNEL);
	if (!sd2059) {
		dev_err(&client->dev, "device memory request failed\n");
		return -ENOMEM;
	}

	sd2059->regmap = devm_regmap_init_i2c(client, &regmap_config);
	if (IS_ERR(sd2059->regmap)) {
		dev_err(&client->dev, "I2C initialization regmap failed\n");
		return PTR_ERR(sd2059->regmap);
	}

	i2c_set_clientdata(client, sd2059);

	sd2059->rtc = devm_rtc_allocate_device(&client->dev);
	if (IS_ERR(sd2059->rtc)) {
		dev_err(&client->dev, "alloc rtc device failed\n");
		return PTR_ERR(sd2059->rtc);
	}

	sd2059->rtc->ops = &sd2059_rtc_ops;
	sd2059->rtc->range_min = RTC_TIMESTAMP_BEGIN_2000;
	sd2059->rtc->range_max = RTC_TIMESTAMP_END_2099;

	ret = devm_rtc_register_device(sd2059->rtc);
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
						"sd2059", client);
		if (ret)
			dev_err(&client->dev, "unable to request IQR, client->irq = %d\n", client->irq);
	}

	if (of_property_read_bool(np, "wakeup-source")) {
		device_init_wakeup(&client->dev, true);
		dev_pm_set_wake_irq(&client->dev, client->irq);
	}

	sd2059_enable_reg_write(sd2059);

	return 0;
}

static const struct i2c_device_id sd2059_id[] = {
	{"sd2059", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, sd2059_id);

static const __maybe_unused struct of_device_id rtc_dt_ids[] = {
	{ .compatible = "whwave,sd2059" },
	{},
};
MODULE_DEVICE_TABLE(of, rtc_dt_ids);

static struct i2c_driver sd2059_driver = {
	.driver     = {
		.name   = "sd2059",
		.of_match_table = of_match_ptr(rtc_dt_ids),
	},
	.probe      = sd2059_probe,
	.id_table   = sd2059_id,
};

module_i2c_driver(sd2059_driver);

MODULE_AUTHOR("Dang Hao <danghao@allwinnertech.com>");
MODULE_DESCRIPTION("SD2059 RTC driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
