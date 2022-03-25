// SPDX-License-Identifier: GPL-2.0
/* ADC driver for sunxi platforms' (A10, A13 and A31) GPADC
 *
 * Copyright (c) 2016 Quentin Schulz <quentin.schulz@free-electrons.com>
 *
 * The Allwinner SoCs all have an ADC that can also act as a touchscreen
 * controller and a thermal sensor.
 * The thermal sensor works only when the ADC acts as a touchscreen controller
 * and is configured to throw an interrupt every fixed periods of time (let say
 * every X seconds).
 * One would be tempted to disable the IP on the hardware side rather than
 * disabling interrupts to save some power but that resets the internal clock of
 * the IP, resulting in having to wait X seconds every time we want to read the
 * value of the thermal sensor.
 * This is also the reason of using autosuspend in pm_runtime. If there was no
 * autosuspend, the thermal sensor would need X seconds after every
 * pm_runtime_get_sync to get a value from the ADC. The autosuspend allows the
 * thermal sensor to be requested again in a certain time span before it gets
 * shutdown for not being used.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/thermal.h>
#include <linux/delay.h>

#include <linux/iio/iio.h>
#include <linux/iio/driver.h>
#include <linux/iio/machine.h>
#include <linux/mfd/sun4i-gpadc.h>

static unsigned int sun4i_gpadc_chan_select(unsigned int chan)
{
	return SUN4I_GPADC_CTRL1_ADC_CHAN_SELECT(chan);
}

static unsigned int sun6i_gpadc_chan_select(unsigned int chan)
{
	return SUN6I_GPADC_CTRL1_ADC_CHAN_SELECT(chan);
}

struct sun4i_gpadc_iio;

/*
 * Prototypes for these functions, which enable these functions to be
 * referenced in gpadc_data structures.
 */
static int sun4i_gpadc_sample_start(struct sun4i_gpadc_iio *info);
static int sun4i_gpadc_sample_end(struct sun4i_gpadc_iio *info);

static int sunxi_ths_sample_start(struct sun4i_gpadc_iio *info);
static int sunxi_ths_sample_end(struct sun4i_gpadc_iio *info);

struct gpadc_data {
	int		temp_offset;
	int		temp_scale;
	unsigned int	tp_mode_en;
	unsigned int	tp_adc_select;
	unsigned int	(*adc_chan_select)(unsigned int chan);
	unsigned int	adc_chan_mask;
	unsigned int	temp_data[MAX_SENSOR_COUNT];
	int		(*sample_start)(struct sun4i_gpadc_iio *info);
	int		(*sample_end)(struct sun4i_gpadc_iio *info);
	u32		ctrl0_map;
	u32		ctrl2_map;
	u32		sensor_en_map;
	u32		filter_map;
	u32		irq_clear_map;
	u32		irq_control_map;
	bool		has_bus_clk;
	bool		has_bus_rst;
	bool		has_mod_clk;
	int		sensor_count;
	bool		supports_nvmem;
	bool		support_irq;
};

static const struct gpadc_data sun4i_gpadc_data = {
	.temp_offset = -1932,
	.temp_scale = 133,
	.tp_mode_en = SUN4I_GPADC_CTRL1_TP_MODE_EN,
	.tp_adc_select = SUN4I_GPADC_CTRL1_TP_ADC_SELECT,
	.adc_chan_select = &sun4i_gpadc_chan_select,
	.adc_chan_mask = SUN4I_GPADC_CTRL1_ADC_CHAN_MASK,
	.temp_data = {SUN4I_GPADC_TEMP_DATA, 0, 0, 0},
	.sample_start = sun4i_gpadc_sample_start,
	.sample_end = sun4i_gpadc_sample_end,
	.sensor_count = 1,
	.supports_nvmem = false,
	.support_irq = false,
};

static const struct gpadc_data sun5i_gpadc_data = {
	.temp_offset = -1447,
	.temp_scale = 100,
	.tp_mode_en = SUN4I_GPADC_CTRL1_TP_MODE_EN,
	.tp_adc_select = SUN4I_GPADC_CTRL1_TP_ADC_SELECT,
	.adc_chan_select = &sun4i_gpadc_chan_select,
	.adc_chan_mask = SUN4I_GPADC_CTRL1_ADC_CHAN_MASK,
	.temp_data = {SUN4I_GPADC_TEMP_DATA, 0, 0, 0},
	.sample_start = sun4i_gpadc_sample_start,
	.sample_end = sun4i_gpadc_sample_end,
	.sensor_count = 1,
	.supports_nvmem = false,
	.support_irq = false,
};

static const struct gpadc_data sun6i_gpadc_data = {
	.temp_offset = -1623,
	.temp_scale = 167,
	.tp_mode_en = SUN6I_GPADC_CTRL1_TP_MODE_EN,
	.tp_adc_select = SUN6I_GPADC_CTRL1_TP_ADC_SELECT,
	.adc_chan_select = &sun6i_gpadc_chan_select,
	.adc_chan_mask = SUN6I_GPADC_CTRL1_ADC_CHAN_MASK,
	.temp_data = {SUN4I_GPADC_TEMP_DATA, 0, 0, 0},
	.sample_start = sun4i_gpadc_sample_start,
	.sample_end = sun4i_gpadc_sample_end,
	.sensor_count = 1,
	.supports_nvmem = false,
	.support_irq = false,
};

static const struct gpadc_data sun8i_a33_gpadc_data = {
	.temp_offset = -1662,
	.temp_scale = 162,
	.tp_mode_en = SUN8I_A33_GPADC_CTRL1_CHOP_TEMP_EN,
	.temp_data = {SUN4I_GPADC_TEMP_DATA, 0, 0, 0},
	.sample_start = sun4i_gpadc_sample_start,
	.sample_end = sun4i_gpadc_sample_end,
	.sensor_count = 1,
	.supports_nvmem = false,
	.support_irq = false,
};

static const struct gpadc_data sun8i_h3_ths_data = {
	.temp_offset = -1791,
	.temp_scale = -121,
	.temp_data = {SUNXI_THS_TDATA0, 0, 0, 0},
	.sample_start = sunxi_ths_sample_start,
	.sample_end = sunxi_ths_sample_end,
	.has_bus_clk = true,
	.has_bus_rst = true,
	.has_mod_clk = true,
	.sensor_count = 1,
	.supports_nvmem = true,
	.support_irq = true,
	.ctrl0_map = SUNXI_THS_ACQ0(0xff),
	.ctrl2_map = SUNXI_THS_ACQ1(0x3f),
	.sensor_en_map = SUNXI_THS_TEMP_SENSE_EN0,
	.filter_map = SUNXI_THS_FILTER_EN |
		SUNXI_THS_FILTER_TYPE(0x2),
	.irq_clear_map = SUNXI_THS_INTS_ALARM_INT_0 |
			SUNXI_THS_INTS_SHUT_INT_0   |
			SUNXI_THS_INTS_TDATA_IRQ_0  |
			SUNXI_THS_INTS_ALARM_OFF_0,
	.irq_control_map = SUNXI_THS_INTC_TDATA_IRQ_EN0 |
		SUNXI_THS_TEMP_PERIOD(0x7),
};

static const struct gpadc_data sun8i_a83t_ths_data = {
	.temp_offset = -2724,
	.temp_scale = -70,
	.temp_data = {SUNXI_THS_TDATA0,
		SUNXI_THS_TDATA1,
		SUNXI_THS_TDATA2,
		0},
	.sample_start = sunxi_ths_sample_start,
	.sample_end = sunxi_ths_sample_end,
	.sensor_count = 3,
	.supports_nvmem = false,
	.support_irq = true,
	.ctrl0_map = SUNXI_THS_ACQ0(0x1f3),
	.ctrl2_map = SUNXI_THS_ACQ1(0x1f3),
	.sensor_en_map = SUNXI_THS_TEMP_SENSE_EN0 |
		SUNXI_THS_TEMP_SENSE_EN1 |
		SUNXI_THS_TEMP_SENSE_EN2,
	.filter_map = SUNXI_THS_FILTER_EN |
		SUNXI_THS_FILTER_TYPE(0x2),
	.irq_clear_map = SUNXI_THS_INTS_ALARM_INT_0 |
		SUNXI_THS_INTS_ALARM_INT_1 |
		SUNXI_THS_INTS_ALARM_INT_2 |
		SUNXI_THS_INTS_SHUT_INT_0  |
		SUNXI_THS_INTS_SHUT_INT_1  |
		SUNXI_THS_INTS_SHUT_INT_2  |
		SUNXI_THS_INTS_TDATA_IRQ_0 |
		SUNXI_THS_INTS_TDATA_IRQ_1 |
		SUNXI_THS_INTS_TDATA_IRQ_2,
	.irq_control_map = SUNXI_THS_INTC_TDATA_IRQ_EN0 |
		SUNXI_THS_INTC_TDATA_IRQ_EN1 |
		SUNXI_THS_INTC_TDATA_IRQ_EN2 |
		SUNXI_THS_TEMP_PERIOD(0x257),
};

static const struct gpadc_data sun9i_a80_ths_data = {
	.temp_offset = -2794,
	.temp_scale = -67,
	.temp_data = {SUNXI_THS_TDATA0,
		SUNXI_THS_TDATA1,
		SUNXI_THS_TDATA2,
		SUNXI_THS_TDATA3},
	.sample_start = sunxi_ths_sample_start,
	.sample_end = sunxi_ths_sample_end,
	.has_bus_clk = true,
	.has_bus_rst = true,
	.has_mod_clk = true,
	.sensor_count = 4,
	.supports_nvmem = false,
	.support_irq = true,
	.ctrl0_map = SUNXI_THS_ACQ0(0x1f3),
	.ctrl2_map = SUNXI_THS_TEMP_SENSE_EN0 |
		SUNXI_THS_TEMP_SENSE_EN1 |
		SUNXI_THS_TEMP_SENSE_EN2 |
		SUNXI_THS_TEMP_SENSE_EN3 |
		SUNXI_THS_ACQ1(0x1f3),
	.filter_map = SUNXI_THS_FILTER_EN |
		SUNXI_THS_FILTER_TYPE(0x2),
	.irq_clear_map = SUNXI_THS_INTS_ALARM_INT_0 |
		SUNXI_THS_INTS_ALARM_INT_1 |
		SUNXI_THS_INTS_ALARM_INT_2 |
		SUNXI_THS_INTS_ALARM_INT_3 |
		SUNXI_THS_INTS_SHUT_INT_0  |
		SUNXI_THS_INTS_SHUT_INT_1  |
		SUNXI_THS_INTS_SHUT_INT_2  |
		SUNXI_THS_INTS_SHUT_INT_3  |
		SUNXI_THS_INTS_TDATA_IRQ_0 |
		SUNXI_THS_INTS_TDATA_IRQ_1 |
		SUNXI_THS_INTS_TDATA_IRQ_2 |
		SUNXI_THS_INTS_TDATA_IRQ_3,
	.irq_control_map = SUNXI_THS_INTC_TDATA_IRQ_EN0 |
		SUNXI_THS_INTC_TDATA_IRQ_EN1 |
		SUNXI_THS_INTC_TDATA_IRQ_EN2 |
		SUNXI_THS_INTC_TDATA_IRQ_EN3 |
		SUNXI_THS_TEMP_PERIOD(0x3a),
};

static const struct gpadc_data sun50i_a64_ths_data = {
	.temp_offset = -2170,
	.temp_scale = -117,
	.temp_data = {SUNXI_THS_TDATA0,
		SUNXI_THS_TDATA1,
		SUNXI_THS_TDATA2,
		0},
	.sample_start = sunxi_ths_sample_start,
	.sample_end = sunxi_ths_sample_end,
	.has_bus_clk = true,
	.has_bus_rst = true,
	.has_mod_clk = true,
	.sensor_count = 3,
	.supports_nvmem = false,
	.support_irq = true,

	/* The final sample period is calculated as follows:
	 * (THERMAL_PER + 1) * 4096 / 24MHz * 2^(FILTER_TYPE + 1)
	 *
	 * This results to about 1Hz with these settings.
	 */
	.ctrl0_map = SUNXI_THS_ACQ0(0xff),
	.ctrl2_map = SUNXI_THS_TEMP_SENSE_EN0 |
		SUNXI_THS_TEMP_SENSE_EN1 |
		SUNXI_THS_TEMP_SENSE_EN2 |
		SUNXI_THS_ACQ1(0x3f),
	.filter_map = SUNXI_THS_FILTER_EN |
		SUNXI_THS_FILTER_TYPE(0x1),
	.irq_clear_map = SUNXI_THS_INTS_ALARM_INT_0 |
		SUNXI_THS_INTS_ALARM_INT_1 |
		SUNXI_THS_INTS_ALARM_INT_2 |
		SUNXI_THS_INTS_SHUT_INT_0  |
		SUNXI_THS_INTS_SHUT_INT_1  |
		SUNXI_THS_INTS_SHUT_INT_2  |
		SUNXI_THS_INTS_TDATA_IRQ_0 |
		SUNXI_THS_INTS_TDATA_IRQ_1 |
		SUNXI_THS_INTS_TDATA_IRQ_2 |
		SUNXI_THS_INTS_ALARM_OFF_0 |
		SUNXI_THS_INTS_ALARM_OFF_1 |
		SUNXI_THS_INTS_ALARM_OFF_2,
	.irq_control_map = SUNXI_THS_INTC_TDATA_IRQ_EN0 |
		SUNXI_THS_INTC_TDATA_IRQ_EN1 |
		SUNXI_THS_INTC_TDATA_IRQ_EN2 |
		SUNXI_THS_TEMP_PERIOD(0x7),
};

struct sun4i_gpadc_iio {
	struct iio_dev			*indio_dev;
	struct completion		completion;
	int				temp_data;
	u32				adc_data;
	struct regmap			*regmap;
	unsigned int			fifo_data_irq;
	atomic_t			ignore_fifo_data_irq;
	unsigned int			temp_data_irq;
	atomic_t			ignore_temp_data_irq;
	const struct gpadc_data		*data;
	bool				no_irq;
	struct clk			*bus_clk;
	struct clk			*mod_clk;
	struct reset_control		*reset;
	int				sensor_id;
	u32				calibration_data[2];
	bool				has_calibration_data[2];
	/* prevents concurrent reads of temperature and ADC */
	struct mutex			mutex;
	struct thermal_zone_device	*tzd;
	struct device			*sensor_device;
};

#define SUN4I_GPADC_ADC_CHANNEL(_channel, _name) {		\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = _channel,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
	.datasheet_name = _name,				\
}

static struct iio_map sun4i_gpadc_hwmon_maps[] = {
	{
		.adc_channel_label = "temp_adc",
		.consumer_dev_name = "iio_hwmon.0",
	},
	{ /* sentinel */ },
};

static const struct iio_chan_spec sun4i_gpadc_channels[] = {
	SUN4I_GPADC_ADC_CHANNEL(0, "adc_chan0"),
	SUN4I_GPADC_ADC_CHANNEL(1, "adc_chan1"),
	SUN4I_GPADC_ADC_CHANNEL(2, "adc_chan2"),
	SUN4I_GPADC_ADC_CHANNEL(3, "adc_chan3"),
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_SCALE) |
				      BIT(IIO_CHAN_INFO_OFFSET),
		.datasheet_name = "temp_adc",
	},
};

static const struct iio_chan_spec sun4i_gpadc_channels_no_temp[] = {
	SUN4I_GPADC_ADC_CHANNEL(0, "adc_chan0"),
	SUN4I_GPADC_ADC_CHANNEL(1, "adc_chan1"),
	SUN4I_GPADC_ADC_CHANNEL(2, "adc_chan2"),
	SUN4I_GPADC_ADC_CHANNEL(3, "adc_chan3"),
};

static const struct iio_chan_spec sun8i_a33_gpadc_channels[] = {
	{
		.type = IIO_TEMP,
		.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) |
				      BIT(IIO_CHAN_INFO_SCALE) |
				      BIT(IIO_CHAN_INFO_OFFSET),
		.datasheet_name = "temp_adc",
	},
};

static const struct regmap_config sun4i_gpadc_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.fast_io = true,
};

static int sun4i_prepare_for_irq(struct iio_dev *indio_dev, int channel,
				 unsigned int irq)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);
	int ret;
	u32 reg;

	pm_runtime_get_sync(indio_dev->dev.parent);

	reinit_completion(&info->completion);

	ret = regmap_write(info->regmap, SUN4I_GPADC_INT_FIFOC,
			   SUN4I_GPADC_INT_FIFOC_TP_FIFO_TRIG_LEVEL(1) |
			   SUN4I_GPADC_INT_FIFOC_TP_FIFO_FLUSH);
	if (ret)
		return ret;

	ret = regmap_read(info->regmap, SUN4I_GPADC_CTRL1, &reg);
	if (ret)
		return ret;

	if (irq == info->fifo_data_irq) {
		ret = regmap_write(info->regmap, SUN4I_GPADC_CTRL1,
				   info->data->tp_mode_en |
				   info->data->tp_adc_select |
				   info->data->adc_chan_select(channel));
		/*
		 * When the IP changes channel, it needs a bit of time to get
		 * correct values.
		 */
		if ((reg & info->data->adc_chan_mask) !=
			 info->data->adc_chan_select(channel))
			mdelay(10);

	} else {
		/*
		 * The temperature sensor returns valid data only when the ADC
		 * operates in touchscreen mode.
		 */
		ret = regmap_write(info->regmap, SUN4I_GPADC_CTRL1,
				   info->data->tp_mode_en);
	}

	if (ret)
		return ret;

	/*
	 * When the IP changes mode between ADC or touchscreen, it
	 * needs a bit of time to get correct values.
	 */
	if ((reg & info->data->tp_adc_select) != info->data->tp_adc_select)
		mdelay(100);

	return 0;
}

static int sun4i_gpadc_read(struct iio_dev *indio_dev, int channel, int *val,
			    unsigned int irq)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);
	int ret;

	mutex_lock(&info->mutex);

	ret = sun4i_prepare_for_irq(indio_dev, channel, irq);
	if (ret)
		goto err;

	enable_irq(irq);

	/*
	 * The temperature sensor throws an interruption periodically (currently
	 * set at periods of ~0.6s in sun4i_gpadc_runtime_resume). A 1s delay
	 * makes sure an interruption occurs in normal conditions. If it doesn't
	 * occur, then there is a timeout.
	 */
	if (!wait_for_completion_timeout(&info->completion,
					 msecs_to_jiffies(1000))) {
		ret = -ETIMEDOUT;
		goto err;
	}

	if (irq == info->fifo_data_irq)
		*val = info->adc_data;
	else
		*val = info->temp_data;

	ret = 0;
	pm_runtime_mark_last_busy(indio_dev->dev.parent);

err:
	pm_runtime_put_autosuspend(indio_dev->dev.parent);
	disable_irq(irq);
	mutex_unlock(&info->mutex);

	return ret;
}

static int sun4i_gpadc_adc_read(struct iio_dev *indio_dev, int channel,
				int *val)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);

	return sun4i_gpadc_read(indio_dev, channel, val, info->fifo_data_irq);
}

static int sun4i_gpadc_temp_read(struct iio_dev *indio_dev, int *val,
				int sensor)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);

	if (info->no_irq) {
		pm_runtime_get_sync(indio_dev->dev.parent);

		regmap_read(info->regmap, info->data->temp_data[sensor], val);

		pm_runtime_mark_last_busy(indio_dev->dev.parent);
		pm_runtime_put_autosuspend(indio_dev->dev.parent);

		return 0;
	}

	if (info->data->support_irq) {
		regmap_read(info->regmap, info->data->temp_data[sensor], val);
		return 0;
	}

	return sun4i_gpadc_read(indio_dev, 0, val, info->temp_data_irq);
}

static int sun4i_gpadc_temp_offset(struct iio_dev *indio_dev, int *val)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);

	*val = info->data->temp_offset;

	return 0;
}

static int sun4i_gpadc_temp_scale(struct iio_dev *indio_dev, int *val)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);

	*val = info->data->temp_scale;

	return 0;
}

static int sun4i_gpadc_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan, int *val,
				int *val2, long mask)
{
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_OFFSET:
		ret = sun4i_gpadc_temp_offset(indio_dev, val);
		if (ret)
			return ret;

		return IIO_VAL_INT;
	case IIO_CHAN_INFO_RAW:
		if (chan->type == IIO_VOLTAGE)
			ret = sun4i_gpadc_adc_read(indio_dev, chan->channel,
						   val);
		else
			ret = sun4i_gpadc_temp_read(indio_dev, val, 0);

		if (ret)
			return ret;

		return IIO_VAL_INT;
	case IIO_CHAN_INFO_SCALE:
		if (chan->type == IIO_VOLTAGE) {
			/* 3000mV / 4096 * raw */
			*val = 0;
			*val2 = 732421875;
			return IIO_VAL_INT_PLUS_NANO;
		}

		ret = sun4i_gpadc_temp_scale(indio_dev, val);
		if (ret)
			return ret;

		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static const struct iio_info sun4i_gpadc_iio_info = {
	.read_raw = sun4i_gpadc_read_raw,
};

static irqreturn_t sun4i_gpadc_temp_data_irq_handler(int irq, void *dev_id)
{
	struct sun4i_gpadc_iio *info = dev_id;

	if (atomic_read(&info->ignore_temp_data_irq))
		goto out;

	if (!regmap_read(info->regmap, SUN4I_GPADC_TEMP_DATA, &info->temp_data))
		complete(&info->completion);

out:
	return IRQ_HANDLED;
}

static irqreturn_t sun4i_gpadc_fifo_data_irq_handler(int irq, void *dev_id)
{
	struct sun4i_gpadc_iio *info = dev_id;

	if (atomic_read(&info->ignore_fifo_data_irq))
		goto out;

	if (!regmap_read(info->regmap, SUN4I_GPADC_DATA, &info->adc_data))
		complete(&info->completion);

out:
	return IRQ_HANDLED;
}

static irqreturn_t sunxi_irq_thread(int irq, void *data)
{
	struct sun4i_gpadc_iio *info = data;

	regmap_write(info->regmap, SUNXI_THS_STAT, info->data->irq_clear_map);

	thermal_zone_device_update(info->tzd, THERMAL_EVENT_TEMP_SAMPLE);

	return IRQ_HANDLED;
}

static int sun4i_gpadc_sample_end(struct sun4i_gpadc_iio *info)
{
	/* Disable the ADC on IP */
	regmap_write(info->regmap, SUN4I_GPADC_CTRL1, 0);
	/* Disable temperature sensor on IP */
	regmap_write(info->regmap, SUN4I_GPADC_TPR, 0);

	return 0;
}

static int sunxi_ths_sample_end(struct sun4i_gpadc_iio *info)
{
	/* Disable ths interrupt*/
	regmap_write(info->regmap, SUNXI_THS_INTC, 0x0);
	/* Disable temperature sensor */
	regmap_write(info->regmap, SUNXI_THS_CTRL2, 0x0);

	return 0;
}

static int sun4i_gpadc_runtime_suspend(struct device *dev)
{
	struct sun4i_gpadc_iio *info = iio_priv(dev_get_drvdata(dev));

	if (info->data->has_mod_clk)
		clk_disable(info->mod_clk);

	if (info->data->has_bus_clk)
		clk_disable(info->bus_clk);

	return info->data->sample_end(info);
}

static void sunxi_calibrate(struct sun4i_gpadc_iio *info)
{
	if (info->has_calibration_data[0])
		regmap_write(info->regmap, SUNXI_THS_CDATA_0_1,
			info->calibration_data[0]);

	if (info->has_calibration_data[1])
		regmap_write(info->regmap, SUNXI_THS_CDATA_2_3,
			info->calibration_data[1]);
}

static int sun4i_gpadc_sample_start(struct sun4i_gpadc_iio *info)
{
	/* clkin = 6MHz */
	regmap_write(info->regmap, SUN4I_GPADC_CTRL0,
		     SUN4I_GPADC_CTRL0_ADC_CLK_DIVIDER(2) |
		     SUN4I_GPADC_CTRL0_FS_DIV(7) |
		     SUNXI_THS_ACQ0(63));
	regmap_write(info->regmap, SUN4I_GPADC_CTRL1, info->data->tp_mode_en);
	regmap_write(info->regmap, SUN4I_GPADC_CTRL3,
		     SUNXI_THS_FILTER_EN |
		     SUNXI_THS_FILTER_TYPE(1));
	/* period = SUN4I_GPADC_TPR_TEMP_PERIOD * 256 * 16 / clkin; ~0.6s */
	regmap_write(info->regmap, SUN4I_GPADC_TPR,
		     SUN4I_GPADC_TPR_TEMP_ENABLE |
		     SUN4I_GPADC_TPR_TEMP_PERIOD(800));

	return 0;
}

static int sunxi_ths_sample_start(struct sun4i_gpadc_iio *info)
{
	u32 value;
	sunxi_calibrate(info);

	if (info->data->ctrl0_map)
		regmap_write(info->regmap, SUNXI_THS_CTRL0,
			info->data->ctrl0_map);

	regmap_write(info->regmap, SUNXI_THS_CTRL2,
		info->data->ctrl2_map);

	regmap_write(info->regmap, SUNXI_THS_STAT,
			info->data->irq_clear_map);

	regmap_write(info->regmap, SUNXI_THS_FILTER,
		info->data->filter_map);

	regmap_write(info->regmap, SUNXI_THS_INTC,
		info->data->irq_control_map);

	regmap_read(info->regmap, SUNXI_THS_CTRL2, &value);

	regmap_write(info->regmap, SUNXI_THS_CTRL2,
		info->data->sensor_en_map | value);

	return 0;
}

static int sun4i_gpadc_runtime_resume(struct device *dev)
{
	struct sun4i_gpadc_iio *info = iio_priv(dev_get_drvdata(dev));

	if (info->data->has_mod_clk)
		clk_enable(info->mod_clk);

	if (info->data->has_bus_clk)
		clk_enable(info->bus_clk);

	return info->data->sample_start(info);
}

static int sun4i_gpadc_get_temp(void *data, int *temp)
{
	struct sun4i_gpadc_iio *info = data;
	int val, scale, offset;

	if (sun4i_gpadc_temp_read(info->indio_dev, &val, info->sensor_id))
		return -ETIMEDOUT;

	/* Ignore first sample which is always zero. 0 is either too
	 * cold or too hot, so we can safely ignore it
	 */
	if (val == 0)
		return -ETIMEDOUT;

	sun4i_gpadc_temp_scale(info->indio_dev, &scale);
	sun4i_gpadc_temp_offset(info->indio_dev, &offset);

	*temp = (val + offset) * scale;

	return 0;
}

static const struct thermal_zone_of_device_ops sun4i_ts_tz_ops = {
	.get_temp = &sun4i_gpadc_get_temp,
};

static const struct dev_pm_ops sun4i_gpadc_pm_ops = {
	.runtime_suspend = &sun4i_gpadc_runtime_suspend,
	.runtime_resume = &sun4i_gpadc_runtime_resume,
};

static int sun4i_irq_init(struct platform_device *pdev, const char *name,
			  irq_handler_t handler, const char *devname,
			  unsigned int *irq, atomic_t *atomic)
{
	int ret;
	struct sun4i_gpadc_dev *mfd_dev = dev_get_drvdata(pdev->dev.parent);
	struct sun4i_gpadc_iio *info = iio_priv(dev_get_drvdata(&pdev->dev));

	/*
	 * Once the interrupt is activated, the IP continuously performs
	 * conversions thus throws interrupts. The interrupt is activated right
	 * after being requested but we want to control when these interrupts
	 * occur thus we disable it right after being requested. However, an
	 * interrupt might occur between these two instructions and we have to
	 * make sure that does not happen, by using atomic flags. We set the
	 * flag before requesting the interrupt and unset it right after
	 * disabling the interrupt. When an interrupt occurs between these two
	 * instructions, reading the atomic flag will tell us to ignore the
	 * interrupt.
	 */
	atomic_set(atomic, 1);

	ret = platform_get_irq_byname(pdev, name);
	if (ret < 0)
		return ret;

	ret = regmap_irq_get_virq(mfd_dev->regmap_irqc, ret);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get virq for irq %s\n", name);
		return ret;
	}

	*irq = ret;
	ret = devm_request_any_context_irq(&pdev->dev, *irq, handler,
					   IRQF_NO_AUTOEN,
					   devname, info);
	if (ret < 0) {
		dev_err(&pdev->dev, "could not request %s interrupt: %d\n",
			name, ret);
		return ret;
	}

	atomic_set(atomic, 0);

	return 0;
}

static const struct of_device_id sun4i_gpadc_of_id[] = {
	{
		.compatible = "allwinner,sun8i-a33-ths",
		.data = &sun8i_a33_gpadc_data,
	},
	{
		.compatible = "allwinner,sun8i-h3-ths",
		.data = &sun8i_h3_ths_data,
	},
	{
		.compatible = "allwinner,sun8i-a83t-ths",
		.data = &sun8i_a83t_ths_data,
	},
	{
		.compatible = "allwinner,sun9i-a80-ths",
		.data = &sun9i_a80_ths_data,
	},
	{
		.compatible = "allwinner,sun50i-a64-ths",
		.data = &sun50i_a64_ths_data,
	},
	{ /* sentinel */ }
};

static int sun4i_gpadc_probe_dt(struct platform_device *pdev,
				struct iio_dev *indio_dev)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);
	void __iomem *base;
	int ret;
	struct nvmem_cell *cell;
	ssize_t cell_size;
	u64 *cell_data;
	int irq;

	info->data = of_device_get_match_data(&pdev->dev);
	if (!info->data)
		return -ENODEV;

	if (info->data->support_irq) {
		/* only the new versions of ths support right now irqs */
		irq = platform_get_irq(pdev, 0);
		if (irq < 0) {
			dev_err(&pdev->dev, "failed to get IRQ: %d\n", irq);
			return irq;
		}

		ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
				sunxi_irq_thread, IRQF_ONESHOT,
				dev_name(&pdev->dev), info);
		if (ret)
			return ret;

	} else
		info->no_irq = true;

	indio_dev->num_channels = ARRAY_SIZE(sun8i_a33_gpadc_channels);
	indio_dev->channels = sun8i_a33_gpadc_channels;

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	info->has_calibration_data[0] = false;
	info->has_calibration_data[1] = false;

	if (!info->data->supports_nvmem)
		goto no_nvmem;

	cell = devm_nvmem_cell_get(&pdev->dev, "calibration");
	if (IS_ERR(cell)) {
		if (PTR_ERR(cell) == -EPROBE_DEFER)
			return PTR_ERR(cell);
	} else {
		cell_data = (u64 *)nvmem_cell_read(cell, &cell_size);
		devm_nvmem_cell_put(&pdev->dev, cell);
		if (cell_size <= 4) {
			info->has_calibration_data[0] = true;
			info->calibration_data[0] = be32_to_cpu(cell_data[0] &
					GENMASK(31, 0));
		} else if (cell_size <= 8) {
			info->has_calibration_data[0] = true;
			info->calibration_data[0] = be32_to_cpu(cell_data[0] &
					GENMASK(31, 0));
			info->has_calibration_data[1] = true;
			info->calibration_data[1] = be32_to_cpu(
					(cell_data[0] >> 32) & GENMASK(31, 0));
		}
	}

no_nvmem:

	info->regmap = devm_regmap_init_mmio(&pdev->dev, base,
					     &sun4i_gpadc_regmap_config);
	if (IS_ERR(info->regmap)) {
		ret = PTR_ERR(info->regmap);
		dev_err(&pdev->dev, "failed to init regmap: %d\n", ret);
		return ret;
	}

	if (info->data->has_bus_rst) {
		info->reset = devm_reset_control_get(&pdev->dev, NULL);
		if (IS_ERR(info->reset)) {
			ret = PTR_ERR(info->reset);
			return ret;
		}

		ret = reset_control_deassert(info->reset);
		if (ret)
			return ret;
	}

	if (info->data->has_bus_clk) {
		info->bus_clk = devm_clk_get(&pdev->dev, "bus");
		if (IS_ERR(info->bus_clk)) {
			ret = PTR_ERR(info->bus_clk);
			goto assert_reset;
		}

		ret = clk_prepare_enable(info->bus_clk);
		if (ret)
			goto assert_reset;
	}

	if (info->data->has_mod_clk) {
		info->mod_clk = devm_clk_get(&pdev->dev, "mod");
		if (IS_ERR(info->mod_clk)) {
			ret = PTR_ERR(info->mod_clk);
			goto disable_bus_clk;
		}

		/* Running at 6MHz */
		ret = clk_set_rate(info->mod_clk, 4000000);
		if (ret)
			goto disable_bus_clk;

		ret = clk_prepare_enable(info->mod_clk);
		if (ret)
			goto disable_bus_clk;
	}

	if (IS_ENABLED(CONFIG_THERMAL_OF))
		info->sensor_device = &pdev->dev;

	return 0;

disable_bus_clk:
	if (info->data->has_bus_clk)
		clk_disable_unprepare(info->bus_clk);

assert_reset:
	if (info->data->has_bus_rst)
		reset_control_assert(info->reset);

	return ret;
}

static int sun4i_gpadc_probe_mfd(struct platform_device *pdev,
				 struct iio_dev *indio_dev)
{
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);
	struct sun4i_gpadc_dev *sun4i_gpadc_dev =
		dev_get_drvdata(pdev->dev.parent);
	int ret;

	info->no_irq = false;
	info->regmap = sun4i_gpadc_dev->regmap;

	indio_dev->num_channels = ARRAY_SIZE(sun4i_gpadc_channels);
	indio_dev->channels = sun4i_gpadc_channels;

	info->data = (struct gpadc_data *)platform_get_device_id(pdev)->driver_data;

	/*
	 * Since the controller needs to be in touchscreen mode for its thermal
	 * sensor to operate properly, and that switching between the two modes
	 * needs a delay, always registering in the thermal framework will
	 * significantly slow down the conversion rate of the ADCs.
	 *
	 * Therefore, instead of depending on THERMAL_OF in Kconfig, we only
	 * register the sensor if that option is enabled, eventually leaving
	 * that choice to the user.
	 */

	if (IS_ENABLED(CONFIG_THERMAL_OF)) {
		/*
		 * This driver is a child of an MFD which has a node in the DT
		 * but not its children, because of DT backward compatibility
		 * for A10, A13 and A31 SoCs. Therefore, the resulting devices
		 * of this driver do not have an of_node variable.
		 * However, its parent (the MFD driver) has an of_node variable
		 * and since devm_thermal_zone_of_sensor_register uses its first
		 * argument to match the phandle defined in the node of the
		 * thermal driver with the of_node of the device passed as first
		 * argument and the third argument to call ops from
		 * thermal_zone_of_device_ops, the solution is to use the parent
		 * device as first argument to match the phandle with its
		 * of_node, and the device from this driver as third argument to
		 * return the temperature.
		 */
		info->sensor_device = pdev->dev.parent;
	} else {
		indio_dev->num_channels =
			ARRAY_SIZE(sun4i_gpadc_channels_no_temp);
		indio_dev->channels = sun4i_gpadc_channels_no_temp;
	}

	if (IS_ENABLED(CONFIG_THERMAL_OF)) {
		ret = sun4i_irq_init(pdev, "TEMP_DATA_PENDING",
				     sun4i_gpadc_temp_data_irq_handler,
				     "temp_data", &info->temp_data_irq,
				     &info->ignore_temp_data_irq);
		if (ret < 0)
			return ret;
	}

	ret = sun4i_irq_init(pdev, "FIFO_DATA_PENDING",
			     sun4i_gpadc_fifo_data_irq_handler, "fifo_data",
			     &info->fifo_data_irq, &info->ignore_fifo_data_irq);
	if (ret < 0)
		return ret;

	if (IS_ENABLED(CONFIG_THERMAL_OF)) {
		ret = iio_map_array_register(indio_dev, sun4i_gpadc_hwmon_maps);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"failed to register iio map array\n");
			return ret;
		}
	}

	return 0;
}

static int sun4i_gpadc_probe(struct platform_device *pdev)
{
	struct sun4i_gpadc_iio *info;
	struct iio_dev *indio_dev;
	int ret, i;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*info));
	if (!indio_dev)
		return -ENOMEM;

	info = iio_priv(indio_dev);
	platform_set_drvdata(pdev, indio_dev);

	mutex_init(&info->mutex);
	info->indio_dev = indio_dev;
	init_completion(&info->completion);
	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->info = &sun4i_gpadc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;

	if (pdev->dev.of_node && of_match_node(sun4i_gpadc_of_id,
					       pdev->dev.of_node))
		ret = sun4i_gpadc_probe_dt(pdev, indio_dev);
	else
		ret = sun4i_gpadc_probe_mfd(pdev, indio_dev);

	if (ret)
		return ret;

	if (!info->data->support_irq) {
		pm_runtime_set_autosuspend_delay(&pdev->dev,
						 SUN4I_GPADC_AUTOSUSPEND_DELAY);
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
	}

	if (IS_ENABLED(CONFIG_THERMAL_OF)) {
		for (i = 0; i < info->data->sensor_count; i++) {
			info->sensor_id = i;
			info->tzd = thermal_zone_of_sensor_register(
					info->sensor_device,
					i, info, &sun4i_ts_tz_ops);
		}
		/*
		 * Do not fail driver probing when failing to register in
		 * thermal because no thermal DT node is found.
		 */
		if (IS_ERR(info->tzd) && PTR_ERR(info->tzd) != -ENODEV) {
			dev_err(&pdev->dev,
				"could not register thermal sensor: %ld\n",
				PTR_ERR(info->tzd));
			return PTR_ERR(info->tzd);
		}
	}

	if (info->data->support_irq)
		info->data->sample_start(info);

	ret = devm_iio_device_register(&pdev->dev, indio_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "could not register the device\n");
		goto err_map;
	}

	return 0;

err_map:
	if (!info->no_irq && IS_ENABLED(CONFIG_THERMAL_OF))
		iio_map_array_unregister(indio_dev);

	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	return ret;
}

static int sun4i_gpadc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct sun4i_gpadc_iio *info = iio_priv(indio_dev);

	pm_runtime_put(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	if (!IS_ENABLED(CONFIG_THERMAL_OF))
		return 0;

	if (info->data->support_irq)
		info->data->sample_end(info);

	thermal_zone_of_sensor_unregister(info->sensor_device, info->tzd);

	if (!info->no_irq)
		iio_map_array_unregister(indio_dev);

	if (info->data->has_mod_clk)
		clk_disable_unprepare(info->mod_clk);

	if (info->data->has_bus_clk)
		clk_disable_unprepare(info->bus_clk);

	if (info->data->has_bus_rst)
		reset_control_assert(info->reset);

	return 0;
}

static const struct platform_device_id sun4i_gpadc_id[] = {
	{ "sun4i-a10-gpadc-iio", (kernel_ulong_t)&sun4i_gpadc_data },
	{ "sun5i-a13-gpadc-iio", (kernel_ulong_t)&sun5i_gpadc_data },
	{ "sun6i-a31-gpadc-iio", (kernel_ulong_t)&sun6i_gpadc_data },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, sun4i_gpadc_id);

static struct platform_driver sun4i_gpadc_driver = {
	.driver = {
		.name = "sun4i-gpadc-iio",
		.of_match_table = sun4i_gpadc_of_id,
		.pm = &sun4i_gpadc_pm_ops,
	},
	.id_table = sun4i_gpadc_id,
	.probe = sun4i_gpadc_probe,
	.remove = sun4i_gpadc_remove,
};
MODULE_DEVICE_TABLE(of, sun4i_gpadc_of_id);

module_platform_driver(sun4i_gpadc_driver);

MODULE_DESCRIPTION("ADC driver for sunxi platforms");
MODULE_AUTHOR("Quentin Schulz <quentin.schulz@free-electrons.com>");
MODULE_LICENSE("GPL v2");
