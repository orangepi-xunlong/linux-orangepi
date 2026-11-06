include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk

$(CVIARCH_L)_vo-objs += common/vo_rgn_ctrl.o

obj-m += $(CVIARCH_L)_vo.o

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ccflags-y += -I$(src)/chip/$(CVIARCH_L)/ -I$(src)/common/
ccflags-y += -I$(src)/../include/common/uapi/ -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/common/kapi/ -I$(src)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(src)/../sys/common/ -I$(src)/../sys/common/uapi
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)/
ccflags-y += -I$(src)/../base/
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/

KBUILD_EXTRA_SYMBOLS = $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -f mipi_tx/Module.symvers mipi_tx/*.ko mipi_tx/*.mod.c mipi_tx/*.mod mipi_tx/.*.cmd
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
