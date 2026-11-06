include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk
ifeq ($(CVIARCH_L), soph)
$(CVIARCH_L)_ldc-y += common/ldc_proc.o
$(CVIARCH_L)_ldc-y += common/ldc_core.o
else
$(CVIARCH_L)_ldc-y += common/gdc_proc.o
$(CVIARCH_L)_ldc-y += common/ldc_platform.o
endif
obj-m += $(CVIARCH_L)_ldc.o
#ccflags-y += -DPORTING_TEST
ccflags-y += -I$(src)/chip/$(CVIARCH_L)/ -I$(src)/common/
ccflags-y += -I$(src)/../include/common/uapi/ -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/common/kapi/ -I$(src)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(src)/../include/
ccflags-y += -I$(src)/../base/
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)
ccflags-y += -I$(src)/../sys/common/ -I$(src)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../Module.symvers
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o* *.ko
	@rm -f Module.symvers modules.order
	@rm -f *.mod.c
	@rm -rf .tmp_versions
	@rm -f .*cmd
	@rm -f common/*.o*
	@rm -f common/.*cmd
	@rm -f $(CHIP)/*.o*
	@rm -f $(CHIP)/.*cmd
	@find . -name \*.o* -type f -delete
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko  *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
