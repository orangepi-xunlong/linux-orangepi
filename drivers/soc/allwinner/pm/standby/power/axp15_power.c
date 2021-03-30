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
#include "axp15_power.h"

static const int axp15_ldo0_table[] = {
	5000, 3300, 2800, 2500
};

static const int axp15_dcdc1_table[] = {
	1700, 1800, 1900, 2000,
	2100, 2400, 2500, 2600,
	2700, 2800, 3000, 3100,
	3200, 3300, 3400, 3500
};

static const int axp15_aldo12_table[] = {
	1200, 1300, 1400, 1500,
	1600, 1700, 1800, 1900,
	2000, 2500, 2700, 2800,
	3000, 3100, 3200, 3300
};

static struct axp_regulator_info axp15_regulator_info[] = {
	AXP_DCDC_SEL(AXP15, 1,  1700, 3500,       DCDC1, 0, 4,  DCDC1EN,
		0x80, 0x80,     0,   axp15_dcdc1),
	AXP_DCDC(AXP15,     2,   700, 2275,  25,  DCDC2, 0, 6,  DCDC2EN,
		0x40, 0x40,     0),
	AXP_DCDC(AXP15,     3,   700, 3500,  50,  DCDC3, 0, 6,  DCDC3EN,
		0x20, 0x20,     0),
	AXP_DCDC(AXP15,     4,   700, 3500,  25,  DCDC4, 0, 7,  DCDC4EN,
		0x10, 0x10,     0),
	AXP_LDO_SEL(AXP15,  0,  2500, 5000,        LDO0, 4, 2,   LDO0EN,
		0x80, 0x80,     0,    axp15_ldo0),
	AXP_LDO(AXP15,      1,  3100, 3100,   0,    RTC, 0, 0, RTCLDOEN,
		0x01, 0x01,     0),
	AXP_LDO_SEL(AXP15,  2,  1200, 3300,       ALDO1, 4, 4,  ALDO1EN,
		0x08, 0x08,     0,  axp15_aldo12),
	AXP_LDO_SEL(AXP15,  3,  1200, 3300,       ALDO2, 0, 4,  ALDO2EN,
		0x04, 0x04,     0,  axp15_aldo12),
	AXP_LDO(AXP15,      4,   700, 3500, 100,  DLDO1, 0, 5,  DLDO1EN,
		0x02, 0x02,     0),
	AXP_LDO(AXP15,      5,   700, 3500, 100,  DLDO2, 0, 5,  DLDO2EN,
		0x01, 0x01,     0),
	AXP_LDO(AXP15,    IO0,  1800, 3300, 100, LDOIO0, 0, 4,  LDOI0EN,
		0x07, 0x02,  0x07),
};

struct axp_dev_info axp15_dev_info = {
	.pmu_addr       = AXP15_ADDR,
	.pmu_id_max     = AXP15_ID_MAX,
	.pmu_regu_table = &axp15_regulator_info[0],
};

/* AXP common operations */
s32 axp15_set_volt(u32 id, u32 voltage)
{
	return axp_set_volt(&axp15_dev_info, id, voltage);
}

s32 axp15_get_volt(u32 id)
{
	return axp_get_volt(&axp15_dev_info, id);
}

s32 axp15_set_state(u32 id, u32 state)
{
	return axp_set_state(&axp15_dev_info, id, state);
}

s32 axp15_get_state(u32 id)
{
	return axp_get_state(&axp15_dev_info, id);
}

s32 __axp15_set_wakeup(u8 saddr)
{
	u8 reg_val;
	s32 ret = 0;

	ret = twi_byte_rw(TWI_OP_RD, saddr, 0x31, &reg_val);
	if (ret)
		goto out;

	reg_val |= 0x1 << 3;

	ret = twi_byte_rw(TWI_OP_WR, saddr, 0x31, &reg_val);
	if (ret)
		goto out;

	ret = twi_byte_rw(TWI_OP_RD, saddr, 0x42, &reg_val);
	if (ret)
		goto out;

	reg_val |= 0x1;

	ret = twi_byte_rw(TWI_OP_WR, saddr, 0x42, &reg_val);
	if (ret)
		goto out;

	ret = twi_byte_rw(TWI_OP_RD, saddr, 0x90, &reg_val);
	if (ret)
		goto out;

	/*
	 * enable gpio0 negative edge trigger IRQ or wake up
	 * when gpio0 is digital input
	 */
	reg_val |= 0x1 << 6;

	/* gpio0 as digital input */
	reg_val &= ~(0x7 << 0);
	reg_val |= 0x3 << 0;

	ret = twi_byte_rw(TWI_OP_WR, saddr, 0x90, &reg_val);
out:
	return ret;
}

/* default == 0 */
static u8 dcdc_disable_val;
static u8 dcdc_enable_mask;

void axp15_losc_enter_ss(void)
{
	s32 ret = 0;
	printk("%s: dcdc_disable_val=0x%x, dcdc_enable_mask=0x%x\n",
		__func__, dcdc_disable_val, dcdc_enable_mask);

	ret = __twi_byte_update(AXP15_ADDR, AXP15_DCDC1EN,
			dcdc_disable_val, dcdc_enable_mask);
	if (ret)
		printk("axp15 dcdc failed\n");

}

s32 axp15_suspend_calc(u32 id, losc_enter_ss_func *func)
{
	struct axp_regulator_info *info = NULL;
	u32 i = 0;
	s32 ret = 0;

	printk("%s: id=0x%x\n", __func__, id);

	__axp15_set_wakeup(AXP15_ADDR);

	for (i = 0; i <= AXP15_ID_MAX; i++) {
		if (id & (0x1 << i)) {
			info = find_info(&axp15_dev_info, (0x1 << i));
			dcdc_disable_val |= info->disable_val;
			dcdc_enable_mask |= info->enable_mask;
		}
	}

	*func = (void (*)(void))axp15_losc_enter_ss;
	axp15_losc_enter_ss();
	asm("b .");
	return ret;
}
