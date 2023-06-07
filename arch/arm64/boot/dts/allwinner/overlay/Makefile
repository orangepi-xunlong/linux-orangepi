# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_SUNXI) += \
	sun50i-h616-gpu.dtbo \
	sun50i-h616-uart2.dtbo \
	sun50i-h616-uart5.dtbo \
	sun50i-h616-i2c1.dtbo \
	sun50i-h616-i2c2.dtbo \
	sun50i-h616-i2c3.dtbo \
	sun50i-h616-i2c4.dtbo \
	sun50i-h616-pwm12.dtbo \
	sun50i-h616-pwm34.dtbo \
	sun50i-h616-spi0-spidev.dtbo \
	sun50i-h616-spi1-spidev.dtbo \
	sun50i-h616-disable-leds.dtbo \
	sun50i-h616-disable-uart0.dtbo

scr-$(CONFIG_ARCH_SUNXI) += \
	sun50i-h616-fixup.scr

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always		:= $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files	:= *.dtbo *.scr
