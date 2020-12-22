#
# Makefile for sunxi cpus test driver
#

PWD=$(shell pwd)

obj-$(CONFIG_SUNXI_CPUS_TEST)+= cpus_test.o

install: build
	cp cpus_test.ko $(LICHEE_MOD_DIR)/

build:
	@echo $(LICHEE_KDIR)
	$(MAKE) -C $(LICHEE_KDIR) M=$(PWD)

#clean:
#	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~
