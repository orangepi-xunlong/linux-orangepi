#
# Makefile for sunxi cpus test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_SUNXI_SOC_INFO)+= sunxi_soc_info.o

install: build
	cp sunxi_soc_info.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

#clean:
#	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
