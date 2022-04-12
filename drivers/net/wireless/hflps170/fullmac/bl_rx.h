/**
 ******************************************************************************
 *
 * @file bl_rx.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#ifndef _BL_RX_H_
#define _BL_RX_H_

enum rx_status_bits
{
    /// The buffer can be forwarded to the networking stack
    RX_STAT_FORWARD = 1 << 0,
    /// A new buffer has to be allocated
    RX_STAT_ALLOC = 1 << 1,
    /// The buffer has to be deleted
    RX_STAT_DELETE = 1 << 2,
    /// The length of the buffer has to be updated
    RX_STAT_LEN_UPDATE = 1 << 3,
    /// The length in the Ethernet header has to be updated
    RX_STAT_ETH_LEN_UPDATE = 1 << 4,
    /// Simple copy
    RX_STAT_COPY = 1 << 5,
};


/*
 * Decryption status subfields.
 * {
 */
#define BL_RX_HD_DECR_UNENC           0 // Frame unencrypted
#define BL_RX_HD_DECR_ICVFAIL         1 // WEP/TKIP ICV failure
#define BL_RX_HD_DECR_CCMPFAIL        2 // CCMP failure
#define BL_RX_HD_DECR_AMSDUDISCARD    3 // A-MSDU discarded at HW
#define BL_RX_HD_DECR_NULLKEY         4 // NULL key found
#define BL_RX_HD_DECR_WEPSUCCESS      5 // Security type WEP
#define BL_RX_HD_DECR_TKIPSUCCESS     6 // Security type TKIP
#define BL_RX_HD_DECR_CCMPSUCCESS     7 // Security type CCMP
// @}

struct hw_vect {
    /** Total length for the MPDU transfer */
    u32 len                   :16;

    u32 reserved              : 8;

    /** AMPDU Status Information */
    u32 mpdu_cnt              : 6;
    u32 ampdu_cnt             : 2;


    /** TSF Low */
    __le32 tsf_lo;
    /** TSF High */
    __le32 tsf_hi;

    /** Receive Vector 1a */
    u32    leg_length         :12;
    u32    leg_rate           : 4;
    u32    ht_length          :16;

    /** Receive Vector 1b */
    u32    _ht_length         : 4; // FIXME
    u32    short_gi           : 1;
    u32    stbc               : 2;
    u32    smoothing          : 1;
    u32    mcs                : 7;
    u32    pre_type           : 1;
    u32    format_mod         : 3;
    u32    ch_bw              : 2;
    u32    n_sts              : 3;
    u32    lsig_valid         : 1;
    u32    sounding           : 1;
    u32    num_extn_ss        : 2;
    u32    aggregation        : 1;
    u32    fec_coding         : 1;
    u32    dyn_bw             : 1;
    u32    doze_not_allowed   : 1;

    /** Receive Vector 1c */
    u32    antenna_set        : 8;
    u32    partial_aid        : 9;
    u32    group_id           : 6;
    u32    reserved_1c        : 1;
    s32    rssi1              : 8;

    /** Receive Vector 1d */
    s32    rssi2              : 8;
    s32    rssi3              : 8;
    s32    rssi4              : 8;
    u32    reserved_1d        : 8;

    /** Receive Vector 2a */
    u32    rcpi               : 8;
    u32    evm1               : 8;
    u32    evm2               : 8;
    u32    evm3               : 8;

    /** Receive Vector 2b */
    u32    evm4               : 8;
    u32    reserved2b_1       : 8;
    u32    reserved2b_2       : 8;
    u32    reserved2b_3       : 8;

    /** Status **/
    u32    rx_vect2_valid     : 1;
    u32    resp_frame         : 1;
    /** Decryption Status */
    u32    decr_status        : 3;
    u32    rx_fifo_oflow      : 1;

    /** Frame Unsuccessful */
    u32    undef_err          : 1;
    u32    phy_err            : 1;
    u32    fcs_err            : 1;
    u32    addr_mismatch      : 1;
    u32    ga_frame           : 1;
    u32    current_ac         : 2;

    u32    frm_successful_rx  : 1;
    /** Descriptor Done  */
    u32    desc_done_rx       : 1;
    /** Key Storage RAM Index */
    u32    key_sram_index     : 10;
    /** Key Storage RAM Index Valid */
    u32    key_sram_v         : 1;
    u32    type               : 2;
    u32    subtype            : 4;
};

struct hw_rxhdr {
    /** RX vector */
    struct hw_vect hwvect;

    /** PHY channel information 1 */
    u32    phy_band           : 8;
    u32    phy_channel_type   : 8;
    u32    phy_prim20_freq    : 16;
    /** PHY channel information 2 */
    u32    phy_center1_freq   : 16;
    u32    phy_center2_freq   : 16;
    /** RX flags */
    u32    flags_is_amsdu     : 1;
    u32    flags_is_80211_mpdu: 1;
    u32    flags_is_4addr     : 1;
    u32    flags_new_peer     : 1;
    u32    flags_user_prio    : 3;
    u32    flags_rsvd0        : 1;
    u32    flags_vif_idx      : 8;    // 0xFF if invalid VIF index
    u32    flags_sta_idx      : 8;    // 0xFF if invalid STA index
    u32    flags_dst_idx      : 8;    // 0xFF if unknown destination STA
    u16    sn;
	u16	   tid;
}__packed;

struct bl_agg_reodr_msg {
    u16 sn;
    u8 sta_idx;
    u8 tid;
    u8 status;
	u8 num;
}__packed;

extern const u8 legrates_lut[];
#define BL_FC_GET_TYPE(fc)	(((fc) & 0x000c) >> 2)
#define BL_FC_GET_STYPE(fc)	(((fc) & 0x00f0) >> 4)

#define AGG_REORD_MSG_MAX_NUM 4096


u8 bl_rxdataind(void *pthis, void *hostid);

#endif /* _BL_RX_H_ */
