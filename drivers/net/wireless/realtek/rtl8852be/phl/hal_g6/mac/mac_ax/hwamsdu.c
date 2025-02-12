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

#include "hwamsdu.h"

#if MAC_AX_FW_REG_OFLD
u32 mac_enable_cut_hwamsdu(struct mac_ax_adapter *adapter,
			   u8 enable,
			   u8 low_th,
			   u16 high_th,
			   enum mac_ax_ex_shift aligned)
{
	u32 ret = 0;
	struct h2c_info h2c_info = {0};
	struct mac_ax_en_amsdu_cut *content;

	if (chk_patch_cut_amsdu_rls_ple_issue(adapter) == (u32)PATCH_ENABLE)
		return MACNOTSUP;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_shcut_update);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_FW_OFLD;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_AMSDU_CUT_REG;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	content = (struct mac_ax_en_amsdu_cut *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content)
		return MACBUFALLOC;
	content->enable = enable;
	content->low_th = low_th;
	content->high_th = high_th;
	content->aligned = aligned;

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);

	PLTFM_FREE(content, h2c_info.content_len);

	return ret;
}

u32 mac_enable_hwmasdu(struct mac_ax_adapter *adapter,
		       u8 enable,
		       enum mac_ax_amsdu_pkt_num max_num,
		       u8 en_single_amsdu,
		       u8 en_last_amsdu_padding)

{
	u32 ret = 0;
	struct h2c_info h2c_info = {0};
	struct mac_ax_en_hwamsdu *content;

	if (chk_patch_txamsdu_rls_wd_issue(adapter) == (u32)PATCH_ENABLE)
		return MACNOTSUP;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_shcut_update);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_FW_OFLD;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_HWAMSDU_REG;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	content = (struct mac_ax_en_hwamsdu *)PLTFM_MALLOC(h2c_info.content_len);
	if (!content)
		return MACBUFALLOC;
	content->enable = enable;
	content->max_num = max_num;
	content->en_single_amsdu = en_single_amsdu;
	content->en_last_amsdu_padding = en_last_amsdu_padding;

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)content);

	PLTFM_FREE(content, h2c_info.content_len);

	return ret;
}
#else

u32 mac_enable_hwamsdu(struct mac_ax_adapter *adapter,
		       u8 enable,
		       enum mac_ax_amsdu_pkt_num max_num,
		       u8 en_single_amsdu,
		       u8 en_last_amsdu_padding)
{
	u32 val;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_txamsdu_rls_wd_issue(adapter) == (u32)PATCH_ENABLE)
		return MACNOTSUP;

	if (max_num >= MAC_AX_AMSDU_AGG_NUM_MAX)
		return MACNOITEM;

	//HW AMSDU register
	val = MAC_REG_R32(R_AX_HWAMSDU_CTRL);
	val = (SET_CLR_WORD(val, max_num, B_AX_MAX_AMSDU_NUM) |
	       B_AX_HWAMSDU_EN);
	if (!enable)
		val &= ~B_AX_HWAMSDU_EN;
	MAC_REG_W32(R_AX_HWAMSDU_CTRL, val);

	MAC_REG_W32(R_AX_HWAMSDU_CTRL, (MAC_REG_R32(R_AX_HWAMSDU_CTRL) &
		    (~B_AX_SINGLE_AMSDU)) |
		    (en_single_amsdu ? B_AX_SINGLE_AMSDU : 0));

	MAC_REG_W32(R_AX_DMAC_TABLE_CTRL, (MAC_REG_R32(R_AX_DMAC_TABLE_CTRL) &
		    (~B_AX_HWAMSDU_PADDING_MODE)) |
		    (en_last_amsdu_padding ? B_AX_HWAMSDU_PADDING_MODE : 0));

	return MACSUCCESS;
}
#endif

u32 mac_hwamsdu_fwd_search_en(struct mac_ax_adapter *adapter, u8 enable)
{
	u32 val;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
		val = MAC_REG_R32(R_AX_HWAMSDU_CTRL);
		if (!enable)
			val &= ~B_AX_AMSDU_FS_ENABLE;
		else
			val |= B_AX_AMSDU_FS_ENABLE;

		MAC_REG_W32(R_AX_HWAMSDU_CTRL, val);
		return MACSUCCESS;
	} else {
		return MACNOTSUP;
	}
}

u32 mac_hwamsdu_macid_en(struct mac_ax_adapter *adapter, u8 macid, u8 enable)
{
	struct mac_ax_dctl_info info = {0};
	struct mac_ax_dctl_info mask = {0};
	struct mac_ax_ops *ops = adapter_to_mac_ops(adapter);
	u32 ret = 0;

	info.sta_amsdu_en = enable;
	mask.sta_amsdu_en = HW_AMSDU_MACID_ENABLE;
	ret = ops->upd_dctl_info(adapter, &info, &mask, macid, 1);

	return ret;
}

u8 mac_hwamsdu_get_macid_en(struct mac_ax_adapter *adapter, u8 macid)
{
	struct mac_ax_dctl_info info = {0};
	struct mac_ax_dctl_info mask = {0};
	struct mac_ax_ops *ops = adapter_to_mac_ops(adapter);
	u32 ret = 0;

	mask.sta_amsdu_en = HW_AMSDU_MACID_ENABLE;
	ret = ops->upd_dctl_info(adapter, &info, &mask, macid, 0);

	if (ret != MACSUCCESS)
		return 0;
	else
		return (u8)info.sta_amsdu_en;
}

u32 mac_hwamsdu_max_len(struct mac_ax_adapter *adapter, u8 macid, u8 amsdu_max_len)
{
	struct mac_ax_dctl_info info = {0};
	struct mac_ax_dctl_info mask = {0};
	struct mac_ax_ops *ops = adapter_to_mac_ops(adapter);
	u32 ret = 0;

	info.amsdu_max_length = amsdu_max_len;
	mask.amsdu_max_length = FWCMD_H2C_DCTRL_AMSDU_MAX_LEN_MSK;
	ret = ops->upd_dctl_info(adapter, &info, &mask, macid, 1);

	return ret;
}

u8 mac_hwamsdu_get_max_len(struct mac_ax_adapter *adapter, u8 macid)
{
	struct mac_ax_dctl_info info = {0};
	struct mac_ax_dctl_info mask = {0};
	struct mac_ax_ops *ops = adapter_to_mac_ops(adapter);
	u32 ret = 0;

	mask.amsdu_max_length = FWCMD_H2C_DCTRL_AMSDU_MAX_LEN_MSK;
	ret = ops->upd_dctl_info(adapter, &info, &mask, macid, 0);

	if (ret != MACSUCCESS)
		return 0;
	else
		return (u8)info.amsdu_max_length;
}
