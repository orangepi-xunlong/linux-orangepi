include $(PWD)/../Makefile.interdrv.param

BMUSB = bmusb
CVIUSB = cviusb
GADGET_FUNC = gadget/function

obj-$(CONFIG_USB_BM_DRD)			+= bmusb-drd.o

bmusb-drd-y					:= $(BMUSB)/bmusb_drd.o \
							$(BMUSB)/otg_fsm.o

ifneq ($(CONFIG_USB_BM_MISC),)
	bmusb-drd-y				+= $(BMUSB)/bmusb_misc.o
endif

obj-$(CONFIG_USB_BM_GADGET)			+= bmusb-gadget.o
bmusb-gadget-y					:= $(BMUSB)/bmusb_gadget.o \
							$(BMUSB)/debug.o

ifneq ($(CONFIG_USB_BM_MISC),)
	bmusb-gadget-y				+= $(BMUSB)/bmusb_misc.o
endif

obj-$(CONFIG_USB_CVITEK_DRD)			+= cv183x_usb_drd.o

cv183x_usb_drd-y				:= $(CVIUSB)/cviusb_drd.o \
							$(CVIUSB)/otg_fsm.o

ifneq ($(CONFIG_USB_CVITEK_MISC),)
	cv183x_usb_drd-y			+= $(CVIUSB)/cviusb_misc.o
endif

obj-$(CONFIG_USB_CVITEK_GADGET)			+= cv183x_usb_gadget.o
cv183x_usb_gadget-y				:= $(CVIUSB)/cviusb_gadget.o \
							$(CVIUSB)/debug.o

ifneq ($(CONFIG_USB_CVITEK_MISC),)
	cv183x_usb_gadget-y			+= $(CVIUSB)/cviusb_misc.o
endif

usb_f_bcm-y			:= $(GADGET_FUNC)/f_bcm.o
obj-$(CONFIG_USB_F_BCM)		+= usb_f_bcm.o
usb_bm_serial-y			:= $(GADGET_FUNC)/bm_serial.o
obj-$(CONFIG_USB_BM_SERIAL)	+= usb_bm_serial.o
cvi_usb_f_cvg-y			:= $(GADGET_FUNC)/f_cvg.o
obj-m		+= cvi_usb_f_cvg.o

ccflags-y += -I$(srctree)/drivers/usb/
ccflags-y += -I$(srctree)/drivers/usb/gadget/
ccflags-$(CONFIG_CVITEK_PINCTRL_CV1835) += -I$(srctree)/drivers/pinctrl/cvitek/

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f $(CVIUSB)/*.o $(CVIUSB)/.*.cmd
	rm -f $(BMUSB)/*.o $(BMUSB)/.*.cmd
	rm -f $(GADGET_FUNC)/*.o $(GADGET_FUNC)/.*.cmd
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	rm -rf .tmp_versions
