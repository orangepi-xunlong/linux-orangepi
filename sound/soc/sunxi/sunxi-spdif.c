/*
 * sound\soc\sunxi\sunxi-spdif.c
 * (C) Copyright 2019-2025
 * allwinnertech Technology Co., Ltd. <www.allwinnertech.com>
 * yumingfeng <yumingfeng@allwinnertech.com>
 * luguofang <luguofang@allwinnertech.com>
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
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>
#include <asm/dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "sunxi-spdif.h"
#include "sunxi-pcm.h"

#define	DRV_NAME	"sunxi-spdif"

struct sample_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

/* Origin freq convert */
static const struct sample_rate sample_rate_orig[] = {
	{22050,  0xB},
	{24000,  0x9},
	{32000,  0xC},
	{44100,  0xF},
	{48000,  0xD},
	{88200,  0x7},
	{96000,  0x5},
	{176400, 0x3},
	{192000, 0x1},
};

static const struct sample_rate sample_rate_freq[] = {
	{22050,  0x4},
	{24000,  0x6},
	{32000,  0x3},
	{44100,  0x0},
	{48000,  0x2},
	{88200,  0x8},
	{96000,  0xA},
	{176400, 0xC},
	{192000, 0xE},
};

#ifdef SUNXI_SPDIF_DEBUG
static struct sunxi_spdif_reg_label reg_labels[] = {
	SPDIF_REG_LABEL(SUNXI_SPDIF_CTL),
	SPDIF_REG_LABEL(SUNXI_SPDIF_TXCFG),
	SPDIF_REG_LABEL(SUNXI_SPDIF_RXCFG),
	SPDIF_REG_LABEL(SUNXI_SPDIF_INT_STA),
	SPDIF_REG_LABEL(SUNXI_SPDIF_FIFO_CTL),
	SPDIF_REG_LABEL(SUNXI_SPDIF_FIFO_STA),
	SPDIF_REG_LABEL(SUNXI_SPDIF_INT),
	SPDIF_REG_LABEL(SUNXI_SPDIF_TXCNT),
	SPDIF_REG_LABEL(SUNXI_SPDIF_RXCNT),
	SPDIF_REG_LABEL(SUNXI_SPDIF_TXCH_STA0),
	SPDIF_REG_LABEL(SUNXI_SPDIF_TXCH_STA1),
	SPDIF_REG_LABEL(SUNXI_SPDIF_RXCH_STA0),
	SPDIF_REG_LABEL(SUNXI_SPDIF_RXCH_STA1),
	SPDIF_REG_LABEL_END,
};

static ssize_t show_spdif_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(dev);
	struct sunxi_spdif_mem_info *mem_info = NULL;
	int count = 0;
	unsigned int reg_val;
	int ret = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(sunxi_spdif)) {
		dev_err(dev, "sunxi_spdif is NULL!\n");
		return count;
	}
	mem_info = &sunxi_spdif->mem_info;

	count = snprintf(buf, PAGE_SIZE, "Dump spdif reg:\n");
	if (count > 0) {
		ret += count;
	} else {
		dev_err(dev, "snprintf start error=%d.\n", count);
		return 0;
	}

	while (reg_labels[i].name != NULL) {
		regmap_read(mem_info->regmap, reg_labels[i].value, &reg_val);
		count = snprintf(buf + ret, PAGE_SIZE - ret,
			"%-23s[0x%02X]: 0x%08X\n",
			reg_labels[i].name,
			(reg_labels[i].value), reg_val);
		if (count > 0) {
			ret += count;
		} else {
			dev_err(dev, "snprintf [i=%d] error=%d.\n", i, count);
			break;
		}
		if (ret > PAGE_SIZE) {
			ret = PAGE_SIZE;
			break;
		}
		i++;
	}

	return ret;
}

/* ex:
 *param 1: 0 read;1 write
 *param 2: reg value;
 *param 3: write value;
	read:
		echo 0,0x0 > spdif_reg
	write:
		echo 1,0x00,0xa > spdif_reg
*/
static ssize_t store_spdif_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	unsigned int input_reg_val = 0;
	unsigned int input_reg_offset = 0;
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(dev);
	struct sunxi_spdif_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_spdif)) {
		dev_err(dev, "sunxi_spdif is NULL!\n");
		return count;
	}
	mem_info = &sunxi_spdif->mem_info;

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag, &input_reg_offset,
			&input_reg_val);

	if (ret == 3 || ret == 2) {
		if (!(rw_flag == 1 || rw_flag == 0)) {
			dev_err(dev, "rw_flag should be 0(read) or 1(write).\n");
			return count;
		}
		if (input_reg_offset > SUNXI_SPDIF_REG_MAX) {
			pr_err("the reg offset is invalid! [0x0 - 0x%x]\n",
				SUNXI_SPDIF_REG_MAX);
			return count;
		}
		if (rw_flag) {
			regmap_write(mem_info->regmap, input_reg_offset,
					input_reg_val);
		}
		regmap_read(mem_info->regmap, input_reg_offset, &reg_val_read);
		pr_err("\n\n Reg[0x%x] : 0x%x\n\n", input_reg_offset, reg_val_read);
	} else {
		pr_err("ret:%d, The num of params invalid!\n", ret);
		pr_err("\nExample(reg range: 0x0 - 0x%x):\n", SUNXI_SPDIF_REG_MAX);
		pr_err("\nRead reg[0x04]:\n");
		pr_err("      echo 0,0x04 > spdif_reg\n");
		pr_err("Write reg[0x04]=0x10\n");
		pr_err("      echo 1,0x04,0x10 > spdif_reg\n");
	}

	return count;
}

static DEVICE_ATTR(spdif_reg, 0644, show_spdif_reg, store_spdif_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_spdif_reg.attr,
	NULL,
};

static struct attribute_group spdif_debug_attr_group = {
	.name	= "spdif_debug",
	.attrs	= audio_debug_attrs,
};
#endif

static int sunxi_spdif_set_audio_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int reg_val;

	regmap_read(mem_info->regmap, SUNXI_SPDIF_TXCH_STA0, &reg_val);

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		reg_val = 0;
		break;
	case 1:
		reg_val = 1;
		break;
	default:
		return -EINVAL;
	}

	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
			(1 << TXCFG_CHAN_STA_EN), (reg_val << TXCFG_CHAN_STA_EN));
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
			(1 << TXCFG_DATA_TYPE), (reg_val << TXCFG_DATA_TYPE));
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCH_STA0,
			(1 << TXCHSTA0_AUDIO), (reg_val << TXCHSTA0_AUDIO));
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_RXCH_STA0,
			(1 << RXCHSTA0_AUDIO), (reg_val << RXCHSTA0_AUDIO));

	return 0;
}

static int sunxi_spdif_get_audio_mode(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int reg_val;

	regmap_read(mem_info->regmap, SUNXI_SPDIF_TXCFG, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> TXCFG_DATA_TYPE) & 0x1;

	return 0;
}

static const char *spdif_format_function[] = {"PCM", "DTS"};
static const struct soc_enum spdif_format_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spdif_format_function),
			spdif_format_function),
};

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
static int sunxi_spdif_set_rx_data_type(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int reg_val;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		reg_val = 0;
		break;
	case 1:
		reg_val = 1;
		break;
	default:
		return -EINVAL;
	}

	/* Spdif Rx use the non-auto mode */
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_EXP_CTL,
			(1 << RX_MODE), (0 << RX_MODE));
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_EXP_CTL,
			(1 << RX_MODE_MAN), (reg_val << RX_MODE_MAN));

	return 0;
}

static int sunxi_spdif_get_rx_data_type(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int reg_val;

	regmap_read(mem_info->regmap, SUNXI_SPDIF_EXP_CTL, &reg_val);
	ucontrol->value.integer.value[0] = (reg_val >> RX_MODE_MAN) & 0x1;

	return 0;
}

static const char *spdif_rx_data_type[] = {"IEC-60958", "IEC-61937"};
static const struct soc_enum spdif_rx_data_type_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spdif_rx_data_type),
			spdif_rx_data_type),
};
#endif

static int sunxi_spdif_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int reg_val;

	regmap_read(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL, &reg_val);

	ucontrol->value.integer.value[0] =
			((reg_val & (1 << FIFO_CTL_HUBEN)) ? 1 : 0);
	return 0;
}

static int sunxi_spdif_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_component_get_drvdata(component);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;

	switch (ucontrol->value.integer.value[0]) {
	case 0:
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
				(1 << FIFO_CTL_HUBEN), (0 << FIFO_CTL_HUBEN));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
				(1 << TXCFG_TXEN), (0 << TXCFG_TXEN));
		break;
	case 1:
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
				(1 << FIFO_CTL_HUBEN), (1 << FIFO_CTL_HUBEN));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
				(1 << TXCFG_TXEN), (1 << TXCFG_TXEN));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* sunxi spdif hub mdoe select */
static const char *spdif_hub_function[] = {"Disable", "Enable"};

static const struct soc_enum spdif_hub_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(spdif_hub_function),
			spdif_hub_function),
};

/* dts pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_spdif_controls[] = {
	SOC_ENUM_EXT("spdif audio format function", spdif_format_enum[0],
			sunxi_spdif_get_audio_mode, sunxi_spdif_set_audio_mode),
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	SOC_ENUM_EXT("spdif rx data type", spdif_rx_data_type_enum[0],
			sunxi_spdif_get_rx_data_type, sunxi_spdif_set_rx_data_type),
#endif
	SOC_ENUM_EXT("sunxi spdif hub mode", spdif_hub_mode_enum[0],
			sunxi_spdif_get_hub_mode, sunxi_spdif_set_hub_mode),
	SOC_SINGLE("sunxi spdif loopback debug", SUNXI_SPDIF_CTL,
			CTL_LOOP_EN, 1, 0),
};

static void sunxi_spdif_txctrl_enable(struct sunxi_spdif_info *sunxi_spdif,
				int enable)
{
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;

	if (enable) {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
				(1 << TXCFG_TXEN), (1 << TXCFG_TXEN));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_INT,
				(1 << INT_TXDRQEN), (1 << INT_TXDRQEN));
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
				(1 << TXCFG_TXEN), (0 << TXCFG_TXEN));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_INT,
				(1 << INT_TXDRQEN), (0 << INT_TXDRQEN));
	}
}

static void sunxi_spdif_rxctrl_enable(struct sunxi_spdif_info *sunxi_spdif,
				int enable)
{
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;

	if (enable) {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_RXCFG,
				(1 << RXCFG_CHSR_CP), (1 << RXCFG_CHSR_CP));

		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_INT,
				(1 << INT_RXDRQEN), (1 << INT_RXDRQEN));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_RXCFG,
				(1 << RXCFG_RXEN), (1 << RXCFG_RXEN));
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_RXCFG,
				(1 << RXCFG_RXEN), (0 << RXCFG_RXEN));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_INT,
				(1 << INT_RXDRQEN), (0 << INT_RXDRQEN));
	}
}

static void sunxi_spdif_init(struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;

	/* FIFO CTL register default setting */
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
			(CTL_TXTL_MASK << FIFO_CTL_TXTL),
			(CTL_TXTL_DEFAULT << FIFO_CTL_TXTL));
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
			(CTL_RXTL_MASK << FIFO_CTL_RXTL),
			(CTL_RXTL_DEFAULT << FIFO_CTL_RXTL));

	regmap_write(mem_info->regmap, SUNXI_SPDIF_TXCH_STA0,
			0x2 << TXCHSTA0_CHNUM);
	regmap_write(mem_info->regmap, SUNXI_SPDIF_RXCH_STA0,
			0x2 << RXCHSTA0_CHNUM);
}

static int sunxi_spdif_dai_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int tx_input_mode = 0;
	unsigned int rx_output_mode = 0;
	unsigned int origin_freq_bit = 0, sample_freq_bit = 0;
	unsigned int reg_temp;
	unsigned int i;

	/* two substream should be warking on same samplerate */
	mutex_lock(&sunxi_spdif->mutex);
	if (sunxi_spdif->active > 1) {
		if (params_rate(params) != sunxi_spdif->rate) {
			dev_err(sunxi_spdif->dev, "params_rate[%d] != %d\n",
				params_rate(params), sunxi_spdif->rate);
			mutex_unlock(&sunxi_spdif->mutex);
			return -EINVAL;
		}
	}
	mutex_unlock(&sunxi_spdif->mutex);

	switch (params_format(params)) {
	case	SNDRV_PCM_FORMAT_S16_LE:
		reg_temp = 0;
		tx_input_mode = 1;
		rx_output_mode = 3;
		break;
	case	SNDRV_PCM_FORMAT_S20_3LE:
		reg_temp = 1;
		tx_input_mode = 0;
		rx_output_mode = 0;
		break;
	case	SNDRV_PCM_FORMAT_S32_LE:
		/* only for the compatible of tinyalsa */
	case	SNDRV_PCM_FORMAT_S24_LE:
		reg_temp = 2;
		tx_input_mode = 0;
		rx_output_mode = 0;
		break;
	default:
		dev_err(sunxi_spdif->dev, "params_format[%d] error!\n",
			params_format(params));
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_orig); i++) {
		if (params_rate(params) == sample_rate_orig[i].samplerate) {
			origin_freq_bit = sample_rate_orig[i].rate_bit;
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(sample_rate_freq); i++) {
		if (params_rate(params) == sample_rate_freq[i].samplerate) {
			sample_freq_bit = sample_rate_freq[i].rate_bit;
			sunxi_spdif->rate = sample_rate_freq[i].samplerate;
			break;
		}
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCFG,
			(3<<TXCFG_SAMPLE_BIT), (reg_temp<<TXCFG_SAMPLE_BIT));

		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
					(1<<FIFO_CTL_TXIM),
					(tx_input_mode << FIFO_CTL_TXIM));

		if (params_channels(params) == 1) {
			regmap_update_bits(mem_info->regmap,
				SUNXI_SPDIF_TXCFG, (1 << TXCFG_SINGLE_MOD),
				(1 << TXCFG_SINGLE_MOD));
		} else {
			regmap_update_bits(mem_info->regmap,
				SUNXI_SPDIF_TXCFG, (1 << TXCFG_SINGLE_MOD),
				(0 << TXCFG_SINGLE_MOD));
		}

		/* samplerate conversion */
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCH_STA0,
				(0xF << TXCHSTA0_SAMFREQ),
				(sample_freq_bit << TXCHSTA0_SAMFREQ));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_TXCH_STA1,
				(0xF << TXCHSTA1_ORISAMFREQ),
				(origin_freq_bit << TXCHSTA1_ORISAMFREQ));
		switch (reg_temp) {
		case	0:
			regmap_update_bits(mem_info->regmap,
				SUNXI_SPDIF_TXCH_STA1,
				(0xF << TXCHSTA1_MAXWORDLEN),
				(2 << TXCHSTA1_MAXWORDLEN));
			break;
		case	1:
			regmap_update_bits(mem_info->regmap,
				SUNXI_SPDIF_TXCH_STA1,
				(0xF << TXCHSTA1_MAXWORDLEN),
				(0xC << TXCHSTA1_MAXWORDLEN));
			break;
		case	2:
			regmap_update_bits(mem_info->regmap,
					SUNXI_SPDIF_TXCH_STA1,
					(0xF << TXCHSTA1_MAXWORDLEN),
					(0xB << TXCHSTA1_MAXWORDLEN));
			break;
		default:
			dev_err(sunxi_spdif->dev, "unexpection error\n");
			return -EINVAL;
		}
	} else {
		/*
		 * FIXME, not sync as spec says, just test 16bit & 24bit,
		 * using 3 working ok
		 */
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
				(0x3 << FIFO_CTL_RXOM),
				(rx_output_mode << FIFO_CTL_RXOM));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_RXCH_STA0,
				(0xF<<RXCHSTA0_SAMFREQ),
				(sample_freq_bit << RXCHSTA0_SAMFREQ));
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_RXCH_STA1,
				(0xF<<RXCHSTA1_ORISAMFREQ),
				(origin_freq_bit << RXCHSTA1_ORISAMFREQ));

		switch (reg_temp) {
		case	0:
			regmap_update_bits(mem_info->regmap,
					SUNXI_SPDIF_RXCH_STA1,
					(0xF << RXCHSTA1_MAXWORDLEN),
					(2 << RXCHSTA1_MAXWORDLEN));
			break;
		case	1:
			regmap_update_bits(mem_info->regmap,
					SUNXI_SPDIF_RXCH_STA1,
					(0xF << RXCHSTA1_MAXWORDLEN),
					(0xC << RXCHSTA1_MAXWORDLEN));
			break;
		case	2:
			regmap_update_bits(mem_info->regmap,
					SUNXI_SPDIF_RXCH_STA1,
					(0xF << RXCHSTA1_MAXWORDLEN),
					(0xB << RXCHSTA1_MAXWORDLEN));
			break;
		default:
			dev_err(sunxi_spdif->dev, "unexpection error\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int sunxi_spdif_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
						unsigned int freq, int dir)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_clk_info *clk_info = &sunxi_spdif->clk_info;

	mutex_lock(&sunxi_spdif->mutex);
	if (sunxi_spdif->active == 0) {
		dev_info(sunxi_spdif->dev, "active: %u\n", sunxi_spdif->active);
		if (clk_set_rate(clk_info->clk_pll, freq)) {
			dev_err(sunxi_spdif->dev,
				"clk pll set rate to %uHz failed\n", freq);
			mutex_unlock(&sunxi_spdif->mutex);
			return -EBUSY;
		}
		switch (clk_info->clk_parent) {
		case SPDIF_CLK_PLL_AUDIO_X1:
			if (clk_set_rate(clk_info->clk_pll, freq)) {
				dev_err(sunxi_spdif->dev,
					"clk pll set rate to %uHz failed\n", freq);
				mutex_unlock(&sunxi_spdif->mutex);
				return -EBUSY;
			}

			if (clk_set_rate(clk_info->clk_module, freq)) {
				dev_err(sunxi_spdif->dev,
					"clk module set rate to %uHz failed\n", freq);
				mutex_unlock(&sunxi_spdif->mutex);
				return -EBUSY;
			}
			break;
		default:
		case SPDIF_CLK_PLL_AUDIO_X4:
			if (clk_set_rate(clk_info->clk_pll, freq * 4)) {
				dev_err(sunxi_spdif->dev,
					"clk module set rate to %uHz failed\n", freq * 4);
				mutex_unlock(&sunxi_spdif->mutex);
				return -EBUSY;
			}

			if (clk_set_rate(clk_info->clk_module, freq)) {
				dev_err(sunxi_spdif->dev,
					"clk module set rate to %uHz failed\n", freq);
				mutex_unlock(&sunxi_spdif->mutex);
				return -EBUSY;
			}
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
			if (clk_set_parent(clk_info->clk_module_rx, clk_info->clk_pll1_div)) {
				pr_err("set parent of clk_module to clk_pll failed!\n");
				return -EBUSY;
			}

			if (clk_set_rate(clk_info->clk_pll1, 1000000000)) {
				pr_err("spdif: spdif rx source set pll1 rate failed\n");
				return -EBUSY;
			}
#endif
			break;
		}
	}
	sunxi_spdif->active++;
	mutex_unlock(&sunxi_spdif->mutex);

	return 0;
}

static int sunxi_spdif_dai_set_clkdiv(struct snd_soc_dai *dai,
					int clk_id, int clk_div)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;

	mutex_lock(&sunxi_spdif->mutex);
	if (sunxi_spdif->configured == false) {
		switch (clk_id) {
		case 0:
			clk_div = clk_div >> 7;//fs = PLL/[(div+1)*64*2]
			regmap_update_bits(mem_info->regmap,
				SUNXI_SPDIF_TXCFG,
				(0x1F << TXCFG_CLK_DIV_RATIO),
				((clk_div - 1) << TXCFG_CLK_DIV_RATIO));
			break;
		default:
			break;
		}
	}
	sunxi_spdif->configured = true;
	mutex_unlock(&sunxi_spdif->mutex);

	return 0;
}

static int sunxi_spdif_dai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream,
				&sunxi_spdif->playback_dma_param);
	} else {
		snd_soc_dai_set_dma_data(dai, substream,
				&sunxi_spdif->capture_dma_param);
	}

	return 0;
}

static void sunxi_spdif_dai_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	mutex_lock(&sunxi_spdif->mutex);
	if (sunxi_spdif->active) {
		sunxi_spdif->active--;
		if (sunxi_spdif->active == 0)
			sunxi_spdif->configured = false;
	}
	mutex_unlock(&sunxi_spdif->mutex);
}

static int sunxi_spdif_trigger(struct snd_pcm_substream *substream,
					int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_pinctl_info *pin_info = &sunxi_spdif->pin_info;
	int ret = 0;

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (pin_info->gpio_cfg.used) {
			gpio_set_value(pin_info->gpio_cfg.gpio,
					pin_info->gpio_cfg.level);
		}

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			sunxi_spdif_txctrl_enable(sunxi_spdif, 1);
		else
			sunxi_spdif_rxctrl_enable(sunxi_spdif, 1);
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			sunxi_spdif_txctrl_enable(sunxi_spdif, 0);
		else
			sunxi_spdif_rxctrl_enable(sunxi_spdif, 0);

		if (pin_info->gpio_cfg.used) {
			gpio_set_value(pin_info->gpio_cfg.gpio,
					!pin_info->gpio_cfg.level);
		}
		break;
	default:
		dev_err(sunxi_spdif->dev, "cmd error.\n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* Flush FIFO & Interrupt */
static int sunxi_spdif_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	unsigned int reg_val;

	/*as you need to clean up TX or RX FIFO , need to turn off GEN bit*/
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_CTL,
			(1 << CTL_GEN_EN), (0 << CTL_GEN_EN));

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
				(1 << FIFO_CTL_FTX), (1 << FIFO_CTL_FTX));
		regmap_write(mem_info->regmap, SUNXI_SPDIF_TXCNT, 0);
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_FIFO_CTL,
				(1 << FIFO_CTL_FRX), (1 << FIFO_CTL_FRX));
		regmap_write(mem_info->regmap, SUNXI_SPDIF_RXCNT, 0);
	}

	/* clear all interrupt status */
	regmap_read(mem_info->regmap, SUNXI_SPDIF_INT_STA, &reg_val);
	regmap_write(mem_info->regmap, SUNXI_SPDIF_INT_STA, reg_val);

	/* need reset */
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_CTL,
			(1 << CTL_RESET) | (1 << CTL_GEN_EN),
			(1 << CTL_RESET) | (1 << CTL_GEN_EN));

	return 0;
}

static int sunxi_spdif_probe(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;

	mutex_init(&sunxi_spdif->mutex);

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, &sunxi_spdif->playback_dma_param,
				&sunxi_spdif->capture_dma_param);

	sunxi_spdif_init(sunxi_spdif);
	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_CTL,
			(1 << CTL_GEN_EN), (1 << CTL_GEN_EN));

	return 0;
}

static int sunxi_spdif_remove(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);

	mutex_destroy(&sunxi_spdif->mutex);

	return 0;
}

static int sunxi_spdif_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	struct sunxi_spdif_clk_info *clk_info = &sunxi_spdif->clk_info;
	struct sunxi_spdif_pinctl_info *pin_info = &sunxi_spdif->pin_info;

	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_CTL,
				(1 << CTL_GEN_EN), (0 << CTL_GEN_EN));

	pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate_sleep);

	clk_disable_unprepare(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	clk_disable_unprepare(clk_info->clk_module_rx);
	clk_disable_unprepare(clk_info->clk_pll1_div);
	clk_disable_unprepare(clk_info->clk_pll1);
#endif
	clk_disable_unprepare(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	return 0;
}

static int sunxi_spdif_resume(struct snd_soc_dai *dai)
{
	struct sunxi_spdif_info *sunxi_spdif = snd_soc_dai_get_drvdata(dai);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	struct sunxi_spdif_clk_info *clk_info = &sunxi_spdif->clk_info;
	struct sunxi_spdif_pinctl_info *pin_info = &sunxi_spdif->pin_info;

	reset_control_deassert(clk_info->clk_rst);
	clk_prepare_enable(clk_info->clk_bus);
	clk_prepare_enable(clk_info->clk_pll);
	clk_prepare_enable(clk_info->clk_module);
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	clk_prepare_enable(clk_info->clk_pll1);
	clk_prepare_enable(clk_info->clk_pll1_div);
	clk_prepare_enable(clk_info->clk_module_rx);
#endif

	pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);

	sunxi_spdif_init(sunxi_spdif);

	regmap_update_bits(mem_info->regmap, SUNXI_SPDIF_CTL,
				(1 << CTL_GEN_EN), (1 << CTL_GEN_EN));

	return 0;
}

static struct snd_soc_dai_ops sunxi_spdif_dai_ops = {
	.hw_params	= sunxi_spdif_dai_hw_params,
	.set_clkdiv	= sunxi_spdif_dai_set_clkdiv,
	.set_sysclk	= sunxi_spdif_dai_set_sysclk,
	.startup	= sunxi_spdif_dai_startup,
	.shutdown	= sunxi_spdif_dai_shutdown,
	.trigger	= sunxi_spdif_trigger,
	.prepare	= sunxi_spdif_prepare,
};

static struct snd_soc_dai_driver sunxi_spdif_dai = {
	.probe		= sunxi_spdif_probe,
	.suspend	= sunxi_spdif_suspend,
	.resume		= sunxi_spdif_resume,
	.remove		= sunxi_spdif_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUNXI_SPDIF_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = SUNXI_SPDIF_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_spdif_dai_ops,
};

/*****************************************************************************/
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
static int sunxi_spdif_get_params_info(struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	struct sunxi_spdif_rx_info *rx_info = &sunxi_spdif->rx_info;
	int reg_val;

	regmap_read(mem_info->regmap, SUNXI_SPDIF_RXCH_STA0, &reg_val);
	rx_info->rx_params.freq = (reg_val & (0xF << RXCHSTA0_SAMFREQ));
	rx_info->rx_params.channels = (reg_val & (0xF << RXCHSTA0_CHNUM));

	regmap_read(mem_info->regmap, SUNXI_SPDIF_RXCH_STA1, &reg_val);
	rx_info->rx_params.orig_freq = (reg_val & (0xF << RXCHSTA1_ORISAMFREQ));
	rx_info->rx_params.bits = (reg_val & (0x7 << RXCHSTA1_SAMWORDLEN));

	regmap_read(mem_info->regmap, SUNXI_SPDIF_EXP_INFO1, &reg_val);
	rx_info->rx_params.refreq = (reg_val & (0xFFFF << SAMPLE_RATE_VAL));

	switch (rx_info->rx_params.freq) {
	case 0:
		rx_info->rx_params.freq = 44100;
		break;
	case 2:
		rx_info->rx_params.freq = 48000;
		break;
	case 3:
		rx_info->rx_params.freq = 32000;
		break;
	case 4:
		rx_info->rx_params.freq = 22050;
		break;
	case 6:
		rx_info->rx_params.freq = 24000;
		break;
	case 9:
		rx_info->rx_params.freq = 768000;
		break;
	case 10:
		rx_info->rx_params.freq = 96000;
		break;
	case 12:
		rx_info->rx_params.freq = 176400;
		break;
	case 14:
		rx_info->rx_params.freq = 192000;
		break;
	default:
		rx_info->rx_params.freq = 0;
	}

	switch (rx_info->rx_params.orig_freq) {
	case 1:
		rx_info->rx_params.orig_freq = 192000;
		break;
	case 2:
		rx_info->rx_params.orig_freq = 12000;
		break;
	case 3:
		rx_info->rx_params.orig_freq = 176400;
		break;
	case 5:
		rx_info->rx_params.orig_freq = 96000;
		break;
	case 6:
		rx_info->rx_params.orig_freq = 8000;
		break;
	case 7:
		rx_info->rx_params.orig_freq = 88200;
		break;
	case 8:
		rx_info->rx_params.orig_freq = 16000;
		break;
	case 9:
		rx_info->rx_params.orig_freq = 24000;
		break;
	case 10:
		rx_info->rx_params.orig_freq = 11250;
		break;
	case 11:
		rx_info->rx_params.orig_freq = 22050;
		break;
	case 12:
		rx_info->rx_params.orig_freq = 32000;
		break;
	case 13:
		rx_info->rx_params.orig_freq = 48000;
		break;
	case 15:
		rx_info->rx_params.orig_freq = 44100;
		break;
	default:
		rx_info->rx_params.orig_freq = 0;
	}

#if 0
	/* 200MHZ - Sysclk */
	if (rx_info->rx_params.refreq > 9065 && rx_info->rx_params.freq < 9075) {
		rx_info->rx_params.orig_freq = 22050;
	} else if (rx_info->rx_params.refreq > 8328 && rx_info->rx_params.freq < 8338) {
		rx_info->rx_params.orig_freq = 24000;
	} else if (rx_info->rx_params.refreq > 6245 && rx_info->rx_params.freq < 6255) {
		rx_info->rx_params.orig_freq = 32000;
	} else if (rx_info->rx_params.refreq > 4530 && rx_info->rx_params.freq < 4540) {
		rx_info->rx_params.orig_freq = 44100;
	} else if (rx_info->rx_params.refreq > 4161 && rx_info->rx_params.freq < 4171) {
		rx_info->rx_params.orig_freq = 48000;
	} else if (rx_info->rx_params.refreq > 2078 && rx_info->rx_params.freq < 2088) {
		rx_info->rx_params.orig_freq = 96000;
	} else if (rx_info->rx_params.refreq > 1128 && rx_info->rx_params.freq < 1138) {
		rx_info->rx_params.orig_freq = 176400;
	} else if (rx_info->rx_params.refreq > 1036 && rx_info->rx_params.freq < 1046) {
		rx_info->rx_params.orig_freq = 192000;
	} else {
		rx_info->rx_params.orig_freq = 0;
	}
#endif
	/* 196.608MHZ - Sysclk */
	if (rx_info->rx_params.refreq > 8911 && rx_info->rx_params.freq < 8921) {
		rx_info->rx_params.orig_freq = 22050;
	} else if (rx_info->rx_params.refreq > 8187 && rx_info->rx_params.freq < 8197) {
		rx_info->rx_params.orig_freq = 24000;
	} else if (rx_info->rx_params.refreq > 6139 && rx_info->rx_params.freq < 6149) {
		rx_info->rx_params.orig_freq = 32000;
	} else if (rx_info->rx_params.refreq > 4453 && rx_info->rx_params.freq < 4463) {
		rx_info->rx_params.orig_freq = 44100;
	} else if (rx_info->rx_params.refreq > 4091 && rx_info->rx_params.freq < 4101) {
		rx_info->rx_params.orig_freq = 48000;
	} else if (rx_info->rx_params.refreq > 2043 && rx_info->rx_params.freq < 2053) {
		rx_info->rx_params.orig_freq = 96000;
	} else if (rx_info->rx_params.refreq > 1109 && rx_info->rx_params.freq < 1119) {
		rx_info->rx_params.orig_freq = 176400;
	} else if (rx_info->rx_params.refreq > 1019 && rx_info->rx_params.freq < 1029) {
		rx_info->rx_params.orig_freq = 192000;
	} else {
		rx_info->rx_params.orig_freq = 0;
	}

	return 0;
};

static ssize_t spdif_rx_show_bits(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(dev);
	struct sunxi_spdif_rx_info *rx_info = &sunxi_spdif->rx_info;
	int count = 0;

	sunxi_spdif_get_params_info(sunxi_spdif);
	count += sprintf(buf, "%d\n", rx_info->rx_params.bits);

	return count;
}

static ssize_t spdif_rx_show_orig_freq(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(dev);
	struct sunxi_spdif_rx_info *rx_info = &sunxi_spdif->rx_info;
	int count = 0;

	sunxi_spdif_get_params_info(sunxi_spdif);
	count += sprintf(buf, "%d\n", rx_info->rx_params.orig_freq);

	return count;
}

static ssize_t spdif_rx_show_channels(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(dev);
	struct sunxi_spdif_rx_info *rx_info = &sunxi_spdif->rx_info;
	int count = 0;

	sunxi_spdif_get_params_info(sunxi_spdif);
	count += sprintf(buf, "%d\n", (rx_info->rx_params.channels + 1));

	return count;
}

static ssize_t spdif_rx_show_freq(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(dev);
	struct sunxi_spdif_rx_info *rx_info = &sunxi_spdif->rx_info;
	int count = 0;

	sunxi_spdif_get_params_info(sunxi_spdif);
	count += sprintf(buf, "%d\n", rx_info->rx_params.freq);

	return count;
}

static DEVICE_ATTR(spdif_rx_bits, 0444, spdif_rx_show_bits, NULL);
static DEVICE_ATTR(spdif_rx_orig_freq, 0444, spdif_rx_show_orig_freq, NULL);
static DEVICE_ATTR(spdif_rx_channels, 0444, spdif_rx_show_channels, NULL);
static DEVICE_ATTR(spdif_rx_freq, 0444, spdif_rx_show_freq, NULL);
#endif

static int sunxi_spdif_component_probe(struct snd_soc_component *component)
{
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	/* To get the SPDIF RX Params */
	device_create_file(component->dev, &dev_attr_spdif_rx_freq);
	device_create_file(component->dev, &dev_attr_spdif_rx_channels);
	device_create_file(component->dev, &dev_attr_spdif_rx_bits);
	device_create_file(component->dev, &dev_attr_spdif_rx_orig_freq);

#endif

	return 0;
}

static const struct snd_soc_component_driver sunxi_spdif_component = {
	.name		= DRV_NAME,
	.probe		= sunxi_spdif_component_probe,
	.controls	= sunxi_spdif_controls,
	.num_controls	= ARRAY_SIZE(sunxi_spdif_controls),
};

/*****************************************************************************/

static const struct regmap_config sunxi_spdif_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_SPDIF_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_spdif_mem_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	int ret = 0;

	ret = of_address_to_resource(np, 0, &mem_info->res);
	if (ret) {
		dev_err(&pdev->dev, "Can't parse device node resource\n");
		ret = -ENODEV;
		goto err_of_get_addr_resource;
	}

	mem_info->memregion = devm_request_mem_region(&pdev->dev, mem_info->res.start,
					    resource_size(&mem_info->res), DRV_NAME);
	if (mem_info->memregion == NULL) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_mem_region;
	}

	mem_info->membase = devm_ioremap(&pdev->dev, mem_info->res.start,
						resource_size(&mem_info->res));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		dev_err(&pdev->dev, "Can't remap sunxi spdif registers\n");
		ret = -EINVAL;
		goto err_devm_ioremap;
	}

	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev,
			mem_info->membase, &sunxi_spdif_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		dev_err(&pdev->dev, "regmap sunxi spdif membase failed\n");
		ret = PTR_ERR(mem_info->regmap);
		goto err_devm_regmap;
	}

	return 0;

err_devm_regmap:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
err_devm_mem_region:
err_of_get_addr_resource:
	devm_kfree(&pdev->dev, sunxi_spdif);
	return ret;
};

static int sunxi_spdif_clk_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_clk_info *clk_info = &sunxi_spdif->clk_info;
	int ret = 0;

	ret = of_property_read_u32(np, "clk_parent", (u32 *)&clk_info->clk_parent);
	if (ret != 0) {
		dev_warn(&pdev->dev, "clk_parent config missing or invalid.\n");
		clk_info->clk_parent = SPDIF_CLK_PLL_AUDIO_X4;
	}

	clk_info->clk_pll = of_clk_get_by_name(np, "pll_audio");
	clk_info->clk_module = of_clk_get_by_name(np, "spdif");
	clk_info->clk_bus = of_clk_get_by_name(np, "spdif_bus");
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	clk_info->clk_pll1 = of_clk_get_by_name(np, "pll_audio1");
	clk_info->clk_pll1_div = of_clk_get_by_name(np, "pll_audio1_div5");
	clk_info->clk_module_rx = of_clk_get_by_name(np, "spdif_rx");
#endif
	clk_info->clk_rst = devm_reset_control_get(&pdev->dev, NULL);

	if (clk_set_parent(clk_info->clk_module, clk_info->clk_pll)) {
		dev_err(&pdev->dev,
			"set parent of clk_module to clk_pll failed!\n");
		ret = -EINVAL;
		goto err_clk_set_parent;
	}
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	if (clk_set_parent(clk_info->clk_pll1_div, clk_info->clk_pll1)) {
		dev_err(&pdev->dev,
			"set parent of clk_module to clk_pll failed!\n");
		ret = -EINVAL;
		return ret;
	}

	if (clk_set_parent(clk_info->clk_module_rx, clk_info->clk_pll1_div)) {
		dev_err(&pdev->dev,
			"set parent of clk_module to clk_pll failed!\n");
		ret = -EINVAL;
		return ret;
	}

	if (clk_set_rate(clk_info->clk_pll1, 1000000000)) {
		pr_err("spdif: spdif rx source set pll1 rate failed\n");
		return ret;
	}
#endif

	if (reset_control_deassert(clk_info->clk_rst)) {
		pr_err("spdif: deassert the spdif reset failed\n");
		return -EINVAL;
	}
	if (clk_prepare_enable(clk_info->clk_bus)) {
		dev_err(&pdev->dev, "spdif clk bus enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_bus;
	}
	if (clk_prepare_enable(clk_info->clk_pll)) {
		dev_err(&pdev->dev, "clk_pll enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_pll;
	}
	if (clk_prepare_enable(clk_info->clk_module)) {
		dev_err(&pdev->dev, "moduleclk enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_module;
	}
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_SPDIF_RX_IEC61937)
	if (clk_prepare_enable(clk_info->clk_pll1)) {
		dev_err(&pdev->dev, "clk_pll1 enable failed\n");
		ret = -EBUSY;
		return ret;
	}
	if (clk_prepare_enable(clk_info->clk_pll1_div)) {
		dev_err(&pdev->dev, "clk_pll1_div enable failed\n");
		ret = -EBUSY;
		return ret;
	}
	if (clk_prepare_enable(clk_info->clk_module_rx)) {
		dev_err(&pdev->dev, "clk_module_rx enable failed\n");
		ret = -EBUSY;
		return ret;
	}
#endif

	return 0;

err_clk_enable_module:
	clk_disable_unprepare(clk_info->clk_pll);
err_clk_enable_pll:
err_clk_set_parent:
	clk_put(clk_info->clk_bus);
err_clk_enable_bus:
	reset_control_assert(clk_info->clk_rst);
	return ret;
};

static int sunxi_spdif_pinctrl_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_pinctl_info *pin_info = &sunxi_spdif->pin_info;
	enum of_gpio_flags config;
	int ret = 0;

	pin_info->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin_info->pinctrl)) {
		dev_err(&pdev->dev,
			"request pinctrl handle for audio failed\n");
		ret = -EINVAL;
		return ret;
	}

	pin_info->pinstate = pinctrl_lookup_state(pin_info->pinctrl,
					PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin_info->pinstate)) {
		dev_err(&pdev->dev, "lookup pin default state failed\n");
		ret = -EINVAL;
		return ret;
	}

	pin_info->pinstate_sleep = pinctrl_lookup_state(
				pin_info->pinctrl, PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin_info->pinstate_sleep)) {
		dev_err(&pdev->dev, "lookup pin sleep state failed\n");
		ret = -EINVAL;
		return ret;
	}

	ret = pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);
	if (ret) {
		dev_err(sunxi_spdif->dev, "select pin default state failed\n");
		ret = -EBUSY;
		return ret;
	}

	pin_info->gpio_cfg.gpio = of_get_named_gpio_flags(np, "gpio-spdif", 0, &config);
	if (!gpio_is_valid(pin_info->gpio_cfg.gpio)) {
		dev_err(&pdev->dev, "Not using gpio-spdif gpio from dts\n");
		pin_info->gpio_cfg.used = 0;
	} else {
		pin_info->gpio_cfg.level = of_property_read_bool(np, "gpio-level");
		dev_info(&pdev->dev, "get gpio-level = %d.\n",
					pin_info->gpio_cfg.level);

		ret = devm_gpio_request(&pdev->dev, pin_info->gpio_cfg.gpio, "SPDIF");
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request gpio-spdif gpio\n");
			goto err_devm_gpio_request;
		} else {
			pin_info->gpio_cfg.used = 1;
			gpio_direction_output(pin_info->gpio_cfg.gpio,
				!pin_info->gpio_cfg.level);
		}
	}

	return 0;

err_devm_gpio_request:
	devm_pinctrl_put(pin_info->pinctrl);
	return ret;
};

static int sunxi_spdif_dts_params_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_dts_info *dts_info = &sunxi_spdif->dts_info;
	unsigned int temp_val = 0;
	int ret = 0;

	ret = of_property_read_u32(np, "playback_cma", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "playback cma kbytes config missing or invalid.\n");
		dts_info->playback_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		dts_info->playback_cma = temp_val;
	}

	ret = of_property_read_u32(np, "capture_cma", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "capture cma kbytes config missing or invalid.\n");
		dts_info->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		dts_info->capture_cma = temp_val;
	}

	return 0;
};

static void sunxi_spdif_dma_params_init(struct resource res,
					struct sunxi_spdif_info *sunxi_spdif)
{
	struct sunxi_spdif_dts_info *dts_info = &sunxi_spdif->dts_info;

	sunxi_spdif->playback_dma_param.dma_addr = res.start + SUNXI_SPDIF_TXFIFO;
	sunxi_spdif->playback_dma_param.dst_maxburst = 8;
	sunxi_spdif->playback_dma_param.src_maxburst = 8;
	sunxi_spdif->playback_dma_param.cma_kbytes = dts_info->playback_cma;
	sunxi_spdif->playback_dma_param.fifo_size = SPDIF_TX_FIFO_SIZE;

	sunxi_spdif->capture_dma_param.dma_addr = res.start + SUNXI_SPDIF_RXFIFO;
	sunxi_spdif->capture_dma_param.src_maxburst = 8;
	sunxi_spdif->capture_dma_param.dst_maxburst = 8;
	sunxi_spdif->capture_dma_param.cma_kbytes = dts_info->capture_cma;
	sunxi_spdif->capture_dma_param.fifo_size = SPDIF_RX_FIFO_SIZE;
};

static int  sunxi_spdif_dev_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sunxi_spdif_info *sunxi_spdif = NULL;
	struct sunxi_spdif_dts_info *dts_info = NULL;
	struct sunxi_spdif_mem_info *mem_info = NULL;
	struct sunxi_spdif_clk_info *clk_info = NULL;
	struct sunxi_spdif_pinctl_info *pin_info = NULL;
	int ret;

	sunxi_spdif = devm_kzalloc(&pdev->dev,
				sizeof(struct sunxi_spdif_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_spdif)) {
		ret = -ENOMEM;
		goto err_devm_malloc_spdif;
	}

	dev_set_drvdata(&pdev->dev, sunxi_spdif);
	sunxi_spdif->dev = &pdev->dev;
	sunxi_spdif->dai = sunxi_spdif_dai;
	sunxi_spdif->dai.name = dev_name(&pdev->dev);
	dts_info = &sunxi_spdif->dts_info;
	mem_info = &sunxi_spdif->mem_info;
	clk_info = &sunxi_spdif->clk_info;
	pin_info = &sunxi_spdif->pin_info;

	/* mem init about */
	ret = sunxi_spdif_mem_init(pdev, np, sunxi_spdif);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* clk init about */
	ret = sunxi_spdif_clk_init(pdev, np, sunxi_spdif);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* pinctrl init about */
	ret = sunxi_spdif_pinctrl_init(pdev, np, sunxi_spdif);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* dts params about */
	ret = sunxi_spdif_dts_params_init(pdev, np, sunxi_spdif);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* dma params about */
	sunxi_spdif_dma_params_init(sunxi_spdif->mem_info.res, sunxi_spdif);

	/* Codec component about register */
	ret = snd_soc_register_component(&pdev->dev, &sunxi_spdif_component,
					&sunxi_spdif->dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err_snd_register_component;
	}

	/* PCM Interface about register */
	ret = asoc_dma_platform_register(&pdev->dev, 0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		ret = -ENOMEM;
		goto err_register_platform;
	}

#ifdef SUNXI_SPDIF_DEBUG
	ret  = sysfs_create_group(&pdev->dev.kobj, &spdif_debug_attr_group);
	if (ret)
		dev_err(&pdev->dev, "failed to create attr group\n");
#endif

	return 0;

err_register_platform:
	snd_soc_unregister_component(&pdev->dev);
err_snd_register_component:
	devm_kfree(&pdev->dev, sunxi_spdif);
err_devm_malloc_spdif:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_spdif_dev_remove(struct platform_device *pdev)
{
	struct sunxi_spdif_info *sunxi_spdif = dev_get_drvdata(&pdev->dev);
	struct sunxi_spdif_mem_info *mem_info = &sunxi_spdif->mem_info;
	struct sunxi_spdif_clk_info *clk_info = &sunxi_spdif->clk_info;
	struct sunxi_spdif_pinctl_info *pin_info = &sunxi_spdif->pin_info;


#ifdef SUNXI_SPDIF_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &spdif_debug_attr_group);
#endif

	snd_soc_unregister_component(&pdev->dev);

	if (gpio_is_valid(pin_info->gpio_cfg.gpio))
		devm_gpio_free(&pdev->dev, pin_info->gpio_cfg.gpio);
	devm_pinctrl_put(pin_info->pinctrl);

	clk_disable_unprepare(clk_info->clk_module);
	clk_put(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	clk_put(clk_info->clk_pll);
	clk_disable_unprepare(clk_info->clk_bus);
	clk_put(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_kfree(&pdev->dev, sunxi_spdif);

	return 0;
}

static const struct of_device_id sunxi_spdif_of_match[] = {
	{ .compatible = "allwinner,sunxi-spdif", },
	{},
};

static struct platform_driver sunxi_spdif_driver = {
	.probe = sunxi_spdif_dev_probe,
	.remove = __exit_p(sunxi_spdif_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_spdif_of_match,
	},
};

module_platform_driver(sunxi_spdif_driver);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI SPDIF ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-spdif");
