/* sound\soc\sunxi\snd_sunxi_dmic.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef __SND_SUNXI_DMIC_H
#define __SND_SUNXI_DMIC_H

#define SUNXI_DMIC_EN			0x00
#define SUNXI_DMIC_SR			0x04
#define SUNXI_DMIC_CTR			0x08
#define SUNXI_DMIC_DATA			0x10
#define SUNXI_DMIC_INTC			0x14
#define SUNXI_DMIC_INTS			0x18
#define SUNXI_DMIC_FIFO_CTR		0x1c
#define SUNXI_DMIC_FIFO_STA		0x20
#define SUNXI_DMIC_CH_NUM		0x24
#define SUNXI_DMIC_CH_MAP		0x28
#define SUNXI_DMIC_CNT			0x2c
#define SUNXI_DMIC_DATA0_1_VOL		0x30
#define SUNXI_DMIC_DATA2_3_VOL		0x34
#define	SUNXI_DMIC_HPF_CTRL		0x38
#define	SUNXI_DMIC_HPF_COEF		0x3C
#define	SUNXI_DMIC_HPF_GAIN		0x40
#define SUNXI_DMIC_REV			0x50
#define SUNXI_DMIC_REG_MAX			SUNXI_DMIC_REV

/*0x00:SUNXI_DMIC_EN*/
#define RX_SYNC_EN_START		29
#define RX_SYNC_EN			28
#define GLOBE_EN			8
#define DATA3_CHR_EN			7
#define DATA3_CHL_EN			6
#define DATA2_CHR_EN			5
#define DATA2_CHL_EN			4
#define DATA1_CHR_EN			3
#define DATA1_CHL_EN			2
#define DATA0_CHR_EN			1
#define DATA0_CHL_EN			0
#define DATA_CH_EN			0

/*SUNXI_DMIC_SR:0x04*/
#define DMIC_SR				0

/*SUNXI_DMIC_CTR:0x08*/
#define DMICFDT				9
#define DMICDFEN			8
#define DATA3_LR_SWAP_EN		7
#define DATA2_LR_SWAP_EN		6
#define DATA1_LR_SWAP_EN		5
#define DATA0_LR_SWAP_EN		4
#define DMIC_OVERSAMPLE_RATE		0

/*SUNXI_DMIC_DATA:0x10*/
#define DMIC_DATA			0

/*SUNXI_DMIC_INTC:0x14*/
#define FIFO_DRQ_EN			2
#define FIFO_OVERRUN_IRQ_EN		1
#define DATA_IRQ_EN			0

/*SUNXI_DMIC_INTS:0x18*/
#define FIFO_OVERRUN_IRQ_PENDING	1
#define FIFO_DATA_IRQ_PENDING		0

/*SUNXI_DMIC_FIFO_CTR:0x1c*/
#define DMIC_FIFO_FLUSH			31
#define DMIC_FIFO_MODE			9
#define DMIC_SAMPLE_RESOLUTION		8
#define FIFO_TRG_LEVEL			0

/*SUNXI_DMIC_FIFO_STA:0x20*/
#define DMIC_DATA_CNT			0

/*SUNXI_DMIC_CH_NUM:0x24*/
#define DMIC_CH_NUM			0

/*SUNXI_DMIC_CH_MAP:0x28*/
#define DMIC_CH7_MAP			28
#define DMIC_CH6_MAP			24
#define DMIC_CH5_MAP			20
#define DMIC_CH4_MAP			16
#define DMIC_CH3_MAP			12
#define DMIC_CH2_MAP			8
#define DMIC_CH1_MAP			4
#define DMIC_CH0_MAP			0
#define DMIC_CHANMAP_DEFAULT		(0x76543210)
/*SUNXI_DMIC_CNT:0x2c*/
#define DMIC_CNT			0

/*SUNXI_DMIC_DATA0_1_VOL:0x30*/
#define DATA1L_VOL			24
#define DATA1R_VOL			16
#define DATA0L_VOL			8
#define DATA0R_VOL			0

/*SUNXI_DMIC_DATA2_3_VOL:0x34*/
#define DATA3L_VOL			24
#define DATA3R_VOL			16
#define DATA2L_VOL			8
#define DATA2R_VOL			0
#define	DMIC_DEFAULT_VOL		0xB0

/*SUNXI_DMIC_HPF_EN_CTR:0x38*/
#define HPF_DATA3_CHR_EN		7
#define HPF_DATA3_CHL_EN		6
#define HPF_DATA2_CHR_EN		5
#define HPF_DATA2_CHL_EN		4
#define HPF_DATA1_CHR_EN		3
#define HPF_DATA1_CHL_EN		2
#define HPF_DATA0_CHR_EN		1
#define HPF_DATA0_CHL_EN		0

/*SUNXI_DMIC_HPF_COEF:0x3C*/
#define HPF_COEF			0

/*SUNXI_DMIC_HPF_GAIN:0x40*/
#define HPF_GAIN			0

struct sunxi_dmic_mem_info {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap   *regmap;
};

struct sunxi_dmic_clk_info {
	struct clk *clk_pll_audio;
	struct clk *clk_pll_audio_4x;
	struct clk *clk_parent;
	struct clk *clk_dmic;

	unsigned int pll_fs;

	struct clk *clk_bus;
	struct reset_control *clk_rst;
};

struct sunxi_dmic_pinctl_info {
	struct pinctrl *pinctrl;
	struct pinctrl_state  *pinstate;
	struct pinctrl_state  *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_dmic_dts_info {
	/* value must be (2^n)Kbyte */
	size_t capture_cma;
	size_t capture_fifo_size;

	unsigned int rx_chmap;
	unsigned int data_vol;
	unsigned int rx_dtime;

	/* components func -> rx_sync */
	bool rx_sync_en;	/* read from dts */
	bool rx_sync_ctl;
	int rx_sync_id;
	rx_sync_domain_t rx_sync_domain;
};

struct sunxi_dmic_info {
	struct platform_device *pdev;

	struct sunxi_dmic_mem_info mem_info;
	struct sunxi_dmic_clk_info clk_info;
	struct sunxi_dmic_pinctl_info pin_info;
	struct sunxi_dmic_dts_info dts_info;

	struct sunxi_dma_params capture_dma_param;
};

#endif /* __SND_SUNXI_DMIC_H */