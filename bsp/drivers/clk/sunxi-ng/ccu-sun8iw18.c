//SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun8iw18.h"

/*
 * The CPU PLL is actually NP clock, with P being /1, /2 or /4. However
 * P should only be used for output frequencies lower than 288 MHz.
 *
 * For now we can just model it as a multiplier clock, and force P to /1.
 *
 * The M factor is present in the register's description, but not in the
 * frequency formula, and it's documented as "M is only used for backdoor
 * testing", so it's not modelled and then force to 0.
 */

/* ccu_des_start */
#define SUN8IW18_PLL_CPU0_CTRL_REG   0x0000
static struct ccu_mult pll_cpu0_clk = {
	.enable         = BIT(27),
	.lock           = BIT(28),
	.mult           = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.common         = {
		.reg            = 0x0000,
		.hw.init        = CLK_HW_INIT("pll-cpu0", "dcxo24M",
				&ccu_mult_ops,
				CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW18_PLL_DDR_CTRL_REG   0x0010
static struct ccu_nkmp pll_ddr_clk = {
	.enable         = BIT(27),
	.lock           = BIT(28),
	.n              = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m              = _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p              = _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common         = {
		.reg            = 0x0010,
		.hw.init        = CLK_HW_INIT("pll-ddr", "dcxo24M",
				&ccu_nkmp_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

#define SUN8IW18_PLL_PERI0_CTRL_REG   0x0020
static struct ccu_nm pll_peri0_2x_clk = {
	.enable         = BIT(27),
	.lock           = BIT(28),
	.n              = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m              = _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common         = {
		.reg            = 0x0020,
		.hw.init        = CLK_HW_INIT("pll-peri0-2x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_peri0_1x_clk, "pll-peri0-1x",
		&pll_peri0_2x_clk.common.hw,
		2, 1, 0);

#define SUN8IW18_PLL_PERI1_CTRL_REG   0x0028
static struct ccu_nm pll_peri1_2x_clk = {
	.enable         = BIT(27),
	.lock           = BIT(28),
	.n              = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m              = _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common         = {
		.reg            = 0x0028,
		.hw.init        = CLK_HW_INIT("pll-peri1-2x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static CLK_FIXED_FACTOR_HW(pll_peri1_1x_clk, "pll-peri1-1x",
		&pll_peri1_2x_clk.common.hw,
		2, 1, 0);

#define SUN8IW18_PLL_AUDIO0_REG         0x078
static struct ccu_sdm_setting pll_audio0_sdm_table[] = {
	{ .rate = 45158400, .pattern = 0xc001bcd3, .m = 18, .n = 33 },
	{ .rate = 49152000, .pattern = 0xc001eb85, .m = 20, .n = 40 },
	{ .rate = 180633600, .pattern = 0xc001288d, .m = 3, .n = 22 },
	{ .rate = 196608000, .pattern = 0xc001eb85, .m = 5, .n = 40 },
};

static struct ccu_nm pll_audio0_4x_clk = {
	.enable         = BIT(27),
	.lock           = BIT(28),
	.n              = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m              = _SUNXI_CCU_DIV(16, 6),
	.fixed_post_div = 2,
	.sdm            = _SUNXI_CCU_SDM(pll_audio0_sdm_table, BIT(24),
			0x178, BIT(31)),
	.common         = {
		.reg            = 0x078,
		.features       = CCU_FEATURE_FIXED_POSTDIV |
			CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init        = CLK_HW_INIT("pll-audio0-4x", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_audio0_2x_clk, "pll-audio0-2x",
		&pll_audio0_4x_clk.common.hw,
		2, 1, 0);

#define SUN8IW18_PLL_32K_CTRL_REG   0x00d8
static struct ccu_nkmp pll_32k_ctrl_clk = {
	.enable         = BIT(27),
	.lock           = BIT(28),
	.n              = _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m              = _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p              = _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common         = {
		.reg            = 0x00d8,
		.hw.init        = CLK_HW_INIT("pll-32k", "dcxo24M",
				&ccu_nm_ops,
				CLK_SET_RATE_UNGATE |
				CLK_IS_CRITICAL),
	},
};

static const char * const cpux_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "pll-cpu0", "pll-peri0-1x"};

static SUNXI_CCU_MUX(cpux_clk, "cpux", cpux_parents,
		0x0500, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static const char * const psi_ahb1_ahb2_parents[] = { "dcxo24M", "rtc-32k", "rc16m", "pll-peri0-1x"};

static SUNXI_CCU_MP_WITH_MUX(psi_ahb1_ahb2_clk, "psi-ahb1-ahb2",
		psi_ahb1_ahb2_parents,
		0x0510,
		0, 2,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const ahb3_parents[] = { "dcxo24M", "rtc-32k", "psi-ahb1-ahb2", "pll-peri0-1x"};

static SUNXI_CCU_MP_WITH_MUX(ahb3_clk, "ahb3",
		ahb3_parents,
		0x051C,
		0, 2,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb1_parents[] = { "dcxo24M", "rtc-32k", "psi-ahb1-ahb2", "pll-peri0-1x"};

static SUNXI_CCU_MP_WITH_MUX(apb1_clk, "apb1",
		apb1_parents,
		0x520,
		0, 2,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const apb2_parents[] = { "dcxo24M", "rtc-32k", "psi-ahb1-ahb2", "pll-peri0-1x"};

static SUNXI_CCU_MP_WITH_MUX(apb2_clk, "apb2",
		apb2_parents,
		0x524,
		0, 2,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		0);

static const char * const ce_clk_parents[] = {"dcxo24M", "pll-peri0-2x"};

static SUNXI_CCU_MP_WITH_MUX_GATE(ce_clk, "ce",
		ce_clk_parents, 0x0680,
		0, 4,	/* M */
		8, 2,	/* P */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(ce_bus_clk, "ce-bus",
		"dcxo24M",
		0x068C, BIT(0), 0);

static SUNXI_CCU_GATE(dma_bus_clk, "dma-bus",
		"dcxo24M",
		0x070C, BIT(0), 0);

static SUNXI_CCU_GATE(hstimer_bus_clk, "hstimer-bus",
		"dcxo24M",
		0x073C, BIT(0), 0);

static SUNXI_CCU_GATE(avs_clk, "avs",
		"dcxo24M",
		0x0740, BIT(31), 0);

static SUNXI_CCU_GATE(dbgsys_clk, "dbgsys",
		"dcxo24M",
		0x078C, BIT(0), 0);

static SUNXI_CCU_GATE(psi_clk, "psi",
		"dcxo24M",
		0x079C, BIT(0), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
		"dcxo24M",
		0x07AC, BIT(0), 0);

static const char * const dram_clk_parents[] = {"hosc", "pll-ddr"};

static SUNXI_CCU_M_WITH_MUX_GATE(dram_clk, "dram",
		dram_clk_parents, 0x0800,
		0, 2,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(mbus_nand0_clk, "mbus-nand0",
		"dcxo24M",
		0x0804, BIT(5), 0);

static SUNXI_CCU_GATE(mbus_ce_clk, "mbus-ce",
		"dcxo24M",
		0x0804, BIT(2), 0);

static SUNXI_CCU_GATE(mbus_dma_clk, "mbus-dma",
		"dcxo24M",
		0x0804, BIT(0), 0);

static SUNXI_CCU_GATE(mbus_dram_clk, "mbus-dram",
		"dcxo24M",
		0x080C, BIT(0), 0);

static const char * const nand0_0_clk_parents[] = {"dcxo24M", "pll-peri0-1x", "pll-peri1-1x", "pll-peri0-2x", "pll-peri1-2x"};

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_0_clk, "nand0-0",
		nand0_0_clk_parents, 0x0810,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const nand0_1_clk_parents[] = {"dcxo24M", "pll-peri0-1x", "pll-peri1-1x", "pll-peri0-2x", "pll-peri1-2x"};

static SUNXI_CCU_M_WITH_MUX_GATE(nand0_1_clk, "nand0-1",
		nand0_1_clk_parents, 0x0814,
		0, 4,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(nand0_bus_clk, "nand0-bus",
		"dcxo24M",
		0x082C, BIT(0), 0);

static const char * const smhc1_clk_parents[] = { "dcxo24M", "pll-peri0-2x", "pll-peri1-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(smhc1_clk, "smhc1",
		smhc1_clk_parents, 0x0834,
		0, 4,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(smhc2_bus_clk, "smhc2-bus",
		"dcxo24M",
		0x084C, BIT(2), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc1-bus",
		"dcxo24M",
		0x084C, BIT(1), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc0-bus",
		"dcxo24M",
		0x084C, BIT(0), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo24M",
		0x090C, BIT(3), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo24M",
		0x090C, BIT(2), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo24M",
		0x090C, BIT(1), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo24M",
		0x090C, BIT(0), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo24M",
		0x091C, BIT(2), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo24M",
		0x091C, BIT(1), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo24M",
		0x091C, BIT(0), 0);

static const char * const spi0_clk_parents[] = { "dcxo24M", "pll-peri0-1x", "pll-peri1-1x", "pll-peri0-2x", "pll-peri1-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi0_clk, "spi0", spi0_clk_parents, 0x0940,
				0, 4,/* M */
				8, 2,/* P */
				24, 3,/* mux */
				BIT(31),/* gate */
				0);

static const char * const spi1_clk_parents[] = { "dcxo24M", "pll-peri0-1x", "pll-peri1-1x", "pll-peri0-2x", "pll-peri1-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi1_clk, "spi1", spi1_clk_parents, 0x0944,
				0, 4,/* M */
				8, 2,/* P */
				24, 3,/* mux */
				BIT(31),/* gate */
				0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-bus",
		"dcxo24M",
		0x096C, BIT(1), 0);

static SUNXI_CCU_GATE(spi0_bus_clk, "spi0-bus",
		"dcxo24M",
		0x096C, BIT(0), 0);

static SUNXI_CCU_GATE(gpadc_bus_clk, "gpadc-bus",
		"dcxo24M",
		0x09EC, BIT(0), 0);

static SUNXI_CCU_GATE(ths_bus_clk, "ths-bus",
		"dcxo24M",
		0x09FC, BIT(0), 0);

static const char * const i2s_pcm0_clk_parents[] = { "pll-audio0-4x", "pll-32k" };
static struct ccu_div i2s_pcm0_clk = {
	.enable         = BIT(31),
	.div            = _SUNXI_CCU_DIV_TABLE_FLAGS(8, 2, 0, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 2),
	.common         = {
		.reg            = 0x0A10,
		.hw.init        = CLK_HW_INIT_PARENTS("i2s-pcm0", i2s_pcm0_clk_parents,
				&ccu_div_ops, 0),
	},
};

static const char * const i2s_pcm1_clk_parents[] = { "pll-audio0-4x", "pll-32k" };
static struct ccu_div i2s_pcm1_clk = {
	.enable         = BIT(31),
	.div            = _SUNXI_CCU_DIV_TABLE_FLAGS(8, 2, 0, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 2),
	.common         = {
		.reg            = 0x0A14,
		.hw.init        = CLK_HW_INIT_PARENTS("i2s-pcm1", i2s_pcm1_clk_parents,
				&ccu_div_ops,
				0),
	},
};

static const char * const i2s_pcm2_clk_parents[] = { "pll-audio0-4x", "pll-32k" };
static struct ccu_div i2s_pcm2_clk = {
	.enable         = BIT(31),
	.div            = _SUNXI_CCU_DIV_TABLE_FLAGS(8, 2, 0, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 2),
	.common         = {
		.reg            = 0x0A18,
		.hw.init        = CLK_HW_INIT_PARENTS("i2s-pcm2", i2s_pcm2_clk_parents,
				&ccu_div_ops,
				0),
	},
};

static SUNXI_CCU_GATE(i2s_pcm0_bus_clk, "i2s-pcm0-bus",
		"dcxo24M",
		0x0A1C, BIT(0), 0);

static SUNXI_CCU_GATE(i2s_pcm1_bus_clk, "i2s-pcm1-bus",
		"dcxo24M",
		0x0A1C, BIT(1), 0);

static SUNXI_CCU_GATE(i2s_pcm2_bus_clk, "i2s-pcm2-bus",
		"dcxo24M",
		0x0A1C, BIT(2), 0);

static const char * const spdif_clk_parents[] = { "pll-audio0-4x", "pll-32k" };
static struct ccu_div spdif_clk = {
	.enable         = BIT(31),
	.div            = _SUNXI_CCU_DIV_TABLE_FLAGS(8, 2, 0, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 2),
	.common         = {
		.reg            = 0x0A20,
		.hw.init        = CLK_HW_INIT_PARENTS("spdif", spdif_clk_parents,
				&ccu_div_ops,
				0),
	},
};

static SUNXI_CCU_GATE(spdif_bus_clk, "spdif-bus",
		"dcxo24M",
		0x0A2C, BIT(0), 0);

static const char * const dmic_clk_parents[] = { "pll-audio0-4x", "pll-32k" };
static struct ccu_div dmic_clk = {
	.enable         = BIT(31),
	.div            = _SUNXI_CCU_DIV_TABLE_FLAGS(8, 2, 0, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(24, 2),
	.common         = {
		.reg            = 0x0A40,
		.hw.init        = CLK_HW_INIT_PARENTS("dmic", dmic_clk_parents,
				&ccu_div_ops,
				0),
	},
};

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo24M",
		0x0A4C, BIT(0), 0);

static const char * const audio_codec_1x_clk_parents[] = { "pll-audio0-4x", "pll-32k" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_1x_clk, "audio-codec-1x-clk",
		audio_codec_1x_clk_parents, 0x0A50,
		0, 4,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const audio_codec_4x_clk_parents[] = { "pll-audio0-4x", "pll-32k" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_codec_4x_clk, "audio-codec-4x-clk",
		audio_codec_4x_clk_parents, 0x0A54,
		0, 4,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x0A5C, BIT(0), 0);

#define	SUN8IW18_USB0_CTRL_REG	0x0A70
static SUNXI_CCU_GATE(usb0_clk, "usb0",
		"dcxo24M",
		0x0A70, BIT(31), 0);

static SUNXI_CCU_GATE(usbphy0_clk, "usbpyh0",
		"dcxo24M",
		0x0A70, BIT(29), 0);

static SUNXI_CCU_GATE(usbotg_clk, "usbotg",
		"dcxo24M",
		0x0A8C, BIT(8), 0);

static SUNXI_CCU_GATE(usbehci1_clk, "usbehci1",
		"dcxo24M",
		0x0A8C, BIT(5), 0);

static SUNXI_CCU_GATE(usbehci0_clk, "usbehci0",
		"dcxo24M",
		0x0A8C, BIT(4), 0);

static SUNXI_CCU_GATE(usbohci1_clk, "usbohci1",
		"dcxo24M",
		0x0A8C, BIT(1), 0);

static SUNXI_CCU_GATE(usbohci0_clk, "usbohci0",
		"dcxo24M",
		0x0A8C, BIT(0), 0);

static SUNXI_CCU_GATE(mad_cfg_clk, "mad-cfg",
		"dcxo24M",
		0x0ACC, BIT(2), 0);

static SUNXI_CCU_GATE(mad_ad_clk, "mad-ad",
		"dcxo24M",
		0x0ACC, BIT(1), 0);

static SUNXI_CCU_GATE(mad_clk, "mad",
		"dcxo24M",
		0x0ACC, BIT(0), 0);

static const char * const lpsd_parents[] = { "dcxo24M", "pll-audio-1x", "pll-32k" };
static SUNXI_CCU_MP_WITH_MUX_GATE(lpsd_clk, "lpsd",
		lpsd_parents,
		0x0ad0,
		0, 4,	/* M */
		8, 2,	/* P */
		24, 2,	/* mux */
		BIT(31), 0);

static const char * const led_parents[] = { "dcxo24M", "pll-audio-1x"};
static SUNXI_CCU_MP_WITH_MUX_GATE(ledc_clk, "ledc",
		led_parents,
		0x0bf0,
		0, 4,	/* M */
		8, 2,	/* P */
		24, 1,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(ledc_bus_clk, "ledc-bus",
		"dcxo24M",
		0x0BFC, BIT(0), 0);

static SUNXI_CCU_GATE(acodec_24m_clk, "acodec-24m",
		"dcxo24M",
		0x0F30, BIT(16), 0);

/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun8iw18_ccu_resets[] = {
	[RST_MBUS]			= { 0x540, BIT(30) },
	[RST_BUS_CE]			= { 0x068c, BIT(16) },
	[RST_BUS_DMA]			= { 0x070c, BIT(16) },
	[RST_BUS_HSTIME]		= { 0x073c, BIT(16) },
	[RST_BUS_DBGSY]			= { 0x078c, BIT(16) },
	[RST_BUS_PSI]			= { 0x079c, BIT(16) },
	[RST_BUS_PWM]			= { 0x07ac, BIT(16) },
	[RST_BUS_MODULE]		= { 0x0800, BIT(30) },
	[RST_BUS_DRAM]			= { 0x080c, BIT(16) },
	[RST_BUS_NAND0]			= { 0x082c, BIT(16) },
	[RST_BUS_SMHC2]			= { 0x084c, BIT(18) },
	[RST_BUS_SMHC1]			= { 0x084c, BIT(17) },
	[RST_BUS_SMHC0]			= { 0x084c, BIT(16) },
	[RST_BUS_UART3]			= { 0x090c, BIT(19) },
	[RST_BUS_UART2]			= { 0x090c, BIT(18) },
	[RST_BUS_UART1]			= { 0x090c, BIT(17) },
	[RST_BUS_UART0]			= { 0x090c, BIT(16) },
	[RST_BUS_TWI2]			= { 0x091c, BIT(18) },
	[RST_BUS_TWI1]			= { 0x091c, BIT(17) },
	[RST_BUS_TWI0]			= { 0x091c, BIT(16) },
	[RST_BUS_SPI1]			= { 0x096c, BIT(17) },
	[RST_BUS_SPI0]			= { 0x096c, BIT(16) },
	[RST_BUS_GPADC]			= { 0x09ec, BIT(16) },
	[RST_BUS_THS]			= { 0x09fc, BIT(16) },
	[RST_BUS_I2S_PCM0]		= { 0x0a1c, BIT(16) },
	[RST_BUS_I2S_PCM1]		= { 0x0a1c, BIT(17) },
	[RST_BUS_I2S_PCM2]		= { 0x0a1c, BIT(18) },
	[RST_BUS_SPDIF]			= { 0x0a2c, BIT(16) },
	[RST_BUS_DMIC]			= { 0x0a4c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]		= { 0x0a5c, BIT(16) },
	[RST_USB_PHY0]			= { 0x0a70, BIT(30) },
	[RST_USB_OTG]			= { 0x0a8c, BIT(24) },
	[RST_USB_EHCI1]			= { 0x0a8c, BIT(21) },
	[RST_USB_EHCI0]			= { 0x0a8c, BIT(20) },
	[RST_USB_OHCI1]			= { 0x0a8c, BIT(17) },
	[RST_USB_OHCI0]			= { 0x0a8c, BIT(16) },
	[RST_BUS_MAD_CFG]		= { 0x0acc, BIT(18) },
	[RST_BUS_MAD_AD]		= { 0x0acc, BIT(17) },
	[RST_BUS_MAD]			= { 0x0acc, BIT(16) },
	[RST_BUS_LEDC]			= { 0x0bfc, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */

static struct clk_hw_onecell_data sun8iw18_hw_clks = {
	.hws    = {
		[CLK_PLL_CPU0]			= &pll_cpu0_clk.common.hw,
		[CLK_PLL_DDR]			= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERI0_2X]		= &pll_peri0_2x_clk.common.hw,
		[CLK_PLL_PERI0_1X]		= &pll_peri0_1x_clk.hw,
		[CLK_PLL_PERI1_2X]		= &pll_peri1_2x_clk.common.hw,
		[CLK_PLL_PERI1_1X]		= &pll_peri1_1x_clk.hw,
		[CLK_PLL_AUDIO0_4X]		= &pll_audio0_4x_clk.common.hw,
		[CLK_PLL_AUDIO0_2X]		= &pll_audio0_2x_clk.hw,
		[CLK_PLL_32K_CTRL]		= &pll_32k_ctrl_clk.common.hw,
		[CLK_CPUX]			= &cpux_clk.common.hw,
		[CLK_PSI_AHB1_AHB2]		= &psi_ahb1_ahb2_clk.common.hw,
		[CLK_AHB3]			= &ahb3_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB2]			= &apb2_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_BUS_CE]			= &ce_bus_clk.common.hw,
		[CLK_BUS_DMA]			= &dma_bus_clk.common.hw,
		[CLK_BUS_HSTIMER]		= &hstimer_bus_clk.common.hw,
		[CLK_AVS]			= &avs_clk.common.hw,
		[CLK_DBGSYS]			= &dbgsys_clk.common.hw,
		[CLK_PSI]			= &psi_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_MBUS_NAND0]		= &mbus_nand0_clk.common.hw,
		[CLK_MBUS_CE]			= &mbus_ce_clk.common.hw,
		[CLK_MBUS_DMA]			= &mbus_dma_clk.common.hw,
		[CLK_MBUS_DRAM]			= &mbus_dram_clk.common.hw,
		[CLK_NAND0_0]			= &nand0_0_clk.common.hw,
		[CLK_NAND0_1]			= &nand0_1_clk.common.hw,
		[CLK_BUS_NAND0]			= &nand0_bus_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_BUS_SMHC2]			= &smhc2_bus_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_BUS_UART3]			= &uart3_clk.common.hw,
		[CLK_BUS_UART2]			= &uart2_clk.common.hw,
		[CLK_BUS_UART1]			= &uart1_clk.common.hw,
		[CLK_BUS_UART0]			= &uart0_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_BUS_SPI0]			= &spi0_bus_clk.common.hw,
		[CLK_BUS_GPADC]			= &gpadc_bus_clk.common.hw,
		[CLK_BUS_THS]			= &ths_bus_clk.common.hw,
		[CLK_I2S_PCM0]			= &i2s_pcm0_clk.common.hw,
		[CLK_I2S_PCM1]			= &i2s_pcm1_clk.common.hw,
		[CLK_I2S_PCM2]			= &i2s_pcm2_clk.common.hw,
		[CLK_BUS_I2S_PCM0]		= &i2s_pcm0_bus_clk.common.hw,
		[CLK_BUS_I2S_PCM1]		= &i2s_pcm1_bus_clk.common.hw,
		[CLK_BUS_I2S_PCM2]		= &i2s_pcm2_bus_clk.common.hw,
		[CLK_SPDIF]			= &spdif_clk.common.hw,
		[CLK_BUS_SPDIF]			= &spdif_bus_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_AUDIO_CODEC_1X]		= &audio_codec_1x_clk.common.hw,
		[CLK_AUDIO_CODEC_4X]		= &audio_codec_4x_clk.common.hw,
		[CLK_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_USB0]			= &usb0_clk.common.hw,
		[CLK_USBPHY0]			= &usbphy0_clk.common.hw,
		[CLK_USBOTG]			= &usbotg_clk.common.hw,
		[CLK_USBEHCI1]			= &usbehci1_clk.common.hw,
		[CLK_USBEHCI0]			= &usbehci0_clk.common.hw,
		[CLK_USBOHCI1]			= &usbohci1_clk.common.hw,
		[CLK_USBOHCI0]			= &usbohci0_clk.common.hw,
		[CLK_MAD_CFG]			= &mad_cfg_clk.common.hw,
		[CLK_MAD_AD]			= &mad_ad_clk.common.hw,
		[CLK_MAD]			= &mad_clk.common.hw,
		[CLK_LPSD]			= &lpsd_clk.common.hw,
		[CLK_LEDC]			= &ledc_clk.common.hw,
		[CLK_BUS_LEDC]			= &ledc_bus_clk.common.hw,
		[CLK_ACODEC_24M]		= &acodec_24m_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun8iw18_ccu_clks[] = {
	&pll_cpu0_clk.common,
	&pll_ddr_clk.common,
	&pll_peri0_2x_clk.common,
	&pll_peri1_2x_clk.common,
	&pll_audio0_4x_clk.common,
	&pll_32k_ctrl_clk.common,
	&cpux_clk.common,
	&psi_ahb1_ahb2_clk.common,
	&ahb3_clk.common,
	&apb1_clk.common,
	&apb2_clk.common,
	&ce_clk.common,
	&ce_bus_clk.common,
	&dma_bus_clk.common,
	&hstimer_bus_clk.common,
	&avs_clk.common,
	&dbgsys_clk.common,
	&psi_clk.common,
	&pwm_clk.common,
	&dram_clk.common,
	&mbus_nand0_clk.common,
	&mbus_ce_clk.common,
	&mbus_dma_clk.common,
	&mbus_dram_clk.common,
	&nand0_0_clk.common,
	&nand0_1_clk.common,
	&nand0_bus_clk.common,
	&smhc1_clk.common,
	&smhc2_bus_clk.common,
	&smhc1_bus_clk.common,
	&smhc0_bus_clk.common,
	&uart3_clk.common,
	&uart2_clk.common,
	&uart1_clk.common,
	&uart0_clk.common,
	&twi2_clk.common,
	&twi1_clk.common,
	&twi0_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spi1_bus_clk.common,
	&spi0_bus_clk.common,
	&gpadc_bus_clk.common,
	&ths_bus_clk.common,
	&i2s_pcm0_clk.common,
	&i2s_pcm1_clk.common,
	&i2s_pcm2_clk.common,
	&i2s_pcm0_bus_clk.common,
	&i2s_pcm1_bus_clk.common,
	&i2s_pcm2_bus_clk.common,
	&spdif_clk.common,
	&spdif_bus_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&audio_codec_1x_clk.common,
	&audio_codec_4x_clk.common,
	&audio_codec_clk.common,
	&usb0_clk.common,
	&usbphy0_clk.common,
	&usbotg_clk.common,
	&usbehci1_clk.common,
	&usbehci0_clk.common,
	&usbohci1_clk.common,
	&usbohci0_clk.common,
	&mad_cfg_clk.common,
	&mad_ad_clk.common,
	&mad_clk.common,
	&lpsd_clk.common,
	&ledc_clk.common,
	&ledc_bus_clk.common,
	&acodec_24m_clk.common,
};

static const struct sunxi_ccu_desc sun8iw18_ccu_desc = {
	.ccu_clks       = sun8iw18_ccu_clks,
	.num_ccu_clks   = ARRAY_SIZE(sun8iw18_ccu_clks),

	.hw_clks        = &sun8iw18_hw_clks,

	.resets         = sun8iw18_ccu_resets,
	.num_resets     = ARRAY_SIZE(sun8iw18_ccu_resets),
};

static const u32 sun8iw18_pll_regs[] = {
	SUN8IW18_PLL_CPU0_CTRL_REG,
	SUN8IW18_PLL_DDR_CTRL_REG,
	SUN8IW18_PLL_PERI0_CTRL_REG,
	SUN8IW18_PLL_PERI1_CTRL_REG,
	SUN8IW18_PLL_AUDIO0_REG,
	SUN8IW18_PLL_32K_CTRL_REG,
};

static const u32 sun8iw18_usb_clk_regs[] = {
	SUN8IW18_USB0_CTRL_REG,
};

static void __init of_sun8iw18_ccu_init(struct device_node *node)
{
	void __iomem *reg;
	int i;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return;

	/* Enable the lock bits on all PLLs */
	for (i = 0; i < ARRAY_SIZE(sun8iw18_pll_regs); i++) {
		set_reg(reg + sun8iw18_pll_regs[i], 1, 1, 29);
	}

	/* Enforce m1 = 0, m0 = 1 for Audio PLL */
	set_reg(reg + SUN8IW18_PLL_AUDIO0_REG, 0x1, 2, 0);
	for (i = 0; i < ARRAY_SIZE(sun8iw18_usb_clk_regs); i++) {
		set_reg(reg + sun8iw18_usb_clk_regs[i], 0x0, 2, 24);
	}

	sunxi_ccu_probe(node, reg, &sun8iw18_ccu_desc);

	sunxi_ccu_sleep_init(reg, sun8iw18_ccu_clks,
			ARRAY_SIZE(sun8iw18_ccu_clks),
			NULL, 0);

	return;
}

CLK_OF_DECLARE(sun8iw18_ccu_init, "allwinner,sun8iw18-ccu", of_sun8iw18_ccu_init);
MODULE_VERSION("1.0.6");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
