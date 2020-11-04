CC:= /home/zequnzheng/A63/lichee/out/gcc-linaro-5.3.1-2016.05/gcc-aarch64/bin/aarch64-linux-gnu-gcc
#CC:=/home/zequnzheng/A63/lichee/out/gcc-linaro-5.3.1-2016.05/gcc-arm/bin/arm-linux-gnueabi-gcc
LINUX_DIR = $(shell pwd)/../../../../../include/:$(shell pwd)/../../../../../arch/arm64/include
ARCH_DIR=../../../../../arch/arm64/include/
CFLAGS := -I $(LINUX_DIR)
TARGET := csi_test

.PHONY: all clean

all: $(TARGET)

csi_test: csi_test.c mem_alloc_ion/ion_alloc.c
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)
