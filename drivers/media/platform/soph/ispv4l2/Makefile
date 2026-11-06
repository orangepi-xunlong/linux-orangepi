ifneq ($(KERNELRELEASE),)
PWD = $(srctree)/drivers/media/platform/soph/ispv4l2
KO_PREFEX = soph

export CVIARCH_L := cv186x

include $(PWD)/chip/$(CVIARCH_L)/chip.mk

$(KO_PREFEX)_ispv4l2-objs += isp_dev.o
$(KO_PREFEX)_ispv4l2-objs += isp_subdev.o

obj-y += $(KO_PREFEX)_ispv4l2.o

ccflags-y += -I$(PWD)/
ccflags-y += -I$(PWD)/chip/$(CVIARCH_L)/
ccflags-y += -I$(PWD)/../osdrv_include/common/uapi/ -I$(PWD)/../osdrv_include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../osdrv_include/common/kapi/ -I$(PWD)/../osdrv_include/chip/$(CVIARCH_L)/kapi/
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

#ccflags-y += -DFPGA_PORTING

else

all:
	$(MAKE) ARCH=$(ARCH) $(CFLAGS) $(EXTRA_CFLAGS) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete

endif
