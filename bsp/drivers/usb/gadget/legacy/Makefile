# SPDX-License-Identifier: GPL-2.0
#
# USB gadget drivers
#

ccflags-y			:= -I $(srctree)/drivers/usb/gadget/
ccflags-y			+= -I $(srctree)/drivers/usb/gadget/udc/
ccflags-y			+= -I $(srctree)/drivers/usb/gadget/function/

g_sunxi_webcam-y			:= webcam.o
obj-$(CONFIG_USB_SUNXI_G_WEBCAM)	+= g_sunxi_webcam.o
