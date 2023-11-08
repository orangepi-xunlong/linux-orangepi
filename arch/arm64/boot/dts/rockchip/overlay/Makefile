# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_ROCKCHIP) += \
	rk356x-uart9-m2.dtbo\
	rk356x-uart7-m2.dtbo\
	rk356x-uart3-m0.dtbo\
	rk356x-i2c2-m1.dtbo\
	rk356x-i2c3-m0.dtbo\
	rk356x-i2c4-m0.dtbo\
	rk356x-pwm11-m1.dtbo\
	rk356x-pwm15-m1.dtbo\
	rk356x-spi3-m0-cs0-spidev.dtbo\
	rk356x-gpu.dtbo

scr-$(CONFIG_ARCH_ROCKCHIP) += \
	rk356x-fixup.scr

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
	README.rockchip-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always         := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr

