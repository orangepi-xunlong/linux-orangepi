/**
 ******************************************************************************
 *
 * @file bl_msg_tx.c
 *
 * @brief TX function definitions
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#include <linux/version.h>
#include "bl_msg_tx.h"
#include "bl_mod_params.h"
#ifdef CONFIG_BL_BFMER
#include "bl_bfmer.h"
#endif //(CONFIG_BL_BFMER)
#include "bl_compat.h"

const struct mac_addr mac_addr_bcst = {{0xFFFF, 0xFFFF, 0xFFFF}};

/* Default MAC Rx filters that can be changed by mac80211
 * (via the configure_filter() callback) */
#define BL_MAC80211_CHANGEABLE        (                                       \
                                         NXMAC_ACCEPT_BA_BIT                  | \
                                         NXMAC_ACCEPT_BAR_BIT                 | \
                                         NXMAC_ACCEPT_OTHER_DATA_FRAMES_BIT   | \
                                         NXMAC_ACCEPT_PROBE_REQ_BIT           | \
                                         NXMAC_ACCEPT_PS_POLL_BIT               \
                                        )

/* Default MAC Rx filters that cannot be changed by mac80211 */
#define BL_MAC80211_NOT_CHANGEABLE    (                                       \
                                         NXMAC_ACCEPT_QO_S_NULL_BIT           | \
                                         NXMAC_ACCEPT_Q_DATA_BIT              | \
                                         NXMAC_ACCEPT_DATA_BIT                | \
                                         NXMAC_ACCEPT_OTHER_MGMT_FRAMES_BIT   | \
                                         NXMAC_ACCEPT_MY_UNICAST_BIT          | \
                                         NXMAC_ACCEPT_BROADCAST_BIT           | \
                                         NXMAC_ACCEPT_BEACON_BIT              | \
                                         NXMAC_ACCEPT_PROBE_RESP_BIT            \
                                        )

/* Default MAC Rx filter */
#define BL_DEFAULT_RX_FILTER  (BL_MAC80211_CHANGEABLE | BL_MAC80211_NOT_CHANGEABLE)

static const int bw2chnl[] = {
    [NL80211_CHAN_WIDTH_20_NOHT] = PHY_CHNL_BW_20,
    [NL80211_CHAN_WIDTH_20]      = PHY_CHNL_BW_20,
    [NL80211_CHAN_WIDTH_40]      = PHY_CHNL_BW_40,
    [NL80211_CHAN_WIDTH_80]      = PHY_CHNL_BW_80,
    [NL80211_CHAN_WIDTH_160]     = PHY_CHNL_BW_160,
    [NL80211_CHAN_WIDTH_80P80]   = PHY_CHNL_BW_80P80,
};

static const int chnl2bw[] = {
    [PHY_CHNL_BW_20]      = NL80211_CHAN_WIDTH_20,
    [PHY_CHNL_BW_40]      = NL80211_CHAN_WIDTH_40,
    [PHY_CHNL_BW_80]      = NL80211_CHAN_WIDTH_80,
    [PHY_CHNL_BW_160]     = NL80211_CHAN_WIDTH_160,
    [PHY_CHNL_BW_80P80]   = NL80211_CHAN_WIDTH_80P80,
};

/*****************************************************************************/
/*
 * Parse the ampdu density to retrieve the value in usec, according to the
 * values defined in ieee80211.h
 */
static inline u8 bl_ampdudensity2usec(u8 ampdudensity)
{
    switch (ampdudensity) {
    case IEEE80211_HT_MPDU_DENSITY_NONE:
        return 0;
        /* 1 microsecond is our granularity */
    case IEEE80211_HT_MPDU_DENSITY_0_25:
    case IEEE80211_HT_MPDU_DENSITY_0_5:
    case IEEE80211_HT_MPDU_DENSITY_1:
        return 1;
    case IEEE80211_HT_MPDU_DENSITY_2:
        return 2;
    case IEEE80211_HT_MPDU_DENSITY_4:
        return 4;
    case IEEE80211_HT_MPDU_DENSITY_8:
        return 8;
    case IEEE80211_HT_MPDU_DENSITY_16:
        return 16;
    default:
        return 0;
    }
}

static inline bool use_pairwise_key(struct cfg80211_crypto_settings *crypto)
{
    if ((crypto->cipher_group ==  WLAN_CIPHER_SUITE_WEP40) ||
        (crypto->cipher_group ==  WLAN_CIPHER_SUITE_WEP104))
        return false;

    return true;
}

static inline bool is_non_blocking_msg(int id) {
    return ((id == MM_TIM_UPDATE_REQ) || (id == ME_RC_SET_RATE_REQ) ||
            (id == ME_TRAFFIC_IND_REQ));
}

static inline uint8_t passive_scan_flag(uint32_t flags) {
    if (flags & (IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_RADAR))
        return SCAN_PASSIVE_BIT;
    return 0;
}

/**
 ******************************************************************************
 * @brief Allocate memory for a message
 *
 * This primitive allocates memory for a message that has to be sent. The memory
 * is allocated dynamically on the heap and the length of the variable parameter
 * structure has to be provided in order to allocate the correct size.
 *
 * Several additional parameters are provided which will be preset in the message
 * and which may be used internally to choose the kind of memory to allocate.
 *
 * The memory allocated will be automatically freed by the kernel, after the
 * pointer has been sent to ke_msg_send(). If the message is not sent, it must
 * be freed explicitly with ke_msg_free().
 *
 * Allocation failure is considered critical and should not happen.
 *
 * @param[in] id        Message identifier
 * @param[in] dest_id   Destination Task Identifier
 * @param[in] src_id    Source Task Identifier
 * @param[in] param_len Size of the message parameters to be allocated
 *
 * @return Pointer to the parameter member of the ke_msg. If the parameter
 *         structure is empty, the pointer will point to the end of the message
 *         and should not be used (except to retrieve the message pointer or to
 *         send the message)
 ******************************************************************************
 */
static inline void *bl_msg_zalloc(lmac_msg_id_t const id,
                                    lmac_task_id_t const dest_id,
                                    lmac_task_id_t const src_id,
                                    uint16_t const param_len)
{
    struct lmac_msg *msg;
    gfp_t flags;
     int len;

    len =(sizeof(struct lmac_msg) + param_len + 3)/4*4;


    if (is_non_blocking_msg(id) && in_softirq())
        flags = GFP_ATOMIC;
    else
        flags = GFP_KERNEL;


    msg = (struct lmac_msg *)kzalloc(len, flags);
    if (msg == NULL) {
        printk(KERN_CRIT "%s: msg allocation failed\n", __func__);
        return NULL;
    }

	msg->sdio_hdr.type = BL_TYPE_MSG;
	msg->sdio_hdr.len = len;
	msg->sdio_hdr.queue_idx = 0;
    msg->id = id;
    msg->dest_id = dest_id;
    msg->src_id = src_id;
    msg->param_len = param_len;

    return msg->param;
}

static int bl_send_msg(struct bl_hw *bl_hw, const void *msg_params,
                         int reqcfm, lmac_msg_id_t reqid, void *cfm)
{
    struct lmac_msg *msg;
    struct bl_cmd *cmd;
    bool nonblock;
    int ret;

    BL_DBG(BL_FN_ENTRY_STR);

    msg = container_of((void *)msg_params, struct lmac_msg, param);

    if (!test_bit(BL_DEV_STARTED, &bl_hw->drv_flags) &&
        reqid != MM_RESET_CFM && reqid != MM_VERSION_CFM &&
        reqid != MM_START_CFM && reqid != MM_SET_IDLE_CFM &&
        reqid != ME_CONFIG_CFM && reqid != MM_SET_PS_MODE_CFM &&
        reqid != ME_CHAN_CONFIG_CFM) {
        printk(KERN_CRIT "%s: bypassing (BL_DEV_RESTARTING set) 0x%02x\n",
               __func__, reqid);
        kfree(msg);
        return -EBUSY;
    } else if (!bl_hw->ipc_env) {
        printk(KERN_CRIT "%s: bypassing (restart must have failed)\n", __func__);
        kfree(msg);
        return -EBUSY;
    }

    nonblock = is_non_blocking_msg(msg->id);

    cmd = kzalloc(sizeof(struct bl_cmd), nonblock ? GFP_ATOMIC : GFP_KERNEL);
    cmd->result  = -EINTR;
    cmd->id      = msg->id;
    cmd->reqid   = reqid;
    cmd->a2e_msg = msg;
    cmd->e2a_msg = cfm;
    if (nonblock)
        cmd->flags = BL_CMD_FLAG_NONBLOCK;
    if (reqcfm)
        cmd->flags |= BL_CMD_FLAG_REQ_CFM;
    ret = bl_hw->cmd_mgr.queue(&bl_hw->cmd_mgr, cmd);

    if (!nonblock)
        kfree(cmd);
    else
        ret = cmd->result;

    return ret;
}

/******************************************************************************
 *    Control messages handling functions (SOFTMAC and  FULLMAC)
 *****************************************************************************/
int bl_send_reset(struct bl_hw *bl_hw)
{
    void *void_param;

    BL_DBG(BL_FN_ENTRY_STR);

    /* RESET REQ has no parameter */
    void_param = bl_msg_zalloc(MM_RESET_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;

    return bl_send_msg(bl_hw, void_param, 1, MM_RESET_CFM, NULL);
}

int bl_send_start(struct bl_hw *bl_hw)
{
    struct mm_start_req *start_req_param;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the START REQ message */
    start_req_param = bl_msg_zalloc(MM_START_REQ, TASK_MM, DRV_TASK_ID,
                                      sizeof(struct mm_start_req));
    if (!start_req_param)
        return -ENOMEM;

    /* Set parameters for the START message */
    memcpy(&start_req_param->phy_cfg, &bl_hw->phy_config, sizeof(bl_hw->phy_config));
    start_req_param->uapsd_timeout = (u32_l)bl_hw->mod_params->uapsd_timeout;
    start_req_param->lp_clk_accuracy = (u16_l)bl_hw->mod_params->lp_clk_ppm;

    /* Send the START REQ message to LMAC FW */
    return bl_send_msg(bl_hw, start_req_param, 1, MM_START_CFM, NULL);
}

int bl_send_version_req(struct bl_hw *bl_hw, struct mm_version_cfm *cfm)
{
    void *void_param;

    BL_DBG(BL_FN_ENTRY_STR);

    /* VERSION REQ has no parameter */
    void_param = bl_msg_zalloc(MM_VERSION_REQ, TASK_MM, DRV_TASK_ID, 0);
    if (!void_param)
        return -ENOMEM;

    return bl_send_msg(bl_hw, void_param, 1, MM_VERSION_CFM, cfm);
}

int bl_send_add_if(struct bl_hw *bl_hw, const unsigned char *mac,
                     enum nl80211_iftype iftype, bool p2p, struct mm_add_if_cfm *cfm)
{
    struct mm_add_if_req *add_if_req_param;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ADD_IF_REQ message */
    add_if_req_param = bl_msg_zalloc(MM_ADD_IF_REQ, TASK_MM, DRV_TASK_ID,
                                       sizeof(struct mm_add_if_req));
    if (!add_if_req_param)
        return -ENOMEM;

    /* Set parameters for the ADD_IF_REQ message */
    memcpy(&(add_if_req_param->addr.array[0]), mac, ETH_ALEN);
    switch (iftype) {
    #ifdef CONFIG_BL_FULLMAC
    case NL80211_IFTYPE_P2P_CLIENT:
        add_if_req_param->p2p = true;
        // no break
    #endif /* CONFIG_BL_FULLMAC */
    case NL80211_IFTYPE_STATION:
        add_if_req_param->type = MM_STA;
        break;

    case NL80211_IFTYPE_ADHOC:
        add_if_req_param->type = MM_IBSS;
        break;

    #ifdef CONFIG_BL_FULLMAC
    case NL80211_IFTYPE_P2P_GO:
        add_if_req_param->p2p = true;
        // no break
    #endif /* CONFIG_BL_FULLMAC */
    case NL80211_IFTYPE_AP:
        add_if_req_param->type = MM_AP;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        add_if_req_param->type = MM_MESH_POINT;
        break;
    case NL80211_IFTYPE_AP_VLAN:
        return -1;
    default:
        add_if_req_param->type = MM_STA;
        break;
    }

    /* Send the ADD_IF_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, add_if_req_param, 1, MM_ADD_IF_CFM, cfm);
}

int bl_send_remove_if(struct bl_hw *bl_hw, u8 vif_index)
{
    struct mm_remove_if_req *remove_if_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_REMOVE_IF_REQ message */
    remove_if_req = bl_msg_zalloc(MM_REMOVE_IF_REQ, TASK_MM, DRV_TASK_ID,
                                    sizeof(struct mm_remove_if_req));
    if (!remove_if_req)
        return -ENOMEM;

    /* Set parameters for the MM_REMOVE_IF_REQ message */
    remove_if_req->inst_nbr = vif_index;

    /* Send the MM_REMOVE_IF_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, remove_if_req, 1, MM_REMOVE_IF_CFM, NULL);
}

int bl_send_set_channel(struct bl_hw *bl_hw, int phy_idx,
                          struct mm_set_channel_cfm *cfm)
{
    struct mm_set_channel_req *set_chnl_par;
    enum nl80211_chan_width width;
    u16 center_freq, center_freq1, center_freq2;
    s8 tx_power = 0;
    enum nl80211_band band;

    BL_DBG(BL_FN_ENTRY_STR);

    if (phy_idx >= bl_hw->phy_cnt)
        return -ENOTSUPP;

    set_chnl_par = bl_msg_zalloc(MM_SET_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                                   sizeof(struct mm_set_channel_req));
    if (!set_chnl_par)
        return -ENOMEM;

    if (phy_idx == 0) {
        /* On FULLMAC only setting channel of secondary chain */
        wiphy_err(bl_hw->wiphy, "Trying to set channel of primary chain");
        return 0;
    } else {
        struct bl_sec_phy_chan *chan = &bl_hw->sec_phy_chan;

        width = chnl2bw[chan->type];
        band  = chan->band;
        center_freq  = chan->prim20_freq;
        center_freq1 = chan->center_freq1;
        center_freq2 = chan->center_freq2;
    }

    set_chnl_par->band = band;
    set_chnl_par->type = bw2chnl[width];
    set_chnl_par->prim20_freq  = center_freq;
    set_chnl_par->center1_freq = center_freq1;
    set_chnl_par->center2_freq = center_freq2;
    set_chnl_par->index = phy_idx;
    set_chnl_par->tx_power = tx_power;

    if (bl_hw->use_phy_bw_tweaks) {
        /* XXX Tweak for 80MHz VHT */
        if (width > NL80211_CHAN_WIDTH_40) {
            int _offs = center_freq1 - center_freq;
            set_chnl_par->type = PHY_CHNL_BW_40;
            set_chnl_par->center1_freq = center_freq + 10 *
                (_offs > 0 ? 1 : -1) * (abs(_offs) > 10 ? 1 : -1);
            BL_DBG("Tweak for 80MHz VHT: 80MHz chan requested\n");
        }
    }

    BL_DBG("mac80211:   freq=%d(c1:%d - c2:%d)/width=%d - band=%d\n"
             "   hw(%d): prim20=%d(c1:%d - c2:%d)/ type=%d - band=%d\n",
             center_freq, center_freq1,
             center_freq2, width, band,
             phy_idx, set_chnl_par->prim20_freq, set_chnl_par->center1_freq,
             set_chnl_par->center2_freq, set_chnl_par->type, set_chnl_par->band);

    /* Send the MM_SET_CHANNEL_REQ REQ message to LMAC FW */
    return bl_send_msg(bl_hw, set_chnl_par, 1, MM_SET_CHANNEL_CFM, cfm);
}


int bl_send_key_add(struct bl_hw *bl_hw, u8 vif_idx, u8 sta_idx, bool pairwise,
                      u8 *key, u8 key_len, u8 key_idx, u8 cipher_suite,
                      struct mm_key_add_cfm *cfm)
{
    struct mm_key_add_req *key_add_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_KEY_ADD_REQ message */
    key_add_req = bl_msg_zalloc(MM_KEY_ADD_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_key_add_req));
    if (!key_add_req)
        return -ENOMEM;

    /* Set parameters for the MM_KEY_ADD_REQ message */
    if (sta_idx != 0xFF) {
        /* Pairwise key */
        key_add_req->sta_idx = sta_idx;
    } else {
        /* Default key */
        key_add_req->sta_idx = sta_idx;
        key_add_req->key_idx = (u8_l)key_idx; /* only useful for default keys */
    }
    key_add_req->pairwise = pairwise;
    key_add_req->inst_nbr = vif_idx;
    key_add_req->key.length = key_len;
    memcpy(&(key_add_req->key.array[0]), key, key_len);

    key_add_req->cipher_suite = cipher_suite;

    BL_DBG("%s: sta_idx:%d key_idx:%d inst_nbr:%d cipher:%d key_len:%d\n", __func__,
             key_add_req->sta_idx, key_add_req->key_idx, key_add_req->inst_nbr,
             key_add_req->cipher_suite, key_add_req->key.length);
#if defined(CONFIG_BL_DBG) || defined(CONFIG_DYNAMIC_DEBUG)
    print_hex_dump_bytes("key: ", DUMP_PREFIX_OFFSET, key_add_req->key.array, key_add_req->key.length);
#endif

    /* Send the MM_KEY_ADD_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, key_add_req, 1, MM_KEY_ADD_CFM, cfm);
}

int bl_send_key_del(struct bl_hw *bl_hw, uint8_t hw_key_idx)
{
    struct mm_key_del_req *key_del_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_KEY_DEL_REQ message */
    key_del_req = bl_msg_zalloc(MM_KEY_DEL_REQ, TASK_MM, DRV_TASK_ID,
                                  sizeof(struct mm_key_del_req));
    if (!key_del_req)
        return -ENOMEM;

    /* Set parameters for the MM_KEY_DEL_REQ message */
    key_del_req->hw_key_idx = hw_key_idx;

    /* Send the MM_KEY_DEL_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, key_del_req, 1, MM_KEY_DEL_CFM, NULL);
}

int bl_send_bcn_change(struct bl_hw *bl_hw, u8 vif_idx, u8 *buf,
                         u16 bcn_len, u16 tim_oft, u16 tim_len, u16 *csa_oft)
{
    struct mm_bcn_change_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_BCN_CHANGE_REQ message */
    req = bl_msg_zalloc(MM_BCN_CHANGE_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_bcn_change_req) + bcn_len);
    if (!req)
        return -ENOMEM;

	memcpy(req->bcn_buf, buf, bcn_len);	

    /* Set parameters for the MM_BCN_CHANGE_REQ message */
    req->bcn_len = bcn_len;
    req->tim_oft = tim_oft;
    req->tim_len = tim_len;
    req->inst_nbr = vif_idx;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
    if (csa_oft) {
        int i;
        for (i = 0; i < BCN_MAX_CSA_CPT; i++) {
            req->csa_oft[i] = csa_oft[i];
        }
    }
#endif /* VERSION >= 3.16.0 */

    /* Send the MM_BCN_CHANGE_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, MM_BCN_CHANGE_CFM, NULL);
}

int bl_send_roc(struct bl_hw *bl_hw, struct bl_vif *vif,
                  struct ieee80211_channel *chan, unsigned  int duration)
{
    struct mm_remain_on_channel_req *req;
    struct cfg80211_chan_def chandef;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Create channel definition structure */
    cfg80211_chandef_create(&chandef, chan, NL80211_CHAN_NO_HT);

    /* Build the MM_REMAIN_ON_CHANNEL_REQ message */
    req = bl_msg_zalloc(MM_REMAIN_ON_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_remain_on_channel_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMAIN_ON_CHANNEL_REQ message */
    req->op_code      = MM_ROC_OP_START;
    req->vif_index    = vif->vif_index;
    req->duration_ms  = duration;
    req->band         = chan->band;
    req->type         = bw2chnl[chandef.width];
    req->prim20_freq  = chan->center_freq;
    req->center1_freq = chandef.center_freq1;
    req->center2_freq = chandef.center_freq2;
    req->tx_power     = chan->max_power;

    /* Send the MM_REMAIN_ON_CHANNEL_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, MM_REMAIN_ON_CHANNEL_CFM, NULL);
}

int bl_send_cancel_roc(struct bl_hw *bl_hw)
{
    struct mm_remain_on_channel_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_REMAIN_ON_CHANNEL_REQ message */
    req = bl_msg_zalloc(MM_REMAIN_ON_CHANNEL_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_remain_on_channel_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_REMAIN_ON_CHANNEL_REQ message */
    req->op_code = MM_ROC_OP_CANCEL;

    /* Send the MM_REMAIN_ON_CHANNEL_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 0, 0, NULL);
}

int bl_send_set_power(struct bl_hw *bl_hw, u8 vif_idx, s8 pwr,
                        struct mm_set_power_cfm *cfm)
{
    struct mm_set_power_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_SET_POWER_REQ message */
    req = bl_msg_zalloc(MM_SET_POWER_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_set_power_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_POWER_REQ message */
    req->inst_nbr = vif_idx;
    req->power = pwr;

    /* Send the MM_SET_POWER_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, MM_SET_POWER_CFM, cfm);
}

int bl_send_set_edca(struct bl_hw *bl_hw, u8 hw_queue, u32 param,
                       bool uapsd, u8 inst_nbr)
{
    struct mm_set_edca_req *set_edca_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_SET_EDCA_REQ message */
    set_edca_req = bl_msg_zalloc(MM_SET_EDCA_REQ, TASK_MM, DRV_TASK_ID,
                                   sizeof(struct mm_set_edca_req));
    if (!set_edca_req)
        return -ENOMEM;

    /* Set parameters for the MM_SET_EDCA_REQ message */
    set_edca_req->ac_param = param;
    set_edca_req->uapsd = uapsd;
    set_edca_req->hw_queue = hw_queue;
    set_edca_req->inst_nbr = inst_nbr;

    /* Send the MM_SET_EDCA_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, set_edca_req, 1, MM_SET_EDCA_CFM, NULL);
}

/******************************************************************************
 *    Control messages handling functions (FULLMAC only)
 *****************************************************************************/
#ifdef CONFIG_BL_FULLMAC
int bl_send_me_config_req(struct bl_hw *bl_hw)
{
    struct me_config_req *req;
    struct wiphy *wiphy = bl_hw->wiphy;
    struct ieee80211_sta_ht_cap *ht_cap = &wiphy->bands[NL80211_BAND_2GHZ]->ht_cap;
    uint8_t *ht_mcs = (uint8_t *)&ht_cap->mcs;
    int i;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ME_CONFIG_REQ message */
    req = bl_msg_zalloc(ME_CONFIG_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_config_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_CONFIG_REQ message */
    printk("HT supp %d\n", ht_cap->ht_supported);

    req->ht_supp = ht_cap->ht_supported;
    req->ht_cap.ht_capa_info = cpu_to_le16(ht_cap->cap);
    req->ht_cap.a_mpdu_param = ht_cap->ampdu_factor |
                                     (ht_cap->ampdu_density <<
                                         IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);
    for (i = 0; i < sizeof(ht_cap->mcs); i++)
        req->ht_cap.mcs_rate[i] = ht_mcs[i];
    req->ht_cap.ht_extended_capa = 0;
    req->ht_cap.tx_beamforming_capa = 0;
    req->ht_cap.asel_capa = 0;
	
    req->ps_on = bl_hw->mod_params->ps_on;
    req->tx_lft = bl_hw->mod_params->tx_lft;

    /* Send the ME_CONFIG_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_CONFIG_CFM, NULL);
}

int bl_send_me_chan_config_req(struct bl_hw *bl_hw)
{
    struct me_chan_config_req *req;
    struct wiphy *wiphy = bl_hw->wiphy;
    int i;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ME_CHAN_CONFIG_REQ message */
    req = bl_msg_zalloc(ME_CHAN_CONFIG_REQ, TASK_ME, DRV_TASK_ID,
                                            sizeof(struct me_chan_config_req));
    if (!req)
        return -ENOMEM;

    req->chan2G4_cnt=  0;
    if (wiphy->bands[NL80211_BAND_2GHZ] != NULL) {
        struct ieee80211_supported_band *b = wiphy->bands[NL80211_BAND_2GHZ];
        for (i = 0; i < b->n_channels; i++) {
            req->chan2G4[req->chan2G4_cnt].flags = 0;
            if (b->channels[i].flags & IEEE80211_CHAN_DISABLED)
                req->chan2G4[req->chan2G4_cnt].flags |= SCAN_DISABLED_BIT;
            req->chan2G4[req->chan2G4_cnt].flags |= passive_scan_flag(b->channels[i].flags);
            req->chan2G4[req->chan2G4_cnt].band = NL80211_BAND_2GHZ;
            req->chan2G4[req->chan2G4_cnt].freq = b->channels[i].center_freq;
            req->chan2G4[req->chan2G4_cnt].tx_power = b->channels[i].max_power;
            req->chan2G4_cnt++;
            if (req->chan2G4_cnt == SCAN_CHANNEL_2G4)
                break;
        }
    }

    req->chan5G_cnt = 0;
    if (wiphy->bands[NL80211_BAND_5GHZ] != NULL) {
        struct ieee80211_supported_band *b = wiphy->bands[NL80211_BAND_5GHZ];
        for (i = 0; i < b->n_channels; i++) {
            req->chan5G[req->chan5G_cnt].flags = 0;
            if (b->channels[i].flags & IEEE80211_CHAN_DISABLED)
                req->chan5G[req->chan5G_cnt].flags |= SCAN_DISABLED_BIT;
            req->chan5G[req->chan5G_cnt].flags |= passive_scan_flag(b->channels[i].flags);
            req->chan5G[req->chan5G_cnt].band = NL80211_BAND_5GHZ;
            req->chan5G[req->chan5G_cnt].freq = b->channels[i].center_freq;
            req->chan5G[req->chan5G_cnt].tx_power = b->channels[i].max_power;
            req->chan5G_cnt++;
            if (req->chan5G_cnt == SCAN_CHANNEL_5G)
                break;
        }
    }

    /* Send the ME_CHAN_CONFIG_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_CHAN_CONFIG_CFM, NULL);
}

int bl_send_me_set_control_port_req(struct bl_hw *bl_hw, bool opened, u8 sta_idx)
{
    struct me_set_control_port_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ME_SET_CONTROL_PORT_REQ message */
    req = bl_msg_zalloc(ME_SET_CONTROL_PORT_REQ, TASK_ME, DRV_TASK_ID,
                                   sizeof(struct me_set_control_port_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_SET_CONTROL_PORT_REQ message */
    req->sta_idx = sta_idx;
    req->control_port_open = opened;

    /* Send the ME_SET_CONTROL_PORT_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_SET_CONTROL_PORT_CFM, NULL);
}

int bl_send_me_sta_add(struct bl_hw *bl_hw, struct station_parameters *params,
                         const u8 *mac, u8 inst_nbr, struct me_sta_add_cfm *cfm)
{
    struct me_sta_add_req *req;
    u8 *ht_mcs = (u8 *)&params->ht_capa->mcs;
    int i;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_STA_ADD_REQ message */
    req = bl_msg_zalloc(ME_STA_ADD_REQ, TASK_ME, DRV_TASK_ID,
                                  sizeof(struct me_sta_add_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_ADD_REQ message */
    memcpy(&(req->mac_addr.array[0]), mac, ETH_ALEN);

    req->rate_set.length = params->supported_rates_len;
    for (i = 0; i < params->supported_rates_len; i++)
        req->rate_set.array[i] = params->supported_rates[i];

    req->flags = 0;
    if (params->ht_capa) {
        const struct ieee80211_ht_cap *ht_capa = params->ht_capa;

        req->flags |= STA_HT_CAPA;
        req->ht_cap.ht_capa_info = cpu_to_le16(ht_capa->cap_info);
        req->ht_cap.a_mpdu_param = ht_capa->ampdu_params_info;
        for (i = 0; i < sizeof(ht_capa->mcs); i++)
            req->ht_cap.mcs_rate[i] = ht_mcs[i];
        req->ht_cap.ht_extended_capa = cpu_to_le16(ht_capa->extended_ht_cap_info);
        req->ht_cap.tx_beamforming_capa = cpu_to_le32(ht_capa->tx_BF_cap_info);
        req->ht_cap.asel_capa = ht_capa->antenna_selection_info;
    }

    if (params->vht_capa) {
        const struct ieee80211_vht_cap *vht_capa = params->vht_capa;

        req->flags |= STA_VHT_CAPA;
        req->vht_cap.vht_capa_info = cpu_to_le32(vht_capa->vht_cap_info);
        req->vht_cap.rx_highest = cpu_to_le16(vht_capa->supp_mcs.rx_highest);
        req->vht_cap.rx_mcs_map = cpu_to_le16(vht_capa->supp_mcs.rx_mcs_map);
        req->vht_cap.tx_highest = cpu_to_le16(vht_capa->supp_mcs.tx_highest);
        req->vht_cap.tx_mcs_map = cpu_to_le16(vht_capa->supp_mcs.tx_mcs_map);
    }

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME))
        req->flags |= STA_QOS_CAPA;

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_MFP))
        req->flags |= STA_MFP_CAPA;

    if (params->opmode_notif_used) {
        req->flags |= STA_OPMOD_NOTIF;
        req->opmode = params->opmode_notif;
    }

    req->aid = cpu_to_le16(params->aid);
    req->uapsd_queues = params->uapsd_queues;
    req->max_sp_len = params->max_sp * 2;
    req->vif_idx = inst_nbr;

    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        req->tdls_sta = true;

    /* Send the ME_STA_ADD_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_STA_ADD_CFM, cfm);
}

int bl_send_me_sta_del(struct bl_hw *bl_hw, u8 sta_idx, bool tdls_sta)
{
    struct me_sta_del_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_STA_DEL_REQ message */
    req = bl_msg_zalloc(ME_STA_DEL_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_sta_del_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_STA_DEL_REQ message */
    req->sta_idx = sta_idx;
    req->tdls_sta = tdls_sta;

    /* Send the ME_STA_DEL_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_STA_DEL_CFM, NULL);
}

int bl_send_me_traffic_ind(struct bl_hw *bl_hw, u8 sta_idx, bool uapsd, u8 tx_status)
{
    struct me_traffic_ind_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ME_UTRAFFIC_IND_REQ message */
    req = bl_msg_zalloc(ME_TRAFFIC_IND_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_traffic_ind_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_TRAFFIC_IND_REQ message */
    req->sta_idx = sta_idx;
    req->tx_avail = tx_status;
    req->uapsd = uapsd;

    /* Send the ME_TRAFFIC_IND_REQ to UMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_TRAFFIC_IND_CFM, NULL);
}

int bl_send_me_rc_stats(struct bl_hw *bl_hw,
                          u8 sta_idx,
                          struct me_rc_stats_cfm *cfm)
{
    struct me_rc_stats_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ME_RC_STATS_REQ message */
    req = bl_msg_zalloc(ME_RC_STATS_REQ, TASK_ME, DRV_TASK_ID,
                                  sizeof(struct me_rc_stats_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_RC_STATS_REQ message */
    req->sta_idx = sta_idx;

    /* Send the ME_RC_STATS_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, ME_RC_STATS_CFM, cfm);
}

int bl_send_me_rc_set_rate(struct bl_hw *bl_hw,
                             u8 sta_idx,
                             u16 rate_cfg)
{
    struct me_rc_set_rate_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the ME_RC_SET_RATE_REQ message */
    req = bl_msg_zalloc(ME_RC_SET_RATE_REQ, TASK_ME, DRV_TASK_ID,
                          sizeof(struct me_rc_set_rate_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the ME_RC_SET_RATE_REQ message */
    req->sta_idx = sta_idx;
    req->fixed_rate_cfg = rate_cfg;

    /* Send the ME_RC_SET_RATE_REQ message to FW */
    return bl_send_msg(bl_hw, req, 0, 0, NULL);
}


int bl_send_sm_connect_req(struct bl_hw *bl_hw,
                             struct bl_vif *bl_vif,
                             struct cfg80211_connect_params *sme,
                             struct sm_connect_cfm *cfm)
{
    struct sm_connect_req *req;
    int i;
    u32_l flags = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the SM_CONNECT_REQ message */
    req = bl_msg_zalloc(SM_CONNECT_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_connect_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_CONNECT_REQ message */
    if (sme->crypto.n_ciphers_pairwise &&
        ((sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40) ||
         (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_TKIP) ||
         (sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP104)))
        flags |= DISABLE_HT;

    if (sme->crypto.control_port)
        flags |= CONTROL_PORT_HOST;

    if (sme->crypto.control_port_no_encrypt)
        flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&sme->crypto))
        flags |= WPA_WPA2_IN_USE;

    if (sme->mfp == NL80211_MFP_REQUIRED)
        flags |= MFP_IN_USE;

    if (sme->crypto.control_port_ethertype)
        req->ctrl_port_ethertype = sme->crypto.control_port_ethertype;
    else
        req->ctrl_port_ethertype = ETH_P_PAE;

    if (sme->bssid)
        memcpy(&req->bssid, sme->bssid, ETH_ALEN);
    else
        req->bssid = mac_addr_bcst;
    req->vif_idx = bl_vif->vif_index;
    if (sme->channel) {
        req->chan.band = sme->channel->band;
        req->chan.freq = sme->channel->center_freq;
        req->chan.flags = passive_scan_flag(sme->channel->flags);
    } else {
        req->chan.freq = (u16_l)-1;
    }
    for (i = 0; i < sme->ssid_len; i++)
        req->ssid.array[i] = sme->ssid[i];
    req->ssid.length = sme->ssid_len;
    req->flags = flags;
    if (WARN_ON(sme->ie_len > sizeof(req->ie_buf)))
        return -EINVAL;
    if (sme->ie_len)
        memcpy(req->ie_buf, sme->ie, sme->ie_len);
    req->ie_len = sme->ie_len;
    req->listen_interval = bl_mod_params.listen_itv;
    req->dont_wait_bcmc = !bl_mod_params.listen_bcmc;

    /* Set auth_type */
    if (sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC)
        req->auth_type = NL80211_AUTHTYPE_OPEN_SYSTEM;
    else
        req->auth_type = sme->auth_type;

    /* Set UAPSD queues */
    req->uapsd_queues = bl_mod_params.uapsd_queues;

    /* Send the SM_CONNECT_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, SM_CONNECT_CFM, cfm);
}

int bl_send_sm_disconnect_req(struct bl_hw *bl_hw,
                                struct bl_vif *bl_vif,
                                u16 reason)
{
    struct sm_disconnect_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the SM_DISCONNECT_REQ message */
    req = bl_msg_zalloc(SM_DISCONNECT_REQ, TASK_SM, DRV_TASK_ID,
                                   sizeof(struct sm_disconnect_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the SM_DISCONNECT_REQ message */
    req->reason_code = reason;
    req->vif_idx = bl_vif->vif_index;

    /* Send the SM_DISCONNECT_REQ message to LMAC FW */
    //return bl_send_msg(bl_hw, req, 1, SM_DISCONNECT_IND, NULL);
    return bl_send_msg(bl_hw, req, 1, SM_DISCONNECT_CFM, NULL);
}

int bl_send_apm_start_req(struct bl_hw *bl_hw, struct bl_vif *vif,
                            struct cfg80211_ap_settings *settings,
                            struct apm_start_cfm *cfm, struct bl_dma_elem *elem)
{
    struct apm_start_req *req;
    struct bl_bcn *bcn = &vif->ap.bcn;
    u8 *buf;
    u32 flags = 0;
    const u8 *rate_ie;
    u8 rate_len = 0;
    int var_offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
    const u8 *var_pos;
    int len, i;

    BL_DBG(BL_FN_ENTRY_STR);

    // Build the beacon
    bcn->dtim = (u8)settings->dtim_period;
    buf = bl_build_bcn(bcn, &settings->beacon);
    if (!buf) {
        return -ENOMEM;
    }

    /* Build the APM_START_REQ message */
    req = bl_msg_zalloc(APM_START_REQ, TASK_APM, DRV_TASK_ID,
                                   sizeof(struct apm_start_req) + bcn->len);
    if (!req)
        return -ENOMEM;

    // Retrieve the basic rate set from the beacon buffer
    len = bcn->len - var_offset;
    var_pos = buf + var_offset;

    rate_ie = cfg80211_find_ie(WLAN_EID_SUPP_RATES, var_pos, len);
    if (rate_ie) {
        const u8 *rates = rate_ie + 2;
        for (i = 0; i < rate_ie[1]; i++) {
            if (rates[i] & 0x80)
                req->basic_rates.array[rate_len++] = rates[i];
        }
    }
    rate_ie = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, var_pos, len);
    if (rate_ie) {
        const u8 *rates = rate_ie + 2;
        for (i = 0; i < rate_ie[1]; i++) {
            if (rates[i] & 0x80)
                req->basic_rates.array[rate_len++] = rates[i];
        }
    }
    req->basic_rates.length = rate_len;

    // Fill in the DMA structure
    elem->buf = buf;
    elem->len = bcn->len;
    //elem->dma_addr = dma_map_single(bl_hw->dev, elem->buf, elem->len,
    //                                DMA_TO_DEVICE);
	memcpy(req->bcn_buf, elem->buf, elem->len);
    /* Set parameters for the APM_START_REQ message */
    req->vif_idx = vif->vif_index;
    //req->bcn_addr = elem->dma_addr;
    req->bcn_len = bcn->len;
    req->tim_oft = bcn->head_len;
    req->tim_len = bcn->tim_len;
    req->chan.band = settings->chandef.chan->band;
    req->chan.freq = settings->chandef.chan->center_freq;
    req->chan.flags = 0;
    req->chan.tx_power = settings->chandef.chan->max_power;
    req->center_freq1 = settings->chandef.center_freq1;
    req->center_freq2 = settings->chandef.center_freq2;
    req->ch_width = bw2chnl[settings->chandef.width];
    req->bcn_int = settings->beacon_interval;
    if (settings->crypto.control_port)
        flags |= CONTROL_PORT_HOST;

    if (settings->crypto.control_port_no_encrypt)
        flags |= CONTROL_PORT_NO_ENC;

    if (use_pairwise_key(&settings->crypto))
        flags |= WPA_WPA2_IN_USE;

    if (settings->crypto.control_port_ethertype)
        req->ctrl_port_ethertype = settings->crypto.control_port_ethertype;
    else
        req->ctrl_port_ethertype = ETH_P_PAE;
    req->flags = flags;

    /* Send the APM_START_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, APM_START_CFM, cfm);
}

int bl_send_apm_stop_req(struct bl_hw *bl_hw, struct bl_vif *vif)
{
    struct apm_stop_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the APM_STOP_REQ message */
    req = bl_msg_zalloc(APM_STOP_REQ, TASK_APM, DRV_TASK_ID,
                                   sizeof(struct apm_stop_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_STOP_REQ message */
    req->vif_idx = vif->vif_index;

    /* Send the APM_STOP_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, APM_STOP_CFM, NULL);
}

int bl_send_scanu_req(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                        struct cfg80211_scan_request *param)
{
    struct scanu_start_req *req;
    int i;
    uint8_t chan_flags = 0;
	u16_l len;

    BL_DBG(BL_FN_ENTRY_STR);

	len = (param->ie == NULL) ? 0: param->ie_len;
	
    /* Build the SCANU_START_REQ message */
    req = bl_msg_zalloc(SCANU_START_REQ, TASK_SCANU, DRV_TASK_ID,
                          sizeof(struct scanu_start_req) + len);
    if (!req)
        return -ENOMEM;

    /* Set parameters */
    req->vif_idx = bl_vif->vif_index;
    req->chan_cnt = (u8)min_t(int, SCAN_CHANNEL_MAX, param->n_channels);
    req->ssid_cnt = (u8)min_t(int, SCAN_SSID_MAX, param->n_ssids);
    req->bssid = mac_addr_bcst;
    req->no_cck = param->no_cck;

    if (req->ssid_cnt == 0)
        chan_flags |= SCAN_PASSIVE_BIT;
    for (i = 0; i < req->ssid_cnt; i++) {
        int j;
        for (j = 0; j < param->ssids[i].ssid_len; j++)
            req->ssid[i].array[j] = param->ssids[i].ssid[j];
        req->ssid[i].length = param->ssids[i].ssid_len;
    }

	if (param->ie){
		memcpy(req->add_ies_buf, param->ie, param->ie_len);	
		req->add_ie_len = param->ie_len;
	}
	
    for (i = 0; i < req->chan_cnt; i++) {
        struct ieee80211_channel *chan = param->channels[i];

        req->chan[i].band = chan->band;
        req->chan[i].freq = chan->center_freq;
        req->chan[i].flags = chan_flags | passive_scan_flag(chan->flags);
        req->chan[i].tx_power = chan->max_reg_power;
    }

    /* Send the SCANU_START_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 0, 0, NULL);
}

int bl_send_apm_start_cac_req(struct bl_hw *bl_hw, struct bl_vif *vif,
                                struct cfg80211_chan_def *chandef,
                                struct apm_start_cac_cfm *cfm)
{
    struct apm_start_cac_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the APM_START_CAC_REQ message */
    req = bl_msg_zalloc(APM_START_CAC_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_start_cac_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_START_CAC_REQ message */
    req->vif_idx = vif->vif_index;
    req->chan.band = chandef->chan->band;
    req->chan.freq = chandef->chan->center_freq;
    req->chan.flags = 0;
    req->center_freq1 = chandef->center_freq1;
    req->center_freq2 = chandef->center_freq2;
    req->ch_width = bw2chnl[chandef->width];

    /* Send the APM_START_CAC_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, APM_START_CAC_CFM, cfm);
}

int bl_send_apm_stop_cac_req(struct bl_hw *bl_hw, struct bl_vif *vif)
{
    struct apm_stop_cac_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the APM_STOP_CAC_REQ message */
    req = bl_msg_zalloc(APM_STOP_CAC_REQ, TASK_APM, DRV_TASK_ID,
                          sizeof(struct apm_stop_cac_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the APM_STOP_CAC_REQ message */
    req->vif_idx = vif->vif_index;

    /* Send the APM_STOP_CAC_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, APM_STOP_CAC_CFM, NULL);
}
#endif /* CONFIG_BL_FULLMAC */

/**********************************************************************
 *    Debug Messages
 *********************************************************************/
int bl_send_dbg_trigger_req(struct bl_hw *bl_hw, char *msg)
{
    struct mm_dbg_trigger_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_DBG_TRIGGER_REQ message */
    req = bl_msg_zalloc(MM_DBG_TRIGGER_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_dbg_trigger_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_DBG_TRIGGER_REQ message */
    strncpy(req->error, msg, sizeof(req->error));

    /* Send the MM_DBG_TRIGGER_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 0, -1, NULL);
}

int bl_send_dbg_mem_read_req(struct bl_hw *bl_hw, u32 mem_addr,
                               struct dbg_mem_read_cfm *cfm)
{
    struct dbg_mem_read_req *mem_read_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the DBG_MEM_READ_REQ message */
    mem_read_req = bl_msg_zalloc(DBG_MEM_READ_REQ, TASK_DBG, DRV_TASK_ID,
                                   sizeof(struct dbg_mem_read_req));
    if (!mem_read_req)
        return -ENOMEM;

    /* Set parameters for the DBG_MEM_READ_REQ message */
    mem_read_req->memaddr = mem_addr;

    /* Send the DBG_MEM_READ_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, mem_read_req, 1, DBG_MEM_READ_CFM, cfm);
}

int bl_send_dbg_mem_write_req(struct bl_hw *bl_hw, u32 mem_addr,
                                u32 mem_data)
{
    struct dbg_mem_write_req *mem_write_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the DBG_MEM_WRITE_REQ message */
    mem_write_req = bl_msg_zalloc(DBG_MEM_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
                                    sizeof(struct dbg_mem_write_req));
    if (!mem_write_req)
        return -ENOMEM;

    /* Set parameters for the DBG_MEM_WRITE_REQ message */
    mem_write_req->memaddr = mem_addr;
    mem_write_req->memdata = mem_data;

    /* Send the DBG_MEM_WRITE_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, mem_write_req, 1, DBG_MEM_WRITE_CFM, NULL);
}

int bl_send_dbg_set_mod_filter_req(struct bl_hw *bl_hw, u32 filter)
{
    struct dbg_set_mod_filter_req *set_mod_filter_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the DBG_SET_MOD_FILTER_REQ message */
    set_mod_filter_req =
        bl_msg_zalloc(DBG_SET_MOD_FILTER_REQ, TASK_DBG, DRV_TASK_ID,
                        sizeof(struct dbg_set_mod_filter_req));
    if (!set_mod_filter_req)
        return -ENOMEM;

    /* Set parameters for the DBG_SET_MOD_FILTER_REQ message */
    set_mod_filter_req->mod_filter = filter;

    /* Send the DBG_SET_MOD_FILTER_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, set_mod_filter_req, 1, DBG_SET_MOD_FILTER_CFM, NULL);
}

int bl_send_dbg_set_sev_filter_req(struct bl_hw *bl_hw, u32 filter)
{
    struct dbg_set_sev_filter_req *set_sev_filter_req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the DBG_SET_SEV_FILTER_REQ message */
    set_sev_filter_req =
        bl_msg_zalloc(DBG_SET_SEV_FILTER_REQ, TASK_DBG, DRV_TASK_ID,
                        sizeof(struct dbg_set_sev_filter_req));
    if (!set_sev_filter_req)
        return -ENOMEM;

    /* Set parameters for the DBG_SET_SEV_FILTER_REQ message */
    set_sev_filter_req->sev_filter = filter;

    /* Send the DBG_SET_SEV_FILTER_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, set_sev_filter_req, 1, DBG_SET_SEV_FILTER_CFM, NULL);
}

int bl_send_dbg_get_sys_stat_req(struct bl_hw *bl_hw,
                                   struct dbg_get_sys_stat_cfm *cfm)
{
    void *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Allocate the message */
    req = bl_msg_zalloc(DBG_GET_SYS_STAT_REQ, TASK_DBG, DRV_TASK_ID, 0);
    if (!req)
        return -ENOMEM;

    /* Send the DBG_MEM_READ_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 1, DBG_GET_SYS_STAT_CFM, cfm);
}

int bl_send_cfg_rssi_req(struct bl_hw *bl_hw, u8 vif_index, int rssi_thold, u32 rssi_hyst)
{
    struct mm_cfg_rssi_req *req;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Build the MM_CFG_RSSI_REQ message */
    req = bl_msg_zalloc(MM_CFG_RSSI_REQ, TASK_MM, DRV_TASK_ID,
                          sizeof(struct mm_cfg_rssi_req));
    if (!req)
        return -ENOMEM;

    /* Set parameters for the MM_CFG_RSSI_REQ message */
    req->vif_index = vif_index;
    req->rssi_thold = (s8)rssi_thold;
    req->rssi_hyst = (u8)rssi_hyst;

    /* Send the MM_CFG_RSSI_REQ message to LMAC FW */
    return bl_send_msg(bl_hw, req, 0, 0, NULL);
}

