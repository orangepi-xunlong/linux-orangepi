// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Thermal sensor driver for Allwinner SOC
 * Copyright (C) 2019 frank@allwinnertech.com
 */
#include <sunxi-log.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/reset.h>
#include <sunxi-sid.h>
#include <linux/version.h>

#include "sunxi_thermal.h"
#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
#include "sunxi_critical_handler.h"
#endif

#define FT_TEMP_MASK				GENMASK(11, 0)
#define TEMP_CALIB_MASK				GENMASK(11, 0)
#define CALIBRATE_DEFAULT			0x800

#define SUN50I_H616_THS_CTRL0			0x00
#define SUN50I_H616_THS_ENABLE			0x04
#define SUN50I_H616_THS_PC			0x08
#define SUN50I_H616_THS_DATA_INTS		0x20
#define SUN50I_H616_THS_MFC			0x30
#define SUN50I_H616_THS_TEMP_CALIB		0xa0
#define SUN50I_H616_THS_TEMP_DATA		0xc0

#define SUN50I_THS_CTRL0_T_ACQ(x)		(GENMASK(15, 0) & (x))
#define SUN50I_THS_CTRL0_FS_DIV(x)		((GENMASK(15, 0) & (x)) << 16)
#define SUN50I_THS_FILTER_EN			BIT(2)
#define SUN50I_THS_FILTER_TYPE(x)		(GENMASK(1, 0) & (x))
#define SUN50I_H616_THS_PC_TEMP_PERIOD(x)	((GENMASK(19, 0) & (x)) << 12)

#define SUN8IW11_THS_CTRL0			(0x00)
#define SUN8IW11_THS_CTRL1			(0x04)
#define SUN8IW11_THS_CTRL2			(0x40)
#define SUN8IW11_THS_INT_CTRL			(0x44)
#define SUN8IW11_THS_INT_STA			(0x48)
#define SUN8IW11_THS_0_INT_SHUT_TH		(0x60)
#define SUN8IW11_THS_1_INT_SHUT_TH		(0x64)
#define SUN8IW11_THS_FILT_CTRL			(0x70)
#define SUN8IW11_THS_0_1_CDATA			(0x74)
#define SUN8IW11_THS_0_DATA			(0x80)
#define SUN8IW11_THS_1_DATA			(0x84)

enum virtual_sensor {
	cpul_idle_sensor = 5,
	cpub_idle_sensor = 6,
	skin_virtual_sensor = 7,
};

/* Temp Unit: millidegree Celsius */
static int sunxi_ths_reg2temp(struct ths_device *tmdev, int reg)
{
	return (reg + tmdev->chip->offset) * tmdev->chip->scale;
}

static int sun8i_ths_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;

	regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
		    0x4 * s->id, &val);

	/* ths have no data yet */
	if (unlikely(!val))
		return -EAGAIN;

	*temp = sunxi_ths_reg2temp(tmdev, val);

	/*
	 * There are problems with the calibration values of some platforms,
	 * which makes the temperature calculated by the original temperature
	 * calculation formula inaccurate. If the chip is calibrated, this
	 * value is added by default. */
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

	return 0;
}

#define SUN55IW3_SENSOR_DATA_CODE (1991)
#define SUN55IW3_OFFSET_BELOW (-2736)
#define SUN55IW3_SCALE_BELOW (-74)
#define SUN55IW3_OFFSET_ABOVE (-2825)
#define SUN55IW3_SCALE_ABOVE (-65)
static int sun55iw3_calc_temp(struct ths_device *tmdev,
			       int id, int reg)
{
	if (reg > SUN55IW3_SENSOR_DATA_CODE)
		return ((reg + SUN55IW3_OFFSET_BELOW) * SUN55IW3_SCALE_BELOW);
	else
		return ((reg + SUN55IW3_OFFSET_ABOVE) * SUN55IW3_SCALE_ABOVE);
}

#define SUN55IW3_GPU_SENSOR_ID (2)
#define SUN55IW3_NPU_SENSOR_ID (3)
static int sun55iw3_ths1_calc_temp(struct ths_device *tmdev,
			       int id, int reg)
{
	/* use npu sensor data instead of gpu */
	if (id == SUN55IW3_GPU_SENSOR_ID) {
		id = SUN55IW3_NPU_SENSOR_ID;
		regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
			    0x4 * id, &reg);
	}

	if (reg > SUN55IW3_SENSOR_DATA_CODE)
		return ((reg + SUN55IW3_OFFSET_BELOW) * SUN55IW3_SCALE_BELOW);
	else
		return ((reg + SUN55IW3_OFFSET_ABOVE) * SUN55IW3_SCALE_ABOVE);
}

static int sun55iw3_ths0_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;
	unsigned int data_ints;

	regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
		    0x4 * s->id, &val);
	regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
	data_ints &= BIT(s->id);

	/* ths have no data yet */
	if (unlikely((!val) || (!data_ints)))
		return -EAGAIN;

	*temp = sun55iw3_calc_temp(tmdev, s->id, val);

	/*
	 * There are problems with the calibration values of some platforms,
	 * which makes the temperature calculated by the original temperature
	 * calculation formula inaccurate. If the chip is calibrated, this
	 * value is added by default. */
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

	return 0;
}

static int sun55iw3_ths1_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;
	unsigned int data_ints;

	regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
		    0x4 * s->id, &val);
	regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
	data_ints &= BIT(s->id);

	/* ths have no data yet */
	if (unlikely((!val) || (!data_ints)))
		return -EAGAIN;

	*temp = sun55iw3_ths1_calc_temp(tmdev, s->id, val);

	/*
	 * There are problems with the calibration values of some platforms,
	 * which makes the temperature calculated by the original temperature
	 * calculation formula inaccurate. If the chip is calibrated, this
	 * value is added by default. */
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

	return 0;
}

#define SUN8IW21_SENSOR_DATA_CODE (1869)
#define SUN8IW21_OFFSET_BELOW (-2796)
#define SUN8IW21_SCALE_BELOW (-70)
#define SUN8IW21_OFFSET_ABOVE (-2822)
#define SUN8IW21_SCALE_ABOVE (-68)
static int sun8iw21_calc_temp(struct ths_device *tmdev,
			       int id, int reg)
{
	if (reg > SUN8IW21_SENSOR_DATA_CODE)
		return ((reg + SUN8IW21_OFFSET_BELOW) * SUN8IW21_SCALE_BELOW);
	else
		return ((reg + SUN8IW21_OFFSET_ABOVE) * SUN8IW21_SCALE_ABOVE);
}

static int sun8iw21_ths_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;

	regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
		    0x4 * s->id, &val);

	/* ths have no data yet */
	if (unlikely(!val))
		return -EAGAIN;

	*temp = sun8iw21_calc_temp(tmdev, s->id, val);

	/*
	 * There are problems with the calibration values of some platforms,
	 * which makes the temperature calculated by the original temperature
	 * calculation formula inaccurate. If the chip is calibrated, this
	 * value is added by default. */
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
	s->last_temp = *temp;
#endif

	return 0;
}

#define SUN55IW6_SENSOR_DATA_CODE (1894)
#define SUN55IW6_OFFSET_BELOW (-2775)
#define SUN55IW6_SCALE_BELOW (-74)
#define SUN55IW6_OFFSET_ABOVE (-2855)
#define SUN55IW6_SCALE_ABOVE (-66)
static int sun55iw6_calc_temp(struct ths_device *tmdev,
			       int id, int reg)
{
	if (reg > SUN55IW6_SENSOR_DATA_CODE)
		return ((reg + SUN55IW6_OFFSET_BELOW) * SUN55IW6_SCALE_BELOW);
	else
		return ((reg + SUN55IW6_OFFSET_ABOVE) * SUN55IW6_SCALE_ABOVE);
}

static int sun55iw6_ths_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;
	unsigned int data_ints;

	regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
		    0x4 * s->id, &val);
	regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
	data_ints &= BIT(s->id);

	/* ths have no data yet */
	if (unlikely((!val) || (!data_ints)))
		return -EAGAIN;

	*temp = sun55iw6_calc_temp(tmdev, s->id, val);

	/*
	 * There are problems with the calibration values of some platforms,
	 * which makes the temperature calculated by the original temperature
	 * calculation formula inaccurate. If the chip is calibrated, this
	 * value is added by default. */
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

	return 0;
}

#define SUN60IW2_SENSOR_DATA_CODE (1769)
#define SUN60IW2_OFFSET_BELOW (-2822)
#define SUN60IW2_SCALE_BELOW (-62)
#define SUN60IW2_OFFSET_ABOVE (-2835)
#define SUN60IW2_SCALE_ABOVE (-59)
static int sun60iw2_calc_temp(struct ths_device *tmdev,
			       int id, int reg)
{
	if (reg > SUN60IW2_SENSOR_DATA_CODE)
		return ((reg + SUN60IW2_OFFSET_BELOW) * SUN60IW2_SCALE_BELOW);
	else
		return ((reg + SUN60IW2_OFFSET_ABOVE) * SUN60IW2_SCALE_ABOVE);
}

#define CPUL_SENSOR_NUM (0)
#define CPUB_SENSOR_NUM (3)
static int sun60iw2_ths_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;
	int val = 0;
	unsigned int data_ints;

	switch (s->id) {
	case cpul_idle_sensor:
		{
			regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
					0x4 * CPUL_SENSOR_NUM, &val);

			regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
			data_ints &= BIT(CPUL_SENSOR_NUM);

			/* ths have no data yet */
			if (unlikely((!val) || (!data_ints)))
				return -EAGAIN;

			*temp = sun60iw2_calc_temp(tmdev, CPUL_SENSOR_NUM, val);
			break;
		}
	case cpub_idle_sensor:
		{
			regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
					0x4 * CPUB_SENSOR_NUM, &val);

			regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
			data_ints &= BIT(CPUB_SENSOR_NUM);

			/* ths have no data yet */
			if (unlikely((!val) || (!data_ints)))
				return -EAGAIN;

			*temp = sun60iw2_calc_temp(tmdev, CPUB_SENSOR_NUM, val);
			break;
		}
	case skin_virtual_sensor:
		{
			regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
				0x4 * CPUB_SENSOR_NUM, &val);
			regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
			data_ints &= BIT(CPUB_SENSOR_NUM);
			/* ths have no data yet */
			if (unlikely((!val) || (!data_ints)))
				return -EAGAIN;
			*temp = sun60iw2_calc_temp(tmdev, CPUB_SENSOR_NUM, val);
			if (*temp <= 30000)
				*temp = 30000;
			else if (*temp <= 100000)
				*temp = (30000 + (*temp - 30000) / 5);
			else
				*temp = (44000 + (*temp -100000) / 10);
			break;
		}
	default:
		{
			regmap_read(tmdev->regmap, tmdev->chip->temp_data_base +
					0x4 * s->id, &val);

			regmap_read(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, &data_ints);
			data_ints &= BIT(s->id);

			/* ths have no data yet */
			if (unlikely((!val) || (!data_ints)))
				return -EAGAIN;

			*temp = sun60iw2_calc_temp(tmdev, s->id, val);
			break;
		}
	}
	/*
	 * There are problems with the calibration values of some platforms,
	 * which makes the temperature calculated by the original temperature
	 * calculation formula inaccurate. If the chip is calibrated, this
	 * value is added by default. */
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sunxi_ths_get_temp(void *data, int *temp)
{
	struct tsensor *s = data;
	struct ths_device *tmdev = s->tmdev;

	return tmdev->chip->get_temp(s, temp);
}

static const struct thermal_zone_of_device_ops ths_ops = {
	.get_temp = sunxi_ths_get_temp,
};
#else
static int sunxi_ths_get_temp(struct thermal_zone_device *data, int *temp)
{
	struct tsensor *s = (struct tsensor *)data->devdata;
	struct ths_device *tmdev = s->tmdev;

	return tmdev->chip->get_temp(s, temp);
}

static const struct thermal_zone_device_ops ths_ops = {
	.get_temp = sunxi_ths_get_temp,
};

#endif
static const struct regmap_config config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.fast_io = true,
};
/* efuse offset depend on specific ic platform */
#define SUN50IW9_THS_EFUSE_OFF (0x14)
static int sun50i_h616_ths_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal[2];
	u16 *caldata;
	int i, ft_temp;

	sunxi_get_module_param_from_sid(ths_cal, SUN50IW9_THS_EFUSE_OFF, 8);

	caldata = (u16 *)(ths_cal);
	if (!caldata[0])
		return -EINVAL;

	/*
	 * efuse layout:
	 *
	 * 0      11  16     27   32     43   48    57
	 * +----------+-----------+-----------+-----------+
	 * |  temp |  |sensor0|   |sensor1|   |sensor2|   |
	 * +----------+-----------+-----------+-----------+
	 *                      ^           ^           ^
	 *                      |           |           |
	 *                      |           |           sensor3[11:8]
	 *                      |           sensor3[7:4]
	 *                      sensor3[3:0]
	 *
	 * The calibration data on the H616 is the ambient temperature and
	 * sensor values that are filled during the factory test stage.
	 *
	 * The unit of stored FT temperature is 0.1 degreee celusis.
	 *
	 * We need to calculate a delta between measured and caluclated
	 * register values and this will become a calibration offset.
	 */
	ft_temp = caldata[0] & FT_TEMP_MASK;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		if (i == 3)
			reg = (caldata[1] >> 12)
			      | (caldata[2] >> 12 << 4)
			      | (caldata[3] >> 12 << 8);
		else
			reg = (int)caldata[i + 1] & TEMP_CALIB_MASK;

		/*
		 * Our calculation formula is like this,
		 * the temp unit above is Celsius:
		 *
		 * T = (sensor_data + a) / b
		 * cdata = 0x800 - [(ft_temp - T) * b]
		 *
		 * b is a floating-point number
		 * with an absolute value less than 1000.
		 *
		 * sunxi_ths_reg2temp uses milli-degrees Celsius,
		 * with offset and scale parameters.
		 * T = (sensor_data + a) * 1000 / b
		 *
		 * ----------------------------------------------
		 *
		 * So:
		 *
		 * offset = a, scale = 1000 / b
		 * cdata = 0x800 - [(ft_temp - T) * 1000 / scale]
		 */
		delta = (ft_temp * 100 - sunxi_ths_reg2temp(tmdev, reg))
			/ tmdev->chip->scale;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			sunxi_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap,
				   SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4),
				   0xfff << offset,
				   cdata << offset);
	}

	tmdev->has_calibration = true;
	return 0;
}

#define SUN8IW11_THS_EFUSE_OFF (0x40)
static int sun8iw11_ths_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal;

	sunxi_get_module_param_from_sid(&ths_cal, SUN8IW11_THS_EFUSE_OFF, 4);
	sunxi_info(dev, "sensor cal is 0x%x\n", ths_cal);

	if (!ths_cal)
		return -EINVAL;

	regmap_write(tmdev->regmap, SUN8IW11_THS_0_1_CDATA, ths_cal);

	tmdev->has_calibration = true;
	return 0;
}

#define SUN8IW21_THS_EFUSE_OFF0 (0x14)
#define SUN8IW21_THS_EFUSE_OFF1 (0x18)
#define SUN8IW21_CAL_COM (5000)
static int sun8iw21_ths_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal[2];
	int i, ft_temp;

	sunxi_get_module_param_from_sid(&ths_cal[0], SUN8IW21_THS_EFUSE_OFF0, 4);
	sunxi_get_module_param_from_sid(&ths_cal[1], SUN8IW21_THS_EFUSE_OFF1, 4);

	ft_temp = ths_cal[0] & FT_TEMP_MASK;
	if (!ft_temp)
		return 0;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		switch (i) {
		case 0:
			reg = (ths_cal[1] >> 4) & TEMP_CALIB_MASK;
			break;
		case 1:
			reg = ((ths_cal[0] >> 24) | (ths_cal[1] << 8)) & TEMP_CALIB_MASK;;
			break;
		case 2:
			reg = (ths_cal[0] >> 12) & TEMP_CALIB_MASK;
			break;
		default:
			reg = 0;
			break;
		}

		delta = (ft_temp * 100 + SUN8IW21_CAL_COM - sun8iw21_calc_temp(tmdev, i, reg))
			/ SUN8IW21_SCALE_BELOW;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			sunxi_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap, SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4), 0xfff << offset, cdata << offset);
	}

	tmdev->has_calibration = true;
	return 0;
}

#define SUN55IW6_THS_EFUSE_OFF0 (0x2C)
#define SUN55IW6_THS_EFUSE_OFF1 (0x30)
#define SUN55IW6_CAL_COM (5000)
static int sun55iw6_ths_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal[2];
	int i, ft_temp;
	sunxi_get_module_param_from_sid(&ths_cal[0], SUN55IW6_THS_EFUSE_OFF0, 4);
	sunxi_get_module_param_from_sid(&ths_cal[1], SUN55IW6_THS_EFUSE_OFF1, 4);

	ft_temp = ((ths_cal[1] << 8) | (ths_cal[0] >> 24)) & FT_TEMP_MASK;
	if (!ft_temp)
		return 0;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		switch (i) {
		case 0:
			reg = (ths_cal[1] >> 4) & TEMP_CALIB_MASK;
			break;
		case 1:
			reg = ths_cal[0] & TEMP_CALIB_MASK;;
			break;
		case 2:
			reg = (ths_cal[0] >> 12) & TEMP_CALIB_MASK;
			break;
		default:
			reg = 0;
			break;
		}

		delta = (ft_temp * 100 + SUN55IW6_CAL_COM - sun55iw6_calc_temp(tmdev, i, reg))
			/ SUN55IW6_SCALE_BELOW;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			sunxi_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap, SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4), 0xfff << offset, cdata << offset);
	}
	tmdev->has_calibration = true;
	return 0;
}

#define SUN60IW2_THS_EFUSE_OFF0 (0x44)
#define SUN60IW2_THS_EFUSE_OFF1 (0x48)
#define SUN60IW2_THS_EFUSE_OFF2 (0x4c)
#define SUN60IW2_CAL_COM (0)
static int sun60iw2_ths_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal[3];
	int i, ft_temp;
	sunxi_get_module_param_from_sid(&ths_cal[0], SUN60IW2_THS_EFUSE_OFF0, 4);
	sunxi_get_module_param_from_sid(&ths_cal[1], SUN60IW2_THS_EFUSE_OFF1, 4);
	sunxi_get_module_param_from_sid(&ths_cal[2], SUN60IW2_THS_EFUSE_OFF2, 4);

	ft_temp = (ths_cal[0] & FT_TEMP_MASK);
	if (!ft_temp)
		return 0;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		switch (i) {
		case 0:
			reg = (ths_cal[0] >> 12) & TEMP_CALIB_MASK;
			break;
		case 1:
			reg = ((ths_cal[0] >> 24) | (ths_cal[1] << 8)) & TEMP_CALIB_MASK;
			break;
		case 2:
			reg = (ths_cal[1] >> 4) & TEMP_CALIB_MASK;
			break;
		case 3:
			reg = (ths_cal[1] >> 16) & TEMP_CALIB_MASK;
			break;
		case 4:
			reg = ((ths_cal[1] >> 28) | (ths_cal[2] << 4)) & TEMP_CALIB_MASK;
			break;
		default:
			return 0;
		}

		delta = (ft_temp * 100 + SUN60IW2_CAL_COM - sun60iw2_calc_temp(tmdev, i, reg))
			/ SUN60IW2_SCALE_BELOW;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			sunxi_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap, SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4), 0xfff << offset, cdata << offset);
	}
	tmdev->has_calibration = true;
	return 0;
}

#define SUN55IW3_THS_EFUSE_OFF0 (0x38)
#define SUN55IW3_THS_EFUSE_OFF1 (0x3c)
#define SUN55IW3_THS_EFUSE_OFF2 (0x44)
#define SUN55IW3_THS_EFUSE_OFF3 (0x48)
#define SUN55IW3_CAL_COM (5000)
static int sun55iw3_ths1_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal[4];
	int i, ft_temp;

	sunxi_get_module_param_from_sid(&ths_cal[0], SUN55IW3_THS_EFUSE_OFF0, 4);
	sunxi_get_module_param_from_sid(&ths_cal[1], SUN55IW3_THS_EFUSE_OFF1, 4);
	sunxi_get_module_param_from_sid(&ths_cal[2], SUN55IW3_THS_EFUSE_OFF2, 4);
	sunxi_get_module_param_from_sid(&ths_cal[3], SUN55IW3_THS_EFUSE_OFF3, 4);

	ft_temp = ((ths_cal[1] << 8) | (ths_cal[0] >> 24)) & FT_TEMP_MASK;
	if (!ft_temp)
		return 0;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		switch (i) {
		case 0:
			reg = (ths_cal[1] >> 4) & TEMP_CALIB_MASK;
			break;
		case 1:
			reg = (ths_cal[1] >> 16) & TEMP_CALIB_MASK;
			break;
		case 2:
			reg = ths_cal[2] & TEMP_CALIB_MASK;
			break;
		case 3:
			reg = (ths_cal[2] >> 12) & TEMP_CALIB_MASK;
			break;
		default:
			reg = 0;
			break;
		}

		delta = (ft_temp * 100 + SUN55IW3_CAL_COM - sun55iw3_calc_temp(tmdev, i, reg))
			/ SUN55IW3_SCALE_BELOW;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			sunxi_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap,
				   SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4),
				   0xfff << offset,
				   cdata << offset);
	}

	tmdev->has_calibration = true;
	return 0;
}

static int sun55iw3_ths0_calibrate(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	u32 ths_cal[4];
	int i, ft_temp;

	sunxi_get_module_param_from_sid(&ths_cal[0], SUN55IW3_THS_EFUSE_OFF0, 4);
	sunxi_get_module_param_from_sid(&ths_cal[1], SUN55IW3_THS_EFUSE_OFF1, 4);
	sunxi_get_module_param_from_sid(&ths_cal[2], SUN55IW3_THS_EFUSE_OFF2, 4);
	sunxi_get_module_param_from_sid(&ths_cal[3], SUN55IW3_THS_EFUSE_OFF3, 4);

	ft_temp = ((ths_cal[1] << 8) | (ths_cal[0] >> 24)) & FT_TEMP_MASK;
	if (!ft_temp)
		return 0;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		int delta, cdata, offset, reg;

		switch (i) {
		case 0:
			reg = ((ths_cal[2] >> 24) | (ths_cal[3] << 8)) & TEMP_CALIB_MASK;
			break;
		default:
			reg = 0;
			break;
		}

		delta = (ft_temp * 100 + SUN55IW3_CAL_COM - sun55iw3_calc_temp(tmdev, i, reg))
			/ SUN55IW3_SCALE_BELOW;
		cdata = CALIBRATE_DEFAULT - delta;
		if (cdata & ~TEMP_CALIB_MASK) {
			sunxi_warn(dev, "sensor%d is not calibrated.\n", i);

			continue;
		}

		offset = (i % 2) * 16;
		regmap_update_bits(tmdev->regmap,
				   SUN50I_H616_THS_TEMP_CALIB + (i / 2 * 4),
				   0xfff << offset,
				   cdata << offset);
	}

	tmdev->has_calibration = true;
	return 0;
}

static int sun8iw17_ths_calibrate(struct ths_device *tmdev)
{
	return 0;
}

static int sunxi_ths_calibrate(struct ths_device *tmdev)
{
	return tmdev->chip->calibrate(tmdev);
}

static int sunxi_ths_resource_init(struct ths_device *tmdev)
{
	struct device *dev = tmdev->dev;
	struct platform_device *pdev = to_platform_device(dev);
	void __iomem *base;
	int ret;

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	tmdev->regmap = devm_regmap_init_mmio(dev, base, &config);
	if (IS_ERR(tmdev->regmap))
		return PTR_ERR(tmdev->regmap);

	if (tmdev->chip->has_ths_sclk) {
		tmdev->ths_sclk = devm_clk_get(&pdev->dev, "sclk");
		if (IS_ERR(tmdev->ths_sclk))
			return PTR_ERR(tmdev->ths_sclk);
	}

	if (tmdev->chip->has_gpadc_clk) {
		tmdev->gpadc_clk = devm_clk_get(&pdev->dev, "gpadc");
		if (IS_ERR(tmdev->gpadc_clk))
			return PTR_ERR(tmdev->gpadc_clk);
	}

	if (tmdev->chip->has_bus_clk) {
		tmdev->reset = devm_reset_control_get_optional_shared(dev, NULL);
		if (IS_ERR(tmdev->reset))
			return PTR_ERR(tmdev->reset);

		tmdev->bus_clk = devm_clk_get(&pdev->dev, "bus");
		if (IS_ERR(tmdev->bus_clk))
			return PTR_ERR(tmdev->bus_clk);
	}

	ret = clk_prepare_enable(tmdev->ths_sclk);
	if (ret)
		return ret;

	ret = clk_prepare_enable(tmdev->gpadc_clk);
	if (ret)
		goto sclk_disable;

	ret = reset_control_deassert(tmdev->reset);
	if (ret)
		goto gpadc_disable;

	ret = clk_prepare_enable(tmdev->bus_clk);
	if (ret)
		goto assert_reset;

	ret = sunxi_ths_calibrate(tmdev);
	if (ret)
		goto bus_disable;

	return 0;

bus_disable:
	clk_disable_unprepare(tmdev->bus_clk);
assert_reset:
	reset_control_assert(tmdev->reset);
gpadc_disable:
	clk_disable_unprepare(tmdev->gpadc_clk);
sclk_disable:
	clk_disable_unprepare(tmdev->ths_sclk);

	return ret;
}

static int sun50i_h616_thermal_init(struct ths_device *tmdev)
{
	int val;

	/*
	 * For sun50iw9p1:
	 * It is necessary that reg[0x03000000] bit[16] is 0.
	 */
	regmap_write(tmdev->regmap, SUN50I_H616_THS_CTRL0,
		     SUN50I_THS_CTRL0_T_ACQ(47) | SUN50I_THS_CTRL0_FS_DIV(479));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_MFC,
		     SUN50I_THS_FILTER_EN |
		     SUN50I_THS_FILTER_TYPE(1));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_PC,
		     SUN50I_H616_THS_PC_TEMP_PERIOD(365));
	val = GENMASK(tmdev->chip->sensor_num - 1, 0);
	regmap_write(tmdev->regmap, SUN50I_H616_THS_ENABLE, val);

	return 0;
}

static int sun55iw3_thermal_init(struct ths_device *tmdev)
{
	int val;

	regmap_write(tmdev->regmap, SUN50I_H616_THS_CTRL0,
		     SUN50I_THS_CTRL0_T_ACQ(47) | SUN50I_THS_CTRL0_FS_DIV(479));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_MFC,
		     SUN50I_THS_FILTER_EN |
		     SUN50I_THS_FILTER_TYPE(1));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_PC,
		     SUN50I_H616_THS_PC_TEMP_PERIOD(28));
	val = GENMASK(tmdev->chip->sensor_num - 1, 0);
	regmap_write(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, val);
	val = GENMASK(tmdev->chip->sensor_num - 1, 0);
	regmap_write(tmdev->regmap, SUN50I_H616_THS_ENABLE, val);

	return 0;
}

static int sun60iw2_thermal_init(struct ths_device *tmdev)
{
	int val;

	regmap_write(tmdev->regmap, SUN50I_H616_THS_CTRL0,
		     SUN50I_THS_CTRL0_T_ACQ(47) | SUN50I_THS_CTRL0_FS_DIV(479));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_MFC,
		     SUN50I_THS_FILTER_EN |
		     SUN50I_THS_FILTER_TYPE(1));
	regmap_write(tmdev->regmap, SUN50I_H616_THS_PC,
		     SUN50I_H616_THS_PC_TEMP_PERIOD(28));
	val = GENMASK(4, 0);
	regmap_write(tmdev->regmap, SUN50I_H616_THS_DATA_INTS, val);
	val = GENMASK(4, 0);
	regmap_write(tmdev->regmap, SUN50I_H616_THS_ENABLE, val);

	return 0;
}

static int sun8iw11_thermal_init(struct ths_device *tmdev)
{
	regmap_write(tmdev->regmap, SUN8IW11_THS_CTRL0, 0x000001df);
	regmap_write(tmdev->regmap, SUN8IW11_THS_CTRL1, 0x00020000);
	regmap_write(tmdev->regmap, SUN8IW11_THS_CTRL2, 0x01df0000);
	regmap_write(tmdev->regmap, SUN8IW11_THS_INT_CTRL, 0x0003a030);
	regmap_write(tmdev->regmap, SUN8IW11_THS_INT_STA, 0x00000333);
	regmap_write(tmdev->regmap, SUN8IW11_THS_0_INT_SHUT_TH, 0x00000000);
	regmap_write(tmdev->regmap, SUN8IW11_THS_1_INT_SHUT_TH, 0x00000000);
	regmap_write(tmdev->regmap, SUN8IW11_THS_FILT_CTRL, 0x00000006);
	regmap_update_bits(tmdev->regmap, SUN8IW11_THS_CTRL2, 0x3, 0x3);

	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)

static int sunxi_ths_register(struct ths_device *tmdev)
{
	int i;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		tmdev->sensor[i].tmdev = tmdev;
		tmdev->sensor[i].id = i;
		tmdev->sensor[i].tzd =
			devm_thermal_zone_of_sensor_register(tmdev->dev,
							     i,
							     &tmdev->sensor[i],
							     &ths_ops);
		if (IS_ERR(tmdev->sensor[i].tzd))
			return PTR_ERR(tmdev->sensor[i].tzd);
	}

	return 0;
}
#else
static int sunxi_ths_register(struct ths_device *tmdev)
{
	int i;

	for (i = 0; i < tmdev->chip->sensor_num; i++) {
		tmdev->sensor[i].tmdev = tmdev;
		tmdev->sensor[i].id = i;
		tmdev->sensor[i].tzd =
			devm_thermal_of_zone_register(tmdev->dev,
							i,
							&tmdev->sensor[i],
							&ths_ops);
		if (IS_ERR(tmdev->sensor[i].tzd))
			return PTR_ERR(tmdev->sensor[i].tzd);
	}

	return 0;
}
#endif

static int sunxi_ths_probe(struct platform_device *pdev)
{
	struct ths_device *tmdev;
	struct device *dev = &pdev->dev;
	int ret;

	tmdev = devm_kzalloc(dev, sizeof(*tmdev), GFP_KERNEL);
	if (!tmdev)
		return -ENOMEM;

	tmdev->dev = dev;
	tmdev->chip = of_device_get_match_data(&pdev->dev);
	if (!tmdev->chip)
		return -EINVAL;

	platform_set_drvdata(pdev, tmdev);

	ret = sunxi_ths_resource_init(tmdev);
	if (ret)
		return ret;

	ret = tmdev->chip->init(tmdev);
	if (ret)
		return ret;

#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
	ret = sunxi_ths_critical_handler_init(dev, tmdev);
	if (ret)
		return ret;
#endif

	ret = sunxi_ths_register(tmdev);
	if (ret)
		return ret;

#if IS_ENABLED(CONFIG_AW_THERMAL_REWRITE_CRITICAL_OPS)
	sunxi_ths_critical_rewrite_ops(tmdev);
#endif

	return ret;
}

static int sunxi_ths_remove(struct platform_device *pdev)
{
	struct ths_device *tmdev = platform_get_drvdata(pdev);

	clk_disable_unprepare(tmdev->bus_clk);
	clk_disable_unprepare(tmdev->gpadc_clk);
	clk_disable_unprepare(tmdev->ths_sclk);

#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
	sunxi_ths_critical_handler_deinit();
#endif

	return 0;
}

static int __maybe_unused sunxi_thermal_suspend(struct device *dev)
{
	struct ths_device *tmdev = dev_get_drvdata(dev);

	clk_disable_unprepare(tmdev->bus_clk);
	clk_disable_unprepare(tmdev->gpadc_clk);
	clk_disable_unprepare(tmdev->ths_sclk);

#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
	sunxi_ths_critical_suspend_handler(tmdev);
#endif

	return 0;
}

static int __maybe_unused sunxi_thermal_resume(struct device *dev)
{
	struct ths_device *tmdev = dev_get_drvdata(dev);

#if IS_ENABLED(CONFIG_AW_THERMAL_CRITICAL_HANDLER)
	sunxi_ths_critical_resume_handler(tmdev);
#endif

	clk_prepare_enable(tmdev->ths_sclk);
	clk_prepare_enable(tmdev->gpadc_clk);
	clk_prepare_enable(tmdev->bus_clk);
	sunxi_ths_calibrate(tmdev);
	tmdev->chip->init(tmdev);

	return 0;
}

static const struct ths_thermal_chip sun50iw9p1_ths = {
	.sensor_num = 4,
	.has_bus_clk = true,
	.offset = -3255,
	.scale = -81,
	.ft_deviation = 8000,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun50i_h616_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun8i_ths_get_temp,
};

static const struct ths_thermal_chip sun50iw10p1_ths = {
	.sensor_num = 3,
	.has_bus_clk = true,
	.offset = -2794,
	.scale = -67,
	.ft_deviation = 8000,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun50i_h616_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun8i_ths_get_temp,
};

static const struct ths_thermal_chip sun8iw20p1_ths = {
	.sensor_num = 1,
	.has_bus_clk = true,
	.offset = -2800,
	.scale = -67,
	.ft_deviation = 0,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun50i_h616_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun8i_ths_get_temp,
};

static const struct ths_thermal_chip sun8iw11p1_ths = {
	.sensor_num = 2,
	.has_bus_clk = true,
	.has_ths_sclk = true,
	.offset = -2222,
	.scale = -102,
	.ft_deviation = 0,
	.temp_data_base = SUN8IW11_THS_0_DATA,
	.calibrate = sun8iw11_ths_calibrate,
	.init = sun8iw11_thermal_init,
	.get_temp = sun8i_ths_get_temp,
};

static const struct ths_thermal_chip sun8iw18p1_ths = {
	.sensor_num = 1,
	.has_bus_clk = true,
	.offset = -2794,
	.scale = -67,
	.ft_deviation = 0,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun50i_h616_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun8i_ths_get_temp,
};

static const struct ths_thermal_chip sun55iw3p1_ths0 = {
	.sensor_num = 1,
	.has_bus_clk = true,
	.has_ths_sclk = true,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun55iw3_ths0_calibrate,
	.init = sun55iw3_thermal_init,
	.get_temp = sun55iw3_ths0_get_temp,
};

static const struct ths_thermal_chip sun55iw3p1_ths1 = {
	.sensor_num = 4,
	.has_bus_clk = true,
	.has_ths_sclk = true,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun55iw3_ths1_calibrate,
	.init = sun55iw3_thermal_init,
	.get_temp = sun55iw3_ths1_get_temp,
};

static const struct ths_thermal_chip sun8iw21p1_ths = {
	.sensor_num = 3,
	.has_bus_clk = true,
	.has_gpadc_clk = true,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun8iw21_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun8iw21_ths_get_temp,
};

static const struct ths_thermal_chip sun55iw6p1_ths = {
	.sensor_num = 3,
	.has_bus_clk = true,
	.has_gpadc_clk = true,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun55iw6_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun55iw6_ths_get_temp,
};

static const struct ths_thermal_chip sun60iw2p1_ths = {
	.sensor_num = 8,
	.has_bus_clk = true,
	.has_gpadc_clk = true,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun60iw2_ths_calibrate,
	.init = sun60iw2_thermal_init,
	.get_temp = sun60iw2_ths_get_temp,
};

static const struct ths_thermal_chip sun8iw17p1_ths = {
	.sensor_num = 4,
	.has_bus_clk = true,
	.offset = -2266,
	.scale = -117,
	.temp_data_base = SUN50I_H616_THS_TEMP_DATA,
	.calibrate = sun8iw17_ths_calibrate,
	.init = sun50i_h616_thermal_init,
	.get_temp = sun8i_ths_get_temp,
};

static const struct of_device_id of_ths_match[] = {
	{ .compatible = "allwinner,sun50iw9p1-ths", .data = &sun50iw9p1_ths },
	{ .compatible = "allwinner,sun50iw10p1-ths", .data = &sun50iw10p1_ths },
	{ .compatible = "allwinner,sun8iw20p1-ths", .data = &sun8iw20p1_ths },
	{ .compatible = "allwinner,sun20iw1p1-ths", .data = &sun8iw20p1_ths },
	{ .compatible = "allwinner,sun8iw11p1-ths", .data = &sun8iw11p1_ths },
	{ .compatible = "allwinner,sun8iw18p1-ths", .data = &sun8iw18p1_ths },
	{ .compatible = "allwinner,sun55iw3p1-ths0", .data = &sun55iw3p1_ths0 },
	{ .compatible = "allwinner,sun55iw3p1-ths1", .data = &sun55iw3p1_ths1 },
	{ .compatible = "allwinner,sun8iw21p1-ths", .data = &sun8iw21p1_ths },
	{ .compatible = "allwinner,sun55iw6p1-ths", .data = &sun55iw6p1_ths },
	{ .compatible = "allwinner,sun60iw2p1-ths", .data = &sun60iw2p1_ths },
	{ .compatible = "allwinner,sun8iw17p1-ths", .data = &sun8iw17p1_ths },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, of_ths_match);

static SIMPLE_DEV_PM_OPS(sunxi_thermal_pm_ops,
			 sunxi_thermal_suspend, sunxi_thermal_resume);

static struct platform_driver ths_driver = {
	.probe = sunxi_ths_probe,
	.remove = sunxi_ths_remove,
	.driver = {
		.name = "sunxi-thermal",
		.pm = &sunxi_thermal_pm_ops,
		.of_match_table = of_ths_match,
	},
};
module_platform_driver(ths_driver);

MODULE_DESCRIPTION("Thermal sensor driver for Allwinner SOC");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("ALLWINNER");
