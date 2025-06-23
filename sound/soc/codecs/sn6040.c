// SPDX-License-Identifier: GPL-2.0
//
// ALSA SoC SN6040 codec driver
//
// Copyright:	(C) 2016 senarytech, Inc.
// Author:	bo liu, <bo.liu@senarytech.com>
//
// TODO: add support for TDM mode.
//
// Initially based on sound/soc/codecs/cx2072x.c
// Copyright:	(C) 2017 Conexant Systems, Inc.
// Author:	Simon Ho, <Simon.ho@conexant.com>
//

#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/initval.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include "sn6040.h"

#define PLL_OUT_HZ_48	(1024 * 3 * 48000)
#define BITS_PER_SLOT	8

/* codec private data, need to check */
struct sn6040_priv {
	struct regmap *regmap;
	struct clk *mclk;
	unsigned int mclk_rate;
	struct device *dev;
	struct snd_soc_component *codec;
	struct snd_soc_jack_gpio jack_gpio;
	struct mutex lock;
	unsigned int bclk_ratio;
	bool pll_changed;
	bool i2spcm_changed;
	int sample_size;
	int frame_size;
	int sample_rate;
	unsigned int dai_fmt;
	bool en_aec_ref;
};

/*
 * DAC/ADC Volume
 *
 * max : 74 : 0 dB
 *	 ( in 1 dB  step )
 * min : 0 : -74 dB
 */
static const DECLARE_TLV_DB_SCALE(adc_tlv, -7400, 100, 0);
static const DECLARE_TLV_DB_SCALE(dac_tlv, -7400, 100, 0);
static const DECLARE_TLV_DB_SCALE(boost_tlv, 0, 1200, 0);

/*struct sn6040_eq_ctrl {
	u8 ch;
	u8 band;
};

static const DECLARE_TLV_DB_RANGE(hpf_tlv,
	0, 0, TLV_DB_SCALE_ITEM(120, 0, 0),
	1, 63, TLV_DB_SCALE_ITEM(30, 30, 0)
);*/

/* Lookup table for PRE_DIV */
static const struct {
	unsigned int mclk;
	unsigned int div;
} mclk_pre_div[] = {
	{ 6144000, 1 },
	{ 6912000, 3 },
	{ 12288000, 2 },
	{ 16384000, 3 },
	{ 18432000, 4 },
	{ 19200000, 3 },
	{ 26000000, 4 },
	{ 27000000, 5 },
	{ 27648000, 6 },
	{ 28224000, 5 },
	{ 36864000, 6 },
	{ 49152000, 8 },
};

/*
 * sn6040 register cache.
 */
static const struct reg_default sn6040_reg_defaults[] = {
	//TODO, define regs' default value
	{},
};

/*
 * register initialization
 */
static const struct reg_sequence sn6040_reg_init[] = {
	//TODO, define regs' init value
	{ SN6040_DIGITAL_BIOS_TEST3_MSB, 0x03 },
	{ SN6040_DIGITAL_BIOS_TEST2_LSB,  0x6C},
	// {SN6040_ANALOG_TEST4, 0x000},
	// {SN6040_ANALOG_TEST5, 0x000},
	// {SN6040_ANALOG_TEST6, 0xBF8},
	// {SN6040_ANALOG_TEST7, 0x193},
	// {SN6040_ANALOG_TEST8, 0x02F},
	// {SN6040_DIGITAL_BIOS_TEST2_MSB, 0x01},
	// {SN6040_DIGITAL_BIOS_TEST2_LSB, 0xEC},
	// {SN6040_I2SPCM_CONTROL1, 0x8E7F8E7F},
	// {SN6040_I2SPCM_CONTROL2, 0xA8002003},
	// {SN6040_I2SPCM_CONTROL3, 0x88002003},
	// {SN6040_CODEC_TEST0, 0x040},
	// {SN6040_DAC1_CONVERTER_FORMAT, 0x0031},
	// {SN6040_DAC2_CONVERTER_FORMAT, 0x0031},
	// {SN6040_AFG_POWER_STATE, 0x00},
	// {SN6040_DAC1_POWER_STATE, 0x00},
	// {SN6040_DAC2_POWER_STATE, 0x03},
	// {SN6040_DAC1_AMP_GAIN_RIGHT, 0x4A},
	// {SN6040_DAC1_AMP_GAIN_LEFT, 0x4A},
	// {SN6040_DAC2_AMP_GAIN_RIGHT, 0x4A},
	// {SN6040_DAC2_AMP_GAIN_LEFT, 0x4A},
	// {SN6040_PORTG_POWER_STATE, 0x03},
	// {SN6040_PORTG_CONNECTION_SELECT, 0x01},
	// {SN6040_PORTG_PIN_CTRL, 0x00},
	// {SN6040_PORTA_POWER_STATE, 0x00},
	// {SN6040_PORTA_CONNECTION_SELECT, 0x00},
	// {SN6040_PORTA_PIN_CTRL, 0xC0},
};

/* get register's size */
static unsigned int sn6040_register_size(unsigned int reg)
{
	//TODO
	switch (reg) {
	case SN6040_VENDOR_ID:
	case SN6040_REVISION_ID:
	case SN6040_PORTA_PIN_SENSE:
	case SN6040_PORTB_PIN_SENSE:
	case SN6040_PORTC_PIN_SENSE:
	case SN6040_PORTD_PIN_SENSE:
	case SN6040_I2SPCM_CONTROL1:
	case SN6040_I2SPCM_CONTROL2:
	case SN6040_I2SPCM_CONTROL3:
	case SN6040_I2SPCM_CONTROL4:
	case SN6040_I2SPCM_CONTROL5:
	case SN6040_I2SPCM_CONTROL6:
	case SN6040_PORTC_DEFAULT_CONFIG:
	case SN6040_PORTB_DEFAULT_CONFIG:
		return 4;
	case SN6040_EQ_ENABLE_BYPASS:
	case SN6040_EQ_B0_COEFF:
	case SN6040_EQ_B1_COEFF:
	case SN6040_EQ_B2_COEFF:
	case SN6040_EQ_A1_COEFF:
	case SN6040_EQ_A2_COEFF:
	case SN6040_DAC1_CONVERTER_FORMAT:
	case SN6040_DAC2_CONVERTER_FORMAT:
	case SN6040_DAC3_CONVERTER_FORMAT:
	case SN6040_ADC1_CONVERTER_FORMAT:
	case SN6040_ADC2_CONVERTER_FORMAT:
	case SN6040_DIGITAL_BIOS_TEST2:
	case SN6040_CODEC_TEST0:
	case SN6040_CODEC_TEST2:
	case SN6040_CODEC_TEST9:
	case SN6040_CODEC_TEST16:
	case SN6040_CODEC_TEST20:
	case SN6040_CODEC_TEST24:
	case SN6040_CODEC_TEST25:
	case SN6040_CODEC_TEST26:
	case SN6040_ANALOG_TEST3:
	case SN6040_ANALOG_TEST4:
	case SN6040_ANALOG_TEST5:
	case SN6040_ANALOG_TEST6:
	case SN6040_ANALOG_TEST7:
	case SN6040_ANALOG_TEST8:
	case SN6040_ANALOG_TEST9:
	case SN6040_ANALOG_TEST10:
	case SN6040_ANALOG_TEST11:
	case SN6040_ANALOG_TEST12:
	case SN6040_ANALOG_TEST13:
	case SN6040_DIGITAL_TEST0:
	case SN6040_DIGITAL_TEST1:
	case SN6040_DIGITAL_TEST2:
	case SN6040_DIGITAL_TEST11:
	case SN6040_DIGITAL_TEST12:
	case SN6040_DIGITAL_TEST13:
	case SN6040_DIGITAL_TEST15:
	case SN6040_DIGITAL_TEST16:
	case SN6040_DIGITAL_TEST17:
	case SN6040_DIGITAL_TEST18:
	case SN6040_DIGITAL_TEST19:
	case SN6040_DIGITAL_TEST20:
		return 2;
	default:
		return 1;
	}
}

/* get register's readable status */
static bool sn6040_readable_register(struct device *dev, unsigned int reg)
{
	//TODO
	switch(reg){
		case SN6040_VENDOR_ID:
		case SN6040_REVISION_ID:
		case SN6040_CURRENT_BCLK_FREQUENCY:
		case SN6040_AFG_POWER_STATE:
		case SN6040_AFG_UNSOLICITED:
		case SN6040_AFG_GPIO_DATA:
		case SN6040_AFG_GPIO_ENABLE:
		case SN6040_AFG_GPIO_DIRECTION:
		case SN6040_AFG_GPIO_WAKE:
		case SN6040_AFG_GPIO_UM_ENABLE:
		case SN6040_AFG_GPIO_STICKY_MASK:
		case SN6040_DAC1_CONVERTER_FORMAT:
		case SN6040_DAC1_AMP_GAIN_RIGHT:
		case SN6040_DAC1_AMP_GAIN_LEFT:
		case SN6040_DAC1_POWER_STATE:
		case SN6040_DAC1_CONVERTER_STREAM_CHANNEL:
		case SN6040_DAC1_EAPD:
		case SN6040_DAC2_CONVERTER_FORMAT:
		case SN6040_DAC2_AMP_GAIN_RIGHT:
		case SN6040_DAC2_AMP_GAIN_LEFT:
		case SN6040_DAC2_POWER_STATE:
		case SN6040_DAC2_CONVERTER_STREAM_CHANNEL:
		case SN6040_ADC1_CONVERTER_FORMAT:
		case SN6040_ADC1_AMP_GAIN_RIGHT_0:
		case SN6040_ADC1_AMP_GAIN_LEFT_0:
		case SN6040_ADC1_AMP_GAIN_RIGHT_1:
		case SN6040_ADC1_AMP_GAIN_LEFT_1:
		case SN6040_ADC1_AMP_GAIN_RIGHT_2:
		case SN6040_ADC1_AMP_GAIN_LEFT_2:
		case SN6040_ADC1_AMP_GAIN_RIGHT_3:
		case SN6040_ADC1_AMP_GAIN_LEFT_3:
		case SN6040_ADC1_AMP_GAIN_RIGHT_4:
		case SN6040_ADC1_AMP_GAIN_LEFT_4:
		case SN6040_ADC1_AMP_GAIN_RIGHT_5:
		case SN6040_ADC1_AMP_GAIN_LEFT_5:
		case SN6040_ADC1_AMP_GAIN_RIGHT_6:
		case SN6040_ADC1_AMP_GAIN_LEFT_6:
		case SN6040_ADC1_CONNECTION_SELECT:
		case SN6040_ADC1_POWER_STATE:
		case SN6040_ADC1_CONVERTER_STREAM_CHANNEL:
		case SN6040_ADC2_CONVERTER_FORMAT:
		case SN6040_ADC2_AMP_GAIN_RIGHT_0:
		case SN6040_ADC2_AMP_GAIN_LEFT_0:
		case SN6040_ADC2_AMP_GAIN_RIGHT_1:
		case SN6040_ADC2_AMP_GAIN_LEFT_1:
		case SN6040_ADC2_AMP_GAIN_RIGHT_2:
		case SN6040_ADC2_AMP_GAIN_LEFT_2:
		case SN6040_ADC2_CONNECTION_SELECT:
		case SN6040_ADC2_POWER_STATE:
		case SN6040_ADC2_CONVERTER_STREAM_CHANNEL:
		case SN6040_PORTA_CONNECTION_SELECT:
		case SN6040_PORTA_POWER_STATE:
		case SN6040_PORTA_PIN_CTRL:
		case SN6040_PORTA_UNSOLICITED_RESPONSE:
		case SN6040_PORTA_PIN_SENSE:
		case SN6040_PORTA_EAPD:
		case SN6040_PORTB_POWER_STATE:
		case SN6040_PORTB_PIN_CTRL:
		case SN6040_PORTB_UNSOLICITED_RESPONSE:
		case SN6040_PORTB_PIN_SENSE:
		case SN6040_PORTB_EAPD:
		case SN6040_PORTB_AMP_GAIN_RIGHT:
		case SN6040_PORTB_AMP_GAIN_LEFT:
		case SN6040_PORTC_POWER_STATE:
		case SN6040_PORTC_PIN_CTRL:
		case SN6040_PORTC_UNSOLICITED_RESPONSE:
		case SN6040_PORTC_PIN_SENSE:
		case SN6040_PORTC_AMP_GAIN_RIGHT:
		case SN6040_PORTC_AMP_GAIN_LEFT:
		case SN6040_PORTD_POWER_STATE:
		case SN6040_PORTD_PIN_CTRL:
		case SN6040_PORTD_UNSOLICITED_RESPONSE:
		case SN6040_PORTD_PIN_SENSE:
		case SN6040_PORTD_AMP_GAIN_RIGHT:
		case SN6040_PORTD_AMP_GAIN_LEFT:
		case SN6040_PORTG_POWER_STATE:
		case SN6040_PORTG_PIN_CTRL:
		case SN6040_PORTG_CONNECTION_SELECT:
		case SN6040_PORTG_EAPD:
		case SN6040_MIXER_POWER_STATE:
		case SN6040_MIXER_AMP_GAIN_RIGHT_0:
		case SN6040_MIXER_AMP_GAIN_LEFT_0:
		case SN6040_MIXER_AMP_GAIN_RIGHT_1:
		case SN6040_MIXER_AMP_GAIN_LEFT_1:
		case SN6040_EQ_ENABLE_BYPASS_LSB:
		case SN6040_EQ_ENABLE_BYPASS_MSB:
		case SN6040_EQ_B0_COEFF_LSB:
		case SN6040_EQ_B0_COEFF_MSB:
		case SN6040_EQ_B1_COEFF_LSB:
		case SN6040_EQ_B1_COEFF_MSB:
		case SN6040_EQ_B2_COEFF_LSB:
		case SN6040_EQ_B2_COEFF_MSB:
		case SN6040_EQ_A1_COEFF_LSB:
		case SN6040_EQ_A1_COEFF_MSB:
		case SN6040_EQ_A2_COEFF_LSB:
		case SN6040_EQ_A2_COEFF_MSB:
		case SN6040_EQ_G_COEFF_REGISTER:
		case SN6040_SPKR_DRC_ENABLE_STEP:
		case SN6040_SPKR_DRC_TEST:
		case SN6040_DIGITAL_BIOS_TEST0_LSB:
		case SN6040_DIGITAL_BIOS_TEST0_MSB:
		case SN6040_DIGITAL_BIOS_TEST2_LSB:
		case SN6040_DIGITAL_BIOS_TEST2_MSB:
		case SN6040_DIGITAL_BIOS_TEST3_MSB:
		case SN6040_I2SPCM_CONTROL1:
		case SN6040_I2SPCM_CONTROL2:
		case SN6040_I2SPCM_CONTROL3:
		case SN6040_I2SPCM_CONTROL4:
		case SN6040_I2SPCM_CONTROL5:
		case SN6040_I2SPCM_CONTROL6:
		case SN6040_UM_INTERRUPT_CRTL:
		case SN6040_CODEC_TEST0:
		case SN6040_CODEC_TEST2:
		case SN6040_CODEC_TEST20:
		case SN6040_CODEC_TEST24:
		case SN6040_CODEC_TEST25:
		case SN6040_CODEC_TEST26:
		case SN6040_ANALOG_TEST4:
		case SN6040_ANALOG_TEST5:
		case SN6040_ANALOG_TEST6:
		case SN6040_ANALOG_TEST7:
		case SN6040_ANALOG_TEST8:
		case SN6040_ANALOG_TEST9:
		case SN6040_ANALOG_TEST10:
		case SN6040_ANALOG_TEST11:
		case SN6040_ANALOG_TEST12:
		case SN6040_ANALOG_TEST13:
		case SN6040_DIGITAL_TEST0:
		case SN6040_DIGITAL_TEST1:
		case SN6040_DIGITAL_TEST2:
		case SN6040_DIGITAL_TEST11:
		case SN6040_DIGITAL_TEST12:
		case SN6040_DIGITAL_TEST13:
		case SN6040_DIGITAL_TEST15:
		case SN6040_DIGITAL_TEST16:
		case SN6040_DIGITAL_TEST17:
		case SN6040_DIGITAL_TEST18:
		case SN6040_DIGITAL_TEST19:
		case SN6040_DIGITAL_TEST20:
			return true;
		default:
			return false;
	}
}

/* get register's volatile status */
static bool sn6040_volatile_register(struct device *dev, unsigned int reg)
{
	//TODO
	switch(reg) {
		case SN6040_VENDOR_ID:
		case SN6040_REVISION_ID:
		case SN6040_UM_INTERRUPT_CRTL:
		case SN6040_DIGITAL_TEST11:
		case SN6040_PORTA_PIN_SENSE:
		case SN6040_PORTB_PIN_SENSE:
		case SN6040_PORTC_PIN_SENSE:
		case SN6040_PORTD_PIN_SENSE:
		case SN6040_AFG_GPIO_DATA:
		case SN6040_EQ_BAND_CONTROL:
			return true;
		default:
			return false;
	}
}

/* reg raw write */
static int sn6040_reg_raw_write(struct i2c_client *client,
				 unsigned int reg,
				 const void *val, size_t val_count)
{
	struct device *dev = &client->dev;
	u8 buf[2 + SN6040_MAX_EQ_COEFF];
	int ret;

	if (WARN_ON(val_count + 2 > sizeof(buf)))
		return -EINVAL;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	memcpy(buf + 2, val, val_count);

	ret = i2c_master_send(client, buf, val_count + 2);
	if (ret != val_count + 2) {
		dev_err(dev, "I2C write failed, ret = %d\n", ret);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

/* reg write */
static int sn6040_reg_write(void *context, unsigned int reg,
			     unsigned int value)
{
	__le32 raw_value;
	unsigned int size;

	size = sn6040_register_size(reg);

	printk("+reg_write:reg=0x%x;size=%d;reg_val=0x%x+",reg,size,value);
	raw_value = cpu_to_le32(value);
	return sn6040_reg_raw_write(context, reg, &raw_value, size);
}

/* reg read */
static int sn6040_reg_read(void *context, unsigned int reg,
			    unsigned int *value)
{
	struct i2c_client *client = context;
	struct device *dev = &client->dev;
	__le32 recv_buf = 0;
	struct i2c_msg msgs[2];
	unsigned int size;
	u8 send_buf[2];
	int ret;

	size = sn6040_register_size(reg);

	send_buf[0] = reg >> 8;
	send_buf[1] = reg & 0xff;

	msgs[0].addr = client->addr;
	msgs[0].len = sizeof(send_buf);
	msgs[0].buf = send_buf;
	msgs[0].flags = 0;

	msgs[1].addr = client->addr;
	msgs[1].len = size;
	msgs[1].buf = (u8 *)&recv_buf;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev, "Failed to read register, ret = %d\n", ret);
		return ret < 0 ? ret : -EIO;
	}

	*value = le32_to_cpu(recv_buf);
	return 0;
}

/* get suggested pre_div valuce from mclk frequency */
static unsigned int get_div_from_mclk(unsigned int mclk)
{
	unsigned int div = 8;
	int i;

	for (i = 0; i < ARRAY_SIZE(mclk_pre_div); i++) {
		if (mclk <= mclk_pre_div[i].mclk) {
			div = mclk_pre_div[i].div;
			break;
		}
	}
	return div;
}

static int sn6040_config_pll(struct sn6040_priv *sn6040)
{
	struct device *dev = sn6040->dev;
	unsigned int pre_div;
	unsigned int pre_div_val;
	unsigned int pll_input;
	unsigned int pll_output;
	unsigned int int_div;
	unsigned int frac_div;
	u64 frac_num;
	unsigned int frac;
	unsigned int sample_rate = sn6040->sample_rate;
	int pt_sample_per_sync = 2;
	int pt_clock_per_sample = 96;
	int pt_voc_per_sync = 3072;
	int regdbtl_val;
	unsigned int bclk_ratio = sn6040->bclk_ratio;
	unsigned int bclk_rate;

	switch (sample_rate) {
	case 44100:
	case 48000:
	case 32000:
	case 24000:
	case 16000:
		break;

	case 96000:
		pt_sample_per_sync = 1;
		pt_clock_per_sample = 48;
		pt_voc_per_sync = 1536;
		break;

	case 192000:
		pt_sample_per_sync = 0;
		pt_clock_per_sample = 24;
		pt_voc_per_sync = 768;
		break;

	default:
		dev_err(dev, "Unsupported sample rate %d\n", sample_rate);
		return -EINVAL;
	}

	regmap_read(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, &regdbtl_val);

	/* Configure PLL settings */
	if( regdbtl_val & 0x40 ){  //BCLK as reference clock
		printk("+1+bclk_ratio:%d+\n", bclk_ratio);
		if( bclk_ratio == 0){
			bclk_ratio = sn6040->frame_size;
		}
		bclk_rate = sample_rate * bclk_ratio;
		printk("+bclk_rate:%d+\n", bclk_rate);
		pre_div =  get_div_from_mclk(bclk_rate);
		pll_input = bclk_rate / pre_div;
	} else {  //MCLK as reference clock
		pre_div = get_div_from_mclk(sn6040->mclk_rate);
		pll_input = sn6040->mclk_rate / pre_div;
	}
	pll_output = sample_rate * pt_voc_per_sync;
	int_div = pll_output / pll_input;
	frac_div = pll_output - (int_div * pll_input);

	if (frac_div) {
		frac_div *= 1000;
		frac_div /= pll_input;
		frac_num = (u64)(4000 + frac_div) * ((1 << 20) - 4);
		do_div(frac_num, 7);
		frac = ((u32)frac_num + 499) / 1000;
	}
	pre_div_val = (pre_div - 1) * 2;

	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST4,
		     (pre_div_val << 8));
	if (frac_div == 0) {
		/* Int mode */
		regmap_write(sn6040->regmap, SN6040_ANALOG_TEST6, 0x000);
		regmap_write(sn6040->regmap, SN6040_ANALOG_TEST7, 0x100);
	} else {
		/* frac mode */
		regmap_write(sn6040->regmap, SN6040_ANALOG_TEST6,
			     frac & 0xfff);
		regmap_write(sn6040->regmap, SN6040_ANALOG_TEST7,
			     (u8)(frac >> 12));
	}

	int_div--;
	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST8, int_div);

	/* configure PLL tracking */
	// if (frac_div == 0) {
	// 	/* disable PLL tracking */
	// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST16, 0x00);
	// } else {
	// 	/* configure and enable PLL tracking */
	// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST16,
	// 		     (pt_sample_per_sync << 4) & 0xf0);
	// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST17,
	// 		     pt_clock_per_sample);
	// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST18,
	// 		     pt_clock_per_sample * 3 / 2);
	// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST19, 0x01);
	// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST20, 0x02);
	// 	regmap_update_bits(sn6040->regmap, SN6040_DIGITAL_TEST16,
	// 			   0x01, 0x01);
	// }

	return 0;
}

static int sn6040_config_i2spcm(struct sn6040_priv *sn6040)
{
	//TODO
	struct device *dev = sn6040->dev;
	unsigned int bclk_rate = 0;
	int is_i2s = 0;
	int has_one_bit_delay = 0;
	int is_frame_inv = 0;
	int is_bclk_inv = 0;
	int pulse_len;
	int frame_len = sn6040->frame_size;
	int sample_size = sn6040->sample_size;
	int i2s_right_slot;
	int i2s_right_pause_interval = 0;
	int i2s_right_pause_pos;
	int is_big_endian = 1;
	u64 div;
	unsigned int mod;
	union sn6040_reg_i2spcm_ctrl_reg1 reg1;
	union sn6040_reg_i2spcm_ctrl_reg2 reg2;
	union sn6040_reg_i2spcm_ctrl_reg3 reg3;
	union sn6040_reg_i2spcm_ctrl_reg4 reg4;
	union sn6040_reg_i2spcm_ctrl_reg5 reg5;
	union sn6040_reg_i2spcm_ctrl_reg6 reg6;
	union sn6040_reg_digital_bios_test2 regdbt2;
	const unsigned int fmt = sn6040->dai_fmt;

	if (frame_len <= 0) {
		dev_err(dev, "Incorrect frame len %d\n", frame_len);
		return -EINVAL;
	}

	if (sample_size <= 0) {
		dev_err(dev, "Incorrect sample size %d\n", sample_size);
		return -EINVAL;
	}

	dev_dbg(dev, "config_i2spcm set_dai_fmt- %08x\n", fmt);

	regdbt2.ulval = 0xec;

	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		reg2.r.tx_master = 1;
		reg3.r.rx_master = 1;
		dev_dbg(dev, "Sets Master mode\n");
		printk("+Sets Master mode+\n");
		break;

	case SND_SOC_DAIFMT_CBS_CFS:
		reg2.r.tx_master = 0;
		reg3.r.rx_master = 0;
		dev_dbg(dev, "Sets Slave mode\n");
		printk("+Sets Slave mode+\n");
		break;

	default:
		dev_err(dev, "Unsupported DAI master mode\n");
		printk("+Unsupported DAI master mode+\n");
		return -EINVAL;
	}

	/* set format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		is_i2s = 1;
		has_one_bit_delay = 1;
		pulse_len = frame_len / 2;
		break;

	case SND_SOC_DAIFMT_RIGHT_J:
		is_i2s = 1;
		pulse_len = frame_len / 2;
		break;

	case SND_SOC_DAIFMT_LEFT_J:
		is_i2s = 1;
		pulse_len = frame_len / 2;
		break;

	default:
		dev_err(dev, "Unsupported DAI format\n");
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		is_frame_inv = is_i2s;
		is_bclk_inv = is_i2s;
		break;

	case SND_SOC_DAIFMT_IB_IF:
		is_frame_inv = !is_i2s;
		is_bclk_inv = !is_i2s;
		break;

	case SND_SOC_DAIFMT_IB_NF:
		is_frame_inv = is_i2s;
		is_bclk_inv = !is_i2s;
		break;

	case SND_SOC_DAIFMT_NB_IF:
		is_frame_inv = !is_i2s;
		is_bclk_inv = is_i2s;
		break;

	default:
		dev_err(dev, "Unsupported DAI clock inversion\n");
		return -EINVAL;
	}

	reg1.r.rx_data_one_line = 1;
	reg1.r.tx_data_one_line = 1;

	if (is_i2s) {
		i2s_right_slot = (frame_len / 2) / BITS_PER_SLOT;
		i2s_right_pause_interval = (frame_len / 2) % BITS_PER_SLOT;
		i2s_right_pause_pos = i2s_right_slot * BITS_PER_SLOT;
	}

	reg1.r.rx_ws_pol = is_frame_inv;
	reg1.r.rx_ws_wid = pulse_len - 1;

	reg1.r.rx_frm_len = frame_len / BITS_PER_SLOT - 1;
	reg1.r.rx_sa_size = (sample_size / BITS_PER_SLOT) - 1;

	reg1.r.tx_ws_pol = reg1.r.rx_ws_pol;
	reg1.r.tx_ws_wid = pulse_len - 1;
	reg1.r.tx_frm_len = reg1.r.rx_frm_len;
	reg1.r.tx_sa_size = reg1.r.rx_sa_size;

	reg2.r.tx_endian_sel = !is_big_endian;
	reg2.r.tx_dstart_dly = has_one_bit_delay;
	if (sn6040->en_aec_ref)
		reg2.r.tx_dstart_dly = 0;

	reg3.r.rx_endian_sel = !is_big_endian;
	reg3.r.rx_dstart_dly = has_one_bit_delay;

	reg4.ulval = 0;

	if (is_i2s) {
		reg2.r.tx_slot_1 = 0;
		reg2.r.tx_slot_2 = i2s_right_slot;
		reg3.r.rx_slot_1 = 0;
		if (sn6040->en_aec_ref)
			reg3.r.rx_slot_2 = 0;
		else
			reg3.r.rx_slot_2 = i2s_right_slot;
		reg2.r.tx_en_ch1 = 1;
		reg2.r.tx_en_ch2 = 1;
		reg3.r.rx_en_ch1 = 1;
		reg3.r.rx_en_ch2 = 1;
		reg2.r.tx_data_neg_bclk = 1;
		reg3.r.rx_data_neg_bclk = 1;
		reg6.r.rx_pause_start_pos = i2s_right_pause_pos;
		reg6.r.rx_pause_cycles = i2s_right_pause_interval;
		reg6.r.tx_pause_start_pos = i2s_right_pause_pos;
		reg6.r.tx_pause_cycles = i2s_right_pause_interval;
	} else {
		dev_err(dev, "TDM mode is not implemented yet\n");
		return -EINVAL;
	}
	regdbt2.r.i2s_bclk_invert = is_bclk_inv;

	reg1.r.rx_data_one_line = 1;
	reg1.r.tx_data_one_line = 1;

	/* Configures the BCLK output */
	bclk_rate = sn6040->sample_rate * frame_len;
	reg5.r.i2s_pcm_clk_div_chan_en = 0;

	/* Disables bclk output before setting new value */
	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL5, 0);

	if (reg2.r.tx_master) {
		/* Configures BCLK rate */
		div = PLL_OUT_HZ_48;
		mod = do_div(div, bclk_rate);
		if (mod) {
			dev_err(dev, "Unsupported BCLK %dHz\n", bclk_rate);
			return -EINVAL;
		}
		dev_dbg(dev, "enables BCLK %dHz output\n", bclk_rate);
		printk("+enables BCLK %dHz output+\n", bclk_rate);
		reg5.r.i2s_pcm_clk_div = (u32)div - 1;
		reg5.r.i2s_pcm_clk_div_chan_en = 1;
	}

	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL1, reg1.ulval);
	regmap_update_bits(sn6040->regmap, SN6040_I2SPCM_CONTROL2, 0xffffffff,
			   reg2.ulval);
	regmap_update_bits(sn6040->regmap, SN6040_I2SPCM_CONTROL3, 0xffffffff,
			   reg3.ulval);
	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL4, reg4.ulval);
	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL6, reg6.ulval);
	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL5, reg5.ulval);

	regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2,
		     regdbt2.ulval);

	return 0;
}

static int afg_power_ev(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	//TODO
	struct snd_soc_component *codec = snd_soc_dapm_to_component(w->dapm);
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST0_LSB,
				   0x10, 0x00);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		regmap_update_bits(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST0_LSB,
				   0x10, 0x10);
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new sn6040_snd_controls[] = {
	//TODO
	SOC_DOUBLE_R_TLV("PortD Boost Volume", SN6040_PORTD_AMP_GAIN_LEFT,
			 SN6040_PORTD_AMP_GAIN_RIGHT, 0, 3, 0, boost_tlv),
	SOC_DOUBLE_R_TLV("PortC Boost Volume", SN6040_PORTC_AMP_GAIN_LEFT,
			 SN6040_PORTC_AMP_GAIN_RIGHT, 0, 3, 0, boost_tlv),
	SOC_DOUBLE_R_TLV("PortB Boost Volume", SN6040_PORTB_AMP_GAIN_LEFT,
			 SN6040_PORTB_AMP_GAIN_RIGHT, 0, 3, 0, boost_tlv),
	SOC_DOUBLE_R_TLV("PortB ADC1 Volume", SN6040_ADC1_AMP_GAIN_LEFT_0,
			 SN6040_ADC1_AMP_GAIN_RIGHT_0, 0, 0x4a, 0, adc_tlv),
	SOC_DOUBLE_R_TLV("PortD ADC1 Volume", SN6040_ADC1_AMP_GAIN_LEFT_1,
			 SN6040_ADC1_AMP_GAIN_RIGHT_1, 0, 0x4a, 0, adc_tlv),
	SOC_DOUBLE_R_TLV("PortC ADC1 Volume", SN6040_ADC1_AMP_GAIN_LEFT_2,
			 SN6040_ADC1_AMP_GAIN_RIGHT_2, 0, 0x4a, 0, adc_tlv),
	SOC_DOUBLE_R_TLV("DAC1 Volume", SN6040_DAC1_AMP_GAIN_LEFT,
			 SN6040_DAC1_AMP_GAIN_RIGHT, 0, 0x4a, 0, dac_tlv),
	SOC_DOUBLE_R("DAC1 Switch", SN6040_DAC1_AMP_GAIN_LEFT,
		     SN6040_DAC1_AMP_GAIN_RIGHT, 7,  1, 0),
	SOC_DOUBLE_R_TLV("DAC2 Volume", SN6040_DAC2_AMP_GAIN_LEFT,
			 SN6040_DAC2_AMP_GAIN_RIGHT, 0, 0x4a, 0, dac_tlv),
	SOC_SINGLE("PortA HP Amp Switch", SN6040_PORTA_PIN_CTRL, 7, 1, 0),
};

static int sn6040_set_portA(struct snd_soc_component *codec)
{
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	unsigned int lch, rch;
	sn6040->codec = codec;

	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, 0x6C);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST4, 0x000);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST5, 0x000);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST6, 0xBF8);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST7, 0x193);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST8, 0x02F);

	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_MSB, 0x01);
	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, 0xEC);
	// regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL1, 0x8E7F8E7F);
	// regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL2, 0xA8002003);
	// regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL3, 0x88002003);

	regmap_write(sn6040->regmap, SN6040_CODEC_TEST0, 0x040);

	regmap_write(sn6040->regmap, SN6040_DAC1_CONVERTER_FORMAT, 0x0031);
	regmap_write(sn6040->regmap, SN6040_DAC2_CONVERTER_FORMAT, 0x0031);
	regmap_write(sn6040->regmap, SN6040_AFG_POWER_STATE, 0x00);
	regmap_write(sn6040->regmap, SN6040_DAC1_POWER_STATE, 0x00);
	regmap_write(sn6040->regmap, SN6040_DAC2_POWER_STATE, 0x03);

	// regmap_write(sn6040->regmap, SN6040_DAC1_AMP_GAIN_RIGHT, 0x4A);
	// regmap_write(sn6040->regmap, SN6040_DAC1_AMP_GAIN_LEFT, 0x4A);
	// regmap_write(sn6040->regmap, SN6040_DAC2_AMP_GAIN_RIGHT, 0x4A);
	// regmap_write(sn6040->regmap, SN6040_DAC2_AMP_GAIN_LEFT, 0x4A);

	regmap_write(sn6040->regmap, SN6040_PORTG_POWER_STATE, 0x03);
	regmap_write(sn6040->regmap, SN6040_PORTG_CONNECTION_SELECT, 0x01);
	regmap_write(sn6040->regmap, SN6040_PORTG_PIN_CTRL, 0x00);

	regmap_write(sn6040->regmap, SN6040_PORTA_POWER_STATE, 0x00);
	regmap_write(sn6040->regmap, SN6040_PORTA_CONNECTION_SELECT, 0x00);
	regmap_write(sn6040->regmap, SN6040_PORTA_PIN_CTRL, 0xC0);

	regmap_write(sn6040->regmap, SN6040_CODEC_TEST16, 0x000);
	regmap_read(sn6040->regmap, SN6040_DIGITAL_TEST13, &lch);
	regmap_write(sn6040->regmap, SN6040_CODEC_TEST16, 0x040);
	regmap_read(sn6040->regmap, SN6040_DIGITAL_TEST13, &rch);
	printk("+PortA Lch:0x%x; Rch:0x%x+\n", lch, rch);

	return 0;
}

static int sn6040_set_portD(struct snd_soc_component *codec)
{
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	sn6040->codec = codec;

	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, 0x6C);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST4, 0x000);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST5, 0x000);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST6, 0xBF8);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST7, 0x193);
	// regmap_write(sn6040->regmap, SN6040_ANALOG_TEST8, 0x02F);

	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_MSB, 0x01);
	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, 0xEC);
	// regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL1, 0x8E7F8E7F);
	// regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL2, 0xA8002003);
	// regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL3, 0x88002003);

	regmap_write(sn6040->regmap, SN6040_ADC1_CONVERTER_FORMAT, 0x0031);
	regmap_write(sn6040->regmap, SN6040_ADC1_CONNECTION_SELECT, 0x01);
	// regmap_write(sn6040->regmap, SN6040_ADC1_AMP_GAIN_RIGHT_0, 0x4A);
	// regmap_write(sn6040->regmap, SN6040_ADC1_AMP_GAIN_LEFT_0, 0x4A);
	// regmap_write(sn6040->regmap, SN6040_AFG_POWER_STATE, 0x00);
	regmap_write(sn6040->regmap, SN6040_ADC1_POWER_STATE, 0x00);
	// regmap_write(sn6040->regmap, SN6040_PORTD_AMP_GAIN_RIGHT, 0x03);
	// regmap_write(sn6040->regmap, SN6040_PORTD_AMP_GAIN_LEFT, 0x03);
	regmap_write(sn6040->regmap, SN6040_PORTD_POWER_STATE, 0x00);
	regmap_write(sn6040->regmap, SN6040_PORTD_PIN_CTRL, 0x21);
	
	return 0;
}

static int sn6040_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	//TODO
	struct snd_soc_component *codec = dai->component;
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	struct device *dev = codec->dev;
	const unsigned int sample_rate = params_rate(params);
	int sample_size, frame_size;
	int ret;

	/* Data sizes if not using TDM */
	sample_size = params_width(params);

	if (sample_size < 0)
		return sample_size;

	frame_size = snd_soc_params_to_frame_size(params);
	if (frame_size < 0){
		return frame_size;
	}else if (frame_size <= 32)
	{
		frame_size = 32;
	}else if (frame_size > 32 && frame_size <=64)
	{
		frame_size = 64;
	}
	
	//frame_size = 64;

	if (sn6040->mclk_rate == 0) {
		dev_err(dev, "Master clock rate is not configured\n");
		return -EINVAL;
	}

	if (sn6040->bclk_ratio){
		printk("+bclk_ratio:%d+\n", sn6040->bclk_ratio);
		frame_size = sn6040->bclk_ratio;
	}

	switch (sample_rate) {
	case 44100:
	case 48000:
	case 32000:
	case 24000:
	case 16000:
	case 96000:
	case 192000:
		break;

	default:
		dev_err(dev, "Unsupported sample rate %d\n", sample_rate);
		return -EINVAL;
	}

	dev_dbg(dev, "Sample size %d bits, frame = %d bits, rate = %d Hz\n",
		sample_size, frame_size, sample_rate);
	printk("+senary hw param+Sample size %d bits, frame = %d bits, rate = %d Hz\n",
		sample_size, frame_size, sample_rate);

	sn6040->frame_size = frame_size;
	sn6040->sample_size = sample_size;
	sn6040->sample_rate = sample_rate;

	// if (sn6040->pll_changed) {
		printk("+set pll+\n");
		ret = sn6040_config_pll(sn6040);
		// if(ret){
		// 	printk("+set pll failed, reset pll+\n");
		// 	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST4, 0x000);
		// 	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST5, 0x000);
		// 	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST6, 0xBF8);
		// 	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST7, 0x193);
		// 	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST8, 0x02F);
		// }
		// sn6040->pll_changed = false;
	// }

	// if (sn6040->i2spcm_changed) {
		// if(sample_rate == 44100){
		// 	printk("+set i2spcm as 48k default as sample rate is %d+\n", sample_rate);
		// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_MSB, 0x01);
		// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, 0xEC);
		// 	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL1, 0x8E7F8E7F);
		// 	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL2, 0xA8002003);
		// 	regmap_write(sn6040->regmap, SN6040_I2SPCM_CONTROL3, 0x88002003);
		// }else{
			printk("+set i2spcm dynamic+\n");
			sn6040_config_i2spcm(sn6040);
		// }
	// 	sn6040->i2spcm_changed = false;
	// }

	sn6040_set_portA(codec);
	sn6040_set_portD(codec);

	return 0;
}

static int sn6040_set_dai_bclk_ratio(struct snd_soc_dai *dai,
				      unsigned int ratio)
{
	struct snd_soc_component *codec = dai->component;
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	printk("+set bclk_ratio:%d+\n", ratio);
	sn6040->bclk_ratio = ratio;
	return 0;
}

static int sn6040_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
				  unsigned int freq, int dir)
{
	struct snd_soc_component *codec = dai->component;
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);

	if (clk_set_rate(sn6040->mclk, freq)) {
		dev_err(codec->dev, "set clk rate failed\n");
		return -EINVAL;
	}
	printk("+set mclk_rate:%d+\n", freq);
	sn6040->mclk_rate = freq;
	return 0;
}

static int sn6040_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *codec = dai->component;
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	struct device *dev = codec->dev;

	//dev_dbg(dev, "set_dai_fmt- %08x\n", fmt);
	printk("+set_dai_fmt- %08x+\n", fmt);
	/* set master/slave */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_CBS_CFS:
		break;

	default:
		dev_err(dev, "Unsupported DAI master mode\n");
		return -EINVAL;
	}

	/* set format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;

	default:
		dev_err(dev, "Unsupported DAI format\n");
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_NB_IF:
		break;

	default:
		dev_err(dev, "Unsupported DAI clock inversion\n");
		return -EINVAL;
	}

	sn6040->dai_fmt = fmt;
	return 0;
}

/* dapm kcontrol define */
//TODO
static const struct snd_kcontrol_new portaouten_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_PORTA_PIN_CTRL, 6, 1, 0);
	
static const struct snd_kcontrol_new portgouten_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_PORTG_PIN_CTRL, 6, 1, 0);
	
static const struct snd_kcontrol_new portiouten_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_PORTI_PIN_CTRL, 6, 1, 0);
	
static const struct snd_kcontrol_new portbinen_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_PORTB_PIN_CTRL, 5, 1, 0);

static const struct snd_kcontrol_new portcinen_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_PORTC_PIN_CTRL, 5, 1, 0);

static const struct snd_kcontrol_new portdinen_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_PORTD_PIN_CTRL, 5, 1, 0);
	
static const struct snd_kcontrol_new i2sadc1l_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL2, 0, 1, 0);

static const struct snd_kcontrol_new i2sadc1r_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL2, 1, 1, 0);

static const struct snd_kcontrol_new i2sadc2l_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL2, 2, 1, 0);

static const struct snd_kcontrol_new i2sadc2r_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL2, 3, 1, 0);

static const struct snd_kcontrol_new i2sdac1l_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL3, 0, 1, 0);

static const struct snd_kcontrol_new i2sdac1r_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL3, 1, 1, 0);

static const struct snd_kcontrol_new i2sdac2l_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL3, 2, 1, 0);

static const struct snd_kcontrol_new i2sdac2r_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL3, 3, 1, 0);

static const struct snd_kcontrol_new i2sdac3l_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL3, 4, 1, 0);

static const struct snd_kcontrol_new i2sdac3r_ctl =
	SOC_DAPM_SINGLE("Switch", SN6040_I2SPCM_CONTROL3, 5, 1, 0);

static const char * const dac_enum_text[] = {
	"DAC1 Switch", "DAC2 Switch",
};

static const struct soc_enum porta_dac_enum =
SOC_ENUM_SINGLE(SN6040_PORTA_CONNECTION_SELECT, 0, 2, dac_enum_text);

static const struct snd_kcontrol_new porta_mux =
SOC_DAPM_ENUM("PortA Mux", porta_dac_enum);

static const struct soc_enum portg_dac_enum =
SOC_ENUM_SINGLE(SN6040_PORTG_CONNECTION_SELECT, 0, 2, dac_enum_text);

static const struct snd_kcontrol_new portg_mux =
SOC_DAPM_ENUM("PortG Mux", portg_dac_enum);

static const char * const adc1in_sel_text[] = {
	"PortB Switch", "PortD Switch", "PortC Switch", "Widget15 Switch",
};

static const struct soc_enum adc1in_sel_enum =
SOC_ENUM_SINGLE(SN6040_ADC1_CONNECTION_SELECT, 0, 4, adc1in_sel_text);

static const struct snd_kcontrol_new adc1_mux =
SOC_DAPM_ENUM("ADC1 Mux", adc1in_sel_enum);

static const char * const adc2in_sel_text[] = {
	"PortC Switch", "Widget15 Switch",
};

static const struct soc_enum adc2in_sel_enum =
SOC_ENUM_SINGLE(SN6040_ADC2_CONNECTION_SELECT, 0, 2, adc2in_sel_text);

static const struct snd_kcontrol_new adc2_mux =
SOC_DAPM_ENUM("ADC2 Mux", adc2in_sel_enum);

static const struct snd_kcontrol_new wid15_mix[] = {
	SOC_DAPM_SINGLE("DAC1L Switch", SN6040_MIXER_AMP_GAIN_LEFT_0, 7, 1, 1),
	SOC_DAPM_SINGLE("DAC1R Switch", SN6040_MIXER_AMP_GAIN_RIGHT_0, 7, 1, 1),
	SOC_DAPM_SINGLE("DAC2L Switch", SN6040_MIXER_AMP_GAIN_LEFT_1, 7, 1, 1),
	SOC_DAPM_SINGLE("DAC2R Switch", SN6040_MIXER_AMP_GAIN_RIGHT_1, 7, 1, 1),
};

/* Extern jack device, should set in machine driver */
// static const struct snd_kcontrol_new sn6040_jack_controls[] = {
// 	SOC_DAPM_PIN_SWITCH("Headphone"),
// 	SOC_DAPM_PIN_SWITCH("Headset Mic"),
// };

#define SN6040_DAPM_SUPPLY_S(wname, wsubseq, wreg, wshift, wmask,  won_val, \
	woff_val, wevent, wflags) \
	{.id = snd_soc_dapm_supply, .name = wname, .kcontrol_news = NULL, \
	.num_kcontrols = 0, .reg = wreg, .shift = wshift, .mask = wmask, \
	.on_val = won_val, .off_val = woff_val, \
	.subseq = wsubseq, .event = wevent, .event_flags = wflags}

/* dapm widgets */
static const struct snd_soc_dapm_widget sn6040_dapm_widgets[] = {
	//TODO
	/*Playback*/
	SND_SOC_DAPM_AIF_IN("In AIF", "Playback", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SWITCH("I2S DAC1L", SND_SOC_NOPM, 0, 0, &i2sdac1l_ctl),
	SND_SOC_DAPM_SWITCH("I2S DAC1R", SND_SOC_NOPM, 0, 0, &i2sdac1r_ctl),
	SND_SOC_DAPM_SWITCH("I2S DAC2L", SND_SOC_NOPM, 0, 0, &i2sdac2l_ctl),
	SND_SOC_DAPM_SWITCH("I2S DAC2R", SND_SOC_NOPM, 0, 0, &i2sdac2r_ctl),
	SND_SOC_DAPM_SWITCH("I2S DAC3L", SND_SOC_NOPM, 0, 0, &i2sdac3l_ctl),
	SND_SOC_DAPM_SWITCH("I2S DAC3R", SND_SOC_NOPM, 0, 0, &i2sdac3r_ctl),

	SND_SOC_DAPM_REG(snd_soc_dapm_dac, "DAC1", SN6040_DAC1_POWER_STATE,
			 0, 0xfff, 0x00, 0x03),

	SND_SOC_DAPM_REG(snd_soc_dapm_dac, "DAC2", SN6040_DAC2_POWER_STATE,
			 0, 0xfff, 0x00, 0x03),

	SND_SOC_DAPM_REG(snd_soc_dapm_dac, "DAC3", SN6040_DAC3_POWER_STATE,
			 0, 0xfff, 0x00, 0x03),

	SND_SOC_DAPM_MUX("PortA Mux", SND_SOC_NOPM, 0, 0, &porta_mux),
	SND_SOC_DAPM_MUX("PortG Mux", SND_SOC_NOPM, 0, 0, &portg_mux),

	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "PortA Power",
			 SN6040_PORTA_POWER_STATE, 0, 0xfff, 0x00, 0x03),

	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "PortG Power",
			 SN6040_PORTG_POWER_STATE, 0, 0xfff, 0x00, 0x03),

	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "PortI Power",
			SN6040_PORTI_POWER_STATE, 0, 0xfff, 0x00, 0x03),

	SN6040_DAPM_SUPPLY_S("AFG Power", 0, SN6040_AFG_POWER_STATE,
			      0, 0xfff, 0x00, 0x03, afg_power_ev,
			      SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

	SND_SOC_DAPM_SWITCH("PortA Out En", SND_SOC_NOPM, 0, 0,
			    &portaouten_ctl),
	SND_SOC_DAPM_SWITCH("PortG Out En", SND_SOC_NOPM, 0, 0,
			    &portgouten_ctl),
	SND_SOC_DAPM_SWITCH("PortI Out En", SND_SOC_NOPM, 0, 0,
			    &portiouten_ctl),
	SND_SOC_DAPM_OUTPUT("PORTA"),
	SND_SOC_DAPM_OUTPUT("PORTG"),
	SND_SOC_DAPM_OUTPUT("PORTI"),
	SND_SOC_DAPM_OUTPUT("AEC REF"),

	/*Capture*/
	SND_SOC_DAPM_AIF_OUT("Out AIF", "Capture", 0, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_SWITCH("I2S ADC1L", SND_SOC_NOPM, 0, 0, &i2sadc1l_ctl),
	SND_SOC_DAPM_SWITCH("I2S ADC1R", SND_SOC_NOPM, 0, 0, &i2sadc1r_ctl),
	SND_SOC_DAPM_SWITCH("I2S ADC2L", SND_SOC_NOPM, 0, 0, &i2sadc2l_ctl),
	SND_SOC_DAPM_SWITCH("I2S ADC2R", SND_SOC_NOPM, 0, 0, &i2sadc2r_ctl),

	SND_SOC_DAPM_REG(snd_soc_dapm_adc, "ADC1", SN6040_ADC1_POWER_STATE,
			 0, 0xff, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_adc, "ADC2", SN6040_ADC2_POWER_STATE,
			 0, 0xff, 0x00, 0x03),

	SND_SOC_DAPM_MUX("ADC1 Mux", SND_SOC_NOPM, 0, 0, &adc1_mux),
	SND_SOC_DAPM_MUX("ADC2 Mux", SND_SOC_NOPM, 0, 0, &adc2_mux),

	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "PortB Power",
			 SN6040_PORTB_POWER_STATE, 0, 0xfff, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "PortC Power",
			 SN6040_PORTC_POWER_STATE, 0, 0xfff, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "PortD Power",
			 SN6040_PORTD_POWER_STATE, 0, 0xfff, 0x00, 0x03),
	SND_SOC_DAPM_REG(snd_soc_dapm_supply, "Widget15 Power",
			 SN6040_MIXER_POWER_STATE, 0, 0xfff, 0x00, 0x03),

	SND_SOC_DAPM_MIXER("Widget15 Mixer", SND_SOC_NOPM, 0, 0,
			   wid15_mix, ARRAY_SIZE(wid15_mix)),
	SND_SOC_DAPM_SWITCH("PortB In En", SND_SOC_NOPM, 0, 0, &portbinen_ctl),
	SND_SOC_DAPM_SWITCH("PortC In En", SND_SOC_NOPM, 0, 0, &portcinen_ctl),
	SND_SOC_DAPM_SWITCH("PortD In En", SND_SOC_NOPM, 0, 0, &portdinen_ctl),

	//MICBIAS to be check
	SND_SOC_DAPM_MICBIAS("Headset Bias", SN6040_ANALOG_TEST11, 1, 0),
	SND_SOC_DAPM_MICBIAS("PortB Mic Bias", SN6040_PORTB_PIN_CTRL, 2, 0),
	SND_SOC_DAPM_MICBIAS("PortD Mic Bias", SN6040_PORTD_PIN_CTRL, 2, 0),
	SND_SOC_DAPM_INPUT("PORTB"),
	SND_SOC_DAPM_INPUT("PORTC"),
	SND_SOC_DAPM_INPUT("PORTD"),

	/* Extern jack device, should set in machine driver */
	// SND_SOC_DAPM_HP("Headphone", NULL),
	// SND_SOC_DAPM_MIC("Headset Mic", NULL)
};

/* dapm route */
static const struct snd_soc_dapm_route sn6040_intercon[] = {
	//TODO
	/* Playback */
	{"In AIF", NULL, "AFG Power"},
	{"I2S DAC1L", "Switch", "In AIF"},
	{"I2S DAC1R", "Switch", "In AIF"},
	{"I2S DAC2L", "Switch", "In AIF"},
	{"I2S DAC2R", "Switch", "In AIF"},
	{"I2S DAC3L", "Switch", "In AIF"},
	{"I2S DAC3R", "Switch", "In AIF"},
	{"DAC1", NULL, "I2S DAC1L"},
	{"DAC1", NULL, "I2S DAC1R"},
	{"DAC2", NULL, "I2S DAC2L"},
	{"DAC2", NULL, "I2S DAC2R"},
	{"DAC3", NULL, "I2S DAC3L"},
	{"DAC3", NULL, "I2S DAC3R"},
	{"PortA Mux", "DAC1 Switch", "DAC1"},
	{"PortA Mux", "DAC2 Switch", "DAC2"},
	{"PortG Mux", "DAC1 Switch", "DAC1"},
	{"PortG Mux", "DAC2 Switch", "DAC2"},
	{"Widget15 Mixer", "DAC1L Switch", "DAC1"},
	{"Widget15 Mixer", "DAC1R Switch", "DAC2"},
	{"Widget15 Mixer", "DAC2L Switch", "DAC1"},
	{"Widget15 Mixer", "DAC2R Switch", "DAC2"},
	{"Widget15 Mixer", NULL, "Widget15 Power"},
	{"PortA Out En", "Switch", "PortA Mux"},
	{"PortG Out En", "Switch", "PortG Mux"},
	{"PortI Out En", "Switch", "DAC3"},
	{"PortA Mux", NULL, "PortA Power"},
	{"PortG Mux", NULL, "PortG Power"},
	{"PortA Out En", NULL, "PortA Power"},
	{"PortG Out En", NULL, "PortG Power"},
	{"PortI Out En", NULL, "PortI Power"},
	{"PORTA", NULL, "PortA Out En"},
	{"PORTG", NULL, "PortG Out En"},
	{"PORTI", NULL, "PortI Out En"},

	/* Capture */
	{"PORTD", NULL, "Headset Bias"},
	{"PortB In En", "Switch", "PORTB"},
	{"PortC In En", "Switch", "PORTC"},
	{"PortD In En", "Switch", "PORTD"},
	{"ADC1 Mux", "PortB Switch", "PortB In En"},
	{"ADC1 Mux", "PortC Switch", "PortC In En"},
	{"ADC1 Mux", "PortD Switch", "PortD In En"},
	{"ADC1 Mux", "Widget15 Switch", "Widget15 Mixer"},
	{"ADC2 Mux", "PortC Switch", "PortC In En"},
	{"ADC2 Mux", "Widget15 Switch", "Widget15 Mixer"},
	{"ADC1", NULL, "ADC1 Mux"},
	{"ADC2", NULL, "ADC2 Mux"},
	{"I2S ADC1L", "Switch", "ADC1"},
	{"I2S ADC1R", "Switch", "ADC1"},
	{"I2S ADC2L", "Switch", "ADC2"},
	{"I2S ADC2R", "Switch", "ADC2"},
	{"Out AIF", NULL, "I2S ADC1L"},
	{"Out AIF", NULL, "I2S ADC1R"},
	{"Out AIF", NULL, "I2S ADC2L"},
	{"Out AIF", NULL, "I2S ADC2R"},
	{"Out AIF", NULL, "AFG Power"},
	{"AEC REF", NULL, "Out AIF"},
	{"PortB In En", NULL, "PortB Power"},
	{"PortC In En", NULL, "PortC Power"},
	{"PortD In En", NULL, "PortD Power"},

	/* Extern jack device, should set in machine driver */
	// {"Headphone", NULL, "PORTA"},
	// {"PORTD", NULL, "Headset Mic"},
};

static int sn6040_set_bias_level(struct snd_soc_component *codec,
				  enum snd_soc_bias_level level)
{
	//TODO
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	const enum snd_soc_bias_level old_level =
		snd_soc_component_get_bias_level(codec);

	if (level == SND_SOC_BIAS_STANDBY && old_level == SND_SOC_BIAS_OFF)
		regmap_write(sn6040->regmap, SN6040_AFG_POWER_STATE, 0);
	else if (level == SND_SOC_BIAS_OFF && old_level != SND_SOC_BIAS_OFF)
		regmap_write(sn6040->regmap, SN6040_AFG_POWER_STATE, 3);

	return 0;
}

// /* jack detect code */
// static void sn6040_enable_jack_detect(struct snd_soc_component *codec)
// {
// 	//TODO
// 	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
// 	struct snd_soc_dapm_context *dapm = snd_soc_component_get_dapm(codec);

// 	/* No-sticky input type */
// 	regmap_write(sn6040->regmap, SN6040_AFG_GPIO_STICKY_MASK, 0x1f);

// 	/* Use GPOI0 as interrupt pin */
// 	regmap_write(sn6040->regmap, SN6040_UM_INTERRUPT_CRTL, 0x12);

// 	/* Enables unsolitited message on PortA */
// 	regmap_write(sn6040->regmap, SN6040_PORTA_UNSOLICITED_RESPONSE, 0x80);

// 	/* support both nokia and apple headset set. Monitor time = 275 ms */
// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST15, 0x73);

// 	/* Disable TIP detection */
// 	regmap_write(sn6040->regmap, SN6040_ANALOG_TEST12, 0x300);

// 	/* Switch MusicD3Live pin to GPIO */
// 	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST1, 0);

// 	snd_soc_dapm_mutex_lock(dapm);

// 	snd_soc_dapm_force_enable_pin_unlocked(dapm, "PORTD");
// 	snd_soc_dapm_force_enable_pin_unlocked(dapm, "Headset Bias");
// 	snd_soc_dapm_force_enable_pin_unlocked(dapm, "PortD Mic Bias");

// 	snd_soc_dapm_mutex_unlock(dapm);
// }

// static void sn6040_disable_jack_detect(struct snd_soc_component *codec)
// {
// 	//TODO
// 	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);

// 	regmap_write(sn6040->regmap, SN6040_UM_INTERRUPT_CRTL, 0);
// 	regmap_write(sn6040->regmap, SN6040_PORTA_UNSOLICITED_RESPONSE, 0);
// }

// static int sn6040_jack_status_check(void *data)
// {
// 	//TODO
// 	struct snd_soc_component *codec = data;
// 	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
// 	unsigned int jack;
// 	unsigned int type = 0;
// 	int state = 0;

// 	mutex_lock(&sn6040->lock);

// 	regmap_read(sn6040->regmap, SN6040_PORTA_PIN_SENSE, &jack);
// 	jack = jack >> 24;
// 	regmap_read(sn6040->regmap, SN6040_DIGITAL_TEST11, &type);

// 	if (jack == 0x80) {
// 		type = type >> 8;

// 		if (type & 0x8) {
// 			/* Apple headset */
// 			state |= SND_JACK_HEADSET;
// 			if (type & 0x2)
// 				state |= SND_JACK_BTN_0;
// 		} else if (type & 0x4) {
// 			/* Nokia headset */
// 			state |= SND_JACK_HEADSET;
// 		} else {
// 			/* Headphone */
// 			state |= SND_JACK_HEADPHONE;
// 		}
// 	}

// 	/* clear interrupt */
// 	regmap_write(sn6040->regmap, SN6040_UM_INTERRUPT_CRTL, 0x12);

// 	mutex_unlock(&sn6040->lock);

// 	dev_dbg(codec->dev, "SN6040_HSDETECT type=0x%X,Jack state = %x\n",
// 		type, state);
// 	return state;
// 	return 0;
// }

// static const struct snd_soc_jack_gpio sn6040_jack_gpio = {
// 	//TODO
// 	.name = "headset",
// 	.report = SND_JACK_HEADSET,
// 	.debounce_time = 150,
// 	.wake = true,
// 	.jack_status_check = sn6040_jack_status_check,
// };

// static int sn6040_set_jack(struct snd_soc_component *codec,
// 			    struct snd_soc_jack *jack, void *data)
// {
// 	//TODO
// 	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
// 	int err;
// 	printk("+sn6040_set_jack+");
// 	if (!jack) {
// 		sn6040_disable_jack_detect(codec);
// 		return 0;
// 	}

// 	if (!sn6040->jack_gpio.gpiod_dev) {
// 		sn6040->jack_gpio = sn6040_jack_gpio;
// 		sn6040->jack_gpio.gpiod_dev = codec->dev;
// 		sn6040->jack_gpio.data = codec;
// 		err = snd_soc_jack_add_gpios(jack, 1, &sn6040->jack_gpio);
// 		if (err) {
// 			sn6040->jack_gpio.gpiod_dev = NULL;
// 			return err;
// 		}
// 	}

// 	sn6040_enable_jack_detect(codec);
// 	return 0;
// }

static int por_toI2Sinterface(struct snd_soc_component *codec)
{
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	int ret;
	unsigned int cnt = 0;
	sn6040->codec = codec;

	regmap_write(sn6040->regmap, SN6040_CODEC_TEST24, 0xCBA);
	regmap_write(sn6040->regmap, SN6040_CODEC_TEST24, 0xAAA);
	regmap_write(sn6040->regmap, SN6040_CODEC_TEST25, 0xBC5);
	regmap_write(sn6040->regmap, SN6040_CODEC_TEST25, 0x555);
	regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST2, 0x001);
	mdelay(10);
	do{
		ret = regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST3_MSB, 0x03);
		cnt++;
	}while((ret<0) &&(cnt<5));
	printk("+i2s mode set, ret:0x%d, cnt:0x%d+", ret, cnt);
	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST3_MSB, 0x03);
	return 0;
}

// static struct snd_soc_jack sn6040_headset;

// /* Headset jack detection DAPM pins */
// static struct snd_soc_jack_pin sn6040_headset_pins[] = {
// 	{
// 		.pin = "Headset Mic",
// 		.mask = SND_JACK_MICROPHONE,
// 	},
// 	{
// 		.pin = "Headphone",
// 		.mask = SND_JACK_HEADPHONE,
// 	},
// };

static int sn6040_probe(struct snd_soc_component *codec)
{
	//TO BE MODIFY
	struct sn6040_priv *sn6040 = snd_soc_component_get_drvdata(codec);
	// int reg, reg_val;
	// struct snd_soc_card *card = codec->card;
//int ret;
	printk("+sn6040_probe+\n");
// 	sn6040->codec = codec;

// 	ret = snd_soc_card_jack_new(card, "Headset",
// 				    SND_JACK_HEADSET,
// 				    &sn6040_headset,
// 				    sn6040_headset_pins,
// 				    ARRAY_SIZE(sn6040_headset_pins));
// 	if (ret){
// 		printk("+create jack error+");
// 		return ret;
// 	}

	// snd_soc_component_set_jack(codec, &sn6040_headset, NULL);
	/*
	 * FIXME: below is, again, a very platform-specific init sequence,
	 * but we keep the code here just for simplicity.  It seems that all
	 * existing hardware implementations require this, so there is no very
	 * much reason to move this out of the codec driver to the platform
	 * data.
	 * But of course it's no "right" thing; if you are a good boy, don't
	 * read and follow the code like this!
	 */
	// pm_runtime_get_sync(codec->dev);
	// regmap_write(sn6040->regmap, SN6040_AFG_POWER_STATE, 0);

	// regmap_multi_reg_write(sn6040->regmap, sn6040_reg_init,
	// 		       ARRAY_SIZE(sn6040_reg_init));

	//sn6040_enable_jack_detect(codec);
	por_toI2Sinterface(codec);
	// regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST3_MSB, 0x03);
	regmap_write(sn6040->regmap, SN6040_CODEC_TEST0, 0x040);
	regmap_write(sn6040->regmap, SN6040_DAC3_DIGITAL_CONVERTER_CTRL, 0x01);
	regmap_write(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB, 0x6C);
	//80hz 1db
	regmap_write(sn6040->regmap, SN6040_CODEC_TEST9, 0x01);
	//portc analog microphone
	//regmap_write(sn6040->regmap, SN6040_PORTC_PIN_CTRL, 0x20);
	//regmap_write(sn6040->regmap, SN6040_DIGITAL_TEST0, 0x408);
	// sn6040_set_portA(codec);

	/* configure PortC as input device */
	//regmap_update_bits(sn6040->regmap, SN6040_PORTC_PIN_CTRL,
	//		   0x20, 0x20);

	//regmap_update_bits(sn6040->regmap, SN6040_DIGITAL_BIOS_TEST2_LSB,
	//		   0x84, 0xff);
	// for(reg=0x200;reg<SN6040_REG_MAX;reg++){
	// 	if(sn6040_readable_register(codec->dev, reg)){
	// 		regmap_read(sn6040->regmap, reg, &reg_val);
	// 		printk("reg:%x\treg_val:%x", reg, reg_val);
	// 	}
	// }


	// regmap_write(sn6040->regmap, SN6040_AFG_POWER_STATE, 3);
	// pm_runtime_put(codec->dev);

	return 0;
}

// static const struct snd_soc_component_driver soc_codec_driver_sn6040 = {
// 	.probe = sn6040_probe,
// };

static const struct snd_soc_component_driver soc_codec_driver_sn6040 = {
	//TODO, need to check
	.probe = sn6040_probe,
	.set_bias_level = sn6040_set_bias_level,
	// .set_jack = sn6040_set_jack,
	.controls = sn6040_snd_controls,
	.num_controls = ARRAY_SIZE(sn6040_snd_controls),
	.dapm_widgets = sn6040_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(sn6040_dapm_widgets),
	.dapm_routes = sn6040_intercon,
	.num_dapm_routes = ARRAY_SIZE(sn6040_intercon),
};

/*
 * DAI ops, need to check
 */
static struct snd_soc_dai_ops sn6040_dai_ops = {
	.set_sysclk = sn6040_set_dai_sysclk,
	.set_fmt = sn6040_set_dai_fmt,
	.hw_params = sn6040_hw_params,
	.set_bclk_ratio = sn6040_set_dai_bclk_ratio,
};

// static struct snd_soc_dai_driver soc_codec_sn6040_dai[] = {};

static struct snd_soc_dai_driver soc_codec_sn6040_dai[] = {
	//TODO
	{ /* playback and capture */
		.name = "sn6040-hifi",
		.id	= SN6040_DAI_HIFI,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SN6040_RATES,
			.formats = SN6040_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SN6040_RATES,
			.formats = SN6040_FORMATS,
		},
		.ops = &sn6040_dai_ops,
		.symmetric_rate = 1,
	},
};

/* need to modify */
static const struct regmap_config sn6040_regmap = {
	.reg_bits = 16,
	.val_bits = 32,
	.max_register = SN6040_REG_MAX,
	.reg_defaults = sn6040_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(sn6040_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
	.readable_reg = sn6040_readable_register,
	.volatile_reg = sn6040_volatile_register,
	/* Needs custom read/write functions for various register lengths */
	.reg_read = sn6040_reg_read,
	.reg_write = sn6040_reg_write,
};

#ifdef CONFIG_PM
static int __maybe_unused sn6040_runtime_suspend(struct device *dev)
{
	struct sn6040_priv *sn6040 = dev_get_drvdata(dev);

	clk_disable_unprepare(sn6040->mclk);
	return 0;
}

static int __maybe_unused sn6040_runtime_resume(struct device *dev)
{
	struct sn6040_priv *sn6040 = dev_get_drvdata(dev);

	return clk_prepare_enable(sn6040->mclk);
}
#else
#define sn6040_suspend NULL
#define sn6040_resume NULL
#endif

static int sn6040_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct sn6040_priv *sn6040;
	unsigned int ven_id, rev_id;
	int ret;
	printk("+sn6040_i2c_probe+");
	sn6040 = devm_kzalloc(&i2c->dev, sizeof(struct sn6040_priv),
			       GFP_KERNEL);
	if (!sn6040)
		return -ENOMEM;

	sn6040->regmap = devm_regmap_init(&i2c->dev, NULL, i2c,
					   &sn6040_regmap);
	if (IS_ERR(sn6040->regmap))
		return PTR_ERR(sn6040->regmap);

	mutex_init(&sn6040->lock);

	i2c_set_clientdata(i2c, sn6040);

	sn6040->dev = &i2c->dev;
	sn6040->pll_changed = true;
	sn6040->i2spcm_changed = true;
	sn6040->bclk_ratio = 0;

	sn6040->mclk = devm_clk_get(sn6040->dev, "mclk");
	if (IS_ERR(sn6040->mclk)) {
		dev_err(sn6040->dev, "Failed to get MCLK\n");
		return PTR_ERR(sn6040->mclk);
	}

	regmap_read(sn6040->regmap, SN6040_VENDOR_ID, &ven_id);
	regmap_read(sn6040->regmap, SN6040_REVISION_ID, &rev_id);
	dev_info(sn6040->dev, "codec version: %08x,%08x\n", ven_id, rev_id);
	//printk("+sn6040_reg_read codec version: %08x,%08x+\n", ven_id, rev_id);

	ret = devm_snd_soc_register_component(sn6040->dev,
					      &soc_codec_driver_sn6040,
					      soc_codec_sn6040_dai,
					      ARRAY_SIZE(soc_codec_sn6040_dai));
	if (ret < 0){
		printk("+failed to register codec:%d+", ret);
		return ret;
	}

	pm_runtime_use_autosuspend(sn6040->dev);
	pm_runtime_enable(sn6040->dev);
	printk("+sn6040_i2c_probe end+");
	return 0;
}

static void sn6040_i2c_remove(struct i2c_client *i2c)
{
	pm_runtime_disable(&i2c->dev);
}

static const struct i2c_device_id sn6040_i2c_id[] = {
	{ "sn6040", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, sn6040_i2c_id);

//#ifdef CONFIG_ACPI
//static struct acpi_device_id SN6040_acpi_match[] = {
//	{ "14F11F86", 0 },
//	{},
//};
//MODULE_DEVICE_TABLE(acpi, sn6040_acpi_match);
//#endif
static struct of_device_id sn6040_i2c_ids[] = {
	{ .compatible = "senarytech,sn6040-codec", },
	{}
};

static const struct dev_pm_ops sn6040_runtime_pm = {
	SET_RUNTIME_PM_OPS(sn6040_runtime_suspend, sn6040_runtime_resume,
			   NULL)
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
				pm_runtime_force_resume)
};

static struct i2c_driver sn6040_i2c_driver = {
	.driver = {
		.name = "sn6040",
		//.acpi_match_table = ACPI_PTR(sn6040_acpi_match),
		.of_match_table = of_match_ptr(sn6040_i2c_ids),
		.pm = &sn6040_runtime_pm,
	},
	.probe = sn6040_i2c_probe,
	.remove = sn6040_i2c_remove,
	.id_table = sn6040_i2c_id,
};

module_i2c_driver(sn6040_i2c_driver);

MODULE_DESCRIPTION("ASoC sn6040 Codec Driver");
MODULE_AUTHOR("bo liu <bo.liu@senarytech.com>");
MODULE_LICENSE("GPL");
