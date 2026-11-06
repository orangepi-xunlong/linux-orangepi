obj-m += ms9132.o
ms9132-y += usbdisp_fbdev.o hal_adaptor.o usb_device.o usb_hal_interface.o usb_hal_sysfs.o usb_hal_thread.o

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	rm -rf .tmp_versions