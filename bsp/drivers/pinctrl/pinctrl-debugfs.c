/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <dt-bindings/pinctrl/sun4i-a10.h>

#include "core.h"
#include "pinconf.h"
#include "../gpio/gpiolib.h"
#include "pinctrl-sunxi.h"

static struct dentry *debugfs_root;

#define SUNXI_PINCTRL  "pio"
#define SUNXI_R_PINCTRL "r_pio"
#define SUNXI_MAX_NAME_LEN 20
#define SUNXI_MAX_FUNC_LEN 20

static char sunxi_dbg_pinname[SUNXI_MAX_NAME_LEN];
static char sunxi_dbg_funcname[SUNXI_MAX_FUNC_LEN];
static char sunxi_dbg_devname[SUNXI_MAX_NAME_LEN] = SUNXI_PINCTRL;

static int pin_config_get(const char *dev_name, const char *name,
			unsigned long *config)
{
	struct pinctrl_dev *pctldev;
	int pin, ret;
	const struct pinconf_ops *ops = NULL;

	pctldev = get_pinctrl_dev_from_devname(dev_name);
	if (IS_ERR_OR_NULL(pctldev))
		return -EINVAL;

	mutex_lock(&pctldev->mutex);

	pin = pin_get_from_name(pctldev, name);
	if (pin < 0) {
		ret = pin;
		goto unlock;
	}

	ops = pctldev->desc->confops;
	if (!ops || !ops->pin_config_get) {
		sunxi_err(pctldev->dev,
			"cannot get pin configuration, .pin_config_get missing in driver\n");
		mutex_unlock(&pctldev->mutex);
		return -ENOTSUPP;
	}

	ret = ops->pin_config_get(pctldev, pin, config);
	if (ret < 0) {
		sunxi_err(pctldev->dev, "get config faile\n");
		mutex_unlock(&pctldev->mutex);
		return -EINVAL;
	}
unlock:
	mutex_unlock(&pctldev->mutex);
	return ret;
}

int pin_config_set(const char *dev_name, const char *name,
		unsigned long config)
{
	struct pinctrl_dev *pctldev;
	int pin, ret;
	const struct pinconf_ops *ops = NULL;

	pctldev = get_pinctrl_dev_from_devname(dev_name);
	if (!pctldev) {
		ret = -EINVAL;
		return ret;
	}

	mutex_lock(&pctldev->mutex);

	pin = pin_get_from_name(pctldev, name);
	if (pin < 0) {
		ret = pin;
		goto unlock;
	}

	ops = pctldev->desc->confops;
	if (!ops || !ops->pin_config_set) {
		sunxi_err(pctldev->dev, "cannot configure pin, missing "
			"config function in driver\n");
		mutex_unlock(&pctldev->mutex);
		return -EINVAL;
	}

	ret = ops->pin_config_set(pctldev, pin, &config, 1);
	if (ret < 0) {
		sunxi_err(pctldev->dev, "set config faile\n");
		mutex_unlock(&pctldev->mutex);
		return -EINVAL;
	}

unlock:
	mutex_unlock(&pctldev->mutex);
	return ret;

}

static int sunxi_pin_configure_show(struct seq_file *s, void *d)
{
	int pin;
	unsigned long config;
	struct pinctrl_dev *pctldev;
	struct sunxi_pinctrl *pctl;

	/* get pinctrl device */
	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev) {
		seq_puts(s, "cannot get pinctrl device from devname\n");
		return -EINVAL;
	}

	/* change pin name to pin index */
	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	pctl = pinctrl_dev_get_drvdata(pctldev);

	/* get pin func */
	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_FUNC, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] funciton: %d\n", sunxi_dbg_pinname,
					pinconf_to_config_argument(config));

	/* get pin data */
	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_DAT, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] data: %d\n", sunxi_dbg_pinname,
					pinconf_to_config_argument(config));

	/* get pin dlevel */
	config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0xFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] dlevel: %dmA\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	/* get pin pull */
	config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull up: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull down: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	config = pinconf_to_config_packed(PIN_CONFIG_BIAS_DISABLE, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull disable: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	/* get pin power_source */
	config = pinconf_to_config_packed(PIN_CONFIG_POWER_SOURCE, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] power_source: %d\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	return 0;
}

static ssize_t sunxi_pin_configure_write(struct file *file,
		const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	int pin;
	unsigned int function;
	unsigned int data;
	unsigned int pull;
	unsigned int dlevel;
	unsigned long config;
	struct pinctrl_dev *pctldev;
	struct sunxi_pinctrl *pctl;
	unsigned char buf[SUNXI_MAX_NAME_LEN];

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %u %u %u %u", sunxi_dbg_pinname,
			&function, &data, &dlevel, &pull);

	if (err != 5)
		return -EINVAL;

	if (function > 15) {
		sunxi_err(NULL, "Input Parameters function error!\n");
		return -EINVAL;
	}

	if (data > 1) {
		sunxi_err(NULL, "Input Parameters data error!\n");
		return -EINVAL;
	}

	if (pull > 3) {
		sunxi_err(NULL, "Input Parameters pull error!\n");
		return -EINVAL;
	}

	if (dlevel > 3) {
		sunxi_err(NULL, "Input Parameters dlevel error!\n");
		return -EINVAL;
	}
	dlevel = (dlevel + 1) * 10;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	pctl = pinctrl_dev_get_drvdata(pctldev);

	/* set function value*/
	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_FUNC, function);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	sunxi_debug(NULL, "pin[%s] set function:     %x;\n", sunxi_dbg_pinname, function);

	/* set data value*/
	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_DAT, data);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	sunxi_debug(NULL, "pin[%s] set data:     %x;\n", sunxi_dbg_pinname, data);

	/* set dlevel value */
	config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, dlevel);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	sunxi_debug(NULL, "pin[%s] set dlevel:     %dmA;\n", sunxi_dbg_pinname, dlevel);

	/* set pull value */
	switch (pull) {
	case SUN4I_PINCTRL_NO_PULL:
		config = pinconf_to_config_packed(PIN_CONFIG_BIAS_DISABLE, pull);
		pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
		sunxi_debug(NULL, "pin[%s] set pull disable:     0x%x;\n", sunxi_dbg_pinname, pull);
		break;

	case SUN4I_PINCTRL_PULL_UP:
		config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, pull);
		pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
		sunxi_debug(NULL, "pin[%s] set pull up:     0x%x;\n", sunxi_dbg_pinname, pull);
		break;
	case SUN4I_PINCTRL_PULL_DOWN:
		config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, pull);
		pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
		sunxi_debug(NULL, "pin[%s] set pull down:     0x%x;\n", sunxi_dbg_pinname, pull);
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static int sunxi_pin_show(struct seq_file *s, void *d)
{
	if (strlen(sunxi_dbg_pinname))
		seq_printf(s, "%s\n", sunxi_dbg_pinname);
	else
		seq_puts(s, "No pin name set\n");

	return 0;
}

static ssize_t sunxi_pin_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned char buf[SUNXI_MAX_NAME_LEN];

	if (count > SUNXI_MAX_NAME_LEN)
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s", sunxi_dbg_pinname);
	if (err != 1)
		return -EINVAL;

	return count;
}

static int sunxi_pin_dlevel_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;
	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, 0xFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] dlevel: %dmA\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));
	return 0;
}

static ssize_t sunxi_pin_dlevel_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned long dlevel;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &dlevel);
	if (err != 2)
		return err;

	if (dlevel > 3) {
		sunxi_debug(NULL, "Input Parameters dlevel error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	dlevel = (dlevel + 1) * 10;

	config = pinconf_to_config_packed(PIN_CONFIG_DRIVE_STRENGTH, dlevel);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);

	return count;
}

static int sunxi_pin_pull_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	/* get pin pull */
	config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull up: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull down: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	config = pinconf_to_config_packed(PIN_CONFIG_BIAS_DISABLE, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] pull disable: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	return 0;
}

static ssize_t sunxi_pin_pull_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int err;
	unsigned long pull;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &pull);
	if (err != 2)
		return err;

	if (pull > 3) {
		sunxi_debug(NULL, "Input Parameters pull error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	/* set pull value */
	switch (pull) {
	case SUN4I_PINCTRL_NO_PULL:
		config = pinconf_to_config_packed(PIN_CONFIG_BIAS_DISABLE, pull);
		pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
		sunxi_debug(NULL, "pin[%s] set pull disable: 0x%lx;\n", sunxi_dbg_pinname, pull);
		break;

	case SUN4I_PINCTRL_PULL_UP:
		config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, pull);
		pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
		sunxi_debug(NULL, "pin[%s] set pull up:     0x%lx;\n", sunxi_dbg_pinname, pull);
		break;
	case SUN4I_PINCTRL_PULL_DOWN:
		config = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_DOWN, pull);
		pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
		sunxi_debug(NULL, "pin[%s] set pull down:     0x%lx;\n", sunxi_dbg_pinname, pull);
		break;
	default:
		return -EINVAL;
	}
	return count;
}

static int sunxi_pin_data_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_DAT, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] data: 0x%x\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));
	return 0;
}

static ssize_t sunxi_pin_data_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	int err;
	unsigned long data;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &data);
	if (err != 2)
		return err;

	if (data > 1) {
		sunxi_debug(NULL, "Input Parameters data error!\n");
		return -EINVAL;
	}
	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "unvalid pin:%s for sunxi_dbg_devname:%s\n", sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_DAT, data);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);

	return count;
}

static int sunxi_dev_name_show(struct seq_file *s, void *d)
{
	if (strlen(sunxi_dbg_devname))
		seq_printf(s, "%s\n", sunxi_dbg_devname);
	else
		seq_puts(s, "No dev name set\n");

	return 0;
}

static ssize_t sunxi_dev_name_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned char buf[SUNXI_MAX_NAME_LEN];

	if (count > SUNXI_MAX_NAME_LEN)
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s", sunxi_dbg_devname);
	if (err != 1)
		return -EINVAL;

	return count;
}

/*
 * add gpio function modify node
 */
static ssize_t sunxi_pin_func_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned long function;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &function);
	if (err != 2)
		return err;

	if (function > 15) {
		sunxi_debug(NULL, "Input Parameters function error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_FUNC, function);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	sunxi_debug(NULL, "pin[%s] set function:     %lx;\n", sunxi_dbg_pinname, function);

	return count;
}

static int sunxi_pin_power_source_show(struct seq_file *s, void *d)
{
	unsigned long config;
	int pin;
	struct pinctrl_dev *pctldev;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}
	/* get pin power_source */
	config = pinconf_to_config_packed(PIN_CONFIG_POWER_SOURCE, 0XFFFFFF);
	pin_config_get(sunxi_dbg_devname, sunxi_dbg_pinname, &config);
	seq_printf(s, "pin[%s] power_source: %d\n", sunxi_dbg_pinname,
			pinconf_to_config_argument(config));

	return 0;
}

static ssize_t sunxi_pin_power_source_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	int err;
	unsigned long power_source;
	unsigned long config;
	unsigned char buf[SUNXI_MAX_NAME_LEN];
	int pin;
	struct pinctrl_dev *pctldev;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	err = sscanf(buf, "%19s %lu", sunxi_dbg_pinname, &power_source);
	if (err != 2)
		return err;

	if (power_source != 1800 && power_source != 3300) {
		sunxi_debug(NULL, "Input Parameters power_source error!\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pin = pin_get_from_name(pctldev, sunxi_dbg_pinname);
	if (pin < 0) {
		sunxi_err(NULL, "invalid pin:%s for sunxi_dbg_devname:%s\n",
			sunxi_dbg_pinname, sunxi_dbg_devname);
		return -EINVAL;
	}

	config = pinconf_to_config_packed(PIN_CONFIG_POWER_SOURCE, power_source);
	pin_config_set(sunxi_dbg_devname, sunxi_dbg_pinname, config);
	sunxi_debug(NULL, "pin[%s] set power_source:%ld \n", sunxi_dbg_pinname, power_source);

	return count;
}

static const char *sunxi_pinctrl_get_func_by_pin(struct pinctrl_dev *pctldev, int pinnum, int pinmux)
{
	int i;
	struct sunxi_pinctrl *pctl;

	pctl = pinctrl_dev_get_drvdata(pctldev);
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct sunxi_desc_pin *pin = pctl->desc->pins + i;

		if (pin->pin.number == pinnum) {
			struct sunxi_desc_function *func = pin->functions;

			while (func->name) {
				if (func->muxval == pinmux)
					return func->name;
				func++;
			}
		}
	}

	return NULL;
}

static int sunxi_get_all_pinmux_list(const char **pin_list)
{
	int pin, i, ret;
	const char *func_name;
	struct sunxi_pinctrl *pctl;
	struct pinctrl_dev *pctldev;
	unsigned long config;
	const struct pinconf_ops *ops = NULL;

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pctl = pinctrl_dev_get_drvdata(pctldev);

	ops = pctldev->desc->confops;
	config = pinconf_to_config_packed(SUNXI_PINCFG_TYPE_FUNC, 0XFFFFFF);

	for (i = 0; i < pctl->desc->npins; i++) {
		pin = pctldev->desc->pins[i].number;

		ret = ops->pin_config_get(pctldev, pin, &config);
		if (ret) {
			sunxi_err(NULL, "please check pin_config_get!\n");
			return -ENOTSUPP;
		}

		func_name = sunxi_pinctrl_get_func_by_pin(pctldev, pin,
				pinconf_to_config_argument(config));

		*pin_list++ = func_name;
	}
	return 0;
}

static int sunxi_pinmux_list_show(struct seq_file *s, void *d)
{
	int pin, i, ret, func_len;
	struct pin_desc *desc;
	struct sunxi_pinctrl *pctl;
	struct pinctrl_dev *pctldev;
	const char **pin_list;

	if (strlen(sunxi_dbg_funcname) == 0) {
		sunxi_err(NULL, "[WARNING] Please write 'all' or 'function name' to the pinmux_list node first\n");
		return -EINVAL;
	}

	pctldev = get_pinctrl_dev_from_devname(sunxi_dbg_devname);
	if (!pctldev)
		return -EINVAL;

	pctl = pinctrl_dev_get_drvdata(pctldev);

	pin_list = kmalloc(sizeof(char *) * (pctl->desc->npins), GFP_KERNEL);
	if (!pin_list) {
		sunxi_err(NULL, "pin_list fail to alloc dump buffer!\n");
		return -ENOMEM;
	}

	ret = sunxi_get_all_pinmux_list(pin_list);
	if (ret) {
		kfree(pin_list);
		return -EINVAL;
	}

	for (i = 0; i < pctl->desc->npins; i++) {
		pin = pctldev->desc->pins[i].number;
		desc = pin_desc_get(pctldev, pin);

		func_len = strlen(pin_list[i]);
		if (!strncmp(sunxi_dbg_funcname, pin_list[i], func_len))
			seq_printf(s, "pin %d (%s): funciton: %s\n", pin, desc->name, pin_list[i]);
		else if (!strncmp(sunxi_dbg_funcname, "all", 3))
			seq_printf(s, "pin %d (%s): funciton: %s\n", pin, desc->name, pin_list[i]);
	}

	kfree(pin_list);
	return 0;
}

static ssize_t sunxi_pinmux_list_write(struct file *file,
	const char __user *user_buf, size_t count, loff_t *ppos)
{
	if (copy_from_user(sunxi_dbg_funcname, user_buf, count))
		return -EFAULT;

	return count;
}

static int sunxi_pin_configure_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_configure_show, inode->i_private);
}

static int sunxi_pin_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_show, inode->i_private);
}

static int sunxi_pin_dlevel_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_dlevel_show, inode->i_private);
}

static int sunxi_pin_pull_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_pull_show, inode->i_private);
}

static int sunxi_pin_data_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_data_show, inode->i_private);
}
static int sunxi_dev_name_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_dev_name_show, inode->i_private);
}

static int sunxi_pin_func_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_dev_name_show, inode->i_private);
}

static int sunxi_pin_power_source_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pin_power_source_show, inode->i_private);
}
static int sunxi_pinmux_list_open(struct inode *inode, struct file *file)
{
	return single_open(file, sunxi_pinmux_list_show, inode->i_private);
}

static const struct file_operations sunxi_pin_configure_ops = {
	.open		= sunxi_pin_configure_open,
	.write		= sunxi_pin_configure_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_ops = {
	.open		= sunxi_pin_open,
	.write		= sunxi_pin_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_dev_name_ops = {
	.open		= sunxi_dev_name_open,
	.write		= sunxi_dev_name_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_dlevel_ops = {
	.open		= sunxi_pin_dlevel_open,
	.write		= sunxi_pin_dlevel_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_pull_ops = {
	.open		= sunxi_pin_pull_open,
	.write		= sunxi_pin_pull_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_data_ops = {
	.open           = sunxi_pin_data_open,
	.write          = sunxi_pin_data_write,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
	.owner          = THIS_MODULE,
};

static const struct file_operations sunxi_pin_func_ops = {
	.open		= sunxi_pin_func_open,
	.write		= sunxi_pin_func_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pin_power_source_ops = {
	.open		= sunxi_pin_power_source_open,
	.write		= sunxi_pin_power_source_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static const struct file_operations sunxi_pinmux_list_ops = {
	.open		= sunxi_pinmux_list_open,
	.write		= sunxi_pinmux_list_write,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static int __init sunxi_pinctrl_debugfs_init(void)
{
	debugfs_root = debugfs_create_dir("sunxi_pinctrl", NULL);
	printk("create sunxi_pinctrl success!\n");
	if (IS_ERR_OR_NULL(debugfs_root)) {
		sunxi_debug(NULL, "failed to create debugfs directory\n");
		debugfs_root = NULL;
		return -EFAULT;
	}

	debugfs_create_file("sunxi_pin_configure",
				(S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_configure_ops);
	debugfs_create_file("sunxi_pin", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_ops);
	debugfs_create_file("dlevel", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_dlevel_ops);
	debugfs_create_file("pull", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_pull_ops);
	debugfs_create_file("data", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_data_ops);
	debugfs_create_file("dev_name", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_dev_name_ops);
	debugfs_create_file("function", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_func_ops);
	debugfs_create_file("power_source", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pin_power_source_ops);
	debugfs_create_file("pinmux_list", (S_IRUGO | S_IWUSR | S_IWGRP),
			    debugfs_root, NULL, &sunxi_pinmux_list_ops);

	return 0;
}
module_init(sunxi_pinctrl_debugfs_init);

static void __exit sunxi_pinctrl_debugfs_exit(void)
{
	debugfs_remove_recursive(debugfs_root);
	debugfs_root = NULL;
}
module_exit(sunxi_pinctrl_debugfs_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_VERSION("1.0.4");
