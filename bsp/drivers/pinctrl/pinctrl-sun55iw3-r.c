// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2021 liujuan1@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun55iw3_r_pins[] = {
	/* BANK L */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),		/* s_twi0_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),		/* s_twi0_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart0_tx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart1_tx */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_0"),	/* mcu_pwm0_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart0_rx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart1_rx */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_1"),	/* mcu_pwm0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_ms */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_2"),	/* mcu_pwm0_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_ck */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_3"),	/* mcu_pwm0_3 */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_mclk */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_do */
		SUNXI_FUNCTION(0x3, "mcu_pwm0_4"),	/* mcu_pwm0_4 */
		SUNXI_FUNCTION(0x4, "i2s0_din"),	/* i2s0_din1 */
		SUNXI_FUNCTION(0x5, "i2s0_dout"),	/* i2s0_dout0 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag"),		/* s_jtag_di */
		SUNXI_FUNCTION(0x3, "mcu_pwm0_5"),	/* mcu_pwm0_5 */
		SUNXI_FUNCTION(0x4, "i2s0_dout"),	/* i2s0_dout1 */
		SUNXI_FUNCTION(0x5, "i2s0_din"),	/* i2s0_din0 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi1_sck */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_ms */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_ms */
		SUNXI_FUNCTION(0x5, "i2s0"),		/* i2s0_bclk */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi1_sda */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_ck */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_ck */
		SUNXI_FUNCTION(0x5, "s_pwm0_1"),	/* s_pwm0_1 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_pwm0_0"),	/* s_pwm0_0 */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_do */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_do */
		SUNXI_FUNCTION(0x5, "dmic"),		/* dmic */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_cs */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_cir"),		/* s_cir_in */
		SUNXI_FUNCTION(0x3, "d_jtag"),		/* d_jtag_di */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_di */
		SUNXI_FUNCTION(0x5, "s_pwm0_1"),	/* s_pwm0_1 */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi2"),		/* s_twi2_sck */
		SUNXI_FUNCTION(0x3, "mcu_pwm0_6"),	/* mcu_pwm0_6 */
		SUNXI_FUNCTION(0x4, "s_uart0"),		/* r_uart0_tx */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_mosi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi2"),		/* s_twi2_sda */
		SUNXI_FUNCTION(0x3, "mcu_pwm0_7"),	/* mcu_pwm0_7 */
		SUNXI_FUNCTION(0x4, "s_uart0"),		/* r_uart0_rx */
		SUNXI_FUNCTION(0x6, "s_spi0"),		/* s_spi0_miso */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	/* BANK M */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart0_tx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart0_tx */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_0"),	/* mcu_pwm0_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),		/* s_uart1_rx */
		SUNXI_FUNCTION(0x3, "s_uart1"),		/* s_uart1_rx */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_1"),	/* mcu_pwm0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi1_sck */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* s_jtag_ms */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_4"),	/* mcu_pwm0_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),		/* s_twi_sda */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* r_jtag_ck */
		SUNXI_FUNCTION(0x4, "mcu_pwm0_5"),	/* mcu_pwm0_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mcu_pwm0_6"),	/* mcu_pwm0_6 */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* r_jtag_do */
		SUNXI_FUNCTION(0x4, "s_twi2"),		/* s_twi2_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */

	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "s_cir"),		/* s_cir_in */
		SUNXI_FUNCTION(0x3, "r_jtag"),		/* r_jtag_di */
		SUNXI_FUNCTION(0x4, "s_twi2"),		/* s_twi2_sda */
		SUNXI_FUNCTION(0x5, "mcu_pwm0_7"),	/* mcu_pwm0_7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
};

static const unsigned int sun55iw3_r_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
};

static const unsigned int sun55iw3_r_bank_base[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
};

static const struct sunxi_pinctrl_desc sun55iw3_r_pinctrl_data = {
	.pins = sun55iw3_r_pins,
	.npins = ARRAY_SIZE(sun55iw3_r_pins),
	.banks = ARRAY_SIZE(sun55iw3_r_bank_base),
	.bank_base = sun55iw3_r_bank_base,
	.irq_banks = ARRAY_SIZE(sun55iw3_r_irq_bank_map),
	.irq_bank_map = sun55iw3_r_irq_bank_map,
	.pin_base = SUNXI_PIN_BASE('L'),
	.hw_type = SUNXI_PCTL_HW_TYPE_6,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL_V2,
};

static int sun55iw3_r_pinctrl_probe(struct platform_device *pdev)
{
	return sunxi_bsp_pinctrl_init(pdev, &sun55iw3_r_pinctrl_data);
}

static struct of_device_id sun55iw3_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun55iw3-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun55iw3_r_pinctrl_match);

static struct platform_driver sun55iw3_r_pinctrl_driver = {
	.probe	= sun55iw3_r_pinctrl_probe,
	.driver	= {
		.name		= "sun55iw3-r-pinctrl",
		.of_match_table	= sun55iw3_r_pinctrl_match,
	},
};

static int __init sun55iw3_r_pio_init(void)
{
	return platform_driver_register(&sun55iw3_r_pinctrl_driver);
}
postcore_initcall(sun55iw3_r_pio_init);

MODULE_DESCRIPTION("Allwinner sun55iw3 R_PIO pinctrl driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<rengaomin@allwinnertech>");
MODULE_VERSION("1.1.5");
