/******************************************************************************
 *
 * Copyright(c)2023 Realtek Corporation.
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
 *****************************************************************************/
#ifndef _PHL_PS_DEF_H_
#define _PHL_PS_DEF_H_

#ifdef CONFIG_POWER_SAVE
enum phl_ps_mode {
	PS_MODE_NONE,
	PS_MODE_LPS,
	PS_MODE_IPS
};
#endif /* CONFIG_POWER_SAVE */
#endif /* _PHL_PS_DEF_H_ */
