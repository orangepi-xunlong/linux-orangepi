/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * AXP22X PMIC USB power supply status driver
 *
 * Copyright (C) 2015 Hans de Goede <hdegoede@redhat.com>
 * Copyright (C) 2014 Bruno Premont <bonbons@linux-vserver.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 */

#include "axp22x_charger.h"

struct axp22x_bat_power {
	char                      *name;
	struct device             *dev;
	struct regmap             *regmap;
	struct power_supply       *bat_supply;
	struct delayed_work        bat_supply_mon;
	struct axp_config_info     dts_info;
};

static enum power_supply_property axp22x_bat_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static int axp22x_get_present(struct axp22x_bat_power *bat_power)
{
	int reg_value;
	struct regmap *map = bat_power->regmap;

	if (bat_power->dts_info.pmu_bat_unused)
		return 0;

	regmap_read(map, AXP22X_MODE_CHGSTATUS, &reg_value);

	if (reg_value & BIT(4))
		if (reg_value & BIT(5))
			return 1;

	return 0;
}

static int axp22x_get_rest_cap(struct axp22x_bat_power *bat_power)
{
	int val;
	uint8_t temp_val[2], batt_max_cap_val[2];
	int batt_max_cap, coulumb_counter;
	int rest_vol = 0;
	struct regmap *map = bat_power->regmap;

	regmap_read(map, AXP22X_CAP, &val);
	if (val & 0x80)
		rest_vol = (int) (val & 0x7F);

	regmap_bulk_read(map, AXP22X_COUCNT0, temp_val, 2);
	coulumb_counter = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;
	regmap_bulk_read(map, AXP22X_BATCAP0, temp_val, 2);
	batt_max_cap = (((temp_val[0] & 0x7f) << 8) + temp_val[1])
						* 1456 / 1000;

	/* Avoid the power stay in 100% for a long time. */
	if (coulumb_counter > batt_max_cap) {
		batt_max_cap_val[0] = temp_val[0] | (0x1<<7);
		batt_max_cap_val[1] = temp_val[1];
		regmap_bulk_write(map, AXP22X_COUCNT0, batt_max_cap_val, 2);
		pr_info("axp22x coulumb_counter = %d\n", batt_max_cap);
	}

	return rest_vol;
}

static int axp22x_get_bat_health(struct axp22x_bat_power *bat_power)
{
	int val;
	struct regmap *map = bat_power->regmap;

	regmap_read(map, AXP22X_MODE_CHGSTATUS, &val);
	if (val & AXP22X_FAULT_LOG_BATINACT)
		return POWER_SUPPLY_HEALTH_DEAD;
	else if (val & AXP22X_FAULT_LOG_OVER_TEMP)
		return POWER_SUPPLY_HEALTH_OVERHEAT;
	else if (val & AXP22X_FAULT_LOG_COLD)
		return POWER_SUPPLY_HEALTH_COLD;
	else
		return POWER_SUPPLY_HEALTH_GOOD;
}

static int axp22x_get_bat_status(struct power_supply *psy,
					union power_supply_propval *val)
{
	bool bat_det, bat_charging;
	bool ac_valid, vbus_valid;
	unsigned int rest_vol;
	unsigned int reg_value;
	int ret;

	struct axp22x_bat_power *bat_power = power_supply_get_drvdata(psy);

	ret = regmap_read(bat_power->regmap, AXP22X_MODE_CHGSTATUS, &reg_value);
	if (ret)
		return ret;
	bat_det = !!(reg_value & AXP22X_CHGSTATUS_BAT_PST_VALID) &&
		!!(reg_value & AXP22X_CHGSTATUS_BAT_PRESENT);
	bat_charging = !!(reg_value & AXP22X_CHGSTATUS_BAT_CHARGING);

	ret = regmap_read(bat_power->regmap, AXP22X_STATUS, &reg_value);
	if (ret)
		return ret;
	ac_valid = !!(reg_value & AXP22X_STATUS_AC_USED);
	vbus_valid = !!(reg_value & AXP22X_STATUS_VBUS_USED);

	rest_vol = axp22x_get_rest_cap(bat_power);

	if (ac_valid || vbus_valid) {
		if (bat_det) {
			if (rest_vol == 100)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if (bat_charging)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else {
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
		}
	} else {
		ret = regmap_read(bat_power->regmap, AXP22X_MODE_CHGSTATUS, &reg_value);
		if (ret)
			return ret;
		bat_det = !!(reg_value & AXP22X_CHGSTATUS_BAT_PRESENT);
		if (bat_det)
			val->intval =  POWER_SUPPLY_STATUS_DISCHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return 0;
}

static inline int axp22x_vbat_to_mV(u32 reg)
{
	return ((int)(((reg >> 8) << 4) | (reg & 0x000F))) * 1100 / 1000;
}

static inline int axp22x_ibat_to_mA(u32 reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static inline int axp22x_icharge_to_mA(u32 reg)
{
	return (int)(((reg >> 8) << 4) | (reg & 0x000F));
}

static int axp22x_get_temp(struct axp22x_bat_power *bat_power)
{
	u32 res;
	uint8_t tmp[2];
	struct regmap *map = bat_power->regmap;

	regmap_bulk_read(map, AXP22X_INTTEMP, tmp, 2);
	res = ((tmp[0] << 4) | tmp[1]) * 10625 / 10000 - 2677;

	return (int)res;
}

static int axp22x_get_vbat(struct axp22x_bat_power *bat_power)
{
	uint8_t tmp[2];
	u32 res;
	struct regmap *map = bat_power->regmap;

	regmap_bulk_read(map, AXP22X_VBATH_RES, tmp, 2);
	res = (tmp[0] << 8) | tmp[1];

	return axp22x_vbat_to_mV(res);
}

static int axp22x_get_ibat(struct axp22x_bat_power *bat_power)
{
	uint8_t tmp[2];
	u32 res;
	struct regmap *map = bat_power->regmap;

	regmap_bulk_read(map, AXP22X_IBATH_REG, tmp, 2);
	res = (tmp[0] << 8) | tmp[1];

	return axp22x_icharge_to_mA(res);
}

static int axp22x_get_disibat(struct axp22x_bat_power *bat_power)
{
	uint8_t tmp[2];
	u32 dis_res;
	struct regmap *map = bat_power->regmap;

	regmap_bulk_read(map, AXP22X_DISIBATH_REG, tmp, 2);
	dis_res = (tmp[0] << 8) | tmp[1];

	return axp22x_ibat_to_mA(dis_res);
}

static int axp22x_set_bat_chg_cur(struct axp22x_bat_power *bat_power, int cur)
{
	struct regmap *map = bat_power->regmap;

	cur = clamp_val(cur, 0, 2550);
	if (cur == 0)
		regmap_update_bits(map, AXP22X_CHRG_CTRL1, 0x80, 0x00);
	else
		regmap_update_bits(map, AXP22X_CHRG_CTRL1, 0x80, 0x80);

	cur = clamp_val(cur, 300, 2550);
	cur = (cur - 300) / 150;
	regmap_update_bits(map, AXP22X_CHRG_CTRL1, 0x0F, cur);

	return 0;
}

static int axp22x_bat_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	struct axp22x_bat_power *bat_power = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->desc->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		ret = axp22x_get_present(bat_power);
		if (ret < 0)
			return ret;
		val->intval = ret;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = axp22x_get_bat_health(bat_power);
		if (ret < 0)
			return ret;
		val->intval = ret;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = axp22x_get_present(bat_power);
		if (ret) {
			ret = axp22x_get_rest_cap(bat_power);
			if (ret < 0)
				return ret;
			val->intval = ret;
		} else {
			val->intval = 0;
		}
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = axp22x_get_present(bat_power);
		if (ret) {
			ret = axp22x_get_vbat(bat_power);
			if (ret < 0)
				return ret;
			val->intval = ret;
		} else {
			val->intval = 0;
		}
		break;
	case POWER_SUPPLY_PROP_TEMP:
		ret = axp22x_get_temp(bat_power);
		val->intval = ret;
		break;
	case POWER_SUPPLY_PROP_STATUS:
		ret = axp22x_get_bat_status(psy, val);
		val->intval = ret;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (axp22x_get_ibat(bat_power)
				- axp22x_get_disibat(bat_power)) * 1000;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static const struct power_supply_desc axp22x_bat_desc = {
	.name = "axp22x-bat",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.get_property = axp22x_bat_get_property,
	.properties = axp22x_bat_props,
	.num_properties = ARRAY_SIZE(axp22x_bat_props),
};

static int axp22x_bat_update(struct axp22x_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *map = bat_power->regmap;
	unsigned char ocv_cap[32];

	/* bat para */
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
	regmap_bulk_write(map, AXP22X_OCV_BASE, ocv_cap, 32);

	return 0;
}

static int axp22x_bat_power_init(struct axp22x_bat_power *bat_power)
{
	unsigned int val;
	int cur_coulomb_counter, rdc;
	int i;
	int update_min_times[8] = {30, 60, 120, 164, 0, 5, 10, 20};

	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct regmap *map = bat_power->regmap;


	regmap_read(map, AXP22X_CHRG_CTRL1, &val);
	if (axp_config->pmu_init_chgend_rate == 10)
		val &= ~(1 << 4);
	else
		val |= 1 << 4;
	val &= 0x7F;
	regmap_write(map, AXP22X_CHRG_CTRL1, val);

	if (axp_config->pmu_init_chg_pretime < 40)
		axp_config->pmu_init_chg_pretime = 40;

	if (axp_config->pmu_init_chg_csttime < 360)
		axp_config->pmu_init_chg_csttime = 360;

	val = ((((axp_config->pmu_init_chg_pretime - 40) / 10) << 6)
			| ((axp_config->pmu_init_chg_csttime - 360) / 120));
	regmap_update_bits(map, AXP22X_CHRG_CTRL2, 0xC2, val);


	/* adc set */
	val = AXP22X_ADC_BATVOL_ENABLE | AXP22X_ADC_BATCUR_ENABLE;
	if (axp_config->pmu_bat_temp_enable != 0)
		val = val | AXP22X_ADC_TSVOL_ENABLE;
	regmap_update_bits(map, AXP22X_ADC_EN,
						AXP22X_ADC_BATVOL_ENABLE
						| AXP22X_ADC_BATCUR_ENABLE
						| AXP22X_ADC_TSVOL_ENABLE, val);


	regmap_read(map, AXP22X_ADC_CONTROL3, &val);
	switch (axp_config->pmu_init_adc_freq / 100) {
	case 1:
		val &= ~(3 << 6);
		break;
	case 2:
		val &= ~(3 << 6);
		val |= 1 << 6;
		break;
	case 4:
		val &= ~(3 << 6);
		val |= 2 << 6;
		break;
	case 8:
		val |= 3 << 6;
		break;
	default:
		break;
	}

	if (axp_config->pmu_bat_temp_enable != 0)
		val &= (~(1 << 2));
	regmap_write(map, AXP22X_ADC_CONTROL3, val);

	/* set target voltage */
	if (axp_config->pmu_init_chgvol < 4200) {
		val = 0;
	} else if (axp_config->pmu_init_chgvol < 4220) {
		val = 1;
	} else if (axp_config->pmu_init_chgvol < 4240) {
		val = 2;
	} else {
		val = 3;
	}
	val <<= 5;
	regmap_update_bits(map, AXP22X_CHRG_CTRL1, 0x60, val);

	/* update battery model*/
	if (axp22x_get_present(bat_power)) {
		val = clamp_val(axp_config->pmu_battery_warning_level1 - 5, 0, 15) << 4;
		val |= clamp_val(axp_config->pmu_battery_warning_level2, 0, 15);
		regmap_write(map, AXP22X_WARNING_LEVEL, val);

		axp22x_bat_update(bat_power);
	}
	/*end of update battery model*/

	/*Init CHGLED function*/
	if (axp_config->pmu_chgled_func)
		regmap_update_bits(map, AXP22X_OFF_CTL, 0x08, 0x08); /* by charger */
	else
		regmap_update_bits(map, AXP22X_OFF_CTL, 0x08, 0x00); /* drive MOTO */

	/*set CHGLED Indication Type*/
	if (axp_config->pmu_chgled_type)
		regmap_update_bits(map, AXP22X_CHRG_CTRL2, 0x10, 0x10); /* Type B */
	else
		regmap_update_bits(map, AXP22X_CHRG_CTRL2, 0x10, 0x00); /* Type A */

	/* Init battery capacity correct function */
	if (axp_config->pmu_batt_cap_correct)
		regmap_update_bits(map, AXP22X_COULOMB_CTL, 0x20, 0x20);
	else
		regmap_update_bits(map, AXP22X_COULOMB_CTL, 0x20, 0x00);

	/* Init battery regulator enable or not when charge finish */
	if (axp_config->pmu_chg_end_on_en)
		regmap_update_bits(map, AXP22X_CHRG_CTRL2, 0x20, 0x20);
	else
		regmap_update_bits(map, AXP22X_CHRG_CTRL2, 0x20, 0x00);

	if (axp_config->pmu_batdeten)
		regmap_update_bits(map, AXP22X_OFF_CTL, 0x40, 0x40);
	else
		regmap_update_bits(map, AXP22X_OFF_CTL, 0x40, 0x00);

	/* RDC initial */
	regmap_read(map, AXP22X_RDC0, &val);
	if ((axp_config->pmu_battery_rdc) && (!(val & 0x40))) {
		rdc = (axp_config->pmu_battery_rdc * 10000 + 5371) / 10742;
		regmap_write(map, AXP22X_RDC0, ((rdc >> 8) & 0x1F)|0x80);
		regmap_write(map, AXP22X_RDC1, rdc & 0x00FF);
	}

	regmap_read(map, AXP22X_BATCAP0, &val);
	if ((axp_config->pmu_battery_cap) && (!(val & 0x80))) {
		cur_coulomb_counter = axp_config->pmu_battery_cap
					* 1000 / 1456;
		regmap_write(map, AXP22X_BATCAP0, ((cur_coulomb_counter >> 8) | 0x80));
		regmap_write(map, AXP22X_BATCAP1, cur_coulomb_counter & 0x00FF);
	} else if (!axp_config->pmu_battery_cap) {
		regmap_write(map, AXP22X_BATCAP0, 0x00);
		regmap_write(map, AXP22X_BATCAP1, 0x00);
	}


	if (axp_config->pmu_ocv_en == 0) {
		pr_warn("axp22x ocv must be enabled\n");
		axp_config->pmu_ocv_en = 1;
	}

	if (axp_config->pmu_cou_en == 1) {
		/* use ocv and cou */
		regmap_update_bits(map, AXP22X_COULOMB_CTL, 0x80, 0x80);
		regmap_update_bits(map, AXP22X_COULOMB_CTL, 0x40, 0x40);
	} else if (axp_config->pmu_cou_en == 0) {
		/* only use ocv */
		regmap_update_bits(map, AXP22X_COULOMB_CTL, 0x80, 0x80);
		regmap_update_bits(map, AXP22X_COULOMB_CTL, 0x40, 0x00);
	}

	for (i = 0; i < ARRAY_SIZE(update_min_times); i++) {
		if (update_min_times[i] == axp_config->pmu_update_min_time)
			break;
	}
	regmap_update_bits(map, AXP22X_ADJUST_PARA, 0x07, i);

	axp22x_set_bat_chg_cur(bat_power, axp_config->pmu_runtime_chgcur);

	return 0;
}

static irqreturn_t axp22x_irq_handler_bat_stat_change(int irq, void *data)
{
	struct irq_desc *id = irq_to_desc(irq);
	struct axp22x_bat_power *bat_power = data;

	pr_debug("%s: enter interrupt %d\n", __func__, irq);

	power_supply_changed(bat_power->bat_supply);

	switch (id->irq_data.hwirq) {
	case AXP22X_IRQ_CHARG_DONE:
		pr_debug("interrupt:charger done");
		break;
	case AXP22X_IRQ_CHARG:
		pr_debug("interrutp:charger start");
		break;
	case AXP22X_IRQ_BATT_PLUGIN:
		pr_debug("interrupt:battery insert");
		break;
	case AXP22X_IRQ_BATT_REMOVAL:
		pr_debug("interrupt:battery remove");
		break;
	default:
		pr_debug("interrupt:others");
		break;
	}

	return IRQ_HANDLED;
}

enum axp22x_bat_power_virqs {
	AXP22X_BATT_PLUGIN,
	AXP22X_BATT_REMOVAL,
	AXP22X_CHARG,
	AXP22X_CHARG_DONE,
	AXP22X_LOW_PWR_LVL1,
	AXP22X_LOW_PWR_LVL2,
	AXP22X_BATT_TEMP_LOW,
	AXP22X_BATT_TEMP_HIGH,
	AXP22X_DIE_TEMP_HIGH,
	AXP22X_BAT_VIRQ_MAX_VIRQ,
};

static struct axp_interrupts axp22x_bat_irq[] = {
	[AXP22X_BATT_PLUGIN] = { "BATT_PLUGIN",
				  axp22x_irq_handler_bat_stat_change },
	[AXP22X_BATT_REMOVAL] = { "BATT_REMOVAL",
				   axp22x_irq_handler_bat_stat_change },
	[AXP22X_CHARG] = { "CHARG",
				    axp22x_irq_handler_bat_stat_change },
	[AXP22X_CHARG_DONE] = { "CHARG_DONE",
					axp22x_irq_handler_bat_stat_change },
	[AXP22X_LOW_PWR_LVL1] = { "LOW_PWR_LVL1",
					axp22x_irq_handler_bat_stat_change },
	[AXP22X_LOW_PWR_LVL2] = { "LOW_PWR_LVL2",
					axp22x_irq_handler_bat_stat_change },
	[AXP22X_BATT_TEMP_LOW] = { "BATT_TEMP_LOW",
					   axp22x_irq_handler_bat_stat_change },
	[AXP22X_BATT_TEMP_HIGH] = { "BATT_TEMP_HIGH",
					   axp22x_irq_handler_bat_stat_change },
	[AXP22X_DIE_TEMP_HIGH] = { "DIE_TEMP_HIGH",
					  axp22x_irq_handler_bat_stat_change },

};

static void axp22x_bat_power_monitor(struct work_struct *work)
{
	struct axp22x_bat_power *bat_power =
		container_of(work, typeof(*bat_power), bat_supply_mon.work);

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
}

static int axp22x_bat_power_dt_parse(struct axp22x_bat_power *bat_power)
{
	struct axp_config_info *axp_config = &bat_power->dts_info;
	struct device_node *node = bat_power->dev->of_node;

	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

	AXP_OF_PROP_READ(pmu_bat_unused,            		0);
	AXP_OF_PROP_READ(pmu_battery_rdc,              BATRDC);
	AXP_OF_PROP_READ(pmu_battery_cap,                4000);
	AXP_OF_PROP_READ(pmu_batdeten,                      1);
	AXP_OF_PROP_READ(pmu_chg_ic_temp,                   0);
	AXP_OF_PROP_READ(pmu_runtime_chgcur, INTCHGCUR / 1000);
	AXP_OF_PROP_READ(pmu_suspend_chgcur,             1200);
	AXP_OF_PROP_READ(pmu_shutdown_chgcur,            1200);
	AXP_OF_PROP_READ(pmu_init_chgvol,    INTCHGVOL / 1000);
	AXP_OF_PROP_READ(pmu_init_chgend_rate,  INTCHGENDRATE);
	AXP_OF_PROP_READ(pmu_init_chg_enabled,              1);
	AXP_OF_PROP_READ(pmu_init_bc_en,                    0);
	AXP_OF_PROP_READ(pmu_init_adc_freq,        INTADCFREQ);
	AXP_OF_PROP_READ(pmu_init_adcts_freq,     INTADCFREQC);
	AXP_OF_PROP_READ(pmu_init_chg_pretime,  INTCHGPRETIME);
	AXP_OF_PROP_READ(pmu_init_chg_csttime,  INTCHGCSTTIME);
	AXP_OF_PROP_READ(pmu_batt_cap_correct,              1);
	AXP_OF_PROP_READ(pmu_chg_end_on_en,                 0);
	AXP_OF_PROP_READ(ocv_coulumb_100,                   0);
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
	AXP_OF_PROP_READ(pmu_ac_vol,                     4400);
	AXP_OF_PROP_READ(pmu_usbpc_vol,                  4400);
	AXP_OF_PROP_READ(pmu_ac_cur,                        0);
	AXP_OF_PROP_READ(pmu_usbpc_cur,                     0);
	AXP_OF_PROP_READ(pmu_pwroff_vol,                 3300);
	AXP_OF_PROP_READ(pmu_pwron_vol,                  2900);
	AXP_OF_PROP_READ(pmu_battery_warning_level1,       15);
	AXP_OF_PROP_READ(pmu_battery_warning_level2,        0);
	AXP_OF_PROP_READ(pmu_restvol_adjust_time,          30);
	AXP_OF_PROP_READ(pmu_ocv_cou_adjust_time,          60);
	AXP_OF_PROP_READ(pmu_chgled_func,                   0);
	AXP_OF_PROP_READ(pmu_chgled_type,                   0);
	AXP_OF_PROP_READ(pmu_bat_temp_enable,               0);
	AXP_OF_PROP_READ(pmu_bat_charge_ltf,             0xA5);
	AXP_OF_PROP_READ(pmu_bat_charge_htf,             0x1F);
	AXP_OF_PROP_READ(pmu_bat_shutdown_ltf,           0xFC);
	AXP_OF_PROP_READ(pmu_bat_shutdown_htf,           0x16);
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
	AXP_OF_PROP_READ(pmu_bat_unused,                    0);
	AXP_OF_PROP_READ(power_start,                       0);
	AXP_OF_PROP_READ(pmu_ocv_en,                        1);
	AXP_OF_PROP_READ(pmu_cou_en,                        1);
	AXP_OF_PROP_READ(pmu_update_min_time,   UPDATEMINTIME);

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
	axp_config->wakeup_ovtemp_chg =
		of_property_read_bool(node, "wakeup_bat_ovtemp_chg");

	return 0;
}

static int axp22x_bat_power_probe(struct platform_device *pdev)
{
	struct axp20x_dev *axp_dev = dev_get_drvdata(pdev->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct device_node *node = pdev->dev.of_node;
	struct axp22x_bat_power *bat_power;
	int i, irq;
	int ret = 0;

	if (!of_device_is_available(node)) {
		pr_err("axp22x-battery device is not configed\n");
		return -ENODEV;
	}

	if (!axp_dev->irq) {
		pr_err("can not register axp22x bat without irq\n");
		return -EINVAL;
	}

	bat_power = devm_kzalloc(&pdev->dev, sizeof(*bat_power), GFP_KERNEL);
	if (!bat_power) {
		pr_err("axp22x bat power alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	bat_power->name = "axp22x-bat-power";
	bat_power->dev = &pdev->dev;
	bat_power->regmap = axp_dev->regmap;

	platform_set_drvdata(pdev, bat_power);

	ret = axp22x_bat_power_dt_parse(bat_power);
	if (ret) {
		pr_err("%s parse device tree err\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = axp22x_bat_power_init(bat_power);
	if (ret < 0) {
		pr_err("axp22x init bat power fail!\n");
		ret = -ENODEV;
		return ret;
	}

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = bat_power;

	bat_power->bat_supply = devm_power_supply_register(bat_power->dev,
			&axp22x_bat_desc, &psy_cfg);

	if (IS_ERR(bat_power->bat_supply)) {
		pr_err("axp22x failed to register usb power\n");
		ret = PTR_ERR(bat_power->bat_supply);
		return ret;
	}

	INIT_DELAYED_WORK(&bat_power->bat_supply_mon, axp22x_bat_power_monitor);


	for (i = 0; i < ARRAY_SIZE(axp22x_bat_irq); i++) {
		irq = platform_get_irq_byname(pdev, axp22x_bat_irq[i].name);
		if (irq < 0) {
			dev_warn(&pdev->dev, "No IRQ for %s: %d\n",
				 axp22x_bat_irq[i].name, irq);
			continue;
		}
		irq = regmap_irq_get_virq(axp_dev->regmap_irqc, irq);
		ret = devm_request_any_context_irq(&pdev->dev, irq,
						   axp22x_bat_irq[i].isr, 0,
						   axp22x_bat_irq[i].name, bat_power);
		if (ret < 0)
			dev_warn(&pdev->dev, "Error requesting %s IRQ %d: %d\n",
				axp22x_bat_irq[i].name, irq, ret);

		dev_dbg(&pdev->dev, "Requested %s IRQ %d: %d\n",
			axp22x_bat_irq[i].name, irq, ret);

		/* we use this variable to suspend irq */
		axp22x_bat_irq[i].irq = irq;
	}

	schedule_delayed_work(&bat_power->bat_supply_mon, msecs_to_jiffies(10 * 1000));
	return 0;
}

static int axp22x_bat_power_remove(struct platform_device *pdev)
{
	struct axp22x_bat_power *bat_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);

	return 0;
}

static inline void axp_irq_set(unsigned int irq, bool enable)
{
	if (enable)
		enable_irq(irq);
	else
		disable_irq(irq);
}

static void axp22x_bat_virq_dts_set(struct axp22x_bat_power *bat_power, bool enable)
{
	struct axp_config_info *dts_info = &bat_power->dts_info;

	if (!dts_info->wakeup_bat_in)
		axp_irq_set(axp22x_bat_irq[AXP22X_BATT_PLUGIN].irq,
				enable);
	if (!dts_info->wakeup_bat_out)
		axp_irq_set(axp22x_bat_irq[AXP22X_BATT_REMOVAL].irq,
				enable);
	if (!dts_info->wakeup_bat_charging)
		axp_irq_set(axp22x_bat_irq[AXP22X_CHARG].irq,
				enable);
	if (!dts_info->wakeup_bat_charge_over)
		axp_irq_set(axp22x_bat_irq[AXP22X_CHARG_DONE].irq,
				enable);
	if (!dts_info->wakeup_low_warning1)
		axp_irq_set(axp22x_bat_irq[AXP22X_LOW_PWR_LVL1].irq,
				enable);
	if (!dts_info->wakeup_low_warning2)
		axp_irq_set(axp22x_bat_irq[AXP22X_LOW_PWR_LVL2].irq,
				enable);
	if (!dts_info->wakeup_bat_untemp_work)
		axp_irq_set(
			axp22x_bat_irq[AXP22X_BATT_TEMP_LOW].irq,
			enable);
	if (!dts_info->wakeup_bat_ovtemp_work)
		axp_irq_set(
			axp22x_bat_irq[AXP22X_BATT_TEMP_HIGH].irq,
			enable);
	if (!dts_info->wakeup_ovtemp_chg)
		axp_irq_set(
			axp22x_bat_irq[AXP22X_DIE_TEMP_HIGH].irq,
			enable);

}

static void axp22x_bat_shutdown(struct platform_device *pdev)
{
	struct axp22x_bat_power *bat_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);
	axp22x_set_bat_chg_cur(bat_power, bat_power->dts_info.pmu_shutdown_chgcur);

}

static int axp22x_bat_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct axp22x_bat_power *bat_power = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&bat_power->bat_supply_mon);
	axp22x_set_bat_chg_cur(bat_power, bat_power->dts_info.pmu_suspend_chgcur);
	axp22x_bat_virq_dts_set(bat_power, false);

	return 0;
}

static int axp22x_bat_power_resume(struct platform_device *pdev)
{
	struct axp22x_bat_power *bat_power = platform_get_drvdata(pdev);

	schedule_delayed_work(&bat_power->bat_supply_mon, 0);
	axp22x_set_bat_chg_cur(bat_power, bat_power->dts_info.pmu_runtime_chgcur);
	axp22x_bat_virq_dts_set(bat_power, true);

	return 0;
}

static const struct of_device_id axp22x_bat_power_match[] = {
	{
		.compatible = "x-powers,axp22x-bat-power-supply",
		.data = (void *)AXP223_ID,
	},
	{
		.compatible = "x-powers,axp22x-bat-power-supply",
		.data = (void *)AXP221_ID,
	}, { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axp22x_bat_power_match);

static struct platform_driver axp22x_bat_power_driver = {
	.driver = {
		.name = "axp22x-bat-power-supply",
		.of_match_table = axp22x_bat_power_match,
	},
	.probe = axp22x_bat_power_probe,
	.remove = axp22x_bat_power_remove,
	.shutdown = axp22x_bat_shutdown,
	.suspend = axp22x_bat_power_suspend,
	.resume = axp22x_bat_power_resume,
};

module_platform_driver(axp22x_bat_power_driver);

MODULE_AUTHOR("wangxiaoliang <wangxiaoliang@x-powers.com>");
MODULE_DESCRIPTION("axp22x bat power driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
