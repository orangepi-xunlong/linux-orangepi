/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp2202_charger.h"

#define DIVIDE_BY_64(n) (((n) >> 6) << 6)

struct axp2202_bat_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *bat_supply;
	struct delayed_work        bat_supply_mon;
	struct delayed_work        bat_power_curve;
	struct axp_config_info     dts_info;
	struct wakeup_source       *ws;

	bool			qc_supply_exist;
	bool			qc_supply_probe_finish;
	bool			qc_bat_exist;
	struct power_supply	*qc_psy;
	struct device_node	*qc_supply_np;

	int			charger_cooler_type;

	/* charge_limit_process */
	atomic_t		pmu_limit_status;

	/* charge_cycle_count */
	atomic_t	 	charge_cycle_count;
};

static enum power_supply_property axp2202_bat_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_MANUFACTURE_YEAR,
	POWER_SUPPLY_PROP_MANUFACTURE_MONTH,
	POWER_SUPPLY_PROP_MANUFACTURE_DAY,
};

static int axp2202_qc_power_get(struct axp2202_bat_power *bat_power)
{
	struct device_node *np = NULL;
	static int check_count = 1;
	int ret = 0;

	if (bat_power->qc_supply_exist)
		return ret;

	if (check_count == 2)
		return -1;

	bat_power->qc_supply_exist = false;
	bat_power->qc_supply_probe_finish = false;

	if (of_find_property(bat_power->dev->of_node, "det_qc_supply", NULL)) {
		np = of_parse_phandle(bat_power->dev->of_node, "det_qc_supply", 0);
		if (!of_device_is_available(np)) {
			bat_power->qc_psy =  NULL;
			bat_power->qc_supply_np =  NULL;
			ret = -1;
		} else {
			bat_power->qc_psy = devm_power_supply_get_by_phandle(bat_power->dev,
							"det_qc_supply");
			bat_power->qc_supply_np = np;
			if (!(bat_power->qc_psy) || (IS_ERR(bat_power->qc_psy))) {
				return -EPROBE_DEFER;
			}
		}
	} else {
			bat_power->qc_psy =  NULL;
			bat_power->qc_supply_np =  NULL;
			ret = -1;
	}

	if (!ret)
		bat_power->qc_supply_exist = true;

	check_count = 2;
	return ret;
}

static int _axp2202_qc_para_get(struct axp2202_bat_power *bat_power, enum power_supply_property psp, const char *name, int paras)
{
	union power_supply_propval val;
	int _paras = -1;

	if (name != NULL) {
		of_property_read_u32(bat_power->qc_supply_np, name, &_paras);
	}

	if (_paras < 0) {
		val.intval = paras;
	} else {
		val.intval = _paras;
	}

	_paras = psp;

	if (_paras > -1) {
		power_supply_get_property(bat_power->qc_psy, psp, &val);
	}

	return val.intval;
}

static int axp2202_qc_para_get(struct axp2202_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	static int check_count = 1;
	int ret = 0;

	ret = axp2202_qc_power_get(bat_power);
	if (ret) {
		return ret;
	}

	if (check_count == 2)
		return 0;

	/* get ichg limit paras */
	axp_config->pmu_runtime_chgcur = _axp2202_qc_para_get(bat_power, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, NULL, axp_config->pmu_runtime_chgcur);
	axp_config->pmu_bat_charge_control_lim = _axp2202_qc_para_get(bat_power, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, NULL, axp_config->pmu_bat_charge_control_lim);

	/* get battery exist */
	bat_power->qc_bat_exist = _axp2202_qc_para_get(bat_power, POWER_SUPPLY_PROP_CALIBRATE, NULL, 0);

	PMIC_INFO("axp2202:pmu_runtime_chgcur:%d\n", axp_config->pmu_runtime_chgcur);
	PMIC_INFO("axp2202:pmu_bat_charge_control_lim:%d\n", axp_config->pmu_bat_charge_control_lim);
	PMIC_INFO("axp2202:qc_bat_exist:%d\n", bat_power->qc_bat_exist);
	check_count = 2;
	return 0;
}

static int axp2202_get_vbat_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	uint8_t data[2];
	uint16_t vtemp[3], tempv;
	int ret = 0;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2202_VBAT_H, data, 2);
		if (ret < 0)
			return ret;

		vtemp[i] = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	}
	if (vtemp[0] > vtemp[1]) {
		tempv = vtemp[0];
		vtemp[0] = vtemp[1];
		vtemp[1] = tempv;
	}
	if (vtemp[1] > vtemp[2]) {
		tempv = vtemp[1];
		vtemp[1] = vtemp[2];
		vtemp[2] = tempv;
	}
	if (vtemp[0] > vtemp[1]) {
		tempv = vtemp[0];
		vtemp[0] = vtemp[1];
		vtemp[1] = tempv;
	} /* Why three times? */
	/*incase vtemp[1] exceed AXP2202_VBAT_MAX */
	if ((vtemp[1] > AXP2202_VBAT_MAX) || (vtemp[1] < AXP2202_VBAT_MIN)) {
		val->intval = 0;
		return 0;
	}

	val->intval = vtemp[1] * 1000;

	return 0;
}

static int axp2202_get_bat_health(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;

	unsigned int reg_value, reg_value2;
	int ret = 0;

	ret = regmap_read(regmap, AXP2202_COMM_STAT0, &reg_value);
	ret = regmap_read(regmap, AXP2202_COMM_FAULT, &reg_value2);

	if (bat_power->qc_supply_exist) {
		if (bat_power->qc_bat_exist)
			reg_value |= AXP2202_MASK_BAT_STAT;
		else
			reg_value &= ~(AXP2202_MASK_BAT_STAT);

		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_PRESENT, val);
		if (val->intval == POWER_SUPPLY_HEALTH_DEAD)
			reg_value |= AXP2202_MASK_BAT_ACT_STAT;
		else
			reg_value &= ~(AXP2202_MASK_BAT_ACT_STAT);
	}

	if (reg_value & AXP2202_MASK_BAT_STAT) {
		if (reg_value & AXP2202_MASK_BAT_ACT_STAT)
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		else if (reg_value2 & AXP2202_MASK_BAT_WOT)
			val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
		else if (reg_value2 & AXP2202_MASK_BAT_WUT)
			val->intval = POWER_SUPPLY_HEALTH_COLD;
		else
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
	} else {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
	}
	return val->intval;
}

static inline int axp_vts_to_temp(int data,
		const struct axp_config_info *axp_config)
{
	int temp;

	if (!axp_config->pmu_bat_temp_enable)
		return 300;
	else if (data < axp_config->pmu_bat_temp_para16)
		return 800;
	else if (data <= axp_config->pmu_bat_temp_para15) {
		temp = 700 + (axp_config->pmu_bat_temp_para15-data)*100/
		(axp_config->pmu_bat_temp_para15-axp_config->pmu_bat_temp_para16);
	} else if (data <= axp_config->pmu_bat_temp_para14) {
		temp = 600 + (axp_config->pmu_bat_temp_para14-data)*100/
		(axp_config->pmu_bat_temp_para14-axp_config->pmu_bat_temp_para15);
	} else if (data <= axp_config->pmu_bat_temp_para13) {
		temp = 550 + (axp_config->pmu_bat_temp_para13-data)*50/
		(axp_config->pmu_bat_temp_para13-axp_config->pmu_bat_temp_para14);
	} else if (data <= axp_config->pmu_bat_temp_para12) {
		temp = 500 + (axp_config->pmu_bat_temp_para12-data)*50/
		(axp_config->pmu_bat_temp_para12-axp_config->pmu_bat_temp_para13);
	} else if (data <= axp_config->pmu_bat_temp_para11) {
		temp = 450 + (axp_config->pmu_bat_temp_para11-data)*50/
		(axp_config->pmu_bat_temp_para11-axp_config->pmu_bat_temp_para12);
	} else if (data <= axp_config->pmu_bat_temp_para10) {
		temp = 400 + (axp_config->pmu_bat_temp_para10-data)*50/
		(axp_config->pmu_bat_temp_para10-axp_config->pmu_bat_temp_para11);
	} else if (data <= axp_config->pmu_bat_temp_para9) {
		temp = 300 + (axp_config->pmu_bat_temp_para9-data)*100/
		(axp_config->pmu_bat_temp_para9-axp_config->pmu_bat_temp_para10);
	} else if (data <= axp_config->pmu_bat_temp_para8) {
		temp = 200 + (axp_config->pmu_bat_temp_para8-data)*100/
		(axp_config->pmu_bat_temp_para8-axp_config->pmu_bat_temp_para9);
	} else if (data <= axp_config->pmu_bat_temp_para7) {
		temp = 100 + (axp_config->pmu_bat_temp_para7-data)*100/
		(axp_config->pmu_bat_temp_para7-axp_config->pmu_bat_temp_para8);
	} else if (data <= axp_config->pmu_bat_temp_para6) {
		temp = 50 + (axp_config->pmu_bat_temp_para6-data)*50/
		(axp_config->pmu_bat_temp_para6-axp_config->pmu_bat_temp_para7);
	} else if (data <= axp_config->pmu_bat_temp_para5) {
		temp = 0 + (axp_config->pmu_bat_temp_para5-data)*50/
		(axp_config->pmu_bat_temp_para5-axp_config->pmu_bat_temp_para6);
	} else if (data <= axp_config->pmu_bat_temp_para4) {
		temp = -50 + (axp_config->pmu_bat_temp_para4-data)*50/
		(axp_config->pmu_bat_temp_para4-axp_config->pmu_bat_temp_para5);
	} else if (data <= axp_config->pmu_bat_temp_para3) {
		temp = -100 + (axp_config->pmu_bat_temp_para3-data)*50/
		(axp_config->pmu_bat_temp_para3-axp_config->pmu_bat_temp_para4);
	} else if (data <= axp_config->pmu_bat_temp_para2) {
		temp = -150 + (axp_config->pmu_bat_temp_para2-data)*50/
		(axp_config->pmu_bat_temp_para2-axp_config->pmu_bat_temp_para3);
	} else if (data <= axp_config->pmu_bat_temp_para1) {
		temp = -250 + (axp_config->pmu_bat_temp_para1-data)*100/
		(axp_config->pmu_bat_temp_para1-axp_config->pmu_bat_temp_para2);
	} else
		temp = -250;
	return temp;
}

/* read temperature */
static int axp2202_get_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	struct axp_config_info *axp_config = &bat_power->dts_info;

	uint8_t data[2];
	uint16_t temp;
	int ret = 0, i = 0, tmp, old_temp;

	ret = regmap_update_bits(regmap, AXP2202_ADC_DATA_SEL, 0x03, AXP2202_ADC_TS_SEL); /* ADC channel select */
	if (ret < 0)
		return ret;
	mdelay(1);

	ret = regmap_bulk_read(regmap, AXP2202_ADC_DATA_H, data, 2);
	if (ret < 0)
		return ret;
	temp = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	tmp = temp * 500 / 1000;

	old_temp = axp_vts_to_temp(tmp, axp_config);

	/* read until abs(old_temp - temp) < 10*/
	ret = regmap_bulk_read(regmap, AXP2202_ADC_DATA_H, data, 2);
	if (ret < 0)
		return ret;
	temp = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
	tmp = temp * 500 / 1000;
	tmp = axp_vts_to_temp(tmp, axp_config);

	PMIC_DEBUG("old_temp:%d, temp:%d\n", old_temp, temp);
	while ((abs(old_temp - temp) > 100) && (i < 10)) {
		old_temp = temp;
		ret = regmap_bulk_read(regmap, AXP2202_ADC_DATA_H, data, 2);
		if (ret < 0)
			return ret;
		temp = (((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]));
		tmp = temp * 500 / 1000;
		tmp = axp_vts_to_temp(tmp, axp_config);
		i++;
		PMIC_DEBUG("turn[%d]:old_temp:%d, temp:%d\n", i, old_temp, temp);
	}

	val->intval = tmp;

	return 0;
}

static int axp2202_bat_get_max_voltage(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;

	int ret, reg_value;

	ret = regmap_read(regmap, AXP2202_VTERM_CFG, &reg_value);
	if (ret)
		return ret;

	switch (reg_value & AXP2202_CHRG_TGT_VOLT) {
	case AXP2202_CHRG_CTRL1_TGT_4_0V:
		val->intval = 4000000;
		break;
	case AXP2202_CHRG_CTRL1_TGT_4_1V:
		val->intval = 4100000;
		break;
	case AXP2202_CHRG_CTRL1_TGT_4_2V:
		val->intval = 4200000;
		break;
	case AXP2202_CHRG_CTRL1_TGT_4_35V:
		val->intval = 4350000;
		break;
	case AXP2202_CHRG_CTRL1_TGT_4_4V:
		val->intval = 4400000;
		break;
	case AXP2202_CHRG_CTRL1_TGT_5_0V:
		val->intval = 5000000;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int axp2202_get_ichg(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;

	uint8_t data[2];
	uint16_t ichg;
	int ret = 0;

	ret = regmap_bulk_read(regmap, AXP2202_ICHG_H, data, 2);
	if (ret < 0)
		return ret;
	ichg = (((data[0] & GENMASK(5, 0)) << 8) | (data[1])); /* mA */
	ichg = ichg / 2;
	val->intval = ichg;

	return 0;
}

static int axp2202_get_ichg_lim(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2202_ICC_CFG, &data);
	if (ret < 0)
		return ret;

	data &= 0x3F;
	data = data * 64;

	val->intval = data;

	return 0;
}

static int _axp2202_get_bat_status(struct axp2202_bat_power *bat_power,
				  union power_supply_propval *val)
{
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret;

	ret = regmap_read(regmap, AXP2202_COMM_STAT1, &data);
	if (ret < 0) {
		PMIC_DEBUG("error read AXP2202_COM_STAT1\n");
		return ret;
	}

	/* chg_stat = bit[2:0] */
	switch (data & 0x07) {
	case AXP2202_CHARGING_TRI:
	case AXP2202_CHARGING_NCHG:
		val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case AXP2202_CHARGING_PRE:
	case AXP2202_CHARGING_CC:
	case AXP2202_CHARGING_CV:
		val->intval = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case AXP2202_CHARGING_DONE:
		val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	default:
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		break;
	}

	if (bat_power->qc_supply_exist) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, val);
	}

	ret = regmap_read(bat_power->regmap, AXP2202_COMM_STAT0, &data);
	if (ret < 0)
		return ret;

	if (data & 0x20) {
		ret = regmap_read(regmap, AXP2202_GAUGE_SOC, &data);
		if (ret < 0)
			return ret;
		if (data == 100)
			val->intval = POWER_SUPPLY_STATUS_FULL;
	}

	return 0;
}

static int axp2202_get_bat_status(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	int ret;

	ret = _axp2202_get_bat_status(bat_power, val);

	return ret;
}

static int axp2202_get_soc(struct power_supply *ps,
			    union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(bat_power->regmap, AXP2202_CURVE_CHECK, &data);
	if (ret < 0)
		return ret;

	if (data & 0x80) {
		ret = regmap_read(regmap, AXP2202_CURVE_CHECK, &data);
		data &= 0x7F;
	} else {
		ret = regmap_read(regmap, AXP2202_GAUGE_SOC, &data);
	}

	if (ret < 0)
		return ret;

	axp2202_get_bat_status(ps, val);
	if ((val->intval == POWER_SUPPLY_STATUS_FULL) && data == 99)
		data = 100;

	if (data > AXP2202_SOC_MAX)
		data = AXP2202_SOC_MAX;
	else if (data < AXP2202_SOC_MIN)
		data = AXP2202_SOC_MIN;

	val->intval = data;

	return 0;
}

static int axp2202_get_charger_count(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	unsigned int data;
	int charge_cycle_count = 0, reduce_cap;

	charge_cycle_count = atomic_read(&bat_power->charge_cycle_count);

	data = axp_config->pmu_battery_cap * 1000;
	reduce_cap = charge_cycle_count * axp_config->pmu_bat_cycle_cap_reduce;
	data -= reduce_cap;

	val->intval = data;
	return 0;
}

static int axp2202_get_charger_count_current(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	unsigned int data[2];
	int ret = 0;

	data[0] = axp_config->pmu_battery_cap * 1000;

	ret = axp2202_get_soc(ps, val);
	if (ret < 0)
		return ret;
	data[1] = val->intval;
	data[1] = data[1] * data[0] / 100;

	val->intval = data[1];
	return 0;
}

static int axp2202_get_time2empty(struct power_supply *ps,
				   union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	uint8_t data[2];
	uint16_t ttemp[3], tempt;
	int ret = 0;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2202_GAUGE_TIME2EMPTY_H, data,
				       2);
		if (ret < 0)
			return ret;

		ttemp[i] = ((data[0] << 0x08) | (data[1]));
	}

	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	if (ttemp[1] > ttemp[2]) {
		tempt = ttemp[1];
		ttemp[1] = ttemp[2];
		ttemp[2] = tempt;
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}

	val->intval = ttemp[1] * 60;

	return 0;
}


static int axp2202_get_time2full(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	uint16_t ttemp[3], tempt;
	uint8_t data[2];
	int ret = 0;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2202_GAUGE_TIME2FULL_H, data, 2);
		if (ret < 0)
			return ret;

		ttemp[i] = ((data[0] << 0x08) | (data[1]));
	}

	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}
	if (ttemp[1] > ttemp[2]) {
		tempt = ttemp[1];
		ttemp[1] = ttemp[2];
		ttemp[2] = tempt;
	}
	if (ttemp[0] > ttemp[1]) {
		tempt = ttemp[0];
		ttemp[0] = ttemp[1];
		ttemp[1] = tempt;
	}

	val->intval = ttemp[1] * 60;

	return 0;
}

static int axp2202_get_bat_present(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret;

	if (bat_power->qc_supply_exist) {
		val->intval = bat_power->qc_bat_exist;
		return 0;
	}

	ret = regmap_read(regmap, AXP2202_COMM_STAT0, &data);
	if (ret < 0) {
		PMIC_DEV_DEBUG(&ps->dev, "error read AXP2202_COM_STAT0\n");
		return ret;
	}

	if (data & AXP2202_MASK_BAT_STAT)
		val->intval = 1;
	else
		val->intval = 0;
	return 0;
}

static int axp2202_get_lowsocth(struct power_supply *ps,
				 union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP2202_GAUGE_THLD, &data);
	if (ret < 0)
		return ret;

	val->intval = (data >> 4) + 5;

	return 0;
}


static int axp2202_set_lowsocth(struct regmap *regmap, int v)
{
	unsigned int data;
	int ret = 0;

	data = v;

	if (data > 20 || data < 5)
		return -EINVAL;

	data = (data - 5);
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_THLD, GENMASK(7, 4),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2202_set_ichg(struct regmap *regmap, int mA)
{
	unsigned int data;
	int ret = 0;

	data = mA;

	if (data > 3072)
		data = 3072;
	if	(data < 0)
		data = 0;
	else {
	data = (data / 64);
	ret = regmap_update_bits(regmap, AXP2202_ICC_CFG, GENMASK(5, 0),
				 data);
	if (ret < 0)
		return ret;
	}

	return 0;
}

static int axp2202_set_bat_max_voltage(struct regmap *regmap, int mV)
{
	unsigned int data;
	int ret = 0;

	data = mV;

	if (data > 5000)	{
		data = 5000;
		ret = regmap_update_bits(regmap, AXP2202_VTERM_CFG, GENMASK(2, 0),
					 AXP2202_CHRG_CTRL1_TGT_5_0V);
	} else if	(data < 4100) {
		data = 4000;
		ret = regmap_update_bits(regmap, AXP2202_VTERM_CFG, GENMASK(2, 0),
					 AXP2202_CHRG_CTRL1_TGT_4_0V);
	} else if	(data > 4100 && data < 4200) {
		data = 4100;
		ret = regmap_update_bits(regmap, AXP2202_VTERM_CFG, GENMASK(2, 0),
					 AXP2202_CHRG_CTRL1_TGT_4_1V);
	} else if	(data > 4200 && data < 4350) {
		data = 4200;
		ret = regmap_update_bits(regmap, AXP2202_VTERM_CFG, GENMASK(2, 0),
					 AXP2202_CHRG_CTRL1_TGT_4_2V);
	} else if	(data > 4350 && data < 4400) {
		data = 4350;
		ret = regmap_update_bits(regmap, AXP2202_VTERM_CFG, GENMASK(2, 0),
					 AXP2202_CHRG_CTRL1_TGT_4_35V);
	} else if	(data > 4400 && data < 5000) {
		data = 4400;
		ret = regmap_update_bits(regmap, AXP2202_VTERM_CFG, GENMASK(2, 0),
					 AXP2202_CHRG_CTRL1_TGT_4_4V);
	} else
		ret = 0;
	return 0;
}

static int axp2202_set_status(struct axp2202_bat_power *bat_power, int status)
{
	union power_supply_propval qc_val;
	struct regmap *regmap = bat_power->regmap;

	if (!status) {
		PMIC_INFO("disable charge\n");
		if (bat_power->qc_supply_exist) {
			qc_val.intval = 1;
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
		}
		regmap_update_bits(regmap, AXP2202_MODULE_EN, BIT(1), 0);
	} else {
		PMIC_INFO("enable charge\n");
		if (bat_power->qc_supply_exist) {
			qc_val.intval = 2;
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
		}
		regmap_update_bits(regmap, AXP2202_MODULE_EN, BIT(1), BIT(1));
	}
	return 0;
}

static int _axp2202_reset_mcu(struct regmap *regmap)
{
	int ret = 0;

	ret = regmap_update_bits(regmap, AXP2202_RESET_CFG, AXP2202_MODE_RSTMCU,
				 AXP2202_MODE_RSTMCU);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, AXP2202_RESET_CFG, AXP2202_MODE_RSTMCU,
				 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2202_reset_mcu(struct axp2202_bat_power *bat_power)
{
	int ret = 0;
	union power_supply_propval qc_val;
	struct regmap *regmap = bat_power->regmap;

	if (bat_power->qc_supply_exist) {
		qc_val.intval = 1;
		power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
	}
	regmap_update_bits(regmap, AXP2202_MODULE_EN, BIT(1), 0);
	msleep(500);

	ret = _axp2202_reset_mcu(regmap);

	msleep(500);
	if (bat_power->qc_supply_exist) {
		qc_val.intval = 2;
		power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
	}
	regmap_update_bits(regmap, AXP2202_MODULE_EN, BIT(1), BIT(1));

	return ret;
}

/**
 * axp2202_get_param - get battery config from dts
 *
 * is not get battery config parameter from dts,
 * then it use the default config.
 */
static int axp2202_get_param(struct axp2202_bat_power *bat_power, uint8_t *para,
			     unsigned int *len)
{
	struct device_node *n_para, *r_para;
	const char *pparam;
	int cnt;

	n_para = of_parse_phandle(bat_power->dev->of_node, "param", 0);
	if (!n_para)
		goto e_n_para;

	if (of_property_read_string(n_para, "select", &pparam))
		goto e_para;

	r_para = of_get_child_by_name(n_para, pparam);
	if (!r_para)
		goto e_para;

	cnt = of_property_read_variable_u8_array(r_para, "parameter", para, 1,
						 *len);
	if (cnt <= 0)
		goto e_n_parameter;
	*len = cnt;

	of_node_put(r_para);
	of_node_put(n_para);

	return 0;

e_n_parameter:
	of_node_put(r_para);
e_para:
	of_node_put(n_para);
e_n_para:
	return -ENODATA;
}

static int axp2202_model_update(struct axp2202_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	unsigned int data;
	unsigned int len;
	uint8_t i;
	uint8_t *param;

	/* reset power curve regs */
	ret = regmap_write(regmap, AXP2202_CURVE_CHECK, 0);
	if (ret < 0)
		PMIC_ERR("can not clear curve_check \n");;

	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, AXP2202_BROMUP_EN);
	if (ret < 0)
		goto UPDATE_ERR;

	/* down load battery parameters */
	len = AXP2202_MAX_PARAM;
	param = devm_kzalloc(bat_power->dev, AXP2202_MAX_PARAM, GFP_KERNEL);
	if (!param) {
		PMIC_ERR("can not find memory for param\n");
		goto UPDATE_ERR;
	}
	ret = axp2202_get_param(bat_power, param, &len);
	if (ret < 0)
		goto err_param;

	for (i = 0; i < len; i++) {
		ret = regmap_write(regmap, AXP2202_GAUGE_BROM, param[i]);
		if (ret < 0)
			goto err_param;
	}
	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, 0);
	if (ret < 0)
		goto err_param;

	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_BROMUP_EN, AXP2202_BROMUP_EN);
	if (ret < 0)
		goto err_param;

	/* check battery parameters is ok ? */
	for (i = 0; i < len; i++) {
		ret = regmap_read(regmap, AXP2202_GAUGE_BROM, &data);
		if (ret < 0)
			goto err_param;

		if (data != param[i]) {
			PMIC_ERR("model param check %02x error!\n", i);
			goto err_param;
		}
	}

	devm_kfree(bat_power->dev, param);

	/* close brom and set battery update flag */
	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG, AXP2202_BROMUP_EN,
				 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG,
				 AXP2202_CFG_UPDATE_MARK,
				 AXP2202_CFG_UPDATE_MARK);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_read(regmap, AXP2202_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset_mcu */
	ret = axp2202_reset_mcu(bat_power);
	if (ret < 0)
		goto UPDATE_ERR;
	axp2202_set_ichg(regmap, bat_power->dts_info.pmu_runtime_chgcur);

	/* update ok */
	return 0;

err_param:
	devm_kfree(bat_power->dev, param);

UPDATE_ERR:
	regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG, AXP2202_BROMUP_EN, 0);
	axp2202_reset_mcu(bat_power);

	return ret;
}

static int axp2202_blance_vol(struct regmap *regmap)
{
	int vol_in_reg, reg_value;
	int ret;

	ret = regmap_read(regmap, AXP2202_CURVE_CHECK, &reg_value);
	if (ret < 0)
		return ret;

	reg_value &= 0x7F;

	ret = regmap_read(regmap, AXP2202_GAUGE_SOC, &vol_in_reg);
	if (ret < 0)
		return ret;

	if (vol_in_reg > AXP2202_SOC_MAX)
		vol_in_reg = AXP2202_SOC_MAX;
	else if (vol_in_reg < AXP2202_SOC_MIN)
		vol_in_reg = AXP2202_SOC_MIN;

	return vol_in_reg - reg_value;

}

static bool axp2202_model_update_check(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP2202_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto CHECK_ERR;

	if ((data & AXP2202_CFG_UPDATE_MARK) == 0)
		goto CHECK_ERR;


	return true;

CHECK_ERR:
	regmap_update_bits(regmap, AXP2202_GAUGE_CONFIG, AXP2202_BROMUP_EN, 0);
	return false;
}

static int axp2202_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;

	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(psy);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = axp2202_bat_get_max_voltage(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL: // customer modify
		ret = axp2202_get_bat_present(psy, val);
		if (ret < 0)
			return ret;

		if (val->intval) {
			ret = axp2202_get_soc(psy, val);
			if (ret < 0)
				return ret;

			if (val->intval == 100)
				val->intval = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
			else if (val->intval > 80)
				val->intval = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
			else if (val->intval > bat_power->dts_info.pmu_battery_warning_level1)
				val->intval = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;
			else if (val->intval > bat_power->dts_info.pmu_battery_warning_level2)
				val->intval = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
			else if (val->intval >= 0)
				val->intval = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
			else
				val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
			}
		else
			val->intval = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2202_get_bat_status(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp2202_get_bat_present(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp2202_get_vbat_vol(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = axp2202_get_soc(psy, val); // unit %;
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp2202_get_lowsocth(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		if (bat_power->qc_supply_exist) {
			if ((val->intval > 0) && !bat_power->qc_supply_probe_finish) {
				val->intval = axp_vts_to_temp(val->intval, axp_config);
				break;
			}
		}
		ret = axp2202_get_temp(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp2202_get_ichg(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		if (bat_power->charger_cooler_type) {
			val->intval = atomic_read(&bat_power->pmu_limit_status);
			break;
		}
		if (bat_power->qc_supply_exist) {
			power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, val);
		} else {
			ret = axp2202_get_ichg_lim(psy, val);
			if (ret < 0)
				return ret;
		}
		if (val->intval < bat_power->dts_info.pmu_bat_charge_control_lim)
			val->intval = 0;
		else if (val->intval < bat_power->dts_info.pmu_runtime_chgcur)
			val->intval = 1;
		else
			val->intval = 2;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = 2;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = axp2202_get_bat_health(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		ret = bat_power->dts_info.pmu_bat_shutdown_ltf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MAX:
		ret = bat_power->dts_info.pmu_bat_shutdown_htf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN:
		ret = bat_power->dts_info.pmu_bat_charge_ltf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX:
		ret = bat_power->dts_info.pmu_bat_charge_htf;
		val->intval = axp_vts_to_temp(ret, &bat_power->dts_info);
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		ret = axp2202_get_time2empty(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		ret = axp2202_get_time2full(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP2202_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		ret = axp2202_get_charger_count_current(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = axp2202_get_charger_count(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		val->intval = -1;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURE_YEAR:
		val->intval = bat_power->dts_info.pmu_bat_manufacture_year;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURE_MONTH:
		val->intval = bat_power->dts_info.pmu_bat_manufacture_month;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURE_DAY:
		val->intval = bat_power->dts_info.pmu_bat_manufacture_day;
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int axp2202_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp2202_bat_power *bat_power = power_supply_get_drvdata(psy);
	union power_supply_propval qc_val;
	struct regmap *regmap = bat_power->regmap;
	int ret = 0, lim_cur = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp2202_set_lowsocth(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		if (bat_power->qc_supply_exist) {
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, val);
			break;
		}
		ret = axp2202_set_ichg(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		if (!bat_power->charger_cooler_type) {
			if (val->intval == 1) {
				lim_cur = bat_power->dts_info.pmu_bat_charge_control_lim;
			} else if (val->intval == 2) {
				lim_cur = bat_power->dts_info.pmu_runtime_chgcur;
			} else {
				lim_cur = 0;
			}
		} else {
			atomic_set(&bat_power->pmu_limit_status, val->intval);
			if (val->intval == PAUSE_CHARGING_STATE) {
				lim_cur = 0;
			} else if (val->intval == LIMIT_CUR_STATE) {
				lim_cur = bat_power->dts_info.pmu_bat_charge_control_lim;
			} else if (val->intval == NORMAL_STATE) {
				lim_cur = bat_power->dts_info.pmu_runtime_chgcur;
			} else {
				break;
			}
		}
		PMIC_INFO("ichg set:%d\n", lim_cur);
		if (bat_power->qc_supply_exist) {
			qc_val.intval = lim_cur;
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, &qc_val);
			break;
		}
		ret = axp2202_set_ichg(regmap, lim_cur);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = axp2202_set_bat_max_voltage(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		if (bat_power->qc_supply_exist) {
			if ((val->intval == 1) && !bat_power->qc_supply_probe_finish) {
				bat_power->qc_supply_probe_finish = true;
			}
		}
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		atomic_set(&bat_power->charge_cycle_count, val->intval);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2202_set_status(bat_power, val->intval);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int axp2202_usb_power_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CALIBRATE:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		ret = 1;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = 1;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static const struct power_supply_desc axp2202_bat_desc = {
	.name = "axp2202-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp2202_bat_get_property,
	.set_property = axp2202_bat_set_property,
	.properties = axp2202_bat_props,
	.num_properties = ARRAY_SIZE(axp2202_bat_props),
	.property_is_writeable = axp2202_usb_power_property_is_writeable,
};

static int axp2202_init_chip(struct axp2202_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	union power_supply_propval bat_val;
	int ret = 0;
	int val;
	uint8_t data[2];
	unsigned int reg_value, battery_exist = 1;

	if (bat_power == NULL)
		return -ENODEV;

	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp2202 reg update, i2c communication err!\n");
		return ret;
	}

	/* init bat cycle*/
	atomic_set(&bat_power->charge_cycle_count, 0);
	axp_config->pmu_bat_cycle_cap_reduce *= axp_config->pmu_battery_cap * 10 / axp_config->pmu_bat_cycle_life;

	/* default sets */
	atomic_set(&bat_power->pmu_limit_status, 0);

	/* get battery exist*/
	if (!bat_power->qc_supply_exist) {
		regmap_read(bat_power->regmap, AXP2202_COMM_STAT0, &val);
		if (!(val & AXP2202_MASK_BAT_STAT))
			battery_exist = 0;
	} else {
		battery_exist = bat_power->qc_bat_exist;
	}

	/* update battery model*/
	if (battery_exist && !axp2202_model_update_check(bat_power->regmap)) {
		PMIC_DEV_ERR(bat_power->dev, "axp2202 model need update!\n");
		ret = axp2202_model_update(bat_power);
		if (ret < 0) {
			PMIC_DEV_ERR(bat_power->dev, "axp2202 model update fail!\n");
			return ret;
		}
	}
	PMIC_DEV_DEBUG(bat_power->dev, "axp2202 model update ok:battery_exist:%d\n", battery_exist);
	/*end of update battery model*/

	if (!bat_power->qc_supply_exist) {
		/* set pre-charge current to 128mA*/
		val = 0x02;
		regmap_update_bits(bat_power->regmap, AXP2202_IPRECHG_CFG, 0x0f, val);

		/* set terminal charge current */
		if (axp_config->pmu_terminal_chgcur < 64)
			val = 0x01;
		else if (axp_config->pmu_terminal_chgcur > 960)
			val = 0x0f;
		else
			val = axp_config->pmu_terminal_chgcur / 64;
		regmap_update_bits(bat_power->regmap, AXP2202_ITERM_CFG, 0x0f, val);
	}

	if (!battery_exist)
		axp_config->pmu_bat_temp_enable = 0;

	/* enable ntc */
	if (axp_config->pmu_bat_temp_enable) {
		regmap_update_bits(bat_power->regmap, AXP2202_TS_CFG, AXP2202_TS_ENABLE_MARK, 0);

		/* set ntc curr */
		regmap_read(bat_power->regmap, AXP2202_TS_CFG, &val);
		val &= 0xFC;
		if (axp_config->pmu_bat_ts_current < 40)
			val |= 0x00;
		else if (axp_config->pmu_bat_ts_current < 50)
			val |= 0x01;
		else if (axp_config->pmu_bat_ts_current < 60)
			val |= 0x02;
		else
			val |= 0x03;
		regmap_write(bat_power->regmap, AXP2202_TS_CFG, val);

		/* set ntc vol */
		if (axp_config->pmu_bat_charge_ltf) {
			if (axp_config->pmu_bat_charge_ltf < axp_config->pmu_bat_charge_htf)
				axp_config->pmu_bat_charge_ltf = axp_config->pmu_bat_charge_htf;

			val = axp_config->pmu_bat_charge_ltf / 32;
			regmap_write(bat_power->regmap, AXP2202_VLTF_CHG, val);
		}

		if (axp_config->pmu_bat_charge_htf) {
			if (axp_config->pmu_bat_charge_htf > 510)
				axp_config->pmu_bat_charge_htf = 510;

			val = axp_config->pmu_bat_charge_htf / 2;
			regmap_write(bat_power->regmap, AXP2202_VHTF_CHG, val);
		}

		/* set work vol */
		if (axp_config->pmu_bat_shutdown_ltf) {
			if (axp_config->pmu_bat_shutdown_ltf < axp_config->pmu_bat_charge_ltf)
				axp_config->pmu_bat_shutdown_ltf = axp_config->pmu_bat_charge_ltf;

			val = axp_config->pmu_bat_shutdown_ltf / 32;
			regmap_write(bat_power->regmap, AXP2202_VLTF_WORK, val);
		}

		if (axp_config->pmu_bat_shutdown_htf) {
			if (axp_config->pmu_bat_shutdown_htf > axp_config->pmu_bat_charge_htf)
				axp_config->pmu_bat_shutdown_htf = axp_config->pmu_bat_charge_htf;

			val = axp_config->pmu_bat_shutdown_htf / 2;
			regmap_write(bat_power->regmap, AXP2202_VHTF_WORK, val);
		}

		if (bat_power->qc_supply_exist) {
			val = axp_config->pmu_bat_charge_ltf / 32;
			regmap_write(bat_power->regmap, AXP2202_VLTF_WORK, val);
			val = axp_config->pmu_bat_charge_htf / 2;
			regmap_write(bat_power->regmap, AXP2202_VHTF_WORK, val);
		}
	} else {
		regmap_update_bits(bat_power->regmap, AXP2202_TS_CFG, AXP2202_TS_ENABLE_MARK, AXP2202_TS_ENABLE_MARK);
	}

	/* set jeita enable */
	if (axp_config->pmu_jetia_en && axp_config->pmu_bat_temp_enable) {
		regmap_update_bits(bat_power->regmap, AXP2202_JEITA_CFG, AXP2202_JEITA_ENABLE_MARK, 1);

		/* set jeita cool vol */
		if (axp_config->pmu_jetia_cool) {
			if (axp_config->pmu_jetia_cool < axp_config->pmu_jetia_warm)
				axp_config->pmu_jetia_cool = axp_config->pmu_jetia_warm;

			val = axp_config->pmu_jetia_cool / 16;
			regmap_write(bat_power->regmap, AXP2202_JEITA_COOL, val);
		}

		/* set jeita warm vol */
		if (axp_config->pmu_jetia_warm) {
			if (axp_config->pmu_jetia_warm > 2040)
				axp_config->pmu_jetia_warm = 2040;

			val = axp_config->pmu_jetia_warm / 8;
			regmap_write(bat_power->regmap, AXP2202_JEITA_WARM, val);
		}

		/* set jeita config */
		regmap_read(bat_power->regmap, AXP2202_JEITA_CV_CFG, &val);
		val &= 0x0F;
		if (axp_config->pmu_jwarm_ifall)
			val |= axp_config->pmu_jwarm_ifall << 6;

		if (axp_config->pmu_jcool_ifall)
			val |= axp_config->pmu_jcool_ifall << 4;

		regmap_write(bat_power->regmap, AXP2202_JEITA_CV_CFG, val);

		if (bat_power->qc_supply_exist) {
			val = axp_config->pmu_jetia_cool / 32;
			regmap_write(bat_power->regmap, AXP2202_VLTF_CHG, val);
			val = axp_config->pmu_jetia_warm / 2;
			regmap_write(bat_power->regmap, AXP2202_VHTF_CHG, val);
		}
	} else {
		regmap_update_bits(bat_power->regmap, AXP2202_JEITA_CFG, AXP2202_JEITA_ENABLE_MARK, 0);
	}

	if (!bat_power->qc_supply_exist) {
		/* set CHGLED */
		regmap_read(bat_power->regmap, AXP2202_CHGLED_CFG, &reg_value);
		reg_value &= 0xf8;
		if (axp_config->pmu_chgled_func) {
			reg_value |= axp_config->pmu_chgled_type;
			regmap_write(bat_power->regmap, AXP2202_CHGLED_CFG, reg_value);
		}
	}

	/* set charger voltage limit */
	if (axp_config->pmu_init_chgvol < 4100) {
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_4_0V);
	} else if (axp_config->pmu_init_chgvol < 4200) {
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_4_1V);
	} else if (axp_config->pmu_init_chgvol < 4350) {
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_4_2V);
	} else if (axp_config->pmu_init_chgvol < 4400) {
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_4_35V);
	} else if (axp_config->pmu_init_chgvol < 5000) {
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_4_4V);
	} else {
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_5_0V);
	}

	if (!bat_power->qc_supply_exist) {
		/*  charger change current can be divided by 64 */
		axp_config->pmu_runtime_chgcur = clamp_val(axp_config->pmu_runtime_chgcur, 0, 3072);
		axp_config->pmu_suspend_chgcur = clamp_val(axp_config->pmu_suspend_chgcur, 0, 3072);
		axp_config->pmu_shutdown_chgcur = clamp_val(axp_config->pmu_shutdown_chgcur, 0, 3072);
		axp_config->pmu_bat_charge_control_lim = clamp_val(axp_config->pmu_bat_charge_control_lim, 0, 3072);

		axp_config->pmu_runtime_chgcur = DIVIDE_BY_64(axp_config->pmu_runtime_chgcur);
		axp_config->pmu_suspend_chgcur = DIVIDE_BY_64(axp_config->pmu_suspend_chgcur);
		axp_config->pmu_shutdown_chgcur = DIVIDE_BY_64(axp_config->pmu_shutdown_chgcur);
		axp_config->pmu_bat_charge_control_lim = DIVIDE_BY_64(axp_config->pmu_bat_charge_control_lim);

		/*  set charger charge current */
		val = axp_config->pmu_runtime_chgcur / 64;
		regmap_update_bits(bat_power->regmap, AXP2202_ICC_CFG, GENMASK(5, 0),
					   val);
	}


	/* set gauge_thld */
	val = clamp_val(axp_config->pmu_battery_warning_level1 - 5, 0, 15) << 4;
	val |= clamp_val(axp_config->pmu_battery_warning_level2, 0, 15);
	regmap_write(bat_power->regmap, AXP2202_GAUGE_THLD, val);

	/* battery ratio check */
	regmap_read(bat_power->regmap, AXP2202_GAUGE_SOC, &val);
	PMIC_INFO("radio:%d", val);
	if (val > 60) {
		ret = regmap_bulk_read(bat_power->regmap, AXP2202_VBAT_H, data, 2);
		val = ((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]);
		PMIC_WARN("bat_vol:%d\n\n", val);
		if (val < 3500) {
			_axp2202_get_bat_status(bat_power, &bat_val);
			if (bat_val.intval == POWER_SUPPLY_STATUS_CHARGING) {
				axp2202_reset_mcu(bat_power);
				regmap_write(bat_power->regmap, AXP2202_CURVE_CHECK, 0x81);
				PMIC_WARN("adapt reset gauge: soc > 60%% , bat_vol < 3500\n");
			}
		} else {
			regmap_write(bat_power->regmap, AXP2202_CURVE_CHECK, 0x00);
		}
	}

	/* init charger cooling type */
	if (of_find_property(bat_power->dev->of_node, "#cooling-cells", NULL)) {
		PMIC_INFO("charger cooling type: new\n");
		bat_power->charger_cooler_type = 1;
	} else {
		PMIC_INFO("charger cooling type: old\n");
		bat_power->charger_cooler_type = 0;
	}

	return ret;
}

static irqreturn_t axp2202_irq_handler_bat_stat_change(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	struct axp2202_bat_power *bat_power = data;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	switch (id->irq_data.hwirq) {
	case AXP2202_IRQ_CHGDN:
		PMIC_DEBUG("interrupt:charger done");
		break;
	case AXP2202_IRQ_CHGST:
		PMIC_DEBUG("interrutp:charger start");
		break;
	case AXP2202_IRQ_BINSERT:
		PMIC_DEBUG("interrupt:battery insert");
		break;
	case AXP2202_IRQ_BREMOVE:
		PMIC_DEBUG("interrupt:battery remove");
		break;
	case AXP2202_IRQ_BWOT:
		PMIC_DEBUG("interrupt:battery over temp work");
		break;
	case AXP2202_IRQ_BWUT:
		PMIC_DEBUG("interrupt:battery under temp work");
		break;
	case AXP2202_IRQ_NEWSOC:
		if (bat_power->qc_supply_exist) {
			if (bat_power->dts_info.pmu_bat_temp_enable) {
				union power_supply_propval qc_val;
				qc_val.intval = 1;
				power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP, &qc_val);
			}
		}
		PMIC_DEBUG("interrupt:battery new soc");
		break;
	default:
		PMIC_DEBUG("interrupt:others");
		break;
	}

	return IRQ_HANDLED;
}

static irqreturn_t axp2202_irq_handler_bat_temp_change(int irq, void *data)
{
	struct axp2202_bat_power *bat_power = data;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	if (bat_power->qc_supply_exist) {
		if (bat_power->dts_info.pmu_bat_temp_enable) {
			union power_supply_propval qc_val;
			qc_val.intval = 1;
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP, &qc_val);
		}
	}

	return IRQ_HANDLED;
}

enum axp2202_bat_virq_index {
	AXP2202_VIRQ_BAT_IN,
	AXP2202_VIRQ_BAT_OUT,
	AXP2202_VIRQ_CHARGING,
	AXP2202_VIRQ_CHARGE_OVER,
	AXP2202_VIRQ_LOW_WARNING1,
	AXP2202_VIRQ_LOW_WARNING2,
	AXP2202_VIRQ_BAT_UNTEMP_WORK,
	AXP2202_VIRQ_BAT_OVTEMP_WORK,
	AXP2202_VIRQ_BAT_UNTEMP_CHG,
	AXP2202_VIRQ_BAT_OVTEMP_CHG,
	AXP2202_VIRQ_BAT_OV,
	AXP2202_VIRQ_BAT_NEW_SOC,
	AXP2202_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_bat_irq[] = {
	[AXP2202_VIRQ_BAT_IN] = { "battery_insert",
				  axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_BAT_OUT] = { "battery_remove",
				   axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_CHARGING] = { "charge_start",
				    axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_CHARGE_OVER] = { "charge_done",
				       axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_LOW_WARNING1] = { "SOC_low_warning1",
					axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_LOW_WARNING2] = { "SOC_low_warning2",
					axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_BAT_UNTEMP_WORK] = { "battery_under_temp_work",
					   axp2202_irq_handler_bat_temp_change },
	[AXP2202_VIRQ_BAT_OVTEMP_WORK] = { "battery_over_temp_work",
					   axp2202_irq_handler_bat_temp_change },
	[AXP2202_VIRQ_BAT_UNTEMP_CHG] = { "battery_under_temp_chg",
					  axp2202_irq_handler_bat_temp_change },
	[AXP2202_VIRQ_BAT_OVTEMP_CHG] = { "battery_over_temp_chg",
					  axp2202_irq_handler_bat_temp_change },
	[AXP2202_VIRQ_BAT_OV] = { "battery_over_voltage",
					  axp2202_irq_handler_bat_stat_change },
	[AXP2202_VIRQ_BAT_NEW_SOC] = { "gauge_new_soc",
					  axp2202_irq_handler_bat_stat_change },
};

static int axp2202_bat_dt_parse(struct device_node *node,
			 struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_chg_ic_temp,                   0);
	AXP_OF_PROP_READ(pmu_runtime_chgcur,              500);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_prechg_chgcur,               100);
	AXP_OF_PROP_READ(pmu_terminal_chgcur,             128);
	AXP_OF_PROP_READ(pmu_init_chgvol,                4200);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_chgled_func,                   1);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);
	AXP_OF_PROP_READ(pmu_batdeten,                      1);

	AXP_OF_PROP_READ(pmu_bat_ts_current,               50);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             1312);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,              176);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           1984);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,            152);

	AXP_OF_PROP_READ(pmu_jetia_en,                      0);
	AXP_OF_PROP_READ(pmu_jetia_cool,                  880);
	AXP_OF_PROP_READ(pmu_jetia_warm,                  240);
	AXP_OF_PROP_READ(pmu_jcool_ifall,                   1);
	AXP_OF_PROP_READ(pmu_jwarm_ifall,                   0);

	AXP_OF_PROP_READ(pmu_bat_temp_enable,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para1,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para2,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para3,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para4,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para5,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para6,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para7,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para8,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para9,                0);
	AXP_OF_PROP_READ(pmu_bat_temp_para10,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para11,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para12,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para13,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para14,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para15,               0);
	AXP_OF_PROP_READ(pmu_bat_temp_para16,               0);

	AXP_OF_PROP_READ(pmu_bat_charge_control_lim,        600);
	AXP_OF_PROP_READ(pmu_bat_cycle_life,                800);
	AXP_OF_PROP_READ(pmu_bat_cycle_cap_reduce,          20);

	AXP_OF_PROP_READ(pmu_bat_manufacture_year,       2024);
	AXP_OF_PROP_READ(pmu_bat_manufacture_month,         1);
	AXP_OF_PROP_READ(pmu_bat_manufacture_day,           1);

	axp_config->wakeup_bat_in =
		of_property_read_bool(node, "wakeup_bat_in");
	axp_config->wakeup_bat_out =
		of_property_read_bool(node, "wakeup_bat_out");
	axp_config->wakeup_bat_charging =
		of_property_read_bool(node, "wakeup_bat_charging");
	axp_config->wakeup_bat_charge_over =
		of_property_read_bool(node, "wakeup_bat_charge_over");
	axp_config->wakeup_low_warning1 =
		of_property_read_bool(node, "wakeup_low_warning1");
	axp_config->wakeup_low_warning2 =
		of_property_read_bool(node, "wakeup_low_warning2");
	axp_config->wakeup_bat_untemp_work =
		of_property_read_bool(node, "wakeup_bat_untemp_work");
	axp_config->wakeup_bat_ovtemp_work =
		of_property_read_bool(node, "wakeup_bat_ovtemp_work");
	axp_config->wakeup_untemp_chg =
		of_property_read_bool(node, "wakeup_bat_untemp_chg");
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");
	axp_config->wakeup_bat_ov =
		of_property_read_bool(node, "wakeup_bat_ov");
	axp_config->wakeup_new_soc =
		of_property_read_bool(node, "wakeup_new_soc");


	return 0;
}

static void axp2202_bat_parse_device_tree(struct axp2202_bat_power *bat_power)
{
	int ret;
	struct axp_config_info *axp_config;

	/* set input current limit */
	if (!bat_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	axp_config = &bat_power->dts_info;
	ret = axp2202_bat_dt_parse(bat_power->dev->of_node, axp_config);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}
}

static void axp2202_bat_power_monitor(struct work_struct *work)
{
	struct axp2202_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	union power_supply_propval qc_val;
	unsigned char temp_val[2];
	unsigned int reg_value;
	int ret;

	power_supply_changed(bat_power->bat_supply);

	 /* 0xb2, 0xb3 return mcoul high 16 bit value */
	reg_value = 0x2e;
	ret = regmap_write(bat_power->regmap, AXP2202_GAUGE_FG_ADDR, reg_value);

	regmap_read(bat_power->regmap, AXP2202_GAUGE_CONFIG, &reg_value);
	if (reg_value & BIT(4)) {
		regmap_read(bat_power->regmap, AXP2202_GAUGE_FG_ADDR, &reg_value);
		if (reg_value == 0x2e) {
		/* reset gauge if  is overflow */
		regmap_bulk_read(bat_power->regmap, AXP2202_GAUGE_FG_DATA_H, temp_val, 2);
		reg_value = (temp_val[0] << 8) + temp_val[1];
			if ((reg_value != 0xffff) && (reg_value != 0x0000)) {
				axp2202_reset_mcu(bat_power);
				PMIC_WARN("reset gauge:mcoul overflow\n");
			}
		}
	}

	switch (axp_config->pmu_init_chgvol) {
	case 4100:
	case 4200:
	case 4350:
	case 4400:
	case 5000:
		break;
	default:
		regmap_read(bat_power->regmap, AXP2202_GAUGE_SOC, &reg_value);
		if (reg_value == 100) {
			if (bat_power->qc_supply_exist) {
				qc_val.intval = 1;
				power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
			}
			regmap_update_bits(bat_power->regmap, AXP2202_MODULE_EN, BIT(1), 0);
		} else {
			if (bat_power->qc_supply_exist) {
				qc_val.intval = 2;
				power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
			}
			regmap_update_bits(bat_power->regmap, AXP2202_MODULE_EN, BIT(1), BIT(1));
		}
		break;
	}

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

static void axp2202_bat_power_curve_monitor(struct work_struct *work)
{
	struct axp2202_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_power_curve.work);
	union power_supply_propval val;
	struct regmap *regmap = bat_power->regmap;
	static int rest_vol, blance_vol;
	unsigned int reg_value;
	int ret;

	power_supply_changed(bat_power->bat_supply);

	ret = regmap_read(regmap, AXP2202_CURVE_CHECK, &reg_value);

	if (reg_value & 0x80) {
		blance_vol = axp2202_blance_vol(regmap);
		PMIC_DEBUG("blance_vol = %d\n", blance_vol);

		rest_vol = reg_value & 0x7F;
		if (blance_vol >= 1) {
			if (!bat_power->qc_supply_exist) {
				ret = regmap_read(regmap, AXP2202_COMM_STAT0, &reg_value);
				reg_value &= AXP2202_MASK_VBUS_STAT;
			} else {
				power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_PRESENT, &val);
				reg_value = val.intval;
			}

			if (reg_value)
				rest_vol++;

			PMIC_DEBUG("%s:rest_vol:%d\n", __func__, rest_vol);
			ret = regmap_update_bits(regmap, AXP2202_CURVE_CHECK, GENMASK(6, 0), rest_vol);
			schedule_delayed_work(&bat_power->bat_power_curve, msecs_to_jiffies(30 * 1000));
		} else {
			PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
			__pm_relax(bat_power->ws);
			ret = regmap_write(regmap, AXP2202_CURVE_CHECK, 0);
		}
	} else {
		PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
		__pm_relax(bat_power->ws);
		ret = regmap_write(regmap, AXP2202_CURVE_CHECK, 0);
	}
}

static int axp2202_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq, reg_value;

	struct axp2202_bat_power *bat_power;
	struct power_supply_config psy_cfg = {};
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *node = pdev->dev.of_node;
	union power_supply_propval val;

	if (!of_device_is_available(node)) {
		PMIC_ERR("axp2202-battery device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp2202-battery without irq\n");
		return -EINVAL;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (bat_power == NULL) {
		PMIC_ERR("axp2202_bat_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	bat_power->name = "axp2202_battery";
	bat_power->dev = &pdev->dev;
	bat_power->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp2202_bat_parse_device_tree(bat_power);

	ret = axp2202_qc_para_get(bat_power);
	if (!ret) {
		PMIC_INFO("axp2202:used quick charge, paras init finish\n");
		regmap_update_bits(bat_power->regmap, AXP2202_MODULE_EN, BIT(1), 0);
		regmap_update_bits(bat_power->regmap, AXP2202_VTERM_CFG, 0x07, AXP2202_CHRG_CTRL1_TGT_4_0V);
	} else if (ret == -EPROBE_DEFER) {
		PMIC_INFO("axp2202:used quick charge, paras init later\n");
		return ret;
	} else {
		PMIC_INFO("axp2202:no quick charge\n");
	}

	ret = axp2202_init_chip(bat_power);
	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp2202 init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
			&axp2202_bat_desc, &psy_cfg);

	if (IS_ERR(bat_power->bat_supply)) {
		PMIC_ERR("axp2202 failed to register bat power\n");
		ret = PTR_ERR(bat_power->bat_supply);
		return ret;
	}

	axp20x_register_cooler(bat_power->bat_supply);

	for (i = 0; i < ARRAY_SIZE(axp_bat_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp_bat_irq[i].name);
		if (irq < 0)
			continue;

		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		if (irq < 0) {
			PMIC_DEV_ERR(&pdev->dev, "can not get irq\n");
			return irq;
		}
		/* we use this variable to suspend irq */
		axp_bat_irq[i].irq = irq;
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp_bat_irq[i].isr, 0,
						   axp_bat_irq[i].name, bat_power);
		if (ret < 0) {
			PMIC_DEV_ERR(&pdev->dev, "failed to request %s IRQ %d: %d\n",
				axp_bat_irq[i].name, irq, ret);
			return ret;
		} else {
			ret = 0;
		}

		PMIC_DEV_DEBUG(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp_bat_irq[i].name, irq, ret);
	}
	platform_set_drvdata(pdev, bat_power);

	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp2202_bat_power_monitor);
	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(500));

	ret = regmap_read(bat_power->regmap, AXP2202_CURVE_CHECK, &reg_value);
	if (ret < 0)
		return ret;

	if (reg_value & 0x80) {
		reg_value = axp2202_blance_vol(bat_power->regmap);
		PMIC_DEBUG("bat curve smoothing: hold wake lock,blance_vol = %d\n", reg_value);
		bat_power->ws = wakeup_source_register(&pdev->dev, "bat_curve_smooth");
		__pm_stay_awake(bat_power->ws);
		INIT_DELAYED_WORK(&bat_power->bat_power_curve, axp2202_bat_power_curve_monitor);
		schedule_delayed_work(&bat_power->bat_power_curve, msecs_to_jiffies(30 * 1000));
	}

	if (bat_power->qc_supply_exist) {
		val.intval = 1;
		power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CALIBRATE, &val);
	}
	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp2202_battery_remove(struct platform_device *pdev)
{
	struct axp2202_bat_power *bat_power = platform_get_drvdata(pdev);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP2202 unegister==============\n");
	if (bat_power->bat_supply) {
		power_supply_unregister(bat_power->bat_supply);
		axp20x_unregister_cooler(bat_power->bat_supply);
	}
	PMIC_DEV_DEBUG(&pdev->dev, "axp2202 teardown battery dev\n");

	return 0;
}

static void axp2202_charger_ichg_set(struct axp2202_bat_power *bat_power, int mA)
{

	mA = clamp_val(mA, 0, 3072);
	mA = mA / 64;
	/* bit 5:0 is the ctrl bit */
	regmap_update_bits(bat_power->regmap, AXP2202_ICC_CFG, GENMASK(5, 0), mA);
}

static inline void axp2202_bat_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2202_bat_virq_dts_set(struct axp2202_bat_power *bat_power, bool enable)
{
	struct axp_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp2202_bat_irq_set(axp_bat_irq[AXP2202_VIRQ_BAT_IN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp2202_bat_irq_set(axp_bat_irq[AXP2202_VIRQ_BAT_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp2202_bat_irq_set(axp_bat_irq[AXP2202_VIRQ_CHARGING].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp2202_bat_irq_set(axp_bat_irq[AXP2202_VIRQ_CHARGE_OVER].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp2202_bat_irq_set(axp_bat_irq[AXP2202_VIRQ_LOW_WARNING1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp2202_bat_irq_set(axp_bat_irq[AXP2202_VIRQ_LOW_WARNING2].irq,
				enable);
	if (!dts_info->wakeup_bat_untemp_work)
		axp2202_bat_irq_set(
			axp_bat_irq[AXP2202_VIRQ_BAT_UNTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp2202_bat_irq_set(
			axp_bat_irq[AXP2202_VIRQ_BAT_OVTEMP_WORK].irq,
			enable);
	if (!dts_info->wakeup_untemp_chg)
		axp2202_bat_irq_set(
			axp_bat_irq[AXP2202_VIRQ_BAT_UNTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp2202_bat_irq_set(
			axp_bat_irq[AXP2202_VIRQ_BAT_OVTEMP_CHG].irq,
			enable);
	if (!dts_info->wakeup_bat_ov)
		axp2202_bat_irq_set(
			axp_bat_irq[AXP2202_VIRQ_BAT_OV].irq,
			enable);
	if (!dts_info->wakeup_new_soc)
		axp2202_bat_irq_set(
			axp_bat_irq[AXP2202_VIRQ_BAT_NEW_SOC].irq,
			enable);

}

static void axp2202_bat_shutdown(struct platform_device *pdev)
{
	struct axp2202_bat_power *bat_power = platform_get_drvdata(pdev);
	int reg_value;

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	regmap_read(bat_power->regmap, AXP2202_CURVE_CHECK, &reg_value);
	if (reg_value & 0x80) {
		cancel_delayed_work_sync(&bat_power->bat_power_curve);
	}

	if (!bat_power->qc_supply_exist) {
		axp2202_charger_ichg_set(bat_power, bat_power->dts_info.pmu_shutdown_chgcur);
	}

}

static int axp2202_bat_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp2202_bat_power *bat_power = platform_get_drvdata(pdev);
	int reg_value;

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);
	atomic_set(&bat_power->pmu_limit_status, 0);
	regmap_read(bat_power->regmap, AXP2202_CURVE_CHECK, &reg_value);
	if (reg_value & 0x80) {
		cancel_delayed_work_sync(&bat_power->bat_power_curve);
	}
	if (!bat_power->qc_supply_exist) {
		axp2202_charger_ichg_set(bat_power, bat_power->dts_info.pmu_suspend_chgcur);
	}
	axp2202_bat_virq_dts_set(bat_power, false);
	return 0;
}

static int axp2202_bat_resume(struct platform_device *pdev)
{
	struct axp2202_bat_power *bat_power = platform_get_drvdata(pdev);
	int reg_value;

	power_supply_changed(bat_power->bat_supply);

	schedule_delayed_work(&bat_power->bat_supply_mon, 0);
	regmap_read(bat_power->regmap, AXP2202_CURVE_CHECK, &reg_value);
	if (reg_value & 0x80) {
		schedule_delayed_work(&bat_power->bat_power_curve, 0);
	}
	if (!bat_power->qc_supply_exist) {
		axp2202_charger_ichg_set(bat_power, bat_power->dts_info.pmu_runtime_chgcur);
	}
	axp2202_bat_virq_dts_set(bat_power, true);
	return 0;
}

static const struct of_device_id axp2202_bat_power_match[] = {
	{
		.compatible = "x-powers,axp2202-bat-power-supply",
		.data = (void *)AXP2202_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp2202_bat_power_match);

static struct platform_driver axp2202_bat_power_driver = {
	.driver = {
		.name = "axp2202-bat-power-supply",
		.of_match_table = axp2202_bat_power_match,
	},
	.probe = axp2202_battery_probe,
	.remove = axp2202_battery_remove,
	.shutdown = axp2202_bat_shutdown,
	.suspend = axp2202_bat_suspend,
	.resume = axp2202_bat_resume,
};

module_platform_driver(axp2202_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp2202 battery driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
