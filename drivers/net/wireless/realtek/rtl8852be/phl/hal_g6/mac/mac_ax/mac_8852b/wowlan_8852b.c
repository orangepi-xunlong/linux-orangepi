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
#include "wowlan_8852b.h"
#include "../mac_priv.h"

#if MAC_AX_8852B_SUPPORT

u32 get_wake_reason_8852b(struct mac_ax_adapter *adapter, u8 *wowlan_wake_reason)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_c2hreg_offset *c2hreg_offset;

	c2hreg_offset = p_ops->get_c2hreg_offset(adapter);
	if (!c2hreg_offset) {
		PLTFM_MSG_ERR("%s: get c2hreg offset fail\n", __func__);
		return MACNPTR;
	}

	*wowlan_wake_reason = MAC_REG_R8(c2hreg_offset->data3 + 3);

	return MACSUCCESS;
}

#endif