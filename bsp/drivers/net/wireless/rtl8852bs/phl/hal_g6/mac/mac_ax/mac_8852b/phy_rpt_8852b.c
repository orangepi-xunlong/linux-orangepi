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

#include "phy_rpt_8852b.h"

#if MAC_AX_8852B_SUPPORT

#define MAC_AX_DISP_QID_HOST 0x2
#define MAC_AX_DISP_QID_WLCPU 0xB
#define MAC_AX_DISP_PID_HOST 0x0
#define MAC_AX_DISP_PID_WLCPU 0x0

u32 get_bbrpt_dle_cfg_8852b(struct mac_ax_adapter *adapter,
			    u8 is2wlcpu, u32 *port_id, u32 *queue_id)
{
	if (is2wlcpu) {
		*port_id = MAC_AX_DISP_PID_WLCPU;
		*queue_id = MAC_AX_DISP_QID_WLCPU;
	} else {
		*port_id = MAC_AX_DISP_PID_HOST;
		*queue_id = MAC_AX_DISP_QID_HOST;
	}

	return MACSUCCESS;
}

u32 mac_cfg_per_pkt_phy_rpt_8852b(struct mac_ax_adapter *adapter,
				  struct mac_ax_per_pkt_phy_rpt *rpt)
{
	return MACNOTSUP;
}

#endif /* #if MAC_AX_8852B_SUPPORT */
