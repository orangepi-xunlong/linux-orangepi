# SPDX-License-Identifier: GPL-2.0

linlon-aeu-y := linlon_aeu.o linlon_aeu_v4l2.o linlon_aeu_hw.o linlon_aeu_log.o

ifneq ($(CONFIG_VIDEO_ADV_DEBUG),)
    linlon-aeu-y += linlon_aeu_reg_dump.o
endif

obj-$(CONFIG_LINLON_AEU) += linlon-aeu.o
