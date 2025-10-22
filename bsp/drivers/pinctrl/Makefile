# SPDX-License-Identifier: GPL-2.0
# Core
ccflags-y += -I $(srctree)/drivers/pinctrl
obj-$(CONFIG_AW_PINCTRL)			+= pinctrl-sunxi.o
obj-$(CONFIG_AW_PINCTRL_DEBUGFS)		+= pinctrl-debugfs.o
obj-$(CONFIG_AW_PINCTRL_LEDS)			+= pinctrl-leds.o
# Private SoC Drivers
obj-$(CONFIG_AW_PINCTRL_SUN8IW11)		+= pinctrl-sun8iw11.o
obj-$(CONFIG_AW_PINCTRL_SUN8IW18)		+= pinctrl-sun8iw18p1.o
obj-$(CONFIG_AW_PINCTRL_SUN8IW15P1)		+= pinctrl-sun8iw15p1.o
obj-$(CONFIG_AW_PINCTRL_SUN8IW15P1_R)		+= pinctrl-sun8iw15p1-r.o
obj-$(CONFIG_AW_PINCTRL_SUN8IW20)		+= pinctrl-sun8iw20.o
obj-$(CONFIG_AW_PINCTRL_SUN8IW21)		+= pinctrl-sun8iw21.o
obj-$(CONFIG_AW_PINCTRL_SUN50IW9)		+= pinctrl-sun50iw9.o
obj-$(CONFIG_AW_PINCTRL_SUN50IW9_R)		+= pinctrl-sun50iw9-r.o
obj-$(CONFIG_AW_PINCTRL_SUN55IW3)		+= pinctrl-sun55iw3.o
obj-$(CONFIG_AW_PINCTRL_SUN55IW3_R)		+= pinctrl-sun55iw3-r.o
obj-$(CONFIG_AW_PINCTRL_SUN55IW5)		+= pinctrl-sun55iw5.o
obj-$(CONFIG_AW_PINCTRL_SUN55IW6)		+= pinctrl-sun55iw6.o
obj-$(CONFIG_AW_PINCTRL_SUN50IW10P1)		+= pinctrl-sun50iw10p1.o
obj-$(CONFIG_AW_PINCTRL_SUN50IW10P1_R)		+= pinctrl-sun50iw10p1-r.o
obj-$(CONFIG_AW_PINCTRL_SUN50IW12)		+= pinctrl-sun50iw12.o
obj-$(CONFIG_AW_PINCTRL_SUN50IW12_R)		+= pinctrl-sun50iw12-r.o
obj-$(CONFIG_AW_PINCTRL_SUN60IW1)		+= pinctrl-sun60iw1.o
obj-$(CONFIG_AW_PINCTRL_SUN60IW2)		+= pinctrl-sun60iw2.o
obj-$(CONFIG_AW_PINCTRL_SUN60IW2_R)		+= pinctrl-sun60iw2-r.o
obj-$(CONFIG_AW_PINCTRL_SUN20IW5)		+= pinctrl-sun20iw5.o
obj-$(CONFIG_AW_PINCTRL_SUN65IW1)		+= pinctrl-sun65iw1.o
obj-$(CONFIG_AW_PINCTRL_AXP22X)			+= pinctrl-axp22x.o

ccflags-$(CONFIG_AW_PINCTRL_DYNAMIC_DEBUG) += -DDYNAMIC_DEBUG_MODULE
