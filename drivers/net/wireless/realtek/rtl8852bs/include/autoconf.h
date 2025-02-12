/******************************************************************************
 *
 * Copyright(c) 2019 - 2021 Realtek Corporation.
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
#ifndef _AUTOCONF_H_
#define _AUTOCONF_H_

/***** temporarily flag *******/
/*#define CONFIG_NO_FW*/
/*#define CONFIG_DISABLE_ODM*/

/*
 * Debug Related Config
 */
#ifdef CONFIG_RTW_DEBUG
	#define DBG	1	/* for ODM & BTC debug */
#else /* !CONFIG_RTW_DEBUG */
	#define DBG	0	/* for ODM & BTC debug */
#endif /* CONFIG_RTW_DEBUG */


/*#define DBG_CONFIG_ERROR_DETECT*/
/*#define DBG_XMIT_BUF*/
/*#define DBG_XMIT_BUF_EXT*/
/*#define CONFIG_FW_C2H_DEBUG*/

/* debug memory management */
#if 0
	#define DBG_MEM_ALLOC
	#define DBG_PHL_MEM_ALLOC
	#define DBG_HAL_MAC_MEM_MOINTOR
	#define DBG_HAL_MEM_MOINTOR
#endif
/*#define CONFIG_PHL_USE_KMEM_ALLOC*/

/* debug TX/RX */
/*#define DBG_XMIT_BLOCK*/
/*#define DBG_RX_COUNTER_DUMP*/

/*#define DBG_C2H_MAC_HIDDEN_RPT_HANDLE	1*/
/*#define DBG_H2C_CONTENT*/

#define CONFIG_ERROR_STATE_MONITOR
/*#define CONFIG_MONITOR_OVERFLOW*/

/*#define CONFIG_SDIO_INDIRECT_ACCESS*/
/*#define DBG_SDIO_INDIRECT_ACCESS*/


/*
 * Work around Config
 */
#define RTW_WKARD_DIS_PROBE_REQ_RPT_TO_HOSTAPD

#define RTW_WKARD_CORE_RSSI_V1
#ifdef RTW_WKARD_CORE_RSSI_V1
#define CONFIG_RX_PSTS_PER_PKT
#define CONFIG_SIGNAL_STAT_PROCESS
/*#define DBG_RX_SIGNAL_DISPLAY_PROCESSING*/
#endif

#ifdef CONFIG_BTC
#define RTK_WKARD_CORE_BTC_STBC_CAP
#endif

/*
 * Public General Config
 */
#define AUTOCONF_INCLUDED
#define DRV_NAME "rtl8852bs"

#define CONFIG_SDIO_HCI


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

#define CONFIG_TX_AMSDU
#ifdef CONFIG_TX_AMSDU
	#define CONFIG_TX_AMSDU_SW_MODE
#endif

#define CONFIG_BEAMFORMING
#ifdef CONFIG_BEAMFORMING
/*#define RTW_WKARD_TX_DISABLE_BFEE*/
#endif

/* Set CONFIG_IOCTL_CFG80211 from Makefile */
#ifdef CONFIG_IOCTL_CFG80211
	/*
	 * Indecate new sta asoc through cfg80211_new_sta
	 * If kernel version >= 3.2 or
	 * version < 3.2 but already apply cfg80211 patch,
	 * RTW_USE_CFG80211_STA_EVENT must be defiend!
	 */
	/* Set RTW_USE_CFG80211_STA_EVENT from Makefile */
	#define CONFIG_CFG80211_FORCE_COMPATIBLE_2_6_37_UNDER
	/* #define CONFIG_DEBUG_CFG80211 */
	#define CONFIG_SET_SCAN_DENY_TIMER
#endif /* CONFIG_IOCTL_CFG80211 */

#define CONFIG_AP_MODE
#ifdef CONFIG_AP_MODE
	#define CONFIG_NATIVEAP_MLME
	#ifndef CONFIG_NATIVEAP_MLME
		#define CONFIG_HOSTAPD_MLME
	#endif
	/*#define CONFIG_RTW_HOSTAPD_ACS*/
	/*#define CONFIG_FIND_BEST_CHANNEL*/
#endif


#define CONFIG_P2P

#ifdef CONFIG_P2P
	#define CONFIG_WFD	/* Wi-Fi display */
	#define CONFIG_P2P_REMOVE_GROUP_INFO
	/*#define CONFIG_DBG_P2P*/
	#define CONFIG_P2P_PS
	/*#define CONFIG_P2P_IPS*/
	#define CONFIG_P2P_OP_CHK_SOCIAL_CH
	#define CONFIG_CFG80211_ONECHANNEL_UNDER_CONCURRENT  /* Replace CONFIG_P2P_CHK_INVITE_CH_LIST flag */
	/*#define CONFIG_P2P_INVITE_IOT*/
#endif /* CONFIG_P2P */

/* Set CONFIG_TDLS from Makefile */
#ifdef CONFIG_TDLS
	#define CONFIG_TDLS_DRIVER_SETUP
/*	#ifndef CONFIG_WFD
		#define CONFIG_WFD
	#endif*/
/*	#define CONFIG_TDLS_AUTOSETUP*/
	#define CONFIG_TDLS_AUTOCHECKALIVE
	/*
	 * Enable "CONFIG_TDLS_CH_SW" by default,
	 * however limit it to only work in wifi logo test mode
	 * but not in normal mode currently
	 */
	/* #define CONFIG_TDLS_CH_SW */ /* Not support yet */
#endif /* CONFIG_TDLS */


/*#define CONFIG_RTW_80211K*/


#define CONFIG_HW_RTS

/*
 * Hareware/Firmware Related Config
 */
/* Set CONFIG_ANTENNA_DIVERSITY from Makefile */

/*#define CONFIG_RTW_LED*/
#ifdef CONFIG_RTW_LED
	/*#define CONFIG_RTW_SW_LED*/
#endif /* CONFIG_RTW_LED */

#define DISABLE_BB_RF		0

#define CONFIG_TCP_CSUM_OFFLOAD_RX

/*
 * Interface Related Config
 */
/*#define CONFIG_SDIO_TX_ENABLE_AVAL_INT => Related MAC reg must setting => HAL-MAC ?? */
#define CONFIG_SDIO_RX_COPY

#define CONFIG_SDIO_RX_NETBUF_ALLOC_IN_PHL
#define CONFIG_SDIO_READ_RXFF_IN_INT


/*#define RTW_XMIT_THREAD_HIGH_PRIORITY*/
/*#define RTW_XMIT_THREAD_CB_HIGH_PRIORITY*/
/*#define RTW_RECV_THREAD_HIGH_PRIORITY*/

#ifdef CONFIG_RTW_NAPI
/*#define CONFIG_RTW_NAPI_DYNAMIC*/
#define CONFIG_RTW_NAPI_V2
#ifdef CONFIG_RTW_NAPI_V2
#define CONFIG_RX_BATCH_IND
#endif
#endif

#define CONFIG_QUOTA_TURBO_ENABLE

#define MAX_XMITBUF_SZ	26624	/* 26 KB */
#define MAX_RECVBUF_SZ	68608	/* 68 KB*/
#define MAX_PHL_TX_RING_ENTRY_NUM 512
#define MAX_PHL_RX_RING_ENTRY_NUM 536
#define CONFIG_RTW_REDUCE_MEM
#define CONFIG_SCAN_BACKOP_STA
#define NR_XMITFRAME	MAX_PHL_TX_RING_ENTRY_NUM
#define MAX_TX_RING_NUM		MAX_PHL_TX_RING_ENTRY_NUM
#define RTW_MAX_FRAG_NUM 1
#define NR_XMITFRAME_EXT 120
#define RTW_MAX_FW_SIZE 0x80000


/*fw reduce code size*/
#define CONFIG_FW_SPECIFY_FROM_CORE
#ifdef CONFIG_FW_SPECIFY_FROM_CORE
#ifdef CONFIG_WOWLAN
#define MAC_FW_CATEGORY_WOWLAN
#endif /*CONFIG_WOWLAN*/
#define MAC_FW_8852B_U2
#define MAC_FW_8852B_U3

#define MAC_FW_CATEGORY_NIC  /*pwr gating*/
/*#define MAC_FW_CATEGORY_NICCE*/  /*clock gating*/
/*#define MAC_FW_CATEGORY_NIC_PLE*/
#endif /*CONFIG_FW_SPECIFY_FROM_CORE*/

/*
 * Others
 */
/*#define CONFIG_MAC_LOOPBACK_DRIVER*/
#define CONFIG_SKB_COPY		/* for amsdu */
#define CONFIG_EMBEDDED_FWIMG
/*#define CONFIG_FILE_FWIMG*/
#define CONFIG_LONG_DELAY_ISSUE
/*#define CONFIG_PATCH_JOIN_WRONG_CHANNEL*/


/*
 * Platform
 */


/*
 * Auto Config Section
 */
#ifdef CONFIG_MAC_LOOPBACK_DRIVER
#undef CONFIG_IOCTL_CFG80211
#undef CONFIG_AP_MODE
#undef CONFIG_NATIVEAP_MLME
#undef CONFIG_ANTENNA_DIVERSITY
#endif /* CONFIG_MAC_LOOPBACK_DRIVER */

#ifdef CONFIG_MP_INCLUDED
	#define MP_DRIVER	1
	/* disable unnecessary functions for MP*/
	/*#undef CONFIG_ANTENNA_DIVERSITY*/
#else /* !CONFIG_MP_INCLUDED */
	#define MP_DRIVER	0
#endif /* !CONFIG_MP_INCLUDED */

#ifdef CONFIG_POWER_SAVE
	/*#define CONFIG_RTW_IPS*/
	#define CONFIG_RTW_LPS
	#ifdef CONFIG_RTW_IPS
		#define CONFIG_FWIPS
	#endif
	#if defined(CONFIG_RTW_IPS) || defined(CONFIG_RTW_LPS)
		#define CONFIG_PS_FW_DBG
	#endif
	#ifdef CONFIG_RTW_LPS
	#define CONFIG_RTW_LPS_DEFAULT_OFF
	#endif
#endif /* CONFIG_POWER_SAVE */

#ifdef CONFIG_WOWLAN
	#define CONFIG_GTK_OL
	/* #define CONFIG_ARP_KEEP_ALIVE */
#endif /* CONFIG_WOWLAN */

#ifdef CONFIG_GPIO_WAKEUP
	#ifndef WAKEUP_GPIO_IDX
		#define WAKEUP_GPIO_IDX	10	/* WIFI Chip Side */
	#endif /*!WAKEUP_GPIO_IDX*/
#endif /* CONFIG_GPIO_WAKEUP */

#ifdef CONFIG_ANTENNA_DIVERSITY
#define CONFIG_HW_ANTENNA_DIVERSITY
#endif /* CONFIG_ANTENNA_DIVERSITY */


#define CONFIG_RTW_DISABLE_PHL_LOG
#define CONFIG_RTW_DEBUG_CCCR

#define CONFIG_MSG_NUM 100
#define SCAN_PER_CH_EX_TIME 350
#define RTW_MAX_SCHEDULE_TIMEOUT 4000 /*unit:ms*/

/*#define CONFIG_XMIT_ACK*/
#ifdef CONFIG_XMIT_ACK
	/*#define DBG_XMIT_ACK*/
	#define CONFIG_XMIT_ACK_BY_CCX_RPT
	#ifdef CONFIG_XMIT_ACK_BY_CCX_RPT
		#define RTW_WKARD_CCX_RPT_LIMIT_CTRL
		#define CONFIG_PHL_DEFAULT_MGNT_Q_RPT_EN
	#endif
	#define CONFIG_ACTIVE_KEEP_ALIVE_CHECK
	#define RTW_WKARD_TX_NULL_WD_RP
	#define RTW_MAX_MGMT_TX_MS_GAS 2000
#endif /* CONFIG_XMIT_ACK */

#define CONFIG_PHL_CPU_BALANCE_THREAD
#ifdef CONFIG_PHL_CPU_BALANCE_THREAD
#define CPU_ID_TX 2
#define CPU_ID_TX_CB 3
#define CPU_ID_RX_CB 1
/*#define DBG_THREAD_PID*/
/*#define DBG_CPU_INFO*/
#endif /*CONFIG_PHL_CPU_BALANCE_THREAD*/

#endif /* _AUTOCONF_H_ */
