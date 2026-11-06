#include $(PWD)/../Makefile.interdrv.param

CONFIG_LOG = 1

$(CVIARCH_L)_base-objs = base.o \
				base_common.o \
				bind.o \
				callback.o \
				ion.o \
				vb.o \
				vbq.o \
				proc/log_proc.o \
				proc/sys_proc.o \
				proc/vb_proc.o



obj-m += $(CVIARCH_L)_base.o

ccflags-y += -I$(srctree)/drivers/tee \
             -I$(srctree)/drivers/osdrv/interdrv/v2/include/common/uapi/ \
             -I$(srctree)/drivers/osdrv/interdrv/v2/include/common/kapi/ \
             -I$(srctree)/drivers/osdrv/interdrv/v2/base/ \
             -I$(srctree)/drivers/osdrv/interdrv/v2/include/chip/$(CVIARCH_L)/uapi/ \
             -I$(srctree)/drivers/osdrv/interdrv/v2/sys/common/ \
             -I$(srctree)/drivers/osdrv/interdrv/v2/sys/common/uapi \
             -I$(srctree)/drivers/staging/android \
             -I$(srctree)/drivers/osdrv/interdrv/v2/base/proc \

ifeq ($(CONFIG_LOG), 1)
ccflags-y += -DCONFIG_LOG
endif

ifeq ($(SUBTYPE), fpga)
ccflags-y += -DDRV_FPGA_PORTING
endif

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare


all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
