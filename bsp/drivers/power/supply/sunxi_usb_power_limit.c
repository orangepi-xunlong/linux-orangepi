/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#define pr_fmt(x) KBUILD_MODNAME ": " x "\n"

#include "sunxi_usb_power_limit.h"

/*------------------------------
 * check supply online status
 *------------------------------
 */

static bool sunxi_usb_power_limit_check_supply_online(struct sunxi_usb_power_limit_supply_data *power_limit,
							enum sunxi_supply_list_id list_id)
{
	return sunxi_power_supply_check_online(power_limit->power_psy, list_id, SUNXI_SUPPLY_LIST_MAX);
}

static bool sunxi_usb_power_limit_is_usb_power_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	return sunxi_usb_power_limit_check_supply_online(power_limit, SUNXI_SUPPLY_LIST_USB_POWER);
}

static bool sunxi_usb_power_limit_is_tcpc_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	return sunxi_usb_power_limit_check_supply_online(power_limit, SUNXI_SUPPLY_LIST_TCPC);
}

static bool sunxi_usb_power_limit_is_multi_charge_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	return sunxi_usb_power_limit_check_supply_online(power_limit, SUNXI_SUPPLY_LIST_MULTI_CHARGE);
}

static bool sunxi_usb_power_limit_is_battery_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	if (power_limit->power_psy[SUNXI_SUPPLY_LIST_BATTERY].node_status == SUNXI_NODE_NOT_EXIST)
		return true;

	return sunxi_usb_power_limit_check_supply_online(power_limit, SUNXI_SUPPLY_LIST_BATTERY);
}

/*------------------------------
 * check extcon online status
 *------------------------------
 */

static bool sunxi_usb_power_limit_check_extcon_online(struct sunxi_usb_power_limit_supply_data *power_limit,
							enum sunxi_extcon_list_id list_id)
{
	return sunxi_power_supply_check_extcon_online(power_limit->extcon, list_id, SUNXI_EXTCON_LIST_MAX);
}

static bool sunxi_usb_power_limit_extcon_is_usb_sdp_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	return sunxi_usb_power_limit_check_extcon_online(power_limit, SUNXI_EXTCON_LIST_USB_UDC);
}

static bool sunxi_usb_power_limit_extcon_is_usb_phy_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	return sunxi_usb_power_limit_check_extcon_online(power_limit, SUNXI_EXTCON_LIST_USB_PHY);
}

static bool sunxi_usb_power_limit_extcon_is_usb_pc_online(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	return sunxi_usb_power_limit_extcon_is_usb_sdp_online(power_limit) || sunxi_usb_power_limit_extcon_is_usb_phy_online(power_limit);
}

/*---------------------------------------------
 * get psy property
 *---------------------------------------------
 */

static int _sunxi_usb_power_limit_get_input_limit(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	struct power_supply *psy = power_limit->power_psy[SUNXI_SUPPLY_LIST_USB_POWER].psy;
	union power_supply_propval temp;
	int ret = 0;

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);
	if (ret < 0)
		return 0;

	return temp.intval;
}

static void _sunxi_usb_power_limit_get_tcpc_paras(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	struct power_supply *psy = power_limit->power_psy[SUNXI_SUPPLY_LIST_TCPC].psy;
	union power_supply_propval temp;
	int ret = 0;

	if (IS_ERR_OR_NULL(psy))
		return;

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &temp);
	if (ret < 0)
		temp.intval = 5000 * 1000;

	power_limit->tcpc_limit.input_vol = temp.intval / 1000;

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_CURRENT_NOW, &temp);
	if (ret < 0)
		temp.intval = power_limit->node_info.pmu_usbpc_cur * 1000;

	power_limit->tcpc_limit.input_cur = temp.intval / 1000;
}

/*---------------------------------------------
 * set psy property
 *---------------------------------------------
 */

static int __sunxi_usb_power_limit_set_input_limit(struct sunxi_usb_power_limit_supply_data *power_limit, int input_limit)
{
	struct power_supply *psy = power_limit->power_psy[SUNXI_SUPPLY_LIST_USB_POWER].psy;
	union power_supply_propval temp;
	int ret = 0;

	temp.intval = input_limit;
	ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &temp);

	return ret;
}

static int _sunxi_usb_power_limit_set_input_limit(struct sunxi_usb_power_limit_supply_data *power_limit, int input_limit)
{
	int ret = 0;

	if (!sunxi_usb_power_limit_is_battery_online(power_limit))
		return ret;

	ret = __sunxi_usb_power_limit_set_input_limit(power_limit, input_limit);

	return ret;
}

static int _sunxi_usb_power_limit_set_input_vol(struct sunxi_usb_power_limit_supply_data *power_limit,
				  int input_vol)
{
	struct power_supply *psy = power_limit->power_psy[SUNXI_SUPPLY_LIST_USB_POWER].psy;
	union power_supply_propval temp;
	int ret = 0;

	temp.intval = input_vol;
	ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &temp);

	return ret;
}

/*------------------------------
 * limit type
 *------------------------------
 */

static void _sunxi_usb_power_limit_set_input_limit_type(struct sunxi_usb_power_limit_supply_data *power_limit,
							enum sunxi_power_input_limit_type input_limit_type,
							bool status)
{
	int val = atomic_read(&power_limit->input_limit_type);

	if (status)
		val |= 1 << input_limit_type;
	else
		val &= ~(1 << input_limit_type);

	atomic_set(&power_limit->input_limit_type, val);
}

static enum sunxi_power_input_limit_type _sunxi_usb_power_limit_get_input_limit_type(int limit_staus)
{
	enum sunxi_power_input_limit_type limit_type = SUNXI_POWER_INPUT_LIMIT_UNKNOWN;
	int type;

	for (type = (SUNXI_POWER_INPUT_LIMIT_MAX - 1); type >= 0; type--) {
		if (((limit_staus >> type) & 0x1) && (limit_type == SUNXI_POWER_INPUT_LIMIT_UNKNOWN)) {
			limit_type = type;
		}
	}

	return limit_type;
}

static void _sunxi_usb_power_limit_clear_input_limit_type(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	if (!sunxi_usb_power_limit_extcon_is_usb_pc_online(power_limit))
		_sunxi_usb_power_limit_set_input_limit_type(power_limit, SUNXI_POWER_INPUT_LIMIT_PC, false);
	if (!sunxi_usb_power_limit_is_multi_charge_online(power_limit))
		_sunxi_usb_power_limit_set_input_limit_type(power_limit, SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE, false);

	_sunxi_usb_power_limit_set_input_limit_type(power_limit, SUNXI_POWER_INPUT_LIMIT_OTHERS, false);
	_sunxi_usb_power_limit_set_input_limit_type(power_limit, SUNXI_POWER_INPUT_LIMIT_PD_CHARGE, false);
	_sunxi_usb_power_limit_set_input_limit_type(power_limit, SUNXI_POWER_INPUT_LIMIT_TCPC, false);
	_sunxi_usb_power_limit_set_input_limit_type(power_limit, SUNXI_POWER_INPUT_LIMIT_DEFAULT, false);
}

/*------------------------------
 * self input limit
 *------------------------------
 */

static bool sunxi_usb_power_limit_self_limit_input_check(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	struct sunxi_usb_power_limit_config_info *power_limit_config = &power_limit->node_info;

	if (power_limit_config->pmu_bc12_en)
		return false;

	if (power_limit->power_psy[SUNXI_SUPPLY_LIST_TCPC].node_status == SUNXI_NODE_ENABLED)
		return false;

	return true;
}

static void sunxi_usb_power_limit_self_limit_input(struct sunxi_usb_power_limit_supply_data *power_limit, int input_limit)
{
	if (!sunxi_usb_power_limit_self_limit_input_check(power_limit))
		return;

	_sunxi_usb_power_limit_set_input_limit(power_limit, input_limit);
}

static void sunxi_usb_power_limit_reset_default_settings(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	_sunxi_usb_power_limit_set_input_vol(power_limit, 5000);

	_sunxi_usb_power_limit_clear_input_limit_type(power_limit);

	if (!sunxi_usb_power_limit_is_multi_charge_online(power_limit))
		_sunxi_usb_power_limit_set_input_limit(power_limit, power_limit->node_info.pmu_usbpc_cur);
}

static void sunxi_usb_power_limit_vbus_online_process(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	sunxi_usb_power_limit_self_limit_input(power_limit, power_limit->node_info.pmu_usbpc_cur);

	cancel_delayed_work_sync(&power_limit->usb_power_self_managed_chg);
	__pm_stay_awake(power_limit->usb_power_self_managed_chg_wakeup);

	schedule_delayed_work(&power_limit->usb_power_self_managed_chg, msecs_to_jiffies(5 * 1000));
}

static void sunxi_usb_power_limit_vbus_offline_process(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	cancel_delayed_work_sync(&power_limit->usb_power_self_managed_chg);
	__pm_relax(power_limit->usb_power_self_managed_chg_wakeup);

	sunxi_usb_power_limit_reset_default_settings(power_limit);
}

/*------------------------------
 * input limit process
 *------------------------------
 */

/**
 * sunxi_usb_power_limit_input_limit_multi_charge_process - Process input current limit in multi-charge mode
 * @power_limit: Pointer to USB power supply data structure
 *
 * This function ensures using the preset current limit (pmu_usbad_cur) in multi-charge mode,
 * and sets USB type to DCP (Dedicated Charging Port)
 */

static void sunxi_usb_power_limit_input_limit_multi_charge_process(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	struct sunxi_usb_power_limit_config_info *power_limit_config = &power_limit->node_info;
	int limit_cur = _sunxi_usb_power_limit_get_input_limit(power_limit);

	if (limit_cur != power_limit_config->pmu_usbad_cur) {
		limit_cur = power_limit_config->pmu_usbad_cur;
		_sunxi_usb_power_limit_set_input_limit(power_limit, limit_cur);
	}
}

/**
 * sunxi_usb_power_limit_input_limit_pc_process - Process input current limit in PC/SDP mode
 * @power_limit: Pointer to USB power supply data structure
 *
 * This function ensures using the preset current limit (pmu_usbpc_cur) in PC/SDP mode,
 * and sets USB type to SDP (Standard Downstream Port)
 */

static void sunxi_usb_power_limit_input_limit_pc_process(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	struct sunxi_usb_power_limit_config_info *power_limit_config = &power_limit->node_info;
	int limit_cur = _sunxi_usb_power_limit_get_input_limit(power_limit);

	if (limit_cur != power_limit_config->pmu_usbpc_cur) {
		limit_cur = power_limit_config->pmu_usbpc_cur;
		_sunxi_usb_power_limit_set_input_limit(power_limit, limit_cur);
	}
}

/**
 * sunxi_usb_power_limit_input_limit_tcpc_process - Process input limit in Type-C PD mode
 * @power_limit: Pointer to USB power supply data structure
 *
 * This function sets input limits according to Type-C PD negotiated voltage and current,
 * and automatically sets USB type (PD or Type-C) based on voltage value
 */

static void sunxi_usb_power_limit_input_limit_tcpc_process(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	_sunxi_usb_power_limit_set_input_vol(power_limit, power_limit->tcpc_limit.input_vol);
	_sunxi_usb_power_limit_set_input_limit(power_limit, power_limit->tcpc_limit.input_cur);
}

/**
 * sunxi_usb_power_limit_input_limit_others_process - Process input current limit from other sources
 * @power_limit: Pointer to USB power supply data structure
 * @limit_cur_new: New current limit value (mA)
 *
 * This function applies current limit from other sources and sets USB type to DCP.
 * If no new value is provided, keeps the current limit unchanged.
 */

static void sunxi_usb_power_limit_input_limit_others_process(struct sunxi_usb_power_limit_supply_data *power_limit, int limit_cur_new)
{
	int limit_cur = _sunxi_usb_power_limit_get_input_limit(power_limit);

	if (!limit_cur_new)
		limit_cur_new = limit_cur;

	_sunxi_usb_power_limit_set_input_limit(power_limit, limit_cur_new);
}

/**
 * sunxi_usb_power_limit_input_limit_default_process - Restore default input limit settings
 * @power_limit: Pointer to USB power supply data structure
 *
 * This function resets system to default state:
 * 1. Clears current limit flag
 * 2. Sets default voltage (5000mV)
 * 3. Uses preset default current limit (pmu_usbpc_cur)
 * 4. Sets USB type to DCP
 * 5. Clears all input limit type flags
 */

static void sunxi_usb_power_limit_input_limit_default_process(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	_sunxi_usb_power_limit_set_input_vol(power_limit, 5000);
	_sunxi_usb_power_limit_set_input_limit(power_limit, power_limit->node_info.pmu_usbad_cur);
}

/**
 * _sunxi_usb_power_limit_input_limit_process - Execute corresponding process based on input limit type
 * @power_limit: Pointer to USB power supply data structure
 * @input_limit_type: Input limit type
 * @limit_cur: Current limit value (mA)
 *
 * This function calls corresponding processing function based on input limit type,
 * and logs the setting result after processing
 */

static void _sunxi_usb_power_limit_input_limit_process(struct sunxi_usb_power_limit_supply_data *power_limit,
						enum sunxi_power_input_limit_type input_limit_type,
						int limit_cur)
{
	switch (input_limit_type) {
	case SUNXI_POWER_INPUT_LIMIT_PD_CHARGE:
		sunxi_usb_power_limit_input_limit_tcpc_process(power_limit);
		break;
	case SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE:
		sunxi_usb_power_limit_input_limit_multi_charge_process(power_limit);
		break;
	case SUNXI_POWER_INPUT_LIMIT_PC:
		sunxi_usb_power_limit_input_limit_pc_process(power_limit);
		break;
	case SUNXI_POWER_INPUT_LIMIT_TCPC:
		sunxi_usb_power_limit_input_limit_tcpc_process(power_limit);
		break;
	case SUNXI_POWER_INPUT_LIMIT_OTHERS:
		sunxi_usb_power_limit_input_limit_others_process(power_limit, limit_cur);
		break;
	case SUNXI_POWER_INPUT_LIMIT_DEFAULT:
		sunxi_usb_power_limit_input_limit_default_process(power_limit);
		break;
	case SUNXI_POWER_INPUT_LIMIT_UNKNOWN:
		sunxi_usb_power_limit_reset_default_settings(power_limit);
		break;
	default:
		break;
	}

	limit_cur = _sunxi_usb_power_limit_get_input_limit(power_limit);

	PMIC_INFO("current limit setted : %s, limit_cur: %d mA\n",
			SUNXI_SUPPLY_INPUT_LIMIT_TYPE_TEXT[input_limit_type], limit_cur);
	SUNXI_POWER_LOG_INFO(power_limit->debug, "current limit setted : %s, limit_cur: %d mA",
			SUNXI_SUPPLY_INPUT_LIMIT_TYPE_TEXT[input_limit_type], limit_cur);
}

/**
 * sunxi_usb_power_limit_input_limit_process - Process and apply input current limit changes
 * @power_limit: Pointer to USB power supply data structure
 * @input_limit_type: Type of input limit to process (from enum sunxi_power_input_limit_type)
 * @status: Enable(true)/disable(false) status for the limit type
 * @limit_cur: Current limit value in milliamperes (mA)
 *
 * This function handles input current limit changes by:
 * 1. Updating the input limit type status in atomic flags
 * 2. Determining the highest priority active limit type by scanning from highest to lowest priority
 * 3. Executing the corresponding limit process if the active type has changed
 * 4. Logging the final limit setting through PMIC_DEBUG
 *
 * Note: The priority order is defined by enum sunxi_power_input_limit_type values (higher value = higher priority)
 * Priority order from high to low:
 * 1. PD_CHARGE (Type-C PD charging)
 * 2. TCPC (Type-C)
 * 3. MULTI_CHARGE
 * 4. PC (USB SDP)
 * 5. OTHERS
 * 6. DEFAULT
 */

static void sunxi_usb_power_limit_input_limit_process(struct sunxi_usb_power_limit_supply_data *power_limit,
							enum sunxi_power_input_limit_type input_limit_type,
							bool status, int limit_cur)
{
	enum sunxi_power_input_limit_type limit_type = SUNXI_POWER_INPUT_LIMIT_UNKNOWN;
	int limit_staus_old, limit_staus;

	limit_staus_old = atomic_read(&power_limit->input_limit_type);

	_sunxi_usb_power_limit_set_input_limit_type(power_limit, input_limit_type, status);

	limit_staus = atomic_read(&power_limit->input_limit_type);

	PMIC_DEBUG("current limit staus old : 0x%x, limit staus : 0x%x\n",
			limit_staus_old, limit_staus);
	SUNXI_POWER_LOG_INFO(power_limit->debug, "current limit staus old : 0x%x, limit staus : 0x%x",
			limit_staus_old, limit_staus);

	limit_type = _sunxi_usb_power_limit_get_input_limit_type(limit_staus);

	if (limit_type != SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE) {
		if (!atomic_read(&power_limit->vbus_online_status)) {
			limit_type = SUNXI_POWER_INPUT_LIMIT_UNKNOWN;
		}
	}

	_sunxi_usb_power_limit_input_limit_process(power_limit, limit_type, limit_cur);
}

/*---------------------------------------------
 * online check & delay work process
 *---------------------------------------------
 */

/**
 * sunxi_usb_power_limit_check_supply_online_process - Process power supply online status changes
 * @power_limit: USB power supply data structure
 * @psy: Power supply device that triggered the change
 *
 * Handles power supply status changes by:
 * 1. Identifying which supply changed (USB/TCPC/Multi-charge)
 * 2. Triggering appropriate delayed work based on supply type:
 *    - VBUS detection monitoring (immediate)
 *    - Multi-charge status (immediate)
 *    - TCPC status (100ms delay)
 */

static void sunxi_usb_power_limit_check_supply_online_process(struct sunxi_usb_power_limit_supply_data *power_limit, struct power_supply *psy)
{
	enum sunxi_supply_list_id list_id;

	for (list_id = 0; list_id < SUNXI_SUPPLY_LIST_MAX; list_id++) {
		if (psy && psy == power_limit->power_psy[list_id].psy) {
			sunxi_usb_power_limit_check_supply_online(power_limit, list_id);
			break;
		}
	}

	if (list_id == SUNXI_SUPPLY_LIST_USB_POWER) {
		cancel_delayed_work_sync(&power_limit->vbus_online_det_mon);
		schedule_delayed_work(&power_limit->vbus_online_det_mon, 0);
	}

	if (list_id == SUNXI_SUPPLY_LIST_MULTI_CHARGE) {
		cancel_delayed_work_sync(&power_limit->multi_charge_online_change);
		schedule_delayed_work(&power_limit->multi_charge_online_change, 0);
	}

	if (list_id == SUNXI_SUPPLY_LIST_TCPC) {
		cancel_delayed_work_sync(&power_limit->tcpc_online_change);
		schedule_delayed_work(&power_limit->tcpc_online_change, 100);
	}
}

static void sunxi_usb_power_limit_tcpc_online_change_process(struct work_struct *work)
{
	struct sunxi_usb_power_limit_supply_data *power_limit =
		container_of(work, typeof(*power_limit), tcpc_online_change.work);

	if (sunxi_usb_power_limit_is_tcpc_online(power_limit)) {
		_sunxi_usb_power_limit_get_tcpc_paras(power_limit);
		if (power_limit->tcpc_limit.input_vol > 5000) {
			sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_PD_CHARGE, true, 0);
		} else {
			sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_TCPC, true, 0);
		}
	} else {
		sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_PD_CHARGE, false, 0);
		sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_TCPC, false, 0);
	}
}

static void sunxi_usb_power_limit_multi_charge_online_change_process(struct work_struct *work)
{
	struct sunxi_usb_power_limit_supply_data *power_limit =
		container_of(work, typeof(*power_limit), multi_charge_online_change.work);

	if (sunxi_usb_power_limit_is_multi_charge_online(power_limit)) {
		sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE, true, 0);
	} else {
		sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_MULTI_CHARGE, false, 0);
	}
}

static void sunxi_usb_power_limit_pc_online_change_process(struct work_struct *work)
{
	struct sunxi_usb_power_limit_supply_data *power_limit =
		container_of(work, typeof(*power_limit), pc_online_change.work);

	if (sunxi_usb_power_limit_extcon_is_usb_pc_online(power_limit)) {
		sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_PC, true, 0);
	} else {
		sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_PC, false, 0);
	}
}

/**
 * sunxi_usb_power_limit_vbus_online_det_monitor - VBUS state monitoring work handler
 * @work: Delayed work structure
 *
 * Monitors VBUS state and handles changes by:
 * 1. Checking current VBUS status via sunxi_usb_power_limit_is_usb_power_online()
 * 2. Comparing with previous status stored in atomic variable
 * 3. If changed:
 *    - Updates atomic status flag
 *    - Notifies power supply subsystem via power_supply_changed()
 *    - Triggers appropriate actions:
 *      * Online: Calls sunxi_usb_power_limit_vbus_online_process()
 *      * Offline: Calls sunxi_usb_power_limit_vbus_offline_process()
 * 4. Self-schedules to run again when VBUS status changes
 */

static void sunxi_usb_power_limit_vbus_online_det_monitor(struct work_struct *work)
{
	struct sunxi_usb_power_limit_supply_data *power_limit =
		container_of(work, typeof(*power_limit), vbus_online_det_mon.work);
	int vbus_det_status, vbus_det_status_old, ret = 0;

	ret = sunxi_usb_power_limit_is_usb_power_online(power_limit);

	vbus_det_status = ret;
	vbus_det_status_old = atomic_read(&power_limit->vbus_online_status);

	if (vbus_det_status_old == vbus_det_status) {
		return;
	} else {
		atomic_set(&power_limit->vbus_online_status, vbus_det_status);
	}

	power_supply_changed(power_limit->power_limit_psy);

	if (vbus_det_status) {
		sunxi_usb_power_limit_vbus_online_process(power_limit);
	} else {
		sunxi_usb_power_limit_vbus_offline_process(power_limit);
	}
}

static void sunxi_usb_power_limit_set_current_fsm(struct work_struct *work)
{
	struct sunxi_usb_power_limit_supply_data *power_limit =
		container_of(work, typeof(*power_limit), usb_power_self_managed_chg.work);
	enum sunxi_power_input_limit_type limit_type = SUNXI_POWER_INPUT_LIMIT_UNKNOWN;
	int limit_cur, limit_staus;

	sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_DEFAULT, true, 0);

	limit_staus = atomic_read(&power_limit->input_limit_type);
	limit_type = _sunxi_usb_power_limit_get_input_limit_type(limit_staus);
	limit_cur = _sunxi_usb_power_limit_get_input_limit(power_limit);

	PMIC_INFO("current limit now : %s, limit_cur: %d mA\n",
			SUNXI_SUPPLY_INPUT_LIMIT_TYPE_TEXT[limit_type], limit_cur);

	__pm_relax(power_limit->usb_power_self_managed_chg_wakeup);
}

static void sunxi_usb_power_limit_monitor(struct work_struct *work)
{
	struct sunxi_usb_power_limit_supply_data *power_limit =
		container_of(work, typeof(*power_limit), power_limit_supply_mon.work);

	schedule_delayed_work(&power_limit->power_limit_supply_mon, msecs_to_jiffies(500));
}

/*---------------------------------------------
 * vbus det notify & extcon notify
 *---------------------------------------------
 */

static int sunxi_usb_power_limit_online_det_notify(struct notifier_block *nb, unsigned long val, void *v)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = container_of(nb, struct sunxi_usb_power_limit_supply_data, psy_nb);
	struct power_supply *psy = v;

	if (val == PSY_EVENT_PROP_CHANGED) {
		sunxi_usb_power_limit_check_supply_online_process(power_limit, psy);
	}

	return NOTIFY_OK;
}

static int sunxi_usb_power_limit_usb_udc_extcon(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = container_of(nb,
				struct sunxi_usb_power_limit_supply_data, extcon[SUNXI_EXTCON_LIST_USB_UDC].nb);

	if (event) {
		power_limit->extcon[SUNXI_EXTCON_LIST_USB_UDC].online_status = SUNXI_STATUS_ONLINE;
	} else {
		power_limit->extcon[SUNXI_EXTCON_LIST_USB_UDC].online_status = SUNXI_STATUS_OFFLINE;
	}

	cancel_delayed_work_sync(&power_limit->pc_online_change);
	schedule_delayed_work(&power_limit->pc_online_change, 0);

	return NOTIFY_DONE;
}

static int sunxi_usb_power_limit_usb_phy_extcon(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = container_of(nb,
				struct sunxi_usb_power_limit_supply_data, extcon[SUNXI_EXTCON_LIST_USB_PHY].nb);

	if (event) {
		power_limit->extcon[SUNXI_EXTCON_LIST_USB_PHY].online_status = SUNXI_STATUS_ONLINE;
	} else {
		power_limit->extcon[SUNXI_EXTCON_LIST_USB_PHY].online_status = SUNXI_STATUS_OFFLINE;
	}

	cancel_delayed_work_sync(&power_limit->pc_online_change);
	schedule_delayed_work(&power_limit->pc_online_change, 0);

	return NOTIFY_DONE;
}

/*---------------------------------------------
 * init base settings
 *---------------------------------------------
 */

static int sunxi_usb_power_limit_init_paras(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	struct power_supply *psy = power_limit->power_psy[SUNXI_SUPPLY_LIST_USB_POWER].psy;
	struct sunxi_usb_power_limit_config_info *power_limit_config = &power_limit->node_info;
	union power_supply_propval temp;
	int ret = 0;

	if (psy == NULL)
		return -ENODEV;

	/* init default value */
	sunxi_usb_power_limit_reset_default_settings(power_limit);

	if (!sunxi_usb_power_limit_is_battery_online(power_limit)) {
		__sunxi_usb_power_limit_set_input_limit(power_limit, power_limit_config->pmu_usbad_cur);
		return ret;
	}

	/* set vindpm value */
	temp.intval = power_limit_config->pmu_usbad_vol;
	ret = power_supply_set_property(psy, POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN, &temp);
	if (ret < 0)
		return ret;

	return ret;
}

/*---------------------------------------------
 * init notify
 *---------------------------------------------
 */

static int sunxi_usb_power_limit_online_det_notify_init(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	int ret = 0;

	power_limit->psy_nb.notifier_call = sunxi_usb_power_limit_online_det_notify;
	power_limit->psy_nb.priority = 0;
	ret = power_supply_reg_notifier(&power_limit->psy_nb);
	if (ret < 0) {
		PMIC_ERR("failed to register notifier :%d\n", ret);
		return ret;
	}

	return ret;
}

static int _sunxi_usb_power_limit_extcon_init(struct sunxi_usb_power_limit_supply_data *power_limit,
						enum sunxi_extcon_list_id list_id, int extcon_id,
						void (*notifier_call))
{
	int ret = 0;
	const char *phandle_name = SUNXI_EXTCON_LIST_PHANDLE_NAME[list_id];

	if (IS_ERR_OR_NULL(phandle_name)) {
		power_limit->extcon[list_id].node_status = SUNXI_NODE_NOT_EXIST;
		power_limit->extcon[list_id].online_status = SUNXI_STATUS_OFFLINE;
		PMIC_INFO("error phandle_name: %s\n", phandle_name);
		return ret;
	}

	power_limit->extcon[list_id].phandle_name = phandle_name;

	ret = sunxi_power_supply_init_dt_extcon(power_limit->dev,
						power_limit->dev->of_node,
						power_limit->extcon,
						extcon_id,
						list_id,
						SUNXI_EXTCON_LIST_MAX,
						notifier_call);

	return ret;
}

static int sunxi_usb_power_limit_extcon_init(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	int ret = 0;

	ret = _sunxi_usb_power_limit_extcon_init(power_limit, SUNXI_EXTCON_LIST_USB_UDC,
						EXTCON_CHG_USB_SDP, sunxi_usb_power_limit_usb_udc_extcon);
	if (ret < 0)
		return ret;

	ret = _sunxi_usb_power_limit_extcon_init(power_limit, SUNXI_EXTCON_LIST_USB_PHY,
						EXTCON_CHG_USB_SDP, sunxi_usb_power_limit_usb_phy_extcon);

	return ret;
}

/*---------------------------------------------
 * init by node paras
 *---------------------------------------------
 */

static int _sunxi_usb_power_limit_init_dt_supply(struct sunxi_usb_power_limit_supply_data *power_limit,
						enum sunxi_supply_list_id list_id)
{
	int ret = 0;
	const char *phandle_name = SUNXI_SUPPLY_LIST_PHANDLE_NAME[list_id];

	if (IS_ERR_OR_NULL(phandle_name)) {
		power_limit->power_psy[list_id].node_status = SUNXI_NODE_NOT_EXIST;
		power_limit->power_psy[list_id].online_status = SUNXI_STATUS_OFFLINE;
		PMIC_INFO("error phandle_name: %s\n", phandle_name);
		return ret;
	}

	power_limit->power_psy[list_id].phandle_name = phandle_name;

	ret = sunxi_power_supply_init_dt_supply(power_limit->dev,
						power_limit->dev->of_node,
						power_limit->power_psy,
						list_id,
						SUNXI_SUPPLY_LIST_MAX);
	if (ret < 0)
		return ret;

	if (list_id == SUNXI_SUPPLY_LIST_USB_POWER) {
		if (IS_ERR_OR_NULL(power_limit->power_psy[list_id].psy)) {
			PMIC_ERR("error usb supply\n");
			return -ENODEV;
		}
	}

	return 0;
}

static int sunxi_usb_power_limit_init_dt_supply(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	enum sunxi_supply_list_id list_id;
	int ret = 0;

	for (list_id = 0; list_id < SUNXI_SUPPLY_LIST_MAX; list_id++) {
		ret = _sunxi_usb_power_limit_init_dt_supply(power_limit, list_id);
		if (ret < 0)
			return ret;
	}

	return ret;
}

static int sunxi_usb_power_limit_dt_parse(struct device_node *node,
			 struct sunxi_usb_power_limit_config_info *config)
{
	if (!of_device_is_available(node)) {
		PMIC_ERR("%s: failed\n", __func__);
		return -1;
	}

	/* main */
	sunxi_usb_power_limit_OF_PROP_READ(pmu_bc12_en,		0);
	/* input ctrl */
	sunxi_usb_power_limit_OF_PROP_READ(pmu_usbpc_vol,	4600);
	sunxi_usb_power_limit_OF_PROP_READ(pmu_usbpc_cur,	500);
	sunxi_usb_power_limit_OF_PROP_READ(pmu_usbad_vol,	4600);
	sunxi_usb_power_limit_OF_PROP_READ(pmu_usbad_cur,	1500);

	return 0;
}

static int sunxi_usb_power_limit_parse_device_tree(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	int ret;
	struct power_supply *psy;
	struct device_node *np = NULL;
	struct sunxi_usb_power_limit_config_info *power_limit_config = &power_limit->node_info;

	ret = sunxi_usb_power_limit_init_dt_supply(power_limit);
	if (ret < 0)
		return ret;

	psy = power_limit->power_psy[SUNXI_SUPPLY_LIST_USB_POWER].psy;

	np = of_parse_phandle(psy->of_node, "det_usb_supply", 0);
	if (np) {
		ret = sunxi_usb_power_limit_dt_parse(np, power_limit_config);
		if (ret) {
			PMIC_ERR("can not parse device tree err\n");
			return -ENODEV;
		}
		ret = sunxi_usb_power_limit_init_paras(power_limit);
		if (ret) {
			PMIC_ERR("sunxi usb init paras err\n");
			return -ENODEV;
		}
	} else {
		PMIC_ERR("can not parse usb supply device tree err\n");
		return -ENODEV;
	}

	ret = sunxi_usb_power_limit_extcon_init(power_limit);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static int sunxi_usb_power_limit_init_paras_late(struct sunxi_usb_power_limit_supply_data *power_limit)
{
	int ret = 0;

	INIT_DELAYED_WORK(&power_limit->power_limit_supply_mon, sunxi_usb_power_limit_monitor);
	INIT_DELAYED_WORK(&power_limit->usb_power_self_managed_chg, sunxi_usb_power_limit_set_current_fsm);
	INIT_DELAYED_WORK(&power_limit->vbus_online_det_mon, sunxi_usb_power_limit_vbus_online_det_monitor);

	INIT_DELAYED_WORK(&power_limit->tcpc_online_change, sunxi_usb_power_limit_tcpc_online_change_process);
	INIT_DELAYED_WORK(&power_limit->multi_charge_online_change, sunxi_usb_power_limit_multi_charge_online_change_process);
	INIT_DELAYED_WORK(&power_limit->pc_online_change, sunxi_usb_power_limit_pc_online_change_process);
	power_limit->usb_power_self_managed_chg_wakeup = wakeup_source_register(power_limit->dev, "usb_power_self_managed_chg_wakeup");

	ret = sunxi_usb_power_limit_online_det_notify_init(power_limit);
	if (ret < 0) {
		return ret;
	}

	schedule_delayed_work(&power_limit->multi_charge_online_change, 100);
	schedule_delayed_work(&power_limit->power_limit_supply_mon, 500);
	schedule_delayed_work(&power_limit->vbus_online_det_mon, 100);
	schedule_delayed_work(&power_limit->usb_power_self_managed_chg, msecs_to_jiffies(20 * 1000));

	return ret;
}

/*---------------------------------------------
 * init psy
 *---------------------------------------------
 */

static enum power_supply_property sunxi_usb_power_limit_props[] = {
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
};

int sunxi_usb_power_limit_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		val->intval = _sunxi_usb_power_limit_get_input_limit(power_limit);
		break;
	default:
		break;
	}

	return ret;
}

int sunxi_usb_power_limit_set_property(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		if (val->intval > 0) {
			sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_OTHERS, true, val->intval);
		} else {
			sunxi_usb_power_limit_input_limit_process(power_limit, SUNXI_POWER_INPUT_LIMIT_OTHERS, false, val->intval);
		}
		break;
	default:
		break;
	}

	return ret;
}

static int sunxi_usb_power_limit_property_is_writeable(struct power_supply *psy,
				 enum power_supply_property psp)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = 1;
		break;
	default:
		break;
	}

	return ret;
}

static struct power_supply_desc sunxi_usb_power_limit_desc = {
	.name = "sunxi_usb_power_limit",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property = sunxi_usb_power_limit_get_property,
	.set_property = sunxi_usb_power_limit_set_property,
	.properties = sunxi_usb_power_limit_props,
	.num_properties = ARRAY_SIZE(sunxi_usb_power_limit_props),
	.property_is_writeable = sunxi_usb_power_limit_property_is_writeable,
};

static int sunxi_usb_power_limit_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct sunxi_usb_power_limit_supply_data *power_limit;
	struct power_supply_config psy_cfg = {};
	struct device_node *node = pdev->dev.of_node;

	if (!of_device_is_available(node)) {
		PMIC_ERR("sunxi_usb_power_limit device is not configed\n");
		return -ENODEV;
	}

	power_limit = devm_kzalloc(&pdev->dev, sizeof(*power_limit), GFP_KERNEL);
	if (power_limit == NULL) {
		PMIC_ERR("sunxi_usb_power_limit alloc failed\n");
		ret = -ENOMEM;
		goto err;
	}

	power_limit->name = "sunxi_usb_power_limit";
	power_limit->dev = &pdev->dev;

	psy_cfg.of_node = pdev->dev.of_node;
	psy_cfg.drv_data = power_limit;

	ret = sunxi_usb_power_limit_parse_device_tree(power_limit);
	if (ret < 0) {
		goto err;
	}

	power_limit->power_limit_psy = devm_power_supply_register(power_limit->dev,
			&sunxi_usb_power_limit_desc, &psy_cfg);

	if (IS_ERR(power_limit->power_limit_psy)) {
		PMIC_ERR("failed to register sunxi power limit\n");
		ret = PTR_ERR(power_limit->power_limit_psy);
		goto err;
	}

	platform_set_drvdata(pdev, power_limit);

	ret = sunxi_usb_power_limit_init_paras_late(power_limit);
	if (ret < 0) {
		goto err;
	}

	power_limit->debug = sunxi_power_debugfs_init(&pdev->dev);
	if (IS_ERR_OR_NULL(power_limit->debug))
		dev_warn(&pdev->dev, "Failed to init debugfs\n");

	SUNXI_POWER_LOG_INFO(power_limit->debug, "sunxi-usb-power-limit driver initialized");

	return ret;
err:
	PMIC_ERR("%s,probe fail, ret = %d\n", __func__, ret);

	return ret;
}

static void sunxi_usb_power_limit_delayed_work_set(struct sunxi_usb_power_limit_supply_data *power_limit, bool enable)
{
	if (enable) {
		schedule_delayed_work(&power_limit->vbus_online_det_mon, 0);
		schedule_delayed_work(&power_limit->usb_power_self_managed_chg, 0);
		schedule_delayed_work(&power_limit->power_limit_supply_mon, 0);
	} else {
		cancel_delayed_work_sync(&power_limit->vbus_online_det_mon);
		cancel_delayed_work_sync(&power_limit->usb_power_self_managed_chg);
		cancel_delayed_work_sync(&power_limit->power_limit_supply_mon);
	}
}

static int sunxi_usb_power_limit_remove(struct platform_device *pdev)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = platform_get_drvdata(pdev);

	PMIC_DEV_DEBUG(&pdev->dev, "==============sunxi power limit unegister==============\n");
	if (power_limit->power_limit_psy) {
		sunxi_usb_power_limit_delayed_work_set(power_limit, false);
		power_supply_unregister(power_limit->power_limit_psy);
	}
	sunxi_power_debugfs_exit(power_limit->debug);
	PMIC_DEV_DEBUG(&pdev->dev, "teardown sunxi power limit dev\n");

	return 0;
}

static void sunxi_usb_power_limit_shutdown(struct platform_device *pdev)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = platform_get_drvdata(pdev);

	sunxi_usb_power_limit_delayed_work_set(power_limit, false);
}

static int sunxi_usb_power_limit_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = platform_get_drvdata(pdev);

	sunxi_usb_power_limit_delayed_work_set(power_limit, false);

	return 0;
}

static int sunxi_usb_power_limit_resume(struct platform_device *pdev)
{
	struct sunxi_usb_power_limit_supply_data *power_limit = platform_get_drvdata(pdev);

	sunxi_usb_power_limit_delayed_work_set(power_limit, true);

	return 0;
}

static const struct of_device_id sunxi_usb_power_limit_match[] = {
	{
		.compatible = "x-powers,sunxi-usb-power-limit",
	}, {/* sentinel */}
};
MODULE_DEVICE_TABLE(of, sunxi_usb_power_limit_match);

static struct platform_driver sunxi_usb_power_limit_driver = {
	.driver = {
		.name = "sunxi-usb-power-limit",
		.of_match_table = sunxi_usb_power_limit_match,
	},
	.probe = sunxi_usb_power_limit_probe,
	.remove = sunxi_usb_power_limit_remove,
	.shutdown = sunxi_usb_power_limit_shutdown,
	.suspend = sunxi_usb_power_limit_suspend,
	.resume = sunxi_usb_power_limit_resume,
};

module_platform_driver(sunxi_usb_power_limit_driver);

MODULE_VERSION("1.0.3");
MODULE_AUTHOR("xinouyang <xinouyang@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi power limit driver");
MODULE_LICENSE("GPL");
