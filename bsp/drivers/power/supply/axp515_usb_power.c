/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp515_charger.h"

struct axp515_usb_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *usb_supply;
	struct axp_config_info    dts_info;
	struct delayed_work       usb_supply_mon;
	struct delayed_work       usb_chg_state;
	struct wakeup_source	  *vbus_input_check;
	atomic_t                  set_current_limit;
	atomic_t                  current_limit_sets;

	/* battery_power_supply */
	bool                      battery_supply_exist;
	bool                      battery_supply_enable;

	/* extcon_dev */
	struct extcon_dev         *edev;

	/* gpio_acin_power_supply */
	int                       vbus_det_used;
	struct delayed_work       usb_det_mon;
	struct axp_gpio_para      axp_vbus_det;
	struct axp_gpio_para      usbid_drv;
	struct power_supply       *acin_supply;

};

static enum power_supply_property axp515_usb_props[] = {
	/* real_time */
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_USB_TYPE,
	/* static */
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
};

static const unsigned int axp515_usb_extcon_cable[] = {
	EXTCON_JACK_HEADPHONE,
	EXTCON_NONE,
};

ATOMIC_NOTIFIER_HEAD(usb_power_notifier_list);
EXPORT_SYMBOL(usb_power_notifier_list);

static int axp515_check_acin(struct axp515_usb_power *usb_power)
{
	union power_supply_propval temp;

	if (!usb_power->vbus_det_used)
		return 0;

	if (usb_power->acin_supply == NULL) {
		usb_power->acin_supply = devm_power_supply_get_by_phandle(usb_power->dev,
									"det_acin_supply");
		if (!usb_power->acin_supply)
			return 0;
	}
	power_supply_get_property(usb_power->acin_supply, POWER_SUPPLY_PROP_ONLINE, &temp);

	return temp.intval;
}

static int axp515_check_battery(struct axp515_usb_power *usb_power)
{
	int battery_check = 1, data;

	if (!usb_power->battery_supply_exist) {
		battery_check = 1;
	} else if (!usb_power->battery_supply_enable) {
		battery_check = 0;
	} else {
		regmap_read(usb_power->regmap, AXP515_TIMER2_SET, &data);
		battery_check = !((data & BIT(3)) >> 3);
	}

	return battery_check;
}

static int axp515_get_vbus_state(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_supply = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_supply->regmap;
	unsigned int data;
	int ret = 0;

	if (usb_supply->vbus_det_used) {
		val->intval = gpio_get_value(usb_supply->axp_vbus_det.gpio);
		return ret;
	}

	ret = regmap_read(regmap, AXP515_STATUS0, &data);
	if (ret < 0)
		return ret;

	val->intval = !!(data & AXP515_MASK_VBUS_STAT);
	return 0;
}

static int _axp515_get_iin_limit(struct axp515_usb_power *usb_supply)
{
	struct regmap *regmap = usb_supply->regmap;
	unsigned int data;
	int limit_cur, ret = 0;

	ret = regmap_read(regmap, AXP515_ILIMIT, &data);
	if (ret < 0)
		return ret;
	data &= 0x3F;

	limit_cur = data * 50 + 100;

	return limit_cur;
}

static int axp515_get_iin_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_supply = power_supply_get_drvdata(ps);
	int limit_cur;

	limit_cur = _axp515_get_iin_limit(usb_supply);
	if (limit_cur < 0)
		return limit_cur;

	val->intval = limit_cur;

	return 0;
}

static int axp515_get_vindpm(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP515_RBFET_SET, &data);
	if (ret < 0)
		return ret;

	data = (data * 80) + 3880;
	val->intval = data;

	return 0;
}

static int axp515_get_usb_type(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);

	if (atomic_read(&usb_power->set_current_limit)) {
		val->intval = POWER_SUPPLY_USB_TYPE_SDP;
	} else {
		val->intval = POWER_SUPPLY_USB_TYPE_DCP;
	}

	return 0;
}

static int axp515_get_cc_status(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct axp_config_info *dinfo = &usb_power->dts_info;
	struct regmap *regmap = usb_power->regmap;
	unsigned int data, cc_audio = 0;
	int ret = 0;

	if (!dinfo->pmu_usb_typec_used) {
		val->intval = POWER_SUPPLY_SCOPE_UNKNOWN;
		return ret;
	}

	ret = regmap_read(regmap, AXP515_CC_STATUS0, &data);
	if (ret < 0)
		return ret;

	data &= 0x0f;

	/* intval bit[1:0]: 0x0 - unknown, 0x1 - source, 0x2 - sink */
	if (data == 6) {
		val->intval = POWER_SUPPLY_SCOPE_SYSTEM;//source/host
	} else if (data == 3) {
		val->intval = POWER_SUPPLY_SCOPE_DEVICE;//sink/
	} else {
		if (data == 7)
			cc_audio = 1;
		val->intval = POWER_SUPPLY_SCOPE_UNKNOWN;//disable
	}

	ret = regmap_read(regmap, AXP515_CC_STATUS4, &data);
	if (ret < 0)
		return ret;

	data &= BIT(6);

	/* intval bit[2]: 0x0 - cc2, 0x1 - cc1 */
	val->intval |= (data >> 4);

	/* intval bit[3]: 0x0 - disabled, 0x1 - cc_audio */
	val->intval |= (cc_audio << 3);

	return ret;
}

/* read temperature */
static inline int axp_vts_to_temp(u16 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 10625 / 10000 - 2677;
}

static inline int axp515_get_adc_temp(struct regmap *regmap)
{
	unsigned char temp_val[2];
	unsigned short ts_res;
	int die_temp;
	int ret = 0;

	ret = regmap_bulk_read(regmap, AXP515_DIE_TEMP, temp_val, 2);
	if (ret < 0)
		return ret;

	ts_res = ((unsigned short) temp_val[0] << 8) | temp_val[1];
	die_temp = axp_vts_to_temp(ts_res);

	return die_temp;
}

static int axp515_get_ic_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = usb_power->regmap;

	int i = 0, temp, old_temp;

	old_temp = axp515_get_adc_temp(regmap);

	/* read until abs(old_temp - temp) < 10*/
	temp = axp515_get_adc_temp(regmap);

	PMIC_DEBUG("old_temp:%d, temp:%d\n", old_temp, temp);
	while ((abs(old_temp - temp) > 100) && (i < 10)) {
		old_temp = temp;
		temp = axp515_get_adc_temp(regmap);
		i++;
		PMIC_DEBUG("turn[%d]:old_temp:%d, temp:%d\n", i, old_temp, temp);
	}

	val->intval = temp;

	return 0;
}


static int axp515_set_iin_limit(struct regmap *regmap, int mA)
{
	unsigned int data;
	int ret = 0, reg_val;

	data = mA;
	if (data > 3250)
		data = 3250;
	if	(data < 100)
		data = 100;

	regmap_read(regmap, AXP515_DPM_LOOP_SET, &reg_val);
	if ((data < 3000) && (reg_val & BIT(4)))
		regmap_update_bits(regmap, AXP515_DPM_LOOP_SET, BIT(4), 0);
	else if  ((data >= 3000) && (!(reg_val & BIT(4))))
		regmap_update_bits(regmap, AXP515_DPM_LOOP_SET, BIT(4), BIT(4));

	data = ((data - 100) / 50);
	ret = regmap_update_bits(regmap, AXP515_ILIMIT, GENMASK(5, 0),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp515_set_vindpm(struct regmap *regmap, int mV)
{
	unsigned int data;
	int ret = 0;

	data = mV;

	if (data > 5080)
		data = 5080;
	if	(data < 3880)
		data = 3880;
	data = ((data - 3880) / 80);
	ret = regmap_update_bits(regmap, AXP515_RBFET_SET, GENMASK(3, 0),
				 data);
	if (ret < 0)
		return ret;
	return 0;
}

static int axp515_set_cc_status(struct power_supply *ps, int type)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(ps);
	struct axp_config_info *dinfo = &usb_power->dts_info;
	struct regmap *regmap = usb_power->regmap;
	unsigned int data = 0x03;
	int ret = 0;

	if (!dinfo->pmu_usb_typec_used) {
		return ret;
	}

	switch (type) {
	case POWER_SUPPLY_SCOPE_SYSTEM://source/host
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(5), BIT(5));
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(4), 0);
		data = 0x02;
		break;
	case POWER_SUPPLY_SCOPE_DEVICE://sink/
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(4), BIT(4));
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(5), 0);
		data = 0x01;
		break;
	case POWER_SUPPLY_SCOPE_UNKNOWN:
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(4), BIT(4));
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, BIT(5), 0);
		break;
	default:
		break;
	}

	regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, GENMASK(1, 0), data);
	return ret;
}

static int axp515_usb_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = axp515_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp515_get_vbus_state(psy, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = axp515_get_iin_limit(psy, val);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp515_get_ic_temp(psy, val);
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = axp515_get_cc_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_USB_TYPE:
		ret = axp515_get_usb_type(psy, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP515_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = axp515_get_vindpm(psy, val);
		break;
	default:
		break;
	}

	return ret;
}

static int axp515_usb_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp515_usb_power *usb_power = power_supply_get_drvdata(psy);

	struct regmap *regmap = usb_power->regmap;
	int ret = 0, usb_cur;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if (!axp515_check_battery(usb_power))
			return ret;

		usb_cur = val->intval;
		if (usb_cur < usb_power->dts_info.pmu_usbad_cur) {
			atomic_set(&usb_power->set_current_limit, 1);
			if (axp515_check_acin(usb_power))
				return ret;
		}

		atomic_set(&usb_power->current_limit_sets, usb_cur);
		ret = axp515_set_iin_limit(regmap, usb_cur);
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = axp515_set_cc_status(psy, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = axp515_set_vindpm(regmap, val->intval);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int axp515_usb_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

const enum power_supply_usb_type axp515_usb_types_array[] = {
    POWER_SUPPLY_USB_TYPE_SDP,
    POWER_SUPPLY_USB_TYPE_DCP,
};

static const struct power_supply_desc axp515_usb_desc = {
	.name = "axp515-usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = axp515_usb_types_array,
	.num_usb_types = ARRAY_SIZE(axp515_usb_types_array),
	.get_property = axp515_usb_get_property,
	.properties = axp515_usb_props,
	.set_property = axp515_usb_set_property,
	.num_properties = ARRAY_SIZE(axp515_usb_props),
	.property_is_writeable = axp515_usb_power_property_is_writeable,
};

static void axp515_irq_limit_input_process(struct axp515_usb_power *usb_power)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;

	if (!axp_config->pmu_bc12_en) {
		if (!axp515_check_acin(usb_power)) {
			axp515_set_iin_limit(usb_power->regmap, axp_config->pmu_usbpc_cur);
			cancel_delayed_work_sync(&usb_power->usb_chg_state);
			__pm_stay_awake(usb_power->vbus_input_check);
			schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(5 * 1000));
		}
		atomic_set(&usb_power->set_current_limit, 0);
		atomic_set(&usb_power->current_limit_sets, 0);
	}
}

static void axp515_typec_iin_limit_check(struct axp515_usb_power *usb_power, int limit_cur_old)
{
	struct axp_config_info *axp_config = &usb_power->dts_info;
	int limit_cur_now, limit_cur_sets, i;

	for (i = 0; i < 10; i++) {
		limit_cur_now = _axp515_get_iin_limit(usb_power);
		PMIC_DEBUG("old_input_cur:%d, now_input_cur:%d\n", limit_cur_old, limit_cur_now);

		if (axp515_check_acin(usb_power)) {
			if (limit_cur_now != axp_config->pmu_usbad_cur) {
				axp515_set_iin_limit(usb_power->regmap, axp_config->pmu_usbad_cur);
			}
			continue;
		}

		limit_cur_sets = atomic_read(&usb_power->current_limit_sets);
		if (limit_cur_sets) {
			if (limit_cur_old != limit_cur_sets) {
				axp515_set_iin_limit(usb_power->regmap, limit_cur_sets);
			}
			continue;
		}

		if (limit_cur_old != limit_cur_now) {
			axp515_set_iin_limit(usb_power->regmap, limit_cur_old);
			break;
		}
		mdelay(10);
	}
}

static irqreturn_t axp515_irq_handler_usb_in(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;

	if (usb_power->vbus_det_used)
		return IRQ_HANDLED;

	mdelay(50);

	atomic_notifier_call_chain(&usb_power_notifier_list, 1, NULL);
	power_supply_changed(usb_power->usb_supply);

	if (!axp515_check_battery(usb_power))
		return IRQ_HANDLED;

	axp515_irq_limit_input_process(usb_power);

	return IRQ_HANDLED;
}

static irqreturn_t axp515_irq_handler_usb_out(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;

	if (usb_power->vbus_det_used)
		return IRQ_HANDLED;

	atomic_notifier_call_chain(&usb_power_notifier_list, 1, NULL);
	atomic_set(&usb_power->set_current_limit, 0);
	atomic_set(&usb_power->current_limit_sets, 0);
	power_supply_changed(usb_power->usb_supply);

	return IRQ_HANDLED;
}

static irqreturn_t axp515_irq_handler_typec_in(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;
	struct axp_config_info *axp_config = &usb_power->dts_info;
	unsigned int reg_val;
	int limit_cur_old;

	regmap_read(usb_power->regmap, AXP515_CC_STATUS0, &reg_val);
	atomic_notifier_call_chain(&usb_power_notifier_list, 0, NULL);
	limit_cur_old = _axp515_get_iin_limit(usb_power);

	switch (reg_val & 0xf) {
	case 0x07:
		extcon_set_state_sync(usb_power->edev, EXTCON_JACK_HEADPHONE, true);
		break;
	case 0x6:
		if (usb_power->vbus_det_used && usb_power->usbid_drv.gpio)
			gpio_direction_output(usb_power->usbid_drv.gpio, 0);
		break;
	default:
		PMIC_INFO("No operation cc_status\n");
	}

	if (!axp_config->pmu_bc12_en) {
		axp515_typec_iin_limit_check(usb_power, limit_cur_old);
	}

	return IRQ_HANDLED;
}

static irqreturn_t axp515_irq_handler_typec_out(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;

	atomic_notifier_call_chain(&usb_power_notifier_list, 0, NULL);
	extcon_set_state_sync(usb_power->edev, EXTCON_JACK_HEADPHONE, false);
	if (usb_power->vbus_det_used && usb_power->usbid_drv.gpio)
		gpio_direction_output(usb_power->usbid_drv.gpio, 1);

	return IRQ_HANDLED;
}

static irqreturn_t axp515_acin_vbus_det_isr(int irq, void *data)
{
	struct axp515_usb_power *usb_power = data;

	cancel_delayed_work_sync(&usb_power->usb_det_mon);
	schedule_delayed_work(&usb_power->usb_det_mon, 0);

	return IRQ_HANDLED;
}

enum axp515_usb_virq_index {
	AXP515_VIRQ_USB_IN,
	AXP515_VIRQ_USB_OUT,
	AXP515_VIRQ_TYPEC_IN,
	AXP515_VIRQ_TYPEC_OUT,

	AXP515_USB_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_usb_irq[] = {
	[AXP515_VIRQ_USB_IN] = { "usb in", axp515_irq_handler_usb_in },
	[AXP515_VIRQ_USB_OUT] = { "usb out", axp515_irq_handler_usb_out },
	[AXP515_VIRQ_TYPEC_IN] = { "tc in", axp515_irq_handler_typec_in },
	[AXP515_VIRQ_TYPEC_OUT] = { "tc out", axp515_irq_handler_typec_out },
};

static void axp515_usb_power_monitor(struct work_struct *work)
{
	struct axp515_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_supply_mon.work);

	schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
}

static void axp515_usb_set_current_fsm(struct work_struct *work)
{
	struct axp515_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_chg_state.work);
	struct axp_config_info *axp_config = &usb_power->dts_info;
	int limit_cur;

	if (atomic_read(&usb_power->set_current_limit)) {
		PMIC_INFO("current limit setted: usb pc type\n");
	} else {
		axp515_set_iin_limit(usb_power->regmap, axp_config->pmu_usbad_cur);
		PMIC_INFO("current limit not set: usb adapter type\n");
	}
	limit_cur = _axp515_get_iin_limit(usb_power);
	atomic_set(&usb_power->current_limit_sets, limit_cur);
	__pm_relax(usb_power->vbus_input_check);
}

static void axp515_usb_det_monitor(struct work_struct *work)
{
	struct axp515_usb_power *usb_power =
		container_of(work, typeof(*usb_power), usb_det_mon.work);
	struct axp_config_info *dinfo = &usb_power->dts_info;
	int vbus_det_gpio_value, limit_cur_old, limit_cur;
	static int vbus_det_gpio_value_old;

	if (!usb_power->vbus_det_used) {
		PMIC_ERR("[usb_det] acin_usb_det not used\n");
		return;
	}

	vbus_det_gpio_value = gpio_get_value(usb_power->axp_vbus_det.gpio);

	if (vbus_det_gpio_value_old == vbus_det_gpio_value) {
		return;
	} else {
		vbus_det_gpio_value_old = vbus_det_gpio_value;
	}

	atomic_notifier_call_chain(&usb_power_notifier_list, 1, NULL);
	power_supply_changed(usb_power->usb_supply);

	PMIC_INFO("[usb_det] vbus_dev_flag = %d\n", vbus_det_gpio_value);

	if (!vbus_det_gpio_value) {
		if (dinfo->pmu_usb_typec_used) {
			limit_cur_old = _axp515_get_iin_limit(usb_power);
			regmap_update_bits(usb_power->regmap, AXP515_CC_MODE_CTRL, 0x03, 0x0);
			mdelay(50);
			regmap_update_bits(usb_power->regmap, AXP515_CC_MODE_CTRL, 0x03, 0x03);
			limit_cur = _axp515_get_iin_limit(usb_power);
			if (limit_cur_old != limit_cur) {
				axp515_set_iin_limit(usb_power->regmap, limit_cur_old);
				PMIC_DEBUG("old_input_cur:%d, now_input_cur:%d\n", limit_cur_old, limit_cur);
			}
		}
	} else {
		if (!axp515_check_battery(usb_power))
			return;

		axp515_irq_limit_input_process(usb_power);
	}
}

static int axp515_acin_vbus_det_init(struct axp515_usb_power *usb_power)
{
	int ret = 0;
	unsigned long irq_flags = 0;

	usb_power->vbus_det_used = 0;
	usb_power->axp_vbus_det.gpio = 0;
	usb_power->axp_vbus_det.irq_num = 0;

	usb_power->axp_vbus_det.gpio =
		of_get_named_gpio(usb_power->dev->of_node,
				"pmu_vbus_det_gpio", 0);
	if (!gpio_is_valid(usb_power->axp_vbus_det.gpio)) {
		PMIC_ERR("get axp_vbus_det_gpio is fail\n");
		usb_power->axp_vbus_det.gpio = 0;
		return -EPROBE_DEFER;
	}

	ret = of_get_named_gpio(usb_power->dev->of_node,
				"pmu_acin_usbid_drv", 0);
	if (ret < 0) {
		PMIC_INFO("no pmu_usbid_drv_gpio\n");
		usb_power->usbid_drv.gpio = 0;
	} else {
		usb_power->usbid_drv.gpio = ret;
		if (!gpio_is_valid(usb_power->usbid_drv.gpio)) {
			PMIC_ERR("get pmu_usbid_drv_gpio is fail\n");
			usb_power->usbid_drv.gpio = 0;
			return -EPROBE_DEFER;
		}
	}

	/* set vbus_det input usbid output */
	ret = gpio_request(
			usb_power->axp_vbus_det.gpio,
			"pmu_vbus_det_gpio");
	if (ret != 0) {
		PMIC_ERR("pmu_vbus_det_gpio gpio_request failed\n");
		return -EINVAL;
	}
	gpio_direction_input(usb_power->axp_vbus_det.gpio);

	if (usb_power->usbid_drv.gpio) {
		ret = gpio_request(
				usb_power->usbid_drv.gpio,
				"pmu_acin_usbid_drv");
		if (ret != 0) {
			PMIC_ERR("pmu_usbid_drv gpio_request failed\n");
			return -EINVAL;
		}
		gpio_direction_output(usb_power->usbid_drv.gpio, 1);
	}

	/* init delay work */
	INIT_DELAYED_WORK(&usb_power->usb_det_mon, axp515_usb_det_monitor);

	/* irq config setting */
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
			IRQF_ONESHOT | IRQF_NO_SUSPEND;
	usb_power->axp_vbus_det.irq_num = gpio_to_irq(usb_power->axp_vbus_det.gpio);

	usb_power->vbus_det_used = 1;
	ret = devm_request_threaded_irq(usb_power->dev, usb_power->axp_vbus_det.irq_num, NULL, axp515_acin_vbus_det_isr, irq_flags,
				"pmu_vbus_det_gpio", usb_power);
	if (IS_ERR_VALUE((unsigned long)ret)) {
		cancel_delayed_work_sync(&usb_power->usb_det_mon);
		PMIC_ERR("Requested pmu_vbus_det_gpio IRQ failed, err %d\n", ret);
		usb_power->vbus_det_used = 0;
		usb_power->axp_vbus_det.gpio = 0;
		usb_power->axp_vbus_det.irq_num = 0;
		return -EINVAL;
	}
	PMIC_DEV_DEBUG(usb_power->dev, "Requested pmu_vbus_det_gpio IRQ successed: %d\n", ret);

	return 0;
}

static void axp515_usb_power_init(struct axp515_usb_power *usb_power)
{
	struct regmap *regmap = usb_power->regmap;
	struct axp_config_info *dinfo = &usb_power->dts_info;

	/* set vindpm value */
	axp515_set_vindpm(regmap, dinfo->pmu_usbad_vol);

	/* set type-c en/disable & mode */
	if (dinfo->pmu_usb_typec_used) {
		regmap_update_bits(regmap, AXP515_CC_EN, BIT(1), BIT(1));
		regmap_update_bits(regmap, AXP515_CC_LOW_POWER_CTRL, BIT(2), 0x00);
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, 0x03, 0x0);
		mdelay(50);
		regmap_update_bits(regmap, AXP515_CC_MODE_CTRL, 0x03, 0x03);
		regmap_update_bits(regmap, AXP515_CC_GLOBAL_CTRL, BIT(5), BIT(5));
	} else {
		regmap_update_bits(regmap, AXP515_CC_EN, BIT(1), 0);
	}

	/* enable die adc */
	regmap_update_bits(regmap, AXP515_ADC_CONTROL, AXP515_ADC_DIETMP_ENABLE, AXP515_ADC_DIETMP_ENABLE);

	/* set bc12 en/disable  */
	if (dinfo->pmu_bc12_en) {
		regmap_update_bits(regmap, AXP515_AUTO_SETS, BIT(6), BIT(6));
	} else {
		regmap_update_bits(regmap, AXP515_AUTO_SETS, BIT(6), 0);
		if (!axp515_check_battery(usb_power))
			axp515_set_iin_limit(usb_power->regmap, dinfo->pmu_usbad_cur);
	}
}

static void axp515_extcon_state_init(struct axp515_usb_power *usb_power)
{
	unsigned int reg_val;

	regmap_read(usb_power->regmap, AXP515_CC_STATUS0, &reg_val);

	if ((reg_val & 0xf) == 0x7)
		extcon_set_state_sync(usb_power->edev, EXTCON_JACK_HEADPHONE, true);
	else
		extcon_set_state_sync(usb_power->edev, EXTCON_JACK_HEADPHONE, false);
}

static int axp515_usb_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}


	AXP_OF_PROP_READ(pmu_usbpc_vol,                    4600);
	AXP_OF_PROP_READ(pmu_usbpc_cur,                    500);
	AXP_OF_PROP_READ(pmu_usbad_vol,                    4600);
	AXP_OF_PROP_READ(pmu_usbad_cur,                    1500);
	AXP_OF_PROP_READ(pmu_bc12_en,                         0);
	AXP_OF_PROP_READ(pmu_usb_typec_used,                  1);

	axp_config->wakeup_usb_in =
		of_property_read_bool(node, "wakeup_usb_in");
	axp_config->wakeup_usb_out =
		of_property_read_bool(node, "wakeup_usb_out");
	axp_config->wakeup_typec_in =
		of_property_read_bool(node, "wakeup_typec_in");
	axp_config->wakeup_typec_out =
		of_property_read_bool(node, "wakeup_typec_out");

	return 0;
}

static void axp515_usb_parse_device_tree(struct axp515_usb_power *usb_power)
{
	int ret;
	struct axp_config_info *cfg;

	/* set input current limit */
	if (!usb_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	cfg = &usb_power->dts_info;
	ret = axp515_usb_dt_parse(usb_power->dev->of_node, cfg);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}

	/*init axp515 usb by device tree*/
	axp515_usb_power_init(usb_power);
}

static int axp515_usb_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;

	struct axp515_usb_power *usb_power;

	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct device_node *np = NULL;

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp515-usb without irq\n");
		return -EINVAL;
	}

	usb_power = devm_kzalloc(&pdev->dev, sizeof(*usb_power), GFP_KERNEL);
	if (usb_power == NULL) {
		PMIC_ERR("axp515_usb_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	usb_power->name = "axp515_usb";
	usb_power->dev = &pdev->dev;
	usb_power->regmap = axp_dev->regmap;

	usb_power->battery_supply_exist = false;
	usb_power->battery_supply_enable = false;

	np = of_parse_phandle(usb_power->dev->of_node, "det_battery_supply", 0);
	if (!np) {
		usb_power->battery_supply_exist = false;
		PMIC_INFO("axp515-battery device is not configed\n");
	} else {
		usb_power->battery_supply_exist = true;
		if (of_device_is_available(np)) {
			usb_power->battery_supply_enable = true;
			PMIC_INFO("axp515-battery device is enabled\n");
		} else {
			usb_power->battery_supply_enable = false;
			PMIC_INFO("axp515-battery device is not enabled\n");
		}
	}

	/* parse device tree and set register */
	axp515_usb_parse_device_tree(usb_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = usb_power;

	usb_power->usb_supply = devm_power_supply_register(usb_power->dev,
			&axp515_usb_desc, &psy_cfg);

	if (IS_ERR(usb_power->usb_supply)) {
		PMIC_ERR("axp515 failed to register usb power\n");
		ret = PTR_ERR(usb_power->usb_supply);
		return ret;
	}

	usb_power->edev = devm_extcon_dev_allocate(usb_power->dev, axp515_usb_extcon_cable);
	if (IS_ERR(usb_power->edev)) {
		PMIC_DEV_ERR(usb_power->dev, "failed to allocate extcon device\n");
		return -ENOMEM;
	}

	ret = devm_extcon_dev_register(usb_power->dev, usb_power->edev);
	if (ret < 0) {
		PMIC_DEV_ERR(usb_power->dev, "failed to register extcon device\n");
		return ret;
	}

	axp515_extcon_state_init(usb_power);

	if (!usb_power->dts_info.pmu_bc12_en) {
		INIT_DELAYED_WORK(&usb_power->usb_supply_mon, axp515_usb_power_monitor);
		INIT_DELAYED_WORK(&usb_power->usb_chg_state, axp515_usb_set_current_fsm);
	}

	np = of_parse_phandle(usb_power->dev->of_node, "det_acin_supply", 0);
	usb_power->acin_supply = NULL;
	if (!of_device_is_available(np)) {
		PMIC_INFO("axp515-acin device is not configed, not use vbus-det\n");
	} else {
		ret = axp515_acin_vbus_det_init(usb_power);
		if (ret < 0) {
			PMIC_ERR("failed to register axp515-acin function\n");
			return ret;
		}
	}

	for (i = 0; i < ARRAY_SIZE(axp_usb_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_usb_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_DEV_ERR(&pdev->dev, "can not get irq\n");
			ret = irq;
			goto cancel_work;
		}
		/* we use this variable to suspend irq */
		axp_usb_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_usb_irq[i].isr, 0,
						   axp_usb_irq[i].name, usb_power);
		if (ret < 0) {
			PMIC_DEV_ERR(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_usb_irq[i].name, irq, ret);
			goto cancel_work;
		} else {
			ret = 0;
		}

		PMIC_DEV_DEBUG(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_usb_irq[i].name, irq, ret);
	}

	if (!usb_power->dts_info.pmu_bc12_en) {
		schedule_delayed_work(&usb_power->usb_supply_mon, msecs_to_jiffies(500));
		usb_power->vbus_input_check = wakeup_source_register(usb_power->dev, "vbus_input_check");
		__pm_stay_awake(usb_power->vbus_input_check);
		schedule_delayed_work(&usb_power->usb_chg_state, msecs_to_jiffies(20 * 1000));
	}

	platform_set_drvdata(pdev, usb_power);

	return ret;

cancel_work:
	if (!usb_power->dts_info.pmu_bc12_en) {
		cancel_delayed_work_sync(&usb_power->usb_supply_mon);
		cancel_delayed_work_sync(&usb_power->usb_chg_state);
	}


err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp515_usb_remove(struct platform_device *pdev)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(pdev);

	if (!usb_power->dts_info.pmu_bc12_en) {
		cancel_delayed_work_sync(&usb_power->usb_supply_mon);
		cancel_delayed_work_sync(&usb_power->usb_chg_state);
	}
	if (usb_power->vbus_det_used)
		cancel_delayed_work_sync(&usb_power->usb_det_mon);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP515 usb unegister==============\n");
	if (usb_power->usb_supply)
		power_supply_unregister(usb_power->usb_supply);
	PMIC_DEV_DEBUG(&pdev->dev, "axp515 teardown usb dev\n");

	return 0;
}

static inline void axp515_usb_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp515_usb_virq_dts_set(struct axp515_usb_power *usb_power, bool enable)
{
	struct axp_config_info *dts_info = &usb_power->dts_info;

	if (!dts_info->wakeup_usb_in)
		axp515_usb_irq_set(axp_usb_irq[AXP515_VIRQ_USB_IN].irq,
				enable);

	if (!dts_info->wakeup_usb_out)
		axp515_usb_irq_set(axp_usb_irq[AXP515_VIRQ_USB_OUT].irq,
				enable);

	if (dts_info->pmu_usb_typec_used) {
		if (!dts_info->wakeup_typec_in)
			axp515_usb_irq_set(axp_usb_irq[AXP515_VIRQ_TYPEC_IN].irq,
				enable);
		if (!dts_info->wakeup_typec_out)
			axp515_usb_irq_set(axp_usb_irq[AXP515_VIRQ_TYPEC_OUT].irq,
					enable);
	}

	if (usb_power->vbus_det_used) {
		axp515_usb_irq_set(usb_power->axp_vbus_det.irq_num,
				enable);
	}
}

static void axp515_usb_shutdown(struct platform_device *pdev)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(pdev);

	if (!usb_power->dts_info.pmu_bc12_en) {
		cancel_delayed_work_sync(&usb_power->usb_supply_mon);
		cancel_delayed_work_sync(&usb_power->usb_chg_state);
	}
	if (usb_power->vbus_det_used) {
		cancel_delayed_work_sync(&usb_power->usb_det_mon);
		axp515_usb_irq_set(usb_power->axp_vbus_det.irq_num,
				false);
	}
}

static int axp515_usb_suspend(struct platform_device *p, pm_message_t state)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(p);

	axp515_usb_virq_dts_set(usb_power, false);

	if (!usb_power->dts_info.pmu_bc12_en) {
		cancel_delayed_work_sync(&usb_power->usb_supply_mon);
		cancel_delayed_work_sync(&usb_power->usb_chg_state);
	}
	if (usb_power->vbus_det_used)
		cancel_delayed_work_sync(&usb_power->usb_det_mon);

	return 0;
}

static int axp515_usb_resume(struct platform_device *p)
{
	struct axp515_usb_power *usb_power = platform_get_drvdata(p);

	if (!usb_power->dts_info.pmu_bc12_en) {
		schedule_delayed_work(&usb_power->usb_supply_mon, 0);
		schedule_delayed_work(&usb_power->usb_chg_state, 0);
	}
	if (usb_power->vbus_det_used)
		schedule_delayed_work(&usb_power->usb_det_mon, 0);

	axp515_usb_virq_dts_set(usb_power, true);

	return 0;
}

static const struct of_device_id axp515_usb_power_match[] = {
	{
		.compatible = "x-powers,axp515-usb-power-supply",
		.data = (void *)AXP515_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp515_usb_power_match);

static struct platform_driver axp515_usb_power_driver = {
	.driver = {
		.name = "axp515-usb-power-supply",
		.of_match_table = axp515_usb_power_match,
	},
	.probe = axp515_usb_probe,
	.remove = axp515_usb_remove,
	.shutdown = axp515_usb_shutdown,
	.suspend = axp515_usb_suspend,
	.resume = axp515_usb_resume,
};

module_platform_driver(axp515_usb_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp515 usb driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
