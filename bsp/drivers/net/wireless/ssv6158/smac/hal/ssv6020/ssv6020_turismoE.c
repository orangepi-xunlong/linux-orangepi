/*
 * Copyright (c) 2015 iComm Semiconductor Ltd.
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
#include <linux/version.h>
#if ((defined SSV_SUPPORT_SSV6020))
#include <linux/nl80211.h>
#include <ssv6200.h>
#include "ssv6020/ssv6020_reg.h"
#include "ssv6020/ssv6020_aux.h"
#include <smac/dev.h>
#include <smac/efuse.h>
#include <smac/ssv_skb.h>
#include <hal.h>
#include <ssvdevice/ssv_cmd.h>
#include "ssv6020_mac.h"
#include "ssv6020_priv.h"
#include "ssv6020_priv_normal.h"
#include "ssv_desc.h"
#include "turismo_common.h"
#include "turismo_common.c"

#include <ssv6xxx_common.h>
#include <linux_80211.h>


struct st_rf_table def_rf_table = {
                    /* rt_config */
                    { { 9,  9,  9,  9,  9,  9,  9} , 0x98, 0x98, 7, 9, 10, 7, 7, 7, 7, 7, 1},
                    /* ht_config */
                    { {11, 11, 11, 11, 11, 11, 11} , 0x98, 0x98, 7, 9, 10, 7, 7, 7, 7, 7, 1},
                    /* lt_config */
                    { { 8,  8,  8,  8,  8,  8,  8} , 0xa5, 0xa5, 7, 9, 10, 7, 7, 7, 7, 7, 1},
                    /* rf_gain */
                    4,
                    /* rate_gain_b */
                    4,
                    /* rate_config_g */
                    {15, 13, 11, 9},
                    /* rate_config_20n */
                    {15, 13, 11, 9},
                    /* rate_config_40n */
                    {15, 13, 11, 9},
                    /* low_boundarty */
                    10,
                    /* high_boundary */
                    75,
                    /* boot flag */
                    EN_FIRST_BOOT,
                    /* work mode */
                    EN_WORK_NOMAL,
                    /* rt_5g_config */
                    { 13, 10, 10, 9, 0x9264924a, 0x96dbb6cc },
                    /* ht_5g_config */
                    { 13, 10, 10, 9, 0x9264924a, 0x96dbb6cc },
                    /* lt_5g_config */
                    { 13, 10, 10, 9, 0x9264924a, 0x96dbb6cc },
                    /* band_f0_threshold */
                    0x141E,
                    /* band_f1_threshold */
                    0x157C,
                    /* band_f2_threshold */
                    0x1644,
                    /* Signature */
                    RF_API_SIGNATURE,
                    /* Structure version */
                    RF_API_TABLE_VERSION,
                    /* DCDC flag */
                    1,
                    /* external pa settings */
                    { 0, 0, 1, 0xd, 8, 8, 0xc, 0},
                    /* RTC 32K XTAL setting */
                    0,
                    /* Tx Lowpower setting */
                    1,
                    /* ble_rt_config */
                    {{8, 8}},
                    /* ble_ht_config */
                    {{8, 8}},
                    /* ble_lt_config */
                    {{8, 8}},
                    /*ble DTM mode*/
                     0,
                    /*external padpd flag*/
                     0,
                    /*external padpd setting*/
                    {0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
                     0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
                     0x200, 0x200, 0x200, 0x200, 0x200, 0x200,
                     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                     0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                     0x0, 0x0},
                    /* ht2_config */
                    { {13, 13, 13, 13, 13, 13, 13} , 0xb4, 0xb4, 7, 9, 10, 7, 7, 7, 7, 7, 1},
                    /* lt2_config */
                    { { 7,  7,  7,  7,  7,  7,  7} , 0xa5, 0xa5, 7, 9, 10, 7, 7, 7, 7, 7, 1},
                    /*ble_ht2_config*/
                    {{8, 8}},
                    /*ble_lt2_config*/
                    {{8, 8}},
                    /* low2_boundarty */
                    -5,
                    /* high2_boundary */
                    90,
};

static void ssv6020_turismoE_load_phy_table(ssv_cabrio_reg **phy_table)
{
    return;
}

static u32 ssv6020_turismoE_get_phy_table_size(struct ssv_hw *sh)
{
    return 0;
}

static void ssv6020_turismoE_load_rf_table(ssv_cabrio_reg **rf_table)
{
    return;
}

static u32 ssv6020_turismoE_get_rf_table_size(struct ssv_hw *sh)
{
    return 0;
}

static void ssv6020_turismoE_init_PLL(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return;
    }

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_INIT_PLL;
    host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_INIT_PLL);
    host_cmd->len = HOST_CMD_HDR_LEN;

    HCI_SEND_CMD(sc->sh, skb);
}

static int ssv6020_turismoE_set_channel(struct ssv_softc *sc, struct ieee80211_channel *chan,
        enum nl80211_channel_type channel_type, bool offchan)
{
#if 0
    u16 ch = chan->hw_value;
    struct ssv_hw *sh = sc->sh;
#endif
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;
    struct ssv_rf_chan *ptr = NULL;
    int ret = 0;

#ifdef	CONFIG_ENABLE_ACS_FUNC
	HAL_EDCA_UPDATE_SURVEY(sc->sh);
	HAL_EDCA_ENABLE(sc->sh, false);
#endif

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_chan));
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return -1;
    }

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_chan));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_CHAN;
    host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_CHAN);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_chan);

    ptr = (struct ssv_rf_chan *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_rf_chan));
    ptr->chan = chan->hw_value;
    ptr->chan_type = channel_type;
    ptr->off_chan = offchan;

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0)) && defined(CONFIG_HW_SCAN))
    if ((false == sc->hw_scan_done) || ((true == sc->bScanning) && (true == sc->hw_scan_start))) {
        ptr->scan = 1;
        //IEEE80211_CHAN_NO_IR: do not initiate radiation, this includes sending probe requests or beaconing.
        //IEEE80211_CHAN_RADAR: Radar detection is required on this channel.
        if (sc->scan_is_passive || (chan->flags & IEEE80211_CHAN_NO_IR)) {
            ptr->passive_chan = 1;
        }
        memcpy(&ptr->scan_param, &sc->scan_param, sizeof(struct ssv_scan_param));
        if (chan->hw_value > 14) {
            ptr->scan_param.no_cck = true;
            memcpy(ptr->scan_param.ie, sc->scan_ie_band5g, sc->scan_ie_len_band5g);
            ptr->scan_param.ie_len = sc->scan_ie_len_band5g;
        } else {
            memcpy(ptr->scan_param.ie, sc->scan_ie_band2g, sc->scan_ie_len_band2g);
            ptr->scan_param.ie_len = sc->scan_ie_len_band2g;
        }
    }
#endif
    printk("chan change ch %d, type %d, off_chan %d\n", ptr->chan, ptr->chan_type, ptr->off_chan);
    ret = HCI_SEND_CMD(sc->sh, skb);

    // update tx power
    sc->dpd.pwr_mode  = NORMAL_PWR;
#ifdef CONFIG_ENABLE_ACS_FUNC
	sc->survey_cur_chan = chan->hw_value;
	HAL_EDCA_ENABLE(sc->sh, true);
#endif
    return 0;
}

#if 0
static void ssv6020_turismoE_init_cali(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;
    struct ssv_rf_cali_param *cali_param = NULL;

    skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_cali_param)));
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return;
    }

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_cali_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_INIT_CALI;
    host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_INIT_CALI);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_cali_param);
    cali_param = (struct ssv_rf_cali_param *)host_cmd->dat8;
    memset(cali_param, 0, sizeof(struct ssv_rf_cali_param));
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ)
        cali_param->support_5g = 1;
    HCI_SEND_CMD(sc->sh, skb);
}
#endif

static u32 ssv6020_turismoE_update_xtal(struct ssv_hw *sh)
{
    u32 xtal = 0;

    switch (sh->cfg.crystal_type) {
        case SSV6XXX_IQK_CFG_XTAL_16M:
            xtal = 16;
            break;
        case SSV6XXX_IQK_CFG_XTAL_24M:
            xtal = 24;
            break;
        case SSV6XXX_IQK_CFG_XTAL_26M:
            xtal = 26;
            break;
        case SSV6XXX_IQK_CFG_XTAL_40M:
            xtal = 40;
            break;
        case SSV6XXX_IQK_CFG_XTAL_12M:
            xtal = 12;
            break;
        case SSV6XXX_IQK_CFG_XTAL_20M:
            xtal = 20;
            break;
        case SSV6XXX_IQK_CFG_XTAL_25M:
            xtal = 25;
            break;
        case SSV6XXX_IQK_CFG_XTAL_32M:
            xtal = 32;
            break;
        default:
            xtal = 40;
            printk("Please redefine xtal_clock(wifi.cfg)!!\n");
            WARN_ON(1);
            break;
    }

    return xtal;
}

static int ssv6020_turismoE_set_pll_phy_rf(struct ssv_hw *sh ,ssv_cabrio_reg *rf_tbl, ssv_cabrio_reg *phy_tbl)
{
    struct ssv_softc *sc = sh->sc;
    int  ret = 0;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;
    struct ssv_rf_cali *ptr = NULL;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_cali));
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return -1;
    }

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_cali));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_INIT_PLL_PHY_RF;
    host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_INIT_PLL_PHY_RF);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_cali);

    ptr = (struct ssv_rf_cali *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_rf_cali));
    // support 5g
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ)
        ptr->support_5g = 1;
    // tempature
    if (sh->cfg.disable_fw_thermal == 1) {
        ptr->thermal = 0;
    } else {
        ptr->thermal = 1;
    }

    ptr->disable_greentx = (true == sh->cfg.disable_greentx) ? 1 : 0;
    ptr->disable_cci = (true == sh->cfg.disable_cci) ? 1 : 0;
    ptr->xtal = ssv6020_turismoE_update_xtal(sh);
    ptr->bus_clk = 160; // 6020 use 160 bus clock

    // update rf table
    memcpy(&ptr->rf_table, &sh->rf_table, sizeof(struct st_rf_table));
    // dcdc
    if (sh->cfg.volt_regulator == SSV6XXX_VOLT_LDO_CONVERT)
        ptr->rf_table.dcdc_flag = 0;
    else
        ptr->rf_table.dcdc_flag = 1;



    ret = HCI_SEND_CMD(sc->sh, skb);

    return ret;
}

static bool ssv6020_turismoE_set_rf_enable(struct ssv_hw *sh, bool val)
{
    struct ssv_softc *sc = sh->sc;
    bool  ret = true;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return false;
    }

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    if (val) {
        host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_ENABLE;
        host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_ENABLE);
    } else {
        host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_DISABLE;
        host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_DISABLE);
    }
    host_cmd->len = HOST_CMD_HDR_LEN;
    ret = HCI_SEND_CMD(sc->sh, skb);

    return true;
}

static bool ssv6020_turismoE_dump_phy_reg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    int  ret = 0;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return false;
    }

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_DUMP_PHY_REG;
    host_cmd->len = HOST_CMD_HDR_LEN;
    ret = HCI_SEND_CMD(sc->sh, skb);

    return true;
}

static bool ssv6020_turismoE_dump_rf_reg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    int  ret = 0;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
    if (skb == NULL) {
        printk("%s(): Fail to alloc cmd buffer.\n", __FUNCTION__);
        return false;
    }

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_DUMP_RF_REG;
    host_cmd->len = HOST_CMD_HDR_LEN;
    ret = HCI_SEND_CMD(sc->sh, skb);

    return true;
}

static bool ssv6020_turismoE_support_iqk_cmd(struct ssv_hw *sh)
{
    return false;
}

static void ssv6020_cmd_turismoE_txgen(struct ssv_hw *sh, u8 drate)
{
    struct sk_buff *skb = NULL;
    int    len = (int) sizeof(ssv6020_pkt1614);
    unsigned char *data = NULL;
    struct ssv6020_tx_desc *tx_desc;
    struct ssv_softc *sc = sh->sc;

    skb = ssv_skb_alloc(sc, len);
    if (!skb) {
        return;
    }

    data = skb_put(skb, len);
    memcpy(data, ssv6020_pkt1614, len);
    tx_desc = (struct ssv6020_tx_desc *)data;
    tx_desc->drate_idx0 = drate;

    SET_RG_TXD_SEL(0);  // set tx gen path disable
    SET_RG_TX_START(0); // disable trigger tx!!
    HCI_SEND_CMD(sc->sh, skb);

}

static void ssv6020_cmd_turismoE_rfphy_ops(struct ssv_softc *sc,  ssv_rf_phy_ops PHY_OPS,struct ssv_rf_tool_param *param)
{
    struct sk_buff *skb = NULL;
	struct cfg_host_cmd *host_rf_cmd;
	struct ssv_rf_tool_param *rf_tool_param;
	u32  ssv_skb_lenth = HOST_CMD_HDR_LEN;

	if (NULL != param) {
	    ssv_skb_lenth = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);
	}

    skb = ssv_skb_alloc(sc, ssv_skb_lenth);
    if (skb != NULL) {
        skb_put(skb, ssv_skb_lenth);
	    host_rf_cmd = (struct cfg_host_cmd *)skb->data;
	    memset(host_rf_cmd, 0x0, sizeof(struct cfg_host_cmd));
	    host_rf_cmd->c_type = HOST_CMD;
	    host_rf_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
	    host_rf_cmd->sub_h_cmd = (u32)PHY_OPS;
	    host_rf_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)PHY_OPS);
	    host_rf_cmd->len = ssv_skb_lenth;
		if (NULL != param) {
			rf_tool_param = (struct ssv_rf_tool_param *)host_rf_cmd->un.dat8;
			memcpy(rf_tool_param, param, sizeof(struct ssv_rf_tool_param));
		}

		HCI_SEND_CMD(sc->sh, skb);
    }
}

static void ssv6020_cmd_turismoE_rfphy_ops_start(struct ssv_softc *sc, u32 regval)
{
	struct ssv_rf_tool_param rf_param;;
	memset(&rf_param ,0 ,sizeof(struct ssv_rf_tool_param));
	rf_param.count = 0xffffffff;
	rf_param.interval = regval;
	ssv6020_cmd_turismoE_rfphy_ops(sc, SSV6XXX_RFPHY_CMD_RF_TOOL_TX, &rf_param);
}

static void ssv6020_cmd_turismoE_rfphy_ops_stop(struct ssv_softc *sc)
{
	ssv6020_cmd_turismoE_rfphy_ops(sc, SSV6XXX_RFPHY_CMD_RF_TOOL_STOP, NULL);
}


static void ssv6020_cmd_turismoE_rf(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    u32 regval = 0;
    char *endp;
    struct ssv_softc *sc = sh->sc;
    int ch = 0;
    enum nl80211_channel_type ch_type;
    struct ssv_rf_tool_param *rf_tool_param;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd;

    unsigned char rate_tbl[] = {
        0x00,0x01,0x02,0x03,                        // B mode long preamble [0~3]
        0x00,0x12,0x13,                             // B mode short preamble [4~6], no 2M short preamble
        0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,    // G mode [7~14]
        0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,    // N mode HT20 long GI mixed format [15~22]
        0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,    // N mode HT20 short GI mixed format  [23~30]
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,    // N mode HT40 long GI mixed format [31~38]
        0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,    // N mode HT40 short GI mixed format  [39~46]
    };
#define SIZE_RATE_TBL   (sizeof(rate_tbl) / sizeof((rate_tbl)[0]))

    if (argc < 2)
        goto out;

    if (!strcmp(argv[1], "tx")) {

        if (argc == 3)
            regval = simple_strtoul(argv[2], &endp, 0);
        else
            regval = 0;

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_TX;
            host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_TX);
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

            rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
            memset(rf_tool_param, 0x0, sizeof(struct ssv_rf_tool_param));
            rf_tool_param->count = 0xffffffff;
            rf_tool_param->interval = regval;

            HCI_SEND_CMD(sc->sh, skb);
            sc->sc_flags |= SC_OP_BLOCK_CNTL;
            sc->sc_flags |= SC_OP_CHAN_FIXED; // fixed channel
            snprintf_res(cmd_data,"\n   RF TX\n");
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

        return;

    } else if (!strcmp(argv[1], "rx")) {
        sc->sc_flags |= SC_OP_BLOCK_CNTL;
        sc->sc_flags |= SC_OP_CHAN_FIXED; // fixed channel
        snprintf_res(cmd_data,"\n   RF RX\n");
        return ;

    } else if (!strcmp(argv[1], "rxreset")) {
        if (argc == 2) {
            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_RX_RESET;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_RX_RESET);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0x0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->rx_reset = 1;
                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"\n   RF RX Reset\n");
            } else {
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
            }
        }
        return;

    } else if(!strcmp(argv[1], "stop")) {

        skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
        if (skb != NULL) {
            skb_put(skb, HOST_CMD_HDR_LEN);
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_STOP;
            host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_STOP);
            host_cmd->len = HOST_CMD_HDR_LEN;
            HCI_SEND_CMD(sc->sh, skb);
            sc->sc_flags &= ~SC_OP_BLOCK_CNTL;
            sc->sc_flags &= ~SC_OP_CHAN_FIXED;
            snprintf_res(cmd_data,"\n   RF STOP\n");
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        return;

    } else if (!strcmp(argv[1], "ch")) {

        if ((argc == 3) || (argc == 4)) {
            ch = simple_strtoul(argv[2], &endp, 0);
            ch_type = NL80211_CHAN_HT20;

            if (argc == 4) {
                if (!strcmp(argv[3], "bw40")) {
                    if ((ch == 3) || (ch == 4) || (ch == 5) || (ch == 6) ||
                        (ch == 7) || (ch == 8) || (ch == 9) || (ch == 10) ||
                        (ch == 11)) {

                        ch_type = NL80211_CHAN_HT40PLUS;
                        ch = ch - 2; // find center chan
                    }
                }
            }

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_CH;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_CH);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0x0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->ch = ch;
                rf_tool_param->ch_type = ch_type;
                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"\n RF ch %d, ch_type %d\n", ch, (int)ch_type);
            } else {
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
            }
        }
        return;

    } else if(!strcmp(argv[1], "rate")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            if ((regval != 4) && (regval < SIZE_RATE_TBL)) {
                skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                if (skb != NULL) {
                    skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                    host_cmd = (struct cfg_host_cmd *)skb->data;
                    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                    host_cmd->c_type = HOST_CMD;
                    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                    host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_RATE;
                    host_cmd->blocking_seq_no =
                        (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_RATE);
                    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                    rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                    memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                    rf_tool_param->rate = regval;

                    HCI_SEND_CMD(sc->sh, skb);
                    snprintf_res(cmd_data,"\n   rf rate index %d\n", regval);
                } else{
                    snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
				}

            } else {
                snprintf_res(cmd_data,"Not support rf rate index %d\n", regval);
            }
            return;

        } else {
            snprintf_res(cmd_data,"\n\t Incorrect rf rate set format\n");
            return;
        }

    } else if(!strcmp(argv[1], "disablethermal")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            regval = (0 == regval) ? 0 : 1;

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_THERMAL;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_THERMAL);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->disable_thermal = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"\n   rf thermal %d\n", regval);
            } else {
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
		    }

            return;

        } else {
            snprintf_res(cmd_data,"\n\t Incorrect rf rate set format\n");
            return;
        }

    } else if(!strcmp(argv[1], "freq")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_FREQ;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_FREQ);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->freq = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set cbanki/cbanko to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf freq [value]\n");
        }
        return;

    } else if(!strcmp(argv[1], "rfreq")){
        if (argc == 2){
            snprintf_res(cmd_data,"Get freq 0x%x/0x%x\n", GET_RG_XO_CBANKI_7_0, GET_RG_XO_CBANKO_7_0);
        } else {
            snprintf_res(cmd_data,"./cli rf rfreq\n");
        }
        return;

    } else if(!strcmp(argv[1], "dcdc")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            regval = (regval == 0) ? 0 : 1;

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_DCDC;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_DCDC);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->dcdc = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set dcdc to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf dcdc [value]\n");
        }
        return;

    } else if(!strcmp(argv[1], "dacgain")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            if (regval > 15)
                regval = 15;

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_DACGAIN;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_DACGAIN);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->dacgain = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set dgcgain to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf dacgain [value]\n");
        }
        return;

    } else if(!strcmp(argv[1], "ratebgain")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            if (regval > 15)
                regval = 15;

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_RATEBGAIN;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_RATEBGAIN);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->ratebgain = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set rate b gain to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf ratebgain [value]\n");
        }
        return;

    } else if(!strcmp(argv[1], "padpd")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            regval = (regval == 0) ? 0 : 3;

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_PADPD;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_PADPD);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->padpd = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set padpd to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf padpd [value]\n");
        }
        return;
    } else if(!strcmp(argv[1], "wfble")){
        if (argc == 3) {
        #define RF_TOOL_MODE_WIFI       1
        #define RF_TOOL_MODE_BLE        0
            regval = simple_strtoul(argv[2], &endp, 0);

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_WFBLE;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_WFBLE);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->wfble = (regval == 0) ? RF_TOOL_MODE_WIFI : RF_TOOL_MODE_BLE;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set wfble to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf wfble [0|1]\n");
        }
        return;

    } else if(!strcmp(argv[1], "count")){
        if  (argc == 3) {

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TOOL_RX_MIB;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TOOL_RX_MIB);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0x0, sizeof(struct ssv_rf_tool_param));
                if ((!strcmp(argv[2], "0")) || (!strcmp(argv[2], "2") ))
                    rf_tool_param->rx_mib = 1;
                else
                    rf_tool_param->rx_mib = 0;

                HCI_SEND_CMD(sc->sh, skb);

                // show result
                snprintf_res(cmd_data,"count = %d\n", sc->rf_tool_rx_count);
                snprintf_res(cmd_data,"err = %d\n", sc->rf_tool_rx_err_count);
                return;
            } else {
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
                return;
            }
        }
        snprintf_res(cmd_data,"\n\t./cli rf count 0|1\n");
        return;

    } else if(!strcmp(argv[1], "tonegen")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);

            skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
            if (skb != NULL) {
                skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param)));
                host_cmd = (struct cfg_host_cmd *)skb->data;
                memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
                host_cmd->c_type = HOST_CMD;
                host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
                host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RF_TONE_GEN;
                host_cmd->blocking_seq_no =
                    (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RF_TONE_GEN);
                host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rf_tool_param);

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->un.dat8;
                memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                rf_tool_param->tone_gen = regval;

                HCI_SEND_CMD(sc->sh, skb);
                snprintf_res(cmd_data,"Set tone gen to 0x%x\n", regval);
            } else
                snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
        } else{
            snprintf_res(cmd_data,"./cli rf tonegen [value]\n");
        }
        return;

    } else if(!strcmp(argv[1], "phy_txgen")) {

        ssv6020_cmd_turismoE_rfphy_ops_start(sc, 1);
        sc->sc_flags |= SC_OP_BLOCK_CNTL;
        sc->sc_flags |= SC_OP_CHAN_FIXED; // fixed channel
        snprintf_res(cmd_data,"\n   RF TX\n");
        return;
	}else if(!strcmp(argv[1], "block")){
		sc->sc_flags |= SC_OP_BLOCK_CNTL;
        sc->sc_flags |= SC_OP_CHAN_FIXED;
        snprintf_res(cmd_data,"\n\t block control form system\n");
        return;
    } else if(!strcmp(argv[1], "unblock")){
		sc->sc_flags &= ~SC_OP_BLOCK_CNTL;
        sc->sc_flags &= ~SC_OP_CHAN_FIXED;
        ssv6020_cmd_turismoE_rfphy_ops_stop(sc);
        snprintf_res(cmd_data,"\n\t unblock control form system\n");
        return;
	}else {

        snprintf_res(cmd_data,
                "\n\t./cli rf phy_txgen|block|unblock|count|freq|rfreq|rate|ch|thermal|stop|rxreset|rx|tx\n");
        return;
    }
out:

    snprintf_res(cmd_data,"\n\t Current RF tool settings: ch %d\n", ch);

    if (sc->sc_flags && SC_OP_BLOCK_CNTL) {
        snprintf_res(cmd_data,"\t system control is blocked\n");
    } else {
        snprintf_res(cmd_data,"\t WARNING system control is not blocked\n");
    }
}

static void ssv6020_cmd_turismoE_rfble(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    u32 value = 0;
    char *endp;
    struct ssv_softc *sc = sh->sc;
    struct ssv_rfble_tool_param *rfble_tool_param;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd;

    if (argc < 2) {
        snprintf_res(cmd_data,"\n rfble [tx|rate|ch|stop|rx|rxreset|count]\n");
        return;
    }

    if (!strcmp(argv[1], "tx")) {

        if (argc == 3)
            value = simple_strtoul(argv[2], &endp, 0);
        else
            value = 0;

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_START;
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));
            rfble_tool_param->count = value;

            HCI_SEND_CMD(sc->sh, skb);
            sc->sc_flags |= SC_OP_BLOCK_CNTL;
            sc->sc_flags |= SC_OP_CHAN_FIXED; // fixed channel
            snprintf_res(cmd_data,"\n   RFBLE TX\n");
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "rate")) {

        if (argc != 3) {
            snprintf_res(cmd_data,"\n rfble rate value\n");
            return;
        }

        value = simple_strtoul(argv[2], &endp, 0);
        if ((value < 0) || (value > 7)) {
            snprintf_res(cmd_data,"\n rate range 0 ~ 7\n");
            return;
        }

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_RATE;
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));
            rfble_tool_param->rate = value;

            HCI_SEND_CMD(sc->sh, skb);
            snprintf_res(cmd_data,"\n   RFBLE rate %d\n", value);
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "dacgain")) {

        if (argc != 3) {
            snprintf_res(cmd_data,"\n rfble dacgain value\n");
            return;
        }

        value = simple_strtoul(argv[2], &endp, 0);
        if ((value < 1) || (value > 15)) {
            snprintf_res(cmd_data,"\n dacgain range 1 ~ 15\n");
            return;
        }

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_DACGAIN;
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));
            rfble_tool_param->dacgain = value;

            HCI_SEND_CMD(sc->sh, skb);
            snprintf_res(cmd_data,"\n   RFBLE dacgain %d\n", value);
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "ch")) {

        if (argc != 3) {
            snprintf_res(cmd_data,"\n rfble ch value\n");
            return;
        }

        value = simple_strtoul(argv[2], &endp, 0);
        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_CH;
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));
            rfble_tool_param->chan = value;

            HCI_SEND_CMD(sc->sh, skb);
            snprintf_res(cmd_data,"\n   RFBLE chan %d\n", value);
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "stop")) {

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_STOP;
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));

            HCI_SEND_CMD(sc->sh, skb);
            snprintf_res(cmd_data,"\n   RFBLE stop\n");
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "rxreset")) {

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_RESET;
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));

            HCI_SEND_CMD(sc->sh, skb);
            snprintf_res(cmd_data,"\n   RFBLE reset\n");
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "count")) {

        skb = ssv_skb_alloc(sc, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
        if (skb != NULL) {
            skb_put(skb, (HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param)));
            host_cmd = (struct cfg_host_cmd *)skb->data;
            memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
            host_cmd->c_type = HOST_CMD;
            host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_RFPHY_OPS;
            host_cmd->sub_h_cmd = (u32)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_COUNT;
            host_cmd->blocking_seq_no = (((u16)SSV6XXX_HOST_CMD_RFPHY_OPS << 16) | (u16)SSV6XXX_RFPHY_CMD_RFBLE_TOOL_COUNT);
            host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_rfble_tool_param);

            rfble_tool_param = (struct ssv_rfble_tool_param *)host_cmd->un.dat8;
            memset(rfble_tool_param, 0x0, sizeof(struct ssv_rfble_tool_param));

            HCI_SEND_CMD(sc->sh, skb);
            // show result
            snprintf_res(cmd_data,"count = %d\n", sc->rfble_tool_rx_count);
            snprintf_res(cmd_data,"err = %d\n", sc->rfble_tool_rx_err_count);
        } else
            snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");

    } else if (!strcmp(argv[1], "rx")) {
        sc->sc_flags |= SC_OP_BLOCK_CNTL;
        sc->sc_flags |= SC_OP_CHAN_FIXED; // fixed channel
        snprintf_res(cmd_data,"\n   RFBLE RX\n");

    } else {
        snprintf_res(cmd_data,"\n rfble [tx|rate|ch|stop|rx|rxreset|count]\n");
    }

    return;
}

static void ssv6020_turismoE_chg_xtal_freq_offset(struct ssv_hw *sh)
{
    if (sh->flash_config.exist) {
        // [15:8] xi, [7:0] xo
        sh->rf_table.low_boundary = sh->flash_config.xtal_offset_low_boundary;
        sh->rf_table.high_boundary = sh->flash_config.xtal_offset_high_boundary;
        sh->rf_table.low_2_boundary = sh->flash_config.xtal_offset_low_boundary;
        sh->rf_table.high_2_boundary = sh->flash_config.xtal_offset_high_boundary;
        sh->rf_table.rt_config.freq_xi = ((sh->flash_config.rt_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.rt_config.freq_xo = ((sh->flash_config.rt_config.xtal_offset & 0x00ff) >> 0);
        sh->rf_table.lt_config.freq_xi = ((sh->flash_config.lt_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.lt_config.freq_xo = ((sh->flash_config.lt_config.xtal_offset & 0x00ff) >> 0);
        sh->rf_table.lt_2_config.freq_xi = ((sh->flash_config.lt_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.lt_2_config.freq_xo = ((sh->flash_config.lt_config.xtal_offset & 0x00ff) >> 0);
        sh->rf_table.ht_config.freq_xi = ((sh->flash_config.ht_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.ht_config.freq_xo = ((sh->flash_config.ht_config.xtal_offset & 0x00ff) >> 0);
        sh->rf_table.ht_2_config.freq_xi = ((sh->flash_config.ht_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.ht_2_config.freq_xo = ((sh->flash_config.ht_config.xtal_offset & 0x00ff) >> 0);
    } else if (sh->efuse_bitmap & BIT(EFUSE_CRYSTAL_FREQUENCY_OFFSET)) {
        sh->rf_table.rt_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.rt_config.freq_xo = sh->cfg.crystal_frequency_offset;
        sh->rf_table.lt_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.lt_config.freq_xo = sh->cfg.crystal_frequency_offset;
        sh->rf_table.lt_2_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.lt_2_config.freq_xo = sh->cfg.crystal_frequency_offset;
        sh->rf_table.ht_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.ht_config.freq_xo = sh->cfg.crystal_frequency_offset;
        sh->rf_table.ht_2_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.ht_2_config.freq_xo = sh->cfg.crystal_frequency_offset;
        printk("freq xi/xo use value %d/%d\n", sh->rf_table.rt_config.freq_xi, sh->rf_table.rt_config.freq_xo);
    } else {
        printk("freq xi/xo use default value %d/%d\n", sh->rf_table.rt_config.freq_xi, sh->rf_table.rt_config.freq_xo);
    }
}

static void ssv6020_turismoE_chg_cck_bbscale(struct ssv_hw *sh, int cck)
{
    sh->rf_table.rate_gain_b = (u8)cck;
}

static void ssv6020_turismoE_chg_legacy_bbscale(struct ssv_hw *sh, int bpsk, int qpsk, int qam16, int qam64)
{
    sh->rf_table.rate_config_g.rate1 = bpsk;
    sh->rf_table.rate_config_g.rate2 = qpsk;
    sh->rf_table.rate_config_g.rate3 = qam16;
    sh->rf_table.rate_config_g.rate4 = qam64;
}

static void ssv6020_turismoE_chg_ht20_bbscale(struct ssv_hw *sh, int bpsk, int qpsk, int qam16, int qam64)
{
    sh->rf_table.rate_config_20n.rate1 = bpsk;
    sh->rf_table.rate_config_20n.rate2 = qpsk;
    sh->rf_table.rate_config_20n.rate3 = qam16;
    sh->rf_table.rate_config_20n.rate4 = qam64;
}

static void ssv6020_turismoE_chg_ht40_bbscale(struct ssv_hw *sh, int bpsk, int qpsk, int qam16, int qam64)
{
    sh->rf_table.rate_config_40n.rate1 = bpsk;
    sh->rf_table.rate_config_40n.rate2 = qpsk;
    sh->rf_table.rate_config_40n.rate3 = qam16;
    sh->rf_table.rate_config_40n.rate4 = qam64;
}

static void ssv6020_turismoE_chg_bbscale(struct ssv_hw *sh)
{
    int value = 0;

#define MAX_OFDM_RATE_GAIN_INDEX                 (9)
#define OFDM_RATE_GAIN_64QAM_OFFSET              (0)
#define OFDM_RATE_GAIN_16QAM_OFFSET              (2)
#define OFDM_RATE_GAIN_QPSK_OFFSET               (4)
#define OFDM_RATE_GAIN_BPSK_OFFSET               (6)

#define EFUSE_RATE_GAIN_MASK_CCK                 (0x01)
#define EFUSE_RATE_GAIN_MASK_LEGACY              (0x02)
#define EFUSE_RATE_GAIN_MASK_HT20                (0x04)
#define EFUSE_RATE_GAIN_MASK_HT40                (0x08)

#define EFUSE_RATE_TBL_MASK_B_RATE               (0xf0)
#define EFUSE_RATE_TBL_SFT_B_RATE                (4)
#define EFUSE_RATE_TBL_MASK_LEGACY_RATE          (0xf0)
#define EFUSE_RATE_TBL_SFT_LEGACY_RATE           (4)
#define EFUSE_RATE_TBL_MASK_HT20_RATE            (0x0f)
#define EFUSE_RATE_TBL_SFT_HT20_RATE             (0)
#define EFUSE_RATE_TBL_MASK_HT40_RATE            (0x0f)
#define EFUSE_RATE_TBL_SFT_HT40_RATE             (0)

    // set b rate gain
    if (sh->flash_config.exist) {
        ssv6020_turismoE_chg_cck_bbscale(sh, sh->flash_config.rate_delta[0]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_2)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_CCK)) {

            value = ((sh->cfg.rate_table_2 & EFUSE_RATE_TBL_MASK_B_RATE) >> EFUSE_RATE_TBL_SFT_B_RATE);
            if (sh->cfg.rf_b_rate_offset != 0)
            {
                if (((value + sh->cfg.rf_b_rate_offset) > 0) && ((value + sh->cfg.rf_b_rate_offset) <= 15 ))
                {
                    value += sh->cfg.rf_b_rate_offset;
                    printk("rate gain b gain offset %d, value %d\n", sh->cfg.rf_b_rate_offset, value);
                }
            }

            ssv6020_turismoE_chg_cck_bbscale(sh, value);
            printk("rate gain b use value %d\n", value);
        }
        else
        {
            if (sh->cfg.rf_b_rate_offset != 0)
            {
                value = sh->rf_table.rate_gain_b;
                if (((value + sh->cfg.rf_b_rate_offset) > 0) && ((value + sh->cfg.rf_b_rate_offset) <= 15 ))
                {
                    value += sh->cfg.rf_b_rate_offset;
                    printk("rate gain b gain offset %d, value %d\n", sh->cfg.rf_b_rate_offset, value);
                    ssv6020_turismoE_chg_cck_bbscale(sh, value);
                }
            }

            printk("rate gain b use default value\n");
        }
    }

    // set legacy rate gain
    if (sh->flash_config.exist) {
        ssv6020_turismoE_chg_legacy_bbscale(sh,
                sh->flash_config.rate_delta[1],
                sh->flash_config.rate_delta[2],
                sh->flash_config.rate_delta[3],
                sh->flash_config.rate_delta[4]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_1)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_LEGACY)) {

            value = ((sh->cfg.rate_table_1 & EFUSE_RATE_TBL_MASK_LEGACY_RATE) >> EFUSE_RATE_TBL_SFT_LEGACY_RATE);
            if (sh->cfg.rf_g_rate_offset != 0)
            {
                if (((value + sh->cfg.rf_g_rate_offset) > 0) && ((value + sh->cfg.rf_g_rate_offset) <= MAX_OFDM_RATE_GAIN_INDEX ))
                {
                    value += sh->cfg.rf_g_rate_offset;
                    printk("rate gain g gain offset %d, value %d\n", sh->cfg.rf_g_rate_offset, value);
                }
            }

            if (value>0 && value <= MAX_OFDM_RATE_GAIN_INDEX) {
                ssv6020_turismoE_chg_legacy_bbscale(sh, value+OFDM_RATE_GAIN_BPSK_OFFSET,
                                                    value+OFDM_RATE_GAIN_QPSK_OFFSET,
                                                    value+OFDM_RATE_GAIN_16QAM_OFFSET,
                                                    value+OFDM_RATE_GAIN_64QAM_OFFSET);
                printk("rate gain legacy use value %d\n", value);
            } else
                printk("incorrect rate gain legacy index value %d\n", value);
        } else {

            if (sh->cfg.rf_g_rate_offset != 0)
            {
                value = sh->rf_table.rate_config_g.rate4;
                if (((value + sh->cfg.rf_g_rate_offset) > 0) && ((value + sh->cfg.rf_g_rate_offset) <= MAX_OFDM_RATE_GAIN_INDEX ))
                {
                    value += sh->cfg.rf_g_rate_offset;
                    printk("rate gain g gain offset %d, value %d\n", sh->cfg.rf_g_rate_offset, value);
                    ssv6020_turismoE_chg_legacy_bbscale(sh, value+OFDM_RATE_GAIN_BPSK_OFFSET,
                                                    value+OFDM_RATE_GAIN_QPSK_OFFSET,
                                                    value+OFDM_RATE_GAIN_16QAM_OFFSET,
                                                    value+OFDM_RATE_GAIN_64QAM_OFFSET);
                }
            }

            printk("rate gain legacy use default value\n");
        }
    }

    // set ht20 rate gain
    if (sh->flash_config.exist) {
        ssv6020_turismoE_chg_ht20_bbscale(sh,
                sh->flash_config.rate_delta[5],
                sh->flash_config.rate_delta[6],
                sh->flash_config.rate_delta[7],
                sh->flash_config.rate_delta[8]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_1)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_HT20)) {

            value = ((sh->cfg.rate_table_1 & EFUSE_RATE_TBL_MASK_HT20_RATE) >> EFUSE_RATE_TBL_SFT_HT20_RATE);
            if (sh->cfg.rf_ht20_rate_offset != 0)
            {
                if (((value + sh->cfg.rf_ht20_rate_offset) > 0) && ((value + sh->cfg.rf_ht20_rate_offset) <= MAX_OFDM_RATE_GAIN_INDEX ))
                {
                    value += sh->cfg.rf_ht20_rate_offset;
                    printk("rate gain ht20 gain offset %d\n", sh->cfg.rf_ht20_rate_offset);
                }
            }
            if (value>0 && value <= MAX_OFDM_RATE_GAIN_INDEX) {
                ssv6020_turismoE_chg_ht20_bbscale(sh,
                        value+OFDM_RATE_GAIN_BPSK_OFFSET,
                        value+OFDM_RATE_GAIN_QPSK_OFFSET,
                        value+OFDM_RATE_GAIN_16QAM_OFFSET,
                        value+OFDM_RATE_GAIN_64QAM_OFFSET);
                printk("rate gain ht20 use value %d\n", value);
            } else
                printk("incorrect rate gain ht20 index value %d\n", value);
        } else {
            if (sh->cfg.rf_ht20_rate_offset != 0)
            {
                value = sh->rf_table.rate_config_20n.rate4;
                if ((value + sh->cfg.rf_ht20_rate_offset) > 0 && ((value + sh->cfg.rf_ht20_rate_offset) <= MAX_OFDM_RATE_GAIN_INDEX ))
                {
                    value += sh->cfg.rf_ht20_rate_offset;
                    printk("rate gain ht20 gain offset %d, value %d\n", sh->cfg.rf_ht20_rate_offset, value);
                    ssv6020_turismoE_chg_ht20_bbscale(sh,
                        value+OFDM_RATE_GAIN_BPSK_OFFSET,
                        value+OFDM_RATE_GAIN_QPSK_OFFSET,
                        value+OFDM_RATE_GAIN_16QAM_OFFSET,
                        value+OFDM_RATE_GAIN_64QAM_OFFSET);
                }
            }
            printk("rate gain ht20 use default value\n");
        }
    }

    // set ht40 rate gain
    if (sh->flash_config.exist) {
        ssv6020_turismoE_chg_ht40_bbscale(sh,
                sh->flash_config.rate_delta[9],
                sh->flash_config.rate_delta[10],
                sh->flash_config.rate_delta[11],
                sh->flash_config.rate_delta[12]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_2)) &&
            (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_HT40)) {

            value = ((sh->cfg.rate_table_2 & EFUSE_RATE_TBL_MASK_HT40_RATE) >> EFUSE_RATE_TBL_SFT_HT40_RATE);
            if (sh->cfg.rf_ht40_rate_offset != 0)
            {
                if (((value + sh->cfg.rf_ht40_rate_offset) > 0) && ((value + sh->cfg.rf_ht40_rate_offset) <= MAX_OFDM_RATE_GAIN_INDEX ))
                {
                    value += sh->cfg.rf_ht40_rate_offset;
                    printk("rate gain ht40 gain offset %d, value %d\n", sh->cfg.rf_ht40_rate_offset, value);
                }
            }
            if (value>0 && value <= MAX_OFDM_RATE_GAIN_INDEX) {
                ssv6020_turismoE_chg_ht40_bbscale(sh,
                        value+OFDM_RATE_GAIN_BPSK_OFFSET,
                        value+OFDM_RATE_GAIN_QPSK_OFFSET,
                        value+OFDM_RATE_GAIN_16QAM_OFFSET,
                        value+OFDM_RATE_GAIN_64QAM_OFFSET);
                printk("rate gain ht40 use value %d\n", value);
            } else
                printk("incorrect rate gain ht40 index value %d\n", value);
        } else {
            if (sh->cfg.rf_ht40_rate_offset != 0)
            {
                value = sh->rf_table.rate_config_40n.rate4;
                if (((value + sh->cfg.rf_ht40_rate_offset) > 0) && ((value + sh->cfg.rf_ht40_rate_offset) <= MAX_OFDM_RATE_GAIN_INDEX ))
                {
                    value += sh->cfg.rf_ht40_rate_offset;
                    printk("rate gain ht40 gain offset %d, value %d\n", sh->cfg.rf_ht40_rate_offset, value);
                    ssv6020_turismoE_chg_ht40_bbscale(sh,
                        value+OFDM_RATE_GAIN_BPSK_OFFSET,
                        value+OFDM_RATE_GAIN_QPSK_OFFSET,
                        value+OFDM_RATE_GAIN_16QAM_OFFSET,
                        value+OFDM_RATE_GAIN_64QAM_OFFSET);
                }
            }
            printk("rate gain ht40 use default value\n");
        }
    }
}

static void ssv6020_turismoE_chg_dpd_bbscale(struct ssv_hw *sh)
{
#define MAX_2G_BAND_GAIN    7
    int i = 0, band_gain = 0;

    // set 2.4g band
    if (sh->flash_config.exist) {
        for (i = 0; i < MAX_2G_BAND_GAIN; i++) {
            sh->rf_table.rt_config.band_gain[i] = sh->flash_config.rt_config.g_band_gain[i];
            sh->rf_table.lt_config.band_gain[i] = sh->flash_config.lt_config.g_band_gain[i];
            sh->rf_table.lt_2_config.band_gain[i] = sh->flash_config.lt_config.g_band_gain[i];
            sh->rf_table.ht_config.band_gain[i] = sh->flash_config.ht_config.g_band_gain[i];
            sh->rf_table.ht_2_config.band_gain[i] = sh->flash_config.ht_config.g_band_gain[i];
        }
    } else if (sh->efuse_bitmap & BIT(EFUSE_TX_POWER_INDEX_1)) {
        band_gain = ((sh->cfg.tx_power_index_1 & 0x0f) >> 0);
        if (sh->cfg.rf_band_gain_offset != 0)
        {
            if (((band_gain + sh->cfg.rf_band_gain_offset) > 0) && ((band_gain + sh->cfg.rf_band_gain_offset) <= 15 ))
            {
                band_gain += sh->cfg.rf_band_gain_offset;
                printk("band gain offset %d, value %d\n", sh->cfg.rf_band_gain_offset, band_gain);
            }
        }

        printk("band gain use %d\n", band_gain);
        for (i = 0; i < MAX_2G_BAND_GAIN; i++) {
            sh->rf_table.rt_config.band_gain[i] = band_gain;
            sh->rf_table.lt_config.band_gain[i] = band_gain;
            sh->rf_table.lt_2_config.band_gain[i] = band_gain;
            sh->rf_table.ht_config.band_gain[i] = band_gain;
            sh->rf_table.ht_2_config.band_gain[i] = band_gain;
        }
    } else {
        band_gain = sh->rf_table.rt_config.band_gain[0];
        if (sh->cfg.rf_band_gain_offset != 0)
        {
            if (((band_gain + sh->cfg.rf_band_gain_offset) > 0) && ((band_gain + sh->cfg.rf_band_gain_offset) <= 15))
            {
                band_gain += sh->cfg.rf_band_gain_offset;
                printk("band gain offset %d, value %d\n", sh->cfg.rf_band_gain_offset, band_gain);
                for (i = 0; i < MAX_2G_BAND_GAIN; i++) {
                    sh->rf_table.rt_config.band_gain[i] = band_gain;
                    sh->rf_table.lt_config.band_gain[i] = band_gain;
                    sh->rf_table.lt_2_config.band_gain[i] = band_gain;
                    sh->rf_table.ht_config.band_gain[i] = band_gain;
                    sh->rf_table.ht_2_config.band_gain[i] = band_gain;
                }
            }
        }


        printk("band gain use default value\n");
    }
    // 6020 not support 5G band.
}

static void ssv6020_turismo_chg_dcdc_flag(struct ssv_hw *sh)
{
    if (sh->cfg.volt_regulator == SSV6XXX_VOLT_LDO_CONVERT)
        sh->rf_table.dcdc_flag = 0;
    else
        sh->rf_table.dcdc_flag = 1;
}

static void ssv6020_turismo_chg_tx_lowpower(struct ssv_hw *sh)
{
    sh->rf_table.tx_lowpower_flag = sh->cfg.tx_lowpower;
}

static void ssv6020_turismo_chg_padpd(struct ssv_hw *sh)
{
    if(0 != sh->cfg.padpd)
    {
        sh->rf_table.lt_config.padpd_cali = 1;
        sh->rf_table.lt_2_config.padpd_cali = 1;
        sh->rf_table.rt_config.padpd_cali = 1;
        sh->rf_table.ht_config.padpd_cali = 1;
        sh->rf_table.ht_2_config.padpd_cali = 1;
    }
    else
    {
        sh->rf_table.lt_config.padpd_cali = 0;
        sh->rf_table.lt_2_config.padpd_cali = 0;
        sh->rf_table.rt_config.padpd_cali = 0;
        sh->rf_table.ht_config.padpd_cali = 0;
        sh->rf_table.ht_2_config.padpd_cali = 0;
    }
}

static void ssv6020_turismo_chg_ble_gain(struct ssv_hw *sh)
{
    int i = 0, ble_gain = 0;

    if (sh->efuse_bitmap & BIT(EFUSE_BLE_GAIN)) {
        ble_gain = ((sh->cfg.ble_gain & 0x0f) >> 0);
        printk("ble gain use %d\n", ble_gain);
        for (i = 0; i < BLE_CHANNEL_PART_COUNT; i++) {
            sh->rf_table.ble_rt_config.band_gain[i] = ble_gain;
            sh->rf_table.ble_lt_config.band_gain[i] = ble_gain;
            sh->rf_table.ble_lt_2_config.band_gain[i] = ble_gain;
            sh->rf_table.ble_ht_config.band_gain[i] = ble_gain;
            sh->rf_table.ble_ht_2_config.band_gain[i] = ble_gain;
        }
    } else {
        printk("ble gain use default value\n");
    }

}

static void ssv6020_turismoE_update_rf_table(struct ssv_hw *sh)
{
    // init rf_table
    memcpy(&sh->rf_table, &def_rf_table, sizeof(struct st_rf_table));
    ssv6020_turismoE_chg_xtal_freq_offset(sh);
    ssv6020_turismoE_chg_bbscale(sh);
    ssv6020_turismoE_chg_dpd_bbscale(sh);
    ssv6020_turismo_chg_dcdc_flag(sh);
    ssv6020_turismo_chg_tx_lowpower(sh);
    ssv6020_turismo_chg_padpd(sh);
    ssv6020_turismo_chg_ble_gain(sh);
}

void ssv_attach_ssv6020_turismoE_BBRF(struct ssv_hal_ops *hal_ops)
{
    hal_ops->load_phy_table = ssv6020_turismoE_load_phy_table;
    hal_ops->get_phy_table_size = ssv6020_turismoE_get_phy_table_size;
    hal_ops->load_rf_table = ssv6020_turismoE_load_rf_table;
    hal_ops->get_rf_table_size = ssv6020_turismoE_get_rf_table_size;
    hal_ops->init_pll = ssv6020_turismoE_init_PLL;
    hal_ops->set_channel = ssv6020_turismoE_set_channel;
    hal_ops->set_pll_phy_rf = ssv6020_turismoE_set_pll_phy_rf;
    hal_ops->set_rf_enable = ssv6020_turismoE_set_rf_enable;

    hal_ops->dump_phy_reg = ssv6020_turismoE_dump_phy_reg;
    hal_ops->dump_rf_reg = ssv6020_turismoE_dump_rf_reg;
    hal_ops->support_iqk_cmd = ssv6020_turismoE_support_iqk_cmd;
    hal_ops->cmd_txgen = ssv6020_cmd_turismoE_txgen;
    hal_ops->cmd_rf = ssv6020_cmd_turismoE_rf;
    hal_ops->cmd_rfble = ssv6020_cmd_turismoE_rfble;
}

void ssv_attach_ssv6020_turismoE_ext_BBRF(struct ssv_hal_ops *hal_ops)
{
    hal_ops->cmd_txgen = ssv6020_cmd_turismoE_txgen;
    hal_ops->cmd_rf = ssv6020_cmd_turismoE_rf;
    hal_ops->cmd_rfble = ssv6020_cmd_turismoE_rfble;
    hal_ops->update_rf_table = ssv6020_turismoE_update_rf_table;
}

static void ssv6020_fpga_cmd_turismoE_txgen(struct ssv_hw *sh, u8 drate)
{
    printk("FPGA cannot support the function %s\n", __FUNCTION__);
}

static void ssv6020_fpga_cmd_turismoE_rf(struct ssv_hw *sh, int argc, char *argv[])
{
    printk("FPGA cannot support the function %s\n", __FUNCTION__);
}

static void ssv6020_fpga_cmd_turismoE_rfble(struct ssv_hw *sh, int argc, char *argv[])
{
    printk("FPGA cannot support the function %s\n", __FUNCTION__);
}

static void ssv6020_fpga_turismoE_update_rf_table(struct ssv_hw *sh)
{
    printk("FPGA cannot support the function %s\n", __FUNCTION__);
}

void ssv_attach_ssv6020_turismoE_fpga_BBRF(struct ssv_hal_ops *hal_ops)
{
    hal_ops->cmd_txgen = ssv6020_fpga_cmd_turismoE_txgen;
    hal_ops->cmd_rf = ssv6020_fpga_cmd_turismoE_rf;
    hal_ops->cmd_rfble = ssv6020_fpga_cmd_turismoE_rfble;
    hal_ops->update_rf_table = ssv6020_fpga_turismoE_update_rf_table;
}
#endif
