/**
 ****************************************************************************************
 *
 * @file bl_msg_tx.h
 *
 * @brief TX function declarations
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */

#ifndef _BL_MSG_TX_H_
#define _BL_MSG_TX_H_

#include "bl_defs.h"

/*
 * c.f LMAC/src/co/mac/mac_frame.h
 */
#define MAC_RSNIE_CIPHER_WEP40    0x00
#define MAC_RSNIE_CIPHER_TKIP     0x01
#define MAC_RSNIE_CIPHER_CCMP     0x02
#define MAC_RSNIE_CIPHER_WEP104   0x03
#define MAC_RSNIE_CIPHER_SMS4     0x04
#define MAC_RSNIE_CIPHER_AES_CMAC 0x05

enum bl_chan_types {
    PHY_CHNL_BW_20,
    PHY_CHNL_BW_40,
    PHY_CHNL_BW_80,
    PHY_CHNL_BW_160,
    PHY_CHNL_BW_80P80,
    PHY_CHNL_BW_OTHER,
};

int bl_send_reset(struct bl_hw *bl_hw);
int bl_send_start(struct bl_hw *bl_hw);
int bl_send_version_req(struct bl_hw *bl_hw, struct mm_version_cfm *cfm);
int bl_send_add_if(struct bl_hw *bl_hw, const unsigned char *mac,
                     enum nl80211_iftype iftype, bool p2p, struct mm_add_if_cfm *cfm);
int bl_send_remove_if(struct bl_hw *bl_hw, u8 vif_index);
int bl_send_set_channel(struct bl_hw *bl_hw, int phy_idx,
                          struct mm_set_channel_cfm *cfm);
int bl_send_key_add(struct bl_hw *bl_hw, u8 vif_idx, u8 sta_idx, bool pairwise,
                      u8 *key, u8 key_len, u8 key_idx, u8 cipher_suite,
                      struct mm_key_add_cfm *cfm);
int bl_send_key_del(struct bl_hw *bl_hw, uint8_t hw_key_idx);
int bl_send_bcn_change(struct bl_hw *bl_hw, u8 vif_idx, u8 *buf,
                         u16 bcn_len, u16 tim_oft, u16 tim_len, u16 *csa_oft);
int bl_send_tim_update(struct bl_hw *bl_hw, u8 vif_idx, u16 aid,
                         u8 tx_status);
int bl_send_roc(struct bl_hw *bl_hw, struct bl_vif *vif,
                  struct ieee80211_channel *chan, unsigned int duration);
int bl_send_cancel_roc(struct bl_hw *bl_hw);
int bl_send_set_power(struct bl_hw *bl_hw,  u8 vif_idx, s8 pwr,
                        struct mm_set_power_cfm *cfm);
int bl_send_set_edca(struct bl_hw *bl_hw, u8 hw_queue, u32 param,
                       bool uapsd, u8 inst_nbr);

int bl_send_me_config_req(struct bl_hw *bl_hw);
int bl_send_me_chan_config_req(struct bl_hw *bl_hw);
int bl_send_me_set_control_port_req(struct bl_hw *bl_hw, bool opened,
                                      u8 sta_idx);
int bl_send_me_sta_add(struct bl_hw *bl_hw, struct station_parameters *params,
                         const u8 *mac, u8 inst_nbr, struct me_sta_add_cfm *cfm);
int bl_send_me_sta_del(struct bl_hw *bl_hw, u8 sta_idx, bool tdls_sta);
int bl_send_me_traffic_ind(struct bl_hw *bl_hw, u8 sta_idx, bool uapsd, u8 tx_status);
int bl_send_me_rc_stats(struct bl_hw *bl_hw, u8 sta_idx,
                          struct me_rc_stats_cfm *cfm);
int bl_send_me_rc_set_rate(struct bl_hw *bl_hw,
                             u8 sta_idx,
                             u16 rate_idx);
int bl_send_sm_connect_req(struct bl_hw *bl_hw,
                             struct bl_vif *bl_vif,
                             struct cfg80211_connect_params *sme,
                             struct sm_connect_cfm *cfm);
int bl_send_sm_disconnect_req(struct bl_hw *bl_hw,
                                struct bl_vif *bl_vif,
                                u16 reason);
int bl_send_apm_start_req(struct bl_hw *bl_hw, struct bl_vif *vif,
                            struct cfg80211_ap_settings *settings,
                            struct apm_start_cfm *cfm, struct bl_dma_elem *elem);
int bl_send_apm_stop_req(struct bl_hw *bl_hw, struct bl_vif *vif);
int bl_send_scanu_req(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                        struct cfg80211_scan_request *param);
int bl_send_apm_start_cac_req(struct bl_hw *bl_hw, struct bl_vif *vif,
                                struct cfg80211_chan_def *chandef,
                                struct apm_start_cac_cfm *cfm);
int bl_send_apm_stop_cac_req(struct bl_hw *bl_hw, struct bl_vif *vif);
/* Debug messages */
int bl_send_dbg_trigger_req(struct bl_hw *bl_hw, char *msg);
int bl_send_dbg_mem_read_req(struct bl_hw *bl_hw, u32 mem_addr,
                               struct dbg_mem_read_cfm *cfm);
int bl_send_dbg_mem_write_req(struct bl_hw *bl_hw, u32 mem_addr,
                                u32 mem_data);
int bl_send_dbg_set_mod_filter_req(struct bl_hw *bl_hw, u32 filter);
int bl_send_dbg_set_sev_filter_req(struct bl_hw *bl_hw, u32 filter);
int bl_send_dbg_get_sys_stat_req(struct bl_hw *bl_hw,
                                   struct dbg_get_sys_stat_cfm *cfm);
int bl_send_cfg_rssi_req(struct bl_hw *bl_hw, u8 vif_index, int rssi_thold, u32 rssi_hyst);

#endif /* _BL_MSG_TX_H_ */
