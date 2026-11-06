#include $(PWD)/../Makefile.interdrv.param

CONFIG_LOG = 1

ccflags-y += -I$(srctree)/drivers/osdrv/interdrv/v2/include/common/uapi/ \
		-I$(srctree)/drivers/osdrv/interdrv/v2/include/common/kapi/
ccflags-y += -I$(srctree)/drivers/osdrv/interdrv/v2/sys/chip/$(CVIARCH_L)/ \
             -I$(srctree)/drivers/osdrv/interdrv/v2/include/chip/$(CVIARCH_L)/uapi \
             -I$(srctree)/drivers/osdrv/interdrv/v2/include/chip/$(CVIARCH_L)/kapi \
			 -I$(srctree)/drivers/osdrv/interdrv/v2/sys/common

ifeq ($(CONFIG_LOG), 1)
ccflags-y += -DCONFIG_LOG
endif

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

obj-m += $(CVIARCH_L)_sys.o
$(CVIARCH_L)_sys-y += common/sys.o \
                      common/reg.o \
                      chip/$(CVIARCH_L)/sys_common.o \
                      chip/$(CVIARCH_L)/vi_sys.o \
                      chip/$(CVIARCH_L)/vo_sys.o \
                      chip/$(CVIARCH_L)/cdma.o

KBUILD_EXTRA_SYMBOLS += $(srctree)/drivers/osdrv/interdrv/v2/base/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o* *.ko
	@rm -f Module.symvers modules.order
	@rm -f *.mod *.mod.c
	@rm -rf .tmp_versions
	@rm -f .*cmd
	@rm -f $(CHIP)/*.o*
	@rm -f $(CHIP)/.*cmd
	@find . -name \*.o* -type f -delete
