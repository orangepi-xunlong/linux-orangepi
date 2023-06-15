#longan system
#CC:=/home/username/workspace/lichee/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
#CC:=/home/username/workspace/lichee/longan/out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
#tina system
#CC:=/home/username/workspace/tina/prebuilt/gcc/linux-x86/arm/toolchain-sunxi-musl/toolchain/bin/arm-openwrt-linux-muslgnueabi-gcc-6.4.1

#longan system
CC := ../../../../../../../../out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc
ARCH_DIR := ../../../../../linux-5.4/arch/arm/include/
CFLAGS := -I $(ARCH_DIR)
TARGET := csi_test_mplane

.PHONY: all clean

all: $(TARGET)

csi_test_mplane: csi_test_mplane.c
	$(CC) $(CFLAGS) -static  $^  -o  $@ -lc -lgcc_eh

	#mv csi_test_mplane ~/to_server/camera_test/

clean:
	rm -rf $(TARGET)
