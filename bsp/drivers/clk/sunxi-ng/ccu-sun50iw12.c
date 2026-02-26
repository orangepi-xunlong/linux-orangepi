// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 frank@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
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

#include "ccu-sun50iw12.h"

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
#define SUN50IW12_PLL_CPUX_REG		0x000
static struct ccu_mult pll_cpux_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.mult		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.common		= {
		.reg		= 0x000,
		.hw.init	= CLK_HW_INIT("pll-cpux", "dcxo24M",
					      &ccu_mult_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

/* Some PLLs are input * N / div1 / P. Model them as NKMP with no K */
#define SUN50IW12_PLL_DDR_REG		0x010
static struct ccu_nkmp pll_ddr_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common		= {
		.reg		= 0x010,
		.hw.init	= CLK_HW_INIT("pll-ddr", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_PERIPH0_REG	0x020
static struct ccu_nkmp pll_periph0_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.fixed_post_div	= 2,
	.common		= {
		.reg		= 0x020,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-periph0", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_PERIPH1_REG	0x028
static struct ccu_nkmp pll_periph1_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.fixed_post_div	= 2,
	.common		= {
		.reg		= 0x028,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-periph1", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_GPU_REG		0x030
static struct ccu_nkmp pll_gpu_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common		= {
		.reg		= 0x030,
		.hw.init	= CLK_HW_INIT("pll-gpu", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

/*
 * For Video PLLs, the output divider is described as "used for testing"
 * in the user manual. So it's not modelled and forced to 0.
 */
#define SUN50IW12_PLL_VIDEO0_REG	0x040
static struct ccu_nm pll_video0_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.fixed_post_div	= 4,
	.min_rate	= 288000000,
	.max_rate	= 2400000000UL,
	.common		= {
		.reg		= 0x040,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-video0", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_VIDEO1_REG	0x048
static struct ccu_nm pll_video1_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.fixed_post_div	= 4,
	.min_rate	= 288000000,
	.max_rate	= 2400000000UL,
	.common		= {
		.reg		= 0x048,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-video1", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_VIDEO2_REG	0x050
static struct ccu_nm pll_video2_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.fixed_post_div	= 4,
	.min_rate	= 288000000,
	.max_rate	= 2400000000UL,
	.common		= {
		.reg		= 0x050,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-video2", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_VE_REG		0x058
static struct ccu_nkmp pll_ve_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common		= {
		.reg		= 0x058,
		.hw.init	= CLK_HW_INIT("pll-ve", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_ADC_REG		0x060
static struct ccu_nkmp pll_adc_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.p		= _SUNXI_CCU_DIV(0, 1), /* output divider */
	.common		= {
		.reg		= 0x060,
		.hw.init	= CLK_HW_INIT("pll-adc", "dcxo24M",
					      &ccu_nkmp_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

#define SUN50IW12_PLL_VIDEO3_REG	0x068
static struct ccu_nm pll_video3_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.fixed_post_div	= 4,
	.min_rate	= 288000000,
	.max_rate	= 2400000000UL,
	.common		= {
		.reg		= 0x068,
		.features	= CCU_FEATURE_FIXED_POSTDIV,
		.hw.init	= CLK_HW_INIT("pll-video3", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

/*
 * We don't have any need for the variable divider for now, so we just
 * hardcode it to match with the clock names.
 */
#define SUN50IW12_PLL_AUDIO_REG		0x078
static struct ccu_nm pll_audio_clk = {
	.enable		= BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* input divider */
	.common		= {
		.reg		= 0x078,
		.hw.init	= CLK_HW_INIT("pll-audio", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

/*
 * When pll_cpux is selected as the clock source for cpux, there is
 * an additional frequency division factor m, which we will ignore here.
 */
static const char * const cpux_parents[] = { "dcxo24M", "osc32k",
					     "iosc", "pll-cpux",
					     "pll-periph0",
					     "pll-periph0-2x" };
static SUNXI_CCU_MUX(cpux_clk, "cpux", cpux_parents,
		     0x500, 24, 3, CLK_SET_RATE_PARENT | CLK_IS_CRITICAL);
static SUNXI_CCU_M(axi_clk, "axi", "cpux", 0x500, 0, 2, 0);
static SUNXI_CCU_M(cpux_apb_clk, "cpux-apb", "cpux", 0x500, 8, 2, 0);

static const char * const ahb_apb0_apb1_parents[] = { "dcxo24M", "osc32k",
						      "iosc", "pll-periph0",
						      "pll-periph0-2x" };
static SUNXI_CCU_MP_WITH_MUX(ahb_clk, "ahb",
			     ahb_apb0_apb1_parents,
			     0x510,
			     0, 2,	/* M */
			     8, 2,	/* P */
			     24, 3,	/* mux */
			     0);

static SUNXI_CCU_MP_WITH_MUX(apb0_clk, "apb0", ahb_apb0_apb1_parents, 0x520,
			     0, 2,	/* M */
			     8, 2,	/* P */
			     24, 3,	/* mux */
			     0);

static SUNXI_CCU_MP_WITH_MUX(apb1_clk, "apb1", ahb_apb0_apb1_parents, 0x524,
			     0, 2,	/* M */
			     8, 2,	/* P */
			     24, 3,	/* mux */
			     0);

static const char * const mbus_parents[] = { "dcxo24M", "pll-ddr",
					     "pll-periph0", "pll-periph0-2x" };
static SUNXI_CCU_M_WITH_MUX_GATE(mbus_clk, "mbus", mbus_parents, 0x540,
				 0, 5,		/* M */
				 24, 2,		/* mux */
				 BIT(31),	/* gate */
				 CLK_IS_CRITICAL);

static const char * const mips_parents[] = { "pll-periph0-2x", "pll-video0-4x",
					     "dcxo24M" };
static SUNXI_CCU_M_WITH_MUX_GATE(mips_clk, "mips", mips_parents, 0x600,
				 0, 3,		/* M */
				 24, 2,		/* mux */
				 BIT(31),	/* gate */
				 CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_mips_clk, "bus-mips", "ahb",
		      0x60c, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(gpu_clk, "gpu", "pll-gpu",
			     0x670, 0, 5, BIT(31), 0);

static SUNXI_CCU_GATE(bus_gpu_clk, "bus-gpu", "ahb",
		      0x67c, BIT(0), 0);

static const char * const ce_parents[] = { "dcxo24M", "pll-periph0-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(ce_clk, "ce", ce_parents, 0x680,
				  0, 4,		/* M */
				  8, 2,		/* N */
				  24, 1,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_GATE(bus_ce_clk, "bus-ce", "ahb",
		      0x68c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_ce_sys_clk, "bus-ce-sys", "ahb",
		      0x68c, BIT(1), 0);

static const char * const ve_core_parents[] = { "pll-ve",
						"pll-periph0-2x" };
static SUNXI_CCU_M_WITH_MUX_GATE(ve_core_clk, "ve-core", ve_core_parents,
				 0x690,
				 0, 3,		/* M */
				 24, 1,		/* mux */
				 BIT(31),	/* gate */
				 0);

static SUNXI_CCU_GATE(bus_ve_clk, "bus-ve", "ahb",
		      0x69c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_av1_clk, "bus-av1", "ahb",
		      0x69c, BIT(1), 0);
static SUNXI_CCU_GATE(bus_ve3_clk, "bus-ve3", "ahb",
		      0x69c, BIT(2), 0);

static SUNXI_CCU_GATE(bus_dma_clk, "bus-dma", "ahb",
		      0x70c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_msgbox_clk, "bus-msgbox", "ahb",
		      0x71c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_spinlock_clk, "bus-spinlock", "ahb",
		      0x72c, BIT(0), 0);

static const struct ccu_mux_var_prediv ahb_predivs[] = {
	{ .index = 3, .shift = 0, .width = 5 },
};

static const char * const timer_parents[] = {"dcxo24M", "iosc",
					     "osc32k", "ahb"};

static struct ccu_div timer0_clk = {
	.enable = BIT(0),
	.div	= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= {
		.shift = 4,
		.width = 2,

		.var_predivs	= ahb_predivs,
		.n_var_predivs	= ARRAY_SIZE(ahb_predivs),
	},
	.common = {
		.reg = 0x730,
		.features = CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init  = CLK_HW_INIT_PARENTS("timer0",
				timer_parents,
				&ccu_div_ops,
				0),
	},
};

static struct ccu_div timer1_clk = {
	.enable = BIT(0),
	.div	= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= {
		.shift = 4,
		.width = 2,

		.var_predivs	= ahb_predivs,
		.n_var_predivs	= ARRAY_SIZE(ahb_predivs),
	},
	.common = {
		.reg = 0x734,
		.features = CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init  = CLK_HW_INIT_PARENTS("timer1",
				timer_parents,
				&ccu_div_ops,
				0),
	},
};

static struct ccu_div timer2_clk = {
	.enable = BIT(0),
	.div	= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= {
		.shift = 4,
		.width = 2,

		.var_predivs	= ahb_predivs,
		.n_var_predivs	= ARRAY_SIZE(ahb_predivs),
	},
	.common = {
		.reg = 0x738,
		.features = CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init  = CLK_HW_INIT_PARENTS("timer2",
				timer_parents,
				&ccu_div_ops,
				0),
	},
};

static struct ccu_div timer3_clk = {
	.enable = BIT(0),
	.div	= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= {
		.shift = 4,
		.width = 2,

		.var_predivs	= ahb_predivs,
		.n_var_predivs	= ARRAY_SIZE(ahb_predivs),
	},
	.common = {
		.reg = 0x73c,
		.features = CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init  = CLK_HW_INIT_PARENTS("timer3",
				timer_parents,
				&ccu_div_ops,
				0),
	},
};

static struct ccu_div timer4_clk = {
	.enable = BIT(0),
	.div	= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= {
		.shift = 4,
		.width = 2,

		.var_predivs	= ahb_predivs,
		.n_var_predivs	= ARRAY_SIZE(ahb_predivs),
	},
	.common = {
		.reg = 0x740,
		.features = CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init  = CLK_HW_INIT_PARENTS("timer4",
				timer_parents,
				&ccu_div_ops,
				0),
	},
};

static struct ccu_div timer5_clk = {
	.enable = BIT(0),
	.div	= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux	= {
		.shift = 4,
		.width = 2,

		.var_predivs	= ahb_predivs,
		.n_var_predivs	= ARRAY_SIZE(ahb_predivs),
	},
	.common = {
		.reg = 0x744,
		.features = CCU_FEATURE_VARIABLE_PREDIV,
		.hw.init  = CLK_HW_INIT_PARENTS("timer5",
				timer_parents,
				&ccu_div_ops,
				0),
	},
};

static SUNXI_CCU_GATE(bus_timer0_clk, "bus-timer0", "ahb",
		      0x750, BIT(0), 0);

static SUNXI_CCU_GATE(bus_dbg_clk, "bus-dbg", "ahb",
		      0x78c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_pwm_clk, "bus-pwm", "apb0",
		      0x7ac, BIT(0), 0);

static SUNXI_CCU_GATE(bus_iommu_clk, "bus-iommu", "apb0",
		      0x7bc, BIT(0), 0);

static const char * const dram_parents[] = { "pll-ddr",
					     "pll-periph1-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(dram_clk, "dram",
				 dram_parents,
				 0x800,
				 0, 5,		/* M */
				 8, 2,		/* N */
				 24, 2,		/* mux */
				 BIT(31),	/* gate */
				 CLK_IS_CRITICAL);

static SUNXI_CCU_GATE(mbus_dma_clk, "mbus-dma", "mbus",
		      0x804, BIT(0), 0);
static SUNXI_CCU_GATE(mbus_ve3_clk, "mbus-ve3", "mbus",
		      0x804, BIT(1), 0);
static SUNXI_CCU_GATE(mbus_ce_clk, "mbus-ce", "mbus",
		      0x804, BIT(2), 0);
static SUNXI_CCU_GATE(mbus_av1_clk, "mbus-av1", "mbus",
		      0x804, BIT(3), 0);
static SUNXI_CCU_GATE(mbus_nand_clk, "mbus-nand", "mbus",
		      0x804, BIT(5), 0);

static SUNXI_CCU_GATE(bus_dram_clk, "bus-dram", "ahb",
		      0x80c, BIT(0), CLK_IS_CRITICAL);

static const char * const nand_spi_parents[] = { "dcxo24M", "pll-periph0",
					     "pll-periph1", "pll-periph0-2x",
					     "pll-periph1-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(nand0_clk, "nand0", nand_spi_parents, 0x810,
				  0, 4,		/* M */
				  8, 2,		/* P */
				  24, 3,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_MP_WITH_MUX_GATE(nand1_clk, "nand1", nand_spi_parents, 0x814,
				  0, 4,		/* M */
				  8, 2,		/* P */
				  24, 3,	/* mux */
				  BIT(31),	/* gate */
				  0);

static SUNXI_CCU_GATE(bus_nand_clk, "bus-nand", "ahb", 0x82c, BIT(0), 0);

/* XXX: don't use POSTDIV for BSP kernel */
static const char * const mmc_parents[] = { "dcxo24M", "pll-periph0-2x",
					    "pll-periph1-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE(mmc0_clk, "mmc0", mmc_parents, 0x830,
					  0, 4,		/* M */
					  8, 2,		/* P */
					  24, 3,	/* mux */
					  BIT(31),	/* gate */
					  0);

static SUNXI_CCU_MP_WITH_MUX_GATE(mmc1_clk, "mmc1", mmc_parents, 0x834,
					  0, 4,		/* M */
					  8, 2,		/* P */
					  24, 3,	/* mux */
					  BIT(31),	/* gate */
					  0);

static SUNXI_CCU_MP_WITH_MUX_GATE(mmc2_clk, "mmc2", mmc_parents, 0x838,
					  0, 4,		/* M */
					  8, 2,		/* P */
					  24, 3,	/* mux */
					  BIT(31),	/* gate */
					  0);

static SUNXI_CCU_GATE(bus_mmc0_clk, "bus-mmc0", "ahb", 0x84c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_mmc1_clk, "bus-mmc1", "ahb", 0x84c, BIT(1), 0);
static SUNXI_CCU_GATE(bus_mmc2_clk, "bus-mmc2", "ahb", 0x84c, BIT(2), 0);

static SUNXI_CCU_GATE(bus_uart0_clk, "bus-uart0", "apb1", 0x90c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_uart1_clk, "bus-uart1", "apb1", 0x90c, BIT(1), 0);
static SUNXI_CCU_GATE(bus_uart2_clk, "bus-uart2", "apb1", 0x90c, BIT(2), 0);
static SUNXI_CCU_GATE(bus_uart3_clk, "bus-uart3", "apb1", 0x90c, BIT(3), 0);

static SUNXI_CCU_GATE(bus_i2c0_clk, "bus-i2c0", "apb1", 0x91c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_i2c1_clk, "bus-i2c1", "apb1", 0x91c, BIT(1), 0);
static SUNXI_CCU_GATE(bus_i2c2_clk, "bus-i2c2", "apb1", 0x91c, BIT(2), 0);
static SUNXI_CCU_GATE(bus_i2c3_clk, "bus-i2c3", "apb1", 0x91c, BIT(3), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE(spi0_clk, "spi0", nand_spi_parents, 0x940,
					0, 4,	/* M */
					8, 2,	/* P */
					24, 3,	/* mux */
					BIT(31),/* gate */
					0);

static SUNXI_CCU_MP_WITH_MUX_GATE(spi1_clk, "spi1", nand_spi_parents, 0x944,
					0, 4,	/* M */
					8, 2,	/* P */
					24, 3,	/* mux */
					BIT(31),/* gate */
					0);

static SUNXI_CCU_GATE(bus_spi0_clk, "bus-spi0", "ahb", 0x96c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_spi1_clk, "bus-spi1", "ahb", 0x96c, BIT(1), 0);

static SUNXI_CCU_GATE(emac_25M_clk, "emac-25M", "ahb",
		      0x970, BIT(30) | BIT(31), 0);
static SUNXI_CCU_GATE(bus_emac_clk, "bus-emac", "ahb", 0x97c, BIT(0), 0);

static SUNXI_CCU_GATE(bus_gpadc_clk, "bus-gpadc", "apb1", 0x9ec, BIT(0), 0);

static SUNXI_CCU_GATE(bus_ths_clk, "bus-ths", "apb1", 0x9fc, BIT(0), 0);

static const char * const i2s_spdif_tx_parents[] = { "pll-audio"};
static SUNXI_CCU_MP_WITH_MUX_GATE(i2s0_clk, "i2s0", i2s_spdif_tx_parents,
			     0xa10,
			     0, 5,
			     8, 2,
			     24, 3,
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(i2s1_clk, "i2s1", i2s_spdif_tx_parents,
			     0xa14,
			     0, 5,
			     8, 2,
			     24, 3,
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(i2s2_clk, "i2s2", i2s_spdif_tx_parents,
				  0xa14,
				  0, 5,
				  8, 2,
				  24, 3,
				  BIT(31),
				  0);

static SUNXI_CCU_GATE(bus_i2s0_clk, "bus-i2s0", "apb1", 0xa20, BIT(0), 0);
static SUNXI_CCU_GATE(bus_i2s1_clk, "bus-i2s1", "apb1", 0xa20, BIT(1), 0);
static SUNXI_CCU_GATE(bus_i2s2_clk, "bus-i2s2", "apb1", 0xa20, BIT(2), 0);

static const char * const spdif_rx_parents[] = {"pll-periph0-2x",
						"pll-audio"};
static SUNXI_CCU_MP_WITH_MUX_GATE(spdif0_rx_clk, "spdif0-rx",
			     spdif_rx_parents,
			     0xa30,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(spdif0_tx_clk, "spdif0-tx",
			     i2s_spdif_tx_parents,
			     0xa34,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(spdif1_rx_clk, "spdif1-rx",
			     spdif_rx_parents,
			     0xa40,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(spdif1_tx_clk, "spdif1-tx",
			     i2s_spdif_tx_parents,
			     0xa44,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_GATE(bus_spdif0_clk, "bus-spdif0", "apb1", 0xa2c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_spdif1_clk, "bus-spdif1", "apb1", 0xa2c, BIT(1), 0);

static const char * const audio_hub_parents[] = { "pll-audio"};

static SUNXI_CCU_MP_WITH_MUX_GATE(audio_hub_clk, "audio-hub",
			     audio_hub_parents,
			     0xa50,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_GATE(bus_audio_hub_clk, "bus-audio_hub", "apb1", 0xa5c, BIT(0), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE(audio_codec_dac_clk, "audio-codec-dac",
			     audio_hub_parents,
			     0xa60,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(audio_codec_adc_clk, "audio-codec-adc",
			     audio_hub_parents,
			     0xa64,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_GATE(bus_audio_codec_clk, "bus-audio-codec", "apb1",
		      0xa6c, BIT(0), 0);

static SUNXI_CCU_GATE(usb_ohci0_clk, "usb-ohci0", "osc12M", 0xa70, BIT(31), 0);
static SUNXI_CCU_GATE(usb_ohci1_clk, "usb-ohci1", "osc12M", 0xa78, BIT(31), 0);
static SUNXI_CCU_GATE(usb_ohci2_clk, "usb-ohci2", "osc12M", 0xa80, BIT(31), 0);

/*
 * There are OHCI 12M clock source selection bits for 3 USB 2.0 ports.
 * We will force them to 0 (12M divided from 48M).
 */
static SUNXI_CCU_GATE(bus_ohci0_clk, "bus-ohci0", "osc12M", 0xa8c, BIT(0), 0);
static SUNXI_CCU_GATE(bus_ohci1_clk, "bus-ohci1", "osc12M", 0xa8c, BIT(1), 0);
static SUNXI_CCU_GATE(bus_ohci2_clk, "bus-ohci2", "osc12M", 0xa8c, BIT(2), 0);
static SUNXI_CCU_GATE(bus_ehci0_clk, "bus-ehci0", "ahb", 0xa8c, BIT(4), 0);
static SUNXI_CCU_GATE(bus_ehci1_clk, "bus-ehci1", "ahb", 0xa8c, BIT(5), 0);
static SUNXI_CCU_GATE(bus_ehci2_clk, "bus-ehci2", "ahb", 0xa8c, BIT(6), 0);
static SUNXI_CCU_GATE(bus_otg0_clk, "bus-otg0", "ahb", 0xa8c, BIT(8), 0);

static SUNXI_CCU_GATE(bus_lradc_clk, "bus-lradc", "ahb", 0xa9c, BIT(8), 0);

static const char *const adc_parents[] = {"pll-adc", "pll-video0"};

static SUNXI_CCU_MP_WITH_MUX_GATE(adc_clk, "adc",
			     adc_parents,
			     0xd10,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static const char * const dtmb_120M_parents[] = { "pll-adc", "pll-periph0" };

static SUNXI_CCU_MP_WITH_MUX_GATE(dtmb_120M_clk, "dtmb-120M",
			     dtmb_120M_parents,
			     0xd18,
			     0, 2,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static const char * const tvfe_1296M_parents[] = { "pll-video0-4x", "pll-adc" };

static SUNXI_CCU_MP_WITH_MUX_GATE(tvfe_1296M_clk, "tvfe_1296M_clk",
			     tvfe_1296M_parents,
			     0xd18,
			     0, 2,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static const char * const i2h_parents[] = { "pll-video0-4x",
					    "pll-periph0-2x",
					    "pll-periph1-2x"};

static SUNXI_CCU_M_WITH_MUX_GATE(i2h_clk, "i2h", i2h_parents, 0xd24,
				 0, 5,		/* M */
				 24, 3,		/* mux */
				 BIT(31),	/* gate */
				 0);

static const char * const cip_tsx_parents[] = { "pll-periph0",
					    "pll-periph1"};

static SUNXI_CCU_M_WITH_MUX_GATE(cip_tsx_clk, "cip-tsx", cip_tsx_parents,  0xd28,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static SUNXI_CCU_M_WITH_MUX_GATE(cip_mcx_clk, "cip-mcx", cip_tsx_parents,  0xd2c,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static const char * const cip_tsp_parents[] = { "pll-video0-4x",
					    "pll-adc"};

static SUNXI_CCU_M_WITH_MUX_GATE(cip_tsp_clk, "cip-tsp", cip_tsp_parents,  0xd30,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static SUNXI_CCU_M_WITH_MUX_GATE(tsa_tsp_clk, "tsa-tsp", cip_tsp_parents,  0xd34,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static const char * const cip27_parents[] = { "pll-video0",
					      "pll-adc" };

static SUNXI_CCU_MP_WITH_MUX_GATE(cip27_clk, "cip27",
			     cip27_parents,
			     0xd38,
			     0, 4,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static const char *const cip_mts0_parents[] = { "pll-video1-4x", "pll-video0-4x",
					      "pll-video3-4x", "pll-adc",
					      "pll-periph0-2x"};

static SUNXI_CCU_MP_WITH_MUX_GATE(cip_mts0_clk, "cip-mts",
			     cip_mts0_parents,
			     0xd40,
			     0, 5,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static const char * const audio_cpu_parents[] = { "pll-periph0-2x",
						  "pll-periph1-2x"};

static SUNXI_CCU_M_WITH_MUX_GATE(audio_cpu_clk, "audio_cpu", audio_cpu_parents,  0xd48,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static const char * const audio_umac_parents[] = { "pll-periph0",
						  "pll-periph1"};

static SUNXI_CCU_M_WITH_MUX_GATE(audio_umac_clk, "audio_umac", audio_umac_parents,  0xd4c,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static const char * const audio_ihb_parents[] = { "pll-video0-4x",
						  "pll-periph0",
						  "pll-periph1"};

static SUNXI_CCU_M_WITH_MUX_GATE(audio_ihb_clk, "audio_ihb", audio_ihb_parents,  0xd50,
				 0, 4,
				 24, 3,
				 BIT(31),
				 0);

static const char * const tsa432_parents[] = { "pll-video0-4x",
					       "pll-adc"};

static SUNXI_CCU_M_WITH_MUX_GATE(tsa432_clk, "tsa432", tsa432_parents,  0xd58,
				 0, 3,
				 24, 1,
				 BIT(31),
				 0);

static const char *const mpg_parents[] = { "pll-video1-4x", "dcxo24M",
					  "pll-adc"};

static SUNXI_CCU_MP_WITH_MUX_GATE(mpg0_clk, "mpg0",
			     mpg_parents,
			     0xd5c,
			     0, 4,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_MP_WITH_MUX_GATE(mpg1_clk, "mpg1",
			     mpg_parents,
			     0xd60,
			     0, 4,	/* m */
			     8, 2,	/* p */
			     24, 3,	/* mux */
			     BIT(31),
			     0);

static SUNXI_CCU_GATE(bus_demod_clk, "bus-demod", "pll-video0-4x",
		      0xd64, BIT(0), 0);

static const char *const tcd3_parents[] = { "pll-video0-4x", "pll-adc"};

static SUNXI_CCU_MP_WITH_MUX_GATE(tcd3_clk, "tcd3",
			     tcd3_parents,
			     0xd6c,
			     0, 3,	/* m */
			     8, 2,	/* p */
			     24, 1,	/* mux */
			     BIT(31),
			     0);

static const char *const vincap_dma_parents[] = { "pll-video2-4x", "pll-periph0"};
static SUNXI_CCU_M_WITH_MUX_GATE(vincap_dma_clk, "vincap-dma", vincap_dma_parents,
			     0xd74,
			     0, 5,	/* M */
			     24, 1,	/* mux */
			     BIT(31),	/* gate */
			     0);
static SUNXI_CCU_GATE(bus_hdmi_audio_clk, "bus-hdmi-audio", "ahb",
		      0xd80, BIT(31), 0);
static SUNXI_CCU_GATE(bus_cap_300M_clk, "bus-cap-300M", "ahb",
		      0xd80, BIT(30), 0);

static const char *const hdmi_audio_parents[] = { "pll-video3-4x", "pll-periph0-2x"};
static SUNXI_CCU_M_WITH_MUX_GATE(hdmi_audio_clk, "hdmi-audio", hdmi_audio_parents,
			     0xd84,
			     0, 5,	/* M */
			     24, 1,	/* mux */
			     BIT(31),	/* gate */
			     0);

static SUNXI_CCU_GATE(bus_tvcap_clk, "bus-tvcap", "ahb",
		      0xd88, BIT(0), 0);

static SUNXI_CCU_M_WITH_GATE(deint_clk, "deint", "pll-video2-4x",
			     0xdb0, 0, 5, BIT(31), 0);

static const char * const svp_dtl_parents[] = { "pll-periph0-2x",
						"pll-video0-4x",
						"pll-periph1-2x"};
static SUNXI_CCU_M_WITH_MUX_GATE(svp_dtl_clk, "svp-dtl", svp_dtl_parents,
				 0xdb8,
				 0, 5,		/* M */
				 24, 3,		/* mux */
				 BIT(31) | BIT(30),	/* gate */
				 0);

static const char * const afbd_parents[] = { "pll-periph0-2x",
					     "pll-periph0",
					     "pll-video0-4x",
					     "pll-adc"};
static SUNXI_CCU_M_WITH_MUX_GATE(afbd_clk, "afbd", afbd_parents,
				 0xdc0,
				 0, 4,		/* M */
				 24, 3,		/* mux */
				 BIT(31),	/* gate */
				 0);

static SUNXI_CCU_GATE(bus_disp_clk, "bus-disp", "apb1",
		      0xdd8, BIT(0), 0);

/* Fixed factor clocks */
static CLK_FIXED_FACTOR_FW_NAME(osc12M_clk, "osc12M", "hosc", 2, 1, 0);

static CLK_FIXED_FACTOR_HW(pll_periph0_2x_clk, "pll-periph0-2x",
			   &pll_periph0_clk.common.hw,
			   1, 2, 0);

static CLK_FIXED_FACTOR_HW(pll_periph1_2x_clk, "pll-periph1-2x",
			   &pll_periph1_clk.common.hw,
			   1, 2, 0);

static CLK_FIXED_FACTOR_HW(pll_video0_4x_clk, "pll-video0-4x",
			   &pll_video0_clk.common.hw,
			   1, 4, CLK_SET_RATE_PARENT);
static CLK_FIXED_FACTOR_HW(pll_video1_4x_clk, "pll-video1-4x",
			   &pll_video1_clk.common.hw,
			   1, 4, CLK_SET_RATE_PARENT);
static CLK_FIXED_FACTOR_HW(pll_video2_4x_clk, "pll-video2-4x",
			   &pll_video2_clk.common.hw,
			   1, 4, CLK_SET_RATE_PARENT);
static CLK_FIXED_FACTOR_HW(pll_video3_4x_clk, "pll-video3-4x",
			   &pll_video3_clk.common.hw,
			   1, 4, CLK_SET_RATE_PARENT);

static struct ccu_common *sun50iw12_ccu_clks[] = {
	&pll_cpux_clk.common,
	&pll_ddr_clk.common,
	&pll_periph0_clk.common,
	&pll_periph1_clk.common,
	&pll_gpu_clk.common,
	&pll_video0_clk.common,
	&pll_video1_clk.common,
	&pll_video2_clk.common,
	&pll_ve_clk.common,
	&pll_adc_clk.common,
	&pll_video3_clk.common,
	&pll_audio_clk.common,
	&cpux_clk.common,
	&axi_clk.common,
	&cpux_apb_clk.common,
	&ahb_clk.common,
	&apb0_clk.common,
	&apb1_clk.common,
	&mbus_clk.common,
	&mips_clk.common,
	&bus_mips_clk.common,
	&gpu_clk.common,
	&bus_gpu_clk.common,
	&ce_clk.common,
	&bus_ce_clk.common,
	&bus_ce_sys_clk.common,
	&ve_core_clk.common,
	&bus_ve_clk.common,
	&bus_av1_clk.common,
	&bus_ve3_clk.common,
	&bus_dma_clk.common,
	&bus_msgbox_clk.common,
	&bus_spinlock_clk.common,
	&timer0_clk.common,
	&timer1_clk.common,
	&timer2_clk.common,
	&timer3_clk.common,
	&timer4_clk.common,
	&timer5_clk.common,
	&bus_timer0_clk.common,
	&bus_dbg_clk.common,
	&bus_pwm_clk.common,
	&bus_iommu_clk.common,
	&dram_clk.common,
	&mbus_dma_clk.common,
	&mbus_ve3_clk.common,
	&mbus_ce_clk.common,
	&mbus_av1_clk.common,
	&mbus_nand_clk.common,
	&bus_dram_clk.common,
	&nand0_clk.common,
	&nand1_clk.common,
	&bus_nand_clk.common,
	&mmc0_clk.common,
	&mmc1_clk.common,
	&mmc2_clk.common,
	&bus_mmc0_clk.common,
	&bus_mmc1_clk.common,
	&bus_mmc2_clk.common,
	&bus_uart0_clk.common,
	&bus_uart1_clk.common,
	&bus_uart2_clk.common,
	&bus_uart3_clk.common,
	&bus_i2c0_clk.common,
	&bus_i2c1_clk.common,
	&bus_i2c2_clk.common,
	&bus_i2c3_clk.common,
	&spi0_clk.common,
	&spi1_clk.common,
	&bus_spi0_clk.common,
	&bus_spi1_clk.common,
	&emac_25M_clk.common,
	&bus_emac_clk.common,
	&bus_gpadc_clk.common,
	&bus_ths_clk.common,
	&i2s0_clk.common,
	&i2s1_clk.common,
	&i2s2_clk.common,
	&bus_i2s0_clk.common,
	&bus_i2s1_clk.common,
	&bus_i2s2_clk.common,
	&spdif0_rx_clk.common,
	&spdif0_tx_clk.common,
	&spdif1_rx_clk.common,
	&spdif1_tx_clk.common,
	&bus_spdif0_clk.common,
	&bus_spdif1_clk.common,
	&audio_hub_clk.common,
	&bus_audio_hub_clk.common,
	&audio_codec_dac_clk.common,
	&audio_codec_adc_clk.common,
	&bus_audio_codec_clk.common,
	&usb_ohci0_clk.common,
	&usb_ohci1_clk.common,
	&usb_ohci2_clk.common,
	&bus_ohci0_clk.common,
	&bus_ohci1_clk.common,
	&bus_ohci2_clk.common,
	&bus_ehci0_clk.common,
	&bus_ehci1_clk.common,
	&bus_ehci2_clk.common,
	&bus_otg0_clk.common,
	&bus_lradc_clk.common,
	&adc_clk.common,
	&dtmb_120M_clk.common,
	&tvfe_1296M_clk.common,
	&i2h_clk.common,
	&cip_tsx_clk.common,
	&cip_mcx_clk.common,
	&cip_tsp_clk.common,
	&tsa_tsp_clk.common,
	&cip27_clk.common,
	&cip_mts0_clk.common,
	&audio_cpu_clk.common,
	&audio_umac_clk.common,
	&audio_ihb_clk.common,
	&tsa432_clk.common,
	&mpg0_clk.common,
	&mpg1_clk.common,
	&bus_demod_clk.common,
	&tcd3_clk.common,
	&vincap_dma_clk.common,
	&bus_hdmi_audio_clk.common,
	&bus_cap_300M_clk.common,
	&hdmi_audio_clk.common,
	&bus_tvcap_clk.common,
	&deint_clk.common,
	&svp_dtl_clk.common,
	&afbd_clk.common,
	&bus_disp_clk.common,
};

static struct clk_hw_onecell_data sun50iw12_hw_clks = {
	.hws	= {
		[CLK_OSC12M]		= &osc12M_clk.hw,
		[CLK_PLL_CPUX]		= &pll_cpux_clk.common.hw,
		[CLK_PLL_DDR0]		= &pll_ddr_clk.common.hw,
		[CLK_PLL_PERIPH0]	= &pll_periph0_clk.common.hw,
		[CLK_PLL_PERIPH0_2X]	= &pll_periph0_2x_clk.hw,
		[CLK_PLL_PERIPH1]	= &pll_periph1_clk.common.hw,
		[CLK_PLL_PERIPH1_2X]	= &pll_periph1_2x_clk.hw,
		[CLK_PLL_GPU]		= &pll_gpu_clk.common.hw,
		[CLK_PLL_VIDEO0]	= &pll_video0_clk.common.hw,
		[CLK_PLL_VIDEO0_4X]	= &pll_video0_4x_clk.hw,
		[CLK_PLL_VIDEO1]	= &pll_video1_clk.common.hw,
		[CLK_PLL_VIDEO1_4X]	= &pll_video1_4x_clk.hw,
		[CLK_PLL_VIDEO2]	= &pll_video2_clk.common.hw,
		[CLK_PLL_VIDEO2_4X]	= &pll_video2_4x_clk.hw,
		[CLK_PLL_VE]		= &pll_ve_clk.common.hw,
		[CLK_PLL_ADC]		= &pll_adc_clk.common.hw,
		[CLK_PLL_VIDEO3]	= &pll_video3_clk.common.hw,
		[CLK_PLL_VIDEO3_4X]	= &pll_video3_4x_clk.hw,
		[CLK_PLL_AUDIO]		= &pll_audio_clk.common.hw,
		[CLK_CPUX]		= &cpux_clk.common.hw,
		[CLK_AXI]		= &axi_clk.common.hw,
		[CLK_CPUX_APB]		= &cpux_apb_clk.common.hw,
		[CLK_AHB]		= &ahb_clk.common.hw,
		[CLK_APB0]		= &apb0_clk.common.hw,
		[CLK_APB1]		= &apb1_clk.common.hw,
		[CLK_MBUS]		= &mbus_clk.common.hw,
		[CLK_MIPS]		= &mips_clk.common.hw,
		[CLK_BUS_MIPS]		= &bus_mips_clk.common.hw,
		[CLK_GPU]		= &gpu_clk.common.hw,
		[CLK_BUS_GPU]		= &bus_gpu_clk.common.hw,
		[CLK_CE]		= &ce_clk.common.hw,
		[CLK_BUS_CE]		= &bus_ce_clk.common.hw,
		[CLK_VE_CORE]		= &ve_core_clk.common.hw,
		[CLK_BUS_VE]		= &bus_ve_clk.common.hw,
		[CLK_BUS_AV1]		= &bus_av1_clk.common.hw,
		[CLK_BUS_VE3]		= &bus_ve3_clk.common.hw,
		[CLK_BUS_DMA]		= &bus_dma_clk.common.hw,
		[CLK_MSGBOX]		= &bus_msgbox_clk.common.hw,
		[CLK_SPINLOCK]		= &bus_spinlock_clk.common.hw,
		[CLK_TIMER0]		= &timer0_clk.common.hw,
		[CLK_TIMER1]		= &timer1_clk.common.hw,
		[CLK_TIMER2]		= &timer2_clk.common.hw,
		[CLK_TIMER3]		= &timer3_clk.common.hw,
		[CLK_TIMER4]		= &timer4_clk.common.hw,
		[CLK_TIMER5]		= &timer5_clk.common.hw,
		[CLK_BUS_TIMER0]	= &bus_timer0_clk.common.hw,
		[CLK_BUS_DBG]		= &bus_dbg_clk.common.hw,
		[CLK_BUS_PWM]		= &bus_pwm_clk.common.hw,
		[CLK_BUS_IOMMU]		= &bus_iommu_clk.common.hw,
		[CLK_DRAM]		= &dram_clk.common.hw,
		[CLK_MBUS_DMA]		= &mbus_dma_clk.common.hw,
		[CLK_MBUS_VE3]		= &mbus_ve3_clk.common.hw,
		[CLK_MBUS_CE]		= &mbus_ce_clk.common.hw,
		[CLK_MBUS_AV1]		= &mbus_av1_clk.common.hw,
		[CLK_MBUS_NAND]		= &mbus_nand_clk.common.hw,
		[CLK_BUS_DRAM]		= &bus_dram_clk.common.hw,
		[CLK_NAND0]		= &nand0_clk.common.hw,
		[CLK_NAND1]		= &nand1_clk.common.hw,
		[CLK_BUS_NAND]		= &bus_nand_clk.common.hw,
		[CLK_MMC0]		= &mmc0_clk.common.hw,
		[CLK_MMC1]		= &mmc1_clk.common.hw,
		[CLK_MMC2]		= &mmc2_clk.common.hw,
		[CLK_BUS_MMC0]		= &bus_mmc0_clk.common.hw,
		[CLK_BUS_MMC1]		= &bus_mmc1_clk.common.hw,
		[CLK_BUS_MMC2]		= &bus_mmc2_clk.common.hw,
		[CLK_BUS_UART0]		= &bus_uart0_clk.common.hw,
		[CLK_BUS_UART1]		= &bus_uart1_clk.common.hw,
		[CLK_BUS_UART2]		= &bus_uart2_clk.common.hw,
		[CLK_BUS_UART3]		= &bus_uart3_clk.common.hw,
		[CLK_BUS_I2C0]		= &bus_i2c0_clk.common.hw,
		[CLK_BUS_I2C1]		= &bus_i2c1_clk.common.hw,
		[CLK_BUS_I2C2]		= &bus_i2c2_clk.common.hw,
		[CLK_BUS_I2C3]		= &bus_i2c3_clk.common.hw,
		[CLK_SPI0]		= &spi0_clk.common.hw,
		[CLK_SPI1]		= &spi1_clk.common.hw,
		[CLK_BUS_SPI0]		= &bus_spi0_clk.common.hw,
		[CLK_BUS_SPI1]		= &bus_spi1_clk.common.hw,
		[CLK_EMAC_25M]		= &emac_25M_clk.common.hw,
		[CLK_BUS_EMAC]		= &bus_emac_clk.common.hw,
		[CLK_BUS_GPADC]		= &bus_gpadc_clk.common.hw,
		[CLK_BUS_THS]		= &bus_ths_clk.common.hw,
		[CLK_I2S0]		= &i2s0_clk.common.hw,
		[CLK_I2S1]		= &i2s1_clk.common.hw,
		[CLK_I2S2]		= &i2s2_clk.common.hw,
		[CLK_BUS_I2S0]		= &bus_i2s0_clk.common.hw,
		[CLK_BUS_I2S1]		= &bus_i2s1_clk.common.hw,
		[CLK_BUS_I2S2]		= &bus_i2s2_clk.common.hw,
		[CLK_SPDIF0_RX]		= &spdif0_rx_clk.common.hw,
		[CLK_SPDIF0_TX]		= &spdif0_tx_clk.common.hw,
		[CLK_SPDIF1_RX]		= &spdif1_rx_clk.common.hw,
		[CLK_SPDIF1_TX]		= &spdif1_tx_clk.common.hw,
		[CLK_BUS_SPDIF0]	= &bus_spdif0_clk.common.hw,
		[CLK_BUS_SPDIF1]	= &bus_spdif1_clk.common.hw,
		[CLK_AUDIO_HUB]		= &audio_hub_clk.common.hw,
		[CLK_AUDIO_CODEC_DAC]	= &audio_codec_dac_clk.common.hw,
		[CLK_AUDIO_CODEC_ADC]	= &audio_codec_adc_clk.common.hw,
		[CLK_BUS_AUDIO_CODEC]	= &bus_audio_codec_clk.common.hw,
		[CLK_USB_OHCI0]		= &usb_ohci0_clk.common.hw,
		[CLK_USB_OHCI1]		= &usb_ohci1_clk.common.hw,
		[CLK_USB_OHCI2]		= &usb_ohci2_clk.common.hw,
		[CLK_BUS_OHCI0]		= &bus_ohci0_clk.common.hw,
		[CLK_BUS_OHCI1]		= &bus_ohci1_clk.common.hw,
		[CLK_BUS_OHCI2]		= &bus_ohci2_clk.common.hw,
		[CLK_BUS_EHCI0]		= &bus_ehci0_clk.common.hw,
		[CLK_BUS_EHCI1]		= &bus_ehci1_clk.common.hw,
		[CLK_BUS_EHCI2]		= &bus_ehci2_clk.common.hw,
		[CLK_BUS_OTG]		= &bus_otg0_clk.common.hw,
		[CLK_BUS_LRADC]		= &bus_lradc_clk.common.hw,
		[CLK_ADC]		= &adc_clk.common.hw,
		[CLK_DTMB_120M]		= &dtmb_120M_clk.common.hw,
		[CLK_TVFE_1296M]	= &tvfe_1296M_clk.common.hw,
		[CLK_I2H]		= &i2h_clk.common.hw,
		[CLK_CIP_TSX]		= &cip_tsx_clk.common.hw,
		[CLK_CIP_MCX]		= &cip_mcx_clk.common.hw,
		[CLK_CIP_TSP]		= &cip_tsp_clk.common.hw,
		[CLK_TSA_TSP]		= &tsa_tsp_clk.common.hw,
		[CLK_CIP27]		= &cip27_clk.common.hw,
		[CLK_CIP_MTS0]		= &cip_mts0_clk.common.hw,
		[CLK_AUDIO_CPU]		= &audio_cpu_clk.common.hw,
		[CLK_AUDIO_UMAC]	= &audio_umac_clk.common.hw,
		[CLK_AUDIO_IHB]		= &audio_ihb_clk.common.hw,
		[CLK_TSA432]		= &tsa432_clk.common.hw,
		[CLK_MPG0]		= &mpg0_clk.common.hw,
		[CLK_MPG1]		= &mpg1_clk.common.hw,
		[CLK_BUS_DEMOD]		= &bus_demod_clk.common.hw,
		[CLK_TCD3]		= &tcd3_clk.common.hw,
		[CLK_VINCAP_DMA]	= &vincap_dma_clk.common.hw,
		[CLK_BUS_HDMI_AUDIO]	= &bus_hdmi_audio_clk.common.hw,
		[CLK_BUS_CAP_300M]	= &bus_cap_300M_clk.common.hw,
		[CLK_HDMI_AUDIO]	= &hdmi_audio_clk.common.hw,
		[CLK_BUS_TVCAP]		= &bus_tvcap_clk.common.hw,
		[CLK_DEINT]		= &deint_clk.common.hw,
		[CLK_SVP_DTL]		= &svp_dtl_clk.common.hw,
		[CLK_AFBD]		= &afbd_clk.common.hw,
		[CLK_BUS_DISP]		= &bus_disp_clk.common.hw,
	},
	.num = CLK_NUMBER,
};

static struct ccu_reset_map sun50iw12_ccu_resets[] = {
	[RST_MBUS]		= { 0x540, BIT(30) },
	[RST_BUS_MIPS]		= { 0x60c, BIT(16) },
	[RST_BUS_MIPS_COLD]	= { 0x60c, BIT(17) },
	[RST_BUS_MIPS_SOFT]	= { 0x60c, BIT(18) },
	[RST_BUS_GPU]		= { 0x67c, BIT(16) },
	[RST_BUS_CE]		= { 0x68c, BIT(16) },
	[RST_BUS_CE_SYS]	= { 0x68c, BIT(17) },
	[RST_BUS_VE]		= { 0x69c, BIT(16) },
	[RST_BUS_AV1]		= { 0x69c, BIT(17) },
	[RST_BUS_VE3]		= { 0x69c, BIT(18) },
	[RST_BUS_DMA]		= { 0x70c, BIT(16) },
	[RST_BUS_MSGBOX]	= { 0x71c, BIT(16) },
	[RST_BUS_SPINLOCK]	= { 0x72c, BIT(16) },
	[RST_BUS_TIMER0]	= { 0x750, BIT(16) },
	[RST_BUS_DBGSYS]	= { 0x78c, BIT(16) },
	[RST_BUS_PWM]		= { 0x7ac, BIT(16) },
	[RST_BUS_DRAM_MODULE]	= { 0x800, BIT(30) },
	[RST_BUS_DRAM]		= { 0x80c, BIT(16) },
	[RST_BUS_NAND]		= { 0x82c, BIT(16) },
	[RST_BUS_MMC0]		= { 0x84c, BIT(16) },
	[RST_BUS_MMC1]		= { 0x84c, BIT(17) },
	[RST_BUS_MMC2]		= { 0x84c, BIT(18) },
	[RST_BUS_UART0]		= { 0x90c, BIT(16) },
	[RST_BUS_UART1]		= { 0x90c, BIT(17) },
	[RST_BUS_UART2]		= { 0x90c, BIT(18) },
	[RST_BUS_UART3]		= { 0x90c, BIT(19) },
	[RST_BUS_UART4]		= { 0x90c, BIT(20) },
	[RST_BUS_I2C0]		= { 0x91c, BIT(16) },
	[RST_BUS_I2C1]		= { 0x91c, BIT(17) },
	[RST_BUS_I2C2]		= { 0x91c, BIT(18) },
	[RST_BUS_I2C3]		= { 0x91c, BIT(19) },
	[RST_BUS_SPI0]		= { 0x96c, BIT(16) },
	[RST_BUS_SPI1]		= { 0x96c, BIT(17) },
	[RST_BUS_EMAC]		= { 0x97c, BIT(16) },
	[RST_BUS_GPADC]		= { 0x9ec, BIT(16) },
	[RST_BUS_THS]		= { 0x9fc, BIT(16) },
	[RST_BUS_I2S0]		= { 0xa20, BIT(16) },
	[RST_BUS_I2S1]		= { 0xa20, BIT(17) },
	[RST_BUS_I2S2]		= { 0xa20, BIT(18) },
	[RST_BUS_SPDIF0]	= { 0xa2c, BIT(16) },
	[RST_BUS_SPDIF1]	= { 0xa2c, BIT(17) },
	[RST_BUS_AUDIO_HUB]	= { 0xa5c, BIT(16) },
	[RST_BUS_AUDIO_CODEC]	= { 0xa6c, BIT(16) },
	[RST_USB_PHY0]		= { 0xa70, BIT(30) },
	[RST_USB_PHY1]		= { 0xa78, BIT(30) },
	[RST_USB_PHY2]		= { 0xa80, BIT(30) },
	[RST_BUS_OHCI0]		= { 0xa8c, BIT(16) },
	[RST_BUS_OHCI1]		= { 0xa8c, BIT(17) },
	[RST_BUS_OHCI2]		= { 0xa8c, BIT(18) },
	[RST_BUS_EHCI0]		= { 0xa8c, BIT(20) },
	[RST_BUS_EHCI1]		= { 0xa8c, BIT(21) },
	[RST_BUS_EHCI2]		= { 0xa8c, BIT(22) },
	[RST_BUS_OTG]		= { 0xa8c, BIT(24) },
	[RST_BUS_LRADC]		= { 0xa9c, BIT(16) },
	[RST_BUS_LVDS]		= { 0xbac, BIT(16) },
	[RST_BUS_DEMOD]		= { 0xd64, BIT(16) },
	[RST_BUS_TVCAP]		= { 0xd88, BIT(16) },
	[RST_BUS_DISP]		= { 0xdd8, BIT(16) },
};

static const struct sunxi_ccu_desc sun50iw12_ccu_desc = {
	.ccu_clks	= sun50iw12_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun50iw12_ccu_clks),

	.hw_clks	= &sun50iw12_hw_clks,

	.resets		= sun50iw12_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun50iw12_ccu_resets),
};

static const u32 pll_regs[] = {
	SUN50IW12_PLL_CPUX_REG,
	SUN50IW12_PLL_DDR_REG,
	SUN50IW12_PLL_PERIPH0_REG,
	SUN50IW12_PLL_PERIPH1_REG,
	SUN50IW12_PLL_GPU_REG,
	SUN50IW12_PLL_VIDEO0_REG,
	SUN50IW12_PLL_VIDEO1_REG,
	SUN50IW12_PLL_VIDEO2_REG,
	SUN50IW12_PLL_VE_REG,
	SUN50IW12_PLL_ADC_REG,
	SUN50IW12_PLL_VIDEO3_REG,
	SUN50IW12_PLL_AUDIO_REG,
};

static void __init of_sun50iw12_ccu_init(struct device_node *node)
{
	void __iomem *reg;
	int i;
	u32 val;

	reg = of_iomap(node, 0);
	if (IS_ERR(reg))
		return;

	/* Enable the lock bits on all PLLs */
	for (i = 0; i < ARRAY_SIZE(pll_regs); i++) {
		val = readl(reg + pll_regs[i]);
		val |= BIT(29);
		writel(val, reg + pll_regs[i]);
	}

	sunxi_ccu_probe(node, reg, &sun50iw12_ccu_desc);
}

CLK_OF_DECLARE(sun50iw12_ccu_init, "allwinner,sun50iw12-ccu", of_sun50iw12_ccu_init);
