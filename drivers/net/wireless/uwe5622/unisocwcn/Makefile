PWD := $(shell pwd)
CURFOLDER ?= $(pwd)

LD_CFLAGS += -D__linux__
EXTRA_CFLAGS=-fno-pic

all_dependencies := driver
driver_dependencies :=

ifeq ($(CONFIG_AW_WIFI_DEVICE_UWE5622),y)
#ccflags-y += -DCONFIG_CHECK_DRIVER_BY_CHIPID
ccflags-y += -DCONFIG_UWE5622
BSP_CHIP_ID := uwe5622
WCN_HW_TYPE := sdio
endif

MODULE_NAME := $(BSP_CHIP_ID)_bsp_$(WCN_HW_TYPE)

ifneq ($(UNISOC_FW_PATH_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_FW_PATH=\"$(UNISOC_FW_PATH_CONFIG)\"
endif
ifneq ($(UNISOC_DBG_FILESIZE_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_DBG_FILESIZE=$(UNISOC_DBG_FILESIZE_CONFIG)
endif
ifneq ($(UNISOC_DBG_FILENUM_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_DBG_FILENUM=$(UNISOC_DBG_FILENUM_CONFIG)
endif
ifneq ($(UNISOC_DBG_PATH_CONFIG),)
ccflags-y += -DCONFIG_CUSTOMIZE_UNISOC_DBG_PATH=\"$(UNISOC_DBG_PATH_CONFIG)\"
endif

#### add cflag ######
ccflags-y += -DCONFIG_SDIOHAL
ccflags-y += -DCONFIG_WCN_SLP
ccflags-y += -DCONFIG_WCN_BOOT
ccflags-y += -DCONFIG_WCN_UTILS

#### include path ######
ccflags-y += -I$(src)/include/
ccflags-y += -I$(src)/platform/

#### add cflag ######
#ccflags-y += -DCONFIG_AW_BOARD
ccflags-y += -DCONFIG_WCN_PARSE_DTS
#ccflags-y += -DCONFIG_SDIO_TX_ADMA_MODE
#ccflags-y += -DCONFIG_SDIO_RX_ADMA_MODE
ccflags-y += -DCONFIG_SDIO_INBAND_INT
#ccflags-y += -DCONFIG_SDIO_INBAND_POLLING
#ccflags-y += -DCONFIG_SDIO_BLKSIZE_512
#ccflags-y += -DCONFIG_SDIO_PWRSEQ

all_dependencies += install

obj-$(CONFIG_AW_WIFI_DEVICE_UWE5622) := $(MODULE_NAME).o

$(MODULE_NAME)-y += wcn_bus.o \
		    platform/wcn_boot.o \
		    platform/wcn_txrx.o \
		    platform/wcn_misc.o \
		    platform/wcn_procfs.o \
		    platform/loopcheck.o

$(MODULE_NAME)-y += \
		    sdio/sdiohal_main.o \
		    sdio/sdiohal_common.o \
		    sdio/sdiohal_tx.o \
		    sdio/sdiohal_rx.o \
		    sdio/sdio_v3.o \
		    sdio/sdiohal_ctl.o \
		    sleep/sdio_int.o \
		    sleep/slp_mgr.o \
		    sleep/slp_sdio.o

KDIR ?= $(ANDROID_PRODUCT_OUT)/obj/KERNEL_OBJ
ARCH ?= arm
CROSS_COMPILE ?= arm-histbv310-linux-

EXTRA_CFLAGS += -I$(src)/include -D__linux__

all: $(all_dependencies)

driver: $(driver_dependencies)
	@echo build driver
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

install:
	@echo install targets
	mkdir -p $(ANDROID_PRODUCT_OUT)/system/lib/modules
	cp -fv $(PWD)/$(MODULE_NAME).ko $(ANDROID_PRODUCT_OUT)/system/lib/modules/$(MODULE_NAME).ko
	make clean

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) Module.markers
	$(RM) modules.order
