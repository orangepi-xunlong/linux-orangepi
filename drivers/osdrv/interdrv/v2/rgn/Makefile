include $(src)/../Makefile.interdrv.param

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk

$(CVIARCH_L)_rgn-objs += common/rgn_core.o

obj-m += $(CVIARCH_L)_rgn.o

ccflags-y += -I$(src)/common/ \
             -I$(src)/../base/ \
             -I$(src)/../base/chip/$(CVIARCH_L) \
             -I$(src)/../sys/common/ \
             -I$(src)/../sys/common/uapi \
             -I$(srctree)/drivers/staging/android \
             -I$(src)/../include/common/uapi/ \
             -I$(src)/../include/chip/$(CVIARCH_L)/uapi/ \
             -I$(src)/../include/common/kapi/ \
             -I$(src)/chip/$(CVIARCH_L) \
             -I$(src)/../rtos_cmdqu/

KBUILD_EXTRA_SYMBOLS = $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../vpss/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
