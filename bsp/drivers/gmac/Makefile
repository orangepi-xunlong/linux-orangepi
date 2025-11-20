# SPDX-License-Identifier: GPL-2.0-only
ccflag-y+= -DDYNAMIC_DEBUG_MODULE

ccflags-y += -I $(srctree)/bsp/drivers/gmac
obj-$(CONFIG_AW_GMAC) += sunxi-gmac.o
obj-$(CONFIG_AW_GMAC_MDIO) += sunxi-mdio.o
obj-$(CONFIG_AW_EPHY) += sunxi-ephy.o
