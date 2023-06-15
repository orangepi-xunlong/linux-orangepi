/*
 * sound\soc\sunxi\sunxi-dmic.c
 * (C) Copyright 2019-2025
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
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
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/reset.h>
#include <asm/dma.h>
#include <sound/dmaengine_pcm.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>

#include "sunxi-dmic.h"
#include "sunxi-pcm.h"

#define	DRV_NAME	"sunxi-dmic"
#define SUNXI_DMIC_DEBUG

struct dmic_rate {
	unsigned int samplerate;
	unsigned int rate_bit;
};

static const struct dmic_rate dmic_rate_s[] = {
	{44100, 0x0},
	{48000, 0x0},
	{22050, 0x2},
	/* KNOT support */
	{24000, 0x2},
	{11025, 0x4},
	{12000, 0x4},
	{32000, 0x1},
	{16000, 0x3},
	{8000,  0x5},
};

#ifdef SUNXI_DMIC_DEBUG
static struct sunxi_dmic_reg_label reg_labels[] = {
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_EN),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_SR),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CTR),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_INTC),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_INTS),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_FIFO_CTR),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_FIFO_STA),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CH_NUM),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CH_MAP),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_CNT),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_DATA0_1_VOL),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_DATA2_3_VOL),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_HPF_CTRL),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_HPF_COEF),
	SUNXI_DMIC_REG_LABEL(SUNXI_DMIC_HPF_GAIN),
	SUNXI_DMIC_REG_LABEL_END,
};

static ssize_t show_dmic_reg(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sunxi_dmic_info *sunxi_dmic = dev_get_drvdata(dev);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	int count = 0;
	unsigned int reg_val;
	int ret = 0;
	int i = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dev, "sunxi_dmic is NULL!\n");
		return count;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &sunxi_dmic->mem_info;

	count = snprintf(buf, PAGE_SIZE, "Dump dmic reg:\n");
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
		echo 0,0x0 > dmic_reg
	write:
		echo 1,0x00,0xa > dmic_reg
*/
static ssize_t store_dmic_reg(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t count)
{
	int ret;
	int rw_flag;
	int reg_val_read;
	unsigned int input_reg_val = 0;
	unsigned int input_reg_offset = 0;
	struct sunxi_dmic_info *sunxi_dmic = dev_get_drvdata(dev);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dev, "sunxi_dmic is NULL!\n");
		return count;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &sunxi_dmic->mem_info;

	ret = sscanf(buf, "%d,0x%x,0x%x", &rw_flag, &input_reg_offset,
			&input_reg_val);

	if (ret == 3 || ret == 2) {
		if (!(rw_flag == 1 || rw_flag == 0)) {
			dev_err(dev, "rw_flag should be 0(read) or 1(write).\n");
			return count;
		}
		if (input_reg_offset > SUNXI_DMIC_REG_MAX) {
			pr_err("the reg offset is invalid! [0x0 - 0x%x]\n",
				SUNXI_DMIC_REG_MAX);
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
		pr_err("\nExample(reg range:0x0 - 0x%x):\n", SUNXI_DMIC_REG_MAX);
		pr_err("\nRead reg[0x04]:\n");
		pr_err("      echo 0,0x04 > dmic_reg\n");
		pr_err("Write reg[0x04]=0x10\n");
		pr_err("      echo 1,0x04,0x10 > dmic_reg\n");
	}

	return count;
}

static DEVICE_ATTR(dmic_reg, 0644, show_dmic_reg, store_dmic_reg);

static struct attribute *audio_debug_attrs[] = {
	&dev_attr_dmic_reg.attr,
	NULL,
};

static struct attribute_group dmic_debug_attr_group = {
	.name	= "dmic_debug",
	.attrs	= audio_debug_attrs,
};
#endif

/*
 * Configure DMA , Chan enable & Global enable
 */
static void sunxi_dmic_enable(struct sunxi_dmic_info *sunxi_dmic, bool enable)
{
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		pr_err("[%s] sunxi_dmic is NULL!\n", __func__);
		return;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &sunxi_dmic->mem_info;

	if (enable) {
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_INTC,
				(0x1 << FIFO_DRQ_EN), (0x1 << FIFO_DRQ_EN));
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0xFF << DATA_CH_EN),
				(sunxi_dmic->chan_en << DATA_CH_EN));
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0x1 << GLOBE_EN), (0x1 << GLOBE_EN));

		if (dts_info->dmic_rxsync_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0x1 << DMIC_RX_EN_MUX), (0x1 << DMIC_RX_EN_MUX));
			regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0x1 << DMIC_RX_SYNC_EN), (0x1 << DMIC_RX_SYNC_EN));
		}
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0x1 << GLOBE_EN), (0x0 << GLOBE_EN));
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0xFF << DATA_CH_EN),
				(0x0 << DATA_CH_EN));
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_INTC,
				(0x1 << FIFO_DRQ_EN),
				(0x0 << FIFO_DRQ_EN));

		if (dts_info->dmic_rxsync_en) {
			regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0x1 << DMIC_RX_EN_MUX), (0x0 << DMIC_RX_EN_MUX));
			regmap_update_bits(mem_info->regmap, SUNXI_DMIC_EN,
				(0x1 << DMIC_RX_SYNC_EN), (0x0 << DMIC_RX_SYNC_EN));
		}
	}
}

static void sunxi_dmic_init(struct sunxi_dmic_info *sunxi_dmic)
{
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		pr_err("[%s] sunxi_dmic is NULL!\n", __func__);
		return;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &sunxi_dmic->mem_info;

	regmap_write(mem_info->regmap, SUNXI_DMIC_CH_MAP, dts_info->rx_chmap);
	regmap_update_bits(mem_info->regmap, SUNXI_DMIC_CTR,
			(0x7 << DMICDFEN), (0x7 << DMICDFEN));

	/* set the vol */
	regmap_update_bits(mem_info->regmap, SUNXI_DMIC_DATA0_1_VOL,
			(0xFF << DATA0L_VOL) | (0xFF << DATA0R_VOL),
			(dts_info->data_vol << DATA0L_VOL) |
			(dts_info->data_vol << DATA0R_VOL));
	regmap_update_bits(mem_info->regmap, SUNXI_DMIC_DATA0_1_VOL,
			(0xFF << DATA1L_VOL) | (0xFF << DATA1R_VOL),
			(dts_info->data_vol << DATA1L_VOL) |
			(dts_info->data_vol << DATA1R_VOL));

	regmap_update_bits(mem_info->regmap, SUNXI_DMIC_DATA2_3_VOL,
			(0xFF << DATA2L_VOL) | (0xFF << DATA2R_VOL),
			(dts_info->data_vol << DATA2L_VOL) |
			(dts_info->data_vol << DATA2R_VOL));
	regmap_update_bits(mem_info->regmap, SUNXI_DMIC_DATA2_3_VOL,
			(0xFF << DATA3L_VOL) | (0xFF << DATA3R_VOL),
			(dts_info->data_vol << DATA3L_VOL) |
			(dts_info->data_vol << DATA3R_VOL));
}

static int sunxi_dmic_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	int i;
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &sunxi_dmic->mem_info;

	/* sample resolution & sample fifo format */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_FIFO_CTR,
				(0x1 << DMIC_SAMPLE_RESOLUTION),
				(0x0 << DMIC_SAMPLE_RESOLUTION));
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_FIFO_CTR,
				(0x1 << DMIC_FIFO_MODE),
				(0x1 << DMIC_FIFO_MODE));
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_FIFO_CTR,
				(0x1 << DMIC_SAMPLE_RESOLUTION),
				(0x1 << DMIC_SAMPLE_RESOLUTION));
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_FIFO_CTR,
				(0x1 << DMIC_FIFO_MODE),
				(0x0 << DMIC_FIFO_MODE));
		break;
	default:
		dev_err(sunxi_dmic->dev, "Invalid format set\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(dmic_rate_s); i++) {
		if (dmic_rate_s[i].samplerate == params_rate(params)) {
			regmap_update_bits(mem_info->regmap, SUNXI_DMIC_SR,
			(0x7 << DMIC_SR),
			(dmic_rate_s[i].rate_bit << DMIC_SR));
			break;
		}
	}

	/* oversamplerate adjust */
	if (params_rate(params) >= 24000) {
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_CTR,
			(1 << DMIC_OVERSAMPLE_RATE),
			(1 << DMIC_OVERSAMPLE_RATE));
	} else {
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_CTR,
			(1 << DMIC_OVERSAMPLE_RATE),
			(0 << DMIC_OVERSAMPLE_RATE));
	}

	sunxi_dmic->chan_en = (1 << params_channels(params)) - 1;
	regmap_write(mem_info->regmap, SUNXI_DMIC_HPF_CTRL, sunxi_dmic->chan_en);

	/* DMIC num is M+1 */
	regmap_update_bits(mem_info->regmap, SUNXI_DMIC_CH_NUM,
		(0x7 << DMIC_CH_NUM),
		((params_channels(params) - 1) << DMIC_CH_NUM));

	return 0;
}

static int sunxi_dmic_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		sunxi_dmic_enable(sunxi_dmic, true);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		sunxi_dmic_enable(sunxi_dmic, false);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*
 * Reset & Flush FIFO
 */
static int sunxi_dmic_prepare(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	unsigned int i = 0;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	dts_info = &sunxi_dmic->dts_info;
	mem_info = &sunxi_dmic->mem_info;

	regmap_write(mem_info->regmap, SUNXI_DMIC_INTS,
			(1 << FIFO_OVERRUN_IRQ_PENDING) |
			(1 << FIFO_DATA_IRQ_PENDING));

	for (i = 0; i < SUNXI_DMIC_FTX_TIMES; i++) {
		regmap_update_bits(mem_info->regmap, SUNXI_DMIC_FIFO_CTR,
			(1 << DMIC_FIFO_FLUSH), (1 << DMIC_FIFO_FLUSH));
		regmap_write(mem_info->regmap, SUNXI_DMIC_CNT, 0x1);
		mdelay(1);
	}

	return 0;
}

static int sunxi_dmic_startup(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	snd_soc_dai_set_dma_data(dai, substream,
				&sunxi_dmic->capture_dma_param);

	return 0;
}

static int sunxi_dmic_set_sysclk(struct snd_soc_dai *dai, int clk_id,
						unsigned int freq, int dir)
{

	struct snd_soc_component *component = dai->component;
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_component_get_drvdata(component);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_clk_info *clk_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}
	dts_info = &sunxi_dmic->dts_info;
	clk_info = &sunxi_dmic->clk_info;

	sunxi_dmic_init(sunxi_dmic);

	switch (clk_info->clk_parent) {
	case DMIC_CLK_PLL_AUDIO_X1:
		if (clk_set_rate(clk_info->clk_pll, freq)) {
			dev_err(dai->dev, "Freq : %u not support\n", freq);
			return -EINVAL;
		}
		break;
	default:
	case DMIC_CLK_PLL_AUDIO_X4:
		if (clk_set_rate(clk_info->clk_pll, freq * 4)) {
			dev_err(dai->dev, "Freq : %u not support\n", freq * 4);
			return -EINVAL;
		}
		break;
	}

	if (clk_set_rate(clk_info->clk_pll, freq)) {
		dev_err(dai->dev, "Freq : %u not support\n", freq);
		return -EINVAL;
	}

	return 0;
}

/* Dmic module init status */
static int sunxi_dmic_probe(struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	/* pcm_new will using the dma_param about the cma and fifo params. */
	snd_soc_dai_init_dma_data(dai, NULL, &sunxi_dmic->capture_dma_param);

	sunxi_dmic_init(sunxi_dmic);

	return 0;
}

static int sunxi_dmic_suspend(struct snd_soc_dai *cpu_dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_clk_info *clk_info = NULL;
	struct sunxi_dmic_pinctl_info *pin_info = NULL;
	u32 ret = 0;

	pr_alert("[DMIC]Enter %s\n", __func__);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(cpu_dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	dts_info = &sunxi_dmic->dts_info;
	clk_info = &sunxi_dmic->clk_info;
	pin_info = &sunxi_dmic->pin_info;

	ret = pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate_sleep);
	if (ret != 0) {
		dev_err(cpu_dai->dev, "[dmic] select pin sleep state failed\n");
		goto err_suspend_pinctl_select;
	}

	clk_disable_unprepare(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	clk_disable_unprepare(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	pr_alert("[DMIC]End %s\n", __func__);

	return 0;

err_suspend_pinctl_select:
	return ret;
}

static int sunxi_dmic_resume(struct snd_soc_dai *cpu_dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(cpu_dai);
	struct sunxi_dmic_mem_info *mem_info = NULL;
	struct sunxi_dmic_clk_info *clk_info = NULL;
	struct sunxi_dmic_pinctl_info *pin_info = NULL;
	s32 ret = 0;

	pr_alert("[DMIC]Enter %s\n", __func__);

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(cpu_dai->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	mem_info = &sunxi_dmic->mem_info;
	clk_info = &sunxi_dmic->clk_info;
	pin_info = &sunxi_dmic->pin_info;

	if (reset_control_deassert(clk_info->clk_rst)) {
		pr_err("dmic: resume deassert the dmic reset failed\n");
		return -EINVAL;
	}
	if (clk_prepare_enable(clk_info->clk_bus)) {
		dev_err(sunxi_dmic->dev, "enable dmic bus clk failed, resume exit\n");
		goto err_resume_clk_enable_bus;
	}
	if (clk_prepare_enable(clk_info->clk_pll)) {
		pr_err("enable clk_info->clk_pll failed!\n");
		ret = -EBUSY;
		goto err_resume_clk_enable_pll;
	}
	if (clk_prepare_enable(clk_info->clk_module)) {
		dev_err(cpu_dai->dev, "enable clk_info->clk_module failed!\n");
		ret = -EBUSY;
		goto err_resume_clk_enable_module;
	}

	ret = pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);
	if (ret != 0) {
		dev_err(cpu_dai->dev, "select pinstate failed!\n");
		goto err_resume_pinctl_select;
	}

	sunxi_dmic_init(sunxi_dmic);

	pr_debug("[DMIC]End %s\n", __func__);

	return 0;

err_resume_pinctl_select:
	clk_disable_unprepare(clk_info->clk_module);
err_resume_clk_enable_module:
	clk_disable_unprepare(clk_info->clk_pll);
err_resume_clk_enable_pll:
	clk_disable_unprepare(clk_info->clk_bus);
err_resume_clk_enable_bus:
	reset_control_assert(clk_info->clk_rst);
	return ret;
}

static void sunxi_dmic_shutdown(struct snd_pcm_substream *substream,
			struct snd_soc_dai *dai)
{
	struct sunxi_dmic_info *sunxi_dmic = snd_soc_dai_get_drvdata(dai);
	struct sunxi_dmic_clk_info *clk_info = NULL;

	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(dai->dev, "sunxi_dmic is NULL!\n");
		return;
	}
	clk_info = &sunxi_dmic->clk_info;
}

static struct snd_soc_dai_ops sunxi_dmic_dai_ops = {
	.startup = sunxi_dmic_startup,
	.set_sysclk = sunxi_dmic_set_sysclk,
	.hw_params = sunxi_dmic_hw_params,
	.prepare = sunxi_dmic_prepare,
	.trigger = sunxi_dmic_trigger,
	.shutdown = sunxi_dmic_shutdown,
};

static struct snd_soc_dai_driver sunxi_dmic_dai = {
	.probe = sunxi_dmic_probe,
	.suspend = sunxi_dmic_suspend,
	.resume = sunxi_dmic_resume,
	.capture = {
		.channels_min = 1,
		.channels_max = 8,
		.rates = SUNXI_DMIC_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops = &sunxi_dmic_dai_ops,
};

static const struct snd_soc_component_driver sunxi_dmic_component = {
	.name = DRV_NAME,
};

/*****************************************************************************/
static const struct regmap_config sunxi_dmic_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = SUNXI_DMIC_REG_MAX,
	.cache_type = REGCACHE_NONE,
};

static int sunxi_dmic_mem_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_dmic_info *sunxi_dmic)
{
	struct sunxi_dmic_mem_info *mem_info = &sunxi_dmic->mem_info;
	int ret = 0;

	ret = of_address_to_resource(np, 0, &mem_info->res);
	if (ret != 0) {
		dev_err(&pdev->dev, "Failed to get sunxi dmic resource\n");
		ret = -EINVAL;
		goto err_np_get_addr_res;
	}

	/* for get mem info */
	mem_info->memregion = devm_request_mem_region(&pdev->dev, mem_info->res.start,
					    resource_size(&mem_info->res), DRV_NAME);
	if (IS_ERR_OR_NULL(mem_info->memregion)) {
		dev_err(&pdev->dev, "sunxi dmic memory region already claimed\n");
		ret = -EBUSY;
		goto err_devm_request_mem;
	}

	/* Managed ioremap().  Map is automatically unmapped on driver detach. */
	mem_info->membase = devm_ioremap(&pdev->dev, mem_info->res.start,
					resource_size(&mem_info->res));
	if (IS_ERR_OR_NULL(mem_info->membase)) {
		dev_err(&pdev->dev, "sunxi dmic ioremap failed\n");
		ret = -ENOMEM;
		goto err_devm_ioremap;
	}
	mem_info->regmap = devm_regmap_init_mmio(&pdev->dev,
				mem_info->membase, &sunxi_dmic_regmap_config);
	if (IS_ERR_OR_NULL(mem_info->regmap)) {
		dev_err(&pdev->dev, "sunxi dmic registers regmap failed\n");
		ret = -ENOMEM;
		goto err_devm_regmap_init;
	}

	return 0;

err_devm_regmap_init:
err_devm_ioremap:
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));
err_devm_request_mem:
err_np_get_addr_res:
	devm_kfree(&pdev->dev, sunxi_dmic);
	return ret;
};

static int sunxi_dmic_clk_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_dmic_info *sunxi_dmic)
{
	struct sunxi_dmic_clk_info *clk_info = &sunxi_dmic->clk_info;
	unsigned int temp_val = 0;
	int ret = 0;

	ret = of_property_read_u32(np, "clk_parent", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "clk_parent config missing or invalid.\n");
		clk_info->clk_parent = DMIC_CLK_PLL_AUDIO_X1;
	} else {
		clk_info->clk_parent = temp_val;
	}

	/* to get clk info */
	clk_info->clk_pll = of_clk_get_by_name(np, "pll_audio");
	clk_info->clk_module = of_clk_get_by_name(np, "dmic");
	clk_info->clk_bus = of_clk_get_by_name(np, "dmic_bus");
	clk_info->clk_rst = devm_reset_control_get(&pdev->dev, NULL);

	if (clk_set_parent(clk_info->clk_module, clk_info->clk_pll)) {
		dev_err(&pdev->dev,
			"set parent of clk_module to clk_pll failed!\n");
		ret = -EINVAL;
		goto err_clk_set_parent;
	}

	if (reset_control_deassert(clk_info->clk_rst)) {
		pr_err("dmic: deassert the dmic reset failed\n");
		return -EINVAL;
	}
	if (clk_prepare_enable(clk_info->clk_bus)) {
		dev_err(&pdev->dev, "dmic clk bus enable failed\n");
		ret = -EBUSY;
		goto err_clk_enable_bus;
	}
	if (clk_prepare_enable(clk_info->clk_pll)) {
		dev_err(&pdev->dev, "clk_pll enable failed\n");
		ret = -EBUSY;
		goto err_clk_pll_enable;
	}
	if (clk_prepare_enable(clk_info->clk_module)) {
		dev_err(&pdev->dev, "clk_module enable failed\n");
		ret = -EBUSY;
		goto err_clk_module_enable;
	}

	return 0;

err_clk_module_enable:
	clk_disable_unprepare(clk_info->clk_pll);
err_clk_pll_enable:
err_clk_set_parent:
	clk_put(clk_info->clk_bus);
err_clk_enable_bus:
	reset_control_assert(clk_info->clk_rst);
	return ret;
};

static int sunxi_dmic_pinctrl_init(struct platform_device *pdev,
					struct sunxi_dmic_info *sunxi_dmic)
{
	struct sunxi_dmic_pinctl_info *pin_info = &sunxi_dmic->pin_info;
	int ret = 0;

	/* for get pin info */
	pin_info->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(pin_info->pinctrl)) {
		dev_err(&pdev->dev, "request pinctrl handle for audio failed\n");
		ret =  -EINVAL;
		goto err_devm_pin_get;
	}
	pin_info->pinstate = pinctrl_lookup_state(pin_info->pinctrl,
						PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(pin_info->pinstate)) {
		dev_err(&pdev->dev, "lookup pin default state failed\n");
		ret = -EINVAL;
		goto err_pinctrl_lookup_sate;
	}
	pin_info->pinstate_sleep = pinctrl_lookup_state(pin_info->pinctrl,
						PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(pin_info->pinstate_sleep)) {
		dev_err(&pdev->dev, "lookup pin sleep state failed\n");
		ret = -EINVAL;
		goto err_pinctrl_lookup_sleep;
	}
	/*only until this step , can the pinctrl system find pin conlict*/
	ret = pinctrl_select_state(pin_info->pinctrl, pin_info->pinstate);
	if (ret != 0) {
		dev_err(&pdev->dev, "pin select state failed\n");
		ret = -EINVAL;
		goto err_pinctrl_select_state;
	}

	return 0;

err_pinctrl_select_state:
err_pinctrl_lookup_sleep:
err_pinctrl_lookup_sate:
err_devm_pin_get:
	devm_pinctrl_put(pin_info->pinctrl);
	return ret;
};

static int sunxi_dmic_dts_params_init(struct platform_device *pdev,
		struct device_node *np, struct sunxi_dmic_info *sunxi_dmic)
{
	struct sunxi_dmic_dts_info *dts_info = &sunxi_dmic->dts_info;
	unsigned int temp_val = 0;
	int ret = 0;

	/* for get other dts info */
	ret = of_property_read_u32(np, "rx_chmap", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "rx_chmap config missing or invalid.\n");
		dts_info->rx_chmap = DMIC_CHANMAP_DEFAULT;
	} else {
		dts_info->rx_chmap = temp_val;
	}

	ret = of_property_read_u32(np, "data_vol", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "data_vol config missing or invalid.\n");
		dts_info->data_vol = DMIC_DEFAULT_VOL;
	} else {
		dts_info->data_vol = temp_val;
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

	ret = of_property_read_u32(np, "dmic_rxsync_en", &temp_val);
	if (ret != 0) {
		dev_warn(&pdev->dev, "dmic_rxsync_en config missing or invalid.\n");
		dts_info->dmic_rxsync_en = 0;
	} else {
		dts_info->dmic_rxsync_en = temp_val;
	}

	return 0;
};

static void sunxi_dmic_dma_params_init(struct resource res,
					struct sunxi_dmic_info *sunxi_dmic)
{
	struct sunxi_dmic_dts_info *dts_info = &sunxi_dmic->dts_info;

	sunxi_dmic->capture_dma_param.dma_addr = res.start + SUNXI_DMIC_DATA;
	sunxi_dmic->capture_dma_param.src_maxburst = 8;
	sunxi_dmic->capture_dma_param.dst_maxburst = 8;
	sunxi_dmic->capture_dma_param.cma_kbytes = dts_info->capture_cma;
	sunxi_dmic->capture_dma_param.fifo_size = DMIC_RX_FIFO_SIZE;
};

static int sunxi_dmic_dev_probe(struct platform_device *pdev)
{
	struct sunxi_dmic_info *sunxi_dmic = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	struct sunxi_dmic_clk_info *clk_info = NULL;
	struct sunxi_dmic_dts_info *dts_info = NULL;
	struct sunxi_dmic_pinctl_info *pin_info = NULL;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	sunxi_dmic = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_dmic_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(&pdev->dev, "sunxi_dmic allocate failed\n");
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, sunxi_dmic);

	sunxi_dmic->dev = &pdev->dev;
	sunxi_dmic->dai = sunxi_dmic_dai;
	sunxi_dmic->dai.name = dev_name(&pdev->dev);
	mem_info = &sunxi_dmic->mem_info;
	clk_info = &sunxi_dmic->clk_info;
	dts_info = &sunxi_dmic->dts_info;
	pin_info = &sunxi_dmic->pin_info;

	/* mem init about */
	ret = sunxi_dmic_mem_init(pdev, np, sunxi_dmic);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* clk init about */
	ret = sunxi_dmic_clk_init(pdev, np, sunxi_dmic);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* pinctrl init about */
	ret = sunxi_dmic_pinctrl_init(pdev, sunxi_dmic);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* dts params about */
	ret = sunxi_dmic_dts_params_init(pdev, np, sunxi_dmic);
	if (ret) {
		dev_err(&pdev->dev, "[%s] failed\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	/* dmic dma params about */
	sunxi_dmic_dma_params_init(sunxi_dmic->mem_info.res, sunxi_dmic);

	/*
	 * Register a component with automatic unregistration when the device is
	 * unregistered.
	 */
	ret = devm_snd_soc_register_component(&pdev->dev, &sunxi_dmic_component,
				   &sunxi_dmic->dai, 1);
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err_devm_register_component;
	}

	/*
	 * Register a platform driver with automatic unregistration when the device is
	 * unregistered.
	 */
	ret = asoc_dma_platform_register(&pdev->dev, 0);
	if (ret != 0) {
		dev_err(&pdev->dev, "Could not register PCM: %d\n", ret);
		ret = -ENOMEM;
		goto err_platform_register;
	}

#ifdef SUNXI_DMIC_DEBUG
	ret  = sysfs_create_group(&pdev->dev.kobj, &dmic_debug_attr_group);
	if (ret)
		dev_err(&pdev->dev, "failed to create attr group\n");
#endif

	return 0;

err_platform_register:
err_devm_register_component:
	devm_kfree(&pdev->dev, sunxi_dmic);
	return ret;
}

static int __exit sunxi_dmic_dev_remove(struct platform_device *pdev)
{
	struct sunxi_dmic_info *sunxi_dmic = NULL;
	struct sunxi_dmic_mem_info *mem_info = NULL;
	struct sunxi_dmic_clk_info *clk_info = NULL;
	struct sunxi_dmic_pinctl_info *pin_info = NULL;

	sunxi_dmic = platform_get_drvdata(pdev);
	if (IS_ERR_OR_NULL(sunxi_dmic)) {
		dev_err(&pdev->dev, "sunxi_dmic is NULL!\n");
		return -ENOMEM;
	}

	mem_info = &sunxi_dmic->mem_info;
	clk_info = &sunxi_dmic->clk_info;
	pin_info = &sunxi_dmic->pin_info;

#ifdef SUNXI_DMIC_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &dmic_debug_attr_group);
#endif

	clk_disable_unprepare(clk_info->clk_module);
	clk_put(clk_info->clk_module);
	clk_disable_unprepare(clk_info->clk_pll);
	clk_put(clk_info->clk_pll);
	clk_disable_unprepare(clk_info->clk_bus);
	clk_put(clk_info->clk_bus);
	reset_control_assert(clk_info->clk_rst);

	snd_soc_unregister_component(&pdev->dev);

	devm_pinctrl_put(pin_info->pinctrl);
	devm_release_mem_region(&pdev->dev, mem_info->memregion->start,
				resource_size(mem_info->memregion));

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id sunxi_dmic_of_match[] = {
	{ .compatible = "allwinner,sunxi-dmic", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_dmic_of_match);

static struct platform_driver sunxi_dmic_driver = {
	.probe = sunxi_dmic_dev_probe,
	.remove = __exit_p(sunxi_dmic_dev_remove),
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_dmic_of_match,
	},
};

static int __init sunxi_dmic_driver_init(void)
{
	return platform_driver_register(&sunxi_dmic_driver);
}
module_init(sunxi_dmic_driver_init);

static void __exit sunxi_dmic_driver_exit(void)
{
	platform_driver_unregister(&sunxi_dmic_driver);
}
module_exit(sunxi_dmic_driver_exit);

MODULE_AUTHOR("yumingfeng <yumingfeng@allwinnertech.com>");
MODULE_DESCRIPTION("SUNXI DMIC dmic ASoC driver");
MODULE_ALIAS("platform:sunxi-dmic");
MODULE_LICENSE("GPL");
