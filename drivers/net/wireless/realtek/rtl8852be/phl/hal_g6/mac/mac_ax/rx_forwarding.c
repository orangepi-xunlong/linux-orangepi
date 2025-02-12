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
#include "rx_forwarding.h"

#if MAC_AX_FW_REG_OFLD
u32 mac_set_rx_forwarding(struct mac_ax_adapter *adapter,
			  struct mac_ax_rx_fwd_ctrl_t *rf_ctrl_p)
{
	u32 ret = 0;
	u8 *buf;
	struct h2c_info h2c_info = {0};
	struct fwcmd_rx_fwd *rx_fwd;
	struct mac_ax_af_ud_ctrl_t *af_ud;
	struct mac_ax_pm_cam_ctrl_t *pm_cam;

	if (!rf_ctrl_p)
		return MACNPTR;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_rx_fwd);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_FW_OFLD;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_RX_FWD;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	buf = PLTFM_MALLOC(sizeof(struct fwcmd_rx_fwd));
	if (!buf) {
		PLTFM_MSG_ERR("%s malloc h2c error\n", __func__);
		return MACNPTR;
	}

	rx_fwd = (struct fwcmd_rx_fwd *)buf;
	rx_fwd->dword0 =
	cpu_to_le32(SET_WORD(rf_ctrl_p->type, FWCMD_H2C_RX_FWD_TYPE) |
		    SET_WORD(rf_ctrl_p->frame, FWCMD_H2C_RX_FWD_FRAME) |
		    SET_WORD(rf_ctrl_p->fwd_tg, FWCMD_H2C_RX_FWD_FWD_TG));

	af_ud = &rf_ctrl_p->af_ud_ctrl;
	rx_fwd->dword1 =
	cpu_to_le32(SET_WORD(af_ud->index, FWCMD_H2C_RX_FWD_AF_UD_INDEX) |
		    SET_WORD(af_ud->fwd_tg, FWCMD_H2C_RX_FWD_AF_UD_FWD_TG) |
		    SET_WORD(af_ud->category, FWCMD_H2C_RX_FWD_AF_UD_CATEGORY) |
		    SET_WORD(af_ud->action_field,
			     FWCMD_H2C_RX_FWD_AF_UD_ACTION_FIELD));

	pm_cam = &rf_ctrl_p->pm_cam_ctrl;
	rx_fwd->dword2 =
	cpu_to_le32((pm_cam->valid ? FWCMD_H2C_RX_FWD_PM_CAM_VALID : 0) |
		    SET_WORD(pm_cam->type, FWCMD_H2C_RX_FWD_PM_CAM_TYPE) |
		    SET_WORD(pm_cam->subtype, FWCMD_H2C_RX_FWD_PM_CAM_SUBTYPE) |
		    (pm_cam->skip_mac_iv_hdr ?
		     FWCMD_H2C_RX_FWD_PM_CAM_SKIP_MAC_IV_HDR : 0) |
		    SET_WORD(pm_cam->target_ind,
			     FWCMD_H2C_RX_FWD_PM_CAM_TARGET_IND) |
		    SET_WORD(pm_cam->entry_index,
			     FWCMD_H2C_RX_FWD_PM_CAM_INDEX) |
		    SET_WORD(pm_cam->crc16, FWCMD_H2C_RX_FWD_PM_CAM_CRC16));

	rx_fwd->dword3 =
	cpu_to_le32(SET_WORD(pm_cam->pld_mask0,
			     FWCMD_H2C_RX_FWD_PM_CAM_PLD_MASK0));

	rx_fwd->dword4 =
	cpu_to_le32(SET_WORD(pm_cam->pld_mask1,
			     FWCMD_H2C_RX_FWD_PM_CAM_PLD_MASK1));

	rx_fwd->dword5 =
	cpu_to_le32(SET_WORD(pm_cam->pld_mask2,
			     FWCMD_H2C_RX_FWD_PM_CAM_PLD_MASK2));

	rx_fwd->dword6 =
	cpu_to_le32(SET_WORD(pm_cam->pld_mask3,
			     FWCMD_H2C_RX_FWD_PM_CAM_PLD_MASK3));

	if (adapter->sm.fwdl == MAC_AX_FWDL_INIT_RDY) {
		ret = mac_h2c_common(adapter, &h2c_info, (u32 *)rx_fwd);
		if (ret)
			goto fail;
	} else {
		ret = MACFWNONRDY;
		goto fail;
	}

	ret = MACSUCCESS;
fail:
	PLTFM_FREE(rx_fwd, sizeof(struct fwcmd_rx_fwd));

	return ret;
}

#else
static inline u32 af_fwd_cfg(struct mac_ax_adapter *adapter,
			     enum mac_ax_action_frame frame,
			     enum mac_ax_fwd_target fwd_tg)
{
	u32 val32;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	val32 = MAC_REG_R32(R_AX_ACTION_FWD0);
	switch (frame) {
	case MAC_AX_AF_CSA:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_CSA);
		break;
	case MAC_AX_AF_ADDTS_REQ:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_ADDTS_REQ);
		break;
	case MAC_AX_AF_ADDTS_RES:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_ADDTS_RES);
		break;
	case MAC_AX_AF_DELTS:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_DELTS);
		break;
	case MAC_AX_AF_ADDBA_REQ:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_ADDBA_REQ);
		break;
	case MAC_AX_AF_ADDBA_RES:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_ADDBA_RES);
		break;
	case MAC_AX_AF_DELBA:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_DELBA);
		break;
	case MAC_AX_AF_NCW:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_NCW);
		break;
	case MAC_AX_AF_GID_MGNT:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_GID_MGNT);
		break;
	case MAC_AX_AF_OP_MODE:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_OP_MODE);
		break;
	case MAC_AX_AF_CSI:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_CSI);
		break;
	case MAC_AX_AF_HT_CBFM:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_HT_CBFM);
		break;
	case MAC_AX_AF_VHT_CBFM:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_VHT_CBFM);
		break;
	default:
		return MACNOITEM;
	}
	MAC_REG_W32(R_AX_ACTION_FWD0, val32);
	return MACSUCCESS;
}

static inline u32 af_ud_fwd_cfg(struct mac_ax_adapter *adapter,
				struct mac_ax_af_ud_ctrl_t *af_ud_ctrl_p)
{
	u32 val32;
	u16 val16;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	val32 = MAC_REG_R32(R_AX_ACTION_FWD1);
	switch (af_ud_ctrl_p->index) {
	case MAC_AX_AF_UD_0:
		val16 = MAC_REG_R16(R_AX_ACTION_FWD_CTRL0);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->category,
				     B_AX_FWD_ACTN_CAT0);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->action_field,
				     B_AX_FWD_ACTN_ACTN0);
		MAC_REG_W16(R_AX_ACTION_FWD_CTRL0, val16);
		val32 = SET_CLR_WORD(val32, af_ud_ctrl_p->fwd_tg,
				     B_AX_FWD_ACTN_CTRL0);
		break;
	case MAC_AX_AF_UD_1:
		val16 = MAC_REG_R16(R_AX_ACTION_FWD_CTRL1);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->category,
				     B_AX_FWD_ACTN_CAT1);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->action_field,
				     B_AX_FWD_ACTN_ACTN1);
		MAC_REG_W16(R_AX_ACTION_FWD_CTRL1, val16);
		val32 = SET_CLR_WORD(val32, af_ud_ctrl_p->fwd_tg,
				     B_AX_FWD_ACTN_CTRL1);
		break;
	case MAC_AX_AF_UD_2:
		val16 = MAC_REG_R16(R_AX_ACTION_FWD_CTRL2);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->category,
				     B_AX_FWD_ACTN_CAT2);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->action_field,
				     B_AX_FWD_ACTN_ACTN2);
		MAC_REG_W16(R_AX_ACTION_FWD_CTRL2, val16);
		val32 = SET_CLR_WORD(val32, af_ud_ctrl_p->fwd_tg,
				     B_AX_FWD_ACTN_CTRL2);
		break;
	case MAC_AX_AF_UD_3:
		val16 = MAC_REG_R16(R_AX_ACTION_FWD_CTRL3);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->category,
				     B_AX_FWD_ACTN_CAT3);
		val16 = SET_CLR_WORD(val16, af_ud_ctrl_p->action_field,
				     B_AX_FWD_ACTN_ACTN3);
		MAC_REG_W16(R_AX_ACTION_FWD_CTRL3, val16);
		val32 = SET_CLR_WORD(val32, af_ud_ctrl_p->fwd_tg,
				     B_AX_FWD_ACTN_CTRL3);
		break;
	default:
		return MACNOITEM;
	}
	MAC_REG_W32(R_AX_ACTION_FWD1, val32);
	return MACSUCCESS;
}

static inline u32 tf_fwd_cfg(struct mac_ax_adapter *adapter,
			     enum mac_ax_trigger_frame frame,
			     enum mac_ax_fwd_target fwd_tg)
{
	u32 val32;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	val32 = MAC_REG_R32(R_AX_TF_FWD);
	switch (frame) {
	case MAC_AX_TF_BT:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF0);
		break;
	case MAC_AX_TF_BFRP:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF1);
		break;
	case MAC_AX_TF_MU_BAR:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF2);
		break;
	case MAC_AX_TF_MU_RTS:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF3);
		break;
	case MAC_AX_TF_BSRP:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF4);
		break;
	case MAC_AX_TF_GCR_MU_BAR:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF5);
		break;
	case MAC_AX_TF_BQRP:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF6);
		break;
	case MAC_AX_TF_NFRP:
		val32 = SET_CLR_WORD(val32, fwd_tg, B_AX_FWD_TF7);
		break;
	default:
		return MACNOITEM;
	}
	MAC_REG_W32(R_AX_TF_FWD, val32);
	return MACSUCCESS;
}

static inline u32 pm_cam_access_polling(struct mac_ax_adapter *adapter)
{
	u32 cnt = PM_CAM_WAIT_CNT;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	while (cnt--) {
		if (!(MAC_REG_R32(R_AX_PLD_CAM_ACCESS) & B_AX_PLD_CAM_POLL))
			break;
		PLTFM_DELAY_US(PM_CAM_WAIT_US);
	}

	if (!++cnt) {
		PLTFM_MSG_ERR("[ERR]PM CAM access timeout\n");
		return MACPOLLTO;
	}
	return MACSUCCESS;
}

static inline u32 pm_cam_indirect_r(struct mac_ax_adapter *adapter,
				    u32 offset)
{
	u32 val32;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	val32 = 0;
	val32 = SET_CLR_WORD(val32, offset, B_AX_PLD_CAM_OFFSET);
	val32 &= ~B_AX_PLD_CAM_CLR;
	val32 &= ~B_AX_PLD_CAM_RW;
	val32 |= B_AX_PLD_CAM_POLL;
	MAC_REG_W32(R_AX_PLD_CAM_ACCESS, val32);

	if (pm_cam_access_polling(adapter)) {
		PLTFM_MSG_ERR("[ERR]PM CAM read timeout\n");
		return MACSUCCESS;
	}

	return MAC_REG_R32(R_AX_PLD_CAM_RDATA);
}

static inline void pm_cam_indirect_w(struct mac_ax_adapter *adapter,
				     u32 offset, u32 data)
{
	u32 val32;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	MAC_REG_W32(R_AX_PLD_CAM_WDATA, data);

	val32 = 0;
	val32 = SET_CLR_WORD(val32, offset, B_AX_PLD_CAM_OFFSET);
	val32 &= ~B_AX_PLD_CAM_CLR;
	val32 |= B_AX_PLD_CAM_RW;
	val32 |= B_AX_PLD_CAM_POLL;
	MAC_REG_W32(R_AX_PLD_CAM_ACCESS, val32);

	if (pm_cam_access_polling(adapter))
		PLTFM_MSG_ERR("[ERR]PM CAM write timeout\n");
}

static inline u32 pm_cam_fwd_cfg(struct mac_ax_adapter *adapter,
				 struct mac_ax_pm_cam_ctrl_t *pm_cam_ctrl_p)
{
	u32 val32, offset;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
		PLTFM_MSG_ERR("[ERR]%s PM_CAM API is removed in 52C & 92XB & 51E & 52D.\n"
			      , __func__);
		return MACSUCCESS;
	}

	if (!pm_cam_ctrl_p)
		return MACNPTR;

	if (pm_cam_ctrl_p->entry_index >= MAC_AX_PM_CAM_ENTRY_NUM_MAX)
		return MACNOITEM;

	// Set as indirect access and enable PM CAM
	val32 = MAC_REG_R32(R_AX_PLD_CAM_CTRL);
	val32 = SET_CLR_WORD(val32, 0xf, B_AX_PLD_CAM_RANGE);
	val32 |= B_AX_PLD_CAM_ACC;
	val32 |= B_AX_PLD_CAM_EN;
	MAC_REG_W32(R_AX_PLD_CAM_CTRL, val32);

	// offset unit: 4 bytes
	offset = (pm_cam_ctrl_p->entry_index *
		 MAC_AX_PM_CAM_ENTRY_CONTENT_SIZE) / 4;

	val32 = 0;
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->pld_mask0, PM_CAM_PLD_MASK0);
	pm_cam_indirect_w(adapter, offset + PM_CAM_OFFSET_DW1, val32);
	if (val32 != pm_cam_indirect_r(adapter, offset + PM_CAM_OFFSET_DW1)) {
		PLTFM_MSG_ERR("[ERR]PM CAM cfg fail 1\n");
		return MACRFPMCAM;
	}

	val32 = 0;
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->pld_mask1, PM_CAM_PLD_MASK1);
	pm_cam_indirect_w(adapter, offset + PM_CAM_OFFSET_DW2, val32);
	if (val32 != pm_cam_indirect_r(adapter, offset + PM_CAM_OFFSET_DW2)) {
		PLTFM_MSG_ERR("[ERR]PM CAM cfg fail 2\n");
		return MACRFPMCAM;
	}

	val32 = 0;
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->pld_mask2, PM_CAM_PLD_MASK2);
	pm_cam_indirect_w(adapter, offset + PM_CAM_OFFSET_DW3, val32);
	if (val32 != pm_cam_indirect_r(adapter, offset + PM_CAM_OFFSET_DW3)) {
		PLTFM_MSG_ERR("[ERR]PM CAM cfg fail 3\n");
		return MACRFPMCAM;
	}

	val32 = 0;
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->pld_mask3, PM_CAM_PLD_MASK3);
	pm_cam_indirect_w(adapter, offset + PM_CAM_OFFSET_DW4, val32);
	if (val32 != pm_cam_indirect_r(adapter, offset + PM_CAM_OFFSET_DW4)) {
		PLTFM_MSG_ERR("[ERR]PM CAM cfg fail 4\n");
		return MACRFPMCAM;
	}

	// PMCAM is not trigger-type reg!
	val32 = 0;
	val32 |= (pm_cam_ctrl_p->valid ? PM_CAM_VALID : 0);
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->type, PM_CAM_TYPE);
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->subtype, PM_CAM_SUBTYPE);
	val32 |= (pm_cam_ctrl_p->skip_mac_iv_hdr ? PM_CAM_SKIP_MAC_IV_HDR : 0);
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->target_ind,
			     PM_CAM_TARGET_IND);
	val32 = SET_CLR_WORD(val32, pm_cam_ctrl_p->crc16, PM_CAM_CRC16);
	pm_cam_indirect_w(adapter, offset, val32);
	if (val32 != pm_cam_indirect_r(adapter, offset)) {
		PLTFM_MSG_ERR("[ERR]PM CAM cfg fail 5, %x, %x\n",
			      val32, pm_cam_indirect_r(adapter, offset));
		return MACRFPMCAM;
	}

	return MACSUCCESS;
}

u32 mac_set_rx_forwarding(struct mac_ax_adapter *adapter,
			  struct mac_ax_rx_fwd_ctrl_t *rf_ctrl_p)
{
	u32 ret = 0;

	if (!rf_ctrl_p) {
		PLTFM_MSG_ERR("[ERR]%s NULL pointer!\n", __func__);
		return MACNPTR;
	}

	switch (rf_ctrl_p->type) {
	case MAC_AX_FT_ACTION:
		ret = af_fwd_cfg(adapter,
				 (enum mac_ax_action_frame)rf_ctrl_p->frame,
				 (enum mac_ax_fwd_target)rf_ctrl_p->fwd_tg);
		break;
	case MAC_AX_FT_ACTION_UD:
		ret = af_ud_fwd_cfg(adapter, &rf_ctrl_p->af_ud_ctrl);
		break;
	case MAC_AX_FT_TRIGGER:
		ret = tf_fwd_cfg(adapter,
				 (enum mac_ax_trigger_frame)rf_ctrl_p->frame,
				 (enum mac_ax_fwd_target)rf_ctrl_p->fwd_tg);
		break;
	case MAC_AX_FT_PM_CAM:
		// Don't suggest using indirect access.
		ret = pm_cam_fwd_cfg(adapter, &rf_ctrl_p->pm_cam_ctrl);
		break;
	}

	return ret;
}

#endif
