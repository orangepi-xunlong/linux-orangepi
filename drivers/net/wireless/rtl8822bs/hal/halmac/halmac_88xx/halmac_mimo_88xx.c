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

#include "halmac_mimo_88xx.h"
#include "halmac_88xx_cfg.h"
#include "halmac_common_88xx.h"
#include "halmac_init_88xx.h"

#if HALMAC_88XX_SUPPORT

static HALMAC_FW_SNDING_CMD_CONSTRUCT_STATE
halmac_query_fw_snding_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

static HALMAC_RET_STATUS
halmac_transition_fw_snding_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FW_SNDING_CMD_CONSTRUCT_STATE dest_state
);

static u8
halmac_snding_pkt_chk_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pSnd_pkt
);

/**
 * halmac_cfg_txbf_88xx() - enable/disable specific user's txbf
 * @pHalmac_adapter : the adapter of halmac
 * @userid : su bfee userid = 0 or 1 to apply TXBF
 * @bw : the sounding bandwidth
 * @txbf_en : 0: disable TXBF, 1: enable TXBF
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_txbf_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid,
	IN HALMAC_BW bw,
	IN u8 txbf_en
)
{
	u16 temp42C = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (txbf_en) {
		switch (bw) {
		case HALMAC_BW_80:
			temp42C |= BIT_R_TXBF0_80M;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	__attribute__ ((fallthrough));
#else
			__attribute__ ((__fallthrough__));
#endif
		case HALMAC_BW_40:
			temp42C |= BIT_R_TXBF0_40M;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	__attribute__ ((fallthrough));
#else
			__attribute__ ((__fallthrough__));
#endif
		case HALMAC_BW_20:
			temp42C |= BIT_R_TXBF0_20M;
			break;
		default:
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_cfg_txbf_88xx invalid TXBF BW setting 0x%x of userid %d\n", bw, userid);
			return HALMAC_RET_INVALID_SOUNDING_SETTING;
		}
	}

	switch (userid) {
	case 0:
		temp42C |= HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL) & ~(BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL, temp42C);
		break;
	case 1:
		temp42C |= HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL + 2) & ~(BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL + 2, temp42C);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_cfg_txbf_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_txbf_88xx, txbf_en = %x <==========\n", txbf_en);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_mumimo_88xx() -config mumimo
 * @pHalmac_adapter : the adapter of halmac
 * @pCfgmu : parameters to configure MU PPDU Tx/Rx
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_mumimo_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_CFG_MUMIMO_PARA pCfgmu
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u8 i, idx, id0, id1, gid, mu_tab_sel;
	u8 mu_tab_valid = 0;
	u32 gid_valid[6] = {0};
	u8 temp14C0 = 0;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (pCfgmu->role == HAL_BFEE) {
		/*config MU BFEE*/
		temp14C0 = HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL) & ~BIT_MASK_R_MU_TABLE_VALID;
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, (temp14C0 | BIT(0) | BIT(1)) & ~(BIT(7)));	/*enable MU table 0 and 1, disable MU TX*/

		/*config GID valid table and user position table*/
		mu_tab_sel = HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL + 1) & ~(BIT(0) | BIT(1) | BIT(2));
		for (i = 0; i < 2; i++) {
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL + 1, mu_tab_sel | i);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_GID_VLD, pCfgmu->given_gid_tab[i]);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO, pCfgmu->given_user_pos[i * 2]);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO + 4, pCfgmu->given_user_pos[i * 2 + 1]);
		}
	} else {
		/*config MU BFER*/
		if (pCfgmu->mu_tx_en == _FALSE) {
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL) & ~(BIT(7)));
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_mumimo_88xx disable mu tx <==========\n");
			return HALMAC_RET_SUCCESS;
		}

		/*Transform BB grouping bitmap[14:0] to MAC GID_valid table*/
		for (idx = 0; idx < 15; idx++) {
			if (idx < 5) {
				/*grouping_bitmap bit0~4, MU_STA0 with MUSTA1~5*/
				id0 = 0;
				id1 = (u8)(idx + 1);
			} else if (idx < 9) {
				/*grouping_bitmap bit5~8, MU_STA1 with MUSTA2~5*/
				id0 = 1;
				id1 = (u8)(idx - 3);
			} else if (idx < 12) {
				/*grouping_bitmap bit9~11, MU_STA2 with MUSTA3~5*/
				id0 = 2;
				id1 = (u8)(idx - 6);
			} else if (idx < 14) {
				/*grouping_bitmap bit12~13, MU_STA3 with MUSTA4~5*/
				id0 = 3;
				id1 = (u8)(idx - 8);
			} else {
				/*grouping_bitmap bit14, MU_STA4 with MUSTA5*/
				id0 = 4;
				id1 = (u8)(idx - 9);
			}
			if (pCfgmu->grouping_bitmap & BIT(idx)) {
				/*Pair 1*/
				gid = (idx << 1) + 1;
				gid_valid[id0] |= (BIT(gid));
				gid_valid[id1] |= (BIT(gid));
				/*Pair 2*/
				gid += 1;
				gid_valid[id0] |= (BIT(gid));
				gid_valid[id1] |= (BIT(gid));
			} else {
				/*Pair 1*/
				gid = (idx << 1) + 1;
				gid_valid[id0] &= ~(BIT(gid));
				gid_valid[id1] &= ~(BIT(gid));
				/*Pair 2*/
				gid += 1;
				gid_valid[id0] &= ~(BIT(gid));
				gid_valid[id1] &= ~(BIT(gid));
			}
		}

		/*set MU STA GID valid TABLE*/
		mu_tab_sel = HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL + 1) & ~(BIT(0) | BIT(1) | BIT(2));
		for (idx = 0; idx < 6; idx++) {
			HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL + 1, idx | mu_tab_sel);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_GID_VLD, gid_valid[idx]);
		}

		/*To validate the sounding successful MU STA and enable MU TX*/
		for (i = 0; i < 6; i++) {
			if (pCfgmu->sounding_sts[i] == _TRUE)
				mu_tab_valid |= BIT(i);
		}
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, mu_tab_valid | BIT(7));
	}
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_mumimo_88xx <==========\n");
	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_sounding_88xx() - configure general sounding
 * @pHalmac_adapter : the adapter of halmac
 * @role : driver's role, BFer or BFee
 * @datarate : set ndpa tx rate if driver is BFer, or set csi response rate if driver is BFee
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_sounding_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SND_ROLE role,
	IN HALMAC_DATA_RATE	datarate
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (role) {
	case HAL_BFER:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_TXBF_CTRL, HALMAC_REG_READ_32(pHalmac_adapter, REG_TXBF_CTRL) | BIT_R_ENABLE_NDPA
			| BIT_USE_NDPA_PARAMETER | BIT_R_EN_NDPA_INT | BIT_DIS_NDP_BFEN);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_NDPA_RATE, datarate);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_NDPA_OPT_CTRL, HALMAC_REG_READ_8(pHalmac_adapter, REG_NDPA_OPT_CTRL) & (~(BIT(0) | BIT(1))));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL + 1, 0x2 | BIT(7));	/*service file length 2 bytes; fix non-STA1 csi start offset */
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL + 2, 0x2);
		break;
	case HAL_BFEE:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL, 0xDB);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL + 3, 0x26);
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_BBPSF_CTRL + 3, HALMAC_OFDM54 | BIT(6));		//use ndpa rx rate to decide csi rate
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RRSR, HALMAC_REG_READ_16(pHalmac_adapter, REG_RRSR) | BIT(datarate));
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RXFLTMAP1, HALMAC_REG_READ_8(pHalmac_adapter, REG_RXFLTMAP1) & (~(BIT(4))));	/*RXFF do not accept BF Rpt Poll, avoid CSI crc error*/
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_RXFLTMAP4, HALMAC_REG_READ_8(pHalmac_adapter, REG_RXFLTMAP4) & (~(BIT(4))));	/*FWFF do not accept BF Rpt Poll, avoid CSI crc error*/
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_cfg_sounding_88xx invalid role\n");
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_sounding_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_del_sounding_88xx() - reset general sounding
 * @pHalmac_adapter : the adapter of halmac
 * @role : driver's role, BFer or BFee
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_del_sounding_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SND_ROLE role
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (role) {
	case HAL_BFER:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_TXBF_CTRL + 3, 0);
		break;
	case HAL_BFEE:
		HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SND_PTCL_CTRL, 0);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_del_sounding_88xx invalid role\n");
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_del_sounding_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_init_88xx() - config SU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : SU bfee userid = 0 or 1 to be added
 * @paid : partial AID of this bfee
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfee_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid,
	IN u16 paid
)
{
	u16 temp42C = 0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (userid) {
	case 0:
		temp42C = HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL) & ~(BIT_MASK_R_TXBF0_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL, temp42C | paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL, paid);
		break;
	case 1:
		temp42C = HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL + 2) & ~(BIT_MASK_R_TXBF1_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL + 2, temp42C | paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL + 2, paid | BIT(9));
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_su_bfee_entry_init_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_su_bfee_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_init_88xx() - config SU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * @pSu_bfer_init : parameters to configure SU BFER entry
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfer_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SU_BFER_INIT_PARA pSu_bfer_init
)
{
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	/* mac_address_L = bfer_address.Address_L_H.Address_Low; */
	/* mac_address_H = bfer_address.Address_L_H.Address_High; */

	mac_address_L = rtk_le32_to_cpu(pSu_bfer_init->bfer_address.Address_L_H.Address_Low);
	mac_address_H = rtk_le16_to_cpu(pSu_bfer_init->bfer_address.Address_L_H.Address_High);

	switch (pSu_bfer_init->userid) {
	case 0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, mac_address_H);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 6, pSu_bfer_init->paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TX_CSI_RPT_PARAM_BW20, pSu_bfer_init->csi_para);
		break;
	case 1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO, mac_address_L);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO + 4, mac_address_H);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO + 6, pSu_bfer_init->paid);
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TX_CSI_RPT_PARAM_BW20 + 2, pSu_bfer_init->csi_para);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_su_bfer_entry_init_88xx invalid userid %d\n", pSu_bfer_init->userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_su_bfer_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfee_entry_init_88xx() - config MU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @pMu_bfee_init : parameters to configure MU BFEE entry
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfee_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_MU_BFEE_INIT_PARA pMu_bfee_init
)
{
	u16 temp168X = 0, temp14C0;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	temp168X |= pMu_bfee_init->paid | BIT(9);
	HALMAC_REG_WRITE_16(pHalmac_adapter, (0x1680 + pMu_bfee_init->userid * 2), temp168X);

	temp14C0 = HALMAC_REG_READ_16(pHalmac_adapter, REG_MU_TX_CTL) & ~(BIT(8) | BIT(9) | BIT(10));
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_MU_TX_CTL, temp14C0 | ((pMu_bfee_init->userid - 2) << 8));
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_GID_VLD, 0);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO, pMu_bfee_init->user_position_l);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_MU_STA_USER_POS_INFO + 4, pMu_bfee_init->user_position_h);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_mu_bfee_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfer_entry_init_88xx() - config MU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * @pMu_bfer_init : parameters to configure MU BFER entry
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfer_entry_init_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_MU_BFER_INIT_PARA pMu_bfer_init
)
{
	u16 temp1680 = 0;
	u16 mac_address_H;
	u32 mac_address_L;
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	/* mac_address_L = pHalmac_adapter->snd_info.bfer_address.Address_L_H.Address_Low; */
	/* mac_address_H = pHalmac_adapter->snd_info.bfer_address.Address_L_H.Address_High; */

	mac_address_L = rtk_le32_to_cpu(pMu_bfer_init->bfer_address.Address_L_H.Address_Low);
	mac_address_H = rtk_le16_to_cpu(pMu_bfer_init->bfer_address.Address_L_H.Address_High);

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, mac_address_L);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, mac_address_H);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 6, pMu_bfer_init->paid);
	HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TX_CSI_RPT_PARAM_BW20, pMu_bfer_init->csi_para);

	temp1680 = HALMAC_REG_READ_16(pHalmac_adapter, 0x1680) & 0xC000;
	temp1680 |= pMu_bfer_init->my_aid | (pMu_bfer_init->csi_length_sel << 12);
	HALMAC_REG_WRITE_16(pHalmac_adapter, 0x1680, temp1680);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_mu_bfer_entry_init_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_del_88xx() - reset SU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : the SU BFee userid to be deleted
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfee_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (userid) {
	case 0:
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL, HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL) &
			~(BIT_MASK_R_TXBF0_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M));
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL, 0);
		break;
	case 1:
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_TXBF_CTRL + 2, HALMAC_REG_READ_16(pHalmac_adapter, REG_TXBF_CTRL + 2) &
			~(BIT_MASK_R_TXBF1_AID | BIT_R_TXBF0_20M | BIT_R_TXBF0_40M | BIT_R_TXBF0_80M));
		HALMAC_REG_WRITE_16(pHalmac_adapter, REG_ASSOCIATED_BFMEE_SEL + 2, 0);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_su_bfee_entry_del_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_su_bfee_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_su_bfee_entry_del_88xx() - reset SU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : the SU BFer userid to be deleted
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_su_bfer_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	switch (userid) {
	case 0:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, 0);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, 0);
		break;
	case 1:
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO, 0);
		HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER1_INFO + 4, 0);
		break;
	default:
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_ERR, "[ERR]halmac_su_bfer_entry_del_88xx invalid userid %d\n", userid);
		return HALMAC_RET_INVALID_SOUNDING_SETTING;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_su_bfer_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfee_entry_del_88xx() - reset MU beamformee's registers
 * @pHalmac_adapter : the adapter of halmac
 * @userid : the MU STA userid to be deleted
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfee_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 userid
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_16(pHalmac_adapter, 0x1680 + userid * 2, 0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, HALMAC_REG_READ_8(pHalmac_adapter, REG_MU_TX_CTL) & ~(BIT(userid - 2)));

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_mu_bfee_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_mu_bfer_entry_del_88xx() -reset MU beamformer's registers
 * @pHalmac_adapter : the adapter of halmac
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_mu_bfer_entry_del_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO, 0);
	HALMAC_REG_WRITE_32(pHalmac_adapter, REG_ASSOCIATED_BFMER0_INFO + 4, 0);
	HALMAC_REG_WRITE_16(pHalmac_adapter, 0x1680, 0);
	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_MU_TX_CTL, 0);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_mu_bfer_entry_del_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_cfg_csi_rate_88xx() - config CSI frame Tx rate
 * @pHalmac_adapter : the adapter of halmac
 * @rssi : rssi in decimal value
 * @current_rate : current CSI frame rate
 * @fixrate_en : enable to fix CSI frame in VHT rate, otherwise legacy OFDM rate
 * @new_rate : API returns the final CSI frame rate
 * Author : chunchu
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_cfg_csi_rate_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 rssi,
	IN u8 current_rate,
	IN u8 fixrate_en,
	OUT u8 *new_rate
)
{
	VOID *pDriver_adapter = NULL;
	PHALMAC_API pHalmac_api;
	u32 temp_csi_setting;
	u16 current_rrsr;

	if (halmac_adapter_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ADAPTER_INVALID;

	if (halmac_api_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_API_INVALID;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_SND, HALMAC_DBG_TRACE, "[TRACE]halmac_cfg_csi_rate_88xx ==========>\n");

#if HALMAC_8821C_SUPPORT
	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C) {
		if (fixrate_en) {
			temp_csi_setting = HALMAC_REG_READ_32(pHalmac_adapter, REG_BBPSF_CTRL) & ~(BIT_MASK_WMAC_CSI_RATE << BIT_SHIFT_WMAC_CSI_RATE);
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BBPSF_CTRL, temp_csi_setting | BIT_CSI_FORCE_RATE_EN | BIT_CSI_RSC(1) | BIT_WMAC_CSI_RATE(HALMAC_VHT_NSS1_MCS3));
			*new_rate = HALMAC_VHT_NSS1_MCS3;
			return HALMAC_RET_SUCCESS;
		}
	}
	temp_csi_setting = HALMAC_REG_READ_32(pHalmac_adapter, REG_BBPSF_CTRL) & ~(BIT_MASK_WMAC_CSI_RATE << BIT_SHIFT_WMAC_CSI_RATE) & ~BIT_CSI_FORCE_RATE_EN;
#else
	temp_csi_setting = HALMAC_REG_READ_32(pHalmac_adapter, REG_BBPSF_CTRL) & ~(BIT_MASK_WMAC_CSI_RATE << BIT_SHIFT_WMAC_CSI_RATE);
#endif

	current_rrsr = HALMAC_REG_READ_16(pHalmac_adapter, REG_RRSR);

	if (rssi >= 40) {
		if (current_rate != HALMAC_OFDM54) {
			HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RRSR, current_rrsr | BIT(HALMAC_OFDM54));
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BBPSF_CTRL, temp_csi_setting | BIT_WMAC_CSI_RATE(HALMAC_OFDM54));
		}
		*new_rate = HALMAC_OFDM54;
	} else {
		if (current_rate != HALMAC_OFDM24) {
			HALMAC_REG_WRITE_16(pHalmac_adapter, REG_RRSR, current_rrsr & ~(BIT(HALMAC_OFDM54)));
			HALMAC_REG_WRITE_32(pHalmac_adapter, REG_BBPSF_CTRL, temp_csi_setting | BIT_WMAC_CSI_RATE(HALMAC_OFDM24));
		}
		*new_rate = HALMAC_OFDM24;
	}

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_fw_snding_88xx() - fw sounding control
 * @pHalmac_adapter : the adapter of halmac
 * @pSu_snding :
 *	su0_en : enable/disable fw sounding
 *	pSu0_ndpa_pkt : ndpa pkt, shall include txdesc
 *	su0_pkt_sz : ndpa pkt size, shall include txdesc
 * @pMu_snding : currently not in use, input NULL is acceptable
 * @period : sounding period, unit is 5ms
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_fw_snding_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SU_SNDING_INFO pSu_snding,
	IN PHALMAC_MU_SNDING_INFO pMu_snding,
	IN u8 period
)
{
	u8 pH2c_buff[HALMAC_H2C_CMD_SIZE_88XX] = { 0 };
	u16 h2c_seq_mum;
	VOID *pDriver_adapter = NULL;
	HALMAC_H2C_HEADER_INFO h2c_header_info;
	HALMAC_CMD_PROCESS_STATUS *pProcess_status = &pHalmac_adapter->halmac_state.fw_snding_set.process_status;
	HALMAC_RET_STATUS status;

	if (pHalmac_adapter->chip_id == HALMAC_CHIP_ID_8821C)
		return HALMAC_RET_NOT_SUPPORT;

	if (halmac_fw_validate(pHalmac_adapter) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_NO_DLFW;

	if (pHalmac_adapter->fw_version.h2c_version < 9)
		return HALMAC_RET_FW_NO_SUPPORT;

	if (*pProcess_status == HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_TRACE, "[TRACE]Wait event(fw sounding)...\n");
		return HALMAC_RET_BUSY_STATE;
	}

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (pSu_snding->su0_en == 1) {
		if (pSu_snding->pSu0_ndpa_pkt == NULL)
			return HALMAC_RET_NULL_POINTER;

		if (pSu_snding->su0_pkt_sz > (u32)HALMAC_TX_PAGE_SIZE_88XX - pHalmac_adapter->hw_config_info.txdesc_size)
			return HALMAC_RET_DATA_SIZE_INCORRECT;

		if (halmac_snding_pkt_chk_88xx(pHalmac_adapter, pSu_snding->pSu0_ndpa_pkt) == _FALSE)
			return HALMAC_RET_TXDESC_SET_FAIL;

		if (halmac_query_fw_snding_curr_state_88xx(pHalmac_adapter) != HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not idle state(fw sounding)...\n");
			return HALMAC_RET_ERROR_STATE;
		}

		status = halmac_download_rsvd_page_88xx(pHalmac_adapter,
						pHalmac_adapter->txff_allocation.rsvd_h2c_static_info_pg_bndy + HALMAC_SU0_SNDING_PKT_OFFSET_88XX,
						pSu_snding->pSu0_ndpa_pkt, pSu_snding->su0_pkt_sz);
		if (status != HALMAC_RET_SUCCESS) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_INIT, HALMAC_DBG_ERR, "[ERR]halmac_download_rsvd_page_88xx Fail = %x!!\n", status);
			return status;
		}

		FW_SNDING_SET_SU0(pH2c_buff, 1);
		FW_SNDING_SET_PERIOD(pH2c_buff, period);
		FW_SNDING_SET_NDPA0_HEAD_PG(pH2c_buff,
					pHalmac_adapter->txff_allocation.rsvd_h2c_static_info_pg_bndy + HALMAC_SU0_SNDING_PKT_OFFSET_88XX
					- pHalmac_adapter->txff_allocation.rsvd_pg_bndy);
	} else {
		if (halmac_query_fw_snding_curr_state_88xx(pHalmac_adapter) != HALMAC_FW_SNDING_CMD_CONSTRUCT_SNDING) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not snding state(fw sounding)...\n");
			return HALMAC_RET_ERROR_STATE;
		}
		FW_SNDING_SET_SU0(pH2c_buff, 0);
	}

	*pProcess_status = HALMAC_CMD_PROCESS_SENDING;

	h2c_header_info.sub_cmd_id = SUB_CMD_ID_FW_SNDING;
	h2c_header_info.content_size = 8;
	h2c_header_info.ack = _TRUE;
	halmac_set_fw_offload_h2c_header_88xx(pHalmac_adapter, pH2c_buff, &h2c_header_info, &h2c_seq_mum);
	pHalmac_adapter->halmac_state.fw_snding_set.seq_num = h2c_seq_mum;

	status = halmac_send_h2c_pkt_88xx(pHalmac_adapter, pH2c_buff, HALMAC_H2C_CMD_SIZE_88XX, _TRUE);
	if (status != HALMAC_RET_SUCCESS) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]halmac_send_h2c_fw_snding_88xx Fail = %x!!\n", status);
		halmac_reset_feature_88xx(pHalmac_adapter, HALMAC_FEATURE_UPDATE_PACKET);
		return status;
	}

	if (halmac_transition_fw_snding_state_88xx(pHalmac_adapter, (pSu_snding->su0_en == 1) ?
				HALMAC_FW_SNDING_CMD_CONSTRUCT_SNDING : HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE) != HALMAC_RET_SUCCESS)
		return HALMAC_RET_ERROR_STATE;

	return HALMAC_RET_SUCCESS;
}

static u8
halmac_snding_pkt_chk_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pSnd_pkt
)
{
	u8 data_rate;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	if (GET_TX_DESC_NDPA(pSnd_pkt) == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]txdesc ndpa = 0\n");
		return _FALSE;
	}

	data_rate = (u8)GET_TX_DESC_DATARATE(pSnd_pkt);
	if (!(data_rate >= HALMAC_VHT_NSS2_MCS0 && data_rate <= HALMAC_VHT_NSS2_MCS9)) {
		if (!(data_rate >= HALMAC_MCS8 && data_rate <= HALMAC_MCS15)) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]txdesc rate = %d\n", data_rate);
			return _FALSE;
		}
	}

	if (GET_TX_DESC_NAVUSEHDR(pSnd_pkt) == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]txdesc navusehdr = 0\n");
		return _FALSE;
	}

	if (GET_TX_DESC_USE_RATE(pSnd_pkt) == 0) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]txdesc userate = 0\n");
		return _FALSE;
	}

	return _TRUE;
}

static HALMAC_FW_SNDING_CMD_CONSTRUCT_STATE
halmac_query_fw_snding_curr_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
)
{
	return pHalmac_adapter->halmac_state.fw_snding_set.fw_snding_cmd_construct_state;
}

HALMAC_RET_STATUS
halmac_parse_h2c_ack_fw_snding_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *pC2h_buf,
	IN u32 c2h_size
)
{
	u8 h2c_seq = 0;
	u8 h2c_return_code;
	VOID *pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	HALMAC_CMD_PROCESS_STATUS process_status;

	h2c_seq = (u8)H2C_ACK_HDR_GET_H2C_SEQ(pC2h_buf);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_TRACE, "[TRACE]Seq num : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.fw_snding_set.seq_num, h2c_seq);
	if (h2c_seq != pHalmac_adapter->halmac_state.fw_snding_set.seq_num) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Seq num mismactch : h2c -> %d c2h -> %d\n", pHalmac_adapter->halmac_state.fw_snding_set.seq_num, h2c_seq);
		return HALMAC_RET_SUCCESS;
	}

	if (pHalmac_adapter->halmac_state.fw_snding_set.process_status != HALMAC_CMD_PROCESS_SENDING) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_H2C, HALMAC_DBG_ERR, "[ERR]Not in HALMAC_CMD_PROCESS_SENDING\n");
		return HALMAC_RET_SUCCESS;
	}

	h2c_return_code = (u8)H2C_ACK_HDR_GET_H2C_RETURN_CODE(pC2h_buf);
	pHalmac_adapter->halmac_state.fw_snding_set.fw_return_code = h2c_return_code;

	if ((HALMAC_H2C_RETURN_CODE)h2c_return_code == HALMAC_H2C_RETURN_SUCCESS) {
		process_status = HALMAC_CMD_PROCESS_DONE;
		pHalmac_adapter->halmac_state.fw_snding_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_FW_SNDING, process_status, NULL, 0);
	} else {
		process_status = HALMAC_CMD_PROCESS_ERROR;
		pHalmac_adapter->halmac_state.fw_snding_set.process_status = process_status;
		PLATFORM_EVENT_INDICATION(pDriver_adapter, HALMAC_FEATURE_FW_SNDING, process_status, &pHalmac_adapter->halmac_state.fw_snding_set.fw_return_code, 1);
	}

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_query_fw_snding_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	OUT HALMAC_CMD_PROCESS_STATUS *pProcess_status,
	INOUT u8 *data,
	INOUT u32 *size
)
{
	PHALMAC_FW_SNDING_STATE_SET pfw_snding_state_set = &pHalmac_adapter->halmac_state.fw_snding_set;

	*pProcess_status = pfw_snding_state_set->process_status;

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_transition_fw_snding_state_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FW_SNDING_CMD_CONSTRUCT_STATE dest_state
)
{
	PHALMAC_FW_SNDING_STATE_SET pFw_snding = &pHalmac_adapter->halmac_state.fw_snding_set;

	if ((pFw_snding->fw_snding_cmd_construct_state != HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE) &&
	    (pFw_snding->fw_snding_cmd_construct_state != HALMAC_FW_SNDING_CMD_CONSTRUCT_SNDING))
		return HALMAC_RET_ERROR_STATE;

	if (dest_state == HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE) {
		if (pFw_snding->fw_snding_cmd_construct_state == HALMAC_FW_SNDING_CMD_CONSTRUCT_IDLE)
			return HALMAC_RET_ERROR_STATE;
	} else if (dest_state == HALMAC_FW_SNDING_CMD_CONSTRUCT_SNDING) {
		if (pFw_snding->fw_snding_cmd_construct_state == HALMAC_FW_SNDING_CMD_CONSTRUCT_SNDING)
			return HALMAC_RET_ERROR_STATE;
	}

	pFw_snding->fw_snding_cmd_construct_state = dest_state;

	return HALMAC_RET_SUCCESS;
}
#endif /* HALMAC_88XX_SUPPORT */
