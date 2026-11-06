/******************************************************************************
 *
 * Copyright(c) 2015 - 2017 Realtek Corporation.
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
/***** temporarily flag *******/
#define CONFIG_SINGLE_IMG
/* #define CONFIG_DISABLE_ODM */

/***** temporarily flag *******/
/*
 * Public  General Config
 */
#define AUTOCONF_INCLUDED
#define DRV_NAME "rtl88x2ce"

#define CONFIG_PCI_HCI
#define CONFIG_PCIE_HCI
#define PLATFORM_LINUX

/*
 * Wi-Fi Functions Config
 */

#define CONFIG_RECV_REORDERING_CTRL

#define CONFIG_80211N_HT
#define CONFIG_80211AC_VHT
#ifdef CONFIG_80211AC_VHT
	#ifndef CONFIG_80211N_HT
		#define CONFIG_80211N_HT
	#endif
#endif

/*#define CONFIG_IOCTL_CFG80211*/
#ifdef CONFIG_IOCTL_CFG80211
	/*#define RTW_USE_CFG80211_STA_EVENT*/ /* Indecate new sta asoc through cfg80211_new_sta */
	#define CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER
	/*#define CONFIG_DEBUG_CFG80211*/
	/*#define CONFIG_DRV_ISSUE_PROV_REQ*/ /* IOT FOR S2 */
	#define CONFIG_SET_SCAN_DENY_TIMER
#endif
#define CONFIG_TX_AMSDU

/*
 * Internal  General Config
 */
/*#define CONFIG_PWRCTRL*/
/*#define CONFIG_H2CLBK*/
#define CONFIG_TRX_BD_ARCH	/* PCI only */
#define USING_RX_TAG

#define CONFIG_EMBEDDED_FWIMG

#ifdef CONFIG_EMBEDDED_FWIMG
	#define	LOAD_FW_HEADER_FROM_DRIVER
#endif
/*#define CONFIG_FILE_FWIMG*/

#define CONFIG_XMIT_ACK
#ifdef CONFIG_XMIT_ACK
	#define CONFIG_ACTIVE_KEEP_ALIVE_CHECK
#endif

/*#define CONFIG_DISABLE_MCS13TO15	1*/	/* Disable MSC13-15 rates for more stable TX throughput with some 5G APs */

#define BUF_DESC_ARCH		/* if defined, hardware follows Rx buffer descriptor architecture */

#ifdef CONFIG_POWER_SAVING

	#define CONFIG_IPS
	#ifdef CONFIG_IPS
		/*#define CONFIG_IPS_LEVEL_2*/ /* enable this to set default IPS mode to IPS_LEVEL_2 */
		/*#define CONFIG_FWLPS_IN_IPS*/
	#endif

	#define CONFIG_LPS

	#if defined(CONFIG_LPS)
		/*#define CONFIG_LPS_LCLK*/ /* 32K */
	#endif

	#ifdef CONFIG_LPS_LCLK
		#define CONFIG_XMIT_THREAD_MODE
		#define LPS_RPWM_WAIT_MS 300
		/*#define CONFIG_LPS_PG*/
		#define DBG_CHECK_FW_PS_STATE
	#endif

	#ifdef CONFIG_LPS
		#define CONFIG_WMMPS_STA 1
	#endif /* CONFIG_LPS */

#endif

/*#define CONFIG_PCI_ASPM*/
#ifdef CONFIG_PCI_ASPM
#define CONFIG_PCI_DYNAMIC_ASPM_L1_LATENCY
#endif

#define CONFIG_HIGH_CHAN_SUPER_CALIBRATION
/*#define SUPPORT_HW_RFOFF_DETECTED*/
/*#define CONFIG_ANTENNA_DIVERSITY*/

#ifdef CONFIG_AP_MODE
	/*#define CONFIG_INTERRUPT_BASED_TXBCN*/ /* Tx Beacon when driver BCN_OK ,BCN_ERR interrupt occurs */
	#if defined(CONFIG_CONCURRENT_MODE) && defined(CONFIG_INTERRUPT_BASED_TXBCN)
		#undef CONFIG_INTERRUPT_BASED_TXBCN
	#endif
	#ifdef CONFIG_INTERRUPT_BASED_TXBCN
		/*#define CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT*/
		#define CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR
	#endif

	#define CONFIG_NATIVEAP_MLME
	#ifndef CONFIG_NATIVEAP_MLME
		#define CONFIG_HOSTAPD_MLME
	#endif
	#define CONFIG_FIND_BEST_CHANNEL
	/*#define CONFIG_AUTO_AP_MODE*/
#endif

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
	#define CONFIG_TDLS_CH_SW		/* Enable "CONFIG_TDLS_CH_SW" by default, however limit it to only work in wifi logo test mode but not in normal mode currently */
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

/*#define CONFIG_RTW_80211K*/
/*#define CONFIG_ADAPTOR_INFO_CACHING_FILE*/ /* now just applied on 8192cu only, should make it general...*/
/*#define CONFIG_RESUME_IN_WORKQUEUE*/
/*#define CONFIG_SET_SCAN_DENY_TIMER*/
#define CONFIG_LONG_DELAY_ISSUE
#define CONFIG_NEW_SIGNAL_STAT_PROCESS
/*#define CONFIG_SIGNAL_DISPLAY_DBM*/ /* display RX signal with dbm */
#ifdef CONFIG_SIGNAL_DISPLAY_DBM
/*#define CONFIG_BACKGROUND_NOISE_MONITOR*/
#endif

#define RTW_NOTCH_FILTER 0 /* 0:Disable, 1:Enable, */

#define CONFIG_BEAMFORMING

/*#define CONFIG_SUPPORT_TRX_SHARED*/
#ifdef CONFIG_SUPPORT_TRX_SHARED
#define DFT_TRX_SHARE_MODE	1
#endif

/*
 * Software feature Related Config
 */
#define RTW_HALMAC		/* Use HALMAC architecture, necessary for 8822B */

/*
 * Interface  Related Config
 */
#define CONFIG_USB_CONFIG_OFFLOAD_8822C

/*
 * Define for using dma_alloc_coherent/dma_free_coherent DMA API for PCIe Tx/Rx
 */
/*#define CONFIG_PCIE_DMA_COHERENT*/

#ifdef CONFIG_SECURITY_MEM
#ifndef CONFIG_PCIE_DMA_COHERENT
#define CONFIG_PCIE_DMA_COHERENT
#endif
#endif

/*
 * HAL  Related Config
 */

#define RTL8192E_RX_PACKET_INCLUDE_CRC	0
#define CONFIG_RX_PACKET_APPEND_FCS
#define SUPPORTED_BLOCK_IO
#define RTL8188E_FW_DOWNLOAD_ENABLE

/*#define CONFIG_ONLY_ONE_OUT_EP_TO_LOW	0*/

#define CONFIG_OUT_EP_WIFI_MODE	0

#define ENABLE_USB_DROP_INCORRECT_OUT


#define DISABLE_BB_RF	0

/*#define RTL8191C_FPGA_NETWORKTYPE_ADHOC 0*/

#ifdef CONFIG_MP_INCLUDED
	#define MP_DRIVER 1
	#define CONFIG_MP_IWPRIV_SUPPORT	1
#else
	#define MP_DRIVER 0
#endif

/* Use cmd frame to issue beacon. Use a fixed buffer for beacon. */
#define CONFIG_BCN_ICF

/*
 * Platform  Related Config
 */


#ifdef CONFIG_BT_COEXIST
	/* for ODM and outsrc BT-Coex */
	#ifndef CONFIG_LPS
		#define CONFIG_LPS	/* download reserved page to FW */
	#endif
	/* Enable ZigBee coexist */
	/* #define CONFIG_RF4CE_COEXIST */

#endif /* !CONFIG_BT_COEXIST */


#ifdef CONFIG_USB_TX_AGGREGATION
/*#define CONFIG_TX_EARLY_MODE*/
#endif

#ifdef CONFIG_TX_EARLY_MODE
#define	RTL8192E_EARLY_MODE_PKT_NUM_10	0
#endif

/* Try to handle the Beacon error found in some types of TP-LINK APs */
#define CONFIG_ATTEMPT_TO_FIX_AP_BEACON_ERROR

/*
 * Debug Related Config
 */
#define DBG	1

#define CONFIG_DBG_COUNTER

#define DBG_CONFIG_ERROR_DETECT
/*
#define DBG_CONFIG_ERROR_DETECT_INT
#define DBG_CONFIG_ERROR_RESET

#define DBG_IO
#define DBG_DELAY_OS
#define DBG_MEM_ALLOC
#define DBG_IOCTL

#define DBG_TX
#define DBG_XMIT_BUF
#define DBG_XMIT_BUF_EXT
#define DBG_TX_DROP_FRAME

#define DBG_RX_DROP_FRAME
#define DBG_RX_SEQ
#define DBG_RX_SIGNAL_DISPLAY_PROCESSING
#define DBG_RX_SIGNAL_DISPLAY_SSID_MONITORED "jeff-ap"
*/
#define DBG_RX_SIGNAL_DISPLAY_RAW_DATA
#if 0
#define DBG_NOISE_MONITOR

#define DBG_SHOW_MCUFWDL_BEFORE_51_ENABLE
#define DBG_ROAMING_TEST

#define DBG_HAL_INIT_PROFILING

#define DBG_MEMORY_LEAK	1

/* TX use 1 urb */
#define CONFIG_SINGLE_XMIT_BUF
/* RX use 1 urb */
#define CONFIG_SINGLE_RECV_BUF
#endif

#define	DBG_TXBD_DESC_DUMP


/* #define CONFIG_NO_FW */
/* #define CONFIG_DISABLE_ODM */

/* #define CONFIG_8822CE_INT_MIGRATION */

#define CONFIG_PCI_TX_POLLING
/*#define CONFIG_PCI_TX_POLLING_V2*/

#define CONFIG_64BIT_DMA
