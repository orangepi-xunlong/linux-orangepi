arch_cvitek_chip := $(patsubst "%",%,$(CHIP_ARCH_L))

SPACC_PATH = $(arch_cvitek_chip)

$(arch_cvitek_chip)_spacc-objs = $(SPACC_PATH)/$(arch_cvitek_chip)_spacc.o

obj-m += $(arch_cvitek_chip)_spacc.o

ccflags-y += -I$(src)/

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(src) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
