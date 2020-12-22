ccflags-$(CONFIG_USB_SUNXI_DEBUG)	:= -DDEBUG
ccflags-$(CONFIG_USB_SUNXI_VERBOSE)	+= -DVERBOSE_DEBUG

obj-$(CONFIG_USB_SUNXI)			+= sunxi.o

sunxi-y					:= core.o
sunxi-y					+= gadget.o
sunxi-y					+= ep0.o
sunxi-y					+= osal.o
sunxi-y					+= libusb300.a
sunxi-y					+= host.o


ifneq ($(CONFIG_DEBUG_FS),)
	sunxi-y				+= debugfs.o
endif



quiet_cmd_libusb300 = COPY    $@
      cmd_libusb300 = cp -vfr $(obj)/libusb300 $@

$(obj)/libusb300.a: $(obj)/libusb300
	$(call if_changed,libusb300)

$(obj)/libusb300: FORCE
	$(if $(wildcard $(obj)/library_usb300/Makefile), \
		$(Q)$(MAKE) $(build)=$(obj)/library_usb300 libusb300_install, \
		)
