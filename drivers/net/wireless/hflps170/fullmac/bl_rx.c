/**
 ******************************************************************************
 *
 * @file bl_rx.c
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>

#include "bl_defs.h"
#include "bl_rx.h"
#include "bl_tx.h"
#include "bl_events.h"
#include "bl_compat.h"

const u8 legrates_lut[] = {
    0,                          /* 0 */
    1,                          /* 1 */
    2,                          /* 2 */
    3,                          /* 3 */
    -1,                         /* 4 */
    -1,                         /* 5 */
    -1,                         /* 6 */
    -1,                         /* 7 */
    10,                         /* 8 */
    8,                          /* 9 */
    6,                          /* 10 */
    4,                          /* 11 */
    11,                         /* 12 */
    9,                          /* 13 */
    7,                          /* 14 */
    5                           /* 15 */
};


/**
 * bl_rx_statistic - save some statistics about received frames
 *
 * @bl_hw: main driver data.
 * @hw_rxhdr: Rx Hardware descriptor of the received frame.
 * @sta: STA that sent the frame.
 */
static void bl_rx_statistic(struct bl_hw *bl_hw, struct hw_rxhdr *hw_rxhdr,
                              struct bl_sta *sta)
{
#ifdef CONFIG_BL_DEBUGFS
    struct bl_stats *stats = &bl_hw->stats;
    struct bl_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
    struct hw_vect *hwvect = &hw_rxhdr->hwvect;
    int mpdu, ampdu, mpdu_prev, rate_idx;

    /* save complete hwvect */
    sta->stats.last_rx = *hwvect;

    /* update ampdu rx stats */
    mpdu = hwvect->mpdu_cnt;
    ampdu = hwvect->ampdu_cnt;
    mpdu_prev = stats->ampdus_rx_map[ampdu];

    if (mpdu_prev < mpdu ) {
        stats->ampdus_rx_miss += mpdu - mpdu_prev - 1;
    } else {
        stats->ampdus_rx[mpdu_prev]++;
    }
    stats->ampdus_rx_map[ampdu] = mpdu;

    /* update rx rate statistic */
    if (hwvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) {
        int mcs = hwvect->mcs;
        int bw = hwvect->ch_bw;
        int sgi = hwvect->short_gi;
        int nss;
        if (hwvect->format_mod <= FORMATMOD_HT_GF) {
            nss = hwvect->stbc ? hwvect->stbc : hwvect->n_sts;
            rate_idx = 16 + nss * 32 + mcs * 4 +  bw * 2 + sgi;
        } else {
            nss = hwvect->stbc ? hwvect->stbc/2 : hwvect->n_sts;
            rate_idx = 144 + nss * 80 + mcs * 8 + bw * 2 + sgi;
        }
    } else {
        int idx = legrates_lut[hwvect->leg_rate];
        if (idx < 4) {
            rate_idx = idx * 2 + hwvect->pre_type;
        } else {
            rate_idx = 8 + idx - 4;
        }
    }
    if (rate_idx < rate_stats->size) {
        rate_stats->table[rate_idx]++;
        rate_stats->cpt++;
    } else {
        pr_err("Invalid index conversion => %d / %d\n", rate_idx, rate_stats->size);
    }
#endif
}
							  
  static const u16 bl_1d_to_wmm_queue[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };

u16 bl_recal_priority_netdevidx(struct bl_vif *bl_vif, struct sk_buff *skb)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct wireless_dev *wdev = &bl_vif->wdev;
    struct bl_sta *sta = NULL;
    struct bl_txq *txq;
    u16 netdev_queue;
    bool tdls_mgmgt_frame = false;

	BL_DBG(BL_FN_ENTRY_STR);

    switch (wdev->iftype) {
    case NL80211_IFTYPE_STATION:
    {
        struct ethhdr *eth;
        eth = (struct ethhdr *)skb->data;
        if (eth->h_proto == cpu_to_be16(ETH_P_TDLS)) {
            tdls_mgmgt_frame = true;
        }
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
    {
        struct bl_sta *cur;
        struct ethhdr *eth = (struct ethhdr *)skb->data;

        if (is_multicast_ether_addr(eth->h_dest)) {
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

    if (sta && sta->qos)
    {
        /* use the data classifier to determine what 802.1d tag the data frame has */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
        skb->priority = cfg80211_classify8021d(skb, NULL) & IEEE80211_QOS_CTL_TAG1D_MASK;
#else
        skb->priority = cfg80211_classify8021d(skb) & IEEE80211_QOS_CTL_TAG1D_MASK;
#endif
        if (sta->acm)
            bl_downgrade_ac(sta, skb);

        txq = bl_txq_sta_get(sta, skb->priority, NULL, bl_hw);
        netdev_queue = txq->ndev_idx;
    }
    else if (sta)
    {
        skb->priority = 0xFF;
        txq = bl_txq_sta_get(sta, 0, NULL, bl_hw);
        netdev_queue = txq->ndev_idx;
    }
    else
    {
        /* This packet will be dropped in xmit function, still need to select
           an active queue for xmit to be called. As it most likely to happen
           for AP interface, select BCMC queue
           (TODO: select another queue if BCMC queue is stopped) */
        skb->priority = 0xAA;
        netdev_queue = NX_BCMC_TXQ_NDEV_IDX;
    }

    BUG_ON(netdev_queue >= NX_NB_NDEV_TXQ);
	
    if(skb->priority < 8)
        return bl_1d_to_wmm_queue[skb->priority];
	else
        return 0;

//    return netdev_queue;
}

/**
 * bl_rx_data_skb - Process one data frame
 *
 * @bl_hw: main driver data
 * @bl_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 * @return: true if buffer has been forwarded to upper layer
 *
 * If buffer is amsdu , it is first split into a list of skb.
 * Then each skb may be:
 * - forwarded to upper layer
 * - resent on wireless interface
 *
 * When vif is a STA interface, every skb is only forwarded to upper layer.
 * When vif is an AP interface, multicast skb are forwarded and resent, whereas
 * skb for other BSS's STA are only resent.
 */
static bool bl_rx_data_skb(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                             struct sk_buff *skb,  struct hw_rxhdr *rxhdr)
{
    struct sk_buff_head list;
    struct sk_buff *rx_skb;
	struct bl_sta *sta = NULL;
    u16 netdev_queue;
    bool amsdu = rxhdr->flags_is_amsdu;
    bool resend = false, forward = true;

    skb->dev = bl_vif->ndev;

    __skb_queue_head_init(&list);

    if (amsdu) {
        int count;

        ieee80211_amsdu_to_8023s(skb, &list, bl_vif->ndev->dev_addr,
                                 BL_VIF_TYPE(bl_vif), 0, NULL, NULL);

        count = skb_queue_len(&list);
        if (count > ARRAY_SIZE(bl_hw->stats.amsdus_rx))
            count = ARRAY_SIZE(bl_hw->stats.amsdus_rx);
        bl_hw->stats.amsdus_rx[count - 1]++;
    } else {
        bl_hw->stats.amsdus_rx[0]++;
        __skb_queue_head(&list, skb);
    }

    if (((BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP) ||
         (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN)) &&
        !(bl_vif->ap.flags & BL_AP_ISOLATE)) {
        const struct ethhdr *eth;
        rx_skb = skb_peek(&list);
        if(rx_skb == NULL)
            printk("rx_skb=NULL!!!\n");
		/*go through mac_header, after mac_header is eth header*/
        skb_reset_mac_header(rx_skb);
        eth = eth_hdr(rx_skb);

        if (unlikely(is_multicast_ether_addr(eth->h_dest))) {
            /* broadcast pkt need to be forwared to upper layer and resent
               on wireless interface */
			BL_DBG("this is a broadcast packet, forwared true and resend true!\n");
            resend = true;
        } else {
            /* unicast pkt for STA inside the BSS, no need to forward to upper
               layer simply resend on wireless interface */
            if (rxhdr->flags_dst_idx != 0xFF)
            {
				BL_DBG("unicast pkt for STA inside the BSS, and forward false and resend true\n");
                sta = &bl_hw->sta_table[rxhdr->flags_dst_idx];
                if (sta->valid && (sta->vlan_idx == bl_vif->vif_index))
                {
                    forward = false;
                    resend = true;
                }
            }
        }
    }

    while (!skb_queue_empty(&list)) {
        rx_skb = __skb_dequeue(&list);
		BL_DBG("resend rx_skb->priority=%d, ndev_idx=%d\n", rx_skb->priority, skb_get_queue_mapping(rx_skb));

		resend = false;
        /* resend pkt on wireless interface */
        if (resend) {
            struct sk_buff *skb_copy;
            /* always need to copy buffer when forward=0 to get enough headrom for tsdesc */
            skb_copy = skb_copy_expand(rx_skb, sizeof(struct bl_txhdr) + sizeof(struct txdesc_api) + sizeof(struct sdio_hdr) +
                                       BL_SWTXHDR_ALIGN_SZ, 0, GFP_ATOMIC);
            if (skb_copy) {
                int res;
                skb_copy->protocol = htons(ETH_P_802_3);
                skb_reset_network_header(skb_copy);
                skb_reset_mac_header(skb_copy);

				BL_DBG("resend skb_copy=%p, skb len=0x%02x, skb->priority=%d, ndev_idx=%d\n", skb_copy, skb_copy->len, skb_copy->priority, skb_get_queue_mapping(skb_copy));

                bl_vif->is_resending = true;

				netdev_queue = bl_recal_priority_netdevidx(bl_vif, skb_copy);

				skb_set_queue_mapping(skb_copy, netdev_queue);

				BL_DBG("after recal: skb->priority=%d, netdev_queue=%d\n", skb_copy->priority, netdev_queue);

				bl_hw->resend = true;
                //res = dev_queue_xmit(skb_copy);
                res = bl_requeue_multicast_skb(skb_copy, bl_vif);
                bl_vif->is_resending = false;
                /* note: buffer is always consummed by dev_queue_xmit */
                if (res == NET_XMIT_DROP) {
                    bl_vif->net_stats.rx_dropped++;
                    bl_vif->net_stats.tx_dropped++;
                } else if (res != NET_XMIT_SUCCESS) {
                    netdev_err(bl_vif->ndev,
                               "Failed to re-send buffer to driver (res=%d)",
                               res);
                    bl_vif->net_stats.tx_errors++;
                }
            } else {
                netdev_err(bl_vif->ndev, "Failed to copy skb");
            }
        }

        /* forward pkt to upper layer */
        if (forward) {
            rx_skb->protocol = eth_type_trans(rx_skb, bl_vif->ndev);
            memset(rx_skb->cb, 0, sizeof(rx_skb->cb));

            netif_receive_skb(rx_skb);

            /* Update statistics */
            bl_vif->net_stats.rx_packets++;
            bl_vif->net_stats.rx_bytes += rx_skb->len;
			BL_DBG("forward this packet to network layer success\n");
        }
    }

    return forward;
}

/**
 * bl_rx_mgmt - Process one 802.11 management frame
 *
 * @bl_hw: main driver data
 * @bl_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 *
 * Process the management frame and free the corresponding skb
 */
static void bl_rx_mgmt(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                         struct sk_buff *skb,  struct hw_rxhdr *hw_rxhdr)
{
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;

    trace_mgmt_rx(hw_rxhdr->phy_prim20_freq, hw_rxhdr->flags_vif_idx,
                  hw_rxhdr->flags_sta_idx, mgmt);

    if (ieee80211_is_beacon(mgmt->frame_control)) {
            cfg80211_report_obss_beacon(bl_hw->wiphy, skb->data, skb->len,
                                        hw_rxhdr->phy_prim20_freq,
                                        hw_rxhdr->hwvect.rssi1);
    } else if ((ieee80211_is_deauth(mgmt->frame_control) ||
                ieee80211_is_disassoc(mgmt->frame_control)) &&
               (mgmt->u.deauth.reason_code == WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA ||
                mgmt->u.deauth.reason_code == WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA)) {
        cfg80211_rx_unprot_mlme_mgmt(bl_vif->ndev, skb->data, skb->len);
    } else if ((BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_STATION) &&
               (ieee80211_is_action(mgmt->frame_control) &&
                (mgmt->u.action.category == 6))) {
        struct cfg80211_ft_event_params ft_event;
        ft_event.target_ap = (uint8_t *)&mgmt->u.action + ETH_ALEN + 2;
        ft_event.ies = (uint8_t *)&mgmt->u.action + ETH_ALEN*2 + 2;
        ft_event.ies_len = hw_rxhdr->hwvect.len - (ft_event.ies - (uint8_t *)mgmt);
        ft_event.ric_ies = NULL;
        ft_event.ric_ies_len = 0;
        cfg80211_ft_event(bl_vif->ndev, &ft_event);
    } else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
        cfg80211_rx_mgmt(&bl_vif->wdev, hw_rxhdr->phy_prim20_freq,
                         hw_rxhdr->hwvect.rssi1, skb->data, skb->len, 0);
#else
        cfg80211_rx_mgmt(&bl_vif->wdev, hw_rxhdr->phy_prim20_freq,
                         hw_rxhdr->hwvect.rssi1, skb->data, skb->len, 0, GFP_ATOMIC);
#endif
    }
    dev_kfree_skb(skb);
}

/**
 * bl_rx_get_vif - Return pointer to the destination vif
 *
 * @bl_hw: main driver data
 * @vif_idx: vif index present in rx descriptor
 *
 * Select the vif that should receive this frame returns NULL if the destination
 * vif is not active.
 * If no vif is specified, this is probably a mgmt broadcast frame, so select
 * the first active interface
 */
static inline
struct bl_vif *bl_rx_get_vif(struct bl_hw * bl_hw, int vif_idx)
{
    struct bl_vif *bl_vif = NULL;

    if (vif_idx == 0xFF) {
        list_for_each_entry(bl_vif, &bl_hw->vifs, list) {
            if (bl_vif->up)
                return bl_vif;
        }
        return NULL;
    } else if (vif_idx < NX_VIRT_DEV_MAX) {
        bl_vif = bl_hw->vif_table[vif_idx];
        if (!bl_vif || !bl_vif->up)
            return NULL;
    }

    return bl_vif;
}

/**
 * bl_rxdataind - Process rx buffer
 *
 * @pthis: Pointer to the object attached to the IPC structure
 *         (points to struct bl_hw is this case)
 * @hostid: Address of the RX descriptor
 *
 * This function is called for each buffer received by the fw
 *
 */
u8 bl_rxdataind(void *pthis, void *hostid)
{
    u32 status;
	int sta_idx;
	int tid;
	u16 sn;
	bool found = false;

    struct bl_hw *bl_hw = (struct bl_hw *)pthis;
    struct hw_rxhdr *hw_rxhdr;
    struct bl_vif *bl_vif;
    struct sk_buff *skb = (struct sk_buff *)hostid;
	struct bl_agg_reord_pkt *reord_pkt = NULL;
	struct bl_agg_reord_pkt *reord_pkt_inst = NULL;

    int msdu_offset = sizeof(struct hw_rxhdr);

	/*status occupy 4 bytes*/
	memcpy(&status, skb->data, 4);
	skb_pull(skb, 4);

    /* check that frame is completely uploaded */
    if (!status) {
		printk("receive status error, status=0x%x\n", status);
        return -1;
    }

	BL_DBG("status---->:0x%x\n", status);

	if(status & RX_STAT_DELETE) {
		BL_DBG("staus->delete, just free skb\n");
		skb_push(skb, sizeof(struct sdio_hdr) + 4);
		dev_kfree_skb(skb);
		goto end;
	}
	
    /* Check if we need to forward the buffer */
    if (status & RX_STAT_FORWARD) {
        hw_rxhdr = (struct hw_rxhdr *)skb->data;
        bl_vif = bl_rx_get_vif(bl_hw, hw_rxhdr->flags_vif_idx);

		sn = hw_rxhdr->sn;
		tid = hw_rxhdr->tid;
		BL_DBG("RX_STATE_FORWARD: sn=%u, tid=%d\n", sn, tid);

        if (!bl_vif) {
            dev_err(bl_hw->dev, "Frame received but no active vif (%d)",
                    hw_rxhdr->flags_vif_idx);
            dev_kfree_skb(skb);
            goto end;
        }

        skb_pull(skb, msdu_offset);
		/*Now, skb->data pointed to the real payload*/

        if (hw_rxhdr->flags_is_80211_mpdu) {
			BL_DBG("receive mgmt packet: type=0x%x, subtype=0x%x\n", 
				BL_FC_GET_TYPE(((struct ieee80211_mgmt *)skb->data)->frame_control), BL_FC_GET_STYPE(((struct ieee80211_mgmt *)skb->data)->frame_control));
            bl_rx_mgmt(bl_hw, bl_vif, skb, hw_rxhdr);
        } else {
        	BL_DBG("receive data packet: type=0x%x, subtype=0x%x\n", 
					BL_FC_GET_TYPE(*((u8 *)(skb->data))), BL_FC_GET_STYPE(*((u8 *)(skb->data))));
			
            if (hw_rxhdr->flags_sta_idx != 0xff) {
                struct bl_sta *sta= &bl_hw->sta_table[hw_rxhdr->flags_sta_idx];

                bl_rx_statistic(bl_hw, hw_rxhdr, sta);

				//printk("sta->vlan_idx=%d, bl_vif->vif_index=%d\n", sta->vlan_idx, bl_vif->vif_index);
                if (sta->vlan_idx != bl_vif->vif_index) {
                    bl_vif = bl_hw->vif_table[sta->vlan_idx];
                    if (!bl_vif) {
						printk("cannot find the vif\n");
                        dev_kfree_skb(skb);
                        goto end;
                    }
                }

                if (hw_rxhdr->flags_is_4addr && !bl_vif->use_4addr) {
                    cfg80211_rx_unexpected_4addr_frame(bl_vif->ndev,
                                                       sta->mac_addr, GFP_ATOMIC);
                }
            }

			BL_DBG("original skb->priority=%d\n", skb->priority);
            skb->priority = 256 + hw_rxhdr->flags_user_prio;
			BL_DBG("hw_rxhdr->flags_user_prio=%d, skb->priority=%d, ndev_idx=%d\n", hw_rxhdr->flags_user_prio, skb->priority, skb_get_queue_mapping(skb));
            if (!bl_rx_data_skb(bl_hw, bl_vif, skb, hw_rxhdr)) {
                dev_kfree_skb(skb);
			}
        }
    }

	if (status & RX_STAT_ALLOC) {
		/*put skb into right list according sta_idx & tid*/
		hw_rxhdr = (struct hw_rxhdr *)skb->data;
		sta_idx = hw_rxhdr->flags_sta_idx;
		sn = hw_rxhdr->sn;
		tid = hw_rxhdr->tid;

		BL_DBG("RX_STATE_ALLOC: sta_idx=%d, sn=%u, tid=%d\n", sta_idx, sn, tid);

		//restore the skb and add into the tail of reorder list
		skb_push(skb, 4);

		/*construct the reorder pkt*/
		reord_pkt = kmem_cache_alloc(bl_hw->agg_reodr_pkt_cache, GFP_ATOMIC);
		if(reord_pkt == NULL) {
			printk("KMEM CACHE ALLOC failed\n");
			return -1;
		}

		reord_pkt->skb = skb;
		reord_pkt->sn = sn;

		if (!list_empty(&bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA + tid])) {
				list_for_each_entry(reord_pkt_inst, &bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA + tid], list) {
					BL_DBG("reord_pkt_inst->sn=%u, sn=%u\n", reord_pkt_inst->sn, sn);
					if(reord_pkt_inst->sn > sn) {
						BL_DBG("Add sn %u before sn %u\n", sn, reord_pkt_inst->sn);
						found = true;
						list_add(&(reord_pkt->list), reord_pkt_inst->list.prev);
						break;
					}
				}

				if(!found) {
					BL_DBG("not found, add sn %u to tail\n", sn);
					list_add_tail(&(reord_pkt->list), &bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA+tid]);		
				}
		} else {
			BL_DBG("list is empty, add sn %u in tail\n", sn);
			list_add_tail(&(reord_pkt->list), &bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA+tid]);		
		}
	}

  end:

    return 0;
}
