/**
 ******************************************************************************
 *
 * @file bl_events.h
 *
 * @brief Trace events definition
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM bl

#if !defined(_BL_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _BL_EVENTS_H

#include <linux/tracepoint.h>

#include "bl_compat.h"

/*****************************************************************************
 * TRACE function for MGMT TX (FULLMAC)
 ****************************************************************************/
#ifdef CONFIG_BL_FULLMAC
#include "linux/ieee80211.h"
#if defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS)
#include <linux/trace_seq.h>
#include <linux/version.h>

/* P2P Public Action Frames Definitions (see WiFi P2P Technical Specification, section 4.2.8) */
/* IEEE 802.11 Public Action Usage Category - Define P2P public action frames */
#define MGMT_ACTION_PUBLIC_CAT              (0x04)
/* Offset of OUI Subtype field in P2P Action Frame format */
#define MGMT_ACTION_OUI_SUBTYPE_OFFSET      (6)
/* P2P Public Action Frame Types */
enum p2p_action_type {
    P2P_ACTION_GO_NEG_REQ   = 0,    /* GO Negociation Request */
    P2P_ACTION_GO_NEG_RSP,          /* GO Negociation Response */
    P2P_ACTION_GO_NEG_CFM,          /* GO Negociation Confirmation */
    P2P_ACTION_INVIT_REQ,           /* P2P Invitation Request */
    P2P_ACTION_INVIT_RSP,           /* P2P Invitation Response */
    P2P_ACTION_DEV_DISC_REQ,        /* Device Discoverability Request */
    P2P_ACTION_DEV_DISC_RSP,        /* Device Discoverability Response */
    P2P_ACTION_PROV_DISC_REQ,       /* Provision Discovery Request */
    P2P_ACTION_PROV_DISC_RSP,       /* Provision Discovery Response */
};

const char *ftrace_print_mgmt_info(struct trace_seq *p, u16 frame_control, u8 cat, u8 type, u8 p2p) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    const char *ret = trace_seq_buffer_ptr(p);
#else
    const char *ret = p->buffer + p->len;
#endif

    switch (frame_control & IEEE80211_FCTL_STYPE) {
        case (IEEE80211_STYPE_ASSOC_REQ): trace_seq_printf(p, "Association Request"); break;
        case (IEEE80211_STYPE_ASSOC_RESP): trace_seq_printf(p, "Association Response"); break;
        case (IEEE80211_STYPE_REASSOC_REQ): trace_seq_printf(p, "Reassociation Request"); break;
        case (IEEE80211_STYPE_REASSOC_RESP): trace_seq_printf(p, "Reassociation Response"); break;
        case (IEEE80211_STYPE_PROBE_REQ): trace_seq_printf(p, "Probe Request"); break;
        case (IEEE80211_STYPE_PROBE_RESP): trace_seq_printf(p, "Probe Response"); break;
        case (IEEE80211_STYPE_BEACON): trace_seq_printf(p, "Beacon"); break;
        case (IEEE80211_STYPE_ATIM): trace_seq_printf(p, "ATIM"); break;
        case (IEEE80211_STYPE_DISASSOC): trace_seq_printf(p, "Disassociation"); break;
        case (IEEE80211_STYPE_AUTH): trace_seq_printf(p, "Authentication"); break;
        case (IEEE80211_STYPE_DEAUTH): trace_seq_printf(p, "Deauthentication"); break;
        case (IEEE80211_STYPE_ACTION):
            trace_seq_printf(p, "Action");
            if (cat == MGMT_ACTION_PUBLIC_CAT && type == 0x9)
                switch (p2p) {
                    case (P2P_ACTION_GO_NEG_REQ): trace_seq_printf(p, ": GO Negociation Request"); break;
                    case (P2P_ACTION_GO_NEG_RSP): trace_seq_printf(p, ": GO Negociation Response"); break;
                    case (P2P_ACTION_GO_NEG_CFM): trace_seq_printf(p, ": GO Negociation Confirmation"); break;
                    case (P2P_ACTION_INVIT_REQ): trace_seq_printf(p, ": P2P Invitation Request"); break;
                    case (P2P_ACTION_INVIT_RSP): trace_seq_printf(p, ": P2P Invitation Response"); break;
                    case (P2P_ACTION_DEV_DISC_REQ): trace_seq_printf(p, ": Device Discoverability Request"); break;
                    case (P2P_ACTION_DEV_DISC_RSP): trace_seq_printf(p, ": Device Discoverability Response"); break;
                    case (P2P_ACTION_PROV_DISC_REQ): trace_seq_printf(p, ": Provision Discovery Request"); break;
                    case (P2P_ACTION_PROV_DISC_RSP): trace_seq_printf(p, ": Provision Discovery Response"); break;
                    default: trace_seq_printf(p, "Unknown p2p %d", p2p); break;
                }
            else {
                switch (cat) {
                    case 0: trace_seq_printf(p, ":Spectrum %d", type); break;
                    case 1: trace_seq_printf(p, ":QOS %d", type); break;
                    case 2: trace_seq_printf(p, ":DLS %d", type); break;
                    case 3: trace_seq_printf(p, ":BA %d", type); break;
                    case 4: trace_seq_printf(p, ":Public %d", type); break;
                    case 5: trace_seq_printf(p, ":Radio Measure %d", type); break;
                    case 6: trace_seq_printf(p, ":Fast BSS %d", type); break;
                    case 7: trace_seq_printf(p, ":HT Action %d", type); break;
                    case 8: trace_seq_printf(p, ":SA Query %d", type); break;
                    case 9: trace_seq_printf(p, ":Protected Public %d", type); break;
                    case 10: trace_seq_printf(p, ":WNM %d", type); break;
                    case 11: trace_seq_printf(p, ":Unprotected WNM %d", type); break;
                    case 12: trace_seq_printf(p, ":TDLS %d", type); break;
                    case 13: trace_seq_printf(p, ":Mesh %d", type); break;
                    case 14: trace_seq_printf(p, ":MultiHop %d", type); break;
                    case 15: trace_seq_printf(p, ":Self Protected %d", type); break;
                    case 126: trace_seq_printf(p, ":Vendor protected"); break;
                    case 127: trace_seq_printf(p, ":Vendor"); break;
                    default: trace_seq_printf(p, ":Unknown category %d", cat); break;
                }
            }
            break;
        default: trace_seq_printf(p, "Unknown subtype %d", frame_control & IEEE80211_FCTL_STYPE); break;
    }

    trace_seq_putc(p, 0);

    return ret;
}
#endif /* defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS) */

#undef __print_mgmt_info
#define __print_mgmt_info(frame_control, cat, type, p2p) ftrace_print_mgmt_info(p, frame_control, cat, type, p2p)

TRACE_EVENT(
    roc,
    TP_PROTO(u8 vif_idx, u16 freq, unsigned int duration),
    TP_ARGS(vif_idx, freq, duration),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
        __field(u16, freq)
        __field(unsigned int, duration)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
        __entry->freq = freq;
        __entry->duration = duration;
                   ),
    TP_printk("f=%d vif=%d dur=%d",
            __entry->freq, __entry->vif_idx, __entry->duration)
);

TRACE_EVENT(
    cancel_roc,
    TP_PROTO(u8 vif_idx),
    TP_ARGS(vif_idx),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
                   ),
    TP_printk("vif=%d", __entry->vif_idx)
);

TRACE_EVENT(
    roc_exp,
    TP_PROTO(u8 vif_idx),
    TP_ARGS(vif_idx),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
                   ),
    TP_printk("vif=%d", __entry->vif_idx)
);

TRACE_EVENT(
    switch_roc,
    TP_PROTO(u8 vif_idx),
    TP_ARGS(vif_idx),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
                   ),
    TP_printk("vif=%d", __entry->vif_idx)
);

DECLARE_EVENT_CLASS(
    mgmt_template,
    TP_PROTO(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt),
    TP_ARGS(freq, vif_idx, sta_idx, mgmt),
    TP_STRUCT__entry(
        __field(u16, freq)
        __field(u8, vif_idx)
        __field(u8, sta_idx)
        __field(u16, frame_control)
        __field(u8, action_cat)
        __field(u8, action_type)
        __field(u8, action_p2p)
        __array(u8, sa, 6)
        __array(u8, da, 6)
        __array(u8, bssid, 6)
        __array(u8, name, 8)
                     ),
    TP_fast_assign(
		int i;
        __entry->freq = freq;
        __entry->vif_idx = vif_idx;
        __entry->sta_idx = sta_idx;
        __entry->frame_control = mgmt->frame_control;
        __entry->action_cat = mgmt->u.action.category;
        __entry->action_type = mgmt->u.action.u.wme_action.action_code;
        __entry->action_p2p = *((u8 *)&mgmt->u.action.category
                                 + MGMT_ACTION_OUI_SUBTYPE_OFFSET);
		for(i=0; i<6; i++) {
			__entry->sa[i] = mgmt->sa[i];
			__entry->da[i] = mgmt->da[i];
			__entry->bssid[i] = mgmt->bssid[i];

		}
		for(i=0; i<8; i++) {
			__entry->name[i] = mgmt->u.beacon.variable[i];
		}
        		),
    TP_printk("f=%d vif=%d sta=%d -> %s, sa:%x:%x:%x:%x:%x:%x, \
			   da:%x:%x:%x:%x:%x:%x, bssid:%x:%x:%x:%x:%x:%x, \
			   name: %c%c%c%c%c%c", 
            __entry->freq, __entry->vif_idx, __entry->sta_idx,
              __print_mgmt_info(__entry->frame_control, __entry->action_cat,
                                __entry->action_type, __entry->action_p2p),
			 __entry->sa[0], __entry->sa[1], __entry->sa[2], __entry->sa[3], __entry->sa[4], __entry->sa[5],
			 __entry->da[0], __entry->da[1], __entry->da[2], __entry->da[3], __entry->da[4], __entry->da[5],
			 __entry->bssid[0], __entry->bssid[1], __entry->bssid[2], __entry->bssid[3], __entry->bssid[4], __entry->bssid[5],
			 __entry->name[2], __entry->name[3], __entry->name[4], __entry->name[5], __entry->name[6], __entry->name[7])
);

DEFINE_EVENT(mgmt_template, mgmt_tx,
             TP_PROTO(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt),
             TP_ARGS(freq, vif_idx, sta_idx, mgmt));

DEFINE_EVENT(mgmt_template, mgmt_rx,
             TP_PROTO(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt),
             TP_ARGS(freq, vif_idx, sta_idx, mgmt));

TRACE_EVENT(
    mgmt_cfm,
    TP_PROTO(u8 vif_idx, u8 sta_idx, bool acked),
    TP_ARGS(vif_idx, sta_idx, acked),
    TP_STRUCT__entry(
        __field(u8, vif_idx)
        __field(u8, sta_idx)
        __field(bool, acked)
                     ),
    TP_fast_assign(
        __entry->vif_idx = vif_idx;
        __entry->sta_idx = sta_idx;
        __entry->acked = acked;
                   ),
    TP_printk("vif=%d sta=%d ack=%d",
            __entry->vif_idx, __entry->sta_idx, __entry->acked)
);
#endif /* CONFIG_BL_FULLMAC */

/*****************************************************************************
 * TRACE function for TXQ
 ****************************************************************************/
#include "bl_tx.h"
#if defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS)

#include <linux/trace_seq.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
#include <linux/trace_events.h>
#else
#include <linux/ftrace_event.h>
#endif


const char *
ftrace_print_txq(struct trace_seq *p, int txq_idx) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    const char *ret = trace_seq_buffer_ptr(p);
#else
    const char *ret = p->buffer + p->len;
#endif

    if (txq_idx == TXQ_INACTIVE) {
        trace_seq_printf(p, "[INACTIVE]");
    } else if (txq_idx < NX_FIRST_VIF_TXQ_IDX) {
        trace_seq_printf(p, "[STA %d/%d]",
                         txq_idx / NX_NB_TXQ_PER_STA,
                         txq_idx % NX_NB_TXQ_PER_STA);
#ifdef CONFIG_BL_FULLMAC
    } else if (txq_idx < NX_FIRST_UNK_TXQ_IDX) {
        trace_seq_printf(p, "[BC/MC %d]",
                         txq_idx - NX_FIRST_BCMC_TXQ_IDX);
    } else if (txq_idx < NX_OFF_CHAN_TXQ_IDX) {
        trace_seq_printf(p, "[UNKNOWN %d]",
                         txq_idx - NX_FIRST_UNK_TXQ_IDX);
    } else if (txq_idx == NX_OFF_CHAN_TXQ_IDX) {
        trace_seq_printf(p, "[OFFCHAN]");
#else
    } else if (txq_idx < NX_NB_TXQ) {
        txq_idx -= NX_FIRST_VIF_TXQ_IDX;
        trace_seq_printf(p, "[VIF %d/%d]",
                         txq_idx / NX_NB_TXQ_PER_VIF,
                         txq_idx % NX_NB_TXQ_PER_VIF);
#endif
    } else {
        trace_seq_printf(p, "[ERROR %d]", txq_idx);
    }

    trace_seq_putc(p, 0);

    return ret;
}

const char *
ftrace_print_sta(struct trace_seq *p, int sta_idx) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    const char *ret = trace_seq_buffer_ptr(p);
#else
    const char *ret = p->buffer + p->len;
#endif

    if (sta_idx < NX_REMOTE_STA_MAX) {
        trace_seq_printf(p, "[STA %d]", sta_idx);
    } else {
        trace_seq_printf(p, "[BC/MC %d]", sta_idx - NX_REMOTE_STA_MAX);
    }

    trace_seq_putc(p, 0);

    return ret;
}

const char *
ftrace_print_hwq(struct trace_seq *p, int hwq_idx) {

    static const struct trace_print_flags symbols[] =
        {{BL_HWQ_BK, "BK"},
         {BL_HWQ_BE, "BE"},
         {BL_HWQ_VI, "VI"},
         {BL_HWQ_VO, "VO"},
#ifdef CONFIG_BL_FULLMAC
         {BL_HWQ_BCMC, "BCMC"},
#else
         {BL_HWQ_BCN, "BCN"},
#endif
         { -1, NULL }};
    return trace_print_symbols_seq(p, hwq_idx, symbols);
}

const char *
ftrace_print_hwq_cred(struct trace_seq *p, u8 *cred) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    const char *ret = trace_seq_buffer_ptr(p);
#else
    const char *ret = p->buffer + p->len;
#endif

#if CONFIG_USER_MAX == 1
    trace_seq_printf(p, "%d", cred[0]);
#else
    int i;

    for (i = 0; i < CONFIG_USER_MAX - 1; i++)
        trace_seq_printf(p, "%d-", cred[i]);
    trace_seq_printf(p, "%d", cred[i]);
#endif

    trace_seq_putc(p, 0);
    return ret;
}

const char *
ftrace_print_mu_info(struct trace_seq *p, u8 mu_info) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    const char *ret = trace_seq_buffer_ptr(p);
#else
    const char *ret = p->buffer + p->len;
#endif

    if (mu_info)
        trace_seq_printf(p, "MU: %d-%d", (mu_info & 0x3f), (mu_info >> 6));

    trace_seq_putc(p, 0);
    return ret;
}

const char *
ftrace_print_mu_group(struct trace_seq *p, int nb_user, u8 *users) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    const char *ret = trace_seq_buffer_ptr(p);
#else
    const char *ret = p->buffer + p->len;
#endif
    int i;

    if (users[0] != 0xff)
        trace_seq_printf(p, "(%d", users[0]);
    else
        trace_seq_printf(p, "(-");
    for (i = 1; i < CONFIG_USER_MAX ; i++) {
        if (users[i] != 0xff)
            trace_seq_printf(p, ",%d", users[i]);
        else
            trace_seq_printf(p, ",-");
    }

    trace_seq_printf(p, ")");
    trace_seq_putc(p, 0);
    return ret;
}

#endif /* defined(CONFIG_TRACEPOINTS) && defined(CREATE_TRACE_POINTS) */

#undef __print_txq
#define __print_txq(txq_idx) ftrace_print_txq(p, txq_idx)

#undef __print_sta
#define __print_sta(sta_idx) ftrace_print_sta(p, sta_idx)

#undef __print_hwq
#define __print_hwq(hwq) ftrace_print_hwq(p, hwq)

#undef __print_hwq_cred
#define __print_hwq_cred(cred) ftrace_print_hwq_cred(p, cred)

#undef __print_mu_info
#define __print_mu_info(mu_info) ftrace_print_mu_info(p, mu_info)

#undef __print_mu_group
#define __print_mu_group(nb, users) ftrace_print_mu_group(p, nb, users)

#ifdef CONFIG_BL_FULLMAC

TRACE_EVENT(
    txq_select,
    TP_PROTO(int txq_idx, u16 pkt_ready_up, struct sk_buff *skb),
    TP_ARGS(txq_idx, pkt_ready_up, skb),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, pkt_ready)
        __field(struct sk_buff *, skb)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq_idx;
        __entry->pkt_ready = pkt_ready_up;
        __entry->skb = skb;
                   ),
    TP_printk("%s pkt_ready_up=%d skb=%p", __print_txq(__entry->txq_idx),
              __entry->pkt_ready, __entry->skb)
);

#endif /* CONFIG_BL_FULLMAC */

DECLARE_EVENT_CLASS(
    hwq_template,
    TP_PROTO(u8 hwq_idx),
    TP_ARGS(hwq_idx),
    TP_STRUCT__entry(
        __field(u8, hwq_idx)
                     ),
    TP_fast_assign(
        __entry->hwq_idx = hwq_idx;
                   ),
    TP_printk("%s", __print_hwq(__entry->hwq_idx))
);

DEFINE_EVENT(hwq_template, hwq_flowctrl_stop,
             TP_PROTO(u8 hwq_idx),
             TP_ARGS(hwq_idx));

DEFINE_EVENT(hwq_template, hwq_flowctrl_start,
             TP_PROTO(u8 hwq_idx),
             TP_ARGS(hwq_idx));


DECLARE_EVENT_CLASS(
    txq_template,
    TP_PROTO(struct bl_txq *txq),
    TP_ARGS(txq),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
                   ),
    TP_printk("%s", __print_txq(__entry->txq_idx))
);

DEFINE_EVENT(txq_template, txq_add_to_hw,
             TP_PROTO(struct bl_txq *txq),
             TP_ARGS(txq));

DEFINE_EVENT(txq_template, txq_del_from_hw,
             TP_PROTO(struct bl_txq *txq),
             TP_ARGS(txq));

#ifdef CONFIG_BL_FULLMAC

DEFINE_EVENT(txq_template, txq_flowctrl_stop,
             TP_PROTO(struct bl_txq *txq),
             TP_ARGS(txq));

DEFINE_EVENT(txq_template, txq_flowctrl_restart,
             TP_PROTO(struct bl_txq *txq),
             TP_ARGS(txq));

#endif  /* CONFIG_BL_FULLMAC */

TRACE_EVENT(
    process_txq,
    TP_PROTO(struct bl_txq *txq),
    TP_ARGS(txq),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, len)
        __field(u16, len_retry)
        __field(s8, credit)
        #ifdef CONFIG_BL_FULLMAC
        __field(u16, limit)
        #endif /* CONFIG_BL_FULLMAC*/
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->len = skb_queue_len(&txq->sk_list);
        __entry->len_retry = txq->nb_retry;
        __entry->credit = txq->credits;
        #ifdef CONFIG_BL_FULLMAC
        __entry->limit = txq->push_limit;
        #endif /* CONFIG_BL_FULLMAC*/
                   ),

    #ifdef CONFIG_BL_FULLMAC
    TP_printk("%s txq_credits=%d, len=%d, retry_len=%d, push_limit=%d",
              __print_txq(__entry->txq_idx), __entry->credit,
              __entry->len, __entry->len_retry, __entry->limit)
    #else
    TP_printk("%s txq_credits=%d, len=%d, retry_len=%d",
              __print_txq(__entry->txq_idx), __entry->credit,
              __entry->len, __entry->len_retry)
    #endif /* CONFIG_BL_FULLMAC*/
);

DECLARE_EVENT_CLASS(
    txq_reason_template,
    TP_PROTO(struct bl_txq *txq, u16 reason),
    TP_ARGS(txq, reason),
    TP_STRUCT__entry(
        __field(u16, txq_idx)
        __field(u16, reason)
        __field(u16, status)
                     ),
    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->reason = reason;
        __entry->status = txq->status;
                   ),
    TP_printk("%s reason=%s status=%s",
              __print_txq(__entry->txq_idx),
              __print_symbolic(__entry->reason,
                               {BL_TXQ_STOP_FULL, "FULL"},
                               {BL_TXQ_STOP_CSA, "CSA"},
                               {BL_TXQ_STOP_STA_PS, "PS"},
                               {BL_TXQ_STOP_VIF_PS, "VPS"},
                               {BL_TXQ_STOP_CHAN, "CHAN"},
                               {BL_TXQ_STOP_MU_POS, "MU"}),
              __print_flags(__entry->status, "|",
                            {BL_TXQ_IN_HWQ_LIST, "IN LIST"},
                            {BL_TXQ_STOP_FULL, "FULL"},
                            {BL_TXQ_STOP_CSA, "CSA"},
                            {BL_TXQ_STOP_STA_PS, "PS"},
                            {BL_TXQ_STOP_VIF_PS, "VPS"},
                            {BL_TXQ_STOP_CHAN, "CHAN"},
                            {BL_TXQ_STOP_MU_POS, "MU"},
                            {BL_TXQ_NDEV_FLOW_CTRL, "FLW_CTRL"}))
);

DEFINE_EVENT(txq_reason_template, txq_start,
             TP_PROTO(struct bl_txq *txq, u16 reason),
             TP_ARGS(txq, reason));

DEFINE_EVENT(txq_reason_template, txq_stop,
             TP_PROTO(struct bl_txq *txq, u16 reason),
             TP_ARGS(txq, reason));


TRACE_EVENT(
    push_desc,
    TP_PROTO(struct sk_buff *skb, struct bl_sw_txhdr *sw_txhdr, int push_flags),

    TP_ARGS(skb, sw_txhdr, push_flags),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(unsigned int, len)
        __field(u16, tx_queue)
        __field(u8, hw_queue)
        __field(u8, push_flag)
        __field(u32, flag)
        __field(s8, txq_cred)
        __field(u8, hwq_cred)
        __field(u8, mu_info)
                     ),
    TP_fast_assign(
        __entry->skb = skb;
        __entry->tx_queue = sw_txhdr->txq->idx;
        __entry->push_flag = push_flags;
        __entry->hw_queue = sw_txhdr->txq->hwq->id;
        __entry->txq_cred = sw_txhdr->txq->credits;
        __entry->hwq_cred = sw_txhdr->txq->hwq->credits[BL_TXQ_POS_ID(sw_txhdr->txq)];

#ifdef CONFIG_BL_FULLMAC
        __entry->flag = sw_txhdr->desc.host.flags;
#ifdef CONFIG_BL_SPLIT_TX_BUF
#ifdef CONFIG_BL_AMSDUS_TX
        if (sw_txhdr->amsdu.len)
            __entry->len = sw_txhdr->amsdu.len;
        else
#endif /* CONFIG_BL_AMSDUS_TX */
            __entry->len = sw_txhdr->desc.host.packet_len[0];
#else
        __entry->len = sw_txhdr->desc.host.packet_len;
#endif /* CONFIG_BL_SPLIT_TX_BUF */

#else /* CONFIG_BL_FULLMAC */
        __entry->flag = sw_txhdr->desc.umac.flags;
        __entry->len = sw_txhdr->frame_len;
        __entry->sn = sw_txhdr->sn;
#endif /* CONFIG_BL_FULLMAC */
        __entry->mu_info = 0;
                   ),

#ifdef CONFIG_BL_FULLMAC
    TP_printk("%s skb=%p (len=%d) hw_queue=%s cred_txq=%d cred_hwq=%d %s flag=%s %s%s",
              __print_txq(__entry->tx_queue), __entry->skb, __entry->len,
              __print_hwq(__entry->hw_queue),
              __entry->txq_cred, __entry->hwq_cred,
              __print_mu_info(__entry->mu_info),
              __print_flags(__entry->flag, "|",
                            {TXU_CNTRL_RETRY, "RETRY"},
                            {TXU_CNTRL_MORE_DATA, "MOREDATA"},
                            {TXU_CNTRL_MGMT, "MGMT"},
                            {TXU_CNTRL_MGMT_NO_CCK, "NO_CCK"},
                            {TXU_CNTRL_MGMT_ROBUST, "ROBUST"},
                            {TXU_CNTRL_AMSDU, "AMSDU"},
                            {TXU_CNTRL_USE_4ADDR, "4ADDR"},
                            {TXU_CNTRL_EOSP, "EOSP"},
                            {TXU_CNTRL_MESH_FWD, "MESH_FWD"},
                            {TXU_CNTRL_TDLS, "TDLS"}),
              (__entry->push_flag & BL_PUSH_IMMEDIATE) ? "(IMMEDIATE)" : "",
              (!(__entry->flag & TXU_CNTRL_RETRY) &&
               (__entry->push_flag & BL_PUSH_RETRY)) ? "(SW_RETRY)" : "")
#else
    TP_printk("%s skb=%p (len=%d) hw_queue=%s cred_txq=%d cred_hwq=%d %s flag=%x (%s) sn=%d",
              __print_txq(__entry->tx_queue), __entry->skb, __entry->len,
              __print_hwq(__entry->hw_queue), __entry->txq_cred, __entry->hwq_cred,
              __print_mu_info(__entry->mu_info),
              __entry->flag,
              __print_flags(__entry->push_flag, "|",
                            {BL_PUSH_RETRY, "RETRY"},
                            {BL_PUSH_IMMEDIATE, "IMMEDIATE"}),
              __entry->sn)
#endif /* CONFIG_BL_FULLMAC */
);


TRACE_EVENT(
    txq_queue_skb,
    TP_PROTO(struct sk_buff *skb, struct bl_txq *txq, bool retry),
    TP_ARGS(skb, txq, retry),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(s8, credit)
        __field(u16, q_len)
        __field(u16, q_len_retry)
        __field(bool, retry)
                     ),
    TP_fast_assign(
        __entry->skb = skb;
        __entry->txq_idx = txq->idx;
        __entry->credit = txq->credits;
        __entry->q_len = skb_queue_len(&txq->sk_list);
        __entry->q_len_retry = txq->nb_retry;
        __entry->retry = retry;
                   ),

    TP_printk("%s skb=%p retry=%d txq_credits=%d queue_len=%d (retry = %d)",
              __print_txq(__entry->txq_idx), __entry->skb, __entry->retry,
              __entry->credit, __entry->q_len, __entry->q_len_retry)
);


DECLARE_EVENT_CLASS(
    idx_template,
    TP_PROTO(u16 idx),
    TP_ARGS(idx),
    TP_STRUCT__entry(
        __field(u16, idx)
                     ),
    TP_fast_assign(
        __entry->idx = idx;
                   ),
    TP_printk("idx=%d", __entry->idx)
);


DEFINE_EVENT(idx_template, txq_vif_start,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

DEFINE_EVENT(idx_template, txq_vif_stop,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

TRACE_EVENT(
    process_hw_queue,
    TP_PROTO(struct bl_hwq *hwq),
    TP_ARGS(hwq),
    TP_STRUCT__entry(
        __field(u16, hwq)
        __array(u8, credits, CONFIG_USER_MAX)
                     ),
    TP_fast_assign(
        int i;
        __entry->hwq = hwq->id;
        for (i=0; i < CONFIG_USER_MAX; i ++)
            __entry->credits[i] = hwq->credits[i];
                   ),
    TP_printk("hw_queue=%s hw_credits=%s",
              __print_hwq(__entry->hwq), __print_hwq_cred(__entry->credits))
);

DECLARE_EVENT_CLASS(
    sta_idx_template,
    TP_PROTO(u16 idx),
    TP_ARGS(idx),
    TP_STRUCT__entry(
        __field(u16, idx)
                     ),
    TP_fast_assign(
        __entry->idx = idx;
                   ),
    TP_printk("%s", __print_sta(__entry->idx))
);

DEFINE_EVENT(sta_idx_template, txq_sta_start,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

DEFINE_EVENT(sta_idx_template, txq_sta_stop,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

#ifdef CONFIG_BL_FULLMAC

DEFINE_EVENT(sta_idx_template, ps_disable,
             TP_PROTO(u16 idx),
             TP_ARGS(idx));

#endif  /* CONFIG_BL_FULLMAC */

TRACE_EVENT(
    skb_confirm,
    TP_PROTO(struct sk_buff *skb, struct bl_txq *txq, struct bl_hwq *hwq,
#ifdef CONFIG_BL_FULLMAC
             struct tx_cfm_tag *cfm
#else
             u8 cfm
#endif
             ),

    TP_ARGS(skb, txq, hwq, cfm),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(u8, hw_queue)
        __array(u8, hw_credit, CONFIG_USER_MAX)
        __field(s8, sw_credit)
        __field(s8, sw_credit_up)
#ifdef CONFIG_BL_FULLMAC
        __field(u8, ampdu_size)
#ifdef CONFIG_BL_SPLIT_TX_BUF
        __field(u16, amsdu)
#endif /* CONFIG_BL_SPLIT_TX_BUF */
#endif /* CONFIG_BL_FULLMAC*/
                     ),

    TP_fast_assign(
        int i;
        __entry->skb = skb;
        __entry->txq_idx = txq->idx;
        __entry->hw_queue = hwq->id;
        for (i = 0 ; i < CONFIG_USER_MAX ; i++)
            __entry->hw_credit[i] = hwq->credits[i];
        __entry->sw_credit = txq->credits;
#if defined CONFIG_BL_FULLMAC
        __entry->sw_credit_up = cfm->credits;
        __entry->ampdu_size = cfm->ampdu_size;
#ifdef CONFIG_BL_SPLIT_TX_BUF
        __entry->amsdu = cfm->amsdu_size;
#endif
#else
        __entry->sw_credit_up = cfm
#endif /* CONFIG_BL_FULLMAC */
                   ),

    TP_printk("%s skb=%p hw_queue=%s, hw_credits=%s, txq_credits=%d (+%d)"
#ifdef CONFIG_BL_FULLMAC
              " ampdu=%d"
#ifdef CONFIG_BL_SPLIT_TX_BUF
              " amsdu=%u"
#endif
#endif
              , __print_txq(__entry->txq_idx), __entry->skb,
              __print_hwq(__entry->hw_queue),
              __print_hwq_cred(__entry->hw_credit),
               __entry->sw_credit, __entry->sw_credit_up
#ifdef CONFIG_BL_FULLMAC
              , __entry->ampdu_size
#ifdef CONFIG_BL_SPLIT_TX_BUF
              , __entry->amsdu
#endif
#endif
              )
);

TRACE_EVENT(
    credit_update,
    TP_PROTO(struct bl_txq *txq, s8_l cred_up),

    TP_ARGS(txq, cred_up),

    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(s8, sw_credit)
        __field(s8, sw_credit_up)
                     ),

    TP_fast_assign(
        __entry->txq_idx = txq->idx;
        __entry->sw_credit = txq->credits;
        __entry->sw_credit_up = cred_up;
                   ),

    TP_printk("%s txq_credits=%d (%+d)", __print_txq(__entry->txq_idx),
              __entry->sw_credit, __entry->sw_credit_up)
)

#ifdef CONFIG_BL_FULLMAC

DECLARE_EVENT_CLASS(
    ps_template,
    TP_PROTO(struct bl_sta *sta),
    TP_ARGS(sta),
    TP_STRUCT__entry(
        __field(u16, idx)
        __field(u16, ready_ps)
        __field(u16, sp_ps)
        __field(u16, ready_uapsd)
        __field(u16, sp_uapsd)
                     ),
    TP_fast_assign(
        __entry->idx  = sta->sta_idx;
        __entry->ready_ps = sta->ps.pkt_ready[LEGACY_PS_ID];
        __entry->sp_ps = sta->ps.sp_cnt[LEGACY_PS_ID];
        __entry->ready_uapsd = sta->ps.pkt_ready[UAPSD_ID];
        __entry->sp_uapsd = sta->ps.sp_cnt[UAPSD_ID];
                   ),

    TP_printk("%s [PS] ready=%d sp=%d [UAPSD] ready=%d sp=%d",
              __print_sta(__entry->idx), __entry->ready_ps, __entry->sp_ps,
              __entry->ready_uapsd, __entry->sp_uapsd)
);

DEFINE_EVENT(ps_template, ps_queue,
             TP_PROTO(struct bl_sta *sta),
             TP_ARGS(sta));

DEFINE_EVENT(ps_template, ps_push,
             TP_PROTO(struct bl_sta *sta),
             TP_ARGS(sta));

DEFINE_EVENT(ps_template, ps_enable,
             TP_PROTO(struct bl_sta *sta),
             TP_ARGS(sta));

TRACE_EVENT(
    ps_traffic_update,
    TP_PROTO(u16 sta_idx, u8 traffic, bool uapsd),

    TP_ARGS(sta_idx, traffic, uapsd),

    TP_STRUCT__entry(
        __field(u16, sta_idx)
        __field(u8, traffic)
        __field(bool, uapsd)
                     ),

    TP_fast_assign(
        __entry->sta_idx = sta_idx;
        __entry->traffic = traffic;
        __entry->uapsd = uapsd;
                   ),

    TP_printk("%s %s%s traffic available ", __print_sta(__entry->sta_idx),
              __entry->traffic ? "" : "no more ",
              __entry->uapsd ? "U-APSD" : "legacy PS")
);

TRACE_EVENT(
    ps_traffic_req,
    TP_PROTO(struct bl_sta *sta, u16 pkt_req, u8 ps_id),
    TP_ARGS(sta, pkt_req, ps_id),
    TP_STRUCT__entry(
        __field(u16, idx)
        __field(u16, pkt_req)
        __field(u8, ps_id)
        __field(u16, ready)
        __field(u16, sp)
                     ),
    TP_fast_assign(
        __entry->idx  = sta->sta_idx;
        __entry->pkt_req  = pkt_req;
        __entry->ps_id  = ps_id;
        __entry->ready = sta->ps.pkt_ready[ps_id];
        __entry->sp = sta->ps.sp_cnt[ps_id];
                   ),

    TP_printk("%s %s traffic request %d pkt (ready=%d, sp=%d)",
              __print_sta(__entry->idx),
              __entry->ps_id == UAPSD_ID ? "U-APSD" : "legacy PS" ,
              __entry->pkt_req, __entry->ready, __entry->sp)
);


#ifdef CONFIG_BL_AMSDUS_TX
TRACE_EVENT(
    amsdu_subframe,
    TP_PROTO(struct bl_sw_txhdr *sw_txhdr),
    TP_ARGS(sw_txhdr),
    TP_STRUCT__entry(
        __field(struct sk_buff *, skb)
        __field(u16, txq_idx)
        __field(u8, nb)
        __field(u32, len)
                     ),
    TP_fast_assign(
        __entry->skb = sw_txhdr->skb;
        __entry->nb = sw_txhdr->amsdu.nb;
        __entry->len = sw_txhdr->amsdu.len;
        __entry->txq_idx = sw_txhdr->txq->idx;
                   ),

    TP_printk("%s skb=%p %s nb_subframe=%d, len=%u",
              __print_txq(__entry->txq_idx), __entry->skb,
              (__entry->nb == 2) ? "Start new AMSDU" : "Add subframe",
              __entry->nb, __entry->len)
);
#endif

#endif /* CONFIG_BL_FULLMAC */

/*****************************************************************************
 * TRACE functions for IPC message
 ****************************************************************************/
#include "bl_strs.h"

DECLARE_EVENT_CLASS(
    ipc_msg_template,
    TP_PROTO(u16 id),
    TP_ARGS(id),
    TP_STRUCT__entry(
        __field(u16, id)
                     ),
    TP_fast_assign(
        __entry->id  = id;
                   ),

    TP_printk("%s (%d - %d)", BL_ID2STR(__entry->id),
              MSG_T(__entry->id), MSG_I(__entry->id))
);

DEFINE_EVENT(ipc_msg_template, msg_send,
             TP_PROTO(u16 id),
             TP_ARGS(id));

DEFINE_EVENT(ipc_msg_template, msg_recv,
             TP_PROTO(u16 id),
             TP_ARGS(id));



#endif /* !defined(_BL_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ) */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE bl_events
#include <trace/define_trace.h>
