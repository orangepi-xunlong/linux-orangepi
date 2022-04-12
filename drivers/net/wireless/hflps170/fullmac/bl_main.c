/**
 ******************************************************************************
 *
 * @file bl_main.c
 *
 * @brief Entry point of the BL driver
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>

#include "bl_compat.h"
#include "bl_defs.h"
#include "bl_msg_tx.h"
#include "bl_tx.h"
#include "hal_desc.h"
#include "bl_debugfs.h"
#include "bl_cfgfile.h"
#include "bl_irqs.h"
#include "bl_version.h"
#include "bl_events.h"
#include "bl_sdio.h"

#define RW_DRV_DESCRIPTION  "BouffaloLab 11nac driver for Linux cfg80211"
#define RW_DRV_COPYRIGHT    "Copyright(c) 2015-2016 BouffaloLab"
#define RW_DRV_AUTHOR       "BouffaloLab S.A.S"

#define BL_PRINT_CFM_ERR(req) \
        printk(KERN_CRIT "%s: Status Error(%d)\n", #req, (&req##_cfm)->status)

#define BL_HT_CAPABILITIES                                    \
{                                                               \
    .ht_supported   = true,                                     \
    .cap            = 0,                                        \
    .ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,               \
    .ampdu_density  = IEEE80211_HT_MPDU_DENSITY_16,             \
    .mcs        = {                                             \
        .rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },        \
        .rx_highest = cpu_to_le16(65),                          \
        .tx_params = IEEE80211_HT_MCS_TX_DEFINED,               \
    },                                                          \
}

#define BL_VHT_CAPABILITIES                                   \
{                                                               \
    .vht_supported = false,                                     \
    .cap       =                                                \
      (7 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT),\
    .vht_mcs       = {                                          \
        .rx_mcs_map = cpu_to_le16(                              \
                      IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
        .tx_mcs_map = cpu_to_le16(                              \
                      IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
    }                                                           \
}

#define RATE(_bitrate, _hw_rate, _flags) {      \
    .bitrate    = (_bitrate),                   \
    .flags      = (_flags),                     \
    .hw_value   = (_hw_rate),                   \
}

#define CHAN(_freq) {                           \
    .center_freq    = (_freq),                  \
    .max_power  = 30, /* FIXME */               \
}

void *kbuff = NULL;
extern struct device_attribute dev_attr_filter_severity;
extern struct device_attribute dev_attr_filter_module;

static struct ieee80211_rate bl_ratetable[] = {
    RATE(10,  0x00, 0),
    RATE(20,  0x01, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(55,  0x02, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(60,  0x04, 0),
    RATE(90,  0x05, 0),
    RATE(120, 0x06, 0),
    RATE(180, 0x07, 0),
    RATE(240, 0x08, 0),
    RATE(360, 0x09, 0),
    RATE(480, 0x0A, 0),
    RATE(540, 0x0B, 0),
};

/* The channels indexes here are not used anymore */
static struct ieee80211_channel bl_2ghz_channels[] = {
    CHAN(2412),
    CHAN(2417),
    CHAN(2422),
    CHAN(2427),
    CHAN(2432),
    CHAN(2437),
    CHAN(2442),
    CHAN(2447),
    CHAN(2452),
    CHAN(2457),
    CHAN(2462),
    CHAN(2467),
    CHAN(2472),
    CHAN(2484),
};

static struct ieee80211_supported_band bl_band_2GHz = {
    .channels   = bl_2ghz_channels,
    .n_channels = ARRAY_SIZE(bl_2ghz_channels),
    .bitrates   = bl_ratetable,
    .n_bitrates = ARRAY_SIZE(bl_ratetable),
    .ht_cap     = BL_HT_CAPABILITIES,
};

static struct ieee80211_iface_limit bl_limits[] = {
    { .max = NX_VIRT_DEV_MAX, .types = BIT(NL80211_IFTYPE_AP) |
                                       BIT(NL80211_IFTYPE_STATION)}
};

static struct ieee80211_iface_limit bl_limits_dfs[] = {
    { .max = NX_VIRT_DEV_MAX, .types = BIT(NL80211_IFTYPE_AP)}
};

static const struct ieee80211_iface_combination bl_combinations[] = {
    {
        .limits                 = bl_limits,
        .n_limits               = ARRAY_SIZE(bl_limits),
        .num_different_channels = NX_CHAN_CTXT_CNT,
        .max_interfaces         = NX_VIRT_DEV_MAX,
    },
    /* Keep this combination as the last one */
    {
        .limits                 = bl_limits_dfs,
        .n_limits               = ARRAY_SIZE(bl_limits_dfs),
        .num_different_channels = 1,
        .max_interfaces         = NX_VIRT_DEV_MAX,
        .radar_detect_widths = (BIT(NL80211_CHAN_WIDTH_20_NOHT) |
                                BIT(NL80211_CHAN_WIDTH_20) |
                                BIT(NL80211_CHAN_WIDTH_40) |
                                BIT(NL80211_CHAN_WIDTH_80)),
    }
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static struct ieee80211_txrx_stypes
bl_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
    [NL80211_IFTYPE_STATION] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
    },
    [NL80211_IFTYPE_AP] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
            BIT(IEEE80211_STYPE_DISASSOC >> 4) |
            BIT(IEEE80211_STYPE_AUTH >> 4) |
            BIT(IEEE80211_STYPE_DEAUTH >> 4) |
            BIT(IEEE80211_STYPE_ACTION >> 4),
    },
    [NL80211_IFTYPE_AP_VLAN] = {
        /* copy AP */
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
            BIT(IEEE80211_STYPE_DISASSOC >> 4) |
            BIT(IEEE80211_STYPE_AUTH >> 4) |
            BIT(IEEE80211_STYPE_DEAUTH >> 4) |
            BIT(IEEE80211_STYPE_ACTION >> 4),
    },
    [NL80211_IFTYPE_P2P_CLIENT] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
    },
    [NL80211_IFTYPE_P2P_GO] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
            BIT(IEEE80211_STYPE_DISASSOC >> 4) |
            BIT(IEEE80211_STYPE_AUTH >> 4) |
            BIT(IEEE80211_STYPE_DEAUTH >> 4) |
            BIT(IEEE80211_STYPE_ACTION >> 4),
    },
    [NL80211_IFTYPE_P2P_DEVICE] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
    },
};


static u32 cipher_suites[] = {
    WLAN_CIPHER_SUITE_WEP40,
    WLAN_CIPHER_SUITE_WEP104,
    WLAN_CIPHER_SUITE_TKIP,
    WLAN_CIPHER_SUITE_CCMP,
    0, // reserved entries to enable AES-CMAC and/or SMS4
    0,
};
#define NB_RESERVED_CIPHER 2;

static const int bl_ac2hwq[1][NL80211_NUM_ACS] = {
    {
        [NL80211_TXQ_Q_VO] = BL_HWQ_VO,
        [NL80211_TXQ_Q_VI] = BL_HWQ_VI,
        [NL80211_TXQ_Q_BE] = BL_HWQ_BE,
        [NL80211_TXQ_Q_BK] = BL_HWQ_BK
    }
};

const int bl_tid2hwq[IEEE80211_NUM_TIDS] = {
    BL_HWQ_BE,
    BL_HWQ_BK,
    BL_HWQ_BK,
    BL_HWQ_BE,
    BL_HWQ_VI,
    BL_HWQ_VI,
    BL_HWQ_VO,
    BL_HWQ_VO,
    /* TID_8 is used for management frames */
    BL_HWQ_VO,
    /* At the moment, all others TID are mapped to BE */
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
};

static const int bl_hwq2uapsd[NL80211_NUM_ACS] = {
    [BL_HWQ_VO] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VO,
    [BL_HWQ_VI] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VI,
    [BL_HWQ_BE] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BE,
    [BL_HWQ_BK] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BK,
};

/*********************************************************************
 * helper
 *********************************************************************/
struct bl_sta *bl_get_sta(struct bl_hw *bl_hw, const u8 *mac_addr)
{
    int i;

    for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
        struct bl_sta *sta = &bl_hw->sta_table[i];
        if (sta->valid && (memcmp(mac_addr, &sta->mac_addr, 6) == 0))
            return sta;
    }

    return NULL;
}

void bl_enable_wapi(struct bl_hw *bl_hw)
{
    cipher_suites[bl_hw->wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_SMS4;
    bl_hw->wiphy->n_cipher_suites ++;
    bl_hw->wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;
}

void bl_enable_mfp(struct bl_hw *bl_hw)
{
    cipher_suites[bl_hw->wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_AES_CMAC;
    bl_hw->wiphy->n_cipher_suites ++;
}

u8 *bl_build_bcn(struct bl_bcn *bcn, struct cfg80211_beacon_data *new)
{
    u8 *buf, *pos;

    if (new->head) {
        u8 *head = kmalloc(new->head_len, GFP_KERNEL);

        if (!head)
            return NULL;

        if (bcn->head)
            kfree(bcn->head);

        bcn->head = head;
        bcn->head_len = new->head_len;
        memcpy(bcn->head, new->head, new->head_len);
    }
    if (new->tail) {
        u8 *tail = kmalloc(new->tail_len, GFP_KERNEL);

        if (!tail)
            return NULL;

        if (bcn->tail)
            kfree(bcn->tail);

        bcn->tail = tail;
        bcn->tail_len = new->tail_len;
        memcpy(bcn->tail, new->tail, new->tail_len);
    }

    if (!bcn->head)
        return NULL;

    bcn->tim_len = 6;
    bcn->len = bcn->head_len + bcn->tail_len + bcn->ies_len + bcn->tim_len;

    buf = kmalloc(bcn->len, GFP_KERNEL);
    if (!buf)
        return NULL;

    // Build the beacon buffer
    pos = buf;
    memcpy(pos, bcn->head, bcn->head_len);
    pos += bcn->head_len;
    *pos++ = WLAN_EID_TIM;
    *pos++ = 4;
    *pos++ = 0;
    *pos++ = bcn->dtim;
    *pos++ = 0;
    *pos++ = 0;
    if (bcn->tail) {
        memcpy(pos, bcn->tail, bcn->tail_len);
        pos += bcn->tail_len;
    }
    if (bcn->ies) {
        memcpy(pos, bcn->ies, bcn->ies_len);
    }

    return buf;
}


static void bl_del_bcn(struct bl_bcn *bcn)
{
    if (bcn->head) {
        kfree(bcn->head);
        bcn->head = NULL;
    }
    bcn->head_len = 0;

    if (bcn->tail) {
        kfree(bcn->tail);
        bcn->tail = NULL;
    }
    bcn->tail_len = 0;

    if (bcn->ies) {
        kfree(bcn->ies);
        bcn->ies = NULL;
    }
    bcn->ies_len = 0;
    bcn->tim_len = 0;
    bcn->dtim = 0;
    bcn->len = 0;
}

/**
 * Link channel ctxt to a vif and thus increments count for this context.
 */
void bl_chanctx_link(struct bl_vif *vif, u8 ch_idx,
                       struct cfg80211_chan_def *chandef)
{
    struct bl_chanctx *ctxt;

    if (ch_idx >= NX_CHAN_CTXT_CNT) {
        WARN(1, "Invalid channel ctxt id %d", ch_idx);
        return;
    }

    vif->ch_index = ch_idx;
    ctxt = &vif->bl_hw->chanctx_table[ch_idx];
    ctxt->count++;

    // For now chandef is NULL for STATION interface
    if (chandef) {
        if (!ctxt->chan_def.chan)
            ctxt->chan_def = *chandef;
        else {
            // TODO. check that chandef is the same as the one already
            // set for this ctxt
        }
    }
}

/**
 * Unlink channel ctxt from a vif and thus decrements count for this context
 */
void bl_chanctx_unlink(struct bl_vif *vif)
{
    struct bl_chanctx *ctxt;

    if (vif->ch_index == BL_CH_NOT_SET)
        return;

    ctxt = &vif->bl_hw->chanctx_table[vif->ch_index];

    if (ctxt->count == 0) {
        WARN(1, "Chan ctxt ref count is already 0");
    } else {
        ctxt->count--;
    }

    if (ctxt->count == 0) {
        /* set chan to null, so that if this ctxt is relinked to a vif that
           don't have channel information, don't use wrong information */
        ctxt->chan_def.chan = NULL;
    }
    vif->ch_index = BL_CH_NOT_SET;
}

int bl_chanctx_valid(struct bl_hw *bl_hw, u8 ch_idx)
{
    if (ch_idx >= NX_CHAN_CTXT_CNT ||
        bl_hw->chanctx_table[ch_idx].chan_def.chan == NULL) {
        return 0;
    }

    return 1;
}

static void bl_del_csa(struct bl_vif *vif)
{
    struct bl_csa *csa = vif->ap.csa;

    if (!csa)
        return;

    if (csa->dma.buf) {
        kfree(csa->dma.buf);
    }
    bl_del_bcn(&csa->bcn);
    kfree(csa);
    vif->ap.csa = NULL;
}

static void bl_csa_finish(struct work_struct *ws)
{
    struct bl_csa *csa = container_of(ws, struct bl_csa, work);
    struct bl_vif *vif = csa->vif;
    struct bl_hw *bl_hw = vif->bl_hw;
    int error = csa->status;

    if (!error)
        error = bl_send_bcn_change(bl_hw, vif->vif_index, csa->dma.buf,
                                     csa->bcn.len, csa->bcn.head_len,
                                     csa->bcn.tim_len, NULL);

    if (error)
        cfg80211_stop_iface(bl_hw->wiphy, &vif->wdev, GFP_KERNEL);
    else {
        bl_chanctx_unlink(vif);
        bl_chanctx_link(vif, csa->ch_idx, &csa->chandef);
        spin_lock_bh(&bl_hw->cmd_mgr.lock);
        if (bl_hw->cur_chanctx == csa->ch_idx) {
            bl_txq_vif_start(vif, BL_TXQ_STOP_CHAN, bl_hw);
        } else
            bl_txq_vif_stop(vif, BL_TXQ_STOP_CHAN, bl_hw);
        spin_unlock_bh(&bl_hw->cmd_mgr.lock);
        cfg80211_ch_switch_notify(vif->ndev, &csa->chandef);
    }
    bl_del_csa(vif);
}

/*********************************************************************
 * netdev callbacks
 ********************************************************************/
/**
 * int (*ndo_open)(struct net_device *dev);
 *     This function is called when network device transistions to the up
 *     state.
 *
 * - Start FW if this is the first interface opened
 * - Add interface at fw level
 */
static int bl_open(struct net_device *dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct mm_add_if_cfm add_if_cfm;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    // Check if it is the first opened VIF
    if (bl_hw->vif_started == 0)
    {
        // Start the FW
       if ((error = bl_send_start(bl_hw)))
           return error;

       /* Device is now started */
       set_bit(BL_DEV_STARTED, &bl_hw->drv_flags);
    }

    if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN) {
        /* For AP_vlan use same fw and drv indexes. We ensure that this index
           will not be used by fw for another vif by taking index >= NX_VIRT_DEV_MAX */
        add_if_cfm.inst_nbr = bl_vif->drv_vif_index;
        netif_tx_stop_all_queues(dev);
    } else {
        /* Forward the information to the LMAC,
         *     p2p value not used in FMAC configuration, iftype is sufficient */
        if ((error = bl_send_add_if(bl_hw, dev->dev_addr,
                                      BL_VIF_TYPE(bl_vif), false, &add_if_cfm)))
            return error;

        if (add_if_cfm.status != 0) {
            BL_PRINT_CFM_ERR(add_if);
            return -EIO;
        }
    }

    /* Save the index retrieved from LMAC */
    bl_vif->vif_index = add_if_cfm.inst_nbr;
    bl_vif->up = true;
    bl_hw->vif_started++;
    bl_hw->vif_table[add_if_cfm.inst_nbr] = bl_vif;

    netif_carrier_off(dev);

    return error;
}

/**
 * int (*ndo_stop)(struct net_device *dev);
 *     This function is called when network device transistions to the down
 *     state.
 *
 * - Remove interface at fw level
 * - Reset FW if this is the last interface opened
 */
static int bl_close(struct net_device *dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;

    BL_DBG(BL_FN_ENTRY_STR);

    netdev_info(dev, "CLOSE");

    /* Abort scan request on the vif */
    if (bl_hw->scan_request &&
        bl_hw->scan_request->wdev == &bl_vif->wdev) {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        struct cfg80211_scan_info info = {
            .aborted = true,
        };

        cfg80211_scan_done(bl_hw->scan_request, &info);
#else
        cfg80211_scan_done(bl_hw->scan_request, true);
#endif
        bl_hw->scan_request = NULL;
    }

    bl_send_remove_if(bl_hw, bl_vif->vif_index);

    if (bl_hw->roc_elem && (bl_hw->roc_elem->wdev == &bl_vif->wdev)) {
        /* Initialize RoC element pointer to NULL, indicate that RoC can be started */
        bl_hw->roc_elem = NULL;
    }

    /* Ensure that we won't process disconnect ind */
    spin_lock_bh(&bl_hw->cmd_mgr.lock);

    bl_vif->up = false;
    if (netif_carrier_ok(dev)) {
        if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_STATION ||
            BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_P2P_CLIENT) {
            cfg80211_disconnected(dev, WLAN_REASON_DEAUTH_LEAVING,
                                  NULL, 0, true, GFP_ATOMIC);
            netif_tx_stop_all_queues(dev);
            netif_carrier_off(dev);
        } else if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN) {
            netif_carrier_off(dev);
        } else {
            netdev_warn(dev, "AP not stopped when disabling interface");
        }
    }

    spin_unlock_bh(&bl_hw->cmd_mgr.lock);

    bl_hw->vif_table[bl_vif->vif_index] = NULL;
    bl_hw->vif_started--;
    if (bl_hw->vif_started == 0) {
        /* This also lets both ipc sides remain in sync before resetting */
        bl_ipc_tx_drain(bl_hw);

        bl_send_reset(bl_hw);

        // Set parameters to firmware
        bl_send_me_config_req(bl_hw);

        // Set channel parameters to firmware
        bl_send_me_chan_config_req(bl_hw);

        clear_bit(BL_DEV_STARTED, &bl_hw->drv_flags);
    }

    return 0;
}

/**
 * struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
 *	Called when a user wants to get the network device usage
 *	statistics. Drivers must do one of the following:
 *	1. Define @ndo_get_stats64 to fill in a zero-initialised
 *	   rtnl_link_stats64 structure passed by the caller.
 *	2. Define @ndo_get_stats to update a net_device_stats structure
 *	   (which should normally be dev->stats) and return a pointer to
 *	   it. The structure may be changed asynchronously only if each
 *	   field is written atomically.
 *	3. Update dev->stats asynchronously and atomically, and define
 *	   neither operation.
 */
static struct net_device_stats *bl_get_stats(struct net_device *dev)
{
    struct bl_vif *vif = netdev_priv(dev);

    return &vif->net_stats;
}

static const u16 bl_1d_to_wmm_queue[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };
/**
 * u16 (*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb,
 *                         void *accel_priv, select_queue_fallback_t fallback);
 *	Called to decide which queue to when device supports multiple
 *	transmit queues.
 */
u16 bl_select_queue(struct net_device *dev, struct sk_buff *skb,
                    struct net_device *sb_dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct wireless_dev *wdev = &bl_vif->wdev;
    struct bl_sta *sta = NULL;
    struct bl_txq *txq;
    u16 netdev_queue;

	BL_DBG(BL_FN_ENTRY_STR);
#define PRIO_STA_NULL 0xAA

    switch (wdev->iftype) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
    {
        struct ethhdr *eth;
        eth = (struct ethhdr *)skb->data;
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
        skb->priority = PRIO_STA_NULL;
        netdev_queue = NX_BCMC_TXQ_NDEV_IDX;
    }

    if(skb->priority < 8)
        return bl_1d_to_wmm_queue[skb->priority];
	else
        return 0;

    //BUG_ON(netdev_queue >= NX_NB_NDEV_TXQ);
    //return netdev_queue;
}

/**
 * int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
 *	This function  is called when the Media Access Control address
 *	needs to be changed. If this interface is not defined, the
 *	mac address can not be changed.
 */
static int bl_set_mac_address(struct net_device *dev, void *addr)
{
    struct sockaddr *sa = addr;
    int ret;

    ret = eth_mac_addr(dev, sa);

    return ret;
}

static const struct net_device_ops bl_netdev_ops = {
    .ndo_open               = bl_open,
    .ndo_stop               = bl_close,
    .ndo_start_xmit         = bl_start_xmit,
    .ndo_get_stats          = bl_get_stats,
    .ndo_select_queue       = bl_select_queue,
    .ndo_set_mac_address    = bl_set_mac_address
};

static void bl_netdev_setup(struct net_device *dev)
{
    ether_setup(dev);
    dev->priv_flags &= ~IFF_TX_SKB_SHARING;
    dev->netdev_ops = &bl_netdev_ops;
#if LINUX_VERSION_CODE <  KERNEL_VERSION(4, 12, 0)
    dev->destructor = free_netdev;
#else
    dev->needs_free_netdev = true;
#endif

    dev->watchdog_timeo = BL_TX_LIFETIME_MS;

    dev->needed_headroom = sizeof(struct bl_txhdr) + BL_SWTXHDR_ALIGN_SZ;
#ifdef CONFIG_BL_AMSDUS_TX
    dev->needed_headroom = max(dev->needed_headroom,
                               (unsigned short)(sizeof(struct bl_amsdu_txhdr)
                                                + sizeof(struct ethhdr) + 4
                                                + sizeof(rfc1042_header) + 2));
#endif /* CONFIG_BL_AMSDUS_TX */
	dev->flags |= IFF_BROADCAST | IFF_MULTICAST;

    dev->hw_features = 0;
}

/*********************************************************************
 * Cfg80211 callbacks (and helper)
 *********************************************************************/
static struct wireless_dev *bl_interface_add(struct bl_hw *bl_hw,
                                               const char *name,
                                               enum nl80211_iftype type,
                                               struct vif_params *params)
{
    struct net_device *ndev;
    struct bl_vif *vif;
    int min_idx, max_idx;
    int vif_idx = -1;
    int i;

	BL_DBG(BL_FN_ENTRY_STR);
	
    // Look for an available VIF
    if (type == NL80211_IFTYPE_AP_VLAN) {
        min_idx = NX_VIRT_DEV_MAX;
        max_idx = NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX;
    } else {
        min_idx = 0;
        max_idx = NX_VIRT_DEV_MAX;
    }

    for (i = min_idx; i < max_idx; i++) {
        if ((bl_hw->avail_idx_map) & BIT(i)) {
            vif_idx = i;
            break;
        }
    }
    if (vif_idx < 0)
        return NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    ndev = alloc_netdev_mqs(sizeof(*vif), name, NET_NAME_UNKNOWN,
                            bl_netdev_setup, IEEE80211_NUM_ACS, 1);
#else
    ndev = alloc_netdev_mqs(sizeof(*vif), name, bl_netdev_setup,
                            IEEE80211_NUM_ACS, 1);
#endif
    if (!ndev)
        return NULL;

    vif = netdev_priv(ndev);
    ndev->ieee80211_ptr = &vif->wdev;
    vif->wdev.wiphy = bl_hw->wiphy;
    vif->bl_hw = bl_hw;
    vif->ndev = ndev;
    vif->drv_vif_index = vif_idx;
    SET_NETDEV_DEV(ndev, wiphy_dev(vif->wdev.wiphy));
    vif->wdev.netdev = ndev;
    vif->wdev.iftype = type;
    vif->up = false;
    vif->ch_index = BL_CH_NOT_SET;
    memset(&vif->net_stats, 0, sizeof(vif->net_stats));

    switch (type) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
        vif->sta.ap = NULL;
        vif->sta.tdls_sta = NULL;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        INIT_LIST_HEAD(&vif->ap.mpath_list);
        INIT_LIST_HEAD(&vif->ap.proxy_list);
        vif->ap.create_path = false;
        vif->ap.generation = 0;
        // no break
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        break;
    case NL80211_IFTYPE_AP_VLAN:
    {
        struct bl_vif *master_vif;
        bool found = false;
        list_for_each_entry(master_vif, &bl_hw->vifs, list) {
            if ((BL_VIF_TYPE(master_vif) == NL80211_IFTYPE_AP) &&
                !(!memcmp(master_vif->ndev->dev_addr, params->macaddr,
                           ETH_ALEN))) {
                 found=true;
                 break;
            }
        }

        if (!found)
            goto err;

         vif->ap_vlan.master = master_vif;
         vif->ap_vlan.sta_4a = NULL;
         break;
    }
    default:
        break;
    }

    if (type == NL80211_IFTYPE_AP_VLAN)
        memcpy(ndev->dev_addr, params->macaddr, ETH_ALEN);
    else {
        memcpy(ndev->dev_addr, bl_hw->wiphy->perm_addr, ETH_ALEN);
        ndev->dev_addr[5] ^= vif_idx;
    }

    if (params) {
        vif->use_4addr = params->use_4addr;
        ndev->ieee80211_ptr->use_4addr = params->use_4addr;
    } else
        vif->use_4addr = false;

    if (register_netdevice(ndev))
        goto err;
	
    list_add_tail(&vif->list, &bl_hw->vifs);
    bl_hw->avail_idx_map &= ~BIT(vif_idx);

    return &vif->wdev;

err:
    free_netdev(ndev);
    return NULL;
}


/*
 * @brief Retrieve the bl_sta object allocated for a given MAC address
 * and a given role.
 */
static struct bl_sta *bl_retrieve_sta(struct bl_hw *bl_hw,
                                          struct bl_vif *bl_vif, u8 *addr,
                                          __le16 fc, bool ap)
{
    if (ap) {
        /* only deauth, disassoc and action are bufferable MMPDUs */
        bool bufferable = ieee80211_is_deauth(fc) ||
                          ieee80211_is_disassoc(fc) ||
                          ieee80211_is_action(fc);

        /* Check if the packet is bufferable or not */
        if (bufferable)
        {
            /* Check if address is a broadcast or a multicast address */
            if (is_broadcast_ether_addr(addr) || is_multicast_ether_addr(addr)) {
                /* Returned STA pointer */
                struct bl_sta *bl_sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];

                if (bl_sta->valid)
                    return bl_sta;
            } else {
                /* Returned STA pointer */
                struct bl_sta *bl_sta;

                /* Go through list of STAs linked with the provided VIF */
                list_for_each_entry(bl_sta, &bl_vif->ap.sta_list, list) {
                    if (bl_sta->valid &&
                        ether_addr_equal(bl_sta->mac_addr, addr)) {
                        /* Return the found STA */
                        return bl_sta;
                    }
                }
            }
        }
    } else {
        return bl_vif->sta.ap;
    }

    return NULL;
}

/**
 * @add_virtual_intf: create a new virtual interface with the given name,
 *	must set the struct wireless_dev's iftype. Beware: You must create
 *	the new netdev in the wiphy's network namespace! Returns the struct
 *	wireless_dev, or an ERR_PTR. For P2P device wdevs, the driver must
 *	also set the address member in the wdev.
 */
static struct wireless_dev *bl_cfg80211_add_iface(struct wiphy *wiphy,
                                                    const char *name,
													unsigned char name_assign_type,
                                                    enum nl80211_iftype type,
                                                    struct vif_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct wireless_dev *wdev;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0))
     unsigned char name_assign_type = NET_NAME_UNKNOWN;
#endif

    wdev = bl_interface_add(bl_hw, name, type, params);

    return wdev;
}

/**
 * @del_virtual_intf: remove the virtual interface
 */
static int bl_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
    struct net_device *dev = wdev->netdev;
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = wiphy_priv(wiphy);

    netdev_info(dev, "Remove Interface");

    if (dev->reg_state == NETREG_REGISTERED) {
        /* Will call bl_close if interface is UP */
        unregister_netdevice(dev);
    }

    list_del(&bl_vif->list);
    bl_hw->avail_idx_map |= BIT(bl_vif->drv_vif_index);
    bl_vif->ndev = NULL;

    /* Clear the priv in adapter */
    dev->ieee80211_ptr = NULL;

    return 0;
}

/**
 * @change_virtual_intf: change type/configuration of virtual interface,
 *	keep the struct wireless_dev's iftype updated.
 */
static int bl_cfg80211_change_iface(struct wiphy *wiphy,
                   struct net_device *dev,
                   enum nl80211_iftype type,
                   struct vif_params *params)
{
    struct bl_vif *vif = netdev_priv(dev);

    if (vif->up)
        return (-EBUSY);

    switch (type) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
        vif->sta.ap = NULL;
        vif->sta.tdls_sta = NULL;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        INIT_LIST_HEAD(&vif->ap.mpath_list);
        INIT_LIST_HEAD(&vif->ap.proxy_list);
        vif->ap.create_path = false;
        vif->ap.generation = 0;
        // no break
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        break;
    case NL80211_IFTYPE_AP_VLAN:
        return -EPERM;
    default:
        break;
    }

    vif->wdev.iftype = type;
    if (params->use_4addr != -1)
        vif->use_4addr = params->use_4addr;

    return 0;
}

/**
 * @scan: Request to do a scan. If returning zero, the scan request is given
 *	the driver, and will be valid until passed to cfg80211_scan_done().
 *	For scan results, call cfg80211_inform_bss(); you can call this outside
 *	the scan/scan_done bracket too.
 */
static int bl_cfg80211_scan(struct wiphy *wiphy,
                              struct cfg80211_scan_request *request)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = container_of(request->wdev, struct bl_vif,
                                             wdev);
    int error;

    BL_DBG(BL_FN_ENTRY_STR);

    if ((error = bl_send_scanu_req(bl_hw, bl_vif, request)))
        return error;

    bl_hw->scan_request = request;

    return 0;
}

/**
 * @add_key: add a key with the given parameters. @mac_addr will be %NULL
 *	when adding a group key.
 */
static int bl_cfg80211_add_key(struct wiphy *wiphy, struct net_device *netdev,
                                 u8 key_index, bool pairwise, const u8 *mac_addr,
                                 struct key_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(netdev);
    int i, error = 0;
    struct mm_key_add_cfm key_add_cfm;
    u8_l cipher = 0;
    struct bl_sta *sta = NULL;
    struct bl_key *bl_key;

    BL_DBG(BL_FN_ENTRY_STR);

    if (mac_addr) {
        sta = bl_get_sta(bl_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        bl_key = &sta->key;
    }
    else
        bl_key = &vif->key[key_index];

    /* Retrieve the cipher suite selector */
    switch (params->cipher) {
    case WLAN_CIPHER_SUITE_WEP40:
        cipher = MAC_RSNIE_CIPHER_WEP40;
        break;
    case WLAN_CIPHER_SUITE_WEP104:
        cipher = MAC_RSNIE_CIPHER_WEP104;
        break;
    case WLAN_CIPHER_SUITE_TKIP:
        cipher = MAC_RSNIE_CIPHER_TKIP;
        break;
    case WLAN_CIPHER_SUITE_CCMP:
        cipher = MAC_RSNIE_CIPHER_CCMP;
        break;
    case WLAN_CIPHER_SUITE_AES_CMAC:
        cipher = MAC_RSNIE_CIPHER_AES_CMAC;
        break;
    case WLAN_CIPHER_SUITE_SMS4:
    {
        // Need to reverse key order
        u8 tmp, *key = (u8 *)params->key;
        cipher = MAC_RSNIE_CIPHER_SMS4;
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i];
            key[i] = key[WPI_SUBKEY_LEN - 1 - i];
            key[WPI_SUBKEY_LEN - 1 - i] = tmp;
        }
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i + WPI_SUBKEY_LEN];
            key[i + WPI_SUBKEY_LEN] = key[WPI_KEY_LEN - 1 - i];
            key[WPI_KEY_LEN - 1 - i] = tmp;
        }
        break;
    }
    default:
        return -EINVAL;
    }

    if ((error = bl_send_key_add(bl_hw, vif->vif_index,
                                   (sta ? sta->sta_idx : 0xFF), pairwise,
                                   (u8 *)params->key, params->key_len,
                                   key_index, cipher, &key_add_cfm)))
        return error;

    if (key_add_cfm.status != 0) {
        BL_PRINT_CFM_ERR(key_add);
        return -EIO;
    }

    /* Save the index retrieved from LMAC */
    bl_key->hw_idx = key_add_cfm.hw_key_idx;

    return 0;
}

/**
 * @get_key: get information about the key with the given parameters.
 *	@mac_addr will be %NULL when requesting information for a group
 *	key. All pointers given to the @callback function need not be valid
 *	after it returns. This function should return an error if it is
 *	not possible to retrieve the key, -ENOENT if it doesn't exist.
 *
 */
static int bl_cfg80211_get_key(struct wiphy *wiphy, struct net_device *netdev,
                                 u8 key_index, bool pairwise, const u8 *mac_addr,
                                 void *cookie,
                                 void (*callback)(void *cookie, struct key_params*))
{
    BL_DBG(BL_FN_ENTRY_STR);

    return -1;
}


/**
 * @del_key: remove a key given the @mac_addr (%NULL for a group key)
 *	and @key_index, return -ENOENT if the key doesn't exist.
 */
static int bl_cfg80211_del_key(struct wiphy *wiphy, struct net_device *netdev,
                                 u8 key_index, bool pairwise, const u8 *mac_addr)
{
	struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(netdev);
    int error;
    struct bl_sta *sta = NULL;
    struct bl_key *bl_key;

    BL_DBG(BL_FN_ENTRY_STR);
    if (mac_addr) {
        sta = bl_get_sta(bl_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        bl_key = &sta->key;
    }
    else
        bl_key = &vif->key[key_index];

    error = bl_send_key_del(bl_hw, bl_key->hw_idx);
    return error;
}

/**
 * @set_default_key: set the default key on an interface
 */
static int bl_cfg80211_set_default_key(struct wiphy *wiphy,
                                         struct net_device *netdev,
                                         u8 key_index, bool unicast, bool multicast)
{
    BL_DBG(BL_FN_ENTRY_STR);

    return 0;
}

/**
 * @set_default_mgmt_key: set the default management frame key on an interface
 */
static int bl_cfg80211_set_default_mgmt_key(struct wiphy *wiphy,
                                              struct net_device *netdev,
                                              u8 key_index)
{
    return 0;
}

/**
 * @connect: Connect to the ESS with the specified parameters. When connected,
 *	call cfg80211_connect_result() with status code %WLAN_STATUS_SUCCESS.
 *	If the connection fails for some reason, call cfg80211_connect_result()
 *	with the status from the AP.
 *	(invoked with the wireless_dev mutex held)
 */
static int bl_cfg80211_connect(struct wiphy *wiphy, struct net_device *dev,
                                 struct cfg80211_connect_params *sme)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct sm_connect_cfm sm_connect_cfm;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    /* For SHARED-KEY authentication, must install key first */
    if (sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY && sme->key)
    {
        struct key_params key_params;
        key_params.key = sme->key;
        key_params.seq = NULL;
        key_params.key_len = sme->key_len;
        key_params.seq_len = 0;
        key_params.cipher = sme->crypto.cipher_group;
        bl_cfg80211_add_key(wiphy, dev, sme->key_idx, false, NULL, &key_params);
    }

    /* Forward the information to the LMAC */
    if ((error = bl_send_sm_connect_req(bl_hw, bl_vif, sme, &sm_connect_cfm)))
        return error;

    // Check the status
    switch (sm_connect_cfm.status)
    {
        case CO_OK:
            error = 0;
            break;
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

    return error;
}

/**
 * @disconnect: Disconnect from the BSS/ESS.
 *	(invoked with the wireless_dev mutex held)
 */
static int bl_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *dev,
                                    u16 reason_code)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);

    BL_DBG(BL_FN_ENTRY_STR);

    return(bl_send_sm_disconnect_req(bl_hw, bl_vif, reason_code));
}

/**
 * @add_station: Add a new station.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
static int bl_cfg80211_add_station(struct wiphy *wiphy, struct net_device *dev,
                                     const u8 *mac, struct station_parameters *params)

#else
static int bl_cfg80211_add_station(struct wiphy *wiphy, struct net_device *dev,
                                     u8 *mac, struct station_parameters *params)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct me_sta_add_cfm me_sta_add_cfm;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    WARN_ON(BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN);

    /* Do not add TDLS station */
    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        return 0;

    /* Indicate we are in a STA addition process - This will allow handling
     * potential PS mode change indications correctly
     */
    bl_hw->adding_sta = true;

    /* Forward the information to the LMAC */
    if ((error = bl_send_me_sta_add(bl_hw, params, mac, bl_vif->vif_index,
                                      &me_sta_add_cfm)))
        return error;

    // Check the status
    switch (me_sta_add_cfm.status)
    {
        case CO_OK:
        {
            struct bl_sta *sta = &bl_hw->sta_table[me_sta_add_cfm.sta_idx];
            int tid;
            sta->aid = params->aid;

            sta->sta_idx = me_sta_add_cfm.sta_idx;
            sta->ch_idx = bl_vif->ch_index;
            sta->vif_idx = bl_vif->vif_index;
            sta->vlan_idx = sta->vif_idx;
            sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
            sta->ht = params->ht_capa ? 1 : 0;
            sta->vht = params->vht_capa ? 1 : 0;
            sta->acm = 0;
            for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                int uapsd_bit = bl_hwq2uapsd[bl_tid2hwq[tid]];
                if (params->uapsd_queues & uapsd_bit)
                    sta->uapsd_tids |= 1 << tid;
                else
                    sta->uapsd_tids &= ~(1 << tid);
            }
            memcpy(sta->mac_addr, mac, ETH_ALEN);
            bl_dbgfs_register_rc_stat(bl_hw, sta);

            /* Ensure that we won't process PS change or channel switch ind*/
            spin_lock_bh(&bl_hw->cmd_mgr.lock);
            bl_txq_sta_init(bl_hw, sta, bl_txq_vif_get_status(bl_vif));
            list_add_tail(&sta->list, &bl_vif->ap.sta_list);
            sta->valid = true;
            bl_ps_bh_enable(bl_hw, sta, sta->ps.active || me_sta_add_cfm.pm_state);
            spin_unlock_bh(&bl_hw->cmd_mgr.lock);

            error = 0;

#ifdef CONFIG_BL_BFMER
            if (bl_hw->mod_params->bfmer)
                bl_send_bfmer_enable(bl_hw, sta, params->vht_capa);

            bl_mu_group_sta_init(sta, params->vht_capa);
#endif /* CONFIG_BL_BFMER */

            #define PRINT_STA_FLAG(f)                               \
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_##f) ? "["#f"]" : "")

            netdev_info(dev, "Add sta %d (%pM) flags=%s%s%s%s%s%s%s",
                        sta->sta_idx, mac,
                        PRINT_STA_FLAG(AUTHORIZED),
                        PRINT_STA_FLAG(SHORT_PREAMBLE),
                        PRINT_STA_FLAG(WME),
                        PRINT_STA_FLAG(MFP),
                        PRINT_STA_FLAG(AUTHENTICATED),
                        PRINT_STA_FLAG(TDLS_PEER),
                        PRINT_STA_FLAG(ASSOCIATED));
            #undef PRINT_STA_FLAG
            break;
        }
        default:
            error = -EBUSY;
            break;
    }

    bl_hw->adding_sta = false;

    return error;
}

/**
 * @del_station: Remove a station
 */
static int bl_cfg80211_del_station_compat(struct wiphy *wiphy, struct net_device *dev,
                              struct station_del_parameters *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_sta *cur, *tmp;
    int error = 0, found = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
    const u8 *mac = NULL;
    if(params)
        mac = params->mac;
#endif

    list_for_each_entry_safe(cur, tmp, &bl_vif->ap.sta_list, list) {
        if ((!mac) || (!memcmp(cur->mac_addr, mac, ETH_ALEN))) {
            netdev_info(dev, "Del sta %d (%pM)", cur->sta_idx, cur->mac_addr);
            /* Ensure that we won't process PS change ind */
            spin_lock_bh(&bl_hw->cmd_mgr.lock);
            cur->ps.active = false;
            cur->valid = false;
            spin_unlock_bh(&bl_hw->cmd_mgr.lock);

            if (cur->vif_idx != cur->vlan_idx) {
                struct bl_vif *vlan_vif;
                vlan_vif = bl_hw->vif_table[cur->vlan_idx];
                if (vlan_vif->up) {
                    if ((BL_VIF_TYPE(vlan_vif) == NL80211_IFTYPE_AP_VLAN) &&
                        (vlan_vif->use_4addr)) {
                        vlan_vif->ap_vlan.sta_4a = NULL;
                    } else {
                        WARN(1, "Deleting sta belonging to VLAN other than AP_VLAN 4A");
                    }
                }
            }

            bl_txq_sta_deinit(bl_hw, cur);
            error = bl_send_me_sta_del(bl_hw, cur->sta_idx, false);
            if ((error != 0) && (error != -EPIPE))
                return error;

#ifdef CONFIG_BL_BFMER
            // Disable Beamformer if supported
            bl_bfmer_report_del(bl_hw, cur);
            bl_mu_group_sta_del(bl_hw, cur);
#endif /* CONFIG_BL_BFMER */

            list_del(&cur->list);
            bl_dbgfs_unregister_rc_stat(bl_hw, cur);
            found ++;
            break;
        }
    }

    if (!found)
        return -ENOENT;
    else
        return 0;
}

/**
 * @change_station: Modify a given station. Note that flags changes are not much
 *	validated in cfg80211, in particular the auth/assoc/authorized flags
 *	might come to the driver in invalid combinations -- make sure to check
 *	them, also against the existing state! Drivers must call
 *	cfg80211_check_station_change() to validate the information.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
static int bl_cfg80211_change_station(struct wiphy *wiphy, struct net_device *dev,
                                        const u8 *mac, struct station_parameters *params)
#else
static int bl_cfg80211_change_station(struct wiphy *wiphy, struct net_device *dev,
                                        u8 *mac, struct station_parameters *params)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_sta *sta;

	BL_DBG(BL_FN_ENTRY_STR);

    sta = bl_get_sta(bl_hw, mac);
    if (!sta)
    {
        /* Add the TDLS station */
        if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        {
            struct bl_vif *bl_vif = netdev_priv(dev);
            struct me_sta_add_cfm me_sta_add_cfm;
            int error = 0;

            /* Indicate we are in a STA addition process - This will allow handling
             * potential PS mode change indications correctly
             */
            bl_hw->adding_sta = true;

            /* Forward the information to the LMAC */
            if ((error = bl_send_me_sta_add(bl_hw, params, mac, bl_vif->vif_index,
                                              &me_sta_add_cfm)))
                return error;

            // Check the status
            switch (me_sta_add_cfm.status)
            {
                case CO_OK:
                {
                    int tid;
                    sta = &bl_hw->sta_table[me_sta_add_cfm.sta_idx];
                    sta->aid = params->aid;
                    sta->sta_idx = me_sta_add_cfm.sta_idx;
                    sta->ch_idx = bl_vif->ch_index;
                    sta->vif_idx = bl_vif->vif_index;
                    sta->vlan_idx = sta->vif_idx;
                    sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
                    sta->ht = params->ht_capa ? 1 : 0;
                    sta->vht = params->vht_capa ? 1 : 0;
                    sta->acm = 0;
                    for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                        int uapsd_bit = bl_hwq2uapsd[bl_tid2hwq[tid]];
                        if (params->uapsd_queues & uapsd_bit)
                            sta->uapsd_tids |= 1 << tid;
                        else
                            sta->uapsd_tids &= ~(1 << tid);
                    }
                    memcpy(sta->mac_addr, mac, ETH_ALEN);
                    bl_dbgfs_register_rc_stat(bl_hw, sta);

                    /* Ensure that we won't process PS change or channel switch ind*/
                    spin_lock_bh(&bl_hw->cmd_mgr.lock);
                    bl_txq_sta_init(bl_hw, sta, bl_txq_vif_get_status(bl_vif));
                    sta->valid = true;
                    spin_unlock_bh(&bl_hw->cmd_mgr.lock);

                    #define PRINT_STA_FLAG(f)                               \
                        (params->sta_flags_set & BIT(NL80211_STA_FLAG_##f) ? "["#f"]" : "")

                    netdev_info(dev, "Add TDLS sta %d (%pM) flags=%s%s%s%s%s%s%s",
                                sta->sta_idx, mac,
                                PRINT_STA_FLAG(AUTHORIZED),
                                PRINT_STA_FLAG(SHORT_PREAMBLE),
                                PRINT_STA_FLAG(WME),
                                PRINT_STA_FLAG(MFP),
                                PRINT_STA_FLAG(AUTHENTICATED),
                                PRINT_STA_FLAG(TDLS_PEER),
                                PRINT_STA_FLAG(ASSOCIATED));
                    #undef PRINT_STA_FLAG

                    break;
                }
                default:
                    error = -EBUSY;
                    break;
            }

            bl_hw->adding_sta = false;
        } else  {
            return -EINVAL;
        }
    }

    if (params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED))
        bl_send_me_set_control_port_req(bl_hw,
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED)) != 0,
                sta->sta_idx);

    if (params->vlan) {
        uint8_t vlan_idx;

        vif = netdev_priv(params->vlan);
        vlan_idx = vif->vif_index;

        if (sta->vlan_idx != vlan_idx) {
            struct bl_vif *old_vif;
            old_vif = bl_hw->vif_table[sta->vlan_idx];
            bl_txq_sta_switch_vif(sta, old_vif, vif);
            sta->vlan_idx = vlan_idx;

            if ((BL_VIF_TYPE(vif) == NL80211_IFTYPE_AP_VLAN) &&
                (vif->use_4addr)) {
                WARN((vif->ap_vlan.sta_4a),
                     "4A AP_VLAN interface with more than one sta");
                vif->ap_vlan.sta_4a = sta;
            }

            if ((BL_VIF_TYPE(old_vif) == NL80211_IFTYPE_AP_VLAN) &&
                (old_vif->use_4addr)) {
                old_vif->ap_vlan.sta_4a = NULL;
            }
        }
    }

    return 0;
}

/**
 * @start_ap: Start acting in AP mode defined by the parameters.
 */
static int bl_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *dev,
                                  struct cfg80211_ap_settings *settings)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct apm_start_cfm apm_start_cfm;
    struct bl_dma_elem elem;
    struct bl_sta *sta;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Forward the information to the LMAC */
    if ((error = bl_send_apm_start_req(bl_hw, bl_vif, settings, &apm_start_cfm, &elem))) {
        return error;
	}

    // Check the status
    switch (apm_start_cfm.status)
    {
        case CO_OK:
        {
            u8 txq_status = 0;
            bl_vif->ap.bcmc_index = apm_start_cfm.bcmc_idx;
            bl_vif->ap.flags = 0;
            sta = &bl_hw->sta_table[apm_start_cfm.bcmc_idx];
            sta->valid = true;
            sta->aid = 0;
            sta->sta_idx = apm_start_cfm.bcmc_idx;
            sta->ch_idx = apm_start_cfm.ch_idx;
            sta->vif_idx = bl_vif->vif_index;
            sta->qos = false;
            sta->acm = 0;
            sta->ps.active = false;
            spin_lock_bh(&bl_hw->cmd_mgr.lock);
            bl_chanctx_link(bl_vif, apm_start_cfm.ch_idx,
                              &settings->chandef);
            if (bl_hw->cur_chanctx != apm_start_cfm.ch_idx) {
                txq_status = BL_TXQ_STOP_CHAN;
            }
            bl_txq_vif_init(bl_hw, bl_vif, txq_status);
            spin_unlock_bh(&bl_hw->cmd_mgr.lock);

            netif_tx_start_all_queues(dev);
            netif_carrier_on(dev);
            error = 0;
            break;
        }
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

    if (error) {
        netdev_info(dev, "Failed to start AP (%d)", error);
    } else {
        netdev_info(dev, "AP started: ch=%d, bcmc_idx=%d",
                    bl_vif->ch_index, bl_vif->ap.bcmc_index);
    }

    // Free the buffer used to build the beacon
    kfree(elem.buf);

    return error;
}


/**
 * @change_beacon: Change the beacon parameters for an access point mode
 *	interface. This should reject the call when AP mode wasn't started.
 */
static int bl_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *dev,
                                       struct cfg80211_beacon_data *info)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_bcn *bcn = &vif->ap.bcn;
    struct bl_dma_elem elem;
    u8 *buf;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    // Build the beacon
    buf = bl_build_bcn(bcn, info);
    if (!buf)
        return -ENOMEM;

    // Fill in the DMA structure
    elem.buf = buf;
    elem.len = bcn->len;

    // Forward the information to the LMAC
    error = bl_send_bcn_change(bl_hw, vif->vif_index, elem.buf,
                                 bcn->len, bcn->head_len, bcn->tim_len, NULL);

    kfree(elem.buf);

    return error;
}

/**
 * * @stop_ap: Stop being an AP, including stopping beaconing.
 */
static int bl_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_sta *sta;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
    struct station_del_parameters params;
    params.mac = NULL;
#endif

    bl_send_apm_stop_req(bl_hw, bl_vif);
    bl_chanctx_unlink(bl_vif);

    /* delete any remaining STA*/
    while (!list_empty(&bl_vif->ap.sta_list)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
        bl_cfg80211_del_station(wiphy, dev, &params);
#else
        bl_cfg80211_del_station(wiphy, dev, NULL);
#endif
    }

    /* delete BC/MC STA */
    sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];
    bl_txq_vif_deinit(bl_hw, bl_vif);
    bl_del_bcn(&bl_vif->ap.bcn);
    bl_del_csa(bl_vif);

    netif_tx_stop_all_queues(dev);
    netif_carrier_off(dev);

    netdev_info(dev, "AP Stopped");

    return 0;
}

/**
 * @set_monitor_channel: Set the monitor mode channel for the device. If other
 *	interfaces are active this callback should reject the configuration.
 *	If no interfaces are active or the device is down, the channel should
 *	be stored for when a monitor interface becomes active.
 */
static int bl_cfg80211_set_monitor_channel(struct wiphy *wiphy,
                                             struct cfg80211_chan_def *chandef)
{
    return 0;
}

/**
 * @probe_client: probe an associated client, must return a cookie that it
 *	later passes to cfg80211_probe_status().
 */
int bl_cfg80211_probe_client(struct wiphy *wiphy, struct net_device *dev,
            const u8 *peer, u64 *cookie)
{
    return 0;
}

/**
 * @mgmt_frame_register: Notify driver that a management frame type was
 *	registered. Note that this callback may not sleep, and cannot run
 *	concurrently with itself.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))

void bl_cfg80211_update_mgmt_frame_registrations(struct wiphy *wiphy,
                                            struct wireless_dev *wdev,
                                            struct mgmt_frame_regs *upd)
{
}

#else

void bl_cfg80211_mgmt_frame_register(struct wiphy *wiphy,
                   struct wireless_dev *wdev,
                   u16 frame_type, bool reg)
{
}

#endif


/**
 * @set_wiphy_params: Notify that wiphy parameters have changed;
 *	@changed bitfield (see &enum wiphy_params_flags) describes which values
 *	have changed. The actual parameter values are available in
 *	struct wiphy. If returning an error, no value should be changed.
 */
static int bl_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
    return 0;
}


/**
 * @set_tx_power: set the transmit power according to the parameters,
 *	the power passed is in mBm, to get dBm use MBM_TO_DBM(). The
 *	wdev may be %NULL if power was set for the wiphy, and will
 *	always be %NULL unless the driver supports per-vif TX power
 *	(as advertised by the nl80211 feature flag.)
 */
static int bl_cfg80211_set_tx_power(struct wiphy *wiphy, struct wireless_dev *wdev,
                                      enum nl80211_tx_power_setting type, int mbm)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif;
    s8 pwr;
    int res = 0;

    if (type == NL80211_TX_POWER_AUTOMATIC) {
        pwr = 0x7f;
    } else {
        pwr = MBM_TO_DBM(mbm);
    }

    if (wdev) {
        vif = container_of(wdev, struct bl_vif, wdev);
        res = bl_send_set_power(bl_hw, vif->vif_index, pwr, NULL);
    } else {
        list_for_each_entry(vif, &bl_hw->vifs, list) {
            res = bl_send_set_power(bl_hw, vif->vif_index, pwr, NULL);
            if (res)
                break;
        }
    }

    return res;
}

static int bl_cfg80211_set_txq_params(struct wiphy *wiphy, struct net_device *dev,
                                        struct ieee80211_txq_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    u8 hw_queue, aifs, cwmin, cwmax;
    u32 param;

    BL_DBG(BL_FN_ENTRY_STR);

    hw_queue = bl_ac2hwq[0][params->ac];

    aifs  = params->aifs;
    cwmin = fls(params->cwmin);
    cwmax = fls(params->cwmax);

    /* Store queue information in general structure */
    param  = (u32) (aifs << 0);
    param |= (u32) (cwmin << 4);
    param |= (u32) (cwmax << 8);
    param |= (u32) (params->txop) << 12;

    /* Send the MM_SET_EDCA_REQ message to the FW */
    return bl_send_set_edca(bl_hw, hw_queue, param, false, bl_vif->vif_index);
}


/**
 * @remain_on_channel: Request the driver to remain awake on the specified
 *	channel for the specified duration to complete an off-channel
 *	operation (e.g., public action frame exchange). When the driver is
 *	ready on the requested channel, it must indicate this with an event
 *	notification by calling cfg80211_ready_on_channel().
 */
static int
bl_cfg80211_remain_on_channel(struct wiphy *wiphy, struct wireless_dev *wdev,
                                struct ieee80211_channel *chan,
                                unsigned int duration, u64 *cookie)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(wdev->netdev);
    struct bl_roc_elem *roc_elem;
    int error;

    BL_DBG(BL_FN_ENTRY_STR);

    /* For debug purpose (use ftrace kernel option) */
    trace_roc(bl_vif->vif_index, chan->center_freq, duration);

    /* Check that no other RoC procedure has been launched */
    if (bl_hw->roc_elem) {
        return -EBUSY;
    }

    /* Allocate a temporary RoC element */
    roc_elem = kmalloc(sizeof(struct bl_roc_elem), GFP_KERNEL);

    /* Verify that element has well been allocated */
    if (!roc_elem) {
        return -ENOMEM;
    }

    /* Initialize the RoC information element */
    roc_elem->wdev = wdev;
    roc_elem->chan = chan;
    roc_elem->duration = duration;
    roc_elem->mgmt_roc = false;
    roc_elem->on_chan = false;

    /* Forward the information to the FMAC */
    error = bl_send_roc(bl_hw, bl_vif, chan, duration);

    /* If no error, keep all the information for handling of end of procedure */
    if (error == 0) {
        bl_hw->roc_elem = roc_elem;

        /* Set the cookie value */
        *cookie = (u64)(bl_hw->roc_cookie_cnt);

        /* Initialize the OFFCHAN TX queue to allow off-channel transmissions */
        bl_txq_offchan_init(bl_vif);
    } else {
        /* Free the allocated element */
        kfree(roc_elem);
    }

    return error;
}

/**
 * @cancel_remain_on_channel: Cancel an on-going remain-on-channel operation.
 *	This allows the operation to be terminated prior to timeout based on
 *	the duration value.
 */
static int bl_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
                                                  struct wireless_dev *wdev,
                                                  u64 cookie)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(wdev->netdev);

    BL_DBG(BL_FN_ENTRY_STR);

    /* For debug purpose (use ftrace kernel option) */
    trace_cancel_roc(bl_vif->vif_index);

    /* Check if a RoC procedure is pending */
    if (!bl_hw->roc_elem) {
        return 0;
    }

    /* Forward the information to the FMAC */
    return bl_send_cancel_roc(bl_hw);
}

/**
 * @dump_survey: get site survey information.
 */
static int bl_cfg80211_dump_survey(struct wiphy *wiphy, struct net_device *netdev,
                                     int idx, struct survey_info *info)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct ieee80211_supported_band *sband;
    struct bl_survey_info *bl_survey;

    BL_DBG(BL_FN_ENTRY_STR);

    if (idx >= ARRAY_SIZE(bl_hw->survey))
        return -ENOENT;

    bl_survey = &bl_hw->survey[idx];

    // Check if provided index matches with a supported 2.4GHz channel
    sband = wiphy->bands[NL80211_BAND_2GHZ];
    if (sband && idx >= sband->n_channels) {
        idx -= sband->n_channels;
        sband = NULL;
		return -ENOENT;
    }

    // Fill the survey
    info->channel = &sband->channels[idx];
    info->filled = bl_survey->filled;

    if (bl_survey->filled != 0) {
        info->time = (u64)bl_survey->chan_time_ms;
        info->time_busy = (u64)bl_survey->chan_time_busy_ms;
        info->noise = bl_survey->noise_dbm;

        // Set the survey report as not used
        bl_survey->filled = 0;
    }

    return 0;
}

/**
 * @get_channel: Get the current operating channel for the virtual interface.
 *	For monitor interfaces, it should return %NULL unless there's a single
 *	current monitoring channel.
 */
static int bl_cfg80211_get_channel(struct wiphy *wiphy,
                                     struct wireless_dev *wdev,
                                     struct cfg80211_chan_def *chandef) {

    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = container_of(wdev, struct bl_vif, wdev);
    struct bl_chanctx *ctxt;

    if (!bl_vif->up ||
        !bl_chanctx_valid(bl_hw, bl_vif->ch_index)) {
        return -ENODATA;
    }

    ctxt = &bl_hw->chanctx_table[bl_vif->ch_index];
    *chandef = ctxt->chan_def;

    return 0;
}

/**
 * @mgmt_tx: Transmit a management frame.
 */
static int bl_cfg80211_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
                                 struct cfg80211_mgmt_tx_params *params,
                                 u64 *cookie)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(wdev->netdev);
    struct bl_sta *bl_sta;
    struct ieee80211_mgmt *mgmt = (void *)params->buf;
    int error = 0;
    bool ap = false;
    bool offchan = false;

    do
    {
        /* Check if provided VIF is an AP or a STA one */
        switch (BL_VIF_TYPE(bl_vif)) {
        case NL80211_IFTYPE_AP_VLAN:
            bl_vif = bl_vif->ap_vlan.master;
        case NL80211_IFTYPE_AP:
        case NL80211_IFTYPE_P2P_GO:
        case NL80211_IFTYPE_MESH_POINT:
            ap = true;
            break;
        case NL80211_IFTYPE_STATION:
        case NL80211_IFTYPE_P2P_CLIENT:
        default:
            break;
        }

        /* Get STA on which management frame has to be sent */
        bl_sta = bl_retrieve_sta(bl_hw, bl_vif, mgmt->da,
                                     mgmt->frame_control, ap);

        /* For debug purpose (use ftrace kernel option) */
        trace_mgmt_tx((params->chan) ? params->chan->center_freq : 0,
                      bl_vif->vif_index,
                      (bl_sta) ? bl_sta->sta_idx : 0xFF,
                      mgmt);

        /* If AP, STA have to exist */
        if (!bl_sta) {
            if (!ap) {
                /* ROC is needed, check that channel parameters have been provided */
                if (!params->chan) {
                    error = -EINVAL;
                    break;
                }

                /* Check that a RoC is already pending */
                if (bl_hw->roc_elem) {
                    /* Get VIF used for current ROC */
                    struct bl_vif *bl_roc_vif = netdev_priv(bl_hw->roc_elem->wdev->netdev);

                    /* Check if RoC channel is the same than the required one */
                    if ((bl_hw->roc_elem->chan->center_freq != params->chan->center_freq)
                            || (bl_vif->vif_index != bl_roc_vif->vif_index)) {
                        error = -EINVAL;
                        break;
                    }
                } else {
                    u64 cookie;

                    /* Start a ROC procedure for 30ms */
                    error = bl_cfg80211_remain_on_channel(wiphy, wdev, params->chan,
                                                            30, &cookie);

                    if (error) {
                        break;
                    }

                    /*
                     * Need to keep in mind that RoC has been launched internally in order to
                     * avoid to call the cfg80211 callback once expired
                     */
                    bl_hw->roc_elem->mgmt_roc = true;
                }

                offchan = true;
            }
        }

        /* Push the management frame on the TX path */
        error = bl_start_mgmt_xmit(bl_vif, bl_sta, params, offchan, cookie);
    } while (0);

    return error;
}

/**
 * @update_ft_ies: Provide updated Fast BSS Transition information to the
 *	driver. If the SME is in the driver/firmware, this information can be
 *	used in building Authentication and Reassociation Request frames.
 */
static
int bl_cfg80211_update_ft_ies(struct wiphy *wiphy,
                            struct net_device *dev,
                            struct cfg80211_update_ft_ies_params *ftie)
{
    return 0;
}

/**
 * @set_cqm_rssi_config: Configure connection quality monitor RSSI threshold.
 */
static
int bl_cfg80211_set_cqm_rssi_config(struct wiphy *wiphy,
                                  struct net_device *dev,
                                  int32_t rssi_thold, uint32_t rssi_hyst)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);

    return bl_send_cfg_rssi_req(bl_hw, bl_vif->vif_index, rssi_thold, rssi_hyst);
}

/**
 *
 * @channel_switch: initiate channel-switch procedure (with CSA). Driver is
 *	responsible for veryfing if the switch is possible. Since this is
 *	inherently tricky driver may decide to disconnect an interface later
 *	with cfg80211_stop_iface(). This doesn't mean driver can accept
 *	everything. It should do it's best to verify requests and reject them
 *	as soon as possible.
 */
int bl_cfg80211_channel_switch(struct wiphy *wiphy,
                                 struct net_device *dev,
                                 struct cfg80211_csa_settings *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_dma_elem elem;
    struct bl_bcn *bcn, *bcn_after;
    struct bl_csa *csa;
    u16 csa_oft[BCN_MAX_CSA_CPT];
    u8 *buf;
    int i, error = 0;


    if (vif->ap.csa)
        return -EBUSY;

    if (params->n_counter_offsets_beacon > BCN_MAX_CSA_CPT)
        return -EINVAL;

    /* Build the new beacon with CSA IE */
    bcn = &vif->ap.bcn;
    buf = bl_build_bcn(bcn, &params->beacon_csa);
    if (!buf)
        return -ENOMEM;

    memset(csa_oft, 0, sizeof(csa_oft));
    for (i = 0; i < params->n_counter_offsets_beacon; i++)
    {
        csa_oft[i] = params->counter_offsets_beacon[i] + bcn->head_len +
            bcn->tim_len;
    }

    /* If count is set to 0 (i.e anytime after this beacon) force it to 2 */
    if (params->count == 0) {
        params->count = 2;
        for (i = 0; i < params->n_counter_offsets_beacon; i++)
        {
            buf[csa_oft[i]] = 2;
        }
    }

    elem.buf = buf;
    elem.len = bcn->len;

    /* Build the beacon to use after CSA. It will only be sent to fw once
       CSA is over. */
    csa = kzalloc(sizeof(struct bl_csa), GFP_KERNEL);
    if (!csa) {
        error = -ENOMEM;
        goto end;
    }


    bcn_after = &csa->bcn;
    buf = bl_build_bcn(bcn_after, &params->beacon_after);
    if (!buf) {
        error = -ENOMEM;
        bl_del_csa(vif);
        goto end;
    }

    vif->ap.csa = csa;
    csa->vif = vif;
    csa->chandef = params->chandef;
    csa->dma.buf = buf;
    csa->dma.len = bcn_after->len;

    /* Send new Beacon. FW will extract channel and count from the beacon */
    error = bl_send_bcn_change(bl_hw, vif->vif_index, elem.buf,
                                 bcn->len, bcn->head_len, bcn->tim_len, csa_oft);

    if (error) {
        bl_del_csa(vif);
        goto end;
    } else {
        INIT_WORK(&csa->work, bl_csa_finish);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0))
        cfg80211_ch_switch_started_notify(dev, &csa->chandef, params->count, false);
#else
        cfg80211_ch_switch_started_notify(dev, &csa->chandef, params->count);
#endif
    }

  end:
    kfree(elem.buf);

    return error;
}

/**
 * @change_bss: Modify parameters for a given BSS (mainly for AP mode).
 */
int bl_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *dev,
                             struct bss_parameters *params)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    int res =  -EOPNOTSUPP;

    if (((BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP) ||
         (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_P2P_GO)) &&
        (params->ap_isolate > -1)) {

        if (params->ap_isolate)
            bl_vif->ap.flags |= BL_AP_ISOLATE;
        else
            bl_vif->ap.flags &= ~BL_AP_ISOLATE;

        res = 0;
    }

    return res;
}

static struct cfg80211_ops bl_cfg80211_ops = {
    .add_virtual_intf = bl_cfg80211_add_iface,
    .del_virtual_intf = bl_cfg80211_del_iface,
    .change_virtual_intf = bl_cfg80211_change_iface,
    .scan = bl_cfg80211_scan,
    .connect = bl_cfg80211_connect,
    .disconnect = bl_cfg80211_disconnect,
    .add_key = bl_cfg80211_add_key,
    .get_key = bl_cfg80211_get_key,
    .del_key = bl_cfg80211_del_key,
    .set_default_key = bl_cfg80211_set_default_key,
    .set_default_mgmt_key = bl_cfg80211_set_default_mgmt_key,
    .add_station = bl_cfg80211_add_station,
    .del_station = bl_cfg80211_del_station,
    .change_station = bl_cfg80211_change_station,
    .mgmt_tx = bl_cfg80211_mgmt_tx,
    .start_ap = bl_cfg80211_start_ap,
    .change_beacon = bl_cfg80211_change_beacon,
    .stop_ap = bl_cfg80211_stop_ap,
    .set_monitor_channel = bl_cfg80211_set_monitor_channel,
    .probe_client = bl_cfg80211_probe_client,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0))
//	.update_mgmt_frame_registrations = cfg80211_rtw_update_mgmt_frame_registrations,
	.update_mgmt_frame_registrations = bl_cfg80211_update_mgmt_frame_registrations,
#else
//	.mgmt_frame_register = cfg80211_rtw_mgmt_frame_register,
    .mgmt_frame_register = bl_cfg80211_mgmt_frame_register,
#endif
    .set_wiphy_params = bl_cfg80211_set_wiphy_params,
    .set_txq_params = bl_cfg80211_set_txq_params,
    .set_tx_power = bl_cfg80211_set_tx_power,
    .remain_on_channel = bl_cfg80211_remain_on_channel,
    .cancel_remain_on_channel = bl_cfg80211_cancel_remain_on_channel,
    .dump_survey = bl_cfg80211_dump_survey,
    .get_channel = bl_cfg80211_get_channel,
    .update_ft_ies = bl_cfg80211_update_ft_ies,
    .set_cqm_rssi_config = bl_cfg80211_set_cqm_rssi_config,
    .channel_switch = bl_cfg80211_channel_switch,
    .change_bss = bl_cfg80211_change_bss,
};


/*********************************************************************
 * Init/Exit functions
 *********************************************************************/
static void bl_wdev_unregister(struct bl_hw *bl_hw)
{
    struct bl_vif *bl_vif, *tmp;

    rtnl_lock();
    list_for_each_entry_safe(bl_vif, tmp, &bl_hw->vifs, list) {
        bl_cfg80211_del_iface(bl_hw->wiphy, &bl_vif->wdev);
    }
    rtnl_unlock();
}

static void bl_set_vers(struct bl_hw *bl_hw)
{
    u32 vers = bl_hw->version_cfm.version_lmac;

    BL_DBG(BL_FN_ENTRY_STR);

    snprintf(bl_hw->wiphy->fw_version,
             sizeof(bl_hw->wiphy->fw_version), "%d.%d.%d.%d",
             (vers & (0xff << 24)) >> 24, (vers & (0xff << 16)) >> 16,
             (vers & (0xff <<  8)) >>  8, (vers & (0xff <<  0)) >>  0);
}

static void bl_reg_notifier(struct wiphy *wiphy,
                              struct regulatory_request *request)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);

    // For now trust all initiator
    bl_send_me_chan_config_req(bl_hw);
}

static int remap_pfn_open(struct inode *inode, struct file *file)
{
     struct mm_struct *mm = current->mm;
 
     printk("client: %s (%d)\n", current->comm, current->pid);
     printk("code  section: [0x%lx   0x%lx]\n", mm->start_code, mm->end_code);
     printk("data  section: [0x%lx   0x%lx]\n", mm->start_data, mm->end_data);
     printk("brk   section: s: 0x%lx, c: 0x%lx\n", mm->start_brk, mm->brk);
     printk("mmap  section: s: 0x%lx\n", mm->mmap_base);
     printk("stack section: s: 0x%lx\n", mm->start_stack);
     printk("arg   section: [0x%lx   0x%lx]\n", mm->arg_start, mm->arg_end);
     printk("env   section: [0x%lx   0x%lx]\n", mm->env_start, mm->env_end);
 
     return 0;
}

static int remap_pfn_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long pfn_start = (virt_to_phys(kbuff) >> PAGE_SHIFT) + vma->vm_pgoff;
    unsigned long virt_start = (unsigned long)kbuff + offset;
    unsigned long size = vma->vm_end - vma->vm_start;
    int ret = 0;

    printk("phy: 0x%lx, offset: 0x%lx, size: 0x%lx\n", pfn_start << PAGE_SHIFT, offset, size);

    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
    if (ret)
        printk("%s: remap_pfn_range failed at [0x%lx  0x%lx]\n",
            __func__, vma->vm_start, vma->vm_end);
    else
        printk("%s: map 0x%lx to 0x%lx, size: 0x%lx\n", __func__, virt_start,
            vma->vm_start, size);

    return ret;
}

static const struct file_operations remap_pfn_fops = {
    .owner = THIS_MODULE,
    .open = remap_pfn_open,
    .mmap = remap_pfn_mmap,
};

static struct miscdevice remap_pfn_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "bl_log",
    .fops = &remap_pfn_fops,
};


int bl_logbuf_create(struct bl_hw *bl_hw)
{
	int ret = 0;

	kbuff = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
	bl_hw->log_buff = kbuff;

	device_create_file(bl_hw->dev, &dev_attr_filter_severity);
	device_create_file(bl_hw->dev, &dev_attr_filter_module);
	
	ret = misc_register(&remap_pfn_misc);

	return ret;
}

void bl_logbuf_destroy(struct bl_hw *bl_hw)
{
	kfree(kbuff);

	device_remove_file(bl_hw->dev, &dev_attr_filter_severity);
	device_remove_file(bl_hw->dev, &dev_attr_filter_module);

    misc_deregister(&remap_pfn_misc);    
}

int bl_cfg80211_init(struct bl_plat *bl_plat, void **platform_data)
{
    struct bl_hw *bl_hw;
    struct bl_conf_file init_conf;
    int ret = 0;
    struct wiphy *wiphy;
    struct wireless_dev *wdev;
    int i;

    BL_DBG(BL_FN_ENTRY_STR);

    /* create a new wiphy for use with cfg80211 */
    wiphy = wiphy_new(&bl_cfg80211_ops, sizeof(struct bl_hw));

    if (!wiphy) {
        dev_err(bl_platform_get_dev(bl_plat), "Failed to create new wiphy\n");
        ret = -ENOMEM;
        goto err_out;
    }

    bl_hw = wiphy_priv(wiphy);
    bl_hw->wiphy = wiphy;
    bl_hw->plat = bl_plat;
    bl_hw->dev = bl_platform_get_dev(bl_plat);
    bl_hw->mod_params = &bl_mod_params;

    /* set device pointer for wiphy */
    set_wiphy_dev(wiphy, bl_hw->dev);

    /* Create cache to allocate sw_txhdr */
    bl_hw->sw_txhdr_cache = kmem_cache_create("bl_sw_txhdr_cache",
                                                sizeof(struct bl_sw_txhdr),
                                                0, 0, NULL);
    if (!bl_hw->sw_txhdr_cache) {
        wiphy_err(wiphy, "Cannot allocate cache for sw TX header\n");
        ret = -ENOMEM;
        goto err_cache;
    }

	bl_hw->agg_reodr_pkt_cache= kmem_cache_create("bl_agg_reodr_pkt_cache",
                                                sizeof(struct bl_agg_reord_pkt),
                                                0, 0, NULL);
    if (!bl_hw->agg_reodr_pkt_cache) {
        wiphy_err(wiphy, "Cannot allocate cache for agg reorder packet\n");
        ret = -ENOMEM;
        goto err_reodr;
    }

    bl_hw->mpa_rx_data.buf = kzalloc(BL_RX_DATA_BUF_SIZE_16K, GFP_KERNEL);
    if(!bl_hw->mpa_rx_data.buf){
        BL_DBG("allocate rx buf fail \n");
        ret = -ENOMEM;
        goto err_config;
	}

    bl_hw->mpa_tx_data.buf = kzalloc(BL_TX_DATA_BUF_SIZE_16K, GFP_KERNEL);	
	if(!bl_hw->mpa_tx_data.buf){
        BL_DBG("allocate tx buf fail \n");
        ret = -ENOMEM;
        goto err_alloc;	
    }

    if ((ret = bl_parse_configfile(bl_hw, BL_CONFIG_FW_NAME, &init_conf))) {
        wiphy_err(wiphy, "bl_parse_configfile failed\n");
        goto err_platon;
    }

    bl_hw->sec_phy_chan.band = NL80211_BAND_5GHZ;
    bl_hw->sec_phy_chan.type = PHY_CHNL_BW_20;
    bl_hw->sec_phy_chan.prim20_freq = 5500;
    bl_hw->sec_phy_chan.center_freq1 = 5500;
    bl_hw->sec_phy_chan.center_freq2 = 0;
    bl_hw->vif_started = 0;
    bl_hw->adding_sta = false;

    bl_hw->preq_ie.buf = NULL;

    for (i = 0; i < NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX; i++)
        bl_hw->avail_idx_map |= BIT(i);

    bl_hwq_init(bl_hw);

    for (i = 0; i < NX_NB_TXQ; i++) {
        bl_hw->txq[i].idx = TXQ_INACTIVE;
    }
	
    /* Initialize RoC element pointer to NULL, indicate that RoC can be started */
    bl_hw->roc_elem = NULL;
    /* Cookie can not be 0 */
    bl_hw->roc_cookie_cnt = 1;

    memcpy(wiphy->perm_addr, init_conf.mac_addr, ETH_ALEN);
    wiphy->mgmt_stypes = bl_default_mgmt_stypes;
    wiphy->bands[NL80211_BAND_2GHZ] = &bl_band_2GHz;
    wiphy->interface_modes =
        BIT(NL80211_IFTYPE_STATION)     |
        BIT(NL80211_IFTYPE_AP)          |
        BIT(NL80211_IFTYPE_AP_VLAN)     |
        BIT(NL80211_IFTYPE_P2P_CLIENT)  |
        BIT(NL80211_IFTYPE_P2P_GO);
    wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
        WIPHY_FLAG_HAS_CHANNEL_SWITCH |
        WIPHY_FLAG_4ADDR_STATION |
        WIPHY_FLAG_4ADDR_AP;

    wiphy->max_num_csa_counters = BCN_MAX_CSA_CPT;

    wiphy->max_remain_on_channel_duration = bl_hw->mod_params->roc_dur_max;

    wiphy->features |= NL80211_FEATURE_NEED_OBSS_SCAN |
        NL80211_FEATURE_SK_TX_STATUS |
        NL80211_FEATURE_VIF_TXPOWER;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
    wiphy->features |= NL80211_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE;
#endif

    wiphy->iface_combinations   = bl_combinations;
    /* -1 not to include combination with radar detection, will be re-added in
       bl_handle_dynparams if supported */
    wiphy->n_iface_combinations = ARRAY_SIZE(bl_combinations) - 1;
    wiphy->reg_notifier = bl_reg_notifier;

    wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

    wiphy->cipher_suites = cipher_suites;
    wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites) - NB_RESERVED_CIPHER;

    bl_hw->ext_capa[0] = WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING;
    bl_hw->ext_capa[7] = WLAN_EXT_CAPA8_OPMODE_NOTIF;

    wiphy->extended_capabilities = bl_hw->ext_capa;
    wiphy->extended_capabilities_mask = bl_hw->ext_capa;
    wiphy->extended_capabilities_len = ARRAY_SIZE(bl_hw->ext_capa);
	bl_hw->workqueue = alloc_workqueue("BL_WORK_QUEUE", WQ_HIGHPRI | WQ_MEM_RECLAIM | WQ_UNBOUND, 1);
	if (!bl_hw->workqueue) {	
		printk("creat workqueue failed\n");
		goto err_platon;
	}
	INIT_WORK(&bl_hw->main_work, main_wq_hdlr);

    INIT_LIST_HEAD(&bl_hw->vifs);

    mutex_init(&bl_hw->dbginfo.mutex);
    spin_lock_init(&bl_hw->tx_lock);
	spin_lock_init(&bl_hw->main_proc_lock);
	spin_lock_init(&bl_hw->int_lock);
	spin_lock_init(&bl_hw->cmd_lock);
	spin_lock_init(&bl_hw->resend_lock);
	spin_lock_init(&bl_hw->txq_lock);
	bl_hw->bl_processing = false;
	bl_hw->more_task_flag = false;
	bl_hw->cmd_sent = false;
	bl_hw->resend = false;
	bl_hw->recovery_flag = false;
	bl_hw->msg_idx = 1;

	for(i = 0; i < NX_REMOTE_STA_MAX*NX_NB_TID_PER_STA; i++)
		INIT_LIST_HEAD(&bl_hw->reorder_list[i]);

    if ((ret = bl_platform_on(bl_hw)))
        goto err_workqueue;

	*platform_data = bl_hw;
	sdio_set_drvdata(bl_hw->plat->func, bl_hw);

	bl_logbuf_create(bl_hw);
    //TODO add magic char 'L' to header file
    BL_DBG("Notify FW to start...\n");
    bl_write_reg(bl_hw, 0x60, 'L'); 

    /* Reset FW */
    if ((ret = bl_send_reset(bl_hw)))
        goto err_lmac_reqs;

    if ((ret = bl_send_version_req(bl_hw, &bl_hw->version_cfm)))
        goto err_lmac_reqs;
    bl_set_vers(bl_hw);

    if ((ret = bl_handle_dynparams(bl_hw, bl_hw->wiphy)))
        goto err_lmac_reqs;

    /* Set parameters to firmware */
    bl_send_me_config_req(bl_hw);

    if ((ret = wiphy_register(wiphy))) {
        wiphy_err(wiphy, "Could not register wiphy device\n");
        goto err_register_wiphy;
    }


    /* Set channel parameters to firmware (must be done after WiPHY registration) */
    bl_send_me_chan_config_req(bl_hw);
	
    *platform_data = bl_hw;

	//bl_logbuf_create(bl_hw);
	
    bl_dbgfs_register(bl_hw, "bl");


    rtnl_lock();

    /* Add an initial station interface */
    wdev = bl_interface_add(bl_hw, "wlan%d", NL80211_IFTYPE_STATION, NULL);

    rtnl_unlock();

    if (!wdev) {
        wiphy_err(wiphy, "Failed to instantiate a network device\n");
        ret = -ENOMEM;
        goto err_add_interface;
    }

    wiphy_info(wiphy, "New interface create %s", wdev->netdev->name);

    return 0;

err_add_interface:
    wiphy_unregister(bl_hw->wiphy);
err_register_wiphy:
err_lmac_reqs:
    bl_platform_off(bl_hw);
err_workqueue:
    if(bl_hw->workqueue)
        destroy_workqueue(bl_hw->workqueue);
err_platon:
    kfree(bl_hw->mpa_tx_data.buf);
err_alloc:
    kfree(bl_hw->mpa_rx_data.buf);
err_config:
    kmem_cache_destroy(bl_hw->agg_reodr_pkt_cache);
err_reodr:
    kmem_cache_destroy(bl_hw->sw_txhdr_cache);
err_cache:
    wiphy_free(wiphy);
err_out:
    return ret;
}

void bl_cfg80211_deinit(struct bl_hw *bl_hw)
{
    BL_DBG(BL_FN_ENTRY_STR);

	bl_logbuf_destroy(bl_hw);
    kfree(bl_hw->mpa_rx_data.buf);
    bl_hw->mpa_rx_data.buf = NULL;
    kfree(bl_hw->mpa_tx_data.buf);
    bl_hw->mpa_tx_data.buf = NULL;

    bl_dbgfs_unregister(bl_hw);

    bl_wdev_unregister(bl_hw);
    wiphy_unregister(bl_hw->wiphy);

	msleep(5000);

	destroy_workqueue(bl_hw->workqueue);

    bl_platform_off(bl_hw);
    kmem_cache_destroy(bl_hw->sw_txhdr_cache);
	kmem_cache_destroy(bl_hw->agg_reodr_pkt_cache);
    wiphy_free(bl_hw->wiphy);
}

static int __init bl_mod_init(void)
{
    BL_DBG(BL_FN_ENTRY_STR);
    bl_print_version();

    return bl_platform_register_drv();
}

static void __exit bl_mod_exit(void)
{
    BL_DBG(BL_FN_ENTRY_STR);

    bl_platform_unregister_drv();
}

module_init(bl_mod_init);
module_exit(bl_mod_exit);

MODULE_FIRMWARE(BL_CONFIG_FW_NAME);

MODULE_DESCRIPTION(RW_DRV_DESCRIPTION);
MODULE_VERSION(BL_VERS_MOD);
MODULE_AUTHOR(RW_DRV_COPYRIGHT " " RW_DRV_AUTHOR);
MODULE_LICENSE("GPL");
