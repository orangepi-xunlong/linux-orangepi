include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk
$(CVIARCH_L)_vpss-objs += common/vpss_proc.o
$(CVIARCH_L)_vpss-objs += common/vpss.o
$(CVIARCH_L)_vpss-objs += common/vpss_rgn_ctrl.o
$(CVIARCH_L)_vpss-objs += common/vpss_ioctl.o

obj-m += $(CVIARCH_L)_vpss.o

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

ifeq ($(SUBTYPE), fpga)
ccflags-y += -DFPGA_PORTING
endif

KBUILD_EXTRA_SYMBOLS += $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
