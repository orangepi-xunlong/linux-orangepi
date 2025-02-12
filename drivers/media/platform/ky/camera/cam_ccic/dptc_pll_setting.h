/* SPDX-License-Identifier: GPL-2.0 */
/*
 * dptc_pll_setting.h - pll setting for dptc
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef DPTC_PLL_SETTING_H_
#define DPTC_PLL_SETTING_H_

enum {
	PLL_RATE_1620,
	PLL_RATE_2700,
	PLL_RATE_5400,
	PLL_RATE_LIMIT
};

enum {
	PLL_SSC_0,
	PLL_SSC_5000,
	PLL_SSC_LIMIT,
};

enum {
	DP_CLK_DSI	= 0x1,
	DP_CLK_CSI	= 0x2,
	DP_CLK_LS	= 0x4,
	DP_CLK_ESC	= 0x8,
	DP_CLK_20X_40X	= 0x10,
	DP_CLK_LIMIT
};

#define DP_CLK_ALL		(DP_CLK_DSI|DP_CLK_CSI|DP_CLK_LS|DP_CLK_ESC|DP_CLK_20X_40X)
#define DP_CLK_FUNC3	DP_CLK_ESC	//(DP_CLK_CSI|DP_CLK_LS|DP_CLK_ESC)//(DP_CLK_CSI|DP_CLK_ESC)

#define PLL_CLK_20X_40X	BIT(1)
#define PLL_CLK_ESC		BIT(2)
#define PLL_CLK_LS		BIT(3)
#define PLL_CLK_CSI		BIT(4)
#define PLL_CLK_DSI		BIT(5)

struct s_pll_setting {
	uint8_t reg0;
	uint8_t reg1;
	uint8_t reg2;
	uint8_t reg3;
	uint8_t reg4;
	uint8_t reg5;
	uint8_t reg6;
	uint8_t reg7;
	uint8_t reg8;
	uint8_t reserved1;
	uint16_t reserved2;
};

#endif /*DPTC_PLL_SETTING_H_ */
