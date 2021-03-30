#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/power/aw_pm.h>
#include "../axp-core.h"
#include "axp80.h"

static struct axp_dev *axp80_pm_power;
struct axp_config_info axp80_config;
struct wakeup_source *axp80_ws;
static int axp80_pmu_num;

static struct axp_regmap_irq_chip axp80_regmap_irq_chip = {
	.name        = "axp80_irq_chip",
	.status_base = AXP80_INTSTS1,
	.enable_base = AXP80_INTEN1,
	.num_regs    = 2,
};

static struct resource axp80_pek_resources[] = {
	{AXP80_IRQ_PEKRE, AXP80_IRQ_PEKRE, "PEK_DBR",   IORESOURCE_IRQ,},
	{AXP80_IRQ_PEKFE, AXP80_IRQ_PEKFE, "PEK_DBF",   IORESOURCE_IRQ,},
};

static struct mfd_cell axp80_cells[] = {
	{
		.name          = "axp80-powerkey",
		.num_resources = ARRAY_SIZE(axp80_pek_resources),
		.resources     = axp80_pek_resources,
	},
	{
		.name          = "axp80-regulator",
	},
};

void axp80_power_off(void)
{
	pr_info("[axp] send power-off command!\n");
	axp_regmap_write(axp80_pm_power->regmap, AXP80_OFF_CTL,
					 0x80);
}

static int axp80_init_chip(struct axp_dev *axp80)
{
	uint8_t chip_id;
	int err;

	err = axp_regmap_read(axp80->regmap, AXP80_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[%s] try to read chip id failed!\n",
				axp_name[axp80_pmu_num]);
		return err;
	}

	if (((((chip_id & 0xc0) >> 6) << 4) | (chip_id & 0xf)) == 0x10)
		pr_info("[%s] chip id detect 0x%x !\n",
				axp_name[axp80_pmu_num],  chip_id);
	else
		pr_info("[%s] chip id not detect 0x%x !\n",
				axp_name[axp80_pmu_num], chip_id);

	/*init irq wakeup en*/
	if (axp80_config.pmu_irq_wakeup)
		axp_regmap_set_bits(axp80->regmap,
				AXP80_IFQ_PWROK_SET, 0x80);
	else
		axp_regmap_clr_bits(axp80->regmap,
				AXP80_IFQ_PWROK_SET, 0x80);

	/*init pmu over temperature protection*/
	if (axp80_config.pmu_hot_shutdown)
		axp_regmap_set_bits(axp80->regmap, AXP80_OFF_CTL, 0x02);
	else
		axp_regmap_clr_bits(axp80->regmap, AXP80_OFF_CTL, 0x02);

	return 0;
}

static void axp80_wakeup_event(void)
{
	__pm_wakeup_event(axp80_ws, 0);
}

static s32 axp80_usb_det(void)
{
	return 0;
}

static s32 axp80_usb_vbus_output(int high)
{
	return 0;
}

static int axp80_cfg_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
{
	char name[8];
	struct device_node *np;

	sprintf(name, "pmu%d", num);

	np = of_find_node_by_type(NULL, name);
	if (NULL == np) {
		pr_err("can not find device_type for %s\n", name);
		return -1;
	}

	if (!of_device_is_available(np)) {
		pr_err("can not find node for %s\n", name);
		return -1;
	}

	api->pmu_arg.twi_port = axp80_pm_power->regmap->client->adapter->nr;
	api->pmu_arg.dev_addr = axp80_pm_power->regmap->client->addr;
	*pmu_id = axp80_config.pmu_id;

	return 0;
}

static const char *axp80_get_pmu_name(void)
{
	return axp_name[axp80_pmu_num];
}

static struct axp_dev *axp80_get_pmu_dev(void)
{
	return axp80_pm_power;
}

static struct axp_platform_ops axp80_platform_ops = {
	.usb_det = axp80_usb_det,
	.usb_vbus_output = axp80_usb_vbus_output,
	.cfg_pmux_para = axp80_cfg_pmux_para,
	.get_pmu_name = axp80_get_pmu_name,
	.get_pmu_dev  = axp80_get_pmu_dev,
	.powerkey_name = {
		"axp806-powerkey"
	},
	.regulator_name = {
		"axp806-regulator"
	},
	.gpio_name = {
		"axp806-gpio"
	},
};


static const struct of_device_id axp80_dt_ids[] = {
	{ .compatible = "axp806", },
	{},
};
MODULE_DEVICE_TABLE(of, axp80_dt_ids);

static int axp80_probe(struct platform_device *pdev)
{
	int ret;
	struct axp_dev *axp80;
	struct device_node *node = pdev->dev.of_node;

	axp80_pmu_num = axp_get_pmu_num(axp80_dt_ids, ARRAY_SIZE(axp80_dt_ids));
	if (axp80_pmu_num < 0) {
		pr_err("%s get pmu num failed\n", __func__);
		return axp80_pmu_num;
	}

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			axp80_config.pmu_used = 0;
			pr_err("%s: pmu_used = %u\n", __func__,
					axp80_config.pmu_used);
			return -EPERM;
		} else {
			axp80_config.pmu_used = 1;
			ret = axp_dt_parse(node, axp80_pmu_num, &axp80_config);
			if (ret) {
				pr_err("%s parse device tree err\n", __func__);
				return -EINVAL;
			}
		}
	} else {
		pr_err("AXP80 device tree err!\n");
		return -EBUSY;
	}
	axp80 = devm_kzalloc(&pdev->dev, sizeof(*axp80), GFP_KERNEL);
	if (!axp80)
		return -ENOMEM;

	axp80->dev = &pdev->dev;
	axp80->nr_cells = ARRAY_SIZE(axp80_cells);
	axp80->cells = axp80_cells;
	axp80->pmu_num = axp80_pmu_num;

	ret = axp_mfd_cell_name_init(&axp80_platform_ops,
				ARRAY_SIZE(axp80_dt_ids), axp80->pmu_num,
				axp80->nr_cells, axp80->cells);
	if (ret)
		return ret;

	axp80->regmap = axp_regmap_init_arisc_twi(&pdev->dev);
	if (IS_ERR(axp80->regmap)) {
		ret = PTR_ERR(axp80->regmap);
		dev_err(&pdev->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, axp80);
	ret = axp80_init_chip(axp80);
	if (ret)
		return ret;

	ret = axp_mfd_add_devices(axp80);
	if (ret) {
		dev_err(axp80->dev, "failed to add MFD devices: %d\n", ret);
		return ret;
	}

	axp80->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	axp80->irq_data = axp_irq_chip_register(axp80->regmap, axp80->irq,
						IRQF_SHARED
						| IRQF_DISABLED
						| IRQF_NO_SUSPEND,
						&axp80_regmap_irq_chip,
						axp80_wakeup_event);
	if (IS_ERR(axp80->irq_data)) {
		ret = PTR_ERR(axp80->irq_data);
		dev_err(&pdev->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	axp80_pm_power = axp80;

	if (!pm_power_off)
		pm_power_off = axp80_power_off;

	axp_platform_ops_set(axp80->pmu_num, &axp80_platform_ops);

	axp80_ws = wakeup_source_register("axp80_wakeup_source");

	return 0;
}

static int axp80_remove(struct platform_device *pdev)
{
	struct axp_dev *axp80 = platform_get_drvdata(pdev);

	if (axp80 == axp80_pm_power) {
		axp80_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axp80);
	axp_irq_chip_unregister(axp80->irq, axp80->irq_data);

	return 0;
}

static struct platform_driver axp80_driver = {
	.driver = {
		.name   = "axp80",
		.owner  = THIS_MODULE,
		.of_match_table = axp80_dt_ids,
	},
	.probe      = axp80_probe,
	.remove     = axp80_remove,
};

static int __init axp80_init(void)
{
	int ret;
	ret = platform_driver_register(&axp80_driver);
	if (ret != 0)
		pr_err("Failed to register axp80 driver: %d\n", ret);
	return ret;
}
subsys_initcall(axp80_init);

static void __exit axp80_exit(void)
{
	platform_driver_unregister(&axp80_driver);
}
module_exit(axp80_exit);

MODULE_DESCRIPTION("PMIC Driver for AXP80");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");
