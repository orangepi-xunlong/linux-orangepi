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

static struct axp_dev *axpdummy_pm_power;
struct axp_config_info axpdummy_config;
static int axpdummy_pmu_num;

static struct mfd_cell axpdummy_cells[] = {
	{
		.name          = "axpdummy-regulator",
	},
};

void axpdummy_power_off(void)
{
	return;
}

static s32 axpdummy_usb_det(void)
{
	return 0;
}

static s32 axpdummy_usb_vbus_output(int high)
{
	return 0;
}

static int axpdummy_cfg_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
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

	api->pmu_arg.twi_port = 0;
	api->pmu_arg.dev_addr = 0x34;
	*pmu_id = axpdummy_config.pmu_id;

	return 0;
}

static const char *axpdummy_get_pmu_name(void)
{
	return axp_name[axpdummy_pmu_num];
}

static struct axp_dev *axpdummy_get_pmu_dev(void)
{
	return axpdummy_pm_power;
}

struct axp_platform_ops axpdummy_platform_ops = {
	.usb_det = axpdummy_usb_det,
	.usb_vbus_output = axpdummy_usb_vbus_output,
	.cfg_pmux_para = axpdummy_cfg_pmux_para,
	.get_pmu_name = axpdummy_get_pmu_name,
	.get_pmu_dev  = axpdummy_get_pmu_dev,
	.regulator_name = {"axpdummy-regulator"},
};

static const struct of_device_id axpdummy_dt_ids[] = {
	{ .compatible = "axpdummy", },
	{},
};
MODULE_DEVICE_TABLE(of, axpdummy_dt_ids);

static int axpdummy_probe(struct platform_device *pdev)
{
	int ret;
	struct axp_dev *axpdummy;
	struct device_node *node = pdev->dev.of_node;

	axpdummy_pmu_num = axp_get_pmu_num(axpdummy_dt_ids,
				ARRAY_SIZE(axpdummy_dt_ids));
	if (axpdummy_pmu_num < 0) {
		pr_err("%s get pmu num failed\n", __func__);
		return axpdummy_pmu_num;
	}

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			axpdummy_config.pmu_used = 0;
			pr_err("%s: pmu_used = %u\n", __func__,
					axpdummy_config.pmu_used);
			return -EPERM;
		} else {
			axpdummy_config.pmu_used = 1;
			ret = axp_dt_parse(node, axpdummy_pmu_num,
				&axpdummy_config);
			if (ret) {
				pr_err("%s parse device tree err\n", __func__);
				return -EINVAL;
			}
		}
	} else {
		pr_err("AXPDUMMY device tree err!\n");
		return -EBUSY;
	}
	axpdummy = devm_kzalloc(&pdev->dev, sizeof(*axpdummy), GFP_KERNEL);
	if (!axpdummy)
		return -ENOMEM;

	axpdummy->dev = &pdev->dev;
	axpdummy->nr_cells = ARRAY_SIZE(axpdummy_cells);
	axpdummy->cells = axpdummy_cells;
	axpdummy->pmu_num = axpdummy_pmu_num;

	ret = axp_mfd_cell_name_init(&axpdummy_platform_ops,
				ARRAY_SIZE(axpdummy_dt_ids), axpdummy->pmu_num,
				axpdummy->nr_cells, axpdummy->cells);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, axpdummy);
	ret = axp_mfd_add_devices(axpdummy);
	if (ret)
		goto fail_init;

	axpdummy->is_dummy = true;
	axpdummy_pm_power = axpdummy;

	if (!pm_power_off)
		pm_power_off = axpdummy_power_off;

	axp_platform_ops_set(axpdummy->pmu_num, &axpdummy_platform_ops);

	return 0;
fail_init:
	dev_err(axpdummy->dev, "failed to add MFD devices: %d\n", ret);
	return ret;
}

static int axpdummy_remove(struct platform_device *pdev)
{
	struct axp_dev *axpdummy = platform_get_drvdata(pdev);

	if (axpdummy == axpdummy_pm_power) {
		axpdummy_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axpdummy);

	return 0;
}


static struct platform_driver axpdummy_driver = {
	.driver = {
		.name   = "axpdummy",
		.owner  = THIS_MODULE,
		.of_match_table = axpdummy_dt_ids,
	},
	.probe      = axpdummy_probe,
	.remove     = axpdummy_remove,
};

static int __init axpdummy_init(void)
{
	int ret;
	ret = platform_driver_register(&axpdummy_driver);
	if (ret != 0)
		pr_err("Failed to register axpdummy driver: %d\n", ret);
	return ret;
}
subsys_initcall(axpdummy_init);

static void __exit axpdummy_exit(void)
{
	platform_driver_unregister(&axpdummy_driver);
}
module_exit(axpdummy_exit);

MODULE_DESCRIPTION("PMIC Driver for AXPDUMMY");
MODULE_AUTHOR("<pannan@allwinnertech.com>");
MODULE_LICENSE("GPL");
