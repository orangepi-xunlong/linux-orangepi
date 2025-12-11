/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, huhaoxin <huhaoxin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUN65IW1_AC203C_H
#define __SND_SUN65IW1_AC203C_H

/* REG-Digital */
#define SUNXI_MOD_VER		0x00
#define SUNXI_PLL_LOCK_CTL	0x04
#define SUNXI_SYS_CLK_CTL	0x08
#define SUNXI_I2S_CTL		0x1C
#define SUNXI_ADDA1_DAC_DATA	0X20
#define SUNXI_I2S_BCLK_CTL	0x24
#define SUNXI_I2S_LRCK_CTL	0x28
#define	SUNXI_I2S_FMT_CTL	0x2C
#define SUNXI_I2S_TX_CTL	0x30
#define SUNXI_ADC_MIX_CTL	0x34
#define SUNXI_I2S_TX_CHMP_CTL	0x38
#define SUNXI_I2S_RX_CTL	0x3C
#define SUNXI_DAC_MIX_CTL	0x40
#define SUNXI_I2S_RX_CHMP_CTL	0x44

#define SUNXI_DIG_BYPASS_CTL	0x108
#define SUNXI_ADDA_FS_CTL	0x120

#define SUNXI_ADC_DIG_EN	0x200
#define SUNXI_ADC_DDT_CTL	0x208
#define SUNXI_HPF_EN		0x20C
#define SUNXI_HPF1_COEF_REG	0x210
#define SUNXI_HPF2_COEF_REG	0x214
#define SUNXI_HPF3_COEF_REG	0x218
#define SUNXI_ADC_MUX_CTL	0x21C
#define SUNXI_ADC_DVOL_CTL	0x220
#define SUNXI_ADC_DIG_DEBUG	0x224

#define SUNXI_DAC_DIG_EN	0x300
#define SUNXI_DAC_DIG_CTL	0x304
#define SUNXI_DAC_DHP_GAIN_CTL	0x308
#define SUNXI_DAC_LOUT_CTL	0x30C
#define SUNXI_DAC_DVC		0x310
#define SUNXI_EQ_CTL		0x314
#define SUNXI_EQ_COFE_CTL	0x318
#define SUNXI_DAC_MUX_CTL	0x31C
#define SUNXI_HP_AVR_CTL	0x320
#define SUNXI_HP_AVR_TH		0x324
#define SUNXI_HP_AVR_DBC	0x328

#define SUNXI_EQ_B0		0x400
#define SUNXI_EQ_B1		0x404
#define SUNXI_EQ_B2		0x408
#define SUNXI_EQ_A1		0x40C
#define SUNXI_EQ_A2		0x410

#define SUNXI_DRC_ENA		0x500
#define SUNXI_DRC_HPFC		0x504
#define SUNXI_DRC_CTL		0x508
#define SUNXI_DRC_DLY_CTL	0x50C
#define SUNXI_DRC_OPT		0x510
#define SUNXI_DRC_LPFAT		0x514
#define SUNXI_DRC_RPFAT		0x518
#define SUNXI_DRC_LPFRT		0x51C
#define SUNXI_DRC_RPFRT		0x520
#define SUNXI_DRC_LRMSAT	0x524
#define SUNXI_DRC_RRMSAT	0x528
#define SUNXI_DRC_CT		0x52C
#define SUNXI_DRC_KC		0x530
#define SUNXI_DRC_OPC		0x534
#define SUNXI_DRC_LT		0x538
#define SUNXI_DRC_KL		0x53C
#define SUNXI_DRC_OPL		0x540
#define SUNXI_DRC_ET		0x544
#define SUNXI_DRC_KE		0x548
#define SUNXI_DRC_OPE		0x54C
#define SUNXI_DRC_KN		0x550
#define SUNXI_DRC_SFAT		0x554
#define SUNXI_DRC_SFRT		0x558
#define SUNXI_DRC_MXGS		0x55C
#define SUNXI_DRC_MNGS		0x560
#define SUNXI_DRC_EPSC		0x564

#define SUNXI_ADDA1_INT_CTL	0x600
#define SUNXI_ADDA1_INT_STAT	0x604
#define SUNXI_ADDA1_FIFO_CTL	0x608
#define SUNXI_ADDA1_FIFO_STAT	0x60C
#define SUNXI_ADDA1_DAC_CNT	0x614
#define SUNXI_ADDA1_ADC_DATA	0x618
#define SUNXI_ADDA1_ADC_CNT	0x61C

#define SUNXI_ADDA1_SYNC_CTL	0x700
#define SUNXI_AUDIO_DIG_MAX_REG	SUNXI_ADDA1_SYNC_CTL

#define SUNXI_VIR_OFFSET	0x800

/* REG-Analog */
#define SUNXI_CHIP_SOFT_RST		(0X00 + SUNXI_VIR_OFFSET)
#define SUNXI_POWER_REG1		(0X01 + SUNXI_VIR_OFFSET)
#define SUNXI_POWER_REG2		(0X02 + SUNXI_VIR_OFFSET)
#define SUNXI_POWER_REG3		(0X03 + SUNXI_VIR_OFFSET)
#define SUNXI_POWER_REG4		(0X04 + SUNXI_VIR_OFFSET)
#define SUNXI_POWER_REG5		(0X05 + SUNXI_VIR_OFFSET)
#define SUNXI_POWER_REG6		(0X06 + SUNXI_VIR_OFFSET)
#define SUNXI_MBIAS_REG			(0X07 + SUNXI_VIR_OFFSET)
#define SUNXI_HBIAS_REG			(0X08 + SUNXI_VIR_OFFSET)
#define SUNXI_DAC_REG1			(0X09 + SUNXI_VIR_OFFSET)
#define SUNXI_DAC_REG2			(0X0A + SUNXI_VIR_OFFSET)
#define SUNXI_DAC_REG3			(0X0B + SUNXI_VIR_OFFSET)
#define SUNXI_DAC_REG4			(0X0C + SUNXI_VIR_OFFSET)
#define SUNXI_HP_REG1			(0X19 + SUNXI_VIR_OFFSET)
#define SUNXI_HP_REG2			(0X1A + SUNXI_VIR_OFFSET)
#define SUNXI_HP_REG3			(0X1B + SUNXI_VIR_OFFSET)
#define SUNXI_HP_REG4			(0X1C + SUNXI_VIR_OFFSET)
#define SUNXI_HP_REG5			(0X1D + SUNXI_VIR_OFFSET)
#define SUNXI_SYSCLK_CTL		(0X20 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC1_REG1			(0X24 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC1_REG2			(0X25 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC1_REG3			(0X26 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC1_REG4			(0X27 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC2_REG1			(0X28 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC2_REG2			(0X29 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC2_REG3			(0X2A + SUNXI_VIR_OFFSET)
#define SUNXI_ADC2_REG4			(0X2B + SUNXI_VIR_OFFSET)
#define SUNXI_ADC3_REG1			(0X2C + SUNXI_VIR_OFFSET)
#define SUNXI_ADC3_REG2			(0X2D + SUNXI_VIR_OFFSET)
#define SUNXI_ADC3_REG3			(0X2E + SUNXI_VIR_OFFSET)
#define SUNXI_ADC3_REG4			(0X2F + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_CTL_2			(0X30 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_BCLK_CLT1		(0X31 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_BCLK_CLT2		(0X37 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_LRCK_CLT1		(0X32 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_LRCK_CLT2		(0X45 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_LRCK_CLT3		(0X33 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_LRCK_CLT4		(0X46 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_FMT_CLT1		(0X34 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S0_FMT_CLT2		(0X35 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S1_FMT_CLT2		(0X47 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_FMT_CLT3		(0X36 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_FMT_CLT4		(0X48 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CLT1		(0X38 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CLT2		(0X39 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CLT3		(0X3A + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_MIX_CTL		(0X3B + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CHMP_CTL1		(0X3C + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CHMP_CTL2		(0X3D + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CHMP_CTL3		(0X3E + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_TX_CHMP_CTL4		(0X3F + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_RX_CTL1		(0X40 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_RX_CTL2		(0X41 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_RX_CTL3		(0X42 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_RX_MIX_CTL		(0X43 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_RX_CHMP_CTL_2		(0X44 + SUNXI_VIR_OFFSET)
#define SUNXI_DEBUG_CTL			(0X50 + SUNXI_VIR_OFFSET)
#define SUNXI_I2S_PADDRV_CTL		(0X51 + SUNXI_VIR_OFFSET)
#define SUNXI_DEBUG_PADDRV_CTL		(0X52 + SUNXI_VIR_OFFSET)
#define SUNXI_DIG_BYPASS_CTL_2		(0X53 + SUNXI_VIR_OFFSET)
#define SUNXI_EFUSE_WR_CTL		(0X54 + SUNXI_VIR_OFFSET)
#define SUNXI_ADDA_FS_CTL_2		(0X60 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC_DIG_EN_2		(0X61 + SUNXI_VIR_OFFSET)
#define SUNXI_ADC_DDT_CTL_2		(0X63 + SUNXI_VIR_OFFSET)
#define SUNXI_HMIC_DET_CTL		(0X73 + SUNXI_VIR_OFFSET)
#define SUNXI_HMIC_DET_DBC		(0X74 + SUNXI_VIR_OFFSET)
#define SUNXI_HMIC_DET_TH1		(0X75 + SUNXI_VIR_OFFSET)
#define SUNXI_HMIC_DET_TH2		(0X76 + SUNXI_VIR_OFFSET)
#define SUNXI_HMIC_DET_DATA		(0X77 + SUNXI_VIR_OFFSET)
#define SUNXI_HP_DET_CTL		(0X78 + SUNXI_VIR_OFFSET)
#define SUNXI_HP_DET_DBC		(0X79 + SUNXI_VIR_OFFSET)
#define SUNXI_HP_DET_IRQ		(0X7A + SUNXI_VIR_OFFSET)
#define SUNXI_HP_DET_STA		(0X7B + SUNXI_VIR_OFFSET)
#define SUNXI_ADC_DIG_DEBUG_2		(0X7F + SUNXI_VIR_OFFSET)
#define SUNXI_DAC_DIG_EN_2		(0X81 + SUNXI_VIR_OFFSET)
#define SUNXI_DAC_DIG_CTL_2		(0X82 + SUNXI_VIR_OFFSET)
#define SUNXI_HP_AVR_CTL_2		(0X8A + SUNXI_VIR_OFFSET)
#define SUNXI_HP_AVR_THH		(0X8B + SUNXI_VIR_OFFSET)
#define SUNXI_HP_AVR_THM		(0X8C + SUNXI_VIR_OFFSET)
#define SUNXI_HP_AVR_THL		(0X8D + SUNXI_VIR_OFFSET)
#define SUNXI_HP_AVR_DBC_2		(0X8E + SUNXI_VIR_OFFSET)
#define SUNXI_INT_ADDR_CONF_REG		(0X8E + SUNXI_VIR_OFFSET)
#define SUNXI_AUDIO_ANA_MAX_REG		(SUNXI_INT_ADDR_CONF_REG - SUNXI_VIR_OFFSET)

/* BITS-Digital */
/* SUNXI_MOD_VER */
#define MOD_VER		0
/* SUNXI_PLL_LOCK_CTL */
#define HOLD_TIME	0
/* SUNXI_SYS_CLK_CTL */
#define SYSCLK_EN	0
/* SUNXI_I2S_CTL */
#define BCLK_RX_IOEN	9
#define LRCK_RX_IOEN	8
#define BCLK_TX_IOEN	7
#define LRCK_TX_IOEN	6
#define SDO_EN		4
#define OUT_MUTE	3
#define TXEN		2
#define RXEN		1
#define I2SGEN		0
/* SUNXI_ADDA1_DAC_DATA */
#define TX_DATA		0
/* SUNXI_I2S_BCLK_CTL */
#define BCLK_RX_POLARITY	6
#define EDGE_TRANSFER		5
#define BCLK_TX_POLARITY	4
#define BCLKDIV			0
/* SUNXI_I2S_LRCK_CTL */
#define LRCK_PERIOD		6
#define LRCK_RX_POLARITY	2
#define LRCK_WIDTH		1
#define LRCK_TX_POLARITY	0
/* SUNXI_I2S_FMT_CTL */
#define RX_MLS			23
#define TX_MLS			22
#define SEXT			20
#define RX_PDM			18
#define TX_PDM			16
#define SW			12
#define SR			8
#define ENCD_FMT		7
#define ENCD_SEL		6
#define MODE_SEL		4
#define OFFSET			2
#define TX_SLOT_HIZ		1
#define TX_STATE		0
/* SUNXI_I2S_TX_CTL */
#define TX_CHEN_HIGH		16
#define TX_CHEN_LOW		8
#define TX_CHSEL		0
/* SUNXI_ADC_MIX_CTL */
#define ADC_MIX3		4
#define ADC_MIX2		2
#define ADC_MIX1		0
/* SUNXI_I2S_TX_CHMP_CTL */
#define TX_CH16_MAP		30
#define TX_CH15_MAP		28
#define TX_CH14_MAP		26
#define TX_CH13_MAP		24
#define TX_CH12_MAP		22
#define TX_CH11_MAP		20
#define TX_CH10_MAP		18
#define TX_CH9_MAP		16
#define TX_CH8_MAP		14
#define TX_CH7_MAP		12
#define TX_CH6_MAP		10
#define TX_CH5_MAP		8
#define TX_CH4_MAP		6
#define TX_CH3_MAP		4
#define TX_CH2_MAP		2
#define TX_CH1_MAP		0
/* SUNXI_I2S_RX_CTL */
#define RX_CHEN_HIGH		16
#define RX_CHEN_LOW		8
#define RX_CHSEL		0
/* SUNXI_DAC_MIX_CTL */
#define DAC_MIX3		4
#define DAC_MIX2		2
#define DAC_MIX1		0
/* SUNXI_I2S_RX_CHMP_CTL */
#define RXR_MAP			4
#define RXL_MAP			0
/* SUNXI_DIG_BYPASS_CTL */
#define DITHER_EN		2
#define ADC_HPF_BYPASS		0
/* SUNXI_ADDA_FS_CTL */
#define ADDA_FS_DIV		0
/* SUNXI_ADC_DIG_EN */
#define REQ_WIDTH		4
#define ADC_EN			3
#define ENAD3			2
#define ENAD2			1
#define ENAD1			0
/* SUNXI_ADC_DDT_CTL */
#define ADC_DVC_ZCD_EN		7
#define ADC_OSR			5
#define ADC_DLY_MODE		3
#define ADC_DTS			0
/* SUNXI_HPF_EN */
#define DIG_ADC3_HPF_EN		2
#define DIG_ADC2_HPF_EN		1
#define DIG_ADC1_HPF_EN		0
/* SUNXI_HPF1_COEF_REG */
#define HPF1_COEF		0
/* SUNXI_HPF2_COEF_REG */
#define HPF2_COEF		0
/* SUNXI_HPF3_COEF_REG */
#define HPF3_COEF		0
/* SUNXI_ADC_MUX_CTL */
#define DIG_ADC3_MUX		16
#define DIG_ADC2_MUX		8
#define DIG_ADC1_MUX		0
/* SUNXI_ADC_DVOL_CTL */
#define DIG_ADC3_VOL		16
#define DIG_ADC2_VOL		8
#define DIG_ADC1_VOL		0
/* SUNXI_ADC_DIG_DEBUG */
#define I2S_LPB_DEBUG		3
#define ADC_PTN_SEL		0
/* SUNXI_DAC_DIG_EN */
#define DAC_FS			8
#define DAC_EN			2
#define ENDAL			1
#define ENDAR			0
/* SUNXI_DAC_DIG_CTL */
#define DVCZCDEN		7
#define DAC_SWP			2
#define DAC_OSR			0
/* SUNXI_DAC_DHP_GAIN_CTL */
#define DAC_MUTE_DET_TIME	4
#define DHP_OUT_GAIN		0
/* SUNXI_DAC_LOUT_CTL */
#define ATT_STEP		4
#define LOUT_AUTO_ATT		2
#define LOUT_AUTO_MUTE		1
#define DHP_GAIN_ZC_DEN		0
/* SUNXI_DAC_DVC */
#define DAC_DVC_R		8
#define DAC_DVC_L		0
/* SUNXI_EQ_CTL */
#define EQ_LCHNL_EN		4
#define EQ_RCHNL_EN		0
/* SUNXI_EQ_COFE_CTL */
#define EQ_CHNL_SEL		7
#define EQ_BAND_COEF_CTL	0
/* SUNXI_DAC_MUX_CTL */
#define DIG_DAC2_MUX		4
#define DIG_DAC1_MUX		0
/* SUNXI_HP_AVR_CTL */
#define HP_AVR_DEN		7
#define HVH			4
#define HVL			0
/* SUNXI_HP_AVR_TH */
#define THH			16
#define THM			8
#define THL			0
/* SUNXI_HP_AVR_DBC */
#define HV_DBC			0
/* SUNXI_EQ_B0 */
#define EQ_B0_COEF		0
/* SUNXI_EQ_B1 */
#define EQ_B1_COEF		0
/* SUNXI_EQ_B2 */
#define EQ_B2_COEF		0
/* SUNXI_EQ_A1 */
#define EQ_A1_COEF		0
/* SUNXI_EQ_A2 */
#define EQ_A2_COEF		0
/* SUNXI_DRC_ENA */
#define DRC_CAL_ENA		1
#define DRC_HPF_ENA		0
/* SUNXI_DRC_HPFC */
#define DRC_HPF_COEF		0
/* SUNXI_DRC_CTL */
#define DRC_GAIN_MAX_LIM_EN	6
#define DRC_GAIN_MIN_LIM_EN	5
#define DRC_DET_NOISE_EN	4
#define DRC_FUNC_SEL		3
#define DLY_EN			2
#define DRC_LT_EN		1
#define DRC_ET_EN		0
/* SUNXI_DRC_DLY_CTL */
#define DRC_DLY_BUF_SEL		7
#define DRC_DLY_BUF_OUT_STAT	6
#define DLY_TIME_SET		0
/* SUNXI_DRC_OPT */
#define DRC_GAIN_SMTH_COEF_SEL		11
#define DRC_GAIN_SMTH_MODE		10
#define DRC_PEAK_DET_MODE_MIN_ENE	9
#define DRC_ENE_MODE			8
#define DRC_OUT_CTL			6
#define DRC_GAIN_DEFAULT_VALUE		5
#define DRC_GAIN_SMTH_FILTER_HYSTER	0
/* SUNXI_DRC_LPFAT */
#define DRC_LPFAT		0
/* SUNXI_DRC_RPFAT */
#define DRC_RPFAT		0
/* SUNXI_DRC_LPFRT */
#define DRC_LPFRT		0
/* SUNXI_DRC_RPFRT */
#define DRC_RPFRT		0
/* SUNXI_DRC_LRMSAT */
#define DRC_LRMSAT		0
/* SUNXI_DRC_RRMSAT */
#define DRC_RRMSAT		0
/* SUNXI_DRC_CT */
#define DRC_CT			0
/* SUNXI_DRC_KC */
#define DRC_KC			0
/* SUNXI_DRC_OPC */
#define DRC_OPC			0
/* SUNXI_DRC_LT */
#define DRC_LT			0
/* SUNXI_DRC_KL */
#define DRC_KL			0
/* SUNXI_DRC_OPL */
#define DRC_OPL			0
/* SUNXI_DRC_ET */
#define DRC_ET			0
/* SUNXI_DRC_KE */
#define DRC_KE			0
/* SUNXI_DRC_OPE */
#define DRC_OPE			0
/* SUNXI_DRC_KN */
#define DRC_KN			0
/* SUNXI_DRC_SFAT */
#define DRC_SMTH_FILTER_ATTA_TIME	0
/* SUNXI_DRC_SFRT */
#define DRC_SMTH_FILTER_RELE_TIME	0
/* SUNXI_DRC_MXGS */
#define DRC_MAX_GAIN_SET	0
/* SUNXI_DRC_MNGS */
#define DRC_MIN_GAIN_SET	0
/* SUNXI_DRC_EPSC */
#define DRC_EXP_SMTH_TIME	0
/* SUNXI_ADDA1_INT_CTL */
#define TX_OI_EN		19
#define TX_UI_EN		18
#define TX_EI_EN		17
#define TX_DRQ_EN		16
#define RX_OI_EN		3
#define RX_UI_EN		2
#define RX_AI_EN		1
#define RX_DRQ_EN		0
/* SUNXI_ADDA1_INT_STAT */
#define TX_OI			19
#define TX_UI			18
#define TX_EI			17
#define RX_OI			3
#define RX_UI			2
#define RX_AI			1
/* SUNXI_ADDA1_FIFO_CTL */
#define TX_TL			17
#define FTX			16
#define RX_TL			1
#define FRX			0
/* SUNXI_ADDA1_FIFO_STAT */
#define TXE			24
#define TXE_CNT			16
#define RXA			8
#define RXA_CNT			0
/* SUNXI_ADDA1_DAC_CNT */
#define TX_CNT			0
/* SUNXI_ADDA1_ADC_DATA */
#define RX_DATA			0
/* SUNXI_ADDA1_ADC_CNT */
#define RX_CNT			0
/* SUNXI_ADDA1_SYNC_CTL */
#define HUB_EN			2
#define SYNC_EN			1
#define EN_MUX			0

/* BITS-Analog */
/* SUNXI_CHIP_SOFT_RST */
#define SOFT_RESET		0
/* SUNXI_POWER_REG1 */
#define VRA1_SPEEDUP_DISABLE	7
#define BG_BUFEN		6
#define BG_VSEL			4
#define BG_TEMP_DRIFT_TRIM	0
/* SUNXI_POWER_REG2 */
#define BG_TRIM			4
#define OSC_EN			3
/* SUNXI_POWER_REG3 */
#define DLDO_VSEL		0
/* SUNXI_POWER_REG4 */
#define ALDO_EN			7
#define ALDO_BYPASS		3
#define AVCC_POR		1
#define ADDA_BIAS_EN		0
/* SUNXI_POWER_REG5 */
#define CURRENT_TEST_SELECT	7
/* SUNXI_POWER_REG6 */
#define IOPDRVS			6
#define IOPVRS			4
#define IOPDACS			2
#define ISPKS			0
/* SUNXI_MBIAS_REG */
#define MBIAS_EN		7
#define MBIAS_VOL_SEL		5
#define MBIAS_CHOPPER_EN	4
#define MBIAS_CHOPPER_CLK_SEL	2
/* SUNXI_HBIAS_REG */
#define SBU12_SEL_DISABLE	7
#define HBIAS_VOL_SEL		4
#define HBIAS_SW		3
#define HBIAS_MODE		2
#define HBIAS_EN		1
#define HBIASADC_EN		0
/* SUNXI_DAC_REG1 */
#define DACL_EN			7
#define DACR_EN			6
#define CKDAC_DLY_SET		4
/* SUNXI_DAC_REG2 */
#define DAC_CHOPPER_EN		7
#define DAC_CHOPPER_NOL_EN	6
#define DAC_CHOPPER_CKSET	4
#define DAC_CHOPPER_DLY_SET	2
#define DAC_CHOPPER_NOL_DLY_SET	0
/* SUNXI_DAC_REG3 */
#define SPKL_EN			7
#define SPKR_EN			6
#define SPK_SELPGA_EN		5
#define SPK_CH_OUT_EDGE		0
/* SUNXI_DAC_REG4 */
#define SPK_CHOPPER_EN		7
#define SPK_CHOPPER_NOL_EN	6
#define SPK_CHOPPER_CKSET	4
#define SPK_CHOPPER_DLY_SET	2
#define SPK_CHOPPER_NOL_DLY_SET	0
/* SUNXI_HP_REG1 */
#define HPDRV_EN		7
#define CP_EN			6
#define HPOUT_EN		5
#define HPOUT_SHORT_CTL		4
#define USB_HP_SEL_CTL		3
#define CP_CLKS			0
/* SUNXI_HP_REG2 */
#define HP_CHOPPER_EN		6
#define HP_CHOPPER_NOL_EN	5
#define HP_CHOPPER_CKSET	4
#define HP_CHOPPER_DLY_SET	3
#define HP_CHOPPER_NOL_DLY_SET	0
/* SUNXI_HP_REG3 */
#define CAPLESS_LDO1_EN_CTL		7
#define CAPLESS_LDO1_BYPASS_CTL		6
#define CAPLESS_LDO1_OUT_VOLT_VPP_CTL	4
#define CAPLESS_LDO1_OUT_VOLT_TEST	3
#define CAPLESS_LDO2_EN_CTL		2
#define CAPLESS_LDO2_OUT_VOLT_VEE_CTL	0
/* SUNXI_HP_REG4 */
#define EDP_SEL				2
#define EDP_SW				0
/* SUNXI_HP_REG5 */
#define OPDRV_CUR			0
/* SUNXI_SYSCLK_CTL */
#define DAC_CLK_SEL			5
#define ADC_CLK_SEL			4
#define SYSCLK_EN_2			0
/* SUNXI_ADC1_REG1 */
#define ADC1_EN				7
#define ADC1_DSM_DIS			6
#define ADC1_DSM_OTA_CTL		4
#define ADC123_DSM_EN_DITHER		3
#define ADC123_DSM_DITHER_LVL		0
/* SUNXI_ADC1_REG2 */
#define ADC123_PGA_CHOPPER_EDGE		7
#define ADC1_DSM_OTA_IB_SEL		4
#define ADC1_DSM_OP_BIAS_CTL		2
#define ADC1_PGA_OP_BIAS_CTL		0
/* SUNXI_ADC1_REG3 */
#define ADC1_PGA_OTA_IB_SEL		5
#define MIC_START_CTL			4
#define ADC1_PGA_GAIN_CTL		0
/* SUNXI_ADC1_REG4 */
#define ADC1_PGA_CHOPPER_EN		7
#define ADC1_PGA_CHOPPER_NOL_EN		6
#define ADC1_PGA_CHOPPER_CKSET		4
#define ADC1_PGA_CHOPPER_DLY_SET	2
#define ADC1_PGA_CHOPPER_NOL_DLY_SET	0
/* SUNXI_ADC2_REG1 */
#define ADC2_EN				7
#define ADC2_DSM_DIS			6
#define ADC2_DSM_OTA_CTL		4
#define ADC2_DSM_DEMOFF			3
#define MIC2_MIC4_CROSSTALK_AVOID_EN	2
#define ADC2_MIC_MIX_MUX2		0
/* SUNXI_ADC2_REG2 */
#define ADC2_DSM_SEL_OUT_EDGE		7
#define ADC2_DSM_OTA_IB_SEL		4
#define ADC2_DSM_OP_BIAS_CTL		2
#define ADC2_PGA_OP_BIAS_CTL		0
/* SUNXI_ADC2_REG3 */
#define ADC2_PGA_OTA_IB_SEL		5
#define MIC_START_CTL			4
#define ADC2_PGA_GAIN_CTL		0
/* SUNXI_ADC2_REG4 */
#define ADC2_PGA_CHOPPER_EN		7
#define ADC2_PGA_CHOPPER_NOL_EN		6
#define ADC2_PGA_CHOPPER_CKSET		4
#define ADC2_PGA_CHOPPER_DLY_SET	2
#define ADC2_PGA_CHOPPER_NOL_DLY_SET	0
/* SUNXI_ADC3_REG1 */
#define ADC3_EN				7
#define ADC3_DSM_DIS			6
#define ADC3_DSM_OTA_CTL		4
#define ADC3_DSM_DEMOFF			3
#define ADC1_DSM_DEMOFF			2
#define ADC3_MIC_MIX			1
#define ADC1_DSM_SEL_OUT_EDGE		0
/* SUNXI_ADC3_REG2 */
#define ADC3_DSM_SEL_OUT_EDGE		7
#define ADC3_DSM_OTA_IB_SEL		4
#define ADC3_DSM_OP_BIAS_CTL		2
#define ADC3_PGA_OP_BIAS_CTL		0
/* SUNXI_ADC3_REG3 */
#define ADC3_PGA_OTA_IB_SEL		5
#define MIC_START_CTL			4
#define ADC3_PGA_GAIN_CTL		0
/* SUNXI_ADC3_REG4 */
#define ADC3_PGA_CHOPPER_EN		7
#define ADC3_PGA_CHOPPER_NOL_EN		6
#define ADC3_PGA_CHOPPER_CKSET		4
#define ADC3_PGA_CHOPPER_DLY_SET	2
#define ADC3_PGA_CHOPPER_NOL_DLY_SET	0
/* SUNXI_I2S_CTL_2 */
#define SPK_OUT_MUTE			7
#define I2S1_BCLK_IOEN			6
#define I2S0_BCLK_IOEN			5
#define SDO_EN_2			4
#define OUT_MUTE_2			3
#define TX_EN				2
#define RX_EN				1
#define I2S_GEN				0
/* SUNXI_I2S_BCLK_CLT1 */
#define I2S1_BCLK_DIV			4
#define I2S0_BCLK_DIV			0
/* SUNXI_I2S_BCLK_CLT2 */
#define I2S1_EDGE_TRANSFER		5
#define I2S1_BCLK_POLARITY		4
#define I2S0_EDGE_TRANSFER		1
#define I2S0_BCLK_POLARITY		0
/* SUNXI_I2S_LRCK_CLT1 */
#define I2S1_LRCK_DIR			6
#define I2S1_LRCK_WIDTH			5
#define I2S1_LRCK_POLARITY		4
#define I2S0_LRCK_DIR			2
#define I2S0_LRCK_WIDTH			1
#define I2S0_LRCK_POLARITY		0
/* SUNXI_I2S_LRCK_CLT2 */
#define I2S1_LRCK_PERIOD_H		4
#define I2S0_LRCK_PERIOD_H		0
/* SUNXI_I2S_LRCK_CLT3 */
#define I2S0_LRCK_PERIOD_L		0
/* SUNXI_I2S_LRCK_CLT4 */
#define I2S1_LRCK_PERIOD_L		0
/* SUNXI_I2S_FMT_CLT1 */
#define ENCD_FMT			7
#define ENCD_SEL			6
#define OFFSET				2
#define TX_SLOT_HIZ			1
#define TX_STATE			0
/* SUNXI_I2S0_FMT_CLT2 */
#define I2S0_SW				4
#define I2S0_SR				0
/* SUNXI_I2S1_FMT_CLT2 */
#define I2S1_SW				4
#define I2S1_SR				0
/* SUNXI_I2S_FMT_CLT3 */
#define RX_MLS_2			7
#define TX_MLS_2			6
#define RX_PDM_2			2
#define TX_PDM_2			0
/* SUNXI_I2S_FMT_CLT4 */
#define I2S1_SEXT			6
#define I2S1_MODE_SEL			4
#define I2S0_SEXT			2
#define I2S0_MODE_SEL			0
/* SUNXI_I2S_TX_CLT1 */
#define TX_CHSEL			0
/* SUNXI_I2S_TX_CLT2 */
#define TX_CHSEL_LOW			0
/* SUNXI_I2S_TX_CLT3 */
#define TX_CHSEL_HIGH			0
/* SUNXI_I2S_TX_MIX_CTL */
#define TX_MIX3				4
#define TX_MIX2				2
#define TX_MIX1				0
/* SUNXI_I2S_TX_CHMP_CTL1 */
#define TX_CH4_MAP_2			6
#define TX_CH3_MAP_2			4
#define TX_CH2_MAP_2			2
#define TX_CH1_MAP_2			0
/* SUNXI_I2S_TX_CHMP_CTL2 */
#define TX_CH8_MAP_2			6
#define TX_CH7_MAP_2			4
#define TX_CH6_MAP_2			2
#define TX_CH5_MAP_2			0
/* SUNXI_I2S_TX_CHMP_CTL3 */
#define TX_CH12_MAP_2			6
#define TX_CH11_MAP_2			4
#define TX_CH10_MAP_2			2
#define TX_CH9_MAP_2			0
/* SUNXI_I2S_TX_CHMP_CTL4 */
#define TX_CH16_MAP_2			6
#define TX_CH15_MAP_2			4
#define TX_CH14_MAP_2			2
#define TX_CH13_MAP_2			0
/* SUNXI_I2S_RX_CTL1 */
#define RX_CHSEL			0
/* SUNXI_I2S_RX_CTL2 */
#define RX_CHSEL_LOW			0
/* SUNXI_I2S_RX_CTL3 */
#define RX_CHSEL_HIGH			0
/* SUNXI_I2S_RX_MIX_CTL */
#define RX_MIX3				4
#define RX_MIX2				2
#define RX_MIX1				0
/* SUNXI_I2S_RX_CHMP_CTL_2 */
#define RXR_MAP				4
#define RXL_MAP				0
/* SUNXI_DEBUG_CTL */
#define ADDA_DEBUG_MODE			0
/* SUNXI_I2S_PADDRV_CTL */
#define PADDRV_CTL_FOR_MCLK		6
#define PADDRV_CTL_FOR_BCLK		4
#define PADDRV_CTL_FOR_LRCK		2
#define PADDRV_CTL_FOR_SDOUTIN		0
/* SUNXI_DEBUG_PADDRV_CTL */
#define PADDRV_CTL_FOR_IRQ		6
#define PADDRV_CTL_FOR_DEVID		4
#define PADDRV_CTL_FOR_HP_DET		2
#define PADDRV_CTL_FOR_DMIC_CLK		0
/* SUNXI_DIG_BYPASS_CTL_2 */
#define HP_AUTO_CLEAR			1
/* SUNXI_EFUSE_WR_CTL */
#define EFUSE_BIT_VTRL			3
#define EFUSE_READ_EN			0
/* SUNXI_ADDA_FS_CTL_2 */
#define DAC_FS_DIV			4
#define ADC_FS_DIV			0
/* SUNXI_ADC_DIG_EN_2 */
#define REQ_WIDTH			4
#define ADC_EN_2			3
/* SUNXI_ADC_DDT_CTL_2 */
#define ADC_DVC_ZCD_EN			7
#define ADC_OSR				5
#define ADC_DLY_MODE			3
#define ADC_DTS				0
/* SUNXI_HMIC_DET_CTL */
#define KEYUP_CLEAR			5
#define HMIC_DATA_IRQ_MODE		4
#define HMIC_SF				2
#define HMIC_SAMPLE_SEL			0
/* SUNXI_HMIC_DET_DBC */
#define HMIC_M				4
#define HMIC_N				0
/* SUNXI_HMIC_DET_TH1 */
#define HMIC_TH1_HYSTERESIS		5
#define HMIC_TH1			0
/* SUNXI_HMIC_DET_TH2 */
#define HMIC_TH2_HYSTERESIS		5
#define HMIC_TH2			0
/* SUNXI_HMIC_DET_DATA */
#define HMIC_DATA			0
/* SUNXI_HP_DET_CTL */
#define HP_DET				4
#define HMIC_DET_AUTO_DIS		3
#define HMIC_DET_AUTO_EN		2
#define DET_POLY			1
#define HP_DET_EN			0
/* SUNXI_HP_DET_DBC */
#define PULL_OUT_DBC			4
#define PLUG_IN_DBC			0
/* SUNXI_HP_DET_IRQ */
#define HP_PULLOUT_IRQ			6
#define HP_PLUGIN_IRQ			5
#define HMIC_PULLOUT_IRQ		4
#define HMIC_PLUGIN_IRQ			3
#define HMIC_KEYUP_IRQ			2
#define HMIC_KEYDOWN_IRQ		1
#define HMIC_DATA_IRQ_EN		0
/* SUNXI_HP_DET_STA */
#define HP_PULLOUT_PENDING		6
#define HP_PLUGIN_PENDING		5
#define HMIC_PULLOUT_PENDING		4
#define HMIC_PLUGIN_PENDING		3
#define HMIC_KEYUP_PENDING		2
#define HMIC_KEYDOWN_PENDING		1
#define HMIC_DATA_PENDING		0
/* SUNXI_ADC_DIG_DEBUG_2 */
#define DSM_DITHER_CTL			4
/* SUNXI_DAC_DIG_EN_2 */
#define EN_DAC				2
#define EN_DACL				1
#define EN_DACR				0
/* SUNXI_DAC_DIG_CTL_2 */
#define DAC_OSR				0
/* SUNXI_HP_AVR_CTL_2 */
#define HP_AVR_DEN			7
#define HVH				4
#define HVL				0
/* SUNXI_HP_AVR_THH */
#define THH_2				0
/* SUNXI_HP_AVR_THM */
#define THM_2				0
/* SUNXI_HP_AVR_THL */
#define THL_2				0
/* SUNXI_HP_AVR_DBC_2 */
#define HV_DBC				0
/* SUNXI_INT_ADDR_CONF_REG */
#define INT_ADDR_CONF_EN		7
#define INT_ADDR			0

#define ADCHPF1_SHIFT		1
#define ADCHPF2_SHIFT		2
#define ADCHPF3_SHIFT		3
#define DACDRC_SHIFT		4

struct ac203c_status {
	struct mutex audio_mutex;
	bool lineoutl;
	bool lineoutr;
	bool hpout;

	bool adc1;
	bool adc2;
	bool adc3;

	bool play;
	bool cap;
	unsigned int play_rate;
	unsigned int cap_rate;
};

enum SUNXI_JACK_IRQ_STA {
	JACK_IRQ_NULL	= -1,
	JACK_IRQ_OUT	= 0,
	JACK_IRQ_IN,
	JACK_IRQ_KEYDOWN,
};

/* jack */
struct sunxi_jack_adv_priv {
	struct regmap *regmap;
	struct device *dev;

	bool typec;

	unsigned int det_threshold;
	unsigned int key_threshold;
	unsigned int det_debounce;
	unsigned int key_debounce;

	/* key_det_vol[0][0] - key_hook_vol_min,  key_det_vol[0][1] - key_hook_vol_max
	 * key_det_vol[1][0] - key_up_vol_min,    key_det_vol[1][1] - key_up_vol_max
	 * key_det_vol[2][0] - key_down_vol_min,  key_det_vol[2][1] - key_down_vol_max
	 * key_det_vol[3][0] - key_voice_vol_min, key_det_vol[3][1] - key_voice_vol_max
	 */
	unsigned int key_det_vol[4][2];

	enum SUNXI_JACK_IRQ_STA irq_sta;

	int rst_gpio;

	// gpio get irq
	int irq_gpio;
	struct gpio_desc *desc;

	enum snd_jack_types jack_type;

	/* pa config */
	unsigned int pa_pin_max;
	struct snd_sunxi_pacfg *pa_cfg;
};

struct sunxi_codec_mem {
	struct resource res;
	void __iomem *membase;
	struct resource *memregion;
	struct regmap *dig_regmap;
	struct regmap *ana_regmap;
};

struct sunxi_codec_clk {
	/* parent */
	struct clk *clk_pll_audio0;
	struct clk *clk_pll_audio1_5x;
	/* module */
	struct clk *clk_adda_dac;
	/* bus & reset */
	struct clk *clk_bus;
	struct reset_control *clk_rst;
	/* record current clk */
	struct clk *clk_pll_play;
};

struct sunxi_codec_pinctl {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct ac203c_data {
	unsigned int adc1_vol;
	unsigned int adc2_vol;
	unsigned int adc3_vol;

	unsigned int dacl_vol;
	unsigned int dacr_vol;

	unsigned int mic1_gain;
	unsigned int mic2_gain;
	unsigned int mic3_gain;

	unsigned int dac_gain;

	struct ac203c_status ac203c_sta;
	struct sunxi_jack_adv_priv jack_adv_priv;
};

#endif /* __SND_SUN65IW1_AC203C_H */
