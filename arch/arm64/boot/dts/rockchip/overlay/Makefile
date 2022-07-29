# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_ROCKCHIP) += \
	rockchip-i2c2.dtbo \
	rockchip-i2c4.dtbo \
	rockchip-i2c7.dtbo \
	rockchip-i2c8.dtbo \
	rockchip-pcie-gen2.dtbo \
	rockchip-spi-jedec-nor.dtbo \
	rockchip-spi-spidev.dtbo \
	rockchip-uart4.dtbo \
	rockchip-dwc3-0-host.dtbo \
	rockchip-w1-gpio.dtbo \
	rockchip-lcd1-sl101.dtbo \
	rockchip-lcd1-afj101.dtbo

scr-$(CONFIG_ARCH_ROCKCHIP) += \
       rockchip-fixup.scr

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
       README.rockchip-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always         := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr

