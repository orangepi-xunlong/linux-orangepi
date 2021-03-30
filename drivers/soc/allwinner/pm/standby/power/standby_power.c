/*
 * standby driver for allwinnertech
 *
 * Copyright (C) 2015 allwinnertech Ltd.
 * Author: Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../standby.h"
#include "power.h"

static u32 dm_on;
static u32 dm_off;

/* power domain */
#define IS_DM_ON(dm)  ((dm_on >> dm) & 0x1)
#define IS_DM_OFF(dm) (!((dm_off >> dm) & 0x1))
#define POWER_VOL_OFF (0)
#define POWER_VOL_ON  (1)

static s32 volt_bak[VCC_MAX_INDEX];
static unsigned int (*power_regu_tree)[VCC_MAX_INDEX];

static s32 pmu_get_voltage(u32 pmux_id, u32 tree)
{
	s32 ret = -1;
	if (0 == tree) {
		printk("pmu_get_voltage:tree is 0\n");
		return ret;
	}

	pmux_id &= 0xf;
	tree &= 0x0fffffff;

	switch (pmux_id) {
	case AXP_19X_ID:
		break;
	case AXP_209_ID:
		ret = axp20_get_volt(tree);
		break;
	case AXP_22X_ID:
		ret = axp22_get_volt(tree);
		break;
	case AXP_806_ID:
		break;
	case AXP_808_ID:
		break;
	case AXP_809_ID:
		break;
	case AXP_803_ID:
		break;
	case AXP_813_ID:
		break;
	case AXP_152_ID:
		ret = axp15_get_volt(tree);
		break;
	default:
		printk("pmu_get_voltage :pmu id err, tree = 0x%x\n", tree);
		return -1;
	}

	return ret;
}

static s32 pmu_set_voltage(u32 pmux_id, u32 tree, u32 voltage)
{
	s32 ret = -1;
	if (0 == tree) {
		printk("pmu_set_voltage: tree is 0\n");
		return ret;
	}

	pmux_id &= 0xf;
	tree &= 0x0fffffff;

	switch (pmux_id) {
	case AXP_19X_ID:
		break;
	case AXP_209_ID:
		ret = axp20_set_volt(tree, voltage);
		break;
	case AXP_22X_ID:
		ret = axp22_set_volt(tree, voltage);
		break;
	case AXP_806_ID:
		break;
	case AXP_808_ID:
		break;
	case AXP_809_ID:
		break;
	case AXP_803_ID:
		break;
	case AXP_813_ID:
		break;
	case AXP_152_ID:
		ret = axp15_set_volt(tree, voltage);
		break;
	default:
		printk("pmu_set_voltage :pmu id err, tree = 0x%x\n", tree);
		return -1;
	}

	if (0 != ret)
		printk("pmu_set_voltage faied\n");

	return ret;
}

static s32 pmu_get_state(u32 pmux_id, u32 tree)
{
	s32 ret = -1;
	if (0 == tree) {
		printk("pmu_get_state: tree is 0\n");
		return ret;
	}

	pmux_id &= 0xf;
	tree &= 0x0fffffff;

	switch (pmux_id) {
	case AXP_19X_ID:
		break;
	case AXP_209_ID:
		ret = axp20_get_state(tree);
		break;
	case AXP_22X_ID:
		ret = axp22_get_state(tree);
		break;
	case AXP_806_ID:
		break;
	case AXP_808_ID:
		break;
	case AXP_809_ID:
		break;
	case AXP_803_ID:
		break;
	case AXP_813_ID:
		break;
	case AXP_152_ID:
		ret = axp15_get_state(tree);
		break;
	default:
		printk("pmu_get_state :pmu id err, tree = 0x%x\n", tree);
		return -1;
	}

	if (0 != ret)
		printk("pmu_get_state faied\n");

	return ret;
}

static s32 pmu_set_state(u32 pmux_id, u32 tree, u32 state)
{
	s32 ret = -1;
	if (0 == tree) {
		printk("pmu_set_state: tree is 0\n");
		return ret;
	}

	pmux_id &= 0xf;
	tree &= 0x0fffffff;

	switch (pmux_id) {
	case AXP_19X_ID:
		break;
	case AXP_209_ID:
		ret = axp20_set_state(tree, state);
		break;
	case AXP_22X_ID:
		ret = axp22_set_state(tree, state);
		break;
	case AXP_806_ID:
		break;
	case AXP_808_ID:
		break;
	case AXP_809_ID:
		break;
	case AXP_803_ID:
		break;
	case AXP_813_ID:
		break;
	case AXP_152_ID:
		ret = axp15_set_state(tree, state);
		break;
	default:
		printk("pmu_set_state :pmu id err, tree = 0x%x\n", tree);
		return -1;
	}

	if (0 != ret)
		printk("pmu_set_state faied\n");

	return ret;
}

static void pmu_suspend_calc(u32 pmux_id, u32 mask, losc_enter_ss_func *func)
{
	s32 ret = -1;
	s32 tmpctrl = 1;
	if (0 == mask) {
		printk("pmu_suspend: tree is 0\n");
		return;
	}

	pmux_id &= 0xf;
	mask &= 0x0fffffff;

	switch (pmux_id) {
	case AXP_19X_ID:
		break;
	case AXP_209_ID:
		ret = axp20_suspend_calc(mask, func);
		break;
	case AXP_22X_ID:
		ret = axp22_suspend_calc(mask, func);
		break;
	case AXP_806_ID:
		break;
	case AXP_808_ID:
		break;
	case AXP_809_ID:
		break;
	case AXP_803_ID:
		break;
	case AXP_813_ID:
		break;
	case AXP_152_ID:
		ret = axp15_suspend_calc(mask, func);
		break;
	default:
		printk("pmu_suspend :pmu id err, tree = 0x%x\n", mask);
		return;
	}

	if (0 != ret)
		printk("pmu_suspend faied\n");

	return;
}

/* default = 0 */
static u32 close_mask;
void power_enter_super_calc(struct aw_pm_info *config,
		       extended_standby_t *extended_config, losc_enter_ss_func *func)
{
	int dm;
	power_regu_tree = &(config->pmu_arg.soc_power_tree);
	u32 on_mask_adjust = 0;
	close_mask = 0;

	dm_on =
		extended_config->
		soc_pwr_dm_state.sys_mask & extended_config->
		soc_pwr_dm_state.state;
	dm_off =
		(~(extended_config->soc_pwr_dm_state.sys_mask) |
		 extended_config->soc_pwr_dm_state.state) | dm_on;

	for (dm = VCC_MAX_INDEX - 1; dm >= 0; dm--) {
		if (IS_DM_OFF(dm))
			close_mask |= (*power_regu_tree)[dm];
		else if (IS_DM_ON(dm))
			on_mask_adjust |= (*power_regu_tree)[dm];
	}

	printk("dm_on = 0x%x, dm_off = 0x%x, close_mask = 0x%x\n",
				dm_on, dm_off, close_mask);
	close_mask &= ~on_mask_adjust;
	printk("adjust on mask = 0x%x, adjust close mask = 0x%x\n",
				on_mask_adjust, close_mask);

	pmu_suspend_calc(extended_config->pmu_id, close_mask, func);
}

void dm_suspend(struct aw_pm_info *config, extended_standby_t *extended_config)
{
	/* one dm maybe have some output */
	int dm;
	close_mask = 0;
	u32 on_mask_adjust = 0;

	printk("extended_config->pmu_id = 0x%x. \n", extended_config->pmu_id);
	power_regu_tree = &(config->pmu_arg.soc_power_tree);

	dm_on =
		extended_config->
		soc_pwr_dm_state.sys_mask & extended_config->
		soc_pwr_dm_state.state;
	dm_off =
		(~(extended_config->soc_pwr_dm_state.sys_mask) |
		 extended_config->soc_pwr_dm_state.state) | dm_on;

	for (dm = VCC_MAX_INDEX - 1; dm >= 0; dm--) {
		if (IS_DM_ON(dm)) {
			if (extended_config->
					soc_pwr_dm_state.volt[dm] != 0) {
				volt_bak[dm] =
					pmu_get_voltage(extended_config->pmu_id,
						(*power_regu_tree)[dm]);
				if (0 < volt_bak[dm])
					printk("volt_bak[%d]=%d\n",
							dm, volt_bak[dm]);
				pmu_set_voltage(extended_config->pmu_id, (*power_regu_tree)
						[dm],
						extended_config->soc_pwr_dm_state.volt
						[dm]);
			}
		}
	}

	for (dm = VCC_MAX_INDEX - 1; dm >= 0; dm--) {
		if (IS_DM_OFF(dm))
			close_mask |= (*power_regu_tree)[dm];
		else if (IS_DM_ON(dm))
			on_mask_adjust |= (*power_regu_tree)[dm];
	}

	close_mask &= ~on_mask_adjust;
	for (dm = VCC_MAX_INDEX - 1; dm >= 0; dm--) {
		if (IS_DM_OFF(dm)) {
			if ((*power_regu_tree)[dm] & close_mask)
				pmu_set_state(extended_config->pmu_id,
					(*power_regu_tree)[dm], POWER_VOL_OFF);
		}
	}
	return;
}

void dm_resume(extended_standby_t *extended_config)
{
	u32 dm;

	for (dm = 0; dm < VCC_MAX_INDEX; dm++) {
		if (IS_DM_ON(dm)) {
			if (extended_config->
					soc_pwr_dm_state.volt[dm] != 0) {
				pmu_set_voltage(extended_config->pmu_id, (*power_regu_tree)[dm],
						volt_bak[dm]);
			}
		}
	}

	for (dm = 0; dm < VCC_MAX_INDEX; dm++) {
		if (IS_DM_OFF(dm)) {
			pmu_set_state(extended_config->pmu_id, (*power_regu_tree)[dm],
					POWER_VOL_ON);
		}
	}
	return;
}
