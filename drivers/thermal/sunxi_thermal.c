// SPDX-License-Identifier: GPL-2.0
/*
 * Thermal sensor driver for Allwinner SOC
 * Copyright (C) 2019 frank@allwinnertech.com
 */
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/reset.h>

#define MAX_SENSOR_NUM	4

#define FT_TEMP_MASK				GENMASK(11, 0)
#define TEMP_CALIB_MASK				GENMASK(11, 0)
#define CALIBRATE_DEFAULT			0x800

#define SUN50I_H616_THS_CTRL0			0x00
#define SUN50I_H616_THS_ENABLE			0x04
#define SUN50I_H616_THS_PC			0x08
#define SUN50I_H616_THS_MFC			0x30
#define SUN50I_H616_THS_TEMP_CALIB		0xa0
#define SUN50I_H616_THS_TEMP_DATA		0xc0

#define SUN50I_THS_CTRL0_T_ACQ(x)		(GENMASK(15, 0) & (x))
#define SUN50I_THS_CTRL0_FS_DIV(x)		((GENMASK(15, 0) & (x)) << 16)
#define SUN50I_THS_FILTER_EN			BIT(2)
#define SUN50I_THS_FILTER_TYPE(x)		(GENMASK(1, 0) & (x))
#define SUN50I_H616_THS_PC_TEMP_PERIOD(x)	((GENMASK(19, 0) & (x)) << 12)

struct ths_device;

struct tsensor {
	struct ths_device		*tmdev;
	struct thermal_zone_device	*tzd;
	int				id;
};

struct ths_thermal_chip {
	bool            has_bus_clk;
	int		sensor_num;
	int		offset;
	int		scale;
	int		ft_deviation;
	int		temp_data_base;
	int		(*calibrate)(struct ths_device *tmdev,
				     u16 *caldata, int callen);
	int		(*init)(struct ths_device *tmdev);
};

struct ths_device {
	bool					has_calibration;
	const struct ths_thermal_chip		*chip;
	struct device				*dev;
	struct regmap				*regmap;
	struct clk				*bus_clk;
	struct tsensor				sensor[MAX_SENSOR_NUM];
	struct reset_control			*reset;
};

/* Temp Unit: millidegree Celsius */
static int sunxi_ths_reg2temp(struct ths_device *tmdev, int reg)
{
	return (reg + tmdev->chip->offset) * tmdev->chip->scale;
}

static int sunxi_ths_get_temp(void *data, int *temp)
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
	 * value is added by default.*/
	if (tmdev->has_calibration)
		*temp += tmdev->chip->ft_deviation;

	return 0;
}

static const struct thermal_zone_of_device_ops ths_ops = {
	.get_temp = sunxi_ths_get_temp,
};

static const struct regmap_config config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.fast_io = true,
};

static int sun50i_h616_ths_calibrate(struct ths_device *tmdev,
				   u16 *caldata, int callen)
{
	struct device *dev = tmdev->dev;
	int i, ft_temp;

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
			dev_warn(dev, "sensor%d is not calibrated.\n", i);

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

static int sunxi_ths_calibrate(struct ths_device *tmdev)
{
	struct nvmem_cell *calcell;
	struct device *dev = tmdev->dev;
	u16 *caldata;
	size_t callen;
	int ret = 0;

	calcell = nvmem_cell_get(dev, "calibration");
	if (IS_ERR(calcell)) {
		if (PTR_ERR(calcell) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		goto out;
	}

	caldata = nvmem_cell_read(calcell, &callen);
	if (IS_ERR(caldata)) {
		ret = PTR_ERR(caldata);
		goto out_put;
	}

	tmdev->chip->calibrate(tmdev, caldata, callen);

	kfree(caldata);
out_put:
	nvmem_cell_put(calcell);
out:
	return ret;
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

	if (tmdev->chip->has_bus_clk) {
		tmdev->reset = devm_reset_control_get(dev, NULL);
		if (IS_ERR(tmdev->reset))
			return PTR_ERR(tmdev->reset);

		tmdev->bus_clk = devm_clk_get(&pdev->dev, "bus");
		if (IS_ERR(tmdev->bus_clk))
			return PTR_ERR(tmdev->bus_clk);
	}

	ret = reset_control_deassert(tmdev->reset);
	if (ret)
		return ret;

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

	ret = sunxi_ths_register(tmdev);
	if (ret)
		return ret;
	return ret;
}

static int sunxi_ths_remove(struct platform_device *pdev)
{
	struct ths_device *tmdev = platform_get_drvdata(pdev);

	clk_disable_unprepare(tmdev->bus_clk);

	return 0;
}

static int __maybe_unused sunxi_thermal_suspend(struct device *dev)
{
	struct ths_device *tmdev = dev_get_drvdata(dev);

	clk_disable_unprepare(tmdev->bus_clk);

	return 0;
}

static int __maybe_unused sunxi_thermal_resume(struct device *dev)
{
	struct ths_device *tmdev = dev_get_drvdata(dev);

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
};


static const struct of_device_id of_ths_match[] = {
	{ .compatible = "allwinner,sun50iw9p1-ths", .data = &sun50iw9p1_ths },
	{ .compatible = "allwinner,sun50iw10p1-ths", .data = &sun50iw10p1_ths },
	{ .compatible = "allwinner,sun8iw20p1-ths", .data = &sun8iw20p1_ths },
	{ .compatible = "allwinner,sun20iw1p1-ths", .data = &sun8iw20p1_ths },
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
