# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the Allwinner device drivers.
#

ccflags-$(CONFIG_RTL8363_NB) += -I$(src)/../../phy/
ccflags-$(CONFIG_RTL8363_NB) += -I$(src)/../../phy/rtl8363_nb
obj-$(CONFIG_SUN4I_EMAC) += sun4i-emac.o
obj-$(CONFIG_SUNXI_GMAC) += sunxi_gmac.o
sunxi_gmac-objs := sunxi-gmac.o sunxi-gmac-ops.o
