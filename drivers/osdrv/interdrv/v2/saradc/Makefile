#include $(PWD)/../Makefile.interdrv.param

SARADC_PATH = $(CVIARCH_L)

$(CVIARCH_L)_saradc-objs = $(SARADC_PATH)/cvi_saradc.o

ifeq ($(CONFIG_CVITEK_ADC),y)
CONFIG_CVITEK_ADC = m
endif

ifeq ($(CVIARCH_L),cv186x)
obj-$(CONFIG_CVITEK_ADC) += $(CVIARCH_L)_saradc.o
else
obj-m += $(CVIARCH_L)_saradc.o
endif

subdir-ccflags-y += -I$(src)/common/
subdir-ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek

subdir-ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) -k ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
