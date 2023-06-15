RWNX_VERS_NUM := 6.4.3.0

CONFIG_PLATFORM_NANOPI_M4 ?= n
CONFIG_PLATFORM_ALLWINNER ?= y
CONFIG_PLATFORM_INGENIC_T20 ?= n

# Support of bootrom start
CONFIG_START_FROM_BOOTROM = y

# Support of pmic setting, new version bootrom avaliable
CONFIG_PMIC_SETTING ?=y

# ROM patch enabled option
CONFIG_ROM_PATCH_EN ?=n

#
# WAITING FOR KCONFIG {
#
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

CONFIG_SDIO_SUPPORT =y
CONFIG_USB_SUPPORT =n
CONFIG_RX_REORDER ?=y
CONFIG_ARP_OFFLOAD =y
CONFIG_RADAR_OR_IR_DETECT =n
CONFIG_DOWNLOAD_FW =y
CONFIG_RFTEST=y
CONFIG_USB_BT =n
CONFIG_GKI_OPT_FEATURES ?= y


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
	rwnx_pci.o             \
	rwnx_dini.o            \
	rwnx_v7.o              \
	ipc_host.o             \
	rwnx_tdls.o            \
	aic_vendor.o

aic8800_fdrv-$(CONFIG_RWNX_RADAR)       += rwnx_radar.o
aic8800_fdrv-$(CONFIG_DEBUG_FS)         += rwnx_debugfs.o
aic8800_fdrv-$(CONFIG_DEBUG_FS)         += rwnx_fw_trace.o
aic8800_fdrv-$(CONFIG_NL80211_TESTMODE) += rwnx_testmode.o
aic8800_fdrv-$(CONFIG_RWNX_BFMER)       += rwnx_bfmer.o
aic8800_fdrv-$(CONFIG_RWNX_MUMIMO_TX)   += rwnx_mu_group.o
aic8800_fdrv-$(CONFIG_SDIO_SUPPORT)     += sdio_host.o
aic8800_fdrv-$(CONFIG_SDIO_SUPPORT)     += aicwf_txrxif.o
aic8800_fdrv-$(CONFIG_SDIO_SUPPORT)     += aicwf_sdio.o

aic8800_fdrv-$(CONFIG_USB_SUPPORT)     += usb_host.o
aic8800_fdrv-$(CONFIG_USB_SUPPORT)     += aicwf_txrxif.o
aic8800_fdrv-$(CONFIG_USB_SUPPORT)     += aicwf_usb.o

ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_RWNX_DEBUGFS
ccflags-$(CONFIG_DEBUG_FS) += -DCONFIG_RWNX_UM_HELPER_DFLT=\"$(CONFIG_RWNX_UM_HELPER_DFLT)\"
ccflags-$(CONFIG_RWNX_P2P_DEBUGFS) += -DCONFIG_RWNX_P2P_DEBUGFS
ccflags-$(CONFIG_GKI_OPT_FEATURES) += -DCONFIG_GKI_OPT_FEATURES

# FW VARS
ccflags-y += -DNX_VIRT_DEV_MAX=4
ccflags-y += -DNX_REMOTE_STA_MAX=10
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
ccflags-$(CONFIG_START_FROM_BOOTROM) += -DCONFIG_START_FROM_BOOTROM
ccflags-$(CONFIG_PMIC_SETTING) += -DCONFIG_PMIC_SETTING
ccflags-$(CONFIG_ROM_PATCH_EN) += -DCONFIG_ROM_PATCH_EN

CURRENT_CODE_VERSION := $(VERSION).$(PATCHLEVEL).$(SUBLEVEL)
MINI_VERSION_REQUEST := 4.14.0
ifeq ($(shell expr $(CURRENT_CODE_VERSION) \< $(MINI_VERSION_REQUEST)),1)
ccflags-y += -DCONFIG_HE_FOR_OLD_KERNEL
endif
ccflags-$(CONFIG_PLATFORM_ALLWINNER) += -DCONFIG_COEX
ccflags-$(CONFIG_PLATFORM_NANOPI_M4) += -DCONFIG_COEX

ccflags-y += -DCONFIG_RWNX_FULLMAC
ccflags-y += -I$(srctree)
ccflags-y += -I$(srctree)/$(src)
ccflags-y += -I$(srctree)/$(src)/../aic8800_bsp
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

ifeq ($(CONFIG_SDIO_SUPPORT), y)
ccflags-y += -DAICWF_SDIO_SUPPORT
endif

ifeq ($(CONFIG_USB_SUPPORT), y)
ccflags-y += -DAICWF_USB_SUPPORT
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
