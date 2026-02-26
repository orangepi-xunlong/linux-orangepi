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

#if ((defined SSV_SUPPORT_SSV6020))
#include <linux/etherdevice.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/platform_device.h>
#include <smac/dev.h>
#include "ssv6020_cfg.h"
#include "ssv6020_mac.h"
#include "ssv6020/ssv6020_reg.h"
#include "ssv6020/ssv6020_aux.h"
#include "turismoE_ext_dpll_init.c"
#include <smac/wow.h>
#include <smac/efuse.h>

#include <hal.h>
#include "ssv6020_priv.h"
#include "ssv6020_priv_normal.h"
#include <smac/ssv_skb.h>
#include <ssvdevice/ssv_cmd.h>
#include <hci/hctrl.h>
#include <hwif/usb/usb.h>
#include <linux_80211.h>


extern void ssv_attach_ssv6020_turismoE_BBRF(struct ssv_hal_ops *hal_ops);
extern void ssv_attach_ssv6020_turismoE_ext_BBRF(struct ssv_hal_ops *hal_ops);
extern void ssv_attach_ssv6020_turismoE_fpga_BBRF(struct ssv_hal_ops *hal_ops);

static u32 ssv6020_alloc_pbuf(struct ssv_softc *sc, int size, int type);
static void ssv6020_write_key_to_hw(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv,
        void *key, int wsid, int key_idx, enum SSV6XXX_WSID_SEC key_type);
static int ssv6020_set_macaddr(struct ssv_hw *sh, int vif_idx);
static int ssv6020_set_bssid(struct ssv_hw *sh, u8 *bssid, int vif_idx);
static void ssv6020_write_hw_group_keyidx(struct ssv_hw *sh, struct ssv_vif_priv_data *vif_priv, int key_idx);
static int ssv6020_reset_cpu(struct ssv_hw *sh);

static const u32 reg_wsid[] = {ADR_WSID0, ADR_WSID1, ADR_WSID2, ADR_WSID3,
    ADR_WSID4, ADR_WSID5, ADR_WSID6, ADR_WSID7, ADR_WSID8};

static const ssv_cabrio_reg ssv6020_mac_ini_table[]=
{
    //-----------------------------------------------------------------------------------------------------------------------------------------
    /* Set wmm parameter to EDCA Q4
       (use to send mgmt frame/null data frame in STA mode and broadcast frame in AP mode) */
    //{ADR_TXQ4_MTX_Q_AIFSN,	           0xffff2101},
    //---
    //HCI module config
    //keep it in host
    //HCI-enable 4 bits
    //RX to Host(bit2)
    //AUTO_SEQNO enable(bit3)
    //Fill tx queue info in rx pacet(Bit 25)
    //tx rx packet debug counter enable(bit28)
    //TX on_demand interrupt control between allocate size and transmit data mismatch
    {ADR_CONTROL,                      0x12000006},
    //---
    //RX module config
    //(bit 0)                       Enable hardware timestamp for TSF
    //(bit8~bit15) value(28)  Time stamp write location
    {ADR_RX_TIME_STAMP_CFG,            ((28 << MRX_STP_OFST_SFT) | 0x01)},
    //-----------------------------------------------------------------------------------------------------------------------------------------
    //MAC config
    {ADR_GLBLE_SET,                    (0 << OP_MODE_SFT)  |                          /* STA mode by default */
        (0 << SNIFFER_MODE_SFT) |                      /* disable sniffer mode */
            (1 << DUP_FLT_SFT) |                           /* Enable duplicate detection */
            (SSV6020_TX_PKT_RSVD_SETTING << TX_PKT_RSVD_SFT) |
            ((u32)(RXPB_OFFSET) << PB_OFFSET_SFT) },      /* set rx packet buffer offset */

    /**
     * Disable tx/rx ether trap table.
     */
    {ADR_TX_ETHER_TYPE_0,              0x00000000},
    {ADR_TX_ETHER_TYPE_1,              0x00000000},
    {ADR_RX_ETHER_TYPE_0,              0x00000000},
    {ADR_RX_ETHER_TYPE_1,              0x00000000},
    /**
     * Set reason trap to discard frames.
     */
    {ADR_REASON_TRAP0,                 0x7FBC7F87},
    {ADR_REASON_TRAP1,                 0x0000033F},

    // trap HW_ID not match to CPU for smartlink
    {ADR_TRAP_HW_ID,                   M_ENG_CPU},   /* Trap to CPU */


    /**
     * Reset all wsid table entry to invalid.
     */
    {ADR_WSID0,                        0x00000000},
    {ADR_WSID1,                        0x00000000},
    {ADR_WSID2,                        0x00000000},
    {ADR_WSID3,                        0x00000000},
    {ADR_WSID4,                        0x00000000},
    {ADR_WSID5,                        0x00000000},
    {ADR_WSID6,                        0x00000000},
    {ADR_WSID7,                        0x00000000},
    {ADR_WSID8,                        0x00000000},
    //-----------------------------------------------------------------------------------------------------------------------------------------
    //SDIO interrupt
    /*
     * Set tx interrupt threshold for EACA0 ~ EACA3 queue & low threshold
     */
    {ADR_MASK_TYPHOST_INT_MAP,         0xffff7fff}, //bit 15 for int 15 map group
    {ADR_MASK_TYPHOST_INT_MAP_15,      0xff0fffff}, //bit 20,21,22,23
    /* update rate for ack/cts response */
    /* set B mode ack/cts rate 1M */
    {ADR_MTX_RESPFRM_RATE_TABLE_01, 0x0000},
    {ADR_MTX_RESPFRM_RATE_TABLE_02, 0x0000},
    {ADR_MTX_RESPFRM_RATE_TABLE_03, 0x0002},
    {ADR_MTX_RESPFRM_RATE_TABLE_11, 0x0000},
    {ADR_MTX_RESPFRM_RATE_TABLE_12, 0x0000},
    {ADR_MTX_RESPFRM_RATE_TABLE_13, 0x0012},
    /* set G mode ack/cts rate */
    //G_12M response 6M control rate
    {ADR_MTX_RESPFRM_RATE_TABLE_82, 0x9090},
    //G_24M response 12M control rate
    {ADR_MTX_RESPFRM_RATE_TABLE_84, 0x9292},
    /* set N mode ack/cts rate */
    //MCS1(LGI) response 6M control rate
    {ADR_MTX_RESPFRM_RATE_TABLE_C1, 0x9090},
    //MCS3(LGI) response 12M control rate
    {ADR_MTX_RESPFRM_RATE_TABLE_C3, 0x9292},
    //MCS1(SGI) response 6M control rate
    {ADR_MTX_RESPFRM_RATE_TABLE_D1, 0x9090},
    //MCS3(SGI) response 12M control rate
    {ADR_MTX_RESPFRM_RATE_TABLE_D3, 0x9292},
    //SET default BA control setting
    {ADR_BA_CTRL, 0x9},

};

static void ssv6020_sec_lut_setting(struct ssv_hw *sh)
{
    u8 lut_sel = 1;

#if 0 //Ian.Wu. It doesn't need on AMPDU 1.3
    sh->sc->ccmp_h_sel = 1;

    if (sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_TX) {
        printk("Support AMPDU TX mode, ccmp header source must from SW\n");
        sh->sc->ccmp_h_sel = 1;
    }

    printk("CCMP header source from %s, Security LUT version V%d\n", (sh->sc->ccmp_h_sel == 1) ?  "SW" : "LUT", lut_sel+1);
    //CCMP header source select.
    //bit22 set 0: CCMP header source from LUT (Frame to Security Engine: 80211_H + Payload)
    //          1: CCMP header source from SW  (Frame to Security Engine: 80211_H + CCMP_H + Payload + MIC)
    SMAC_REG_SET_BITS(sh, ADR_GLBLE_SET, (sh->sc->ccmp_h_sel << 22), 0x400000);
#endif
    //LUT version selset.
    //bit23 set 0: V1 (Cabrio design)
    //          1: V2 (Turismo design)
    SMAC_REG_SET_BITS(sh, ADR_GLBLE_SET, (lut_sel << 23), 0x800000);
}

static void ssv6020_set_page_id(struct ssv_hw *sh)
{
    SMAC_REG_SET_BITS(sh, ADR_TRX_ID_THRESHOLD,
            ((sh->tx_info.tx_id_threshold << TX_ID_THOLD_SFT)|
             (sh->rx_info.rx_id_threshold << RX_ID_THOLD_SFT)),
            (TX_ID_THOLD_MSK | RX_ID_THOLD_MSK));

    SMAC_REG_SET_BITS(sh, ADR_ID_LEN_THREADSHOLD1,
            ((sh->tx_info.tx_page_threshold << ID_TX_LEN_THOLD_SFT)|
             (sh->rx_info.rx_page_threshold << ID_RX_LEN_THOLD_SFT)),
            (ID_TX_LEN_THOLD_MSK | ID_RX_LEN_THOLD_MSK));
}

static void ssv6020_update_page_id(struct ssv_hw *sh)
{
    HAL_INIT_TX_CFG(sh);
    HAL_INIT_RX_CFG(sh);
    ssv6020_set_page_id(sh);
}

static int ssv6020_init_mac(struct ssv_hw *sh)
{
    struct ssv_softc *sc=sh->sc;
    int ret = 0, i = 0;
    u32 regval;
    u8 null_address[6]={0};

    // load mac init_table
    ret = HAL_WRITE_MAC_INI(sh);

    if (ret)
        goto exit;

    //Enable TSF to be a hw jiffies(add one by us)

    SMAC_REG_SET_BITS(sh, ADR_MTX_BCN_EN_MISC,
            1 << MTX_TSF_TIMER_EN_SFT, MTX_TSF_TIMER_EN_MSK);

    //RX module config
    SMAC_REG_WRITE(sh, ADR_HCI_TX_RX_INFO_SIZE,
            ((u32)(TXPB_OFFSET) << TX_PBOFFSET_SFT) |    /* packet buffer offset for tx */
            ((u32)(sh->tx_desc_len)  << TX_INFO_SIZE_SFT) |   /* tx_info_size send (bytes) (need word boundry, times of 4bytes) */
            ((u32)(sh->rx_desc_len)  << RX_INFO_SIZE_SFT) |   /* rx_info_size send (bytes) (need word boundry, times of 4bytes) */
            ((u32)(sh->rx_pinfo_pad) << RX_LAST_PHY_SIZE_SFT )    /* rx_last_phy_size send (bytes) */
            );

    //-----------------------------------------------------------------------------------------------------------------------------------------
    //MMU[decide packet buffer no.]
    /* Setting MMU to 256 pages */
    // removed, turismo page size is fixed. remove this registers.
    //SMAC_REG_SET_BITS(sh, ADR_MMU_CTRL, (255<<MMU_SHARE_MCU_SFT), MMU_SHARE_MCU_MSK); // not exist for new mac

    //-----------------------------------------------------------------------------------------------------------------------------------------
    //Dual interface

    // Freddie ToDo:
    //   1. Check against HW capability. Only SSV6200 ECO version support RX MAC address filter mask.
    //   2. Enable filter only for the second MA address.
    // @C600011C
    // bit0: default 1. 0 to enable don't care bit1 of RX RA, i.e. bit 1 of first byte.
    // bit1: default 1. 0 to enable don't care bit40 of RX RA, i.e. bit0 of last byte.
    // bit2: default 1. 0 to enable don't care bit41 of RX RA. i.e. bit1 of last byte.
    // bit3: default 1. 0 to accept ToDS in non-AP mode and non-ToDS in AP mode.
    /* Setting RX mask to allow muti-MAC*/
    SMAC_REG_READ(sh,ADR_MRX_WATCH_DOG, &regval);
    regval &= 0xfffffff0;
    SMAC_REG_WRITE(sh,ADR_MRX_WATCH_DOG, regval);


    //-----------------------------------------------------------------------------------------------------------------------------------------
    //Setting Tx resource low
    SET_TX_PAGE_LIMIT(sh->tx_info.tx_lowthreshold_page_trigger);
    SET_TX_COUNT_LIMIT(sh->tx_info.tx_lowthreshold_id_trigger);
    SET_TX_LIMIT_INT_EN(1);

    /**
     * Set ssv6200 mac address and set default BSSID. In hardware reset,
     * we the BSSID to 00:00:00:00:00:00.
     */
    // Freddie ToDo: Set MAC addresss when primary interface is being added.
    ssv6020_set_macaddr(sh, 0);
    for (i=0; i<SSV6020_NUM_HW_BSSID; i++)
        ssv6020_set_bssid(sh, null_address, i);

    /**
     * Set Tx/Rx processing flows.
     */
    if (USE_MAC80211_RX(sh)){
        HAL_SET_RX_FLOW(sh, RX_DATA_FLOW, RX_CPU_HCI);
    } else {
    #ifdef CONFIG_RX2HOST_DIRECTLY
        HAL_SET_RX_FLOW(sh, RX_DATA_FLOW, RX_CIPHER_MIC_HCI);
    #else
        HAL_SET_RX_FLOW(sh, RX_DATA_FLOW, RX_CIPHER_MIC_CPU_HCI);
    #endif
    }
    HAL_SET_RX_FLOW(sh, RX_MGMT_FLOW, RX_CPU_HCI);
    HAL_SET_RX_FLOW(sh, RX_CTRL_FLOW, RX_CPU_HCI);

    HAL_SET_REPLAY_IGNORE(sh, 1); //?? move to init_mac

    /**
     * Set ssv6200 mac decision table for hardware. The table
     * selection is according to the type of wireless interface:
     * AP & STA mode.
     */
    HAL_UPDATE_DECISION_TABLE(sc);

    SMAC_REG_SET_BITS(sc->sh, ADR_GLBLE_SET, SSV6XXX_OPMODE_STA, OP_MODE_MSK);
    //Enale security LUT V2
    ssv6020_sec_lut_setting(sh);

    /* Set Rate Report HWID */
    SMAC_REG_SET_BITS(sc->sh, ADR_MTX_RATERPT, M_ENG_CPU, MTX_RATERPT_HWID_MSK);
    /* Set rate report length */
    SET_PEERPS_REJECT_ENABLE(1);

    /* set ba window size according to max rx aggr size*/
    SMAC_REG_WRITE(sh, ADR_AMPDU_SCOREBOAD_SIZE, sh->cfg.max_rx_aggr_size);

    SET_MACTX_OPT_DONTFILL_RETRY_FOR_AMPDU(0);
    SET_MTX_AIFSN_IGNORE_PHY_CCA(0x1);
exit:
    return ret;

}

static void ssv6020_reset_sysplf(struct ssv_hw *sh)
{
    SMAC_SYSPLF_RESET(sh, ADR_BRG_SW_RST, (1 << PLF_SW_RST_SFT));
}


static int ssv6020_init_hw_sec_phy_table(struct ssv_softc *sc)
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN);
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_INIT;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_INIT);
    host_cmd->len = HOST_CMD_HDR_LEN;

    retval = HCI_SEND_CMD(sc->sh, skb);
    return retval;
}


static int ssv6020_write_mac_ini(struct ssv_hw *sh)
{
    return SSV6XXX_SET_HW_TABLE(sh, ssv6020_mac_ini_table);
}


static int ssv6020_set_rx_flow(struct ssv_hw *sh, enum ssv_rx_flow type, u32 rxflow)
{

    switch (type){
        case RX_DATA_FLOW:
            return SMAC_REG_WRITE(sh, ADR_RX_FLOW_DATA, rxflow);

        case RX_MGMT_FLOW:
            return SMAC_REG_WRITE(sh, ADR_RX_FLOW_MNG, rxflow);

        case RX_CTRL_FLOW:
            return SMAC_REG_WRITE(sh, ADR_RX_FLOW_CTRL, rxflow);
        default:
            return 1;
    }
}

static int ssv6020_set_rx_ctrl_flow(struct ssv_hw *sh)
{
    return ssv6020_set_rx_flow(sh, ADR_RX_FLOW_CTRL, RX_CPU_HCI);
}

static int ssv6020_set_macaddr(struct ssv_hw *sh, int vif_idx)
{
    int  ret = 0;

    switch (vif_idx) {
        case 0: //interface 0
            ret = SMAC_REG_WRITE(sh, ADR_STA_MAC_0, *((u32 *)&sh->cfg.maddr[0][0]));
            if (!ret)
                ret = SMAC_REG_WRITE(sh, ADR_STA_MAC_1, *((u32 *)&sh->cfg.maddr[0][4]));
            break;
        case 1: //interface 1
            ret = SMAC_REG_WRITE(sh, ADR_STA_MAC1_0, *((u32 *)&sh->cfg.maddr[1][0]));
            if (!ret)
                ret = SMAC_REG_WRITE(sh, ADR_STA_MAC1_1, *((u32 *)&sh->cfg.maddr[1][4]));
            break;
        default:
            printk("Does not support set MAC address to HW for VIF %d\n", vif_idx);
            ret = -1;
            break;
    }
    return ret;
}

static int ssv6020_set_macaddr_2(struct ssv_hw *sh, int vif_idx, u8 *macaddr)
{
    int  ret = 0;

    switch (vif_idx) {
        case 0: //interface 0
            ret = SMAC_REG_WRITE(sh, ADR_STA_MAC_0, *((u32 *)macaddr));
            if (!ret)
                ret = SMAC_REG_WRITE(sh, ADR_STA_MAC_1, *((u32 *)(macaddr + 4)));
            break;
        case 1: //interface 1
            ret = SMAC_REG_WRITE(sh, ADR_STA_MAC1_0, *((u32 *)macaddr));
            if (!ret)
                ret = SMAC_REG_WRITE(sh, ADR_STA_MAC1_1, *((u32 *)(macaddr + 4)));
            break;
        default:
            printk("Does not support set MAC address to HW for VIF %d\n", vif_idx);
            ret = -1;
            break;
    }
    return ret;
}

static int ssv6020_set_bssid(struct ssv_hw *sh, u8 *bssid, int vif_idx)
{
    int  ret = 0;
    struct ssv_softc *sc = sh->sc;

    switch (vif_idx) {
        case 0: //interface 0
            memcpy(sc->bssid[vif_idx], bssid, 6);
            ret = SMAC_REG_WRITE(sh, ADR_BSSID_0, *((u32 *)&sc->bssid[0][0]));
            if (!ret)
                ret = SMAC_REG_WRITE(sh, ADR_BSSID_1, *((u32 *)&sc->bssid[0][4]));
            break;
        case 1: //interface 1
            memcpy(sc->bssid[vif_idx], bssid, 6);
            ret = SMAC_REG_WRITE(sh, ADR_BSSID1_0, *((u32 *)&sc->bssid[1][0]));
            if (!ret)
                ret = SMAC_REG_WRITE(sh, ADR_BSSID1_1, *((u32 *)&sc->bssid[1][4]));
            break;
        default:
            printk("Does not support set BSSID to HW for VIF %d\n", vif_idx);
            ret = -1;
            break;
    }
    return ret;
}

static u64 ssv6020_get_ic_time_tag(struct ssv_hw *sh)
{
    u32 regval;

    SMAC_REG_READ(sh, ADR_CHIP_DATE_YYYYMMDD, &regval);
    sh->chip_tag = ((u64)regval<<32);
    SMAC_REG_READ(sh, ADR_CHIP_DATE_00HHMMSS, &regval);
    sh->chip_tag |= (regval);

    return sh->chip_tag;
}

static void ssv6020_get_chip_id(struct ssv_hw *sh)
{
    char *chip_id = sh->chip_id;
    u32 regval;

    //CHIP ID
    SMAC_REG_READ(sh, ADR_CHIP_ID_3, &regval);
    *((u32 *)&chip_id[0]) = __be32_to_cpu(regval);
    SMAC_REG_READ(sh, ADR_CHIP_ID_2, &regval);
    *((u32 *)&chip_id[4]) = __be32_to_cpu(regval);
    SMAC_REG_READ(sh, ADR_CHIP_ID_1, &regval);
    *((u32 *)&chip_id[8]) = __be32_to_cpu(regval);
    SMAC_REG_READ(sh, ADR_CHIP_ID_0, &regval);
    *((u32 *)&chip_id[12]) = __be32_to_cpu(regval);
    chip_id[12+sizeof(u32)] = 0;
}

static void ssv6020_set_mrx_mode(struct ssv_hw *sh, u32 val)
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    int retval = 0;

    skb = ssv_skb_alloc(sh->sc, HOST_CMD_HDR_LEN);
    if (skb == NULL) {
        printk("%s:_skb_alloc fail!!!\n", __func__);
        return;
    }

    skb_put(skb, HOST_CMD_HDR_LEN);
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->sub_h_cmd = ((val == MRX_MODE_NORMAL) ? SSV6XXX_MRX_NORMAL : SSV6XXX_MRX_PROMISCUOUS);
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_MRX_MODE;
    host_cmd->len = skb->len;
    retval = HCI_SEND_CMD(sh, skb);
    if (retval)
        printk("%s(): Fail to send mrx mode\n", __FUNCTION__);

    SMAC_REG_WRITE(sh, ADR_MRX_FLT_TB13, val);
}

static void ssv6020_get_mrx_mode(struct ssv_hw *sh, u32 *val)
{
    SMAC_REG_READ(sh, ADR_MRX_FLT_TB13, val);
}

static u32 _ssv6020_get_sec_key_tbl_addr(struct ssv_softc *sc)
{
    struct ssv_hw *sh = sc->sh;
    u32     word_data = 0;
    u32     sec_key_tbl = 0;

    /* Get the packet ID of secure table. */
    SMAC_REG_READ(sh, ADR_SCRT_SET, &word_data);
    sec_key_tbl = PBUF_MapIDtoPkt((word_data&SCRT_PKT_ID_MSK)>>SCRT_PKT_ID_SFT);

    return sec_key_tbl;
}

static void ssv6020_save_hw_status(struct ssv_softc *sc)
{
    struct ssv_hw *sh = sc->sh;
    int     i = 0;
    int     address = 0;
    u32     word_data = 0;
    u32     sec_key_tbl = 0;

    /* Get the packet ID of secure table. */
    SMAC_REG_READ(sh, ADR_SCRT_SET, &word_data);
    sec_key_tbl = _ssv6020_get_sec_key_tbl_addr(sc);

    /* security table */
    for (i = 0; i < sizeof(struct ssv6020_hw_sec); i += 4) {
        address = sec_key_tbl + i;
        SMAC_REG_READ(sh, address, &word_data);
        *(u32 *)(&sh->hw_sec_key_data[i]) = word_data;
    }
}

static void ssv6020_restore_hw_config(struct ssv_softc *sc)
{
    struct ssv_hw *sh = sc->sh;
    int     i = 0;
    int     address = 0;
    u32     word_data = 0;
    u32     sec_key_tbl = 0;

    /* Get the packet ID of secure table. */
    SMAC_REG_READ(sh, ADR_SCRT_SET, &word_data);
    sec_key_tbl = PBUF_BASE_ADDR|(((word_data&SCRT_PKT_ID_MSK)>>SCRT_PKT_ID_SFT)<<16);

    /* security table */
    sec_key_tbl = _ssv6020_get_sec_key_tbl_addr(sc);
    for (i = 0; i < sizeof(struct ssv6020_hw_sec); i += 4) {
        address = sec_key_tbl + i;
        word_data = *(u32 *)(&sh->hw_sec_key_data[i]);
        SMAC_REG_WRITE(sh, address, word_data);
    }
    memset((void *)&sh->hw_sec_key_data[0], 0, sizeof(sh->hw_sec_key_data));
}

static void ssv6020_set_hw_wsid(struct ssv_softc *sc, struct ieee80211_vif *vif,
        struct ieee80211_sta *sta, int wsid)
{
    struct ssv_sta_priv_data *sta_priv_dat=(struct ssv_sta_priv_data *)sta->drv_priv;
    struct ssv_sta_info *sta_info;/* sta_info is already protected by ssv6200_sta_add(). */

    sta_info = &sc->sta_info[wsid];/* sta_info is already protected by ssv6200_sta_add(). */
    /**
     * Allocate a free hardware WSID for the added STA. If no more
     * hardware entry present, set hw_wsid=-1 for
     * struct ssv_sta_info.
     */
    if (sta_priv_dat->sta_idx < SSV6020_NUM_HW_STA)
    {
        u32 reg_peer_mac0[] = {ADR_PEER_MAC0_0, ADR_PEER_MAC1_0, ADR_PEER_MAC2_0, ADR_PEER_MAC3_0,
            ADR_PEER_MAC4_0, ADR_PEER_MAC5_0, ADR_PEER_MAC6_0, ADR_PEER_MAC7_0, ADR_PEER_MAC8_0};
        u32 reg_peer_mac1[] = {ADR_PEER_MAC0_1, ADR_PEER_MAC1_1, ADR_PEER_MAC2_1, ADR_PEER_MAC3_1,
            ADR_PEER_MAC4_1, ADR_PEER_MAC5_1, ADR_PEER_MAC6_1, ADR_PEER_MAC7_1, ADR_PEER_MAC8_1};

        /* Add STA into hardware for hardware offload */
        SMAC_REG_WRITE(sc->sh, reg_peer_mac0[wsid], *((u32 *)&sta->addr[0]));
        SMAC_REG_WRITE(sc->sh, reg_peer_mac1[wsid], *((u32 *)&sta->addr[4]));

        /* Valid this wsid entity */
        SMAC_REG_WRITE(sc->sh, reg_wsid[wsid], 1);

        sta_info->hw_wsid = sta_priv_dat->sta_idx;/* sta_info is already protected by ssv6200_sta_add(). */

    }
}

static void ssv6020_del_hw_wsid(struct ssv_softc *sc, int hw_wsid)
{
    if ((hw_wsid != -1) && (hw_wsid < SSV6020_NUM_HW_STA)) {
        u32 reg_peer_mac0[] = {ADR_PEER_MAC0_0, ADR_PEER_MAC1_0, ADR_PEER_MAC2_0, ADR_PEER_MAC3_0,
            ADR_PEER_MAC4_0, ADR_PEER_MAC5_0, ADR_PEER_MAC6_0, ADR_PEER_MAC7_0, ADR_PEER_MAC8_0};
        u32 reg_peer_mac1[] = {ADR_PEER_MAC0_1, ADR_PEER_MAC1_1, ADR_PEER_MAC2_1, ADR_PEER_MAC3_1,
            ADR_PEER_MAC4_1, ADR_PEER_MAC5_1, ADR_PEER_MAC6_1, ADR_PEER_MAC7_1, ADR_PEER_MAC8_1};
        SMAC_REG_WRITE(sc->sh, reg_wsid[hw_wsid], 0x00);
        /* Delete STA in hardware */
        SMAC_REG_WRITE(sc->sh, reg_peer_mac0[hw_wsid], 0x0);
        SMAC_REG_WRITE(sc->sh, reg_peer_mac1[hw_wsid], 0x0);
    }
}

static int _ssv6020_write_pairwise_keyidx_to_hw(struct ssv_hw *sh, int key_idx, int wsid)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv_sec_param *ptr = NULL;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_PAIRWIRE_KEY_IDX;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_PAIRWIRE_KEY_IDX);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param);

    ptr = (struct ssv_sec_param *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_sec_param));
    ptr->wsid_idx = wsid;
    ptr->key_idx = key_idx;

    printk("%s, wsid_idx %d, key_idx %d\n", __FUNCTION__, ptr->wsid_idx, ptr->key_idx);
    retval = HCI_SEND_CMD(sc->sh, skb);
    return retval;
}

static int _ssv6020_write_group_keyidx_to_hw(struct ssv_hw *sh, int key_idx, int vif_idx)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv_sec_param *ptr = NULL;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_GROUP_KEY_IDX;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_GROUP_KEY_IDX);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param);

    ptr = (struct ssv_sec_param *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_sec_param));
    ptr->vif_idx = vif_idx;
    ptr->key_idx = key_idx;

    printk("%s, vif_idx %d, ptr->key_idx %d\n", __FUNCTION__, vif_idx, key_idx);
    retval = HCI_SEND_CMD(sc->sh, skb);
    return retval;
}

static int _ssv6020_set_pairwise_cipher_type(struct ssv_hw *sh, u8 cipher, u8 wsid)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv_sec_param *ptr = NULL;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_PAIRWIRE_CIPHER_TYPE;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_PAIRWIRE_CIPHER_TYPE);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param);

    ptr = (struct ssv_sec_param *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_sec_param));
    ptr->wsid_idx = wsid;
    ptr->cipher = cipher;

    printk("%s, wsid_idx %d, cipher %d\n", __FUNCTION__, ptr->wsid_idx, ptr->cipher);
    retval = HCI_SEND_CMD(sc->sh, skb);
    return retval;
}

static int _ssv6020_set_group_cipher_type(struct ssv_hw *sh, u8 cipher, u8 vif_idx)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv_sec_param *ptr = NULL;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_GROUP_CIPHER_TYPE;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_GROUP_CIPHER_TYPE);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param);

    ptr = (struct ssv_sec_param *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_sec_param));
    ptr->vif_idx = vif_idx;
    ptr->cipher = cipher;

    printk("%s, vif_idx %d, cipher %d\n", __FUNCTION__, vif_idx, cipher);
    retval = HCI_SEND_CMD(sc->sh, skb);
    return retval;
}

static int _ssv6020_write_pairwise_key_to_hw(struct ssv_hw *sh,
        u8 wsid, u8 algorithm, u8 key_idx, const u8 *key, int key_len)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv_sec_param *ptr = NULL;
    struct ssv6020_hw_sec *p_hw_sec = (struct ssv6020_hw_sec *)&sh->hw_sec_key_data[0];
    u32    pn_offset  = 0;
    u32    word_data = 0;
    u32    sec_key_tbl = 0;
    int    address = 0;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_PAIRWIRE_KEY;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_PAIRWIRE_KEY);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param);

    ptr = (struct ssv_sec_param *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_sec_param));
    ptr->wsid_idx = wsid;
    ptr->key_idx = key_idx;
    ptr->key_len = key_len;
    ptr->alg = algorithm;
    memcpy((u8 *)ptr->key, (u8 *)key, key_len);

    printk("%s, wsid_idx %d, key_idx %d, key_len %d, alg %d\n",
            __FUNCTION__, ptr->wsid_idx, ptr->key_idx, ptr->key_len, ptr->alg);

    retval = HCI_SEND_CMD(sc->sh, skb);
    //Restore PN value if need.
    if((p_hw_sec != NULL) && (p_hw_sec->sta_key[wsid].pair_cipher_type != SECURITY_NONE))
    {
        sec_key_tbl = _ssv6020_get_sec_key_tbl_addr(sc);
        pn_offset = offsetof(struct ssv6020_hw_sec, sta_key)+sizeof(struct ssv6020_hw_sta_key)*wsid+offsetof(struct ssv6020_hw_sta_key, tx_pn_l);
        address = sec_key_tbl + pn_offset;
        word_data = p_hw_sec->sta_key[wsid].tx_pn_l;
        SMAC_REG_WRITE(sh, address, word_data);
        address += 4; //pointer to tx_pn_h
        word_data = p_hw_sec->sta_key[wsid].tx_pn_h;
        SMAC_REG_WRITE(sh, address, word_data);
        //Clear the stored key setting after restore.
        memset((void *)&(p_hw_sec->sta_key[wsid]), 0, sizeof(struct ssv6020_hw_sta_key));
    }
    return retval;
}

static int _ssv6020_write_group_key_to_hw(struct ssv_hw *sh,
        u8 vif_idx, u8 algorithm, u8 key_idx, const u8 *key, int key_len)
{
    struct ssv_softc *sc = sh->sc;
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv_sec_param *ptr = NULL;
    struct ssv6020_hw_sec *p_hw_sec = (struct ssv6020_hw_sec *)&sh->hw_sec_key_data[0];
    u32    pn_offset  = 0;
    u32    word_data = 0;
    u32    sec_key_tbl = 0;
    int    address = 0;
    int retval = 0;

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    if (skb == NULL)
        return -ENOMEM;

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_SECURITY;
    host_cmd->sub_h_cmd = (u32)SSV6XXX_SECURITY_CMD_GROUP_KEY;
    host_cmd->blocking_seq_no =
        (((u16)SSV6XXX_HOST_CMD_SECURITY << 16) | (u16)SSV6XXX_SECURITY_CMD_GROUP_KEY);
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv_sec_param);

    ptr = (struct ssv_sec_param *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv_sec_param));
    ptr->vif_idx = vif_idx;
    ptr->key_idx = key_idx;
    ptr->key_len = key_len;
    ptr->alg = algorithm;
    memcpy((u8 *)ptr->key, (u8 *)key, key_len);

    printk("%s, vif_idx %d, key_idx %d, key_len %d, alg %d\n",
            __FUNCTION__, ptr->vif_idx, ptr->key_idx, ptr->key_len, ptr->alg);

    retval = HCI_SEND_CMD(sc->sh, skb);
    //Restore PN value if need.
    if((p_hw_sec != NULL) && (p_hw_sec->bss_group[vif_idx].group_cipher_type != SECURITY_NONE))
    {
        sec_key_tbl = _ssv6020_get_sec_key_tbl_addr(sc);
        pn_offset = offsetof(struct ssv6020_hw_sec, bss_group)*vif_idx+offsetof(struct ssv6020_bss, tx_pn_l);
        address = sec_key_tbl + pn_offset;
        word_data = p_hw_sec->bss_group[vif_idx].tx_pn_l;
        SMAC_REG_WRITE(sh, address, word_data);
        address += 4; //pointer to tx_pn_h
        word_data = p_hw_sec->bss_group[vif_idx].tx_pn_h;
        SMAC_REG_WRITE(sh, address, word_data);
        //Clear the stored key setting after restore.
        memset((void *)&(p_hw_sec->bss_group[vif_idx]), 0, sizeof(struct ssv6020_bss));
    }
    return retval;
}


static void ssv6020_set_aes_tkip_hw_crypto_group_key(struct ssv_softc *sc,
        struct ssv_vif_info *vif_info,
        struct ssv_sta_info *sta_info,
        void *param)
{
    int wsid = sta_info->hw_wsid;/* sta_info is protected by ssv6200_set_key(). */
    int key_idx = *(u8 *)param;

    if (wsid == (-1))
        return;

    BUG_ON(key_idx == 0);

    printk("Set CCMP/TKIP group key index %d to WSID %d.\n", key_idx, wsid);

    if (vif_info->vif_priv == NULL) {
        dev_err(sc->dev, "NULL VIF.\n");
        return;
    }

    dev_info(sc->dev, "Write group key index %d to VIF %d \n", key_idx, vif_info->vif_priv->vif_idx);
    _ssv6020_write_group_keyidx_to_hw(sc->sh, key_idx, vif_info->vif_priv->vif_idx);
}

static void ssv6020_write_pairwise_keyidx_to_hw(struct ssv_hw *sh, int key_idx, int wsid)
{
    _ssv6020_write_pairwise_keyidx_to_hw(sh, key_idx, wsid);
}

static void ssv6020_write_hw_group_keyidx(struct ssv_hw *sh, struct ssv_vif_priv_data *vif_priv, int key_idx)
{
    _ssv6020_write_group_keyidx_to_hw(sh, key_idx, vif_priv->vif_idx);
}

static int ssv6020_write_pairwise_key_to_hw(struct ssv_softc *sc,
        int index, u8 algorithm, const u8 *key, int key_len,
        struct ieee80211_key_conf *keyconf,
        struct ssv_vif_priv_data *vif_priv,
        struct ssv_sta_priv_data *sta_priv)
{
    int wsid = (-1);

    if (sta_priv == NULL)
    {
        dev_err(sc->dev, "Set pair-wise key with NULL STA.\n");
        return -EOPNOTSUPP;
    }

    wsid = sta_priv->sta_info->hw_wsid;/* sta_info is already protected by ssv6200_set_key(). */

    if ((wsid < 0) || (wsid >= SSV_NUM_STA))
    {
        dev_err(sc->dev, "Set pair-wise key to invalid WSID %d.\n", wsid);
        return -EOPNOTSUPP;
    }

    dev_info(sc->dev, "Set STA %d's pair-wise key of %d bytes.\n", wsid, key_len);

    if(index >= 0)
        ssv6020_write_pairwise_keyidx_to_hw(sc->sh, index, wsid);

    return _ssv6020_write_pairwise_key_to_hw(sc->sh, wsid, algorithm, index, key, key_len);
}

static int ssv6020_write_group_key_to_hw(struct ssv_softc *sc,
        int index, u8 algorithm, const u8 *key, int key_len,
        struct ieee80211_key_conf *keyconf,
        struct ssv_vif_priv_data *vif_priv,
        struct ssv_sta_priv_data *sta_priv)
{

    int wsid = sta_priv ? sta_priv->sta_info->hw_wsid : (-1);/* sta_info is already protected by ssv6200_set_key(). */

    if (vif_priv == NULL) {
        dev_err(sc->dev, "Setting group key to NULL VIF\n");
        return -EOPNOTSUPP;
    }

    dev_info(sc->dev, "Setting VIF %d group key %d of length %d to WSID %d.\n",
            vif_priv->vif_idx, index, key_len, wsid);

    /*save group key index */
    vif_priv->group_key_idx = index;

    if (sta_priv)
        sta_priv->group_key_idx = index;

    /* write group key index to all sta entity*/
    WARN_ON(sc->vif_info[vif_priv->vif_idx].vif_priv == NULL);
    ssv6xxx_foreach_vif_sta(sc, &sc->vif_info[vif_priv->vif_idx],
            ssv6020_set_aes_tkip_hw_crypto_group_key, &index);
    return _ssv6020_write_group_key_to_hw(sc->sh, vif_priv->vif_idx, algorithm, index, key, key_len);
}

static void ssv6020_write_key_to_hw(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv,
        void *key, int wsid, int key_idx, enum SSV6XXX_WSID_SEC key_type)
{
    return;
}

static void ssv6020_set_pairwise_cipher_type(struct ssv_hw *sh, u8 cipher, u8 wsid)
{
    printk(KERN_INFO "Set parewise key type %d\n", cipher);
    _ssv6020_set_pairwise_cipher_type(sh, cipher, wsid);
}

static void ssv6020_set_group_cipher_type(struct ssv_hw *sh, struct ssv_vif_priv_data *vif_priv, u8 cipher)
{
    printk(KERN_INFO "Set group key type %d\n", cipher);
    _ssv6020_set_group_cipher_type(sh, cipher, vif_priv->vif_idx);
}

#ifdef CONFIG_PM
static void ssv6020_save_clear_trap_reason(struct ssv_softc *sc)
{
    u32 trap0, trap1;
    SMAC_REG_READ(sc->sh, ADR_REASON_TRAP0, &trap0);
    SMAC_REG_READ(sc->sh, ADR_REASON_TRAP1, &trap1);
    SMAC_REG_WRITE(sc->sh, ADR_REASON_TRAP0, 0x00000000);
    SMAC_REG_WRITE(sc->sh, ADR_REASON_TRAP1, 0x00000000);
    printk("trap0 %08x, trap1 %08x\n", trap0, trap1);
}

static void ssv6020_restore_trap_reason(struct ssv_softc *sc)
{
    SMAC_REG_WRITE(sc->sh, ADR_REASON_TRAP0, 0x7FBC7F87);
    SMAC_REG_WRITE(sc->sh, ADR_REASON_TRAP1, 0x0000033F);
}

static void ssv6020_ps_save_reset_rx_flow(struct ssv_softc *sc)
{
#if 0
    struct ssv_hw *sh = sc->sh;

    SET_MTX_RATERPT_HWID(M_ENG_CPU);
    // don't change rx ctrl flow to trash can
    // otherwise, fw may be busy loop state to wait ba response.
    //ssv6020_set_rx_flow(sc->sh, RX_CTRL_FLOW, RX_TRASH);
    ssv6020_set_rx_flow(sc->sh, RX_MGMT_FLOW, RX_CPU_TRASH);
    if (USE_MAC80211_RX(sc->sh)) {
        HAL_SET_RX_FLOW(sc->sh, RX_DATA_FLOW, RX_CPU_TRASH);
    } else {
        HAL_SET_RX_FLOW(sc->sh, RX_DATA_FLOW, RX_CIPHER_MIC_CPU_TRASH);
    }
#endif
}

static void ssv6020_ps_restore_rx_flow(struct ssv_softc *sc)
{
#if 0
    struct ssv_hw *sh = sc->sh;

    ssv6020_set_rx_flow(sc->sh, RX_CTRL_FLOW, RX_CPU_HCI);
    ssv6020_set_rx_flow(sc->sh, RX_MGMT_FLOW, RX_CPU_HCI);
    if (USE_MAC80211_RX(sc->sh)){
        HAL_SET_RX_FLOW(sc->sh, RX_DATA_FLOW, RX_CPU_HCI);
    } else {
        HAL_SET_RX_FLOW(sc->sh, RX_DATA_FLOW, RX_CIPHER_MIC_CPU_HCI);
    }
    SET_MTX_RATERPT_HWID(M_ENG_CPU);
#endif
}

static void ssv6020_pmu_awake(struct ssv_softc *sc)
{
    struct sk_buff *skb;
    struct cfg_host_cmd *host_cmd;
    struct ssv6xxx_ps_params *ptr;
    int dev_type = HCI_DEVICE_TYPE(sc->sh->hci.hci_ctrl);

    skb = ssv_skb_alloc(sc, HOST_CMD_HDR_LEN + sizeof(struct ssv6xxx_ps_params));
    if (skb == NULL) {
        printk("%s:_skb_alloc fail!!!\n", __func__);
        return;
    }

    skb_put(skb, HOST_CMD_HDR_LEN + sizeof(struct ssv6xxx_ps_params));
    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd = (u8)SSV6XXX_HOST_CMD_PS;
    host_cmd->len = HOST_CMD_HDR_LEN + sizeof(struct ssv6xxx_ps_params);
    ptr = (struct ssv6xxx_ps_params *)host_cmd->un.dat8;
    memset(ptr, 0, sizeof(struct ssv6xxx_ps_params));
    ptr->if_type = ((dev_type == SSV_HWIF_INTERFACE_USB) ? 0 : 1);
    ptr->aid = sc->ps_aid;
    ptr->ops = SSV6XXX_PS_WAKEUP_FINISH;
    ptr->seqno = (u8)sc->ps_event_cnt;
    printk("%s() ps cmd: %d\n", __FUNCTION__, ptr->seqno);
    sc->ps_event_cnt++;
    if (128 == sc->ps_event_cnt)
        sc->ps_event_cnt = 0;

    HCI_SEND_CMD(sc->sh, skb);
}

static void ssv6020_ps_hold_on3(struct ssv_softc *sc, int value)
{
    //SMAC WoW phase 2 - tell F/W to hold ON3, or host cmd can't work.
    SMAC_REG_WRITE(sc->sh, ADR_PMU_RAM_05, value);
}
#endif

static void ssv6020_set_wep_hw_crypto_setting (struct ssv_softc *sc,
        struct ssv_vif_info *vif_info, struct ssv_sta_info *sta_info,
        void *param)
{
    int                       wsid = sta_info->hw_wsid;/* sta_info is protected by ssv6200_set_key(). */
    struct ssv_sta_priv_data *sta_priv = (struct ssv_sta_priv_data *)sta_info->sta->drv_priv;/* sta_info is protected by ssv6200_set_key(). */
    struct ssv_vif_priv_data *vif_priv = (struct ssv_vif_priv_data *)vif_info->vif->drv_priv;

    if (wsid == (-1))
        return;

    sta_priv->has_hw_encrypt = vif_priv->has_hw_encrypt;
    sta_priv->has_hw_decrypt = vif_priv->has_hw_decrypt;
    sta_priv->need_sw_encrypt = vif_priv->need_sw_encrypt;
    sta_priv->need_sw_decrypt = vif_priv->need_sw_decrypt;

    if (false == sta_priv->wep_key_update) {
        _ssv6020_set_pairwise_cipher_type(sc->sh, vif_priv->wep_cipher, wsid);
        _ssv6020_write_pairwise_keyidx_to_hw(sc->sh, vif_priv->wep_idx, wsid);
        sta_priv->wep_key_update = true;
    }
}

static void ssv6020_set_wep_key(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv,
        struct ssv_sta_priv_data *sta_priv, enum sec_type_en cipher, struct ieee80211_key_conf *key)
{
    if ((vif_priv->has_hw_decrypt == true) && (vif_priv->has_hw_encrypt == true)) {
        //store all wep key in group_key[] of BSSIDX
        printk("Store WEP key index %d to HW group_key[%d] of VIF %d\n", key->keyidx, key->keyidx,vif_priv->vif_idx);
        ssv6xxx_foreach_vif_sta(sc, &sc->vif_info[vif_priv->vif_idx], ssv6020_set_wep_hw_crypto_setting, key);

        _ssv6020_set_group_cipher_type(sc->sh, cipher, vif_priv->vif_idx);
        _ssv6020_write_group_keyidx_to_hw(sc->sh, key->keyidx, vif_priv->vif_idx);
        _ssv6020_write_group_key_to_hw(sc->sh, vif_priv->vif_idx, key->cipher, key->keyidx, key->key, key->keylen);
    }
    else
        printk("Not support HW security\n");
}

static void ssv6020_set_wep_hw_crypto_key(struct ssv_softc *sc,
        struct ssv_sta_priv_data *sta_priv, struct ssv_vif_priv_data *vif_priv)
{
    if (sta_priv->sta_idx == (-1))
        return;

    if (false == sta_priv->wep_key_update) {
        _ssv6020_set_pairwise_cipher_type(sc->sh, vif_priv->wep_cipher, sta_priv->sta_idx);
        _ssv6020_write_pairwise_keyidx_to_hw(sc->sh, vif_priv->wep_idx, sta_priv->sta_idx);
        sta_priv->wep_key_update = true;
    }
}

static void ssv6020_set_replay_ignore(struct ssv_hw *sh,u8 ignore)
{
    u32 temp;
    SMAC_REG_READ(sh,ADR_SCRT_SET,&temp);
    temp = temp & SCRT_RPLY_IGNORE_I_MSK;
    temp |= (ignore << SCRT_RPLY_IGNORE_SFT);
    SMAC_REG_WRITE(sh,ADR_SCRT_SET, temp);
}

static void ssv6020_update_decision_table_6(struct ssv_hw *sh, u32 value)
{
    SMAC_REG_WRITE(sh, ADR_MRX_FLT_TB6, value);
}

static int ssv6020_update_decision_table(struct ssv_softc *sc)
{
    int i;
#ifndef USE_CONCURRENT_DECI_TBL
    for(i=0; i<MAC_DECITBL1_SIZE; i++) {
        SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_TB0+i*4,
                sc->mac_deci_tbl[i]);
        SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_TB0+i*4,
                sc->mac_deci_tbl[i]);
    }
    for(i=0; i<MAC_DECITBL2_SIZE; i++) {
        SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN0+i*4,
                sc->mac_deci_tbl[i+MAC_DECITBL1_SIZE]);
        SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_EN0+i*4,
                sc->mac_deci_tbl[i+MAC_DECITBL1_SIZE]);
    }
#else
    extern u16 concurrent_deci_tbl[];
    for(i=0; i<MAC_DECITBL1_SIZE; i++) {
        SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_TB0+i*4,
                concurrent_deci_tbl[i]);
        SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_TB0+i*4,
                concurrent_deci_tbl[i]);
    }
    for(i=0; i<MAC_DECITBL2_SIZE; i++) {
        SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN0+i*4,
                concurrent_deci_tbl[i+MAC_DECITBL1_SIZE]);
        SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_EN0+i*4,
                concurrent_deci_tbl[i+MAC_DECITBL1_SIZE]);
    }
    SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN9,
            concurrent_deci_tbl[8]);
    SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_EN10,
            concurrent_deci_tbl[9]);
    SMAC_REG_CONFIRM(sc->sh, ADR_DUAL_IDX_EXTEND, 1);
#endif
    return 0;
}

static void ssv6020_get_fw_version(struct ssv_hw *sh, u32 *regval)
{
    SMAC_REG_READ(sh, ADR_TX_SEG, regval);
}

static void ssv6020_set_op_mode(struct ssv_hw *sh, u32 op_mode, int vif_idx)
{
    switch (vif_idx) {
        case 0:
            SMAC_REG_SET_BITS(sh, ADR_GLBLE_SET, op_mode, OP_MODE_MSK);
            break;
        case 1:
            SMAC_REG_SET_BITS(sh, ADR_OP_MODE1, op_mode, OP_MODE1_MSK);
            break;
        default:
            printk("Does not support set OP mode to HW for VIF %d\n", vif_idx);
            break;
    }
}

static void ssv6020_set_dur_burst_sifs_g(struct ssv_hw *sh, u32 val)
{
    // function removed at ssv6020 MP chio
}

static void ssv6020_set_dur_slot(struct ssv_hw *sh, u32 val)
{
    SET_SLOTTIME(val);
}

static void ssv6020_set_sifs(struct ssv_hw *sh, int band)
{
    if (band == INDEX_80211_BAND_2GHZ){
        SET_SIFS(10);
        SET_SIGEXT(6);
    } else {
        SET_SIFS(16);
        SET_SIGEXT(0);
    }
}

static void ssv6020_set_qos_enable(struct ssv_hw *sh, bool val)
{
    //set QoS status
    SMAC_REG_SET_BITS(sh, ADR_GLBLE_SET,
            (val<<QOS_EN_SFT), QOS_EN_MSK);

}

static void ssv6020_set_wmm_param(struct ssv_softc *sc,
        const struct ieee80211_tx_queue_params *params, u16 queue)
{
    struct ssv_hw *sh = sc->sh;
    u32 i = 0;
    u32 regval = 0;
    u32 cw;
    u8 hw_txqid = sc->tx.hw_txqid[queue];

    if (sc->sh->cfg.backoff_enable) {
        u32 backoff = 0;
        u32 aifs2 = 0x1f2102;

        printk("set fix backoff value 0x%x, aifs2 0x%x\n", backoff, aifs2);
        SET_TXQ0_MTX_Q_RND_MODE(0x4);
        SET_TXQ0_MTX_Q_BKF_CNT_FIX(backoff);
        SMAC_REG_WRITE(sc->sh, ADR_TXQ0_MTX_Q_AIFSN, aifs2);

        SET_TXQ1_MTX_Q_RND_MODE(0x4);
        SET_TXQ1_MTX_Q_BKF_CNT_FIX(backoff);
        SMAC_REG_WRITE(sc->sh, ADR_TXQ1_MTX_Q_AIFSN, aifs2);

        SET_TXQ2_MTX_Q_RND_MODE(0x4);
        SET_TXQ2_MTX_Q_BKF_CNT_FIX(backoff);
        SMAC_REG_WRITE(sc->sh, ADR_TXQ2_MTX_Q_AIFSN, aifs2);

        SET_TXQ3_MTX_Q_RND_MODE(0x4);
        SET_TXQ3_MTX_Q_BKF_CNT_FIX(backoff);
        SMAC_REG_WRITE(sc->sh, ADR_TXQ3_MTX_Q_AIFSN, aifs2);
        return;
    }

    cw = params->aifs&0xf;
    cw|= ((ilog2(params->cw_min+1))&0xf)<<TXQ1_MTX_Q_ECWMIN_SFT;//8;
    cw|= ((ilog2(params->cw_max+1))&0xf)<<TXQ1_MTX_Q_ECWMAX_SFT;//12;
    cw|= ((params->txop)&0xff)<<TXQ1_MTX_Q_TXOP_LIMIT_SFT;//16;

    //Check whether all queues to follow the VO setting.
    if(sc->sh->cfg.wmm_follow_vo)
    {
        if(WMM_AC_VO == queue)
        {
            for(i = 0; i < WMM_NUM_AC; i++)
            {
                SMAC_REG_WRITE(sc->sh, ADR_TXQ0_MTX_Q_AIFSN+0x100*hw_txqid, cw);
            }
        }
        else
        {
            SMAC_REG_READ(sc->sh, ADR_TXQ0_MTX_Q_AIFSN+0x100*(sc->tx.hw_txqid[WMM_AC_VO]), &regval);
            if(0 != regval)
            {
                SMAC_REG_WRITE(sc->sh, ADR_TXQ0_MTX_Q_AIFSN+0x100*hw_txqid, regval);
            }
            else
            {
                SMAC_REG_WRITE(sc->sh, ADR_TXQ0_MTX_Q_AIFSN+0x100*hw_txqid, cw);
            }
        }
    }
    else
    {
        SMAC_REG_WRITE(sc->sh, ADR_TXQ0_MTX_Q_AIFSN+0x100*hw_txqid, cw);
    }
}

// allocate pbuf
static u32 ssv6020_alloc_pbuf(struct ssv_softc *sc, int size, int type)
{
    u32 regval, pad;
    int cnt = MAX_RETRY_COUNT;
    int page_cnt = (size + ((1 << HW_MMU_PAGE_SHIFT) - 1)) >> HW_MMU_PAGE_SHIFT;

    regval = 0;

    mutex_lock(&sc->mem_mutex);

    //brust could be dividen by 4
    pad = size%4;
    size += pad;

    do{
        //printk("[A] ssv6xxx_pbuf_alloc\n");

        SMAC_REG_WRITE(sc->sh, ADR_WR_ALC, (size | (type << 16)));
        SMAC_REG_READ(sc->sh, ADR_WR_ALC, &regval);

        if (regval == 0) {
            cnt--;
            msleep(1);
        }
        else
            break;

    } while (cnt);

    // If TX buffer is allocated, AMPDU maximum size m
    if (type == TX_BUF) {
        sc->sh->tx_page_available -= page_cnt;
        sc->sh->page_count[PACKET_ADDR_2_ID(regval)] = page_cnt;
    }

    mutex_unlock(&sc->mem_mutex);

    if (regval == 0)
        dev_err(sc->dev, "Failed to allocate packet buffer of %d bytes in %d type.",
                size, type);
    else {
        dev_info(sc->dev, "Allocated %d type packet buffer of size %d (%d) at address %x.\n",
                type, size, page_cnt, regval);
    }

    return regval;
}

static inline bool ssv6020_mcu_input_full(struct ssv_softc *sc)
{
    u32 regval=0;

    SMAC_REG_READ(sc->sh, ADR_MCU_STATUS, &regval);
    return (CH0_FULL_MSK & regval);
}

// free pbuf
static bool ssv6020_free_pbuf(struct ssv_softc *sc, u32 pbuf_addr)
{
    u32  regval=0;
    u16  failCount=0;
    u8  *p_tx_page_cnt = &sc->sh->page_count[PACKET_ADDR_2_ID(pbuf_addr)];

    while (ssv6020_mcu_input_full(sc))
    {
        if (failCount++ < 1000) continue;
        printk("=============>ERROR!!MAILBOX Block[%d]\n", failCount);
        return false;
    } //Wait until input queue of cho is not full.

    mutex_lock(&sc->mem_mutex);

    // {HWID[3:0], PKTID[6:0]}
    regval = ((M_ENG_TRASH_CAN << HW_ID_OFFSET) |(pbuf_addr >> ADDRESS_OFFSET));

    printk("[A] ssv6xxx_pbuf_free addr[%08x][%x]\n", pbuf_addr, regval);
    SMAC_REG_WRITE(sc->sh, ADR_CH0_TRIG_1, regval);

    if (*p_tx_page_cnt)
    {
        sc->sh->tx_page_available += *p_tx_page_cnt;
        *p_tx_page_cnt = 0;
    }

    mutex_unlock(&sc->mem_mutex);

    return true;
}

static void ssv6020_ampdu_auto_crc_en(struct ssv_hw *sh)
{
    // Enable HW_AUTO_CRC_32 ======================================
    SMAC_REG_SET_BITS(sh, ADR_MTX_MISC_EN, (0x1 << MTX_AMPDU_CRC8_AUTO_SFT),
            MTX_AMPDU_CRC8_AUTO_MSK);
}

static void ssv6020_set_rx_ba(struct ssv_hw *sh, bool on, u8 *ta,
        u16 tid, u16 ssn, u8 buf_size)
{
#if 0   /* move to init_mac*/
    if (on) {
        //turn on ba session
        SMAC_REG_WRITE(sh, ADR_BA_CTRL,
                (1 << BA_AGRE_EN_SFT)| (0x01 << BA_CTRL_SFT));
    } else {
        //turn off ba session
        if (sh->sc->rx_ba_session_count == 0)
            SMAC_REG_WRITE(sh, ADR_BA_CTRL, 0x0);
    }
#endif
    // set a wrong TID first, then HW will flush TID while receive new onw
    // to avoid some problem
    SET_BA_TID (0xf);

}

static u8 ssv6020_read_efuse(struct ssv_hw *sh, u8 *mac)
{
#define EFUSE_RATE_GAIN_G_HT20      0xFF000000
#define EFUSE_RATE_GAIN_B_HT40      0x00FF0000
#define EFUSE_BAND_GAIN_2G          0x0000FF00
#define EFUSE_XTAL_OFFSET           0x000000FF

#define EFUSE_SHIFT_RATE_GAIN_G_HT20      24
#define EFUSE_SHIFT_RATE_GAIN_B_HT40      16
#define EFUSE_SHIFT_BAND_GAIN_2G           8
#define EFUSE_SHIFT_XTAL_OFFSET            0


    u32 regval1 = 0, regval2 = 0;
    u32 mac1 = 0, mac2 = 0, mac3 = 0;
    u32 ble_gain1 = 0, ble_gain2 = 0;
    u32 sku_id = 0;
    u32 value = 0;

    SMAC_REG_READ(sh, 0xD9003FFC, &sku_id);
    SMAC_REG_READ(sh, 0xD9003FB4, &regval1);
    SMAC_REG_READ(sh, 0xD9003FB0, &regval2);
    SMAC_REG_READ(sh, 0xD9003FC0, &mac1);
    SMAC_REG_READ(sh, 0xD9003FBC, &mac2);
    SMAC_REG_READ(sh, 0xD9003FB8, &mac3);
    SMAC_REG_READ(sh, 0xD9003F94, &ble_gain1);
    SMAC_REG_READ(sh, 0xD9003F90, &ble_gain2);

    // set sku id
    sh->cfg.chip_identity = sku_id;

    // mac address
    // EFUSE_MAC
    mac[0] = (mac3 & 0xFF000000) >> 24;
    mac[1] = (mac3 & 0x00FF0000) >> 16;
    mac[2] = (mac3 & 0x0000FF00) >> 8;
    mac[3] = (mac3 & 0x000000FF) >> 0;
    mac[4] = (mac2 & 0xFF000000) >> 24;
    mac[5] = (mac2 & 0x00FF0000) >> 16;
    if (is_valid_ether_addr(mac)) {
       sh->efuse_bitmap |= BIT(EFUSE_MAC);
    } else {
        mac[0] = (mac2 & 0x0000FF00) >> 8;
        mac[1] = (mac2 & 0x000000FF) >> 0;
        mac[2] = (mac1 & 0xFF000000) >> 24;
        mac[3] = (mac1 & 0x00FF0000) >> 16;
        mac[4] = (mac1 & 0x0000FF00) >> 8;
        mac[5] = (mac1 & 0x000000FF) >> 0;
        if (is_valid_ether_addr(mac)) {
            sh->efuse_bitmap |= BIT(EFUSE_MAC);
        } else {
            sh->efuse_bitmap &= ~BIT(EFUSE_MAC);
        }
    }

    // rate gain
    // rate gain g/ht20
    value = ((regval2 & EFUSE_RATE_GAIN_G_HT20) >> EFUSE_SHIFT_RATE_GAIN_G_HT20);
    if (0 != value) {
       sh->efuse_bitmap |= BIT(EFUSE_RATE_TABLE_1);
       sh->cfg.rate_table_1 = value;
    } else {
        value = ((regval1 & EFUSE_RATE_GAIN_G_HT20) >> EFUSE_SHIFT_RATE_GAIN_G_HT20);
        if (0 != value) {
            sh->efuse_bitmap |= BIT(EFUSE_RATE_TABLE_1);
            sh->cfg.rate_table_1 = value;
        } else {
            sh->efuse_bitmap &= ~BIT(EFUSE_RATE_TABLE_1);
            sh->cfg.rate_table_1 = 0;
        }
    }

    // rate gain b/ht40
    value = ((regval2 & EFUSE_RATE_GAIN_B_HT40) >> EFUSE_SHIFT_RATE_GAIN_B_HT40);
    if (0 != value) {
       sh->efuse_bitmap |= BIT(EFUSE_RATE_TABLE_2);
       sh->cfg.rate_table_2 = value;
    } else {
        value = ((regval1 & EFUSE_RATE_GAIN_B_HT40) >> EFUSE_SHIFT_RATE_GAIN_B_HT40);
        if (0 != value) {
            sh->efuse_bitmap |= BIT(EFUSE_RATE_TABLE_2);
            sh->cfg.rate_table_2 = value;
        } else {
            sh->efuse_bitmap &= ~BIT(EFUSE_RATE_TABLE_2);
            sh->cfg.rate_table_2 = 0;
        }
    }

    // band gain
    value = ((regval2 & EFUSE_BAND_GAIN_2G) >> EFUSE_SHIFT_BAND_GAIN_2G);
    if (0 != value) {
       sh->efuse_bitmap |= BIT(EFUSE_TX_POWER_INDEX_1);
       sh->cfg.tx_power_index_1 = value;
    } else {
        value = ((regval1 & EFUSE_BAND_GAIN_2G) >> EFUSE_SHIFT_BAND_GAIN_2G);
        if (0 != value) {
            sh->efuse_bitmap |= BIT(EFUSE_TX_POWER_INDEX_1);
            sh->cfg.tx_power_index_1 = value;
        } else {
            sh->efuse_bitmap &= ~BIT(EFUSE_TX_POWER_INDEX_1);
            sh->cfg.tx_power_index_1 = 0;
        }
    }

    //xtal offset
    value = ((regval2 & EFUSE_XTAL_OFFSET) >> EFUSE_SHIFT_XTAL_OFFSET);
    if (0 != value) {
       sh->efuse_bitmap |= BIT(EFUSE_CRYSTAL_FREQUENCY_OFFSET);
       sh->cfg.crystal_frequency_offset = value;
    } else {
        value = ((regval1 & EFUSE_XTAL_OFFSET) >> EFUSE_SHIFT_XTAL_OFFSET);
        if (0 != value) {
            sh->efuse_bitmap |= BIT(EFUSE_CRYSTAL_FREQUENCY_OFFSET);
            sh->cfg.crystal_frequency_offset = value;
        } else {
            sh->efuse_bitmap &= ~BIT(EFUSE_CRYSTAL_FREQUENCY_OFFSET);
            sh->cfg.crystal_frequency_offset = 0;
        }
    }

    //ble gain
    value = ble_gain2;
    if (0 != value) {
       sh->efuse_bitmap |= BIT(EFUSE_BLE_GAIN);
       sh->cfg.ble_gain = value;
    } else {
        value = ble_gain1;
        if (0 != value) {
            sh->efuse_bitmap |= BIT(EFUSE_BLE_GAIN);
            sh->cfg.ble_gain = value;
        } else {
            sh->efuse_bitmap &= ~BIT(EFUSE_BLE_GAIN);
            sh->cfg.ble_gain = 0;
        }
    }

    return 0;
}

#define CLK_SRC_SYNTH_40M  4
#define CLK_80M_PHY        8
static int ssv6020_chg_clk_src(struct ssv_hw *sh)
{
    // move these code to init pll.
#if 0
    int ret = 0;

    if (sh->cfg.clk_src_80m)
        ret = SMAC_REG_WRITE(sh, ADR_CLOCK_SELECTION, CLK_80M_PHY);
    else
        ret = SMAC_REG_WRITE(sh, ADR_CLOCK_SELECTION, CLK_SRC_SYNTH_40M);

    msleep(1);
#endif
    return 0;
}

static void ssv6020_beacon_loss_enable(struct ssv_hw *sh)
{
    SET_RG_RX_BEACON_LOSS_ON(1);
}

static void ssv6020_beacon_loss_disable(struct ssv_hw *sh)
{
    SET_RG_RX_BEACON_LOSS_ON(0);
}

static void ssv6020_beacon_loss_config(struct ssv_hw *sh, u16 beacon_int, const u8 *bssid)
{
    struct ssv_softc *sc = sh->sc;
    u32   mac_31_0;
    u16   mac_47_32;

    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_BEACON,
            "%s(): beacon_int %x, bssid %02x:%02x:%02x:%02x:%02x:%02x\n",
            __FUNCTION__, beacon_int, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

    memcpy(&mac_31_0, bssid, 4);
    memcpy(&mac_47_32, bssid+4, 2);

    ssv6020_beacon_loss_disable(sh);
    SET_RG_RX_BEACON_INTERVAL(beacon_int);
    SET_RG_RX_BEACON_LOSS_CNT_LMT(10);

    SET_RG_RX_PKT_FC(0x0080);
    SET_RG_RX_PKT_ADDR1_ON(0);
    SET_RG_RX_PKT_ADDR2_ON(0);
    SET_RG_RX_PKT_ADDR3_ON(1);
    SET_RG_RX_PKT_ADDR3_31_0((u32)mac_31_0);
    SET_RG_RX_PKT_ADDR3_47_32((u16)mac_47_32);

    ssv6020_beacon_loss_enable(sh);
    return;
}

static void ssv6020_update_txq_mask(struct ssv_hw *sh, u32 txq_mask)
{
    SMAC_REG_SET_BITS(sh, ADR_MTX_MISC_EN,
            (txq_mask << MTX_HALT_Q_MB_SFT), MTX_HALT_Q_MB_MSK);
}

static void ssv6020_readrg_hci_inq_info(struct ssv_hw *sh, int *hci_used_id, int *tx_use_page)
{
    int ret = 0;
    u32 regval = 0;

    *hci_used_id = 0;

    ret = SMAC_REG_READ(sh, ADR_TX_ID_ALL_INFO, &regval);
    if (ret == 0)
        *tx_use_page = ((regval & TX_PAGE_USE_7_0_MSK) >> TX_PAGE_USE_7_0_SFT);
}

#define MAX_HW_TXQ_INFO_LEN     1
static bool ssv6020_readrg_txq_info(struct ssv_hw *sh, u32 *txq_info, int *hci_used_id)
{
    int ret = 0;
    struct ssv6xxx_hci_txq_info2 value;
    u32 dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);

    ret = SMAC_REG_READ(sh, ADR_TX_ID_ALL_INFO2, (u32 *)&value);
    if (ret == 0) {
        if (dev_type == SSV_HWIF_INTERFACE_USB) {
            value.tx_use_page = value.tx_use_page + 1;
        }
        memcpy(txq_info, &value, sizeof(struct ssv6xxx_hci_txq_info));
    }
    *hci_used_id = 0;

    return ret;
}

static bool ssv6020_dump_wsid(struct ssv_hw *sh)
{
    const u8 *op_mode_str[]={"STA", "AP", "AD-HOC", "WDS"};
    const u8 *ht_mode_str[]={"Non-HT", "HT-MF", "HT-GF", "RSVD"};
    u32 regval;
    int s;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    // ToDO Liam
    // s < 2 should changed to SSV6020_NUM_HW_STA when above register define extended.
    for (s = 0; s < SSV6020_NUM_HW_STA; s++) {

        SMAC_REG_READ(sh, reg_wsid[s], &regval);
        snprintf_res(cmd_data, "==>WSID[%d]\n\tvalid[%d] qos[%d] op_mode[%s] ht_mode[%s]\n",
                s, regval&0x1, (regval>>1)&0x1, op_mode_str[((regval>>2)&3)], ht_mode_str[((regval>>4)&3)]);

        SMAC_REG_READ(sh, reg_wsid[s]+4, &regval);
        snprintf_res(cmd_data, "\tMAC[%02x:%02x:%02x:%02x:",
                (regval&0xff), ((regval>>8)&0xff), ((regval>>16)&0xff), ((regval>>24)&0xff));

        SMAC_REG_READ(sh, reg_wsid[s]+8, &regval);
        snprintf_res(cmd_data, "%02x:%02x]\n",
                (regval&0xff), ((regval>>8)&0xff));
    }

    return 0;
}

static bool ssv6020_dump_decision(struct ssv_hw *sh)
{
    u32 addr, regval;
    int s;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    snprintf_res(cmd_data, ">> Decision Table:\n");
    for(s = 0, addr = ADR_MRX_FLT_TB0; s < 16; s++, addr+=4) {
        SMAC_REG_READ(sh, addr, &regval);
        snprintf_res(cmd_data, "   [%d]: ADDR[0x%08x] = 0x%08x\n",
                s, addr, regval);
    }
    snprintf_res(cmd_data, "\n\n>> Decision Mask:\n");
    for (s = 0, addr = ADR_MRX_FLT_EN0; s < 9; s++, addr+=4) {
        SMAC_REG_READ(sh, addr, &regval);
        snprintf_res(cmd_data, "   [%d]: ADDR[0x%08x] = 0x%08x\n",
                s, addr, regval);
    }
    snprintf_res(cmd_data, "\n\n");
    return 0;
}

static u32 ssv6020_get_ffout_cnt(u32 value, int tag)
{
    switch (tag) {
        case M_ENG_CPU:
            return ((value & FFO0_CNT_MSK) >> FFO0_CNT_SFT);
        case M_ENG_HWHCI:
            return ((value & FFO1_CNT_MSK) >> FFO1_CNT_SFT);
        case M_ENG_ENCRYPT:
            return ((value & FFO3_CNT_MSK) >> FFO3_CNT_SFT);
        case M_ENG_MACRX:
            return ((value & FFO4_CNT_MSK) >> FFO4_CNT_SFT);
        case M_ENG_MIC:
            return ((value & FFO5_CNT_MSK) >> FFO5_CNT_SFT);
        case M_ENG_TX_EDCA0:
            return ((value & FFO6_CNT_MSK) >> FFO6_CNT_SFT);
        case M_ENG_TX_EDCA1:
            return ((value & FFO7_CNT_MSK) >> FFO7_CNT_SFT);
        case M_ENG_TX_EDCA2:
            return ((value & FFO8_CNT_MSK) >> FFO8_CNT_SFT);
        case M_ENG_TX_EDCA3:
            return ((value & FFO9_CNT_MSK) >> FFO9_CNT_SFT);
        case M_ENG_TX_MNG:
            return ((value & FFO10_CNT_MSK) >> FFO10_CNT_SFT);
        case M_ENG_ENCRYPT_SEC:
            return ((value & FFO11_CNT_MSK) >> FFO11_CNT_SFT);
        case M_ENG_MIC_SEC:
            return ((value & FFO12_CNT_MSK) >> FFO12_CNT_SFT);
        case M_ENG_TRASH_CAN:
            return ((value & FFO15_CNT_MSK) >> FFO15_CNT_SFT);
        default:
            return 0;
    }
}

static u32 ssv6020_get_in_ffcnt(u32 value, int tag)
{
    switch (tag) {
        case M_ENG_CPU:
            return ((value & FF0_CNT_MSK) >> FF0_CNT_SFT);
        case M_ENG_HWHCI:
            return ((value & FF1_CNT_MSK) >> FF1_CNT_SFT);
        case M_ENG_ENCRYPT:
            return ((value & FF3_CNT_MSK) >> FF3_CNT_SFT);
        case M_ENG_MACRX:
            return ((value & FF4_CNT_MSK) >> FF4_CNT_SFT);
        case M_ENG_MIC:
            return ((value & FF5_CNT_MSK) >> FF5_CNT_SFT);
        case M_ENG_TX_EDCA0:
            return ((value & FF6_CNT_MSK) >> FF6_CNT_SFT);
        case M_ENG_TX_EDCA1:
            return ((value & FF7_CNT_MSK) >> FF7_CNT_SFT);
        case M_ENG_TX_EDCA2:
            return ((value & FF8_CNT_MSK) >> FF8_CNT_SFT);
        case M_ENG_TX_EDCA3:
            return ((value & FF9_CNT_MSK) >> FF9_CNT_SFT);
        case M_ENG_TX_MNG:
            return ((value & FF10_CNT_MSK) >> FF10_CNT_SFT);
        case M_ENG_ENCRYPT_SEC:
            return ((value & FF11_CNT_MSK) >> FF11_CNT_SFT);
        case M_ENG_MIC_SEC:
            return ((value & FF12_CNT_MSK) >> FF12_CNT_SFT);
        case M_ENG_TRASH_CAN:
            return ((value & FF15_CNT_MSK) >> FF15_CNT_SFT);
        default:
            return 0;
    }
}

static void ssv6020_read_ffout_cnt(struct ssv_hw *sh,
        u32 *value, u32 *value1, u32 *value2)
{
    SMAC_REG_READ(sh, ADR_RD_FFOUT_CNT1, value);
    SMAC_REG_READ(sh, ADR_RD_FFOUT_CNT2, value1);
    SMAC_REG_READ(sh, ADR_RD_FFOUT_CNT3, value2);
}

static void ssv6020_read_in_ffcnt(struct ssv_hw *sh,
        u32 *value, u32 *value1)
{
    SMAC_REG_READ(sh, ADR_RD_IN_FFCNT1, value);
    SMAC_REG_READ(sh, ADR_RD_IN_FFCNT2, value1);
}

static void ssv6020_read_id_len_threshold(struct ssv_hw *sh,
        u32 *tx_len, u32 *rx_len)
{
    u32 regval = 0;

    if(SMAC_REG_READ(sh, ADR_ID_LEN_THREADSHOLD2, &regval));
    *tx_len = ((regval & TX_ID_ALC_LEN_MSK) >> TX_ID_ALC_LEN_SFT);
    *rx_len = ((regval & RX_ID_ALC_LEN_MSK) >> RX_ID_ALC_LEN_SFT);
}

static void ssv6020_read_allid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    SMAC_REG_READ(sh, ADR_RD_ID0, id0);
    SMAC_REG_READ(sh, ADR_RD_ID1, id1);
    SMAC_REG_READ(sh, ADR_RD_ID2, id2);
    SMAC_REG_READ(sh, ADR_RD_ID3, id3);
}

static void ssv6020_read_txid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    SMAC_REG_READ(sh, ADR_TX_ID0, id0);
    SMAC_REG_READ(sh, ADR_TX_ID1, id1);
    SMAC_REG_READ(sh, ADR_TX_ID2, id2);
    SMAC_REG_READ(sh, ADR_TX_ID3, id3);
}

static void ssv6020_read_rxid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    SMAC_REG_READ(sh, ADR_RX_ID0, id0);
    SMAC_REG_READ(sh, ADR_RX_ID1, id1);
    SMAC_REG_READ(sh, ADR_RX_ID2, id2);
    SMAC_REG_READ(sh, ADR_RX_ID3, id3);
}

static void ssv6020_read_wifi_halt_status(struct ssv_hw *sh, u32 *status)
{
    *status=GET_BLE_HALT_WIFI_MODE;
}

static void ssv6020_read_tag_status(struct ssv_hw *sh,
        u32 *ava_status)
{
    u32 regval = 0;

    if(SMAC_REG_READ(sh, ADR_TAG_STATUS, &regval));
    *ava_status = ((regval & AVA_TAG_MSK) >> AVA_TAG_SFT);
}

void ssv6020_reset_mib_mac(struct ssv_hw *sh)
{
    SMAC_REG_WRITE(sh, ADR_MIB_EN, 0);
    msleep(1);
    SMAC_REG_WRITE(sh, ADR_MIB_EN, 0xffffffff);
}

static void ssv6020_reset_mib(struct ssv_hw *sh)
{
    ssv6020_reset_mib_mac(sh);

    HAL_RESET_MIB_PHY(sh);
}

static void ssv6020_list_mib(struct ssv_hw *sh)
{
    u32 addr, value;
    int i;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    addr = MIB_REG_BASE;

    for (i = 0; i < 120; i++, addr+=4) {
        SMAC_REG_READ(sh, addr, &value);
        snprintf_res(cmd_data, "%08x ", value);

        if (((i+1) & 0x07) == 0)
            snprintf_res(cmd_data, "\n");
    }
    snprintf_res(cmd_data, "\n");
}

static void ssv6020_dump_mib_rx(struct ssv_hw *sh)
{
    u32  value;
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    snprintf_res(cmd_data, "HWHCI status:\n");
    snprintf_res(cmd_data, "%-12s\t%-12s\t%-12s\t%-12s\n",
            "HCI_RX_PKT_CNT", "HCI_RX_DROP_CNT", "HCI_RX_TRAP_CNT", "HCI_RX_FAIL_CNT");

    snprintf_res(cmd_data, "[%08x]\t", GET_RX_PKT_COUNTER);
    snprintf_res(cmd_data, "[%08x]\t", GET_RX_PKT_DROP_COUNTER);
    snprintf_res(cmd_data, "[%08x]\t", GET_RX_PKT_TRAP_COUNTER);
    snprintf_res(cmd_data, "[%08x]\n\n", GET_HOST_RX_FAIL_COUNTER);

    snprintf_res(cmd_data, "MAC RX status:\n");
    snprintf_res(cmd_data, "%-12s\t%-12s\t%-12s\t%-12s\n",
            "MRX_FCS_SUCC", "MRX_FCS_ERR", "MRX_ALC_FAIL", "MRX_MISS");

    SMAC_REG_READ(sh, ADR_MRX_FCS_SUCC, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_MRX_FCS_ERR, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_MRX_ALC_FAIL, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_MRX_MISS, &value);
    snprintf_res(cmd_data, "[%08x]\n", value);

    snprintf_res(cmd_data, "%-12s\t%-12s\t%-12s\t%-12s\n",
            "MRX_MB_MISS", "MRX_NIDLE_MISS", "LEN_ALC_FAIL", "LEN_CRC_FAIL");

    SMAC_REG_READ(sh, ADR_MRX_MB_MISS, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_MRX_NIDLE_MISS, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_DBG_LEN_ALC_FAIL, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_DBG_LEN_CRC_FAIL, &value);
    snprintf_res(cmd_data, "[%08x]\n", value);

    snprintf_res(cmd_data, "%-12s\t%-12s\t%-12s\t%-12s\n",
            "DBG_AMPDU_PASS", "DBG_AMPDU_FAIL", "ID_ALC_FAIL1", "ID_ALC_FAIL2");

    SMAC_REG_READ(sh, ADR_DBG_AMPDU_PASS, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_DBG_AMPDU_FAIL, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_ID_ALC_FAIL1, &value);
    snprintf_res(cmd_data, "[%08x]\t", value);

    SMAC_REG_READ(sh, ADR_ID_ALC_FAIL2, &value);
    snprintf_res(cmd_data, "[%08x]\n\n", value);

    HAL_DUMP_MIB_RX_PHY(sh);
}

static void ssv6020_dump_mib_tx(struct ssv_hw *sh)
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;

    snprintf_res(cmd_data, "HWHCI status:\n");
    snprintf_res(cmd_data, "  %-16s  :%08x\t%-16s  :%08x\n",
            "HCI_TX_ALLOC_CNT", GET_HCI_TX_ALLOC_CNT, "HCI_TX_PKT_CNT", GET_TX_PKT_COUNTER);
    snprintf_res(cmd_data, "  %-16s  :%08x\t%-16s  :%08x\n",
            "HCI_TX_DROP_CNT", GET_TX_PKT_DROP_COUNTER, "HCI_TX_TRAP_CNT", GET_TX_PKT_TRAP_COUNTER);
    snprintf_res(cmd_data, "  %-16s  :%08x\n\n", "HCI_TX_FAIL_CNT", GET_HOST_TX_FAIL_COUNTER);

    snprintf_res(cmd_data, "MAC TX status:\n");
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx Group"
            , GET_MTX_GRP,"Tx Fail", GET_MTX_FAIL);
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx Retry"
            , GET_MTX_RETRY,"Tx Multi Retry", GET_MTX_MULTI_RETRY);
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx RTS success"
            , GET_MTX_RTS_SUCC,"Tx RTS Fail", GET_MTX_RTS_FAIL);
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx ACK Fail"
            , GET_MTX_ACK_FAIL,"Tx total count", GET_MTX_FRM);
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx ack count"
            , GET_MTX_ACK_TX,"Tx WSID0 success", GET_MTX_WSID0_SUCC);
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx WSID0 frame"
            , GET_MTX_WSID0_FRM,"Tx WSID0 retry", GET_MTX_WSID0_RETRY);
    snprintf_res(cmd_data, "  %-16s  :%08d\t%-16s  :%08d\n", "Tx WSID0 Total "
            , GET_MTX_WSID0_TOTAL, "CTS_TX", GET_MTX_CTS_TX);

}

static void ssv6020_cmd_mib(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    /**
     *  mib [reset|rx|tx]
     * (1) mib reset
     * (2) mib rx
     * (3) mib tx
     */
    if ((argc == 2) && (!strcmp(argv[1], "reset"))) {
        ssv6020_reset_mib(sc->sh);
        snprintf_res(cmd_data, " => MIB reseted\n");

    } else if ((argc == 2) && (!strcmp(argv[1], "list"))) {
        ssv6020_list_mib(sc->sh);
    } else if ((argc == 2) && (strcmp(argv[1], "rx") == 0)) {
        ssv6020_dump_mib_rx(sc->sh);
    } else if ((argc == 2) && (strcmp(argv[1], "tx") == 0)) {
        ssv6020_dump_mib_tx(sc->sh);
    } else {
        snprintf_res(cmd_data, "mib [reset|list|rx|tx]\n\n");
    }
}

static void ssv6020_cmd_power_saving(struct ssv_softc *sc, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sc->cmd_data;
    char *endp;
    u8 triggers = 0;
    /**
     *  ps [aid]
     * aid: association id for station
     * aid 0 for disconnection
     */
    if (argc == 3) {
        sc->ps_aid = simple_strtoul(argv[1], &endp, 10);
        triggers = simple_strtoul(argv[2], &endp, 10);
#ifdef CONFIG_PM
#ifndef CONFIG_MIFI
        ssv6xxx_trigger_pmu(sc, triggers, 0);
#endif
#endif
    } else {
        snprintf_res(cmd_data, "ps [aid_value]\n\n");
    }
}

static void ssv6020_cmd_hwinfo_mac_beacon(struct ssv_hw *sh)
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    u32 value, value1;

    snprintf_res(cmd_data, "HWBCN_PKTID\n");
    snprintf_res(cmd_data, "\tMTX_BCN_CFG_VLD      : %d\n", GET_MTX_BCN_CFG_VLD);
    snprintf_res(cmd_data, "\tMTX_AUTO_BCN_ONGOING : 0x%x\n", GET_MTX_AUTO_BCN_ONGOING);
    snprintf_res(cmd_data, "\tMTX_BCN_PKT_ID0      : %d\n", GET_MTX_BCN_PKT_ID0);
    snprintf_res(cmd_data, "\tMTX_BCN_PKT_ID1      : %d\n", GET_MTX_BCN_PKT_ID1);
    snprintf_res(cmd_data, "\tMTX_BCN_PKTID_CH_LOCK (deprecated): 0x%x\n\n", GET_MTX_BCN_PKTID_CH_LOCK);

    value = GET_MTX_BCN_TIMER;
    value1 = GET_MTX_BCN_PERIOD;
    snprintf_res(cmd_data, "HWBCN_TIMER\n");
    snprintf_res(cmd_data, "\tMTX_BCN_TIMER_EN                         : 0x%x\n", GET_MTX_BCN_TIMER_EN);
    snprintf_res(cmd_data, "\tMTX_BCN_TIMER(active interval count down): 0x%x=%d\n", value, value);
    snprintf_res(cmd_data, "\tMTX_BCN_PERIOD(fixed period in TBTT)     : 0x%x=%d\n\n", value1, value1);

    snprintf_res(cmd_data, "HWBCN_TSF\n");
    snprintf_res(cmd_data, "\tMTX_TSF_TIMER_EN         : 0x%x\n", GET_MTX_TSF_TIMER_EN);
    snprintf_res(cmd_data, "\tMTX_BCN_TSF              : 0x%x/0x%x(U/L)\n", GET_MTX_BCN_TSF_U, GET_MTX_BCN_TSF_L);
    snprintf_res(cmd_data, "\tMTX_TIME_STAMP_AUTO_FILL : 0x%x\n\n", GET_MTX_TIME_STAMP_AUTO_FILL);

    snprintf_res(cmd_data, "HWBCN_DTIM\n");
    snprintf_res(cmd_data, "\tMTX_DTIM_CNT_AUTO_FILL  : 0x%x\n", GET_MTX_DTIM_CNT_AUTO_FILL);
    snprintf_res(cmd_data, "\tMTX_DTIM_NUM            : %d\n", GET_MTX_DTIM_NUM);
    snprintf_res(cmd_data, "\tMTX_DTIM_OFST0          : %d\n", GET_MTX_DTIM_OFST0);
    snprintf_res(cmd_data, "\tMTX_DTIM_OFST1          : %d\n\n", GET_MTX_DTIM_OFST1);

    snprintf_res(cmd_data, "HWBCN_MISC_OPTION\n");
    snprintf_res(cmd_data, "\tMTX_BCN_AUTO_SEQ_NO        : 0x%x\n", GET_MTX_BCN_AUTO_SEQ_NO);
    snprintf_res(cmd_data, "\tTXQ5_DTIM_BEACON_BURST_MNG : 0x%x\n\n", GET_TXQ5_DTIM_BEACON_BURST_MNG);

    snprintf_res(cmd_data, "HWBCN_INT_DEPRECATED\n");
    snprintf_res(cmd_data, "\tMTX_INT_DTIM_NUM (deprecated) : %d\n", GET_MTX_INT_DTIM_NUM);
    snprintf_res(cmd_data, "\tMTX_INT_DTIM     (deprecated) : 0x%x\n", GET_MTX_INT_DTIM);
    snprintf_res(cmd_data, "\tMTX_INT_BCN      (deprecated) : 0x%x\n", GET_MTX_INT_BCN);
    snprintf_res(cmd_data, "\tMTX_EN_INT_BCN   (deprecated) : 0x%x\n", GET_MTX_EN_INT_BCN);
    snprintf_res(cmd_data, "\tMTX_EN_INT_DTIM  (deprecated) : 0x%x\n\n", GET_MTX_EN_INT_DTIM);
}

static void ssv6020_cmd_fw_left_letter(struct ssv_hw *sh)
{
#define PMU_RAM_09 ADR_PMU_RAM_09
#define PMU_RAM_10 ADR_PMU_RAM_10

    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    u32 log_buf=0, log_size=0;
    u32 s=0, value=0;
    u8 temp[5]={0};
    u32 dbg_msg = 0, fw_task_tracker = 0, msg_evt_tracker = 0;

    // reserved 32byte to fw
    // byte 0: mac task track
    // byte 1: mac task wait event count
    // byte 2: cmd task track
    // byte 3: cmd task wait event count
    // byte 4 ~ 7: msg event queue address
    SMAC_REG_READ(sh, PMU_RAM_09, &dbg_msg);

    SMAC_REG_READ(sh, dbg_msg, &fw_task_tracker);
    snprintf_res(cmd_data, "mac track:0x%x, mac wait event:%d\n",
            (fw_task_tracker & 0x000000ff), ((fw_task_tracker & 0x0000ff00) >> 8));
    snprintf_res(cmd_data, "cmd track:0x%x, cmd wait event:%d\n",
            ((fw_task_tracker & 0x00ff0000) >> 16), ((fw_task_tracker & 0xff000000) >> 24));

    SMAC_REG_READ(sh, dbg_msg+4, &msg_evt_tracker);
    snprintf_res(cmd_data, "msg evt address 0x%08x\n", msg_evt_tracker);


    log_buf = dbg_msg+32;
    SMAC_REG_READ(sh, PMU_RAM_10, &log_size);

    snprintf_res(cmd_data, "log_buf:0x%x,log_size:%d\n", log_buf, log_size);

    for(s=0;s<log_size;s+=4,log_buf+=sizeof(u32))
    {
        memset(temp,0,sizeof(temp));
        SMAC_REG_READ(sh, log_buf, &value);
        memcpy(temp,&value,4);
        snprintf_res(cmd_data, "%s",(char *)temp);
    }
    return ;
}

static void ssv6020_cmd_fw_pc(struct ssv_hw *sh)
{

    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    u32 value=0;
    SMAC_REG_READ(sh, ADR_N10_DBG1, &value);

    snprintf_res(cmd_data, "fw pc:0x%x\n", value);

    return ;
}

static void ssv6020_cmd_hwinfo(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_cmd_data *cmd_data = &sh->sc->cmd_data;
    /**
     *  hwmsg [mac|fw]
     */
    if (argc < 2) {
        snprintf_res(cmd_data, "hwmsg [mac|fw]\n\n");
        return;
    }

    if (!strcmp(argv[1], "mac")) {
        if ((argc == 3) && (!strcmp(argv[2], "beacon"))) {
            ssv6020_cmd_hwinfo_mac_beacon(sh);
        } else {
            snprintf_res(cmd_data, "hwmsg mac beacon\n\n");
        }
    }else if (!strcmp(argv[1], "fw")){
        if ((argc == 3) && (!strcmp(argv[2], "pc"))) {
            ssv6020_cmd_fw_pc(sh);
        } else if ((argc == 3) && (!strcmp(argv[2], "left_letter"))) {
            ssv6020_cmd_fw_left_letter(sh);
    } else {
            snprintf_res(cmd_data, "hwmsg fw [pc|left_letter]\n\n");
        }

    }else {
        snprintf_res(cmd_data, "hwmsg [mac|fw]\n\n");
    }
    return;
}

static void ssv6020_get_rd_id_adr(u32 *id_base_address)
{
    id_base_address[0] = ADR_RD_ID0;
    id_base_address[1] = ADR_RD_ID1;
    id_base_address[2] = ADR_RD_ID2;
    id_base_address[3] = ADR_RD_ID3;
}

static int ssv6020_burst_read_reg(struct ssv_hw *sh, u32 *addr, u32 *buf, u8 reg_amount)
{
    int ret = (-1), i;
    u32 dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);
    if (dev_type == SSV_HWIF_INTERFACE_SDIO) {
        ret = SMAC_BURST_REG_READ(sh, addr, buf, reg_amount);
    } else {
        for (i = 0 ; i < reg_amount ; i++) {
            ret = SMAC_REG_READ(sh, addr[i], &buf[i]);
            if (ret != 0) {
                printk("%s(): read 0x%08x failed.\n", __func__, addr[i]);
                break;
            }
        }
    }

    return ret;
}

static int ssv6020_burst_write_reg(struct ssv_hw *sh, u32 *addr, u32 *buf, u8 reg_amount)
{
    int ret = (-1), i;
    u32 dev_type = HCI_DEVICE_TYPE(sh->hci.hci_ctrl);
    if (dev_type == SSV_HWIF_INTERFACE_SDIO) {
        ret = SMAC_BURST_REG_WRITE(sh, addr, buf, reg_amount);
    } else {
        for (i = 0 ; i < reg_amount ; i++) {
            ret = SMAC_REG_WRITE(sh, addr[i], buf[i]);
            if (ret != 0) {
                printk("%s(): write 0x%08x failed.\n", __func__, addr[i]);
                break;
            }
        }
    }

    return ret;
}

static int ssv6020_auto_gen_nullpkt(struct ssv_hw *sh, int hwq)
{
    switch (hwq) {
        case 0:
            SET_TXQ0_Q_NULLDATAFRAME_GEN_EN(1);
            MDELAY(100);
            SET_TXQ0_Q_NULLDATAFRAME_GEN_EN(0);
            break;
        case 1:
            SET_TXQ1_Q_NULLDATAFRAME_GEN_EN(1);
            MDELAY(100);
            SET_TXQ1_Q_NULLDATAFRAME_GEN_EN(0);
            break;
        case 2:
            SET_TXQ2_Q_NULLDATAFRAME_GEN_EN(1);
            MDELAY(100);
            SET_TXQ2_Q_NULLDATAFRAME_GEN_EN(0);
            break;
        case 3:
            SET_TXQ3_Q_NULLDATAFRAME_GEN_EN(1);
            MDELAY(100);
            SET_TXQ3_Q_NULLDATAFRAME_GEN_EN(0);
            break;
        case 4:
            SET_TXQ4_Q_NULLDATAFRAME_GEN_EN(1);
            MDELAY(100);
            SET_TXQ4_Q_NULLDATAFRAME_GEN_EN(0);
            break;
        case 5:
            SET_TXQ5_Q_NULLDATAFRAME_GEN_EN(1);
            MDELAY(100);
            SET_TXQ5_Q_NULLDATAFRAME_GEN_EN(0);
            break;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

static void ssv6020_load_fw_enable_mcu(struct ssv_hw *sh)
{
    u32 regval = 0, status = 0;

    // After FW loaded, set IVB to 0, boot from SRAM, enable N10 clock, and release N10
    SET_SYS_N10_IVB_VAL(0);
    // reboot MCU

    status = GET_N10_CORE_CURRENT_PC;
    if (status & 0x10000000)
        SET_SYSCTRL_CMD(0x400B); // standby mode
    else
        SET_SYSCTRL_CMD(0x4008);

    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SYSCTRL_STATUS, &regval))
        {
            return;
        }
    }while(regval != 0x307);
}

static int ssv6020_load_fw_disable_mcu(struct ssv_hw *sh)
{
    // Before loading FW, reset N10
    if (ssv6020_reset_cpu(sh) != 0)
        return -1;

    return 0;
}

static int ssv6020_load_fw_set_status(struct ssv_hw *sh, u32 status)
{
    return SMAC_REG_WRITE(sh, ADR_TX_SEG, status);
}

static int ssv6020_load_fw_get_status(struct ssv_hw *sh, u32 *status)
{
    return SMAC_REG_READ(sh, ADR_TX_SEG, status);
}

static void ssv6020_load_fw_pre_config_device(struct ssv_hw *sh)
{
    HCI_LOAD_FW_PRE_CONFIG_DEVICE(sh->hci.hci_ctrl);
}

static void ssv6020_load_fw_post_config_device(struct ssv_hw *sh)
{
    HCI_LOAD_FW_POST_CONFIG_DEVICE(sh->hci.hci_ctrl);
}

// Reset CPU (after reset, CPU is stopped)
static int ssv6020_reset_cpu(struct ssv_hw *sh)
{
    // Keep original interrupt mask
    u32 org_int_mask = GET_MASK_TYPMCU_INT_MAP;
    u32 regval = 0, status = 0;

    /* Safly reset CPU: (Precondition: CPU must be alive)
     * Through sysctrl to make CPU enter standby first, then do CPU reset.
     */
    // Mask all interrupt for CPU, except SYSCTRL interrupt
    SET_MASK_TYPMCU_INT_MAP(0xffffffff);
    // reboot MCU from ROMCODE
    SET_SYS_N10_IVB_VAL(0x4);
    // Request CPU enter standby through SYSCTRL COMMAND
    // check status to decide that mcu enter standby mode
    status = GET_N10_CORE_CURRENT_PC;
    if (status & 0x10000000)
        SET_SYSCTRL_CMD(0x400B); // standby mode
    else
        SET_SYSCTRL_CMD(0x4008);

    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SYSCTRL_STATUS, &regval))
        {
            return -1;
        }
    }while(regval!=0x307);
    msleep(1);
    // Set original interrupt mask back
    SET_MASK_TYPMCU_INT_MAP(org_int_mask);

    return 0;
}

/* Set SRAM mapping mode to:
 * 0: ILM  64KB, DLM 128KB
 * 1: ILM 160KB, DLM 32KB
 */
static void ssv6020_set_sram_mode(struct ssv_hw *sh, enum SSV_SRAM_MODE mode)
{
    return;
}

static int ssv6020_get_sram_mode(struct ssv_hw *sh)
{
    return 1; // don't check sram mapping
}

static void ssv6020_enable_sram(struct ssv_hw *sh)
{
    u32 regval = 0;
    u32 sram_pg_ctrl0 = 0xfffff; //SRAM 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    u32 sram_pg_ctrl1 = 0x3fff; //SRAM 10, 11, 12, 13, 14, 15, 16

    if(-1 == SMAC_REG_WRITE(sh, ADR_SRAM_PG_CTRL0, sram_pg_ctrl0))
    {
        return;
    }
    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SRAM_PG_STATUS0, &regval))
        {
            return;
        }
    }while(regval != sram_pg_ctrl0);
    if(-1 == SMAC_REG_WRITE(sh, ADR_SRAM_PG_CTRL1, sram_pg_ctrl1))
    {
        return;
    }
    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SRAM_PG_STATUS1, &regval))
        {
            return;
        }
    }while(regval != sram_pg_ctrl1);
}

static void ssv6020_enable_usb_acc(struct ssv_softc *sc, u8 epnum)
{
    return; //Not support USB.
}

static void ssv6020_disable_usb_acc(struct ssv_softc *sc, u8 epnum)
{
    return; //Not support USB.
}

// Set Hardware support USB Link Power Management (LPM)
static void ssv6020_set_usb_lpm(struct ssv_softc *sc, u8 enable)
{
    return; //Not support USB.
}

static int ssv6020_jump_to_rom(struct ssv_softc *sc)
{
    return 0; //Not support USB.
}

static void ssv6020_init_gpio_cfg(struct ssv_hw *sh)
{
    return;
}

void ssv6020_set_on3_enable(struct ssv_hw *sh, bool val)
{
    u32 regval=0;
    //0x307:
    //bit[0]: RTC ready
    //bit[1]: oscillator ready
    //bit[2]: DPLL ready
    //bit[8~9]: PMU in chip on status
    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SYSCTRL_STATUS, &regval))
        {
            goto EXIT;
        }
    }while(regval!=0x307);

    if(-1 == SMAC_REG_WRITE(sh, ADR_POWER_ON_OFF_CTRL, 0xB0F))
    {
        goto EXIT;
    }
    if (val)
    {
        if(-1 == SMAC_REG_WRITE(sh, ADR_SYSCTRL_COMMAND, 0x808))
        {
            goto EXIT;
        }
    }
    else
    {
        if(-1 ==SMAC_REG_WRITE(sh, ADR_SYSCTRL_COMMAND, 0x408))
        {
            goto EXIT;
        }
    }

    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SYSCTRL_STATUS, &regval))
        {
            goto EXIT;
        }
    }while(regval!=0x307);

EXIT:
    return;
}

static int ssv6020_init_hci_rx_aggr(struct ssv_hw *sh)
{
    u32 regval = 0;
    int i = 0;

    if (sh->cfg.hw_caps & SSV6200_HW_CAP_HCI_TX_AGGR) {
        printk(KERN_ERR "Enable HCI TX aggregation\n");
        //SMAC_REG_WRITE(sh,0xc1000004, 0x40000003);
        SMAC_REG_SET_BITS(sh, ADR_HCI_TRX_MODE, (1<<HCI_TX_AGG_EN_SFT), HCI_TX_AGG_EN_MSK);
    }

    /*
     * HCI RX Aggregation
     * FLOW CONTROL
     * 1. STOP HCI RX
     * 2. CONFIRM RX STATUS
     * 3. ENABLE HCI RX AGGREGATION
     * 4. SET AGGREGATION LENGTH[LENGTH=AGGREGATION+MPDU]
     * 5. START HCI RX
     */
    if (sh->cfg.hw_caps & SSV6200_HW_CAP_HCI_RX_AGGR) {
        SMAC_REG_SET_BITS(sh, ADR_HCI_TRX_MODE, (0<<HCI_RX_EN_SFT), HCI_RX_EN_MSK);
        do {
            SMAC_REG_READ(sh, ADR_RX_PACKET_LENGTH_STATUS, &regval);
            regval &= HCI_RX_LEN_I_MSK;
            i++;
            if (i > 10000) {
                printk("CANNOT ENABLE HCI RX AGGREGATION!!!\n");
                WARN_ON(1);
                return -1;
            }
        } while (regval != 0);


        regval = 0;
        SMAC_REG_SET_BITS(sh, ADR_HCI_TRX_MODE, (1<<HCI_RX_FORM_1_SFT), HCI_RX_FORM_1_MSK);
        regval = (sh->cfg.hw_rx_agg_cnt << RX_AGG_CNT_SFT) |
            (sh->cfg.hw_rx_agg_method_3 << RX_AGG_METHOD_3_SFT) |
            (sh->cfg.hw_rx_agg_timer_reload << RX_AGG_TIMER_RELOAD_VALUE_SFT);
        SMAC_REG_WRITE(sh, ADR_FORCE_RX_AGGREGATION_MODE, regval);
        SMAC_REG_SET_BITS(sh, ADR_HCI_FORCE_PRE_BULK_IN, HCI_RX_AGGR_SIZE, HCI_BULK_IN_HOST_SIZE_MSK);
        SMAC_REG_SET_BITS(sh, ADR_HCI_TRX_MODE, (1<<HCI_RX_EN_SFT), HCI_RX_EN_MSK);

        sh->rx_mode = (sh->cfg.hw_rx_agg_method_3) ? RX_HW_AGG_MODE_METH3 : RX_HW_AGG_MODE;
    }

    return 0;
}

static void _sv6020c_dvdd11_trim_code(struct ssv_hw *sh)
{
    u32 volt_patch = 0;
#if 1
    // MP chip
    SMAC_REG_READ(sh, 0xD9003FE4, &volt_patch);
#else
    // for QA testing code.
    volt_patch = 0x00000000;
    //volt_patch = 0x00800000; // trim code +1
#endif
    // 0xD9003FE4 bit 23 is 1, setting 0xccb0b00c = 0x5858f980
    // 0xD9003FE4 bit 23 is 0, setting 0xccb0b00c = 0x5858f940
    switch((volt_patch >> 23)&0x3) {
        case 1:
            SMAC_REG_WRITE(sh, 0xCCB0B00C, 0x5858F980);
            break;
        default:
            SMAC_REG_WRITE(sh, 0xCCB0B00C, 0x5858F940);
            break;
    }
}

static void ssv6020_pre_load_rf_table(struct ssv_hw *sh)
{
    // XO cload adjust, XO_LDO[2]=0
    SMAC_REG_WRITE(sh, 0xCCB0B000, 0x24989813);
    // BBPLL LDO to 1V; , b15/16 from 01 to 11 , carefull xtal_freq setting is 26M,1111
    SMAC_REG_WRITE(sh, 0xCCB0B014, 0x0071F242);
    //BBPLL VCO BandSEL from 100 to 111 for best tunning range,1111;
    SMAC_REG_WRITE(sh, 0xCCB0B01C, 0x272AB195);
    // PMU default setting : LDO mode , stb 0.8v/sleep 0.6v
    // dcdc_rldr_uv_lv from 100 to 110,1.45v,vdd11_lv=000(slp mode 0.6v)
    _sv6020c_dvdd11_trim_code(sh); // 0xccb0b00c setting from _sv6020c_dvdd11_trim_code
    //SMAC_REG_WRITE(sh, 0xCCB0B00C, 0x5858F940);
    // discharge time~10T , 1A for sleep flow
    SMAC_REG_WRITE(sh, 0xCCB0B058, 0x00000A1A);
}

static void _ssv6020_set_volt_patch(struct ssv_hw *sh)
{
    u32 volt_patch = 0;
    u32 ft_check = 0;

#if 1
    // MP chip
    SMAC_REG_READ(sh, 0xD9003FF8, &volt_patch);
    SMAC_REG_READ(sh, 0xD9003FE4, &ft_check);
#else
    // for QA testing code.
    volt_patch = 0x00000000;
    //volt_patch = 0x40000000;
    //volt_patch = 0x80000000;
#endif
    if (ft_check & (1<<27)) {
        // new ft.
        switch((ft_check >> 25)&0x3) {
            case 1:
                SET_RG_DLDO_LV(5);
                break;
            case 2:
                SET_RG_DLDO_LV(6);
                break;
            default:
                break;
        }
    } else {
        // old ft.
        SET_RG_DLDO_LV(6);
    }
    // EMAW force to 1.
    SMAC_REG_WRITE(sh, 0xC0000188, 0x60402ca);

    printk("volt_patch=0x%x, ft_check=0x%x\n", volt_patch, ft_check);
}

static void _ssv6020_set_crystal_clk(struct ssv_hw *sh)
{
    //No need this anymore.
    //SET_RG_EN_IOTADC_160M(0);

    _ssv6020_set_volt_patch(sh);

    switch (sh->cfg.crystal_type) {
        case SSV6XXX_IQK_CFG_XTAL_16M:
            SET_RG_DP_XTAL_FREQ(0);
            SET_RG_SX_XTAL_FREQ(0);
            SET_RG_EN_DPL_MOD(0);
            break;
        case SSV6XXX_IQK_CFG_XTAL_24M:
            SET_RG_DP_XTAL_FREQ(1);
            SET_RG_SX_XTAL_FREQ(1);
            SET_RG_EN_DPL_MOD(0);
            break;
        case SSV6XXX_IQK_CFG_XTAL_26M:
            SET_RG_DP_XTAL_FREQ(2);
            SET_RG_SX_XTAL_FREQ(2);
            SET_RG_EN_DPL_MOD(1);
            break;
        case SSV6XXX_IQK_CFG_XTAL_40M:
            SET_RG_DP_XTAL_FREQ(3);
            SET_RG_SX_XTAL_FREQ(3);
            SET_RG_EN_DPL_MOD(0);
            break;
        case SSV6XXX_IQK_CFG_XTAL_12M:
            SET_RG_DP_XTAL_FREQ(4);
            SET_RG_SX_XTAL_FREQ(4);
            SET_RG_EN_DPL_MOD(0);
            break;
        case SSV6XXX_IQK_CFG_XTAL_20M:
            SET_RG_DP_XTAL_FREQ(5);
            SET_RG_SX_XTAL_FREQ(5);
            SET_RG_EN_DPL_MOD(0);
            break;
        case SSV6XXX_IQK_CFG_XTAL_25M:
            SET_RG_DP_XTAL_FREQ(6);
            SET_RG_SX_XTAL_FREQ(6);
            SET_RG_EN_DPL_MOD(1);
            break;
        case SSV6XXX_IQK_CFG_XTAL_32M:
            SET_RG_DP_XTAL_FREQ(7);
            SET_RG_SX_XTAL_FREQ(7);
            SET_RG_EN_DPL_MOD(0);
            break;
        default:
            printk("Please redefine xtal_clock(wifi.cfg)!!\n");
            WARN_ON(1);
            break;
    }
}

static void ssv6020_init_PLL(struct ssv_hw *sh)
{
    u32 regval = 0;
    u32 status = 0, sys_ctrl = 0;

    ssv6020_pre_load_rf_table(sh);
    _ssv6020_set_crystal_clk(sh);
    /* for turismo, it just needs to set register once , pll is initialized by hw auto */
    SET_RG_LOAD_RFTABLE_RDY(0x1);
    //while (GET_SYS_DPLL_ON != 0x1); //Check DPLL ready.
    //0x307:
    //bit[0]: RTC ready
    //bit[1]: oscillator ready
    //bit[2]: DPLL ready
    //bit[8~9]: PMU in chip on status
    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SYSCTRL_STATUS, &regval))
        {
            return;
        }
    }while(regval != 0x307);

    /* do clock switch */
    //For MCU clock 240MHz/480MHz(1), 320MHz(0) and others.
    SET_RG_BBPLL_MAC_480M_320M_SEL(0);
    /*
    (bit31-28) pwr_on digi_clk frequency
            XOSC(chip on)/RTC(sleep) (others value);
            40Mhz                    (value 4);
            80Mhz                    (value 8);
            160Mhz                   (value 9);

    (bit27-24) pwr_on fbus_clk frequency
            4'b000\? (clk_digi)
            4'b001\? (40Mhz)
            4'b01\?\? (80Mhz)
            4'b1000 (160Mhz)
            4'b1001 (240Mhz)
            4'b1010 (320Mhz)
            4'b1100 (480Mhz)
            others  (160Mhz)
    */
    //Bus clock 160MHz, MCU clock 480MHz: 0x9c0000cb
    //Bus clock 160MHz, MCU clock 320MHz: 0x9a0000cb
    //Bus clock 160MHz, MCU clock 160MHz: 0x910000cb
    //Bus clock 80MHz, MCU clock 160MHz: 0x880000cb
    //Bus clock 80MHz, MCU clock 80MHz: 0x810000cb
    //Bus clock 40MHz, MCU clock 80MHz: 0x440000cb
    //Bus clock 40MHz, MCU clock 40MHz: 0x410000cb

    status = GET_N10_CORE_CURRENT_PC;
    if (status & 0x10000000)
        sys_ctrl = 0x9a0000cb;  // standby mode
    else
        sys_ctrl = 0x9a0000c8;

    if(-1 == SMAC_REG_WRITE(sh, ADR_SYSCTRL_COMMAND, sys_ctrl))
    {
        return;
    }
    do{
        if(-1 == SMAC_REG_READ(sh, ADR_SYSCTRL_STATUS, &regval))
        {
            return;
        }
    }while(regval != 0x307);
}

static void ssv6020_set_crystal_clk(struct ssv_hw *sh)
{
    ssv6020_init_PLL(sh);
}

static void ssv6020_set_fpga_crystal_clk(struct ssv_hw *sh)
{
    int i = 0;

    // load turismoE ext pll table
    for( i = 0; i < sizeof(ssv6020_turismoE_ext_dpll_setting)/sizeof(ssv_cabrio_reg); i++) {
        REG32_W(ssv6020_turismoE_ext_dpll_setting[i].address,
                ssv6020_turismoE_ext_dpll_setting[i].data);
        udelay(50);
    }
    mdelay(1);

    SET_RG_LOAD_RFTABLE_RDY(true);
    mdelay(10);
    /* enable PHY clock */
    REG32_W(ADR_WIFI_PHY_COMMON_SYS_REG, 0x80010000);
    udelay(50);
    /* do clock switch */
    REG32_W(ADR_CLOCK_SELECTION, 0x00000004); //BUS clock 40MHz for FPGA
    mdelay(1);
}

static int ssv6020_reset_hw_mac(struct ssv_hw *sh)
{
    int i = 0;
    u32 regval = 0;

    SMAC_REG_WRITE(sh, ADR_BRG_SW_RST, 1 << MAC_SW_RST_SFT);
    do
    {
        SMAC_REG_READ(sh, ADR_BRG_SW_RST, &regval);
        i ++;
        if (i >10000) {
            printk("MAC reset fail !!!!\n");
            WARN_ON(1);
            return -1;
        }
    } while (regval != 0);

    // set mac clk frequency
    regval = GET_CLK_DIGI_SEL;
    switch (regval) {
        case 4: // 40M
            SET_MAC_CLK_FREQ(0);
            SET_PHYTXSTART_NCYCLE(13);
            SET_PRESCALER_US(40);
            break;
        case 8: // 80M
            SET_MAC_CLK_FREQ(1);
            SET_PHYTXSTART_NCYCLE(26);
            SET_PRESCALER_US(80);
            break;
        case 16: // 160M
            SET_MAC_CLK_FREQ(2);
            SET_PHYTXSTART_NCYCLE(52);
            SET_PRESCALER_US(160);
            break;
        default:
            printk("digi clk is invalid for mac!!!!\n");
            WARN_ON(1);
            return -1;
            break;
    }

    return 0;
}

static void ssv6020_wait_usb_rom_ready(struct ssv_hw *sh)
{
    return; //Not support USB.
}

static void ssv6020_detach_usb_hci(struct ssv_hw *sh)
{
    return; //Not support USB.
}

static void ssv6020_pll_chk(struct ssv_hw *sh)
{
    u32 regval;

    // check clock selection 160M
    regval = GET_CLK_DIGI_SEL;
    if (regval != 16) {
        HAL_INIT_PLL(sh); // reset pll
        SET_MAC_CLK_FREQ(2);
        SET_PHYTXSTART_NCYCLE(52);
        SET_PRESCALER_US(160);
    }
}

static void ssv6020_fpga_pll_chk(struct ssv_hw *sh)
{
    u32 regval;

    // check clock selection 40M
    regval = GET_CLK_DIGI_SEL;
    if (regval != 4) {
        HAL_INIT_PLL(sh); // reset pll
        SET_MAC_CLK_FREQ(0);
        SET_PHYTXSTART_NCYCLE(13);
        SET_PRESCALER_US(40);
    }
}

#ifdef CONFIG_STA_BCN_FILTER
void ssv6020_set_mrx_filter(struct ssv_hw *sh, u8 ID, bool enable, u16 rule_bitmap)
{
    u32 addr = ADR_MRX_FLT_EN0 + (ID * 4);
    u32 val = 0;

    SMAC_REG_READ(sh, addr, &val);
    if(enable) {
        val = (val | rule_bitmap);
    } else {
        val = (val & (~rule_bitmap));
    }

    SMAC_REG_WRITE(sh, addr, val);
}
#endif

void ssv_attach_ssv6020_mac(struct ssv_hal_ops *hal_ops)
{
    hal_ops->init_mac = ssv6020_init_mac;
    hal_ops->reset_sysplf = ssv6020_reset_sysplf;

    hal_ops->init_hw_sec_phy_table = ssv6020_init_hw_sec_phy_table;
    hal_ops->write_mac_ini = ssv6020_write_mac_ini;

    hal_ops->set_rx_flow = ssv6020_set_rx_flow;
    hal_ops->set_rx_ctrl_flow = ssv6020_set_rx_ctrl_flow;
    hal_ops->set_macaddr = ssv6020_set_macaddr;
    hal_ops->set_macaddr_2 = ssv6020_set_macaddr_2;
    hal_ops->set_bssid = ssv6020_set_bssid;
    hal_ops->get_ic_time_tag = ssv6020_get_ic_time_tag;
    hal_ops->get_chip_id = ssv6020_get_chip_id;
    hal_ops->set_mrx_mode = ssv6020_set_mrx_mode;
    hal_ops->get_mrx_mode = ssv6020_get_mrx_mode;

    hal_ops->save_hw_status = ssv6020_save_hw_status;
    hal_ops->restore_hw_config = ssv6020_restore_hw_config;
    hal_ops->set_hw_wsid = ssv6020_set_hw_wsid;
    hal_ops->del_hw_wsid = ssv6020_del_hw_wsid;

    hal_ops->set_aes_tkip_hw_crypto_group_key = ssv6020_set_aes_tkip_hw_crypto_group_key;
    hal_ops->write_pairwise_keyidx_to_hw = ssv6020_write_pairwise_keyidx_to_hw;
    hal_ops->write_group_keyidx_to_hw = ssv6020_write_hw_group_keyidx;
    hal_ops->write_pairwise_key_to_hw = ssv6020_write_pairwise_key_to_hw;
    hal_ops->write_group_key_to_hw = ssv6020_write_group_key_to_hw;
    hal_ops->write_key_to_hw = ssv6020_write_key_to_hw;
    hal_ops->set_pairwise_cipher_type = ssv6020_set_pairwise_cipher_type;
    hal_ops->set_group_cipher_type = ssv6020_set_group_cipher_type;
    hal_ops->set_wep_key = ssv6020_set_wep_key;
	hal_ops->set_wep_hw_crypto_key = ssv6020_set_wep_hw_crypto_key;

#ifdef CONFIG_PM
    hal_ops->save_clear_trap_reason = ssv6020_save_clear_trap_reason;
    hal_ops->restore_trap_reason = ssv6020_restore_trap_reason;
    hal_ops->ps_save_reset_rx_flow = ssv6020_ps_save_reset_rx_flow;
    hal_ops->ps_restore_rx_flow = ssv6020_ps_restore_rx_flow;
    hal_ops->pmu_awake = ssv6020_pmu_awake;
    hal_ops->ps_hold_on3 = ssv6020_ps_hold_on3;
#endif

    hal_ops->set_replay_ignore = ssv6020_set_replay_ignore;
    hal_ops->update_decision_table_6 = ssv6020_update_decision_table_6;
    hal_ops->update_decision_table = ssv6020_update_decision_table;
    hal_ops->get_fw_version = ssv6020_get_fw_version;
    hal_ops->set_op_mode = ssv6020_set_op_mode;
    hal_ops->set_dur_burst_sifs_g = ssv6020_set_dur_burst_sifs_g;
    hal_ops->set_dur_slot = ssv6020_set_dur_slot;
    hal_ops->set_sifs = ssv6020_set_sifs;
    hal_ops->set_qos_enable = ssv6020_set_qos_enable;
    hal_ops->set_wmm_param = ssv6020_set_wmm_param;
    hal_ops->update_page_id = ssv6020_update_page_id;

    hal_ops->alloc_pbuf = ssv6020_alloc_pbuf;
    hal_ops->free_pbuf = ssv6020_free_pbuf;
    hal_ops->ampdu_auto_crc_en = ssv6020_ampdu_auto_crc_en;
    hal_ops->set_rx_ba = ssv6020_set_rx_ba;
    hal_ops->read_efuse = ssv6020_read_efuse;
    hal_ops->chg_clk_src = ssv6020_chg_clk_src;

    hal_ops->beacon_loss_enable = ssv6020_beacon_loss_enable;
    hal_ops->beacon_loss_disable = ssv6020_beacon_loss_disable;
    hal_ops->beacon_loss_config = ssv6020_beacon_loss_config;

    hal_ops->update_txq_mask = ssv6020_update_txq_mask;
    hal_ops->readrg_hci_inq_info = ssv6020_readrg_hci_inq_info;
    hal_ops->readrg_txq_info = ssv6020_readrg_txq_info;

    hal_ops->dump_wsid = ssv6020_dump_wsid;
    hal_ops->dump_decision = ssv6020_dump_decision;
    hal_ops->get_ffout_cnt = ssv6020_get_ffout_cnt;
    hal_ops->get_in_ffcnt = ssv6020_get_in_ffcnt;
    hal_ops->read_ffout_cnt = ssv6020_read_ffout_cnt;
    hal_ops->read_in_ffcnt = ssv6020_read_in_ffcnt;
    hal_ops->read_id_len_threshold = ssv6020_read_id_len_threshold;
    hal_ops->read_allid_map = ssv6020_read_allid_map;
    hal_ops->read_txid_map = ssv6020_read_txid_map;
    hal_ops->read_rxid_map = ssv6020_read_rxid_map;
    hal_ops->read_wifi_halt_status = ssv6020_read_wifi_halt_status;
    hal_ops->read_tag_status = ssv6020_read_tag_status;
    hal_ops->cmd_mib = ssv6020_cmd_mib;
    hal_ops->cmd_power_saving = ssv6020_cmd_power_saving;
    hal_ops->cmd_hwinfo = ssv6020_cmd_hwinfo;
    hal_ops->get_rd_id_adr = ssv6020_get_rd_id_adr;
    hal_ops->burst_read_reg = ssv6020_burst_read_reg;
    hal_ops->burst_write_reg = ssv6020_burst_write_reg;
    hal_ops->auto_gen_nullpkt = ssv6020_auto_gen_nullpkt;

    hal_ops->load_fw_enable_mcu = ssv6020_load_fw_enable_mcu;
    hal_ops->load_fw_disable_mcu = ssv6020_load_fw_disable_mcu;
    hal_ops->load_fw_set_status = ssv6020_load_fw_set_status;
    hal_ops->load_fw_get_status = ssv6020_load_fw_get_status;
    hal_ops->load_fw_pre_config_device = ssv6020_load_fw_pre_config_device;
    hal_ops->load_fw_post_config_device = ssv6020_load_fw_post_config_device;
    hal_ops->reset_cpu = ssv6020_reset_cpu;
    hal_ops->set_sram_mode = ssv6020_set_sram_mode;
    hal_ops->get_sram_mode = ssv6020_get_sram_mode;
    hal_ops->enable_sram = ssv6020_enable_sram;
    hal_ops->enable_usb_acc = ssv6020_enable_usb_acc;
    hal_ops->disable_usb_acc = ssv6020_disable_usb_acc;
    hal_ops->set_usb_lpm = ssv6020_set_usb_lpm;
    hal_ops->jump_to_rom = ssv6020_jump_to_rom;
    hal_ops->init_gpio_cfg = ssv6020_init_gpio_cfg;
    hal_ops->set_on3_enable = ssv6020_set_on3_enable;
    hal_ops->init_hci_rx_aggr = ssv6020_init_hci_rx_aggr;
    hal_ops->reset_hw_mac = ssv6020_reset_hw_mac;
    hal_ops->set_crystal_clk = ssv6020_set_crystal_clk;
    hal_ops->wait_usb_rom_ready = ssv6020_wait_usb_rom_ready;
    hal_ops->detach_usb_hci = ssv6020_detach_usb_hci;
    hal_ops->pll_chk = ssv6020_pll_chk;
#ifdef CONFIG_STA_BCN_FILTER
    hal_ops->set_mrx_filter = ssv6020_set_mrx_filter;
#endif

}

void ssv_attach_ssv6020_fpga_mac(struct ssv_hal_ops *hal_ops)
{
    hal_ops->set_crystal_clk = ssv6020_set_fpga_crystal_clk;
    hal_ops->pll_chk = ssv6020_fpga_pll_chk;
}

/* Used before ssv_hw is initialized */
static int ssv_hwif_read_reg (struct ssv_softc *sc, u32 addr, u32 *val)
{
    struct ssv6xxx_platform_data *priv = sc->dev->platform_data;
    return priv->ops->readreg(sc->dev, addr, val);
}

static u64 _ssv6020_get_ic_time_tag(struct ssv_softc *sc)
{
    u32 regval;
    u64 ic_time_tag;

    ssv_hwif_read_reg(sc, ADR_CHIP_DATE_YYYYMMDD, &regval);
    ic_time_tag = ((u64)regval<<32);
    ssv_hwif_read_reg(sc, ADR_CHIP_DATE_00HHMMSS, &regval);
    ic_time_tag |= (regval);

    return ic_time_tag;
}

void ssv_attach_ssv6020(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops)
{
    u32  regval, chip_type;
    u64  ic_time_tag;

    // get ic time tag to identify phy v4.
    ic_time_tag = _ssv6020_get_ic_time_tag(sc);

    printk(KERN_INFO"Load SSV6020 common code\n");
    ssv_attach_ssv6020_common(hal_ops);
    printk(KERN_INFO"Load SSV6020 HAL MAC function \n");
    ssv_attach_ssv6020_mac(hal_ops);
    printk(KERN_INFO"Load SSV6020 HAL PHY function \n");
    ssv_attach_ssv6020_phy(hal_ops);
    printk(KERN_INFO"Load SSV6020 HAL BB-RF function \n");
    ssv_attach_ssv6020_turismoE_BBRF(hal_ops);

    ssv_hwif_read_reg(sc, ADR_CHIP_TYPE_VER, &regval);
    chip_type = regval >>24;

    printk(KERN_INFO"Chip type %x\n", chip_type);

    if (chip_type == CHIP_TYPE_CHIP){
        printk(KERN_INFO"Load SSV6020 HAL ASIC EXT BB-RF function \n");
        ssv_attach_ssv6020_turismoE_ext_BBRF(hal_ops);
    } else {
        printk(KERN_INFO"Load SSV6020 HAL FPGA MAC function \n");
        ssv_attach_ssv6020_fpga_mac(hal_ops);
        printk(KERN_INFO"Load SSV6020 HAL FPGA BB-RF function \n");
        ssv_attach_ssv6020_turismoE_fpga_BBRF(hal_ops);
    }
}

#endif
