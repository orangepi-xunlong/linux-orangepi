/*
 * Copyright (c) 2020 iComm-semi Ltd.
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
#if ((defined SSV_SUPPORT_SSV6020))
#include <linux/etherdevice.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include "ssv6020_cfg.h"
#include "ssv6020_mac.h"

#include <smac/dev.h>
#include <hal.h>
#include "ssv6020_priv.h"
#include <smac/ssv_skb.h>
#include <hci/hctrl.h>
#include <ssvdevice/ssv_cmd.h>
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
static struct ssv_hw * ssv6020_alloc_hw (void)
{
    struct ssv_hw *sh;

    sh = kzalloc(sizeof(struct ssv_hw), GFP_KERNEL);
    if (sh == NULL)
        goto out;

    memset((void *)sh, 0, sizeof(struct ssv_hw));

    sh->page_count = (u8 *)kzalloc(sizeof(u8) * SSV6020_ID_NUMBER, GFP_KERNEL);
    if (sh->page_count == NULL)
        goto out;

    memset(sh->page_count, 0, sizeof(u8) * SSV6020_ID_NUMBER);

    return sh;
out:
    if (sh->page_count)
        kfree(sh->page_count);
    if (sh)
        kfree(sh);

    return NULL;
}


static bool ssv6020_use_hw_encrypt(int cipher, struct ssv_softc *sc,
        struct ssv_sta_priv_data *sta_priv, struct ssv_vif_priv_data *vif_priv )
{

    return true;
}

static bool ssv6020_if_chk_mac2(struct ssv_hw *sh)
{
    printk(" %s: is not need to check MAC addres 2 for this model \n",__func__);
    return false;
}

static int ssv6020_get_wsid(struct ssv_softc *sc, struct ieee80211_vif *vif,
        struct ieee80211_sta *sta)
{
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
    int     s;
    struct ssv_sta_priv_data *sta_priv_dat=NULL;
    struct ssv_sta_info *sta_info;/* sta_info is already protected by ssv6200_sta_add(). */

    for (s = 0; s < SSV_NUM_STA; s++)
    {
        sta_info = &sc->sta_info[s];/* sta_info is already protected by ssv6200_sta_add(). */
        if ((sta_info->s_flags & STA_FLAG_VALID) == 0)/* sta_info is already protected by ssv6200_sta_add(). */
        {
            sta_info->aid = sta->aid;/* sta_info is already protected by ssv6200_sta_add(). */
            sta_info->sta = sta;/* sta_info is already protected by ssv6200_sta_add(). */
            sta_info->vif = vif;/* sta_info is already protected by ssv6200_sta_add(). */
            sta_info->s_flags = STA_FLAG_VALID;/* sta_info is already protected by ssv6200_sta_add(). */

            sta_priv_dat =
                (struct ssv_sta_priv_data *)sta->drv_priv;
            sta_priv_dat->sta_idx = s;
            sta_priv_dat->sta_info = sta_info;/* sta_info is already protected by ssv6200_sta_add(). */
            sta_priv_dat->has_hw_encrypt = false;
            sta_priv_dat->has_hw_decrypt = false;
            sta_priv_dat->need_sw_decrypt = false;
            sta_priv_dat->need_sw_encrypt = false;

            // WEP use single key for pairwise and broadcast frames.
            // In AP mode, key is only set when AP mode is initialized.
            if (   (vif_priv->pair_cipher == SECURITY_WEP40)
                    || (vif_priv->pair_cipher == SECURITY_WEP104))
            {
                sta_priv_dat->has_hw_encrypt = vif_priv->has_hw_encrypt;
                sta_priv_dat->has_hw_decrypt = vif_priv->has_hw_decrypt;
                sta_priv_dat->need_sw_encrypt = vif_priv->need_sw_encrypt;
                sta_priv_dat->need_sw_decrypt = vif_priv->need_sw_decrypt;
            }

            list_add_tail(&sta_priv_dat->list, &vif_priv->sta_list);

            //temp mark: PS
            //sc->ps_aid = sta->aid;
            break;
        }
    }
    return s;
}


static void ssv6020_add_fw_wsid(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv,
        struct ieee80211_sta *sta, struct ssv_sta_info *sta_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

static void ssv6020_del_fw_wsid(struct ssv_softc *sc, struct ieee80211_sta *sta,
        struct ssv_sta_info *sta_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

static void ssv6020_enable_fw_wsid(struct ssv_softc *sc, struct ieee80211_sta *sta,
        struct ssv_sta_info *sta_info, enum SSV6XXX_WSID_SEC key_type)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

static void ssv6020_disable_fw_wsid(struct ssv_softc *sc, int key_idx,
        struct ssv_sta_priv_data *sta_priv, struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

static void ssv6020_set_fw_hwwsid_sec_type(struct ssv_softc *sc, struct ieee80211_sta *sta,
        struct ssv_sta_info *sta_info, struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

//  return true if wep use hw cipher, default always use hw cipher
static bool ssv6020_wep_use_hw_cipher(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv)
{
    bool ret = false;

    if (!USE_MAC80211_CIPHER(sc->sh)) {
        ret = true;
    }

    return ret;
}

//  return true if wpa use hw cipher for pairwise, default always use hw cipher
static bool ssv6020_pairwise_wpa_use_hw_cipher(struct ssv_softc *sc,
        struct ssv_vif_priv_data *vif_priv, enum sec_type_en cipher,
        struct ssv_sta_priv_data *sta_priv)
{
    bool ret = false;

    if (!USE_MAC80211_CIPHER(sc->sh)) {
        ret = true;
    }

    return ret;
}

//  return true if group wpa use hw cipher
static bool ssv6020_group_wpa_use_hw_cipher(struct ssv_softc *sc,
        struct ssv_vif_priv_data *vif_priv, enum sec_type_en cipher)
{
    int     ret =false;

    if (!USE_MAC80211_CIPHER(sc->sh)) {
        ret = true;
    }

    return ret;
}


static bool ssv6020_chk_if_support_hw_bssid(struct ssv_softc *sc,
        int vif_idx)
{
    if ((vif_idx >= 0) && (vif_idx < SSV6020_NUM_HW_BSSID))
        return true;

    printk(" %s: VIF %d doesn't support HW BSSID\n", __func__, vif_idx);
    return false;
}

static void ssv6020_chk_dual_vif_chg_rx_flow(struct ssv_softc *sc,
        struct ssv_vif_priv_data *vif_priv)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

static void ssv6020_restore_rx_flow(struct ssv_softc *sc,
        struct ssv_vif_priv_data *vif_priv, struct ieee80211_sta *sta)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
            " %s: is not need for this model \n",__func__);
}

// SSV6020, when CCMP security use HW cipher and ccmp header was generated from SW.
// Allocates CCMP MIC field but not process it.
static bool ssv6020_put_mic_space_for_hw_ccmp_encrypt(struct ssv_softc *sc, struct sk_buff *skb)
{
    struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
    struct ieee80211_key_conf *hw_key = info->control.hw_key;
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) skb->data;
    struct SKB_info_st *skb_info = (struct SKB_info_st *)skb->head;
    struct ieee80211_sta *sta = skb_info->sta;
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
    struct ssv_sta_priv_data *ssv_sta_priv = sta ? (struct ssv_sta_priv_data *)sta->drv_priv : NULL;

    if ( (!ieee80211_is_data_qos(hdr->frame_control)
                && !ieee80211_is_data(hdr->frame_control))
            || !ieee80211_has_protected(hdr->frame_control) )
        return false;

    if (hw_key)
    {
        if(hw_key->cipher != WLAN_CIPHER_SUITE_CCMP)
            return false;
    }

    // Broadcast or Multicast frame
    if (!is_unicast_ether_addr(hdr->addr1))
    {
        if (vif_priv->is_security_valid
                && vif_priv->has_hw_encrypt)
        {
            pskb_expand_head(skb, 0, CCMP_MIC_LEN, GFP_ATOMIC);
            skb_put(skb, CCMP_MIC_LEN);
            return true;
        }
    }
    // Unicast
    else if (ssv_sta_priv != NULL)
    {
        if (ssv_sta_priv->has_hw_encrypt)
        {
            pskb_expand_head(skb, 0, CCMP_MIC_LEN, GFP_ATOMIC);
            skb_put(skb, CCMP_MIC_LEN);
            return true;
        }
    }

    return false;
}

static bool ssv6020_use_turismo_hw_encrypt(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv)
{
    if ((vif_priv->pair_cipher == SECURITY_CCMP)
            && (!USE_MAC80211_CIPHER(sc->sh))) {
        return true;
    } else {
        return false;
    }
}

static void ssv6020_init_tx_cfg(struct ssv_hw *sh)
{
    u32 dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);

    sh->total_tx_rx_page = SSV6020_TOTAL_PAGE;
    if ((sh->cfg.tx_page_threshold == 0) || (sh->cfg.tx_page_threshold > SSV6020_PAGE_TX_THRESHOLD)) {
        sh->tx_info.tx_page_threshold = SSV6020_PAGE_TX_THRESHOLD;
        sh->tx_info.tx_lowthreshold_page_trigger = SSV6020_TX_LOWTHRESHOLD_PAGE_TRIGGER;
    } else {
        sh->tx_info.tx_page_threshold = sh->cfg.tx_page_threshold;
        sh->tx_info.tx_lowthreshold_page_trigger =
            (sh->tx_info.tx_page_threshold - (sh->tx_info.tx_page_threshold/SSV6020_AMPDU_DIVIDER));
    }

    /* For security,  reserve the security table*/
    sh->total_tx_rx_page = sh->total_tx_rx_page - SSV6020_RESERVED_SEC_PAGE;
    sh->tx_info.tx_page_threshold = sh->tx_info.tx_page_threshold - SSV6020_RESERVED_SEC_PAGE;
    sh->tx_page_available = sh->tx_info.tx_page_threshold;

    /* For USB, sw resoure should take care of USB FIFO */
    if (dev_type == SSV_HWIF_INTERFACE_USB) {
        //sh->total_tx_rx_page = sh->total_tx_rx_page - SSV6020_USB_FIFO;
        //sh->tx_info.tx_page_threshold = sh->tx_info.tx_page_threshold - SSV6020_USB_FIFO;
        sh->tx_page_available = sh->tx_info.tx_page_threshold - SSV6020_RESERVED_USB_PAGE;
    }

    // set txid = txpage
    sh->tx_info.tx_id_threshold = sh->tx_info.tx_page_threshold>>1; //Divide by 2, to avoid tx+rx id is bigger than 128.
    sh->tx_info.tx_lowthreshold_id_trigger = (sh->tx_info.tx_id_threshold - 1);

    //printk ("%s: usb_hw_resource %d, tx_id_threshold %d, tx_page threshold %d\n", __func__,sh->cfg.usb_hw_resource,
    //    sh->tx_info.tx_id_threshold , sh->tx_info.tx_page_threshold );

    sh->tx_info.bk_txq_size = SSV6020_ID_AC_BK_OUT_QUEUE;
    sh->tx_info.be_txq_size = SSV6020_ID_AC_BE_OUT_QUEUE;
    sh->tx_info.vi_txq_size = SSV6020_ID_AC_VI_OUT_QUEUE;
    sh->tx_info.vo_txq_size = SSV6020_ID_AC_VO_OUT_QUEUE;
    sh->tx_info.manage_txq_size = SSV6020_ID_MANAGER_QUEUE;

    sh->ampdu_divider = SSV6020_AMPDU_DIVIDER;

    //info hci
    memcpy(&(sh->hci.hci_ctrl->tx_info), &(sh->tx_info), sizeof(struct ssv6xxx_tx_hw_info));
}

static void ssv6020_init_rx_cfg(struct ssv_hw *sh)
{

    // Set rx page
    sh->rx_info.rx_page_threshold = sh->total_tx_rx_page - sh->tx_info.tx_page_threshold;
    // Set rx id
    sh->rx_info.rx_id_threshold = sh->rx_info.rx_page_threshold>>1; //Divide by 2, to avoid tx+rx id is bigger than 128.

    sh->rx_info.rx_ba_ma_sessions = SSV6020_RX_BA_MAX_SESSIONS;
    //info hci
    memcpy(&(sh->hci.hci_ctrl->rx_info), &(sh->rx_info), sizeof(struct ssv6xxx_rx_hw_info));
    //printk ("%s: usb_hw_resource %d, rx_id_threshold %d, rx_page threshold %d\n", __func__,sh->cfg.usb_hw_resource,
    //	    sh->rx_info.rx_id_threshold , sh->rx_info.rx_page_threshold );
}

int ssv6020_ampdu_rx_start(struct ieee80211_hw *hw, struct ieee80211_vif *vif, struct ieee80211_sta *sta,
        u16 tid, u16 *ssn, u8 buf_size)
{
    struct ssv_softc *sc = hw->priv;
    struct ssv_sta_info *sta_info;/* sta_info is already protected by ssv6200_ampdu_action(). */
    int i = 0;
    bool find_peer = false;

    for (i = 0; i < SSV_NUM_STA; i++) {
        sta_info = &sc->sta_info[i];/* sta_info is already protected by ssv6200_ampdu_action(). */
        if ((sta_info->s_flags & STA_FLAG_VALID) && (sta == sta_info->sta)) {/* sta_info is already protected by ssv6200_ampdu_action(). */
            find_peer = true;
            break;
        }
    }

    if ((find_peer == false) || (sc->rx_ba_session_count > sc->sh->rx_info.rx_ba_ma_sessions))
        return -EBUSY;

    // notify mac80211 amdpu rx session success
    if (sta_info->s_flags & STA_FLAG_AMPDU_RX)/* sta_info is already protected by ssv6200_ampdu_action(). */
        return 0;

    printk(KERN_ERR "IEEE80211_AMPDU_RX_START %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
            sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
            sta->addr[4], sta->addr[5], tid);

    sc->rx_ba_session_count++;
    sta_info->s_flags |= STA_FLAG_AMPDU_RX;/* sta_info is already protected by ssv6200_ampdu_action(). */

    return 0;
}

static void ssv6020_adj_config(struct ssv_hw *sh)
{
    int dev_type;

    sh->cfg.hw_caps = sh->cfg.hw_caps & (~(SSV6200_HW_CAP_5GHZ));

    dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);

    if ((sh->cfg.tx_stuck_detect) && (dev_type == SSV_HWIF_INTERFACE_SDIO)) {
        printk("%s: tx_stuck_detect set to 1, force it to 0 \n", __func__);
        sh->cfg.tx_stuck_detect = false;
    }

    if (sh->cfg.hw_caps & SSV6200_HW_CAP_BEACON) {
        printk("%s: clear hw beacon \n", __func__);
        sh->cfg.hw_caps &= ~SSV6200_HW_CAP_BEACON;
    }

    if (sh->cfg.scan_hc_period == 0) {
        printk("%s: change scan home channel period to default value\n", __func__);
        sh->cfg.scan_hc_period = 100;
    }

    if (sh->cfg.scan_oc_period == 0) {
        printk("%s: change off home channel period to default value\n", __func__);
        sh->cfg.scan_oc_period = 50;
    }
}

static void ssv6020_get_fw_name(u8 *fw_name)
{
    strcpy(fw_name, "ssv6x5x-sw.bin");
}

static bool ssv6020_need_sw_cipher(struct ssv_hw *sh)
{
    return false;
}

static void ssv6020_send_tx_poll_cmd(struct ssv_hw *sh, u32 type)
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    int retval = 0;

    if (!sh->cfg.tx_stuck_detect)
        return;

    skb = ssv_skb_alloc(sh->sc, HOST_CMD_HDR_LEN);
    if (skb == NULL) {
        printk("%s:_skb_alloc fail!!!\n", __func__);
        return;
    }

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->RSVD0 = type;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_TX_POLL;
    host_cmd->len = skb->len;
    retval = HCI_SEND_CMD(sh, skb);
    if (retval)
        printk("%s(): Fail to send tx polling cmd\n", __FUNCTION__);
}

static void ssv6020_cmd_set_hwq_limit(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    char *endp;

    if ( argc != 3) return;

    if (!strcmp(argv[1], "bk")) {
        sh->cfg.bk_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "be")) {
        sh->cfg.be_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "vi")) {
        sh->cfg.vi_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "vo")) {
        sh->cfg.vo_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else if (!strcmp(argv[1], "mng")) {
        sh->cfg.manage_txq_size = simple_strtoul(argv[2], &endp, 0);
    } else {
        snprintf_res(cmd_data,"\t\t %s is unknown!\n", argv[1]);
    }
}

static void ssv6020_flash_read_all_map(struct ssv_hw *sh)
{
    struct file *fp = (struct file *)NULL;
    int rdlen = 0, i = 0;
    struct ssv6020_flash_layout_table flash_table;

    memset(&flash_table, 0, sizeof(struct ssv6020_flash_layout_table));

    if (sh->cfg.flash_bin_path[0] != 0x00)
        fp = filp_open(sh->cfg.flash_bin_path, O_RDONLY, 0);
    else
        fp = filp_open(DEFAULT_CFG_BIN_NAME, O_RDONLY, 0);

    if (IS_ERR(fp) || fp == NULL)
        fp = filp_open(SEC_CFG_BIN_NAME, O_RDONLY, 0);

    if (IS_ERR(fp) || fp == NULL) {
        printk("flash_file %s not found\n", DEFAULT_CFG_BIN_NAME);
        return;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    rdlen = kernel_read(fp, (u8 *)&flash_table, sizeof(struct ssv6020_flash_layout_table), &fp->f_pos);
#else
    rdlen = kernel_read(fp, fp->f_pos, (u8 *)&flash_table, sizeof(struct ssv6020_flash_layout_table));
#endif
    filp_close((struct file *)fp, NULL);
    if (rdlen < 0)
        return;
#if 0
#define SV6155P_IC      0x6155
#define SV6156P_IC      0x6156
#define SV6255P_IC      0x6255
#define SV6256P_IC      0x6256
    if (!((flash_table.ic == SV6155P_IC) ||
                (flash_table.ic == SV6156P_IC) ||
                (flash_table.ic == SV6255P_IC) ||
                (flash_table.ic == SV6256P_IC))) {
        printk("Invalid flash table [ic=0x%04x]\n", flash_table.ic);
        BUG_ON(1);
    }
#endif
    sh->flash_config.exist = true;
    sh->flash_config.dcdc = flash_table.dcdc;
    sh->cfg.volt_regulator = ((sh->flash_config.dcdc) ? SSV6XXX_VOLT_DCDC_CONVERT : SSV6XXX_VOLT_LDO_CONVERT);
    sh->flash_config.padpd = flash_table.padpd;
    sh->cfg.padpd = ((sh->flash_config.padpd) ? 0x1 : 0);
    sh->flash_config.xtal_offset_tempe_state = SSV_TEMPERATURE_NORMAL;
    sh->flash_config.xtal_offset_high_boundary = flash_table.xtal_offset_high_boundary;
    sh->flash_config.xtal_offset_low_boundary = flash_table.xtal_offset_low_boundary;
    sh->flash_config.band_gain_tempe_state = SSV_TEMPERATURE_NORMAL;
    sh->flash_config.band_gain_high_boundary = flash_table.band_gain_high_boundary;
    sh->flash_config.band_gain_low_boundary = flash_table.band_gain_low_boundary;

    sh->flash_config.rt_config.xtal_offset = flash_table.xtal_offset_rt_config;
    sh->flash_config.lt_config.xtal_offset = flash_table.xtal_offset_lt_config;
    sh->flash_config.ht_config.xtal_offset = flash_table.xtal_offset_ht_config;

    sh->flash_config.g_band_pa_bias0 = flash_table.g_band_pa_bias0;
    sh->flash_config.g_band_pa_bias1 = flash_table.g_band_pa_bias1;
    sh->flash_config.a_band_pa_bias0 = flash_table.a_band_pa_bias0;
    sh->flash_config.a_band_pa_bias1 = flash_table.a_band_pa_bias1;

#define SIZE_G_BAND_GAIN   (sizeof(sh->flash_config.rt_config.g_band_gain) / sizeof((sh->flash_config.rt_config.g_band_gain)[0]))
    for (i = 0; i < SIZE_G_BAND_GAIN; i++) {
        sh->flash_config.rt_config.g_band_gain[i] = flash_table.g_band_gain_rt[i];
        sh->flash_config.lt_config.g_band_gain[i] = flash_table.g_band_gain_lt[i];
        sh->flash_config.ht_config.g_band_gain[i] = flash_table.g_band_gain_ht[i];
    }

#define SIZE_A_BAND_GAIN   (sizeof(sh->flash_config.rt_config.a_band_gain) / sizeof((sh->flash_config.rt_config.a_band_gain)[0]))
    for (i = 0; i < SIZE_A_BAND_GAIN; i++) {
        sh->flash_config.rt_config.a_band_gain[i] = flash_table.a_band_gain_rt[i];
        sh->flash_config.lt_config.a_band_gain[i] = flash_table.a_band_gain_lt[i];
        sh->flash_config.ht_config.a_band_gain[i] = flash_table.a_band_gain_ht[i];
    }

#define SIZE_RATE_DELTA   (sizeof(sh->flash_config.rate_delta) / sizeof((sh->flash_config.rate_delta)[0]))
    for (i = 0; i < SIZE_RATE_DELTA; i++) {
        sh->flash_config.rate_delta[i] = flash_table.rate_delta[i];
    }

    if (!(sh->sc->log_ctrl & LOG_FLASH_BIN))
        return;

    printk("flash.bin configuration\n");

    printk("xtal_offset_boundary, high = %d, low = %d\n",
            sh->flash_config.xtal_offset_high_boundary, sh->flash_config.xtal_offset_low_boundary);
    printk("band_gain_boundary, high = %d, low = %d\n",
            sh->flash_config.band_gain_high_boundary, sh->flash_config.band_gain_low_boundary);

    printk("xtal_offset, rt = 0x%04x, lt = 0x%04x, ht = 0x%04x\n",
            sh->flash_config.rt_config.xtal_offset,
            sh->flash_config.lt_config.xtal_offset,
            sh->flash_config.ht_config.xtal_offset);

    printk("g_band_pa_bias0 = 0x%08x, g_band_pa_bias1 = 0x%08x\n",
            sh->flash_config.g_band_pa_bias0, sh->flash_config.g_band_pa_bias1);
    printk("a_band_pa_bias0 = 0x%08x, a_band_pa_bias1 = 0x%08x\n",
            sh->flash_config.a_band_pa_bias0, sh->flash_config.a_band_pa_bias1);

    printk("g band gain:\trt\tlt\tht\n");
    for (i = 0; i < SIZE_G_BAND_GAIN; i++) {
        printk("\t\t\t0x%x,\t0x%x,\t0x%x", sh->flash_config.rt_config.g_band_gain[i],
                sh->flash_config.lt_config.g_band_gain[i],
                sh->flash_config.ht_config.g_band_gain[i]);
    }
    printk("\n");

    printk("a band gain:\trt\tlt\tht\n");
    for (i = 0; i < SIZE_A_BAND_GAIN; i++) {
        printk("\t\t\t0x%x,\t0x%x,\t0x%x", sh->flash_config.rt_config.a_band_gain[i],
                sh->flash_config.lt_config.a_band_gain[i],
                sh->flash_config.ht_config.a_band_gain[i]);
    }
    printk("\n");

    printk("rate delta: ");
    for (i = 0; i < SIZE_RATE_DELTA; i++)
        printk("%x, ", sh->flash_config.rate_delta[i]);

    return;
}

static void ssv6020_ps_evt_handler(struct ssv_softc *sc, struct sk_buff *ps_evt)
{
    struct cfg_host_event *h_evt = (struct cfg_host_event *)ps_evt->data;
    struct ssv6xxx_ps_params *ps_params = (struct ssv6xxx_ps_params *)h_evt->dat;

    switch (ps_params->ops) {
        case SSV6XXX_PS_WAKEUP:
            printk("%s(): wakeup the host by sdio\n", __FUNCTION__);
            break;
        case SSV6XXX_PS_WAKEUP_FIN_ACK:
            complete(&sc->wakeup_done);
            printk("%s(): wakeup FIN ACK is received\n", __FUNCTION__);
            break;
        case SSV6XXX_PS_HOLD_ON3_ACK:
            complete(&sc->hold_on3);
            printk("%s(): holding ON3 ACK is received\n", __FUNCTION__);
            break;
        default:
            printk("%s(): unknown ps_params->ops = %d\n", __FUNCTION__, ps_params->ops);
            break;
    }
}

static void ssv6020_cmd_efuse(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    char *endp;
    struct ssv_softc *sc = sh->sc;
    struct ssv_efuse_tool_param *efuse_tool_param;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd;
    int i = 0, value1 = 0, value2 = 0;
    unsigned char mac[6];

    if (!strcmp(argv[1], "writepsk")) {
        if (argc == 3) {

            if (strlen(argv[2]) > 32) {
                snprintf_res(cmd_data, "write psk range 1 ~ 32\n");
                return;
            }

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_WRITE_PSK;
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                efuse_tool_param->psk_len = strlen(argv[2]);
                for (i= 0; i < efuse_tool_param->psk_len; i++)
                    efuse_tool_param->psk[i] = argv[2][i];
                HCI_SEND_CMD(sc->sh, skb);
                // show result
                snprintf_res(cmd_data, "write psk = %s, len %d\n", argv[2], strlen(argv[2]));
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readpsk")) {
        if (argc == 2) {
            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_PSK;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_PSK);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);
                // show result
                snprintf_res(cmd_data,"psk = %s, len %d\n", sc->efuse_psk, sc->efuse_psk_len);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readchipid")) {
        if (argc == 2) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_CHIPID;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_CHIPID);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);
                // show result
                snprintf_res(cmd_data,"chip_id = 0x%08x\n", sc->efuse_chip_id);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readmac")) {
        if (argc == 2) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_MAC;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_MAC);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"efuse mac = %02X:%02X:%02X:%02X:%02X:%02X\n",
                    sc->efuse_mac[0], sc->efuse_mac[1], sc->efuse_mac[2],
                    sc->efuse_mac[3], sc->efuse_mac[4], sc->efuse_mac[5]);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "writemac")) {
        if (argc == 4) {
            if (strlen(argv[2]) > 32) {
                snprintf_res(cmd_data,"psk len 1 ~ 32\n");
                return;
            }

            value1 = sscanf(argv[3], "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX",
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

            if (6 != value1) {
                snprintf_res(cmd_data,"write incorrect mac format\n");
                return;
            }

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_WRITE_MAC;
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                memcpy(efuse_tool_param->mac, mac, 6);
                efuse_tool_param->psk_len = strlen(argv[2]);
                for (i= 0; i < efuse_tool_param->psk_len; i++)
                    efuse_tool_param->psk[i] = argv[2][i];
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"write efuse mac = %02X:%02X:%02X:%02X:%02X:%02X\n",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readtxpower1")) {
        if (argc == 2) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_TX_POWER1;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_TX_POWER1);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"efuse txpower1 = %d\n", sc->efuse_txpower1);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "writetxpower1")) {
        if (argc == 5) {
            if (strlen(argv[2]) > 32) {
                snprintf_res(cmd_data,"psk len 1 ~ 32\n");
                return;
            }

            value1 = simple_strtoul(argv[3], &endp, 0);
            if ((value1 < 1) || (value1 > 15)) {
                snprintf_res(cmd_data,"txpower1 gain2000 range 1 ~ 15\n");
                return;
            }

            value2 = simple_strtoul(argv[4], &endp, 0);
            if ((value2 < 1) || (value2 > 15)) {
                snprintf_res(cmd_data,"txpower1 gain5200 range 1 ~ 15\n");
                return;
            }

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_WRITE_TX_POWER1;
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                efuse_tool_param->txpower1_gain2000 = value1;
                efuse_tool_param->txpower1_gain5200 = value2;
                efuse_tool_param->psk_len = strlen(argv[2]);
                for (i= 0; i < efuse_tool_param->psk_len; i++)
                    efuse_tool_param->psk[i] = argv[2][i];
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"write efuse txpower1 = %d\n", value1);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readxtal")) {
        if (argc == 2) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_XTAL;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_XTAL);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"efuse xtal = %d\n", sc->efuse_xtal);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "writextal")) {
        if (argc == 4) {
            if (strlen(argv[2]) > 32) {
                snprintf_res(cmd_data,"psk len 1 ~ 32\n");
                return;
            }

            value1 = simple_strtoul(argv[3], &endp, 0);
            if ((value1 < 0) || (value1 > 255)) {
                snprintf_res(cmd_data,"xtal range 0 ~ 255\n");
                return;
            }
            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_WRITE_XTAL;
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                efuse_tool_param->xtal = value1;
                efuse_tool_param->psk_len = strlen(argv[2]);
                for (i= 0; i < efuse_tool_param->psk_len; i++)
                    efuse_tool_param->psk[i] = argv[2][i];
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"write efuse xtal = %d\n", value1);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readrategainbn40")) {
        if (argc == 2) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_RATE_GAIN_B_N40;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_RATE_GAIN_B_N40);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"efuse rate gain b   = %d\n", sc->efuse_rate_gain_b);
                snprintf_res(cmd_data,"efuse rate gain n40 = %d\n", sc->efuse_rate_gain_n40);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "writerategainbn40")) {
        if (argc == 5) {
            if (strlen(argv[2]) > 32) {
                snprintf_res(cmd_data,"psk len 1 ~ 32\n");
                return;
            }
            value1 = simple_strtoul(argv[3], &endp, 0);
            if ((value1 < 1) || (value1 > 15)) {
                snprintf_res(cmd_data,"rate gain b range 1 ~ 15\n");
                return;
            }

            value2 = simple_strtoul(argv[4], &endp, 0);
            if ((value1 < 1) || (value1 > 15)) {
                snprintf_res(cmd_data,"rate gain n40 range 1 ~ 15\n");
                return;
            }

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_WRITE_RATE_GAIN_B_N40;
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                efuse_tool_param->rate_gain_b = value1;
                efuse_tool_param->rate_gain_n40 = value2;
                efuse_tool_param->psk_len = strlen(argv[2]);
                for (i= 0; i < efuse_tool_param->psk_len; i++)
                    efuse_tool_param->psk[i] = argv[2][i];
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"write efuse rate gain b   = %d\n", value1);
                snprintf_res(cmd_data,"write efuse rate gain n40 = %d\n", value2);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "checkitemfree")) {
        if (argc == 3) {

            value1 = simple_strtoul(argv[2], &endp, 0);
            if ((value1 < 0) || (value1 > 4)) {
                snprintf_res(cmd_data,"item range 0 ~ 4\n");
                snprintf_res(cmd_data,"0: macaddress, 1: freq offset, 2: txpower1, 3: rate gain, 4: ble power\n");
                return;
            }

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_CHECK_ITEM_FREE;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_CHECK_ITEM_FREE);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                efuse_tool_param->check_item_free = value1;
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"free count = %d\n", sc->efuse_free_count);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "readblepower")) {
        if (argc == 2) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_READ_BLE_POWER;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_EFUSE_OPS << 16) | (u16)SSV6XXX_EFUSE_CMD_READ_BLE_POWER);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"efuse ble gain = %d\n", sc->efuse_ble_gain);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else if (!strcmp(argv[1], "writeblepower")) {
        if (argc == 4) {
            if (strlen(argv[2]) > 32) {
                snprintf_res(cmd_data,"psk len 1 ~ 32\n");
                return;
            }

            value1 = simple_strtoul(argv[3], &endp, 0);
            if ((value1 < 1) || (value1 > 15)) {
                snprintf_res(cmd_data,"ble power range 1 ~ 15\n");
                return;
            }
            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_EFUSE_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_EFUSE_CMD_WRITE_BLE_POWER;
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_efuse_tool_param);

                efuse_tool_param = (struct ssv_efuse_tool_param *)host_cmd->un.dat8;
                memset(efuse_tool_param, 0x0, sizeof(struct ssv_efuse_tool_param));
                efuse_tool_param->ble_gain = value1;
                efuse_tool_param->psk_len = strlen(argv[2]);
                for (i= 0; i < efuse_tool_param->psk_len; i++)
                    efuse_tool_param->psk[i] = argv[2][i];
                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"write efuse ble power = %d\n", value1);
            }
        } else {
            snprintf_res(cmd_data,"Incorrect efuse command format\n");
        }
    } else {
        snprintf_res(cmd_data,"\n efuse [readchipid|readmac|writemac|readtxpower1|writetxpower1|readxtal|writextal|readrategainbn40|writerategainbn40|raadblepower|writeblepower|checkitemfree]\n");
    }

    return;
}

void ssv_attach_ssv6020_common(struct ssv_hal_ops *hal_ops)
{
    hal_ops->alloc_hw = ssv6020_alloc_hw;
    hal_ops->use_hw_encrypt = ssv6020_use_hw_encrypt;
    hal_ops->if_chk_mac2= ssv6020_if_chk_mac2;

    hal_ops->get_wsid = ssv6020_get_wsid;
    hal_ops->add_fw_wsid = ssv6020_add_fw_wsid;
    hal_ops->del_fw_wsid = ssv6020_del_fw_wsid;
    hal_ops->enable_fw_wsid = ssv6020_enable_fw_wsid;
    hal_ops->disable_fw_wsid = ssv6020_disable_fw_wsid;

    hal_ops->set_fw_hwwsid_sec_type = ssv6020_set_fw_hwwsid_sec_type;
    hal_ops->wep_use_hw_cipher = ssv6020_wep_use_hw_cipher;
    hal_ops->pairwise_wpa_use_hw_cipher = ssv6020_pairwise_wpa_use_hw_cipher;
    hal_ops->group_wpa_use_hw_cipher = ssv6020_group_wpa_use_hw_cipher;
    hal_ops->chk_if_support_hw_bssid = ssv6020_chk_if_support_hw_bssid;
    hal_ops->chk_dual_vif_chg_rx_flow = ssv6020_chk_dual_vif_chg_rx_flow;
    hal_ops->restore_rx_flow = ssv6020_restore_rx_flow;

    hal_ops->put_mic_space_for_hw_ccmp_encrypt = ssv6020_put_mic_space_for_hw_ccmp_encrypt;
    hal_ops->use_turismo_hw_encrpt = ssv6020_use_turismo_hw_encrypt;

    hal_ops->init_tx_cfg = ssv6020_init_tx_cfg;
    hal_ops->init_rx_cfg = ssv6020_init_rx_cfg;

    hal_ops->ampdu_rx_start = ssv6020_ampdu_rx_start;
    hal_ops->adj_config = ssv6020_adj_config;
    hal_ops->get_fw_name = ssv6020_get_fw_name;
    hal_ops->need_sw_cipher = ssv6020_need_sw_cipher;
    hal_ops->send_tx_poll_cmd = ssv6020_send_tx_poll_cmd;
    hal_ops->cmd_hwq_limit = ssv6020_cmd_set_hwq_limit;
    hal_ops->flash_read_all_map = ssv6020_flash_read_all_map;
    hal_ops->ps_evt_handler = ssv6020_ps_evt_handler;
    hal_ops->cmd_efuse = ssv6020_cmd_efuse;
}



#endif
