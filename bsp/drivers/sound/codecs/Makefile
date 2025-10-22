# SPDX-License-Identifier: GPL-2.0-or-later
#
# Allwinner audio codecs driver Makefile
#
# Copyright(c) 2022-2027 Allwinnertech Co., Ltd.

ccflags-y += -I $(srctree)/bsp/drivers/sound/platform
ccflags-y += -I $(srctree)/bsp/drivers/sound/adapter
# Sunxi
snd_soc_ac101-objs := ac101.o
snd_soc_ac101b-objs := ac101b.o
snd-soc-ac107-objs := ac107.o
snd-soc-ac108-objs := ac108.o
snd-soc-es8323-objs := es8323.o

# Fsm
snd-soc-fsm-objs   := fsm/fsm_core.o fsm/fsm_i2c.o fsm/fs16xx/fs1603.o

# Sunxi
obj-$(CONFIG_SND_SOC_AC101)	+= snd_soc_ac101.o
obj-$(CONFIG_SND_SOC_AC101B)	+= snd_soc_ac101b.o
obj-$(CONFIG_SND_SOC_AC107)	+= snd-soc-ac107.o
obj-$(CONFIG_SND_SOC_AC108)	+= snd-soc-ac108.o
# Fsm
obj-$(CONFIG_SND_SOC_FS16XX)+= snd-soc-fsm.o

obj-$(CONFIG_SND_SOC_ES8323)    += snd-soc-es8323.o
