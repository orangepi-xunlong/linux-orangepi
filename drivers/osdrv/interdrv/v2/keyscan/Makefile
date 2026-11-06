#include $(PWD)/../Makefile.interdrv.param

ifeq ($(CONFIG_CVITEK_KEYSCAN),y)
CONFIG_CVITEK_KEYSCAN = m
endif
obj-$(CONFIG_CVITEK_KEYSCAN) += keyscan.o

subdir-ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare
subdir-ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
