/*
 * sound\soc\sunxi\i2s0\sunxi-i2s0.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef SUNXI_I2S0_H_
#define SUNXI_I2S0_H_

/*------------------------------------------------------------*/
/* REGISTER definition */
#ifdef CONFIG_ARCH_SUN9I
#define SUNXI_I2S0BASE 							(0x06002000)
#else
/* I2S0 REGISTER */
#define SUNXI_I2S0BASE 							(0x01C22000)
#endif
#define SUNXI_I2S0CTL 	  						(0x00)
	#define SUNXI_I2S0CTL_SDO3EN					(1<<11)
	#define SUNXI_I2S0CTL_SDO2EN					(1<<10)
	#define SUNXI_I2S0CTL_SDO1EN					(1<<9)
	#define SUNXI_I2S0CTL_SDO0EN					(1<<8) 
	#define SUNXI_I2S0CTL_ASS					(1<<6) 
	#define SUNXI_I2S0CTL_MS						(1<<5)
	#define SUNXI_I2S0CTL_PCM					(1<<4)
	#define SUNXI_I2S0CTL_LOOP					(1<<3)
	#define SUNXI_I2S0CTL_TXEN					(1<<2)
	#define SUNXI_I2S0CTL_RXEN					(1<<1)
	#define SUNXI_I2S0CTL_GEN					(1<<0)
	                                			
#define SUNXI_I2S0FAT0 							(0x04)
	#define SUNXI_I2S0FAT0_LRCP					(1<<7)
	#define SUNXI_I2S0FAT0_BCP					(1<<6)
	#define SUNXI_I2S0FAT0_SR_RVD				(3<<4)
	#define SUNXI_I2S0FAT0_SR_16BIT				(0<<4)
	#define	SUNXI_I2S0FAT0_SR_20BIT				(1<<4)
	#define SUNXI_I2S0FAT0_SR_24BIT				(2<<4)
	#define SUNXI_I2S0FAT0_WSS_16BCLK			(0<<2)
	#define SUNXI_I2S0FAT0_WSS_20BCLK			(1<<2)
	#define SUNXI_I2S0FAT0_WSS_24BCLK			(2<<2)
	#define SUNXI_I2S0FAT0_WSS_32BCLK			(3<<2)
	#define SUNXI_I2S0FAT0_FMT_I2S0				(0<<0)
	#define SUNXI_I2S0FAT0_FMT_LFT				(1<<0)
	#define SUNXI_I2S0FAT0_FMT_RGT				(2<<0)
	#define SUNXI_I2S0FAT0_FMT_RVD				(3<<0)
	
#define SUNXI_I2S0FAT1							(0x08)
	#define SUNXI_I2S0FAT1_SYNCLEN_16BCLK		(0<<12)
	#define SUNXI_I2S0FAT1_SYNCLEN_32BCLK		(1<<12)
	#define SUNXI_I2S0FAT1_SYNCLEN_64BCLK		(2<<12)
	#define SUNXI_I2S0FAT1_SYNCLEN_128BCLK		(3<<12)
	#define SUNXI_I2S0FAT1_SYNCLEN_256BCLK		(4<<12)
	#define SUNXI_I2S0FAT1_SYNCOUTEN				(1<<11)
	#define SUNXI_I2S0FAT1_OUTMUTE 				(1<<10)
	#define SUNXI_I2S0FAT1_MLS		 			(1<<9)
	#define SUNXI_I2S0FAT1_SEXT		 			(1<<8)
	#define SUNXI_I2S0FAT1_SI_1ST				(0<<6)
	#define SUNXI_I2S0FAT1_SI_2ND			 	(1<<6)
	#define SUNXI_I2S0FAT1_SI_3RD			 	(2<<6)
	#define SUNXI_I2S0FAT1_SI_4TH			 	(3<<6)
	#define SUNXI_I2S0FAT1_SW			 		(1<<5)
	#define SUNXI_I2S0FAT1_SSYNC	 				(1<<4)
	#define SUNXI_I2S0FAT1_RXPDM_16PCM			(0<<2)
	#define SUNXI_I2S0FAT1_RXPDM_8PCM			(1<<2)
	#define SUNXI_I2S0FAT1_RXPDM_8ULAW			(2<<2)
	#define SUNXI_I2S0FAT1_RXPDM_8ALAW  			(3<<2)
	#define SUNXI_I2S0FAT1_TXPDM_16PCM			(0<<0)
	#define SUNXI_I2S0FAT1_TXPDM_8PCM			(1<<0)
	#define SUNXI_I2S0FAT1_TXPDM_8ULAW			(2<<0)
	#define SUNXI_I2S0FAT1_TXPDM_8ALAW  			(3<<0)
	
#if defined(CONFIG_ARCH_SUN9IW1) \
	|| defined(CONFIG_ARCH_SUN8IW6)
#define SUNXI_I2S0TXFIFO 						(0x20)	
#else
#define SUNXI_I2S0TXFIFO 						(0x0C)
#endif
#define SUNXI_I2S0RXFIFO 						(0x10)

#define SUNXI_I2S0FCTL  							(0x14)
#if defined(CONFIG_ARCH_SUN9IW1) \
	|| defined(CONFIG_ARCH_SUN8IW6)
#define SUNXI_I2S0FCTL_HUBEN					(1<<31)
#else
	#define SUNXI_I2S0FCTL_FIFOSRC				(1<<31)
#endif
	#define SUNXI_I2S0FCTL_FTX					(1<<25)
	#define SUNXI_I2S0FCTL_FRX					(1<<24)
	#define SUNXI_I2S0FCTL_TXTL(v)				((v)<<12)
	#define SUNXI_I2S0FCTL_RXTL(v)  				((v)<<4)
	#define SUNXI_I2S0FCTL_TXIM_MOD0				(0<<2)
	#define SUNXI_I2S0FCTL_TXIM_MOD1				(1<<2)
	#define SUNXI_I2S0FCTL_RXOM_MOD0				(0<<0)
	#define SUNXI_I2S0FCTL_RXOM_MOD1				(1<<0)
	#define SUNXI_I2S0FCTL_RXOM_MOD2				(2<<0)
	#define SUNXI_I2S0FCTL_RXOM_MOD3				(3<<0)
	
#define SUNXI_I2S0FSTA   						(0x18)
	#define SUNXI_I2S0FSTA_TXE					(1<<28)
	#define SUNXI_I2S0FSTA_TXECNT(v)				((v)<<16)
	#define SUNXI_I2S0FSTA_RXA					(1<<8)
	#define SUNXI_I2S0FSTA_RXACNT(v)				((v)<<0)
	
#define SUNXI_I2S0INT    						(0x1C)
	#define SUNXI_I2S0INT_TXDRQEN				(1<<7)
	#define SUNXI_I2S0INT_TXUIEN					(1<<6)
	#define SUNXI_I2S0INT_TXOIEN					(1<<5)
	#define SUNXI_I2S0INT_TXEIEN					(1<<4)
	#define SUNXI_I2S0INT_RXDRQEN				(1<<3)
	#define SUNXI_I2S0INT_RXUIEN					(1<<2)
	#define SUNXI_I2S0INT_RXOIEN					(1<<1)
	#define SUNXI_I2S0INT_RXAIEN					(1<<0)
#if defined(CONFIG_ARCH_SUN9IW1) \
	|| defined(CONFIG_ARCH_SUN8IW6)
#define SUNXI_I2S0ISTA   						(0x0c)
#else
#define SUNXI_I2S0ISTA   						(0x20)
#endif
	#define SUNXI_I2S0ISTA_TXUISTA				(1<<6)
	#define SUNXI_I2S0ISTA_TXOISTA				(1<<5)
	#define SUNXI_I2S0ISTA_TXEISTA				(1<<4)
	#define SUNXI_I2S0ISTA_RXOISTA				(1<<1)
	#define SUNXI_I2S0ISTA_RXAISTA				(1<<0)
		
#define SUNXI_I2S0CLKD   						(0x24)
	#define SUNXI_I2S0CLKD_MCLKOEN				(1<<7)
	#define SUNXI_I2S0CLKD_BCLKDIV_2				(0<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_4				(1<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_6				(2<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_8				(3<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_12			(4<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_16			(5<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_32			(6<<4)
	#define SUNXI_I2S0CLKD_BCLKDIV_64			(7<<4)
	#define SUNXI_I2S0CLKD_MCLKDIV_1				(0<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_2				(1<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_4				(2<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_6				(3<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_8				(4<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_12			(5<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_16			(6<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_24			(7<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_32			(8<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_48			(9<<0)
	#define SUNXI_I2S0CLKD_MCLKDIV_64			(10<<0)
		
#define SUNXI_I2S0TXCNT  						(0x28)
                            					
#define SUNXI_I2S0RXCNT  						(0x2C)
                            					
#define SUNXI_TXCHSEL							(0x30)
	#define SUNXI_TXCHSEL_CHNUM(v)				(((v)-1)<<0)

#define SUNXI_TXCHMAP							(0x34)
	#define SUNXI_TXCHMAP_CH7(v)				(((v)-1)<<28)
	#define SUNXI_TXCHMAP_CH6(v)				(((v)-1)<<24)
	#define SUNXI_TXCHMAP_CH5(v)				(((v)-1)<<20)
	#define SUNXI_TXCHMAP_CH4(v)				(((v)-1)<<16)
	#define SUNXI_TXCHMAP_CH3(v)				(((v)-1)<<12)
	#define SUNXI_TXCHMAP_CH2(v)				(((v)-1)<<8)
	#define SUNXI_TXCHMAP_CH1(v)				(((v)-1)<<4)
	#define SUNXI_TXCHMAP_CH0(v)				(((v)-1)<<0)

#define SUNXI_RXCHSEL							(0x38)
	#define SUNXI_RXCHSEL_CHNUM(v)				(((v)-1)<<0)

#define SUNXI_RXCHMAP							(0x3C)
	#define SUNXI_RXCHMAP_CH3(v)				(((v)-1)<<12)
	#define SUNXI_RXCHMAP_CH2(v)				(((v)-1)<<8)
	#define SUNXI_RXCHMAP_CH1(v)				(((v)-1)<<4)
	#define SUNXI_RXCHMAP_CH0(v)				(((v)-1)<<0)	

#define SUNXI_I2S0CLKD_MCLK_MASK   0x0f
#define SUNXI_I2S0CLKD_MCLK_OFFS   0
#define SUNXI_I2S0CLKD_BCLK_MASK   0x070
#define SUNXI_I2S0CLKD_BCLK_OFFS   4
#define SUNXI_I2S0CLKD_MCLKEN_OFFS 7

#define SUNXI_I2S0_DIV_MCLK	(0)
#define SUNXI_I2S0_DIV_BCLK	(1)

struct sunxi_i2s0_info {
	void __iomem   *regs;

	u32 slave;
	u32 mono;
	u32 samp_fs;
	u32 samp_res;
	u32 samp_format;
	u32 ws_size;
	u32 mclk_rate;
	u32 lrc_pol;
	u32 bclk_pol;
	u32 pcm_txtype;
	u32 pcm_rxtype;	
	u32 pcm_sw;
	u32 pcm_sync_period;
	u32 pcm_sync_type;
	u32 pcm_start_slot;
	u32 pcm_lsb_first;
	u32 pcm_ch_num;
};

extern struct sunxi_i2s0_info sunxi_i2s0;
#endif
