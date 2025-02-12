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
#define _PHL_TRX_SDIO_C_
#include "../phl_headers.h"

#ifdef SDIO_TX_THREAD
#define XMIT_BUFFER_RETRY_LIMIT		0x100	/* > 0xFF: No limit */
#endif /* SDIO_TX_THREAD */

struct rtw_tx_buf {
#ifdef SDIO_TX_THREAD
	_os_list list;

	enum rtw_packet_type tag;	/* return queue type */
	u8 mgnt_pkt;			/* used for xmit management frame */
	u8 retry;			/* TX retry count */
#endif

	u8 *buffer;			/* DMA:able scratch buffer */
	u32 buf_len;			/* buffer size */
	u32 used_len;			/* total valid data size */
	u8 dma_ch;
	u8 agg_cnt;			/* bus aggregation nubmer */
#ifndef PHL_SDIO_TX_AGG_MAX
#define PHL_SDIO_TX_AGG_MAX	50
#endif
	u16 pkt_len[PHL_SDIO_TX_AGG_MAX];
	u8 wp_offset[PHL_SDIO_TX_AGG_MAX];
};

#ifdef SDIO_TX_THREAD
struct rtw_tx_buf_ring {
	struct rtw_tx_buf *txbufblock;
	u32 block_cnt_alloc;		/* Total number of rtw_tx_buf allocated */
	u32 total_blocks_size;		/* block_cnt_alloc * sizeof(rtw_tx_buf) */

	struct phl_queue idle_list;
	struct phl_queue busy_list;		/* ready to send buffer list */
	struct phl_queue mgnt_idle_list;	/* management buffer list */
	struct phl_queue mgnt_busy_list;	/* ready to send management buffer list */
};
#endif /* SDIO_TX_THREAD */

struct rtw_rx_buf_ring {
	struct rtw_rx_buf *rxbufblock;
	u32 block_cnt_alloc;	/* Total number of rtw_rx_buf allocated */
	u32 total_blocks_size;	/* block_cnt_alloc * sizeof(rtw_rx_buf) */

	struct phl_queue idle_rxbuf_list;
	struct phl_queue busy_rxbuf_list;
	struct phl_queue pend_rxbuf_list;
};

#ifdef SDIO_TX_THREAD
void phl_tx_sdio_wake_thrd(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci = phl_info->hci;


	_os_sema_up(phl_to_drvpriv(phl_info), &hci->tx_thrd_sema);
}

static void enqueue_txbuf(struct phl_info_t *phl_info,
			  struct phl_queue *pool_list, struct rtw_tx_buf *txbuf,
			  enum list_pos pos)
{
	void *drv = phl_to_drvpriv(phl_info);


	pq_push(drv, pool_list, &txbuf->list, pos, _ps);
}

static struct rtw_tx_buf *dequeue_txbuf(struct phl_info_t *phl_info,
					struct phl_queue *pool_list)
{
	struct rtw_tx_buf *txbuf = NULL;
	void *drv = phl_to_drvpriv(phl_info);
	_os_list *buflist;
	u8 res;


	res = pq_pop(drv, pool_list, &buflist, _first, _ps);
	if (!res)
		return NULL;

	txbuf = list_entry(buflist, struct rtw_tx_buf, list);

	return txbuf;
}

static struct rtw_tx_buf* alloc_txbuf(struct phl_info_t *phl_info,
				      struct rtw_tx_buf_ring *pool,
                                      enum rtw_phl_ring_cat cat)
{
	struct rtw_tx_buf *txbuf = NULL;


	if (cat == RTW_PHL_RING_CAT_MGNT) {
		txbuf = dequeue_txbuf(phl_info, &pool->mgnt_idle_list);
		if (txbuf)
			return txbuf;
	}

	txbuf = dequeue_txbuf(phl_info, &pool->idle_list);
	if (!txbuf)
		return NULL;

	if (cat == RTW_PHL_RING_CAT_MGNT)
		txbuf->mgnt_pkt = true;

	return txbuf;
}

/*
 * Enqueue tx buffer to queue tail and notify TX I/O thread to send.
 * Usually this function would be called outside TX I/O thread.
 */
static void enqueue_busy_txbuf(struct phl_info_t *phl_info,
			       struct rtw_tx_buf_ring *pool,
			       struct rtw_tx_buf *txbuf)
{
	if ((txbuf->tag == RTW_PHL_PKT_TYPE_MGNT) || (txbuf->mgnt_pkt))
		enqueue_txbuf(phl_info, &pool->mgnt_busy_list, txbuf, _tail);
	else
		enqueue_txbuf(phl_info, &pool->busy_list, txbuf, _tail);

	_os_atomic_set(phl_to_drvpriv(phl_info), &phl_info->phl_sw_tx_more, 1);
	phl_tx_sdio_wake_thrd(phl_info);
}

/*
 * Enqueue tx buffer to queue head but without notifying TX I/O thread again.
 * Usually this function would be called inside TX I/O thread.
 */
static void enqueue_busy_txbuf_to_head(struct phl_info_t *phl_info,
				       struct rtw_tx_buf_ring *pool,
				       struct rtw_tx_buf *txbuf)
{
	if ((txbuf->tag == RTW_PHL_PKT_TYPE_MGNT) || (txbuf->mgnt_pkt))
		enqueue_txbuf(phl_info, &pool->mgnt_busy_list, txbuf, _first);
	else
		enqueue_txbuf(phl_info, &pool->busy_list, txbuf, _first);

	_os_atomic_set(phl_to_drvpriv(phl_info), &phl_info->phl_sw_tx_more, 1);
}

static struct rtw_tx_buf* dequeue_busy_txbuf(struct phl_info_t *phl_info,
					     struct rtw_tx_buf_ring *pool)
{
	struct rtw_tx_buf *txbuf;

	txbuf = dequeue_txbuf(phl_info, &pool->mgnt_busy_list);
	if (!txbuf) {
		txbuf = dequeue_txbuf(phl_info, &pool->busy_list);
		if (!txbuf)
			_os_atomic_set(phl_to_drvpriv(phl_info), &phl_info->phl_sw_tx_more, 0);
	}
	return txbuf;
}

static void free_txbuf(struct phl_info_t *phl_info,
		       struct rtw_tx_buf_ring *pool, struct rtw_tx_buf *txbuf)
{
	struct phl_queue *pool_list;


	txbuf->retry = 0;
	txbuf->used_len = 0;
	txbuf->agg_cnt = 0;
	if (txbuf->tag == RTW_PHL_PKT_TYPE_MGNT) {
		pool_list = &pool->mgnt_idle_list;
	} else {
		txbuf->mgnt_pkt = false;
		pool_list = &pool->idle_list;
	}

	enqueue_txbuf(phl_info, pool_list, txbuf, _tail);
}

#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
#ifndef RTW_WKARD_SDIO_TX_USE_YIELD
/**
 * txbuf_wait - waits for idle tx buffer (w/timeout)
 * @phl_info:	pointer of struct phl_info_t
 * @timeout:	timeout value in millisecond
 *
 * This waits for either a tx buffer has been returned to idle list or for a
 * specified timeout to expire. The timeout is in millisecond.
 *
 * Return: 0 if timed out, 1 if tx buffer idle list is not empty, and positive
 * if completed.
 */
static int txbuf_wait(struct phl_info_t *phl_info, int timeout)
{
	void *drv = phl_to_drvpriv(phl_info);
	struct hci_info_t *hci = phl_info->hci;
	struct rtw_tx_buf_ring *tx_pool = (struct rtw_tx_buf_ring *)hci->txbuf_pool;
	_os_event event;
	_os_spinlockfg sp_flags;
	int ret = 1;


	_os_spinlock(drv, &hci->tx_buf_lock, _ps, &sp_flags);
	if (tx_pool->idle_list.cnt == 0) {
		_os_event_init(drv, &event);
		hci->tx_buf_event = &event;
		_os_spinunlock(drv, &hci->tx_buf_lock, _ps, &sp_flags);
		ret = _os_event_wait(drv, &event, timeout);
		_os_spinlock(drv, &hci->tx_buf_lock, _ps, &sp_flags);
		if (hci->tx_buf_event)
			hci->tx_buf_event = NULL;
		_os_event_free(drv, &event);
	}
	_os_spinunlock(drv, &hci->tx_buf_lock, _ps, &sp_flags);

	return ret;
}

static void txbuf_set_ready(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	struct hci_info_t *hci = phl_info->hci;
	_os_spinlockfg sp_flags;


	_os_spinlock(drv, &hci->tx_buf_lock, _ps, &sp_flags);
	if (hci->tx_buf_event) {
		_os_event_set(drv, hci->tx_buf_event);
		hci->tx_buf_event = NULL;
	}
	_os_spinunlock(drv, &hci->tx_buf_lock, _ps, &sp_flags);
}
#endif /* !RTW_WKARD_SDIO_TX_USE_YIELD */
#endif /* CONFIG_PHL_SDIO_TX_CB_THREAD */
#endif /* SDIO_TX_THREAD */

static void enqueue_rxbuf(struct phl_info_t *phl_info,
			  struct phl_queue *pool_list, struct rtw_rx_buf *rxbuf)
{
	void *drv = phl_to_drvpriv(phl_info);
	_os_spinlockfg sp_flags;


	_os_spinlock(drv, &pool_list->lock, _irq, &sp_flags);
	list_add_tail(&rxbuf->list, &pool_list->queue);
	pool_list->cnt++;
	_os_spinunlock(drv, &pool_list->lock, _irq, &sp_flags);
}

static struct rtw_rx_buf *dequeue_rxbuf(struct phl_info_t *phl_info,
					struct phl_queue *pool_list)
{
	struct rtw_rx_buf *rxbuf = NULL;
	void *drv = phl_to_drvpriv(phl_info);
	_os_spinlockfg sp_flags;


	_os_spinlock(drv, &pool_list->lock, _irq, &sp_flags);
	if (!list_empty(&pool_list->queue)) {
		rxbuf = list_first_entry(&pool_list->queue, struct rtw_rx_buf,
					 list);
		list_del(&rxbuf->list);
		pool_list->cnt--;
	}
	_os_spinunlock(drv, &pool_list->lock, _irq, &sp_flags);

	return rxbuf;
}

/*
 * Return RTW_PHL_STATUS_RESOURCE if space in txbuf is not enough for tx_req,
 * or RTW_PHL_STATUS_SUCCESS for tx_req has already been handled and recycled.
 */
static enum rtw_phl_status _phl_prepare_tx_sdio(struct phl_info_t *phl_info,
					        struct rtw_xmit_req *tx_req,
					        struct rtw_tx_buf *txbuf)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;
	enum rtw_hal_status hstatus;
	void *drv_priv = phl_to_drvpriv(phl_info);
	struct rtw_phl_evt_ops *ops = &phl_info->phl_com->evt_ops;
	u32 align_size = 8;
	u32 wd_len = 48; /* default set to max 48 bytes */
	u32 used_len;
	u32 total_len;
	u8 *tx_buf_data;
	struct rtw_pkt_buf_list *pkt_buf = NULL;
	u8 i = 0;


	if (txbuf->agg_cnt == PHL_SDIO_TX_AGG_MAX)
		return RTW_PHL_STATUS_RESOURCE;

	used_len = _ALIGN(txbuf->used_len, align_size);
	total_len = used_len + wd_len + tx_req->total_len;
	if (total_len > txbuf->buf_len) {
		if (txbuf->agg_cnt)
			return RTW_PHL_STATUS_RESOURCE;

		PHL_ERR(
				"%s: unexpected tx size(%d + %d)!!\n",
			  __FUNCTION__, tx_req->total_len, wd_len);
		/* drop, skip this packet */
		goto recycle;
	}

	tx_buf_data = txbuf->buffer + used_len;	/* align start address */

	hstatus = rtw_hal_fill_wd(phl_info->hal, tx_req, tx_buf_data, &wd_len);
	if (hstatus != RTW_HAL_STATUS_SUCCESS) {
		PHL_ERR(
			  "%s: Fail to fill txdesc!(0x%x)\n",
			  __FUNCTION__, hstatus);
		/* drop, skip this packet */
		goto recycle;
	}

	tx_buf_data += wd_len;

	pkt_buf = (struct rtw_pkt_buf_list *)tx_req->pkt_list;
	for (i = 0; i < tx_req->pkt_cnt; i++, pkt_buf++) {
		_os_mem_cpy(drv_priv, tx_buf_data,
			    pkt_buf->vir_addr, pkt_buf->length);
		tx_buf_data += pkt_buf->length;
	}

	txbuf->used_len = used_len + wd_len + tx_req->total_len;
	txbuf->pkt_len[txbuf->agg_cnt] = tx_req->mdata.pktlen;
	txbuf->wp_offset[txbuf->agg_cnt] = tx_req->mdata.wp_offset;
	txbuf->agg_cnt++;
	if (txbuf->agg_cnt == 1) {
		txbuf->dma_ch = tx_req->mdata.dma_ch;
	} else {
		/* update first packet's txagg_num of wd */
		txbuf->buffer[5] = txbuf->agg_cnt;
		/* Todo: update checksum field */
	}

recycle:
	if (RTW_PHL_TREQ_TYPE_TEST_PATTERN == tx_req->treq_type) {
		if (ops->tx_test_recycle) {
			pstatus = ops->tx_test_recycle(phl_info, tx_req);
			if (pstatus != RTW_PHL_STATUS_SUCCESS) {
				PHL_ERR(
					  "%s: tx_test_recycle fail!! (%d)\n",
					  __FUNCTION__, pstatus);
			}
		}
	} else {
		/* RTW_PHL_TREQ_TYPE_NORMAL == tx_req->treq_type */
		if (ops->tx_recycle) {
			pstatus = ops->tx_recycle(drv_priv, tx_req);
			if (pstatus != RTW_PHL_STATUS_SUCCESS)
				PHL_ERR(
					  "%s: tx recycle fail!! (%d)\n",
					  __FUNCTION__, pstatus);
		}
	}

	return RTW_PHL_STATUS_SUCCESS;
}

static void _phl_tx_flow_ctrl_sdio(struct phl_info_t *phl_info,
						      _os_list *sta_list)
{
	phl_tx_flow_ctrl(phl_info, sta_list);
}

static enum rtw_phl_status phl_handle_xmit_ring_sdio(
					struct phl_info_t *phl_info,
					struct phl_ring_status *ring_sts)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;
	struct rtw_phl_tx_ring *tring = ring_sts->ring_ptr;
	struct rtw_xmit_req *tx_req = NULL;
	u16 rptr = 0;
	void *drv_priv = phl_to_drvpriv(phl_info);
#ifdef SDIO_TX_THREAD
	struct rtw_tx_buf_ring *tx_pool = (struct rtw_tx_buf_ring *)phl_info->hci->txbuf_pool;
	struct rtw_tx_buf *txbuf = NULL;
#else
	struct rtw_tx_buf *txbuf = (struct rtw_tx_buf *)phl_info->hci->txbuf_pool;
#endif


	do {
		rptr = (u16)_os_atomic_read(drv_priv, &tring->phl_next_idx);

		tx_req = (struct rtw_xmit_req *)tring->entry[rptr];
		if (!tx_req) {
			PHL_ERR(
				  "%s: tx_req is NULL!\n", __FUNCTION__);
			pstatus = RTW_PHL_STATUS_FAILURE;
			break;
		}
		tx_req->mdata.band = ring_sts->band;
		tx_req->mdata.wmm = ring_sts->wmm;
		tx_req->mdata.hal_port = ring_sts->port;
		/*tx_req->mdata.mbssid = ring_sts->mbssid;*/
		tx_req->mdata.dma_ch = tring->dma_ch;
		tx_req->mdata.pktlen = (u16)tx_req->total_len;

#ifdef SDIO_TX_THREAD
get_txbuf:
		if (!txbuf) {
			txbuf = alloc_txbuf(phl_info, tx_pool, tring->cat);
			if (!txbuf) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				break;
			}
		}
#endif /* SDIO_TX_THREAD */
		pstatus = _phl_prepare_tx_sdio(phl_info, tx_req, txbuf);
#ifdef SDIO_TX_THREAD
		if (pstatus == RTW_PHL_STATUS_RESOURCE) {
			/* enqueue txbuf */
			enqueue_busy_txbuf(phl_info, tx_pool, txbuf);
			txbuf = NULL;
			goto get_txbuf;
		}
		if (pstatus != RTW_PHL_STATUS_SUCCESS) {
			/* impossible case, never entered for now */
			PHL_ERR(
				  "%s: prepare tx fail!(%d)\n",
				  __FUNCTION__, pstatus);
			break;
		}
#else /* !SDIO_TX_THREAD */
		if (pstatus != RTW_PHL_STATUS_SUCCESS) {
			if (pstatus == RTW_PHL_STATUS_RESOURCE) {
				pstatus = RTW_PHL_STATUS_SUCCESS;
			} else {
				PHL_ERR(
					  "%s: prepare tx fail!(%d)\n",
					  __FUNCTION__, pstatus);
			}
			break;
		}
#endif /* !SDIO_TX_THREAD */

		_os_atomic_set(drv_priv, &tring->phl_idx, rptr);

		ring_sts->req_busy--;
		/* TODO: aggregate more packets! */
		if (!ring_sts->req_busy)
			break;

		if ((rptr + 1) >= MAX_PHL_TX_RING_ENTRY_NUM)
			_os_atomic_set(drv_priv, &tring->phl_next_idx, 0);
		else
			_os_atomic_inc(drv_priv, &tring->phl_next_idx);
	} while (1);

#ifdef SDIO_TX_THREAD
	if (txbuf)
		enqueue_busy_txbuf(phl_info, tx_pool, txbuf);
#endif /* SDIO_TX_THREAD */
	phl_release_ring_sts(phl_info, ring_sts);

	return pstatus;
}

#ifdef SDIO_TX_THREAD
static int phl_tx_sdio_thrd_hdl(void *context)
{
	struct phl_info_t *phl = context;
	struct hci_info_t *hci = phl->hci;
	struct rtw_tx_buf_ring *tx_pool;
	struct rtw_tx_buf *txbuf = NULL;
	void *drv = phl_to_drvpriv(phl);
	enum rtw_hal_status hstatus;
	enum phl_tx_status tx_status;

	#ifdef RTW_XMIT_THREAD_HIGH_PRIORITY
	#ifdef PLATFORM_LINUX
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
		sched_set_fifo_low(current);
	#else
		struct sched_param param = { .sched_priority = 1 };

		sched_setscheduler(current, SCHED_FIFO, &param);
	#endif
	#endif /* PLATFORM_LINUX */
	#endif /*RTW_XMIT_THREAD_HIGH_PRIORITY*/



	PHL_INFO("SDIO: tx thread start\n");

	tx_pool = (struct rtw_tx_buf_ring *)hci->txbuf_pool;
	while (1) {
		_os_sema_down(drv, &hci->tx_thrd_sema);

check_stop:
		if (_os_thread_check_stop(drv, &hci->tx_thrd))
			break;

		tx_status = _os_atomic_read(drv, &phl->phl_sw_tx_sts);
		if (tx_status == PHL_TX_STATUS_STOP_INPROGRESS){
			_os_atomic_set(drv, &phl->phl_sw_tx_sts, PHL_TX_STATUS_SW_PAUSE);
			continue;
		}

		if (phl_datapath_chk_trx_pause(phl, PHL_CTRL_TX))
			continue;

		txbuf = dequeue_busy_txbuf(phl, tx_pool);
		if (!txbuf)
			continue;

		hstatus = rtw_hal_sdio_tx(phl->hal, txbuf->dma_ch, txbuf->buffer,
					  txbuf->used_len, txbuf->agg_cnt,
					  txbuf->pkt_len, txbuf->wp_offset);
		if (hstatus != RTW_HAL_STATUS_SUCCESS) {
			bool overflow;

			if ((hstatus == RTW_HAL_STATUS_RESOURCE)
			    && (txbuf->retry < XMIT_BUFFER_RETRY_LIMIT)) {
				txbuf->retry++;
				enqueue_busy_txbuf_to_head(phl, tx_pool, txbuf);
				/* Todo: What to do when TX FIFO not ready? */
				goto check_stop;
			}

			/* Keep overflow bit(bit31) */
			overflow = (hci->tx_drop_cnt & BIT31) ? true : false;
			hci->tx_drop_cnt++;
			if (overflow)
				hci->tx_drop_cnt |= BIT31;

			/* Show msg on 2^n times */
			if (!(hci->tx_drop_cnt & (hci->tx_drop_cnt - 1))) {
				PHL_ERR(
					  "%s: drop!(%d) type=%u mgnt=%u len=%u "
					  "agg_cnt=%u drop_cnt=%u%s\n",
					  __FUNCTION__, hstatus, txbuf->tag, txbuf->mgnt_pkt,
					  txbuf->used_len, txbuf->agg_cnt,
					  hci->tx_drop_cnt & ~BIT31,
					  (hci->tx_drop_cnt & BIT31) ? "(overflow)" : "");
			}
		}
		free_txbuf(phl, tx_pool, txbuf);
		/* if agg thread is waiting tx buffer, notify it */
#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
#ifndef RTW_WKARD_SDIO_TX_USE_YIELD
		txbuf_set_ready(phl);
#endif /* !RTW_WKARD_SDIO_TX_USE_YIELD */
#else /* CONFIG_PHL_SDIO_TX_CB_THREAD */
		phl_schedule_handler(phl->phl_com, &phl->phl_tx_handler);
#endif /* CONFIG_PHL_SDIO_TX_CB_THREAD */

		/* check stop and if more txbuf for tx */
		goto check_stop;
	}

	_os_thread_wait_stop(drv, &hci->tx_thrd);
	PHL_INFO("SDIO: tx thread down\n");

	return 0;
}
#else /* !SDIO_TX_THREAD */
static enum rtw_phl_status _phl_tx_sdio(struct phl_info_t *phl_info)
{
	enum rtw_hal_status hstatus;
	struct hci_info_t *hci = phl_info->hci;
	struct rtw_tx_buf *txbuf = (struct rtw_tx_buf*)hci->txbuf_pool;


	if (!txbuf->buffer || !txbuf->buf_len)
		return RTW_PHL_STATUS_SUCCESS;

	hstatus = rtw_hal_sdio_tx(phl_info->hal, txbuf->dma_ch, txbuf->buffer,
				  txbuf->used_len, txbuf->agg_cnt,
				  txbuf->pkt_len, txbuf->wp_offset);
	txbuf->used_len = 0;
	txbuf->agg_cnt = 0;
	if (hstatus == RTW_HAL_STATUS_SUCCESS)
		return RTW_PHL_STATUS_SUCCESS;

	return RTW_PHL_STATUS_FAILURE;
}
#endif /* !SDIO_TX_THREAD */

static enum rtw_phl_status phl_tx_check_status_sdio(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;
	enum phl_tx_status tx_status = _os_atomic_read(drv, &phl_info->phl_sw_tx_sts);

	if (tx_status == PHL_TX_STATUS_STOP_INPROGRESS) {
		#ifdef SDIO_TX_THREAD
		phl_tx_sdio_wake_thrd(phl_info);
		#else
		_os_atomic_set(drv, &phl_info->phl_sw_tx_sts, PHL_TX_STATUS_SW_PAUSE);
		#endif
		pstatus = RTW_PHL_STATUS_FAILURE;
	}
	return pstatus;
}

static void phl_tx_callback_sdio(void *context)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_phl_handler *phl_handler;
	struct phl_info_t *phl_info;
	struct phl_ring_status *ring_sts = NULL, *t;
	_os_list sta_list;
	bool tx_pause = false;
#ifdef SDIO_TX_THREAD
	bool rsrc;
#endif /* SDIO_TX_THREAD */

	#ifdef RTW_XMIT_THREAD_CB_HIGH_PRIORITY
	#ifdef PLATFORM_LINUX
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	sched_set_fifo_low(current);
	#else
		struct sched_param param = { .sched_priority = 1 };

		sched_setscheduler(current, SCHED_FIFO, &param);
	#endif
	#endif /* PLATFORM_LINUX */
	#endif /* RTW_XMIT_THREAD_CB_HIGH_PRIORITY */

	phl_handler = (struct rtw_phl_handler *)phl_container_of(context,
						     struct rtw_phl_handler,
						     os_handler);
	phl_info = (struct phl_info_t *)phl_handler->context;

	INIT_LIST_HEAD(&sta_list);

	pstatus = phl_tx_check_status_sdio(phl_info);
	if (pstatus == RTW_PHL_STATUS_FAILURE)
		goto end;

	/* check datapath sw state */
	tx_pause = phl_datapath_chk_trx_pause(phl_info, PHL_CTRL_TX);
	if (true == tx_pause)
		goto end;

#ifdef CONFIG_POWER_SAVE
	/* check ps state when tx is not paused */
	if (false == phl_ps_is_datapath_allowed(phl_info)) {
		PHL_WARN("%s(): datapath is not allowed now... may in low power.\n", __func__);
		goto end;
	}
#endif
	do {
		if (!phl_check_xmit_ring_resource(phl_info, &sta_list))
			break;

		/* phl_info->t_fctrl_result would be filled inside phl_tx_flow_ctrl() */
		phl_tx_flow_ctrl(phl_info, &sta_list);

#ifdef SDIO_TX_THREAD
		rsrc = false;	/* default suppose no enough tx resource */
#endif /* SDIO_TX_THREAD */
		phl_list_for_loop_safe(ring_sts, t, struct phl_ring_status,
				       &phl_info->t_fctrl_result, list) {
			list_del(&ring_sts->list);

			/* ring_sts would be release inside phl_handle_xmit_ring_sdio() */
			pstatus = phl_handle_xmit_ring_sdio(phl_info, ring_sts);

#ifdef SDIO_TX_THREAD
			if (pstatus != RTW_PHL_STATUS_RESOURCE)
				rsrc = true; /* some tx data has been sent */
#else /* !SDIO_TX_THREAD */
			pstatus = _phl_tx_sdio(phl_info);
			if (pstatus != RTW_PHL_STATUS_SUCCESS) {
				u32 drop = phl_info->hci->tx_drop_cnt;

				/* Keep overflow bit(bit31) */
				phl_info->hci->tx_drop_cnt = (drop & BIT31)
							    | (drop + 1);
				drop = phl_info->hci->tx_drop_cnt;
				/* Show msg on 2^n times */
				if (!(drop & (drop - 1))) {
					PHL_ERR(
						  "%s: phl_tx fail!(%d) drop cnt=%u%s\n",
						  __FUNCTION__, pstatus,
						  drop & ~BIT31,
						  drop & BIT31 ? "(overflow)" : "");
				}
			}
#endif /* !SDIO_TX_THREAD */
		}

		pstatus = phl_tx_check_status_sdio(phl_info);
		if (pstatus == RTW_PHL_STATUS_FAILURE)
			break;

		phl_free_deferred_tx_ring(phl_info);

#ifdef SDIO_TX_THREAD
		/*
		 * Break loop when no txbuf for tx data and this function would
		 * be schedule again when I/O thread return txbuf.
		 */
		if (!rsrc) {
#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
#ifdef RTW_WKARD_SDIO_TX_USE_YIELD
			_os_yield(phl_to_drvpriv(phl_info));
#else /* !RTW_WKARD_SDIO_TX_USE_YIELD */
			txbuf_wait(phl_info, 1);
#endif /* !RTW_WKARD_SDIO_TX_USE_YIELD */
#else /* !CONFIG_PHL_SDIO_TX_CB_THREAD */
			break;
#endif /* !CONFIG_PHL_SDIO_TX_CB_THREAD */
		}
#endif /* SDIO_TX_THREAD */
	} while (1);

end:
	phl_free_deferred_tx_ring(phl_info);
}

static enum rtw_phl_status phl_prepare_tx_sdio(struct phl_info_t *phl_info,
					       struct rtw_xmit_req *tx_req)
{
	/* not implement yet */
	return RTW_PHL_STATUS_FAILURE;
}

static enum rtw_phl_status phl_tx_sdio(struct phl_info_t *phl_info)
{
	/* not implement yet */
	return RTW_PHL_STATUS_FAILURE;
}

static enum rtw_phl_status phl_recycle_rx_buf_sdio(struct phl_info_t *phl,
						   void *r, u8 ch,
						   enum rtw_rx_type type)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl->hci;
	struct rtw_rx_buf_ring *rx_pool = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;
	struct rtw_rx_buf *rxbuf = r;
	void *drv_priv = phl_to_drvpriv(phl);

#if 1
	/* Just for debugging, could consider to disable in the future */
	if (!_os_atomic_read(drv_priv, &rxbuf->ref)) {
		PHL_TRACE(COMP_PHL_RECV, _PHL_ERR_,
			  "%s: ref count error! (%px)\n", __FUNCTION__, rxbuf);
		_os_warn_on(1);
		return RTW_PHL_STATUS_SUCCESS;
	}
#endif

	if (!_os_atomic_dec_return(drv_priv, &rxbuf->ref))
		enqueue_rxbuf(phl, &rx_pool->idle_rxbuf_list, rxbuf);

	return RTW_PHL_STATUS_SUCCESS;
}
void phl_rx_handle_normal(struct phl_info_t *phl,
					 struct rtw_phl_rx_pkt *phl_rx)
{
 	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	_os_list frames;
	FUNCIN_WSTS(pstatus);

	do {
		INIT_LIST_HEAD(&frames);

		if (phl_rx->r.mdata.rx_rate <= RTW_DATA_RATE_HE_NSS4_MCS11)
			phl->phl_com->phl_stats.rx_rate_nmr[phl_rx->r.mdata.rx_rate]++;

		pstatus = phl_rx_reorder(phl, phl_rx, &frames);

		if (pstatus != RTW_PHL_STATUS_SUCCESS) {
			PHL_TRACE(COMP_PHL_RECV, _PHL_ERR_,
					"%s: phl_rx_reorder"
					" FAIL! (%d)\n",
					__FUNCTION__, pstatus);
			break;
		}
		phl_handle_rx_frame_list(phl, &frames);
	} while (0);

	FUNCOUT_WSTS(pstatus);

}

static enum rtw_phl_status phl_rx_sdio(struct phl_info_t *phl)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;
	enum rtw_hal_status hstatus;
	struct hci_info_t *hci_info = (struct hci_info_t *)phl->hci;
	struct rtw_phl_rx_pkt *phl_rx = NULL;
	struct rtw_rx_buf_ring *rx_pool = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;
	struct rtw_rx_buf *rxbuf = NULL;
	void *drv_priv = phl_to_drvpriv(phl);
#ifndef CONFIG_PHL_RX_PSTS_PER_PKT
	_os_list frames;
#endif
	u32 len;
	bool flag = true;
	u8 i;
#ifdef CONFIG_PHL_SDIO_RX_NETBUF_ALLOC_IN_PHL
	u8 mfrag = 0, frag_num = 0;
	u16 netbuf_len = 0;
#endif


	do {
		if (rxbuf) {
			len = rtw_hal_sdio_parse_rx(phl->hal, rxbuf);
			if (!len) {
				if (!_os_atomic_dec_return(drv_priv, &rxbuf->ref))
					enqueue_rxbuf(phl, &rx_pool->idle_rxbuf_list, rxbuf);
				rxbuf = NULL;
				continue;
			}
		} else {
			rxbuf = dequeue_rxbuf(phl, &rx_pool->pend_rxbuf_list);
			if (!rxbuf) {
#ifdef CONFIG_PHL_SDIO_READ_RXFF_IN_INT
				rxbuf = dequeue_rxbuf(phl, &rx_pool->busy_rxbuf_list);
				if (!rxbuf)
					break;

				len = rtw_hal_sdio_parse_rx(phl->hal, rxbuf);
				if (!len) {
					enqueue_rxbuf(phl, &rx_pool->idle_rxbuf_list, rxbuf);
					break;
				}
#else
				rxbuf = dequeue_rxbuf(phl, &rx_pool->idle_rxbuf_list);
				if (!rxbuf) {
					pstatus = RTW_PHL_STATUS_RESOURCE;
					break;
				}

				len = rtw_hal_sdio_rx(phl->hal, rxbuf);
				if (!len) {
					enqueue_rxbuf(phl, &rx_pool->idle_rxbuf_list, rxbuf);
					break;
				}
#endif
				_os_atomic_set(drv_priv, &rxbuf->ref, 1);
			}
		}

		for (i = rxbuf->agg_start; i < rxbuf->agg_cnt; i++) {
			phl_rx = rtw_phl_query_phl_rx(phl);
			if (!phl_rx) {
				pstatus = RTW_PHL_STATUS_RESOURCE;
				/* No enough resource to handle rx data, */
				/* so maybe take a break */
				rxbuf->agg_start = i;
				enqueue_rxbuf(phl, &rx_pool->pend_rxbuf_list, rxbuf);
				rxbuf = NULL;
				flag = false;

#ifdef PHL_RX_BATCH_IND
				_phl_indic_new_rxpkt(phl);
#endif

				break;
			}

			phl_rx->type = rxbuf->pkt[i].meta.rpkt_type;
			phl_rx->rxbuf_ptr = (u8*)rxbuf;
			_os_atomic_inc(drv_priv, &rxbuf->ref);
			phl_rx->r.os_priv = NULL;
			_os_mem_cpy(phl->phl_com->drv_priv,
				    &phl_rx->r.mdata, &rxbuf->pkt[i].meta,
				    sizeof(struct rtw_r_meta_data));
			/*phl_rx->r.shortcut_id;*/
			phl_rx->r.pkt_cnt = 1;
			phl_rx->r.pkt_list->vir_addr = rxbuf->pkt[i].pkt;
			phl_rx->r.pkt_list->length = rxbuf->pkt[i].pkt_len;

			/* length include WD and packet size */
			len = ((u32)(rxbuf->pkt[i].pkt - rxbuf->pkt[i].wd))
			      + rxbuf->pkt[i].pkt_len;
			hstatus = rtw_hal_handle_rx_buffer(phl->phl_com,
							   phl->hal,
							   rxbuf->pkt[i].wd,
							   len, phl_rx);
			if (hstatus != RTW_HAL_STATUS_SUCCESS) {
				PHL_TRACE(COMP_PHL_RECV, _PHL_ERR_,
					  "%s: hal_handle_rx FAIL! (%d)"
					  " type=0x%X len=%u\n",
					  __FUNCTION__, hstatus,
					  phl_rx->type, len);
			}

#ifdef CONFIG_PHL_DUMP_TRX_STATUS
			if (phl->phl_com->check_rx_int == 1)
				PHL_TRACE(COMP_PHL_RECV, _PHL_ALWAYS_,
					  "%s type(0x%x)\n", __func__, phl_rx->type);
#endif /*CONFIG_PHL_DUMP_TRX_STATUS*/

			switch (phl_rx->type) {
			case RTW_RX_TYPE_WIFI:

#ifdef CONFIG_PHL_SDIO_RX_NETBUF_ALLOC_IN_PHL
				{
					u8 *netbuf = NULL;
					void *drv = phl_to_drvpriv(phl);

					/* Pre-alloc netbuf and replace pkt_list[0].vir_addr */

					/* For first fragment packet, driver need allocate 1536 to defrag packet.*/
					mfrag = PHL_GET_80211_HDR_MORE_FRAG(phl_rx->r.pkt_list[0].vir_addr);
					frag_num = PHL_GET_80211_HDR_FRAG_NUM(phl_rx->r.pkt_list[0].vir_addr);

					if (mfrag == 1 && frag_num == 0) {
						if (phl_rx->r.pkt_list[0].length < RTW_MAX_ETH_PKT_LEN)
							netbuf_len = RTW_MAX_ETH_PKT_LEN;
						else
							netbuf_len = phl_rx->r.pkt_list[0].length;
					} else {
						netbuf_len = phl_rx->r.pkt_list[0].length;
					}

					netbuf = _os_alloc_netbuf(drv,
								netbuf_len,
								&(phl_rx->r.os_priv));

					if (netbuf) {
						_os_mem_cpy(drv, netbuf,
							phl_rx->r.pkt_list[0].vir_addr, phl_rx->r.pkt_list[0].length);
						phl_rx->r.pkt_list[0].vir_addr = netbuf;

						phl_rx->r.os_netbuf_len = netbuf_len;
						phl_rx->rxbuf_ptr = NULL;
						_os_atomic_dec(drv_priv, &rxbuf->ref);
					}
				}
#endif

#ifdef CONFIG_PHL_RX_PSTS_PER_PKT
				if (false == phl_rx_proc_wait_phy_sts(phl, phl_rx)) {
					PHL_TRACE(COMP_PHL_PSTS, _PHL_DEBUG_,
						  "phl_rx_proc_wait_phy_sts() return false \n");
					phl_rx_handle_normal(phl, phl_rx);
				} else {
					pstatus = RTW_PHL_STATUS_SUCCESS;
				}
				/*
				 * phl_rx already has been took over by
				 * phl_rx_reorder(), so clear it here.
				 */
				phl_rx = NULL;
#else
				INIT_LIST_HEAD(&frames);
				pstatus = phl_rx_reorder(phl, phl_rx, &frames);
				/*
				 * phl_rx already has been took over by
				 * phl_rx_reorder(), so clear it here.
				 */
				phl_rx = NULL;
				if (pstatus != RTW_PHL_STATUS_SUCCESS) {
					PHL_TRACE(COMP_PHL_RECV, _PHL_ERR_,
						  "%s: phl_rx_reorder"
						  " FAIL! (%d)\n",
						  __FUNCTION__, pstatus);
					break;
				}
				phl_handle_rx_frame_list(phl, &frames);

#endif
				break;

			case RTW_RX_TYPE_PPDU_STATUS:
				phl_rx_proc_ppdu_sts(phl, phl_rx);
#ifdef CONFIG_PHL_RX_PSTS_PER_PKT
				phl_rx_proc_phy_sts(phl, phl_rx);
#endif
				break;
#ifdef CONFIG_PHL_RELEASE_RPT_ENABLE
			case RTW_RX_TYPE_TX_WP_RELEASE_HOST:
				PHL_TRACE(COMP_PHL_RECV, _PHL_WARNING_,
					  "%s: RTW_RX_TYPE_TX_WP_RELEASE_HOST not support!\n", __FUNCTION__);
				break;
#endif
			default:
				break;
			}

			if (phl_rx) {
				_os_atomic_dec(drv_priv, &rxbuf->ref);
				phl_release_phl_rx(phl, phl_rx);
				phl_rx = NULL;
			}
		}

		if (rxbuf) {
			if (rxbuf->next_ptr) {
				rxbuf->len -= (u32)(rxbuf->next_ptr - rxbuf->ptr);
				rxbuf->ptr = rxbuf->next_ptr;
				rxbuf->next_ptr = NULL;
				continue;
			}
			if (!_os_atomic_dec_return(drv_priv, &rxbuf->ref))
				enqueue_rxbuf(phl, &rx_pool->idle_rxbuf_list, rxbuf);
			rxbuf = NULL;

#ifdef PHL_RX_BATCH_IND
			if (phl->rx_new_pending)
				_phl_indic_new_rxpkt(phl);
#endif
		}
#ifdef CONFIG_PHL_SDIO_READ_RXFF_IN_INT
		/** When defining CONFIG_PHL_SDIO_READ_RXFF_IN_INT,
		* checking in function::phl_recv_rxfifo_sdio.
		*/
#else
		if (rtw_hal_sdio_lps_flg(phl->hal) == RTW_HAL_LPS_FLG_STATE_LPS)
			break;
#endif
	} while (flag);

	return pstatus;
}

static void phl_rx_stop_sdio(struct phl_info_t *phl)
{
	void *drv = phl_to_drvpriv(phl);

	_os_atomic_set(drv, &phl->phl_sw_rx_sts, PHL_RX_STATUS_SW_PAUSE);
}

static void phl_rx_callback_sdio(void *context)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct rtw_phl_handler *phl_handler;
	struct phl_info_t *phl_info;
	bool rx_pause = false;

	#ifdef RTW_RECV_THREAD_HIGH_PRIORITY
	#ifdef PLATFORM_LINUX
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
	sched_set_fifo_low(current);
	#else
	struct sched_param param = { .sched_priority = 1 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	#endif
	#endif /* PLATFORM_LINUX */
	#endif /*RTW_RECV_THREAD_HIGH_PRIORITY*/

	phl_handler = (struct rtw_phl_handler *)phl_container_of(context,
						     struct rtw_phl_handler,
						     os_handler);
	phl_info = (struct phl_info_t *)phl_handler->context;

	/* check datapath sw state */
	rx_pause = phl_datapath_chk_trx_pause(phl_info, PHL_CTRL_RX);
	if (true == rx_pause)
		goto end;

	if (false == phl_check_recv_ring_resource(phl_info))
		goto chk_stop;

	pstatus = phl_rx_sdio(phl_info);
	if (pstatus == RTW_PHL_STATUS_RESOURCE) {
		PHL_TRACE(COMP_PHL_RECV, _PHL_WARNING_,
			  "%s: resource starvation!\n", __FUNCTION__);
	} else if (pstatus != RTW_PHL_STATUS_SUCCESS) {
		PHL_TRACE(COMP_PHL_RECV, _PHL_ERR_,
			  "%s: phl_rx fail!(%d)\n", __FUNCTION__, pstatus);
	}

chk_stop:
	if (PHL_RX_STATUS_STOP_INPROGRESS ==
	    _os_atomic_read(phl_to_drvpriv(phl_info), &phl_info->phl_sw_rx_sts))
		phl_rx_stop_sdio(phl_info);
end:
	/* enable rx interrupt*/
	rtw_hal_config_interrupt(phl_info->hal , RTW_PHL_RESUME_RX_INT);
}

static enum rtw_phl_status phl_register_trx_hdlr_sdio(struct phl_info_t *phl)
{
	struct rtw_phl_handler *tx_handler = &phl->phl_tx_handler;
	struct rtw_phl_handler *rx_handler = &phl->phl_rx_handler;
	void *drv = phl_to_drvpriv(phl);
#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
	const char *tx_hdl_cb_name = "RTW_TX_CB_THREAD";
#endif

#ifdef CONFIG_PHL_SDIO_RX_CB_THREAD
	const char *rx_hdl_cb_name = "RTW_RX_CB_THREAD";
#endif

	enum rtw_phl_status pstatus;


#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
	tx_handler->type = RTW_PHL_HANDLER_PRIO_NORMAL;
	_os_strncpy(tx_handler->cb_name, tx_hdl_cb_name,
		    (_os_strlen((u8*)tx_hdl_cb_name) > RTW_PHL_HANDLER_CB_NAME_LEN) ?
			RTW_PHL_HANDLER_CB_NAME_LEN : _os_strlen((u8*)tx_hdl_cb_name));
	#ifdef CONFIG_PHL_CPU_BALANCE_THREAD
	tx_handler->os_handler.u.thread.en_assign_cpuid = _TRUE;
	tx_handler->os_handler.u.thread.cpu_id = CPU_ID_TX_CB;
	#endif /*CONFIG_PHL_CPU_BALANCE_THREAD*/
#else
	tx_handler->type = RTW_PHL_HANDLER_PRIO_LOW;
#endif
	tx_handler->callback = phl_tx_callback_sdio;
	tx_handler->context = phl;
	tx_handler->drv_priv = drv;
	pstatus = phl_register_handler(phl->phl_com, tx_handler);
	if (pstatus != RTW_PHL_STATUS_SUCCESS)
		return pstatus;

#ifdef CONFIG_PHL_SDIO_RX_CB_THREAD
	rx_handler->type = RTW_PHL_HANDLER_PRIO_NORMAL;
	_os_strncpy(rx_handler->cb_name, rx_hdl_cb_name,
		   (_os_strlen((u8*)rx_hdl_cb_name) > RTW_PHL_HANDLER_CB_NAME_LEN) ?
			RTW_PHL_HANDLER_CB_NAME_LEN : _os_strlen((u8*)rx_hdl_cb_name));
	#ifdef CONFIG_PHL_CPU_BALANCE_THREAD
	rx_handler->os_handler.u.thread.en_assign_cpuid= _TRUE;
	rx_handler->os_handler.u.thread.cpu_id = CPU_ID_RX_CB;
	#endif /*CONFIG_PHL_CPU_BALANCE_THREAD*/
#else
	rx_handler->type = RTW_PHL_HANDLER_PRIO_LOW;
#endif
	rx_handler->callback = phl_rx_callback_sdio;
	rx_handler->context = phl;
	rx_handler->drv_priv = drv;
	pstatus = phl_register_handler(phl->phl_com, rx_handler);
	if (pstatus != RTW_PHL_STATUS_SUCCESS)
		return pstatus;

	return RTW_PHL_STATUS_SUCCESS;
}

#ifdef CONFIG_PHL_SDIO_READ_RXFF_IN_INT
static enum rtw_phl_status phl_recv_rxfifo_sdio(struct phl_info_t *phl)
{
	struct hci_info_t *hci_info = (struct hci_info_t *)phl->hci;
	struct rtw_rx_buf_ring *rx_pool = (struct rtw_rx_buf_ring *)hci_info->rxbuf_pool;
	struct rtw_rx_buf *rxbuf = NULL;
	u32 len;
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_SUCCESS;

	do {
		rxbuf = dequeue_rxbuf(phl, &rx_pool->idle_rxbuf_list);
		if (!rxbuf) {
			pstatus = RTW_PHL_STATUS_RESOURCE;
			break;
		}

		len = rtw_hal_sdio_rx(phl->hal, rxbuf);
		if (!len) {
			enqueue_rxbuf(phl, &rx_pool->idle_rxbuf_list, rxbuf);
			break;
		}

		enqueue_rxbuf(phl, &rx_pool->busy_rxbuf_list, rxbuf);
		rtw_phl_start_rx_process(phl);

		if (rtw_hal_sdio_lps_flg(phl->hal) == RTW_HAL_LPS_FLG_STATE_LPS)
			break;
	} while (1);

	return pstatus;
}
#endif

static void phl_trx_deinit_sdio(struct phl_info_t *phl_info)
{
	struct hci_info_t *hci = phl_info->hci;
	void *drv = phl_to_drvpriv(phl_info);
	struct rtw_rx_buf_ring *rx_pool;
	struct rtw_rx_buf *rxbuf;
	struct rtw_tx_buf *txbuf;
#ifdef SDIO_TX_THREAD
	struct rtw_tx_buf_ring *tx_pool;
	u32 i;
#endif


	FUNCIN();

	/* TODO: stop RX callback */
	/* TODO: stop TX callback */

	/* freee RX resource */
	if (hci->rxbuf_pool) {
		rx_pool = (struct rtw_rx_buf_ring *)hci->rxbuf_pool;

		while (rx_pool->idle_rxbuf_list.cnt) {
			rxbuf = dequeue_rxbuf(phl_info, &rx_pool->idle_rxbuf_list);
			if (!rxbuf)
				break;
			_os_kmem_free(drv, rxbuf->buffer, rxbuf->buf_len);
		}

		while (rx_pool->pend_rxbuf_list.cnt) {
			rxbuf = dequeue_rxbuf(phl_info, &rx_pool->pend_rxbuf_list);
			if (!rxbuf)
				break;
			_os_kmem_free(drv, rxbuf->buffer, rxbuf->buf_len);
		}

		while (rx_pool->busy_rxbuf_list.cnt) {
			rxbuf = dequeue_rxbuf(phl_info, &rx_pool->busy_rxbuf_list);
			if (!rxbuf)
				break;
			_os_kmem_free(drv, rxbuf->buffer, rxbuf->buf_len);
		}

		pq_deinit(drv, &rx_pool->idle_rxbuf_list);
		pq_deinit(drv, &rx_pool->busy_rxbuf_list);
		pq_deinit(drv, &rx_pool->pend_rxbuf_list);

		_os_mem_free(drv, rx_pool->rxbufblock,
			     rx_pool->total_blocks_size);
		_os_mem_free(drv, hci->rxbuf_pool,
			     sizeof(struct rtw_rx_buf_ring));
		hci->rxbuf_pool = NULL;
	}

	/* freee TX resource */
#ifdef SDIO_TX_THREAD
	_os_thread_stop(drv, &hci->tx_thrd);
	phl_tx_sdio_wake_thrd(phl_info);
	_os_thread_deinit(drv, &hci->tx_thrd);

#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
#ifndef RTW_WKARD_SDIO_TX_USE_YIELD
	_os_spinlock_free(drv, &hci->tx_buf_lock);
#endif /* !RTW_WKARD_SDIO_TX_USE_YIELD */
#endif /* CONFIG_PHL_SDIO_TX_CB_THREAD */

	if (hci->txbuf_pool) {
		tx_pool = (struct rtw_tx_buf_ring *)hci->txbuf_pool;
		hci->txbuf_pool = NULL;

		txbuf = (struct rtw_tx_buf *)tx_pool->txbufblock;
		for (i = 0; i < tx_pool->block_cnt_alloc; i++) {
			list_del(&txbuf->list);
			_os_kmem_free(drv, txbuf->buffer, txbuf->buf_len);
			txbuf->buffer = NULL;
			txbuf->buf_len = 0;
			txbuf++;
		}

		pq_deinit(drv, &tx_pool->idle_list);
		pq_deinit(drv, &tx_pool->busy_list);
		pq_deinit(drv, &tx_pool->mgnt_idle_list);
		pq_deinit(drv, &tx_pool->mgnt_busy_list);

		_os_mem_free(drv, tx_pool->txbufblock,
			     tx_pool->total_blocks_size);
		_os_mem_free(drv, tx_pool,
			     sizeof(struct rtw_tx_buf_ring));
	}
#else /* !SDIO_TX_THREAD */
	if (hci->txbuf_pool) {
		txbuf = (struct rtw_tx_buf*)hci->txbuf_pool;
		hci->txbuf_pool = NULL;
		if (txbuf->buffer)
			_os_kmem_free(drv, txbuf->buffer, txbuf->buf_len);
		_os_mem_free(drv, txbuf, sizeof(struct rtw_tx_buf));
	}
#endif /* !SDIO_TX_THREAD */
	if (hci->tx_drop_cnt) {
		PHL_WARN(
			  "%s: tx_drop_cnt=%u%s\n", __FUNCTION__,
			  hci->tx_drop_cnt & ~BIT31,
			  hci->tx_drop_cnt & BIT31 ? "(overflow)" : "");
		hci->tx_drop_cnt = 0;
	}

	FUNCOUT();
}

static enum rtw_phl_status phl_trx_init_sdio(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;
	struct hci_info_t *hci = phl_info->hci;
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl_info->hal);
	struct bus_cap_t *bus_cap = &hal_com->bus_cap;
	void *drv = phl_to_drvpriv(phl_info);
#ifdef SDIO_TX_THREAD
	struct rtw_tx_buf_ring *tx_pool;
#ifdef CONFIG_PHL_CPU_BALANCE_THREAD
	_os_thread *pthread = &hci->tx_thrd;
#endif /*CONFIG_PHL_CPU_BALANCE_THREAD*/
#endif
	struct rtw_tx_buf *txbuf;
	struct rtw_rx_buf_ring *rx_pool;
	struct rtw_rx_buf *rxbuf;
	u32 i;


	FUNCIN_WSTS(pstatus);
	PHL_DBG(
		  "%s: tx_buf_num(%u)\n", __FUNCTION__, bus_cap->tx_buf_num);
	PHL_DBG(
		  "%s: tx_buf_size(%u)\n", __FUNCTION__, bus_cap->tx_buf_size);
	PHL_DBG(
		  "%s: mgnt_buf_num(%u)\n", __FUNCTION__, bus_cap->tx_mgnt_buf_num);
	PHL_DBG(
		  "%s: mgnt_buf_size(%u)\n", __FUNCTION__, bus_cap->tx_mgnt_buf_size);
	PHL_DBG(
		  "%s: rx_buf_num(%u)\n", __FUNCTION__, bus_cap->rx_buf_num);
	PHL_DBG(
		  "%s: rx_buf_size(%u)\n", __FUNCTION__, bus_cap->rx_buf_size);

	do {
#ifdef SDIO_TX_THREAD
		hci->txbuf_pool = _os_mem_alloc(drv,
						sizeof(struct rtw_tx_buf_ring));
		if (!hci->txbuf_pool)
			break;
		tx_pool = (struct rtw_tx_buf_ring*)hci->txbuf_pool;
		tx_pool->block_cnt_alloc = bus_cap->tx_mgnt_buf_num
					   + bus_cap->tx_buf_num;
		tx_pool->total_blocks_size = tx_pool->block_cnt_alloc
					     * sizeof(struct rtw_tx_buf);
		tx_pool->txbufblock = _os_mem_alloc(drv,
						tx_pool->total_blocks_size);
		if (!tx_pool->txbufblock)
			break;

		pq_init(drv, &tx_pool->idle_list);
		pq_init(drv, &tx_pool->busy_list);
		pq_init(drv, &tx_pool->mgnt_idle_list);
		pq_init(drv, &tx_pool->mgnt_busy_list);

		txbuf = (struct rtw_tx_buf *)tx_pool->txbufblock;
		for (i = 0; i < bus_cap->tx_mgnt_buf_num; i++, txbuf++) {
			txbuf->tag = RTW_PHL_PKT_TYPE_MGNT;
			txbuf->buf_len = bus_cap->tx_mgnt_buf_size;
			txbuf->buffer = _os_kmem_alloc(drv, txbuf->buf_len);
			if (!txbuf->buffer)
				break;
			INIT_LIST_HEAD(&txbuf->list);
			enqueue_txbuf(phl_info, &tx_pool->mgnt_idle_list, txbuf, _tail);
		}
		if (i != bus_cap->tx_mgnt_buf_num)
			break;
		for (; i < tx_pool->block_cnt_alloc; i++, txbuf++) {
			txbuf->tag = RTW_PHL_PKT_TYPE_DATA;
			txbuf->buf_len = bus_cap->tx_buf_size;
			txbuf->buffer = _os_kmem_alloc(drv, txbuf->buf_len);
			if (!txbuf->buffer)
				break;
			INIT_LIST_HEAD(&txbuf->list);
			enqueue_txbuf(phl_info, &tx_pool->idle_list, txbuf, _tail);
		}
		if (i != tx_pool->block_cnt_alloc)
			break;

#ifdef CONFIG_PHL_SDIO_TX_CB_THREAD
#ifndef RTW_WKARD_SDIO_TX_USE_YIELD
		_os_spinlock_init(drv, &hci->tx_buf_lock);
#endif /* !RTW_WKARD_SDIO_TX_USE_YIELD */
#endif /* CONFIG_PHL_SDIO_TX_CB_THREAD */
#else /* !SDIO_TX_THREAD */
		hci->txbuf_pool = _os_mem_alloc(drv, sizeof(struct rtw_tx_buf));
		if (!hci->txbuf_pool)
			break;
		txbuf = (struct rtw_tx_buf*)hci->txbuf_pool;
		txbuf->buf_len = bus_cap->tx_buf_size;
		txbuf->buffer = _os_kmem_alloc(drv, txbuf->buf_len);
#endif /* !SDIO_TX_THREAD */

		hci->rxbuf_pool = _os_mem_alloc(drv,
						sizeof(struct rtw_rx_buf_ring));
		if (!hci->rxbuf_pool)
			break;
		rx_pool = (struct rtw_rx_buf_ring*)hci->rxbuf_pool;
		rx_pool->block_cnt_alloc = bus_cap->rx_buf_num;
		rx_pool->total_blocks_size = rx_pool->block_cnt_alloc
					     * sizeof(struct rtw_rx_buf);
		rx_pool->rxbufblock = _os_mem_alloc(drv,
						rx_pool->total_blocks_size);
		if (!rx_pool->rxbufblock)
			break;

		pq_init(drv, &rx_pool->idle_rxbuf_list);
		pq_init(drv, &rx_pool->busy_rxbuf_list);
		pq_init(drv, &rx_pool->pend_rxbuf_list);

		rxbuf = (struct rtw_rx_buf *)rx_pool->rxbufblock;
		for (i = 0; i < rx_pool->block_cnt_alloc; i++) {
			rxbuf->buf_len = bus_cap->rx_buf_size;
			rxbuf->buffer = _os_kmem_alloc(drv, rxbuf->buf_len);
			if (!rxbuf->buffer)
				break;

			INIT_LIST_HEAD(&rxbuf->list);
			enqueue_rxbuf(phl_info, &rx_pool->idle_rxbuf_list,
				      rxbuf);

			rxbuf++;
		}
		if (i != rx_pool->block_cnt_alloc)
			break;

		pstatus = phl_register_trx_hdlr_sdio(phl_info);

#ifdef SDIO_TX_THREAD
		_os_sema_init(drv, &hci->tx_thrd_sema, 0);
#ifdef CONFIG_PHL_CPU_BALANCE_THREAD
		pthread->en_assign_cpuid = _TRUE;
		pthread->cpu_id = CPU_ID_TX;
#endif /*CONFIG_PHL_CPU_BALANCE_THREAD*/
		if (RTW_PHL_STATUS_SUCCESS != _os_thread_init(drv, &hci->tx_thrd, phl_tx_sdio_thrd_hdl,
				phl_info, "rtw_sdio_tx")) {
			PHL_ERR("thread init rtw_sdio_tx fail.\n");
			break;
		}
		_os_thread_schedule(drv, &hci->tx_thrd);
#endif
	} while (false);

	if (RTW_PHL_STATUS_SUCCESS != pstatus)
		phl_trx_deinit_sdio(phl_info);

	FUNCOUT_WSTS(pstatus);

	return pstatus;
}

static u8 _rx_buf_size_kb(struct phl_info_t *phl)
{
	struct rtw_hal_com_t *hal_com = rtw_hal_get_halcom(phl->hal);
	struct bus_cap_t *bus_cap = &hal_com->bus_cap;
	u32 size;


	size = bus_cap->rx_buf_size >> 10; /* change unit to KB */
	return (u8)((size > 0xFF) ? 0xFF : size);
}

static enum rtw_phl_status phl_trx_cfg_sdio(struct phl_info_t *phl)
{
	/*
	 * Default TX setting.
	 * Include following setting:
	 * a. Release tx size limitation of (32K-4) bytes.
	 */
	rtw_hal_sdio_tx_cfg(phl->hal);

	/*
	 * default RX agg setting form mac team,
	 * timeout threshold 32us, size threshold is depended on rx buffer size.
	 * RX agg setting still need to be optimized by IC and real case.
	 */
	rtw_hal_sdio_rx_agg_cfg(phl->hal, true, 1, 32, _rx_buf_size_kb(phl), 0);

	return RTW_PHL_STATUS_SUCCESS;
}

static void phl_trx_stop_sdio(struct phl_info_t *phl)
{
}

#define _PLTFM_TX_TIMEOUT	50	/* unit: ms */
static enum rtw_phl_status phl_pltfm_tx_sdio(struct phl_info_t *phl, void *pkt)
{
	struct rtw_h2c_pkt *h2c_pkt = (struct rtw_h2c_pkt *)pkt;
	u8 dma_ch;
	u32 start;
	enum rtw_hal_status res;


	dma_ch = rtw_hal_get_fwcmd_queue_idx(phl->hal);

	start = _os_get_cur_time_ms();
	do {
		res = rtw_hal_sdio_tx(phl->hal, dma_ch, h2c_pkt->vir_head,
				      h2c_pkt->data_len, 1, NULL, NULL);
		if (res == RTW_HAL_STATUS_RESOURCE) {
			if (phl_get_passing_time_ms(start) < _PLTFM_TX_TIMEOUT)
				continue;

			PHL_ERR(
				  "%s: pltfm_tx timeout(> %u ms)!\n",
				  __FUNCTION__, _PLTFM_TX_TIMEOUT);
		}
		break;
	} while (1);

	phl_enqueue_idle_h2c_pkt(phl, h2c_pkt);
	if (res == RTW_HAL_STATUS_SUCCESS)
		return RTW_PHL_STATUS_SUCCESS;

	PHL_ERR(
		  "%s: pltfm_tx fail!(0x%x)\n", __FUNCTION__, res);
	return RTW_PHL_STATUS_FAILURE;
}

static void phl_free_h2c_pkt_buf_sdio(struct phl_info_t *phl_info,
				      struct rtw_h2c_pkt *h2c_pkt)
{
	if (!h2c_pkt->vir_head || !h2c_pkt->buf_len)
		return;

	_os_kmem_free(phl_to_drvpriv(phl_info),
		      h2c_pkt->vir_head, h2c_pkt->buf_len);
	h2c_pkt->vir_head = NULL;
	h2c_pkt->buf_len = 0;
}

static enum rtw_phl_status phl_alloc_h2c_pkt_buf_sdio(
				struct phl_info_t *phl_info,
				struct rtw_h2c_pkt *h2c_pkt, u32 buf_len)
{
	void *buf = NULL;


	buf = _os_kmem_alloc(phl_to_drvpriv(phl_info), buf_len);
	if (!buf)
		return RTW_PHL_STATUS_FAILURE;

	h2c_pkt->vir_head = buf;
	h2c_pkt->buf_len = buf_len;

	return RTW_PHL_STATUS_SUCCESS;
}

static void phl_trx_reset_sdio(struct phl_info_t *phl, u8 type)
{
	struct rtw_phl_com_t *phl_com = phl->phl_com;
	struct rtw_stats *phl_stats = &phl_com->phl_stats;

	if (PHL_CTRL_TX & type) {
		phl_reset_tx_stats(phl_stats);
	}

	if (PHL_CTRL_RX & type) {
		phl_reset_rx_stats(phl_stats);
	}
}

static void phl_tx_reset_hwband_sdio(struct phl_info_t *phl_info, enum phl_band_idx band_idx)
{
}

static void phl_tx_resume_sdio(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);

	_os_atomic_set(drv, &phl_info->phl_sw_tx_sts, PHL_TX_STATUS_RUNNING);
}

static void phl_rx_resume_sdio(struct phl_info_t *phl_info)
{
	void *drv = phl_to_drvpriv(phl_info);

	_os_atomic_set(drv, &phl_info->phl_sw_rx_sts, PHL_RX_STATUS_RUNNING);
}

static void phl_trx_resume_sdio(struct phl_info_t *phl, u8 type)
{
	if (PHL_CTRL_TX & type)
		phl_tx_resume_sdio(phl);

	if (PHL_CTRL_RX & type)
		phl_rx_resume_sdio(phl);
}

static void phl_req_tx_stop_sdio(struct phl_info_t *phl)
{
	void *drv = phl_to_drvpriv(phl);

	_os_atomic_set(drv, &phl->phl_sw_tx_sts,
		PHL_TX_STATUS_STOP_INPROGRESS);
}

static void phl_req_rx_stop_sdio(struct phl_info_t *phl)
{
	void *drv = phl_to_drvpriv(phl);

	_os_atomic_set(drv, &phl->phl_sw_rx_sts,
		PHL_RX_STATUS_STOP_INPROGRESS);
}

static bool phl_is_tx_pause_sdio(struct phl_info_t *phl)
{
	void *drvpriv = phl_to_drvpriv(phl);

	if (PHL_TX_STATUS_SW_PAUSE != _os_atomic_read(drvpriv, &phl->phl_sw_tx_sts))
		return false;
	return true;
}

static bool phl_is_rx_pause_sdio(struct phl_info_t *phl)
{
	void *drvpriv = phl_to_drvpriv(phl);

	if (PHL_RX_STATUS_SW_PAUSE == _os_atomic_read(drvpriv,
		&phl->phl_sw_rx_sts)) {
		if (true == rtw_phl_is_phl_rx_idle(phl))
			return true;
		else
			return false;
	} else {
		return false;
	}
}

static void *phl_get_txbd_buf_sdio(struct phl_info_t *phl)
{
	return NULL;
}

static void *phl_get_rxbd_buf_sdio(struct phl_info_t *phl)
{
	return NULL;
}

void phl_recycle_rx_pkt_sdio(struct phl_info_t *phl_info,
				struct rtw_phl_rx_pkt *phl_rx)
{
	if (phl_rx->r.os_priv)
		_os_free_netbuf(phl_to_drvpriv(phl_info),
			phl_rx->r.pkt_list[0].vir_addr,
			phl_rx->r.os_netbuf_len,
			phl_rx->r.os_priv);

	phl_recycle_rx_buf(phl_info, phl_rx);
}


void phl_tx_watchdog_sdio(struct phl_info_t *phl_info)
{

}

static struct phl_hci_trx_ops ops_sdio = {
	.hci_trx_init = phl_trx_init_sdio,
	.hci_trx_deinit = phl_trx_deinit_sdio,

	.prepare_tx = phl_prepare_tx_sdio,
	.recycle_rx_buf = phl_recycle_rx_buf_sdio,
	.tx = phl_tx_sdio,
	.rx = phl_rx_sdio,
	.trx_cfg = phl_trx_cfg_sdio,
	.trx_stop = phl_trx_stop_sdio,
	.pltfm_tx = phl_pltfm_tx_sdio,
	.free_h2c_pkt_buf = phl_free_h2c_pkt_buf_sdio,
 	.alloc_h2c_pkt_buf = phl_alloc_h2c_pkt_buf_sdio,
	.trx_reset = phl_trx_reset_sdio,
	.trx_resume = phl_trx_resume_sdio,
	.tx_reset_hwband = phl_tx_reset_hwband_sdio,
	.req_tx_stop = phl_req_tx_stop_sdio,
	.req_rx_stop = phl_req_rx_stop_sdio,
	.is_tx_pause = phl_is_tx_pause_sdio,
	.is_rx_pause = phl_is_rx_pause_sdio,
	.get_txbd_buf = phl_get_txbd_buf_sdio,
	.get_rxbd_buf = phl_get_rxbd_buf_sdio,
	.recycle_rx_pkt = phl_recycle_rx_pkt_sdio,
	.rx_handle_normal = phl_rx_handle_normal,
	.tx_watchdog = phl_tx_watchdog_sdio,
#ifdef CONFIG_PHL_SDIO_READ_RXFF_IN_INT
	.recv_rxfifo = phl_recv_rxfifo_sdio,
#endif
};

enum rtw_phl_status phl_hook_trx_ops_sdio(struct phl_info_t *phl_info)
{
	enum rtw_phl_status pstatus = RTW_PHL_STATUS_FAILURE;

	if (NULL != phl_info) {
		phl_info->hci_trx_ops = &ops_sdio;
		pstatus = RTW_PHL_STATUS_SUCCESS;
	}

	return pstatus;
}
