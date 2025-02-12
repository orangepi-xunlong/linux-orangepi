dtbo-$(CONFIG_SOC_KY_X1) += \
	x1-i2c3.dtbo \
	x1-i2c4.dtbo \
	x1-can0.dtbo \
	x1-uart5.dtbo \
	x1-uart8.dtbo \
	x1-uart9.dtbo \
	x1-pwm7.dtbo \
	x1-pwm9.dtbo \
	x1-lcd.dtbo \
	x1-spi3-cs0-spidev.dtbo \

#scr-$(CONFIG_SOC_KY_X1) += \
#        sun55i-t527-fixup.scr
#
#dtbotxt-$(CONFIG_SOC_KY_X1) += \
#        README.sun55i-t527-overlays
#
targets += $(dtbo-y)
#$(scr-y) $(dtbotxt-y)

always-y       := $(dtbo-y)
#$(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo
#*.scr
