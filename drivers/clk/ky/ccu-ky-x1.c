// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ky x1 clock controller driver
 *
 * Copyright (c) 2023, ky Corporation.
 *
 */

#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <dt-bindings/clock/ky-x1-clock.h>
#include "ccu-ky-x1.h"
#include "ccu_mix.h"
#include "ccu_pll.h"
#include "ccu_ddn.h"
#include "ccu_dpll.h"
#include "ccu_ddr.h"

#define LOG_INFO(fmt, arg...)    pr_info("[X1-CLK][%s][%d]:" fmt "\n", __func__, __LINE__, ##arg)

DEFINE_SPINLOCK(g_cru_lock);

/* APBS register offset */
//pll1
#define APB_SPARE1_REG          0x100
#define APB_SPARE2_REG          0x104
#define APB_SPARE3_REG          0x108
//pll2
#define APB_SPARE7_REG          0x118
#define APB_SPARE8_REG          0x11c
#define APB_SPARE9_REG          0x120
//pll3
#define APB_SPARE10_REG         0x124
#define APB_SPARE11_REG         0x128
#define APB_SPARE12_REG         0x12c
/* end of APBS register offset */

/* APBC register offset */
#define APBC_UART1_CLK_RST      0x0
#define APBC_UART2_CLK_RST      0x4
#define APBC_GPIO_CLK_RST       0x8
#define APBC_PWM0_CLK_RST       0xc
#define APBC_PWM1_CLK_RST       0x10
#define APBC_PWM2_CLK_RST       0x14
#define APBC_PWM3_CLK_RST       0x18
#define APBC_TWSI8_CLK_RST      0x20
#define APBC_UART3_CLK_RST      0x24
#define APBC_RTC_CLK_RST        0x28 //reserved
#define APBC_TWSI0_CLK_RST      0x2c
#define APBC_TWSI1_CLK_RST      0x30
#define APBC_TIMERS1_CLK_RST    0x34
#define APBC_TWSI2_CLK_RST      0x38
#define APBC_AIB_CLK_RST        0x3c
#define APBC_TWSI4_CLK_RST      0x40
#define APBC_TIMERS2_CLK_RST    0x44
#define APBC_ONEWIRE_CLK_RST    0x48
#define APBC_TWSI5_CLK_RST      0x4c
#define APBC_DRO_CLK_RST        0x58
#define APBC_IR_CLK_RST         0x5c
#define APBC_TWSI6_CLK_RST      0x60
#define APBC_COUNTER_CLK_SEL    0x64

#define APBC_TWSI7_CLK_RST      0x68
#define APBC_TSEN_CLK_RST       0x6c

#define APBC_UART4_CLK_RST      0x70
#define APBC_UART5_CLK_RST      0x74
#define APBC_UART6_CLK_RST      0x78
#define APBC_SSP3_CLK_RST       0x7c

#define APBC_SSPA0_CLK_RST      0x80
#define APBC_SSPA1_CLK_RST      0x84

#define APBC_IPC_AP2AUD_CLK_RST 0x90
#define APBC_UART7_CLK_RST      0x94
#define APBC_UART8_CLK_RST      0x98
#define APBC_UART9_CLK_RST      0x9c

#define APBC_CAN0_CLK_RST       0xa0
#define APBC_PWM4_CLK_RST       0xa8
#define APBC_PWM5_CLK_RST       0xac
#define APBC_PWM6_CLK_RST       0xb0
#define APBC_PWM7_CLK_RST       0xb4
#define APBC_PWM8_CLK_RST       0xb8
#define APBC_PWM9_CLK_RST       0xbc
#define APBC_PWM10_CLK_RST      0xc0
#define APBC_PWM11_CLK_RST      0xc4
#define APBC_PWM12_CLK_RST      0xc8
#define APBC_PWM13_CLK_RST      0xcc
#define APBC_PWM14_CLK_RST      0xd0
#define APBC_PWM15_CLK_RST      0xd4
#define APBC_PWM16_CLK_RST      0xd8
#define APBC_PWM17_CLK_RST      0xdc
#define APBC_PWM18_CLK_RST      0xe0
#define APBC_PWM19_CLK_RST      0xe4
/* end of APBC register offset */

/* MPMU register offset */
#define MPMU_POSR			0x10 //no define
#define POSR_PLL1_LOCK			BIT(27)
#define POSR_PLL2_LOCK			BIT(28)
#define POSR_PLL3_LOCK			BIT(29)

#define MPMU_VRCR			0x18 //no define
#define MPMU_VRCR_REQ_EN0		BIT(0)
#define MPMU_VRCR_REQ_EN2		BIT(2)
#define MPMU_VRCR_REQ_POL2		BIT(6)
#define MPMU_VRCR_VCXO_OUT_REQ_EN2	BIT(14)

#define MPMU_WDTPCR     0x200
#define MPMU_RIPCCR     0x210 //no define
#define MPMU_ACGR       0x1024
#define MPMU_SUCCR      0x14
#define MPMU_ISCCR      0x44
#define MPMU_SUCCR_1    0x10b0
#define MPMU_APBCSCR    0x1050

/* end of MPMU register offset */

/* APMU register offset */
#define APMU_JPG_CLK_RES_CTRL       0x20
#define APMU_CSI_CCIC2_CLK_RES_CTRL 0x24
#define APMU_ISP_CLK_RES_CTRL       0x38
#define APMU_LCD_CLK_RES_CTRL1      0x44
#define APMU_LCD_SPI_CLK_RES_CTRL   0x48
#define APMU_LCD_CLK_RES_CTRL2      0x4c
#define APMU_CCIC_CLK_RES_CTRL      0x50
#define APMU_SDH0_CLK_RES_CTRL      0x54
#define APMU_SDH1_CLK_RES_CTRL      0x58
#define APMU_USB_CLK_RES_CTRL       0x5c
#define APMU_QSPI_CLK_RES_CTRL      0x60
#define APMU_USB_CLK_RES_CTRL       0x5c
#define APMU_DMA_CLK_RES_CTRL       0x64
#define APMU_AES_CLK_RES_CTRL       0x68
#define APMU_VPU_CLK_RES_CTRL       0xa4
#define APMU_GPU_CLK_RES_CTRL       0xcc
#define APMU_SDH2_CLK_RES_CTRL      0xe0
#define APMU_PMUA_MC_CTRL           0xe8
#define APMU_PMU_CC2_AP             0x100
#define APMU_PMUA_EM_CLK_RES_CTRL   0x104

#define APMU_AUDIO_CLK_RES_CTRL     0x14c
#define APMU_HDMI_CLK_RES_CTRL      0x1B8
#define APMU_CCI550_CLK_CTRL        0x300
#define APMU_ACLK_CLK_CTRL          0x388
#define APMU_CPU_C0_CLK_CTRL        0x38C
#define APMU_CPU_C1_CLK_CTRL        0x390

#define APMU_PCIE_CLK_RES_CTRL_0    0x3cc
#define APMU_PCIE_CLK_RES_CTRL_1    0x3d4
#define APMU_PCIE_CLK_RES_CTRL_2    0x3dc

#define APMU_EMAC0_CLK_RES_CTRL     0x3e4
#define APMU_EMAC1_CLK_RES_CTRL     0x3ec

#define APMU_DFC_AP                 0x180
#define APMU_DFC_STATUS             0x188

#define APMU_DFC_LEVEL0             0x190
#define APMU_DFC_LEVEL1             0x194
#define APMU_DFC_LEVEL2             0x198
#define APMU_DFC_LEVEL3             0x19c
#define APMU_DFC_LEVEL4             0x1a0
#define APMU_DFC_LEVEL5             0x1a4
#define APMU_DFC_LEVEL6             0x1a8
#define APMU_DFC_LEVEL7             0x1ac

#define APMU_DPLL1_CLK_CTRL1        0x39c
#define APMU_DPLL1_CLK_CTRL2        0x3a0
#define APMU_DPLL2_CLK_CTRL1        0x3a8
#define APMU_DPLL2_CLK_CTRL2        0x3ac
/* end of APMU register offset */

/* APBC2 register offset */
#define APBC2_UART1_CLK_RST		0x00
#define APBC2_SSP2_CLK_RST		0x04
#define APBC2_TWSI3_CLK_RST		0x08
#define APBC2_RTC_CLK_RST		0x0c
#define APBC2_TIMERS0_CLK_RST		0x10
#define APBC2_KPC_CLK_RST		0x14
#define APBC2_GPIO_CLK_RST		0x1c
/* end of APBC2 register offset */

/* RCPU register offset */
#define RCPU_HDMI_CLK_RST		0x2044
#define RCPU_CAN_CLK_RST		0x4c
#define RCPU_I2C0_CLK_RST		0x30

#define RCPU_SSP0_CLK_RST		0x28
#define RCPU_IR_CLK_RST 		0x48
#define RCPU_UART0_CLK_RST		0xd8
#define RCPU_UART1_CLK_RST		0x3c
/* end of RCPU register offset */

/* RCPU2 register offset */
#define RCPU2_PWM0_CLK_RST		0x00
#define RCPU2_PWM1_CLK_RST		0x04
#define RCPU2_PWM2_CLK_RST		0x08
#define RCPU2_PWM3_CLK_RST		0x0c
#define RCPU2_PWM4_CLK_RST		0x10
#define RCPU2_PWM5_CLK_RST		0x14
#define RCPU2_PWM6_CLK_RST		0x18
#define RCPU2_PWM7_CLK_RST		0x1c
#define RCPU2_PWM8_CLK_RST		0x20
#define RCPU2_PWM9_CLK_RST		0x24
/* end of RCPU2 register offset */

struct ky_x1_clk x1_clock_controller;

//apbs
static const struct ccu_pll_rate_tbl pll2_rate_tbl[] = {
	PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
	PLL_RATE(3200000000UL, 0x67, 0xdd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
	PLL_RATE(2800000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3a, 0x155555),
};

static const struct ccu_pll_rate_tbl pll3_rate_tbl[] = {
	PLL_RATE(1600000000UL, 0x61, 0xcd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(1800000000UL, 0x61, 0xcd, 0x50, 0x00, 0x4b, 0x000000),
	PLL_RATE(2000000000UL, 0x62, 0xdd, 0x50, 0x00, 0x2a, 0xeaaaab),
	PLL_RATE(3000000000UL, 0x66, 0xdd, 0x50, 0x00, 0x3f, 0xe00000),
	PLL_RATE(3200000000UL, 0x67, 0xdd, 0x50, 0x00, 0x43, 0xeaaaab),
	PLL_RATE(2457600000UL, 0x64, 0xdd, 0x50, 0x00, 0x33, 0x0ccccd),
};

static KY_CCU_PLL(pll2, "pll2", &pll2_rate_tbl, ARRAY_SIZE(pll2_rate_tbl),
	BASE_TYPE_APBS, APB_SPARE7_REG, APB_SPARE8_REG, APB_SPARE9_REG,
	MPMU_POSR, POSR_PLL2_LOCK, 1,
	CLK_IGNORE_UNUSED);

static KY_CCU_PLL(pll3, "pll3", &pll3_rate_tbl, ARRAY_SIZE(pll3_rate_tbl),
	BASE_TYPE_APBS, APB_SPARE10_REG, APB_SPARE11_REG, APB_SPARE12_REG,
	MPMU_POSR, POSR_PLL3_LOCK, 1,
	CLK_IGNORE_UNUSED);

//pll1
static KY_CCU_GATE_FACTOR(pll1_d2, "pll1_d2", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d3, "pll1_d3", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(2), BIT(2), 0x0,
	3, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d4, "pll1_d4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d5, "pll1_d5", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(4), BIT(4), 0x0,
	5, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d6, "pll1_d6", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d7, "pll1_d7", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(6), BIT(6), 0x0,
	7, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d8, "pll1_d8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d11_223p4, "pll1_d11_223p4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(15), BIT(15), 0x0,
	11, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d13_189, "pll1_d13_189", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(16), BIT(16), 0x0,
	13, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d23_106p8, "pll1_d23_106p8", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(20), BIT(20), 0x0,
	23, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d64_38p4, "pll1_d64_38p4", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(0), BIT(0), 0x0,
	64, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_aud_245p7, "pll1_aud_245p7", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(10), BIT(10), 0x0,
	10, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_aud_24p5, "pll1_aud_24p5", "pll1_2457p6_vco",
	BASE_TYPE_APBS, APB_SPARE2_REG,
	BIT(11), BIT(11), 0x0,
	100, 1, CLK_IGNORE_UNUSED);

//pll2
static KY_CCU_GATE_FACTOR(pll2_d1, "pll2_d1", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(0), BIT(0), 0x0,
	1, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d2, "pll2_d2", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d3, "pll2_d3", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(2), BIT(2), 0x0,
	3, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d4, "pll2_d4", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d5, "pll2_d5", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(4), BIT(4), 0x0,
	5, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d6, "pll2_d6", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d7, "pll2_d7", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(6), BIT(6), 0x0,
	7, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll2_d8, "pll2_d8", "pll2",
	BASE_TYPE_APBS, APB_SPARE8_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, CLK_IGNORE_UNUSED);

//pll3
static KY_CCU_GATE_FACTOR(pll3_d1, "pll3_d1", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(0), BIT(0), 0x0,
	1, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d2, "pll3_d2", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(1), BIT(1), 0x0,
	2, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d3, "pll3_d3", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(2), BIT(2), 0x0,
	3, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d4, "pll3_d4", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(3), BIT(3), 0x0,
	4, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d5, "pll3_d5", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(4), BIT(4), 0x0,
	5, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d6, "pll3_d6", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(5), BIT(5), 0x0,
	6, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d7, "pll3_d7", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(6), BIT(6), 0x0,
	7, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll3_d8, "pll3_d8", "pll3",
	BASE_TYPE_APBS, APB_SPARE11_REG,
	BIT(7), BIT(7), 0x0,
	8, 1, CLK_IGNORE_UNUSED);

//pll3_div
static KY_CCU_FACTOR(pll3_80, "pll3_80", "pll3_d8",
	5, 1);
static KY_CCU_FACTOR(pll3_40, "pll3_40", "pll3_d8",
	10, 1);
static KY_CCU_FACTOR(pll3_20, "pll3_20", "pll3_d8",
	20, 1);

//pll1_d8
static KY_CCU_GATE(pll1_d8_307p2, "pll1_d8_307p2", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(13), BIT(13), 0x0,
	CLK_IGNORE_UNUSED);
static KY_CCU_FACTOR(pll1_d32_76p8, "pll1_d32_76p8", "pll1_d8_307p2",
	4, 1);
static KY_CCU_FACTOR(pll1_d40_61p44, "pll1_d40_61p44", "pll1_d8_307p2",
	5, 1);
static KY_CCU_FACTOR(pll1_d16_153p6, "pll1_d16_153p6", "pll1_d8",
	2, 1);
static KY_CCU_GATE_FACTOR(pll1_d24_102p4, "pll1_d24_102p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(12), BIT(12), 0x0,
	3, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d48_51p2, "pll1_d48_51p2", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(7), BIT(7), 0x0,
	6, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d48_51p2_ap, "pll1_d48_51p2_ap", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(11), BIT(11), 0x0,
	6, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_m3d128_57p6, "pll1_m3d128_57p6", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(8), BIT(8), 0x0,
	16, 3, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d96_25p6, "pll1_d96_25p6", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(4), BIT(4), 0x0,
	12, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d192_12p8, "pll1_d192_12p8", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(3), BIT(3), 0x0,
	24, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d192_12p8_wdt, "pll1_d192_12p8_wdt", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(19), BIT(19), 0x0,
	24, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d384_6p4, "pll1_d384_6p4", "pll1_d8",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(2), BIT(2), 0x0,
	48, 1, CLK_IGNORE_UNUSED);
static KY_CCU_FACTOR(pll1_d768_3p2, "pll1_d768_3p2", "pll1_d384_6p4",
	2, 1);
static KY_CCU_FACTOR(pll1_d1536_1p6, "pll1_d1536_1p6", "pll1_d384_6p4",
	4, 1);
static KY_CCU_FACTOR(pll1_d3072_0p8, "pll1_d3072_0p8", "pll1_d384_6p4",
	8, 1);
//pll1_d7
static KY_CCU_FACTOR(pll1_d7_351p08, "pll1_d7_351p08", "pll1_d7",
	1, 1);
//pll1_d6
static KY_CCU_GATE(pll1_d6_409p6, "pll1_d6_409p6", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(0), BIT(0), 0x0,
	CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d12_204p8, "pll1_d12_204p8", "pll1_d6",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(5), BIT(5), 0x0,
	2, 1, CLK_IGNORE_UNUSED);
//pll1_d5
static KY_CCU_GATE(pll1_d5_491p52, "pll1_d5_491p52", "pll1_d5",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(21), BIT(21), 0x0,
	CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d10_245p76, "pll1_d10_245p76", "pll1_d5",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(18), BIT(18), 0x0,
	2, 1, CLK_IGNORE_UNUSED);
//pll1_d4
static KY_CCU_GATE(pll1_d4_614p4, "pll1_d4_614p4", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(15), BIT(15), 0x0,
	CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d52_47p26, "pll1_d52_47p26", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(10), BIT(10), 0x0,
	13, 1, CLK_IGNORE_UNUSED);
static KY_CCU_GATE_FACTOR(pll1_d78_31p5, "pll1_d78_31p5", "pll1_d4",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(6), BIT(6), 0x0,
	39, 2, CLK_IGNORE_UNUSED);
//pll1_d3
static KY_CCU_GATE(pll1_d3_819p2, "pll1_d3_819p2", "pll1_d3",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(14), BIT(14), 0x0,
	CLK_IGNORE_UNUSED);
//pll1_d2
static KY_CCU_GATE(pll1_d2_1228p8, "pll1_d2_1228p8", "pll1_d2",
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(16), BIT(16), 0x0,
	CLK_IGNORE_UNUSED);

//dpll
static const struct ccu_dpll_rate_tbl dpll1_rate_tbl[] = {
	DPLL_RATE(2400000000UL, 0x00, 0x00, 0x20, 0x2a, 0x32, 0x64, 0xdd, 0x50), //5000ppm
	DPLL_RATE(2400000000UL, 0x00, 0x3b, 0x20, 0x2a, 0x32, 0x64, 0xdd, 0x50), //5000ppm + pre-setting
};

static const struct ccu_dpll_rate_tbl dpll2_rate_tbl[] = {
	DPLL_RATE(3200000000UL, 0x55, 0x55, 0x3d, 0x2a, 0x43, 0x67, 0xdd, 0x50), //5000ppm
};

static KY_CCU_DPLL(dpll1, "dpll1", &dpll1_rate_tbl, ARRAY_SIZE(dpll1_rate_tbl),
	BASE_TYPE_APMU, APMU_DPLL1_CLK_CTRL1, APMU_DPLL1_CLK_CTRL2,
	0, CLK_IGNORE_UNUSED);

static KY_CCU_DPLL(dpll2, "dpll2", &dpll2_rate_tbl, ARRAY_SIZE(dpll2_rate_tbl),
	BASE_TYPE_APMU, APMU_DPLL2_CLK_CTRL1, APMU_DPLL2_CLK_CTRL2,
	0, CLK_IGNORE_UNUSED);

static const char * const dfc_lvl_parents[] = {
	"dpll2", "dpll1"
};

static KY_CCU_DIV_MUX(dfc_lvl0, "dfc_lvl0", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL0,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl1, "dfc_lvl1", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL1,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl2, "dfc_lvl2", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL2,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl3, "dfc_lvl3", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL3,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl4, "dfc_lvl4", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL4,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl5, "dfc_lvl5", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL5,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl6, "dfc_lvl6", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL6,
	14, 2, 8, 1, 0);
static KY_CCU_DIV_MUX(dfc_lvl7, "dfc_lvl7", dfc_lvl_parents,
	BASE_TYPE_APMU, APMU_DFC_LEVEL7,
	14, 2, 8, 1, 0);
static const char * const ddr_clk_parents[] = {
	"dfc_lvl0", "dfc_lvl1", "dfc_lvl2", "dfc_lvl3", "dfc_lvl4", "dfc_lvl5", "dfc_lvl6", "dfc_lvl7"
};
static KY_CCU_DDR_FC(ddr, "ddr", ddr_clk_parents,
	BASE_TYPE_APMU, APMU_DFC_AP, BIT(0),
	1, 3, 0);

//mpmu
static struct ccu_ddn_info uart_ddn_mask_info = {
	.factor = 2,
	.num_mask = 0x1fff,
	.den_mask = 0x1fff,
	.num_shift = 16,
	.den_shift = 0,
};
static struct ccu_ddn_tbl slow_uart1_tbl[] = {
	{.num = 125, .den = 24}, /*rate = parent_rate*24/125/2) */
};
static struct ccu_ddn_tbl slow_uart2_tbl[] = {
	{.num = 6144, .den = 960},/*rate = parent_rate*960/6144/2) */
};

static KY_CCU_GATE_NO_PARENT(slow_uart, "slow_uart", NULL,
	BASE_TYPE_MPMU, MPMU_ACGR,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DDN(slow_uart1_14p74, "slow_uart1_14p74", "pll1_d16_153p6",
	&uart_ddn_mask_info, &slow_uart1_tbl, ARRAY_SIZE(slow_uart1_tbl),
	BASE_TYPE_MPMU, MPMU_SUCCR,
	CLK_IGNORE_UNUSED);
static KY_CCU_DDN(slow_uart2_48, "slow_uart2_48", "pll1_d4_614p4",
	&uart_ddn_mask_info, &slow_uart2_tbl, ARRAY_SIZE(slow_uart2_tbl),
	BASE_TYPE_MPMU, MPMU_SUCCR_1,
	CLK_IGNORE_UNUSED);

//apbc
static const char * const uart_parent_names[] = {
	"pll1_m3d128_57p6", "slow_uart1_14p74", "slow_uart2_48"
};
static KY_CCU_MUX_GATE(uart1_clk, "uart1_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart2_clk, "uart2_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart3_clk, "uart3_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart4_clk, "uart4_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART4_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart5_clk, "uart5_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART5_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart6_clk, "uart6_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART6_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart7_clk, "uart7_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART7_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart8_clk, "uart8_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART8_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(uart9_clk, "uart9_clk", uart_parent_names,
	BASE_TYPE_APBC, APBC_UART9_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_GATE(gpio_clk, "gpio_clk", "vctcxo_24",
	BASE_TYPE_APBC, APBC_GPIO_CLK_RST,
	0x3, 0x3, 0x0,
	0);
static const char *pwm_parent_names[] = {
	"pll1_d192_12p8", "clk_32k"
};
static KY_CCU_MUX_GATE(pwm0_clk, "pwm0_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM0_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm1_clk, "pwm1_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM1_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm2_clk, "pwm2_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM2_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm3_clk, "pwm3_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM3_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm4_clk, "pwm4_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM4_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm5_clk, "pwm5_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM5_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm6_clk, "pwm6_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM6_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm7_clk, "pwm7_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM7_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm8_clk, "pwm8_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM8_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm9_clk, "pwm9_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM9_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm10_clk, "pwm10_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM10_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm11_clk, "pwm11_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM11_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm12_clk, "pwm12_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM12_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm13_clk, "pwm13_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM13_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm14_clk, "pwm14_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM14_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm15_clk, "pwm15_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM15_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm16_clk, "pwm16_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM16_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm17_clk, "pwm17_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM17_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm18_clk, "pwm18_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM18_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static KY_CCU_MUX_GATE(pwm19_clk, "pwm19_clk", pwm_parent_names,
	BASE_TYPE_APBC, APBC_PWM19_CLK_RST,
	4, 3, 0x2, 0x2, 0x0,
	0);
static const char *ssp_parent_names[] = { "pll1_d384_6p4", "pll1_d192_12p8", "pll1_d96_25p6",
	"pll1_d48_51p2", "pll1_d768_3p2", "pll1_d1536_1p6", "pll1_d3072_0p8"
};
static KY_CCU_MUX_GATE(ssp3_clk, "ssp3_clk", ssp_parent_names,
	BASE_TYPE_APBC, APBC_SSP3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_GATE(rtc_clk, "rtc_clk", "clk_32k",
	BASE_TYPE_APBC, APBC_RTC_CLK_RST,
	0x83, 0x83, 0x0, 0);
static const char *twsi_parent_names[] = {
	"pll1_d78_31p5", "pll1_d48_51p2", "pll1_d40_61p44"
};
static KY_CCU_MUX_GATE(twsi0_clk, "twsi0_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi1_clk, "twsi1_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi2_clk, "twsi2_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi4_clk, "twsi4_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI4_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi5_clk, "twsi5_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI5_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi6_clk, "twsi6_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI6_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi7_clk, "twsi7_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI7_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(twsi8_clk, "twsi8_clk", twsi_parent_names,
	BASE_TYPE_APBC, APBC_TWSI8_CLK_RST,
	4, 3, 0x7, 0x3, 0x4,
	0);
static const char *timer_parent_names[] = {
	"pll1_d192_12p8", "clk_32k", "pll1_d384_6p4", "vctcxo_3", "vctcxo_1"
};
static KY_CCU_MUX_GATE(timers1_clk, "timers1_clk", timer_parent_names,
	BASE_TYPE_APBC, APBC_TIMERS1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(timers2_clk, "timers2_clk", timer_parent_names,
	BASE_TYPE_APBC, APBC_TIMERS2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_GATE(aib_clk, "aib_clk", "vctcxo_24",
	BASE_TYPE_APBC, APBC_AIB_CLK_RST,
	0x3, 0x3, 0x0, 0);
static KY_CCU_GATE_NO_PARENT(onewire_clk, "onewire_clk", NULL,
	BASE_TYPE_APBC, APBC_ONEWIRE_CLK_RST,
	0x3, 0x3, 0x0, 0);

static KY_CCU_GATE_FACTOR(i2s_sysclk, "i2s_sysclk", "pll1_d8_307p2",
	BASE_TYPE_MPMU, MPMU_ISCCR,
	BIT(31), BIT(31), 0x0,
	200, 1, 0);
static KY_CCU_GATE_FACTOR(i2s_bclk, "i2s_bclk", "i2s_sysclk",
	BASE_TYPE_MPMU, MPMU_ISCCR,
	BIT(29), BIT(29), 0x0,
	1, 1, 0);
static const char *sspa_parent_names[] = { "pll1_d384_6p4", "pll1_d192_12p8", "pll1_d96_25p6",
	"pll1_d48_51p2", "pll1_d768_3p2", "pll1_d1536_1p6", "pll1_d3072_0p8", "i2s_bclk"
};
static KY_CCU_MUX_GATE(sspa0_clk, "sspa0_clk", sspa_parent_names,
	BASE_TYPE_APBC, APBC_SSPA0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_MUX_GATE(sspa1_clk, "sspa1_clk", sspa_parent_names,
	BASE_TYPE_APBC, APBC_SSPA1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(dro_clk, "dro_clk", NULL,
	BASE_TYPE_APBC, APBC_DRO_CLK_RST,
	0x1, 0x1, 0x0, 0);
static KY_CCU_GATE_NO_PARENT(ir_clk, "ir_clk", NULL,
	BASE_TYPE_APBC, APBC_IR_CLK_RST,
	0x1, 0x1, 0x0, 0);
static KY_CCU_GATE_NO_PARENT(tsen_clk, "tsen_clk", NULL,
	BASE_TYPE_APBC, APBC_TSEN_CLK_RST,
	0x3, 0x3, 0x0, 0);
static KY_CCU_GATE_NO_PARENT(ipc_ap2aud_clk, "ipc_ap2aud_clk", NULL,
	BASE_TYPE_APBC, APBC_IPC_AP2AUD_CLK_RST,
	0x3, 0x3, 0x0, 0);
static const char *can_parent_names[] = {
	"pll3_20", "pll3_40", "pll3_80"
};
static KY_CCU_MUX_GATE(can0_clk, "can0_clk", can_parent_names,
	BASE_TYPE_APBC, APBC_CAN0_CLK_RST,
	4, 3, BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(can0_bus_clk, "can0_bus_clk", NULL,
	BASE_TYPE_APBC, APBC_CAN0_CLK_RST,
	BIT(0), BIT(0), 0x0, 0);

//mpmu
static KY_CCU_GATE(wdt_clk, "wdt_clk", "pll1_d96_25p6",
	BASE_TYPE_MPMU, MPMU_WDTPCR,
	0x3, 0x3, 0x0, 0);
static KY_CCU_GATE_NO_PARENT(ripc_clk, "ripc_clk", NULL,
	BASE_TYPE_MPMU, MPMU_RIPCCR,
	0x3, 0x3, 0x0, 0);

//apmu
static const char * const jpg_parent_names[] = {
	 "pll1_d4_614p4", "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d3_819p2",
	 "pll1_d2_1228p8", "pll2_d4", "pll2_d3"
};
static KY_CCU_DIV_FC_MUX_GATE(jpg_clk, "jpg_clk", jpg_parent_names,
	BASE_TYPE_APMU, APMU_JPG_CLK_RES_CTRL,
	5, 3, BIT(15),
	2, 3, BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(jpg_4kafbc_clk, "jpg_4kafbc_clk", NULL,
	BASE_TYPE_APMU, APMU_JPG_CLK_RES_CTRL,
	BIT(16), BIT(16), 0x0, 0);
static KY_CCU_GATE_NO_PARENT(jpg_2kafbc_clk, "jpg_2kafbc_clk", NULL,
	BASE_TYPE_APMU, APMU_JPG_CLK_RES_CTRL,
	BIT(17), BIT(17), 0x0, 0);
static const char * const ccic2phy_parent_names[] = {
	"pll1_d24_102p4", "pll1_d48_51p2_ap"
};
static KY_CCU_MUX_GATE(ccic2phy_clk, "ccic2phy_clk", ccic2phy_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	7, 1, BIT(5), BIT(5), 0x0,
	0);
static const char * const ccic3phy_parent_names[] = {
	"pll1_d24_102p4", "pll1_d48_51p2_ap"
};
static KY_CCU_MUX_GATE(ccic3phy_clk, "ccic3phy_clk", ccic3phy_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	31, 1, BIT(30), BIT(30), 0x0, 0);
static const char * const csi_parent_names[] = {
	 "pll1_d5_491p52", "pll1_d6_409p6", "pll1_d4_614p4", "pll1_d3_819p2",
	 "pll2_d2", "pll2_d3", "pll2_d4", "pll1_d2_1228p8"
};
static KY_CCU_DIV_FC_MUX_GATE(csi_clk, "csi_clk", csi_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	20, 3, BIT(15),
	16, 3, BIT(4), BIT(4), 0x0,
	0);
static const char * const camm_parent_names[] = {
	"pll1_d8_307p2", "pll2_d5", "pll1_d6_409p6", "vctcxo_24"
};
static KY_CCU_DIV_MUX_GATE(camm0_clk, "camm0_clk", camm_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	23, 4, 8, 2,
	BIT(28), BIT(28), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(camm1_clk, "camm1_clk", camm_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	23, 4, 8, 2,
	BIT(6), BIT(6), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(camm2_clk, "camm2_clk", camm_parent_names,
	BASE_TYPE_APMU, APMU_CSI_CCIC2_CLK_RES_CTRL,
	23, 4, 8, 2,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const isp_cpp_parent_names[] = {
	 "pll1_d8_307p2", "pll1_d6_409p6"
};
static KY_CCU_DIV_MUX_GATE(isp_cpp_clk, "isp_cpp_clk", isp_cpp_parent_names,
	BASE_TYPE_APMU, APMU_ISP_CLK_RES_CTRL,
	24, 2, 26, 1,
	BIT(28), BIT(28), 0x0,
	0);
static const char * const isp_bus_parent_names[] = {
	 "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d8_307p2", "pll1_d10_245p76"
};
static KY_CCU_DIV_FC_MUX_GATE(isp_bus_clk, "isp_bus_clk", isp_bus_parent_names,
	BASE_TYPE_APMU, APMU_ISP_CLK_RES_CTRL,
	18, 3, BIT(23),
	21, 2, BIT(17), BIT(17), 0x0,
	0);
static const char * const isp_parent_names[] = {
	 "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2"
};
static KY_CCU_DIV_FC_MUX_GATE(isp_clk, "isp_clk", isp_parent_names,
	BASE_TYPE_APMU, APMU_ISP_CLK_RES_CTRL,
	4, 3, BIT(7),
	8, 2, BIT(1), BIT(1), 0x0,
	0);
static const char * const dpumclk_parent_names[] = {
	"pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2"
};
static KY_CCU_DIV2_FC_MUX_GATE(dpu_mclk, "dpu_mclk", dpumclk_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1, APMU_LCD_CLK_RES_CTRL2,
	1, 4, BIT(29),
	5, 3, BIT(0), BIT(0), 0x0,
	0);
static const char * const dpuesc_parent_names[] = {
	"pll1_d48_51p2_ap", "pll1_d52_47p26", "pll1_d96_25p6", "pll1_d32_76p8"
};
static KY_CCU_MUX_GATE(dpu_esc_clk, "dpu_esc_clk", dpuesc_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	0, 2, BIT(2), BIT(2), 0x0,
	0);
static const char * const dpubit_parent_names[] = { "pll1_d3_819p2", "pll2_d2", "pll2_d3",
	"pll1_d2_1228p8", "pll2_d4", "pll2_d5", "pll2_d8", "pll2_d8" //6 should be 429M?
};
static KY_CCU_DIV_FC_MUX_GATE(dpu_bit_clk, "dpu_bit_clk", dpubit_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	17, 3, BIT(31),
	20, 3, BIT(16), BIT(16), 0x0,
	0);
static const char * const dpupx_parent_names[] = {
	"pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2", "pll2_d7", "pll2_d8"
};
static KY_CCU_DIV2_FC_MUX_GATE(dpu_pxclk, "dpu_pxclk", dpupx_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1, APMU_LCD_CLK_RES_CTRL2,
	17, 4, BIT(30),
	21, 3, BIT(16), BIT(16), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(dpu_hclk, "dpu_hclk", NULL,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	BIT(5), BIT(5), 0x0,
	0);
static const char * const dpu_spi_parent_names[] = {
	 "pll1_d8_307p2", "pll1_d6_409p6", "pll1_d10_245p76", "pll1_d11_223p4",
	 "pll1_d13_189", "pll1_d23_106p8", "pll2_d3", "pll2_d5"
};
static KY_CCU_DIV_FC_MUX_GATE(dpu_spi_clk, "dpu_spi_clk", dpu_spi_parent_names,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	8, 3, BIT(7),
	12, 3, BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(dpu_spi_hbus_clk, "dpu_spi_hbus_clk", NULL,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(dpu_spi_bus_clk, "dpu_spi_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	BIT(5), BIT(5), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(dpu_spi_aclk, "dpu_spi_aclk", NULL,
	BASE_TYPE_APMU, APMU_LCD_SPI_CLK_RES_CTRL,
	BIT(6), BIT(6), 0x0,
	0);
static const char * const v2d_parent_names[] = {
	"pll1_d5_491p52", "pll1_d6_409p6", "pll1_d8_307p2", "pll1_d4_614p4",
};
static KY_CCU_DIV_FC_MUX_GATE(v2d_clk, "v2d_clk", v2d_parent_names,
	BASE_TYPE_APMU, APMU_LCD_CLK_RES_CTRL1,
	9, 3, BIT(28),
	12, 2, BIT(8), BIT(8), 0x0,
	0);
static const char * const ccic_4x_parent_names[] = {
	 "pll1_d5_491p52", "pll1_d6_409p6", "pll1_d4_614p4", "pll1_d3_819p2",
	 "pll2_d2", "pll2_d3", "pll2_d4", "pll1_d2_1228p8"
};
static KY_CCU_DIV_FC_MUX_GATE(ccic_4x_clk, "ccic_4x_clk", ccic_4x_parent_names,
	BASE_TYPE_APMU, APMU_CCIC_CLK_RES_CTRL,
	18, 3, BIT(15),
	23, 2, BIT(4), BIT(4), 0x0,
	0);
static const char * const ccic1phy_parent_names[] = {
	"pll1_d24_102p4", "pll1_d48_51p2_ap"
};
static KY_CCU_MUX_GATE(ccic1phy_clk, "ccic1phy_clk", ccic1phy_parent_names,
	BASE_TYPE_APMU, APMU_CCIC_CLK_RES_CTRL,
	7, 1, BIT(5), BIT(5), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(sdh_axi_aclk, "sdh_axi_aclk", NULL,
	BASE_TYPE_APMU, APMU_SDH0_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const sdh01_parent_names[] = {"pll1_d6_409p6",
	"pll1_d4_614p4", "pll2_d8", "pll2_d5", "pll1_d11_223p4", "pll1_d13_189", "pll1_d23_106p8" };

static KY_CCU_DIV_FC_MUX_GATE(sdh0_clk, "sdh0_clk", sdh01_parent_names,
	BASE_TYPE_APMU, APMU_SDH0_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static KY_CCU_DIV_FC_MUX_GATE(sdh1_clk, "sdh1_clk", sdh01_parent_names,
	BASE_TYPE_APMU, APMU_SDH1_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static const char * const sdh2_parent_names[] = {"pll1_d6_409p6",
	"pll1_d4_614p4", "pll2_d8", "pll1_d3_819p2", "pll1_d11_223p4", "pll1_d13_189", "pll1_d23_106p8" };

static KY_CCU_DIV_FC_MUX_GATE(sdh2_clk, "sdh2_clk", sdh2_parent_names,
	BASE_TYPE_APMU, APMU_SDH2_CLK_RES_CTRL,
	8, 3, BIT(11),
	5, 3, BIT(4), BIT(4), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(usb_axi_clk, "usb_axi_clk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(usb_p1_aclk, "usb_p1_aclk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(5), BIT(5), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(usb30_clk, "usb30_clk", NULL,
	BASE_TYPE_APMU, APMU_USB_CLK_RES_CTRL,
	BIT(8), BIT(8), 0x0,
	0);
static const char * const qspi_parent_names[] = {"pll1_d6_409p6", "pll2_d8", "pll1_d8_307p2",
		"pll1_d10_245p76", "pll1_d11_223p4", "pll1_d23_106p8", "pll1_d5_491p52", "pll1_d13_189"};
static KY_CCU_DIV_MUX_GATE(qspi_clk, "qspi_clk", qspi_parent_names,
	BASE_TYPE_APMU, APMU_QSPI_CLK_RES_CTRL,
	9, 3,
	6, 3, BIT(4), BIT(4), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(qspi_bus_clk, "qspi_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_QSPI_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(dma_clk, "dma_clk", NULL,
	BASE_TYPE_APMU, APMU_DMA_CLK_RES_CTRL,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const aes_parent_names[] = {
	"pll1_d12_204p8", "pll1_d24_102p4"
};
static KY_CCU_MUX_GATE(aes_clk, "aes_clk", aes_parent_names,
	BASE_TYPE_APMU, APMU_AES_CLK_RES_CTRL,
	6, 1, BIT(5), BIT(5), 0x0,
	0);
static const char * const vpu_parent_names[] = {
	"pll1_d4_614p4", "pll1_d5_491p52", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll3_d6", "pll2_d3", "pll2_d4", "pll2_d5"
};
static KY_CCU_DIV_FC_MUX_GATE(vpu_clk, "vpu_clk", vpu_parent_names,
	BASE_TYPE_APMU, APMU_VPU_CLK_RES_CTRL,
	13, 3, BIT(21),
	10, 3,
	BIT(3), BIT(3), 0x0,
	0);
static const char * const gpu_parent_names[] = {
	"pll1_d4_614p4", "pll1_d5_491p52", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll3_d6", "pll2_d3", "pll2_d4", "pll2_d5"
};
static KY_CCU_DIV_FC_MUX_GATE(gpu_clk, "gpu_clk", gpu_parent_names,
	BASE_TYPE_APMU, APMU_GPU_CLK_RES_CTRL,
	12, 3, BIT(15),
	18, 3,
	BIT(4), BIT(4), 0x0,
	0);
static const char * const emmc_parent_names[] = {
	"pll1_d6_409p6", "pll1_d4_614p4", "pll1_d52_47p26", "pll1_d3_819p2"
};
static KY_CCU_DIV_FC_MUX_GATE(emmc_clk, "emmc_clk", emmc_parent_names,
	BASE_TYPE_APMU, APMU_PMUA_EM_CLK_RES_CTRL,
	8, 3, BIT(11),
	6, 2,
	0x18, 0x18, 0x0,
	0);
static KY_CCU_DIV_GATE(emmc_x_clk, "emmc_x_clk", "pll1_d2_1228p8",
	BASE_TYPE_APMU, APMU_PMUA_EM_CLK_RES_CTRL,
	12, 3, BIT(15), BIT(15), 0x0,
	0);
static const char * const audio_parent_names[] = {
	 "pll1_aud_245p7", "pll1_d8_307p2", "pll1_d6_409p6"
};
static KY_CCU_DIV_FC_MUX_GATE(audio_clk, "audio_clk", audio_parent_names,
	BASE_TYPE_APMU, APMU_AUDIO_CLK_RES_CTRL,
	4, 3, BIT(15),
	7, 3,
	BIT(12), BIT(12), 0x0,
	0);
static const char * const hdmi_parent_names[] = {
	 "pll1_d6_409p6", "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d8_307p2"
};
static KY_CCU_DIV_FC_MUX_GATE(hdmi_mclk, "hdmi_mclk", hdmi_parent_names,
	BASE_TYPE_APMU, APMU_HDMI_CLK_RES_CTRL,
	1, 4, BIT(29),
	5, 3,
	BIT(0), BIT(0), 0x0,
	0);
static const char * const cci550_parent_names[] = {
	 "pll1_d5_491p52", "pll1_d4_614p4", "pll1_d3_819p2", "pll2_d3"
};
static KY_CCU_DIV_FC_MUX(cci550_clk, "cci550_clk", cci550_parent_names,
	BASE_TYPE_APMU, APMU_CCI550_CLK_CTRL,
	8, 3, BIT(12),
	0, 2,
	0);
static const char * const pmua_aclk_parent_names[] = {
	 "pll1_d10_245p76", "pll1_d8_307p2"
};
static KY_CCU_DIV_FC_MUX(pmua_aclk, "pmua_aclk", pmua_aclk_parent_names,
	BASE_TYPE_APMU, APMU_ACLK_CLK_CTRL,
	1, 2, BIT(4),
	0, 1,
	0);
static const char * const cpu_c0_hi_parent_names[] = {
	 "pll3_d2", "pll3_d1"
};
static KY_CCU_MUX(cpu_c0_hi_clk, "cpu_c0_hi_clk", cpu_c0_hi_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	13, 1,
	0);
static const char * const cpu_c0_parent_names[] = { "pll1_d4_614p4", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll1_d5_491p52", "pll1_d2_1228p8", "pll3_d3", "pll2_d3", "cpu_c0_hi_clk"
};
static KY_CCU_MUX_FC(cpu_c0_core_clk, "cpu_c0_core_clk", cpu_c0_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	BIT(12),
	0, 3,
	0);
static KY_CCU_DIV(cpu_c0_ace_clk, "cpu_c0_ace_clk", "cpu_c0_core_clk",
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	6, 3,
	0);
static KY_CCU_DIV(cpu_c0_tcm_clk, "cpu_c0_tcm_clk", "cpu_c0_core_clk",
	BASE_TYPE_APMU, APMU_CPU_C0_CLK_CTRL,
	9, 3,
	0);
static const char * const cpu_c1_hi_parent_names[] = {
	 "pll3_d2", "pll3_d1"
};
static KY_CCU_MUX(cpu_c1_hi_clk, "cpu_c1_hi_clk", cpu_c1_hi_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C1_CLK_CTRL,
	13, 1,
	0);
static const char * const cpu_c1_parent_names[] = { "pll1_d4_614p4", "pll1_d3_819p2", "pll1_d6_409p6",
	"pll1_d5_491p52", "pll1_d2_1228p8", "pll3_d3", "pll2_d3", "cpu_c1_hi_clk"
};
static KY_CCU_MUX_FC(cpu_c1_pclk, "cpu_c1_pclk", cpu_c1_parent_names,
	BASE_TYPE_APMU, APMU_CPU_C1_CLK_CTRL,
	BIT(12),
	0, 3,
	0);
static KY_CCU_DIV(cpu_c1_ace_clk, "cpu_c1_ace_clk", "cpu_c1_pclk",
	BASE_TYPE_APMU, APMU_CPU_C1_CLK_CTRL,
	6, 3,
	0);
static KY_CCU_GATE_NO_PARENT(pcie0_clk, "pcie0_clk", NULL,
	BASE_TYPE_APMU, APMU_PCIE_CLK_RES_CTRL_0,
	0x7, 0x7, 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(pcie1_clk, "pcie1_clk", NULL,
	BASE_TYPE_APMU, APMU_PCIE_CLK_RES_CTRL_1,
	0x7, 0x7, 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(pcie2_clk, "pcie2_clk", NULL,
	BASE_TYPE_APMU, APMU_PCIE_CLK_RES_CTRL_2,
	0x7, 0x7, 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(emac0_bus_clk, "emac0_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_EMAC0_CLK_RES_CTRL,
	BIT(0), BIT(0), 0x0,
	0);
static KY_CCU_GATE(emac0_ptp_clk, "emac0_ptp_clk", "pll2_d6",
	BASE_TYPE_APMU, APMU_EMAC0_CLK_RES_CTRL,
	BIT(15), BIT(15), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(emac1_bus_clk, "emac1_bus_clk", NULL,
	BASE_TYPE_APMU, APMU_EMAC1_CLK_RES_CTRL,
	BIT(0), BIT(0), 0x0,
	0);
static KY_CCU_GATE(emac1_ptp_clk, "emac1_ptp_clk", "pll2_d6",
	BASE_TYPE_APMU, APMU_EMAC1_CLK_RES_CTRL,
	BIT(15), BIT(15), 0x0,
	0);

//apbc2
static const char * const uart1_sec_parent_names[] = {
	"pll1_m3d128_57p6", "slow_uart1_14p74", "slow_uart2_48"
};
static KY_CCU_MUX_GATE(uart1_sec_clk, "uart1_sec_clk", uart1_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_UART1_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);

static const char *ssp2_sec_parent_names[] = { "pll1_d384_6p4", "pll1_d192_12p8", "pll1_d96_25p6",
	"pll1_d48_51p2", "pll1_d768_3p2", "pll1_d1536_1p6", "pll1_d3072_0p8"
};
static KY_CCU_MUX_GATE(ssp2_sec_clk, "ssp2_sec_clk", ssp2_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_SSP2_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static const char *twsi3_sec_parent_names[] = {
	"pll1_d78_31p5", "pll1_d48_51p2", "pll1_d40_61p44"
};
static KY_CCU_MUX_GATE(twsi3_sec_clk, "twsi3_sec_clk", twsi3_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_TWSI3_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_GATE(rtc_sec_clk, "rtc_sec_clk", "clk_32k",
	BASE_TYPE_APBC2, APBC2_RTC_CLK_RST,
	0x83, 0x83, 0x0, 0);
static const char *timer_sec_parent_names[] = {
	"pll1_d192_12p8", "clk_32k", "pll1_d384_6p4", "vctcxo_3", "vctcxo_1"
};
static KY_CCU_MUX_GATE(timers0_sec_clk, "timers0_sec_clk", timer_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_TIMERS0_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static const char *kpc_sec_parent_names[] = {
	"pll1_d192_12p8", "clk_32k", "pll1_d384_6p4", "vctcxo_3", "vctcxo_1"
};
static KY_CCU_MUX_GATE(kpc_sec_clk, "kpc_sec_clk", kpc_sec_parent_names,
	BASE_TYPE_APBC2, APBC2_KPC_CLK_RST,
	4, 3, 0x3, 0x3, 0x0,
	0);
static KY_CCU_GATE(gpio_sec_clk, "gpio_sec_clk", "vctcxo_24",
	BASE_TYPE_APBC2, APBC2_GPIO_CLK_RST,
	0x3, 0x3, 0x0,
	0);

static const char * const apb_parent_names[] = {
	"pll1_d96_25p6", "pll1_d48_51p2", "pll1_d96_25p6", "pll1_d24_102p4"
};
static KY_CCU_MUX(apb_clk, "apb_clk", apb_parent_names,
	BASE_TYPE_MPMU, MPMU_APBCSCR,
	0, 2, 0);
//rcpu
static const char *rhdmi_audio_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7"
};
static KY_CCU_DIV_MUX_GATE(rhdmi_audio_clk, "rhdmi_audio_clk", rhdmi_audio_parent_names,
	BASE_TYPE_RCPU, RCPU_HDMI_CLK_RST,
	4, 11, 16, 2,
	0x6, 0x6, 0x0,
	0);

static const char *rcan_parent_names[] = {
	"pll3_20", "pll3_40", "pll3_80"
};
static KY_CCU_DIV_MUX_GATE(rcan_clk, "rcan_clk", rcan_parent_names,
	BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(rcan_bus_clk, "rcan_bus_clk", NULL,
	BASE_TYPE_RCPU, RCPU_CAN_CLK_RST,
	BIT(2), BIT(2), 0x0, 0);
//rcpu2
static const char *rpwm_parent_names[] = {
	"pll1_aud_245p7", "pll1_aud_24p5"
};
static KY_CCU_DIV_MUX_GATE(rpwm0_clk, "rpwm0_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM0_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm1_clk, "rpwm1_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM1_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm2_clk, "rpwm2_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM2_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm3_clk, "rpwm3_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM3_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm4_clk, "rpwm4_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM4_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm5_clk, "rpwm5_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM5_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm6_clk, "rpwm6_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM6_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm7_clk, "rpwm7_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM7_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm8_clk, "rpwm8_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM8_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);
static KY_CCU_DIV_MUX_GATE(rpwm9_clk, "rpwm9_clk", rpwm_parent_names,
	BASE_TYPE_RCPU2, RCPU2_PWM9_CLK_RST,
	8, 11, 4, 2,
	BIT(1), BIT(1), 0x0,
	0);

static const char *ri2c_parent_names[] = {
	"pll1_d40_61p44", "pll1_d96_25p6", "pll1_d192_12p8", "vctcxo_3"
};
static KY_CCU_DIV_MUX_GATE(ri2c0_clk, "ri2c0_clk", ri2c_parent_names,
	BASE_TYPE_RCPU, RCPU_I2C0_CLK_RST,
	8, 11, 4, 2,
	0x6, 0x6, 0x0,
	0);

static const char *rssp0_parent_names[] = {
	"pll1_d40_61p44", "pll1_d96_25p6", "pll1_d192_12p8", "vctcxo_3"
};
static KY_CCU_DIV_MUX_GATE(rssp0_clk, "rssp0_clk", rssp0_parent_names,
	BASE_TYPE_RCPU, RCPU_SSP0_CLK_RST,
	8, 11, 4, 2,
	0x6, 0x6, 0x0,
	0);
static KY_CCU_GATE_NO_PARENT(rir_clk, "rir_clk", NULL,
	BASE_TYPE_RCPU, RCPU_IR_CLK_RST,
	BIT(2), BIT(2), 0x0,
	0);
static const char *ruart0_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "vctcxo_24", "vctcxo_3"
};
static KY_CCU_DIV_MUX_GATE(ruart0_clk, "ruart0_clk", ruart0_parent_names,
	BASE_TYPE_RCPU, RCPU_UART0_CLK_RST,
	8, 11, 4, 2,
	0x6, 0x6, 0x0,
	0);
static const char *ruart1_parent_names[] = {
	"pll1_aud_24p5", "pll1_aud_245p7", "vctcxo_24", "vctcxo_3"
};
static KY_CCU_DIV_MUX_GATE(ruart1_clk, "ruart1_clk", ruart1_parent_names,
	BASE_TYPE_RCPU, RCPU_UART1_CLK_RST,
	8, 11, 4, 2,
	0x6, 0x6, 0x0,
	0);
static struct clk_hw_onecell_data ky_x1_hw_clks = {
	.hws	= {
		[CLK_PLL2]		= &pll2.common.hw,
		[CLK_PLL3]		= &pll3.common.hw,
		[CLK_PLL1_D2]		= &pll1_d2.common.hw,
		[CLK_PLL1_D3]		= &pll1_d3.common.hw,
		[CLK_PLL1_D4]		= &pll1_d4.common.hw,
		[CLK_PLL1_D5]		= &pll1_d5.common.hw,
		[CLK_PLL1_D6]		= &pll1_d6.common.hw,
		[CLK_PLL1_D7]		= &pll1_d7.common.hw,
		[CLK_PLL1_D8]		= &pll1_d8.common.hw,
		[CLK_PLL1_D11]		= &pll1_d11_223p4.common.hw,
		[CLK_PLL1_D13]		= &pll1_d13_189.common.hw,
		[CLK_PLL1_D23]		= &pll1_d23_106p8.common.hw,
		[CLK_PLL1_D64]		= &pll1_d64_38p4.common.hw,
		[CLK_PLL1_D10_AUD]	= &pll1_aud_245p7.common.hw,
		[CLK_PLL1_D100_AUD]	= &pll1_aud_24p5.common.hw,
		[CLK_PLL2_D1]		= &pll2_d1.common.hw,
		[CLK_PLL2_D2]		= &pll2_d2.common.hw,
		[CLK_PLL2_D3]		= &pll2_d3.common.hw,
		[CLK_PLL2_D4]		= &pll2_d4.common.hw,
		[CLK_PLL2_D5]		= &pll2_d5.common.hw,
		[CLK_PLL2_D6]		= &pll2_d6.common.hw,
		[CLK_PLL2_D7]		= &pll2_d7.common.hw,
		[CLK_PLL2_D8]		= &pll2_d8.common.hw,
		[CLK_PLL3_D1]		= &pll3_d1.common.hw,
		[CLK_PLL3_D2]		= &pll3_d2.common.hw,
		[CLK_PLL3_D3]		= &pll3_d3.common.hw,
		[CLK_PLL3_D4]		= &pll3_d4.common.hw,
		[CLK_PLL3_D5]		= &pll3_d5.common.hw,
		[CLK_PLL3_D6]		= &pll3_d6.common.hw,
		[CLK_PLL3_D7]		= &pll3_d7.common.hw,
		[CLK_PLL3_D8]		= &pll3_d8.common.hw,
		[CLK_PLL3_80]		= &pll3_80.common.hw,
		[CLK_PLL3_40]		= &pll3_40.common.hw,
		[CLK_PLL3_20]		= &pll3_20.common.hw,
		[CLK_PLL1_307P2]	= &pll1_d8_307p2.common.hw,
		[CLK_PLL1_76P8]		= &pll1_d32_76p8.common.hw,
		[CLK_PLL1_61P44]	= &pll1_d40_61p44.common.hw,
		[CLK_PLL1_153P6]	= &pll1_d16_153p6.common.hw,
		[CLK_PLL1_102P4]	= &pll1_d24_102p4.common.hw,
		[CLK_PLL1_51P2]		= &pll1_d48_51p2.common.hw,
		[CLK_PLL1_51P2_AP]	= &pll1_d48_51p2_ap.common.hw,
		[CLK_PLL1_57P6]		= &pll1_m3d128_57p6.common.hw,
		[CLK_PLL1_25P6]		= &pll1_d96_25p6.common.hw,
		[CLK_PLL1_12P8]		= &pll1_d192_12p8.common.hw,
		[CLK_PLL1_12P8_WDT]	= &pll1_d192_12p8_wdt.common.hw,
		[CLK_PLL1_6P4]		= &pll1_d384_6p4.common.hw,
		[CLK_PLL1_3P2]		= &pll1_d768_3p2.common.hw,
		[CLK_PLL1_1P6]		= &pll1_d1536_1p6.common.hw,
		[CLK_PLL1_0P8]		= &pll1_d3072_0p8.common.hw,
		[CLK_PLL1_351]		= &pll1_d7_351p08.common.hw,
		[CLK_PLL1_409P6]	= &pll1_d6_409p6.common.hw,
		[CLK_PLL1_204P8]	= &pll1_d12_204p8.common.hw,
		[CLK_PLL1_491]		= &pll1_d5_491p52.common.hw,
		[CLK_PLL1_245P76]	= &pll1_d10_245p76.common.hw,
		[CLK_PLL1_614]		= &pll1_d4_614p4.common.hw,
		[CLK_PLL1_47P26]	= &pll1_d52_47p26.common.hw,
		[CLK_PLL1_31P5]		= &pll1_d78_31p5.common.hw,
		[CLK_PLL1_819]		= &pll1_d3_819p2.common.hw,
		[CLK_PLL1_1228]		= &pll1_d2_1228p8.common.hw,
		[CLK_SLOW_UART1]	= &slow_uart1_14p74.common.hw,
		[CLK_SLOW_UART2]	= &slow_uart2_48.common.hw,
		[CLK_UART1]		= &uart1_clk.common.hw,
		[CLK_UART2]		= &uart2_clk.common.hw,
		[CLK_UART3]		= &uart3_clk.common.hw,
		[CLK_UART4]		= &uart4_clk.common.hw,
		[CLK_UART5]		= &uart5_clk.common.hw,
		[CLK_UART6]		= &uart6_clk.common.hw,
		[CLK_UART7]		= &uart7_clk.common.hw,
		[CLK_UART8]		= &uart8_clk.common.hw,
		[CLK_UART9]		= &uart9_clk.common.hw,
		[CLK_GPIO]		= &gpio_clk.common.hw,
		[CLK_PWM0]		= &pwm0_clk.common.hw,
		[CLK_PWM1]		= &pwm1_clk.common.hw,
		[CLK_PWM2]		= &pwm2_clk.common.hw,
		[CLK_PWM3]		= &pwm3_clk.common.hw,
		[CLK_PWM4]		= &pwm4_clk.common.hw,
		[CLK_PWM5]		= &pwm5_clk.common.hw,
		[CLK_PWM6]		= &pwm6_clk.common.hw,
		[CLK_PWM7]		= &pwm7_clk.common.hw,
		[CLK_PWM8]		= &pwm8_clk.common.hw,
		[CLK_PWM9]		= &pwm9_clk.common.hw,
		[CLK_PWM10]		= &pwm10_clk.common.hw,
		[CLK_PWM11]		= &pwm11_clk.common.hw,
		[CLK_PWM12]		= &pwm12_clk.common.hw,
		[CLK_PWM13]		= &pwm13_clk.common.hw,
		[CLK_PWM14]		= &pwm14_clk.common.hw,
		[CLK_PWM15]		= &pwm15_clk.common.hw,
		[CLK_PWM16]		= &pwm16_clk.common.hw,
		[CLK_PWM17]		= &pwm17_clk.common.hw,
		[CLK_PWM18]		= &pwm18_clk.common.hw,
		[CLK_PWM19]		= &pwm19_clk.common.hw,
		[CLK_SSP3]		= &ssp3_clk.common.hw,
		[CLK_RTC]		= &rtc_clk.common.hw,
		[CLK_TWSI0]		= &twsi0_clk.common.hw,
		[CLK_TWSI1]		= &twsi1_clk.common.hw,
		[CLK_TWSI2]		= &twsi2_clk.common.hw,
		[CLK_TWSI4]		= &twsi4_clk.common.hw,
		[CLK_TWSI5]		= &twsi5_clk.common.hw,
		[CLK_TWSI6]		= &twsi6_clk.common.hw,
		[CLK_TWSI7]		= &twsi7_clk.common.hw,
		[CLK_TWSI8]		= &twsi8_clk.common.hw,
		[CLK_TIMERS1]		= &timers1_clk.common.hw,
		[CLK_TIMERS2]		= &timers2_clk.common.hw,
		[CLK_AIB]		= &aib_clk.common.hw,
		[CLK_ONEWIRE]		= &onewire_clk.common.hw,
		[CLK_SSPA0]		= &sspa0_clk.common.hw,
		[CLK_SSPA1]		= &sspa1_clk.common.hw,
		[CLK_DRO]		= &dro_clk.common.hw,
		[CLK_IR]		= &ir_clk.common.hw,
		[CLK_TSEN]		= &tsen_clk.common.hw,
		[CLK_IPC_AP2AUD]	= &ipc_ap2aud_clk.common.hw,
		[CLK_CAN0]		= &can0_clk.common.hw,
		[CLK_CAN0_BUS]		= &can0_bus_clk.common.hw,
		[CLK_WDT] 		= &wdt_clk.common.hw,
		[CLK_RIPC] 		= &ripc_clk.common.hw,
		[CLK_JPG] 		= &jpg_clk.common.hw,
		[CLK_JPF_4KAFBC]	= &jpg_4kafbc_clk.common.hw,
		[CLK_JPF_2KAFBC]	= &jpg_2kafbc_clk.common.hw,
		[CLK_CCIC2PHY]		= &ccic2phy_clk.common.hw,
		[CLK_CCIC3PHY]		= &ccic3phy_clk.common.hw,
		[CLK_CSI]		= &csi_clk.common.hw,
		[CLK_CAMM0]		= &camm0_clk.common.hw,
		[CLK_CAMM1]		= &camm1_clk.common.hw,
		[CLK_CAMM2]		= &camm2_clk.common.hw,
		[CLK_ISP_CPP]		= &isp_cpp_clk.common.hw,
		[CLK_ISP_BUS]		= &isp_bus_clk.common.hw,
		[CLK_ISP]		= &isp_clk.common.hw,
		[CLK_DPU_MCLK]		= &dpu_mclk.common.hw,
		[CLK_DPU_ESC]		= &dpu_esc_clk.common.hw,
		[CLK_DPU_BIT]		= &dpu_bit_clk.common.hw,
		[CLK_DPU_PXCLK]		= &dpu_pxclk.common.hw,
		[CLK_DPU_HCLK]		= &dpu_hclk.common.hw,
		[CLK_DPU_SPI]		= &dpu_spi_clk.common.hw,
		[CLK_DPU_SPI_HBUS]	= &dpu_spi_hbus_clk.common.hw,
		[CLK_DPU_SPIBUS]	= &dpu_spi_bus_clk.common.hw,
		[CLK_SPU_SPI_ACLK]	= &dpu_spi_aclk.common.hw,
		[CLK_V2D]		= &v2d_clk.common.hw,
		[CLK_CCIC_4X]		= &ccic_4x_clk.common.hw,
		[CLK_CCIC1PHY]		= &ccic1phy_clk.common.hw,
		[CLK_SDH_AXI]		= &sdh_axi_aclk.common.hw,
		[CLK_SDH0] 		= &sdh0_clk.common.hw,
		[CLK_SDH1]		= &sdh1_clk.common.hw,
		[CLK_SDH2]		= &sdh2_clk.common.hw,
		[CLK_USB_P1]		= &usb_p1_aclk.common.hw,
		[CLK_USB_AXI]		= &usb_axi_clk.common.hw,
		[CLK_USB30]		= &usb30_clk.common.hw,
		[CLK_QSPI]		= &qspi_clk.common.hw,
		[CLK_QSPI_BUS]		= &qspi_bus_clk.common.hw,
		[CLK_DMA]		= &dma_clk.common.hw,
		[CLK_AES]		= &aes_clk.common.hw,
		[CLK_VPU]		= &vpu_clk.common.hw,
		[CLK_GPU]		= &gpu_clk.common.hw,
		[CLK_EMMC]		= &emmc_clk.common.hw,
		[CLK_EMMC_X]		= &emmc_x_clk.common.hw,
		[CLK_AUDIO]		= &audio_clk.common.hw,
		[CLK_HDMI]		= &hdmi_mclk.common.hw,
		[CLK_CCI550]		= &cci550_clk.common.hw,
		[CLK_PMUA_ACLK]		= &pmua_aclk.common.hw,
		[CLK_CPU_C0_HI]		= &cpu_c0_hi_clk.common.hw,
		[CLK_CPU_C0_CORE]	= &cpu_c0_core_clk.common.hw,
		[CLK_CPU_C0_ACE]	= &cpu_c0_ace_clk.common.hw,
		[CLK_CPU_C0_TCM]	= &cpu_c0_tcm_clk.common.hw,
		[CLK_CPU_C1_HI]		= &cpu_c1_hi_clk.common.hw,
		[CLK_CPU_C1_CORE]	= &cpu_c1_pclk.common.hw,
		[CLK_CPU_C1_ACE]	= &cpu_c1_ace_clk.common.hw,
		[CLK_PCIE0]		= &pcie0_clk.common.hw,
		[CLK_PCIE1]		= &pcie1_clk.common.hw,
		[CLK_PCIE2]		= &pcie2_clk.common.hw,
		[CLK_EMAC0_BUS]		= &emac0_bus_clk.common.hw,
		[CLK_EMAC0_PTP]		= &emac0_ptp_clk.common.hw,
		[CLK_EMAC1_BUS]		= &emac1_bus_clk.common.hw,
		[CLK_EMAC1_PTP]		= &emac1_ptp_clk.common.hw,
		[CLK_SEC_UART1]		= &uart1_sec_clk.common.hw,
		[CLK_SEC_SSP2]		= &ssp2_sec_clk.common.hw,
		[CLK_SEC_TWSI3]		= &twsi3_sec_clk.common.hw,
		[CLK_SEC_RTC]		= &rtc_sec_clk.common.hw,
		[CLK_SEC_TIMERS0]	= &timers0_sec_clk.common.hw,
		[CLK_SEC_KPC]		= &kpc_sec_clk.common.hw,
		[CLK_SEC_GPIO]		= &gpio_sec_clk.common.hw,
		[CLK_APB]		= &apb_clk.common.hw,
		[CLK_SLOW_UART]		= &slow_uart.common.hw,
		[CLK_I2S_SYSCLK]	= &i2s_sysclk.common.hw,
		[CLK_I2S_BCLK]		= &i2s_bclk.common.hw,
		[CLK_RCPU_HDMIAUDIO]	= &rhdmi_audio_clk.common.hw,
		[CLK_RCPU_CAN] 		= &rcan_clk.common.hw,
		[CLK_RCPU_CAN_BUS]	= &rcan_bus_clk.common.hw,
		[CLK_RCPU_I2C0] 	= &ri2c0_clk.common.hw,
		[CLK_RCPU_SSP0] 	= &rssp0_clk.common.hw,
		[CLK_RCPU_IR] 		= &rir_clk.common.hw,
		[CLK_RCPU_UART0] 	= &ruart0_clk.common.hw,
		[CLK_RCPU_UART1] 	= &ruart1_clk.common.hw,
		[CLK_DPLL1] 	= &dpll1.common.hw,
		[CLK_DPLL2] 	= &dpll2.common.hw,
		[CLK_DFC_LVL0] 	= &dfc_lvl0.common.hw,
		[CLK_DFC_LVL1] 	= &dfc_lvl1.common.hw,
		[CLK_DFC_LVL2] 	= &dfc_lvl2.common.hw,
		[CLK_DFC_LVL3] 	= &dfc_lvl3.common.hw,
		[CLK_DFC_LVL4] 	= &dfc_lvl4.common.hw,
		[CLK_DFC_LVL5] 	= &dfc_lvl5.common.hw,
		[CLK_DFC_LVL6] 	= &dfc_lvl6.common.hw,
		[CLK_DFC_LVL7] 	= &dfc_lvl7.common.hw,
		[CLK_DDR] 	= &ddr.common.hw,
		[CLK_RCPU2_PWM0] 	= &rpwm0_clk.common.hw,
		[CLK_RCPU2_PWM1] 	= &rpwm1_clk.common.hw,
		[CLK_RCPU2_PWM2] 	= &rpwm2_clk.common.hw,
		[CLK_RCPU2_PWM3] 	= &rpwm3_clk.common.hw,
		[CLK_RCPU2_PWM4] 	= &rpwm4_clk.common.hw,
		[CLK_RCPU2_PWM5] 	= &rpwm5_clk.common.hw,
		[CLK_RCPU2_PWM6] 	= &rpwm6_clk.common.hw,
		[CLK_RCPU2_PWM7] 	= &rpwm7_clk.common.hw,
		[CLK_RCPU2_PWM8] 	= &rpwm8_clk.common.hw,
		[CLK_RCPU2_PWM9] 	= &rpwm9_clk.common.hw,
	},
	.num = CLK_MAX_NO,
};

static struct clk_hw_table bootup_enable_clk_table[] = {
	{"pll1_d8_307p2", 	CLK_PLL1_307P2},
	{"pll1_d6_409p6", 	CLK_PLL1_409P6},
	{"pll1_d5_491p52", 	CLK_PLL1_491},
	{"pll1_d4_614p4", 	CLK_PLL1_614},
	{"pll1_d3_819p2", 	CLK_PLL1_819},
	{"pll1_d2_1228p8", 	CLK_PLL1_1228},
	{"pll1_d10_245p76",	CLK_PLL1_245P76},
	{"pll1_d48_51p2", 	CLK_PLL1_51P2},
	{"pll1_d48_51p2_ap",	CLK_PLL1_51P2_AP},
	{"pll1_d96_25p6", 	CLK_PLL1_25P6},
	{"pll3_d1", 	CLK_PLL3_D1},
	{"pll3_d2", 	CLK_PLL3_D2},
	{"pll3_d3", 	CLK_PLL3_D3},
	{"pll2_d3", 	CLK_PLL2_D3},
	{"apb_clk", 	CLK_APB},
	{"pmua_aclk", 	CLK_PMUA_ACLK},
};

void ky_clocks_enable(struct clk_hw_table *tbl, int tbl_size)
{
	int i;
	struct clk *clk;

	for (i = 0; i < tbl_size; i++) {
		clk = clk_hw_get_clk(ky_x1_hw_clks.hws[tbl[i].clk_hw_id], tbl[i].name);
		if (!IS_ERR_OR_NULL(clk))
			clk_prepare_enable(clk);
		else
			pr_err("%s : can't find clk %s\n", __func__, tbl[i].name);
	}
}

unsigned long ky_x1_ddr_freq_tbl[MAX_FREQ_LV + 1] = {0};
void ky_fill_ddr_freq_tbl(void)
{
	int i;
	struct clk *clk;

	for (i = 0; i < ARRAY_SIZE(ky_x1_ddr_freq_tbl); i++) {
		clk = clk_hw_get_clk(ky_x1_hw_clks.hws[CLK_DFC_LVL0 + i], ddr_clk_parents[i]);

		if (!IS_ERR_OR_NULL(clk))
			ky_x1_ddr_freq_tbl[i] = clk_get_rate(clk);
		else
			pr_err("%s : can't find clk %s\n", __func__, ddr_clk_parents[i]);
	}
}

int ccu_common_init(struct clk_hw * hw, struct ky_x1_clk *clk_info)
{
	struct ccu_common *common = hw_to_ccu_common(hw);
	struct ccu_pll *pll = hw_to_ccu_pll(hw);

	if (!common)
		return -1;

	common->lock = &g_cru_lock;

	switch(common->base_type){
	case BASE_TYPE_MPMU:
		common->base = clk_info->mpmu_base;
		break;
	case BASE_TYPE_APMU:
		common->base = clk_info->apmu_base;
		break;
	case BASE_TYPE_APBC:
		common->base = clk_info->apbc_base;
		break;
	case BASE_TYPE_APBS:
		common->base = clk_info->apbs_base;
		break;
	case BASE_TYPE_CIU:
		common->base = clk_info->ciu_base;
		break;
	case BASE_TYPE_DCIU:
		common->base = clk_info->dciu_base;
		break;
	case BASE_TYPE_DDRC:
		common->base = clk_info->ddrc_base;
		break;
	case BASE_TYPE_AUDC:
		common->base = clk_info->audio_ctrl_base;
		break;
	case BASE_TYPE_APBC2:
		common->base = clk_info->apbc2_base;
		break;
	case BASE_TYPE_RCPU:
		common->base = clk_info->rcpu_base;
		break;
	case BASE_TYPE_RCPU2:
		common->base = clk_info->rcpu2_base;
		break;
	default:
		common->base = clk_info->apbc_base;
		break;

	}
	if(common->is_pll)
		pll->pll.lock_base = clk_info->mpmu_base;

	return 0;
}

int ky_ccu_probe(struct device_node *node, struct ky_x1_clk *clk_info,
		    struct clk_hw_onecell_data *hw_clks)
{
	int i, ret;
	for (i = 0; i < hw_clks->num ; i++) {
		struct clk_hw *hw = hw_clks->hws[i];
		const char *name;
		if (!hw)
			continue;
		if (!hw->init)
			continue;

		ccu_common_init(hw, clk_info);
		name = hw->init->name;

		ret = of_clk_hw_register(node, hw);
		if (ret) {
			pr_err("Couldn't register clock %d - %s\n", i, name);
			goto err_clk_unreg;
		}
	}
	ret = of_clk_add_hw_provider(node, of_clk_hw_onecell_get,
				     hw_clks);
	if (ret)
		goto err_clk_unreg;

	//enable some clocks
	ky_clocks_enable(bootup_enable_clk_table, ARRAY_SIZE(bootup_enable_clk_table));
	//fill ddr frequency table
	ky_fill_ddr_freq_tbl();

	return 0;

err_clk_unreg:
	while (--i >= 0) {
		struct clk_hw *hw = hw_clks->hws[i];
		if (!hw)
			continue;
		clk_hw_unregister(hw);
	}
	LOG_INFO("clock init fail");
	return ret;
}

static void ky_x1_ccu_probe(struct device_node *np)
{
	int ret;
	struct ky_x1_clk *clk_info;
	struct clk_hw_onecell_data *hw_clks = &ky_x1_hw_clks;

	//LOG_INFO("init clock");
	if (of_device_is_compatible(np, "ky,x1-clock")){
		clk_info = &x1_clock_controller;

		clk_info->mpmu_base = of_iomap(np, 0);
		if (!clk_info->mpmu_base) {
			pr_err("failed to map mpmu registers\n");
			goto out;
		}

		clk_info->apmu_base = of_iomap(np, 1);
		if (!clk_info->apmu_base) {
			pr_err("failed to map apmu registers\n");
			goto out;
		}

		clk_info->apbc_base = of_iomap(np, 2);
		if (!clk_info->apbc_base) {
			pr_err("failed to map apbc registers\n");
			goto out;
		}

		clk_info->apbs_base = of_iomap(np, 3);
		if (!clk_info->apbs_base) {
			pr_err("failed to map apbs registers\n");
			goto out;
		}

		clk_info->ciu_base = of_iomap(np, 4);
		if (!clk_info->ciu_base) {
			pr_err("failed to map ciu registers\n");
			goto out;
		}

		clk_info->dciu_base = of_iomap(np, 5);
		if (!clk_info->dciu_base) {
			pr_err("failed to map dragon ciu registers\n");
			goto out;
		}

		clk_info->ddrc_base = of_iomap(np, 6);
		if (!clk_info->ddrc_base) {
			pr_err("failed to map ddrc registers\n");
			goto out;
		}

		clk_info->apbc2_base = of_iomap(np, 7);
		if (!clk_info->apbc2_base) {
			pr_err("failed to map apbc2 registers\n");
			goto out;
		}

		clk_info->rcpu_base = of_iomap(np, 8);
		if (!clk_info->rcpu_base) {
			pr_err("failed to map rcpu registers\n");
			goto out;
		}

		clk_info->rcpu2_base = of_iomap(np, 9);
		if (!clk_info->rcpu2_base) {
			pr_err("failed to map rcpu2 registers\n");
			goto out;
		}
	} else {
		pr_err("not ky,x1-clock\n");
		goto out;
	}
	ret = ky_ccu_probe(np, clk_info, hw_clks);
	//LOG_INFO("init clock finish");
	if (ret)
		return;
out:
	return;
}

void * ky_get_ddr_freq_tbl(void)
{
	return ky_x1_ddr_freq_tbl;
}
EXPORT_SYMBOL_GPL(ky_get_ddr_freq_tbl);

u32 ky_get_ddr_freq_level(void)
{
	u32 ddr_freq_lvl = 0;

	struct clk_hw *hw = ky_x1_hw_clks.hws[CLK_DDR];
	ddr_freq_lvl = clk_hw_get_parent_index(hw);

	return ddr_freq_lvl;
}
EXPORT_SYMBOL_GPL(ky_get_ddr_freq_level);

int ky_set_ddr_freq_level(u32 level)
{
	int ret = 0;
	struct clk_hw *hw = ky_x1_hw_clks.hws[CLK_DDR];

	if (level < 0 || level > MAX_FREQ_LV)
		return -EINVAL;

	ret = clk_hw_set_parent(hw, clk_hw_get_parent_by_index(hw, level));
	if (ret)
		pr_err("%s : set ddr freq fail\n", __func__);

	return 0;
}
EXPORT_SYMBOL_GPL(ky_set_ddr_freq_level);

CLK_OF_DECLARE(x1_clock, "ky,x1-clock", ky_x1_ccu_probe);

