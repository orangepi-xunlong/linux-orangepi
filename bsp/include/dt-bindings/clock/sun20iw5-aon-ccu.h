// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 rengaomin@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN20IW5_AON_H_
#define _DT_BINDINGS_CLK_SUN20IW5_AON_H_

#define CLK_PLL_CPU		0
#define CLK_PLL_DDR		1
#define CLK_PLL_PERI_PARENT	2
#define CLK_PLL_PERI_CKO_1536M 	3
#define CLK_PLL_PERI_CKO_1024M 	4
#define CLK_PLL_PERI_CKO_768M 	5
#define CLK_PLL_PERI_CKO_614M 	6
#define CLK_PLL_PERI_CKO_512M 	7
#define CLK_PLL_PERI_CKO_384M 	8
#define CLK_PLL_PERI_CKO_341M 	9
#define CLK_PLL_PERI_CKO_307M 	10
#define CLK_PLL_PERI_CKO_236M 	11
#define CLK_PLL_PERI_CKO_219M 	12
#define CLK_PLL_PERI_CKO_192M 	13
#define CLK_PLL_PERI_CKO_118M 	14
#define CLK_PLL_PERI_CKO_96M 	15
#define CLK_PLL_PERI_CKO_48M 	16
#define CLK_PLL_PERI_CKO_24M 	17
#define CLK_PLL_PERI_CKO_12M 	18
#define CLK_PLL_VIDEO0_4X	19
#define CLK_PLL_CSI_4X		20
#define CLK_DCXO		21
#define CLK_AHB			22
#define CLK_APB0		23
#define CLK_RTC_APB		24
#define CLK_PWRCTRL		25
#define CLK_TCCAL		26
#define CLK_APB_SPC		27
#define CLK_E907		28
#define CLK_A27L2		29

#define CLK_MAX_NO		(CLK_A27L2 + 1)

#endif /* _DT_BINDINGS_CLK_SUN20IW5_AON_H_ */
