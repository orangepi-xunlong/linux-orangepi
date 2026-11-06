/*
 * Copyright (c) 2021 CVITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/syscore_ops.h>

#include <dt-bindings/clock/cv186x-clock.h>

//#define CV186X_CLK_FLAGS_ALL	(CLK_GET_RATE_NOCACHE)
// #define CV186X_CLK_FLAGS_ALL	(CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL)
#define CV186X_CLK_FLAGS_ALL	(CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED)
#define CV186X_CLK_FLAGS_MUX	(CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT)

/* top_pll_g0 */
#define REG_PLL_G0_CTRL			0x400
#define REG_PLL_G0_STATUS		0x404
#define REG_MIPIMPLL0_CSR		0x480
#define REG_CAM0PLL_CSR			0x4A0
#define REG_DISPPLL0_CSR		0x4C0
#define REG_CAM0PLL_SSC_SYN_CTRL	0x470
#define REG_DISPPLL0_SSC_SYN_CTRL	0x460

/* top_pll_g1 */
#define REG_PLL_G1_CTRL			0x500
#define REG_PLL_G1_STATUS		0x504
#define REG_MIPIMPLL1_CSR		0x580
#define REG_CAM1PLL_CSR			0x5A0
#define REG_DISPPLL1_CSR		0x5C0
#define REG_CAM1PLL_SSC_SYN_CTRL	0x570
#define REG_DISPPLL1_SSC_SYN_CTRL	0x560

/* top_pll_g5 */
#define REG_PLL_G5_CTRL			0x600
#define REG_PLL_G5_STATUS		0x604
#define REG_A53PLL_CSR			0x680
#define REG_RVPLL_CSR			0x6C0
#define REG_RVPLL_SSC_SYN_CTRL		0x660

/* top_pll_g2 */
#define REG_PLL_G2_CTRL			0x700
#define REG_PLL_G2_STATUS		0x704
#define REG_MIPIMPLL2_CSR		0x780
#define REG_APLL0_CSR			0x7C0
#define REG_APLL0_SSC_SYN_CTRL		0x760

/* top_pll_g6 */
#define REG_PLL_G6_CTRL			0x900
#define REG_PLL_G6_STATUS		0x904
#define REG_FPLL_CSR			0x980
#define REG_TPLL_CSR			0x9A0
#define REG_MPLL_CSR			0x9C0

/* clkgen */
#define REG_CLK_EN_0			0x168
#define REG_CLK_EN_1			0x16c
#define REG_CLK_EN_2			0x170
#define REG_CLK_EN_3			0x174
#define REG_CLK_EN_4			0x178
#define REG_CLK_EN_5			0x17c
#define REG_CLK_EN_6			0x180
#define REG_CLK_EN_7			0x184
#define REG_CLK_EN_8			0x188
#define REG_CLK_EN_9			0x18c
#define REG_CLK_EN_10			0x190
#define REG_CLK_SEL_0			0x194
#define REG_CLK_BYP_0			0x198
#define REG_CLK_BYP_1			0x19c
#define REG_CLK_BYP_2			0x1a0
#define REG_CLK_BYP_3			0x1a4
#define REG_CLK_BYP_4			0x1a8

#define REG_DIV_TOP_CLK_FAB0		0x0
#define REG_DIV_TOP_CLK_FAB1		0x4
#define REG_DIV_TOP_CLK_HSPERI		0x8
#define REG_DIV_RTC_CLK_RTC_FAB		0xc
#define REG_DIV_TOP_CLK_1M		0x10
#define REG_DIV_AP_CPU_CLK_0		0x14
#define REG_DIV_AP_CPU_CLK_1		0x18
#define REG_DIV_AP_CLK_RV0_0		0x1c
#define REG_DIV_AP_CLK_RV0_1		0x20
#define REG_DIV_AP_DBG_CLK		0x24
#define REG_DIV_AP_BUS_CLK		0x28
#define REG_DIV_TPU_CLK_TPU		0x2c
#define REG_DIV_TPU_CLK_TC906B		0x30
#define REG_DIV_TPU_CLK_TIMER_50M	0x34
#define REG_DIV_TPU_CLK_GDMA		0x38
#define REG_DIV_VD0_CLK_SRC1_VDSYS_0	0x3c
#define REG_DIV_VD0_CLK_SRC1_VDSYS_1	0x40
#define REG_DIV_VD0_CLK_SRC0_VDSYS	0x44
#define REG_DIV_VD0_CLK_SRC2_VDSYS	0x48
#define REG_DIV_VD0_CLK_AXI_VDSYS_0	0x4c
#define REG_DIV_VD0_CLK_AXI_VDSYS_1	0x50
#define REG_DIV_VD1_CLK_SRC1_VDSYS_0	0x54
#define REG_DIV_VD1_CLK_SRC1_VDSYS_1	0x58
#define REG_DIV_VD1_CLK_SRC0_VDSYS	0x5c
#define REG_DIV_VD1_CLK_SRC2_VDSYS	0x60
#define REG_DIV_VD1_CLK_AXI_VDSYS_0	0x64
#define REG_DIV_VD1_CLK_AXI_VDSYS_1	0x68
#define REG_DIV_VE_CLK_SRC1_VESYS_0	0x6c
#define REG_DIV_VE_CLK_SRC1_VESYS_1	0x70
#define REG_DIV_VE_CLK_SRC0_VESYS	0x74
#define REG_DIV_VE_CLK_SRC2_VESYS	0x78
#define REG_DIV_VE_CLK_AXI_VESYS	0x7c
#define REG_DIV_VI_CLK_RAW_AXI		0x80
#define REG_DIV_VI_CAM0_CLK		0x84
#define REG_DIV_VI_CAM1_CLK		0x88
#define REG_DIV_VI_CAM2_CLK		0x8c
#define REG_DIV_VI_CLK_SRC_VI_SYS_0	0x90
#define REG_DIV_VI_CLK_SRC_VI_SYS_1	0x94
#define REG_DIV_VI_CLK_SRC_VI_SYS_2	0x98
#define REG_DIV_VI_CLK_SRC_VI_SYS_3	0x9c
#define REG_DIV_VI_CLK_SRC_VI_SYS_4	0xa0
#define REG_DIV_VI_CLK_SRC_VI_SYS_5	0xa4
#define REG_DIV_VO_CLK_RAW_AXI		0xa8
#define REG_DIV_VO_CLK_SRC_VO_SYS_0	0xac
#define REG_DIV_VO_CLK_SRC_VO_SYS_1	0xb0
#define REG_DIV_VO_CLK_SRC_VO_SYS_2	0xb4
#define REG_DIV_VO_CLK_SYS_DISP0	0xb8
#define REG_DIV_VO_CLK_SYS_DISP1	0xbc
#define REG_DIV_VO_CLK_RAW_DSI_TX_ESC0	0xc0
#define REG_DIV_VO_CLK_RAW_DSI_TX_ESC1	0xc4
#define REG_DIV_VO_CLK_SFR		0xc8
#define REG_DIV_VO_CLK_MIPIMPLL0	0xcc
#define REG_DIV_DDR_ACLK_M2_SYS1	0xd0
#define REG_DIV_USB_TEST_PHY_CLK_480	0xd4
#define REG_DIV_USB_TEST_PHY_CLK_500	0xd8
#define REG_DIV_USB_CLK_USB_FAB		0xdc
#define REG_DIV_USB_CLK_USB_SUSPEND	0xe0
#define REG_DIV_SSPERI_ACLK_DDR		0xe4
#define REG_DIV_SSPERI_TEST_PHY_CLK	0xe8
#define REG_DIV_SSPERI_CLK_RXOOB	0xec
#define REG_DIV_HSPERI_CLK_SD0		0xf0
#define REG_DIV_HSPERI_CLK_SD1		0xf4
#define REG_DIV_HSPERI_CLK_SD2		0xf8
#define REG_DIV_HSPERI_EMMC_CARD_CLK	0xfc
#define REG_DIV_HSPERI_ETHER0_CLK_ETH_TX		0x100
#define REG_DIV_HSPERI_ETHER1_CLK_ETH_TX		0x104
#define REG_DIV_HSPERI_ETHER1_CLK_PTP_REF_I		0x108
#define REG_DIV_HSPERI_ETHER0_CLK_PTP_REF_I		0x10c
#define REG_DIV_HSPERI_CLK_SPI_NAND	0x110
#define REG_DIV_HSPERI_CLK_AUDSRC	0x114
#define REG_DIV_HSPERI_CLK_AUD0		0x118
#define REG_DIV_HSPERI_CLK_AUD1		0x11c
#define REG_DIV_HSPERI_CLK_AUD2		0x120
#define REG_DIV_HSPERI_CLK_AUD3		0x124
#define REG_DIV_HSPERI_CLK_AUD4		0x128
#define REG_DIV_HSPERI_CLK_AUD5		0x12c
#define REG_DIV_HSPERI_CLK_AUD_DW	0x130
#define REG_DIV_HSPERI_CLK_SPI		0x134
#define REG_DIV_HSPERI_CLK_I2C		0x138
#define REG_DIV_HSPERI_CLK_UART0	0x13c
#define REG_DIV_HSPERI_CLK_UART1	0x140
#define REG_DIV_HSPERI_CLK_UART2	0x144
#define REG_DIV_HSPERI_CLK_UART3	0x148
#define REG_DIV_HSPERI_CLK_UART4	0x14c
#define REG_DIV_HSPERI_CLK_UART5	0x150
#define REG_DIV_HSPERI_CLK_UART6	0x154
#define REG_DIV_HSPERI_CLK_UART7	0x158
#define REG_DIV_HSPERI_CLK_CAN		0x15c
#define REG_DIV_PERI_PWM_CLK		0x160
#define REG_DIV_PERI_CLK_XTAL_MISC	0x164

#define REG_CLK_EN_START		REG_CLK_EN_0
#define REG_CLK_SEL_START		REG_CLK_SEL_0
#define REG_CLK_BYP_START		REG_CLK_BYP_0
#define REG_CLK_DIV_START		REG_DIV_TOP_CLK_FAB0

#define REG_CLK_EN_NUM (REG_CLK_EN_10 / 4 - REG_CLK_EN_0 / 4 + 1)
#define REG_CLK_SEL_NUM (REG_CLK_SEL_0 / 4 - REG_CLK_SEL_0 / 4 + 1)
#define REG_CLK_BYP_NUM (REG_CLK_BYP_4 / 4 - REG_CLK_BYP_0 / 4 + 1)
#define REG_CLK_DIV_NUM (REG_DIV_PERI_CLK_XTAL_MISC / 4 - REG_DIV_TOP_CLK_FAB0 / 4 + 1)

#define CV186X_PLL_LOCK_TIMEOUT_MS	200

/* PLL status register offset */
#define PLL_STATUS_MASK			0xFF
#define PLL_STATUS_OFFSET		0x04

/* G2 Synthesizer register offset */
#define G2_SSC_CTRL_MASK		0xFF
#define G2_SSC_CTRL_OFFSET		0x40
#define SSC_SYN_SET_MASK		0x0F
#define SSC_SYN_SET_OFFSET		0x04

#define to_cv186x_pll_clk(_hw) container_of(_hw, struct cv186x_pll_hw_clock, hw)
#define to_cv186x_clk(_hw) container_of(_hw, struct cv186x_hw_clock, hw)

#define div_mask(width) ((1 << (width)) - 1)

static DEFINE_SPINLOCK(cv186x_clk_lock);

struct cv186x_clock_data {
	void __iomem *base;
	spinlock_t *lock;
#ifdef CONFIG_PM_SLEEP
	uint32_t clken_saved_regs[REG_CLK_EN_NUM];
	uint32_t clksel_saved_regs[REG_CLK_SEL_NUM];
	uint32_t clkbyp_saved_regs[REG_CLK_BYP_NUM];
	uint32_t clkdiv_saved_regs[REG_CLK_DIV_NUM];
	//uint32_t g2_clkdiv_saved_regs[REG_CLK_G2_DIV_NUM];
	//uint32_t pll_g2_csr_saved_regs[REG_PLL_G2_CSR_NUM];
	//uint32_t a0pll_ssc_syn_set_saved_reg;
	//uint32_t disppll_ssc_syn_set_saved_reg;
	//uint32_t cam0pll_ssc_syn_set_saved_reg;
	//uint32_t cam1pll_ssc_syn_set_saved_reg;
	//uint32_t pll_g6_csr_saved_regs[REG_PLL_G6_CSR_NUM];
#endif /* CONFIG_PM_SLEEP */
	struct clk_hw_onecell_data hw_data;
};

struct cv186x_gate {
	u32		reg;
	s8		shift;
	unsigned long	flags;
};

struct cv186x_div {
	u32		reg;
	s8		shift;
	s8		width;
	s16		initval;
	unsigned long	flags;
};

struct cv186x_mux {
	u32		reg;
	s8		shift;
	s8		width;
	unsigned long	flags;
};

struct cv186x_hw_clock {
	unsigned int id;
	const char *name;
	struct clk_hw hw;
	void __iomem *base;
	spinlock_t *lock;

	struct cv186x_gate gate;
	struct cv186x_div div[2]; /* 0: DIV_IN0, 1: DIV_IN1 */
	struct cv186x_mux mux[3]; /* 0: bypass, 1: CLK_SEL, 2: CLK_SRC(DIV_IN0_SRC_MUX) */
};

struct cv186x_pll_clock {
	unsigned int	id;
	const char	*name;
	u32		reg_csr;
	u32		reg_ssc;
	s16		post_div_sel; /* -1: postdiv*/
	unsigned long	flags;
};

struct cv186x_pll_hw_clock {
	struct cv186x_pll_clock pll;
	void __iomem *base;
	spinlock_t *lock;
	struct clk_hw hw;
};

static const struct clk_ops cv186x_g6_pll_ops;
static const struct clk_ops cv186x_g2_pll_ops;
static const struct clk_ops cv186x_g2d_pll_ops;
static const struct clk_ops cv186x_clk_ops;

static struct cv186x_clock_data *clk_data;

static unsigned long cvi_clk_flags;

#define CV186X_CLK(_id, _name, _parents, _gate_reg, _gate_shift,		\
			_div_0_reg, _div_0_initval, 			\
			_div_1_reg, _div_1_initval,			\
			_mux_0_reg, _mux_0_shift,			\
			_mux_1_reg, _mux_1_shift,			\
			_mux_2_reg, _flags) {		\
		.id = _id,						\
		.name = _name,						\
		.gate.reg = _gate_reg,					\
		.gate.shift = _gate_shift,				\
		.div[0].reg = _div_0_reg,				\
		.div[0].shift = 16,				\
		.div[0].width = 16,				\
		.div[0].initval = _div_0_initval,			\
		.div[1].reg = _div_1_reg,				\
		.div[1].shift = 16,				\
		.div[1].width = 16,				\
		.div[1].initval = _div_1_initval,			\
		.mux[0].reg = _mux_0_reg,				\
		.mux[0].shift = _mux_0_shift,				\
		.mux[0].width = 1,					\
		.mux[1].reg = _mux_1_reg,				\
		.mux[1].shift = _mux_1_shift,				\
		.mux[1].width = 1,					\
		.mux[2].reg = _mux_2_reg,				\
		.mux[2].shift = 8,				\
		.mux[2].width = 2,					\
		.hw.init = CLK_HW_INIT_PARENTS(				\
				_name, _parents,			\
				&cv186x_clk_ops,				\
				_flags | CV186X_CLK_FLAGS_ALL),		\
	}

#define CLK_G6_PLL(_id, _name, _parent, _reg_csr, _flags) {		\
		.pll.id = _id,						\
		.pll.name = _name,					\
		.pll.reg_csr = _reg_csr,				\
		.pll.reg_ssc = 0,					\
		.pll.post_div_sel = -1,					\
		.hw.init = CLK_HW_INIT_PARENTS(_name, _parent,		\
					       &cv186x_g6_pll_ops,	\
					       _flags |			\
                                               CLK_IS_CRITICAL |        \
					       CV186X_CLK_FLAGS_ALL),	\
	}

#define CLK_G2_PLL(_id, _name, _parent, _reg_csr, _reg_ssc, _flags) {	\
		.pll.id = _id,						\
		.pll.name = _name,					\
		.pll.reg_csr = _reg_csr,				\
		.pll.reg_ssc = _reg_ssc,				\
		.pll.post_div_sel = -1,					\
		.hw.init = CLK_HW_INIT_PARENTS(_name, _parent,		\
					       &cv186x_g2_pll_ops,	\
					       _flags |			\
                                               CLK_IS_CRITICAL |        \
					       CV186X_CLK_FLAGS_ALL),	\
	}

#define CLK_G2D_PLL(_id, _name, _parent, _reg_csr, _reg_ssc,		\
			_post_div_sel, _flags) {			\
		.pll.id = _id,						\
		.pll.name = _name,					\
		.pll.reg_csr = _reg_csr,				\
		.pll.reg_ssc = _reg_ssc,				\
		.pll.post_div_sel = _post_div_sel,			\
		.hw.init = CLK_HW_INIT_PARENTS(_name, _parent,		\
					       &cv186x_g2d_pll_ops,	\
					       _flags |			\
                                               CLK_IS_CRITICAL |        \
					       CV186X_CLK_FLAGS_ALL),	\
	}

#define CLK_INT_N_PLL(_id, _name, _parent, _reg_csr, _flags) {          \
                .pll.id = _id,                                          \
                .pll.name = _name,                                      \
                .pll.reg_csr = _reg_csr,                                \
                .pll.reg_ssc = 0,                                       \
                .pll.post_div_sel = -1,                                 \
                .hw.init = CLK_HW_INIT_PARENTS(_name, _parent,          \
                                               &cv186x_g6_pll_ops,  \
                                               _flags |                 \
                                               CLK_IS_CRITICAL |        \
                                               CV186X_CLK_FLAGS_ALL),  \
        }

#define CLK_FRAC_N_PLL(_id, _name, _parent, _reg_csr, _reg_ssc, _flags) {   \
                .pll.id = _id,                                          \
                .pll.name = _name,                                      \
                .pll.reg_csr = _reg_csr,                                \
                .pll.reg_ssc = _reg_ssc,                                \
                .pll.post_div_sel = -1,                                 \
                .hw.init = CLK_HW_INIT_PARENTS(_name, _parent,          \
                                               &cv186x_g2_pll_ops,     \
                                               _flags |                 \
                                               CLK_IS_CRITICAL |        \
                                               CV186X_CLK_FLAGS_ALL),  \
        }

const char *const cv186x_pll_parent[] = {"osc"};
const char *const cv186x_frac_pll_parent[] = {"clk_mipimpll2"};

/*
 * All PLL clocks are marked as CRITICAL, hence they are very crucial
 * for the functioning of the SoC
 */
static struct cv186x_pll_hw_clock cv186x_pll_clks[] = {
	CLK_G6_PLL(CV186X_CLK_FPLL, "clk_fpll", cv186x_pll_parent, REG_FPLL_CSR, 0),
	CLK_G6_PLL(CV186X_CLK_MPLL, "clk_mpll", cv186x_pll_parent, REG_MPLL_CSR, 0),
	CLK_G6_PLL(CV186X_CLK_TPLL, "clk_tpll", cv186x_pll_parent, REG_TPLL_CSR, 0),
	CLK_G2_PLL(CV186X_CLK_MIPIMPLL0, "clk_mipimpll0", cv186x_pll_parent,
			REG_MIPIMPLL0_CSR, 0, 0),
	CLK_G2_PLL(CV186X_CLK_CAM0PLL, "clk_cam0pll", cv186x_pll_parent,
			REG_CAM0PLL_CSR, 0, CLK_IGNORE_UNUSED),
	CLK_G2_PLL(CV186X_CLK_DISPPLL0, "clk_disppll0", ((const char *[]) {"clk_mipimpll0"}),
			REG_DISPPLL0_CSR, REG_DISPPLL0_SSC_SYN_CTRL, 0),
	CLK_G2_PLL(CV186X_CLK_MIPIMPLL1, "clk_mipimpll1", cv186x_pll_parent,
			REG_MIPIMPLL1_CSR, 0, 0),
	CLK_G2_PLL(CV186X_CLK_CAM1PLL, "clk_cam1pll", cv186x_pll_parent,
			REG_CAM1PLL_CSR, 0, 0),
	CLK_G2_PLL(CV186X_CLK_DISPPLL1, "clk_disppll1", ((const char *[]) {"clk_mipimpll1"}),
			REG_DISPPLL1_CSR, REG_DISPPLL1_SSC_SYN_CTRL, 0),
	CLK_G2_PLL(CV186X_CLK_MIPIMPLL2, "clk_mipimpll2", cv186x_pll_parent,
			REG_MIPIMPLL2_CSR, 0, 0),
	CLK_G2_PLL(CV186X_CLK_A0PLL, "clk_a0pll", ((const char *[]) {"clk_mipimpll2"}),
			REG_APLL0_CSR, REG_APLL0_SSC_SYN_CTRL, 0),
	CLK_INT_N_PLL(CV186X_CLK_A53PLL, "clk_a53pll", cv186x_pll_parent, REG_A53PLL_CSR, 0),
	CLK_INT_N_PLL(CV186X_CLK_RVPLL, "clk_rvpll", cv186x_pll_parent, REG_RVPLL_CSR, 0)
};

/*
 * Clocks marked as CRITICAL are needed for the proper functioning
 * of the SoC.
 */
static struct cv186x_hw_clock cv186x_clks[] = {
	CV186X_CLK(CV186X_TOP_CLK_FAB0, "clk_fab0",
			((const char *[]) { "osc", "clk_rvpll", "clk_disppll0", "clk_mipimpll0", "clk_fpll"}),
			REG_CLK_EN_0, 0,
			REG_DIV_TOP_CLK_FAB0, 3, 0xff, 0,
			REG_CLK_BYP_0, 0, 0, -1,
			REG_DIV_TOP_CLK_FAB0, CLK_IS_CRITICAL),
	CV186X_CLK(CV186X_TOP_CLK_FAB1, "clk_fab1",
			((const char *[]) { "osc", "clk_fpll", "clk_a53pll"}),
			REG_CLK_EN_0, 1,
			REG_DIV_TOP_CLK_FAB1, 10, 0xff, 0,
			REG_CLK_BYP_0, 1, 0, -1,
			REG_DIV_TOP_CLK_FAB1, CLK_IS_CRITICAL),
	CV186X_CLK(CV186X_TOP_CLK_HSPERI, "clk_axi4",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mpll", "clk_fpll"}),
			REG_CLK_EN_0, 2,
			REG_DIV_TOP_CLK_HSPERI, 6, 0xff, 0,
			REG_CLK_BYP_0, 2, 0, -1,
			REG_DIV_TOP_CLK_HSPERI, CLK_IS_CRITICAL),
	CV186X_CLK(CV186X_RTC_CLK_RTC_FAB, "clk_rtc_sys",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mpll", "clk_fpll"}),
			REG_CLK_EN_0, 3,
			REG_DIV_RTC_CLK_RTC_FAB, 6, 0xff, 0,
			REG_CLK_BYP_0, 3, 0, -1,
			REG_DIV_RTC_CLK_RTC_FAB, 0),
	CV186X_CLK(CV186X_TOP_CLK_1M, "top_clk_1m",
			((const char *[]) { "osc", "clk_xtal_free"}),
			REG_CLK_EN_0, 4,
			REG_DIV_TOP_CLK_1M, 250, 0xff, 0,
			REG_CLK_BYP_0, 4, 0, -1,
			REG_DIV_TOP_CLK_1M, 0),
	CV186X_CLK(CV186X_AP_CPU_CLK, "ap_cpu_clk",
			((const char *[]) { "osc", "clk_a53pll", "clk_cam1pll", "clk_mpll", "clk_rvpll", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 5,
			REG_DIV_AP_CPU_CLK_0, 1,
			REG_DIV_AP_CPU_CLK_1, 1,
			REG_CLK_BYP_0, 5,
			REG_CLK_SEL_0, 0,
			REG_DIV_AP_CPU_CLK_0, CLK_IS_CRITICAL),
	CV186X_CLK(CV186X_AP_CLK_RV0, "ap_clk_rv0",
			((const char *[]) { "osc", "clk_rvpll", "clk_tpll", "clk_mpll", "clk_a53pll", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 6,
			REG_DIV_AP_CLK_RV0_0, 1,
			REG_DIV_AP_CLK_RV0_1, 1,
			REG_CLK_BYP_0, 6,
			REG_CLK_SEL_0, 1,
			REG_DIV_AP_CLK_RV0_0, 0),
	CV186X_CLK(CV186X_AP_DBG_CLK, "ap_dbg_clk",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_0, 7,
			REG_DIV_AP_DBG_CLK, 10, 0xff, 0,
			REG_CLK_BYP_0, 7, 0, -1,
			REG_DIV_AP_DBG_CLK, 0),
	CV186X_CLK(CV186X_AP_SC_CLK, "ap_sc_clk",
			((const char *[]) { "osc"}),
			REG_CLK_EN_0, 8,
			0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_AP_BUS_CLK, "ap_bus_clk",
			((const char *[]) { "osc", "clk_a53pll", "clk_cam1pll", "clk_mpll", "clk_fpll"}),
			REG_CLK_EN_0, 9,
			REG_DIV_AP_BUS_CLK, 2, 0xff, 0,
			REG_CLK_BYP_0, 9, 0, -1,
			REG_DIV_AP_BUS_CLK, 0),
	CV186X_CLK(CV186X_AP_CLK_SECURITY, "clk_security",
			((const char *[]) { "osc"}),
			REG_CLK_EN_0, 10,
			0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, CLK_IS_CRITICAL),
	CV186X_CLK(CV186X_AP_CLK_HSPERI_FAB, "ap_clk_hsperi_fab",
			((const char *[]) { "osc", "clk_axi4"}),
			REG_CLK_EN_0, 11,
			0xff, 0, 0xff, 0, REG_CLK_BYP_0, 11, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_AP_CLK_PERI_FAB, "ap_clk_peri_fab",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_0, 12,
			0xff, 0, 0xff, 0, REG_CLK_BYP_0, 12, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_AP_CA53_PM_CLKIN, "ap_ca53_pm_clkin",
			((const char *[]) { "osc"}),
			REG_CLK_EN_0, 13, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_AP_CLK_RTC_FAB, "ap_clk_rtc_fab",
			((const char *[]) { "osc", "clk_rtc_sys"}),
			REG_CLK_EN_0, 14, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_0, 14, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_AP_CLK_TOP_FAB0, "ap_clk_top_fab0",
			((const char *[]) { "osc", "clk_fab0"}),
			REG_CLK_EN_0, 15, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_0, 15, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_TPU_CLK_TPU, "tpu_clk_tpu",
			((const char *[]) { "osc", "clk_tpll", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 16,
			REG_DIV_TPU_CLK_TPU, 1, 0xff, 0,
			REG_CLK_BYP_0, 16, 0, -1,
			REG_DIV_TPU_CLK_TPU, 0),
	CV186X_CLK(CV186X_TPU_CLK_TOP_FAB0, "tpu_clk_top_fab0",
			((const char *[]) { "osc", "clk_fab0"}),
			REG_CLK_EN_0, 17, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_0, 17, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_TPU_CLK_TC906B, "tpu_clk_tc906b",
			((const char *[]) { "osc", "clk_rvpll", "clk_tpll", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 18,
			REG_DIV_TPU_CLK_TC906B, 1, 0xff, 0,
			REG_CLK_BYP_0, 18, 0, -1,
			REG_DIV_TPU_CLK_TC906B, 0),
	CV186X_CLK(CV186X_TPU_CLK_TIMER_50M, "tpu_clk_timer_50m",
			((const char *[]) { "osc", "clk_fpll", "clk_tpll"}),
			REG_CLK_EN_0, 19,
			REG_DIV_TPU_CLK_TIMER_50M, 20, 0xff, 0,
			REG_CLK_BYP_0, 19, 0, -1,
			REG_DIV_TPU_CLK_TIMER_50M, 0),
	CV186X_CLK(CV186X_TPU_CLK_GDMA, "tpu_clk_gdma",
			((const char *[]) { "osc", "clk_mpll", "clk_tpll"}),
			REG_CLK_EN_0, 20,
			REG_DIV_TPU_CLK_GDMA, 2, 0xff, 0,
			REG_CLK_BYP_0, 20, 0, -1,
			REG_DIV_TPU_CLK_GDMA, 0),
	CV186X_CLK(CV186X_VD0_CLK_SRC1_VDSYS, "vd0_clk_src1_vdsys",
			((const char *[]) { "osc", "clk_cam1pll", "clk_mpll", "clk_rvpll", "clk_a53pll", "clk_tpll", "clk_cam0pll", "clk_disppll0"}),
			REG_CLK_EN_0, 21,
			REG_DIV_VD0_CLK_SRC1_VDSYS_0, 2,
			REG_DIV_VD0_CLK_SRC1_VDSYS_1, 1,
			REG_CLK_BYP_0, 21,
			REG_CLK_SEL_0, 2,
			REG_DIV_VD0_CLK_SRC1_VDSYS_0, 0),
	CV186X_CLK(CV186X_VD0_CLK_SRC0_VDSYS, "vd0_clk_src0_vdsys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 22,
			REG_DIV_VD0_CLK_SRC0_VDSYS, 2, 0xff, 0,
			REG_CLK_BYP_0, 22, 0, -1,
			REG_DIV_VD0_CLK_SRC0_VDSYS, 0),
	CV186X_CLK(CV186X_VD0_CLK_SRC2_VDSYS, "vd0_clk_src2_vdsys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 23,
			REG_DIV_VD0_CLK_SRC2_VDSYS, 2, 0xff, 0,
			REG_CLK_BYP_0, 23, 0, -1,
			REG_DIV_VD0_CLK_SRC2_VDSYS, 0),
	CV186X_CLK(CV186X_VD0_CLK_AXI_VDSYS_0, "vd0_clk_axi_vdsys_0",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 24,
			REG_DIV_VD0_CLK_AXI_VDSYS_0, 2, 0xff, 0,
			REG_CLK_BYP_0, 24, 0, -1,
			REG_DIV_VD0_CLK_AXI_VDSYS_0, 0),
	CV186X_CLK(CV186X_VD0_CLK_AXI_VDSYS_1, "vd0_clk_axi_vdsys_1",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 25,
			REG_DIV_VD0_CLK_AXI_VDSYS_1, 2, 0xff, 0,
			REG_CLK_BYP_0, 25, 0, -1,
			REG_DIV_VD0_CLK_AXI_VDSYS_1, 0),
	CV186X_CLK(CV186X_VD0_CLK_TOP_AXI, "vd0_clk_top_axi",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_0, 26, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_0, 26, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_SRC1_VDSYS, "vd1_clk_src1_vdsys",
			((const char *[]) { "osc", "clk_cam1pll", "clk_mpll", "clk_rvpll", "clk_a53pll", "clk_tpll", "clk_cam0pll", "clk_disppll0"}),
			REG_CLK_EN_0, 27,
			REG_DIV_VD1_CLK_SRC1_VDSYS_0, 2,
			REG_DIV_VD1_CLK_SRC1_VDSYS_1, 0,
			REG_CLK_BYP_0, 27,
			REG_CLK_SEL_0, 3,
			REG_DIV_VD1_CLK_SRC1_VDSYS_0, 0),
	CV186X_CLK(CV186X_VD1_CLK_SRC0_VDSYS, "vd1_clk_src0_vdsys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 28,
			REG_DIV_VD1_CLK_SRC0_VDSYS, 2, 0xff, 0,
			REG_CLK_BYP_0, 28, 0, -1,
			REG_DIV_VD1_CLK_SRC0_VDSYS, 0),
	CV186X_CLK(CV186X_VD1_CLK_SRC2_VDSYS, "vd1_clk_src2_vdsys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 29,
			REG_DIV_VD1_CLK_SRC2_VDSYS, 2, 0xff, 0,
			REG_CLK_BYP_0, 29, 0, -1,
			REG_DIV_VD1_CLK_SRC2_VDSYS, 0),
	CV186X_CLK(CV186X_VD1_CLK_AXI_VDSYS_0, "vd1_clk_axi_vdsys_0",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 30,
			REG_DIV_VD1_CLK_AXI_VDSYS_0, 2, 0xff, 0,
			REG_CLK_BYP_0, 30, 0, -1,
			 REG_DIV_VD1_CLK_AXI_VDSYS_0, 0),
	CV186X_CLK(CV186X_VD1_CLK_AXI_VDSYS_1, "vd1_clk_axi_vdsys_1",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_0, 31,
			REG_DIV_VD1_CLK_AXI_VDSYS_1, 2, 0xff, 0,
			REG_CLK_BYP_0, 31, 0, -1,
			REG_DIV_VD1_CLK_AXI_VDSYS_1, 0),
	CV186X_CLK(CV186X_VD1_CLK_TOP_AXI, "vd1_clk_top_axi",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_1, 0, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_1, 0, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VE_CLK_SRC1_VESYS, "ve_clk_src1_vesys",
			((const char *[]) { "osc", "clk_cam1pll", "clk_mpll", "clk_rvpll", "clk_a53pll", "clk_tpll", "clk_cam0pll", "clk_disppll0"}),
			REG_CLK_EN_1, 1,
			REG_DIV_VE_CLK_SRC1_VESYS_0, 2,
			REG_DIV_VE_CLK_SRC1_VESYS_1, 0,
			REG_CLK_BYP_1, 1,
			REG_CLK_SEL_0, 4,
			REG_DIV_VE_CLK_SRC1_VESYS_0, 0),
	CV186X_CLK(CV186X_VE_CLK_SRC0_VESYS, "ve_clk_src0_vesys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_1, 2,
			REG_DIV_VE_CLK_SRC0_VESYS, 2, 0xff, 0,
			REG_CLK_BYP_1, 2, 0, -1,
			REG_DIV_VE_CLK_SRC0_VESYS, 0),
	CV186X_CLK(CV186X_VE_CLK_SRC2_VESYS, "ve_clk_src2_vesys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_1, 3,
			REG_DIV_VE_CLK_SRC2_VESYS, 2, 0xff, 0,
			REG_CLK_BYP_1, 3, 0, -1,
			REG_DIV_VE_CLK_SRC2_VESYS, 0),
	CV186X_CLK(CV186X_VE_CLK_AXI_VESYS, "ve_clk_axi_vesys",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_1, 4,
			REG_DIV_VE_CLK_AXI_VESYS, 2, 0xff, 0,
			REG_CLK_BYP_1, 4, 0, -1,
			REG_DIV_VE_CLK_AXI_VESYS, 0),
	CV186X_CLK(CV186X_VE_CLK_TOP_AXI, "ve_clk_top_axi",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_1, 5, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_1, 5, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VI_CLK_X2P, "vi_clk_x2p",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_1, 6, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_1, 6, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VI_CLK_RAW_AXI, "vi_clk_raw_axi",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0"}),
			REG_CLK_EN_1, 7,
			REG_DIV_VI_CLK_RAW_AXI, 2, 0xff, 0,
			REG_CLK_BYP_1, 7, 0, -1,
			REG_DIV_VI_CLK_RAW_AXI, 0),
	CV186X_CLK(CV186X_VI_CAM0_CLK, "vi_cam0_clk",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0", "clk_disppll0", "clk_fpll"}),
			REG_CLK_EN_1, 8,
			REG_DIV_VI_CAM0_CLK, 26, 0xff, 0,
			REG_CLK_BYP_1, 8, 0, -1,
			REG_DIV_VI_CAM0_CLK, 0),
	CV186X_CLK(CV186X_VI_CAM1_CLK, "vi_cam1_clk",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0", "clk_disppll0", "clk_fpll"}),
			REG_CLK_EN_1, 9,
			REG_DIV_VI_CAM1_CLK, 26, 0xff, 0,
			REG_CLK_BYP_1, 9, 0, -1,
			REG_DIV_VI_CAM1_CLK, 0),
	CV186X_CLK(CV186X_VI_CAM2_CLK, "vi_cam2_clk",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll0", "clk_disppll0", "clk_fpll"}),
			REG_CLK_EN_1, 10,
			REG_DIV_VI_CAM2_CLK, 26, 0xff, 0,
			REG_CLK_BYP_1, 10, 0, -1,
			REG_DIV_VI_CAM2_CLK, 0),
	CV186X_CLK(CV186X_VI_CLK_SRC_VI_SYS_0, "vi_clk_src_vi_sys_0",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mipimpll2", "clk_mipimpll1"}),
			REG_CLK_EN_1, 11,
			REG_DIV_VI_CLK_SRC_VI_SYS_0, 12, 0xff, 0,
			REG_CLK_BYP_1, 11, 0, -1,
			REG_DIV_VI_CLK_SRC_VI_SYS_0, 0),
	CV186X_CLK(CV186X_VI_CLK_SRC_VI_SYS_1, "vi_clk_src_vi_sys_1",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mipimpll2", "clk_mipimpll1"}),
			REG_CLK_EN_1, 12,
			REG_DIV_VI_CLK_SRC_VI_SYS_1, 6, 0xff, 0,
			REG_CLK_BYP_1, 12, 0, -1,
			REG_DIV_VI_CLK_SRC_VI_SYS_1, 0),
	CV186X_CLK(CV186X_VI_CLK_SRC_VI_SYS_2, "vi_clk_src_vi_sys_2",
			((const char *[]) { "osc", "clk_rvpll", "clk_mipimpll2", "clk_mipimpll1"}),
			REG_CLK_EN_1, 13,
			REG_DIV_VI_CLK_SRC_VI_SYS_2, 2, 0xff, 0,
			REG_CLK_BYP_1, 13, 0, -1,
			REG_DIV_VI_CLK_SRC_VI_SYS_2, 0),
	CV186X_CLK(CV186X_VI_CLK_SRC_VI_SYS_3, "vi_clk_src_vi_sys_3",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll2", "clk_mipimpll1"}),
			REG_CLK_EN_1, 14,
			REG_DIV_VI_CLK_SRC_VI_SYS_3, 2, 0xff, 0,
			REG_CLK_BYP_1, 14, 0, -1,
			REG_DIV_VI_CLK_SRC_VI_SYS_3, 0),
	CV186X_CLK(CV186X_VI_CLK_SRC_VI_SYS_4, "vi_clk_src_vi_sys_4",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mipimpll2", "clk_mipimpll1"}),
			REG_CLK_EN_1, 15,
			REG_DIV_VI_CLK_SRC_VI_SYS_4, 2, 0xff, 0,
			REG_CLK_BYP_1, 15, 0, -1,
			REG_DIV_VI_CLK_SRC_VI_SYS_4, 0),
	CV186X_CLK(CV186X_VI_CLK_SRC_VI_SYS_5, "vi_clk_src_vi_sys_5",
			((const char *[]) { "osc", "clk_cam1pll", "clk_mipimpll2", "clk_mipimpll1", "clk_fpll"}),
			REG_CLK_EN_1, 16,
			REG_DIV_VI_CLK_SRC_VI_SYS_5, 2, 0xff, 0,
			REG_CLK_BYP_1, 16, 0, -1,
			REG_DIV_VI_CLK_SRC_VI_SYS_5, 0),
	CV186X_CLK(CV186X_VO_CLK_RAW_AXI, "vo_clk_raw_axi",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll2", "clk_mipimpll1"}),
			REG_CLK_EN_1, 17,
			REG_DIV_VO_CLK_RAW_AXI, 2, 0xff, 0,
			REG_CLK_BYP_1, 17, 0, -1,
			REG_DIV_VO_CLK_RAW_AXI, 0),
	CV186X_CLK(CV186X_VO_CLK_X2P, "vo_clk_x2p",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_1, 18, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_1, 18, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_SRC_VO_SYS_0, "clk_src_vo_sys_0",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mipimpll1", "clk_mipimpll2"}),
			REG_CLK_EN_1, 19,
			REG_DIV_VO_CLK_SRC_VO_SYS_0, 6, 0xff, 0,
			REG_CLK_BYP_1, 19, 0, -1,
			REG_DIV_VO_CLK_SRC_VO_SYS_0, 0),
	CV186X_CLK(CV186X_VO_CLK_SRC_VO_SYS_1, "clk_src_vo_sys_1",
			((const char *[]) { "osc", "clk_cam0pll", "clk_mipimpll1", "clk_mipimpll2"}),
			REG_CLK_EN_1, 20,
			REG_DIV_VO_CLK_SRC_VO_SYS_1, 2, 0xff, 0,
			REG_CLK_BYP_1, 20, 0, -1,
			REG_DIV_VO_CLK_SRC_VO_SYS_1, 0),
	CV186X_CLK(CV186X_VO_CLK_SRC_VO_SYS_2, "clk_src_vo_sys_2",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_fpll", "clk_mipimpll2"}),
			REG_CLK_EN_1, 21,
			REG_DIV_VO_CLK_SRC_VO_SYS_2, 6, 0xff, 0,
			REG_CLK_BYP_1, 21, 0, -1,
			REG_DIV_VO_CLK_SRC_VO_SYS_2, 0),
	CV186X_CLK(CV186X_VO_CLK_SYS_DISP0, "vo_clk_sys_disp0",
			((const char *[]) { "osc", "clk_disppll0", "clk_mipimpll1", "clk_fpll"}),
			REG_CLK_EN_1, 22,
			REG_DIV_VO_CLK_SYS_DISP0, 1, 0xff, 0,
			REG_CLK_BYP_1, 22, 0, -1,
			REG_DIV_VO_CLK_SYS_DISP0, 0),
	CV186X_CLK(CV186X_VO_CLK_SYS_DISP1, "vo_clk_sys_disp1",
			((const char *[]) { "osc", "clk_disppll1", "clk_mipimpll1", "clk_fpll"}),
			REG_CLK_EN_1, 23,
			REG_DIV_VO_CLK_SYS_DISP1, 1, 0xff, 0,
			REG_CLK_BYP_1, 23, 0, -1,
			REG_DIV_VO_CLK_SYS_DISP1, 0),
	CV186X_CLK(CV186X_VO_CLK_RAW_DSI_TX_ESC0, "vo_clk_raw_dsi_tx_esc0",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_1, 24,
			REG_DIV_VO_CLK_RAW_DSI_TX_ESC0, 51, 0xff, 0,
			REG_CLK_BYP_1, 24, 0, -1,
			REG_DIV_VO_CLK_RAW_DSI_TX_ESC0, 0),
	CV186X_CLK(CV186X_VO_CLK_RAW_DSI_TX_ESC1, "vo_clk_raw_dsi_tx_esc1",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_1, 25,
			REG_DIV_VO_CLK_RAW_DSI_TX_ESC1, 51, 0xff, 0,
			REG_CLK_BYP_1, 25, 0, -1,
			REG_DIV_VO_CLK_RAW_DSI_TX_ESC1, 0),
	CV186X_CLK(CV186X_VO_CLK_SFR, "vo_clk_sfr",
			((const char *[]) { "osc", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_1, 26,
			REG_DIV_VO_CLK_SFR, 40, 0xff, 0,
			REG_CLK_BYP_1, 26, 0, -1,
			REG_DIV_VO_CLK_SFR, 0),
	CV186X_CLK(CV186X_VO_CLK_MIPIMPLL0, "vo_clk_mipimpll0",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_mipimpll1", "clk_mipimpll2"}),
			REG_CLK_EN_1, 27,
			REG_DIV_VO_CLK_MIPIMPLL0, 2, 0xff, 0,
			REG_CLK_BYP_1, 27, 0, -1,
			REG_DIV_VO_CLK_MIPIMPLL0, 0),
	CV186X_CLK(CV186X_VO_CLK_MIPIMPLL1, "vo_clk_mipimpll1",
			((const char *[]) { "osc", "vo_clk_mipimpll0"}),
			REG_CLK_EN_1, 28, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_1, 28, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_ACLK_M2_SYS1, "ddr_aclk_m2_sys1",
			((const char *[]) { "osc", "clk_fpll", "clk_mipimpll1", "clk_mpll"}),
			REG_CLK_EN_1, 29,
			REG_DIV_DDR_ACLK_M2_SYS1, 1, 0xff, 0,
			REG_CLK_BYP_1, 29, 0, -1,
			REG_DIV_DDR_ACLK_M2_SYS1, 0),
	CV186X_CLK(CV186X_DDR_CLKI_DDRPLL_MAS_REF_SYS1, "ddr_clki_ddrpll_mas_ref_sys1",
			((const char *[]) { "osc"}),
			REG_CLK_EN_1, 30, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_CLKI_DDRPLL_MAS_REF_SYS2, "ddr_clki_ddrpll_mas_ref_sys2",
			((const char *[]) { "osc"}),
			REG_CLK_EN_1, 31, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_1, 31, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_ACLK_M1_SYS1, "ddr_aclk_m1_sys1",
			((const char *[]) { "osc", "vi_clk_raw_axi"}),
			REG_CLK_EN_2, 0, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 0, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_ACLK_M1_SYS2, "ddr_aclk_m1_sys2",
			((const char *[]) { "osc", "vi_clk_raw_axi"}),
			REG_CLK_EN_2, 1, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_ACLK_M4_SYS1, "ddr_aclk_m4_sys1",
			((const char *[]) { "osc", "tpu_clk_tpu"}),
			REG_CLK_EN_2, 2, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 2, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_ACLK_M4_SYS2, "ddr_aclk_m4_sys2",
			((const char *[]) { "osc", "tpu_clk_tpu"}),
			REG_CLK_EN_2, 3, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 3, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_PCLK_CTRL_SYS1, "ddr_pclk_ctrl_sys1",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_2, 4, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 4, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_PCLK_CTRL_SYS2, "ddr_pclk_ctrl_sys2",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_2, 5, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 5, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE, "ddr_top_fab_aclk_offline",
			((const char *[]) { "osc", "ddr_aclk_m2_sys1"}),
			REG_CLK_EN_2, 6, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 6, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_RLT, "ddr_top_fab_aclk_rlt",
			((const char *[]) { "osc", "vi_clk_raw_axi"}),
			REG_CLK_EN_2, 7, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 7, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_TPU, "ddr_top_fab_aclk_tpu",
			((const char *[]) { "osc", "tpu_clk_tpu"}),
			REG_CLK_EN_2, 8, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 8, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M1, "ddr_top_fab_aclk_offline_m1",
			((const char *[]) { "osc", "vi_clk_raw_axi"}),
			REG_CLK_EN_2, 9, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 9, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M2, "ddr_top_fab_aclk_offline_m2",
			((const char *[]) { "osc", "vi_clk_raw_axi"}),
			REG_CLK_EN_2, 10, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 10, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M3, "ddr_top_fab_aclk_offline_m3",
			((const char *[]) { "osc", "vi_clk_raw_axi"}),
			REG_CLK_EN_2, 11, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 11, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M4, "ddr_top_fab_aclk_offline_m4",
			((const char *[]) { "osc", "vd0_clk_axi_vdsys_0"}),
			REG_CLK_EN_2, 12, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 12, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M5, "ddr_top_fab_aclk_offline_m5",
			((const char *[]) { "osc", "vd0_clk_axi_vdsys_1"}),
			REG_CLK_EN_2, 13, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 13, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M6, "ddr_top_fab_aclk_offline_m6",
			((const char *[]) { "osc", "vd1_clk_axi_vdsys_0"}),
			REG_CLK_EN_2, 14, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 14, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M7, "ddr_top_fab_aclk_offline_m7",
			((const char *[]) { "osc", "vd1_clk_axi_vdsys_1"}),
			REG_CLK_EN_2, 15, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 15, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M8, "ddr_top_fab_aclk_offline_m8",
			((const char *[]) { "osc", "ve_clk_axi_vesys"}),
			REG_CLK_EN_2, 16, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 16, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M9, "ddr_top_fab_aclk_offline_m9",
			((const char *[]) { "osc", "clk_axi4"}),
			REG_CLK_EN_2, 17, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 17, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M10, "ddr_top_fab_aclk_offline_m10",
			((const char *[]) { "osc", "usb_clk_usb_fab"}),
			REG_CLK_EN_2, 18, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 18, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M11, "ddr_top_fab_aclk_offline_m11",
			((const char *[]) { "osc", "ssperi_aclk_ddr"}),
			REG_CLK_EN_2, 19, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_2, 19, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_ACLK_OFFLINE_M12, "ddr_top_fab_aclk_offline_m12",
			((const char *[]) { "osc", "ap_bus_clk"}),
			REG_CLK_EN_2, 20, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_2, 20, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_PCLK_FW, "ddr_top_fab_pclk_fw",
			((const char *[]) { "osc", "clk_security"}),
			REG_CLK_EN_2, 21, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_2, 21, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_DDR_TOP_FAB_PCLK_TOP, "ddr_top_fab_pclk_top",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_2, 22, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_2, 22, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_USB_CLK_TOP_USB_FAB, "usb_clk_top_usb_fab",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_2, 23, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_2, 23, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_USB_TEST_PHY_CLK_480, "usb_test_phy_clk_480",
			((const char *[]) { "osc", "clk_rvpll", "clk_disppll0", "clk_mipimpll0", "clk_mpll"}),
			REG_CLK_EN_2, 24,
			REG_DIV_USB_TEST_PHY_CLK_480, 3, 0xff, 0,
			REG_CLK_BYP_2, 24, 0, -1,
			REG_DIV_USB_TEST_PHY_CLK_480, 0),
	CV186X_CLK(CV186X_USB_TEST_PHY_CLK_500, "usb_test_phy_clk_500",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_2, 25,
			REG_DIV_USB_TEST_PHY_CLK_500, 2, 0xff, 0,
			REG_CLK_BYP_2, 25, 0, -1,
			REG_DIV_USB_TEST_PHY_CLK_500, 0),
	CV186X_CLK(CV186X_USB_CLK_USB_FAB, "usb_clk_usb_fab",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_2, 26,
			REG_DIV_USB_CLK_USB_FAB, 5, 0xff, 0,
			REG_CLK_BYP_2, 26, 0, -1,
			REG_DIV_USB_CLK_USB_FAB, 0),
	CV186X_CLK(CV186X_USB_CLK_USB_SUSPEND, "usb_clk_usb_suspend",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_2, 27,
			REG_DIV_USB_CLK_USB_SUSPEND, 5000, 0xff, 0,
			REG_CLK_BYP_2, 27, 0, -1,
			REG_DIV_USB_CLK_USB_SUSPEND, 0),
	CV186X_CLK(CV186X_SSPERI_ACLK_DDR, "ssperi_aclk_ddr",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_fpll", "clk_rvpll", "clk_mpll"}),
			REG_CLK_EN_2, 28,
			REG_DIV_SSPERI_ACLK_DDR, 6, 0xff, 0,
			REG_CLK_BYP_2, 28, 0, -1,
			REG_DIV_SSPERI_ACLK_DDR, 0),
	CV186X_CLK(CV186X_SSPERI_ACLK_CFG2TOP, "ssperi_aclk_cfg2top",
			((const char *[]) { "osc", "clk_fab0"}),
			REG_CLK_EN_2, 29, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_2, 29, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_SSPERI_ACLK_CFG, "ssperi_aclk_cfg",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_2, 30, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_2, 30, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_SSPERI_TEST_PHY_CLK, "ssperi_test_phy_clk",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_2, 31,
			REG_DIV_SSPERI_TEST_PHY_CLK, 1, 0xff, 0,
			REG_CLK_BYP_2, 31, 0, -1,
			REG_DIV_SSPERI_TEST_PHY_CLK, 0),
	CV186X_CLK(CV186X_CLK_RXOOB, "clk_rxoob",
			((const char *[]) { "osc", "clk_mipimpll0", "clk_fpll", "clk_rvpll", "clk_mpll"}),
			REG_CLK_EN_3, 0,
			REG_DIV_SSPERI_CLK_RXOOB, 36, 0xff, 0,
			REG_CLK_BYP_3, 0, 0, -1,
			REG_DIV_SSPERI_CLK_RXOOB, 0),
	CV186X_CLK(CV186X_CLK_SD0, "clk_sd0",
			((const char *[]) { "osc", "clk_cam1pll", "clk_rvpll", "clk_disppll0", "clk_mpll"}),
			REG_CLK_EN_3, 1,
			REG_DIV_HSPERI_CLK_SD0, 3, 0xff, 0,
			REG_CLK_BYP_3, 1, 0, -1,
			REG_DIV_HSPERI_CLK_SD0, 0),
	CV186X_CLK(CV186X_CLK_100K_SD0, "clk_100k_sd0",
			((const char *[]) { "osc", "clk_100k"}),
			REG_CLK_EN_3, 2, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_3, 2, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SD1, "clk_sd1",
			((const char *[]) { "osc", "clk_cam1pll", "clk_rvpll", "clk_disppll0", "clk_mpll"}),
			REG_CLK_EN_3, 3,
			REG_DIV_HSPERI_CLK_SD1, 3, 0xff, 0,
			REG_CLK_BYP_3, 3, 0, -1,
			REG_DIV_HSPERI_CLK_SD1, 0),
	CV186X_CLK(CV186X_CLK_100K_SD1, "clk_100k_sd1",
			((const char *[]) { "osc", "clk_100k"}),
			REG_CLK_EN_3, 4, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_3, 4, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SD2, "clk_sd2",
			((const char *[]) { "osc", "clk_cam1pll", "clk_rvpll", "clk_disppll0", "clk_mpll"}),
			REG_CLK_EN_3, 5,
			REG_DIV_HSPERI_CLK_SD2, 3, 0xff, 0,
			REG_CLK_BYP_3, 5, 0, -1,
			REG_DIV_HSPERI_CLK_SD2, 0),
	CV186X_CLK(CV186X_CLK_100K_SD2, "clk_100k_sd2",
			((const char *[]) { "osc", "clk_100k"}),
			REG_CLK_EN_3, 6, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_3, 6, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_EMMC, "clk_emmc",
			((const char *[]) { "osc", "clk_cam1pll", "clk_rvpll", "clk_disppll0", "clk_mpll"}),
			REG_CLK_EN_3, 7,
			REG_DIV_HSPERI_EMMC_CARD_CLK, 3, 0xff, 0,
			REG_CLK_BYP_3, 7, 0, -1,
			REG_DIV_HSPERI_EMMC_CARD_CLK, 0),
	CV186X_CLK(CV186X_CLK_100K_EMMC, "clk_100k_emmc",
			((const char *[]) { "osc", "clk_100k"}),
			REG_CLK_EN_3, 8, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_3, 8, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_ETH0_TX, "clk_eth0_tx",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 9,
			REG_DIV_HSPERI_ETHER0_CLK_ETH_TX, 8, 0xff, 0,
			REG_CLK_BYP_3, 9, 0, -1,
			REG_DIV_HSPERI_ETHER0_CLK_ETH_TX, 0),
	CV186X_CLK(CV186X_CLK_ETH1_TX, "clk_eth1_tx",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 10,
			REG_DIV_HSPERI_ETHER1_CLK_ETH_TX, 8, 0xff, 0,
			REG_CLK_BYP_3, 10, 0, -1,
			REG_DIV_HSPERI_ETHER1_CLK_ETH_TX, 0),
	CV186X_CLK(CV186X_CLK_ETH1_REF, "clk_eth1_ref",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 11,
			REG_DIV_HSPERI_ETHER1_CLK_PTP_REF_I, 20, 0xff, 0,
			REG_CLK_BYP_3, 11, 0, -1,
			REG_DIV_HSPERI_ETHER1_CLK_PTP_REF_I, 0),
	CV186X_CLK(CV186X_CLK_ETH0_REF, "clk_eth0_ref",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 12,
			REG_DIV_HSPERI_ETHER0_CLK_PTP_REF_I, 20, 0xff, 0,
			REG_CLK_BYP_3, 12, 0, -1,
			REG_DIV_HSPERI_ETHER0_CLK_PTP_REF_I, 0),
	CV186X_CLK(CV186X_CLK_AXI4_ACLK, "clk_axi4_aclk",
			((const char *[]) { "osc", "clk_axi4"}),
			REG_CLK_EN_3, 13, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_3, 13, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SPI_NAND, "clk_spi_nand",
			((const char *[]) { "osc", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_3, 14,
			REG_DIV_HSPERI_CLK_SPI_NAND, 5, 0xff, 0,
			REG_CLK_BYP_3, 14, 0, -1,
			REG_DIV_HSPERI_CLK_SPI_NAND, 0),
	CV186X_CLK(CV186X_CLK_AUDSRC, "clk_audsrc",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 15,
			REG_DIV_HSPERI_CLK_AUDSRC, 17, 0xff, 0,
			REG_CLK_BYP_3, 15, 0, -1,
			REG_DIV_HSPERI_CLK_AUDSRC, 0),
	CV186X_CLK(CV186X_CLK_AUD0, "clk_aud0",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 16,
			REG_DIV_HSPERI_CLK_AUD0, 17, 0xff, 0,
			REG_CLK_BYP_3, 16, 0, -1,
			REG_DIV_HSPERI_CLK_AUD0, 0),
	CV186X_CLK(CV186X_CLK_AUD1, "clk_aud1",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 17,
			REG_DIV_HSPERI_CLK_AUD1, 17, 0xff, 0,
			REG_CLK_BYP_3, 17, 0, -1,
			REG_DIV_HSPERI_CLK_AUD1, 0),
	CV186X_CLK(CV186X_CLK_AUD2, "clk_aud2",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 18,
			REG_DIV_HSPERI_CLK_AUD2, 17, 0xff, 0,
			REG_CLK_BYP_3, 18, 0, -1,
			REG_DIV_HSPERI_CLK_AUD2, 0),
	CV186X_CLK(CV186X_CLK_AUD3, "clk_aud3",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 19,
			REG_DIV_HSPERI_CLK_AUD3, 17, 0xff, 0,
			REG_CLK_BYP_3, 19, 0, -1,
			REG_DIV_HSPERI_CLK_AUD3, 0),
	CV186X_CLK(CV186X_CLK_AUD4, "clk_aud4",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 20,
			REG_DIV_HSPERI_CLK_AUD4, 17, 0xff, 0,
			REG_CLK_BYP_3, 20, 0, -1,
			REG_DIV_HSPERI_CLK_AUD4, 0),
	CV186X_CLK(CV186X_CLK_AUD5, "clk_aud5",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 21,
			REG_DIV_HSPERI_CLK_AUD5, 17, 0xff, 0,
			REG_CLK_BYP_3, 21, 0, -1,
			REG_DIV_HSPERI_CLK_AUD5, 0),
	CV186X_CLK(CV186X_CLK_AUD_DW, "clk_aud_dw",
			((const char *[]) { "osc", "clk_a0pll", "clk_a24kpll"}),
			REG_CLK_EN_3, 22,
			REG_DIV_HSPERI_CLK_AUD_DW, 17, 0xff, 0,
			REG_CLK_BYP_3, 22, 0, -1,
			REG_DIV_HSPERI_CLK_AUD_DW, 0),
	CV186X_CLK(CV186X_CLK_SPI, "clk_spi",
			((const char *[]) { "osc", "clk_fpll", "clk_mipimpll0"}),
			REG_CLK_EN_3, 23,
			REG_DIV_HSPERI_CLK_SPI, 5, 0xff, 0,
			REG_CLK_BYP_3, 23, 0, -1,
			REG_DIV_HSPERI_CLK_SPI, 0),
	CV186X_CLK(CV186X_CLK_I2C, "clk_i2c",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 24,
			REG_DIV_HSPERI_CLK_I2C, 10, 0xff, 0,
			REG_CLK_BYP_3, 24, 0, -1,
			REG_DIV_HSPERI_CLK_I2C, 0),
	CV186X_CLK(CV186X_CLK_UART0, "clk_uart0",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 25,
			REG_DIV_HSPERI_CLK_UART0, 5, 0xff, 0,
			REG_CLK_BYP_3, 25, 0, -1,
			REG_DIV_HSPERI_CLK_UART0, 0),
	CV186X_CLK(CV186X_CLK_UART1, "clk_uart1",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 26,
			REG_DIV_HSPERI_CLK_UART1, 5, 0xff, 0,
			REG_CLK_BYP_3, 26, 0, -1,
			REG_DIV_HSPERI_CLK_UART1, 0),
	CV186X_CLK(CV186X_CLK_UART2, "clk_uart2",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 27,
			REG_DIV_HSPERI_CLK_UART2, 5, 0xff, 0,
			REG_CLK_BYP_3, 27, 0, -1,
			REG_DIV_HSPERI_CLK_UART2, 0),
	CV186X_CLK(CV186X_CLK_UART3, "clk_uart3",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 28,
			REG_DIV_HSPERI_CLK_UART3, 5, 0xff, 0,
			REG_CLK_BYP_3, 28, 0, -1,
			REG_DIV_HSPERI_CLK_UART3, 0),
	CV186X_CLK(CV186X_CLK_UART4, "clk_uart4",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 29,
			REG_DIV_HSPERI_CLK_UART4, 5, 0xff, 0,
			REG_CLK_BYP_3, 29, 0, -1,
			REG_DIV_HSPERI_CLK_UART4, 0),
	CV186X_CLK(CV186X_CLK_UART5, "clk_uart5",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 30,
			REG_DIV_HSPERI_CLK_UART5, 5, 0xff, 0,
			REG_CLK_BYP_3, 30, 0, -1,
			REG_DIV_HSPERI_CLK_UART5, 0),
	CV186X_CLK(CV186X_CLK_UART6, "clk_uart6",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_3, 31,
			REG_DIV_HSPERI_CLK_UART6, 5, 0xff, 0,
			REG_CLK_BYP_3, 31, 0, -1,
			REG_DIV_HSPERI_CLK_UART6, 0),
	CV186X_CLK(CV186X_CLK_UART7, "clk_uart7",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_4, 0,
			REG_DIV_HSPERI_CLK_UART7, 5, 0xff, 0,
			REG_CLK_BYP_4, 0, 0, -1,
			REG_DIV_HSPERI_CLK_UART7, 0),
	CV186X_CLK(CV186X_CLK_CAN, "clk_can",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_4, 1,
			REG_DIV_HSPERI_CLK_CAN, 10, 0xff, 0,
			REG_CLK_BYP_4, 1, 0, -1,
			REG_DIV_HSPERI_CLK_CAN, 0),
	CV186X_CLK(CV186X_CLK_WDT, "clk_wdt",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 2,  0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_GPIO_DB, "clk_gpio_db",
			((const char *[]) { "osc", "top_clk_1m"}),
			REG_CLK_EN_4, 3, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_4, 3, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_WGN, "clk_wgn",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 4, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_KEYSCAN, "clk_keyscan",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 5, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_EFUSE, "clk_apb_efuse",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_4, 6, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_4, 6, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_EFUSE, "clk_efuse",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 7, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_INTC, "clk_intc",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_4, 8, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_4, 8, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_OTP_C, "clk_otp_c",
			((const char *[]) { "osc", "clk_security"}),
			REG_CLK_EN_4, 9, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_4, 9, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_100M, "clk_100m",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_4, 10, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_4, 10, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_PWM, "clk_pwm",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_4, 11,
			REG_DIV_PERI_PWM_CLK, 10, 0xff, 0,
			REG_CLK_BYP_4, 11, 0, -1,
			REG_DIV_PERI_PWM_CLK, 0),
	CV186X_CLK(CV186X_CLK_RTC_32K, "clk_rtc_32k",
			((const char *[]) { "osc", "clk_int32kpll"}),
			REG_CLK_EN_4, 12, 0xff, 1, 0xff, 0,
			REG_CLK_BYP_4, 12, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_XTAL_MISC, "clk_xtal_misc",
			((const char *[]) { "osc", "clk_fpll"}),
			REG_CLK_EN_4, 13,
			REG_DIV_PERI_CLK_XTAL_MISC, 40, 0xff, 0,
			REG_CLK_BYP_4, 13, 0, -1,
			REG_DIV_PERI_CLK_XTAL_MISC, 0),
	CV186X_CLK(CV186X_CLK_TEMPSEN, "clk_tempsen",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 14, 0xff, 1, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SARADC, "clk_saradc",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 15, 0xff, 1, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_100M_FREE, "clk_100m_free",
			((const char *[]) { "osc", "clk_fab1"}),
			REG_CLK_EN_4, 16, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_4, 16, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_OTP, "clk_otp",
			((const char *[]) { "osc", "clk_security"}),
			REG_CLK_EN_4, 17, 0xff, 0, 0xff, 0,
			REG_CLK_BYP_4, 17, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_DBGSYS, "clk_dbgsys",
			((const char *[]) { "osc"}),
			REG_CLK_EN_4, 18, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AXI_VI, "clk_axi_vi",
			((const char *[]) {"vi_clk_raw_axi"}),
			REG_CLK_EN_4, 19, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC0_VI, "clk_csi_mac0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_4"}),
			REG_CLK_EN_4, 20, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC1_VI, "clk_csi_mac1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_4, 21, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC2_VI, "clk_csi_mac2_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_4, 22, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC3_VI, "clk_csi_mac3_vi",
			((const char *[]) {"vi_clk_src_vi_sys_2"}),
			REG_CLK_EN_4, 23, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC4_VI, "clk_csi_mac4_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_4, 24, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC5_VI, "clk_csi_mac5_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_4, 25, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC6_VI, "clk_csi_mac6_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_4, 26, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_MAC7_VI, "clk_csi_mac7_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_4, 27, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI_BE_VI, "clk_csi_be_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_4, 28, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_ISP_TOP_VI, "clk_isp_top_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_4, 29, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_RAW_VI, "clk_raw_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_4, 30, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VPSS0_VI, "clk_vpss0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_4, 31, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VPSS1_VI, "clk_vpss1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 0, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VPSS2_VI, "clk_vpss2_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 1, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VPSS3_VI, "clk_vpss3_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 2, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_DWA0_VI, "clk_dwa0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 3, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_DWA1_VI, "clk_dwa1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 4, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LDC0_VI, "clk_ldc0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 5, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LDC1_VI, "clk_ldc1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 6, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_DPU_VI, "clk_dpu_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 7, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_IVE0_VI, "clk_ive0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 8, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_IVE1_VI, "clk_ive1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 9, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_STITCHING_VI, "clk_stitching_vi",
			((const char *[]) {"vi_clk_src_vi_sys_3"}),
			REG_CLK_EN_5, 10, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LVDS0_VI, "clk_lvds0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 14, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LVDS1_VI, "clk_lvds1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 15, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LVDS2_VI, "clk_lvds2_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 16, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LVDS3_VI, "clk_lvds3_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 17, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LVDS4_VI, "clk_lvds4_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 18, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_LVDS5_VI, "clk_lvds5_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 19, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI0_RX_VI, "clk_csi0_rx_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 20, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI1_RX_VI, "clk_csi1_rx_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 21, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI2_RX_VI, "clk_csi2_rx_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 22, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI3_RX_VI, "clk_csi3_rx_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 23, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI4_RX_VI, "clk_csi4_rx_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 24, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CSI5_RX_VI, "clk_csi5_rx_vi",
			((const char *[]) {"vi_clk_src_vi_sys_1"}),
			REG_CLK_EN_5, 25, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_PAD_VI0_CLK0_VI, "pad_vi0_clk0_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_5, 26, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_PAD_VI0_CLK1_VI, "pad_vi0_clk1_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_5, 27, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_PAD_VI1_CLK_VI, "pad_vi1_clk_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_5, 28, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_PAD_VI2_CLK_VI, "pad_vi2_clk_vi",
			((const char *[]) {"vi_clk_src_vi_sys_0"}),
			REG_CLK_EN_5, 29, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_APB_VDSYS_TOP, "vd0_clk_apb_vdsys_top",
			((const char *[]) {"vd0_clk_top_axi"}),
			REG_CLK_EN_5, 30, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_APB_VDSYS_VD, "vd0_clk_apb_vdsys_vd",
			((const char *[]) {"vd0_clk_top_axi"}),
			REG_CLK_EN_5, 31, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_APB_VDSYS_VPSS_0, "vd0_clk_apb_vdsys_vpss_0",
			((const char *[]) {"vd0_clk_top_axi"}),
			REG_CLK_EN_6, 0, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_APB_VDSYS_VPSS_1, "vd0_clk_apb_vdsys_vpss_1",
			((const char *[]) {"vd0_clk_top_axi"}),
			REG_CLK_EN_6, 1, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_VDSYS_VD, "vd0_clk_vdsys_vd",
			((const char *[]) {"vd0_clk_axi_vdsys_0"}),
			REG_CLK_EN_6, 2, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_VDSYS_VPSS_0, "vd0_clk_vdsys_vpss_0",
			((const char *[]) {"vd0_clk_src2_vdsys"}),
			REG_CLK_EN_6, 3, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD0_CLK_VDSYS_VPSS_1, "vd0_clk_vdsys_vpss_1",
			((const char *[]) {"vd0_clk_src2_vdsys"}),
			REG_CLK_EN_6, 4, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_APB_VDSYS_TOP, "vd1_clk_apb_vdsys_top",
			((const char *[]) {"vd1_clk_top_axi"}),
			REG_CLK_EN_6, 5, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_APB_VDSYS_VD, "vd1_clk_apb_vdsys_vd",
			((const char *[]) {"vd1_clk_top_axi"}),
			REG_CLK_EN_6, 6, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_APB_VDSYS_VPSS_0, "vd1_clk_apb_vdsys_vpss_0",
			((const char *[]) {"vd1_clk_top_axi"}),
			REG_CLK_EN_6, 7, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_APB_VDSYS_VPSS_1, "vd1_clk_apb_vdsys_vpss_1",
			((const char *[]) {"vd1_clk_top_axi"}),
			REG_CLK_EN_6, 8, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_VDSYS_VD, "vd1_clk_vdsys_vd",
			((const char *[]) {"vd1_clk_axi_vdsys_0"}),
			REG_CLK_EN_6, 9, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_VDSYS_VPSS_0, "vd1_clk_vdsys_vpss_0",
			((const char *[]) {"vd1_clk_src2_vdsys"}),
			REG_CLK_EN_6, 10, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VD1_CLK_VDSYS_VPSS_1, "vd1_clk_vdsys_vpss_1",
			((const char *[]) {"vd1_clk_src2_vdsys"}),
			REG_CLK_EN_6, 11, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_VESYS_TOP, "clk_apb_vesys_top",
			((const char *[]) { "ve_clk_top_axi"}),
			REG_CLK_EN_6, 12, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_VESYS_VE, "clk_apb_vesys_ve",
			((const char *[]) { "ve_clk_top_axi"}),
			REG_CLK_EN_6, 13, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_VESYS_JPEG_0, "clk_apb_vesys_jpeg_0",
			((const char *[]) { "ve_clk_top_axi"}),
			REG_CLK_EN_6, 14, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_VESYS_JPEG_1, "clk_apb_vesys_jpeg_1",
			((const char *[]) { "ve_clk_top_axi"}),
			REG_CLK_EN_6, 15, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_VESYS_JPEG_2, "clk_apb_vesys_jpeg_2",
			((const char *[]) { "ve_clk_top_axi"}),
			REG_CLK_EN_6, 16, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_VESYS_JPEG_3, "clk_apb_vesys_jpeg_3",
			((const char *[]) { "ve_clk_top_axi"}),
			REG_CLK_EN_6, 17, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VESYS_VE, "clk_vesys_ve",
			((const char *[]) { "ve_clk_axi_vesys"}),
			REG_CLK_EN_6, 18, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VESYS_JPEG_0, "clk_vesys_jpeg_0",
			((const char *[]) { "ve_clk_src2_vesys"}),
			REG_CLK_EN_6, 19, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VESYS_JPEG_1, "clk_vesys_jpeg_1",
			((const char *[]) { "ve_clk_src2_vesys"}),
			REG_CLK_EN_6, 20, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VESYS_JPEG_2, "clk_vesys_jpeg_2",
			((const char *[]) { "ve_clk_src2_vesys"}),
			REG_CLK_EN_6, 21, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_VESYS_JPEG_3, "clk_vesys_jpeg_3",
			((const char *[]) { "ve_clk_src2_vesys"}),
			REG_CLK_EN_6, 22, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_AXI, "vo_clk_axi",
			((const char *[]) { "vo_clk_raw_axi"}),
			REG_CLK_EN_6, 23, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_2DE0, "vo_clk_2de0",
			((const char *[]) { "clk_src_vo_sys_0"}),
			REG_CLK_EN_6, 24, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_2DE1, "vo_clk_2de1",
			((const char *[]) { "clk_src_vo_sys_1"}),
			REG_CLK_EN_6, 25, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_OENC0, "vo_clk_oenc0",
			((const char *[]) { "clk_src_vo_sys_0"}),
			REG_CLK_EN_6, 26, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_OENC1, "vo_clk_oenc1",
			((const char *[]) { "clk_src_vo_sys_1"}),
			REG_CLK_EN_6, 27, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_VPSS0, "vo_clk_vpss0",
			((const char *[]) { "clk_src_vo_sys_1"}),
			REG_CLK_EN_6, 28, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_VPSS1, "vo_clk_vpss1",
			((const char *[]) { "clk_src_vo_sys_1"}),
			REG_CLK_EN_6, 29, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_DISP0, "vo_clk_disp0",
			((const char *[]) { "vo_clk_sys_disp0"}),
			REG_CLK_EN_6, 30, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_DISP1, "vo_clk_disp1",
			((const char *[]) { "vo_clk_sys_disp1"}),
			REG_CLK_EN_6, 31, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_VO_MAC0, "vo_clk_vo_mac0",
			((const char *[]) { "clk_src_vo_sys_2"}),
			REG_CLK_EN_7, 0, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_VO_MAC1, "vo_clk_vo_mac1",
			((const char *[]) { "clk_src_vo_sys_2"}),
			REG_CLK_EN_7, 1, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_DSI_MAC0, "vo_clk_dsi_mac0",
			((const char *[]) { "clk_src_vo_sys_2"}),
			REG_CLK_EN_7, 2, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_VO_CLK_DSI_MAC1, "vo_clk_dsi_mac1",
			((const char *[]) { "clk_src_vo_sys_2"}),
			REG_CLK_EN_7, 3, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_PCIEX2_CLK, "clk_pciex2",
			((const char *[]) { "ssperi_aclk_ddr"}),
			REG_CLK_EN_7, 5, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_PCIEX4_CLK, "clk_pciex4",
			((const char *[]) { "ssperi_aclk_ddr"}),
			REG_CLK_EN_7, 6, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_SATA_CLK, "clk_sata",
			((const char *[]) { "ssperi_aclk_ddr"}),
			REG_CLK_EN_7, 7, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AHB_ROM, "clk_ahb_rom",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 8, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AXI4_EMMC, "clk_axi4_emmc",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 9, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AXI4_SD0, "clk_axi4_sd0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 10, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AXI4_SD1, "clk_axi4_sd1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 11, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AXI4_ETH0, "clk_axi4_eth0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 13, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AXI4_ETH1, "clk_axi4_eth1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 14, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_AHB_SF, "clk_ahb_sf",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 15, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SDMA0_AXI, "clk_sdma0_axi",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 16, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SDMA1_AXI, "clk_sdma1_axi",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 17, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SPI0, "clk_spi0",
			((const char *[]) { "clk_spi"}),
			REG_CLK_EN_7, 18, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SPI1, "clk_spi1",
			((const char *[]) { "clk_spi"}),
			REG_CLK_EN_7, 19, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SPI2, "clk_spi2",
			((const char *[]) { "clk_spi"}),
			REG_CLK_EN_7, 20, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_SPI3, "clk_spi3",
			((const char *[]) { "clk_spi"}),
			REG_CLK_EN_7, 21, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_SPI0, "clk_apb_spi0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 22, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_SPI1, "clk_apb_spi1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 23, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_SPI2, "clk_apb_spi2",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 24, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_SPI3, "clk_apb_spi3",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 25, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART0, "clk_apb_uart0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 27, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART1, "clk_apb_uart1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 29, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART2, "clk_apb_uart2",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_7, 31, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART3, "clk_apb_uart3",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 1, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART4, "clk_apb_uart4",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 3, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART5, "clk_apb_uart5",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 5, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART6, "clk_apb_uart6",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 7, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_UART7, "clk_apb_uart7",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 9, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S_GLOBAL, "clk_apb_i2s_global",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 10, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S0, "clk_apb_i2s0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 11, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S1, "clk_apb_i2s1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 12, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S2, "clk_apb_i2s2",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 13, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S3, "clk_apb_i2s3",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 14, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S4, "clk_apb_i2s4",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 15, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S5, "clk_apb_i2s5",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 16, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2S6, "clk_apb_i2s6",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 17, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C0, "clk_i2c0",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 18, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C1, "clk_i2c1",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 19, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C2, "clk_i2c2",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 20, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C3, "clk_i2c3",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 21, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C4, "clk_i2c4",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 22, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C5, "clk_i2c5",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 23, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C6, "clk_i2c6",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 24, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C7, "clk_i2c7",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 25, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C8, "clk_i2c8",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 26, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_I2C9, "clk_i2c9",
			((const char *[]) { "clk_i2c"}),
			REG_CLK_EN_8, 27, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C0, "clk_apb_i2c0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 28, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C1, "clk_apb_i2c1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 29, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C2, "clk_apb_i2c2",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 30, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C3, "clk_apb_i2c3",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_8, 31, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C4, "clk_apb_i2c4",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 0, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C5, "clk_apb_i2c5",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 1, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C6, "clk_apb_i2c6",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 2, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C7, "clk_apb_i2c7",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 3, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C8, "clk_apb_i2c8",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 4, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_I2C9, "clk_apb_i2c9",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 5, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CAN0, "clk_can0",
			((const char *[]) { "clk_can"}),
			REG_CLK_EN_9, 6, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_CAN1, "clk_can1",
			((const char *[]) { "clk_can"}),
			REG_CLK_EN_9, 7, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_CAN0, "clk_apb_can0",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 8, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_CAN1, "clk_apb_can1",
			((const char *[]) { "clk_axi4"}),
			REG_CLK_EN_9, 9, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_WDT, "clk_apb_wdt",
			((const char *[]) { "clk_fab1"}),
			REG_CLK_EN_9, 10, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_MAILBOX, "clk_apb_mailbox",
			((const char *[]) { "clk_fab1"}),
			REG_CLK_EN_9, 11, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_OTP, "clk_apb_otp",
			((const char *[]) { "clk_fab1"}),
			REG_CLK_EN_9, 13, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_APB_OTP_C, "clk_apb_otp_c",
			((const char *[]) { "clk_security"}),
			REG_CLK_EN_9, 14, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, CLK_IS_CRITICAL),
	CV186X_CLK(CV186X_CLK_APB_GPIO, "clk_apb_gpio",
			((const char *[]) { "clk_fab1"}),
			REG_CLK_EN_9, 15, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_WGN0, "clk_wgn0",
			((const char *[]) { "clk_wgn"}),
			REG_CLK_EN_9, 17, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_WGN1, "clk_wgn1",
			((const char *[]) { "clk_wgn"}),
			REG_CLK_EN_9, 18, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER0, "clk_timer0",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 20, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER1, "clk_timer1",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 21, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER2, "clk_timer2",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 22, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER3, "clk_timer3",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 23, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER4, "clk_timer4",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 24, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER5, "clk_timer5",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 25, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER6, "clk_timer6",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 26, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_TIMER7, "clk_timer7",
			((const char *[]) { "clk_xtal_misc"}),
			REG_CLK_EN_9, 27, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_DUMMY_0, "clk_apb_audsrc",
			((const char *[]) {"clk_axi4"}),
			REG_CLK_EN_9, 30, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	CV186X_CLK(CV186X_CLK_DUMMY_1, "clk_apb_timer",
			((const char *[]) {"clk_fab1"}),
			REG_CLK_EN_9, 31, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	//CV186X_CLK(CV186X_CLK_DUMMY_2, "clk_dummy_2",
	//		((const char *[]) {"none"}),
	//		REG_CLK_EN_10, 0, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),
	//CV186X_CLK(CV186X_VO_CLK_HDMI_MAC, "vo_clk_hdmi_mac",
	//		((const char *[]) { "none"}),
	//		REG_CLK_EN_7, 4, 0xff, 0, 0xff, 0, 0, -1, 0, -1, 0xff, 0),

	//CV186X_CLK(CV186X_CLK_AXI4_SD2, "clk_axi4_sd2", ((const char *[]) {"clk_axi4"}), REG_CLK_EN_9, 29, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0),
};

static int __init cvi_clk_flags_setup(char *arg)
{
	int ret;
	unsigned long flags;

	ret = kstrtol(arg, 0, &flags);
	if (ret)
		return ret;

	cvi_clk_flags = flags;
	pr_info("cvi_clk_flags = 0x%lX\n", cvi_clk_flags);

	return 1;
}
__setup("cvi_clk_flags=", cvi_clk_flags_setup);

static unsigned long cv186x_pll_rate_calc(u32 regval, s16 post_div_sel, unsigned long parent_rate)
{
	u64 numerator;
	u32 predivsel, postdivsel, divsel;
	u32 denominator;

	predivsel = (regval >> 4) & 0x7f;
	postdivsel = post_div_sel < 0 ? (regval >> 24) & 0x7f : (u32)post_div_sel;
	divsel = (regval >> 16) & 0x7f;

	numerator = parent_rate * divsel;
	predivsel = predivsel ? predivsel : 1;
	postdivsel = postdivsel ? postdivsel : 1;
	denominator = predivsel * postdivsel;
	do_div(numerator, denominator);

	return (unsigned long)numerator;
}

static unsigned long cv186x_g6_pll_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct cv186x_pll_hw_clock *pll_hw = to_cv186x_pll_clk(hw);
	unsigned long rate;
	u32 regval;

	regval = readl(pll_hw->base + pll_hw->pll.reg_csr);
	rate = cv186x_pll_rate_calc(regval, pll_hw->pll.post_div_sel, parent_rate);

	return rate;
}

static long cv186x_g6_pll_round_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long *prate)
{
	return rate;
}

static int cv186x_g6_pll_calc_csr(unsigned long parent_rate, unsigned long rate, u32 *csr)
{
	u64 numerator;
	u32 denominator;
	u32 ictrl = 7;		/* [3:0] ICTRL */
	u32 predivsel = 1;	/* [10:4] PRE_DIV_SEL */
	u32 divsel;		/* [22:16] DIV_SEL */
	u32 postdivsel = 1;	/* [30:24] POST_DIV_SEL */
	//u32 selmode = 1;	/* [16:15] SEL_MODE */
	u32 vco_clks[] = {900, 1000, 1100, 1200, 1300, 1400, 1500, 1600};
	int i;

	for (i = 0; i < ARRAY_SIZE(vco_clks); i++) {
		if ((vco_clks[i] * 1000000) % rate == 0) {
			postdivsel = vco_clks[i] * 1000000 / rate;
			rate = vco_clks[i] * 1000000;
			pr_debug("rate=%ld, postdivsel=%d\n", rate, postdivsel);
			break;
		}
	}

	numerator = rate;
	denominator = parent_rate;

	do_div(numerator, denominator);

	divsel = (u32)numerator & 0x7f;
	*csr = (divsel << 16) | (postdivsel << 24) | ictrl | (predivsel << 4);

	pr_debug("csr=0x%08x\n", *csr);

	return 0;
}

static int cv186x_g6_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct cv186x_pll_hw_clock *pll_hw = to_cv186x_pll_clk(hw);
	unsigned long flags = 0;
	int ret;
	u32 reg_g6_pll_status;
	u32 regval_csr;
	u32 regval_g6_pll_status;
	u32 g6_pll_update_status = 0;
	ktime_t timeout;

	reg_g6_pll_status = (pll_hw->pll.reg_csr & ~PLL_STATUS_MASK) + PLL_STATUS_OFFSET;

	if (pll_hw->lock)
		spin_lock_irqsave(pll_hw->lock, flags);
	else
		__acquire(pll_hw->lock);

	/* calculate csr register */
	ret = cv186x_g6_pll_calc_csr(parent_rate, rate, &regval_csr);
	if (ret < 0)
		return ret;

	/* csr register */
	writel(regval_csr, pll_hw->base + pll_hw->pll.reg_csr);

	if ((pll_hw->pll.reg_csr & PLL_STATUS_MASK) == 0xa0)
		g6_pll_update_status = BIT(1);
	else if ((pll_hw->pll.reg_csr & PLL_STATUS_MASK) == 0x80)
		g6_pll_update_status = BIT(2);

	/* wait for pll setting updated */
	timeout = ktime_add_ms(ktime_get(), CV186X_PLL_LOCK_TIMEOUT_MS);
	while (1) {
		regval_g6_pll_status = readl(pll_hw->base + reg_g6_pll_status);
		if ((regval_g6_pll_status & g6_pll_update_status) == 0)
			break;

		if (ktime_after(ktime_get(), timeout)) {
			pr_err("timeout waiting for pll update, g6_pll_status = 0x%08x\n",
					regval_g6_pll_status);
			break;
		}
		cpu_relax();
	}

	if (pll_hw->lock)
		spin_unlock_irqrestore(pll_hw->lock, flags);
	else
		__release(pll_hw->lock);

	return 0;
}

static const struct clk_ops cv186x_g6_pll_ops = {
	.recalc_rate = cv186x_g6_pll_recalc_rate,
	.round_rate = cv186x_g6_pll_round_rate,
	.set_rate = cv186x_g6_pll_set_rate,
};

static unsigned long cv186x_g2_pll_recalc_rate(struct clk_hw *hw,
					       unsigned long parent_rate)
{
	struct cv186x_pll_hw_clock *pll_hw = to_cv186x_pll_clk(hw);
	u32 reg_ssc_set;
	u32 reg_g2_ssc_ctrl;
	u32 regval_csr;
	u32 regval_ssc_set;
	u32 regval_g2_ssc_ctrl;
	u64 numerator;
	u32 denominator;
	unsigned long clk_ref;
	unsigned long rate;

	regval_csr = readl(pll_hw->base + pll_hw->pll.reg_csr);

	/* pll without synthesizer */
	if (pll_hw->pll.reg_ssc == 0) {
		clk_ref = parent_rate;
		goto rate_calc;
	}

	/* calculate synthesizer freq */
	reg_ssc_set = (pll_hw->pll.reg_ssc & ~SSC_SYN_SET_MASK) + SSC_SYN_SET_OFFSET;
	reg_g2_ssc_ctrl = (pll_hw->pll.reg_ssc & ~G2_SSC_CTRL_MASK);

	regval_ssc_set = readl(pll_hw->base + reg_ssc_set);
	regval_g2_ssc_ctrl = readl(pll_hw->base + reg_g2_ssc_ctrl);

	/* bit0 sel_syn_clk */
	//TODO for reg_ssc_xxx_mux_sel
	numerator = (regval_g2_ssc_ctrl & 0x1) ? parent_rate : (parent_rate);

	numerator <<= 26;
	denominator = regval_ssc_set;
	if (denominator)
		do_div(numerator, denominator);
	else
		pr_err("pll ssc_set is zero\n");

	clk_ref = numerator;

rate_calc:
	rate = cv186x_pll_rate_calc(regval_csr, pll_hw->pll.post_div_sel, clk_ref);

	return rate;
}

static const struct {
	unsigned long rate;
	u32 csr;
	u32 ssc_set;
} g2_pll_rate_lut[] = {
	//{.rate = 1188000000, .csr = 0x01480018, .ssc_set = 0x060f83e1},
	//{.rate =  417792000, .csr = 0x01480018, .ssc_set = 0x113bc3c4}
	{.rate =  406425600, .csr = 0x04100018, .ssc_set = 0x46dc96ee},
	{.rate =  417792000, .csr = 0x04100018, .ssc_set = 0x44ef0f0e},
	{.rate =  425984000, .csr = 0x04100018, .ssc_set = 0x439bb13b},
	{.rate = 1188000000, .csr = 0x010c0018, .ssc_set = 0x48ba2e8b},
};

static int cv186x_g2_pll_get_setting_from_lut(unsigned long rate, u32 *csr,
					      u32 *ssc_set)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g2_pll_rate_lut); i++) {
		if (rate == g2_pll_rate_lut[i].rate) {
			*csr = g2_pll_rate_lut[i].csr;
			*ssc_set = g2_pll_rate_lut[i].ssc_set;
			return 0;
		}
	}

	*csr = 0;
	*ssc_set = 0;
	return -ENOENT;
}

static long cv186x_g2_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				     unsigned long *prate)
{
	return rate;
}

static int cv186x_g2_pll_set_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long parent_rate)
{
	struct cv186x_pll_hw_clock *pll_hw = to_cv186x_pll_clk(hw);
	unsigned long flags = 0;
	int ret;
	u32 reg_ssc_set;
	u32 reg_ssc_ctrl;
	u32 reg_g2_pll_status;
	u32 regval_csr;
	u32 regval_ssc_set;
	u32 regval_ssc_ctrl;
	u32 regval_g2_pll_status;
	u32 g2_pll_update_status = 0;
	ktime_t timeout;

	/* pll without synthesizer */
	if (pll_hw->pll.reg_ssc == 0)
		return -ENOENT;

	ret = cv186x_g2_pll_get_setting_from_lut(rate, &regval_csr,
						 &regval_ssc_set);
	if (ret < 0)
		return ret;

	reg_ssc_set = (pll_hw->pll.reg_ssc & ~SSC_SYN_SET_MASK) + SSC_SYN_SET_OFFSET;
	reg_ssc_ctrl = pll_hw->pll.reg_ssc;
	reg_g2_pll_status = (pll_hw->pll.reg_csr & ~PLL_STATUS_MASK) + PLL_STATUS_OFFSET;

	if (pll_hw->lock)
		spin_lock_irqsave(pll_hw->lock, flags);
	else
		__acquire(pll_hw->lock);

	/* set synthersizer */
	writel(regval_ssc_set, pll_hw->base + reg_ssc_set);

	/* bit 0 toggle */
	regval_ssc_ctrl = readl(pll_hw->base + reg_ssc_ctrl);
	regval_ssc_ctrl ^= 0x00000001;
	writel(regval_ssc_ctrl, pll_hw->base + reg_ssc_ctrl);

	/* csr register */
	writel(regval_csr, pll_hw->base + pll_hw->pll.reg_csr);

	//TODO for updating_xxx_val check
	// DISPPLL/A0PLL ==> bit0
	g2_pll_update_status = BIT(0);

	/* wait for pll setting updated */
	timeout = ktime_add_ms(ktime_get(), CV186X_PLL_LOCK_TIMEOUT_MS);
	while (1) {
		regval_g2_pll_status = readl(pll_hw->base + reg_g2_pll_status);
		if ((regval_g2_pll_status & g2_pll_update_status) == 0)
			break;

		if (ktime_after(ktime_get(), timeout)) {
			pr_err("timeout waiting for pll update, g2_pll_status = 0x%08x\n",
			       regval_g2_pll_status);
			break;
		}
		cpu_relax();
	}

	if (pll_hw->lock)
		spin_unlock_irqrestore(pll_hw->lock, flags);
	else
		__release(pll_hw->lock);

	return 0;
}

static const struct clk_ops cv186x_g2_pll_ops = {
	.recalc_rate = cv186x_g2_pll_recalc_rate,
	.round_rate = cv186x_g2_pll_round_rate,
	.set_rate = cv186x_g2_pll_set_rate,
};

static const struct clk_ops cv186x_g2d_pll_ops = {
	.recalc_rate = cv186x_g2_pll_recalc_rate,
};

static struct clk_hw *cv186x_clk_register_pll(struct cv186x_pll_hw_clock *pll_clk,
					    void __iomem *sys_base)
{
	struct clk_hw *hw;
	struct clk_init_data init;
	int err;

	pll_clk->lock = &cv186x_clk_lock;
	pll_clk->base = sys_base;

	if (cvi_clk_flags) {
		/* copy clk_init_data for modification */
		memcpy(&init, pll_clk->hw.init, sizeof(init));

		init.flags |= cvi_clk_flags;
		pll_clk->hw.init = &init;
	}

	hw = &pll_clk->hw;

	err = clk_hw_register(NULL, hw);
	if (err)
		return ERR_PTR(err);

	return hw;
}

static void cv186x_clk_unregister_pll(struct clk_hw *hw)
{
	struct cv186x_pll_hw_clock *pll_hw = to_cv186x_pll_clk(hw);

	clk_hw_unregister(hw);
	kfree(pll_hw);
}

static int cv186x_clk_register_plls(struct cv186x_pll_hw_clock *clks,
				    int num_clks,
				    struct cv186x_clock_data *data)
{
	struct clk_hw *hw;
	void __iomem *pll_base = data->base;
	int i;

	for (i = 0; i < num_clks; i++) {
		struct cv186x_pll_hw_clock *cv186x_clk = &clks[i];

		hw = cv186x_clk_register_pll(cv186x_clk, pll_base);
		if (IS_ERR(hw)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, cv186x_clk->pll.name);
			goto err_clk;
		}

		data->hw_data.hws[clks[i].pll.id] = hw;

		clk_hw_register_clkdev(hw, cv186x_clk->pll.name, NULL);
	}

	return 0;

err_clk:
	while (i--)
		cv186x_clk_unregister_pll(data->hw_data.hws[clks[i].pll.id]);

	return PTR_ERR(hw);
}

static int cv186x_clk_is_bypassed(struct cv186x_hw_clock *clk_hw)
{
	u32 val;
	void __iomem *reg_addr = clk_hw->base + clk_hw->mux[0].reg;

	if (clk_hw->mux[0].shift >= 0) {
		val = readl(reg_addr) >> clk_hw->mux[0].shift;
		val &= 0x1; //width
	} else {
		val = 0;
	}

	return val;
}

static int cv186x_clk_get_clk_sel(struct cv186x_hw_clock *clk_hw)
{
	u32 val;
	void __iomem *reg_addr = clk_hw->base + clk_hw->mux[1].reg;

	if (clk_hw->mux[1].shift >= 0) {
		val = readl(reg_addr) >> clk_hw->mux[1].shift;
		val &= 0x1; //width
		val ^= 0x1; //invert value
	} else {
		val = 0;
	}

	return val;
}

static int cv186x_clk_get_src_sel(struct cv186x_hw_clock *clk_hw)
{
	u32 val;
	void __iomem *reg_addr = clk_hw->base + clk_hw->mux[2].reg;

	if (clk_hw->mux[2].reg != 0xff) {
		val = readl(reg_addr) >> clk_hw->mux[2].shift;
		val &= 0x3; //width
	} else {
		val = 0;
	}

	return val;
}

static unsigned long cv186x_clk_div_recalc_rate(struct clk_hw *hw,
					      unsigned long parent_rate)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	unsigned int clk_sel = cv186x_clk_get_clk_sel(clk_hw);
	void __iomem *reg_addr = clk_hw->base + clk_hw->div[clk_sel].reg;
	unsigned int val;
	unsigned long rate;

	if (clk_hw->div[0].reg == 0xff)
		return parent_rate;

	if ((clk_hw->mux[0].shift >= 0) && cv186x_clk_is_bypassed(clk_hw))
		return parent_rate;

	if ((clk_hw->div[clk_sel].initval > 0) && !(readl(reg_addr) & BIT(3))) {
		val = clk_hw->div[clk_sel].initval;
	} else {
		val = readl(reg_addr) >> clk_hw->div[clk_sel].shift;
		val &= div_mask(clk_hw->div[clk_sel].width);
	}
	rate = divider_recalc_rate(hw, parent_rate, val, NULL,
				   clk_hw->div[clk_sel].flags,
				   clk_hw->div[clk_sel].width);

	return rate;
}

static long cv186x_clk_div_round_rate(struct clk_hw *hw, unsigned long rate,
				      unsigned long *prate)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	unsigned int clk_sel = cv186x_clk_get_clk_sel(clk_hw);

	if ((clk_hw->mux[0].shift >= 0) && cv186x_clk_is_bypassed(clk_hw))
		return DIV_ROUND_UP_ULL((u64)*prate, 1);

	if (clk_hw->div[0].reg == 0xff)
                return DIV_ROUND_UP_ULL((u64)*prate, 1);

	return divider_round_rate(hw, rate, prate, NULL,
				  clk_hw->div[clk_sel].width, clk_hw->div[clk_sel].flags);
}

static long cv186x_clk_div_calc_round_rate(struct clk_hw *hw, unsigned long rate,
					 unsigned long *prate)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);

	if (clk_hw->div[0].reg != 0xff)
		return divider_round_rate(hw, rate, prate, NULL,
					clk_hw->div[0].width, clk_hw->div[0].flags);
	else
		return DIV_ROUND_UP_ULL((u64)*prate, 1);
}

static int cv186x_clk_div_determine_rate(struct clk_hw *hw,
				       struct clk_rate_request *req)
{
	struct clk_hw *current_parent;
	unsigned long parent_rate;
	unsigned long best_delta;
	unsigned long best_rate;
	u32 parent_count;
	long rate;
	u32 which;

	pr_debug("%s()_%d: req->rate=%ld\n", __func__, __LINE__, req->rate);

	parent_count = clk_hw_get_num_parents(hw);
	pr_debug("%s()_%d: parent_count=%d\n", __func__, __LINE__, parent_count);

	if ((parent_count < 2) || (clk_hw_get_flags(hw) & CLK_SET_RATE_NO_REPARENT)) {
		rate = cv186x_clk_div_round_rate(hw, req->rate, &req->best_parent_rate);
		if (rate < 0)
			return rate;

		req->rate = rate;
		return 0;
	}

	/* Unless we can do better, stick with current parent */
	current_parent = clk_hw_get_parent(hw);
	parent_rate = clk_hw_get_rate(current_parent);
	best_rate = cv186x_clk_div_calc_round_rate(hw, req->rate, &parent_rate);
	best_delta = abs(best_rate - req->rate);

	pr_debug("%s()_%d: parent_rate=%ld, best_rate=%ld, best_delta=%ld\n",
		 __func__, __LINE__, parent_rate, best_rate, best_delta);

	/* Check whether any other parent clock can produce a better result */
	for (which = 0; which < parent_count; which++) {
		struct clk_hw *parent = clk_hw_get_parent_by_index(hw, which);
		unsigned long delta;
		unsigned long other_rate;

		pr_debug("%s()_%d: idx=%d, parent_rate=%ld, best_rate=%ld, best_delta=%ld\n",
			 __func__, __LINE__, which, parent_rate, best_rate, best_delta);

		if (!parent)
			continue;

		if (parent == current_parent)
			continue;

		/* Not support CLK_SET_RATE_PARENT */
		parent_rate = clk_hw_get_rate(parent);
		other_rate = cv186x_clk_div_calc_round_rate(hw, req->rate, &parent_rate);
		delta = abs(other_rate - req->rate);
		pr_debug("%s()_%d: parent_rate=%ld, other_rate=%ld, delta=%ld\n",
			 __func__, __LINE__, parent_rate, other_rate, delta);
		if (delta < best_delta) {
			best_delta = delta;
			best_rate = other_rate;
			req->best_parent_hw = parent;
			req->best_parent_rate = parent_rate;
			pr_debug("%s()_%d: parent_rate=%ld, best_rate=%ld, best_delta=%ld\n",
				 __func__, __LINE__, parent_rate, best_rate, best_delta);
		}
	}

	req->rate = best_rate;

	return 0;
}

static int cv186x_clk_div_set_rate(struct clk_hw *hw, unsigned long rate,
				 unsigned long parent_rate)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	unsigned int clk_sel = cv186x_clk_get_clk_sel(clk_hw);
	void __iomem *reg_addr = clk_hw->base + clk_hw->div[clk_sel].reg;
	unsigned long flags = 0;
	int value;
	u32 val;

	if (clk_hw->div[0].reg == 0xff) {
		if (rate != parent_rate) {
			pr_warn("w/o div: %lu to %lu\n", parent_rate, rate);
		}
		return 0;
	}

	value = divider_get_val(rate, parent_rate, NULL,
				clk_hw->div[clk_sel].width,
				clk_hw->div[clk_sel].flags);
	if (value < 0)
		return value;

	if (clk_hw->lock)
		spin_lock_irqsave(clk_hw->lock, flags);
	else
		__acquire(clk_hw->lock);

	val = readl(reg_addr);
	val &= ~(div_mask(clk_hw->div[clk_sel].width) << clk_hw->div[clk_sel].shift);
	val |= (u32)value << clk_hw->div[clk_sel].shift;
	if (!(clk_hw->div[clk_sel].initval < 0))
		val |= BIT(3);
	writel(val, reg_addr);

	if (clk_hw->lock)
		spin_unlock_irqrestore(clk_hw->lock, flags);
	else
		__release(clk_hw->lock);

	return 0;
}

static void cv186x_clk_gate_endisable(struct clk_hw *hw, int enable)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	void __iomem *reg_addr = clk_hw->base + clk_hw->gate.reg;
	unsigned long flags = 0;
	u32 reg;

	if (clk_hw->lock)
		spin_lock_irqsave(clk_hw->lock, flags);
	else
		__acquire(clk_hw->lock);

	reg = readl(reg_addr);

	if (enable)
		reg |= BIT(clk_hw->gate.shift);
	else
		reg &= ~BIT(clk_hw->gate.shift);

	writel(reg, reg_addr);

	if (clk_hw->lock)
		spin_unlock_irqrestore(clk_hw->lock, flags);
	else
		__release(clk_hw->lock);
}

static int cv186x_clk_gate_enable(struct clk_hw *hw)
{
	cv186x_clk_gate_endisable(hw, 1);

	return 0;
}

static void cv186x_clk_gate_disable(struct clk_hw *hw)
{
	cv186x_clk_gate_endisable(hw, 0);
}

static int cv186x_clk_gate_is_enabled(struct clk_hw *hw)
{
	u32 reg;
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	void __iomem *reg_addr = clk_hw->base + clk_hw->gate.reg;

	reg = readl(reg_addr);

	reg &= BIT(clk_hw->gate.shift);

	if (clk_hw_get_flags(hw) & CLK_IGNORE_UNUSED)
		return __clk_get_enable_count(hw->clk) ? (reg ? 1 : 0) : 0;
	else
		return reg ? 1 : 0;
}

static u8 cv186x_clk_mux_get_parent(struct clk_hw *hw)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	u8 clk_sel = cv186x_clk_get_clk_sel(clk_hw);
	u8 src_sel = cv186x_clk_get_src_sel(clk_hw);
	u8 parent_idx = 0;

	/*
	 * | 0     | 1     | 2     | 3     | 4     | 5     |
	 * +-------+-------+-------+-------+-------+-------+
	 * | XTAL  | src_0 | src_1 | src_2 | src_3 | DIV_1 |
	 * | XTAL  | src_0 | src_1 | src_2 | src_3 |       |
	 * | src_0 | src_1 | src_2 | src_3 | DIV_1 |       |
	 * | src_0 | src_1 | src_2 | src_3 |       |       |
	 * +-------+-------+-------+-------+-------+-------+
	 */

	if (clk_hw->mux[0].shift >= 0) {
		// clk with bypass reg
		if (cv186x_clk_is_bypassed(clk_hw)) {
			parent_idx = 0;
		} else {
			if (clk_hw->mux[1].shift >= 0) {
				// clk with clk_sel reg
				if (clk_sel) {
					parent_idx = 5; //TODO +src_sel2
				} else {
					parent_idx = src_sel + 1;
				}
			} else {
				// clk without clk_sel reg
				parent_idx = src_sel + 1;
			}
		}
	} else {
		// clk without bypass reg
		if (clk_hw->mux[1].shift >= 0) {
			// clk with clk_sel reg
			if (clk_sel) {
				parent_idx = 4; //TODO +src_sel2
			} else {
				parent_idx = src_sel;
			}
		} else {
			//clk without clk_sel reg
			parent_idx = src_sel;
		}
	}

	return parent_idx;
}

static int cv186x_clk_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct cv186x_hw_clock *clk_hw = to_cv186x_clk(hw);
	unsigned long flags = 0;
	void __iomem *reg_addr;
	unsigned int reg;

	if (clk_hw->lock)
		spin_lock_irqsave(clk_hw->lock, flags);
	else
		__acquire(clk_hw->lock);

	/*
	 * | 0     | 1     | 2     | 3     | 4     | 5     |
	 * +-------+-------+-------+-------+-------+-------+
	 * | XTAL  | DIV_1 | src_0 | src_1 | src_2 | src_3 |
	 * | XTAL  | src_0 | src_1 | src_2 | src_3 |       |
	 * | DIV_1 | src_0 | src_1 | src_2 | src_3 |       |
	 * | src_0 | src_1 | src_2 | src_3 |       |       |
	 * +-------+-------+-------+-------+-------+-------+
	 */

	if (index == 0) {
		if (clk_hw->mux[0].shift >= 0) {
			// set bypass
			reg_addr = clk_hw->base + clk_hw->mux[0].reg;
			reg = readl(reg_addr);
			reg |= 1 << clk_hw->mux[0].shift;
			writel(reg, reg_addr);
			goto unlock_release;
		} else if (clk_hw->mux[1].shift >= 0) {
			// set clk_sel to DIV_1
			reg_addr = clk_hw->base + clk_hw->mux[1].reg;
			reg = readl(reg_addr);
			reg &= ~(1 << clk_hw->mux[1].shift);
			writel(reg, reg_addr);
			goto unlock_release;
		}
	} else if (index == 1) {
		if (clk_hw->mux[0].shift >= 0) {
			// clear bypass
			reg_addr = clk_hw->base + clk_hw->mux[0].reg;
			reg = readl(reg_addr);
			reg &= ~(0x1 << clk_hw->mux[0].shift);
			writel(reg, reg_addr);

			if (clk_hw->mux[1].shift >= 0) {
				// set clk_sel to DIV_1
				reg_addr = clk_hw->base + clk_hw->mux[1].reg;
				reg = readl(reg_addr);
				reg &= ~(1 << clk_hw->mux[1].shift);
				writel(reg, reg_addr);
				goto unlock_release;
			} else {
				index--;
			}
		} else if (clk_hw->mux[1].shift >= 0) {
			// set clk_sel to DIV_0
			reg_addr = clk_hw->base + clk_hw->mux[1].reg;
			reg = readl(reg_addr);
			reg |= 1 << clk_hw->mux[1].shift;
			writel(reg, reg_addr);
			index--;
		}
	} else {
		if (clk_hw->mux[0].shift >= 0) {
			// clear bypass
			reg_addr = clk_hw->base + clk_hw->mux[0].reg;
			reg = readl(reg_addr);
			reg &= ~(0x1 << clk_hw->mux[0].shift);
			writel(reg, reg_addr);
			index--;
		}

		if (clk_hw->mux[1].shift >= 0) {
			// set clk_sel to DIV_0
			reg_addr = clk_hw->base + clk_hw->mux[1].reg;
			reg = readl(reg_addr);
			reg |= 1 << clk_hw->mux[1].shift;
			writel(reg, reg_addr);
			index--;
		}
	}

	if (index < 0) {
		pr_err("index is negative(%d)\n", index);
		goto unlock_release;
	}

	// set src_sel reg
	reg_addr = clk_hw->base + clk_hw->mux[2].reg;
	reg = readl(reg_addr);
	reg &= ~(0x3 << clk_hw->mux[2].shift); // clear bits
	reg |= (index & 0x3) << clk_hw->mux[2].shift; //set bits
	writel(reg, reg_addr);

unlock_release:
	if (clk_hw->lock)
		spin_unlock_irqrestore(clk_hw->lock, flags);
	else
		__release(clk_hw->lock);

	return 0;
}

static const struct clk_ops cv186x_clk_ops = {
	// gate
	.enable = cv186x_clk_gate_enable,
	.disable = cv186x_clk_gate_disable,
	.is_enabled = cv186x_clk_gate_is_enabled,

	// div
	.recalc_rate = cv186x_clk_div_recalc_rate,
	.round_rate = cv186x_clk_div_round_rate,
	.determine_rate = cv186x_clk_div_determine_rate,
	.set_rate = cv186x_clk_div_set_rate,

	//mux
	.get_parent = cv186x_clk_mux_get_parent,
	.set_parent = cv186x_clk_mux_set_parent,
};

static struct clk_hw *cv186x_register_clk(struct cv186x_hw_clock *cv186x_clk,
					  void __iomem *sys_base)
{
	struct clk_hw *hw;
	struct clk_init_data init;
	int err;

	cv186x_clk->gate.flags = 0;
	cv186x_clk->div[0].flags = CLK_DIVIDER_ONE_BASED | CLK_DIVIDER_ALLOW_ZERO |
				 CLK_DIVIDER_ROUND_CLOSEST;
	cv186x_clk->div[1].flags = CLK_DIVIDER_ONE_BASED | CLK_DIVIDER_ALLOW_ZERO |
				 CLK_DIVIDER_ROUND_CLOSEST;
	cv186x_clk->mux[0].flags = 0; /* clk byp */
	cv186x_clk->mux[1].flags = 0; /* clk sel */
	cv186x_clk->mux[2].flags = 0; /* div_0 src_sel */
	cv186x_clk->base = sys_base;
	cv186x_clk->lock = &cv186x_clk_lock;

	if (cvi_clk_flags) {
		/* copy clk_init_data for modification */
		memcpy(&init, cv186x_clk->hw.init, sizeof(init));

		init.flags |= cvi_clk_flags;
		cv186x_clk->hw.init = &init;
	}

	hw = &cv186x_clk->hw;
	err = clk_hw_register(NULL, hw);
	if (err) {
		return ERR_PTR(err);
	}

	return hw;
}

static void cv186x_unregister_clk(struct clk_hw *hw)
{
	struct cv186x_hw_clock *cv186x_clk = to_cv186x_clk(hw);

	clk_hw_unregister(hw);
	kfree(cv186x_clk);
}
static int cv186x_register_clks(struct cv186x_hw_clock *clks,
				int num_clks,
				struct cv186x_clock_data *data)
{
	struct clk_hw *hw;
	void __iomem *sys_base = data->base;
	unsigned int i;

	for (i = 0; i < num_clks; i++) {
		struct cv186x_hw_clock *cv186x_clk = &clks[i];

		//pr_err("%s: register clock %s\n", __func__, cv186x_clk->name);
		hw = cv186x_register_clk(cv186x_clk, sys_base);

		if (IS_ERR(hw)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, cv186x_clk->name);
			goto err_clk;
		}
		data->hw_data.hws[clks[i].id] = hw;
		clk_hw_register_clkdev(hw, cv186x_clk->name, NULL);
	}

	return 0;

err_clk:
	while (i--)
		cv186x_unregister_clk(data->hw_data.hws[clks[i].id]);

	return PTR_ERR(hw);
}

#ifdef CONFIG_PM_SLEEP
static void cv186x_clk_readl(volatile void __iomem *from, const void *to, size_t count)
{
        while (count--) {
                *(u32 *)to = __raw_readl(from);
                from += 4;
                to += 4;
        }
}

static void cv186x_clk_writel(volatile void __iomem *to, const void *from, size_t count)
{
        while (count--) {
                __raw_writel(*(u32 *)from, to);
                from += 4;
                to += 4;
        }
}

static int cv186x_clk_suspend(void)
{
	cv186x_clk_readl(clk_data->base + REG_CLK_EN_START,
		clk_data->clken_saved_regs,
		REG_CLK_EN_NUM);

	cv186x_clk_readl(clk_data->base + REG_CLK_SEL_START,
		clk_data->clksel_saved_regs,
		REG_CLK_SEL_NUM);

	cv186x_clk_readl(clk_data->base + REG_CLK_BYP_START,
			clk_data->clkbyp_saved_regs,
			REG_CLK_BYP_NUM);

	cv186x_clk_readl(clk_data->base + REG_CLK_DIV_START,
			clk_data->clkdiv_saved_regs,
			REG_CLK_DIV_NUM);

	return 0;
}

static void cv186x_clk_resume(void)
{
	/* switch clock to xtal */
	writel(0xffffffff, clk_data->base + REG_CLK_BYP_0);
	writel(0xffffffff, clk_data->base + REG_CLK_BYP_1);
	writel(0xffffffff, clk_data->base + REG_CLK_BYP_2);
	writel(0xffffffff, clk_data->base + REG_CLK_BYP_3);
	writel(0xffffffff, clk_data->base + REG_CLK_BYP_4);

	cv186x_clk_writel(clk_data->base + REG_CLK_EN_START,
			clk_data->clken_saved_regs,
			REG_CLK_EN_NUM);

	cv186x_clk_writel(clk_data->base + REG_CLK_SEL_START,
			clk_data->clksel_saved_regs,
			REG_CLK_SEL_NUM);

	cv186x_clk_writel(clk_data->base + REG_CLK_DIV_START,
			clk_data->clkdiv_saved_regs,
			REG_CLK_DIV_NUM);

	cv186x_clk_writel(clk_data->base + REG_CLK_BYP_START,
			clk_data->clkbyp_saved_regs,
			REG_CLK_BYP_NUM);
}

static struct syscore_ops cv186x_clk_syscore_ops = {
	.suspend = cv186x_clk_suspend,
	.resume = cv186x_clk_resume,
};
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id cvi_clk_match_ids_tables[] = {
	{
		.compatible = "cvitek,cv186x-clk",
	},
	{}
};

static void __init cvi_clk_init(struct device_node *node)
{
	int num_clks;
	int i;
	int ret = 0;
	int of_num_clks;
	struct clk *clk;

	of_num_clks = of_clk_get_parent_count(node);
	for (i = 0; i < of_num_clks; i++) {
		clk = of_clk_get(node, i);
		clk_register_clkdev(clk, __clk_get_name(clk), NULL);
		clk_put(clk);
	}

	num_clks = ARRAY_SIZE(cv186x_pll_clks) +
		   ARRAY_SIZE(cv186x_clks);

	clk_data = kzalloc(sizeof(struct cv186x_clock_data) +
			   sizeof(struct clk_hw) * num_clks,
			   GFP_KERNEL);
	if (!clk_data) {
		ret = -ENOMEM;
		goto out;
	}

	for (i = 0; i < num_clks; i++)
		clk_data->hw_data.hws[i] = ERR_PTR(-ENOENT);

	clk_data->hw_data.num = num_clks;

	clk_data->lock = &cv186x_clk_lock;

	clk_data->base = of_iomap(node, 0);
	if (!clk_data->base) {
		pr_err("Failed to map address range for cvitek,cv186x-clk node\n");
		return;
	}

	cv186x_clk_register_plls(cv186x_pll_clks,
			       ARRAY_SIZE(cv186x_pll_clks),
			       clk_data);
	cv186x_register_clks(cv186x_clks,
			   ARRAY_SIZE(cv186x_clks),
			   clk_data);


	/* register clk-provider */
	ret = of_clk_add_hw_provider(node, of_clk_hw_onecell_get, &clk_data->hw_data);
	if (ret)
		pr_err("Unable to add hw clk provider\n");

	/* force enable clocks */
	// clk_prepare_enable(clk_data->hw_data.hws[CV186X_CLK_DSI_MAC_VIP]->clk);
	// clk_prepare_enable(clk_data->hw_data.hws[CV186X_CLK_DISP_VIP]->clk);
	// clk_prepare_enable(clk_data->hw_data.hws[CV186X_CLK_BT_VIP]->clk);
	clk_prepare_enable(clk_data->hw_data.hws[CV186X_TPU_CLK_TPU]->clk);

#ifdef CONFIG_PM_SLEEP
	register_syscore_ops(&cv186x_clk_syscore_ops);
#endif

	if (!ret)
		return;

out:
	pr_err("%s failed error number %d\n", __func__, ret);
}
CLK_OF_DECLARE(cvi_clk, "cvitek,cv186x-clk", cvi_clk_init);
