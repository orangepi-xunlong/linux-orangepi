# SPDX-License-Identifier: GPL-2.0

# DEVFREQ Drivers
ifneq ($(strip $(CONFIG_ARCH_SUN55IW3)),)
obj-$(CONFIG_AW_DMC_DEVFREQ)	+= sun55iw3-devfreq.o
else ifneq ($(strip $(CONFIG_ARCH_SUN65IW1)),)
obj-$(CONFIG_AW_DMC_DEVFREQ)	+= sun55iw3-devfreq.o
else ifneq ($(strip $(CONFIG_ARCH_SUN55IW6)),)
obj-$(CONFIG_AW_DMC_DEVFREQ)	+= sun55iw3-devfreq.o
else ifneq ($(strip $(CONFIG_ARCH_SUN60IW2)),)
obj-$(CONFIG_AW_DMC_DEVFREQ)	+= sun55iw3-devfreq.o
else ifneq ($(strip $(CONFIG_ARCH_SUN50IW10)),)
obj-$(CONFIG_AW_DMC_DEVFREQ)	+= sunxi-dmc.o
else
endif

# DEVFREQ Event Drivers
obj-$(CONFIG_PM_DEVFREQ_EVENT)		+= event/
ccflags-y := -DDYNAMIC_DEBUG_MODULE