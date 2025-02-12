/*
 * ALSA SoC ES7210 codec driver
 *
 * Copyright:   (C) 2018 Everest Semiconductor Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES7210_H
#define _ES7210_H

#define ES7210_RESET_CTL_REG00		0x00
#define ES7210_CLK_ON_OFF_REG01		0x01
#define ES7210_MCLK_CTL_REG02		0x02
#define ES7210_MST_CLK_CTL_REG03	0x03
#define ES7210_MST_LRCDIVH_REG04	0x04
#define ES7210_MST_LRCDIVL_REG05	0x05
#define ES7210_DIGITAL_PDN_REG06	0x06
#define ES7210_ADC_OSR_REG07		0x07
#define ES7210_MODE_CFG_REG08		0x08

#define ES7210_TCT0_CHPINI_REG09	0x09
#define ES7210_TCT1_CHPINI_REG0A	0x0A
#define ES7210_CHIP_STA_REG0B		0x0B

#define ES7210_IRQ_CTL_REG0C		0x0C
#define ES7210_MISC_CTL_REG0D		0x0D
#define ES7210_DMIC_CTL_REG10		0x10

#define ES7210_SDP_CFG1_REG11		0x11
#define ES7210_SDP_CFG2_REG12		0x12

#define ES7210_ADC_AUTOMUTE_REG13	0x13
#define ES7210_ADC34_MUTE_REG14		0x14
#define ES7210_ADC12_MUTE_REG15		0x15

#define ES7210_ALC_SEL_REG16		0x16
#define ES7210_ALC_COM_CFG1_REG17	0x17
#define ES7210_ALC34_LVL_REG18		0x18
#define ES7210_ALC12_LVL_REG19		0x19
#define ES7210_ALC_COM_CFG2_REG1A	0x1A
#define ES7210_ALC4_MAX_GAIN_REG1B	0x1B
#define ES7210_ALC3_MAX_GAIN_REG1C	0x1C
#define ES7210_ALC2_MAX_GAIN_REG1D	0x1D
#define ES7210_ALC1_MAX_GAIN_REG1E	0x1E

#define ES7210_ADC34_HPF2_REG20		0x20
#define ES7210_ADC34_HPF1_REG21		0x21
#define ES7210_ADC12_HPF2_REG22		0x22
#define ES7210_ADC12_HPF1_REG23		0x23

#define ES7210_CHP_ID1_REG3D		0x3D
#define ES7210_CHP_ID0_REG3E		0x3E
#define ES7210_CHP_VER_REG3F		0x3F

#define ES7210_ANALOG_SYS_REG40		0x40

#define ES7210_MICBIAS12_REG41		0x41
#define ES7210_MICBIAS34_REG42		0x42
#define ES7210_MIC1_GAIN_REG43		0x43
#define ES7210_MIC2_GAIN_REG44		0x44
#define ES7210_MIC3_GAIN_REG45		0x45
#define ES7210_MIC4_GAIN_REG46		0x46
#define ES7210_MIC1_LP_REG47		0x47
#define ES7210_MIC2_LP_REG48		0x48
#define ES7210_MIC3_LP_REG49		0x49
#define ES7210_MIC4_LP_REG4A		0x4A
#define ES7210_MIC12_PDN_REG4B		0x4B
#define ES7210_MIC34_PDN_REG4C		0x4C

/* codec private data */
struct es7210_priv {
        struct regmap *regmap;
        struct i2c_client *i2c_client;
        unsigned int dmic_enable;
        unsigned int sysclk;
        struct clk *mclk;
        struct snd_pcm_hw_constraint_list *sysclk_constraints;
        struct regulator *mvdd;
        struct regulator *avdd;
        struct regulator *dvdd;
        struct regulator *pvdd;
        unsigned int tdm_mode;
        struct delayed_work pcm_pop_work;
        int mclk_lrck_ratio;
        int pcm_format;
        int mclk_sclk_ratio; // mclk_sclk_ratio=1 is when mclk hardwired to sclk.
        int sclk_lrck_ratio;

        u8 suspend_reg00[4];
        u8 suspend_reg01[4];
        u8 suspend_reg02[4];
        u8 suspend_reg03[4];
        u8 suspend_reg04[4];
        u8 suspend_reg05[4];
        u8 suspend_reg06[4];
        u8 suspend_reg07[4];
        u8 suspend_reg08[4];
        u8 suspend_reg09[4];
        u8 suspend_reg0A[4];
        u8 suspend_reg0B[4];
        u8 suspend_reg0C[4];
        u8 suspend_reg0D[4];
        u8 suspend_reg10[4];
        u8 suspend_reg11[4];
        u8 suspend_reg12[4];
        u8 suspend_reg13[4];
        u8 suspend_reg14[4];
        u8 suspend_reg15[4];
        u8 suspend_reg16[4];
        u8 suspend_reg17[4];
        u8 suspend_reg18[4];
        u8 suspend_reg19[4];
        u8 suspend_reg1A[4];
        u8 suspend_reg1B[4];
        u8 suspend_reg1C[4];
        u8 suspend_reg1D[4];
        u8 suspend_reg1E[4];
        u8 suspend_reg20[4];
        u8 suspend_reg21[4];
        u8 suspend_reg22[4];
        u8 suspend_reg23[4];
        u8 suspend_reg40[4];
        u8 suspend_reg41[4];
        u8 suspend_reg42[4];
        u8 suspend_reg43[4];
        u8 suspend_reg44[4];
        u8 suspend_reg45[4];
        u8 suspend_reg46[4];
        u8 suspend_reg47[4];
        u8 suspend_reg48[4];
        u8 suspend_reg49[4];
        u8 suspend_reg4A[4];
        u8 suspend_reg4B[4];
        u8 suspend_reg4C[4];
};

#endif	/* _ES7210_H_ */
