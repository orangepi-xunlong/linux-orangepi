#include $(PWD)/../Makefile.interdrv.param

RTC_PATH = $(CVIARCH_L)

$(CVIARCH_L)_rtc-objs = $(RTC_PATH)/cvi_rtc.o

obj-m += $(CVIARCH_L)_rtc.o

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
