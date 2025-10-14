# SPDX-License-Identifier: GPL-2.0-only
ifneq ($(KERNELRELEASE),)
	ccflags-y += -I$(srctree)/drivers/media/i2c/cix/lt7911uxc
	ccflags-y += -DLT7911UXC_DEBUG
	obj-m += lt7911uxc.o
else
	PWD := $(shell pwd)
ifeq ($(ARCH), )
	ARCH := arm64
endif

# if not defined, replace with your path here
ifeq ($(CROSS_COMPILE), )
	CROSS_COMPILE := aarch64-none-linux-gnu-
endif

# if not defined, replace with your path here
ifeq ($(KSRC), )
	KSRC := /$(PWD)/../../../../../
endif

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD) modules

clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	find . -name '*.mod.c' -type f -print -exec rm -rf {} \;
	find . -name '*.ko' -type f -print -exec rm -rf {} \;
	find . -name '*.symvers' -type f -print -exec rm -rf {} \;
	find . -name '*.order' -type f -print -exec rm -rf {} \;
	find . -name '*.a' -type f -print -exec rm -rf {} \;
	find . -name '*.mod' -type f -print -exec rm -rf {} \;
	find . -name '*.cmd' -type f -print -exec rm -rf {} \;
endif