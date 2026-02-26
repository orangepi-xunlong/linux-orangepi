// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2025 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2025 Allwinner Technology Co., Ltd.
 *
 * Allwinner PMIC USB Type-C Port Manager Driver
 */

#include "sunxi_cc_logic.h"

static int sunxi_pmic_cc_logic_get_cc_status(struct sunxi_pmic_cc_logic *port)
{
	struct regmap *regmap = port->regmap;
	enum sunxi_cc_logic_state state;
	unsigned int data;
	int ret;

	ret = regmap_read(regmap, port->cc_logic_data.cc_status_reg, &data);
	if (ret < 0)
		return ret;

	state = TYPEC_STATE(data);
	port->sunxi_cc_logic_state = state;
	return 0;
}

static int sunxi_pmic_cc_logic_get_power_status(struct sunxi_pmic_cc_logic *port)
{
	struct regmap *regmap = port->regmap;
	enum power_state state;
	unsigned int data;
	int ret;

	ret = regmap_read(regmap, port->cc_logic_data.cc_status_reg, &data);
	if (ret < 0)
		return ret;

	state = POWER_STATE(data);
	port->power_state = state;
	return 0;
}

static int sunxi_pmic_cc_logic_get_flag_status(struct sunxi_pmic_cc_logic *port)
{
	struct regmap *regmap = port->regmap;
	enum flag_state state;
	unsigned int data;
	int ret;

	ret = regmap_read(regmap, port->cc_logic_data.cc_flag_reg, &data);
	if (ret < 0)
		return ret;

	state = FLAG_STATE(data);
	port->flag_state = state;
	return 0;
}

static int sunxi_pmic_cc_logic_set_scope(struct sunxi_pmic_cc_logic *port, int cc_type)
{
	struct regmap *regmap = port->regmap;
	unsigned int data = 0x03;
	int ret;

	ret = regmap_read(port->regmap, port->cc_logic_data.cc_mode_ctrl_reg, &data);
	if (ret < 0)
		return ret;

	switch (cc_type) {
	case POWER_SUPPLY_SCOPE_SYSTEM://source/host
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, BIT(5), BIT(5));
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, BIT(4), 0);
		data = 0x02;
		break;
	case POWER_SUPPLY_SCOPE_DEVICE://sink/
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, BIT(4), BIT(4));
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, BIT(5), 0);
		data = 0x01;
		break;
	case POWER_SUPPLY_SCOPE_UNKNOWN:
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, BIT(4), BIT(4));
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, BIT(5), 0);
		/* reset type-c*/
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, GENMASK(1, 0), 0);
		mdelay(50);
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, GENMASK(1, 0), GENMASK(1, 0));
		break;
	default:
		break;
	}

	SUNXI_POWER_LOG_INFO(port->debug, "set_scope to %d", cc_type);

	regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, GENMASK(1, 0), data);
	return 0;
}

static int sunxi_cc_logic_set_vbus(struct sunxi_pmic_cc_logic *port, bool on)
{
	int ret = 0;

	if (!port->vbus)
		return ret;

	mutex_lock(&port->vbus_lock);
	if (port->vbus_on == on) {
		dev_dbg(port->dev, " vbus is already %s", on ? "On" : "Off");
		goto done;
	}

	SUNXI_POWER_LOG_INFO(port->debug, "vbus usb on : %d -> %d",
			port->vbus_on,
			on);

	if (on)
		ret = regulator_enable(port->vbus);
	else
		ret = regulator_disable(port->vbus);
	if (ret < 0) {
		PMIC_ERR("cannot %s vbus regulator, ret=%d",
			on ? "enable" : "disable", ret);
		goto done;
	}
	dev_info(port->dev, " set vbus %s", on ? "On" : "Off");

	port->vbus_on = on;

done:
	mutex_unlock(&port->vbus_lock);

	return ret;
}

static void sunxi_cc_logic_set_usb_role(struct sunxi_pmic_cc_logic *port, enum usb_role role)
{
	SUNXI_POWER_LOG_INFO(port->debug, "set usb role : %d -> %d",
			port->current_usb_role,
			role);

	if (port->role_sw)
		usb_role_switch_set_role(port->role_sw, role);
	port->current_usb_role = role;
}

static void handle_src_attached(struct sunxi_pmic_cc_logic *port)
{
	if (atomic_read(&port->set_vbus_online)) {
		port->vbus_vsafe0v = false;
		return;
	}

	if (!port->vbus_vsafe0v)
		mdelay(port->vsafe0v_delay);

	port->vbus_vsafe0v = true;
	port->state_machine_times = 0;

	if (port->vbus)
		sunxi_cc_logic_set_vbus(port, true);

	sunxi_cc_logic_set_usb_role(port, USB_ROLE_HOST);

	if (port->usbid_drv.gpio)
		gpio_direction_output(port->usbid_drv.gpio, 1);
}

static void handle_snk_attached(struct sunxi_pmic_cc_logic *port)
{
	port->state_machine_times = 0;

	if (port->vbus)
		sunxi_cc_logic_set_vbus(port, false);

	sunxi_cc_logic_set_usb_role(port, USB_ROLE_DEVICE);
}

static void handle_snk_attached_wait(struct sunxi_pmic_cc_logic *port)
{
	int ret;

	mdelay(200);

	ret = sunxi_pmic_cc_logic_get_cc_status(port);
	if (ret < 0) {
		PMIC_ERR("Failed to get CC status: %d\n", ret);
		return;
	}

	if (port->sunxi_cc_logic_state == SNK_ATTACH_WAIT) {
		port->prev_sunxi_cc_logic_state = port->sunxi_cc_logic_state;
		port->sunxi_cc_logic_state = SNK_ATTACHED;
		SUNXI_POWER_LOG_INFO(port->debug, "in %s state for 200ms, set to %s status",
				sunxi_cc_logic_states[port->prev_sunxi_cc_logic_state],
				sunxi_cc_logic_states[port->sunxi_cc_logic_state]);
		handle_snk_attached(port);
	}
}

static void handle_unattached(struct sunxi_pmic_cc_logic *port)
{
	if (port->vbus)
		sunxi_cc_logic_set_vbus(port, false);

	sunxi_cc_logic_set_usb_role(port, USB_ROLE_NONE);
}

static void run_state_machine(struct sunxi_pmic_cc_logic *port)
{
	int ret;

	ret = sunxi_pmic_cc_logic_get_cc_status(port);
	if (ret < 0) {
		PMIC_ERR("Failed to get CC status: %d\n", ret);
		return;
	}

	ret = sunxi_pmic_cc_logic_get_flag_status(port);
	if (ret < 0) {
		PMIC_ERR("Failed to get CC flag status: %d\n", ret);
		return;
	}

	PMIC_DEBUG(" state change %s -> %s\n",
			sunxi_cc_logic_states[port->prev_sunxi_cc_logic_state],
			sunxi_cc_logic_states[port->sunxi_cc_logic_state]);

	if (port->prev_sunxi_cc_logic_state == port->sunxi_cc_logic_state && port->sunxi_cc_logic_state != SRC_ATTACHED)
		return;

	SUNXI_POWER_LOG_INFO(port->debug, " state change %s -> %s",
			sunxi_cc_logic_states[port->prev_sunxi_cc_logic_state],
			sunxi_cc_logic_states[port->sunxi_cc_logic_state]);

	switch (port->sunxi_cc_logic_state) {
	case SRC_ATTACH_WAIT:
	case SRC_TRY:
	case SRC_TRYWAIT:
		/* Ignore intermediate states */
		break;
	case SRC_ATTACHED:
		handle_src_attached(port);
		break;
	case SNK_ATTACH_WAIT:
		handle_snk_attached_wait(port);
		break;
	case SNK_TRYWAIT:
	case SNK_TRY:
		/* Ignore intermediate states */
		break;
	case SNK_ATTACHED:
		handle_snk_attached(port);
		break;
	case AUDIO_ACC_ATTACHED:
		extcon_set_state_sync(port->edev, EXTCON_JACK_HEADPHONE, true);
		break;
	case INVALID_STATE:
	case RESERVED0:
	case RESERVED1:
	case ERROR_RECOVERY:
	case RESERVED2:
		/* Ignore unsupported states */
		break;
	case SNK_UNATTACHED:
	case SRC_UNATTACHED:
	default:
		handle_unattached(port);
		break;
	}
}

static void state_machine_work(struct kthread_work *work)
{
	struct sunxi_pmic_cc_logic *port = container_of(work, struct sunxi_pmic_cc_logic, state_machine);

	mutex_lock(&port->lock);
	port->state_machine_running = true;

	port->prev_sunxi_cc_logic_state = port->sunxi_cc_logic_state;
	run_state_machine(port);

	port->state_machine_running = false;
	mutex_unlock(&port->lock);
}

static enum hrtimer_restart state_machine_timer_handler(struct hrtimer *timer)
{
	struct sunxi_pmic_cc_logic *port = container_of(timer, struct sunxi_pmic_cc_logic,
							state_machine_timer);

	if (port->registered)
		kthread_queue_work(port->wq, &port->state_machine);

	if (port->state_machine_times--) {
		hrtimer_forward_now(timer, ms_to_ktime(port->hrtimer_interval));
		return HRTIMER_RESTART;
	}

	return HRTIMER_NORESTART;
}

static void sunxi_cc_logic_detect_cable(struct work_struct *work)
{
	struct sunxi_pmic_cc_logic *port = container_of(to_delayed_work(work), struct sunxi_pmic_cc_logic,
							wq_detcable);
	int vbus_online = atomic_read(&port->set_vbus_online);

	if (port->sunxi_cc_logic_state != SRC_ATTACHED) {
		if (vbus_online && port->current_usb_role != USB_ROLE_DEVICE)
			sunxi_cc_logic_set_usb_role(port, USB_ROLE_DEVICE);
		else if (!vbus_online && port->current_usb_role != USB_ROLE_NONE)
			sunxi_cc_logic_set_usb_role(port, USB_ROLE_NONE);
	}
}

static void state_machine_delayed_work(struct sunxi_pmic_cc_logic *port, unsigned int delay_ms)
{
	port->hrtimer_interval = delay_ms;
	port->state_machine_times = STATE_MACHINE_LOOP_TIMES;

	if (!port->state_machine_running)
		hrtimer_start(&port->state_machine_timer, ms_to_ktime(delay_ms), HRTIMER_MODE_REL);
}

static void sunxi_pmic_cc_logic_work_process(struct sunxi_pmic_cc_logic *port)
{
	state_machine_delayed_work(port, STATE_MACHINE_DELAY_TIME);
	queue_delayed_work(system_power_efficient_wq, &port->wq_detcable, port->debounce_jiffies);
}

static int sunxi_pmic_cc_logic_typec_change_process(struct sunxi_pmic_cc_logic *port, bool status)
{
	if (!status) {
		if (port->vbus)
			sunxi_cc_logic_set_vbus(port, false);

		extcon_set_state_sync(port->edev, EXTCON_JACK_HEADPHONE, false);
		if (port->usbid_drv.gpio)
			gpio_direction_output(port->usbid_drv.gpio, 1);
	}
	PMIC_DEBUG("%s %d typec_status=%d\n", __func__, __LINE__, status);

	sunxi_pmic_cc_logic_work_process(port);

	power_supply_changed(port->cc_logic_psy);

	return 0;
}

static void sunxi_pmic_cc_logic_handle_power_limit(struct sunxi_pmic_cc_logic *port)
{
	int ret = 0, new_limit;

	ret = sunxi_pmic_cc_logic_get_power_status(port);
	if (ret < 0) {
		PMIC_ERR("Failed to get Power status: %d\n", ret);
		return;
	}

	switch (port->power_state) {
	case POWER_DEF:
		new_limit = port->cc_default_cur ? port->cc_default_cur : 100; // 100mA
		break;
	case POWER_1_5:
		new_limit = 500; // 500mA
		break;
	case POWER_3_0:
		new_limit = 3000; // 3000mA
		break;
	default:
		return;
	}

	if (new_limit == atomic_read(&port->current_limit_ma))
		return;

	atomic_set(&port->current_limit_ma, new_limit);

	power_supply_changed(port->cc_logic_psy);
}

static irqreturn_t sunxi_pmic_cc_logic_typec_in_handler(int irq, void *data)
{
	struct sunxi_pmic_cc_logic *port = data;

	sunxi_pmic_cc_logic_typec_change_process(port, true);

	sunxi_pmic_cc_logic_handle_power_limit(port);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_pmic_cc_logic_typec_out_handler(int irq, void *data)
{
	struct sunxi_pmic_cc_logic *port = data;

	sunxi_pmic_cc_logic_typec_change_process(port, false);

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_pmic_cc_logic_pwr_chng_handler(int irq, void *data)
{
	struct sunxi_pmic_cc_logic *port = data;

	sunxi_pmic_cc_logic_handle_power_limit(port);

	return IRQ_HANDLED;
}

static const struct sunxi_pmic_interrupts sunxi_pmic_cc_logic_irqs[] = {
	{NULL, sunxi_pmic_cc_logic_typec_in_handler},
	{NULL, sunxi_pmic_cc_logic_typec_out_handler},
	{NULL, sunxi_pmic_cc_logic_pwr_chng_handler},
};

static int sunxi_pmic_cc_logic_irq_init(struct platform_device *pdev, struct sunxi_pmic_cc_logic *port)
{
	struct sunxi_power_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	int ret, i, irq;

	for (i = 0; i < ARRAY_SIZE(sunxi_pmic_cc_logic_irqs); i++) {
		const char *irq_name = port->cc_logic_data.irq_names[i];

		if (!irq_name)
			continue;

		irq = platform_get_irq_byname(pdev, irq_name);

		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_ERR("can not get irq\n");
			return irq;
		}

		ret = devm_request_any_context_irq(port->dev, irq,
						sunxi_pmic_cc_logic_irqs[i].isr,
						0, irq_name, port);
		if (ret < 0) {
			PMIC_ERR("failed to request IRQ %d: %d\n", irq, ret);
			return ret;
		} else {
			ret = 0;
		}
	}

	return ret;
};

static void sunxi_pmic_cc_logic_check_vbus_online_process(struct work_struct *work)
{
	struct sunxi_pmic_cc_logic *port =
		container_of(work, struct sunxi_pmic_cc_logic, vbus_online_mon.work);
	int ret = 0;
	union power_supply_propval temp;
	struct regmap *regmap = port->regmap;

	ret = power_supply_get_property(port->usb_power_psy, POWER_SUPPLY_PROP_ONLINE, &temp);
	if (ret < 0)
		temp.intval = 0;

	if (temp.intval == atomic_read(&port->set_vbus_online))
		return;

	atomic_set(&port->set_vbus_online, temp.intval);

	if (!temp.intval) {
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, GENMASK(1, 0), 0);
		mdelay(50);
		regmap_update_bits(regmap, port->cc_logic_data.cc_mode_ctrl_reg, GENMASK(1, 0), GENMASK(1, 0));
	}

	power_supply_changed(port->cc_logic_psy);
}

static int sunxi_pmic_cc_logic_vbus_online_det_notify(struct notifier_block *nb, unsigned long val, void *v)
{
	struct sunxi_pmic_cc_logic *port = container_of(nb, struct sunxi_pmic_cc_logic, sunxi_pmic_cc_logic_nb);
	struct power_supply *psy = v;

	if ((val == PSY_EVENT_PROP_CHANGED) && (port->usb_power_psy == psy)) {
		schedule_delayed_work(&port->vbus_online_mon, 0);
		sunxi_pmic_cc_logic_work_process(port);
	}

	return NOTIFY_OK;
}

static int sunxi_pmic_cc_logic_vbus_online_det_notify_init(struct sunxi_pmic_cc_logic *port)
{
	int ret = 0;

	port->sunxi_pmic_cc_logic_nb.notifier_call = sunxi_pmic_cc_logic_vbus_online_det_notify;
	port->sunxi_pmic_cc_logic_nb.priority = 0;
	ret = power_supply_reg_notifier(&port->sunxi_pmic_cc_logic_nb);
	if (ret < 0) {
		PMIC_ERR("failed to register notifier :%d\n", ret);
		return ret;
	}

	return ret;
}

static int sunxi_pmic_cc_logic_init_chip(struct sunxi_pmic_cc_logic *port)
{
	int ret = 0;

	mutex_init(&port->lock);
	mutex_init(&port->vbus_lock);

	/* set default value */
	if (port->cc_default_cur)
		atomic_set(&port->current_limit_ma, port->cc_default_cur);
	else
		atomic_set(&port->current_limit_ma, 100);

	/* parse the delay time configuration about Vbus drop to VSAFE0V */
	port->wq = kthread_create_worker(0, dev_name(port->dev));
	if (IS_ERR(port->wq)) {
		return PTR_ERR(port->wq);
	}
	sched_set_fifo(port->wq->task);

	return ret;
}

static const unsigned int sunxi_pmic_cc_logic_extcon_cable[] = {
	EXTCON_JACK_HEADPHONE,
	EXTCON_NONE,
};

static int sunxi_pmic_cc_logic_extcon_state_init(struct sunxi_pmic_cc_logic *port)
{
	int ret = 0;

	port->edev = devm_extcon_dev_allocate(port->dev, sunxi_pmic_cc_logic_extcon_cable);
	if (IS_ERR_OR_NULL(port->edev)) {
		PMIC_ERR("failed to allocate extcon device\n");
		return PTR_ERR(port->edev);
	}

	ret = devm_extcon_dev_register(port->dev, port->edev);
	if (ret < 0) {
		PMIC_ERR("failed to register extcon device\n");
		return ret;
	}

	return ret;
}

static int sunxi_pmic_cc_logic_work_init(struct sunxi_pmic_cc_logic *port)
{
	int ret = 0;

	port->debounce_jiffies = msecs_to_jiffies(TCPM_DEBOUNCE_MS);
	INIT_DELAYED_WORK(&port->wq_detcable, sunxi_cc_logic_detect_cable);
	kthread_init_work(&port->state_machine, state_machine_work);
	hrtimer_init(&port->state_machine_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	port->state_machine_timer.function = state_machine_timer_handler;

	INIT_DELAYED_WORK(&port->vbus_online_mon, sunxi_pmic_cc_logic_check_vbus_online_process);

	return ret;
}

static enum power_supply_property sunxi_pmic_cc_logic_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static int sunxi_pmic_cc_logic_get_scope(struct sunxi_pmic_cc_logic *port, union power_supply_propval *val)
{
	unsigned int data = 0, cc_audio = 0, flag_state = 0;
	int ret = 0;

	/* intval bit[1:0]: 0x0 - unknown, 0x1 - source, 0x2 - sink */
	/* intval bit[2]: 0x0 - cc2, 0x1 - cc1 */
	/* intval bit[3]: 0x0 - disabled, 0x1 - cc_audio */

	switch (port->sunxi_cc_logic_state) {
	case SRC_ATTACHED:
		data = POWER_SUPPLY_SCOPE_SYSTEM;
		break;
	case SNK_ATTACHED:
		data = POWER_SUPPLY_SCOPE_DEVICE;
		break;
	case AUDIO_ACC_ATTACHED:
		data = POWER_SUPPLY_SCOPE_UNKNOWN;
		cc_audio = 1;
		break;
	default:
		data = POWER_SUPPLY_SCOPE_UNKNOWN;
		break;
	}

	switch (port->flag_state) {
	case ACITVE_CC2:
		flag_state = 0;
		break;
	case ACITVE_CC1:
		flag_state = 1;
		break;
	default:
		flag_state = 0;
		break;
	}

	data |= flag_state << 2;
	data |= cc_audio << 3;

	SUNXI_POWER_LOG_INFO(port->debug, "get cc scope 0x%x", data);

	val->intval = data;

	return ret;
}

static int sunxi_pmic_cc_logic_get_property(struct power_supply *psy,
					   enum power_supply_property psp,
					   union power_supply_propval *val)
{
	struct sunxi_pmic_cc_logic *port = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		fallthrough;
	case POWER_SUPPLY_PROP_PRESENT:
		if (port->sunxi_cc_logic_state == SNK_ATTACHED)
			val->intval = 1;
		else
			val->intval = 0;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = sunxi_pmic_cc_logic_get_scope(port, val);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = 5000000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = atomic_read(&port->current_limit_ma) * 1000;
		break;
	default:
		break;
	}

	return ret;
}

static int sunxi_pmic_cc_logic_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sunxi_pmic_cc_logic *port = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_SCOPE:
		ret = sunxi_pmic_cc_logic_set_scope(port, val->intval);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int sunxi_pmic_cc_logic_power_property_is_writeable(struct power_supply *psy,
				 enum power_supply_property psp)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_SCOPE:
		ret = 1;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static struct power_supply_desc sunxi_pmic_cc_logic_desc = {
	.name = "sunxi-cc-logic",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property = sunxi_pmic_cc_logic_get_property,
	.set_property = sunxi_pmic_cc_logic_set_property,
	.properties = sunxi_pmic_cc_logic_props,
	.num_properties = ARRAY_SIZE(sunxi_pmic_cc_logic_props),
	.property_is_writeable = sunxi_pmic_cc_logic_power_property_is_writeable,
};

static int sunxi_pmic_cc_logic_usbid_gpio_init(struct sunxi_pmic_cc_logic *port)
{
	int ret = 0;

	ret = of_get_named_gpio(port->dev->of_node,
				"sunxi_pmic_cc_logic_usbid_drv", 0);
	if (ret < 0) {
		PMIC_DEBUG("no sunxi_pmic_cc_logic_usbid_drv_gpio\n");
		port->usbid_drv.gpio = 0;
	} else {
		port->usbid_drv.gpio = ret;
		if (!gpio_is_valid(port->usbid_drv.gpio)) {
			PMIC_ERR("get sunxi_pmic_cc_logic_usbid_drv_gpio is fail\n");
			port->usbid_drv.gpio = 0;
			return -EPROBE_DEFER;
		}
	}

	/* set vbus_det input usbid output */
	if (port->usbid_drv.gpio) {
		ret = gpio_request(
				port->usbid_drv.gpio,
				"sunxi_pmic_cc_logic_usbid_drv");
		if (ret != 0) {
			PMIC_ERR("sunxi_pmic_cc_logic_usbid_drv gpio_request failed\n");
			return -EINVAL;
		}
		gpio_direction_output(port->usbid_drv.gpio, 1);
	}
	return 0;
}

static int sunxi_pmic_cc_logic_dt_parse(struct sunxi_pmic_cc_logic *port)
{
	struct device_node *node = port->dev->of_node;
	int ret = 0;

	ret = of_property_read_u32(node, "vsafe0v_delay", &port->vsafe0v_delay);
	if (ret)
		port->vsafe0v_delay = 0;

	ret = of_property_read_u32(node, "cc_default_cur", &port->cc_default_cur);
	if (ret)
		port->cc_default_cur = 0;

	ret = sunxi_pmic_cc_logic_usbid_gpio_init(port);

	return ret;
}

static int sunxi_pmic_cc_logic_parse_device_tree(struct sunxi_pmic_cc_logic *port)
{
	struct device_node *node = port->dev->of_node;
	struct device_node *np;
	int ret = 0;

	if (!node) {
		PMIC_ERR("no device tree node\n");
		return -ENODEV;
	}

	np = of_parse_phandle(node, "det_usb_supply", 0);
	if (np) {
		if (of_device_is_available(np)) {
			port->usb_power_psy = devm_power_supply_get_by_phandle(port->dev, "det_usb_supply");
			if (IS_ERR_OR_NULL(port->usb_power_psy))
				return -EPROBE_DEFER;
			PMIC_INFO("get det_usb_supply\n");
			np = of_parse_phandle(port->usb_power_psy->of_node, "det_usb_supply", 0);
			if (!of_property_read_bool(np, "pmu_usb_typec_used")) {
				PMIC_ERR("no used type-c device\n");
				return -ENODEV;
			}
		} else {
			PMIC_ERR("failed to get det_usb_supply\n");
			return -ENODEV;
		}
	} else {
		PMIC_ERR("failed to get det_usb_supply settings\n");
		return -ENODEV;
	}

	np = of_get_child_by_name(node, "ports");
	if (np) {
		port->role_sw = usb_role_switch_get(port->dev);
		if (IS_ERR(port->role_sw)) {
			ret = PTR_ERR(port->role_sw);
			PMIC_ERR("fail to get usb role switch, %ld\n", PTR_ERR(port->role_sw));
			return ret;
		}
	} else {
		port->role_sw = NULL;
		PMIC_INFO("no usb role switch configured\n");
	}

	if (of_property_read_bool(node, "vbus-supply")) {
		port->vbus = devm_regulator_get_optional(port->dev, "vbus");
		if (IS_ERR(port->vbus)) {
			if (PTR_ERR(port->vbus) == -EPROBE_DEFER) {
				return -EPROBE_DEFER;
			}
			port->vbus = NULL;
			PMIC_ERR("couldn't get vbus supply\n");
		}
	} else {
		port->vbus = NULL;
		PMIC_INFO("no vbus supply configured\n");
	}

	port->cc_logic_data.init_regs(port, true);
	ret = sunxi_pmic_cc_logic_dt_parse(port);

	return ret;
}

static int axp2202_cc_logic_enable(struct sunxi_pmic_cc_logic *port, bool enable)
{
	struct regmap *regmap = port->regmap;
	int ret = 0;

	/* set type-c en/disable & mode */
	if (enable) {
		regmap_update_bits(regmap, AXP2202_CLK_EN, BIT(3), BIT(3));
		regmap_update_bits(regmap, AXP2202_CC_GLB_CTRL, BIT(2), 0);
		regmap_update_bits(regmap, AXP2202_CC_GLB_CTRL, BIT(5), BIT(5));
		regmap_update_bits(regmap, AXP2202_CC_MODE_CTRL, BIT(4), 0);
		regmap_update_bits(regmap, AXP2202_CC_MODE_CTRL, BIT(1), BIT(1));
		regmap_update_bits(regmap, AXP2202_CC_MODE_CTRL, BIT(0), BIT(0));
	} else {
		regmap_update_bits(regmap, AXP2202_CLK_EN, BIT(3), 0);
		regmap_update_bits(regmap, AXP2202_CC_MODE_CTRL, BIT(1), 0);
		regmap_update_bits(regmap, AXP2202_CC_MODE_CTRL, BIT(0), 0);
	}
	return ret;
}

static int axp515_cc_logic_enable(struct sunxi_pmic_cc_logic *port, bool enable)
{
	struct regmap *regmap = port->regmap;
	int ret = 0;

	/* set type-c en/disable & mode */
	if (enable) {
		regmap_update_bits(regmap, AXP515_CC_EN, BIT(1), BIT(1));
		regmap_update_bits(regmap, AXP515_CC_LOW_POWER_CTRL, BIT(2), 0x00);
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, 0x03, 0x0);
		mdelay(50);
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(4), 0);
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(1), BIT(1));
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(0), BIT(0));
		regmap_update_bits(regmap, AXP515_CC_GLOBAL_CTRL, BIT(5), BIT(5));
		regmap_update_bits(regmap, AXP515_CC_TOGGLE_CTRL, BIT(0), BIT(0));
	} else {
		regmap_update_bits(regmap, AXP515_CC_EN, BIT(1), 0);
	}
	return ret;
}

static int sunxi_pmic_cc_logic_match_device(struct sunxi_pmic_cc_logic *port, int sunxi_pmic_id)
{
	switch (sunxi_pmic_id) {
	case AXP2202_ID:
		port->cc_logic_data.cc_status_reg = AXP2202_CC_STAT0;
		port->cc_logic_data.cc_flag_reg = AXP2202_CC_STAT4;
		port->cc_logic_data.cc_mode_ctrl_reg = AXP2202_CC_MODE_CTRL;
		port->cc_logic_data.init_regs = axp2202_cc_logic_enable;
		port->cc_logic_data.irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_TYPEC_IN] = "type-c_insert";
		port->cc_logic_data.irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_TYPEC_OUT] = "type-c_remove";
		port->cc_logic_data.irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_PWR_CHNG] = "type-c_state_change";
		break;
	case AXP515_ID:
		port->cc_logic_data.cc_status_reg = AXP515_CC_STATUS0;
		port->cc_logic_data.cc_flag_reg = AXP515_CC_STATUS4;
		port->cc_logic_data.cc_mode_ctrl_reg = AXP515_CC_MODE_CTRL;
		port->cc_logic_data.init_regs = axp515_cc_logic_enable;
		port->cc_logic_data.irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_TYPEC_IN] = "tc in";
		port->cc_logic_data.irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_TYPEC_OUT] = "tc out";
		port->cc_logic_data.irq_names[SUNXI_PMIC_CC_LOGIC_VIRQ_PWR_CHNG] = "type-c_state_change";
		break;
/*-------------------*/
	default:
		return -EINVAL;
	}
	return 0;
}

static int sunxi_pmic_cc_logic_probe(struct platform_device *pdev)
{
	struct sunxi_power_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device *dev = &pdev->dev;
	struct sunxi_pmic_cc_logic *port;
	struct power_supply_config psy_cfg = {};
	int ret;

	port = devm_kzalloc(&pdev->dev, sizeof(*port), GFP_KERNEL);
	if (!port) {
		dev_err(&pdev->dev, " alloc failed\n");
		return -ENOMEM;
	}

	if (!axp_dev || !axp_dev->regmap) {
		dev_err(dev, "failed to get parent regmap\n");
		return -ENODEV;
	}

	ret = sunxi_pmic_cc_logic_match_device(port, axp_dev->variant);
	if (ret)
		return ret;

	port->dev = &pdev->dev;
	port->regmap = axp_dev->regmap;

	ret = sunxi_pmic_cc_logic_parse_device_tree(port);
	if (ret < 0) {
		goto out_init_regs_false;
	}

	ret = sunxi_pmic_cc_logic_init_chip(port);
	if (ret < 0) {
		goto out_init_regs_false;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = port;

	port->cc_logic_psy = devm_power_supply_register(port->dev, &sunxi_pmic_cc_logic_desc,
						   &psy_cfg);
	if (IS_ERR(port->cc_logic_psy)) {
		PMIC_ERR("failed to register sunxi cc logic supply\n");
		ret = PTR_ERR(port->cc_logic_psy);
		goto out_role_sw_put;
	}

	ret = sunxi_pmic_cc_logic_work_init(port);
	if (ret < 0) {
		goto out_destroy_wq;
	}

	ret = sunxi_pmic_cc_logic_extcon_state_init(port);
	if (ret < 0)
		goto out_destroy_wq;

	ret = sunxi_pmic_cc_logic_vbus_online_det_notify_init(port);
	if (ret < 0)
		goto out_destroy_wq;

	ret = sunxi_pmic_cc_logic_irq_init(pdev, port);
	if (ret < 0)
		goto out_destroy_wq;

	port->registered = true;
	platform_set_drvdata(pdev, port);

	/* Perform initial detection */
	schedule_delayed_work(&port->vbus_online_mon, 0);
	sunxi_pmic_cc_logic_work_process(port);

	port->debug = sunxi_power_debugfs_init(&pdev->dev);
	if (IS_ERR_OR_NULL(port->debug))
		dev_warn(&pdev->dev, "Failed to init debugfs\n");

	SUNXI_POWER_LOG_INFO(port->debug, "sunxi-cc-logic driver initialized");

	return 0;

out_destroy_wq:
	kthread_destroy_worker(port->wq);

out_role_sw_put:
	if (port->role_sw)
		usb_role_switch_put(port->role_sw);

out_init_regs_false:
	if (ret != -EPROBE_DEFER)
		port->cc_logic_data.init_regs(port, false);

	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}


static inline void sunxi_pmic_cc_logic_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void sunxi_pmic_cc_logic_virq_dts_set(struct sunxi_pmic_cc_logic *port, bool enable)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_pmic_cc_logic_irqs); i++) {
		const char *irq_name = port->cc_logic_data.irq_names[i];

		if (!irq_name)
			continue;

		sunxi_pmic_cc_logic_irq_set(sunxi_pmic_cc_logic_irqs[i].irq,
				enable);
	}
}

static int sunxi_pmic_cc_logic_remove(struct platform_device *pdev)
{
	struct sunxi_pmic_cc_logic *port = platform_get_drvdata(pdev);

	sunxi_pmic_cc_logic_virq_dts_set(port, false);
	port->registered = false;
	kthread_destroy_worker(port->wq);
	hrtimer_cancel(&port->state_machine_timer);
	cancel_delayed_work_sync(&port->wq_detcable);
	usb_role_switch_put(port->role_sw);
	cancel_delayed_work_sync(&port->vbus_online_mon);
	power_supply_unregister(port->cc_logic_psy);
	sunxi_power_debugfs_exit(port->debug);

	return 0;
}

static void sunxi_pmic_cc_logic_shutdown(struct platform_device *pdev)
{
	struct sunxi_pmic_cc_logic *port = platform_get_drvdata(pdev);

	sunxi_pmic_cc_logic_virq_dts_set(port, false);
	port->registered = false;
	kthread_destroy_worker(port->wq);
	hrtimer_cancel(&port->state_machine_timer);
	cancel_delayed_work_sync(&port->wq_detcable);
	usb_role_switch_put(port->role_sw);
	cancel_delayed_work_sync(&port->vbus_online_mon);
}

static int sunxi_pmic_cc_logic_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sunxi_pmic_cc_logic *port = platform_get_drvdata(pdev);

	sunxi_pmic_cc_logic_virq_dts_set(port, false);
	hrtimer_try_to_cancel(&port->state_machine_timer);
	cancel_delayed_work(&port->wq_detcable);
	cancel_delayed_work_sync(&port->vbus_online_mon);

	return 0;
}

static int sunxi_pmic_cc_logic_resume(struct platform_device *pdev)
{
	struct sunxi_pmic_cc_logic *port = platform_get_drvdata(pdev);

	sunxi_pmic_cc_logic_virq_dts_set(port, true);
	if (port->registered) {
		schedule_delayed_work(&port->vbus_online_mon, 0);
		sunxi_pmic_cc_logic_work_process(port);
	}

	return 0;
}

static const struct of_device_id sunxi_pmic_cc_logic_of_id[] = {
	{ .compatible = "x-powers,axp2202-cc-logic"},
	{ .compatible = "x-powers,axp515-cc-logic"},
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_pmic_cc_logic_of_id);

static struct platform_driver sunxi_pmic_cc_logic_driver = {
	.driver = {
		.name = "sunxi-cc-logic",
		.of_match_table = sunxi_pmic_cc_logic_of_id,
	},
	.probe = sunxi_pmic_cc_logic_probe,
	.remove = sunxi_pmic_cc_logic_remove,
	.shutdown = sunxi_pmic_cc_logic_shutdown,
	.suspend = sunxi_pmic_cc_logic_suspend,
	.resume = sunxi_pmic_cc_logic_resume,
};
module_platform_driver(sunxi_pmic_cc_logic_driver);

MODULE_DESCRIPTION("Allwinner PMIC USB Type-C Port Manager Driver");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.6");
