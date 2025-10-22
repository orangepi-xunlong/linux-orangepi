/* SPDX-License-Identifier: GPL-2.0-or-later */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "axp2601_battery.h"

#define DIVIDE_BY_50(n) (((n) / 50) * 50)

struct axp2601_bat_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *bat_supply;
	struct power_supply       *qc_psy;
	struct power_supply       *gpio_vbus_psy;
	struct delayed_work        bat_supply_mon;
	struct delayed_work        bat_power_curve;
	struct delayed_work        bat_power_full_curve;
	struct bmu_ext_config_info     dts_info;
	struct wakeup_source       *ws;
	void __iomem			   *writebase;

	int			charger_cooler_type;

	/* charge_limit_process */
	atomic_t		pmu_limit_status;
	/* charge_cycle_count */
	atomic_t	 	charge_cycle_count;
};

static enum power_supply_property axp2601_bat_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_ALERT_MAX,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN,
	POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT_MAX,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_MANUFACTURE_YEAR,
	POWER_SUPPLY_PROP_MANUFACTURE_MONTH,
	POWER_SUPPLY_PROP_MANUFACTURE_DAY,
};

static int axp2601_usb_power_get(struct axp2601_bat_power *bat_power)
{
	if (!IS_ERR_OR_NULL(bat_power->qc_psy)) {
		return 1;
	}

	return 0;
}

static int axp2601_gpio_vbus_power_get(struct axp2601_bat_power *bat_power)
{
	if (!IS_ERR_OR_NULL(bat_power->gpio_vbus_psy)) {
		return 1;
	}

	return 0;
}

static int axp2601_can_charge_status_get(struct axp2601_bat_power *bat_power)
{
	return axp2601_usb_power_get(bat_power) || axp2601_gpio_vbus_power_get(bat_power);
}

static int _axp2601_get_vbat_vol(struct axp2601_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	uint8_t data[2];
	uint16_t vtemp[3], tempv;
	int ret = 0, bat_vol;
	uint8_t i;

	for (i = 0; i < 3; i++) {
		ret = regmap_bulk_read(regmap, AXP2601_VBAT_H, data, 2);
		if (ret < 0)
			return ret;

		vtemp[i] = ((data[0] << 0x08) | (data[1]));
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

	/*incase vtemp[1] exceed AXP2601_VBAT_MAX */
	if ((vtemp[1] > AXP2601_VBAT_MAX) || (vtemp[1] < AXP2601_VBAT_MIN)) {
		bat_vol = 0;
	} else {
		bat_vol = vtemp[1] * 1000;
	}

	return bat_vol;
}

static int axp2601_get_vbat_vol(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);

	val->intval = _axp2601_get_vbat_vol(bat_power);

	return 0;
}

/* read temperature */
static int axp2601_get_temp(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP, val);
	} else {
		val->intval = 300;
	}

	return 0;
}

static int axp2601_get_bat_health(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_HEALTH, val);
	} else {
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
	}

	return 0;
}

static int _axp2601_get_bat_present(struct axp2601_bat_power *bat_power)
{
	int bat_vol;

	bat_vol = _axp2601_get_vbat_vol(bat_power);
	if (bat_vol > 3000)
		return 1;

	return 0;
}

static int axp2601_get_bat_present(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CALIBRATE, val);
	} else {
		val->intval = _axp2601_get_bat_present(bat_power);
	}

	return 0;
}

static int axp2601_get_soc(struct power_supply *ps,
			    union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct regmap *regmap = bat_power->regmap;
	struct power_supply       *vbus_status;
	unsigned int data;
	u32 reg_value;
	int ret = 0;

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CALIBRATE, val);
	} else {
		axp2601_get_bat_present(ps, val);
	}

	if (!val->intval) {
		val->intval = 0;
		return 0;
	}

	if (!(axp2601_can_charge_status_get(bat_power))) {
		writel(0, bat_power->writebase);
	} else {
		if (axp2601_usb_power_get(bat_power)) {
			vbus_status = bat_power->qc_psy;
		} else if (axp2601_gpio_vbus_power_get(bat_power)) {
			vbus_status = bat_power->gpio_vbus_psy;
		}
	}

	reg_value = readl(bat_power->writebase);

	if (reg_value & 0x80 || reg_value & 0x100) {
		data = reg_value & 0x7F;
		if (data == 101) {
			if (axp2601_can_charge_status_get(bat_power)) {
				power_supply_get_property(vbus_status, POWER_SUPPLY_PROP_STATUS, val);
				if (val->intval == POWER_SUPPLY_STATUS_DISCHARGING) {
					__pm_stay_awake(bat_power->ws);
					schedule_delayed_work(&bat_power->bat_power_full_curve, msecs_to_jiffies(60 * 1000));
				}
			}
		}
	} else {
		ret = regmap_read(regmap, AXP2601_GAUGE_SOC, &data);
		if (ret < 0)
			return ret;
		data &= 0x7F;

		if (axp2601_can_charge_status_get(bat_power)) {
			power_supply_get_property(vbus_status, POWER_SUPPLY_PROP_STATUS, val);
			if (val->intval == POWER_SUPPLY_STATUS_CHARGING || val->intval == POWER_SUPPLY_STATUS_FULL) {
				if (data > 94) {
					reg_value &= ~(0x1FF);
					if (data > 99) {
						reg_value |= 101;
						writel(reg_value, bat_power->writebase);
					} else {
						reg_value |= 0x100;
						reg_value |= data;
						writel(reg_value, bat_power->writebase);
						__pm_stay_awake(bat_power->ws);
						schedule_delayed_work(&bat_power->bat_power_full_curve, msecs_to_jiffies(60 * 1000));
					}
				}
			}
		}
	}

	if (data > AXP2601_SOC_MAX)
		data = AXP2601_SOC_MAX;
	else if (data < AXP2601_SOC_MIN)
		data = AXP2601_SOC_MIN;

	val->intval = data;

	return 0;
}

static int axp2601_get_bat_status(struct power_supply *ps,
				  union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, val);
	} else if (axp2601_gpio_vbus_power_get(bat_power)) {
		power_supply_get_property(bat_power->gpio_vbus_psy, POWER_SUPPLY_PROP_STATUS, val);
	} else {
		val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	if (val->intval == POWER_SUPPLY_STATUS_CHARGING || val->intval == POWER_SUPPLY_STATUS_FULL) {
		axp2601_get_soc(ps, val);
		if (val->intval == 100) {
			val->intval = POWER_SUPPLY_STATUS_FULL;
		} else {
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		}
	}

	return 0;
}

static int axp2601_get_ichg_lim(struct power_supply *ps,
			     union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, val);
	} else {
		val->intval = 0;
	}

	return 0;
}

static int axp2601_get_charger_count(struct power_supply *ps,
		union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(ps);
	struct bmu_ext_config_info *bmp_ext_config = &bat_power->dts_info;
	unsigned int data;
	int charge_cycle_count = 0, reduce_cap;

	charge_cycle_count = atomic_read(&bat_power->charge_cycle_count);

	data = bmp_ext_config->pmu_battery_cap * 1000;
	reduce_cap = charge_cycle_count * bmp_ext_config->pmu_bat_cycle_cap_reduce;
	data -= reduce_cap;

	val->intval = data;

	return 0;
}

static int axp2601_set_ichg(struct axp2601_bat_power *bat_power, int mA)
{
	union power_supply_propval qc_val;

	qc_val.intval = mA;
	if (axp2601_usb_power_get(bat_power)) {
		power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT, &qc_val);
	}
	return 0;
}

static int axp2601_set_status(struct axp2601_bat_power *bat_power, int status)
{
	union power_supply_propval qc_val;

	if (!status) {
		PMIC_INFO("disable charge\n");
		if (axp2601_usb_power_get(bat_power)) {
			qc_val.intval = 1;
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
		}
	} else {
		PMIC_INFO("enable charge\n");
		if (axp2601_usb_power_get(bat_power)) {
			qc_val.intval = 2;
			power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
		}
	}

	return 0;
}

static int _axp2601_reset_mcu(struct regmap *regmap)
{
	int ret = 0;

	ret = regmap_update_bits(regmap, AXP2601_RESET_CFG, AXP2601_MODE_RSTMCU, AXP2601_MODE_RSTMCU);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, AXP2601_RESET_CFG, AXP2601_MODE_RSTMCU,
				 0);
	if (ret < 0)
		return ret;

	return 0;
}

static int axp2601_reset_mcu(struct axp2601_bat_power *bat_power)
{
	int ret = 0;
	union power_supply_propval qc_val;
	struct regmap *regmap = bat_power->regmap;

	if (axp2601_usb_power_get(bat_power)) {
		qc_val.intval = 1;
		power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
	}
	msleep(500);

	ret = _axp2601_reset_mcu(regmap);

	msleep(500);
	if (axp2601_usb_power_get(bat_power)) {
		qc_val.intval = 2;
		power_supply_set_property(bat_power->qc_psy, POWER_SUPPLY_PROP_STATUS, &qc_val);
	}

	return ret;
}

/**
 * axp2601_get_param - get battery config from dts
 *
 * is not get battery config parameter from dts,
 * then it use the default config.
 */
static int axp2601_get_param(struct axp2601_bat_power *bat_power, uint8_t *para,
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

static int axp2601_model_update(struct axp2601_bat_power *bat_power)
{
	struct regmap *regmap = bat_power->regmap;
	int ret = 0;
	unsigned int data;
	unsigned int len;
	uint8_t i;
	u32 reg;
	uint8_t *param;

	/* reset power curve regs */
	reg = readl(bat_power->writebase);
	reg &= ~(0xFF);
	writel(reg, bat_power->writebase);

	/* reset and open brom */

	ret = regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG,
				 AXP2601_BROMUP_EN, AXP2601_BROMUP_EN);
	if (ret < 0) {
		PMIC_ERR("%s %d:reg update, i2c communication err!\n", __func__, __LINE__);
		goto UPDATE_ERR;
	}

	mdelay(500);

	/* down load battery parameters */
	len = AXP2601_MAX_PARAM;
	param = devm_kzalloc(bat_power->dev, AXP2601_MAX_PARAM, GFP_KERNEL);
	if (!param) {
		PMIC_ERR("can not find memory for param\n");
		goto UPDATE_ERR;
	}
	ret = axp2601_get_param(bat_power, param, &len);
	if (ret < 0) {
		PMIC_ERR("can not find get param\n");
		goto err_param;
	}

	for (i = 0; i < len; i++) {
		ret = regmap_write(regmap, AXP2601_GAUGE_BROM, param[i]);
		if (ret < 0) {
			PMIC_ERR("%s %d:reg update, i2c communication err!\n", __func__, __LINE__);
			goto err_param;
		}
	}
	/* reset and open brom */
	ret = regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG,
				 AXP2601_BROMUP_EN, 0);
	if (ret < 0) {
		PMIC_ERR("%s %d:reg update, i2c communication err!\n", __func__, __LINE__);
		goto err_param;
	}

	ret = regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG,
				 AXP2601_BROMUP_EN, AXP2601_BROMUP_EN);
	if (ret < 0) {
		PMIC_ERR("%s %d:reg update, i2c communication err!\n", __func__, __LINE__);
		goto err_param;
	}

	/* check battery parameters is ok ? */
	for (i = 0; i < len; i++) {
		ret = regmap_read(regmap, AXP2601_GAUGE_BROM, &data);
		if (ret < 0) {
			PMIC_ERR("%s %d:reg update, i2c communication err!\n", __func__, __LINE__);
			goto err_param;
		}

		if (data != param[i]) {
			PMIC_ERR("model param check %02x error! data:0x%x[para:0x%x]\n", i, data, param[i]);
			goto err_param;
		}
	}

	devm_kfree(bat_power->dev, param);

	/* close brom and set battery update flag */
	ret = regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG, AXP2601_BROMUP_EN,
				 0);
	if (ret < 0)
		goto UPDATE_ERR;

	ret = regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG,
				 AXP2601_CFG_UPDATE_MARK,
				 AXP2601_CFG_UPDATE_MARK);
	if (ret < 0)
		goto UPDATE_ERR;

	/* reset_mcu */
	ret = axp2601_reset_mcu(bat_power);
	if (ret < 0)
		goto UPDATE_ERR;

	mdelay(1000);
	axp2601_set_ichg(bat_power, bat_power->dts_info.pmu_runtime_chgcur);

	/* update ok */
	return 0;

err_param:
	devm_kfree(bat_power->dev, param);

UPDATE_ERR:
	regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG, AXP2601_BROMUP_EN, 0);
	axp2601_reset_mcu(bat_power);

	return ret;
}

static int axp2601_blance_vol(struct axp2601_bat_power *bat_power)
{
	int vol_in_reg, reg_value;
	int ret;
	u32 reg;

	reg = readl(bat_power->writebase);

	reg_value = reg & 0x7F;

	ret = regmap_read(bat_power->regmap, AXP2601_GAUGE_SOC, &vol_in_reg);
	if (ret < 0)
		return ret;
	vol_in_reg &= 0x7F;

	if (vol_in_reg > AXP2601_SOC_MAX)
		vol_in_reg = AXP2601_SOC_MAX;
	else if (vol_in_reg < AXP2601_SOC_MIN)
		vol_in_reg = AXP2601_SOC_MIN;

	PMIC_INFO("vol_in_reg = %d, reg_value = %d\n", vol_in_reg, reg_value);
	return vol_in_reg - reg_value;

}

static bool axp2601_model_update_check(struct regmap *regmap)
{
	int ret = 0;
	unsigned int data;

	ret = regmap_read(regmap, AXP2601_GAUGE_CONFIG, &data);
	if (ret < 0)
		goto CHECK_ERR;

	if ((data & AXP2601_CFG_UPDATE_MARK) == 0)
		goto CHECK_ERR;


	return true;

CHECK_ERR:
	regmap_update_bits(regmap, AXP2601_GAUGE_CONFIG, AXP2601_BROMUP_EN, 0);
	return false;
}

static int axp2601_bat_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	int ret = 0;

	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2601_get_bat_status(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp2601_get_bat_present(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp2601_get_vbat_vol(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = axp2601_get_soc(psy, val); // unit %;
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp2601_get_temp(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp2601_get_ichg_lim(psy, val);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
		if (bat_power->charger_cooler_type) {
			val->intval = atomic_read(&bat_power->pmu_limit_status);
			break;
		}
		ret = axp2601_get_ichg_lim(psy, val);
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
		ret = axp2601_get_bat_health(psy, val);
		if (ret < 0)
			return ret;
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MIN:
		if (axp2601_usb_power_get(bat_power)) {
			power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP_ALERT_MIN, val);
		} else {
			val->intval = -200000;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP_ALERT_MAX:
		if (axp2601_usb_power_get(bat_power)) {
			power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP_ALERT_MAX, val);
		} else {
			val->intval = 200000;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN:
		if (axp2601_usb_power_get(bat_power)) {
			power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MIN, val);
		} else {
			val->intval = -200000;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX:
		if (axp2601_usb_power_get(bat_power)) {
			power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_TEMP_AMBIENT_ALERT_MAX, val);
		} else {
			val->intval = 200000;
		}
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = AXP2601_MANUFACTURER;
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = bat_power->dts_info.pmu_battery_cap * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		ret = axp2601_get_charger_count(psy, val);
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

static int axp2601_bat_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct axp2601_bat_power *bat_power = power_supply_get_drvdata(psy);
	int ret = 0, lim_cur = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = axp2601_set_ichg(bat_power, val->intval);
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
		ret = axp2601_set_ichg(bat_power, lim_cur);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		atomic_set(&bat_power->charge_cycle_count, val->intval);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp2601_set_status(bat_power, val->intval);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int axp2601_bat_property_is_writeable(struct power_supply *psy,
			     enum power_supply_property psp)
{
	int ret = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL_LIMIT:
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

static const struct power_supply_desc axp2601_bat_desc = {
	.name = "axp2601-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp2601_bat_get_property,
	.set_property = axp2601_bat_set_property,
	.properties = axp2601_bat_props,
	.num_properties = ARRAY_SIZE(axp2601_bat_props),
	.property_is_writeable = axp2601_bat_property_is_writeable,
};

static int axp2601_init_chip(struct axp2601_bat_power *bat_power)
{
	int ret = 0;
	int val;
	u32 reg;
	uint8_t data[2];
	union power_supply_propval qc_val;
	struct bmu_ext_config_info *bmp_ext_config = &bat_power->dts_info;

	if (bat_power == NULL)
		return -ENODEV;

	/* init bat cycle*/
	atomic_set(&bat_power->charge_cycle_count, 0);
	bmp_ext_config->pmu_bat_cycle_cap_reduce *= bmp_ext_config->pmu_battery_cap * 10 / bmp_ext_config->pmu_bat_cycle_life;

	/* default sets */
	atomic_set(&bat_power->pmu_limit_status, 0);

	ret = regmap_update_bits(bat_power->regmap, AXP2601_RESET_CFG, AXP2601_MODE_SLEEP, 0);
	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp2601 reg update, i2c communication err!\n");
		return ret;
	}

	if (axp2601_usb_power_get(bat_power)) {
		power_supply_get_property(bat_power->qc_psy, POWER_SUPPLY_PROP_CALIBRATE, &qc_val);
	} else {
		qc_val.intval = _axp2601_get_bat_present(bat_power);
	}

	if (!qc_val.intval)
		return ret;

	/* update battery model*/
	if (!axp2601_model_update_check(bat_power->regmap)) {
		ret = axp2601_model_update(bat_power);
		if (ret < 0) {
			PMIC_DEV_ERR(bat_power->dev, "axp2601 model update fail!\n");
			return ret;
		}
	}
	PMIC_DEV_DEBUG(bat_power->dev, "axp2601 model update ok\n");
	/*end of update battery model*/

	/*  charger change current can be divided by 50 */
	bmp_ext_config->pmu_bat_charge_control_lim = clamp_val(bmp_ext_config->pmu_bat_charge_control_lim, 0, 3072);
	bmp_ext_config->pmu_bat_charge_control_lim = DIVIDE_BY_50(bmp_ext_config->pmu_bat_charge_control_lim);
	bmp_ext_config->pmu_runtime_chgcur = clamp_val(bmp_ext_config->pmu_runtime_chgcur, 0, 3072);
	bmp_ext_config->pmu_runtime_chgcur = DIVIDE_BY_50(bmp_ext_config->pmu_runtime_chgcur);

	/* battery ratio check */
	regmap_read(bat_power->regmap, AXP2601_GAUGE_SOC, &val);
	val &= 0x7F;
	reg = readl(bat_power->writebase);
	PMIC_INFO("radio:%d", val);
	if (val > 60) {
		ret = regmap_bulk_read(bat_power->regmap, AXP2601_VBAT_H, data, 2);
		val = ((data[0] & GENMASK(5, 0)) << 0x08) | (data[1]);
		PMIC_WARN("\n\nbat_vol:%d\n\n", val);
		if (val < 3500) {
			axp2601_reset_mcu(bat_power);
			reg |= 0x81;
			writel(reg, bat_power->writebase);
			PMIC_WARN("adapt reset gauge: soc > 60%% , bat_vol < 3500\n");
		} else {
			reg &= ~(0xFF);
			writel(reg, bat_power->writebase);
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

static irqreturn_t axp2601_irq_handler_bat_stat_change(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	struct axp2601_bat_power *bat_power = data;

	PMIC_DEBUG("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	switch (id->irq_data.hwirq) {
	case AXP2601_IRQ_NEWSOC:
		regmap_update_bits(bat_power->regmap, AXP2601_IRQ, 0x02, 0);
		regmap_update_bits(bat_power->regmap, AXP2601_IRQ, 0x02, 0x02);
		PMIC_INFO("interrupt:battery new soc");
		break;
	default:
		PMIC_DEBUG("interrupt:others");
		break;
	}

	return IRQ_HANDLED;
}

enum axp2601_bat_virq_index {
	AXP2601_VIRQ_BAT_NEW_SOC,
	AXP2601_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp_bat_irq[] = {
	[AXP2601_VIRQ_BAT_NEW_SOC] = { "gauge_new_soc",
					  axp2601_irq_handler_bat_stat_change },
};

static int axp2601_bat_dt_parse(struct device_node *node,
			 struct bmu_ext_config_info *bmp_ext_config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	BMU_EXT_OF_PROP_READ(pmu_battery_cap,                 4000);
	BMU_EXT_OF_PROP_READ(pmu_battery_warning_level,       15);
	BMU_EXT_OF_PROP_READ(pmu_runtime_chgcur,              2000);
	BMU_EXT_OF_PROP_READ(pmu_bat_charge_control_lim,	  600);
	BMU_EXT_OF_PROP_READ(pmu_bat_cycle_life,              800);
	BMU_EXT_OF_PROP_READ(pmu_bat_cycle_cap_reduce,        20);
	BMU_EXT_OF_PROP_READ(pmu_bat_manufacture_year,       2024);
	BMU_EXT_OF_PROP_READ(pmu_bat_manufacture_month,         1);
	BMU_EXT_OF_PROP_READ(pmu_bat_manufacture_day,           1);

	bmp_ext_config->wakeup_new_soc =
		of_property_read_bool(node, "wakeup_new_soc");

	return 0;
}

static void axp2601_bat_parse_device_tree(struct axp2601_bat_power *bat_power)
{
	int ret;
	struct bmu_ext_config_info *bmp_ext_config;

	/* set input current limit */
	if (!bat_power->dev->of_node) {
		PMIC_INFO("can not find device tree\n");
		return;
	}

	bmp_ext_config = &bat_power->dts_info;
	ret = axp2601_bat_dt_parse(bat_power->dev->of_node, bmp_ext_config);
	if (ret) {
		PMIC_INFO("can not parse device tree err\n");
		return;
	}
}

static void axp2601_bat_power_monitor(struct work_struct *work)
{
	struct axp2601_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);

	power_supply_changed(bat_power->bat_supply);

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

static void axp2601_bat_power_full_curve_monitor(struct work_struct *work)
{
	struct axp2601_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_power_full_curve.work);
	union power_supply_propval val;
	static int rest_vol, blance_vol;
	u32 reg;
	unsigned int reg_value = 0, ret = 0;
	struct power_supply       *vbus_status;

	power_supply_changed(bat_power->bat_supply);

	if (!(axp2601_can_charge_status_get(bat_power))) {
		return;
	}

	if (axp2601_usb_power_get(bat_power)) {
		vbus_status = bat_power->qc_psy;
	} else if (axp2601_gpio_vbus_power_get(bat_power)) {
		vbus_status = bat_power->gpio_vbus_psy;
	}


	reg = readl(bat_power->writebase);

	if (reg & 0x100) {
		blance_vol = axp2601_blance_vol(bat_power);
		PMIC_DEBUG("blance_vol = %d\n", blance_vol);

		regmap_read(bat_power->regmap, AXP2601_GAUGE_SOC, &reg_value);
		rest_vol = reg & 0x7F;
		reg_value &= 0x7F;
		if (reg_value <= 90) {
			PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
			__pm_relax(bat_power->ws);
			reg &= ~(0x17F);
			writel(reg, bat_power->writebase);
			return;
		}
		if (blance_vol <= 0) {
			if (axp2601_usb_power_get(bat_power)) {
				power_supply_get_property(vbus_status, POWER_SUPPLY_PROP_STATUS, &val);
				if (val.intval == POWER_SUPPLY_STATUS_CHARGING || val.intval == POWER_SUPPLY_STATUS_FULL)
					ret = 1;
				else
					ret = 0;
			}

			if (ret) {
				if (rest_vol < 99) {
					rest_vol++;
				} else {
					rest_vol = 101;
					if (reg_value == 100) {
						PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
						__pm_relax(bat_power->ws);
						return;
					}
				}
			} else {
				if (blance_vol < 0) {
					rest_vol--;
				} else {
					PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
					__pm_relax(bat_power->ws);
					reg &= ~(0x17F);
					writel(reg, bat_power->writebase);
					return;
				}
			}
			PMIC_DEBUG("%s:rest_vol:%d, AXP2601_GAUGE_SOC:%d\n", __func__, rest_vol, reg_value);
			reg &= ~(0x7F);
			reg |= rest_vol;
			writel(reg, bat_power->writebase);
			schedule_delayed_work(&bat_power->bat_power_full_curve, msecs_to_jiffies(60 * 1000));
		} else {
			PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
			__pm_relax(bat_power->ws);
			reg &= ~(0x17F);
			writel(reg, bat_power->writebase);
		}
	} else {
		PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
		__pm_relax(bat_power->ws);
		reg &= ~(0x17F);
		writel(reg, bat_power->writebase);
	}
}

static void axp2601_bat_power_curve_monitor(struct work_struct *work)
{
	struct axp2601_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_power_curve.work);
	union power_supply_propval val;
	static int rest_vol, blance_vol;
	u32 reg;
	unsigned int reg_value = 0;
	struct power_supply       *vbus_status;

	power_supply_changed(bat_power->bat_supply);

	if (!(axp2601_can_charge_status_get(bat_power))) {
		return;
	}

	if (axp2601_usb_power_get(bat_power)) {
		vbus_status = bat_power->qc_psy;
	} else if (axp2601_gpio_vbus_power_get(bat_power)) {
		vbus_status = bat_power->gpio_vbus_psy;
	}

	reg = readl(bat_power->writebase);

	if (reg & 0x80) {
		blance_vol = axp2601_blance_vol(bat_power);
		PMIC_DEBUG("blance_vol = %d\n", blance_vol);

		rest_vol = reg & 0x7F;
		if (blance_vol >= 1) {
			if (axp2601_usb_power_get(bat_power)) {
				power_supply_get_property(vbus_status, POWER_SUPPLY_PROP_ONLINE, &val);
				reg_value = val.intval;
			}

			if (reg_value)
				rest_vol++;

			PMIC_DEBUG("%s:rest_vol:%d\n", __func__, rest_vol);
			reg |= rest_vol;
			writel(reg, bat_power->writebase);
			schedule_delayed_work(&bat_power->bat_power_curve, msecs_to_jiffies(30 * 1000));
		} else {
			PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
			__pm_relax(bat_power->ws);
			reg &= ~(0xFF);
			writel(reg, bat_power->writebase);
		}
	} else {
		PMIC_DEBUG("release wake lock:rest_vol:%d\n", rest_vol);
		__pm_relax(bat_power->ws);
		reg &= ~(0xFF);
		writel(reg, bat_power->writebase);
	}
}

static int axp2601_battery_probe(struct platform_device *pdev)
{
	u32 reg;
	int ret = 0;
	int i = 0, irq, reg_value;

	struct axp2601_bat_power *bat_power;
	struct power_supply_config psy_cfg = {};
	struct bmu_ext_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct device_node *node = pdev->dev.of_node;
	struct device_node *np = NULL;


	PMIC_ERR("axp2601-battery device probe\n");
	if (!of_device_is_available(node)) {
		PMIC_ERR("axp2601-battery device is not configed\n");
		return -ENODEV;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (bat_power == NULL) {
		PMIC_ERR("axp2601_bat_power alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	bat_power->name = "axp2601_battery";
	bat_power->dev = &pdev->dev;
	bat_power->regmap = axp_dev->regmap;

	/* for device tree parse */
	axp2601_bat_parse_device_tree(bat_power);

	bat_power->writebase = ioremap(AXP2601_FLAGE_REG, 4);
	if (!bat_power->writebase) {
		PMIC_DEV_ERR(bat_power->dev, "unable to ioremap flag regs!\n");
		ret = -ENXIO;
		goto err;
	}

	if (of_find_property(bat_power->dev->of_node, "det_qc_supply", NULL)) {
		np = of_parse_phandle(bat_power->dev->of_node, "det_qc_supply", 0);
		if (!of_device_is_available(np)) {
			PMIC_INFO("no quick charge\n");
			bat_power->qc_psy =  NULL;
		} else {
			bat_power->qc_psy = devm_power_supply_get_by_phandle(bat_power->dev, "det_qc_supply");
			PMIC_INFO("used quick charge\n");
			if (!(bat_power->qc_psy) || (IS_ERR(bat_power->qc_psy)))
				return -EPROBE_DEFER;
			of_property_read_u32(np, "pmu_runtime_chgcur", &ret);
			if (ret) {
				bat_power->dts_info.pmu_runtime_chgcur = ret;
			}
		}
	} else {
		PMIC_INFO("axp2601:no quick charge\n");
		bat_power->qc_psy = NULL;
		if (of_find_property(bat_power->dev->of_node, "gpio_vbus_det_supply", NULL)) {
			np = of_parse_phandle(bat_power->dev->of_node, "gpio_vbus_det_supply", 0);
			if (!of_device_is_available(np)) {
				PMIC_INFO("no gpio vbus det\n");
				bat_power->gpio_vbus_psy =  NULL;
			} else {
				bat_power->gpio_vbus_psy = devm_power_supply_get_by_phandle(bat_power->dev, "gpio_vbus_det_supply");
				PMIC_INFO("used gpio vbus det\n");
				if (!(bat_power->gpio_vbus_psy) || (IS_ERR(bat_power->gpio_vbus_psy)))
					return -EPROBE_DEFER;
			}
		} else {
			PMIC_INFO("no gpio vbus det\n");
			bat_power->gpio_vbus_psy = NULL;
		}
	}

	ret = axp2601_init_chip(bat_power);
	if (ret < 0) {
		PMIC_DEV_ERR(bat_power->dev, "axp2601 init chip fail!\n");
		ret = -ENODEV;
		goto err;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
			&axp2601_bat_desc, &psy_cfg);

	if (IS_ERR(bat_power->bat_supply)) {
		PMIC_ERR("axp2601 failed to register bat power\n");
		ret = PTR_ERR(bat_power->bat_supply);
		return ret;
	}

	bmu_ext_register_cooler(bat_power->bat_supply);

	if (axp_dev->irq) {
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
	} else {
		PMIC_INFO("axp2601:no irq\n");
	}

	platform_set_drvdata(pdev, bat_power);

	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp2601_bat_power_monitor);
	INIT_DELAYED_WORK(&bat_power->bat_power_full_curve, axp2601_bat_power_full_curve_monitor);
	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(500));

	if (!(axp2601_usb_power_get(bat_power))) {
		writel(0, bat_power->writebase);
	}

	reg = readl(bat_power->writebase);
	if (ret < 0)
		return ret;

	reg &= ~(0x100);
	writel(reg, bat_power->writebase);
	bat_power->ws = wakeup_source_register(&pdev->dev, "bat_curve_smooth");
	if (reg & 0x80) {
		reg_value = axp2601_blance_vol(bat_power);
		PMIC_DEBUG("bat curve smoothing: hold wake lock,blance_vol = %d\n", reg_value);
		__pm_stay_awake(bat_power->ws);
		INIT_DELAYED_WORK(&bat_power->bat_power_curve, axp2601_bat_power_curve_monitor);
		schedule_delayed_work(&bat_power->bat_power_curve, msecs_to_jiffies(30 * 1000));
	}

	return ret;

err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static int axp2601_battery_remove(struct platform_device *pdev)
{
	struct axp2601_bat_power *bat_power = platform_get_drvdata(pdev);

	PMIC_DEV_DEBUG(&pdev->dev, "==============AXP2601 unegister==============\n");
	if (bat_power->bat_supply) {
		power_supply_unregister(bat_power->bat_supply);
		bmu_ext_unregister_cooler(bat_power->bat_supply);
	}
	PMIC_DEV_DEBUG(&pdev->dev, "axp2601 teardown battery dev\n");

	return 0;
}

static inline void axp2601_bat_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp2601_bat_virq_dts_set(struct axp2601_bat_power *bat_power, bool enable)
{
	struct bmu_ext_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_new_soc)
		axp2601_bat_irq_set(
			axp_bat_irq[AXP2601_VIRQ_BAT_NEW_SOC].irq,
			enable);

}

static void axp2601_bat_shutdown(struct platform_device *pdev)
{
	struct axp2601_bat_power *bat_power = platform_get_drvdata(pdev);
	u32 reg_value;

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	reg_value = readl(bat_power->writebase);
	if (reg_value & 0x80) {
		cancel_delayed_work_sync(&bat_power->bat_power_curve);
	}
	if (reg_value & 0x100) {
		if ((reg_value & 0x7F) < 99)
			cancel_delayed_work_sync(&bat_power->bat_power_full_curve);
	}
}

static int axp2601_bat_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp2601_bat_power *bat_power = platform_get_drvdata(pdev);
	u32 reg_value;

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);
	atomic_set(&bat_power->pmu_limit_status, 0);
	reg_value = readl(bat_power->writebase);
	if (reg_value & 0x80) {
		cancel_delayed_work_sync(&bat_power->bat_power_curve);
	}
	if (reg_value & 0x100) {
		if ((reg_value & 0x7F) < 99)
			cancel_delayed_work_sync(&bat_power->bat_power_full_curve);
	}
	axp2601_bat_virq_dts_set(bat_power, false);
	return 0;
}

static int axp2601_bat_resume(struct platform_device *pdev)
{
	struct axp2601_bat_power *bat_power = platform_get_drvdata(pdev);
	u32 reg_value;

	power_supply_changed(bat_power->bat_supply);

	schedule_delayed_work(&bat_power->bat_supply_mon, 0);
	reg_value = readl(bat_power->writebase);
	if (reg_value & 0x80) {
		schedule_delayed_work(&bat_power->bat_power_curve, 0);
	}
	if (reg_value & 0x100) {
		if ((reg_value & 0x7F) < 99) {
			__pm_stay_awake(bat_power->ws);
			schedule_delayed_work(&bat_power->bat_power_full_curve, 0);
		}
	}
	axp2601_bat_virq_dts_set(bat_power, true);
	return 0;
}

static const struct of_device_id axp2601_bat_power_match[] = {
	{
		.compatible = "x-powers-ext,axp2601-bat-power-supply",
		.data = (void *)AXP2601_ID,
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, axp2601_bat_power_match);

static struct platform_driver axp2601_bat_power_driver = {
	.driver = {
		.name = "axp2601-bat-power-supply",
		.of_match_table = axp2601_bat_power_match,
	},
	.probe = axp2601_battery_probe,
	.remove = axp2601_battery_remove,
	.shutdown = axp2601_bat_shutdown,
	.suspend = axp2601_bat_suspend,
	.resume = axp2601_bat_resume,
};

module_platform_driver(axp2601_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp2601 battery driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
