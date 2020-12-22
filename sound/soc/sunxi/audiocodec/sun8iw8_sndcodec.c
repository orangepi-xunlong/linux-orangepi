/*
 * sound\soc\sunxi\audiocodec\sun8iw8_sndcodec.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liushaohua <liushaohua@allwinnertech.com>
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
#include <linux/power/scenelock.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/consumer.h>
#include "sunxi_codecdma.h"
#include "sun8iw8_sndcodec.h"

#define sndpcm_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndpcm_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)
static struct regulator* hp_ldo = NULL;
static char *hp_ldo_str = NULL;

/*for pa gpio ctrl*/
static script_item_u item;
static script_item_value_type_e  type;

static  bool codec_mainmic_en           = false;
static  bool codec_mic2_en        = false;
static  bool codec_linein           = false;
static  bool codec_headphoneout_en      = false;
static	bool codec_speakerout_en		=false;
static	bool codec_noise_reduced_en       = false;
static	bool version_v3_used       = false;
static	bool adcagc_used       = false;
static	bool adcdrc_used       = false;
static	bool dacdrc_used       = false;
static	bool adchpf_used       = false;
static	bool dachpf_used       = false;

static  int codec_recordsrc		= 0;
static	int codec_speaker_headset_en = 0;
static	int pa_vol 						= 0;
static	int cap_vol 						= 4;
static	int headphone_vol 				= 0;
static	int pa_double_used 				= 0;
static	int phone_mic_vol 			= 4;
static	int headphone_direct_used 		= 0;
static bool pa_gpio_config = false;

static struct clk *codec_pll2clk,*codec_moduleclk;

static struct sunxi_dma_params sunxi_pcm_pcm_stereo_out = {
	.name		= "audio_play",
	.dma_addr	= CODEC_BASSADDRESS + SUNXI_DAC_TXDATA,//send data address
};

static struct sunxi_dma_params sunxi_pcm_pcm_stereo_in = {
	.name   	= "audio_capture",
	.dma_addr	= CODEC_BASSADDRESS + SUNXI_ADC_RXDATA,//accept data address
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

static int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}

static void adcagc_config(void)
{

}
static void adcdrc_config(void)
{
	codec_wr_control( SUNXI_ADC_DRC_CTRL    , 0xffff, 0, 0x00000003);
	codec_wr_control( SUNXI_ADC_DRC_LPFHAT  , 0xffff, 0, 0x0000000B);
	codec_wr_control( SUNXI_ADC_DRC_LPFLAT  , 0xffff, 0, 0x000077EF);
	codec_wr_control( SUNXI_ADC_DRC_RPFHAT  , 0xffff, 0, 0x0000000B);
	codec_wr_control( SUNXI_ADC_DRC_RPFLAT  , 0xffff, 0, 0x000077EF);
	codec_wr_control( SUNXI_ADC_DRC_LPFHRT  , 0xffff, 0, 0x000000FF);
	codec_wr_control( SUNXI_ADC_DRC_LPFLRT  , 0xffff, 0, 0x0000E1F8);
	codec_wr_control( SUNXI_ADC_DRC_RPFHRT  , 0xffff, 0, 0x000000FF);
	codec_wr_control( SUNXI_ADC_DRC_RPFLRT  , 0xffff, 0, 0x0000E1F8);
	codec_wr_control( SUNXI_ADC_DRC_LRMSHAT , 0xffff, 0, 0x00000001);
	codec_wr_control( SUNXI_ADC_DRC_LRMSLAT , 0xffff, 0, 0x00002BAF);
	codec_wr_control( SUNXI_ADC_DRC_RRMSHAT , 0xffff, 0, 0x00000001);
	codec_wr_control( SUNXI_ADC_DRC_RRMSLAT , 0xffff, 0, 0x00002BAF);
	codec_wr_control( SUNXI_ADC_DRC_HCT     , 0xffff, 0, 0x000005D0);
	codec_wr_control( SUNXI_ADC_DRC_LCT     , 0xffff, 0, 0x00003948);
	codec_wr_control( SUNXI_ADC_DRC_HKC     , 0xffff, 0, 0x00000100);
	codec_wr_control( SUNXI_ADC_DRC_LKC     , 0xffff, 0, 0x00000000);
	codec_wr_control( SUNXI_ADC_DRC_HOPC    , 0xffff, 0, 0x0000FA2F);
	codec_wr_control( SUNXI_ADC_DRC_LOPC    , 0xffff, 0, 0x0000C6B8);
	codec_wr_control( SUNXI_ADC_DRC_HLT     , 0xffff, 0, 0x000001A9);
	codec_wr_control( SUNXI_ADC_DRC_LLT     , 0xffff, 0, 0x000034F0);
	codec_wr_control( SUNXI_ADC_DRC_HKI     , 0xffff, 0, 0x00000100);
	codec_wr_control( SUNXI_ADC_DRC_LKI     , 0xffff, 0, 0x00000000);
	codec_wr_control( SUNXI_ADC_DRC_HOPL    , 0xffff, 0, 0x0000FE56);
	codec_wr_control( SUNXI_ADC_DRC_LOPL    , 0xffff, 0, 0x0000CB10);
	codec_wr_control( SUNXI_ADC_DRC_HET     , 0xffff, 0, 0x000006A4);
	codec_wr_control( SUNXI_ADC_DRC_LET     , 0xffff, 0, 0x0000D3C0);
	codec_wr_control( SUNXI_ADC_DRC_HKE     , 0xffff, 0, 0x00000200);
	codec_wr_control( SUNXI_ADC_DRC_LKE     , 0xffff, 0, 0x00000000);
	codec_wr_control( SUNXI_ADC_DRC_HOPE    , 0xffff, 0, 0x0000F8B1);
	codec_wr_control( SUNXI_ADC_DRC_LOPE    , 0xffff, 0, 0x00001713);
	codec_wr_control( SUNXI_ADC_DRC_HKN     , 0xffff, 0, 0x000001CC);
	codec_wr_control( SUNXI_ADC_DRC_LKN     , 0xffff, 0, 0x0000CCCC);
	codec_wr_control( SUNXI_ADC_DRC_SFHAT   , 0xffff, 0, 0x00000002);
	codec_wr_control( SUNXI_ADC_DRC_SFLAT   , 0xffff, 0, 0x00005600);
	codec_wr_control( SUNXI_ADC_DRC_SFHRT   , 0xffff, 0, 0x00000000);
	codec_wr_control( SUNXI_ADC_DRC_SFLRT   , 0xffff, 0, 0x00000F04);
	codec_wr_control( SUNXI_ADC_DRC_MXGHS   , 0xffff, 0, 0x0000FE56);
	codec_wr_control( SUNXI_ADC_DRC_MXGLS   , 0xffff, 0, 0x0000CB0F);
	codec_wr_control( SUNXI_ADC_DRC_MNGHS   , 0xffff, 0, 0x0000F95B);
	codec_wr_control( SUNXI_ADC_DRC_MNGLS   , 0xffff, 0, 0x00002C3F);
	codec_wr_control( SUNXI_ADC_DRC_EPSHC   , 0xffff, 0, 0x00000000);
	codec_wr_control( SUNXI_ADC_DRC_EPSLC   , 0xffff, 0, 0x0000640C);
	codec_wr_control( SUNXI_ADC_DRC_OPT     , 0xffff, 0, 0x00000400);
}
static void dacdrc_config(void)
{
	codec_wr_control(SUNXI_DAC_DRC_CTRL     , 0xffff, 0,    0x00000003);
	codec_wr_control(SUNXI_DAC_DRC_LPFHAT   , 0xffff, 0,    0x0000000B);
	codec_wr_control(SUNXI_DAC_DRC_LPFLAT   , 0xffff, 0,    0x000077EF);
	codec_wr_control(SUNXI_DAC_DRC_RPFHAT   , 0xffff, 0,    0x0000000B);
	codec_wr_control(SUNXI_DAC_DRC_RPFLAT   , 0xffff, 0,    0x000077EF);
	codec_wr_control(SUNXI_DAC_DRC_LPFHRT   , 0xffff, 0,    0x000000FF);
	codec_wr_control(SUNXI_DAC_DRC_LPFLRT   , 0xffff, 0,    0x0000E1F8);
	codec_wr_control(SUNXI_DAC_DRC_RPFHRT   , 0xffff, 0,    0x000000FF);
	codec_wr_control(SUNXI_DAC_DRC_RPFLRT   , 0xffff, 0,    0x0000E1F8);
	codec_wr_control(SUNXI_DAC_DRC_LRMSHAT  , 0xffff, 0,    0x00000001);
	codec_wr_control(SUNXI_DAC_DRC_LRMSLAT  , 0xffff, 0,    0x00002BAF);
	codec_wr_control(SUNXI_DAC_DRC_RRMSHAT  , 0xffff, 0,    0x00000001);
	codec_wr_control(SUNXI_DAC_DRC_RRMSLAT  , 0xffff, 0,    0x00002BAF);
	codec_wr_control(SUNXI_DAC_DRC_HCT      , 0xffff, 0,    0x000004FB);
	codec_wr_control(SUNXI_DAC_DRC_LCT      , 0xffff, 0,    0x00009ED0);
	codec_wr_control(SUNXI_DAC_DRC_HKC      , 0xffff, 0,    0x00000100);
	codec_wr_control(SUNXI_DAC_DRC_LKC      , 0xffff, 0,    0x00000000);
	codec_wr_control(SUNXI_DAC_DRC_HOPC     , 0xffff, 0,    0x0000FBD8);
	codec_wr_control(SUNXI_DAC_DRC_LOPC     , 0xffff, 0,    0x0000FBA8);
	codec_wr_control(SUNXI_DAC_DRC_HLT      , 0xffff, 0,    0x00000352);
	codec_wr_control(SUNXI_DAC_DRC_LLT      , 0xffff, 0,    0x000069E0);
	codec_wr_control(SUNXI_DAC_DRC_HKI      , 0xffff, 0,    0x00000080);
	codec_wr_control(SUNXI_DAC_DRC_LKI      , 0xffff, 0,    0x00000000);
	codec_wr_control(SUNXI_DAC_DRC_HOPL     , 0xffff, 0,    0x0000FD82);
	codec_wr_control(SUNXI_DAC_DRC_LOPL     , 0xffff, 0,    0x00003098);
	codec_wr_control(SUNXI_DAC_DRC_HET      , 0xffff, 0,    0x00000779);
	codec_wr_control(SUNXI_DAC_DRC_LET      , 0xffff, 0,    0x00006E38);
	codec_wr_control(SUNXI_DAC_DRC_HKE      , 0xffff, 0,    0x00000100);
	codec_wr_control(SUNXI_DAC_DRC_LKE      , 0xffff, 0,    0x00000000);
	codec_wr_control(SUNXI_DAC_DRC_HOPE     , 0xffff, 0,    0x0000F906);
	codec_wr_control(SUNXI_DAC_DRC_LOPE     , 0xffff, 0,    0x000021A9);
	codec_wr_control(SUNXI_DAC_DRC_HKN      , 0xffff, 0,    0x00000122);
	codec_wr_control(SUNXI_DAC_DRC_LKN      , 0xffff, 0,    0x00002222);
	codec_wr_control(SUNXI_DAC_DRC_SFHAT    , 0xffff, 0,    0x00000002);
	codec_wr_control(SUNXI_DAC_DRC_SFLAT    , 0xffff, 0,    0x00005600);
	codec_wr_control(SUNXI_DAC_DRC_SFHRT    , 0xffff, 0,    0x00000000);
	codec_wr_control(SUNXI_DAC_DRC_SFLRT    , 0xffff, 0,    0x00000F04);
	codec_wr_control(SUNXI_DAC_DRC_MXGHS    , 0xffff, 0,    0x0000FE56);
	codec_wr_control(SUNXI_DAC_DRC_MXGLS    , 0xffff, 0,    0x0000CB0F);
	codec_wr_control(SUNXI_DAC_DRC_MNGHS    , 0xffff, 0,    0x0000F95B);
	codec_wr_control(SUNXI_DAC_DRC_MNGLS    , 0xffff, 0,    0x00002C3F);
	codec_wr_control(SUNXI_DAC_DRC_EPSHC    , 0xffff, 0,    0x00000000);
	codec_wr_control(SUNXI_DAC_DRC_EPSLC    , 0xffff, 0,    0x0000640C);
	codec_wr_control(SUNXI_DAC_DRC_OPT      , 0xffff, 0,	0x00000400);

}

static void adcdrc_enable(bool on)
{
	if (on) {
		codec_wr_control( SUNXI_ADC_DAP_CTR	, 0x3, 25, 	3);
	} else {
		codec_wr_control( SUNXI_ADC_DAP_CTR	, 0x3, 25, 0);
	}

}
static void dacdrc_enable(bool on)
{
	if (on) {
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 15, 	1);
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 31, 	1);

	} else {
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 15, 	0);
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 31, 	0);
	}

}
static void adcagc_enable(bool on)
{
	if (on) {

	} else {

	}
}
static void adchpf_config(void)
{
	codec_wr_control( SUNXI_ADC_DRC_HHPFC	, 0xffff, 0, 0x000000FF );
	codec_wr_control( SUNXI_ADC_DRC_LHPFC	, 0xffff, 0, 0x0000FAC1 );
	codec_wr_control( SUNXI_ADC_HPF_HG	, 0xffff, 0, 0x00000100 );
	codec_wr_control( SUNXI_ADC_HPF_LG	, 0xffff, 0, 0x00000000 );
}
static void dachpf_config(void)
{
	codec_wr_control(SUNXI_DAC_DRC_HHPFC	, 0xffff, 0, 	0x000000FF);
	codec_wr_control(SUNXI_DAC_DRC_LHPFC	, 0xffff, 0, 	0x0000FAC1);
	codec_wr_control(SUNXI_DAC_HPF_HG	, 0xffff, 0, 	0x00000100);
	codec_wr_control(SUNXI_DAC_HPF_LG	, 0xffff, 0, 	0x00000000);

}
static void dachpf_enable(bool on)
{
	if (on) {
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 14, 	1);
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 31, 	1);
	} else {
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 14, 	0);
		codec_wr_control( SUNXI_DAC_DAP_CTR	, 0x1, 31, 	0);
	}

}
static void adchpf_enable(bool on)
{
	if (on) {
		codec_wr_control( SUNXI_ADC_DAP_CTR	, 0x1, 24, 	1);
		codec_wr_control( SUNXI_ADC_DAP_CTR	, 0x1, 26, 	1);
	} else {
		codec_wr_control( SUNXI_ADC_DAP_CTR	, 0x1, 24, 	1);
		codec_wr_control( SUNXI_ADC_DAP_CTR	, 0x1, 26, 	1);
	}

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

	type = script_get_item("audio0", "cap_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[audiocodec] cap_vol type err!\n");
	}  else {
		cap_vol = val.val;
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
	type = script_get_item("audio0", "phone_mic_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] phone_mic_vol type err!\n");
    	} else {
		phone_mic_vol = val.val;
	}

	type = script_get_item("audio0", "headphone_direct_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] headphone_direct_used type err!\n");
    	} else {
		headphone_direct_used = val.val;
	}
	type = script_get_item("audio0", "version_v3_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] version_v3_used type err!\n");
    	} else {
		version_v3_used = val.val;
	}

	type = script_get_item("audio0", "adcagc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] adcagc_used type err!\n");
    	} else {
		adcagc_used = val.val;
	}

	type = script_get_item("audio0", "adcdrc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] adcdrc_used type err!\n");
    	} else {
		adcdrc_used = val.val;
	}

	type = script_get_item("audio0", "dacdrc_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] dacdrc_used type err!\n");
    	} else {
		dacdrc_used = val.val;
	}

	type = script_get_item("audio0", "adchpf_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] adchpf_used type err!\n");
    	} else {
		adchpf_used = val.val;
	}

	type = script_get_item("audio0", "dachpf_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        	pr_err("[audiocodec] dachpf_used type err!\n");
    	} else {
		dachpf_used = val.val;
	}

}

/*
* enable the codec function which should be enable during system init.
*/
static void codec_init(void)
{
	get_audio_param();
	if (headphone_direct_used) {
		codec_wr_prcm_control(HP_CTRL, 0x3, HPCOM_FC, 0x3);
		codec_wr_prcm_control(HP_CTRL, 0x1, COMPTEN, 0x1);
	} else {
		codec_wr_prcm_control(HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, COMPTEN, 0x0);
	}
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	/*when TX FIFO available room less than or equal N,
	* DRQ Requeest will be de-asserted.
	*/
	codec_wr_control(SUNXI_DAC_FIFOC, 0x3, DRA_LEVEL, 0x3);
	/*write 1 to flush tx fifo*/
	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, DAC_FIFO_FLUSH, 0x1);
	/*write 1 to flush rx fifo*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, FIR_VERSION, 0x0);
	if (adcagc_used) {
		adcagc_config();
	}
	if (adcdrc_used) {
		adcdrc_config();
	}
	if (adchpf_used) {
		adchpf_config();
	}
	if (dacdrc_used) {
		dacdrc_config();
	}
	if (dachpf_used) {
		dachpf_config();
	}
}

/*
*	use for system record from mic1.
*
*/
static int codec_capture_open(void)
{
	/*disable mic2 pa*/
	codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);
	/*disable Right LINEIN-R*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x0);
	/*disable Left LINEIN-L*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x0);
	/*disable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
	/*disable Left MIC2 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
	/*enable mic1 pa*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x7, MIC1BOOST, cap_vol);
	/*enable Master microphone bias*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	/*enable Right MIC1 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x1);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*enable adc drq*/
	//codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for system record from mic2.
*/
static int codec_mic2_capture_open(void)
{
	/*disable mic1 pa*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	/*disable Master microphone bias*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);

	/*disable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
	/*disable Right MIC1 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);

	/*disable Right LINEIN-R*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x0);
	/*disable Left LINEIN-L*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x0);

	codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x1);
	/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
	codec_wr_prcm_control(MIC2_CTRL,0x7,MIC2BOOST, cap_vol);
	if(codec_recordsrc == 1) {/*mic2*/
		/*open mic2 routeway*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
		codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 1);
	} else if (codec_recordsrc == 2) {/*headset mic*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
		codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 1);
	}else if(codec_recordsrc == 3) {
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
		codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 0);
	}
	/*enable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC2 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x1);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for system record from linein.
*/
static int codec_linein_capture_open(void)
{
	/*disable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
	/*disable Right MIC1 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
	/*disable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
	/*disable Left MIC2 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
	/*enable Right LINEIN-R*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x1);
	/*enable Left LINEIN-L*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x1);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	/*hardware fifo delay*/
	msleep(200);
	return 0;
}

/*
*	use for the phone noise reduced .
*	mic1,mic3
*/
static int codec_noise_reduced_capture_open(void)
{
	/*enable mic1 pa*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
	/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x7, MIC1BOOST, cap_vol);
	/*enable Master microphone bias*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);

	codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x1);
	/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
	codec_wr_prcm_control(MIC2_CTRL,0x7,MIC2BOOST, cap_vol);
	codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 0);
	/*enable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
	/*enable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
	/*enable adc_r adc_l analog*/
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x1);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x1);
	/*set RX FIFO mode*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	/*set RX FIFO rec drq level*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1f, RX_TRI_LEVEL, 0xf);
	/*enable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x1);
	msleep(200);
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
		if (version_v3_used) {
			if (!pa_double_used) {
				codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x1);
				codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x1);
				codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x1);
				codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x1);

			} else {
				codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
				codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x0);
				codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x1);
				codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x1);
			}
			/*set LINEOUT volume*/
			codec_wr_prcm_control(LINEOUT_VOLC, 0x1f, LINEOUTVOL, pa_vol);

		} else {
			if (!pa_double_used) {
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
				codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x1);
				codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
			} else {
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);
				codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
				codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
				codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
			}
			codec_wr_prcm_control(ZERO_CROSS_CTRL, 0x1, ZERO_CROSS_EN, 0x0);
			/*set HPVOL volume*/
			codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, pa_vol);
		}
		msleep(10);
		if (pa_gpio_config)
			gpio_set_value(item.gpio.gpio, 1);

	} else {
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
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
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x1);
		codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
		codec_wr_prcm_control(ZERO_CROSS_CTRL, 0x1, ZERO_CROSS_EN, 0x0);
		/*set HPVOL volume*/
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, headphone_vol);
	} else {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
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
*	codec_mainmic_en == 1, open mainmic.
*	codec_mainmic_en == 0, close mainmic.
*/
static int codec_set_mainmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_mainmic_en = ucontrol->value.integer.value[0];
	if (codec_mainmic_en) {
		/*close headset mic(mic2) routeway*/
		codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEMIC2BOOST, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEMIC2BOOST, 0x0);
		/*open main mic(mic1) routeway*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
		/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
		codec_wr_prcm_control(BIAS_MIC_CTRL,0x7, MIC1BOOST, phone_mic_vol);
		/*enable Master microphone bias*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
		/*enable Right MIC1 Boost stage*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEMIC1BOOST, 0x1);
		/*enable Left MIC1 Boost stage*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEMIC1BOOST, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);
		codec_mic2_en = 0;
	} else {
		/*open main mic(mic1) routeway*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		/*enable Master microphone bias*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
		/*enable Right MIC1 Boost stage*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEMIC1BOOST, 0x0);
		/*enable Left MIC1 Boost stage*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEMIC1BOOST, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
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
*	codec_linein == 1, open linin.
*	codec_linein == 0, close linein.
*/
static int codec_set_linein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_linein = ucontrol->value.integer.value[0];
	if (codec_linein) {
		/*enable Right linein stage*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTELINEINR, 0x1);
		/*enable Left linein stage*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTELINEINL, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);
	} else {
		/*disable Right linein Boost stage*/
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTELINEINR, 0x0);
		/*disable Left linein Boost stage*/
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTELINEINL, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
	}

	return 0;
}
static int codec_get_linein(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_linein;
	return 0;
}

/*
*	codec_noise_reduced_en == 1, set status.
*	codec_noise_reduced_en == 0, set status.
*/
static int codec_set_noise_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_noise_reduced_en = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_noise_reduced(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_noise_reduced_en;
	return 0;
}

/*
*	codec_mic2_en == 1, open mic2.
*	codec_mic2_en == 0, close mic2.
*/
static int codec_set_mic2(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_mic2_en = ucontrol->value.integer.value[0];
	if (codec_mic2_en) {

		/*close main mic(mic1) routeway*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
		if(codec_recordsrc == 1) {/*mic2*/
			/*open headset mic(mic2) routeway*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
			codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 1);
		} else if (codec_recordsrc == 2) {/*headset mic*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
			codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 1);
		} else if(codec_recordsrc == 3) {/*mic3*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
			codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 0);
		}
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEMIC1BOOST, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEMIC1BOOST, 0x0);
		/*enable mic2 pa*/
		codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x1);
		/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
		codec_wr_prcm_control(MIC2_CTRL,0x7,MIC2BOOST, phone_mic_vol);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEMIC2BOOST, 0x1);
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEMIC2BOOST, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);
		/*set the main mic flag false*/
		codec_mainmic_en	= 0;
	} else {
		/*close headset mic(mic2) routeway*/
		codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
		/*disable mic2 pa*/
		codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);
		codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEMIC2BOOST, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x1, LMIXMUTEMIC2BOOST, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
	}

	return 0;
}

static int codec_get_mic2(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_mic2_en;
	return 0;
}
/*
*	close all routeway
*/
static int codec_set_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	if (pa_gpio_config)
		gpio_set_value(item.gpio.gpio, 0);
	codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);
	/*disable analog part*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);

	codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
	codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x0);

	/*disable output mixer */
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

	codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
	codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
	/*disable dac drq*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);
	/*disable mic1 pa*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	/*disable Master microphone bias*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
	codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);

	codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x0);
	codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x0);
	/*disable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
	/*disable Right MIC1 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);

	/*disable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
	/*disable Left MIC2 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);

	/*disable Right LINEIN-R*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x0);
	/*disable Left LINEIN-L*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x0);
	/*disable adc_r adc_l analog*/
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x0);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x0);
	/*disable adc digital part*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x0);
	codec_mainmic_en           = 0;
	codec_mic2_en        = 0;
	codec_linein = 0;
	codec_headphoneout_en      = 0;
	codec_noise_reduced_en       = 0;
	codec_recordsrc		= 0;
	codec_speakerout_en = 0;
	return 0;
}

static int codec_get_endcall(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}
/*
*	change capture routeway according to the catpute src
*/
static int codec_set_capture_route_switch(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	/*disable mic1 pa*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	/*disable Master microphone bias*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
	codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);
	/*disable Left MIC1 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
	/*disable Right MIC1 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);
	/*disable Right MIC2 Boost stage*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
	/*disable Left MIC2 Boost stage*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);
	/*disable Right LINEIN-R*/
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x0);
	/*disable Left LINEIN-L*/
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x0);

	switch(codec_recordsrc)
	{
		case 0:/*mic1*/
			/*enable mic1 pa*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);
			/*mic1 gain 36dB,if capture volume is too small, enlarge the mic1boost*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x7, MIC1BOOST, cap_vol);
			/*enable Master microphone bias*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
			/*enable Left MIC1 Boost stage*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x1);
			/*enable Right MIC1 Boost stage*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x1);
			break;
		case 1: /*mic2(normal)*/
			codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x1);
			/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
			codec_wr_prcm_control(MIC2_CTRL,0x7,MIC2BOOST, cap_vol);
			/*open mic2 routeway*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
			codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 1);
			/*enable Right MIC2 Boost stage*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
			/*enable Left MIC2 Boost stage*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
			break;
		case 2:	/*headphonemic*/
			codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x1);
			/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
			codec_wr_prcm_control(MIC2_CTRL,0x7,MIC2BOOST, cap_vol);

			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
			codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 1);

			/*enable Right MIC2 Boost stage*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
			/*enable Left MIC2 Boost stage*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
			break;
		case 3: /*mic3*/
			codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x1);
			/*mic2 gain 36dB,if capture volume is too small, enlarge the mic2boost*/
			codec_wr_prcm_control(MIC2_CTRL,0x7,MIC2BOOST, cap_vol);
			codec_wr_prcm_control(BIAS_MIC_CTRL,0x1,MIC2_SS, 0);
			/*enable Master microphone bias*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x1);
			/*enable Right MIC2 Boost stage*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x1);
			/*enable Left MIC2 Boost stage*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x1);
			break;
		case 4: /*linein*/
			/*enable Right LINEIN-R*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x1);
			/*enable Left LINEIN-L*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x1);
			break;
		default:
			pr_err("%s: the wrong arg:%d\n", __func__, codec_recordsrc);
	}
	return 0;
}

static int codec_get_capture_route_switch(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	return 0;
}
static int codec_headphone_play_open(void)
{
	int reg_val = 0;
	int i = 0;
	if (pa_gpio_config)
		gpio_set_value(item.gpio.gpio, 0);
	if (version_v3_used) {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x0);
	} else {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	}
	codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);
	codec_wr_control(SUNXI_DAC_DPC, 0x3f, DIGITAL_VOL, 0x0);
	/*enable dac digital part*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	//codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);
	/*send last sample when dac fifo under run*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
	codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x0);
	codec_wr_prcm_control(LOMIXSC, 0x1, RMIXMUTEDACR, 0x0);
	codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
	codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
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
	return 0;

}


static int codec_pa_play_open(void)
{
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, 0);

	/*enable dac digital part*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	/*send last sample when dac fifo under run*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);
#ifdef CONFIG_ARCH_SUN8IW8
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, 0, 0x0);
#else
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
#endif

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);
	if (version_v3_used) {
		if (!pa_double_used) {
			codec_wr_control(SUNXI_DAC_DPC, 0x3f, DIGITAL_VOL, 0x6);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
			codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
			codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x3);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x1);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x1);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x1);
		} else {
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);
			codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x1);
			codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x1);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x0);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x1);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x1);
		}
		/*set LINEOUTVOL volume*/
		codec_wr_prcm_control(LINEOUT_VOLC, 0x1f, LINEOUTVOL, pa_vol);
		usleep_range(2000, 3000);

	} else {
		if (!pa_double_used) {
			codec_wr_control(SUNXI_DAC_DPC, 0x3f, DIGITAL_VOL, 0x6);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
			codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
			codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x3);
			codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x1);
			codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
		} else {
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
			codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x0);
			codec_wr_prcm_control(LOMIXSC, 0x1, RMIXMUTEDACR, 0x0);
			codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
			codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x1);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x1);
		}

		/*set HPVOL volume*/
		codec_wr_prcm_control(HP_VOLC, 0x3f, HPVOL, pa_vol);
		usleep_range(2000, 3000);
	}
	if (pa_gpio_config)
		gpio_set_value(item.gpio.gpio, 1);
	msleep(62);
	return 0;

}
/*
* codec_pa_headphone_play_open support v3
*/
static int codec_pa_headphone_play_open(void)
{
	int reg_val = 0;
	int i = 0;
	/*enable dac digital part*/
	codec_wr_control(SUNXI_DAC_DPC, 0x1, DAC_EN, 0x1);
	/*set TX FIFO send drq level*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x7f, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	/*send last sample when dac fifo under run*/
	codec_wr_control(SUNXI_DAC_FIFOC ,0x1, LAST_SE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x1);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x1);
	if (!pa_double_used) {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
		codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x3);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x1);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x1);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x1);
	} else {
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x1);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x1);
		codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x1);
		codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x1);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LEFTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, RIGHTLINEOUTSRC, 0x0);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x1);
		codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x1);
	}
	/*set LINEOUTVOL volume*/
	codec_wr_prcm_control(LINEOUT_VOLC, 0x1f, LINEOUTVOL, pa_vol);

	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPIS, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPIS, 0x0);
	codec_wr_prcm_control(ROMIXSC, 0x1, RMIXMUTEDACL, 0x0);
	codec_wr_prcm_control(LOMIXSC, 0x1, RMIXMUTEDACR, 0x0);
	codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
	codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);
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
	if (pa_gpio_config)
		gpio_set_value(item.gpio.gpio, 1);
	msleep(62);
	return 0;

}

static int sndpcm_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int sndpcm_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (dacdrc_used) {
			dacdrc_enable(1);
		}
		if (dachpf_used) {
			dachpf_enable(1);
		}
	} else {
		if (adcdrc_used) {
			adcdrc_enable(1);
		}
		if (adcagc_used) {
			adcagc_enable(1);
		}
		if (adchpf_used) {
			adchpf_enable(1);
		}
	}
	return 0;
}

static void sndpcm_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	int i = 0, cur_vol = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {

		codec_wr_prcm_control(ZERO_CROSS_CTRL, 0x1, ZERO_CROSS_EN, 0x0);
		cur_vol = read_prcm_wvalue(HP_VOLC);
		cur_vol &= 0x3f;
		if (!(codec_headphoneout_en || codec_speakerout_en)){
			if (pa_gpio_config)
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

			codec_wr_prcm_control(ROMIXSC, 0x7f, RMIXMUTE, 0x0);
			codec_wr_prcm_control(LOMIXSC, 0x7f, LMIXMUTE, 0x0);

			/*disable output mixer */
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
			codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);

			codec_wr_prcm_control(HP_CTRL, 0x1, LTRNMUTE, 0x0);
			codec_wr_prcm_control(HP_CTRL, 0x1, RTLNMUTE, 0x0);

			codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x0);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x0);
		}
		/*disable dac drq*/
		codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
		codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);
		/*disable dac digital*/
		codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);
		if (dacdrc_used) {
			dacdrc_enable(0);
		}
		if (dachpf_used) {
			dachpf_enable(0);
		}

	}else{
		/*disable adc drq*/
		codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
		if (!(codec_headphoneout_en || codec_speakerout_en)){
			/*disable mic1 pa*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
			/*disable Master microphone bias*/
			codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
			codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);

			/*disable Left MIC1 Boost stage*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC1BOOST, 0x0);
			/*disable Right MIC1 Boost stage*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC1BOOST, 0x0);

			/*disable Right MIC2 Boost stage*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTEMIC2BOOST, 0x0);
			/*disable Left MIC2 Boost stage*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTEMIC2BOOST, 0x0);

			/*disable Right LINEIN-R*/
			codec_wr_prcm_control(RADC_MIX_MUTE, 0x1, RADCMIXMUTELINEINR, 0x0);
			/*disable Left LINEIN-L*/
			codec_wr_prcm_control(LADC_MIX_MUTE, 0x1, LADCMIXMUTELINEINL, 0x0);
		}
		/*disable adc_r adc_l analog*/
		codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x0);
		codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x0);
		/*disable adc digital part*/
		codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x0);
		if (adcdrc_used) {
			adcdrc_enable(0);
		}
		if (adcagc_used) {
			adcagc_enable(0);
		}
		if (adchpf_used) {
			adchpf_enable(0);
		}
	}

}

static int sndpcm_prepare(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	unsigned int reg_val;
	unsigned int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;

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
		/*set TX FIFO MODE*/
		//codec_wr_control(SUNXI_DAC_FIFOC ,0x3, TX_FIFO_MODE, 0x3);
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
		if (runtime->status->state == SNDRV_PCM_STATE_XRUN) {
			pr_warn("[audio codec]play xrun.\n");
			if (codec_speaker_headset_en == 1) {
				ret = codec_pa_play_open();
			} else if (codec_speaker_headset_en == 0) {
				ret = codec_headphone_play_open();
			}else if(codec_speaker_headset_en == 2) {
				ret = codec_pa_headphone_play_open();
			}
		}
		return ret;
	} else {
		if (runtime->status->state == SNDRV_PCM_STATE_XRUN) {
			pr_warn("[audio codec]capture xrun.\n");
		}
	   	/*open the adc channel register*/
	   	if ((codec_recordsrc == 1) || (codec_recordsrc == 2) || (codec_recordsrc == 3)){
			ret = codec_mic2_capture_open();
		} else if (codec_recordsrc == 4) {
			ret = codec_linein_capture_open();
		} else if (codec_noise_reduced_en) {
			ret = codec_noise_reduced_capture_open();
		} else {
	   		ret = codec_capture_open();
		}
		return ret;
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
				/*DAC FIFO Flush,Write '1' to flush TX FIFO, self clear to '0'*/
				codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
				/*enable dac drq*/
				codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x1);
				return 0;
			case SNDRV_PCM_TRIGGER_SUSPEND:
			case SNDRV_PCM_TRIGGER_STOP:
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
				codec_wr_control(SUNXI_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);
				return 0;
			default:
				return -EINVAL;
			}
	} else {
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_FIFO_FLUSH, 0x1);
			codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
			return 0;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			codec_wr_control(SUNXI_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
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
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data = &sunxi_pcm_pcm_stereo_out;
		if (SNDRV_PCM_FORMAT_S16_LE == params_format(params)) {
			/*set TX FIFO MODE:16bit*/
			codec_wr_control(SUNXI_DAC_FIFOC ,0x3, TX_FIFO_MODE, 0x3);
			codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TASR, 0x0);
		} else {
			/*set TX FIFO MODE:24bit*/
			codec_wr_control(SUNXI_DAC_FIFOC ,0x3, TX_FIFO_MODE, 0x2);
			codec_wr_control(SUNXI_DAC_FIFOC ,0x1, TASR, 0x1);
		}
	} else
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
static const char *record_function[] = {"mic1", "mic2","headsetmic","mic3","linein"};
static const struct soc_enum record_source[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(record_function), record_function),
};

/*
*	use for select record source.
*	4:linein
*	3:mic3
*	2:headsetmic
*	1:mic2
*	0:mic1
*/
static int codec_set_recordsrc(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_recordsrc = ucontrol->value.integer.value[0];
	return 0;
}

static int codec_get_recordsrc(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_recordsrc;
	return 0;
}

static const char *spk_headset_function[] = {"headset", "spk","headset-spk"};
static const struct soc_enum spk_headset_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spk_headset_function), spk_headset_function),
};

/*
*	codec_speaker_headset_en == 0, headphone is open.
*	codec_speaker_headset_en == 1, speaker is open.
*	this function just used for the system voice
*/
static int codec_set_spk_headset(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_speaker_headset_en = ucontrol->value.integer.value[0];

	if (codec_speaker_headset_en == 1) {
		ret = codec_pa_play_open();
	} else if (codec_speaker_headset_en == 0) {
		ret = codec_headphone_play_open();
	}else if(codec_speaker_headset_en == 2) {
		ret = codec_pa_headphone_play_open();
	}
	return 0;
}

static int codec_get_spk_headset(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_speaker_headset_en;
	return 0;
}


static const struct snd_kcontrol_new sunxi_codec_controls[] = {

	CODEC_SINGLE("Master Playback Volume", HP_VOLC, 0, 0x3f, 0),
	CODEC_SINGLE("Line Volume", LINEOUT_VOLC, LINEOUTVOL, 0x1f, 0),
	CODEC_SINGLE("MIC1_G boost stage output mixer control", 	MIC_GCTR, MIC1G, 0x7, 0),
	CODEC_SINGLE("MIC2_G boost stage output mixer control", 	MIC_GCTR, MIC2G, 0x7, 0),
	CODEC_SINGLE("MIC1 boost AMP gain control", 				BIAS_MIC_CTRL, MIC1BOOST, 0x7, 0),
	CODEC_SINGLE("MIC2 boost AMP gain control", 				MIC2_CTRL, MIC2BOOST, 0x7, 0),
	CODEC_SINGLE("ADC input gain ctrl", 						AC_ADC_CTRL, ADCG, 0x7, 0),
	SOC_SINGLE_BOOL_EXT("Audio headphone out", 	0, codec_get_headphoneout, 	codec_set_headphoneout),
	SOC_SINGLE_BOOL_EXT("Audio speaker out", 	0, codec_get_speakerout, 	codec_set_speakerout),
	SOC_SINGLE_BOOL_EXT("Audio main mic", 		0, codec_get_mainmic, 		codec_set_mainmic),
	SOC_SINGLE_BOOL_EXT("Audio sub mic", 	0, codec_get_mic2, 	codec_set_mic2),
	SOC_ENUM_EXT("Audio record source", record_source[0], codec_get_recordsrc, codec_set_recordsrc),
	SOC_SINGLE_BOOL_EXT("Audio noise reduced", 	0, codec_get_noise_reduced, codec_set_noise_reduced),
	SOC_SINGLE_BOOL_EXT("Audio linein", 	0, codec_get_linein, codec_set_linein),
	SOC_SINGLE_BOOL_EXT("Audio capture route switch", 	0, codec_get_capture_route_switch, codec_set_capture_route_switch),
	SOC_SINGLE_BOOL_EXT("Audio clear path", 	0, codec_get_endcall, codec_set_endcall),
	SOC_ENUM_EXT("Speaker Function", spk_headset_enum[0], codec_get_spk_headset, codec_set_spk_headset),
};

static struct snd_soc_dai_ops sndpcm_dai_ops = {
	.startup = sndpcm_startup,
	.shutdown = sndpcm_shutdown,
	.prepare  =	sndpcm_prepare,
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
	char pin_name[SUNXI_PIN_NAME_MAX_LEN];
	unsigned long      config;
	if (pa_gpio_config) {
		gpio_set_value(item.gpio.gpio, 0);
	}
	/*mute l_pa and r_pa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTRIGHTEN, 0x0);
	codec_wr_prcm_control(MIC2_CTRL, 0x1, LINEOUTLEFTEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LMIXEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RMIXEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);

	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);
	codec_wr_prcm_control(ZERO_CROSS_CTRL, 0x1, ZERO_CROSS_EN, 0x0);
	/*disable mic pa*/
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	codec_wr_prcm_control(MIC2_CTRL, 0x1, MIC2AMPEN, 0x0);
	codec_wr_prcm_control(RADC_MIX_MUTE, 0x7f, RADCMIXMUTE, 0x0);
	codec_wr_prcm_control(LADC_MIX_MUTE, 0x7f, LADCMIXMUTE, 0x0);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCREN, 0x0);
	codec_wr_prcm_control(AC_ADC_CTRL, 0x1,  ADCLEN, 0x0);
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1,ADC_EN, 0x0);

	if ((NULL == codec_moduleclk)||(IS_ERR(codec_moduleclk))) {
		pr_err("codec_moduleclk handle is invaled, just return\n");
	} else {
		clk_disable_unprepare(codec_moduleclk);
	}
	if ((NULL == codec_pll2clk) ||(IS_ERR(codec_pll2clk))) {
		pr_err("codec_pll2clk handle is invalid.\n");
	} else {
		/*release the module clock*/
		clk_disable_unprepare(codec_pll2clk);
	}
	if (pa_gpio_config) {
		sunxi_gpio_to_name(item.gpio.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
	}
	pr_debug("[audio codec]:suspend end\n");
	return 0;
}

static int sndpcm_resume(struct snd_soc_codec *codec)
{
	pr_debug("[audio codec]:resume start\n");
	if (clk_prepare_enable(codec_pll2clk)) {
		pr_err("open codec_moduleclk failed; \n");
	}
	if (clk_prepare_enable(codec_moduleclk)) {
		pr_err("open codec_moduleclk failed; \n");
	}
	if (headphone_direct_used) {
		codec_wr_prcm_control(HP_CTRL, 0x3, HPCOM_FC, 0x3);
		codec_wr_prcm_control(HP_CTRL, 0x1, COMPTEN, 0x1);
	} else {
		codec_wr_prcm_control(HP_CTRL, 0x3, HPCOM_FC, 0x0);
		codec_wr_prcm_control(HP_CTRL, 0x1, COMPTEN, 0x0);
	}

	/*when TX FIFO available room less than or equal N,
	* DRQ Requeest will be de-asserted.
	*/
	codec_wr_control(SUNXI_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
	/*write 1 to flush tx fifo*/
	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, DAC_FIFO_FLUSH, 0x1);
	/*write 1 to flush rx fifo*/
	codec_wr_control(SUNXI_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
	codec_wr_control(SUNXI_DAC_FIFOC, 0x1, FIR_VERSION, 0x0);
	if (adcagc_used) {
		adcagc_config();
	}
	if (adcdrc_used) {
		adcdrc_config();
	}
	if (adchpf_used) {
		adchpf_config();
	}
	if (dacdrc_used) {
		dacdrc_config();
	}
	if (dachpf_used) {
		dachpf_config();
	}
	if (pa_gpio_config) {
		gpio_direction_output(item.gpio.gpio, 1);
		gpio_set_value(item.gpio.gpio, 0);
	}
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

	return 0;
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
	int input_reg_group =0;
	int input_reg_offset =0;
	int input_reg_val =0;
	int reg_val_read;
	int rw_flag;

	printk("%s,line:%d\n",__func__,__LINE__);
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


static int __init sndpcm_codec_probe(struct platform_device *pdev)
{
	int err = -1;
	int req_status;
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL, "pll2");
	if ((!codec_pll2clk)||(IS_ERR(codec_pll2clk))) {
		pr_err("try to get codec_pll2clk failed!\n");
	}
	if (clk_set_rate(codec_pll2clk, 24576000)) {
		pr_err("set codec_pll2clk rate fail\n");
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
		pr_err("script_get_item return audio_pa_ctrl type err\n");
		pa_gpio_config = false;
	} else {
		pa_gpio_config = true;
		/*request gpio*/
		req_status = gpio_request(item.gpio.gpio, NULL);
		if (0 != req_status) {
			pr_err("request gpio failed!\n");
		}
		gpio_direction_output(item.gpio.gpio, 1);

		gpio_set_value(item.gpio.gpio, 0);
	}

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
	if (pa_gpio_config) {
		gpio_set_value(item.gpio.gpio, 0);
	}
	codec_wr_prcm_control(ZERO_CROSS_CTRL, 0x1, ZERO_CROSS_EN, 0x0);
	/*mute l_pa and r_pa*/
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, LHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, RHPPAMUTE, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACALEN, 0x0);
	codec_wr_prcm_control(DAC_PA_SRC, 0x1, DACAREN, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUNXI_DAC_DPC ,  0x1, DAC_EN, 0x0);
	codec_wr_prcm_control(BIAS_MIC_CTRL, 0x1, MMICBIASEN, 0x0);
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
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-pcm-codec");
