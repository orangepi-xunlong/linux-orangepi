/**
******************************************************************************
*
* @file bl_mod_params.c
*
* @brief Set configuration according to modules parameters
*
* Copyright (C) BouffaloLab 2017-2018
*
******************************************************************************
*/
#include <linux/module.h>

#include "bl_defs.h"
#include "bl_tx.h"
#include "hal_desc.h"
#include "bl_cfgfile.h"
#include "bl_compat.h"

#define COMMON_PARAM(name, default_softmac, default_fullmac)    \
    .name = default_fullmac,
#define SOFTMAC_PARAM(name, default)
#define FULLMAC_PARAM(name, default) .name = default,

struct bl_mod_params bl_mod_params = {
    /* common parameters */
    COMMON_PARAM(ht_on, true, true)
    COMMON_PARAM(vht_on, true, true)
    COMMON_PARAM(mcs_map, IEEE80211_VHT_MCS_SUPPORT_0_9, IEEE80211_VHT_MCS_SUPPORT_0_7)
    COMMON_PARAM(ldpc_on, false, false)
    COMMON_PARAM(vht_stbc, true, true)
    COMMON_PARAM(phy_cfg, 2, 2)
    COMMON_PARAM(uapsd_timeout, 300, 300)
    COMMON_PARAM(ap_uapsd_on, true, true)
    COMMON_PARAM(sgi, true, true)
    COMMON_PARAM(sgi80, true, true)
    COMMON_PARAM(use_2040, 0, 0)
    COMMON_PARAM(nss, 1, 1)
    COMMON_PARAM(bfmee, true, true)
    COMMON_PARAM(bfmer, false, false)
    COMMON_PARAM(mesh, false, false)
    COMMON_PARAM(murx, true, true)
    COMMON_PARAM(mutx, true, true)
    COMMON_PARAM(mutx_on, true, true)
    COMMON_PARAM(use_80, true, true)
    COMMON_PARAM(custregd, false, false)
    COMMON_PARAM(roc_dur_max, 500, 500)
    COMMON_PARAM(listen_itv, 0, 0)
    COMMON_PARAM(listen_bcmc, true, true)
    COMMON_PARAM(lp_clk_ppm, 20, 20)
    COMMON_PARAM(ps_on, false, false)
    COMMON_PARAM(tx_lft, BL_TX_LIFETIME_MS, BL_TX_LIFETIME_MS)
    COMMON_PARAM(amsdu_maxnb, NX_TX_PAYLOAD_MAX, NX_TX_PAYLOAD_MAX)
    // By default, only enable UAPSD for Voice queue (see IEEE80211_DEFAULT_UAPSD_QUEUE comment)
    COMMON_PARAM(uapsd_queues, IEEE80211_WMM_IE_STA_QOSINFO_AC_VO, IEEE80211_WMM_IE_STA_QOSINFO_AC_VO)
    COMMON_PARAM(tdls, true, true)
};

module_param_named(ht_on, bl_mod_params.ht_on, bool, S_IRUGO);
MODULE_PARM_DESC(ht_on, "Enable HT (Default: 1)");

module_param_named(vht_on, bl_mod_params.vht_on, bool, S_IRUGO);
MODULE_PARM_DESC(vht_on, "Enable VHT (Default: 1)");

module_param_named(mcs_map, bl_mod_params.mcs_map, int, S_IRUGO);
MODULE_PARM_DESC(mcs_map,  "VHT MCS map value  0: MCS0_7, 1: MCS0_8, 2: MCS0_9"
                 " (Default: 0)");

module_param_named(amsdu_maxnb, bl_mod_params.amsdu_maxnb, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(amsdu_maxnb, "Maximum number of MSDUs inside an A-MSDU in TX: (Default: NX_TX_PAYLOAD_MAX)");

module_param_named(ps_on, bl_mod_params.ps_on, bool, S_IRUGO);
MODULE_PARM_DESC(ps_on, "Enable PowerSaving (Default: 1-Enabled)");

module_param_named(tx_lft, bl_mod_params.tx_lft, int, 0644);
MODULE_PARM_DESC(tx_lft, "Tx lifetime (ms) - setting it to 0 disables retries "
                 "(Default: "__stringify(BL_TX_LIFETIME_MS)")");

module_param_named(ldpc_on, bl_mod_params.ldpc_on, bool, S_IRUGO);
MODULE_PARM_DESC(ldpc_on, "Enable LDPC (Default: 1)");

module_param_named(vht_stbc, bl_mod_params.vht_stbc, bool, S_IRUGO);
MODULE_PARM_DESC(vht_stbc, "Enable VHT STBC in RX (Default: 1)");

module_param_named(phycfg, bl_mod_params.phy_cfg, int, S_IRUGO);
MODULE_PARM_DESC(phycfg,
                 "0 <= phycfg <= 5 : RF Channel Conf (Default: 2(C0-A1-B2))");

module_param_named(uapsd_timeout, bl_mod_params.uapsd_timeout, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uapsd_timeout,
                 "UAPSD Timer timeout, in ms (Default: 300). If 0, UAPSD is disabled");

module_param_named(uapsd_queues, bl_mod_params.uapsd_queues, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uapsd_queues, "UAPSD Queues, integer value, must be seen as a bitfield\n"
                 "        Bit 0 = VO\n"
                 "        Bit 1 = VI\n"
                 "        Bit 2 = BK\n"
                 "        Bit 3 = BE\n"
                 "     -> uapsd_queues=7 will enable uapsd for VO, VI and BK queues");

module_param_named(ap_uapsd_on, bl_mod_params.ap_uapsd_on, bool, S_IRUGO);
MODULE_PARM_DESC(ap_uapsd_on, "Enable UAPSD in AP mode (Default: 1)");

module_param_named(sgi, bl_mod_params.sgi, bool, S_IRUGO);
MODULE_PARM_DESC(sgi, "Advertise Short Guard Interval support (Default: 1)");

module_param_named(sgi80, bl_mod_params.sgi80, bool, S_IRUGO);
MODULE_PARM_DESC(sgi80, "Advertise Short Guard Interval support for 80MHz (Default: 1)");

module_param_named(use_2040, bl_mod_params.use_2040, bool, S_IRUGO);
MODULE_PARM_DESC(use_2040, "Use tweaked 20-40MHz mode (Default: 1)");

module_param_named(use_80, bl_mod_params.use_80, bool, S_IRUGO);
MODULE_PARM_DESC(use_80, "Enable 80MHz (Default: 1)");

module_param_named(custregd, bl_mod_params.custregd, bool, S_IRUGO);
MODULE_PARM_DESC(custregd,
                 "Use permissive custom regulatory rules (for testing ONLY) (Default: 0)");

module_param_named(nss, bl_mod_params.nss, int, S_IRUGO);
MODULE_PARM_DESC(nss, "1 <= nss <= 2 : Supported number of Spatial Streams (Default: 1)");

module_param_named(bfmee, bl_mod_params.bfmee, bool, S_IRUGO);
MODULE_PARM_DESC(bfmee, "Enable Beamformee Capability (Default: 1-Enabled)");

module_param_named(bfmer, bl_mod_params.bfmer, bool, S_IRUGO);
MODULE_PARM_DESC(bfmer, "Enable Beamformer Capability (Default: 0-Disabled)");

module_param_named(mesh, bl_mod_params.mesh, bool, S_IRUGO);
MODULE_PARM_DESC(mesh, "Enable Meshing Capability (Default: 0-Disabled)");

module_param_named(murx, bl_mod_params.murx, bool, S_IRUGO);
MODULE_PARM_DESC(murx, "Enable MU-MIMO RX Capability (Default: 1-Enabled)");

module_param_named(mutx, bl_mod_params.mutx, bool, S_IRUGO);
MODULE_PARM_DESC(mutx, "Enable MU-MIMO TX Capability (Default: 1-Enabled)");

module_param_named(mutx_on, bl_mod_params.mutx_on, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(mutx_on, "Enable MU-MIMO transmissions (Default: 1-Enabled)");

module_param_named(roc_dur_max, bl_mod_params.roc_dur_max, int, S_IRUGO);
MODULE_PARM_DESC(roc_dur_max, "Maximum Remain on Channel duration");

module_param_named(listen_itv, bl_mod_params.listen_itv, int, S_IRUGO);
MODULE_PARM_DESC(listen_itv, "Maximum listen interval");

module_param_named(listen_bcmc, bl_mod_params.listen_bcmc, bool, S_IRUGO);
MODULE_PARM_DESC(listen_bcmc, "Wait for BC/MC traffic following DTIM beacon");

module_param_named(lp_clk_ppm, bl_mod_params.lp_clk_ppm, int, S_IRUGO);
MODULE_PARM_DESC(lp_clk_ppm, "Low Power Clock accuracy of the local device");

module_param_named(tdls, bl_mod_params.tdls, bool, S_IRUGO);
MODULE_PARM_DESC(tdls, "Enable TDLS (Default: 1-Enabled)");

/* Regulatory rules */
static const struct ieee80211_regdomain bl_regdom = {
    .n_reg_rules = 3,
    .alpha2 = "99",
    .reg_rules = {
        REG_RULE(2412 - 10, 2472 + 10, 40, 0, 1000, 0),
        REG_RULE(2484 - 10, 2484 + 10, 20, 0, 1000, 0),
        REG_RULE(5150 - 10, 5850 + 10, 80, 0, 1000, 0),
    }
};



/**
 * Do some sanity check
 *
 */
static int bl_check_fw_hw_feature(struct bl_hw *bl_hw,
                                    struct wiphy *wiphy)
{
    u32_l sys_feat = bl_hw->version_cfm.features;
    u32_l phy_feat = bl_hw->version_cfm.version_phy_1;

    if (!(sys_feat & BIT(MM_FEAT_UMAC_BIT))) {
        wiphy_err(wiphy,
                  "Loading softmac firmware with fullmac driver\n");
        return -1;
    }

    if (!(sys_feat & BIT(MM_FEAT_VHT_BIT))) {
        bl_hw->mod_params->vht_on = false;
    }

    if (!(sys_feat & BIT(MM_FEAT_PS_BIT))) {
        bl_hw->mod_params->ps_on = false;
    }

    /* AMSDU (non)support implies different shared structure definition
       so insure that fw and drv have consistent compilation option */
    if (sys_feat & BIT(MM_FEAT_AMSDU_BIT)) {
#ifndef CONFIG_BL_SPLIT_TX_BUF
        wiphy_err(wiphy,
                  "AMSDU enabled in firmware but support not compiled in driver\n");
        return -1;
#else
        if (bl_hw->mod_params->amsdu_maxnb > NX_TX_PAYLOAD_MAX)
            bl_hw->mod_params->amsdu_maxnb = NX_TX_PAYLOAD_MAX;
#endif /* CONFIG_BL_SPLIT_TX_BUF */
    } else {
#ifdef CONFIG_BL_SPLIT_TX_BUF
        wiphy_err(wiphy,
                  "AMSDU disabled in firmware but support compiled in driver\n");
        return -1;
#endif /* CONFIG_BL_SPLIT_TX_BUF */
    }

    if (!(sys_feat & BIT(MM_FEAT_UAPSD_BIT))) {
        bl_hw->mod_params->uapsd_timeout = 0;
    }

    if (sys_feat & BIT(MM_FEAT_WAPI_BIT)) {
        bl_enable_wapi(bl_hw);
    }

#ifdef CONFIG_BL_FULLMAC
    if (sys_feat & BIT(MM_FEAT_MFP_BIT)) {
        bl_enable_mfp(bl_hw);
    }
#endif

#define QUEUE_NAME "Broadcast/Multicast queue "

    if (sys_feat & BIT(MM_FEAT_BCN_BIT)) {
#if NX_TXQ_CNT == 4
        wiphy_err(wiphy, QUEUE_NAME
                  "enabled in firmware but support not compiled in driver\n");
        return -1;
#endif /* NX_TXQ_CNT == 4 */
    } else {
#if NX_TXQ_CNT == 5
        wiphy_err(wiphy, QUEUE_NAME
                  "disabled in firmware but support compiled in driver\n");
        return -1;
#endif /* NX_TXQ_CNT == 5 */
    }
#undef QUEUE_NAME

#ifdef CONFIG_BL_RADAR
    if (sys_feat & BIT(MM_FEAT_RADAR_BIT)) {
        /* Enable combination with radar detection */
        wiphy->n_iface_combinations++;
    }
#endif /* CONFIG_BL_RADAR */

#ifndef CONFIG_BL_SDM
    switch (__MDM_PHYCFG_FROM_VERS(phy_feat)) {
        case MDM_PHY_CONFIG_TRIDENT:
        case MDM_PHY_CONFIG_ELMA:
            bl_hw->mod_params->nss = 1;
            break;
        case MDM_PHY_CONFIG_KARST:
            {
                int nss_supp = (phy_feat & MDM_NSS_MASK) >> MDM_NSS_LSB;
                if (bl_hw->mod_params->nss > nss_supp)
                    bl_hw->mod_params->nss = nss_supp;
            }
            break;
        default:
            WARN_ON(1);
            break;
    }
#endif /* CONFIG_BL_SDM */

    if (bl_hw->mod_params->nss < 1 || bl_hw->mod_params->nss > 2)
        bl_hw->mod_params->nss = 1;

    wiphy_info(wiphy, "PHY features: [NSS=%d][CHBW=%d]%s\n",
               bl_hw->mod_params->nss,
               20 * (1 << ((phy_feat & MDM_CHBW_MASK) >> MDM_CHBW_LSB)),
               bl_hw->mod_params->ldpc_on ? "[LDPC]" : "");

#define PRINT_BL_FEAT(feat)                                   \
    (sys_feat & BIT(MM_FEAT_##feat##_BIT) ? "["#feat"]" : "")

    wiphy_info(wiphy, "FW features: %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
               PRINT_BL_FEAT(BCN),
               PRINT_BL_FEAT(AUTOBCN),
               PRINT_BL_FEAT(HWSCAN),
               PRINT_BL_FEAT(CMON),
               PRINT_BL_FEAT(MROLE),
               PRINT_BL_FEAT(RADAR),
               PRINT_BL_FEAT(PS),
               PRINT_BL_FEAT(UAPSD),
               PRINT_BL_FEAT(DPSM),
               PRINT_BL_FEAT(AMPDU),
               PRINT_BL_FEAT(AMSDU),
               PRINT_BL_FEAT(CHNL_CTXT),
               PRINT_BL_FEAT(REORD),
               PRINT_BL_FEAT(UMAC),
               PRINT_BL_FEAT(VHT),
               PRINT_BL_FEAT(WAPI),
               PRINT_BL_FEAT(MFP));
#undef PRINT_BL_FEAT

    return 0;
}



int bl_handle_dynparams(struct bl_hw *bl_hw, struct wiphy *wiphy)
{
    struct ieee80211_supported_band *band_2GHz = wiphy->bands[NL80211_BAND_2GHZ];
#ifndef CONFIG_BL_SDM
    u32 mdm_phy_cfg;
#endif
    int i, ret;
    int nss;
    int mcs_map;

    ret = bl_check_fw_hw_feature(bl_hw, wiphy);
    if (ret)
        return ret;

    /* FULLMAC specific parameters */
    wiphy->flags |= WIPHY_FLAG_REPORTS_OBSS;

    if (bl_hw->mod_params->tdls) {
        /* TDLS support */
        wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
#ifdef CONFIG_BL_FULLMAC
        /* TDLS external setup support */
        wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;
#endif
    }

    if (bl_hw->mod_params->ap_uapsd_on)
        wiphy->flags |= WIPHY_FLAG_AP_UAPSD;

    if (bl_hw->mod_params->phy_cfg < 0 || bl_hw->mod_params->phy_cfg > 5)
        bl_hw->mod_params->phy_cfg = 2;

    if (bl_hw->mod_params->mcs_map < 0 || bl_hw->mod_params->mcs_map > 2)
        bl_hw->mod_params->mcs_map = 0;

#ifndef CONFIG_BL_SDM
    mdm_phy_cfg = __MDM_PHYCFG_FROM_VERS(bl_hw->version_cfm.version_phy_1);
    if (mdm_phy_cfg == MDM_PHY_CONFIG_TRIDENT) {
        struct bl_phy_conf_file phy_conf;
        // Retrieve the Trident configuration
        bl_parse_phy_configfile(bl_hw, BL_PHY_CONFIG_TRD_NAME, &phy_conf);
        memcpy(&bl_hw->phy_config, &phy_conf.trd, sizeof(phy_conf.trd));
    } else if (mdm_phy_cfg == MDM_PHY_CONFIG_ELMA) {
    } else if (mdm_phy_cfg == MDM_PHY_CONFIG_KARST) {
        struct bl_phy_conf_file phy_conf;
        // We use the NSS parameter as is
        // Retrieve the Karst configuration
        bl_parse_phy_configfile(bl_hw, BL_PHY_CONFIG_KARST_NAME, &phy_conf);

        memcpy(&bl_hw->phy_config, &phy_conf.karst, sizeof(phy_conf.karst));
    } else {
        WARN_ON(1);
    }
#endif /* CONFIG_BL_SDM */

    nss = bl_hw->mod_params->nss;

    /* VHT capabilities */
    /*
     * MCS map:
     * This capabilities are filled according to the mcs_map module parameter.
     * However currently we have some limitations due to FPGA clock constraints
     * that prevent always using the range of MCS that is defined by the
     * parameter:
     *   - in RX, 2SS, we support up to MCS7
     *   - in TX, 2SS, we support up to MCS8
     */
    mcs_map = bl_hw->mod_params->mcs_map;
    //band_5GHz->vht_cap.vht_mcs.rx_mcs_map = cpu_to_le16(0);
    for (i = 0; i < nss; i++) {
        band_2GHz->ht_cap.mcs.rx_mask[i] = 0xFF;
        //band_5GHz->vht_cap.vht_mcs.rx_mcs_map |= cpu_to_le16(mcs_map << (i*2));
        mcs_map = IEEE80211_VHT_MCS_SUPPORT_0_7;
    }
    for (; i < 8; i++) {
        //band_5GHz->vht_cap.vht_mcs.rx_mcs_map |= cpu_to_le16(
            //IEEE80211_VHT_MCS_NOT_SUPPORTED << (i*2));
    }
    mcs_map = bl_hw->mod_params->mcs_map;
    //band_5GHz->vht_cap.vht_mcs.tx_mcs_map = cpu_to_le16(0);
    for (i = 0; i < nss; i++) {
        //band_5GHz->vht_cap.vht_mcs.tx_mcs_map |= cpu_to_le16(mcs_map << (i*2));
        mcs_map = min_t(int, bl_hw->mod_params->mcs_map,
                        IEEE80211_VHT_MCS_SUPPORT_0_8);
    }

    /*
     * LDPC capability:
     * This capability is filled according to the ldpc_on module parameter.
     * However currently we have some limitations due to FPGA clock constraints
     * that prevent correctly receiving more than MCS4-2SS when using LDPC.
     * We therefore disable the LDPC support if 2SS is supported.
     */
    bl_hw->mod_params->ldpc_on = nss > 1 ? false: bl_hw->mod_params->ldpc_on;

    /* HT capabilities */
    band_2GHz->ht_cap.cap |= 1 << IEEE80211_HT_CAP_RX_STBC_SHIFT;
    if (bl_hw->mod_params->ldpc_on)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_LDPC_CODING;
    if (bl_hw->mod_params->use_2040) {
        band_2GHz->ht_cap.mcs.rx_mask[4] = 0x1; /* MCS32 */
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
        band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(135 * nss);
    } else {
        band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(65 * nss);
    }
    if (nss > 1)
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_TX_STBC;

    if (bl_hw->mod_params->sgi) {
        band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SGI_20;
        if (bl_hw->mod_params->use_2040) {
            band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_SGI_40;
            band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(150 * nss);
        } else
            band_2GHz->ht_cap.mcs.rx_highest = cpu_to_le16(72 * nss);
    }
    band_2GHz->ht_cap.cap |= IEEE80211_HT_CAP_GRN_FLD;
    printk("--->ht_on=%d\n", bl_hw->mod_params->ht_on);
    if (!bl_hw->mod_params->ht_on)
        band_2GHz->ht_cap.ht_supported = false;

    if (bl_hw->mod_params->custregd) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
        // Apply custom regulatory. Note that for recent kernel versions we use instead the
        // REGULATORY_WIPHY_SELF_MANAGED flag, along with the regulatory_set_wiphy_regd()
        // function, that needs to be called after wiphy registration
        printk(KERN_CRIT
               "\n\n%s: CAUTION: USING PERMISSIVE CUSTOM REGULATORY RULES\n\n",
               __func__);
        wiphy->regulatory_flags |= REGULATORY_CUSTOM_REG;
        wiphy_apply_custom_regulatory(wiphy, &bl_regdom);
#endif
    }

    wiphy->max_scan_ssids = SCAN_SSID_MAX;
    wiphy->max_scan_ie_len = IEEE80211_MAX_DATA_LEN;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
    wiphy->support_mbssid = 1;
#endif

    /**
     * adjust caps with lower layers bl_hw->version_cfm
     */
#ifndef CONFIG_BL_SDM
    switch (mdm_phy_cfg) {
        case MDM_PHY_CONFIG_TRIDENT:
        {
            BL_DBG("%s: found Trident phy .. using phy bw tweaks\n", __func__);
            bl_hw->use_phy_bw_tweaks = true;
            break;
        }
        case MDM_PHY_CONFIG_ELMA:
            BL_DBG("%s: found ELMA phy .. disabling 2.4GHz and greenfield rx\n", __func__);
            wiphy->bands[NL80211_BAND_2GHZ] = NULL;
            band_2GHz->ht_cap.cap &= ~IEEE80211_HT_CAP_GRN_FLD;
            break;
        case MDM_PHY_CONFIG_KARST:
        {
            break;
        }
        default:
            WARN_ON(1);
            break;
    }
#endif /* CONFIG_BL_SDM */

    return 0;
}
