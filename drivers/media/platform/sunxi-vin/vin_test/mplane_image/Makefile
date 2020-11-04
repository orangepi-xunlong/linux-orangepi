#CC:=/home/zequnzheng/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
CC:=/home/zequnzheng/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
ARCH_DIR=/home/zequnzheng/longan/kernel/linux-4.9/arch/arm64/include/
CFLAGS := -I $(ARCH_DIR)
TARGET := csi_test_mplane

.PHONY: all clean

all: $(TARGET)

csi_test_mplane: csi_test_mplane.c
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)
