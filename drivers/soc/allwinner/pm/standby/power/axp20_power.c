/*
 * axp192 for standby driver
 *
 * Copyright (C) 2015 allwinnertech	Ltd.
 * Author: Ming	Li <liming@allwinnertech.com>
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under	the	terms of the GNU General Public	License	version	2 as
 * published by	the	Free Software Foundation.
 */

#include "../standby.h"
#include "../standby_twi.h"
#include "axp20_power.h"

static struct axp_regulator_info axp20_regulator_info[]	= {
	AXP_DCDC(AXP20,	0,	700,	2275,	25,		BUCK2,	0,	 6, \
			BUCK2EN,	0x10,	0x0,	0),
	AXP_DCDC(AXP20,	1,	700,	3500,	25,		BUCK3,	0,	 7, \
			BUCK3EN,	0x2,	0x0,	0),
	AXP_LDO(AXP20,	2,	3300,	3300,	0,		LDO1,	0,	 0, \
			LDO1EN,		0,		0,		0),										/* ldo1	for	rtc	*/
	AXP_LDO(AXP20,	3,	1800,	3300,	100,	LDO2,	4,	 4, \
			LDO2EN,		0x6,	0x0,	0),										/* ldo2	for	aldo1 */
	AXP_LDO(AXP20,	4,	700,	3500,	25,		LDO3,	0,	 7, \
			LDO3EN,		0x60,	0x0, 0),										/* ldo3	for	aldo2 */
	AXP_LDO(AXP20,	5,	1250,	3300,	0,		LDO4,	0,	 4, \
			LDO4EN,		0x8,	0x0,	0),										/* ldo4	for	aldo3 */
	AXP_LDO(AXP20,	6,	1800,	3300,	100,	LDOIO0,	4,	 4, \
			LDOIOEN,	0,		0,		0),										/* ldo5	for	dldo1 */
};

struct axp_dev_info	axp20_dev_info = {
	.pmu_addr		= AXP20_ADDR,
	.pmu_id_max		= AXP20_ID_MAX,
	.pmu_regu_table	= &axp20_regulator_info[0],
};

/* AXP common operations */
s32	axp20_set_volt(u32 id, u32 voltage)
{
	return axp_set_volt(&axp20_dev_info, id, voltage);
}

s32	axp20_get_volt(u32 id)
{
	return axp_get_volt(&axp20_dev_info, id);
}

s32	axp20_set_state(u32	id,	u32	state)
{
	return axp_set_state(&axp20_dev_info, id, state);
}

s32	axp20_get_state(u32	id)
{
	return axp_get_state(&axp20_dev_info, id);
}

s32	__axp20_set_wakeup(u8 saddr, u8	reg)
{
	u8 reg_val;
	s32	ret	= 0;

	ret	= twi_byte_rw(TWI_OP_RD, saddr,	reg, &reg_val);
	if (ret)
		goto out;

	reg_val	|= 0x1 << 3;

	ret	= twi_byte_rw(TWI_OP_WR, saddr,	reg, &reg_val);

out:
	return ret;
}

/* default == 0	*/
static u8 dcdc_disable_val;
static u8 dcdc_enable_mask;

void axp20_losc_enter_ss(void)
{
	s32	ret	= 0;
	pr_info("%s: dcdc_disable_val=0x%x,	dcdc_enable_mask=0x%x\n",\
		__func__, dcdc_disable_val,	dcdc_enable_mask);

	ret	= __twi_byte_update(AXP20_ADDR,	AXP20_BUCK2EN,
			dcdc_disable_val, dcdc_enable_mask);
	if (ret)
		pr_info("axp20 dcdc	failed\n");

}

s32	axp20_suspend_calc(u32 id, losc_enter_ss_func *func)
{
	struct axp_regulator_info *info	= NULL;
	u32	i =	0;
	s32	ret	= 0;

	pr_info("%s: id=0x%x\n", __func__, id);

	__axp20_set_wakeup(AXP20_ADDR, 0x31);

	for	(i = 0;	i <= AXP20_ID_MAX; i++)	{
		if (id & (0x1 << i)) {
			info = find_info(&axp20_dev_info, (0x1 << i));
			if (i <	AXP20_DCDC_SUM)	{
				dcdc_disable_val |=	info->disable_val;
				dcdc_enable_mask |=	info->enable_mask;
				continue;
			} else {
				pr_info("close non-cpu src : 0x%x 0x%x,	0x%x\n", \
						info->enable_reg,
						info->disable_val,
						info->enable_mask);
						ret	= __twi_byte_update(AXP20_ADDR,
						info->enable_reg,
						info->disable_val,
						info->enable_mask);
				if (ret)
					pr_info("axp20-%d failed\n", i);
			}
		}
	}
	*func =	(void (*)(void))axp20_losc_enter_ss;
	axp20_losc_enter_ss();
	asm("b .");
	return ret;
}
