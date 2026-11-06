/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
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
#define _RTL8852CE_OPS_C_
#include "../rtl8852c_hal.h"
#include "rtl8852ce.h"

void hal_set_ops_8852ce(struct rtw_phl_com_t *phl_com,
			struct hal_info_t *hal)
{
	struct hal_ops_t *ops = hal_get_ops(hal);

	hal_set_ops_8852c(phl_com, hal);

	ops->init_hal_spec = init_hal_spec_8852ce;
	ops->hal_get_efuse = hal_get_efuse_8852ce;
	ops->hal_init = hal_init_8852ce;
	ops->hal_deinit = hal_deinit_8852ce;
	ops->hal_start = hal_start_8852ce;
	ops->hal_stop = hal_stop_8852ce;
#ifdef CONFIG_WOWLAN
	ops->hal_wow_init = hal_wow_init_8852ce;
	ops->hal_wow_deinit = hal_wow_deinit_8852ce;
#endif /* CONFIG_WOWLAN */
	ops->hal_mp_init = hal_mp_init_8852ce;
	ops->hal_mp_deinit = hal_mp_deinit_8852ce;
	ops->hal_mp_path_chk = hal_mp_path_chk_8852ce;

	ops->hal_hci_configure = hal_hci_cfg_8852ce;
	ops->init_default_value = hal_init_default_value_8852ce;
	ops->init_int_default_value = hal_init_int_default_value_8852ce;
	ops->disable_interrupt_isr = hal_disable_int_isr_8852ce;
	ops->enable_interrupt = hal_enable_int_8852ce;
	ops->disable_interrupt = hal_disable_int_8852ce;
	ops->recognize_interrupt = hal_recognize_int_8852ce;
	ops->clear_interrupt = hal_clear_int_8852ce;
	ops->interrupt_handler = hal_int_hdler_8852ce;
	ops->restore_interrupt = hal_restore_int_8852ce;
	ops->restore_rx_interrupt = hal_rx_int_restore_8852ce;
#ifdef PHL_RXSC_ISR
	ops->check_rpq_isr = hal_rx_rpq_int_check_8852ce;
#endif
}

