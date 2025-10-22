// SPDX-License-Identifier: GPL-3.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_nm.h"

#include "ccu-sun55iw3-mcu.h"

/* ccu_des_start */

#define SUN55IW3_PLL_AUDIO_CTRL_REG   0x000c
static struct ccu_sdm_setting pll_audio1_sdm_table[] = {
	{ .rate = 2167603200, .pattern = 0xA000A234, .m = 1, .n = 90 }, /* div2->22.5792 */
	{ .rate = 2359296000, .pattern = 0xA0009BA6, .m = 1, .n = 98 }, /* div2->24.576 */
	{ .rate = 1806336000, .pattern = 0xA000872B, .m = 1, .n = 75 }, /* div5->22.576 */
};

static struct ccu_nm pll_audio1_clk = {
	.enable		= BIT(27) | BIT(29) | BIT(30) | BIT(31),
	.lock		= BIT(28),
	.n		= _SUNXI_CCU_MULT_MIN(8, 8, 12),
	.m		= _SUNXI_CCU_DIV(1, 1), /* output divider */
	.sdm		= _SUNXI_CCU_SDM(pll_audio1_sdm_table, BIT(24),
			0x10, BIT(31)),
	.common		= {
		.reg		= 0x000C,
		.features	= CCU_FEATURE_SIGMA_DELTA_MOD,
		.hw.init	= CLK_HW_INIT("pll-audio1", "dcxo24M",
					      &ccu_nm_ops,
					      CLK_SET_RATE_UNGATE),
	},
};

static CLK_FIXED_FACTOR_HW(pll_audio1_div2_clk, "pll-audio1-div2",
		&pll_audio1_clk.common.hw,
		2, 1, CLK_SET_RATE_PARENT);

static CLK_FIXED_FACTOR_HW(pll_audio1_div5_clk, "pll-audio1-div5",
		&pll_audio1_clk.common.hw,
		5, 1, CLK_SET_RATE_PARENT);

static SUNXI_CCU_M_WITH_GATE(pll_audio_out_clk, "pll-audio-out",
				"pll-audio1-div2", 0x001C,
				0, 5, BIT(31), 0);

static const char * const dsp_parents[] = { "dcxo24M", "osc32k", "rc-16m", "pll-audio1-div5", "pll-audio1-div2", "dsp" };

static SUNXI_CCU_M_WITH_MUX_GATE(dsp_dsp_clk, "dsp_dsp", dsp_parents, 0x0020,
				  0, 5,
				  24, 3,
				  BIT(31), CLK_SET_RATE_NO_REPARENT);

static const char * const i2s_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(i2s0_clk, "i2s0",
					i2s_parents, 0x002C,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(i2s1_clk, "i2s1",
					i2s_parents, 0x0030,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(i2s2_clk, "i2s2",
					i2s_parents, 0x0034,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(i2s3_clk, "i2s3",
					i2s_parents, 0x0038,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const i2s3_asrc_parents[] = { "pll-peri1-300m", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(i2s3_asrc_clk, "i2s3-asrc",
					i2s3_asrc_parents, 0x003C,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(i2s0_bgr_clk, "i2s0-bgr",
			"dcxo24M",
			0x0040, BIT(0), 0);

static SUNXI_CCU_GATE(i2s1_bgr_clk, "i2s1-bgr",
			"dcxo24M",
			0x0040, BIT(1), 0);

static SUNXI_CCU_GATE(i2s2_bgr_clk, "i2s2-bgr",
			"dcxo24M",
			0x0040, BIT(2), 0);

static SUNXI_CCU_GATE(i2s3_bgr_clk, "i2s3-bgr",
			"dcxo24M",
			0x0040, BIT(3), 0);

static const char * const owa_tx_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(owa_tx_clk, "owa-tx",
					owa_tx_parents, 0x0044,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const owa_rx_parents[] = { "pll-peri0-300m", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(owa_rx_clk, "owa-rx",
					owa_rx_parents, 0x0048,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(bus_owa_clk, "bus-owa",
			"dcxo24M",
			0x004C, BIT(0), 0);

static const char * const dmic_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(dmic_clk, "dmic",
					dmic_parents, 0x0050,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
			"dcxo24M",
			0x0054, BIT(0), 0);

static const char * const audio_codec_dac_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(audio_codec_dac_clk, "audio-codec-dac",
					audio_codec_dac_parents, 0x0058,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const audio_codec_adc_parents[] = { "pll-audio0-4x", "pll-audio1-div2", "pll-audio1-div5" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(audio_codec_adc_clk, "audio-codec-adc",
					audio_codec_adc_parents, 0x005C,
					0, 5,	/* M */
					5, 5,	/* N */
					24, 3,	/* mux */
					BIT(31), CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static SUNXI_CCU_GATE(audio_codec_clk, "audio-codec",
		"dcxo24M",
		0x0060, BIT(0), 0);

static SUNXI_CCU_GATE(dsp_msg_clk, "dsp-msg",
		"dcxo24M",
		0x0068, BIT(0), 0);

static SUNXI_CCU_GATE(dsp_cfg_clk, "dsp-cfg",
		"dcxo24M",
		0x006C, BIT(0), 0);

static SUNXI_CCU_GATE(npu_aclk, "npu-aclk",
		"dcxo24M",
		0x0070, BIT(2), 0);

static SUNXI_CCU_GATE(npu_hclk, "npu-hclk",
		"dcxo24M",
		0x0070, BIT(1), 0);

static const char * const mcu_timer_parents[] = { "dcxo24M", "rtc32k", "rc-16m", "r-ahb" };

static struct ccu_div mcu_timer0_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0074,
		.hw.init	= CLK_HW_INIT_PARENTS("mcu-timer0", mcu_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div mcu_timer1_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0078,
		.hw.init	= CLK_HW_INIT_PARENTS("mcu-timer1", mcu_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div mcu_timer2_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x007C,
		.hw.init	= CLK_HW_INIT_PARENTS("mcu-timer2", mcu_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div mcu_timer3_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0080,
		.hw.init	= CLK_HW_INIT_PARENTS("mcu-timer3", mcu_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div mcu_timer4_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0084,
		.hw.init	= CLK_HW_INIT_PARENTS("mcu-timer4", mcu_timer_parents, &ccu_div_ops, 0),
	},
};

static struct ccu_div mcu_timer5_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 2),
	.common		= {
		.reg		= 0x0088,
		.hw.init	= CLK_HW_INIT_PARENTS("mcu-timer5", mcu_timer_parents, &ccu_div_ops, 0),
	},
};

static SUNXI_CCU_GATE(bus_mcu_timer_clk, "bus-mcu-timer",
		"dcxo24M",
		0x008C, BIT(0), 0);

static SUNXI_CCU_GATE(mcu_dma_clk, "mcu-dma",
		"dcxo24M",
		0x0104, BIT(0), 0);

static SUNXI_CCU_GATE(tzma0_clk, "tzma0",
		"dcxo24M",
		0x0108, BIT(0), 0);

static SUNXI_CCU_GATE(tzma1_clk, "tzma1",
		"dcxo24M",
		0x010C, BIT(0), 0);

static SUNXI_CCU_GATE(pubsram_clk, "pubsram",
		"dcxo24M",
		0x0114, BIT(0), 0);

static SUNXI_CCU_GATE(mcu_mclk, "mcu-mclk",
		"dcxo24M",
		0x011C, BIT(1), 0);

static SUNXI_CCU_GATE(dma_mclk, "dma-mclk",
		"dcxo24M",
		0x011C, BIT(0), 0);

static const char * const rv_parents[] = { "dcxo24M", "rtc-32k", "rc-16m" };

static SUNXI_CCU_MUX_WITH_GATE(rv_clk, "rv",
		rv_parents, 0x0120,
		27, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(rv_cfg_clk, "rv-cfg",
		"dcxo24M",
		0x0124, BIT(0), 0);

static SUNXI_CCU_GATE(riscv_msg_clk, "riscv-msg",
		"dcxo24M",
		0x0128, BIT(0), 0);

static const char * const pwm_parents[] = { "dcxo24M", "rtc-32k", "rc-16m" };

static SUNXI_CCU_MUX_WITH_GATE(mcu_pwm_clk, "mcu-pwm",
		pwm_parents, 0x0130,
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(pwm_bgr_clk, "pwm-bgr",
		"dcxo24M",
		0x0134, BIT(0), 0);

static SUNXI_CCU_GATE(ahb_auto_clk, "ahb-auto",
		"dcxo24M",
		0x013C, BIT(24), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun55iw3_mcu_ccu_resets[] = {
	[RST_BUS_MCU_I2S3]			= { 0x0040, BIT(19) },
	[RST_BUS_MCU_I2S2]			= { 0x0040, BIT(18) },
	[RST_BUS_MCU_I2S1]			= { 0x0040, BIT(17) },
	[RST_BUS_MCU_I2S0]			= { 0x0040, BIT(16) },
	[RST_BUS_MCU_OWA]			= { 0x004c, BIT(16) },
	[RST_BUS_MCU_DMIC]			= { 0x0054, BIT(16) },
	[RST_BUS_MCU_AUDIO_CODEC]		= { 0x0060, BIT(16) },
	[RST_BUS_DSP_MSG]			= { 0x0068, BIT(16) },
	[RST_BUS_DSP_CFG]			= { 0x006c, BIT(16) },
	[RST_BUS_MCU_NPU]			= { 0x0070, BIT(16) },
	[RST_BUS_MCU_TIME]			= { 0x008c, BIT(16) },
	[RST_BUS_DSP]				= { 0x0100, BIT(17) },
	[RST_BUS_DSP_DBG]			= { 0x0100, BIT(16) },
	[RST_BUS_MCU_DMA]			= { 0x0104, BIT(16) },
	[RST_BUS_PUBSRAM]			= { 0x0114, BIT(16) },
	[RST_BUS_RV]				= { 0x0124, BIT(18) },
	[RST_BUS_RV_DBG]			= { 0x0124, BIT(17) },
	[RST_BUS_RV_CFG]			= { 0x0124, BIT(16) },
	[RST_BUS_MCU_RV_MSG]			= { 0x0128, BIT(16) },
	[RST_BUS_MCU_PWM]			= { 0x0134, BIT(16) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun55iw3_mcu_hw_clks = {
	.hws    = {
		[CLK_PLL_MCU_AUDIO1]		= &pll_audio1_clk.common.hw,
		[CLK_PLL_MCU_AUDIO1_DIV2]		= &pll_audio1_div2_clk.hw,
		[CLK_PLL_MCU_AUDIO1_DIV5]		= &pll_audio1_div5_clk.hw,
		[CLK_PLL_MCU_AUDIO_OUT]		= &pll_audio_out_clk.common.hw,
		[CLK_DSP_DSP]			= &dsp_dsp_clk.common.hw,
		[CLK_MCU_I2S0]			= &i2s0_clk.common.hw,
		[CLK_MCU_I2S1]			= &i2s1_clk.common.hw,
		[CLK_MCU_I2S2]			= &i2s2_clk.common.hw,
		[CLK_MCU_I2S3]			= &i2s3_clk.common.hw,
		[CLK_MCU_I2S3_ASRC]			= &i2s3_asrc_clk.common.hw,
		[CLK_BUS_MCU_I2S0]			= &i2s0_bgr_clk.common.hw,
		[CLK_BUS_MCU_I2S1]			= &i2s1_bgr_clk.common.hw,
		[CLK_BUS_MCU_I2S2]			= &i2s2_bgr_clk.common.hw,
		[CLK_BUS_MCU_I2S3]			= &i2s3_bgr_clk.common.hw,
		[CLK_MCU_OWA_TX]			= &owa_tx_clk.common.hw,
		[CLK_MCU_OWA_RX]			= &owa_rx_clk.common.hw,
		[CLK_BUS_MCU_OWA]			= &bus_owa_clk.common.hw,
		[CLK_MCU_DMIC]			= &dmic_clk.common.hw,
		[CLK_BUS_MCU_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_MCU_AUDIO_CODEC_DAC]		= &audio_codec_dac_clk.common.hw,
		[CLK_MCU_AUDIO_CODEC_ADC]		= &audio_codec_adc_clk.common.hw,
		[CLK_BUS_MCU_AUDIO_CODEC]		= &audio_codec_clk.common.hw,
		[CLK_BUS_DSP_MSG]			= &dsp_msg_clk.common.hw,
		[CLK_BUS_DSP_CFG]			= &dsp_cfg_clk.common.hw,
		[CLK_BUS_MCU_NPU_ACLK]			= &npu_aclk.common.hw,
		[CLK_BUS_MCU_NPU_HCLK]			= &npu_hclk.common.hw,
		[CLK_MCU_TIMER0]			= &mcu_timer0_clk.common.hw,
		[CLK_MCU_TIMER1]			= &mcu_timer1_clk.common.hw,
		[CLK_MCU_TIMER2]			= &mcu_timer2_clk.common.hw,
		[CLK_MCU_TIMER3]			= &mcu_timer3_clk.common.hw,
		[CLK_MCU_TIMER4]			= &mcu_timer4_clk.common.hw,
		[CLK_MCU_TIMER5]			= &mcu_timer5_clk.common.hw,
		[CLK_BUS_MCU_TIMER]			= &bus_mcu_timer_clk.common.hw,
		[CLK_BUS_MCU_DMA]			= &mcu_dma_clk.common.hw,
		[CLK_BUS_MCU_TZMA0]			= &tzma0_clk.common.hw,
		[CLK_BUS_MCU_TZMA1]			= &tzma1_clk.common.hw,
		[CLK_BUS_PUBSRAM]			= &pubsram_clk.common.hw,
		[CLK_BUS_MCU_MBUS]			= &mcu_mclk.common.hw,
		[CLK_BUS_MCU_DMA_MBUS]			= &dma_mclk.common.hw,
		[CLK_BUS_RV]			= &rv_clk.common.hw,
		[CLK_BUS_RV_CFG]			= &rv_cfg_clk.common.hw,
		[CLK_BUS_MCU_RISCV_MSG]			= &riscv_msg_clk.common.hw,
		[CLK_MCU_PWM]			= &mcu_pwm_clk.common.hw,
		[CLK_BUS_MCU_PWM]			= &pwm_bgr_clk.common.hw,
		[CLK_BUS_MCU_AHB_AUTO]			= &ahb_auto_clk.common.hw,
	},
	.num = CLK_MCU_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun55iw3_mcu_ccu_clks[] = {
	&pll_audio1_clk.common,
	&pll_audio_out_clk.common,
	&dsp_dsp_clk.common,
	&i2s0_clk.common,
	&i2s1_clk.common,
	&i2s2_clk.common,
	&i2s3_clk.common,
	&i2s3_asrc_clk.common,
	&i2s0_bgr_clk.common,
	&i2s1_bgr_clk.common,
	&i2s2_bgr_clk.common,
	&i2s3_bgr_clk.common,
	&owa_tx_clk.common,
	&owa_rx_clk.common,
	&bus_owa_clk.common,
	&dmic_clk.common,
	&dmic_bus_clk.common,
	&audio_codec_dac_clk.common,
	&audio_codec_adc_clk.common,
	&audio_codec_clk.common,
	&dsp_msg_clk.common,
	&dsp_cfg_clk.common,
	&npu_aclk.common,
	&npu_hclk.common,
	&mcu_timer0_clk.common,
	&mcu_timer1_clk.common,
	&mcu_timer2_clk.common,
	&mcu_timer3_clk.common,
	&mcu_timer4_clk.common,
	&mcu_timer5_clk.common,
	&bus_mcu_timer_clk.common,
	&mcu_dma_clk.common,
	&tzma0_clk.common,
	&tzma1_clk.common,
	&pubsram_clk.common,
	&mcu_mclk.common,
	&dma_mclk.common,
	&rv_clk.common,
	&rv_cfg_clk.common,
	&riscv_msg_clk.common,
	&mcu_pwm_clk.common,
	&pwm_bgr_clk.common,
	&ahb_auto_clk.common,
};

static const struct sunxi_ccu_desc sun55iw3_mcu_ccu_desc = {
	.ccu_clks	= sun55iw3_mcu_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun55iw3_mcu_ccu_clks),

	.hw_clks	= &sun55iw3_mcu_hw_clks,

	.resets		= sun55iw3_mcu_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun55iw3_mcu_ccu_resets),
};

static int sun55iw3_mcu_ccu_probe(struct platform_device *pdev)
{
	void __iomem *reg;
	int ret;

	reg = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(reg))
		return PTR_ERR(reg);

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun55iw3_mcu_ccu_desc);

	/* Keep off audio1 during startup */
	set_reg(reg + SUN55IW3_PLL_AUDIO_CTRL_REG, 0x0, 1, 30);

	/* Enable audio PLL P0 to 2 */
	set_reg(reg + SUN55IW3_PLL_AUDIO_CTRL_REG, 0x1, 3, 16);

	/* Enable audio PLL P1 to 5 */
	set_reg(reg + SUN55IW3_PLL_AUDIO_CTRL_REG, 0x4, 3, 20);

	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun55iw3_mcu_ccu_clks,
			     ARRAY_SIZE(sun55iw3_mcu_ccu_clks),
			     NULL, 0);

	return 0;
}

static const struct of_device_id sun55iw3_mcu_ccu_ids[] = {
	{ .compatible = "allwinner,sun55iw3-mcu-ccu" },
	{ }
};

static struct platform_driver sun55iw3_mcu_ccu_driver = {
	.probe	= sun55iw3_mcu_ccu_probe,
	.driver	= {
		.name	= "sun55iw3-mcu-ccu",
		.of_match_table	= sun55iw3_mcu_ccu_ids,
	},
};

static int __init sunxi_ccu_sun55iw3_mcu_init(void)
{
	int ret;

	ret = platform_driver_register(&sun55iw3_mcu_ccu_driver);
	if (ret)
		pr_err("register ccu sun55iw3-mcu failed\n");

	return ret;
}
core_initcall(sunxi_ccu_sun55iw3_mcu_init);

static void __exit sunxi_ccu_sun55iw3_mcu_exit(void)
{
	return platform_driver_unregister(&sun55iw3_mcu_ccu_driver);
}
module_exit(sunxi_ccu_sun55iw3_mcu_exit);

MODULE_DESCRIPTION("Allwinner sun55iw3-mcu clk driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_VERSION("1.1.2");
