/*
 * axp192 for standby driver
 *
 * Copyright (C) 2015 allwinnertech Ltd.
 * Author: Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../standby.h"
#include "../standby_twi.h"
#include "axp22_power.h"

static struct axp_regulator_info axp22_regulator_info[] = {
	AXP_DCDC(AXP22, 1,  1600, 3400, 100,  DCDC1, 0, 5,  DCDC1EN,
		0x02, 0x02, 0),
	AXP_DCDC(AXP22, 2,   600, 1540,  20,  DCDC2, 0, 6,  DCDC2EN,
		0x04, 0x04, 0),
	AXP_DCDC(AXP22, 3,   600, 1860,  20,  DCDC3, 0, 6,  DCDC3EN,
		0x08, 0x08, 0),
	AXP_DCDC(AXP22, 4,   600, 1540,  20,  DCDC4, 0, 6,  DCDC4EN,
		0x10, 0x10, 0),
	AXP_DCDC(AXP22, 5,  1000, 2550,  50,  DCDC5, 0, 5,  DCDC5EN,
		0x20, 0x20, 0),
	AXP_LDO(AXP22,  2,   700, 3300, 100,   LDO2, 0, 5,   LDO2EN,
		0x40, 0x40, 0),
	AXP_LDO(AXP22,  3,   700, 3300, 100,   LDO3, 0, 5,   LDO3EN,
		0x80, 0x80, 0),
	AXP_LDO(AXP22,  4,   700, 3300, 100,   LDO4, 0, 5,   LDO4EN,
		0x80, 0x80, 0),
	AXP_LDO(AXP22,  5,   700, 3300, 100,   LDO5, 0, 5,   LDO5EN,
		0x08, 0x08, 0),
	AXP_LDO(AXP22,  6,   700, 3300, 100,   LDO6, 0, 5,   LDO6EN,
		0x10, 0x10, 0),
	AXP_LDO(AXP22,  7,   700, 3300, 100,   LDO7, 0, 5,   LDO7EN,
		0x20, 0x20, 0),
	AXP_LDO(AXP22,  8,   700, 3300, 100,   LDO8, 0, 5,   LDO8EN,
		0x40, 0x40, 0),
	AXP_LDO(AXP22,  9,   700, 3300, 100,   LDO9, 0, 5,   LDO9EN,
		0x01, 0x01, 0),
	AXP_LDO(AXP22, 10,   700, 3300, 100,  LDO10, 0, 5,  LDO10EN,
		0x02, 0x02, 0),
	AXP_LDO(AXP22, 11,   700, 3300, 100,  LDO11, 0, 5,  LDO11EN,
		0x04, 0x04, 0),
	AXP_LDO(AXP22, 12,   700, 1400, 100,  LDO12, 0, 3,  LDO12EN,
		0x01, 0x01, 0),
	AXP_LDO(AXP22, IO0,  700, 3300, 100, LDOIO0, 0, 5, LDOIO0EN,
		0x07, 0x03, 0),
	AXP_LDO(AXP22, IO1,  700, 3300, 100, LDOIO1, 0, 5, LDOIO1EN,
		0x07, 0x03, 0),
	AXP_SW(AXP22, 1,    3300, 3300, 100,  DC1SW, 0, 0,  DC1SWEN,
		0x80, 0x80, 0),
	AXP_LDO(AXP22, 1,   3000, 3000,   0,   LDO1, 0, 0,   LDO1EN,
		0x01, 0x01, 0),
};

struct axp_dev_info axp22_dev_info = {
	.pmu_addr       = AXP22_ADDR,
	.pmu_id_max     = AXP22_ID_MAX,
	.pmu_regu_table = &axp22_regulator_info[0],
};

/* AXP common operations */
s32 axp22_set_volt(u32 id, u32 voltage)
{
	return axp_set_volt(&axp22_dev_info, id, voltage);
}

s32 axp22_get_volt(u32 id)
{
	return axp_get_volt(&axp22_dev_info, id);
}

s32 axp22_set_state(u32 id, u32 state)
{
	return axp_set_state(&axp22_dev_info, id, state);
}

s32 axp22_get_state(u32 id)
{
	return axp_get_state(&axp22_dev_info, id);
}

s32 __axp22_set_wakeup(u8 saddr, u8 reg)
{
	u8 reg_val;
	s32 ret = 0;

	ret = twi_byte_rw(TWI_OP_RD, saddr, reg, &reg_val);
	if (ret)
		goto out;

	reg_val |= 0x1 << 3;
	reg_val &= ~(0x1 << 4);
	reg_val |= 0x1 << 7;

	ret = twi_byte_rw(TWI_OP_WR, saddr, reg, &reg_val);

out:
	return ret;
}

/* default == 0 */
static u8 dcdc_disable_val;
static u8 dcdc_enable_mask;

void axp22_losc_enter_ss(void)
{
	s32 ret = 0;
	printk("%s: dcdc_disable_val=0x%x, dcdc_enable_mask=0x%x\n",\
		__func__, dcdc_disable_val, dcdc_enable_mask);

	ret = __twi_byte_update(AXP22_ADDR, AXP22_DCDC1EN,
			dcdc_disable_val, dcdc_enable_mask);
	if (ret)
		printk("axp22 dcdc failed\n");

}

s32 axp22_suspend_calc(u32 id, losc_enter_ss_func *func)
{
	struct axp_regulator_info *info = NULL;
	u32 i = 0;
	s32 ret = 0;

	printk("%s: id=0x%x\n", __func__, id);

	__axp22_set_wakeup(AXP22_ADDR, 0x31);

	for (i = 0; i <= AXP22_ID_MAX; i++) {
		if (id & (0x1 << i)) {
			info = find_info(&axp22_dev_info, (0x1 << i));
			if (i < AXP22_DCDC_SUM) {
				dcdc_disable_val |= info->disable_val;
				dcdc_enable_mask |= info->enable_mask;
				continue;
			} else {
				printk("close non-cpu src : 0x%x 0x%x, 0x%x\n", \
						info->enable_reg,
						info->disable_val,
						info->enable_mask);
				ret = __twi_byte_update(AXP22_ADDR,
						info->enable_reg,
						info->disable_val,
						info->enable_mask);
				if (ret)
					printk("axp22-%d failed\n", i);
			}
		}
	}
	*func = (void (*)(void))axp22_losc_enter_ss;
	axp22_losc_enter_ss();
	asm("b .");

	return ret;
}
