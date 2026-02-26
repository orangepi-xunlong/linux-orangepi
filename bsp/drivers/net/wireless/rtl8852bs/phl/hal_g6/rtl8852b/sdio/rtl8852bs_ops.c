/******************************************************************************
 *
 * Copyright(c) 2016 - 2019 Realtek Corporation.
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
#define _RTL8852BS_OPS_C_
#include "../rtl8852b_hal.h"
#include "rtl8852bs.h"

void hal_set_ops_8852bs(struct rtw_phl_com_t *phl_com,
					struct hal_info_t *hal)
{
	struct hal_ops_t *ops = hal_get_ops(hal);

	hal_set_ops_8852b(phl_com, hal);

	ops->init_hal_spec = init_hal_spec_8852bs;
	ops->hal_get_efuse = hal_get_efuse_8852bs;
	ops->hal_init = hal_init_8852bs;
	ops->hal_deinit = hal_deinit_8852bs;
	ops->hal_start = hal_start_8852bs;
	ops->hal_stop = hal_stop_8852bs;
#ifdef CONFIG_WOWLAN
	ops->hal_wow_init = hal_wow_init_8852bs;
	ops->hal_wow_deinit = hal_wow_deinit_8852bs;
#endif /* CONFIG_WOWLAN */
	ops->hal_hci_configure = hal_hci_cfg_8852bs;
	ops->init_default_value = init_default_value_8852bs;
	ops->init_int_default_value = init_int_default_value_8852bs;

	ops->hal_mp_init = hal_mp_init_8852bs;
	ops->hal_mp_deinit = hal_mp_deinit_8852bs;
	ops->hal_mp_path_chk = hal_mp_path_chk_8852bs;

	ops->enable_interrupt = hal_enable_int_8852bs;
	ops->disable_interrupt = hal_disable_int_8852bs;
	ops->config_interrupt = hal_config_int_8852bs;
	ops->recognize_interrupt = hal_recognize_int_8852bs;
	ops->recognize_halt_c2h_interrupt = hal_recognize_halt_c2h_int_8852bs;
	ops->clear_interrupt = hal_clear_interrupt_8852bs;
	ops->interrupt_handler = hal_int_hdler_8852bs;
	ops->restore_interrupt = hal_enable_int_8852bs;
}

/*
 * This function copied from 8852bu, maybe we should refine it later...
 */
static u8 hal_mapping_hw_tx_chnl_8852bs(struct hal_info_t *hal,
				u16 macid, enum rtw_phl_ring_cat cat, u8 band)
{
	u8 dma_ch = 0;


	if (0 == band) {
		switch (cat) {
		case RTW_PHL_RING_CAT_TID0:/*AC_BE*/
		case RTW_PHL_RING_CAT_TID3:
		case RTW_PHL_RING_CAT_TID6:/*AC_VO*/
		case RTW_PHL_RING_CAT_TID7:
			dma_ch = ACH0_QUEUE_IDX_8852B;
			break;
		case RTW_PHL_RING_CAT_TID1:/*AC_BK*/
		case RTW_PHL_RING_CAT_TID2:
		case RTW_PHL_RING_CAT_TID4:/*AC_VI*/
		case RTW_PHL_RING_CAT_TID5:
			dma_ch = ACH2_QUEUE_IDX_8852B;
			break;
		case RTW_PHL_RING_CAT_MGNT:
			dma_ch = MGQ_B0_QUEUE_IDX_8852B;
			break;
		case RTW_PHL_RING_CAT_HIQ:
			dma_ch = HIQ_B0_QUEUE_IDX_8852B;
			break;
		default:
			dma_ch = ACH0_QUEUE_IDX_8852B;
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				  "[WARNING]unknown category (%d)\n", cat);
			break;
		}
	}else {
		dma_ch = ACH0_QUEUE_IDX_8852B;
		PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
			  "[WARNING]unknown band (%d)\n", band);
	}

	return dma_ch;
}

u16 hal_get_avail_page_8852bs(struct rtw_hal_com_t *hal_com, u8 dma_ch,
			u16 *host_idx, u16 *hw_idx)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;

	return hstatus;
}
static void hal_trx_deinit_8852bs(struct hal_info_t *hal)
{
}

enum rtw_hal_status hal_trx_init_8852bs(struct hal_info_t *hal)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;

	return hstatus;
}

static u8 hal_get_fwcmd_queue_idx_8852bs(void)
{
	return FWCMD_QUEUE_IDX_8852B;
}
static struct hal_trx_ops ops = {
	.init = hal_trx_init_8852bs,
	.deinit = hal_trx_deinit_8852bs,
	.query_tx_res = hal_get_avail_page_8852bs,
	.map_hw_tx_chnl = hal_mapping_hw_tx_chnl_8852bs,
	.get_fwcmd_queue_idx = hal_get_fwcmd_queue_idx_8852bs,
	.handle_rx_buffer = hal_handle_rx_buffer_8852b,
};

u32 hal_hook_trx_ops_8852bs(struct hal_info_t *hal_info)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;

	if (NULL != hal_info) {
		hal_info->trx_ops = &ops;
		hstatus = RTW_HAL_STATUS_SUCCESS;
	}

	return hstatus;
}
