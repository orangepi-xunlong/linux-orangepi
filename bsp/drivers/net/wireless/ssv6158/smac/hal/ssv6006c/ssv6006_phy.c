/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// Include defines from config.mak to feed eclipse defines from ccflags-y
#ifdef ECLIPSE
#include <ssv_mod_conf.h>
#endif // ECLIPSE

#include <linux/version.h>

#if ((defined SSV_SUPPORT_SSV6006))

#include <linux/etherdevice.h>
#include <ssv6200.h>
#include "ssv6006_mac.h"
#include "ssv6006c/ssv6006C_reg.h"
#include "ssv6006c/ssv6006C_aux.h"
#include <smac/dev.h>
#include <smac/ssv_skb.h>
#include <hal.h>
#include "ssv6006_priv.h"
#include "ssv6006_priv_normal.h"
#include <ssvdevice/ssv_cmd.h>
#include <linux_80211.h>
#include "ssv_desc.h"
#include "turismo_common.h"
#include "ssv6006_priv.h"


#define DEBUG_MITIGATE_CCI


//const size_t ssv6006_tx_desc_length = sizeof(struct ssv6006_tx_desc);
//const size_t ssv6006_rx_desc_length = sizeof(struct ssv6006_rx_desc) + sizeof (struct ssv6006_rxphy_info);

static const u32 rc_b_data_rate[4] = {1000, 2000, 5500, 11000};
static const u32 rc_g_data_rate[8] = {6000, 9000, 12000, 18000, 24000, 36000, 48000, 54000};
static const u32 rc_n_ht20_lgi_data_rate[8] = { 6500, 13000, 19500, 26000, 39000,  52000,  58500,  65000};
static const u32 rc_n_ht20_sgi_data_rate[8] = { 7200, 14400, 21700, 28900, 43300,  57800,  65000,  72200};
static const u32 rc_n_ht40_lgi_data_rate[8] = {13500, 27000, 40500, 54000, 81000, 108000, 121500, 135000};
static const u32 rc_n_ht40_sgi_data_rate[8] = {15000, 30000, 45000, 60000, 90000, 120000, 135000, 150000};

#if 0
static bool ssv6006_get_tx_desc_last_rate(struct ssv6006_tx_desc *tx_desc, int idx)
{
    switch (idx) {
        case 0:
            return tx_desc->is_last_rate0;
        case 1:
            return tx_desc->is_last_rate1;
        case 2:
            return tx_desc->is_last_rate2;
        case 3:
            return tx_desc->is_last_rate3;
        default:
            return false;
    }
}

static u32 ssv6006_get_data_rates(struct ssv6006_rc_idx *rc_word)
{
    union  {
        struct ssv6006_rc_idx   rcword;
        u8 val;
    } u;
    u.rcword = *rc_word;

    switch (rc_word->phy_mode){
        case SSV6006RC_B_MODE:
            return rc_b_data_rate[rc_word->rate_idx];

        case SSV6006RC_G_MODE:
            return rc_g_data_rate[rc_word->rate_idx];

        case SSV6006RC_N_MODE:
            if (rc_word->long_short == SSV6006RC_LONG){
                if (rc_word->ht40 == SSV6006RC_HT40){
                    return rc_n_ht40_lgi_data_rate[rc_word->rate_idx];
                } else {
                    return rc_n_ht20_lgi_data_rate[rc_word->rate_idx];
                }
            } else {
                if (rc_word->ht40 == SSV6006RC_HT40){
                    return rc_n_ht40_sgi_data_rate[rc_word->rate_idx];
                } else {
                    return rc_n_ht20_sgi_data_rate[rc_word->rate_idx];
                }
            }

        default:
            printk(" %s: invalid rate control word %x\n", __func__, u.val);
            memset(rc_word, 0 , sizeof(struct ssv6006_rc_idx));
            return 1000;
    }
}

static void ssv6006_set_frame_duration(struct ssv_softc *sc, struct ieee80211_tx_info *info,
    struct ssv6006_rc_idx *drate_idx, struct ssv6006_rc_idx *crate_idx,
    u16 len, struct ssv6006_tx_desc *tx_desc, u32 *nav)
{
    //struct ieee80211_tx_rate *tx_drate;
    // frame_consume_time is obosulete.
    u32 frame_time=0, ack_time = 0;// , frame_consume_time=0;
    u32 drate_kbps=0, crate_kbps=0;
    u32 rts_cts_nav[SSV6006RC_MAX_RATE_SERIES] = {0, 0, 0, 0};
    u32 l_length[SSV6006RC_MAX_RATE_SERIES] = {0, 0, 0, 0};
    bool ctrl_short_preamble=false, is_sgi, is_ht40;
    bool is_ht, is_gf, do_rts_cts;
    int d_phy ,c_phy, i, mcsidx;
    struct ssv6006_rc_idx *drate, *crate;
    bool last_rate = false;

    for (i = 0; i < SSV6006RC_MAX_RATE_SERIES ;i++)
    {
        drate = &drate_idx[i];
        mcsidx = drate->rate_idx;
        is_sgi = drate->long_short;
        ctrl_short_preamble = drate->long_short;
	    is_ht40 = drate->ht40;
        is_ht = (drate->phy_mode == SSV6006RC_N_MODE);
        is_gf = drate->mf;
        drate_kbps = ssv6006_get_data_rates(drate);
        crate = &crate_idx[i];
        crate_kbps = ssv6006_get_data_rates(crate);
        frame_time = 0 ;
        ack_time = 0;
        *nav = 0;

        /* Calculate data frame transmission time (include SIFS) */
        if (is_ht) {
            frame_time = ssv6xxx_ht_txtime(mcsidx,
                    len, is_ht40, is_sgi, is_gf);
            d_phy = WLAN_RC_PHY_OFDM;//no need use this flags in n mode.
        } else {
            /**
             * Calculate frame transmission time for b/g mode:
             *     frame_time = TX_TIME(frame) + SIFS
             */
            if (drate->phy_mode == SSV6006RC_B_MODE)
                d_phy = WLAN_RC_PHY_CCK;
            else
                d_phy = WLAN_RC_PHY_OFDM;

            frame_time = ssv6xxx_non_ht_txtime(d_phy, drate_kbps,
                len, ctrl_short_preamble);
        }

        /* get control frame phy
         *n mode data frmaes also response g mode control frames.
         */

        if (crate->phy_mode == SSV6006RC_B_MODE)
            c_phy = WLAN_RC_PHY_CCK;
        else
            c_phy = WLAN_RC_PHY_OFDM;

        /**
            * Calculate NAV duration for data frame. The NAV can be classified
            * into the following cases:
            *    [1] NAV = 0 if the frame addr1 is MC/BC or ack_policy = no_ack
            *    [2] NAV = TX_TIME(ACK) + SIFS if non-A-MPDU frame
            *    [3] NAV = TX_TIME(BA) + SIFS if A-MPDU frame
            */
        if (tx_desc->unicast) {

            if (info->flags & IEEE80211_TX_CTL_AMPDU){
                ack_time = ssv6xxx_non_ht_txtime(c_phy,
                    crate_kbps, BA_LEN, crate->long_short);
            } else {
                ack_time = ssv6xxx_non_ht_txtime(c_phy,
                    crate_kbps, ACK_LEN, crate->long_short);
            }

           // printk("ack_time[%d] d_phy[%d] drate_kbp[%d] c_phy[%d] crate_kbps[%d] \n ctrl_short_preamble[%d]\n",
           //    ack_time, d_phy, drate_kbps, c_phy, crate_kbps, ctrl_short_preamble);

            /* to do ..... */
        }

        /**
            * Calculate NAV for RTS/CTS-to-Self frame if RTS/CTS-to-Self
            * is needed for the frame transmission:
            *       RTS_NAV = cts_time + frame_time + ack_time
            *       CTS_NAV = frame_time + ack_time
            */
        switch (i){
            case 0:
                do_rts_cts = tx_desc->do_rts_cts0;
                break;
            case 1:
                do_rts_cts = tx_desc->do_rts_cts1;
                break;
            case 2:
                do_rts_cts = tx_desc->do_rts_cts2;
                break;
            case 3:
                do_rts_cts = tx_desc->do_rts_cts3;
                break;
            default:
                do_rts_cts = tx_desc->do_rts_cts0;
                break;
        }
        if (do_rts_cts & IEEE80211_TX_RC_USE_RTS_CTS) {
            rts_cts_nav[i] = frame_time;
            rts_cts_nav[i] += ack_time;
            rts_cts_nav[i] += ssv6xxx_non_ht_txtime(c_phy,
                crate_kbps, CTS_LEN, crate->long_short);

            /**
                    * frame consume time:
                    *     TxTime(RTS) + SIFS + TxTime(CTS) + SIFS + TxTime(DATA)
                    *     + SIFS + TxTime(ACK)
                    */
            //frame_consume_time = rts_cts_nav;
            // frame_consume_time += ssv6xxx_non_ht_txtime(c_phy,
            //     crate_kbps, RTS_LEN, ctrl_short_preamble);
        }else if (do_rts_cts & IEEE80211_TX_RC_USE_CTS_PROTECT) {
            rts_cts_nav[i] = frame_time;
            rts_cts_nav[i] += ack_time;

            /**
                    * frame consume time:
                    *     TxTime(CTS) + SIFS + TxTime(DATA) + SIFS + TxTime(ACK)
                    */
           // frame_consume_time = rts_cts_nav;
           // frame_consume_time += ssv6xxx_non_ht_txtime(c_phy,
           //     crate_kbps, CTS_LEN, ctrl_short_preamble);
        } else{
            rts_cts_nav[i] = 0;
        }

        /* Calculate L-Length if using HT mode */
        if (is_ht) {
            /**
                    * Calculate frame transmission time & L-Length if the
                    * frame is transmitted using HT-MF/HT-GF format:
                    *
                    *  [1]. ceil[TXTIME-T_SIGEXT-20)/4], plus 3 cause
                    *         we need to get ceil
                    *  [2]. ceil[TXTIME-T_SIGEXT-20]/4]*3 -3
                    */
            l_length[i] = frame_time - HT_SIFS_TIME;
            l_length[i] = ((l_length[i]-(HT_SIGNAL_EXT+20))+3)>>2;
            l_length[i] += ((l_length[i]<<1) - 3);
        } else {
            l_length[i] = 0;
        }
        *nav++ = ack_time ;

        last_rate = ssv6006_get_tx_desc_last_rate(tx_desc, i);
        if (last_rate)
            break;
    }

    tx_desc->rts_cts_nav0 = rts_cts_nav[0];
    tx_desc->rts_cts_nav1 = rts_cts_nav[1];
    tx_desc->rts_cts_nav2 = rts_cts_nav[2];
    tx_desc->rts_cts_nav3 = rts_cts_nav[3];

    //tx_desc->frame_consume_time = (frame_consume_time>>5)+1;;
    tx_desc->dl_length0 = l_length[0];
    tx_desc->dl_length1 = l_length[1];
    tx_desc->dl_length2 = l_length[2];
    tx_desc->dl_length3 = l_length[3];
}
#endif

static void ssv6006_update_txinfo (struct ssv_softc *sc, struct sk_buff *skb)
{
    struct ieee80211_hdr            *hdr;
    struct ieee80211_tx_info        *info = IEEE80211_SKB_CB(skb);
    struct ieee80211_vif            *vif = info->control.vif;
    struct ieee80211_sta            *sta = NULL;
    struct ssv_sta_info             *sta_info = NULL;/* Only protect sta_info when sta is not NULL. */
    struct ssv_sta_priv_data        *ssv_sta_priv = NULL;
    struct ssv_vif_priv_data        *vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
    struct ssv6006_tx_desc          *tx_desc = (struct ssv6006_tx_desc *)skb->data;
    int                              hw_txqid = 0;
    struct SKB_info_st              *skb_info = (struct SKB_info_st *)skb->head;
    static int                       ack_ctl_idx = 0;

    sta = skb_info->sta;
    hdr = (struct ieee80211_hdr *)(skb->data + TXPB_OFFSET);

    /**
     * Note that the 'sta' may be NULL. In case of the NULL condition,
     * we assign WSID to 0x0F always. The NULL condition always
     * happens before associating to an AP.
     */
    if (sta) {
        ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
        sta_info = ssv_sta_priv->sta_info;
        down_read(&sc->sta_info_sem);/* START protect sta_info */
    }

    /**
     * Decide frame tid & hardware output queue for outgoing
     * frames. Management frames have a dedicate output queue
     * with higher priority in station mode.
     */
    if (ieee80211_is_mgmt(hdr->frame_control) ||
             ieee80211_is_nullfunc(hdr->frame_control) ||
             ieee80211_is_qos_nullfunc(hdr->frame_control))) {
        hw_txqid = 4;
    } else if(info->flags & IEEE80211_TX_CTL_SEND_AFTER_DTIM){
        /* In AP mode we use queue 4 to send broadcast frame,
           when more than one station in sleep mode */
        hw_txqid = 4;
    } else {
        /* The skb_get_queue_mapping() returns AC */
        hw_txqid = sc->tx.hw_txqid[skb_get_queue_mapping(skb)];
    }

    /**
     * Generate tx info (tx descriptor) in M2 format for outgoing frames.
     * The software MAC of ssv6200 uses M2 format.
     */
    tx_desc->len = skb->len;
    //tx_desc->len = skb->len - sc->sh->tx_desc_len; // Exclude TX descriptor length
    tx_desc->c_type = M2_TXREQ;
    tx_desc->f80211 = 1;
    tx_desc->qos = (ieee80211_is_data_qos(hdr->frame_control))? 1: 0;
    tx_desc->use_4addr = (ieee80211_has_a4(hdr->frame_control))? 1: 0;

    tx_desc->more_data = (ieee80211_has_morefrags(hdr->frame_control))? 1: 0;
    tx_desc->stype_b5b4 = (cpu_to_le16(hdr->frame_control)>>4)&0x3;

    tx_desc->frag = (tx_desc->more_data||(hdr->seq_ctrl&0xf))? 1: 0;
    tx_desc->unicast = (is_multicast_ether_addr(hdr->addr1)) ? 0: 1;
    tx_desc->security = 0;

    /* ToDo Liam tx_burst is obsolete. Should re-consider about this ???*/
    //tx_desc->tx_burst = (tx_desc->frag)? 1: 0;

    if (vif == NULL)
        tx_desc->bssidx = 0;
    else {
        vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
        tx_desc->bssidx = vif_priv->vif_idx;
    }

    tx_desc->wsid = (!sta_info || (sta_info->hw_wsid < 0)) ? 0x0F : sta_info->hw_wsid;
    tx_desc->txq_idx = hw_txqid;
    tx_desc->hdr_offset = TXPB_OFFSET; //SSV6XXX_TX_DESC_LEN
    tx_desc->hdr_len = ssv6xxx_frame_hdrlen(hdr, tx_desc->ht);

    /* ToDo Liam payload_offset is obsolet, should re-consider about this??*/
    //tx_desc->payload_offset = tx_desc->hdr_offset + tx_desc->hdr_len;

    /* for eapol, it need to check ack */
    if (ssv6xxx_is_eapol_frame(sc, hdr)) {
        ack_ctl_idx++;
        tx_desc->sw_ack_ctl = 1;
        tx_desc->sw_ack_seq = ack_ctl_idx;
        if (64 == ack_ctl_idx)
            ack_ctl_idx = 0;
    }
    if(ieee80211_is_nullfunc(hdr->frame_control)||ieee80211_is_qos_nullfunc(hdr->frame_control))
    {
        ack_ctl_idx++;
        tx_desc->sw_ack_ctl = 1;
        tx_desc->sw_ack_seq = ack_ctl_idx;
        if (64 == ack_ctl_idx)
            ack_ctl_idx = 0;
    }
    /* set ampdu ctl flag, FW should do ampdu aggregation */
    if (info->flags & IEEE80211_TX_CTL_AMPDU)
        tx_desc->sw_ampdu_ctl = true;
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,0,8)
    /* set ampdu ctl flag, FW should not use cck rate */
    if (info->flags & IEEE80211_TX_CTL_NO_CCK_RATE)
        tx_desc->sw_no_cck = true;
#endif
    /* set RTS according to upper layer setting*/
    /* Check for RTS protection */
    if (tx_desc->unicast && ieee80211_is_data(hdr->frame_control)) {
    #if 0
        if (sc->hw->wiphy->rts_threshold != (u32) -1) {
            if ((skb->len - sc->sh->tx_desc_len) > sc->hw->wiphy->rts_threshold) {
                // Enable rts/cts flag, FW should send rts/cts by flag
                tx_desc->sw_force_rts = true;
            }
        }
    #endif
        /* CTS-to-self */
        if (sc->sc_flags & SC_OP_CTS_PROT) {
            // Enable rts/cts flag, FW should send rts/cts by flag
            tx_desc->sw_force_cts2self = true;
        }
    }

    tx_desc->fCmdIdx = 0;
    // Packet command flow
    //  - TX queue is always the last one.
    tx_desc->fCmd = (hw_txqid+M_ENG_TX_EDCA0);
    tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_CPU;

    // Check if need HW encryption
    if ((vif != NULL) &&
        ieee80211_has_protected(hdr->frame_control) &&
        (ieee80211_is_data_qos(hdr->frame_control) || ieee80211_is_data(hdr->frame_control)))
    {
        vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
        if (   (tx_desc->unicast && ssv_sta_priv && ssv_sta_priv->has_hw_encrypt)
            || (!tx_desc->unicast && vif_priv && vif_priv->has_hw_encrypt))
        {
            // For multicast frame, find first STA's WSID for group key.
            if (!tx_desc->unicast && !list_empty(&vif_priv->sta_list))
            {
                struct ssv_sta_priv_data *one_sta_priv;
                int                       hw_wsid;
                one_sta_priv = list_first_entry(&vif_priv->sta_list, struct ssv_sta_priv_data, list);
                hw_wsid = one_sta_priv->sta_info->hw_wsid;
                if (hw_wsid != (-1))
                {
                    tx_desc->wsid = hw_wsid;
                }
            }
            tx_desc->security = 1;
            tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_ENCRYPT;
            tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_MIC;
        }
    }
    //  - HCI is always at the first position.
    tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_HWHCI;

    if (sta) {
        up_read(&sc->sta_info_sem);/* END protect sta_info */
    }
}

static void ssv6006_dump_ssv6006_txdesc(struct sk_buff *skb)
{   // dump ssv6006_tx_desc structure.
    struct ssv6006_tx_desc *tx_desc;
    int s;
    u8 *dat;

    tx_desc = (struct ssv6006_tx_desc *)skb->data;

    printk("\n>> TX desc:\n");
    for(s = 0, dat= (u8 *)skb->data; s < sizeof(struct ssv6006_tx_desc) /4; s++) {
        printk("%02x%02x%02x%02x ", dat[4*s+3], dat[4*s+2], dat[4*s+1], dat[4*s]);
        if (((s+1)& 0x03) == 0)
            printk("\n");
    }
}

static void ssv6006_dump_ssv6006_txframe(struct sk_buff *skb)
{
    struct ssv6006_tx_desc *tx_desc;
    int s;
    u8 *dat;

    tx_desc = (struct ssv6006_tx_desc *)skb->data;
    printk(">> Tx Frame:\n");
    for(s = 0, dat = skb->data; s < (tx_desc->len - TXPB_OFFSET); s++) {
        printk("%02x ", dat[TXPB_OFFSET+s]);
        if (((s+1)& 0x0F) == 0)
            printk("\n");
    }

}

static void ssv6006_dump_tx_desc(struct sk_buff *skb)
{
    struct ssv6006_tx_desc *tx_desc;

    tx_desc = (struct ssv6006_tx_desc *)skb->data;

    ssv6006_dump_ssv6006_txdesc(skb);

    printk("\nlength: %d, c_type=%d, f80211=%d, qos=%d, ht=%d, use_4addr=%d, sec=%d\n",
        tx_desc->len, tx_desc->c_type, tx_desc->f80211, tx_desc->qos, tx_desc->ht,
        tx_desc->use_4addr, tx_desc->security);
    printk("more_data=%d, sub_type=%x, extra_info=%d, aggr = %d\n", tx_desc->more_data,
        tx_desc->stype_b5b4, tx_desc->extra_info, tx_desc->aggr);
    printk("fcmd=0x%08x, hdr_offset=%d, frag=%d, unicast=%d, hdr_len=%d\n",
        tx_desc->fCmd, tx_desc->hdr_offset, tx_desc->frag, tx_desc->unicast,
        tx_desc->hdr_len);
    printk("ack_policy0=%d, do_rts_cts0=%d, ack_policy1=%d, do_rts_cts1=%d, reason=%d\n",
        tx_desc->ack_policy0, tx_desc->do_rts_cts0,tx_desc->ack_policy1, tx_desc->do_rts_cts1,
        tx_desc->reason);
    printk("ack_policy2=%d, do_rts_cts2=%d,ack_policy3=%d, do_rts_cts3=%d\n",
         tx_desc->ack_policy2, tx_desc->do_rts_cts2,tx_desc->ack_policy3, tx_desc->do_rts_cts3);
    printk("fcmdidx=%d, wsid=%d, txq_idx=%d\n",
         tx_desc->fCmdIdx, tx_desc->wsid, tx_desc->txq_idx);
    printk("0:RTS/CTS Nav=%d, crate_idx=%d, drate_idx=%d, dl_len=%d, retry=%d, last_rate %d \n",
        tx_desc->rts_cts_nav0, tx_desc->crate_idx0, tx_desc->drate_idx0,
        tx_desc->dl_length0, tx_desc->try_cnt0, tx_desc->is_last_rate0);
    printk("1:RTS/CTS Nav=%d, crate_idx=%d, drate_idx=%d, dl_len=%d, retry=%d, last_rate %d  \n",
        tx_desc->rts_cts_nav1, tx_desc->crate_idx1, tx_desc->drate_idx1,
        tx_desc->dl_length1, tx_desc->try_cnt1, tx_desc->is_last_rate1);
    printk("2:RTS/CTS Nav=%d, crate_idx=%d, drate_idx=%d, dl_len=%d, retry=%d, last_rate %d  \n",
        tx_desc->rts_cts_nav2, tx_desc->crate_idx2, tx_desc->drate_idx2,
        tx_desc->dl_length2, tx_desc->try_cnt2, tx_desc->is_last_rate2);
    printk("3:RTS/CTS Nav=%d, crate_idx=%d, drate_idx=%d, dl_len=%d, retry=%d , last_rate %d \n",
        tx_desc->rts_cts_nav3, tx_desc->crate_idx3, tx_desc->drate_idx3,
        tx_desc->dl_length3, tx_desc->try_cnt3, tx_desc->is_last_rate3);

}

static void ssv6006_add_txinfo (struct ssv_softc *sc, struct sk_buff *skb)
{
    struct ssv6006_tx_desc          *tx_desc;

    /* Request more spaces in front of the payload for ssv6006 tx info: */
    skb_push(skb, TXPB_OFFSET);
    tx_desc = (struct ssv6006_tx_desc *)skb->data;
    memset((void *)tx_desc, 0, TXPB_OFFSET);
    ssv6006_update_txinfo(sc, skb);
    if (sc->log_ctrl & LOG_TX_FRAME){
        ssv6006_dump_ssv6006_txframe(skb);
    }

    if (sc->log_ctrl & LOG_TX_DESC){
        printk(" dump tx desciptor after tx add info:\n");
        ssv6006_dump_tx_desc(skb);
    }
} // end of - ssv6006_add_txinfo -

static void ssv6006_update_ampdu_txinfo(struct ssv_softc *sc, struct sk_buff *ampdu_skb)
{
    struct ssv6006_tx_desc *tx_desc = (struct ssv6006_tx_desc *)ampdu_skb->data;

    tx_desc->tx_pkt_run_no = sc->tx_pkt_run_no;

}

static void ssv6006_add_ampdu_txinfo(struct ssv_softc *sc, struct sk_buff *ampdu_skb)
{
    struct ssv6006_tx_desc *tx_desc;

    /* Request more spaces in front of the payload for ssv6006 tx info: */
    skb_push(ampdu_skb, TXPB_OFFSET);
    tx_desc = (struct ssv6006_tx_desc *)ampdu_skb->data;
    memset((void *)tx_desc, 0, TXPB_OFFSET);
    tx_desc->aggr = 2;      // enable ampdu tx 1.2
    ssv6006_update_txinfo(sc, ampdu_skb);

    if (sc->log_ctrl & LOG_TX_FRAME){
        ssv6006_dump_ssv6006_txframe(ampdu_skb);
    }

    if (sc->log_ctrl & LOG_TX_DESC){
        printk(" dump tx desciptor after tx ampdu add info:\n");
        ssv6006_dump_tx_desc(ampdu_skb);
    }
}

static int ssv6006_update_null_func_txinfo(struct ssv_softc *sc, struct ieee80211_sta *sta, struct sk_buff *skb)
{

    struct ieee80211_hdr            *hdr = (struct ieee80211_hdr *)(skb->data + TXPB_OFFSET);
    struct ssv_sta_priv_data        *ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
    struct ssv_sta_info             *sta_info = (struct ssv_sta_info *)ssv_sta_priv->sta_info;/* sta_info is already protected by ssv6xxx_beacon_miss_work(). */
    struct ssv6006_tx_desc          *tx_desc = (struct ssv6006_tx_desc *)skb->data;
    int                              hw_txqid = 4;

    memset(tx_desc, 0x0, sizeof(struct ssv6006_tx_desc));
    tx_desc->len = skb->len;
    tx_desc->c_type = M2_TXREQ;
    tx_desc->f80211 = 1;
    tx_desc->unicast = 1;
    /*
     * tx_pkt_run_no
     * 0 : mpdu packet
     * 1 ~ 127 : ampdu packet
     * 128 : nullfunc packet
     */
    tx_desc->tx_pkt_run_no = SSV6XXX_PKT_RUN_TYPE_NULLFUN;
    tx_desc->wsid = sta_info->hw_wsid;/* sta_info is already protected by ssv6xxx_beacon_miss_work(). */
    tx_desc->txq_idx = hw_txqid;
    tx_desc->hdr_offset = TXPB_OFFSET; //SSV6XXX_TX_DESC_LEN
    tx_desc->hdr_len = ssv6xxx_frame_hdrlen(hdr, false);
    tx_desc->drate_idx0 = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_6M << SSV6006RC_RATE_SFT));
    tx_desc->crate_idx0 = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_6M << SSV6006RC_RATE_SFT));
    tx_desc->try_cnt0 = 0xf;
    tx_desc->is_last_rate0 = 1;
	tx_desc->rate_rpt_mode = 1;

    tx_desc->fCmdIdx = 0;
    tx_desc->fCmd = (hw_txqid+M_ENG_TX_EDCA0);
    tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_HWHCI;

    //set duration time
    hdr->duration_id = ssv6xxx_non_ht_txtime(WLAN_RC_PHY_OFDM, 6000, ACK_LEN, false);//always G mode, 6M


    return 0;
}

static void ssv6006_dump_rx_desc(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rx_desc;
    int s;
    u8 *dat;

    rx_desc = (struct ssv6006_rx_desc *)skb->data;

    printk(">> RX Descriptor:\n");
    for(s = 0, dat= (u8 *)skb->data; s < sizeof(struct ssv6006_rx_desc) /4; s++) {
        printk("%02x%02x%02x%02x ", dat[4*s+3], dat[4*s+2], dat[4*s+1], dat[4*s]);
        if (((s+1)& 0x03) == 0)
            printk("\n");
    }

    printk("\nlen=%d, c_type=%d, f80211=%d, qos=%d, ht=%d, use_4addr=%d\n",
        rx_desc->len, rx_desc->c_type, rx_desc->f80211, rx_desc->qos, rx_desc->ht, rx_desc->use_4addr);
    printk("psm=%d, stype_b5b4=%d, reason=%d, rx_result=%d, channel = %d\n",
        rx_desc->psm, rx_desc->stype_b5b4, rx_desc->reason, rx_desc->RxResult, rx_desc->channel);

    printk("phy_rate=0x%x, aggregate=%d, l_length=%d, l_rate=%d, rssi = %d\n",
        rx_desc->phy_rate, rx_desc->phy_aggregation, rx_desc->phy_l_length, rx_desc->phy_l_rate, rx_desc->phy_rssi);
    printk("snr=%d, rx_freq_offset=%d\n", rx_desc->phy_snr, rx_desc->phy_rx_freq_offset);

}

static int ssv6006_get_tx_desc_size(struct ssv_hw *sh)
{
	return  sizeof(struct ssv6006_tx_desc);
}

static int ssv6006_get_tx_desc_ctype(struct sk_buff *skb)
{
	struct ssv6006_tx_desc *tx_desc = (struct ssv6006_tx_desc *) skb->data;

    return tx_desc->c_type ;
}

static int ssv6006_get_tx_desc_reason(struct sk_buff *skb)
{
	struct ssv6006_tx_desc *tx_desc = (struct ssv6006_tx_desc *) skb->data;

    return tx_desc->reason ;
}

static int ssv6006_get_tx_desc_wsid(struct sk_buff *skb)
{
	struct ssv6006_tx_desc *tx_desc = (struct ssv6006_tx_desc *) skb->data;

    return tx_desc->wsid;
}

static int ssv6006_get_tx_desc_txq_idx(struct sk_buff *skb)
{
	struct ssv6006_tx_desc *tx_desc = (struct ssv6006_tx_desc *) skb->data;

    return tx_desc->txq_idx ;
}

static void ssv6006_txtput_set_desc(struct ssv_hw *sh, struct sk_buff *skb )
{
    struct ssv6006_tx_desc *tx_desc;

	tx_desc = (struct ssv6006_tx_desc *)skb->data;
	memset((void *)tx_desc, 0xff, sizeof(struct ssv6006_tx_desc));
	tx_desc->len    = skb->len;
	tx_desc->c_type = M2_TXREQ;
	tx_desc->fCmd = (M_ENG_CPU << 4) | M_ENG_HWHCI;
	tx_desc->reason = ID_TRAP_SW_TXTPUT;
}

static void ssv6006_fill_beacon_tx_desc(struct ssv_softc *sc, struct sk_buff* beacon_skb)
{
	struct ssv6006_tx_desc *tx_desc;

	/* Insert description space */
	skb_push(beacon_skb, TXPB_OFFSET);

    tx_desc = (struct ssv6006_tx_desc *)beacon_skb->data;
	memset(tx_desc,0, TXPB_OFFSET);

    tx_desc->len            = beacon_skb->len-TXPB_OFFSET;
	tx_desc->c_type         = M2_TXREQ;
    tx_desc->f80211         = 1;
	tx_desc->ack_policy0    = 1;//no ack;
	tx_desc->ack_policy1    = 1;//no ack;
	tx_desc->ack_policy2    = 1;//no ack;
	tx_desc->ack_policy3    = 1;//no ack;
    tx_desc->hdr_offset		= TXPB_OFFSET;
    tx_desc->hdr_len		= 24;

	tx_desc->wsid			= 0xf;
	if ((sc->cur_channel->band == INDEX_80211_BAND_5GHZ) || (sc->ap_vif->p2p == true)){
        tx_desc->drate_idx0 = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT)
                            | (SSV6006RC_G_6M << SSV6006RC_RATE_SFT));
        tx_desc->crate_idx0 = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT)
                            | (SSV6006RC_G_6M << SSV6006RC_RATE_SFT));
	}else{
	    tx_desc->drate_idx0 = ((SSV6006RC_B_MODE << SSV6006RC_PHY_MODE_SFT)
                            | (SSV6006RC_B_1M << SSV6006RC_RATE_SFT));
        tx_desc->crate_idx0 = ((SSV6006RC_B_MODE << SSV6006RC_PHY_MODE_SFT)
                            | (SSV6006RC_B_1M << SSV6006RC_RATE_SFT));
    }
	tx_desc->try_cnt0		= 1;
	tx_desc->is_last_rate0  = 1;
}

static int ssv6006_get_tkip_mmic_err(struct sk_buff *skb)
{
	struct ssv6006_rx_desc *rx_desc = (struct ssv6006_rx_desc *) skb->data;
	bool ret = false;

    if (rx_desc->tkip_mmic_err == 1)
        ret = true;

    return ret;
}

static int ssv6006_get_rx_desc_size(struct ssv_hw *sh)
{
#define RXPB_OFFSET_KRACK 80
   if (sh->cfg.hw_caps & SSV6200_HW_CAP_KRACK){
       return RXPB_OFFSET_KRACK;
   } else {
/*#ifdef CAL_RX_DELAY

    #define EXTRA_SPACE	(2 * sizeof(u32))
       //reserve extra space for log time stamp
	   return sizeof(struct ssv6006_rx_desc) + sizeof (struct ssv6006_rxphy_info) + EXTRA_SPACE;
#else
       return sizeof(struct ssv6006_rx_desc) + sizeof (struct ssv6006_rxphy_info);
#endif*/
	   return sizeof(struct ssv6006_rx_desc);
   }
}

static int ssv6006_get_rx_desc_length(struct ssv_hw *sh)
{
	return sizeof(struct ssv6006_rx_desc);
}

static u32 ssv6006_get_rx_desc_wsid(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rx_desc = (struct ssv6006_rx_desc *)skb->data;

    return rx_desc->wsid;
}

static u32 ssv6006_get_rx_desc_rate_idx(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)skb->data;

    return rxdesc->phy_rate;
}

static u32 ssv6006_get_rx_desc_mng_used(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rx_desc = (struct ssv6006_rx_desc *)skb->data;

    return rx_desc->mng_used;
}

static bool ssv6006_is_rx_aggr(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)skb->data;

    return rxdesc->phy_aggregation;
}

static void ssv6006_get_rx_desc_info_hdr(unsigned char *desc, u32 *packet_len, u32 *c_type,
        u32 *tx_pkt_run_no)
{
    struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)desc;

    if ((rxdesc->c_type == M0_RXEVENT) || (rxdesc->c_type == M2_RXEVENT))
        rxdesc->len = rxdesc->rx_len;

    *packet_len = rxdesc->len;
    *c_type = rxdesc->c_type;
    *tx_pkt_run_no = rxdesc->rx_pkt_run_no;
}

static u32 ssv6006_get_rx_desc_ctype(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)skb->data;

    return rxdesc->c_type;
}

static int ssv6006_get_rx_desc_hdr_offset(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)skb->data;

    return rxdesc->hdr_offset;
}

static u32 ssv6006_get_rx_desc_phy_rssi(struct sk_buff *skb)
{
    struct ssv6006_rx_desc *rx_desc = (struct ssv6006_rx_desc *)skb->data;

    return rx_desc->phy_rssi;
}

static bool ssv6006_nullfun_frame_filter(struct sk_buff *skb)
{
    struct ssv6006_tx_desc *txdesc = (struct ssv6006_tx_desc *)skb->data;

    if (txdesc->tx_pkt_run_no == SSV6XXX_PKT_RUN_TYPE_NULLFUN)
        return true;

    return false;
}

static void ssv6006_phy_enable(struct ssv_hw *sh, bool val)
{
	struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
	struct cfg_host_cmd *host_cmd;

    if(sc->sc_flags & SC_OP_HW_RESET)
    {
        SMAC_REG_SET_BITS(sh, ADR_WIFI_PHY_COMMON_ENABLE_REG, (val << RG_PHY_MD_EN_SFT), RG_PHY_MD_EN_MSK);
        return;
    }

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
	if (skb != NULL) {
	    skb_put(skb, HOST_CMD_HDR_LEN);
	    host_cmd = (struct cfg_host_cmd *)skb->data;
        memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
	    host_cmd->c_type = HOST_CMD;
	    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
        if (val) {
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_PHY_ENABLE;
            host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_PHY_ENABLE);
	    } else {
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_PHY_DISABLE;
            host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_PHY_DISABLE);
        }
        host_cmd->len = HOST_CMD_HDR_LEN;

        HCI_SEND_CMD(sc->sh, skb);
    } else
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
}

static void ssv6006_set_phy_mode(struct ssv_hw *sh, bool val)
{
    if (val) { // set phy mode on without enable
        SMAC_REG_WRITE(sh, ADR_WIFI_PHY_COMMON_ENABLE_REG,(RG_PHYRX_MD_EN_MSK | RG_PHYTX_MD_EN_MSK |
            RG_PHY11GN_MD_EN_MSK | RG_PHY11B_MD_EN_MSK | RG_PHYRXFIFO_MD_EN_MSK |
            RG_PHYTXFIFO_MD_EN_MSK | RG_PHY11BGN_MD_EN_MSK));
    } else { //clear phy mode
        SMAC_REG_WRITE(sh, ADR_WIFI_PHY_COMMON_ENABLE_REG, 0x00000000);
    }
}

static void ssv6006_reset_mib_phy(struct ssv_hw *sh)
{
    //Reset PHY MIB
    SET_RG_MRX_EN_CNT_RST_N(0);
    SET_RG_PACKET_STAT_EN_11B_RX(0);
    SET_RG_PACKET_STAT_EN_11GN_RX(0);
    //printk("%s: %d, %d, %d\n", __func__, GET_RG_MRX_EN_CNT_RST_N, GET_RG_PACKET_STAT_EN_11B_RX, GET_RG_PACKET_STAT_EN_11GN_RX);
    msleep(1);
    SET_RG_MRX_EN_CNT_RST_N(1);
    SET_RG_PACKET_STAT_EN_11B_RX(1);
    SET_RG_PACKET_STAT_EN_11GN_RX(1);
}

static void ssv6006_dump_mib_rx_phy(struct ssv_hw *sh)
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    snprintf_res(cmd_data, "PHY total Rx\t:[%08x]\n", GET_RO_MRX_EN_CNT );

    snprintf_res(cmd_data, "PHY B mode:\n");
    snprintf_res(cmd_data, "%-10s\t%-10s\t%-10s\t%-10s\t%-10s\n", "SFD_CNT","CRC_CNT","PKT_ERR","CCA","PKT_CNT");
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_11B_SFD_CNT);
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_11B_CRC_CNT);
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_11B_PACKET_ERR_CNT);
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_11B_CCA_CNT);
    snprintf_res(cmd_data, "[%08x]\t\n", GET_RO_11B_PACKET_CNT);

    snprintf_res(cmd_data, "PHY G/N mode:\n");

    snprintf_res(cmd_data, "%-10s\t%-10s\t%-10s\t%-10s\t%-10s\n","AMPDU ERR", "AMPDU PKT","PKT_ERR","CCA","PKT_CNT");
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_AMPDU_PACKET_ERR_CNT);
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_AMPDU_PACKET_CNT);
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_11GN_PACKET_ERR_CNT);
    snprintf_res(cmd_data, "[%08x]\t", GET_RO_11GN_CCA_CNT);
    snprintf_res(cmd_data, "[%08x]\t\n\n", GET_RO_11GN_PACKET_CNT);

}

static void ssv6006_update_mac80211_chan_info(struct ssv_softc *sc,
            struct sk_buff *skb, struct ieee80211_rx_status *rxs)
{
	struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)skb->data;

    rxs->band = (rxdesc->channel > 14) ? INDEX_80211_BAND_5GHZ: INDEX_80211_BAND_2GHZ;
    if (rxdesc->channel == 14)
        rxs->freq = 2484;
    else if (rxdesc->channel < 14)
        rxs->freq = 2407 + rxdesc->channel * 5;
    else
        rxs->freq = 5000 + rxdesc->channel * 5;
}

void ssv6006_rc_mac80211_rate_idx(struct ssv_softc *sc,
            int hw_rate_idx, struct ieee80211_rx_status *rxs)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
    if (((hw_rate_idx & SSV6006RC_PHY_MODE_MSK)  >>
        SSV6006RC_PHY_MODE_SFT)== SSV6006RC_N_MODE){
        rxs->encoding = RX_ENC_HT;

        if (((hw_rate_idx & SSV6006RC_20_40_MSK) >>
            SSV6006RC_20_40_SFT) == SSV6006RC_HT40){
            rxs->bw = RATE_INFO_BW_40;
        }

        if (((hw_rate_idx & SSV6006RC_LONG_SHORT_MSK) >>
            SSV6006RC_LONG_SHORT_SFT) == SSV6006RC_SHORT){
            rxs->enc_flags |= RX_ENC_FLAG_SHORT_GI;
        }
    } else {
        rxs->encoding = RX_ENC_LEGACY;
        if (((hw_rate_idx & SSV6006RC_LONG_SHORT_MSK) >>
            SSV6006RC_LONG_SHORT_SFT) == SSV6006RC_SHORT){
            rxs->enc_flags |= RX_ENC_FLAG_SHORTPRE;
        }
    }
#else
    if (((hw_rate_idx & SSV6006RC_PHY_MODE_MSK)  >>
        SSV6006RC_PHY_MODE_SFT)== SSV6006RC_N_MODE){
        rxs->flag |= RX_FLAG_HT;

        if (((hw_rate_idx & SSV6006RC_20_40_MSK) >>
            SSV6006RC_20_40_SFT) == SSV6006RC_HT40){
            rxs->flag |= RX_FLAG_40MHZ;
        }

        if (((hw_rate_idx & SSV6006RC_LONG_SHORT_MSK) >>
            SSV6006RC_LONG_SHORT_SFT) == SSV6006RC_SHORT){
            rxs->flag |= RX_FLAG_SHORT_GI;
        }
    } else {
        if (((hw_rate_idx & SSV6006RC_LONG_SHORT_MSK) >>
            SSV6006RC_LONG_SHORT_SFT) == SSV6006RC_SHORT){
            rxs->flag |= RX_FLAG_SHORTPRE;
        }
    }
#endif

    rxs->rate_idx = (hw_rate_idx & SSV6006RC_RATE_MSK) >>
            SSV6006RC_RATE_SFT;
    /*
     * For legacy, g mode rate index need to conside b mode rate (rate index: 0 ~ 3)
     * Therefore, g mode rate index range is from 4 to 11 for mac80211 layer in kernel.
     */
    if ((((hw_rate_idx & SSV6006RC_PHY_MODE_MSK)  >> SSV6006RC_PHY_MODE_SFT)== SSV6006RC_G_MODE) &&
        (rxs->band == INDEX_80211_BAND_2GHZ)) {
        rxs->rate_idx += DOT11_G_RATE_IDX_OFFSET;
    }
}

void ssv6006_rc_mac80211_tx_rate_idx(struct ssv_softc *sc,
            int hw_rate_idx, struct ieee80211_tx_info *tx_info)
{
    u16 hw_chan = sc->hw_chan;
    u8 rate_idx = 0;
    u8 flags = 0;

    //[7:6] phy mode
    //[5]: ht40
    //[4]: long/short
    //[3]: mf
    //[2:0]: RateIndex
    if (SSV6006RC_N_MODE == ((hw_rate_idx & SSV6006RC_PHY_MODE_MSK)>>SSV6006RC_PHY_MODE_SFT))
    {
        flags |= IEEE80211_TX_RC_MCS;

        if (SSV6006RC_HT40 == ((hw_rate_idx & SSV6006RC_20_40_MSK)>>SSV6006RC_20_40_SFT))
        {
            flags |= IEEE80211_TX_RC_40_MHZ_WIDTH;
        }

        if (SSV6006RC_SHORT == ((hw_rate_idx & SSV6006RC_LONG_SHORT_MSK)>>SSV6006RC_LONG_SHORT_SFT))
        {
            flags |= IEEE80211_TX_RC_SHORT_GI;
        }
    }
    else
    {
        if (SSV6006RC_SHORT == ((hw_rate_idx & SSV6006RC_LONG_SHORT_MSK)>>SSV6006RC_LONG_SHORT_SFT))
        {
            flags |= IEEE80211_TX_RC_USE_SHORT_PREAMBLE;
        }
    }

    rate_idx = (hw_rate_idx&SSV6006RC_RATE_MSK)>>SSV6006RC_RATE_SFT;
    /*
     * For legacy, g mode rate index need to conside b mode rate (rate index: 0 ~ 3)
     * Therefore, g mode rate index range is from 4 to 11 for mac80211 layer in kernel.
     */
    if ((SSV6006RC_G_MODE == ((hw_rate_idx & SSV6006RC_PHY_MODE_MSK)>>SSV6006RC_PHY_MODE_SFT)) &&
        (14 >= hw_chan))
    {
        rate_idx += DOT11_G_RATE_IDX_OFFSET;
    }
    //printk("cur_rate = 0x%02x, rate_idx = %u, flags = 0x%02x\n", hw_rate_idx, rate_idx, flags);
    tx_info->status.rates[0].idx = rate_idx;
    tx_info->status.rates[0].flags = flags;
}

void _update_green_tx(struct ssv_softc *sc, u16 rssi)
{
    int  atteneuation_pwr = 0;
    u8  gt_pwr_start;
    int max_attenuation;

    if ((!(sc->gt_enabled)) || (sc->green_pwr == 0xff))
        return;

    gt_pwr_start = sc->sh->cfg.greentx & GT_PWR_START_MASK;
    max_attenuation = sc->sh->cfg.gt_max_attenuation;
    if ((sc->sh->cfg.external_pa & EX_PA_ENABLE) && (sc->hw_chan <=14)) {
        max_attenuation += 8;
        gt_pwr_start += 10;
    }

    if ((rssi < gt_pwr_start) && (rssi >= 0)) {

        atteneuation_pwr = (gt_pwr_start - rssi )*2 + sc->default_pwr; // 0.5 db resolution.

        sc->dpd.pwr_mode = GREEN_PWR;

        if (sc->sh->cfg.greentx & GT_DBG) {
            printk("[greentx]:current_power %d\n", sc->green_pwr);
        }
        if (atteneuation_pwr > (sc->green_pwr + (sc->sh->cfg.gt_stepsize*2))){

            sc->green_pwr += (sc->sh->cfg.gt_stepsize*2);

            if (sc->green_pwr > ((max_attenuation*2) + sc->default_pwr))
                sc->green_pwr = (max_attenuation*2 + sc->default_pwr);


        } else if (atteneuation_pwr <(int)  (sc->green_pwr - (sc->sh->cfg.gt_stepsize*2))){

            sc->green_pwr -= (sc->sh->cfg.gt_stepsize*2);
            if (sc->green_pwr < sc->default_pwr)
                sc->green_pwr = sc->default_pwr;
        }

        if (sc->sh->cfg.greentx & GT_DBG) {
            printk("[greentx]:rssi: %d expected green tx pwr gain %d, set tx pwr gain %d\n",
            rssi, atteneuation_pwr, sc->green_pwr);
        }

    } else {
        if ( sc->dpd.pwr_mode == GREEN_PWR) {
            sc->dpd.pwr_mode  = NORMAL_PWR;
            sc->green_pwr = 0xff;
            if (sc->sh->cfg.greentx & GT_DBG) {
                printk("[greentx]:restore normal pwr\n");
            }
        }
    }
}

#ifdef CONFIG_SSV_CCI_IMPROVEMENT

static struct ssv6xxx_cca_control adjust_cci_40[] = {
    //{0 , 33, 0x01164000, 0x40000180},
    { 0, 38, 0x01162000, 0x20000180},
    {35, 43, 0x01161000, 0x10000180},
    {40, 48, 0x01160800, 0x08000180},
    {45, 53, 0x01160400, 0x04000180},
    {50, 63, 0x01160200, 0x02000180},
    {60, 68, 0x01160100, 0x01000180},
    {65, 128, 0x00000000, 0x00000000},
};

static struct ssv6xxx_cca_control adjust_cci_20[] = {
   // {0 , 33, 0x01168000, 0x80000180},
    { 0, 38, 0x01164000, 0x40000180},
    {35, 43, 0x01162000, 0x20000180},
    {40, 48, 0x01161000, 0x10000180},
    {45, 53, 0x01160800, 0x08000180},
    {50, 63, 0x01160400, 0x04000180},
    {60, 68, 0x01160200, 0x02000180},
    {65, 73, 0x01160100, 0x01000180},
    {70, 128, 0x00000000, 0x00000000},
};



void ssv6006_update_data_cci_setting
(struct ssv_softc *sc, struct ssv_sta_priv_data *sta_priv, u32 input_level)
{
    s32 i;
    struct ssv_vif_priv_data               *vif_priv = NULL;
    struct ieee80211_vif                   *vif = NULL;
    struct ssv6xxx_cca_control *adjust_cci;
    int    max_cci_idx;

    if (((sc->sh->cfg.cci & CCI_CTL) == 0) ||  (sc->bScanning == true)){
            return;
    }

    if (time_after(jiffies, sc->cci_last_jiffies + msecs_to_jiffies(500)) ||
        (sc->cci_current_level == MAX_CCI_LEVEL)){
        if ((input_level > MAX_CCI_LEVEL) && (sc->sh->cfg.cci & CCI_DBG)) {
            printk("mitigate_cci input error[%d]!!\n",input_level);
            return;
        }
#ifndef IRQ_PROC_RX_DATA
        down_read(&sc->sta_info_sem);/* START protect sta_info */
#else
        while(!down_read_trylock(&sc->sta_info_sem));/* START protect sta_info */
#endif
        if ((sta_priv->sta_info->s_flags & STA_FLAG_VALID) == 0) {
            up_read(&sc->sta_info_sem);/* END protect sta_info */
            printk("%s(): sta_info is gone.\n", __func__);
            return;
        }
        sc->cci_last_jiffies = jiffies;
        /*
            Only support first interfac.
            We expect the first one is the staion mode or AP mode not P2P.
        */
        vif = sta_priv->sta_info->vif;
        vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
        if ((vif_priv->vif_idx)&& (sc->sh->cfg.cci & CCI_DBG)){
            up_read(&sc->sta_info_sem);/* END protect sta_info */
            printk("Interface skip CCI[%d]!!\n",vif_priv->vif_idx);
            return;
        }
        if ((vif->p2p == true) && (sc->sh->cfg.cci & CCI_DBG)) {
            up_read(&sc->sta_info_sem);/* END protect sta_info */
            printk("Interface skip CCI by P2P!!\n");
            return;
        }
        up_read(&sc->sta_info_sem);/* END protect sta_info */

        if ((sc->hw_chan_type == NL80211_CHAN_HT20)||
            (sc->hw_chan_type == NL80211_CHAN_NO_HT)) {
            adjust_cci = &adjust_cci_20[0];
            max_cci_idx =  (sizeof(adjust_cci_20)/sizeof(adjust_cci_20[0]));
        } else {
            adjust_cci = &adjust_cci_40[0];
            max_cci_idx =  (sizeof(adjust_cci_40)/sizeof(adjust_cci_40[0]));
        }

        if (sc->cci_current_level == 0)
            sc->cci_current_level = MAX_CCI_LEVEL;

        if (sc->cci_current_level == MAX_CCI_LEVEL){
            sc->cci_current_gate = max_cci_idx - 1;
            sc->cci_set = true;
        }

        sc->cci_start = true;
#ifdef  DEBUG_MITIGATE_CCI
       /* if (sc->sh->cfg.cci & CCI_DBG){
            printk("jiffies=%lu, input_level=%d\n", jiffies, input_level);
        }*/
#endif
        if(( input_level >= adjust_cci[sc->cci_current_gate].down_level) && (input_level <= adjust_cci[sc->cci_current_gate].upper_level)) {
            sc->cci_current_level = input_level;
#ifdef  DEBUG_MITIGATE_CCI
            if (sc->sh->cfg.cci & CCI_DBG){
                printk("Keep the ADR_WIFI_11B_RX_REG_040[%x] ADR_WIFI_11GN_RX_REG_040[%x]!!\n",
                adjust_cci[sc->cci_current_gate].adjust_cck_cca_control,
                adjust_cci[sc->cci_current_gate].adjust_ofdm_cca_control);
            }
#endif
        }
        else {
            // [current_level]30 -> [input_level]75
            if(sc->cci_current_level < input_level) {
                for (i = 0; i < max_cci_idx; i++) {
                    if (input_level <= adjust_cci[i].upper_level) {
#ifdef  DEBUG_MITIGATE_CCI
                    if (sc->sh->cfg.cci & CCI_DBG){
                        printk("gate=%d, input_level=%d, adjust_cci[%d].upper_level=%d, cck value=%08x, ofdm value= %08x\n",
                                sc->cci_current_gate, input_level, i, adjust_cci[i].upper_level
                                , adjust_cci[i].adjust_cck_cca_control, adjust_cci[i].adjust_ofdm_cca_control);
                    }
#endif
                        sc->cci_current_level = input_level;
                        sc->cci_current_gate = i;

                        sc->cci_modified = true;
#ifdef  DEBUG_MITIGATE_CCI
                        if (sc->sh->cfg.cci & CCI_DBG){
                            printk("Set to ADR_WIFI_11B_RX_REG_040[%x] ADR_WIFI_11GN_RX_REG_040[%x]!!\n",
                                adjust_cci[sc->cci_current_gate].adjust_cck_cca_control,
                                adjust_cci[sc->cci_current_gate].adjust_ofdm_cca_control);
                        }
#endif
                        return;
                    }
                }
            }
            // [current_level]75 -> [input_level]30
            else {
                for (i = (max_cci_idx -1); i >= 0; i--) {
                    if (input_level >= adjust_cci[i].down_level) {
#ifdef  DEBUG_MITIGATE_CCI
                    if (sc->sh->cfg.cci & CCI_DBG){
                        printk("gate=%d, input_level=%d, adjust_cci[%d].upper_level=%d, cck value=%08x, ofdm value= %08x\n",
                                sc->cci_current_gate, input_level, i, adjust_cci[i].upper_level
                                , adjust_cci[i].adjust_cck_cca_control, adjust_cci[i].adjust_ofdm_cca_control);
                    }
#endif
                        sc->cci_current_level = input_level;
                        sc->cci_current_gate = i;

                        sc->cci_modified = true;
#ifdef  DEBUG_MITIGATE_CCI
                        if (sc->sh->cfg.cci & CCI_DBG){
                            printk("Set to ADR_WIFI_11B_RX_REG_040[%x] ADR_WIFI_11GN_RX_REG_040[%x]!!\n",
                                adjust_cci[sc->cci_current_gate].adjust_cck_cca_control,
                                adjust_cci[sc->cci_current_gate].adjust_ofdm_cca_control);
                        }
#endif
                        return;
                    }
                }
            }
        }
    }
    return;
}
#endif

static void _update_rx_data_rate_stats(struct ssv_softc *sc, struct ssv_sta_priv_data *sta_priv, struct ssv6006_rx_desc *rxdesc)
{
      // update rx rate statistics
     sta_priv->rxstats.phy_mode = ((rxdesc->phy_rate & SSV6006RC_PHY_MODE_MSK) >> SSV6006RC_PHY_MODE_SFT);
     sta_priv->rxstats.ht40 = ((rxdesc->phy_rate & SSV6006RC_20_40_MSK) >> SSV6006RC_20_40_SFT);
     if (sta_priv->rxstats.phy_mode == SSV6006RC_B_MODE) {
         sta_priv->rxstats.cck_pkts[(rxdesc->phy_rate) & SSV6006RC_B_RATE_MSK] ++;
     } else if (sta_priv->rxstats.phy_mode == SSV6006RC_N_MODE) {
         sta_priv->rxstats.n_pkts[(rxdesc->phy_rate) & SSV6006RC_RATE_MSK] ++;
     } else {
         sta_priv->rxstats.g_pkts[(rxdesc->phy_rate) & SSV6006RC_RATE_MSK] ++;
     }
     //printk("phy rate %x, phy mode %d, phy_rate idx  %d\n",rxphy-> phy_rate, sta_priv->rxstats.phy_mode, (rxphy-> phy_rate) & SSV6006RC_RATE_MSK);
}

static int _check_for_krack( struct ssv_softc *sc, struct ssv_sta_priv_data *sta_priv,
    struct sk_buff *rx_skb, struct ieee80211_hdr *hdr)
{
	struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)rx_skb->data;
    struct ssv_sta_info         *sta_info = sta_priv->sta_info;
    struct ieee80211_vif        *vif = sta_info->vif;
    struct ssv_vif_priv_data    *vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
    u64    pn;

//    pn = (rxpn->pn_low & 0xffff) + (u64)(rxpn->pn_high >> 16);
	pn = (u64)rxdesc->rx_pn_0 + (((u64)rxdesc->rx_pn_1) << 8) + (((u64)rxdesc->rx_pn_2) << 16)
	+ (((u64)rxdesc->rx_pn_3) << 24) + (((u64)rxdesc->rx_pn_4) << 32) + (((u64)rxdesc->rx_pn_5) << 40);

    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_KRACK,
        "last_pn %llX, last pn mcast %llx, unitcast %d, pn %llX, seqno %d\n",
        sta_priv->last_pn, vif_priv->last_pn_mcast, is_unicast_ether_addr(hdr->addr1), pn, hdr->seq_ctrl >>4);

    if (is_unicast_ether_addr(hdr->addr1)) {
        if (sta_priv->last_pn != 0){
            if ((sta_priv->last_pn) >= pn) {
                if (sta_priv->unicast_key_changed) {
                    sta_priv->unicast_key_changed = false;
                } else {
                    printk("unicast resend pn\n");
                    return -1;
                }
            }
        }
        sta_priv->last_pn = pn;
    } else {
        if (vif_priv->last_pn_mcast != 0){
            if ((vif_priv->last_pn_mcast) >= pn) {
                if (vif_priv->group_key_changed) {
                    vif_priv->group_key_changed = false;
                } else {
                    printk("multicast resend pn\n");
                    return -1;
                }
            }
        }
        vif_priv->last_pn_mcast = pn;
#if 0	// this patch will drop all broadcast data, not necesaary now.
        if (is_broadcast_ether_addr(hdr->addr1)) {
            printk("broacast!! force to drop\n");
            return -1;
        }
#endif
    }
    return 0;
}

#if 0 //No need in currently.
static void _set_external_LNA(struct ssv_softc *sc, int rssi)
{
    u32 high_threshold, low_threshold;

    high_threshold = ((sc->sh-> cfg.external_lna & EX_PA_LNA_HIGH_THR_MASK) >> EX_PA_LNA_HIGH_THR_SFT);
    low_threshold = (sc->sh-> cfg.external_lna & EX_PA_LNA_LOW_THR_MASK);
    if (sc->hw_chan > 14) { //lower 10db for 5G channel
        if (sc->LNA_status) {
            return;
        } else {
            sc->LNA_status = true;
            ssv6006c_set_external_lna(sc->sh, true);
            return;
        }
    }
    if ((sc->sh->cfg.external_lna & EX_PA_AUTO_LNA)  && (sc->sh->cfg.external_pa & EX_PA_ENABLE)){
        if (sc->LNA_status) {
            if (rssi < low_threshold) {
                sc->mod_chg_cnt++;
                if (sc->mod_chg_cnt > 10) {
                    sc->LNA_status =false;
                    ssv6006c_set_external_lna(sc->sh, false);
                    sc->mod_chg_cnt = 0;
                }
            } else {
                sc->mod_chg_cnt = 0;
            }
        } else {
            if (rssi > high_threshold) {
                sc->mod_chg_cnt++;
                if (sc->mod_chg_cnt > 10) {
                    sc->LNA_status = true;
                    ssv6006c_set_external_lna(sc->sh, true);
                    sc->mod_chg_cnt = 0;
                }
            } else {
                sc->mod_chg_cnt = 0;
            }
        }
    }
}


static void _update_beacon_rssi(struct ssv_softc *sc, struct ieee80211_sta *sta, struct ssv_sta_priv_data *sta_priv,
    struct ieee80211_hdr *hdr, struct ssv6006_rx_desc *rxdesc)
{
    int org_rssi;

    if (sta)
    {
        sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
    #ifdef SSV_RSSI_DEBUG /*For debug*/
        printk("beacon %02X:%02X:%02X:%02X:%02X:%02X rxphy->rssi=%d\n",
           hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
           hdr->addr2[3], hdr->addr2[4], hdr->addr2[5], rxphy->rssi);
    #endif
        if(sta_priv->beacon_rssi)
        {
            sta_priv->beacon_rssi = ((rxdesc->phy_rssi<< RSSI_DECIMAL_POINT_SHIFT)
                + ((sta_priv->beacon_rssi<<RSSI_SMOOTHING_SHIFT) - sta_priv->beacon_rssi)) >> RSSI_SMOOTHING_SHIFT;
            rxdesc->phy_rssi = (sta_priv->beacon_rssi >> RSSI_DECIMAL_POINT_SHIFT);
        }
        else
            sta_priv->beacon_rssi = (rxdesc->phy_rssi<< RSSI_DECIMAL_POINT_SHIFT);
    #ifdef SSV_RSSI_DEBUG
        printk("Beacon smoothing RSSI %d %d\n",rxdesc->phy_rssi, sta_priv->beacon_rssi>> RSSI_DECIMAL_POINT_SHIFT);
    #endif

        org_rssi =  rxdesc->phy_rssi;
        if (sc->sh->cfg.external_pa & EX_PA_ENABLE) {
            if (sc->hw_chan <= 14) {
                if (sc->LNA_status) {
                    org_rssi -=  ((sc->sh-> cfg.external_pa & EX_PA_RX_RSSI_OFFSET_P_MASK) >> EX_PA_RX_RSSI_OFFSET_P_SFT);
                } else {
                    org_rssi +=  ((sc->sh-> cfg.external_pa & EX_PA_RX_RSSI_OFFSET_N_MASK) >> EX_PA_RX_RSSI_OFFSET_N_SFT);
                }
            } else {
                if (sc->LNA_status) {
                    org_rssi -=  ((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_P_MASK) >> EX_PA_5G_RSSI_OFFSET_P_SFT);
                } else {
                    org_rssi +=  ((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_N_MASK) >> EX_PA_5G_RSSI_OFFSET_N_SFT);
                }
            }
        }


        {   int assoc;

            assoc = ssvxxx_get_sta_assco_cnt(sc);
            if ((sc->ap_vif == NULL) && (assoc == 1)){
    #ifdef CONFIG_SSV_CCI_IMPROVEMENT
                //ssv6006_update_data_cci_setting(sc, sta_priv, org_rssi);
    #endif
                //_update_green_tx(sc, rxdesc->phy_rssi);
                _set_external_LNA(sc, rxdesc->phy_rssi);
            }
        }

    }

    if ( sc->sh->cfg.beacon_rssi_minimal )
    {
        if ( rxdesc->phy_rssi > sc->sh->cfg.beacon_rssi_minimal )
            rxdesc->phy_rssi = sc->sh->cfg.beacon_rssi_minimal;
    }
    #if 0 /*For debug*/
    printk("beacon %02X:%02X:%02X:%02X:%02X:%02X rxphypad-rpci=%d RxResult=%x wsid=%x\n",
           hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
           hdr->addr2[3], hdr->addr2[4], hdr->addr2[5], rxdesc->rssi, rxdesc->RxResult, wsid);
    #endif
}
#endif

int ssv6006_update_rxstatus(struct ssv_softc *sc,
            struct sk_buff *rx_skb, struct ieee80211_rx_status *rxs)
{
    struct ssv6006_rx_desc *rxdesc = (struct ssv6006_rx_desc *)rx_skb->data;
    struct ieee80211_sta *sta = NULL;
    struct ssv_sta_priv_data *sta_priv = NULL;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)(rx_skb->data + sc->sh->rx_desc_len);
    int    ret = 0;

    sta = ssv6xxx_find_sta_by_rx_skb(sc, rx_skb);

    if (sc->sh->cfg.external_pa & EX_PA_ENABLE) {
        //printk("raw rssi %d ", -rxphy->rssi);
        if (sc->hw_chan <= 14) {
            if (sc->LNA_status) {
                rxdesc->phy_rssi +=  ((sc->sh-> cfg.external_pa & EX_PA_RX_RSSI_OFFSET_P_MASK) >> EX_PA_RX_RSSI_OFFSET_P_SFT);
            } else {
                if (rxdesc->phy_rssi > ((sc->sh-> cfg.external_pa & EX_PA_RX_RSSI_OFFSET_N_MASK) >> EX_PA_RX_RSSI_OFFSET_N_SFT))
                    rxdesc->phy_rssi -=  ((sc->sh-> cfg.external_pa & EX_PA_RX_RSSI_OFFSET_N_MASK) >> EX_PA_RX_RSSI_OFFSET_N_SFT);
                else
                    rxdesc->phy_rssi = 0;
            }
        } else {
            if (sc->LNA_status) {
                rxdesc->phy_rssi +=  ((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_P_MASK) >> EX_PA_5G_RSSI_OFFSET_P_SFT);
            } else {
                if (rxdesc->phy_rssi > ((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_N_MASK) >> EX_PA_5G_RSSI_OFFSET_N_SFT))
                    rxdesc->phy_rssi -=  ((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_N_MASK) >> EX_PA_5G_RSSI_OFFSET_N_SFT);
                else
                    rxdesc->phy_rssi = 0;
            }
        }
        //printk("LNA %d modified rssi %d, poffset, noffset \n", sc->LNA_status, -rxphy->rssi, ((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_P_MASK) >> EX_PA_RX_RSSI_OFFSET_P_SFT),((sc->sh-> cfg.external_pa & EX_PA_5G_RSSI_OFFSET_N_MASK) >> EX_PA_RX_RSSI_OFFSET_N_SFT));
    }

    if (sta){
        sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;

#ifdef CONFIG_SSV_CCI_IMPROVEMENT
        //rx avaible for cci
        sc-> cci_rx_unavailable_counter = 0;
#endif
        if (ieee80211_is_data(hdr->frame_control)){

            sc->rx_data_exist = true;
            _update_rx_data_rate_stats(sc, sta_priv, rxdesc);
        }

        if ((ieee80211_has_protected(hdr->frame_control)) && (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_KRACK)){
            ret = _check_for_krack(sc, sta_priv, rx_skb, hdr);
        }
    }

    if (sta) {
        // update RSSI for beacon mode
        if(ieee80211_is_beacon(hdr->frame_control)){
            sta_priv->beacon_rssi = rxdesc->phy_rssi;
        }
        rxs->signal = (-sta_priv->beacon_rssi);
    } else {
        rxs->signal = (-rxdesc->phy_rssi);
    }

    return ret;
}

static int _ssv6xxx_rc_opertaion(struct ssv_softc *sc, ssv6xxx_rc_ops ops, u32 val)
{
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;
    int ret = 0;

    skb = dev_alloc_skb(HOST_CMD_HDR_LEN + sizeof(u32));
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return -1;
    }

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(u32));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd)+sizeof(u32));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RC_OPS;
    host_cmd->sub_h_cmd = (u32)ops;
    host_cmd->dat32[0] = val;
    host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RC_OPS << 16)|(u16)ops);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(u32);
    ret = HCI_SEND_CMD(sc->sh, skb);

    return 0;
}

static void _ssv6xxx_rc_show_rate_info(struct ssv_cmd_data *cmd_data)
{
    snprintf_res(cmd_data, " - hex value\n");
    snprintf_res(cmd_data, "   [7:6]: phy mode \n");
    snprintf_res(cmd_data, "     [5]: ht40 \n");
    snprintf_res(cmd_data, "     [4]: long/short GI \n");
    snprintf_res(cmd_data, "     [3]: mf \n");
    snprintf_res(cmd_data, "   [2:0]: rate index \n");
    snprintf_res(cmd_data, "    B mode:\n");
    snprintf_res(cmd_data, "     0x0:  1M, 0x1:    2M, 0x2: 5.5M, 0x3:  11M\n");
    snprintf_res(cmd_data, "     with short preamble\n");
    snprintf_res(cmd_data, "     0x11: 2M, 0x12: 5.5M, 0x13: 11M\n");
    snprintf_res(cmd_data, "    G mode:\n");
    snprintf_res(cmd_data, "     0x80:  6M, 0x81:  9M, 0x82: 12M, 0x83: 18M\n");
    snprintf_res(cmd_data, "     0x84: 24M, 0x85: 36M, 0x86: 48M, 0x87: 54M\n");
    snprintf_res(cmd_data, "    N mode:\n");
    snprintf_res(cmd_data, "     HT20:\n");
    snprintf_res(cmd_data, "     with long GI\n");
    snprintf_res(cmd_data, "     0xC0: MCS0, 0xC1: MCS1, 0xC2: MCS2, 0xC3: MCS3\n");
    snprintf_res(cmd_data, "     0xC4: MCS4, 0xC5: MCS5, 0xC6: MCS6, 0xC7: MCS7\n");
    snprintf_res(cmd_data, "     with short GI\n");
    snprintf_res(cmd_data, "     0xD0: MCS0, 0xD1: MCS1, 0xD2: MCS2, 0xD3: MCS3\n");
    snprintf_res(cmd_data, "     0xD4: MCS4, 0xD5: MCS5, 0xD6: MCS6, 0xD7: MCS7\n");
    snprintf_res(cmd_data, "     HT40:\n");
    snprintf_res(cmd_data, "     with long GI\n");
    snprintf_res(cmd_data, "     0xE0: MCS0, 0xE1: MCS1, 0xE2: MCS2, 0xE3: MCS3\n");
    snprintf_res(cmd_data, "     0xE4: MCS4, 0xE5: MCS5, 0xE6: MCS6, 0xE7: MCS7\n");
    snprintf_res(cmd_data, "     with short GI\n");
    snprintf_res(cmd_data, "     0xF0: MCS0, 0xF1: MCS1, 0xF2: MCS2, 0xF3: MCS3\n");
    snprintf_res(cmd_data, "     0xF4: MCS4, 0xF5: MCS5, 0xF6: MCS6, 0xF7: MCS7\n");
}

static void ssv6006_cmd_rc(struct ssv_hw *sh, int argc, char *argv[])
{
    char *endp;
    int  val;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    if (argc < 2) {
        snprintf_res(cmd_data, "\n rc set\n");
        return;
    }
    if (!strcmp(argv[1], "set")){
        if (argc == 4){
            val = simple_strtoul(argv[3], &endp, 0);
            if (!strcmp(argv[2], "auto")){
                snprintf_res(cmd_data, " call cmd rc set auto %s\n", argv[3]);
                _ssv6xxx_rc_opertaion(sh->sc, SSV6XXX_RC_CMD_AUTO_RATE, (u32)val);
                sh->cfg.auto_rate_enable = val;
            } else if (!strcmp(argv[2], "hex")){
                snprintf_res(cmd_data, " call cmd rc set hex %s\n", argv[3]);
                _ssv6xxx_rc_show_rate_info(cmd_data);
                _ssv6xxx_rc_opertaion(sh->sc, SSV6XXX_RC_CMD_FIXED_RATE, (u32)val);
            } else {
                snprintf_res(cmd_data, "\n rc set auto | hex [val]\n");
                return;
            }
        }
        else {
            snprintf_res(cmd_data, "\n rc set auto | hex\n");
            _ssv6xxx_rc_show_rate_info(cmd_data);
            return;
        }
    }
    else
    {
        snprintf_res(cmd_data, "\n rc set\n");
    }
    return;
}

static void ssv6006_rc_algorithm(struct ssv_softc *sc)
{
    return;
}

static void ssv6006_set_80211_hw_rate_config(struct ssv_softc *sc)
{
	struct ieee80211_hw *hw=sc->hw;

	hw->max_rates = SSV6006RC_MAX_RATE_SERIES;
	hw->max_rate_tries = SSV6006RC_MAX_RATE_RETRY;
}

static void ssv6006_rc_rx_data_handler(struct ssv_softc *sc, struct sk_buff *skb, u32 rate_index)
{
    if (sc->log_ctrl & LOG_RX_DESC){
	    struct ieee80211_sta *sta;
	    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)(skb->data + sc->sh->rx_desc_len);

        if (!(ieee80211_is_beacon(hdr->frame_control))){   // log for not beacon frame
            sta = ssv6xxx_find_sta_by_rx_skb(sc, skb);
            if (sta != NULL)
                ssv6006_dump_rx_desc(skb);
        }
    }
}

void ssv6006_rc_legacy_bitrate_to_rate_desc(int bitrate, u8 *drate)
{
	/*
	 * build the mapping with mac80211 rate index and ssv rate descript.
	 * mac80211 rate index   <---->   ssv rate descript
	 * b mode 1M             <---->   0x00
	 * b mode 2M             <---->   0x01
	 * b mode 5.5M           <---->   0x02
	 * b mode 11M            <---->   0x03
	 * g mode 6M             <---->   0x80
	 * g mode 9M             <---->   0x81
	 */
	*drate = 0;

	switch (bitrate) {
		case 10:
				*drate = ((SSV6006RC_B_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_B_1M << SSV6006RC_RATE_SFT));
			return;
		case 20:
				*drate = ((SSV6006RC_B_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_B_2M << SSV6006RC_RATE_SFT));
			return;
		case 55:
				*drate = ((SSV6006RC_B_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_B_5_5M << SSV6006RC_RATE_SFT));
			return;
		case 110:
				*drate = ((SSV6006RC_B_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_B_11M << SSV6006RC_RATE_SFT));
			return;
		case 60:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_6M << SSV6006RC_RATE_SFT));
			return;
		case 90:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_9M << SSV6006RC_RATE_SFT));
			return;
		case 120:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_12M << SSV6006RC_RATE_SFT));
			return;
		case 180:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_18M << SSV6006RC_RATE_SFT));
			return;
		case 240:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_24M << SSV6006RC_RATE_SFT));
			return;
		case 360:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_36M << SSV6006RC_RATE_SFT));
			return;
		case 480:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_48M << SSV6006RC_RATE_SFT));
			return;
		case 540:
				*drate = ((SSV6006RC_G_MODE << SSV6006RC_PHY_MODE_SFT) | (SSV6006RC_G_54M << SSV6006RC_RATE_SFT));
			return;
		default:
				printk("For B/G mode, it doesn't support the bitrate %d kbps\n", bitrate * 100);
				WARN_ON(1);
			return;
	}
}

void ssv_attach_ssv6006_phy(struct ssv_hal_ops *hal_ops)
{
    hal_ops->add_txinfo = ssv6006_add_txinfo;
    hal_ops->update_txinfo = ssv6006_update_txinfo;
    hal_ops->update_ampdu_txinfo = ssv6006_update_ampdu_txinfo;
    hal_ops->add_ampdu_txinfo = ssv6006_add_ampdu_txinfo;
    hal_ops->update_null_func_txinfo = ssv6006_update_null_func_txinfo;
    hal_ops->get_tx_desc_size = ssv6006_get_tx_desc_size;
    hal_ops->get_tx_desc_ctype = ssv6006_get_tx_desc_ctype;
    hal_ops->get_tx_desc_reason = ssv6006_get_tx_desc_reason;
    hal_ops->get_tx_desc_wsid = ssv6006_get_tx_desc_wsid;
    hal_ops->get_tx_desc_txq_idx = ssv6006_get_tx_desc_txq_idx;
    hal_ops->txtput_set_desc = ssv6006_txtput_set_desc;
    hal_ops->fill_beacon_tx_desc = ssv6006_fill_beacon_tx_desc;

    hal_ops->get_tkip_mmic_err = ssv6006_get_tkip_mmic_err;
    hal_ops->get_rx_desc_size = ssv6006_get_rx_desc_size;
    hal_ops->get_rx_desc_length = ssv6006_get_rx_desc_length;
    hal_ops->get_rx_desc_wsid = ssv6006_get_rx_desc_wsid;
    hal_ops->get_rx_desc_rate_idx = ssv6006_get_rx_desc_rate_idx;
    hal_ops->get_rx_desc_mng_used = ssv6006_get_rx_desc_mng_used;
    hal_ops->is_rx_aggr = ssv6006_is_rx_aggr;
    hal_ops->get_rx_desc_ctype = ssv6006_get_rx_desc_ctype;
    hal_ops->get_rx_desc_hdr_offset = ssv6006_get_rx_desc_hdr_offset;
    hal_ops->get_rx_desc_info_hdr = ssv6006_get_rx_desc_info_hdr;
    hal_ops->get_rx_desc_phy_rssi = ssv6006_get_rx_desc_phy_rssi;
    hal_ops->nullfun_frame_filter = ssv6006_nullfun_frame_filter;

    hal_ops->phy_enable = ssv6006_phy_enable;
    hal_ops->set_phy_mode = ssv6006_set_phy_mode;
    hal_ops->reset_mib_phy = ssv6006_reset_mib_phy;
    hal_ops->dump_mib_rx_phy = ssv6006_dump_mib_rx_phy;
    hal_ops->update_mac80211_chan_info = ssv6006_update_mac80211_chan_info;
    hal_ops->rc_mac80211_rate_idx = ssv6006_rc_mac80211_rate_idx;
    hal_ops->rc_mac80211_tx_rate_idx = ssv6006_rc_mac80211_tx_rate_idx;
    hal_ops->update_rxstatus = ssv6006_update_rxstatus;
#ifdef CONFIG_SSV_CCI_IMPROVEMENT
    hal_ops->update_scan_cci_setting = ssv6006_update_scan_cci_setting;
    hal_ops->recover_scan_cci_setting = ssv6006_recover_scan_cci_setting;
#endif
    hal_ops->cmd_rc = ssv6006_cmd_rc;

	hal_ops->rc_algorithm = ssv6006_rc_algorithm;
	hal_ops->set_80211_hw_rate_config = ssv6006_set_80211_hw_rate_config;
	hal_ops->rc_legacy_bitrate_to_rate_desc = ssv6006_rc_legacy_bitrate_to_rate_desc;
	hal_ops->rc_rx_data_handler = ssv6006_rc_rx_data_handler;
}
#endif
