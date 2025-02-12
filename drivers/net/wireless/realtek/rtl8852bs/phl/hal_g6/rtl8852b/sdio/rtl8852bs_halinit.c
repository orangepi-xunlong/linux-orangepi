/******************************************************************************
 *
 * Copyright(c) 2019 - 2022 Realtek Corporation.
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
#define _RTL8852BS_HALINIT_C_
#include "../rtl8852b_hal.h"
#include "rtl8852bs_halinit.h"

static void _hal_pre_init_8852bs(struct rtw_phl_com_t *phl_com,
				 struct hal_info_t *hal_info,
				 struct hal_init_info_t *init_52bs)
{
	struct mac_ax_trx_info *trx_info = &init_52bs->trx_info;
	struct mac_ax_host_rpr_cfg *rpr_cfg = (struct mac_ax_host_rpr_cfg *)hal_info->rpr_cfg;
	/*struct mac_ax_intf_info *intf_info = &init_52bs->intf_info;*/

	if (true == phl_com->dev_cap.tx_mu_ru)
		trx_info->trx_mode = MAC_AX_TRX_SW_MODE;
	else
		trx_info->trx_mode = MAC_AX_TRX_HW_MODE;

	if (phl_com->dev_cap.logo_test == true) {
		trx_info->qta_mode = MAC_AX_QTA_SCC_LOGO;
	} else {
		if (phl_com->dev_cap.quota_turbo == true)
			trx_info->qta_mode = MAC_AX_QTA_SCC_TURBO;
		else
			trx_info->qta_mode = MAC_AX_QTA_SCC;
	}

	#ifdef RTW_WKARD_LAMODE
	PHL_INFO("%s : la_mode %d\n", __func__, phl_com->dev_cap.la_mode);
	if (phl_com->dev_cap.la_mode)
		trx_info->qta_mode = MAC_AX_QTA_LAMODE;
	#endif

	if (phl_com->dev_cap.rpq_agg_num) {
		rpr_cfg->agg_def = 0;
		rpr_cfg->agg = phl_com->dev_cap.rpq_agg_num;
	} else {
		rpr_cfg->agg_def = 1;
	}

	rpr_cfg->tmr_def = 1;
	rpr_cfg->txok_en = MAC_AX_FUNC_DEF;
	rpr_cfg->rty_lmt_en = MAC_AX_FUNC_DEF;
	rpr_cfg->lft_drop_en = MAC_AX_FUNC_DEF;
	rpr_cfg->macid_drop_en = MAC_AX_FUNC_DEF;

	trx_info->rpr_cfg = rpr_cfg;

	init_52bs->ic_name = "rtl8852bs";
}

void init_hal_spec_8852bs(struct rtw_phl_com_t *phl_com,
					struct hal_info_t *hal)
{
	struct bus_hw_cap_t *bus_hw = &hal->hal_com->bus_hw_cap;

	init_hal_spec_8852b(phl_com, hal);
	phl_com->dev_cap.hw_sup_flags |= HW_SUP_SDIO_MULTI_FUN;

	/* default TX/RX resouce setting */
	bus_hw->tx_buf_size = 20480;		/* 20KB */
	bus_hw->tx_buf_num = 8;
	bus_hw->tx_mgnt_buf_size = 3096;	/* 3KB */
	bus_hw->tx_mgnt_buf_num = 3;
	bus_hw->rx_buf_size = 30720;		/* 30KB */
	bus_hw->rx_buf_num = 8;

	hal->hal_com->dev_hw_cap.ps_cap.lps_pause_tx = true;
	phl_com->hal_spec.ser_cfg_int = false;
	phl_com->hal_spec.ps_cfg_int = false;
}

enum rtw_hal_status hal_get_efuse_8852bs(struct rtw_phl_com_t *phl_com,
					 struct hal_info_t *hal_info)
{
	struct hal_init_info_t init_52bs;

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52bs, 0, sizeof(init_52bs));
	_hal_pre_init_8852bs(phl_com, hal_info, &init_52bs);

	return hal_get_efuse_8852b(phl_com, hal_info, &init_52bs);
}

enum rtw_hal_status hal_init_8852bs(struct rtw_phl_com_t *phl_com,
				    struct hal_info_t *hal_info)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	/* allocate memory for hal */
	hal_info->rpr_cfg = _os_mem_alloc(phlcom_to_drvpriv(phl_com),
					  sizeof(struct mac_ax_host_rpr_cfg));
	if (hal_info->rpr_cfg == NULL) {
		hal_status = RTW_HAL_STATUS_RESOURCE;
		PHL_ERR("%s: alloc rpr_cfg failed\n", __func__);
		goto error_rpr_cfg;
	}

	hal_status = RTW_HAL_STATUS_SUCCESS;

error_rpr_cfg:
	return hal_status;
}

void hal_deinit_8852bs(struct rtw_phl_com_t *phl_com,
		       struct hal_info_t *hal_info)
{
	/* free memory for hal */
	_os_mem_free(phlcom_to_drvpriv(phl_com),
		     hal_info->rpr_cfg,
		     sizeof(struct mac_ax_host_rpr_cfg));
}

enum rtw_hal_status hal_start_8852bs(struct rtw_phl_com_t *phl_com,
				    struct hal_info_t *hal_info)
{
	struct hal_init_info_t init_52bs;

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52bs, 0, sizeof(init_52bs));
	_hal_pre_init_8852bs(phl_com, hal_info, &init_52bs);

	return hal_start_8852b(phl_com, hal_info, &init_52bs);
}

enum rtw_hal_status hal_stop_8852bs(struct rtw_phl_com_t *phl_com,
				      struct hal_info_t *hal)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = hal_stop_8852b(phl_com, hal);

#if 0
	#ifdef CONFIG_FWLPS_IN_IPS
	if (_SUCCESS == rtl8852bs_fw_ips_deinit(adapter))
		return _SUCCESS;
	#endif

	return hal_deinit_rtl8852b(adapter);
#endif
	return hal_status;
}

#ifdef CONFIG_WOWLAN
enum rtw_hal_status
hal_wow_init_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info,
						struct rtw_phl_stainfo_t *sta)
{
	struct hal_init_info_t init_52bs;
	struct mac_ax_trx_info *trx_info = &init_52bs.trx_info;

	_os_mem_set( hal_to_drvpriv(hal_info), &init_52bs, 0, sizeof(init_52bs));
	if (true == phl_com->dev_cap.tx_mu_ru)
		trx_info->trx_mode = MAC_AX_TRX_SW_MODE;
	else
		trx_info->trx_mode = MAC_AX_TRX_HW_MODE;

	if (phl_com->dev_cap.logo_test == true) {
		trx_info->qta_mode = MAC_AX_QTA_SCC_LOGO;
	} else {
		if (phl_com->dev_cap.quota_turbo == true)
			trx_info->qta_mode = MAC_AX_QTA_SCC_TURBO;
		else
			trx_info->qta_mode = MAC_AX_QTA_SCC;
	}

	init_52bs.ic_name = "rtl8852bs";

	return hal_wow_init_8852b(phl_com, hal_info, sta, &init_52bs);
}

enum rtw_hal_status
hal_wow_deinit_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info,
							struct rtw_phl_stainfo_t *sta)
{
	struct hal_init_info_t init_52bs;
	struct mac_ax_trx_info *trx_info = &init_52bs.trx_info;

	_os_mem_set( hal_to_drvpriv(hal_info), &init_52bs, 0, sizeof(init_52bs));
	if (true == phl_com->dev_cap.tx_mu_ru)
		trx_info->trx_mode = MAC_AX_TRX_SW_MODE;
	else
		trx_info->trx_mode = MAC_AX_TRX_HW_MODE;

	if (phl_com->dev_cap.logo_test == true) {
		trx_info->qta_mode = MAC_AX_QTA_SCC_LOGO;
	} else {
		if (phl_com->dev_cap.quota_turbo == true)
			trx_info->qta_mode = MAC_AX_QTA_SCC_TURBO;
		else
			trx_info->qta_mode = MAC_AX_QTA_SCC;
	}
	init_52bs.ic_name = "rtl8852bs";

	return hal_wow_deinit_8852b(phl_com, hal_info, sta, &init_52bs);
}
#endif /* CONFIG_WOWLAN */

void init_default_value_8852bs(struct hal_info_t *hal)
{
	init_default_value_8852b(hal);

	init_int_default_value_8852bs(hal, INT_SET_OPT_HAL_INIT);
}

void init_int_default_value_8852bs(struct hal_info_t *hal, enum rtw_hal_int_set_opt opt)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	hal_com->int_mask_default = (u32)(
#if 0
		B_AX_HC10ISR_IND_EN |
		B_AX_HC00ISR_IND_EN |
		B_AX_HD1ISR_IND_EN |
		B_AX_HD0ISR_IND_EN |
#endif
		B_AX_HS0ISR_IND_EN |
#if 0
		B_AX_BT_INT_EN |
		B_AX_SDIO_AVAL_INT_EN |
#endif
		B_AX_RX_REQUEST_INT_EN |
		0);

	hal_com->intr.halt_c2h_int.val_default = (u32)(
		B_AX_HALT_C2H_INT_EN |
		0);

	hal_com->intr.watchdog_timer_int.val_default = (u32)(
		B_AX_WDT_PTFM_INT_EN |
		0);

	hal_com->int_mask = hal_com->int_mask_default;

	hal_com->intr.halt_c2h_int.val_mask = hal_com->intr.halt_c2h_int.val_default;
	hal_com->intr.watchdog_timer_int.val_mask = hal_com->intr.watchdog_timer_int.val_default;

	PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
		  "Initialize interrupt mask: 0x%08lX, 0x%08x\n",
		  hal_com->int_mask, hal_com->intr.halt_c2h_int.val_mask);

}

u32 hal_hci_cfg_8852bs(struct rtw_phl_com_t *phl_com,
		struct hal_info_t *hal, struct rtw_ic_info *ic_info)
{
	/*struct rtw_hal_com_t *hal_com = hal->hal_com;*/

	/*sync SDIO Bus-info from os*/
	PHL_INFO("%s ===>\n", __func__);
	PHL_INFO("sdio clock: %d Hz\n", ic_info->sdio_info.clock);
	PHL_INFO("sdio V3 :%s\n", (ic_info->sdio_info.sd3_bus_mode) ? "YES" : "NO");


	rtw_hal_mac_sdio_cfg(phl_com, hal, ic_info);

	return RTW_HAL_STATUS_SUCCESS;
}

static void _hal_config_int_8852bs(struct hal_info_t *hal, enum rtw_hal_config_int hal_int_mode)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	long bit_of_rx_req = 0;

	switch (hal_int_mode) {
	case RTW_HAL_EN_DEFAULT_INT:
	hal_write32(hal_com, R_AX_SDIO_HIMR, hal_com->int_mask_default);
	hal_write32(hal_com, R_AX_HIMR0, hal_com->intr.halt_c2h_int.val_default);
	PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
		  "Enabled interrupt: 0x%04X=%08X, 0x%04X=%08X\n",
		  R_AX_SDIO_HIMR, hal_read32(hal_com, R_AX_SDIO_HIMR),
		  R_AX_HIMR0, hal_read32(hal_com, R_AX_HIMR0));
		break;
	case RTW_HAL_DIS_DEFAULT_INT:
		hal_write32(hal_com, R_AX_SDIO_HIMR, 0);
		hal_write32(hal_com, R_AX_HIMR0, 0);
		break;
	case RTW_HAL_STOP_RX_INT:
#ifndef CONFIG_PHL_SDIO_READ_RXFF_IN_INT
		if (_os_test_and_clear_bit(bit_of_rx_req, &hal_com->int_mask))
			hal_write32(hal_com, R_AX_SDIO_HIMR, (u32)hal_com->int_mask);
#endif
		break;
	case RTW_HAL_RESUME_RX_INT:
#ifndef CONFIG_PHL_SDIO_READ_RXFF_IN_INT
		if (!_os_test_and_set_bit(bit_of_rx_req, &hal_com->int_mask))
			hal_write32(hal_com, R_AX_SDIO_HIMR, (u32)hal_com->int_mask);
#endif
		break;
	case RTW_HAL_SER_HANDSHAKE_MODE:
		hal_write32(hal_com, R_AX_SDIO_HIMR, B_AX_HS0ISR_IND_EN);
		hal_write32(hal_com, R_AX_HIMR0, B_AX_HALT_C2H_INT_EN);
		break;
	case RTW_HAL_EN_HCI_INT:
		hal_write32(hal_com, R_AX_SDIO_HIMR, hal_com->int_mask_default);
		break;
	case RTW_HAL_DIS_HCI_INT:
		hal_write32(hal_com, R_AX_SDIO_HIMR, 0);
		break;
	default:
		break;
	}
}

void hal_config_int_8852bs(struct hal_info_t *hal, enum rtw_phl_config_int int_mode)
{
	u8 hal_int_mode = RTW_HAL_CONFIG_INT_MAX;

	switch (int_mode) {
	case RTW_PHL_STOP_RX_INT:
		hal_int_mode = RTW_HAL_STOP_RX_INT;
		break;
	case RTW_PHL_RESUME_RX_INT:
		hal_int_mode = RTW_HAL_RESUME_RX_INT;
		break;
	case RTW_PHL_SER_HANDSHAKE_MODE:
		hal_int_mode = RTW_HAL_SER_HANDSHAKE_MODE;
		break;
	case RTW_PHL_EN_HCI_INT:
		hal_int_mode = RTW_HAL_EN_HCI_INT;
		break;
	case RTW_PHL_DIS_HCI_INT:
		hal_int_mode = RTW_HAL_DIS_HCI_INT;
		break;
	default:
		PHL_ERR("%s: int_mode %d can't be supported!\n", __func__, int_mode);
		break;
	}

	if (hal_int_mode != RTW_HAL_CONFIG_INT_MAX)
		_hal_config_int_8852bs(hal, hal_int_mode);
}

void hal_enable_int_8852bs(struct hal_info_t *hal)
{
	_hal_config_int_8852bs(hal, RTW_HAL_EN_DEFAULT_INT);
}

void hal_disable_int_8852bs(struct hal_info_t *hal)
{
	_hal_config_int_8852bs(hal, RTW_HAL_DIS_DEFAULT_INT);
}

bool hal_recognize_int_8852bs(struct hal_info_t *hal)
{
	return true;
}

bool hal_recognize_halt_c2h_int_8852bs(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	struct hal_halt_c2h_int *g_hisr;

	g_hisr = &hal_com->intr.halt_c2h_int;
	g_hisr->intr = hal_read32(hal_com, R_AX_HISR0);
	g_hisr->intr &= g_hisr->val_mask;
	/* clear interrupt */
	if (g_hisr->intr)
		hal_write32(hal_com, R_AX_HISR0, g_hisr->intr);
	/* check halt c2h */
	if (g_hisr->intr & B_AX_HALT_C2H_INT_EN)
		return true;

	return false;
}

void hal_clear_interrupt_8852bs(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	u32 hisr; /* SDIO hisr */
	u32 g_hisr; /* general hisr */


	/* clear SDIO HISR */
	hisr = hal_read32(hal_com, R_AX_SDIO_HISR);
	hal_write32(hal_com, R_AX_SDIO_HISR, hisr);

	/* clear general HISR */
	g_hisr = hal_read32(hal_com, R_AX_HISR0);
	hal_write32(hal_com, R_AX_HISR0, g_hisr);
}

#define R_AX_SDIO_HISR_W1C_MASK	B_AX_BT_INT

u32 hal_int_hdler_8852bs(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	u32 hisr; /* SDIO hisr */
	u32 w1c; /* write 1 clear */
	u32 phl_int = 0;


	hisr = hal_read32(hal_com, R_AX_SDIO_HISR);
	hisr &= hal_com->int_mask;
	w1c = hisr & R_AX_SDIO_HISR_W1C_MASK;
	/* clear interrupt */
	if (w1c)
		hal_write32(hal_com, R_AX_SDIO_HISR, w1c);
	/* check interrupt */
	if (hisr & B_AX_RX_REQUEST_INT_EN){
		/* disable rx interrupt */
		_hal_config_int_8852bs(hal, RTW_HAL_STOP_RX_INT);
		phl_int |= BIT1;
	}
	if (hisr & B_AX_SDIO_AVAL_INT)
		phl_int |= BIT2;

	/* Check General interrupt */
	if (hisr & B_AX_HS0ISR_IND_EN) {
		_hal_config_int_8852bs(hal, RTW_HAL_DIS_HCI_INT);
		phl_int |= BIT6;
	}

	return phl_int;
}

enum rtw_hal_status
hal_mp_init_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_init_info_t init_52bs;

	FUNCIN_WSTS(hal_status);

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52bs, 0, sizeof(init_52bs));

	init_52bs.ic_name = "rtl8852bs";

	hal_status = hal_mp_init_8852b(phl_com, hal_info, &init_52bs);

	FUNCOUT_WSTS(hal_status);
	return hal_status;
}

enum rtw_hal_status
hal_mp_deinit_8852bs(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_init_info_t init_52bs;

	FUNCIN_WSTS(hal_status);

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52bs, 0, sizeof(init_52bs));

	init_52bs.ic_name = "rtl8852bs";

	hal_status = hal_mp_deinit_8852b(phl_com, hal_info, &init_52bs);

	if (RTW_HAL_STATUS_SUCCESS != hal_status) {

		PHL_ERR("hal_mp_deinit_8852bs: status = %u\n", hal_status);
		return hal_status;
	}

	FUNCOUT_WSTS(hal_status);
	return hal_status;
}

bool
hal_mp_path_chk_8852bs(struct rtw_phl_com_t *phl_com, u8 ant_tx, u8 cur_phy)
{
	if (phl_com->phy_cap[cur_phy].txss == 1 && ant_tx != RF_PATH_B)
		return false;
	else
		return true;
}
