#include $(PWD)/../Makefile.interdrv.param

PWD := $(shell pwd)

ccflags-y += -I$(src)/common -I$(src)/hal/$(CVIARCH_L)

obj-m += $(CVIARCH_L)_mon.o
$(CVIARCH_L)_mon-y += common/cvi_mon_interface.o
$(CVIARCH_L)_mon-y += hal/$(CVIARCH_L)/mon_platform.o

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
