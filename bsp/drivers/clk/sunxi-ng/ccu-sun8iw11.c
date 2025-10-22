// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
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

#include "ccu-sun8iw11.h"

/* ccu_des_start */
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
#define SUN8IW11_PLL_CLK_NUM		13
#define KEY_FIELD_MAGIC			0x16aa
#define SUN8IW11_PLL_SYS_32K_CTL_REG	0x310
#define SUN8IW11_PLL_LOCK_CTRL_REG	0x320

#define SUN8IW11_PLL_CPUX_REG		0x000
static struct ccu_nkmp pll_cpux_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 5, 10),
	.k		= _SUNXI_CCU_MULT(4, 2),
	.m		= _SUNXI_CCU_DIV(0, 2), /* input divider */
	.p		= _SUNXI_CCU_DIV(16, 2), /* output divider */
	.common		= {
		.reg		= 0x000,
		.hw.init	= CLK_HW_INIT("pll-cpux", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

#define SUN8IW11_PLL_AUDIO_REG		0x008
static struct ccu_sdm_setting pll_audio_sdm_table[] = {
	{ .rate = 45158400, .pattern = 0xc0010d84, .m = 7, .n = 6 },
	{ .rate = 49152000, .pattern = 0xc000ac02, .m = 13, .n = 13 },
};
static struct ccu_nm pll_audio_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT(8, 7),
	.m		= _SUNXI_CCU_DIV(16, 4), /* input divider */
	.fixed_post_div	= 2,
	.sdm		= _SUNXI_CCU_SDM(pll_audio_sdm_table, BIT(24),
				 0x284, BIT(31)),
	.common		= {
		.reg		= 0x008,
		.features	= CCU_FEATURE_FIXED_POSTDIV |
					CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init	= CLK_HW_INIT("pll-audio", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR(pll_audio_1x_clk, "pll-audio-1x", "pll-audio", 1, 1, 0);
static CLK_FIXED_FACTOR(pll_audio_4x_clk, "pll-audio-4x", "pll-audio", 1, 4, 0);
static CLK_FIXED_FACTOR(pll_audio_8x_clk, "pll-audio-8x", "pll-audio", 1, 8, 0);

#define SUN8IW11_PLL_VIDEO_REG		0x010
static struct ccu_nm pll_video0_1x_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 7, 8),
	.m		= _SUNXI_CCU_DIV(0, 4), /* input divider */
	.common		= {
		.reg		= 0x010,
		.hw.init	= CLK_HW_INIT("pll-video0-1x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video0_2x_clk, "pll-video0-2x",
			   &pll_video0_1x_clk.common.hw,
			   1, 2, 0);

#define SUN8IW11_PLL_VE_REG		0x018
static struct ccu_nm pll_ve_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT(8, 7),
	.m		= _SUNXI_CCU_DIV(0, 4), /* input divider */
	.common		= {
		.reg		= 0x018,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW11_PLL_DDR0		0x020
static struct ccu_nkmp pll_ddr0_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 5, 5),
	.k		= _SUNXI_CCU_MULT_MIN(4, 2, 2),
	.m		= _SUNXI_CCU_DIV(0, 2), /* input divider */
	.common		= {
		.reg		= 0x020,
		.hw.init	= CLK_HW_INIT("pll-ddr0", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

#define SUN8IW11_PLL_PERIPH0_REG	0x028
static struct ccu_nk pll_periph0_2x_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 5, 9),
	.k		= _SUNXI_CCU_MULT_MIN(4, 2, 2),
	.common		= {
		.reg		= 0x028,
		.hw.init	= CLK_HW_INIT("pll-periph0-2x", "dcxo24M",
					      &ccu_nk_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_periph0_1x_clk, "pll-periph0-1x",
			   &pll_periph0_2x_clk.common.hw,
			   2, 1, CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL);

#define SUN8IW11_PLL_PERIPH1_REG	0x02c
static struct ccu_nk pll_periph1_2x_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 5, 9),
	.k		= _SUNXI_CCU_MULT_MIN(4, 2, 2),
	.common		= {
		.reg		= 0x02c,
		.hw.init	= CLK_HW_INIT("pll-periph1-2x", "dcxo24M",
					      &ccu_nk_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_periph1_1x_clk, "pll-periph1-1x",
			   &pll_periph1_2x_clk.common.hw,
			   2, 1, 0);

#define SUN8IW11_PLL_VIDEO1_REG		0x030
static struct ccu_nm pll_video1_1x_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 7, 1),
	.m		= _SUNXI_CCU_DIV(0, 4), /* input divider */
	.common		= {
		.reg		= 0x030,
		.hw.init	= CLK_HW_INIT("pll-video1-1x", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_video1_2x_clk, "pll-video1-2x",
			   &pll_periph1_2x_clk.common.hw,
			   1, 2, 0);

#define SUN8IW11_PLL_SATA		0x034
static struct ccu_nkmp pll_sata_clk = {
	.enable		= BIT(31) | BIT(14),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 5, 1),
	.k		= _SUNXI_CCU_MULT_MIN(4, 2, 2),
	.m		= _SUNXI_CCU_DIV(0, 2), /* input divider */
	.fixed_post_div	= 6,
	.common		= {
		.reg		= 0x034,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-sata", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL),
	},
};

#define SUN8IW11_PLL_GPU_REG		0x038
static struct ccu_nm pll_gpu_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 7, 1),
	.m		= _SUNXI_CCU_DIV(0, 4), /* input divider */
	.common		= {
		.reg		= 0x038,
		.hw.init	= CLK_HW_INIT("pll-gpu", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW11_PLL_MIPI		0x040
static struct ccu_nkmp pll_mipi_clk = {
	.enable		= BIT(31) | BIT(23) | BIT(22),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT(8, 4),
	.k		= _SUNXI_CCU_MULT_MIN(4, 2, 2),
	.m		= _SUNXI_CCU_DIV(0, 4), /* input divider */
	.common		= {
		.reg		= 0x040,
		.hw.init	= CLK_HW_INIT("pll-mipi", "pll-video0-1x",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW11_PLL_DE_REG		0x048
static struct ccu_nm pll_de_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 7, 1),
	.m		= _SUNXI_CCU_DIV(0, 4), /* input divider */
	.common		= {
		.reg		= 0x048,
		.hw.init	= CLK_HW_INIT("pll-de", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN8IW11_PLL_DDR1_REG		0x04c
static struct ccu_nm pll_ddr1_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 7, 16),
	.m		= _SUNXI_CCU_DIV(0, 2), /* input divider */
	.common		= {
		.reg		= 0x04c,
		.hw.init	= CLK_HW_INIT("pll-ddr1", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static const char * const cpux_parents[] = {"osc32k", "dcxo24M", "pll-cpux", "pll-cpux"};
static SUNXI_CCU_MUX(cpux_clk, "cpux", cpux_parents,
		     0x050, 16, 2, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);

static SUNXI_CCU_M(axi_clk, "axi", "cpux", 0x500, 0, 2, 0);

static SUNXI_CCU_M(pll_periphahb0_clk, "pll-periphahb0", "pll-periph0-1x", 0x054, 6, 2, 0);

static const char * const ahb1_parents[] = {"osc32k", "dcxo24M", "axi", "pll-periphahb0"};
static SUNXI_CCU_M_WITH_MUX(ahb1_clk, "ahb1", ahb1_parents, 0x054,
			     4, 2,
			     12, 2,
			     CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL);

static SUNXI_CCU_M(apb1_clk, "apb1", "ahb1", 0x054, 8, 2, CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL);

static const char * const apb2_parents[] = {"osc32k", "dcxo24M", "pll-periph0-2x", "pll-periph0-2x"};

static SUNXI_CCU_MP_WITH_MUX(apb2_clk, "apb2",
			     apb2_parents,
			     0x058,
			     0, 5,	/* M */
			     16, 2,	/* P */
			     24, 2,	/* mux */
			     CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(usb_ohci2_clk, "usb-ohci2", "dcxo24M", 0x060, BIT(31), 0);
static SUNXI_CCU_GATE(usb_ohci1_clk, "usb-ohci1", "dcxo24M", 0x060, BIT(30), 0);
static SUNXI_CCU_GATE(usb_ohci0_clk, "usb-ohci0", "dcxo24M", 0x060, BIT(29), 0);
static SUNXI_CCU_GATE(bus_ehci2_clk, "bus-ehci2", "dcxo24M", 0x060, BIT(28), 0);
static SUNXI_CCU_GATE(bus_ehci1_clk, "bus-ehci1", "dcxo24M", 0x060, BIT(27), 0);
static SUNXI_CCU_GATE(bus_ehci0_clk, "bus-ehci0", "dcxo24M", 0x060, BIT(26), 0);
static SUNXI_CCU_GATE(bus_otg_clk, "bus-otg", "dcxo24M", 0x060, BIT(25), 0);
static SUNXI_CCU_GATE(bus_sata_clk, "bus-sata", "dcxo24M", 0x060, BIT(24), 0);
static SUNXI_CCU_GATE(bus_spi3_clk, "bus-spi3", "dcxo24M", 0x060, BIT(23), 0);
static SUNXI_CCU_GATE(bus_spi2_clk, "bus-spi2", "dcxo24M", 0x060, BIT(22), 0);
static SUNXI_CCU_GATE(bus_spi1_clk, "bus-spi1", "dcxo24M", 0x060, BIT(21), 0);
static SUNXI_CCU_GATE(bus_spi0_clk, "bus-spi0", "dcxo24M", 0x060, BIT(20), 0);
static SUNXI_CCU_GATE(bus_hstmr_clk, "bus-hstmr", "dcxo24M", 0x060, BIT(19), 0);
static SUNXI_CCU_GATE(bus_ts_clk, "bus-ts", "dcxo24M", 0x060, BIT(18), 0);
static SUNXI_CCU_GATE(bus_emac_clk, "bus-emac", "dcxo24M", 0x060, BIT(17), 0);
static SUNXI_CCU_GATE(bus_dram_clk, "bus-dram", "dcxo24M", 0x060, BIT(14), 0);
static SUNXI_CCU_GATE(bus_nand_clk, "bus-nand", "dcxo24M", 0x060, BIT(13), 0);
static SUNXI_CCU_GATE(bus_mmc3_clk, "bus-mmc3", "dcxo24M", 0x060, BIT(11), 0);
static SUNXI_CCU_GATE(bus_mmc2_clk, "bus-mmc2", "dcxo24M", 0x060, BIT(10), 0);
static SUNXI_CCU_GATE(bus_mmc1_clk, "bus-mmc1", "dcxo24M", 0x060, BIT(9), 0);
static SUNXI_CCU_GATE(bus_mmc0_clk, "bus-mmc0", "dcxo24M", 0x060, BIT(8), 0);
static SUNXI_CCU_GATE(bus_dma_clk, "bus-dma", "dcxo24M", 0x060, BIT(6), 0);
static SUNXI_CCU_GATE(bus_ce_clk, "bus-ce", "dcxo24M", 0x060, BIT(5), 0);
static SUNXI_CCU_GATE(bus_mipidsi_clk, "bus-mipidsi", "dcxo24M", 0x060, BIT(1), 0);

static SUNXI_CCU_GATE(bus_tcontop_clk, "bus-toontop", "dcxo24M", 0x064, BIT(30), 0);
static SUNXI_CCU_GATE(bus_tcontv1_clk, "bus-tcontv1", "dcxo24M", 0x064, BIT(29), 0);
static SUNXI_CCU_GATE(bus_tcontv0_clk, "bus-tcontv0", "dcxo24M", 0x064, BIT(28), 0);
static SUNXI_CCU_GATE(bus_tconlcd1_clk, "bus-tconlcd1", "dcxo24M", 0x064, BIT(27), 0);
static SUNXI_CCU_GATE(bus_tconlcd0_clk, "bus-tconlcd0", "dcxo24M", 0x064, BIT(26), 0);
static SUNXI_CCU_GATE(bus_tvdtop_clk, "bus-tvdtop", "dcxo24M", 0x064, BIT(25), 0);
static SUNXI_CCU_GATE(bus_tvd3_clk, "bus-tvd3", "dcxo24M", 0x064, BIT(24), 0);
static SUNXI_CCU_GATE(bus_tvd2_clk, "bus-tvd2", "dcxo24M", 0x064, BIT(23), 0);
static SUNXI_CCU_GATE(bus_tvd1_clk, "bus-tvd1", "dcxo24M", 0x064, BIT(22), 0);
static SUNXI_CCU_GATE(bus_tvd0_clk, "bus-tvd0", "dcxo24M", 0x064, BIT(21), 0);
static SUNXI_CCU_GATE(bus_gpu_clk, "bus-gpu", "dcxo24M", 0x064, BIT(20), 0);
static SUNXI_CCU_GATE(bus_gmac_clk, "bus-gmac", "dcxo24M", 0x064, BIT(17), 0);
static SUNXI_CCU_GATE(bus_tvetop_clk, "bus-tvetop", "dcxo24M", 0x064, BIT(15), 0);
static SUNXI_CCU_GATE(bus_tve1_clk, "bus-tve1", "dcxo24M", 0x064, BIT(14), 0);
static SUNXI_CCU_GATE(bus_tve0_clk, "bus-tve0", "dcxo24M", 0x064, BIT(13), 0);
static SUNXI_CCU_GATE(bus_de_clk, "bus-de", "dcxo24M", 0x064, BIT(12), 0);
static SUNXI_CCU_GATE(bus_hdmi1_clk, "bus-hdmi1", "dcxo24M", 0x064, BIT(11), 0);
static SUNXI_CCU_GATE(bus_hdmi0_clk, "bus-hdmi0", "dcxo24M", 0x064, BIT(10), 0);
static SUNXI_CCU_GATE(bus_csi1_clk, "bus-csi1", "dcxo24M", 0x064, BIT(9), 0);
static SUNXI_CCU_GATE(bus_csi0_clk, "bus-csi0", "dcxo24M", 0x064, BIT(8), 0);
static SUNXI_CCU_GATE(bus_di_clk, "bus-di", "dcxo24M", 0x064, BIT(5), 0);
static SUNXI_CCU_GATE(bus_mp_clk, "bus-mp", "dcxo24M", 0x064, BIT(2), 0);
static SUNXI_CCU_GATE(bus_ve_clk, "bus-ve", "dcxo24M", 0x064, BIT(0), 0);

static SUNXI_CCU_GATE(bus_daudio2_clk, "bus-daudio2", "dcxo24M", 0x068, BIT(14), 0);
static SUNXI_CCU_GATE(bus_daudio1_clk, "bus-daudio1", "dcxo24M", 0x068, BIT(13), 0);
static SUNXI_CCU_GATE(bus_daudio0_clk, "bus-daudio0", "dcxo24M", 0x068, BIT(12), 0);
static SUNXI_CCU_GATE(bus_keypad_clk, "bus-keypad", "dcxo24M", 0x068, BIT(10), 0);
static SUNXI_CCU_GATE(bus_ths_clk, "bus-ths", "dcxo24M", 0x068, BIT(8), 0);
static SUNXI_CCU_GATE(bus_ir1_clk, "bus-ir1", "dcxo24M", 0x068, BIT(7), 0);
static SUNXI_CCU_GATE(bus_ir0_clk, "bus-ir0", "dcxo24M", 0x068, BIT(6), 0);
static SUNXI_CCU_GATE(bus_pio_clk, "bus-pio", "dcxo24M", 0x068, BIT(5), 0);
static SUNXI_CCU_GATE(bus_ac97_clk, "bus-ac97", "dcxo24M", 0x068, BIT(2), 0);
static SUNXI_CCU_GATE(bus_spdif_clk, "bus-spdif", "dcxo24M", 0x068, BIT(1), 0);
static SUNXI_CCU_GATE(bus_ac_dig_clk, "bus-ac-dig", "dcxo24M", 0x068, BIT(0), 0);

static SUNXI_CCU_GATE(bus_uart7_clk, "bus-uart7", "dcxo24M", 0x06c, BIT(23), 0);
static SUNXI_CCU_GATE(bus_uart6_clk, "bus-uart6", "dcxo24M", 0x06c, BIT(22), 0);
static SUNXI_CCU_GATE(bus_uart5_clk, "bus-uart5", "dcxo24M", 0x06c, BIT(21), 0);
static SUNXI_CCU_GATE(bus_uart4_clk, "bus-uart4", "dcxo24M", 0x06c, BIT(20), 0);
static SUNXI_CCU_GATE(bus_uart3_clk, "bus-uart3", "dcxo24M", 0x06c, BIT(19), 0);
static SUNXI_CCU_GATE(bus_uart2_clk, "bus-uart2", "dcxo24M", 0x06c, BIT(18), 0);
static SUNXI_CCU_GATE(bus_uart1_clk, "bus-uart1", "dcxo24M", 0x06c, BIT(17), 0);
static SUNXI_CCU_GATE(bus_uart0_clk, "bus-uart0", "dcxo24M", 0x06c, BIT(16), 0);
static SUNXI_CCU_GATE(bus_twi4_clk, "bus-twi4", "dcxo24M", 0x06c, BIT(15), 0);
static SUNXI_CCU_GATE(bus_ps2_1_clk, "bus-ps2-1", "dcxo24M", 0x06c, BIT(7), 0);
static SUNXI_CCU_GATE(bus_ps2_0_clk, "bus-ps2-0", "dcxo24M", 0x06c, BIT(6), 0);
static SUNXI_CCU_GATE(bus_scr_clk, "bus-scr", "dcxo24M", 0x06c, BIT(5), 0);
static SUNXI_CCU_GATE(bus_twi3_clk, "bus-twi3", "dcxo24M", 0x06c, BIT(3), 0);
static SUNXI_CCU_GATE(bus_twi2_clk, "bus-twi2", "dcxo24M", 0x06c, BIT(2), 0);
static SUNXI_CCU_GATE(bus_twi1_clk, "bus-twi1", "dcxo24M", 0x06c, BIT(1), 0);
static SUNXI_CCU_GATE(bus_twi0_clk, "bus-twi0", "dcxo24M", 0x06c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_dbgsys_clk, "bus-dbgsys", "dcxo24M", 0x070, BIT(7), 0);

static const char * const sdmm2_mod_parents[] = {"dcxo24M", "pll-periph0-x2", "pll-periph1-2x"};
static SUNXI_CCU_MP_WITH_MUX(sdmm2_mod_clk, "sdmm2-mod",
			     sdmm2_mod_parents,
			     0x090,
			     0, 4,	/* M */
			     16, 2,	/* P */
			     24, 2,	/* mux */
			     CLK_SET_RATE_UNGATE |
					      CLK_IS_CRITICAL);

static const char * const ths_parents[] = {"dcxo24M" };
static struct ccu_div ths_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV_FLAGS(0, 2, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= 0x074,
		.hw.init	= CLK_HW_INIT_PARENTS("ths", ths_parents,
						      &ccu_div_ops,
						      0),
	},
};

static const char * const nand_parents[] = {"dcxo24M", "pll-periph0-1x", "pll-periph1-1x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(nand_clk, "nand", nand_parents, 0x080,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static const char * const mmc_parents[] = {"dcxo24M", "pll-periph0-2x", "pll-periph1-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(mmc0_clk, "mmc0", mmc_parents, 0x088,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(mmc1_clk, "mmc1", mmc_parents, 0x08C,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);


static SUNXI_CCU_MP_WITH_MUX_GATE(mmc2_clk, "mmc2", mmc_parents, 0x090,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(mmc3_clk, "mmc3", mmc_parents, 0x094,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);


static const char * const ts_parents[] = {"dcxo24M", "pll-periph0-1x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(ts_clk, "ts", ts_parents, 0x098,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 1,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static const char * const ce_parents[] = {"dcxo24M", "pll-periph0-2x", "pll-periph1-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(ce_clk, "ce", ce_parents, 0x09C,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static const char * const spi_parents[] = {"dcxo24M", "pll-periph0-1x", "pll-periph1-1x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(spi0_clk, "spi0", spi_parents, 0x0A0,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(spi1_clk, "spi1", spi_parents, 0x0A4,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(spi2_clk, "spi2", spi_parents, 0x0A8,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(spi3_clk, "spi3", spi_parents, 0x0AC,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

/*
 *  To ensure the stability of pll-audio, pll-audio-1x is not configured correctly.
 *  At the same time, the frequency of pll-audio-4x and pll-audio-8x is also inaccurate.
 *  When configuring related modules, priority should be given to using pll-audio.
 *  */
static const char * const daudio_parents[] = {"pll-audio-8x", "pll-audio-4x", "pll-audio-1x", "pll-audio" };
static SUNXI_CCU_MUX_WITH_GATE(daudio0_clk, "daudio0", daudio_parents, 0x0B0,
			       16, 2, BIT(31), 0);

static SUNXI_CCU_MUX_WITH_GATE(daudio1_clk, "daudio1", daudio_parents, 0x0B4,
			       16, 2, BIT(31), 0);

static SUNXI_CCU_MUX_WITH_GATE(daudio2_clk, "daudio2", daudio_parents, 0x0B8,
			       16, 2, BIT(31), 0);

static SUNXI_CCU_MUX_WITH_GATE(ac97_clk, "ac97", daudio_parents, 0x0BC,
			       16, 2, BIT(31), 0);

static SUNXI_CCU_MUX_WITH_GATE(spdif_clk, "spdif", daudio_parents, 0x0C0,
			       16, 2, BIT(31), 0);

static const char * const keypad_parents[] = {"dcxo24M", "dcxo24M", "osc32k", "osc32k" };
static SUNXI_CCU_MP_WITH_MUX_GATE(keypad_clk, "keypad", keypad_parents, 0x0C4,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static const char * const sata_parents[] = {"pll-sata", "pll-sata" };
static SUNXI_CCU_MUX_WITH_GATE(sata_clk, "sata", sata_parents, 0x0C8,
			       24, 1, BIT(31), 0);

static CLK_FIXED_FACTOR(usb_12M_from_48M_clk, "12M-from-48M", "dcxo24M", 2, 1, 0);
static CLK_FIXED_FACTOR(usb_12M_from_24M_clk, "12M-from-24M", "dcxo24M", 2, 1, 0);
static CLK_FIXED_FACTOR(usb_12M_from_32K_clk, "12M-from-32K", "dcxo24M", 2, 1, 0);
static CLK_FIXED_FACTOR(clk_32K_from_24M_clk, "32K-from-24M", "dcxo24M", 750, 1, 0);

static const char * const ohci2_12m_parents[] = {"12M-from-48M", "12M-from-24M", "12M-from-32K", "" };

static SUNXI_CCU_MUX_WITH_GATE(ohci2_12m_clk, "ohci2-12m", ohci2_12m_parents, 0x0CC,
			       24, 2, BIT(18), 0);

static SUNXI_CCU_MUX_WITH_GATE(ohci1_12m_clk, "ohci1-12m", ohci2_12m_parents, 0x0CC,
			       22, 2, BIT(17), 0);

static SUNXI_CCU_MUX_WITH_GATE(ohci0_12m_clk, "ohci0-12m", ohci2_12m_parents, 0x0CC,
			       20, 2, BIT(16), 0);

static SUNXI_CCU_GATE(usbphy2_gate_clk, "usbphy2-gate", "dcxo24M", 0x0CC, BIT(10), 0);
static SUNXI_CCU_GATE(usbphy1_gate_clk, "usbphy1-gate", "dcxo24M", 0x0CC, BIT(9), 0);
static SUNXI_CCU_GATE(usbphy0_gate_clk, "usbphy0-gate", "dcxo24M", 0x0CC, BIT(8), 0);

static const char * const ir_parents[] = {"dcxo24M", "pll-periph0-1x", "pll-periph1-1x", "osc32k" };
static SUNXI_CCU_MP_WITH_MUX_GATE(ir0_clk, "ir0", ir_parents, 0x0D0,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(ir1_clk, "ir1", ir_parents, 0x0D4,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static const char * const dram_parents[] = {"pll-periph0-2x", "pll-ddr1" };
static struct ccu_div dram_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 2),
	.mux	= _SUNXI_CCU_MUX(20, 1),
	.common	= {
		.reg		= 0x0F4,
		.hw.init	= CLK_HW_INIT_PARENTS("dram", dram_parents,
						      &ccu_div_ops,
						      0),
	},
};

static SUNXI_CCU_GATE(di_gate_clk, "di-gate", "dcxo24M", 0x100, BIT(2), 0);
static SUNXI_CCU_GATE(mp_gate_clk, "mp-gate", "dcxo24M", 0x100, BIT(5), 0);
static SUNXI_CCU_GATE(tvd_gate_clk, "tvd-gate", "dcxo24M", 0x100, BIT(4), 0);
static SUNXI_CCU_GATE(ts_gate_clk, "ts-gate", "dcxo24M", 0x100, BIT(3), 0);
static SUNXI_CCU_GATE(csi1_gate_clk, "csi1-gate", "dcxo24M", 0x100, BIT(2), 0);
static SUNXI_CCU_GATE(csi0_gate_clk, "csi0-gate", "dcxo24M", 0x100, BIT(1), 0);
static SUNXI_CCU_GATE(ve_gate_clk, "ve-gate", "dcxo24M", 0x100, BIT(0), 0);

static const char * const de_parents[] = {"pll-periph0-2x", "pll-de" };
static struct ccu_div de_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 1),
	.common	= {
		.reg		= 0x104,
		.hw.init	= CLK_HW_INIT_PARENTS("de", de_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div de_mp_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 1),
	.common	= {
		.reg		= 0x108,
		.hw.init	= CLK_HW_INIT_PARENTS("de-mp", de_parents,
						      &ccu_div_ops,
						      0),
	},
};

static const char * const tcon_lcd_parents[] = {"pll-video0-1x", "pll-video1-1x", "pll-video0-2x", "pll-video1-2x", "pll-mipi" };
static SUNXI_CCU_MUX_WITH_GATE(tcon_lcd0_clk, "tcon-lcd0", tcon_lcd_parents, 0x110,
			       24, 3, BIT(31), 0);
static SUNXI_CCU_MUX_WITH_GATE(tcon_lcd1_clk, "tcon-lcd1", tcon_lcd_parents, 0x114,
			       24, 3, BIT(31), 0);

static SUNXI_CCU_M_WITH_MUX_GATE(tcon_tv0_clk, "tcon-tv0", tcon_lcd_parents, 0x118,
				0, 4,
			       24, 3,
			       BIT(31), 0);
static SUNXI_CCU_M_WITH_MUX_GATE(tcon_tv1_clk, "tcon-tv1", tcon_lcd_parents, 0x11c,
				0, 4,
			       24, 3,
			       BIT(31), 0);

static const char * const deinterlace_parents[] = {"pll-periph0-1x", "pll-periph1-1x" };
static struct ccu_div deinterlace_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x124,
		.hw.init	= CLK_HW_INIT_PARENTS("deinterlace", deinterlace_parents,
						      &ccu_div_ops,
						      0),
	},
};

static const char * const csi_misc_parents[] = {"dcxo24M", "pll-video1-1x", "pll-periph1-1x" };
static struct ccu_div csi_misc_clk = {
	.enable = BIT(15),
	.div	= _SUNXI_CCU_DIV(0, 54),
	.mux	= _SUNXI_CCU_MUX(8, 3),
	.common	= {
		.reg		= 0x130,
		.hw.init	= CLK_HW_INIT_PARENTS("csi-misc", csi_misc_parents,
						      &ccu_div_ops,
						      0),
	},
};

static const char * const csi_sclk_parents[] = {"pll-periph0-1x", "pll-periph1-1x" };
static struct ccu_div csi_sclk_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(16, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x134,
		.hw.init	= CLK_HW_INIT_PARENTS("csi-sclk", csi_sclk_parents,
						      &ccu_div_ops,
						      0),
	},
};

static const char * const csi_mclk0_parents[] = {"dcxo24M", "pll-video1-1x", "pll-periph1-1x" };
static struct ccu_div csi_mclk0_clk = {
	.enable = BIT(15),
	.div	= _SUNXI_CCU_DIV(0, 5),
	.mux	= _SUNXI_CCU_MUX(8, 3),
	.common	= {
		.reg		= 0x134,
		.hw.init	= CLK_HW_INIT_PARENTS("csi-mclk0", csi_mclk0_parents,
						      &ccu_div_ops,
						      0),
	},
};

static SUNXI_CCU_M_WITH_GATE(ve_clk, "ve",
				"pll-ve", 0x13c,
				16, 3, BIT(31), CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(ac_digital_gate_clk, "ac-digital-gate", "pll-audio", 0x140, BIT(31), 0);
static SUNXI_CCU_GATE(avs_gate_clk, "avs-gate", "dcxo24M", 0x144, BIT(31), 0);

static const char * const hdmi_parents[] = {"pll-video0-1x", "pll-video1-1x" };
static struct ccu_div hdmi_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 2),
	.common	= {
		.reg		= 0x150,
		.hw.init	= CLK_HW_INIT_PARENTS("hdmi", hdmi_parents,
						      &ccu_div_ops,
						      0),
	},
};

static SUNXI_CCU_GATE(hdmi_ddc_gate_clk, "hdmi-ddc-gate", "dcxo24M", 0x154, BIT(31), 0);

static const char * const mbus_parents[] = {"dcxo24M", "pll-periph0-2x", "pll-ddr0" };
static SUNXI_CCU_MP_WITH_MUX_GATE(mbus_clk, "mbus", mbus_parents, 0x15C,
					  0, 4,		/* M */
					  16, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_GATE(rmii_gate_clk, "rmii-gate", "dcxo24M", 0x164, BIT(13), 0);
static SUNXI_CCU_GATE(epit_gate_clk, "epit-gate", "dcxo24M", 0x164, BIT(3), 0);

static const char * const mipi_dsi_parents[] = {"pll-video0-1x", "pll-video1-1x", "pll-periph0-1x" };
static struct ccu_div mipi_dsi_clk = {
	.enable = BIT(15),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(8, 2),
	.common	= {
		.reg		= 0x168,
		.hw.init	= CLK_HW_INIT_PARENTS("mipi-dsi", mipi_dsi_parents,
						      &ccu_div_ops,
						      0),
	},
};


static const char * const tve_parents[] = {"pll-video0-1x", "pll-video1-1x", "pll-video0-2x", "pll-video1-2x", "pll-mipi" };

static struct ccu_div tve0_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x180,
		.hw.init	= CLK_HW_INIT_PARENTS("tve0", tve_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div tve1_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x184,
		.hw.init	= CLK_HW_INIT_PARENTS("tve1", tve_parents,
						      &ccu_div_ops,
						      0),
	},
};


static const char * const tvd_parents[] = {"pll-video0-1x", "pll-video1-1x", "pll-video0-2x", "pll-video1-2x" };
static struct ccu_div tvd0_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x188,
		.hw.init	= CLK_HW_INIT_PARENTS("tvd0", tvd_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div tvd1_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x18c,
		.hw.init	= CLK_HW_INIT_PARENTS("tvd1", tvd_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div tvd2_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x190,
		.hw.init	= CLK_HW_INIT_PARENTS("tvd2", tvd_parents,
						      &ccu_div_ops,
						      0),
	},
};

static struct ccu_div tvd3_clk = {
	.enable = BIT(31),
	.div	= _SUNXI_CCU_DIV(0, 4),
	.mux	= _SUNXI_CCU_MUX(24, 3),
	.common	= {
		.reg		= 0x194,
		.hw.init	= CLK_HW_INIT_PARENTS("tvd3", tvd_parents,
						      &ccu_div_ops,
						      0),
	},
};

static SUNXI_CCU_M_WITH_GATE(gpu_clk, "gpu",
				"pll-gpu", 0x1A0,
				0, 3, BIT(31), 0);

static const char * const out_parents[] = {"32K-from-24M", "osc32k", "dcxo24M" };
static SUNXI_CCU_MP_WITH_MUX_GATE(outa_clk, "outa", out_parents, 0x1F0,
					  8, 5,		/* M */
					  20, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE(outb_clk, "outb", out_parents, 0x1F4,
					  8, 5,		/* M */
					  20, 2,	/* P */
					  24, 2,	/* mux */
					  BIT(31),	/* gate */
					  CLK_SET_RATE_NO_REPARENT);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun8iw11_ccu_resets[] = {
	[RST_BUS_USBPHY2]	= { 0x0cc, BIT(2)},
	[RST_BUS_USBPHY1]	= { 0x0cc, BIT(1)},
	[RST_BUS_USBPHY0]	= { 0x0cc, BIT(0)},
	[RST_MBUS]		= { 0x0fc, BIT(31)},
	[RST_BUS_USBOHCI2]	= { 0x2c0, BIT(31)},
	[RST_BUS_USBOHCI1]	= { 0x2c0, BIT(30)},
	[RST_BUS_USBOHCI0]	= { 0x2c0, BIT(29)},
	[RST_BUS_USBEHCI2]	= { 0x2c0, BIT(28)},
	[RST_BUS_USBEHCI1]	= { 0x2c0, BIT(27)},
	[RST_BUS_USBEHCI0]	= { 0x2c0, BIT(26)},
	[RST_BUS_USBOTG]	= { 0x2c0, BIT(25)},
	[RST_BUS_SATA]		= { 0x2c0, BIT(24)},
	[RST_BUS_SPI3]		= { 0x2c0, BIT(23)},
	[RST_BUS_SPI2]		= { 0x2c0, BIT(22)},
	[RST_BUS_SPI1]		= { 0x2c0, BIT(21)},
	[RST_BUS_SPI0]		= { 0x2c0, BIT(20)},
	[RST_BUS_HSTMR]		= { 0x2c0, BIT(19)},
	[RST_BUS_TS]		= { 0x2c0, BIT(18)},
	[RST_BUS_EMAC]		= { 0x2c0, BIT(17)},
	[RST_BUS_SDRAM]		= { 0x2c0, BIT(14)},
	[RST_BUS_NAND]		= { 0x2c0, BIT(13)},
	[RST_BUS_MMC3]		= { 0x2c0, BIT(11)},
	[RST_BUS_MMC2]		= { 0x2c0, BIT(10)},
	[RST_BUS_MMC1]		= { 0x2c0, BIT(9)},
	[RST_BUS_MMC0]		= { 0x2c0, BIT(8)},
	[RST_BUS_DMA]		= { 0x2c0, BIT(6)},
	[RST_BUS_CE]		= { 0x2c0, BIT(5)},
	[RST_BUS_MIPI_DSI]	= { 0x2c0, BIT(1)},
	[RST_BUS_DCU]		= { 0x2c4, BIT(31)},
	[RST_BUS_TCON_TOP]	= { 0x2c4, BIT(30)},
	[RST_BUS_TCON_TV1]	= { 0x2c4, BIT(29)},
	[RST_BUS_TCON_TV0]	= { 0x2c4, BIT(28)},
	[RST_BUS_TCON_LCD1]	= { 0x2c4, BIT(27)},
	[RST_BUS_TCON_LCD0]	= { 0x2c4, BIT(26)},
	[RST_BUS_TVD_TOP]	= { 0x2c4, BIT(25)},
	[RST_BUS_TVD3]		= { 0x2c4, BIT(24)},
	[RST_BUS_TVD2]		= { 0x2c4, BIT(23)},
	[RST_BUS_TVD1]		= { 0x2c4, BIT(22)},
	[RST_BUS_TVD0]		= { 0x2c4, BIT(21)},
	[RST_BUS_GPU]		= { 0x2c4, BIT(20)},
	[RST_BUS_GMAC]		= { 0x2c4, BIT(17)},
	[RST_BUS_TVE_TOP]	= { 0x2c4, BIT(15)},
	[RST_BUS_TVE1]		= { 0x2c4, BIT(14)},
	[RST_BUS_TVE0]		= { 0x2c4, BIT(13)},
	[RST_BUS_DE]		= { 0x2c4, BIT(12)},
	[RST_BUS_HDMI1]		= { 0x2c4, BIT(11)},
	[RST_BUS_HDMI0]		= { 0x2c4, BIT(10)},
	[RST_BUS_CSI1]		= { 0x2c4, BIT(9)},
	[RST_BUS_CSI0]		= { 0x2c4, BIT(8)},
	[RST_BUS_DI]		= { 0x2c4, BIT(5)},
	[RST_BUS_MP]		= { 0x2c4, BIT(2)},
	[RST_BUS_VE]		= { 0x2c4, BIT(0)},
	[RST_BUS_LVDS]		= { 0x2c8, BIT(0)},
	[RST_BUS_DAUDIO2]	= { 0x2d0, BIT(14)},
	[RST_BUS_DAUDIO1]	= { 0x2d0, BIT(13)},
	[RST_BUS_DAUDIO0]	= { 0x2d0, BIT(12)},
	[RST_BUS_KEY]		= { 0x2d0, BIT(10)},
	[RST_BUS_THS]		= { 0x2d0, BIT(8)},
	[RST_BUS_IR1]		= { 0x2d0, BIT(7)},
	[RST_BUS_IR]		= { 0x2d0, BIT(6)},
	[RST_BUS_AC97]		= { 0x2d0, BIT(2)},
	[RST_BUS_SPDIF]		= { 0x2d0, BIT(1)},
	[RST_BUS_AC]		= { 0x2d0, BIT(0)},
	[RST_BUS_UART7]		= { 0x2d8, BIT(23)},
	[RST_BUS_UART6]		= { 0x2d8, BIT(22)},
	[RST_BUS_UART5]		= { 0x2d8, BIT(21)},
	[RST_BUS_UART4]		= { 0x2d8, BIT(20)},
	[RST_BUS_UART3]		= { 0x2d8, BIT(19)},
	[RST_BUS_UART2]		= { 0x2d8, BIT(18)},
	[RST_BUS_UART1]		= { 0x2d8, BIT(17)},
	[RST_BUS_UART0]		= { 0x2d8, BIT(16)},
	[RST_BUS_TWI4]		= { 0x2d8, BIT(15)},
	[RST_BUS_PS21]		= { 0x2d8, BIT(7)},
	[RST_BUS_PS20]		= { 0x2d8, BIT(6)},
	[RST_BUS_SCR]		= { 0x2d8, BIT(5)},
	[RST_BUS_TWI3]		= { 0x2d8, BIT(3)},
	[RST_BUS_TWI2]		= { 0x2d8, BIT(2)},
	[RST_BUS_TWI1]		= { 0x2d8, BIT(1)},
	[RST_BUS_TWI0]		= { 0x2d8, BIT(0)},
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun8iw11_hw_clks = {
	.hws    = {
		[CLK_PLL_CPUX]			= &pll_cpux_clk.common.hw,
		[CLK_PLL_AUDIO_1X]		= &pll_audio_1x_clk.hw,
		[CLK_PLL_AUDIO_4X]		= &pll_audio_4x_clk.hw,
		[CLK_PLL_AUDIO_8X]		= &pll_audio_8x_clk.hw,
		[CLK_PLL_AUDIO]			= &pll_audio_clk.common.hw,
		[CLK_PLL_VIDEO0_1X]		= &pll_video0_1x_clk.common.hw,
		[CLK_PLL_VIDEO0_2X]		= &pll_video0_2x_clk.hw,
		[CLK_PLL_VE]			= &pll_ve_clk.common.hw,
		[CLK_PLL_DDR0]			= &pll_ddr0_clk.common.hw,
		[CLK_PLL_PERIPH0_2X]		= &pll_periph0_2x_clk.common.hw,
		[CLK_PLL_PERIPH0_1X]		= &pll_periph0_1x_clk.hw,
		[CLK_PLL_PERIPH1_2X]		= &pll_periph1_2x_clk.common.hw,
		[CLK_PLL_PERIPH1_1X]		= &pll_periph1_1x_clk.hw,
		[CLK_PLL_VIDEO1_1X]		= &pll_video1_1x_clk.common.hw,
		[CLK_PLL_VIDEO1_2X]		= &pll_video1_2x_clk.hw,
		[CLK_PLL_SATA]			= &pll_sata_clk.common.hw,
		[CLK_PLL_GPU]			= &pll_gpu_clk.common.hw,
		[CLK_PLL_MIPI]			= &pll_mipi_clk.common.hw,
		[CLK_PLL_DE]			= &pll_de_clk.common.hw,
		[CLK_PLL_DDR1]			= &pll_ddr1_clk.common.hw,
		[CLK_CPUX]			= &cpux_clk.common.hw,
		[CLK_AXI]			= &axi_clk.common.hw,
		[CLK_PLL_PERIPHAHB0]		= &pll_periphahb0_clk.common.hw,
		[CLK_AHB1]			= &ahb1_clk.common.hw,
		[CLK_APB1]			= &apb1_clk.common.hw,
		[CLK_APB2]			= &apb2_clk.common.hw,
		[CLK_USB_OHCI2]			= &usb_ohci2_clk.common.hw,
		[CLK_USB_OHCI1]			= &usb_ohci1_clk.common.hw,
		[CLK_USB_OHCI0]			= &usb_ohci0_clk.common.hw,
		[CLK_BUS_EHCI2]			= &bus_ehci2_clk.common.hw,
		[CLK_BUS_EHCI1]			= &bus_ehci1_clk.common.hw,
		[CLK_BUS_EHCI0]			= &bus_ehci0_clk.common.hw,
		[CLK_BUS_OTG]			= &bus_otg_clk.common.hw,
		[CLK_BUS_SATA]			= &bus_sata_clk.common.hw,
		[CLK_BUS_SPI3]			= &bus_spi3_clk.common.hw,
		[CLK_BUS_SPI2]			= &bus_spi2_clk.common.hw,
		[CLK_BUS_SPI1]			= &bus_spi1_clk.common.hw,
		[CLK_BUS_SPI0]			= &bus_spi0_clk.common.hw,
		[CLK_BUS_HSTMR]			= &bus_hstmr_clk.common.hw,
		[CLK_BUS_TS]			= &bus_ts_clk.common.hw,
		[CLK_BUS_EMAC]			= &bus_emac_clk.common.hw,
		[CLK_BUS_DRAM]			= &bus_dram_clk.common.hw,
		[CLK_BUS_NAND]			= &bus_nand_clk.common.hw,
		[CLK_BUS_MMC3]			= &bus_mmc3_clk.common.hw,
		[CLK_BUS_MMC2]			= &bus_mmc2_clk.common.hw,
		[CLK_BUS_MMC1]			= &bus_mmc1_clk.common.hw,
		[CLK_BUS_MMC0]			= &bus_mmc0_clk.common.hw,
		[CLK_BUS_DMA]			= &bus_dma_clk.common.hw,
		[CLK_BUS_CE]			= &bus_ce_clk.common.hw,
		[CLK_BUS_MIPIDSI]		= &bus_mipidsi_clk.common.hw,
		[CLK_BUS_TCONTOP]		= &bus_tcontop_clk.common.hw,
		[CLK_BUS_TCONTV1]		= &bus_tcontv1_clk.common.hw,
		[CLK_BUS_TCONTV0]		= &bus_tcontv0_clk.common.hw,
		[CLK_BUS_TCONLCD1]		= &bus_tconlcd1_clk.common.hw,
		[CLK_BUS_TCONLCD0]		= &bus_tconlcd0_clk.common.hw,
		[CLK_BUS_TVDTOP]		= &bus_tvdtop_clk.common.hw,
		[CLK_BUS_TVD3]			= &bus_tvd3_clk.common.hw,
		[CLK_BUS_TVD2]			= &bus_tvd2_clk.common.hw,
		[CLK_BUS_TVD1]			= &bus_tvd1_clk.common.hw,
		[CLK_BUS_TVD0]			= &bus_tvd0_clk.common.hw,
		[CLK_BUS_GPU]			= &bus_gpu_clk.common.hw,
		[CLK_BUS_GMAC]			= &bus_gmac_clk.common.hw,
		[CLK_BUS_TVETOP]		= &bus_tvetop_clk.common.hw,
		[CLK_BUS_TVE1]			= &bus_tve1_clk.common.hw,
		[CLK_BUS_TVE0]			= &bus_tve0_clk.common.hw,
		[CLK_BUS_DE]			= &bus_de_clk.common.hw,
		[CLK_BUS_HDMI1]			= &bus_hdmi1_clk.common.hw,
		[CLK_BUS_HDMI0]			= &bus_hdmi0_clk.common.hw,
		[CLK_BUS_CSI1]			= &bus_csi1_clk.common.hw,
		[CLK_BUS_CSI0]			= &bus_csi0_clk.common.hw,
		[CLK_BUS_DI]			= &bus_di_clk.common.hw,
		[CLK_BUS_MP]			= &bus_mp_clk.common.hw,
		[CLK_BUS_VE]			= &bus_ve_clk.common.hw,
		[CLK_BUS_DAUDIO2]		= &bus_daudio2_clk.common.hw,
		[CLK_BUS_DAUDIO1]		= &bus_daudio1_clk.common.hw,
		[CLK_BUS_DAUDIO0]		= &bus_daudio0_clk.common.hw,
		[CLK_BUS_KEYPAD]		= &bus_keypad_clk.common.hw,
		[CLK_BUS_THS]			= &bus_ths_clk.common.hw,
		[CLK_BUS_IR1]			= &bus_ir1_clk.common.hw,
		[CLK_BUS_IR0]			= &bus_ir0_clk.common.hw,
		[CLK_BUS_PIO]			= &bus_pio_clk.common.hw,
		[CLK_BUS_AC97]			= &bus_ac97_clk.common.hw,
		[CLK_BUS_SPDIF]			= &bus_spdif_clk.common.hw,
		[CLK_BUS_AC_DIG]		= &bus_ac_dig_clk.common.hw,
		[CLK_BUS_UART7]			= &bus_uart7_clk.common.hw,
		[CLK_BUS_UART6]			= &bus_uart6_clk.common.hw,
		[CLK_BUS_UART5]			= &bus_uart5_clk.common.hw,
		[CLK_BUS_UART4]			= &bus_uart4_clk.common.hw,
		[CLK_BUS_UART3]			= &bus_uart3_clk.common.hw,
		[CLK_BUS_UART2]			= &bus_uart2_clk.common.hw,
		[CLK_BUS_UART1]			= &bus_uart1_clk.common.hw,
		[CLK_BUS_UART0]			= &bus_uart0_clk.common.hw,
		[CLK_BUS_TWI4]			= &bus_twi4_clk.common.hw,
		[CLK_BUS_PS2_1]			= &bus_ps2_1_clk.common.hw,
		[CLK_BUS_PS2_0]			= &bus_ps2_0_clk.common.hw,
		[CLK_BUS_SCR]			= &bus_scr_clk.common.hw,
		[CLK_BUS_TWI3]			= &bus_twi3_clk.common.hw,
		[CLK_BUS_TWI2]			= &bus_twi2_clk.common.hw,
		[CLK_BUS_TWI1]			= &bus_twi1_clk.common.hw,
		[CLK_BUS_TWI0]			= &bus_twi0_clk.common.hw,
		[CLK_BUS_DBGSYS]		= &bus_dbgsys_clk.common.hw,
		[CLK_SDMM2_MOD]			= &sdmm2_mod_clk.common.hw,
		[CLK_THS]			= &ths_clk.common.hw,
		[CLK_NAND]			= &nand_clk.common.hw,
		[CLK_MMC0]			= &mmc0_clk.common.hw,
		[CLK_MMC1]			= &mmc1_clk.common.hw,
		[CLK_MMC2]			= &mmc2_clk.common.hw,
		[CLK_MMC3]			= &mmc3_clk.common.hw,
		[CLK_TS]			= &ts_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_SPI0]			= &spi0_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_SPI3]			= &spi3_clk.common.hw,
		[CLK_DAUDIO0]			= &daudio0_clk.common.hw,
		[CLK_DAUDIO1]			= &daudio1_clk.common.hw,
		[CLK_DAUDIO2]			= &daudio2_clk.common.hw,
		[CLK_AC97]			= &ac97_clk.common.hw,
		[CLK_SPDIF]			= &spdif_clk.common.hw,
		[CLK_KEYPAD]			= &keypad_clk.common.hw,
		[CLK_SATA]			= &sata_clk.common.hw,
		[CLK_12M_FROM_48M]		= &usb_12M_from_48M_clk.hw,
		[CLK_12M_FROM_24M]		= &usb_12M_from_24M_clk.hw,
		[CLK_12M_FROM_32K]		= &usb_12M_from_32K_clk.hw,
		[CLK_32K_FROM_24M]		= &clk_32K_from_24M_clk.hw,
		[CLK_OHCI2_12M]			= &ohci2_12m_clk.common.hw,
		[CLK_OHCI1_12M]			= &ohci1_12m_clk.common.hw,
		[CLK_OHCI0_12M]			= &ohci0_12m_clk.common.hw,
		[CLK_USBPHY2_GATE]		= &usbphy2_gate_clk.common.hw,
		[CLK_USBPHY1_GATE]		= &usbphy1_gate_clk.common.hw,
		[CLK_USBPHY0_GATE]		= &usbphy0_gate_clk.common.hw,
		[CLK_IR0]			= &ir0_clk.common.hw,
		[CLK_IR1]			= &ir1_clk.common.hw,
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_DI_GATE]			= &di_gate_clk.common.hw,
		[CLK_MP_GATE]			= &mp_gate_clk.common.hw,
		[CLK_TVD_GATE]			= &tvd_gate_clk.common.hw,
		[CLK_TS_GATE]			= &ts_gate_clk.common.hw,
		[CLK_CSI1_GATE]			= &csi1_gate_clk.common.hw,
		[CLK_CSI0_GATE]			= &csi0_gate_clk.common.hw,
		[CLK_VE_GATE]			= &ve_gate_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_DE_MP]			= &de_mp_clk.common.hw,
		[CLK_TCON_LCD0]			= &tcon_lcd0_clk.common.hw,
		[CLK_TCON_LCD1]			= &tcon_lcd1_clk.common.hw,
		[CLK_TCON_TV0]			= &tcon_tv0_clk.common.hw,
		[CLK_TCON_TV1]			= &tcon_tv1_clk.common.hw,
		[CLK_DEINTERLACE]		= &deinterlace_clk.common.hw,
		[CLK_CSI_MISC]			= &csi_misc_clk.common.hw,
		[CLK_CSI_SCLK]			= &csi_sclk_clk.common.hw,
		[CLK_CSI_MCLK0]			= &csi_mclk0_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_AC_DIGITAL_GATE]		= &ac_digital_gate_clk.common.hw,
		[CLK_AVS_GATE]			= &avs_gate_clk.common.hw,
		[CLK_HDMI]			= &hdmi_clk.common.hw,
		[CLK_HDMI_DDC_GATE]		= &hdmi_ddc_gate_clk.common.hw,
		[CLK_MBUS]			= &mbus_clk.common.hw,
		[CLK_RMII_GATE]			= &rmii_gate_clk.common.hw,
		[CLK_EPIT_GATE]			= &epit_gate_clk.common.hw,
		[CLK_MIPI_DSI]			= &mipi_dsi_clk.common.hw,
		[CLK_TVE0]			= &tve0_clk.common.hw,
		[CLK_TVE1]			= &tve1_clk.common.hw,
		[CLK_TVD0]			= &tvd0_clk.common.hw,
		[CLK_TVD1]			= &tvd1_clk.common.hw,
		[CLK_TVD2]			= &tvd2_clk.common.hw,
		[CLK_TVD3]			= &tvd3_clk.common.hw,
		[CLK_GPU]			= &gpu_clk.common.hw,
		[CLK_OUTA]			= &outa_clk.common.hw,
		[CLK_OUTB]			= &outb_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun8iw11_ccu_clks[] = {
	&pll_cpux_clk.common,
	&pll_audio_clk.common,
	&pll_video0_1x_clk.common,
	&pll_ve_clk.common,
	&pll_ddr0_clk.common,
	&pll_periph0_2x_clk.common,
	&pll_periph1_2x_clk.common,
	&pll_video1_1x_clk.common,
	&pll_sata_clk.common,
	&pll_gpu_clk.common,
	&pll_mipi_clk.common,
	&pll_de_clk.common,
	&pll_ddr1_clk.common,
	&cpux_clk.common,
	&axi_clk.common,
	&pll_periphahb0_clk.common,
	&ahb1_clk.common,
	&apb1_clk.common,
	&apb2_clk.common,
	&usb_ohci2_clk.common,
	&usb_ohci1_clk.common,
	&usb_ohci0_clk.common,
	&bus_ehci2_clk.common,
	&bus_ehci1_clk.common,
	&bus_ehci0_clk.common,
	&bus_otg_clk.common,
	&bus_sata_clk.common,
	&bus_spi3_clk.common,
	&bus_spi2_clk.common,
	&bus_spi1_clk.common,
	&bus_spi0_clk.common,
	&bus_hstmr_clk.common,
	&bus_ts_clk.common,
	&bus_emac_clk.common,
	&bus_dram_clk.common,
	&bus_nand_clk.common,
	&bus_mmc3_clk.common,
	&bus_mmc2_clk.common,
	&bus_mmc1_clk.common,
	&bus_mmc0_clk.common,
	&bus_dma_clk.common,
	&bus_ce_clk.common,
	&bus_mipidsi_clk.common,
	&bus_tcontop_clk.common,
	&bus_tcontv1_clk.common,
	&bus_tcontv0_clk.common,
	&bus_tconlcd1_clk.common,
	&bus_tconlcd0_clk.common,
	&bus_tvdtop_clk.common,
	&bus_tvd3_clk.common,
	&bus_tvd2_clk.common,
	&bus_tvd1_clk.common,
	&bus_tvd0_clk.common,
	&bus_gpu_clk.common,
	&bus_gmac_clk.common,
	&bus_tvetop_clk.common,
	&bus_tve1_clk.common,
	&bus_tve0_clk.common,
	&bus_de_clk.common,
	&bus_hdmi1_clk.common,
	&bus_hdmi0_clk.common,
	&bus_csi1_clk.common,
	&bus_csi0_clk.common,
	&bus_di_clk.common,
	&bus_mp_clk.common,
	&bus_ve_clk.common,
	&bus_daudio2_clk.common,
	&bus_daudio1_clk.common,
	&bus_daudio0_clk.common,
	&bus_keypad_clk.common,
	&bus_ths_clk.common,
	&bus_ir1_clk.common,
	&bus_ir0_clk.common,
	&bus_pio_clk.common,
	&bus_ac97_clk.common,
	&bus_spdif_clk.common,
	&bus_ac_dig_clk.common,
	&bus_uart7_clk.common,
	&bus_uart6_clk.common,
	&bus_uart5_clk.common,
	&bus_uart4_clk.common,
	&bus_uart3_clk.common,
	&bus_uart2_clk.common,
	&bus_uart1_clk.common,
	&bus_uart0_clk.common,
	&bus_twi4_clk.common,
	&bus_ps2_1_clk.common,
	&bus_ps2_0_clk.common,
	&bus_scr_clk.common,
	&bus_twi3_clk.common,
	&bus_twi2_clk.common,
	&bus_twi1_clk.common,
	&bus_twi0_clk.common,
	&bus_dbgsys_clk.common,
	&sdmm2_mod_clk.common,
	&ths_clk.common,
	&nand_clk.common,
	&mmc0_clk.common,
	&mmc1_clk.common,
	&mmc2_clk.common,
	&mmc3_clk.common,
	&ts_clk.common,
	&ce_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&spi2_clk.common,
	&spi3_clk.common,
	&daudio0_clk.common,
	&daudio1_clk.common,
	&daudio2_clk.common,
	&ac97_clk.common,
	&spdif_clk.common,
	&keypad_clk.common,
	&sata_clk.common,
	&ohci2_12m_clk.common,
	&ohci1_12m_clk.common,
	&ohci0_12m_clk.common,
	&usbphy2_gate_clk.common,
	&usbphy1_gate_clk.common,
	&usbphy0_gate_clk.common,
	&ir0_clk.common,
	&ir1_clk.common,
	&dram_clk.common,
	&di_gate_clk.common,
	&mp_gate_clk.common,
	&tvd_gate_clk.common,
	&ts_gate_clk.common,
	&csi1_gate_clk.common,
	&csi0_gate_clk.common,
	&ve_gate_clk.common,
	&de_clk.common,
	&de_mp_clk.common,
	&tcon_lcd0_clk.common,
	&tcon_lcd1_clk.common,
	&tcon_tv0_clk.common,
	&tcon_tv1_clk.common,
	&deinterlace_clk.common,
	&csi_misc_clk.common,
	&csi_sclk_clk.common,
	&csi_mclk0_clk.common,
	&ve_clk.common,
	&ac_digital_gate_clk.common,
	&avs_gate_clk.common,
	&hdmi_clk.common,
	&hdmi_ddc_gate_clk.common,
	&mbus_clk.common,
	&rmii_gate_clk.common,
	&epit_gate_clk.common,
	&mipi_dsi_clk.common,
	&tve0_clk.common,
	&tve1_clk.common,
	&tvd0_clk.common,
	&tvd1_clk.common,
	&tvd2_clk.common,
	&tvd3_clk.common,
	&gpu_clk.common,
	&outa_clk.common,
	&outb_clk.common,
};


static const struct sunxi_ccu_desc sun8iw11_ccu_desc = {
	.ccu_clks	= sun8iw11_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun8iw11_ccu_clks),

	.hw_clks	= &sun8iw11_hw_clks,

	.resets		= sun8iw11_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun8iw11_ccu_resets),
};

static const u32 pll_video_regs[] = {
	SUN8IW11_PLL_VIDEO_REG,
};

static int sun8iw11_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	u32 val;
	int i, ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	/* Enable the lock bits on all PLLs */
	val = readl(reg + SUN8IW11_PLL_LOCK_CTRL_REG);
	for (i = 0; i < SUN8IW11_PLL_CLK_NUM; i++)
		val |= BIT(i);
	writel(val, reg + SUN8IW11_PLL_LOCK_CTRL_REG);


	/* set the parent of sys-32k to losc */
	set_reg_key(reg + SUN8IW11_PLL_SYS_32K_CTL_REG,
		    KEY_FIELD_MAGIC, 16, 16,
		    0x1, 1, 8);

	/* set the div of sys-32k */
	set_reg_key(reg + SUN8IW11_PLL_SYS_32K_CTL_REG,
		    KEY_FIELD_MAGIC, 16, 16,
		    0xf, 4, 0);

	/*
	 * Force the output divider of video PLLs to 0.
	 *
	 * See the comment before pll-video0 definition for the reason.
	 */
	for (i = 0; i < ARRAY_SIZE(pll_video_regs); i++) {
		val = readl(reg + pll_video_regs[i]);
		val &= ~BIT(0);
		writel(val, reg + pll_video_regs[i]);
	}

	/* Enforce m = 0 for Audio0 PLL */
	set_reg(reg + SUN8IW11_PLL_AUDIO_REG, 0x0, 5, 0);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun8iw11_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun8iw11_ccu_clks,
			     ARRAY_SIZE(sun8iw11_ccu_clks),
			     NULL, 0);

	return 0;
}

static const struct of_device_id sun8iw11_ccu_ids[] = {
	{ .compatible = "allwinner,sun8iw11-ccu" },
	{ .compatible = "allwinner,sun20iw1-ccu" },
	{ }
};

static struct platform_driver sun8iw11_ccu_driver = {
	.probe	= sun8iw11_ccu_probe,
	.driver	= {
		.name	= "sun8iw11-ccu",
		.of_match_table	= sun8iw11_ccu_ids,
	},
};

static int __init sunxi_ccu_sun8iw11_init(void)
{
	int ret;

	ret = platform_driver_register(&sun8iw11_ccu_driver);
	if (ret)
		pr_err("register ccu sun8iw11 failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun8iw11_init);

static void __exit sunxi_ccu_sun8iw11_exit(void)
{
	return platform_driver_unregister(&sun8iw11_ccu_driver);
}
module_exit(sunxi_ccu_sun8iw11_exit);

MODULE_VERSION("1.1.5");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
