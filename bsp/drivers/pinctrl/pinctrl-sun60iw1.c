// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>

#include "pinctrl-sunxi.h"

static const struct sunxi_desc_pin sun60iw1_pins[] = {
	/* Pin banks are: A B C D F G */

	/* bank A */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[3] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[2] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[1] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxd_di[0] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxd_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[3] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[2] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[1] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txd_do[0] */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clkrx_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_clkrx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_rxdv_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_rxdv_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_mdc_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_mdc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_md_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_md_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_txen_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_txen_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_di */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_clktx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(A, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "gmac0"),         /* gmac0_clktx_do */
		SUNXI_FUNCTION(0x2, "gmac1"),         /* gmac1_clktx_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "cpus_twi0"),     /* cpus_twi0_scl_di */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "cpus_twi0"),     /* cpus_twi0_sda_di */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_sda_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spdif"),         /* spdif_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "spdif"),         /* spdif_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
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
		SUNXI_FUNCTION(0x2, "cpus_uart0"),    /* cpus_uart0_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "cpus_uart0"),    /* cpus_uart0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "cpus_fir"),      /* cpus_fir_di */
		SUNXI_FUNCTION(0x3, "ir"),            /* ir_rx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 21),  /* eint */
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
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_ale_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_cen_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_ren_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_rb_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dq_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dq_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_wpn_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_re_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dqsn_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[0] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[1] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[2] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[3] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[4] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[5] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[6] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[7] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[8] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[9] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[10] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[11] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[12] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[13] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[14] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[15] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[16] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[17] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[18] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[19] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[20] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[21] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[22] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[23] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[24] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[25] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[26] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[27] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_ccmd_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_clk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x1, "spif"),          /* spif_dqs_di */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dqs_oe */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[4] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq4_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[5] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq5_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[5] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[6] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq6_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cdat_do[7] */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_dq7_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_ccmd_di */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_mosi_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_cle_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_mosi_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_cclk_do */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_ss_do[0] */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_rb_di[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_ss_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_hold_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_hold_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd2"),           /* sd2_ds_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_miso_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_cen_do[0] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_miso_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_wp_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_dq_do[1] */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_wp_do/di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spif"),          /* spif_sck_do */
		SUNXI_FUNCTION(0x4, "ndfc0"),         /* ndfc0_wen_do */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0_sck_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 31),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "ledc"),          /* ledc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir_tx_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d3_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d2_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic_d1_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

};

static const unsigned int sun60iw1_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('A', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
};

static const struct sunxi_pinctrl_desc sun60iw1_pinctrl_data = {
	.pins = sun60iw1_pins,
	.npins = ARRAY_SIZE(sun60iw1_pins),
	.irq_banks = ARRAY_SIZE(sun60iw1_irq_bank_map),
	.irq_bank_map = sun60iw1_irq_bank_map,
	.io_bias_cfg_variant = BIAS_VOLTAGE_PIO_POW_MODE_CTL,
	.pf_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_2,
};

static void *mem;
static int mem_size;

static int sun60iw1_pinctrl_probe(struct platform_device *pdev)
{
	struct resource *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	return sunxi_bsp_pinctrl_init(pdev, &sun60iw1_pinctrl_data);
}

static int __maybe_unused sun60iw1_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int __maybe_unused sun60iw1_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy(pctl->membase, mem, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static struct of_device_id sun60iw1_pinctrl_match[] = {
	{ .compatible = "allwinner,sun60iw1-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun60iw1_pinctrl_match);

static const struct dev_pm_ops sun60iw1_pinctrl_pm_ops = {
	.suspend_noirq = sun60iw1_pinctrl_suspend_noirq,
	.resume_noirq = sun60iw1_pinctrl_resume_noirq,
};

static struct platform_driver sun60iw1_pinctrl_driver = {
	.probe	= sun60iw1_pinctrl_probe,
	.driver	= {
		.name		= "sun60iw1-pinctrl",
		.pm = &sun60iw1_pinctrl_pm_ops,
		.of_match_table	= sun60iw1_pinctrl_match,
	},
};

static int __init sun60iw1_pio_init(void)
{
	return platform_driver_register(&sun60iw1_pinctrl_driver);
}
postcore_initcall(sun60iw1_pio_init);

MODULE_DESCRIPTION("Allwinner sun60iw1 pio pinctrl driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_VERSION("1.0.4");
