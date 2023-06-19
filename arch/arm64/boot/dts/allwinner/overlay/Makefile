# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_SUNXI) += \
	sun50i-h616-ir.dtbo \
	sun50i-h616-gpu.dtbo \
	sun50i-h616-ph-uart2.dtbo \
	sun50i-h616-ph-uart5.dtbo \
	sun50i-h616-ph-i2c1.dtbo \
	sun50i-h616-ph-i2c2.dtbo \
	sun50i-h616-ph-i2c3.dtbo \
	sun50i-h616-ph-i2c4.dtbo \
	sun50i-h616-ph-pwm12.dtbo \
	sun50i-h616-ph-pwm34.dtbo \
	sun50i-h616-usb0-host.dtbo \
	sun50i-h616-spi0-spidev.dtbo \
	sun50i-h616-spi1-cs0-spidev.dtbo \
	sun50i-h616-spi1-cs1-spidev.dtbo \
	sun50i-h616-spi1-cs0-cs1-spidev.dtbo \
	sun50i-h616-disable-leds.dtbo \
	sun50i-h616-disable-uart0.dtbo \
	sun50i-h616-zero2w-disable-led.dtbo \
	sun50i-h616-pi-i2c0.dtbo \
	sun50i-h616-pi-i2c1.dtbo \
	sun50i-h616-pi-i2c2.dtbo \
	sun50i-h616-pi-pwm1.dtbo \
	sun50i-h616-pi-pwm2.dtbo \
	sun50i-h616-pi-pwm3.dtbo \
	sun50i-h616-pi-pwm4.dtbo \
	sun50i-h616-pi-uart2.dtbo \
	sun50i-h616-pi-uart3.dtbo \
	sun50i-h616-pi-uart4.dtbo

scr-$(CONFIG_ARCH_SUNXI) += \
	sun50i-h616-fixup.scr

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always		:= $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files	:= *.dtbo *.scr
