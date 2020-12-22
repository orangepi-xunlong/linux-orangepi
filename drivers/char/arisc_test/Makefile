#
# Makefile for sunxi arisc test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_SUNXI_ARISC_TEST)+= arisc_test.o

install: build
	cp arisc_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

#clean:
#	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
