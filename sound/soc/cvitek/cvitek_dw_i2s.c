// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * I2S driver on CVITEK CV1835
 *
 * Copyright 2018 CVITEK
 *
 * Author: Rachel.jiang
 *
 */


#include <linux/clk.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <sound/cv1835_i2s.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "local.h"
#include "cvitek_dw_i2s.h"
#include "cv1835_i2s_subsys.h"

struct proc_dir_entry *proc_dwi2s_dir;
u32 dwi2s_mode_txrx;
u32 dei2s_mode_masterslave;

static inline void i2s_write_reg(void __iomem *io_base, int reg, u32 val)
{
	iowrite32(val, io_base + reg);
}

static inline u32 i2s_read_reg(void __iomem *io_base, int reg)
{
	return ioread32(io_base + reg);
}

static void dwi2s_debug(void __iomem *i2s_base)
{
	printk("DWI2S_DEBUG:\n");
	printk("DWI2S_EN:0x%x", i2s_read_reg(i2s_base, DWI2S_EN));
	printk("DWI2S_RX_BLOCK_EN:0x%x", i2s_read_reg(i2s_base, DWI2S_RX_BLOCK_EN));
	printk("DWI2S_TX_BLOCK_EN:0x%x", i2s_read_reg(i2s_base, DWI2S_TX_BLOCK_EN));
	printk("----\n");
	printk("DWI2S_CLK_EN:0x%x", i2s_read_reg(i2s_base, DWI2S_CLK_EN));
	printk("DWI2S_CLK_CFG:0x%x", i2s_read_reg(i2s_base, DWI2S_CLK_CFG));
	printk("DWI2S_RX_FIFO_RESET:0x%x", i2s_read_reg(i2s_base, DWI2S_RX_FIFO_RESET));
	printk("DWI2S_TX_FIFO_RESET:0x%x", i2s_read_reg(i2s_base, DWI2S_TX_FIFO_RESET));
	printk("----\n");

	printk("DWI2S_RX0_EN:0x%x", i2s_read_reg(i2s_base, DWI2S_RX0_EN));
	printk("DWI2S_TX0_EN:0x%x", i2s_read_reg(i2s_base, DWI2S_TX0_EN));
	printk("DWI2S_RX0_CFG_WLEN:0x%x", i2s_read_reg(i2s_base, DWI2S_RX0_CFG_WLEN));
	printk("DWI2S_TX0_CFG_WLEN:0x%x", i2s_read_reg(i2s_base, DWI2S_TX0_CFG_WLEN));
	printk("DWI2S_IRQ0_STATUS:0x%x", i2s_read_reg(i2s_base, DWI2S_IRQ0_STATUS));
	printk("DWI2S_IRQ0_MASK:0x%x", i2s_read_reg(i2s_base, DWI2S_IRQ0_MASK));
	printk("----\n");
	printk("DWI2S_RX0_OVRUN:0x%x", i2s_read_reg(i2s_base, DWI2S_RX0_OVRUN));
	printk("DWI2S_TX0_OVRUN:0x%x", i2s_read_reg(i2s_base, DWI2S_TX0_OVRUN));
	printk("DWI2S_RX0_CFG_FIFO_LEVEL:0x%x", i2s_read_reg(i2s_base, DWI2S_RX0_CFG_FIFO_LEVEL));
	printk("DWI2S_TX0_CFG_FIFO_LEVEL:0x%x", i2s_read_reg(i2s_base, DWI2S_TX0_CFG_FIFO_LEVEL));
	printk("DWI2S_RX0_FIFO_FLUSH:0x%x", i2s_read_reg(i2s_base, DWI2S_RX0_FIFO_FLUSH));
	printk("DWI2S_TX0_FIFO_FLUSH:0x%x", i2s_read_reg(i2s_base, DWI2S_TX0_FIFO_FLUSH));
	printk("==================================\n");
}


void dwi2s_ctrl(struct cvi_i2s_dev *dev, u32 stream, unsigned int on)
{
	u32 dmacr = i2s_read_reg(dev->i2s_base, DWI2S_DMA_CONTROL);
    if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dmacr = dmacr & ~DMAEN_TXBLOCK_MASK;
        if (on) {
			dmacr |= DMAEN_TXBLOCK;
            i2s_write_reg(dev->i2s_base, DWI2S_TX_BLOCK_EN, I2S_TX_ON);
            i2s_write_reg(dev->i2s_base, DWI2S_TX0_EN, I2S_TX_ON);
            i2s_write_reg(dev->i2s_base, DWI2S_TX1_EN, I2S_TX_OFF);

            printk("[%s][%d]tx enable\n", __func__, __LINE__);
        } else {
			dmacr |= DMADIS_TXBLOCK;
            i2s_write_reg(dev->i2s_base, DWI2S_TX_BLOCK_EN, I2S_TX_OFF);
            i2s_write_reg(dev->i2s_base, DWI2S_TX0_EN, I2S_TX_OFF);
            i2s_write_reg(dev->i2s_base, DWI2S_TX1_EN, I2S_TX_OFF);
            printk("[%s][%d]tx disable\n", __func__, __LINE__);
        }

    } else {
		dmacr = dmacr & ~DMAEN_RXBLOCK_MASK;
        if (on) {
			dmacr |= DMAEN_RXBLOCK;
            i2s_write_reg(dev->i2s_base, DWI2S_RX_BLOCK_EN, I2S_RX_ON);
            i2s_write_reg(dev->i2s_base, DWI2S_RX0_EN, I2S_RX_ON);
            i2s_write_reg(dev->i2s_base, DWI2S_RX1_EN, I2S_RX_OFF);

            printk("[%s][%d]rx enable\n", __func__, __LINE__);
        } else {
			dmacr |= DMADIS_RXBLOCK;
            i2s_write_reg(dev->i2s_base, DWI2S_RX_BLOCK_EN, I2S_RX_OFF);
            i2s_write_reg(dev->i2s_base, DWI2S_RX0_EN, I2S_RX_OFF);
            i2s_write_reg(dev->i2s_base, DWI2S_RX1_EN, I2S_RX_OFF);
            printk("[%s][%d]rx disable\n", __func__, __LINE__);
        }

    }
	i2s_write_reg(dev->i2s_base, DWI2S_DMA_CONTROL, dmacr);
	dev_info(dev->dev, "[%s]i2s_base:%px, dmacr:0x%x\n", __func__, dev->i2s_base, i2s_read_reg(dev->i2s_base, DWI2S_DMA_CONTROL));
	printk("888\n");
}

void dwi2s_switch(struct cvi_i2s_dev *dev, unsigned int on)
{
printk("[%s]rrbase:%px, 10DWI2S_EN:0x%x\n", __func__, dev->i2s_base, i2s_read_reg(dev->i2s_base, DWI2S_EN));
	if (on)
         i2s_write_reg(dev->i2s_base, DWI2S_EN, I2S_ON);
	 else
	 	i2s_write_reg(dev->i2s_base, DWI2S_EN, I2S_OFF);

printk("[%s]1DWI2S_EN:0x%x\n", __func__, i2s_read_reg(dev->i2s_base, DWI2S_EN));
}

void dwi2s_fifo_reset(struct cvi_i2s_dev *dev, u32 stream)
{
/* First:
dwi2s_ctrl(dev, SNDRV_PCM_STREAM_PLAYBACK, I2S_TX_OFF);
dwi2s_ctrl(dev, SNDRV_PCM_STREAM_CAPTURE, I2S_RX_OFF);
dwi2s_switch(dev, I2S_OFF);
*/
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
        i2s_write_reg(dev->i2s_base, DWI2S_TX_FIFO_RESET, DWI2S_FIFO_FLUSH_VAL);
        i2s_write_reg(dev->i2s_base, DWI2S_TX0_FIFO_FLUSH, DWI2S_FIFO_FLUSH_VAL);
        i2s_write_reg(dev->i2s_base, DWI2S_TX1_FIFO_FLUSH, DWI2S_FIFO_FLUSH_VAL);
        printk("[%s][%d]tx flush\n", __func__, __LINE__);

    } else {
        i2s_write_reg(dev->i2s_base, DWI2S_RX_FIFO_RESET, DWI2S_FIFO_FLUSH_VAL);
        i2s_write_reg(dev->i2s_base, DWI2S_RX0_FIFO_FLUSH, DWI2S_FIFO_FLUSH_VAL);
        i2s_write_reg(dev->i2s_base, DWI2S_RX1_FIFO_FLUSH, DWI2S_FIFO_FLUSH_VAL);
		printk("[%s][%d]rx flush\n", __func__, __LINE__);
    }

}

static inline void dwi2s_disable_irqs(struct cvi_i2s_dev *dev, u32 stream)
{
	u32 irq = i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_MASK);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		irq |= (DWI2S_INT_TXFE | DWI2S_INT_TXFO);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ0_MASK, irq);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ1_MASK, irq);
		printk("txdis irq:0x%x\n", irq);
	} else {
		irq |= (DWI2S_INT_RXDA | DWI2S_INT_RXFO);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ0_MASK, irq);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ1_MASK, irq);
		printk("rxdis irq:0x%x\n", irq);
	}
	dev_info(dev->dev, "[%s]irq0mask:0x%x, irq1mask:0x%x\n", __func__,
			i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_MASK),
			i2s_read_reg(dev->i2s_base, DWI2S_IRQ1_MASK));
}

static inline void dwi2s_enable_irqs(struct cvi_i2s_dev *dev, u32 stream)
{
	u32 irq = i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_MASK);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		irq &= ~(DWI2S_INT_TXFE | DWI2S_INT_TXFO);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ0_MASK, irq);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ1_MASK, irq);
	} else {
		irq &= (DWI2S_INT_RXDA | DWI2S_INT_RXFO);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ0_MASK, irq);
        i2s_write_reg(dev->i2s_base, DWI2S_IRQ1_MASK, irq);
	}
	dev_info(dev->dev, "[%s]irq0mask:0x%x, irq1mask:0x%x\n", __func__,
			i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_MASK),
			i2s_read_reg(dev->i2s_base, DWI2S_IRQ1_MASK));
}

static inline void dwi2s_clear_irqs(struct cvi_i2s_dev *dev, u32 stream)
{

	/* read rx/tx overrun reg to clean interrupt */
	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		i2s_read_reg(dev->i2s_base, DWI2S_TX1_OVRUN);
        i2s_read_reg(dev->i2s_base, DWI2S_TX0_OVRUN);
    } else {
		i2s_read_reg(dev->i2s_base, DWI2S_RX1_OVRUN);
        i2s_read_reg(dev->i2s_base, DWI2S_RX0_OVRUN);
    }

}

static irqreturn_t dwi2s_irq_handler(int irq, void *dev_id)
{
    struct cvi_i2s_dev *dev = dev_id;
    u32 val0 = i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_STATUS);
    u32 val1 = i2s_read_reg(dev->i2s_base, DWI2S_IRQ1_STATUS);

	//dwi2s_debug(dev->i2s_base);

	if (dev->active >= 1) { /* If I2S is really active */
        if ((val0 & DWI2S_INT_RXFO) & (val1 & DWI2S_INT_RXFO)) {
            dev_info(dev->dev, "WARNING!!! I2S RX FIFO exception occur int_status=0x%x, 0x%x\n", val0, val1);
            dwi2s_ctrl(dev, SNDRV_PCM_STREAM_CAPTURE, I2S_RX_OFF);
            dwi2s_switch(dev, I2S_OFF);
            dwi2s_fifo_reset(dev, SNDRV_PCM_STREAM_CAPTURE);

            dwi2s_ctrl(dev, SNDRV_PCM_STREAM_CAPTURE, I2S_RX_ON);
            dwi2s_switch(dev, I2S_ON);

        } else if ((val0 & DWI2S_INT_TXFO) & (val1 & DWI2S_INT_TXFO)) {
            dev_info(dev->dev, "WARNING!!! I2S TX FIFO exception occur int_status=0x%x, 0x%x\n", val0, val1);
            dwi2s_ctrl(dev, SNDRV_PCM_STREAM_PLAYBACK, I2S_TX_OFF);
            dwi2s_switch(dev, I2S_OFF);
            dwi2s_fifo_reset(dev, SNDRV_PCM_STREAM_PLAYBACK);

            dwi2s_ctrl(dev, SNDRV_PCM_STREAM_PLAYBACK, I2S_TX_ON);
            dwi2s_switch(dev, I2S_ON);

        }
    }

	dwi2s_clear_irqs(dev, SNDRV_PCM_STREAM_PLAYBACK);
	dwi2s_clear_irqs(dev, SNDRV_PCM_STREAM_CAPTURE);
	//dwi2s_disable_irqs(dev, SNDRV_PCM_STREAM_PLAYBACK);
	//dwi2s_disable_irqs(dev, SNDRV_PCM_STREAM_CAPTURE);

    return IRQ_HANDLED;
}

static void dwi2s_start(struct cvi_i2s_dev *dev,
		      struct snd_pcm_substream *substream)
{

	dwi2s_clear_irqs(dev, substream->stream);
	dwi2s_disable_irqs(dev, substream->stream);

	dwi2s_fifo_reset(dev, substream->stream);

	dwi2s_ctrl(dev, SNDRV_PCM_STREAM_PLAYBACK, false);
	dwi2s_ctrl(dev, SNDRV_PCM_STREAM_CAPTURE, false);

	dwi2s_ctrl(dev, substream->stream, true);
	dev_info(dev->dev,"[%s]0", __func__);
	dwi2s_switch(dev, true);

	dev_info(dev->dev,"[%s]1", __func__);
}

static void dwi2s_stop(struct cvi_i2s_dev *dev,
		     struct snd_pcm_substream *substream)
{
	dwi2s_clear_irqs(dev, substream->stream);
	dwi2s_disable_irqs(dev, substream->stream);

	dwi2s_ctrl(dev, substream->stream, false);
	dwi2s_switch(dev, false);
	dwi2s_fifo_reset(dev, substream->stream);

	dev_info(dev->dev,"test[%s]", __func__);
}

#ifdef CONFIG_PM
static int cvi_dwi2s_runtime_suspend(struct device *dev)
{
	struct cvi_i2s_dev *cvi_dev = dev_get_drvdata(dev);

	if (cvi_dev->capability & CVI_I2S_MASTER)
		clk_disable(cvi_dev->clk);
	return 0;
}

static int cvi_dwi2s_runtime_resume(struct device *dev)
{
	struct cvi_i2s_dev *cvi_dev = dev_get_drvdata(dev);

	if (cvi_dev->capability & CVI_I2S_MASTER)
		clk_enable(cvi_dev->clk);
	return 0;
}
#endif

static int cvi_dwi2s_suspend(struct snd_soc_dai *dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);

	if (dev->capability & CVI_I2S_MASTER)
		clk_disable(dev->clk);
	return 0;
}

static int cvi_dwi2s_resume(struct snd_soc_dai *dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);

	if (dev->capability & CVI_I2S_MASTER)
		clk_enable(dev->clk);

	return 0;
}

static int cvi_dwi2s_startup(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *cpu_dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
	union cvi_i2s_snd_dma_data *dma_data = NULL;

	dev_info(dev->dev, "%s start *cpu_dai = %p name = %s\n", __func__, cpu_dai, cpu_dai->name);
	if (!(dev->capability & CVI_I2S_RECORD) &&
	    (substream->stream == SNDRV_PCM_STREAM_CAPTURE)) {
		dev_info(dev->dev, "%s return -EINVAL;\n", __func__);
		return -EINVAL;
	}

	if (!(dev->capability & CVI_I2S_PLAY) &&
	    (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)) {
		dev_info(dev->dev, "%s return -EINVAL; 2\n", __func__);
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &dev->play_dma_data;
	else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
		dma_data = &dev->capture_dma_data;
	if (dma_data == NULL) {
		dev_info(dev->dev, "%s dma_data == NULL\n", __func__);
	}

	dev_info(dev->dev, "%s start *dma_data = %px\n", __func__, dma_data);
	snd_soc_dai_set_dma_data(cpu_dai, substream, (void *)dma_data);
	dev_info(dev->dev, "%s end cpu_dai->playback_dma_data = %p\n",
		__func__, cpu_dai->playback_dma_data);
	return 0;
}


static void cvi_dwi2s_shutdown(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{
	pr_info("%s not start *dai = %px, *dai->playback_dma_data = %px\n", __func__, dai, dai->playback_dma_data);
	//snd_soc_dai_set_dma_data(dai, substream, NULL);
}

static int cvi_dwi2s_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);
	struct i2s_clk_config_data *config = &dev->config;
    u32 clk_cfg_ws = 0;
	u32 clk_ctrl1 = 0;
	u32 audio_clk = 0;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	config->chan_nr = params_channels(params);
	dev_info(dev->dev, "[%s]\n", __func__);

	switch (params_format(params)) {
        case SNDRV_PCM_FORMAT_S16_LE:
        case SNDRV_PCM_FORMAT_U16_LE:
            config->data_size = 16;
            dev->wss = WSS_16_CLKCYCLE;
            clk_cfg_ws |= WSS_SET(0x0);
            break;
        case SNDRV_PCM_FORMAT_S24_LE:
        case SNDRV_PCM_FORMAT_U24_LE:
        case SNDRV_PCM_FORMAT_S24_3LE:
        case SNDRV_PCM_FORMAT_U24_3LE:
            config->data_size = 24;
            dev->wss = WSS_32_CLKCYCLE;
            clk_cfg_ws |= WSS_SET(0x1);
            break;
        case SNDRV_PCM_FORMAT_S32_LE:
        case SNDRV_PCM_FORMAT_U32_LE:
            config->data_size = 32;
            dev->wss = WSS_32_CLKCYCLE;
            clk_cfg_ws |= WSS_SET(0x2);
            break;
        default:
            dev_err(dev->dev, "CVI-i2s: unsupported PCM fmt\n");
            return -EINVAL;
    }
/* program the gating of sclk. The programmed gating value must be
less than or equal to the largest configured/programmed audio resolution*/
    clk_cfg_ws |= SCLKG_SET(0x0);
 	i2s_write_reg(dev->i2s_base, DWI2S_CLK_EN, I2S_OFF);
	i2s_write_reg(dev->i2s_base, DWI2S_CLK_CFG, clk_cfg_ws);
	i2s_write_reg(dev->i2s_base, DWI2S_CLK_EN, I2S_ON);

/* Make sure the data to the DDR is high aligned*/
 	i2s_write_reg(dev->i2s_base, DWI2S_RX0_CFG_WLEN, DW_32_RESOLUTION);
	i2s_write_reg(dev->i2s_base, DWI2S_TX0_CFG_WLEN, DW_32_RESOLUTION);
	i2s_write_reg(dev->i2s_base, DWI2S_RX1_CFG_WLEN, DW_32_RESOLUTION);
	i2s_write_reg(dev->i2s_base, DWI2S_TX1_CFG_WLEN, DW_32_RESOLUTION);
/*dwi2s_fifo size:16 * 4chn * 4byte*/
 	i2s_write_reg(dev->i2s_base, DWI2S_RX0_CFG_FIFO_LEVEL, DWI2S_DEFAULT_FIFO_LEVEL);
	i2s_write_reg(dev->i2s_base, DWI2S_TX0_CFG_FIFO_LEVEL, DWI2S_DEFAULT_FIFO_LEVEL);
	i2s_write_reg(dev->i2s_base, DWI2S_RX1_CFG_FIFO_LEVEL, DWI2S_DEFAULT_FIFO_LEVEL);
	i2s_write_reg(dev->i2s_base, DWI2S_TX1_CFG_FIFO_LEVEL, DWI2S_DEFAULT_FIFO_LEVEL);


/*config audio clk and mclk_div*/
	config->sample_rate = params_rate(params);
#if 0
	switch (config->sample_rate) {
	case 11025:
	case 22050:
	case 44100:
		audio_clk = CVI_22579_MHZ;
		break;
	case 8000:
	case 16000:
	case 32000:
			audio_clk = CVI_16384_MHZ;
		break;
	case 12000:
	case 24000:
	case 48000:
	case 96000:
		audio_clk = CVI_24576_MHZ;
		break;
	default:
		dev_err(dev->dev, "Warning!!! this sample rate is not supported\n");
		return -1;
	}

    dev_info(dev->dev, "Audio system clk=%d, sample rate=%d\n", audio_clk, config->sample_rate);
    cv1835_set_mclk(audio_clk);

    switch (config->sample_rate) {
    case 8000:
    case 16000:
    case 32000:
        /* apll is 16.384Mhz, no need to divide */
        clk_ctrl1 |= MCLK_DIV(1);
        mclk_div = 1;
        break;
    case 11025:
    case 96000:
    case 22050:
	case 24000:
    case 44100:
    case 48000:
        clk_ctrl1 |= MCLK_DIV(2);
        mclk_div = 2;
        break;
    default:
        dev_err(dev->dev, "%s doesn't support this sample rate\n", __func__);
        break;
    }

/* Configure I2S word length,  bclk_div and sync_div here*/

	switch (dev->wss) {
	case (WSS_32_CLKCYCLE):
        bclk_div = (audio_clk / 1000) / (WSS_32_CLKCYCLE * (config->sample_rate / 1000) * mclk_div);
		break;
	case (WSS_24_CLKCYCLE):
        bclk_div = (audio_clk / 1000) / (WSS_24_CLKCYCLE * (config->sample_rate / 1000) * mclk_div);
        break;
	case (WSS_16_CLKCYCLE):
        bclk_div = (audio_clk / 1000) / (WSS_16_CLKCYCLE * (config->sample_rate / 1000) * mclk_div);
        break;
	default:
		dev_err(dev->dev, "resolution not supported\n");
	}

    clk_ctrl1 |= BCLK_DIV(bclk_div);

#else
/*because of i2s work clk is 24576khz in fpga*/
audio_clk = CVI_24576_MHZ;
clk_ctrl1 |= MCLK_DIV(2);
mclk_div = 2;
bclk_div = (audio_clk / 1000) / (WSS_32_CLKCYCLE * (config->sample_rate / 1000) * mclk_div);
clk_ctrl1 |= BCLK_DIV(bclk_div);

#endif

    dev_info(dev->dev, "Set clock ctrl1=0x%08x, mclk_div:%d bclk_div:%d\n", clk_ctrl1, mclk_div, bclk_div);
	/* master mode*/
    dwi2s_set_mclk(DWI2S_MODE_REG_VAL, DWI2S_SLAVEMODE_SOURCE, DWI2S_CLK_CTRL0_VAL, clk_ctrl1);
    dwi2s_get_subsys();

	dev_info(dev->dev, "[dwi2s] clken=0x%08x, clk_cfg=0x%08x, cfg_wlen=0x%08x, cfg_fifo_len=0x%08x\n",
		i2s_read_reg(dev->i2s_base, DWI2S_CLK_EN),
		i2s_read_reg(dev->i2s_base, DWI2S_CLK_CFG),
		i2s_read_reg(dev->i2s_base, DWI2S_RX0_CFG_WLEN),
		i2s_read_reg(dev->i2s_base, DWI2S_TX1_CFG_FIFO_LEVEL));

	return 0;
}


static int cvi_dwi2s_prepare(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dwi2s_mode_txrx |= TX_MODE;
	else
		dwi2s_mode_txrx |= RX_MODE;

	dev_info(dev->dev, "[%s]dwi2s_mode_txrx:%d\n", __func__, dwi2s_mode_txrx);
	return 0;
}

static int cvi_dwi2s_trigger(struct snd_pcm_substream *substream,
			   int cmd, struct snd_soc_dai *dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	dev_info(dev->dev, "[%s]cmd:%d\n", __func__, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		snd_pcm_stream_unlock_irq(substream);
		dev->active++;
		cvi_dwi2s_resume(dai);
		dwi2s_start(dev, substream);
dev_info(dev->dev, "[%s]0start_cmd:%d\n", __func__, cmd);
		snd_pcm_stream_lock_irq(substream);
dev_info(dev->dev, "[%s]1start_cmd:%d\n", __func__, cmd);
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
//		snd_pcm_stream_unlock_irq(substream);
		dev->active--;
		dwi2s_stop(dev, substream);
		cvi_dwi2s_suspend(dai);
dev_info(dev->dev, "[%s]end_cmd:%d\n", __func__, cmd);
//		snd_pcm_stream_lock_irq(substream);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


static int cvi_dwi2s_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);
	int ret = 0;

	dev_info(dev->dev, "%s, fmt=0x%08x\n", __func__, fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM: /* Set codec to Master mode, so I2S IP need to be Slave mode */
		dei2s_mode_masterslave |= SLAVE_MODE;
		dev->role = SLAVE_MODE;
		break;
	case SND_SOC_DAIFMT_CBS_CFS: /* Set codec to Slave mode, so I2S IP need to be Master mode */
		dei2s_mode_masterslave |= MASTER_MODE;
		dev->role = MASTER_MODE;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
	case SND_SOC_DAIFMT_CBS_CFM:
		ret = -EINVAL;
		break;
	default:
		dev_info(dev->dev, "cvitek : Invalid master/slave format\n");
		ret = -EINVAL;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		dev->sample_edge = FMT_NB_NF;
		break;
	case SND_SOC_DAIFMT_NB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	case SND_SOC_DAIFMT_IB_IF:
		dev->sample_edge = FMT_NB_NF;
		dev_err(dev->dev, "cvitek : only support FMT_NB_NF\n");
		break;
	default:
		dev_info(dev->dev, "cvitek : Invalid frame format\n");
		ret = -EINVAL;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_PDM:
		dev->mode = SND_SOC_DAIFMT_I2S;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		dev->mode = SND_SOC_DAIFMT_I2S;
		dev_err(dev->dev, "cvitek : only support daifmt_i2s\n");
		break;
	default:
		dev_info(dev->dev, "cvitek : Invalid I2S mode\n");
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int cvi_dwi2s_set_tdm_slot(struct snd_soc_dai *cpu_dai, unsigned int tx_mask,
				unsigned int rx_mask, int slots, int width)
{
	printk("[%s]\n", __func__);
	return 0;
}




static int cvi_configure_dai(struct cvi_i2s_dev *dev,
			     struct snd_soc_dai_driver *cvi_i2s_dai,
			     unsigned int rates)
{

	struct device_node *np = dev->dev->of_node;
	const char *capability;

	if (of_property_read_string(np, "capability", &capability) < 0)
		return -EINVAL;


	if ((!strcmp(capability, "tx")) || (!strcmp(capability, "txrx"))) {
		dev_info(dev->dev, "CV: playback support\n");
		cvi_i2s_dai->playback.channels_min = 1;//4
		cvi_i2s_dai->playback.channels_max = 8;//4
		cvi_i2s_dai->playback.formats = SNDRV_PCM_FMTBIT_S32_LE
						| SNDRV_PCM_FMTBIT_S24_LE
						| SNDRV_PCM_FORMAT_S24_3LE
						| SNDRV_PCM_FMTBIT_U24_LE
						| SNDRV_PCM_FORMAT_U24_3LE
						| SNDRV_PCM_FMTBIT_S16_LE;
		cvi_i2s_dai->playback.rates = rates;
	} else {
		/* this device doesn't have playback capability */
		dev_info(dev->dev, "CV: playback not support\n");
		cvi_i2s_dai->playback.channels_min = 0;
		cvi_i2s_dai->playback.channels_max = 0;
	}

	if ((!strcmp(capability, "rx")) || (!strcmp(capability, "txrx"))) {
		dev_info(dev->dev, "CV: capature support\n");
		cvi_i2s_dai->capture.channels_min = 1;//4
		cvi_i2s_dai->capture.channels_max = 8;//4
		cvi_i2s_dai->capture.formats = SNDRV_PCM_FMTBIT_S32_LE
					       | SNDRV_PCM_FMTBIT_S24_LE
					       | SNDRV_PCM_FMTBIT_S16_LE;
		cvi_i2s_dai->capture.rates = rates;
	} else {
		/* this device doesn't have capature capability */
		dev_info(dev->dev, "CV: capature not support\n");
		cvi_i2s_dai->capture.channels_min = 0;
		cvi_i2s_dai->capture.channels_max = 0;
	}

	dev_info(dev->dev, "CV: i2s master/slave mode supported\n");
	dev->capability |= CVI_I2S_MASTER | CVI_I2S_SLAVE;

	dev->fifo_th = I2STDM_FIFO_DEPTH / 2;
	return 0;
}

static int cvi_configure_dai_by_dt(struct cvi_i2s_dev *dev,
				   struct snd_soc_dai_driver *cvi_i2s_dai,
				   struct resource *res)
{
	int ret;
	struct device_node *np = dev->dev->of_node;

	dev_info(dev->dev, "%s start\n", __func__);
	ret = cvi_configure_dai(dev, cvi_i2s_dai, SNDRV_PCM_RATE_8000_192000);
	if (ret < 0)
		return ret;

	/* Set TX parameters */
	if (of_property_match_string(np, "dma-names", "tx") >= 0) {
		dev_info(dev->dev, "%s dma-names  tx\n", __func__);
		dev->capability |= CVI_I2S_PLAY;
		dev->play_dma_data.dt.addr = res->start + DWI2S_TX_BLOCK_DMA;
		dev->play_dma_data.dt.addr_width = 4;
		dev->play_dma_data.dt.fifo_size = I2STDM_FIFO_DEPTH * I2STDM_FIFO_WIDTH;//fifo_level
		dev->play_dma_data.dt.maxburst = 8;
	}

	/* Set RX parameters */
	if (of_property_match_string(np, "dma-names", "rx") >= 0) {
		dev_info(dev->dev, "%s dma-names  rx\n", __func__);
		dev->capability |= CVI_I2S_RECORD;
		dev->capture_dma_data.dt.addr = res->start + DWI2S_RX_BLOCK_DMA;
		dev->capture_dma_data.dt.addr_width = 4;
		dev->capture_dma_data.dt.fifo_size = I2STDM_FIFO_DEPTH * I2STDM_FIFO_WIDTH;
		dev->capture_dma_data.dt.maxburst = 8;
	}

	return 0;

}

static int cvi_dw_i2s_dai_probe(struct snd_soc_dai *cpu_dai)
{
	struct cvi_i2s_dev *dev = snd_soc_dai_get_drvdata(cpu_dai);

	dev_info(cpu_dai->dev, "%s start *cpu_dai = %p name = %s\n", __func__, cpu_dai, cpu_dai->name);
	cpu_dai->playback_dma_data = &dev->play_dma_data;
	cpu_dai->capture_dma_data = &dev->capture_dma_data;

	if (cpu_dai->playback_dma_data == NULL) {
		dev_err(cpu_dai->dev, "%s playback_dma_data == NULL\n", __func__);
	}

	if (cpu_dai->capture_dma_data == NULL) {
		dev_err(cpu_dai->dev, "%s capture_dma_data == NULL\n", __func__);
	}

	dev_info(cpu_dai->dev, "%s end cpu_dai->playback_dma_data = %p\n", __func__, cpu_dai->playback_dma_data);
    dev_info(cpu_dai->dev, "%s end cpu_dai->capture_dma_data = %p\n", __func__, cpu_dai->capture_dma_data);
	return 0;

}

static int i2s_proc_show(struct seq_file *m, void *v)
{
	struct cvi_i2s_dev *dev = m->private;

	if (i2s_read_reg(dev->i2s_base, DWI2S_EN))
		seq_printf(m, "\ni2s%d is enabled\n", dev->dev_id);
	else
		seq_printf(m, "\ni2s%d is disabled\n", dev->dev_id);

	seq_printf(m, "\n===== Dump I2S%d register status =====\n", dev->dev_id);

	seq_printf(m, "\nrx_block_en=0x%08x,          rx0=0x%08x,          rx1=0x%08x\n",
		   i2s_read_reg(dev->i2s_base, DWI2S_RX_BLOCK_EN),
		   i2s_read_reg(dev->i2s_base, DWI2S_RX0_EN),
		   i2s_read_reg(dev->i2s_base, DWI2S_RX1_EN));

	seq_printf(m, "\ntx_block_en=0x%08x,          tx0=0x%08x,          tx1=0x%08x\n",
		   i2s_read_reg(dev->i2s_base, DWI2S_TX_BLOCK_EN),
		   i2s_read_reg(dev->i2s_base, DWI2S_TX0_EN),
		   i2s_read_reg(dev->i2s_base, DWI2S_TX1_EN));

	seq_printf(m, "\nrxfifo_reset=0x%08x,     rx0_cfg_wlen=0x%08x,      rx0_cfg_fifolevel=0x%08x\n",
		   i2s_read_reg(dev->i2s_base, DWI2S_RX_FIFO_RESET),
		   i2s_read_reg(dev->i2s_base, DWI2S_RX0_CFG_WLEN),
		   i2s_read_reg(dev->i2s_base, DWI2S_RX0_CFG_FIFO_LEVEL));

	seq_printf(m, "\ntxfifo_reset=0x%08x,     tx0_cfg_wlen=0x%08x,      tx0_cfg_fifolevel=0x%08x\n",
		   i2s_read_reg(dev->i2s_base, DWI2S_TX_FIFO_RESET),
		   i2s_read_reg(dev->i2s_base, DWI2S_TX0_CFG_WLEN),
		   i2s_read_reg(dev->i2s_base, DWI2S_TX0_CFG_FIFO_LEVEL));

	seq_printf(m, "\nclk_en=0x%08x,           clk_cfg=0x%08x\n",
		   i2s_read_reg(dev->i2s_base, DWI2S_CLK_EN),
		   i2s_read_reg(dev->i2s_base, DWI2S_CLK_CFG));

	seq_printf(m, "\nirq0_mask=0x%08x,         irq_status=0x%08x\n",
		   i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_MASK),
		   i2s_read_reg(dev->i2s_base, DWI2S_IRQ0_STATUS));

	return 0;
}

static int seq_i2s_open(struct inode *inode, struct file *file)
{
	return single_open(file, i2s_proc_show, PDE_DATA(inode));
}

static const struct proc_ops i2s_proc_ops = {
	.proc_read	= seq_read,
	.proc_open	= seq_i2s_open,
	.proc_release	= single_release,
};


static const struct snd_soc_component_driver cvi_dw_i2s_component = {
	.name		= "cvitek-dw-i2s",
};

static struct snd_soc_dai_ops cvi_dw_i2s_dai_ops = {
	.startup	= cvi_dwi2s_startup,
	.shutdown	= cvi_dwi2s_shutdown,
	.hw_params	= cvi_dwi2s_hw_params,
	.prepare	= cvi_dwi2s_prepare,
	.trigger	= cvi_dwi2s_trigger,
	.set_fmt	= cvi_dwi2s_set_fmt,
	.set_tdm_slot = cvi_dwi2s_set_tdm_slot,
};

static int cvi_dw_i2s_probe(struct platform_device *pdev)
{
    const struct i2s_platform_data *pdata = pdev->dev.platform_data;
	struct cvi_i2s_dev *dev;
	struct snd_soc_dai_driver *cvi_i2s_dai;
    struct resource *res;
    const char *clk_id;
    int ret, irq;
    char *i2s_dev_name;
    struct proc_dir_entry *proc_i2s;
	const char *mclk_out;

	dev_info(&pdev->dev, "%s\n", __func__);

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	cvi_i2s_dai = devm_kzalloc(&pdev->dev, sizeof(*cvi_i2s_dai), GFP_KERNEL);
	if (!cvi_i2s_dai)
		return -ENOMEM;

	//for kernel version witch is less than 5.10.4
	//cvi_i2s_dai->suspend = cvi_dwi2s_suspend;
	//cvi_i2s_dai->resume = cvi_dwi2s_resume;
	cvi_i2s_dai->ops = &cvi_dw_i2s_dai_ops;
	cvi_i2s_dai->name = "cvi_dw_i2s_probe";
    cvi_i2s_dai->probe = cvi_dw_i2s_dai_probe;


	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->i2s_base = devm_ioremap_resource(&pdev->dev, res);
	dev_info(&pdev->dev, "I2S get i2s_base=0x%x\n", readl(dev->i2s_base));
	if (IS_ERR(dev->i2s_base))
		return PTR_ERR(dev->i2s_base);

    dev->dev = &pdev->dev;

	irq = platform_get_irq(pdev, 0);
	if (irq >= 0) {
		dev_info(&pdev->dev, "I2S get IRQ=0x%x\n", irq);
		ret = devm_request_irq(&pdev->dev, irq, dwi2s_irq_handler, 0,
				       pdev->name, dev);
		if (ret < 0) {
			dev_err(&pdev->dev, "failed to request irq\n");
			return ret;
		}
	}

	if (pdata) {
		dev->capability = pdata->cap;
		clk_id = NULL;
		dev->quirks = pdata->quirks;
	} else {
		clk_id = "i2sclk";
		ret = cvi_configure_dai_by_dt(dev, cvi_i2s_dai, res);
		device_property_read_u32(&pdev->dev, "dev-id",
					 &dev->dev_id);
		dev->clk = devm_clk_get(&pdev->dev, clk_id);
        if (ret < 0)
            return ret;
	}

	if (dev->capability & CVI_I2S_MASTER) {
		if (pdata) {
			dev->i2s_clk_cfg = pdata->i2s_clk_cfg;
			if (!dev->i2s_clk_cfg) {
				dev_err(&pdev->dev, "no clock configure method\n");
				return -ENODEV;
			}
		}
		dev->clk = devm_clk_get(&pdev->dev, clk_id);

		if (IS_ERR(dev->clk))
			return PTR_ERR(dev->clk);

		ret = clk_prepare_enable(dev->clk);
		if (ret < 0) {
			dev_err(&pdev->dev, "I2S clock prepare failed\n");
			return ret;
		}
	}
	device_property_read_string(&pdev->dev, "mclk_out", &mclk_out);

	if (!strcmp(mclk_out, "true"))
		dev->mclk_out = true;
	else
		dev->mclk_out = false;

//i2s subsys set I2S_CLK_CTRL0

 	dev_set_drvdata(&pdev->dev, dev);
	ret = devm_snd_soc_register_component(&pdev->dev, &cvi_dw_i2s_component,
					      cvi_i2s_dai, 1);
	if (ret != 0) {
		dev_err(&pdev->dev, "not able to register dai\n");
		goto dwerr_clk_disable;
	}

	if (!pdata) {
		ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);

		if (ret == -EPROBE_DEFER) {
			dev_err(&pdev->dev,
				"failed to register PCM, deferring probe\n");
			return ret;
		} else if (ret) {
			dev_err(&pdev->dev,
				"Could not register DMA PCM: %d\n"
				"falling back to PIO mode\n", ret);
			ret = cvi_pcm_register(pdev);
			if (ret) {
				dev_err(&pdev->dev,
					"Could not register PIO PCM: %d\n",
					ret);
				goto dwerr_clk_disable;
			}
		}
	}

    if (!proc_dwi2s_dir) {
		proc_dwi2s_dir = proc_mkdir("auddwi2s_debug", NULL);
		if (!proc_dwi2s_dir)
			dev_err(&pdev->dev, "Error creating auddwi2s_debug proc folder entry\n");
    }


	if (proc_dwi2s_dir) {
		i2s_dev_name = devm_kasprintf(&pdev->dev, GFP_KERNEL, "i2s%d", dev->dev_id);
		proc_i2s = proc_create_data(i2s_dev_name, 0664, proc_dwi2s_dir, &i2s_proc_ops, dev);
		if (!proc_i2s)
			dev_err(&pdev->dev, "Create i2s%d status proc failed!\n", dev->dev_id);
		devm_kfree(&pdev->dev, i2s_dev_name);
	}

	pm_runtime_enable(&pdev->dev);

	return 0;
dwerr_clk_disable:
	if (dev->capability & CVI_I2S_MASTER)
		clk_disable_unprepare(dev->clk);
	return -1;
};


static int cvi_dw_i2s_remove(struct platform_device *pdev)
{
	struct cvi_i2s_dev *dev = dev_get_drvdata(&pdev->dev);

	if (dev->capability & CVI_I2S_MASTER)
		clk_disable_unprepare(dev->clk);

	pm_runtime_disable(&pdev->dev);
	return 0;
};

#ifdef CONFIG_OF
static const struct of_device_id cvi_dwi2s_of_match[] = {
	{ .compatible = "cvitek,cvitek-dwi2s", },
	{},
};

MODULE_DEVICE_TABLE(of, cvi_dwi2s_of_match);
#endif

#ifdef CONFIG_PM_SLEEP
static int cvi_dwi2s_pm_suspend(struct device *dev)
{
	struct cvi_i2s_dev *i2s_dev = dev_get_drvdata(dev);

	if (!i2s_dev->reg_dwctx) {
		i2s_dev->reg_dwctx = devm_kzalloc(i2s_dev->dev, sizeof(struct cvi_dwi2s_reg_context), GFP_KERNEL);
		if (!i2s_dev->reg_dwctx)
			return -ENOMEM;
	}

	i2s_dev->reg_dwctx->i2s_dwapb_ier = i2s_read_reg(i2s_dev->i2s_base, DWI2S_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_irer = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX_BLOCK_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_iter = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX_BLOCK_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_cer = i2s_read_reg(i2s_dev->i2s_base, DWI2S_CLK_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_ccr = i2s_read_reg(i2s_dev->i2s_base, DWI2S_CLK_CFG);

	i2s_dev->reg_dwctx->i2s_dwapb_rer0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX0_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_ter0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX0_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_rcr0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX0_CFG_WLEN);
	i2s_dev->reg_dwctx->i2s_dwapb_tcr0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX0_CFG_WLEN);

	i2s_dev->reg_dwctx->i2s_dwapb_rfcr0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX0_CFG_FIFO_LEVEL);
	i2s_dev->reg_dwctx->i2s_dwapb_tfcr0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX0_CFG_FIFO_LEVEL);
	i2s_dev->reg_dwctx->i2s_dwapb_rff0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX0_FIFO_FLUSH);
	i2s_dev->reg_dwctx->i2s_dwapb_tff0 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX0_FIFO_FLUSH);

	i2s_dev->reg_dwctx->i2s_dwapb_rer1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX1_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_ter1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX1_EN);
	i2s_dev->reg_dwctx->i2s_dwapb_rcr1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX1_CFG_WLEN);
	i2s_dev->reg_dwctx->i2s_dwapb_tcr1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX1_CFG_WLEN);

	i2s_dev->reg_dwctx->i2s_dwapb_rfcr1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX1_CFG_FIFO_LEVEL);
	i2s_dev->reg_dwctx->i2s_dwapb_tfcr1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX1_CFG_FIFO_LEVEL);
	i2s_dev->reg_dwctx->i2s_dwapb_rff1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_RX1_FIFO_FLUSH);
	i2s_dev->reg_dwctx->i2s_dwapb_tff1 = i2s_read_reg(i2s_dev->i2s_base, DWI2S_TX1_FIFO_FLUSH);

	i2s_dev->reg_dwctx->i2s_dwapb_dmacr = i2s_read_reg(i2s_dev->i2s_base, DWI2S_DMA_CONTROL);

	if (i2s_dev->capability & CVI_I2S_MASTER)
		clk_disable(i2s_dev->clk);

	return 0;
}

static int cvi_dwi2s_pm_resume(struct device *dev)
{
	struct cvi_i2s_dev *i2s_dev = dev_get_drvdata(dev);

	if (i2s_dev->capability & CVI_I2S_MASTER)
		clk_enable(i2s_dev->clk);

	i2s_write_reg(i2s_dev->i2s_base, DWI2S_EN, i2s_dev->reg_dwctx->i2s_dwapb_ier);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX_BLOCK_EN, i2s_dev->reg_dwctx->i2s_dwapb_irer);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX_BLOCK_EN, i2s_dev->reg_dwctx->i2s_dwapb_iter);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_CLK_EN, i2s_dev->reg_dwctx->i2s_dwapb_cer);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_CLK_CFG, i2s_dev->reg_dwctx->i2s_dwapb_ccr);

	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX0_EN, i2s_dev->reg_dwctx->i2s_dwapb_rer0);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX0_EN, i2s_dev->reg_dwctx->i2s_dwapb_ter0);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX0_CFG_WLEN, i2s_dev->reg_dwctx->i2s_dwapb_rcr0);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX0_CFG_WLEN, i2s_dev->reg_dwctx->i2s_dwapb_tcr0);

	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX0_CFG_FIFO_LEVEL, i2s_dev->reg_dwctx->i2s_dwapb_rfcr0);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX0_CFG_FIFO_LEVEL, i2s_dev->reg_dwctx->i2s_dwapb_tfcr0);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX0_FIFO_FLUSH, i2s_dev->reg_dwctx->i2s_dwapb_rff0);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX0_FIFO_FLUSH, i2s_dev->reg_dwctx->i2s_dwapb_tff0);

	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX1_EN, i2s_dev->reg_dwctx->i2s_dwapb_rer1);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX1_EN, i2s_dev->reg_dwctx->i2s_dwapb_ter1);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX1_CFG_WLEN, i2s_dev->reg_dwctx->i2s_dwapb_rcr1);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX1_CFG_WLEN, i2s_dev->reg_dwctx->i2s_dwapb_tcr1);

	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX1_CFG_FIFO_LEVEL, i2s_dev->reg_dwctx->i2s_dwapb_rfcr1);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX1_CFG_FIFO_LEVEL, i2s_dev->reg_dwctx->i2s_dwapb_tfcr1);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_RX1_FIFO_FLUSH, i2s_dev->reg_dwctx->i2s_dwapb_rff1);
	i2s_write_reg(i2s_dev->i2s_base, DWI2S_TX1_FIFO_FLUSH, i2s_dev->reg_dwctx->i2s_dwapb_tff1);

	i2s_write_reg(i2s_dev->i2s_base, DWI2S_DMA_CONTROL, i2s_dev->reg_dwctx->i2s_dwapb_dmacr);

	return 0;
}

#else
#define cvi_dwi2s_pm_suspend	NULL
#define cvi_dwi2s_pm_resume	NULL
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops cvi_pm_ops = {
	SET_RUNTIME_PM_OPS(cvi_dwi2s_runtime_suspend, cvi_dwi2s_runtime_resume, NULL)
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(cvi_dwi2s_pm_suspend, cvi_dwi2s_pm_resume)
#endif
};
#endif

static struct platform_driver cvi_dw_i2s_driver = {
	.probe		= cvi_dw_i2s_probe,
	.remove		= cvi_dw_i2s_remove,
	.driver		= {
		.name	= "cvitek-dw-i2s",
		.of_match_table = of_match_ptr(cvi_dwi2s_of_match),
#ifdef CONFIG_PM
		.pm = &cvi_pm_ops,
#endif
	},
};

module_platform_driver(cvi_dw_i2s_driver);

MODULE_AUTHOR("<ethan.chen@wisecore.com.tw>");
MODULE_DESCRIPTION("CVITEK DW I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:CVITEK_DW_I2S");

