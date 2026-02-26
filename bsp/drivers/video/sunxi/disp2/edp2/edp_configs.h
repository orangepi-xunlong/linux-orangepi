/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * edp_configs.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __DE_EDP_CONFIG_H__
#define __DE_EDP_CONFIG_H__

/*edp bit rate  unit:Hz*/
#define BIT_RATE_1G62 ((unsigned long long)1620 * 1000 * 1000)
#define BIT_RATE_2G7  ((unsigned long long)2700 * 1000 * 1000)
#define BIT_RATE_5G4  ((unsigned long long)5400 * 1000 * 1000)
#define BIT_RATE_8G1  ((unsigned long long)8100 * 1000 * 1000)

enum disp_edp_colordepth {
	EDP_8_BIT = 0,
	EDP_10_BIT = 1,
};



#endif /*End of file*/
