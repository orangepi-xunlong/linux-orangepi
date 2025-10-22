/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's GPIO based Serial Interface
 *
 * Copyright (c) 2023, Martin <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <sunxi-common.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/serial.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <sunxi-ioserial.h>

#define BAUDRATE		115200  /* Default: 115200 8N1 */
#define BIT_DURATION		(1 * 1000 * 1000 / BAUDRATE)
#define N_START_BIT		1
#define N_DATA_BIT		8
#define N_PARITY_BIT		0
#define N_STOP_BIT		1
#define N_PACKET_BIT		(N_START_BIT + N_DATA_BIT + N_PARITY_BIT + N_STOP_BIT)
#define TX_BUF_SIZE		512

#define ODD_PARITY		0  /* 1: odd_parity, 0: even_parity */

#define IS_ODD(x)		((x % 2) == 1)

static bool ioserial_probed;

static struct miscdevice mdev;
static struct gpio_desc *tx_gpio;
static char *ioserial_cmdline_args;
static spinlock_t lock;
static char *tx_buf;

/*
 * Please pass in the GPIO serial number directly,
 * and the calculation method is as follows:
 *
 * eg:
 * gpio = base * 32 + offset
 * PB11 = 1 * 32 + 11 = 43
 *
 */
static int __init ioserial_setup(char *s)
{
	ioserial_cmdline_args = s;
	return 1;
}
__setup("ioserial=", ioserial_setup);

static bool byte_bit_is_odd(char byte)
{
	int i, count = 0;
	bool is_odd = false;

	for (i = 0; i < N_DATA_BIT; i++) {
		if (byte & BIT(i))
			count++;
	}

	if (IS_ODD(count))
		is_odd = true;

	return is_odd;
}

static bool cal_parity(char byte, bool is_odd_parity)
{
	bool parity, is_odd;

	is_odd = byte_bit_is_odd(byte);

	if (is_odd)
		parity = !is_odd_parity;
	else
		parity = is_odd_parity;

	return parity;
}

static void ioserial_send_byte(char byte)
{
	int i;
	bool bit, parity;

	u16 data = (1 << (N_START_BIT + N_DATA_BIT + N_PARITY_BIT))	/* STOP_BIT */
		 | ((u16)byte << N_START_BIT)				/* DATA_BIT */
		 | (0 << 0);						/* START_BIT */

	if (N_PARITY_BIT)  {
		parity = cal_parity(byte, ODD_PARITY);
		data |= (parity << (N_START_BIT + N_DATA_BIT));
	}

	for (i = 0; i < N_PACKET_BIT; i++) {
		bit = (bool)(data & BIT(i));
		gpiod_set_value(tx_gpio, bit);
		udelay(BIT_DURATION);
	}
}

static int __ioserial_puts(const char *str)
{
	int i = 0;

	while (str[i]) {
		if (str[i] == '\n')
			ioserial_send_byte('\r');
		ioserial_send_byte(str[i]);
		i++;
	}

	return i;
}

/* For Kernel-Space Usage */
int ioserial_printk(const char *fmt, ...)
{
	va_list args;
	int len;
	unsigned long flags;

	if (!ioserial_probed)
		return 0;

	spin_lock_irqsave(&lock, flags);

	va_start(args, fmt);
	len = vsnprintf(tx_buf, TX_BUF_SIZE, fmt, args);
	va_end(args);

	len = __ioserial_puts(tx_buf);

	spin_unlock_irqrestore(&lock, flags);
	return len;
}
EXPORT_SYMBOL(ioserial_printk);

/* For User-Space Usage */
static ssize_t sunxi_ioserial_write(struct file *file, const char __user *buf,
					size_t count, loff_t *ppos)
{
	unsigned long flags;
	u32 len;

	spin_lock_irqsave(&lock, flags);

	len = min_t(u32, TX_BUF_SIZE - 1, (u32)count);
	if (copy_from_user(tx_buf, buf, len)) {
		pr_err("copy from user failed\n");
		return -EFAULT;
	}
	tx_buf[len] = '\0';
	len = __ioserial_puts(tx_buf);

	spin_unlock_irqrestore(&lock, flags);

	return len;
}

static int sunxi_ioserial_open(struct inode *inode, struct file *file)
{
	gpiod_direction_output(tx_gpio, 1);

	return 0;
}

struct file_operations sunxi_ioserial_fops = {
	.owner = THIS_MODULE,
	.open = sunxi_ioserial_open,
	.write = sunxi_ioserial_write,
};

static int sunxi_ioserial_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	u32 gpio_num;

	tx_buf = devm_kzalloc(dev, sizeof(char) * TX_BUF_SIZE, GFP_KERNEL);
	if (!tx_buf)
		return -ENOMEM;

	if (ioserial_cmdline_args) {
		dev_info(dev, "ioserial get gpio from cmdline '%s'\n", ioserial_cmdline_args);
		ret = kstrtou32(ioserial_cmdline_args, 0, &gpio_num);
		if (ret)
			return ret;

		ret = gpio_request(gpio_num, NULL);
		if (ret < 0) {
			dev_err(dev, "ioserial request gpio from cmdline failed\n");
			return ret;
		}

		tx_gpio = gpio_to_desc(gpio_num);
	} else {
		dev_info(dev, "ioserial get gpio from dts\n");
		tx_gpio = devm_gpiod_get(dev, "tx", GPIOD_OUT_HIGH);
		if (IS_ERR(tx_gpio)) {
			dev_err(dev, "failed to get tx-gpios");
			return PTR_ERR(tx_gpio);
		}
	}

	gpiod_direction_output(tx_gpio, 1);

	spin_lock_init(&lock);

	mdev.parent = dev;
	mdev.minor = MISC_DYNAMIC_MINOR;
	mdev.name = "ioserial";
	mdev.fops = &sunxi_ioserial_fops;

	ret = misc_register(&mdev);
	if (ret) {
		dev_err(dev, "failed to register misc device");
		return ret;
	}

	ioserial_probed = true;
	return 0;
}

static int sunxi_ioserial_remove(struct platform_device *pdev)
{
	misc_deregister(&mdev);

	if (tx_gpio)
		gpiod_put(tx_gpio);

	return 0;
}

static const struct of_device_id sunxi_ioserial_match[] = {
	{ .compatible = "allwinner,ioserial-100", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_ioserial_match);

static struct platform_driver sunxi_ioserial_driver = {
	.probe  = sunxi_ioserial_probe,
	.remove = sunxi_ioserial_remove,
	.driver = {
		.name  = "sunxi-ioserial",
		.of_match_table = sunxi_ioserial_match,
	},
};
module_platform_driver(sunxi_ioserial_driver);  /* TODO: make it early_init or core_init ... */

MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner's GPIO based Serial Interface");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.2.0");
