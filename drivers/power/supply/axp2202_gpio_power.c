#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include "linux/irq.h"
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/irq.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/err.h>
#include <linux/sunxi-gpio.h>
#include <linux/pinctrl/pinctrl-sunxi.h>
#include <linux/of_gpio.h>
#include "linux/mfd/axp2101.h"

#include <linux/err.h>
//#include "../drivers/gpio/gpiolib.h"
#include "axp2202_charger.h"

struct axp2202_gpio_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *gpio_supply;
	struct axp_config_info  dts_info;
	struct delayed_work        gpio_supply_mon;
	struct delayed_work        gpio_chg_state;
	struct gpio_config axp_acin_det;
	struct gpio_config usbid_drv;
	struct gpio_config axp_vbus_det;
	int acin_det_gpio_value;
};

int ac_in_flag;
int vbus_in_flag;

static enum power_supply_property axp2202_gpio_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static int axp2202_gpio_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct power_supply_desc axp2202_gpio_desc = {
	.name = "axp2202-gpio",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.get_property = axp2202_gpio_get_property,
	.properties = axp2202_gpio_props,
	.num_properties = ARRAY_SIZE(axp2202_gpio_props),
};


int axp2202_irq_handler_gpio_in(void *data)
{
	struct axp2202_gpio_power *gpio_power = data;

	power_supply_changed(gpio_power->gpio_supply);
	ac_in_flag = 1;

	return 0;
}

int axp2202_irq_handler_gpio_out(void *data)
{
	struct axp2202_gpio_power *gpio_power = data;

	power_supply_changed(gpio_power->gpio_supply);
	ac_in_flag = 0;

	return 0;
}

static irqreturn_t axp_acin_gpio_isr(int irq, void *data)
{
	struct axp2202_gpio_power *chg_dev = data;
	chg_dev->acin_det_gpio_value = __gpio_get_value(
			chg_dev->axp_acin_det.gpio);

	if (chg_dev->acin_det_gpio_value == 1) {
		pr_info("[acin_irq] ac_in_flag = %d\n",
			ac_in_flag);
		pr_info("acin_det_gpio_value == 1\n\n");
		axp2202_irq_handler_gpio_in(chg_dev);
	} else {
		pr_info("[acout_irq] ac_in_flag = %d\n",
			ac_in_flag);
		pr_info("nacin_det_gpio_value == 0\n\n");
		axp2202_irq_handler_gpio_out(chg_dev);
	}

	return IRQ_HANDLED;
}

static irqreturn_t axp_vbus_det_isr(int irq, void *data)
{
	struct axp2202_gpio_power *chg_dev = data;
	chg_dev->acin_det_gpio_value = __gpio_get_value(
			chg_dev->axp_vbus_det.gpio);

	if (chg_dev->acin_det_gpio_value == 1) {
		vbus_in_flag = 1;
		pr_info("[acin_irq] vbus_dev_flag = %d\n",
			vbus_in_flag);
		pr_info("vbus_det_gpio_value == 1\n\n");
	} else {
		vbus_in_flag = 0;
		pr_info("[acout_irq] vbus_dev_flag = %d\n",
			vbus_in_flag);
		pr_info("vbus_det_gpio_value == 0\n\n");
	}

	return IRQ_HANDLED;
}

enum axp2202_gpio_virq_index {
	AXP2202_VIRQ_GPIO,
	AXP2202_VBUS_DET,
};

static struct axp_interrupts axp_gpio_irq[] = {
	[AXP2202_VIRQ_GPIO] = { "pmu_acin_det_gpio", axp_acin_gpio_isr },
	[AXP2202_VBUS_DET] = { "pmu_acin_det_gpio", axp_vbus_det_isr },
};

static int axp_acin_gpio_init(struct axp2202_gpio_power *chg_dev)
{
	unsigned long int config_set;
	int pull = 0;

	int id_irq_num = 0;
	unsigned long irq_flags = 0;
	int ret = 0;

	if (!gpio_is_valid(chg_dev->axp_acin_det.gpio)) {
		pr_err("ERR: get pmu_acin_det_gpio is fail\n");
		return -EINVAL;
	}

	if (!gpio_is_valid(chg_dev->usbid_drv.gpio)) {
		pr_err("ERR: get pmu_usbid_drv_gpio is fail\n");
		return -EINVAL;
	}

	if (!gpio_is_valid(chg_dev->axp_vbus_det.gpio)) {
		pr_err("ERR: get axp_vbus_det_gpio is fail\n");
		return -EINVAL;
	}

	pr_info("axp_acin_gpio_init:%d, usbid_drv:%d, axp_vbus_det:%d\n",
			chg_dev->axp_acin_det.gpio, chg_dev->usbid_drv.gpio, chg_dev->axp_vbus_det.gpio);

	ret = gpio_request(
			chg_dev->axp_acin_det.gpio,
			"pmu_acin_det_gpio");

	if (ret != 0) {
		pr_err("ERR: pmu_acin_det gpio_request failed\n");
		return -EINVAL;
	}

	ret = gpio_request(
			chg_dev->usbid_drv.gpio,
			"pmu_acin_usbid_drv");

	if (ret != 0) {
		pr_err("ERR: pmu_usbid_drv gpio_request failed\n");
		return -EINVAL;
	}

	ret = gpio_request(
			chg_dev->axp_vbus_det.gpio,
			"pmu_vbus_det_gpio");

	if (ret != 0) {
		pr_err("ERR: pmu_vbus_det_gpio gpio_request failed\n");
		return -EINVAL;
	}


	/* set acin input usbid output */

	gpio_direction_input(chg_dev->axp_acin_det.gpio);
	gpio_direction_output(chg_dev->usbid_drv.gpio, 1);
	gpio_direction_input(chg_dev->axp_vbus_det.gpio);

	/* set id gpio pull up */
	config_set = SUNXI_PINCFG_PACK(
				PIN_CONFIG_BIAS_PULL_UP,
				pull);
	pinctrl_gpio_set_config(chg_dev->axp_acin_det.gpio,
			config_set);

	id_irq_num = gpio_to_irq(chg_dev->axp_acin_det.gpio);

	if (IS_ERR_VALUE((unsigned long)id_irq_num)) {
		pr_err("ERR: map pmu_acin_det gpio to virq failed, err %d\n",
			   id_irq_num);
		return -EINVAL;
	}

	axp_gpio_irq[0].irq = id_irq_num;

	pr_info("acin_id_irq_num:%d, %d \n", axp_gpio_irq[0].irq, id_irq_num);

	irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_ONESHOT | IRQF_NO_SUSPEND;
	ret = request_irq(id_irq_num, axp_gpio_irq[0].isr, irq_flags,
			axp_gpio_irq[0].name, chg_dev);


	if (IS_ERR_VALUE((unsigned long)ret)) {
		pr_err("ERR: request pmu_acin_det virq %d failed, err %d\n",
			 id_irq_num, ret);
		return -EINVAL;
	}

	dev_dbg(chg_dev->dev, "Requested %s IRQ %d: %d\n",
		axp_gpio_irq[0].name, id_irq_num, ret);

	/* set id vbus_det pull up */
	config_set = SUNXI_PINCFG_PACK(
				PIN_CONFIG_BIAS_PULL_UP,
				pull);
	pinctrl_gpio_set_config(chg_dev->axp_vbus_det.gpio,
			config_set);

	id_irq_num = gpio_to_irq(chg_dev->axp_vbus_det.gpio);

	if (IS_ERR_VALUE((unsigned long)id_irq_num)) {
		pr_err("ERR: map pmu_vbus_det gpio to virq failed, err %d\n",
			   id_irq_num);
		return -EINVAL;
	}

	axp_gpio_irq[1].irq = id_irq_num;

	pr_info("vbus_det_irq_num:%d, %d \n", axp_gpio_irq[1].irq, id_irq_num);

	irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_ONESHOT | IRQF_NO_SUSPEND;
	ret = request_irq(id_irq_num, axp_gpio_irq[1].isr, irq_flags,
			axp_gpio_irq[1].name, chg_dev);


	if (IS_ERR_VALUE((unsigned long)ret)) {
		pr_err("ERR: request pmu_vbus_det virq %d failed, err %d\n",
			 id_irq_num, ret);
		return -EINVAL;
	}

	dev_dbg(chg_dev->dev, "Requested %s IRQ %d: %d\n",
		axp_gpio_irq[1].name, id_irq_num, ret);

	return 0;
}


static void axp2202_gpio_power_monitor(struct work_struct *work)
{
	struct axp2202_gpio_power *gpio_power =
		container_of(work, typeof(*gpio_power), gpio_supply_mon.work);

	unsigned int reg_value;
	int gpio_value;

	gpio_value = __gpio_get_value(gpio_power->axp_acin_det.gpio);
	regmap_read(gpio_power->regmap, AXP2202_CC_STAT0, &reg_value);
	reg_value &= AXP2202_OTG_MARK;

	if ((reg_value == 0x06) | (reg_value == 0x09) | (reg_value == 0x0c)) {
		pr_info("\n\nset_otg_type: %s , %d \n", __func__, __LINE__);
		if (!gpio_value) {
			regmap_update_bits(gpio_power->regmap, AXP2202_MODULE_EN, AXP2202_CHARGER_ENABLE_MARK, 0);
			pr_info("\n\nset_otg_type_disable_charger: %s , %d \n", __func__, __LINE__);
		}
		gpio_direction_output(gpio_power->usbid_drv.gpio, 0);
	} else {
		regmap_update_bits(gpio_power->regmap, AXP2202_MODULE_EN, AXP2202_CHARGER_ENABLE_MARK, AXP2202_CHARGER_ENABLE_MARK);
		pr_info("\n\nset_no_otg_type_enable_charger: %s , %d \n", __func__, __LINE__);

		gpio_direction_output(gpio_power->usbid_drv.gpio, 1);

	}

	schedule_delayed_work(&gpio_power->gpio_supply_mon, msecs_to_jiffies(500));
}


static void axp2202_gpio_power_init(struct axp2202_gpio_power *gpio_power)
{

}


int axp2202_gpio_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	axp_config->wakeup_gpio =
		of_property_read_bool(node, "wakeup_gpio");

	return 0;
}

static void axp2202_gpio_parse_device_tree(struct axp2202_gpio_power *gpio_power)
{
	int ret;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!gpio_power->dev->of_node) {
		pr_info("can not find device tree\n");
		return;
	}

	cfg = &gpio_power->dts_info;
	ret = axp2202_gpio_dt_parse(gpio_power->dev->of_node, cfg);
	if (ret) {
		pr_info("can not parse device tree err\n");
		return;
	}

	/*init axp2202 gpio by device tree*/
	axp2202_gpio_power_init(gpio_power);
}


static int axp2202_gpio_probe(struct platform_device *pdev)
{
	int ret = 0;

	struct axp2202_gpio_power *gpio_power;

	struct device_node *node = pdev->dev.of_node;
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};

	if (!of_device_is_available(node)) {
		pr_err("axp2202-gpio device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		pr_err("can not register axp2202-gpio without irq\n");
		return -EINVAL;
	}

	gpio_power = devm_kzalloc(&pdev->dev, sizeof(*gpio_power), GFP_KERNEL);
	if (gpio_power == NULL) {
		pr_err("axp2202_gpio_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	gpio_power->name = "axp2202_gpio";
	gpio_power->dev = &pdev->dev;
	gpio_power->regmap = axp_dev->regmap;

	/* parse device tree and set register */
	axp2202_gpio_parse_device_tree(gpio_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = gpio_power;

	gpio_power->gpio_supply = devm_power_supply_register(gpio_power->dev,
			&axp2202_gpio_desc, &psy_cfg);

	if (IS_ERR(gpio_power->gpio_supply)) {
		pr_err("axp2202 failed to register gpio power\n");
		ret = PTR_ERR(gpio_power->gpio_supply);
		return ret;
	}

	gpio_power->axp_acin_det.gpio =
		of_get_named_gpio(pdev->dev.of_node,
				"pmu_acin_det_gpio", 0);

	gpio_power->usbid_drv.gpio =
		of_get_named_gpio(pdev->dev.of_node,
				"pmu_acin_usbid_drv", 0);

	gpio_power->axp_vbus_det.gpio =
		of_get_named_gpio(pdev->dev.of_node,
				"pmu_vbus_det_gpio", 0);

	if (gpio_is_valid(gpio_power->axp_acin_det.gpio)) {
		if (gpio_is_valid(gpio_power->usbid_drv.gpio)) {
			if (gpio_is_valid(gpio_power->axp_vbus_det.gpio)) {
				ret = axp_acin_gpio_init(gpio_power);
			if (ret != 0)
				pr_err("axp acin init failed\n");
			} else {
				pr_info("get pmu_vbus_det_gpio is fail\n");
			}
		} else {
			pr_info("get pmu_acin_usbid_drv is fail\n");
		}
	} else {
		pr_info("get pmu_acin_det_gpio is fail\n");
	}

	platform_set_drvdata(pdev, gpio_power);
	printk("axp2202_gpio_probe: %s , %d \n", __func__, __LINE__);

	INIT_DELAYED_WORK(&gpio_power->gpio_supply_mon, axp2202_gpio_power_monitor);
	schedule_delayed_work(&gpio_power->gpio_supply_mon, msecs_to_jiffies(500));

	return ret;

err:
	pr_err("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}


static int axp2202_gpio_remove(struct platform_device *pdev)
{
	struct axp2202_gpio_power *gpio_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&gpio_power->gpio_supply_mon);
	cancel_delayed_work_sync(&gpio_power->gpio_chg_state);

	dev_dbg(&pdev->dev, "==============AXP2202 gpio unegister==============\n");
	if (gpio_power->gpio_supply)
		power_supply_unregister(gpio_power->gpio_supply);
	dev_dbg(&pdev->dev, "axp2202 teardown gpio dev\n");

	return 0;
}



static inline void axp2202_gpio_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2202_gpio_virq_dts_set(struct axp2202_gpio_power *gpio_power, bool enable)
{
	struct axp_config_info *dts_info = &gpio_power->dts_info;

	if (!dts_info->wakeup_gpio)
		axp2202_gpio_irq_set(axp_gpio_irq[AXP2202_VIRQ_GPIO].irq,
				enable);
}

static int axp2202_gpio_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp2202_gpio_power *gpio_power = platform_get_drvdata(p);


	axp2202_gpio_virq_dts_set(gpio_power, false);
	return 0;
}

static int axp2202_gpio_resume(struct platform_device *p)
{
	struct axp2202_gpio_power *gpio_power = platform_get_drvdata(p);


	axp2202_gpio_virq_dts_set(gpio_power, true);

	return 0;
}

static const struct of_device_id axp2202_gpio_power_match[] = {
	{
		.compatible = "x-powers,gpio-supply",
		.data = (void *)AXP2202_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp2202_gpio_power_match);

static struct platform_driver axp2202_gpio_power_driver = {
	.driver = {
		.name = "gpio-supply",
		.of_match_table = axp2202_gpio_power_match,
	},
	.probe = axp2202_gpio_probe,
	.remove = axp2202_gpio_remove,
	.suspend = axp2202_gpio_suspend,
	.resume = axp2202_gpio_resume,
};

module_platform_driver(axp2202_gpio_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp2202 gpio driver");
MODULE_LICENSE("GPL");

