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
#define _RTL8852CE_HALINIT_C_
#include "../rtl8852c_hal.h"
#include "hal_trx_8852ce.h"

static enum   mac_ax_pcie_func_ctrl
_hal_set_each_pcicfg(enum rtw_pcie_bus_func_cap_t ctrl)
{
	switch(ctrl) {

		case RTW_PCIE_BUS_FUNC_DISABLE:
			return MAC_AX_PCIE_DISABLE;
		case RTW_PCIE_BUS_FUNC_ENABLE:
			return MAC_AX_PCIE_ENABLE;
		default:
			return MAC_AX_PCIE_DEFAULT;
	}

}
enum rtw_hal_status _hal_set_pcicfg_8852ce(struct hal_info_t *hal_info)
{
	struct mac_ax_pcie_cfgspc_param pcicfg;
	enum rtw_hal_status hsts = RTW_HAL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;

	_os_mem_set(hal_to_drvpriv(hal_info), &pcicfg, 0, sizeof(pcicfg));
	pcicfg.write = 1;
	pcicfg.l0s_ctrl = _hal_set_each_pcicfg(hal_com->bus_cap.l0s_ctrl);
	pcicfg.l1_ctrl = _hal_set_each_pcicfg(hal_com->bus_cap.l1_ctrl);
	pcicfg.l1ss_ctrl = _hal_set_each_pcicfg(hal_com->bus_cap.l1ss_ctrl);
	pcicfg.wake_ctrl = _hal_set_each_pcicfg(hal_com->bus_cap.wake_ctrl);
	pcicfg.crq_ctrl = _hal_set_each_pcicfg(hal_com->bus_cap.crq_ctrl);
	pcicfg.clkdly_ctrl = hal_com->bus_cap.clkdly_ctrl;
	pcicfg.l0sdly_ctrl = hal_com->bus_cap.l0sdly_ctrl;
	pcicfg.l1dly_ctrl = hal_com->bus_cap.l1dly_ctrl;


	PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
		"%s : l0s/l1/l1ss/wake/crq/l0sdly/l1dly/clkdly = %#X/%#X/%#X/%#X/%#X/%#X/%#X/%#X \n",
		__func__, pcicfg.l0s_ctrl, pcicfg.l1_ctrl, pcicfg.l1ss_ctrl, pcicfg.wake_ctrl,
		pcicfg.crq_ctrl, pcicfg.l0sdly_ctrl, pcicfg.l1dly_ctrl, pcicfg.clkdly_ctrl);

	hsts = rtw_hal_mac_set_pcicfg(hal_info, &pcicfg);

	return hsts;
}

enum rtw_hal_status _hal_ltr_sw_init_state_8852ce(struct hal_info_t *hal_info)
{

	enum rtw_hal_status hsts = RTW_HAL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;
	enum rtw_pcie_ltr_state state;

	PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
		"%s :sw state = %u \n",__func__, hal_com->bus_cap.ltr_init_state);

	if (hal_com->bus_cap.ltr_init_state > 0) {

		state = hal_com->bus_cap.ltr_init_state;
		hsts = rtw_hal_mac_ltr_sw_trigger(hal_info,state);

	} else {

		hsts = RTW_HAL_STATUS_SUCCESS;
	}
	return hsts;
}

enum rtw_hal_status _hal_ltr_set_pcie_8852ce(struct hal_info_t *hal_info)
{
	enum rtw_hal_status hsts = RTW_HAL_STATUS_SUCCESS;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;
	struct bus_cap_t *bus_cap = &hal_com->bus_cap;

	PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
		"%s :idle_ctrl/idle_val/act_ctrl/act_val = %u/%#X/%u/%#X \n",
		__func__, bus_cap->ltr_idle.ctrl,
		bus_cap->ltr_idle.val, bus_cap->ltr_act.ctrl, bus_cap->ltr_act.val);

	if (bus_cap->ltr_idle.ctrl || bus_cap->ltr_act.ctrl) {
		hsts = rtw_hal_mac_ltr_set_pcie(hal_info, RTW_PCIE_BUS_FUNC_IGNORE,
				bus_cap->ltr_idle.ctrl, bus_cap->ltr_idle.val,
				bus_cap->ltr_act.ctrl, bus_cap->ltr_act.val);
	}

	return hsts;
}

static void _hal_aspm_hw_cfg(struct hal_info_t *hal)
{

	struct rtw_hal_com_t *hal_com = hal->hal_com;

	/* func */
	hal_com->bus_hw_cap.l0s_ctrl = RTW_PCIE_BUS_FUNC_DEFAULT;
	hal_com->bus_hw_cap.l1_ctrl = RTW_PCIE_BUS_FUNC_ENABLE;
	hal_com->bus_hw_cap.l1ss_ctrl = RTW_PCIE_BUS_FUNC_DEFAULT;
	hal_com->bus_hw_cap.wake_ctrl = RTW_PCIE_BUS_FUNC_DEFAULT;
	hal_com->bus_hw_cap.crq_ctrl = RTW_PCIE_BUS_FUNC_DEFAULT;

	/*delay*/
	hal_com->bus_hw_cap.l0sdly_ctrl = MAC_AX_PCIE_L0SDLY_DEF;
	hal_com->bus_hw_cap.l1dly_ctrl = MAC_AX_PCIE_L1DLY_DEF;
	hal_com->bus_hw_cap.clkdly_ctrl = MAC_AX_PCIE_CLKDLY_DEF;

}

static void _hal_tx_ch_config_8852ce(struct hal_info_t *hal)
{
	struct mac_ax_txdma_ch_map *txch_map = (struct mac_ax_txdma_ch_map *)hal->txch_map;

	txch_map->ch0 = MAC_AX_PCIE_ENABLE;
	txch_map->ch1 = MAC_AX_PCIE_ENABLE;
	txch_map->ch2 = MAC_AX_PCIE_ENABLE;
	txch_map->ch3 = MAC_AX_PCIE_ENABLE;
	txch_map->ch4 = MAC_AX_PCIE_ENABLE;
	txch_map->ch5 = MAC_AX_PCIE_ENABLE;
	txch_map->ch6 = MAC_AX_PCIE_ENABLE;
	txch_map->ch7 = MAC_AX_PCIE_ENABLE;
	txch_map->ch8 = MAC_AX_PCIE_ENABLE;
	txch_map->ch9 = MAC_AX_PCIE_ENABLE;
	txch_map->ch10 = MAC_AX_PCIE_ENABLE;
	txch_map->ch11 = MAC_AX_PCIE_ENABLE;
	txch_map->ch12 = MAC_AX_PCIE_ENABLE;
}

static void _hal_pre_init_8852ce(struct rtw_phl_com_t *phl_com,
				 struct hal_info_t *hal_info,
				 struct hal_init_info_t *init_52ce)
{
	struct mac_ax_trx_info *trx_info = &init_52ce->trx_info;
	struct mac_ax_intf_info *intf_info = &init_52ce->intf_info;
	struct mac_ax_host_rpr_cfg *rpr_cfg = (struct mac_ax_host_rpr_cfg *)hal_info->rpr_cfg;
	void *txbd_buf = NULL;
	void *rxbd_buf = NULL;

	/* trx_info */
	if (true == phl_com->dev_cap.tx_mu_ru)
		trx_info->trx_mode = MAC_AX_TRX_SW_MODE;
	else
		trx_info->trx_mode = MAC_AX_TRX_HW_MODE;

	if (hal_info->hal_com->dbcc_en == false)
		trx_info->qta_mode = MAC_AX_QTA_SCC;
	else
		trx_info->qta_mode = MAC_AX_QTA_DBCC;

#ifdef RTW_WKARD_LAMODE
	PHL_INFO("%s : la_mode %d\n", __func__,	phl_com->dev_cap.la_mode);
	if (phl_com->dev_cap.la_mode)
		trx_info->qta_mode = MAC_AX_QTA_LAMODE;
#endif
	/* Calculate max release report aggregation number that could fit
	 * into RPQ buffer */
	if (hal_info->hal_com->bus_cap.rpbuf_size) {
		s32 agg =   hal_info->hal_com->bus_cap.rpbuf_size
			  - RX_BD_INFO_SIZE
		          - RX_DESC_S_SIZE_8852C;

		if (agg <= 0)
			PHL_ERR("RP buffer is too small!(%u)\n",
				hal_info->hal_com->bus_cap.rpbuf_size);
		agg = (agg / RX_RP_PACKET_SIZE) - 1; /* set N - 1 to agg N. */
		/* Not exceed HALMAC's default 121 */
		if (agg > 121)
			agg = 121;
		/* Use maximum number fit into RPQ if not set or set to too
		   large */
		if ((phl_com->dev_cap.rpq_agg_num == 0)
		    || (phl_com->dev_cap.rpq_agg_num > (u8)agg)) {
			PHL_INFO("RP agg set to %d for %uB (orig is %u).\n",
			         agg, hal_info->hal_com->bus_cap.rpbuf_size,
			         phl_com->dev_cap.rpq_agg_num);

			phl_com->dev_cap.rpq_agg_num = (u8)agg;
		}
	}

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

	/* intf_info */
	txbd_buf = rtw_phl_get_txbd_buf(phl_com);
	rxbd_buf = rtw_phl_get_rxbd_buf(phl_com);

	intf_info->txbd_trunc_mode = MAC_AX_BD_TRUNC;
	intf_info->rxbd_trunc_mode = MAC_AX_BD_TRUNC;

	intf_info->rxbd_mode = MAC_AX_RXBD_PKT;
	intf_info->tag_mode = MAC_AX_TAG_MULTI;
	intf_info->tx_burst = MAC_AX_TX_BURST_DEF;
	intf_info->rx_burst = MAC_AX_RX_BURST_DEF;
	intf_info->wd_dma_idle_intvl = MAC_AX_WD_DMA_INTVL_DEF;
	intf_info->wd_dma_act_intvl = MAC_AX_WD_DMA_INTVL_DEF;
	intf_info->multi_tag_num = MAC_AX_TAG_NUM_DEF;
	intf_info->rx_sep_append_len = 0;
	intf_info->txbd_buf = txbd_buf;
	intf_info->rxbd_buf = rxbd_buf;
	intf_info->skip_all = false;
	intf_info->autok_en = MAC_AX_PCIE_DISABLE;

	intf_info->txbd_num = (u16)hal_info->hal_com->bus_cap.txbd_num;
	intf_info->rxbd_num = (u16)hal_info->hal_com->bus_cap.rxbd_num;
	intf_info->rpbd_num = (u16)hal_info->hal_com->bus_cap.rpbd_num;

	_hal_tx_ch_config_8852ce(hal_info);
	intf_info->txch_map = (struct mac_ax_txdma_ch_map *)hal_info->txch_map;

	intf_info->lbc_en = MAC_AX_PCIE_DEFAULT;
	intf_info->lbc_tmr = MAC_AX_LBC_TMR_DEF;

	intf_info->io_rcy_en = MAC_AX_PCIE_DEFAULT;
	intf_info->io_rcy_tmr = MAC_AX_IO_RCY_ANA_TMR_DEF;

	/* others */
	init_52ce->ic_name = "rtl8852ce";
}

void init_hal_spec_8852ce(struct rtw_phl_com_t *phl_com,
					struct hal_info_t *hal)
{
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_com);
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	struct bus_hw_cap_t *bus_hw_cap = &hal_com->bus_hw_cap;

	init_hal_spec_8852c(phl_com, hal);
	hal_spec->rx_bd_info_sz = RX_BD_INFO_SIZE;
	hal_spec->rx_tag[0] = 0;
	hal_spec->rx_tag[1] = 0;

	bus_hw_cap->max_txbd_num = 0x3FF;
	bus_hw_cap->max_rxbd_num = 0x3FF;
	bus_hw_cap->max_rpbd_num = 0x3FF;
	bus_hw_cap->max_rxbuf_size = RX_BUF_SIZE;
	bus_hw_cap->max_rpbuf_size = RX_BUF_SIZE;
	bus_hw_cap->max_wd_page_size = 128;
	bus_hw_cap->wdb_size = 32;
	bus_hw_cap->wdi_size = 24;
	bus_hw_cap->txbd_len = 8;
	bus_hw_cap->rxbd_len = 8;
	bus_hw_cap->addr_info_size = 6;
	bus_hw_cap->seq_info_size = 8;
	/* phyaddr num = (wd page - (wdb + wdi + seqinfo)) / addrinfo */
#ifdef RTW_WKARD_BUSCAP_IN_HALSPEC
	hal_spec->phyaddr_num = 10;
#endif
	hal_spec->addr_info_len_lmt = 2047;

	phl_com->dev_cap.hw_sup_flags |= HW_SUP_PCIE_PLFH;/*PCIe payload from host*/

	_hal_aspm_hw_cfg(hal);

	/* temp */
	bus_hw_cap->ltr_sw_ctrl = false;
	bus_hw_cap->ltr_hw_ctrl = true;

	hal_com->dev_hw_cap.ps_cap.lps_pause_tx = false;
	hal_spec->ser_cfg_int = true;
	hal_spec->ps_cfg_int = true;
}

enum rtw_hal_status hal_get_efuse_8852ce(struct rtw_phl_com_t *phl_com,
					 struct hal_info_t *hal_info)
{
	struct hal_init_info_t init_52ce;
	_os_mem_set(hal_to_drvpriv(hal_info), &init_52ce, 0, sizeof(init_52ce));
	_hal_pre_init_8852ce(phl_com, hal_info, &init_52ce);

	return hal_get_efuse_8852c(phl_com, hal_info, &init_52ce);
}

enum rtw_hal_status hal_init_8852ce(struct rtw_phl_com_t *phl_com,
				    struct hal_info_t *hal_info)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;
	void *drv = phlcom_to_drvpriv(phl_com);
	u32 len = 0;
	u8 txch_num = 0;

	txch_num = rtw_hal_query_txch_num(hal_info);

	len = sizeof(struct rtw_wp_rpt_stats) * txch_num;
	hal_com->trx_stat.wp_rpt_stats = _os_mem_alloc(drv, len);
	if (hal_com->trx_stat.wp_rpt_stats == NULL) {
		hal_status = RTW_HAL_STATUS_RESOURCE;
		PHL_ERR("%s: alloc txch_map failed\n", __func__);
		goto error_trx_stats_wp_rpt;
	}

	hal_info->txch_map = _os_mem_alloc(drv,
					   sizeof(struct mac_ax_txdma_ch_map));
	if (hal_info->txch_map == NULL) {
		hal_status = RTW_HAL_STATUS_RESOURCE;
		PHL_ERR("%s: alloc txch_map failed\n", __func__);
		goto error_txch_map;
	}

	hal_info->rpr_cfg = _os_mem_alloc(drv,
					  sizeof(struct mac_ax_host_rpr_cfg));
	if (hal_info->rpr_cfg == NULL) {
		hal_status = RTW_HAL_STATUS_RESOURCE;
		PHL_ERR("%s: alloc rpr_cfg failed\n", __func__);
		goto error_rpr_cfg;
	}

	hal_status = RTW_HAL_STATUS_SUCCESS;

	return hal_status;

error_rpr_cfg:
	_os_mem_free(drv,
		     hal_info->txch_map,
		     sizeof(struct mac_ax_txdma_ch_map));
error_txch_map:
	_os_mem_free(drv,
		     hal_com->trx_stat.wp_rpt_stats,
		     sizeof(struct rtw_wp_rpt_stats) * txch_num);
error_trx_stats_wp_rpt:
	return hal_status;
}

void hal_deinit_8852ce(struct rtw_phl_com_t *phl_com,
		       struct hal_info_t *hal_info)
{
	u8 txch_num = 0;

	txch_num = rtw_hal_query_txch_num(hal_info);

	_os_mem_free(phlcom_to_drvpriv(phl_com),
		     hal_info->hal_com->trx_stat.wp_rpt_stats,
		     sizeof(struct rtw_wp_rpt_stats) * txch_num);
	_os_mem_free(phlcom_to_drvpriv(phl_com),
		     hal_info->txch_map,
		     sizeof(struct mac_ax_txdma_ch_map));
	_os_mem_free(phlcom_to_drvpriv(phl_com),
		     hal_info->rpr_cfg,
		     sizeof(struct mac_ax_host_rpr_cfg));
}

enum rtw_hal_status hal_start_8852ce(struct rtw_phl_com_t *phl_com,
				    struct hal_info_t *hal_info)
{
	struct hal_init_info_t init_52ce;
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = hal_info->hal_com;

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52ce, 0, sizeof(init_52ce));
	_hal_pre_init_8852ce(phl_com, hal_info, &init_52ce);

	hal_status = hal_start_8852c(phl_com, hal_info, &init_52ce);
	if(RTW_HAL_STATUS_SUCCESS != hal_status) {

		PHL_ERR("hal_init_8852c: status = %u\n",hal_status);
		return hal_status;
	}

	hal_status = _hal_set_pcicfg_8852ce(hal_info);
	if(RTW_HAL_STATUS_SUCCESS != hal_status) {
		PHL_ERR("_hal_set_pcicfg_8852ce: status = %u\n",hal_status);
		return hal_status;
	}

	hal_status = _hal_ltr_set_pcie_8852ce(hal_info);
	if(RTW_HAL_STATUS_SUCCESS != hal_status) {
		PHL_ERR("_hal_ltr_set_pcie_8852ce: status = %u\n",hal_status);
		return hal_status;
	}

	hal_status = _hal_ltr_sw_init_state_8852ce(hal_info);
	if(RTW_HAL_STATUS_SUCCESS != hal_status) {
		PHL_ERR("_hal_ltr_sw_init_state_8852ce: status = %u\n",hal_status);
		return hal_status;
	}

#ifdef PHL_DMA_ADDR_64
	/* init default 64bit WP mapping */
	hal_write32(hal_com, R_AX_WP_ADDR_H_SEL0_3, 0x03020100);
	hal_write32(hal_com, R_AX_WP_ADDR_H_SEL4_7, 0x07060504);
	hal_write32(hal_com, R_AX_WP_ADDR_H_SEL8_11, 0x0B0A0908);
	hal_write32(hal_com, R_AX_WP_ADDR_H_SEL12_15, 0x0F0E0D0C);
#endif

	return hal_status;
}

enum rtw_hal_status hal_stop_8852ce(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	hal_status = hal_stop_8852c(phl_com, hal);
	return hal_status;
}

#ifdef CONFIG_WOWLAN

enum rtw_hal_status
hal_wow_init_8852ce(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info,
							struct rtw_phl_stainfo_t *sta)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_init_info_t init_52ce;
	struct mac_ax_trx_info *trx_info = &init_52ce.trx_info;

	FUNCIN_WSTS(hal_status);

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52ce, 0, sizeof(init_52ce));
	if (true == phl_com->dev_cap.tx_mu_ru)
		trx_info->trx_mode = MAC_AX_TRX_SW_MODE;
	else
		trx_info->trx_mode = MAC_AX_TRX_HW_MODE;
	trx_info->qta_mode = MAC_AX_QTA_SCC;
	/*
	if (hal_info->hal_com->dbcc_en == false)
		trx_info->qta_mode = MAC_AX_QTA_SCC;
	else
		trx_info->qta_mode = MAC_AX_QTA_DBCC;
	*/
	init_52ce.ic_name = "rtl8852ce";

	hal_status = hal_wow_init_8852c(phl_com, hal_info, sta, &init_52ce);

	FUNCOUT_WSTS(hal_status);
	return hal_status;
}

enum rtw_hal_status
hal_wow_deinit_8852ce(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info,
								struct rtw_phl_stainfo_t *sta)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_init_info_t init_52ce;
	struct mac_ax_trx_info *trx_info = &init_52ce.trx_info;

	FUNCIN_WSTS(hal_status);

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52ce, 0, sizeof(init_52ce));
	if (true == phl_com->dev_cap.tx_mu_ru)
		trx_info->trx_mode = MAC_AX_TRX_SW_MODE;
	else
		trx_info->trx_mode = MAC_AX_TRX_HW_MODE;
	trx_info->qta_mode = MAC_AX_QTA_SCC;
	/*
	if (hal_info->hal_com->dbcc_en == false)
		trx_info->qta_mode = MAC_AX_QTA_SCC;
	else
		trx_info->qta_mode = MAC_AX_QTA_DBCC;
	*/
	init_52ce.ic_name = "rtl8852ce";

	hal_status = hal_wow_deinit_8852c(phl_com, hal_info, sta, &init_52ce);

	if (RTW_HAL_STATUS_SUCCESS != hal_status) {

		PHL_ERR("hal_wow_deinit_8852ce: status = %u\n", hal_status);
		return hal_status;
	}

	if (RTW_HAL_STATUS_SUCCESS != _hal_set_pcicfg_8852ce(hal_info))
		PHL_ERR("_hal_set_pcicfg_8852ce: status = %u\n", hal_status);


	FUNCOUT_WSTS(hal_status);
	return hal_status;
}

#endif /* CONFIG_WOWLAN */

u32 hal_hci_cfg_8852ce(struct rtw_phl_com_t *phl_com,
		struct hal_info_t *hal, struct rtw_ic_info *ic_info)
{
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_com);
	hal_spec->cts2_thres_en = true;
	hal_spec->cts2_thres = 1792;

	hal_spec->txbd_multi_tag = 8;
#ifdef RTW_WKARD_TXBD_UPD_LMT
	hal_spec->txbd_upd_lmt = true;
#else
	hal_spec->txbd_upd_lmt = false;
#endif
	return RTW_HAL_STATUS_SUCCESS;
}

static void _hal_dump_int_8852ce(struct hal_info_t *hal, bool mask, bool imr, bool isr, const char *func)
{
	struct rtw_hal_com_t *hal_com = NULL;

	hal_com = hal->hal_com;

	PHL_INFO("%s, %s\n", __func__, func);

	if (mask) {
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "mask: 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
			 hal_com->_intr_ind[0].mask, hal_com->_intr[0].mask,
			 hal_com->_intr[1].mask, hal_com->_intr[2].mask, hal_com->_intr[3].mask);
	}

	if (imr) {
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "imr: 0x%04X : 0x%08X, 0x%04X : 0x%08X, 0x%04X : 0x%08X, 0x%04X : 0x%08X, 0x%04X : 0x%08X\n",
			  R_AX_PCIE_HIMR00_V1, (hal_com->_intr_ind[0].en ? hal_read32(hal_com, R_AX_PCIE_HIMR00_V1) : 0xEAEAEAEA),
			  R_AX_HAXI_HIMR00, (hal_com->_intr[0].en ? hal_read32(hal_com, R_AX_HAXI_HIMR00) : 0xEAEAEAEA),
			  R_AX_HAXI_HIMR10, (hal_com->_intr[1].en ? hal_read32(hal_com, R_AX_HAXI_HIMR10) : 0xEAEAEAEA),
			  R_AX_HIMR0, (hal_com->_intr[2].en ? hal_read32(hal_com, R_AX_HIMR0) : 0xEAEAEAEA),
			  R_AX_HIMR1, (hal_com->_intr[3].en ? hal_read32(hal_com, R_AX_HIMR1) : 0xEAEAEAEA));
	}

	if (isr) {
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "isr: 0x%04X : 0x%08X, 0x%04X : 0x%08X, 0x%04X : 0x%08X, 0x%04X : 0x%08X, 0x%04X : 0x%08X\n",
			  R_AX_PCIE_HISR00_V1, (hal_com->_intr_ind[0].en ? hal_read32(hal_com, R_AX_PCIE_HISR00_V1) : 0xEAEAEAEA),
			  R_AX_HAXI_HISR00, (hal_com->_intr[0].en ? hal_read32(hal_com, R_AX_HAXI_HISR00) : 0xEAEAEAEA),
			  R_AX_HAXI_HISR10, (hal_com->_intr[1].en ? hal_read32(hal_com, R_AX_HAXI_HISR10) : 0xEAEAEAEA),
			  R_AX_HISR0, (hal_com->_intr[2].en ? hal_read32(hal_com, R_AX_HISR0) : 0xEAEAEAEA),
			  R_AX_HISR1, (hal_com->_intr[3].en ? hal_read32(hal_com, R_AX_HISR1) : 0xEAEAEAEA));
	}
}

#define HAL_DUMP_INT_8852CE(hal, mask, imr, isr) \
		_hal_dump_int_8852ce(hal, mask, imr, isr, __func__)

void hal_init_default_value_8852ce(struct hal_info_t *hal)
{
	init_default_value_8852c(hal);

	hal_init_int_default_value_8852ce(hal, INT_SET_OPT_HAL_INIT);
}

void hal_init_int_default_value_8852ce(struct hal_info_t *hal, enum rtw_hal_int_set_opt opt)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

#ifdef RTW_WKARD_RESET_LPS_STS
	if (opt == INT_SET_OPT_HAL_INIT)
		hal_com->in_ps_intr_cfg = false;
#endif /* RTW_WKARD_RESET_LPS_STS */

	if (opt == INT_SET_OPT_PS_START)
		hal_com->in_ps_intr_cfg = true;
	else if (opt == INT_SET_OPT_PS_STOP)
		hal_com->in_ps_intr_cfg = false;

	/* R_AX_PCIE_HIMR00_V1 */
	hal_com->_intr_ind[0].mask_dflt = (u32)(
		((opt == INT_SET_OPT_SER_START ||
		  hal_com->in_ps_intr_cfg) ? 0 : B_AX_HCI_AXIDMA_INT_EN) |
		(hal_com->in_ps_intr_cfg ? B_AX_HS1ISR_IND_INT_EN : 0) |
		B_AX_HS0ISR_IND_INT_EN |
		0);
	hal_com->_intr_ind[0].en = true;
	hal_com->_intr_ind[0].val = 0;

	/* R_AX_HAXI_HIMR00 */
	hal_com->_intr[0].mask_dflt = (u32)(
		B_AX_RPQBD_FULL_INT_EN | /* RPQBD full */
		B_AX_RDU_INT_EN | /* Rx Descriptor Unavailable */
		B_AX_RPQDMA_INT_EN | /* RPQ DMA OK */
		B_AX_RXDMA_INT_EN | /* RXQ (Packet mode or Part2) DMA OK */
		0);
	hal_com->_intr[0].en = ((opt == INT_SET_OPT_SER_START ||
							 hal_com->in_ps_intr_cfg) ? false : true);
	hal_com->_intr[0].val = 0;

	/* R_AX_HAXI_HIMR10 */
	hal_com->_intr[1].mask_dflt = (u32)(
		0);
	hal_com->_intr[1].en = ((opt == INT_SET_OPT_SER_START ||
							 hal_com->in_ps_intr_cfg) ? false : false);
	hal_com->_intr[1].val = 0;

	/* R_AX_HIMR0 */
	hal_com->_intr[2].mask_dflt = (u32)(
		B_AX_HALT_C2H_INT_EN |
		(hal_com->cv == CAV ? (hal_com->in_ps_intr_cfg ?
		0 : B_AX_WDT_TIMEOUT_INT_EN) : B_AX_WDT_TIMEOUT_INT_EN)
		| 0);
	hal_com->_intr[2].en = true;
	hal_com->_intr[2].val = 0;

	/* R_AX_HIMR1 */
	hal_com->_intr[3].mask_dflt = (u32)(
		B_AX_GPIO18_INT_EN |
		0);
	hal_com->_intr[3].en = (hal_com->in_ps_intr_cfg ? true : false);
	hal_com->_intr[3].val = 0;

	hal_com->_intr_ind[0].mask = hal_com->_intr_ind[0].mask_dflt;
	hal_com->_intr[0].mask = hal_com->_intr[0].mask_dflt;
	hal_com->_intr[1].mask = hal_com->_intr[1].mask_dflt;
	hal_com->_intr[2].mask = hal_com->_intr[2].mask_dflt;
	hal_com->_intr[3].mask = hal_com->_intr[3].mask_dflt;

	PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "%s : 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X (opt %u)\n", __func__,
			 hal_com->_intr_ind[0].mask, hal_com->_intr[0].mask,
			 hal_com->_intr[1].mask, hal_com->_intr[2].mask, hal_com->_intr[3].mask, opt);
}

void hal_enable_int_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr[0].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR00, hal_com->_intr[0].mask);

	if (hal_com->_intr[1].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR10, hal_com->_intr[1].mask);

	if (hal_com->_intr[2].en)
		hal_write32(hal_com, R_AX_HIMR0, hal_com->_intr[2].mask);

	if (hal_com->_intr[3].en)
		hal_write32(hal_com, R_AX_HIMR1, hal_com->_intr[3].mask);

	if (hal_com->_intr_ind[0].en)
		hal_write32(hal_com, R_AX_PCIE_HIMR00_V1, hal_com->_intr_ind[0].mask);

	HAL_DUMP_INT_8852CE(hal, false, true, true);
}

void hal_disable_int_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr_ind[0].en)
		hal_write32(hal_com, R_AX_PCIE_HIMR00_V1, 0);

	if (hal_com->_intr[0].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR00, 0);

	if (hal_com->_intr[1].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR10, 0);

	if (hal_com->_intr[2].en)
		hal_write32(hal_com, R_AX_HIMR0, 0);

	if (hal_com->_intr[3].en)
		hal_write32(hal_com, R_AX_HIMR1, 0);
}

/**
 * When interrupt occured, call this function to disable imr.
 */
void hal_disable_int_isr_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr_ind[0].en)
		hal_write32(hal_com, R_AX_PCIE_HIMR00_V1, 0);
}

/**
 * This function is used to disable the imr that are not disable
 * in function "hal_disable_int_isr_8852ce".
 */
void hal_disable_int_rmn_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr[0].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR00, 0);

	if (hal_com->_intr[1].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR10, 0);

	if (hal_com->_intr[2].en)
		hal_write32(hal_com, R_AX_HIMR0, 0);

	if (hal_com->_intr[3].en)
		hal_write32(hal_com, R_AX_HIMR1, 0);
}

void hal_clear_int_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr[0].en && hal_com->_intr[0].val)
		hal_write32(hal_com, R_AX_HAXI_HISR00, hal_com->_intr[0].val);

	if (hal_com->_intr[1].en && hal_com->_intr[1].val)
		hal_write32(hal_com, R_AX_HAXI_HISR10, hal_com->_intr[1].val);

	if (hal_com->_intr[2].en && hal_com->_intr[2].val)
		hal_write32(hal_com, R_AX_HISR0, hal_com->_intr[2].val);

	if (hal_com->_intr[3].en && hal_com->_intr[3].val)
		hal_write32(hal_com, R_AX_HISR1, hal_com->_intr[3].val);

	if (hal_com->cv == CAV) {
		if (hal_com->_intr_ind[0].en && hal_com->_intr_ind[0].val)
			hal_write32(hal_com, R_AX_PCIE_HISR00_V1, hal_com->_intr_ind[0].val);
	}
}

void hal_clear_int_mask_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	hal_com->_intr_ind[0].en = false;
	hal_com->_intr[0].en = false;
	hal_com->_intr[1].en = false;
	hal_com->_intr[2].en = false;
	hal_com->_intr[3].en = false;

	hal_com->_intr_ind[0].mask = 0;
	hal_com->_intr[0].mask = 0;
	hal_com->_intr[1].mask = 0;
	hal_com->_intr[2].mask = 0;
	hal_com->_intr[3].mask = 0;
}

void hal_restore_int_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr[0].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR00, hal_com->_intr[0].mask);

	if (hal_com->_intr[1].en)
		hal_write32(hal_com, R_AX_HAXI_HIMR10, hal_com->_intr[1].mask);

	if (hal_com->_intr[2].en)
		hal_write32(hal_com, R_AX_HIMR0, hal_com->_intr[2].mask);

	if (hal_com->_intr[3].en)
		hal_write32(hal_com, R_AX_HIMR1, hal_com->_intr[3].mask);

	if (hal_com->_intr_ind[0].en)
		hal_write32(hal_com, R_AX_PCIE_HIMR00_V1, hal_com->_intr_ind[0].mask);
}

bool hal_recognize_int_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	bool recognized = false;

#ifndef CONFIG_SYNC_INTERRUPT
	/**
	 * We call hal_disable_int_isr_8852ce only instead of hal_disable_int_8852ce
	 * which will only disable imr of indicator. Otherwise, if we disable imr
	 * of wlan mac. ISR of indicator will be auto cleared due to imr & isr is '0'
	 * in wlan mac.
	 */
	hal_disable_int_isr_8852ce(hal);
#endif /* CONFIG_SYNC_INTERRUPT */

	/* check isr of indicator */
	if (hal_com->_intr_ind[0].en) {
		hal_com->_intr_ind[0].val = hal_read32(hal_com, R_AX_PCIE_HISR00_V1);
		hal_com->_intr_ind[0].val &= hal_com->_intr_ind[0].mask;
	}

	if (hal_com->_intr_ind[0].val) {
		hal_disable_int_rmn_8852ce(hal);
	} else {
		/* if isr is not recognized, go to end */
		goto end;
	}

	/* indicator isr is recognized, check isr of wlan mac */

	if (hal_com->_intr_ind[0].val & B_AX_HCI_AXIDMA_INT) {
		/* haxidma isr */
		if (hal_com->_intr[0].en) {
			hal_com->_intr[0].val = hal_read32(hal_com, R_AX_HAXI_HISR00);
			hal_com->_intr[0].val &= hal_com->_intr[0].mask;
		}
		if (hal_com->_intr[1].en) {
			hal_com->_intr[1].val = hal_read32(hal_com, R_AX_HAXI_HISR10);
			hal_com->_intr[1].val &= hal_com->_intr[1].mask;
		}
	} else {
		hal_com->_intr[0].val = 0;
		hal_com->_intr[1].val = 0;
	}

	if (hal_com->_intr_ind[0].val & B_AX_HS0ISR_IND_INT) {
		/* aon isr */
		if (hal_com->_intr[2].en) {
			hal_com->_intr[2].val = hal_read32(hal_com, R_AX_HISR0);
			hal_com->_intr[2].val &= hal_com->_intr[2].mask;
		}
	} else {
		hal_com->_intr[2].val = 0;
	}

	if (hal_com->_intr_ind[0].val & B_AX_HS1ISR_IND_INT) {
		/* aon isr */
		if (hal_com->_intr[3].en) {
			hal_com->_intr[3].val = hal_read32(hal_com, R_AX_HISR1);
			hal_com->_intr[3].val &= hal_com->_intr[3].mask;
		}
	} else {
		hal_com->_intr[3].val = 0;
	}

	if (hal_com->_intr[0].val || hal_com->_intr[1].val ||
		hal_com->_intr[2].val || hal_com->_intr[3].val)
		recognized = true;

	if (hal_com->_intr[2].val & B_AX_HALT_C2H_INT)
		PHL_WARN("%s : halt c2h interrupt is recognized (0x%x).\n", __func__, hal_com->_intr[2].val);

	if (hal_com->_intr[2].val & B_AX_WDT_TIMEOUT_INT) {
		PHL_WARN("%s : watchdog timer is recognized (0x%x).\n", __func__, hal_com->_intr[2].val);
		hal_clear_int_mask_8852ce(hal);
	}

end:
#ifndef CONFIG_SYNC_INTERRUPT
	/* clear isr */
	hal_clear_int_8852ce(hal);
	/* restore imr */
	hal_restore_int_8852ce(hal);
#endif /* CONFIG_SYNC_INTERRUPT */

	/*if (!recognized)
		HAL_DUMP_INT_8852CE(hal, true, true, true);*/

	return recognized;
}

static u32 hal_rx_handler_8852ce(struct hal_info_t *hal, u32 *handled)
{
	u32 ret = 0;
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	static const u32 rx_handle_irq = (
					B_AX_RXDMA_INT_EN |
					B_AX_RPQDMA_INT_EN|
					B_AX_RDU_INT_EN |
					B_AX_RPQBD_FULL_INT_EN);
	static const u32 rx_handle_irq_lps = (
					B_AX_GPIO18_INT_EN);
	u32	handled0 = ((hal_com->_intr[0].val & rx_handle_irq) |
					(hal_com->_intr[3].val & rx_handle_irq_lps));

	if (handled0 == 0)
		return ret;

	/* disable rx related IMR, rx thread will restore them */
	hal_com->_intr[0].mask &= ~rx_handle_irq;
	hal_com->_intr[3].mask &= ~rx_handle_irq_lps;
#ifndef CONFIG_SYNC_INTERRUPT
	if (hal_com->_intr[0].en)
		hal_write32(hal_com, R_AX_PCIE_HIMR00, hal_com->_intr[0].mask);
	if (hal_com->_intr[3].en)
		hal_write32(hal_com, R_AX_HIMR1, hal_com->_intr[3].mask);
#endif /* CONFIG_SYNC_INTERRUPT */
#ifdef PHL_RXSC_ISR
	hal_com->rx_int_array = handled0;
#endif
	handled[0] |= handled0;
	ret = 1;

	if ((hal_com->_intr[0].val & rx_handle_irq) & B_AX_RDU_INT_EN)
		hal_com->trx_stat.rx_rdu_cnt++;

	PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : rx handle 0x%08X, int_array 0x%08X\n",
			  __func__, handled0, hal_com->_intr[0].val);

	return ret;
}

static u32 hal_tx_handler_8852ce(struct hal_info_t *hal, u32 *handled)
{
	u32 ret = 0;
	struct rtw_hal_com_t *hal_com = hal->hal_com;
	u32 handled0 = 0x0;
	u32 handled1 = 0x0;

	handled0 = hal_com->_intr[0].val & (
			B_AX_TXDMA_ACH0_INT_EN |
			B_AX_TXDMA_ACH1_INT_EN |
			B_AX_TXDMA_ACH2_INT_EN |
			B_AX_TXDMA_ACH3_INT_EN |
			B_AX_TXDMA_ACH4_INT_EN |
			B_AX_TXDMA_ACH5_INT_EN |
			B_AX_TXDMA_ACH6_INT_EN |
			B_AX_TXDMA_ACH7_INT_EN |
			B_AX_TXDMA_CH8_INT_EN |
			B_AX_TXDMA_CH9_INT_EN |
			B_AX_TXDMA_CH12_INT_EN);

	handled1 = hal_com->_intr[1].val & (
			B_AX_TXDMA_CH10_INT_EN_V1 |
			B_AX_TXDMA_CH11_INT_EN_V1);

	if (handled0 != 0 || handled1 != 0)
		ret = 1;

	handled[0] |= handled0;
	handled[1] |= handled1;

	return ret;
}

static u32 hal_halt_c2h_handler_8852ce(struct hal_info_t *hal, u32 *handled)
{
	u32 ret = 0;
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr[2].val & B_AX_HALT_C2H_INT_EN) {
		handled[2] |= B_AX_HALT_C2H_INT_EN;
		ret = 1;
	}

	return ret;
}

static u32 hal_watchdog_timer_handler_8852ce(struct hal_info_t *hal, u32 *handled)
{
	u32 ret = 0;
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	if (hal_com->_intr[2].val & B_AX_WDT_TIMEOUT_INT_EN) {
		handled[2] |= B_AX_WDT_TIMEOUT_INT_EN;
		ret = 1;
	}

	return ret;
}

u32 hal_int_hdler_8852ce(struct hal_info_t *hal)
{
	u32 int_hdler_msk = 0x0;
	u32 handled[4] = {0};

	/* bit 1 : rx related */
	int_hdler_msk |= (hal_rx_handler_8852ce(hal, handled) << 1);

	/* bit 2 : tx related */
	int_hdler_msk |= (hal_tx_handler_8852ce(hal, handled) << 2);

	/* bit 4 : halt c2h */
	int_hdler_msk |= (hal_halt_c2h_handler_8852ce(hal, handled) << 4);

	/* bit 5 : watchdog timeout */
	int_hdler_msk |= (hal_watchdog_timer_handler_8852ce(hal, handled) << 5);

	PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : int_hdler_msk = 0x%x\n", __func__, int_hdler_msk);

#if 0
	if ((hal_com->int_array[0] & (~handled[0])) ||
		(hal_com->int_array[1] & (~handled[1])) ||
		(hal_com->int_array[2] & (~handled[2])) ||
		(hal_com->int_array[3] & (~handled[3]))) {
		PHL_WARN("%s : unhandled ISR => 0x%08X, 0x%08X, 0x%08X, 0x%08X. (handled[0-3])\n",
				 __func__,
				 (hal_com->int_array[0] & (~handled[0])),
				 (hal_com->int_array[1] & (~handled[1])),
				 (hal_com->int_array[2] & (~handled[2])),
				 (hal_com->int_array[3] & (~handled[3])));
	}
#endif

	return int_hdler_msk;
}


void hal_rx_int_restore_8852ce(struct hal_info_t *hal)
{
	struct rtw_hal_com_t *hal_com = hal->hal_com;

	hal_com->_intr[0].mask |= (B_AX_RXDMA_INT_EN | B_AX_RPQDMA_INT_EN |
							 B_AX_RDU_INT_EN | B_AX_RPQBD_FULL_INT_EN);
	hal_com->_intr[3].mask |= B_AX_GPIO18_INT_EN;
#ifndef CONFIG_SYNC_INTERRUPT
	if (hal_com->_intr[0].en) {
		hal_write32(hal_com, R_AX_PCIE_HIMR00, hal_com->_intr[0].mask);
	}
	if (hal_com->_intr[3].en) {
		hal_write32(hal_com, R_AX_HIMR1, hal_com->_intr[3].mask);
	}
#endif /* CONFIG_SYNC_INTERRUPT */

}

enum rtw_hal_status
hal_rx_rpq_int_check_8852ce(u8 dma_ch, u32 hal_int_array)
{
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;

	if (dma_ch == MAC_AX_RX_CH_RPQ && (hal_int_array & B_AX_RPQDMA_INT_EN))
		hstatus = RTW_HAL_STATUS_SUCCESS;

	return hstatus;
}

enum rtw_hal_status
hal_mp_init_8852ce(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_init_info_t init_52ce;

	FUNCIN_WSTS(hal_status);

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52ce, 0, sizeof(init_52ce));

	init_52ce.ic_name = "rtl8852ce";

	hal_status = hal_mp_init_8852c(phl_com, hal_info, &init_52ce);

	FUNCOUT_WSTS(hal_status);
	return hal_status;
}

enum rtw_hal_status
hal_mp_deinit_8852ce(struct rtw_phl_com_t *phl_com, struct hal_info_t *hal_info)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	struct hal_init_info_t init_52ce;

	FUNCIN_WSTS(hal_status);

	_os_mem_set(hal_to_drvpriv(hal_info), &init_52ce, 0, sizeof(init_52ce));

	init_52ce.ic_name = "rtl8852ce";

	hal_status = hal_mp_deinit_8852c(phl_com, hal_info, &init_52ce);

	if (RTW_HAL_STATUS_SUCCESS != hal_status) {

		PHL_ERR("hal_mp_deinit_8852ce: status = %u\n", hal_status);
		return hal_status;
	}

	FUNCOUT_WSTS(hal_status);
	return hal_status;
}

bool
hal_mp_path_chk_8852ce(struct rtw_phl_com_t *phl_com, u8 ant_tx, u8 cur_phy)
{
	if (phl_com->phy_cap[cur_phy].txss == 1 && ant_tx != RF_PATH_B)
		return false;
	else
		return true;
}
