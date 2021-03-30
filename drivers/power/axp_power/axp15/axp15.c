#include <linux/interrupt.h>
#include <linux/irq.h>
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
#include "axp15.h"

static struct axp_dev *axp15_pm_power;
struct axp_config_info axp15_config;
struct wakeup_source *axp15_ws;
static int axp15_pmu_num;

static struct axp_regmap_irq_chip axp15_regmap_irq_chip = {
	.name        = "axp15_irq_chip",
	.status_base = AXP15_INTSTS1,
	.enable_base = AXP15_INTEN1,
	.num_regs    = 3,

};

static struct resource axp15_pek_resources[] = {
	{AXP15_IRQ_PEKRE, AXP15_IRQ_PEKRE, "PEK_DBR",   IORESOURCE_IRQ,},
	{AXP15_IRQ_PEKFE, AXP15_IRQ_PEKFE, "PEK_DBF",   IORESOURCE_IRQ,},
};

static struct resource axp15_gpio_resources[] = {
	{AXP15_IRQ_GPIO0, AXP15_IRQ_GPIO0, "gpio0",     IORESOURCE_IRQ,},
	{AXP15_IRQ_GPIO1, AXP15_IRQ_GPIO1, "gpio1",     IORESOURCE_IRQ,},
	{AXP15_IRQ_GPIO2, AXP15_IRQ_GPIO2, "gpio2",     IORESOURCE_IRQ,},
	{AXP15_IRQ_GPIO3, AXP15_IRQ_GPIO3, "gpio3",     IORESOURCE_IRQ,},
};

static struct mfd_cell axp15_cells[] = {
	{
		.name          = "axp15-powerkey",
		.num_resources = ARRAY_SIZE(axp15_pek_resources),
		.resources     = axp15_pek_resources,
	},
	{
		.name          = "axp15-regulator",
	},
	{
		.name          = "axp15-gpio",
		.num_resources = ARRAY_SIZE(axp15_gpio_resources),
		.resources     = axp15_gpio_resources,
	},
};

void axp15_power_off(void)
{
	pr_info("[axp] send power-off command!\n");
	axp_regmap_write(axp15_pm_power->regmap, AXP15_OFF_CTL,
					 0x80);
}

static int axp15_init_chip(struct axp_dev *axp15)
{
	uint8_t chip_id;
	int err;

	err = axp_regmap_read(axp15->regmap, AXP15_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[%s] try to read chip id failed!\n",
				axp_name[axp15_pmu_num]);
		return err;
	}

	if ((chip_id & 0xff) == 0x5)
		pr_info("[%s] chip id detect 0x%x !\n",
				axp_name[axp15_pmu_num], chip_id);
	else
		pr_info("[%s] chip id not detect 0x%x !\n",
				axp_name[axp15_pmu_num], chip_id);

	/* enable dcdc2 dvm */
	err = axp_regmap_update(axp15->regmap, AXP15_DCDC2_DVM_CTRL,
				0x4, 0x4);
	if (err) {
		pr_err("[%s] enable dcdc2 dvm failed!\n",
				axp_name[axp15_pmu_num]);
		return err;
	} else {
		pr_info("[%s] enable dcdc2 dvm.\n",
				axp_name[axp15_pmu_num]);
	}

	/*init irq wakeup en*/
	if (axp15_config.pmu_irq_wakeup)
		axp_regmap_set_bits(axp15->regmap, AXP15_HOTOVER_CTL, 0x80);
	else
		axp_regmap_clr_bits(axp15->regmap, AXP15_HOTOVER_CTL, 0x80);

	/*init pmu over temperature protection*/
	if (axp15_config.pmu_hot_shutdown)
		axp_regmap_set_bits(axp15->regmap, AXP15_HOTOVER_CTL, 0x04);
	else
		axp_regmap_clr_bits(axp15->regmap, AXP15_HOTOVER_CTL, 0x04);

	return 0;
}

static void axp15_wakeup_event(void)
{
	__pm_wakeup_event(axp15_ws, 0);
}

static s32 axp15_usb_det(void)
{
	return 0;
}

static s32 axp15_usb_vbus_output(int high)
{
	u8 ret = 0;

	if (high)
		ret = axp_regmap_update_sync(axp15_pm_power->regmap,
					AXP15_GPIO1_CTL, 0x1, 0x7);
	else
		ret = axp_regmap_update_sync(axp15_pm_power->regmap,
					AXP15_GPIO1_CTL, 0x0, 0x7);

	return ret;
}

static int axp15_cfg_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
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

	api->pmu_arg.twi_port = axp15_pm_power->regmap->client->adapter->nr;
	api->pmu_arg.dev_addr = axp15_pm_power->regmap->client->addr;
	*pmu_id = axp15_config.pmu_id;

	return 0;
}

static const char *axp15_get_pmu_name(void)
{
	return axp_name[axp15_pmu_num];
}

static struct axp_dev *axp15_get_pmu_dev(void)
{
	return axp15_pm_power;
}

struct axp_platform_ops axp15_platform_ops = {
	.usb_det = axp15_usb_det,
	.usb_vbus_output = axp15_usb_vbus_output,
	.cfg_pmux_para = axp15_cfg_pmux_para,
	.get_pmu_name = axp15_get_pmu_name,
	.get_pmu_dev  = axp15_get_pmu_dev,
	.powerkey_name = {
		"axp157-powerkey"
	},
	.regulator_name = {
		"axp157-regulator"
	},
	.gpio_name = {
		"axp157-gpio"
	},
};

static const struct i2c_device_id axp15_id_table[] = {
	{ "axp157", 0 },
	{}
};

static const struct of_device_id axp15_dt_ids[] = {
	{ .compatible = "axp157", },
	{},
};
MODULE_DEVICE_TABLE(of, axp15_dt_ids);

static int axp15_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct axp_dev *axp15;
	struct device_node *node = client->dev.of_node;

	axp15_pmu_num = axp_get_pmu_num(axp15_dt_ids, ARRAY_SIZE(axp15_dt_ids));
	if (axp15_pmu_num < 0) {
		pr_err("%s: get pmu num failed\n", __func__);
		return axp15_pmu_num;
	}

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			axp15_config.pmu_used = 0;
			pr_err("%s: pmu_used = %u\n", __func__,
					axp15_config.pmu_used);
			return -EPERM;
		} else {
			axp15_config.pmu_used = 1;
			ret = axp_dt_parse(node, axp15_pmu_num, &axp15_config);
			if (ret) {
				pr_err("%s parse device tree err\n", __func__);
				return -EINVAL;
			}
		}
	} else {
		pr_err("AXP15 device tree err!\n");
		return -EBUSY;
	}

	axp15 = devm_kzalloc(&client->dev, sizeof(*axp15), GFP_KERNEL);
	if (!axp15)
		return -ENOMEM;

	axp15->dev = &client->dev;
	axp15->nr_cells = ARRAY_SIZE(axp15_cells);
	axp15->cells = axp15_cells;
	axp15->pmu_num = axp15_pmu_num;
	axp15->is_slave = axp15_config.pmu_as_slave;

	ret = axp_mfd_cell_name_init(&axp15_platform_ops,
				ARRAY_SIZE(axp15_dt_ids), axp15->pmu_num,
				axp15->nr_cells, axp15->cells);
	if (ret)
		return ret;

	axp15->regmap = axp_regmap_init_i2c(&client->dev);
	if (IS_ERR(axp15->regmap)) {
		ret = PTR_ERR(axp15->regmap);
		dev_err(&client->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	ret = axp15_init_chip(axp15);
	if (ret)
		return ret;

	ret = axp_mfd_add_devices(axp15);
	if (ret) {
		dev_err(axp15->dev, "failed to add MFD devices: %d\n", ret);
		return ret;
	}

	if (axp15->is_slave) {
		axp15->irq_data = axp_irq_chip_register(axp15->regmap,
				client->irq,
				IRQF_SHARED
				| IRQF_DISABLED
				| IRQF_NO_SUSPEND,
				&axp15_regmap_irq_chip,
				NULL);
	} else {
		axp15->irq_data = axp_irq_chip_register(axp15->regmap,
				client->irq,
				IRQF_SHARED
				| IRQF_DISABLED
				| IRQF_NO_SUSPEND,
				&axp15_regmap_irq_chip,
				axp15_wakeup_event);
	}

	if (IS_ERR(axp15->irq_data)) {
		ret = PTR_ERR(axp15->irq_data);
		dev_err(&client->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	axp15_pm_power = axp15;

	if (!axp15->is_slave) {
		if (!pm_power_off)
			pm_power_off = axp15_power_off;
	}

	axp_platform_ops_set(axp15->pmu_num, &axp15_platform_ops);

	axp15_ws = wakeup_source_register("axp15_wakeup_source");

	return 0;
}

static int axp15_remove(struct i2c_client *client)
{
	struct axp_dev *axp15 = i2c_get_clientdata(client);

	if (axp15 == axp15_pm_power) {
		axp15_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axp15);
	axp_irq_chip_unregister(client->irq, axp15->irq_data);

	return 0;
}

static struct i2c_driver axp15_driver = {
	.driver = {
		.name   = "axp15",
		.owner  = THIS_MODULE,
		.of_match_table = axp15_dt_ids,
	},
	.probe      = axp15_probe,
	.remove     = axp15_remove,
	.id_table   = axp15_id_table,
};

static int __init axp15_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&axp15_driver);
	if (ret != 0)
		pr_err("Failed to register axp15 I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(axp15_i2c_init);

static void __exit axp15_i2c_exit(void)
{
	i2c_del_driver(&axp15_driver);
}
module_exit(axp15_i2c_exit);

MODULE_DESCRIPTION("PMIC Driver for AXP15");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");
