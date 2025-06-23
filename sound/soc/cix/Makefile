# SPDX-License-Identifier: GPL-2.0
# Cixtech Platform Support
snd-soc-cdns-i2s-sc-objs := cdns_i2s_sc.o
snd-soc-cdns-i2s-mc-objs := cdns_i2s_mc.o
snd-soc-hda-objs := hdac.o hda_pcm.o ipb_hda.o
snd-hda-codec-realtek-objs := hdacodec.o patch_realtek.o hda_jack.o hda_proc.o
snd-soc-sky1-sound-card-objs := card-utils.o sky1-card.o

obj-$(CONFIG_SND_SOC_CDNS_I2S_SC) += snd-soc-cdns-i2s-sc.o
obj-$(CONFIG_SND_SOC_CDNS_I2S_MC) += snd-soc-cdns-i2s-mc.o
obj-$(CONFIG_SND_SOC_IPBLOQ_HDA) += snd-soc-hda.o
obj-$(CONFIG_SND_HDACODEC_REALTEK) += snd-hda-codec-realtek.o
obj-${CONFIG_SND_SOC_CIX} += snd-soc-sky1-sound-card.o
