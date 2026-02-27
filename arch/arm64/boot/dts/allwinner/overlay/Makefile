# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_SUNXI) += \
	sun55i-t527-lcd.dtbo \
	sun55i-t527-edp.dtbo \
	sun55i-t527-i2c1.dtbo \
	sun55i-t527-i2c4.dtbo \
	sun55i-t527-i2c5.dtbo \
	sun55i-t527-spi1-cs0-spidev.dtbo \
	sun55i-t527-spi2-cs0-spidev.dtbo \
	sun55i-t527-uart2.dtbo \
	sun55i-t527-uart3.dtbo \
	sun55i-t527-uart4.dtbo \
	sun55i-t527-uart5.dtbo \
	sun55i-t527-uart6.dtbo \
	sun55i-t527-uart7.dtbo \
	sun55i-t527-pwm10.dtbo \
	sun55i-t527-pwm11.dtbo \
	sun55i-t527-pwm12.dtbo \
	sun55i-t527-pwm13.dtbo \
	sun55i-t527-pwm14.dtbo \
	sun55i-t527-pwm15.dtbo \
	sun55i-t527-pwm1.dtbo \
	sun55i-t527-pwm2.dtbo \
	sun55i-t527-pwm4.dtbo \
	sun55i-t527-pwm5.dtbo \
	sun55i-t527-pwm6.dtbo \
	sun55i-t527-pwm7.dtbo \
	sun55i-t527-pwm8.dtbo \
	sun55i-t527-pwm9.dtbo \
	sun60i-t736-lcd.dtbo \
	sun60i-t736-spi4-spidev.dtbo  \
	sun60i-t736-uart5.dtbo \
	sun60i-t736-i2c3.dtbo \
	sun60i-t736-pwm1.dtbo \
	sun60i-a733-i2c0.dtbo \
	sun60i-a733-i2c1.dtbo \
	sun60i-a733-i2c2.dtbo \
	sun60i-a733-i2c3.dtbo \
	sun60i-a733-lcd.dtbo \
	sun60i-a733-pwm0.dtbo \
	sun60i-a733-pwm1.dtbo \
	sun60i-a733-pwm2.dtbo \
	sun60i-a733-pwm3.dtbo \
	sun60i-a733-pwm4.dtbo \
	sun60i-a733-pwm5.dtbo \
	sun60i-a733-pwm6.dtbo \
	sun60i-a733-pwm7.dtbo \
	sun60i-a733-pwm8.dtbo \
	sun60i-a733-pwm9.dtbo \
	sun60i-a733-spi3-cs0-cs1-spidev.dtbo \
	sun60i-a733-spwm2.dtbo \
	sun60i-a733-uart2.dtbo \
	sun60i-a733-uart6.dtbo \
	sun60i-a733-uart7.dtbo \
	sun60i-a733-uart8.dtbo \

#scr-$(CONFIG_ARCH_SUNXI) += \
#	sun55i-t527-fixup.scr

#dtbotxt-$(CONFIG_ARCH_SUNXI) += \
#	README.sun55i-t527-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always-y       := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr
