# SPDX-License-Identifier: GPL-2.0-only

# # Standalone Makefile - uncomment for out-of-tree compilation
# CONFIG_WLAN_VENDOR_XRADIO := m
# ccflags-y += -DCONFIG_XRADIO_USE_EXTENSIONS
# ccflags-y += -DCONFIG_XRADIO_WAPI_SUPPORT

# Kernel part

obj-$(CONFIG_WLAN_VENDOR_XRADIO) += xradio_wlan.o

xradio_wlan-objs := \
	fwio.o \
	tx.o \
	rx.o \
	main.o \
	queue.o \
	hwio.o \
	bh.o \
	wsm.o \
	sta.o \
	ap.o \
	keys.o \
	scan.o \
	module.o \
	sdio.o \
	pm.o \
	ht.o \
	p2p.o

# ccflags-y += -DMCAST_FWDING
# ccflags-y += -DXRADIO_SUSPEND_RESUME_FILTER_ENABLE
# ccflags-y += -DAP_AGGREGATE_FW_FIX

# ccflags-y += -DAP_HT_CAP_UPDATE
# ccflags-y += -DAP_HT_COMPAT_FIX

# # Extra IE for probe response from upper layer is needed in P2P GO
# # For offloading probe response to FW, the extra IE must be included
# # in the probe response template. Required for hidden SSID.
# ccflags-y += -DPROBE_RESP_EXTRA_IE

# # Modified for P2P stability.
# ccflags-y += -DTES_P2P_0002_ROC_RESTART
# ccflags-y += -DTES_P2P_000B_EXTEND_INACTIVITY_CNT
# ccflags-y += -DTES_P2P_000B_DISABLE_EAPOL_FILTER

# # Modified for power save.
# ccflags-y += -DCONFIG_PM
# ccflags-y += -DCONFIG_XRADIO_SUSPEND_POWER_OFF
# ccflags-y += -DXRADIO_USE_LONG_DTIM_PERIOD
# ccflags-y += -DXRADIO_USE_LONG_KEEP_ALIVE_PERIOD

# # Not functional at this point
# #ccflags-y += -DROAM_OFFLOAD

# #ccflags-y += -DDEBUG

ldflags-y += --strip-debug

