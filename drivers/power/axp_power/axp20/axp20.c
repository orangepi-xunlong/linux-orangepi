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
#include "axp20.h"

static struct axp_dev *axp20_pm_power;
struct axp_config_info axp20_config;
struct wakeup_source *axp20_ws;
static int axp20_pmu_num;

static struct axp_regmap_irq_chip axp20_regmap_irq_chip = {
	.name           = "axp20_irq_chip",
	.status_base    = AXP20_INTSTS1,
	.enable_base    = AXP20_INTEN1,
	.num_regs       = 5,
};

static struct resource axp20_pek_resources[] = {
	{AXP20_IRQ_PEKRE,  AXP20_IRQ_PEKRE,  "PEK_DBR",    IORESOURCE_IRQ,},
	{AXP20_IRQ_PEKFE,  AXP20_IRQ_PEKFE,  "PEK_DBF",    IORESOURCE_IRQ,},
};

static struct resource axp20_charger_resources[] = {
	{AXP20_IRQ_USBIN,  AXP20_IRQ_USBIN,  "usb in",        IORESOURCE_IRQ,},
	{AXP20_IRQ_USBRE,  AXP20_IRQ_USBRE,  "usb out",       IORESOURCE_IRQ,},
	{AXP20_IRQ_ACIN,   AXP20_IRQ_ACIN,   "ac in",         IORESOURCE_IRQ,},
	{AXP20_IRQ_ACRE,   AXP20_IRQ_ACRE,   "ac out",        IORESOURCE_IRQ,},
	{AXP20_IRQ_BATIN,  AXP20_IRQ_BATIN,  "bat in",        IORESOURCE_IRQ,},
	{AXP20_IRQ_BATRE,  AXP20_IRQ_BATRE,  "bat out",       IORESOURCE_IRQ,},
	{AXP20_IRQ_TEMLO,  AXP20_IRQ_TEMLO,  "bat temp low",  IORESOURCE_IRQ,},
	{AXP20_IRQ_TEMOV,  AXP20_IRQ_TEMOV,  "bat temp over", IORESOURCE_IRQ,},
	{AXP20_IRQ_CHAST,  AXP20_IRQ_CHAST,  "charging",      IORESOURCE_IRQ,},
	{AXP20_IRQ_CHAOV,  AXP20_IRQ_CHAOV,  "charge over",   IORESOURCE_IRQ,},
};

static struct mfd_cell axp20_cells[] = {
	{
		.name          = "axp20-powerkey",
		.num_resources = ARRAY_SIZE(axp20_pek_resources),
		.resources     = axp20_pek_resources,
	},
	{
		.name          = "axp20-regulator",
	},
	{
		.name          = "axp20-charger",
		.num_resources = ARRAY_SIZE(axp20_charger_resources),
		.resources     = axp20_charger_resources,
	},
	{
		.name          = "axp20-gpio",
	},
};

void axp20_power_off(void)
{
	uint8_t val;

	if (axp20_config.pmu_pwroff_vol >= 2600
		&& axp20_config.pmu_pwroff_vol <= 3300) {
		if (axp20_config.pmu_pwroff_vol > 3200)
			val = 0x7;
		else if (axp20_config.pmu_pwroff_vol > 3100)
			val = 0x6;
		else if (axp20_config.pmu_pwroff_vol > 3000)
			val = 0x5;
		else if (axp20_config.pmu_pwroff_vol > 2900)
			val = 0x4;
		else if (axp20_config.pmu_pwroff_vol > 2800)
			val = 0x3;
		else if (axp20_config.pmu_pwroff_vol > 2700)
			val = 0x2;
		else if (axp20_config.pmu_pwroff_vol > 2600)
			val = 0x1;
		else
			val = 0x0;

		axp_regmap_update(axp20_pm_power->regmap,
				AXP20_VOFF_SET, val, 0x7);
	}

	/* led auto */
	axp_regmap_clr_bits(axp20_pm_power->regmap, 0x32, 0x38);
	axp_regmap_clr_bits(axp20_pm_power->regmap, 0xb9, 0x80);

	pr_info("[axp] send power-off command!\n");
	mdelay(20);

	if (axp20_config.power_start != 1) {
		axp_regmap_write(axp20_pm_power->regmap, AXP20_INTSTS3, 0x03);
		axp_regmap_read(axp20_pm_power->regmap, AXP20_STATUS, &val);
		if (val & 0xF0) {
			axp_regmap_read(axp20_pm_power->regmap,
					AXP20_MODE_CHGSTATUS, &val);
			if (val & 0x20) {
				pr_info("[axp] set flag!\n");
				axp_regmap_write(axp20_pm_power->regmap,
					AXP20_DATA_BUFFERC, 0x0f);
				mdelay(20);
				pr_info("[axp] reboot!\n");
				machine_restart(NULL);
				pr_warn("[axp] warning!!! arch can't reboot,\"\
					\" maybe some error happend!\n");
			}
		}
	}

	axp_regmap_write(axp20_pm_power->regmap, AXP20_DATA_BUFFERC, 0x00);
	axp_regmap_set_bits(axp20_pm_power->regmap, AXP20_OFF_CTL, 0x80);
	mdelay(20);
	pr_warn("[axp] warning!!! axp can't power-off,\"\
		\" maybe some error happend!\n");
}

static int axp20_init_chip(struct axp_dev *axp20)
{
	uint8_t chip_id, dcdc2_ctl;
	int err;

	err = axp_regmap_read(axp20->regmap, AXP20_IC_TYPE, &chip_id);
	if (err) {
		pr_err("[%s] try to read chip id failed!\n",
				axp_name[axp20_pmu_num]);
		return err;
	}

	if ((chip_id & 0x0f) == 0x1)
		pr_info("[%s] chip id detect 0x%x !\n",
				axp_name[axp20_pmu_num], chip_id);
	else
		pr_info("[%s] chip id not detect 0x%x !\n",
				axp_name[axp20_pmu_num], chip_id);

	/* enable dcdc2 dvm */
	err = axp_regmap_read(axp20->regmap, AXP20_LDO3_DC2_DVM, &dcdc2_ctl);
	if (err) {
		pr_err("[%s] try to read dcdc2 dvm failed!\n",
				axp_name[axp20_pmu_num]);
		return err;
	}

	dcdc2_ctl |= (0x1<<2);
	err = axp_regmap_write(axp20->regmap, AXP20_LDO3_DC2_DVM, dcdc2_ctl);
	if (err) {
		pr_err("[%s] try to enable dcdc2 dvm failed!\n",
				axp_name[axp20_pmu_num]);
		return err;
	}
	pr_info("[%s] enable dcdc2 dvm.\n", axp_name[axp20_pmu_num]);

	/*init 16's reset pmu en */
	if (axp20_config.pmu_reset)
		axp_regmap_set_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x08);
	else
		axp_regmap_clr_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x08);

	/*init irq wakeup en*/
	if (axp20_config.pmu_irq_wakeup)
		axp_regmap_set_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x80);
	else
		axp_regmap_clr_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x80);

	/*init pmu over temperature protection*/
	if (axp20_config.pmu_hot_shutdown)
		axp_regmap_set_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x04);
	else
		axp_regmap_clr_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x04);

	/*init inshort status*/
	if (axp20_config.pmu_inshort)
		axp_regmap_set_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x60);
	else
		axp_regmap_clr_bits(axp20->regmap, AXP20_HOTOVER_CTL, 0x60);

	return 0;
}

static void axp20_wakeup_event(void)
{
	__pm_wakeup_event(axp20_ws, 0);
}

static s32 axp20_usb_det(void)
{
	u8 value = 0;
	axp_regmap_read(axp20_pm_power->regmap, AXP20_STATUS, &value);
	return (value & 0x10) ? 1 : 0;
}

static s32 axp20_usb_vbus_output(int high)
{
	return 0;
}

static int axp20_cfg_pmux_para(int num, struct aw_pm_info *api, int *pmu_id)
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

	api->pmu_arg.twi_port = axp20_pm_power->regmap->client->adapter->nr;
	api->pmu_arg.dev_addr = axp20_pm_power->regmap->client->addr;
	*pmu_id = axp20_config.pmu_id;

	return 0;
}

static const char *axp20_get_pmu_name(void)
{
	return axp_name[axp20_pmu_num];
}

static struct axp_dev *axp20_get_pmu_dev(void)
{
	return axp20_pm_power;
}

struct axp_platform_ops axp20_plat_ops = {
	.usb_det = axp20_usb_det,
	.usb_vbus_output = axp20_usb_vbus_output,
	.cfg_pmux_para = axp20_cfg_pmux_para,
	.get_pmu_name = axp20_get_pmu_name,
	.get_pmu_dev  = axp20_get_pmu_dev,
	.powerkey_name = {
		"axp209-powerkey"
	},
	.charger_name = {
		"axp209-charger"
	},
	.regulator_name = {
		"axp209-regulator"
	},
	.gpio_name = {
		"axp209-gpio"
	},
};

static const struct i2c_device_id axp20_id_table[] = {
	{ "axp209", 0 },
	{}
};

static const struct of_device_id axp20_dt_ids[] = {
	{ .compatible = "axp209", },
	{},
};
MODULE_DEVICE_TABLE(of, axp20_dt_ids);

static int axp20_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int ret;
	struct axp_dev *axp20;
	struct device_node *node = client->dev.of_node;

	axp20_pmu_num = axp_get_pmu_num(axp20_dt_ids, ARRAY_SIZE(axp20_dt_ids));
	if (axp20_pmu_num < 0) {
		pr_err("%s get pmu num failed\n", __func__);
		return axp20_pmu_num;
	}

	if (node) {
		/* get dt and sysconfig */
		if (!of_device_is_available(node)) {
			axp20_config.pmu_used = 0;
			pr_err("%s: pmu_used = %u\n", __func__,
					axp20_config.pmu_used);
			return -EPERM;
		} else {
			axp20_config.pmu_used = 1;
			ret = axp_dt_parse(node, axp20_pmu_num, &axp20_config);
			if (ret) {
				pr_err("%s parse device tree err\n", __func__);
				return -EINVAL;
			}
		}
	} else {
		pr_err("AXP20x device tree err!\n");
		return -EBUSY;
	}

	axp20 = devm_kzalloc(&client->dev, sizeof(*axp20), GFP_KERNEL);
	if (!axp20)
		return -ENOMEM;

	axp20->dev = &client->dev;
	axp20->nr_cells = ARRAY_SIZE(axp20_cells);
	axp20->cells = axp20_cells;
	axp20->pmu_num = axp20_pmu_num;

	ret = axp_mfd_cell_name_init(&axp20_plat_ops,
				ARRAY_SIZE(axp20_dt_ids), axp20->pmu_num,
				axp20->nr_cells, axp20->cells);
	if (ret)
		return ret;

	axp20->regmap = axp_regmap_init_i2c(&client->dev);
	if (IS_ERR(axp20->regmap)) {
		ret = PTR_ERR(axp20->regmap);
		dev_err(&client->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	ret = axp20_init_chip(axp20);
	if (ret)
		return ret;

	ret = axp_mfd_add_devices(axp20);
	if (ret) {
		dev_err(axp20->dev, "failed to add MFD devices: %d\n", ret);
		return ret;
	}

	axp20->irq_data = axp_irq_chip_register(axp20->regmap, client->irq,
						IRQF_SHARED
						| IRQF_DISABLED
						| IRQF_NO_SUSPEND,
						&axp20_regmap_irq_chip,
						axp20_wakeup_event);
	if (IS_ERR(axp20->irq_data)) {
		ret = PTR_ERR(axp20->irq_data);
		dev_err(&client->dev, "axp init irq failed: %d\n", ret);
		return ret;
	}

	axp20_pm_power = axp20;

	if (!pm_power_off)
		pm_power_off = axp20_power_off;

	axp_platform_ops_set(axp20->pmu_num, &axp20_plat_ops);

	axp20_ws = wakeup_source_register("axp20_wakeup_source");

	return 0;
}

static int axp20_remove(struct i2c_client *client)
{
	struct axp_dev *axp20 = i2c_get_clientdata(client);

	if (axp20 == axp20_pm_power) {
		axp20_pm_power = NULL;
		pm_power_off = NULL;
	}

	axp_mfd_remove_devices(axp20);
	axp_irq_chip_unregister(client->irq, axp20->irq_data);

	return 0;
}

static struct i2c_driver axp20_driver = {
	.driver = {
		.name   = "axp20",
		.owner  = THIS_MODULE,
		.of_match_table = axp20_dt_ids,
	},
	.probe      = axp20_probe,
	.remove     = axp20_remove,
	.id_table   = axp20_id_table,
};

static int __init axp20_i2c_init(void)
{
	int ret;
	ret = i2c_add_driver(&axp20_driver);
	if (ret != 0)
		pr_err("Failed to register axp20 I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(axp20_i2c_init);

static void __exit axp20_i2c_exit(void)
{
	i2c_del_driver(&axp20_driver);
}
module_exit(axp20_i2c_exit);

MODULE_DESCRIPTION("PMIC Driver for AXP20X");
MODULE_AUTHOR("Qin <qinyongshen@allwinnertech.com>");
MODULE_LICENSE("GPL");
