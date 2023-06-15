# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_SND_SUN4I_CODEC) += sun4i-codec.o
obj-$(CONFIG_SND_SUN4I_I2S) += sun4i-i2s.o
obj-$(CONFIG_SND_SUN4I_SPDIF) += sun4i-spdif.o
obj-$(CONFIG_SND_SUN8I_CODEC_ANALOG) += sun8i-codec-analog.o
obj-$(CONFIG_SND_SUN50I_CODEC_ANALOG) += sun50i-codec-analog.o
obj-$(CONFIG_SND_SUN8I_CODEC) += sun8i-codec.o
obj-$(CONFIG_SND_SUN8I_ADDA_PR_REGMAP) += sun8i-adda-pr-regmap.o

snd-soc-sunxi-objs := sunxi-pcm.o
snd-soc-sunxi-simple-card-objs := sunxi-simple-card.o
snd-soc-sunxi-dummy-cpudai-objs := sunxi-dummy-cpudai.o
snd-soc-sunxi-spdif-objs := sunxi-spdif.o
snd-soc-sunxi-dmic-objs := sunxi-dmic.o
snd-soc-sunxi-daudio-objs := sunxi-daudio.o
snd-soc-sunxi-hdmi-objs := sunxi-hdmi.o

obj-$(CONFIG_SND_SUNXI_SOC) += snd-soc-sunxi.o

# SUNXI Audio Simple Card Support
obj-$(CONFIG_SND_SUNXI_SOC_SIMPLE_CARD) += snd-soc-sunxi-simple-card.o

# SUNXI Dummy CPUDAI Support
obj-$(CONFIG_SND_SUNXI_SOC_CPUDAI) += snd-soc-sunxi-dummy-cpudai.o

# SUNXI SPDIF Support
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += snd-soc-sunxi-spdif.o

# SUNXI DMIC Support
obj-$(CONFIG_SND_SUNXI_SOC_DMIC) += snd-soc-sunxi-dmic.o

# SUNXI I2S Support
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO) += snd-soc-sunxi-daudio.o

# SUNXI HDMIAUDIO Support
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_HDMIAUDIO) += snd-soc-sunxi-hdmi.o

# SUNXI Sun50iw10 Codec Support
obj-$(CONFIG_SND_SUN50IW10_CODEC) += sun50iw10-codec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW10_CODEC) += sun50iw10-sndcodec.o
