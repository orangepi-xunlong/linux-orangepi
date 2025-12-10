/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2025 Cix Technology Group Co., Ltd.
 *
 */
#ifndef __Z_IMX490_5300_9295_I2C_TABLES__
#define __Z_IMX490_5300_9295_I2C_TABLES__

#include "camera_common.h"

enum {
	Z_IMX490_5300_9295_MODE_30FPS,
	Z_IMX490_5300_9295_MODE_START_STREAM,
	Z_IMX490_5300_9295_MODE_STOP_STREAM,
};

static const int imx390_30fps[] = {
	30,
};

static const struct camera_common_frmfmt z_imx490_5300_9295_frmfmt[] = {
	{{2880, 1860}, imx390_30fps, 1, 0, Z_IMX490_5300_9295_MODE_30FPS},
};
#endif /* __Z_IMX490_5300_9295_I2C_TABLES__ */
