CC:=../../../../out/toolchain/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
ARCH_DIR=../../../../kernel/linux-5.10/arch/arm/include/
CFLAGS := -I $(ARCH_DIR)
TARGET := csi_test

.PHONY: all clean

all: $(TARGET)

csi_test: csi_test.c
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)
