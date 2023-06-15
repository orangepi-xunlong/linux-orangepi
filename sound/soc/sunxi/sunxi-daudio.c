/* sound\soc\sunxi\sunxi-daudio.c
 * (C) Copyright 2019-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
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
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/reset.h>
#include <asm/dma.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>
#include <sound/simple_card.h>

#include "sunxi-daudio.h"

#define	DRV_NAME	"sunxi-daudio"

#ifdef SUNXI_DAUDIO_DEBUG
static struct daudio_reg_label reg_labels[] = {
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_CTL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FMT0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FMT1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_INTSTA),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FIFOCTL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_FIFOSTA),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_INTCTL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_CLKDIV),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TXCNT),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCNT),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_CHCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX0CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX1CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX2CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX3CHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX0CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX0CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX1CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX1CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX2CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX2CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX3CHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_TX3CHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHSEL),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP0),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP1),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP2),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_RXCHMAP3),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_DEBUG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_REV),
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_MCLKCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_FSOUTCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_FSIN_EXTCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_ASRCEN),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_MANCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_RATIOSTAT),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_FIFOSTAT),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_MBISTCFG),
	DAUDIO_REG_LABEL(SUNXI_DAUDIO_ASRC_MBISTSTA),
#endif
	DAUDIO_REG_LABEL_END,
};

static ssize_t show_daudio_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(dev);
	int cnt = 0, i = 0, j = 0;
	unsigned int reg_val, reg_val_tmp;
	unsigned int size = ARRAY_SIZE(reg_labels);
	char *reg_name_play = "REG NAME";
	char *reg_offset_play = "OFFSET";
	char *reg_val_play = "VALUE";

	cnt += sprintf(buf + cnt,
		       "%-30s|%-6s|%-10s"
		       "|31-28|27-24|23-20|19-16|15-12|11-08|07-04|03-00|"
		       "save_value\n",
		       reg_name_play, reg_offset_play, reg_val_play);

	while ((i < size) && (reg_labels[i].name != NULL)) {
		regmap_read(sunxi_daudio->mem_info.regmap,
			    reg_labels[i].address, &reg_val);

		cnt += sprintf(buf + cnt,
			       "%-30s|0x%4x|0x%8x",
			       reg_labels[i].name,
			       reg_labels[i].address,
			       reg_val);
		for (j = 7; j >= 0; j--) {
			reg_val_tmp = reg_val >> (j * 4);
			cnt += sprintf(buf + cnt,
				       "|%c%c%c%c ",
				       (((reg_val_tmp) & 0x08ull) ? '1' : '0'),
				       (((reg_val_tmp) & 0x04ull) ? '1' : '0'),
				       (((reg_val_tmp) & 0x02ull) ? '1' : '0'),
				       (((reg_val_tmp) & 0x01ull) ? '1' : '0')
				       );
		}
		cnt += sprintf(buf + cnt, "|0x%8x\n", reg_labels[i].value);

		i++;
	}

	return cnt;
}

/* ex:
 *param 1: 0 read;1 write
 *param 2: reg value;
 *param 3: write value;
	read:
		echo 0,0x0 > daudio_reg
	write:
		echo 1,0x00,0xa > daudio_reg
*/
static ssize_t store_daudio_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	unsigned int input_reg_val = 0;
	unsigned int input_reg_offset = 0;
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(dev);
	struct sunxi_daudio_mem_info *mem_info = NULL;
	unsigned int res_size = 0;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dev, "sunxi_daudio is NULL!\n");
		return count;
	}
	mem_info = &sunxi_daudio->mem_info;
	res_size = resource_size(&mem_info->res),

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag, &input_reg_offset,
			&input_reg_val);

	if (ret == 3 || ret == 2) {
		if (!(rw_flag == 1 || rw_flag == 0)) {
			dev_err(dev, "rw_flag should be 0(read) or 1(write).\n");
			return count;
		}
		if (input_reg_offset > res_size) {
			dev_err(dev, "the reg offset is invalid! [0x0 - 0x%x]\n",
				res_size);
			return count;
		}
		if (rw_flag) {
			regmap_write(mem_info->regmap, input_reg_offset,
					input_reg_val);
		}
		regmap_read(mem_info->regmap, input_reg_offset, &reg_val_read);
		pr_err("Reg[0x%x] : 0x%x\n", input_reg_offset, reg_val_read);
	} else {
		pr_err("ret:%d, The num of params invalid!\n", ret);
		pr_err("\nExample(reg range: 0x0 - 0x%x):\n", res_size);
		pr_err("\nRead reg[0x04]:\n");
		pr_err("      echo 0,0x04 > daudio_reg\n");
		pr_err("Write reg[0x04]=0x10\n");
		pr_err("      echo 1,0x04,0x10 > daudio_reg\n");
	}

	return count;
}

static DEVICE_ATTR(daudio_reg, 0644, show_daudio_reg, store_daudio_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_daudio_reg.attr,
	NULL,
};

static struct attribute_group daudio_debug_attr_group = {
	.name	= "daudio_debug",
	.attrs	= audio_debug_attrs,
};
#endif

static void sunxi_daudio_global_lock_irqsave(struct sunxi_daudio_info *sunxi_daudio)
{
	local_irq_save(sunxi_daudio->global_spinlock.flags);
	spin_lock(&sunxi_daudio->global_spinlock.lock);
}

static void sunxi_daudio_global_unlock_irqrestore(struct sunxi_daudio_info *sunxi_daudio)
{
	spin_unlock(&sunxi_daudio->global_spinlock.lock);
	local_irq_restore(sunxi_daudio->global_spinlock.flags);
}

static int sunxi_daudio_get_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *sunxi_daudio =
					snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	unsigned int reg_val;

	regmap_read(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL, &reg_val);
	ucontrol->value.integer.value[0] = ((reg_val & (1 << HUB_EN)) ? 1 : 0);

	return 0;
}

static int sunxi_daudio_set_hub_mode(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *sunxi_daudio =
					snd_soc_component_get_drvdata(component);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;

	switch (ucontrol->value.integer.value[0]) {
	case	0:
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1 << HUB_EN), (0 << HUB_EN));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1 << CTL_TXEN), (0 << CTL_TXEN));
		sunxi_daudio->hub_mode = 0;
		break;
	case	1:
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1 << HUB_EN), (1 << HUB_EN));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1 << CTL_TXEN), (1 << CTL_TXEN));
		sunxi_daudio->hub_mode = 1;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static const char *daudio_format_function[] = {"Disable", "Enable"};

static const struct soc_enum daudio_format_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(daudio_format_function),
			daudio_format_function),
};

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
static int sunxi_daudio_get_asrc_function(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *sunxi_daudio =
					snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = sunxi_daudio->asrc_function_en;

	return 0;
}

static int sunxi_daudio_set_asrc_function(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct sunxi_daudio_info *sunxi_daudio =
					snd_soc_component_get_drvdata(component);

	sunxi_daudio->asrc_function_en = ucontrol->value.integer.value[0];

	return 0;
}

static const char * const daudio_asrc_function[] = {"Disable", "Enable"};

static const struct soc_enum daudio_asrc_function_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(daudio_asrc_function),
			daudio_asrc_function),
};
#endif

/* dts pcm Audio Mode Select */
static const struct snd_kcontrol_new sunxi_daudio_controls[] = {
	SOC_ENUM_EXT("sunxi daudio audio hub mode", daudio_format_enum[0],
		sunxi_daudio_get_hub_mode, sunxi_daudio_set_hub_mode),
	SOC_SINGLE("sunxi daudio loopback debug", SUNXI_DAUDIO_CTL,
		LOOP_EN, 1, 0),
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	SOC_ENUM_EXT("sunxi daudio asrc function", daudio_asrc_function_enum[0],
		sunxi_daudio_get_asrc_function, sunxi_daudio_set_asrc_function),
#endif
};

static void sunxi_daudio_txctrl_enable(struct sunxi_daudio_info *sunxi_daudio,
					bool enable)
{
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	unsigned int DAUDIO_SDO_MASK = 0;
	unsigned int i = 0;

	for (i = 0; i < pdata_info->tx_num; i++)
		DAUDIO_SDO_MASK |= 0x1 << i;

	if (enable) {
		/* HDMI audio Transmit Clock just enable at startup */
		if (pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
						(DAUDIO_SDO_MASK << SDO0_EN),
						(DAUDIO_SDO_MASK << SDO0_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
						(1 << CTL_TXEN), (1 << CTL_TXEN));
		}

		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_INTCTL,
					(1 << TXDRQEN), (1 << TXDRQEN));
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_INTCTL,
					(1 << TXDRQEN), (0 << TXDRQEN));

		if (pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(DAUDIO_SDO_MASK << SDO0_EN) | (1 << CTL_TXEN),
					(0 << SDO0_EN) | (0 << CTL_TXEN));
		}
	}
}

static void sunxi_daudio_rxctrl_enable(struct sunxi_daudio_info *sunxi_daudio,
					bool enable)
{
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_platform_data_info *pdata_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		pr_err("[%s] sunxi_daudio is null.\n", __func__);
		return;
	}
	mem_info = &sunxi_daudio->mem_info;
	dts_info = &sunxi_daudio->dts_info;
	pdata_info = &dts_info->pdata_info;

	if (enable) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1 << CTL_RXEN), (1 << CTL_RXEN));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_INTCTL,
				(1 << RXDRQEN), (1 << RXDRQEN));

		if (pdata_info->daudio_rxsync_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1 << RX_EN_MUX), (1 << RX_EN_MUX));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1 << RX_SYNC_EN), (1 << RX_SYNC_EN));
		}
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_INTCTL,
				(1 << RXDRQEN), (0 << RXDRQEN));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1 << CTL_RXEN), (0 << CTL_RXEN));

		if (pdata_info->daudio_rxsync_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1 << RX_EN_MUX), (0 << RX_EN_MUX));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1 << RX_SYNC_EN), (0 << RX_SYNC_EN));
		}
	}
}

static int sunxi_daudio_global_enable(struct sunxi_daudio_info *sunxi_daudio,
					bool enable)
{
	struct sunxi_daudio_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		pr_err("[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	mem_info = &sunxi_daudio->mem_info;

	sunxi_daudio_global_lock_irqsave(sunxi_daudio);
	if (enable) {
		if (sunxi_daudio->global_enable++ == 0)
			regmap_update_bits(mem_info->regmap,
				SUNXI_DAUDIO_CTL,
				(1 << GLOBAL_EN), (1 << GLOBAL_EN));
	} else {
		if (--sunxi_daudio->global_enable <= 0) {
			sunxi_daudio->global_enable = 0;
			regmap_update_bits(mem_info->regmap,
				SUNXI_DAUDIO_CTL,
				(1 << GLOBAL_EN), (0 << GLOBAL_EN));
		}
	}
	sunxi_daudio_global_unlock_irqrestore(sunxi_daudio);

	return 0;
}

static int sunxi_daudio_mclk_setting(struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	unsigned int mclk_div;

	if (pdata_info->mclk_div) {
		switch (pdata_info->mclk_div) {
		case	1:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_1;
			break;
		case	2:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_2;
			break;
		case	4:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_3;
			break;
		case	6:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_4;
			break;
		case	8:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_5;
			break;
		case	12:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_6;
			break;
		case	16:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_7;
			break;
		case	24:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_8;
			break;
		case	32:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_9;
			break;
		case	48:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_10;
			break;
		case	64:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_11;
			break;
		case	96:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_12;
			break;
		case	128:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_13;
			break;
		case	176:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_14;
			break;
		case	192:
			mclk_div = SUNXI_DAUDIO_MCLK_DIV_15;
			break;
		default:
			dev_err(sunxi_daudio->dev, "unsupport  mclk_div\n");
			return -EINVAL;
		}

		/* setting Mclk output as external codec input clk */
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_MCLK_DIV_MASK << MCLK_DIV),
			(mclk_div << MCLK_DIV));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CLKDIV,
				(1 << MCLKOUT_EN), (1 << MCLKOUT_EN));
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CLKDIV,
				(1 << MCLKOUT_EN), (0 << MCLKOUT_EN));
	}

	return 0;
}

static int sunxi_daudio_init_fmt(struct sunxi_daudio_info *sunxi_daudio,
				unsigned int fmt)
{
	struct sunxi_daudio_mem_info *mem_info = NULL;
	unsigned int offset, mode;
	unsigned int lrck_polarity, brck_polarity;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		pr_err("[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	mem_info = &sunxi_daudio->mem_info;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case	SND_SOC_DAIFMT_CBM_CFM:
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(SUNXI_DAUDIO_LRCK_OUT_MASK<<LRCK_OUT),
				(SUNXI_DAUDIO_LRCK_OUT_DISABLE<<LRCK_OUT));
		break;
	case	SND_SOC_DAIFMT_CBS_CFS:
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(SUNXI_DAUDIO_LRCK_OUT_MASK<<LRCK_OUT),
				(SUNXI_DAUDIO_LRCK_OUT_ENABLE<<LRCK_OUT));
		break;
	default:
		dev_err(sunxi_daudio->dev, "unknown maser/slave format\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case	SND_SOC_DAIFMT_I2S:
		offset = SUNXI_DAUDIO_TX_OFFSET_1;
		mode = SUNXI_DAUDIO_MODE_CTL_I2S;
		break;
	case	SND_SOC_DAIFMT_RIGHT_J:
		offset = SUNXI_DAUDIO_TX_OFFSET_0;
		mode = SUNXI_DAUDIO_MODE_CTL_RIGHT;
		break;
	case	SND_SOC_DAIFMT_LEFT_J:
		offset = SUNXI_DAUDIO_TX_OFFSET_0;
		mode = SUNXI_DAUDIO_MODE_CTL_LEFT;
		break;
	case	SND_SOC_DAIFMT_DSP_A:
		offset = SUNXI_DAUDIO_TX_OFFSET_1;
		mode = SUNXI_DAUDIO_MODE_CTL_PCM;
		break;
	case	SND_SOC_DAIFMT_DSP_B:
		offset = SUNXI_DAUDIO_TX_OFFSET_0;
		mode = SUNXI_DAUDIO_MODE_CTL_PCM;
		break;
	default:
		dev_err(sunxi_daudio->dev, "format setting failed\n");
		return -EINVAL;
	}

	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
			(SUNXI_DAUDIO_MODE_CTL_MASK<<MODE_SEL),
			(mode<<MODE_SEL));

	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_TX0CHSEL,
			(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
			(offset<<TX_OFFSET));

	if (sunxi_daudio->hdmi_en) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_TX1CHSEL,
				(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
				(offset<<TX_OFFSET));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_TX2CHSEL,
				(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
				(offset<<TX_OFFSET));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_TX3CHSEL,
				(SUNXI_DAUDIO_TX_OFFSET_MASK<<TX_OFFSET),
				(offset<<TX_OFFSET));
	}

	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_RXCHSEL,
			(SUNXI_DAUDIO_RX_OFFSET_MASK<<RX_OFFSET),
			(offset<<RX_OFFSET));

	/* linux-4.9 kernel: SND_SOC_DAIFMT_NB_NF: 1 */
	/* linux-5.4 kernel: SND_SOC_DAIFMT_NB_NF: 0 */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case	SND_SOC_DAIFMT_NB_NF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_NOR;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_NOR;
		break;
	case	SND_SOC_DAIFMT_NB_IF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_INV;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_NOR;
		break;
	case	SND_SOC_DAIFMT_IB_NF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_NOR;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_INV;
		break;
	case	SND_SOC_DAIFMT_IB_IF:
		lrck_polarity = SUNXI_DAUDIO_LRCK_POLARITY_INV;
		brck_polarity = SUNXI_DAUDIO_BCLK_POLARITY_INV;
		break;
	default:
		dev_err(sunxi_daudio->dev, "invert clk setting failed!\n");
		return -EINVAL;
	}

	if (((fmt & SND_SOC_DAIFMT_FORMAT_MASK) == SND_SOC_DAIFMT_DSP_A) ||
		((fmt & SND_SOC_DAIFMT_FORMAT_MASK) == SND_SOC_DAIFMT_DSP_B))
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
				(1<<LRCK_POLARITY), ((lrck_polarity^1)<<LRCK_POLARITY));
	else
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
				(1<<LRCK_POLARITY), (lrck_polarity<<LRCK_POLARITY));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
			(1<<BRCK_POLARITY), (brck_polarity<<BRCK_POLARITY));

	return 0;
}

static int sunxi_daudio_init(struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;

	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
			(1 << LRCK_WIDTH),
			(pdata_info->frame_type << LRCK_WIDTH));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_LRCK_PERIOD_MASK) << LRCK_PERIOD,
			((pdata_info->pcm_lrck_period - 1) << LRCK_PERIOD));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_SLOT_WIDTH_MASK << SLOT_WIDTH),
			(((pdata_info->slot_width_select >> 2) - 1) << SLOT_WIDTH));

	/*
	 * MSB on the transmit format, always be first.
	 * default using Linear-PCM, without no companding.
	 * A-law<Eourpean standard> or U-law<US-Japan> not working ok.
	 */
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT1,
			(0x1 <<  TX_MLS),
			(pdata_info->msb_lsb_first << TX_MLS));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT1,
			(0x1 <<  RX_MLS),
			(pdata_info->msb_lsb_first << RX_MLS));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT1,
			(0x3 <<  SEXT),
			(pdata_info->sign_extend << SEXT));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT1,
			(0x3 <<  TX_PDM),
			(pdata_info->tx_data_mode << TX_PDM));
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT1,
			(0x3 <<  RX_PDM),
			(pdata_info->rx_data_mode << RX_PDM));

	return sunxi_daudio_mclk_setting(sunxi_daudio);
}

static int sunxi_daudio_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_platform_data_info *pdata_info = NULL;
	struct asoc_simple_priv *sndhdmi_priv =
				snd_soc_card_get_drvdata(dai->component->card);

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	mem_info = &sunxi_daudio->mem_info;
	dts_info = &sunxi_daudio->dts_info;
	pdata_info = &dts_info->pdata_info;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		/*
		 * Special processing for hdmi, HDMI card name is
		 * "sndhdmi" or sndhdmiraw. if card not HDMI,
		 * strstr func just return NULL, jump to right section.
		 * Not HDMI card, sunxi_hdmi maybe a NULL pointer.
		 */
		if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE &&
					(sndhdmi_priv->hdmi_format > 1)) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK<<DAUDIO_SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_24BIT<<DAUDIO_SAMPLE_RESOLUTION));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
				(SUNXI_DAUDIO_TXIM_MASK<<TXIM),
				(SUNXI_DAUDIO_TXIM_VALID_MSB<<TXIM));
		} else {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
				(SUNXI_DAUDIO_SR_MASK << DAUDIO_SAMPLE_RESOLUTION),
				(SUNXI_DAUDIO_SR_16BIT << DAUDIO_SAMPLE_RESOLUTION));
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
				regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM));
			} else {
				regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK << RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH << RXOM));
			}
		}
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_SR_MASK << DAUDIO_SAMPLE_RESOLUTION),
			(SUNXI_DAUDIO_SR_24BIT << DAUDIO_SAMPLE_RESOLUTION));

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM));
		} else {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK << RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH << RXOM));
		}
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FMT0,
			(SUNXI_DAUDIO_SR_MASK << DAUDIO_SAMPLE_RESOLUTION),
			(SUNXI_DAUDIO_SR_32BIT << DAUDIO_SAMPLE_RESOLUTION));

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_TXIM_MASK << TXIM),
					(SUNXI_DAUDIO_TXIM_VALID_LSB << TXIM));
		} else {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
					(SUNXI_DAUDIO_RXOM_MASK << RXOM),
					(SUNXI_DAUDIO_RXOM_EXPH << RXOM));
		}
		break;
	default:
		dev_err(sunxi_daudio->dev, "unrecognized format\n");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		unsigned int SUNXI_DAUDIO_TXCHMAP0 = 0;
		unsigned int SUNXI_DAUDIO_TXCHMAP1 = 0;
		unsigned int SUNXI_DAUDIO_TXCHSEL = 0;
		int index = 0;

		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CHCFG,
				(SUNXI_DAUDIO_TX_SLOT_MASK << TX_SLOT_NUM),
				((params_channels(params)-1) << TX_SLOT_NUM));

		if (sunxi_daudio->hdmi_en == 0) {

		for (index = 0; index < pdata_info->tx_num; index++) {
			switch (index) {
			case 0:
				SUNXI_DAUDIO_TXCHMAP0 = SUNXI_DAUDIO_TX0CHMAP0;
				SUNXI_DAUDIO_TXCHMAP1 = SUNXI_DAUDIO_TX0CHMAP0;
				SUNXI_DAUDIO_TXCHSEL = SUNXI_DAUDIO_TX0CHSEL;
			break;
			case 1:
				SUNXI_DAUDIO_TXCHMAP0 = SUNXI_DAUDIO_TX1CHMAP0;
				SUNXI_DAUDIO_TXCHMAP1 = SUNXI_DAUDIO_TX1CHMAP1;
				SUNXI_DAUDIO_TXCHSEL = SUNXI_DAUDIO_TX1CHSEL;
			break;
			case 2:
				SUNXI_DAUDIO_TXCHMAP0 = SUNXI_DAUDIO_TX2CHMAP0;
				SUNXI_DAUDIO_TXCHMAP1 = SUNXI_DAUDIO_TX2CHMAP1;
				SUNXI_DAUDIO_TXCHSEL = SUNXI_DAUDIO_TX2CHSEL;
			break;
			case 3:
				SUNXI_DAUDIO_TXCHMAP0 = SUNXI_DAUDIO_TX3CHMAP0;
				SUNXI_DAUDIO_TXCHMAP1 = SUNXI_DAUDIO_TX3CHMAP1;
				SUNXI_DAUDIO_TXCHSEL = SUNXI_DAUDIO_TX3CHSEL;
			break;
			default:
				dev_err(sunxi_daudio->dev, "tx_num %d error.\n",
						pdata_info->tx_num);
			break;
			}
			regmap_write(mem_info->regmap,
				SUNXI_DAUDIO_TXCHMAP0, pdata_info->tx_chmap0);
			regmap_write(mem_info->regmap,
				SUNXI_DAUDIO_TXCHMAP1, pdata_info->tx_chmap1);
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_TXCHSEL,
				(SUNXI_DAUDIO_TX_CHSEL_MASK << TX_CHSEL),
				((params_channels(params)-1) << TX_CHSEL));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_TXCHSEL,
				(SUNXI_DAUDIO_TX_CHEN_MASK << TX_CHEN),
				((1<<params_channels(params))-1) << TX_CHEN);
		}

		} else {
		/* HDMI multi-channel processing */
			regmap_write(mem_info->regmap,
					SUNXI_DAUDIO_TX0CHMAP1, 0x10);
			if (sndhdmi_priv->hdmi_format > 1) {
				regmap_write(mem_info->regmap,
						SUNXI_DAUDIO_TX1CHMAP1, 0x32);
				regmap_write(mem_info->regmap,
						SUNXI_DAUDIO_TX2CHMAP1, 0x54);
				regmap_write(mem_info->regmap,
						SUNXI_DAUDIO_TX3CHMAP1, 0x76);
			} else {
				if (params_channels(params) > 2)
					regmap_write(mem_info->regmap,
						SUNXI_DAUDIO_TX1CHMAP1, 0x23);
				if (params_channels(params) > 4) {
					if (params_channels(params) == 6)
						regmap_write(
							mem_info->regmap,
							SUNXI_DAUDIO_TX2CHMAP1,
							0x54);
					else
						regmap_write(
							mem_info->regmap,
							SUNXI_DAUDIO_TX2CHMAP1,
							0x76);
				}
				if (params_channels(params) > 6)
					regmap_write(mem_info->regmap,
							SUNXI_DAUDIO_TX3CHMAP1,
							0x54);
			}

			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX0CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX0CHSEL,
					0x03 << TX_CHEN, 0x03 << TX_CHEN);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX1CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX1CHSEL,
					(0x03)<<TX_CHEN, 0x03 << TX_CHEN);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX2CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX2CHSEL,
					(0x03)<<TX_CHEN, 0x03 << TX_CHEN);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX3CHSEL,
					0x01 << TX_CHSEL, 0x01 << TX_CHSEL);
			regmap_update_bits(mem_info->regmap,
					SUNXI_DAUDIO_TX3CHSEL,
					(0x03)<<TX_CHEN, 0x03 << TX_CHEN);
/* HDMI */
		}
	} else {
		unsigned int SUNXI_DAUDIO_RXCHMAPX = 0;
		unsigned int SUNXI_DAUDIO_RXCHMAP_VAL = 0;
		int index = 0;

		for (index = 0; index < pdata_info->rx_num; index++) {
			switch (index) {
			case 0:
				SUNXI_DAUDIO_RXCHMAPX = SUNXI_DAUDIO_RXCHMAP0;
				SUNXI_DAUDIO_RXCHMAP_VAL = pdata_info->rx_chmap0;
			break;
			case 1:
				SUNXI_DAUDIO_RXCHMAPX = SUNXI_DAUDIO_RXCHMAP1;
				SUNXI_DAUDIO_RXCHMAP_VAL = pdata_info->rx_chmap1;
			break;
			case 2:
				SUNXI_DAUDIO_RXCHMAPX = SUNXI_DAUDIO_RXCHMAP2;
				SUNXI_DAUDIO_RXCHMAP_VAL = pdata_info->rx_chmap2;
			break;
			case 3:
				SUNXI_DAUDIO_RXCHMAPX = SUNXI_DAUDIO_RXCHMAP3;
				SUNXI_DAUDIO_RXCHMAP_VAL = pdata_info->rx_chmap3;
			break;
			default:
				dev_err(sunxi_daudio->dev, "rx_num %d error.\n",
						pdata_info->rx_num);
			break;
			}
			regmap_write(mem_info->regmap, SUNXI_DAUDIO_RXCHMAPX,
					SUNXI_DAUDIO_RXCHMAP_VAL);
		}
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CHCFG,
				(SUNXI_DAUDIO_RX_SLOT_MASK << RX_SLOT_NUM),
				((params_channels(params)-1) << RX_SLOT_NUM));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_RXCHSEL,
				(SUNXI_DAUDIO_RX_CHSEL_MASK << RX_CHSEL),
				((params_channels(params)-1) << RX_CHSEL));
	}

	/* Special processing for HDMI hub playback to enable hdmi module */
	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
		if (sunxi_daudio->hub_mode) {
			sunxi_hdmi_codec_hw_params(substream, params, NULL);
			sunxi_hdmi_codec_prepare(substream, NULL);
		}
	}

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (sunxi_daudio->asrc_function_en) {
		pr_warn("set asrc params\n");
		/* 0x80 */
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_MCLKCFG,
					(0x1 << DAUDIO_ASRC_MCLK_GATE),
					(0x1 << DAUDIO_ASRC_MCLK_GATE));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_MCLKCFG,
					(0xFFFF << DAUDIO_ASRC_MCLK_RATIO),
					(0x1 << DAUDIO_ASRC_MCLK_RATIO));
		/* 0x84 */
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_FSOUTCFG,
					(0x1 << DAUDIO_ASRC_FSOUT_GATE),
					(0x1 << DAUDIO_ASRC_FSOUT_GATE));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_FSOUTCFG,
					(0xF << DAUDIO_ASRC_FSOUT_CLKSRC),
					(0x0 << DAUDIO_ASRC_FSOUT_CLKSRC));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_FSOUTCFG,
					(0xF << DAUDIO_ASRC_FSOUT_CLKDIV1),
//					(0xD << DAUDIO_ASRC_FSOUT_CLKDIV1));
					(0xF << DAUDIO_ASRC_FSOUT_CLKDIV1));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_FSOUTCFG,
					(0xF << DAUDIO_ASRC_FSOUT_CLKDIV2),
					(0xA << DAUDIO_ASRC_FSOUT_CLKDIV2));
		/* 0x88 */
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_FSIN_EXTCFG,
			(0x1 << DAUDIO_ASRC_FSIN_EXTEN),
			(0x1 << DAUDIO_ASRC_FSIN_EXTEN));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_FSIN_EXTCFG,
			(0xFF << DAUDIO_ASRC_FSIN_EXTCYCLE),
			(0xA << DAUDIO_ASRC_FSIN_EXTCYCLE));
		/* 0x90 */
		regmap_write(mem_info->regmap, SUNXI_DAUDIO_ASRC_MANCFG, 0);
#if 0
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_MANCFG,
			(0x1 << DAUDIO_ASRC_MANRATIOEN),
			(0x0 << DAUDIO_ASRC_MANRATIOEN));
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_MANCFG,
			(0x3FFFFFF << DAUDIO_ASRC_MAN_RATIO),
			(0x155555 << DAUDIO_ASRC_MAN_RATIO));
#endif
		/* 0x8C */
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_ASRCEN,
			(0x1 << DAUDIO_ASRC_ASRCEN),
			(0x1 << DAUDIO_ASRC_ASRCEN));
		} else {
		/* 0x8C */
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_ASRC_ASRCEN,
			(0x1 << DAUDIO_ASRC_ASRCEN),
			(0x0 << DAUDIO_ASRC_ASRCEN));
		}
	}
#endif

	return 0;
}

static int sunxi_daudio_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	sunxi_daudio_init_fmt(sunxi_daudio, fmt);

	return 0;
}

static int sunxi_daudio_set_sysclk(struct snd_soc_dai *dai,
			int clk_id, unsigned int freq, int dir)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_clk_info *clk_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	clk_info = &sunxi_daudio->clk_info;

	switch (clk_info->clk_parent) {
	case DAUDIO_CLK_PLL_AUDIO_X1:
		if (clk_set_rate(clk_info->clk_pll, freq)) {
			dev_err(sunxi_daudio->dev, "Freq : %u not support\n", freq);
			return -EINVAL;
		}
		break;
	default:
	case DAUDIO_CLK_PLL_AUDIO_X4:
		if (clk_set_rate(clk_info->clk_pll, freq * 4)) {
			dev_err(sunxi_daudio->dev, "Freq : %u not support\n", freq * 4);
			return -EINVAL;
		}
		break;
	}

	if (clk_set_rate(clk_info->clk_module, freq)) {
		dev_err(sunxi_daudio->dev, "Freq : %u not support\n", freq);
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	/* set the asrc clk to 98MHz */
	if (clk_set_rate(clk_info->clk_asrc, 98304000)) {
		dev_err(sunxi_daudio->dev, "asrc clk set the freq: %d failed!\n", freq);
		return -EINVAL;
	}
#endif

	return 0;
}

static int sunxi_daudio_set_clkdiv(struct snd_soc_dai *dai,
				int clk_id, int clk_div)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_platform_data_info *pdata_info = NULL;
	unsigned int bclk_div, div_ratio;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	mem_info = &sunxi_daudio->mem_info;
	dts_info = &sunxi_daudio->dts_info;
	pdata_info = &dts_info->pdata_info;

	if (pdata_info->tdm_config)
		/* I2S/TDM two channel mode */
		div_ratio = clk_div/(2 * pdata_info->pcm_lrck_period);
	else
		/* PCM mode */
		div_ratio = clk_div / pdata_info->pcm_lrck_period;

	switch (div_ratio) {
	case	1:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_1;
		break;
	case	2:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_2;
		break;
	case	4:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_3;
		break;
	case	6:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_4;
		break;
	case	8:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_5;
		break;
	case	12:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_6;
		break;
	case	16:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_7;
		break;
	case	24:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_8;
		break;
	case	32:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_9;
		break;
	case	48:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_10;
		break;
	case	64:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_11;
		break;
	case	96:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_12;
		break;
	case	128:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_13;
		break;
	case	176:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_14;
		break;
	case	192:
		bclk_div = SUNXI_DAUDIO_BCLK_DIV_15;
		break;
	default:
		dev_err(sunxi_daudio->dev, "unsupport clk_div\n");
		return -EINVAL;
	}
	/* setting bclk to driver external codec bit clk */
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CLKDIV,
			(SUNXI_DAUDIO_BCLK_DIV_MASK << BCLK_DIV),
			(bclk_div << BCLK_DIV));

	return 0;
}

static int sunxi_daudio_dai_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_platform_data_info *pdata_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is NULL!\n", __func__);
		return -ENOMEM;
	}
	mem_info = &sunxi_daudio->mem_info;
	dts_info = &sunxi_daudio->dts_info;
	pdata_info = &dts_info->pdata_info;

	/* FIXME: As HDMI module to play audio, it need at least 1100ms to sync.
	 * if we not wait we lost audio data to playback, or we wait for 1100ms
	 * to playback, user experience worst than you can imagine. So we need
	 * to cutdown that sync time by keeping clock signal on. we just enable
	 * it at startup and resume, cutdown it at remove and suspend time.
	 */
	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_daudio->playback_dma_param);
	} else {
		snd_soc_dai_set_dma_data(dai, substream,
					&sunxi_daudio->capture_dma_param);
	}

	return 0;
}

static int sunxi_daudio_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;

	switch (cmd) {
	case	SNDRV_PCM_TRIGGER_START:
	case	SNDRV_PCM_TRIGGER_RESUME:
	case	SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_daudio_txctrl_enable(sunxi_daudio, true);
			/* Global enable I2S/TDM module */
			if ((pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE)
					&& (!sunxi_daudio->playback_en)) {
				sunxi_daudio_global_enable(sunxi_daudio, true);
				sunxi_daudio->playback_en = 1;
			}
		} else {
			sunxi_daudio_rxctrl_enable(sunxi_daudio, true);
			if (pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE)
				sunxi_daudio_global_enable(sunxi_daudio, true);
		}
		break;
	case	SNDRV_PCM_TRIGGER_STOP:
	case	SNDRV_PCM_TRIGGER_SUSPEND:
	case	SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			sunxi_daudio_txctrl_enable(sunxi_daudio, false);
			/* Global enable I2S/TDM module */
			if ((pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE)
					&& (sunxi_daudio->playback_en)) {
				sunxi_daudio_global_enable(sunxi_daudio, false);
				sunxi_daudio->playback_en = 0;
			}
		} else {
			if (pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
				sunxi_daudio_rxctrl_enable(sunxi_daudio, false);
				sunxi_daudio_global_enable(sunxi_daudio, false);
			} else {
				sunxi_daudio_rxctrl_enable(sunxi_daudio, false);
			}
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sunxi_daudio_prepare(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	unsigned int i;
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_platform_data_info *pdata_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_daudio)) {
		dev_err(dai->dev, "[%s] sunxi_daudio is null.\n", __func__);
		return -ENOMEM;
	}
	mem_info = &sunxi_daudio->mem_info;
	dts_info = &sunxi_daudio->dts_info;
	pdata_info = &dts_info->pdata_info;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		for (i = 0 ; i < SUNXI_DAUDIO_FTX_TIMES ; i++) {
			regmap_update_bits(mem_info->regmap,
				SUNXI_DAUDIO_FIFOCTL,
				(1<<FIFO_CTL_FTX), (1<<FIFO_CTL_FTX));
			mdelay(1);
		}
		regmap_write(mem_info->regmap, SUNXI_DAUDIO_TXCNT, 0);

		if (sunxi_daudio->hub_mode &&
			(pdata_info->daudio_type != SUNXI_DAUDIO_HDMIAUDIO_TYPE)) {
			unsigned int DAUDIO_SDO_MASK = 0;
			for (i = 0; i < pdata_info->tx_num; i++)
				DAUDIO_SDO_MASK |= 0x1 << i;
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(DAUDIO_SDO_MASK << SDO0_EN),
					(DAUDIO_SDO_MASK << SDO0_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1 << CTL_TXEN), (1 << CTL_TXEN));
			sunxi_daudio_global_enable(sunxi_daudio, true);
			sunxi_daudio->playback_en = 1;
		}
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1<<FIFO_CTL_FRX), (1<<FIFO_CTL_FRX));
		regmap_write(mem_info->regmap, SUNXI_DAUDIO_RXCNT, 0);
	}

	return 0;
}

static int sunxi_daudio_probe(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, &sunxi_daudio->playback_dma_param,
				&sunxi_daudio->capture_dma_param);

	sunxi_daudio_init(sunxi_daudio);

	return 0;
}

static void sunxi_daudio_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;

	/* Special processing for HDMI hub playback to shutdown hdmi module */
	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
		if (sunxi_daudio->hub_mode) {
			sunxi_hdmi_codec_shutdown(substream, NULL);
		}
	} else if ((substream->stream == SNDRV_PCM_STREAM_PLAYBACK) &&
		(sunxi_daudio->hub_mode)) {
		unsigned int i = 0;
		unsigned int DAUDIO_SDO_MASK = 0;
		for (i = 0; i < pdata_info->tx_num; i++)
			DAUDIO_SDO_MASK |= 0x1 << i;
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(DAUDIO_SDO_MASK << SDO0_EN) | (1 << CTL_TXEN),
				(0 << SDO0_EN) | (0 << CTL_TXEN));
		if (sunxi_daudio->playback_en) {
			sunxi_daudio_global_enable(sunxi_daudio, false);
			sunxi_daudio->playback_en = 0;
		}

		return;
	}
}

static int sunxi_daudio_remove(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;

	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE)
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));

	return 0;
}

static int sunxi_daudio_suspend(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_clk_info *clk_info = &sunxi_daudio->clk_info;
	struct sunxi_daudio_pinctl_info *pin_info = &sunxi_daudio->pin_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	struct sunxi_daudio_regulator_info *regulator_info = &sunxi_daudio->regulator_info;
	unsigned int res_size = 0;
	int i = 0;

	pr_debug("[daudio] suspend .%s start\n", dev_name(sunxi_daudio->dev));
	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (0<<SDO0_EN));
		if (sunxi_daudio->hdmi_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO1_EN), (0<<SDO1_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO2_EN), (0<<SDO2_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO3_EN), (0<<SDO3_EN));
		}
		/* Global enable I2S/TDM module */
		sunxi_daudio_global_enable(sunxi_daudio, false);
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (0<<CTL_TXEN));
	}

	res_size = resource_size(&mem_info->res);
	sunxi_daudio->reg_label =
		kzalloc(sizeof(struct daudio_label) * res_size, GFP_KERNEL);
	if (!IS_ERR_OR_NULL(sunxi_daudio->reg_label)) {
		/* for save daudio reg */
		for (i = 0; i <= res_size; i++) {
			regmap_read(mem_info->regmap, i << 2,
					&((sunxi_daudio->reg_label+i)->value));
		}
	}

	clk_disable_unprepare(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	clk_disable_unprepare(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	if (pin_info->pinctrl_used) {
		pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate_sleep);
	}

	if (!IS_ERR_OR_NULL(regulator_info->daudio_regulator))
		regulator_disable(regulator_info->daudio_regulator);

	pr_debug("[daudio] suspend .%s end \n", dev_name(sunxi_daudio->dev));

	return 0;
}

static int sunxi_daudio_resume(struct snd_soc_dai *dai)
{
	struct sunxi_daudio_info *sunxi_daudio = snd_soc_dai_get_drvdata(dai);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_clk_info *clk_info = &sunxi_daudio->clk_info;
	struct sunxi_daudio_pinctl_info *pin_info = &sunxi_daudio->pin_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	struct sunxi_daudio_regulator_info *regulator_info = &sunxi_daudio->regulator_info;
	unsigned int res_size = 0;
	int ret;
	int i = 0;

	pr_debug("[%s] resume .%s start\n", __func__,
			dev_name(sunxi_daudio->dev));

	if (!IS_ERR_OR_NULL(regulator_info->daudio_regulator)) {
		ret = regulator_enable(regulator_info->daudio_regulator);
		if (ret < 0) {
			dev_err(sunxi_daudio->dev,
				"resume enable duaido vcc-pin failed\n");
				goto err_resume_regulator_enable;
		}
	}

	if (reset_control_deassert(clk_info->clk_rst)) {
		pr_err("daudio: resume deassert the daudio reset failed\n");
		return -EINVAL;
	}
	ret = clk_prepare_enable(clk_info->clk_bus);
	if (ret < 0) {
		dev_err(sunxi_daudio->dev, "enable daudio bus clk failed, resume exit\n");
		goto err_resume_clk_enable_bus;
	}
	ret = clk_prepare_enable(clk_info->clk_pll);
	if (ret < 0) {
		dev_err(sunxi_daudio->dev, "pllclk resume failed\n");
		ret = -EBUSY;
		goto err_resume_clk_pll_enable;
	}
	ret = clk_prepare_enable(clk_info->clk_module);
	if (ret < 0) {
		dev_err(sunxi_daudio->dev, "moduleclk resume failed\n");
		ret = -EBUSY;
		goto err_resume_clk_module_enable;
	}

	if (pin_info->pinctrl_used) {
		ret = pinctrl_select_state(pin_info->pinctrl,
					pin_info->pinstate);
		if (ret < 0) {
			dev_warn(sunxi_daudio->dev,
				"daudio set pinctrl default state fail\n");
			goto err_resume_pin_select_default;
		}
	}

	sunxi_daudio_init(sunxi_daudio);

	res_size = resource_size(&mem_info->res);
	if (!IS_ERR_OR_NULL(sunxi_daudio->reg_label)) {
		/* for echo the save reg */
		for (i = 0; i <= res_size; i++) {
			regmap_write(mem_info->regmap, i << 2,
					(sunxi_daudio->reg_label+i)->value);
		}
		kfree(sunxi_daudio->reg_label);
	}

	/* for clear fifo */
	for (i = 0 ; i < SUNXI_DAUDIO_FTX_TIMES ; i++) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1 << FIFO_CTL_FTX), (1 << FIFO_CTL_FTX));
		mdelay(1);
	}

	regmap_write(mem_info->regmap, SUNXI_DAUDIO_TXCNT, 0);
	regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_FIFOCTL,
				(1 << FIFO_CTL_FRX), (1 << FIFO_CTL_FRX));
	regmap_write(mem_info->regmap, SUNXI_DAUDIO_RXCNT, 0);

	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (1<<SDO0_EN));
		if (sunxi_daudio->hdmi_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO1_EN), (1<<SDO1_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO2_EN), (1<<SDO2_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO3_EN), (1<<SDO3_EN));
		}
		/* Global enable I2S/TDM module */
		sunxi_daudio_global_enable(sunxi_daudio, true);
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<CTL_TXEN), (1<<CTL_TXEN));
	}
	pr_debug("[%s] resume .%s end\n", __func__,
			dev_name(sunxi_daudio->dev));

	return 0;

err_resume_pin_select_default:
		clk_disable_unprepare(clk_info->clk_module);
err_resume_clk_module_enable:
		clk_disable_unprepare(clk_info->clk_pll);
err_resume_clk_pll_enable:
		clk_disable_unprepare(clk_info->clk_bus);
err_resume_clk_enable_bus:
		reset_control_assert(clk_info->clk_rst);
err_resume_regulator_enable:
	if (!IS_ERR_OR_NULL(regulator_info->daudio_regulator))
		regulator_disable(regulator_info->daudio_regulator);
	return ret;
}

static struct snd_soc_dai_ops sunxi_daudio_dai_ops = {
	.startup = sunxi_daudio_dai_startup,
	.set_sysclk = sunxi_daudio_set_sysclk,
	.set_clkdiv = sunxi_daudio_set_clkdiv,
	.set_fmt = sunxi_daudio_set_fmt,
	.hw_params = sunxi_daudio_hw_params,
	.prepare = sunxi_daudio_prepare,
	.trigger = sunxi_daudio_trigger,
	.shutdown = sunxi_daudio_shutdown,
};

static struct snd_soc_dai_driver sunxi_daudio_dai = {
	.probe = sunxi_daudio_probe,
	.suspend = sunxi_daudio_suspend,
	.resume = sunxi_daudio_resume,
	.remove = sunxi_daudio_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_3LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DAUDIO_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE
			| SNDRV_PCM_FMTBIT_S20_3LE
			| SNDRV_PCM_FMTBIT_S24_LE
			| SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &sunxi_daudio_dai_ops,
};

static const struct snd_soc_component_driver sunxi_daudio_component = {
	.name		= DRV_NAME,
	.controls	= sunxi_daudio_controls,
	.num_controls	= ARRAY_SIZE(sunxi_daudio_controls),
};

static const struct of_device_id sunxi_daudio_of_match[] = {
	{
		.compatible = "allwinner,sunxi-daudio",
	},
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_daudio_of_match);

static struct regmap_config sunxi_daudio_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DAUDIO_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_daudio_mem_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	int ret = 0;

	ret = of_address_to_resource(np, 0, &(mem_info->res));
	if (ret) {
		dev_err(&pdev->dev, "parse device node resource failed\n");
		ret = -EINVAL;
		goto err_of_addr_to_resource;
	}

	mem_info->memregion = devm_request_mem_region(&pdev->dev,
					mem_info->res.start,
					resource_size(&mem_info->res),
					DRV_NAME);
	if (IS_ERR_OR_NULL(mem_info->memregion)) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_region;
	}

	mem_info->membase = devm_ioremap(&pdev->dev,
					mem_info->memregion->start,
					resource_size(mem_info->memregion));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -EBUSY;
		goto err_devm_ioremap;
	}

	sunxi_daudio_regmap_config.max_register = resource_size(&mem_info->res);
	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev,
					mem_info->membase,
					&sunxi_daudio_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		dev_err(&pdev->dev, "regmap init failed\n");
		ret = -EINVAL;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
	devm_iounmap(&pdev->dev, mem_info->membase);
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_region:
err_of_addr_to_resource:
	devm_kfree(&pdev->dev, sunxi_daudio);
	return ret;
};

static int sunxi_daudio_clk_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_clk_info *clk_info = &sunxi_daudio->clk_info;
	int ret = 0;

	ret = of_property_read_u32(np, "clk_parent", (u32 *)&clk_info->clk_parent);
	if (ret != 0) {
		dev_warn(&pdev->dev, "clk_parent config missing or invalid.\n");
		clk_info->clk_parent = DAUDIO_CLK_PLL_AUDIO_X1;
	}

	clk_info->clk_pll = of_clk_get(np, 0);
	clk_info->clk_module = of_clk_get(np, 1);
	clk_info->clk_bus = of_clk_get(np, 2);
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	clk_info->clk_pll1 = of_clk_get(np, 3);
	clk_info->clk_asrc = of_clk_get(np, 4);
#endif
	clk_info->clk_rst = devm_reset_control_get(&pdev->dev, NULL);

	if (clk_set_parent(clk_info->clk_module, clk_info->clk_pll)) {
		dev_err(&pdev->dev,
			"set parent of clk_module to clk_pll failed!\n");
		ret = -EINVAL;
		goto err_clk_set_parent;
	}
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	if (clk_set_parent(clk_info->clk_asrc, clk_info->clk_pll1)) {
		dev_err(&pdev->dev,
			"set parent of clk_module to clk_pll1 failed!\n");
		ret = -EINVAL;
		goto err_clk_set_parent;
	}
#endif

	if (reset_control_deassert(clk_info->clk_rst)) {
		pr_err("daudio: deassert the daudio reset failed\n");
		return -EINVAL;
	}
	if (clk_prepare_enable(clk_info->clk_bus)) {
		dev_err(&pdev->dev, "daudio clk bus enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_bus;
	}
	if (clk_prepare_enable(clk_info->clk_pll)) {
		dev_err(&pdev->dev, "clk_pll enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_pll;
	}
	if (clk_prepare_enable(clk_info->clk_module)) {
		dev_err(&pdev->dev, "clk_module enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_module;
	}
#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	if (clk_prepare_enable(clk_info->clk_pll1)) {
		dev_err(&pdev->dev, "clk_pll1 enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_pll;
	}
	if (clk_prepare_enable(clk_info->clk_asrc)) {
		dev_err(&pdev->dev, "clk_asrc enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_module;
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

static int sunxi_daudio_pinctrl_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_pinctl_info *pin_info = &sunxi_daudio->pin_info;
//	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
//	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	int ret = 0;
	int temp_val;

	ret = of_property_read_u32(np, "pinctrl_used", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pinctrl_used config missing\n");
		pin_info->pinctrl_used = 1;
	} else {
		pin_info->pinctrl_used = temp_val?1:0;
	}

	if (pin_info->pinctrl_used) {
		pin_info->pinctrl = devm_pinctrl_get(&pdev->dev);
		if (IS_ERR_OR_NULL(pin_info->pinctrl)) {
			dev_err(&pdev->dev, "pinctrl get failed\n");
			ret = -EINVAL;
			return ret;
		}
		pin_info->pinstate = pinctrl_lookup_state(
				pin_info->pinctrl, PINCTRL_STATE_DEFAULT);
		if (IS_ERR_OR_NULL(pin_info->pinstate)) {
			dev_err(&pdev->dev, "pinctrl default state get fail\n");
			ret = -EINVAL;
			goto err_loopup_pinstate;
		}
		pin_info->pinstate_sleep = pinctrl_lookup_state(
				pin_info->pinctrl, PINCTRL_STATE_SLEEP);
		if (IS_ERR_OR_NULL(pin_info->pinstate_sleep)) {
			dev_err(&pdev->dev, "pinctrl sleep state get failed\n");
			ret = -EINVAL;
			goto err_loopup_pin_sleep;
		}

		ret = pinctrl_select_state(pin_info->pinctrl,
					pin_info->pinstate);
		if (ret < 0) {
			dev_err(sunxi_daudio->dev,
				"daudio set pinctrl default state fail\n");
			ret = -EBUSY;
			goto err_pinctrl_select_default;
		}
	}

	return 0;

err_pinctrl_select_default:
err_loopup_pin_sleep:
err_loopup_pinstate:
	if (pin_info->pinctrl_used)
		devm_pinctrl_put(pin_info->pinctrl);
	return ret;
};

static int sunxi_daudio_dts_params_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
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
	if (ret != 0) {
		dev_warn(&pdev->dev, "capture cma kbytes config missing or invalid.\n");
		dts_info->capture_cma = SUNXI_AUDIO_CMA_MAX_KBYTES;
	} else {
		if (temp_val > SUNXI_AUDIO_CMA_MAX_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MAX_KBYTES;
		else if (temp_val < SUNXI_AUDIO_CMA_MIN_KBYTES)
			temp_val = SUNXI_AUDIO_CMA_MIN_KBYTES;
		dts_info->capture_cma = temp_val;
	}

	ret = of_property_read_u32(np, "tdm_num", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "tdm configuration missing\n");
		/*
		 * warnning just continue,
		 * making tdm_num as default setting
		 */
		pdata_info->tdm_num = 0;
	} else {
		/*
		 * FIXME, for channel number mess,
		 * so just not check channel overflow
		 */
		pdata_info->tdm_num = temp_val;
	}

	ret = of_property_read_u32(np, "daudio_rxsync_en", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "daudio_rxsync_en config missing\n");
		pdata_info->daudio_rxsync_en = 0;
	} else {
		pdata_info->daudio_rxsync_en = temp_val;
	}

	ret = of_property_read_u32(np, "pcm_lrck_period", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "pcm_lrck_period config missing or invalid\n");
		pdata_info->pcm_lrck_period = 0;
	} else {
		pdata_info->pcm_lrck_period = temp_val;
	}

	ret = of_property_read_u32(np, "msb_lsb_first", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "msb_lsb_first config missing or invalid\n");
		pdata_info->msb_lsb_first = 0;
	} else {
		pdata_info->msb_lsb_first = temp_val;
	}

	ret = of_property_read_u32(np, "slot_width_select", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "slot_width_select config missing or invalid\n");
		pdata_info->slot_width_select = 0;
	} else {
		pdata_info->slot_width_select = temp_val;
	}

	ret = of_property_read_u32(np, "frametype", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "frametype config missing or invalid\n");
		pdata_info->frame_type = 0;
	} else {
		pdata_info->frame_type = temp_val;
	}

	ret = of_property_read_u32(np, "sign_extend", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "sign_extend config missing or invalid\n");
		pdata_info->sign_extend = 0;
	} else {
		pdata_info->sign_extend = temp_val;
	}

	ret = of_property_read_u32(np, "tx_data_mode", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "tx_data_mode config missing or invalid\n");
		pdata_info->tx_data_mode = 0;
	} else {
		pdata_info->tx_data_mode = temp_val;
	}

	ret = of_property_read_u32(np, "rx_data_mode", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "rx_data_mode config missing or invalid\n");
		pdata_info->rx_data_mode = 0;
	} else {
		pdata_info->rx_data_mode = temp_val;
	}

	ret = of_property_read_u32(np, "tdm_config", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "tdm_config config missing or invalid\n");
		pdata_info->tdm_config = 1;
	} else {
		pdata_info->tdm_config = temp_val;
	}

	ret = of_property_read_u32(np, "mclk_div", &temp_val);
	if (ret < 0) {
		pdata_info->mclk_div = 0;
	} else {
		pdata_info->mclk_div = temp_val;
	}

	ret = of_property_read_u32(np, "tx_num", &temp_val);
	if (ret < 0) {
		pdata_info->tx_num = SUNXI_DAUDIO_TXCH_NUM;
	} else {
		if ((temp_val < SUNXI_DAUDIO_TXCH_NUM) && (temp_val != 0))
			pdata_info->tx_num = temp_val;
		else
			pdata_info->tx_num = SUNXI_DAUDIO_TXCH_NUM;
	}

	ret = of_property_read_u32(np, "tx_chmap0", &temp_val);
	if (ret < 0) {
		pdata_info->tx_chmap0 = SUNXI_DEFAULT_TXCHMAP0;
	} else {
		pdata_info->tx_chmap0 = temp_val;
	}

	ret = of_property_read_u32(np, "tx_chmap1", &temp_val);
	if (ret < 0) {
		pdata_info->tx_chmap1 = SUNXI_DEFAULT_TXCHMAP1;
	} else {
		pdata_info->tx_chmap1 = temp_val;
	}

	ret = of_property_read_u32(np, "rx_num", &temp_val);
	if (ret < 0) {
		pdata_info->rx_num = SUNXI_DAUDIO_RXCH_NUM;
	} else {
		if ((temp_val < SUNXI_DAUDIO_RXCH_NUM) && (temp_val != 0))
			pdata_info->rx_num = temp_val;
		else
			pdata_info->rx_num = SUNXI_DAUDIO_RXCH_NUM;
	}

	ret = of_property_read_u32(np, "rx_chmap0", &temp_val);
	if (ret < 0) {
		if (pdata_info->rx_num == 1)
			pdata_info->rx_chmap0 = SUNXI_DEFAULT_RXCHMAP;
		else
			pdata_info->rx_chmap0 = SUNXI_DEFAULT_RXCHMAP0;
	} else {
		pdata_info->rx_chmap0 = temp_val;
	}

	ret = of_property_read_u32(np, "rx_chmap1", &temp_val);
	if (ret < 0) {
		pdata_info->rx_chmap1 = SUNXI_DEFAULT_RXCHMAP1;
	} else {
		pdata_info->rx_chmap1 = temp_val;
	}

	ret = of_property_read_u32(np, "rx_chmap2", &temp_val);
	if (ret < 0) {
		pdata_info->rx_chmap2 = SUNXI_DEFAULT_RXCHMAP3;
	} else {
		pdata_info->rx_chmap2 = temp_val;
	}

	ret = of_property_read_u32(np, "rx_chmap3", &temp_val);
	if (ret < 0) {
		pdata_info->rx_chmap3 = SUNXI_DEFAULT_RXCHMAP3;
	} else {
		pdata_info->rx_chmap3 = temp_val;
	}

	ret = of_property_read_u32(np, "daudio_type", &temp_val);
	if (ret < 0) {
		pdata_info->daudio_type = SUNXI_DAUDIO_EXTERNAL_TYPE;
	} else {
		pdata_info->daudio_type = temp_val;
	}

#if IS_ENABLED(CONFIG_SND_SUNXI_SOC_DAUDIO_ASRC)
	ret = of_property_read_u32(np, "asrc_function_en", &temp_val);
	if (ret < 0) {
		dev_warn(&pdev->dev, "asrc_function_en config missing and set 0\n");
		sunxi_daudio->asrc_function_en = 0;
	} else {
		sunxi_daudio->asrc_function_en = temp_val;
	}
#endif
	return 0;
};

static int sunxi_daudio_regulator_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_regulator_info *regulator_info
					= &sunxi_daudio->regulator_info;
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	int ret = 0;

	regulator_info->regulator_name = NULL;
	ret = of_property_read_string(np, "daudio_regulator",
			&regulator_info->regulator_name);
	if (ret < 0) {
		dev_warn(&pdev->dev, "regulator missing or invalid\n");
		regulator_info->daudio_regulator = NULL;
	} else {
		regulator_info->daudio_regulator =
			regulator_get(NULL, regulator_info->regulator_name);
		if (IS_ERR_OR_NULL(regulator_info->daudio_regulator)) {
			dev_err(&pdev->dev, "get duaido[%d] vcc-pin failed\n",
				pdata_info->tdm_num);
			ret = -EFAULT;
			goto err_regulator_get;
		}
		ret = regulator_set_voltage(regulator_info->daudio_regulator,
					3300000, 3300000);
		if (ret < 0) {
			dev_err(&pdev->dev, "set duaido[%d] voltage failed\n",
					pdata_info->tdm_num);
			ret = -EFAULT;
			goto err_regulator_set_vol;
		}
		ret = regulator_enable(regulator_info->daudio_regulator);
		if (ret < 0) {
			dev_err(&pdev->dev, "enable duaido[%d] vcc-pin failed\n",
					pdata_info->tdm_num);
			ret = -EFAULT;
			goto err_regulator_enable;
		}
	}

	return 0;

err_regulator_enable:
err_regulator_set_vol:
	if (regulator_info->daudio_regulator)
		regulator_put(regulator_info->daudio_regulator);
err_regulator_get:
	return ret;
};

static void sunxi_daudio_dma_params_init(struct resource res,
					struct sunxi_daudio_info *sunxi_daudio)
{
	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;

	switch (pdata_info->daudio_type) {
	case	SUNXI_DAUDIO_EXTERNAL_TYPE:
		sunxi_daudio->playback_dma_param.src_maxburst = 4;
		sunxi_daudio->playback_dma_param.dst_maxburst = 4;
		sunxi_daudio->playback_dma_param.dma_addr = res.start + SUNXI_DAUDIO_TXFIFO;
		sunxi_daudio->playback_dma_param.cma_kbytes = dts_info->playback_cma;
		sunxi_daudio->playback_dma_param.fifo_size = DAUDIO_TX_FIFO_SIZE;

		sunxi_daudio->capture_dma_param.dma_addr = res.start + SUNXI_DAUDIO_RXFIFO;
		sunxi_daudio->capture_dma_param.src_maxburst = 4;
		sunxi_daudio->capture_dma_param.dst_maxburst = 4;
		sunxi_daudio->capture_dma_param.cma_kbytes = dts_info->capture_cma;
		sunxi_daudio->capture_dma_param.fifo_size = DAUDIO_RX_FIFO_SIZE;
		break;
	case	SUNXI_DAUDIO_HDMIAUDIO_TYPE:
		sunxi_daudio->playback_dma_param.src_maxburst = 8;
		sunxi_daudio->playback_dma_param.dst_maxburst = 8;
		sunxi_daudio->playback_dma_param.dma_addr = res.start + SUNXI_DAUDIO_TXFIFO;
		sunxi_daudio->playback_dma_param.cma_kbytes = dts_info->playback_cma;
		sunxi_daudio->playback_dma_param.fifo_size = DAUDIO_TX_FIFO_SIZE;
		sunxi_daudio->hdmi_en = 1;

		sunxi_daudio->capture_dma_param.src_maxburst = 8;
		sunxi_daudio->capture_dma_param.dst_maxburst = 8;
		sunxi_daudio->capture_dma_param.dma_addr = res.start + SUNXI_DAUDIO_RXFIFO;
		sunxi_daudio->capture_dma_param.cma_kbytes = dts_info->capture_cma;
		sunxi_daudio->capture_dma_param.fifo_size = DAUDIO_RX_FIFO_SIZE;
		break;
	default:
		pr_err("[%s] missing digital audio type\n", __func__);
	}
};

static int sunxi_daudio_dev_probe(struct platform_device *pdev)
{
	const struct of_device_id *match = NULL;
	struct sunxi_daudio_info *sunxi_daudio = NULL;
	struct sunxi_daudio_mem_info *mem_info = NULL;
	struct sunxi_daudio_clk_info *clk_info = NULL;
	struct sunxi_daudio_pinctl_info *pin_info = NULL;
	struct sunxi_daudio_dts_info *dts_info = NULL;
	struct sunxi_daudio_regulator_info *regulator_info = NULL;
	struct sunxi_daudio_platform_data_info *pdata_info = NULL;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	match = of_match_device(sunxi_daudio_of_match, &pdev->dev);
	if (match) {
		sunxi_daudio = devm_kzalloc(&pdev->dev,
					sizeof(struct sunxi_daudio_info),
					GFP_KERNEL);
		if (IS_ERR_OR_NULL(sunxi_daudio)) {
			dev_err(&pdev->dev, "alloc sunxi_daudio failed\n");
			ret = -ENOMEM;
			goto err_devm_malloc_sunxi_daudio;
		}
		dev_set_drvdata(&pdev->dev, sunxi_daudio);

		sunxi_daudio->dev = &pdev->dev;
		mem_info = &sunxi_daudio->mem_info;
		clk_info = &sunxi_daudio->clk_info;
		pin_info = &sunxi_daudio->pin_info;
		dts_info = &sunxi_daudio->dts_info;
		pdata_info = &dts_info->pdata_info;
		regulator_info = &sunxi_daudio->regulator_info;
	} else {
		dev_err(&pdev->dev, "node match failed\n");
		return -EINVAL;
	}

	/* init some params */
	sunxi_daudio->hub_mode = 0;
	sunxi_daudio->playback_en = 0;
	sunxi_daudio->capture_en = 0;

	/* mem init about */
	ret = sunxi_daudio_mem_init(pdev, np, sunxi_daudio);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* clk init about */
	ret = sunxi_daudio_clk_init(pdev, np, sunxi_daudio);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* pinctrl init about */
	ret = sunxi_daudio_pinctrl_init(pdev, np, sunxi_daudio);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* regulator init about */
	ret = sunxi_daudio_regulator_init(pdev, np, sunxi_daudio);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* dts params init about */
	ret = sunxi_daudio_dts_params_init(pdev, np, sunxi_daudio);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* daudio dma params about */
	sunxi_daudio_dma_params_init(sunxi_daudio->mem_info.res, sunxi_daudio);

	/* sunxi daudio component about register */
	ret = snd_soc_register_component(&pdev->dev, &sunxi_daudio_component,
					&sunxi_daudio_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "component register failed\n");
		ret = -ENOMEM;
		goto err_register_component;
	}

	switch (pdata_info->daudio_type) {
	case	SUNXI_DAUDIO_EXTERNAL_TYPE:
		ret = asoc_dma_platform_register(&pdev->dev, 0);
		if (ret) {
			dev_err(&pdev->dev, "register ASoC platform failed\n");
			ret = -ENOMEM;
			goto err_asoc_dma_platform_register;
		}
		break;
	case	SUNXI_DAUDIO_HDMIAUDIO_TYPE:
		ret = asoc_dma_platform_register(&pdev->dev,
						SND_DMAENGINE_PCM_FLAG_NO_DT);
		if (ret) {
			dev_err(&pdev->dev, "register ASoC platform failed\n");
			ret = -ENOMEM;
			goto err_asoc_dma_platform_register;
		}
		break;
	default:
		dev_err(&pdev->dev, "missing digital audio type\n");
		ret = -EINVAL;
		goto err_daudio_type_asoc_dma_register;
	}

#ifdef SUNXI_DAUDIO_DEBUG
	ret  = sysfs_create_group(&pdev->dev.kobj, &daudio_debug_attr_group);
	if (ret)
		dev_err(&pdev->dev, "failed to create attr group\n");
#endif

	if (pdata_info->daudio_type == SUNXI_DAUDIO_HDMIAUDIO_TYPE) {
		regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
				(1<<SDO0_EN), (1<<SDO0_EN));
		if (sunxi_daudio->hdmi_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO1_EN), (1<<SDO1_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO2_EN), (1<<SDO2_EN));
			regmap_update_bits(mem_info->regmap, SUNXI_DAUDIO_CTL,
					(1<<SDO3_EN), (1<<SDO3_EN));
		}
		sunxi_daudio_global_enable(sunxi_daudio, true);
	}

	return 0;

err_asoc_dma_platform_register:
err_daudio_type_asoc_dma_register:
	snd_soc_unregister_component(&pdev->dev);
err_register_component:
	devm_kfree(&pdev->dev, sunxi_daudio);
err_devm_malloc_sunxi_daudio:
	of_node_put(np);
	return ret;
}

static int __exit sunxi_daudio_dev_remove(struct platform_device *pdev)
{
	struct sunxi_daudio_info *sunxi_daudio = dev_get_drvdata(&pdev->dev);
	struct sunxi_daudio_mem_info *mem_info = &sunxi_daudio->mem_info;
	struct sunxi_daudio_clk_info *clk_info = &sunxi_daudio->clk_info;
	struct sunxi_daudio_pinctl_info *pin_info = &sunxi_daudio->pin_info;
//	struct sunxi_daudio_dts_info *dts_info = &sunxi_daudio->dts_info;
//	struct sunxi_daudio_platform_data_info *pdata_info = &dts_info->pdata_info;
	struct sunxi_daudio_regulator_info *regulator_info = &sunxi_daudio->regulator_info;

#ifdef SUNXI_DAUDIO_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &daudio_debug_attr_group);
#endif

	snd_soc_unregister_component(&pdev->dev);
	devm_iounmap(&pdev->dev, mem_info->membase);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));

	if (regulator_info->daudio_regulator) {
		if (!IS_ERR_OR_NULL(regulator_info->daudio_regulator)) {
			regulator_disable(regulator_info->daudio_regulator);
			regulator_put(regulator_info->daudio_regulator);
		}
		if (pin_info->pinctrl_used) {
			devm_pinctrl_put(pin_info->pinctrl);
		}
	}

	clk_disable_unprepare(clk_info->clk_module);
	clk_put(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	clk_put(clk_info->clk_pll);
	clk_disable_unprepare(clk_info->clk_bus);
	clk_put(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	devm_kfree(&pdev->dev, sunxi_daudio);

	return 0;
}

static struct platform_driver sunxi_daudio_driver = {
	.probe = sunxi_daudio_dev_probe,
	.remove = __exit_p(sunxi_daudio_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_daudio_of_match,
	},
};

module_platform_driver(sunxi_daudio_driver);
MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DAI AUDIO ASoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi-daudio");
