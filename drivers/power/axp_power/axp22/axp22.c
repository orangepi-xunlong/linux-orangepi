#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/reboot.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/power/aw_pm.h>
#include "../axp-core.h"
#include "../axp-charger.h"
#include "../axp-regulator.h"
#include "axp22.h"
#include "axp22-regu.h"

static struct axp_dev *axp22_pm_power;
struct axp_config_info axp22_config;
struct wakeup_source *axp22_ws;
static int axp22_pmu_num;

static struct axp_regmap_irq_chip axp22_regmap_irq_chip = {
	.name           = "axp22_irq_chip",
	.status_base    = AXP22_INTSTS1,
	.enable_base    = AXP22_INTEN1,
	.num_regs       = 5,
};

static struct resource axp22_pek_resources[] = {
	{AXP22_IRQ_PEKRE,  AXP22_IRQ_PEKRE,  "PEK_DBR",      IORESOURCE_IRQ,},
	{AXP22_IRQ_PEKFE,  AXP22_IRQ_PEKFE,  "PEK_DBF",      IORESOURCE_IRQ,},
};

static struct resource axp22_charger_resources[] = {
	{AXP22_IRQ_USBIN,  AXP22_IRQ_USBIN,  "usb in",       IORESOURCE_IRQ,},
	{AXP22_IRQ_USBRE,  AXP22_IRQ_USBRE,  "usb out",      IORESOURCE_IRQ,},
	{AXP22_IRQ_ACIN,   AXP22_IRQ_ACIN,   "ac in",        IORESOURCE_IRQ,},
	{AXP22_IRQ_ACRE,   AXP22_IRQ_ACRE,   "ac out",       IORESOURCE_IRQ,},
	{AXP22_IRQ_BATIN,  AXP22_IRQ_BATIN,  "bat in",       IORESOURCE_IRQ,},
	{AXP22_IRQ_BATRE,  AXP22_IRQ_BATRE,  "bat out",      IORESOURCE_IRQ,},
	{AXP22_IRQ_TEMLO,  AXP22_IRQ_TEMLO,  "bat templow",  IORESOURCE_IRQ,},
	{AXP22_IRQ_TEMOV,  AXP22_IRQ_TEMOV,  "bat tempover", IORESOURCE_IRQ,},
	{AXP22_IRQ_CHAST,  AXP22_IRQ_CHAST,  "charging",     IORESOURCE_IRQ,},
	{AXP22_IRQ_CHAOV,  AXP22_IRQ_CHAOV,  "charge over",  IORESOURCE_IRQ,},
	{AXP22_IRQ_LOWN1,  AXP22_IRQ_LOWN1,  "low warning1", IORESOURCE_IRQ,},
	{AXP22_IRQ_LOWN2,  AXP22_IRQ_LOWN2,  "low warning2", IORESOURCE_IRQ,},
};

static struct resource axp22_gpio_resources[] = {
	{AXP22_IRQ_GPIO0,  AXP22_IRQ_GPIO0,  "gpio0",        IORESOURCE_IRQ,},
	{AXP22_IRQ_GPIO1,  AXP22_IRQ_GPIO1,  "gpio1",        IORESOURCE_IRQ,},
};

static struct mfd_cell axp22_cells[] = {
	{
		.name          = "axp22-powerkey",
		.num_resources = ARRAY_SIZE(axp22_pek_resources),
		.resources     = axp22_pek_resources,
	},
	{
		.name          = "axp22-regulator",
	},
	{
		.name          = "axp22-charger",
		.num_resources = ARRAY_SIZE(axp22_charger_resources),
		.resources     = axp22_charger_resources,
	},
	{
		.name          = "axp22-gpio",
		.num_resources = ARRAY_SIZE(axp22_gpio_resources),
		.resources     = axp22_gpio_resources,
	},
};

void axp22_power_off(void)
{
	uint8_t val;

	pr_info("[axp] send power-off command!\n");
	mdelay(20);

	if (axp22_config.power_start != 1) {
		axp_regmap_read(axp22_pm_power->regmap, AXP22_STATUS, &val);
		if (val & 0xF0) {
			axp_regmap_read(axp22_pm_power->regmap,
					AXP22_MODE_CHGSTATUS, &val);
			if ((axp22_config.pmu_bat_unused == 0)
				&& (val & 0x20)
				&& (val & 0x10)) {
				pr_info("[axp] set flag!\n");
				axp_regmap_read(axp22_pm_power->regmap,
					AXP22_BUFFERC, &val);
				if (0x0d != val)
					axp_regmap_write(axp22_pm_power->regmap,
						AXP22_BUFFERC, 0x0f);

				mdelay(20);
				pr_info("[axp] reboot!\n");
				machine_restart(NULL);
				pr_warn("[axp] warning!!! arch can't reboot,\"\
					\" maybe some error happend!\n");
			}
		}
	}

	axp_regmap_read(axp22_pm_power->regmap, AXP22_BUFFERC, &val);
	if (0x0d != val)
		axp_regmap_write(axp22_pm_power->regmap, AXP22_BUFFERC, 0x00);

	mdelay(20);
	axp_regmap_set_bits(axp22_pm_power->regmap, AXP22_OFF_CTL, 0x80);
	mdelay(20);
	pr_warn("[axp] warning!!! axp can't power-off,\"\
		\" maybe some error happend!\n");
}

static int axp22_init_chip(struct axp_dev *axp22)
{
	uint8_t chip_id, dcdc2_ctl;
	int err;

	err = axp_regmap_read(axp22->regmap, AXP22_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[%s] try to read chip id failed!\n",
				axp_name[axp22_pmu_num]);
		return err;
	}

	if (((chip_id & 0xc0) == 0x00) &&
		(((chip_id & 0x0f) == 0x06)
			|| ((chip_id & 0x0f) == 0x07)
			|| ((chip_id & 0xff) == 0x42))
		) {
		if ((chip_id & 0x0f) == 0x07)
			axp22_need_save_regulator = 1;
		pr_info("[%s] chip id detect 0x%x !\n",
				axp_name[axp22_pmu_num], chip_id);
	} else {
		pr_info("[%s] chip id not detect 0x%x !\n",
				axp_name[axp22_pmu_num], chip_id);
	}

	/* enable dcdc2 dvm */
	err = axp_regmap_read(axp22->regmap, AXP22_DC23_DVM_CTL, &dcdc2_ctl);
	if (err) {
		pr_err("[%s] try to read dcdc2 dvm failed!\n",
				axp_name[axp22_pmu_num]);
		return err;
	}

	dcdc2_ctl |= (0x1<<2);
	err = axp_regmap_write(axp22->regmap, AXP22_DC23_DVM_CTL, dcdc2_ctl);
	if (err) {
		pr_err("[%s] try to enable dcdc2 dvm failed!\n",
				axp_name[axp22_pmu_num]);
		return err;
	}
	pr_info("[%s] enable dcdc2 dvm.\n",
				axp_name[axp22_pmu_num]);

	/*init N_VBUSEN status*/
	if (axp22_config.pmu_vbusen_func)
		axp_regmap_set_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x10);
	else
		axp_regmap_clr_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x10);

	/*init 16's reset pmu en */
	if (axp22_config.pmu_reset)
		axp_regmap_set_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x08);
	else
		axp_regmap_clr_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x08);

	/*init irq wakeup en*/
	if (axp22_config.pmu_irq_wakeup)
		axp_regmap_set_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x80);
	else
		axp_regmap_clr_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x80);

	/*init pmu over temperature protection*/
	if (axp22_config.pmu_hot_shutdown)
		axp_regmap_set_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x04);
	else
		axp_regmap_clr_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x04);

	/*init inshort status*/
	if (axp22_config.pmu_inshort)
		axp_regmap_set_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x60);
	else
		axp_regmap_clr_bits(axp22->regmap, AXP22_HOTOVER_CTL, 0x60);

	return 0;
}

static void axp22_wakeup_event(void)
{
	__pm_wakeup_event(axp22_ws, 0);
}

static s32 axp22_usb_det(void)
{
	u8 value = 0;
	axp_regmap_read(axp22_pm_power->regmap, AXP22_STATUS, &value);
	return (value & 0x10) ? 1 : 0;
}

static s32 axp22_usb_vbus_output(int high)
{
	u8 ret = 0;

	ret = axp_regmap_clr_bits_sync(axp22_pm_power->regmap,
				AXP22_HOTOVER_CTL, 0x10);
	if (ret)
		return ret;

	if (high)
		ret = axp_regmap_set_bits_sync(axp22_pm_power->regmap,
					AXP22_IPS_SET, 0x04);
	else
		ret = axp_regmap_clr_bits_sync(axp22_pm_power->regmap,
					AXP22_IPS_SET, 0x04);

	return ret;
}

static int axp22_cfg_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
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

	api->pmu_arg.twi_port = axp22_pm_power->regmap->client->adapter->nr;
	api->pmu_arg.dev_addr = axp22_pm_power->regmap->client->addr;
	*pmu_id = axp22_config.pmu_id;

	return 0;
}

static const char *axp22_get_pmu_name(void)
{
	return axp_name[axp22_pmu_num];
}

static struct axp_dev *axp22_get_pmu_dev(void)
{
	return axp22_pm_power;
}

struct axp_platform_ops axp22_platform_ops = {
	.usb_det = axp22_usb_det,
	.usb_vbus_output = axp22_usb_vbus_output,
	.cfg_pmux_para = axp22_cfg_pmux_para,
	.get_pmu_name = axp22_get_pmu_name,
	.get_pmu_dev  = axp22_get_pmu_dev,
	.pmu_regulator_save = axp22_regulator_save,
	.pmu_regulator_restore = axp22_regulator_restore,
	.powerkey_name = {
		"axp221s-powerkey",
		"axp227-powerkey"
	},
	.charger_name = {
		"axp221s-charger",
		"axp227-charger"
	},
	.regulator_name = {
		"axp221s-regulator",
		"axp227-regulator"
	},
	.gpio_name = {
		"axp221s-gpio",
		"axp227-gpio"
	},
};

static const struct i2c_device_id axp22_id_table[] = {
	{ "axp221s", 0 },
	{ "axp227", 0 },
	{}
};

static const struct of_device_id axp22_dt_ids[] = {
	{ .compatible = "axp221s", },
	{ .compatible = "axp227", },
	{},
};
MODULE_DEVICE_TABLE(of, axp22_dt_ids);

static int axp22_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct axp_dev *axp22;
	struct device_node *node = client->dev.of_node;

	axp22_pmu_num = axp_get_pmu_num(axp22_dt_ids, ARRAY_SIZE(axp22_dt_ids));
	if (axp22_pmu_num < 0) {
		pr_err("%s get pmu num failed\n", __func__);
		return axp22_pmu_num;
	}

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			axp22_config.pmu_used = 0;
			pr_err("%s: pmu_used = %u\n", __func__,
					axp22_config.pmu_used);
			return -EPERM;
		} else {
			axp22_config.pmu_used = 1;
			ret = axp_dt_parse(node, axp22_pmu_num, &axp22_config);
			if (ret) {
				pr_err("%s parse device tree err\n", __func__);
				return -EINVAL;
			}
		}
	} else {
		pr_err("AXP22x device tree err!\n");
		return -EBUSY;
	}

	axp22 = devm_kzalloc(&client->dev, sizeof(*axp22), GFP_KERNEL);
	if (!axp22)
		return -ENOMEM;

	axp22->dev = &client->dev;
	axp22->nr_cells = ARRAY_SIZE(axp22_cells);
	axp22->cells = axp22_cells;
	axp22->pmu_num = axp22_pmu_num;

	ret = axp_mfd_cell_name_init(&axp22_platform_ops,
				ARRAY_SIZE(axp22_dt_ids), axp22->pmu_num,
				axp22->nr_cells, axp22->cells);
	if (ret)
		return ret;

	axp22->regmap = axp_regmap_init_i2c(&client->dev);
	if (IS_ERR(axp22->regmap)) {
		ret = PTR_ERR(axp22->regmap);
		dev_err(&client->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	ret = axp22_init_chip(axp22);
	if (ret)
		return ret;

	ret = axp_mfd_add_devices(axp22);
	if (ret) {
		dev_err(axp22->dev, "failed to add MFD devices: %d\n", ret);
		return ret;
	}

	axp22->irq_data = axp_irq_chip_register(axp22->regmap, client->irq,
						IRQF_SHARED
						| IRQF_DISABLED
						| IRQF_NO_SUSPEND,
						&axp22_regmap_irq_chip,
						axp22_wakeup_event);
	if (IS_ERR(axp22->irq_data)) {
		ret = PTR_ERR(axp22->irq_data);
		dev_err(&client->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	axp22_pm_power = axp22;

	if (!pm_power_off)
		pm_power_off = axp22_power_off;

	axp_platform_ops_set(axp22->pmu_num, &axp22_platform_ops);

	axp22_ws = wakeup_source_register("axp22_wakeup_source");

	return 0;
}

static int axp22_remove(struct i2c_client *client)
{
	struct axp_dev *axp22 = i2c_get_clientdata(client);

	if (axp22 == axp22_pm_power) {
		axp22_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axp22);
	axp_irq_chip_unregister(client->irq, axp22->irq_data);

	return 0;
}


static struct i2c_driver axp22_driver = {
	.driver = {
		.name   = "axp22",
		.owner  = THIS_MODULE,
		.of_match_table = axp22_dt_ids,
	},
	.probe      = axp22_probe,
	.remove     = axp22_remove,
	.id_table   = axp22_id_table,
};

static int __init axp22_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&axp22_driver);
	if (ret != 0)
		pr_err("Failed to register axp22 I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(axp22_i2c_init);

static void __exit axp22_i2c_exit(void)
{
	i2c_del_driver(&axp22_driver);
}
module_exit(axp22_i2c_exit);

MODULE_DESCRIPTION("PMIC Driver for AXP22X");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");
