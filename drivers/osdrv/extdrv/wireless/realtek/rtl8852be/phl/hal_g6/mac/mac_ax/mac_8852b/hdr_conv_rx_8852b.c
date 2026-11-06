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
#include "hdr_conv_rx_8852b.h"

#if MAC_AX_8852B_SUPPORT
u32 mac_hdr_conv_rx_en_8852b(struct mac_ax_adapter *adapter,
			     struct mac_ax_rx_hdr_conv_cfg *cfg)
{
	return MACNOTSUP;
}

u32 mac_hdr_conv_rx_en_driv_info_hdr_8852b(struct mac_ax_adapter *adapter,
					   struct mac_ax_rx_driv_info_hdr_cfg *cfg)
{
	return MACNOTSUP;
}
#endif
