/**
 ******************************************************************************
 *
 * @file bl_tx.c
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <net/dsfield.h>

#include "bl_defs.h"
#include "bl_tx.h"
#include "bl_msg_tx.h"
#include "bl_events.h"
#include "bl_sdio.h"
#include "bl_irqs.h"
#include "bl_compat.h"

static u16 g_pkt_sn = 0;

/******************************************************************************
 * Power Save functions
 *****************************************************************************/
/**
 * bl_set_traffic_status - Inform FW if traffic is available for STA in PS
 *
 * @bl_hw: Driver main data
 * @sta: Sta in PS mode
 * @available: whether traffic is buffered for the STA
 * @ps_id: type of PS data requested (@LEGACY_PS_ID or @UAPSD_ID)
  */
void bl_set_traffic_status(struct bl_hw *bl_hw,
                             struct bl_sta *sta,
                             bool available,
                             u8 ps_id)
{
    bool uapsd = (ps_id != LEGACY_PS_ID);
    bl_send_me_traffic_ind(bl_hw, sta->sta_idx, uapsd, available);
    trace_ps_traffic_update(sta->sta_idx, available, uapsd);
}

/**
 * bl_ps_bh_enable - Enable/disable PS mode for one STA
 *
 * @bl_hw: Driver main data
 * @sta: Sta which enters/leaves PS mode
 * @enable: PS mode status
 *
 * This function will enable/disable PS mode for one STA.
 * When enabling PS mode:
 *  - Stop all STA's txq for BL_TXQ_STOP_STA_PS reason
 *  - Count how many buffers are already ready for this STA
 *  - For BC/MC sta, update all queued SKB to use hw_queue BCMC
 *  - Update TIM if some packet are ready
 *
 * When disabling PS mode:
 *  - Start all STA's txq for BL_TXQ_STOP_STA_PS reason
 *  - For BC/MC sta, update all queued SKB to use hw_queue AC_BE
 *  - Update TIM if some packet are ready (otherwise fw will not update TIM
 *    in beacon for this STA)
 *
 * All counter/skb updates are protected from TX path by taking tx_lock
 *
 * NOTE: _bh_ in function name indicates that this function is called
 * from a bottom_half tasklet.
 */
void bl_ps_bh_enable(struct bl_hw *bl_hw, struct bl_sta *sta,
                       bool enable)
{
	struct bl_txq *txq = NULL;
    txq = bl_txq_sta_get(sta, 0, NULL, bl_hw);

    if (enable) {
        trace_ps_enable(sta);

        spin_lock(&bl_hw->tx_lock);
        sta->ps.active = true;
        sta->ps.sp_cnt[LEGACY_PS_ID] = 0;
        sta->ps.sp_cnt[UAPSD_ID] = 0;
        bl_txq_sta_stop(sta, BL_TXQ_STOP_STA_PS, bl_hw);

        if (is_multicast_sta(sta->sta_idx)) {
            sta->ps.pkt_ready[LEGACY_PS_ID] = skb_queue_len(&txq->sk_list);
            sta->ps.pkt_ready[UAPSD_ID] = 0;
            txq->hwq = &bl_hw->hwq[BL_HWQ_BCMC];
        } else {
            int i;

            sta->ps.pkt_ready[LEGACY_PS_ID] = 0;
            sta->ps.pkt_ready[UAPSD_ID] = 0;
            for (i = 0; i < NX_NB_TXQ_PER_STA; i++, txq++) {
                sta->ps.pkt_ready[txq->ps_id] += skb_queue_len(&txq->sk_list);
            }
        }

        spin_unlock(&bl_hw->tx_lock);

        if (sta->ps.pkt_ready[LEGACY_PS_ID])
            bl_set_traffic_status(bl_hw, sta, true, LEGACY_PS_ID);

        if (sta->ps.pkt_ready[UAPSD_ID])
            bl_set_traffic_status(bl_hw, sta, true, UAPSD_ID);
    } else {
        trace_ps_disable(sta->sta_idx);

        spin_lock(&bl_hw->tx_lock);
        sta->ps.active = false;

        if (is_multicast_sta(sta->sta_idx)) {
            txq->hwq = &bl_hw->hwq[BL_HWQ_BE];
            txq->push_limit = 0;
        } else {
            int i;
            for (i = 0; i < NX_NB_TXQ_PER_STA; i++, txq++) {
                txq->push_limit = 0;
            }
        }

        bl_txq_sta_start(sta, BL_TXQ_STOP_STA_PS, bl_hw);
        spin_unlock(&bl_hw->tx_lock);

        if (sta->ps.pkt_ready[LEGACY_PS_ID])
            bl_set_traffic_status(bl_hw, sta, false, LEGACY_PS_ID);

        if (sta->ps.pkt_ready[UAPSD_ID])
            bl_set_traffic_status(bl_hw, sta, false, UAPSD_ID);
    }
}

/**
 * bl_ps_bh_traffic_req - Handle traffic request for STA in PS mode
 *
 * @bl_hw: Driver main data
 * @sta: Sta which enters/leaves PS mode
 * @pkt_req: number of pkt to push
 * @ps_id: type of PS data requested (@LEGACY_PS_ID or @UAPSD_ID)
 *
 * This function will make sure that @pkt_req are pushed to fw
 * whereas the STA is in PS mode.
 * If request is 0, send all traffic
 * If request is greater than available pkt, reduce request
 * Note: request will also be reduce if txq credits are not available
 *
 * All counter updates are protected from TX path by taking tx_lock
 *
 * NOTE: _bh_ in function name indicates that this function is called
 * from the bottom_half tasklet.
 */
void bl_ps_bh_traffic_req(struct bl_hw *bl_hw, struct bl_sta *sta,
                            u16 pkt_req, u8 ps_id)
{
    int pkt_ready_all;
    struct bl_txq *txq;

    if (WARN(!sta->ps.active, "sta %pM is not in Power Save mode",
             sta->mac_addr))
        return;

    trace_ps_traffic_req(sta, pkt_req, ps_id);

    spin_lock(&bl_hw->tx_lock);

    pkt_ready_all = (sta->ps.pkt_ready[ps_id] - sta->ps.sp_cnt[ps_id]);

    /* Don't start SP until previous one is finished or we don't have
       packet ready (which must not happen for U-APSD) */
    if (sta->ps.sp_cnt[ps_id] || pkt_ready_all <= 0) {
        goto done;
    }

    /* Adapt request to what is available. */
    if (pkt_req == 0 || pkt_req > pkt_ready_all) {
        pkt_req = pkt_ready_all;
    }

    /* Reset the SP counter */
    sta->ps.sp_cnt[ps_id] = 0;

    /* "dispatch" the request between txq */
    txq = bl_txq_sta_get(sta, NX_NB_TXQ_PER_STA - 1, NULL, bl_hw);

    if (is_multicast_sta(sta->sta_idx)) {
        if (txq->credits <= 0)
            goto done;
        if (pkt_req > txq->credits)
            pkt_req = txq->credits;
        txq->push_limit = pkt_req;
        sta->ps.sp_cnt[ps_id] = pkt_req;
        bl_txq_add_to_hw_list(txq);
    } else {
        int i;
        /* TODO: dispatch using correct txq priority */
        for (i = NX_NB_TXQ_PER_STA - 1; i >= 0; i--, txq--) {
            u16 txq_len = skb_queue_len(&txq->sk_list);

            if (txq->ps_id != ps_id)
                continue;

            if (txq_len > txq->credits)
                txq_len = txq->credits;

            if (txq_len > 0) {
                if (txq_len < pkt_req) {
                    /* Not enough pkt queued in this txq, add this
                       txq to hwq list and process next txq */
                    pkt_req -= txq_len;
                    txq->push_limit = txq_len;
                    sta->ps.sp_cnt[ps_id] += txq_len;
                    bl_txq_add_to_hw_list(txq);
                } else {
                    /* Enough pkt in this txq to comlete the request
                       add this txq to hwq list and stop processing txq */
                    txq->push_limit = pkt_req;
                    sta->ps.sp_cnt[ps_id] += pkt_req;
                    bl_txq_add_to_hw_list(txq);
                    break;
                }
            }
        }
    }

  done:
    spin_unlock(&bl_hw->tx_lock);
}

/******************************************************************************
 * TX functions
 *****************************************************************************/
static const int bl_down_hwq2tid[3] = {
    [BL_HWQ_BK] = 2,
    [BL_HWQ_BE] = 3,
    [BL_HWQ_VI] = 5,
};

void bl_downgrade_ac(struct bl_sta *sta, struct sk_buff *skb)
{
    int8_t ac = bl_tid2hwq[skb->priority];

    if (WARN((ac > BL_HWQ_VO),
             "Unexepcted ac %d for skb before downgrade", ac))
        ac = BL_HWQ_VO;

    while (sta->acm & BIT(ac)) {
        if (ac == BL_HWQ_BK) {
            skb->priority = 1;
            return;
        }
        ac--;
        skb->priority = bl_down_hwq2tid[ac];
    }
}


/**
 * bl_set_more_data_flag - Update MORE_DATA flag in tx sw desc
 *
 * @bl_hw: Driver main data
 * @sw_txhdr: Header for pkt to be pushed
 *
 * If STA is in PS mode
 *  - Set EOSP in case the packet is the last of the UAPSD service period
 *  - Set MORE_DATA flag if more pkt are ready for this sta
 *  - Update TIM if this is the last pkt buffered for this sta
 *
 * note: tx_lock already taken.
 */
static inline void bl_set_more_data_flag(struct bl_hw *bl_hw,
                                           struct bl_sw_txhdr *sw_txhdr)
{
    struct bl_sta *sta = sw_txhdr->bl_sta;
    struct bl_txq *txq = sw_txhdr->txq;

    if (unlikely(sta->ps.active)) {
        sta->ps.pkt_ready[txq->ps_id]--;
        sta->ps.sp_cnt[txq->ps_id]--;

        trace_ps_push(sta);

        if (((txq->ps_id == UAPSD_ID))
                && !sta->ps.sp_cnt[txq->ps_id]) {
            sw_txhdr->desc.host.flags |= TXU_CNTRL_EOSP;
        }

        if (sta->ps.pkt_ready[txq->ps_id]) {
            sw_txhdr->desc.host.flags |= TXU_CNTRL_MORE_DATA;
        } else {
            bl_set_traffic_status(bl_hw, sta, false, txq->ps_id);
        }
    }
}

#define PRIO_STA_NULL 0xAA
/**
 * bl_get_tx_info - Get STA and tid for one skb
 *
 * @bl_vif: vif ptr
 * @skb: skb
 * @tid: pointer updated with the tid to use for this skb
 *
 * @return: pointer on the destination STA (may be NULL)
 *
 * skb has already been parsed in bl_select_queue function
 * simply re-read information form skb.
 */
static struct bl_sta *bl_get_tx_info(struct bl_vif *bl_vif,
                                         struct sk_buff *skb,
                                         u8 *tid)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct bl_sta *sta = NULL;
    struct wireless_dev *wdev = &bl_vif->wdev;
	struct ethhdr *eth = (struct ethhdr *)skb->data;

	*tid = skb->priority;

	 if (unlikely(skb->priority == PRIO_STA_NULL)) {
        return NULL;
	 } else {
		switch (wdev->iftype) {
		case NL80211_IFTYPE_STATION:
		case NL80211_IFTYPE_P2P_CLIENT:
		{
			sta = bl_vif->sta.ap;
			break;
		}
		case NL80211_IFTYPE_AP_VLAN:
			if (bl_vif->ap_vlan.sta_4a) {
				sta = bl_vif->ap_vlan.sta_4a;
				break;
			}
		
			/* AP_VLAN interface is not used for a 4A STA,
			   fallback searching sta amongs all AP's clients */
			bl_vif = bl_vif->ap_vlan.master;
		case NL80211_IFTYPE_AP:
		case NL80211_IFTYPE_P2P_GO:
		{
			struct bl_sta *cur;
			if (is_broadcast_ether_addr(eth->h_dest) || is_multicast_ether_addr(eth->h_dest)) {
				sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];
			} else {
				list_for_each_entry(cur, &bl_vif->ap.sta_list, list) {
					if (!memcmp(cur->mac_addr, eth->h_dest, ETH_ALEN)) {
						sta = cur;
						break;
					}
				}
			}
		
			break;
		}
		default:
			break;
		}
	 }

	 
	return sta;
}

#if 0
/**
 * bl_get_tx_info - Get STA and tid for one skb
 *
 * @bl_vif: vif ptr
 * @skb: skb
 * @tid: pointer updated with the tid to use for this skb
 *
 * @return: pointer on the destination STA (may be NULL)
 *
 * skb has already been parsed in bl_select_queue function
 * simply re-read information form skb.
 */
static struct bl_sta *bl_get_tx_info(struct bl_vif *bl_vif,
                                         struct sk_buff *skb,
                                         u8 *tid)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct bl_sta *sta;
    int sta_idx;

    *tid = skb->priority;
    if (unlikely(skb->priority == PRIO_STA_NULL)) {
        return NULL;
    } else {
        int ndev_idx = skb_get_queue_mapping(skb);
        if (ndev_idx == NX_BCMC_TXQ_NDEV_IDX)
            sta_idx = NX_REMOTE_STA_MAX + master_vif_idx(bl_vif);
        else
            sta_idx = ndev_idx / NX_NB_TID_PER_STA;

        sta = &bl_hw->sta_table[sta_idx];
    }

    return sta;
}
#endif

/**
 *  bl_tx_push - Push one packet to fw
 *
 * @bl_hw: Driver main data
 * @txhdr: tx desc of the buffer to push
 * @flags: push flags (see @bl_push_flags)
 *
 * Push one packet to fw. Sw desc of the packet has already been updated.
 * Only MORE_DATA flag will be set if needed.
 */
void bl_tx_push(struct bl_hw *bl_hw, struct bl_txhdr *txhdr, int flags)
{
    struct bl_sw_txhdr *sw_txhdr = txhdr->sw_hdr;
    struct txdesc_api *desc;
    struct sk_buff *skb = sw_txhdr->skb;
    struct bl_txq *txq = sw_txhdr->txq;
    u16 hw_queue = txq->hwq->id;
    int user = 0;
	u32 wr_port;
	u32 ret = 0;
	ktime_t start;

    lockdep_assert_held(&bl_hw->tx_lock);

    /* RETRY flag is not always set so retest here */
    if (txq->nb_retry) {
        flags |= BL_PUSH_RETRY;
        txq->nb_retry--;
        if (txq->nb_retry == 0) {
            WARN(skb != txq->last_retry_skb,
                 "last retry buffer is not the expected one");
            txq->last_retry_skb = NULL;
        }
    } else if (!(flags & BL_PUSH_RETRY)) {
        txq->pkt_sent++;
    }

#ifdef CONFIG_BL_AMSDUS_TX
    if (txq->amsdu == sw_txhdr) {
        WARN((flags & BL_PUSH_RETRY), "End A-MSDU on a retry");
        bl_hw->stats.amsdus[sw_txhdr->amsdu.nb - 1].done++;
        txq->amsdu = NULL;
    } else if (!(flags & BL_PUSH_RETRY) &&
               !(sw_txhdr->desc.host.flags & TXU_CNTRL_AMSDU)) {
        bl_hw->stats.amsdus[0].done++;
    }
#endif /* CONFIG_BL_AMSDUS_TX */

    /* Wait here to update hw_queue, as for multicast STA hwq may change
       between queue and push (because of PS) */
    sw_txhdr->hw_queue = hw_queue;

    if (sw_txhdr->bl_sta) {
        /* only for AP mode */
        bl_set_more_data_flag(bl_hw, sw_txhdr);
    }

    trace_push_desc(skb, sw_txhdr, flags);
	BL_DBG("bl_tx_push:txq->idx=%d, txq->status=0x%x, txq->credits: %d--->%d\n", txq->idx, txq->status, txq->credits, txq->credits-1);
    txq->credits--;
    txq->pkt_pushed[user]++;

    if (txq->credits <= 0) {
        bl_txq_stop(txq, BL_TXQ_STOP_FULL);
		BL_DBG("delete txq from hwq, txq->idx=%d, txq->status=0x%x, txq-credits=%d\n", txq->idx, txq->status, txq->credits);
	}

    if (txq->push_limit)
        txq->push_limit--;

	/*use sdio interface to send the whole skb packet */
	/* fisrt, we should ignore the txhdr, after skb_pull, we got the real data */
	skb_pull(skb, sw_txhdr->headroom);
	/*use sw_txhdr to override the sdio special header*/
	sw_txhdr->hdr.queue_idx = txq->hwq->id; //update queue idx to avoid ps mode modify it.
	memcpy((void *)skb->data, &sw_txhdr->hdr, sizeof(struct sdio_hdr));
	memcpy((void *)skb->data + sizeof(struct sdio_hdr), &sw_txhdr->desc, sizeof(*desc));

	/*when get wr_port failed*/
	ret = bl_get_wr_port(bl_hw, &wr_port);
	if (ret) {
		printk("get wr port failed ret=%d, requeue skb=%p\n", ret, skb);
		skb_push(skb, sw_txhdr->headroom);
		bl_txq_queue_skb(skb, txq, bl_hw, false);
		return;
	}

	BL_DBG("bl_tx_push: send skb=%p, skb->len=%d\n", skb, skb->len);

	start = ktime_get();
	ret = bl_write_data_sync(bl_hw, skb->data, skb->len, bl_hw->plat->io_port + wr_port);
	bl_hw->dbg_time[bl_hw->dbg_time_idx].sdio_tx = ktime_us_delta(ktime_get(), start);
	bl_hw->dbg_time_idx++;

	if(bl_hw->dbg_time_idx == 50)
		bl_hw->dbg_time_idx = 0;
	if(ret) {
		printk("bl_write_data_sync failed, ret=%d\n", ret);
		dev_kfree_skb_any(skb);
		return;
	}

	/*restore skb, txcfm will use this skb*/
	skb_push(skb, sw_txhdr->headroom);
    ipc_host_txdesc_push(bl_hw->ipc_env, hw_queue, user, skb);
    txq->hwq->credits[user]--;
    bl_hw->stats.cfm_balance[hw_queue]++;
}

/** 2K buf size */
#define BL_TX_DATA_BUF_SIZE_16K        16*1024
#define BL_SDIO_MP_AGGR_PKT_LIMIT_MAX 	8
#define BL_SDIO_MPA_ADDR_BASE			0x1000

//typedef struct _sdio_mpa_tx {
//		u8 *buf;
//		u32 buf_len;
//		u32 pkt_cnt;
//		u32 ports;
//		u16 start_port;
//		u16 mp_wr_info[BL_SDIO_MP_AGGR_PKT_LIMIT_MAX];
//}sdio_mpa_tx;

void bl_tx_multi_pkt_push(struct bl_hw *bl_hw, struct sk_buff_head *sk_list_push)
{
	struct sk_buff *skb;
	struct bl_txhdr *txhdr;
	sdio_mpa_tx mpa_tx_data = {0};
	int ret;
	u32 port;
	u32 cmd53_port;
	u32 buf_block_len;
	int flags = 0;

	/*fix alloc buf size, such as 16K(2*8(aggr num))*/
	mpa_tx_data.buf = bl_hw->mpa_tx_data.buf;
	if(mpa_tx_data.buf == NULL){
        printk("mpa_tx_data.buf is null!\n");
        return;
	}
	
	/*copy multi skbs into one large buf*/
	while ((skb = __skb_dequeue(sk_list_push)) != NULL) {
		txhdr = (struct bl_txhdr *)skb->data;
    	/* RETRY flag is not always set so retest here */
    	if (txhdr->sw_hdr->txq->nb_retry) {
        	flags |= BL_PUSH_RETRY;
        	txhdr->sw_hdr->txq->nb_retry--;
        	if (txhdr->sw_hdr->txq->nb_retry == 0) {
            	WARN(skb != txhdr->sw_hdr->txq->last_retry_skb,
                	 "last retry buffer is not the expected one");
            	txhdr->sw_hdr->txq->last_retry_skb = NULL;
        	}
    	} else if (!(flags & BL_PUSH_RETRY)) {
        	txhdr->sw_hdr->txq->pkt_sent++;
    	}

		txhdr->sw_hdr->hw_queue = txhdr->sw_hdr->txq->hwq->id;

		if(txhdr->sw_hdr->bl_sta)
			bl_set_more_data_flag(bl_hw, txhdr->sw_hdr);

//		txhdr->sw_hdr->txq->credits--;
		txhdr->sw_hdr->txq->pkt_pushed[0]++;

//		if(txhdr->sw_hdr->txq->credits <= 0)
//			bl_txq_stop(txhdr->sw_hdr->txq, BL_TXQ_STOP_FULL);

		skb_pull(skb, txhdr->sw_hdr->headroom);
		txhdr->sw_hdr->hdr.queue_idx = txhdr->sw_hdr->txq->hwq->id;
		memcpy((void *)skb->data, &txhdr->sw_hdr->hdr, sizeof(struct sdio_hdr));
		memcpy((void *)skb->data + sizeof(struct sdio_hdr), &txhdr->sw_hdr->desc, sizeof(struct txdesc_api));
		buf_block_len = (skb->len + BL_SDIO_BLOCK_SIZE - 1) / BL_SDIO_BLOCK_SIZE;
		memcpy((void *)&mpa_tx_data.buf[mpa_tx_data.buf_len], skb->data, buf_block_len * BL_SDIO_BLOCK_SIZE);
		mpa_tx_data.buf_len += buf_block_len * BL_SDIO_BLOCK_SIZE;
		
		//printk("###%d: skb->len: %d, pad_len: %d\n", mpa_tx_data.pkt_cnt, skb->len, buf_block_len*BL_SDIO_BLOCK_SIZE);

		//mpa_tx_data.mp_wr_info[mpa_tx_data.pkt_cnt] = *(u16 *)skb->data;

		bl_get_wr_port(bl_hw, &port);
		if(!mpa_tx_data.pkt_cnt) {
			mpa_tx_data.start_port = port;
		}

		if(mpa_tx_data.start_port <= port) {
			mpa_tx_data.ports |= (1 << (mpa_tx_data.pkt_cnt));
		} else {
			mpa_tx_data.ports |= (1 << (mpa_tx_data.pkt_cnt + 1));
		}

		mpa_tx_data.pkt_cnt++;

		skb_push(skb, txhdr->sw_hdr->headroom);
		ipc_host_txdesc_push(bl_hw->ipc_env, txhdr->sw_hdr->hw_queue, 0, skb);
//		txhdr->sw_hdr->txq->hwq->credits[0]--;
		bl_hw->stats.cfm_balance[txhdr->sw_hdr->hw_queue]++;
		
	}
/*
	printk("mpa_tx_data:ports=0x%02x, start_port=%d, buf=%p, buf_len=%d, pkt_cnt=%d\n",
			mpa_tx_data.ports, 
			mpa_tx_data.start_port,
			mpa_tx_data.buf,
			mpa_tx_data.buf_len,
			mpa_tx_data.pkt_cnt);
*/

	/*send packet*/
	cmd53_port = (bl_hw->plat->io_port | BL_SDIO_MPA_ADDR_BASE |
					(mpa_tx_data.ports << 4)) + mpa_tx_data.start_port;

//	printk("cmd53_port=0x%08x\n", cmd53_port);
	
	ret = bl_write_data_sync(bl_hw, mpa_tx_data.buf, mpa_tx_data.buf_len, cmd53_port);
	if(ret)
		printk("bl_write_data_sync failed, ret=%d\n", ret);

    if(bl_hw->mpa_tx_data.buf)
	    memset(bl_hw->mpa_tx_data.buf, 0, BL_RX_DATA_BUF_SIZE_16K);
}

/**
 * bl_tx_retry - Push an AMPDU pkt that need to be retried
 *
 * @bl_hw: Driver main data
 * @skb: pkt to re-push
 * @txhdr: tx desc of the pkt to re-push
 * @sw_retry: Indicates if fw decide to retry this buffer
 *            (i.e. it has never been transmitted over the air)
 *
 * Called when a packet needs to be repushed to the firmware.
 * First update sw descriptor and then queue it in the retry list.
 */
static void bl_tx_retry(struct bl_hw *bl_hw, struct sk_buff *skb,
                           struct bl_txhdr *txhdr, bool sw_retry)
{
    struct bl_sw_txhdr *sw_txhdr = txhdr->sw_hdr;
    struct tx_cfm_tag *cfm = &txhdr->hw_hdr.cfm;
    struct bl_txq *txq = sw_txhdr->txq;

    if (!sw_retry) {
        /* update sw desc */
        sw_txhdr->desc.host.sn = cfm->sn;
        sw_txhdr->desc.host.pn[0] = cfm->pn[0];
        sw_txhdr->desc.host.pn[1] = cfm->pn[1];
        sw_txhdr->desc.host.pn[2] = cfm->pn[2];
        sw_txhdr->desc.host.pn[3] = cfm->pn[3];
        sw_txhdr->desc.host.timestamp = cfm->timestamp;
        sw_txhdr->desc.host.flags |= TXU_CNTRL_RETRY;

        #ifdef CONFIG_BL_AMSDUS_TX
        if (sw_txhdr->desc.host.flags & TXU_CNTRL_AMSDU)
            bl_hw->stats.amsdus[sw_txhdr->amsdu.nb - 1].failed++;
        #endif
    }

    /* MORE_DATA will be re-set if needed when pkt will be repushed */
    sw_txhdr->desc.host.flags &= ~TXU_CNTRL_MORE_DATA;

    cfm->status.value = 0;

	BL_DBG("bl_tx_retry: skb=%p, sn=%u\n", skb, sw_txhdr->desc.host.sn);
	BL_DBG("bl_tx_retry: txq->idx=%d, txq->status=0x%x, txq->credits=%d-->%d\n", txq->idx, txq->status, txq->credits, txq->credits+1);
    txq->credits++;
/*	
	spin_lock_bh(&bl_hw->txq_lock);
    if (txq->credits > 0)
        bl_txq_start(txq, BL_TXQ_STOP_FULL);
	spin_unlock_bh(&bl_hw->txq_lock);
	*/
    /* Queue the buffer */
    bl_txq_queue_skb(skb, txq, bl_hw, true);
}


#ifdef CONFIG_BL_AMSDUS_TX
/* return size of subframe (including header) */
static inline int bl_amsdu_subframe_length(struct ethhdr *eth, int eth_len)
{
    /* ethernet header is replaced with amdsu header that have the same size
       Only need to check if LLC/SNAP header will be added */
    int len = eth_len;

    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        len += sizeof(rfc1042_header) + 2;
    }

    return len;
}

static inline bool bl_amsdu_is_aggregable(struct sk_buff *skb)
{
    /* need to add some check on buffer to see if it can be aggregated ? */
    return true;
}


/**
 * bl_amsdu_del_subframe_header - remove AMSDU header
 *
 * amsdu_txhdr: amsdu tx descriptor
 *
 * Move back the ethernet header at the "beginning" of the data buffer.
 * (which has been moved in @bl_amsdu_add_subframe_header)
 */
static void bl_amsdu_del_subframe_header(struct bl_amsdu_txhdr *amsdu_txhdr)
{
    struct sk_buff *skb = amsdu_txhdr->skb;
    struct ethhdr *eth;
    u8 *pos;

    pos = skb->data;
    pos += sizeof(struct bl_amsdu_txhdr);
    eth = (struct ethhdr*)pos;
    pos += amsdu_txhdr->pad + sizeof(struct ethhdr);

    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        pos += sizeof(rfc1042_header) + 2;
    }

    memmove(pos, eth, sizeof(*eth));
    skb_pull(skb, (pos - skb->data));
}

/**
 * bl_amsdu_add_subframe_header - Add AMSDU header and link subframe
 *
 * @bl_hw Driver main data
 * @skb Buffer to aggregate
 * @sw_txhdr Tx descriptor for the first A-MSDU subframe
 *
 * return 0 on sucess, -1 otherwise
 *
 * This functions Add A-MSDU header and LLC/SNAP header in the buffer
 * and update sw_txhdr of the first subframe to link this buffer.
 * If an error happens, the buffer will be queued as a normal buffer.
 *
 *
 *            Before           After
 *         +-------------+  +-------------+
 *         | HEADROOM    |  | HEADROOM    |
 *         |             |  +-------------+ <- data
 *         |             |  | amsdu_txhdr |
 *         |             |  | * pad size  |
 *         |             |  +-------------+
 *         |             |  | ETH hdr     | keep original eth hdr
 *         |             |  |             | to restore it once transmitted
 *         |             |  +-------------+ <- packet_addr[x]
 *         |             |  | Pad         |
 *         |             |  +-------------+
 * data -> +-------------+  | AMSDU HDR   |
 *         | ETH hdr     |  +-------------+
 *         |             |  | LLC/SNAP    |
 *         +-------------+  +-------------+
 *         | DATA        |  | DATA        |
 *         |             |  |             |
 *         +-------------+  +-------------+
 *
 * Called with tx_lock hold
 */
static int bl_amsdu_add_subframe_header(struct bl_hw *bl_hw,
                                          struct sk_buff *skb,
                                          struct bl_sw_txhdr *sw_txhdr)
{
    struct bl_amsdu *amsdu = &sw_txhdr->amsdu;
    struct bl_amsdu_txhdr *amsdu_txhdr;
    struct ethhdr *amsdu_hdr, *eth = (struct ethhdr *)skb->data;
    int headroom_need, map_len, msdu_len;
    dma_addr_t dma_addr;
    u8 *pos, *map_start;

    msdu_len = skb->len - sizeof(*eth);
    headroom_need = sizeof(*amsdu_txhdr) + amsdu->pad +
        sizeof(*amsdu_hdr);
    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        headroom_need += sizeof(rfc1042_header) + 2;
        msdu_len += sizeof(rfc1042_header) + 2;
    }

    /* we should have enough headroom (checked in xmit) */
    if (WARN_ON(skb_headroom(skb) < headroom_need)) {
        return -1;
    }

    /* allocate headroom */
    pos = skb_push(skb, headroom_need);
    amsdu_txhdr = (struct bl_amsdu_txhdr *)pos;
    pos += sizeof(*amsdu_txhdr);

    /* move eth header */
    memmove(pos, eth, sizeof(*eth));
    eth = (struct ethhdr *)pos;
    pos += sizeof(*eth);

    /* Add padding from previous subframe */
    map_start = pos;
    memset(pos, 0, amsdu->pad);
    pos += amsdu->pad;

    /* Add AMSDU hdr */
    amsdu_hdr = (struct ethhdr *)pos;
    memcpy(amsdu_hdr->h_dest, eth->h_dest, ETH_ALEN);
    memcpy(amsdu_hdr->h_source, eth->h_source, ETH_ALEN);
    amsdu_hdr->h_proto = htons(msdu_len);
    pos += sizeof(*amsdu_hdr);

    if (ntohs(eth->h_proto) >= ETH_P_802_3_MIN) {
        memcpy(pos, rfc1042_header, sizeof(rfc1042_header));
        pos += sizeof(rfc1042_header);
    }

    /* MAP (and sync) memory for DMA */
    map_len = msdu_len + amsdu->pad + sizeof(*amsdu_hdr);
    dma_addr = dma_map_single(bl_hw->dev, map_start, map_len,
                              DMA_BIDIRECTIONAL);
    if (WARN_ON(dma_mapping_error(bl_hw->dev, dma_addr))) {
        pos -= sizeof(*eth);
        memmove(pos, eth, sizeof(*eth));
        skb_pull(skb, headroom_need);
        return -1;
    }

    /* update amdsu_txhdr */
    amsdu_txhdr->map_len = map_len;
    amsdu_txhdr->dma_addr = dma_addr;
    amsdu_txhdr->skb = skb;
    amsdu_txhdr->pad = amsdu->pad;

    /* update bl_sw_txhdr (of the first subframe) */
    BUG_ON(amsdu->nb != sw_txhdr->desc.host.packet_cnt);
    sw_txhdr->desc.host.packet_addr[amsdu->nb] = dma_addr;
    sw_txhdr->desc.host.packet_len[amsdu->nb] = map_len;
    sw_txhdr->desc.host.packet_cnt++;
    amsdu->nb++;

    amsdu->pad = AMSDU_PADDING(map_len - amsdu->pad);
    list_add_tail(&amsdu_txhdr->list, &amsdu->hdrs);
    amsdu->len += map_len;

    trace_amsdu_subframe(sw_txhdr);
    return 0;
}

/**
 * bl_amsdu_add_subframe - Add this buffer as an A-MSDU subframe if possible
 *
 * @bl_hw Driver main data
 * @skb Buffer to aggregate if possible
 * @sta Destination STA
 * @txq sta's txq used for this buffer
 *
 * Tyr to aggregate the buffer in an A-MSDU. If it succeed then the
 * buffer is added as a new A-MSDU subframe with AMSDU and LLC/SNAP
 * headers added (so FW won't have to modify this subframe).
 *
 * To be added as subframe :
 * - sta must allow amsdu
 * - buffer must be aggregable (to be defined)
 * - at least one other aggregable buffer is pending in the queue
 *  or an a-msdu (with enough free space) is currently in progress
 *
 * returns true if buffer has been added as A-MDSP subframe, false otherwise
 *
 */
static bool bl_amsdu_add_subframe(struct bl_hw *bl_hw, struct sk_buff *skb,
                                    struct bl_sta *sta, struct bl_txq *txq)
{
    bool res = false;
    struct ethhdr *eth;

    /* immedialtely return if amsdu are not allowed for this sta */
    if (!txq->amsdu_len || bl_hw->mod_params->amsdu_maxnb < 2 ||
        !bl_amsdu_is_aggregable(skb))
        return false;

    spin_lock_bh(&bl_hw->tx_lock);
    if (txq->amsdu) {
        /* aggreagation already in progress, add this buffer if enough space
           available, otherwise end the current amsdu */
        struct bl_sw_txhdr *sw_txhdr = txq->amsdu;
        eth = (struct ethhdr *)(skb->data);

        if (((sw_txhdr->amsdu.len + sw_txhdr->amsdu.pad +
              bl_amsdu_subframe_length(eth, skb->len)) > txq->amsdu_len) ||
            bl_amsdu_add_subframe_header(bl_hw, skb, sw_txhdr)) {
            txq->amsdu = NULL;
            goto end;
        }

        if (sw_txhdr->amsdu.nb >= bl_hw->mod_params->amsdu_maxnb) {
            bl_hw->stats.amsdus[sw_txhdr->amsdu.nb - 1].done++;
            /* max number of subframes reached */
            txq->amsdu = NULL;
        }
    } else {
        /* Check if a new amsdu can be started with the previous buffer
           (if any) and this one */
        struct sk_buff *skb_prev = skb_peek_tail(&txq->sk_list);
        struct bl_txhdr *txhdr;
        struct bl_sw_txhdr *sw_txhdr;
        int len1, len2;

        if (!skb_prev || !bl_amsdu_is_aggregable(skb_prev))
            goto end;

        txhdr = (struct bl_txhdr *)skb_prev->data;
        sw_txhdr = txhdr->sw_hdr;
        if ((sw_txhdr->amsdu.len) ||
            (sw_txhdr->desc.host.flags & TXU_CNTRL_RETRY))
            /* previous buffer is already a complete amsdu or a retry */
            goto end;

        eth = (struct ethhdr *)(skb_prev->data + sw_txhdr->headroom);
        len1 = bl_amsdu_subframe_length(eth, (sw_txhdr->frame_len +
                                                sizeof(struct ethhdr)));

        eth = (struct ethhdr *)(skb->data);
        len2 = bl_amsdu_subframe_length(eth, skb->len);

        if (len1 + AMSDU_PADDING(len1) + len2 > txq->amsdu_len)
            /* not enough space to aggregate those two buffers */
            goto end;

        /* Add subframe header.
           Note: Fw will take care of adding AMDSU header for the first
           subframe while generating 802.11 MAC header */
        INIT_LIST_HEAD(&sw_txhdr->amsdu.hdrs);
        sw_txhdr->amsdu.len = len1;
        sw_txhdr->amsdu.nb = 1;
        sw_txhdr->amsdu.pad = AMSDU_PADDING(len1);
        if (bl_amsdu_add_subframe_header(bl_hw, skb, sw_txhdr))
            goto end;

        sw_txhdr->desc.host.flags |= TXU_CNTRL_AMSDU;

        if (sw_txhdr->amsdu.nb < bl_hw->mod_params->amsdu_maxnb)
            txq->amsdu = sw_txhdr;
        else
            bl_hw->stats.amsdus[sw_txhdr->amsdu.nb - 1].done++;
    }

    res = true;

  end:
    spin_unlock_bh(&bl_hw->tx_lock);
    return res;
}
#endif /* CONFIG_BL_AMSDUS_TX */

int bl_requeue_multicast_skb(struct sk_buff *skb, struct bl_vif *bl_vif)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct bl_txhdr *txhdr;
    struct bl_sw_txhdr *sw_txhdr;
    struct ethhdr *eth;
	struct ethhdr tmp_eth;
    struct txdesc_api *desc;
    struct bl_sta *sta;
    struct bl_txq *txq;
    int headroom;
	int hdr_pads;

    u16 frame_len;
    u16 frame_oft;
    u8 tid;

	BL_DBG(BL_FN_ENTRY_STR);
	
	BL_DBG("1111bl_requeue_multicast_skb: skb=%p, skb->priority=%d\n", skb, skb->priority);

    /* Get the STA id and TID information */
    sta = bl_get_tx_info(bl_vif, skb, &tid);
    if (!sta)
        goto free;
	
    txq = bl_txq_sta_get(sta, tid, NULL, bl_hw);

	BL_DBG("requeue: txq->idx=%d, txq->status=%d, txq->ndev_idx=%d\n", txq->idx, txq->status, txq->ndev_idx);

#ifdef CONFIG_BL_AMSDUS_TX
    if (bl_amsdu_add_subframe(bl_hw, skb, sta, txq))
        return NETDEV_TX_OK;
#endif

    /* Retrieve the pointer to the Ethernet data */
    eth = (struct ethhdr *)skb->data;
    memcpy(tmp_eth.h_dest, eth->h_dest, ETH_ALEN);
    memcpy(tmp_eth.h_source, eth->h_source, ETH_ALEN);
    tmp_eth.h_proto = eth->h_proto;
	hdr_pads = sizeof(struct ethhdr);
    headroom  = sizeof(struct bl_txhdr);

	/* first we increase the area for tx descriptor */
	skb_push(skb, sizeof(struct txdesc_api));
	/* then we increase the area for sdio header*/
	skb_push(skb, sizeof(struct sdio_hdr));
	/* then we increase the area for tx header */
    skb_push(skb, headroom);

    txhdr = (struct bl_txhdr *)skb->data;
    sw_txhdr = kmem_cache_alloc(bl_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL))
        goto free;
    txhdr->sw_hdr = sw_txhdr;
    desc = &sw_txhdr->desc;

	/* the frame len contains sdio special and actual skb->data */
    frame_len = (u16)skb->len - headroom - sizeof(struct sdio_hdr)-sizeof(struct txdesc_api);

	sw_txhdr->hdr.type = BL_TYPE_DATA;
	sw_txhdr->hdr.len = frame_len + sizeof(struct txdesc_api);
	sw_txhdr->hdr.queue_idx = txq->hwq->id;
	sw_txhdr->hdr.reserved = 0x0;

    sw_txhdr->txq       = txq;
    sw_txhdr->frame_len = frame_len;
    sw_txhdr->bl_sta  = sta;
    sw_txhdr->bl_vif  = bl_vif;
    sw_txhdr->skb       = skb;
    sw_txhdr->headroom  = headroom;
    sw_txhdr->map_len   = skb->len - offsetof(struct bl_txhdr, hw_hdr);

#ifdef CONFIG_BL_AMSDUS_TX
    sw_txhdr->amsdu.len = 0;
    sw_txhdr->amsdu.nb = 0;
#endif
    // Fill-in the descriptor
    memcpy(&desc->host.eth_dest_addr, tmp_eth.h_dest, ETH_ALEN);
    memcpy(&desc->host.eth_src_addr, tmp_eth.h_source, ETH_ALEN);
    desc->host.ethertype = tmp_eth.h_proto;
	BL_DBG("dest_addr:%04x%04x%04x\n", desc->host.eth_dest_addr.array[0], desc->host.eth_dest_addr.array[1], desc->host.eth_dest_addr.array[2]);
	BL_DBG("src_addr:%04x%04x%04x\n", desc->host.eth_src_addr.array[0], desc->host.eth_src_addr.array[1], desc->host.eth_src_addr.array[2]);
	BL_DBG("desc->host.ethertype=0x%x\n", desc->host.ethertype);
    desc->host.staid = sta->sta_idx;
    desc->host.tid = tid;
	desc->host.host_hdr_pads = hdr_pads;
    if (unlikely(bl_vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN)) {
        desc->host.vif_idx = bl_vif->ap_vlan.master->vif_index;
    } else {
        desc->host.vif_idx = bl_vif->vif_index;
    }

    if (bl_vif->use_4addr && (sta->sta_idx < NX_REMOTE_STA_MAX))
        desc->host.flags = TXU_CNTRL_USE_4ADDR;
    else
        desc->host.flags = 0;

    if (bl_vif->wdev.iftype == NL80211_IFTYPE_MESH_POINT) {
        if (bl_vif->is_resending) {
            desc->host.flags |= TXU_CNTRL_MESH_FWD;
        }
    }

#ifdef CONFIG_BL_SPLIT_TX_BUF
    desc->host.packet_len[0] = frame_len;
#else
    desc->host.packet_len = frame_len;
#endif

    txhdr->hw_hdr.cfm.status.value = 0;

    /* Fill-in TX descriptor */
    frame_oft = sizeof(struct bl_txhdr) - offsetof(struct bl_txhdr, hw_hdr)
                + sizeof(*eth);
#ifdef CONFIG_BL_SPLIT_TX_BUF
    desc->host.packet_addr[0] = sw_txhdr->dma_addr + frame_oft;
    desc->host.packet_cnt = 1;
#else
    desc->host.packet_addr = sw_txhdr->dma_addr + frame_oft;
#endif
    //desc->host.status_desc_addr = sw_txhdr->dma_addr;

	BL_DBG("requeue total length[%lu], sdio_txdesc[%lu], payload[%u]\n", frame_len+sizeof(struct sdio_hdr)+sizeof(struct txdesc_api), sizeof(struct sdio_hdr)+sizeof(struct txdesc_api), frame_len);
	BL_DBG("requeue sdio_hdr: [%04x %04x %04x %04x]\n", sw_txhdr->hdr.type, sw_txhdr->hdr.len, sw_txhdr->hdr.queue_idx, sw_txhdr->hdr.reserved);
	BL_DBG("txdesc: %08x%08x%04x%08x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%04x%02x%02x%02x%04x\n", \
		desc->ready, desc->host.packet_addr, desc->host.packet_len, desc->host.host_hdr_pads, desc->host.eth_dest_addr.array[0], desc->host.eth_dest_addr.array[1], desc->host.eth_dest_addr.array[2], \
		desc->host.eth_src_addr.array[0], desc->host.eth_src_addr.array[1], desc->host.eth_src_addr.array[2],desc->host.ethertype, desc->host.pn[0], desc->host.pn[1], desc->host.pn[2], desc->host.pn[3], \
		desc->host.sn, desc->host.timestamp, desc->host.tid, desc->host.vif_idx, desc->host.staid, desc->host.flags);
	BL_DBG("requeue sw_txhdr: frame_len[%04x], headroom[%04x]\n", sw_txhdr->frame_len,sw_txhdr->headroom);
	BL_DBG("requeue hw_txhdr: staid[%02x]vif_idx[%02x]tid[%02x]flags[%04x]\n", desc->host.staid, desc->host.vif_idx, desc->host.tid, desc->host.flags);
	BL_DBG("requeue skb=%p\n", skb);
    spin_lock_bh(&bl_hw->txq_lock);
    if (bl_txq_queue_skb(skb, txq, bl_hw, false)) {
		BL_DBG("rwqueue multicast_skb success!\n");
        spin_unlock_bh(&bl_hw->txq_lock);
		//bl_queue_main_work(bl_hw);
    } else {
		BL_DBG("rwqueue multicast_skb success failed");
        spin_unlock_bh(&bl_hw->txq_lock);
		return NETDEV_TX_OK;
    }

    return NETDEV_TX_OK;

free:
    dev_kfree_skb_any(skb);

    return NETDEV_TX_OK;
}

/**
 * netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
 *                               struct net_device *dev);
 *	Called when a packet needs to be transmitted.
 *	Must return NETDEV_TX_OK , NETDEV_TX_BUSY.
 *        (can also return NETDEV_TX_LOCKED if NETIF_F_LLTX)
 *
 *  - Initialize the desciptor for this pkt (stored in skb before data)
 *  - Push the pkt in the corresponding Txq
 *  - If possible (i.e. credit available and not in PS) the pkt is pushed
 *    to fw
 */
netdev_tx_t bl_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct bl_txhdr *txhdr;
    struct bl_sw_txhdr *sw_txhdr;
    struct ethhdr *eth;
	struct ethhdr tmp_eth;
    struct txdesc_api *desc;
    struct bl_sta *sta;
    struct bl_txq *txq;
    int headroom;
    int max_headroom;
	int hdr_pads;

    u16 frame_len;
    u16 frame_oft;
    u8 tid;

	BL_DBG(BL_FN_ENTRY_STR);
	
    max_headroom = sizeof(struct bl_txhdr)+sizeof(struct txdesc_api) + sizeof(struct sdio_hdr);

    /* check whether the current skb can be used */
    if (skb_shared(skb) || (skb_headroom(skb) < max_headroom)) {
        struct sk_buff *newskb = skb_copy_expand(skb, max_headroom, 0,
                                                 GFP_ATOMIC);
        if (unlikely(newskb == NULL))
            goto free;

        dev_kfree_skb_any(skb);

        skb = newskb;
    }

    /* Get the STA id and TID information */
    sta = bl_get_tx_info(bl_vif, skb, &tid);
    if (!sta)
        goto free;

    txq = bl_txq_sta_get(sta, tid, NULL, bl_hw);

#ifdef CONFIG_BL_AMSDUS_TX
    if (bl_amsdu_add_subframe(bl_hw, skb, sta, txq))
        return NETDEV_TX_OK;
#endif

    /* Retrieve the pointer to the Ethernet data */
    eth = (struct ethhdr *)skb->data;
    memcpy(tmp_eth.h_dest, eth->h_dest, ETH_ALEN);
    memcpy(tmp_eth.h_source, eth->h_source, ETH_ALEN);
    tmp_eth.h_proto = eth->h_proto;

	hdr_pads = sizeof(struct ethhdr);	
    headroom  = sizeof(struct bl_txhdr);

	/* first we increase the area for tx descriptor */
	skb_push(skb, sizeof(struct txdesc_api));
	/* then we increase the area for sdio header*/
	skb_push(skb, sizeof(struct sdio_hdr));
	/* then we increase the area for tx header */
    skb_push(skb, headroom);

    txhdr = (struct bl_txhdr *)skb->data;
    sw_txhdr = kmem_cache_alloc(bl_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL))
        goto free;
    txhdr->sw_hdr = sw_txhdr;
    desc = &sw_txhdr->desc;

	/* the frame len contains sdio special and actual skb->data */
    frame_len = (u16)skb->len - headroom - sizeof(struct sdio_hdr)-sizeof(struct txdesc_api) - hdr_pads;

	sw_txhdr->hdr.type = BL_TYPE_DATA;
	sw_txhdr->hdr.len = skb->len - headroom;
	sw_txhdr->hdr.queue_idx = txq->hwq->id;
	sw_txhdr->hdr.reserved = g_pkt_sn;

	BL_DBG("xmit_pkt_sn: hw_idx=%d, frame_len=%u, sn=%u\n",txq->hwq->id, frame_len, g_pkt_sn);
	g_pkt_sn++;

    sw_txhdr->txq       = txq;
    sw_txhdr->frame_len = frame_len;
    sw_txhdr->bl_sta  = sta;
    sw_txhdr->bl_vif  = bl_vif;
    sw_txhdr->skb       = skb;
    sw_txhdr->headroom  = headroom;
    sw_txhdr->map_len   = skb->len - offsetof(struct bl_txhdr, hw_hdr);

#ifdef CONFIG_BL_AMSDUS_TX
    sw_txhdr->amsdu.len = 0;
    sw_txhdr->amsdu.nb = 0;
#endif
    // Fill-in the descriptor
    memcpy(&desc->host.eth_dest_addr, tmp_eth.h_dest, ETH_ALEN);
    memcpy(&desc->host.eth_src_addr, tmp_eth.h_source, ETH_ALEN);
    desc->host.ethertype = tmp_eth.h_proto;
    desc->host.staid = sta->sta_idx;
    desc->host.tid = tid;
	desc->host.host_hdr_pads = hdr_pads;
    if (unlikely(bl_vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN)) {
        desc->host.vif_idx = bl_vif->ap_vlan.master->vif_index;
    } else {
        desc->host.vif_idx = bl_vif->vif_index;
    }

    if (bl_vif->use_4addr && (sta->sta_idx < NX_REMOTE_STA_MAX))
        desc->host.flags = TXU_CNTRL_USE_4ADDR;
    else
        desc->host.flags = 0;

#ifdef CONFIG_BL_SPLIT_TX_BUF
    desc->host.packet_len[0] = frame_len;
#else
    desc->host.packet_len = frame_len;
#endif

    txhdr->hw_hdr.cfm.status.value = 0;

    /* Fill-in TX descriptor */
    frame_oft = sizeof(struct bl_txhdr) - offsetof(struct bl_txhdr, hw_hdr) + sizeof(*eth);
#ifdef CONFIG_BL_SPLIT_TX_BUF
    desc->host.packet_addr[0] = sw_txhdr->dma_addr + frame_oft;
    desc->host.packet_cnt = 1;
#else
    desc->host.packet_addr = sw_txhdr->dma_addr + frame_oft;
#endif

	BL_DBG_MOD_LEVEL(bl_hw, D_TX D_INF "bl_start_xmit: queue skb=%p on hwq->idx=%d, txq->idx=%d\n", skb, sw_txhdr->hdr.queue_idx, txq->idx);

	spin_lock_bh(&bl_hw->txq_lock);
    if (bl_txq_queue_skb(skb, txq, bl_hw, false)) {
		BL_DBG("start_xmit: queue_skb success!\n");
		bl_queue_main_work(bl_hw);
		spin_unlock_bh(&bl_hw->txq_lock);
    } else {
    	/*when in ps mode, it just add skb into txq, but donot add txq in hwq*/
		BL_DBG("start_xmit: queue_skb success failed");
		spin_unlock_bh(&bl_hw->txq_lock);
		return NETDEV_TX_OK;
    }

    return NETDEV_TX_OK;

free:
    dev_kfree_skb_any(skb);

    return NETDEV_TX_OK;
}

/**
 * bl_start_mgmt_xmit - Transmit a management frame
 *
 * @vif: Vif that send the frame
 * @sta: Destination of the frame. May be NULL if the destiantion is unknown
 *       to the AP.
 * @params: Mgmt frame parameters
 * @offchan: Indicate whether the frame must be send via the offchan TXQ.
 *           (is is redundant with params->offchan ?)
 * @cookie: updated with a unique value to identify the frame with upper layer
 *
 */
int bl_start_mgmt_xmit(struct bl_vif *vif, struct bl_sta *sta,
                         struct cfg80211_mgmt_tx_params *params, bool offchan,
                         u64 *cookie)
{
    struct bl_hw *bl_hw = vif->bl_hw;
    struct bl_txhdr *txhdr;
    struct bl_sw_txhdr *sw_txhdr;
    struct txdesc_api *desc;
    struct sk_buff *skb;
    u16 frame_len, headroom, frame_oft;
    u8 *data;
    struct bl_txq *txq;
    bool robust;
	int i;
	int hdr_pads;

	BL_DBG(BL_FN_ENTRY_STR);

	headroom = sizeof(struct bl_txhdr) + sizeof(struct txdesc_api) + sizeof(struct sdio_hdr);
    frame_len = params->len;

    /* Set TID and Queues indexes */
    if (sta) {
        txq = bl_txq_sta_get(sta, 8, NULL, bl_hw);
    } else {
        if (offchan) {
            txq = &bl_hw->txq[NX_OFF_CHAN_TXQ_IDX];
        } else {
            txq = bl_txq_vif_get(vif, NX_UNK_TXQ_TYPE, NULL);
		}
    }

    /* Ensure that TXQ is active */
    if (txq->idx == TXQ_INACTIVE) {
        netdev_dbg(vif->ndev, "TXQ inactive\n");
        return -EBUSY;
    }

    /* Create a SK Buff object that will contain the provided data */
    skb = dev_alloc_skb(headroom + frame_len);
    if (!skb) {
        return -ENOMEM;
    }


    *cookie = (unsigned long)skb;

    /*
     * Move skb->data pointer in order to reserve room for bl_txhdr
     * headroom value will be equal to sizeof(struct bl_txhdr)
     */
    skb_reserve(skb, headroom);

    /*
     * Extend the buffer data area in order to contain the provided packet
     * len value (for skb) will be equal to param->len
     */
    data = skb_put(skb, frame_len);
    /* Copy the provided data */
    memcpy(data, params->buf, frame_len);
    robust = ieee80211_is_robust_mgmt_frame(skb);

    /* Update CSA counter if present */
    if (unlikely(params->n_csa_offsets) &&
        vif->wdev.iftype == NL80211_IFTYPE_AP &&
        vif->ap.csa) {
        data = skb->data;
        for (i = 0; i < params->n_csa_offsets ; i++) {
            data[params->csa_offsets[i]] = vif->ap.csa->count;
        }
    }
	
	hdr_pads = BL_SWTXHDR_ALIGN_PADS((long)(skb->data)-sizeof(struct txdesc_api)-sizeof(struct sdio_hdr));

    /*
     * Go back to the beginning of the allocated data area
     * skb->data pointer will move backward
     */
    skb_push(skb, hdr_pads);
 	/* first we increase the area for tx descriptor */
	skb_push(skb, sizeof(struct txdesc_api));
	/* then we increase the area for sdio header*/
	skb_push(skb, sizeof(struct sdio_hdr));
	/* then we increase the area for tx header */
    skb_push(skb, headroom);

    /* Fill the TX Header */
    txhdr = (struct bl_txhdr *)skb->data;
    txhdr->hw_hdr.cfm.status.value = 0;

    /* Fill the SW TX Header */
    sw_txhdr = kmem_cache_alloc(bl_hw->sw_txhdr_cache, GFP_ATOMIC);
    if (unlikely(sw_txhdr == NULL)) {
        dev_kfree_skb(skb);
        return -ENOMEM;
    }
    txhdr->sw_hdr = sw_txhdr;

	sw_txhdr->hdr.type = BL_TYPE_DATA;
	sw_txhdr->hdr.len = frame_len + sizeof(struct txdesc_api);
	sw_txhdr->hdr.queue_idx = txq->hwq->id;
	sw_txhdr->hdr.reserved = g_pkt_sn;
	
	BL_DBG("xmit_pkt_sn: hw_idx=%d, frame_len=%u, sn=%u\n",txq->hwq->id, frame_len, g_pkt_sn);
	g_pkt_sn++;

    sw_txhdr->txq = txq;
    sw_txhdr->frame_len = frame_len;
    sw_txhdr->bl_sta = sta;
    sw_txhdr->bl_vif = vif;
    sw_txhdr->skb = skb;
    sw_txhdr->headroom = headroom;
    sw_txhdr->map_len = skb->len - offsetof(struct bl_txhdr, hw_hdr);
#ifdef CONFIG_BL_AMSDUS_TX
    sw_txhdr->amsdu.len = 0;
    sw_txhdr->amsdu.nb = 0;
#endif

    /* Fill the Descriptor to be provided to the MAC SW */
    desc = &sw_txhdr->desc;

    desc->host.staid = (sta) ? sta->sta_idx : 0xFF;
    desc->host.vif_idx = vif->vif_index;
    desc->host.tid = 0xFF;
	desc->host.host_hdr_pads = hdr_pads;
    desc->host.flags = TXU_CNTRL_MGMT;
    if (robust)
        desc->host.flags |= TXU_CNTRL_MGMT_ROBUST;

#ifdef CONFIG_BL_SPLIT_TX_BUF
    desc->host.packet_len[0] = frame_len;
#else
    desc->host.packet_len = frame_len;
#endif

    if (params->no_cck) {
        desc->host.flags |= TXU_CNTRL_MGMT_NO_CCK;
    }

    frame_oft = sizeof(struct bl_txhdr) - offsetof(struct bl_txhdr, hw_hdr);
#ifdef CONFIG_BL_SPLIT_TX_BUF
    desc->host.packet_addr[0] = sw_txhdr->dma_addr + frame_oft;
    desc->host.packet_cnt = 1;
#else
    desc->host.packet_addr = sw_txhdr->dma_addr + frame_oft;
#endif

	BL_DBG("mgmt_xmit: queue_skb=%p\n", skb);
	spin_lock_bh(&bl_hw->txq_lock);
	if (bl_txq_queue_skb(skb, txq, bl_hw, false)) {
		BL_DBG("mgmt_xmit: queue_skb success!\n");
		bl_queue_main_work(bl_hw);
		spin_unlock_bh(&bl_hw->txq_lock);
    } else {
    	/*when in ps mode, it just add skb into txq, but donot add txq in hwq*/
		BL_DBG("mgmt_xmit: queue_skb success failed");
		spin_unlock_bh(&bl_hw->txq_lock);
		return NETDEV_TX_OK;
    }

    return 0;
}

/**
 * bl_txdatacfm - FW callback for TX confirmation
 *
 * called with tx_lock hold
 */
int bl_txdatacfm(void *pthis, void *host_id, void *_hw_hdr, void **txq_saved)
{
    struct bl_hw *bl_hw = (struct bl_hw *)pthis;
	struct bl_hw_txhdr *hw_hdr = (struct bl_hw_txhdr *)_hw_hdr;
    struct sk_buff *skb = host_id;
    struct bl_txhdr *txhdr;
    union bl_hw_txstatus bl_txst;
    struct bl_sw_txhdr *sw_txhdr;
    struct bl_hwq *hwq;
    struct bl_txq *txq;

    txhdr = (struct bl_txhdr *)skb->data;
    sw_txhdr = txhdr->sw_hdr;
	bl_txst = hw_hdr->cfm.status;

	memcpy(&txhdr->hw_hdr, hw_hdr, sizeof(struct bl_hw_txhdr));

    txq = sw_txhdr->txq;
	*txq_saved = txq;
    /* don't use txq->hwq as it may have changed between push and confirm */
    hwq = &bl_hw->hwq[sw_txhdr->hw_queue];
	if(hwq == NULL) {
		printk("bl_txdatacfm: hwq is NULL!\n");
		return -1;
	}

    bl_txq_confirm_any(bl_hw, txq, hwq, sw_txhdr);

    /* Update txq and HW queue credits */
    if (sw_txhdr->desc.host.flags & TXU_CNTRL_MGMT) {
        /* For debug purpose (use ftrace kernel option) */
        trace_mgmt_cfm(sw_txhdr->bl_vif->vif_index,
                       (sw_txhdr->bl_sta) ? sw_txhdr->bl_sta->sta_idx : 0xFF,
                       !(bl_txst.retry_required || bl_txst.sw_retry_required));

        /* Confirm transmission to CFG80211 */
        cfg80211_mgmt_tx_status(&sw_txhdr->bl_vif->wdev,
                                (unsigned long)skb,
                                (skb->data + sw_txhdr->headroom + sizeof(struct txdesc_api) + sizeof(struct sdio_hdr)),
                                sw_txhdr->frame_len,
                                !(bl_txst.retry_required || bl_txst.sw_retry_required),
                                GFP_ATOMIC);
    } else if ((txq->idx != TXQ_INACTIVE) &&
               (bl_txst.retry_required || bl_txst.sw_retry_required)) {
        bool sw_retry = (bl_txst.sw_retry_required) ? true : false;

        /* Reset the status */
        txhdr->hw_hdr.cfm.status.value = 0;

        /* The confirmed packet was part of an AMPDU and not acked
         * correctly, so reinject it in the TX path to be retried */
        bl_tx_retry(bl_hw, skb, txhdr, sw_retry);
        return 0;
    }

    /* Update statistics */
    sw_txhdr->bl_vif->net_stats.tx_packets++;
    sw_txhdr->bl_vif->net_stats.tx_bytes += sw_txhdr->frame_len;

    /* Release SKBs */
#ifdef CONFIG_BL_AMSDUS_TX
    if (sw_txhdr->desc.host.flags & TXU_CNTRL_AMSDU) {
        struct bl_amsdu_txhdr *amsdu_txhdr;
        list_for_each_entry(amsdu_txhdr, &sw_txhdr->amsdu.hdrs, list) {
            bl_amsdu_del_subframe_header(amsdu_txhdr);
            dma_unmap_single(bl_hw->dev, amsdu_txhdr->dma_addr,
                             amsdu_txhdr->map_len, DMA_TO_DEVICE);
            consume_skb(amsdu_txhdr->skb);
        }
    }
#endif /* CONFIG_BL_AMSDUS_TX */

    kmem_cache_free(bl_hw->sw_txhdr_cache, sw_txhdr);
    skb_pull(skb, sw_txhdr->headroom);
	BL_DBG("txdata_cfm, free skb=%p, sn=%u\n", skb, hw_hdr->cfm.sn);
    consume_skb(skb);

    return 0;
}

/**
 * bl_txq_credit_update - Update credit for one txq
 *
 * @bl_hw: Driver main data
 * @sta_idx: STA idx
 * @tid: TID
 * @update: offset to apply in txq credits
 *
 * Called when fw send ME_TX_CREDITS_UPDATE_IND message.
 * Apply @update to txq credits, and stop/start the txq if needed
 */
void bl_txq_credit_update(struct bl_hw *bl_hw, int sta_idx, u8 tid,
                            s8 update)
{
    struct bl_sta *sta = &bl_hw->sta_table[sta_idx];
    struct bl_txq *txq;

    txq = bl_txq_sta_get(sta, tid, NULL, bl_hw);

    spin_lock(&bl_hw->tx_lock);

    if (txq->idx != TXQ_INACTIVE) {
        txq->credits += update;
        trace_credit_update(txq, update);
  /*      if (txq->credits <= 0)
            bl_txq_stop(txq, BL_TXQ_STOP_FULL);
        else
            bl_txq_start(txq, BL_TXQ_STOP_FULL);*/
    }

    spin_unlock(&bl_hw->tx_lock);
}
