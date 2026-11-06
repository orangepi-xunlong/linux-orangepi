CHIP_ID := $(patsubst "%",%,$(CHIP_ARCH_L))

ifneq ($(KERNELRELEASE),)

obj-m += $(CHIP_ID)_gyro.o

$(CHIP_ID)_gyro-y += hal/cv181x/mpu9250_reg.o
$(CHIP_ID)_gyro-y += hal/cv181x/mpu9250.o
$(CHIP_ID)_gyro-y += common/gyro_i2c.o

MW = $(PWD)/../../interdrv/v2
ccflags-y += -I$(PWD)/chip/$(CHIP_ID)/ -I$(PWD)/common/ -I$(PWD)/hal/cv181x
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/
ccflags-y += -I$(MW)/include/chip/$(arch_cvitek_chip)/uapi
ccflags-y += -I$(MW)/include/common/uapi/
ccflags-y += -I$(MW)/base/chip/$(arch_cvitek_chip)/


KBUILD_EXTRA_SYMBOLS = $(MW)/base/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
