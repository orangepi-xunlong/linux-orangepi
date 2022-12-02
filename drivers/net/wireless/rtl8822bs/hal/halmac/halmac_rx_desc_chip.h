/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/

#ifndef _HALMAC_RX_DESC_CHIP_H_
#define _HALMAC_RX_DESC_CHIP_H_
#if (HALMAC_8814A_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8814A(__pRxDesc)    GET_RX_DESC_EOR(__pRxDesc)
#define GET_RX_DESC_PHYPKTIDC_8814A(__pRxDesc)    GET_RX_DESC_PHYPKTIDC(__pRxDesc)
#define GET_RX_DESC_SWDEC_8814A(__pRxDesc)    GET_RX_DESC_SWDEC(__pRxDesc)
#define GET_RX_DESC_PHYST_8814A(__pRxDesc)    GET_RX_DESC_PHYST(__pRxDesc)
#define GET_RX_DESC_SHIFT_8814A(__pRxDesc)    GET_RX_DESC_SHIFT(__pRxDesc)
#define GET_RX_DESC_QOS_8814A(__pRxDesc)    GET_RX_DESC_QOS(__pRxDesc)
#define GET_RX_DESC_SECURITY_8814A(__pRxDesc)    GET_RX_DESC_SECURITY(__pRxDesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8814A(__pRxDesc)    GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)
#define GET_RX_DESC_ICV_ERR_8814A(__pRxDesc)    GET_RX_DESC_ICV_ERR(__pRxDesc)
#define GET_RX_DESC_CRC32_8814A(__pRxDesc)    GET_RX_DESC_CRC32(__pRxDesc)
#define GET_RX_DESC_PKT_LEN_8814A(__pRxDesc)    GET_RX_DESC_PKT_LEN(__pRxDesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8814A(__pRxDesc)    GET_RX_DESC_BC(__pRxDesc)
#define GET_RX_DESC_MC_8814A(__pRxDesc)    GET_RX_DESC_MC(__pRxDesc)
#define GET_RX_DESC_TY_PE_8814A(__pRxDesc)    GET_RX_DESC_TY_PE(__pRxDesc)
#define GET_RX_DESC_MF_8814A(__pRxDesc)    GET_RX_DESC_MF(__pRxDesc)
#define GET_RX_DESC_MD_8814A(__pRxDesc)    GET_RX_DESC_MD(__pRxDesc)
#define GET_RX_DESC_PWR_8814A(__pRxDesc)    GET_RX_DESC_PWR(__pRxDesc)
#define GET_RX_DESC_PAM_8814A(__pRxDesc)    GET_RX_DESC_PAM(__pRxDesc)
#define GET_RX_DESC_CHK_VLD_8814A(__pRxDesc)    GET_RX_DESC_CHK_VLD(__pRxDesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8814A(__pRxDesc)    GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)
#define GET_RX_DESC_RX_IPV_8814A(__pRxDesc)    GET_RX_DESC_RX_IPV(__pRxDesc)
#define GET_RX_DESC_CHKERR_8814A(__pRxDesc)    GET_RX_DESC_CHKERR(__pRxDesc)
#define GET_RX_DESC_PAGGR_8814A(__pRxDesc)    GET_RX_DESC_PAGGR(__pRxDesc)
#define GET_RX_DESC_RXID_MATCH_8814A(__pRxDesc)    GET_RX_DESC_RXID_MATCH(__pRxDesc)
#define GET_RX_DESC_AMSDU_8814A(__pRxDesc)    GET_RX_DESC_AMSDU(__pRxDesc)
#define GET_RX_DESC_MACID_VLD_8814A(__pRxDesc)    GET_RX_DESC_MACID_VLD(__pRxDesc)
#define GET_RX_DESC_TID_8814A(__pRxDesc)    GET_RX_DESC_TID(__pRxDesc)
#define GET_RX_DESC_MACID_8814A(__pRxDesc)    GET_RX_DESC_MACID(__pRxDesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8814A(__pRxDesc)    GET_RX_DESC_FCS_OK(__pRxDesc)
#define GET_RX_DESC_C2H_8814A(__pRxDesc)    GET_RX_DESC_C2H(__pRxDesc)
#define GET_RX_DESC_HWRSVD_8814A(__pRxDesc)    GET_RX_DESC_HWRSVD(__pRxDesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8814A(__pRxDesc)    GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)
#define GET_RX_DESC_RX_IS_QOS_8814A(__pRxDesc)    GET_RX_DESC_RX_IS_QOS(__pRxDesc)
#define GET_RX_DESC_FRAG_8814A(__pRxDesc)    GET_RX_DESC_FRAG(__pRxDesc)
#define GET_RX_DESC_SEQ_8814A(__pRxDesc)    GET_RX_DESC_SEQ(__pRxDesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8814A(__pRxDesc)    GET_RX_DESC_MAGIC_WAKE(__pRxDesc)
#define GET_RX_DESC_UNICAST_WAKE_8814A(__pRxDesc)    GET_RX_DESC_UNICAST_WAKE(__pRxDesc)
#define GET_RX_DESC_PATTERN_MATCH_8814A(__pRxDesc)    GET_RX_DESC_PATTERN_MATCH(__pRxDesc)
#define GET_RX_DESC_DMA_AGG_NUM_8814A(__pRxDesc)    GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8814A(__pRxDesc)    GET_RX_DESC_BSSID_FIT_1_0(__pRxDesc)
#define GET_RX_DESC_EOSP_8814A(__pRxDesc)    GET_RX_DESC_EOSP(__pRxDesc)
#define GET_RX_DESC_HTC_8814A(__pRxDesc)    GET_RX_DESC_HTC(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8814A(__pRxDesc)    GET_RX_DESC_BSSID_FIT_4_2(__pRxDesc)
#define GET_RX_DESC_RX_RATE_8814A(__pRxDesc)    GET_RX_DESC_RX_RATE(__pRxDesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8814A(__pRxDesc)    GET_RX_DESC_A1_FIT(__pRxDesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8814A(__pRxDesc)    GET_RX_DESC_MACID_RPT_BUFF(__pRxDesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8814A(__pRxDesc)    GET_RX_DESC_RX_PRE_NDP_VLD(__pRxDesc)
#define GET_RX_DESC_RX_SCRAMBLER_8814A(__pRxDesc)    GET_RX_DESC_RX_SCRAMBLER(__pRxDesc)
#define GET_RX_DESC_RX_EOF_8814A(__pRxDesc)    GET_RX_DESC_RX_EOF(__pRxDesc)
#define GET_RX_DESC_PATTERN_IDX_8814A(__pRxDesc)    GET_RX_DESC_PATTERN_IDX(__pRxDesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8814A(__pRxDesc)    GET_RX_DESC_TSFL(__pRxDesc)

#endif

#if (HALMAC_8822B_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8822B(__pRxDesc)    GET_RX_DESC_EOR(__pRxDesc)
#define GET_RX_DESC_PHYPKTIDC_8822B(__pRxDesc)    GET_RX_DESC_PHYPKTIDC(__pRxDesc)
#define GET_RX_DESC_SWDEC_8822B(__pRxDesc)    GET_RX_DESC_SWDEC(__pRxDesc)
#define GET_RX_DESC_PHYST_8822B(__pRxDesc)    GET_RX_DESC_PHYST(__pRxDesc)
#define GET_RX_DESC_SHIFT_8822B(__pRxDesc)    GET_RX_DESC_SHIFT(__pRxDesc)
#define GET_RX_DESC_QOS_8822B(__pRxDesc)    GET_RX_DESC_QOS(__pRxDesc)
#define GET_RX_DESC_SECURITY_8822B(__pRxDesc)    GET_RX_DESC_SECURITY(__pRxDesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8822B(__pRxDesc)    GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)
#define GET_RX_DESC_ICV_ERR_8822B(__pRxDesc)    GET_RX_DESC_ICV_ERR(__pRxDesc)
#define GET_RX_DESC_CRC32_8822B(__pRxDesc)    GET_RX_DESC_CRC32(__pRxDesc)
#define GET_RX_DESC_PKT_LEN_8822B(__pRxDesc)    GET_RX_DESC_PKT_LEN(__pRxDesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8822B(__pRxDesc)    GET_RX_DESC_BC(__pRxDesc)
#define GET_RX_DESC_MC_8822B(__pRxDesc)    GET_RX_DESC_MC(__pRxDesc)
#define GET_RX_DESC_TY_PE_8822B(__pRxDesc)    GET_RX_DESC_TY_PE(__pRxDesc)
#define GET_RX_DESC_MF_8822B(__pRxDesc)    GET_RX_DESC_MF(__pRxDesc)
#define GET_RX_DESC_MD_8822B(__pRxDesc)    GET_RX_DESC_MD(__pRxDesc)
#define GET_RX_DESC_PWR_8822B(__pRxDesc)    GET_RX_DESC_PWR(__pRxDesc)
#define GET_RX_DESC_PAM_8822B(__pRxDesc)    GET_RX_DESC_PAM(__pRxDesc)
#define GET_RX_DESC_CHK_VLD_8822B(__pRxDesc)    GET_RX_DESC_CHK_VLD(__pRxDesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8822B(__pRxDesc)    GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)
#define GET_RX_DESC_RX_IPV_8822B(__pRxDesc)    GET_RX_DESC_RX_IPV(__pRxDesc)
#define GET_RX_DESC_CHKERR_8822B(__pRxDesc)    GET_RX_DESC_CHKERR(__pRxDesc)
#define GET_RX_DESC_PAGGR_8822B(__pRxDesc)    GET_RX_DESC_PAGGR(__pRxDesc)
#define GET_RX_DESC_RXID_MATCH_8822B(__pRxDesc)    GET_RX_DESC_RXID_MATCH(__pRxDesc)
#define GET_RX_DESC_AMSDU_8822B(__pRxDesc)    GET_RX_DESC_AMSDU(__pRxDesc)
#define GET_RX_DESC_MACID_VLD_8822B(__pRxDesc)    GET_RX_DESC_MACID_VLD(__pRxDesc)
#define GET_RX_DESC_TID_8822B(__pRxDesc)    GET_RX_DESC_TID(__pRxDesc)
#define GET_RX_DESC_MACID_8822B(__pRxDesc)    GET_RX_DESC_MACID(__pRxDesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8822B(__pRxDesc)    GET_RX_DESC_FCS_OK(__pRxDesc)
#define GET_RX_DESC_PPDU_CNT_8822B(__pRxDesc)    GET_RX_DESC_PPDU_CNT(__pRxDesc)
#define GET_RX_DESC_C2H_8822B(__pRxDesc)    GET_RX_DESC_C2H(__pRxDesc)
#define GET_RX_DESC_HWRSVD_8822B(__pRxDesc)    GET_RX_DESC_HWRSVD(__pRxDesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8822B(__pRxDesc)    GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)
#define GET_RX_DESC_RX_IS_QOS_8822B(__pRxDesc)    GET_RX_DESC_RX_IS_QOS(__pRxDesc)
#define GET_RX_DESC_FRAG_8822B(__pRxDesc)    GET_RX_DESC_FRAG(__pRxDesc)
#define GET_RX_DESC_SEQ_8822B(__pRxDesc)    GET_RX_DESC_SEQ(__pRxDesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8822B(__pRxDesc)    GET_RX_DESC_MAGIC_WAKE(__pRxDesc)
#define GET_RX_DESC_UNICAST_WAKE_8822B(__pRxDesc)    GET_RX_DESC_UNICAST_WAKE(__pRxDesc)
#define GET_RX_DESC_PATTERN_MATCH_8822B(__pRxDesc)    GET_RX_DESC_PATTERN_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8822B(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8822B(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_ID(__pRxDesc)
#define GET_RX_DESC_DMA_AGG_NUM_8822B(__pRxDesc)    GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8822B(__pRxDesc)    GET_RX_DESC_BSSID_FIT_1_0(__pRxDesc)
#define GET_RX_DESC_EOSP_8822B(__pRxDesc)    GET_RX_DESC_EOSP(__pRxDesc)
#define GET_RX_DESC_HTC_8822B(__pRxDesc)    GET_RX_DESC_HTC(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8822B(__pRxDesc)    GET_RX_DESC_BSSID_FIT_4_2(__pRxDesc)
#define GET_RX_DESC_RX_RATE_8822B(__pRxDesc)    GET_RX_DESC_RX_RATE(__pRxDesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8822B(__pRxDesc)    GET_RX_DESC_A1_FIT(__pRxDesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8822B(__pRxDesc)    GET_RX_DESC_MACID_RPT_BUFF(__pRxDesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8822B(__pRxDesc)    GET_RX_DESC_RX_PRE_NDP_VLD(__pRxDesc)
#define GET_RX_DESC_RX_SCRAMBLER_8822B(__pRxDesc)    GET_RX_DESC_RX_SCRAMBLER(__pRxDesc)
#define GET_RX_DESC_RX_EOF_8822B(__pRxDesc)    GET_RX_DESC_RX_EOF(__pRxDesc)
#define GET_RX_DESC_PATTERN_IDX_8822B(__pRxDesc)    GET_RX_DESC_PATTERN_IDX(__pRxDesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8822B(__pRxDesc)    GET_RX_DESC_TSFL(__pRxDesc)

#endif

#if (HALMAC_8197F_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8197F(__pRxDesc)    GET_RX_DESC_EOR(__pRxDesc)
#define GET_RX_DESC_PHYPKTIDC_8197F(__pRxDesc)    GET_RX_DESC_PHYPKTIDC(__pRxDesc)
#define GET_RX_DESC_SWDEC_8197F(__pRxDesc)    GET_RX_DESC_SWDEC(__pRxDesc)
#define GET_RX_DESC_PHYST_8197F(__pRxDesc)    GET_RX_DESC_PHYST(__pRxDesc)
#define GET_RX_DESC_SHIFT_8197F(__pRxDesc)    GET_RX_DESC_SHIFT(__pRxDesc)
#define GET_RX_DESC_QOS_8197F(__pRxDesc)    GET_RX_DESC_QOS(__pRxDesc)
#define GET_RX_DESC_SECURITY_8197F(__pRxDesc)    GET_RX_DESC_SECURITY(__pRxDesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8197F(__pRxDesc)    GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)
#define GET_RX_DESC_ICV_ERR_8197F(__pRxDesc)    GET_RX_DESC_ICV_ERR(__pRxDesc)
#define GET_RX_DESC_CRC32_8197F(__pRxDesc)    GET_RX_DESC_CRC32(__pRxDesc)
#define GET_RX_DESC_PKT_LEN_8197F(__pRxDesc)    GET_RX_DESC_PKT_LEN(__pRxDesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8197F(__pRxDesc)    GET_RX_DESC_BC(__pRxDesc)
#define GET_RX_DESC_MC_8197F(__pRxDesc)    GET_RX_DESC_MC(__pRxDesc)
#define GET_RX_DESC_TY_PE_8197F(__pRxDesc)    GET_RX_DESC_TY_PE(__pRxDesc)
#define GET_RX_DESC_MF_8197F(__pRxDesc)    GET_RX_DESC_MF(__pRxDesc)
#define GET_RX_DESC_MD_8197F(__pRxDesc)    GET_RX_DESC_MD(__pRxDesc)
#define GET_RX_DESC_PWR_8197F(__pRxDesc)    GET_RX_DESC_PWR(__pRxDesc)
#define GET_RX_DESC_PAM_8197F(__pRxDesc)    GET_RX_DESC_PAM(__pRxDesc)
#define GET_RX_DESC_CHK_VLD_8197F(__pRxDesc)    GET_RX_DESC_CHK_VLD(__pRxDesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8197F(__pRxDesc)    GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)
#define GET_RX_DESC_RX_IPV_8197F(__pRxDesc)    GET_RX_DESC_RX_IPV(__pRxDesc)
#define GET_RX_DESC_CHKERR_8197F(__pRxDesc)    GET_RX_DESC_CHKERR(__pRxDesc)
#define GET_RX_DESC_PAGGR_8197F(__pRxDesc)    GET_RX_DESC_PAGGR(__pRxDesc)
#define GET_RX_DESC_RXID_MATCH_8197F(__pRxDesc)    GET_RX_DESC_RXID_MATCH(__pRxDesc)
#define GET_RX_DESC_AMSDU_8197F(__pRxDesc)    GET_RX_DESC_AMSDU(__pRxDesc)
#define GET_RX_DESC_MACID_VLD_8197F(__pRxDesc)    GET_RX_DESC_MACID_VLD(__pRxDesc)
#define GET_RX_DESC_TID_8197F(__pRxDesc)    GET_RX_DESC_TID(__pRxDesc)
#define GET_RX_DESC_MACID_8197F(__pRxDesc)    GET_RX_DESC_MACID(__pRxDesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8197F(__pRxDesc)    GET_RX_DESC_FCS_OK(__pRxDesc)
#define GET_RX_DESC_C2H_8197F(__pRxDesc)    GET_RX_DESC_C2H(__pRxDesc)
#define GET_RX_DESC_HWRSVD_8197F(__pRxDesc)    GET_RX_DESC_HWRSVD(__pRxDesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8197F(__pRxDesc)    GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)
#define GET_RX_DESC_RX_IS_QOS_8197F(__pRxDesc)    GET_RX_DESC_RX_IS_QOS(__pRxDesc)
#define GET_RX_DESC_FRAG_8197F(__pRxDesc)    GET_RX_DESC_FRAG(__pRxDesc)
#define GET_RX_DESC_SEQ_8197F(__pRxDesc)    GET_RX_DESC_SEQ(__pRxDesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8197F(__pRxDesc)    GET_RX_DESC_MAGIC_WAKE(__pRxDesc)
#define GET_RX_DESC_UNICAST_WAKE_8197F(__pRxDesc)    GET_RX_DESC_UNICAST_WAKE(__pRxDesc)
#define GET_RX_DESC_PATTERN_MATCH_8197F(__pRxDesc)    GET_RX_DESC_PATTERN_MATCH(__pRxDesc)
#define GET_RX_DESC_DMA_AGG_NUM_8197F(__pRxDesc)    GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8197F(__pRxDesc)    GET_RX_DESC_BSSID_FIT_1_0(__pRxDesc)
#define GET_RX_DESC_EOSP_8197F(__pRxDesc)    GET_RX_DESC_EOSP(__pRxDesc)
#define GET_RX_DESC_HTC_8197F(__pRxDesc)    GET_RX_DESC_HTC(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8197F(__pRxDesc)    GET_RX_DESC_BSSID_FIT_4_2(__pRxDesc)
#define GET_RX_DESC_RX_RATE_8197F(__pRxDesc)    GET_RX_DESC_RX_RATE(__pRxDesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8197F(__pRxDesc)    GET_RX_DESC_A1_FIT(__pRxDesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8197F(__pRxDesc)    GET_RX_DESC_MACID_RPT_BUFF(__pRxDesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8197F(__pRxDesc)    GET_RX_DESC_RX_PRE_NDP_VLD(__pRxDesc)
#define GET_RX_DESC_RX_SCRAMBLER_8197F(__pRxDesc)    GET_RX_DESC_RX_SCRAMBLER(__pRxDesc)
#define GET_RX_DESC_RX_EOF_8197F(__pRxDesc)    GET_RX_DESC_RX_EOF(__pRxDesc)
#define GET_RX_DESC_FC_POWER_8197F(__pRxDesc)    GET_RX_DESC_FC_POWER(__pRxDesc)
#define GET_RX_DESC_PATTERN_IDX_8197F(__pRxDesc)    GET_RX_DESC_PATTERN_IDX(__pRxDesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8197F(__pRxDesc)    GET_RX_DESC_TSFL(__pRxDesc)

#endif

#if (HALMAC_8821C_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8821C(__pRxDesc)    GET_RX_DESC_EOR(__pRxDesc)
#define GET_RX_DESC_PHYPKTIDC_8821C(__pRxDesc)    GET_RX_DESC_PHYPKTIDC(__pRxDesc)
#define GET_RX_DESC_SWDEC_8821C(__pRxDesc)    GET_RX_DESC_SWDEC(__pRxDesc)
#define GET_RX_DESC_PHYST_8821C(__pRxDesc)    GET_RX_DESC_PHYST(__pRxDesc)
#define GET_RX_DESC_SHIFT_8821C(__pRxDesc)    GET_RX_DESC_SHIFT(__pRxDesc)
#define GET_RX_DESC_QOS_8821C(__pRxDesc)    GET_RX_DESC_QOS(__pRxDesc)
#define GET_RX_DESC_SECURITY_8821C(__pRxDesc)    GET_RX_DESC_SECURITY(__pRxDesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8821C(__pRxDesc)    GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)
#define GET_RX_DESC_ICV_ERR_8821C(__pRxDesc)    GET_RX_DESC_ICV_ERR(__pRxDesc)
#define GET_RX_DESC_CRC32_8821C(__pRxDesc)    GET_RX_DESC_CRC32(__pRxDesc)
#define GET_RX_DESC_PKT_LEN_8821C(__pRxDesc)    GET_RX_DESC_PKT_LEN(__pRxDesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8821C(__pRxDesc)    GET_RX_DESC_BC(__pRxDesc)
#define GET_RX_DESC_MC_8821C(__pRxDesc)    GET_RX_DESC_MC(__pRxDesc)
#define GET_RX_DESC_TY_PE_8821C(__pRxDesc)    GET_RX_DESC_TY_PE(__pRxDesc)
#define GET_RX_DESC_MF_8821C(__pRxDesc)    GET_RX_DESC_MF(__pRxDesc)
#define GET_RX_DESC_MD_8821C(__pRxDesc)    GET_RX_DESC_MD(__pRxDesc)
#define GET_RX_DESC_PWR_8821C(__pRxDesc)    GET_RX_DESC_PWR(__pRxDesc)
#define GET_RX_DESC_PAM_8821C(__pRxDesc)    GET_RX_DESC_PAM(__pRxDesc)
#define GET_RX_DESC_CHK_VLD_8821C(__pRxDesc)    GET_RX_DESC_CHK_VLD(__pRxDesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8821C(__pRxDesc)    GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)
#define GET_RX_DESC_RX_IPV_8821C(__pRxDesc)    GET_RX_DESC_RX_IPV(__pRxDesc)
#define GET_RX_DESC_CHKERR_8821C(__pRxDesc)    GET_RX_DESC_CHKERR(__pRxDesc)
#define GET_RX_DESC_PAGGR_8821C(__pRxDesc)    GET_RX_DESC_PAGGR(__pRxDesc)
#define GET_RX_DESC_RXID_MATCH_8821C(__pRxDesc)    GET_RX_DESC_RXID_MATCH(__pRxDesc)
#define GET_RX_DESC_AMSDU_8821C(__pRxDesc)    GET_RX_DESC_AMSDU(__pRxDesc)
#define GET_RX_DESC_MACID_VLD_8821C(__pRxDesc)    GET_RX_DESC_MACID_VLD(__pRxDesc)
#define GET_RX_DESC_TID_8821C(__pRxDesc)    GET_RX_DESC_TID(__pRxDesc)
#define GET_RX_DESC_MACID_8821C(__pRxDesc)    GET_RX_DESC_MACID(__pRxDesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8821C(__pRxDesc)    GET_RX_DESC_FCS_OK(__pRxDesc)
#define GET_RX_DESC_PPDU_CNT_8821C(__pRxDesc)    GET_RX_DESC_PPDU_CNT(__pRxDesc)
#define GET_RX_DESC_C2H_8821C(__pRxDesc)    GET_RX_DESC_C2H(__pRxDesc)
#define GET_RX_DESC_HWRSVD_8821C(__pRxDesc)    GET_RX_DESC_HWRSVD(__pRxDesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8821C(__pRxDesc)    GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)
#define GET_RX_DESC_RX_IS_QOS_8821C(__pRxDesc)    GET_RX_DESC_RX_IS_QOS(__pRxDesc)
#define GET_RX_DESC_FRAG_8821C(__pRxDesc)    GET_RX_DESC_FRAG(__pRxDesc)
#define GET_RX_DESC_SEQ_8821C(__pRxDesc)    GET_RX_DESC_SEQ(__pRxDesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8821C(__pRxDesc)    GET_RX_DESC_MAGIC_WAKE(__pRxDesc)
#define GET_RX_DESC_UNICAST_WAKE_8821C(__pRxDesc)    GET_RX_DESC_UNICAST_WAKE(__pRxDesc)
#define GET_RX_DESC_PATTERN_MATCH_8821C(__pRxDesc)    GET_RX_DESC_PATTERN_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8821C(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8821C(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_ID(__pRxDesc)
#define GET_RX_DESC_DMA_AGG_NUM_8821C(__pRxDesc)    GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8821C(__pRxDesc)    GET_RX_DESC_BSSID_FIT_1_0(__pRxDesc)
#define GET_RX_DESC_EOSP_8821C(__pRxDesc)    GET_RX_DESC_EOSP(__pRxDesc)
#define GET_RX_DESC_HTC_8821C(__pRxDesc)    GET_RX_DESC_HTC(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8821C(__pRxDesc)    GET_RX_DESC_BSSID_FIT_4_2(__pRxDesc)
#define GET_RX_DESC_RX_RATE_8821C(__pRxDesc)    GET_RX_DESC_RX_RATE(__pRxDesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8821C(__pRxDesc)    GET_RX_DESC_A1_FIT(__pRxDesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8821C(__pRxDesc)    GET_RX_DESC_MACID_RPT_BUFF(__pRxDesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8821C(__pRxDesc)    GET_RX_DESC_RX_PRE_NDP_VLD(__pRxDesc)
#define GET_RX_DESC_RX_SCRAMBLER_8821C(__pRxDesc)    GET_RX_DESC_RX_SCRAMBLER(__pRxDesc)
#define GET_RX_DESC_RX_EOF_8821C(__pRxDesc)    GET_RX_DESC_RX_EOF(__pRxDesc)
#define GET_RX_DESC_PATTERN_IDX_8821C(__pRxDesc)    GET_RX_DESC_PATTERN_IDX(__pRxDesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8821C(__pRxDesc)    GET_RX_DESC_TSFL(__pRxDesc)

#endif

#if (HALMAC_8814B_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EVT_PKT_8814B(__pRxDesc)    GET_RX_DESC_EVT_PKT(__pRxDesc)
#define GET_RX_DESC_SWDEC_8814B(__pRxDesc)    GET_RX_DESC_SWDEC(__pRxDesc)
#define GET_RX_DESC_PHYST_8814B(__pRxDesc)    GET_RX_DESC_PHYST(__pRxDesc)
#define GET_RX_DESC_SHIFT_8814B(__pRxDesc)    GET_RX_DESC_SHIFT(__pRxDesc)
#define GET_RX_DESC_QOS_8814B(__pRxDesc)    GET_RX_DESC_QOS(__pRxDesc)
#define GET_RX_DESC_SECURITY_8814B(__pRxDesc)    GET_RX_DESC_SECURITY(__pRxDesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8814B(__pRxDesc)    GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)
#define GET_RX_DESC_ICV_ERR_8814B(__pRxDesc)    GET_RX_DESC_ICV_ERR(__pRxDesc)
#define GET_RX_DESC_CRC32_8814B(__pRxDesc)    GET_RX_DESC_CRC32(__pRxDesc)
#define GET_RX_DESC_PKT_LEN_8814B(__pRxDesc)    GET_RX_DESC_PKT_LEN(__pRxDesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8814B(__pRxDesc)    GET_RX_DESC_BC(__pRxDesc)
#define GET_RX_DESC_MC_8814B(__pRxDesc)    GET_RX_DESC_MC(__pRxDesc)
#define GET_RX_DESC_TYPE_8814B(__pRxDesc)    GET_RX_DESC_TYPE(__pRxDesc)
#define GET_RX_DESC_MF_8814B(__pRxDesc)    GET_RX_DESC_MF(__pRxDesc)
#define GET_RX_DESC_MD_8814B(__pRxDesc)    GET_RX_DESC_MD(__pRxDesc)
#define GET_RX_DESC_PWR_8814B(__pRxDesc)    GET_RX_DESC_PWR(__pRxDesc)
#define GET_RX_DESC_A1_MATCH_8814B(__pRxDesc)    GET_RX_DESC_A1_MATCH(__pRxDesc)
#define GET_RX_DESC_TCP_CHKSUM_VLD_8814B(__pRxDesc)    GET_RX_DESC_TCP_CHKSUM_VLD(__pRxDesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8814B(__pRxDesc)    GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)
#define GET_RX_DESC_RX_IPV_8814B(__pRxDesc)    GET_RX_DESC_RX_IPV(__pRxDesc)
#define GET_RX_DESC_TCP_CHKSUM_ERR_8814B(__pRxDesc)    GET_RX_DESC_TCP_CHKSUM_ERR(__pRxDesc)
#define GET_RX_DESC_PHY_PKT_IDC_8814B(__pRxDesc)    GET_RX_DESC_PHY_PKT_IDC(__pRxDesc)
#define GET_RX_DESC_FW_FIFO_FULL_8814B(__pRxDesc)    GET_RX_DESC_FW_FIFO_FULL(__pRxDesc)
#define GET_RX_DESC_AMPDU_8814B(__pRxDesc)    GET_RX_DESC_AMPDU(__pRxDesc)
#define GET_RX_DESC_RXCMD_IDC_8814B(__pRxDesc)    GET_RX_DESC_RXCMD_IDC(__pRxDesc)
#define GET_RX_DESC_AMSDU_8814B(__pRxDesc)    GET_RX_DESC_AMSDU(__pRxDesc)
#define GET_RX_DESC_MACID_VLD_8814B(__pRxDesc)    GET_RX_DESC_MACID_VLD(__pRxDesc)
#define GET_RX_DESC_TID_8814B(__pRxDesc)    GET_RX_DESC_TID(__pRxDesc)
#define GET_RX_DESC_MACID_8814B(__pRxDesc)    GET_RX_DESC_MACID(__pRxDesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_AMSDU_CUT_8814B(__pRxDesc)    GET_RX_DESC_AMSDU_CUT(__pRxDesc)
#define GET_RX_DESC_PPDU_CNT_8814B(__pRxDesc)    GET_RX_DESC_PPDU_CNT(__pRxDesc)
#define GET_RX_DESC_C2H_8814B(__pRxDesc)    GET_RX_DESC_C2H(__pRxDesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8814B(__pRxDesc)    GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)
#define GET_RX_DESC_LAST_MSDU_8814B(__pRxDesc)    GET_RX_DESC_LAST_MSDU(__pRxDesc)
#define GET_RX_DESC_EXT_SEC_TYPE_8814B(__pRxDesc)    GET_RX_DESC_EXT_SEC_TYPE(__pRxDesc)
#define GET_RX_DESC_FRAG_8814B(__pRxDesc)    GET_RX_DESC_FRAG(__pRxDesc)
#define GET_RX_DESC_SEQ_8814B(__pRxDesc)    GET_RX_DESC_SEQ(__pRxDesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8814B(__pRxDesc)    GET_RX_DESC_MAGIC_WAKE(__pRxDesc)
#define GET_RX_DESC_UNICAST_WAKE_8814B(__pRxDesc)    GET_RX_DESC_UNICAST_WAKE(__pRxDesc)
#define GET_RX_DESC_PATTERN_WAKE_8814B(__pRxDesc)    GET_RX_DESC_PATTERN_WAKE(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8814B(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8814B(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_ID(__pRxDesc)
#define GET_RX_DESC_DMA_AGG_NUM_8814B(__pRxDesc)    GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)
#define GET_RX_DESC_EOSP_8814B(__pRxDesc)    GET_RX_DESC_EOSP(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_8814B(__pRxDesc)    GET_RX_DESC_BSSID_FIT(__pRxDesc)
#define GET_RX_DESC_HTC_8814B(__pRxDesc)    GET_RX_DESC_HTC(__pRxDesc)
#define GET_RX_DESC_AMPDU_END_PKT_8814B(__pRxDesc)    GET_RX_DESC_AMPDU_END_PKT(__pRxDesc)
#define GET_RX_DESC_ADDRESS_CAM_VLD_8814B(__pRxDesc)    GET_RX_DESC_ADDRESS_CAM_VLD(__pRxDesc)
#define GET_RX_DESC_RX_RATE_8814B(__pRxDesc)    GET_RX_DESC_RX_RATE(__pRxDesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_ADDRESS_CAM_8814B(__pRxDesc)    GET_RX_DESC_ADDRESS_CAM(__pRxDesc)
#define GET_RX_DESC_SWPS_RPT_8814B(__pRxDesc)    GET_RX_DESC_SWPS_RPT(__pRxDesc)
#define GET_RX_DESC_PATTERN_IDX_8814B(__pRxDesc)    GET_RX_DESC_PATTERN_IDX(__pRxDesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_FREERUN_CNT_8814B(__pRxDesc)    GET_RX_DESC_FREERUN_CNT(__pRxDesc)

#endif

#if (HALMAC_8198F_SUPPORT)

/*RXDESC_WORD0*/

#define GET_RX_DESC_EOR_8198F(__pRxDesc)    GET_RX_DESC_EOR(__pRxDesc)
#define GET_RX_DESC_PHYPKTIDC_8198F(__pRxDesc)    GET_RX_DESC_PHYPKTIDC(__pRxDesc)
#define GET_RX_DESC_SWDEC_8198F(__pRxDesc)    GET_RX_DESC_SWDEC(__pRxDesc)
#define GET_RX_DESC_PHYST_8198F(__pRxDesc)    GET_RX_DESC_PHYST(__pRxDesc)
#define GET_RX_DESC_SHIFT_8198F(__pRxDesc)    GET_RX_DESC_SHIFT(__pRxDesc)
#define GET_RX_DESC_QOS_8198F(__pRxDesc)    GET_RX_DESC_QOS(__pRxDesc)
#define GET_RX_DESC_SECURITY_8198F(__pRxDesc)    GET_RX_DESC_SECURITY(__pRxDesc)
#define GET_RX_DESC_DRV_INFO_SIZE_8198F(__pRxDesc)    GET_RX_DESC_DRV_INFO_SIZE(__pRxDesc)
#define GET_RX_DESC_ICV_ERR_8198F(__pRxDesc)    GET_RX_DESC_ICV_ERR(__pRxDesc)
#define GET_RX_DESC_CRC32_8198F(__pRxDesc)    GET_RX_DESC_CRC32(__pRxDesc)
#define GET_RX_DESC_PKT_LEN_8198F(__pRxDesc)    GET_RX_DESC_PKT_LEN(__pRxDesc)

/*RXDESC_WORD1*/

#define GET_RX_DESC_BC_8198F(__pRxDesc)    GET_RX_DESC_BC(__pRxDesc)
#define GET_RX_DESC_MC_8198F(__pRxDesc)    GET_RX_DESC_MC(__pRxDesc)
#define GET_RX_DESC_TY_PE_8198F(__pRxDesc)    GET_RX_DESC_TY_PE(__pRxDesc)
#define GET_RX_DESC_MF_8198F(__pRxDesc)    GET_RX_DESC_MF(__pRxDesc)
#define GET_RX_DESC_MD_8198F(__pRxDesc)    GET_RX_DESC_MD(__pRxDesc)
#define GET_RX_DESC_PWR_8198F(__pRxDesc)    GET_RX_DESC_PWR(__pRxDesc)
#define GET_RX_DESC_PAM_8198F(__pRxDesc)    GET_RX_DESC_PAM(__pRxDesc)
#define GET_RX_DESC_CHK_VLD_8198F(__pRxDesc)    GET_RX_DESC_CHK_VLD(__pRxDesc)
#define GET_RX_DESC_RX_IS_TCP_UDP_8198F(__pRxDesc)    GET_RX_DESC_RX_IS_TCP_UDP(__pRxDesc)
#define GET_RX_DESC_RX_IPV_8198F(__pRxDesc)    GET_RX_DESC_RX_IPV(__pRxDesc)
#define GET_RX_DESC_CHKERR_8198F(__pRxDesc)    GET_RX_DESC_CHKERR(__pRxDesc)
#define GET_RX_DESC_PAGGR_8198F(__pRxDesc)    GET_RX_DESC_PAGGR(__pRxDesc)
#define GET_RX_DESC_RXID_MATCH_8198F(__pRxDesc)    GET_RX_DESC_RXID_MATCH(__pRxDesc)
#define GET_RX_DESC_AMSDU_8198F(__pRxDesc)    GET_RX_DESC_AMSDU(__pRxDesc)
#define GET_RX_DESC_MACID_VLD_8198F(__pRxDesc)    GET_RX_DESC_MACID_VLD(__pRxDesc)
#define GET_RX_DESC_TID_8198F(__pRxDesc)    GET_RX_DESC_TID(__pRxDesc)
#define GET_RX_DESC_MACID_8198F(__pRxDesc)    GET_RX_DESC_MACID(__pRxDesc)

/*RXDESC_WORD2*/

#define GET_RX_DESC_FCS_OK_8198F(__pRxDesc)    GET_RX_DESC_FCS_OK(__pRxDesc)
#define GET_RX_DESC_PPDU_CNT_8198F(__pRxDesc)    GET_RX_DESC_PPDU_CNT(__pRxDesc)
#define GET_RX_DESC_C2H_8198F(__pRxDesc)    GET_RX_DESC_C2H(__pRxDesc)
#define GET_RX_DESC_HWRSVD_8198F(__pRxDesc)    GET_RX_DESC_HWRSVD(__pRxDesc)
#define GET_RX_DESC_RXMAGPKT_8198F(__pRxDesc)    GET_RX_DESC_RXMAGPKT(__pRxDesc)
#define GET_RX_DESC_WLANHD_IV_LEN_8198F(__pRxDesc)    GET_RX_DESC_WLANHD_IV_LEN(__pRxDesc)
#define GET_RX_DESC_RX_IS_QOS_8198F(__pRxDesc)    GET_RX_DESC_RX_IS_QOS(__pRxDesc)
#define GET_RX_DESC_FRAG_8198F(__pRxDesc)    GET_RX_DESC_FRAG(__pRxDesc)
#define GET_RX_DESC_SEQ_8198F(__pRxDesc)    GET_RX_DESC_SEQ(__pRxDesc)

/*RXDESC_WORD3*/

#define GET_RX_DESC_MAGIC_WAKE_8198F(__pRxDesc)    GET_RX_DESC_MAGIC_WAKE(__pRxDesc)
#define GET_RX_DESC_UNICAST_WAKE_8198F(__pRxDesc)    GET_RX_DESC_UNICAST_WAKE(__pRxDesc)
#define GET_RX_DESC_PATTERN_MATCH_8198F(__pRxDesc)    GET_RX_DESC_PATTERN_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_MATCH_8198F(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_MATCH(__pRxDesc)
#define GET_RX_DESC_RXPAYLOAD_ID_8198F(__pRxDesc)    GET_RX_DESC_RXPAYLOAD_ID(__pRxDesc)
#define GET_RX_DESC_DMA_AGG_NUM_8198F(__pRxDesc)    GET_RX_DESC_DMA_AGG_NUM(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_1_0_8198F(__pRxDesc)    GET_RX_DESC_BSSID_FIT_1_0(__pRxDesc)
#define GET_RX_DESC_EOSP_8198F(__pRxDesc)    GET_RX_DESC_EOSP(__pRxDesc)
#define GET_RX_DESC_HTC_8198F(__pRxDesc)    GET_RX_DESC_HTC(__pRxDesc)
#define GET_RX_DESC_BSSID_FIT_4_2_8198F(__pRxDesc)    GET_RX_DESC_BSSID_FIT_4_2(__pRxDesc)
#define GET_RX_DESC_RX_RATE_8198F(__pRxDesc)    GET_RX_DESC_RX_RATE(__pRxDesc)

/*RXDESC_WORD4*/

#define GET_RX_DESC_A1_FIT_8198F(__pRxDesc)    GET_RX_DESC_A1_FIT(__pRxDesc)
#define GET_RX_DESC_MACID_RPT_BUFF_8198F(__pRxDesc)    GET_RX_DESC_MACID_RPT_BUFF(__pRxDesc)
#define GET_RX_DESC_RX_PRE_NDP_VLD_8198F(__pRxDesc)    GET_RX_DESC_RX_PRE_NDP_VLD(__pRxDesc)
#define GET_RX_DESC_RX_SCRAMBLER_8198F(__pRxDesc)    GET_RX_DESC_RX_SCRAMBLER(__pRxDesc)
#define GET_RX_DESC_RX_EOF_8198F(__pRxDesc)    GET_RX_DESC_RX_EOF(__pRxDesc)
#define GET_RX_DESC_FC_POWER_8198F(__pRxDesc)    GET_RX_DESC_FC_POWER(__pRxDesc)
#define GET_RX_DESC_TXRPTMID_CTL_MASK_8198F(__pRxDesc)    GET_RX_DESC_TXRPTMID_CTL_MASK(__pRxDesc)
#define GET_RX_DESC_SWPS_RPT_8198F(__pRxDesc)    GET_RX_DESC_SWPS_RPT(__pRxDesc)
#define GET_RX_DESC_PATTERN_IDX_8198F(__pRxDesc)    GET_RX_DESC_PATTERN_IDX(__pRxDesc)

/*RXDESC_WORD5*/

#define GET_RX_DESC_TSFL_8198F(__pRxDesc)    GET_RX_DESC_TSFL(__pRxDesc)

#endif


#endif

