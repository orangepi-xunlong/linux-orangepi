include $(src)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(src)/chip/$(CVIARCH_L)/chip.mk

obj-m += $(CVIARCH_L)_mipi_rx.o

ccflags-y += -I$(src)/chip/$(CVIARCH_L)/ -I$(src)/common/
ccflags-y += -I$(src)/../include/common/uapi/ -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/
ccflags-y += -I$(src)/../include/common/kapi/
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)/

ifeq ($(SUBTYPE), fpga)
export FPGA_PORTING := true
ccflags-y += -DFPGA_PORTING
endif

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../sys/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
