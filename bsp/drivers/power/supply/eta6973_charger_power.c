/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "eta6973_charger.h"

#define DIVIDE_BY_60(n) (((n) / 60) * 60)

struct eta6973_power {
	char				*name;
	struct device			*dev;
	struct regmap			*regmap;
	struct power_supply		*charger_supply;
	struct bmu_ext_config_info	dts_info;
	struct delayed_work		charger_supply_mon;
	unsigned int 			reg_val_08_old;
	unsigned int 			reg_val_09_old;
	unsigned int 			reg_val_0a_old;
	int				charger_type;
	int 				gpio_int_n;
	int 				gpio_int_n_irq;
	int 				vbus_set_limit;
	int 				vbus_set_vol_limit;
	int 				battery_exist;

	bool				bat_supply_exist;
	bool				bat_supply_probe_finish;
	struct delayed_work		bat_supply_det;
	struct power_supply		*bat_supply;
	struct device_node		*bat_supply_np;

	struct delayed_work		bat_qc_temp_process;
	struct wakeup_source		*bat_qc_temp_ws;

	atomic_t			charge_over_time;
	atomic_t			pmu_jetia_status;
	atomic_t			pmu_limit_status;
	atomic_t 			set_current_limit_max;

	atomic_t 			charge_set_limit;
	atomic_t 			charge_control_lim;
};

static enum power_supply_property eta6973_charger_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_SCOPE,
	POWER_SUPPLY_PROP_CALIBRATE,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
};

static void eta6973_power_init_late(struct eta6973_power *charger_power);
static int eta6973_battery_power_get(struct eta6973_power *charger_power);
static void eta6973_temp_process_monitor(struct work_struct *work);

static int _eta6973_battery_para_get(struct eta6973_power *charger_power, enum power_supply_property psp, const char *name, int paras)
{
	union power_supply_propval val;
	int _paras = -1;

	if (name != NULL) {
		of_property_read_u32(charger_power->bat_supply_np, name, &_paras);
	}

	if (_paras < 0) {
		val.intval = paras;
	} else {
		val.intval = _paras;
	}

	_paras = psp;

	if (_paras > -1) {
		power_supply_get_property(charger_power->bat_supply, psp, &val);
	}

	return val.intval;
}

static int eta6973_battery_para_get(struct eta6973_power *charger_power)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	union power_supply_propval val;
	static int check_count = 1;

	if (eta6973_battery_power_get(charger_power)) {
		bmp_dts_info->pmu_bat_temp_enable = 0;
		return -1;
	}

	if (check_count == 2)
		return 0;

	/* get ntc paras */
	if (!charger_power->battery_exist) {
		bmp_dts_info->pmu_bat_temp_enable = 0;
	} else {
		bmp_dts_info->pmu_bat_temp_enable = _eta6973_battery_para_get(charger_power, -1, "pmu_bat_temp_enable", 0);
		if (bmp_dts_info->pmu_bat_temp_enable) {
			bmp_dts_info->pmu_bat_charge_ltf = _eta6973_battery_para_get(charger_power, POWER_SUPPLY_PROP_TEMP, "pmu_bat_charge_ltf", -200000);
			bmp_dts_info->pmu_bat_charge_htf = _eta6973_battery_para_get(charger_power, POWER_SUPPLY_PROP_TEMP, "pmu_bat_charge_htf", 200000);
			of_property_read_u32(charger_power->bat_supply_np, "pmu_jetia_en", &bmp_dts_info->pmu_jetia_en);
			if (bmp_dts_info->pmu_jetia_en) {
				bmp_dts_info->pmu_jetia_cool = _eta6973_battery_para_get(charger_power, POWER_SUPPLY_PROP_TEMP, "pmu_jetia_cool", -200000);
				bmp_dts_info->pmu_jetia_warm = _eta6973_battery_para_get(charger_power, POWER_SUPPLY_PROP_TEMP, "pmu_jetia_warm", 200000);
			}
		}
	}

	/* get ichg limit paras */
	bmp_dts_info->pmu_runtime_chgcur = DIVIDE_BY_60(_eta6973_battery_para_get(charger_power, -1, "pmu_runtime_chgcur", bmp_dts_info->pmu_runtime_chgcur));
	bmp_dts_info->pmu_suspend_chgcur = DIVIDE_BY_60(_eta6973_battery_para_get(charger_power, -1, "pmu_suspend_chgcur", bmp_dts_info->pmu_suspend_chgcur));
	bmp_dts_info->pmu_shutdown_chgcur = DIVIDE_BY_60(_eta6973_battery_para_get(charger_power, -1, "pmu_shutdown_chgcur", bmp_dts_info->pmu_shutdown_chgcur));

	/* get charger voltage limit */
	bmp_dts_info->pmu_init_chgvol = _eta6973_battery_para_get(charger_power, -1, "pmu_init_chgvol", bmp_dts_info->pmu_init_chgvol);
	if (bmp_dts_info->pmu_init_chgvol > ETA6973_VTERM_MAX)
		bmp_dts_info->pmu_init_chgvol = ETA6973_VTERM_MAX;
	if (bmp_dts_info->pmu_init_chgvol < ETA6973_VTERM_MIN)
		bmp_dts_info->pmu_init_chgvol = ETA6973_VTERM_MIN;

	PMIC_INFO("pmu_bat_temp_enable:%d\n", bmp_dts_info->pmu_bat_temp_enable);
	PMIC_INFO("pmu_bat_charge_ltf:%d\n", bmp_dts_info->pmu_bat_charge_ltf);
	PMIC_INFO("pmu_bat_charge_htf:%d\n", bmp_dts_info->pmu_bat_charge_htf);
	PMIC_INFO("pmu_jetia_cool:%d\n", bmp_dts_info->pmu_jetia_cool);
	PMIC_INFO("pmu_jetia_warm:%d\n", bmp_dts_info->pmu_jetia_warm);
	PMIC_INFO("pmu_runtime_chgcur:%d\n", bmp_dts_info->pmu_runtime_chgcur);
	PMIC_INFO("pmu_suspend_chgcur:%d\n", bmp_dts_info->pmu_suspend_chgcur);
	PMIC_INFO("pmu_shutdown_chgcur:%d\n", bmp_dts_info->pmu_shutdown_chgcur);
	PMIC_INFO("pmu_init_chgvol:%d\n", bmp_dts_info->pmu_init_chgvol);

	val.intval = 1;
	power_supply_set_property(charger_power->bat_supply, POWER_SUPPLY_PROP_CALIBRATE, &val);

	check_count = 2;
	return 0;
}

static void _eta6973_battery_power_check(struct work_struct *work)
{
	struct eta6973_power *charger_power =
		container_of(work, typeof(*charger_power), bat_supply_det.work);

	if (!(charger_power->bat_supply) || (IS_ERR(charger_power->bat_supply))) {
		charger_power->bat_supply =  NULL;
		charger_power->bat_supply_np =  NULL;
		charger_power->bat_supply_exist = false;
	}

	eta6973_battery_para_get(charger_power);
	eta6973_power_init_late(charger_power);

	if (charger_power->dts_info.pmu_bat_temp_enable) {
		charger_power->bat_qc_temp_ws = wakeup_source_register(charger_power->dev, "bat_qc_temp_proce");
		INIT_DELAYED_WORK(&charger_power->bat_qc_temp_process, eta6973_temp_process_monitor);
		schedule_delayed_work(&charger_power->bat_qc_temp_process, msecs_to_jiffies(5 * 1000));
	}

	PMIC_INFO("bmu-ext:chip init late finish\n");
	power_supply_changed(charger_power->charger_supply);
}

static int eta6973_battery_power_get(struct eta6973_power *charger_power)
{
	struct device_node *np = NULL;
	static int check_count = 1;
	int ret = 0;

	if (charger_power->bat_supply_exist)
		return ret;

	if (check_count == 2)
		return -1;

	charger_power->bat_supply_exist = false;
	charger_power->bat_supply_probe_finish = false;

	if (of_find_property(charger_power->dev->of_node, "det_battery_supply", NULL)) {
		np = of_parse_phandle(charger_power->dev->of_node, "det_battery_supply", 0);
		if (!of_device_is_available(np)) {
			charger_power->bat_supply =  NULL;
			charger_power->bat_supply_np =  NULL;
			ret = -1;
		} else {
			charger_power->bat_supply = devm_power_supply_get_by_phandle(charger_power->dev,
							"det_battery_supply");
			charger_power->bat_supply_np = of_parse_phandle(charger_power->dev->of_node, "det_battery_supply", 0);
			if (!(charger_power->bat_supply) || (IS_ERR(charger_power->bat_supply))) {
				INIT_DELAYED_WORK(&charger_power->bat_supply_det, _eta6973_battery_power_check);
				charger_power->bat_supply_exist = true;
				return -1;
			}
		}
	} else {
			charger_power->bat_supply =  NULL;
			charger_power->bat_supply_np =  NULL;
			ret = -1;
	}

	if (!ret)
		charger_power->bat_supply_exist = true;

	check_count = 2;
	return ret;
}

static int eta6973_charger_input_limit(struct eta6973_power *charger_power, int mA)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	int input_vol, input_cur;

	input_vol = charger_power->vbus_set_vol_limit;
	input_cur = mA;

	if (input_cur * input_vol > bmp_dts_info->bmu_ext_max_power)
		input_cur = bmp_dts_info->bmu_ext_max_power / input_vol;

	PMIC_DEBUG("input_cur:%d input_vol:%d max_power:%d\n", input_cur, input_vol, bmp_dts_info->bmu_ext_max_power);

	return input_cur;
}

static int eta6973_get_vbus_online(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, ETA6973_REG_0A, &data);
	if (ret < 0)
		return ret;

	/* vbus is good when vbus state set */
	val->intval = !!(data & ETA6973_REG0A_VBUS_GD_MASK);

	return ret;
}

static int eta6973_get_bat_status(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret;

	ret = regmap_read(regmap, ETA6973_REG_08, &data);
	if (ret < 0) {
		PMIC_DEV_DEBUG(&ps->dev, "error read ETA6973_REG_08\n");
		return ret;
	}

	/* chg_stat = bit[2:0] */
	switch ((data & ETA6973_REG08_CHRG_STAT_MASK) >> ETA6973_REG08_CHRG_STAT_SHIFT) {
	case ETA6973_REG08_CHRG_STAT_IDLE:
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case ETA6973_REG08_CHRG_STAT_PRECHG:
	case ETA6973_REG08_CHRG_STAT_FASTCHG:
		val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case ETA6973_REG08_CHRG_STAT_CHGDONE:
		val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	default:
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	return 0;
}

static int eta6973_set_bat_status(struct regmap *regmap, int status)
{
	unsigned int data;
	int ret = 0;

	if (!status)
		return 0;

	data = status - 1;
	ret = regmap_update_bits(regmap, ETA6973_REG_01, ETA6973_REG01_CHG_CONFIG_MASK,
				 data << ETA6973_REG01_CHG_CONFIG_SHIFT);
	ret = regmap_read(regmap, ETA6973_REG_01, &data);
	PMIC_DEBUG("%s:%d ETA6973_REG_01:0x%x\n", __func__, __LINE__, data);
	if (ret < 0)
		return ret;

	return 0;
}

static int eta6973_get_ovp_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	/* get OVP */
	ret = regmap_read(regmap, ETA6973_REG_06, &data);
	if (ret < 0)
		return ret;

	switch ((data & ETA6973_REG06_OVP_MASK) >> ETA6973_REG06_OVP_SHIFT) {
	case ETA6973_REG06_OVP_5P5V:
		val->intval = 5500;
		break;
	case ETA6973_REG06_OVP_6P2V:
		val->intval = 6500;
		break;
	case ETA6973_REG06_OVP_10P5V:
		val->intval = 10500;
		break;
	case ETA6973_REG06_OVP_14P3V:
		val->intval = 14000;
		break;
	}

	return 0;
}

static int eta6973_set_ovp_vol(struct regmap *regmap, int mV)
{
	unsigned int data;
	int ret = 0;

	data = mV;

	if (data > 10500)
		data = ETA6973_REG06_OVP_14P3V;
	else if	(data > 6500)
		data = ETA6973_REG06_OVP_10P5V;
	else if	(data > 5500)
		data = ETA6973_REG06_OVP_6P2V;
	else
		data = ETA6973_REG06_OVP_5P5V;

	ret = regmap_update_bits(regmap, ETA6973_REG_06, ETA6973_REG06_OVP_MASK,
				 data << ETA6973_REG06_OVP_SHIFT);
	if (ret < 0)
		return ret;

	return 0;
}

static int eta6973_get_iin_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, ETA6973_REG_00, &data);
	if (ret < 0)
		return ret;
	data &= ETA6973_REG00_IINLIM_MASK;
	data = (data * 100) + 100;
	val->intval = data;
	return ret;
}

static int eta6973_set_iin_limit(struct eta6973_power *charger_power, int mA)
{
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	data = eta6973_charger_input_limit(charger_power, mA);

	if (data > 3200)
		data = 3200;
	if (data < 100)
		data = 100;
	data = ((data - 100) / 100);
	ret = regmap_update_bits(regmap, ETA6973_REG_00, ETA6973_REG00_IINLIM_MASK,
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int eta6973_get_ichg_limit(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, ETA6973_REG_02, &data);
	if (ret < 0)
		return ret;
	data &= ETA6973_REG02_ICHG_MASK;
	data = data * 60;

	val->intval = data;
	return ret;
}

static int eta6973_set_ichg_limit(struct regmap *regmap, int mA)
{
	unsigned int data;
	int ret = 0;

	data = mA;
	if (data > 3000)
		data = 3000;
	if (data < 60)
		data = 60;
	data = data / 60;
	ret = regmap_update_bits(regmap, ETA6973_REG_02, ETA6973_REG02_ICHG_MASK,
				 data);
	if (ret < 0)
		return ret;

	ret = regmap_read(regmap, ETA6973_REG_02, &data);
	PMIC_DEBUG("%s:%d ETA6973_REG_02:0x%x set cur:%d\n", __func__, __LINE__, data, mA);
	return 0;
}

static int eta6973_ichg_limit_ext(struct eta6973_power *charger_power, int new_cur)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	bool limit_status = false, jetia_status = false;
	int old_cur, charge_lim, set_cur, jetia_cur;

	if (!charger_power->bat_supply_probe_finish) {
		return 0;
	}

	limit_status = atomic_read(&charger_power->pmu_limit_status);
	jetia_status = atomic_read(&charger_power->pmu_jetia_status);
	old_cur = atomic_read(&charger_power->charge_set_limit);
	jetia_cur = bmp_dts_info->pmu_runtime_chgcur / 2;

	if (new_cur == 0 || old_cur == new_cur)
		return old_cur;

	if (limit_status) {
		charge_lim = atomic_read(&charger_power->charge_control_lim);
		if (jetia_status && (charge_lim > jetia_cur)) {
			set_cur = jetia_cur;
		} else {
			set_cur = charge_lim;
		}
	} else {
		if (jetia_status) {
			set_cur = jetia_cur;
		} else {
			set_cur = new_cur;
		}
	}
	atomic_set(&charger_power->charge_set_limit, set_cur);
	eta6973_set_ichg_limit(charger_power->regmap, set_cur);
	return set_cur;
}

static int eta6973_get_time_to_full(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, ETA6973_REG_05, &data);
	if (ret < 0)
		return ret;

	if (!(data & ETA6973_REG05_EN_TIMER_MASK)) {
		val->intval = 0;
		return ret;
	}


	data = (data & ETA6973_REG05_CHG_TIMER_MASK) >> ETA6973_REG05_CHG_TIMER_SHIFT;
	if (data & ETA6973_REG05_CHG_TIMER_10HOURS)
		val->intval = 10;
	else
		val->intval = 5;

	return ret;
}

static int eta6973_set_time_to_full(struct regmap *regmap, int hour)
{
	unsigned int data;
	int ret = 0;

	data = hour;
	if (data == 0) {
		ret = regmap_update_bits(regmap, ETA6973_REG_05, ETA6973_REG05_EN_TIMER_MASK, 0);
		if (ret < 0)
			return ret;

		return 0;
	} else {
		ret = regmap_update_bits(regmap, ETA6973_REG_05, ETA6973_REG05_EN_TIMER_MASK, ETA6973_REG05_EN_TIMER_MASK);
	}

	if (data > 5)
		data = 1;
	else
		data = 0;
	ret = regmap_update_bits(regmap, ETA6973_REG_05, ETA6973_REG05_CHG_TIMER_MASK,
				 data << ETA6973_REG05_CHG_TIMER_SHIFT);
	if (ret < 0)
		return ret;

	return 0;
}

static int eta6973_get_scope(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = charger_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, ETA6973_REG_05, &data);
	if (ret < 0)
		return ret;
	if (!(data & ETA6973_REG01_WDT_RESET_MASK)) {
		ret = regmap_read(regmap, ETA6973_REG_01, &data);
		if (!(data & ETA6973_REG01_WDT_RESET_MASK)) {
			/* not use watchdog */
			val->intval = POWER_SUPPLY_SCOPE_SYSTEM;
			return ret;
		}
	}
	/* used watchdog */
	val->intval = POWER_SUPPLY_SCOPE_DEVICE;

	return ret;
}

static int eta6973_set_scope(struct regmap *regmap, int time)
{
	u32 value, tmpval = time;

	if (tmpval) {
		regmap_read(regmap, ETA6973_REG_05, &value);
		value &= ~(ETA6973_REG05_WDT_MASK);
		switch (tmpval) {
		case 80:
			tmpval |= ETA6973_REG05_WDT_80S;
			break;
		case 160:
			tmpval |= ETA6973_REG05_WDT_160S;
			break;
		default:
			tmpval |= ETA6973_REG05_WDT_40S;
			break;
		}
		value |= tmpval << ETA6973_REG05_WDT_SHIFT;
		regmap_update_bits(regmap, ETA6973_REG_05, ETA6973_REG05_WDT_MASK, value);
		regmap_update_bits(regmap, ETA6973_REG_01, ETA6973_REG01_WDT_RESET_MASK, ETA6973_REG01_WDT_RESET_MASK);
	} else {
		regmap_update_bits(regmap, ETA6973_REG_05, ETA6973_REG05_WDT_MASK, ETA6973_REG05_WDT_DISABLE);
	}

	return 0;
}

static int eta6973_charger_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(psy);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = eta6973_get_vbus_online(psy, val);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = eta6973_get_vbus_online(psy, val);
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = ETA6973_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		if (!charger_power->battery_exist) {
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
			break;
		}
		ret = eta6973_get_bat_status(psy, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		if (!charger_power->battery_exist) {
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
			break;
		}
		if (atomic_read(&charger_power->charge_over_time))
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = eta6973_get_ovp_vol(psy, val);
		break;
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = eta6973_get_iin_limit(psy, val);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = eta6973_get_time_to_full(psy, val);
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = eta6973_get_scope(psy, val);
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		val->intval = charger_power->battery_exist;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		if (charger_power->bat_supply_exist && (val->intval > 0)) {
			if (!charger_power->bat_supply_probe_finish) {
				val->intval = DIVIDE_BY_60(val->intval);
				break;
			}
		}
		if (atomic_read(&charger_power->pmu_jetia_status)) {
			if (!atomic_read(&charger_power->pmu_limit_status)) {
				val->intval = bmp_dts_info->pmu_runtime_chgcur;
			} else {
				val->intval = atomic_read(&charger_power->charge_control_lim);
			}
		} else {
			ret = eta6973_get_ichg_limit(psy, val);
		}
		break;
	default:
		break;
	}

	return ret;
}

static int eta6973_charger_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct eta6973_power *charger_power = power_supply_get_drvdata(psy);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	unsigned int reg_val;
	int ret = 0, qc_cur;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		qc_cur = val->intval;
		charger_power->vbus_set_limit = qc_cur;
		if (qc_cur == 0) {
			charger_power->vbus_set_limit = charger_power->dts_info.pmu_usbad_input_cur;
			break;
		}
		if (!charger_power->battery_exist && qc_cur < charger_power->dts_info.pmu_usbad_input_cur) {
			ret = eta6973_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbad_input_cur);
			break;
		} else {
			regmap_read(regmap, ETA6973_REG_08, &reg_val);
			if (reg_val == 0x00)
				atomic_set(&charger_power->set_current_limit_max, 0);
			reg_val = (reg_val & ETA6973_REG08_VBUS_STAT_MASK) >> ETA6973_REG08_VBUS_STAT_SHIFT;
			if (charger_power->charger_type == 1) {
				if (reg_val == ETA6974_REG08_VBUS_TYPE_SDP) {
					qc_cur = 500;
				}
				if (reg_val == ETA6974_REG08_VBUS_TYPE_CDP) {
					qc_cur = 1500;
				}
			} else {
				if (reg_val == ETA6973_REG08_VBUS_TYPE_USB) {
					qc_cur = 500;
				}
			}
		}

		if (charger_power->dts_info.pmu_usbpc_input_cur < qc_cur) {
			atomic_set(&charger_power->set_current_limit_max, 1);
			charger_power->vbus_set_limit = qc_cur;
		}
		ret = eta6973_set_iin_limit(charger_power, qc_cur);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if (val->intval == 0)
			break;
		charger_power->vbus_set_vol_limit = val->intval;
		ret = eta6973_set_ovp_vol(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = eta6973_set_time_to_full(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = eta6973_set_scope(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = eta6973_set_bat_status(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		qc_cur = atomic_read(&charger_power->charge_set_limit);

		if (val->intval < bmp_dts_info->pmu_runtime_chgcur) {
			atomic_set(&charger_power->charge_control_lim, val->intval);
			atomic_set(&charger_power->pmu_limit_status, true);
		} else {
			atomic_set(&charger_power->pmu_limit_status, false);
		}
		eta6973_ichg_limit_ext(charger_power, val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		if (charger_power->bat_supply_exist) {
			if (val->intval == 1) {
				schedule_delayed_work(&charger_power->bat_qc_temp_process, 0);
			}
		}
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		if (charger_power->bat_supply_exist) {
			if ((val->intval == 1) && !charger_power->bat_supply_probe_finish) {
				charger_power->bat_supply_probe_finish = true;
				schedule_delayed_work(&charger_power->bat_supply_det, 0);
				break;
			}
		}
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int eta6973_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_SCOPE:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static void eta6973_irq_vbus_stat(struct eta6973_power *charger_power)
{
	struct regmap *regmap = charger_power->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ETA6973_REG_08, &reg_val);

	reg_val = (reg_val & ETA6973_REG08_VBUS_STAT_MASK) >> ETA6973_REG08_VBUS_STAT_SHIFT;
	PMIC_INFO("%s:%d ETA6973_REG_08:%d\n", __func__, __LINE__, reg_val);

	if (!charger_power->battery_exist) {
		eta6973_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbad_input_cur);
		return;
	}

	if (charger_power->charger_type == 1) {
		switch (reg_val) {
		case ETA6974_REG08_VBUS_TYPE_UNKNOW:
		case ETA6974_REG08_VBUS_TYPE_NON_STANDER:
		case ETA6974_REG08_VBUS_TYPE_DCP:
			break;
		case ETA6974_REG08_VBUS_TYPE_SDP:
		case ETA6974_REG08_VBUS_TYPE_CDP:
		default:
			atomic_set(&charger_power->set_current_limit_max, 0);
		}
	} else {
		switch (reg_val) {
		case ETA6973_REG08_VBUS_TYPE_ADAPTER:
			break;
		case ETA6973_REG08_VBUS_TYPE_USB:
		default:
			atomic_set(&charger_power->set_current_limit_max, 0);
		}
	}

	if (atomic_read(&charger_power->set_current_limit_max)) {
		atomic_set(&charger_power->set_current_limit_max, 0);
		eta6973_set_iin_limit(charger_power, charger_power->vbus_set_limit);
	}

}

static void eta6973_irq_vbus_gd(struct eta6973_power *charger_power)
{
	struct regmap *regmap = charger_power->regmap;
	unsigned int reg_val;

	power_supply_changed(charger_power->charger_supply);
	regmap_read(regmap, ETA6973_REG_0A, &reg_val);
	PMIC_INFO("%s:%d ETA6973_REG_0A:0x%x\n", __func__, __LINE__, reg_val);

	if (!charger_power->battery_exist) {
		return;
	}

	if (!(reg_val & ETA6973_REG0A_VBUS_GD_MASK)) {
		atomic_set(&charger_power->set_current_limit_max, 0);
		charger_power->vbus_set_limit = charger_power->dts_info.pmu_usbpc_input_cur;
		charger_power->vbus_set_vol_limit = charger_power->dts_info.pmu_usbpc_input_vol;
		eta6973_set_ovp_vol(charger_power->regmap, charger_power->vbus_set_vol_limit);
		eta6973_set_iin_limit(charger_power, charger_power->vbus_set_limit);
	}
}

static void eta6973_irq_fault(struct eta6973_power *charger_power, unsigned int reg_val_old, unsigned int reg_val_new)
{
	struct regmap *regmap = charger_power->regmap;
	unsigned int reg_val;

	regmap_read(regmap, ETA6973_REG_09, &reg_val);
	if (reg_val & (ETA6973_REG09_FAULT_CHRG_TIMER << ETA6973_REG09_FAULT_CHRG_SHIFT)) {
		if (!atomic_read(&charger_power->charge_over_time))
			atomic_set(&charger_power->charge_over_time, true);
	} else {
		if (atomic_read(&charger_power->charge_over_time))
			atomic_set(&charger_power->charge_over_time, false);
	}

	PMIC_INFO("%s:%d ETA_FAULT:ETA6973_REG_09:0x%x, reg_val_old:0x%x, reg_val_new:0x%x\n", __func__, __LINE__, reg_val, reg_val_old, reg_val_new);
}

static void eta6973_irq_reg(struct eta6973_power *charger_power,
			     unsigned int reg, unsigned int reg_val_old, unsigned int reg_val_new)
{
	unsigned int data = reg_val_old ^ reg_val_new;

	if (reg == ETA6973_REG_08) {
		if (data & ETA6973_REG08_VBUS_STAT_MASK)
			eta6973_irq_vbus_stat(charger_power);
	}
	if (reg == ETA6973_REG_09) {
		eta6973_irq_fault(charger_power, reg_val_old, reg_val_new);
	}
	if (reg == ETA6973_REG_0A) {
		if (data & ETA6973_REG0A_VBUS_GD_MASK)
			eta6973_irq_vbus_gd(charger_power);
	}
}

static irqreturn_t eta6973_irq_handler(int irq, void *data)
{
	struct eta6973_power *charger_power = data;
	struct regmap *regmap = charger_power->regmap;
	unsigned int reg_val_new;

	regmap_read(regmap, ETA6973_REG_08, &reg_val_new);
	if (charger_power->reg_val_08_old ^ reg_val_new)
		eta6973_irq_reg(charger_power, ETA6973_REG_08, charger_power->reg_val_08_old, reg_val_new);
	charger_power->reg_val_08_old = reg_val_new;

	regmap_read(regmap, ETA6973_REG_09, &reg_val_new);
	if (charger_power->reg_val_09_old ^ reg_val_new)
		eta6973_irq_reg(charger_power, ETA6973_REG_09, charger_power->reg_val_09_old, reg_val_new);
	charger_power->reg_val_09_old = reg_val_new;

	regmap_read(regmap, ETA6973_REG_0A, &reg_val_new);
	if (charger_power->reg_val_0a_old ^ reg_val_new)
		eta6973_irq_reg(charger_power, ETA6973_REG_0A, charger_power->reg_val_0a_old, reg_val_new);
	charger_power->reg_val_0a_old = reg_val_new;


	return IRQ_HANDLED;
}

static const struct power_supply_desc eta6973_charger_desc = {
	.name = "eta6973-charger",
	.type = POWER_SUPPLY_TYPE_USB,
	.get_property = eta6973_charger_get_property,
	.properties = eta6973_charger_props,
	.set_property = eta6973_charger_set_property,
	.num_properties = ARRAY_SIZE(eta6973_charger_props),
	.property_is_writeable = eta6973_power_property_is_writeable,
};

static void eta6973_power_monitor(struct work_struct *work)
{
	struct eta6973_power *charger_power =
		container_of(work, typeof(*charger_power), charger_supply_mon.work);
	schedule_delayed_work(&charger_power->charger_supply_mon, msecs_to_jiffies(1 * 1000));
}

static void eta6973_temp_process_monitor(struct work_struct *work)
{
	struct eta6973_power *charger_power =
		container_of(work, typeof(*charger_power), bat_qc_temp_process.work);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	union power_supply_propval bat_temp_val;
	static int charge_status = 2;
	int tmp, qc_status_val = 0, qc_cur_val = 0;
	bool jetia_status = atomic_read(&charger_power->pmu_jetia_status);

	power_supply_get_property(charger_power->bat_supply, POWER_SUPPLY_PROP_TEMP, &bat_temp_val);
	tmp = bat_temp_val.intval;

	if (charge_status != 1 && ((tmp <= bmp_dts_info->pmu_bat_charge_ltf) || (tmp >= bmp_dts_info->pmu_bat_charge_htf))) {
		PMIC_DEBUG("bat temp(%d) is over/under than chage mode temp(%d/%d): hold wake lock and discharge\n", tmp,
				 bmp_dts_info->pmu_bat_charge_htf, bmp_dts_info->pmu_bat_charge_ltf);
		qc_status_val = 1;
	} else if (charge_status == 1 && (tmp > (bmp_dts_info->pmu_bat_charge_ltf + 30) && (tmp < (bmp_dts_info->pmu_bat_charge_htf - 30)))) {
		PMIC_DEBUG("release wake lock and recharge:bat_temp:%d\n", tmp);
		qc_status_val = 2;
	}

	if ((charge_status != qc_status_val) && (qc_status_val != 0)) {
		charge_status = qc_status_val;
		eta6973_set_bat_status(regmap, charge_status);
		if (charge_status == 1) {
			__pm_stay_awake(charger_power->bat_qc_temp_ws);
		} else {
			__pm_relax(charger_power->bat_qc_temp_ws);
		}
	}

	if (bmp_dts_info->pmu_jetia_en && (charge_status == 2)) {
		if ((tmp <= bmp_dts_info->pmu_jetia_cool) || (tmp >= bmp_dts_info->pmu_jetia_warm)) {
			PMIC_DEBUG("bat temp(%d) is over/under than chage mode temp(%d/%d):limit chgcur\n", tmp,
					  bmp_dts_info->pmu_jetia_warm, bmp_dts_info->pmu_jetia_cool);
			qc_cur_val = bmp_dts_info->pmu_runtime_chgcur / 2;
			jetia_status = true;
		} else if ((tmp > (bmp_dts_info->pmu_jetia_cool + 30)) && (tmp < (bmp_dts_info->pmu_jetia_warm - 30))) {
			PMIC_DEBUG("restore bat chgcur:bat_temp:%d\n", tmp);
			qc_cur_val = bmp_dts_info->pmu_runtime_chgcur;
			jetia_status = false;
		}
	}

	atomic_set(&charger_power->pmu_jetia_status, jetia_status);
	qc_cur_val = eta6973_ichg_limit_ext(charger_power, qc_cur_val);

	PMIC_DEBUG("%s:%d bat_temp:%d, charge_status:%d, jetia_status:%d, lim_cur:%d\n", __func__, __LINE__,
			  tmp, charge_status, jetia_status, qc_cur_val);
	power_supply_changed(charger_power->charger_supply);

	if ((charge_status == 1) || jetia_status) {
		schedule_delayed_work(&charger_power->bat_qc_temp_process, msecs_to_jiffies(5 * 1000));
	}
}

static int eta6973_power_gpio_check(struct eta6973_power *charger_power, int gpio_no, const char *name)
{
	int ret = 0;
	if (!gpio_is_valid(gpio_no)) {
		PMIC_WARN("eta6973 power gpio[%d]:%s is no valid\n", gpio_no, name);
		return -EPROBE_DEFER;
	}
	ret = devm_gpio_request(charger_power->dev, gpio_no, name);
	if (ret != 0) {
		PMIC_WARN("eta6973_power_gpio gpio[%d]:%s request failed\n", gpio_no, name);
		return -EPROBE_DEFER;
	}
	return ret;
}

static int eta6973_power_gpio_init(struct eta6973_power *charger_power)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	unsigned long int config_set;
	int bat_det_gpio, bat_det_gpio_value, qc_nce_gpio;
	int ret = 0;

	/* set gpio pull down */
	config_set = SUNXI_PINCFG_PACK(
				PIN_CONFIG_BIAS_PULL_DOWN,
				1);

	/* set charge pin state low */
	qc_nce_gpio =
		of_get_named_gpio(charger_power->dev->of_node, "qc_nce_gpio", 0);
	if (qc_nce_gpio > 0) {
		ret = eta6973_power_gpio_check(charger_power, qc_nce_gpio, "eta6973_charge_en");
		if (ret < 0) {
			return ret;
		}
		gpio_direction_output(qc_nce_gpio, 0);
		pinctrl_gpio_set_config(qc_nce_gpio, config_set);
	}

	/* get battery status */
	if (!bmp_dts_info->battery_exist_sets) {
		bat_det_gpio =
			of_get_named_gpio(charger_power->dev->of_node, "bat_state_gpio", 0);
		if (bat_det_gpio > 0) {
			ret = eta6973_power_gpio_check(charger_power, bat_det_gpio, "eta6973_bat_status");
			if (ret < 0) {
				return ret;
			}
			pinctrl_gpio_set_config(bat_det_gpio, config_set);
			bat_det_gpio_value = gpio_get_value(bat_det_gpio);
			if (bat_det_gpio_value)
				charger_power->battery_exist = 0;
			else
				charger_power->battery_exist = 1;
			PMIC_DEBUG("eta6973_battery_check:%d", charger_power->battery_exist);
		} else {
			charger_power->battery_exist = 0;
		}
	} else {
		charger_power->battery_exist = bmp_dts_info->battery_exist_sets - 1;
	}

	/* set gpio pull up */
	config_set = SUNXI_PINCFG_PACK(
				PIN_CONFIG_BIAS_PULL_UP,
				1);

	/* set charge irq pin */
	charger_power->gpio_int_n =
		of_get_named_gpio(charger_power->dev->of_node, "qc_det_gpio", 0);

	if (charger_power->gpio_int_n < 0) {
		PMIC_INFO("no gpio interrupts:%d", charger_power->gpio_int_n);
	} else {
		ret = eta6973_power_gpio_check(charger_power, charger_power->gpio_int_n, "eta6973_gpio_int");
		if (ret < 0) {
			return ret;
		}
		gpio_direction_input(charger_power->gpio_int_n);
		pinctrl_gpio_set_config(charger_power->gpio_int_n, config_set);
		/* irq config setting */
		charger_power->gpio_int_n_irq = gpio_to_irq(charger_power->gpio_int_n);
	}

	PMIC_DEBUG("eta6973_gpio_int:%d irq:%d", charger_power->gpio_int_n, charger_power->gpio_int_n_irq);

	return 0;
}

static void eta6973_power_init_late(struct eta6973_power *charger_power)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	unsigned int data = 0;

	/* set charger voltage limit */
	data = bmp_dts_info->pmu_init_chgvol;
	data = ((data - 3848) / 32);
	regmap_update_bits(regmap, ETA6973_REG_04, ETA6973_REG04_VREG_MASK,
				 data << ETA6973_REG04_VREG_SHIFT);

	/* set chgcur lim */
	atomic_set(&charger_power->charge_set_limit, bmp_dts_info->pmu_runtime_chgcur);
	eta6973_ichg_limit_ext(charger_power, bmp_dts_info->pmu_runtime_chgcur);
}

static void eta6973_power_init(struct eta6973_power *charger_power, int flags)
{
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	struct regmap *regmap = charger_power->regmap;
	unsigned int data = 0;

	/* distinguish between ETA6973 and ETA6973*/
	regmap_read(regmap, ETA6973_REG_0B, &data);
	data &= ETA6973_REG0B_PN_MASK;
	if (data == 0x78) {
		PMIC_INFO("bmu-ext:use ETA6973\n");
		charger_power->charger_type = 0;
	} else {
		PMIC_INFO("bmu-ext:use ETA6974\n");
		charger_power->charger_type = 1;
	}

	/* init eta6973 private paras */
	regmap_read(regmap, ETA6973_REG_08, &data);
	charger_power->reg_val_08_old = data;

	regmap_read(regmap, ETA6973_REG_09, &data);
	charger_power->reg_val_09_old = data;

	regmap_read(regmap, ETA6973_REG_0A, &data);
	charger_power->reg_val_0a_old = data;

	atomic_set(&charger_power->charge_over_time, false);
	atomic_set(&charger_power->pmu_jetia_status, false);
	atomic_set(&charger_power->pmu_limit_status, false);
	atomic_set(&charger_power->charge_control_lim, 0);
	atomic_set(&charger_power->charge_set_limit, 2000);

	/* set default input voltage limit to 5V*/
	charger_power->vbus_set_vol_limit = 5000;
	eta6973_set_ovp_vol(charger_power->regmap, charger_power->vbus_set_vol_limit);

	/* check battery exist & set default input cur when non-battery*/
	if (!charger_power->battery_exist) {
		eta6973_set_iin_limit(charger_power, charger_power->dts_info.pmu_usbad_input_cur);
		PMIC_INFO("bmu-ext:chip init non-battery finish\n");
		return;
	}

	/* set default charge time */
	if (bmp_dts_info->bmu_ext_save_charge_time > 5)
		data = 0x01;
	else
		data = 0x00;

	regmap_update_bits(regmap, ETA6973_REG_05, ETA6973_REG05_CHG_TIMER_MASK,
				 data << ETA6973_REG05_CHG_TIMER_SHIFT);

	if (!flags) {
		charger_power->bat_supply_probe_finish = true;
		eta6973_power_init_late(charger_power);
	}

	PMIC_INFO("bmu-ext:chip init finish\n");
}

int eta6973_charger_dt_parse(struct device_node *node,
			 struct bmu_ext_config_info *bmp_ext_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	BMU_EXT_OF_PROP_READ(pmu_usbpc_input_vol,                    5000);
	BMU_EXT_OF_PROP_READ(pmu_usbpc_input_cur,                    500);
	BMU_EXT_OF_PROP_READ(pmu_usbad_input_vol,                    5000);
	BMU_EXT_OF_PROP_READ(pmu_usbad_input_cur,                    2400);
	BMU_EXT_OF_PROP_READ(bmu_ext_save_charge_time,               10);
	BMU_EXT_OF_PROP_READ(bmu_ext_max_power,                      14000000);
	BMU_EXT_OF_PROP_READ(battery_exist_sets,               		 0);

	BMU_EXT_OF_PROP_READ(pmu_runtime_chgcur,             		 2000);
	BMU_EXT_OF_PROP_READ(pmu_suspend_chgcur,            		 3000);
	BMU_EXT_OF_PROP_READ(pmu_shutdown_chgcur,           		 3000);

	return 0;
}

static void eta6973_charger_parse_device_tree(struct eta6973_power *charger_power)
{
	int ret;
	struct bmu_ext_config_info *cfg;

	/* set input current limit */
	if (!charger_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	cfg = &charger_power->dts_info;
	ret = eta6973_charger_dt_parse(charger_power->dev->of_node, cfg);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}

	/* get battery paras */
	if (!eta6973_battery_power_get(charger_power)) {
		PMIC_INFO("bmu-ext:Matching battery driver found, init paras finish\n");
		eta6973_power_init(charger_power, 0);
	} else if (charger_power->bat_supply_exist) {
		PMIC_INFO("bmu-ext:Matching battery driver found, init paras after battery driver loaded\n");
		eta6973_power_init(charger_power, 1);
	} else {
		PMIC_INFO("bmu-ext:No matching battery driver found\n");
		/*init eta6973 charger by device tree*/
		eta6973_power_init(charger_power, 0);
	}
}

static int eta6973_charger_probe(struct platform_device *pdev)
{
	struct eta6973_power *charger_power;
	struct bmu_ext_dev *ext = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	int ret = 0;

	charger_power = devm_kzalloc(&pdev->dev, sizeof(*charger_power), GFP_KERNEL);
	if (charger_power == NULL) {
		PMIC_ERR("eta6973_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	charger_power->name = "eta6973_charger";
	charger_power->dev = &pdev->dev;
	charger_power->regmap = ext->regmap;

	/* init gpio control: irq pin, nce pin, battery det pin */
	ret = eta6973_power_gpio_init(charger_power);
	if (ret != 0) {
		PMIC_INFO("gpio init failed\n");
		return ret;
	}

	/* parse device tree and set register */
	eta6973_charger_parse_device_tree(charger_power);

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = charger_power;

	charger_power->charger_supply = devm_power_supply_register(charger_power->dev,
			&eta6973_charger_desc, &psy_cfg);

	if (IS_ERR(charger_power->charger_supply)) {
		PMIC_ERR("eta6973 failed to register charger power\n");
		ret = PTR_ERR(charger_power->charger_supply);
		return ret;
	}

	INIT_DELAYED_WORK(&charger_power->charger_supply_mon, eta6973_power_monitor);

	if (charger_power->gpio_int_n > 0) {
		PMIC_DEBUG("gpio interrupts:%d", charger_power->gpio_int_n);
		ret = devm_request_threaded_irq(&pdev->dev, charger_power->gpio_int_n_irq, NULL,
										eta6973_irq_handler, IRQF_ONESHOT | IRQF_NO_SUSPEND | IRQF_TRIGGER_LOW,
										"eta6973-charger", charger_power);
		if (ret < 0) {
				PMIC_DEV_ERR(&pdev->dev, "failed to request eta6973-charger IRQ %d: %d\n", charger_power->gpio_int_n_irq, ret);
				goto cancel_work;
		}
		if (!of_property_read_bool(pdev->dev.of_node, "wakeup-source")) {
			PMIC_INFO("wakeup source is disabled!\n");
		} else {
			ret = device_init_wakeup(&pdev->dev, true);
			if (ret < 0) {
					PMIC_DEV_ERR(&pdev->dev, "failed to init wake IRQ %d: %d\n", charger_power->gpio_int_n_irq, ret);
					goto cancel_work;
			}
			ret = dev_pm_set_wake_irq(&pdev->dev, charger_power->gpio_int_n_irq);
			if (ret < 0) {
					PMIC_DEV_ERR(&pdev->dev, "failed to set wake IRQ %d: %d\n", charger_power->gpio_int_n_irq, ret);
					goto cancel_work;
			}
		}
	}

	schedule_delayed_work(&charger_power->charger_supply_mon, msecs_to_jiffies(500));

	platform_set_drvdata(pdev, charger_power);

	return ret;

cancel_work:
	cancel_delayed_work_sync(&charger_power->charger_supply_mon);

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int eta6973_charger_remove(struct platform_device *pdev)
{
	struct eta6973_power *charger_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&charger_power->charger_supply_mon);

	PMIC_DEV_DEBUG(&pdev->dev, "==============eta6973 charger unegister==============\n");
	if (charger_power->charger_supply)
		power_supply_unregister(charger_power->charger_supply);
	PMIC_DEV_DEBUG(&pdev->dev, "eta6973 teardown charger dev\n");

	return 0;
}

static void eta6973_charger_shutdown(struct platform_device *pdev)
{
	struct eta6973_power *charger_power = platform_get_drvdata(pdev);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	int charge_limit = atomic_read(&charger_power->charge_set_limit);

	if (charge_limit >= bmp_dts_info->pmu_runtime_chgcur)
		eta6973_set_ichg_limit(charger_power->regmap, bmp_dts_info->pmu_shutdown_chgcur);

	cancel_delayed_work_sync(&charger_power->charger_supply_mon);

	if (charger_power->dts_info.pmu_bat_temp_enable) {
		cancel_delayed_work_sync(&charger_power->bat_qc_temp_process);
	}
}

static int eta6973_charger_suspend(struct platform_device *p, pm_message_t state)
{
	struct eta6973_power *charger_power = platform_get_drvdata(p);
	struct bmu_ext_config_info *bmp_dts_info = &charger_power->dts_info;
	int charge_limit = atomic_read(&charger_power->charge_set_limit);

	if (charge_limit >= bmp_dts_info->pmu_runtime_chgcur)
		eta6973_set_ichg_limit(charger_power->regmap, bmp_dts_info->pmu_suspend_chgcur);

	cancel_delayed_work_sync(&charger_power->charger_supply_mon);

	if (charger_power->dts_info.pmu_bat_temp_enable) {
		cancel_delayed_work_sync(&charger_power->bat_qc_temp_process);
	}

	return 0;
}

static int eta6973_charger_resume(struct platform_device *p)
{
	struct eta6973_power *charger_power = platform_get_drvdata(p);
	int charge_limit = atomic_read(&charger_power->charge_set_limit);

	eta6973_set_ichg_limit(charger_power->regmap, charge_limit);
	schedule_delayed_work(&charger_power->charger_supply_mon, 0);

	if (charger_power->dts_info.pmu_bat_temp_enable) {
		schedule_delayed_work(&charger_power->bat_qc_temp_process, 0);
	}

	return 0;
}

static const struct of_device_id eta6973_charger_power_match[] = {
	{
		.compatible = "eta,eta6973-charger-power-supply",
		.data = (void *)ETA6973_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, eta6973_charger_power_match);

static struct platform_driver eta6973_charger_power_driver = {
	.driver = {
		.name = "eta6973-charger-power-supply",
		.of_match_table = eta6973_charger_power_match,
	},
	.probe = eta6973_charger_probe,
	.remove = eta6973_charger_remove,
	.shutdown = eta6973_charger_shutdown,
	.suspend = eta6973_charger_suspend,
	.resume = eta6973_charger_resume,
};

module_platform_driver(eta6973_charger_power_driver);

MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("eta6973 charger driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
