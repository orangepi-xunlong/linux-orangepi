/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner Soft JTAG Master driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#define SOFT_JTAG_VERSION	"0.0.1"
#define SCAN_IR			_IOWR('X', 1, unsigned int)
#define SCAN_DR			_IOWR('X', 2, unsigned int)
#define RESET			_IOWR('X', 3, unsigned int)

#define SET_TCK(chip, i)	gpiod_set_value(chip->tck, i)
#define SET_TMS(chip, i)	gpiod_set_value(chip->tms, i)
#define SET_TDI(chip, i)	gpiod_set_value(chip->tdi, i)
#define GET_TDO(chip)		gpiod_get_value(chip->tdo)

enum tap_state_machine {
	TAP_RESET = 0,
	TAP_IDLE,
	TAP_DRSELECT,
	TAP_DRCAPTURE,
	TAP_DRSHIFT,
	TAP_DREXIT1,
	TAP_DRPAUSE,
	TAP_DREXIT2,
	TAP_DRUPDATE,
	TAP_IRSELECT,
	TAP_IRCAPTURE,
	TAP_IRSHIFT,
	TAP_IREXIT1,
	TAP_IRPAUSE,
	TAP_IREXIT2,
	TAP_IRUPDATE,
	TAP_UNKNOWN
};

typedef union {
	struct {
		u64 lo4 : 4;
		u64 hi58 : 58;
	} parts;

	u64 all;
} inst_data_u;

typedef union {
	struct {
		u64 lo32 : 32;
		u64 hi32 : 32;
	} parts;

	u64 all;
} tdi_data_u;

typedef union {
	struct {
		u64 lo32 : 32;
		u64 hi32 : 32;
	} parts;

	u64 all;
} tdo_data_u;

struct jtag_user_data {
	u64 data;
	u32 len;
};

struct sunxi_soft_jtag {
	struct device *dev;

	/* only for soft jtag master */
	struct gpio_desc *tck;
	struct gpio_desc *tms;
	struct gpio_desc *tdi;	/* jtag master output, slave input */
	struct gpio_desc *tdo;	/* jtag master input, slave output */

	struct miscdevice mdev;

	u32 delay;
	u32 len;
	tdi_data_u tdi_data;
	tdo_data_u tdo_data;
	inst_data_u inst_data;
};

void sunxi_soft_jtag_tap_move_onecycle(struct sunxi_soft_jtag *chip, bool tms_state)
{
	SET_TMS(chip, tms_state);

	SET_TCK(chip, 0);
	udelay(chip->delay);
	SET_TCK(chip, 1);
	udelay(chip->delay);
}

static void sunxi_soft_jtag_tap_state_move(struct sunxi_soft_jtag *chip,
		enum tap_state_machine tap_from, enum tap_state_machine tap_to)
{
	int i;

	if (tap_from == TAP_UNKNOWN && tap_to == TAP_IDLE) {
		for (i = 0; i < 8; i++)
			sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		return;
	}

	if (tap_from == TAP_IDLE && tap_to == TAP_IDLE) {
		for (i = 0; i < 3; i++)
			sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		return;
	}

	if (tap_from == TAP_IDLE && tap_to == TAP_IRSHIFT) {
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		return;
	}

	if (tap_from == TAP_IDLE && tap_to == TAP_DRSHIFT) {
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		return;
	}

	if (tap_from == TAP_IRSHIFT && tap_to == TAP_IREXIT1) {
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		return;
	}

	if (tap_from == TAP_DRSHIFT && tap_to == TAP_DREXIT1) {
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		return;
	}

	if (tap_from == TAP_IREXIT1 && tap_to == TAP_IDLE) {
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		return;
	}

	if (tap_from == TAP_DREXIT1 && tap_to == TAP_IDLE) {
		sunxi_soft_jtag_tap_move_onecycle(chip, 1);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		sunxi_soft_jtag_tap_move_onecycle(chip, 0);
		return;
	}

	dev_err(chip->dev, "Error: unsupported tap state machine change from %d to %d\n",
			tap_from, tap_to);
}

static void sunxi_soft_jtag_reset(struct sunxi_soft_jtag *chip)
{
	sunxi_soft_jtag_tap_state_move(chip, TAP_UNKNOWN, TAP_IDLE);
}

static void sunxi_soft_jtag_write(struct sunxi_soft_jtag *chip, u64 din, u32 len)
{
	int i, tmp;

	SET_TMS(chip, 0);

	for (i = 0; i < len; i++) {
		SET_TMS(chip, (i == (len - 1)));

		tmp = din >> i;
		SET_TDI(chip, (tmp & BIT(0)));

		SET_TCK(chip, 0);
		udelay(chip->delay);
		SET_TCK(chip, 1);
		udelay(chip->delay);

		chip->tdo_data.all <<= 1;
		chip->tdo_data.all |= (GET_TDO(chip) & BIT(0));
	}
}

static void sunxi_soft_jtag_scan_ir(struct sunxi_soft_jtag *chip)
{
	sunxi_soft_jtag_tap_state_move(chip, TAP_IDLE, TAP_IRSHIFT);
	sunxi_soft_jtag_write(chip, chip->inst_data.all, chip->len);
	sunxi_soft_jtag_tap_state_move(chip, TAP_IREXIT1, TAP_IDLE);
	chip->len = 0;
	chip->inst_data.all = 0;
	chip->tdo_data.all = 0;
}

static void sunxi_soft_jtag_scan_dr(struct sunxi_soft_jtag *chip)
{
	sunxi_soft_jtag_tap_state_move(chip, TAP_IDLE, TAP_DRSHIFT);
	sunxi_soft_jtag_write(chip, chip->tdi_data.all, chip->len);
	sunxi_soft_jtag_tap_state_move(chip, TAP_DREXIT1, TAP_IDLE);
	chip->len = 0;
	chip->tdi_data.all = 0;
}

static int sunxi_soft_jtag_open(struct inode *inode, struct file *file)
{
	struct miscdevice *mdev = file->private_data;
	struct sunxi_soft_jtag *chip = container_of(mdev, struct sunxi_soft_jtag, mdev);

	file->private_data = chip;

	return 0;
}

static long sunxi_soft_jtag_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sunxi_soft_jtag *chip = file->private_data;
	struct jtag_user_data user_data;
	int ret = 0;

	switch (cmd) {
	case SCAN_IR:
		ret = copy_from_user(&user_data, (void __user *)arg, sizeof(user_data));
		if (ret) {
			dev_err(chip->dev, "inst copy from user err\n");
			return -EFAULT;
		}

		chip->inst_data.all = user_data.data;
		chip->len = user_data.len;
		sunxi_soft_jtag_scan_ir(chip);

		break;
	case SCAN_DR:
		ret = copy_from_user(&user_data, (void __user *)arg, sizeof(user_data));
		if (ret) {
			dev_err(chip->dev, "data copy from user err\n");
			return -EFAULT;
		}

		chip->tdo_data.all = 0;
		chip->tdi_data.all = user_data.data;
		chip->len = user_data.len;

		sunxi_soft_jtag_scan_dr(chip);
		user_data.data = (u64)chip->tdo_data.parts.lo32;
		ret = copy_to_user((void __user *)arg, &user_data, sizeof(user_data));
		if (ret) {
			dev_err(chip->dev, "tdo copy to user err\n");
			return -EFAULT;
		}
		break;
	case RESET:
		sunxi_soft_jtag_reset(chip);
		break;
	default:
		ret = -EFAULT;
		dev_err(chip->dev, "Unspported cmd\n");
		break;
	}

	return ret;
}

struct file_operations sunxi_soft_jtag_fops = {
	.owner = THIS_MODULE,
	.open = sunxi_soft_jtag_open,
	.unlocked_ioctl = sunxi_soft_jtag_ioctl,
};

static int sunxi_soft_jtag_resource_get(struct sunxi_soft_jtag *chip)
{
	int ret;

	chip->tck = devm_gpiod_get(chip->dev, "tck", GPIOD_OUT_LOW);
	if (IS_ERR(chip->tck)) {
		dev_err(chip->dev, "Error: request tck failed\n");
		return PTR_ERR(chip->tck);
	}

	chip->tms = devm_gpiod_get(chip->dev, "tms", GPIOD_OUT_LOW);
	if (IS_ERR(chip->tms)) {
		dev_err(chip->dev, "Error: request tms failed\n");
		return PTR_ERR(chip->tms);
	}

	chip->tdi = devm_gpiod_get(chip->dev, "tdi", GPIOD_OUT_LOW);
	if (IS_ERR(chip->tdi)) {
		dev_err(chip->dev, "Error: request tdi failed\n");
		return PTR_ERR(chip->tdi);
	}

	chip->tdo = devm_gpiod_get(chip->dev, "tdo", GPIOD_IN);
	if (IS_ERR(chip->tdo)) {
		dev_err(chip->dev, "Error: request tdo failed\n");
		return PTR_ERR(chip->tdo);
	}

	ret = of_property_read_u32(chip->dev->of_node, "delay-us", &chip->delay);
	if (ret) {
		dev_warn(chip->dev, "Warning: not find delay-us in dts, use default 20Khz\n");
		chip->delay = 50;
	}

	return 0;
}

static int sunxi_soft_jtag_class_init(struct sunxi_soft_jtag *chip)
{
	chip->mdev.parent	= chip->dev;
	chip->mdev.minor	= MISC_DYNAMIC_MINOR;
	chip->mdev.name		= "soft-jtag";
	chip->mdev.fops		= &sunxi_soft_jtag_fops;
	return misc_register(&chip->mdev);
}

static void sunxi_soft_jtag_class_exit(struct sunxi_soft_jtag *chip)
{
	misc_deregister(&chip->mdev);
}

static const struct of_device_id sunxi_soft_jtag_match[] = {
	{ .compatible = "allwinner,soft-jtag-master"},
	{ }
};
MODULE_DEVICE_TABLE(of, sunxi_soft_jtag_match);

static int sunxi_soft_jtag_probe(struct platform_device *pdev)
{
	struct sunxi_soft_jtag *chip;
	int ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	ret = sunxi_soft_jtag_resource_get(chip);
	if (ret)
		return ret;

	ret = sunxi_soft_jtag_class_init(chip);
	if (ret)
		return ret;

	dev_info(&pdev->dev, "sunxi soft jtag probe success\n");
	return 0;
}

static int sunxi_soft_jtag_remove(struct platform_device *pdev)
{
	struct sunxi_soft_jtag *chip = platform_get_drvdata(pdev);

	sunxi_soft_jtag_class_exit(chip);

	return 0;
}

static struct platform_driver sunxi_soft_jtag_driver = {
	.probe  = sunxi_soft_jtag_probe,
	.remove = sunxi_soft_jtag_remove,
	.driver = {
		.name           = "sunxi-soft-jtag",
		.of_match_table = sunxi_soft_jtag_match,
	},
};
module_platform_driver(sunxi_soft_jtag_driver);

MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner soft jtag master driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SOFT_JTAG_VERSION);
