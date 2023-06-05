// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2014 Emilio López <emilio@elopez.com.ar>
 * Copyright 2014 Jon Smirl <jonsmirl@gmail.com>
 * Copyright 2015 Maxime Ripard <maxime.ripard@free-electrons.com>
 * Copyright 2015 Adam Sampson <ats@offog.org>
 * Copyright 2016 Chen-Yu Tsai <wens@csie.org>
 * Copyright 2021 gryzun <gryzun_an@rambler.ru>
 *
 * Based on the Allwinner SDK driver, released under the GPL.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/gpio/consumer.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <sound/dmaengine_pcm.h>

#define SUNXI_DAC_DPC (0x00)
#define SUNXI_DAC_DPC_EN_DA (31)
#define SUNXI_DAC_DPC_DVOL (12)
#define SUNXI_DAC_DPC_HPF_EN (18)

#define SUNXI_DAC_FIFOC (0x10)
#define SUNXI_DAC_FIFOC_DAC_FS (29)
#define SUNXI_DAC_FIFOC_TX_FIFO_MODE (24)
#define SUNXI_DAC_FIFOC_DRQ_CLR_CNT (21)
#define SUNXI_DAC_FIFOC_MONO_EN (6)
#define SUNXI_DAC_FIFOC_TX_SAMPLE_BITS (5)
#define SUNXI_DAC_FIFOC_DAC_DRQ_EN (4)
#define SUNXI_DAC_FIFOC_FIFO_FLUSH (0)

#define SUNXI_DAC_FIFO_STA (0x14)
#define SUNXI_DAC_TXE_INT (3)
#define SUNXI_DAC_TXU_INT (2)
#define SUNXI_DAC_TXO_INT (1)
#define SUNXI_DAC_TXDATA (0x20)
#define SUNXI_DAC_CNT (0x24)
#define SUNXI_DAC_DG_REG (0x28)
#define SUNXI_DAC_DAP_CTL (0xf0)

#define SUNXI_DAC_AC_DAC_REG (0x310)
#define SUNXI_DAC_LEN (15)
#define SUNXI_DAC_REN (14)
#define SUNXI_LINEOUTL_EN (13)
#define SUNXI_LMUTE (12)
#define SUNXI_LINEOUTR_EN (11)
#define SUNXI_RMUTE (10)
#define SUNXI_RSWITCH (9)
#define SUNXI_RAMPEN (8)
#define SUNXI_LINEOUTL_SEL (6)
#define SUNXI_LINEOUTR_SEL (5)
#define SUNXI_LINEOUT_VOL (0)

#define SUNXI_DAC_AC_MIXER_REG (0x314)
#define SUNXI_LMIX_LDAC (21)
#define SUNXI_LMIX_RDAC (20)
#define SUNXI_RMIX_RDAC (17)
#define SUNXI_RMIX_LDAC (16)
#define SUNXI_LMIXEN (11)
#define SUNXI_RMIXEN (10)

#define SUNXI_DAC_AC_RAMP_REG (0x31c)
#define SUNXI_RAMP_STEP (4)
#define SUNXI_RDEN (0)

#define LABEL(constant)        \
    {                          \
#constant, constant, 0 \
    }
#define LABEL_END   \
    {               \
        NULL, 0, -1 \
    }

struct audiocodec_reg_label
{
    const char *name;
    const unsigned int address;
    int value;
};

static struct audiocodec_reg_label reg_labels[] = {
    LABEL(SUNXI_DAC_DPC),
    LABEL(SUNXI_DAC_FIFOC),
    LABEL(SUNXI_DAC_FIFO_STA),
    LABEL(SUNXI_DAC_CNT),
    LABEL(SUNXI_DAC_DG_REG),
    LABEL(SUNXI_DAC_DAP_CTL),
    LABEL(SUNXI_DAC_AC_DAC_REG),
    LABEL(SUNXI_DAC_AC_MIXER_REG),
    LABEL(SUNXI_DAC_AC_RAMP_REG),
    LABEL_END,
};

struct regmap *codec_regmap_debug = NULL;

struct sun50i_h616_codec
{
    unsigned char *name;
    struct device *dev;
    struct regmap *regmap;
    struct clk *clk_apb;
    struct clk *clk_module;
    struct reset_control *rst;
    struct gpio_desc *gpio_pa;

    /* ADC_FIFOC register is at different offset on different SoCs */
    struct regmap_field *reg_adc_fifoc;

    struct snd_dmaengine_dai_dma_data playback_dma_data;
};

static void sun50i_h616_codec_start_playback(struct sun50i_h616_codec *scodec)
{
    /* Flush TX FIFO */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       BIT(SUNXI_DAC_FIFOC_FIFO_FLUSH),
                       BIT(SUNXI_DAC_FIFOC_FIFO_FLUSH));

    /* Enable DAC DRQ */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       BIT(SUNXI_DAC_FIFOC_DAC_DRQ_EN),
                       BIT(SUNXI_DAC_FIFOC_DAC_DRQ_EN));
}

static void sun50i_h616_codec_stop_playback(struct sun50i_h616_codec *scodec)
{
    /* Disable DAC DRQ */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       BIT(SUNXI_DAC_FIFOC_DAC_DRQ_EN),
                       0);
}

static int sun50i_h616_codec_trigger(struct snd_pcm_substream *substream, int cmd,
                                     struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

    switch (cmd)
    {
    case SNDRV_PCM_TRIGGER_START:
    case SNDRV_PCM_TRIGGER_RESUME:
    case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x1 << SUNXI_DAC_FIFOC_DAC_DRQ_EN),
                               (0x1 << SUNXI_DAC_FIFOC_DAC_DRQ_EN));
        break;
    case SNDRV_PCM_TRIGGER_STOP:
    case SNDRV_PCM_TRIGGER_SUSPEND:
    case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x1 << SUNXI_DAC_FIFOC_DAC_DRQ_EN),
                               (0x0 << SUNXI_DAC_FIFOC_DAC_DRQ_EN));
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static int sun50i_h616_codec_prepare(struct snd_pcm_substream *substream,
                                     struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       (0x1 << SUNXI_DAC_FIFOC_FIFO_FLUSH),
                       (0x1 << SUNXI_DAC_FIFOC_FIFO_FLUSH));
    regmap_write(scodec->regmap, SUNXI_DAC_FIFO_STA,
                 (0x1 << SUNXI_DAC_TXE_INT | 1 << SUNXI_DAC_TXU_INT | 0x1 << SUNXI_DAC_TXO_INT));
    regmap_write(scodec->regmap, SUNXI_DAC_CNT, 0);

    return 0;
}

static unsigned long sun50i_h616_codec_get_mod_freq(struct snd_pcm_hw_params *params)
{
    unsigned int rate = params_rate(params);

    switch (rate)
    {
    case 176400:
    case 88200:
    case 44100:
    case 33075:
    case 22050:
    case 14700:
    case 11025:
    case 7350:
        return 22579200;

    case 192000:
    case 96000:
    case 48000:
    case 32000:
    case 24000:
    case 16000:
    case 12000:
    case 8000:
        return 24576000;

    default:
        return 0;
    }
}

static int sun50i_h616_codec_get_hw_rate(struct snd_pcm_hw_params *params)
{
    unsigned int rate = params_rate(params);

    switch (rate)
    {
    case 192000:
    case 176400:
        return 6;

    case 96000:
    case 88200:
        return 7;

    case 48000:
    case 44100:
        return 0;

    case 32000:
    case 33075:
        return 1;

    case 24000:
    case 22050:
        return 2;

    case 16000:
    case 14700:
        return 3;

    case 12000:
    case 11025:
        return 4;

    case 8000:
    case 7350:
        return 5;

    default:
        return -EINVAL;
    }
}

static int sun50i_h616_codec_hw_params_playback(struct sun50i_h616_codec *scodec,
                                                struct snd_pcm_hw_params *params,
                                                unsigned int hwrate)
{
    u32 val;

    /* Set DAC sample rate */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       7 << SUNXI_DAC_FIFOC_DAC_FS,
                       hwrate << SUNXI_DAC_FIFOC_DAC_FS);

    /* Set the number of channels we want to use */
    if (params_channels(params) == 1)
        val = BIT(SUNXI_DAC_FIFOC_MONO_EN);
    else
        val = 0;

    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       BIT(SUNXI_DAC_FIFOC_MONO_EN),
                       val);

    /* Set the number of sample bits to either 16 or 24 bits */
    if (hw_param_interval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS)->min == 32)
    {
        regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                           BIT(SUNXI_DAC_FIFOC_TX_SAMPLE_BITS),
                           BIT(SUNXI_DAC_FIFOC_TX_SAMPLE_BITS));

        /* Set TX FIFO mode to padding the LSBs with 0 */
        regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                           BIT(SUNXI_DAC_FIFOC_TX_FIFO_MODE),
                           0);

        scodec->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
    }
    else
    {
        regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                           BIT(SUNXI_DAC_FIFOC_TX_SAMPLE_BITS),
                           0);

        /* Set TX FIFO mode to repeat the MSB */
        regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                           BIT(SUNXI_DAC_FIFOC_TX_FIFO_MODE),
                           BIT(SUNXI_DAC_FIFOC_TX_FIFO_MODE));

        scodec->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
    }

    return 0;
}

struct sample_rate
{
    unsigned int samplerate;
    unsigned int rate_bit;
};

static const struct sample_rate sample_rate_conv[] = {
    {44100, 0},
    {48000, 0},
    {8000, 5},
    {32000, 1},
    {22050, 2},
    {24000, 2},
    {16000, 3},
    {11025, 4},
    {12000, 4},
    {192000, 6},
    {96000, 7},
};

static int sun50i_h616_codec_hw_params(struct snd_pcm_substream *substream,
                                       struct snd_pcm_hw_params *params,
                                       struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(rtd->card);
    unsigned long clk_freq;
    int ret, hwrate;
    int i;

    clk_freq = sun50i_h616_codec_get_mod_freq(params);
    if (!clk_freq)
        return -EINVAL;

    ret = clk_set_rate(scodec->clk_module, clk_freq * 2);
    if (ret)
        return ret;

    hwrate = sun50i_h616_codec_get_hw_rate(params);
    if (hwrate < 0)
        return hwrate;

    switch (params_format(params))
    {
    case SNDRV_PCM_FORMAT_S16_LE:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        {
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x3 << SUNXI_DAC_FIFOC_TX_FIFO_MODE),
                               (0x3 << SUNXI_DAC_FIFOC_TX_FIFO_MODE));
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x1 << SUNXI_DAC_FIFOC_TX_SAMPLE_BITS),
                               (0x0 << SUNXI_DAC_FIFOC_TX_SAMPLE_BITS));
        }
        break;
    case SNDRV_PCM_FORMAT_S24_LE:
        if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
        {
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x3 << SUNXI_DAC_FIFOC_TX_FIFO_MODE),
                               (0x0 << SUNXI_DAC_FIFOC_TX_FIFO_MODE));
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x1 << SUNXI_DAC_FIFOC_TX_SAMPLE_BITS),
                               (0x1 << SUNXI_DAC_FIFOC_TX_SAMPLE_BITS));
        }
        break;
    default:
        break;
    }

    for (i = 0; i < ARRAY_SIZE(sample_rate_conv); i++)
    {
        if (sample_rate_conv[i].samplerate == params_rate(params))
        {
            if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
            {
                regmap_update_bits(scodec->regmap,
                                   SUNXI_DAC_FIFOC,
                                   (0x7 << SUNXI_DAC_FIFOC_DAC_FS),
                                   (sample_rate_conv[i].rate_bit << SUNXI_DAC_FIFOC_DAC_FS));
            }
        }
    }

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
    {
        switch (params_channels(params))
        {
        case 1:
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x1 << SUNXI_DAC_FIFOC_MONO_EN),
                               (0x1 << SUNXI_DAC_FIFOC_MONO_EN));
            break;
        case 2:
            regmap_update_bits(scodec->regmap,
                               SUNXI_DAC_FIFOC,
                               (0x1 << SUNXI_DAC_FIFOC_MONO_EN),
                               (0x0 << SUNXI_DAC_FIFOC_MONO_EN));
            break;
        default:
            pr_err("[%s] Playback cannot support %d channels.\n",
                   __func__, params_channels(params));
            return -EINVAL;
        }
    }

    return 0;
}

static unsigned int sun50i_h616_codec_src_rates[] = {
    8000, 11025, 12000, 16000, 22050, 24000, 32000,
    44100, 48000, 96000, 192000};

static struct snd_pcm_hw_constraint_list sun50i_h616_codec_constraints = {
    .count = ARRAY_SIZE(sun50i_h616_codec_src_rates),
    .list = sun50i_h616_codec_src_rates,
};

static int sun50i_h616_codec_startup(struct snd_pcm_substream *substream,
                                     struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

    snd_pcm_hw_constraint_list(substream->runtime, 0,
                               SNDRV_PCM_HW_PARAM_RATE, &sun50i_h616_codec_constraints);

    /*
     * Stop issuing DRQ when we have room for less than 16 samples
     * in our TX FIFO
     */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_FIFOC,
                       3 << SUNXI_DAC_FIFOC_DRQ_CLR_CNT,
                       3 << SUNXI_DAC_FIFOC_DRQ_CLR_CNT);

    return clk_prepare_enable(scodec->clk_module);
}

static void sun50i_h616_codec_shutdown(struct snd_pcm_substream *substream,
                                       struct snd_soc_dai *dai)
{
    struct snd_soc_pcm_runtime *rtd = asoc_substream_to_rtd(substream);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(rtd->card);

    clk_disable_unprepare(scodec->clk_module);
}

static const struct snd_soc_dai_ops sun50i_h616_codec_dai_ops = {
    .startup = sun50i_h616_codec_startup,
    .shutdown = sun50i_h616_codec_shutdown,
    .trigger = sun50i_h616_codec_trigger,
    .hw_params = sun50i_h616_codec_hw_params,
    .prepare = sun50i_h616_codec_prepare,
};

static struct snd_soc_dai_driver sun50i_h616_codec_dai = {
    .name = "Codec",
    .ops = &sun50i_h616_codec_dai_ops,
    .playback = {
        .stream_name = "Codec Playback",
        .channels_min = 1,
        .channels_max = 2,
        .rate_min = 8000,
        .rate_max = 192000,
        .rates = SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
        .formats = SNDRV_PCM_FMTBIT_S16_LE |
                   SNDRV_PCM_FMTBIT_S24_LE,
        .sig_bits = 24,
    },
};

static int sunxi_lineout_event(struct snd_soc_dapm_widget *w,
                               struct snd_kcontrol *k, int event)
{
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(w->dapm->card);

    switch (event)
    {
    case SND_SOC_DAPM_POST_PMU:

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_RAMP_REG,
                           (0x1 << SUNXI_RDEN), (0x1 << SUNXI_RDEN));
        msleep(25);

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                           (0x1 << SUNXI_LINEOUTL_EN) | (0x1 << SUNXI_LINEOUTR_EN),
                           (0x1 << SUNXI_LINEOUTL_EN) | (0x1 << SUNXI_LINEOUTR_EN));
        break;
    case SND_SOC_DAPM_PRE_PMD:

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_RAMP_REG,
                           (0x1 << SUNXI_RDEN), (0x0 << SUNXI_RDEN));
        msleep(25);

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                           (0x1 << SUNXI_LINEOUTL_EN) | (0x1 << SUNXI_LINEOUTR_EN),
                           (0x0 << SUNXI_LINEOUTL_EN) | (0x0 << SUNXI_LINEOUTR_EN));

        break;
    default:
        break;
    }

    return 0;
}

static const DECLARE_TLV_DB_SCALE(digital_tlv, 0, -116, -7424);
static const DECLARE_TLV_DB_SCALE(linein_to_l_r_mix_vol_tlv, -450, 150, 0);
static const DECLARE_TLV_DB_SCALE(fmin_to_l_r_mix_vol_tlv, -450, 150, 0);

static const unsigned int lineout_tlv[] = {
    TLV_DB_RANGE_HEAD(2),
    0,
    0,
    TLV_DB_SCALE_ITEM(0, 0, 1),
    1,
    31,
    TLV_DB_SCALE_ITEM(-4350, 150, 1),
};

/*lineoutL mux select */
const char *const left_lineoutl_text[] = {
    "LOMixer",
    "LROMixer",
};

static const struct soc_enum left_lineout_enum =
    SOC_ENUM_SINGLE(SUNXI_DAC_AC_DAC_REG, SUNXI_LINEOUTL_SEL,
                    ARRAY_SIZE(left_lineoutl_text), left_lineoutl_text);

static const struct snd_kcontrol_new left_lineout_mux =
    SOC_DAPM_ENUM("Left LINEOUT Mux", left_lineout_enum);

/*lineoutR mux select */
const char *const right_lineoutr_text[] = {
    "ROMixer",
    "LROMixer",
};

static const struct soc_enum right_lineout_enum =
    SOC_ENUM_SINGLE(SUNXI_DAC_AC_DAC_REG, SUNXI_LINEOUTR_SEL,
                    ARRAY_SIZE(right_lineoutr_text), right_lineoutr_text);

static const struct snd_kcontrol_new right_lineout_mux =
    SOC_DAPM_ENUM("Right LINEOUT Mux", right_lineout_enum);

static const struct snd_kcontrol_new sun50i_h616_codec_codec_controls[] = {

    SOC_SINGLE_TLV("digital volume", SUNXI_DAC_DPC,
                   SUNXI_DAC_DPC_DVOL, 0x3F, 0, digital_tlv),

    SOC_SINGLE_TLV("LINEOUT volume", SUNXI_DAC_AC_DAC_REG,
                   SUNXI_LINEOUT_VOL, 0x1F, 0, lineout_tlv),
};

static const struct snd_kcontrol_new left_output_mixer[] = {
    SOC_DAPM_SINGLE("DACL Switch", SUNXI_DAC_AC_MIXER_REG, SUNXI_LMIX_LDAC, 1, 0),
    SOC_DAPM_SINGLE("DACR Switch", SUNXI_DAC_AC_MIXER_REG, SUNXI_LMIX_RDAC, 1, 0),
};

static const struct snd_kcontrol_new right_output_mixer[] = {
    SOC_DAPM_SINGLE("DACL Switch", SUNXI_DAC_AC_MIXER_REG, SUNXI_RMIX_LDAC, 1, 0),
    SOC_DAPM_SINGLE("DACR Switch", SUNXI_DAC_AC_MIXER_REG, SUNXI_RMIX_RDAC, 1, 0),
};

static const struct snd_soc_dapm_widget sun50i_h616_codec_codec_widgets[] = {

    /* Digital parts of the DACs */
    SND_SOC_DAPM_SUPPLY("DAC Enable", SUNXI_DAC_DPC,
                        SUNXI_DAC_DPC_EN_DA, 0, NULL, 0),

    SND_SOC_DAPM_AIF_IN_E("DACL", "Codec Playback", 0, SUNXI_DAC_AC_DAC_REG, SUNXI_DAC_LEN, 0,
                          NULL, 0),
    SND_SOC_DAPM_AIF_IN_E("DACR", "Codec Playback", 0, SUNXI_DAC_AC_DAC_REG, SUNXI_DAC_REN, 0,
                          NULL, 0),

    SND_SOC_DAPM_MIXER("Left Output Mixer", SUNXI_DAC_AC_MIXER_REG, SUNXI_LMIXEN, 0,
                       left_output_mixer, ARRAY_SIZE(left_output_mixer)),
    SND_SOC_DAPM_MIXER("Right Output Mixer", SUNXI_DAC_AC_MIXER_REG, SUNXI_RMIXEN, 0,
                       right_output_mixer, ARRAY_SIZE(right_output_mixer)),

    SND_SOC_DAPM_MUX("Left LINEOUT Mux", SND_SOC_NOPM,
                     0, 0, &left_lineout_mux),
    SND_SOC_DAPM_MUX("Right LINEOUT Mux", SND_SOC_NOPM,
                     0, 0, &right_lineout_mux),

    SND_SOC_DAPM_OUTPUT("LINEOUTL"),
    SND_SOC_DAPM_OUTPUT("LINEOUTR"),

    SND_SOC_DAPM_LINE("LINEOUT", sunxi_lineout_event),
};

static const struct snd_soc_component_driver sun50i_h616_codec_codec = {
    .controls = sun50i_h616_codec_codec_controls,
    .num_controls = ARRAY_SIZE(sun50i_h616_codec_codec_controls),
    .dapm_widgets = sun50i_h616_codec_codec_widgets,
    .num_dapm_widgets = ARRAY_SIZE(sun50i_h616_codec_codec_widgets),
    .idle_bias_on = 1,
    .use_pmdown_time = 1,
    .endianness = 1,
};

static const struct snd_soc_component_driver sun50i_h616_codec_component = {
    .name = "sun50i_h616-codec",
    .legacy_dai_naming = 1,
#ifdef CONFIG_DEBUG_FS
	.debugfs_prefix		= "cpu",
#endif
};

#define SUN50IW9_CODEC_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
#define SUN50IW9_CODEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

static int sun50i_h616_codec_dai_probe(struct snd_soc_dai *dai)
{
    struct snd_soc_card *card = snd_soc_dai_get_drvdata(dai);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(card);

    snd_soc_dai_init_dma_data(dai, &scodec->playback_dma_data,
                              NULL);

    return 0;
}

static struct snd_soc_dai_driver dummy_cpu_dai = {
    .name = "sun50i_h616-codec-cpu-dai",
    .probe = sun50i_h616_codec_dai_probe,
    .playback = {
        .stream_name = "Playback",
        .channels_min = 1,
        .channels_max = 2,
        .rates = SUN50IW9_CODEC_RATES,
        .formats = SUN50IW9_CODEC_FORMATS,
        .sig_bits = 24,
    },
};

static struct snd_soc_dai_link *sun50i_h616_codec_create_link(struct device *dev,
                                                              int *num_links)
{
    struct snd_soc_dai_link *link = devm_kzalloc(dev, sizeof(*link),
                                                 GFP_KERNEL);
    struct snd_soc_dai_link_component *dlc = devm_kzalloc(dev,
                                                          3 * sizeof(*dlc), GFP_KERNEL);
    if (!link || !dlc)
        return NULL;

    link->cpus = &dlc[0];
    link->codecs = &dlc[1];
    link->platforms = &dlc[2];

    link->num_cpus = 1;
    link->num_codecs = 1;
    link->num_platforms = 1;

    link->name = "cdc";
    link->stream_name = "CDC PCM";
    link->codecs->dai_name = "Codec";
    link->cpus->dai_name = dev_name(dev);
    link->codecs->name = dev_name(dev);
    link->platforms->name = dev_name(dev);
    link->dai_fmt = SND_SOC_DAIFMT_I2S;
    link->playback_only = true;
    link->capture_only = false;

    *num_links = 1;

    return link;
};

static int sun50i_h616_codec_spk_event(struct snd_soc_dapm_widget *w,
                                       struct snd_kcontrol *k, int event)
{
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(w->dapm->card);

    gpiod_set_value_cansleep(scodec->gpio_pa,
                             !!SND_SOC_DAPM_EVENT_ON(event));

    if (SND_SOC_DAPM_EVENT_ON(event))
    {
        /*
         * Need a delay to wait for DAC to push the data. 700ms seems
         * to be the best compromise not to feel this delay while
         * playing a sound.
         */
        msleep(700);
    }

    return 0;
}

static const struct snd_soc_dapm_widget sun6i_codec_card_dapm_widgets[] = {
    SND_SOC_DAPM_LINE("Line Out", NULL),
    SND_SOC_DAPM_SPK("Speaker", sun50i_h616_codec_spk_event),
};

/* Connect digital side enables to analog side widgets */
static const struct snd_soc_dapm_route sun8i_codec_card_routes[] = {
    /* DAC Routes */
    {"DACR", NULL, "DAC Enable"},
    {"DACL", NULL, "DAC Enable"},

    {"Left Output Mixer", "DACR Switch", "DACR"},
    {"Left Output Mixer", "DACL Switch", "DACL"},

    {"Right Output Mixer", "DACL Switch", "DACL"},
    {"Right Output Mixer", "DACR Switch", "DACR"},

    {"Left LINEOUT Mux", "LOMixer", "Left Output Mixer"},
    {"Left LINEOUT Mux", "LROMixer", "Right Output Mixer"},
    {"Right LINEOUT Mux", "ROMixer", "Right Output Mixer"},
    {"Right LINEOUT Mux", "LROMixer", "Left Output Mixer"},

    {"LINEOUTL", NULL, "Left LINEOUT Mux"},
    {"LINEOUTR", NULL, "Right LINEOUT Mux"},

    {"LINEOUT", NULL, "LINEOUTL"},
    {"LINEOUT", NULL, "LINEOUTR"},

    {"Speaker", NULL, "LINEOUTL"},
    {"Speaker", NULL, "LINEOUTR"},
};

static const struct snd_kcontrol_new sunxi_card_controls[] = {
    SOC_DAPM_PIN_SWITCH("LINEOUT"),
};

static struct snd_soc_card *sun50i_h616_codec_create_card(struct device *dev)
{
    struct snd_soc_card *card;
    int ret;

    card = devm_kzalloc(dev, sizeof(*card), GFP_KERNEL);
    if (!card)
        return ERR_PTR(-ENOMEM);

    card->dai_link = sun50i_h616_codec_create_link(dev, &card->num_links);
    if (!card->dai_link)
        return ERR_PTR(-ENOMEM);

    card->dev = dev;
    card->owner = THIS_MODULE;
    card->name = "audiocodec";
    card->controls = sunxi_card_controls;
    card->num_controls = ARRAY_SIZE(sunxi_card_controls),
    card->dapm_widgets = sun6i_codec_card_dapm_widgets;
    card->num_dapm_widgets = ARRAY_SIZE(sun6i_codec_card_dapm_widgets);
    card->dapm_routes = sun8i_codec_card_routes;
    card->num_dapm_routes = ARRAY_SIZE(sun8i_codec_card_routes);
    card->fully_routed = true;

    ret = snd_soc_of_parse_audio_routing(card, "allwinner,audio-routing");
    if (ret)
        dev_warn(dev, "failed to parse audio-routing: %d\n", ret);

    return card;
};

static const struct regmap_config sun50i_h616_codec_regmap_config = {
    .reg_bits = 32,
    .reg_stride = 4,
    .val_bits = 32,
    .max_register = SUNXI_DAC_AC_RAMP_REG,
    .cache_type = REGCACHE_NONE,
};

struct sun50i_h616_codec_quirks
{
    const struct regmap_config *regmap_config;
    const struct snd_soc_component_driver *codec;
    struct snd_soc_card *(*create_card)(struct device *dev);
    struct reg_field reg_adc_fifoc; /* used for regmap_field */
    unsigned int reg_dac_txdata;    /* TX FIFO offset for DMA config */
    unsigned int reg_adc_rxdata;    /* RX FIFO offset for DMA config */
    bool has_reset;
};

static const struct sun50i_h616_codec_quirks sun50i_h616_codec_quirks = {
    .regmap_config = &sun50i_h616_codec_regmap_config,
    .codec = &sun50i_h616_codec_codec,
    .create_card = sun50i_h616_codec_create_card,
    .reg_dac_txdata = SUNXI_DAC_TXDATA,
    .has_reset = true,
};

static const struct of_device_id sun50i_h616_codec_of_match[] = {
    {
        .compatible = "allwinner,sun50i-h616-codec",
        .data = &sun50i_h616_codec_quirks,
    },
    {}};
MODULE_DEVICE_TABLE(of, sun50i_h616_codec_of_match);

static ssize_t show_audio_reg(struct device *dev,
                              struct device_attribute *attr, char *buf)
{
    int count = 0, i = 0;
    unsigned int reg_val;
    unsigned int size = ARRAY_SIZE(reg_labels);

    count += sprintf(buf, "dump audiocodec reg:\n");

    while ((i < size) && (reg_labels[i].name != NULL))
    {
        regmap_read(codec_regmap_debug,
                    reg_labels[i].address, &reg_val);
        count += sprintf(buf + count, "%-20s [0x%03x]: 0x%-10x save_val:0x%x\n",
                         reg_labels[i].name, (reg_labels[i].address),
                         reg_val, reg_labels[i].value);
        i++;
    }

    return count;
}

static DEVICE_ATTR(audio_reg, 0644, show_audio_reg, NULL);

static struct attribute *audio_debug_attrs[] = {
    &dev_attr_audio_reg.attr,
    NULL,
};

static struct attribute_group audio_debug_attr_group = {
    .name = "audio_reg_debug",
    .attrs = audio_debug_attrs,
};

static void sunxi_codec_init(struct sun50i_h616_codec *scodec)
{
    /* Disable DRC function for playback */
    regmap_write(scodec->regmap, SUNXI_DAC_DAP_CTL, 0);

    /* Enable HPF(high passed filter) */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_DPC,
                       (0x1 << SUNXI_DAC_DPC_HPF_EN), (0x1 << SUNXI_DAC_DPC_HPF_EN));

    regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                       (0x1f << SUNXI_LINEOUT_VOL),
                       (0x1a << SUNXI_LINEOUT_VOL));

    regmap_update_bits(scodec->regmap, SUNXI_DAC_DPC,
                       (0x3f << SUNXI_DAC_DPC_DVOL), (0 << SUNXI_DAC_DPC_DVOL));

    /* Mixer to channel LINEOUT MUTE control init */
    regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                       (0x1 << SUNXI_LMUTE), (0x1 << SUNXI_LMUTE));
    regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                       (0x1 << SUNXI_RMUTE), (0x1 << SUNXI_RMUTE));

    /* ramp func about */
    if (0)
    {
        /* Not used the ramp func cause there is the MUTE to avoid pop noise */
        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                           (0x1 << SUNXI_RSWITCH), (0x1 << SUNXI_RSWITCH));

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                           (0x1 << SUNXI_RAMPEN), (0x0 << SUNXI_RAMPEN));

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_RAMP_REG,
                           (0x7 << SUNXI_RAMP_STEP), (0x0 << SUNXI_RAMP_STEP));

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_RAMP_REG,
                           (0x1 << SUNXI_RDEN), (0x0 << SUNXI_RDEN));
    }
    else
    {
        /* If no MUTE to avoid pop, just use the ramp func to avoid it */
        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                           (0x1 << SUNXI_RSWITCH), (0x0 << SUNXI_RSWITCH));

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_DAC_REG,
                           (0x1 << SUNXI_RAMPEN), (0x1 << SUNXI_RAMPEN));

        regmap_update_bits(scodec->regmap, SUNXI_DAC_AC_RAMP_REG,
                           (0x7 << SUNXI_RAMP_STEP), (0x1 << SUNXI_RAMP_STEP));
    }
}

static int sun50i_h616_codec_probe(struct platform_device *pdev)
{
    struct snd_soc_card *card;
    struct sun50i_h616_codec *scodec;
    const struct sun50i_h616_codec_quirks *quirks;
    struct resource *res;
    void __iomem *base;
    int ret;

    scodec = devm_kzalloc(&pdev->dev, sizeof(struct sun50i_h616_codec), GFP_KERNEL);
    if (!scodec)
        return -ENOMEM;

    scodec->dev = &pdev->dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base))
        return PTR_ERR(base);

    quirks = of_device_get_match_data(&pdev->dev);
    if (quirks == NULL)
    {
        dev_err(&pdev->dev, "Failed to determine the quirks to use\n");
        return -ENODEV;
    }

    scodec->regmap = devm_regmap_init_mmio(&pdev->dev, base,
                                           quirks->regmap_config);
    if (IS_ERR(scodec->regmap))
    {
        dev_err(&pdev->dev, "Failed to create our regmap\n");
        return PTR_ERR(scodec->regmap);
    }

    /* Get the clocks from the DT */
    scodec->clk_apb = devm_clk_get(&pdev->dev, "apb");
    if (IS_ERR(scodec->clk_apb))
    {
        dev_err(&pdev->dev, "Failed to get the APB clock\n");
        return PTR_ERR(scodec->clk_apb);
    }

    scodec->clk_module = devm_clk_get(&pdev->dev, "audio-codec-1x");
    if (IS_ERR(scodec->clk_module))
    {
        dev_err(&pdev->dev, "Failed to get the codec module clock\n");
        return PTR_ERR(scodec->clk_module);
    }

    if (quirks->has_reset)
    {
        scodec->rst = devm_reset_control_get_exclusive(&pdev->dev,
                                                       NULL);
        if (IS_ERR(scodec->rst))
        {
            dev_err(&pdev->dev, "Failed to get reset control\n");
            return PTR_ERR(scodec->rst);
        }
    }

    scodec->gpio_pa = devm_gpiod_get_optional(&pdev->dev, "allwinner,pa",
                                              GPIOD_OUT_LOW);
    if (IS_ERR(scodec->gpio_pa))
    {
        ret = PTR_ERR(scodec->gpio_pa);
        if (ret != -EPROBE_DEFER)
            dev_err(&pdev->dev, "Failed to get pa gpio: %d\n", ret);
        return ret;
    }

    /* Enable the bus clock */
    if (clk_prepare_enable(scodec->clk_apb))
    {
        dev_err(&pdev->dev, "Failed to enable the APB clock\n");
        return -EINVAL;
    }

    /* Deassert the reset control */
    if (scodec->rst)
    {
        ret = reset_control_deassert(scodec->rst);
        if (ret)
        {
            dev_err(&pdev->dev,
                    "Failed to deassert the reset control\n");
            goto err_clk_disable;
        }
    }

    /* DMA configuration for TX FIFO */
    scodec->playback_dma_data.addr = res->start + quirks->reg_dac_txdata;
    scodec->playback_dma_data.maxburst = 8;
    scodec->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;

    ret = devm_snd_soc_register_component(&pdev->dev, quirks->codec,
                                          &sun50i_h616_codec_dai, 1);
    if (ret)
    {
        dev_err(&pdev->dev, "Failed to register our codec\n");
        goto err_assert_reset;
    }

    ret = devm_snd_soc_register_component(&pdev->dev,
                                          &sun50i_h616_codec_component,
                                          &dummy_cpu_dai, 1);
    if (ret)
    {
        dev_err(&pdev->dev, "Failed to register our DAI\n");
        goto err_assert_reset;
    }

    ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
    if (ret)
    {
        dev_err(&pdev->dev, "Failed to register against DMAEngine\n");
        goto err_assert_reset;
    }

    card = quirks->create_card(&pdev->dev);
    if (IS_ERR(card))
    {
        ret = PTR_ERR(card);
        dev_err(&pdev->dev, "Failed to create our card\n");
        goto err_assert_reset;
    }

    snd_soc_card_set_drvdata(card, scodec);

    codec_regmap_debug = scodec->regmap;

    ret = snd_soc_register_card(card);
    if (ret)
    {
        dev_err(&pdev->dev, "Failed to register our card\n");
        goto err_assert_reset;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &audio_debug_attr_group);
    if (ret)
        dev_warn(&pdev->dev, "failed to create attr group\n");

    sunxi_codec_init(scodec);

    return 0;

err_assert_reset:
    if (scodec->rst)
        reset_control_assert(scodec->rst);
err_clk_disable:
    clk_disable_unprepare(scodec->clk_apb);
    return ret;
}

static int sun50i_h616_codec_remove(struct platform_device *pdev)
{
    struct snd_soc_card *card = platform_get_drvdata(pdev);
    struct sun50i_h616_codec *scodec = snd_soc_card_get_drvdata(card);

    snd_soc_unregister_card(card);
    if (scodec->rst)
        reset_control_assert(scodec->rst);
    clk_disable_unprepare(scodec->clk_apb);

    return 0;
}

static struct platform_driver sun50i_h616_codec_driver = {
    .driver = {
        .name = "sun50i-h616-codec",
        .of_match_table = sun50i_h616_codec_of_match,
    },
    .probe = sun50i_h616_codec_probe,
    .remove = sun50i_h616_codec_remove,
};
module_platform_driver(sun50i_h616_codec_driver);

MODULE_DESCRIPTION("Allwinner H616 codec driver");
MODULE_AUTHOR("Emilio López <emilio@elopez.com.ar>");
MODULE_AUTHOR("Jon Smirl <jonsmirl@gmail.com>");
MODULE_AUTHOR("Maxime Ripard <maxime.ripard@free-electrons.com>");
MODULE_AUTHOR("Chen-Yu Tsai <wens@csie.org>");
MODULE_AUTHOR("Leeboby <leeboby@aliyun.com>");
MODULE_LICENSE("GPL");
