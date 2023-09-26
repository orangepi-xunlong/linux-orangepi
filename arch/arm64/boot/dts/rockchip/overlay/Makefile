# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_ROCKCHIP) += \
	rk3588-i2c1-m2.dtbo \
	rk3588-i2c1-m4.dtbo \
	rk3588-i2c2-m0.dtbo \
	rk3588-i2c2-m4.dtbo \
	rk3588-i2c3-m0.dtbo \
	rk3588-i2c4-m3.dtbo \
	rk3588-i2c5-m3.dtbo \
	rk3588-i2c6-m4.dtbo \
	rk3588-i2c8-m2.dtbo \
	rk3588-pwm0-m0.dtbo \
	rk3588-pwm0-m1.dtbo \
	rk3588-pwm0-m2.dtbo \
	rk3588-pwm1-m0.dtbo \
	rk3588-pwm1-m1.dtbo \
	rk3588-pwm1-m2.dtbo \
	rk3588-pwm3-m0.dtbo \
	rk3588-pwm3-m2.dtbo \
	rk3588-pwm3-m3.dtbo \
	rk3588-pwm10-m0.dtbo \
	rk3588-pwm11-m0.dtbo \
	rk3588-pwm12-m0.dtbo \
	rk3588-pwm13-m0.dtbo \
	rk3588-pwm13-m2.dtbo \
	rk3588-pwm14-m0.dtbo \
	rk3588-pwm14-m1.dtbo \
	rk3588-pwm14-m2.dtbo \
	rk3588-pwm15-m1.dtbo \
	rk3588-pwm15-m2.dtbo \
	rk3588-uart0-m2.dtbo \
	rk3588-uart1-m1.dtbo \
	rk3588-uart3-m0.dtbo \
	rk3588-uart3-m1.dtbo \
	rk3588-uart4-m0.dtbo \
	rk3588-uart4-m2.dtbo \
	rk3588-uart6-m1.dtbo \
	rk3588-uart7-m2.dtbo \
	rk3588-uart8-m1.dtbo \
	rk3588-can0-m0.dtbo \
	rk3588-can1-m0.dtbo \
	rk3588-can1-m1.dtbo \
	rk3588-can2-m1.dtbo \
	rk3588-lcd1.dtbo \
	rk3588-lcd2.dtbo \
	rk3588-opi5plus-lcd.dtbo \
	rk3588-ov13850-c1.dtbo \
	rk3588-ov13850-c2.dtbo \
	rk3588-ov13850-c3.dtbo \
	rk3588-ov13855-c1.dtbo \
	rk3588-ov13855-c2.dtbo \
	rk3588-ov13855-c3.dtbo \
	rk3588-opi5plus-gc5035.dtbo \
	rk3588-opi5plus-ov13850.dtbo \
	rk3588-opi5plus-ov13855.dtbo \
	rk3588-dmc.dtbo \
	rk3588-ssd-sata0.dtbo \
	rk3588-ssd-sata2.dtbo \
	rk3588-hdmirx.dtbo \
	rk3588-hdmi2-8k.dtbo \
	rk3588-wifi-ap6275p.dtbo \
	rk3588-wifi-pcie.dtbo \
	rk3588-spi0-m2-cs0-spidev.dtbo \
	rk3588-spi0-m2-cs1-spidev.dtbo \
	rk3588-spi0-m2-cs0-cs1-spidev.dtbo \
	rk3588-spi4-m0-cs1-spidev.dtbo \
	rk3588-spi4-m1-cs0-spidev.dtbo \
	rk3588-spi4-m1-cs1-spidev.dtbo \
	rk3588-spi4-m1-cs0-cs1-spidev.dtbo \
	rk3588-spi4-m2-cs0-spidev.dtbo \
	rk3588-disable-led.dtbo \
	rk3588-opi5plus-disable-leds.dtbo \
	rk356x-edp.dtbo \
	rk356x-i2c2-m1.dtbo \
	rk356x-i2c3-m0.dtbo \
	rk356x-i2c4-m0.dtbo \
	rk356x-raspi-7inch-touchscreen.dtbo \
	rk356x-raspi-7inch-touchscreen-dsi0.dtbo \
	rk356x-spi3-m0-cs0-spidev.dtbo \
	rk356x-uart2-m0.dtbo \
	rk356x-uart3-m0.dtbo \
	rk356x-uart7-m2.dtbo \
	rk356x-uart9-m2.dtbo \
	rk356x-pwm11-m1.dtbo \
	rk356x-pwm15-m1.dtbo \
	rk356x-pwm7.dtbo \
	rk356x-ov5647-c1.dtbo \
	rk356x-ov5647-c2.dtbo

scr-$(CONFIG_ARCH_ROCKCHIP) += \
	rk3588-fixup.scr \
	rk356x-fixup.scr

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
	README.rockchip-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always-y       := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr
