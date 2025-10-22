/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner sun8iw21p1 SoCs pinctrl driver.
*
* Copyright(c) 2016-2021 Allwinnertech Co., Ltd.
* Author: weidonghui <weidonghui@allwinnertech.com>
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun8iw21p1_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_S4) || IS_ENABLED(CONFIG_AW_FPGA_V7)
	/* Pin banks are: A B D F G */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[0] */
		SUNXI_FUNCTION(0x5, "test"),          /* test */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[1] */
		SUNXI_FUNCTION(0x5, "test"),          /* test */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clkrx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxdv_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_mdc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_md_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txen_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_sda_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1_sda_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_bclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_lrc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_mclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[1] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[2] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[3] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[4] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[5] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[6] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[7] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[8] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[9] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[10] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[11] */
		SUNXI_FUNCTION(0x7, "rmii"),          /* rmii */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[12] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[13] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[14] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[15] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[16] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[17] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[18] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[19] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[20] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[21] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[22] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[23] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[24] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[25] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[26] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[27] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0_cdat_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0_cdat_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0_cdat_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0_cdat_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0_ccmd_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0_cclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_clk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),          /* spif_dqs_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2_cdat_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2_cdat_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2_cdat_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2_cdat_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq4_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq5_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq6_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq7_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2_ccmd_di */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_mosi_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_mosi_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc2"),          /* sdc2_cclk_do */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_ss_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_ss_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_hold_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_hold_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_miso_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_miso_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_wp_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_wp_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_sck_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_sck_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 31),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d3_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d2_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d1_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

#else
	/* Pin banks are: A C D E F G H I */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia_rx"),	/* mipia_rx_ckop */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d8 */
		SUNXI_FUNCTION(0x5, "test"),		/* test */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia_rx"),	/* mipia_rx_ckon */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d9 */
		SUNXI_FUNCTION(0x5, "test"),		/* test */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia_rx"),	/* mipia_rx_d1n */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d10 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia_rx"),	/* mipia_rx_d1p */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia_rx"),	/* mipia_rx_d0p */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d12 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipia_rx"),	/* mipia_rx_d0n */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d13 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib_rx"),	/* mipib_rx_d0n */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d14 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* twi1_sck */
		SUNXI_FUNCTION(0x5, "pwm0"),		/* pwm0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib_rx"),	/* mipib_rx_d0p */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d15 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* twi1_sda */
		SUNXI_FUNCTION(0x5, "pwm1"),		/* pwm1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib_rx"),	/* mipib_rx_d1n */
		SUNXI_FUNCTION(0x3, "twi4"),		/* twi4_sck */
		SUNXI_FUNCTION(0x4, "twi3"),		/* twi3_sck */
		SUNXI_FUNCTION(0x5, "pwm2"),		/* pwm2 */
		SUNXI_FUNCTION(0x6, "uart2"),		/* uart2_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib_rx"),	/* mipib_rx_d1p */
		SUNXI_FUNCTION(0x3, "twi4"),		/* twi4_sda */
		SUNXI_FUNCTION(0x4, "twi3"),		/* twi3_sda */
		SUNXI_FUNCTION(0x5, "pwm3"),		/* pwm3 */
		SUNXI_FUNCTION(0x6, "uart2"),		/* uart2_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib_rx"),	/* mipib_rx_ckon */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_hsync */
		SUNXI_FUNCTION(0x4, "mipi_csi_mclk0"),	/* mipi_csi_mclk0 */
		SUNXI_FUNCTION(0x5, "twi0"),		/* twi0_sck */
		SUNXI_FUNCTION(0x6, "clk"),		/* clk_fanout0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "mipib_rx"),	/* mipib_rx_ckop */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_vsync */
		SUNXI_FUNCTION(0x4, "mipi_csi_mclk1"),	/* mipi_csi_mclk1 */
		SUNXI_FUNCTION(0x5, "twi0"),		/* twi0_sda */
		SUNXI_FUNCTION(0x6, "clk"),		/* clk_fanout1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d0 */
		SUNXI_FUNCTION(0x4, "mipi_csi_mclk0"),	/* mipi_csi_mclk0 */
		SUNXI_FUNCTION(0x5, "uart0"),		/* uart0_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d1 */
		SUNXI_FUNCTION(0x4, "mipi_csi_mclk1"),	/* mipi_csi_mclk1 */
		SUNXI_FUNCTION(0x5, "uart0"),		/* uart0_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d2 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* twi1_sck */
		SUNXI_FUNCTION(0x5, "clk"),		/* clk_fanout0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d3 */
		SUNXI_FUNCTION(0x4, "twi1"),		/* twi1_sda */
		SUNXI_FUNCTION(0x5, "clk"),		/* clk_fanout1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 15),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d4 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* twi0_sck */
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 16),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d5 */
		SUNXI_FUNCTION(0x4, "twi0"),		/* twi0_sda */
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 17),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d6 */
		SUNXI_FUNCTION(0x4, "wiegand"),		/* wiegand_d0	*/
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 18),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_d7 */
		SUNXI_FUNCTION(0x4, "wiegand"),		/* wiegand_d1	*/
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 19),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi_mclk"),		/* ncsi_mclk */
		SUNXI_FUNCTION(0x4, "csi"),		/* csi_sm_vs	*/
		SUNXI_FUNCTION(0x5, "tcon"),		/* tcon_trig */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 20),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "ncsi"),		/* ncsi_pclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 21),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_clk */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_clk */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_cs0 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_cmd */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_cs0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_mosi_io0 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d2 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_mosi */
		SUNXI_FUNCTION(0x5, "boot_sel0"),	/* boot_sel0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_mosi_io1 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d1 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_miso */
		SUNXI_FUNCTION(0x5, "boot_sel1"),	/* boot_sel1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_wp_io2 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d0 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_wp */
		SUNXI_FUNCTION(0x5, "pwm4"),		/* pwm4 */
		SUNXI_FUNCTION(0x6, "twi1"),		/* twi1_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_hold_io3 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d3 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_hold */
		SUNXI_FUNCTION(0x5, "pwm4"),		/* pwm5 */
		SUNXI_FUNCTION(0x6, "twi1"),		/* twi1_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_io4 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d4 */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_csi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_io5 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_io6 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_io7 */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_d7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "spif"),		/* spif_dqs */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_ds */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x3, "sdc2"),		/* sdc2_rst */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d3 */
		SUNXI_FUNCTION(0x3, "pwm0"),		/* pwm0 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_rxd1 */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d0n */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_cs0/dbi_csx */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_txd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d4 */
		SUNXI_FUNCTION(0x3, "pwm1"),		/* pwm1 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_rxd0 */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d0p */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_clk/dbi_sclk */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_txd1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d5 */
		SUNXI_FUNCTION(0x3, "pwm2"),		/* pwm2 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_crs_dv */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d1n */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_mosi/dbi_sdo */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_rxer */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d6 */
		SUNXI_FUNCTION(0x3, "pwm3"),		/* pwm3 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_rxer */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d1p */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_miso/dbi_sdi/dbi_te/dbi_dcx */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_crs_dv */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d7 */
		SUNXI_FUNCTION(0x3, "pwm4"),		/* pwm4 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_txd1 */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_ckn */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_hold/dbi_dcx/dbi_wrx */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_rxd1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d10 */
		SUNXI_FUNCTION(0x3, "pwm5"),		/* pwm5 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_txd0 */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_ckp */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_wp/dbi_te */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_rxd0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d11 */
		SUNXI_FUNCTION(0x3, "pwm6"),		/* pwm6 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_txck */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d2n */
		SUNXI_FUNCTION(0x6, "spi1"),		/* spi1_cs1 */
		SUNXI_FUNCTION(0x7, "rmii"),		/* mdc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d12 */
		SUNXI_FUNCTION(0x3, "pwm7"),		/* pwm7 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii_txen */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d13 */
		SUNXI_FUNCTION(0x3, "pwm8"),		/* pwm8 */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d2p */
		SUNXI_FUNCTION(0x7, "rmii"),		/* mdio */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d14 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_mclk */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d3n */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_txen */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d15 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_bclk */
		SUNXI_FUNCTION(0x5, "dsi"),		/* dsi_d3p */
		SUNXI_FUNCTION(0x7, "rmii"),		/* rmii_txck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d18 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_lclk */
		SUNXI_FUNCTION(0x5, "dmic"),		/* dmic_data3 */
		SUNXI_FUNCTION(0x7, "pwm11"),		/* pwm11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d19 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_dout0 */
		SUNXI_FUNCTION(0x5, "dmic"),		/* dmic_data2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d20 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_dout1 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_din1 */
		SUNXI_FUNCTION(0x5, "dmic"),		/* dmic_data1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d21 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_dout2 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_din2 */
		SUNXI_FUNCTION(0x5, "dmic"),		/* dmic_data0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d22 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_dout3 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_din3 */
		SUNXI_FUNCTION(0x5, "dmic"),		/* dmic_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_d23 */
		SUNXI_FUNCTION(0x3, "i2s1"),		/* i2s1_din0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_clk */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_clk */
		SUNXI_FUNCTION(0x6, "twi3"),		/* twi3_sck */
		SUNXI_FUNCTION(0x7, "uart2"),		/* uart2_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_de */
		SUNXI_FUNCTION(0x3, "pwm9"),		/* pwm9 */
		SUNXI_FUNCTION(0x4, "tcon"),		/* tcon_trig */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_mosi */
		SUNXI_FUNCTION(0x6, "twi3"),		/* twi3_sda */
		SUNXI_FUNCTION(0x7, "uart2"),		/* uart2_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 19),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_hsync */
		SUNXI_FUNCTION(0x3, "pwm10"),		/* pwm10 */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_miso */
		SUNXI_FUNCTION(0x6, "twi2"),		/* twi2_sck */
		SUNXI_FUNCTION(0x7, "uart2"),		/* uart2_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 20),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd"),		/* lcd_vsync */
		SUNXI_FUNCTION(0x4, "rmii"),		/* rmii */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_cs0 */
		SUNXI_FUNCTION(0x6, "twi2"),		/* twi2_sda */
		SUNXI_FUNCTION(0x7, "uart2"),		/* uart2_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 21),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm9"),		/* pwm9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 22),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank E */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_pclk */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxd/rmii_rxd1 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_mclk */
		SUNXI_FUNCTION(0x5, "pwm0"),		/* pwm0 */
		SUNXI_FUNCTION(0x6, "sdc1"),		/* sdc_clk */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_tx */
		SUNXI_FUNCTION(0x8, "twi3"),		/* twi3_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi_mclk"),		/* ncsi_mclk */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxck/rmii_txck */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_bclk */
		SUNXI_FUNCTION(0x5, "pwm1"),		/* pwm1 */
		SUNXI_FUNCTION(0x6, "sdc1"),		/* sdc_cmd */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_rx */
		SUNXI_FUNCTION(0x8, "twi3"),		/* twi3_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_hsync */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxctl/rmii_crs_dv */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_lclk */
		SUNXI_FUNCTION(0x5, "pwm2"),		/* pwm2 */
		SUNXI_FUNCTION(0x6, "sdc1"),		/* sdc_d0 */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_cts */
		SUNXI_FUNCTION(0x8, "twi1"),		/* twi1_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_vsync */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxd0/rmii_rxd0 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_din0 */
		SUNXI_FUNCTION(0x5, "pwm3"),		/* pwm3 */
		SUNXI_FUNCTION(0x6, "sdc1"),		/* sdc_d1 */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_rts */
		SUNXI_FUNCTION(0x8, "twi1"),		/* twi1_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d0 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_txd0/rmii_txd0 */
		SUNXI_FUNCTION(0x4, "i2s1"),		/* i2s1_dout0 */
		SUNXI_FUNCTION(0x5, "pwm4"),		/* pwm4 */
		SUNXI_FUNCTION(0x6, "sdc1"),		/* sdc_d2 */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_sck */
		SUNXI_FUNCTION(0x8, "twi0"),		/* twi0_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d1 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_txd1/rmii_txd1 */
		SUNXI_FUNCTION(0x5, "pwm5"),		/* pwm5 */
		SUNXI_FUNCTION(0x6, "sdc1"),		/* sdc_d3 */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_sda */
		SUNXI_FUNCTION(0x8, "twi0"),		/* twi0_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d2 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_txctl/rmii_txen */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d2 */
		SUNXI_FUNCTION(0x5, "pwm6"),		/* pwm6 */
		SUNXI_FUNCTION(0x6, "uart1"),		/* uart1_tx */
		SUNXI_FUNCTION(0x8, "twi4"),		/* twi4_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d3 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_clkin/rmii_rxer */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d15 */
		SUNXI_FUNCTION(0x5, "pwm7"),		/* pwm7 */
		SUNXI_FUNCTION(0x6, "uart1"),		/* uart1_tx */
		SUNXI_FUNCTION(0x7, "i2s1"),		/* i2s1_dout0 */
		SUNXI_FUNCTION(0x8, "twi4"),		/* twi4_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d4 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* mdc */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d18 */
		SUNXI_FUNCTION(0x5, "pwm8"),		/* pwm8 */
		SUNXI_FUNCTION(0x6, "wiegand"),		/* wiegand_d0 */
		SUNXI_FUNCTION(0x7, "i2s1"),		/* i2s1_din0 */
		SUNXI_FUNCTION(0x8, "twi1"),		/* twi1_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d5 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* mdio */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d19 */
		SUNXI_FUNCTION(0x5, "pwm9"),		/* pwm9 */
		SUNXI_FUNCTION(0x6, "wiegand"),		/* wiegand_d1 */
		SUNXI_FUNCTION(0x7, "i2s1"),		/* i2s1_lrck */
		SUNXI_FUNCTION(0x8, "twi1"),		/* twi1_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d6 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* ephy_25m */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d20 */
		SUNXI_FUNCTION(0x5, "pwm10"),		/* pwm10 */
		SUNXI_FUNCTION(0x6, "uart2"),		/* uart2_rts */
		SUNXI_FUNCTION(0x7, "i2s1"),		/* i2s1_bclk */
		SUNXI_FUNCTION(0x8, "wiegand"),		/* wiegand_d0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d7 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxd3 */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d21 */
		SUNXI_FUNCTION(0x5, "csi"),		/* csi_sm_vs */
		SUNXI_FUNCTION(0x6, "uart2"),		/* uart2_cts */
		SUNXI_FUNCTION(0x7, "i2s1"),		/* i2s1_mclk */
		SUNXI_FUNCTION(0x8, "wiegand"),		/* wiegand_d1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d8 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxd2 */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d22 */
		SUNXI_FUNCTION(0x5, "mipi_csi_mclk0"),	/* mipi_csi_mclk0 */
		SUNXI_FUNCTION(0x6, "uart2"),		/* uart2_tx */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d9 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_rxck */
		SUNXI_FUNCTION(0x4, "lcd"),		/* lcd_d23 */
		SUNXI_FUNCTION(0x5, "mipi_csi_mclk1"),	/* mipi_csi_mclk1 */
		SUNXI_FUNCTION(0x6, "uart2"),		/* uart2_rx */
		SUNXI_FUNCTION(0x7, "uart3"),		/* uart3_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d10 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_txd3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "ncsi"),		/* ncsi_d11 */
		SUNXI_FUNCTION(0x3, "rgmii"),		/* rgmii_txd2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),		/* twi0_sck */
		SUNXI_FUNCTION(0x3, "twi4"),		/* twi4_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),		/* twi0_sda */
		SUNXI_FUNCTION(0x3, "twi4"),		/* twi4_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),		/* sdc0_d1 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* jtag_ms */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_clk */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_clk */
		SUNXI_FUNCTION(0x6, "r_jtag"),		/* r_jtag_ms */
		SUNXI_FUNCTION(0x7, "cpu"),		/* cpu_bist0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),		/* sdc0_d0 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* jtag_di */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_mosi */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_mosi */
		SUNXI_FUNCTION(0x6, "r_jtag"),		/* r_jtag_di */
		SUNXI_FUNCTION(0x7, "cpu"),		/* cpu_bist1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),		/* sdc0_clk */
		SUNXI_FUNCTION(0x3, "uart0"),		/* uart0_tx */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_miso */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_miso */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),		/* sdc0_cmd */
		SUNXI_FUNCTION(0x3, "jtag"),		/* jtag_do */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_cs0 */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_cs0 */
		SUNXI_FUNCTION(0x6, "r_jtag"),		/* r_jtag_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),		/* sdc0_d3 */
		SUNXI_FUNCTION(0x3, "uart0"),		/* uart0_rx */
		SUNXI_FUNCTION(0x4, "spi0"),		/* spi0_cs1 */
		SUNXI_FUNCTION(0x5, "spi2"),		/* spi2_cs1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),		/* sdc0_d2 */
		SUNXI_FUNCTION(0x3, "jtag"),		/* jtag_ck */
		SUNXI_FUNCTION(0x6, "r_jtag"),		/* r_jtag_ck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "dbg_clk"),		/* dbg_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),		/* sdc1_clk */
		SUNXI_FUNCTION(0x3, "lcd"),		/* lcd_d0 */
		SUNXI_FUNCTION(0x4, "uart3"),		/* uart3_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),		/* sdc1_cmd */
		SUNXI_FUNCTION(0x3, "lcd"),		/* lcd_d1 */
		SUNXI_FUNCTION(0x4, "uart3"),		/* uart3_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),		/* sdc1_d0 */
		SUNXI_FUNCTION(0x3, "lcd"),		/* lcd_d8 */
		SUNXI_FUNCTION(0x4, "uart3"),		/* uart3_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),		/* sdc1_d1 */
		SUNXI_FUNCTION(0x3, "lcd"),		/* lcd_d9 */
		SUNXI_FUNCTION(0x4, "uart3"),		/* uart3_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),		/* sdc1_d2 */
		SUNXI_FUNCTION(0x3, "lcd"),		/* lcd_d16 */
		SUNXI_FUNCTION(0x4, "uart1"),		/* uart1_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),		/* sdc1_d3 */
		SUNXI_FUNCTION(0x3, "lcd"),		/* lcd_d17 */
		SUNXI_FUNCTION(0x4, "uart1"),		/* uart1_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),		/* twi4_sck */
		SUNXI_FUNCTION(0x3, "clk"),		/* clk_fanout0 */
		SUNXI_FUNCTION(0x4, "uart1"),		/* uart1_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),		/* twi4_sda */
		SUNXI_FUNCTION(0x3, "clk"),		/* clk_fanout1 */
		SUNXI_FUNCTION(0x4, "uart1"),		/* uart1_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 7),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank H */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0"),		/* pwm0 */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* i2s0_mclk */
		SUNXI_FUNCTION(0x4, "spi1"),		/* spi1_clk */
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_tx */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm1"),		/* pwm1 */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* i2s0_bclk */
		SUNXI_FUNCTION(0x4, "spi1"),		/* spi1_mosi */
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_rx */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm2"),		/* pwm2 */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* i2s0_lrck */
		SUNXI_FUNCTION(0x4, "spi1"),		/* spi1_miso */
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_cts */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm3"),		/* pwm3 */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* i2s0_dout0 */
		SUNXI_FUNCTION(0x4, "spi1"),		/* spi1_cs0 */
		SUNXI_FUNCTION(0x5, "uart3"),		/* uart3_rts */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_data0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm4"),		/* pwm4 */
		SUNXI_FUNCTION(0x3, "i2s0"),		/* i2s0_din0 */
		SUNXI_FUNCTION(0x4, "spi1"),		/* spi1_cs1 */
		SUNXI_FUNCTION(0x5, "clk"),		/* clk_fanout2 */
		SUNXI_FUNCTION(0x6, "dmic"),		/* dmic_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm5"),		/* pwm5 */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_rxd1 */
		SUNXI_FUNCTION(0x4, "twi2"),		/* twi2_sck */
		SUNXI_FUNCTION(0x5, "uart2"),		/* uart2_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 5),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm6"),		/* pwm6 */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_rxd0 */
		SUNXI_FUNCTION(0x4, "twi2"),		/* twi2_sda */
		SUNXI_FUNCTION(0x5, "uart2"),		/* uart2_rx */
		SUNXI_FUNCTION(0x6, "i2s1"),		/* i2s1_mclk */
		SUNXI_FUNCTION(0x7, "dmic"),		/* dmic_data3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 6),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm7"),		/* pwm7 */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_crs_dv */
		SUNXI_FUNCTION(0x4, "uart0"),		/* uart0_tx */
		SUNXI_FUNCTION(0x5, "uart2"),		/* uart2_rts */
		SUNXI_FUNCTION(0x6, "i2s1"),		/* i2s1_bclk */
		SUNXI_FUNCTION(0x7, "dmic"),		/* dmic_data2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 7),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm8"),		/* pwm8 */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_rxer */
		SUNXI_FUNCTION(0x4, "uart0"),		/* uart0_rx */
		SUNXI_FUNCTION(0x5, "uart2"),		/* uart2_cts */
		SUNXI_FUNCTION(0x6, "i2s1"),		/* i2s1_lrck */
		SUNXI_FUNCTION(0x7, "dmic"),		/* dmic_data1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 8),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm9"),		/* pwm9 */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_txd1 */
		SUNXI_FUNCTION(0x4, "twi3"),		/* twi3_sck */
		SUNXI_FUNCTION(0x5, "uart0"),		/* uart0_tx */
		SUNXI_FUNCTION(0x6, "i2s1"),		/* i2s1_din0 */
		SUNXI_FUNCTION(0x7, "dmic"),		/* dmic_data0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 9),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm10"),		/* pwm10 */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_txd0 */
		SUNXI_FUNCTION(0x4, "twi3"),		/* twi3_sda */
		SUNXI_FUNCTION(0x5, "uart0"),		/* uart0_rx */
		SUNXI_FUNCTION(0x6, "i2s1"),		/* i2s1_dout0 */
		SUNXI_FUNCTION(0x7, "dmic"),		/* dmic_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 10),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "jtag"),		/* jtag_ms */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_txck */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_ms */
		SUNXI_FUNCTION(0x5, "twi2"),		/* twi2_sck */
		SUNXI_FUNCTION(0x6, "spi3"),		/* spi3_clk */
		SUNXI_FUNCTION(0x7, "clk"),		/* clk_fanout0 */
		SUNXI_FUNCTION(0x8, "pwm4"),		/* pwm4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 11),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "jtag"),		/* jtag_ck */
		SUNXI_FUNCTION(0x3, "rmii"),		/* rmii_txen */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_ck */
		SUNXI_FUNCTION(0x5, "twi2"),		/* twi2_sda */
		SUNXI_FUNCTION(0x6, "spi3"),		/* spi3_mosi */
		SUNXI_FUNCTION(0x7, "clk"),		/* clk_fanout1 */
		SUNXI_FUNCTION(0x8, "pwm5"),		/* pwm5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 12),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "jtag"),		/* jtag_do */
		SUNXI_FUNCTION(0x3, "mdc"),		/* mdc */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_do */
		SUNXI_FUNCTION(0x5, "twi3"),		/* twi3_sck */
		SUNXI_FUNCTION(0x6, "spi3"),		/* spi3_miso */
		SUNXI_FUNCTION(0x7, "wiegand"),		/* wiegand_d0 */
		SUNXI_FUNCTION(0x8, "pwm6"),		/* pwm6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 13),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "jtag"),		/* jtag_di */
		SUNXI_FUNCTION(0x3, "mdio"),		/* mdio */
		SUNXI_FUNCTION(0x4, "r_jtag"),		/* r_jtag_di */
		SUNXI_FUNCTION(0x5, "twi3"),		/* twi3_sda */
		SUNXI_FUNCTION(0x6, "spi3"),		/* spi3_cs0 */
		SUNXI_FUNCTION(0x7, "wiegand"),		/* wiegand_d1 */
		SUNXI_FUNCTION(0x8, "pwm7"),		/* pwm7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 14),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),		/* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),	/* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),		/* clk_fanout2 */
		SUNXI_FUNCTION(0x3, "ephy_25m"),	/* ephy_25m */
		SUNXI_FUNCTION(0x6, "spi3"),		/* spi3_cs1 */
		SUNXI_FUNCTION(0x8, "pwm8"),		/* pwm8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 15),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	/* bank I */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "mipi_csi_mclk0"),	/* mipi_csi_mclk0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 0),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi"),		/* csi_sm_hs */
		SUNXI_FUNCTION(0x4, "spi2"),		/* spi2_clk */
		SUNXI_FUNCTION(0x5, "twi1"),		/* twi1_sck */
		SUNXI_FUNCTION(0x6, "twi4"),		/* twi4_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 1),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x2, "csi"),		/* csi_sm_vs */
		SUNXI_FUNCTION(0x3, "tcon"),		/* tcon_trig */
		SUNXI_FUNCTION(0x4, "spi2"),		/* spi2_mosi */
		SUNXI_FUNCTION(0x5, "twi1"),		/* twi1_sda */
		SUNXI_FUNCTION(0x6, "twi4"),		/* twi4_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 2),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x4, "spi2"),		/* spi2_miso */
		SUNXI_FUNCTION(0x5, "twi0"),		/* twi0_sck */
		SUNXI_FUNCTION(0x6, "twi3"),		/* twi3_sck */
		SUNXI_FUNCTION(0x7, "clk"),		/* clk_fanout0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 3),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),
		SUNXI_FUNCTION(0x1, "gpio_out"),
		SUNXI_FUNCTION(0x4, "spi2"),		/* spi2_cs0 */
		SUNXI_FUNCTION(0x5, "twi0"),		/* twi0_sda */
		SUNXI_FUNCTION(0x6, "twi3"),		/* twi3_sda */
		SUNXI_FUNCTION(0x7, "clk"),		/* clk_fanout1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 4),	/* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),	/* io_disabled */
#endif
};

static const unsigned sun8iw21p1_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
	SUNXI_BANK_OFFSET('I', 'A'),
};

static const struct sunxi_pinctrl_desc sun8iw21p1_pinctrl_data = {
	.pins = sun8iw21p1_pins,
	.npins = ARRAY_SIZE(sun8iw21p1_pins),
	.pin_base = 0,
	.irq_banks = ARRAY_SIZE(sun8iw21p1_irq_bank_map),
	.irq_bank_map = sun8iw21p1_irq_bank_map,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL,
	.hw_type = SUNXI_PCTL_HW_TYPE_1,
	.pf_power_source_switch = true,
};

#if IS_ENABLED(CONFIG_PM_SLEEP)
static void *mem;
static int mem_size;
#endif

static int sun8iw21p1_pinctrl_probe(struct platform_device *pdev)
{
#if IS_ENABLED(CONFIG_PM_SLEEP)
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;
#endif

	return sunxi_bsp_pinctrl_init(pdev, &sun8iw21p1_pinctrl_data);
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
static int __maybe_unused sun8iw21p1_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int __maybe_unused sun8iw21p1_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_toio(pctl->membase, mem, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}
#endif

static struct of_device_id sun8iw21p1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun8iw21p1-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun8iw21p1_pinctrl_match);

#if IS_ENABLED(CONFIG_PM_SLEEP)
static const struct dev_pm_ops sun8iw21p1_pinctrl_pm_ops = {
	.suspend_noirq = sun8iw21p1_pinctrl_suspend_noirq,
	.resume_noirq = sun8iw21p1_pinctrl_resume_noirq,
};
#endif

static struct platform_driver sun8iw21p1_pinctrl_driver = {
	.probe	= sun8iw21p1_pinctrl_probe,
	.driver	= {
		.name		= "sun8iw21p1-pinctrl",
		.owner		= THIS_MODULE,
		.of_match_table	= sun8iw21p1_pinctrl_match,
#if IS_ENABLED(CONFIG_PM_SLEEP)
		.pm		= &sun8iw21p1_pinctrl_pm_ops,
#endif
	},
};

static int __init sun8iw21p1_pio_init(void)
{
	int ret;
	ret = platform_driver_register(&sun8iw21p1_pinctrl_driver);
	if (IS_ERR_VALUE(ret)) {
		sunxi_err(NULL, "register sun8iw21p1 pio controller failed\n");
		return -EINVAL;
	}
	return 0;
}
subsys_initcall(sun8iw21p1_pio_init);

MODULE_AUTHOR("weidonghui <weidonghui@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner sun8iw21p1 pio pinctrl driver");
MODULE_VERSION("1.0.9");
MODULE_LICENSE("GPL");
