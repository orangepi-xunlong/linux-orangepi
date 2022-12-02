PWD := $(shell pwd)
KVER   := $(shell uname -r)
KSRC   := /lib/modules/$(KVER)/build
MODULE_NAME := 8822BS_BT_EN
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers

$(MODULE_NAME)-objs := 	BT_GPIO.o\
			BT_GPIO_handle.o

obj-m := $(MODULE_NAME).o


all: bt_en rtk_hciattach


bt_en:
	$(MAKE) -C $(KSRC) M=$(PWD) modules

rtk_hciattach: hciattach.c hciattach_rtk.o
	$(CC) -o 8822b_hciattach hciattach.c hciattach_rtk.o

hciattach_rtk.o:hciattach_rtk.c
	$(CC) -c hciattach_rtk.c

install :
	sudo rm -r -f /lib/firmware/rtlbt/
	sudo mkdir /lib/firmware/rtlbt/
	sudo cp ./rtl8822b_fw.bin /lib/firmware/rtlbt/rtl8822b_fw
	sudo cp ./rtl8822b_config.bin /lib/firmware/rtlbt/rtl8822b_config
	sudo insmod $(MODULE_NAME).ko
#	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
#	/sbin/depmod -a ${KVER}

uninstall :
	sudo rm -r -f /lib/firmware/rtlbt/
	rmmod $(MODULE_NAME)
#	modprobe -r $(MODULE_NAME)
#	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
#	/sbin/depmod -a ${KVER}

clean:
	rm -f *.o 8822b_hciattach tags cscope.*
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko .tmp_versions Module.symvers modules.order

tags: FORCE
	ctags -R
	find ./ -name "*.h" -o -name "*.c" -o -name "*.cc" -o -name "*.cpp" > cscope.files
	cscope -bkq -i cscope.files
PHONY += FORCE
FORCE:
