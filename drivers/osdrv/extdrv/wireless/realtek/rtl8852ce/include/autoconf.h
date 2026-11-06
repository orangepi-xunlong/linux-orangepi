/******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
/***** temporarily flag for IC development phase *******/
#define CONFIG_SINGLE_IMG

/*#define CONFIG_NO_FW*/
/*#define CONFIG_DISABLE_ODM*/

#ifdef CONFIG_CONCURRENT_MODE
#define CONFIG_DBCC_SUPPORT

#ifdef CONFIG_DBCC_SUPPORT
/*#define CONFIG_DBCC_FORCE*/
#endif
#endif

#define RTW_WKARD_CORE_RSSI_V1

#ifdef RTW_WKARD_CORE_RSSI_V1
#define CONFIG_RX_PSTS_PER_PKT
#define CONFIG_SIGNAL_STAT_PROCESS
#endif

#ifndef DBG_MEM_ALLOC
#define DBG_MEM_ALLOC

#define DBG_PHL_MEM_ALLOC
#define DBG_HAL_MAC_MEM_MOINTOR
#define DBG_HAL_MEM_MOINTOR
#endif
/*#define CONFIG_PHL_USE_KMEM_ALLOC*/
#define CONFIG_HW_RTS

/*
 * Work around Config
 */
#define RTW_WKARD_DIS_PROBE_REQ_RPT_TO_HOSTAPD

#ifdef CONFIG_BTC
#define RTK_WKARD_CORE_BTC_STBC_CAP
#endif

#define RTW_WKARD_LIMIT_MAX_TXAGG

#define RTW_WKARD_PCI_DEVRM_DIS_INT

/***** temporarily flag *******/
/*
 * Public  General Config
 */
#define AUTOCONF_INCLUDED
#define DRV_NAME "rtl8852ce"

#define CONFIG_PCI_HCI

#define CONFIG_PCIE_TRX_MIT
#ifdef CONFIG_PCIE_TRX_MIT
	#define PCIE_RX_INT_MIT_TIMER 4096
	/*#define CONFIG_PCIE_TRX_MIT_FIX*/ /*  if defined, the mitigation mode will be set to fixed */
	#ifndef CONFIG_PCIE_TRX_MIT_FIX
	#define CONFIG_PCIE_TRX_MIT_DYN
	#endif
#endif

#define CONFIG_RPQ_AGG_NUM 30
/* #define CONFIG_TX_WD_NUM 2048 */
#define MAX_PHL_TX_RING_ENTRY_NUM 512
#define MAX_PHL_RX_RING_ENTRY_NUM 1024
#define CONFIG_RTW_REDUCE_MEM  /* Note: if CONFIG_RTW_REDUCE_MEM is not defined, MAX_PHL_RX_RING_ENTRY_NUM have no effect. */
#ifdef CONFIG_RTW_REDUCE_MEM
#define CORE_RXBUF_SIZE 11460
#define CORE_RPBUF_SIZE 192 /* 192: up to 42 agg num */
#define CORE_RPBD_NUM 64
#define CORE_RPBUF_NUM (CORE_RPBD_NUM + 32)
#define MAX_ASMDU_LEN 2
/* HT -   0: 3839, 1: 7935  octets - Maximum A-MSDU Length
 * VHT - 0: 3895, 1: 7991, 2:11454  octets - Maximum MPDU Length
 */

#define NR_XMITFRAME		512
#define MAX_TX_RING_NUM		512
#define RTW_MAX_FRAG_NUM	1
#endif

/*
 * Wi-Fi Functions Config
 */

/*#define CONFIG_RECV_REORDERING_CTRL*/

#define CONFIG_80211N_HT
#define CONFIG_80211AC_VHT
#define CONFIG_80211AX_HE
#ifdef CONFIG_80211AC_VHT
	#ifndef CONFIG_80211N_HT
		#define CONFIG_80211N_HT
	#endif
#endif

#ifdef CONFIG_80211AX_HE
	#ifndef CONFIG_80211N_HT
		#define CONFIG_80211N_HT
	#endif
	#ifndef CONFIG_80211AC_VHT
		#define CONFIG_80211AC_VHT
	#endif
#endif

#define CONFIG_BEAMFORMING
#ifdef CONFIG_BEAMFORMING
/*#define RTW_WKARD_TX_DISABLE_BFEE*/
#endif

/*#define CONFIG_IOCTL_CFG80211*/
#ifdef CONFIG_IOCTL_CFG80211
	/*#define RTW_USE_CFG80211_STA_EVENT*/ /* Indecate new sta asoc through cfg80211_new_sta */
	#define CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER
	/*#define CONFIG_DEBUG_CFG80211*/
	#define CONFIG_SET_SCAN_DENY_TIMER
#endif
#define CONFIG_TX_AMSDU
#ifdef CONFIG_TX_AMSDU
	#ifdef CONFIG_PLATFORM_RTL8198D
	#define CONFIG_TX_AMSDU_HW_MODE	1
	#else
	#define CONFIG_TX_AMSDU_SW_MODE	1
	#endif
#endif

/*
 * Internal  General Config
 */
/*#define CONFIG_PWRCTRL*/
#define CONFIG_TRX_BD_ARCH	/* PCI only */
#define USING_RX_TAG

#define CONFIG_EMBEDDED_FWIMG

#ifdef CONFIG_EMBEDDED_FWIMG
	#define	LOAD_FW_HEADER_FROM_DRIVER
#endif
/*#define CONFIG_FILE_FWIMG*/

#define CONFIG_XMIT_ACK
#ifdef CONFIG_XMIT_ACK
	#define CONFIG_XMIT_ACK_BY_REL_RPT
	/*#define CONFIG_XMIT_ACK_BY_REL_RPT_DBG*/
	#define CONFIG_ACTIVE_KEEP_ALIVE_CHECK
	#define RTW_WKARD_TX_NULL_WD_RP
#endif

#define BUF_DESC_ARCH		/* if defined, hardware follows Rx buffer descriptor architecture */

#ifdef CONFIG_POWER_SAVE
	/* #define CONFIG_RTW_IPS */
	/* #define CONFIG_RTW_LPS */
#endif /* CONFIG_POWER_SAVE */

	/*#define CONFIG_ANTENNA_DIVERSITY*/


/*#define CONFIG_PCI_ASPM*/
#ifdef CONFIG_PCI_ASPM
#define CONFIG_PCI_DYNAMIC_ASPM
#endif

#define CONFIG_AP_MODE
#ifdef CONFIG_AP_MODE
	#define CONFIG_NATIVEAP_MLME
	#ifndef CONFIG_NATIVEAP_MLME
		#define CONFIG_HOSTAPD_MLME
	#endif
	/*#define CONFIG_RTW_HOSTAPD_ACS*/
	/*#define CONFIG_FIND_BEST_CHANNEL*/
	/*#define CONFIG_AUTO_AP_MODE*/
#endif

#define CONFIG_P2P
#ifdef CONFIG_P2P
	/* The CONFIG_WFD is for supporting the Wi-Fi display */
	#define CONFIG_WFD

	#define CONFIG_P2P_REMOVE_GROUP_INFO

	/*#define CONFIG_DBG_P2P*/

	#define CONFIG_P2P_PS
	/*#define CONFIG_P2P_IPS*/
	#define CONFIG_P2P_OP_CHK_SOCIAL_CH
	#define CONFIG_CFG80211_ONECHANNEL_UNDER_CONCURRENT  /* replace CONFIG_P2P_CHK_INVITE_CH_LIST flag */
	/*#define CONFIG_P2P_INVITE_IOT*/
#endif

/* Added by Kurt 20110511 */
#ifdef CONFIG_TDLS
	#define CONFIG_TDLS_DRIVER_SETUP
#if 0
	#ifndef CONFIG_WFD
		#define CONFIG_WFD
	#endif
	#define CONFIG_TDLS_AUTOSETUP
#endif
	#define CONFIG_TDLS_AUTOCHECKALIVE
	/* #define CONFIG_TDLS_CH_SW */ /* Not support yet */
#endif

#define CONFIG_SKB_COPY	/* for amsdu */

/*#define CONFIG_RTW_LED*/
#ifdef CONFIG_RTW_LED
	/*#define CONFIG_RTW_SW_LED*/
	#ifdef CONFIG_RTW_SW_LED
		/*#define CONFIG_RTW_LED_HANDLED_BY_CMD_THREAD*/
	#endif
#endif /* CONFIG_RTW_LED */

#define CONFIG_GLOBAL_UI_PID

/*#define CONFIG_ADAPTOR_INFO_CACHING_FILE*/ /* now just applied on 8192cu only, should make it general...*/
/*#define CONFIG_RESUME_IN_WORKQUEUE*/
/*#define CONFIG_SET_SCAN_DENY_TIMER*/
#define CONFIG_LONG_DELAY_ISSUE
/*#define CONFIG_SIGNAL_DISPLAY_DBM*/ /* display RX signal with dbm */
#ifdef CONFIG_SIGNAL_DISPLAY_DBM
/*#define CONFIG_BACKGROUND_NOISE_MONITOR*/
#endif

#define CONFIG_IEEE80211_BAND_6GHZ 1

/*
 * Software feature Related Config
 */
#define CONFIG_SCAN_BACKOP_STA

/*
 * Interface  Related Config
 */
/* #define CONFIG_RTW_FORCE_PCI_MSI_DISABLE */

#define CONFIG_DIS_DYN_RXBUF

#ifndef CONFIG_DIS_DYN_RXBUF
#define CONFIG_DYNAMIC_RX_BUF
#endif

/*
 * HAL  Related Config
 */
#define CONFIG_RX_PACKET_APPEND_FCS


#define DISABLE_BB_RF	0

#ifdef CONFIG_MP_INCLUDED
	#define MP_DRIVER 1
#else
	#define MP_DRIVER 0
#endif

#ifndef EFUSE_MAP_PATH
	#define EFUSE_MAP_PATH "/system/etc/wifi/wifi_efuse.map"
#endif
#ifndef WIFIMAC_PATH
	#define WIFIMAC_PATH "/data/wifimac.txt"
#endif

/* Use cmd frame to issue beacon. Use a fixed buffer for beacon. */
#define CONFIG_BCN_ICF

#ifdef CONFIG_HWSIM
/* Use pure sw beacon */
#undef CONFIG_BCN_ICF
#endif

/* #define RTL8814BE_AMPDU_PRE_TX_OFF */

/*
 * Platform  Related Config
 */


/* #define	CONFIG_TX_EARLY_MODE */


/*
 * Debug Related Config
 */
#define DBG	1


/*#define DBG_CONFIG_ERROR_DETECT*/
/* #define DBG_CONFIG_ERROR_DETECT_INT */
/* #define DBG_CONFIG_ERROR_RESET */

/* #define DBG_IO */
/* #define DBG_DELAY_OS */
/* #define DBG_MEM_ALLOC */
/* #define DBG_IOCTL */

/* #define DBG_TX */
/* #define DBG_XMIT_BUF */
/* #define DBG_XMIT_BUF_EXT */
/* #define DBG_TX_DROP_FRAME */

/* #define DBG_RX_DROP_FRAME */
/* #define DBG_RX_SEQ */
/* #define DBG_RX_SIGNAL_DISPLAY_PROCESSING */
/* #define DBG_RX_SIGNAL_DISPLAY_SSID_MONITORED "jeff-ap" */

/* #define DBG_ROAMING_TEST */

/* #define DBG_HAL_INIT_PROFILING */

/*#define DBG_MEMORY_LEAK*/
/* #define CONFIG_FW_C2H_DEBUG */

#define CONFIG_DBG_COUNTER
#define	DBG_RX_DFRAME_RAW_DATA
/*#define	DBG_TXBD_DESC_DUMP*/

#define CONFIG_PCI_BCN_POLLING
//#define RTW_PHL_TEST_FPGA //For 8852A PCIE FPGA TEST


/*#define CONFIG_RTW_BTM_ROAM*/
/*#define CONFIG_RTW_80211R*/

#ifdef CONFIG_RTW_MBO
	#ifndef CONFIG_RTW_WNM
		#define CONFIG_RTW_WNM
	#endif
	#ifndef CONFIG_RTW_80211K
		#define CONFIG_RTW_80211K
	#endif
#endif /* CONFIG_RTW_MBO */

#ifdef RTW_PHL_TEST_FPGA

	#ifndef RTW_PHL_TX
	#define RTW_PHL_TX
	#endif
	#ifndef RTW_PHL_RX
	#define RTW_PHL_RX
	#endif
	#ifndef DIRTY_FOR_WORK
	#define DIRTY_FOR_WORK
	#endif
	#ifndef CONFIG_DYNAMIC_RX_BUF
	#define CONFIG_DYNAMIC_RX_BUF
	#endif
	#ifndef RTW_PHL_DBG_CMD
	#define RTW_PHL_DBG_CMD
	#endif
	#ifndef CONFIG_DRV_FAKE_AP
	#error "Please enable CONFIG_DRV_FAKE_AP in Makefile before Beacon ready\n"
	#endif
	#ifndef RTW_PHL_FWDL
	#define RTW_PHL_FWDL
	#endif

	#ifdef CONFIG_RTW_NAPI
	#undef CONFIG_RTW_NAPI
	#endif
	#ifdef CONFIG_RTW_GRO
	#undef CONFIG_RTW_GRO
	#endif
	#ifdef CONFIG_RTW_NETIF_SG
	#undef CONFIG_RTW_NETIF_SG
	#endif

	#if 1
	#define	DBGP(fmt, args...)	printk("dbg [%s][%d]"fmt, __FUNCTION__, __LINE__, ## args)
	#else
	#define DBGP(arg...) do {} while (0)
	#endif

#else //RTW_PHL_TEST_FPGA

	#define DBGP(arg...) do {} while (0)

#endif

/* Platform dependent config, shall put on the bottom of this file */
#ifdef CONFIG_PLATFORM_RTL8198D
#include "autoconf_mips_98d.h"
#endif

/* Platform dependent config, shall put on the bottom of this file */
#ifdef CONFIG_I386_BUILD_VERIFY
#include "autoconf_i386_ap_func.h"
#endif

#ifdef CONFIG_ARCH_CORTINA
#include "autoconf_arm_9617b.h"
#endif /* CONFIG_ARCH_CORTINA */

#define CONFIG_64BIT_DMA
