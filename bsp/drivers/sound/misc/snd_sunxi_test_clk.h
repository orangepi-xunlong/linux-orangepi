/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

/* =============================================================================
 * It is a demo to measure clock frequency for clk developers.
 * Actions that clock developers need to perform are:
 * 1.add dtsi node, like:
 * audio_test_clk:audio_test_clk@7112000 {
 *	compatible = "allwinner,sun55iw3";
 *	reg		= <0x0 0x07112000 0x0 0xA0>;
 *	resets		= <&mcu_ccu RST_BUS_MCU_I2S0>;
 *	clocks		= <&ccu CLK_PLL_PERI0_2X>,
 *			  <&ccu CLK_MCU>,
 *			  <&mcu_ccu CLK_MCU_MCU>,
 *			  <&mcu_ccu CLK_BUS_MCU_I2S0>,
 *			  <&ccu CLK_PLL_AUDIO0_4X>,
 *			  <&mcu_ccu CLK_PLL_MCU_AUDIO1_DIV2>,
 *			  <&mcu_ccu CLK_PLL_MCU_AUDIO1_DIV5>,
 *			  <&mcu_ccu CLK_MCU_I2S0>;
 *	clock-names	= "clk_pll_peri0_2x",
 *			  "clk_mcu_src",
 *			  "clk_mcu_core",
 *			  "clk_bus_i2s",
 *			  "clk_pll_audio0_4x",
 *			  "clk_pll_audio1_div2",
 *			  "clk_pll_audio1_div5",
 *			  "clk_i2s";
 *	status = "disabled";
 * };
 * 2.add dts node, like:
 * &audio_test_clk {
 *	pinctrl-used;
 *	pinctrl-names= "default","sleep";
 *	pinctrl-0	= <&i2s0_pins_a>;
 *	pinctrl-1	= <&i2s0_pins_b>;
 *	status		= "okay";
 * };
 * note: we need to disable corresponding i2s node in dts.
 * 3.modify the header file:
 * 3.1 define struct sunxi_test_clk_clk for specified IC.
 * 4.modify the source file:
 * 4.1 define struct sunxi_test_clk_quirks for specified IC.
 * 4.2 add struct of_device_id for specified IC.
 * 4.3 define clk related interfaces:
 *	snd_sunxi_clk_init_sunxxiwxx()
 *	snd_sunxi_clk_exit_sunxxiwxx()
 *	snd_sunxi_clk_enable_sunxxiwxx()
 *	snd_sunxi_clk_disable_sunxxiwxx()
 *	snd_sunxi_clk_rate_sunxxiwxx()
 * 5.tick menuconfig:
 * Allwinner BSP  --->
 *	Device Drivers  --->
 *		SOUND Drivers  --->
 *			Platform drivers  --->
 *				<*> Allwinner Function Components
 *				<*>   Components Test CLK
 * =============================================================================
 */
#ifndef __SND_SUNXI_TEST_CLK_H
#define __SND_SUNXI_TEST_CLK_H

#define SUNXI_I2S_CTL			0x00
#define SUNXI_I2S_CLKDIV		0x24
#define SUNXI_I2S_REV			0x7C
#define SUNXI_I2S_MAX_REG		SUNXI_I2S_REV

/* SUNXI_I2S_CTL:0x00 */
#define GLOBAL_EN			0

/* SUNXI_I2S_CLKDIV:0x24 */
#define MCLKOUT_EN			8
#define MCLK_DIV			0

/* Need to be filled by clk developers.
 * Clocks that need to be included are:
 *	 1.clk_reset
 *	 2.clk_bus
 *	 3.clk_pll
 *	 4.clk_modelue
 *	[5.dependent clock]
 */
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
struct sunxi_test_clk_clk {
	/* reset clk and bus clk */
	struct reset_control *clk_rst;
	struct clk *clk_bus;

	/* parent clk - A523 */
	struct clk *clk_pll_audio0_4x;
	struct clk *clk_pll_audio1_div2;
	struct clk *clk_pll_audio1_div5;
	/* module clk - A523 */
	struct clk *clk_i2s;
	/* dependent clk - A523 */
	struct clk *clk_peri0_2x;
	struct clk *clk_mcu_src;
};
/* e.g.
 * #elif IS_ENABLED(CONFIG_ARCH_SUNXXIWXX)
 * struct sunxi_test_clk {
 *	struct reset_control *clk_rst;
 *	struct clk *clk_bus;
 *
 *	struct clk *clk_xxx;
 * }
 */
#endif

struct sunxi_test_clk_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_test_clk_pinctl {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_test_clk_quirks {
	int (*snd_sunxi_clk_init)(struct platform_device *pdev, struct sunxi_test_clk_clk *clk);
	void (*snd_sunxi_clk_exit)(struct sunxi_test_clk_clk *clk);
	int (*snd_sunxi_clk_enable)(struct sunxi_test_clk_clk *clk);
	void (*snd_sunxi_clk_disable)(struct sunxi_test_clk_clk *clk);
	int (*snd_sunxi_clk_rate)(struct sunxi_test_clk_clk *clk, unsigned int freq);
};

struct sunxi_test_clk {
	struct sunxi_test_clk_clk clk;
	struct sunxi_test_clk_mem mem;
	struct sunxi_test_clk_pinctl pin;
	struct sunxi_test_clk_quirks *quirks;
};

#endif /* __SND_SUNXI_TEST_CLK_H */
