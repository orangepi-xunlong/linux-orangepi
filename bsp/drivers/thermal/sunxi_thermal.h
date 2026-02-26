/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2023 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Critical temperature handler for Allwinner SOC
 *
 * Copyright (c) 2023 Allwinnertech Ltd.
 */
#ifndef __SUNXI_THERMAL_H__
#define __SUNXI_THERMAL_H__
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/thermal.h>

#define MAX_SENSOR_NUM	8

struct ths_device;

struct tsensor {
	struct ths_device		*tmdev;
	struct thermal_zone_device	*tzd;
	int				id;
#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
	int				last_temp;
#endif
};

struct ths_thermal_chip {
	bool            has_bus_clk;
	bool            has_ths_sclk;
	bool            has_gpadc_clk;
	int		sensor_num;
	int		offset;
	int		scale;
	int		ft_deviation;
	int		temp_data_base;
	int		(*calibrate)(struct ths_device *tmdev);
	int		(*init)(struct ths_device *tmdev);
	int		(*get_temp)(void *data, int *temp);
};

struct ths_device {
	bool					has_calibration;
	const struct ths_thermal_chip		*chip;
	struct device				*dev;
	struct regmap				*regmap;
	struct clk				*bus_clk;
	struct clk				*ths_sclk;
	struct clk				*gpadc_clk;
	struct tsensor				sensor[MAX_SENSOR_NUM];
	struct reset_control			*reset;
};

#endif
