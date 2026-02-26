// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner sunxi resistive touch panel controller driver
 *
 * Copyright (C) 2021 Allwinner Tech Co., Ltd
 * Author: Xu Minghui <xuminghuis@allwinnertech.com>
 *
 * Copyright (C) 2013 - 2014 Hans de Goede <hdegoede@redhat.com>
 *
 * The hwmon parts are based on work by Corentin LABBE which is:
 * Copyright (C) 2013 Corentin LABBE <clabbe.montjoie@gmail.com>
 */

/*
 * The sunxi-rtp controller is capable of detecting a second touch, but when a
 * second touch is present then the accuracy becomes so bad the reported touch
 * location is not useable.
 *
 * The original android driver contains some complicated heuristics using the
 * aprox. distance between the 2 touches to see if the user is making a pinch
 * open / close movement, and then reports emulated multi-touch events around
 * the last touch coordinate (as the dual-touch coordinates are worthless).
 *
 * These kinds of heuristics are just asking for trouble (and don't belong
 * in the kernel). So this driver offers straight forward, reliable single
 * touch functionality only.
 *
 */

//#define DEBUG /* Enable dev_dbg */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/reset.h>

#define RTP_CTRL0			0x00
#define RTP_CTRL1			0x04
#define RTP_CTRL2			0x08
#define RTP_CTRL3			0x0c
#define RTP_INT_FIFOC			0x10
#define RTP_INT_FIFOS			0x14
#define RTP_TPR				0x18
#define RTP_CDAT			0x1c
#define RTP_TEMP_DATA			0x20
#define RTP_DATA			0x24

/* RTP_CTRL0 bits */
#define ADC_FIRST_DLY(x)		((x) << 24) /* 8 bits */
#define ADC_FIRST_DLY_MODE(x)		((x) << 23)
#define ADC_CLK_SEL(x)			((x) << 22)
#define ADC_CLK_DIV(x)			((x) << 20) /* 3 bits */
#define FS_DIV(x)			((x) << 16) /* 4 bits */
#define T_ACQ(x)			((x) << 0) /* 16 bits */
#define T_ACQ_DEFAULT			(0xffff)

/* RTP_CTRL0 bits select */
#define ADC_FIRST_DLY_SELECT		(0xf)
#define ADC_FIRST_DLY_MODE_SELECT	(0)
#define ADC_CLK_SELECT			(0)
#define ADC_CLK_DIV_SELECT		(0)
#define FS_DIV_SELECT			(9)
#define T_ACQ_SELECT			(0xf)

/* RTP_CTRL1 bits */
#define STYLUS_UP_DEBOUN(x)		((x) << 12) /* 8 bits */
#define STYLUS_UP_DEBOUN_EN(x)		((x) << 9)
#define STYLUS_UP_DEBOUN_SELECT		(0xf)
/* the other bits is written in compatible .data */

/* RTP_CTRL2 bits */
#define RTP_SENSITIVE_ADJUST(x)		((x) << 28) /* 4 bits */
#define RTP_MODE_SELECT(x)		((x) << 26) /* 2 bits */
#define PRE_MEA_EN(x)			((x) << 24)
#define PRE_MEA_THRE_CNT(x)		((x) << 0) /* 24 bits */
#define PRE_MEA_THRE_CNT_SELECT		(0xffffff)

/* RTP_CTRL3 bits */
#define FILTER_EN(x)			((x) << 2)
#define FILTER_TYPE(x)			((x) << 0)  /* 2 bits */

/* RTP_INT_FIFOC irq and fifo mask / control bits */
#define TEMP_IRQ_EN(x)			((x) << 18)
#define OVERRUN_IRQ_EN(x)		((x) << 17)
#define DATA_IRQ_EN(x)			((x) << 16)
#define RTP_DATA_XY_CHANGE(x)		((x) << 13)
#define FIFO_TRIG(x)			((x) << 8)  /* 5 bits */
#define DATA_DRQ_EN(x)			((x) << 7)
#define FIFO_FLUSH(x)			((x) << 4)
#define RTP_UP_IRQ_EN(x)		((x) << 1)
#define RTP_DOWN_IRQ_EN(x)		((x) << 0)

/* RTP_INT_FIFOS irq and fifo status bits */
#define TEMP_DATA_PENDING		BIT(18)
#define FIFO_OVERRUN_PENDING		BIT(17)
#define FIFO_DATA_PENDING		BIT(16)
#define RTP_IDLE_FLG			BIT(2)
#define RTP_UP_PENDING			BIT(1)
#define RTP_DOWN_PENDING		BIT(0)

#define	RTP_VENDOR			(0x0001)
#define	RTP_PRODUCT			(0x0001)
#define	RTP_VERSION			(0x0100)

/* Registers which needs to be saved and restored before and after sleeping */
u32 sunxi_rtp_regs_offset[] = {
	RTP_INT_FIFOC,
	RTP_TPR,
};

struct sunxi_rtp_hwdata {
	u32 chopper_en_bitofs;
	u32 touch_pan_cali_en_bitofs;
	u32 tp_dual_en_bitofs;
	u32 tp_mode_en_bitofs;
	u32 tp_adc_select_bitofs;
	bool has_clock;  /* flags for the existence of clock and reset resources */
};

struct sunxi_rtp_config {
	u32 tp_sensitive_adjust;	/* tpadc sensitive parameter, from 0000(least sensitive) to 1111(most sensitive) */

	/* tpadc filter type, eg:(Median Filter Size/Averaging Filter Size)
	 * 00(4/2), 01(5/3), 10(8/4), 11(16/8)
	 */
	u32 filter_type;
};

struct sunxi_rtp {
	struct platform_device *pdev;
	struct device *dev;
	struct input_dev *input;
	void __iomem *base;
	int irq;
	struct clk *bus_clk;
	struct clk *mod_clk;
	struct reset_control *reset;
	struct sunxi_rtp_hwdata *hwdata;  /* to distinguish platform own register */
	struct sunxi_rtp_config rtp_config;
	u32 regs_backup[ARRAY_SIZE(sunxi_rtp_regs_offset)];
};

static struct sunxi_rtp_hwdata sunxi_rtp_hwdata_v100 = {
	.chopper_en_bitofs        = 8,
	.touch_pan_cali_en_bitofs = 7,
	.tp_dual_en_bitofs        = 6,
	.tp_mode_en_bitofs        = 5,
	.tp_adc_select_bitofs     = 4,
	.has_clock = false,
};

static struct sunxi_rtp_hwdata sunxi_rtp_hwdata_v101 = {
	.chopper_en_bitofs        = 8,
	.touch_pan_cali_en_bitofs = 7,
	.tp_dual_en_bitofs        = 6,
	.tp_mode_en_bitofs        = 5,
	.tp_adc_select_bitofs     = 4,
	.has_clock = true,
};

static const struct of_device_id sunxi_rtp_of_match[] = {
	{ .compatible = "allwinner,sunxi-rtp-v100", .data = &sunxi_rtp_hwdata_v100},
	{ .compatible = "allwinner,sunxi-rtp-v101", .data = &sunxi_rtp_hwdata_v101},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_rtp_of_match);

static int sunxi_rtp_clk_get(struct sunxi_rtp *chip)
{
	struct device *dev = chip->dev;

	chip->reset = devm_reset_control_get(chip->dev, NULL);
	if (IS_ERR(chip->reset)) {
		dev_err(dev, "get tpadc reset failed\n");
		return PTR_ERR(chip->reset);
	}

	chip->mod_clk = devm_clk_get(chip->dev, "mod");
	if (IS_ERR(chip->mod_clk)) {
		dev_err(dev, "get tpadc mode clock failed\n");
		return PTR_ERR(chip->mod_clk);
	}

	chip->bus_clk = devm_clk_get(chip->dev, "bus");
	if (IS_ERR(chip->bus_clk)) {
		dev_err(dev, "get tpadc bus clock failed\n");
		return PTR_ERR(chip->bus_clk);
	}

	return 0;
}

static int sunxi_rtp_clk_enable(struct sunxi_rtp *chip)
{
	int err;
	struct device *dev = chip->dev;

	err = reset_control_reset(chip->reset);
	if (err) {
		dev_err(dev, "reset_control_reset() failed\n");
		goto err0;
	}

	err = clk_prepare_enable(chip->mod_clk);
	if (err) {
		dev_err(dev, "Cannot enable rtp->mod_clk\n");
		goto err1;
	}

	err = clk_prepare_enable(chip->bus_clk);
	if (err) {
		dev_err(dev, "Cannot enable rtp->bus_clk\n");
		goto err2;
	}

	return 0;

err2:
	clk_disable_unprepare(chip->mod_clk);
err1:
	reset_control_assert(chip->reset);
err0:
	return err;
}

static void sunxi_rtp_clk_disable(struct sunxi_rtp *chip)
{
	clk_disable_unprepare(chip->bus_clk);
	clk_disable_unprepare(chip->mod_clk);
	reset_control_assert(chip->reset);
}

static void sunxi_rtp_irq_enable(struct sunxi_rtp *chip)
{
	u32 reg_val;

	/* Active input IRQs */
	reg_val = readl(chip->base + RTP_INT_FIFOC);
	reg_val |= TEMP_IRQ_EN(1) | DATA_IRQ_EN(1) | FIFO_TRIG(1) | FIFO_FLUSH(1) | RTP_UP_IRQ_EN(1) |
		OVERRUN_IRQ_EN(1) | DATA_DRQ_EN(1) | RTP_DOWN_IRQ_EN(1);
	writel(reg_val, chip->base + RTP_INT_FIFOC);
}

static void sunxi_rtp_irq_disable(struct sunxi_rtp *chip)
{
	writel(0, chip->base + RTP_INT_FIFOC);
}

static irqreturn_t sunxi_rtp_irq_handler(int irq, void *dev_id)
{
	struct sunxi_rtp *chip = dev_id;
	struct device *dev = chip->dev;
	u32 x, y, reg_val;
	static bool ignore_first_packet = true;

	/* Read irq flags */
	reg_val = readl(chip->base + RTP_INT_FIFOS);

	sunxi_rtp_irq_disable(chip);

	if (reg_val & FIFO_DATA_PENDING) {
		dev_dbg(dev, "sunxi-rtp fifo data pending\n");

		/* The 1st location reported after an up event is unreliable */
		if (!ignore_first_packet) {
			dev_dbg(dev, "sunxi-rtp report fifo data\n");
			x = readl(chip->base + RTP_DATA);
			y = readl(chip->base + RTP_DATA);
			input_report_abs(chip->input, ABS_X, x);
			input_report_abs(chip->input, ABS_Y, y);
			/*
			 * The hardware has a separate down status bit, but
			 * that gets set before we get the first location,
			 * resulting in reporting a click on the old location.
			 */
			input_report_key(chip->input, BTN_TOUCH, 1);  /* 1: report touch down event, 0: report touch up event */
			input_sync(chip->input);
		} else {
			dev_dbg(dev, "sunxi-rtp ignore first fifo data\n");
			ignore_first_packet = false;
		}
	}

	if (reg_val & RTP_UP_PENDING) {
		dev_dbg(dev, "sunxi-rtp up pending\n");
		ignore_first_packet = true;
		input_report_key(chip->input, BTN_TOUCH, 0);
		input_sync(chip->input);
	}

	/* Clear irq flags */
	writel(reg_val, chip->base + RTP_INT_FIFOS);

	sunxi_rtp_irq_enable(chip);
	return IRQ_HANDLED;
}

static int sunxi_rtp_open(struct input_dev *dev)
{
	struct sunxi_rtp *chip = input_get_drvdata(dev);

	/* Active input IRQs */
	sunxi_rtp_irq_enable(chip);

	return 0;
}

static void sunxi_rtp_close(struct input_dev *dev)
{
	struct sunxi_rtp *chip = input_get_drvdata(dev);

	/* Deactive input IRQs */
	sunxi_rtp_irq_disable(chip);
}

static int sunxi_rtp_resource_get(struct sunxi_rtp *chip)
{
	const struct of_device_id *of_id;
	int err;

	of_id = of_match_device(sunxi_rtp_of_match, chip->dev);
	if (!of_id) {
		dev_err(chip->dev, "of_match_device() failed\n");
		return -EINVAL;
	}
	chip->hwdata = (struct sunxi_rtp_hwdata *)(of_id->data);

	chip->base = devm_platform_ioremap_resource(chip->pdev, 0);
	if (IS_ERR(chip->base)) {
		dev_err(chip->dev, "Fail to map IO resource\n");
		return PTR_ERR(chip->base);
	}

	/* If there are clock resources, has_clock is true, apply for clock resources;
	 * otherwise, has_clock is false, you do not need to apply for clock resources.
	 */
	if (chip->hwdata->has_clock) {
		err = sunxi_rtp_clk_get(chip);
		if (err) {
			dev_err(chip->dev, "sunxi rtp get clock failed\n");
			return err;
		}
	}

	chip->irq = platform_get_irq(chip->pdev, 0);
	err = devm_request_irq(chip->dev, chip->irq, sunxi_rtp_irq_handler, 0, "sunxi-rtp", chip);
	if (err) {
		dev_err(chip->dev, "tpadc request irq failed\n");
		return err;
	}

	return 0;
}

static void sunxi_rtp_resource_put(struct sunxi_rtp *chip)
{
	/* TODO:
	 * If there is an operation that needs to be released later, you can add it directly.
	 */
}

static int sunxi_rtp_inputdev_register(struct sunxi_rtp *chip)
{
	int err;

	chip->input = devm_input_allocate_device(chip->dev);
	if (!chip->input) {
		dev_err(chip->dev, "devm_input_allocate_device() failed\n");
		return -ENOMEM;
	}

	chip->input->name = "sunxi-rtp";
	chip->input->phys = "sunxi_rtp/input0";
	chip->input->open = sunxi_rtp_open;
	chip->input->close = sunxi_rtp_close;
	chip->input->id.bustype = BUS_HOST;
	chip->input->id.vendor = RTP_VENDOR;
	chip->input->id.product = RTP_PRODUCT;
	chip->input->id.version = RTP_VERSION;
	chip->input->evbit[0] =  BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	__set_bit(BTN_TOUCH, chip->input->keybit);
	input_set_abs_params(chip->input, ABS_X, 0, 4095, 0, 0);
	input_set_abs_params(chip->input, ABS_Y, 0, 4095, 0, 0);
	input_set_drvdata(chip->input, chip);

	err = input_register_device(chip->input);
	if (err) {
		dev_err(chip->dev, "sunxi rtp register input device failed\n");
		return err;
	}

	return 0;
}

static void sunxi_rtp_inputdev_unregister(struct sunxi_rtp *chip)
{
	input_unregister_device(chip->input);
}

static void sunxi_rtp_calibrate(struct sunxi_rtp *chip)
{
	u32 reg_val;

	reg_val = readl(chip->base + RTP_CTRL0);
	writel(reg_val | T_ACQ_DEFAULT, chip->base + RTP_CTRL0);

	reg_val = readl(chip->base + RTP_CTRL1);
	writel(reg_val | BIT(chip->hwdata->touch_pan_cali_en_bitofs), chip->base + RTP_CTRL1);
}

static void sunxi_rtp_reg_setup(struct sunxi_rtp *chip)
{
	u32 reg_val;
	struct device_node *np = chip->dev->of_node;
	struct sunxi_rtp_config *rtp_config = &chip->rtp_config;
	rtp_config->tp_sensitive_adjust = 15;
	rtp_config->filter_type = 1;

	sunxi_rtp_calibrate(chip);

	writel(ADC_FIRST_DLY(ADC_FIRST_DLY_SELECT) | ADC_FIRST_DLY_MODE(ADC_FIRST_DLY_MODE_SELECT) |
			ADC_CLK_DIV(ADC_CLK_DIV_SELECT) | FS_DIV(FS_DIV_SELECT) | T_ACQ(T_ACQ_SELECT),
			chip->base + RTP_CTRL0);

	/*
	 * tp_sensitive_adjust is an optional property
	 * tp_mode = 0 : only x and y coordinates, as we don't use dual touch
	 */
	of_property_read_u32(np, "allwinner,tp-sensitive-adjust",
			     &rtp_config->tp_sensitive_adjust);
	writel(RTP_SENSITIVE_ADJUST(rtp_config->tp_sensitive_adjust) | RTP_MODE_SELECT(0),
	       chip->base + RTP_CTRL2);

	reg_val = readl(chip->base + RTP_CTRL2);
	writel(reg_val | PRE_MEA_THRE_CNT(PRE_MEA_THRE_CNT_SELECT), chip->base + RTP_CTRL2);

	/*
	 * Enable median and averaging filter, optional property for
	 * filter type.
	 */
	of_property_read_u32(np, "allwinner,filter-type", &rtp_config->filter_type);
	writel(FILTER_EN(1) | FILTER_TYPE(rtp_config->filter_type), chip->base + RTP_CTRL3);

	/*
	 * Set stylus up debounce to aprox 10 ms, enable debounce, and
	 * finally enable tp mode.
	 */
	reg_val = STYLUS_UP_DEBOUN(STYLUS_UP_DEBOUN_SELECT) | STYLUS_UP_DEBOUN_EN(1)
		| BIT(chip->hwdata->chopper_en_bitofs) | BIT(chip->hwdata->tp_mode_en_bitofs);

	writel(reg_val, chip->base + RTP_CTRL1);
}

static void sunxi_rtp_reg_destroy(struct sunxi_rtp *chip)
{
	/* TODO:
	 * If there is an operation that needs to be released later, you can add it directly.
	 */
}

static int sunxi_rtp_hw_init(struct sunxi_rtp *chip)
{
	int err;

	if (chip->hwdata->has_clock) {
		err = sunxi_rtp_clk_enable(chip);
		if (err) {
			dev_err(chip->dev, "enable rtp clock failed\n");
			return err;
		}
	}

	sunxi_rtp_reg_setup(chip);

	return 0;
}

static void sunxi_rtp_hw_exit(struct sunxi_rtp *chip)
{
	sunxi_rtp_reg_destroy(chip);

	if (chip->hwdata->has_clock)
		sunxi_rtp_clk_disable(chip);
}

static int sunxi_rtp_probe(struct platform_device *pdev)
{
	struct sunxi_rtp *chip;
	int err;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		err = -ENOMEM;
		goto err0;
	}

	chip->pdev = pdev;
	chip->dev = &pdev->dev;

	err = sunxi_rtp_resource_get(chip);
	if (err) {
		dev_err(chip->dev, "sunxi rtp get resource failed\n");
		goto err0;
	}

	err = sunxi_rtp_inputdev_register(chip);
	if (err) {
		dev_err(chip->dev, "sunxi rtp inputdev register failed\n");
		goto err1;
	}

	err = sunxi_rtp_hw_init(chip);
	if (err) {
		dev_err(chip->dev, "sunxi rtp hw_init failed\n");
		goto err2;
	}

	platform_set_drvdata(pdev, chip);
	dev_dbg(chip->dev, "sunxi rtp probe success\n");
	return 0;

err2:
	sunxi_rtp_inputdev_unregister(chip);
err1:
	sunxi_rtp_resource_put(chip);
err0:
	return err;
}

static int sunxi_rtp_remove(struct platform_device *pdev)
{
	struct sunxi_rtp *chip = platform_get_drvdata(pdev);

	sunxi_rtp_hw_exit(chip);
	sunxi_rtp_inputdev_unregister(chip);
	sunxi_rtp_resource_put(chip);

	return 0;
}

#ifdef CONFIG_PM
static inline void sunxi_rtp_save_regs(struct sunxi_rtp *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_rtp_regs_offset); i++)
		chip->regs_backup[i] = readl(chip->base + sunxi_rtp_regs_offset[i]);
}

static inline void sunxi_rtp_restore_regs(struct sunxi_rtp *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_rtp_regs_offset); i++)
		writel(chip->regs_backup[i], chip->base + sunxi_rtp_regs_offset[i]);
}

static int sunxi_rtp_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtp *chip = platform_get_drvdata(pdev);

	disable_irq(chip->irq);
	sunxi_rtp_save_regs(chip);
	if (chip->hwdata->has_clock)
		sunxi_rtp_clk_disable(chip);

	return 0;
}

static int sunxi_rtp_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_rtp *chip = platform_get_drvdata(pdev);
	int err;

	if (chip->hwdata->has_clock) {
		err = sunxi_rtp_clk_enable(chip);
		if (err) {
			dev_err(chip->dev, "resume: enable rtp clock failed\n");
			return err;
		}
	}
	sunxi_rtp_restore_regs(chip);
	enable_irq(chip->irq);

	return 0;
}

static const struct dev_pm_ops sunxi_rtp_dev_pm_ops = {
	.suspend_noirq = sunxi_rtp_suspend_noirq,
	.resume_noirq = sunxi_rtp_resume_noirq,
};
#define SUNXI_RTP_DEV_PM_OPS (&sunxi_rtp_dev_pm_ops)
#else
#define SUNXI_RTP_DEV_PM_OPS NULL
#endif

static struct platform_driver sunxi_rtp_driver = {
	.driver = {
		.name	= "sunxi-rtp",
		.of_match_table = of_match_ptr(sunxi_rtp_of_match),
		.pm = SUNXI_RTP_DEV_PM_OPS,
	},
	.probe	= sunxi_rtp_probe,
	.remove	= sunxi_rtp_remove,
};

module_platform_driver(sunxi_rtp_driver);

MODULE_DESCRIPTION("Allwinner sunxi resistive touch panel controller driver");
MODULE_AUTHOR("Xu Minghui<Xuminghuis@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("2.0.1");
