/*
 * sound\soc\sunxi\audiocodec\sun8iw1_sndcodec.h
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
#ifndef _SUN8IW1_SNDCODEC_H
#define _SUN8IW1_SNDCODEC_H

/*Codec Register*/
#define CODEC_BASSADDRESS         (0x01c22c00)
#define SUNXI_DAC_DPC                (0x00)
#define SUNXI_DAC_FIFOC              (0x04)
#define SUNXI_DAC_FIFOS              (0x08)
#define SUNXI_DAC_TXDATA             (0x0c)
#define SUNXI_ADC_FIFOC              (0x10)
#define SUNXI_ADC_FIFOS              (0x14)
#define SUNXI_ADC_RXDATA			 (0x18)
#define SUNXI_HMIC_CTRL				 (0x1c)
#define SUNXI_DAC_ACTL				 (0x20)
#define SUNXI_PA_CTRL				 (0x24)
#define SUNXI_MIC_CTRL				 (0x28)
#define SUNXI_ADC_ACTL				 (0x2c)
#define SUNXI_ADDAC_TUNE		     (0x30)
#define SUNXI_BIAS_CRT				 (0x34)
#define SUNXI_DAC_TXCNT              (0x40)
#define SUNXI_ADC_RXCNT              (0x44)
#define SUNXI_DAC_DEBUG              (0x48)
#define SUNXI_ADC_DEBUG              (0x4c)
#define SUNXI_HMIC_CTL 	             (0x50)
#define SUNXI_HMIC_DATA	             (0x54)
#define SUNXI_DAC_DAP_CTL			 (0x60)
#define SUNXI_DAC_DAP_VOL			 (0x64)
#define SUNXI_DAC_DAP_COF			 (0x68)
#define SUNXI_DAC_DAP_OPT			 (0x6c)
#define SUNXI_ADC_DAP_CTL 			 (0x70)
#define SUNXI_ADC_DAP_VOL 			 (0x74)
#define SUNXI_ADC_DAP_LCTL			 (0x78)
#define SUNXI_ADC_DAP_RCTL			 (0x7c)
#define SUNXI_ADC_DAP_PARA			 (0x80)
#define SUNXI_ADC_DAP_LAC			 (0x84)
#define SUNXI_ADC_DAP_LDAT			 (0x88)
#define SUNXI_ADC_DAP_RAC			 (0x8c)
#define SUNXI_ADC_DAP_RDAC			 (0x90)
#define SUNXI_ADC_DAP_HPFC			 (0x94)

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

/*DAC Output Mixer & DAC Analog Control Register
* codecbase+0x20
*/
#define VOLUME                    (0)
#define LHPPA_MUTE                (6)
#define RHPPA_MUTE                (7)
#define LHPIS					  (8)
#define RHPIS					  (9)
#define LMIXMUTE				  (10)
#define RMIXMUTE				  (17)
#define LMIXMUTEDACR			  (10)
#define LMIXMUTEDACL			  (11)
#define LMIXMUTELINEINL			  (12)
#define LMIXMUTEPHONEN			  (13)
#define LMIXMUTEPHONEPN			  (14)
#define LMIXMUTEMIC2BOOST		  (15)
#define LMIXMUTEMIC1BOOST		  (16)
#define RMIXMUTEDACL			  (17)
#define RMIXMUTEDACR			  (18)
#define RMIXMUTELINEINR			  (19)
#define RMIXMUTEPHONEP			  (20)
#define RMIXMUTEPHONEPN			  (21)
#define RMIXMUTEMIC2BOOST		  (22)
#define RMIXMUTEMIC1BOOST		  (23)

#define LMIXEN					  (28)
#define RMIXEN					  (29)
#define DACALEN					  (30)
#define DACAREN					  (31)

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

/*Output Mixer & PA Control Register
* codecbase+0x24
* new function
*/
#define HPPAEN					  (31)
#define HPCOM_CTL				  (29)
#define HPCOM_PRO				  (28)
#define PA_ANTI_POP_CTL			  (26)
#define LTRNMUTE				  (25)
#define RTLNMUTE				  (24)
#define MIC1G					  (15)
#define MIG2G					  (12)
#define LINEING					  (9)
#define PHONEG					  (6)
#define PHONEPG					  (3)
#define PHONENG					  (0)

/*Microphone,Lineout and Phoneout Control Register
* codecbase+0x28
* new function
*/
#define HBIASEN					  (31)
#define MBIASEN					  (30)
#define HBIASADCEN				  (29)
#define MIC1AMPEN				  (28)
#define MIC1BOOST				  (25)
#define MIC2AMPEN				  (24)
#define MIC2BOOST				  (21)
#define MIC2_SEL				  (20)
#define LINEOUTL_EN			  	  (19)
#define LINEOUTR_EN			  	  (18)
#define LINEOUTL_SRC_SEL		  (17)
#define	LINEOUTR_SRC_SEL		  (16)
#define	LINEOUT_VOL				  (11)
#define PHONEPREG				  (8)
#define PHONEOUTG				  (5)
#define PHONEOUT_EN				  (4)
#define PHONEOUTS0				  (3)
#define PHONEOUTS1				  (2)
#define PHONEOUTS2				  (1)
#define PHONEOUTS3				  (0)

/*ADC Analog Control Register
* codecbase+0x2c
*/
#define ADCREN					  (31)
#define ADCLEN					  (30)
#define ADCRG					  (27)
#define ADCLG					  (24)
#define RADCMIXMUTEMIC1BOOST	  (13)
#define RADCMIXMUTEMIC2BOOST	  (12)
#define RADCMIXMUTEPHONEPN		  (11)
#define RADCMIXMUTEPHONEP		  (10)
#define RADCMIXMUTELINEINR		  (9)
#define RADCMIXMUTEROUTPUT		  (8)
#define RADCMIXMUTELOUTPUT		  (7)
#define LADCMIXMUTEMIC1BOOST	  (6)
#define LADCMIXMUTEMIC2BOOST	  (5)
#define LADCMIXMUTEPHONEPN		  (4)
#define LADCMIXMUTEPHONEP		  (3)
#define LADCMIXMUTELINEINL		  (2)
#define LADCMIXMUTELOUTPUT		  (1)
#define LADCMIXMUTEROUTPUT		  (0)

#define RADCMIXMUTE				  (7)
#define LADCMIXMUTE				  (0)

/*ADDA Analog Performance Tuning Register
* codecbase+0x30
*/
#define PA_SLOPE_SELECT			  (30)
#define DITHER				   	  (25)
#define ZERO_CROSS_EN			  (22)
/*bias&DA16 Calibration verify register
* codecbase+0x34
*/
#define OPMIC_BIAS_CUR			  (30)
#define BIASCALIVERIFY			  (29)
#define BIASVERIFY				  (23)
#define BIASCALI				  (17)
#define DA16CALIVERIFY			  (16)
#define DA16VERIFY				  (8)
#define DA16CALI				  (0)

/*DAC Debug Register
* codecbase+0x48
*/
#define DAC_SWP					  (6)

/*HMIC Control Register
*codecbase+0x50
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

/*HMIC Data Register
* codecbase+0x54
*/
#define HMIC_EARPHONE_OUT_IRQ_PEND  (20)
#define HMIC_EARPHONE_IN_IRQ_PEND   (19)
#define HMIC_KEY_UP_IRQ_PEND 	    (18)
#define HMIC_KEY_DOWN_IRQ_PEND 		(17)
#define HMIC_DATA_IRQ_PEND			(16)
#define HMIC_ADC_DATA				(0)

/*DAC DAP Control Register
* codecbase+0x60
*/
#define DAC_DAP_EN						(31)
#define DAC_DAP_CTL						(30)
#define DAC_DAP_STATE					(29)
#define DAC_BQ_EN						(16)
#define DAC_DRC_EN						(15)
#define DAC_HPF_EN						(14)
#define DAC_DE_CTL						(12)
#define DAC_RAM_ADDR					(0)

/* DAC DAP volume register
*	codecbase+0x64
*/
#define DAC_LEFT_CHAN_SOFT_MUTE_CTL		(30)
#define DAC_RIGHT_CHAN_SOFT_MUTE_CTL	(29)
#define DAC_MASTER_SOFT_MUTE_CTL		(28)
#define DAC_SKEW_TIME_VOL_CTL			(24)
#define DAC_MASTER_VOL					(16)
#define DAC_LEFT_CHAN_VOL				(8)					
#define DAC_RIGHT_CHAN_VOL				(0)

/*ADC DAP Control Register
* codecbase+0x70
*/
#define ADC_DAP_EN					(31)
#define ADC_DAP_START				(30)
#define AGC_LEFT_SATURATION_FLAG	(21)
#define AGC_LEFT_NOISE_THRES_FLAG	(20)
#define AGC_LEFT_GAIN_APP			(12)
#define AGC_RIGHT_SATURATION_FLAG	(9)
#define AGC_RIGHT_NOISE_THRES_FLAG	(8)
#define AGC_RIGHT_GAIN_APP			(0)

/*ADC DAP Volume Register
* codecbase+0x74
*/
#define ADC_LEFT_CHAN_VOL_MUTE		(18)
#define ADC_RIGHT_CHAN_VOL_MUTE		(17)
#define ADC_SKEW_TIME_VOL			(16)
#define ADC_LEFT_CHAN_VOL			(8)
#define ADC_RIGHT_CHAN_VOL			(0)

/*ADC DAP Left control register
* codecbase+0x78
*/
#define ADC_LEFT_CHAN_NOISE_THRES_SET	(16)
#define ADC_LEFT_AGC_EN					(14)
#define ADC_LEFT_HPF_EN					(13)
#define ADC_LEFT_NOISE_DET_EN			(12)
#define ADC_LEFT_HYS_SET				(8)
#define ADC_LEFT_NOISE_DEBOURCE_TIME	(4)
#define ADC_LEFT_SIGNAL_DEBOUNCE_TIME	(0)

/*ADC DAP Right Control Register
* codecbase+0x7c
*/
#define ADC_RIGHT_CHAN_NOISE_THRES_SET	(16)
#define ADC_RIGHT_AGC_EN				(14)
#define ADC_RIGHT_HPF_EN				(13)
#define ADC_RIGHT_NOISE_DET_EN			(12)
#define ADC_RIGHT_HYS_SET				(8)
#define ADC_RIGHT_NOISE_DEBOURCE_TIME	(4)
#define ADC_RIGHT_SIGNAL_DEBOUNCE_TIME	(0)

/*ADC DAP Parameter Register
* codecbase+0x80
*/
#define ADC_LEFT_CHAN_TARG_LEVEL_SET	(24)
#define ADC_RIGHT_CHAN_TARG_LEVEL_SET	(16)
#define ADC_LEFT_CHAN_MAX_GAIN_SET		(8)
#define ADC_RIGHT_CHAN_MAX_GAIN_SET		(0)

/*ADC DAP Left Decay&Attack time register
* codecbase+0x88
*/
#define ADC_LEFT_ATTACK_TIME_COEFF_SET	(16)
#define ADC_LEFT_DECAY_TIME_COEFF_SET	(0)

/*ADC DAP Right decay&Attack Time register
* codecbase+0x90
*/
#define ADC_RIGHT_ATTACK_TIME_COEFF_SET	(16)
#define ADC_RIGHT_DECAY_TIME_COEFF_SET	(0)

static void  __iomem *baseaddr;
#define codec_rdreg(reg)	    readl((baseaddr+(reg)))
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
	int	min;
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

#endif
