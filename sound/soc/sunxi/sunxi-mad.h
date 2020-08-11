/*
 * sound\soc\sunxi\sunxi_mad.h
 * (C) Copyright 2018-2023
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Wolfgang <qinzhenying@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_MAD_H
#define __SUNXI_MAD_H

#include "sunxi-pcm.h"

struct sunxi_mad_priv {
    unsigned int mad_bind;
};

/*------------------MAD register definition--------------------*/
#define SUNXI_MAD_CTRL			0x00
#define SUNXI_MAD_SRAM_POINT		0x04
#define SUNXI_MAD_SRAM_SIZE		0x08
#define SUNXI_MAD_SRAM_RD_POINT		0x0C
#define SUNXI_MAD_RD_SIZE		0x10
#define SUNXI_MAD_SRAM_STORE_TH		0x14
#define SUNXI_MAD_SRAM_AHB1_TX_TH	0x18
#define SUNXI_MAD_SRAM_AHB1_RX_TH	0x1C
#define SUNXI_MAD_SRAM_WAKE_BACK_DATA	0x20
#define SUNXI_MAD_AD_PATH_SEL		0x24
#define SUNXI_MAD_LPSD_AD_SYNC_FC	0x28
#define SUNXI_MAD_LPSD_TH		0x2C
#define SUNXI_MAD_LPSD_RRUN		0x30
#define	SUNXI_MAD_LPSD_RSTOP		0x34
#define	SUNXI_MAD_LPSD_ECNT		0x38
#define SUNXI_MAD_SRAM_CH_MASK		0x3C
#define SUNXI_MAD_LPSD_CH_MASK		0x40
#define SUNXI_MAD_SRAM_SEC_REGION_REG	0x44
#define SUNXI_MAD_SRAM_PRE_DSIZE	0x48
#define SUNXI_MAD_DMA_TF_SIZE		0x4C
#define SUNXI_MAD_DMA_TF_LAST_SIZE	0x50
#define	SUNXI_MAD_INT_ST_CLR		0x60
#define	SUNXI_MAD_INT_MASK		0x64
#define	SUNXI_MAD_STA			0x68
#define	SUNXI_MAD_DEBUG			0x6C


/*SUNXI_MAD_CTRL: 0x00*/
#define AUDIO_DATA_SYNC_FRC		7
#define SRAM_RST			6
#define DMA_TYPE			5
#define DMA_EN				4
#define CPUS_RD_DONE			3
#define GO_ON_SLEEP			2
#define KEY_WORD_OK			1
#define MAD_EN				0
/* DMA type*/
#define DMA_TYPE_MASK			0x1
#define DMA_TYPE_IO			0x1
#define DMA_TYPE_MEM			0x0

/*SUNXI_MAD_SRAM_POINT: 0x04*/
#define MAD_SRAM_PONT			0

/*SUNXI_MAD_SRAM_SIZE: 0x08*/
#define MAD_SRAM_SIZE			0

/*SUNXI_MAD_SRAM_RD_POINT: 0x0C*/
#define MAD_SRAM_RD_POINT		0

/*SUNXI_MAD_SRAM_RD_SIZE(unit: half word): 0x10*/
#define MAD_SRAM_RD_SIZE		0

/*SUNXI_MAD_SRAM_STORE_TH(unit: half word): 0x14*/
#define MAD_SRAM_STORE_TH		0

/*SUNXI_MAD_SRAM_AHB1_TX_TH(unit: byte): 0x18*/
#define MAD_SRAM_AHB1_TX_TH		0

/*SUNXI_MAD_SRAM_AHB1_RX_TH(unit: byte): 0x1C*/
#define MAD_SRAM_AHB1_RX_TH		0

/*SUNXI_MAD_SRAM_WAKE_BACK_DATA(unit: frame): 0x20*/
#define MAD_SRAM_WAKE_BACK_DATA		0

/*SUNXI_MAD_AD_PATH_SEL: 0x24*/
#define MAD_AD_PATH_SEL			0
#define MAD_AD_PATH_SEL_MASK		0xF
/*MAD audio src sel*/
#define MAD_AD_PATH_NO_SRC		0x0
#define MAD_AD_PATH_I2S0_SRC		0x1
#define MAD_AD_PATH_CODEC_SRC		0x2
#define MAD_AD_PATH_DMIC_SRC		0x3
#define MAD_AD_PATH_I2S1_SRC		0x4
#define MAD_AD_PATH_I2S2_SRC		0x5

/*SUNXI_MAD_LPSD_AD_SYNC_FC: 0x28*/
#define MAD_LPSD_AD_SYNC_FC		0
#define MAD_LPSD_AD_SYNC_FC_DEF		0X20

/*SUNXI_MAD_LPSD_TH: 0x2C*/
#define MAD_LPSD_TH			0

/*SUNXI_MAD_LPSD_RRUN: 0x30*/
#define MAD_LPSD_RRUN			0

/*SUNXI_MAD_LPSD_RSTOP: 0x34*/
#define MAD_LPSD_RSTOP			0

/*SUNXI_MAD_LPSD_ECNT: 0x38*/
#define MAD_LPSD_ECNT			0

/*SUNXI_MAD_SRAM_CH_MASK: 0x3C*/
#define MAD_CH_CHANGE_EN		30
#define MAD_CH_COM_NUM			26
#define MAD_AD_SRC_CH_NUM		21
#define MAD_SRAM_CH_NUM			16
#define MAD_SRAM_CH_MASK		0

#define MAD_SRAM_CH_NUM_MASK    0x1F

/*MAD channel change sel*/
#define MAD_CH_COM_NUM_MASK		0xF
#define MAD_CH_COM_NON			0x0
#define MAD_CH_COM_2CH_TO_4CH		0x1
#define MAD_CH_COM_2CH_TO_6CH		0x2
#define MAD_CH_COM_2CH_TO_8CH		0x3
#define MAD_CH_COM_4CH_TO_6CH		0x4
#define MAD_CH_COM_4CH_TO_8CH		0x5

/*SUNXI_MAD_LPSD_CH_MASK: 0x40*/
#define MAD_LPSD_DCBLOCK_EN		20
#define MAD_LPSD_CH_NUM			16
#define MAD_LPSD_CH_MASK		0
/*LPSD receive 0/1 audio channel mask*/
#define MAD_LPSD_CH_NUM_MASK		0xF
/*LPSD AUDIO channel num sel*/
#define MAD_LPSD_CH_NUM_NON		0x0
#define MAD_LPSD_CH_NUM_1CH		0x1

/*SUNXI_MAD_SRAM_SEC_REGION: 0x44*/
#define MAD_SRAM_SEC_REGION		0

/*SUNXI_MAD_SRAM_PRE_DATA_SIZE(unit: half word): 0x48*/
#define MAD_SRAM_PRE_DATA_SIZE		0

/*SUNXI_MAD_DMA_TF_SIZE: 0x4C*/
#define MAD_DMA_TF_SIZE			0

/*SUNXI_MAD_DMA_TF_LAST_SIZE: 0x50*/
#define MAD_DMA_TF_LAST_SIZE		0

/*SUNXI_MAD_INT_ST_CLR: 0x60*/
#define DATA_REQ_INT			1
#define WAKE_INT			0

/*SUNXI_MAD_INT_MASK: 0x64*/
#define DATA_REQ_INT_MASK		1
#define MAD_REQ_INT_MASK		0

/*SUNXI_MAD_STATE_REG: 0x68*/
#define MAD_LPSD_STAT			8
#define MAD_STATE			4
#define MAD_SRAM_FULL			2
#define MAD_SRAM_EMPTY			1
#define MAD_RUN				0
/*MAD STATE(read only)*/
#define MAD_STATE_IDLE			0x0
#define MAD_STATE_WAIT			0x1
#define MAD_STATE_RUN			0x2
#define MAD_STATE_NORMAL		0x4

/*SUNXI_MAD_DEBUG: 0x6C*/
#define MAD_CFG_ERR			4
#define MAD_SRAM_FULL_ERR		3
#define MAD_SRAM_EMPTY_ERR		2
#define DATA_SRAM_ADDR_ERR		1
#define MAD_SRAM_SEC_ERR		0
/*MAD_CFG_ERR mask*/
#define MAD_CFG_ERR_MASK		0x3

extern void sunxi_mad_init(void);
extern void sunxi_sram_dma_config(struct sunxi_dma_params *capture_dma_param);
extern int sunxi_mad_hw_params(unsigned int mad_channels, unsigned int sample_rate);
extern int sunxi_mad_audio_source_sel(unsigned int path_sel, unsigned int enable);
extern int sunxi_mad_suspend_external(unsigned int lpsd_chan_sel,
	unsigned int mad_standby_chan_sel);
extern int sunxi_mad_resume_external(unsigned int mad_standby_chan_sel,
	unsigned int audio_src_chan_num);
extern void mad_dma_en(unsigned int en);
#endif /* SUNXI_MAD_H */
