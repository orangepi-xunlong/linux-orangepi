// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/io.h>
#include <sunxi-sid.h>
#include "pinctrl-sunxi.h"

#define SUNXI_PINCTRL_VERSION	"0.6.8"

#define SUNXI_SOC_VER_B 	(0x1)

static const struct sunxi_desc_pin sun60iw2_pins[] = {
#if IS_ENABLED(CONFIG_AW_FPGA_S4) || IS_ENABLED(CONFIG_AW_FPGA_V7)
	/* Pin banks are: B C D E F G H */

	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "test"),          /* test */
		SUNXI_FUNCTION(0x3, "cpus_twi0"),     /* cpus_twi0_scl_di */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "test"),          /* test */
		SUNXI_FUNCTION(0x3, "cpus_twi0"),     /* cpus_twi0_sda_di */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0_sda_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1_sda_di */
		SUNXI_FUNCTION(0x4, "hdmi0"),         /* hdmi0_cec_in */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa"),         /* owa_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa"),         /* owa_di */
		SUNXI_FUNCTION(0x2, "//owa"),       /* //owa_mclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1_do */
		SUNXI_FUNCTION(0x2, "cpus_uart0"),    /* cpus_uart0_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1_di */
		SUNXI_FUNCTION(0x2, "cpus_uart0"),    /* cpus_uart0_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[1] */
		SUNXI_FUNCTION(0x2, "i2s2"),          /* i2s2_sd_di [1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "cpus_fir"),      /* cpus_fir_di */
		SUNXI_FUNCTION(0x3, "ir"),            /* ir_rx_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_di[0] */
		SUNXI_FUNCTION(0x2, "i2s1"),          /* i2s1_sd_di */
		SUNXI_FUNCTION(0x2, "i2s2"),          /* i2s2_sd_di [0] */
		SUNXI_FUNCTION(0x2, "i2s3"),          /* i2s3_sd_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_bclk_do */
		SUNXI_FUNCTION(0x3, "i2s1"),          /* i2s1_bclk_do */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_bclk_do */
		SUNXI_FUNCTION(0x5, "i2s3"),          /* i2s3_bclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_lrc_do */
		SUNXI_FUNCTION(0x3, "i2s1"),          /* i2s1_lrc_do */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_lrc_do */
		SUNXI_FUNCTION(0x5, "i2s3"),          /* i2s3_lrc_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[0] */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_sd_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[1] */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_sd_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[2] */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_sd_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 28),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_sd_do[3] */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_sd_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 28),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 29),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s0"),          /* i2s0_mclk_do */
		SUNXI_FUNCTION(0x3, "i2s1"),          /* i2s1_mclk_do */
		SUNXI_FUNCTION(0x4, "i2s2"),          /* i2s2_mclk_do */
		SUNXI_FUNCTION(0x5, "i2s3"),          /* i2s3_mclk_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 29),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2_scl_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2_sda_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 31),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_ale_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_cen_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_ren_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_rb_di[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dq_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dq_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_wpn_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_re_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ndfc0"),         /* ndfc0_dqsn_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[0] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[0] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[1] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[1] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[2] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[2] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[3] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[3] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[4] */
		SUNXI_FUNCTION(0x3, "lvds00"),        /* lvds00_in[4] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[5] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[0] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[5] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[6] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[1] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[7] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[2] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[8] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[3] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[8] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[9] */
		SUNXI_FUNCTION(0x3, "lvds01"),        /* lvds01_in[4] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[9] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[10] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[10] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[11] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[11] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[12] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[12] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[13] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[13] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[14] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[14] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[15] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[15] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[16] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[16] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[17] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[17] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[18] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[18] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[19] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[19] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[20] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[20] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[21] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[21] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[22] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[22] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[23] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[23] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[24] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[24] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[25] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[25] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[26] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[26] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dpss"),          /* dpss_rgb0_do[27] */
		SUNXI_FUNCTION(0x5, "dpss1"),         /* dpss1_rgb1_do[27] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */

	/* bank E */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_error */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_psyn */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_dvld */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[0] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[1] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[2] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[3] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[4] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[5] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ts00"),          /* ts00_data[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
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
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[6] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sd0"),           /* sd0_cdat_di[7] */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 7),   /* eint */
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

	/* bank H */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 30),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 for hdmi phy */
		SUNXI_FUNCTION(0x5, "hdmi0"),         /* hdmi0_ddc_scli */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 30),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 31),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 for hdmi phy */
		SUNXI_FUNCTION(0x5, "hdmi0"),         /* hdmi0_ddc_sdai */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 31),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#else
	/* Pin banks are: B C D E F G H I J K */
	/* bank B */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x6, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x7, "jtag"),          /* jtag */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x6, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x7, "jtag"),          /* jtag */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x6, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x7, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x8, "twi0"),          /* twi0_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x6, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x7, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x8, "twi0"),          /* twi0_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x3, "i2s0_mclk"),     /* i2s0_mclk */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x6, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x7, "trace"),         /* trace */
		SUNXI_FUNCTION(0x8, "twi1"),          /* twi1_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "i2s0_bclk"),     /* i2s0_bclk */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x7, "trace"),         /* trace */
		SUNXI_FUNCTION(0x8, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),           /* clk */
		SUNXI_FUNCTION(0x3, "i2s0_lrck"),     /* i2s0_lrck */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x7, "trace"),         /* trace */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),           /* clk */
		SUNXI_FUNCTION(0x3, "i2s0_dout0"),    /* i2s0_dout0 */
		SUNXI_FUNCTION(0x4, "i2s0_din1"),     /* i2s0_din1 */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x6, "owa0"),          /* owa0 */
		SUNXI_FUNCTION(0x7, "trace"),         /* trace */
		SUNXI_FUNCTION(0x8, "twi1"),          /* twi1_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),           /* clk */
		SUNXI_FUNCTION(0x3, "i2s0_din0"),     /* i2s0_din0 */
		SUNXI_FUNCTION(0x4, "i2s0_dout1"),    /* i2s0_dout1 */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION(0x6, "owa0"),          /* owa0 */
		SUNXI_FUNCTION(0x7, "trace"),         /* trace */
		SUNXI_FUNCTION(0x8, "twi1"),          /* twi1_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x3, "i2s0_din2"),     /* i2s0_din2 */
		SUNXI_FUNCTION(0x4, "i2s0_dout2"),    /* i2s0_dout2 */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x6, "watchdog"),      /* watchdog */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x8, "twi8"),          /* twi8_sck */
		SUNXI_FUNCTION(0x9, "twi0"),          /* twi0_sck */
		SUNXI_FUNCTION(0xa, "test"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(B, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x3, "i2s0_din3"),     /* i2s0_din3 */
		SUNXI_FUNCTION(0x4, "i2s0_dout3"),    /* i2s0_dout3 */
		SUNXI_FUNCTION(0x5, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x6, "pll"),           /* pll */
		SUNXI_FUNCTION(0x7, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x8, "twi8"),          /* twi8_sda */
		SUNXI_FUNCTION(0x9, "twi0"),          /* twi0_sda */
		SUNXI_FUNCTION(0xa, "test"),
		SUNXI_FUNCTION_IRQ_BANK(0xe, 0, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank C */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(C, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "nand"),          /* nand */
		SUNXI_FUNCTION(0x3, "sdc2"),          /* sdc2 */
		SUNXI_FUNCTION(0x4, "sdc3"),          /* sdc3 */
		SUNXI_FUNCTION(0x5, "spi0"),          /* spi0 */
		SUNXI_FUNCTION(0x6, "spif0"),         /* spif0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 1, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank D */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds0"),         /* lvds0 */
		SUNXI_FUNCTION(0x4, "dsi0"),          /* dsi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x7, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x7, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x7, "uart3"),         /* uart3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "spi1"),          /* spi1 */
		SUNXI_FUNCTION(0x7, "uart4"),         /* uart4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "lvds1"),         /* lvds1 */
		SUNXI_FUNCTION(0x4, "dsi1"),          /* dsi1 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION(0x7, "uart4"),         /* uart4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x3, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x7, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x8, "twi3"),          /* twi3_sck */
		SUNXI_FUNCTION(0x9, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x7, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x8, "twi3"),          /* twi3_sda */
		SUNXI_FUNCTION(0x9, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x5, "eink"),          /* eink */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x7, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x8, "twi2"),          /* twi2_sck */
		SUNXI_FUNCTION(0x9, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(D, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x7, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x8, "twi2"),          /* twi2_sda */
		SUNXI_FUNCTION(0x9, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 2, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank E */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi0"),         /* mcsi0 */
		SUNXI_FUNCTION(0x3, "csi0"),          /* csi0 */
		SUNXI_FUNCTION(0x5, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "csi1"),          /* csi1 */
		SUNXI_FUNCTION(0x5, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x3, "csi0"),          /* csi0 */
		SUNXI_FUNCTION(0x5, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "csi1"),          /* csi1 */
		SUNXI_FUNCTION(0x4, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x5, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x3, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x4, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x5, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_dcd */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi1"),         /* mcsi1 */
		SUNXI_FUNCTION(0x3, "pll"),           /* pll */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_dsr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),           /* clk */
		SUNXI_FUNCTION(0x3, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x4, "i2s3_din0"),/* i2s3_din0 */
		SUNXI_FUNCTION(0x5, "i2s3_dout1"),/* i2s3_dout1 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_dtr */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),           /* clk */
		SUNXI_FUNCTION(0x3, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x4, "i2s3_bclk"),/* i2s3_bclk */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "twi11"),         /* twi11_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "clk"),           /* clk */
		SUNXI_FUNCTION(0x3, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x4, "i2s3_lrck"),/* i2s3_lrck */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "twi11"),         /* twi11_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsi2"),         /* mcsi2 */
		SUNXI_FUNCTION(0x3, "tcon"),          /* tcon */
		SUNXI_FUNCTION(0x4, "i2s3_mclk"),/* i2s3_mclk */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart6"),         /* uart6_ri */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x3, "tcon"),          /* tcon */
		SUNXI_FUNCTION(0x4, "i2s3_din3"),/* i2s3_din3 */
		SUNXI_FUNCTION(0x5, "i2s3_dout3"),/* i2s3_dout3 */
		SUNXI_FUNCTION(0x6, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart6_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x4, "i2s3_din2"),/* i2s3_din2 */
		SUNXI_FUNCTION(0x5, "i2s3_dout2"),/* i2s3_dout2 */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x7, "ncsi0"),         /* ncsi0 */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart6_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x4, "i2s3_dout0"),/* i2s3_dout0 */
		SUNXI_FUNCTION(0x5, "i2s3_din1"),/* i2s3_din1 */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart6_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart6_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm1_8"),        /* pwm1_8 */
		SUNXI_FUNCTION(0x6, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x7, "ir"),            /* ir */
		SUNXI_FUNCTION(0x8, "twi9"),          /* twi9_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(E, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x5, "pwm1_9"),        /* pwm1_9 */
		SUNXI_FUNCTION(0x6, "ir"),            /* ir */
		SUNXI_FUNCTION(0x8, "twi9"),          /* twi9_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 3, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank F */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "trace"),         /* trace */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x6, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "trace"),         /* trace */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "trace"),         /* trace */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "trace"),         /* trace */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x6, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc0"),          /* sdc0 */
		SUNXI_FUNCTION(0x3, "jtag"),          /* jtag */
		SUNXI_FUNCTION(0x4, "trace"),         /* trace */
		SUNXI_FUNCTION(0x5, "uart5"),         /* uart5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(F, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 4, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank G */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "sdc1"),          /* sdc1 */
		SUNXI_FUNCTION(0x3, "lcd0"),          /* lcd0 */
		SUNXI_FUNCTION(0x4, "pwm1_6"),        /* pwm1_6 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "pwm1_7"),        /* pwm1_7 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "pwm1_8"),        /* pwm1_8 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x6, "twi0"),          /* twi0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_mclk"),     /* i2s1_mclk */
		SUNXI_FUNCTION(0x3, "clk"),           /* clk */
		SUNXI_FUNCTION(0x4, "pwm1_9"),        /* pwm1_9 */
		SUNXI_FUNCTION(0x5, "lpc"),           /* lpc */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_bclk"),     /* i2s1_bclk */
		SUNXI_FUNCTION(0x3, "twi12"),         /* twi12 */
		SUNXI_FUNCTION(0x4, "twi9"),          /* twi9 */
		SUNXI_FUNCTION(0x5, "sgpio"),         /* sgpio */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_lrck"),     /* i2s1_lrck */
		SUNXI_FUNCTION(0x3, "twi12"),         /* twi12 */
		SUNXI_FUNCTION(0x4, "twi9"),          /* twi9 */
		SUNXI_FUNCTION(0x5, "sgpio"),         /* sgpio */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_dout0"),    /* i2s1_dout0 */
		SUNXI_FUNCTION(0x3, "i2s1_din1"),     /* i2s1_din1 */
		SUNXI_FUNCTION(0x4, "twi6"),          /* twi6 */
		SUNXI_FUNCTION(0x5, "sgpio"),         /* sgpio */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x7, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(G, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "i2s1_din0"),     /* i2s1_din0 */
		SUNXI_FUNCTION(0x3, "i2s1_dout1"),    /* i2s1_dout1 */
		SUNXI_FUNCTION(0x4, "twi6"),          /* twi6 */
		SUNXI_FUNCTION(0x5, "sgpio"),         /* sgpio */
		SUNXI_FUNCTION(0x6, "dmic"),          /* dmic */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 5, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank H */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout2"),    /* i2s2_dout2 */
		SUNXI_FUNCTION(0x4, "i2s2_din2"),     /* i2s2_din2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi0"),          /* twi0 */
		SUNXI_FUNCTION(0x3, "i2s2_dout3"),    /* i2s2_dout3 */
		SUNXI_FUNCTION(0x4, "i2s2_din3"),     /* i2s2_din3 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "i2s2_mclk"),     /* i2s2_mclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x4, "i2s2_bclk"),     /* i2s2_bclk */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x4, "i2s2_lrck"),     /* i2s2_lrck */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "i2s2_din1"),     /* i2s2_din1 */
		SUNXI_FUNCTION(0x4, "i2s2_dout0"),    /* i2s2_dout0 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "i2s2_dout1"),    /* i2s2_dout1 */
		SUNXI_FUNCTION(0x4, "i2s2_din0"),     /* i2s2_din0 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x7, "spi1"),          /* spi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "dmic"),          /* dmic */
		SUNXI_FUNCTION(0x3, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi7"),          /* twi7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION(0x6, "twi7"),          /* twi7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x3, "ledc"),          /* ledc */
		SUNXI_FUNCTION(0x4, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x5, "rgmii0"),        /* rgmii0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x7, "twi6"),          /* twi6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "ir"),            /* ir */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x7, "twi6"),          /* twi6 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "pwm1_8"),        /* pwm1_8 */
		SUNXI_FUNCTION(0x7, "twi7"),          /* twi7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(H, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x4, "spi2"),          /* spi2 */
		SUNXI_FUNCTION(0x5, "pwm1_9"),        /* pwm1_9 */
		SUNXI_FUNCTION(0x7, "twi7"),          /* twi7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 6, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank I */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x6, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x7, "twi4"),          /* twi4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x3, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x6, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x7, "twi4"),          /* twi4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x6, "twi11"),         /* twi11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x6, "twi11"),         /* twi11 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x3, "uart0"),         /* uart0 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "ir"),            /* ir */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "spi4"),          /* spi4 */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa0"),          /* owa0 */
		SUNXI_FUNCTION(0x4, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "owa0"),          /* owa0 */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "ir"),            /* ir */
		SUNXI_FUNCTION(0x3, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x5, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x3, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x4, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x5, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "pwm1_6"),        /* pwm1_6 */
		SUNXI_FUNCTION(0x6, "twi8"),          /* twi8 */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(I, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x3, "uart1"),         /* uart1 */
		SUNXI_FUNCTION(0x5, "pwm1_7"),        /* pwm1_7 */
		SUNXI_FUNCTION(0x6, "twi8"),          /* twi8 */
		SUNXI_FUNCTION(0x7, "twi2"),          /* twi2 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 7, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank J */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds2"),         /* lvds2 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x5, "rgmii1"),        /* rgmii1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "i2s4_bclk"),/* i2s4_bclk<mux_edp> */
		SUNXI_FUNCTION(0x7, "twi7"),          /* twi7 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "i2s4_mclk"),/* i2s4_mclk<mux_edp> */
		SUNXI_FUNCTION(0x7, "twi7"),          /* twi7 */
		SUNXI_FUNCTION(0x8, "spi3"),          /* spi3_sc1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "i2s4_lrck"),/* i2s4_lrck<mux_edp> */
		SUNXI_FUNCTION(0x6, "pwm0_0"),        /* pwm0_0 */
		SUNXI_FUNCTION(0x8, "twi4"),          /* twi4_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "lvds3"),         /* lvds3 */
		SUNXI_FUNCTION(0x4, "uart5"),         /* uart5 */
		SUNXI_FUNCTION(0x5, "i2s4_din0"),/* i2s4_din0<mux_edp> */
		SUNXI_FUNCTION(0x6, "i2s4_dout1"),/* i2s4_dout1<mux_edp> */
		SUNXI_FUNCTION(0x8, "twi4"),          /* twi4_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "i2s4_dout0"),/* i2s4_dout0<mux_edp> */
		SUNXI_FUNCTION(0x6, "i2s4_din1"),/* i2s4_din1<mux_edp> */
		SUNXI_FUNCTION(0x7, "twi12"),         /* twi12 */
		SUNXI_FUNCTION(0x8, "twi10"),         /* twi10_sda */
		SUNXI_FUNCTION(0x9, "lcd0"),          /* lcd0_d0 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_3"),        /* pwm1_3 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "twi12"),         /* twi12 */
		SUNXI_FUNCTION(0x8, "twi10"),          /* twi10_sck */
		SUNXI_FUNCTION(0x9, "lcd0"),          /* lcd0_d1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_4"),        /* pwm1_4 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "twi7"),          /* twi7 */
		SUNXI_FUNCTION(0x7, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x8, "twi11"),          /* twi11_sck */
		SUNXI_FUNCTION(0x9, "lcd0"),          /* lcd0_d8 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_5"),        /* pwm1_5 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "twi7"),          /* twi7 */
		SUNXI_FUNCTION(0x7, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x8, "twi11"),         /* twi11_sda */
		SUNXI_FUNCTION(0x9, "lcd0"),          /* lcd0_d9 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_6"),        /* pwm1_6 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x7, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x8, "spi3"),          /* spi3_clk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_7"),        /* pwm1_7 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x7, "twi4"),          /* twi4 */
		SUNXI_FUNCTION(0x8, "spi3"),          /* spi3_mosi */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 26),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_8"),        /* pwm1_8 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x7, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x8, "spi3"),          /* spi3_miso */
		SUNXI_FUNCTION(0x9, "lcd0"),          /* lcd0_d16 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 26),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(J, 27),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "lcd1"),          /* lcd1 */
		SUNXI_FUNCTION(0x3, "pwm1_9"),        /* pwm1_9 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "dsi"),           /* dsi */
		SUNXI_FUNCTION(0x7, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x8, "spi3"),          /* spi3_cs0 */
		SUNXI_FUNCTION(0x9, "lcd0"),          /* lcd0_d17 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 8, 27),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	/* bank K */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 0),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "i2s4_bclk"),/* i2s4_bclk */
		SUNXI_FUNCTION(0x5, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "sgpio"),         /* sgpio_sload */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 0),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 1),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "i2s4_mclk"),/* i2s4_mclk */
		SUNXI_FUNCTION(0x5, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x6, "twi1"),          /* twi1 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "sgpio"),         /* sgpio_sclk */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 1),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 2),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "i2s4_lrck"),/* i2s4_lrck */
		SUNXI_FUNCTION(0x5, "hdmi"),          /* hdmi */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "sgpio"),         /* sgpio_sdatain */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 2),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 3),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x4, "i2s4_din0"),/* i2s4_din0 */
		SUNXI_FUNCTION(0x5, "i2s4_dout1"),/* i2s4_dout1 */
		SUNXI_FUNCTION(0x6, "twi5"),          /* twi5 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "sgpio"),         /* sgpio_sdataout */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 3),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 4),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x4, "i2s4_dout0"),/* i2s4_dout0 */
		SUNXI_FUNCTION(0x5, "i2s4_din1"),/* i2s4_din1 */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "jtag"),         /* jtag_mas_ms */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 4),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 5),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x4, "pwm1_8"),        /* pwm1_8 */
		SUNXI_FUNCTION(0x5, "pwm1_9"),        /* pwm1_9 */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "jtag"),         /* jtag_mas_ck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 5),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 6),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "jtag"),         /* jtag_mas_do */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 6),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 7),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "jtag"),         /* jtag_mas_di */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 7),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 8),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "mcsi0"),         /* mcsi0 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x6, "spi3"),          /* spi3 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "jtag"),         /* jtag_mas_ntrst */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 8),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 9),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsia"),         /* mcsia */
		SUNXI_FUNCTION(0x3, "mcsi2"),         /* mcsi2 */
		SUNXI_FUNCTION(0x4, "uart4"),         /* uart4 */
		SUNXI_FUNCTION(0x5, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "jtag"),         /* jtag_mas_srst */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 9),   /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 10),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x5, "pwm0_3"),        /* pwm0_3 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 10),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 11),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x5, "pwm0_4"),        /* pwm0_4 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 11),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 12),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x5, "pwm0_5"),        /* pwm0_5 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 12),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 13),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "uart6"),         /* uart6 */
		SUNXI_FUNCTION(0x5, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 13),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 14),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "pcie"),          /* pcie */
		SUNXI_FUNCTION(0x5, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 14),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 15),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x5, "pwm0_8"),        /* pwm0_8 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 15),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 16),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "pwm0_9"),        /* pwm0_9 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 16),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 17),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "pwm1_0"),        /* pwm1_0 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 17),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 18),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "mcsi1"),         /* mcsi1 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "pwm1_1"),        /* pwm1_1 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "twi9"),          /* twi9_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 18),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 19),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsib"),         /* mcsib */
		SUNXI_FUNCTION(0x3, "mcsi2"),         /* mcsi2 */
		SUNXI_FUNCTION(0x4, "uart2"),         /* uart2 */
		SUNXI_FUNCTION(0x5, "pwm1_2"),        /* pwm1_2 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "twi9"),          /* twi9_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 19),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 20),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),         /* mcsic */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm0_1"),        /* pwm0_1 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart1_rts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 20),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 21),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),         /* mcsic */
		SUNXI_FUNCTION(0x3, "twi2"),          /* twi2 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm0_2"),        /* pwm0_2 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart1_cts */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 21),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 22),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),         /* mcsic */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm1_6"),        /* pwm1_6 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart1_tx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 22),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 23),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),         /* mcsic */
		SUNXI_FUNCTION(0x3, "twi3"),          /* twi3 */
		SUNXI_FUNCTION(0x4, "uart3"),         /* uart3 */
		SUNXI_FUNCTION(0x5, "pwm1_7"),        /* pwm1_7 */
		SUNXI_FUNCTION(0x7, "ncsi1"),         /* ncsi1 */
		SUNXI_FUNCTION(0x8, "uart1"),         /* uart1_rx */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 23),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 24),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),         /* mcsic */
		SUNXI_FUNCTION(0x3, "mcsi0"),         /* mcsi0 */
		SUNXI_FUNCTION(0x4, "pwm0_6"),        /* pwm0_6 */
		SUNXI_FUNCTION(0x5, "twi12"),         /* twi12 */
		SUNXI_FUNCTION(0x8, "twi10"),         /* twi10_sda */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 24),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
	SUNXI_PIN(SUNXI_PINCTRL_PIN(K, 25),
		SUNXI_FUNCTION(0x0, "gpio_in"),       /* gpio_in */
		SUNXI_FUNCTION(0x1, "gpio_out"),      /* gpio_out */
		SUNXI_FUNCTION(0x2, "mcsic"),         /* mcsic */
		SUNXI_FUNCTION(0x3, "mcsi1"),         /* mcsi1 */
		SUNXI_FUNCTION(0x4, "pwm0_7"),        /* pwm0_7 */
		SUNXI_FUNCTION(0x5, "twi12"),         /* twi12 */
		SUNXI_FUNCTION(0x8, "twi10"),         /* twi10_sck */
		SUNXI_FUNCTION_IRQ_BANK(0xe, 9, 25),  /* eint */
		SUNXI_FUNCTION(0xf, "io_disabled")),  /* io_disabled */
#endif
};

static const unsigned int sun60iw2_bank_base[] = {
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	SUNXI_BANK_OFFSET('I', 'A'),
	SUNXI_BANK_OFFSET('J', 'A'),
	SUNXI_BANK_OFFSET('K', 'A'),
#endif
};

static const unsigned int sun60iw2_irq_bank_map[] = {
	SUNXI_BANK_OFFSET('B', 'A'),
	SUNXI_BANK_OFFSET('C', 'A'),
	SUNXI_BANK_OFFSET('D', 'A'),
	SUNXI_BANK_OFFSET('E', 'A'),
	SUNXI_BANK_OFFSET('F', 'A'),
	SUNXI_BANK_OFFSET('G', 'A'),
	SUNXI_BANK_OFFSET('H', 'A'),
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	SUNXI_BANK_OFFSET('I', 'A'),
	SUNXI_BANK_OFFSET('J', 'A'),
	SUNXI_BANK_OFFSET('K', 'A'),
#endif
};

static const struct sunxi_pinctrl_desc sun60iw2_pinctrl_data = {
	.pins = sun60iw2_pins,
	.npins = ARRAY_SIZE(sun60iw2_pins),
	.banks = ARRAY_SIZE(sun60iw2_bank_base),
	.bank_base = sun60iw2_bank_base,
	.irq_banks = ARRAY_SIZE(sun60iw2_irq_bank_map),
	.irq_bank_map = sun60iw2_irq_bank_map,
	.pf_power_source_switch = true,
	.auto_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_4,
};

static const struct sunxi_pinctrl_desc sun60iw2_b_pinctrl_data = {
	.pins = sun60iw2_pins,
	.npins = ARRAY_SIZE(sun60iw2_pins),
	.banks = ARRAY_SIZE(sun60iw2_bank_base),
	.bank_base = sun60iw2_bank_base,
	.irq_banks = ARRAY_SIZE(sun60iw2_irq_bank_map),
	.irq_bank_map = sun60iw2_irq_bank_map,
	.pf_power_source_switch = true,
	.auto_power_source_switch = true,
	.hw_type = SUNXI_PCTL_HW_TYPE_10,
};

static void *mem;
static int mem_size;

static const struct sunxi_pinctrl_desc *sun60iw2_get_soc_ver(void)
{
	unsigned int ver;

	ver = sunxi_get_soc_ver();
	if (ver == SUNXI_SOC_VER_B)
		return &sun60iw2_b_pinctrl_data;
	else
		return &sun60iw2_pinctrl_data;
}

static int sun60iw2_pinctrl_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct sunxi_pinctrl_desc const *desc = sun60iw2_get_soc_ver();

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -EINVAL;
	mem_size = resource_size(res);

	if (mem)
		return -ENOMEM;
	mem = devm_kzalloc(&pdev->dev, mem_size, GFP_KERNEL);
	if (!mem)
		return -ENOMEM;

	sunxi_info(NULL, "sunxi pinctrl version: %s\n", SUNXI_PINCTRL_VERSION);

	return sunxi_bsp_pinctrl_init(pdev, desc);
}

static int __maybe_unused sun60iw2_pinctrl_suspend_noirq(struct device *dev)
{
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	memcpy_fromio(mem, pctl->membase, mem_size);
	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static int __maybe_unused sun60iw2_pinctrl_resume_noirq(struct device *dev)
{
	struct sunxi_pinctrl_desc const *desc = sun60iw2_get_soc_ver();
	struct sunxi_pinctrl *pctl = dev_get_drvdata(dev);
	unsigned long flags;
	int idx, bank;
	int initial_bank_offset = sunxi_pinctrl_hw_info[desc->hw_type].initial_bank_offset;
	int bank_mem_size = sunxi_pinctrl_hw_info[desc->hw_type].bank_mem_size;
	int irq_cfg_reg = sunxi_pinctrl_hw_info[desc->hw_type].irq_cfg_reg;
	int irq_mem_size = sunxi_pinctrl_hw_info[desc->hw_type].irq_mem_size;
	int irq_mem_used = sunxi_pinctrl_hw_info[desc->hw_type].irq_mem_used;
	int data_clr_regs_offset = sunxi_pinctrl_hw_info[desc->hw_type].data_clr_regs_offset;

	raw_spin_lock_irqsave(&pctl->lock, flags);
	/* recover PX_REG_BASE and GPIO_POW_MOD/CTL/VAL */
	memcpy_toio(pctl->membase, mem, 0x80);

	/* clear data_clr_reg before recover data_reg to ensure data_reg correctness */
	for (idx = 0; idx < desc->banks; idx++) {
		bank = desc->bank_base[idx];
		*(unsigned int *)(mem + initial_bank_offset + bank * bank_mem_size + data_clr_regs_offset) = 0x0;
	}

	for (idx = 0; idx < desc->banks; idx++) {
		bank = desc->bank_base[idx];
		/* cfg/dat/drv/pull */
		memcpy_toio(pctl->membase + initial_bank_offset + bank * bank_mem_size,
			    mem + initial_bank_offset + bank * bank_mem_size,
			    0x40);
	}

	for (idx = 0; idx < desc->irq_banks; idx++) {
		bank = desc->irq_bank_map[idx];
		/* irq cfg/ctr/status/deb */
		memcpy_toio(pctl->membase + irq_cfg_reg + bank * irq_mem_size,
			    mem + irq_cfg_reg + bank * irq_mem_size,
			   irq_mem_used);
	}

	raw_spin_unlock_irqrestore(&pctl->lock, flags);

	return 0;
}

static struct of_device_id sun60iw2_pinctrl_match[] = {
	{ .compatible = "allwinner,sun60iw2-pinctrl", },
	{}
};
MODULE_DEVICE_TABLE(of, sun60iw2_pinctrl_match);

static const struct dev_pm_ops sun60iw2_pinctrl_pm_ops = {
	.suspend_noirq = sun60iw2_pinctrl_suspend_noirq,
	.resume_noirq = sun60iw2_pinctrl_resume_noirq,
};

static struct platform_driver sun60iw2_pinctrl_driver = {
	.probe	= sun60iw2_pinctrl_probe,
	.driver	= {
		.name		= "sun60iw2-pinctrl",
		.pm = &sun60iw2_pinctrl_pm_ops,
		.of_match_table	= sun60iw2_pinctrl_match,
	},
};

static int __init sun60iw2_pio_init(void)
{
	return platform_driver_register(&sun60iw2_pinctrl_driver);
}
postcore_initcall(sun60iw2_pio_init);

MODULE_DESCRIPTION("Allwinner sun60iw2 pio pinctrl driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(SUNXI_PINCTRL_VERSION);
