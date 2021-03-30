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
#include "axp_power.h"

struct axp_regulator_info *find_info(struct axp_dev_info *dev_info, u32 id)
{
	struct axp_regulator_info *ri = NULL;
	int i = 0;

	for (i = 0; i <= dev_info->pmu_id_max; i++) {
		if (id & (0x1 << i)) {
			ri = dev_info->pmu_regu_table + i;
			break;
		}
	}

	return ri;
}

/* AXP common operations */
s32 axp_set_volt(struct axp_dev_info *dev_info, u32 id, u32 voltage)
{
	struct axp_regulator_info *info = NULL;
	u8 val, mask, reg_val;
	s32 ret = -1;

	info = find_info(dev_info, id);

	val = (voltage - info->min_uv + info->step1_uv - 1)
			/ info->step1_uv;
	val <<= info->vol_shift;
	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;

	ret = twi_byte_rw(TWI_OP_RD, dev_info->pmu_addr,
					info->vol_reg, &reg_val);
	if (ret)
		return ret;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = twi_byte_rw(TWI_OP_WR, dev_info->pmu_addr,
					info->vol_reg, &reg_val);
	}

	return ret;
}

s32 axp_get_volt(struct axp_dev_info *dev_info, u32 id)
{
	struct axp_regulator_info *info = NULL;
	u8 val, mask;
	s32 ret, vol;

	info = find_info(dev_info, id);

	ret = twi_byte_rw(TWI_OP_RD, dev_info->pmu_addr, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1) << info->vol_shift;
	val = (val & mask) >> info->vol_shift;
	vol = info->min_uv + info->step1_uv * val;

	return vol;
}

s32 axp_set_state(struct axp_dev_info *dev_info, u32 id, u32 state)
{
	struct axp_regulator_info *info = NULL;
	s32 ret = -1;
	uint8_t reg_val = 0;

	info = find_info(dev_info, id);

	if (0 == state)
		reg_val = info->disable_val;
	else
		reg_val = info->enable_val;

	return __twi_byte_update(dev_info->pmu_addr, info->enable_reg,
				reg_val, info->enable_mask);
}

s32 axp_get_state(struct axp_dev_info *dev_info, u32 id)
{
	struct axp_regulator_info *info = NULL;
	s32 ret = -1;
	uint8_t reg_val;

	info = find_info(dev_info, id);

	ret = twi_byte_rw(TWI_OP_RD, dev_info->pmu_addr,
				info->enable_reg, &reg_val);
	if (ret)
		return ret;

	if ((reg_val & info->enable_mask) == info->enable_val)
		return 1;
	else
		return 0;
}

s32 __twi_byte_update(u8 saddr, u8 reg, u8 val, u8 mask)
{
	u8 reg_val = 0;
	s32 ret = 0;

	ret = twi_byte_rw(TWI_OP_RD, saddr, reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | val;
		ret = twi_byte_rw(TWI_OP_WR, saddr, reg, &reg_val);
	}

out:
	return ret;
}
