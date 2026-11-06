include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)

obj-m += $(CVIARCH_L)_ive.o
$(CVIARCH_L)_ive-y += common/ive_interface.o
$(CVIARCH_L)_ive-y += hal/$(CVIARCH_L)/reg.o
$(CVIARCH_L)_ive-y += hal/$(CVIARCH_L)/ive_platform.o

ccflags-y += -I$(src)/../include/common/uapi -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(src)/../include/common/kapi -I$(src)/../sys/common/uapi/
ccflags-y += -I$(src)/common -I$(src)/hal/$(CVIARCH_L) -I$(src)/hal/$(CVIARCH_L)/include
ccflags-y += -I$(src)/../base/ -I$(srctree)/drivers/staging/android
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../base/Module.symvers

else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
