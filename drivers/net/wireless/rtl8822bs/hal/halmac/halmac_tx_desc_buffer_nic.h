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

#ifndef _HALMAC_TX_DESC_BUFFER_NIC_H_
#define _HALMAC_TX_DESC_BUFFER_NIC_H_
#if (HALMAC_8814B_SUPPORT)

/*TXDESC_WORD0*/

#define SET_TX_DESC_BUFFER_RDG_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x00, 31, 1, __Value)
#define GET_TX_DESC_BUFFER_RDG_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x00, 31, 1)
#define SET_TX_DESC_BUFFER_BCNPKT_TSF_CTRL(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x00, 30, 1, __Value)
#define GET_TX_DESC_BUFFER_BCNPKT_TSF_CTRL(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x00, 30, 1)
#define SET_TX_DESC_BUFFER_AGG_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x00, 29, 1, __Value)
#define GET_TX_DESC_BUFFER_AGG_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x00, 29, 1)
#define SET_TX_DESC_BUFFER_PKT_OFFSET(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x00, 24, 5, __Value)
#define GET_TX_DESC_BUFFER_PKT_OFFSET(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x00, 24, 5)
#define SET_TX_DESC_BUFFER_OFFSET(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x00, 16, 8, __Value)
#define GET_TX_DESC_BUFFER_OFFSET(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x00, 16, 8)
#define SET_TX_DESC_BUFFER_TXPKTSIZE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x00, 0, 16, __Value)
#define GET_TX_DESC_BUFFER_TXPKTSIZE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x00, 0, 16)

/*TXDESC_WORD1*/

#define SET_TX_DESC_BUFFER_USERATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 31, 1, __Value)
#define GET_TX_DESC_BUFFER_USERATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 31, 1)
#define SET_TX_DESC_BUFFER_AMSDU(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 30, 1, __Value)
#define GET_TX_DESC_BUFFER_AMSDU(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 30, 1)
#define SET_TX_DESC_BUFFER_EN_HWSEQ(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 29, 1, __Value)
#define GET_TX_DESC_BUFFER_EN_HWSEQ(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 29, 1)
#define SET_TX_DESC_BUFFER_EN_HWEXSEQ(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 28, 1, __Value)
#define GET_TX_DESC_BUFFER_EN_HWEXSEQ(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 28, 1)
#define SET_TX_DESC_BUFFER_SW_SEQ(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 16, 12, __Value)
#define GET_TX_DESC_BUFFER_SW_SEQ(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 16, 12)
#define SET_TX_DESC_BUFFER_DROP_ID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 14, 2, __Value)
#define GET_TX_DESC_BUFFER_DROP_ID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 14, 2)
#define SET_TX_DESC_BUFFER_MOREDATA(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 13, 1, __Value)
#define GET_TX_DESC_BUFFER_MOREDATA(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 13, 1)
#define SET_TX_DESC_BUFFER_QSEL(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 8, 5, __Value)
#define GET_TX_DESC_BUFFER_QSEL(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 8, 5)
#define SET_TX_DESC_BUFFER_MACID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x04, 0, 8, __Value)
#define GET_TX_DESC_BUFFER_MACID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x04, 0, 8)

/*TXDESC_WORD2*/

#define SET_TX_DESC_BUFFER_CHK_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 31, 1, __Value)
#define GET_TX_DESC_BUFFER_CHK_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 31, 1)
#define SET_TX_DESC_BUFFER_DISQSELSEQ(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 30, 1, __Value)
#define GET_TX_DESC_BUFFER_DISQSELSEQ(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 30, 1)
#define SET_TX_DESC_BUFFER_SND_PKT_SEL(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 28, 2, __Value)
#define GET_TX_DESC_BUFFER_SND_PKT_SEL(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 28, 2)
#define SET_TX_DESC_BUFFER_DMA_PRI(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 26, 1, __Value)
#define GET_TX_DESC_BUFFER_DMA_PRI(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 26, 1)
#define SET_TX_DESC_BUFFER_MAX_AMSDU_MODE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 24, 2, __Value)
#define GET_TX_DESC_BUFFER_MAX_AMSDU_MODE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 24, 2)
#define SET_TX_DESC_BUFFER_DMA_TXAGG_NUM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 16, 8, __Value)
#define GET_TX_DESC_BUFFER_DMA_TXAGG_NUM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 16, 8)
#define SET_TX_DESC_BUFFER_TXDESC_CHECKSUM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x08, 0, 16, __Value)
#define GET_TX_DESC_BUFFER_TXDESC_CHECKSUM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x08, 0, 16)

/*TXDESC_WORD3*/

#define SET_TX_DESC_BUFFER_OFFLOAD_SIZE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x0C, 16, 15, __Value)
#define GET_TX_DESC_BUFFER_OFFLOAD_SIZE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x0C, 16, 15)
#define SET_TX_DESC_BUFFER_CHANNEL_DMA(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x0C, 11, 5, __Value)
#define GET_TX_DESC_BUFFER_CHANNEL_DMA(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x0C, 11, 5)
#define SET_TX_DESC_BUFFER_MBSSID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x0C, 7, 4, __Value)
#define GET_TX_DESC_BUFFER_MBSSID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x0C, 7, 4)
#define SET_TX_DESC_BUFFER_BK(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x0C, 6, 1, __Value)
#define GET_TX_DESC_BUFFER_BK(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x0C, 6, 1)
#define SET_TX_DESC_BUFFER_WHEADER_LEN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x0C, 0, 5, __Value)
#define GET_TX_DESC_BUFFER_WHEADER_LEN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x0C, 0, 5)

/*TXDESC_WORD4*/

#define SET_TX_DESC_BUFFER_TRY_RATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x10, 26, 1, __Value)
#define GET_TX_DESC_BUFFER_TRY_RATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x10, 26, 1)
#define SET_TX_DESC_BUFFER_DATA_BW(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x10, 24, 2, __Value)
#define GET_TX_DESC_BUFFER_DATA_BW(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x10, 24, 2)
#define SET_TX_DESC_BUFFER_DATA_SHORT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x10, 23, 1, __Value)
#define GET_TX_DESC_BUFFER_DATA_SHORT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x10, 23, 1)
#define SET_TX_DESC_BUFFER_DATARATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x10, 16, 7, __Value)
#define GET_TX_DESC_BUFFER_DATARATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x10, 16, 7)
#define SET_TX_DESC_BUFFER_TXBF_PATH(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x10, 11, 1, __Value)
#define GET_TX_DESC_BUFFER_TXBF_PATH(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x10, 11, 1)
#define SET_TX_DESC_BUFFER_GROUP_BIT_IE_OFFSET(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x10, 0, 11, __Value)
#define GET_TX_DESC_BUFFER_GROUP_BIT_IE_OFFSET(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x10, 0, 11)

/*TXDESC_WORD5*/

#define SET_TX_DESC_BUFFER_RTY_LMT_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 31, 1, __Value)
#define GET_TX_DESC_BUFFER_RTY_LMT_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 31, 1)
#define SET_TX_DESC_BUFFER_HW_RTS_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 30, 1, __Value)
#define GET_TX_DESC_BUFFER_HW_RTS_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 30, 1)
#define SET_TX_DESC_BUFFER_RTS_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 29, 1, __Value)
#define GET_TX_DESC_BUFFER_RTS_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 29, 1)
#define SET_TX_DESC_BUFFER_CTS2SELF(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 28, 1, __Value)
#define GET_TX_DESC_BUFFER_CTS2SELF(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 28, 1)
#define SET_TX_DESC_BUFFER_TAILPAGE_H(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 24, 4, __Value)
#define GET_TX_DESC_BUFFER_TAILPAGE_H(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 24, 4)
#define SET_TX_DESC_BUFFER_TAILPAGE_L(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 16, 8, __Value)
#define GET_TX_DESC_BUFFER_TAILPAGE_L(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 16, 8)
#define SET_TX_DESC_BUFFER_NAVUSEHDR(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 15, 1, __Value)
#define GET_TX_DESC_BUFFER_NAVUSEHDR(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 15, 1)
#define SET_TX_DESC_BUFFER_BMC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 14, 1, __Value)
#define GET_TX_DESC_BUFFER_BMC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 14, 1)
#define SET_TX_DESC_BUFFER_RTS_DATA_RTY_LMT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 8, 6, __Value)
#define GET_TX_DESC_BUFFER_RTS_DATA_RTY_LMT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 8, 6)
#define SET_TX_DESC_BUFFER_HW_AES_IV(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 7, 1, __Value)
#define GET_TX_DESC_BUFFER_HW_AES_IV(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 7, 1)
#define SET_TX_DESC_BUFFER_BT_NULL(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 3, 1, __Value)
#define GET_TX_DESC_BUFFER_BT_NULL(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 3, 1)
#define SET_TX_DESC_BUFFER_EN_DESC_ID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 2, 1, __Value)
#define GET_TX_DESC_BUFFER_EN_DESC_ID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 2, 1)
#define SET_TX_DESC_BUFFER_SECTYPE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x14, 0, 2, __Value)
#define GET_TX_DESC_BUFFER_SECTYPE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x14, 0, 2)

/*TXDESC_WORD6*/

#define SET_TX_DESC_BUFFER_MULTIPLE_PORT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 29, 3, __Value)
#define GET_TX_DESC_BUFFER_MULTIPLE_PORT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 29, 3)
#define SET_TX_DESC_BUFFER_POLLUTED(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 28, 1, __Value)
#define GET_TX_DESC_BUFFER_POLLUTED(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 28, 1)
#define SET_TX_DESC_BUFFER_NULL_1(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 27, 1, __Value)
#define GET_TX_DESC_BUFFER_NULL_1(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 27, 1)
#define SET_TX_DESC_BUFFER_NULL_0(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 26, 1, __Value)
#define GET_TX_DESC_BUFFER_NULL_0(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 26, 1)
#define SET_TX_DESC_BUFFER_TRI_FRAME(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 25, 1, __Value)
#define GET_TX_DESC_BUFFER_TRI_FRAME(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 25, 1)
#define SET_TX_DESC_BUFFER_SPE_RPT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 24, 1, __Value)
#define GET_TX_DESC_BUFFER_SPE_RPT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 24, 1)
#define SET_TX_DESC_BUFFER_FTM_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 23, 1, __Value)
#define GET_TX_DESC_BUFFER_FTM_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 23, 1)
#define SET_TX_DESC_BUFFER_MU_DATARATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 16, 7, __Value)
#define GET_TX_DESC_BUFFER_MU_DATARATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 16, 7)
#define SET_TX_DESC_BUFFER_CCA_RTS(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 14, 2, __Value)
#define GET_TX_DESC_BUFFER_CCA_RTS(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 14, 2)
#define SET_TX_DESC_BUFFER_NDPA(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 12, 2, __Value)
#define GET_TX_DESC_BUFFER_NDPA(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 12, 2)
#define SET_TX_DESC_BUFFER_TXPWR_OFSET_TYPE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 9, 2, __Value)
#define GET_TX_DESC_BUFFER_TXPWR_OFSET_TYPE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 9, 2)
#define SET_TX_DESC_BUFFER_P_AID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x18, 0, 9, __Value)
#define GET_TX_DESC_BUFFER_P_AID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x18, 0, 9)

/*TXDESC_WORD7*/

#define SET_TX_DESC_BUFFER_SW_DEFINE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x1C, 16, 12, __Value)
#define GET_TX_DESC_BUFFER_SW_DEFINE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x1C, 16, 12)
#define SET_TX_DESC_BUFFER_CTRL_CNT_VALID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x1C, 9, 1, __Value)
#define GET_TX_DESC_BUFFER_CTRL_CNT_VALID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x1C, 9, 1)
#define SET_TX_DESC_BUFFER_CTRL_CNT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x1C, 5, 4, __Value)
#define GET_TX_DESC_BUFFER_CTRL_CNT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x1C, 5, 4)
#define SET_TX_DESC_BUFFER_DATA_RTY_LOWEST_RATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x1C, 0, 5, __Value)
#define GET_TX_DESC_BUFFER_DATA_RTY_LOWEST_RATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x1C, 0, 5)

/*TXDESC_WORD8*/

#define SET_TX_DESC_BUFFER_PATH_MAPA(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 30, 2, __Value)
#define GET_TX_DESC_BUFFER_PATH_MAPA(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 30, 2)
#define SET_TX_DESC_BUFFER_PATH_MAPB(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 28, 2, __Value)
#define GET_TX_DESC_BUFFER_PATH_MAPB(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 28, 2)
#define SET_TX_DESC_BUFFER_PATH_MAPC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 26, 2, __Value)
#define GET_TX_DESC_BUFFER_PATH_MAPC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 26, 2)
#define SET_TX_DESC_BUFFER_PATH_MAPD(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 24, 2, __Value)
#define GET_TX_DESC_BUFFER_PATH_MAPD(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 24, 2)
#define SET_TX_DESC_BUFFER_ANTSEL_A(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 20, 4, __Value)
#define GET_TX_DESC_BUFFER_ANTSEL_A(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 20, 4)
#define SET_TX_DESC_BUFFER_ANTSEL_B(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 16, 4, __Value)
#define GET_TX_DESC_BUFFER_ANTSEL_B(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 16, 4)
#define SET_TX_DESC_BUFFER_ANTSEL_C(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 12, 4, __Value)
#define GET_TX_DESC_BUFFER_ANTSEL_C(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 12, 4)
#define SET_TX_DESC_BUFFER_ANTSEL_D(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 8, 4, __Value)
#define GET_TX_DESC_BUFFER_ANTSEL_D(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 8, 4)
#define SET_TX_DESC_BUFFER_NTX_PATH_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 4, 4, __Value)
#define GET_TX_DESC_BUFFER_NTX_PATH_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 4, 4)
#define SET_TX_DESC_BUFFER_ANTLSEL_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 3, 1, __Value)
#define GET_TX_DESC_BUFFER_ANTLSEL_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 3, 1)
#define SET_TX_DESC_BUFFER_AMPDU_DENSITY(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x20, 0, 3, __Value)
#define GET_TX_DESC_BUFFER_AMPDU_DENSITY(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x20, 0, 3)

/*TXDESC_WORD9*/

#define SET_TX_DESC_BUFFER_VCS_STBC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 30, 2, __Value)
#define GET_TX_DESC_BUFFER_VCS_STBC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 30, 2)
#define SET_TX_DESC_BUFFER_DATA_STBC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 28, 2, __Value)
#define GET_TX_DESC_BUFFER_DATA_STBC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 28, 2)
#define SET_TX_DESC_BUFFER_RTS_RTY_LOWEST_RATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 24, 4, __Value)
#define GET_TX_DESC_BUFFER_RTS_RTY_LOWEST_RATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 24, 4)
#define SET_TX_DESC_BUFFER_SIGNALING_TA_PKT_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 23, 1, __Value)
#define GET_TX_DESC_BUFFER_SIGNALING_TA_PKT_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 23, 1)
#define SET_TX_DESC_BUFFER_MHR_CP(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 22, 1, __Value)
#define GET_TX_DESC_BUFFER_MHR_CP(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 22, 1)
#define SET_TX_DESC_BUFFER_SMH_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 21, 1, __Value)
#define GET_TX_DESC_BUFFER_SMH_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 21, 1)
#define SET_TX_DESC_BUFFER_RTSRATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 16, 5, __Value)
#define GET_TX_DESC_BUFFER_RTSRATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 16, 5)
#define SET_TX_DESC_BUFFER_SMH_CAM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 8, 8, __Value)
#define GET_TX_DESC_BUFFER_SMH_CAM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 8, 8)
#define SET_TX_DESC_BUFFER_ARFR_TABLE_SEL(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 7, 1, __Value)
#define GET_TX_DESC_BUFFER_ARFR_TABLE_SEL(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 7, 1)
#define SET_TX_DESC_BUFFER_ARFR_HT_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 6, 1, __Value)
#define GET_TX_DESC_BUFFER_ARFR_HT_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 6, 1)
#define SET_TX_DESC_BUFFER_ARFR_OFDM_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 5, 1, __Value)
#define GET_TX_DESC_BUFFER_ARFR_OFDM_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 5, 1)
#define SET_TX_DESC_BUFFER_ARFR_CCK_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 4, 1, __Value)
#define GET_TX_DESC_BUFFER_ARFR_CCK_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 4, 1)
#define SET_TX_DESC_BUFFER_RTS_SHORT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 3, 1, __Value)
#define GET_TX_DESC_BUFFER_RTS_SHORT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 3, 1)
#define SET_TX_DESC_BUFFER_DISDATAFB(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 2, 1, __Value)
#define GET_TX_DESC_BUFFER_DISDATAFB(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 2, 1)
#define SET_TX_DESC_BUFFER_DISRTSFB(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 1, 1, __Value)
#define GET_TX_DESC_BUFFER_DISRTSFB(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 1, 1)
#define SET_TX_DESC_BUFFER_EXT_EDCA(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x24, 0, 1, __Value)
#define GET_TX_DESC_BUFFER_EXT_EDCA(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x24, 0, 1)

/*TXDESC_WORD10*/

#define SET_TX_DESC_BUFFER_AMPDU_MAX_TIME(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 24, 8, __Value)
#define GET_TX_DESC_BUFFER_AMPDU_MAX_TIME(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 24, 8)
#define SET_TX_DESC_BUFFER_SPECIAL_CW(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 23, 1, __Value)
#define GET_TX_DESC_BUFFER_SPECIAL_CW(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 23, 1)
#define SET_TX_DESC_BUFFER_RDG_NAV_EXT(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 22, 1, __Value)
#define GET_TX_DESC_BUFFER_RDG_NAV_EXT(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 22, 1)
#define SET_TX_DESC_BUFFER_RAW(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 21, 1, __Value)
#define GET_TX_DESC_BUFFER_RAW(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 21, 1)
#define SET_TX_DESC_BUFFER_MAX_AGG_NUM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 16, 5, __Value)
#define GET_TX_DESC_BUFFER_MAX_AGG_NUM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 16, 5)
#define SET_TX_DESC_BUFFER_FINAL_DATA_RATE(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 8, 8, __Value)
#define GET_TX_DESC_BUFFER_FINAL_DATA_RATE(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 8, 8)
#define SET_TX_DESC_BUFFER_GF(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 7, 1, __Value)
#define GET_TX_DESC_BUFFER_GF(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 7, 1)
#define SET_TX_DESC_BUFFER_MOREFRAG(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 6, 1, __Value)
#define GET_TX_DESC_BUFFER_MOREFRAG(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 6, 1)
#define SET_TX_DESC_BUFFER_NOACM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 5, 1, __Value)
#define GET_TX_DESC_BUFFER_NOACM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 5, 1)
#define SET_TX_DESC_BUFFER_HTC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 4, 1, __Value)
#define GET_TX_DESC_BUFFER_HTC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 4, 1)
#define SET_TX_DESC_BUFFER_TX_PKT_AFTER_PIFS(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 3, 1, __Value)
#define GET_TX_DESC_BUFFER_TX_PKT_AFTER_PIFS(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 3, 1)
#define SET_TX_DESC_BUFFER_USE_MAX_TIME_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 2, 1, __Value)
#define GET_TX_DESC_BUFFER_USE_MAX_TIME_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 2, 1)
#define SET_TX_DESC_BUFFER_HW_SSN_SEL(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x28, 0, 2, __Value)
#define GET_TX_DESC_BUFFER_HW_SSN_SEL(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x28, 0, 2)

/*TXDESC_WORD11*/

#define SET_TX_DESC_BUFFER_ADDR_CAM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 24, 8, __Value)
#define GET_TX_DESC_BUFFER_ADDR_CAM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 24, 8)
#define SET_TX_DESC_BUFFER_SND_TARGET(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 16, 8, __Value)
#define GET_TX_DESC_BUFFER_SND_TARGET(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 16, 8)
#define SET_TX_DESC_BUFFER_DATA_LDPC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 15, 1, __Value)
#define GET_TX_DESC_BUFFER_DATA_LDPC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 15, 1)
#define SET_TX_DESC_BUFFER_LSIG_TXOP_EN(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 14, 1, __Value)
#define GET_TX_DESC_BUFFER_LSIG_TXOP_EN(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 14, 1)
#define SET_TX_DESC_BUFFER_G_ID(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 8, 6, __Value)
#define GET_TX_DESC_BUFFER_G_ID(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 8, 6)
#define SET_TX_DESC_BUFFER_SIGNALING_TA_PKT_SC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 4, 4, __Value)
#define GET_TX_DESC_BUFFER_SIGNALING_TA_PKT_SC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 4, 4)
#define SET_TX_DESC_BUFFER_DATA_SC(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x2C, 0, 4, __Value)
#define GET_TX_DESC_BUFFER_DATA_SC(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x2C, 0, 4)

/*TXDESC_WORD12*/

#define SET_TX_DESC_BUFFER_LEN1_L(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x30, 17, 7, __Value)
#define GET_TX_DESC_BUFFER_LEN1_L(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x30, 17, 7)
#define SET_TX_DESC_BUFFER_LEN0(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x30, 4, 13, __Value)
#define GET_TX_DESC_BUFFER_LEN0(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x30, 4, 13)
#define SET_TX_DESC_BUFFER_PKT_NUM(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x30, 0, 4, __Value)
#define GET_TX_DESC_BUFFER_PKT_NUM(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x30, 0, 4)

/*TXDESC_WORD13*/

#define SET_TX_DESC_BUFFER_LEN3(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x34, 19, 13, __Value)
#define GET_TX_DESC_BUFFER_LEN3(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x34, 19, 13)
#define SET_TX_DESC_BUFFER_LEN2(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x34, 6, 13, __Value)
#define GET_TX_DESC_BUFFER_LEN2(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x34, 6, 13)
#define SET_TX_DESC_BUFFER_LEN1_H(__pTxDesc, __Value)    SET_BITS_TO_LE_4BYTE(__pTxDesc + 0x34, 0, 6, __Value)
#define GET_TX_DESC_BUFFER_LEN1_H(__pTxDesc)    LE_BITS_TO_4BYTE(__pTxDesc + 0x34, 0, 6)

#endif


#endif

