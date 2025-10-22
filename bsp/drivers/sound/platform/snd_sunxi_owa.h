/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_OWA_H
#define __SND_SUNXI_OWA_H

#include "snd_sunxi_pcm.h"
#include "snd_sunxi_rxsync.h"
#include "snd_sunxi_common.h"
#include "snd_sunxi_owa_rx61937.h"

#define	SUNXI_OWA_CTL		0x00
#define	SUNXI_OWA_TXCFG		0x04
#define	SUNXI_OWA_RXCFG		0x08
#define SUNXI_OWA_INT_STA	0x0C
#define	SUNXI_OWA_RXFIFO	0x10
#define	SUNXI_OWA_FIFO_CTL	0x14
#define	SUNXI_OWA_FIFO_STA	0x18
#define	SUNXI_OWA_INT		0x1C
#define SUNXI_OWA_TXFIFO	0x20
#define	SUNXI_OWA_TXCNT		0x24
#define	SUNXI_OWA_RXCNT		0x28
#define	SUNXI_OWA_TXCH_STA0	0x2C
#define	SUNXI_OWA_TXCH_STA1	0x30
#define	SUNXI_OWA_RXCH_STA0	0x34
#define	SUNXI_OWA_RXCH_STA1	0x38

#if IS_ENABLED(CONFIG_SND_SOC_SUNXI_OWA_RXIEC61937)
#define	SUNXI_OWA_EXP_VER	0x58
#else
#define	SUNXI_OWA_EXP_VER	0x40
#endif

#define SUNXI_OWA_REG_MAX	SUNXI_OWA_EXP_VER

/* SUNXI_OWA_CTL register */
#define	CTL_RESET		0
#define	CTL_GEN_EN		1
#define	CTL_LOOP_EN		2
#define CTL_MCLKDIV		4
#define	CTL_RESET_RX		0

/* SUNXI_OWA_TXCFG register */
#define	TXCFG_TXEN		0
/* Chan status generated form TX_CHSTA */
#define	TXCFG_CHAN_STA_EN	1
#define	TXCFG_SAMPLE_BIT	2
#define	TXCFG_CLK_DIV_RATIO	4
#define	TXCFG_DATA_TYPE		16
/* Only valid in PCM mode */
#define	TXCFG_ASS		17
#define	TXCFG_SINGLE_MOD	31

/* SUNXI_OWA_RXCFG register */
#define	RXCFG_RXEN		0
#define	RXCFG_CHSR_CP		1
#define	RXCFG_CHST_SRC		3
#define	RXCFG_LOCK_FLAG		4

/* SUNXI_OWA_FIFO_CTL register */
#define	FIFO_CTL_RXOM		0
#define	FIFO_CTL_TXIM		2
#define	FIFO_CTL_RXTL		4
#define	FIFO_CTL_TXTL		12
#define	OWA_RX_SYNC_EN		20
#define	OWA_RX_SYNC_EN_START	21
#define	FIFO_CTL_FRX		29
#define	FIFO_CTL_FTX		30
#define	FIFO_CTL_HUBEN		31
#define	CTL_TXTL_MASK		0xFF
#define	CTL_TXTL_DEFAULT	0x40
#define	CTL_RXTL_MASK		0x7F
#define	CTL_RXTL_DEFAULT	0x20

/* SUN8IW11_OWA_FIFO_CTL register */
#define	SUN8IW11_FIFO_CTL_RXOM		0
#define	SUN8IW11_FIFO_CTL_TXIM		2
#define	SUN8IW11_FIFO_CTL_RXTL		3
#define	SUN8IW11_FIFO_CTL_TXTL		8
#define	SUN8IW11_FIFO_CTL_FRX		16
#define	SUN8IW11_FIFO_CTL_FTX		17
#define	SUN8IW11_FIFO_CTL_HUBEN		31
#define	SUN8IW11_CTL_TXTL_MASK		0x1F
#define	SUN8IW11_CTL_TXTL_DEFAULT	16
#define	SUN8IW11_CTL_RXTL_MASK		0x1F
#define	SUN8IW11_CTL_RXTL_DEFAULT	15

/* SUNXI_OWA_FIFO_STA register */
#define	FIFO_STA_RXA_CNT		0
#define	FIFO_STA_RXA			15
#define	FIFO_STA_TXA_CNT		16
#define	FIFO_STA_TXE			31

/* SUN8IW11_OWA_FIFO_STA register */
#define	SUN8IW11_FIFO_STA_RXA_CNT	0
#define	SUN8IW11_FIFO_STA_RXA		6
#define	SUN8IW11_FIFO_STA_TXA_CNT	8
#define	SUN8IW11_FIFO_STA_TXE		14

/* SUNXI_OWA_INT register */
#define	INT_RXAIEN		0
#define	INT_RXOIEN		1
#define	INT_RXDRQEN		2
#define	INT_TXEIEN		4
#define	INT_TXOIEN		5
#define	INT_TXUIEN		6
#define	INT_TXDRQEN		7
#define	INT_RXPAREN		16
#define	INT_RXUNLOCKEN		17
#define	INT_RXLOCKEN		18

/* SUNXI_OWA_INT_STA	*/
#define	INT_STA_RXA		0
#define	INT_STA_RXO		1
#define	INT_STA_TXE		4
#define	INT_STA_TXO		5
#define	INT_STA_TXU		6
#define	INT_STA_RXPAR		16
#define	INT_STA_RXUNLOCK	17
#define	INT_STA_RXLOCK		18

/* SUNXI_OWA_TXCH_STA0 register */
#define	TXCHSTA0_PRO		0
#define	TXCHSTA0_AUDIO		1
#define	TXCHSTA0_CP		2
#define	TXCHSTA0_EMPHASIS	3
#define	TXCHSTA0_MODE		6
#define	TXCHSTA0_CATACOD	8
#define	TXCHSTA0_SRCNUM		16
#define	TXCHSTA0_CHNUM		20
#define	TXCHSTA0_SAMFREQ	24
#define	TXCHSTA0_CLK		28

/* SUNXI_OWA_TXCH_STA1 register */
#define	TXCHSTA1_MAXWORDLEN	0
#define	TXCHSTA1_SAMWORDLEN	1
#define	TXCHSTA1_ORISAMFREQ	4
#define	TXCHSTA1_CGMSA		8

/* SUNXI_OWA_RXCH_STA0 register */
#define	RXCHSTA0_PRO		0
#define	RXCHSTA0_AUDIO		1
#define	RXCHSTA0_CP		2
#define	RXCHSTA0_EMPHASIS	3
#define	RXCHSTA0_MODE		6
#define	RXCHSTA0_CATACOD	8
#define	RXCHSTA0_SRCNUM		16
#define	RXCHSTA0_CHNUM		20
#define	RXCHSTA0_SAMFREQ	24
#define	RXCHSTA0_CLK		28

/* SUNXI_OWA_RXCH_STA1 register */
#define	RXCHSTA1_MAXWORDLEN	0
#define	RXCHSTA1_SAMWORDLEN	1
#define	RXCHSTA1_ORISAMFREQ	4
#define	RXCHSTA1_CGMSA		8

/* SUNXI_OWA_VER register */
#define MOD_VER			0

/* Debug shift */
#define RX_CHANNEL_SHIFT	0
#define RX_WORD_LEN_SHIFT	1
#define RX_RATE_SHIFT		2
#define RX_ORIG_RATE_SHIFT	3
#define RX_DATA_TYPE_SHIFT	4

typedef void sunxi_owa_clk_t;

/* To adapter diffent reg_addr / reg_bits */
struct sunxi_owa_quirks {
	unsigned int fifo_ctl_rxtl;
	unsigned int fifo_ctl_txtl;
	unsigned int fifo_ctl_frx;
	unsigned int fifo_ctl_ftx;
	unsigned int ctl_txtl_mask;
	unsigned int ctl_rxtl_mask;
	unsigned int ctl_txtl_default;
	unsigned int ctl_rxtl_default;
	bool loop_en;
	bool rx_sync_en;
};

struct sunxi_owa_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_owa_pinctl {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_owa_dts {
	/* value must be (2^n)Kbyte */
	size_t playback_cma;
	size_t playback_fifo_size;
	size_t capture_cma;
	size_t capture_fifo_size;

	bool tx_hub_en;		/* tx_hub */

	/* components func -> rx_sync */
	bool rx_sync_en;	/* read from dts */
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;

	/* clk fs */
	unsigned int pll_fs;
};

enum RX_LOCK_FLAG_STA {
	RX_LOCK_DEFAULT = 0,
	RX_LOCK = 0,
	RX_UNLOCK,
};

struct sunxi_owa_irq {
	unsigned int id;

	struct snd_pcm_substream *substream;

	/* add for false trigger interrupt */
	bool running;
	enum RX_LOCK_FLAG_STA flag_sta;
	enum RX_LOCK_FLAG_STA flag_sta_old;

	/* To solve channel exchange after plug and unplug */
	struct delayed_work lock_confirm_work;
};

struct sunxi_owa {
	const char *module_version;
	struct platform_device *pdev;

	struct sunxi_owa_mem mem;
	sunxi_owa_clk_t *clk;
	struct snd_sunxi_rglt *rglt;
	struct sunxi_owa_pinctl pin;
	struct sunxi_owa_dts dts;
	struct sunxi_owa_irq owa_irq;

	enum SND_SUNXI_CLK_STATUS clk_sta;

	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	const struct sunxi_owa_quirks *quirks;

	/* debug */
	char module_name[32];
	struct snd_sunxi_dump dump;
	bool show_reg_all;
};

sunxi_owa_clk_t *snd_owa_clk_init(struct platform_device *pdev);
void snd_owa_clk_exit(void *clk_orig);
int snd_owa_clk_bus_enable(void *clk_orig);
int snd_owa_clk_enable(void *clk_orig);
void snd_owa_clk_bus_disable(void *clk_orig);
void snd_owa_clk_disable(void *clk_orig);
int snd_owa_clk_rate(void *clk_orig, unsigned int freq_in, unsigned int freq_out);

#endif /* __SND_SUNXI_OWA_H */
