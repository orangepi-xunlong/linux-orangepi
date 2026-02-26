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
#if ((defined SSV_SUPPORT_SSV6006))
#include <linux/nl80211.h>
#include <ssv6200.h>
#include "ssv6006c/ssv6006C_reg.h"
#include "ssv6006c/ssv6006C_aux.h"
#include <smac/dev.h>
#include <smac/efuse.h>
#include <smac/ssv_skb.h>
#include <hal.h>
#include <ssvdevice/ssv_cmd.h>
#include "ssv6006_mac.h"
#include "ssv6006_priv.h"
#include "ssv6006_priv_safe.h"
#include "turismoC_rf_reg.c"
#include "turismoC_wifi_phy_reg.c"
#include "ssv_desc.h"
#include "turismo_common.h"
#include "turismo_common.c"

#include <ssv6xxx_common.h>
#include <linux_80211.h>

static void ssv6006_turismoC_init_cali(struct ssv_hw *sh);
static bool ssv6006_turismoC_set_rf_enable(struct ssv_hw *sh, bool val);
static void _set_tx_pwr(struct ssv_softc *sc, u32 pa_band, u32 txpwr);

static void ssv6006_turismoC_load_phy_table(ssv_cabrio_reg **phy_table)
{
    return;
}

static u32 ssv6006_turismoC_get_phy_table_size(struct ssv_hw *sh)
{
    return 0;
}

static void ssv6006_turismoC_load_rf_table(ssv_cabrio_reg **rf_table)
{
    return;
}

static u32 ssv6006_turismoC_get_rf_table_size(struct ssv_hw *sh)
{
    return 0;
}

static void ssv6006_turismoC_init_PLL(struct ssv_hw *sh)
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
    ret = HCI_SEND_CMD(sc->sh, skb);
}

static int ssv6006_turismoC_set_channel(struct ssv_softc *sc, struct ieee80211_channel *chan,
        enum nl80211_channel_type channel_type, bool offchan)
{
    u16 ch = chan->hw_value;
    struct ssv_hw *sh = sc->sh;
    struct sk_buff *skb = NULL;
    struct cfg_host_cmd *host_cmd = NULL;
    struct ssv_rf_chan *ptr = NULL;
    int ret = 0;

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

    ptr = (struct ssv_rf_chan *)host_cmd->dat8;
    memset(ptr, 0, sizeof(struct ssv_rf_chan));
    ptr->chan = chan->hw_value;
    ptr->chan_type = channel_type;
    ptr->off_chan = offchan;
    if ((false == sc->hw_scan_done) || ((true == sc->bScanning) && (true == sc->hw_scan_start))) {
        ptr->scan = 1;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
        if (sc->scan_is_passive || (chan->flags & (IEEE80211_CHAN_NO_IR | IEEE80211_CHAN_RADAR))) {
    #else
        if (sc->scan_is_passive || (chan->flags & (IEEE80211_CHAN_RADAR))) {
    #endif
            ptr->passive_chan = 1;
        }
        memcpy(&ptr->scan_param, &sc->scan_param, sizeof(struct ssv_scan_param));
        if (chan->hw_value > 14)
            ptr->scan_param.no_cck = true;
    }
    printk("chan change ch %d, type %d, off_chan %d\n", ptr->chan, ptr->chan_type, ptr->off_chan);
    ret = HCI_SEND_CMD(sc->sh, skb);

    // update tx power
    sc->dpd.pwr_mode  = NORMAL_PWR;
    // update extend pa
    if (IS_DUAL_BAND_EX_PA(sh)) {
        if (BAND_2G != ssv6006_get_pa_band(ch)) {
            ssv6006c_set_external_pa(sh, EX_PA_5G_ENABLE);
        } else {
            ssv6006c_set_external_pa(sh, EX_PA_2G_ENABLE);
        }
        if (sc->boffchan) {
            ssv6006c_set_external_lna(sh, true);
        } else {
            ssv6006c_set_external_lna(sh, sc->LNA_status);
        }
    }
    return 0;
}

static void ssv6006_turismoC_init_cali(struct ssv_hw *sh)
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

static u32 ssv6006_turismoC_update_xtal(struct ssv_hw *sh)
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

static int ssv6006_turismoC_set_pll_phy_rf(struct ssv_hw *sh ,ssv_cabrio_reg *rf_tbl, ssv_cabrio_reg *phy_tbl)
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

    ptr = (struct ssv_rf_cali *)host_cmd->dat8;
    memset(ptr, 0, sizeof(struct ssv_rf_cali));
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ)
        ptr->support_5g = 1;
    if (sh->cfg.volt_regulator == SSV6XXX_VOLT_LDO_CONVERT)
        ptr->disable_dcdc = 1;
    if ((sh->rf_table.low_boundary == 0) && (sh->rf_table.high_boundary == 0)) {
        ptr->thermal = 0;
    } else {
        ptr->thermal = 1;
        ptr->thermal_low_thershold = sh->rf_table.low_boundary;
        ptr->thermal_high_thershold = sh->rf_table.high_boundary;
    }
    ptr->xtal = ssv6006_turismoC_update_xtal(sh);
    ptr->bus_clk = ((sh->cfg.clk_src_80m == true) ? 80 : 40);
    memcpy(&ptr->rf_table, &sh->rf_table, sizeof(struct st_rf_table));

    ret = HCI_SEND_CMD(sc->sh, skb);

    // default txgain
    sh->default_txgain[BAND_2G] = GET_RG_TX_GAIN;
    sh->default_txgain[BAND_5100] = GET_RG_5G_TX_GAIN_F0;
    sh->default_txgain[BAND_5500] = GET_RG_5G_TX_GAIN_F1;
    sh->default_txgain[BAND_5700] = GET_RG_5G_TX_GAIN_F2;
    sh->default_txgain[BAND_5900] = GET_RG_5G_TX_GAIN_F3;

    return ret;
}

static bool ssv6006_turismoC_set_rf_enable(struct ssv_hw *sh, bool val)
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

static bool ssv6006_turismoC_dump_phy_reg(struct ssv_hw *sh)
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

static bool ssv6006_turismoC_dump_rf_reg(struct ssv_hw *sh)
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

static bool ssv6006_turismoC_support_iqk_cmd(struct ssv_hw *sh)
{
    return false;
}

static void ssv6006_cmd_turismoC_txgen(struct ssv_hw *sh, u8 drate)
{
    struct sk_buff *skb = NULL;
    int    len = (int) sizeof(pkt1614);
    unsigned char *data = NULL;
    struct ssv6006_tx_desc *tx_desc;
    struct ssv_softc *sc = sh->sc;

    skb = ssv_skb_alloc(sc, len);
    if (!skb)
        return;

    data = skb_put(skb, len);
    memcpy(data, pkt1614, len);
    tx_desc = (struct ssv6006_tx_desc *)data;
    tx_desc->drate_idx0 = drate;

    SET_RG_TXD_SEL(0);  // set tx gen path disable
    SET_RG_TX_START(0); // disable trigger tx!!
    HCI_SEND_CMD(sc->sh, skb);
}

static void ssv6006_cmd_turismoC_rfphy_ops(struct ssv_softc *sc,  ssv_rf_phy_ops PHY_OPS,struct ssv_rf_tool_param *param)
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
			rf_tool_param = (struct ssv_rf_tool_param *)host_rf_cmd->dat8;
			memcpy(rf_tool_param, param, sizeof(struct ssv_rf_tool_param));
		}

		HCI_SEND_CMD(sc->sh, skb);
    }
}

static void ssv6006_cmd_turismoC_rfphy_ops_start(struct ssv_softc *sc, u32 regval)
{
	struct ssv_rf_tool_param rf_param;;
	memset(&rf_param ,0 ,sizeof(struct ssv_rf_tool_param));
	rf_param.count = 0xffffffff;
	rf_param.interval = regval;
	ssv6006_cmd_turismoC_rfphy_ops(sc, SSV6XXX_RFPHY_CMD_RF_TOOL_TX, &rf_param);
}

static void ssv6006_cmd_turismoC_rfphy_ops_stop(struct ssv_softc *sc)
{
	ssv6006_cmd_turismoC_rfphy_ops(sc, SSV6XXX_RFPHY_CMD_RF_TOOL_STOP, NULL);
}


static void ssv6006_cmd_turismoC_rf(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    u32 regval = 0;
    char *endp;
	bool is_txgen = false;
	u32 interval = 0;
    struct ssv_softc *sc = sh->sc;
    int ch = sc->hw_chan;
    int pa_band =0;
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

    pa_band = ssv6006_get_pa_band(ch);

    if (argc < 2)
        goto out;

    if (!strcmp(argv[1], "tx")) {

        if (argc == 3)
            regval = simple_strtoul(argv[2], &endp, 0);
        else
            regval = 1;

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

            rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->dat8;
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

    } else if(!strcmp(argv[1], "rate")){
        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            if ((regval != 4) && (regval < SIZE_RATE_TBL)) {
				if((1==GET_RG_TX_START)&&(1==GET_RG_TXD_SEL)){
					is_txgen = true;
					interval = GET_RG_IFS_TIME;
				}

				if(is_txgen){
					ssv6006_cmd_turismoC_rfphy_ops_stop(sc);
				}
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

                    rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->dat8;
                    memset(rf_tool_param, 0, sizeof(struct ssv_rf_tool_param));
                    rf_tool_param->rate = regval;

                    HCI_SEND_CMD(sc->sh, skb);
                    snprintf_res(cmd_data,"\n   rf rate index %d\n", regval);
                } else{
                    snprintf_res(cmd_data,"\n Cannot alloc host command buffer\n");
				}

				if(is_txgen){
					ssv6006_cmd_turismoC_rfphy_ops_start(sc,interval);
				}

            } else {
                snprintf_res(cmd_data,"Not support rf rate index %d\n", regval);
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

                rf_tool_param = (struct ssv_rf_tool_param *)host_cmd->dat8;
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
            snprintf_res(cmd_data,"Get freq 0x%x/0x%x\n", GET_RG_XO_CBANKI, GET_RG_XO_CBANKO);
        } else {
            snprintf_res(cmd_data,"./cli rf rfreq\n");
        }
        return;

    } else if(!strcmp(argv[1], "count")){
        if  (argc == 3) {
            int     count = 0,  err = 0, integer= 0, point = 0;
            bool valid =false;
            if (!strcmp(argv[2], "0")){
                count = GET_RO_11B_PACKET_CNT;
                err = GET_RO_11B_PACKET_ERR_CNT;
                valid = true;
            } else if(!strcmp(argv[2], "1")){
                count = GET_RO_11GN_PACKET_CNT;
                err = GET_RO_11GN_PACKET_ERR_CNT;
                valid = true;
            }
            if (count != 0) {
                integer = (err * 100)/count;
                point = ((err*10000)/count)%100;
            }
            if (valid) {
                snprintf_res(cmd_data,"count = %d\n", count);
                snprintf_res(cmd_data,"err = %d\n", err);
                snprintf_res(cmd_data,"err_rate = %01d.%02d%\n", integer, point);
                return;
            }
        }
        snprintf_res(cmd_data,"\n\t./cli rf count 0|1\n");
        return;

    } else if(!strcmp(argv[1], "greentx")) {
        if (argc == 3) {
            if(!strcmp(argv[2], "enable")) {
                sh->cfg.greentx |= GT_ENABLE;
                sc->gt_channel = sc->hw_chan;
                sc->gt_enabled = true;
                sc->green_pwr = 0xff;
                snprintf_res(cmd_data,"\n\t Green Tx enabled, start attenuation from -%d dB\n"
                        , sh->cfg.greentx & GT_PWR_START_MASK);
            } else if(!strcmp(argv[2], "disable")) {

                sh->cfg.greentx &= (~GT_ENABLE);
                sc->dpd.pwr_mode  = NORMAL_PWR;
                sc->green_pwr = 0xff;
                sc->gt_enabled = false;
                snprintf_res(cmd_data,"\n\t Green Tx disabled");
            }
        } else if ((argc == 4) && (!strcmp(argv[2], "dbg"))) {
            if(!strcmp(argv[3], "enable")) {
                sh->cfg.greentx |= GT_DBG;
                snprintf_res(cmd_data,"\n\t Green Tx Debug enable\n");
            } else if(!strcmp(argv[3], "disable")) {
                sh->cfg.greentx &= (~GT_DBG);
                snprintf_res(cmd_data,"\n\t Green Tx debug disabled");
            }
        } else if ((argc == 4) && (!strcmp(argv[2], "stepsize"))) {
            regval = simple_strtoul(argv[3], &endp, 0);
            sh->cfg.gt_stepsize = regval;
            snprintf_res(cmd_data,"\n\t Green Tx set step size to %d\n", regval);
        } else if ((argc == 4) && (!strcmp(argv[2], "max_atten"))) {
            regval = simple_strtoul(argv[3], &endp, 0);
            sh->cfg.gt_max_attenuation = regval;
            snprintf_res(cmd_data,"\n\t Green Tx set gt_max_attenuation  to %d\n", regval);
        } else{
            snprintf_res(cmd_data,"\n\t Incorrect rf greentx format\n");
        }
        return;

    } else  if (!strcmp(argv[1], "rgreentx")){
        snprintf_res(cmd_data,"\n cfg.greentx 0x%x, Tx Gain : %d %d %d %d %d\n",
                sh->cfg.greentx,  GET_RG_TX_GAIN, GET_RG_5G_TX_GAIN_F0,
                GET_RG_5G_TX_GAIN_F1, GET_RG_5G_TX_GAIN_F2, GET_RG_5G_TX_GAIN_F3);
        snprintf_res(cmd_data,"\n step size %d, max attenuaton %d\n",
                sh->cfg.gt_stepsize, sh->cfg.gt_max_attenuation);
        return;

    } else if(!strcmp(argv[1], "rssi")){
        snprintf_res(cmd_data,"\n ofdm RSSI -%d, B mode RSSI -%d\n",
                GET_RO_11GN_RCPI, GET_RO_11B_RCPI);
        return;

    } else if(!strcmp(argv[1], "external_pa")) {

        if (argc == 3){
            regval = simple_strtoul(argv[2], &endp, 0);
            sh-> cfg.external_pa = regval;
            snprintf_res(cmd_data,"\n\t set external pa %s\n",
                    (sc->sh->cfg.external_pa & EX_PA_ENABLE) ? "enable":"disable");
        } else if ((argc == 4) && (!(strcmp(argv[2], "LNA")))){
            regval = simple_strtoul(argv[3], &endp, 0);
            sc->LNA_status = regval;
            if (regval == 0)
                ssv6006c_set_external_lna(sh, false);
            else
                ssv6006c_set_external_lna(sh, true);
        } else {
            snprintf_res(cmd_data,"\n\t./cli rf exteranl_pa [LNA][val]\n");
        }
        snprintf_res(cmd_data, "\n\texternal PA settings 0x%x, LNA status %d, PUE0x%x\n", sh->cfg.external_pa, sc->LNA_status, GET_IO_PUE);
        return;
    } else if(!strcmp(argv[1], "phy_txgen")) {

        ssv6006_cmd_turismoC_rfphy_ops_start(sc, 1);
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
        ssv6006_cmd_turismoC_rfphy_ops_stop(sc);
        snprintf_res(cmd_data,"\n\t unblock control form system\n");
        return;
	} else if(!strcmp(argv[1], "ack")){
        if (argc == 3) {
            if (!strcmp(argv[2], "disable")){
                snprintf_res(cmd_data,"\n set %s %s\n", argv[1], argv[2]);
            } else if (!strcmp(argv[2], "enable")){
                snprintf_res(cmd_data,"\n set %s %s\n", argv[1], argv[2]);
            }
        } else {
            snprintf_res(cmd_data,"\n\t incorrect set ack format\n");
        }
        return;
	}else {

        snprintf_res(cmd_data,
                "\n\t./cli rf phy_txgen|block|unblock|count|ack|freq|rfreq|rssi|rate|ifs|external_pa\n");
        return;
    }
out:

    snprintf_res(cmd_data,"\n\t Current RF tool settings: ch %d, pa_band %d\n", ch, pa_band);
    snprintf_res(cmd_data,"\t bbscale:\n");

    if (sc->sc_flags && SC_OP_BLOCK_CNTL) {
        snprintf_res(cmd_data,"\t system control is blocked\n");
    } else {
        snprintf_res(cmd_data,"\t WARNING system control is not blocked\n");
    }
    snprintf_res(cmd_data,"\t\t HT40 0x%08x, HT20 0x%08x, Legacy 0x%08x, B 0x%02x\n",
            REG32(ADR_WIFI_PHY_COMMON_BB_SCALE_REG_3), REG32(ADR_WIFI_PHY_COMMON_BB_SCALE_REG_2),
            REG32(ADR_WIFI_PHY_COMMON_BB_SCALE_REG_1), GET_RG_BB_SCALE_BARKER_CCK);

    switch (pa_band){
        case BAND_2G:
            regval = GET_RG_DPD_BB_SCALE_2500;
            break;
        case BAND_5100:
            regval = GET_RG_DPD_BB_SCALE_5100;
            break;
        case BAND_5500:
            regval = GET_RG_DPD_BB_SCALE_5500;
            break;
        case BAND_5700:
            regval = GET_RG_DPD_BB_SCALE_5700;
            break;
        case BAND_5900:
            regval = GET_RG_DPD_BB_SCALE_5900;
            break;
        default:
            break;
    }
    snprintf_res(cmd_data,"\t current band dpd bbscale: 0x%x\n", regval);

    snprintf_res(cmd_data,"\t cbank:\n");
    snprintf_res(cmd_data,"\t\t CBANKI %d, CBANKO %d\n", GET_RG_XO_CBANKI, GET_RG_XO_CBANKO);
}

static void ssv6006_turismoC_chg_xtal_freq_offset(struct ssv_hw *sh)
{
    if (sh->flash_config.exist) {
        // [15:8] xi, [7:0] xo
        sh->rf_table.low_boundary = sh->flash_config.xtal_offset_low_boundary;
        sh->rf_table.high_boundary = sh->flash_config.xtal_offset_high_boundary;
        sh->rf_table.rt_config.freq_xi = ((sh->flash_config.rt_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.rt_config.freq_xo = ((sh->flash_config.rt_config.xtal_offset & 0x00ff) >> 0);
        sh->rf_table.lt_config.freq_xi = ((sh->flash_config.lt_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.lt_config.freq_xo = ((sh->flash_config.lt_config.xtal_offset & 0x00ff) >> 0);
        sh->rf_table.ht_config.freq_xi = ((sh->flash_config.ht_config.xtal_offset & 0xff00) >> 8);
        sh->rf_table.ht_config.freq_xo = ((sh->flash_config.ht_config.xtal_offset & 0x00ff) >> 0);
    } else if (sh->efuse_bitmap & BIT(EFUSE_CRYSTAL_FREQUENCY_OFFSET)) {
        sh->rf_table.low_boundary = 0;
        sh->rf_table.high_boundary = 0;
        sh->rf_table.rt_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.rt_config.freq_xo = sh->cfg.crystal_frequency_offset;
        sh->rf_table.lt_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.lt_config.freq_xo = sh->cfg.crystal_frequency_offset;
        sh->rf_table.ht_config.freq_xi = sh->cfg.crystal_frequency_offset;
        sh->rf_table.ht_config.freq_xo = sh->cfg.crystal_frequency_offset;
    } else {
        // refer to SSV6006_TURISMOC_RF_TABLE setting
        sh->rf_table.low_boundary = 0;
        sh->rf_table.high_boundary = 0;
        sh->rf_table.rt_config.freq_xi = 0x42;
        sh->rf_table.rt_config.freq_xo = 0x42;
        sh->rf_table.lt_config.freq_xi = 0x42;
        sh->rf_table.lt_config.freq_xo = 0x42;
        sh->rf_table.ht_config.freq_xi = 0x42;
        sh->rf_table.ht_config.freq_xo = 0x42;
    }
    sh->rf_table.low_boundary = 30;
    sh->rf_table.high_boundary = 50;
}

static void ssv6006_turismoC_chg_cck_bbscale(struct ssv_hw *sh, int cck)
{
    sh->rf_table.rate_gain_b = (u8)cck;
}

static void ssv6006_turismoC_chg_legacy_bbscale(struct ssv_hw *sh, int bpsk, int qpsk, int qam16, int qam64)
{
    sh->rf_table.rate_config_g.rate1 = bpsk;
    sh->rf_table.rate_config_g.rate2 = qpsk;
    sh->rf_table.rate_config_g.rate3 = qam16;
    sh->rf_table.rate_config_g.rate4 = qam64;
}

static void ssv6006_turismoC_chg_ht20_bbscale(struct ssv_hw *sh, int bpsk, int qpsk, int qam16, int qam64)
{
    sh->rf_table.rate_config_20n.rate1 = bpsk;
    sh->rf_table.rate_config_20n.rate2 = qpsk;
    sh->rf_table.rate_config_20n.rate3 = qam16;
    sh->rf_table.rate_config_20n.rate4 = qam64;
}

static void ssv6006_turismoC_chg_ht40_bbscale(struct ssv_hw *sh, int bpsk, int qpsk, int qam16, int qam64)
{
    sh->rf_table.rate_config_40n.rate1 = bpsk;
    sh->rf_table.rate_config_40n.rate2 = qpsk;
    sh->rf_table.rate_config_40n.rate3 = qam16;
    sh->rf_table.rate_config_40n.rate4 = qam64;
}

static void ssv6006_turismoC_chg_bbscale(struct ssv_hw *sh)
{
    int value = 0;
    int b_rate_gain_tbl[] = {0x98, 0x90, 0x88, 0x80, 0x79, 0x72, 0x6c, 0x66,
        0x60, 0x5b, 0x56, 0x51, 0x4c, 0x48, 0x44, 0x40};
    int ofdm_rate_gain_tbl[] = {0xb5, 0xab, 0xa1, 0x98, 0x90, 0x88, 0x80, 0x79, 0x72, 0x6c,
        0x66, 0x60, 0x5b, 0x56, 0x51, 0x4c, 0x48, 0x44, 0x40};

#define SIZE_B_RATE_GAIN_TBL   (sizeof(b_rate_gain_tbl) / sizeof((b_rate_gain_tbl)[0]))
#define MAX_OFDM_RATE_GAIN_INDEX                 (12)
#define OFDM_RATE_GAIN_64QAM_OFFSET              (6)
#define OFDM_RATE_GAIN_16QAM_OFFSET              (4)
#define OFDM_RATE_GAIN_QPSK_OFFSET               (2)
#define OFDM_RATE_GAIN_BPSK_OFFSET               (0)

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
        ssv6006_turismoC_chg_cck_bbscale(sh, sh->flash_config.rate_delta[0]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_1)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_CCK)) {

            value = ((sh->cfg.rate_table_1 & EFUSE_RATE_TBL_MASK_B_RATE) >> EFUSE_RATE_TBL_SFT_B_RATE);
            if (value < SIZE_B_RATE_GAIN_TBL)
                ssv6006_turismoC_chg_cck_bbscale(sh, b_rate_gain_tbl[value]);
            else
                ssv6006_turismoC_chg_cck_bbscale(sh, DEFAULT_CCK_BBSCALE);
        } else
            ssv6006_turismoC_chg_cck_bbscale(sh, DEFAULT_CCK_BBSCALE);
    }

    // set legacy rate gain
    if (sh->flash_config.exist) {
        ssv6006_turismoC_chg_legacy_bbscale(sh,
                sh->flash_config.rate_delta[1],
                sh->flash_config.rate_delta[2],
                sh->flash_config.rate_delta[3],
                sh->flash_config.rate_delta[4]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_2)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_LEGACY)) {

            value = ((sh->cfg.rate_table_2 & EFUSE_RATE_TBL_MASK_LEGACY_RATE) >> EFUSE_RATE_TBL_SFT_LEGACY_RATE);
            if (value <= MAX_OFDM_RATE_GAIN_INDEX) {
                ssv6006_turismoC_chg_legacy_bbscale(sh,
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_BPSK_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_QPSK_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_16QAM_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_64QAM_OFFSET]);
            } else {
                ssv6006_turismoC_chg_legacy_bbscale(sh,
                        DEFAULT_LEGACY_BPSK_BBSCALE, DEFAULT_LEGACY_QPSK_BBSCALE,
                        DEFAULT_LEGACY_QAM16_BBSCALE, DEFAULT_LEGACY_QAM64_BBSCALE);
            }
        } else {
            ssv6006_turismoC_chg_legacy_bbscale(sh,
                    DEFAULT_LEGACY_BPSK_BBSCALE, DEFAULT_LEGACY_QPSK_BBSCALE,
                    DEFAULT_LEGACY_QAM16_BBSCALE, DEFAULT_LEGACY_QAM64_BBSCALE);

        }
    }

    // set ht20 rate gain
    if (sh->flash_config.exist) {
        ssv6006_turismoC_chg_ht20_bbscale(sh,
                sh->flash_config.rate_delta[5],
                sh->flash_config.rate_delta[6],
                sh->flash_config.rate_delta[7],
                sh->flash_config.rate_delta[8]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_2)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_HT20)) {

            value = ((sh->cfg.rate_table_2 & EFUSE_RATE_TBL_MASK_HT20_RATE) >> EFUSE_RATE_TBL_SFT_HT20_RATE);
            if (value < MAX_OFDM_RATE_GAIN_INDEX) {
                ssv6006_turismoC_chg_ht20_bbscale(sh,
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_BPSK_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_QPSK_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_16QAM_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_64QAM_OFFSET]);
            } else {
                ssv6006_turismoC_chg_ht20_bbscale(sh,
                        DEFAULT_HT20_BPSK_BBSCALE, DEFAULT_HT20_QPSK_BBSCALE,
                        DEFAULT_HT20_QAM16_BBSCALE, DEFAULT_HT20_QAM64_BBSCALE);
            }
        } else {
            ssv6006_turismoC_chg_ht20_bbscale(sh,
                    DEFAULT_HT20_BPSK_BBSCALE, DEFAULT_HT20_QPSK_BBSCALE,
                    DEFAULT_HT20_QAM16_BBSCALE, DEFAULT_HT20_QAM64_BBSCALE);

        }
    }

    // set ht40 rate gain
    if (sh->flash_config.exist) {
        ssv6006_turismoC_chg_ht40_bbscale(sh,
                sh->flash_config.rate_delta[9],
                sh->flash_config.rate_delta[10],
                sh->flash_config.rate_delta[11],
                sh->flash_config.rate_delta[12]);
    } else {
        if ((sh->efuse_bitmap & BIT(EFUSE_RATE_TABLE_1)) &&
                (sh->cfg.efuse_rate_gain_mask & EFUSE_RATE_GAIN_MASK_HT40)) {

            value = ((sh->cfg.rate_table_1 & EFUSE_RATE_TBL_MASK_HT40_RATE) >> EFUSE_RATE_TBL_SFT_HT40_RATE);
            if (value < MAX_OFDM_RATE_GAIN_INDEX) {
                ssv6006_turismoC_chg_ht40_bbscale(sh,
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_BPSK_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_QPSK_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_16QAM_OFFSET],
                        ofdm_rate_gain_tbl[value+OFDM_RATE_GAIN_64QAM_OFFSET]);
            } else {
                ssv6006_turismoC_chg_ht40_bbscale(sh,
                        DEFAULT_HT40_BPSK_BBSCALE, DEFAULT_HT40_QPSK_BBSCALE,
                        DEFAULT_HT40_QAM16_BBSCALE, DEFAULT_HT40_QAM64_BBSCALE);
            }
        } else {
            ssv6006_turismoC_chg_ht40_bbscale(sh,
                    DEFAULT_HT40_BPSK_BBSCALE, DEFAULT_HT40_QPSK_BBSCALE,
                    DEFAULT_HT40_QAM16_BBSCALE, DEFAULT_HT40_QAM64_BBSCALE);
        }
    }
}

static void ssv6006_turismoC_chg_dpd_bbscale(struct ssv_hw *sh)
{
    int i = 0, value = 0;
    int band_gain_tbl[] = {0x48, 0x4c, 0x51, 0x56, 0x5b, 0x60, 0x66, 0x6c,
        0x72, 0x79, 0x80, 0x88, 0x90, 0x98, 0xa1, 0xab};

#define MAX_2G_BAND_GAIN    7
#define SIZE_BAND_GAIN_TBL   (sizeof(band_gain_tbl) / sizeof((band_gain_tbl)[0]))

    // set 2.4g band
    if (sh->flash_config.exist) {
        for (i = 0; i < MAX_2G_BAND_GAIN; i++) {
            sh->rf_table.rt_config.band_gain[i] = sh->flash_config.rt_config.g_band_gain[i];
            sh->rf_table.lt_config.band_gain[i] = sh->flash_config.lt_config.g_band_gain[i];
            sh->rf_table.ht_config.band_gain[i] = sh->flash_config.ht_config.g_band_gain[i];
        }
    } else if (sh->efuse_bitmap & BIT(EFUSE_TX_POWER_INDEX_1)) {
        value = ((sh->cfg.tx_power_index_1 & 0x0f) >> 0);
        for (i = 0; i < MAX_2G_BAND_GAIN; i++) {
            sh->rf_table.rt_config.band_gain[i] = band_gain_tbl[value];
            sh->rf_table.lt_config.band_gain[i] = band_gain_tbl[value];
            sh->rf_table.ht_config.band_gain[i] = band_gain_tbl[value];
        }
    } else {
        for (i = 0; i < MAX_2G_BAND_GAIN; i++) {
            sh->rf_table.rt_config.band_gain[i] = DEFAULT_DPD_BBSCALE_2500;
            sh->rf_table.lt_config.band_gain[i] = DEFAULT_DPD_BBSCALE_2500;
            sh->rf_table.ht_config.band_gain[i] = DEFAULT_DPD_BBSCALE_2500;
        }
    }

    //set band 5100
    if (sh->flash_config.exist) {
        sh->rf_table.rt_5g_config.bbscale_band0 = sh->flash_config.rt_config.a_band_gain[0];
        sh->rf_table.lt_5g_config.bbscale_band0 = sh->flash_config.lt_config.a_band_gain[0];
        sh->rf_table.ht_5g_config.bbscale_band0 = sh->flash_config.ht_config.a_band_gain[0];
    } else {
        sh->rf_table.rt_5g_config.bbscale_band0 = DEFAULT_DPD_BBSCALE_5100;
        sh->rf_table.lt_5g_config.bbscale_band0 = DEFAULT_DPD_BBSCALE_5100;
        sh->rf_table.ht_5g_config.bbscale_band0 = DEFAULT_DPD_BBSCALE_5100;
    }

    //set band 5500
    if (sh->flash_config.exist) {
        sh->rf_table.rt_5g_config.bbscale_band1 = sh->flash_config.rt_config.a_band_gain[1];
        sh->rf_table.lt_5g_config.bbscale_band1 = sh->flash_config.lt_config.a_band_gain[1];
        sh->rf_table.ht_5g_config.bbscale_band1 = sh->flash_config.ht_config.a_band_gain[1];
    } else if (sh->efuse_bitmap & BIT(EFUSE_TX_POWER_INDEX_1)) {
        value = ((sh->cfg.tx_power_index_1 & 0xf0) >> 4);
        sh->rf_table.rt_5g_config.bbscale_band1 = band_gain_tbl[value];
        sh->rf_table.lt_5g_config.bbscale_band1 = band_gain_tbl[value];
        sh->rf_table.ht_5g_config.bbscale_band1 = band_gain_tbl[value];
    } else {
        sh->rf_table.rt_5g_config.bbscale_band1 = DEFAULT_DPD_BBSCALE_5500;
        sh->rf_table.lt_5g_config.bbscale_band1 = DEFAULT_DPD_BBSCALE_5500;
        sh->rf_table.ht_5g_config.bbscale_band1 = DEFAULT_DPD_BBSCALE_5500;
    }

    // set band 5700
    if (sh->flash_config.exist) {
        sh->rf_table.rt_5g_config.bbscale_band2 = sh->flash_config.rt_config.a_band_gain[2];
        sh->rf_table.lt_5g_config.bbscale_band2 = sh->flash_config.lt_config.a_band_gain[2];
        sh->rf_table.ht_5g_config.bbscale_band2 = sh->flash_config.ht_config.a_band_gain[2];
    } else if (sh->efuse_bitmap & BIT(EFUSE_TX_POWER_INDEX_2)) {
        value = ((sh->cfg.tx_power_index_2 & 0x0f) >> 0);
        sh->rf_table.rt_5g_config.bbscale_band2 = band_gain_tbl[value];
        sh->rf_table.lt_5g_config.bbscale_band2 = band_gain_tbl[value];
        sh->rf_table.ht_5g_config.bbscale_band2 = band_gain_tbl[value];
    } else {
        sh->rf_table.rt_5g_config.bbscale_band2 = DEFAULT_DPD_BBSCALE_5700;
        sh->rf_table.lt_5g_config.bbscale_band2 = DEFAULT_DPD_BBSCALE_5700;
        sh->rf_table.ht_5g_config.bbscale_band2 = DEFAULT_DPD_BBSCALE_5700;
    }

    // set band 5900
    if (sh->flash_config.exist) {
        sh->rf_table.rt_5g_config.bbscale_band3 = sh->flash_config.rt_config.a_band_gain[3];
        sh->rf_table.lt_5g_config.bbscale_band3 = sh->flash_config.lt_config.a_band_gain[3];
        sh->rf_table.ht_5g_config.bbscale_band3 = sh->flash_config.ht_config.a_band_gain[3];
    } else if (sh->efuse_bitmap & BIT(EFUSE_TX_POWER_INDEX_2)) {
        value = ((sh->cfg.tx_power_index_2 & 0xf0) >> 4);
        sh->rf_table.rt_5g_config.bbscale_band3 = band_gain_tbl[value];
        sh->rf_table.lt_5g_config.bbscale_band3 = band_gain_tbl[value];
        sh->rf_table.ht_5g_config.bbscale_band3 = band_gain_tbl[value];
    } else {
        sh->rf_table.rt_5g_config.bbscale_band3 = DEFAULT_DPD_BBSCALE_5900;
        sh->rf_table.lt_5g_config.bbscale_band3 = DEFAULT_DPD_BBSCALE_5900;
        sh->rf_table.ht_5g_config.bbscale_band3 = DEFAULT_DPD_BBSCALE_5900;
    }
}

static void ssv6006_turismoC_update_rf_table(struct ssv_hw *sh)
{
    ssv6006_turismoC_chg_xtal_freq_offset(sh);
    ssv6006_turismoC_chg_bbscale(sh);
    ssv6006_turismoC_chg_dpd_bbscale(sh);
}

void ssv6006c_update_edcca(struct ssv_hw *sh, bool val)
{
    if ((sh->sc->hw_chan_type == NL80211_CHAN_HT20)||
            (sh->sc->hw_chan_type == NL80211_CHAN_NO_HT)) {
        if (val) {
            SET_RG_AGC_THRESHOLD(EDCCA_HIGH);
        } else {
            SET_RG_AGC_THRESHOLD(EDCCA_LOW);
        }
    } else {
        if (val) {
            SET_RG_AGC_THRESHOLD(EDCCA_HIGH_HT40);
        } else {
            SET_RG_AGC_THRESHOLD(EDCCA_LOW_HT40);
        }
    }
}

void ssv6006_update_external_pa_phy_setting(struct ssv_hw *sh)
{
    u8 tx_gain;
    int i;

    //change phy settings
    SSV6XXX_SET_HW_TABLE(sh, ssv6006_turismoC_rf_hp_patch);
    SSV6XXX_SET_HW_TABLE(sh, ssv6006_turismoC_phy_hp_patch);

    ssv6006_turismoC_init_cali(sh);
    // TODO, phy offload, how to restore dpd
    //turismo_restore_dpd_bbscale(sh, dpd);
    SET_RG_TXGAIN_PHYCTRL(0);
    // disable padpd
    sh->cfg.disable_dpd = 0x1f;

    for (i = 0; i < PADPDBAND; i++){
        sh->sc->dpd.dpd_disable[i] = true;
    }

    //
    printk(" external pa setting 0x%x \n", sh-> cfg.external_pa);

    tx_gain = sh-> cfg.external_pa & EX_PA_TX_GAIN_OFFSET_MASK;
    if (tx_gain != GET_RG_TX_GAIN){
        SET_RG_TX_GAIN(tx_gain);
    }
    sh->default_txgain[BAND_2G] = tx_gain;
    printk("set tx gain %x to 2G, read back %x\n", tx_gain, GET_RG_TX_GAIN );

    if (sh->cfg.hw_caps & SSV6200_HW_CAP_5GHZ){
        //tx_gain = PAPDP_GAIN_SETTING;
        tx_gain = sh-> cfg.external_pa & EX_PA_TXGAIN_F0_MASK;
        if (tx_gain != GET_RG_5G_TX_GAIN_F0) {
            SET_RG_5G_TX_GAIN_F0(tx_gain);
        }
        sh->default_txgain[BAND_5100] = tx_gain;
        printk("set tx gain %x to F0, read back %x\n", tx_gain, GET_RG_5G_TX_GAIN_F0 );

        //tx_gain = PAPDP_GAIN_SETTING;
        tx_gain = ((sh-> cfg.external_pa_5g & EX_PA_TXGAIN_F1_MASK) >> EX_PA_TXGAIN_F1_SFT);
        if (tx_gain != GET_RG_5G_TX_GAIN_F1){
            SET_RG_5G_TX_GAIN_F1(tx_gain);
        }
        sh->default_txgain[BAND_5500] = tx_gain;
        printk("set tx gain %x to F1, read back %x\n", tx_gain, GET_RG_5G_TX_GAIN_F1 );

        //tx_gain = PAPDP_GAIN_SETTING_F2;
        tx_gain = ((sh-> cfg.external_pa_5g & EX_PA_TXGAIN_F2_MASK)>> EX_PA_TXGAIN_F2_SFT) ;
        if (tx_gain != GET_RG_5G_TX_GAIN_F2){
            SET_RG_5G_TX_GAIN_F2(tx_gain);
        }
        sh->default_txgain[BAND_5700] = tx_gain;
        printk("set tx gain %x to F2, read back %x\n", tx_gain, GET_RG_5G_TX_GAIN_F2 );

        //tx_gain = PAPDP_GAIN_SETTING;
        tx_gain = ((sh-> cfg.external_pa_5g  & EX_PA_TXGAIN_F3_MASK)>> EX_PA_TXGAIN_F3_SFT) ;
        if (tx_gain != GET_RG_5G_TX_GAIN_F3){
            SET_RG_5G_TX_GAIN_F3(tx_gain);
        }
        sh->default_txgain[BAND_5900] = tx_gain;
        printk("set tx gain %x to F3, read back %x\n", tx_gain, GET_RG_5G_TX_GAIN_F3 );
    }

}

void ssv_attach_ssv6006_turismoC_BBRF(struct ssv_hal_ops *hal_ops)
{
    hal_ops->load_phy_table = ssv6006_turismoC_load_phy_table;
    hal_ops->get_phy_table_size = ssv6006_turismoC_get_phy_table_size;
    hal_ops->load_rf_table = ssv6006_turismoC_load_rf_table;
    hal_ops->get_rf_table_size = ssv6006_turismoC_get_rf_table_size;
    hal_ops->init_pll = ssv6006_turismoC_init_PLL;
    hal_ops->set_channel = ssv6006_turismoC_set_channel;
    hal_ops->set_pll_phy_rf = ssv6006_turismoC_set_pll_phy_rf;
    hal_ops->set_rf_enable = ssv6006_turismoC_set_rf_enable;

    hal_ops->dump_phy_reg = ssv6006_turismoC_dump_phy_reg;
    hal_ops->dump_rf_reg = ssv6006_turismoC_dump_rf_reg;
    hal_ops->support_iqk_cmd = ssv6006_turismoC_support_iqk_cmd;
    hal_ops->cmd_txgen = ssv6006_cmd_turismoC_txgen;
    hal_ops->cmd_rf = ssv6006_cmd_turismoC_rf;

    hal_ops->update_rf_table = ssv6006_turismoC_update_rf_table;
}
#endif
