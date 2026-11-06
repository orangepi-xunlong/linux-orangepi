include $(src)/../../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(src)/chip.mk

obj-m += $(CVIARCH_L)_mipi_tx.o

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ccflags-y += -I$(src)/chip/$(CVIARCH_L)/ -I$(src)/common/ -I$(src)/../chip/$(CVIARCH_L)
ccflags-y += -I$(src)/../../include/common/uapi/ -I$(src)/../../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../../include/common/kapi/ -I$(src)/../../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(src)/../../sys/common/ -I$(src)/../../sys/common/uapi
ccflags-y += -I$(src)/../../base/chip/$(CVIARCH_L)/
ccflags-y += -I$(src)/../../base/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/

KBUILD_EXTRA_SYMBOLS = $(src)/../../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
