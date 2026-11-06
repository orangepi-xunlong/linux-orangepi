# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_CVITEK) += \
	bm1688-lcd.dtbo \

scr-$(CONFIG_ARCH_CVITEK) += \
	bm1688-fixup.scr

targets += $(dtbo-y) $(scr-y)

always-y       := $(dtbo-y) $(scr-y)
clean-files    := *.dtbo *.scr
