obj-$(CONFIG_USB_BM_DRD)			+= bmusb-drd.o

bmusb-drd-y					:= bmusb_drd.o otg_fsm.o

ifneq ($(CONFIG_USB_BM_MISC),)
	bmusb-drd-y				+= bmusb_misc.o
endif

obj-$(CONFIG_USB_BM_GADGET)			+= bmusb-gadget.o
bmusb-gadget-y				:= bmusb_gadget.o debug.o

ifneq ($(CONFIG_USB_BM_MISC),)
	bmusb-gadget-y				+= bmusb_misc.o
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
