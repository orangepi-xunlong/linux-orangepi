# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_MESON) += \
	meson-sm1-gpu.dtbo \
	meson-sm1-spifc.dtbo \
	meson-sm1-spicc1.dtbo \
	meson-sm1-i2cAO.dtbo \
	meson-sm1-uart_AO_B.dtbo

scr-$(CONFIG_ARCH_MESON) += \
       meson-fixup.scr

dtbotxt-$(CONFIG_ARCH_MESON) += \
       README.meson-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always         := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr
