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
#define _PHL_TRX_PCIE_C_
#include "../phl_headers.h"
#include "phl_trx_pcie.h"

#define target_in_area(target, start, end) \
	((target < start || target > end) ? false : true)
#define ALIGNMENT_MEMORY_ROUND_UP(_buf_len, _alignment) \
	(((_buf_len) + ((_alignment) - 1)) & (0xFFFFFFFF - ((_alignment) - 1)))
#define ALIGNMENT_MEMORY_POOL_LENGTH(_buf_len, _num, _alignment) \
	((_buf_len) + ((_alignment) - 1) + ALIGNMENT_MEMORY_ROUND_UP(_buf_len, _alignment) * ((_num) - 1))
#define WD_PAGE_SHMEM_POOL_VALID(_wd_page_ring) \
	(_wd_page_ring->wd_page_shmem_pool.vir_addr)
void phl_recycle_payload(struct phl_info_t *phl_info, u8 dma_ch, u16 wp_seq,
			 u8 txsts);

void phl_dump_link_list(void *phl, _os_list *list_head, u8 type)
{
	struct rtw_wd_page *wd_page = NULL, *t = NULL;
	struct rtw_h2c_pkt *h2c_pkt = NULL, *h2c_t = NULL;
	struct rtw_phl_tring_list *phl_tring_list = NULL, *phl_t = NULL;
	struct phl_ring_status *ring_sts = NULL, *rsts_t = NULL;
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	void *drv_priv = phl_to_drvpriv(phl_info);
	u8 *vir_addr = NULL;
	u32 i = 0, j = 0;
	u16 phl_idx = 0, phl_next_idx = 0;

	switch (type) {
	case TYPE_WD_PAGE:
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "===Dump WD Page===\n");
		phl_list_for_loop_safe(wd_page, t, struct rtw_wd_page,
						list_head, list) {
			vir_addr = (u8 *)wd_page->vir_addr;
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "vir_addr = %p, %x; phy_addr_l = %x; phy_addr_h = %x\n",
					vir_addr, *vir_addr,
					wd_page->phy_addr_l,
					wd_page->phy_addr_h);
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "cache = %d; buf_len = %d, wp_seq = %d\n",
					wd_page->cache, wd_page->buf_len,
					wd_page->wp_seq);
		}
		break;
	case TYPE_PHL_RING:
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "===Dump PHL Ring===\n");
		phl_list_for_loop_safe(phl_tring_list, phl_t,
					struct rtw_phl_tring_list,
					list_head, list) {

			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
				"-- macid = %d, band = %d, wmm = %d --\n",
					phl_tring_list->macid,
					phl_tring_list->band,
					phl_tring_list->wmm);

			for (i = 0; i < MAX_PHL_RING_CAT_NUM; i++) {
				phl_idx = (u16)_os_atomic_read(drv_priv,
						&phl_tring_list->phl_ring[i].phl_idx);
				phl_next_idx = (u16)_os_atomic_read(drv_priv,
						&phl_tring_list->phl_ring[i].phl_next_idx);

				PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
						"cat = %d\n"
						"dma_ch = %d\n"
						"tx_thres = %d\n"
						"core_idx = %d\n"
						"phl_idx = %d\n"
						"phl_next_idx = %d\n",
						phl_tring_list->phl_ring[i].cat,
						phl_tring_list->phl_ring[i].dma_ch,
						phl_tring_list->phl_ring[i].tx_thres,
						phl_tring_list->phl_ring[i].core_idx,
						phl_idx,
						phl_next_idx);

				for (j = 0; j < 5; j++) {
					PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
							"entry[%d] = %p\n",
							j,
					phl_tring_list->phl_ring[i].entry[j]);
				}
			}
		}
		break;
	case TYPE_RING_STS:
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "===Dump PHL Ring status===\n");
		phl_list_for_loop_safe(ring_sts, rsts_t, struct phl_ring_status,
					list_head, list) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
					"req_busy = %d\n"
					"ring_ptr = %p\n",
					ring_sts->req_busy,
					ring_sts->ring_ptr);
		}
		break;
	case TYPE_H2C_PKT:
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "===Dump H2C PKT===\n");
		phl_list_for_loop_safe(h2c_pkt, h2c_t, struct rtw_h2c_pkt,
					list_head, list) {
			vir_addr = (u8 *)h2c_pkt->vir_head;
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "vir_addr = %p, %x; phy_addr_l = %x; phy_addr_h = %x\n",
					vir_addr, *vir_addr,
					h2c_pkt->phy_addr_l,
					h2c_pkt->phy_addr_h);
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "type = %d; cache = %d; buf_len = %d\n",
					h2c_pkt->type, h2c_pkt->cache, h2c_pkt->buf_len);
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "vir_head = %p; vir_data = %p; vir_tail = %p; vir_end = %p\n",
					(u8 *)h2c_pkt->vir_head,
					(u8 *)h2c_pkt->vir_data,
					(u8 *)h2c_pkt->vir_tail,
					(u8 *)h2c_pkt->vir_end);
		}
		break;
	default :
		break;
	}
}

void _phl_dump_wp_stats(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct rtw_wp_rpt_stats *rpt_stats = NULL;
	u8 ch = 0;

	rpt_stats = (struct rtw_wp_rpt_stats *)hal_com->trx_stat.wp_rpt_stats;

	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		  "\n== wp report statistics == \n");
	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "ch			: %u\n", (int)ch);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "busy count		: %u\n",
			  (int)rpt_stats[ch].busy_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "ok count		: %u\n",
			  (int)rpt_stats[ch].tx_ok_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "retry fail count	: %u\n",
			  (int)rpt_stats[ch].rty_fail_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "lifetime drop count	: %u\n",
			  (int)rpt_stats[ch].lifetime_drop_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "macid drop count	: %u\n",
			  (int)rpt_stats[ch].macid_drop_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "sw drop count		: %u\n",
			  (int)rpt_stats[ch].sw_drop_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "recycle fail count	: %u\n",
			  (int)rpt_stats[ch].recycle_fail_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "delay ok count			: %u\n",
			  (int)rpt_stats[ch].delay_tx_ok_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "delay retry fail count		: %u\n",
			  (int)rpt_stats[ch].delay_rty_fail_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "delay lifetime drop count	: %u\n",
			  (int)rpt_stats[ch].delay_lifetime_drop_cnt);
		PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
			  "delay macid drop count		: %u\n",
			  (int)rpt_stats[ch].delay_macid_drop_cnt);

	}
}

void _phl_dump_busy_wp(struct phl_info_t *phl_info)
{
#ifdef CONFIG_RTW_DEBUG
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct rtw_xmit_req *treq = NULL;
	void *ptr = NULL;
	u16 wp_seq = 0;
	u8 ch = 0;

	if (!(COMP_PHL_XMIT & phl_log_components)
	    || !(_PHL_DEBUG_ <= phl_log_level))
		return;

	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;
	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		  "\n== dump busy wp == \n");
	for (ch = 0; ch < hci_info->total_txch_num; ch++) {

		for (wp_seq = 0; wp_seq < WP_MAX_SEQ_NUMBER; wp_seq++) {
			if (NULL != wd_ring[ch].wp_tag[wp_seq].ptr) {
				ptr = wd_ring[ch].wp_tag[wp_seq].ptr;
				treq = (struct rtw_xmit_req *)ptr;
				PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
					  "dma_ch = %d, wp_seq = 0x%x, ptr = %p!\n",
					  ch, wp_seq, ptr);
				PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
					  "wifi seq = %d\n",
					  treq->mdata.sw_seq);
			}
		}
	}
#endif /* CONFIG_RTW_DEBUG */
}


void _dec_sta_rpt_stats_busy_cnt(struct phl_info_t *phl_info, u16 macid)
{
	struct rtw_phl_stainfo_t *sta = NULL;
	struct rtw_wp_rpt_stats *rpt_stats = NULL;

	sta = rtw_phl_get_stainfo_by_macid(phl_info, macid);
	if (sta) {
		rpt_stats =
		(struct rtw_wp_rpt_stats *)sta->hal_sta->trx_stat.wp_rpt_stats;
		rpt_stats->busy_cnt--;
	}
}

void _inc_sta_rpt_stats_busy_cnt(struct phl_info_t *phl_info, u16 macid)
{
	struct rtw_phl_stainfo_t *sta = NULL;
	struct rtw_wp_rpt_stats *rpt_stats = NULL;

	sta = rtw_phl_get_stainfo_by_macid(phl_info, macid);
	if (sta) {
		rpt_stats =
		(struct rtw_wp_rpt_stats *)sta->hal_sta->trx_stat.wp_rpt_stats;
		rpt_stats->busy_cnt++;
	}
}


u8 _phl_check_recycle(u16 target, u16 rptr, u16 wptr, u16 bndy)
{
	u8 recycle = false;
	u8 init = 0;	/* starting point */

	if (wptr > rptr) {
		if (true == target_in_area(target, wptr, (bndy-1)))
			recycle = true;
		else if (true == target_in_area(target, init, rptr))
			recycle = true;
		else
			recycle = false;

	} else if (rptr > wptr) {
		if (true == target_in_area(target, wptr, rptr))
			recycle = true;
		else
			recycle = false;
	} else {
		recycle = true;
	}

	return recycle;
}

void phl_tx_start_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_tx_sts, PHL_TX_STATUS_RUNNING);
	PHL_WARN("%s: set PHL_TX_STATUS_RUNNING\n", __FUNCTION__);
}

void phl_tx_resume_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_tx_sts, PHL_TX_STATUS_RUNNING);
	PHL_WARN("%s: set PHL_TX_STATUS_RUNNING\n", __FUNCTION__);
}

void phl_req_tx_stop_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_tx_sts,
				PHL_TX_STATUS_STOP_INPROGRESS);
	PHL_WARN("%s: set PHL_TX_STATUS_STOP_INPROGRESS\n", __FUNCTION__);
}

void phl_tx_stop_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_tx_sts, PHL_TX_STATUS_SW_PAUSE);
	PHL_WARN("%s: set PHL_TX_STATUS_SW_PAUSE\n", __FUNCTION__);
}

bool phl_is_tx_sw_pause_pcie(struct phl_info_t *phl_info)
{
	void *drvpriv = phl_to_drvpriv(phl_info);

	if (PHL_TX_STATUS_SW_PAUSE == _os_atomic_read(drvpriv,
								&phl_info->phl_sw_tx_sts))
		return true;
	else
		return false;

}

void phl_rx_start_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_rx_sts, PHL_RX_STATUS_RUNNING);
	PHL_WARN("%s: set PHL_RX_STATUS_RUNNING\n", __FUNCTION__);
}

void phl_rx_resume_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_rx_sts, PHL_RX_STATUS_RUNNING);
	PHL_WARN("%s: set PHL_RX_STATUS_RUNNING\n", __FUNCTION__);
}

void phl_req_rx_stop_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_rx_sts,
				PHL_RX_STATUS_STOP_INPROGRESS);
	PHL_WARN("%s: set PHL_RX_STATUS_STOP_INPROGRESS\n", __FUNCTION__);
}

void phl_rx_stop_pcie(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_atomic_set(drv, &phl_info->phl_sw_rx_sts, PHL_RX_STATUS_SW_PAUSE);
	PHL_WARN("%s: set PHL_RX_STATUS_SW_PAUSE\n", __FUNCTION__);
}

bool phl_is_rx_sw_pause_pcie(struct phl_info_t *phl_info)
{
	void *drvpriv = phl_to_drvpriv(phl_info);

	if (PHL_RX_STATUS_SW_PAUSE ==
	    _os_atomic_read(drvpriv, &phl_info->phl_sw_rx_sts)) {
		return true;
	} else {
		return false;
	}
}

#ifdef RTW_WKARD_DYNAMIC_LTR
static bool _phl_judge_idle_ltr_switching_conditions(
	struct phl_info_t *phl_info, u16 macid)
{
	struct rtw_phl_stainfo_t *sta_info = NULL;
	struct rtw_stats *stats = &phl_info->phl_com->phl_stats;
	u16 ltr_thre = phl_info->phl_com->bus_sw_cap.ltr_sw_ctrl_thre;
	u8 tx_thre = 0, rx_thre = 0;
	u32 last_time = phl_ltr_get_last_trigger_time(phl_info->phl_com);

	tx_thre = ltr_thre >> 8;
	rx_thre = (u8)(ltr_thre & 0xFF);

	sta_info = rtw_phl_get_stainfo_by_macid(phl_info, macid);

	if (!rtw_hal_ltr_is_sw_ctrl(phl_info->phl_com, phl_info->hal))
		return false;

	if (sta_info == NULL)
		return false;

	if (sta_info->wrole == NULL)
		return false;

	if (stats->tx_traffic.lvl > tx_thre)
		return false;

	if (stats->rx_traffic.lvl > rx_thre)
		return false;

	if (RTW_PCIE_LTR_SW_IDLE == phl_ltr_get_cur_state(phl_info->phl_com))
		return false;

	if (phl_get_passing_time_us(last_time) < 500)
		return false;

	return true;

}
static bool _phl_judge_act_ltr_switching_conditions(
	struct phl_info_t *phl_info, u8 ch)
{
	u32 last_time = phl_ltr_get_last_trigger_time(phl_info->phl_com);
	u8 fwcmd_queue_idx = 0;

	fwcmd_queue_idx = rtw_hal_get_fwcmd_queue_idx(phl_info->hal);

	if (!rtw_hal_ltr_is_sw_ctrl(phl_info->phl_com, phl_info->hal))
		return true;

	if (ch == fwcmd_queue_idx)
		return true;

	if (RTW_PCIE_LTR_SW_ACT == phl_ltr_get_cur_state(phl_info->phl_com))
		return true;

	if (phl_get_passing_time_us(last_time) < 500)
		return false;

	return true;
}

static void _phl_act_ltr_update_stats(struct phl_info_t *phl_info,
		bool success, u8 ch, u16 pending_wd_page_cnt)
{
	static bool bdly = false;
	static u32 dly_start_time = 0;

	if (!rtw_hal_ltr_is_sw_ctrl(phl_info->phl_com, phl_info->hal))
		return;

	if (success) {
		/* only those have been delayed last time*/
		if (bdly) {
			PHL_INFO("%s() ch(%u), %u packets be transmitted after defering %uus\n"
				, __func__, ch,	pending_wd_page_cnt,
				phl_get_passing_time_us(dly_start_time));
			rtw_hal_ltr_update_stats(phl_info->hal, true);
		}
		bdly = false;
	} else {

		/* the first packet that is going to defer */
		if (false == bdly)
			dly_start_time = _os_get_cur_time_us();

		PHL_DBG("%s() ch(%u), %u packets be delayed\n", __func__,
							ch,	pending_wd_page_cnt);

		rtw_hal_ltr_update_stats(phl_info->hal, false);
		bdly = true;
		dly_start_time = _os_get_cur_time_us();
	}
}

static void _phl_switch_act_ltr(struct phl_info_t *phl_info, u8 tx_dma_ch)
{
	u8 fwcmd_queue_idx = 0;

	if (!rtw_hal_ltr_is_sw_ctrl(phl_info->phl_com, phl_info->hal))
		return;

	if (RTW_PCIE_LTR_SW_ACT == phl_ltr_get_cur_state(phl_info->phl_com))
		return;

	fwcmd_queue_idx = rtw_hal_get_fwcmd_queue_idx(phl_info->hal);

	if (tx_dma_ch != fwcmd_queue_idx)
		phl_ltr_sw_trigger(phl_info->phl_com, phl_info->hal,
			RTW_PCIE_LTR_SW_ACT);

}

static void _phl_switch_idle_ltr(struct phl_info_t *phl_info,
					struct rtw_wp_rpt_stats *rpt_stats)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	u8 ch = 0;
	bool bempty = 1;
	u8 fwcmd_queue_idx = 0;

	fwcmd_queue_idx = rtw_hal_get_fwcmd_queue_idx(phl_info->hal);

	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
		if (ch == fwcmd_queue_idx)
			continue;
		if (rpt_stats[ch].busy_cnt != 0)
			bempty = 0;
	}

	if (bempty)
		phl_ltr_sw_trigger(phl_info->phl_com, phl_info->hal,
			RTW_PCIE_LTR_SW_IDLE);

}
#endif

#ifdef RTW_WKARD_TXBD_UPD_LMT
static void
_phl_free_h2c_work_ring(struct phl_info_t *phl_info,
			struct rtw_wd_page_ring *wd_page_ring)
{
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);
	struct rtw_h2c_work *h2c_work = &wd_page_ring->h2c_work;
	struct rtw_h2c_pkt *cmd = h2c_work->cmd;
	struct rtw_h2c_pkt *data = h2c_work->data;
	struct rtw_h2c_pkt *ldata = h2c_work->ldata;
	struct phl_hci_trx_ops *hci_trx_ops = phl_info->hci_trx_ops;
	u16 i = 0, buf_num = 0;

	buf_num = hal_spec->txbd_multi_tag;

	if (NULL != cmd) {
		for (i = 0; i < buf_num; i++) {
			if (NULL == cmd->vir_head)
				continue;
			hci_trx_ops->free_h2c_pkt_buf(phl_info, cmd);
			cmd->vir_head = NULL;
			cmd->cache = NONCACHE_ADDR;
			cmd++;
		}
		_os_mem_free(drv_priv, h2c_work->cmd,
			     buf_num * sizeof(*h2c_work->cmd));
	}
	if (NULL != data) {
		for (i = 0; i < buf_num; i++) {
			if (NULL == data->vir_head)
				continue;
			hci_trx_ops->free_h2c_pkt_buf(phl_info, data);
			data->vir_head = NULL;
			data->cache = NONCACHE_ADDR;
			data++;
		}
		_os_mem_free(drv_priv, h2c_work->data,
			     buf_num * sizeof(*h2c_work->data));
	}
	if (NULL != ldata) {
		for (i = 0; i < buf_num; i++) {
			if (NULL == ldata->vir_head)
				continue;
			hci_trx_ops->free_h2c_pkt_buf(phl_info, ldata);
			ldata->vir_head = NULL;
			ldata->cache = NONCACHE_ADDR;
			ldata++;
		}
		_os_mem_free(drv_priv, h2c_work->ldata,
			     buf_num * sizeof(*h2c_work->ldata));
	}

	if (NULL != h2c_work->cmd_ring) {
		_os_mem_free(drv_priv, h2c_work->cmd_ring,
			     buf_num * sizeof(struct rtw_h2c_pkt *));
        }
	if (NULL != h2c_work->data_ring) {
		_os_mem_free(drv_priv, h2c_work->data_ring,
			     buf_num * sizeof(struct rtw_h2c_pkt *));
        }
	if (NULL != h2c_work->ldata_ring) {
		_os_mem_free(drv_priv, h2c_work->ldata_ring,
			     buf_num * sizeof(struct rtw_h2c_pkt *));
        }
	h2c_work->cmd_cnt = 0;
	h2c_work->cmd_idx = 0;
	h2c_work->data_cnt = 0;
	h2c_work->data_idx = 0;
	h2c_work->ldata_cnt = 0;
	h2c_work->ldata_idx = 0;
	_os_spinlock_free(drv_priv,	&h2c_work->lock);
}


static enum rtw_phl_status
_phl_alloc_h2c_work_ring(struct phl_info_t *phl_info,
			 struct rtw_wd_page_ring *wd_page_ring)
{
	enum rtw_phl_status psts = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct phl_hci_trx_ops *hci_trx_ops = phl_info->hci_trx_ops;
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);
	struct rtw_h2c_work *h2c_work = &wd_page_ring->h2c_work;
	struct rtw_h2c_pkt *cmd = NULL, *data =  NULL, *ldata = NULL;
	u16 buf_num = 0, i = 0;
#ifdef CONFIG_H2C_NONCACHE_ADDR
	enum cache_addr_type cache = NONCACHE_ADDR;
#else
	enum cache_addr_type cache = CACHE_ADDR;
#endif

	buf_num = hal_spec->txbd_multi_tag;
	_os_spinlock_init(drv_priv, &h2c_work->lock);

	h2c_work->cmd = _os_mem_alloc(drv_priv, buf_num * sizeof(*cmd));
	h2c_work->data = _os_mem_alloc(drv_priv, buf_num * sizeof(*data));
	h2c_work->ldata = _os_mem_alloc(drv_priv, buf_num * sizeof(*ldata));

	if (!h2c_work->cmd || !h2c_work->data || !h2c_work->ldata) {
		psts = RTW_PHL_STATUS_RESOURCE;
		goto out;
	}
	cmd = h2c_work->cmd;
	data = h2c_work->data;
	ldata = h2c_work->ldata;

	_os_mem_set(drv_priv, cmd, 0, buf_num * sizeof(*cmd));
	_os_mem_set(drv_priv, data, 0, buf_num * sizeof(*data));
	_os_mem_set(drv_priv, ldata, 0, buf_num * sizeof(*ldata));

	h2c_work->cmd_ring =
		_os_mem_alloc(drv_priv,
			       buf_num * sizeof(struct rtw_h2c_pkt *));
	h2c_work->data_ring =
		_os_mem_alloc(drv_priv,
			       buf_num * sizeof(struct rtw_h2c_pkt *));
	h2c_work->ldata_ring =
		_os_mem_alloc(drv_priv,
			       buf_num * sizeof(struct rtw_h2c_pkt *));

	if (!h2c_work->cmd_ring || !h2c_work->data_ring ||
	    !h2c_work->ldata_ring) {
		psts = RTW_PHL_STATUS_RESOURCE;
		goto out;
	}
	_os_mem_set(drv_priv, h2c_work->cmd_ring, 0,
		    buf_num * sizeof(struct rtw_h2c_pkt *));
	_os_mem_set(drv_priv, h2c_work->data_ring, 0,
		    buf_num * sizeof(struct rtw_h2c_pkt *));
	_os_mem_set(drv_priv, h2c_work->ldata_ring, 0,
		    buf_num * sizeof(struct rtw_h2c_pkt *));

	for (i = 0; i < buf_num; i++) {
		cmd->type = H2CB_TYPE_CMD;
		cmd->cache = cache;
		cmd->buf_len = FWCMD_HDR_LEN + _WD_BODY_LEN + H2C_CMD_LEN;
		hci_trx_ops->alloc_h2c_pkt_buf(phl_info, cmd, cmd->buf_len);
		if (NULL == cmd->vir_head) {
			psts = RTW_PHL_STATUS_RESOURCE;
			goto out;
		}
		cmd->vir_data = cmd->vir_head + FWCMD_HDR_LEN + _WD_BODY_LEN;
		cmd->vir_tail = cmd->vir_data;
		cmd->vir_end = cmd->vir_data + H2C_CMD_LEN;
		INIT_LIST_HEAD(&cmd->list);
		h2c_work->cmd_ring[i] = cmd;
		h2c_work->cmd_cnt++;
		cmd++;
	}
	for (i = 0; i < buf_num; i++) {
		data->type = H2CB_TYPE_DATA;
		data->cache = cache;
		data->buf_len = FWCMD_HDR_LEN + _WD_BODY_LEN + H2C_DATA_LEN;
		hci_trx_ops->alloc_h2c_pkt_buf(phl_info, data, data->buf_len);
		if (NULL == data->vir_head) {
			psts = RTW_PHL_STATUS_RESOURCE;
			goto out;
		}
		data->vir_data = data->vir_head + FWCMD_HDR_LEN + _WD_BODY_LEN;
		data->vir_tail = data->vir_data;
		data->vir_end = data->vir_data + H2C_DATA_LEN;
		INIT_LIST_HEAD(&data->list);
		h2c_work->data_ring[i] = data;
		h2c_work->data_cnt++;
		data++;
	}
	for (i = 0; i < buf_num; i++) {
		ldata->type = H2CB_TYPE_LONG_DATA;
		ldata->cache = cache;
		ldata->buf_len = FWCMD_HDR_LEN + _WD_BODY_LEN +
				 H2C_LONG_DATA_LEN;
		hci_trx_ops->alloc_h2c_pkt_buf(phl_info, ldata, ldata->buf_len);
		if (NULL == ldata->vir_head) {
			psts = RTW_PHL_STATUS_RESOURCE;
			goto out;
		}
		ldata->vir_data = ldata->vir_head + FWCMD_HDR_LEN +
				 _WD_BODY_LEN;
		ldata->vir_tail = ldata->vir_data;
		ldata->vir_end = ldata->vir_data + H2C_LONG_DATA_LEN;
		INIT_LIST_HEAD(&ldata->list);
		h2c_work->ldata_ring[i] = ldata;
		h2c_work->ldata_cnt++;
		ldata++;
	}

	h2c_work->cmd_idx = 0;
	h2c_work->data_idx = 0;
	h2c_work->ldata_idx = 0;
	psts = RTW_PHL_STATUS_SUCCESS;

out:
	if (RTW_PHL_STATUS_SUCCESS != psts) {
		_phl_free_h2c_work_ring(phl_info, wd_page_ring);
		h2c_work->cmd = NULL;
		h2c_work->data = NULL;
		h2c_work->ldata = NULL;
		h2c_work->cmd_ring = NULL;
		h2c_work->data_ring = NULL;
		h2c_work->ldata_ring = NULL;
	}

	return psts;
}


static void
_phl_free_wd_work_ring(struct phl_info_t *phl_info,
		       struct rtw_wd_page_ring *wd_page_ring)
{
	void *drv_priv = phl_to_drvpriv(phl_info);
	void *rtw_dma_pool = NULL;
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);
	u16 i = 0, buf_num = 0;

#ifdef CONFIG_WD_WORK_RING_NONCACHE_ADDR
	rtw_dma_pool = phl_info->hci->wd_dma_pool;
#endif

	buf_num = hal_spec->txbd_multi_tag;

	if (NULL != wd_page_ring->wd_work) {
		for (i = 0; i < buf_num; i++) {

			if (NULL == wd_page_ring->wd_work[i].vir_addr)
				continue;

			wd_page_ring->wd_work[i].wp_seq = WP_RESERVED_SEQ;
			_os_shmem_free(drv_priv, rtw_dma_pool,
			       	wd_page_ring->wd_work[i].vir_addr,
			       	&wd_page_ring->wd_work[i].phy_addr_l,
			       	&wd_page_ring->wd_work[i].phy_addr_h,
				WD_PAGE_SIZE,
				wd_page_ring->wd_work[i].cache,
				DMA_FROM_DEVICE,
				wd_page_ring->wd_work[i].os_rsvd[0]);
			wd_page_ring->wd_work[i].vir_addr = NULL;
			wd_page_ring->wd_work[i].cache = NONCACHE_ADDR;
		}

		_os_mem_free(drv_priv, wd_page_ring->wd_work,
			      buf_num * sizeof(*wd_page_ring->wd_work));
		wd_page_ring->wd_work = NULL;
	}

	if (NULL != wd_page_ring->wd_work_ring) {
		_os_mem_free(drv_priv, wd_page_ring->wd_work_ring,
			      buf_num * sizeof(struct rtw_wd_page *));
		wd_page_ring->wd_work_ring = NULL;
	}
	wd_page_ring->wd_work_cnt = 0;
	wd_page_ring->wd_work_idx = 0;

}

static enum rtw_phl_status
_phl_alloc_wd_work_ring(struct phl_info_t *phl_info,
			struct rtw_wd_page_ring *wd_page_ring)
{
	enum rtw_phl_status psts = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	void *rtw_dma_pool = NULL;
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);
	struct rtw_wd_page *wd_work = NULL;
	u32 buf_len = 0;
	u16 buf_num = 0, i = 0;
#ifdef CONFIG_WD_WORK_RING_NONCACHE_ADDR
	u8 wd_cache_type = NONCACHE_ADDR;
	rtw_dma_pool = phl_info->hci->wd_dma_pool;
#else
	u8 wd_cache_type = CACHE_ADDR;
#endif

	buf_num = hal_spec->txbd_multi_tag;

	wd_page_ring->wd_work = _os_mem_alloc(drv_priv,
					       buf_num * sizeof(*wd_work));
	if (!wd_page_ring->wd_work) {
		psts = RTW_PHL_STATUS_RESOURCE;
		goto out;
	}
	wd_work = wd_page_ring->wd_work;
	_os_mem_set(drv_priv, wd_work, 0, buf_num * sizeof(*wd_work));

	wd_page_ring->wd_work_ring =
		_os_mem_alloc(drv_priv,
			       buf_num * sizeof(struct rtw_wd_page *));
	if (!wd_page_ring->wd_work_ring) {
		psts = RTW_PHL_STATUS_RESOURCE;
		goto out;
	}
	_os_mem_set(drv_priv, wd_page_ring->wd_work_ring, 0,
		    buf_num * sizeof(struct rtw_wd_page *));

	for (i = 0; i < buf_num; i++) {
		wd_work[i].cache = wd_cache_type;
		buf_len = WD_PAGE_SIZE;
		wd_work[i].vir_addr = _os_shmem_alloc(drv_priv,
					rtw_dma_pool,
					&wd_work[i].phy_addr_l,
					&wd_work[i].phy_addr_h,
					buf_len,
					wd_work[i].cache,
					DMA_TO_DEVICE,
					&wd_work[i].os_rsvd[0]);
 		if (NULL == wd_work[i].vir_addr) {
			psts = RTW_PHL_STATUS_RESOURCE;
			goto out;
		}
		wd_work[i].buf_len = buf_len;
		wd_work[i].wp_seq = WP_RESERVED_SEQ;
		INIT_LIST_HEAD(&wd_work[i].list);

		wd_page_ring->wd_work_ring[i] = &wd_work[i];
		wd_page_ring->wd_work_cnt++;
		/* hana_todo now check 4 byte align only */
		/* if ((unsigned long)wd_page_buf & 0xF) { */
		/* 	res = _FAIL; */
		/* 	break; */
		/* } */
	}

	wd_page_ring->wd_work_idx = 0;
	psts = RTW_PHL_STATUS_SUCCESS;

out:
	if (RTW_PHL_STATUS_SUCCESS != psts) {
		_phl_free_wd_work_ring(phl_info, wd_page_ring);
		wd_page_ring->wd_work = NULL;
		wd_page_ring->wd_work_ring = NULL;
	}

	return psts;
}
#else
#define _phl_free_h2c_work_ring(_phl, _ring)
#define _phl_alloc_h2c_work_ring(_phl, _ring) RTW_PHL_STATUS_SUCCESS
#define _phl_free_wd_work_ring(_phl, _ring)
#define _phl_alloc_wd_work_ring(_phl, _ring) RTW_PHL_STATUS_SUCCESS
#endif

static enum rtw_phl_status enqueue_pending_wd_page(struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring,
				struct rtw_wd_page *wd_page, u8 pos)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *list = &wd_page_ring->pending_wd_page_list;

	if (wd_page != NULL) {
		_os_spinlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

		if (_tail == pos)
			list_add_tail(&wd_page->list, list);
		else if (_first == pos)
			list_add(&wd_page->list, list);

		wd_page_ring->pending_wd_page_cnt++;

		_os_spinunlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}

static enum rtw_phl_status enqueue_busy_wd_page(struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring,
				struct rtw_wd_page *wd_page, u8 pos)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *list = &wd_page_ring->busy_wd_page_list;

	if (wd_page != NULL) {
		_os_spinlock(drv_priv, &wd_page_ring->busy_lock, _bh, NULL);

		if (_tail == pos)
			list_add_tail(&wd_page->list, list);
		else if (_first == pos)
			list_add(&wd_page->list, list);

		wd_page_ring->busy_wd_page_cnt++;

		_os_spinunlock(drv_priv, &wd_page_ring->busy_lock, _bh, NULL);

		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}


static enum rtw_phl_status enqueue_idle_wd_page(
				struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring,
				struct rtw_wd_page *wd_page)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *list = &wd_page_ring->idle_wd_page_list;

	if (wd_page != NULL) {
#ifdef CONFIG_PHL_WD_PAGE_RESET
		_os_mem_set(phl_to_drvpriv(phl_info), wd_page->vir_addr, 0,
					WD_PAGE_SIZE);
#endif
		wd_page->buf_len = WD_PAGE_SIZE;
		wd_page->wp_seq = WP_RESERVED_SEQ;
		wd_page->host_idx = 0;
		INIT_LIST_HEAD(&wd_page->list);

		_os_spinlock(drv_priv, &wd_page_ring->idle_lock, _bh, NULL);

		list_add_tail(&wd_page->list, list);
		wd_page_ring->idle_wd_page_cnt++;

		_os_spinunlock(drv_priv, &wd_page_ring->idle_lock, _bh, NULL);

		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}

#ifdef RTW_WKARD_TXBD_UPD_LMT
static enum rtw_phl_status enqueue_h2c_work_ring(
				struct phl_info_t *phl_info,
				struct rtw_h2c_pkt *h2c)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct rtw_h2c_work *h2c_work = NULL;
	struct rtw_h2c_pkt *work_done_h2c = NULL;
	struct rtw_h2c_pkt **ring = NULL;
	u16 *idx = 0, *cnt = 0;
	u8 fwcmd_qidx = 0;

	fwcmd_qidx = rtw_hal_get_fwcmd_queue_idx(phl_info->hal);
	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;
	h2c_work = &wd_ring[fwcmd_qidx].h2c_work;

	if (h2c == NULL)
		goto out;

	_os_spinlock(drv_priv, &h2c_work->lock, _bh, NULL);

	if (H2CB_TYPE_CMD == h2c->type) {
		ring = h2c_work->cmd_ring;
		idx = &h2c_work->cmd_idx;
		cnt = &h2c_work->cmd_cnt;
	} else if (H2CB_TYPE_DATA == h2c->type) {
		ring = h2c_work->data_ring;
		idx = &h2c_work->data_idx;
		cnt = &h2c_work->data_cnt;
	} else if (H2CB_TYPE_LONG_DATA == h2c->type) {
		ring = h2c_work->ldata_ring;
		idx = &h2c_work->ldata_idx;
		cnt = &h2c_work->ldata_cnt;
	} else {
		_os_spinunlock(drv_priv, &h2c_work->lock, _bh, NULL);
		goto out;
	}

	work_done_h2c = ring[*idx];
	ring[*idx] = h2c;
	*idx = (*idx + 1) % *cnt;

	_os_spinunlock(drv_priv, &h2c_work->lock, _bh, NULL);

	pstatus = phl_enqueue_idle_h2c_pkt(phl_info, work_done_h2c);

out:
	return pstatus;
}

static enum rtw_phl_status enqueue_wd_work_ring(
				struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring,
				struct rtw_wd_page *wd_page)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct rtw_wd_page *work_done_wd = NULL;
	struct rtw_wd_page **ring = wd_page_ring->wd_work_ring;

	if (wd_page != NULL) {

		_os_spinlock(drv_priv, &wd_page_ring->work_lock, _bh, NULL);

		work_done_wd = ring[wd_page_ring->wd_work_idx];
		ring[wd_page_ring->wd_work_idx] = wd_page;
		wd_page_ring->wd_work_idx =
		    (wd_page_ring->wd_work_idx + 1) % wd_page_ring->wd_work_cnt;

		_os_spinunlock(drv_priv, &wd_page_ring->work_lock, _bh, NULL);

		enqueue_idle_wd_page(phl_info, wd_page_ring, work_done_wd);

		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}
#else
#define enqueue_h2c_work_ring(_phl, _h2c) RTW_PHL_STATUS_FAILURE
#define enqueue_wd_work_ring(_phl, _ring, _wd) RTW_PHL_STATUS_FAILURE
#endif


static struct rtw_wd_page *query_pending_wd_page(struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring)
{
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *pending_list = &wd_page_ring->pending_wd_page_list;
	struct rtw_wd_page *wd_page = NULL;

	_os_spinlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

	if (true == list_empty(pending_list)) {
		wd_page = NULL;
	} else {
		wd_page = list_first_entry(pending_list, struct rtw_wd_page,
						list);
		wd_page_ring->pending_wd_page_cnt--;
		list_del(&wd_page->list);
	}

	_os_spinunlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

	return wd_page;
}


static struct rtw_wd_page *query_idle_wd_page(struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring)
{
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *idle_list = &wd_page_ring->idle_wd_page_list;
	struct rtw_wd_page *wd_page = NULL;

	_os_spinlock(drv_priv, &wd_page_ring->idle_lock, _bh, NULL);

	if (true == list_empty(idle_list)) {
		wd_page = NULL;
	} else {
		wd_page = list_first_entry(idle_list, struct rtw_wd_page, list);
		wd_page_ring->idle_wd_page_cnt--;
		list_del(&wd_page->list);
	}

	_os_spinunlock(drv_priv, &wd_page_ring->idle_lock, _bh, NULL);

	return wd_page;
}

static enum rtw_phl_status rtw_release_target_wd_page(
					struct phl_info_t *phl_info,
					struct rtw_wd_page_ring *wd_page_ring,
					struct rtw_wd_page *wd_page)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;

	if (wd_page_ring != NULL && wd_page != NULL) {
		enqueue_idle_wd_page(phl_info, wd_page_ring, wd_page);
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}

static enum rtw_phl_status rtw_release_pending_wd_page(
				struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring,
				u16 release_num)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	_os_list *list = &wd_page_ring->pending_wd_page_list;
	struct rtw_wd_page *wd_page = NULL;

	if (wd_page_ring != NULL) {
		while (release_num > 0 && true != list_empty(list)) {

			wd_page = query_pending_wd_page(phl_info, wd_page_ring);

			enqueue_idle_wd_page(phl_info, wd_page_ring, wd_page);

			release_num--;
		}
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}
	return pstatus;
}

static enum rtw_phl_status rtw_release_busy_wd_page(
				struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_page_ring,
				u16 release_num)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	_os_list *list = &wd_page_ring->busy_wd_page_list;
	struct rtw_wd_page *wd_page = NULL;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);

	if (wd_page_ring != NULL) {
		_os_spinlock(drv_priv, &wd_page_ring->busy_lock, _bh, NULL);

		while (release_num > 0 && true != list_empty(list)) {

			wd_page = list_first_entry(list, struct rtw_wd_page,
							list);
			wd_page_ring->busy_wd_page_cnt--;
			list_del(&wd_page->list);

			_os_spinunlock(drv_priv, &wd_page_ring->busy_lock, _bh, NULL);
			if (true == hal_spec->txbd_upd_lmt) {
				pstatus = enqueue_wd_work_ring(phl_info,
							       wd_page_ring,
							       wd_page);
			} else {
				pstatus = enqueue_idle_wd_page(phl_info,
							       wd_page_ring,
							       wd_page);
			}
			_os_spinlock(drv_priv, &wd_page_ring->busy_lock, _bh, NULL);

			if (RTW_PHL_STATUS_SUCCESS != pstatus)
				break;
			release_num--;
		}
		_os_spinunlock(drv_priv, &wd_page_ring->busy_lock, _bh, NULL);

	}
	return pstatus;
}

static void _phl_reset_txbd(struct phl_info_t *phl_info,
				struct tx_base_desc *txbd)
{
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	_os_mem_set(phl_to_drvpriv(phl_info), txbd->vir_addr, 0, txbd->buf_len);
	txbd->host_idx = 0;
	txbd->avail_num = hal_com->bus_cap.txbd_num;
}
static void _phl_reset_wp_tag(struct phl_info_t *phl_info,
			struct rtw_wd_page_ring *wd_page_ring, u8 dma_ch)
{
	u16 wp_seq = 0;

	for (wp_seq = 0; wp_seq < WP_MAX_SEQ_NUMBER; wp_seq++) {
		if (NULL != wd_page_ring->wp_tag[wp_seq].ptr)
			phl_recycle_payload(phl_info, dma_ch, wp_seq,
					    TX_STATUS_TX_FAIL_SW_DROP);
	}
}


static enum rtw_phl_status enqueue_pending_h2c_pkt(struct phl_info_t *phl_info,
				struct phl_h2c_pkt_pool *h2c_pkt_pool,
				struct rtw_h2c_pkt *h2c_pkt, u8 pos)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
#if 0
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *list = &wd_page_ring->pending_wd_page_list;

	if (wd_page != NULL) {
		_os_spinlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

		if (_tail == pos)
			list_add_tail(&wd_page->list, list);
		else if (_first == pos)
			list_add(&wd_page->list, list);

		wd_page_ring->pending_wd_page_cnt++;

		_os_spinunlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

		pstatus = RTW_PHL_STATUS_SUCCESS;
	}
#endif
	return pstatus;
}

static struct rtw_h2c_pkt *query_pending_h2c_pkt(struct phl_info_t *phl_info,
				struct phl_h2c_pkt_pool *h2c_pkt_pool)
{
	//void *drv_priv = phl_to_drvpriv(phl_info);
	//_os_list *pending_list = &wd_page_ring->pending_wd_page_list;
	struct rtw_h2c_pkt *h2c_pkt = NULL;
#if 0
	_os_spinlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);

	if (true == list_empty(pending_list)) {
		wd_page = NULL;
	} else {
		wd_page = list_first_entry(pending_list, struct rtw_wd_page,
						list);
		wd_page_ring->pending_wd_page_cnt--;
		list_del(&wd_page->list);
	}

	_os_spinunlock(drv_priv, &wd_page_ring->pending_lock, _bh, NULL);
#endif
	return h2c_pkt;
}

static enum rtw_phl_status phl_release_busy_h2c_pkt(
				struct phl_info_t *phl_info,
				struct phl_h2c_pkt_pool *h2c_pkt_pool,
				u16 release_num)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	_os_list *list = &h2c_pkt_pool->busy_h2c_pkt_list.queue;
	struct rtw_h2c_pkt *h2c_pkt = NULL;
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);

	if (h2c_pkt_pool != NULL) {

		while (release_num > 0 && true != list_empty(list)) {
			h2c_pkt = phl_query_busy_h2c_pkt(phl_info);

			if (!h2c_pkt)
				break;

			if (true == hal_spec->txbd_upd_lmt) {
				pstatus = enqueue_h2c_work_ring(phl_info,
								h2c_pkt);
			} else {
				pstatus = phl_enqueue_idle_h2c_pkt(phl_info,
								   h2c_pkt);
			}

			if (RTW_PHL_STATUS_SUCCESS != pstatus)
				break;
			release_num--;
		}
	}
	return pstatus;
}

static void phl_tx_reset_pcie(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct tx_base_desc *txbd = NULL;
	struct phl_h2c_pkt_pool *h2c_pool = NULL;
	u8 ch = 0;

	txbd = (struct tx_base_desc *)hci_info->txbd_buf;
	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;
	h2c_pool = (struct phl_h2c_pkt_pool *)phl_info->h2c_pool;

	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
		_phl_reset_txbd(phl_info, &txbd[ch]);
		rtw_release_busy_wd_page(phl_info, &wd_ring[ch],
					 wd_ring[ch].busy_wd_page_cnt);
		rtw_release_pending_wd_page(phl_info, &wd_ring[ch],
					 wd_ring[ch].pending_wd_page_cnt);
		wd_ring[ch].cur_hw_res = 0;
		_phl_reset_wp_tag(phl_info, &wd_ring[ch], ch);
	}

	phl_release_busy_h2c_pkt(phl_info, h2c_pool,
				 (u16)h2c_pool->busy_h2c_pkt_list.cnt);

	phl_dump_h2c_pool_stats(phl_info->h2c_pool);
}

static void phl_tx_reset_hwband_pcie(struct phl_info_t *phl_info, enum phl_band_idx band_idx)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct tx_base_desc *txbd = NULL;
	u8 ch = 0;

	txbd = (struct tx_base_desc *)hci_info->txbd_buf;
	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;

	PHL_INFO("phl_tx_reset_hwband_pcie :: band %d \n", band_idx);
	/* Phase1: Drop packets */
	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
		if(band_idx == rtw_hal_query_txch_hwband(phl_info->hal, ch)) {
			_phl_reset_txbd(phl_info, &txbd[ch]);
			rtw_release_busy_wd_page(phl_info, &wd_ring[ch],
					 wd_ring[ch].busy_wd_page_cnt);
			rtw_release_pending_wd_page(phl_info, &wd_ring[ch],
					 wd_ring[ch].pending_wd_page_cnt);
			_phl_reset_wp_tag(phl_info, &wd_ring[ch], ch);
		}
	}
}

static void phl_tx_return_all_wps(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	u8 ch;

	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;
	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
		_phl_reset_wp_tag(phl_info, &wd_ring[ch], ch);
	}
}

static void
_phl_free_local_buf_pcie(struct phl_info_t *phl_info, struct rtw_xmit_req *treq)
{
	struct rtw_phl_evt_ops *ops = &phl_info->phl_com->evt_ops;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct tx_local_buf *local_buf = NULL;
#ifdef RTW_TX_COALESCE_BAK_PKT_LIST
	struct rtw_pkt_buf_list *pkt_list;
	int i;
#endif

	if (treq->local_buf == NULL)
		return;

	local_buf = treq->local_buf;

#ifdef RTW_TX_COALESCE_BAK_PKT_LIST
	pkt_list = (struct rtw_pkt_buf_list *)treq->pkt_list;
	for (i = 0; i < local_buf->pkt_cnt; i++)
		pkt_list[i] = local_buf->pkt_list[i];
	treq->pkt_cnt = local_buf->pkt_cnt;
#endif

	if (local_buf->vir_addr) {
		if (ops->os_return_local_buf)
			ops->os_return_local_buf(drv_priv, local_buf);
	}
	_os_kmem_free(drv_priv, treq->local_buf, sizeof(struct tx_local_buf));
	treq->local_buf = NULL;
}


static enum rtw_phl_status
_phl_alloc_local_buf_pcie(struct phl_info_t *phl_info,
                          struct rtw_xmit_req *treq)
{
	enum rtw_phl_status sts = RTW_PHL_STATUS_FAILURE;
	struct rtw_phl_evt_ops *ops = &phl_info->phl_com->evt_ops;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct tx_local_buf *local_buf = NULL;
	int local_buf_size = sizeof(struct tx_local_buf);
#ifdef RTW_TX_COALESCE_BAK_PKT_LIST
	struct rtw_pkt_buf_list *pkt_list;
	int i;
#endif

	if (treq->local_buf != NULL) {
		sts = RTW_PHL_STATUS_FAILURE;
		goto error;
	}

#ifdef RTW_TX_COALESCE_BAK_PKT_LIST
	local_buf_size += treq->pkt_cnt * sizeof(local_buf->pkt_list[0]);
#endif
	treq->local_buf = _os_kmem_alloc(drv_priv, local_buf_size);
	if (!treq->local_buf) {
		sts = RTW_PHL_STATUS_RESOURCE;
		goto error;
	}

	local_buf = treq->local_buf;

#ifdef RTW_TX_COALESCE_BAK_PKT_LIST
	pkt_list = (struct rtw_pkt_buf_list *)treq->pkt_list;
	for (i = 0; i < treq->pkt_cnt; i++)
		local_buf->pkt_list[i] = pkt_list[i];
	local_buf->pkt_cnt = treq->pkt_cnt;
#endif

	if (ops->os_query_local_buf)
		ops->os_query_local_buf(drv_priv, local_buf);

	if (!local_buf->vir_addr) {
		_os_kmem_free(drv_priv, treq->local_buf,
		             sizeof(struct tx_local_buf));
		treq->local_buf = NULL;
		PHL_INFO("query local buffer fail!\n");
		sts = RTW_PHL_STATUS_RESOURCE;
		goto error;
	}

	sts = RTW_PHL_STATUS_SUCCESS;
	return sts;
error:
	if (treq->local_buf != NULL) {
		local_buf = treq->local_buf;
		if (local_buf->vir_addr) {
			if (ops->os_return_local_buf)
				ops->os_return_local_buf(drv_priv, local_buf);
		}
		_os_kmem_free(drv_priv, treq->local_buf,
		             sizeof(struct tx_local_buf));
		treq->local_buf = NULL;
	}
	return sts;
}

#ifdef CONFIG_DYNAMIC_RX_BUF
enum rtw_phl_status
_phl_alloc_dynamic_rxbuf_pcie(struct rtw_rx_buf *rx_buf,
					struct phl_info_t *phl_info)
{
	enum rtw_phl_status sts = RTW_PHL_STATUS_FAILURE;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	u32 buf_len = hal_com->bus_cap.rxbuf_size;

	rx_buf->cache = CACHE_ADDR;

	if (rx_buf->reuse) {
		rx_buf->reuse = false;
		if (rx_buf->cache == CACHE_ADDR)
			_os_pkt_buf_map_rx(drv_priv,
				&rx_buf->phy_addr_l,
				&rx_buf->phy_addr_h,
				buf_len,
				rx_buf->os_priv);
		return RTW_PHL_STATUS_SUCCESS;
	}

	if (rx_buf != NULL) {
		rx_buf->vir_addr = _os_pkt_buf_alloc_rx(
				drv_priv,
				&rx_buf->phy_addr_l,
				&rx_buf->phy_addr_h,
				buf_len,
				rx_buf->cache,
				&rx_buf->os_priv);
		if (NULL == rx_buf->vir_addr) {
			sts = RTW_PHL_STATUS_RESOURCE;
		} else {
			rx_buf->buf_len = buf_len;
			rx_buf->dynamic = 1;
			rx_buf->reuse = false;
			/* enqueue_idle_rx_buf(phl_info, rx_buf_ring, rx_buf); */
			sts = RTW_PHL_STATUS_SUCCESS;
		}
	}

	return sts;
}

static void enqueue_empty_rx_buf(
				struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring,
				struct rtw_rx_buf *rx_buf)
{
	_os_list *list = &rx_buf_ring->empty_rxbuf_list;

	_os_spinlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->empty_rxbuf_lock, _bh, NULL);
	list_add_tail(&rx_buf->list, list);
	rx_buf_ring->empty_rxbuf_cnt++;
	_os_spinunlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->empty_rxbuf_lock, _bh, NULL);
}

static struct rtw_rx_buf *query_empty_rx_buf(struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring)
{
	_os_list *rxbuf_list = &rx_buf_ring->empty_rxbuf_list;
	struct rtw_rx_buf *rx_buf = NULL;

	_os_spinlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->empty_rxbuf_lock, _bh, NULL);
	if (!list_empty(rxbuf_list)) {
		rx_buf = list_first_entry(rxbuf_list, struct rtw_rx_buf, list);
		rx_buf_ring->empty_rxbuf_cnt--;
		list_del(&rx_buf->list);
	}
	_os_spinunlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->empty_rxbuf_lock, _bh, NULL);

	return rx_buf;
}

static enum rtw_phl_status enqueue_idle_rx_buf(
				struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring,
				struct rtw_rx_buf *rx_buf);

static void refill_empty_rx_buf(
				struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring)
{
	enum rtw_phl_status pstatus;
	struct rtw_rx_buf *rx_buf = NULL;

	while (rx_buf_ring->empty_rxbuf_cnt) {
		rx_buf = query_empty_rx_buf(phl_info, rx_buf_ring);
		pstatus = _phl_alloc_dynamic_rxbuf_pcie(rx_buf, phl_info);
		if (RTW_PHL_STATUS_SUCCESS != pstatus) {
			enqueue_empty_rx_buf(phl_info, rx_buf_ring, rx_buf);
			break;
		}
		enqueue_idle_rx_buf(phl_info, rx_buf_ring, rx_buf);
	}
}
#endif /* CONFIG_DYNAMIC_RX_BUF */


static enum rtw_phl_status enqueue_busy_rx_buf(
				struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring,
				struct rtw_rx_buf *rx_buf, u8 pos)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	_os_list *list = &rx_buf_ring->busy_rxbuf_list;

	if (rx_buf != NULL) {
		_os_spinlock(phl_to_drvpriv(phl_info),
				&rx_buf_ring->busy_rxbuf_lock, _bh, NULL);
		if (_tail == pos)
			list_add_tail(&rx_buf->list, list);
		else if (_first == pos)
			list_add(&rx_buf->list, list);

		rx_buf_ring->busy_rxbuf_cnt++;
		_os_spinunlock(phl_to_drvpriv(phl_info),
				&rx_buf_ring->busy_rxbuf_lock, _bh, NULL);
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}


static enum rtw_phl_status enqueue_idle_rx_buf(
				struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring,
				struct rtw_rx_buf *rx_buf)
{
	_os_list *list;
	u32 clr_len;
	void *drvpriv;

	if (rx_buf == NULL)
		return RTW_PHL_STATUS_FAILURE;

	list = &rx_buf_ring->idle_rxbuf_list;
	drvpriv = phl_to_drvpriv(phl_info);

	clr_len = phl_get_ic_spec(phl_info->phl_com)->rx_bd_info_sz;

	_os_mem_set(drvpriv, rx_buf->vir_addr, 0, clr_len);

	#if defined(PHL_DMA_NONCOHERENT)
	/* Bidirectional $ WB to clear RX buf and invalidate $ */
	if (rx_buf->cache)
		_os_cache_wback(drvpriv,
				&rx_buf->phy_addr_l,
				&rx_buf->phy_addr_h,
				clr_len, DMA_BIDIRECTIONAL);
	#endif /* PHL_DMA_NONCOHERENT */
	/* Enqueue to idle Q */

	INIT_LIST_HEAD(&rx_buf->list);

	_os_spinlock(drvpriv, &rx_buf_ring->idle_rxbuf_lock, _bh, NULL);
	list_add_tail(&rx_buf->list, list);
	rx_buf_ring->idle_rxbuf_cnt++;
	_os_spinunlock(drvpriv, &rx_buf_ring->idle_rxbuf_lock, _bh, NULL);

	return RTW_PHL_STATUS_SUCCESS;
}

static struct rtw_rx_buf *query_busy_rx_buf(struct phl_info_t *phl_info,
					struct rtw_rx_buf_ring *rx_buf_ring)
{
	_os_list *busy_list = &rx_buf_ring->busy_rxbuf_list;
	struct rtw_rx_buf *rx_buf = NULL;

	_os_spinlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->busy_rxbuf_lock, _bh, NULL);
	if (true == list_empty(busy_list)) {
		rx_buf = NULL;
	} else {
		rx_buf = list_first_entry(busy_list, struct rtw_rx_buf, list);
		rx_buf_ring->busy_rxbuf_cnt--;
		list_del(&rx_buf->list);
	}
	_os_spinunlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->busy_rxbuf_lock, _bh, NULL);
	return rx_buf;
}

static struct rtw_rx_buf *query_idle_rx_buf(struct phl_info_t *phl_info,
					struct rtw_rx_buf_ring *rx_buf_ring)
{
	_os_list *idle_list = &rx_buf_ring->idle_rxbuf_list;
	struct rtw_rx_buf *rx_buf = NULL;

	_os_spinlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->idle_rxbuf_lock, _bh, NULL);
	if (true == list_empty(idle_list)) {
		rx_buf = NULL;
	} else {
		rx_buf = list_first_entry(idle_list, struct rtw_rx_buf, list);
		rx_buf_ring->idle_rxbuf_cnt--;
		list_del(&rx_buf->list);
	}
	_os_spinunlock(phl_to_drvpriv(phl_info),
			&rx_buf_ring->idle_rxbuf_lock, _bh, NULL);

	return rx_buf;
}

enum rtw_phl_status
phl_release_target_rx_buf(struct phl_info_t *phl_info, void *r, u8 ch,
				enum rtw_rx_type type)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_rx_buf_ring *rx_buf_ring = NULL;
	struct rtw_rx_buf *rx_buf = (struct rtw_rx_buf *)r;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;

	rx_buf_ring = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;

	if (rx_buf) {
#ifdef CONFIG_DYNAMIC_RX_BUF
		if (type == RTW_RX_TYPE_WIFI)
			_phl_alloc_dynamic_rxbuf_pcie(rx_buf, phl_info);
		if (NULL == rx_buf->vir_addr) {
			enqueue_empty_rx_buf(phl_info, &rx_buf_ring[ch], rx_buf);
#ifdef DEBUG_PHL_RX
			phl_info->rx_stats.rxbuf_empty++;
#endif
		} else
#endif
		enqueue_idle_rx_buf(phl_info, &rx_buf_ring[ch], rx_buf);
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}

static enum rtw_phl_status phl_release_busy_rx_buf(
				struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring,
				u16 release_num)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_rx_buf *rx_buf = NULL;

	if (rx_buf_ring != NULL) {

		while (release_num > 0) {
			rx_buf = query_busy_rx_buf(phl_info, rx_buf_ring);
			if (NULL == rx_buf)
				break;
			enqueue_idle_rx_buf(phl_info, rx_buf_ring, rx_buf);
			release_num--;
		}
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}
	return pstatus;
}



/* static void rtl8852ae_free_wd_page_buf(_adapter *adapter, void *vir_addr, */
/* 				dma_addr_t *bus_addr, size_t size) */
/* { */
/* 	struct platform_ops *ops = &adapter->platform_func; */
/* 	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter); */
/* 	struct pci_dev *pdev = dvobj->ppcidev; */

/* 	FUNCIN(); */
/* 	ops->free_cache_mem(pdev,vir_addr, bus_addr, size, DMA_TO_DEVICE); */

/* 	/\* NONCACHE hana_todo */
/* 	 * ops->alloc_noncache_mem(pdev, vir_addr, bus_addr, size); */
/* 	 *\/ */
/* 	FUNCOUT(); */
/* } */

static void _phl_free_rxbuf_pcie(struct phl_info_t *phl_info,
				 struct rtw_rx_buf *rx_buf, u8 ch_idx)
{
	u16 rxbuf_num = rtw_hal_get_rxbuf_num(phl_info->hal, ch_idx);
	u16 i = 0;

	if (NULL != rx_buf) {
		for (i = 0; i < rxbuf_num; i++) {

			if (NULL == rx_buf[i].vir_addr)
				continue;
			_os_pkt_buf_free_rx(phl_to_drvpriv(phl_info),
					    rx_buf[i].vir_addr,
					    rx_buf[i].phy_addr_l,
					    rx_buf[i].phy_addr_h,
					    rx_buf[i].buf_len,
					    rx_buf[i].cache,
					    rx_buf[i].os_priv);
			rx_buf[i].vir_addr = NULL;
			rx_buf[i].cache = NONCACHE_ADDR;
		}

		_os_mem_free(phl_to_drvpriv(phl_info), rx_buf,
					sizeof(struct rtw_rx_buf) * rxbuf_num);
	}
}

static void _phl_free_rxbuf_pool_pcie(struct phl_info_t *phl_info,
						u8 *rxbuf_pool, u8 ch_num)
{
	struct rtw_rx_buf_ring *ring = NULL;
	u8 i = 0;

	FUNCIN();
	ring = (struct rtw_rx_buf_ring *)rxbuf_pool;
	if (NULL != ring) {
		for (i = 0; i < ch_num; i++) {

			ring[i].idle_rxbuf_cnt = 0;
#ifdef CONFIG_DYNAMIC_RX_BUF
			ring[i].empty_rxbuf_cnt = 0;
#endif

			if (NULL == ring[i].rx_buf)
				continue;

			_phl_free_rxbuf_pcie(phl_info, ring[i].rx_buf, i);
			ring[i].rx_buf = NULL;
			_os_spinlock_free(phl_to_drvpriv(phl_info),
					&ring[i].idle_rxbuf_lock);
			_os_spinlock_free(phl_to_drvpriv(phl_info),
					&ring[i].busy_rxbuf_lock);
#ifdef CONFIG_DYNAMIC_RX_BUF
			_os_spinlock_free(phl_to_drvpriv(phl_info),
					&ring[i].empty_rxbuf_lock);
#endif
		}
		_os_mem_free(phl_to_drvpriv(phl_info), ring,
			     sizeof(struct rtw_rx_buf_ring) * ch_num);
	}

	FUNCOUT();
}

static void _phl_destory_dma_pool_pcie(struct phl_info_t *phl_info, void *pool)
{
	_os_dma_pool_destory(phl_to_drvpriv(phl_info), pool);
}

/* static void *rtl8852ae_alloc_wd_page_buf(_adapter *adapter, */
/* 					 dma_addr_t *bus_addr, size_t size) */
/* { */
/* 	struct platform_ops *ops = &adapter->platform_func; */
/* 	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter); */
/* 	struct pci_dev *pdev = dvobj->ppcidev; */
/* 	void *vir_addr = NULL; */

/* 	FUNCIN(); */
/* 	vir_addr = ops->alloc_cache_mem(pdev, bus_addr, size, DMA_TO_DEVICE); */

/* 	/\* NONCACHE hana_todo */
/* 	 * vir_addr = ops->alloc_noncache_mem(pdev, bus_addr, size); */
/* 	 *\/ */

/* 	FUNCOUT(); */
/* 	return vir_addr; */
/* } */

static 	struct rtw_rx_buf *
_phl_alloc_rxbuf_pcie(struct phl_info_t *phl_info,
				struct rtw_rx_buf_ring *rx_buf_ring, u8 ch_idx, u32 rx_buf_size)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_rx_buf *rx_buf = NULL;
	u32 buf_len = 0;
	u16 rxbuf_num = rtw_hal_get_rxbuf_num(phl_info->hal, ch_idx);
	void *drv_priv = phl_to_drvpriv(phl_info);
	int i;
#ifdef CONFIG_RX_BUFF_NONCACHE_ADDR
	enum cache_addr_type cache = NONCACHE_ADDR;
#else
	enum cache_addr_type cache = CACHE_ADDR;
#endif

	buf_len = sizeof(*rx_buf) * rxbuf_num;
	rx_buf = _os_mem_alloc(drv_priv, buf_len);
	buf_len = rx_buf_size;
	if (rx_buf != NULL) {
		for (i = 0; i < rxbuf_num; i++) {
			rx_buf[i].cache = cache;
			rx_buf[i].vir_addr = _os_pkt_buf_alloc_rx(
					phl_to_drvpriv(phl_info),
					&rx_buf[i].phy_addr_l,
					&rx_buf[i].phy_addr_h,
					buf_len,
					rx_buf[i].cache,
					&rx_buf[i].os_priv);
			if (NULL == rx_buf[i].vir_addr) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}
			rx_buf[i].buf_len = buf_len;
			rx_buf[i].dynamic = 0;
#ifdef CONFIG_DYNAMIC_RX_BUF
			rx_buf[i].reuse = false;
#endif

			INIT_LIST_HEAD(&rx_buf[i].list);
			enqueue_idle_rx_buf(phl_info, rx_buf_ring, &rx_buf[i]);
			pstatus = RTW_PHL_STATUS_SUCCESS;
				/* hana_todo now check 4 byte align only */
			/* if ((unsigned long)wd_page_buf & 0xF) { */
			/* 	res = _FAIL; */
			/* 	break; */
			/* } */
		}
	}

	if (RTW_PHL_STATUS_SUCCESS != pstatus) {
		_phl_free_rxbuf_pcie(phl_info, rx_buf, ch_idx);
		rx_buf = NULL;
	}
	return rx_buf;
}



static enum rtw_phl_status
_phl_alloc_rxbuf_pool_pcie(struct phl_info_t *phl_info, u8 ch_num)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_rx_buf_ring *rx_buf_ring = NULL;
	struct rtw_rx_buf *rx_buf = NULL;
	u32 buf_len = 0;
	u16 rxbuf_num = 0;
	u8 i = 0;
	FUNCIN_WSTS(pstatus);

	buf_len = sizeof(*rx_buf_ring) * ch_num;
	rx_buf_ring = _os_mem_alloc(phl_to_drvpriv(phl_info), buf_len);

	if (NULL != rx_buf_ring) {
		for (i = 0; i < ch_num; i++) {
			_os_spinlock_init(phl_to_drvpriv(phl_info),
					&rx_buf_ring[i].idle_rxbuf_lock);
			_os_spinlock_init(phl_to_drvpriv(phl_info),
					&rx_buf_ring[i].busy_rxbuf_lock);
			INIT_LIST_HEAD(&rx_buf_ring[i].idle_rxbuf_list);
			INIT_LIST_HEAD(&rx_buf_ring[i].busy_rxbuf_list);
#ifdef CONFIG_DYNAMIC_RX_BUF
			_os_spinlock_init(phl_to_drvpriv(phl_info),
					&rx_buf_ring[i].empty_rxbuf_lock);
			INIT_LIST_HEAD(&rx_buf_ring[i].empty_rxbuf_list);
#endif
			rxbuf_num = rtw_hal_get_rxbuf_num(phl_info->hal, i);
			buf_len = rtw_hal_get_rxbuf_size(phl_info->hal, i);
			/* PHL_INFO("[band:%d][ch:%d] rxbuf_num:%d size:%d\n",
				phl_info->phl_com->wifi_roles[0].chandef.band,
				i, rxbuf_num, hal_com->bus_cap.rxbuf_size); */

			rx_buf = _phl_alloc_rxbuf_pcie(phl_info,
							&rx_buf_ring[i], i, buf_len);
			if (NULL == rx_buf) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}
			rx_buf_ring[i].rx_buf = rx_buf;
			rx_buf_ring[i].idle_rxbuf_cnt = rxbuf_num;
			rx_buf_ring[i].busy_rxbuf_cnt = 0;
			pstatus = RTW_PHL_STATUS_SUCCESS;
		}
	}

	if (RTW_PHL_STATUS_SUCCESS == pstatus) {
		phl_info->hci->rxbuf_pool = (u8 *)rx_buf_ring;
	} else
		_phl_free_rxbuf_pool_pcie(phl_info, (u8 *)rx_buf_ring, ch_num);
	FUNCOUT_WSTS(pstatus);

	return pstatus;
}




/* static void rtl8852ae_free_wd_page_buf(_adapter *adapter, void *vir_addr, */
/* 				dma_addr_t *bus_addr, size_t size) */
/* { */
/* 	struct platform_ops *ops = &adapter->platform_func; */
/* 	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter); */
/* 	struct pci_dev *pdev = dvobj->ppcidev; */

/* 	FUNCIN(); */
/* 	ops->free_cache_mem(pdev,vir_addr, bus_addr, size, DMA_TO_DEVICE); */

/* 	/\* NONCACHE hana_todo */
/* 	 * ops->alloc_noncache_mem(pdev, vir_addr, bus_addr, size); */
/* 	 *\/ */
/* 	FUNCOUT(); */
/* } */

static void _phl_free_wd_page_pcie(struct phl_info_t *phl_info,
					struct rtw_wd_page_ring *wd_page_ring,
					struct rtw_wd_page *wd_page)
{
	void *rtw_dma_pool = NULL;
	u16 i = 0, wd_num = 0;
#ifdef CONFIG_WD_PAGE_NONCACHE_ADDR
	rtw_dma_pool = phl_info->hci->wd_dma_pool;
#endif

	wd_num = phl_info->phl_com->bus_sw_cap.wd_num ?
		 phl_info->phl_com->bus_sw_cap.wd_num : MAX_WD_PAGE_NUM;

#ifdef RTW_WD_PAGE_USE_SHMEM_POOL
	if (WD_PAGE_SHMEM_POOL_VALID(wd_page_ring)) {
		PHL_INFO("wd_page free aligned shmem pool virtual addr= %p\n",
		          wd_page_ring->wd_page_shmem_pool.vir_addr);
		_os_shmem_free(phl_to_drvpriv(phl_info),
		               rtw_dma_pool,
		               wd_page_ring->wd_page_shmem_pool.vir_addr,
		               &wd_page_ring->wd_page_shmem_pool.phy_addr_l,
		               &wd_page_ring->wd_page_shmem_pool.phy_addr_h,
		               wd_page_ring->wd_page_shmem_pool.buf_len,
		               CACHE_ADDR,
		               DMA_FROM_DEVICE,
		               wd_page_ring->wd_page_shmem_pool.os_rsvd[0]);
		wd_page_ring->wd_page_shmem_pool.vir_addr = NULL;
	} else
#endif
	{
		for (i = 0; i < wd_num; i++) {

			if (NULL == wd_page || NULL == wd_page[i].vir_addr)
				continue;

			wd_page[i].wp_seq = WP_RESERVED_SEQ;
			_os_shmem_free(phl_to_drvpriv(phl_info),
			               rtw_dma_pool,
			               wd_page[i].vir_addr,
			               &wd_page[i].phy_addr_l,
			               &wd_page[i].phy_addr_h,
			               WD_PAGE_SIZE,
			               wd_page[i].cache,
			               DMA_FROM_DEVICE,
			               wd_page[i].os_rsvd[0]);
			wd_page[i].vir_addr = NULL;
			wd_page[i].cache = NONCACHE_ADDR;
		}
	}

	if (NULL != wd_page) {
		_os_mem_free(phl_to_drvpriv(phl_info), wd_page,
					sizeof(struct rtw_wd_page) * wd_num);
	}
}

static void _phl_free_wd_ring_pcie(struct phl_info_t *phl_info, u8 *wd_page_buf,
					u8 ch_num)
{
	struct rtw_wd_page_ring *wd_page_ring = NULL;
	void *drv_priv = phl_to_drvpriv(phl_info);
	u8 i = 0;
	FUNCIN();

	wd_page_ring = (struct rtw_wd_page_ring *)wd_page_buf;
	if (NULL != wd_page_ring) {
		for (i = 0; i < ch_num; i++) {

			wd_page_ring[i].idle_wd_page_cnt = 0;

			if (NULL == wd_page_ring[i].wd_page)
				continue;

			if (i == rtw_hal_get_fwcmd_queue_idx(phl_info->hal)) {
				_phl_free_h2c_work_ring(phl_info,
							&wd_page_ring[i]);
			}
			_phl_free_wd_work_ring(phl_info, &wd_page_ring[i]);
			_phl_free_wd_page_pcie(phl_info, &wd_page_ring[i],
			                       wd_page_ring[i].wd_page);
			wd_page_ring[i].wd_page = NULL;
			_os_spinlock_free(drv_priv,
						&wd_page_ring[i].idle_lock);
			_os_spinlock_free(drv_priv,
						&wd_page_ring[i].busy_lock);
			_os_spinlock_free(drv_priv,
						&wd_page_ring[i].pending_lock);
			_os_spinlock_free(drv_priv,
						&wd_page_ring[i].work_lock);
			_os_spinlock_free(drv_priv,
						&wd_page_ring[i].wp_tag_lock);
		}
		_os_mem_free(phl_to_drvpriv(phl_info), wd_page_ring,
						sizeof(struct rtw_wd_page_ring) * ch_num);
	}
	FUNCOUT();
}

/* static void *rtl8852ae_alloc_wd_page_buf(_adapter *adapter, */
/* 					 dma_addr_t *bus_addr, size_t size) */
/* { */
/* 	struct platform_ops *ops = &adapter->platform_func; */
/* 	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter); */
/* 	struct pci_dev *pdev = dvobj->ppcidev; */
/* 	void *vir_addr = NULL; */

/* 	FUNCIN(); */
/* 	vir_addr = ops->alloc_cache_mem(pdev, bus_addr, size, DMA_TO_DEVICE); */

/* 	/\* NONCACHE hana_todo */
/* 	 * vir_addr = ops->alloc_noncache_mem(pdev, bus_addr, size); */
/* 	 *\/ */

/* 	FUNCOUT(); */
/* 	return vir_addr; */
/* } */

#ifdef RTW_WD_PAGE_USE_SHMEM_POOL
static void
_phl_cut_shmem_to_wd_page(struct rtw_wd_page *wd_page,
                          _os_list *list,
                          struct rtw_shmem_pool *original_shmem,
                          u32 length,
                          u16 count,
                          u32 alignment)
{
	/* refer to MemoryCutAlignedSharedMemory */
	u32 offset, i;
	u32 original_phy_addr_h = original_shmem->phy_addr_h;
	u32 original_phy_addr_l = original_shmem->phy_addr_l;
	u8* original_vir_addr = original_shmem->vir_addr;
	struct rtw_wd_page *cur;
	struct rtw_wd_page *prev;

	/* to make alignment, first memory needs phy_addr_l to know offset */
	wd_page->buf_len = length;
	wd_page->phy_addr_l = ALIGNMENT_MEMORY_ROUND_UP(original_phy_addr_l, alignment);
	wd_page->phy_addr_h = original_phy_addr_h;
	wd_page->cache = CACHE_ADDR;
	offset = wd_page->phy_addr_l - original_phy_addr_l;
	wd_page->vir_addr = original_vir_addr + offset;
	wd_page->wp_seq = WP_RESERVED_SEQ;
	wd_page->os_rsvd[0] = original_shmem->os_rsvd[0];
	INIT_LIST_HEAD(&wd_page->list);
	list_add_tail(&wd_page->list, list);
	prev = wd_page;

	for(i = 1; i < count; i++) {
		/* continue to cut and keep alignment */
		/* ...|<-prev + length + R-UP |<-cur */
		cur = (wd_page + i);
		cur->buf_len = length;
		cur->phy_addr_l = prev->phy_addr_l + ALIGNMENT_MEMORY_ROUND_UP(length, alignment);
		cur->phy_addr_h = prev->phy_addr_h;
		cur->cache = CACHE_ADDR;
		cur->vir_addr = prev->vir_addr + ALIGNMENT_MEMORY_ROUND_UP(length, alignment);
		cur->wp_seq = WP_RESERVED_SEQ;
		cur->os_rsvd[0] = original_shmem->os_rsvd[0];
		INIT_LIST_HEAD(&cur->list);
		list_add_tail(&cur->list, list);
		prev = cur;
	}
}
#endif

static struct rtw_wd_page *_phl_alloc_wd_page_pcie(
			struct phl_info_t *phl_info, _os_list *list, struct rtw_wd_page_ring *wd_page_ring)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_wd_page *wd_page = NULL;
	void *drv_priv = phl_to_drvpriv(phl_info);
	void *rtw_dma_pool = NULL;
	u16 wd_num = 0;
	u32 i = 0, buf_len = 0;
#ifdef CONFIG_WD_PAGE_NONCACHE_ADDR
	u8 wd_cache_type = NONCACHE_ADDR;
	rtw_dma_pool = phl_info->hci->wd_dma_pool;
#else
	u8 wd_cache_type = CACHE_ADDR;
#endif
#ifdef RTW_WD_PAGE_USE_SHMEM_POOL
	u32 alignment = 0x10;
#endif
	wd_num = phl_info->phl_com->bus_sw_cap.wd_num ?
		 phl_info->phl_com->bus_sw_cap.wd_num : MAX_WD_PAGE_NUM;

	buf_len = sizeof(*wd_page) * wd_num;
	wd_page = _os_mem_alloc(drv_priv, buf_len);
	if (wd_page != NULL) {
#ifdef RTW_WD_PAGE_USE_SHMEM_POOL
		/* try to alloc shmem pool */
		_os_mem_set(drv_priv, &wd_page_ring->wd_page_shmem_pool, 0, sizeof(struct rtw_shmem_pool));
		buf_len = ALIGNMENT_MEMORY_POOL_LENGTH(WD_PAGE_SIZE, wd_num, alignment); /* alignment size */
		wd_page_ring->wd_page_shmem_pool.vir_addr = _os_shmem_alloc(
		                                               drv_priv,
		                                               rtw_dma_pool,
		                                               &wd_page_ring->wd_page_shmem_pool.phy_addr_l,
		                                               &wd_page_ring->wd_page_shmem_pool.phy_addr_h,
		                                               buf_len,
		                                               CACHE_ADDR,
		                                               DMA_TO_DEVICE,
		                                               &wd_page_ring->wd_page_shmem_pool.os_rsvd[0]);
		if (WD_PAGE_SHMEM_POOL_VALID(wd_page_ring)) {
			wd_page_ring->wd_page_shmem_pool.buf_len = buf_len;
			_phl_cut_shmem_to_wd_page(wd_page,
			                          list,
			                          &wd_page_ring->wd_page_shmem_pool,
			                          WD_PAGE_SIZE,
			                          wd_num,
			                          alignment);
			PHL_INFO("wd_page init aligned shmem pool virtual addr=%p buf_len=%d wd_num=%d wd_page_size=%d successfully\n",
			         wd_page_ring->wd_page_shmem_pool.vir_addr,
			         wd_page_ring->wd_page_shmem_pool.buf_len,
			         wd_num,
			         WD_PAGE_SIZE);
			pstatus = RTW_PHL_STATUS_SUCCESS;
		} else
#endif
		{
			for (i = 0; i < wd_num; i++) {
				wd_page[i].cache = wd_cache_type;
				buf_len = WD_PAGE_SIZE;
				wd_page[i].vir_addr = _os_shmem_alloc(drv_priv,
				                                     rtw_dma_pool,
				                                     &wd_page[i].phy_addr_l,
				                                     &wd_page[i].phy_addr_h,
				                                     buf_len,
				                                     wd_page[i].cache,
				                                     DMA_TO_DEVICE,
				                                     &wd_page[i].os_rsvd[0]);
				if (NULL == wd_page[i].vir_addr) {
					pstatus = RTW_PHL_STATUS_RESOURCE;
					break;
				}
				wd_page[i].buf_len = buf_len;
				wd_page[i].wp_seq = WP_RESERVED_SEQ;
				INIT_LIST_HEAD(&wd_page[i].list);

				list_add_tail(&wd_page[i].list, list);

				pstatus = RTW_PHL_STATUS_SUCCESS;
					/* hana_todo now check 4 byte align only */
				/* if ((unsigned long)wd_page_buf & 0xF) { */
				/* 	res = _FAIL; */
				/* 	break; */
				/* } */
			}
		}
	}

	if (RTW_PHL_STATUS_SUCCESS != pstatus) {
		_phl_free_wd_page_pcie(phl_info, wd_page_ring, wd_page);
		wd_page = NULL;
	}

	return wd_page;
}



static enum rtw_phl_status
_phl_alloc_wd_ring_pcie(struct phl_info_t *phl_info, u8 ch_num)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_wd_page_ring *wd_page_ring = NULL;
	struct rtw_wd_page *wd_page = NULL;
	void *drv_priv = NULL;
	u16 wd_num = 0;
	u32 i = 0, buf_len = 0;

	FUNCIN_WSTS(pstatus);

	wd_num = phl_info->phl_com->bus_sw_cap.wd_num ?
		 phl_info->phl_com->bus_sw_cap.wd_num : MAX_WD_PAGE_NUM;
	drv_priv = phl_to_drvpriv(phl_info);
	buf_len = sizeof(struct rtw_wd_page_ring) * ch_num;
	wd_page_ring = _os_mem_alloc(phl_to_drvpriv(phl_info), buf_len);
	if (NULL != wd_page_ring) {
		for (i = 0; i < ch_num; i++) {
			INIT_LIST_HEAD(&wd_page_ring[i].idle_wd_page_list);
			INIT_LIST_HEAD(&wd_page_ring[i].busy_wd_page_list);
			INIT_LIST_HEAD(&wd_page_ring[i].pending_wd_page_list);
			_os_spinlock_init(drv_priv,
						&wd_page_ring[i].idle_lock);
			_os_spinlock_init(drv_priv,
						&wd_page_ring[i].busy_lock);
			_os_spinlock_init(drv_priv,
						&wd_page_ring[i].pending_lock);
			_os_spinlock_init(drv_priv,
						&wd_page_ring[i].work_lock);
			_os_spinlock_init(drv_priv,
						&wd_page_ring[i].wp_tag_lock);

			wd_page = _phl_alloc_wd_page_pcie(phl_info,
					&wd_page_ring[i].idle_wd_page_list,
					&wd_page_ring[i]);
			if (NULL == wd_page) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}

			pstatus = _phl_alloc_wd_work_ring(phl_info,
							  &wd_page_ring[i]);
			if (RTW_PHL_STATUS_SUCCESS != pstatus)
				break;

			if (i == rtw_hal_get_fwcmd_queue_idx(phl_info->hal)) {
				pstatus = _phl_alloc_h2c_work_ring(phl_info,
							     &wd_page_ring[i]);
				if (RTW_PHL_STATUS_SUCCESS != pstatus)
					break;
			}
			wd_page_ring[i].wd_page = wd_page;
			wd_page_ring[i].idle_wd_page_cnt = wd_num;
			wd_page_ring[i].busy_wd_page_cnt = 0;
			wd_page_ring[i].pending_wd_page_cnt = 0;
			wd_page_ring[i].wp_seq = 1;
			pstatus = RTW_PHL_STATUS_SUCCESS;
		}
	}

	if (RTW_PHL_STATUS_SUCCESS == pstatus) {
		phl_info->hci->wd_ring = (u8 *)wd_page_ring;
	} else
		_phl_free_wd_ring_pcie(phl_info, (u8 *)wd_page_ring, ch_num);
	FUNCOUT_WSTS(pstatus);

	return pstatus;
}

static void _phl_free_h2c_pkt_buf_pcie(struct phl_info_t *phl_info,
				struct rtw_h2c_pkt *_h2c_pkt)
{
	void *rtw_dma_pool = NULL;
	struct rtw_h2c_pkt *h2c_pkt = _h2c_pkt;

#ifdef CONFIG_H2C_NONCACHE_ADDR
	if (h2c_pkt->buf_len < H2C_LONG_DATA_LEN)
		rtw_dma_pool = phl_info->hci->h2c_dma_pool;
#endif
	_os_shmem_free(phl_to_drvpriv(phl_info),
				rtw_dma_pool,
				h2c_pkt->vir_head,
				&h2c_pkt->phy_addr_l,
				&h2c_pkt->phy_addr_h,
				h2c_pkt->buf_len,
				h2c_pkt->cache,
				DMA_FROM_DEVICE,
				h2c_pkt->os_rsvd[0]);
}

enum rtw_phl_status _phl_alloc_h2c_pkt_buf_pcie(struct phl_info_t *phl_info,
	struct rtw_h2c_pkt *_h2c_pkt, u32 buf_len)
{
	void *rtw_dma_pool = NULL;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_h2c_pkt *h2c_pkt = _h2c_pkt;

#ifdef CONFIG_H2C_NONCACHE_ADDR
	if (h2c_pkt->buf_len < H2C_LONG_DATA_LEN)
		rtw_dma_pool = phl_info->hci->h2c_dma_pool;
#endif
	h2c_pkt->vir_head = _os_shmem_alloc(
				phl_to_drvpriv(phl_info),
				rtw_dma_pool,
				&h2c_pkt->phy_addr_l,
				&h2c_pkt->phy_addr_h,
				buf_len,
				h2c_pkt->cache,
				DMA_TO_DEVICE,
				&h2c_pkt->os_rsvd[0]);

	if (h2c_pkt->vir_head)
		pstatus = RTW_PHL_STATUS_SUCCESS;

	return pstatus;
}

static void _phl_free_rxbd_pcie(struct phl_info_t *phl_info,
						u8 *rxbd_buf, u8 ch_num)
{
	void *rtw_dma_pool = NULL;
	struct rx_base_desc *rxbd = (struct rx_base_desc *)rxbd_buf;
	u8 i = 0;

	FUNCIN();

	if (NULL != rxbd) {
		for (i = 0; i < ch_num; i++) {

			if (NULL == rxbd[i].vir_addr)
				continue;
			_os_shmem_free(phl_to_drvpriv(phl_info),
						rtw_dma_pool,
						rxbd[i].vir_addr,
						&rxbd[i].phy_addr_l,
						&rxbd[i].phy_addr_h,
						rxbd[i].buf_len,
						rxbd[i].cache,
						DMA_FROM_DEVICE,
						rxbd[i].os_rsvd[0]);
			rxbd[i].vir_addr = NULL;
			rxbd[i].cache = NONCACHE_ADDR;
		}

		_os_mem_free(phl_to_drvpriv(phl_info), rxbd,
					sizeof(struct rx_base_desc) * ch_num);
	}
	FUNCOUT();
}


static enum rtw_phl_status
_phl_alloc_rxbd_pcie(struct phl_info_t *phl_info, u8 ch_num)
{
	void *rtw_dma_pool = NULL;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct rx_base_desc *rxbd = NULL;
	u32 buf_len = 0;
	u16 rxbd_num = 0;
	u8 rxbd_len = hal_com->bus_hw_cap.rxbd_len;
	u8 i = 0;
#ifdef CONFIG_RXBD_NONCACHE_ADDR
	enum cache_addr_type cache = NONCACHE_ADDR;
#else
	enum cache_addr_type cache = CACHE_ADDR;
#endif
	FUNCIN_WSTS(pstatus);

	buf_len = sizeof(struct rx_base_desc) * ch_num;
	rxbd = _os_mem_alloc(phl_to_drvpriv(phl_info), buf_len);
	if (NULL != rxbd) {
		for (i = 0; i < ch_num; i++) {
			rxbd_num = rtw_hal_get_rxbd_num(phl_info->hal, i);
			rxbd[i].cache = cache;
			buf_len = rxbd_len * rxbd_num;
			PHL_INFO("[band:%d][ch:%d] rxbd_num=%d buf_len=%d\n",
				phl_info->phl_com->wifi_roles[0].rlink[RTW_RLINK_PRIMARY].chandef.band, i, rxbd_num, buf_len);
			rxbd[i].vir_addr = _os_shmem_alloc(
						phl_to_drvpriv(phl_info), rtw_dma_pool,
						&rxbd[i].phy_addr_l,
						&rxbd[i].phy_addr_h,
						buf_len,
						rxbd[i].cache,
						DMA_TO_DEVICE,
						&rxbd[i].os_rsvd[0]);
			if (NULL == rxbd[i].vir_addr) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}
			rxbd[i].buf_len = buf_len;
			rxbd[i].host_idx = 0;
			rxbd[i].hw_idx = 0;
			pstatus = RTW_PHL_STATUS_SUCCESS;
		}
	}

	if (RTW_PHL_STATUS_SUCCESS == pstatus)
		phl_info->hci->rxbd_buf = (u8 *)rxbd;
	else
		_phl_free_rxbd_pcie(phl_info, (u8 *)rxbd, ch_num);
	FUNCOUT_WSTS(pstatus);

	return pstatus;
}


static void _phl_free_txbd_pcie(struct phl_info_t *phl_info, u8 *txbd_buf,
				u8 ch_num)
{
	void *rtw_dma_pool = NULL;
	struct tx_base_desc *txbd = (struct tx_base_desc *)txbd_buf;
	u8 i = 0;
	FUNCIN();

	if (NULL != txbd) {
		for (i = 0; i < ch_num; i++) {

			if (NULL == txbd[i].vir_addr)
				continue;
			_os_shmem_free(phl_to_drvpriv(phl_info),
						rtw_dma_pool,
						txbd[i].vir_addr,
						&txbd[i].phy_addr_l,
						&txbd[i].phy_addr_h,
						txbd[i].buf_len,
						txbd[i].cache,
						DMA_FROM_DEVICE,
						txbd[i].os_rsvd[0]);
			txbd[i].vir_addr = NULL;
			txbd[i].cache = NONCACHE_ADDR;
			_os_spinlock_free(phl_to_drvpriv(phl_info),
						&txbd[i].txbd_lock);
		}

		_os_mem_free(phl_to_drvpriv(phl_info), txbd,
						sizeof(struct tx_base_desc) * ch_num);
	}

	FUNCOUT();
}



static enum rtw_phl_status
_phl_alloc_txbd_pcie(struct phl_info_t *phl_info, u8 ch_num)
{
	void *rtw_dma_pool = NULL;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct tx_base_desc *txbd = NULL;
	u32 buf_len = 0;
	u16 txbd_num = hal_com->bus_cap.txbd_num;
	u8 txbd_len = hal_com->bus_hw_cap.txbd_len;
	u8 i = 0;
#ifdef CONFIG_TXBD_NONCACHE_ADDR
	enum cache_addr_type cache = NONCACHE_ADDR;
#else
	enum cache_addr_type cache = CACHE_ADDR;
#endif

	FUNCIN_WSTS(pstatus);
	buf_len = sizeof(struct tx_base_desc) * ch_num;
	txbd = _os_mem_alloc(phl_to_drvpriv(phl_info), buf_len);
	if (NULL != txbd) {
		for (i = 0; i < ch_num; i++) {
			txbd[i].cache = cache;
			buf_len = txbd_len * txbd_num;
			txbd[i].vir_addr = _os_shmem_alloc(
						phl_to_drvpriv(phl_info), rtw_dma_pool,
						&txbd[i].phy_addr_l,
						&txbd[i].phy_addr_h,
						buf_len,
						txbd[i].cache,
						DMA_TO_DEVICE,
						&txbd[i].os_rsvd[0]);
			if (NULL == txbd[i].vir_addr) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}
			txbd[i].buf_len = buf_len;
			txbd[i].avail_num = txbd_num;
			_os_spinlock_init(phl_to_drvpriv(phl_info),
						&txbd[i].txbd_lock);
			pstatus = RTW_PHL_STATUS_SUCCESS;
		}
	}

	if (RTW_PHL_STATUS_SUCCESS == pstatus)
		phl_info->hci->txbd_buf = (u8 *)txbd;
	else
		_phl_free_txbd_pcie(phl_info, (u8 *)txbd, ch_num);
	FUNCOUT_WSTS(pstatus);

	return pstatus;
}

enum rtw_phl_status _phl_update_default_rx_bd(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct dvobj_priv *pobj = phl_to_drvpriv(phl_info);
	struct pci_dev *pdev = dvobj_to_pci(pobj)->ppcidev;
	struct rx_base_desc *rxbd = NULL;
	struct rtw_rx_buf_ring *ring = NULL;
	struct rtw_rx_buf *rxbuf = NULL;
	u8 i = 0;
	u16 rxbd_num = 0;
	u32 j = 0;

	rxbd = (struct rx_base_desc *)hci_info->rxbd_buf;
	ring = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;
	for (i = 0; i < hci_info->total_rxch_num; i++) {
		rxbd_num = rtw_hal_get_rxbd_num(phl_info->hal, i);
		for (j = 0; j < rxbd_num; j++) {
			rxbuf = query_idle_rx_buf(phl_info, &ring[i]);
			if (NULL == rxbuf) {
				PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
					"[WARNING] there is no resource for rx bd default setting\n");
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}

			hstatus = rtw_hal_update_rxbd(phl_info->hal, &rxbd[i],
								rxbuf, i);
			if (RTW_HAL_STATUS_SUCCESS == hstatus) {
				enqueue_busy_rx_buf(phl_info, &ring[i], rxbuf, _tail);
				pstatus = RTW_PHL_STATUS_SUCCESS;
			} else {
				enqueue_idle_rx_buf(phl_info, &ring[i], rxbuf);
				PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_, "[WARNING] update rx bd fail\n");
				pstatus = RTW_PHL_STATUS_FAILURE;
				break;
			}
		}

		if (rxbd[i].cache == CACHE_ADDR)
			pci_cache_wback(pdev, (dma_addr_t *)&rxbd[i].phy_addr_l, rxbd[i].buf_len, DMA_TO_DEVICE);
	}

	return pstatus;
}

static void _phl_reset_rxbd(struct phl_info_t *phl_info,
					struct rx_base_desc *rxbd, u8 ch_idx)
{
	_os_mem_set(phl_to_drvpriv(phl_info), rxbd->vir_addr, 0, rxbd->buf_len);
	rxbd->host_idx = 0;
	rxbd->hw_idx = 0;
}


static void phl_rx_reset_pcie(struct phl_info_t *phl_info)
{
	struct rtw_phl_com_t *phl_com = phl_info->phl_com;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct hal_spec_t *hal_spec = &phl_com->hal_spec;
	struct rx_base_desc *rxbd = NULL;
	struct rtw_rx_buf_ring *ring = NULL;
	u8 ch = 0;

	rxbd = (struct rx_base_desc *)hci_info->rxbd_buf;
	ring = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;

	for (ch = 0; ch < hci_info->total_rxch_num; ch++) {
		_phl_reset_rxbd(phl_info, &rxbd[ch], ch);
		phl_release_busy_rx_buf(phl_info, &ring[ch],
					ring[ch].busy_rxbuf_cnt);
	}
	hal_spec->rx_tag[0] = 0;
	hal_spec->rx_tag[1] = 0;
	_phl_update_default_rx_bd(phl_info);

}


void _phl_sort_ring_by_hw_res(struct phl_info_t *phl_info)
{
	_os_list *t_fctrl_result = &phl_info->t_fctrl_result;
	struct phl_ring_status *ring_sts, *t;
	u16 hw_res = 0, host_idx = 0, hw_idx = 0;
	u32 avail = 0, no_res = 0;
	_os_list *no_res_first = NULL;

	phl_list_for_loop_safe(ring_sts, t, struct phl_ring_status,
					t_fctrl_result, list) {

		if (ring_sts->ring_ptr->dma_ch > 32)
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
			"[WARNING] dma channel number larger than record map\n");

		if (no_res & (BIT0 << ring_sts->ring_ptr->dma_ch)) {
			if (&ring_sts->list == no_res_first)
				break;
			list_del(&ring_sts->list);
			list_add_tail(&ring_sts->list, t_fctrl_result);
			continue;
		} else if (avail & (BIT0 << ring_sts->ring_ptr->dma_ch)) {
			continue;
		}

		hw_res = rtw_hal_tx_res_query(phl_info->hal,
						ring_sts->ring_ptr->dma_ch,
						&host_idx, &hw_idx);
		if (0 == hw_res) {
			if (no_res_first == NULL)
				no_res_first = &ring_sts->list;
			list_del(&ring_sts->list);
			list_add_tail(&ring_sts->list, t_fctrl_result);
			no_res = no_res | (BIT0 << ring_sts->ring_ptr->dma_ch);
		} else {
			avail = avail | (BIT0 << ring_sts->ring_ptr->dma_ch);
		}
	}
}

void _phl_tx_flow_ctrl_pcie(struct phl_info_t *phl_info, _os_list *sta_list)
{
	/* _phl_sort_ring_by_hw_res(phl_info); */
	phl_tx_flow_ctrl(phl_info, sta_list);
}

#ifdef CONFIG_VW_REFINE
enum rtw_phl_status
_drop_disconnect_pkt(struct phl_info_t *phl_info, struct rtw_xmit_req *tx_req)
{
	struct rtw_phl_stainfo_t *phl_sta =
		rtw_phl_get_stainfo_by_macid(phl_info, tx_req->mdata.macid);

	if (phl_sta == NULL) {
		struct rtw_phl_evt_ops *ops = &phl_info->phl_com->evt_ops;
		if (RTW_PHL_TREQ_TYPE_NORMAL == tx_req->treq_type
#if defined(CONFIG_CORE_TXSC) || defined(CONFIG_PHL_TXSC)
			|| RTW_PHL_TREQ_TYPE_CORE_TXSC == tx_req->treq_type
			|| RTW_PHL_TREQ_TYPE_PHL_ADD_TXSC == tx_req->treq_type
#endif
			) {
			if (NULL != ops->tx_recycle) {
				ops->tx_recycle(phl_to_drvpriv(phl_info), tx_req);
#ifdef DEBUG_PHL_TX
				phl_info->phl_com->tx_stats.phl_txreq_sta_leave_drop++;
#endif
			}
			return RTW_PHL_STATUS_SUCCESS;
		}
	}
	return RTW_PHL_STATUS_FAILURE;
}
#endif

static enum rtw_phl_status _phl_handle_xmit_ring_pcie
						(struct phl_info_t *phl_info,
						struct phl_ring_status *ring_sts)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct phl_hci_trx_ops *hci_trx_ops = phl_info->hci_trx_ops;
	struct rtw_phl_tx_ring *tring = ring_sts->ring_ptr;
	struct rtw_xmit_req *tx_req = NULL;
	u16 rptr = 0, next_idx = 0;
	void *drv_priv = phl_to_drvpriv(phl_info);

#ifdef CONFIG_VW_REFINE
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
#endif

	while (0 != ring_sts->req_busy) {
		rptr = (u16)_os_atomic_read(drv_priv, &tring->phl_next_idx);

		tx_req = (struct rtw_xmit_req *)tring->entry[rptr];
		if (NULL == tx_req || NULL == tx_req->os_priv)  {
			PHL_ERR("tx_req is NULL!\n");
			break;
		}

		tx_req->mdata.band = ring_sts->band;
		tx_req->mdata.wmm = ring_sts->wmm;
		tx_req->mdata.hal_port = ring_sts->port;
		/*tx_req->mdata.mbssid = ring_sts->mbssid;*/
		tx_req->mdata.dma_ch = tring->dma_ch;

#ifdef CONFIG_VW_REFINE
		pstatus = _drop_disconnect_pkt(phl_info, tx_req);
		if (RTW_PHL_STATUS_FAILURE == pstatus)
#endif
			pstatus = hci_trx_ops->prepare_tx(phl_info, tx_req);

		if (RTW_PHL_STATUS_SUCCESS == pstatus ||
		    RTW_PHL_STATUS_FRAME_DROP == pstatus) {
			ring_sts->req_busy--;

#ifdef CONFIG_VW_REFINE
			hal_com->trx_stat.vw_cnt_snd += tx_req->vw_cnt;
#endif

			/* hana_todo, workaround here to update phl_index */
			_os_atomic_set(drv_priv, &tring->phl_idx, rptr);

			if (0 != ring_sts->req_busy) {
				next_idx = rptr + 1;

				if (next_idx >= MAX_PHL_TX_RING_ENTRY_NUM) {
					_os_atomic_set(drv_priv,
						       &tring->phl_next_idx, 0);

				} else {
					_os_atomic_inc(drv_priv,
						       &tring->phl_next_idx);
				}
			}
		} else {
			PHL_INFO("HCI prepare tx fail\n");
#ifdef CONFIG_VW_REFINE
			hal_com->trx_stat.pretx_fail =
				(hal_com->trx_stat.pretx_fail + 1) % WP_MAX_SEQ_NUMBER;
#endif
			break;
		}
	}

	return pstatus;
}

static void _phl_tx_callback_pcie(void *context)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_phl_handler *phl_handler
		= (struct rtw_phl_handler *)phl_container_of(context,
							struct rtw_phl_handler,
							os_handler);
	struct phl_info_t *phl_info = (struct phl_info_t *)phl_handler->context;
	struct phl_hci_trx_ops *hci_trx_ops = phl_info->hci_trx_ops;
	struct phl_ring_status *ring_sts = NULL, *t;
	void *drvpriv = phl_to_drvpriv(phl_info);
	_os_list sta_list;
	bool tx_pause = false;

#ifdef CONFIG_VW_REFINE
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);

	hal_com->trx_stat.phltx_cnt =
		(hal_com->trx_stat.phltx_cnt + 1) % WP_MAX_SEQ_NUMBER;
#endif

	FUNCIN_WSTS(pstatus);
	INIT_LIST_HEAD(&sta_list);

	/* check datapath sw state */
	tx_pause = phl_datapath_chk_trx_pause(phl_info, PHL_CTRL_TX);
	if (true == tx_pause)
		goto end;

#ifdef CONFIG_POWER_SAVE
	/* check ps state when tx is not paused */
	if (false == phl_ps_is_datapath_allowed(phl_info)) {
		PHL_WARN("%s(): datapath is not allowed now... may in low power.\n", __func__);
		goto chk_stop;
	}
#endif

	if (true == phl_check_xmit_ring_resource(phl_info, &sta_list)) {
		_phl_tx_flow_ctrl_pcie(phl_info, &sta_list);

		phl_list_for_loop_safe(ring_sts, t, struct phl_ring_status,
		                       &phl_info->t_fctrl_result, list) {
			list_del(&ring_sts->list);
			_phl_handle_xmit_ring_pcie(phl_info, ring_sts);
			phl_release_ring_sts(phl_info, ring_sts);
		}
	}

	pstatus = hci_trx_ops->tx(phl_info);
	if (RTW_PHL_STATUS_FAILURE == pstatus) {
		PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_, "[WARNING] phl_tx fail!\n");
	}

#ifdef CONFIG_POWER_SAVE
chk_stop:
#endif
	if (PHL_TX_STATUS_STOP_INPROGRESS ==
	    _os_atomic_read(drvpriv, &phl_info->phl_sw_tx_sts)) {
		PHL_WARN("PHL_TX_STATUS_STOP_INPROGRESS, going to stop sw tx.\n");
		phl_tx_stop_pcie(phl_info);
	}

end:
	phl_free_deferred_tx_ring(phl_info);

	FUNCOUT_WSTS(pstatus);
}


static u8 _phl_check_rx_hw_resource(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = phl_info->hci;
	u16 hw_res = 0, host_idx = 0, hw_idx = 0;
	u8 i = 0;
	u8 avail = 0;

	for (i = 0; i < hci_info->total_rxch_num; i++) {
		hw_res = rtw_hal_rx_res_query(phl_info->hal,
							i,
							&host_idx, &hw_idx);

		if (0 != hw_res) {
			avail = true;
			break;
		} else {
			avail = false;
		}
	}

	return avail;
}

static void _phl_rx_callback_pcie(void *context)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_phl_handler *phl_handler
		= (struct rtw_phl_handler *)phl_container_of(context,
							struct rtw_phl_handler,
							os_handler);
	struct phl_info_t *phl_info = (struct phl_info_t *)phl_handler->context;
	struct phl_hci_trx_ops *hci_trx_ops = phl_info->hci_trx_ops;
	void *drvpriv = phl_to_drvpriv(phl_info);
	bool rx_pause = false;
#ifdef CONFIG_SYNC_INTERRUPT
	struct rtw_phl_evt_ops *ops = &phl_info->phl_com->evt_ops;
#endif /* CONFIG_SYNC_INTERRUPT */

	FUNCIN_WSTS(pstatus);

	/* check datapath sw state */
	rx_pause = phl_datapath_chk_trx_pause(phl_info, PHL_CTRL_RX);
	if (true == rx_pause)
		goto end;

	do {
		if (false == phl_check_recv_ring_resource(phl_info))
			break;

		pstatus = hci_trx_ops->rx(phl_info);

		if (RTW_PHL_STATUS_FAILURE == pstatus) {
			PHL_TRACE_LMT(COMP_PHL_DBG, _PHL_WARNING_, "[WARNING] phl_rx fail!\n");
		}
	} while (false);

	if (PHL_RX_STATUS_STOP_INPROGRESS ==
	    _os_atomic_read(drvpriv, &phl_info->phl_sw_rx_sts)) {
		phl_rx_stop_pcie(phl_info);
	}

end:
	/* restore int mask of rx */
	rtw_hal_restore_rx_interrupt(phl_info->hal);
#ifdef CONFIG_SYNC_INTERRUPT
	ops->interrupt_restore(phl_to_drvpriv(phl_info), true);
#endif /* CONFIG_SYNC_INTERRUPT */

	FUNCOUT_WSTS(pstatus);

}
void _phl_fill_tx_meta_data(struct phl_info_t *phl, struct rtw_xmit_req *tx_req)
{
	struct rtw_phl_stainfo_t *sta =
			rtw_phl_get_stainfo_by_macid(phl, tx_req->mdata.macid);

#ifdef CONFIG_RTW_TX_HW_HDR_CONV
	if (!tx_req->hw_hdr_conv)
#endif
	tx_req->mdata.wp_offset = 56;
	tx_req->mdata.wd_page_size = 1;
	tx_req->mdata.addr_info_num = tx_req->pkt_cnt;
	tx_req->mdata.pktlen = (u16)tx_req->total_len;
#ifdef CONFIG_POWER_SAVE
	if (sta && phl_sta_latency_sensitive_chk(phl, sta, LAT_SEN_CHK_LPS_TX)) {
		if (tx_req->lat_sen)
			phl_ps_sta_ext_trx_nty(phl, sta->macid);
		tx_req->mdata.sw_tx_ok = true;
	}
#endif /* CONFIG_POWER_SAVE */
}



void phl_trx_resume_pcie(struct phl_info_t *phl_info, u8 type)
{
	if (PHL_CTRL_TX & type)
		phl_tx_resume_pcie(phl_info);

	if (PHL_CTRL_RX & type)
		phl_rx_resume_pcie(phl_info);
}

void phl_trx_reset_pcie(struct phl_info_t *phl_info, u8 type)
{
	struct rtw_phl_com_t *phl_com = phl_info->phl_com;
	struct rtw_stats *phl_stats = &phl_com->phl_stats;

	PHL_INFO("%s\n", __func__);

	if (PHL_CTRL_TX & type) {
		phl_tx_reset_pcie(phl_info);
		phl_reset_tx_stats(phl_stats);
	}

	if (PHL_CTRL_RX & type) {
		phl_rx_reset_pcie(phl_info);
		phl_reset_rx_stats(phl_stats);
	}
}

void phl_trx_stop_pcie(struct phl_info_t *phl_info)
{

}

void phl_trx_deinit_pcie(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = phl_info->hci;
	FUNCIN();
	_phl_free_rxbuf_pool_pcie(phl_info, hci_info->rxbuf_pool,
					hci_info->total_rxch_num);
	hci_info->rxbuf_pool = NULL;

	_phl_free_rxbd_pcie(phl_info, hci_info->rxbd_buf,
					hci_info->total_rxch_num);
	hci_info->rxbd_buf = NULL;

	_phl_free_wd_ring_pcie(phl_info, hci_info->wd_ring,
					hci_info->total_txch_num);
	hci_info->wd_ring = NULL;

	_phl_free_txbd_pcie(phl_info, hci_info->txbd_buf,
					hci_info->total_txch_num);
	hci_info->txbd_buf = NULL;

	_phl_destory_dma_pool_pcie(phl_info, hci_info->wd_dma_pool);

	hci_info->wd_dma_pool = NULL;

	_phl_destory_dma_pool_pcie(phl_info, hci_info->h2c_dma_pool);

	hci_info->h2c_dma_pool = NULL;

	FUNCOUT();
}

enum rtw_phl_status phl_trx_init_pcie(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct hci_info_t *hci_info = phl_info->hci;
	struct rtw_phl_handler *tx_handler = &phl_info->phl_tx_handler;
	struct rtw_phl_handler *rx_handler = &phl_info->phl_rx_handler;
	struct rtw_phl_handler *ser_handler = &phl_info->phl_ser_handler;
#ifdef CONFIG_RTW_TX_HDL_USE_WQ
	_os_workitem *workitem_tx = &tx_handler->os_handler.u.workitem;
#endif
#ifdef CONFIG_RTW_RX_HDL_USE_WQ
	_os_workitem *workitem_rx = &rx_handler->os_handler.u.workitem;
#endif
	void *drv_priv = phl_to_drvpriv(phl_info);

	u8 txch_num = 0, rxch_num = 0;
	u16 i = 0;

	FUNCIN_WSTS(pstatus);

	hci_info->wd_dma_pool = NULL;
	hci_info->h2c_dma_pool = NULL;
#if defined (CONFIG_WD_PAGE_NONCACHE_ADDR) || defined (CONFIG_WD_WORK_RING_NONCACHE_ADDR)
/* init DMA pool */
	hci_info->wd_dma_pool = _os_dma_pool_create(drv_priv, "WD_DMA_POOL", WD_PAGE_SIZE);

	if (hci_info->wd_dma_pool == NULL)
		return pstatus;
#endif
#ifdef CONFIG_H2C_NONCACHE_ADDR
/* init DMA pool */
	hci_info->h2c_dma_pool = _os_dma_pool_create(drv_priv, "H2C_DMA_POOL",
	                                             FWCMD_HDR_LEN + _WD_BODY_LEN + H2C_DATA_LEN);

	if (hci_info->h2c_dma_pool == NULL)
		return pstatus;
#endif

	do {
#ifdef CONFIG_RTW_TX_HDL_USE_WQ
		_os_workitem_config_cpu(drv_priv, workitem_tx, "TX_HDL", CPU_ID_TX_HDL);
#endif
#ifdef CONFIG_RTW_RX_HDL_USE_WQ
		_os_workitem_config_cpu(drv_priv, workitem_rx, "RX_HDL", CPU_ID_RX_HDL);
#endif

#if defined(CONFIG_RTW_TX_HDL_USE_WQ) || defined (CONFIG_PHL_HANDLER_WQ_HIGHPRI)
		tx_handler->type = RTW_PHL_HANDLER_PRIO_LOW;
#else
#ifdef CONFIG_RTW_TX_HDL_USE_THREAD
		tx_handler->type = RTW_PHL_HANDLER_PRIO_NORMAL; /* thread */
#else
		tx_handler->type = RTW_PHL_HANDLER_PRIO_HIGH; /* tasklet */
#endif
#endif
		tx_handler->callback = _phl_tx_callback_pcie;
		tx_handler->context = phl_info;
		tx_handler->drv_priv = drv_priv;

		#ifdef CONFIG_RTW_OS_HANDLER_EXT
		tx_handler->id = RTW_PHL_TX_HANDLER;
		#endif /* CONFIG_RTW_OS_HANDLER_EXT */

		pstatus = phl_register_handler(phl_info->phl_com, tx_handler);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;

		ser_handler->type = RTW_PHL_HANDLER_PRIO_HIGH; /* tasklet */
		ser_handler->callback = phl_ser_send_check;
		ser_handler->context = phl_info;
		ser_handler->drv_priv = drv_priv;
		pstatus = phl_register_handler(phl_info->phl_com, ser_handler);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;

#ifdef CONFIG_RTW_RX_HDL_USE_THREAD
		rx_handler->type = RTW_PHL_HANDLER_PRIO_NORMAL; /* thread */
#else
#if defined(CONFIG_TX_REQ_NONCACHE_ADDR) || defined(CONFIG_RTW_RX_HDL_USE_WQ) || defined(CONFIG_PHL_HANDLER_WQ_HIGHPRI)
		/* prevent dma_free_coherent() being called in atomic context */
		rx_handler->type = RTW_PHL_HANDLER_PRIO_LOW;
#else
		rx_handler->type = RTW_PHL_HANDLER_PRIO_HIGH;
#endif
#endif
		rx_handler->callback = _phl_rx_callback_pcie;
		rx_handler->context = phl_info;
		rx_handler->drv_priv = drv_priv;

		#ifdef CONFIG_RTW_OS_HANDLER_EXT
		rx_handler->id = RTW_PHL_RX_HANDLER;
		#endif /* CONFIG_RTW_OS_HANDLER_EXT */

		pstatus = phl_register_handler(phl_info->phl_com, rx_handler);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;
		/* pcie tx sw resource */
		txch_num = rtw_hal_query_txch_num(phl_info->hal);
		hci_info->total_txch_num = txch_num;
		/* allocate tx bd */
		pstatus = _phl_alloc_txbd_pcie(phl_info, txch_num);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;
		/* allocate wd page */
		pstatus = _phl_alloc_wd_ring_pcie(phl_info, txch_num);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;

		for (i = 0; i < PHL_MACID_MAX_NUM; i++)
			hci_info->wp_seq[i] = WP_RESERVED_SEQ;

#ifdef CONFIG_VW_REFINE
		for (i = 0; i <= WP_MAX_SEQ_NUMBER; i++)
			phl_info->free_wp[i] = i;
		phl_info->fr_ptr = 0;
		phl_info->fw_ptr = 0;
#endif

		/* pcie rx sw resource */
		rxch_num = rtw_hal_query_rxch_num(phl_info->hal);
		hci_info->total_rxch_num = rxch_num;
		/* allocate rx bd */
		pstatus = _phl_alloc_rxbd_pcie(phl_info, rxch_num);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;
		/* allocate wd page */
		pstatus = _phl_alloc_rxbuf_pool_pcie(phl_info, rxch_num);
		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;

	} while (false);

	if (RTW_PHL_STATUS_SUCCESS != pstatus)
		phl_trx_deinit_pcie(phl_info);
	else
		pstatus = _phl_update_default_rx_bd(phl_info);

	FUNCOUT_WSTS(pstatus);
	return pstatus;
}


enum rtw_phl_status phl_trx_config_pcie(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct hci_info_t *hci_info = phl_info->hci;

	do {
		hstatus = rtw_hal_trx_init(phl_info->hal, hci_info->txbd_buf,
						hci_info->rxbd_buf);
		if (RTW_HAL_STATUS_SUCCESS != hstatus) {
			PHL_ERR("rtw_hal_trx_init fail with status 0x%08X\n",
				hstatus);
			pstatus = RTW_PHL_STATUS_FAILURE;
			break;
		}
		else {
			pstatus = RTW_PHL_STATUS_SUCCESS;
		}
	} while (false);

	return pstatus;
}

#ifdef CONFIG_PHL_TXSC
u8 *_phl_txsc_apply_shortcut(struct phl_info_t *phl_info, struct rtw_xmit_req *tx_req,
							struct rtw_phl_stainfo_t *phl_sta, struct rtw_phl_pkt_req *phl_pkt_req)
{
	struct phl_txsc_entry *ptxsc = NULL;

	if (phl_sta == NULL)
		return (u8 *)ptxsc;

	if (tx_req->shortcut_id >= PHL_TXSC_ENTRY_NUM) {
		PHL_ERR("[PHL][TXSC] wrong shortcut_id:%d, plz check !!!\n", tx_req->shortcut_id);
		return (u8 *)ptxsc;
	}

	ptxsc = &phl_sta->phl_txsc[tx_req->shortcut_id];

	if ((tx_req->treq_type == RTW_PHL_TREQ_TYPE_CORE_TXSC)) {

		if (ptxsc == NULL) {
			PHL_ERR("[txsc][phl] fetal err: ptxsc = NULL, plz check.\n");
			return (u8 *)ptxsc;
		}

		if (!ptxsc->txsc_wd_cached || ptxsc->txsc_wd_seq_offset == 0) {
			PHL_ERR("[txsc][phl] phl_txsc re-update, txsc_wd_cached:%d, txsc_wd_seq_offset:%d\n",
				ptxsc->txsc_wd_cached, ptxsc->txsc_wd_seq_offset);
			ptxsc->txsc_wd_cached = ptxsc->txsc_wd_seq_offset = 0;
			tx_req->treq_type |= RTW_PHL_TREQ_TYPE_PHL_UPDATE_TXSC;
			return (u8 *)ptxsc;
		}

		_os_mem_cpy(phl_info, phl_pkt_req->wd_page, ptxsc->txsc_wd_cache, ptxsc->txsc_wd_seq_offset);
		phl_pkt_req->wd_len = ptxsc->txsc_wd_len;
		phl_pkt_req->wd_seq_offset = ptxsc->txsc_wd_seq_offset;
#ifdef DEBUG_PHL_TX
		phl_info->phl_com->tx_stats.phl_txsc_copy_cnt++;
#endif
		/* update pktlen in wd_page, wd_body[8:15] = pktsize */
		#if 0
		packet_len = cpu_to_le16(tx_req->mdata.pktlen);
		_os_mem_cpy(phl_info, phl_pkt_req.wd_page+8, &packet_len, sizeof(u16));
		#endif

		ptxsc->txsc_cache_hit++;
	}

	return (u8 *)ptxsc;
}

enum rtw_phl_status
_phl_txsc_add_shortcut(struct phl_info_t *phl_info, struct rtw_xmit_req *tx_req,
								struct rtw_phl_pkt_req *phl_pkt_req, struct phl_txsc_entry *ptxsc)
{

	if (tx_req->shortcut_id >= PHL_TXSC_ENTRY_NUM) {
		PHL_ERR("[PHL][TXSC] wrong shortcut_id:%d, plz check.\n", tx_req->shortcut_id);
		return RTW_PHL_STATUS_FAILURE;
	}

	if (ptxsc == NULL) {
		PHL_ERR("[txsc][phl] fetal err: ptxsc = NULL, shortcut_id = %d, plz check.\n", tx_req->shortcut_id);
		return RTW_PHL_STATUS_FAILURE;
	}

	if (tx_req->treq_type & RTW_PHL_TREQ_TYPE_PHL_UPDATE_TXSC) {

		_os_mem_set(phl_info, ptxsc, 0x0, sizeof(struct phl_txsc_entry));
		_os_mem_cpy(phl_info, ptxsc->txsc_wd_cache, phl_pkt_req->wd_page, phl_pkt_req->wd_seq_offset);

		ptxsc->txsc_wd_len = phl_pkt_req->wd_len;
		ptxsc->txsc_wd_seq_offset = phl_pkt_req->wd_seq_offset;
		ptxsc->txsc_wd_cached = 1;

		#if 0
		PHL_PRINT("\n[txsc][phl] shortcut_id:%d, wd_page cached, len:%d. SMH: %u (%u)\n\n",
			tx_req->shortcut_id, ptxsc->txsc_wd_len, tx_req->mdata.smh_en,
			tx_req->treq_type);
		#endif

		tx_req->treq_type &= ~RTW_PHL_TREQ_TYPE_PHL_UPDATE_TXSC;
		if (tx_req->treq_type != RTW_PHL_TREQ_TYPE_PHL_ADD_TXSC)
			PHL_PRINT("Updated WD for request type %u\n", tx_req->treq_type);
	}

	return RTW_PHL_STATUS_SUCCESS;
}
#endif

static void
_phl_cpy_pkt_list(struct rtw_pkt_buf_list *dst, struct tx_local_buf *src,
                  u16 ofst, u16 length)
{
	dst->vir_addr = src->vir_addr + ofst;
	dst->phy_addr_l = src->phy_addr_l + ofst;
	dst->phy_addr_h = src->phy_addr_h;
	if (dst->phy_addr_l < src->phy_addr_l)	/* addr_l overflow */
		dst->phy_addr_h++;
	dst->length = length;
}

static enum rtw_phl_status
_phl_coalesce_tx_pcie(struct phl_info_t *phl_info, struct rtw_xmit_req *treq)
{
	enum rtw_phl_status sts = RTW_PHL_STATUS_FAILURE;
	struct hal_spec_t *hal_spec = phl_get_ic_spec(phl_info->phl_com);
	struct tx_local_buf *local_buf = NULL;
	struct rtw_pkt_buf_list	*pkt_list = NULL;
	u16 ofst = 0, len = 0, len_lmt = 0, len_copy = 0;
	u8 i = 0, addr_idx = 0;

	pkt_list = (struct rtw_pkt_buf_list *)treq->pkt_list;

	if (treq->pkt_cnt <= hal_spec->phyaddr_num) {
		bool re_arch_wp = false;

		for (i = 0; i < treq->pkt_cnt; i++) {
			if (pkt_list[i].length > hal_spec->addr_info_len_lmt) {
				re_arch_wp = true;
				break;
			}
		}

		if (re_arch_wp == false)
			return RTW_PHL_STATUS_SUCCESS;
	}

	sts = _phl_alloc_local_buf_pcie(phl_info, treq);
	if (sts != RTW_PHL_STATUS_SUCCESS) {
		PHL_WARN("allocate phl local buf fail!\n");
		return RTW_PHL_STATUS_RESOURCE;
	}

	if (treq->total_len > treq->local_buf->buf_max_size) {
		PHL_WARN("%s: coalesce size overflow!! packet total len(%d) > coalesce buf size(%d)!\n",
		         __FUNCTION__, treq->total_len,
		         treq->local_buf->buf_max_size);
		sts = RTW_PHL_STATUS_RESOURCE;
		goto fail;
	}

	local_buf = treq->local_buf;
	local_buf->used_size = treq->total_len;

	for (i = 0; i < treq->pkt_cnt; i++) {
		_os_mem_cpy(phl_info, local_buf->vir_addr + len,
		            pkt_list[i].vir_addr,
		            (u32)pkt_list[i].length);
		len += pkt_list[i].length;
	}

	len_lmt = hal_spec->addr_info_len_lmt & (~0x03);

	for (addr_idx = 0; addr_idx < hal_spec->phyaddr_num; addr_idx++) {
		len_copy = (len > hal_spec->addr_info_len_lmt) ? len_lmt : len;
		_phl_cpy_pkt_list(&pkt_list[addr_idx], local_buf, ofst,
		                  len_copy);
		ofst += len_copy;
		len -= len_copy;

		if (len == 0)
			break;
	}

	if (len != 0) {
		PHL_WARN("%s: incorrect coalesce size(%d) != total len(%d), left length (%d)!\n",
		         __FUNCTION__, len_copy, treq->total_len, len);
		sts = RTW_PHL_STATUS_FAILURE;
		goto fail;
	}

	treq->pkt_cnt = addr_idx + 1;
	treq->mdata.addr_info_num = treq->pkt_cnt;

	if(local_buf->cache == CACHE_ADDR) {
		PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "[%s] local_buf cache wback \n",
		          __FUNCTION__);
		_os_cache_wback(phl_to_drvpriv(phl_info),
		                &local_buf->phy_addr_l,
		                &local_buf->phy_addr_h,
		                treq->total_len, DMA_TO_DEVICE);
	}

	return RTW_PHL_STATUS_SUCCESS;
fail:
	_phl_free_local_buf_pcie(phl_info, treq);
	return sts;
}


enum rtw_phl_status
phl_check_wp_tag_resource(struct phl_info_t *phl_info,
	struct rtw_wd_page_ring *wd_ring, u16 *wp_seq, u8 dma_ch)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;
	struct rtw_xmit_req *ptr = NULL;
	struct rtw_wd_page_ring *ch_wd_ring = &wd_ring[dma_ch];
	struct rtw_wp_tag *wp_tag = ch_wd_ring->wp_tag;
	u16 seq = *wp_seq;
	volatile u16 *head_seq = &wd_ring[dma_ch].wp_seq;

	/* error handle when wp_tag out of resource */
	while (NULL != wp_tag[seq].ptr) {
		seq = (seq + 1) % WP_MAX_SEQ_NUMBER;
		if (0 == seq)
			seq = 1;
		if (seq == *head_seq) {
			ptr = (struct rtw_xmit_req *)wp_tag[seq].ptr;
			/* WP tag overflow. The WP of this tag, however,
			 * could be recycled. Just use it if ptr is NULL. */
			if (ptr == NULL)
				break;
#ifdef DEBUG_PHL_TX
			phl_info->phl_com->tx_stats.wp_tg_out_of_resource++;
#endif
			PHL_ERR("CH%u out of tag #%u MACID:%u!\n",
			        dma_ch, seq, ptr->mdata.macid);
			_phl_dump_busy_wp(phl_info);
			pstatus = RTW_PHL_STATUS_FAILURE;
			break;
		}
	}
	*wp_seq = seq;

	return pstatus;
}

#ifdef CONFIG_VW_REFINE
void  _phl_tx_map_ctrl(struct phl_info_t *phl, struct rtw_xmit_req *tx_req)
{
	if ((tx_req->treq_type == RTW_PHL_TREQ_TYPE_CORE_TXSC ||
		tx_req->treq_type == RTW_PHL_TREQ_TYPE_PHL_UPDATE_TXSC) &&
		(!tx_req->is_map))
	{
		struct rtw_phl_evt_ops *ops = &phl->phl_com->evt_ops;

		if (NULL != ops->tx_dev_map)
			ops->tx_dev_map(phl_to_drvpriv(phl), tx_req);
	}
}

u16 _phl_query_wp_seq(struct phl_info_t *phl_info)
{
	u16 val;

	val = (phl_info->fr_ptr + 1) % WP_MAX_SEQ_NUMBER;
	if (val == phl_info->fw_ptr)
		return WP_USED_SEQ;

	return phl_info->free_wp[phl_info->fr_ptr];
}
#endif

static enum rtw_phl_status
phl_handle_busy_wd(struct phl_info_t *phl_info,
                   struct rtw_wd_page_ring *wd_ring, u16 hw_idx);

static inline enum rtw_phl_status
phl_recycle_busy_wd_by_ch(struct phl_info_t *phl_info, u8 ch, u16 *hw_res)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	u16 host_idx, hw_idx;

	FUNCIN_WSTS(pstatus);
	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;

	*hw_res = rtw_hal_tx_res_query(phl_info->hal, ch, &host_idx, &hw_idx);
	pstatus = phl_handle_busy_wd(phl_info, &wd_ring[ch], hw_idx);;

	FUNCOUT_WSTS(pstatus);
	return pstatus;
}

enum rtw_phl_status
phl_prepare_tx_pcie(struct phl_info_t *phl_info, struct rtw_xmit_req *tx_req)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
 	struct rtw_wp_rpt_stats *rpt_stats =
		(struct rtw_wp_rpt_stats *)hal_com->trx_stat.wp_rpt_stats;
	struct hci_info_t *hci_info = NULL;
	struct rtw_pkt_buf_list *pkt_buf = NULL;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct rtw_wd_page *wd_page = NULL;
	struct rtw_phl_pkt_req phl_pkt_req;
	u16 packet_len = 0, wp_seq = 0;
	u8 dma_ch = 0, i = 0;
	u16 mid = 0;
#ifdef CONFIG_PHL_TXSC
	struct phl_txsc_entry *ptxsc = NULL;
	struct rtw_phl_stainfo_t *phl_sta = rtw_phl_get_stainfo_by_macid(phl_info, tx_req->mdata.macid);
#endif
	FUNCIN_WSTS(pstatus);
	do {
		if (NULL == phl_info->hci) {
			PHL_ERR("phl_info->hci is NULL!\n");
			pstatus = RTW_PHL_STATUS_FAILURE;
			break;
		}
		hci_info = (struct hci_info_t *)phl_info->hci;
		wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;

		if (NULL == tx_req) {
			PHL_ERR("tx_req is NULL!\n");
			pstatus = RTW_PHL_STATUS_FRAME_DROP;
			break;
		}
		mid = tx_req->mdata.macid;
		dma_ch = tx_req->mdata.dma_ch;

#ifdef CONFIG_VW_REFINE
		if (0 == dma_ch) {
			if ( WP_USED_SEQ == _phl_query_wp_seq(phl_info)) {
				PHL_ERR("wp_tag %d: wrong rw ptr wp_seq:%d !!! \n",
					phl_info->fr_ptr, wp_seq);
				break;
			}
		} else
#endif
			wp_seq = wd_ring[dma_ch].wp_seq;

		if (phl_check_wp_tag_resource(phl_info, wd_ring, &wp_seq, dma_ch) ==
				RTW_PHL_STATUS_FAILURE)
			break;

		wd_page = query_idle_wd_page(phl_info, &wd_ring[dma_ch]);
		/* No idle WD, recycle busy ones as DMA could've moved some */
		if (NULL == wd_page) {
			u16 hw_res;
			/* No busy WD either. All in pending Q. Just go on. */
			if (wd_ring[dma_ch].busy_wd_page_cnt == 0) {
				pstatus = RTW_PHL_STATUS_FAILURE;
				break;
			}

			phl_recycle_busy_wd_by_ch(phl_info, dma_ch, &hw_res);
			wd_page = query_idle_wd_page(phl_info, &wd_ring[dma_ch]);
			if (NULL == wd_page) {
				/* No busy WD can be recycled. */
				PHL_ERR("CH%u out of idle WD page "
					 "(I%u/B%u/P%u)\n", dma_ch,
					 wd_ring[dma_ch].idle_wd_page_cnt,
					 wd_ring[dma_ch].busy_wd_page_cnt,
					 wd_ring[dma_ch].pending_wd_page_cnt);
				pstatus = RTW_PHL_STATUS_FAILURE;
				if (wd_ring[dma_ch].busy_wd_page_cnt > MAX_WD_PAGE_NUM * 4 / 5) {
#ifdef CONFIG_POWER_SAVE
					if (phl_ps_get_cur_pwr_lvl(phl_info) == PS_PWR_LVL_PWRON)
#endif
						phl_tx_dbg_status_dump(phl_info, tx_req->mdata.band);
				}
				break;
			}
		}

		pkt_buf = (struct rtw_pkt_buf_list *)&tx_req->pkt_list[0];
		for (i = 0; i < tx_req->pkt_cnt; i++) {
			packet_len += pkt_buf->length;
			pkt_buf++;
		}

		tx_req->total_len = packet_len;

		_phl_fill_tx_meta_data(phl_info, tx_req);

		pstatus = _phl_coalesce_tx_pcie(phl_info, tx_req);
		if (RTW_PHL_STATUS_SUCCESS != pstatus) {
			struct rtw_phl_evt_ops *ops =
				&phl_info->phl_com->evt_ops;

			ops->tx_recycle(phl_to_drvpriv(phl_info), tx_req);
			rtw_release_target_wd_page(phl_info, &wd_ring[dma_ch],
			                           wd_page);
			pstatus = RTW_PHL_STATUS_FRAME_DROP;
			break;
		}

		phl_pkt_req.wd_page = wd_page->vir_addr;

		phl_pkt_req.wp_seq = wp_seq;
		phl_pkt_req.tx_req = tx_req;

#ifdef CONFIG_PHL_TXSC
		phl_pkt_req.wd_len = phl_pkt_req.wd_seq_offset = 0;
		ptxsc = (struct phl_txsc_entry *)_phl_txsc_apply_shortcut(phl_info, tx_req, phl_sta, &phl_pkt_req);
#endif

#ifdef CONFIG_VW_REFINE
		_phl_tx_map_ctrl(phl_info, tx_req);
#endif

		hstatus = rtw_hal_update_wd_page(phl_info->hal, &phl_pkt_req);
		wd_page->buf_len = phl_pkt_req.wd_len;

#ifdef DEBUG_PHL_TX
		if (phl_info->phl_com->tx_stats.flag_print_wdpage_once == 1) {
			if (RTW_HAL_STATUS_SUCCESS == hstatus)
				debug_dump_buf(phl_pkt_req.wd_page, (u16)phl_pkt_req.wd_len, "dump wd page");
			phl_info->phl_com->tx_stats.flag_print_wdpage_once = 0;
		}
#endif

		if (RTW_HAL_STATUS_SUCCESS == hstatus) {
			hci_info->wp_seq[mid] = phl_pkt_req.wp_seq;
			enqueue_pending_wd_page(phl_info, &wd_ring[dma_ch],
						wd_page, _tail);
			tx_req->tx_time = _os_get_cur_time_ms();
#ifdef CONFIG_PHL_TX_DBG
			if (tx_req->tx_dbg.en_dbg) {
				tx_req->tx_dbg.enq_pending_wd_t =
						_os_get_cur_time_us();
			}
#endif /* CONFIG_PHL_TX_DBG */
			_os_spinlock(phl_to_drvpriv(phl_info),
				     &wd_ring[dma_ch].wp_tag_lock,
				     _bh, NULL);
			PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
				  "update tx req(%p) in ch(%d) with wp_seq(0x%x) and wifi seq(%d), macid(%d), tid(%d)!\n",
				  tx_req, dma_ch, wp_seq, tx_req->mdata.sw_seq,
				  tx_req->mdata.macid, tx_req->mdata.tid);
			wd_ring[dma_ch].wp_tag[wp_seq].ptr = (u8 *)tx_req;
			rpt_stats[dma_ch].busy_cnt++;
			_inc_sta_rpt_stats_busy_cnt(phl_info, mid);
			_os_spinunlock(phl_to_drvpriv(phl_info),
				       &wd_ring[dma_ch].wp_tag_lock,
				       _bh, NULL);

			wp_seq = (wp_seq + 1) % WP_MAX_SEQ_NUMBER;
			if (0 == wp_seq)
				wp_seq = 1;

			wd_ring[dma_ch].wp_seq = wp_seq;

			pstatus = RTW_PHL_STATUS_SUCCESS;

			#ifdef CONFIG_RTW_MIRROR_DUMP
			phl_mirror_dump_wd(phl_info, dma_ch, wd_page->vir_addr, wd_page->buf_len);
			#endif

			//wb wd page
			if(wd_page->cache == CACHE_ADDR) {
				PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "[%s] wd page cache wback \n",
					__FUNCTION__);
				_os_cache_wback(phl_to_drvpriv(phl_info),
					&wd_page->phy_addr_l,
					&wd_page->phy_addr_h,
					wd_page->buf_len, DMA_TO_DEVICE);
			}

#ifdef CONFIG_PHL_TXSC
			_phl_txsc_add_shortcut(phl_info, tx_req, &phl_pkt_req, ptxsc);
#endif

#ifdef CONFIG_VW_REFINE
			if (0 == dma_ch) {
				phl_info->free_wp[phl_info->fr_ptr] = WP_USED_SEQ;
				phl_info->fr_ptr = (phl_info->fr_ptr + 1) % WP_MAX_SEQ_NUMBER;
			}
#endif

			break;
		} else {
			rtw_release_target_wd_page(phl_info, &wd_ring[dma_ch],
						wd_page);
			pstatus = RTW_PHL_STATUS_FAILURE;
			break;
		}
	} while(false);
	FUNCOUT_WSTS(pstatus);
	return pstatus;
}

static enum rtw_phl_status
phl_handle_pending_wd(struct phl_info_t *phl_info,
				struct rtw_wd_page_ring *wd_ring,
				u16 txcnt, u8 ch)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct tx_base_desc *txbd = NULL;
	struct rtw_wd_page *wd = NULL;
	u16 cnt = 0;

#ifdef RTW_WKARD_DYNAMIC_LTR
	if (true != _phl_judge_act_ltr_switching_conditions(phl_info, ch)) {
		_phl_act_ltr_update_stats(phl_info, false, ch,
		                          wd_ring->pending_wd_page_cnt);
		return RTW_PHL_STATUS_FAILURE;
	} else {
		_phl_act_ltr_update_stats(phl_info, true, ch,
		                          wd_ring->pending_wd_page_cnt);
	}
#endif

	txbd = (struct tx_base_desc *)hci_info->txbd_buf;
	while (txcnt > cnt) {
		wd = query_pending_wd_page(phl_info, wd_ring);

		if (NULL == wd) {
			pstatus = RTW_PHL_STATUS_RESOURCE;
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "query Tx pending WD fail!\n");
			break;
		}

		wd->ls = 1;//tmp set LS=1
		hstatus = rtw_hal_update_txbd(phl_info->hal, txbd, wd, ch, 1);
		if (RTW_HAL_STATUS_SUCCESS == hstatus) {
			enqueue_busy_wd_page(phl_info, wd_ring, wd, _tail);
			pstatus = RTW_PHL_STATUS_SUCCESS;
		} else {
			enqueue_pending_wd_page(phl_info, wd_ring, wd, _first);
			pstatus = RTW_PHL_STATUS_RESOURCE;
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "update Tx BD fail!\n");
			break;
		}

		cnt++;
	}

	if (RTW_PHL_STATUS_SUCCESS == pstatus) {
#ifdef RTW_WKARD_DYNAMIC_LTR
		_phl_switch_act_ltr(phl_info, ch);
#endif
		hstatus = rtw_hal_trigger_txstart(phl_info->hal, txbd, ch);
		if (RTW_HAL_STATUS_SUCCESS != hstatus) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_, "update txbd idx fail!\n");
			pstatus = RTW_PHL_STATUS_FAILURE;
		} else {
			#ifdef CONFIG_POWER_SAVE
			phl_ps_tx_pkt_ntfy(phl_info);
			#endif
			if (wd_ring->cur_hw_res > cnt)
				wd_ring->cur_hw_res -= cnt;
			else
				wd_ring->cur_hw_res = 0;

			#ifdef CONFIG_PHL_OFDMA_GROUP_STATISTIC
			phl_com->trigger_txstart_ok++;
			#endif
		}
	}

	return pstatus;
}


static enum rtw_phl_status
phl_handle_busy_wd(struct phl_info_t *phl_info,
                   struct rtw_wd_page_ring *wd_ring, u16 hw_idx)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	void *drv_priv = phl_to_drvpriv(phl_info);
	_os_list *list = &wd_ring->busy_wd_page_list;
	struct rtw_wd_page *wd = NULL;
	u16 bndy = hal_com->bus_cap.txbd_num;
	u16 target = 0;
	u16 release_num = 0;

	do {
		_os_spinlock(drv_priv, &wd_ring->busy_lock, _bh, NULL);

		if (list_empty(list)) {
			pstatus = RTW_PHL_STATUS_SUCCESS;
			_os_spinunlock(drv_priv, &wd_ring->busy_lock, _bh, NULL);
			break;
		}

		if (wd_ring->busy_wd_page_cnt > (bndy - 1)) {
			release_num = wd_ring->busy_wd_page_cnt - (bndy - 1);
			_os_spinunlock(drv_priv, &wd_ring->busy_lock, _bh, NULL);
			pstatus = rtw_release_busy_wd_page(phl_info, wd_ring,
								release_num);

			if (RTW_PHL_STATUS_SUCCESS != pstatus)
				break;
			else
				_os_spinlock(drv_priv, &wd_ring->busy_lock, _bh, NULL);
		}

		wd = list_first_entry(list, struct rtw_wd_page, list);
		target = wd->host_idx;

		if (hw_idx >= target)
			release_num = ((hw_idx - target) + 1) % bndy;
		else
			release_num = ((bndy - target) + (hw_idx + 1)) % bndy;

		_os_spinunlock(drv_priv, &wd_ring->busy_lock, _bh, NULL);

		pstatus = rtw_release_busy_wd_page(phl_info, wd_ring,
							release_num);

		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;
	} while (false);

	return pstatus;
}

enum rtw_phl_status phl_recycle_busy_wd(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	u8 ch = 0;
	u16 hw_res;
	FUNCIN_WSTS(pstatus);

	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
		pstatus = phl_recycle_busy_wd_by_ch(phl_info, ch, &hw_res);
	}

	FUNCOUT_WSTS(pstatus);
	return pstatus;
}

static enum rtw_phl_status
phl_handle_busy_h2c(struct phl_info_t *phl_info,
			struct phl_h2c_pkt_pool *h2c_pool, u16 hw_idx)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct phl_queue *queue = &h2c_pool->busy_h2c_pkt_list;
	_os_list *list = &h2c_pool->busy_h2c_pkt_list.queue;
	struct rtw_h2c_pkt *h2c = NULL;
	u16 bndy = hal_com->bus_cap.txbd_num;
	u16 target = 0;
	u16 release_num = 0;
	u16 tmp_cnt = 0;

	do {
		_os_spinlock(drv_priv, &queue->lock, _bh, NULL);

		if (list_empty(list)) {
			pstatus = RTW_PHL_STATUS_SUCCESS;

			_os_spinunlock(drv_priv, &queue->lock, _bh, NULL);
			break;
		}

#if !(defined(CONFIG_VW_REFINE) || defined(CONFIG_ONE_TXQ))
		/* h2c corrput content on pcie bus because recycling h2c is too fast */
		tmp_cnt = (u16)queue->cnt;
		if (tmp_cnt > (bndy - 1)) {
			release_num = tmp_cnt - (bndy - 1);
			_os_spinunlock(drv_priv, &queue->lock, _bh, NULL);
			pstatus = phl_release_busy_h2c_pkt(phl_info, h2c_pool,
							release_num);

			if (RTW_PHL_STATUS_SUCCESS != pstatus)
				break;
			else
				_os_spinlock(drv_priv, &queue->lock, _bh, NULL);
		}
#endif

		h2c = list_first_entry(list, struct rtw_h2c_pkt, list);
		target = h2c->host_idx;

		if (hw_idx >= target)
			release_num = ((hw_idx - target) + 1) % bndy;
		else
			release_num = ((bndy - target) + (hw_idx + 1)) % bndy;

		_os_spinunlock(drv_priv, &queue->lock, _bh, NULL);

		PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : release_num %d.\n", __func__, release_num);

		pstatus = phl_release_busy_h2c_pkt(phl_info, h2c_pool,
							release_num);

		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			break;
	} while (false);

	return pstatus;
}

enum rtw_phl_status phl_recycle_busy_h2c(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct phl_h2c_pkt_pool *h2c_pool = NULL;
	u16 hw_res = 0, host_idx = 0, hw_idx = 0;
	u8 fwcmd_queue_idx = 0;

	FUNCIN_WSTS(pstatus);
	h2c_pool = (struct phl_h2c_pkt_pool *)phl_info->h2c_pool;
	_os_spinlock(phl_to_drvpriv(phl_info), &h2c_pool->recycle_lock, _bh, NULL);
	fwcmd_queue_idx = rtw_hal_get_fwcmd_queue_idx(phl_info->hal);

	hw_res = rtw_hal_tx_res_query(phl_info->hal, fwcmd_queue_idx, &host_idx,
						&hw_idx);
	PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : host idx %d, hw_idx %d.\n",
			  __func__, host_idx, hw_idx);
	pstatus = phl_handle_busy_h2c(phl_info, h2c_pool, hw_idx);
	_os_spinunlock(phl_to_drvpriv(phl_info), &h2c_pool->recycle_lock, _bh, NULL);
	FUNCOUT_WSTS(pstatus);
	return pstatus;
}

static enum rtw_phl_status phl_tx_pcie(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	u16 hw_res = 0, txcnt = 0;
	u8 ch = 0;
	FUNCIN_WSTS(pstatus);
	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;

	for (ch = 0; ch < hci_info->total_txch_num; ch++) {
#ifndef RTW_WKARD_WIN_TRX_BALANCE
		/* if wd_ring is empty, do not read hw_idx for saving cpu cycle */
		if (wd_ring[ch].pending_wd_page_cnt == 0 && wd_ring[ch].busy_wd_page_cnt == 0){
			pstatus = RTW_PHL_STATUS_SUCCESS;
			continue;
		}
#endif
		/* hana_todo skip fwcmd queue */
		if (wd_ring[ch].cur_hw_res < hal_com->bus_cap.read_txbd_th ||
		    wd_ring[ch].pending_wd_page_cnt > wd_ring[ch].cur_hw_res) {
			pstatus = phl_recycle_busy_wd_by_ch(phl_info, ch, &hw_res);
			wd_ring[ch].cur_hw_res = hw_res;

			if (RTW_PHL_STATUS_FAILURE == pstatus)
				continue;
		} else {
			hw_res = wd_ring[ch].cur_hw_res;
		}

		if (list_empty(&wd_ring[ch].pending_wd_page_list)) {
			pstatus = RTW_PHL_STATUS_SUCCESS;
			continue;
		}

		if (0 == hw_res) {
			PHL_TRACE(COMP_PHL_XMIT, _PHL_INFO_, "No hw resource, dma_ch %d txbd full!\n",
			          ch);
			continue;
		} else {
			txcnt = (hw_res < wd_ring[ch].pending_wd_page_cnt) ?
				hw_res : wd_ring[ch].pending_wd_page_cnt;

			pstatus = phl_handle_pending_wd(phl_info, &wd_ring[ch],
							txcnt, ch);

			if (RTW_PHL_STATUS_SUCCESS != pstatus)
				continue;
		}
	}
	FUNCOUT_WSTS(pstatus);
	return pstatus;
}


enum rtw_phl_status _phl_refill_rxbd(struct phl_info_t *phl_info,
					void* rx_buf_ring,
					struct rx_base_desc *rxbd,
					u8 ch, u16 refill_cnt)
{
	struct dvobj_priv *pobj = phl_to_drvpriv(phl_info);
	struct pci_dev *pdev = dvobj_to_pci(pobj)->ppcidev;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct rtw_rx_buf *rxbuf = NULL;
	u16 cnt = 0;

	for (cnt = 0; cnt < refill_cnt; cnt++) {
		rxbuf = query_idle_rx_buf(phl_info, rx_buf_ring);
		if (NULL == rxbuf) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"[WARNING] there is no resource for rx bd refill setting\n");
			pstatus = RTW_PHL_STATUS_RESOURCE;
			break;
		}
		hstatus = rtw_hal_update_rxbd(phl_info->hal, rxbd, rxbuf, ch);
		if (RTW_HAL_STATUS_SUCCESS != hstatus) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"[WARNING] update rxbd fail\n");
			pstatus = RTW_PHL_STATUS_FAILURE;
			break;
		}
		enqueue_busy_rx_buf(phl_info, rx_buf_ring, rxbuf, _tail);
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	if (rxbd->cache == CACHE_ADDR)
		pci_cache_wback(pdev, (dma_addr_t *)&rxbd->phy_addr_l, rxbd->buf_len, DMA_TO_DEVICE);

	/* hana_todo */
	/* wmb(); */

	if (cnt) {
		hstatus = rtw_hal_notify_rxdone(phl_info->hal, rxbd, ch);
		if (RTW_HAL_STATUS_SUCCESS != hstatus) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"[WARNING] notify rxdone fail\n");
			pstatus = RTW_PHL_STATUS_FAILURE;
		}
	}
	return pstatus;
}

enum rtw_phl_status phl_get_single_rx(struct phl_info_t *phl_info,
					 struct rtw_rx_buf_ring *rx_buf_ring,
					 u8 ch,
					 struct rtw_phl_rx_pkt **pphl_rx)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct rtw_phl_rx_pkt *phl_rx = NULL;
	struct rtw_rx_buf *rxbuf = NULL;
	u16 buf_size = 0;

	phl_rx = rtw_phl_query_phl_rx(phl_info);
	rxbuf = query_busy_rx_buf(phl_info, rx_buf_ring);

	do {
		if (NULL == phl_rx) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_,
			          "%s(%d) phl_rx out of resource\n",
			          __func__, __LINE__);
			break;
		}
		if (NULL == rxbuf) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
			          "%s(%d) [WARNING] queried NULL rxbuf\n",
			          __func__, __LINE__);
			break;
		}

		phl_rx->rxbuf_ptr = (u8 *)rxbuf;

		if (true != rtw_hal_check_rxrdy(phl_info->phl_com,
						phl_info->hal, rxbuf, ch)) {
			PHL_TRACE_LMT(COMP_PHL_DBG, _PHL_INFO_, "RX:%s(%d) packet not ready\n",
			          __func__, __LINE__);
#ifdef DEBUG_PHL_RX
			phl_info->rx_stats.rx_rdy_fail++;
#endif
			break;
		}

		if (true != rtw_hal_handle_rxbd_info(phl_info->hal,
						     rxbuf->vir_addr,
						     &buf_size)) {
#ifdef DEBUG_PHL_RX
			phl_info->rx_stats.rxbd_fail++;
#endif
			goto drop;
		}

#ifdef CONFIG_DYNAMIC_RX_BUF
		phl_rx->r.os_priv = rxbuf->os_priv;
#endif

		hstatus = rtw_hal_handle_rx_buffer(phl_info->phl_com,
		                                   phl_info->hal,
		                                   rxbuf->vir_addr,
		                                   buf_size, phl_rx);

		if (RTW_HAL_STATUS_SUCCESS != hstatus)
			goto drop;

		pstatus = RTW_PHL_STATUS_SUCCESS;
	} while (false);

	if (RTW_PHL_STATUS_SUCCESS != pstatus) {
		/* hana_todo cache validate api */
		if (NULL != rxbuf) {
			enqueue_busy_rx_buf(phl_info, rx_buf_ring, rxbuf, _first);
		}

		if (NULL != phl_rx) {
			phl_release_phl_rx(phl_info, phl_rx);
			phl_rx = NULL;
		}
	}
	*pphl_rx = phl_rx;

	return pstatus;

drop:
#ifdef DEBUG_PHL_RX
	phl_info->rx_stats.rx_drop_get++;
#endif
#ifdef CONFIG_DYNAMIC_RX_BUF
	/* avoid re-allocating buffer carried on rxbuf */
	phl_rx->type = RTW_RX_TYPE_MAX;
#endif
	phl_rx->r.mdata.dma_ch = ch;
	phl_recycle_rx_buf(phl_info, phl_rx);

	return RTW_PHL_STATUS_FRAME_DROP;
}

void phl_rx_handle_normal(struct phl_info_t *phl_info,
                          struct rtw_phl_rx_pkt *phl_rx)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	_os_list frames;
#ifdef CONFIG_DYNAMIC_RX_BUF
	struct rtw_rx_buf *rxbuf = (struct rtw_rx_buf *)phl_rx->rxbuf_ptr;
#endif
	FUNCIN_WSTS(pstatus);
	INIT_LIST_HEAD(&frames);

	/* unmap rx buffer */
#if defined(CONFIG_DYNAMIC_RX_BUF) && !defined(PHL_UNPAIRED_DMA_MAP_UNMAP)
	if (rxbuf->cache == CACHE_ADDR)
		_os_pkt_buf_unmap_rx(phl_to_drvpriv(phl_info), rxbuf->phy_addr_l,
	                     rxbuf->phy_addr_h, rxbuf->buf_len);
#endif /* CONFIG_DYNAMIC_RX_BUF && PHL_UNPAIRED_DMA_MAP_UNMAP */

	/* stat : rx rate counter */
	if (phl_rx->r.mdata.rx_rate <= RTW_DATA_RATE_HE_NSS4_MCS11)
		phl_info->phl_com->phl_stats.rx_rate_nmr[phl_rx->r.mdata.rx_rate]++;

	pstatus = phl_rx_reorder(phl_info, phl_rx, &frames);
	if (pstatus == RTW_PHL_STATUS_SUCCESS)
		phl_handle_rx_frame_list(phl_info, &frames);
	else
		PHL_TRACE(COMP_PHL_RECV, _PHL_WARNING_, "[WARNING]handle normal rx error (0x%08X)!\n", pstatus);

	FUNCOUT_WSTS(pstatus);
}

void _phl_wp_rpt_statistics(struct phl_info_t *phl_info, u8 ch, u16 wp_seq,
			    u8 txsts, struct rtw_xmit_req *treq)
{
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct rtw_wp_rpt_stats *rpt_stats = NULL;
	u32 diff_t = 0, cur_time = _os_get_cur_time_ms();

	rpt_stats = (struct rtw_wp_rpt_stats *)hal_com->trx_stat.wp_rpt_stats;

	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		  "recycle tx req(%p) in ch(%d) with wp_seq(0x%x) and wifi seq(%d)!\n",
		  treq, ch, wp_seq, treq->mdata.sw_seq);

#ifdef CONFIG_PHL_TX_DBG
	if (treq->tx_dbg.en_dbg) {
		treq->tx_dbg.recycle_wd_t = _os_get_cur_time_us();
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "tx dbg rpt: macid(%02d), tx_dbg_pkt_type(%02d), txsts(%d), sw_seq(%04d), total tx time(%08d) us\n",
			treq->mdata.macid, treq->tx_dbg.tx_dbg_pkt_type, txsts,
			treq->mdata.sw_seq, phl_get_passing_time_us(
			treq->tx_dbg.core_add_tx_t));
		PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "tx dbg rpt: core_add_tx_t(0x%08x), enq_pending_wd_t(0x%08x), recycle_wd_t(0x%08x)\n",
			treq->tx_dbg.core_add_tx_t,
			treq->tx_dbg.enq_pending_wd_t,
			treq->tx_dbg.recycle_wd_t);

		if(TX_STATUS_TX_DONE != txsts) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "tx dbg rpt: tx fail(%d)\n", txsts);
			if(NULL != treq->tx_dbg.statecb) {
				treq->tx_dbg.statecb(phl_to_drvpriv(phl_info), treq->tx_dbg.pctx, false);
			}
		} else {
			PHL_TRACE(COMP_PHL_DBG, _PHL_INFO_, "tx dbg rpt: tx done(%d)\n", txsts);
			if(NULL != treq->tx_dbg.statecb) {
				treq->tx_dbg.statecb(phl_to_drvpriv(phl_info), treq->tx_dbg.pctx, true);
			}
		}
	}
#endif /* CONFIG_PHL_TX_DBG */

	if (cur_time >= treq->tx_time)
		diff_t = cur_time - treq->tx_time;
	else
		diff_t = RTW_U32_MAX - treq->tx_time + cur_time;

	if (diff_t > WP_DELAY_THRES_MS) {
		if (TX_STATUS_TX_DONE == txsts)
			rpt_stats[ch].delay_tx_ok_cnt++;
		else if (TX_STATUS_TX_FAIL_REACH_RTY_LMT == txsts)
			rpt_stats[ch].delay_rty_fail_cnt++;
		else if (TX_STATUS_TX_FAIL_LIFETIME_DROP == txsts)
			rpt_stats[ch].delay_lifetime_drop_cnt++;
		else if (TX_STATUS_TX_FAIL_MACID_DROP == txsts)
			rpt_stats[ch].delay_macid_drop_cnt++;
	} else {
		if (TX_STATUS_TX_DONE == txsts)
			rpt_stats[ch].tx_ok_cnt++;
		else if (TX_STATUS_TX_FAIL_REACH_RTY_LMT == txsts)
			rpt_stats[ch].rty_fail_cnt++;
		else if (TX_STATUS_TX_FAIL_LIFETIME_DROP == txsts)
			rpt_stats[ch].lifetime_drop_cnt++;
		else if (TX_STATUS_TX_FAIL_MACID_DROP == txsts)
			rpt_stats[ch].macid_drop_cnt++;
	}

	if (txsts != TX_STATUS_TX_DONE && txsts != TX_STATUS_TX_FAIL_SW_DROP) {
		rpt_stats[ch].tx_fail_cnt++;
	}
}


void _phl_wp_rpt_chk_txsts(struct phl_info_t *phl_info, u8 ch, u16 wp_seq,
			    u8 txsts, struct rtw_xmit_req *treq)
{
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct rtw_wp_rpt_stats *rpt_stats = NULL;
	struct rtw_pkt_buf_list *pkt_buf = (struct rtw_pkt_buf_list *)treq->pkt_list;
	int i;

	rpt_stats = (struct rtw_wp_rpt_stats *)hal_com->trx_stat.wp_rpt_stats;

	if(TX_STATUS_TX_DONE != txsts) {
		if (TX_STATUS_TX_FAIL_REACH_RTY_LMT == txsts) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"this wp is tx fail (REACH_RTY_LMT): wp(%p), ch(%d), wp_seq(0x%x), hw_band(%d), macid(%d), Sw SN(%d), tid(%d), Rty_lmt_en/cnt(%d/%d)!\n",
				treq, ch, wp_seq, treq->mdata.band, treq->mdata.macid,
				treq->mdata.sw_seq, treq->mdata.tid,
				treq->mdata.data_tx_cnt_lmt_en,
				treq->mdata.data_tx_cnt_lmt);
		} else if (TX_STATUS_TX_FAIL_LIFETIME_DROP == txsts) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"this wp is tx fail (LIFETIME_DROP): wp(%p), ch(%d), wp_seq(0x%x), hw_band(%d), macid(%d), Sw SN(%d), tid(%d), Rty_lmt_en/cnt(%d/%d)!\n",
				treq, ch, wp_seq, treq->mdata.band, treq->mdata.macid,
				treq->mdata.sw_seq, treq->mdata.tid,
				treq->mdata.data_tx_cnt_lmt_en,
				treq->mdata.data_tx_cnt_lmt);
		} else if (TX_STATUS_TX_FAIL_MACID_DROP == txsts) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"this wp is tx fail (MACID_DROP): wp(%p), ch(%d), wp_seq(0x%x), hw_band(%d), macid(%d), Sw SN(%d), tid(%d), Rty_lmt_en/cnt(%d/%d)!\n",
				treq, ch, wp_seq, treq->mdata.band, treq->mdata.macid,
				treq->mdata.sw_seq, treq->mdata.tid,
				treq->mdata.data_tx_cnt_lmt_en,
				treq->mdata.data_tx_cnt_lmt);
		} else if(TX_STATUS_TX_FAIL_SW_DROP == txsts) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"this wp is tx fail (SW_DROP): wp(%p), ch(%d), wp_seq(0x%x), hw_band(%d), macid(%d), Sw SN(%d), tid(%d), Rty_lmt_en/cnt(%d/%d)!\n",
				treq, ch, wp_seq, treq->mdata.band, treq->mdata.macid,
				treq->mdata.sw_seq, treq->mdata.tid,
				treq->mdata.data_tx_cnt_lmt_en,
				treq->mdata.data_tx_cnt_lmt);
		} else {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_,
				"this wp is tx fail (UNKNOWN(%d)): wp(%p), ch(%d), wp_seq(0x%x), hw_band(%d), macid(%d), Sw SN(%d), tid(%d), Rty_lmt_en/cnt(%d/%d)!\n",
				txsts, treq, ch, treq->mdata.band, wp_seq, treq->mdata.macid,
				treq->mdata.sw_seq, treq->mdata.tid,
				treq->mdata.data_tx_cnt_lmt_en,
				treq->mdata.data_tx_cnt_lmt);
		}

		/* dump tx fail mac hdr */
		if(MAC_HDR_LEN <= pkt_buf[0].length) {
			PHL_DATA(COMP_PHL_XMIT, _PHL_INFO_, "=== Dump Tx MAC HDR ===");
			for (i = 0; i < MAC_HDR_LEN; i++) {
				if (!(i % 8))
					PHL_DATA(COMP_PHL_XMIT, _PHL_INFO_, "\n");
				PHL_DATA(COMP_PHL_XMIT, _PHL_INFO_, "%02X ", pkt_buf[0].vir_addr[i]);
			}
			PHL_DATA(COMP_PHL_XMIT, _PHL_INFO_, "\n");
		}
		#ifdef DBG_DUMP_TX_COUNTER
		rtw_hal_dump_tx_status(phl_info->hal, treq->mdata.band);
		#endif

	}

	if (treq->txfb) {
		treq->txfb->txsts = txsts;
		if (treq->txfb->txfb_cb)
			treq->txfb->txfb_cb(treq->txfb);
	}
}

void phl_recycle_payload(struct phl_info_t *phl_info, u8 dma_ch, u16 wp_seq,
			 u8 txsts)
{
	enum rtw_phl_status sts = RTW_PHL_STATUS_FAILURE;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct rtw_wp_rpt_stats *rpt_stats =
		(struct rtw_wp_rpt_stats *)hal_com->trx_stat.wp_rpt_stats;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_phl_evt_ops *ops = &phl_info->phl_com->evt_ops;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct rtw_xmit_req *treq = NULL;
	u16 macid = 0;

#ifdef CONFIG_VW_REFINE
	u8 sts_vw_cnt =0;
#endif

	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;
	treq = (struct rtw_xmit_req *)wd_ring[dma_ch].wp_tag[wp_seq].ptr;

	if (NULL == treq) {
		PHL_WARN("NULL == treq, dma_ch(%d), wp_seq(0x%x), txsts(%d)\n",
			dma_ch, wp_seq, txsts);
		goto end;
	}

	macid = treq->mdata.macid;

	_phl_wp_rpt_statistics(phl_info, dma_ch, wp_seq, txsts, treq);
	_phl_wp_rpt_chk_txsts(phl_info, dma_ch, wp_seq, txsts, treq);

#ifdef CONFIG_VW_REFINE
	sts_vw_cnt = treq->vw_cnt;
#endif

	if (treq->local_buf)
		_phl_free_local_buf_pcie(phl_info, treq);

	if (RTW_PHL_TREQ_TYPE_TEST_PATTERN == treq->treq_type) {
		if (NULL == ops->tx_test_recycle)
			goto end;
		PHL_INFO("call tx_test_recycle\n");
		sts = ops->tx_test_recycle(phl_info, treq);
	} else if (RTW_PHL_TREQ_TYPE_NORMAL == treq->treq_type
#if defined(CONFIG_CORE_TXSC) || defined(CONFIG_PHL_TXSC)
		   || RTW_PHL_TREQ_TYPE_CORE_TXSC == treq->treq_type
		   || RTW_PHL_TREQ_TYPE_PHL_ADD_TXSC == treq->treq_type
#endif
	) {
		if (NULL == ops->tx_recycle)
			goto end;
		PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "call tx_recycle\n");
		sts = ops->tx_recycle(phl_to_drvpriv(phl_info), treq);
	}

end:
	if (RTW_PHL_STATUS_SUCCESS != sts) {
		PHL_WARN("tx recycle fail\n");
		rpt_stats[dma_ch].recycle_fail_cnt++;
	} else {
		_os_spinlock(phl_to_drvpriv(phl_info),
			     &wd_ring[dma_ch].wp_tag_lock,
			     _bh, NULL);
		wd_ring[dma_ch].wp_tag[wp_seq].ptr = NULL;
		rpt_stats[dma_ch].busy_cnt--;
		_dec_sta_rpt_stats_busy_cnt(phl_info, macid);
#ifdef RTW_WKARD_DYNAMIC_LTR
		if (true ==
		    _phl_judge_idle_ltr_switching_conditions(phl_info, macid))
			_phl_switch_idle_ltr(phl_info, rpt_stats);
#endif
		_os_spinunlock(phl_to_drvpriv(phl_info),
			       &wd_ring[dma_ch].wp_tag_lock,
			       _bh, NULL);

#ifdef CONFIG_VW_REFINE
		if (0 == dma_ch) {
			phl_info->free_wp[phl_info->fw_ptr] = wp_seq;
			phl_info->fw_ptr = (phl_info->fw_ptr + 1) % WP_MAX_SEQ_NUMBER;
		}

		if (TX_STATUS_TX_DONE == txsts)
			hal_com->trx_stat.vw_cnt_rev += sts_vw_cnt;
		else
			hal_com->trx_stat.vw_cnt_err += sts_vw_cnt;
#endif

	}
	/* phl_indic_pkt_complete(phl_info); */
}

void _phl_rx_handle_wp_report(struct phl_info_t *phl_info,
							struct rtw_phl_rx_pkt *phl_rx)
{
	struct rtw_recv_pkt *r = &phl_rx->r;
	u8 *pkt = NULL;
	u16 pkt_len = 0;
	u16 wp_seq = 0, rsize = 0;
	u8 sw_retry = 0, dma_ch = 0, txsts = 0;
	u8 macid = 0, ac_queue = 0;

	pkt = r->pkt_list[0].vir_addr;
	pkt_len = r->pkt_list[0].length;

	while (pkt_len > 0) {
		rsize = rtw_hal_handle_wp_rpt(phl_info->hal, pkt, pkt_len,
					      &sw_retry, &dma_ch, &wp_seq,
					      &macid, &ac_queue, &txsts);

		if (0 == rsize)
			break;

#ifdef DEBUG_PHL_RX
		phl_info->rx_stats.rx_type_wp++;
#endif
		phl_rx_wp_report_record_sts(phl_info, macid, ac_queue, txsts);

		if (false == sw_retry) {
			phl_recycle_payload(phl_info, dma_ch, wp_seq, txsts);
		} else {
			/* hana_todo handle sw retry */
			phl_recycle_payload(phl_info, dma_ch, wp_seq, txsts);
		}
		pkt += rsize;
		pkt_len -= rsize;
	}
}


static void phl_rx_process_pcie(struct phl_info_t *phl_info,
							struct rtw_phl_rx_pkt *phl_rx)
{
#ifdef DEBUG_PHL_RX
	phl_info->rx_stats.rx_type_all++;
#endif

	switch (phl_rx->type) {
	case RTW_RX_TYPE_WIFI:
#ifdef DEBUG_PHL_RX
		phl_info->rx_stats.rx_type_wifi++;
		if (phl_rx->r.pkt_list[0].length == phl_info->cnt_rx_pktsz)
			phl_info->rx_stats.rx_pktsz_phl++;
		if (phl_rx->r.mdata.amsdu)
			phl_info->rx_stats.rx_amsdu++;
#endif
#ifdef CONFIG_PHL_RX_PSTS_PER_PKT
		if (false == phl_rx_proc_wait_phy_sts(phl_info, phl_rx)) {
			PHL_TRACE(COMP_PHL_PSTS, _PHL_DEBUG_,
				  "phl_rx_proc_wait_phy_sts() return false \n");
			phl_rx_handle_normal(phl_info, phl_rx);
		}
#else
#ifdef CONFIG_PHL_SNIFFER_SUPPORT
		/* Sniffer mode without PSTS PER PKT: generate radiotap only from RxDesc */
		phl_rx_proc_snif_info_wo_psts(phl_info, phl_rx);
#endif
		phl_rx_handle_normal(phl_info, phl_rx);
#endif
		break;
	case RTW_RX_TYPE_TX_WP_RELEASE_HOST:
		_phl_rx_handle_wp_report(phl_info, phl_rx);
		phl_recycle_rx_buf(phl_info, phl_rx);
		break;
	case RTW_RX_TYPE_PPDU_STATUS:
#ifdef DEBUG_PHL_RX
		phl_info->rx_stats.rx_type_ppdu++;
#endif
		phl_rx_proc_ppdu_sts(phl_info, phl_rx);
#ifdef CONFIG_PHL_RX_PSTS_PER_PKT
		phl_rx_proc_phy_sts(phl_info, phl_rx);
#endif
		phl_recycle_rx_buf(phl_info, phl_rx);
		break;
	case RTW_RX_TYPE_C2H:
#ifdef DEBUG_PHL_RX
		phl_info->rx_stats.rx_type_c2h++;
#endif
		phl_recycle_rx_buf(phl_info, phl_rx);
		break;
	case RTW_RX_TYPE_CHANNEL_INFO:
#ifdef CONFIG_PHL_CHANNEL_INFO
		phl_recycle_rx_buf(phl_info, phl_rx);
		break;
#endif
	case RTW_RX_TYPE_TX_RPT:
	case RTW_RX_TYPE_DFS_RPT:
	case RTW_RX_TYPE_MAX:
		PHL_TRACE(COMP_PHL_RECV, _PHL_WARNING_, "phl_rx_process_pcie(): Unsupported case:%d, please check it\n",
				phl_rx->type);
		phl_recycle_rx_buf(phl_info, phl_rx);
		break;
	default :
		PHL_TRACE(COMP_PHL_RECV, _PHL_WARNING_, "[WARNING] unrecognize rx type\n");
		phl_recycle_rx_buf(phl_info, phl_rx);
		break;
	}
}

static u16 _phl_get_idle_rxbuf_cnt(struct phl_info_t *phl_info,
					struct rtw_rx_buf_ring *rx_buf_ring)
{
	return rx_buf_ring->idle_rxbuf_cnt;
}

static enum rtw_phl_status phl_rx_pcie(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct rtw_rx_buf_ring *rx_buf_ring = NULL;
	struct rx_base_desc *rxbd = NULL;
	struct rtw_phl_rx_pkt *phl_rx = NULL;
	u16 i = 0, rxcnt = 0, idle_rxbuf_cnt = 0;
	u8 ch = 0;

	FUNCIN_WSTS(pstatus);

#ifdef DEBUG_PHL_RX
	phl_info->rx_stats.phl_rx++;
#endif

	rx_buf_ring = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;
	rxbd = (struct rx_base_desc *)hci_info->rxbd_buf;

#ifdef CONFIG_DYNAMIC_RX_BUF
	/* The empty rxbuf (w/o available buffer) happen only on RTW_RX_TYPE_WIFI */
	refill_empty_rx_buf(phl_info, &rx_buf_ring[0]);
#endif

	for (ch = 0; ch < hci_info->total_rxch_num; ch++) {
		rxcnt = phl_calc_avail_rptr(rxbd[ch].host_idx, rxbd[ch].hw_idx,
		                            (u16)hal_com->bus_cap.rxbd_num);
		if (rxcnt == 0) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_,
				"no avail hw rx\n");
			pstatus = RTW_PHL_STATUS_SUCCESS;
			continue;
		}

		idle_rxbuf_cnt = _phl_get_idle_rxbuf_cnt(phl_info,
							 &rx_buf_ring[ch]);

		if (idle_rxbuf_cnt == 0) {
			PHL_WARN("%s, idle rxbuf is empty. (ch = %d)\n",
				 __func__, ch);
			phl_dump_all_sta_rx_info(phl_info);
			PHL_INFO("phl_rx_ring stored rx number = %d\n",
				 rtw_phl_query_new_rx_num(phl_info));
#ifdef PHL_RX_BATCH_IND
			if (ch == 0)
				_phl_indic_new_rxpkt(phl_info);
#endif
			pstatus = RTW_PHL_STATUS_SUCCESS;
			continue;
		}

#ifdef PHL_RXSC_ISR
		/* check if rpq and intr is enable, skip rpq check if rpq intr is not en */
		if (!rtw_hal_rpq_isr_check(phl_info->hal, ch)) {
			/* phl_info->cnt_rx_chk_skip[ch]++;*/
			pstatus = RTW_PHL_STATUS_SUCCESS;
			continue;
		}
#endif

		/* only handle affordable amount of rxpkt */
		if (rxcnt > idle_rxbuf_cnt) {
			PHL_WARN("rxcnt %d is lager than idle rxbuf cnt %d.\n", rxcnt, idle_rxbuf_cnt);
			rxcnt = idle_rxbuf_cnt;
		}

		for (i = 0; i < rxcnt; i++) {
			pstatus = phl_get_single_rx(phl_info, &rx_buf_ring[ch],
							ch, &phl_rx);
			if (RTW_PHL_STATUS_FRAME_DROP == pstatus)
				continue;
			if (NULL == phl_rx) {
				rxcnt = i;
				break;
			}

			/* hana_todo */
			phl_rx->r.mdata.dma_ch = ch;
			phl_rx_process_pcie(phl_info, phl_rx);
		}

#ifdef PHL_RX_BATCH_IND
		if (ch == 0 && phl_info->rx_new_pending)
			_phl_indic_new_rxpkt(phl_info);
#endif

		pstatus = _phl_refill_rxbd(phl_info, &rx_buf_ring[ch],
							&rxbd[ch], ch, rxcnt);

		if (RTW_PHL_STATUS_RESOURCE == pstatus)
			PHL_WARN("%s, rxcnt is not refilled %d.\n", __func__ , rxcnt);

		if (RTW_PHL_STATUS_SUCCESS != pstatus)
			continue;
	}

	FUNCOUT_WSTS(pstatus);

	return pstatus;
}

enum rtw_phl_status phl_pltfm_tx_pcie(struct phl_info_t *phl_info, void *pkt)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	enum rtw_hal_status hstatus = RTW_HAL_STATUS_FAILURE;
	struct rtw_h2c_pkt *h2c_pkt = (struct rtw_h2c_pkt *)pkt;
	struct tx_base_desc *txbd = NULL;
	struct phl_h2c_pkt_pool *h2c_pool = NULL;
	struct rtw_wd_page wd;
	u8 fwcmd_queue_idx = 0;

	txbd = (struct tx_base_desc *)phl_info->hci->txbd_buf;
	h2c_pool = (struct phl_h2c_pkt_pool *)phl_info->h2c_pool;

	_os_mem_set(phl_to_drvpriv(phl_info), &wd, 0, sizeof(wd));
	/* fowart h2c pkt information into the format of wd page */
	wd.phy_addr_l = h2c_pkt->phy_addr_l + (u32)(h2c_pkt->vir_data - h2c_pkt->vir_head);
	wd.phy_addr_h= h2c_pkt->phy_addr_h;
	wd.buf_len = h2c_pkt->data_len;
	wd.ls = 1;

	PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : wd.phy_addr_l %x, wd.phy_addr_h %x\n", __func__ , wd.phy_addr_l, wd.phy_addr_h);
	PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : buf_len %x.\n", __func__ , wd.buf_len);

	fwcmd_queue_idx = rtw_hal_get_fwcmd_queue_idx(phl_info->hal);

	_os_spinlock(phl_to_drvpriv(phl_info), &txbd[fwcmd_queue_idx].txbd_lock, _bh, NULL);
	hstatus = rtw_hal_update_txbd(phl_info->hal, txbd, &wd, fwcmd_queue_idx, 1);

	h2c_pkt->host_idx = wd.host_idx;

	PHL_TRACE(COMP_PHL_DBG, _PHL_DEBUG_, "%s : h2c_pkt->host_idx %d.\n", __func__, h2c_pkt->host_idx);

	if (RTW_HAL_STATUS_SUCCESS == hstatus)
			pstatus = phl_enqueue_busy_h2c_pkt(phl_info, h2c_pkt, _tail);

	if (RTW_PHL_STATUS_SUCCESS == pstatus) {
		hstatus = rtw_hal_trigger_txstart(phl_info->hal, txbd, fwcmd_queue_idx);
		if (RTW_HAL_STATUS_SUCCESS != hstatus) {
			PHL_TRACE(COMP_PHL_DBG, _PHL_WARNING_, "[WARNING]update Tx RW ptr fail!\n");
			pstatus = RTW_PHL_STATUS_FAILURE;
		}
	}

	_os_spinunlock(phl_to_drvpriv(phl_info), &txbd[fwcmd_queue_idx].txbd_lock, _bh, NULL);

	return pstatus;
}

void *phl_get_txbd_buf_pcie(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = phl_info->hci;

	return hci_info->txbd_buf;
}

void *phl_get_rxbd_buf_pcie(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = phl_info->hci;

	return hci_info->rxbd_buf;
}

void phl_recycle_rx_pkt_pcie(struct phl_info_t *phl_info,
				struct rtw_phl_rx_pkt *phl_rx)
{
#ifdef CONFIG_DYNAMIC_RX_BUF
	struct rtw_rx_buf *rx_buf = (struct rtw_rx_buf *)phl_rx->rxbuf_ptr;

	rx_buf->reuse = true;
#endif

	phl_recycle_rx_buf(phl_info, phl_rx);
}

#ifdef CONFIG_VW_REFINE
#if defined(RTW_RX_CPU_BALANCE) || defined(RTW_TX_CPU_BALANCE)
void rtw_phl_dump_cpu_id(struct phl_info_t *phl_info, u32 value)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_phl_handler *tx_handler = &phl_info->phl_tx_handler;
	struct rtw_phl_handler *rx_handler = &phl_info->phl_rx_handler;

	if (value == _CMD_LIST_CPUID) {
		PHL_ERR("phl_tx:%d phl_rx:%d cpu1:%d cpu0:%d cpu3:%d\n",
			tx_handler->cpu_id,
			rx_handler->cpu_id,
			CPU_ID_TX_PHL_1,
			CPU_ID_TX_PHL_0,
			CPU_ID_TX_CORE);
	} else if (value >= _CMD_CPUID_OFFSET) {
		rx_handler->cpu_id = (value - _CMD_CPUID_OFFSET);
	}
}
#endif

void rtw_phl_dump_wd_info(struct phl_info_t *phl_info, u32 value)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rtw_wd_page_ring *wd_ring = NULL;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);

	wd_ring = (struct rtw_wd_page_ring *)hci_info->wd_ring;

	if (value == _CMD_DUMP_WD_INFO) {
		PHL_ERR("wd_info: idle:%d busy:%d pending:%d pretx_f:%d phltx:%d\n",
			wd_ring[0].idle_wd_page_cnt,
			wd_ring[0].busy_wd_page_cnt,
			wd_ring[0].pending_wd_page_cnt,
			hal_com->trx_stat.pretx_fail,
			hal_com->trx_stat.phltx_cnt);

		PHL_ERR("\t fw_ptr:%d fr_ptr:%d\n",
			phl_info->fw_ptr,
			phl_info->fr_ptr);
	} else if (value == _CMD_LIST_WP_INFO) {
		u32 i, used_cnt =0;

		for(i = 0; i < WP_MAX_SEQ_NUMBER ; i++) {
			if (NULL != wd_ring[0].wp_tag[i].ptr)
				used_cnt++;
		}
		PHL_ERR("wp used_cnt:%d avail_cnt:%d\n",
				used_cnt, WP_MAX_SEQ_NUMBER);
	}  else if (value >= _CMD_SHOW_WP_OFFSET) {
		value = value - _CMD_SHOW_WP_OFFSET;
		if (value < WP_MAX_CNT) {
			if (NULL != wd_ring[0].wp_tag[value].ptr)
				PHL_ERR("wp :%d is not empty\n", value);
			else
				PHL_ERR("wp :%d is not null\n", value);
		}
	}
}

void rtw_phl_debug_reset_wp_ptr(struct phl_info_t *phl_info, u32 value)
{
	u16 i;

	if (value == _CMD_DUMP_WP_PTR) {
		for(i = 0; i < WP_MAX_CNT; i++)
			PHL_ERR(" idx :%d seq:%d \n", i, phl_info->free_wp[i]);

		PHL_ERR(" fw_ptr:%d fr_ptr:%d \n",
			phl_info->fw_ptr , phl_info->fr_ptr);
	} else if (value == _CMD_RESET_WP_PTR) {
		for(i = 0; i < WP_MAX_CNT; i++)
			phl_info->free_wp[i] = i;

		phl_info->fr_ptr = 0;
		phl_info->fw_ptr = 0;
	}
}

void
phl_dump_wd_info(struct phl_info_t *phl_info, u32 val)
{
#if defined(RTW_RX_CPU_BALANCE) || defined(RTW_TX_CPU_BALANCE)
	if (val < _CMD_MAX_CPUID_VAL)
		rtw_phl_dump_cpu_id(phl_info, val);
	else
#endif
	if (val < _CMD_MAX_RESET_WP_VAL)
		rtw_phl_debug_reset_wp_ptr(phl_info, (val - _CMD_MAX_CPUID_VAL));
	else if (val < _CMD_MAX_WD_VAL)
		rtw_phl_dump_wd_info(phl_info, (val - _CMD_MAX_RESET_WP_VAL));
	else
		rtw_phl_dump_wd_info(phl_info, val);
}
#endif

void
phl_tx_watchdog_pcie(struct phl_info_t *phl_info)
{
	struct rtw_stats *phl_stats = NULL;

	phl_stats = &phl_info->phl_com->phl_stats;

	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		  "\n=== Tx statistics === \n");
	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		"\nunicast tx bytes	: %llu\n", phl_stats->tx_byte_uni);
	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		"total tx bytes		: %llu\n", phl_stats->tx_byte_total);
	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		 "tx throughput		: %d(kbps)\n",
			 (int)phl_stats->tx_tp_kbits);
	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		"last tx time		: %d(ms)\n",
			 (int)phl_stats->last_tx_time_ms);
	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		"tx request num to phl	: %d\n",
			 (int)phl_stats->txreq_num);

	#ifdef RTW_WKARD_DYNAMIC_LTR
	if (rtw_hal_ltr_is_sw_ctrl(phl_info->phl_com, phl_info->hal)) {
		PHL_INFO(
			"ltr sw ctrl 			: %u\n",
			rtw_hal_ltr_is_sw_ctrl(phl_info->phl_com, phl_info->hal) ? 1 : 0);
		PHL_INFO(
			"ltr current state 		: %u\n",
			phl_ltr_get_cur_state(phl_info->phl_com));
		PHL_INFO(
			"ltr active trigger cnt : %lu\n",
			phl_ltr_get_tri_cnt(phl_info->phl_com, RTW_PCIE_LTR_SW_ACT));
		PHL_INFO(
			"ltr idle trigger cnt   : %lu\n",
			phl_ltr_get_tri_cnt(phl_info->phl_com, RTW_PCIE_LTR_SW_IDLE));
		PHL_INFO(
			"ltr last trigger time  : %lu\n",
			phl_ltr_get_last_trigger_time(phl_info->phl_com));
	}
	#endif

	_phl_dump_wp_stats(phl_info);

	_phl_dump_busy_wp(phl_info);

	PHL_TRACE(COMP_PHL_XMIT, _PHL_DEBUG_,
		  "\n===================== \n");

}

void
phl_read_hw_rx(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;
	struct rx_base_desc *rxbd = NULL;
	u16 host_idx = 0;
	u8 ch = 0;

	rxbd = (struct rx_base_desc *)hci_info->rxbd_buf;

	for (ch = 0; ch < hci_info->total_rxch_num; ch++) {
		rtw_hal_rx_res_query(phl_info->hal, ch, &host_idx,
		                     &rxbd[ch].hw_idx);
	}
}

static struct phl_hci_trx_ops ops= {0};
void phl_hci_trx_ops_init(void)
{
	ops.hci_trx_init = phl_trx_init_pcie;
	ops.hci_trx_deinit = phl_trx_deinit_pcie;
	ops.prepare_tx = phl_prepare_tx_pcie;
	ops.recycle_rx_buf = phl_release_target_rx_buf;
	ops.tx = phl_tx_pcie;
	ops.rx = phl_rx_pcie;
	ops.trx_cfg = phl_trx_config_pcie;
	ops.trx_stop = phl_trx_stop_pcie;
	ops.recycle_busy_wd = phl_recycle_busy_wd;
	ops.recycle_busy_h2c = phl_recycle_busy_h2c;
	ops.read_hw_rx = phl_read_hw_rx;
	ops.pltfm_tx = phl_pltfm_tx_pcie;
	ops.alloc_h2c_pkt_buf = _phl_alloc_h2c_pkt_buf_pcie;
	ops.free_h2c_pkt_buf = _phl_free_h2c_pkt_buf_pcie;
	ops.trx_reset = phl_trx_reset_pcie;
	ops.trx_resume = phl_trx_resume_pcie;
	ops.tx_reset_hwband = phl_tx_reset_hwband_pcie;
	ops.req_tx_stop = phl_req_tx_stop_pcie;
	ops.req_rx_stop = phl_req_rx_stop_pcie;
	ops.is_tx_pause = phl_is_tx_sw_pause_pcie;
	ops.is_rx_pause = phl_is_rx_sw_pause_pcie;
	ops.get_txbd_buf = phl_get_txbd_buf_pcie;
	ops.get_rxbd_buf = phl_get_rxbd_buf_pcie;
	ops.recycle_rx_pkt = phl_recycle_rx_pkt_pcie;
	ops.rx_handle_normal = phl_rx_handle_normal;
	ops.tx_watchdog = phl_tx_watchdog_pcie;
	ops.return_tx_wps = phl_tx_return_all_wps;
#ifdef CONFIG_VW_REFINE
	ops.dump_wd_info = phl_dump_wd_info;
#endif
}


enum rtw_phl_status phl_hook_trx_ops_pci(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;

	if (NULL != phl_info) {
		phl_hci_trx_ops_init();
		phl_info->hci_trx_ops = &ops;
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}

enum rtw_phl_status phl_cmd_set_l2_leave(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;

#ifdef CONFIG_CMD_DISP
	pstatus = phl_cmd_enqueue(phl_info, HW_BAND_0, MSG_EVT_HAL_SET_L2_LEAVE, NULL, 0, NULL, PHL_CMD_WAIT, 0);

	if (is_cmd_failure(pstatus)) {
		/* Send cmd success, but wait cmd fail*/
		pstatus = RTW_PHL_STATUS_FAILURE;
	} else if (pstatus != RTW_PHL_STATUS_SUCCESS) {
		/* Send cmd fail */
		pstatus = RTW_PHL_STATUS_FAILURE;
	}
#else
	if (rtw_hal_set_l2_leave(phl_info->hal) == RTW_HAL_STATUS_SUCCESS)
		pstatus = RTW_PHL_STATUS_SUCCESS;
#endif
	return pstatus;
}

#ifdef CONFIG_PHL_PCI_TRX_RES_DBG
void rtw_phl_get_rxbd(void *phl, u8 ch, u16 *host_idx, u16 *hw_idx, u16 *hw_res)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	*hw_res = rtw_hal_rx_res_query(phl_info->hal, ch, host_idx, hw_idx);
}

u8 rtw_phl_get_rxch_num(void *phl)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;

	return hci_info->total_rxch_num;
}

void rtw_phl_get_txbd(void *phl, u8 ch, u16 *host_idx, u16 *hw_idx, u16 *hw_res)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	*hw_res = rtw_hal_tx_res_query(phl_info->hal, ch, host_idx, hw_idx);
}

u8 rtw_phl_get_txch_num(void *phl)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl_info->hci;

	return hci_info->total_txch_num;
}
#endif /* CONFIG_PHL_PCI_TRX_RES_DBG */
