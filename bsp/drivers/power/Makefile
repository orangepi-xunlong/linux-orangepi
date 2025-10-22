# SPDX-License-Identifier: GPL-2.0-only

# include
ccflags-y += -I $(srctree)/bsp/drivers/power/include

# version
VERSION_FILE := $(srctree)/bsp/drivers/power/version
VERSION_PREFIX := SUNXI_POWER
VERSION := $(shell grep -m 1 'version:' $(VERSION_FILE) | awk '{print $$2}')
CFLAGS += -D$(VERSION_PREFIX)="\"$(VERSION)\""
$(info sunxi power version is $(VERSION))

# core
obj-$(CONFIG_AW_MFD_AXP2101)		+= mfd/axp2101.o
obj-$(CONFIG_AW_MFD_AXP2101_I2C)	+= mfd/axp2101-i2c.o
obj-$(CONFIG_AW_MFD_PMU_EXT)		+= mfd/pmu-ext-core.o
obj-$(CONFIG_AW_MFD_PMU_EXT_I2C)	+= mfd/pmu-ext-i2c.o
obj-$(CONFIG_AW_MFD_BMU_EXT)		+= mfd/bmu-ext-core.o
obj-$(CONFIG_AW_MFD_BMU_EXT_I2C)	+= mfd/bmu-ext-i2c.o

# notifier
obj-$(CONFIG_AW_POWER_NOTIFIER)		+= notifier/sunxi-power-notifier.o

# temp_ctrl
obj-$(CONFIG_AW_AXP8191_TEMP_CTRL)	+= temp_ctrl/axp8191_temp_ctrl.o

# power_key
obj-$(CONFIG_AW_INPUT_AXP2101_PEK)	+= power_key/axp2101-pek.o

# regulator
obj-$(CONFIG_AW_REGULATOR_AXP2101)	+= regulator/axp2101-regulator.o
obj-$(CONFIG_AW_REGULATOR_PMU_EXT)	+= regulator/pmu-ext-regulator.o
obj-$(CONFIG_AW_REGULATOR_BMU_EXT)	+= regulator/bmu-ext-regulator.o
obj-$(CONFIG_AW_REGULATOR_PWM) 		+= regulator/sunxi_pwm_regulator.o
obj-$(CONFIG_AW_REGULATOR_SOC_PMU)	+= regulator/sunxi-soc-regulator.o
obj-$(CONFIG_AW_REGULATOR_VIRTUAL_CONSUMER) 	+= regulator/sunxi_virtual_consumer.o

# supply
obj-$(CONFIG_AW_AXP22X_POWER)		+= supply/axp22x_usb_power.o
obj-$(CONFIG_AW_AXP22X_POWER)		+= supply/axp22x_ac_power.o
obj-$(CONFIG_AW_AXP22X_POWER)		+= supply/axp22x_battery.o

obj-$(CONFIG_AW_AXP803_POWER)		+= supply/axp803_usb_power.o
obj-$(CONFIG_AW_AXP803_POWER)		+= supply/axp803_ac_power.o
obj-$(CONFIG_AW_AXP803_POWER)		+= supply/axp803_battery.o

obj-$(CONFIG_AW_AXP2101_POWER)		+= supply/axp2101_usb_power.o
obj-$(CONFIG_AW_AXP2101_POWER)		+= supply/axp2101_battery.o

obj-$(CONFIG_AW_AXP2202_POWER)		+= supply/axp2202_usb_power.o
obj-$(CONFIG_AW_AXP2202_POWER)		+= supply/axp2202_battery.o
obj-$(CONFIG_AW_AXP2202_GPIO_POWER) += supply/axp2202_gpio_power.o

obj-$(CONFIG_AW_AXP515_POWER)		+= supply/axp515_usb_power.o
obj-$(CONFIG_AW_AXP515_POWER)		+= supply/axp515_battery.o
obj-$(CONFIG_AW_AXP515_ACIN_POWER)  += supply/axp515_acin_power.o

obj-$(CONFIG_AW_ETA6973_POWER)		+= supply/eta6973_charger_power.o
obj-$(CONFIG_AW_AXP519_POWER)		+= supply/axp519_charger_power.o
obj-$(CONFIG_AW_AXP2601_POWER)		+= supply/axp2601_battery.o
obj-$(CONFIG_AW_AXP2602_POWER)		+= supply/axp2602_battery.o

obj-$(CONFIG_AW_VIRTUAL_AC_POWER)	+= supply/sunxi_ac_virtual_power.o
obj-$(CONFIG_AW_GPIO_VBUS_POWER)	+= supply/sunxi_gpio_vbus_power.o
