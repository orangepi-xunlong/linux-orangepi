# SPDX-License-Identifier: GPL-2.0-only
# Core
ccflags-y += -I $(srctree)/drivers/usb/dwc3

# Host
ccflags-y += -I $(srctree)/drivers/usb/host

obj-$(CONFIG_PHY_SUNXI_PLAT)		+= phy-sunxi-plat.o
obj-$(CONFIG_DWC3_SUNXI_PLAT)		+= dwc3-sunxi-plat.o

ccflags-y += -DDYNAMIC_DEBUG_MODULE
