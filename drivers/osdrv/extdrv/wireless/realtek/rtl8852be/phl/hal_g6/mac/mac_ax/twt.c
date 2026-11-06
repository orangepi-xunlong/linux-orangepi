/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation. All rights reserved.
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

#include "twt.h"

u32 twt_info_init(struct mac_ax_adapter *adapter)
{
	adapter->twt_info =
		(struct mac_ax_twt_info *)PLTFM_MALLOC(TWT_INFO_SIZE);
	if (!adapter->twt_info) {
		PLTFM_FREE(adapter->twt_info, TWT_INFO_SIZE);
		return MACBUFALLOC;
	}
	adapter->twt_info->err_rec = 0;
	adapter->twt_info->pdbg_info = (u8 *)PLTFM_MALLOC(TWT_DBG_INFO_SIZE);
	if (!adapter->twt_info->pdbg_info) {
		PLTFM_FREE(adapter->twt_info, TWT_DBG_INFO_SIZE);
		return MACBUFALLOC;
	}
	PLTFM_MEMSET(adapter->twt_info->pdbg_info, 0, TWT_DBG_INFO_SIZE);

	return MACSUCCESS;
}

u32 twt_info_exit(struct mac_ax_adapter *adapter)
{
	PLTFM_FREE(adapter->twt_info->pdbg_info, TWT_DBG_INFO_SIZE);
	PLTFM_FREE(adapter->twt_info, TWT_INFO_SIZE);

	return MACSUCCESS;
}

u32 mac_twt_info_upd_h2c(struct mac_ax_adapter *adapter,
			 struct mac_ax_twt_para *info)
{
	u32 ret;
	struct h2c_info h2c_info = {0};
	struct fwcmd_twtinfo_upd *content;

	/* port 4 not support */
	if (info->port >= MAC_AX_PORT_4) {
		PLTFM_MSG_ERR("[ERR] twt info upd h2c port %d\n", info->port);
		return MACFUNCINPUT;
	}

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_twtinfo_upd);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_TWT;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_TWTINFO_UPD;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;

	content = (struct fwcmd_twtinfo_upd *)PLTFM_MALLOC(h2c_info.content_len);

	if (!content) {
		PLTFM_FREE(content, h2c_info.content_len);
		return MACBUFALLOC;
	}

	content->dword0 =
		cpu_to_le32(SET_WORD(info->nego_tp,
				     FWCMD_H2C_TWTINFO_UPD_NEGOTYPE) |
			    SET_WORD(info->act, FWCMD_H2C_TWTINFO_UPD_ACT) |
			    (info->trig ? FWCMD_H2C_TWTINFO_UPD_TRIGGER : 0) |
			    (info->flow_tp ?
			     FWCMD_H2C_TWTINFO_UPD_FLOWTYPE : 0) |
			    (info->impt ? FWCMD_H2C_TWTINFO_UPD_IMPT : 0) |
			    (info->wake_unit ?
			     FWCMD_H2C_TWTINFO_UPD_WAKEDURUNIT : 0) |
			    (info->rsp_pm ? FWCMD_H2C_TWTINFO_UPD_RSPPM : 0) |
			    (info->proct ? FWCMD_H2C_TWTINFO_UPD_PROT : 0) |
			    SET_WORD(info->flow_id,
				     FWCMD_H2C_TWTINFO_UPD_FLOWID) |
			    SET_WORD(info->id, FWCMD_H2C_TWTINFO_UPD_ID) |
			    (info->band ? FWCMD_H2C_TWTINFO_UPD_BAND : 0) |
			    SET_WORD(info->port, FWCMD_H2C_TWTINFO_UPD_PORT));

	content->dword1 =
		cpu_to_le32(SET_WORD(info->wake_exp,
				     FWCMD_H2C_TWTINFO_UPD_WAKE_EXP) |
			    SET_WORD(info->wake_man,
				     FWCMD_H2C_TWTINFO_UPD_WAKE_MAN) |
			    SET_WORD(info->twtulfixmode,
				     FWCMD_H2C_TWTINFO_UPD_ULFIXMODE) |
			    SET_WORD(info->dur,
				     FWCMD_H2C_TWTINFO_UPD_DUR));

	content->dword2 =
		cpu_to_le32(SET_WORD(info->trgt_l,
				     FWCMD_H2C_TWTINFO_UPD_TGT_L));

	content->dword3 =
		cpu_to_le32(SET_WORD(info->trgt_h,
				     FWCMD_H2C_TWTINFO_UPD_TGT_H));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);

	PLTFM_FREE(content, h2c_info.content_len);

	return ret;
}

u32 mac_twt_act_h2c(struct mac_ax_adapter *adapter,
		    struct mac_ax_twtact_para *info)
{
	u32 ret;
	struct h2c_info h2c_info = {0};
	struct fwcmd_twt_stansp_upd *content;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_twt_stansp_upd);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_TWT;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_TWT_STANSP_UPD;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	content = (struct fwcmd_twt_stansp_upd *)
		  PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_FREE(content, h2c_info.content_len);
		return MACBUFALLOC;
	}

	content->dword0 =
		cpu_to_le32(SET_WORD(info->macid,
				     FWCMD_H2C_TWT_STANSP_UPD_MACID) |
			    SET_WORD(info->id,
				     FWCMD_H2C_TWT_STANSP_UPD_ID) |
			    SET_WORD(info->act,
				     FWCMD_H2C_TWT_STANSP_UPD_ACT));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);

	PLTFM_FREE(content, h2c_info.content_len);

	return ret;
}

u32 mac_twt_staanno_h2c(struct mac_ax_adapter *adapter,
			struct mac_ax_twtanno_para *info)
{
	struct h2c_info h2c_info = {0};
	struct fwcmd_twt_announce_upd *hdr;
	u32 ret = MACSUCCESS;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_twt_announce_upd);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_TWT;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_TWT_ANNOUNCE_UPD;
	h2c_info.rec_ack = 1;
	h2c_info.done_ack = 0;

	hdr = (struct fwcmd_twt_announce_upd *)PLTFM_MALLOC(h2c_info.content_len);

	if (!hdr) {
		PLTFM_FREE(hdr, h2c_info.content_len);
		return MACBUFALLOC;
	}

	hdr->dword0 =
		cpu_to_le32(SET_WORD(info->macid, FWCMD_H2C_TWT_ANNOUNCE_UPD_MACID));


	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)hdr);
	PLTFM_FREE(hdr, h2c_info.content_len);

	return ret;
}

void mac_twt_wait_anno(struct mac_ax_adapter *adapter,
		       u8 *c2h_content, u8 *upd_addr)
{
	u32 plat_c2h_content = *(u32 *)(c2h_content);
	struct mac_ax_twtanno_c2hpara *para =
		(struct mac_ax_twtanno_c2hpara *)upd_addr;

	para->wait_case = GET_FIELD(plat_c2h_content,
				    FWCMD_C2H_WAIT_ANNOUNCE_WAIT_CASE);
	para->macid0 = GET_FIELD(plat_c2h_content,
				 FWCMD_C2H_WAIT_ANNOUNCE_MACID0);
	para->macid1 = GET_FIELD(plat_c2h_content,
				 FWCMD_C2H_WAIT_ANNOUNCE_MACID1);
	para->macid2 = GET_FIELD(plat_c2h_content,
				 FWCMD_C2H_WAIT_ANNOUNCE_MACID2);
}

u32 mac_get_tsf(struct mac_ax_adapter *adapter, struct mac_ax_port_tsf *tsf)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_l = 0;
	u32 reg_h = 0;
	u32 ret;
	u8 port = tsf->port;

	ret = check_mac_en(adapter, tsf->band, MAC_AX_CMAC_SEL);
	if (ret != MACSUCCESS)
		return ret;

	if (port >= adapter->hw_info->port_num) {
		PLTFM_MSG_ERR("%s invalid port idx %d\n", __func__, port);
		return MACPORTERR;
	}

	switch (port) {
	case MAC_AX_PORT_0:
		reg_h = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_HIGH_P0 : R_AX_TSFTR_HIGH_P0_C1;
		reg_l = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_LOW_P0 : R_AX_TSFTR_LOW_P0_C1;
		break;
	case MAC_AX_PORT_1:
		reg_h = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_HIGH_P1 : R_AX_TSFTR_HIGH_P1_C1;
		reg_l = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_LOW_P1 : R_AX_TSFTR_LOW_P1_C1;
		break;
	case MAC_AX_PORT_2:
		reg_h = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_HIGH_P2 : R_AX_TSFTR_HIGH_P2_C1;
		reg_l = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_LOW_P2 : R_AX_TSFTR_LOW_P2_C1;
		break;
	case MAC_AX_PORT_3:
		reg_h = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_HIGH_P3 : R_AX_TSFTR_HIGH_P3_C1;
		reg_l = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_LOW_P3 : R_AX_TSFTR_LOW_P3_C1;
		break;
	case MAC_AX_PORT_4:
		reg_h = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_HIGH_P4 : R_AX_TSFTR_HIGH_P4_C1;
		reg_l = tsf->band == MAC_AX_BAND_0 ?
			R_AX_TSFTR_LOW_P4 : R_AX_TSFTR_LOW_P4_C1;
		break;
	default:
		return MACFUNCINPUT;
	}

	tsf->tsf_h = MAC_REG_R32(reg_h);
	tsf->tsf_l = MAC_REG_R32(reg_l);

	return MACSUCCESS;
}

