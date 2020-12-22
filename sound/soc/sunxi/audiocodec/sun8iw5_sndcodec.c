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
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <linux/power/scenelock.h>

#include "sunxi_codecdma.h"
#include "sun8iw5_sndcodec.h"

#define sndpcm_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndpcm_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
static struct regulator* hp_ldo = NULL;
static char *hp_ldo_str = NULL;

static int play_running = 0;
static int cap_running = 0;
static volatile int current_running = -1;

/*for pa gpio ctrl*/
static script_item_u item;
//static script_item_value_type_e  type;

/*for phone call flag*/
static bool codec_analog_phonein_en		   = false;
static bool codec_analog_mainmic_en    = false;
static bool codec_analog_phoneout_en          = false;
static bool codec_lineinin_en          = false;
static bool codec_lineincap_en         = false;
static bool codec_analog_headsetmic_en = false;
static bool codec_speakerout_en        = false;

static bool codec_earpieceout_en       = false;

static bool codec_voice_record_en      = false;
static bool codec_headphoneout_en      = false;
/*Digital_bb*/
static bool codec_digital_headsetmic_en = false;
static bool codec_digital_mainmic_en	= false;
static bool codec_digital_phoneout_en	= false;

static bool codec_digital_phonein_en 	= false;
static bool codec_digital_bb_clk_format_init = false;

/*bluetooth*/
static bool codec_bt_clk_format 		= false;
static bool codec_bt_out_en 			= false;
static bool bt_bb_button_voice 			= false;

static bool codec_analog_btmic_en 		= false;
static bool codec_analog_btphonein_en 	= false;

static bool codec_digital_btmic_en 		= false;
static bool codec_digital_btphonein_en 	= false;
static bool codec_digital_bb_bt_clk_format = false;

static bool codec_system_bt_capture_en = false;
static bool codec_analog_bb_capture_mic = false;
static int codec_speaker_headset_earpiece_en=0;

static int pa_vol 						= 0;
static int cap_vol 						= 0;
static int earpiece_vol 				= 0;
static int headphone_vol 				= 0;
static int pa_double_used 				= 0;
static int phone_main_mic_vol 			= 0;
static int headphone_direct_used 		= 0;
static int phone_headset_mic_vol 		= 0;
static int aif2_used 				= 0;
static int aif3_used 				= 0;
static int dac_vol_ctrl_spk			=0x9e9e;
static int dac_vol_ctrl_headphone			=0xa0a0;
static int agc_used 		= 0;
static int drc_used 		= 0;
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

static struct clk *codec_pll2clk,*codec_moduleclk,*codec_srcclk;

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
static int codec_wrreg_prcm_bits(unsigned short reg, unsigned int mask, unsigned int value)
{
	unsigned int old, new;

	old	=	read_prcm_wvalue(reg);
	new	=	(old & ~mask) | value;
	write_prcm_wvalue(reg,new);

	return 0;
}

static int codec_wr_prcm_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_prcm_bits(reg, mask, reg_val);
	return 0;
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
int codec_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
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
		(read_prcm_wvalue(reg)>>	shift) & mask;
	if (shift != rshift)
		ucontrol->value.integer.value[1] =
			(read_prcm_wvalue(reg) >> rshift) & mask;

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

	return codec_wrreg_prcm_bits(reg, val_mask, val);
}

/**
*	snd_codec_get_volsw_digital	-	single	mixer	get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control
*	return 0 for success.
*/
int snd_codec_get_volsw_digital(struct snd_kcontrol	*kcontrol,
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
*	snd_codec_put_volsw_digital	-	single	mixer put callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to put the value of a single mixer control
*
* return 0 for success.
*/
int snd_codec_put_volsw_digital(struct	snd_kcontrol	*kcontrol,
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



int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
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
static void codec_resume_events(struct work_struct *work)
{
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, HPPAEN, 0x1);
	//by xzd
	msleep(400);
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, HMICBIASEN, 0x1);
	pr_debug("====codec_resume_events===\n");
}
#endif
static void get_audio_param(void)
{
	script_item_value_type_e  type;
	script_item_u val;

	type = script_get_item("audio0", "headphone_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] headphone_vol type err!\n");
	}  else {
		headphone_vol = val.val;
	}

	type = script_get_item("audio0", "earpiece_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] earpiece_vol type err!\n");
	}  else {
		earpiece_vol = val.val;
	}

	type = script_get_item("audio0", "cap_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] cap_vol type err!\n");
	}  else {
		cap_vol = val.val;
	}

	type = script_get_item("audio0", "headset_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] headset_mic_vol type err!\n");
	}  else {
		phone_headset_mic_vol = val.val;
	}

	type = script_get_item("audio0", "main_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] main_mic_vol type err!\n");
	}  else {
		phone_main_mic_vol = val.val;
	}

	type = script_get_item("audio0", "pa_double_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] pa_double_used type err!\n");
	}  else {
		pa_double_used = val.val;
	}

	if (!pa_double_used) {
		type = script_get_item("audio0", "pa_single_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] pa_single_vol type err!\n");
		}  else {
			pa_vol = val.val;
		}
	} else {
		type = script_get_item("audio0", "pa_double_vol", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[audiocodec] pa_double_vol type err!\n");
		}  else {
			pa_vol = val.val;
		}
	}

	type = script_get_item("audio0", "DAC_VOL_CTRL_SPK", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] DAC_VOL_CTRL_SPK type err!\n");
	} else {
		dac_vol_ctrl_spk = val.val;
	}
	type = script_get_item("audio0", "DAC_VOL_CTRL_HEADPHONE", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] DAC_VOL_CTRL_HEADPHONE type err!\n");
	} else {
		dac_vol_ctrl_headphone = val.val;
	}

	type = script_get_item("audio0", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] headphone_direct_used type err!\n");
	} else {
		headphone_direct_used = val.val;
	}
	type = script_get_item("audio0", "agc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] agc_used type err!\n");
	} else {
		agc_used = val.val;
	}
	type = script_get_item("audio0", "drc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] drc_used type err!\n");
	} else {
		drc_used = val.val;
	}
	pr_debug("headphone_vol=0x%x, earpiece_vol=0x%x, cap_vol=0x%x, \
		phone_headset_mic_vol=0x%x, phone_main_mic_vol=0x%x, \
		pa_double_used=0x%x, pa_vol=0x%x \n" \
		,headphone_vol, earpiece_vol, cap_vol,  \
		phone_headset_mic_vol, phone_main_mic_vol, \
		pa_double_used, pa_vol);
}

/*
*	enable the codec function which should be enable during system init.
*/
static void codec_init(void)
{
	get_audio_param();
	if (headphone_direct_used) {
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x3);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x1);
	} else {
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x0);
	}
	if (drc_used){
		codec_wr_control(0x48c, 0x7ff, 0, 0x1);
//		codec_wr_control(0x48c, 0x7ff, 0, 0x0);
		codec_wr_control(0x490, 0xffff, 0, 0x2baf);
		//codec_wr_control(0x490, 0xffff, 0, 0x1fb6);
		codec_wr_control(0x494, 0x7ff, 0, 0x1);
		codec_wr_control(0x498, 0xffff, 0, 0x2baf);
		codec_wr_control(0x49c, 0x7ff, 0, 0x0);
		codec_wr_control(0x4a0, 0xffff, 0, 0x44a);
		codec_wr_control(0x4a4, 0x7ff, 0, 0x0);
		codec_wr_control(0x4a8, 0xffff, 0, 0x1e06);
		//codec_wr_control(0x4ac, 0x7ff, 0, 0x27d);
		codec_wr_control(0x4ac, 0x7ff, 0, 0x352);
		//codec_wr_control(0x4b0, 0xffff, 0, 0xcf68);
		codec_wr_control(0x4b0, 0xffff, 0, 0x6910);
		codec_wr_control(0x4b4, 0x7ff, 0, 0x77a);
		codec_wr_control(0x4b8, 0xffff, 0, 0xaaaa);
		//codec_wr_control(0x4bc, 0x7ff, 0, 0x1fe);
		codec_wr_control(0x4bc, 0x7ff, 0, 0x2de);
		codec_wr_control(0x4c0, 0xffff, 0, 0xc982);

		codec_wr_control(0x258, 0xffff, 0, 0x9f9f);
	}
	if (agc_used){
		codec_wr_control(0x4d0, 0x3, 6, 0x3);

		codec_wr_control(0x410, 0x3f, 8, 0x31);
		codec_wr_control(0x410, 0xff, 0, 0x28);

		codec_wr_control(0x414, 0x3f, 8, 0x31);
		codec_wr_control(0x414, 0xff, 0, 0x28);

		codec_wr_control(0x428, 0x7fff, 0, 0x24);
		codec_wr_control(0x42c, 0x7fff, 0, 0x2);
		codec_wr_control(0x430, 0x7fff, 0, 0x24);
		codec_wr_control(0x434, 0x7fff, 0, 0x2);
		codec_wr_control(0x438, 0x1f, 8, 0xf);
		codec_wr_control(0x438, 0x1f, 0, 0xf);
		codec_wr_control(0x44c, 0x7ff, 0, 0xfc);
		codec_wr_control(0x450, 0xffff, 0, 0xabb3);
	}
	/*mute headphone pa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
}

/*
*	the system voice come out from speaker
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_pa_play_open(void)
{

	int i = 0;
	int reg_val = 0;
	if(codec_speakerout_en != 1) {
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);
	}
	if (drc_used){
		codec_wr_control(0x4d4, 0xffff, 0, 0x80);
		codec_wr_control(0x210, 0x1, 6, 0x1);
		codec_wr_control(0x214, 0x1, 6, 0x1);
		codec_wr_control(0x480, 0x7, 0, 0x7);
	}
	/*enable AIF1 DAC Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x1);

	/*confige AIF1 DAC Timeslot0 left channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0L_SRC, 0);
	/*confige AIF1 DAC Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 0);

	/*confige dac digital mixer source */
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF1DA0L, 0x1);
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACR_MXR_SRC_AIF1DA0R, 0x1);

	codec_wr_control(SUNXI_DAC_VOL_CTRL, 0xffff, 0, dac_vol_ctrl_spk);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x1);
	codec_wr_prcm_control(HP_VOLC, 0x1, PA_CLK_GC, 0x0);

	/*enable dac analog*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);

	if (!pa_double_used) {/*single speaker*/
		//codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);
		//codec_wr_prcm_control(ROMIXSC, 0x1, LMIXMUTEDACR, 0x1);
		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
        codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x3);

		/*enable output mixer */
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

		/*pa input source select:l_mixer*/
	 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
	 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x1);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);
		/*unmute headphone pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	} else {/*double speaker*/
		if(codec_speakerout_en) {
			/*right output mixer source : dacr*/
			codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACR, 0x1);
			/*left output mixer source : dacl*/
			codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);
			/*enable output mixer */
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);

			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

			/*pa input source select:r_mixer,l_mixer*/
		 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);

		}else {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

			/*pa input source select:r_mixer,l_mixer*/
		 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

		}
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);

	}

	if(play_running == 1) {/*used for change the path*/
		pr_debug("%s,line:%d\n",__func__,__LINE__);
		reg_val = read_prcm_wvalue(HP_VOLC);
		reg_val &= 0x3f;
		if (!reg_val) {
			for(i=0; i < pa_vol; i++) {
				/*set HPVOL volume*/
				codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
				reg_val = read_prcm_wvalue(HP_VOLC);
				reg_val &= 0x3f;
				if ((i%2==0))
					usleep_range(1000,2000);
			}
		}
		usleep_range(2000, 3000);
		gpio_set_value(item.gpio.gpio, 1);
		//msleep(62);

	}
	return 0;
}

/*
*	the system voice come out from headphone
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_headphone_play_open(void)
{
	int i = 0;
	int reg_val =0;
	/*close spk pa*/
	gpio_set_value(item.gpio.gpio, 0);
	usleep_range(1000, 2000);

	codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);
	/*mute headphone pa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	if(codec_headphoneout_en != 1) {
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);
	}
	if (drc_used){
		codec_wr_control(0x4d4, 0xffff, 0, 0x0);
		codec_wr_control(0x210, 0x1, 6, 0x0);
		codec_wr_control(0x214, 0x1, 6, 0x0);
		codec_wr_control(0x480, 0x7, 0, 0x0);
	}
	/*enable AIF1 DAC Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x1);

	/*confige AIF1 DAC Timeslot0 left channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0L_SRC, 0);
	/*confige AIF1 DAC Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 0);

	/*confige dac digital mixer source */
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF1DA0L, 0x1);
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACR_MXR_SRC_AIF1DA0R, 0x1);

	codec_wr_control(SUNXI_DAC_VOL_CTRL, 0xffff, 0, dac_vol_ctrl_headphone);
	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x1);
	/*enable dac_l and dac_r*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);

	if(codec_headphoneout_en == 1) {
		//codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACR, 0x1);
		//codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);
		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x2);
		codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x2);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);
	}else{
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

	}

	/*hpout negative mute*/
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

	if(play_running == 1) {/*used for change the path*/
		pr_debug("%s,line:%d\n",__func__,__LINE__);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
		reg_val = read_prcm_wvalue(HP_VOLC);
		reg_val &= 0x3f;
		if (!reg_val) {
			for(i=0; i < headphone_vol; i++) {
			/*set HPVOL volume*/
			codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
			reg_val = read_prcm_wvalue(HP_VOLC);
			reg_val &= 0x3f;
			if ((i%2==0))
				usleep_range(1000,2000);
			}
		}

	}

	return 0;
}

static int codec_earpiece_play_open(void)
{

	gpio_set_value(item.gpio.gpio, 0);
	usleep_range(2000, 3000);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

	/*enable AIF1 DAC Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x1);

	/*confige AIF1 DAC Timeslot0 left channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0L_SRC, 0);
	/*confige AIF1 DAC Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 0);

	/*confige dac digital mixer source */
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0xf, DACL_MXR_SRC, 0x8);
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0xf, DACR_MXR_SRC, 0x8);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x1);
	/*0*/
	codec_wr_prcm_control(HP_VOLC, 0x1, PA_CLK_GC, 0x0);
	/*enable dac ananlog*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);
	/*left output negative*/
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x1);
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
	/*slect output mixer source*/
	codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
	codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x3);

	codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x1);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);

	return 0;
}

/*
*	the system voice come out from headphone and speaker
*	while the phone call in, the phone use the headset, you can hear the voice from speaker and headset.
* 	this function just used for the system voice(such as music and moive voice and so on).
*/
static int codec_pa_and_headset_play_open(void)
{

	/*mute hppa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

	/*enable AIF1 DAC Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x1);

	/*confige AIF1 DAC Timeslot0 left channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0L_SRC, 0);
	/*confige AIF1 DAC Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 0);
	codec_wr_control(SUNXI_DAC_VOL_CTRL, 0xffff, 0, dac_vol_ctrl_spk);
	/*confige dac digital mixer source */
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0xf, DACL_MXR_SRC, 0x8);
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0xf, DACR_MXR_SRC, 0x8);

	/*enable dac digital*/
	codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x1);

	/*enble dac analog*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);

	codec_wr_prcm_control(HP_VOLC, 0x1, PA_CLK_GC, 0x0);

	if (!pa_double_used) {/*single speaker*/
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x1);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

	 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
	 	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);

		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x3);

		/*unmute headphone pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	} else {/*double speaker*/
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);

		/*right output mixer source : dacr*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACR, 0x1);
		/*left output mixer source : dacl*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
	}
	return 0;
}

static int codec_system_btout_open(void)
{
		/*config clk fmt*/
	/*config aif2 from pll2*/
	codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, AIF2CLK_ENA, 0x1);
	/*aif2 clk source select*/
	codec_wr_control(SUNXI_SYSCLK_CTL, 0x3, AIF2CLK_SRC, 0x3);

	codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF2_MOD_CLK_EN, 0x1);
	codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF2_MOD_RST_CTL, 0x1);

	/*config aif2 fmt :pcm mono 16 lrck=8k,blck/lrck = 64*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_MSTR_MOD, 0x0);/*master*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_BCLK_INV, 0x0);/*bclk:normal*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_LRCK_INV, 0x0);/*lrck:normal*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0xf, AIF2_BCLK_DIV, 0x9);/*aif2/bclk=48*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x7, AIF2_LRCK_DIV, 0x2);/*bclk/lrck=64*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x3, AIF2_WORD_SIZ, 0x1);/*sr=16*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x3, AIF2_DATA_FMT, 0x3);/*dsp mode*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_MONO_PCM, 1);/*dsp mode*/

	/*aif3 mode enable*/
	codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF3_MOD_CLK_EN, 0x1);
	codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF3_MOD_RST_CTL, 0x1);

	/*config aif3: clk ,fmt*/
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x1, AIF3_BCLK_INV, 0x0);
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x1, AIF3_LRCK_INV, 0x0);

	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x3, AIF3_WORD_SIZ, 0x1);/*sr = 16*/
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x3, AIF3_CLOC_SRC, 0x1);/*clk form aif2*/
	/*enable AIF1 DAC Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x1);
	/*confige AIF1 DAC Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 3);
	codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCR_MXR_SRC_AIF1DA0R, 1);

	/*enable aif2 adcl channel*/
	codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x1, AIF2_ADCR_EN, 1);
	/*select aif3 pcm output source*/
	codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF3_ADC_SRC, 2);
	return 0;
}

static int codec_system_bt_buttonvoice_open(void)
{
	/*enable AIF1 DAC Timeslot0  channel enable*/
	//codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x1);
	/*confige AIF1 DAC Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 0);
	codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCR_MXR_SRC_AIF1DA0R, 1);
	return 0;
}

/*
*	use for the base system record(for pad record).
*/
static int codec_capture_open(void)
{
	if (agc_used) {
		codec_wr_control(0x210, 0x1, 7, 0x1);
		codec_wr_control(0x214, 0x1, 7, 0x1);
		codec_wr_control(0x408, 0xf, 0, 0x6);
		codec_wr_control(0x408, 0x7, 12, 0x7);
		codec_wr_control(0x40c, 0xf, 0, 0x6);
		codec_wr_control(0x40c, 0x7, 12, 0x7);
	}
	/*enable AIF1 adc Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x1, AIF1_AD0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x1, AIF1_AD0R_ENA, 0x1);

	/*confige AIF1 adc Timeslot0 left channel data source*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x3, AIF1_AD0L_SRC, 0);
	/*confige AIF1 adc Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x3, AIF1_AD0R_SRC, 0);

	/*confige AIF1 Digital Mixer Source*/
	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0x1, AIF1_AD0L_MXL_SRC_ADCL, 0x1);
	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0x1, AIF1_AD0R_MXR_SRC_ADCR, 0x1);

	/*just for test*/
//	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xf, AIF1_AD0L_MXL_SRC, 0x8);
//	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xf, AIF1_AD0R_MXR_SRC, 0x8);

	/*enable ADC Digital part*/
	codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 1);
	/*enable Digital microphone*/
	codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENDM, 0);

	/*ADC Delay Time*/
	codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x3, ADOUT_DTS, 3);
	codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ADOUT_DLY, 1);

	/*enable adc_r adc_l analog*/
	codec_wr_prcm_control(ADC_AP_EN, 0x1,  ADCREN, 0x1);
	codec_wr_prcm_control(ADC_AP_EN, 0x1,  ADCLEN, 0x1);

	/*enable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	/*enable Right MIC1 Boost stage*/
	codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);

	/*enable mic1 pa*/
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x7, MIC1BOOST, cap_vol);
	/*enable Master microphone bias*/
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x1);
	cap_running = 1;
	/*hardware fifo delay*/
	msleep(200);
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
	int ret = 0;

	codec_speaker_headset_earpiece_en = ucontrol->value.integer.value[0];

	if (codec_speaker_headset_earpiece_en == 1) {
		ret = codec_pa_play_open();
	} else if (codec_speaker_headset_earpiece_en == 0) {
		ret = codec_headphone_play_open();
	} else if (codec_speaker_headset_earpiece_en == 2) {
		ret = codec_pa_and_headset_play_open();
	} else if (codec_speaker_headset_earpiece_en == 3) {
		ret = codec_earpiece_play_open();
	}else if(codec_speaker_headset_earpiece_en == 4) {
		pr_debug("%s,line:%d\n",__func__,__LINE__);
		ret = codec_system_btout_open();
	}else if(codec_speaker_headset_earpiece_en == 5){
		ret = codec_system_bt_buttonvoice_open();

	}
	return 0;
}

static int codec_get_spk_headset_earpiece(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_headset_earpiece_en;
	return 0;
}

static int sndpcm_unmute(struct snd_soc_dai *dai, int mute)
{
	if (current_running == 0) {/*play stream*/
		int i = 0;
		int reg_val = 0;
		if(codec_analog_phonein_en){
			msleep(10);
			codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x1);
			codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);
		}
		if(mute == 0) {
			switch (codec_speaker_headset_earpiece_en) {
			case 0:
				pr_debug("%s,line:%d\n",__func__,__LINE__);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
				reg_val = read_prcm_wvalue(HP_VOLC);
				reg_val &= 0x3f;
				if (!reg_val) {
					for(i=0; i < headphone_vol; i++) {
						/*set HPVOL volume*/
						codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
						reg_val = read_prcm_wvalue(HP_VOLC);
						reg_val &= 0x3f;
						if ((i%2==0))
							usleep_range(1000,2000);
					}
				}
				break;
			case 1:
				pr_debug("%s,line:%d\n",__func__,__LINE__);
				reg_val = read_prcm_wvalue(HP_VOLC);
				reg_val &= 0x3f;
				if (!reg_val) {
					for(i=0; i < pa_vol; i++) {
					/*set HPVOL volume*/
					codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
					reg_val = read_prcm_wvalue(HP_VOLC);
					reg_val &= 0x3f;
					if ((i%2==0))
						usleep_range(1000,2000);
					}
				}
				usleep_range(2000, 3000);
				gpio_set_value(item.gpio.gpio, 1);
				msleep(62);
				break;
			case 2:
				pr_debug("%s,line:%d\n",__func__,__LINE__);
				if (!pa_double_used) {/*single speaker*/
					codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
					codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

				} else {/*double speaker*/
					codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
					codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
				}
				reg_val = read_prcm_wvalue(HP_VOLC);
				reg_val &= 0x3f;
				if (!reg_val) {
					for(i=0; i < pa_vol; i++) {
						/*set HPVOL volume*/
						codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
						reg_val = read_prcm_wvalue(HP_VOLC);
						reg_val &= 0x3f;
						if ((i%2==0))
							usleep_range(1000,2000);
					}
				}
				/*set HPVOL volume*/
				codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, pa_vol);

				usleep_range(2000, 3000);
				gpio_set_value(item.gpio.gpio, 1);
				msleep(62);
				break;
			case 3:
				pr_debug("%s,line:%d\n",__func__,__LINE__);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
				reg_val = read_prcm_wvalue(HP_VOLC);
				reg_val &= 0x3f;
				if (!reg_val) {
					for(i=0; i < earpiece_vol; i++) {
						/*set HPVOL volume*/
						codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
						reg_val = read_prcm_wvalue(HP_VOLC);
						reg_val &= 0x3f;
						if ((i%2==0))
							usleep_range(1000,2000);
					}
				}
				/*set HPVOL volume*/
				codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, earpiece_vol);
				break;
			default:
				break;

			}
		} else {
		}

	}
	return 0;
}

static int sndpcm_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	/*enble aif1 clk */
	codec_wr_control(SUNXI_SYSCLK_CTL ,  0x1, AIF1CLK_ENA, 0x1);
	/*enble sys clk */
	codec_wr_control(SUNXI_SYSCLK_CTL ,  0x1, SYSCLK_ENA, 0x1);
	/**for test loopback******/
	//codec_wr_control(SUNXI_DA_CTL ,  0x1, 3, 0x1);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_running = 1;
		current_running = 0;
		/*enable module AIF1,DAC*/
		codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, AIF1_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, DAC_DIGITAL_MOD_CLK_EN, 0x1);

		/*reset module AIF1, DAC*/
		codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, AIF1_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, DAC_DIGITAL_MOD_RST_CTL, 0x1);
	} else {
		current_running = 1;
		/*enable module AIF1,ADC*/
		codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, AIF1_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, ADC_DIGITAL_MOD_CLK_EN, 0x1);

		/*reset module AIF1, ADC*/
		codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, AIF1_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, ADC_DIGITAL_MOD_RST_CTL, 0x1);

	}
	/*global enable*/
	codec_wr_control(SUNXI_DA_CTL ,  0x1, GEN, 0x1);
	return 0;
}

static void sndpcm_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	int i = 0;
	int cur_vol = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);
		cur_vol = read_prcm_wvalue(HP_VOLC);
		cur_vol &= 0x3f;
		if (!(codec_analog_mainmic_en || codec_analog_headsetmic_en ||codec_digital_headsetmic_en ||
					codec_digital_mainmic_en ||codec_analog_btmic_en || codec_digital_btmic_en)){
			gpio_set_value(item.gpio.gpio, 0);

			if (cur_vol > 48) {
				for (i = cur_vol; i > 52 ; i--) {
					/*set HPVOL volume*/
					codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
					usleep_range(9000, 10000);
				}
				for (i = 52; i > 48 ; i--) {
					/*set HPVOL volume*/
					codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
					usleep_range(6000,7000);
				}
			}
			for (i = 48; i > 32 ; i--) {
				/*set HPVOL volume*/
				codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
				usleep_range(1000,2000);
			}
			for (i = 32; i > 0 ; i--) {
				/*set HPVOL volume*/
				codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
				usleep_range(100,200);
			}
			codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);

			/*disable analog part*/
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

			 /*by xzd left select dacl&dacr*/
			codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
			/*by xzd right don't select src*/
			codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x0);

			/*enable output mixer */
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
			/*by xzd only right negative left*/
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

			/*enable dac analog*/
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);

			/*disable dac digital*/
			codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x0);
			codec_wr_prcm_control(HP_VOLC, 0x1, PA_CLK_GC, 0x0);

			/*confige dac digital mixer source */
			codec_wr_control(SUNXI_DAC_MXR_SRC, 0xf, DACL_MXR_SRC, 0x0);
			codec_wr_control(SUNXI_DAC_MXR_SRC, 0xf, DACR_MXR_SRC, 0x0);

			/*confige AIF1 DAC Timeslot0 left channel data source*/
			codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0L_SRC, 0);
			/*confige AIF1 DAC Timeslot0 right channel data source*/
			codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0R_SRC, 0);

			/*disable AIF1 DAC Timeslot0  channel*/
			codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x0);
			codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0R_ENA, 0x0);

			/*diable clk parts*/
			if (1 == cap_running ){
				pr_debug("[audio stream] capture is running!!!!\n");
			}else{
				/*disable aif1clk*/
				codec_wr_control(SUNXI_SYSCLK_CTL ,  0x1, AIF1CLK_ENA, 0);
				/*disble sys clk */
				codec_wr_control(SUNXI_SYSCLK_CTL ,  0x1, SYSCLK_ENA, 0);

				/*disable module AIF1*/
				codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, AIF1_MOD_CLK_EN, 0x0);

				/*reset module AIF1*/
				codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, AIF1_MOD_RST_CTL, 0x0);

				/*global disable*/
				codec_wr_control(SUNXI_DA_CTL ,  0x1, GEN, 0x0);
			}
				/*disable module DAC*/
				codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, DAC_DIGITAL_MOD_CLK_EN, 0x0);

				/*reset module DAC*/
				codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, DAC_DIGITAL_MOD_RST_CTL, 0x0);

		/* I2S0 TX DISABLE */
		codec_wr_control(SUNXI_DA_CTL ,  0x1, TXEN,0);
		/*SDO ON*/
		codec_wr_control(SUNXI_DA_CTL ,  0x1, SDO_EN, 0);

		}
		if (drc_used){
			codec_wr_control(0x4d4, 0xffff, 0, 0x0);
			codec_wr_control(0x210, 0x1, 6, 0x0);
			codec_wr_control(0x214, 0x1, 6, 0x0);
			codec_wr_control(0x480, 0x7, 0, 0x0);
		}
		play_running = 0;
	}else{

			if (!(codec_analog_mainmic_en || codec_analog_headsetmic_en ||codec_digital_headsetmic_en ||
					codec_digital_mainmic_en ||codec_analog_btmic_en || codec_digital_btmic_en)){

				codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0x0);
				/*disable Master microphone bias*/
				codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x0);
				codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0x1, MIC2AMPEN, 0x0);

				/*disable Left MIC1 Boost stage*/
				codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
				/*disable Right MIC1 Boost stage*/
				codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);

				/*disable Left MIC1 Boost stage*/
				codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
				/*disable Right MIC1 Boost stage*/
				codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);

				/*disable PHONEP-PHONEN Boost stage*/
				codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEPHONEPN, 0x0);
				/*disable PHONEP-PHONEN Boost stage*/
				codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTEPHONEPN, 0x0);

				/*disable LINEINL ADC*/
				codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTELINEINL, 0x0);
				/*disable LINEINR ADC*/
				codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTELINEINR, 0x0);

				/*disable adc_r adc_l analog*/
				codec_wr_prcm_control(ADC_AP_EN, 0x1,  ADCREN, 0x0);
				codec_wr_prcm_control(ADC_AP_EN, 0x1,  ADCLEN, 0x0);

				/*disable AIF1 adc Timeslot0  channel*/
				codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x1, AIF1_AD0L_ENA, 0x0);
				codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x1, AIF1_AD0R_ENA, 0x0);

				/*confige AIF1 Digital Mixer Source*/
				codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xf, AIF1_AD0L_MXL_SRC, 0x0);
				codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xf, AIF1_AD0R_MXR_SRC, 0x0);
				/*disable ADC Digital part*/
				codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 0);
				/*diable clk parts*/
				if (play_running == 1 ) {
					pr_debug("[audio stream]playback is running!!!!\n");
				} else {
					/*disable aif1clk*/
					codec_wr_control(SUNXI_SYSCLK_CTL ,  0x1, AIF1CLK_ENA, 0);
					/*disble sys clk */
					codec_wr_control(SUNXI_SYSCLK_CTL ,  0x1, SYSCLK_ENA, 0);
					/*disable module AIF1*/
					codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, AIF1_MOD_CLK_EN, 0x0);
					/*reset module AIF1*/
					codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, AIF1_MOD_RST_CTL, 0x0);

					/*global disable*/
					codec_wr_control(SUNXI_DA_CTL ,  0x1, GEN, 0x0);
				}

				/*disable module ADC*/
				codec_wr_control(SUNXI_MOD_CLK_ENA ,  0x1, ADC_DIGITAL_MOD_CLK_EN, 0x0);

				/*reset module  ADC*/
				codec_wr_control(SUNXI_MOD_RST_CTL ,  0x1, ADC_DIGITAL_MOD_RST_CTL, 0x0);

			}
		/* I2S0 RX DISABLE */
		codec_wr_control(SUNXI_DA_CTL ,  0x1, RXEN, 0);
		if (agc_used) {
			codec_wr_control(0x210, 0x1, 7, 0x0);
			codec_wr_control(0x214, 0x1, 7, 0x0);
			codec_wr_control(0x408, 0xf, 0, 0x0);
			codec_wr_control(0x408, 0x7, 12, 0x0);
			codec_wr_control(0x40c, 0xf, 0, 0x0);
			codec_wr_control(0x40c, 0x7, 12, 0x0);
		}
		cap_running = 0;
	}


}

static int sndpcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}


/*
*	phone record from main mic + phone in(digital bb) .
* 	or
*	phone record from sub mic + phone in(digital bb) .
*	mic1 uses as main mic. mic2 uses as sub mic
*/
static int codec_digital_voice_mic_bb_capture_open(void)
{
	/*select phonein source */
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_AIF2DACL, 1);
	/*select mic source*/
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_ADCL, 1);
	/*aif1 adc left channel source select */
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0L_SRC, 0);

	/*aif1 adc left channel enable */
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x1, AIF1_AD0L_ENA, 1);
	msleep(200);
	return 0;
}

static int codec_digital_voice_bb_bt_capture_open(void)
{
	/*select AIF1 input  mixer source */
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_AIF2DACR, 1);
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_AIF2DACL, 1);

	/*AIF1 ADC Timeslot0 left channel data source select*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0L_SRC, 0);
	/*open ADC channel slot0 switch*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x1, AIF1_AD0L_ENA, 1);
	msleep(200);
	return 0;
}

static int codec_analog_voice_bb_bt_capture_open(void)
{
	/*select AIF1 input  mixer source */
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0R_MXR_SRC_AIF2DACL, 1);
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0R_MXR_SRC_ADCR, 1);

	/*AIF1 ADC Timeslot0 left channel data source select*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0L_SRC, 1);
	/*open ADC channel slot0 switch*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x1, AIF1_AD0L_ENA, 1);
	msleep(200);
	return 0;
}

/*
*	use for phone record from main/headset mic + phone in.
*	.
*/
static int codec_analog_voice_capture_open(void)
{
	if (codec_analog_mainmic_en) {
		/*select mic source*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	} else {
		/*select mic2 source*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
	}
	/*select phonein source*/
	codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEPHONEPN, 0x1);
	/*enable adc analog*/
	codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0x1);

	/*enable dac digital*/
	codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENAD, 1);
	codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENDM, 0);

	/*select aif1 adc left channel mixer source */
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_ADCL,1);

	/*select aif1 adc left channel source */
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0L_SRC,0);
	/*enable aif1 adc left channel */
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x1, AIF1_AD0L_ENA,1);
	msleep(200);

	return 0;
}

/*
*	use for the line_in record
*/
static int codec_voice_linein_capture_open(void)
{
	/*enable AIF1 adc Timeslot0  channel enable*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x1, AIF1_AD0L_ENA, 0x1);
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x1, AIF1_AD0R_ENA, 0x1);

	/*confige AIF1 adc Timeslot0 left channel data source*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x3, AIF1_AD0L_SRC, 0);
	/*confige AIF1 adc Timeslot0 right channel data source*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0x3, AIF1_AD0R_SRC, 0);

	/*confige AIF1 Digital Mixer Source*/
	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xf, AIF1_AD0L_MXL_SRC, 0x2);
	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xf, AIF1_AD0R_MXR_SRC, 0x2);

	/*enable dac digital*/
	codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENAD, 1);
	codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENDM, 0);

	/*enable adc_r adc_l analog*/
	codec_wr_prcm_control(ADC_AP_EN, 0x1,  ADCREN, 0x1);
	codec_wr_prcm_control(ADC_AP_EN, 0x1,  ADCLEN, 0x1);

	/*enable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTELINEINR, 0x1);
	/*enable Left MIC2 Boost stage*/
	codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTELINEINL, 0x1);

	msleep(200);

	return 0;
}


 static int codec_system_bt_capture_open(void)
{
	/*config clk fmt*/
	/*config aif2 from pll2*/
	codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, AIF2CLK_ENA, 0x1);
	/*aif2 clk source select*/
	codec_wr_control(SUNXI_SYSCLK_CTL, 0x3, AIF2CLK_SRC, 0x3);

	codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF2_MOD_CLK_EN, 0x1);
	codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF2_MOD_RST_CTL, 0x1);

	/*config aif2 fmt :pcm mono 16 lrck=8k,blck/lrck = 64*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_MSTR_MOD, 0x0);/*master*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_BCLK_INV, 0x0);/*bclk:normal*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_LRCK_INV, 0x0);/*lrck:normal*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0xf, AIF2_BCLK_DIV, 0x9);/*aif2/bclk=48*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x7, AIF2_LRCK_DIV, 0x2);/*bclk/lrck=64*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x3, AIF2_WORD_SIZ, 0x1);/*sr=16*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x3, AIF2_DATA_FMT, 0x3);/*dsp mode*/
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0x1, AIF2_MONO_PCM, 1);/*dsp mode*/

	/*aif3 mode enable*/
	codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF3_MOD_CLK_EN, 0x1);
	codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF3_MOD_RST_CTL, 0x1);

	/*config aif3: clk ,fmt*/
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x1, AIF3_BCLK_INV, 0x0);
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x1, AIF3_LRCK_INV, 0x0);

	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x3, AIF3_WORD_SIZ, 0x1);/*sr = 16*/
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0x3, AIF3_CLOC_SRC, 0x1);/*clk form aif2*/

	/*select aif2 dac input source*/
	codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC, 1);

	/*aif2 adc right channel enable*/
	codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x1, AIF2_ADCR_EN,1);
	/*aif2 adc right channel enable*/
	//codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC_AIF2DACR,1);

	/*select bt source*/
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_AIF2DACL, 1);
	/*aif1 adc left channel source select */
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0L_SRC, 0);

	/*aif1 adc left channel enable */
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x1, AIF1_AD0L_ENA, 1);
	pr_debug("%s,line:%d,SUNXI_AIF3_DACDAT_CTRL:%x\n",__func__,__LINE__,codec_rdreg(SUNXI_AIF3_DACDAT_CTRL));

	return 0;
}




static int sndpcm_perpare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	int play_ret = 0, capture_ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;

	/*confige the sample for adc,dac*/
	switch (substream->runtime->rate) {
		case 8000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x0);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x0);
			break;
		case 11025:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x1);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x1);
			break;
		case 12000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x2);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x2);
			break;
		case 16000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x3);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x3);
			break;
		case 22050:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x4);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x4);
			break;
		case 24000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x5);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x5);
			break;
		case 32000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x6);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x6);
			break;
		case 44100:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x7);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x7);
			break;
		case 48000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x8);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x8);
			break;
		case 96000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x9);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x9);
			break;
		case 192000:
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0xa);
			codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0xa);
			break;
		default:
			pr_err("AUDIO SAMPLE IS WRONG:There is no suitable sampling rate!!!\n");
			break;
		}
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (runtime->status->state == SNDRV_PCM_STATE_XRUN) {
			if (!(codec_analog_mainmic_en || codec_analog_headsetmic_en ||codec_digital_headsetmic_en ||
					codec_digital_mainmic_en ||codec_analog_btmic_en || codec_digital_btmic_en)){
				pr_err("%s,-------playback: SNDRV_PCM_STATE_XRUN---------line:%d\n",__func__,__LINE__);
				if (codec_speaker_headset_earpiece_en == 2) {
					play_ret = codec_pa_and_headset_play_open();
				} else if (codec_speaker_headset_earpiece_en == 1) {
					play_ret = codec_pa_play_open();
				} else if (codec_speaker_headset_earpiece_en == 3) {
					play_ret = codec_earpiece_play_open();
				} else if (codec_speaker_headset_earpiece_en == 0){
					play_ret = codec_headphone_play_open();
				}else if(codec_speaker_headset_earpiece_en == 4){
					play_ret = codec_system_btout_open();
				}
				play_running = 1;
			}
		}
		//play_running = 1;
		return play_ret;
	} else {
		if (runtime->status->state == SNDRV_PCM_STATE_XRUN) {
			pr_err("%s,-------capture: SNDRV_PCM_STATE_XRUN---------line:%d\n",__func__,__LINE__);
		}

		if (codec_voice_record_en && (codec_digital_mainmic_en ||codec_digital_headsetmic_en)) {
			capture_ret = codec_digital_voice_mic_bb_capture_open();
		} else if (codec_voice_record_en && codec_digital_btmic_en) {
			pr_debug("%s,line:%d\n",__func__,__LINE__);
			capture_ret = codec_digital_voice_bb_bt_capture_open();
		} else if (codec_voice_record_en && codec_analog_btmic_en) {
			capture_ret = codec_analog_voice_bb_bt_capture_open();
		} else if (codec_voice_record_en && ( codec_analog_mainmic_en ||codec_analog_headsetmic_en )) {
			capture_ret = codec_analog_voice_capture_open();
		} else if (codec_lineinin_en && codec_lineincap_en) {
			capture_ret = codec_voice_linein_capture_open();
		} else if (codec_voice_record_en && codec_system_bt_capture_en) {
			pr_debug("%s,line:%d\n",__func__,__LINE__);
			capture_ret = codec_system_bt_capture_open();
		} else if (!codec_voice_record_en) {
			capture_ret = codec_capture_open();
		}
		return capture_ret;
	}
}


int sunxi_i2s_set_rate(int freq) {
	if (clk_set_rate(codec_pll2clk, freq)) {
		pr_err("set codec_pll2clk rate fail\n");
	}
	if (clk_set_rate(codec_moduleclk, freq)) {
		pr_err("set codec_moduleclk rate fail\n");
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_i2s_set_rate);


static const char *spk_headset_earpiece_function[] = {"headset", "spk", "spk_headset", "earpiece","btout","bt_button_voice"};
static const struct soc_enum spk_headset_earpiece_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_headset_earpiece_function), spk_headset_earpiece_function),
};


/*
*	codec_analog_phoneout_en == 1, the phone out is open. receiver can hear the voice which you say.
*	codec_analog_phoneout_en == 0,	the phone out is close.
*/
static int codec_analog_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_analog_phoneout_en = ucontrol->value.integer.value[0];
	if (codec_analog_phoneout_en ) {
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUT_EN, 0x1);
	} else {
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUT_EN, 0x0);
	}
	return 0;
}

static int codec_analog_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_phoneout_en;
	return 0;
}


/*
*	codec_analog_phonein_en == 1, the phone in is open.
*	while you open one of the device(speaker,earpiece,headphone).
*	you can hear the caller's voice.
*	codec_analog_phonein_en == 0. the phone in is close.
*/
static int codec_analog_set_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_analog_phonein_en = ucontrol->value.integer.value[0];
	if (codec_analog_phonein_en) {
		pr_debug("%s,line:%d\n",__func__,__LINE__);
				/*enable AIF1 DAC Timeslot0  channel enable*/
		codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x1, AIF1_DA0L_ENA, 0x1);
		/*confige AIF1 DAC Timeslot0 left channel data source*/
		codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0x3, AIF1_DA0L_SRC, 0);
		/*confige dac digital mixer source */
		codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF1DA0L, 0x1);
			/*enable dac digital*/
		codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x1);
		/*enable dac_l and dac_r*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
		//codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x1);
		//codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);
		/*select PHONEP-PHONEN*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEPHONEPN, 0x1);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEPHONEPN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);

	} else {
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEPHONEPN, 0x0);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEPHONEPN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

	}

	return 0;
}

static int codec_analog_get_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_phonein_en;
	return 0;
}


/*
*	codec_earpieceout_en == 1, open the earpiece.
*	codec_earpieceout_en == 0, close the earpiece.
*/
static int codec_set_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int i =0;
	int reg_val = 0;
	codec_earpieceout_en = ucontrol->value.integer.value[0];
	if (codec_earpieceout_en) {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

		gpio_set_value(item.gpio.gpio, 0);

		/*mute l_pa and r_pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);

		/*open earpiece out routeway*/
		/*unmute l_pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		/*select the analog mixer input source*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);
		/*select HPL inverting output*/
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x1);
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x1);

		for(i=0; i < headphone_vol; i++) {
			/*set HPVOL volume*/
			codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
			reg_val = read_prcm_wvalue(HP_VOLC);
			reg_val &= 0x3f;
			if ((i%2==0))
				usleep_range(1000,2000);
		}

		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, earpiece_vol);

		codec_speakerout_en = 0;
		codec_headphoneout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
		if (headphone_direct_used) {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x3);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x1);
		} else {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x0);
		}
		//codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);
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
*	codec_speakerout_en == 1, open the speaker.
*	codec_speakerout_en == 0, close the speaker.
*/
static int codec_set_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_speakerout_en = ucontrol->value.integer.value[0];
	if (codec_speakerout_en) {
		//gpio_set_value(item.gpio.gpio, 0);
		/*close headphone and earpiece out routeway*/
		//codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		//codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
		//codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, pa_vol);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x1);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);


		if (headphone_direct_used) {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x3);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x1);
		} else {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x0);
		}
		usleep_range(2000, 3000);
		gpio_set_value(item.gpio.gpio, 1);
		msleep(62);
		codec_headphoneout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		gpio_set_value(item.gpio.gpio, 0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0);


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
*	codec_headphoneout_en == 1, open the headphone.
*	codec_headphoneout_en == 0, close the headphone.
*/
static int codec_set_headphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_headphoneout_en = ucontrol->value.integer.value[0];
	if (codec_headphoneout_en) {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);

		gpio_set_value(item.gpio.gpio, 0);
		//msleep(62);
		if (headphone_direct_used) {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x3);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x1);
		} else {
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
			codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x0);
		}
		/*select HPL inverting output*/
		//codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, LTRNMUTE, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, RTLNMUTE, 0x0);

		/*select the analog mixer input source*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x1);

		/*unmute l_pa and r_pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
		/*set HPVOL volume*/
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, headphone_vol);

		codec_speakerout_en = 0;
		codec_earpieceout_en = 0;
	} else {
		/*mute l_pa and r_pa*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
		/*select the default dac input source*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
		codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);
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
*	codec_analog_mainmic_en == 1, open mic1.
*	codec_analog_mainmic_en == 0, close mic1.
*/
static int codec_analog_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_analog_mainmic_en = ucontrol->value.integer.value[0];
	if (codec_analog_mainmic_en) {
		/*close headset mic(mic2) routeway*/
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_prcm_control(PHONEOUT_CTRL, 0xf, PHONEOUTS0, 0x0);
		/*open main mic(mic1) routeway*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL,0x7, MIC1BOOST, phone_main_mic_vol);
		/*enable Master microphone bias*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x1);
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUTS3, 0x1);
		codec_analog_headsetmic_en = 0;
	} else {
		/*disable mic pa*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x0);
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUTS3, 0x0);

	}

	return 0;
}

static int codec_analog_get_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_mainmic_en;
	return 0;
}

/*
*	codec_analog_headsetmic_en == 1, open mic2.
*	codec_analog_headsetmic_en == 0, close mic2.
*/
static int codec_analog_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_analog_headsetmic_en = ucontrol->value.integer.value[0];
	if (codec_analog_headsetmic_en) {
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0x0);
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x0);
		codec_wr_prcm_control(PHONEOUT_CTRL, 0xf, PHONEOUTS0, 0x0);

		/*open headset mic(mic2) routeway*/
		/*enable mic2 pa*/
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0x1, MIC2AMPEN, 0x1);
		/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL,0x7,MIC2BOOST, phone_headset_mic_vol);

		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC2_SS, 0x1);
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUTS2, 0x1);

		codec_analog_mainmic_en = 0;
	} else {
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUTS2, 0x0);
	}
	return 0;
}

static int codec_analog_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_headsetmic_en;
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
*	close all phone routeway
*/
static int codec_set_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int cur_vol =0;
	int i =0;
	gpio_set_value(item.gpio.gpio, 0);
	msleep(50);
	cur_vol = read_prcm_wvalue(HP_VOLC);
	cur_vol &= 0x3f;

	if (cur_vol > 48) {
		for (i = cur_vol; i > 52 ; i--) {
		/*set HPVOL volume*/
			codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
			usleep_range(9000, 10000);
		}
		for (i = 52; i > 48 ; i--) {
			/*set HPVOL volume*/
			codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
			usleep_range(6000,7000);
		}
	}
	for (i = 48; i > 32 ; i--) {
		/*set HPVOL volume*/
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
		usleep_range(1000,2000);
	}
	for (i = 32; i > 0 ; i--) {
		/*set HPVOL volume*/
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, i);
		usleep_range(100,200);
	}
	codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);
	/*close analog parts*/
	codec_wr_prcm_control(DAC_PA_SRC, 0xff, LHPIS, 0x0);
	codec_wr_prcm_control(LOMIXSC, 0xff, LMIXMUTE, 0x0);
	codec_wr_prcm_control(ROMIXSC, 0xff, RMIXMUTE, 0x0);
	codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, RTLNMUTE, 0x0);
	codec_wr_prcm_control(PHONEOUT_CTRL, 0xff, PHONEOUTS0, 0x60);
	codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0xff, LINEOUTR_SS, 0x40);
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0xf, MIC1BOOST, 0x4);
	codec_wr_prcm_control(RADCMIXSC, 0xff, RADCMIXMUTE, 0x0);
	codec_wr_prcm_control(LADCMIXSC, 0xff, LADCMIXMUTE, 0x0);
	codec_wr_prcm_control(ADC_AP_EN, 0x3, ADCLEN, 0x0);
	/*close digital parts*/
	codec_wr_control(SUNXI_DA_CTL, 0xffffffff, GEN, 0x0);
	codec_wr_control(SUNXI_DA_FAT0, 0xffffffff, FMT, 0x0);
	codec_wr_control(SUNXI_DA_INT, 0x1, TX_DRQ, 0x0);
	codec_wr_control(SUNXI_DA_INT, 0x1, RX_DRQ, 0x0);
	codec_wr_control(SUNXI_DA_CLKD, 0xff, MCLKDIV, 0x0);
	codec_wr_control(SUNXI_SYSCLK_CTL, 0xfff, SYSCLK_SRC, 0x0);
	codec_wr_control(SUNXI_MOD_CLK_ENA, 0xffff, MODULE_CLK_EN_CTL, 0x0);
	codec_wr_control(SUNXI_MOD_RST_CTL, 0xffff, MODULE_RST_CTL_BIT, 0x0);
	codec_wr_control(SUNXI_AIF1CLK_CTRL, 0xffff, AIF1_TDMM_ENA, 0x0);
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL, 0xffff, AIF1_SLOT_SIZ, 0x0);
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL, 0xffff, AIF1_LOOP_ENA, 0x0);
	codec_wr_control(SUNXI_AIF1_MXR_SRC, 0xffff, AIF1_AD1R_MXR_SRC_C, 0x0);
	codec_wr_control(SUNXI_AIF2_CLK_CTRL, 0xffff, 0, 0x0);
	codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0xffff, AIF2_LOOP_EN, 0x0);
	codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0xffff, 0, 0x0);
	codec_wr_control(SUNXI_AIF2_MXR_SRC, 0xff, AIF2_ADCR_MXR_SRC, 0x0);
	codec_wr_control(SUNXI_AIF3_CLK_CTRL, 0xffff, AIF3_CLOC_SRC, 0x0);
	codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 0x0);
	codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0x0);
	codec_wr_control(SUNXI_DAC_MXR_SRC, 0xff, DACR_MXR_SRC, 0x0);

	/*set all routeway flag false*/
	codec_analog_phonein_en		   = 0;
	codec_analog_mainmic_en    = 0;
	codec_analog_phoneout_en          = 0;
	codec_lineinin_en          = 0;
	codec_lineincap_en         = 0;
	codec_analog_headsetmic_en = 0;
	codec_speakerout_en        = 0;
	codec_earpieceout_en       = 0;
	codec_voice_record_en      = 0;
	codec_headphoneout_en      = 0;
	/*Digital_bb*/
	codec_digital_headsetmic_en = 0;
	codec_digital_mainmic_en	= 0;
	codec_digital_phoneout_en	= 0;

	codec_digital_phonein_en 	= 0;
	codec_digital_bb_clk_format_init = 0;

	/*bluetooth*/
	codec_bt_clk_format 		= 0;
	codec_bt_out_en 			= 0;
	bt_bb_button_voice 			= 0;

	codec_analog_btmic_en 		= 0;
	codec_analog_btphonein_en 	= 0;

	codec_digital_btmic_en 		= 0;
	codec_digital_btphonein_en 	= 0;
	codec_digital_bb_bt_clk_format = 0;
	codec_system_bt_capture_en = 0;

	return 0;
}

static int codec_get_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
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
*	codec_lineinin_en == 1, open the linein in.
*	codec_lineinin_en == 0, close the linein in.
*/
static int codec_set_lineinin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_lineinin_en = ucontrol->value.integer.value[0];
	if (codec_lineinin_en) {
		/*select LINEINL*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTELINEINL, 0x1);
		/*select LINEINR*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTELINEINR, 0x1);
		/*enable output mixer*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);
	} else {
		/*close LINEINL*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTELINEINL, 0x0);
		/*close LINEINR*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTELINEINR, 0x0);
		/*disable output mixer*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
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
*	codec_digital_mainmic_en == 1, open mic1 for digital bb.
*	codec_digital_mainmic_en == 0, close mic1 for digital bb.
*/
static int codec_digital_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_mainmic_en = ucontrol->value.integer.value[0];
	if (codec_digital_mainmic_en) {
		/*open main mic(mic1) routeway*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL,0x7, MIC1BOOST, phone_main_mic_vol);
		/*enable Master microphone bias*/
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x1);
		/*enable Left MIC1 Boost stage*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
		/*enable adc analog*/
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0x1);
		/*enable ADC Digital part*/
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 1);
		/*enable Digital microphone*/
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENDM, 0);
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC, 1);

	} else {
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC, 0);
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 0);
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0);
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC1BOOST, 0);
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0);
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC1AMPEN, 0);
	}
	return 0;
}

static int codec_digital_get_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_mainmic_en;
	return 0;
}

/*
*	codec_digital_headsetmic_en == 1, open mic2 for digital bb.
*	codec_digital_headsetmic_en == 0, close mic2 for digital bb.
*/
static int codec_digital_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_headsetmic_en = ucontrol->value.integer.value[0];
	if (codec_digital_headsetmic_en) {
		/*enable mic2 pa*/
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0x1, MIC2AMPEN, 0x1);
		/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL,0x7,MIC2BOOST, phone_headset_mic_vol);
		codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MIC2_SS, 0x1);
		/*enable Left MIC2 Boost stage*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
		/*enable adc analog*/
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0x1);
		/*enable ADC Digital part*/
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 1);
		/*enable Digital microphone*/
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENDM, 0);
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC, 1);

	} else {
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC, 0);
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 0);
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0x0);
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
		codec_wr_prcm_control(MIC2G_LINEEN_CTRL, 0x1, MIC2AMPEN, 0);

	}
	return 0;
}

static int codec_digital_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_headsetmic_en;
	return 0;
}

/*
*	codec_digital_phoneout_en == 1, enable phoneout for digital bb.
*	codec_digital_phoneout_en == 0, disable phoneout for digital bb.
*/
static int codec_digital_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_phoneout_en = ucontrol->value.integer.value[0];
	if (codec_digital_phoneout_en) {
		/*enable aif2 adcl chanel*/
		codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x1, AIF2_ADCL_EN, 1);
		/*select aif2 adc left channel source */
		codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x3, AIF2_ADCL_SRC, 0);

	} else {
		codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x1, AIF2_ADCL_EN, 0);
	}
	return 0;
}

static int codec_digital_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_phoneout_en;
	return 0;
}

/*
*	codec_digital_phonein_en == 1, enble phonein for digital bb .
*	codec_digital_phonein_en == 0. disable phonein for digital bb.
*/
static int codec_set_digital_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_phonein_en = ucontrol->value.integer.value[0];
	if (codec_digital_phonein_en) {
		/*enable aif2 dacl chanel*/
		codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x1, AIF2_DACL_ENA, 1);
		/*select aif2 dac left channel source */
		codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x3, AIF2_DACL_SRC, 0);
		/*aif2 dac input source select*/
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC, 0);
		/*dac left channel source select*/
		codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF2DACL, 1);

		/*dac digital part enable*/
		codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 1);
		/*dac analog part enable*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);

		/*select output mixer source*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x1);
		/*enable output mixer*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);

	} else {
		/*disable output mixer*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
		/**/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x0);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x0);

		/*disable dac digital part*/
		codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0);
		/*disable dac analog part*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);

		/*dac left channel source select*/
		codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF2DACL, 0);
		/*disable aif2 dacl chanel*/
		codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x1, AIF2_DACL_ENA, 0);
	}
	return 0;
}

static int codec_get_digital_phonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_phonein_en;
	return 0;
}

/*
*codec_set_digital_bb_clk_format:confige the clk fmt for call with digital bb
*
*/
static int codec_set_digital_bb_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_bb_clk_format_init = ucontrol->value.integer.value[0];
	if (codec_digital_bb_clk_format_init) {
		/*enable src clk*/
		/*set pll2 :24576k*/
		if (clk_set_rate(codec_pll2clk, 24576000)) {
			pr_err("set codec_pll2clk rate fail\n");
		}
		/*enable pll2*4 for src*/
		if ((!codec_srcclk)||(IS_ERR(codec_srcclk))) {
			pr_err("try to get codec_srcclk failed!\n");
		}
		if (clk_prepare_enable(codec_srcclk)) {
			pr_err("err:open codec_srcclk failed; \n");
		}

		/*enable aif2,system clk from aif2*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, AIF2CLK_ENA, 0x1);
		/*aif2 clk source select*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x3, AIF2CLK_SRC, 0x3);

		/*sysclk from aif2*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, SYSCLK_SRC, 0x1);

		/*enable aif2/da/da module*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF2_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, DAC_DIGITAL_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, ADC_DIGITAL_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC1_MOD_CLK_EN, 0x1);/*src1*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC2_MOD_CLK_EN, 0x1);/*src2*/

		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF2_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, DAC_DIGITAL_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, ADC_DIGITAL_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC2_MOD_RST_CTL, 0x1);/*src2*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC1_MOD_RST_CTL, 0x1);/*src1*/

		/*confige ad/da sr:8k*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x0);
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x0);
		/*select src1 source from aif2*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC1_SRC, 0x1);
		/*select src2 source from aif2*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC2_SRC, 0x1);

		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC1_ENA, 0x1);/*enable src1*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC2_ENA, 0x1);/*enable src2*/

		/*confige aif2 lrck:8k,pcm,mono,slave*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_MSTR_MOD, 0x1);/*confige aif2 slave*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_BCLK_INV, 0x0);/*bclk_inv:0*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_LRCK_INV, 0x0);/*lrck_inv:0*/
//		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x3, AIF2_LRCK_DIV, 0x2);/*bclk/lrck:64*/
//		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0xf, AIF2_BCLK_DIV, 0x2);/*aif2/bclk:48*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x3, AIF2_WORD_SIZ, 0x1);/*wss:16*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x3, AIF2_DATA_FMT, 0x3);/*fmt:pcm*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_MONO_PCM, 0x1);/*fmt:pcm*/
	} else {
		/*disable aif2,system clk from aif2*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0xfff, SYSCLK_SRC, 0);
		/*disable aif2/da/da module*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF2_MOD_CLK_EN, 0x0);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, DAC_DIGITAL_MOD_CLK_EN, 0x0);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, ADC_DIGITAL_MOD_CLK_EN, 0x0);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC1_MOD_CLK_EN, 0x0);/*src1*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC2_MOD_CLK_EN, 0x0);/*src2*/

		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF2_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, DAC_DIGITAL_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, ADC_DIGITAL_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC2_MOD_RST_CTL, 0x0);/*src2*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC1_MOD_RST_CTL, 0x0);/*src1*/

		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC1_ENA, 0x0);/*disable src1*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC2_ENA, 0x0);/*disable src2*/

		/*confige aif2 lrck:8k,pcm,mono,slave*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0xffff, 0, 0x0);

		if ((NULL == codec_srcclk)||(IS_ERR(codec_srcclk))) {
			pr_err("codec_srcclk handle is invaled, just return\n");
		} else {
			clk_disable_unprepare(codec_srcclk);
		}

	}

	return 0;
}

static int codec_get_digital_bb_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_bb_clk_format_init;
	return 0;
}

static int codec_set_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_bt_clk_format	= ucontrol->value.integer.value[0];

	if (codec_bt_clk_format) {
		pr_debug("%s,line:%d\n",__func__,__LINE__);
		/*enable src clk*/
		if (clk_set_rate(codec_pll2clk, 24576000)) {
			pr_err("err:set codec_pll2clk rate fail\n");
		}

		if ((!codec_srcclk)||(IS_ERR(codec_srcclk))) {
			pr_err("err:try to get codec_srcclk failed!\n");
		}
		if (clk_prepare_enable(codec_srcclk)) {
			pr_err("err:open codec_srcclk failed; \n");
		}

		/*enable aif2,system clk from aif2*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, AIF2CLK_ENA, 0x1);
		/*aif2 clk source select*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x3, AIF2CLK_SRC, 0x3);

		/*sysclk from aif2*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, SYSCLK_SRC, 0x1);

		/*enable sysclk*/
		codec_wr_control(SUNXI_SYSCLK_CTL, 0x1, SYSCLK_ENA, 0x1);

		/*enable aif2/da/da module*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF2_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, DAC_DIGITAL_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, ADC_DIGITAL_MOD_CLK_EN, 0x1);

		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC1_MOD_CLK_EN, 0x1);/*src1*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC2_MOD_CLK_EN, 0x1);/*src2*/

		/*enable aif3*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF3_MOD_CLK_EN, 0x1);
		/*reset aif2/da/da module*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF2_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, DAC_DIGITAL_MOD_RST_CTL, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, ADC_DIGITAL_MOD_RST_CTL, 0x1);

//		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC2_MOD_RST_CTL, 0x1);/*src2*/
//		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC1_MOD_RST_CTL, 0x1);/*src1*/

		/*reset aif3*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF3_MOD_RST_CTL, 0x1);

		/*confige ad/da sr:8k*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF1_FS, 0x0);
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0xf, AIF2_FS, 0x0);

//		/*select src1 source from aif2*/
//		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC1_SRC, 0x1);
//		/*select src2 source from aif2*/
//		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC2_SRC, 0x1);

//		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC1_ENA, 0x1);/*enable src1*/
//		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC2_ENA, 0x1);/*enable src2*/

		/*confige aif2 lrck:8k,pcm,mono,master*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_MSTR_MOD, 0x0);/*confige aif2 master*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_BCLK_INV, 0x0);/*bclk_inv:0*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_LRCK_INV, 0x0);/*lrck_inv:0*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x3, AIF2_LRCK_DIV, 0x2);/*bclk/lrck:64*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0xf, AIF2_BCLK_DIV, 0x9);/*aif2/bclk:48*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x3, AIF2_WORD_SIZ, 0x1);/*wss:16*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x3, AIF2_DATA_FMT, 0x3);/*fmt:pcm*/
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0x1, AIF2_MONO_PCM, 0x1);/*fmt:pcm*/

		/*confige aif3*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x1, AIF3_BCLK_INV, 0x0);/*bclk_inv:0*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x1, AIF3_LRCK_INV, 0x0);/*lrck_inv:0*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x3, AIF3_WORD_SIZ, 0x1);/*wss:16*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x3, AIF3_CLOC_SRC, 0x1);/*clk from aif2*/
	} else {
		codec_wr_control(SUNXI_SYSCLK_CTL, 0xfff, SYSCLK_SRC, 0);

		/*enable aif2/da/da module*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF2_MOD_CLK_EN, 0x0);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, DAC_DIGITAL_MOD_CLK_EN, 0x0);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, ADC_DIGITAL_MOD_CLK_EN, 0x0);
		/*enable aif3*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF3_MOD_CLK_EN, 0x0);
		/*reset aif2/da/da module*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF2_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, DAC_DIGITAL_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, ADC_DIGITAL_MOD_RST_CTL, 0x0);
		/*reset aif3*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF3_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_AIF2_CLK_CTRL ,  0xffff, 0, 0x0);
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0xffff, 0, 0x0);
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC1_MOD_CLK_EN, 0x0);/*src1*/
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, SRC2_MOD_CLK_EN, 0x0);/*src2*/

		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC2_MOD_RST_CTL, 0x0);/*src2*/
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, SRC1_MOD_RST_CTL, 0x0);/*src1*/

		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC1_ENA, 0x0);/*disable src1*/
		codec_wr_control(SUNXI_SYS_SR_CTRL ,  0x1, SRC2_ENA, 0x0);/*disable src2*/
		if ((NULL == codec_srcclk)||(IS_ERR(codec_srcclk))) {
			pr_err("codec_srcclk handle is invaled, just return\n");
		} else {
			clk_disable_unprepare(codec_srcclk);
		}
	}

	return 0;
}

static int codec_get_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_bt_clk_format;
	return 0;
}

static int codec_set_bt_out(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_bt_out_en = ucontrol->value.integer.value[0];
	if (codec_bt_out_en) {
		/*enable aif2 adcl channel(this bit for make clk)*/
		codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x1, AIF2_ADCL_EN, 1);
		/*select aif3 pcm output source*/
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF3_ADC_SRC, 2);
	} else {
		/*disable aif2 adcl channel*/
		codec_wr_control(SUNXI_AIF2_ADCDAT_CTRL, 0x1, AIF2_ADCR_EN, 0);
		/*select aif3 pcm output source*/
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF3_ADC_SRC, 0);
	}

	return 0;
}

static int codec_get_bt_out(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_bt_out_en;
	return 0;
}

static int codec_set_analog_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_analog_btmic_en = ucontrol->value.integer.value[0];
	if (codec_analog_btmic_en) {
		/*select aif2 dac input source*/
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC, 1);
		/*dac left channel mixer source select*/
		codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF2DACL, 1);
		/*dac digital enble*/
		codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 1);
		/*dac analog enble*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);

		/*left output  mixer source select*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x1);

		/*left output mixer enble*/
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		/*select phoneout source*/
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUTS0, 0x1);
	} else {
		codec_wr_prcm_control(PHONEOUT_CTRL, 0x1, PHONEOUTS0, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEDACL, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
		codec_wr_control(SUNXI_DAC_DIG_CTRL, 0x1, ENDA, 0);
		codec_wr_control(SUNXI_DAC_MXR_SRC, 0x1, DACL_MXR_SRC_AIF2DACL, 0);
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF3_ADC_SRC, 0);
	}

	return 0;
}

static int codec_get_analog_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_btmic_en;
	return 0;
}

static int codec_set_analog_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_analog_btphonein_en = ucontrol->value.integer.value[0];
	if (codec_analog_btphonein_en) {
		/*right mixer source :phone_pn*/
		codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTEPHONEPN, 0x1);
		/*enable adc analog */
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCREN, 0x1);
		/*enable adc digital*/
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 1);
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENDM, 0);
		/*select AIF2 dac right channel mixer source */
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCR_MXR_SRC_ADCR, 1);

	} else {
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCR_MXR_SRC_ADCR, 0);
		codec_wr_control(SUNXI_ADC_DIG_CTRL, 0x1, ENAD, 0);
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCREN, 0x0);
		codec_wr_prcm_control(RADCMIXSC, 0x1, RADCMIXMUTEPHONEPN, 0);
	}
	return 0;
}

static int codec_get_analog_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_analog_btphonein_en;
	return 0;
}

static int codec_set_digital_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_btmic_en = ucontrol->value.integer.value[0];
	if (codec_digital_btmic_en) {

		/*select aif2 dac input source*/
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC, 2);

		/*aif2 dac right channel enable*/
		//codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x1, AIF2_DACR_ENA,1);

		/*aif2 adc left channel micer source select*/
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC_AIF2DACR,1);
	} else {
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCL_MXR_SRC_AIF2DACR,0);
		//codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x1, AIF2_DACR_ENA,0);
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC, 0);
	}
	return 0;
}

static int codec_get_digital_btmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_btmic_en;
	return 0;
}

static int codec_set_digital_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_btphonein_en = ucontrol->value.integer.value[0];
	if (codec_digital_btphonein_en) {
		/*aif2 dac left channel enable*/
		codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x1, AIF2_DACL_ENA, 1);
		codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x3, AIF2_DACL_SRC, 0);
		/*select aif2 dac input source*/
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC,2);
		/*select aif2 adc right channle mixer source */
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCR_MXR_SRC_AIF2DACL, 1);
	} else {
		codec_wr_control(SUNXI_AIF2_MXR_SRC, 0x1, AIF2_ADCR_MXR_SRC_AIF2DACL, 0);
		codec_wr_control(SUNXI_AIF2_DACDAT_CTRL, 0x1, AIF2_DACL_ENA, 0);
		codec_wr_control(SUNXI_AIF3_SGP_CTRL, 0x3, AIF2_DAC_SRC,0);
	}
	return 0;
}

static int codec_get_digital_btphonein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_btphonein_en;
	return 0;
}


static int codec_set_bt_button_voice(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	bt_bb_button_voice =  ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_bt_button_voice(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = bt_bb_button_voice ;
	return 0;
}


static int codec_set_digital_bb_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_digital_bb_bt_clk_format  = ucontrol->value.integer.value[0];

	if(codec_digital_bb_bt_clk_format){
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF3_MOD_CLK_EN, 0x1);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF3_MOD_RST_CTL, 0x1);

		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x1, AIF3_BCLK_INV, 0x0);/*bclk_inv:0*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x1, AIF3_LRCK_INV, 0x0);/*lrck_inv:0*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x3, AIF3_WORD_SIZ, 0x1);/*wss:16*/
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x3, AIF3_CLOC_SRC, 0x1);/*clk from aif2*/

	}else{
		codec_wr_control(SUNXI_MOD_CLK_ENA, 0x1, AIF3_MOD_CLK_EN, 0x0);
		codec_wr_control(SUNXI_MOD_RST_CTL, 0x1, AIF3_MOD_RST_CTL, 0x0);
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x3, AIF3_WORD_SIZ, 0);
		codec_wr_control(SUNXI_AIF3_CLK_CTRL ,  0x3, AIF3_CLOC_SRC, 0);
	}

	return 0;
}

static int codec_get_digital_bb_bt_clk_format(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_digital_bb_bt_clk_format;
	return 0;
}

static int codec_set_system_bt_capture_flag(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	 codec_system_bt_capture_en =  ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_system_bt_capture_flag(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	  ucontrol->value.integer.value[0]=codec_system_bt_capture_en;
	return 0;
}
static int codec_set_analog_bb_capture_mic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	 codec_analog_bb_capture_mic =  ucontrol->value.integer.value[0];
	 if (codec_analog_bb_capture_mic) {
		codec_analog_voice_capture_open();
	 } else {
		/*enable aif1 adc left channel */
		codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x1, AIF1_AD0L_ENA,0);
		/*select mic source*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
		/*select mic2 source*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
		/*select phonein source*/
		codec_wr_prcm_control(LADCMIXSC, 0x1, LADCMIXMUTEPHONEPN, 0x0);
		/*enable adc analog*/
		codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0x0);

		/*enable dac digital*/
		codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENAD, 0);
		codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENDM, 0);

		/*select aif1 adc left channel mixer source */
		codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0x1, AIF1_AD0L_MXL_SRC_ADCL,0);

		/*select aif1 adc left channel source */
		codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0L_SRC,0);

	 }
	return 0;
}

static int codec_get_analog_bb_capture_mic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	  ucontrol->value.integer.value[0]=codec_analog_bb_capture_mic;
	return 0;
}
static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/*volume ctl: headset ,speaker,earpiece*/
	//CODEC_SINGLE("Master Playback Volume", HP_VOLC, 0, 0x3f, 0),
	CODEC_SINGLE("headphone volume control", HP_VOLC, 0, 0x3f, 0),
	CODEC_SINGLE("earpiece volume control", HP_VOLC, 0, 0x3f, 0),
	CODEC_SINGLE("speaker volume control", HP_VOLC, 0, 0x3f, 0),

	CODEC_SINGLE("MIC1_G boost stage output mixer control", 	MICIN_GCTRL, MIC1G, 0x7, 0),
	CODEC_SINGLE("MIC2_G boost stage output mixer control", 	MICIN_GCTRL, MIC2G, 0x7, 0),
	CODEC_SINGLE("LINEIN_G boost stage output mixer control",	LINEIN_GCTRL, LINEING, 0x7, 0),
	CODEC_SINGLE("PHONE_G boost stage output mixer control",	LINEIN_GCTRL, PHONEG, 0x7, 0),
	CODEC_SINGLE("PHONE_PG boost stage output mixer control",	PHONEIN_GCTRL, PHONEPG, 0x7, 0),
	CODEC_SINGLE("PHONE_NG boost stage output mixer control",	PHONEIN_GCTRL, PHONENG, 0x7, 0),

	CODEC_SINGLE("MIC1 boost AMP gain control", 				MIC1G_MICBIAS_CTRL, MIC1BOOST, 0x7, 0),
	CODEC_SINGLE("MIC2 boost AMP gain control", 				MIC2G_LINEEN_CTRL, MIC2BOOST, 0x7, 0),
	CODEC_SINGLE("Lineout volume control",						HP_VOLC, 0, 0x3f, 0),
	CODEC_SINGLE("PHONEP-PHONEN pre-amp gain control",			LINEOUT_VOLC, PHONEPREG, 0x7, 0),
	CODEC_SINGLE("Phoneout gain control",						PHONEOUT_CTRL, PHONEOUTG, 0x7, 0),

	CODEC_SINGLE("ADC input gain ctrl", 						ADC_AP_EN, ADCG, 0x7, 0),

	SOC_SINGLE_BOOL_EXT("Audio phone out", 		0, codec_analog_get_phoneout, 	codec_analog_set_phoneout),				/*enable phoneout*/
	SOC_SINGLE_BOOL_EXT("Audio phone in", 		0, codec_analog_get_phonein, 	codec_analog_set_phonein),					/*open the phone in call*/
	SOC_SINGLE_BOOL_EXT("Audio earpiece out", 	0, codec_get_earpieceout, 	codec_set_earpieceout),			/*set the phone in call voice through earpiece out*/
	SOC_SINGLE_BOOL_EXT("Audio headphone out", 	0, codec_get_headphoneout, 	codec_set_headphoneout),		/*set the phone in call voice through headphone out*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out", 	0, codec_get_speakerout, 	codec_set_speakerout),			/*set the phone in call voice through speaker out*/

	SOC_SINGLE_BOOL_EXT("Audio analog main mic", 		0, codec_analog_get_mainmic, codec_analog_set_mainmic), 			/*set main mic(mic1)*/
	SOC_SINGLE_BOOL_EXT("Audio analog headsetmic", 	0, codec_analog_get_headsetmic, codec_analog_set_headsetmic),    	/*set headset mic(mic2)*/
	SOC_SINGLE_BOOL_EXT("Audio phone voicerecord", 	0, codec_get_voicerecord, 	codec_set_voicerecord),   	/*set voicerecord status*/
	SOC_SINGLE_BOOL_EXT("Audio phone endcall", 	0, codec_get_endcall, 	codec_set_endcall),    				/*set endcall*/

	SOC_SINGLE_BOOL_EXT("Audio linein record", 	0, codec_get_lineincap, codec_set_lineincap),
	SOC_SINGLE_BOOL_EXT("Audio linein in", 		0, codec_get_lineinin, 	codec_set_lineinin),

	SOC_ENUM_EXT("Speaker Function", spk_headset_earpiece_enum[0], codec_get_spk_headset_earpiece, codec_set_spk_headset_earpiece),

	/*audio digital interface for phone case*/
	SOC_SINGLE_BOOL_EXT("Audio digital main mic", 	0, codec_digital_get_mainmic, codec_digital_set_mainmic),	/*set mic1 for digital bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital headset mic", 	0, codec_digital_get_headsetmic, codec_digital_set_headsetmic),/*set mic2 for digital bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital phone out",	0, codec_digital_get_phoneout, codec_digital_set_phoneout),/*set phoneout for digital bb*/

	SOC_SINGLE_BOOL_EXT("Audio digital phonein",	0, codec_get_digital_phonein, codec_set_digital_phonein),/*set phonein for digtal bb*/
	SOC_SINGLE_BOOL_EXT("Audio digital clk format status",	0, codec_get_digital_bb_clk_format, codec_set_digital_bb_clk_format),/*set clk,format for digtal bb*/

	/*bluetooth*/
	SOC_SINGLE_BOOL_EXT("Audio bt clk format status",	0, codec_get_bt_clk_format, codec_set_bt_clk_format),/*set clk,format for bt*/
	SOC_SINGLE_BOOL_EXT("Audio bt out",	0, codec_get_bt_out, codec_set_bt_out),/*set bt out*/

	SOC_SINGLE_BOOL_EXT("Audio analog bt mic",	0, codec_get_analog_btmic, codec_set_analog_btmic),/*set analog bt mic*/
	SOC_SINGLE_BOOL_EXT("Audio analog bt phonein",	0, codec_get_analog_btphonein, codec_set_analog_btphonein),/*set analog bt phonein*/

	SOC_SINGLE_BOOL_EXT("Audio digital bt mic",	0, codec_get_digital_btmic, codec_set_digital_btmic),/* set bt mic for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio digital bt phonein",	0, codec_get_digital_btphonein, codec_set_digital_btphonein),/*set bt phonein for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio bt button voice",	0, codec_get_bt_button_voice, codec_set_bt_button_voice),/*bt_bb_out*/
	SOC_SINGLE_BOOL_EXT("Audio digital bb bt clk format", 0, codec_get_digital_bb_bt_clk_format, codec_set_digital_bb_bt_clk_format),/*set bt phonein for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio system bt capture flag", 0, codec_get_system_bt_capture_flag, codec_set_system_bt_capture_flag),/*set bt phonein for dbb*/
	SOC_SINGLE_BOOL_EXT("Audio analog bb capture mic", 0, codec_get_analog_bb_capture_mic, codec_set_analog_bb_capture_mic),/*set bt phonein for dbb*/
#if 1
	CODEC_SINGLE_DIGITAL("aif3 loopback", SUNXI_AIF3_DACDAT_CTRL, AIF3_LOOP_ENA, 0x1, 0),/*test :1,default:0*/
	CODEC_SINGLE_DIGITAL("aif2 loopback", SUNXI_AIF2_DACDAT_CTRL, AIF2_LOOP_EN, 0x1, 0),/*test:1,default:0*/

	CODEC_SINGLE_DIGITAL("digital_bb_bt", SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_AIF2DACL, 0x1, 0),/*test:1,default:0*/

	CODEC_SINGLE_DIGITAL("system play_capture set 1", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC, 0xf, 0),/*teset:0x8*/
	CODEC_SINGLE_DIGITAL("system play_capture set 2", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACR, 0xf, 0),/*teset:0x8*/

	/*0x24c*/
	CODEC_SINGLE_DIGITAL("AIF1_AD0L_MXR_SRC AIF1DA0Ldata", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF1DA0L, 0x1, 0),
	CODEC_SINGLE_DIGITAL("AIF1_AD0L_MXR_SRC AIF2DACLdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACL, 0x1, 0),
	CODEC_SINGLE_DIGITAL("AIF1_AD0L_MXR_SRC ADCLdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_ADCL, 0x1, 0),
	CODEC_SINGLE_DIGITAL("AIF1_AD0L_MXR_SRC AIF2DACRdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACR, 0x1, 0),

	CODEC_SINGLE_DIGITAL("AIF1_AD0R_MXR_SRC AIF1DA0Rdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF1DA0R, 0x1, 0),
	CODEC_SINGLE_DIGITAL("AIF1_AD0R_MXR_SRC AIF2DACRdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF2DACR, 0x1, 0),
	CODEC_SINGLE_DIGITAL("AIF1_AD0R_MXR_SRC ADCRdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_ADCR, 0x1, 0),
	CODEC_SINGLE_DIGITAL("AIF1_AD0R_MXR_SRC AIF2DACLdata", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF2DACL, 0x1, 0),

	/*ADC source*/
	CODEC_SINGLE("Analog cap test disable phonein",	LADCMIXSC, LADCMIXMUTEPHONEPN, 0x1, 0),
	CODEC_SINGLE("Analog cap test disable mic1",	LADCMIXSC, LADCMIXMUTEMIC1BOOST, 0x1, 0),
	CODEC_SINGLE("Analog cap test disable mic2",	LADCMIXSC, LADCMIXMUTEMIC2BOOST, 0x1, 0),
#endif
};

static struct snd_soc_dai_ops sndpcm_dai_ops = {
	.startup = sndpcm_startup,
	.shutdown = sndpcm_shutdown,
	.prepare  =	sndpcm_perpare,
	//.trigger 	= sndpcm_trigger,
	.hw_params = sndpcm_hw_params,
	.digital_mute = sndpcm_unmute,
	//.set_sysclk = sndpcm_set_dai_sysclk,
	//.set_clkdiv = sndpcm_set_dai_clkdiv,
	//.set_fmt = sndpcm_set_dai_fmt,
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
	unsigned long config;
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];

	pr_debug("[audio codec]:suspend start\n");
	/* check if called in talking standby */
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		pr_err("In talking standby, audio codec do not suspend!!\n");
		return 0;
	}
	gpio_set_value(item.gpio.gpio, 0);

	sunxi_gpio_to_name(item.gpio.gpio, pin_name);
	config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
	pin_config_set(SUNXI_PINCTRL, pin_name, config);
	/*mute l_pa and r_pa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	/*digital part*/
	codec_wr_control(SUNXI_DA_CTL ,  0xffffffff, GEN, 0x0);
	/*SUNXI_AIF1_ADCDAT_CTRL:disable AIF1_AD0L_ENA, disable AIF1_AD0R_ENA*/
	codec_wr_control(SUNXI_AIF1_ADCDAT_CTRL ,  0x3, AIF1_AD0R_ENA, 0x0);
	/*SUNXI_AIF1_DACDAT_CTRL:disable AIF1_DA0L_ENA, disable AIF1_DA0R_ENA*/
	codec_wr_control(SUNXI_AIF1_DACDAT_CTRL ,  0x3, AIF1_DA0R_ENA, 0x0);
	/*SUNXI_AIF1_MXR_SRC:mute*/
	codec_wr_control(SUNXI_AIF1_MXR_SRC ,  0xffff, AIF1_AD1R_MXR_SRC_C, 0x0);
	/*dsable adc digital*/
	codec_wr_control(SUNXI_ADC_DIG_CTRL ,  0x1, ENAD, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DIG_CTRL ,  0x1, ENDA, 0x0);
	/*analog parts*/
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x0);
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, HMICBIAS_MODE, 0x0);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
	/*disable dac analog*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);
	/*disable adc analog*/
	codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCREN, 0x0);
	codec_wr_prcm_control(ADC_AP_EN, 0x1, ADCLEN, 0x0);
	codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);
	//codec_wr_prcm_control(LINEOUT_VOLC, 0x1f, LINEOUTVOL, 0x0);
	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
	} else {
		clk_disable_unprepare(codec_moduleclk);
	}
	pr_debug("[audio codec]:suspend end\n");
	return 0;
}

static int sndpcm_resume(struct snd_soc_codec *codec)
{
	pr_debug("[audio codec]:resume start\n");
	if ((!codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("try to get codec_moduleclk failed!\n");
	}
	if (clk_prepare_enable(codec_moduleclk)) {
		pr_err("open codec_moduleclk failed; \n");
	}

	if (headphone_direct_used) {
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x3);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x1);
	} else {
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, COMPTEN, 0x0);
	}
	if (drc_used){
		codec_wr_control(0x48c, 0x7ff, 0, 0x1);
//		codec_wr_control(0x48c, 0x7ff, 0, 0x0);
		codec_wr_control(0x490, 0xffff, 0, 0x2baf);
		//codec_wr_control(0x490, 0xffff, 0, 0x1fb6);
		codec_wr_control(0x494, 0x7ff, 0, 0x1);
		codec_wr_control(0x498, 0xffff, 0, 0x2baf);
		codec_wr_control(0x49c, 0x7ff, 0, 0x0);
		codec_wr_control(0x4a0, 0xffff, 0, 0x44a);
		codec_wr_control(0x4a4, 0x7ff, 0, 0x0);
		codec_wr_control(0x4a8, 0xffff, 0, 0x1e06);
		//codec_wr_control(0x4ac, 0x7ff, 0, 0x27d);
		codec_wr_control(0x4ac, 0x7ff, 0, 0x352);
		//codec_wr_control(0x4b0, 0xffff, 0, 0xcf68);
		codec_wr_control(0x4b0, 0xffff, 0, 0x6910);
		codec_wr_control(0x4b4, 0x7ff, 0, 0x77a);
		codec_wr_control(0x4b8, 0xffff, 0, 0xaaaa);
		//codec_wr_control(0x4bc, 0x7ff, 0, 0x1fe);
		codec_wr_control(0x4bc, 0x7ff, 0, 0x2de);
		codec_wr_control(0x4c0, 0xffff, 0, 0xc982);
		codec_wr_control(0x258, 0xffff, 0, 0x9f9f);
	}
	if (agc_used){
		codec_wr_control(0x4d0, 0x3, 6, 0x3);

		codec_wr_control(0x410, 0x3f, 8, 0x31);
		codec_wr_control(0x410, 0xff, 0, 0x28);

		codec_wr_control(0x414, 0x3f, 8, 0x31);
		codec_wr_control(0x414, 0xff, 0, 0x28);

		codec_wr_control(0x428, 0x7fff, 0, 0x24);
		codec_wr_control(0x42c, 0x7fff, 0, 0x2);
		codec_wr_control(0x430, 0x7fff, 0, 0x24);
		codec_wr_control(0x434, 0x7fff, 0, 0x2);
		codec_wr_control(0x438, 0x1f, 8, 0xf);
		codec_wr_control(0x438, 0x1f, 0, 0xf);
		codec_wr_control(0x44c, 0x7ff, 0, 0xfc);
		codec_wr_control(0x450, 0xffff, 0, 0xabb3);
	}
	/*process for normal standby*/
	if (NORMAL_STANDBY == standby_type) {
	/*process for super standby*/
	} else if(SUPER_STANDBY == standby_type) {
		/*when TX FIFO available room less than or equal N,
		* DRQ Requeest will be de-asserted.
		*/
	}
	gpio_direction_output(item.gpio.gpio, 1);
	gpio_set_value(item.gpio.gpio, 0);
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


static ssize_t show_audio_reg(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int count = 0;
	int i = 0;
	int reg_group =0;
	printk("%s,line:%d\n",__func__,__LINE__);

	count += sprintf(buf, "dump audio reg:\n");

	while (reg_labels[i].name != NULL){
		if (reg_labels[i].value == 0){
			reg_group++;
		}
		if (reg_group == 1){
			count +=sprintf(buf + count, "%s 0x%p: 0x%x\n", reg_labels[i].name,
							(baseaddr + reg_labels[i].value),
							readl(baseaddr + reg_labels[i].value) );
		} else 	if (reg_group == 2){
			count +=sprintf(buf + count, "%s 0x%x: 0x%x\n", reg_labels[i].name,
							(reg_labels[i].value),
						        read_prcm_wvalue(reg_labels[i].value) );
		}
		i++;
	}

	return count;
}

/* ex:
	read:
		echo 0,1,0x00> audio_reg
   		echo 0,2,0x00> audio_reg
	write:
		echo 1,1,0x00,0xa > audio_reg
		echo 1,2,0x00,0xff > audio_reg
*/
static ssize_t store_audio_reg(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	//int reg_group =0;
	//int find_labels_reg_offset =-1;
	int input_reg_group =0;
	int input_reg_offset =0;
	int input_reg_val =0;
	int reg_val_read;
	//int i = 0;
	int rw_flag;

	printk("%s,line:%d\n",__func__,__LINE__);
	ret = sscanf(buf, "%d,%d,0x%x,0x%x", &rw_flag,&input_reg_group, &input_reg_offset, &input_reg_val);
	printk("ret:%d, reg_group:%d, reg_offset:%d, reg_val:0x%x\n", ret, input_reg_group, input_reg_offset, input_reg_val);

	if (!(input_reg_group ==1 || input_reg_group ==2)){
		printk("not exist reg group\n");
		ret = count;
		goto out;
	}
	if (!(rw_flag ==1 || rw_flag ==0)){
		printk("not rw_flag\n");
		ret = count;
		goto out;
	}
	if (input_reg_group == 1){
		if (rw_flag){
			writel(input_reg_val, baseaddr + input_reg_offset);
		}else{
			reg_val_read = readl(baseaddr + input_reg_offset);
			printk("\n\n Reg[0x%x] : 0x%x\n\n",input_reg_offset,reg_val_read);
		}
	} else if (input_reg_group == 2){
		if(rw_flag) {
			write_prcm_wvalue(input_reg_offset, input_reg_val & 0xff);
		}else{
			 reg_val_read = read_prcm_wvalue(input_reg_offset);
			 printk("\n\n Reg[0x%x] : 0x%x\n\n",input_reg_offset,reg_val_read);
		}
	}

	ret = count;

out:
	return ret;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, store_audio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_audio_reg.attr,
	NULL,
};

static struct attribute_group audio_debug_attr_group = {
	.name   = "audio_reg_debug",
	.attrs  = audio_debug_attrs,
};

static int __init sndpcm_codec_probe(struct platform_device *pdev)
{
	int err = -1;
	int reg_val = 0;
	int req_status;
	//long unsigned int config_set = 0;
	script_item_u val;
	script_item_value_type_e  type;
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL, "pll2");
	if ((!codec_pll2clk)||(IS_ERR(codec_pll2clk))) {
		pr_err("[ audio ] err:try to get codec_pll2clk failed!\n");
	}
	if (clk_prepare_enable(codec_pll2clk)) {
		pr_err("[ audio ] err:enable codec_pll2clk failed; \n");
	}
	/* codec_moduleclk */
	codec_moduleclk = clk_get(NULL, "adda");
	if ((!codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("[ audio ] err:try to get codec_moduleclk failed!\n");
	}
	if (clk_set_parent(codec_moduleclk, codec_pll2clk)) {
		pr_err("[ audio ] err:try to set parent of codec_moduleclk to codec_pll2clk failed!\n");
	}
	if (clk_set_rate(codec_moduleclk, 24576000)) {
		pr_err("[ audio ] err:set codec_moduleclk clock freq 24576000 failed!\n");
	}
	if (clk_prepare_enable(codec_moduleclk)) {
		pr_err("[ audio ] err:open codec_moduleclk failed; \n");
	}
	/*clk pll_audiox4 for audiocodec src*/
	codec_srcclk = clk_get(NULL, "pll_audiox4");
	if ((!codec_srcclk)||(IS_ERR(codec_srcclk))) {
		pr_err("[ audio ] err:try to get codec_srcclk failed!\n");
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
	/*get the default pa ctl(close)*/
	type = script_get_item("audio0", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("[ audio ] err:try to get audio_pa_ctrl failed!\n");
		return -EFAULT;
	}
	/**
	*If use the aif2,aif3 interface in the audiocodec,
	* you need config the related interfaces about aif2 and aif3.
	*And can not use daudio0 and daudio1
	*/
	type = script_get_item("audio0", "aif2_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] aif2_used type err!\n");
	} else {
		aif2_used = val.val;
	}
	if(aif2_used){
		pr_debug("[audiocodec]: aif2 initialize PB04 PB05 PB06 PB07!!\n");
		#if 0
		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PB07",config_set);

		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PB06",config_set);

		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PB05",config_set);

		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PB04",config_set);
		#endif
		reg_val = readl((void __iomem *)0xf1c20824);
		reg_val &= 0xffff;
		reg_val |= (0x3333<<16);
		writel(reg_val, (void __iomem *)0xf1c20824);
		//pr_debug("%s,line:%d,reg_val:%x\n",__func__,__LINE__,readl((void __iomem *)0xf1c20824));
	} else {
		pr_err("[audiocodec] : aif2 not used!\n");
	}

	type = script_get_item("audio0", "aif3_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] aif3_used type err!\n");
	} else {
		aif3_used = val.val;
	}

	if(aif3_used){
		pr_err("[audiocodec}: aif3 initialize PG10 PG11 PG12 PG13!!\n");
		#if 0
		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PG13",config_set);

		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PG12",config_set);

		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PG11",config_set);

		config_set = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,3);
		pin_config_set(SUNXI_PINCTRL,"PG10",config_set);
		#endif
		reg_val = readl((void __iomem *)0xf1c208dc);
		reg_val &= 0xff;
		reg_val |= (0x3333<<8);
		writel(reg_val, (void __iomem *)0xf1c208dc);
		//pr_err("%s,line:%d,reg_val:%x\n",__func__,__LINE__,readl((void __iomem *)0xf1c208dc));

	} else {
		pr_err("[audiocodec] : aif3 not used!\n");
	}
	/*request gpio*/
	req_status = gpio_request(item.gpio.gpio, NULL);
	if (0 != req_status) {
		pr_err("request gpio failed!\n");
	}
	gpio_direction_output(item.gpio.gpio, 1);

	gpio_set_value(item.gpio.gpio, 0);

	codec_wr_prcm_control(PAEN_HP_CTRL, 0x1, HPPAEN, 0x1);

	snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndpcm, &sndpcm_dai, 1);

	err=sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (err){
		pr_err("failed to create attr group\n");
	}
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

	sysfs_remove_group(&pdev->dev.kobj, &audio_debug_attr_group);

	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

static void sunxi_codec_shutdown(struct platform_device *devptr)
{
	item.gpio.data = 0;
	pr_debug("%s,line:%d\n",__func__,__LINE__);

	codec_wr_prcm_control(ADDA_APT2, 0x1, ZERO_CROSS_EN, 0x0);

	/*mute l_pa and r_pa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);

	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, MMICBIASEN, 0x0);
	codec_wr_prcm_control(MIC1G_MICBIAS_CTRL, 0x1, HMICBIAS_MODE, 0x0);

	gpio_set_value(item.gpio.gpio, 0);

	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
	} else {
		clk_disable_unprepare(codec_moduleclk);
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
MODULE_AUTHOR("huanxin<huanxin@reuuimllatech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm-codec");
