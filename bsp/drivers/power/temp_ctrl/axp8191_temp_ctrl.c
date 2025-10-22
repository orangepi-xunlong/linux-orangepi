/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "sunxi-power-notifier.h"
#include "axp2101.h"
#include "axp8191_temp_ctrl.h"

struct axp8191_temp_ctrl {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct delayed_work        temp_ctrl_mon;
	struct axp_config_info     dts_info;

	/* power supply notifier */
	struct notifier_block      temp_ctrl_nb;
};

static irqreturn_t axp8191_irq_handler_temp_ctrl(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	int temp_status = AW_TEMP_STATUS_GOOD;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	switch (id->irq_data.hwirq) {
	case AXP8191_IRQ_PCBUT:
		temp_status = AW_TEMP_STATUS_COOL;
		PMIC_DEBUG("interrupt:ts_pcb under");
		break;
	case AXP8191_IRQ_QTUT:
		temp_status = AW_TEMP_STATUS_GOOD;
		PMIC_DEBUG("interrupt:quit ts_pcb under");
		break;
	case AXP8191_IRQ_PCBOT:
		temp_status = AW_TEMP_STATUS_WARM;
		PMIC_DEBUG("interrupt:ts_pcb over");
		break;
	case AXP8191_IRQ_QTOT:
		temp_status = AW_TEMP_STATUS_GOOD;
		PMIC_DEBUG("interrupt:quit ts_pcb over");
		break;
	default:
		PMIC_ERR("interrupt:error status");
		break;
	}
	sunxi_call_power_supply_notifier_with_data(AW_PSY_EVENT_BAT_TEMP_CHECK, &temp_status);
	return IRQ_HANDLED;
}

enum axp8191_temp_ctrl_virq_index {
	/* temperature irq */
	AXP8191_VIRQ_TS_PCB_UNDER,
	AXP8191_VIRQ_QUIT_TS_PCB_UNDER,
	AXP8191_VIRQ_TS_PCB_OVER,
	AXP8191_VIRQ_QUIT_TS_PCB_OVER,

	AXP8191_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_temp_ctrl_irq[] = {
	[AXP8191_VIRQ_TS_PCB_UNDER] = { "ts_pcb_under", axp8191_irq_handler_temp_ctrl },
	[AXP8191_VIRQ_TS_PCB_OVER] = { "ts_pcb_over", axp8191_irq_handler_temp_ctrl },
	[AXP8191_VIRQ_QUIT_TS_PCB_UNDER] = { "quit_ts_pcb_under", axp8191_irq_handler_temp_ctrl },
	[AXP8191_VIRQ_QUIT_TS_PCB_OVER] = { "quit_ts_pcb_over", axp8191_irq_handler_temp_ctrl },
};

static int axp8191_temp_ctrl_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_ts_temp_enable,                0);
	AXP_OF_PROP_READ(pmu_ts_temp_ltf,                   0);
	AXP_OF_PROP_READ(pmu_ts_temp_htf,                   0);

	axp_config->wakeup_ts_temp_under =
		of_property_read_bool(node, "wakeup_ts_temp_under");
	axp_config->wakeup_quit_ts_temp_under =
		of_property_read_bool(node, "wakeup_quit_ts_temp_under");
	axp_config->wakeup_ts_temp_over =
		of_property_read_bool(node, "wakeup_ts_temp_over");
	axp_config->wakeup_quit_ts_temp_over =
		of_property_read_bool(node, "wakeup_quit_ts_temp_over");

	return 0;
}

static int axp8191_init_chip(struct axp8191_temp_ctrl *temp_ctrl)
{
	struct regmap *regmap = temp_ctrl->regmap;
	struct axp_config_info *axp_config = &temp_ctrl->dts_info;

	if (!axp_config->pmu_ts_temp_enable) {
		axp_config->wakeup_ts_temp_under = false;
		axp_config->wakeup_quit_ts_temp_under = false;
		axp_config->wakeup_ts_temp_over = false;
		axp_config->wakeup_quit_ts_temp_over = false;
		regmap_update_bits(regmap, AXP8191_TS_ADC_EN_DATA_H, GENMASK(7, 6), 0);
		return -1;
	}

	/* init ts adc */
	regmap_update_bits(regmap, AXP8191_TS_CTRL, GENMASK(3, 0), 0);
	regmap_update_bits(regmap, AXP8191_TS_ADC_EN_DATA_H, GENMASK(7, 6), 0xc0);

	/* set ts vol*/
	if (axp_config->pmu_ts_temp_ltf)
		regmap_update_bits(regmap, AXP8191_VLTF_WORK, GENMASK(7, 0), (axp_config->pmu_ts_temp_ltf / 32));
	if (axp_config->pmu_ts_temp_htf)
		regmap_update_bits(regmap, AXP8191_VHTF_WORK, GENMASK(7, 0), (axp_config->pmu_ts_temp_htf / 2));

	return 0;
}

static void axp8191_temp_ctrl_parse_device_tree(struct axp8191_temp_ctrl *temp_ctrl)
{
	int ret;
	struct axp_config_info *axp_config;
	struct device_node *np = NULL;

	if (!temp_ctrl->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	axp_config = &temp_ctrl->dts_info;
	ret = axp8191_temp_ctrl_dt_parse(temp_ctrl->dev->of_node, axp_config);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}

	np = of_parse_phandle(temp_ctrl->dev->of_node, "det_bat_supply", 0);
	if (!np) {
		PMIC_INFO("battery device is not configed\n");
		return;
	}
	if (!of_device_is_available(np)) {
		axp_config->pmu_ts_temp_enable = 0;
		PMIC_INFO("battery device is not available\n");
	} else {
		of_property_read_u32(np, "pmu_bat_temp_enable", &axp_config->pmu_ts_temp_enable);
		if (axp_config->pmu_ts_temp_enable)
			of_property_read_u32(np, "pmu_jetia_en", &axp_config->pmu_ts_temp_enable);
		of_property_read_u32(np, "pmu_jetia_cool", &axp_config->pmu_ts_temp_ltf);
		of_property_read_u32(np, "pmu_jetia_warm", &axp_config->pmu_ts_temp_htf);
		axp_config->wakeup_ts_temp_under = true;
		axp_config->wakeup_ts_temp_over = true;
		axp_config->wakeup_quit_ts_temp_under = true;
		axp_config->wakeup_quit_ts_temp_over = true;
	}
}

static void axp8191_temp_ctrl_monitor(struct work_struct *work)
{
	struct axp8191_temp_ctrl *temp_ctrl =
		container_of(work, typeof(*temp_ctrl), temp_ctrl_mon.work);

	schedule_delayed_work(&temp_ctrl->temp_ctrl_mon, msecs_to_jiffies(10 * 1000));
}

static int axp8191_temp_ctrl_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;

	struct axp8191_temp_ctrl *temp_ctrl;
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *node = pdev->dev.of_node;

	if (!of_device_is_available(node)) {
		PMIC_ERR("axp8191-temp-ctrl device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp8191-temp-ctrl without irq\n");
		return -EINVAL;
	}

	temp_ctrl = devm_kzalloc(&pdev->dev, sizeof(*temp_ctrl), GFP_KERNEL);
	if (temp_ctrl == NULL) {
		PMIC_ERR("axp8191_temp_ctrl alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	temp_ctrl->name = "axp8191_temp_ctrl";
	temp_ctrl->dev = &pdev->dev;
	temp_ctrl->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp8191_temp_ctrl_parse_device_tree(temp_ctrl);

	ret = axp8191_init_chip(temp_ctrl);
	if (ret < 0) {
		PMIC_ERR("axp8191_temp_ctrl init failed\n");
		ret = -EINVAL;
		goto err;
	}

	for (i = 0; i < ARRAY_SIZE(axp_temp_ctrl_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_temp_ctrl_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_DEV_ERR(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_temp_ctrl_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_temp_ctrl_irq[i].isr, 0,
						   axp_temp_ctrl_irq[i].name, temp_ctrl);
		if (ret < 0) {
			PMIC_DEV_ERR(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_temp_ctrl_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		PMIC_DEV_DEBUG(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_temp_ctrl_irq[i].name, irq, ret);
	}
	platform_set_drvdata(pdev, temp_ctrl);

	INIT_DELAYED_WORK(&temp_ctrl->temp_ctrl_mon, axp8191_temp_ctrl_monitor);
	schedule_delayed_work(&temp_ctrl->temp_ctrl_mon, msecs_to_jiffies(10 * 1000));

	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp8191_temp_ctrl_remove(struct platform_device *pdev)
{
	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP8191 unegister==============\n");
	PMIC_DEV_DEBUG(&pdev->dev, "axp8191 teardown temp_ctrl dev\n");

	return 0;
}

static inline void axp8191_temp_ctrl_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp8191_temp_ctrl_virq_dts_set(struct axp8191_temp_ctrl *temp_ctrl, bool enable)
{
	struct axp_config_info *dts_info = &temp_ctrl->dts_info;

	if (!dts_info->wakeup_ts_temp_under)
		axp8191_temp_ctrl_irq_set(
			axp_temp_ctrl_irq[AXP8191_VIRQ_TS_PCB_UNDER].irq,
			enable);
	if (!dts_info->wakeup_quit_ts_temp_under)
		axp8191_temp_ctrl_irq_set(
			axp_temp_ctrl_irq[AXP8191_VIRQ_QUIT_TS_PCB_UNDER].irq,
			enable);
	if (!dts_info->wakeup_ts_temp_over)
		axp8191_temp_ctrl_irq_set(
			axp_temp_ctrl_irq[AXP8191_VIRQ_TS_PCB_OVER].irq,
			enable);
	if (!dts_info->wakeup_quit_ts_temp_over)
		axp8191_temp_ctrl_irq_set(
			axp_temp_ctrl_irq[AXP8191_VIRQ_QUIT_TS_PCB_UNDER].irq,
			enable);

}

static void axp8191_temp_ctrl_shutdown(struct platform_device *pdev)
{
	struct axp8191_temp_ctrl *temp_ctrl = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&temp_ctrl->temp_ctrl_mon);

}

static int axp8191_temp_ctrl_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp8191_temp_ctrl *temp_ctrl = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&temp_ctrl->temp_ctrl_mon);
	axp8191_temp_ctrl_virq_dts_set(temp_ctrl, false);

	return 0;
}

static int axp8191_temp_ctrl_resume(struct platform_device *pdev)
{
	struct axp8191_temp_ctrl *temp_ctrl = platform_get_drvdata(pdev);

	schedule_delayed_work(&temp_ctrl->temp_ctrl_mon, 0);
	axp8191_temp_ctrl_virq_dts_set(temp_ctrl, true);

	return 0;
}

static const struct of_device_id axp8191_temp_ctrl_match[] = {
	{
		.compatible = "x-powers,axp8191-temp-ctrl",
		.data = (void *)AXP8191_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp8191_temp_ctrl_match);

static struct platform_driver axp8191_temp_ctrl_driver = {
	.driver = {
		.name = "axp8191-temp-ctrl",
		.of_match_table = axp8191_temp_ctrl_match,
	},
	.probe = axp8191_temp_ctrl_probe,
	.remove = axp8191_temp_ctrl_remove,
	.shutdown = axp8191_temp_ctrl_shutdown,
	.suspend = axp8191_temp_ctrl_suspend,
	.resume = axp8191_temp_ctrl_resume,
};

module_platform_driver(axp8191_temp_ctrl_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp8191 temp_ctrl driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
