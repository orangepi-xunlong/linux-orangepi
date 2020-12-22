/*
 * sound\soc\sunxi\audiocodec\switch_hdset.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SWITCH_HEADSET_H
#define _SWITCH_HEADSET_H
#include <mach/platform.h>
/*Codec Register*/

#define baseaddr				SUNXI_AUDIO_VBASE

#ifdef CONFIG_ARCH_SUN8IW5
#define ADDA_PR_CFG_REG     	  (SUNXI_R_PRCM_VBASE+0x1c0)

#define VIR_HMIC_BASSADDRESS      (SUNXI_R_PRCM_VBASE)

#define SUNXI_HMIC_ENABLE          (0x1c4)
#define SUNXI_HMIC_CTL 	           (0x1c8)
#define SUNXI_HMIC_DATA	           (0x1cc)
#else/*CONFIG_ARCH_SUN8IW8*/
#define ADDA_PR_CFG_REG     	  (SUNXI_AUDIO_VBASE+0x400)
#define VIR_HMIC_BASSADDRESS      (SUNXI_AUDIO_VBASE)
//#define SUNXI_HMIC_ENABLE          (0x1c4)
#define SUNXI_HMIC_CTL 	           (0x50)
#define SUNXI_HMIC_DATA	           (0x54)
#endif

#define HP_VOLC					  (0x00)
#define LOMIXSC					  (0x01)
#define ROMIXSC					  (0x02)
#define DAC_PA_SRC				  (0x03)
#define PAEN_HP_CTRL			  (0x07)
#define ADDA_APT2				  (0x12)
#define MIC1G_MICBIAS_CTRL		  (0x0B)
#define PA_ANTI_POP_REG_CTRL	  (0x0E)

/*
*	apb0 base
*	0x00 HP_VOLC
*/
#define PA_CLK_GC		(7)
#define HPVOL			(0)

/*
*	apb0 base
*	0x01 LOMIXSC
*/
#define LMIXMUTE				  (0)
/*
*	apb0 base
*	0x02 ROMIXSC
*/
#define RMIXMUTE				  (0)
/*
*	apb0 base
*	0x03 DAC_PA_SRC
*/
#define RMIXEN			(5)
#define LMIXEN			(4)
/*
*	apb0 base
*	0x07 PAEN_HP_CTRL
*/
#define HPPAEN			 (7)
#define HPCOM_FC		 (5)
#define PA_ANTI_POP_CTRL (2)

/*
*	apb0 base
*	0x0B MIC1G_MICBIAS_CTRL
*/
#define HMICBIASEN		 (7)
#ifdef CONFIG_ARCH_SUN8IW5
#define HMICBIAS_MODE	 (5)
#else/*CONFIG_ARCH_SUN8IW8*/
#define HMICADCEN	 (5)
#endif
/*0xE*/
#define PA_ANTI_POP_EN		(0)
/*
*	apb0 base
*	0x12 ADDA_APT2
*/
#define PA_SLOPE_SELECT	  (3)


/*
*SUNXI_HMIC_ENABLE
*HMIC DIG Enable
*CONFIG_ARCH_SUN8IW5:0x1c4
*/
#define HMIC_DIG_EN		  (0)

/*
*	SUNXI_HMIC_CTL
*HMIC Control Register
*CONFIG_ARCH_SUN8IW5:0x1c8
*/
#define HMIC_M					  (28)
#define HMIC_N					  (24)
#define HMIC_DIRQ				  (23)
#define HMIC_TH1_HYS			  (21)
#define	HMIC_EARPHONE_OUT_IRQ_EN  (20)
#define HMIC_EARPHONE_IN_IRQ_EN	  (19)
#define HMIC_KEY_UP_IRQ_EN		  (18)
#define HMIC_KEY_DOWN_IRQ_EN	  (17)
#define HMIC_DATA_IRQ_EN		  (16)
#define HMIC_DS_SAMP			  (14)
#define HMIC_TH2_HYS			  (13)
#define HMIC_TH2_KEY		      (8)
#define HMIC_SF_SMOOTH_FIL		  (6)
#define KEY_UP_IRQ_PEND			  (5)
#define HMIC_TH1_EARPHONE		  (0)

/*
*	SUNXI_HMIC_DATA
*HMIC Data Register
*
*CONFIG_ARCH_SUN8IW5:0x1cc
*/
#define HMIC_EARPHONE_OUT_IRQ_PEND  (20)
#define HMIC_EARPHONE_IN_IRQ_PEND   (19)
#define HMIC_KEY_UP_IRQ_PEND 	    (18)
#define HMIC_KEY_DOWN_IRQ_PEND 		(17)
#define HMIC_DATA_IRQ_PEND			(16)
#define HMIC_ADC_DATA				(0)

//#define FUNCTION_NAME "h2w"

#define hmic_rdreg(reg)	    readl((VIR_HMIC_BASSADDRESS+(reg)))
#define hmic_wrreg(reg,val)  writel((val),(VIR_HMIC_BASSADDRESS+(reg)))


static unsigned int read_prcm_wvalue(unsigned int addr)
{
  unsigned int reg;
	reg = readl(ADDA_PR_CFG_REG);
	reg |= (0x1<<28);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG);
	reg &= ~(0x1<<24);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG);
	reg &= ~(0x1f<<16);
	reg |= (addr<<16);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG); 
	reg &= (0xff<<0);

	return reg;
}

static void write_prcm_wvalue(unsigned int addr, unsigned int val)
{
  unsigned int reg;
	reg = readl(ADDA_PR_CFG_REG);
	reg |= (0x1<<28);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG);
	reg &= ~(0x1f<<16);
	reg |= (addr<<16);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG);
	reg &= ~(0xff<<8);
	reg |= (val<<8);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG);
	reg |= (0x1<<24);
	writel(reg, ADDA_PR_CFG_REG);

	reg = readl(ADDA_PR_CFG_REG);
	reg &= ~(0x1<<24);
	writel(reg, ADDA_PR_CFG_REG);
}

/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
static int hmic_wrreg_prcm_bits(unsigned short reg, unsigned int mask, unsigned int value)
{
	unsigned int old, new;

	old	=	read_prcm_wvalue(reg);
	new	=	(old & ~mask) | value;
	write_prcm_wvalue(reg,new);

	return 0;
}

static int hmic_wr_prcm_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	hmic_wrreg_prcm_bits(reg, mask, reg_val);
	return 0;
}


static int hmic_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	unsigned int old, new;

	old	=	hmic_rdreg(reg);
	new	=	(old & ~mask) | value;

	hmic_wrreg(reg,new);

	return 0;
}

static int hmic_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	hmic_wrreg_bits(reg, mask, reg_val);
	return 0;
}


#endif
