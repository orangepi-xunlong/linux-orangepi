ifneq ($(KERNELRELEASE),)
PWD = $(srctree)/drivers/media/platform/soph/cif_v4l2

export CVIARCH_L := cv186x

include $(PWD)/chip/$(CVIARCH_L)/chip.mk

obj-y += cif_v4l2.o

ccflags-y += -I$(PWD)/chip/$(CVIARCH_L)/ -I$(PWD)/common/
ccflags-y += -I$(PWD)/../osdrv_include/common/uapi/ -I$(PWD)/../osdrv_include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../osdrv_include/common/kapi/ -I$(PWD)/../osdrv_include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/

ifeq ($(SUBTYPE), fpga)
export FPGA_PORTING := true
ccflags-y += -DFPGA_PORTING
endif

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
