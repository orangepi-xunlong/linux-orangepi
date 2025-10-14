// SPDX-License-Identifier: GPL-2.0-only
/*
 *
 * SKY1 Audio SS Reset driver
 *
 */

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/regmap.h>
#include <dt-bindings/reset/sky1-reset-audss.h>

#define SKY1_RESET_SLEEP_MIN_US		10000
#define SKY1_RESET_SLEEP_MAX_US		20000

struct sky1_rst_signal {
	unsigned int offset, bit;
};

struct sky1_rst_variant {
	const struct sky1_rst_signal *signals;
	unsigned int signals_num;
	struct reset_control_ops ops;
};

struct sky1_rst {
	struct reset_controller_dev rcdev;
	struct regmap *regmap;
	const struct sky1_rst_signal *signals;
};

enum sky1_rst_audss_registers {
	AUDSS_SW_RST			= 0x78,
};

static const struct sky1_rst_signal sky1_rst_audss_signals[SKY1_AUDSS_SW_RESET_NUM] = {
	[AUDSS_I2S0_SW_RST_N]	= { AUDSS_SW_RST, BIT(0) },
	[AUDSS_I2S1_SW_RST_N]	= { AUDSS_SW_RST, BIT(1) },
	[AUDSS_I2S2_SW_RST_N]	= { AUDSS_SW_RST, BIT(2) },
	[AUDSS_I2S3_SW_RST_N]	= { AUDSS_SW_RST, BIT(3) },
	[AUDSS_I2S4_SW_RST_N]	= { AUDSS_SW_RST, BIT(4) },
	[AUDSS_I2S5_SW_RST_N]	= { AUDSS_SW_RST, BIT(5) },
	[AUDSS_I2S6_SW_RST_N]	= { AUDSS_SW_RST, BIT(6) },
	[AUDSS_I2S7_SW_RST_N]	= { AUDSS_SW_RST, BIT(7) },
	[AUDSS_I2S8_SW_RST_N]	= { AUDSS_SW_RST, BIT(8) },
	[AUDSS_I2S9_SW_RST_N]	= { AUDSS_SW_RST, BIT(9) },
	[AUDSS_WDT_SW_RST_N]	= { AUDSS_SW_RST, BIT(10) },
	[AUDSS_TIMER_SW_RST_N]	= { AUDSS_SW_RST, BIT(11) },
	[AUDSS_MB0_SW_RST_N]	= { AUDSS_SW_RST, BIT(12) },
	[AUDSS_MB1_SW_RST_N]	= { AUDSS_SW_RST, BIT(13) },
	[AUDSS_HDA_SW_RST_N]	= { AUDSS_SW_RST, BIT(14) },
	[AUDSS_DMAC_SW_RST_N]	= { AUDSS_SW_RST, BIT(15) },
};

static struct sky1_rst *to_sky1_rst(struct reset_controller_dev *rcdev)
{
	return container_of(rcdev, struct sky1_rst, rcdev);
}

static int sky1_audss_reset_update(struct sky1_rst *sky1rst,
				   unsigned long id, unsigned int value)
{
	const struct sky1_rst_signal *signal = &sky1rst->signals[id];

	return regmap_update_bits(sky1rst->regmap,
				  signal->offset, signal->bit, value);
}

static int sky1_audss_reset_set(struct reset_controller_dev *rcdev,
				unsigned long id, bool assert)
{
	struct sky1_rst *sky1rst = to_sky1_rst(rcdev);
	const unsigned int bit = sky1rst->signals[id].bit;
	unsigned int value = assert ? 0 : bit;

	return sky1_audss_reset_update(sky1rst, id, value);
}

static int sky1_audss_reset(struct reset_controller_dev *rcdev,
			    unsigned long id)
{
	sky1_audss_reset_set(rcdev, id, true);
	usleep_range(SKY1_RESET_SLEEP_MIN_US,
		     SKY1_RESET_SLEEP_MAX_US);

	sky1_audss_reset_set(rcdev, id, false);

	/*
	 * Ensure component is taken out reset state by sleeping also after
	 * deasserting the reset, Otherwise, the component may not be ready
	 * for operation.
	 */
	usleep_range(SKY1_RESET_SLEEP_MIN_US,
		     SKY1_RESET_SLEEP_MAX_US);
	return 0;
}

static int sky1_audss_reset_assert(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	return sky1_audss_reset_set(rcdev, id, true);
}

static int sky1_audss_reset_deassert(struct reset_controller_dev *rcdev,
				     unsigned long id)
{
	return sky1_audss_reset_set(rcdev, id, false);
}

static int sky1_audss_reset_status(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	struct sky1_rst *sky1_rst = to_sky1_rst(rcdev);
	const struct sky1_rst_signal *signal = &sky1_rst->signals[id];
	unsigned int value = 0;

	regmap_read(sky1_rst->regmap, signal->offset, &value);

	return !(value & signal->bit);
}

static const struct sky1_rst_variant variant_sky1_audss = {
	.signals = sky1_rst_audss_signals,
	.signals_num = ARRAY_SIZE(sky1_rst_audss_signals),
	.ops = {
		.reset	  = sky1_audss_reset,
		.assert   = sky1_audss_reset_assert,
		.deassert = sky1_audss_reset_deassert,
		.status	  = sky1_audss_reset_status,
	},
};

static int sky1_audss_reset_probe(struct platform_device *pdev)
{
	struct sky1_rst *sky1rst;
	struct device *dev = &pdev->dev;
	struct device_node *parent_np;
	struct regmap *regmap_cru;
	const struct sky1_rst_variant *variant = ACPI_COMPANION(dev) ?
		acpi_device_get_match_data(dev) : of_device_get_match_data(dev);

	parent_np = of_get_parent(pdev->dev.of_node);
	regmap_cru = syscon_node_to_regmap(parent_np);
	of_node_put(parent_np);

	if (IS_ERR_OR_NULL(regmap_cru))
		regmap_cru = device_syscon_regmap_lookup_by_property(dev,
					"audss_cru");
	if (IS_ERR_OR_NULL(regmap_cru))
		return -EINVAL;

	sky1rst = devm_kzalloc(dev, sizeof(*sky1rst), GFP_KERNEL);
	if (!sky1rst)
		return -ENOMEM;

	sky1rst->signals = variant->signals;
	sky1rst->regmap = regmap_cru;
	sky1rst->rcdev.owner     = THIS_MODULE;
	sky1rst->rcdev.nr_resets = variant->signals_num;
	sky1rst->rcdev.ops       = &variant->ops;
	sky1rst->rcdev.of_node   = dev->of_node;
	sky1rst->rcdev.dev       = dev;

	return devm_reset_controller_register(dev, &sky1rst->rcdev);
}

static const struct of_device_id sky1_audss_reset_dt_ids[] = {
	{ .compatible = "cix,sky1-audss-reset", .data = &variant_sky1_audss },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sky1_audss_reset_dt_ids);

static const struct acpi_device_id sky1_audss_reset_acpi_match[] = {
	{ "CIXH6062", .driver_data = (kernel_ulong_t)&variant_sky1_audss },
	{},
};
MODULE_DEVICE_TABLE(acpi, sky1_audss_reset_acpi_match);

static struct platform_driver sky1_audss_reset_driver = {
	.probe = sky1_audss_reset_probe,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = sky1_audss_reset_dt_ids,
		.acpi_match_table = ACPI_PTR(sky1_audss_reset_acpi_match),
	},
};
module_platform_driver(sky1_audss_reset_driver);

MODULE_AUTHOR("Joakim Zhang <joakim.zhang@cixtech.com>");
MODULE_DESCRIPTION("Cix Sky1 audss reset driver");
MODULE_LICENSE("GPL v2");
