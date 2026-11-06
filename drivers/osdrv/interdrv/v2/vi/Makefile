#include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk

$(CVIARCH_L)_vi-objs += common/vi_core.o

obj-m += $(CVIARCH_L)_vi.o

ccflags-y += -I$(src)/chip/$(CVIARCH_L)/ -I$(src)/common/
ccflags-y += -I$(src)/../include/common/uapi/ -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/common/kapi/ -I$(src)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(src)/../include/
ccflags-y += -I$(src)/../base/
ccflags-y += -I$(src)/../sys/chip/$(CVIARCH_L)
ccflags-y += -I$(src)/../sys/common/ -I$(src)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android

ifeq ($(SUBTYPE), fpga)
ccflags-y += -DFPGA_PORTING
endif

# for isp bringup porting test
ccflags-y += -DPORTING_TEST

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
