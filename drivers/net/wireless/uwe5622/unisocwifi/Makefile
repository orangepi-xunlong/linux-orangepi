PWD := $(shell pwd)
all_dependencies := driver

ccflags-y += -I$(src)/../unisocwcn/include
ccflags-$(ORIGIN_BSP) += -I$(srctree)/../hardware/wifi/unisoc/drivers/unisocwcn/include

ifneq ($(UNISOC_BSP_INCLUDE),)
ccflags-y += -I$(UNISOC_BSP_INCLUDE)
endif

ifneq ($(UNISOC_WIFI_CUS_CONFIG),)
ccflags-y += -DCUSTOMIZE_WIFI_CFG_PATH=\"$(UNISOC_WIFI_CUS_CONFIG)\"
endif

ifneq ($(UNISOC_WIFI_MAC_FILE),)
ccflags-y += -DCUSTOMIZE_WIFI_MAC_FILE=\"$(UNISOC_WIFI_MAC_FILE)\"
endif

####add cflag######
ccflags-y += -DIBSS_SUPPORT -DIBSS_RSN_SUPPORT
ccflags-y += -DNAN_SUPPORT
ccflags-y += -DRTT_SUPPORT
ccflags-y += -DACS_SUPPORT -DRX_HW_CSUM
ccflags-y += -DWMMAC_WFA_CERTIFICATION
ccflags-y += -DCOMPAT_SAMPILE_CODE
ccflags-y += -DRND_MAC_SUPPORT
ccflags-y += -DATCMD_ASSERT
ccflags-y += -DTCPACK_DELAY_SUPPORT
ccflags-$(DEBUGMODE) += -DWL_CONFIG_DEBUG
ccflags-y += -DSPLIT_STACK
ccflags-y += -DOTT_UWE
ccflags-y += -DCP2_RESET_SUPPORT
ifeq ($(UNISOC_STA_SOFTAP_SCC_MODE),y)
ccflags-y += -DSTA_SOFTAP_SCC_MODE
endif

ccflags-$(CONFIG_UNISOC_WIFI_PS) += -DUNISOC_WIFI_PS

#######add .o file#####
sprdwl_ng-objs += main.o cfg80211.o txrx.o cmdevt.o npi.o msg.o work.o vendor.o \
				  tcp_ack.o mm.o reorder.o wl_core.o tx_msg.o rx_msg.o \
				  wl_intf.o qos.o dbg_ini_util.o reg_domain.o
sprdwl_ng-objs += defrag.o
sprdwl_ng-objs += ibss.o
sprdwl_ng-objs += nan.o
sprdwl_ng-objs += tracer.o
sprdwl_ng-objs += rf_marlin3.o
sprdwl_ng-objs += rtt.o
sprdwl_ng-objs += api_version.o
sprdwl_ng-objs += rnd_mac_addr.o
sprdwl_ng-objs += debug.o
#sprdwl_ng-objs += 11h.o

obj-$(CONFIG_WLAN_UWE5622) += sprdwl_ng.o

all: $(all_dependencies)
driver: $(driver_dependencies)
	@echo build driver
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) Module.markers
	$(RM) modules.order
