/*
 * sound\soc\sunxi\audiocodec\sun8iw3-sndcodec.h
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
#ifndef _SUN8IW3_CODEC_H
#define _SUN8IW3_CODEC_H
#include <mach/platform.h>
/*Codec Register*/
#define CODEC_BASSADDRESS         (0x01c22c00)
#define SUNXI_DAC_DPC                (0x00)
#define SUNXI_DAC_FIFOC              (0x04)
#define SUNXI_DAC_FIFOS              (0x08)
#define SUNXI_DAC_TXDATA             (0x0c)
#define SUNXI_ADC_FIFOC              (0x10)
#define SUNXI_ADC_FIFOS              (0x14)
#define SUNXI_ADC_RXDATA			 (0x18)
#define SUNXI_DAC_DEBUG              (0x48)

/*DAC Digital Part Control Register 
* codecbase+0x00
*/
#define DAC_EN                    (31)
#define DIGITAL_VOL               (12)

/*DAC FIFO Control Register 
* codecbase+0x04
*/
#define FIR_VERSION				  (28)
#define LAST_SE                   (26)
#define TX_FIFO_MODE              (24)
#define DRA_LEVEL                 (21)
#define TX_TRI_LEVEL              (8)
#define DAC_MODE                  (6)
#define TASR                      (5)
#define DAC_DRQ                   (4)
#define DAC_FIFO_FLUSH            (0)

/*ADC FIFO Control Register
* codecbase+0x10
*/
#define ADC_EN                	  (28)
#define ADC_DIG_MIC_EN			  (27)
#define RX_FIFO_MODE              (24)
#define ADCFDT					  (17)
#define ADCDFEN					  (16)
#define RX_TRI_LEVEL              (8)
#define ADC_MODE                  (7)
#define RASR                      (6)
#define ADC_DRQ                   (4)
#define ADC_FIFO_FLUSH            (0)

/*DAC Debug Register
* codecbase+0x48
*/
#define DAC_MODU_SELECT			  (11)
#define DAC_PATTERN_SELECT		  (9)
#define DAC_SWP					  (6)

#define ADDA_PR_CFG_REG     	  (SUNXI_R_PRCM_VBASE+0x1c0)
#define HP_VOLC					  (0x00)
#define LOMIXSC					  (0x01)
#define ROMIXSC					  (0x02)
#define DAC_PA_SRC				  (0x03)
#define PHONEIN_GCTRL			  (0x04)
#define LINEIN_GCTRL			  (0x05)
#define MICIN_GCTRL				  (0x06)
#define PAEN_HP_CTRL			  (0x07)
#define PHONEOUT_CTRL			  (0x08)
#define LINEOUT_VOLC			  (0x09)
#define MIC2G_LINEEN_CTRL		  (0x0A)
#define MIC1G_MICBIAS_CTRL		  (0x0B)
#define LADCMIXSC		  		  (0x0C)
#define RADCMIXSC				  (0x0D)
#define ADC_AP_EN				  (0x0F)
#define ADDA_APT0				  (0x10)
#define ADDA_APT1				  (0x11)
#define ADDA_APT2				  (0x12)
#define BIAS_DA16_CAL_CTRL		  (0x13)

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
#define LMIXMUTEDACR			  (0)
#define LMIXMUTEDACL			  (1)
#define LMIXMUTELINEINL			  (2)
#define LMIXMUTEPHONEN			  (3)
#define LMIXMUTEPHONEPN			  (4)
#define LMIXMUTEMIC2BOOST		  (5)
#define LMIXMUTEMIC1BOOST		  (6)

/*
*	apb0 base
*	0x02 ROMIXSC
*/
#define RMIXMUTE				  (0)
#define RMIXMUTEDACL			  (0)
#define RMIXMUTEDACR			  (1)
#define RMIXMUTELINEINR			  (2)
#define RMIXMUTEPHONEP			  (3)
#define RMIXMUTEPHONEPN			  (4)
#define RMIXMUTEMIC2BOOST		  (5)
#define RMIXMUTEMIC1BOOST		  (6)

/*
*	apb0 base
*	0x03 DAC_PA_SRC
*/
#define DACAREN			(7)
#define DACALEN			(6)
#define RMIXEN			(5)
#define LMIXEN			(4)
#define RHPPAMUTE		(3)
#define LHPPAMUTE		(2)
#define RHPIS			(1)
#define LHPIS			(0)

/*
*	apb0 base
*	0x04 PHONEIN_GCTRL
*/
#define PHONEPG			(4)
#define PHONENG			(0)

/*
*	apb0 base
*	0x05 LINEIN_GCTRL
*/
#define LINEING			(4)
#define PHONEG			(0)

/*
*	apb0 base
*	0x06 MICIN_GCTRL
*/
#define MIC1G			(4)
#define MIC2G			(0)

/*
*	apb0 base
*	0x07 PAEN_HP_CTRL
*/
#define HPPAEN			 (7)
#define HPCOM_FC		 (5)
#define COMPTEN			 (4)
#define PA_ANTI_POP_CTRL (2)
#define LTRNMUTE		 (1)
#define RTLNMUTE		 (0)

/*
*	apb0 base
*	0x08 PHONEOUT_CTRL
*/
#define PHONEOUTG		 (5)
#define PHONEOUT_EN		 (4)
#define PHONEOUTS3		 (3)
#define PHONEOUTS2		 (2)
#define PHONEOUTS1	 	 (1)
#define PHONEOUTS0		 (0)

/*
*	apb0 base
*	0x09 LINEOUT_VOLC
*/
#define LINEOUTVOL		 (3)
#define PHONEPREG		 (0)

/*
*	apb0 base
*	0x0A MIC2G_LINEEN_CTRL
*/
#define MIC2AMPEN		 (7)
#define MIC2BOOST		 (4)
#define LINEOUTL_EN		 (3)
#define LINEOUTR_EN		 (2)
#define LINEOUTL_SS		 (1)
#define LINEOUTR_SS		 (0)

/*
*	apb0 base
*	0x0B MIC1G_MICBIAS_CTRL
*/
#define HMICBIASEN		 (7)
#define MMICBIASEN		 (6)
#define HMICBIAS_MODE	 (5)
#define MIC2_SS			 (4)
#define MIC1AMPEN		 (3)
#define MIC1BOOST		 (0)

/*
*	apb0 base
*	0x0C LADCMIXSC
*/
#define LADCMIXMUTE		 		  (0)
#define LADCMIXMUTEMIC1BOOST	  (6)
#define LADCMIXMUTEMIC2BOOST	  (5)
#define LADCMIXMUTEPHONEPN		  (4)
#define LADCMIXMUTEPHONEN		  (3)
#define LADCMIXMUTELINEINL		  (2)
#define LADCMIXMUTELOUTPUT		  (1)
#define LADCMIXMUTEROUTPUT		  (0)

/*
*	apb0 base
*	0x0D RADCMIXSC
*/
#define RADCMIXMUTE		          (0)
#define RADCMIXMUTEMIC1BOOST	  (6)
#define RADCMIXMUTEMIC2BOOST	  (5)
#define RADCMIXMUTEPHONEPN		  (4)
#define RADCMIXMUTEPHONEP		  (3)
#define RADCMIXMUTELINEINR		  (2)
#define RADCMIXMUTEROUTPUT		  (1)
#define RADCMIXMUTELOUTPUT		  (0)

/*
*	apb0 base
*	0x0F ADC_AP_EN
*/
#define ADCREN			 (7)
#define ADCLEN			 (6)
#define ADCG			 (0)
/*
*	apb0 base
*	0x12 ADDA_APT2
*/
#define ZERO_CROSS_EN 	  (7)
#define PA_SLOPE_SELECT	  (3)



#define codec_rdreg(reg)	  readl((baseaddr+(reg)))
#define codec_wrreg(reg,val)  writel((val),(baseaddr+(reg)))

extern int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo);
extern int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol);
extern int snd_codec_put_volsw(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol);

/*
* Convenience kcontrol builders
*/
#define CODEC_SINGLE_VALUE(xreg, xshift, xmax,	xinvert)\
		((unsigned long)&(struct codec_mixer_control)\
		{.reg	=	xreg,	.shift	=	xshift,	.rshift	=	xshift,	.max	=	xmax,\
   	.invert	=	xinvert})

#define CODEC_SINGLE(xname,	reg,	shift,	max,	invert)\
{	.iface	= SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname,\
	.info	= snd_codec_info_volsw,	.get = snd_codec_get_volsw,\
	.put	= snd_codec_put_volsw,\
	.private_value	= CODEC_SINGLE_VALUE(reg, shift, max, invert)}

/*	mixer control*/	
struct	codec_mixer_control{
	int		min;
	int     max;
	int     where;
	unsigned int mask;
	unsigned int reg;
	unsigned int rreg;
	unsigned int shift;
	unsigned int rshift;
	unsigned int invert;
	unsigned int value;
};


//
#define    DAC_Digital_Part_Control 0x00
#define    DAC_FIFO_Control        0x04
#define    DAC_FIFO_Status         0x08
#define    DAC_TX_DATA             0x0c
#define    ADC_FIFO_Control        0x10
#define    ADC_FIFO_Status         0x14
#define    ADC_RX_DATA             0x18
#define    DAC_TX_Counter          0x40
#define    ADC_RX_Counter          0x44
#define    DAC_Debug               0x48
#define    ADC_Debug               0x4c

#define    Headphone_Volume_Control 			0x0  
#define    Left_Output_Mixer_Source_Control 		0x1  
#define    Right_Output_Mixer_Source_Control 		0x2  
#define    DAC_Analog_Enable_and_PA_Source_Control 	0x3  
#define    Phonein_Stereo_Gain_Control 			0x4  
#define    Linein_and_Phone_P_N_Gain_Control 		0x5  
#define    MIC1_and_MIC2_GAIN_Control 			0x6  
#define    PA_Enable_and_HP_Control 			0x7  
#define    Phoneout_Control 				0x8  
#define    Lineout_Volume_Control 			0x9  
#define    Mic2_Boost_and_Lineout_Enable_Control 	0xA  
#define    Mic1_Boost_and_MICBIAS_Control 		0xB  
#define    Left_ADC_Mixer_Source_Control 		0xC  
#define    Right_ADC_Mixer_Source_Control 		0xD  

#define    PA_UPTIME_CTRL                                   0xE  
#define    ADC_ANALIG_PART_ENABLE_REG                    0xF  


struct label {
    const char *name;
    int value;
};

#define LABEL(constant) { #constant, constant }
#define LABEL_END { NULL, -1 }
static struct label reg_labels[]={
        LABEL(DAC_Digital_Part_Control),
        LABEL(DAC_FIFO_Control),
        LABEL(DAC_FIFO_Status),
        LABEL(DAC_TX_DATA),
        LABEL(ADC_FIFO_Control),
        LABEL(ADC_FIFO_Status),
        LABEL(ADC_RX_DATA),
        LABEL(DAC_TX_Counter),
        LABEL(ADC_RX_Counter),
        LABEL(DAC_Debug),
        LABEL(ADC_Debug),
       
        LABEL(Headphone_Volume_Control), 		  
        LABEL(Left_Output_Mixer_Source_Control), 
        LABEL(Right_Output_Mixer_Source_Control), 
        LABEL(DAC_Analog_Enable_and_PA_Source_Control), 
        LABEL(Phonein_Stereo_Gain_Control), 
        LABEL(Linein_and_Phone_P_N_Gain_Control), 
        LABEL(MIC1_and_MIC2_GAIN_Control),   
        LABEL(PA_Enable_and_HP_Control),  
        LABEL(Phoneout_Control), 
        LABEL(Lineout_Volume_Control), 
        LABEL(Mic2_Boost_and_Lineout_Enable_Control),  
        LABEL(Mic1_Boost_and_MICBIAS_Control), 
        LABEL(Left_ADC_Mixer_Source_Control), 
        LABEL(Right_ADC_Mixer_Source_Control),  
		LABEL(PA_UPTIME_CTRL),
		LABEL(ADC_ANALIG_PART_ENABLE_REG),
	LABEL_END,
};

#endif
