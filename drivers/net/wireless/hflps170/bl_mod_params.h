/**
 ******************************************************************************
 *
 * @file bl_mod_params.h
 *
 * @brief Declaration of module parameters
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#ifndef _BL_MOD_PARAM_H_
#define _BL_MOD_PARAM_H_

struct bl_mod_params {
    bool ht_on;
    bool vht_on;
    int mcs_map;
    bool ldpc_on;
    bool vht_stbc;
    int phy_cfg;
    int uapsd_timeout;
    bool ap_uapsd_on;
    bool sgi;
    bool sgi80;
    bool use_2040;
    bool use_80;
    bool custregd;
    int nss;
    bool bfmee;
    bool bfmer;
    bool mesh;
    bool murx;
    bool mutx;
    bool mutx_on;
    unsigned int roc_dur_max;
    int listen_itv;
    bool listen_bcmc;
    int lp_clk_ppm;
    bool ps_on;
    int tx_lft;
    int amsdu_maxnb;
    int uapsd_queues;
    bool tdls;
};

extern struct bl_mod_params bl_mod_params;

struct bl_hw;
int bl_handle_dynparams(struct bl_hw *bl_hw, struct wiphy *wiphy);
void bl_enable_wapi(struct bl_hw *bl_hw);
void bl_enable_mfp(struct bl_hw *bl_hw);

#endif /* _BL_MOD_PARAM_H_ */
