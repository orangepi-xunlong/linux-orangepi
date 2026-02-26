/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's errcode for audio
 *
 * Copyright (c) 2023, zhouxijing <zhouxijing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __SUNXI_ERR_PMIC_H__
#define __SUNXI_ERR_PMIC_H__

#define E_PMIC_TYPE_OFFSET	14
#define E_PMIC(type, user)	(((type & 0x3) << E_PMIC_TYPE_OFFSET) | (user & 0xFFF))

enum sunxi_err_pmic_type {
	PMIC_NORMAL = 0x0,
	PMU_EXT,
	BMU_EXT,
	PMIC_EXT,
};

enum sunxi_err_pmic_func {
	/* platform */
	PMIC_PROBE = 0x0,

	/* i2c read/write */
	I2C_READ,
	I2C_WRITE,

	E_PMIC_FUNC_END = 0xFFF,
};

enum sunxi_err_pmic {
	E_PMIC_DEP_I2C_READ_ERR           = E_PMIC_SW_DEP_ERR0 | E_PMIC(PMIC_NORMAL, I2C_READ),
	E_PMIC_DEP_I2C_WRITE_ERR           = E_PMIC_SW_DEP_ERR0 | E_PMIC(PMIC_NORMAL, I2C_WRITE),

	/* PMIC_NORMAL */
	E_PMIC_NORMAL_MFD_SYS_PORBE_ERR           = E_PMIC_MFD_SW_SYS_ERR0 | E_PMIC(PMIC_NORMAL, PMIC_PROBE),
	E_PMIC_NORMAL_REGU_SYS_PORBE_ERR           = E_PMIC_REGU_SW_SYS_ERR0 | E_PMIC(PMIC_NORMAL, PMIC_PROBE),
	E_PMIC_NORMAL_SUPPLY_SYS_PORBE_ERR           = E_PMIC_SUPPLY_SW_SYS_ERR0 | E_PMIC(PMIC_NORMAL, PMIC_PROBE),
	E_PMIC_NORMAL_KEY_SYS_PORBE_ERR           = E_PMIC_KEY_SW_SYS_ERR0 | E_PMIC(PMIC_NORMAL, PMIC_PROBE),
	E_PMIC_NORMAL_NOTIFIER_SYS_PORBE_ERR           = E_PMIC_NOTIFIER_SW_SYS_ERR0 | E_PMIC(PMIC_NORMAL, PMIC_PROBE),

	/* PMU_EXT */
	E_PMU_EXT_MFD_SYS_PORBE_ERR           = E_PMIC_MFD_SW_SYS_ERR0 | E_PMIC(PMU_EXT, PMIC_PROBE),
	E_PMU_EXT_REGU_SYS_PORBE_ERR           = E_PMIC_REGU_SW_SYS_ERR0 | E_PMIC(PMU_EXT, PMIC_PROBE),

	/* BMU_EXT */
	E_BMU_EXT_MFD_SYS_PORBE_ERR           = E_PMIC_MFD_SW_SYS_ERR0 | E_PMIC(BMU_EXT, PMIC_PROBE),
	E_BMU_EXT_REGU_SYS_PORBE_ERR           = E_PMIC_REGU_SW_SYS_ERR0 | E_PMIC(BMU_EXT, PMIC_PROBE),
	E_BMU_EXT_SUPPLY_SYS_PORBE_ERR           = E_PMIC_SUPPLY_SW_SYS_ERR0 | E_PMIC(BMU_EXT, PMIC_PROBE),
	E_BMU_EXT_KEY_SYS_PORBE_ERR           = E_PMIC_KEY_SW_SYS_ERR0 | E_PMIC(BMU_EXT, PMIC_PROBE),
	E_BMU_EXT_NOTIFIER_SYS_PORBE_ERR           = E_PMIC_NOTIFIER_SW_SYS_ERR0 | E_PMIC(BMU_EXT, PMIC_PROBE),
};

#endif
