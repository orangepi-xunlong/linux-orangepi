# SPDX-License-Identifier: GPL-2.0-or-later
#
# Allwinner audio platform driver Makefile
#
# Copyright(c) 2022-2027 Allwinnertech Co., Ltd.

ccflags-y += -I $(srctree)/bsp/drivers/sound/pa/
ccflags-y += -I $(srctree)/bsp/drivers/sound/adapter
ccflags-y += -I $(srctree)/bsp/drivers/sound/platform
ADPT_DIR := ../adapter

snd_soc_sunxi_component_sfx-objs		+= snd_sunxi_sfx.o
obj-$(CONFIG_SND_SOC_SUNXI_SFX)			+= snd_soc_sunxi_component_sfx.o

# common -> platform of dma
snd_soc_sunxi_pcm-objs				+= $(ADPT_DIR)/snd_sunxi_pcm_adapter.o
snd_soc_sunxi_pcm-objs				+= snd_sunxi_pcm.o
obj-$(CONFIG_SND_SOC_SUNXI_PCM)			+= snd_soc_sunxi_pcm.o

# common -> common interface
snd_soc_sunxi_common-objs			+= $(ADPT_DIR)/snd_sunxi_adapter.o
snd_soc_sunxi_common-objs			+= snd_sunxi_rxsync.o
snd_soc_sunxi_common-objs			+= snd_sunxi_jack_codec.o
snd_soc_sunxi_common-objs			+= snd_sunxi_jack_extcon.o
snd_soc_sunxi_common-objs			+= snd_sunxi_jack_gpio.o
snd_soc_sunxi_common-objs			+= snd_sunxi_jack_advance.o
snd_soc_sunxi_common-objs			+= snd_sunxi_jack.o
snd_soc_sunxi_common-objs			+= snd_sunxi_common_utils.o
snd_soc_sunxi_common-objs			+= snd_sunxi_common.o
obj-$(CONFIG_SND_SOC_SUNXI_COMMON)		+= snd_soc_sunxi_common.o

# codec -> internal codec
ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN50IW10_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun50iw10_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN50IW9_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun50iw9_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN8IW11_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun8iw11_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN8IW18_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun8iw18_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN8IW20_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun8iw20_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN8IW21_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun8iw21_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN20IW1_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun20iw1_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN55IW3_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun55iw3_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN55IW6_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun55iw6_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN251IW1_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun251iw1_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN300IW1_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun300iw1_codec.o
else ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_SUN65IW1_CODEC)))
snd_soc_sunxi_internal_codec-objs		+= snd_sun65iw1_codec.o
endif
snd_soc_sunxi_internal_codec-objs		+= snd_sunxi_dap.o
obj-$(CONFIG_SND_SOC_SUNXI_INTERNALCODEC)	+= snd_soc_sunxi_internal_codec.o

# codec -> dummy codec
snd_soc_sunxi_dummy_codec-objs			+= snd_sunxi_dummy_codec.o
obj-$(CONFIG_SND_SOC_SUNXI_DUMMYCODEC)		+= snd_soc_sunxi_dummy_codec.o

# codec -> hdmi codec
snd_soc_sunxi_codec_hdmi-objs			+= snd_sunxi_codec_hdmi.o
obj-$(CONFIG_SND_SOC_SUNXI_CODEC_HDMI)		+= snd_soc_sunxi_codec_hdmi.o

# codec -> edp codec
snd_soc_sunxi_codec_edp-objs			+= snd_sunxi_codec_edp.o
obj-$(CONFIG_SND_SOC_SUNXI_CODEC_EDP)		+= snd_soc_sunxi_codec_edp.o

# codec -> av codec
snd_soc_sunxi_codec_av-objs			+= snd_sunxi_codec_av.o
obj-$(CONFIG_SND_SOC_SUNXI_CODEC_AV)		+= snd_soc_sunxi_codec_av.o

# platform -> analog audio
snd_soc_sunxi_aaudio-objs			+= snd_sunxi_aaudio.o
obj-$(CONFIG_SND_SOC_SUNXI_AAUDIO)		+= snd_soc_sunxi_aaudio.o

# platform -> owa
ifneq (, $(filter y m, $(CONFIG_ARCH_SUN50IW10)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun55iw10_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN50IW9)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun50iw9_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW11)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun8iw11_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW3)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun55iw3_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW18)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun8iw18_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW20)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun8iw20_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN20IW1)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun20iw1_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW6)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun55iw6_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN65IW1)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun65iw1_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN251IW1)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun251iw1_owa.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN60IW2)))
snd_soc_sunxi_owa-objs              += platforms/snd_sun60iw2_owa.o
endif
ifneq (, $(filter y m, $(CONFIG_SND_SOC_SUNXI_OWA_RXIEC61937)))
snd_soc_sunxi_owa-objs				+= snd_sunxi_owa_rx61937.o
endif
snd_soc_sunxi_owa-objs				+= snd_sunxi_owa.o
obj-$(CONFIG_SND_SOC_SUNXI_OWA)			+= snd_soc_sunxi_owa.o

# platform -> dmic
ifneq (, $(filter y m, $(CONFIG_ARCH_SUN50IW10)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun50iw10_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN50IW9)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun50iw9_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW3)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun55iw3_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW18)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun8iw18_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW20)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun8iw20_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW21)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun8iw21_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN20IW1)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun20iw1_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW6)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun55iw6_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN65IW1)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun65iw1_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN60IW2)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun60iw2_dmic.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN251IW1)))
snd_soc_sunxi_dmic-objs              += platforms/snd_sun251iw1_dmic.o
endif
snd_soc_sunxi_dmic-objs				+= snd_sunxi_dmic.o
obj-$(CONFIG_SND_SOC_SUNXI_DMIC)		+= snd_soc_sunxi_dmic.o

# platform -> i2s
ifneq (, $(filter y m, $(CONFIG_ARCH_SUN50IW10)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun50iw10_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW11)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun8iw11_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW3)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun55iw3_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW18)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun8iw18_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW17)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun8iw17_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW20)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun8iw20_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN8IW21)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun8iw21_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN20IW1)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun20iw1_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN55IW6)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun55iw6_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN251IW1)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun251iw1_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN300IW1)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun300iw1_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN65IW1)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun65iw1_i2s.o
else ifneq (, $(filter y m, $(CONFIG_ARCH_SUN60IW2)))
snd_soc_sunxi_i2s-objs              += platforms/snd_sun60iw2_i2s.o
endif
snd_soc_sunxi_i2s-objs				+= snd_sunxi_i2s.o
obj-$(CONFIG_SND_SOC_SUNXI_I2S)			+= snd_soc_sunxi_i2s.o

# platform -> ahub
snd_soc_sunxi_ahub_dam-objs			+= snd_sunxi_ahub_dam.o
obj-$(CONFIG_SND_SOC_SUNXI_AHUB_DAM)		+= snd_soc_sunxi_ahub_dam.o

snd_soc_sunxi_ahub-objs				+= snd_sunxi_ahub.o
obj-$(CONFIG_SND_SOC_SUNXI_AHUB)		+= snd_soc_sunxi_ahub.o

# common -> machine (note: Finally compile, save system startup time)
snd_soc_sunxi_machine-objs			+= snd_sunxi_mach.o
snd_soc_sunxi_machine-objs			+= snd_sunxi_mach_utils.o
obj-$(CONFIG_SND_SOC_SUNXI_MACH)		+= snd_soc_sunxi_machine.o

ccflags-$(CONFIG_SND_SOC_SUNXI_DYNAMIC_DEBUG) += -DDYNAMIC_DEBUG_MODULE
