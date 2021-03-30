/*
 ******************************************************************************
 *
 * top_reg_i.h
 *
 * CSIC - top_reg_i.h module
 *
 * Copyright (c) 2016 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		      Description
 *
 *   1.0		  Zhao Wei   	2016/06/13	      First Version
 *
 ******************************************************************************
 */
#ifndef __CSIC__TOP__REG__I__H__
#define __CSIC__TOP__REG__I__H__

/*
 * Detail information of registers
 */
#define CSIC_TOP_EN_REG_OFF			0X000
#define CSIC_TOP_EN				0
#define CSIC_TOP_EN_MASK			(0X1 << CSIC_TOP_EN)
#define CSIC_WDR_MODE_EN        		1
#define CSIC_WDR_MODE_EN_MASK			(0X1 << CSIC_WDR_MODE_EN)
#define CSIC_EN_RES0               		2
#define CSIC_EN_RES0_MASK			(0X1 << CSIC_EN_RES0)
#define CSIC_SRAM_PWDN				8
#define CSIC_SRAM_PWDN_MASK			(0X1 << CSIC_SRAM_PWDN)
#define CSIC_EN_RES1               		9
#define CSIC_EN_RES1_MASK			(0X1 << CSIC_EN_RES1)
#define CSIC_NCSI_TOP_SWITCH			30
#define CSIC_NCSI_TOP_SWITCH_MASK		(0X1 << CSIC_NCSI_TOP_SWITCH)
#define CSIC_VER_EN                     	31
#define CSIC_VER_EN_MASK			(0X1 << CSIC_VER_EN)

#define CSIC_ISP0_IN0_REG_OFF			0X030
#define CSIC_ISP0_IN1_REG_OFF			0X034
#define CSIC_ISP0_IN2_REG_OFF			0X038
#define CSIC_ISP0_IN3_REG_OFF			0X03C

#define CSIC_ISP1_IN0_REG_OFF			0X040
#define CSIC_ISP1_IN1_REG_OFF			0X044
#define CSIC_ISP1_IN2_REG_OFF			0X048
#define CSIC_ISP1_IN3_REG_OFF			0X04C

#define CSIC_VIPP0_IN_REG_OFF			0X060
#define CSIC_VIPP1_IN_REG_OFF			0X064
#define CSIC_VIPP2_IN_REG_OFF			0X068
#define CSIC_VIPP3_IN_REG_OFF			0X06C


#define CSIC_FEATURE_REG_OFF			0X070
#define CSIC_FEATURE_RES0			0
#define CSIC_FEATURE_RES0_MASK			(0XFF << CSIC_FEATURE_RES0)
#define CSIC_DMA_NUM				8
#define CSIC_DMA_NUM_MASK			(0XF << CSIC_DMA_NUM)
#define CSIC_VIPP_NUM				12
#define CSIC_VIPP_NUM_MASK			(0XF << CSIC_VIPP_NUM)
#define CSIC_ISP_NUM				16
#define CSIC_ISP_NUM_MASK			(0XF << CSIC_ISP_NUM)
#define CSIC_NCSI_NUM				20
#define CSIC_NCSI_NUM_MASK			(0XF << CSIC_NCSI_NUM)
#define CSIC_MCSI_NUM				24
#define CSIC_MCSI_NUM_MASK			(0XF << CSIC_MCSI_NUM)
#define CSIC_PARSER_NUM				28
#define CSIC_PARSER_NUM_MASK			(0XF << CSIC_PARSER_NUM)

#define CSIC_VER_REG_OFF			0X074
#define CSIC_VER_SMALL				0
#define CSIC_VER_SMALL_MASK			(0XFFF << CSIC_VER_SMALL)
#define CSIC_VER_BIG				12
#define CSIC_VER_BIG_MASK			(0XFFF << CSIC_VER_BIG)


#endif /*__CSIC__TOP__REG__I__H__*/

