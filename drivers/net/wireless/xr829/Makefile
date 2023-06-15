##################### XRADIO Wi-Fi Config #######################
CONFIG_XRMAC_RC_DEFAULT = "minstrel_ht"
CONFIG_XRMAC_RC_PID = n
CONFIG_XRMAC_RC_MINSTREL = y
CONFIG_XRMAC_RC_MINSTREL_HT = y
CONFIG_XRMAC_DEBUGFS = n
CONFIG_XRADIO_SDIO = y
CONFIG_XRADIO_NON_POWER_OF_TWO_BLOCKSIZES = y
CONFIG_XRADIO_USE_GPIO_IRQ = y
CONFIG_XRADIO_SUSPEND_POWER_OFF = n
CONFIG_XRADIO_EXTEND_SUSPEND = n
CONFIG_XRADIO_NOMAL_SUSPEND_FORCE = n
CONFIG_XRADIO_DEBUG = y
CONFIG_XRADIO_ETF = y
CONFIG_XRADIO_DUMP_ON_ERROR = n
CONFIG_XRADIO_DEBUGFS = y
CONFIG_XRMAC_VERBOSE_DEBUG = y
CONFIG_XRMAC_DRIVER_API_TRACER = y
CONFIG_MODULE_NAME = xr829

define boolen_flag
$(strip $(if $(findstring $($(1)),$(2)),-D$(1)))
endef

define string_flag
$(strip $(if $($(1)),-D$(1)=\"$($(1))\"))
endef

DRV_FLAGS += $(call string_flag,CONFIG_XRMAC_RC_DEFAULT)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRMAC_RC_PID,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRMAC_RC_MINSTREL,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRMAC_RC_MINSTREL_HT,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRMAC_DEBUGFS,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRMAC_VERBOSE_DEBUG,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_SDIO,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_NON_POWER_OF_TWO_BLOCKSIZES,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_USE_GPIO_IRQ,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_SUSPEND_POWER_OFF,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_EXTEND_SUSPEND,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_NOMAL_SUSPEND_FORCE,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_DEBUG,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_ETF,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_DUMP_ON_ERROR,y)
DRV_FLAGS += $(call boolen_flag,CONFIG_XRADIO_DEBUGFS,y)

ccflags-y += $(DRV_FLAGS)

NOSTDINC_FLAGS := -I$(srctree)/$(src)/include/
NOSTDINC_FLAGS += -I$(srctree)/$(src)/include/uapi/

ldflags-y += --strip-debug

# xradio_mac objects
xradio_mac-y := \
	umac/main.o \
	umac/status.o \
	umac/sta_info.o \
	umac/wep.o \
	umac/wpa.o \
	umac/scan.o \
	umac/offchannel.o \
	umac/ht.o \
	umac/vht.o \
	umac/he.o \
	umac/agg-tx.o \
	umac/agg-rx.o \
	umac/ibss.o \
	umac/mlme.o \
	umac/iface.o \
	umac/rate.o \
	umac/michael.o \
	umac/tkip.o \
	umac/aead_api.o \
	umac/aes_gmac.o \
	umac/aes_cmac.o \
	umac/fils_aead.o \
	umac/cfg.o \
	umac/rx.o \
	umac/spectmgmt.o \
	umac/tx.o \
	umac/key.o \
	umac/util.o \
	umac/wme.o \
	umac/chan.o \
	umac/ethtool.o \
	umac/ocb.o \
	umac/tdls.o \
	umac/trace.o \

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
	umac/mesh_hwmp.o \
	umac/mesh_ps.o \
	umac/mesh_sync.o

xradio_mac-$(CONFIG_PM) += umac/pm.o

xradio_mac-$(CONFIG_XRMAC_DRIVER_API_TRACER) += umac/driver-ops.o
CFLAGS_driver-trace.o := -I$(src)

# objects for PID algorithm
#rc80211_pid-y := umac/rc80211_pid_algo.o
#rc80211_pid-$(CONFIG_XRMAC_DEBUGFS) += umac/rc80211_pid_debugfs.o

rc80211_minstrel-y := umac/rc80211_minstrel.o
rc80211_minstrel-$(CONFIG_XRMAC_DEBUGFS) += umac/rc80211_minstrel_debugfs.o

rc80211_minstrel_ht-y := umac/rc80211_minstrel_ht.o
rc80211_minstrel_ht-$(CONFIG_XRMAC_DEBUGFS) += umac/rc80211_minstrel_ht_debugfs.o

#xradio_mac-$(CONFIG_XRMAC_RC_PID) += $(rc80211_pid-y)
xradio_mac-$(CONFIG_XRMAC_RC_MINSTREL) += $(rc80211_minstrel-y)
xradio_mac-$(CONFIG_XRMAC_RC_MINSTREL_HT) += $(rc80211_minstrel_ht-y)

ccflags-y += -D__CHECK_ENDIAN__
# Extra IE for probe response from upper layer is needed in P2P GO
# For offloading probe response to FW, the extra IE must be included
# in the probe response template
ccflags-y += -DPROBE_RESP_EXTRA_IE
ccflags-y += -DIPV6_FILTERING
#ccflags-y += -DCONFIG_XRMAC_XR_ROAMING_CHANGES
ccflags-y += -DUSE_RSSI_SMOOTH

#This is use for old p2p mode, say yes to add P2P0 interface
#ccflags-y += -DOLD_P2P_MODE

######## xradio_core objects ########
xradio_core-y := \
	wlan/fwio.o \
	wlan/txrx.o \
	wlan/vendor.o \
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

$(CONFIG_MODULE_NAME)-y := $(xradio_mac-y)
$(CONFIG_MODULE_NAME)-y += $(xradio_core-y)
$(CONFIG_MODULE_NAME)-y += wlan/wlan_init.o

obj-$(CONFIG_XR829_WLAN) += $(CONFIG_MODULE_NAME).o

##=======  User Options =======
## Mac addr config, disable hex for default.
#ccflags-y += -DXRADIO_MACPARAM_HEX
ccflags-y += -DMONITOR_MODE

##======= Follows just for xradio internal, Don't change these macro if not ensure. =======
ccflags-y += -DCONFIG_XRADIO_USE_EXTENSIONS
ccflags-y += -DP2P_MULTIVIF
ccflags-y += -DMCAST_FWDING
ccflags-y += -DXRADIO_SUSPEND_RESUME_FILTER_ENABLE
ccflags-y += -DAP_AGGREGATE_FW_FIX
ccflags-y += -DAP_HT_CAP_UPDATE
ccflags-y += -DAP_HT_COMPAT_FIX
#ccflags-y += -DAP_ARP_COMPAT_FIX
ccflags-y += -DENHANCE_ANTI_INTERFERE
#ccflags-y += -DUSE_RSSI_OFFSET
ccflags-y += -DSCAN_FAILED_WORKAROUND_OF_FW_EXCEPTION
ccflags-y += -DHW_RESTART
ccflags-y += -DHW_ERROR_WIFI_RESET

## Use semaphore to sync bh txrx.
#ccflags-y += -DBH_USE_SEMAPHORE
ccflags-y += -DBH_PROC_THREAD
ccflags-y += -DBH_COMINGRX_FORECAST
#ccflags-y += -H

# Modified for power save.
#ccflags-y += -DXRADIO_USE_LONG_DTIM_PERIOD
ccflags-y += -DXRADIO_USE_LONG_KEEP_ALIVE_PERIOD

## Extra IE for probe response from upper layer is needed in P2P GO
## For offloading probe response to FW, the extra IE must be included
## in the probe response template
ccflags-y += -DPROBE_RESP_EXTRA_IE
ccflags-y += -DIPV6_FILTERING

## Modified for P2P stability.
ccflags-y += -DTES_P2P_0002_ROC_RESTART
ccflags-y += -DTES_P2P_000B_EXTEND_INACTIVITY_CNT
ccflags-y += -DTES_P2P_000B_DISABLE_EAPOL_FILTER

## for chips.
ccflags-y += -DSUPPORT_HT40
ccflags-y += -DSUPPORT_EPTA
ccflags-y += -DSUPPORT_DPLL_CHECK
#ccflags-y += -DSUPPORT_NON_HT40_CHIP
ccflags-y += -DBOOT_NOT_READY_FIX

## for interal debug.
#ccflags-y += -DSUPPORT_FW_DBG_INF
#ccflags-y += -DERROR_HANG_DRIVER
ccflags-y += -DQUEUE_GEN_IF_TABLE
# Debug for hwx_BUG12:P2P setting crash
ccflags-y += -DDEBUG_P2P_SETTING_CRASH

EXTRA_CFLAGS += -Wno-error=date-time
EXTRA_CFLAGS +=-Wno-date-time

clean-files += Module.symvers Module.markers modules modules.order
