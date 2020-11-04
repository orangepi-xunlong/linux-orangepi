# SUNXI Platform Support
snd-soc-sunxi-objs := sunxi-pcm.o
snd-soc-sunxi-hdmi-objs := sunxi-hdmi.o
snd-soc-sunxi-daudio-objs := sunxi-daudio.o
snd-soc-sunxi-spdif-objs := sunxi-spdif.o spdif-utils.o
snd-soc-sunxi-dmic-objs := sunxi-dmic.o
snd-soc-sunxi-cpudai-objs := sunxi-cpudai.o
snd-soc-sunxi-rwfunc-objs := sunxi_rw_func.o
snd-soc-sunxi-inter-i2s-objs := sunxi-inter-i2s.o

obj-$(CONFIG_SND_SUNXI_SOC) += snd-soc-sunxi.o
obj-$(CONFIG_SND_SUNXI_SOC_HDMI) += snd-soc-sunxi-hdmi.o
obj-$(CONFIG_SND_SUNXI_SOC_DAUDIO) += snd-soc-sunxi-daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_SPDIF) += snd-soc-sunxi-spdif.o
obj-$(CONFIG_SND_SUNXI_SOC_DMIC) += snd-soc-sunxi-dmic.o
obj-$(CONFIG_SND_SUNXI_SOC_CODEC_CPU_DAI) += snd-soc-sunxi-cpudai.o
obj-$(CONFIG_SND_SUNXI_SOC_RWFUNC) += snd-soc-sunxi-rwfunc.o
obj-$(CONFIG_SND_SUNXI_SOC_INTER_I2S) += snd-soc-sunxi-inter-i2s.o

# SUNXI Codec Embedded Support
obj-$(CONFIG_SND_SUN50IW1_CODEC) += sun50iw1-codec.o
obj-$(CONFIG_SND_SUN50IW3_CODEC) += sun50iw3-codec.o
obj-$(CONFIG_SND_SUN50IW8_CODEC) += sun50iw8-codec.o
obj-$(CONFIG_SND_SUN50IW9_CODEC) += sun50iw9-codec.o
obj-$(CONFIG_SND_SUN50IW10_CODEC) += sun50iw10-codec.o

obj-$(CONFIG_SND_SUN8IW7_CODEC)  += sun8iw7-codec.o
obj-$(CONFIG_SND_SUN8IW11_CODEC) += sun8iw11-codec.o
obj-$(CONFIG_SND_SUN8IW15_CODEC) += sun8iw15-codec.o
obj-$(CONFIG_SND_SUN8IW17_CODEC) += sun8iw17-codec.o

# SUNXI Machine Support
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW1_CODEC) += sun50iw1-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW3_CODEC) += sun50iw3-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW8_CODEC) += sun50iw8-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW9_CODEC) += sun50iw9-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN50IW10_CODEC) += sun50iw10-sndcodec.o

obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW7_CODEC)  += sun8iw7-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW11_CODEC) += sun8iw11-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW15_CODEC) += sun8iw15-sndcodec.o
obj-$(CONFIG_SND_SUNXI_SOC_SUN8IW17_CODEC) += sun8iw17-sndcodec.o

obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_HDMIAUDIO) += sunxi-sndhdmi.o
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_DAUDIO) += sunxi-snddaudio.o
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_SPDIF) += sunxi-sndspdif.o
obj-$(CONFIG_SND_SUNXI_SOC_SUNXI_DMIC) += sunxi-snddmic.o

# audio hub
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_ahub.o
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_ahub_cpudai.o
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_ahub_daudio.o
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi_netlink.o
# ahub machine
obj-$(CONFIG_SND_SUNXI_SOC_AHUB) += sunxi-sndahub.o

#MAD module for sun50iw8
obj-$(CONFIG_SND_SUNXI_MAD) += sunxi-mad.o

obj-$(CONFIG_SND_SUN4I_I2S) += sun4i-i2s.o
obj-$(CONFIG_SND_SUN4I_CODEC) += sun4i-codec.o
obj-$(CONFIG_SND_SUN4I_SPDIF) += sun4i-spdif.o
