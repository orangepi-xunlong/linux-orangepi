/** @file */
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
#ifndef _MAC_AX_WOWLAN_8852B_H_
#define _MAC_AX_WOWLAN_8852B_H_

#include "../../type.h"

#if MAC_AX_8852B_SUPPORT

u32 get_wake_reason_8852b(struct mac_ax_adapter *adapter, u8 *wowlan_wake_reason);

#endif

#endif // #define _MAC_AX_WOWLAN_8852B_H_
