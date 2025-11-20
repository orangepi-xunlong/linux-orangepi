// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2025 Allwinner Technology Co., Ltd.
 *
 * Allwinner PMIC USB Type-C Port Controller Interface Driver
 */

#include "tcpci_axp517_core.h"

static const char * const typec_cc_status_name[] = {
	[TYPEC_CC_OPEN]		= "Open",
	[TYPEC_CC_RA]		= "Ra",
	[TYPEC_CC_RD]		= "Rd",
	[TYPEC_CC_RP_DEF]	= "Rp-def",
	[TYPEC_CC_RP_1_5]	= "Rp-1.5",
	[TYPEC_CC_RP_3_0]	= "Rp-3.0",
};

static const char * const usb_role_name[] = {
	[USB_ROLE_NONE]		= "NONE",
	[USB_ROLE_HOST]		= "HOST",
	[USB_ROLE_DEVICE]	= "DEVICE",
};

static const unsigned int usb_extcon_cable[] = {
	EXTCON_JACK_HEADPHONE,
	EXTCON_NONE,
};

static struct axp517_tcpc_resources axp517_tcpc_res = {
	.irq_params = {
		{ .irq_name = "rx-msg-change",		.virq = 0x0, },
		{ .irq_name = "rx-hw-rst",		.virq = 0x0, },
		{ .irq_name = "tx-fail",		.virq = 0x0, },
		{ .irq_name = "tx-discard",		.virq = 0x0, },
		{ .irq_name = "tx-success",		.virq = 0x0, },
		{ .irq_name = "rxbuf-overflow",		.virq = 0x0, },
		{ .irq_name = "cc-state-change",	.virq = 0x0, },
		{ .irq_name = "vbus-change",		.virq = 0x0, },
		{ .irq_name = "high-voltage-alarm",	.virq = 0x0, },
		{ .irq_name = "low-voltage-alarm",	.virq = 0x0, },
		{ .irq_name = "fault",			.virq = 0x0, },
		{ .irq_name = "snk-disconnect-detect",	.virq = 0x0, },
		{ .irq_name = "vendor",			.virq = 0x0, },
	},
	.nr_irqs = 13,
};

static bool axp517_tcpc_get_psy(struct axp517_tcpci_chip *chip)
{
	if (!IS_ERR_OR_NULL(chip->usb_psy))
		return true;

	if (of_find_property(chip->dev->of_node, "det_usb_supply", NULL)) {
		chip->usb_psy = devm_power_supply_get_by_phandle(chip->dev, "det_usb_supply");
		if (!IS_ERR_OR_NULL(chip->usb_psy))
			return true;
	}

	return false;
}

static bool axp517_tcpci_is_disconnected(struct axp517_tcpci_chip *chip)
{
	enum typec_cc_status cc1, cc2;

	axp517_tcpci_get_cc(&chip->tcpci->tcpc, &cc1, &cc2);
	if (axp517_tcpci_port_is_close(cc1, cc2))
		return true;

	return false;
}

static inline const char *to_irq_name(int irq)
{
	int i;

	for (i = 0; i < axp517_tcpc_res.nr_irqs; i++) {
		if (axp517_tcpc_res.irq_params[i].virq == irq)
			return axp517_tcpc_res.irq_params[i].irq_name;
	}

	return "unknown";
}

/**
 * axp517_tcpci_irq - Interrupt handler for AXP517 TCPCI
 * @irq: Interrupt number
 * @dev_id: Pointer to axp517_tcpci_chip structure
 *
 * Handles TCPCI interrupts by:
 * 1. Reading PD alert status register
 * 2. Scheduling delayed work for cable detection
 * 3. Managing power save monitoring work
 * 4. Triggering VBUS check on CC status change
 * 5. Calling vendor-specific interrupt overrides
 */

static irqreturn_t axp517_tcpci_irq(int irq, void *dev_id)
{
	struct axp517_tcpci_chip *chip = dev_id;
	struct regmap *regmap = chip->data.regmap;
	u16 status;

	regmap_raw_read(regmap, AXP517_IRQ_PD_ALERTL_STATUS, &status, sizeof(u16));
	dev_dbg(chip->dev, " irq %d name %s\n", irq, to_irq_name(irq));

	/**
	 * NOTE:
	 * We provide axp517_tcpci_irq_overrides instead of axp517_tcpci_irq for vendor hooks.
	 */

	queue_delayed_work(system_power_efficient_wq, &chip->wq_detcable,
			   chip->debounce_jiffies);

	cancel_delayed_work_sync(&chip->power_save_mon);
	schedule_delayed_work(&chip->power_save_mon, msecs_to_jiffies(500));

	if (status & AXP517_IRQ_PD_ALERTL_STATUS_CC_STATUS) {
		cancel_delayed_work_sync(&chip->vbus_check_mon);
		schedule_delayed_work(&chip->vbus_check_mon, msecs_to_jiffies(100));
	}

	return axp517_tcpci_irq_overrides(chip->tcpci);
}

/**
 * axp517_tcpci_detect_cable - Cable detection work handler
 * @work: Delayed work structure
 *
 * Detects cable connection status by:
 * 1. Reading CC1 and CC2 status
 * 2. Handling different cable scenarios:
 *    - Audio accessory: No action
 *    - Audio with open: Set Rd role if not toggling
 *    - Open port: Set NONE role and look for connection
 *    - Sink detected: Set DEVICE role
 * 3. Triggering CC change notification
 */

static void axp517_tcpci_detect_cable(struct work_struct *work)
{
	struct axp517_tcpci_chip *chip = container_of(to_delayed_work(work), struct axp517_tcpci_chip,
						 wq_detcable);
	struct tcpm_port *port = chip->tcpci->port;
	enum typec_cc_status cc1, cc2;
	int reg_val;

	axp517_tcpci_get_cc(&chip->tcpci->tcpc, &cc1, &cc2);

	dev_dbg(chip->dev, " CC1: %d - %s, CC2: %d - %s\n",
		 cc1, typec_cc_status_name[cc1], cc2, typec_cc_status_name[cc2]);

	/**
	 * FIXME:
	 * 1. Support double Rp to Vbus cable as sink and device.
	 * 2. Update symbol list for 'usb_role_switch_get_role' function.
	 */
	if (axp517_tcpci_port_is_audio(cc1, cc2)) {
		/* Nothing to do */
	} else if ((axp517_tcpci_cc_is_audio(cc1) && axp517_tcpci_cc_is_open(cc2)) ||
		   (axp517_tcpci_cc_is_audio(cc2) && axp517_tcpci_cc_is_open(cc1))) {
		regmap_read(chip->data.regmap, AXP517_TCPC_CC_STATUS, &reg_val);
		if (!(reg_val & AXP517_TCPC_CC_STATUS_TOGGLING)) {
			regmap_write(chip->data.regmap, AXP517_TCPC_ROLE_CTRL, AXP517_TCPC_ROLE_CTRL_SET(0, 0, AXP517_TCPC_ROLE_CTRL_CC_RD, AXP517_TCPC_ROLE_CTRL_CC_RD));
			mdelay(1);
			regmap_write(chip->data.regmap, AXP517_TCPC_ROLE_CTRL, AXP517_TCPC_ROLE_CTRL_SET(1, 0, AXP517_TCPC_ROLE_CTRL_CC_RD, AXP517_TCPC_ROLE_CTRL_CC_RD));
		}
	} else if (axp517_tcpci_port_is_open(cc1, cc2)) {

		dev_dbg(chip->dev, "Setting Role [%s]\n", usb_role_name[USB_ROLE_NONE]);
		usb_role_switch_set_role(chip->role_sw, USB_ROLE_NONE);

		/* FIXME: Enable DRP toggling for Ra Cable */
		regmap_write(chip->data.regmap, AXP517_TCPC_COMMAND, AXP517_TCPC_CMD_LOOK4CONNECTION);
	} else if (axp517_tcpci_cc_is_sink(cc1) && axp517_tcpci_cc_is_sink(cc2)) {

		dev_dbg(chip->dev, " Setting Role [%s]\n", usb_role_name[USB_ROLE_DEVICE]);
		usb_role_switch_set_role(chip->role_sw, USB_ROLE_DEVICE);
	}
	/* FIXME: Trigger CC interrupt When System Power On. */
	DO_ONCE_LITE(tcpm_cc_change, port);
	chip->debounce_jiffies = msecs_to_jiffies(AXP517_TCPM_DEBOUNCE_MS);
}

/**
 * axp517_tcpci_power_save - Power saving work handler
 * @work: Delayed work structure
 *
 * Manages power saving by:
 * 1. Checking if port is disconnected
 * 2. Enabling soft LPM mode to save power
 * 3. Automatically waking up on interrupt
 */

static void axp517_tcpci_power_save(struct work_struct *work)
{
	struct axp517_tcpci_chip *chip = container_of(to_delayed_work(work), struct axp517_tcpci_chip,
						 power_save_mon);
	struct regmap *regmap = chip->data.regmap;
	int ret = 0;

	/* FIXME: Disable cc clock if port in disconnect after 500ms. */
	/*
	 * Disable 24M oscillator to save power consumption, and it will be
	 * enabled automatically when INT occur after system resume.
	 */

	if (axp517_tcpci_is_disconnected(chip)) {
		ret = regmap_update_bits(regmap, AXP517_AWAKE_EN, AXP517_SOFT_AWAKE_EN, BIT(0));
		if (ret < 0) {
			dev_err(chip->dev, " fail to soft LPM(%d)\n", ret);
		}
	}
}

/**
 * axp517_tcpci_detect_call - VBUS detection work handler
 * @work: Delayed work structure
 *
 * Monitors VBUS status by:
 * 1. Reading CC connection status
 * 2. Determining VBUS presence:
 *    - Debug mode: Check termination status
 *    - Normal mode: Check VBUS valid range
 * 3. Notifying power supply subsystem of VBUS status
 */

static void axp517_tcpci_detect_call(struct work_struct *work)
{
	struct axp517_tcpci_chip *chip = container_of(to_delayed_work(work), struct axp517_tcpci_chip,
						 vbus_check_mon);
	struct regmap *regmap = chip->data.regmap;
	unsigned int data;
	bool status;
	int ret = 0;

	ret = regmap_read(regmap, AXP517_CC_CNNT_STA, &data);
	if (ret < 0)
		return;

	data &= AXP517_MASK_VBUS_STAT_BY_TYPEC;
	if (data == AXP517_TYPEC_STAT_DEBUG) {
		mdelay(30);

		ret = regmap_read(regmap, AXP517_TCPC_CC_STATUS, &data);
		if (ret < 0)
			return;
		status = (data & AXP517_TCPC_CC_STATUS_TERM) ? 1 : 0;
	} else {
		status = ((data > 0) && ((data <= AXP517_VBUS_STAT_BY_TYPEC_EXIST_MAX) || (data == AXP517_MASK_VBUS_STAT_BY_TYPEC))) ? 1 : 0;
	}

	sunxi_call_power_supply_notifier_with_data(AW_PSY_EVENT_VBUS_ONLINE_CHECK, &status);
}

/**
 * axp517_tcpci_extcon_charge_notifier - Charger event notifier
 * @nb: Notifier block
 * @event: Charger event (enable/disable)
 * @ptr: Not used
 *
 * Handles charger events by:
 * 1. On disable:
 *    - Sending hard reset
 *    - Disabling CC clock
 * 2. On enable:
 *    - Enabling CC clock
 *    - Waiting for module ready
 *    - Sending hard reset
 *    - Resetting TCPC port
 */

static void axp517_tcpci_resume_call(struct work_struct *work)
{
	struct axp517_tcpci_chip *chip = container_of(to_delayed_work(work), struct axp517_tcpci_chip,
						 resume_mon);
	struct regmap *regmap = chip->data.regmap;
	enum typec_cc_status cc1, cc2;
	union power_supply_propval temp;

	axp517_tcpci_get_cc(&chip->tcpci->tcpc, &cc1, &cc2);

	dev_dbg(chip->dev, " CC1: %d - %s, CC2: %d - %s\n",
		 cc1, typec_cc_status_name[cc1], cc2, typec_cc_status_name[cc2]);

	if (axp517_tcpci_cc_is_sink(cc1) && axp517_tcpci_cc_is_sink(cc2)) {
		if (axp517_tcpc_get_psy(chip))
			power_supply_get_property(chip->usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &temp);
		if (temp.intval < 6000) {
		/* Transmit Hard Reset: nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
			regmap_write(regmap, AXP517_TCPC_TRANSMIT,
				  (0x2 << AXP517_TCPC_TRANSMIT_RETRY_SHIFT) | (TCPC_TX_HARD_RESET << AXP517_TCPC_TRANSMIT_TYPE_SHIFT));
			tcpm_tcpc_reset(chip->tcpci->port);
		}
	}
}

static int axp517_tcpci_extcon_charge_notifier(struct notifier_block *nb,
	unsigned long event, void *ptr)
{
	struct axp517_tcpci_chip *chip = container_of(nb, struct axp517_tcpci_chip, charger_nb);
	struct regmap *regmap = chip->data.regmap;
	int ret;

	dev_info(chip->dev, "charger event %s\n", event ? "enable" : "disable");

	if (!event) {
		/* Transmit Hard Reset: nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
		ret = regmap_write(regmap, AXP517_TCPC_TRANSMIT,
				(0x2 << AXP517_TCPC_TRANSMIT_RETRY_SHIFT) | (TCPC_TX_HARD_RESET << AXP517_TCPC_TRANSMIT_TYPE_SHIFT));
		if (ret < 0)
			dev_err(chip->dev, "cannot send hardreset, ret=%d", ret);
		/* CC module clock disable */
		ret = regmap_update_bits(regmap, AXP517_CLK_EN, AXP517_CC_CLK_EN, 0);

		if (ret < 0)
			dev_err(chip->dev, " fail to init chip(%d)\n", ret);
	} else {
		/* CC module clock enable */
		ret = regmap_update_bits(regmap, AXP517_CLK_EN, AXP517_CC_CLK_EN, BIT(3));

		if (ret < 0)
			dev_err(chip->dev, " fail to init chip(%d)\n", ret);

		/* Wait for CC module ready */
		mdelay(20);

		/* Transmit Hard Reset: nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
		ret = regmap_write(regmap, AXP517_TCPC_TRANSMIT,
				(0x2 << AXP517_TCPC_TRANSMIT_RETRY_SHIFT) | (TCPC_TX_HARD_RESET << AXP517_TCPC_TRANSMIT_TYPE_SHIFT));
		if (ret < 0)
			dev_err(chip->dev, "cannot send hardreset, ret=%d", ret);

		tcpm_tcpc_reset(chip->tcpci->port);
	}

	return NOTIFY_DONE;
}

static int axp517_tcpci_check_revision(struct axp517_tcpci_chip *chip)
{
	struct regmap *regmap = chip->data.regmap;
	u16 vendor_id;
	int ret;

	ret = regmap_raw_read(regmap, AXP517_TCPC_VENDOR_ID, &vendor_id, sizeof(u16));
	if (ret < 0) {
		dev_err(chip->dev, " fail to read Vendor id(%d)\n", ret);
		return ret;
	}

	if (vendor_id != AXP517_VENDOR_ID) {
		dev_err(chip->dev, " vid is not correct, 0x%04x\n", vendor_id);
		return -ENODEV;
	}

	chip->vendor_id = vendor_id;

	return 0;
}

static int axp517_tcpci_sw_reset(struct axp517_tcpci_chip *chip)
{
	struct regmap *regmap = chip->data.regmap;
	int ret;

	/* soft reset */
	ret = regmap_update_bits(regmap, AXP517_CC_GENERAL_CONTROL, AXP517_GLOBAL_SW_RESET, BIT(5));

	if (ret < 0) {
		dev_err(chip->dev, " fail to soft reset chip(%d)\n", ret);
		return ret;
	}

	ret = regmap_update_bits(regmap, AXP517_CC_GENERAL_CONTROL, AXP517_GLOBAL_SW_RESET, 0);
	if (ret < 0) {
		dev_err(chip->dev, " fail to clear soft reset registers(%d)\n", ret);
		return ret;
	}

	return ret;
}

static int axp517_tcpci_init_chip(struct axp517_tcpci_chip *chip)
{
	struct regmap *regmap = chip->data.regmap;
	unsigned int reg;
	int ret = 0;

	ret = axp517_tcpci_check_revision(chip);
	if (ret < 0) {
		dev_err(chip->dev, " check vid/pid fail(%d)\n", ret);
		return ret;
	}

	chip->debounce_jiffies = msecs_to_jiffies(AXP517_TCPM_DEBOUNCE_MS * 3);
	INIT_DELAYED_WORK(&chip->wq_detcable, axp517_tcpci_detect_cable);
	INIT_DELAYED_WORK(&chip->vbus_check_mon, axp517_tcpci_detect_call);
	INIT_DELAYED_WORK(&chip->power_save_mon, axp517_tcpci_power_save);
	INIT_DELAYED_WORK(&chip->resume_mon, axp517_tcpci_resume_call);

	/* System status indication */
	ret = regmap_read(regmap, AXP517_STATUS1, &reg);
	if ((ret < 0) || !(AXP517_PWR_OK & reg)) {
		dev_err(chip->dev, " fail to power on(%d) %#x\n", ret, reg);
		ret = -EINVAL;
		return ret;
	}

	/* CC module clock enable */
	ret = regmap_update_bits(regmap, AXP517_CLK_EN, AXP517_CC_CLK_EN, BIT(3));
	if (ret < 0)
		dev_err(chip->dev, " fail to enable clock(%d)\n", ret);

	/* Wait for CC module ready */
	mdelay(20);

	ret = axp517_tcpci_sw_reset(chip);
	if (ret < 0) {
		dev_err(chip->dev, " fail to soft reset, ret = %d\n", ret);
		return ret;
	}

	chip->edev = devm_extcon_dev_allocate(chip->dev, usb_extcon_cable);
	if (IS_ERR(chip->edev)) {
		dev_err(chip->dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}

	ret = devm_extcon_dev_register(chip->dev, chip->edev);
	if (ret < 0) {
		dev_err(chip->dev, "failed to register extcon device\n");
		return ret;
	}

	return ret;
}

static int axp517_tcpci_init_chip_late(struct axp517_tcpci_chip *chip)
{
	struct device_node *np = NULL;
	int pmu_usbad_cur = 0;
	int ret = 0;

	dev_info(chip->dev, " battery exist: %s", chip->battery_exist ? "yes" : "no");

	if (axp517_tcpc_get_psy(chip)) {
		np = of_parse_phandle(chip->dev->of_node, "det_usb_supply", 0);
		if (np) {
			if (of_property_read_u32(np, "pmu_usbad_cur", &pmu_usbad_cur)) {
				np = of_parse_phandle(chip->usb_psy->of_node, "det_usb_supply", 0);
				if (np) {
					if (of_property_read_u32(np, "pmu_usbad_cur", &pmu_usbad_cur))
						pmu_usbad_cur = 0;
				}
			}
		}
		chip->current_limit = pmu_usbad_cur ? pmu_usbad_cur : 2500;
	}

	chip->role_sw = usb_role_switch_get(chip->dev);
	if (IS_ERR(chip->role_sw)) {
		ret = PTR_ERR(chip->role_sw);
		dev_err(chip->dev, "fail to get usb role switch, %ld\n", PTR_ERR(chip->role_sw));
		return ret;
	}

	if (!device_property_read_bool(chip->dev, "wakeup-source")) {
		dev_info(chip->dev, "wakeup source is disabled!\n");
	} else {
		device_init_wakeup(chip->dev, true);
	}

	return ret;
}

static int axp517_tcpci_dt_parse(struct axp517_tcpci_chip *chip)
{
	chip->port_reset_quirk =
		device_property_read_bool(chip->dev, "aw,port-reset-quirk");
	chip->vbus_float_quirk =
		device_property_read_bool(chip->dev, "aw,vbus-float-quirk");

	return 0;
}

static int axp517_tcpci_parse_device_tree(struct axp517_tcpci_chip *chip)
{
	int ret = 0;

	/* set input current limit */
	if (!chip->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return -ENODEV;
	}

	ret = axp517_tcpci_dt_parse(chip);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return ret;
	}

	chip->vbus = devm_regulator_get_optional(chip->dev, "vbus");
	if (IS_ERR(chip->vbus)) {
		ret = PTR_ERR(chip->vbus);
		chip->vbus = NULL;
		if (ret != -ENODEV)
			return ret;
	}

	if (of_property_read_bool(chip->dev->of_node, "extcon")) {
		chip->charger_edev = extcon_get_edev_by_phandle(chip->dev, 0);
		if (IS_ERR_OR_NULL(chip->charger_edev)) {
			dev_vdbg(chip->dev, "couldn't get extcon device\n");
			return -EPROBE_DEFER;
		}

		chip->charger_nb.notifier_call = axp517_tcpci_extcon_charge_notifier;
		ret = devm_extcon_register_notifier(chip->dev, chip->charger_edev,
						    EXTCON_CHG_USB_PD, &chip->charger_nb);
		if (ret < 0)
			dev_vdbg(chip->dev, "failed to register notifier for Charger\n");
	}

	return ret;
}

static int axp517_tcpci_probe(struct platform_device *pdev)
{
	struct sunxi_power_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct regmap_irq_chip_data *irq_data = axp_dev->regmap_pdirqc;
	struct device_node *node = pdev->dev.of_node;
	struct axp517_tcpci_chip *chip;
	int i = 0, irq;
	u32 base = 0;
	int ret;

	if (!of_device_is_available(node)) {
		pr_err(" axp517 device not configured\n");
		return -ENODEV;
	}

	if (!axp_dev->irq || !irq_data) {
		pr_err(" can't register pmic tcpc without irq or irq_data\n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		dev_err(&pdev->dev, " alloc failed\n");
		return -ENOMEM;
	}

	chip->data.regmap = axp_dev->regmap;
	chip->dev = &pdev->dev;

	ret = axp517_tcpci_parse_device_tree(chip);
	if (ret < 0) {
		return ret;
	}

	ret = axp517_tcpci_init_chip(chip);
	if (ret < 0) {
		dev_err(chip->dev, " pmic chip init failed: %d\n", ret);
		return ret;
	}

	chip->tcpci = axp517_tcpci_register_port_overrides(chip->dev, &chip->data);
	if (IS_ERR(chip->tcpci)) {
		dev_err(chip->dev, " fail to register tcpci port, %ld\n", PTR_ERR(chip->tcpci));
		return PTR_ERR(chip->tcpci);
	}

	ret = axp517_tcpci_init_chip_late(chip);
	if (ret < 0) {
		return ret;
	}

	for (i = 0; i < axp517_tcpc_res.nr_irqs; i++) {
		irq = platform_get_irq_byname(pdev, axp517_tcpc_res.irq_params[i].irq_name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(irq_data, irq);
		if (irq < 0) {
			dev_err(chip->dev, " can't get irq %s\n", axp517_tcpc_res.irq_params[i].irq_name);
			axp517_tcpci_unregister_port_overrides(chip->tcpci);
			return irq;
		}
		/* we use this variable to suspend irq */
		axp517_tcpc_res.irq_params[i].virq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp517_tcpci_irq, 0,
						   axp517_tcpc_res.irq_params[i].irq_name, chip);
		if (ret < 0) {
			dev_err(chip->dev, " failed to request %s IRQ %d: %d\n",
				axp517_tcpc_res.irq_params[i].irq_name, irq, ret);
			axp517_tcpci_unregister_port_overrides(chip->tcpci);
			return ret;
		} else {
			ret = 0;
		}

		dev_dbg(chip->dev, " Requested %s IRQ %d: %d\n", axp517_tcpc_res.irq_params[i].irq_name, irq, ret);
	}

	platform_set_drvdata(pdev, chip);

	dev_info(chip->dev, " Vendor ID: 0x%04x Base: 0x%x, probe success\n", chip->vendor_id, base);
	return 0;
}

static void axp517_tcpc_irq_set(bool enable)
{
	int i = 0, irq;

	for (i = 0; i < axp517_tcpc_res.nr_irqs; i++) {
		irq = axp517_tcpc_res.irq_params[i].virq;
		if (enable)
			enable_irq(irq);
		else
			disable_irq(irq);
	}
}

static void axp517_tcpci_delayed_work_set(struct axp517_tcpci_chip *chip, bool enable)
{
	if (enable) {
		queue_delayed_work(system_power_efficient_wq,
				   &chip->wq_detcable, chip->debounce_jiffies);
		schedule_delayed_work(&chip->vbus_check_mon, msecs_to_jiffies(100));
	} else {
		cancel_delayed_work_sync(&chip->wq_detcable);
		cancel_delayed_work_sync(&chip->vbus_check_mon);
	}
}

static int axp517_tcpci_remove(struct platform_device *pdev)
{
	struct axp517_tcpci_chip *chip = platform_get_drvdata(pdev);

	if (device_may_wakeup(chip->dev))
		device_init_wakeup(chip->dev, false);
	axp517_tcpc_irq_set(false);
	axp517_tcpci_delayed_work_set(chip, false);
	usb_role_switch_put(chip->role_sw);
	axp517_tcpci_unregister_port_overrides(chip->tcpci);

	return 0;
}

/*
 * NOTE: The system is about to shutdown and poweroff, that is to say,
 * we'd better hard reset the port and notify attached device.
 */
static void axp517_tcpci_shutdown(struct platform_device *pdev)
{
	struct axp517_tcpci_chip *chip = platform_get_drvdata(pdev);
	struct regmap *regmap = chip->data.regmap;
	int ret;

	axp517_tcpc_irq_set(false);
	axp517_tcpci_delayed_work_set(chip, false);

	if (!chip->battery_exist) {
		dev_dbg(chip->dev, " no battery, hardreset forbidden");
		return;
	}

	/* Transmit Hard Reset: nRetryCount is 3 in PD2.0 spec where 2 in PD3.0 spec */
	ret = regmap_write(regmap, AXP517_TCPC_TRANSMIT,
			  (0x2 << AXP517_TCPC_TRANSMIT_RETRY_SHIFT) | (TCPC_TX_HARD_RESET << AXP517_TCPC_TRANSMIT_TYPE_SHIFT));
	if (ret < 0)
		dev_err(chip->dev, "cannot send hardreset, ret=%d", ret);
}

static int axp517_tcpci_pm_suspend(struct device *dev)
{
	struct axp517_tcpci_chip *chip = dev_get_drvdata(dev);

	axp517_tcpci_delayed_work_set(chip, false);
	/*
	 * When system suspend, disable irq to prevent interrupt trigger
	 * during I2C bus suspend
	 */
	if (!device_may_wakeup(chip->dev))
		axp517_tcpc_irq_set(false);

	return 0;
}

static int axp517_tcpci_pm_resume(struct device *dev)
{
	struct axp517_tcpci_chip *chip = dev_get_drvdata(dev);
	struct regmap *regmap = chip->data.regmap;
	unsigned int reg;
	int ret = 0;

	/* Enable irq after I2C bus already resume */
	if (!device_may_wakeup(chip->dev))
		axp517_tcpc_irq_set(true);

	axp517_tcpci_delayed_work_set(chip, true);

	if (!chip->port_reset_quirk) {
		dev_info(chip->dev, " disable reset this port\n");
		return 0;
	}

	/*
	 * When the power of axp517 is lost or i2c read failed in PM S/R
	 * process, we must reset the tcpm port first to ensure the devices
	 * can attach again.
	 */
	ret = regmap_read(regmap, AXP517_AWAKE_EN, &reg);
	if (reg & BIT(0) || ret < 0) {
		ret = axp517_tcpci_sw_reset(chip);
		if (ret < 0) {
			dev_err(chip->dev, " fail to soft reset, ret = %d\n", ret);
			return ret;
		}

		tcpm_tcpc_reset(chip->tcpci->port);
		dev_info(chip->dev, " reset the port success(%d)\n", reg);
	}

	schedule_delayed_work(&chip->resume_mon, msecs_to_jiffies(5 * 1000));

	return 0;
}

static SIMPLE_DEV_PM_OPS(axp517_tcpci_pm_ops, axp517_tcpci_pm_suspend, axp517_tcpci_pm_resume);

static const struct of_device_id axp517_tcpci_of_id[] = {
	{ .compatible = "x-powers,axp517-tcpc", .data = &axp517_tcpc_res },
	{},
};
MODULE_DEVICE_TABLE(of, axp517_tcpci_of_id);

static struct platform_driver axp517_tcpci_plat_driver = {
	.driver = {
		.name = "axp517-tcpc",
		.pm = &axp517_tcpci_pm_ops,
		.of_match_table = axp517_tcpci_of_id,
	},
	.probe = axp517_tcpci_probe,
	.remove = axp517_tcpci_remove,
	.shutdown = axp517_tcpci_shutdown,
};
module_platform_driver(axp517_tcpci_plat_driver);

MODULE_ALIAS("platform:axp517-plat-driver");
MODULE_DESCRIPTION("AXP517 USB Type-C Port Controller Interface Driver");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.3");
