# SPDX-License-Identifier: GPL-2.0
ccflags-y += -I $(srctree)/drivers/clocksource
obj-$(CONFIG_AW_TIMER)		+= timer-sun4i.o
obj-$(CONFIG_AW_TIMER_SUN50I)	+= timer-sun50i.o

ccflags-$(CONFIG_AW_TIMER_DYNAMIC_DEBUG) += -DDYNAMIC_DEBUG_MODULE
