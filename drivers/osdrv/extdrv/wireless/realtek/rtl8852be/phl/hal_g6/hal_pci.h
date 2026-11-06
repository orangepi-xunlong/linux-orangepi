/******************************************************************************
 *
 * Copyright(c) 2019 - 2023 Realtek Corporation.
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
#ifndef _HAL_PCI_H_
#define _HAL_PCI_H_

#ifdef CONFIG_PCI_HCI
void hal_pci_set_io_ops(struct rtw_hal_com_t *hal, struct hal_io_ops *pops);

#ifdef CONFIG_RTL8851B
#include "rtl8851b/rtl8851b.h"
#endif

#ifdef CONFIG_RTL8852A
#include "rtl8852a/rtl8852a.h"
#endif

#if defined(CONFIG_RTL8852B) || defined(CONFIG_RTL8852BP) || defined(CONFIG_RTL8852BT)
#include "rtl8852b/rtl8852b.h"
#endif

#ifdef CONFIG_RTL8852C
#include "rtl8852c/rtl8852c.h"
#endif

#ifdef CONFIG_RTL8852D
#include "rtl8852d/rtl8852d.h"
#endif

#if defined(CONFIG_RTL8832BR) || defined(CONFIG_RTL8192XB)
#include "rtl8192xb/rtl8192xb.h"
#endif

/* Times to polling RX DMA finish tag in RXBD info */
#ifndef PHL_DMA_NONCOHERENT
#define RX_TAG_POLLING_TIMES	(10000)
#else /* PHL_DMA_NONCOHERENT */
/* Cache handling for coherence takes time. Polling less */
#define RX_TAG_POLLING_TIMES	(100)
#endif /* PHL_DMA_NONCOHERENT */

static inline void hal_set_ops_pci(struct rtw_phl_com_t *phl_com,
					struct hal_info_t *hal)
{
#ifdef CONFIG_RTL8852A
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852A) {
		hal_set_ops_8852ae(phl_com, hal);
		hal_hook_trx_ops_8852ae(phl_com, hal);
	}
#endif

#if defined(CONFIG_RTL8852B) || defined(CONFIG_RTL8852BP) || defined(CONFIG_RTL8852BT)
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852B ||
	    hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852BP ||
		hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852BT) {
		hal_set_ops_8852be(phl_com, hal);
		hal_hook_trx_ops_8852be(phl_com, hal);
	}
#endif

#ifdef CONFIG_RTL8852C
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852C) {
		hal_set_ops_8852ce(phl_com, hal);
		hal_hook_trx_ops_8852ce(phl_com, hal);
	}
#endif

#ifdef CONFIG_RTL8852D
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852D) {
		hal_set_ops_8852de(phl_com, hal);
		hal_hook_trx_ops_8852de(phl_com, hal);
	}
#endif

#ifdef CONFIG_RTL8851B
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8851B) {
		hal_set_ops_8851be(phl_com, hal);
		hal_hook_trx_ops_8851be(phl_com, hal);
	}
#endif

#if defined(CONFIG_RTL8832BR) || defined(CONFIG_RTL8192XB)
	if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8192XB ||
	    hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8832BR) {
		hal_set_ops_8192xbe(phl_com, hal);
		hal_hook_trx_ops_8192xbe(phl_com, hal);
	}
#endif

	/*
	else if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8834A)
		rtl8834ae_set_hal_ops(hal);
	else if (hal_get_chip_id(hal->hal_com) == CHIP_WIFI6_8852B)
		rtl8852be_set_hal_ops(hal);
	*/
}
#endif /*CONFIG_PCI_HCI*/
#endif /*_HAL_PCI_H_*/
