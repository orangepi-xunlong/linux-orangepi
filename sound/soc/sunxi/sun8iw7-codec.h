/*
 * sound\soc\sunxi\sun8iw11-codec.h
 * (C) Copyright 2014-2016
 * allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Wolfgang Huang <huangjinhui@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SUN8IW7_CODEC_H
#define _SUN8IW7_CODEC_H



#define SUNXI_DAC_DPC		0x00
#define SUNXI_DAC_FIFO_CTR	0x04
#define SUNXI_DAC_FIFO_STA	0x08
/* left blank */
#define	SUNXI_ADC_FIFO_CTR	0x10
#define SUNXI_ADC_FIFO_STA	0x14
#define SUNXI_ADC_RXDATA	0x18
#define SUNXI_DAC_TXDATA	0x20
/* left blank */
#define SUNXI_DAC_CNT		0x40
#define SUNXI_ADC_CNT		0x44
#define SUNXI_DAC_DG		0x48
#define SUNXI_ADC_DG		0x4C


/*left blank */
#define	SUNXI_DAC_DAP_CTR	0x60
#define	SUNXI_ADC_DAP_CTR	0x70
#define SUNXI_ADC_DRC_HHPFC	0x200
#define SUNXI_ADC_DRC_LHPFC	0x204

/* Analog register base - Digital register base */
#define SUNXI_PR_CFG		0x01F015C0

#define SUNXI_LINEOUT_PA_GAT		(SUNXI_PR_CFG + 0x00)
#define SUNXI_LOMIX_SRC		(SUNXI_PR_CFG + 0x01)
#define SUNXI_ROMIX_SRC		(SUNXI_PR_CFG + 0x02)
#define SUNXI_DAC_PA_SRC	(SUNXI_PR_CFG + 0x03)
#define SUNXI_LINEIN_GCTR		(SUNXI_PR_CFG + 0x05)
#define SUNXI_MIC_GCTR	(SUNXI_PR_CFG + 0x06)
#define SUNXI_PAEN_LINEOUT_CTR	(SUNXI_PR_CFG + 0x07)
#define SUNXI_LINEOUT_VOL	(SUNXI_PR_CFG + 0x09)

/* left blank */
#define SUNXI_MIC2G_LINEOUT_CTR	(SUNXI_PR_CFG + 0x0A)
#define SUNXI_MIC1G_MICBIAS_CTR	(SUNXI_PR_CFG + 0x0B)
#define SUNXI_LADCMIX_SRC	(SUNXI_PR_CFG + 0x0C)
#define SUNXI_RADCMIX_SRC	(SUNXI_PR_CFG + 0x0D)
#define SUNXI_ADC_AP_EN		(SUNXI_PR_CFG + 0x0F)
#define SUNXI_ADDA_APT0		(SUNXI_PR_CFG + 0x10)
#define SUNXI_ADDA_APT1		(SUNXI_PR_CFG + 0x11)
#define SUNXI_ADDA_APT2		(SUNXI_PR_CFG + 0x12)
#define SUNXI_BIAS_DA16_CAL_CTR0	(SUNXI_PR_CFG + 0x13)
#define SUNXI_BIAS_DA16_CAL_CTR1	(SUNXI_PR_CFG + 0x14)
#define SUNXI_DA16_CALI_DATA	(SUNXI_PR_CFG + 0x15)
#define SUNXI_DA16_VERIFY	(SUNXI_PR_CFG + 0x16)

/* left blank */
#define SUNXI_BIAS_CALI_DATA	(SUNXI_PR_CFG + 0x17)
#define SUNXI_BIAS_VERIFY	(SUNXI_PR_CFG + 0x18)

/* SUNXI_DAC_DPC:0x00 */
#define EN_DAC			31
#define MODQU			25
#define HPF_EN			18
#define DVOL			12
#define HUB_EN			0

/* SUNXI_DAC_FIFO_CTR:0x04 */
#define DAC_FS			29
#define FIR_VER			28
#define SEND_LASAT		26
#define FIFO_MODE		24
#define DAC_DRQ_CLR_CNT		21
#define TX_TRIG_LEVEL		8
#define ADDA_LOOP_EN		7
#define DAC_MONO_EN		6
#define TX_SAMPLE_BITS		5
#define DAC_DRQ_EN		4
#define DAC_IRQ_EN		3
#define FIFO_UNDERRUN_IRQ_EN	2
#define FIFO_OVERRUN_IRQ_EN	1
#define FIFO_FLUSH		0

/* SUNXI_DAC_FIFO_STA:0x08 */
#define	TX_EMPTY		23
#define	TXE_CNT			8
#define	TXE_INT			3
#define	TXU_INT			2
#define	TXO_INT			1

/* SUNXI_ADC_FIFO_CTR:0x10 */
#define ADC_FS			29
#define EN_AD			28
#define RX_FIFO_MODE		24
#define ADCFDT			17
#define ADCDFEN			16
#define RX_FIFO_TRG_LEVEL	8
#define ADC_MONO_EN		7
#define RX_SAMPLE_BITS		6
#define ADC_DRQ_EN		4
#define ADC_IRQ_EN		3
#define ADC_OVERRUN_IRQ_EN	1
#define ADC_FIFO_FLUSH		0

/* SUNXI_ADC_FIFO_STA:0x14 */
#define	RXA			23
#define	RXA_CNT			8
#define	RXA_INT			3
#define	RXO_INT			1

/* SUNXI_DAC_DG:0x48 */
#define DAC_MODU_SELECT			  11
#define DAC_PATTERN_SELECT		  9
#define CODEC_CLK_SELECT		  8
#define DAC_SWP					  6

/* SUNXI_ADC_DG:0xC */
#define AD_SWAP			  24


/* SUNXI_DAC_DAP_CTR:0x60 */
#define	DDAP_EN			31
#define	DDAP_DRC_EN		15
#define	DDAP_HPF_EN		14


/* SUNXI_ADC_DAP_CTR:0x70 */
#define	ENADC_DRC		26
#define	ADC_DRC_EN		25
#define	ADC_DRC_HPF_EN		24

/*AC_ADC_DRC_HHPFC : 0x200*/
#define ADC_HHPF_CONF		0

/*AC_ADC_DRC_LHPFC : 0x204*/
#define ADC_LHPF_CONF		0

/* SUNXI_PR_CFG:0x01F015C0 */
#define AC_PR_RST		28
#define AC_PR_RW		24
#define AC_PR_ADDR		16
#define ADDA_PR_WDAT		8
#define ADDA_PR_RDAT		0

/* SUNXI_LINEOUT_PA_GAT:0x00 */
#define PA_CLK_GATING		7


/* SUNXI_LOMIX_SRC:0x01 */
#define LMIXMUTE		0
#define	LMIX_MIC1_BST		6
#define LMIX_MIC2_BST		5
#define LMIX_LINEINL		2
#define LMIX_LDAC		1
#define LMIX_RDAC		0

/* SUNXI_ROMIX_SRC:0x02 */
#define RMIXMUTE		0
#define RMIX_MIC1_BST		6
#define RMIX_MIC2_BST		5
#define RMIX_LINEINR		2
#define RMIX_RDAC		1
#define RMIX_LDAC		0

/* SUNXI_DAC_PA_SRC:0x03 */
#define DACAREN			7
#define DACALEN			6
#define RMIXEN			5
#define LMIXEN			4

/* SUNXI_LINEIN_GCTR:0x05 */
#define LINEING		4


/* SUNXI_MIC_GCTR:0x06 */
#define MIC1_GAIN		4
#define MIC2_GAIN		0

/* SUNXI_PAEN_LINEOUT_CTR:0x07 */
#define LINEOUTEN			7
#define PA_ANTI_POP_CTRL	2

/* SUNXI_LINEOUT_VOL:0x09 */
#define LINEOUT_VOL			3


/* SUNXI_MIC2G_LINEOUT_CTR:0x0A */
#define MIC2AMPEN		7
#define MIC2BOOST		4
#define LINEOUTL_EN		 3
#define LINEOUTR_EN		 2
#define LINEOUTL_SRC	 1
#define LINEOUTR_SRC	 0


/* SUNXI_MIC1G_MICBIAS_CTR:0x0B */
#define MMICBIASEN		6
#define MIC1_AMPEN		3
#define MIC1_BOOST		0

/* SUNXI_LADCMIX_SRC:0x0C */
#define LADCMIXMUTE		0
#define LADC_MIC1_BST		6
#define LADC_MIC2_BST		5
#define LADC_LINEINL		2
#define LADC_LOUT_MIX		1
#define LADC_ROUT_MIX		0

/* SUNXI_RADCMIX_SRC:0x0D */
#define RADCMIXMUTE		0
#define RADC_MIC1_BST		6
#define RADC_MIC2_BST		5
#define RADC_LINEINR		2
#define RADC_ROUT_MIX		1
#define RADC_LOUT_MIX		0


/* SUNXI_ADC_AP_EN:0x0F */
#define ADCREN			7
#define ADCLEN			6
#define ADCG			0

/* SUNXI_ADDA_APT0:0x10 */
#define	OPDRV_OPCOM_CUR		6
#define	OPADC1_BIAS_CUR		4
#define	OPADC2_BIAS_CUR		2
#define	OPAAF_BIAS_CUR		0

/* SUNXI_ADDA_APT1:0x11 */
#define	OPMIC_BIAS_CUR		6
#define	OPDAC_BIAS_CUR		2
#define	OPMIX_BIAS_CUR		0

/* SUNXI_ADDA_APT2:0x12 */
#define	ZERO_CROSS_EN		7
#define	ZERO_CROSS_TIMEOUT	6
#define	PTDBS			4
#define PA_SLOPE_SELECT		3
#define	USB_BIAS_CUR		0

/* SUNXI_BIAS_DA16_CAL_CTR0:0x13*/
#define MMIC_BIAS_CHOP_EN		7
#define	MMIC_BIAS_CLK_SEL		5
#define	DITHER	4
#define	DITHER_CLK_SELECT	2
#define	BIHE_CTRL		0

/* SUNXI_BIAS_DA16_CAL_CTR1:0x14*/
#define PA_SPEED_SELECT		7
#define	CUR_TEST_SEL		6
#define	BIAS_DA16_CLK_SEL	4
#define	BIAS_CAL_MODE_SEL	3
#define	BIAS_DA16_CAL_CTRL	2
#define	BIAS_CAL_VER		1
#define	DA16_CAL_VERIFY		0

/* SUNXI_DA16_CALI_DATA:0X15*/
#define DA16CALI_DATA			0

/* SUNXI_BIAS_CALI_DATA:0X17*/
#define BIAS_CALI_DATA			0

/* SUNXI_BIAS_CALI_SET:0X18*/
#define BIAS_VERIFY_DATA		0


#endif /* __SUN8IW11_CODEC_H */
