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

#ifndef _MAC_AX_GPIO_8852B_H_
#define _MAC_AX_GPIO_8852B_H_

#include "../../type.h"
#if MAC_AX_8852B_SUPPORT

/**
 * @brief mac_pinmux_set_func_8852b
 *
 * @param *adapter
 * @param func
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_pinmux_set_func_8852b(struct mac_ax_adapter *adapter,
			      enum mac_ax_gpio_func func);

/**
 * @brief mac_gpio_init_8852b
 *
 * @param *adapter
 * @return Please Place Description here.
 * @retval u32
 */

u32 mac_gpio_init_8852b(struct mac_ax_adapter *adapter);

/**
 * @brief mac_set_gpio_func_8852b
 *
 * @param *adapter
 * @param func
 * @param gpio
 * @return Please Place Description here.
 * @retval u32
 */

u32 mac_set_gpio_func_8852b(struct mac_ax_adapter *adapter,
			    enum rtw_mac_gfunc func, s8 gpio);

/**
 * @brief mac_get_gpio_status_8852b
 *
 * @param *adapter
 * @param *func
 * @param gpio
 * @return Please Place Description here.
 * @retval u32
 */

u32 mac_get_gpio_status_8852b(struct mac_ax_adapter *adapter,
			      enum rtw_mac_gfunc *func, u8 gpio);

/**
 * @brief mac_cfg_wps_8852b
 *
 * @param *adapter
 * @param *wps
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_cfg_wps_8852b(struct mac_ax_adapter *adapter,
		      struct mac_ax_cfg_wps *wps);

#endif /* #if MAC_AX_8852B_SUPPORT */
#endif
