include $(src)/../Makefile.interdrv.param

obj-m += soph_fb.o
soph_fb-y += cvifb.o

#cvi_fb-objs := cvifb.o
#obj-$(CONFIG_FB_CVITEK) += cvi_fb.o
ccflags-y += -I$(srctree)/drivers/staging/android/
ccflags-y += -I$(src)/../base/
ccflags-y += -I$(src)/../base/chip/$(CVIARCH_L)/
ccflags-y += -I$(src)/../vo/chip/$(CVIARCH_L)/
ccflags-y += -I$(src)/../include/common/uapi/
ccflags-y += -I$(src)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(src)/../include/common/kapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(src)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(src)/../vo/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(src) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	rm -rf .tmp_versions
