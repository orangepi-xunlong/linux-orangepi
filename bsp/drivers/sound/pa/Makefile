# SPDX-License-Identifier: GPL-2.0-or-later
#
# Allwinner audio PA driver Makefile
#
# Copyright(c) 2022-2027 Allwinnertech Co., Ltd.

ccflags-y += -include $(srctree)/bsp/drivers/sound/pa/snd_sunxi_pa.h

#for AWINIC AW87XXX Smart K PA
snd_soc_aw87x_pa-objs := aw87xxx/aw87xxx.o \
		aw87xxx/aw87xxx_device.o \
		aw87xxx/aw87xxx_monitor.o \
		aw87xxx/aw87xxx_bin_parse.o \
		aw87xxx/aw87xxx_dsp.o \
		aw87xxx/aw87xxx_acf_bin.o

obj-$(CONFIG_SND_SOC_AW87XXX) = snd_soc_aw87x_pa.o

#for ESMT AD8XXXX series PA
snd-soc-ad8xxxx-objs := ad8xxxx/ad82128.o
obj-$(CONFIG_SND_SOC_AD8XXXX) = snd-soc-ad8xxxx.o

#for AWINIC AW882XX Smart K PA
snd_smartpa_aw882xx_pa-objs := aw882xx/aw882xx.o \
        		aw882xx/aw882xx_monitor.o \
			aw882xx/aw882xx_init.o \
        		aw882xx/aw882xx_dsp.o \
			aw882xx/aw882xx_device.o \
        		aw882xx/aw882xx_calib.o \
			aw882xx/aw882xx_bin_parse.o \
        		aw882xx/aw882xx_spin.o

obj-$(CONFIG_SND_SMARTPA_AW882XX) = snd_smartpa_aw882xx_pa.o
