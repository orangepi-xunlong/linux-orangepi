/*
 * sound\soc\sunxi\spdif\sunxi_spdif.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * chenpailin <chenpailin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/pinctrl/consumer.h>
#include "sunxi_spdma.h"
#include "sunxi_spdif.h"
#include <linux/regulator/consumer.h>

static int regsave[9];
static int spdif_used = 0;
static int vol_self_config;
#ifdef CONFIG_ARCH_SUN9IW1
static script_item_u 	spdif_voltage;
struct regulator *spdif_vol= NULL;
#endif
struct sunxi_spdif_info sunxi_spdif;
static struct pinctrl *spdif_pinctrl;
//static struct clk *spdif_apbclk 	= NULL;
#ifndef CONFIG_ARCH_SUN9IW1
static struct clk *spdif_pll2clk	= NULL;
static struct clk *spdif_pll		= NULL;
#else
static struct clk *spdif_pll3clk	= NULL;
#endif
static struct clk *spdif_moduleclk	= NULL;

static struct sunxi_dma_params sunxi_spdif_stereo_out = {
	.name		= "spdif_out",	
	.dma_addr 	=	SUNXI_SPDIFBASE + SUNXI_SPDIF_TXFIFO,	
};

static struct sunxi_dma_params sunxi_spdif_stereo_in = {
	.name		= "spdif_in",
	.dma_addr 	=	SUNXI_SPDIFBASE + SUNXI_SPDIF_RXFIFO,
};

void spdif_txctrl_enable(int tx_en, int chan, int hub_en)
{
	u32 reg_val;
	
	if (chan == 1) {
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		reg_val |= SUNXI_SPDIF_TXCFG_SINGLEMOD;
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
	}
	 
	/*soft reset SPDIF*/
	writel(0x1, sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	
	/*flush TX FIFO*/
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	reg_val |= SUNXI_SPDIF_FCTL_FTX;	
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	
	/*clear interrupt status*/
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_ISTA);
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_ISTA);
	
	/*clear TX counter*/
	writel(0, sunxi_spdif.regs + SUNXI_SPDIF_TXCNT);

	if (tx_en) {
		/*SPDIF TX ENBALE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		reg_val |= SUNXI_SPDIF_TXCFG_TXEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		
		/*DRQ ENABLE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_INT);
		reg_val |= SUNXI_SPDIF_INT_TXDRQEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_INT);
		
		/*global enable*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
		reg_val |= SUNXI_SPDIF_CTL_GEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);		
	} else {
		/*SPDIF TX DISABALE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		reg_val &= ~SUNXI_SPDIF_TXCFG_TXEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		
		/*DRQ DISABLE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_INT);
		reg_val &= ~SUNXI_SPDIF_INT_TXDRQEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_INT);

		/*global disable*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
		reg_val &= ~SUNXI_SPDIF_CTL_GEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	}
	if (hub_en) {
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
		reg_val |= SUNXI_SPDIFFCTL_HUBEN;
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	} else {
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
		reg_val &= ~SUNXI_SPDIFFCTL_HUBEN;
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	}
}
EXPORT_SYMBOL(spdif_txctrl_enable);

void spdif_rxctrl_enable(int rx_en)
{
	u32 reg_val;

	/*flush RX FIFO*/
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	reg_val |= SUNXI_SPDIF_FCTL_FRX;	
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	
	/*clear interrupt status*/
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_ISTA);
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_ISTA);
	
	/*clear RX counter*/
	writel(0, sunxi_spdif.regs + SUNXI_SPDIF_RXCNT);

	if (rx_en) {
		/*SPDIF RX ENBALE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCFG);
		reg_val |= SUNXI_SPDIF_RXCFG_RXEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCFG);
		
		/*DRQ ENABLE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_INT);
		reg_val |= SUNXI_SPDIF_INT_RXDRQEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_INT);
		
		/*global enable*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
		reg_val |= SUNXI_SPDIF_CTL_GEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);		
	} else {
		/*SPDIF TX DISABALE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCFG);
		reg_val &= ~SUNXI_SPDIF_RXCFG_RXEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCFG);
		
		/*DRQ DISABLE*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_INT);
		reg_val &= ~SUNXI_SPDIF_INT_RXDRQEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_INT);
		
		/*global disable*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
		reg_val &= ~SUNXI_SPDIF_CTL_GEN;	
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	}
}

int spdif_set_fmt(unsigned int fmt)
{
	u32 reg_val;
	reg_val = 0;
	reg_val &= ~SUNXI_SPDIF_TXCFG_SINGLEMOD;
	reg_val |= SUNXI_SPDIF_TXCFG_ASS;
	reg_val &= ~SUNXI_SPDIF_TXCFG_NONAUDIO;
	reg_val |= SUNXI_SPDIF_TXCFG_CHSTMODE;
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
	
	reg_val = 0;
#ifdef CONFIG_ARCH_SUN8IW1
	reg_val &= ~SUNXI_SPDIF_FCTL_FIFOSRC;
#endif
	reg_val |= SUNXI_SPDIF_FCTL_TXTL(16);
	reg_val |= SUNXI_SPDIF_FCTL_RXTL(15);
	reg_val |= SUNXI_SPDIF_FCTL_TXIM(1);
	reg_val |= SUNXI_SPDIF_FCTL_RXOM(3);
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	
	if (!fmt) {/*PCM*/
		reg_val = 0;
		reg_val |= (SUNXI_SPDIF_TXCHSTA0_CHNUM(2));
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
		
		reg_val = 0;
		reg_val |= (SUNXI_SPDIF_TXCHSTA1_SAMWORDLEN(1));
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
	} else {  /*non PCM*/
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		reg_val |= SUNXI_SPDIF_TXCFG_NONAUDIO;
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
		
		reg_val = 0;
		reg_val |= (SUNXI_SPDIF_TXCHSTA0_CHNUM(2));
		reg_val |= SUNXI_SPDIF_TXCHSTA0_AUDIO;
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
		
		reg_val = 0;
		reg_val |= (SUNXI_SPDIF_TXCHSTA1_SAMWORDLEN(1));
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
	}
	return 0;
}
EXPORT_SYMBOL(spdif_set_fmt);

int spdif_hw_params(int format)
{
	u32 reg_val;
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
	reg_val &= ~SUNXI_SPDIF_TXCFG_FMTRVD;
	if(format == 16)
		reg_val |= SUNXI_SPDIF_TXCFG_FMT16BIT;
	else if(format == 20)
		reg_val |= SUNXI_SPDIF_TXCFG_FMT20BIT;
	else
		reg_val |= SUNXI_SPDIF_TXCFG_FMT24BIT;
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);

	if (format == 24) {
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
		reg_val &= ~(0x1<<2);
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	} else {
		reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
		reg_val |= (0x1<<2);
		writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	}
	
	return 0;
}
EXPORT_SYMBOL(spdif_hw_params);

int spdif_set_clkdiv(int div_id, int div)
{
	u32 reg_val = 0;

	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
	reg_val &= ~(SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0xf));
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);

	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
	reg_val &= ~(SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0xf));
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
		
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
	reg_val &= ~(SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0xf));
  	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
	reg_val &= ~(SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0xf));
  	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);

	switch(div_id) {
		case SUNXI_DIV_MCLK:
		{
			reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
			reg_val &= ~(SUNXI_SPDIF_TXCFG_TXRATIO(0x1F));	
			reg_val |= SUNXI_SPDIF_TXCFG_TXRATIO(div-1);
			writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
			#ifndef CONFIG_ARCH_SUN9IW1
			if(clk_get_rate(spdif_pll2clk) == 24576000)
			#else
			if(clk_get_rate(spdif_pll3clk) == 24576000)
			#endif
			{
				switch(div)
				{
					/*24KHZ*/
					case 8:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0x6));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0x9));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0x6));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0x9));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);

						break;
						
					/*32KHZ*/
					case 6:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0x3));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0xC));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0x3));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0xC));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
						
					/*48KHZ*/
					case 4:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0x2));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0xD));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);	

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0x2));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0xD));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						break;
						
					/*96KHZ*/
					case 2:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0xA));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0x5));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0xA));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0x5));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
						
					/*192KHZ*/
					case 1:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0xE));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
					
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);	
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0x1));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0xE));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
					
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);	
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0x1));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
						
					default:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(1));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(1));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
				}
			}else{  /*22.5792MHz*/		
				switch(div)
				{
					/*22.05khz*/
					case 8:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0x4));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0xb));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);	

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0x4));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0xb));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);							
						break;
						
					/*44.1KHZ*/
					case 4:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0x0));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0xF));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);	

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0x0));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0xF));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);							
						break;
						
					/*88.2khz*/
					case 2:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0x8));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
					
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0x7));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0x8));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
					
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0x7));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
			
					/*176.4KHZ*/
					case 1:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(0xC));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
					
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0x3));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(0xC));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
					
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0x3));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
						
					default:
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
						reg_val |= (SUNXI_SPDIF_TXCHSTA0_SAMFREQ(1));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
						reg_val |= (SUNXI_SPDIF_TXCHSTA1_ORISAMFREQ(0));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);

						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
						reg_val |= (SUNXI_SPDIF_RXCHSTA0_SAMFREQ(1));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
				
						reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
						reg_val |= (SUNXI_SPDIF_RXCHSTA1_ORISAMFREQ(0));
						writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);						
						break;
				}
			}			
		}
		break;
		case SUNXI_DIV_BCLK:
		break;
			
		default:
			return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(spdif_set_clkdiv);

static int sunxi_spdif_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	spdif_set_fmt(fmt);
	return 0;
}

static int sunxi_spdif_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;
	int format;
	switch (params_format(params))
	{
		case SNDRV_PCM_FORMAT_S16_LE:
		format = 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		format = 20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		format = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		format = 24;
		break;
	default:
		return -EINVAL;
	}
	spdif_hw_params(format);
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sunxi_spdif_stereo_out;
	else
		dma_data = &sunxi_spdif_stereo_in;
		
	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	
	return 0;
}						

static int sunxi_spdif_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				spdif_rxctrl_enable(1);
			} else {
				spdif_txctrl_enable(1,substream->runtime->channels, 0);
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:			
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				spdif_rxctrl_enable(0);
			} else {
			  spdif_txctrl_enable(0, substream->runtime->channels, 0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

		return ret;
}					

/*freq:   1: 22.5792MHz   0: 24.576MHz  */
static int sunxi_spdif_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int dir)
{
	if (!freq) {
		#ifndef CONFIG_ARCH_SUN9IW1
		//#ifdef CONFIG_ARCH_SUN8IW7||CONFIG_ARCH_SUN8IW6
		#if defined(CONFIG_ARCH_SUN8IW7) || defined(CONFIG_ARCH_SUN8IW6)
			if (clk_set_rate(spdif_pll, 24576000)) {
				pr_err("try to set the spdif_pll rate failed!\n");
			}
		#else
			if (clk_set_rate(spdif_pll2clk, 24576000)) {
				pr_err("try to set the spdif_pll2clk rate failed!\n");
			}
		#endif
		#else
		if (clk_set_rate(spdif_pll3clk, 24576000)) {
			pr_err("try to set the spdif_pll2clk rate failed!\n");
		}
		#endif
	} else {
		#ifndef CONFIG_ARCH_SUN9IW1
		//#ifdef CONFIG_ARCH_SUN8IW7||CONFIG_ARCH_SUN8IW6
		#if defined(CONFIG_ARCH_SUN8IW7) || defined(CONFIG_ARCH_SUN8IW6)
			if (clk_set_rate(spdif_pll, 22579200)) {
				pr_err("try to set the spdif_pll rate failed!\n");
			}
		#else
			if (clk_set_rate(spdif_pll2clk, 22579200)) {
				pr_err("try to set the spdif_pll2clk rate failed!\n");
			}
		#endif
		#else
		if (clk_set_rate(spdif_pll3clk, 22579200)) {
			pr_err("try to set the spdif_pll3clk rate failed!\n");
		}
		#endif
	}

	return 0;
}

static int sunxi_spdif_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	spdif_set_clkdiv(div_id, div);
	return 0;
}

static int sunxi_spdif_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}
static int sunxi_spdif_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void spdifregsave(void)
{
	regsave[0] = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	regsave[1] = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
	regsave[2] = readl(sunxi_spdif.regs + SUNXI_SPDIF_FCTL) | (0x3<<16);
	regsave[3] = readl(sunxi_spdif.regs + SUNXI_SPDIF_INT);
	regsave[4] = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
	regsave[5] = readl(sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
	regsave[6] = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCFG);
	regsave[7] = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
	regsave[8] = readl(sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);	
}

static void spdifregrestore(void)
{
	writel(regsave[0], sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	writel(regsave[1], sunxi_spdif.regs + SUNXI_SPDIF_TXCFG);
	writel(regsave[2], sunxi_spdif.regs + SUNXI_SPDIF_FCTL);
	writel(regsave[3], sunxi_spdif.regs + SUNXI_SPDIF_INT);
	writel(regsave[4], sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA0);
	writel(regsave[5], sunxi_spdif.regs + SUNXI_SPDIF_TXCHSTA1);
	writel(regsave[6], sunxi_spdif.regs + SUNXI_SPDIF_RXCFG);
	writel(regsave[7], sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA0);
	writel(regsave[8], sunxi_spdif.regs + SUNXI_SPDIF_RXCHSTA1);
}

static int sunxi_spdif_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[SPDIF]Enter %s\n", __func__);
	
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	reg_val &= ~SUNXI_SPDIF_CTL_GEN;
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);

	spdifregsave();
	if ((NULL == spdif_moduleclk) ||(IS_ERR(spdif_moduleclk))) {
		pr_err("spdif_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*disable the module clock*/
		clk_disable(spdif_moduleclk);
	}
	#ifdef CONFIG_ARCH_SUN8IW6
	if ((NULL == spdif_pll) ||(IS_ERR(spdif_pll))) {
		pr_err("spdif_pll handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*disable the module clock*/
		clk_disable(spdif_pll);
	}
	#endif
	if (vol_self_config == 1){
		#ifdef CONFIG_ARCH_SUN9IW1
		if (spdif_vol) {
			regulator_disable(spdif_vol);
			regulator_put(spdif_vol);
			spdif_vol = NULL;
		} else {
			pr_err("regulator_disable failed: spdif_vol is null!!\n");
		}
		#endif
	}
	devm_pinctrl_put(spdif_pinctrl);
	#ifdef CONFIG_ARCH_SUN8IW6
	/*config owa gpio as input*/
	reg_val = readl((void __iomem *)0xf1c20898);
	reg_val |= 0x7<<8;
	writel(reg_val, (void __iomem *)0xf1c20898);
	#endif

	return 0;
}

static int sunxi_spdif_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	pr_debug("[SPDIF]Enter %s\n", __func__);
	if (vol_self_config == 1){
		#ifdef CONFIG_ARCH_SUN9IW1
		/*spdf:dcdc1*/
		spdif_vol = regulator_get(NULL, spdif_voltage.str);
		if (!spdif_vol) {
			pr_err("get audio spdif_vol failed\n");
			return -EFAULT;
		}
		regulator_enable(spdif_vol);
		#endif
	}
	#ifdef CONFIG_ARCH_SUN8IW6
	if (clk_prepare_enable(spdif_pll)) {
		pr_err("try to enable spdif_pll output failed!\n");
	}
	#endif
	/*enable the module clock*/
	if (clk_prepare_enable(spdif_moduleclk)) {
		pr_err("try to enable spdif_moduleclk output failed!\n");
	}
	spdif_pinctrl = devm_pinctrl_get_select_default(cpu_dai->dev);
	if (IS_ERR_OR_NULL(spdif_pinctrl)) {
		dev_warn(cpu_dai->dev,
			"pins are not configured from the driver\n");
	}
	spdifregrestore();
	
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	reg_val |= SUNXI_SPDIF_CTL_GEN;
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	
	return 0;
}

#define SUNXI_SPDIF_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sunxi_spdif_dai_ops = {
	.trigger 	= sunxi_spdif_trigger,
	.hw_params 	= sunxi_spdif_hw_params,
	.set_fmt 	= sunxi_spdif_set_fmt,
	.set_clkdiv = sunxi_spdif_set_clkdiv,
	.set_sysclk = sunxi_spdif_set_sysclk, 
};
static struct snd_soc_dai_driver sunxi_spdif_dai = {
	.probe 		= sunxi_spdif_dai_probe,
	.suspend 	= sunxi_spdif_suspend,
	.resume 	= sunxi_spdif_resume,
	.remove 	= sunxi_spdif_dai_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 4,
		.rates = SUNXI_SPDIF_RATES,
	.formats = SNDRV_PCM_FMTBIT_S16_LE|SNDRV_PCM_FMTBIT_S20_3LE| SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,},
	.capture = {
		.channels_min = 1,
		.channels_max = 4,
		.rates = SUNXI_SPDIF_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE|SNDRV_PCM_FMTBIT_S20_3LE| SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE,},
	.ops = &sunxi_spdif_dai_ops,
};		

static int __init sunxi_spdif_dev_probe(struct platform_device *pdev)
{
	int reg_val = 0;
	int ret = 0;
	if (vol_self_config == 1){
		#ifdef CONFIG_ARCH_SUN9IW1
		if(script_get_item("spdif0", "spdif_voltage", &spdif_voltage) != SCIRPT_ITEM_VALUE_TYPE_STR){
			pr_err("[aif3_voltage]script_get_item return type err\n");
			return -EFAULT;
		}
		/*spdif:dcdc1*/
		spdif_vol = regulator_get(NULL, spdif_voltage.str);
		if (!spdif_vol) {
			pr_err("get audio spdif_vol failed\n");
			return -EFAULT;
		}
		regulator_enable(spdif_vol);
		#endif
	}

	sunxi_spdif.regs = ioremap(SUNXI_SPDIFBASE, 0x100);
	if (sunxi_spdif.regs == NULL) {
		return -ENXIO;
	}
	pr_debug("%s, line:%d, dev_name(&pdev->dev):%s\n", __func__, __LINE__, dev_name(&pdev->dev));
	spdif_pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR_OR_NULL(spdif_pinctrl)) {
		dev_warn(&pdev->dev,
			"pins are not configured from the driver\n");
	}

#if defined(CONFIG_ARCH_SUN8IW6) || defined(CONFIG_ARCH_SUN8IW7)
	spdif_pll = clk_get(NULL, "pll_audio");
    if ((!spdif_pll)||(IS_ERR(spdif_pll))) {
		pr_err("try to get spdif_pll failed\n");
	}
	if (clk_prepare_enable(spdif_pll)) {
		pr_err("enable spdif_pll2clk failed; \n");
	}
#else
    #ifdef CONFIG_ARCH_SUN9IW1
	/*spdif pll3clk*/
	spdif_pll3clk = clk_get(NULL, "pll3");
	if ((!spdif_pll3clk)||(IS_ERR(spdif_pll3clk))) {
		pr_err("try to get spdif_pll3clk failed\n");
	}
	if (clk_prepare_enable(spdif_pll3clk)) {
		pr_err("enable spdif_pll3clk failed; \n");
	}
    #else
	spdif_pll = clk_get(NULL, "pll2x8");
	if ((!spdif_pll)||(IS_ERR(spdif_pll))) {
		pr_err("try to get spdif_pll failed\n");
	}
	if (clk_prepare_enable(spdif_pll)) {
		pr_err("enable spdif_pll2clk failed; \n");
	}
	#endif
#endif
#ifdef CONFIG_ARCH_SUN8IW1
/********spdif pll2clk can be remove****************/
	/*spdif pll2clk*/
	spdif_pll2clk = clk_get(NULL, "pll2");
	if ((!spdif_pll2clk)||(IS_ERR(spdif_pll2clk))) {
		pr_err("try to get spdif_pll2clk failed\n");
	}
	if (clk_prepare_enable(spdif_pll2clk)) {
		pr_err("enable spdif_pll2clk failed; \n");
	}
#endif
#ifdef CONFIG_ARCH_SUN8IW7
	/*spdif module clk*/
	spdif_moduleclk = clk_get(NULL, "owa");
	if ((!spdif_moduleclk)||(IS_ERR(spdif_moduleclk))) {
		pr_err("try to get spdif_moduleclk failed\n");
	}
#else
	/*spdif module clk*/
	spdif_moduleclk = clk_get(NULL, "spdif");
	if ((!spdif_moduleclk)||(IS_ERR(spdif_moduleclk))) {
		pr_err("try to get spdif_moduleclk failed\n");
	}
#endif
#ifdef CONFIG_ARCH_SUN8IW1
/*******clk_set_parent can be remove*******/
	if (clk_set_parent(spdif_moduleclk, spdif_pll2clk)) {
		pr_err("try to set parent of spdif_moduleclk to spdif_pll2ck failed! line = %d\n",__LINE__);
	}
	if (clk_set_rate(spdif_moduleclk, 24576000/8)) {
		pr_err("set spdif_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
#elif defined CONFIG_ARCH_SUN8IW7
	if (clk_set_parent(spdif_moduleclk, spdif_pll)) {
		pr_err("try to set parent of spdif_moduleclk to spdif_pll failed! line = %d\n",__LINE__);
	}
	if (clk_set_rate(spdif_moduleclk, 24576000)) {
		pr_err("set spdif_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
#else
#ifdef CONFIG_ARCH_SUN8IW6
	if (clk_set_parent(spdif_moduleclk, spdif_pll)) {
		pr_err("try to set parent of spdif_moduleclk to spdif_pll2ck failed! line = %d\n",__LINE__);
	}
#else
	if (clk_set_parent(spdif_moduleclk, spdif_pll3clk)) {/*sun9i*/
		pr_err("try to set parent of spdif_moduleclk to spdif_pll3ck failed! line = %d\n",__LINE__);
	}
#endif
	if (clk_set_rate(spdif_moduleclk, 24576000)) {
		pr_err("set spdif_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
#endif
	if (clk_prepare_enable(spdif_moduleclk)) {
		pr_err("open spdif_moduleclk failed! line = %d\n", __LINE__);
	}
//	if (clk_reset(spdif_moduleclk, AW_CCU_CLK_NRESET)) {
//		pr_err("try to NRESET spdif module clk failed!\n");
//	}

	/*global enbale*/
	reg_val = readl(sunxi_spdif.regs + SUNXI_SPDIF_CTL);
	reg_val |= SUNXI_SPDIF_CTL_GEN;
	writel(reg_val, sunxi_spdif.regs + SUNXI_SPDIF_CTL);

	ret = snd_soc_register_dai(&pdev->dev, &sunxi_spdif_dai);
	return 0;
}

static int __exit sunxi_spdif_dev_remove(struct platform_device *pdev)
{
	if (spdif_used) {
		spdif_used = 0;
		if ((NULL == spdif_moduleclk) ||(IS_ERR(spdif_moduleclk))) {
			pr_err("spdif_moduleclk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release the module clock*/
			clk_disable(spdif_moduleclk);
		}
#ifndef		CONFIG_ARCH_SUN9IW1
		if ((NULL == spdif_pll) ||(IS_ERR(spdif_pll))) {
			pr_err("spdif_pll handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pllx8clk*/
			clk_put(spdif_pll);
		}

		if ((NULL == spdif_pll2clk) ||(IS_ERR(spdif_pll2clk))) {
			pr_err("spdif_pll2clk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pll2clk*/
			clk_put(spdif_pll2clk);
		}
#else
		if ((NULL == spdif_pll3clk) ||(IS_ERR(spdif_pll3clk))) {
			pr_err("spdif_pll2clk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pll3clk*/
			clk_put(spdif_pll3clk);
		}
#endif
		devm_pinctrl_put(spdif_pinctrl);

		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	
	return 0;
}

static struct platform_device sunxi_spdif_device = {
	.name = "spdif0",
	.id = PLATFORM_DEVID_NONE,
};

static struct platform_driver sunxi_spdif_driver = {
	.probe = sunxi_spdif_dev_probe,
	.remove = __exit_p(sunxi_spdif_dev_remove),
	.driver = {
		.name = "spdif0",
		.owner = THIS_MODULE,
	},	
};

static int __init sunxi_spdif_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("spdif0", "spdif_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[SPDIF]:%s,line:%d type err!\n", __func__, __LINE__);
    }

	spdif_used = val.val;
	pr_debug("%s, line:%d, spdif_used:%d\n", __func__, __LINE__, spdif_used);
#ifdef CONFIG_ARCH_SUN9IW1
	if (script_get_item("spdif0", "spdif_vol_config", &val) != SCIRPT_ITEM_VALUE_TYPE_INT){
		pr_err("[spdif_vol_config]script_get_item return type err\n");
	}
	vol_self_config = val.val;
#endif
 	if (spdif_used) {
		if((platform_device_register(&sunxi_spdif_device))<0)
			return err;

		if ((err = platform_driver_register(&sunxi_spdif_driver)) < 0)
			return err;
	} else {
        pr_err("[SPDIF]sunxi-spdif cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }
 
	return 0;
}
module_init(sunxi_spdif_init);

static void __exit sunxi_spdif_exit(void)
{	
	platform_driver_unregister(&sunxi_spdif_driver);
}
module_exit(sunxi_spdif_exit);

/* Module information */
MODULE_AUTHOR("REUUIMLLA");
MODULE_DESCRIPTION("sunxi SPDIF SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-spdif");
