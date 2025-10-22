RWNX_VERS_NUM := 6.4.3.0

CONFIG_PLATFORM_NANOPI_M4 ?= n
ifneq ($(CONFIG_AW_BSP),)
CONFIG_PLATFORM_ALLWINNER := $(CONFIG_AW_BSP)
else
CONFIG_PLATFORM_ALLWINNER := $(CONFIG_ARCH_SUNXI)
endif
CONFIG_PLATFORM_INGENIC_T20 ?= n

CONFIG_AIC_BSP_CTRL ?= y

# ROM patch enabled option
CONFIG_ROM_PATCH_EN ?=n

#
# WAITING FOR KCONFIG {
#
CONFIG_RWNX_SOFTMAC ?= n
CONFIG_RWNX_FULLMAC ?= y
CONFIG_RWNX_FHOST ?= n

#
# DEBUG OPTIONS
CONFIG_RWNX_UM_HELPER_DFLT ?= "/dini/dini_bin/rwnx_umh.sh"

#
# FW ARCH:
CONFIG_RWNX_SDM ?= n
CONFIG_RWNX_TL4 ?= n

# IPC version
CONFIG_RWNX_OLD_IPC ?= n

# Support of P2P DebugFS for enabling/disabling NoA and OppPS
CONFIG_RWNX_P2P_DEBUGFS := n
#
# } // WAITING FOR KCONFIG
#

# Enable A-MSDU support (need FW support)
## Select this if FW is compiled with AMSDU support
CONFIG_RWNX_SPLIT_TX_BUF ?= n
## Select this TO send AMSDU
CONFIG_RWNX_AMSDUS_TX ?= n

# Enable BFMER support (need FW support)
CONFIG_RWNX_BFMER ?= n

CONFIG_SDIO_SUPPORT := $(CONFIG_AIC_INTF_SDIO)
CONFIG_USB_SUPPORT  := $(CONFIG_AIC_INTF_USB)
CONFIG_PCIE_SUPPORT := $(CONFIG_AIC_INTF_PCIE)
CONFIG_RX_REORDER ?=y
CONFIG_ARP_OFFLOAD =y
CONFIG_RADAR_OR_IR_DETECT =n
CONFIG_DOWNLOAD_FW =y
CONFIG_RFTEST=y
CONFIG_SDIO_PWRCTRL ?= y
ifeq ($(CONFIG_USB_SUPPORT),y)
CONFIG_USB_BT := y
endif
CONFIG_GKI_OPT_FEATURES ?= y
CONFIG_WPA3_FOR_OLD_KERNEL ?= n
CONFIG_SCHED_SCAN = n
CONFIG_RX_NETIF_RECV_SKB = y
CONFIG_OOB = n
CONFIG_DPD = y
CONFIG_FORCE_DPD_CALIB = y
CONFIG_FILTER_TCP_ACK = y
# CONFIG_MCC = n for sta and p2p concurrent in same channel.
CONFIG_MCC = y
CONFIG_ONE_TXQ = n
CONFIG_PREALLOC_TXQ = y
CONFIG_APF = y
CONFIG_PREALLOC_RX_SKB = n
CONFIG_FOR_IPCAM = y

# Enable wifi-hal Latency Mode
CONFIG_AICWF_LATENCY_MODE = n

# Support of MU-MIMO transmission (need FW support)
ifeq ($(CONFIG_RWNX_BFMER), y)
CONFIG_RWNX_MUMIMO_TX ?= n
else
CONFIG_RWNX_MUMIMO_TX = n
endif

# Enable handling of radar event
CONFIG_RWNX_RADAR ?= y

# Enable HW queue for Broadcast/Multicast traffic (need FW support)
CONFIG_RWNX_BCMC ?= y

# Enable Monitor+Data interface support (need FW support)
CONFIG_RWNX_MON_DATA =y

# extra DEBUG config
CONFIG_RWNX_SW_PROFILING ?= n
CONFIG_RWNX_DBG ?= y

obj-$(CONFIG_AIC8800_WLAN_SUPPORT) := aic8800_fdrv.o
aic8800_fdrv-y := \
	rwnx_wakelock.o        \
	rwnx_gki.o             \
	rwnx_msg_tx.o          \
	rwnx_msg_rx.o          \
	rwnx_utils.o           \
	rwnx_cmds.o            \
	rwnx_irqs.o            \
	rwnx_cfgfile.o         \
	rwnx_strs.o            \
	rwnx_rx.o              \
	rwnx_tx.o              \
	rwnx_txq.o             \
	rwnx_main.o            \
	rwnx_mod_params.o      \
	rwnx_mesh.o            \
	rwnx_platform.o        \
	rwnx_dini.o            \
	rwnx_v7.o              \
	ipc_host.o             \
	rwnx_tdls.o            \
	aic_vendor.o           \
	aicwf_8800dc.o         \
	aicwf_8800d80.o        \

aic8800_fdrv-$(CONFIG_RWNX_RADAR)       += rwnx_radar.o
aic8800_fdrv-$(CONFIG_DEBUG_FS)         += rwnx_debugfs.o
aic8800_fdrv-$(CONFIG_DEBUG_FS)         += rwnx_fw_trace.o
aic8800_fdrv-$(CONFIG_NL80211_TESTMODE) += rwnx_testmode.o
aic8800_fdrv-$(CONFIG_RWNX_BFMER)       += rwnx_bfmer.o
aic8800_fdrv-$(CONFIG_RWNX_MUMIMO_TX)   += rwnx_mu_group.o
aic8800_fdrv-$(CONFIG_SDIO_SUPPORT)     += sdio_host.o
aic8800_fdrv-$(CONFIG_SDIO_SUPPORT)     += aicwf_txrxif.o
aic8800_fdrv-$(CONFIG_FILTER_TCP_ACK)   += aicwf_tcp_ack.o
aic8800_fdrv-$(CONFIG_SDIO_SUPPORT)     += aicwf_sdio.o
aic8800_fdrv-$(CONFIG_PREALLOC_RX_SKB)  += aicwf_rx_prealloc.o

aic8800_fdrv-$(CONFIG_USB_SUPPORT)     += usb_host.o
aic8800_fdrv-$(CONFIG_USB_SUPPORT)     += aicwf_txrxif.o
aic8800_fdrv-$(CONFIG_USB_SUPPORT)     += aicwf_usb.o

aic8800_fdrv-$(CONFIG_PCIE_SUPPORT)    += rwnx_pci.o

ccflags-y += -Wno-implicit-fallthrough
ccflags-y += -Wno-frame-larger-than

ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_RWNX_DEBUGFS
ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_RWNX_UM_HELPER_DFLT=\"$(CONFIG_RWNX_UM_HELPER_DFLT)\"
ccflags-$(CONFIG_RWNX_P2P_DEBUGFS) += -DCONFIG_RWNX_P2P_DEBUGFS
ccflags-$(CONFIG_GKI_OPT_FEATURES) += -DCONFIG_GKI_OPT_FEATURES

ifeq ($(CONFIG_SDIO_SUPPORT), y)
ccflags-$(CONFIG_SDIO_PWRCTRL) += -DCONFIG_SDIO_PWRCTRL
endif

# FW VARS
ccflags-y += -DNX_VIRT_DEV_MAX=4
ccflags-y += -DNX_REMOTE_STA_MAX_FOR_OLD_IC=10
ccflags-y += -DNX_REMOTE_STA_MAX=32
ccflags-y += -DNX_MU_GROUP_MAX=62
ccflags-y += -DNX_TXDESC_CNT=64
ccflags-y += -DNX_TX_MAX_RATES=4
ccflags-y += -DNX_CHAN_CTXT_CNT=3

# FW ARCH:
ccflags-$(CONFIG_RWNX_SDM) += -DCONFIG_RWNX_SDM
ccflags-$(CONFIG_RWNX_TL4) += -DCONFIG_RWNX_TL4
ccflags-$(CONFIG_RWNX_OLD_IPC) += -DCONFIG_RWNX_OLD_IPC
ccflags-$(CONFIG_PLATFORM_NANOPI_M4) += -DCONFIG_NANOPI_M4
ccflags-$(CONFIG_PLATFORM_INGENIC_T20) += -DCONFIG_INGENIC_T20
ccflags-$(CONFIG_PLATFORM_ALLWINNER) += -DCONFIG_PLATFORM_ALLWINNER
ccflags-$(CONFIG_ROM_PATCH_EN) += -DCONFIG_ROM_PATCH_EN

ccflags-$(CONFIG_PLATFORM_ALLWINNER) += -DCONFIG_COEX
ccflags-$(CONFIG_PLATFORM_NANOPI_M4) += -DCONFIG_COEX

ccflags-$(CONFIG_AW_KERNEL_AOSP) += -DKERNEL_AOSP

SRCPATH := $(shell dirname $(lastword $(MAKEFILE_LIST)))

ccflags-y += -DCONFIG_RWNX_FULLMAC
ccflags-y += -I$(srctree)
ccflags-y += -I$(SRCPATH)
ccflags-y += -I$(SRCPATH)/../aic8800_bsp
ccflags-$(CONFIG_RWNX_RADAR) += -DCONFIG_RWNX_RADAR
ccflags-$(CONFIG_RWNX_MON_DATA) += -DCONFIG_RWNX_MON_DATA
ccflags-$(CONFIG_RWNX_BFMER) += -DCONFIG_RWNX_BFMER
ccflags-$(CONFIG_RWNX_SPLIT_TX_BUF) += -DCONFIG_RWNX_SPLIT_TX_BUF
ifeq ($(CONFIG_RWNX_SPLIT_TX_BUF), y)
ccflags-$(CONFIG_RWNX_AMSDUS_TX) += -DCONFIG_RWNX_AMSDUS_TX
endif
ccflags-$(CONFIG_RWNX_DBG) += -DCONFIG_RWNX_DBG
ccflags-$(CONFIG_RWNX_SW_PROFILING) += -DCONFIG_RWNX_SW_PROFILING
ccflags-$(CONFIG_RWNX_MUMIMO_TX) += -DCONFIG_RWNX_MUMIMO_TX
ccflags-$(CONFIG_RFTEST) += -DCONFIG_RFTEST
ccflags-$(CONFIG_WPA3_FOR_OLD_KERNEL) += -DCONFIG_WPA3_FOR_OLD_KERNEL
ccflags-$(CONFIG_SCHED_SCAN) += -DCONFIG_SCHED_SCAN
ccflags-$(CONFIG_RX_NETIF_RECV_SKB) += -DCONFIG_RX_NETIF_RECV_SKB
ccflags-$(CONFIG_OOB) += -DCONFIG_OOB
ccflags-$(CONFIG_AICWF_LATENCY_MODE) += -DAICWF_LATENCY_MODE
ccflags-$(CONFIG_DPD) += -DCONFIG_DPD
ccflags-$(CONFIG_FORCE_DPD_CALIB) += -DCONFIG_FORCE_DPD_CALIB -DCONFIG_DPD
ccflags-$(CONFIG_FILTER_TCP_ACK) += -DCONFIG_FILTER_TCP_ACK
ccflags-$(CONFIG_MCC) += -DCONFIG_MCC
ccflags-$(CONFIG_ONE_TXQ)  += -DCONFIG_ONE_TXQ
ccflags-$(CONFIG_PREALLOC_TXQ) += -DCONFIG_PREALLOC_TXQ
ccflags-$(CONFIG_APF) += -DCONFIG_APF
ccflags-$(CONFIG_PREALLOC_RX_SKB) += -DCONFIG_PREALLOC_RX_SKB
ccflags-$(CONFIG_FOR_IPCAM) += -DCONFIG_FOR_IPCAM

ccflags-$(CONFIG_AIC_BSP_CTRL) += -DAICWF_BSP_CTRL
ccflags-$(CONFIG_SDIO_SUPPORT) += -DAICWF_SDIO_SUPPORT
ccflags-$(CONFIG_USB_SUPPORT)  += -DAICWF_USB_SUPPORT

ifeq ($(CONFIG_PCIE_SUPPORT), y)
ccflags-y += -DAICWF_PCIE_SUPPORT
endif

ifeq ($(CONFIG_RWNX_MUMIMO_TX), y)
ccflags-y += -DCONFIG_USER_MAX=2
else
ccflags-y += -DCONFIG_USER_MAX=1
endif

ifeq ($(CONFIG_RWNX_BCMC), y)
ccflags-y += -DNX_TXQ_CNT=5
else
ccflags-y += -DNX_TXQ_CNT=4
endif

# For old kernel (<=3.19)
ifeq ($(shell test $(VERSION) -lt 4 -a "$(CONFIG_VENDOR_RWNX)" = y ; echo $$?),0)
ccflags-y += -DCONFIG_VENDOR_RWNX_VHT_NO80
endif

ccflags-$(CONFIG_RX_REORDER) += -DAICWF_RX_REORDER
ccflags-$(CONFIG_ARP_OFFLOAD) += -DAICWF_ARP_OFFLOAD
ccflags-$(CONFIG_RADAR_DETECT) += -DRADAR_OR_IR_DETECT
ccflags-$(CONFIG_DOWNLOAD_FW)  += -DCONFIG_DOWNLOAD_FW
ccflags-$(CONFIG_USB_BT)  += -DCONFIG_USB_BT
ccflags-$(CONFIG_USB_MSG_EP) += -DCONFIG_USB_MSG_EP
