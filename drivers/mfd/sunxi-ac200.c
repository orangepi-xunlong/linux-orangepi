// SPDX-License-Identifier: GPL-2.0-only
/*
 * MFD core driver for X-Powers' AC200 IC
 *
 * The AC200 is a chip which is co-packaged with Allwinner H6 SoC and
 * includes analog audio codec, analog TV encoder, ethernet PHY, eFuse
 * and RTC.
 *
 * Copyright (c) 2020 Jernej Skrabec <jernej.skrabec@siol.net>
 */

#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/core.h>
#include <linux/mfd/ac200.h>
#include <linux/module.h>
#include <linux/of.h>

/* Interrupts */
#define AC200_IRQ_RTC  0
#define AC200_IRQ_EPHY 1
#define AC200_IRQ_TVE  2

/* IRQ enable register */
#define AC200_SYS_IRQ_ENABLE_OUT_EN BIT(15)
#define AC200_SYS_IRQ_ENABLE_RTC    BIT(12)
#define AC200_SYS_IRQ_ENABLE_EPHY   BIT(8)
#define AC200_SYS_IRQ_ENABLE_TVE    BIT(4)

static const struct regmap_range_cfg ac200_range_cfg[] = {
	{
		.range_min = AC200_SYS_VERSION,
		.range_max = AC200_IC_CHARA1,
		.selector_reg = AC200_TWI_REG_ADDR_H,
		.selector_mask = 0xff,
		.selector_shift = 0,
		.window_start = 0,
		.window_len = 256,
	}
};

static const struct regmap_config ac200_regmap_config = {
	.name = "ac200",
	.reg_bits	= 8,
	.val_bits	= 16,
	.ranges		= ac200_range_cfg,
	.num_ranges	= ARRAY_SIZE(ac200_range_cfg),
	.max_register	= AC200_IC_CHARA1,
};

static const struct regmap_irq ac200_regmap_irqs[] = {
	REGMAP_IRQ_REG(AC200_IRQ_RTC,  0, AC200_SYS_IRQ_ENABLE_RTC),
	REGMAP_IRQ_REG(AC200_IRQ_EPHY, 0, AC200_SYS_IRQ_ENABLE_EPHY),
	REGMAP_IRQ_REG(AC200_IRQ_TVE,  0, AC200_SYS_IRQ_ENABLE_TVE),
};

static const struct regmap_irq_chip ac200_regmap_irq_chip = {
	.name			= "ac200_irq_chip",
	.status_base		= AC200_SYS_IRQ_STATUS,
	.mask_base		= AC200_SYS_IRQ_ENABLE,
	.mask_invert		= true,
	.irqs			= ac200_regmap_irqs,
	.num_irqs		= ARRAY_SIZE(ac200_regmap_irqs),
	.num_regs		= 1,
};

static const struct resource ephy_resource[] = {
	DEFINE_RES_IRQ(AC200_IRQ_EPHY),
};

static const struct mfd_cell ac200_cells[] = {
	{
		.name		= "ac200-ephy",
		.num_resources	= ARRAY_SIZE(ephy_resource),
		.resources	= ephy_resource,
		.of_compatible	= "x-powers,ac200-ephy",
	},
	{
		.name = "acx00-codec",
		.of_compatible	= "x-powers,ac200-codec",
	},
};

static int ac200_i2c_probe(struct i2c_client *i2c,
			   const struct i2c_device_id *id)
{
	struct device *dev = &i2c->dev;
	struct ac200_dev *ac200;
	int ret;

	ac200 = devm_kzalloc(dev, sizeof(*ac200), GFP_KERNEL);
	if (!ac200)
		return -ENOMEM;

	i2c_set_clientdata(i2c, ac200);

	ac200->regmap = devm_regmap_init_i2c(i2c, &ac200_regmap_config);
	if (IS_ERR(ac200->regmap)) {
		ret = PTR_ERR(ac200->regmap);
		dev_err(dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	ac200->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(ac200->clk)) {
	      dev_err(dev, "Can't obtain the clock!\n");
	      return PTR_ERR(ac200->clk);
	}

	ret = clk_prepare_enable(ac200->clk);
	if (ret)
	      return ret;

	/* do a reset to put chip in a known state */
	ret = regmap_write(ac200->regmap, AC200_SYS_CONTROL, 0);
	if (ret)
		return ret;

	ret = regmap_write(ac200->regmap, AC200_SYS_CONTROL, 1);
	if (ret)
		return ret;

	/* enable interrupt pin */

	ret = regmap_write(ac200->regmap, AC200_SYS_IRQ_ENABLE,
			   AC200_SYS_IRQ_ENABLE_OUT_EN);
	if (ret)
		return ret;

	ret = regmap_add_irq_chip(ac200->regmap, i2c->irq, IRQF_ONESHOT, 0,
				  &ac200_regmap_irq_chip, &ac200->regmap_irqc);
	if (ret)
		return ret;

	ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE, ac200_cells,
				   ARRAY_SIZE(ac200_cells), NULL, 0, NULL);
	if (ret) {
		dev_err(dev, "failed to add MFD devices: %d\n", ret);
		regmap_del_irq_chip(i2c->irq, ac200->regmap_irqc);
		return ret;
	}

	return 0;
}

static int ac200_i2c_remove(struct i2c_client *i2c)
{
	struct ac200_dev *ac200 = i2c_get_clientdata(i2c);

	regmap_write(ac200->regmap, AC200_SYS_CONTROL, 0);

	mfd_remove_devices(&i2c->dev);
	regmap_del_irq_chip(i2c->irq, ac200->regmap_irqc);

	return 0;
}

static const struct i2c_device_id ac200_ids[] = {
	{ "ac200", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, ac200_ids);

static const struct of_device_id ac200_of_match[] = {
	{ .compatible = "x-powers,ac200" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ac200_of_match);

static struct i2c_driver ac200_i2c_driver = {
	.driver = {
		.name	= "ac200",
		.of_match_table	= of_match_ptr(ac200_of_match),
	},
	.probe	= ac200_i2c_probe,
	.remove = ac200_i2c_remove,
	.id_table = ac200_ids,
};
module_i2c_driver(ac200_i2c_driver);

MODULE_DESCRIPTION("MFD core driver for AC200");
MODULE_AUTHOR("Jernej Skrabec <jernej.skrabec@siol.net>");
MODULE_LICENSE("GPL v2");
