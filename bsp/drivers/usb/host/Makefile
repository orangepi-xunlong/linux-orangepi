# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the usb host drivers.
#
ccflags-y += -I $(srctree)/drivers/usb/host
ccflags-y += -I $(srctree)/bsp/drivers/usb/sunxi_usb/include

obj-$(CONFIG_USB_EHCI_HCD_SUNXI)	+= ehci-sunxi.o
obj-$(CONFIG_USB_OHCI_HCD_SUNXI)	+= ohci-sunxi.o
obj-$(CONFIG_USB_SUNXI_HCI)      	+= sunxi-hci.o
obj-$(CONFIG_USB_SUNXI_XHCI)     	+= xhci_sunxi.o

ccflags-y += -DDYNAMIC_DEBUG_MODULE
