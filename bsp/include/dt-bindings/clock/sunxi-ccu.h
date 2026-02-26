// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 rengaomin@allwinnertech.com
 */

#ifndef __DT_SUNXI_CLK_H
#define __DT_SUNXI_CLK_H

/**
 *  TR_X represents different configurations in different boards
 *  Such as:
 *
 *  Type1 (for example: sun251w1)
 *  TR_0: DC=0
 *  TR_1: DC=1
 *  TR_2: 1bit
 *  TR_3: Nbit
 *
 *  Type2 (for example: sun65w1)
 *  TR_0: pulse swallow
 *  TR_1: 1bit
 *  TR_2: 2bit
 *  TR_3: 3bit
 *
 */
#define TR_0	0
#define TR_1	1
#define TR_2	2
#define TR_3	3

#define FREQ_31_5	0
#define FREQ_32		1
#define FREQ_32_5	2
#define FREQ_33		3

#define SDM_DIR_UP	0
#define SDM_DIR_DOWN	1
#define SDM_DIR_NONE	2

#endif
