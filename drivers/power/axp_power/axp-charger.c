#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of_device.h>
#include "axp-core.h"
#include "axp-charger.h"

static int axp_power_key;
static enum AW_CHARGE_TYPE axp_usbcurflag = CHARGE_AC;
static enum AW_CHARGE_TYPE axp_usbvolflag = CHARGE_AC;

static bool battery_initialized;

static DEFINE_SPINLOCK(axp_powerkey_lock);

void axp_powerkey_set(int value)
{
	spin_lock(&axp_powerkey_lock);
	axp_power_key = value;
	spin_unlock(&axp_powerkey_lock);
}
EXPORT_SYMBOL_GPL(axp_powerkey_set);

int axp_powerkey_get(void)
{
	int value;

	spin_lock(&axp_powerkey_lock);
	value = axp_power_key;
	spin_unlock(&axp_powerkey_lock);

	return value;
}
EXPORT_SYMBOL_GPL(axp_powerkey_get);

int axp_usbvol(enum AW_CHARGE_TYPE type)
{
	axp_usbvolflag = type;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbvol);

int axp_usbcur(enum AW_CHARGE_TYPE type)
{
	axp_usbcurflag = type;
	return 0;
}
EXPORT_SYMBOL_GPL(axp_usbcur);

void axp_charger_update_state(struct axp_charger_dev *chg_dev)
{
	u8 val;
	struct axp_ac_info *ac = chg_dev->spy_info->ac;
	struct axp_usb_info *usb = chg_dev->spy_info->usb;
	struct axp_battery_info *batt = chg_dev->spy_info->batt;
	struct axp_regmap *map = chg_dev->chip->regmap;

	axp_regmap_read(map, batt->det_offset, &val);
	spin_lock(&chg_dev->charger_lock);
	if (batt->det_unused == 0) {
		if (batt->det_valid == 1) {
			if ((val & (1 << batt->det_bit))
					&& (val & (1 << batt->det_valid_bit)))
				chg_dev->bat_det = 1;
			else
				chg_dev->bat_det = 0;
		} else if (batt->det_valid == 0) {
			chg_dev->bat_det = (val & 1 << batt->det_bit) ? 1 : 0;
		}
	} else if (batt->det_unused == 1) {
		chg_dev->bat_det = 0;
	}
	spin_unlock(&chg_dev->charger_lock);

	axp_regmap_read(map, ac->det_offset, &val);
	spin_lock(&chg_dev->charger_lock);
	chg_dev->ac_det = (val & 1 << ac->det_bit) ? 1 : 0;
	spin_unlock(&chg_dev->charger_lock);

	if (usb->det_unused == 0) {
		axp_regmap_read(map, usb->det_offset, &val);
		spin_lock(&chg_dev->charger_lock);
		chg_dev->usb_det = (val & 1 << usb->det_bit) ? 1 : 0;
		spin_unlock(&chg_dev->charger_lock);
	} else if (usb->det_unused == 1) {
		chg_dev->usb_det = 0;
	}

	axp_regmap_read(map, ac->valid_offset, &val);
	spin_lock(&chg_dev->charger_lock);
	chg_dev->ac_valid = (val & 1 << ac->valid_bit) ? 1 : 0;
	spin_unlock(&chg_dev->charger_lock);

	if (usb->det_unused == 0) {
		axp_regmap_read(map, usb->valid_offset, &val);
		spin_lock(&chg_dev->charger_lock);
		chg_dev->usb_valid = (val & 1 << usb->valid_bit) ? 1 : 0;
		spin_unlock(&chg_dev->charger_lock);
	} else if (usb->det_unused == 1) {
		chg_dev->usb_valid = 0;
	}

	chg_dev->ext_valid = (chg_dev->ac_det || chg_dev->usb_det);

	axp_regmap_read(map, ac->in_short_offset, &val);
	spin_lock(&chg_dev->charger_lock);
	chg_dev->in_short = (val & 1 << ac->in_short_bit) ? 1 : 0;
	if (!chg_dev->in_short)
		chg_dev->ac_charging = chg_dev->ac_valid;
	spin_unlock(&chg_dev->charger_lock);

	axp_regmap_read(map, batt->cur_direction_offset, &val);
	spin_lock(&chg_dev->charger_lock);
	if (val & 1 << batt->cur_direction_bit)
		chg_dev->bat_current_direction = 1;
	else
		chg_dev->bat_current_direction = 0;
	spin_unlock(&chg_dev->charger_lock);

	axp_regmap_read(map, batt->chgstat_offset, &val);
	spin_lock(&chg_dev->charger_lock);
	chg_dev->charging = (val & 1 << batt->chgstat_bit) ? 1 : 0;
	spin_unlock(&chg_dev->charger_lock);
}

void axp_charger_update_value(struct axp_charger_dev *chg_dev)
{
	struct axp_ac_info *ac = chg_dev->spy_info->ac;
	struct axp_usb_info *usb = chg_dev->spy_info->usb;
	struct axp_battery_info *batt = chg_dev->spy_info->batt;
	int bat_vol, bat_cur, bat_discur, ac_vol, ac_cur, usb_vol, usb_cur;

	bat_vol = batt->get_vbat(chg_dev);
	bat_cur = batt->get_ibat(chg_dev);
	bat_discur = batt->get_disibat(chg_dev);
	ac_vol  = ac->get_ac_voltage(chg_dev);
	ac_cur  = ac->get_ac_current(chg_dev);
	usb_vol = usb->get_usb_voltage(chg_dev);
	usb_cur = usb->get_usb_current(chg_dev);

	spin_lock(&chg_dev->charger_lock);
	chg_dev->bat_vol = bat_vol;
	chg_dev->bat_cur = bat_cur;
	chg_dev->bat_discur = bat_discur;
	chg_dev->ac_vol  = ac_vol;
	chg_dev->ac_cur  = ac_cur;
	chg_dev->usb_vol = usb_vol;
	chg_dev->usb_cur = usb_cur;
	spin_unlock(&chg_dev->charger_lock);
}

static void axp_usb_ac_check_status(struct axp_charger_dev *chg_dev)
{
	chg_dev->usb_pc_charging = (((CHARGE_USB_20 == axp_usbcurflag)
					|| (CHARGE_USB_30 == axp_usbcurflag))
					&& (chg_dev->ext_valid));
	chg_dev->usb_adapter_charging = ((0 == chg_dev->ac_valid)
					&& (CHARGE_USB_20 != axp_usbcurflag)
					&& (CHARGE_USB_30 != axp_usbcurflag)
					&& (chg_dev->ext_valid));

	if (chg_dev->in_short)
		chg_dev->ac_charging = ((chg_dev->usb_adapter_charging == 0)
					&& (chg_dev->usb_pc_charging == 0)
					&& (chg_dev->ext_valid));
	else
		chg_dev->ac_charging = chg_dev->ac_valid;

	power_supply_changed(&chg_dev->ac);
	power_supply_changed(&chg_dev->usb);

	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
			"ac_charging=%d\n", chg_dev->ac_charging);
	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
			"usb_pc_charging=%d\n", chg_dev->usb_pc_charging);
	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
			"usb_adapter_charging=%d\n",
			chg_dev->usb_adapter_charging);
	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
			"usb_det=%d ac_det=%d\n",
			chg_dev->usb_det, chg_dev->ac_det);
}

static void axp_charger_update_usb_state(unsigned long data)
{
	struct axp_charger_dev *chg_dev = (struct axp_charger_dev *)data;

	axp_usb_ac_check_status(chg_dev);

	if (chg_dev->bat_det)
		schedule_delayed_work(&(chg_dev->usbwork), 0);
}

static void axp_usb(struct work_struct *work)
{
	struct axp_charger_dev *chg_dev = container_of(work,
					struct axp_charger_dev, usbwork.work);
	struct axp_usb_info *usb = chg_dev->spy_info->usb;
	struct axp_ac_info *ac = chg_dev->spy_info->ac;

	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
				"[axp_usb] axp_usbcurflag = %d\n",
				axp_usbcurflag);

	axp_charger_update_state(chg_dev);

	if (chg_dev->in_short) {
		/* usb and ac in short*/
		if (!chg_dev->usb_valid) {
			/*usb or usb adapter can not be used*/
			AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
				"USB not insert!\n");
			usb->set_usb_ihold(chg_dev, 500);
		} else if (CHARGE_USB_20 == axp_usbcurflag) {
			if (usb->usb_pc_cur) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_cur %d mA\n",
						usb->usb_pc_cur);
				usb->set_usb_ihold(chg_dev, usb->usb_pc_cur);
			} else {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_cur 500 mA\n");
				usb->set_usb_ihold(chg_dev, 500);
			}
		} else if (CHARGE_USB_30 == axp_usbcurflag) {
			AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_cur 900 mA\n");
			usb->set_usb_ihold(chg_dev, 900);
		} else {
			/* usb adapter */
			if (usb->usb_ad_cur) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_ad_cur %d mA\n",
							usb->usb_ad_cur);
				usb->set_usb_ihold(chg_dev, usb->usb_ad_cur);
			} else {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_ad_cur 2500 mA\n");
				usb->set_usb_ihold(chg_dev, 2500);
			}
		}

		if (CHARGE_USB_20 == axp_usbvolflag) {
			if (usb->usb_pc_vol) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_vol %d mV\n",
							usb->usb_pc_vol);
				usb->set_usb_vhold(chg_dev, usb->usb_pc_vol);
			}
		} else if (CHARGE_USB_30 == axp_usbvolflag) {
			AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_vol 4700 mV\n");
			usb->set_usb_vhold(chg_dev, 4700);
		} else {
			if (usb->usb_ad_vol) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_ad_vol %d mV\n",
							usb->usb_ad_vol);
				usb->set_usb_vhold(chg_dev, usb->usb_ad_vol);
			}
		}
	} else {
		if (!chg_dev->ac_valid && !chg_dev->usb_valid) {
			/*usb and ac can not be used*/
			AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"AC and USB not insert!\n");
			usb->set_usb_ihold(chg_dev, 500);
		} else if (CHARGE_USB_20 == axp_usbcurflag) {
			if (usb->usb_pc_cur) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_cur %d mA\n",
							usb->usb_pc_cur);
				usb->set_usb_ihold(chg_dev, usb->usb_pc_cur);
			} else {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_cur 500 mA\n");
				usb->set_usb_ihold(chg_dev, 500);
			}
		} else if (CHARGE_USB_30 == axp_usbcurflag) {
			AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_cur 900 mA\n");
			usb->set_usb_ihold(chg_dev, 900);
		} else {
			if (chg_dev->usb_adapter_charging) {
				if ((usb->usb_ad_cur)) {
					AXP_DEBUG(AXP_CHG,
						chg_dev->chip->pmu_num,
						"set adapter cur %d mA\n",
						usb->usb_ad_cur);
					usb->set_usb_ihold(chg_dev,
						usb->usb_ad_cur);
				} else {
					AXP_DEBUG(AXP_CHG,
						chg_dev->chip->pmu_num,
						"set adapter cur 2500 mA\n");
					usb->set_usb_ihold(chg_dev, 2500);
				}
			}
		}

		if (CHARGE_USB_20 == axp_usbvolflag) {
			if (usb->usb_pc_vol) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_vol %d mV\n",
							usb->usb_pc_vol);
				usb->set_usb_vhold(chg_dev, usb->usb_pc_vol);
			}
		} else if (CHARGE_USB_30 == axp_usbvolflag) {
			AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set usb_pc_vol 4700 mV\n");
			usb->set_usb_vhold(chg_dev, 4700);
		} else {
			if (ac->ac_vol) {
				AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num,
						"set ac_vol %d mV\n",
							ac->ac_vol);
				ac->set_ac_vhold(chg_dev, ac->ac_vol);
			}
		}
	}
}

void axp_battery_update_vol(struct axp_charger_dev *chg_dev)
{
	s32 rest_vol = 0;
	struct axp_battery_info *batt = chg_dev->spy_info->batt;

	rest_vol = batt->get_rest_cap(chg_dev);

	spin_lock(&chg_dev->charger_lock);
	if (rest_vol > 100) {
		AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"AXP rest_vol = %d\n", rest_vol);
		chg_dev->rest_vol = 100;
	} else {
		chg_dev->rest_vol = rest_vol;
	}
	spin_unlock(&chg_dev->charger_lock);

	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->rest_vol = %d\n", chg_dev->rest_vol);
}

static enum power_supply_property axp_battery_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property axp_ac_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static enum power_supply_property axp_usb_props[] = {
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

static void axp_battery_check_status(struct axp_charger_dev *chg_dev,
					union power_supply_propval *val)
{
	if (chg_dev->bat_det) {
		if (chg_dev->ext_valid) {
			if (chg_dev->rest_vol == 100)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if (chg_dev->charging)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else {
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		}
	} else {
		val->intval = POWER_SUPPLY_STATUS_FULL;
	}
}

static void axp_battery_check_health(struct axp_charger_dev *chg_dev,
					union power_supply_propval *val)
{
	struct axp_battery_info *batt = chg_dev->spy_info->batt;
	val->intval = batt->get_bat_health(chg_dev);
}

static s32 axp_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct axp_charger_dev *chg_dev = container_of(psy,
					struct axp_charger_dev, batt);
	s32 ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		axp_battery_check_status(chg_dev, val);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		axp_battery_check_health(chg_dev, val);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = chg_dev->battery_info->technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = chg_dev->battery_info->voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = chg_dev->battery_info->voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chg_dev->bat_vol * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = (chg_dev->bat_cur - chg_dev->bat_discur) * 1000;
		break;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->name;
		break;
	case POWER_SUPPLY_PROP_ENERGY_FULL_DESIGN:
		val->intval = chg_dev->battery_info->energy_full_design;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chg_dev->rest_vol;
		break;
	case POWER_SUPPLY_PROP_ONLINE: {
		/* in order to get hardware state,
		 * we must update charger state now.
		 * by sunny at 2012-12-23 11:06:15.
		 */
		axp_charger_update_state(chg_dev);
		val->intval = !chg_dev->bat_current_direction;
		break;
	}
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = chg_dev->bat_det;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = 30;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static s32 axp_ac_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct axp_charger_dev *chg_dev = container_of(psy,
					struct axp_charger_dev, ac);
	s32 ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (chg_dev->ac_charging
					|| chg_dev->usb_adapter_charging);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chg_dev->ac_vol * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = chg_dev->ac_cur * 1000;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static s32 axp_usb_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct axp_charger_dev *chg_dev = container_of(psy,
					struct axp_charger_dev, usb);
	s32 ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = psy->name;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chg_dev->usb_pc_charging;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chg_dev->usb_vol * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = chg_dev->usb_cur * 1000;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static char *supply_list[] = {
	"battery",
};

static void axp_battery_setup_psy(struct axp_charger_dev *chg_dev)
{
	struct power_supply *batt = &chg_dev->batt;
	struct power_supply *ac = &chg_dev->ac;
	struct power_supply *usb = &chg_dev->usb;
	struct power_supply_info *info = chg_dev->battery_info;

	batt->name = "battery";
	batt->use_for_apm = info->use_for_apm;
	batt->type = POWER_SUPPLY_TYPE_BATTERY;
	batt->get_property = axp_battery_get_property;
	batt->properties = axp_battery_props;
	batt->num_properties = ARRAY_SIZE(axp_battery_props);

	ac->name = "ac";
	ac->type = POWER_SUPPLY_TYPE_MAINS;
	ac->get_property = axp_ac_get_property;
	ac->supplied_to = supply_list,
	ac->num_supplicants = ARRAY_SIZE(supply_list),
	ac->properties = axp_ac_props;
	ac->num_properties = ARRAY_SIZE(axp_ac_props);

	usb->name = "usb";
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->get_property = axp_usb_get_property;
	usb->supplied_to = supply_list,
	usb->num_supplicants = ARRAY_SIZE(supply_list),
	usb->properties = axp_usb_props;
	usb->num_properties = ARRAY_SIZE(axp_usb_props);
}

static void axp_charging_monitor(struct work_struct *work)
{
	struct axp_charger_dev *chg_dev = container_of(work,
					struct axp_charger_dev, work.work);
	static s32 pre_rest_vol;
	static bool pre_bat_curr_dir;

	axp_charger_update_state(chg_dev);
	axp_charger_update_value(chg_dev);

	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->bat_vol = %d\n", chg_dev->bat_vol);
	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->bat_cur = %d\n", chg_dev->bat_cur);
	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->bat_discur = %d\n", chg_dev->bat_discur);
	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->is_charging = %d\n", chg_dev->charging);
	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->bat_current_direction = %d\n",
			chg_dev->bat_current_direction);
	AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
			"charger->ext_valid = %d\n", chg_dev->ext_valid);

	if (chg_dev->private_debug)
		chg_dev->private_debug(chg_dev);

	axp_battery_update_vol(chg_dev);

	/* if battery volume changed, inform uevent */
	if ((chg_dev->rest_vol - pre_rest_vol)
			|| (chg_dev->bat_current_direction != pre_bat_curr_dir)
		) {
		AXP_DEBUG(AXP_SPLY, chg_dev->chip->pmu_num,
				"battery vol change: %d->%d\n",
				pre_rest_vol, chg_dev->rest_vol);
		pre_rest_vol = chg_dev->rest_vol;
		pre_bat_curr_dir = chg_dev->bat_current_direction;
		power_supply_changed(&chg_dev->batt);
	}

	/* reschedule for the next time */
	schedule_delayed_work(&chg_dev->work, chg_dev->interval);
}

void axp_change(struct axp_charger_dev *chg_dev)
{
	AXP_DEBUG(AXP_INT, chg_dev->chip->pmu_num, "battery state change\n");
	axp_charger_update_state(chg_dev);
	axp_charger_update_value(chg_dev);
	if (chg_dev->bat_det && battery_initialized)
		power_supply_changed(&chg_dev->batt);
}
EXPORT_SYMBOL_GPL(axp_change);

void axp_usbac_in(struct axp_charger_dev *chg_dev)
{
	struct axp_usb_info *usb = chg_dev->spy_info->usb;

	axp_usbcur(CHARGE_AC);
	axp_usbvol(CHARGE_AC);

	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num, "axp ac/usb in!\n");

	if (timer_pending(&chg_dev->usb_status_timer))
		del_timer_sync(&chg_dev->usb_status_timer);

	/* must limit the current now,
	 * and will again fix it while usb/ac detect finished!
	*/
	if (usb->usb_pc_cur)
		usb->set_usb_ihold(chg_dev, usb->usb_pc_cur);
	else
		usb->set_usb_ihold(chg_dev, 500);

	/* this is about 3.5s,
	* while the flag set in usb drivers after usb plugged
	*/
	mod_timer(&chg_dev->usb_status_timer,
				jiffies + msecs_to_jiffies(5000));
	axp_usb_ac_check_status(chg_dev);
}
EXPORT_SYMBOL_GPL(axp_usbac_in);

void axp_usbac_out(struct axp_charger_dev *chg_dev)
{
	AXP_DEBUG(AXP_CHG, chg_dev->chip->pmu_num, "axp ac/usb out!\n");

	if (timer_pending(&chg_dev->usb_status_timer))
		del_timer_sync(&chg_dev->usb_status_timer);

	/* if we plugged usb & ac at the same time,
	 * then unpluged ac quickly while the usb driver
	 * do not finished detecting,
	 * the charger type is error!So delay the charger type report 2s
	*/
	mod_timer(&chg_dev->usb_status_timer,
					jiffies + msecs_to_jiffies(2000));
	axp_usb_ac_check_status(chg_dev);
}
EXPORT_SYMBOL_GPL(axp_usbac_out);

void axp_capchange(struct axp_charger_dev *chg_dev)
{
	AXP_DEBUG(AXP_INT, chg_dev->chip->pmu_num, "battery change\n");

	axp_charger_update_state(chg_dev);
	axp_charger_update_value(chg_dev);
	axp_battery_update_vol(chg_dev);

	if (chg_dev->bat_det) {
		AXP_DEBUG(AXP_INT, chg_dev->chip->pmu_num, "rest_vol = %d\n",
				chg_dev->rest_vol);
		if (!battery_initialized) {
			power_supply_register(chg_dev->dev, &chg_dev->batt);
			schedule_delayed_work(&chg_dev->usbwork, 0);
			schedule_delayed_work(&chg_dev->work, 0);
			power_supply_changed(&chg_dev->batt);
			battery_initialized = true;
		}
	} else {
		if (battery_initialized) {
			cancel_delayed_work_sync(&chg_dev->work);
			cancel_delayed_work_sync(&chg_dev->usbwork);
			power_supply_unregister(&chg_dev->batt);
			battery_initialized = false;
		}
	}
}
EXPORT_SYMBOL_GPL(axp_capchange);

void axp_charger_suspend(struct axp_charger_dev *chg_dev)
{
	struct axp_battery_info *batt = chg_dev->spy_info->batt;

	axp_charger_update_state(chg_dev);

	if (chg_dev->bat_det) {
		schedule_delayed_work(&chg_dev->usbwork, 0);
		flush_delayed_work(&chg_dev->usbwork);
		cancel_delayed_work_sync(&chg_dev->work);
		cancel_delayed_work_sync(&chg_dev->usbwork);

		batt->set_chg_cur(chg_dev, batt->suspend_chgcur);
	}
}
EXPORT_SYMBOL_GPL(axp_charger_suspend);

void axp_charger_resume(struct axp_charger_dev *chg_dev)
{
	struct axp_battery_info *batt = chg_dev->spy_info->batt;

	axp_charger_update_state(chg_dev);
	axp_charger_update_value(chg_dev);
	axp_battery_update_vol(chg_dev);

	batt->set_chg_cur(chg_dev, batt->runtime_chgcur);

	power_supply_changed(&chg_dev->ac);
	power_supply_changed(&chg_dev->usb);

	if (chg_dev->bat_det) {
		power_supply_changed(&chg_dev->batt);
		schedule_delayed_work(&chg_dev->work, chg_dev->interval);
		schedule_delayed_work(&chg_dev->usbwork,
					msecs_to_jiffies(7 * 1000));
	}
}
EXPORT_SYMBOL_GPL(axp_charger_resume);

void axp_charger_shutdown(struct axp_charger_dev *chg_dev)
{
	struct axp_battery_info *batt = chg_dev->spy_info->batt;
	cancel_delayed_work_sync(&chg_dev->work);
	batt->set_chg_cur(chg_dev, batt->shutdown_chgcur);
}
EXPORT_SYMBOL_GPL(axp_charger_shutdown);

struct axp_charger_dev *axp_power_supply_register(struct device *dev,
					struct axp_dev *axp_dev,
					struct power_supply_info *battery_info,
					struct axp_supply_info *info)
{
	int ret;
	struct axp_charger_dev *chg_dev;

	chg_dev = devm_kzalloc(dev, sizeof(*chg_dev), GFP_KERNEL);
	if (chg_dev == NULL)
		return NULL;

	chg_dev->dev = dev;
	chg_dev->spy_info = info;
	chg_dev->chip = axp_dev;
	chg_dev->battery_info = battery_info;

	axp_battery_setup_psy(chg_dev);

	axp_charger_update_state(chg_dev);
	if (chg_dev->bat_det) {
		ret = power_supply_register(dev, &chg_dev->batt);
		if (ret)
			goto err_ps_register;
		battery_initialized = true;
	}

	ret = power_supply_register(dev, &chg_dev->ac);
	if (ret) {
		if (chg_dev->bat_det) {
			power_supply_unregister(&chg_dev->batt);
			goto err_ps_register;
		}
	}

	ret = power_supply_register(dev, &chg_dev->usb);
	if (ret) {
		power_supply_unregister(&chg_dev->ac);
		if (chg_dev->bat_det) {
			power_supply_unregister(&chg_dev->batt);
			goto err_ps_register;
		}
	}

	spin_lock_init(&chg_dev->charger_lock);

	if (info->ac->ac_vol && info->ac->set_ac_vhold)
		info->ac->set_ac_vhold(chg_dev, info->ac->ac_vol);

	if (info->usb->usb_pc_vol && info->usb->set_usb_vhold)
		info->usb->set_usb_vhold(chg_dev, info->usb->usb_pc_vol);

	if (info->batt->runtime_chgcur && info->batt->set_chg_cur)
		info->batt->set_chg_cur(chg_dev, info->batt->runtime_chgcur);

	setup_timer(&chg_dev->usb_status_timer,
			axp_charger_update_usb_state, (unsigned long)chg_dev);
	INIT_DELAYED_WORK(&(chg_dev->usbwork), axp_usb);

	axp_usb_ac_check_status(chg_dev);
	axp_battery_update_vol(chg_dev);

	spin_lock(&chg_dev->charger_lock);
	chg_dev->interval = msecs_to_jiffies(10 * 1000);
	spin_unlock(&chg_dev->charger_lock);

	INIT_DELAYED_WORK(&chg_dev->work, axp_charging_monitor);

	if (chg_dev->bat_det) {
		schedule_delayed_work(&chg_dev->work, chg_dev->interval);
		schedule_delayed_work(&(chg_dev->usbwork),
					msecs_to_jiffies(30 * 1000));
	}

	return chg_dev;

err_ps_register:
	return NULL;
}
EXPORT_SYMBOL_GPL(axp_power_supply_register);

void axp_power_supply_unregister(struct axp_charger_dev *chg_dev)
{
	del_timer_sync(&chg_dev->usb_status_timer);
	power_supply_unregister(&chg_dev->usb);
	power_supply_unregister(&chg_dev->ac);

	if (chg_dev->bat_det && battery_initialized) {
		cancel_delayed_work_sync(&chg_dev->work);
		cancel_delayed_work_sync(&chg_dev->usbwork);
		power_supply_unregister(&chg_dev->batt);
		battery_initialized = false;
	}
}
EXPORT_SYMBOL_GPL(axp_power_supply_unregister);

int axp_charger_dt_parse(struct device_node *node,
			struct axp_config_info *axp_config)
{
	if (!of_device_is_available(node)) {
		pr_err("%s: failed\n", __func__);
		return -1;
	}

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

	return 0;
}
EXPORT_SYMBOL_GPL(axp_charger_dt_parse);
