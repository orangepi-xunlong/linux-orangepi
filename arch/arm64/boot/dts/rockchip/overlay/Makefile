# SPDX-License-Identifier: GPL-2.0
dtbo-$(CONFIG_ARCH_ROCKCHIP) += \
	rockchip-i2c7.dtbo \
	rockchip-i2c8.dtbo \
	rockchip-pcie-gen2.dtbo \
	rockchip-rk3328-opp-1.4ghz.dtbo \
	rockchip-rk3328-opp-1.5ghz.dtbo \
	rockchip-rk3399-opp-2ghz.dtbo \
	rockchip-spi-jedec-nor.dtbo \
	rockchip-spi-spidev.dtbo \
	rockchip-uart4.dtbo \
	rockchip-dwc3-0-host.dtbo \
	rockchip-w1-gpio.dtbo

scr-$(CONFIG_ARCH_ROCKCHIP) += \
       rockchip-fixup.scr

dtbotxt-$(CONFIG_ARCH_ROCKCHIP) += \
       README.rockchip-overlays

targets += $(dtbo-y) $(scr-y) $(dtbotxt-y)

always         := $(dtbo-y) $(scr-y) $(dtbotxt-y)
clean-files    := *.dtbo *.scr

