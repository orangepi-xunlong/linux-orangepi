/*
 * sound\soc\sunxi\audiocodec\sndcodec.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
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
#include <sound/tlv.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <linux/power/scenelock.h>

#include "sunxi_codecdma.h"
#include "sun8iw9_sndcodec.h"
//#define AIF1_FPGA_LOOPBACK_TEST
#define codec_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define codec_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
static struct regulator* hp_ldo = NULL;
static char *hp_ldo_str = NULL;

static volatile int current_running = -1;

/*for pa gpio ctrl*/
static script_item_u item;

static int pa_vol 						= 0;
static int cap_vol 						= 0;
static int earpiece_vol 				= 0;
static int headphone_vol 				= 0;
static int pa_double_used 				= 0;
static int phone_main_mic_vol 			= 0;
static int headphone_direct_used 		= 0;
static int phone_headset_mic_vol 		= 0;
//static int aif2_used 					= 0;
//static int aif3_used 					= 0;
static int dac_vol_ctrl_spk				=0x9e9e;
static int dac_vol_ctrl_headphone		=0xa0a0;

static struct clk *codec_pll2clk,*codec_moduleclk,*codec_srcclk;

static const DECLARE_TLV_DB_SCALE(headphone_vol_tlv, -6300, 100, 0);
static const DECLARE_TLV_DB_SCALE(lineout_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(speaker_vol_tlv, -4800, 150, 0);
static const DECLARE_TLV_DB_SCALE(earpiece_vol_tlv, -4350, 150, 0);

static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot0_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_da_slot1_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot0_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(aif1_ad_slot1_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(aif2_ad_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif2_da_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(aif2_ad_mix_vol_tlv, -600, 600, 0);

static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -11925, 75, 0);
static const DECLARE_TLV_DB_SCALE(dac_mix_vol_tlv, -600, 600, 0);
static const DECLARE_TLV_DB_SCALE(dig_vol_tlv, -7308, 116, 0);

static const DECLARE_TLV_DB_SCALE(mic1_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic1_boost_vol_tlv, 0, 200, 0);
static const DECLARE_TLV_DB_SCALE(mic2_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(mic2_boost_vol_tlv, 0, 200, 0);
static const DECLARE_TLV_DB_SCALE(linein_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(adc_input_vol_tlv, -450, 150, 0);

struct aif1_fs {
	unsigned int samplerate;
	int aif1_bclk_div;
	int aif1_srbit;
};

struct aif1_lrck {
	int aif1_lrlk_div;
	int aif1_lrlk_bit;
};

struct aif1_word_size {
	int aif1_wsize_val;
	int aif1_wsize_bit;
};

/*struct for audiocodec*/
struct codec_priv {
	u8 dac_enable;
	u8 adc_enable;
	u8 aif1_clken;
	u8 aif2_clken;
	u8 aif3_clken;

	struct mutex dac_mutex;
	struct mutex adc_mutex;
	struct mutex aifclk_mutex;
	struct snd_soc_codec *codec;
};

static const struct aif1_fs codec_aif1_fs[] = {
	{44100, 4, 7},
	{48000, 4, 8},
	{8000, 9, 0},
	{11025, 8, 1},
	{12000, 8, 2},
	{16000, 7, 3},
	{22050, 6, 4},
	{24000, 6, 5},
	{32000, 5, 6},
	{96000, 2, 9},
	{192000, 1, 10},
};

static const struct aif1_lrck codec_aif1_lrck[] = {
	{16, 0},
	{32, 1},
	{64, 2},
	{128, 3},
	{256, 4},
};

static const struct aif1_word_size codec_aif1_wsize[] = {
	{8, 0},
	{16, 1},
	{20, 2},
	{24, 3},
};

static struct label reg_labels[]={
        LABEL(SUNXI_DA_CTL),
        LABEL(SUNXI_DA_FAT0),
        LABEL(SUNXI_DA_FAT1),
        LABEL(SUNXI_DA_FCTL),
        LABEL(SUNXI_DA_INT),
        LABEL(SUNXI_DA_TXFIFO),
        LABEL(SUNXI_DA_CLKD),
        LABEL(SUNXI_DA_TXCNT),
        LABEL(SUNXI_DA_RXCNT),
		LABEL(SUNXI_SYSCLK_CTL),
		LABEL(SUNXI_MOD_CLK_ENA),
		LABEL(SUNXI_MOD_RST_CTL),
		LABEL(SUNXI_SYS_SR_CTRL),
		LABEL(SUNXI_SYS_SRC_CLK),
		LABEL(SUNXI_SYS_DVC_MOD),

		LABEL(SUNXI_AIF1_CLK_CTRL),
		LABEL(SUNXI_AIF1_ADCDAT_CTRL),
		LABEL(SUNXI_AIF1_DACDAT_CTRL),
		LABEL(SUNXI_AIF1_MXR_SRC),
		LABEL(SUNXI_AIF1_VOL_CTRL1),
		LABEL(SUNXI_AIF1_VOL_CTRL2),
		LABEL(SUNXI_AIF1_VOL_CTRL3),
		LABEL(SUNXI_AIF1_VOL_CTRL4),
		LABEL(SUNXI_AIF1_MXR_GAIN),
		LABEL(SUNXI_AIF1_RXD_CTRL),
		LABEL(SUNXI_AIF2_CLK_CTRL),
		LABEL(SUNXI_AIF2_ADCDAT_CTRL),
		LABEL(SUNXI_AIF2_DACDAT_CTRL),
		LABEL(SUNXI_AIF2_MXR_SRC),
		LABEL(SUNXI_AIF2_VOL_CTRL1),
		LABEL(SUNXI_AIF2_VOL_CTRL2),
		LABEL(SUNXI_AIF2_MXR_GAIN),
		LABEL(SUNXI_AIF2_RXD_CTRL),
		LABEL(SUNXI_AIF3_CLK_CTRL),
		LABEL(SUNXI_AIF3_ADCDAT_CTRL),
		LABEL(SUNXI_AIF3_DACDAT_CTRL),
		LABEL(SUNXI_AIF3_SGP_CTRL),
		LABEL(SUNXI_AIF3_RXD_CTRL),
		LABEL(SUNXI_ADC_DIG_CTRL),
		LABEL(SUNXI_ADC_VOL_CTRL),
		LABEL(SUNXI_ADC_DBG_CTRL),
		LABEL(SUNXI_HMIC_CTRL1),
		LABEL(SUNXI_HMIC_CTRL2),
		LABEL(SUNXI_HMIC_STS),
		LABEL(SUNXI_DAC_DIG_CTRL),
		LABEL(SUNXI_DAC_VOL_CTRL),
		LABEL(SUNXI_DAC_DBG_CTRL),
		LABEL(SUNXI_DAC_MXR_SRC),
		LABEL(SUNXI_DAC_MXR_GAIN),
		LABEL(SUNXI_AGC_ENA),
		LABEL(SUNXI_DRC_ENA),

        LABEL(HP_CTRL),
        LABEL(OL_MIX_CTRL),
        LABEL(OR_MIX_CTRL),
        LABEL(EARPIECE_CTRL0),
        LABEL(EARPIECE_CTRL1),
		LABEL(SPKOUT_CTRL0),
        LABEL(SPKOUT_CTRL1),
        LABEL(MIC1_CTRL),
        LABEL(MIC2_CTRL),
        LABEL(LINEIN_CTRL),
        LABEL(MIX_DAC_CTRL),
        LABEL(L_ADCMIX_SRC),
        LABEL(R_ADCMIX_SRC),
        LABEL(ADC_CTRL),
		LABEL(HS_MBIAS_CTRL),
		LABEL(APT_REG),
		LABEL(OP_BIAS_CTRL0),
		LABEL(OP_BIAS_CTRL1),
		LABEL(ZC_VOL_CTRL),
		LABEL(BIAS_CAL_DATA),
		LABEL(BIAS_CAL_SET),
		LABEL(BD_CAL_CTRL),
		LABEL(HP_PA_CTRL),
		LABEL(RHP_CAL_DAT),
		LABEL(RHP_CAL_SET),
		LABEL(LHP_CAL_DAT),
		LABEL(LHP_CAL_SET),
		LABEL(MDET_CTRL),
		LABEL(JACK_MIC_CTRL),
       	LABEL_END,
};

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
}

int ac_aif1clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct codec_priv *sun8iw9_codec = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sun8iw9_codec->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sun8iw9_codec->aif1_clken == 0) {
			/*enable AIF1CLK*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF1CLK_ENA), (0x1<<AIF1CLK_ENA));
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF1_MOD_CLK_EN), (0x1<<AIF1_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF1_MOD_RST_CTL), (0x1<<AIF1_MOD_RST_CTL));
			/*enable systemclk*/
			if (sun8iw9_codec->aif2_clken == 0 && sun8iw9_codec->aif3_clken == 0) {
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
			}
		}
		sun8iw9_codec->aif1_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sun8iw9_codec->aif1_clken > 0) {
			sun8iw9_codec->aif1_clken--;
			if (sun8iw9_codec->aif1_clken == 0) {
				/*disable AIF1CLK*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF1CLK_ENA), (0x0<<AIF1CLK_ENA));
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF1_MOD_CLK_EN), (0x0<<AIF1_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF1_MOD_RST_CTL), (0x0<<AIF1_MOD_RST_CTL));
				/*DISABLE systemclk*/
				if (sun8iw9_codec->aif2_clken == 0 && sun8iw9_codec->aif3_clken == 0) {
					snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
				}
			}
		}
		break;
	}
	mutex_unlock(&sun8iw9_codec->aifclk_mutex);
	return 0;
}

int ac_aif2clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct codec_priv *sun8iw9_codec = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sun8iw9_codec->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sun8iw9_codec->aif2_clken == 0) {
			/*enable AIF2CLK*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x1<<AIF2_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x1<<AIF2_MOD_RST_CTL));
			/*enable systemclk*/
			if (sun8iw9_codec->aif1_clken == 0 && sun8iw9_codec->aif3_clken == 0) {
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
			}
		}
		sun8iw9_codec->aif2_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sun8iw9_codec->aif2_clken > 0) {
			sun8iw9_codec->aif2_clken--;
			if (sun8iw9_codec->aif2_clken == 0) {
				/*disable AIF2CLK*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x0<<AIF2CLK_ENA));
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x0<<AIF2_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x0<<AIF2_MOD_RST_CTL));
				/*DISABLE systemclk*/
				if (sun8iw9_codec->aif1_clken == 0 && sun8iw9_codec->aif3_clken == 0) {
					snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
				}
			}
		}
		break;
	}
	mutex_unlock(&sun8iw9_codec->aifclk_mutex);
	return 0;
}

int ac_aif3clk(struct snd_soc_dapm_widget *w,
		  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct codec_priv *sun8iw9_codec = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sun8iw9_codec->aifclk_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sun8iw9_codec->aif2_clken == 0) {
			/*enable AIF2CLK*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x1<<AIF2CLK_ENA));
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x1<<AIF2_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x1<<AIF2_MOD_RST_CTL));
			/*enable systemclk*/
			if (sun8iw9_codec->aif1_clken == 0 && sun8iw9_codec->aif3_clken == 0) {
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x1<<SYSCLK_ENA));
			}
		}
		sun8iw9_codec->aif2_clken++;
		if (sun8iw9_codec->aif3_clken == 0) {
			/*enable AIF3CLK*/
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF3_MOD_CLK_EN), (0x1<<AIF3_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF3_MOD_RST_CTL), (0x1<<AIF3_MOD_RST_CTL));
		}
		sun8iw9_codec->aif3_clken++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sun8iw9_codec->aif2_clken > 0) {
			sun8iw9_codec->aif2_clken--;
			if (sun8iw9_codec->aif2_clken == 0) {
				/*disable AIF2CLK*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<AIF2CLK_ENA), (0x0<<AIF2CLK_ENA));
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF2_MOD_CLK_EN), (0x0<<AIF2_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF2_MOD_RST_CTL), (0x0<<AIF2_MOD_RST_CTL));
				/*disable systemclk*/
				if (sun8iw9_codec->aif1_clken == 0 && sun8iw9_codec->aif3_clken == 0) {
					snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_ENA), (0x0<<SYSCLK_ENA));
				}
			}
		}
		if (sun8iw9_codec->aif3_clken > 0) {
			sun8iw9_codec->aif3_clken--;
			if (sun8iw9_codec->aif3_clken == 0) {
				/*disable AIF3CLK*/
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<AIF3_MOD_CLK_EN), (0x0<<AIF3_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<AIF3_MOD_RST_CTL), (0x0<<AIF3_MOD_RST_CTL));
			}
		}
		break;
	}
	mutex_unlock(&sun8iw9_codec->aifclk_mutex);
	return 0;
}

static int late_enable_dac(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct codec_priv *sun8iw9_codec = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sun8iw9_codec->dac_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sun8iw9_codec->dac_enable == 0) {
			/*enable dac module clk*/
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x1<<DAC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x1<<DAC_DIGITAL_MOD_RST_CTL));
			snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x1<<ENDA));
		}
		sun8iw9_codec->dac_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sun8iw9_codec->dac_enable > 0) {
			sun8iw9_codec->dac_enable--;
			if (sun8iw9_codec->dac_enable == 0) {
				snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x0<<ENDA));
				/*disable dac module clk*/
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x0<<DAC_DIGITAL_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x0<<DAC_DIGITAL_MOD_RST_CTL));
			}
		}
		break;
	}
	mutex_unlock(&sun8iw9_codec->dac_mutex);
	return 0;
}

static int late_enable_adc(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct codec_priv *sun8iw9_codec = snd_soc_codec_get_drvdata(codec);
	mutex_lock(&sun8iw9_codec->adc_mutex);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (sun8iw9_codec->adc_enable == 0) {
			/*enable adc module clk*/
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<ADC_DIGITAL_MOD_CLK_EN), (0x1<<ADC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<ADC_DIGITAL_MOD_RST_CTL), (0x1<<ADC_DIGITAL_MOD_RST_CTL));
			snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENAD), (0x1<<ENAD));
		}
		sun8iw9_codec->adc_enable++;
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (sun8iw9_codec->adc_enable > 0) {
			sun8iw9_codec->adc_enable--;
			if (sun8iw9_codec->adc_enable == 0) {
				snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENAD), (0x0<<ENAD));
				/*disable adc module clk*/
				snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<ADC_DIGITAL_MOD_CLK_EN), (0x0<<ADC_DIGITAL_MOD_CLK_EN));
				snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<ADC_DIGITAL_MOD_RST_CTL), (0x0<<ADC_DIGITAL_MOD_RST_CTL));
			}
		}
		break;
	}
	mutex_unlock(&sun8iw9_codec->adc_mutex);
	return 0;
}

static int ac_headphone_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *k,	int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*open*/
		snd_soc_update_bits(codec, HP_PA_CTRL, (0xf<<HPOUTPUTENABLE), (0xf<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, HP_CTRL, (0x1<<HPPA_EN), (0x1<<HPPA_EN));
		msleep(10);
		snd_soc_update_bits(codec, MIX_DAC_CTRL, (0x3<<LHPPAMUTE), (0x3<<LHPPAMUTE));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		/*close*/
		snd_soc_update_bits(codec, HP_CTRL, (0x1<<HPPA_EN), (0x0<<HPPA_EN));
		snd_soc_update_bits(codec, HP_PA_CTRL, (0xf<<HPOUTPUTENABLE), (0x0<<HPOUTPUTENABLE));
		snd_soc_update_bits(codec, MIX_DAC_CTRL, (0x3<<LHPPAMUTE), (0x0<<LHPPAMUTE));
		break;
	}
	return 0;
}

static int aif2inl_vir_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x1<<AIF2_DAC_SRC));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x0<<AIF2_DAC_SRC));
		break;
	}
	return 0;
}

static int aif2inr_vir_event(struct snd_soc_dapm_widget *w,
			  struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x2<<AIF2_DAC_SRC));
		break;
	case SND_SOC_DAPM_POST_PMD:
		snd_soc_update_bits(codec, SUNXI_AIF3_SGP_CTRL, (0x3<<AIF2_DAC_SRC), (0x0<<AIF2_DAC_SRC));
		break;
	}
	return 0;
}

static int dmic_mux_ev(struct snd_soc_dapm_widget *w,
		      struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	switch (event){
	case SND_SOC_DAPM_PRE_PMU:
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x1<<DAC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x1<<DAC_DIGITAL_MOD_RST_CTL));
			snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x1<<ENDA));
			snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENDM), (0x1<<ENDM));
		break;
	case SND_SOC_DAPM_POST_PMD:
			snd_soc_update_bits(codec, SUNXI_MOD_CLK_ENA, (0x1<<DAC_DIGITAL_MOD_CLK_EN), (0x0<<DAC_DIGITAL_MOD_CLK_EN));
			snd_soc_update_bits(codec, SUNXI_MOD_RST_CTL, (0x1<<DAC_DIGITAL_MOD_RST_CTL), (0x0<<DAC_DIGITAL_MOD_RST_CTL));
			snd_soc_update_bits(codec, SUNXI_DAC_DIG_CTRL, (0x1<<ENDA), (0x0<<ENDA));
			snd_soc_update_bits(codec, SUNXI_ADC_DIG_CTRL, (0x1<<ENDM), (0x0<<ENDM));
		break;
	}
	pr_debug("%s,line:%d,SUNXI_ADC_DIG_CTRL(300):%x\n", __func__, __LINE__,snd_soc_read(codec,SUNXI_ADC_DIG_CTRL));
	return 0;
}

static const struct snd_kcontrol_new sunxi_codec_controls[] = {
	/*AIF1*/
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 volume", SUNXI_AIF1_VOL_CTRL1, AIF1_AD0L_VOL, AIF1_AD0R_VOL, 0xff, 0, aif1_ad_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 volume", SUNXI_AIF1_VOL_CTRL2, AIF1_AD1L_VOL, AIF1_AD1R_VOL, 0xff, 0, aif1_ad_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 0 volume", SUNXI_AIF1_VOL_CTRL3, AIF1_DA0L_VOL, AIF1_DA0R_VOL, 0xff, 0, aif1_da_slot0_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 DAC timeslot 1 volume", SUNXI_AIF1_VOL_CTRL4, AIF1_DA1L_VOL, AIF1_DA1R_VOL, 0xff, 0, aif1_da_slot1_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 0 mixer gain", SUNXI_AIF1_MXR_GAIN, AIF1_AD0L_MXR_GAIN, AIF1_AD0R_MXR_GAIN, 0xf, 0, aif1_ad_slot0_mix_vol_tlv),
	SOC_DOUBLE_TLV("AIF1 ADC timeslot 1 mixer gain", SUNXI_AIF1_MXR_GAIN, AIF1_AD1L_MXR_GAIN, AIF1_AD1R_MXR_GAIN, 0x3, 0, aif1_ad_slot1_mix_vol_tlv),
	/*AIF2*/
	SOC_DOUBLE_TLV("AIF2 ADC volume", SUNXI_AIF2_VOL_CTRL1, AIF2_ADCL_VOL,AIF2_ADCR_VOL, 0xff, 0, aif2_ad_vol_tlv),
	SOC_DOUBLE_TLV("AIF2 DAC volume", SUNXI_AIF2_VOL_CTRL2, AIF2_DACL_VOL,AIF2_DACR_VOL, 0xff, 0, aif2_da_vol_tlv),
	SOC_DOUBLE_TLV("AIF2 ADC mixer gain", SUNXI_AIF2_MXR_GAIN, AIF2_ADCL_MXR_GAIN,AIF2_ADCR_MXR_GAIN, 0xf, 0, aif2_ad_mix_vol_tlv),
	/*ADC*/
	SOC_DOUBLE_TLV("ADC volume", SUNXI_ADC_VOL_CTRL, ADC_VOL_L, ADC_VOL_R, 0xff, 0, adc_vol_tlv),
	/*DAC*/
	SOC_DOUBLE_TLV("DAC volume", SUNXI_DAC_VOL_CTRL, DAC_VOL_L, DAC_VOL_R, 0xff, 0, dac_vol_tlv),
	SOC_DOUBLE_TLV("DAC mixer gain", SUNXI_DAC_MXR_GAIN, DACL_MXR_GAIN, DACR_MXR_GAIN, 0xf, 0, dac_mix_vol_tlv),

	SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DBG_CTRL, DVC, 0x3f, 0, dig_vol_tlv),

	/*analog control*/
	SOC_SINGLE_TLV("earpiece volume", EARPIECE_CTRL1, ESP_VOL, 0x1f, 0, earpiece_vol_tlv),
	SOC_SINGLE_TLV("speaker volume", SPKOUT_CTRL1, SPKOUT_VOL, 0x1f, 0, speaker_vol_tlv),
	SOC_SINGLE_TLV("headphone volume", HP_CTRL, HPVOL, 0x3f, 0, headphone_vol_tlv),
	
	SOC_SINGLE_TLV("MIC1_G boost stage output mixer control", MIC1_CTRL, MIC1G, 0x7, 0, mic1_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC1 boost AMP gain control", MIC1_CTRL, MIC1BOOST, 0x7, 0, mic1_boost_vol_tlv),

	SOC_SINGLE_TLV("MIC2 BST stage to L_R outp mixer gain", MIC2_CTRL, MIC2G, 0x7, 0, mic2_to_l_r_mix_vol_tlv),
	SOC_SINGLE_TLV("MIC2 boost AMP gain control", MIC2_CTRL, MIC2BOOST, 0x7, 0, mic2_boost_vol_tlv),

	SOC_SINGLE_TLV("LINEINL/R to L_R output mixer gain", LINEIN_CTRL, LINEING, 0x7, 0, linein_to_l_r_mix_vol_tlv),
	/*ADC*/
	SOC_SINGLE_TLV("ADC input gain control", ADC_CTRL, ADCG, 0x7, 0, adc_input_vol_tlv),
};

/*0x244:AIF1 AD0 OUT */
static const char *aif1out0l_text[] = {
	"AIF1_AD0L", "AIF1_AD0R","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};
static const char *aif1out0r_text[] = {
	"AIF1_AD0R", "AIF1_AD0L","SUM_AIF1AD0L_AIF1AD0R", "AVE_AIF1AD0L_AIF1AD0R"
};

static const struct soc_enum aif1out0l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 10, 4, aif1out0l_text);

static const struct snd_kcontrol_new aif1out0l_mux =
	SOC_DAPM_ENUM("AIF1OUT0L Mux", aif1out0l_enum);

static const struct soc_enum aif1out0r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 8, 4, aif1out0r_text);

static const struct snd_kcontrol_new aif1out0r_mux =
	SOC_DAPM_ENUM("AIF1OUT0R Mux", aif1out0r_enum);

/*0x244:AIF1 AD1 OUT */
static const char *aif1out1l_text[] = {
	"AIF1_AD1L", "AIF1_AD1R","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};
static const char *aif1out1r_text[] = {
	"AIF1_AD1R", "AIF1_AD1L","SUM_AIF1ADC1L_AIF1ADC1R", "AVE_AIF1ADC1L_AIF1ADC1R"
};

static const struct soc_enum aif1out1l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 6, 4, aif1out1l_text);

static const struct snd_kcontrol_new aif1out1l_mux =
	SOC_DAPM_ENUM("AIF1OUT1L Mux", aif1out1l_enum);

static const struct soc_enum aif1out1r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_ADCDAT_CTRL, 4, 4, aif1out1r_text);

static const struct snd_kcontrol_new aif1out1r_mux =
	SOC_DAPM_ENUM("AIF1OUT1R Mux", aif1out1r_enum);

/*0x248:AIF1 DA0 IN*/
static const char *aif1in0l_text[] = {
	"AIF1_DA0L", "AIF1_DA0R", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};
static const char *aif1in0r_text[] = {
	"AIF1_DA0R", "AIF1_DA0L", "SUM_AIF1DA0L_AIF1DA0R", "AVE_AIF1DA0L_AIF1DA0R"
};

static const struct soc_enum aif1in0l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 10, 4, aif1in0l_text);

static const struct snd_kcontrol_new aif1in0l_mux =
	SOC_DAPM_ENUM("AIF1IN0L Mux", aif1in0l_enum);

static const struct soc_enum aif1in0r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 8, 4, aif1in0r_text);

static const struct snd_kcontrol_new aif1in0r_mux =
	SOC_DAPM_ENUM("AIF1IN0R Mux", aif1in0r_enum);

/*0x248:AIF1 DA1 IN*/
static const char *aif1in1l_text[] = {
	"AIF1_DA1L", "AIF1_DA1R","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};
static const char *aif1in1r_text[] = {
	"AIF1_DA1R", "AIF1_DA1L","SUM_AIF1DA1L_AIF1DA1R", "AVE_AIF1DA1L_AIF1DA1R"
};

static const struct soc_enum aif1in1l_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 6, 4, aif1in1l_text);

static const struct snd_kcontrol_new aif1in1l_mux =
	SOC_DAPM_ENUM("AIF1IN1L Mux", aif1in1l_enum);

static const struct soc_enum aif1in1r_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF1_DACDAT_CTRL, 4, 4, aif1in1r_text);

static const struct snd_kcontrol_new aif1in1r_mux =
	SOC_DAPM_ENUM("AIF1IN1R Mux", aif1in1r_enum);

/*0x24c:AIF1 ADC0 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad0l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF1DA0L, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_ADCL, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0L_MXL_SRC_AIF2DACR, 1, 0),
};
static const struct snd_kcontrol_new aif1_ad0r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF1DA0R, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_ADCR, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD0R_MXR_SRC_AIF2DACL, 1, 0),
};

/*0x24c:AIF1 ADC1 MIXER SOURCE*/
static const struct snd_kcontrol_new aif1_ad1l_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD1L_MXR_AIF2_DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD1L_MXR_ADCL, 1, 0),
};
static const struct snd_kcontrol_new aif1_ad1r_mxr_src_ctl[] = {
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF1_MXR_SRC, AIF1_AD1R_MXR_AIF2_DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_AIF1_MXR_SRC, AIF1_AD1R_MXR_ADCR, 1, 0),
};

/*0x330 dac digital mixer source select*/
static const struct snd_kcontrol_new dacl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_DAC_MXR_SRC,  DACL_MXR_SRC_ADCL, 1, 0),
	SOC_DAPM_SINGLE("AIF2DACL Switch", 	SUNXI_DAC_MXR_SRC, 	DACL_MXR_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1L Switch", 	SUNXI_DAC_MXR_SRC, 	DACL_MXR_SRC_AIF1DA1L, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0L Switch", 	SUNXI_DAC_MXR_SRC, 	DACL_MXR_SRC_AIF1DA0L, 1, 0),
};
static const struct snd_kcontrol_new dacr_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_DAC_MXR_SRC,  DACR_MXR_SRC_ADCR, 1, 0),
	SOC_DAPM_SINGLE("AIF2DACR Switch", 	SUNXI_DAC_MXR_SRC, 	DACR_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA1R Switch", 	SUNXI_DAC_MXR_SRC, 	DACR_MXR_SRC_AIF1DA1R, 1, 0),
	SOC_DAPM_SINGLE("AIF1DA0R Switch", 	SUNXI_DAC_MXR_SRC, 	DACR_MXR_SRC_AIF1DA0R, 1, 0),
};

/*output mixer source select*/
/*analog:0x01:defined left output mixer*/
static const struct snd_kcontrol_new ac_loutmix_controls[] = {
	SOC_DAPM_SINGLE("DACR Switch", OL_MIX_CTRL, LMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("DACL Switch", OL_MIX_CTRL, LMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", OL_MIX_CTRL, LMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OL_MIX_CTRL, LMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OL_MIX_CTRL, LMIXMUTEMIC1BOOST, 1, 0),
};

/*analog:0x02:defined right output mixer*/
static const struct snd_kcontrol_new ac_routmix_controls[] = {
	SOC_DAPM_SINGLE("DACL Switch", OR_MIX_CTRL, RMIXMUTEDACL, 1, 0),
	SOC_DAPM_SINGLE("DACR Switch", OR_MIX_CTRL, RMIXMUTEDACR, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", OR_MIX_CTRL, RMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("MIC2Booststage Switch", OR_MIX_CTRL, RMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC1Booststage Switch", OR_MIX_CTRL, RMIXMUTEMIC1BOOST, 1, 0),
};

/*hp source select*/
/*0x0a:headphone input source*/
static const char *ac_hp_r_func_sel[] = {
	"DACR HPR Switch", "Right Analog Mixer HPR Switch"};
static const struct soc_enum ac_hp_r_func_enum =
	SOC_ENUM_SINGLE(MIX_DAC_CTRL, RHPIS, 2, ac_hp_r_func_sel);

static const struct snd_kcontrol_new ac_hp_r_func_controls =
	SOC_DAPM_ENUM("HP_R Mux", ac_hp_r_func_enum);

static const char *ac_hp_l_func_sel[] = {
	"DACL HPL Switch", "Left Analog Mixer HPL Switch"};
static const struct soc_enum ac_hp_l_func_enum =
	SOC_ENUM_SINGLE(MIX_DAC_CTRL, LHPIS, 2, ac_hp_l_func_sel);

static const struct snd_kcontrol_new ac_hp_l_func_controls =
	SOC_DAPM_ENUM("HP_L Mux", ac_hp_l_func_enum);

/*0x05:spk source select*/
static const char *ac_rspks_func_sel[] = {
	"MIXER Switch", "MIXR MIXL Switch"};
static const struct soc_enum ac_rspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL0, RIGHT_SPK_SRC_SEL, 2, ac_rspks_func_sel);

static const struct snd_kcontrol_new ac_rspks_func_controls =
	SOC_DAPM_ENUM("SPK_R Mux", ac_rspks_func_enum);

static const char *ac_lspks_l_func_sel[] = {
	"MIXEL Switch", "MIXL MIXR  Switch"};
static const struct soc_enum ac_lspks_func_enum =
	SOC_ENUM_SINGLE(SPKOUT_CTRL0, LEFT_SPK_SRC_SEL, 2, ac_lspks_l_func_sel);

static const struct snd_kcontrol_new ac_lspks_func_controls =
	SOC_DAPM_ENUM("SPK_L Mux", ac_lspks_func_enum);

/*0x03:earpiece source select*/
static const char *ac_earpiece_func_sel[] = {
	"DACR", "DACL", "Right Analog Mixer", "Left Analog Mixer"};
static const struct soc_enum ac_earpiece_func_enum =
	SOC_ENUM_SINGLE(EARPIECE_CTRL0, ESPSR, 4, ac_earpiece_func_sel);

static const struct snd_kcontrol_new ac_earpiece_func_controls =
	SOC_DAPM_ENUM("EAR Mux", ac_earpiece_func_enum);

/*0x284:AIF2 out*/
static const char *aif2outl_text[] = {
	"AIF2_ADCL", "AIF2_ADCR","SUM_AIF2_ADCL_AIF2_ADCR", "AVE_AIF2_ADCL_AIF2_ADCR"
};
static const char *aif2outr_text[] = {
	"AIF2_ADCR", "AIF2_ADCL","SUM_AIF2_ADCL_AIF2_ADCR", "AVE_AIF2_ADCL_AIF2_ADCR"
};

static const struct soc_enum aif2outl_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_ADCDAT_CTRL, 10, 4, aif2outl_text);

static const struct snd_kcontrol_new aif2outl_mux =
	SOC_DAPM_ENUM("AIF2OUTL Mux", aif2outl_enum);

static const struct soc_enum aif2outr_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_ADCDAT_CTRL, 8, 4, aif2outr_text);

static const struct snd_kcontrol_new aif2outr_mux =
	SOC_DAPM_ENUM("AIF2OUTR Mux", aif2outr_enum);

/*0x288:AIF2 IN*/
static const char *aif2inl_text[] = {
	"AIF2_DACL", "AIF2_DACR","SUM_AIF2DACL_AIF2DACR", "AVE_AIF2DACL_AIF2DACR"
};
static const char *aif2inr_text[] = {
	"AIF2_DACR", "AIF2_DACL","SUM_AIF2DACL_AIF2DACR", "AVE_AIF2DACL_AIF2DACR"
};

static const struct soc_enum aif2inl_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_DACDAT_CTRL, 10, 4, aif2inl_text);
static const struct snd_kcontrol_new aif2inl_mux =
	SOC_DAPM_ENUM("AIF2INL Mux", aif2inl_enum);

static const struct soc_enum aif2inr_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF2_DACDAT_CTRL, 8, 4, aif2inr_text);
static const struct snd_kcontrol_new aif2inr_mux =
	SOC_DAPM_ENUM("AIF2INR Mux", aif2inr_enum);

/*0x28c:AIF2 source select*/
static const struct snd_kcontrol_new aif2_adcl_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("AIF1 DA0L Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_AIF1DA0L, 1, 0),
	SOC_DAPM_SINGLE("AIF1 DA1L Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_AIF1DA1L, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACR Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_AIF2DACR, 1, 0),
	SOC_DAPM_SINGLE("ADCL Switch", 		SUNXI_AIF2_MXR_SRC, AIF2_ADCL_MXR_SRC_ADCL, 1, 0),
};
static const struct snd_kcontrol_new aif2_adcr_mxr_src_controls[] = {
	SOC_DAPM_SINGLE("AIF1 DA0R Switch", SUNXI_AIF2_MXR_SRC,	AIF2_ADCR_MXR_SRC_AIF1DA0R, 1, 0),
	SOC_DAPM_SINGLE("AIF1 DA1R Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_AIF1DA1R, 1, 0),
	SOC_DAPM_SINGLE("AIF2 DACL Switch", SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_AIF2DACL, 1, 0),
	SOC_DAPM_SINGLE("ADCR Switch", 		SUNXI_AIF2_MXR_SRC, AIF2_ADCR_MXR_SRC_ADCR, 1, 0),
};

/*0x2cc:aif3 out, AIF3 PCM output source select*/
static const char *aif3out_text[] = {
	"AIF2 ADC left channel", "AIF2 ADC right channel"
};

static const unsigned int aif3out_values[] = {1,2};

static const struct soc_enum aif3out_enum =
		SOC_VALUE_ENUM_SINGLE(SUNXI_AIF3_SGP_CTRL, 10, 3,
		ARRAY_SIZE(aif3out_text),aif3out_text, aif3out_values);

static const struct snd_kcontrol_new aif3out_mux =
	SOC_DAPM_ENUM("AIF3OUT Mux", aif3out_enum);

/*0x2cc:aif2 DAC input source select*/
static const char *aif2dacin_text[] = {
	"Left_s right_s AIF2","Left_s AIF3 Right_s AIF2", "Left_s AIF2 Right_s AIF3"
};

static const struct soc_enum aif2dacin_enum =
	SOC_ENUM_SINGLE(SUNXI_AIF3_SGP_CTRL, 8, 3, aif2dacin_text);

static const struct snd_kcontrol_new aif2dacin_mux =
	SOC_DAPM_ENUM("AIF2 DAC SRC Mux", aif2dacin_enum);

/*ADC SOURCE SELECT*/
/*0x0b:defined left input adc mixer*/
static const struct snd_kcontrol_new ac_ladcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", L_ADCMIX_SRC, LADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", L_ADCMIX_SRC, LADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LINEINL Switch", L_ADCMIX_SRC, LADCMIXMUTELINEINL, 1, 0),
	SOC_DAPM_SINGLE("l_output mixer Switch", L_ADCMIX_SRC, LADCMIXMUTELOUTPUT, 1, 0),
	SOC_DAPM_SINGLE("r_output mixer Switch", L_ADCMIX_SRC, LADCMIXMUTEROUTPUT, 1, 0),
};

/*0x0c:defined right input adc mixer*/
static const struct snd_kcontrol_new ac_radcmix_controls[] = {
	SOC_DAPM_SINGLE("MIC1 boost Switch", R_ADCMIX_SRC, RADCMIXMUTEMIC1BOOST, 1, 0),
	SOC_DAPM_SINGLE("MIC2 boost Switch", R_ADCMIX_SRC, RADCMIXMUTEMIC2BOOST, 1, 0),
	SOC_DAPM_SINGLE("LINEINR Switch", R_ADCMIX_SRC, RADCMIXMUTELINEINR, 1, 0),
	SOC_DAPM_SINGLE("r_output mixer Switch", R_ADCMIX_SRC, RADCMIXMUTEROUTPUT, 1, 0),
	SOC_DAPM_SINGLE("l_output mixer Switch", R_ADCMIX_SRC, RADCMIXMUTELOUTPUT, 1, 0),
};

/*0x08:mic2 source select*/
static const char *mic2src_text[] = {"MIC2","MIC3"};

static const struct soc_enum mic2src_enum =
	SOC_ENUM_SINGLE(MIC2_CTRL, 7, 2, mic2src_text);

static const struct snd_kcontrol_new mic2src_mux =
	SOC_DAPM_ENUM("MIC2 SRC", mic2src_enum);

/*DMIC*/
static const char *adc_mux_text[] = {
	"ADC",
	"DMIC",
};
static const struct soc_enum adc_enum =
	SOC_ENUM_SINGLE(0, 0, 2, adc_mux_text);
static const struct snd_kcontrol_new adcl_mux =
	SOC_DAPM_ENUM_VIRT("ADCL Mux", adc_enum);
static const struct snd_kcontrol_new adcr_mux =
	SOC_DAPM_ENUM_VIRT("ADCR Mux", adc_enum);

/*built widget*/
static const struct snd_soc_dapm_widget ac_dapm_widgets[] = {
	/*0x244*/
	SND_SOC_DAPM_MUX("AIF1OUT0L Mux", SUNXI_AIF1_ADCDAT_CTRL, 15, 0, &aif1out0l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT0R Mux", SUNXI_AIF1_ADCDAT_CTRL, 14, 0, &aif1out0r_mux),
	SND_SOC_DAPM_MUX("AIF1OUT1L Mux", SUNXI_AIF1_ADCDAT_CTRL, 13, 0, &aif1out1l_mux),
	SND_SOC_DAPM_MUX("AIF1OUT1R Mux", SUNXI_AIF1_ADCDAT_CTRL, 12, 0, &aif1out1r_mux),
	/*0x248*/
	SND_SOC_DAPM_MUX("AIF1IN0L Mux", SUNXI_AIF1_DACDAT_CTRL, 15, 0, &aif1in0l_mux),
	SND_SOC_DAPM_MUX("AIF1IN0R Mux", SUNXI_AIF1_DACDAT_CTRL, 14, 0, &aif1in0r_mux),
	SND_SOC_DAPM_MUX("AIF1IN1L Mux", SUNXI_AIF1_DACDAT_CTRL, 13, 0, &aif1in1l_mux),
	SND_SOC_DAPM_MUX("AIF1IN1R Mux", SUNXI_AIF1_DACDAT_CTRL, 12, 0, &aif1in1r_mux),
	/*0x24c*/
#ifdef AIF1_FPGA_LOOPBACK_TEST
	SND_SOC_DAPM_MIXER_E("AIF1 AD0L Mixer", SND_SOC_NOPM, 0, 0,
		aif1_ad0l_mxr_src_ctl, ARRAY_SIZE(aif1_ad0l_mxr_src_ctl), late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("AIF1 AD0R Mixer", SND_SOC_NOPM, 0, 0,
		aif1_ad0r_mxr_src_ctl, ARRAY_SIZE(aif1_ad0r_mxr_src_ctl), late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
#else
	SND_SOC_DAPM_MIXER("AIF1 AD0L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0l_mxr_src_ctl, ARRAY_SIZE(aif1_ad0l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD0R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad0r_mxr_src_ctl, ARRAY_SIZE(aif1_ad0r_mxr_src_ctl)),
#endif
	SND_SOC_DAPM_MIXER("AIF1 AD1L Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1l_mxr_src_ctl, ARRAY_SIZE(aif1_ad1l_mxr_src_ctl)),
	SND_SOC_DAPM_MIXER("AIF1 AD1R Mixer", SND_SOC_NOPM, 0, 0, aif1_ad1r_mxr_src_ctl, ARRAY_SIZE(aif1_ad1r_mxr_src_ctl)),
	/*analog:0x0a*/
	SND_SOC_DAPM_MIXER_E("DACL Mixer", MIX_DAC_CTRL, DACALEN, 0, dacl_mxr_src_controls, ARRAY_SIZE(dacl_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("DACR Mixer", MIX_DAC_CTRL, DACAREN, 0, dacr_mxr_src_controls, ARRAY_SIZE(dacr_mxr_src_controls),
		     	late_enable_dac, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*0x320:dac digital enble*/
	SND_SOC_DAPM_DAC("DAC En", NULL, SUNXI_DAC_DIG_CTRL, ENDA, 0),

	/*0x300:adc digital enble*/
	SND_SOC_DAPM_ADC("ADC En", NULL, SUNXI_ADC_DIG_CTRL, ENAD, 0),
	/*0x0a*/
	SND_SOC_DAPM_MIXER("Left Output Mixer", MIX_DAC_CTRL, LMIXEN, 0,
			ac_loutmix_controls, ARRAY_SIZE(ac_loutmix_controls)),
	SND_SOC_DAPM_MIXER("Right Output Mixer", MIX_DAC_CTRL, RMIXEN, 0,
			ac_routmix_controls, ARRAY_SIZE(ac_routmix_controls)),

	SND_SOC_DAPM_MUX("HP_R Mux", SND_SOC_NOPM, 0, 0,	&ac_hp_r_func_controls),
	SND_SOC_DAPM_MUX("HP_L Mux", SND_SOC_NOPM, 0, 0,	&ac_hp_l_func_controls),
	/*0x05*/
	SND_SOC_DAPM_MUX("SPK_R Mux", SPKOUT_CTRL0, SPKOUT_R_EN, 0,	&ac_rspks_func_controls),
	SND_SOC_DAPM_MUX("SPK_L Mux", SPKOUT_CTRL0, SPKOUT_L_EN, 0,	&ac_lspks_func_controls),

	SND_SOC_DAPM_PGA("SPK_LR Adder", SND_SOC_NOPM, 0, 0, NULL, 0),
	/*0x04*/
	SND_SOC_DAPM_MUX("EAR Mux", EARPIECE_CTRL1, ESPPA_MUTE, 0,	&ac_earpiece_func_controls),
	SND_SOC_DAPM_PGA("EAR PA", EARPIECE_CTRL1, ESPPA_EN, 0, NULL, 0),

	/*output widget*/
	SND_SOC_DAPM_OUTPUT("HPOUTL"),
	SND_SOC_DAPM_OUTPUT("HPOUTR"),
	SND_SOC_DAPM_OUTPUT("EAROUTP"),
	SND_SOC_DAPM_OUTPUT("EAROUTN"),
	/*spk is diff with ac100. need TODO...*/
	SND_SOC_DAPM_OUTPUT("SPK1P"),
	SND_SOC_DAPM_OUTPUT("SPK2P"),
	SND_SOC_DAPM_OUTPUT("SPK1N"),
	SND_SOC_DAPM_OUTPUT("SPK2N"),

	/*0x284*/
	SND_SOC_DAPM_MUX("AIF2OUTL Mux", SUNXI_AIF2_ADCDAT_CTRL, 15, 0, &aif2outl_mux),
	SND_SOC_DAPM_MUX("AIF2OUTR Mux", SUNXI_AIF2_ADCDAT_CTRL, 14, 0, &aif2outr_mux),
	/*0x288*/
	SND_SOC_DAPM_MUX("AIF2INL Mux", SUNXI_AIF2_DACDAT_CTRL, 15, 0, &aif2inl_mux),
	SND_SOC_DAPM_MUX("AIF2INR Mux", SUNXI_AIF2_DACDAT_CTRL, 14, 0, &aif2inr_mux),

	SND_SOC_DAPM_PGA("AIF2INL_VIR", SND_SOC_NOPM, 0, 0, NULL, 0),
	SND_SOC_DAPM_PGA("AIF2INR_VIR", SND_SOC_NOPM, 0, 0, NULL, 0),
	/*0x28c*/
	SND_SOC_DAPM_MIXER("AIF2 ADL Mixer", SND_SOC_NOPM, 0, 0, aif2_adcl_mxr_src_controls, ARRAY_SIZE(aif2_adcl_mxr_src_controls)),
	SND_SOC_DAPM_MIXER("AIF2 ADR Mixer", SND_SOC_NOPM, 0, 0, aif2_adcr_mxr_src_controls, ARRAY_SIZE(aif2_adcr_mxr_src_controls)),
	/*0x2cc*/
	SND_SOC_DAPM_MUX("AIF3OUT Mux", SND_SOC_NOPM, 0, 0, &aif3out_mux),
	SND_SOC_DAPM_MUX("AIF2 DAC SRC Mux", SND_SOC_NOPM, 0, 0, &aif2dacin_mux),
	/*0x2cc virtual widget*/
	SND_SOC_DAPM_PGA_E("AIF2INL Mux VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
			aif2inl_vir_event, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_E("AIF2INR Mux VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
			aif2inr_vir_event, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
#ifdef AIF1_FPGA_LOOPBACK_TEST
	/*0x0d 0x0b 0x0c ADC_CTRL*/
	SND_SOC_DAPM_MIXER("LEFT ADC input Mixer", ADC_CTRL, ADCLEN, 0, ac_ladcmix_controls, ARRAY_SIZE(ac_ladcmix_controls)),
	SND_SOC_DAPM_MIXER("RIGHT ADC input Mixer", ADC_CTRL, ADCREN, 0, ac_radcmix_controls, ARRAY_SIZE(ac_radcmix_controls)),
#else
	/*0x0d 0x0b 0x0c ADC_CTRL*/
	SND_SOC_DAPM_MIXER_E("LEFT ADC input Mixer", ADC_CTRL, ADCLEN, 0,
		ac_ladcmix_controls, ARRAY_SIZE(ac_ladcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_MIXER_E("RIGHT ADC input Mixer", ADC_CTRL, ADCREN, 0,
		ac_radcmix_controls, ARRAY_SIZE(ac_radcmix_controls),late_enable_adc, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
#endif
	/*0x07 mic1 reference*/
	SND_SOC_DAPM_PGA("MIC1 PGA", MIC1_CTRL, MIC1AMPEN, 0, NULL, 0),
	/*0x08 mic2 reference*/
	SND_SOC_DAPM_PGA("MIC2 PGA", MIC2_CTRL, MIC2AMPEN, 0, NULL, 0),

	SND_SOC_DAPM_PGA("LINEIN PGA", SND_SOC_NOPM, 0, 0, NULL, 0),
	/*0x08*/
	SND_SOC_DAPM_MUX("MIC2 SRC", SND_SOC_NOPM, 0, 0, &mic2src_mux),

	/*INPUT widget*/
	SND_SOC_DAPM_INPUT("MIC1P"),
	SND_SOC_DAPM_INPUT("MIC1N"),
	/*0x0e Headset Microphone Bias Control Register*/
	SND_SOC_DAPM_MICBIAS("MainMic Bias", HS_MBIAS_CTRL, MMICBIASEN, 0),
	SND_SOC_DAPM_INPUT("MIC2"),
	SND_SOC_DAPM_INPUT("MIC3"),

	SND_SOC_DAPM_INPUT("LINEINP"),
	SND_SOC_DAPM_INPUT("LINEINN"),

	/*aif1 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF1DACL", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("AIF1DACR", "AIF1 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCL", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_OUT_E("AIF1ADCR", "AIF1 Capture", 0, SND_SOC_NOPM, 0, 0,ac_aif1clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	/*aif2 interface*/
	SND_SOC_DAPM_AIF_IN_E("AIF2DACL", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif2clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_AIF_IN("AIF2DACR", "AIF2 Playback", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_AIF_OUT("AIF2ADCL", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF2ADCR", "AIF2 Capture", 0, SND_SOC_NOPM, 0, 0),

	/*aif3 interface*/
	SND_SOC_DAPM_AIF_OUT("AIF3OUT", "AIF3 Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN_E("AIF3IN", "AIF3 Playback", 0, SND_SOC_NOPM, 0, 0,ac_aif3clk,
		   SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
	/*headphone*/
	SND_SOC_DAPM_HP("Headphone", ac_headphone_event),
	
	/*DMIC*/
	SND_SOC_DAPM_VIRT_MUX("ADCL Mux", SND_SOC_NOPM, 0, 0, &adcl_mux),
	SND_SOC_DAPM_VIRT_MUX("ADCR Mux", SND_SOC_NOPM, 0, 0, &adcr_mux),

	SND_SOC_DAPM_PGA_E("DMICL VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),

	SND_SOC_DAPM_PGA_E("DMICR VIR", SND_SOC_NOPM, 0, 0, NULL, 0,
				dmic_mux_ev, SND_SOC_DAPM_PRE_PMU|SND_SOC_DAPM_POST_PMD),
};

static const struct snd_soc_dapm_route ac_dapm_routes[] = {
	{"AIF1ADCL", NULL, "AIF1OUT0L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT0R Mux"},

	{"AIF1ADCL", NULL, "AIF1OUT1L Mux"},
	{"AIF1ADCR", NULL, "AIF1OUT1R Mux"},

	/* aif1out0 mux 11---13*/
	{"AIF1OUT0L Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},
	{"AIF1OUT0L Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},

	{"AIF1OUT0R Mux", "AIF1_AD0R", "AIF1 AD0R Mixer"},
	{"AIF1OUT0R Mux", "AIF1_AD0L", "AIF1 AD0L Mixer"},

	/*AIF1OUT1 mux 11--13 */
	{"AIF1OUT1L Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},
	{"AIF1OUT1L Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},

	{"AIF1OUT1R Mux", "AIF1_AD1R", "AIF1 AD1R Mixer"},
	{"AIF1OUT1R Mux", "AIF1_AD1L", "AIF1 AD1L Mixer"},

	/*AIF1 AD0L Mixer*/
	{"AIF1 AD0L Mixer", "AIF1 DA0L Switch",		"AIF1IN0L Mux"},
	{"AIF1 AD0L Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},
	#ifdef AIF1_FPGA_LOOPBACK_TEST
	{"AIF1 AD0L Mixer", "ADCL Switch",		"MIC1P"},
	#else
	{"AIF1 AD0L Mixer", "ADCL Switch",		"ADCL Mux"},
	#endif
	{"AIF1 AD0L Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},

	/*AIF1 AD0R Mixer*/
	{"AIF1 AD0R Mixer", "AIF1 DA0R Switch",		"AIF1IN0R Mux"},
	{"AIF1 AD0R Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},

	#ifdef AIF1_FPGA_LOOPBACK_TEST
	{"AIF1 AD0R Mixer", "ADCR Switch",		"MIC1N"},
	#else
	{"AIF1 AD0R Mixer", "ADCR Switch",		"ADCR Mux"},
	#endif
	{"AIF1 AD0R Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},

	/*AIF1 AD1L Mixer*/
	{"AIF1 AD1L Mixer", "AIF2 DACL Switch",		"AIF2INL_VIR"},
	{"AIF1 AD1L Mixer", "ADCL Switch",		"ADCL Mux"},

	/*AIF1 AD1R Mixer*/
	{"AIF1 AD1R Mixer", "AIF2 DACR Switch",		"AIF2INR_VIR"},
	{"AIF1 AD1R Mixer", "ADCR Switch",		"ADCR Mux"},

	/*AIF1 DA0 IN 12h*/
	{"AIF1IN0L Mux", "AIF1_DA0L",		"AIF1DACL"},
	{"AIF1IN0L Mux", "AIF1_DA0R",		"AIF1DACR"},

	{"AIF1IN0R Mux", "AIF1_DA0R",		"AIF1DACR"},
	{"AIF1IN0R Mux", "AIF1_DA0L",		"AIF1DACL"},

	/*AIF1 DA1 IN 12h*/
	{"AIF1IN1L Mux", "AIF1_DA1L",		"AIF1DACL"},
	{"AIF1IN1L Mux", "AIF1_DA1R",		"AIF1DACR"},

	{"AIF1IN1R Mux", "AIF1_DA1R",		"AIF1DACR"},
	{"AIF1IN1R Mux", "AIF1_DA1L",		"AIF1DACL"},

	/*aif2 virtual*/
	{"AIF2INL_VIR", NULL,		"AIF2INL Mux"},
	{"AIF2INR_VIR", NULL,		"AIF2INR Mux"},

	{"AIF2INL_VIR", NULL,		"AIF2INL Mux VIR"},
	{"AIF2INR_VIR", NULL,		"AIF2INR Mux VIR"},

	/*4c*/
	{"DACL Mixer", "AIF1DA0L Switch",		"AIF1IN0L Mux"},
	{"DACL Mixer", "AIF1DA1L Switch",		"AIF1IN1L Mux"},

	{"DACL Mixer", "ADCL Switch",		"ADCL Mux"},
	{"DACL Mixer", "AIF2DACL Switch",		"AIF2INL_VIR"},
	{"DACR Mixer", "AIF1DA0R Switch",		"AIF1IN0R Mux"},
	{"DACR Mixer", "AIF1DA1R Switch",		"AIF1IN1R Mux"},

	{"DACR Mixer", "ADCR Switch",		"ADCR Mux"},
	{"DACR Mixer", "AIF2DACR Switch",		"AIF2INR_VIR"},

	{"Right Output Mixer", "DACR Switch",		"DACR Mixer"},
	{"Right Output Mixer", "DACL Switch",		"DACL Mixer"},

	{"Right Output Mixer", "LINEINR Switch",		"LINEINN"},
	{"Right Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Right Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	{"Left Output Mixer", "DACL Switch",		"DACL Mixer"},
	{"Left Output Mixer", "DACR Switch",		"DACR Mixer"},

	{"Left Output Mixer", "LINEINL Switch",		"LINEINP"},
	{"Left Output Mixer", "MIC2Booststage Switch",		"MIC2 PGA"},
	{"Left Output Mixer", "MIC1Booststage Switch",		"MIC1 PGA"},

	/*hp mux*/
	{"HP_R Mux", "DACR HPR Switch",		"DACR Mixer"},
	{"HP_R Mux", "Right Analog Mixer HPR Switch",		"Right Output Mixer"},

	{"HP_L Mux", "DACL HPL Switch",		"DACL Mixer"},
	{"HP_L Mux", "Left Analog Mixer HPL Switch",		"Left Output Mixer"},

	/*hp endpoint*/
	{"HPOUTR", NULL,				"HP_R Mux"},
	{"HPOUTL", NULL,				"HP_L Mux"},

	{"Headphone", NULL,				"HPOUTR"},
	{"Headphone", NULL,				"HPOUTL"},

	/*spk mux*/
	{"SPK_LR Adder", NULL,				"Right Output Mixer"},
	{"SPK_LR Adder", NULL,				"Left Output Mixer"},

	{"SPK_L Mux", "MIXL MIXR  Switch",				"SPK_LR Adder"},
	{"SPK_L Mux", "MIXEL Switch",				"Left Output Mixer"},

	{"SPK_R Mux", "MIXR MIXL Switch",				"SPK_LR Adder"},
	{"SPK_R Mux", "MIXER Switch",				"Right Output Mixer"},

	{"SPKR", NULL,				"SPK_R Mux"},
	{"SPKL", NULL,				"SPK_L Mux"},

	/*earpiece mux*/
	{"EAR Mux", "DACR",				"DACR Mixer"},
	{"EAR Mux", "DACL",				"DACL Mixer"},
	{"EAR Mux", "Right Analog Mixer",				"Right Output Mixer"},
	{"EAR Mux", "Left Analog Mixer",				"Left Output Mixer"},
	{"EAR PA", NULL,		"EAR Mux"},
	{"EAROUTP", NULL,		"EAR PA"},
	{"EAROUTN", NULL,		"EAR PA"},

	/*LADC SOURCE mixer*/
	{"LEFT ADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"LEFT ADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"LEFT ADC input Mixer", "LINEINL Switch",				"LINEINN"},
	{"LEFT ADC input Mixer", "l_output mixer Switch",				"Left Output Mixer"},
	{"LEFT ADC input Mixer", "r_output mixer Switch",				"Right Output Mixer"},

	/*RADC SOURCE mixer*/
	{"RIGHT ADC input Mixer", "MIC1 boost Switch",				"MIC1 PGA"},
	{"RIGHT ADC input Mixer", "MIC2 boost Switch",				"MIC2 PGA"},
	{"RIGHT ADC input Mixer", "LINEINR Switch",				"LINEINP"},
	{"RIGHT ADC input Mixer", "r_output mixer Switch",				"Right Output Mixer"},
	{"RIGHT ADC input Mixer", "l_output mixer Switch",				"Left Output Mixer"},

	{"MIC1 PGA", NULL,				"MIC1P"},
	{"MIC1 PGA", NULL,				"MIC1N"},

	{"MIC2 PGA", NULL,				"MIC2 SRC"},

	{"MIC2 SRC", "MIC2",				"MIC2"},
	{"MIC2 SRC", "MIC3",				"MIC3"},

	{"LINEIN PGA", NULL,				"LINEINP"},
	{"LINEIN PGA", NULL,				"LINEINN"},

	/*AIF2 out */
	{"AIF2ADCL", NULL,				"AIF2OUTL Mux"},
	{"AIF2ADCR", NULL,				"AIF2OUTR Mux"},

	{"AIF2OUTL Mux", "AIF2_ADCL",				"AIF2 ADL Mixer"},
	{"AIF2OUTL Mux", "AIF2_ADCR",				"AIF2 ADR Mixer"},

	{"AIF2OUTR Mux", "AIF2_ADCR",				"AIF2 ADR Mixer"},
	{"AIF2OUTR Mux", "AIF2_ADCL",				"AIF2 ADL Mixer"},

	/*23*/
	{"AIF2 ADL Mixer", "AIF1 DA0L Switch",				"AIF1IN0L Mux"},
	{"AIF2 ADL Mixer", "AIF1 DA1L Switch",				"AIF1IN1L Mux"},
	{"AIF2 ADL Mixer", "AIF2 DACR Switch",				"AIF2INR_VIR"},
	{"AIF2 ADL Mixer", "ADCL Switch",				"ADCL Mux"},
	
	{"AIF2 ADR Mixer", "AIF1 DA0R Switch",				"AIF1IN0R Mux"},
	{"AIF2 ADR Mixer", "AIF1 DA1R Switch",				"AIF1IN1R Mux"},
	{"AIF2 ADR Mixer", "AIF2 DACL Switch",				"AIF2INL_VIR"},
	{"AIF2 ADR Mixer", "ADCR Switch",				"ADCR Mux"},

	/*aif2*/
	{"AIF2INL Mux", "AIF2_DACL",				"AIF2DACL"},
	{"AIF2INL Mux", "AIF2_DACR",				"AIF2DACR"},

	{"AIF2INR Mux", "AIF2_DACR",				"AIF2DACR"},
	{"AIF2INR Mux", "AIF2_DACL",				"AIF2DACL"},

	/*aif3*/
	{"AIF2INL Mux VIR", NULL,				"AIF3IN"},
	{"AIF2INR Mux VIR", NULL,				"AIF3IN"},

	{"AIF3OUT", NULL,				"AIF3OUT Mux"},
	{"AIF3OUT Mux", "AIF2 ADC left channel",				"AIF2 ADL Mixer"},
	{"AIF3OUT Mux", "AIF2 ADC right channel",				"AIF2 ADR Mixer"},

	/*ADC--ADCMUX*/
	{"ADCR Mux", "ADC", "RIGHT ADC input Mixer"},
	{"ADCL Mux", "ADC", "LEFT ADC input Mixer"},

	/*DMIC*/
	{"ADCR Mux", "DMIC", "DMICR VIR"},
	{"ADCL Mux", "DMIC", "DMICL VIR"},

	{"DMICR VIR", NULL, "D_MIC"},
	{"DMICL VIR", NULL, "D_MIC"},
};

static int codec_aif_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	if (mute) {
		snd_soc_write(codec, SUNXI_DAC_VOL_CTRL, 0);
	} else {
		snd_soc_write(codec, SUNXI_DAC_VOL_CTRL, 0xa0a0);
	}
	return 0;
}

static void codec_aif_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *codec_dai)
{

}

static int codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int i = 0;
	int AIF_CLK_CTRL = 0;
	int aif1_word_size = 16;
	int aif1_lrlk_div = 64;
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (codec_dai->id) {
		case 1:
			AIF_CLK_CTRL = SUNXI_AIF1_CLK_CTRL;
			aif1_lrlk_div = 64;
			break;
		case 2:
			AIF_CLK_CTRL = SUNXI_AIF2_CLK_CTRL;
			aif1_lrlk_div = 64;
			break;
		default:
			return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(codec_aif1_lrck); i++) {
		if (codec_aif1_lrck[i].aif1_lrlk_div == aif1_lrlk_div) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x7<<AIF1_LRCK_DIV), ((codec_aif1_lrck[i].aif1_lrlk_bit)<<AIF1_LRCK_DIV));
			break;
		}
	}
	for (i = 0; i < ARRAY_SIZE(codec_aif1_fs); i++) {
		if (codec_aif1_fs[i].samplerate ==  params_rate(params)) {
			snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0xf<<AIF1_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF1_FS));
			snd_soc_update_bits(codec, SUNXI_SYS_SR_CTRL, (0xf<<AIF2_FS), ((codec_aif1_fs[i].aif1_srbit)<<AIF2_FS));
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0xf<<AIF1_BCLK_DIV), ((codec_aif1_fs[i].aif1_bclk_div)<<AIF1_BCLK_DIV));
			break;
		}
	}
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
		case SNDRV_PCM_FORMAT_S32_LE:
			aif1_word_size = 24;
		break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif1_word_size = 16;
		break;
	}
	for (i = 0; i < ARRAY_SIZE(codec_aif1_wsize); i++) {
		if (codec_aif1_wsize[i].aif1_wsize_val == aif1_word_size) {
			snd_soc_update_bits(codec, AIF_CLK_CTRL, (0x3<<AIF1_WORD_SIZ), ((codec_aif1_wsize[i].aif1_wsize_bit)<<AIF1_WORD_SIZ));
			break;
		}
	}
	return 0;
}

static int codec_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	switch (clk_id) {
		case AIF1_CLK:
			/*system clk from aif1*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_SRC), (0x0<<SYSCLK_SRC));
			break;
		case AIF2_CLK:
			/*system clk from aif2*/
			snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x1<<SYSCLK_SRC), (0x1<<SYSCLK_SRC));
			break;
		default:
			return -EINVAL;
	}
	return 0;
}

static int codec_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	int AIF_CLK_CTRL = 0;
	struct snd_soc_codec *codec = codec_dai->codec;

	switch (codec_dai->id) {
	case 1:
		AIF_CLK_CTRL = SUNXI_AIF1_CLK_CTRL;
		break;
	case 2:
		AIF_CLK_CTRL = SUNXI_AIF2_CLK_CTRL;
		break;
	default:
		return -EINVAL;
	}

	/*
	* 	master or slave selection
	*	0 = Master mode
	*	1 = Slave mode
	*/
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val &=~(0x1<<AIF1_MSTR_MOD);
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master, ap is slave*/
			reg_val |= (0x0<<AIF1_MSTR_MOD);
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave,ap is master*/
			reg_val |= (0x1<<AIF1_MSTR_MOD);
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* i2s mode selection */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	reg_val&=~(3<<AIF1_DATA_FMT);
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_I2S:        /* I2S1 mode */
			reg_val |= (0x0<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val |= (0x2<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val |= (0x1<<AIF1_DATA_FMT);
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L reg_val msb after FRM LRC */
			reg_val |= (0x3<<AIF1_DATA_FMT);
			break;
		default:
			pr_err("%s, line:%d\n", __func__, __LINE__);
			return -EINVAL;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, AIF_CLK_CTRL);
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val &= ~(0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF1_LRCK_INV);
			reg_val |= (0x1<<AIF1_BCLK_INV);
			break;
	}
	snd_soc_write(codec, AIF_CLK_CTRL, reg_val);

	return 0;
}

static int codec_set_fll(struct snd_soc_dai *codec_dai, int pll_id, int source,
									unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;

	if (!freq_out)
		return 0;
	if ((freq_in < 128000) || (freq_in > 24576000)) {
		return -EINVAL;
	} else if ((freq_in == 24576000) || (freq_in == 22579200)) {
		switch (pll_id) {
			case AIF1_CLK:
				/*select aif1/aif2 clk source from pll*/
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x3<<AIF1CLK_SRC), (0x3<<AIF1CLK_SRC));
				snd_soc_update_bits(codec, SUNXI_SYSCLK_CTL, (0x3<<AIF2CLK_SRC), (0x3<<AIF2CLK_SRC));
				break;
			default:
				return -EINVAL;
		}
		return 0;
	}

	return 0;
}

static int codec_aif3_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	int reg_val;
	struct snd_soc_codec *codec = codec_dai->codec;
	/* DAI signal inversions */
	reg_val = snd_soc_read(codec, SUNXI_AIF3_CLK_CTRL);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + nor frame */
			reg_val &= ~(0x1<<AIF3_LRCK_INV);
			reg_val &= ~(0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val |= (0x1<<AIF3_LRCK_INV);
			reg_val &= ~(0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val &= ~(0x1<<AIF3_LRCK_INV);
			reg_val |= (0x1<<AIF3_BCLK_INV);
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + inv frm */
			reg_val |= (0x1<<AIF3_LRCK_INV);
			reg_val |= (0x1<<AIF3_BCLK_INV);
			break;
	}
	snd_soc_write(codec, SUNXI_AIF3_CLK_CTRL, reg_val);

	return 0;
}

static int codec_aif3_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *codec_dai)
{
	int aif3_word_size = 0;
	int aif3_size =0;
	struct snd_soc_codec *codec = codec_dai->codec;
	/*0x2c0 config aif3clk from aif2clk*/
	snd_soc_update_bits(codec, SUNXI_AIF3_CLK_CTRL, (0x3<<AIF3_CLOC_SRC), (0x1<<AIF3_CLOC_SRC));
	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S24_LE:
			aif3_word_size = 24;
			aif3_size = 3;
			break;
		case SNDRV_PCM_FORMAT_S16_LE:
		default:
			aif3_word_size = 16;
			aif3_size = 1;
			break;
	}
	snd_soc_update_bits(codec, SUNXI_AIF3_CLK_CTRL, (0x3<<AIF3_WORD_SIZ), aif3_size<<AIF3_WORD_SIZ);
	return 0;
}

static int codec_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	switch (level) {
	case SND_SOC_BIAS_ON:
		pr_debug("%s,line:%d, SND_SOC_BIAS_ON\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_PREPARE:
		pr_debug("%s,line:%d, SND_SOC_BIAS_PREPARE\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_STANDBY:
		/*on*/
		//switch_hw_config(codec);
		pr_debug("%s,line:%d, SND_SOC_BIAS_STANDBY\n", __func__, __LINE__);
		break;
	case SND_SOC_BIAS_OFF:
		/*off*/
		pr_debug("%s,line:%d, SND_SOC_BIAS_OFF\n", __func__, __LINE__);
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static const struct snd_soc_dai_ops codec_aif1_dai_ops = {
	.set_sysclk		= codec_set_dai_sysclk,
	.set_fmt		= codec_set_dai_fmt,
	.hw_params		= codec_hw_params,
	.shutdown		= codec_aif_shutdown,
	.digital_mute	= codec_aif_mute,
	.set_pll		= codec_set_fll,
};

static const struct snd_soc_dai_ops codec_aif2_dai_ops = {
	.set_sysclk	= codec_set_dai_sysclk,
	.set_fmt	= codec_set_dai_fmt,
	.hw_params	= codec_hw_params,
	.shutdown	= codec_aif_shutdown,
	.set_pll	= codec_set_fll,
};

static const struct snd_soc_dai_ops codec_aif3_dai_ops = {
	.hw_params	= codec_aif3_hw_params,
	.set_fmt	= codec_aif3_set_dai_fmt,
};

static struct snd_soc_dai_driver codec_dai[] = {
	{
		.name = "codec-aif1",//aif1
		.id = 1,
		.playback = {
			.stream_name = "AIF1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.capture = {
			.stream_name = "AIF1 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		 },
		.ops = &codec_aif1_dai_ops,
	},
	{
		.name = "codec-aif2",
		.id = 2,
		.playback = {
			.stream_name = "AIF2 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.capture = {
			.stream_name = "AIF2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.ops = &codec_aif2_dai_ops,
	},
	{
		.name = "codec-aif3",
		.id = 3,
		.playback = {
			.stream_name = "AIF3 Playback",
			.channels_min = 1,
			.channels_max = 1,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		},
		.capture = {
			.stream_name = "AIF3 Capture",
			.channels_min = 1,
			.channels_max = 1,
			.rates = codec_RATES,
			.formats = codec_FORMATS,
		 },
		.ops = &codec_aif3_dai_ops,
	}
};
EXPORT_SYMBOL(codec_dai);

static int codec_soc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct codec_priv *sun8iw9_codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	sun8iw9_codec = kzalloc(sizeof(struct codec_priv), GFP_KERNEL);
	if (sun8iw9_codec == NULL) {
		return -ENOMEM;
	}
	sun8iw9_codec->codec = codec;
	snd_soc_codec_set_drvdata(codec, sun8iw9_codec);
	sun8iw9_codec->dac_enable = 0;
	sun8iw9_codec->adc_enable = 0;
	sun8iw9_codec->aif1_clken = 0;
	sun8iw9_codec->aif2_clken = 0;
	sun8iw9_codec->aif3_clken = 0;
	mutex_init(&sun8iw9_codec->dac_mutex);
	mutex_init(&sun8iw9_codec->adc_mutex);
	mutex_init(&sun8iw9_codec->aifclk_mutex);
	/* Add virtual switch */
	snd_soc_add_codec_controls(codec, sunxi_codec_controls,
					ARRAY_SIZE(sunxi_codec_controls));
	if (ret) {
		pr_err("[audiocodec] Failed to register audio mode control, "
				"will continue without it.\n");
	}
	snd_soc_dapm_new_controls(dapm, ac_dapm_widgets, ARRAY_SIZE(ac_dapm_widgets));
 	snd_soc_dapm_add_routes(dapm, ac_dapm_routes, ARRAY_SIZE(ac_dapm_routes));
	return 0;
}

static int codec_suspend(struct snd_soc_codec *codec)
{
	pr_debug("[audio codec]:suspend start\n");
	/* check if called in talking standby */
	if (check_scene_locked(SCENE_TALKING_STANDBY) == 0) {
		pr_err("In talking standby, audio codec do not suspend!!\n");
		return 0;
	}
	pr_debug("[audio codec]:suspend end\n");
	return 0;
}

static int codec_resume(struct snd_soc_codec *codec)
{
	pr_debug("[audio codec]:resume start\n");

	pr_debug("[audio codec]:resume end\n");
	return 0;
}

/* power down chip */
static int codec_soc_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static unsigned int codec_read(struct snd_soc_codec *codec,
					  unsigned int reg)
{
	if (reg<=0x1d) {
		/*analog reg*/
		return read_prcm_wvalue(reg);
	} else {
		/*digital reg*/
		return codec_rdreg(reg);
	}
}

static int codec_write(struct snd_soc_codec *codec,
				  unsigned int reg, unsigned int value)
{
	if (reg<=0x1d) {
		/*analog reg*/
		write_prcm_wvalue(reg, value);
	} else {
		/*digital reg*/
		codec_wrreg(reg, value);
	}
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_codec = {
	.probe 	 =	codec_soc_probe,
	.remove  =  codec_soc_remove,
	.suspend = 	codec_suspend,
	.resume  =  codec_resume,
	.set_bias_level = codec_set_bias_level,
	.read 	 = 	codec_read,
	.write 	 = 	codec_write,
};

static ssize_t show_audio_reg(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int count = 0;
	int i = 0;
	int reg_group =0;

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
*param 1: 0 read;1 write
*param 2: 1 digital reg; 2 analog reg
*param 3: reg value;
*param 4: write value;
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
	int rw_flag;
	int reg_val_read;
	int input_reg_val =0;
	int input_reg_group =0;
	int input_reg_offset =0;

	ret = sscanf(buf, "%d,%d,0x%x,0x%x", &rw_flag,&input_reg_group, &input_reg_offset, &input_reg_val);
	printk("ret:%d, reg_group:%d, reg_offset:%d, reg_val:0x%x\n", ret, input_reg_group, input_reg_offset, input_reg_val);

	if (!(input_reg_group ==1 || input_reg_group ==2)){
		pr_err("not exist reg group\n");
		ret = count;
		goto out;
	}
	if (!(rw_flag ==1 || rw_flag ==0)){
		pr_err("not rw_flag\n");
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

static int __init codec_codec_probe(struct platform_device *pdev)
{
	int err = -1;
#if 0
	int reg_val = 0;
	int req_status;
	long unsigned int config_set = 0;
	script_item_u val;
#endif
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
#if 0
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
		reg_val = readl((void __iomem *)0xf1c20824);
		reg_val &= 0xffff;
		reg_val |= (0x3333<<16);
		writel(reg_val, (void __iomem *)0xf1c20824);
		//pr_err("%s,line:%d,reg_val:%x\n",__func__,__LINE__,readl((void __iomem *)0xf1c20824));
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
		pr_debug("[audiocodec}: aif3 initialize PG10 PG11 PG12 PG13!!\n");
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
#endif
	snd_soc_register_codec(&pdev->dev, &soc_codec_dev_codec, codec_dai, ARRAY_SIZE(codec_dai));

	err=sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
	if (err){
		pr_err("failed to create attr group\n");
	}
	return 0;
}

static int __exit codec_codec_remove(struct platform_device *pdev)
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

	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
	} else {
		clk_disable_unprepare(codec_moduleclk);
	}
}

/*data relating*/
static struct platform_device codec_codec_device = {
	.name = "sunxi-pcm-codec",
	.id = -1,
};

/*method relating*/
static struct platform_driver codec_codec_driver = {
	.driver = {
		.name = "sunxi-pcm-codec",
		.owner = THIS_MODULE,
	},
	.probe = codec_codec_probe,
	.remove = __exit_p(codec_codec_remove),
	.shutdown = sunxi_codec_shutdown,
};

static int __init codec_codec_init(void)
{
	int err = 0;

	if((err = platform_device_register(&codec_codec_device)) < 0)
		return err;

	if ((err = platform_driver_register(&codec_codec_driver)) < 0)
		return err;

	return 0;
}
module_init(codec_codec_init);

static void __exit codec_codec_exit(void)
{
	platform_driver_unregister(&codec_codec_driver);
}
module_exit(codec_codec_exit);

MODULE_DESCRIPTION("codec ALSA soc codec driver");
MODULE_AUTHOR("huanxin<huanxin@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm-codec");
