/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp515_charger.h"

#define DIVIDE_BY_64(n) (((n) >> 6) << 6)

struct axp515_bat_power {
	char			*name;
	struct device		*dev;
	struct regmap		*regmap;
	struct power_supply	*bat_supply;
	struct delayed_work	bat_supply_mon;
	struct axp_config_info	dts_info;
	struct mutex		lock;

	/* bat_temp_process */
	struct delayed_work	bat_temp_process;
	struct delayed_work	bat_temp_init;
	struct wakeup_source	*bat_temp_ws;
	int			bat_temp_calib;
	int			bat_jeita_cool;
	int			bat_jeita_warm;
	int			bat_charge_ltf;
	int			bat_charge_htf;
	int			bat_shutdown_ltf;
	int			bat_shutdown_htf;

	/* power supply notifier */
	struct notifier_block	bat_nb;

	/* charge_limit_process */
	atomic_t		pmu_ntc_status;
	atomic_t		pmu_jeita_status;
	atomic_t		pmu_limit_status;
	atomic_t		pmu_chg_status_sets;

	atomic_t	 	charge_set_limit;
	atomic_t	 	charge_control_lim;
	/* charge_cycle_count */
	atomic_t	 	charge_cycle_count;
};

static enum power_supply_property axp515_bat_props[] = {
	/* real_time */
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_LEVEL,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_COUNTER,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	/* static */
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_MANUFACTURE_YEAR,
	POWER_SUPPLY_PROP_MANUFACTURE_MONTH,
	POWER_SUPPLY_PROP_MANUFACTURE_DAY,
};

static inline int axp515_vbat_to_mV(u32 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1200 / 1000;
}

static inline int axp515_ibat_to_mA(u32 reg)
{
	return (int)((((reg >> 8) << 4) | (reg & 0x000f)) << 1);
}

static inline int axp_vts_to_mV(u16 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 800 / 1000;
}

static int axp515_ichg(struct regmap *regmap)
{
	uint8_t tmp[2];
	int ibat;
	u32 res;

	regmap_bulk_read(regmap, AXP515_IBATH_REG, tmp, 2);
	res = (tmp[0] << 8) | tmp[1];
	ibat = axp515_ibat_to_mA(res);

	return ibat;
}

static int axp515_disichg(struct regmap *regmap)
{
	uint8_t tmp[2];
	int disibat;
	u32 res;

	regmap_bulk_read(regmap, AXP515_DISIBATH_REG, tmp, 2);
	res = (tmp[0] << 8) | tmp[1];
	disibat = axp515_ibat_to_mA(res);

	return disibat;
}

static int axp515_get_bat_present(struct axp515_bat_power *bat_power)
{
	return 1;
}

static int axp515_get_soc(struct power_supply *ps,
			    union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value;
	int ret, rest_vol, ocv_percent = 0, coul_percent = 0;
	bool bat_charging, vbus_good, bat_charge_done;
	static int rest_vol_old, axp515_max_soc;

	ret = regmap_read(regmap, AXP515_CAP, &reg_value);
	if (ret < 0)
		return ret;

	if (!(reg_value & AXP515_CAP_EN)) {
		val->intval = rest_vol_old;
		return 0;
	}

	rest_vol = (int) (reg_value & 0x7F);

	ret = regmap_read(regmap, AXP515_OCV_PERCENT, &reg_value);
	if (ret < 0)
		return ret;

	if (reg_value & AXP515_OCV_PER_EN)
		ocv_percent = reg_value & 0x7f;

	ret = regmap_read(bat_power->regmap, AXP515_CLUMB_PERCENT, &reg_value);
	if (ret)
		return ret;

	if (reg_value & AXP515_GAUGE_EN)
		coul_percent = (int)(reg_value & 0x7F);


	ret = regmap_read(regmap, AXP515_STATUS0, &reg_value);
	if (ret < 0)
		return ret;

	bat_charging = ((((reg_value & AXP515_MASK_CHARGE) > 0))
			&& ((reg_value & AXP515_MASK_CHARGE) < AXP515_CHARGE_MAX)) ? 1 : 0;
	vbus_good = reg_value & AXP515_MASK_VBUS_STAT;
	bat_charge_done = ((reg_value & AXP515_CHARGE_DONE) == AXP515_CHARGE_DONE) ? 1 : 0;

	if (ocv_percent == 100 && bat_charging == 0 && rest_vol == 99 && vbus_good) {
		ret = regmap_update_bits(regmap, AXP515_COULOMB_CTL,
					 AXP515_GAUGE_EN, 0);

		ret = regmap_update_bits(regmap, AXP515_COULOMB_CTL,
					 AXP515_GAUGE_EN, AXP515_GAUGE_EN);
		if (ret < 0)
			return ret;

		rest_vol = 100;
	}

	if (ocv_percent == 100 && coul_percent == 100 && axp_config->ocv_coulumb_100 == 1) {
		rest_vol = 100;
	}

	if (rest_vol > AXP515_SOC_MAX)
		rest_vol = AXP515_SOC_MAX;
	else if (rest_vol < AXP515_SOC_MIN)
		rest_vol = AXP515_SOC_MIN;

	if (axp_config->pmu_chgled_func && axp_config->ocv_coulumb_100) {
		if (rest_vol == 100) {
			ret = regmap_read(regmap, AXP515_CHGLED_CFG, &reg_value);
			if (ret < 0)
				return ret;
			if (reg_value & BIT(7))
				regmap_update_bits(regmap, AXP515_CHGLED_CFG, BIT(7), BIT(7));
		} else {
			ret = regmap_read(regmap, AXP515_CHGLED_CFG, &reg_value);
			if (ret < 0)
				return ret;
			if (!(reg_value & BIT(7)))
				regmap_update_bits(regmap, AXP515_CHGLED_CFG, BIT(7), 0);
		}
	}

	if (axp515_max_soc) {
		if (!vbus_good) {
			axp515_max_soc = 0;
		} else {
			if (bat_charge_done)
				axp515_max_soc = 0;
		}
	}

	if (bat_charge_done && (rest_vol == 99) && vbus_good) {
		regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), 0);
		regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), BIT(7));
		axp515_max_soc = 1;
	}

	if (axp515_max_soc)
		rest_vol = 100;

	rest_vol_old = rest_vol;

	val->intval = rest_vol;

	return 0;
}

/* read temperature */
static inline int axp_vts_to_temp(int data,
		const struct axp_config_info *axp_config)
{
	int temp;

	if (data < 80 || !axp_config->pmu_bat_temp_enable) {
		return 300;
	} else if (data < axp_config->pmu_bat_temp_para16) {
		return 800;
	} else if (data <= axp_config->pmu_bat_temp_para15) {
		temp = 700 + (axp_config->pmu_bat_temp_para15-data) * 100 /
		(axp_config->pmu_bat_temp_para15 - axp_config->pmu_bat_temp_para16);
	} else if (data <= axp_config->pmu_bat_temp_para14) {
		temp = 600 + (axp_config->pmu_bat_temp_para14-data) * 100 /
		(axp_config->pmu_bat_temp_para14 - axp_config->pmu_bat_temp_para15);
	} else if (data <= axp_config->pmu_bat_temp_para13) {
		temp = 550 + (axp_config->pmu_bat_temp_para13-data) * 50 /
		(axp_config->pmu_bat_temp_para13 - axp_config->pmu_bat_temp_para14);
	} else if (data <= axp_config->pmu_bat_temp_para12) {
		temp = 500 + (axp_config->pmu_bat_temp_para12-data) * 50 /
		(axp_config->pmu_bat_temp_para12 - axp_config->pmu_bat_temp_para13);
	} else if (data <= axp_config->pmu_bat_temp_para11) {
		temp = 450 + (axp_config->pmu_bat_temp_para11-data) * 50 /
		(axp_config->pmu_bat_temp_para11 - axp_config->pmu_bat_temp_para12);
	} else if (data <= axp_config->pmu_bat_temp_para10) {
		temp = 400 + (axp_config->pmu_bat_temp_para10-data) * 50 /
		(axp_config->pmu_bat_temp_para10 - axp_config->pmu_bat_temp_para11);
	} else if (data <= axp_config->pmu_bat_temp_para9) {
		temp = 300 + (axp_config->pmu_bat_temp_para9-data) * 100 /
		(axp_config->pmu_bat_temp_para9 - axp_config->pmu_bat_temp_para10);
	} else if (data <= axp_config->pmu_bat_temp_para8) {
		temp = 200 + (axp_config->pmu_bat_temp_para8-data) * 100 /
		(axp_config->pmu_bat_temp_para8 - axp_config->pmu_bat_temp_para9);
	} else if (data <= axp_config->pmu_bat_temp_para7) {
		temp = 100 + (axp_config->pmu_bat_temp_para7-data) * 100 /
		(axp_config->pmu_bat_temp_para7 - axp_config->pmu_bat_temp_para8);
	} else if (data <= axp_config->pmu_bat_temp_para6) {
		temp = 50 + (axp_config->pmu_bat_temp_para6-data) * 50 /
		(axp_config->pmu_bat_temp_para6 - axp_config->pmu_bat_temp_para7);
	} else if (data <= axp_config->pmu_bat_temp_para5) {
		temp = 0 + (axp_config->pmu_bat_temp_para5-data) * 50 /
		(axp_config->pmu_bat_temp_para5 - axp_config->pmu_bat_temp_para6);
	} else if (data <= axp_config->pmu_bat_temp_para4) {
		temp = -50 + (axp_config->pmu_bat_temp_para4-data) * 50 /
		(axp_config->pmu_bat_temp_para4 - axp_config->pmu_bat_temp_para5);
	} else if (data <= axp_config->pmu_bat_temp_para3) {
		temp = -100 + (axp_config->pmu_bat_temp_para3-data) * 50 /
		(axp_config->pmu_bat_temp_para3 - axp_config->pmu_bat_temp_para4);
	} else if (data <= axp_config->pmu_bat_temp_para2) {
		temp = -150 + (axp_config->pmu_bat_temp_para2-data) * 50 /
		(axp_config->pmu_bat_temp_para2 - axp_config->pmu_bat_temp_para3);
	} else if (data <= axp_config->pmu_bat_temp_para1) {
		temp = -250 + (axp_config->pmu_bat_temp_para1-data) * 100 /
		(axp_config->pmu_bat_temp_para1 - axp_config->pmu_bat_temp_para2);
	} else {
		temp = -250;
	}

	return temp;
}

static inline int axp515_get_adc_raw(struct regmap *regmap)
{
	unsigned char temp_val[2];
	unsigned short ts_res;
	int bat_temp_mv;
	int ret = 0;

	ret = regmap_bulk_read(regmap, AXP515_VTS_RES, temp_val, 2);
	if (ret < 0)
		return ret;

	ts_res = ((unsigned short) temp_val[0] << 8) | temp_val[1];
	bat_temp_mv = axp_vts_to_mV(ts_res);

	return bat_temp_mv;
}

static inline int axp515_get_adc(struct axp515_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	int calib = bat_power->bat_temp_calib;
	int bat_temp_mv, data;
	bool bat_charging;

	regmap_read(regmap, AXP515_STATUS0, &data);
	bat_charging = ((((data & AXP515_MASK_CHARGE) > 0))
			&& ((data & AXP515_MASK_CHARGE) < AXP515_CHARGE_MAX)) ? 1 : 0;

	bat_temp_mv = axp515_get_adc_raw(regmap);

	if (!bat_charging)
		bat_temp_mv += (calib * axp515_disichg(regmap) / 1000);
	else
		bat_temp_mv -= (calib * axp515_ichg(regmap) / 1000);

	return bat_temp_mv;
}

static int _axp515_get_temp_raw(struct axp515_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;

	int i = 0, temp, old_temp;

	old_temp = axp_vts_to_temp(axp515_get_adc(bat_power), axp_config);

	/* read until abs(old_temp - temp) < 10*/
	temp = axp_vts_to_temp(axp515_get_adc(bat_power), axp_config);

	PMIC_DEBUG("old_temp:%d, temp:%d\n", old_temp, temp);
	while ((abs(old_temp - temp) > 100) && (i < 10)) {
		old_temp = temp;
		temp = axp_vts_to_temp(axp515_get_adc(bat_power), axp_config);
		i++;
		PMIC_DEBUG("turn[%d]:old_temp:%d, temp:%d\n", i, old_temp, temp);
	}

	return temp;
}

static int _axp515_get_temp(struct axp515_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	static int old_temp;

	regmap_read(regmap, AXP515_EXT_PARA5, &data);
	if (!data) {
		old_temp = _axp515_get_temp_raw(bat_power);
	}
	return old_temp;
}

static int axp515_get_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	if (!axp_config->pmu_bat_temp_enable) {
		val->intval = 300;
	} else {
		val->intval = _axp515_get_temp(bat_power);
	}

	return 0;
}

static int axp515_get_bat_health(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	unsigned int reg_value;
	int ret, temp;

	ret = regmap_read(regmap, AXP515_COMM_FAULT, &reg_value);
	if (ret < 0) {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		return ret;
	}

	if (axp515_get_bat_present(bat_power)) {
		axp515_get_temp(ps, val);
		temp = val->intval;
		if ((reg_value & AXP515_MASK_BAT_ACT_STAT) == AXP515_MASK_BAT_ACT_STAT) {
			val->intval = POWER_SUPPLY_HEALTH_DEAD;
		} else if (axp_config->pmu_bat_temp_enable) {
			if (temp < bat_power->bat_charge_ltf) {
				val->intval = POWER_SUPPLY_HEALTH_COLD;
			} else if (temp > bat_power->bat_charge_htf) {
				val->intval = POWER_SUPPLY_HEALTH_OVERHEAT;
			} else {
				val->intval = POWER_SUPPLY_HEALTH_GOOD;
			}
		} else {
			val->intval = POWER_SUPPLY_HEALTH_GOOD;
		}
	} else {
		val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
	}

	return 0;
}

static int axp515_get_bat_status(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	bool bat_det, bat_charging, vbus_good;
	unsigned int data, rest_vol;
	int ret;

	ret = regmap_read(regmap, AXP515_STATUS0, &data);
	if (ret < 0)
		return ret;

	bat_charging = ((((data & AXP515_MASK_CHARGE) > 0))
			&& ((data & AXP515_MASK_CHARGE) < AXP515_CHARGE_MAX)) ? 1 : 0;
	vbus_good = data & AXP515_MASK_VBUS_STAT;

	bat_det = axp515_get_bat_present(bat_power);

	axp515_get_soc(ps, val);
	rest_vol = val->intval;

	if (bat_det) {
		if (rest_vol == 100 && vbus_good)
			val->intval = POWER_SUPPLY_STATUS_FULL;
		else if (bat_charging)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
	} else {
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return 0;
}

static int axp515_get_vbat_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	uint8_t tmp[2];
	u32 res;
	int ret;

	ret = regmap_bulk_read(regmap, AXP515_VBATH_REG, tmp, 2);
	if (ret < 0)
		return ret;
	res = (tmp[0] << 8) | tmp[1];

	val->intval = axp515_vbat_to_mV(res) * 1000;

	return 0;
}

static int axp515_get_ichg(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	int ibat, disibat;

	ibat = axp515_ichg(regmap);
	disibat = axp515_disichg(regmap);

	val->intval = (ibat - disibat) * 1000;

	return 0;
}

static int axp515_bat_get_max_voltage(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	val->intval = axp_config->pmu_init_chgvol;

	return 0;
}

static int _axp515_get_ichg_lim(struct regmap *regmap)
{
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP515_ICC_CFG, &data);
	if (ret < 0)
		return ret;

	data &= 0x3F;
	data = data * 64;

	return data;
}

static int axp515_get_ichg_lim(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;

	ret = _axp515_get_ichg_lim(regmap);
	if (ret < 0)
		return ret;

	val->intval = ret;

	return 0;
}

static int axp515_get_charger_count(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
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

static int axp515_get_charger_count_current(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	unsigned int data[2];
	int ret = 0;

	data[0] = axp_config->pmu_battery_cap * 1000;

	ret = axp515_get_soc(ps, val);
	if (ret < 0)
		return ret;
	data[1] = val->intval;
	data[1] = data[1] * data[0] / 100;

	val->intval = data[1];
	return 0;
}

static int axp515_get_lowsocth(struct power_supply *ps,
				 union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int ret = 0;

	ret = regmap_read(regmap, AXP515_WARNING_LEVEL, &data);
	if (ret < 0)
		return ret;

	val->intval = (data >> 4) + 5;

	return 0;
}

static int axp515_set_lowsocth(struct regmap *regmap, int v)
{
	unsigned int data;
	int ret = 0;

	data = v;

	if (data > 20 || data < 5)
		return -EINVAL;

	data = (data - 5);
	ret = regmap_update_bits(regmap, AXP515_WARNING_LEVEL, GENMASK(7, 4),
				 data);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp515_set_gauge_thld(struct regmap *regmap, int level1_v, int level2_v)
{
	level1_v = clamp_val(level1_v - 5, 0, 15);
	axp515_set_lowsocth(regmap, level1_v);

	if (level2_v > -1) {
		level2_v = clamp_val(level2_v, 0, 15);
		regmap_update_bits(regmap, AXP515_WARNING_LEVEL, GENMASK(3, 0),
				 level2_v);
	}

	return 0;
}

static int _axp515_set_ichg(struct regmap *regmap, int mA)
{
	mA = mA / 64;
	if (mA > 0x3f)
		mA = 0x3f;
	/* bit 5:0 is the ctrl bit */
	regmap_update_bits(regmap, AXP515_ICC_CFG, GENMASK(5, 0), mA);

	return 0;
}

static int axp515_set_ichg(struct axp515_bat_power *bat_power, int mA)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	int limit_status = 0, jeita_status = 0, ntc_status = 0, chg_status = 1;
	int cur_now, cur_set = mA;
	int jeita_cur, thermal_limit, runtime_cur;
	unsigned int data;
	bool recharge_flag = false;

	jeita_status = atomic_read(&bat_power->pmu_jeita_status);
	limit_status = atomic_read(&bat_power->pmu_limit_status);
	ntc_status = atomic_read(&bat_power->pmu_ntc_status);
	chg_status = atomic_read(&bat_power->pmu_chg_status_sets);

	cur_now = _axp515_get_ichg_lim(regmap);
	thermal_limit = axp_config->pmu_bat_charge_control_lim;
	runtime_cur = axp_config->pmu_runtime_chgcur;

	/* set discharge */
	if ((cur_set > 0) && (limit_status != PAUSE_CHARGING_STATE) && (!ntc_status) && (chg_status)) {
		regmap_read(regmap, AXP515_IPRECHG_CFG, &data);
		if (!(data & BIT(7)))
			recharge_flag = true;
	} else {
		regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), 0);
		return 0;
	}

	/* thermal_limit check */
	if ((limit_status == LIMIT_CUR_STATE) && (cur_set > thermal_limit)) {
		cur_set = thermal_limit;
	}

	/* jeita status check */
	switch (jeita_status) {
	case AW_TEMP_STATUS_WARM:
		jeita_cur = axp_config->pmu_jwarm_ifall;
		break;
	case AW_TEMP_STATUS_COOL:
		jeita_cur = axp_config->pmu_jcool_ifall;
		break;
	default:
		jeita_cur = runtime_cur;
		break;
	}

	if (jeita_cur != runtime_cur) {
		if (cur_set != jeita_cur) {
			cur_set = (cur_set * jeita_cur) / runtime_cur;
		} else {
			cur_set = jeita_cur;
		}
		if (limit_status == LIMIT_CUR_STATE) {
			thermal_limit = (thermal_limit * jeita_cur) / runtime_cur;
			if (cur_set > thermal_limit)
				cur_set = thermal_limit;
		}
	}

	/* if cur_set is 0, set discharge */
	if (!cur_set) {
		regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), 0);
		return 0;
	} else {
		if (recharge_flag) {
			regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), BIT(7));
			recharge_flag = false;
		}
	}

	/* not set same cur */
	if (cur_now == cur_set)
		return 0;

	_axp515_set_ichg(regmap, cur_set);

	return 0;
}

static int axp515_set_rechg_vol(struct regmap *regmap, int mV)
{
	unsigned int val;

	switch (mV) {
	case 100:
		val = 0;
		break;
	case 200:
		val = 1;
		break;
	default:
		val = 0;
		break;
	}
	regmap_update_bits(regmap, AXP515_CHARGE_CONTROL2, BIT(0), val);

	return 0;
}

static int axp515_set_bat_max_voltage(struct regmap *regmap, int mV)
{
	unsigned int val;

	mV = clamp_val(mV, 3840, 4608);
	val = (mV - 3840) / 16;
	val <<= 2;

	regmap_update_bits(regmap, AXP515_CHARGE_CONTROL2, GENMASK(7, 2), val);

	return 0;
}

static int axp515_set_status(struct regmap *regmap, int status)
{
	if (!status) {
		PMIC_INFO("disable charge\n");
		regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), 0);
	} else {
		PMIC_INFO("enable charge\n");
		regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), BIT(7));
	}

	return 0;
}

/**
 * axp515_get_param - get battery config from dts
 *
 * is not get battery config parameter from dts,
 * then it use the default config.
 */

static int axp515_model_update(struct axp515_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	unsigned char ocv_cap[AXP515_MAX_PARAM];

	/* down load battery parameters */
	ocv_cap[0]  = axp_config->pmu_bat_para1;
	ocv_cap[1]  = axp_config->pmu_bat_para2;
	ocv_cap[2]  = axp_config->pmu_bat_para3;
	ocv_cap[3]  = axp_config->pmu_bat_para4;
	ocv_cap[4]  = axp_config->pmu_bat_para5;
	ocv_cap[5]  = axp_config->pmu_bat_para6;
	ocv_cap[6]  = axp_config->pmu_bat_para7;
	ocv_cap[7]  = axp_config->pmu_bat_para8;
	ocv_cap[8]  = axp_config->pmu_bat_para9;
	ocv_cap[9]  = axp_config->pmu_bat_para10;
	ocv_cap[10] = axp_config->pmu_bat_para11;
	ocv_cap[11] = axp_config->pmu_bat_para12;
	ocv_cap[12] = axp_config->pmu_bat_para13;
	ocv_cap[13] = axp_config->pmu_bat_para14;
	ocv_cap[14] = axp_config->pmu_bat_para15;
	ocv_cap[15] = axp_config->pmu_bat_para16;
	ocv_cap[16] = axp_config->pmu_bat_para17;
	ocv_cap[17] = axp_config->pmu_bat_para18;
	ocv_cap[18] = axp_config->pmu_bat_para19;
	ocv_cap[19] = axp_config->pmu_bat_para20;
	ocv_cap[20] = axp_config->pmu_bat_para21;
	ocv_cap[21] = axp_config->pmu_bat_para22;
	ocv_cap[22] = axp_config->pmu_bat_para23;
	ocv_cap[23] = axp_config->pmu_bat_para24;
	ocv_cap[24] = axp_config->pmu_bat_para25;
	ocv_cap[25] = axp_config->pmu_bat_para26;
	ocv_cap[26] = axp_config->pmu_bat_para27;
	ocv_cap[27] = axp_config->pmu_bat_para28;
	ocv_cap[28] = axp_config->pmu_bat_para29;
	ocv_cap[29] = axp_config->pmu_bat_para30;
	ocv_cap[30] = axp_config->pmu_bat_para31;
	ocv_cap[31] = axp_config->pmu_bat_para32;
	regmap_bulk_write(regmap, AXP515_OCV_CURV0, ocv_cap, 32);
	regmap_update_bits(regmap, AXP515_BATFET_DLY, BIT(3), BIT(3));

	return 0;
}

static bool axp515_model_update_check(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP515_BATFET_DLY, &data);
	if (ret < 0)
		goto CHECK_ERR;

	if ((data & BIT(3)) == 0)
		goto CHECK_ERR;

	return true;

CHECK_ERR:
	return false;
}

static int axp515_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;

	struct axp515_bat_power *bat_power = power_supply_get_drvdata(psy);

	mutex_lock(&bat_power->lock);
	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = axp515_bat_get_max_voltage(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY_LEVEL: // customer modify
		ret = axp515_get_bat_present(bat_power);
		if (ret) {
			ret = axp515_get_soc(psy, val);
			if (ret < 0) {
				mutex_unlock(&bat_power->lock);
				return ret;

			}

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
		ret = axp515_get_bat_status(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = axp515_get_bat_present(bat_power);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp515_get_vbat_vol(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = axp515_get_soc(psy, val); // unit %;
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp515_get_lowsocth(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp515_get_temp(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		ret = axp515_get_ichg(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp515_get_ichg_lim(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		val->intval = atomic_read(&bat_power->pmu_limit_status);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX:
		val->intval = 2;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = axp515_get_bat_health(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		val->intval = bat_power->bat_shutdown_ltf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MAX:
		val->intval = bat_power->bat_shutdown_htf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN:
		val->intval = bat_power->bat_charge_ltf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = -200000;
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX:
		val->intval = bat_power->bat_charge_htf;
		if (!bat_power->dts_info.pmu_bat_temp_enable)
			val->intval = 200000;
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP515_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_CHARGE_COUNTER:
		ret = axp515_get_charger_count_current(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = axp515_get_charger_count(psy, val);
		if (ret < 0) {
			mutex_unlock(&bat_power->lock);
			return ret;
		}
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
		mutex_unlock(&bat_power->lock);
		return -EINVAL;
	}
	mutex_unlock(&bat_power->lock);
	return ret;
}

static int axp515_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp515_bat_power *bat_power = power_supply_get_drvdata(psy);
	struct regmap *regmap = bat_power->regmap;
	int ret = 0, lim_cur = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = axp515_set_lowsocth(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = _axp515_set_ichg(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = axp515_set_bat_max_voltage(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
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
		ret = axp515_set_ichg(bat_power, lim_cur);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		atomic_set(&bat_power->pmu_chg_status_sets, val->intval);
		ret = axp515_set_status(regmap, val->intval);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		atomic_set(&bat_power->charge_cycle_count, val->intval);
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
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = 1;
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		ret = 1;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;

}

static const struct power_supply_desc axp515_bat_desc = {
	.name = "axp515-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp515_bat_get_property,
	.set_property = axp515_bat_set_property,
	.properties = axp515_bat_props,
	.num_properties = ARRAY_SIZE(axp515_bat_props),
	.property_is_writeable = axp515_usb_power_property_is_writeable,
};

static void axp515_bat_temp_check(struct axp515_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	int ntc_status = atomic_read(&bat_power->pmu_ntc_status);
	int jeita_status = atomic_read(&bat_power->pmu_jeita_status);
	int set_cur = axp_config->pmu_runtime_chgcur;
	int bat_temp = _axp515_get_temp(bat_power);

	if (!axp_config->pmu_bat_temp_enable) {
		return;
	}

	/* ntc process */
	switch (ntc_status) {
	case AW_TEMP_STATUS_HOT:
		PMIC_DEBUG("bat temp(%d) is over than chage mode temp(%d): discharge\n", bat_temp, bat_power->bat_charge_htf);
		__pm_stay_awake(bat_power->bat_temp_ws);
		break;
	case AW_TEMP_STATUS_COLD:
		PMIC_DEBUG("bat temp(%d) is under than chage mode temp(%d): discharge\n", bat_temp, bat_power->bat_charge_ltf);
		__pm_stay_awake(bat_power->bat_temp_ws);
		break;
	case AW_TEMP_STATUS_GOOD:
		__pm_relax(bat_power->bat_temp_ws);
		PMIC_DEBUG("bat temp(%d) is recover: recharge\n", bat_temp);
		break;
	default:
		PMIC_ERR("error temp status:ntc_status:%d\n", ntc_status);
		break;
	}

	/* jeita process */
	if (bat_power->dts_info.pmu_jetia_en) {
		switch (jeita_status) {
		case AW_TEMP_STATUS_WARM:
			PMIC_DEBUG("bat temp(%d) is over than chage mode temp(%d):limit chgcur\n", bat_temp, bat_power->bat_jeita_warm);
			set_cur = axp_config->pmu_jwarm_ifall;
			break;
		case AW_TEMP_STATUS_COOL:
			PMIC_DEBUG("bat temp(%d) is under than chage mode temp(%d):limit chgcur\n", bat_temp, bat_power->bat_jeita_cool);
			set_cur = axp_config->pmu_jcool_ifall;
			break;
		case AW_TEMP_STATUS_GOOD:
			PMIC_DEBUG("bat temp(%d) is recover: restore bat chgcur\n", bat_temp);
			set_cur = axp_config->pmu_runtime_chgcur;
			break;
		default:
			PMIC_ERR("error temp status:jeita_status:%d\n", jeita_status);
			break;
		}
	}

	axp515_set_ichg(bat_power, set_cur);
	power_supply_changed(bat_power->bat_supply);

	PMIC_DEBUG("%s:%d ntc_status:%d jeita_status:%d\n", __func__, __LINE__, ntc_status, jeita_status);
}

static int axp515_battery_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	int ret = 0, temp_status;
	struct axp515_bat_power *bat_power = container_of(nb, struct axp515_bat_power, bat_nb);
	struct axp_config_info *axp_config = &bat_power->dts_info;

	PMIC_DEBUG("notifier event %lu\n", event);

	switch (event) {
	case AW_PSY_EVENT_BAT_TEMP_CHECK:
		if (!axp_config->pmu_jetia_en)
			return NOTIFY_DONE;
		temp_status = *(int *)data;
		atomic_set(&bat_power->pmu_jeita_status, temp_status);
		axp515_bat_temp_check(bat_power);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return NOTIFY_DONE;
}

static int axp515_battery_notifier_init(struct axp515_bat_power *bat_power)
{
	int ret = 0;

	bat_power->bat_nb.notifier_call = axp515_battery_notifier;
	sunxi_power_supply_reg_notifier(&bat_power->bat_nb);
	if (ret < 0) {
		PMIC_ERR("failed to register notifier :%d\n", ret);
		return ret;
	}

	return ret;
}

static void axp515_ntc_temp_init(struct axp515_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	int bat_temp = _axp515_get_temp(bat_power);

	/* init bat temp check */
	if (bat_power->bat_charge_htf < bat_temp) {
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_HOT);
	} else if (bat_temp < bat_power->bat_charge_ltf) {
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_COLD);
	} else {
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_GOOD);
		if (axp_config->pmu_jetia_en) {
			if (bat_power->bat_jeita_warm < bat_temp)
				atomic_set(&bat_power->pmu_jeita_status, AW_TEMP_STATUS_WARM);
			else if (bat_temp < bat_power->bat_jeita_cool)
				atomic_set(&bat_power->pmu_jeita_status, AW_TEMP_STATUS_COOL);
			else
				atomic_set(&bat_power->pmu_jeita_status, AW_TEMP_STATUS_GOOD);
		}
	}

	axp515_bat_temp_check(bat_power);
}
static void axp515_ntc_func_init(struct axp515_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	int val;

	/* default sets */
	regmap_update_bits(bat_power->regmap, AXP515_TS_PIN_CONTROL, AXP515_TS_ENABLE_MARK, AXP515_TS_ENABLE_MARK);
	atomic_set(&bat_power->pmu_ntc_status, 0);
	atomic_set(&bat_power->pmu_jeita_status, 0);
	atomic_set(&bat_power->pmu_limit_status, 0);

	if (!axp_config->pmu_bat_temp_enable) {
		axp_config->pmu_jetia_en = 0;
		axp_config->wakeup_untemp_chg = false;
		axp_config->wakeup_ovtemp_chg = false;
		return;
	}

	/* set ntc curr */
	regmap_read(bat_power->regmap, AXP515_TS_PIN_CONTROL, &val);
	val &= 0xF9;
	if (axp_config->pmu_bat_ts_current < 40)
		val |= 0x00 << 1;
	else if (axp_config->pmu_bat_ts_current < 60)
		val |= 0x01 << 1;
	else if (axp_config->pmu_bat_ts_current < 80)
		val |= 0x02 << 1;
	else
		val |= 0x03 << 1;
	regmap_write(bat_power->regmap, AXP515_TS_PIN_CONTROL, val);

	/* set ntc vol not over jeita vol*/
	if (axp_config->pmu_jetia_en) {
		if (axp_config->pmu_bat_charge_ltf) {
			if (axp_config->pmu_bat_charge_ltf < axp_config->pmu_jetia_cool)
				axp_config->pmu_bat_charge_ltf = axp_config->pmu_jetia_cool;
		}

		if (axp_config->pmu_bat_charge_htf) {
			if (axp_config->pmu_bat_charge_htf > axp_config->pmu_jetia_warm)
				axp_config->pmu_bat_charge_htf = axp_config->pmu_jetia_warm;
		}
	}

	/* set work vol not over ntc vol*/
	if (axp_config->pmu_bat_shutdown_ltf) {
		if (axp_config->pmu_bat_shutdown_ltf < axp_config->pmu_bat_charge_ltf)
			axp_config->pmu_bat_shutdown_ltf = axp_config->pmu_bat_charge_ltf;
	}
	if (axp_config->pmu_bat_shutdown_htf) {
		if (axp_config->pmu_bat_shutdown_htf > axp_config->pmu_bat_charge_htf)
			axp_config->pmu_bat_shutdown_htf = axp_config->pmu_bat_charge_htf;
	}

	/* set ntc irq */
	if (axp_config->pmu_bat_charge_ltf) {
		if (axp_config->pmu_bat_charge_ltf < axp_config->pmu_bat_charge_htf)
			axp_config->pmu_bat_charge_ltf = axp_config->pmu_bat_charge_htf;

		val = axp_config->pmu_bat_charge_ltf * 10 / 128;
		regmap_write(bat_power->regmap, AXP515_VLTF_CHARGE, val);
		regmap_write(bat_power->regmap, AXP515_VLTF_WORK, val);
	}

	if (axp_config->pmu_bat_charge_htf) {
		if (axp_config->pmu_bat_charge_htf > 510)
			axp_config->pmu_bat_charge_htf = 510;

		val = axp_config->pmu_bat_charge_htf * 10 / 128;
		regmap_write(bat_power->regmap, AXP515_VHTF_CHARGE, val);
		regmap_write(bat_power->regmap, AXP515_VHTF_WORK, val);
	}

	/* change fault voltage to fault temperature */
	bat_power->bat_charge_ltf = axp_vts_to_temp(axp_config->pmu_bat_charge_ltf, axp_config);
	bat_power->bat_charge_htf = axp_vts_to_temp(axp_config->pmu_bat_charge_htf, axp_config);
	bat_power->bat_shutdown_ltf = axp_vts_to_temp(axp_config->pmu_bat_shutdown_ltf, axp_config);
	bat_power->bat_shutdown_htf = axp_vts_to_temp(axp_config->pmu_bat_shutdown_htf, axp_config);

	if (!axp_config->pmu_jetia_en)
		return;

	/* change fault voltage to fault temperature */
	bat_power->bat_jeita_cool = axp_vts_to_temp(axp_config->pmu_jetia_cool, axp_config);
	bat_power->bat_jeita_warm = axp_vts_to_temp(axp_config->pmu_jetia_warm, axp_config);

	switch (axp_config->pmu_jcool_ifall) {
	case 0:
		axp_config->pmu_jcool_ifall = axp_config->pmu_runtime_chgcur;
		break;
	case 1:
		axp_config->pmu_jcool_ifall = axp_config->pmu_runtime_chgcur / 2;
		break;
	case 2:
		axp_config->pmu_jcool_ifall = axp_config->pmu_runtime_chgcur * 3 / 4;
		break;
	case 3:
		axp_config->pmu_jcool_ifall = 0;
		break;
	default:
		break;
	}
	axp_config->pmu_jcool_ifall = DIVIDE_BY_64(axp_config->pmu_jcool_ifall);

	switch (axp_config->pmu_jwarm_ifall) {
	case 0:
		axp_config->pmu_jwarm_ifall = axp_config->pmu_runtime_chgcur;
		break;
	case 1:
		axp_config->pmu_jwarm_ifall = axp_config->pmu_runtime_chgcur / 2;
		break;
	case 2:
		axp_config->pmu_jwarm_ifall = axp_config->pmu_runtime_chgcur * 3 / 4;
		break;
	case 3:
		axp_config->pmu_jwarm_ifall = 0;
		break;
	default:
		break;
	}
	axp_config->pmu_jwarm_ifall = DIVIDE_BY_64(axp_config->pmu_jwarm_ifall);
}

static int axp515_init_chip(struct axp515_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	union power_supply_propval propval;
	unsigned int battery_exist = 1;
	int update_min_times[8] = {30, 60, 120, 164, 0, 5, 10, 20};
	int ocv_cou_adjust_time[4] = {60, 120, 15, 30};
	int cur_coulomb_counter, rdc;
	int val, i, ret = 0;

	if (bat_power == NULL)
		return -ENODEV;

	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp515 reg update, i2c communication err!\n");
		return ret;
	}

	/* init chg status*/
	atomic_set(&bat_power->pmu_chg_status_sets, 1);

	/* init bat cycle*/
	atomic_set(&bat_power->charge_cycle_count, 0);
	axp_config->pmu_bat_cycle_cap_reduce *= axp_config->pmu_battery_cap * 10 / axp_config->pmu_bat_cycle_life;

	/* get battery exist*/
	battery_exist = axp515_get_bat_present(bat_power);

	if (!battery_exist)
		regmap_update_bits(bat_power->regmap, AXP515_TIMER2_SET, BIT(3), BIT(3));
	else
		regmap_update_bits(bat_power->regmap, AXP515_TIMER2_SET, BIT(3), 0);

	/* update battery model*/
	if (battery_exist && !axp515_model_update_check(bat_power->regmap)) {
		PMIC_DEV_ERR(bat_power->dev, "axp515 model need update!\n");
		ret = axp515_model_update(bat_power);
		if (ret < 0) {
			PMIC_DEV_ERR(bat_power->dev, "axp515 model update fail!\n");
			return ret;
		}
	}
	PMIC_DEV_DEBUG(bat_power->dev, "axp515 model update ok:battery_exist:%d\n", battery_exist);
	/*end of update battery model*/

	/* set pre-charge current to 128mA*/
	val = 0x02 << 1;
	regmap_update_bits(bat_power->regmap, AXP515_IPRECHG_CFG, GENMASK(4, 1), val);

	/* set terminal charge current */
	if (axp_config->pmu_terminal_chgcur < 64)
		val = 0x00;
	else if (axp_config->pmu_terminal_chgcur > 1024)
		val = 0x0f;
	else
		val = axp_config->pmu_terminal_chgcur / 64 - 1;
	val = val << 3;
	regmap_update_bits(bat_power->regmap, AXP515_ITERM_CFG, GENMASK(6, 3), val);

	/* set chg time */
	if (axp_config->pmu_init_chg_pretime) {
		axp_config->pmu_init_chg_pretime = clamp_val(axp_config->pmu_init_chg_pretime, 40, 70);
		val = (axp_config->pmu_init_chg_pretime - 40) / 10;
		val = 0x80 + (val << 5);
		regmap_update_bits(bat_power->regmap, AXP515_TIMER2_SET, 0xe0, val);
	} else {
		regmap_update_bits(bat_power->regmap, AXP515_TIMER2_SET, BIT(7), 0);
	}

	if (axp_config->pmu_init_chg_csttime) {
		if (axp_config->pmu_init_chg_csttime <= 60 * 5)
			val = 0;
		else if (axp_config->pmu_init_chg_csttime <= 60 * 8)
			val = 1;
		else if (axp_config->pmu_init_chg_csttime <= 60 * 12)
			val = 2;
		else if (axp_config->pmu_init_chg_csttime <= 60 * 20)
			val = 3;
		else
			val = 3;
		val = (val << 1) + 0x01;
		regmap_update_bits(bat_power->regmap, AXP515_ITERM_CFG, 0x07, val);
	} else {
		regmap_update_bits(bat_power->regmap, AXP515_ITERM_CFG, BIT(0), 0);
	}

	if (!battery_exist)
		axp_config->pmu_bat_temp_enable = 0;

	/* adc set */
	val = AXP515_ADC_BATVOL_ENABLE | AXP515_ADC_BATCUR_ENABLE | AXP515_ADC_TSVOL_ENABLE;
	regmap_update_bits(bat_power->regmap, AXP515_ADC_CONTROL, AXP515_MASK_ADC_STAT, val);

	regmap_read(bat_power->regmap, AXP515_TS_PIN_CONTROL, &val);
	switch (axp_config->pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(3 << 5);
		break;
	case 2:
		val &= ~(3 << 5);
		val |= 1 << 5;
		break;
	case 4:
		val &= ~(3 << 5);
		val |= 2 << 5;
		break;
	case 8:
		val &= ~(3 << 5);
		val |= 3 << 5;
		break;
	default:
		break;
	}
	regmap_write(bat_power->regmap, AXP515_TS_PIN_CONTROL, val);

	/* set CHGLED */
	if (axp_config->pmu_chgled_func) {
		regmap_update_bits(bat_power->regmap, AXP515_CHGLED_CFG, BIT(7), 0);
		regmap_update_bits(bat_power->regmap, AXP515_CHGLED_CFG, GENMASK(2, 0), axp_config->pmu_chgled_type);
		if (axp_config->ocv_coulumb_100 == 1) {
			axp515_get_soc(bat_power->bat_supply, &propval);
		}
	} else {
		regmap_update_bits(bat_power->regmap, AXP515_CHGLED_CFG, BIT(7), BIT(7));
	}

	/* set charger voltage limit */
	if (axp_config->pmu_init_chgvol == 4100)
		axp_config->pmu_init_chgvol = 4112;
	else if (axp_config->pmu_init_chgvol == 4200)
		axp_config->pmu_init_chgvol = 4208;
	else if (axp_config->pmu_init_chgvol == 4350)
		axp_config->pmu_init_chgvol = 4352;

	axp_config->pmu_init_chgvol = clamp_val(axp_config->pmu_init_chgvol, 3840, 4608);
	axp515_set_bat_max_voltage(bat_power->regmap, axp_config->pmu_init_chgvol);
	axp515_set_rechg_vol(bat_power->regmap, axp_config->pmu_rechg_vol);

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
	ret = _axp515_set_ichg(bat_power->regmap, axp_config->pmu_runtime_chgcur);

	/* set gauge_thld */
	axp515_set_gauge_thld(bat_power->regmap, axp_config->pmu_battery_warning_level1, axp_config->pmu_battery_warning_level2);

	/* legacy function */
	/* init battery capacity correct function */
	if (axp_config->pmu_batt_cap_correct)
		regmap_update_bits(bat_power->regmap, AXP515_COULOMB_CTL, BIT(5), BIT(5));
	else
		regmap_update_bits(bat_power->regmap, AXP515_COULOMB_CTL, BIT(5), 0);

	/* RDC initial */
	regmap_read(bat_power->regmap, AXP515_RDC0, &val);
	if ((axp_config->pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (axp_config->pmu_battery_rdc * 10000 + 5371) / 10742;
		regmap_update_bits(bat_power->regmap, AXP515_RDC0, GENMASK(4, 0), ((rdc >> 8)& 0x1F));
		regmap_write(bat_power->regmap, AXP515_RDC1, rdc & 0x00FF);
	}

	regmap_read(bat_power->regmap, AXP515_BATCAP0, &val);
	if ((axp_config->pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = axp_config->pmu_battery_cap
					* 1000 / 1456;
		regmap_write(bat_power->regmap, AXP515_BATCAP0,
					((cur_coulomb_counter >> 8) | 0x80));
		regmap_write(bat_power->regmap, AXP515_BATCAP1,
					cur_coulomb_counter & 0x00FF);
	} else if (!axp_config->pmu_battery_cap) {
		regmap_write(bat_power->regmap, AXP515_BATCAP0, 0x00);
		regmap_write(bat_power->regmap, AXP515_BATCAP1, 0x00);
	}

	/*enable fast charge */
	regmap_update_bits(bat_power->regmap, AXP515_CC_GLOBAL_CTRL, 0x04, 0x04);

	/* avoid the timer counter error*/
	regmap_update_bits(bat_power->regmap, AXP515_TIMER2_SET, 0x10, 0x0);
	for (i = 0; i < ARRAY_SIZE(update_min_times); i++) {
		if (update_min_times[i] == axp_config->pmu_update_min_time)
			break;
	}
	regmap_update_bits(bat_power->regmap, AXP515_ADJUST_PARA, 0x7, i);
	/*initial the ocv_cou_adjust_time*/
	for (i = 0; i < ARRAY_SIZE(ocv_cou_adjust_time); i++) {
		if (ocv_cou_adjust_time[i] == axp_config->pmu_ocv_cou_adjust_time)
			break;
	}
	i <<= 6;
	regmap_update_bits(bat_power->regmap, AXP515_ADJUST_PARA1, 0xc0, i);

	/* init ntc function */
	axp515_ntc_func_init(bat_power);
	/*init power supply notifier*/
	axp515_battery_notifier_init(bat_power);
	return ret;
}

static irqreturn_t axp515_irq_handler_thread(int irq, void *data)
{
	struct axp515_bat_power *bat_power = data;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	return IRQ_HANDLED;
}

static irqreturn_t axp515_irq_handler_bat_temp_change(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	struct axp515_bat_power *bat_power = data;
	struct axp_config_info *axp_config = &bat_power->dts_info;

	if (!axp_config->pmu_bat_temp_enable)
		return IRQ_HANDLED;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	switch (id->irq_data.hwirq) {
	case AXP515_IRQ_BCOT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_HOT);
		PMIC_DEBUG("interrupt:battery over temp charge");
		break;
	case AXP515_IRQ_BCUT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_COLD);
		PMIC_DEBUG("interrupt:battery under temp charge");
		break;
	case AXP515_IRQ_QBCOT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_GOOD);
		PMIC_DEBUG("interrupt:battery quit over temp charge");
		break;
	case AXP515_IRQ_QBCUT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_GOOD);
		PMIC_DEBUG("interrupt:battery quit under temp charge");
		break;
	case AXP515_IRQ_BWOT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_HOT);
		PMIC_DEBUG("interrupt:battery over temp work");
		break;
	case AXP515_IRQ_BWUT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_COLD);
		PMIC_DEBUG("interrupt:battery under temp work");
		break;
	case AXP515_IRQ_QBWOT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_GOOD);
		PMIC_DEBUG("interrupt:battery quit over temp work");
		break;
	case AXP515_IRQ_QBWUT:
		atomic_set(&bat_power->pmu_ntc_status, AW_TEMP_STATUS_GOOD);
		PMIC_DEBUG("interrupt:battery quit under temp work");
		break;
	default:
		PMIC_ERR("interrupt:error status");
		break;
	}

	axp515_bat_temp_check(bat_power);

	return IRQ_HANDLED;
}

enum axp515_bat_virq_index {
	AXP515_VIRQ_BAT_IN,
	AXP515_VIRQ_BAT_OUT,
	/* charge irq */
	AXP515_VIRQ_CHARGING,
	AXP515_VIRQ_CHARGE_OVER,
	/* battery capacity irq */
	AXP515_VIRQ_LOW_WARNING1,
	AXP515_VIRQ_LOW_WARNING2,
	AXP515_VIRQ_BAT_NEW_SOC,
	/* battery temperature irq */
	AXP515_VIRQ_BAT_UNTEMP_CHG,
	AXP515_VIRQ_BAT_OVTEMP_CHG,
	AXP515_VIRQ_QUIT_BAT_UNTEMP_CHG,
	AXP515_VIRQ_QUIT_BAT_OVTEMP_CHG,
	AXP515_VIRQ_BAT_UNTEMP_WORK,
	AXP515_VIRQ_BAT_OVTEMP_WORK,
	AXP515_VIRQ_QUIT_BAT_UNTEMP_WORK,
	AXP515_VIRQ_QUIT_BAT_OVTEMP_WORK,

	AXP515_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_bat_irq[] = {
	[AXP515_VIRQ_BAT_IN] = { "bat in", axp515_irq_handler_thread },
	[AXP515_VIRQ_BAT_OUT] = { "bat out", axp515_irq_handler_thread },
	[AXP515_VIRQ_CHARGING] = { "charging", axp515_irq_handler_thread },
	[AXP515_VIRQ_CHARGE_OVER] = { "charge over",
						axp515_irq_handler_thread },
	[AXP515_VIRQ_LOW_WARNING1] = { "low warning1",
						axp515_irq_handler_thread },
	[AXP515_VIRQ_LOW_WARNING2] = { "low warning2",
						axp515_irq_handler_thread },
	[AXP515_VIRQ_BAT_UNTEMP_CHG] = { "bat untemp chg",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_BAT_OVTEMP_CHG] = { "bat ovtemp chg",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_QUIT_BAT_UNTEMP_CHG] = { "quit bat untemp chg",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_QUIT_BAT_OVTEMP_CHG] = { "quit bat ovtemp chg",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_BAT_UNTEMP_WORK] = { "bat untemp work",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_BAT_OVTEMP_WORK] = { "bat ovtemp work",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_QUIT_BAT_UNTEMP_WORK] = { "quit bat untemp work",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_QUIT_BAT_OVTEMP_WORK] = { "quit bat ovtemp work",
						axp515_irq_handler_bat_temp_change },
	[AXP515_VIRQ_BAT_NEW_SOC] = { "gauge_new_soc",
					  axp515_irq_handler_thread },
};

static int axp515_bat_dt_parse(struct device_node *node,
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
	AXP_OF_PROP_READ(pmu_bat_charge_control_lim,      600);
	AXP_OF_PROP_READ(pmu_init_chgvol,                4200);
	AXP_OF_PROP_READ(pmu_rechg_vol,                   100);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_chgled_func,                   1);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);
	AXP_OF_PROP_READ(pmu_batdeten,                      1);

	AXP_OF_PROP_READ(pmu_bat_ts_current,               60);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             1312);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,              176);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           1984);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,            152);

	AXP_OF_PROP_READ(pmu_jetia_en,                      0);
	AXP_OF_PROP_READ(pmu_jetia_cool,                  880);
	AXP_OF_PROP_READ(pmu_jetia_warm,                  240);
	AXP_OF_PROP_READ(pmu_jcool_ifall,                   0);
	AXP_OF_PROP_READ(pmu_jwarm_ifall,                   0);

	AXP_OF_PROP_READ(pmu_bat_temp_comp,                11);

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

	AXP_OF_PROP_READ(pmu_bat_cycle_life,              800);
	AXP_OF_PROP_READ(pmu_bat_cycle_cap_reduce,         20);

	AXP_OF_PROP_READ(pmu_bat_manufacture_year,       2024);
	AXP_OF_PROP_READ(pmu_bat_manufacture_month,         1);
	AXP_OF_PROP_READ(pmu_bat_manufacture_day,           1);

	/* old guage */
	AXP_OF_PROP_READ(pmu_bat_para1,               OCVREG0);
	AXP_OF_PROP_READ(pmu_bat_para2,               OCVREG1);
	AXP_OF_PROP_READ(pmu_bat_para3,               OCVREG2);
	AXP_OF_PROP_READ(pmu_bat_para4,               OCVREG3);
	AXP_OF_PROP_READ(pmu_bat_para5,               OCVREG4);
	AXP_OF_PROP_READ(pmu_bat_para6,               OCVREG5);
	AXP_OF_PROP_READ(pmu_bat_para7,               OCVREG6);
	AXP_OF_PROP_READ(pmu_bat_para8,               OCVREG7);
	AXP_OF_PROP_READ(pmu_bat_para9,               OCVREG8);
	AXP_OF_PROP_READ(pmu_bat_para10,              OCVREG9);
	AXP_OF_PROP_READ(pmu_bat_para11,              OCVREGA);
	AXP_OF_PROP_READ(pmu_bat_para12,              OCVREGB);
	AXP_OF_PROP_READ(pmu_bat_para13,              OCVREGC);
	AXP_OF_PROP_READ(pmu_bat_para14,              OCVREGD);
	AXP_OF_PROP_READ(pmu_bat_para15,              OCVREGE);
	AXP_OF_PROP_READ(pmu_bat_para16,              OCVREGF);
	AXP_OF_PROP_READ(pmu_bat_para17,             OCVREG10);
	AXP_OF_PROP_READ(pmu_bat_para18,             OCVREG11);
	AXP_OF_PROP_READ(pmu_bat_para19,             OCVREG12);
	AXP_OF_PROP_READ(pmu_bat_para20,             OCVREG13);
	AXP_OF_PROP_READ(pmu_bat_para21,             OCVREG14);
	AXP_OF_PROP_READ(pmu_bat_para22,             OCVREG15);
	AXP_OF_PROP_READ(pmu_bat_para23,             OCVREG16);
	AXP_OF_PROP_READ(pmu_bat_para24,             OCVREG17);
	AXP_OF_PROP_READ(pmu_bat_para25,             OCVREG18);
	AXP_OF_PROP_READ(pmu_bat_para26,             OCVREG19);
	AXP_OF_PROP_READ(pmu_bat_para27,             OCVREG1A);
	AXP_OF_PROP_READ(pmu_bat_para28,             OCVREG1B);
	AXP_OF_PROP_READ(pmu_bat_para29,             OCVREG1C);
	AXP_OF_PROP_READ(pmu_bat_para30,             OCVREG1D);
	AXP_OF_PROP_READ(pmu_bat_para31,             OCVREG1E);
	AXP_OF_PROP_READ(pmu_bat_para32,             OCVREG1F);

	/* legacy function */
	AXP_OF_PROP_READ(pmu_bat_unused,                    0);
	AXP_OF_PROP_READ(pmu_terminal_chgcur,             256);
	AXP_OF_PROP_READ(pmu_init_chg_pretime,  INTCHGPRETIME);
	AXP_OF_PROP_READ(pmu_init_chg_csttime,  INTCHGCSTTIME);
	AXP_OF_PROP_READ(pmu_init_adc_freq,        INTADCFREQ);
	AXP_OF_PROP_READ(pmu_update_min_time,   UPDATEMINTIME);
	AXP_OF_PROP_READ(pmu_battery_rdc,              BATRDC);
	AXP_OF_PROP_READ(pmu_ocv_cou_adjust_time,          60);
	AXP_OF_PROP_READ(ocv_coulumb_100,                   1);

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
	axp_config->wakeup_untemp_chg =
		of_property_read_bool(node, "wakeup_bat_untemp_chg");
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");

	axp_config->wakeup_new_soc =
		of_property_read_bool(node, "wakeup_new_soc");

	return 0;
}

static void axp515_bat_parse_device_tree(struct axp515_bat_power *bat_power)
{
	int ret;
	struct axp_config_info *axp_config;

	if (!bat_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	axp_config = &bat_power->dts_info;
	ret = axp515_bat_dt_parse(bat_power->dev->of_node, axp_config);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}
}

static void axp515_bat_power_monitor(struct work_struct *work)
{
	struct axp515_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);

	power_supply_changed(bat_power->bat_supply);

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

static void axp515_temp_process_init(struct work_struct *work)
{
	struct axp515_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_temp_init.work);
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *regmap = bat_power->regmap;
	unsigned int data;
	int vts1, vts2, charge_cur, temp_calib;

	regmap_read(regmap, AXP515_EXT_PARA5, &data);
	if (data) {
		schedule_delayed_work(&bat_power->bat_temp_init, msecs_to_jiffies(2 * 1000));
		return;
	}
	axp_config->pmu_bat_temp_enable = 0;

	regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), 0);
	mdelay(500);
	vts1 = axp515_get_adc_raw(regmap);
	regmap_update_bits(regmap, AXP515_IPRECHG_CFG, BIT(7), BIT(7));
	mdelay(500);
	vts2 = axp515_get_adc_raw(regmap);

	charge_cur = axp515_ichg(regmap);

	temp_calib = (vts2 - vts1) / charge_cur;
	bat_power->bat_temp_calib = clamp_val(temp_calib, axp_config->pmu_bat_temp_comp, axp_config->pmu_battery_rdc);
	axp_config->pmu_bat_temp_enable = 1;

	/* init bat temp check */
	axp515_ntc_temp_init(bat_power);
}

static int axp515_battery_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0, irq;

	struct axp515_bat_power *bat_power;
	struct power_supply_config psy_cfg = {};
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *node = pdev->dev.of_node;

	if (!of_device_is_available(node)) {
		PMIC_ERR("axp515-battery device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		PMIC_ERR("can not register axp515-battery without irq\n");
		return -EINVAL;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (bat_power == NULL) {
		PMIC_ERR("axp515_bat_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	bat_power->name = "axp515_battery";
	bat_power->dev = &pdev->dev;
	bat_power->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp515_bat_parse_device_tree(bat_power);

	mutex_init(&bat_power->lock);
	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
			&axp515_bat_desc, &psy_cfg);

	if (IS_ERR(bat_power->bat_supply)) {
		PMIC_ERR("axp515 failed to register bat power\n");
		ret = PTR_ERR(bat_power->bat_supply);
		return ret;
	}

	ret = axp515_init_chip(bat_power);
	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp515 init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}


	/* add thermal cooling */
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

	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp515_bat_power_monitor);
	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));

	if (bat_power->dts_info.pmu_bat_temp_enable) {
		bat_power->bat_temp_ws = wakeup_source_register(bat_power->dev, "bat_temp_proce");
		INIT_DELAYED_WORK(&bat_power->bat_temp_init, axp515_temp_process_init);
		schedule_delayed_work(&bat_power->bat_temp_init, 0);
	}

	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp515_battery_remove(struct platform_device *pdev)
{
	struct axp515_bat_power *bat_power = platform_get_drvdata(pdev);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP515 unegister==============\n");
	if (bat_power->bat_supply) {
		power_supply_unregister(bat_power->bat_supply);
		axp20x_unregister_cooler(bat_power->bat_supply);
		mutex_destroy(&bat_power->lock);
	}
	PMIC_DEV_DEBUG(&pdev->dev, "axp515 teardown battery dev\n");

	return 0;
}

static inline void axp515_bat_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp515_bat_virq_dts_set(struct axp515_bat_power *bat_power, bool enable)
{
	struct axp_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_BAT_IN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_BAT_OUT].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_CHARGING].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_CHARGE_OVER].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_LOW_WARNING1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_LOW_WARNING2].irq,
				enable);
	if (!dts_info->wakeup_untemp_chg) {
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_BAT_UNTEMP_CHG].irq,
			enable);
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_BAT_UNTEMP_WORK].irq,
			enable);
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_QUIT_BAT_UNTEMP_CHG].irq,
			enable);
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_QUIT_BAT_UNTEMP_WORK].irq,
			enable);
	}
	if (!dts_info->wakeup_ovtemp_chg) {
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_BAT_OVTEMP_CHG].irq,
			enable);
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_BAT_OVTEMP_WORK].irq,
			enable);
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_QUIT_BAT_OVTEMP_CHG].irq,
			enable);
		axp515_bat_irq_set(
			axp_bat_irq[AXP515_VIRQ_QUIT_BAT_OVTEMP_WORK].irq,
			enable);
	}
	if (!dts_info->wakeup_new_soc)
		axp515_bat_irq_set(axp_bat_irq[AXP515_VIRQ_BAT_NEW_SOC].irq,
				enable);

}

static void axp515_bat_shutdown(struct platform_device *pdev)
{
	struct axp515_bat_power *bat_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	_axp515_set_ichg(bat_power->regmap, bat_power->dts_info.pmu_shutdown_chgcur);

}

static int axp515_bat_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp515_bat_power *bat_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	atomic_set(&bat_power->pmu_limit_status, 0);
	axp515_set_ichg(bat_power, bat_power->dts_info.pmu_suspend_chgcur);
	axp515_bat_virq_dts_set(bat_power, false);

	return 0;
}

static int axp515_bat_resume(struct platform_device *pdev)
{
	struct axp515_bat_power *bat_power = platform_get_drvdata(pdev);

	power_supply_changed(bat_power->bat_supply);

	schedule_delayed_work(&bat_power->bat_supply_mon, 0);

	axp515_set_ichg(bat_power, bat_power->dts_info.pmu_runtime_chgcur);
	axp515_bat_virq_dts_set(bat_power, true);

	return 0;
}

static const struct of_device_id axp515_bat_power_match[] = {
	{
		.compatible = "x-powers,axp515-bat-power-supply",
		.data = (void *)AXP515_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp515_bat_power_match);

static struct platform_driver axp515_bat_power_driver = {
	.driver = {
		.name = "axp515-bat-power-supply",
		.of_match_table = axp515_bat_power_match,
	},
	.probe = axp515_battery_probe,
	.remove = axp515_battery_remove,
	.shutdown = axp515_bat_shutdown,
	.suspend = axp515_bat_suspend,
	.resume = axp515_bat_resume,
};

module_platform_driver(axp515_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp515 battery driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
