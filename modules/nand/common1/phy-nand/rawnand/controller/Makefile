#
#
# Makefile for controller
#
#

obj-$(CONFIG_SUNXI_RAWNAND) += ndfc_base.o

obj-$(CONFIG_SUNXI_RAWNAND) += ndfc_timings.o
ifdef CONFIG_ARCH_SUN8IW18
obj-$(CONFIG_SUNXI_RAWNAND) += ndfc_v2px.o
else
obj-$(CONFIG_SUNXI_RAWNAND) += ndfc_v1px.o
endif
