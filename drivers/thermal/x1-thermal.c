// SPDX-License-Identifier: GPL-2.0-only

#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/thermal.h>
#include <linux/reset.h>
#include "thermal_hwmon.h"
#include "x1-thermal.h"

#define MAX_SENSOR_NUMBER		5

static struct x1_thermal_sensor_desc gsdesc[] = {
	/* bjt0: local, sensor internal temperature */
	[0] = {
		.int_msk = 0x14, .int_clr = 0x10, .int_sta = 0x18, .offset = 0x0, .bit_msk = 0b110,
		.se = { .bjt_en = 0x8, .en_val = 0x1, .offset = 0x0, .bit_msk = 0x1, },
		.sd = { .data_reg = 0x20, .offset = 0x0, .bit_msk = 0xffff, },
		.sr = { .temp_thrsh = 0x40, .low_offset = 0x0, .high_offset = 16, },
	},

	/* bjt1: top */
	[1] = {
		.int_msk = 0x14, .int_clr = 0x10, .int_sta = 0x18, .offset = 0x3, .bit_msk = 0b11000,
		.se = { .bjt_en = 0x8, .en_val = 0x1, .offset = 0x1, .bit_msk = 0x2, },
		.sd = { .data_reg = 0x20, .offset = 16, .bit_msk = 0xffff0000, },
		.sr = { .temp_thrsh = 0x44, .low_offset = 0x0, .high_offset = 16, },
	},

	/* bjt2: gpu */
	[2] = {
		.int_msk = 0x14, .int_clr = 0x10, .int_sta = 0x18, .offset = 0x5, .bit_msk = 0b1100000,
		.se = { .bjt_en = 0x8, .en_val = 0x1, .offset = 0x2, .bit_msk = 0x4, },
		.sd = { .data_reg = 0x24, .offset = 0, .bit_msk = 0xffff, },
		.sr = { .temp_thrsh = 0x48, .low_offset = 0x0, .high_offset = 16, },
	},

	/* bjt3: cluster0 */
	[3] = {
		.int_msk = 0x14, .int_clr = 0x10, .int_sta = 0x18, .offset = 0x7, .bit_msk = 0b110000000,
		.se = { .bjt_en = 0x8, .en_val = 0x1, .offset = 0x3, .bit_msk = 0x8, },
		.sd = { .data_reg = 0x24, .offset = 16, .bit_msk = 0xffff0000, },
		.sr = { .temp_thrsh = 0x4c, .low_offset = 0x0, .high_offset = 16, },
	},

	/* bjt4: cluster1 */
	[4] = {
		.int_msk = 0x14, .int_clr = 0x10, .int_sta = 0x18, .offset = 0x9, .bit_msk = 0b11000000000,
		.se = { .bjt_en = 0x8, .en_val = 0x1, .offset = 0x4, .bit_msk = 0x10, },
		.sd = { .data_reg = 0x28, .offset = 0, .bit_msk = 0xffff, },
		.sr = { .temp_thrsh = 0x50, .low_offset = 0x0, .high_offset = 16, },
	},
};

static int init_sensors(struct platform_device *pdev)
{
	int ret, i;
	unsigned int /* thresh, emrt, */ temp;
	struct x1_thermal_sensor *s = platform_get_drvdata(pdev);

	/* read the sensor range */
	ret = of_property_read_u32_array(pdev->dev.of_node, "sensor_range", s->sr, 2);
	if (ret < 0) {
		dev_err(&pdev->dev, "get sensor range error\n");
		return ret;
	}

	if (s->sr[1] >= MAX_SENSOR_NUMBER) {
		dev_err(&pdev->dev, "un-fitable sensor range\n");
		return -EINVAL;	
	}

#if 0
	/* first get the emergent_reboot_thrsh */
	ret = of_property_read_u32(pdev->dev.of_node,
			"emergent_reboot_threshold",
			&emrt);
	if (ret) {
		dev_err(&pdev->dev, "no emergent reboot threshold\n");
		return ret;
	}

	thresh = (emrt + TEMPERATURE_OFFSET) & REG_EMERGENT_REBOOT_TEMP_THR_MSK;
	writel(thresh, s->base + REG_EMERGENT_REBOOT_TEMP_THR);
#endif

	/* first: disable all the interrupts */
	writel(0xffffffff, s->base + REG_TSEN_INT_MASK);

#if 0
	/* enable the emergent intterupt */
	temp = readl(s->base + REG_TSEN_INT_MASK);
	temp &= ~(1 << TSEN_EMERGENT_INT_OFFSET);
	writel(temp, s->base + REG_TSEN_INT_MASK);
#endif

	/*
	 * Decrease filter period time from 0x4000 to 0x3000, that
	 * means decrease 1/4 ADC sampling time for each sensor.
	 */
        temp = readl(s->base + REG_TSEN_TIME_CTRL);
        temp &= ~BITS_TIME_CTRL_MASK;
        temp |= VALUE_FILTER_PERIOD;
        temp |= BITS_RST_ADC_CNT;
        temp |= VALUE_WAIT_REF_CNT;
        writel(temp, s->base + REG_TSEN_TIME_CTRL);

	/*
	 * enable all sensors' auto mode, enable dither control,
	 * consecutive mode, and power up sensor.
	 */
	temp = readl(s->base + REG_TSEN_PCTRL);
        temp |= BIT_TSEN_RAW_SEL | BIT_TEMP_MODE | BIT_EN_SENSOR;
        temp &= ~BITS_TSEN_SW_CTRL;
        temp &= ~BITS_CTUNE;
        writel(temp, s->base + REG_TSEN_PCTRL);

	/* select 24M clk for high speed mode */
	temp = readl(s->base + REG_TSEN_PCTRL2);
	temp &= ~BITS_SDM_CLK_SEL;
	temp |= BITS_SDM_CLK_SEL_24M;
	writel(temp, s->base + REG_TSEN_PCTRL2);

	/* enable the sensor interrupt */
	for (i = s->sr[0]; i <= s->sr[1]; ++i) {
		temp = readl(s->base + s->sdesc[i].se.bjt_en);
		temp &= ~s->sdesc[i].se.bit_msk;
		temp |= (s->sdesc[i].se.en_val << s->sdesc[i].se.offset);
		writel(temp, s->base + s->sdesc[i].se.bjt_en);
	}

	return 0;
}

static void enable_sensors(struct platform_device *pdev)
{
	struct x1_thermal_sensor *s = platform_get_drvdata(pdev);

	writel(readl(s->base + REG_TSEN_INT_MASK) | TSEN_INT_MASK,
			s->base + REG_TSEN_INT_MASK);
	writel(readl(s->base + REG_TSEN_PCTRL) | BIT_HW_AUTO_MODE,
			s->base + REG_TSEN_PCTRL);
}

static void enable_sensor_irq(struct x1_thermal_sensor_desc *desc)
{
	unsigned int temp;

	/* clear the interrupt */
	temp = readl(desc->base + desc->int_clr);
	temp |= desc->bit_msk;
	writel(temp, desc->base + desc->int_clr);

	/* enable the interrupt */
	temp = readl(desc->base + desc->int_msk);
	temp &= ~desc->bit_msk;
	writel(temp, desc->base + desc->int_msk);
}

static int x1_thermal_get_temp(struct thermal_zone_device *tz, int *temp)
{
	struct x1_thermal_sensor_desc *desc = (struct x1_thermal_sensor_desc *)tz->devdata;

	*temp = readl(desc->base + desc->sd.data_reg);
	*temp &= desc->sd.bit_msk;
	*temp >>= desc->sd.offset;

	*temp -= TEMPERATURE_OFFSET;

	*temp *= 1000;

	return 0;
}

static int x1_thermal_set_trips(struct thermal_zone_device *tz, int low, int high)
{
	unsigned int temp;
	int over_thrsh = high;
	int under_thrsh = low;
	struct x1_thermal_sensor_desc *desc = (struct x1_thermal_sensor_desc *)tz->devdata;

	/* set overflow */
	over_thrsh /= 1000;
	over_thrsh += TEMPERATURE_OFFSET;

	temp = readl(desc->base + desc->sr.temp_thrsh);
	temp &= ~0xffff0000;
	temp |= (over_thrsh << desc->sr.high_offset);
	writel(temp, desc->base + desc->sr.temp_thrsh);

	/* set underflow */
	if (low < 0)
		under_thrsh = 0;

	under_thrsh /= 1000;
	under_thrsh += TEMPERATURE_OFFSET;
	temp = readl(desc->base + desc->sr.temp_thrsh);
	temp &= ~0xffff;
	temp |= (under_thrsh << desc->sr.low_offset);
	writel(temp, desc->base + desc->sr.temp_thrsh);

	return 0;
}

static const struct thermal_zone_device_ops x1_of_thermal_ops = {
	.get_temp = x1_thermal_get_temp,
	.set_trips = x1_thermal_set_trips,
};

static irqreturn_t x1_thermal_irq(int irq, void *data)
{
	unsigned int status, msk;
	struct x1_thermal_sensor_desc *desc = (struct x1_thermal_sensor_desc *)data;

	/* get the status */
	status = readl(desc->base + desc->int_sta);
	status &= desc->bit_msk;

	if (status == 0x0)
		return IRQ_HANDLED;

	/* then clear the pending */
	msk = readl(desc->base + desc->int_clr);
	msk |= status;
	writel(msk, desc->base + desc->int_clr);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t x1_thermal_irq_thread(int irq, void *data)
{
	struct x1_thermal_sensor_desc *desc = (struct x1_thermal_sensor_desc *)data;

	thermal_zone_device_update(desc->tzd, THERMAL_EVENT_UNSPECIFIED);

	return IRQ_HANDLED;
}

static int x1_thermal_probe(struct platform_device *pdev)
{
	int ret, i;
	struct resource *res;
	struct x1_thermal_sensor *s;
	struct device *dev = &pdev->dev;

	s = devm_kzalloc(dev, sizeof(*s), GFP_KERNEL);
	if (!s)
		return -ENOMEM;

	s->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	s->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(s->base))
		return PTR_ERR(s->base);

	s->irq = platform_get_irq(pdev, 0);
	if (s->irq < 0) {
		dev_err(dev, "failed to get irq number\n");
		return -EINVAL;
	}

	s->resets = devm_reset_control_get_optional(&pdev->dev, NULL);
	if (IS_ERR(s->resets))
		return PTR_ERR(s->resets);

        reset_control_deassert(s->resets);

	s->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(s->clk))
		return PTR_ERR(s->clk);

	clk_prepare_enable(s->clk);

	s->sdesc = (struct x1_thermal_sensor_desc *)of_device_get_match_data(dev);

	platform_set_drvdata(pdev, s);

	/* initialize the sensors */
	ret = init_sensors(pdev);

	/* then register the thermal zone */
	for (i = s->sr[0]; i <= s->sr[1]; ++i) {
		s->sdesc[i].base = s->base;

		s->sdesc[i].tzd = devm_thermal_of_zone_register(dev,
				i, s->sdesc + i, &x1_of_thermal_ops);
		if (IS_ERR(s->sdesc[i].tzd)) {
			ret = PTR_ERR(s->sdesc[i].tzd);
			dev_err(dev, "faild to register sensor id %d: %d\n",
					i, ret);
			return ret;
		}

		ret = devm_request_threaded_irq(dev, s->irq, x1_thermal_irq,
				x1_thermal_irq_thread, IRQF_SHARED,
				dev_name(&s->sdesc[i].tzd->device), s->sdesc + i);
		if (ret < 0) {
			dev_err(dev, "failed to request irq: %d\n", ret);
			return ret;
		}

		/* register the thermal sensor to hwmon */
		if (devm_thermal_add_hwmon_sysfs(dev, s->sdesc[i].tzd))
			dev_warn(dev, "Failed to add hwmon sysfs attributes\n");

		/* enable sensor low & higth threshold interrupt */
		enable_sensor_irq(s->sdesc + i);
	}

	/* enable the sensor interrupt & using auto mode */
	enable_sensors(pdev);

	return 0;
}

static int x1_thermal_remove(struct platform_device *pdev)
{
	int i;
	struct x1_thermal_sensor *s = platform_get_drvdata(pdev);

	/* disable the clk */
	clk_disable_unprepare(s->clk);
	reset_control_assert(s->resets);

	for (i = s->sr[0]; i <= s->sr[1]; ++i) {
		devm_thermal_of_zone_unregister(&pdev->dev, s->sdesc[i].tzd);
		devm_free_irq(&pdev->dev, s->irq, s->sdesc + i);
	}

	return 0;
}

static const struct of_device_id of_x1_thermal_match[] = {
	{
		.compatible = "ky,x1-tsensor",
		.data = (void *)gsdesc,
	},
	{ /* end */ }
};

MODULE_DEVICE_TABLE(of, of_x1_thermal_match);

static struct platform_driver x1_thermal_driver = {
	.driver = {
		.name		= "x1_thermal",
		.of_match_table = of_x1_thermal_match,
	},
	.probe	= x1_thermal_probe,
	.remove	= x1_thermal_remove,
};

module_platform_driver(x1_thermal_driver);
