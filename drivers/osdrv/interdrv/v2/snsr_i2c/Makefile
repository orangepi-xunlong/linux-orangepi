include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
$(CVIARCH_L)_snsr_i2c-objs += common/snsr_i2c.o

obj-m += $(CVIARCH_L)_snsr_i2c.o

ccflags-y += -I$(src)/chip/$(CVIARCH_L)/ -I$(src)/common/ -I$(src)/../include/chip/$(CVIARCH_L)/uapi
ccflags-y += -I$(src)/../include/common/kapi/ -I$(src)/../include/chip/$(CVIARCH_L)/kapi
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)/ -I$(src)/../include/common/uapi

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(src)/../sys/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
