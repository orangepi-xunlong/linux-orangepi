obj-$(CONFIG_USB_CVITEK_DRD)			+= cv183x_usb_drd.o

cv183x_usb_drd-y				:= cviusb_drd.o otg_fsm.o

ifneq ($(CONFIG_USB_CVITEK_MISC),)
	cv183x_usb_drd-y			+= cviusb_misc.o
endif

obj-$(CONFIG_USB_CVITEK_GADGET)			+= cv183x_usb_gadget.o
cv183x_usb_gadget-y				:= cviusb_gadget.o debug.o

ifneq ($(CONFIG_USB_CVITEK_MISC),)
	cv183x_usb_gadget-y			+= cviusb_misc.o
endif

##
# Platform-specific glue layers go here
#
# NOTICE: Make sure your glue layer doesn't depend on anything
# which is arch-specific and that it compiles on all situations.
#
# We want to keep this requirement in order to be able to compile
# the entire driver (with all its glue layers) on several architectures
# and make sure it compiles fine. This will also help with allmodconfig
# and allyesconfig builds.
##
