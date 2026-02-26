// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2024 geyoupengs@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include "pinctrl-sunxi.h"

#define SUNXI_R_PINCTRL_VERSION          "0.0.2"

static const struct sunxi_desc_pin sun60iw2_r_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_S4) || IS_ENABLED(CONFIG_AW_FPGA_V7)
#else
	/* Pin banks are: L M */
	/* bank L */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),    /* s_twi0 */
		SUNXI_FUNCTION(0x3, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x4, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi0"),    /* s_twi0 */
		SUNXI_FUNCTION(0x3, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x4, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x5, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x3, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x4, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_0"),      /* s_pwm0_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x3, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x4, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x5, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x6, "s_pwm0_1"),      /* s_pwm0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_ms"),     /* s_jtag_ms */
		SUNXI_FUNCTION(0x3, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x4, "s_spi0"),    /* s_spi0 */
		SUNXI_FUNCTION(0x5, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x6, "s_pwm0_2"),      /* s_pwm0_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_ck"),     /* s_jtag_ck */
		SUNXI_FUNCTION(0x3, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x4, "s_spi0"),    /* s_spi0 */
		SUNXI_FUNCTION(0x6, "s_pwm0_3"),      /* s_pwm0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_do"),     /* s_jtag_do */
		SUNXI_FUNCTION(0x3, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x4, "s_spi0"),   /* s_spi0 */
		SUNXI_FUNCTION(0x5, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x6, "s_pwm0_4"),      /* s_pwm0_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_di"),     /* s_jtag_di */
		SUNXI_FUNCTION(0x3, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x4, "s_spi0"),   /* s_spi0 */
		SUNXI_FUNCTION(0x6, "s_pwm0_5"),      /* s_pwm0_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x3, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x4, "s_twi0"),    /* s_twi0 */
		SUNXI_FUNCTION(0x5, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x6, "s_pwm0_6"),      /* s_pwm0_6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x3, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x4, "s_twi0"),    /* s_twi0 */
		SUNXI_FUNCTION(0x5, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x6, "s_pwm0_7"),      /* s_pwm0_7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x3, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x4, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_8"),      /* s_pwm0_8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x3, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x4, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x5, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION(0x6, "s_pwm0_9"),      /* s_pwm0_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x3, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x4, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x5, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(L, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x3, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x4, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank M */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_ms"),     /* s_jtag_ms */
		SUNXI_FUNCTION(0x3, "s_spi0"),    /* s_spi0 */
		SUNXI_FUNCTION(0x4, "s_rjtag_ms"),    /* s_rjtag_ms */
		SUNXI_FUNCTION(0x5, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_2"),      /* s_pwm0_2 */
		SUNXI_FUNCTION(0x7, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_ck"),     /* s_jtag_ck */
		SUNXI_FUNCTION(0x3, "s_spi0"),    /* s_spi0 */
		SUNXI_FUNCTION(0x4, "s_rjtag_ck"),    /* s_rjtag_ck */
		SUNXI_FUNCTION(0x5, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_3"),      /* s_pwm0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_do"),     /* s_jtag_do */
		SUNXI_FUNCTION(0x3, "s_spi0"),   /* s_spi0 */
		SUNXI_FUNCTION(0x4, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x5, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_4"),      /* s_pwm0_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_jtag_di"),     /* s_jtag_di */
		SUNXI_FUNCTION(0x3, "s_spi0"),   /* s_spi0 */
		SUNXI_FUNCTION(0x4, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x5, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_5"),      /* s_pwm0_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x3, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x4, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x5, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_0"),      /* s_pwm0_0 */
		SUNXI_FUNCTION(0x7, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(M, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "s_uart0"),    /* s_uart0 */
		SUNXI_FUNCTION(0x3, "s_twi2"),    /* s_twi2 */
		SUNXI_FUNCTION(0x4, "s_twi1"),    /* s_twi1 */
		SUNXI_FUNCTION(0x5, "s_uart1"),    /* s_uart1 */
		SUNXI_FUNCTION(0x6, "s_pwm0_1"),      /* s_pwm0_1 */
		SUNXI_FUNCTION(0x7, "s_ir_rx"),       /* s_ir_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#endif
};

static const unsigned int sun60iw2_r_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
};

static const unsigned int sun60iw2_r_bank_base[] = {
	SUNXI_BANK_OFFSET('L', 'L'),
	SUNXI_BANK_OFFSET('M', 'L'),
};

static const struct sunxi_pinctrl_desc sun60iw2_r_pinctrl_data = {
	.pins = sun60iw2_r_pins,
	.npins = ARRAY_SIZE(sun60iw2_r_pins),
	.banks = ARRAY_SIZE(sun60iw2_r_bank_base),
	.bank_base = sun60iw2_r_bank_base,
	.irq_banks = ARRAY_SIZE(sun60iw2_r_irq_bank_map),
	.irq_bank_map = sun60iw2_r_irq_bank_map,
	.pin_base = SUNXI_PIN_BASE('L'),
	.auto_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
};

static int sun60iw2_r_pinctrl_probe(struct platform_device *pdev)
{
	sunxi_info(NULL, "sunxi r-pinctrl version: %s\n", SUNXI_R_PINCTRL_VERSION);

	return sunxi_bsp_pinctrl_init(pdev, &sun60iw2_r_pinctrl_data);
}

static struct of_device_id sun60iw2_r_pinctrl_match[] = {
	{ .compatible = "allwinner,sun60iw2-r-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun60iw2_r_pinctrl_match);

static struct platform_driver sun60iw2_r_pinctrl_driver = {
	.probe	= sun60iw2_r_pinctrl_probe,
	.driver	= {
		.name		= "sun60iw2-r-pinctrl",
		.of_match_table	= sun60iw2_r_pinctrl_match,
	},
};

static int __init sun60iw2_r_pio_init(void)
{
	return platform_driver_register(&sun60iw2_r_pinctrl_driver);
}
#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
subsys_initcall(sun60iw2_r_pio_init);
#else
postcore_initcall(sun60iw2_r_pio_init);
#endif

MODULE_DESCRIPTION("Allwinner sun60iw2 R_PIO pinctrl driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("<geyoupengs@allwinnertech>");
MODULE_VERSION(SUNXI_R_PINCTRL_VERSION);
