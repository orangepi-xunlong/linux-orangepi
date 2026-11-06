#include $(PWD)/../Makefile.interdrv.param

PWM_PATH = $(CVIARCH_L)

$(CVIARCH_L)_pwm-objs = $(PWM_PATH)/cvi_pwm.o

#ifeq ($(CONFIG_CVITEK_PWM),y)
#CONFIG_CVITEK_PWM = m
#endif
#obj-$(CONFIG_CVITEK_PWM) += $(CVIARCH_L)_pwm.o
obj-m += $(CVIARCH_L)_pwm.o

all:
	$(MAKE) -k ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
