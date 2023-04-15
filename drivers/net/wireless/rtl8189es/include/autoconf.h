/******************************************************************************
 *
 * Copyright(c) 2010 - 2017 Realtek Corporation.
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
/*
 * Automatically generated C config: don't edit
 */
/* ***** temporarily flag ******* */
#define CONFIG_SINGLE_IMG

/* ***** temporarily flag ******* */

/* ***** temporarily flag ******* */


#define AUTOCONF_INCLUDED
#define RTL871X_MODULE_NAME "8189ES"
#define DRV_NAME "rtl8189es"

#ifndef EFUSE_MAP_PATH
	#define EFUSE_MAP_PATH "/system/etc/wifi/wifi_efuse_8189e.map"
#endif

#define CONFIG_SDIO_HCI
#define PLATFORM_LINUX

/* #define CONFIG_IOCTL_CFG80211 */

#ifdef CONFIG_IOCTL_CFG80211
	/* #define RTW_USE_CFG80211_STA_EVENT */ /* Indecate new sta asoc through cfg80211_new_sta */
	#define CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER
	/* #define CONFIG_DEBUG_CFG80211 */
	#define CONFIG_SET_SCAN_DENY_TIMER
#endif

#define CONFIG_EMBEDDED_FWIMG

#ifdef CONFIG_EMBEDDED_FWIMG
	#define	LOAD_FW_HEADER_FROM_DRIVER
#endif
/* #define CONFIG_FILE_FWIMG */

#define CONFIG_XMIT_ACK
#ifdef CONFIG_XMIT_ACK
	#define CONFIG_ACTIVE_KEEP_ALIVE_CHECK
#endif
#define CONFIG_80211N_HT
#define CONFIG_RECV_REORDERING_CTRL

/* #define CONFIG_CONCURRENT_MODE */
#ifdef CONFIG_CONCURRENT_MODE
	#define CONFIG_TSF_RESET_OFFLOAD		/* For 2 PORT TSF SYNC. */
	#define CONFIG_RUNTIME_PORT_SWITCH

	/* #define DBG_RUNTIME_PORT_SWITCH */
#endif

#define CONFIG_AP_MODE
#ifdef CONFIG_AP_MODE
	#define CONFIG_INTERRUPT_BASED_TXBCN /* Tx Beacon when driver early interrupt occurs*/
	#if defined(CONFIG_CONCURRENT_MODE) && defined(CONFIG_INTERRUPT_BASED_TXBCN)
		#undef CONFIG_INTERRUPT_BASED_TXBCN
	#endif
	#ifdef CONFIG_INTERRUPT_BASED_TXBCN
		#define CONFIG_INTERRUPT_BASED_TXBCN_EARLY_INT
		/* #define CONFIG_INTERRUPT_BASED_TXBCN_BCN_OK_ERR */
	#endif

	#define CONFIG_NATIVEAP_MLME
	#ifndef CONFIG_NATIVEAP_MLME
		#define CONFIG_HOSTAPD_MLME
	#endif
	/* #define CONFIG_FIND_BEST_CHANNEL */
#endif

#define CONFIG_P2P
#ifdef CONFIG_P2P
	/* The CONFIG_WFD is for supporting the Wi-Fi display */
	#define CONFIG_WFD

	#define CONFIG_P2P_REMOVE_GROUP_INFO

	/* #define CONFIG_DBG_P2P */

	#define CONFIG_P2P_PS
	#define CONFIG_P2P_OP_CHK_SOCIAL_CH
	#define CONFIG_CFG80211_ONECHANNEL_UNDER_CONCURRENT  /* replace CONFIG_P2P_CHK_INVITE_CH_LIST flag */
	/*#define CONFIG_P2P_INVITE_IOT*/
#endif

/* Added by Kurt 20110511 */
#ifdef CONFIG_TDLS
	#define CONFIG_TDLS_DRIVER_SETUP
/*	#ifndef CONFIG_WFD */
/*		#define CONFIG_WFD */
/*	#endif */
/*	#define CONFIG_TDLS_AUTOSETUP */
	#define CONFIG_TDLS_AUTOCHECKALIVE
	/* #define CONFIG_TDLS_CH_SW */	/* Enable this flag only when we confirm that TDLS CH SW is supported in FW */
#endif

#define CONFIG_SKB_COPY	/* for amsdu */

#define CONFIG_LAYER2_ROAMING
#define CONFIG_LAYER2_ROAMING_RESUME

#define CONFIG_LONG_DELAY_ISSUE
#define CONFIG_NEW_SIGNAL_STAT_PROCESS
#define RTW_NOTCH_FILTER 0 /* 0:Disable, 1:Enable, */

/*
 * Hardware Related Config
 */

/* #define SUPPORT_HW_RFOFF_DETECTED */

/*#define CONFIG_RTW_LED*/
#ifdef CONFIG_RTW_LED
	/*#define CONFIG_RTW_SW_LED*/
#endif /* CONFIG_RTW_LED */
/*
 * Interface Related Config
 */
#define CONFIG_TX_AGGREGATION
/* #define CONFIG_SDIO_TX_TASKLET */
#define CONFIG_SDIO_RX_COPY
#define CONFIG_SDIO_TX_ENABLE_AVAL_INT
#define SDIO_FREE_XMIT_BUF_SEMA
/*
 * Others
 */
/* #define CONFIG_MAC_LOOPBACK_DRIVER */


/*
 * Auto Config Section
 */
#if defined(CONFIG_RTL8188E) && defined(CONFIG_SDIO_HCI)
#define CONFIG_RTL8188E_SDIO
#define CONFIG_XMIT_THREAD_MODE
#endif

#ifdef CONFIG_POWER_SAVING
#define CONFIG_IPS
#define CONFIG_LPS
#if defined(CONFIG_LPS) && defined(CONFIG_SDIO_HCI)
#define CONFIG_LPS_LCLK

#ifdef CONFIG_LPS_LCLK
#define LPS_RPWM_WAIT_MS 300

/* #define CONFIG_DETECT_CPWM_BY_POLLING */
/* #define CONFIG_LPS_RPWM_TIMER */

#if defined(CONFIG_LPS_RPWM_TIMER) || defined(CONFIG_DETECT_CPWM_BY_POLLING)
#define LPS_RPWM_WAIT_MS 300
#endif
/* #define CONFIG_LPS_LCLK_WD_TIMER */ /* Watch Dog timer in LPS LCLK */
#endif

#endif
#endif /*CONFIG_POWER_SAVING*/
#ifdef CONFIG_MAC_LOOPBACK_DRIVER
#undef CONFIG_AP_MODE
#undef CONFIG_NATIVEAP_MLME
#undef CONFIG_POWER_SAVING
#undef SUPPORT_HW_RFOFF_DETECTED
#endif

#ifdef CONFIG_MP_INCLUDED

	#define MP_DRIVER		1
	#define CONFIG_MP_IWPRIV_SUPPORT

	/* disable unnecessary functions for MP */
	/* #undef CONFIG_IPS */
	/* #undef CONFIG_LPS */
	/* #undef CONFIG_LPS_LCLK */
	/* #undef SUPPORT_HW_RFOFF_DETECTED */

#else/*  #ifdef CONFIG_MP_INCLUDED */

	#define MP_DRIVER		0

#endif /*  #ifdef CONFIG_MP_INCLUDED */

#define CONFIG_IOL
#ifdef CONFIG_IOL
	#define CONFIG_IOL_NEW_GENERATION
	#define CONFIG_IOL_READ_EFUSE_MAP
	/* #define DBG_IOL_READ_EFUSE_MAP */
	/* #define CONFIG_IOL_LLT */
	#define CONFIG_IOL_EFUSE_PATCH
	/* #define CONFIG_IOL_IOREG_CFG */
	/* #define CONFIG_IOL_IOREG_CFG_DBG */
#endif

#ifdef CONFIG_WOWLAN
/* #define CONFIG_ARP_KEEP_ALIVE */
#endif


/* #define	CONFIG_TX_EARLY_MODE */
#ifdef CONFIG_TX_EARLY_MODE
#define	RTL8188E_EARLY_MODE_PKT_NUM_10	0
#endif
/* #endif */

/*
 * HAL	Related Config
 */

/* for FPGA VERIFICATION config */
#define RTL8188E_FPGA_TRUE_PHY_VERIFICATION 0

#define DISABLE_BB_RF	0

#define CONFIG_RF_POWER_TRIM

#ifdef CONFIG_GPIO_WAKEUP
	#ifndef WAKEUP_GPIO_IDX
	    #define WAKEUP_GPIO_IDX 7
	#endif
#endif

#define CONFIG_GPIO_API

/*
 * Debug Related Config
 */
#define DBG	1

#define DBG_CONFIG_ERROR_DETECT
#define DBG_CONFIG_ERROR_RESET

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
/*#define DBG_IP_R_MONITOR*//*Android IpReachabilityMonitor*/


/* #define DOWNLOAD_FW_TO_TXPKT_BUF 0 */

/* #define DBG_HAL_INIT_PROFILING */
#define DBG_RX_DFRAME_RAW_DATA
/*#define CONFIG_RTW_80211R*/
/*#define CONFIG_RTW_WNM*/
/*#define CONFIG_RTW_BTM_ROAM*/