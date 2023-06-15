/*
 * sound\soc\sunxi\sunxi-spdif.h
 * (C) Copyright 2019-2025
 * allwinnertech Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__SUNXI_SPDIF_H_
#define	__SUNXI_SPDIF_H_

#include "sunxi-pcm.h"

/* SPDIF register definition */
#define	SUNXI_SPDIF_CTL		0x00
#define	SUNXI_SPDIF_TXCFG	0x04
#define	SUNXI_SPDIF_RXCFG	0x08
#define SUNXI_SPDIF_INT_STA	(0x0C)
#define	SUNXI_SPDIF_RXFIFO	0x10
#define	SUNXI_SPDIF_FIFO_CTL	0x14
#define	SUNXI_SPDIF_FIFO_STA	0x18
#define	SUNXI_SPDIF_INT		0x1C
#define SUNXI_SPDIF_TXFIFO	(0x20)
#define	SUNXI_SPDIF_TXCNT	0x24
#define	SUNXI_SPDIF_RXCNT	0x28
#define	SUNXI_SPDIF_TXCH_STA0	0x2C
#define	SUNXI_SPDIF_TXCH_STA1	0x30
#define	SUNXI_SPDIF_RXCH_STA0	0x34
#define	SUNXI_SPDIF_RXCH_STA1	0x38

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
#define	SUNXI_SPDIF_EXP_CTL	0x40
#define	SUNXI_SPDIF_EXP_ISTA	0x44
#define	SUNXI_SPDIF_EXP_INFO0	0x48
#define	SUNXI_SPDIF_EXP_INFO1	0x4C
#define	SUNXI_SPDIF_EXP_DBG0	0x50
#define	SUNXI_SPDIF_EXP_DBG1	0x54
#define	SUNXI_SPDIF_EXP_VER	0x58
#endif

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
#define SUNXI_SPDIF_REG_MAX SUNXI_SPDIF_EXP_VER
#else
#define SUNXI_SPDIF_REG_MAX SUNXI_SPDIF_RXCH_STA1
#endif

/* SUNXI_SPDIF_CTL register */
#define	CTL_RESET		0
#define	CTL_GEN_EN		1
#define	CTL_LOOP_EN		2
#define	CTL_RESET_RX		0

/* SUNXI_SPDIF_TXCFG register */
#define	TXCFG_TXEN		0
/* Chan status generated form TX_CHSTA */
#define	TXCFG_CHAN_STA_EN	1
#define	TXCFG_SAMPLE_BIT	2
#define	TXCFG_CLK_DIV_RATIO	4
#define	TXCFG_DATA_TYPE		16
/* Only valid in PCM mode */
#define	TXCFG_ASS		17
#define	TXCFG_SINGLE_MOD	31

/* SUNXI_SPDIF_RXCFG register */
#define	RXCFG_RXEN		0
#define	RXCFG_CHSR_CP		1
#define	RXCFG_CHST_SRC		3
#define	RXCFG_LOCK_FLAG		4

/* SUNXI_SPDIF_FIFO_CTL register */
#define	FIFO_CTL_RXOM		0
#define	FIFO_CTL_TXIM		2
#define	FIFO_CTL_RXTL		4
#define	FIFO_CTL_TXTL		12
#define	FIFO_CTL_FRX		29
#define	FIFO_CTL_FTX		30
#define	FIFO_CTL_HUBEN		31
#define	CTL_TXTL_MASK		0xFF
#define	CTL_TXTL_DEFAULT	0x40
#define	CTL_RXTL_MASK		0x7F
#define	CTL_RXTL_DEFAULT	0x20

/* SUNXI_SPDIF_FIFO_STA register */
#define	FIFO_STA_RXA_CNT	0
#define	FIFO_STA_RXA		15
#define	FIFO_STA_TXA_CNT	16
#define	FIFO_STA_TXE		31

/* SUNXI_SPDIF_INT register */
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

/* SUNXI_SPDIF_INT_STA	*/
#define	INT_STA_RXA		0
#define	INT_STA_RXO		1
#define	INT_STA_TXE		4
#define	INT_STA_TXO		5
#define	INT_STA_TXU		6
#define	INT_STA_RXPAR		16
#define	INT_STA_RXUNLOCK	17
#define	INT_STA_RXLOCK		18

/* SUNXI_SPDIF_TXCH_STA0 register */
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

/* SUNXI_SPDIF_TXCH_STA1 register */
#define	TXCHSTA1_MAXWORDLEN	0
#define	TXCHSTA1_SAMWORDLEN	1
#define	TXCHSTA1_ORISAMFREQ	4
#define	TXCHSTA1_CGMSA		8

/* SUNXI_SPDIF_RXCH_STA0 register */
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

/* SUNXI_SPDIF_RXCH_STA1 register */
#define	RXCHSTA1_MAXWORDLEN	0
#define	RXCHSTA1_SAMWORDLEN	1
#define	RXCHSTA1_ORISAMFREQ	4
#define	RXCHSTA1_CGMSA		8

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
/* SUNXI_SPDIF_EXP_CTL register */
#define INSET_DET_NUM		0
#define INSET_DET_EN		8
#define SYNCW_BIT_EN		9
#define DATA_TYPE_BIT_EN	10
#define DATA_LEG_BIT_EN		11
#define AUDIO_DATA_BIT_EN	12
#define RX_MODE			13
#define RX_MODE_MAN		14
#define UNIT_SEL		15
#define RPOTBF_NUM		16
#define BURST_DATA_OUT_SEL	30

/* SUNXI_SPDIF_EXP_ISTA register */
#define INSET_INT		0
#define PAPB_CAP_INT		1
#define PCPD_CAP_INT		2
#define RPDB_ERR_INT		3
#define PC_DTYOE_CH_INT		4
#define PC_ERR_FLAG_INT		5
#define PC_BIT_CH_INT		6
#define PC_PAUSE_STOP_INT	7
#define PD_CHAN_INT		8
#define INSET_INT_EN		16
#define PAPB_CAP_INT_EN		17
#define PCPD_CAP_INT_EN		18
#define RPDB_ERR_INT_EN		19
#define PC_DTYOE_CH_INT_EN	20
#define PC_ERR_FLAG_INT_EN	21
#define PC_BIT_CH_INT_EN	22
#define PC_PAUSE_STOP_INT_EN	23
#define PD_CHAN_INT_EN		24

/* SUNXI_SPDIF_EXP_INFO0 register */
#define PD_DATA_INFO		0
#define PC_DATA_INFO		16

/* SUNXI_SPDIF_EXP_INFO1 register */
#define SAMPLE_RATE_VAL		0
#define RPOTBF_VAL		16

/* SUNXI_SPDIF_EXP_DBG0 register */
#define RE_DATA_COUNT_VAL	0
#define DATA_CAP_STA_MACHE	16

/* SUNXI_SPDIF_EXP_DBG1 register */
#define SAMPLE_RATE_COUNT	0
#define RPOTBF_COUNT		16

/* SUNXI_SPDIF_EXP_VER register */
#define MOD_VER			0
#endif

#define SPDIF_REG_LABEL(constant) \
{ \
	#constant, constant \
}

#define SPDIF_REG_LABEL_END \
{ \
	NULL, -1 \
}

#define SUNXI_SPDIF_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)

#define SPDIF_RX_FIFO_SIZE 64
#define SPDIF_TX_FIFO_SIZE 128

#define SPDIF_CLK_PLL_AUDIO_X1 0
#define SPDIF_CLK_PLL_AUDIO_X4 1

struct sunxi_spdif_reg_label {
	const char *name;
	int value;
};

struct spdif_gpio_ {
	u32 gpio;
	bool level;
	bool used;
};

struct sunxi_spdif_mem_info {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *regmap;
};

struct sunxi_spdif_clk_info {
	struct clk *clk_pll;
	struct clk *clk_module;
	struct clk *clk_bus;
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	struct clk *clk_pll1;
	struct clk *clk_pll1_div;
	struct clk *clk_module_rx;
#endif
	struct reset_control *clk_rst;
	unsigned int clk_parent;
};

struct sunxi_spdif_pinctl_info {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	struct spdif_gpio_ gpio_cfg;
};

struct sunxi_spdif_dts_info {
	/* value must be (2^n)Kbyte */
	size_t playback_cma;
	size_t capture_cma;
};

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
struct sunxi_spdif_rx_params {
	u32 freq;
	u32 orig_freq;
	u32 refreq;
	u32 channels;
	u32 bits;
};

struct sunxi_spdif_rx_info {
#if 0
	u32 spdif_inset_int;
	u32 spdif_rxlock_int;
	u32 spdif_rxunlock_int;
	u32 spdif_papb_int;
	u32 channel_status;
#endif
	struct sunxi_spdif_rx_params rx_params;
};
#endif

struct sunxi_spdif_info {
	struct device *dev;

	struct sunxi_spdif_mem_info mem_info;
	struct sunxi_spdif_clk_info clk_info;
	struct sunxi_spdif_pinctl_info pin_info;
	struct sunxi_spdif_dts_info dts_info;
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	struct sunxi_spdif_rx_info rx_info;

	unsigned int spdif_rx_type;
	/* spdif in irq */
//	int spdif_irq;
#endif
	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	struct mutex mutex;
	struct snd_soc_dai_driver dai;
	unsigned int rate;
	unsigned int active;
	bool configured;
};
#endif	/* __SUNXI_SPDIF_H_ */
