/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cam_sensor.h - camera sensor driver
 *
 * Copyright (C) 2023 KY Micro Limited
 * All Rights Reserved.
 */

#ifndef __CAM_SENSOR_H__
#define __CAM_SENSOR_H__

#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>

struct cam_sensor_device {
	struct platform_device *pdev;
	struct cdev cdev;
	u32 id;
	u32 is_probe_succeed;
	u8 twsi_no;
	u8 dphy_no;

	struct regulator *afvdd;
	struct regulator *avdd;
	struct regulator *dovdd;
	struct regulator *dvdd;

	struct gpio_desc *dvdden;
	struct gpio_desc *dcdcen;
	struct gpio_desc *pwdn;
	struct gpio_desc *rst;
#ifdef CONFIG_ARCH_ZYNQMP
	struct gpio_desc *dptc;
#endif
	struct clk *mclk;
	const char *mclk_name;
	bool is_pinmulti;
	bool req_pinmulti;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_state;

	atomic_t usr_cnt;
	struct mutex lock;	/* Protects streaming, format, interval */
};

#endif
