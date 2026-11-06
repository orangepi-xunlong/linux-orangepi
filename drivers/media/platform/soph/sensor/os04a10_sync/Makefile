# SPDX-License-Identifier: GPL-2.0
ifneq ($(KERNELRELEASE),)

v4l2_os04a10_sync-objs += os04a10_sync.o

obj-m += v4l2_os04a10_sync.o

ccflags-y += -I$(PWD)/osdrv_include/common/uapi/ -I$(PWD)/osdrv_include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/osdrv_include/common/kapi/ -I$(PWD)/osdrv_include/chip/$(CVIARCH_L)/kapi/

else

all:
	$(MAKE) ARCH=$(ARCH) $(CFLAGS) $(EXTRA_CFLAGS) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete

endif