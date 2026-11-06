include $(src)/../Makefile.interdrv.param

ccflags-y += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk
$(CVIARCH_L)_hdmi-y += common/hdmi_ioctl.o
$(CVIARCH_L)_hdmi-y += common/hdmi_proc.o

#CROSS_COMPILE=aarch64-linux-gnu
obj-m += $(CVIARCH_L)_hdmi.o
#ccflags-y += -DPORTING_TEST
ccflags-y += -I$(src)/chip/$(CVIARCH_L)/src/  -I$(src)/
ccflags-y += -I$(src)/common/
ccflags-y += -I$(src)/../vo/chip/soph/
ccflags-y += -I$(src)/../include/common/uapi/ -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/common/kapi/ -I$(src)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(src)/../include/
ccflags-y += -I$(src)/../base/
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)
ccflags-y += -I$(src)/../sys/common/ -I$(src)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android

# ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../vo/Module.symvers

else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
