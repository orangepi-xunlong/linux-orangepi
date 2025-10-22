// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2020 wuyan@allwinnertech.com
 */

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

#include "pinctrl-sunxi.h"

/* PB:8pins, PC:8pins, PD:23pins, PE:18pins, PF:7pins, PG:16pins */
static const struct sunxi_desc_pin sun8iw20_pins[] = {
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm3"),
		SUNXI_FUNCTION(0x3, "ir"),		/* TX */
		SUNXI_FUNCTION(0x4, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x5, "spi1"),		/* WP */
		SUNXI_FUNCTION(0x6, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x7, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x8, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x9, "test"),		/* FOR TEST */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 0),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm4"),
		SUNXI_FUNCTION(0x3, "i2s2_dout"),	/* DOUT3 */
		SUNXI_FUNCTION(0x4, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x5, "i2s2_din"),	/* DIN3 */
		SUNXI_FUNCTION(0x6, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x7, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x8, "ir"),		/* RX */
		SUNXI_FUNCTION(0x9, "test"),		/* FOR TEST */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 1),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout"),	/* DOUT2 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "i2s2_din"),	/* DIN2 */
		SUNXI_FUNCTION(0x6, "lcd0"),		/* D18 */
		SUNXI_FUNCTION(0x7, "uart4"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 2),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "i2s2_dout"),	/* DOUT1 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "i2s2_din"),	/* DIN0 */
		SUNXI_FUNCTION(0x6, "lcd0"),		/* D19 */
		SUNXI_FUNCTION(0x7, "uart4"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 3),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D8 */
		SUNXI_FUNCTION(0x3, "i2s2_dout"),	/* DOUT0 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x5, "i2s2_din"),	/* DIN1 */
		SUNXI_FUNCTION(0x6, "lcd0"),		/* D20 */
		SUNXI_FUNCTION(0x7, "uart5"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 4),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D9 */
		SUNXI_FUNCTION(0x3, "i2s2"),		/* BCLK */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SDA */
		SUNXI_FUNCTION(0x5, "pwm0"),
		SUNXI_FUNCTION(0x6, "lcd0"),		/* D21 */
		SUNXI_FUNCTION(0x7, "uart5"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 5),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D16 */
		SUNXI_FUNCTION(0x3, "i2s2"),		/* LRCK */
		SUNXI_FUNCTION(0x4, "twi3"),		/* SCK */
		SUNXI_FUNCTION(0x5, "pwm1"),
		SUNXI_FUNCTION(0x6, "lcd0"),		/* D22 */
		SUNXI_FUNCTION(0x7, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x8, "bist0"),		/* BIST_RESULT0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 6),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D17 */
		SUNXI_FUNCTION(0x3, "i2s2"),		/* MCLK */
		SUNXI_FUNCTION(0x4, "twi3"),		/* SDA */
		SUNXI_FUNCTION(0x5, "ir"),		/* RX */
		SUNXI_FUNCTION(0x6, "lcd0"),		/* D23 */
		SUNXI_FUNCTION(0x7, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x8, "bist1"),		/* BIST_RESULT1 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 7),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA3 */
		SUNXI_FUNCTION(0x3, "pwm5"),		/* pwm5 */
		SUNXI_FUNCTION(0x4, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x5, "spi1"),		/* HOLD */
		SUNXI_FUNCTION(0x6, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x7, "uart1"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 8),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA2 */
		SUNXI_FUNCTION(0x3, "pwm6"),		/* PWM6 */
		SUNXI_FUNCTION(0x4, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x5, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x6, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x7, "uart1"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 9),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA1 */
		SUNXI_FUNCTION(0x3, "pwm7"),		/* PWM7 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x6, "clk_fanout0"),
		SUNXI_FUNCTION(0x7, "uart1"),		/* RTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 10),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* DATA0 */
		SUNXI_FUNCTION(0x3, "pwm2"),		/* PWM2 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x6, "clk_fanout1"),
		SUNXI_FUNCTION(0x7, "uart1"),		/* CTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 11),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "dmic"),		/* CLK */
		SUNXI_FUNCTION(0x3, "pwm0"),		/* pwm0 */
		SUNXI_FUNCTION(0x4, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x5, "spi1"),		/* CS0 */
		SUNXI_FUNCTION(0x6, "clk_fanout2"),
		SUNXI_FUNCTION(0x7, "ir"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 0, 12),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	/* PC */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x4, "ledc"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 0),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 1),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 2),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi0"),		/* CS0 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* CMD */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 3),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi0"),		/* MOSI */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D2 */
		SUNXI_FUNCTION(0x4, "boot"),		/* SEL0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 4),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi0"),		/* MISO */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D1 */
		SUNXI_FUNCTION(0x4, "boot"),		/* SEL1 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 5),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi0"),		/* WP */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D0 */
		SUNXI_FUNCTION(0x4, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x5, "twi3"),		/* SCK */
		SUNXI_FUNCTION(0x6, "pll"),		/* pll_test_gpio */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 6),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spi0"),		/* HOLD */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* D3 */
		SUNXI_FUNCTION(0x4, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x5, "twi3"),		/* SDA */
		SUNXI_FUNCTION(0x6, "tcon"),		/* TRIG0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 1, 7),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	/* PD */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V0P */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D0P */
		SUNXI_FUNCTION(0x5, "twi0"),		/* SCK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 0),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V0N */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D0N */
		SUNXI_FUNCTION(0x5, "uart2"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 1),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D4 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V1P */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D1P */
		SUNXI_FUNCTION(0x5, "uart2"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 2),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D5 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V1P */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D1P */
		SUNXI_FUNCTION(0x5, "uart2"),		/* RTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 3),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D6 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V2P */
		SUNXI_FUNCTION(0x4, "dsi"),		/* CKP */
		SUNXI_FUNCTION(0x5, "uart2"),		/* CTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 4),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D7 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V2N */
		SUNXI_FUNCTION(0x4, "dsi"),		/* CKN */
		SUNXI_FUNCTION(0x5, "uart5"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 5),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D10 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* CKP */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D2P */
		SUNXI_FUNCTION(0x5, "uart5"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 6),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D11 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* CKN */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D2N */
		SUNXI_FUNCTION(0x5, "uart4"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 7),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D12 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V3P */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D3P */
		SUNXI_FUNCTION(0x5, "uart4"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 8),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D13 */
		SUNXI_FUNCTION(0x3, "lvds0"),		/* V3N */
		SUNXI_FUNCTION(0x4, "dsi"),		/* D3N */
		SUNXI_FUNCTION(0x5, "pwm6"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 9),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D14 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V0P */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CS0 */
		SUNXI_FUNCTION(0x5, "uart3"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 10),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D15 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V0N */
		SUNXI_FUNCTION(0x4, "spi1"),		/* CLK */
		SUNXI_FUNCTION(0x5, "uart3"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 11),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D18 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V1P */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MOSI */
		SUNXI_FUNCTION(0x5, "twi0"),		/* SDA */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 12),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D19 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V1N */
		SUNXI_FUNCTION(0x4, "spi1"),		/* MISO */
		SUNXI_FUNCTION(0x5, "uart3"),		/* RTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 13),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D20 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V2P */
		SUNXI_FUNCTION(0x4, "spi1"),		/* HOLD */
		SUNXI_FUNCTION(0x5, "uart3"),		/* CTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 14),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D21 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V2N */
		SUNXI_FUNCTION(0x4, "spi1"),		/* WP */
		SUNXI_FUNCTION(0x5, "ir"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 15),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D22 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* SKP */
		SUNXI_FUNCTION(0x4, "dmic"),		/* DATA3 */
		SUNXI_FUNCTION(0x5, "pwm0"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 16),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* D23 */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* SKN */
		SUNXI_FUNCTION(0x4, "dmic"),		/* DATA2 */
		SUNXI_FUNCTION(0x5, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 17),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V3P */
		SUNXI_FUNCTION(0x4, "dmic"),		/* DATA1 */
		SUNXI_FUNCTION(0x5, "pwm2"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 18),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* DE */
		SUNXI_FUNCTION(0x3, "lvds1"),		/* V3N */
		SUNXI_FUNCTION(0x4, "dmic"),		/* DATA0 */
		SUNXI_FUNCTION(0x5, "pwm3"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 19),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* HSYNC */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x4, "dmic"),		/* CLK */
		SUNXI_FUNCTION(0x5, "pwm4"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 20),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "lcd0"),		/* VSYNC */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x4, "uart1"),		/* TX */
		SUNXI_FUNCTION(0x5, "pwm5"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 21),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x3, "ir"),		/* RX */
		SUNXI_FUNCTION(0x4, "uart1"),		/* RX */
		SUNXI_FUNCTION(0x5, "pwm7"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 2, 22),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	/* PE */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* HSYNC */
		SUNXI_FUNCTION(0x3, "uart2"),		/* RTS */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x5, "lcd0"),		/* HSYNC */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* RXCTL/CRS_DV */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 0),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* VSYNC */
		SUNXI_FUNCTION(0x3, "uart2"),		/* CTS */
		SUNXI_FUNCTION(0x4, "twi1"),		/* SDA */
		SUNXI_FUNCTION(0x5, "lcd0"),		/* VSYNC */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* RXD0/RXD0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 1),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* PCLK */
		SUNXI_FUNCTION(0x3, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "clk_fanout0"),
		SUNXI_FUNCTION(0x6, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* RXD1/RXD1 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 2),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi0"),		/* MCLK */
		SUNXI_FUNCTION(0x3, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "clk_fanout1"),
		SUNXI_FUNCTION(0x6, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* TXCK/TXCK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 3),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "uart4"),		/* TX */
		SUNXI_FUNCTION(0x4, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x5, "clk_fanout2"),
		SUNXI_FUNCTION(0x6, "d_jtag"),	/* MS */
		SUNXI_FUNCTION(0x7, "r_jtag"),	/* MS */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* TXD0/TXD0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 4),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "uart4"),		/* RX */
		SUNXI_FUNCTION(0x4, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x5, "ledc"),
		SUNXI_FUNCTION(0x6, "d_jtag"),	/* D1 */
		SUNXI_FUNCTION(0x7, "r_jtag"),	/* D1 */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* TXD1/TXD1 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 5),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "uart5"),		/* TX */
		SUNXI_FUNCTION(0x4, "twi3"),		/* SCK */
		SUNXI_FUNCTION(0x5, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x6, "d_jtag"),	/* D0 */
		SUNXI_FUNCTION(0x7, "r_jtag"),	/* D0 */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* TXCTL/TXEN */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 6),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart5"),		/* RX */
		SUNXI_FUNCTION(0x4, "twi3"),		/* SDA */
		SUNXI_FUNCTION(0x5, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x6, "d_jtag"),	/* CK */
		SUNXI_FUNCTION(0x7, "r_jtag"),	/* CK */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* CK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 7),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D4 */
		SUNXI_FUNCTION(0x3, "uart1"),		/* RTS */
		SUNXI_FUNCTION(0x4, "pwm2"),
		SUNXI_FUNCTION(0x5, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x6, "jtag"),		/* MS */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* MS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 8),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D5 */
		SUNXI_FUNCTION(0x3, "uart1"),		/* CTS */
		SUNXI_FUNCTION(0x4, "pwm3"),
		SUNXI_FUNCTION(0x5, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x6, "jtag"),		/* D1 */
		SUNXI_FUNCTION(0x8, "gmac0"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 9),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D6 */
		SUNXI_FUNCTION(0x3, "uart1"),		/* TX */
		SUNXI_FUNCTION(0x4, "pwm4"),
		SUNXI_FUNCTION(0x5, "ir"),		/* RX */
		SUNXI_FUNCTION(0x6, "jtag"),		/* D0 */
		SUNXI_FUNCTION(0x8, "gmac0"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 10),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ncsi0"),		/* D7 */
		SUNXI_FUNCTION(0x3, "uart1"),		/* RX */
		SUNXI_FUNCTION(0x4, "i2s0_dout"),	/* DOUT3 */
		SUNXI_FUNCTION(0x5, "i2s0_din"),	/* DIN3 */
		SUNXI_FUNCTION(0x6, "jtag"),		/* CK */
		SUNXI_FUNCTION(0x8, "gmac0"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 11),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x3, "ncsi0"),		/* FIELD */
		SUNXI_FUNCTION(0x4, "i2s0_dout"),	/* DOUT2 */
		SUNXI_FUNCTION(0x5, "i2s0_din"),	/* DIN2 */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* TXD3/NULL */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 12),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x3, "pwm5"),
		SUNXI_FUNCTION(0x4, "i2s0_dout"),	/* DOUT0 */
		SUNXI_FUNCTION(0x5, "i2s0_din"),	/* DIN1 */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* RXD3/MULL */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 13),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x3, "d_jtag"),	/* MS */
		SUNXI_FUNCTION(0x4, "i2s0_dout"),	/* DOUT1 */
		SUNXI_FUNCTION(0x5, "i2s0_din"),		/* DIN0 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* DATA2 */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* RXD3/MULL */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 14),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi1"),		/* SDA */
		SUNXI_FUNCTION(0x3, "d_jtag"),	/* D1 */
		SUNXI_FUNCTION(0x4, "pwm6"),
		SUNXI_FUNCTION(0x5, "i2s0"),		/* LRCK */
		SUNXI_FUNCTION(0x6, "dmic"),		/* DATA1 */
		SUNXI_FUNCTION(0x8, "gmac0"),		/* RXCK/NULL */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 15),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi3"),		/* SCK */
		SUNXI_FUNCTION(0x3, "d_jtag"),	/* D0 */
		SUNXI_FUNCTION(0x4, "pwm7"),
		SUNXI_FUNCTION(0x5, "i2s0"),		/* BCLK */
		SUNXI_FUNCTION(0x6, "dmic"),		/* DATA0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 16),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "twi3"),		/* SDA */
		SUNXI_FUNCTION(0x3, "d_jtag"),	/* CK */
		SUNXI_FUNCTION(0x4, "ir"),		/* TX */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* MCLK */
		SUNXI_FUNCTION(0x6, "dmic"),		/* CLK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 3, 17),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	/* PF */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D1 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* MS */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* MS */
		SUNXI_FUNCTION(0x5, "i2s2_dout"),	/* DOUT1 */
		SUNXI_FUNCTION(0x6, "i2s2_din"),	/* DIN0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 0),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D0 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DI */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* DI */
		SUNXI_FUNCTION(0x5, "i2s2_dout"),	/* DOUT0 */
		SUNXI_FUNCTION(0x6, "i2s2_din"),	/* DIN1 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 1),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* CLK */
		SUNXI_FUNCTION(0x3, "uart0"),		/* TX */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x5, "ledc"),
		SUNXI_FUNCTION(0x6, "spdif"),		/* IN */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 2),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* CMD */
		SUNXI_FUNCTION(0x3, "jtag"),		/* DO */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* DO */
		SUNXI_FUNCTION(0x5, "i2s2"),		/* BCLK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 3),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart0"),		/* RX */
		SUNXI_FUNCTION(0x4, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x5, "pwm6"),
		SUNXI_FUNCTION(0x6, "ir"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 4),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc0"),		/* D2 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* CK */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* CK */
		SUNXI_FUNCTION(0x5, "i2s2"),		/* LRCK */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 5),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x3, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x4, "ir"),		/* RX */
		SUNXI_FUNCTION(0x5, "i2s2"),		/* MCLK */
		SUNXI_FUNCTION(0x6, "pwm5"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 4, 6),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	/* PG */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* CLK */
		SUNXI_FUNCTION(0x3, "uart3"),		/* TX */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RXCTRL/CRS_DV */
		SUNXI_FUNCTION(0x5, "pwm7"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 0),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* CMD */
		SUNXI_FUNCTION(0x3, "uart3"),		/* RX */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RXD0/RXD0 */
		SUNXI_FUNCTION(0x5, "pwm6"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 1),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D0 */
		SUNXI_FUNCTION(0x3, "uart3"),		/* RTS */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RXD1/RXD1 */
		SUNXI_FUNCTION(0x5, "uart4"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 2),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D1 */
		SUNXI_FUNCTION(0x3, "uart3"),		/* CTS */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RGMII_TXCK / RMII_TXCK */
		SUNXI_FUNCTION(0x5, "uart4"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 3),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D2 */
		SUNXI_FUNCTION(0x3, "uart5"),		/* TX */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* TXD0/TXD0 */
		SUNXI_FUNCTION(0x5, "pwm5"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 4),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "sdc1"),		/* D3 */
		SUNXI_FUNCTION(0x3, "uart5"),		/* RX */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* TXD1/TXD1 */
		SUNXI_FUNCTION(0x5, "pwm4"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 5),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* TX */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* TXD2/NULL */
		SUNXI_FUNCTION(0x5, "pwm1"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 6),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* TXD3/NULL */
		SUNXI_FUNCTION(0x5, "spdif"),		/* IN */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 7),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* RTS */
		SUNXI_FUNCTION(0x3, "twi1"),		/* SCK */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RXD2/NULL */
		SUNXI_FUNCTION(0x5, "uart3"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 8),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart1"),		/* CTS */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RXD3/NULL */
		SUNXI_FUNCTION(0x5, "uart3"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 9),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "pwm3"),
		SUNXI_FUNCTION(0x3, "twi3"),		/* SCK */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* RXCK/NULL */
		SUNXI_FUNCTION(0x5, "clk_fanout0"),
		SUNXI_FUNCTION(0x6, "ir"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 10),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* MCLK */
		SUNXI_FUNCTION(0x3, "twi3"),		/* SDA */
		SUNXI_FUNCTION(0x4, "gmac0"),
		SUNXI_FUNCTION(0x5, "clk_fanout1"),
		SUNXI_FUNCTION(0x6, "tcon"),		/* TRIG0 */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 11),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* LRCK */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SCK */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* TXCTL/TXEN */
		SUNXI_FUNCTION(0x5, "fanout2"),
		SUNXI_FUNCTION(0x6, "pwm0"),
		SUNXI_FUNCTION(0x7, "uart1"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 12),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1"),		/* BCLK */
		SUNXI_FUNCTION(0x3, "twi0"),		/* SDA */
		SUNXI_FUNCTION(0x4, "gmac0"),		/* CLKIN/RXER */
		SUNXI_FUNCTION(0x5, "pwm2"),
		SUNXI_FUNCTION(0x6, "ledc"),
		SUNXI_FUNCTION(0x7, "uart1"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 13),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1_din"),	/* DIN0 */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SCK */
		SUNXI_FUNCTION(0x4, "gmac0"),
		SUNXI_FUNCTION(0x5, "i2s1_dout"),	/* DOUT1 */
		SUNXI_FUNCTION(0x6, "spi0"),		/* WP */
		SUNXI_FUNCTION(0x7, "uart1"),		/* RTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 14),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "i2s1_dout"),	/* DOUT0 */
		SUNXI_FUNCTION(0x3, "twi2"),		/* SDA */
		SUNXI_FUNCTION(0x4, "gmac0"),
		SUNXI_FUNCTION(0x5, "i2s1_din"),	/* DIN1 */
		SUNXI_FUNCTION(0x6, "spi0"),		/* HOLD */
		SUNXI_FUNCTION(0x7, "uart1"),		/* CTS */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 15),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "ir"),		/* RX */
		SUNXI_FUNCTION(0x3, "tcon"),		/* TRIG0 */
		SUNXI_FUNCTION(0x4, "pwm5"),
		SUNXI_FUNCTION(0x5, "clk_fanout2"),
		SUNXI_FUNCTION(0x6, "spdif"),		/* IN */
		SUNXI_FUNCTION(0x7, "ledc"),
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 16),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* TX */
		SUNXI_FUNCTION(0x3, "twi3"),		/* SCK */
		SUNXI_FUNCTION(0x4, "pwm7"),
		SUNXI_FUNCTION(0x5, "clk_fanout0"),
		SUNXI_FUNCTION(0x6, "ir"),		/* TX */
		SUNXI_FUNCTION(0x7, "uart0"),		/* TX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 17),
		SUNXI_FUNCTION(0xF, "io_disabled")),
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "uart2"),		/* RX */
		SUNXI_FUNCTION(0x3, "twi3"),		/* SDA */
		SUNXI_FUNCTION(0x4, "pwm6"),
		SUNXI_FUNCTION(0x5, "clk_fanout1"),
		SUNXI_FUNCTION(0x6, "spdif"),		/* OUT */
		SUNXI_FUNCTION(0x7, "uart0"),		/* RX */
		SUNXI_FUNCTION_IRQ_BANK(0xE, 5, 18),
		SUNXI_FUNCTION(0xF, "io_disabled")),
};

static const unsigned int sun8iw20_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
};

static const struct sunxi_pinctrl_desc sun8iw20_pinctrl_data = {
	.pins = sun8iw20_pins,
	.npins = ARRAY_SIZE(sun8iw20_pins),
	.irq_banks = ARRAY_SIZE(sun8iw20_irq_bank_map),
	.irq_bank_map = sun8iw20_irq_bank_map,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL,
	.pf_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

/* PINCTRL power management code */
#if IS_ENABLED(CONFIG_PM_SLEEP)

static void *mem;
static int mem_size;

static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
	return 0;
}

static int sun8iw20_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	sunxi_info(dev, "pinctrl suspend\n");

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int sun8iw20_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_toio(pctl->membase, mem, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	sunxi_info(dev, "pinctrl resume\n");

	return 0;
}

static const struct dev_pm_ops sun8iw20_pinctrl_pm_ops = {
	.suspend_noirq = sun8iw20_pinctrl_suspend_noirq,
	.resume_noirq = sun8iw20_pinctrl_resume_noirq,
};
#define PINCTRL_PM_OPS	(&sun8iw20_pinctrl_pm_ops)

#else
static int pinctrl_pm_alloc_mem(struct platform_device *pdev)
{
	return 0;
}
#define PINCTRL_PM_OPS	NULL
#endif

static int sun8iw20_pinctrl_probe(struct platform_device *pdev)
{
	int ret;
	ret = pinctrl_pm_alloc_mem(pdev);
	if (ret) {
		sunxi_err(&pdev->dev, "alloc pm mem err\n");
		return ret;
	}
#if IS_ENABLED(CONFIG_AW_PINCTRL_DEBUGFS)
	dev_set_name(&pdev->dev, "pio");
#endif
	return sunxi_bsp_pinctrl_init(pdev, &sun8iw20_pinctrl_data);
}

static struct of_device_id sun8iw20_pinctrl_match[] = {
	{ .compatible = "allwinner,sun8iw20-pinctrl", },
	{ .compatible = "allwinner,sun20iw1-pinctrl", },
	{}
};

MODULE_DEVICE_TABLE(of, sun8iw20_pinctrl_match);

static struct platform_driver sun8iw20_pinctrl_driver = {
	.probe	= sun8iw20_pinctrl_probe,
	.driver	= {
		.name		= "sun8iw20-pinctrl",
		.pm		= PINCTRL_PM_OPS,
		.of_match_table	= sun8iw20_pinctrl_match,
	},
};

static int __init sun8iw20_pio_init(void)
{
	return platform_driver_register(&sun8iw20_pinctrl_driver);
}
/*
 * TODO: To ensure the load time of the pinctrl is after the
 * subsys_initcall("regulator_fixed_voltage_init" will use it)
 */
fs_initcall(sun8iw20_pio_init);

MODULE_DESCRIPTION("Allwinner sun8iw20 pio pinctrl driver");
MODULE_AUTHOR("Martin <wuyan@allwinnertech>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.5");
