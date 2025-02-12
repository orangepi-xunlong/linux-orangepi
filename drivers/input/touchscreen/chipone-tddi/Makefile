obj-$(CONFIG_TOUCHSCREEN_CHIPONE) += icnl9951r.o
obj-y += icnl9951r.o

icnl9951r-objs := cts_driver.o
icnl9951r-objs += cts_core.o
icnl9951r-objs += cts_sfctrlv2.o
icnl9951r-objs += cts_spi_flash.o
icnl9951r-objs += cts_firmware.o
icnl9951r-objs += cts_test.o
icnl9951r-objs += cts_charger_detect.o
icnl9951r-objs += cts_earjack_detect.o
icnl9951r-objs += cts_tcs.o
icnl9951r-objs += cts_platform.o
icnl9951r-objs += cts_tool.o
icnl9951r-objs += cts_sysfs.o
icnl9951r-objs += cts_strerror.o

#ccflags-y := -mno-android -I$(KDIR)/drivers/devkit/tpkit/hostprocessing
#ccflags-y += -mno-android -Werror

#KDIR := $(HOME)/andev/db810/aosp/kernel
#ODIR := $(HOME)/andev/db810/aosp/out/target/product/msm8994/obj/KERNEL_OBJ
#TOOL := /opt/android-ndk-r15c/toolchains/aarch64-linux-android-4.9/prebuilt/linux-x86_64/bin/aarch64-linux-android-
#PWD ?= $(shell pwd)
#ccflags-y := -mno-android
#

all:
	make -C $(KDIR) O=$(ODIR) ARCH=arm64 CROSS_COMPILE=$(TOOL) KCFLAGS="$(ccflags-y)" M=$(PWD) modules

clean:
	$(RM) -r *.o .*.o .*.cmd *.ko *.mod.c Module.symvers modules.order .tmp_versions
