/******************************************************************************
 *
 * Copyright(c) 2021 Realtek Corporation. All rights reserved.
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
#include "nan.h"

u32 mac_get_act_schedule_id(struct mac_ax_adapter *adapter,
			    struct mac_ax_act_ack_info *act_ack_info)
{
	struct mac_ax_act_ack_info *ack_info = &adapter->nan_info.nan_act_ack_info;

	PLTFM_MEMCPY(act_ack_info, &adapter->nan_info.nan_act_ack_info,
		     sizeof(struct mac_ax_act_ack_info));
	PLTFM_MSG_TRACE("act ack id =  %d\n", ack_info->schedule_id);
	return MACSUCCESS;
}

u32 mac_check_cluster_info(struct mac_ax_adapter *adapter, struct mac_ax_nan_info *cluster_info)
{
	struct mac_ax_nan_info *ack_cluster_info = &adapter->nan_info;

	PLTFM_MEMCPY(cluster_info, &adapter->nan_info, sizeof(struct mac_ax_nan_info));
	PLTFM_MSG_TRACE("info ambtt =  %d\n", ack_cluster_info->rpt_ambtt);
	return MACSUCCESS;
}

u32 mac_nan_act_schedule_req(struct mac_ax_adapter *adapter, struct mac_ax_nan_sched_info *info)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_act_schedule_req *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_ACT_SCHEDULE_REQ;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_act_schedule_req);

	content = (struct fwcmd_act_schedule_req *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN act sch req h2c malloc content fail.\n");
		return MACBUFALLOC;
	}
	content->dword0 =
		cpu_to_le32(SET_WORD(info->module_id, FWCMD_H2C_ACT_SCHEDULE_REQ_MODULE_ID) |
			    SET_WORD(info->priority, FWCMD_H2C_ACT_SCHEDULE_REQ_PRIORITY) |
			    SET_WORD(info->options, FWCMD_H2C_ACT_SCHEDULE_REQ_OPTIONS) |
			    (info->faw_en ? FWCMD_H2C_ACT_SCHEDULE_REQ_FAW_EN : 0));

	content->dword1 =
		cpu_to_le32(SET_WORD(info->start_time, FWCMD_H2C_ACT_SCHEDULE_REQ_START_TIME));

	content->dword2 =
		cpu_to_le32(SET_WORD(info->duration, FWCMD_H2C_ACT_SCHEDULE_REQ_DURATION));

	content->dword3 =
		cpu_to_le32(SET_WORD(info->period, FWCMD_H2C_ACT_SCHEDULE_REQ_PERIOD));

	content->dword4 =
		cpu_to_le32(SET_WORD(info->tsf_idx, FWCMD_H2C_ACT_SCHEDULE_REQ_TSF_IDX) |
			SET_WORD(info->channel, FWCMD_H2C_ACT_SCHEDULE_REQ_CHANNEL) |
			SET_WORD(info->bw, FWCMD_H2C_ACT_SCHEDULE_REQ_BW) |
			SET_WORD(info->primary_ch_idx, FWCMD_H2C_ACT_SCHEDULE_REQ_PRIMARY_CH_IDX));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);

	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN act sch req h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_bcn_req(struct mac_ax_adapter *adapter, struct mac_ax_nan_bcn *info)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_bcn_req *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_BCN_REQ;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_bcn_req);

	content = (struct fwcmd_bcn_req *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN bcn req h2c malloc content fail.\n");
		return MACBUFALLOC;
	}

	content->dword0 =
		cpu_to_le32(SET_WORD(info->module_id, FWCMD_H2C_BCN_REQ_MODULE_ID) |
			    SET_WORD(info->bcn_intvl_ms, FWCMD_H2C_BCN_REQ_BCN_INTVL_MS) |
			    SET_WORD(info->priority, FWCMD_H2C_BCN_REQ_PRIORITY));

	content->dword1 =
		cpu_to_le32(SET_WORD(info->bcn_offset_us, FWCMD_H2C_BCN_REQ_BCN_OFFSET_US));

	content->dword2 =
		cpu_to_le32(SET_WORD(info->cur_tbtt, FWCMD_H2C_BCN_REQ_CUR_TBTT));

	content->dword3 =
		cpu_to_le32(SET_WORD(info->cur_tbtt_fr, FWCMD_H2C_BCN_REQ_CUR_TBTT_FR));

	content->dword4 =
		cpu_to_le32(SET_WORD(info->prohibit_before_ms,
				     FWCMD_H2C_BCN_REQ_PROHIBIT_BEFORE_MS) |
			    SET_WORD(info->prohibit_after_ms, FWCMD_H2C_BCN_REQ_PROHIBIT_AFTER_MS) |
			    SET_WORD(info->port_idx, FWCMD_H2C_BCN_REQ_PORT_IDX) |
			    SET_WORD(info->options, FWCMD_H2C_BCN_REQ_OPTIONS));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN bcn req h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_func_ctrl(struct mac_ax_adapter *adapter, struct mac_ax_nan_func_info *info)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_nan_func_ctrl *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_NAN_FUNC_CTRL;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_nan_func_ctrl);

	content = (struct fwcmd_nan_func_ctrl *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN function ctrl h2c malloc content fail.\n");
		return MACBUFALLOC;
	}
	content->dword0 =
		cpu_to_le32(SET_WORD(info->port_idx, FWCMD_H2C_NAN_FUNC_CTRL_PORT_IDX) |
			SET_WORD(info->master_pref, FWCMD_H2C_NAN_FUNC_CTRL_MASTER_PREF) |
			SET_WORD(info->random_factor, FWCMD_H2C_NAN_FUNC_CTRL_RANDOM_FACTOR));

	content->dword1 =
		cpu_to_le32(SET_WORD(info->op_ch_24g, FWCMD_H2C_NAN_FUNC_CTRL_OP_CH_24G) |
			SET_WORD(info->op_ch_5g, FWCMD_H2C_NAN_FUNC_CTRL_OP_CH_5G) |
			SET_WORD(info->options, FWCMD_H2C_NAN_FUNC_CTRL_OPTIONS));

	content->dword2 =
		cpu_to_le32(SET_WORD(info->time_indicate_period,
				     FWCMD_H2C_NAN_FUNC_CTRL_TIME_INDICATE_PERIOD) |
			SET_WORD(info->cluster_id[0], FWCMD_H2C_NAN_FUNC_CTRL_NAN_CLUSTER_ID0) |
			SET_WORD(info->cluster_id[1], FWCMD_H2C_NAN_FUNC_CTRL_NAN_CLUSTER_ID1) |
			SET_WORD(info->cluster_id[2], FWCMD_H2C_NAN_FUNC_CTRL_NAN_CLUSTER_ID2));

	content->dword3 =
		cpu_to_le32(SET_WORD(info->cluster_id[3], FWCMD_H2C_NAN_FUNC_CTRL_NAN_CLUSTER_ID3) |
			SET_WORD(info->cluster_id[4], FWCMD_H2C_NAN_FUNC_CTRL_NAN_CLUSTER_ID4) |
			SET_WORD(info->cluster_id[5], FWCMD_H2C_NAN_FUNC_CTRL_NAN_CLUSTER_ID5));

	content->dword4 =
		cpu_to_le32(SET_WORD(info->para_options, FWCMD_H2C_NAN_FUNC_CTRL_PARA_OPTIONS) |
			SET_WORD(info->fw_test_para_1, FWCMD_H2C_NAN_FUNC_CTRL_NAN_FW_TEST_PARA_1) |
			SET_WORD(info->fw_test_para_2, FWCMD_H2C_NAN_FUNC_CTRL_NAN_FW_TEST_PARA_2));
	content->dword5 =
		cpu_to_le32(SET_WORD(info->mac_id_bcn, FWCMD_H2C_NAN_FUNC_CTRL_MAC_ID_BCN) |
			SET_WORD(info->mac_id_mgn, FWCMD_H2C_NAN_FUNC_CTRL_MAC_ID_MGN));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN function ctrl h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_de_info(struct mac_ax_adapter *adapter, u8 status, u8 loc_bcast_sdf)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_nan_de_info *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_NAN_DE_INFO;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_nan_de_info);

	content = (struct fwcmd_nan_de_info *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN de info h2c malloc content fail.\n");
		return MACBUFALLOC;
	}

	content->dword0 =
		cpu_to_le32(SET_WORD(status, FWCMD_H2C_NAN_DE_INFO_STATUS) |
			    SET_WORD(loc_bcast_sdf, FWCMD_H2C_NAN_DE_INFO_LOC_BCAST_SDF));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN de info h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_join_cluster(struct mac_ax_adapter *adapter, u8 is_allow)
{
	u32 ret = MACSUCCESS;

	struct h2c_info h2c_info = {0};
	struct fwcmd_nan_join_cluster *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_NAN_JOIN_CLUSTER;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_nan_join_cluster);

	content = (struct fwcmd_nan_join_cluster *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN join cluster h2c malloc content fail.\n");
		return MACBUFALLOC;
	}

	content->dword0 =
		cpu_to_le32(SET_WORD(is_allow, FWCMD_H2C_NAN_JOIN_CLUSTER_IS_ALLOW));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN join cluster h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_pause_faw_tx(struct mac_ax_adapter *adapter, u32 id_map)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_pause_faw_tx *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_PAUSE_FAW_TX;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_pause_faw_tx);

	content = (struct  fwcmd_pause_faw_tx *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN pause faw tx h2c malloc content fail.\n");
		return MACBUFALLOC;
	}

	content->dword0 =
		cpu_to_le32(SET_WORD(id_map, FWCMD_H2C_PAUSE_FAW_TX_ID_MAP));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN pause faw tx h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_get_cluster_info(struct mac_ax_adapter *adapter,
			     struct mac_ax_nan_info *cluster_info)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_nan_get_cluster_info *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_NAN_GET_CLUSTER_INFO;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_nan_get_cluster_info);

	content = (struct fwcmd_nan_get_cluster_info *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN get cluster h2c malloc content fail.\n");
		return MACBUFALLOC;
	}
	content->dword0 = 0;

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN get cluster h2c TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}

u32 mac_nan_avail_t_bitmap(struct mac_ax_adapter *adapter,
			   struct mac_ax_nan_avail_t_bitmap_info *info)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_nan_avail_t_bitmap *content = NULL;

	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_NAN;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_NAN_AVAIL_T_BITMAP;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;
	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_nan_avail_t_bitmap);

	content = (struct fwcmd_nan_avail_t_bitmap *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content) {
		PLTFM_MSG_ERR("[ERR]NAN avail bitmap h2c malloc content fail.\n");
		return MACBUFALLOC;
	}
	content->dword0 =
		cpu_to_le32(SET_WORD(info->module_id,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_MODULE_ID) |
			    SET_WORD(info->option,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_OPTION) |
			    SET_WORD(info->start_offset_16tu,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_START_OFFSET_16TU));

	content->dword1 =
		cpu_to_le32(SET_WORD(info->period_tu,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_PERIOD_TU) |
			    SET_WORD(info->bit_duration_tu,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_BIT_DURATION_TU) |
			    SET_WORD(info->time_bitmap_len,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_TIME_BITMAP_LEN));

	content->dword2 =
		cpu_to_le32(SET_WORD(info->time_bitmap_0_3,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_TIME_BITMAP_0_3));

	content->dword3 =
		cpu_to_le32(SET_WORD(info->time_bitmap_4_7,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_TIME_BITMAP_4_7));

	content->dword4 =
		cpu_to_le32(SET_WORD(info->time_bitmap_8_11,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_TIME_BITMAP_8_11));

	content->dword5 =
		cpu_to_le32(SET_WORD(info->time_bitmap_12_15,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_TIME_BITMAP_12_15));

	content->dword6 =
		cpu_to_le32(SET_WORD(info->channel,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_CHANNEL) |
			    SET_WORD(info->bw,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_BW) |
			    SET_WORD(info->primary_ch_idx,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_PRIMARY_CH_IDX));

	content->dword7 =
		cpu_to_le32(SET_WORD(info->mac_id,
				     FWCMD_H2C_NAN_AVAIL_T_BITMAP_MAC_ID));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]NAN avail bitmap h2cc TX fail.\n");
		PLTFM_FREE(content, h2c_info.content_len);
		return ret;
	}

	PLTFM_FREE(content, h2c_info.content_len);
	return MACSUCCESS;
}
