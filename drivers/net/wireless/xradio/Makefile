NOSTDINC_FLAGS := -I$(srctree)/drivers/net/wireless/xradio/include/


######################### XRMAC Features###########################
CONFIG_XRMAC_RC_PID = n
CONFIG_XRMAC_RC_MINSTREL = y
CONFIG_XRMAC_RC_MINSTREL_HT = y
########################## XRWLAN Features#########################
CONFIG_XRADIO_SDIO = y
CONFIG_XRADIO_USE_EXTENSIONS = y
########################## XRADIO_DEBUG Features##################
CONFIG_XRADIO_ETF = y
ifeq ($(CONFIG_DEBUG_FS), y)
CONFIG_XRMAC_DEBUGFS = y
CONFIG_XRADIO_DEBUG =y
ifeq ($(CONFIG_XRADIO_DEBUG), y)
CONFIG_XRADIO_DUMP_ON_ERROR = y
CONFIG_XRADIO_DEBUGFS =y
endif
endif

##########################  Define  ##################
ccflags-y += -DCONFIG_XRMAC_RC_DEFAULT=\"minstrel_ht\"

ifeq ($(CONFIG_XRMAC_RC_PID), y)
EXTRA_CFLAGS += -DCONFIG_XRMAC_RC_PID
endif

ifeq ($(CONFIG_XRMAC_RC_MINSTREL), y)
EXTRA_CFLAGS += -DCONFIG_XRMAC_RC_MINSTREL
endif

ifeq ($(CONFIG_XRMAC_RC_MINSTREL_HT), y)
EXTRA_CFLAGS += -DCONFIG_XRMAC_RC_MINSTREL_HT
endif

ifeq ($(CONFIG_XRADIO_SDIO), y)
EXTRA_CFLAGS += -DCONFIG_XRADIO_SDIO
endif

ifeq ($(CONFIG_XRADIO_USE_EXTENSIONS), y)
EXTRA_CFLAGS += -DCONFIG_XRADIO_USE_EXTENSIONS
endif

ifeq ($(CONFIG_XRMAC_DEBUGFS), y)
EXTRA_CFLAGS += -DCONFIG_XRMAC_DEBUGFS
endif

ifeq ($(CONFIG_XRADIO_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_XRADIO_DEBUG
endif

ifeq ($(CONFIG_XRADIO_ETF), y)
EXTRA_CFLAGS += -DCONFIG_XRADIO_ETF
endif

ifeq ($(CONFIG_XRADIO_DUMP_ON_ERROR), y)
EXTRA_CFLAGS += -DCONFIG_XRADIO_DUMP_ON_ERROR
endif

ifeq ($(CONFIG_XRADIO_DEBUGFS), y)
EXTRA_CFLAGS += -DCONFIG_XRADIO_DEBUGFS
endif
########### XRMAC Objects And Features###############################################
# xradio_mac objects
xradio_mac-y := \
	umac/main.o \
	umac/status.o \
	umac/sta_info.o \
	umac/wep.o \
	umac/wpa.o \
	umac/wapi.o \
	umac/scan.o \
	umac/offchannel.o \
	umac/ht.o \
	umac/agg-tx.o \
	umac/agg-rx.o \
	umac/ibss.o \
	umac/mlme.o \
	umac/work.o \
	umac/iface.o \
	umac/rate.o \
	umac/michael.o \
	umac/tkip.o \
	umac/aes_ccm.o \
	umac/aes_cmac.o \
	umac/cfg.o \
	umac/rx.o \
	umac/spectmgmt.o \
	umac/tx.o \
	umac/key.o \
	umac/util.o \
	umac/wme.o \
	umac/event.o \
	umac/chan.o

xradio_mac-$(CONFIG_XRMAC_LEDS) += umac/led.o
xradio_mac-$(CONFIG_XRMAC_DEBUGFS) += \
	umac/debugfs.o \
	umac/debugfs_sta.o \
	umac/debugfs_netdev.o \
	umac/debugfs_key.o

xradio_mac-$(CONFIG_XRMAC_MESH) += \
	umac/mesh.o \
	umac/mesh_pathtbl.o \
	umac/mesh_plink.o \
	umac/mesh_hwmp.o

xradio_mac-$(CONFIG_PM) += umac/pm.o

xradio_mac-$(CONFIG_XRMAC_DRIVER_API_TRACER) += umac/driver-trace.o
CFLAGS_driver-trace.o := -I$(src)

# objects for PID algorithm
rc80211_pid-y := umac/rc80211_pid_algo.o
rc80211_pid-$(CONFIG_XRMAC_DEBUGFS) += umac/rc80211_pid_debugfs.o

rc80211_minstrel-y := umac/rc80211_minstrel.o
rc80211_minstrel-$(CONFIG_XRMAC_DEBUGFS) += umac/rc80211_minstrel_debugfs.o

rc80211_minstrel_ht-y := umac/rc80211_minstrel_ht.o
rc80211_minstrel_ht-$(CONFIG_XRMAC_DEBUGFS) += umac/rc80211_minstrel_ht_debugfs.o

xradio_mac-$(CONFIG_XRMAC_RC_PID) += $(rc80211_pid-y)
xradio_mac-$(CONFIG_XRMAC_RC_MINSTREL) += $(rc80211_minstrel-y)
xradio_mac-$(CONFIG_XRMAC_RC_MINSTREL_HT) += $(rc80211_minstrel_ht-y)

ccflags-y += -D__CHECK_ENDIAN__
# Extra IE for probe response from upper layer is needed in P2P GO
# For offloading probe response to FW, the extra IE must be included
# in the probe response template
ccflags-y += -DPROBE_RESP_EXTRA_IE
ccflags-y += -DUSE_RSSI_SMOOTH
ccflags-y += -DIPV6_FILTERING
#ccflags-y += -DCONFIG_XRMAC_XR_ROAMING_CHANGES
########### XRWLAN Objects And Features###############################################
xradio_core-y := \
	wlan/common.o \
	wlan/fwio.o \
	wlan/txrx.o \
	wlan/main.o \
	wlan/queue.o \
	wlan/hwio.o \
	wlan/bh.o \
	wlan/wsm.o \
	wlan/sta.o \
	wlan/ap.o \
	wlan/scan.o \
	wlan/platform.o

xradio_core-$(CONFIG_PM)            += wlan/pm.o
xradio_core-$(CONFIG_XRADIO_SDIO)   += wlan/sdio.o
xradio_core-$(CONFIG_XRADIO_DEBUG)  += wlan/debug.o
xradio_core-$(CONFIG_XRADIO_ITP)    += wlan/itp.o
xradio_core-$(CONFIG_XRADIO_ETF)    += wlan/etf.o

ccflags-y += -DP2P_MULTIVIF
ccflags-y += -DMCAST_FWDING
ccflags-y += -DXRADIO_SUSPEND_RESUME_FILTER_ENABLE
ccflags-y += -DAP_AGGREGATE_FW_FIX
ccflags-y += -DAP_HT_CAP_UPDATE
ccflags-y += -DHW_RESTART
ccflags-y += -DHW_ERROR_WIFI_RESET
ccflags-y += -DAP_HT_COMPAT_FIX
ccflags-y += -DENHANCE_ANTI_INTERFERE
ccflags-y += -DMONITOR_MODE
#ccflags-y += -DUSE_RSSI_OFFSET

# Use vfs for firmware load when request_firmware
# can't work on other platform.
ccflags-y += -DUSE_VFS_FIRMWARE

# Extra IE for probe response from upper layer is needed in P2P GO
# For offloading probe response to FW, the extra IE must be included
# in the probe response template
ccflags-y += -DPROBE_RESP_EXTRA_IE
ccflags-y += -DIPV6_FILTERING

# Modified for P2P stability.
ccflags-y += -DTES_P2P_0002_ROC_RESTART
ccflags-y += -DTES_P2P_000B_EXTEND_INACTIVITY_CNT
ccflags-y += -DTES_P2P_000B_DISABLE_EAPOL_FILTER

# Modified for power save.
#ccflags-y += -DXRADIO_USE_LONG_DTIM_PERIOD
ccflags-y += -DXRADIO_USE_LONG_KEEP_ALIVE_PERIOD

# Mac addr config, disable hex for default.
#ccflags-y += -DXRADIO_MACPARAM_HEX

# Mac addr generate from chipid.
ccflags-y += -DXRADIO_MACADDR_FROM_CHIPID

# Use semaphore to sync bh txrx.
#ccflags-y += -DBH_USE_SEMAPHORE

ldflags-y += --strip-debug

########### XRADIO WLAN MODULE COMPILE###############################################
xradio_wlan-y := $(xradio_mac-y)
xradio_wlan-y += $(xradio_core-y)
xradio_wlan-y += wlan/wlan_init.o
obj-$(CONFIG_XR_WLAN) += xradio_wlan.o

clean-files += Module.symvers Module.markers modules modules.order
clean:
	rm -fr umac/*.o umac/.*.o.cmd
	rm -fr wlan/*.o wlan/.*.o.cmd
	rm -fr $(clean-files)
	rm -fr *.mod.c *.ko *.o *.mod.o .*.o.cmd .*.ko.cmd
