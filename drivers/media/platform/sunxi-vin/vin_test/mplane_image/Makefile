#CC:=/home/zequnzheng/A63/lichee/out/gcc-linaro-5.3.1-2016.05/gcc-aarch64/bin/aarch64-linux-gnu-gcc
CC:=/home/zequnzheng/A63/lichee/out/gcc-linaro-5.3.1-2016.05/gcc-arm/bin/arm-linux-gnueabi-gcc
ARCH_DIR=/home/zequnzheng/A63/lichee/linux-4.9/arch/arm64/include/
CFLAGS := -I $(ARCH_DIR)
TARGET := csi_test_mplane

.PHONY: all clean

all: $(TARGET)

csi_test_mplane: csi_test_mplane.c
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)
