/******************************************************************************
 *
 * Copyright(c)2019 Realtek Corporation.
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
#ifndef _HAL_SDIO_H_
#define _HAL_SDIO_H_

#ifdef CONFIG_SDIO_HCI
/* Follow mac team suggestion, default I/O fail return value is 0xFF */
void hal_sdio_set_io_ops(struct rtw_hal_com_t *h, struct hal_io_ops *pops);

#ifdef CONFIG_RTL8851B
#include "rtl8851b/rtl8851b.h"
#endif

#ifdef CONFIG_RTL8852A
#include "rtl8852a/rtl8852a.h"
#endif

#if defined(CONFIG_RTL8852B) || defined(CONFIG_RTL8852BP)
#include "rtl8852b/rtl8852b.h"
#endif

#ifdef CONFIG_RTL8852C
#include "rtl8852c/rtl8852c.h"
#endif

static inline void hal_set_ops_sdio(struct rtw_phl_com_t *phl_com,
						struct hal_info_t *hal)
{
	#ifdef CONFIG_RTL8852A
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852A) {
		hal_set_ops_8852as(phl_com, hal);
		hal_hook_trx_ops_8852as(hal);
	}
	#endif
	#ifdef CONFIG_RTL8852A
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8834A){
	}
	#endif

	#if defined(CONFIG_RTL8852B) || defined(CONFIG_RTL8852BP)
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852B ||
	    hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852BP) {
		hal_set_ops_8852bs(phl_com, hal);
		hal_hook_trx_ops_8852bs(hal);
	}
	#endif
	#ifdef CONFIG_RTL8851B
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8851B) {
		hal_set_ops_8851bs(phl_com, hal);
		hal_hook_trx_ops_8851bs(hal);
	}
#endif

}
#endif /*CONFIG_SDIO_HCI*/

#endif /* _HAL_SDIO_H_ */
