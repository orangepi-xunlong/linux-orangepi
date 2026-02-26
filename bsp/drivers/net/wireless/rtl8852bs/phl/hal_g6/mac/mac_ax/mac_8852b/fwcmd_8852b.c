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

#include "../../type.h"
#include "../mac_priv.h"
#if MAC_AX_8852B_SUPPORT

static struct mac_ax_h2creg_offset h2creg_offset = {
	R_AX_H2CREG_DATA0, /* data0 */
	R_AX_H2CREG_DATA1, /* data1 */
	R_AX_H2CREG_DATA2, /* data2 */
	R_AX_H2CREG_DATA3, /* data3 */
	R_AX_H2CREG_CTRL, /* ctrl */
};

static struct mac_ax_c2hreg_offset c2hreg_offset = {
	R_AX_C2HREG_DATA0, /* data0 */
	R_AX_C2HREG_DATA1, /* data1 */
	R_AX_C2HREG_DATA2, /* data2 */
	R_AX_C2HREG_DATA3, /* data3 */
	R_AX_C2HREG_CTRL, /* ctrl */
};

struct mac_ax_h2creg_offset *
get_h2creg_offset_8852b(struct mac_ax_adapter *adapter)
{
	return &h2creg_offset;
}

struct mac_ax_c2hreg_offset *
get_c2hreg_offset_8852b(struct mac_ax_adapter *adapter)
{
	return &c2hreg_offset;
}
#endif /* #if MAC_AX_8852B_SUPPORT */
