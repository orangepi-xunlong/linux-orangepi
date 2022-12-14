# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_ROCKCHIP) += \
	rk3588-i2c1-m2.dtbo \
	rk3588-i2c1-m4.dtbo \
	rk3588-i2c3-m0.dtbo \
	rk3588-i2c5-m3.dtbo \
	rk3588-pwm0-m1.dtbo \
	rk3588-pwm1-m1.dtbo \
	rk3588-pwm1-m2.dtbo \
	rk3588-pwm3-m0.dtbo \
	rk3588-pwm3-m2.dtbo \
	rk3588-pwm13-m2.dtbo \
	rk3588-pwm14-m1.dtbo \
	rk3588-pwm15-m2.dtbo \
	rk3588-spi4-m0-cs1-spidev.dtbo \
	rk3588-uart0-m2.dtbo \
	rk3588-uart1-m1.dtbo \
	rk3588-uart3-m0.dtbo \
	rk3588-uart4-m0.dtbo \
	rk3588-can1-m1.dtbo \
	rk3588-can2-m1.dtbo \
	rk3588-lcd1.dtbo \
	rk3588-lcd2.dtbo \
	rk3588-wifi-ap6275p.dtbo \
	rk3588-ov13850-c1.dtbo \
	rk3588-ov13850-c2.dtbo \
	rk3588-ov13850-c3.dtbo \
	rk3588-ov13855-c1.dtbo \
	rk3588-ov13855-c2.dtbo \
	rk3588-ov13855-c3.dtbo \
	rk3588-ssd-sata.dtbo

scr-$(CONFIG_ARCH_ROCKCHIP) += \
	rk3588-fixup.scr

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
	README.rockchip-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always-y       := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr
