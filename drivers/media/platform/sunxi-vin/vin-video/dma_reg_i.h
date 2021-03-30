/*
 ******************************************************************************
 *
 * dma_reg_i.h
 *
 * CSIC - dma_reg_i.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    	Description
 *
 *   1.0		  Zhao Wei   	2016/06/13	      First Version
 *
 ******************************************************************************
 */
#ifndef __CSIC__DMA__REG__I__H__
#define __CSIC__DMA__REG__I__H__

/*
 * Detail information of registers
 */
#define	CSIC_DMA_EN_REG_OFF			0X000
#define	CSIC_DMA_EN				0
#define	CSIC_DMA_EN_MASK			(0X1 << CSIC_DMA_EN)
#define	CSIC_CLK_CNT_EN				1
#define	CSIC_CLK_CNT_EN_MASK			(0X1 << CSIC_CLK_CNT_EN)
#define	CSIC_CLK_CNT_SPL			2
#define	CSIC_CLK_CNT_SPL_MASK			(0X1 << CSIC_CLK_CNT_SPL)

#define	CSIC_DMA_CFG_REG_OFF			0X004
#define	MIN_SDR_WR_SIZE				0
#define	MIN_SDR_WR_SIZE_MASK			(0X3 << MIN_SDR_WR_SIZE)
#define	FPS_DS					6
#define	FPS_DS_MASK				(0X3 << FPS_DS)
#define	FIELD_SEL				10
#define	FIELD_SEL_MASK				(0X3 << FIELD_SEL)
#define	HFLIP_EN				12
#define	HFLIP_EN_MASK				(0X1 << HFLIP_EN)
#define	VFLIP_EN				13
#define	VFLIP_EN_MASK				(0X1 << VFLIP_EN)
#define	OUTPUT_FMT				16
#define	OUTPUT_FMT_MASK				(0XF << OUTPUT_FMT)
#define	PAD_VAL					24
#define	PAD_VAL_MASK				(0XFF << PAD_VAL)

#define	CSIC_DMA_HSIZE_REG_OFF			0X010
#define	HOR_START				0
#define	HOR_START_MASK				(0X1FFF << HOR_START)
#define	HOR_LEN					16
#define	HOR_LEN_MASK				(0X1FFF << HOR_LEN)

#define	CSIC_DMA_VSIZE_REG_OFF			0X014
#define	VER_START				0
#define	VER_START_MASK				(0X1FFF << VER_START)
#define	VER_LEN					16
#define	VER_LEN_MASK				(0X1FFF << VER_LEN)

#define	CSIC_DMA_F0_BUFA_REG_OFF		0X020
#define	F0_BUFA					0
#define	F0_BUFA_MASK				(0XFFFFFFFF << F0_BUFA)

#define	CSIC_DMA_F1_BUFA_REG_OFF		0X028
#define	F1_BUFA					0
#define	F1_BUFA_MASK				(0XFFFFFFFF << F1_BUFA)

#define	CSIC_DMA_F2_BUFA_REG_OFF		0X030
#define	F2_BUFA					0
#define	F2_BUFA_MASK				(0XFFFFFFFF << F2_BUFA)

#define	CSIC_DMA_BUF_LEN_REG_OFF		0X038
#define	BUF_LEN					0
#define	BUF_LEN_MASK				(0X3FFF << BUF_LEN)
#define	BUF_LEN_C				16
#define	BUF_LEN_C_MASK				(0X3FFF << BUF_LEN_C)

#define	CSIC_DMA_FLIP_SIZE_REG_OFF		0X03C
#define	VALID_LEN				0
#define	VALID_LEN_MASK				(0X1FFF << VALID_LEN)
#define	VER_LEN					16
#define	VER_LEN_MASK				(0X1FFF << VER_LEN)

#define	CSIC_DMA_CAP_STA_REG_OFF		0X04C
#define	SCAP_STA				0
#define	SCAP_STA_MASK				(0X1 << SCAP_STA)
#define	VCAP_STA				1
#define	VCAP_STA_MASK				(0X1 << VCAP_STA)
#define	FIELD_STA				2
#define	FIELD_STA_MASK				(0X1 << FIELD_STA)

#define	CSIC_DMA_INT_EN_REG_OFF			0X050
#define	CD_INT_EN				0
#define	CD_INT_EN_MASK				(0X1 << CD_INT_EN)
#define	FD_INT_EN				1
#define	FD_INT_EN_MASK				(0X1 << FD_INT_EN)
#define	FIFO0_OF_INT_EN				2
#define	FIFO0_OF_INT_EN_MASK			(0X1 << FIFO0_OF_INT_EN)
#define	FIFO1_OF_INT_EN				3
#define	FIFO1_OF_INT_EN_MASK			(0X1 << FIFO1_OF_INT_EN)
#define	FIFO2_OF_INT_EN				4
#define	FIFO2_OF_INT_EN_MASK			(0X1 << FIFO2_OF_INT_EN)
#define	LC_INT_EN				5
#define	LC_INT_EN_MASK				(0X1 << LC_INT_EN)
#define	HB_OF_INT_EN				6
#define	HB_OF_INT_EN_MASK			(0X1 << HB_OF_INT_EN)
#define	VS_INT_EN				7
#define	VS_INT_EN_MASK				(0X1 << VS_INT_EN)

#define	CSIC_DMA_INT_STA_REG_OFF		0X054
#define	CD_PD					0
#define	CD_PD_MASK				(0X1 << CD_PD)
#define	FD_PD					1
#define	FD_PD_MASK				(0X1 << FD_PD)
#define	FIFO0_OF_PD				2
#define	FIFO0_OF_PD_MASK			(0X1 << FIFO0_OF_PD)
#define	FIFO1_OF_PD				3
#define	FIFO1_OF_PD_MASK			(0X1 << FIFO1_OF_PD)
#define	FIFO2_OF_PD				4
#define	FIFO2_OF_PD_MASK			(0X1 << FIFO2_OF_PD)
#define	LC_PD					5
#define	LC_PD_MASK				(0X1 << LC_PD)
#define	HB_OF_PD				6
#define	HB_OF_PD_MASK				(0X1 << HB_OF_PD)
#define	VS_PD					7
#define	VS_PD_MASK				(0X1 << VS_PD)

#define	CSIC_DMA_LINE_CNT_REG_OFF		0X058
#define	LINE_CNT_NUM				0
#define	LINE_CNT_NUM_MASK			(0X1FFF << LINE_CNT_NUM)

#define	CSIC_DMA_FRM_CLK_CNT_REG_OFF		0X060
#define	FRM_CLK_CNT				0
#define	FRM_CLK_CNT_MASK			(0XFFFFFF << FRM_CLK_CNT)

#define	CSIC_DMA_ACC_ITNL_CLK_CNT_REG_OFF	0X064
#define	ITNL_CLK_CNT				0
#define	ITNL_CLK_CNT_MASK			(0XFFFFFF << ITNL_CLK_CNT)
#define	ACC_CLK_CNT				24
#define	ACC_CLK_CNT_MASK			(0XFF << ACC_CLK_CNT)

#define	CSIC_DMA_FIFO_STAT_REG_OFF		0X068
#define	FIFO_FRM_MAX				0
#define	FIFO_FRM_MAX_MASK			(0XFFF << FIFO_FRM_MAX)

#define	CSIC_DMA_FIFO_THRS_REG_OFF		0X06C
#define	FIFO_THRS				0
#define	FIFO_THRS_MASK				(0XFFF << FIFO_THRS)
#define	FIFO_NEARLY_FULL_TH			13
#define	FIFO_NEARLY_FULL_TH_MASK		(0XFF << FIFO_NEARLY_FULL_TH)

#define	CSIC_DMA_PCLK_STAT_REG_OFF		0X070
#define	PCLK_CNT_LINE_MIN			0
#define	PCLK_CNT_LINE_MIN_MASK			(0XFFF << PCLK_CNT_LINE_MIN)
#define	PCLK_CNT_LINE_MAX			16
#define	PCLK_CNT_LINE_MAX_MASK			(0XFF << PCLK_CNT_LINE_MAX)

#endif /*__CSIC__DMA__REG__I__H__*/
