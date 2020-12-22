/*
 * sound\soc\sunxi\audiocodec\sndcodec.c
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>

#include "sunxi_codecdma.h"
#include "sun8iw1_sndcodec.h"

#define sndpcm_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndpcm_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)
//void  __iomem *baseaddr;
static struct regulator* hp_ldo = NULL;
static char *hp_ldo_str = NULL;

/*for pa gpio ctrl*/
static int req_status;
static script_item_u item;
static script_item_value_type_e  type;

/*for phone call flag*/
static bool codec_phonein_en			= false;
static bool codec_speaker_en           = false;
static bool codec_mainmic_en           = false;
static bool codec_phoneout_en          = false;
static bool codec_lineinin_en          = false;
static bool codec_lineincap_en         = false;
static bool codec_headsetmic_en        = false;
static bool codec_speakerout_en        = false;
static bool codec_adcphonein_en        = false;
static bool codec_earpieceout_en       = false;
static bool codec_dacphoneout_en       = false;
static bool codec_voice_record_en      = false;
static bool codec_headphoneout_en      = false;
static bool codec_phonein_left_en		= false;
static bool codec_speakerout_lntor_en	= false;
static bool codec_headphoneout_lntor_en= false;
static bool codec_dacphoneout_reduced_en=false;
static bool codec_noise_reduced_adcin_en=false;
static int codec_speaker_headset_earpiece_en=0;

static struct clk *codec_pll2clk,*codec_moduleclk;

static struct sunxi_dma_params sunxi_pcm_pcm_stereo_out = {
	.name		= "audio_play",
	.dma_addr	= CODEC_BASSADDRESS + SUNXI_DAC_TXDATA,//send data address	
};

static struct sunxi_dma_params sunxi_pcm_pcm_stereo_in = {
	.name   	= "audio_capture",
	.dma_addr	= CODEC_BASSADDRESS + SUNXI_ADC_RXDATA,//accept data address	
};

/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
static int codec_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	unsigned int old, new;
		
	old	=	codec_rdreg(reg);
	new	=	(old & ~mask) | value;
	codec_wrreg(reg,new);

	return 0;
}

/**
*	snd_codec_info_volsw	-	single	mixer	info	callback
*	@kcontrol:	mixer control
*	@uinfo:	control	element	information
*	Callback to provide information about a single mixer control
*
*	Returns 0 for success
*/
int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo)
{
	struct	codec_mixer_control *mc	= (struct codec_mixer_control*)kcontrol->private_value;
	int	max	=	mc->max;
	unsigned int shift  = mc->shift;
	unsigned int rshift = mc->rshift;

	if (max	== 1)
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;//the info of type
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;

	uinfo->count = shift ==	rshift	?	1:	2;	//the info of elem count
	uinfo->value.integer.min = 0;				//the info of min value
	uinfo->value.integer.max = max;				//the info of max value
	return	0;
}

/**
*	snd_codec_get_volsw	-	single	mixer	get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control
*	return 0 for success.
*/
int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int	max = mc->max;
	/*fls(7) = 3,fls(1)=1,fls(0)=0,fls(15)=4,fls(3)=2,fls(23)=5*/
	unsigned int mask = (1 << fls(max)) -1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;

	ucontrol->value.integer.value[0] =	
		(codec_rdreg(reg)>>	shift) & mask;
	if (shift != rshift)
		ucontrol->value.integer.value[1] =
			(codec_rdreg(reg) >> rshift) & mask;

	if (invert) {
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if(shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
		}
	
		return 0;
}

/**
*	snd_codec_put_volsw	-	single	mixer put callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to put the value of a single mixer control
*
* return 0 for success.
*/
int snd_codec_put_volsw(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1<<fls(max))-1;
	unsigned int invert = mc->invert;
	unsigned int	val, val2, val_mask;

	val = (ucontrol->value.integer.value[0] & mask);
	if(invert)
		val = max - val;
	val <<= shift;
	val_mask = mask << shift;
	if(shift != rshift){
		val2	= (ucontrol->value.integer.value[1] & mask);
		if(invert)
			val2	=	max	- val2;
		val_mask |= mask <<rshift;
		val |= val2 <<rshift;
	}
	
	return codec_wrreg_bits(reg,val_mask,val);
}

static int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}
#if 0
static int codec_rd_control(u32 reg, u32 bit, u32 *val)
{
	return 0;
}
#endif
/*
*	enable the codec function which should be enable during system init.
*/
static void codec_init(void)
{
	int headphone_direct_used = 0;
	script_item_u val;
	script_item_value_type_e  type;
//	enum sw_ic_ver  codec_chip_ver;

//	codec_chip_ver = sw_get_ic_ver();
	type = script_get_item("audio0", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
       pr_err("[audiocodec] headphone_direct_used type err!\n");
    }
	headphone_direct_used = val.val;
//	if (headphone_direct_used && (codec_chip_ver != MAGIC_VER_A31A)) {
//		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x3);
//		codec_wr_control(SUNXI_PA_CTRL, 0x1, HPCOM_PRO, 0x1);
//	} else {
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUNXI_PA_CTRL, 0x1, HPCOM_PRO, 0x0);
//	}

	/*mute l_pa and r_pa.*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);

	/*when TX FIFO available room less than or equal N,
	* DRQ Requeest will be de-asserted.
	*/
	codec_wr_control(SUNXI_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);

	/*write 1 to flush tx fifo*/
	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, DAC_FIFO_FLUSH, 0x1);
	/*write 1 to flush rx fifo*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);

	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, FIR_VERSION, 0x1);
}

/*
*	the system voice come out from speaker
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_pa_play_open(void)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;

	type = script_get_item("audio0", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] pa_double_used type err!\n");
	}

	pa_double_used = val.val;
	if (!pa_double_used) {
		type = script_get_item("audio0", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] pa_single_vol type err!\n");
		}
		pa_vol = val.val;
	} else {
		type = script_get_item("audio0", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] pa_double_vol type err!\n");
		}
		pa_vol = val.val;
	}
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);
	if (!pa_double_used) {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
	}
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);

	usleep_range(2000, 3000);
	gpio_set_value(item.gpio.gpio, 1);
	msleep(62);
	return 0;
}

/*
*	the system voice come out from headphone
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_headphone_play_open(void)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
   	pr_err("[audiocodec], headphone_vol type err!\n");
    }
	headphone_vol = val.val;
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);

	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	/*set HPVOL volume*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

	return 0;
}

static int codec_earpiece_play_open(void)
{
	int earpiece_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "earpiece_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
   	pr_err("[audiocodec] headphone_vol type err!\n");
    }
	earpiece_vol = val.val;
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);

	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	/*send last sample when dac fifo under run*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);

	/*select the analog mixer input source*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	/*unmute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

	/*select HPL inverting output*/
	codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x1);
	/*set HPVOL volume*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, earpiece_vol);

	return 0;
}

/*
*	the system voice come out from headphone and speaker
*	while the phone call in, the phone use the headset, you can hear the voice from speaker and headset.
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_pa_and_headset_play_open(void)
{
	int pa_vol = 0,	headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio0", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] pa_double_used type err!\n");
	}

	pa_double_used = val.val;
	if (!pa_double_used) {
		type = script_get_item("audio0", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] pa_single_vol type err!\n");
		}
		pa_vol = val.val;
	} else {
		type = script_get_item("audio0", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] pa_double_vol type err!\n");
		}
		pa_vol = val.val;
	}

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
       	pr_err("[audiocodec] headphone_vol type err!\n");
    	}
	headphone_vol = val.val;

	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);
	if (!pa_double_used) {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
	}
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);
	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x2);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);
	
	codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);

	/*set HPVOL volume*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

	usleep_range(2000, 3000);
	gpio_set_value(item.gpio.gpio, 1);
	msleep(62);
	/*unmute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

	return 0;
}

/*
*	use for phone record from main mic + phone in.
*	mic1 is use as main mic.
*/
static int codec_voice_main_mic_capture_open(void)
{
	/*enable mic1 pa*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*enable Master microphone bias*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x1);

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCLEN, 0x1);

	/*enable Right MIC1 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);

	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x1);
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x1);

	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for phone record from sub mic + phone in.
*	mic2 is use as sub mic.
* 	mic2 is the headset mic.
*/
static int codec_voice_headset_mic_capture_open(void)
{
	/*enable Right MIC2 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC2 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);

	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x1);
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x1);

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for the line_in record
*/
static int codec_voice_linein_capture_open(void)
{
	/*enable LINEINR ADC*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTELINEINR, 0x1);
	/*enable LINEINL ADC*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTELINEINL, 0x1);

	/*enable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for the phone noise reduced while in phone model.
*	use the mic1 and mic3 to reduecd the noise from the phone in
*	mic3 use the same channel of mic2.
*/
static int codec_noise_reduced_capture_open(void)
{
	/*enable mic1 pa*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*enable Master microphone bias*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x1);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x1);
	/*select mic3 source:0:mic3,1:mic2 */
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2_SEL, 0x0);

	/*enable Right MIC2 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for the base system record(for pad record).
*/
static int codec_capture_open(void)
{
	int cap_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "cap_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] cap_vol type err!\n");
	}
	cap_vol = val.val;

	/*enable mic1 pa*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x7,MIC1BOOST,cap_vol);//36db
	/*enable Master microphone bias*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x1);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x1);
	/*select mic3 source:0:mic3,1:mic2 */
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2_SEL, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x7,MIC2BOOST,cap_vol);//36db

	/*enable Right MIC2 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	/*hardware fifo delay*/
	msleep(200);

	return 0;
}

static int codec_play_start(void)
{
	int i = 0;
	u32 reg_val = 0;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;
	/*enable dac drq*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x1);
	/*DAC FIFO Flush,Write '1' to flush TX FIFO, self clear to '0'*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);

if (codec_speaker_en || (codec_speaker_headset_earpiece_en==1)||(codec_speaker_headset_earpiece_en==2)) {
		;
	} else if ( (codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en || codec_dacphoneout_en) ){
		;
	} else {
		/*set the default output is HPOUTL/R for pad headphone*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);
	
		reg_val = codec_rdreg(SUNXI_DAC_ACTL);
		reg_val &= 0x3f;
		if (!reg_val) {
			for(i=0; i < headphone_vol; i++) {
				/*set HPVOL volume*/
				codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, i);
				reg_val = codec_rdreg(SUNXI_DAC_ACTL);
				reg_val &= 0x3f;
				if ((i%2==0))
					usleep_range(1000,2000);
			}
		}
	}

	return 0;
}

static int codec_play_stop(void)
{
		int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	/*disable dac drq*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);

	if ( !(codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en ||
					codec_dacphoneout_en || codec_lineinin_en || codec_voice_record_en ) ) {
		gpio_set_value(item.gpio.gpio, 0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);
	}

	return 0;
}

static int codec_capture_stop(void)
{
	/*disable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x0);
	/*disable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
	if (!(codec_voice_record_en||codec_mainmic_en||codec_headsetmic_en||codec_lineincap_en)) {
		/*disable mic1 pa*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x0);
		/*disable mic2/mic3 pa*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);

		/*disable Right MIC1 Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
		/*disable Left MIC1 Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);

		/*disable Right MIC2 Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
		/*disable Left MIC2 Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);

		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x0);
		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x0);

		/*disable LINEINR ADC*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTELINEINR, 0x0);
		/*disable LINEINL ADC*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTELINEINL, 0x0);
	}
	/*disable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, ADCREN, 0x0);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, ADCLEN, 0x0);

	return 0;
}
#if 0
static int codec_dev_free(struct snd_device *device)
{
	return 0;
};
#endif
/*
*	codec_lineinin_en == 1, open the linein in.
*	codec_lineinin_en == 0, close the linein in.
*/
static int codec_set_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_lineinin_en = ucontrol->value.integer.value[0];

	if (codec_lineinin_en) {
		/*select LINEINR*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x4);
		/*select LINEINL*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x4);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);
	} else {
		codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x0);
	}
	return 0;
}

static int codec_get_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_lineinin_en;
	return 0;
}

/*
*	use for linein record
*/
static int codec_set_lineincap(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_lineincap_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_lineincap(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_lineincap_en;
	return 0;
}

/*
*	codec_speakerout_lntor_en == 1, open the speaker.
*	codec_speakerout_lntor_en == 0, close the speaker.
*	if the phone in voice just use left channel for phone call(right channel used for noise reduced), 
*	the speaker's right channel's voice must use the left channel to transfer phone call voice.
*	lntor:left negative to right
*/
static int codec_set_speakerout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio0", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
       pr_err("[audiocodec] pa_double_used type err!\n");
    }
    pa_double_used = val.val;
    if (!pa_double_used) {
		type = script_get_item("audio0", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("[audiocodec] pa_single_vol type err!\n");
	    }
		pa_vol = val.val;
	} else {
		type = script_get_item("audio0", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("[audiocodec] pa_double_vol type err!\n");
	    }
		pa_vol = val.val;
	}

	codec_speakerout_lntor_en = ucontrol->value.integer.value[0];

	if (codec_speakerout_lntor_en) {
		/*close headphone and earpiece out routeway*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0x0);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);

		usleep_range(2000, 3000);
		gpio_set_value(item.gpio.gpio, 1);
		msleep(62);

		codec_headphoneout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		gpio_set_value(item.gpio.gpio, 0);
	}

	return 0;
}
/*
*	lntor:left negative to right
*/
static int codec_get_speakerout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speakerout_lntor_en;
	return 0;
}

/*
*	codec_speakerout_en == 1, open the speaker.
*	codec_speakerout_en == 0, close the speaker.
*/
static int codec_set_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int pa_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	int pa_double_used = 0;
	type = script_get_item("audio0", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] pa_double_used type err!\n");
    }
    pa_double_used = val.val;
    if (!pa_double_used) {
		type = script_get_item("audio0", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("[audiocodec] pa_single_vol type err!\n");
	    }
		pa_vol = val.val;
	} else {
		type = script_get_item("audio0", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("[audiocodec] pa_double_vol type err!\n");
	    }
		pa_vol = val.val;
	}

	codec_speakerout_en = ucontrol->value.integer.value[0];

	if (codec_speakerout_en) {
		/*close headphone and earpiece out routeway*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0x0);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x1);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x1);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, pa_vol);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);

		usleep_range(2000, 3000);
		gpio_set_value(item.gpio.gpio, 1);
		msleep(62);

		codec_headphoneout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);

		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		
		gpio_set_value(item.gpio.gpio, 0);
	}

	return 0;
}

static int codec_get_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speakerout_en;
	return 0;
}

/*
*	codec_headphoneout_lntor_en == 1, open the headphone.
*	codec_headphoneout_lntor_en == 0, close the headphone.
* 	lntor:left negative to right
*/
static int codec_set_headphoneout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_headphoneout_lntor_en = ucontrol->value.integer.value[0];

	if (codec_headphoneout_lntor_en) {
		/*close speaker earpiece out routeway*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		gpio_set_value(item.gpio.gpio, 0);

		/*select HPL inverting output*/
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		/*Left channel to right channel no mute*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LTRNMUTE, 0x1);
		/*select the analog mixer input source*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
		/*set HPVOL volume*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

		codec_speakerout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		/*select the default dac input source*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	}

	return 0;
}
/*
* 	lntor:left negative to right
*/
static int codec_get_headphoneout_lntor(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headphoneout_lntor_en;
	return 0;
}

/*
*	codec_headphoneout_en == 1, open the headphone.
*	codec_headphoneout_en == 0, close the headphone.
*/
static int codec_set_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_headphoneout_en = ucontrol->value.integer.value[0];

	if (codec_headphoneout_en) {
		/*close speaker earpiece out routeway*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		gpio_set_value(item.gpio.gpio, 0);

		/*select HPL inverting output*/
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

		/*select the analog mixer input source*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1);
		/*set HPVOL volume*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, headphone_vol);

		codec_speakerout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		/*select the default dac input source*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	}

	return 0;
}

static int codec_get_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headphoneout_en;
	return 0;
}

/*
*	codec_earpieceout_en == 1, open the earpiece.
*	codec_earpieceout_en == 0, close the earpiece.
*/
static int codec_set_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int earpiece_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "earpiece_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_vol type err!\n");
    }
	earpiece_vol = val.val;

	codec_earpieceout_en = ucontrol->value.integer.value[0];

	if (codec_earpieceout_en) {
		/*close speaker out routeway*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		gpio_set_value(item.gpio.gpio, 0);

		/*close headphone routeway*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0x0);

		/*open earpiece out routeway*/
		/*unmute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		/*select the analog mixer input source*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x1);
		/*select HPL inverting output*/
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x1);

		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x1); 
		/*set HPVOL volume*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, earpiece_vol);

		codec_speakerout_en = 0;
		codec_headphoneout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);

		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	}

	return 0;
}

static int codec_get_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_earpieceout_en;
	return 0;
}

/*
*	codec_phonein_left_en == 1, the phone in left channel is open.
*	while you open one of the device(speaker,earpiece,headphone).
*	you can hear the caller's voice.
*	codec_phonein_left_en == 0. the phone in left channel is close.
*/
static int codec_set_phonein_left(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phonein_left_en = ucontrol->value.integer.value[0];

	if (codec_phonein_left_en) {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUNXI_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	} else {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUNXI_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x0);
	}

	return 0;
}

static int codec_get_phonein_left(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phonein_left_en;
	return 0;
}

/*
*	codec_phonein_en == 1, the phone in is open.
*	while you open one of the device(speaker,earpiece,headphone).
*	you can hear the caller's voice.
*	codec_phonein_en == 0. the phone in is close.
*/
static int codec_set_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phonein_en = ucontrol->value.integer.value[0];

	if (codec_phonein_en) {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUNXI_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x1);
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXMUTEPHONEPN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);
	} else {
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUNXI_DAC_ACTL, 0X1, LMIXMUTEPHONEPN, 0x0);
		/*select PHONEP-PHONEN*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXMUTEPHONEPN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x0);
	}

	return 0;
}

static int codec_get_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phonein_en;
	return 0;
}

/*
*	codec_phoneout_en == 1, the phone out is open. receiver can hear the voice which you say.
*	codec_phoneout_en == 0,	the phone out is close.
*/
static int codec_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phoneout_en = ucontrol->value.integer.value[0];

	if (codec_phoneout_en) {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUT_EN, 0x1);
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUT_EN, 0x0);
	}

	return 0;
}

static int codec_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phoneout_en;
	return 0;
}

static int codec_dacphoneout_reduced_open(void)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x2);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS2, 0x1);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS3, 0x1);
	return 0;
}

/*
*	codec_dacphoneout_reduced_en == 1, the dac phone out is open. the receiver can hear the voice from system.
*	codec_dacphoneout_reduced_en == 0,	the dac phone out is close.
*/
static int codec_set_dacphoneout_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_dacphoneout_reduced_en = ucontrol->value.integer.value[0];

	if (codec_dacphoneout_reduced_en) {
		ret = codec_dacphoneout_reduced_open();
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);
	}

	return ret;
}

static int codec_get_dacphoneout_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_dacphoneout_reduced_en;
	return 0;
}

static int codec_dacphoneout_open(void)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXMUTEDACR, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXMUTEDACL, 0x1);

	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);
	
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS2, 0x1);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS3, 0x1);
	return 0;
}

/*
*	codec_dacphoneout_en == 1, the dac phone out is open. the receiver can hear the voice from system.
*	codec_dacphoneout_en == 0,	the dac phone out is close.
*/
static int codec_set_dacphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_dacphoneout_en = ucontrol->value.integer.value[0];

	if (codec_dacphoneout_en) {
		ret = codec_dacphoneout_open();
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);
	}

	return ret;
}

static int codec_get_dacphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_dacphoneout_en;
	return 0;
}

static int codec_adcphonein_open(void)
{
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x1);
	/*enable PHONEP-PHONEN Boost stage*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCREN, 0x1);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);

	return 0;
}

/*
*	codec_adcphonein_en == 1, the adc phone in is open. you can record the phonein from adc.
*	codec_adcphonein_en == 0,	the adc phone in is close.
*/
static int codec_set_adcphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_adcphonein_en = ucontrol->value.integer.value[0];

	if (codec_adcphonein_en) {
		ret = codec_adcphonein_open();
	} else {
		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x0);
		/*disable PHONEP-PHONEN Boost stage*/
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x0);
	}

	return ret;
}

static int codec_get_adcphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_adcphonein_en;
	return 0;
}

/*
*	codec_mainmic_en == 1, open mic1.
*	codec_mainmic_en == 0, close mic1.
*/
static int codec_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "main_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] codec_set_mainmic type err!\n");
    }

	codec_mainmic_en = ucontrol->value.integer.value[0];

	if (codec_mainmic_en) {
		/*close headset mic(mic2) routeway*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS1, 0x0);
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
		
		/*open main mic(mic1) routeway*/
		/*enable mic1 pa*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x7,MIC1BOOST, 0x6);
		/*enable Master microphone bias*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS0, 0x1);

		/*set the headset mic flag false*/
		codec_headsetmic_en = 0;
	} else {
		/*disable mic pa*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS0, 0x0);
	}

	return 0;
}

static int codec_get_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_mainmic_en;
	return 0;
}

/*
*	codec_voice_record_en == 1, set status.
*	codec_voice_record_en == 0, set status.
*/
static int codec_set_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_voice_record_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_voicerecord(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_voice_record_en;
	return 0;
}

/*
*	codec_noise_reduced_adcin_en == 1, set status.
*	codec_noise_reduced_adcin_en == 0, set status.
*/
static int codec_set_noise_adcin_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_noise_reduced_adcin_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_noise_adcin_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_noise_reduced_adcin_en;
	return 0;
}

/*
*	codec_headsetmic_en == 1, open mic2.
*	codec_headsetmic_en == 0, close mic2.
*/
static int codec_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("audio0", "headset_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	    pr_err("[audiocodec] codec_set_headsetmic type err!\n");
    }

	codec_headsetmic_en = ucontrol->value.integer.value[0];

	if (codec_headsetmic_en) {
		/*close main mic(mic1) routeway*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS0, 0x0);
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
		codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
		
		/*open headset mic(mic2) routeway*/
		/*enable mic2 pa*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x1);
		/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x7,MIC2BOOST,0x6);

		/*select mic2 source */
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2_SEL, 0x1);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS1, 0x1);

		/*set the main mic flag false*/
		codec_mainmic_en	= 0;
	} else {
		/*disable mic pa*/
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS1, 0x0);
	}

	return 0;
}

static int codec_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_headsetmic_en;
	return 0;
}

/*
*	close all phone routeway
*/
static int codec_set_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	/*close adc phonein routeway*/
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEPHONEPN, 0x0);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEPHONEPN, 0x0);

	/*close dac phoneout routeway*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS2, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS3, 0x0);

	/*close headset mic(mic2)*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC2AMPEN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS1, 0x0);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
	
	/*close main mic(mic1)*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, MBIASEN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUTS0, 0x0);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
	codec_wr_control(SUNXI_ADC_ACTL, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
	
	/*close earpiece and headphone routeway*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);
	codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
	codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0x0);

	/*close speaker routeway*/	
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_SRC_SEL, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_SRC_SEL, 0x0);
	gpio_set_value(item.gpio.gpio, 0);

	/*close analog phone in routeway*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, RMIXMUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x7f, LMIXMUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x0);
	
	/*disable phone out*/
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, PHONEOUT_EN, 0x0);
	
	/*set all routeway flag false*/
	codec_adcphonein_en 	= 0;
	codec_dacphoneout_en	= 0;
	codec_headsetmic_en		= 0;
	codec_mainmic_en		= 0;

	codec_speakerout_en 	= 0;
	codec_earpieceout_en 	= 0;
	codec_headphoneout_en 	= 0;
	codec_phoneout_en		= 0;
	codec_phonein_en		= 0;
	return 0;
}

static int codec_get_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}

/*
*	codec_speaker_en == 1, speaker is open, headphone is close.
*	codec_speaker_en == 0, speaker is closed, headphone is open.
*	this function just used for the system voice(such as music and moive voice and so on),
*	no the phone call.
*/
static int codec_set_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0, i = 0;
	u32 reg_val;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	codec_speaker_en = ucontrol->value.integer.value[0];
	pr_debug("%s, line:%d, codec_speaker_en:%d\n", __func__, __LINE__, codec_speaker_en);
	
	if (codec_speaker_en) {
		ret = codec_pa_play_open();
	} else {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);

		gpio_set_value(item.gpio.gpio, 0);

		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

		type = script_get_item("audio0", "headphone_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] headphone_vol type err!\n");
		}
		headphone_vol = val.val;
		reg_val = codec_rdreg(SUNXI_DAC_ACTL);
		reg_val &= 0x3f;
		if (!reg_val) {
			for (i=0; i < headphone_vol; i++) {
				/*set HPVOL volume*/
				codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, i);
				reg_val = codec_rdreg(SUNXI_DAC_ACTL);
				reg_val &= 0x3f;
				if ((i%2==0))
					usleep_range(1000,2000);
			}
		}
	}

	return 0;
}

static int codec_get_spk(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_en;
	return 0;
}

/*
*	codec_speaker_headset_earpiece_en == 3, earpiece is open,speaker and headphone is close.
*	codec_speaker_headset_earpiece_en == 2, speaker is open, headphone is open.
*	codec_speaker_headset_earpiece_en == 1, speaker is open, headphone is close.
*	codec_speaker_headset_earpiece_en == 0, speaker is closed, headphone is open.
*	this function just used for the system voice(such as music and moive voice and so on),
*	no the phone call.
*/
static int codec_set_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int i = 0;
	int ret = 0;
	int reg_val = 0;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	codec_speaker_headset_earpiece_en = ucontrol->value.integer.value[0];
pr_debug("%s, line:%d, codec_speaker_headset_earpiece_en:%d\n", __func__, __LINE__, codec_speaker_headset_earpiece_en);
	if (codec_speaker_headset_earpiece_en == 1) {
		ret = codec_pa_play_open();
	} else if (codec_speaker_headset_earpiece_en == 0) {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);

		gpio_set_value(item.gpio.gpio, 0);

		codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x1);

		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LMIXEN, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RMIXEN, 0x1);

		/*unmute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);

		type = script_get_item("audio0", "headphone_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] headphone_vol type err!\n");
		}
		headphone_vol = val.val;
		reg_val = codec_rdreg(SUNXI_DAC_ACTL);
		reg_val &= 0x3f;
		if (!reg_val) {
			for (i=0; i < headphone_vol; i++) {
				/*set HPVOL volume*/
				codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, i);
				reg_val = codec_rdreg(SUNXI_DAC_ACTL);
				reg_val &= 0x3f;
				if ((i%2==0))
					usleep_range(1000,2000);
			}
		}
	} else if (codec_speaker_headset_earpiece_en == 2) {
		pr_debug("%s, line:%d\n", __func__, __LINE__);
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_pa_and_headset_play_open();
	} else if (codec_speaker_headset_earpiece_en == 3) {
		codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTL_EN, 0x0);
		codec_wr_control(SUNXI_MIC_CTRL, 0x1, LINEOUTR_EN, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPIS, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPIS, 0x0);

		gpio_set_value(item.gpio.gpio, 0);
		codec_earpiece_play_open();
	}
	return 0;
}

static int codec_get_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_headset_earpiece_en;
	return 0;
}

static int sndpcm_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int sndpcm_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void sndpcm_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	int i = 0;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;
	pr_debug("%s, line:%d\n", __func__, __LINE__);
	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	if ( !(codec_speakerout_en || codec_headphoneout_en || codec_earpieceout_en ||
					codec_dacphoneout_en || codec_lineinin_en || codec_voice_record_en ) ) {
		for (i = headphone_vol; i > 0 ; i--) {
			/*set HPVOL volume*/
			codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, i);
			usleep_range(2000,3000);
		}
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0);
		/*mute l_pa and r_pa*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
		codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	}
	/*disable dac drq*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);
	/*disable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x0);

	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);
}

static int sndpcm_perpare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
		int play_ret = 0, capture_ret = 0;
	unsigned int reg_val;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (substream->runtime->rate) {
			case 44100:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 22050:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 11025:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 48000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 96000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(7<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 192000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(6<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 32000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 24000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 16000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 12000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 8000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			default:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val |=(1<<6);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			case 2:
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
			default:
				reg_val = readl(baseaddr + SUNXI_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUNXI_DAC_FIFOC);
				break;
		}
	} else {
		switch (substream->runtime->rate) {
			case 44100:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 22050:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 11025:
				if (clk_set_rate(codec_pll2clk, 22579200)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 22579200)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 48000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 32000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 24000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 16000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 12000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			case 8000:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
				break;
			default:
				if (clk_set_rate(codec_pll2clk, 24576000)) {
					pr_err("set codec_pll2clk rate fail\n");
				}
				if (clk_set_rate(codec_moduleclk, 24576000)) {
					pr_err("set codec_moduleclk rate fail\n");
				}
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);		
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val |=(1<<7);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
			break;
			case 2:
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
			break;
			default:
				reg_val = readl(baseaddr + SUNXI_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUNXI_ADC_FIFOC);
			break;
		}
	}
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
   	 	/*open the dac channel register*/
   	 	if (codec_dacphoneout_reduced_en) {
   	 		codec_dacphoneout_reduced_open();
   	 	} else if (codec_dacphoneout_en) {
			play_ret = codec_dacphoneout_open();
		} else if (codec_speaker_headset_earpiece_en == 2) {
			play_ret = codec_pa_and_headset_play_open();
		} else if ((codec_speaker_headset_earpiece_en == 1)||(codec_speaker_en)) {
			play_ret = codec_pa_play_open();
		} else if (codec_speaker_headset_earpiece_en == 3) { 
			play_ret = codec_earpiece_play_open();
		} else {
			play_ret = codec_headphone_play_open();
		}
		codec_play_start();
		return play_ret;
	} else {
	   	/*open the adc channel register*/
	   	if (codec_adcphonein_en) {
	   		codec_adcphonein_open();
	   		/*hardware fifo delay*/
			msleep(200);
		} else if (codec_voice_record_en && codec_mainmic_en) {
			codec_voice_main_mic_capture_open();
		} else if (codec_voice_record_en && codec_headsetmic_en){
			codec_voice_headset_mic_capture_open();
		} else if(codec_lineinin_en && codec_lineincap_en) {
			codec_voice_linein_capture_open();
		} else if(codec_voice_record_en && codec_noise_reduced_adcin_en){
			codec_noise_reduced_capture_open();
		} else {
	   		codec_capture_open();
		}
		return capture_ret;
	}
}	
static int sndpcm_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (cmd) {
			case SNDRV_PCM_TRIGGER_START:
			case SNDRV_PCM_TRIGGER_RESUME:
			case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
				return 0;
			case SNDRV_PCM_TRIGGER_SUSPEND:
			case SNDRV_PCM_TRIGGER_STOP:
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
				codec_play_stop();
				return 0;
			default:
				return -EINVAL;
			}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			codec_capture_stop();
			return 0;
		default:
			pr_err("error:%s,%d\n", __func__, __LINE__);
			return -EINVAL;
		}
	}
	return 0;
}
static int sndpcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sunxi_dma_params *dma_data;

	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sunxi_pcm_pcm_stereo_out;
	else
		dma_data = &sunxi_pcm_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sndpcm_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int sndpcm_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{
	return 0;
}

static int sndpcm_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static const char *spk_headset_earpiece_function[] = {"headset", "spk", "spk_headset", "earpiece"};
static const struct soc_enum spk_headset_earpiece_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_headset_earpiece_function), spk_headset_earpiece_function),
};

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/*SUNXI_DAC_ACTL = 0x20,PAVOL*/
	CODEC_SINGLE("Master Playback Volume", SUNXI_DAC_ACTL, 0, 0x3f, 0),

	/*SUNXI_PA_CTRL = 0x24*/
	CODEC_SINGLE("MIC1_G boost stage output mixer control", 	SUNXI_PA_CTRL, 15, 0x7, 0),
	CODEC_SINGLE("MIC2_G boost stage output mixer control", 	SUNXI_PA_CTRL, 12, 0x7, 0),
	CODEC_SINGLE("LINEIN_G boost stage output mixer control", 	SUNXI_PA_CTRL, 9, 0x7, 0),
	CODEC_SINGLE("PHONE_G boost stage output mixer control", 	SUNXI_PA_CTRL, 6, 0x7, 0),
	CODEC_SINGLE("PHONE_PG boost stage output mixer control", 	SUNXI_PA_CTRL, 3, 0x7, 0),
	CODEC_SINGLE("PHONE_NG boost stage output mixer control", 	SUNXI_PA_CTRL, 0, 0x7, 0),

	/*SUNXI_MIC_CTRL = 0x28*/
	CODEC_SINGLE("MIC1 boost AMP gain control", SUNXI_MIC_CTRL,25,0x7,0),
	CODEC_SINGLE("MIC2 boost AMP gain control", SUNXI_MIC_CTRL,21,0x7,0),
	CODEC_SINGLE("Lineout volume control", 		SUNXI_MIC_CTRL,11,0x1f,0),
	CODEC_SINGLE("PHONEP-PHONEN pre-amp gain control", SUNXI_MIC_CTRL,8,0x7,0),
	CODEC_SINGLE("Phoneout gain control", 		SUNXI_MIC_CTRL,5,0x7,0),

	/*SUNXI_ADC_ACTL = 0x2c*/
	CODEC_SINGLE("ADC input gain ctrl", 		SUNXI_ADC_ACTL,27,0x7,0),

	SOC_SINGLE_BOOL_EXT("Audio Spk Switch", 	0, codec_get_spk, 		codec_set_spk),						/*for pad speaker,headphone switch*/
	SOC_SINGLE_BOOL_EXT("Audio phone out", 		0, codec_get_phoneout, 	codec_set_phoneout),				/*enable phoneout*/
	SOC_SINGLE_BOOL_EXT("Audio phone in", 		0, codec_get_phonein, 	codec_set_phonein),					/*open the phone in call*/
	SOC_SINGLE_BOOL_EXT("Audio phone in left", 	0, codec_get_phonein_left, 	codec_set_phonein_left),		/*open the phone in left channel call*/
	SOC_SINGLE_BOOL_EXT("Audio earpiece out", 	0, codec_get_earpieceout, 	codec_set_earpieceout),			/*set the phone in call voice through earpiece out*/
	SOC_SINGLE_BOOL_EXT("Audio headphone out", 	0, codec_get_headphoneout, 	codec_set_headphoneout),		/*set the phone in call voice through headphone out*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out", 	0, codec_get_speakerout, 	codec_set_speakerout),			/*set the phone in call voice through speaker out*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out left",   0, codec_get_speakerout_lntor,   codec_set_speakerout_lntor),
	SOC_SINGLE_BOOL_EXT("Audio headphone out left", 0, codec_get_headphoneout_lntor, codec_set_headphoneout_lntor),

	SOC_SINGLE_BOOL_EXT("Audio adc phonein", 	0, codec_get_adcphonein, 	codec_set_adcphonein), 			/*bluetooth voice*/
	SOC_SINGLE_BOOL_EXT("Audio dac phoneout", 	0, codec_get_dacphoneout, 	codec_set_dacphoneout),    		/*bluetooth voice */
	SOC_SINGLE_BOOL_EXT("Audio phone mic", 		0, codec_get_mainmic, 		codec_set_mainmic), 			/*set main mic(mic1)*/
	SOC_SINGLE_BOOL_EXT("Audio phone headsetmic", 	0, codec_get_headsetmic, 	codec_set_headsetmic),    	/*set headset mic(mic2)*/
	SOC_SINGLE_BOOL_EXT("Audio phone voicerecord", 	0, codec_get_voicerecord, 	codec_set_voicerecord),   	/*set voicerecord status*/
	SOC_SINGLE_BOOL_EXT("Audio phone endcall", 	0, codec_get_endcall, 	codec_set_endcall),    				/*set voicerecord status*/

	SOC_SINGLE_BOOL_EXT("Audio linein record", 	0, codec_get_lineincap, codec_set_lineincap),
	SOC_SINGLE_BOOL_EXT("Audio linein in", 		0, codec_get_lineinin, 	codec_set_lineinin),
	SOC_SINGLE_BOOL_EXT("Audio noise adcin reduced", 	0, codec_get_noise_adcin_reduced, codec_set_noise_adcin_reduced),
	SOC_SINGLE_BOOL_EXT("Audio noise dacphoneout reduced", 	0, codec_get_dacphoneout_reduced, codec_set_dacphoneout_reduced),

	SOC_ENUM_EXT("Speaker Function", spk_headset_earpiece_enum[0], codec_get_spk_headset_earpiece, codec_set_spk_headset_earpiece),
};

static struct snd_soc_dai_ops sndpcm_dai_ops = {
	.startup = sndpcm_startup,
	.shutdown = sndpcm_shutdown,
	.prepare  =	sndpcm_perpare,
	.trigger 	= sndpcm_trigger,
	.hw_params = sndpcm_hw_params,
	.digital_mute = sndpcm_mute,
	.set_sysclk = sndpcm_set_dai_sysclk,
	.set_clkdiv = sndpcm_set_dai_clkdiv,
	.set_fmt = sndpcm_set_dai_fmt,
};

static struct snd_soc_dai_driver sndpcm_dai = {
	.name = "sndcodec",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndpcm_RATES,
		.formats = sndpcm_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndpcm_RATES,
		.formats = sndpcm_FORMATS,
	},
	/* pcm operations */
	.ops = &sndpcm_dai_ops,
};
EXPORT_SYMBOL(sndpcm_dai);

static int sndpcm_soc_probe(struct snd_soc_codec *codec)
{
	/* Add virtual switch */
	snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));

	return 0;
}

static int sndpcm_suspend(struct snd_soc_codec *codec)
{
	int i =0;
	u32 reg_val;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	item.gpio.data = 0;
	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;
	pr_debug("[audio codec]:suspend\n");

	codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	for (i = headphone_vol; i > 0 ; i--) {
		/*set HPVOL volume*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, i);
		reg_val = codec_rdreg(SUNXI_DAC_ACTL);
		reg_val &= 0x3f;
		if ((i%2==0))
			usleep_range(2000, 3000);
	}
	codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*disable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);
	
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, HBIASEN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, HBIASADCEN, 0x0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	msleep(100);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	msleep(20);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);

	gpio_set_value(item.gpio.gpio, 0);
	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_disable_unprepare(codec_moduleclk);
	}
	pr_debug("[audio codec]:suspend end\n");
	return 0;
}

static int sndpcm_resume(struct snd_soc_codec *codec)
{
	script_item_u val;
	script_item_value_type_e  type;
	int headphone_direct_used = 0;
//	enum sw_ic_ver  codec_chip_ver;

//	codec_chip_ver = sw_get_ic_ver();
	
	pr_debug("[audio codec]:resume start\n");

	if (clk_prepare_enable(codec_moduleclk)) {
		pr_err("open codec_moduleclk failed; \n");
	}
	
	type = script_get_item("audio0", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        pr_err("[audiocodec] headphone_direct_used type err!\n");
    }
	headphone_direct_used = val.val;
//	if (headphone_direct_used && (codec_chip_ver != MAGIC_VER_A31A)) {
//		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x3);
//		codec_wr_control(SUNXI_PA_CTRL, 0x1, HPCOM_PRO, 0x1);
//	} else {
		codec_wr_control(SUNXI_PA_CTRL, 0x3, HPCOM_CTL, 0x0);
		codec_wr_control(SUNXI_PA_CTRL, 0x1, HPCOM_PRO, 0x0);
//	}

	/*process for normal standby*/
//	if (NORMAL_STANDBY == standby_type) {
//	/*process for super standby*/
//	} else if(SUPER_STANDBY == standby_type) {
		/*when TX FIFO available room less than or equal N,
		* DRQ Requeest will be de-asserted.
		*/
		codec_wr_control(SUNXI_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
		/*write 1 to flush tx fifo*/
		codec_wr_control(SUNXI_DAC_FIFOC, 0x1, DAC_FIFO_FLUSH, 0x1);
		/*write 1 to flush rx fifo*/
		codec_wr_control(SUNXI_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
//	}

	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, FIR_VERSION, 0x1);
	pr_debug("[audio codec]:resume end\n");
	return 0;
}

/* power down chip */
static int sndpcm_soc_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndpcm = {
	.probe 	 =	sndpcm_soc_probe,
	.remove  =  sndpcm_soc_remove,
	.suspend = 	sndpcm_suspend,
	.resume  =  sndpcm_resume,
};

static int __init sndpcm_codec_probe(struct platform_device *pdev)
{
	baseaddr = ioremap(CODEC_BASSADDRESS, 0x50);
	if (baseaddr == NULL)
		return -ENXIO;
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL, "pll2");
	if ((!codec_pll2clk)||(IS_ERR(codec_pll2clk))) {
		pr_err("try to get codec_pll2clk failed!\n");
	}
	if (clk_prepare_enable(codec_pll2clk)) {
		pr_err("enable codec_pll2clk failed; \n");
	}
	/* codec_moduleclk */
	codec_moduleclk = clk_get(NULL, "adda");
	if ((!codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("try to get codec_moduleclk failed!\n");
	}
	if (clk_set_parent(codec_moduleclk, codec_pll2clk)) {
		pr_err("err:try to set parent of codec_moduleclk to codec_pll2clk failed!\n");
	}
	if (clk_set_rate(codec_moduleclk, 24576000)) {
		pr_err("err:set codec_moduleclk clock freq 24576000 failed!\n");
	}
	if (clk_prepare_enable(codec_moduleclk)) {
		pr_err("err:open codec_moduleclk failed; \n");
	}

	codec_init();
	
	/* check if hp_vcc_ldo exist, if exist enable it */
	type = script_get_item("audio0", "audio_hp_ldo", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		pr_err("script_get_item return type err, consider it no ldo\n");
	} else {
		if (!strcmp(item.str, "none"))
			hp_ldo = NULL;
		else {
			hp_ldo_str = item.str;
			hp_ldo = regulator_get(NULL, hp_ldo_str);
			if (!hp_ldo) {
				pr_err("get audio hp-vcc(%s) failed\n", hp_ldo_str);
				return -EFAULT;
			}
			regulator_set_voltage(hp_ldo, 3000000, 3000000);
			regulator_enable(hp_ldo);
		}
	}
	/*get the default pa val(close)*/
	type = script_get_item("audio0", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("script_get_item return type err\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(item.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("request gpio failed!\n");
	}
	gpio_direction_output(item.gpio.gpio, 1);
	
	snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndpcm, &sndpcm_dai, 1);
	return 0;
}

static int __exit sndpcm_codec_remove(struct platform_device *pdev)
{
	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_disable_unprepare(codec_moduleclk);
	}
	if ((NULL == codec_pll2clk)||(IS_ERR(codec_pll2clk))) {
		pr_err("codec_pll2clk handle is invaled, just return\n");
		return -EINVAL;
	} else {
		clk_put(codec_pll2clk);
	}

	/* disable audio hp-vcc ldo if it exist */
	if (hp_ldo) {
		regulator_disable(hp_ldo);
		hp_ldo = NULL;
	}

	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static void sunxi_codec_shutdown(struct platform_device *devptr)
{
	int i =0;
	u32 reg_val;
	int headphone_vol = 0;
	script_item_u val;
	script_item_value_type_e  type;

	item.gpio.data = 0;
	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
    	pr_err("[audiocodec] headphone_vol type err!\n");
    }
	headphone_vol = val.val;

	codec_wr_control(SUNXI_ADDAC_TUNE, 0x1, ZERO_CROSS_EN, 0x0);
	for (i = headphone_vol; i > 0 ; i--) {
		/*set HPVOL volume*/
		codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, i);
		reg_val = codec_rdreg(SUNXI_DAC_ACTL);
		reg_val &= 0x3f;
		if ((i%2==0))
			usleep_range(2000, 3000);
	}
	codec_wr_control(SUNXI_DAC_ACTL, 0x3f, VOLUME, 0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*disable dac_l and dac_r*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, DACAREN, 0x0);

	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1, HBIASEN, 0x0);
	codec_wr_control(SUNXI_MIC_CTRL, 0x1, HBIASADCEN, 0x0);
	/*mute l_pa and r_pa*/
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	msleep(100);
	codec_wr_control(SUNXI_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	msleep(20);

	codec_wr_control(SUNXI_MIC_CTRL, 0x1f, LINEOUT_VOL, 0x0);

	gpio_set_value(item.gpio.gpio, 0);

	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
	} else {
		clk_disable(codec_moduleclk);
	}
}

/*data relating*/
static struct platform_device sndpcm_codec_device = {
	.name = "sunxi-pcm-codec",
	.id = -1,
};

/*method relating*/
static struct platform_driver sndpcm_codec_driver = {
	.driver = {
		.name = "sunxi-pcm-codec",
		.owner = THIS_MODULE,
	},
	.probe = sndpcm_codec_probe,
	.remove = __exit_p(sndpcm_codec_remove),
	.shutdown = sunxi_codec_shutdown,
};

static int __init sndpcm_codec_init(void)
{
	int err = 0;

	if((err = platform_device_register(&sndpcm_codec_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sndpcm_codec_driver)) < 0)
		return err;
	
	return 0;
}
module_init(sndpcm_codec_init);

static void __exit sndpcm_codec_exit(void)
{
	platform_driver_unregister(&sndpcm_codec_driver);
}
module_exit(sndpcm_codec_exit);

MODULE_DESCRIPTION("SNDPCM ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm-codec");
