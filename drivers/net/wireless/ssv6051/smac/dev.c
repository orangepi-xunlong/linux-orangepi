/*
 * Copyright (c) 2015 South Silicon Valley Microelectronics Inc.
 * Copyright (c) 2015 iComm Corporation
 *
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/nl80211.h>
#include <linux/etherdevice.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/ktime.h>

#include <net/mac80211.h>
#include <ssv6200.h>
#include <hci/hctrl.h>
#include "linux_80211.h"
#include "lib.h"
#include "ssv_rc.h"
#include "ssv_ht_rc.h"
#include "dev.h"
#include "ap.h"
#include "init.h"
#include "p2p.h"
#ifdef CONFIG_SSV6XXX_DEBUGFS
#include "ssv6xxx_debugfs.h"
#endif
struct rssi_res_st rssi_res, *p_rssi_res;
#define NO_USE_RXQ_LOCK
#ifndef WLAN_CIPHER_SUITE_SMS4
#define WLAN_CIPHER_SUITE_SMS4 0x00147201
#endif
#define MAX_TX_Q_LEN (64)
#define LOW_TX_Q_LEN (MAX_TX_Q_LEN/2)
static u16 bits_per_symbol[][2] = {
	{26, 54},
	{52, 108},
	{78, 162},
	{104, 216},
	{156, 324},
	{208, 432},
	{234, 486},
	{260, 540},
};

#ifdef CONFIG_DEBUG_SKB_TIMESTAMP
extern struct ssv6xxx_hci_ctrl *ssv_dbg_ctrl_hci;
extern unsigned int cal_duration_of_ampdu(struct sk_buff *ampdu_skb, int stage);
#endif
struct ssv6xxx_calib_table {
	u16 channel_id;
	u32 rf_ctrl_N;
	u32 rf_ctrl_F;
	u16 rf_precision_default;
};
static void _process_rx_q(struct ssv_softc *sc, struct sk_buff_head *rx_q,
			  spinlock_t * rx_q_lock);
static u32 _process_tx_done(struct ssv_softc *sc);

void ssv6xxx_txbuf_free_skb(struct sk_buff *skb, void *args)
{
	struct ssv_softc *sc = (struct ssv_softc *)args;
	if (!skb)
		return;
	ieee80211_free_txskb(sc->hw, skb);
}

#define ADDRESS_OFFSET 16
#define HW_ID_OFFSET 7
#define CH0_FULL_MASK CH0_FULL_MSK
#define MAX_FAIL_COUNT 100
#define MAX_RETRY_COUNT 20
inline bool ssv6xxx_mcu_input_full(struct ssv_softc *sc)
{
	u32 regval = 0;
	SMAC_REG_READ(sc->sh, ADR_MCU_STATUS, &regval);
	return CH0_FULL_MASK & regval;
}

u32 ssv6xxx_pbuf_alloc(struct ssv_softc *sc, int size, int type)
{
	u32 regval, pad;
	int cnt = MAX_RETRY_COUNT;
	int page_cnt =
	    (size + ((1 << HW_MMU_PAGE_SHIFT) - 1)) >> HW_MMU_PAGE_SHIFT;
	regval = 0;
	mutex_lock(&sc->mem_mutex);
	pad = size % 4;
	size += pad;
	do {
		SMAC_REG_WRITE(sc->sh, ADR_WR_ALC, (size | (type << 16)));
		SMAC_REG_READ(sc->sh, ADR_WR_ALC, &regval);
		if (regval == 0) {
			cnt--;
			msleep(1);
		} else
			break;
	} while (cnt);
	if (type == TX_BUF) {
		sc->sh->tx_page_available -= page_cnt;
		sc->sh->page_count[PACKET_ADDR_2_ID(regval)] = page_cnt;
	}
	mutex_unlock(&sc->mem_mutex);
	if (regval == 0)
		dev_err(sc->dev,
			"Failed to allocate packet buffer of %d bytes in %d type.",
			size, type);
	else {
		dev_dbg(sc->dev,
			"Allocated %d type packet buffer of size %d (%d) at address %x.\n",
			type, size, page_cnt, regval);
	}
	return regval;
}

bool ssv6xxx_pbuf_free(struct ssv_softc *sc, u32 pbuf_addr)
{
	u32 regval = 0;
	u16 failCount = 0;
	u8 *p_tx_page_cnt = &sc->sh->page_count[PACKET_ADDR_2_ID(pbuf_addr)];
	while (ssv6xxx_mcu_input_full(sc)) {
		if (failCount++ < 1000)
			continue;
		dev_err(sc->dev, "Error in mailbox block after %d iterations\n", failCount);
		return false;
	}
	mutex_lock(&sc->mem_mutex);
	regval =
	    ((M_ENG_TRASH_CAN << HW_ID_OFFSET) | (pbuf_addr >> ADDRESS_OFFSET));
	SMAC_REG_WRITE(sc->sh, ADR_CH0_TRIG_1, regval);
	if (*p_tx_page_cnt) {
		sc->sh->tx_page_available += *p_tx_page_cnt;
		*p_tx_page_cnt = 0;
	}
	mutex_unlock(&sc->mem_mutex);
	return true;
}

static const struct ssv6xxx_calib_table vt_tbl[SSV6XXX_IQK_CFG_XTAL_MAX][14] = {
	{
	 {1, 0xB9, 0x89D89E, 3859},
	 {2, 0xB9, 0xEC4EC5, 3867},
	 {3, 0xBA, 0x4EC4EC, 3875},
	 {4, 0xBA, 0xB13B14, 3883},
	 {5, 0xBB, 0x13B13B, 3891},
	 {6, 0xBB, 0x762762, 3899},
	 {7, 0xBB, 0xD89D8A, 3907},
	 {8, 0xBC, 0x3B13B1, 3915},
	 {9, 0xBC, 0x9D89D9, 3923},
	 {10, 0xBD, 0x000000, 3931},
	 {11, 0xBD, 0x627627, 3939},
	 {12, 0xBD, 0xC4EC4F, 3947},
	 {13, 0xBE, 0x276276, 3955},
	 {14, 0xBF, 0x13B13B, 3974},
	 },
	{
	 {1, 0xf1, 0x333333, 3859},
	 {2, 0xf1, 0xB33333, 3867},
	 {3, 0xf2, 0x333333, 3875},
	 {4, 0xf2, 0xB33333, 3883},
	 {5, 0xf3, 0x333333, 3891},
	 {6, 0xf3, 0xB33333, 3899},
	 {7, 0xf4, 0x333333, 3907},
	 {8, 0xf4, 0xB33333, 3915},
	 {9, 0xf5, 0x333333, 3923},
	 {10, 0xf5, 0xB33333, 3931},
	 {11, 0xf6, 0x333333, 3939},
	 {12, 0xf6, 0xB33333, 3947},
	 {13, 0xf7, 0x333333, 3955},
	 {14, 0xf8, 0x666666, 3974},
	 },
	{
	 {1, 0xC9, 0x000000, 3859},
	 {2, 0xC9, 0x6AAAAB, 3867},
	 {3, 0xC9, 0xD55555, 3875},
	 {4, 0xCA, 0x400000, 3883},
	 {5, 0xCA, 0xAAAAAB, 3891},
	 {6, 0xCB, 0x155555, 3899},
	 {7, 0xCB, 0x800000, 3907},
	 {8, 0xCB, 0xEAAAAB, 3915},
	 {9, 0xCC, 0x555555, 3923},
	 {10, 0xCC, 0xC00000, 3931},
	 {11, 0xCD, 0x2AAAAB, 3939},
	 {12, 0xCD, 0x955555, 3947},
	 {13, 0xCE, 0x000000, 3955},
	 {14, 0xCF, 0x000000, 3974},
	 }
};

#define FAIL_MAX 100
#define RETRY_MAX 20
int ssv6xxx_set_channel(struct ssv_softc *sc, int ch)
{
	struct ssv_hw *sh = sc->sh;
	int retry_cnt, fail_cnt = 0;
	u32 regval;
	int ret = -1;
	int chidx;
	bool chidx_vld = 0;
	dev_dbg(sc->dev, "Setting channel to %d\n", ch);
	if ((sh->cfg.chip_identity == SSV6051Z)
	    || (sc->sh->cfg.chip_identity == SSV6051P)) {
		if ((ch == 13) || (ch == 14)) {
			if (sh->ipd_channel_touch == 0) {
				for (chidx = 0; chidx < sh->ch_cfg_size;
				     chidx++) {
					SMAC_REG_WRITE(sh,
						       sh->p_ch_cfg[chidx].
						       reg_addr,
						       sh->p_ch_cfg[chidx].
						       ch13_14_value);
				}
				sh->ipd_channel_touch = 1;
			}
		} else {
			if (sh->ipd_channel_touch) {
				for (chidx = 0; chidx < sh->ch_cfg_size;
				     chidx++) {
					SMAC_REG_WRITE(sh,
						       sh->p_ch_cfg[chidx].
						       reg_addr,
						       sh->p_ch_cfg[chidx].
						       ch1_12_value);
				}
				sh->ipd_channel_touch = 0;
			}
		}
	}
	for (chidx = 0; chidx < 14; chidx++) {
		if (vt_tbl[sh->cfg.crystal_type][chidx].channel_id == ch) {
			chidx_vld = 1;
			break;
		}
	}
	if (chidx_vld == 0) {
		dev_dbg(sc->dev, "%s(): fail! channel_id not found in vt_tbl\n",
			__FUNCTION__);
		goto exit;
	}
	if ((ret = ssv6xxx_rf_disable(sc->sh)) != 0)
		goto exit;
	do {
		if ((sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_26M)
		    || (sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_24M)) {
			if ((ret =
			     SMAC_REG_SET_BITS(sc->sh, ADR_SYN_DIV_SDM_XOSC,
					       (0x00 << 13),
					       (0x01 << 13))) != 0)
				break;
		} else if (sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_40M) {
			if ((ret =
			     SMAC_REG_SET_BITS(sc->sh, ADR_SYN_DIV_SDM_XOSC,
					       (0x01 << 13),
					       (0x01 << 13))) != 0)
				break;
		} else {
			dev_warn(sc->dev, "Illegal crystal setting in ssv6xxx_set_channel\n");
			BUG_ON(1);
		}
		if ((ret = SMAC_REG_SET_BITS(sc->sh, ADR_SX_LCK_BIN_REGISTERS_I,
					     (0x01 << 19), (0x01 << 19))) != 0)
			break;
		regval = vt_tbl[sh->cfg.crystal_type][chidx].rf_ctrl_F;
		if ((ret = SMAC_REG_SET_BITS(sc->sh, ADR_SYN_REGISTER_1,
					     (regval << 0),
					     (0x00ffffff << 0))) != 0)
			break;
		regval = vt_tbl[sh->cfg.crystal_type][chidx].rf_ctrl_N;
		if ((ret = SMAC_REG_SET_BITS(sc->sh, ADR_SYN_REGISTER_2,
					     (regval << 0),
					     (0x07ff << 0))) != 0)
			break;
		if ((ret =
		     SMAC_REG_READ(sc->sh, ADR_SX_LCK_BIN_REGISTERS_I,
				   &regval)) != 0)
			break;
		regval =
		    vt_tbl[sh->cfg.crystal_type][chidx].rf_precision_default;
		if ((ret =
		     SMAC_REG_SET_BITS(sc->sh, ADR_SX_LCK_BIN_REGISTERS_II,
				       (regval << 0), (0x1fff << 0))) != 0)
			break;
		if ((ret = SMAC_REG_SET_BITS(sc->sh, ADR_MANUAL_ENABLE_REGISTER,
					     (0x00 << 14), (0x01 << 14))) != 0)
			break;
		if ((ret = SMAC_REG_SET_BITS(sc->sh, ADR_MANUAL_ENABLE_REGISTER,
					     (0x01 << 14), (0x01 << 14))) != 0)
			break;
		retry_cnt = 0;
		do {
			mdelay(1);
			if ((ret =
			     SMAC_REG_READ(sc->sh, ADR_READ_ONLY_FLAGS_1,
					   &regval)) != 0)
				break;
			if (regval & 0x00000002) {
				if ((ret =
				     SMAC_REG_READ(sc->sh,
						   ADR_READ_ONLY_FLAGS_2,
						   &regval)) != 0)
					break;
				ret = ssv6xxx_rf_enable(sc->sh);
				//dev_info(sc->dev, "Lock to channel %d ([0xce010098]=%x)!!\n", vt_tbl[sh->cfg.crystal_type][chidx].channel_id, regval);
				sc->hw_chan = ch;
				goto exit;
			}
			retry_cnt++;
		}
		while (retry_cnt < RETRY_MAX);
		fail_cnt++;
		dev_warn(sc->dev, "calibation fail after %d iterations\n", fail_cnt);
	}
	while ((fail_cnt < FAIL_MAX) && (ret == 0));
 exit:
	if (ch == 14 && regval == 0xff0) {
		SMAC_IFC_RESET(sc->sh);
		ssv6xxx_restart_hw(sc);
	}
	if (ch <= 7) {
		if (sh->cfg.tx_power_index_1) {
			SMAC_REG_READ(sc->sh, ADR_RX_TX_FSM_REGISTER, &regval);
			regval &= RG_TX_GAIN_OFFSET_I_MSK;
			regval |=
			    (sh->cfg.tx_power_index_1 << RG_TX_GAIN_OFFSET_SFT);
			SMAC_REG_WRITE(sc->sh, ADR_RX_TX_FSM_REGISTER, regval);
		} else if (sh->cfg.tx_power_index_2) {
			SMAC_REG_READ(sc->sh, ADR_RX_TX_FSM_REGISTER, &regval);
			regval &= RG_TX_GAIN_OFFSET_I_MSK;
			SMAC_REG_WRITE(sc->sh, ADR_RX_TX_FSM_REGISTER, regval);
		}
	} else {
		if (sh->cfg.tx_power_index_2) {
			SMAC_REG_READ(sc->sh, ADR_RX_TX_FSM_REGISTER, &regval);
			regval &= RG_TX_GAIN_OFFSET_I_MSK;
			regval |=
			    (sh->cfg.tx_power_index_2 << RG_TX_GAIN_OFFSET_SFT);
			SMAC_REG_WRITE(sc->sh, ADR_RX_TX_FSM_REGISTER, regval);
		} else if (sh->cfg.tx_power_index_1) {
			SMAC_REG_READ(sc->sh, ADR_RX_TX_FSM_REGISTER, &regval);
			regval &= RG_TX_GAIN_OFFSET_I_MSK;
			SMAC_REG_WRITE(sc->sh, ADR_RX_TX_FSM_REGISTER, regval);
		}
	}
	return ret;
}

#ifdef CONFIG_SSV_SMARTLINK
int ssv6xxx_get_channel(struct ssv_softc *sc, int *pch)
{
	*pch = sc->hw_chan;
	return 0;
}

int ssv6xxx_set_promisc(struct ssv_softc *sc, int accept)
{
	u32 val = 0;
	if (accept) {
		val = 0x2;
	} else {
		val = 0x3;
	}
	SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_TB13, val);
	return 0;
}

int ssv6xxx_get_promisc(struct ssv_softc *sc, int *paccept)
{
	u32 val = 0;
	SMAC_REG_READ(sc->sh, ADR_MRX_FLT_TB13, &val);
	if (val == 0x2) {
		*paccept = 1;
	} else {
		*paccept = 0;
	}
	return 0;
}
#endif
int ssv6xxx_rf_enable(struct ssv_hw *sh)
{
	return SMAC_REG_SET_BITS(sh, 0xce010000, (0x02 << 12), (0x03 << 12)
	    );
}

int ssv6xxx_rf_disable(struct ssv_hw *sh)
{
	return SMAC_REG_SET_BITS(sh, 0xce010000, (0x01 << 12), (0x03 << 12)
	    );
}

int ssv6xxx_update_decision_table(struct ssv_softc *sc)
{
	int i;
	for (i = 0; i < MAC_DECITBL1_SIZE; i++) {
		SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_TB0 + i * 4,
			       sc->mac_deci_tbl[i]);
		SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_TB0 + i * 4,
				 sc->mac_deci_tbl[i]);
	}
	for (i = 0; i < MAC_DECITBL2_SIZE; i++) {
		SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN0 + i * 4,
			       sc->mac_deci_tbl[i + MAC_DECITBL1_SIZE]);
		SMAC_REG_CONFIRM(sc->sh, ADR_MRX_FLT_EN0 + i * 4,
				 sc->mac_deci_tbl[i + MAC_DECITBL1_SIZE]);
	}
	return 0;
}

static int ssv6xxx_frame_hdrlen(struct ieee80211_hdr *hdr, bool is_ht)
{
#define CTRL_FRAME_INDEX(fc) ((hdr->frame_control-IEEE80211_STYPE_BACK_REQ)>>4)
	u16 fc, CTRL_FLEN[] = { 16, 16, 16, 16, 10, 10, 16, 16 };
	int hdr_len = 24;
	fc = hdr->frame_control;
	if (ieee80211_is_ctl(fc))
		hdr_len = CTRL_FLEN[CTRL_FRAME_INDEX(fc)];
	else if (ieee80211_is_mgmt(fc)) {
		if (ieee80211_has_order(fc))
			hdr_len += ((is_ht == 1) ? 4 : 0);
	} else {
		if (ieee80211_has_a4(fc))
			hdr_len += 6;
		if (ieee80211_is_data_qos(fc)) {
			hdr_len += 2;
			if (ieee80211_has_order(hdr->frame_control) &&
			    is_ht == true)
				hdr_len += 4;
		}
	}
	return hdr_len;
}

static u32 ssv6xxx_ht_txtime(u8 rix, int pktlen, int width,
			     int half_gi, bool is_gf)
{
	u32 nbits, nsymbits, duration, nsymbols;
	int streams;
	streams = 1;
	nbits = (pktlen << 3) + OFDM_PLCP_BITS;
	nsymbits = bits_per_symbol[rix % 8][width] * streams;
	nsymbols = (nbits + nsymbits - 1) / nsymbits;
	if (!half_gi)
		duration = SYMBOL_TIME(nsymbols);
	else {
		if (!is_gf)
			duration =
			    DIV_ROUND_UP(SYMBOL_TIME_HALFGI(nsymbols), 4) << 2;
		else
			duration = SYMBOL_TIME_HALFGI(nsymbols);
	}
	duration +=
	    L_STF + L_LTF + L_SIG + HT_SIG + HT_STF + HT_LTF(streams) +
	    HT_SIGNAL_EXT;
	if (is_gf)
		duration -= 12;
	duration += HT_SIFS_TIME;
	return duration;
}

static u32 ssv6xxx_non_ht_txtime(u8 phy, int kbps,
				 u32 frameLen, bool shortPreamble)
{
	u32 bits_per_symbol, num_bits, num_symbols;
	u32 phy_time, tx_time;
	if (kbps == 0)
		return 0;
	switch (phy) {
	case WLAN_RC_PHY_CCK:
		phy_time = CCK_PREAMBLE_BITS + CCK_PLCP_BITS;
		if (shortPreamble)
			phy_time >>= 1;
		num_bits = frameLen << 3;
		tx_time = CCK_SIFS_TIME + phy_time + ((num_bits * 1000) / kbps);
		break;
	case WLAN_RC_PHY_OFDM:
		bits_per_symbol = (kbps * OFDM_SYMBOL_TIME) / 1000;
		num_bits = OFDM_PLCP_BITS + (frameLen << 3);
		num_symbols = DIV_ROUND_UP(num_bits, bits_per_symbol);
		tx_time = OFDM_SIFS_TIME + OFDM_PREAMBLE_TIME
		    + (num_symbols * OFDM_SYMBOL_TIME);
		break;
	default:
		pr_err("ssv6051: unknown phy %u\n", phy);
		BUG_ON(1);
		tx_time = 0;
		break;
	}
	return tx_time;
}

static u32 ssv6xxx_set_frame_duration(struct ieee80211_tx_info *info,
				      struct ssv_rate_info *ssv_rate, u16 len,
				      struct ssv6200_tx_desc *tx_desc,
				      struct fw_rc_retry_params *rc_params,
				      struct ssv_softc *sc)
{
	struct ieee80211_tx_rate *tx_drate;
	u32 frame_time = 0, ack_time = 0, rts_cts_nav = 0, frame_consume_time =
	    0;
	u32 l_length = 0, drate_kbps = 0, crate_kbps = 0;
	bool ctrl_short_preamble = false, is_sgi, is_ht40;
	bool is_ht, is_gf;
	int d_phy, c_phy, nRCParams, mcsidx;
	struct ssv_rate_ctrl *ssv_rc = NULL;
	tx_drate = &info->control.rates[0];
	is_sgi = !!(tx_drate->flags & IEEE80211_TX_RC_SHORT_GI);
	is_ht40 = !!(tx_drate->flags & IEEE80211_TX_RC_40_MHZ_WIDTH);
	is_ht = !!(tx_drate->flags & IEEE80211_TX_RC_MCS);
	is_gf = !!(tx_drate->flags & IEEE80211_TX_RC_GREEN_FIELD);
	if ((info->control.short_preamble) ||
	    (tx_drate->flags & IEEE80211_TX_RC_USE_SHORT_PREAMBLE))
		ctrl_short_preamble = true;
	pr_debug("mcs = %d, data rate idx=%d\n", tx_drate->idx, tx_drate[3].count);
	for (nRCParams = 0; (nRCParams < SSV62XX_TX_MAX_RATES); nRCParams++) {
		if ((rc_params == NULL) || (sc == NULL)) {
			mcsidx = tx_drate->idx;
			drate_kbps = ssv_rate->drate_kbps;
			crate_kbps = ssv_rate->crate_kbps;
		} else {
			if (rc_params[nRCParams].count == 0) {
				break;
			}
			ssv_rc = sc->rc;
			mcsidx =
			    (rc_params[nRCParams].drate -
			     SSV62XX_RATE_MCS_INDEX) % MCS_GROUP_RATES;
			drate_kbps =
			    ssv_rc->rc_table[rc_params[nRCParams].drate].
			    rate_kbps;
			crate_kbps =
			    ssv_rc->rc_table[rc_params[nRCParams].crate].
			    rate_kbps;
		}
		if (tx_drate->flags & IEEE80211_TX_RC_MCS) {
			frame_time = ssv6xxx_ht_txtime(mcsidx,
						       len, is_ht40, is_sgi,
						       is_gf);
			d_phy = 0;
		} else {
			if ((info->band == INDEX_80211_BAND_2GHZ) &&
			    !(ssv_rate->d_flags & IEEE80211_RATE_ERP_G))
				d_phy = WLAN_RC_PHY_CCK;
			else
				d_phy = WLAN_RC_PHY_OFDM;
			frame_time = ssv6xxx_non_ht_txtime(d_phy, drate_kbps,
							   len,
							   ctrl_short_preamble);
		}
		if ((info->band == INDEX_80211_BAND_2GHZ) &&
		    !(ssv_rate->c_flags & IEEE80211_RATE_ERP_G))
			c_phy = WLAN_RC_PHY_CCK;
		else
			c_phy = WLAN_RC_PHY_OFDM;
		if (tx_desc->unicast) {
			if (info->flags & IEEE80211_TX_CTL_AMPDU) {
				ack_time = ssv6xxx_non_ht_txtime(c_phy,
								 crate_kbps,
								 BA_LEN,
								 ctrl_short_preamble);
			} else {
				ack_time = ssv6xxx_non_ht_txtime(c_phy,
								 crate_kbps,
								 ACK_LEN,
								 ctrl_short_preamble);
			}
		}
		if (tx_desc->do_rts_cts & IEEE80211_TX_RC_USE_RTS_CTS) {
			rts_cts_nav = frame_time;
			rts_cts_nav += ack_time;
			rts_cts_nav += ssv6xxx_non_ht_txtime(c_phy,
							     crate_kbps,
							     CTS_LEN,
							     ctrl_short_preamble);
			frame_consume_time = rts_cts_nav;
			frame_consume_time += ssv6xxx_non_ht_txtime(c_phy,
								    crate_kbps,
								    RTS_LEN,
								    ctrl_short_preamble);
		} else if (tx_desc->
			   do_rts_cts & IEEE80211_TX_RC_USE_CTS_PROTECT) {
			rts_cts_nav = frame_time;
			rts_cts_nav += ack_time;
			frame_consume_time = rts_cts_nav;
			frame_consume_time += ssv6xxx_non_ht_txtime(c_phy,
								    crate_kbps,
								    CTS_LEN,
								    ctrl_short_preamble);
		} else {;
		}
		if (tx_drate->flags & IEEE80211_TX_RC_MCS) {
			l_length = frame_time - HT_SIFS_TIME;
			l_length = ((l_length - (HT_SIGNAL_EXT + 20)) + 3) >> 2;
			l_length += ((l_length << 1) - 3);
		}
		if ((rc_params == NULL) || (sc == NULL)) {
			tx_desc->rts_cts_nav = rts_cts_nav;
			tx_desc->frame_consume_time =
			    (frame_consume_time >> 5) + 1;;
			tx_desc->dl_length = l_length;
			break;
		} else {
			rc_params[nRCParams].rts_cts_nav = rts_cts_nav;
			rc_params[nRCParams].frame_consume_time =
			    (frame_consume_time >> 5) + 1;
			rc_params[nRCParams].dl_length = l_length;
			if (nRCParams == 0) {
				tx_desc->drate_idx = rc_params[nRCParams].drate;
				tx_desc->crate_idx = rc_params[nRCParams].crate;
				tx_desc->rts_cts_nav =
				    rc_params[nRCParams].rts_cts_nav;
				tx_desc->frame_consume_time =
				    rc_params[nRCParams].frame_consume_time;
				tx_desc->dl_length =
				    rc_params[nRCParams].dl_length;
			}
		}
	}
	return ack_time;
}

static void ssv6200_hw_set_pair_type(struct ssv_hw *sh, u8 type)
{
	u32 temp;
	SMAC_REG_READ(sh, ADR_SCRT_SET, &temp);
	temp = (temp & PAIR_SCRT_I_MSK);
	temp |= (type << PAIR_SCRT_SFT);
	SMAC_REG_WRITE(sh, ADR_SCRT_SET, temp);
	dev_dbg(sh->sc->dev, "==>%s: write cipher type %d into hw\n", __func__, type);
}

static u32 ssv6200_hw_get_pair_type(struct ssv_hw *sh)
{
	u32 temp;
	SMAC_REG_READ(sh, ADR_SCRT_SET, &temp);
	temp &= PAIR_SCRT_MSK;
	temp = (temp >> PAIR_SCRT_SFT);
	SMAC_REG_WRITE(sh, ADR_SCRT_SET, temp);
	dev_dbg(sh->sc->dev, "==>%s: read cipher type %d from hw\n", __func__, temp);
	return temp;
}

static void ssv6200_hw_set_group_type(struct ssv_hw *sh, u8 type)
{
	u32 temp;
	SMAC_REG_READ(sh, ADR_SCRT_SET, &temp);
	temp = temp & GRP_SCRT_I_MSK;
	temp |= (type << GRP_SCRT_SFT);
	SMAC_REG_WRITE(sh, ADR_SCRT_SET, temp);
	dev_dbg(sh->sc->dev, "Set group key type %d\n", type);
}

void ssv6xxx_reset_sec_module(struct ssv_softc *sc)
{
	ssv6200_hw_set_group_type(sc->sh, ME_NONE);
	ssv6200_hw_set_pair_type(sc->sh, ME_NONE);
}

static int hw_update_watch_wsid(struct ssv_softc *sc, struct ieee80211_sta *sta,
				struct ssv_sta_info *sta_info, int sta_idx,
				int rx_hw_sec, int ops)
{
	int ret = 0;
	int retry_cnt = 20;
	struct sk_buff *skb = NULL;
	struct cfg_host_cmd *host_cmd;
	struct ssv6xxx_wsid_params *ptr;
	dev_dbg(sc->dev, "cmd=%d for fw wsid list, wsid %d \n", ops, sta_idx);
	skb =
	    ssv_skb_alloc(HOST_CMD_HDR_LEN +
			  sizeof(struct ssv6xxx_wsid_params));
	if (skb == NULL || sta_info == NULL || sc == NULL)
		return -1;
	skb->data_len = HOST_CMD_HDR_LEN + sizeof(struct ssv6xxx_wsid_params);
	skb->len = skb->data_len;
	host_cmd = (struct cfg_host_cmd *)skb->data;
	host_cmd->c_type = HOST_CMD;
	host_cmd->h_cmd = (u8) SSV6XXX_HOST_CMD_WSID_OP;
	host_cmd->len = skb->data_len;
	ptr = (struct ssv6xxx_wsid_params *)host_cmd->dat8;
	ptr->cmd = ops;
	ptr->hw_security = rx_hw_sec;
	if ((ptr->cmd != SSV6XXX_WSID_OPS_HWWSID_PAIRWISE_SET_TYPE)
	    && (ptr->cmd != SSV6XXX_WSID_OPS_HWWSID_GROUP_SET_TYPE)) {
		ptr->wsid_idx = (u8) (sta_idx - SSV_NUM_HW_STA);
	} else {
		ptr->wsid_idx = (u8) (sta_idx);
	};
	memcpy(&ptr->target_wsid, &sta->addr[0], 6);
	while (((sc->sh->hci.hci_ops->hci_send_cmd(skb)) != 0) && (retry_cnt)) {
		dev_dbg(sc->dev, "WSID cmd=%d retry=%d!!\n", ops, retry_cnt);
		retry_cnt--;
	}
	dev_dbg(sc->dev, "%s: wsid_idx = %u\n", __FUNCTION__, ptr->wsid_idx);
	ssv_skb_free(skb);
	if (ops == SSV6XXX_WSID_OPS_ADD)
		sta_info->hw_wsid = sta_idx;
	return ret;
}

static void hw_crypto_key_clear(struct ieee80211_hw *hw, int index,
				struct ieee80211_key_conf *key,
				struct ssv_vif_priv_data *vif_priv,
				struct ssv_sta_priv_data *sta_priv)
{
	struct ssv_softc *sc = hw->priv;
	struct ssv_sta_info *sta_info = NULL;
	if ((index == 0) && (sta_priv == NULL))
		return;
	if ((index < 0) || (index >= 4))
		return;
	if (index > 0) {
		if (vif_priv)
			vif_priv->group_key_idx = 0;
		if (sta_priv)
			sta_priv->group_key_idx = 0;
	}
	if (sta_priv) {
		sta_info = &sc->sta_info[sta_priv->sta_idx];
		if ((index == 0) && (sta_priv->has_hw_decrypt == true)
		    && (sta_info->hw_wsid >= SSV_NUM_HW_STA)) {
			hw_update_watch_wsid(sc, sta_info->sta, sta_info,
					     sta_priv->sta_idx,
					     SSV6XXX_WSID_SEC_PAIRWISE,
					     SSV6XXX_WSID_OPS_DISABLE_CAPS);
		}
	}
	if (vif_priv) {
		if ((index != 0) && !list_empty(&vif_priv->sta_list)) {
			struct ssv_sta_priv_data *sta_priv_iter;
			list_for_each_entry(sta_priv_iter, &vif_priv->sta_list,
					    list) {
				if (((sta_priv_iter->sta_info->
				      s_flags & STA_FLAG_VALID) == 0)
				    || (sta_priv_iter->sta_info->hw_wsid <
					SSV_NUM_HW_STA))
					continue;
				hw_update_watch_wsid(sc,
						     sta_priv_iter->sta_info->
						     sta,
						     sta_priv_iter->sta_info,
						     sta_priv_iter->sta_idx,
						     SSV6XXX_WSID_SEC_GROUP,
						     SSV6XXX_WSID_OPS_DISABLE_CAPS);
			}
		}
	}
}

static void _set_wep_sw_crypto_key(struct ssv_softc *sc,
				   struct ssv_vif_info *vif_info,
				   struct ssv_sta_info *sta_info, void *param)
{
	struct ssv_sta_priv_data *sta_priv =
	    (struct ssv_sta_priv_data *)sta_info->sta->drv_priv;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)vif_info->vif->drv_priv;
	sta_priv->has_hw_encrypt = vif_priv->has_hw_encrypt;
	sta_priv->has_hw_decrypt = vif_priv->has_hw_decrypt;
	sta_priv->need_sw_encrypt = vif_priv->need_sw_encrypt;
	sta_priv->need_sw_decrypt = vif_priv->need_sw_decrypt;
}

static void _set_wep_hw_crypto_pair_key(struct ssv_softc *sc,
					struct ssv_vif_info *vif_info,
					struct ssv_sta_info *sta_info,
					void *param)
{
	int wsid = sta_info->hw_wsid;
	struct ssv6xxx_hw_sec *sram_key = (struct ssv6xxx_hw_sec *)param;
	int address = 0;
	int *pointer = NULL;
	u32 sec_key_tbl_base = sc->sh->hw_sec_key[0];
	u32 sec_key_tbl = sec_key_tbl_base;
	int i;
	u8 *key = sram_key->sta_key[0].pair.key;
	u32 key_len = *(u16 *) & sram_key->sta_key[0].reserve[0];
	struct ssv_sta_priv_data *sta_priv =
	    (struct ssv_sta_priv_data *)sta_info->sta->drv_priv;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)vif_info->vif->drv_priv;
	if (wsid == (-1))
		return;
	sram_key->sta_key[wsid].pair_key_idx = 0;
	sram_key->sta_key[wsid].group_key_idx = 0;
	sta_priv->has_hw_encrypt = vif_priv->has_hw_encrypt;
	sta_priv->has_hw_decrypt = vif_priv->has_hw_decrypt;
	sta_priv->need_sw_encrypt = vif_priv->need_sw_encrypt;
	sta_priv->need_sw_decrypt = vif_priv->need_sw_decrypt;
	if (wsid != 0)
		memcpy(sram_key->sta_key[wsid].pair.key, key, key_len);
	address = sec_key_tbl + (3 * sizeof(struct ssv6xxx_hw_key))
	    + wsid * sizeof(struct ssv6xxx_hw_sta_key);
	address += (0x10000 * wsid);
	pointer = (int *)&sram_key->sta_key[wsid];
	for (i = 0; i < (sizeof(struct ssv6xxx_hw_sta_key) / 4); i++)
		SMAC_REG_WRITE(sc->sh, address + (i * 4), *(pointer++));
}

static void _set_wep_hw_crypto_group_key(struct ssv_softc *sc,
					 struct ssv_vif_info *vif_info,
					 struct ssv_sta_info *sta_info,
					 void *param)
{
	int wsid = sta_info->hw_wsid;
	struct ssv6xxx_hw_sec *sram_key = (struct ssv6xxx_hw_sec *)param;
	int address = 0;
	int *pointer = NULL;
	u32 key_idx = sram_key->sta_key[0].pair_key_idx;
	u32 sec_key_tbl_base = sc->sh->hw_sec_key[0];
	u32 key_len = *(u16 *) & sram_key->sta_key[0].reserve[0];
	u8 *key = sram_key->group_key[key_idx - 1].key;
	u32 sec_key_tbl = sec_key_tbl_base;
	struct ssv_sta_priv_data *sta_priv =
	    (struct ssv_sta_priv_data *)sta_info->sta->drv_priv;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)vif_info->vif->drv_priv;
	if (wsid == (-1))
		return;
	if (wsid != 0) {
		sram_key->sta_key[wsid].pair_key_idx = key_idx;
		sram_key->sta_key[wsid].group_key_idx = key_idx;
		sta_priv->has_hw_encrypt = vif_priv->has_hw_encrypt;
		sta_priv->has_hw_decrypt = vif_priv->has_hw_decrypt;
		sta_priv->need_sw_encrypt = vif_priv->need_sw_encrypt;
		sta_priv->need_sw_decrypt = vif_priv->need_sw_decrypt;
	}
	if (wsid != 0)
		memcpy(sram_key->group_key[key_idx - 1].key, key, key_len);
	sec_key_tbl += (0x10000 * wsid);
	address = sec_key_tbl + ((key_idx - 1) * sizeof(struct ssv6xxx_hw_key));
	pointer = (int *)&sram_key->group_key[key_idx - 1];
	{
		int i;
		for (i = 0; i < (sizeof(struct ssv6xxx_hw_key) / 4); i++)
			SMAC_REG_WRITE(sc->sh, address + (i * 4), *(pointer++));
	}
	address = sec_key_tbl + (3 * sizeof(struct ssv6xxx_hw_key))
	    + (wsid * sizeof(struct ssv6xxx_hw_sta_key));
	pointer = (int *)&sram_key->sta_key[wsid];
	SMAC_REG_WRITE(sc->sh, address, *(pointer));
}

static int hw_crypto_key_write_wep(struct ieee80211_hw *hw,
				   struct ieee80211_key_conf *key,
				   u8 algorithm, struct ssv_vif_info *vif_info)
{
	struct ssv_softc *sc = hw->priv;
	struct ssv6xxx_hw_sec *sramKey = &vif_info->sramKey;
	if (key->keyidx == 0) {
		ssv6xxx_foreach_vif_sta(sc, vif_info,
					_set_wep_hw_crypto_pair_key, sramKey);
	} else {
		ssv6xxx_foreach_vif_sta(sc, vif_info,
					_set_wep_hw_crypto_group_key, sramKey);
	}
	return 0;
}

static void _set_aes_tkip_hw_crypto_group_key(struct ssv_softc *sc,
					      struct ssv_vif_info *vif_info,
					      struct ssv_sta_info *sta_info,
					      void *param)
{
	int wsid = sta_info->hw_wsid;
	int j;
	u32 sec_key_tbl_base = sc->sh->hw_sec_key[0];
	u32 sec_key_tbl = sec_key_tbl_base;
	int address = 0;
	int *pointer = 0;
	struct ssv6xxx_hw_sec *sramKey = &(vif_info->sramKey);
	int index = *(u8 *) param;
	if (wsid == (-1))
		return;
	BUG_ON(index == 0);
	sramKey->sta_key[wsid].group_key_idx = index;
	sec_key_tbl += (0x10000 * wsid);
	address = sec_key_tbl + ((index - 1) * sizeof(struct ssv6xxx_hw_key));
	if (vif_info->vif_priv != NULL)
		dev_dbg(sc->dev, "Write group key %d to VIF %d to %08X\n",
			index, vif_info->vif_priv->vif_idx, address);
	else
		dev_err(sc->dev, "NULL VIF.\n");
	pointer = (int *)&sramKey->group_key[index - 1];
	for (j = 0; j < (sizeof(struct ssv6xxx_hw_key) / 4); j++)
		SMAC_REG_WRITE(sc->sh, address + (j * 4), *(pointer++));
	address = sec_key_tbl + (3 * sizeof(struct ssv6xxx_hw_key))
	    + (wsid * sizeof(struct ssv6xxx_hw_sta_key));
	pointer = (int *)&sramKey->sta_key[wsid];
	SMAC_REG_WRITE(sc->sh, address, *(pointer));
	if (wsid >= SSV_NUM_HW_STA) {
		hw_update_watch_wsid(sc, sta_info->sta, sta_info,
				     wsid, SSV6XXX_WSID_SEC_GROUP,
				     SSV6XXX_WSID_OPS_ENABLE_CAPS);
	}
}

static int _write_pairwise_key_to_hw(struct ssv_softc *sc,
				     int index, u8 algorithm,
				     const u8 * key, int key_len,
				     struct ieee80211_key_conf *keyconf,
				     struct ssv_vif_priv_data *vif_priv,
				     struct ssv_sta_priv_data *sta_priv)
{
	int i;
	struct ssv6xxx_hw_sec *sramKey;
	int address = 0;
	int *pointer = NULL;
	u32 sec_key_tbl_base = sc->sh->hw_sec_key[0];
	u32 sec_key_tbl;
	int wsid = (-1);
	if (sta_priv == NULL) {
		dev_err(sc->dev, "Set pair-wise key with NULL STA.\n");
		return -EOPNOTSUPP;
	}
	wsid = sta_priv->sta_info->hw_wsid;
	if ((wsid < 0) || (wsid >= SSV_NUM_STA)) {
		dev_err(sc->dev, "Set pair-wise key to invalid WSID %d.\n",
			wsid);
		return -EOPNOTSUPP;
	}
	dev_dbg(sc->dev, "Set STA %d's pair-wise key of %d bytes.\n", wsid,
		key_len);
	sramKey = &(sc->vif_info[vif_priv->vif_idx].sramKey);
	sramKey->sta_key[wsid].pair_key_idx = 0;
	sramKey->sta_key[wsid].group_key_idx = vif_priv->group_key_idx;
	memcpy(sramKey->sta_key[wsid].pair.key, key, key_len);
	sec_key_tbl = sec_key_tbl_base;
	sec_key_tbl += (0x10000 * wsid);
	address = sec_key_tbl + (3 * sizeof(struct ssv6xxx_hw_key))
	    + wsid * sizeof(struct ssv6xxx_hw_sta_key);
	pointer = (int *)&sramKey->sta_key[wsid];
	for (i = 0; i < (sizeof(struct ssv6xxx_hw_sta_key) / 4); i++)
		SMAC_REG_WRITE(sc->sh, (address + (i * 4)), *(pointer++));
	if (wsid >= SSV_NUM_HW_STA) {
		hw_update_watch_wsid(sc, sta_priv->sta_info->sta,
				     sta_priv->sta_info, sta_priv->sta_idx,
				     SSV6XXX_WSID_SEC_PAIRWISE,
				     SSV6XXX_WSID_OPS_ENABLE_CAPS);
	}
	return 0;
}

static int _write_group_key_to_hw(struct ssv_softc *sc,
				  int index, u8 algorithm,
				  const u8 * key, int key_len,
				  struct ieee80211_key_conf *keyconf,
				  struct ssv_vif_priv_data *vif_priv,
				  struct ssv_sta_priv_data *sta_priv)
{
	struct ssv6xxx_hw_sec *sramKey;
	int wsid = sta_priv ? sta_priv->sta_info->hw_wsid : (-1);
	int ret = 0;
	if (vif_priv == NULL) {
		dev_err(sc->dev, "Setting group key to NULL VIF\n");
		return -EOPNOTSUPP;
	}
	dev_dbg(sc->dev,
		"Setting VIF %d group key %d of length %d to WSID %d.\n",
		vif_priv->vif_idx, index, key_len, wsid);
	sramKey = &(sc->vif_info[vif_priv->vif_idx].sramKey);
	vif_priv->group_key_idx = index;
	if (sta_priv)
		sta_priv->group_key_idx = index;
	memcpy(sramKey->group_key[index - 1].key, key, key_len);
	WARN_ON(sc->vif_info[vif_priv->vif_idx].vif_priv == NULL);
	ssv6xxx_foreach_vif_sta(sc, &sc->vif_info[vif_priv->vif_idx],
				_set_aes_tkip_hw_crypto_group_key, &index);
	ret = 0;
	return ret;
}

static enum SSV_CIPHER_E _prepare_key(struct ieee80211_key_conf *key)
{
	enum SSV_CIPHER_E cipher;
	switch (key->cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
		cipher = SSV_CIPHER_WEP40;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		cipher = SSV_CIPHER_WEP104;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		key->flags |= IEEE80211_KEY_FLAG_GENERATE_MMIC;
		cipher = SSV_CIPHER_TKIP;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		key->flags |=
		    (IEEE80211_KEY_FLAG_SW_MGMT_TX |
		     IEEE80211_KEY_FLAG_RX_MGMT);
		cipher = SSV_CIPHER_CCMP;
		break;
	default:
		cipher = SSV_CIPHER_INVALID;
		break;
	}
	return cipher;
}
int _set_key_wep(struct ssv_softc *sc, struct ssv_vif_priv_data *vif_priv,
		 struct ssv_sta_priv_data *sta_priv, enum SSV_CIPHER_E cipher,
		 struct ieee80211_key_conf *key)
{
	int ret = 0;
	struct ssv_vif_info *vif_info = &sc->vif_info[vif_priv->vif_idx];
	struct ssv6xxx_hw_sec *sram_key = &vif_info->sramKey;
	sram_key->sta_key[0].pair_key_idx = key->keyidx;
	sram_key->sta_key[0].group_key_idx = key->keyidx;
	*(u16 *) & sram_key->sta_key[0].reserve[0] = key->keylen;
	dev_dbg(sc->dev, "Set WEP %02X %02X %02X %02X %02X %02X %02X %02X... (%d %d)\n",
	       key->key[0], key->key[1], key->key[2], key->key[3], key->key[4],
	       key->key[5], key->key[6], key->key[7], key->keyidx, key->keylen);
	if (key->keyidx == 0) {
		memcpy(sram_key->sta_key[0].pair.key, key->key, key->keylen);
	} else {
		memcpy(sram_key->group_key[key->keyidx - 1].key, key->key,
		       key->keylen);
	}
	if (sc->sh->cfg.use_wpa2_only) {
		dev_warn(sc->dev, "WEP: use WPA2 HW security mode only.\n");
	}
	if ((sc->sh->cfg.use_wpa2_only == 0)
	    && vif_priv->vif_idx == 0) {
		vif_priv->has_hw_decrypt = true;
		vif_priv->has_hw_encrypt = true;
		vif_priv->need_sw_decrypt = false;
		vif_priv->need_sw_encrypt = false;
		vif_priv->use_mac80211_decrypt = false;
		ssv6200_hw_set_pair_type(sc->sh, cipher);
		ssv6200_hw_set_group_type(sc->sh, cipher);
		hw_crypto_key_write_wep(sc->hw, key, cipher,
					&sc->vif_info[vif_priv->vif_idx]);
	} else {
		vif_priv->has_hw_decrypt = false;
		vif_priv->has_hw_encrypt = false;
		vif_priv->need_sw_decrypt = false;
		vif_priv->need_sw_encrypt = false;
		vif_priv->use_mac80211_decrypt = true;
		ssv6xxx_foreach_vif_sta(sc, vif_info, _set_wep_sw_crypto_key,
					NULL);
		ret = -EOPNOTSUPP;
	}
	vif_priv->pair_cipher = vif_priv->group_cipher = cipher;
	vif_priv->is_security_valid = true;
	return ret;
}

static int _set_pairwise_key_tkip_ccmp(struct ssv_softc *sc,
				       struct ssv_vif_priv_data *vif_priv,
				       struct ssv_sta_priv_data *sta_priv,
				       enum SSV_CIPHER_E cipher,
				       struct ieee80211_key_conf *key)
{
	int ret = 0;
	const char *cipher_name = (cipher == SSV_CIPHER_CCMP) ? "CCMP" : "TKIP";
	struct ssv_vif_info *vif_info = &sc->vif_info[vif_priv->vif_idx];
	bool tdls_link = false, tdls_use_sw_cipher = false, tkip_use_sw_cipher =
	    false;
	bool use_non_ccmp = false;
	int another_vif_idx = ((vif_priv->vif_idx + 1) % 2);
	struct ssv_vif_priv_data *another_vif_priv =
	    (struct ssv_vif_priv_data *)sc->vif_info[another_vif_idx].vif_priv;
	if (sta_priv == NULL) {
		dev_err(sc->dev,
			"Setting pairwise TKIP/CCMP key to NULL STA.\n");
		return -EOPNOTSUPP;
	}
	if (sc->sh->cfg.use_wpa2_only) {
		dev_warn(sc->dev, "Pairwise TKIP/CCMP: use WPA2 HW security mode only.\n");
	}
	if (vif_info->if_type == NL80211_IFTYPE_STATION) {
		struct ssv_sta_priv_data *first_sta_priv =
		    list_first_entry(&vif_priv->sta_list,
				     struct ssv_sta_priv_data, list);
		if (first_sta_priv->sta_idx != sta_priv->sta_idx) {
			tdls_link = true;
		}
		dev_dbg(sc->dev, "first sta idx %d, current sta idx %d\n",
		       first_sta_priv->sta_idx, sta_priv->sta_idx);
	}
	if ((tdls_link) && (vif_priv->pair_cipher != SSV_CIPHER_CCMP)
	    && (sc->sh->cfg.use_wpa2_only == false)) {
		tdls_use_sw_cipher = true;
	}
	if (another_vif_priv != NULL) {
		if ((another_vif_priv->pair_cipher != SSV_CIPHER_CCMP)
		    && (another_vif_priv->pair_cipher != SSV_CIPHER_NONE)) {
			use_non_ccmp = true;
			dev_dbg(sc->dev, "another vif use none ccmp\n");
		}
	}
	if ((((tdls_link) && (vif_priv->pair_cipher != SSV_CIPHER_CCMP))
	     || (use_non_ccmp))
	    && (sc->sh->cfg.use_wpa2_only == 1) && (cipher == SSV_CIPHER_CCMP)) {
		u32 val;
		SMAC_REG_READ(sc->sh, ADR_RX_FLOW_DATA, &val);
		if (((val >> 4) & 0xF) != M_ENG_CPU) {
			SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_DATA,
				       ((val & 0xf) | (M_ENG_CPU << 4)
					| (val & 0xfffffff0) << 4));
			dev_dbg(sc->dev,
				"orginal Rx_Flow %x , modified flow %x \n", val,
				((val & 0xf) | (M_ENG_CPU << 4) |
				 (val & 0xfffffff0) << 4));
		}
	}
	if ((cipher == SSV_CIPHER_TKIP) && (sc->sh->cfg.use_wpa2_only == 1)) {
		tkip_use_sw_cipher = true;
	}
	if (tkip_use_sw_cipher == true)
		dev_info(sc->dev, "Using software TKIP cipher\n");
	if ((((vif_priv->vif_idx == 0) && (tdls_use_sw_cipher == false)
	      && (tkip_use_sw_cipher == false)))
	    || ((cipher == SSV_CIPHER_CCMP)
		&& (sc->sh->cfg.use_wpa2_only == 1))) {
		sta_priv->has_hw_decrypt = true;
		sta_priv->need_sw_decrypt = false;
		if ((cipher == SSV_CIPHER_TKIP)
		    || ((!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_TX) ||
			 (sta_priv->sta_info->sta->deflink.ht_cap.ht_supported ==
			  false))
			&& (vif_priv->force_sw_encrypt == false))) {
			dev_dbg(sc->dev,
				"STA %d uses HW encrypter for pairwise.\n",
				sta_priv->sta_idx);
			sta_priv->has_hw_encrypt = true;
			sta_priv->need_sw_encrypt = false;
			sta_priv->use_mac80211_decrypt = false;
			ret = 0;
		} else {
			sta_priv->has_hw_encrypt = false;
			sta_priv->need_sw_encrypt = false;
			sta_priv->use_mac80211_decrypt = true;
			ret = -EOPNOTSUPP;
		}
	} else {
		sta_priv->has_hw_encrypt = false;
		sta_priv->has_hw_decrypt = false;
		dev_err(sc->dev, "STA %d MAC80211's %s cipher.\n",
			sta_priv->sta_idx, cipher_name);
		sta_priv->need_sw_encrypt = false;
		sta_priv->need_sw_decrypt = false;
		sta_priv->use_mac80211_decrypt = true;
		ret = -EOPNOTSUPP;
	}
	if (sta_priv->has_hw_encrypt || sta_priv->has_hw_decrypt) {
		ssv6200_hw_set_pair_type(sc->sh, cipher);
		_write_pairwise_key_to_hw(sc, key->keyidx, cipher,
					  key->key, key->keylen, key,
					  vif_priv, sta_priv);
	}
	if ((vif_priv->has_hw_encrypt || vif_priv->has_hw_decrypt)
	    && (vif_priv->group_key_idx > 0)) {
		_set_aes_tkip_hw_crypto_group_key(sc,
						  &sc->vif_info[vif_priv->
								vif_idx],
						  sta_priv->sta_info,
						  &vif_priv->group_key_idx);
	}
	return ret;
}

static int _set_group_key_tkip_ccmp(struct ssv_softc *sc,
				    struct ssv_vif_priv_data *vif_priv,
				    struct ssv_sta_priv_data *sta_priv,
				    enum SSV_CIPHER_E cipher,
				    struct ieee80211_key_conf *key)
{
	int ret = 0;
	const char *cipher_name = (cipher == SSV_CIPHER_CCMP) ? "CCMP" : "TKIP";
	bool tkip_use_sw_cipher = false;
	vif_priv->group_cipher = cipher;
	if (sc->sh->cfg.use_wpa2_only) {
		dev_warn(sc->dev, "Group TKIP/CCMP: use WPA2 HW security mode only.\n");
	}
	if ((cipher == SSV_CIPHER_TKIP) && (sc->sh->cfg.use_wpa2_only == 1)) {
		tkip_use_sw_cipher = true;
	}
	if (((vif_priv->vif_idx == 0) && (tkip_use_sw_cipher == false))
	    || ((cipher == SSV_CIPHER_CCMP)
		&& (sc->sh->cfg.use_wpa2_only == 1))) {
		dev_dbg(sc->dev, "VIF %d uses HW %s cipher for group.\n",
			vif_priv->vif_idx, cipher_name);
#ifdef USE_MAC80211_DECRYPT_BROADCAST
		vif_priv->has_hw_decrypt = false;
		ret = -EOPNOTSUPP;
#else
		vif_priv->has_hw_decrypt = true;
#endif
		vif_priv->has_hw_encrypt = true;
		vif_priv->need_sw_decrypt = false;
		vif_priv->need_sw_encrypt = false;
		vif_priv->use_mac80211_decrypt = false;
	} else {
		vif_priv->has_hw_decrypt = false;
		vif_priv->has_hw_encrypt = false;
		dev_err(sc->dev, "VIF %d uses MAC80211's %s cipher.\n",
			vif_priv->vif_idx, cipher_name);
		vif_priv->need_sw_encrypt = false;
		vif_priv->need_sw_encrypt = false;
		vif_priv->use_mac80211_decrypt = true;
		ret = -EOPNOTSUPP;
	}
	if (vif_priv->has_hw_encrypt || vif_priv->has_hw_decrypt) {
#ifdef USE_MAC80211_DECRYPT_BROADCAST
		ssv6200_hw_set_group_type(sc->sh, ME_NONE);
#else
		ssv6200_hw_set_group_type(sc->sh, cipher);
#endif
		key->hw_key_idx = key->keyidx;
		_write_group_key_to_hw(sc, key->keyidx, cipher,
				       key->key, key->keylen, key,
				       vif_priv, sta_priv);
	}
	vif_priv->is_security_valid = true;
	{
		int another_vif_idx = ((vif_priv->vif_idx + 1) % 2);
		struct ssv_vif_priv_data *another_vif_priv =
		    (struct ssv_vif_priv_data *)sc->vif_info[another_vif_idx].
		    vif_priv;
		if (another_vif_priv != NULL) {
			if (((SSV6XXX_USE_SW_DECRYPT(vif_priv)
			      && SSV6XXX_USE_HW_DECRYPT(another_vif_priv)))
			    || ((SSV6XXX_USE_HW_DECRYPT(vif_priv)
				 &&
				 (SSV6XXX_USE_SW_DECRYPT(another_vif_priv))))) {
				u32 val;
				SMAC_REG_READ(sc->sh, ADR_RX_FLOW_DATA, &val);
				if (((val >> 4) & 0xF) != M_ENG_CPU) {
					SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_DATA,
						       ((val & 0xf) |
							(M_ENG_CPU << 4)
							| (val & 0xfffffff0) <<
							4));
					dev_dbg(sc->dev,
						"orginal Rx_Flow %x , modified flow %x \n",
						val,
						((val & 0xf) | (M_ENG_CPU << 4)
						 | (val & 0xfffffff0) << 4));
				} else {
					dev_dbg(sc->dev, " doesn't need to change rx flow\n");
				}
			}
		}
	}
	return ret;
}

static int _set_key_tkip_ccmp(struct ssv_softc *sc,
			      struct ssv_vif_priv_data *vif_priv,
			      struct ssv_sta_priv_data *sta_priv,
			      enum SSV_CIPHER_E cipher,
			      struct ieee80211_key_conf *key)
{
	if (key->keyidx == 0)
		return _set_pairwise_key_tkip_ccmp(sc, vif_priv, sta_priv,
						   cipher, key);
	else
		return _set_group_key_tkip_ccmp(sc, vif_priv, sta_priv, cipher,
						key);
}

static int ssv6200_set_key(struct ieee80211_hw *hw,
			   enum set_key_cmd cmd,
			   struct ieee80211_vif *vif,
			   struct ieee80211_sta *sta,
			   struct ieee80211_key_conf *key)
{
	struct ssv_softc *sc = hw->priv;
	int ret = 0;
	enum SSV_CIPHER_E cipher = SSV_CIPHER_NONE;
	int sta_idx = (-1);
	struct ssv_sta_info *sta_info = NULL;
	struct ssv_sta_priv_data *sta_priv = NULL;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)vif->drv_priv;
	struct ssv_vif_info *vif_info = &sc->vif_info[vif_priv->vif_idx];
	if (sta) {
		sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
		sta_idx = sta_priv->sta_idx;
		sta_info = sta_priv->sta_info;
	}
	BUG_ON((cmd != SET_KEY) && (cmd != DISABLE_KEY));
	if (!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_SECURITY)) {
		dev_warn(sc->dev, "HW does not support security.\n");
		return -EOPNOTSUPP;
	}
	if (sta_info && (sta_info->hw_wsid == (-1))) {
		dev_warn(sc->dev,
			 "Add STA without HW resource. Use MAC80211's solution.\n");
		return -EOPNOTSUPP;
	}
	cipher = _prepare_key(key);
	dev_dbg(sc->dev,
		"Set key VIF %d VIF type %d STA %d algorithm = %d, key->keyidx = %d, cmd = %d\n",
		vif_priv->vif_idx, vif->type, sta_idx, cipher, key->keyidx,
		cmd);
	if (cipher == SSV_CIPHER_INVALID) {
		dev_warn(sc->dev, "Unsupported cipher type.\n");
		return -EOPNOTSUPP;
	}
	mutex_lock(&sc->mutex);
	switch (cmd) {
	case SET_KEY:
		{
			switch (cipher) {
			case SSV_CIPHER_WEP40:
			case SSV_CIPHER_WEP104:
				ret =
				    _set_key_wep(sc, vif_priv, sta_priv, cipher,
						 key);
				break;
			case SSV_CIPHER_TKIP:
			case SSV_CIPHER_CCMP:
				ret =
				    _set_key_tkip_ccmp(sc, vif_priv, sta_priv,
						       cipher, key);
				break;
			default:
				break;
			}
			if (sta) {
				struct ssv_sta_priv_data *first_sta_priv =
				    list_first_entry(&vif_priv->sta_list,
						     struct ssv_sta_priv_data,
						     list);
				if (first_sta_priv->sta_idx ==
				    sta_priv->sta_idx) {
					vif_priv->pair_cipher = cipher;
				}
				if (SSV6200_USE_HW_WSID(sta_idx)) {
					if (SSV6XXX_USE_SW_DECRYPT(sta_priv)) {
						u32 cipher_setting;
						cipher_setting =
						    ssv6200_hw_get_pair_type
						    (sc->sh);
						if (cipher_setting != ME_NONE) {
							u32 val;
							SMAC_REG_READ(sc->sh,
								      ADR_RX_FLOW_DATA,
								      &val);
							if (((val >> 4) & 0xF)
							    != M_ENG_CPU) {
								SMAC_REG_WRITE
								    (sc->sh,
								     ADR_RX_FLOW_DATA,
								     ((val &
								       0xf) |
								      (M_ENG_CPU
								       << 4)
								      | (val &
									 0xfffffff0)
								      << 4));
								dev_dbg(sc->dev,
									"orginal Rx_Flow %x , modified flow %x \n",
									val,
									((val &
									  0xf) |
									 (M_ENG_CPU
									  << 4)
									 | (val
									    &
									    0xfffffff0)
									 << 4));
							} else {
								dev_dbg(sc->dev, " doesn't need to change rx flow\n");
							}
						}
					}
					if (sta_priv->has_hw_decrypt) {
						hw_update_watch_wsid(sc, sta,
								     sta_info,
								     sta_idx,
								     SSV6XXX_WSID_SEC_HW,
								     SSV6XXX_WSID_OPS_HWWSID_PAIRWISE_SET_TYPE);
						dev_info(sc->dev, "set hw wsid %d cipher mode to HW cipher for pairwise key\n", sta_idx);
					}
				}
			} else {
				if (vif_info->if_type == NL80211_IFTYPE_STATION) {
					struct ssv_sta_priv_data *first_sta_priv
					    =
					    list_first_entry(&vif_priv->
							     sta_list,
							     struct
							     ssv_sta_priv_data,
							     list);
					if (SSV6200_USE_HW_WSID
					    (first_sta_priv->sta_idx)) {
						if (vif_priv->has_hw_decrypt) {
							hw_update_watch_wsid(sc,
									     sta,
									     sta_info,
									     first_sta_priv->
									     sta_idx,
									     SSV6XXX_WSID_SEC_HW,
									     SSV6XXX_WSID_OPS_HWWSID_GROUP_SET_TYPE);
							dev_info(sc->dev, "set hw wsid %d cipher mode to HW cipher for group key\n", first_sta_priv->sta_idx);
						}
					}
				}
			}
		}
		break;
	case DISABLE_KEY:
		{
			int another_vif_idx = ((vif_priv->vif_idx + 1) % 2);
			struct ssv_vif_priv_data *another_vif_priv =
			    (struct ssv_vif_priv_data *)sc->
			    vif_info[another_vif_idx].vif_priv;
			if (another_vif_priv != NULL) {
				struct ssv_vif_info *vif_info =
				    &sc->vif_info[vif_priv->vif_idx];
				if (vif_info->if_type != NL80211_IFTYPE_AP) {
					if ((SSV6XXX_USE_SW_DECRYPT(vif_priv)
					     &&
					     SSV6XXX_USE_HW_DECRYPT
					     (another_vif_priv))
					    ||
					    (SSV6XXX_USE_SW_DECRYPT
					     (another_vif_priv)
					     &&
					     SSV6XXX_USE_HW_DECRYPT(vif_priv)))
					{
						SMAC_REG_WRITE(sc->sh,
							       ADR_RX_FLOW_DATA,
							       M_ENG_MACRX |
							       (M_ENG_ENCRYPT_SEC
								<< 4) |
							       (M_ENG_HWHCI <<
								8));
						dev_dbg(sc->dev, "redirect Rx flow for disconnect\n");
					}
				} else {
					if (sta == NULL) {
						if (SSV6XXX_USE_SW_DECRYPT
						    (another_vif_priv)
						    &&
						    SSV6XXX_USE_HW_DECRYPT
						    (vif_priv)) {
							SMAC_REG_WRITE(sc->sh,
								       ADR_RX_FLOW_DATA,
								       M_ENG_MACRX
								       |
								       (M_ENG_ENCRYPT_SEC
									<< 4) |
								       (M_ENG_HWHCI
									<< 8));
							dev_dbg(sc->dev, "redirect Rx flow for disconnect\n");
						}
					}
				}
			}
			if (sta == NULL) {
				vif_priv->group_cipher = ME_NONE;
				if ((another_vif_priv == NULL)
				    || ((another_vif_priv != NULL)
					&&
					(!SSV6XXX_USE_HW_DECRYPT
					 (another_vif_priv)))) {
					ssv6200_hw_set_group_type(sc->sh,
								  ME_NONE);
				}
			} else {
				struct ssv_vif_info *vif_info =
				    &sc->vif_info[vif_priv->vif_idx];
				if ((vif_info->if_type != NL80211_IFTYPE_AP)
				    && (another_vif_priv == NULL)) {
					struct ssv_sta_priv_data *first_sta_priv
					    =
					    list_first_entry(&vif_priv->
							     sta_list,
							     struct
							     ssv_sta_priv_data,
							     list);
					if (sta_priv == first_sta_priv) {
						ssv6200_hw_set_pair_type(sc->sh,
									 ME_NONE);
					}
				}
				vif_priv->pair_cipher = ME_NONE;
			}
			if ((cipher == ME_TKIP) || (cipher == ME_CCMP)) {
				dev_dbg(sc->dev, "Clear key %d VIF %d, STA %d\n",
				       key->keyidx, (vif != NULL),
				       (sta != NULL));
				hw_crypto_key_clear(hw, key->keyidx, key,
						    vif_priv, sta_priv);
			}
			{
				if ((key->keyidx == 0) && (sta_priv != NULL)) {
					sta_priv->has_hw_decrypt = false;
					sta_priv->has_hw_encrypt = false;
					sta_priv->need_sw_encrypt = false;
					sta_priv->use_mac80211_decrypt = false;
				}
				if ((vif_priv->is_security_valid)
				    && (key->keyidx != 0)) {
					vif_priv->is_security_valid = false;
				}
			}
			ret = 0;
		}
		break;
	default:
		ret = -EINVAL;
	}
	mutex_unlock(&sc->mutex);
	if (sta_priv != NULL) {
		dev_info(sc->dev, "station mode: hardware encrypt:%d/decrypt:%d, software encrypt:%d/decrypt:%d\n",
		       (sta_priv->has_hw_encrypt == true),
		       (sta_priv->has_hw_decrypt == true),
		       (sta_priv->need_sw_encrypt == true),
		       (sta_priv->need_sw_decrypt == true));
	}
	if (vif_priv) {
		dev_info
		    (sc->dev, "vif mode: hardware encrypt:%d/decrypt:%d, software encrypt:%d/decrypt:%d, mac80211 decrypt: %d, valid:%d\n",
		     (vif_priv->has_hw_encrypt == true),
		     (vif_priv->has_hw_decrypt == true),
		     (vif_priv->need_sw_encrypt == true),
		     (vif_priv->need_sw_decrypt == true),
		     (vif_priv->use_mac80211_decrypt == true),
		     (vif_priv->is_security_valid == true));
	}
	if (vif_priv->force_sw_encrypt
	    || (sta_info && (sta_info->hw_wsid != 1)
		&& (sta_info->hw_wsid != 0))) {
		if (vif_priv->force_sw_encrypt == false)
			vif_priv->force_sw_encrypt = true;
		ret = -EOPNOTSUPP;
	}
	dev_dbg(sc->dev, "SET KEY %d\n", ret);
	return ret;
}

u32 _process_tx_done(struct ssv_softc *sc)
{
	struct ieee80211_tx_info *tx_info;
	struct sk_buff *skb;
	while ((skb = skb_dequeue(&sc->tx_done_q))) {
		struct ssv6200_tx_desc *tx_desc;
		tx_info = IEEE80211_SKB_CB(skb);
		tx_desc = (struct ssv6200_tx_desc *)skb->data;
		if (tx_desc->c_type > M2_TXREQ) {
			ssv_skb_free(skb);
			dev_dbg(sc->dev, "free cmd skb!\n");
			continue;
		}
		if (tx_info->flags & IEEE80211_TX_CTL_AMPDU) {
			ssv6200_ampdu_release_skb(skb, sc->hw);
			continue;
		}
		skb_pull(skb, SSV6XXX_TX_DESC_LEN);
		ieee80211_tx_info_clear_status(tx_info);
		tx_info->flags |= IEEE80211_TX_STAT_ACK;
		tx_info->status.ack_signal = 100;
#ifdef REPORT_TX_DONE_IN_IRQ
		ieee80211_tx_status_irqsafe(sc->hw, skb);
#else
		ieee80211_tx_status(sc->hw, skb);
		if (skb_queue_len(&sc->rx_skb_q))
			break;
#endif
	}
	return skb_queue_len(&sc->tx_done_q);
}

#ifdef REPORT_TX_DONE_IN_IRQ
void ssv6xxx_tx_cb(struct sk_buff_head *skb_head, void *args)
{
	struct ssv_softc *sc = (struct ssv_softc *)args;
	_process_tx_done *(sc);
}
#else
void ssv6xxx_tx_cb(struct sk_buff_head *skb_head, void *args)
{
	struct ssv_softc *sc = (struct ssv_softc *)args;
	struct sk_buff *skb;
	while ((skb = skb_dequeue(skb_head))) {
		struct ieee80211_tx_info *tx_info = IEEE80211_SKB_CB(skb);
		struct ssv6200_tx_desc *tx_desc;
		tx_desc = (struct ssv6200_tx_desc *)skb->data;
		if (tx_desc->c_type > M2_TXREQ) {
			ssv_skb_free(skb);
			dev_dbg(sc->dev, "free cmd skb!\n");
			continue;
		}
		if (tx_info->flags & IEEE80211_TX_CTL_AMPDU)
			ssv6xxx_ampdu_sent(sc->hw, skb);
		skb_queue_tail(&sc->tx_done_q, skb);
	}
	wake_up_interruptible(&sc->rx_wait_q);
}
#endif
void ssv6xxx_tx_rate_update(struct sk_buff *skb, void *args)
{
	struct ieee80211_hdr *hdr;
	struct ssv_softc *sc = args;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ssv6200_tx_desc *tx_desc;
	struct ssv_rate_info ssv_rate;
	u32 nav = 0;
	int ret = 0;
	tx_desc = (struct ssv6200_tx_desc *)skb->data;
	if (tx_desc->c_type > M2_TXREQ)
		return;
	if (!(info->flags & IEEE80211_TX_CTL_AMPDU)) {
		hdr = (struct ieee80211_hdr *)(skb->data + SSV6XXX_TX_DESC_LEN);
		if ((ieee80211_is_data_qos(hdr->frame_control)
		     || ieee80211_is_data(hdr->frame_control))
		    && (tx_desc->wsid < SSV_RC_MAX_HARDWARE_SUPPORT)) {
			ret =
			    ssv6xxx_rc_hw_rate_update_check(skb, sc,
							    tx_desc->
							    do_rts_cts);
			if (ret & RC_FIRMWARE_REPORT_FLAG) {
				{
					tx_desc->RSVD_0 = SSV6XXX_RC_REPORT;
					tx_desc->tx_report = 1;
				}
				ret &= 0xf;
			}
			if (ret) {
				ssv6xxx_rc_hw_rate_idx(sc, info, &ssv_rate);
				tx_desc->crate_idx = ssv_rate.crate_hw_idx;
				tx_desc->drate_idx = ssv_rate.drate_hw_idx;
				nav =
				    ssv6xxx_set_frame_duration(info, &ssv_rate,
							       skb->len +
							       FCS_LEN, tx_desc,
							       NULL, NULL);
				if (tx_desc->tx_burst == 0) {
					if (tx_desc->ack_policy != 0x01)
						hdr->duration_id = nav;
				}
			}
		}
	} else {
	}
	return;
}

void ssv6xxx_update_txinfo(struct ssv_softc *sc, struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_sta *sta;
	struct ssv_sta_info *sta_info = NULL;
	struct ssv_sta_priv_data *ssv_sta_priv = NULL;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)info->control.vif->drv_priv;
	struct ssv6200_tx_desc *tx_desc = (struct ssv6200_tx_desc *)skb->data;
	struct ieee80211_tx_rate *tx_drate;
	struct ssv_rate_info ssv_rate;
	int ac, hw_txqid;
	u32 nav = 0;
	if (info->flags & IEEE80211_TX_CTL_AMPDU) {
		struct ampdu_hdr_st *ampdu_hdr =
		    (struct ampdu_hdr_st *)skb->head;
		sta = ampdu_hdr->ampdu_tid->sta;
		hdr =
		    (struct ieee80211_hdr *)(skb->data + TXPB_OFFSET +
					     AMPDU_DELIMITER_LEN);
	} else {
		struct SKB_info_st *skb_info = (struct SKB_info_st *)skb->head;
		sta = skb_info->sta;
		hdr = (struct ieee80211_hdr *)(skb->data + TXPB_OFFSET);
	}
	if (sta) {
		ssv_sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
		sta_info = ssv_sta_priv->sta_info;
	}
	if ((!sc->bq4_dtim) &&
	    (ieee80211_is_mgmt(hdr->frame_control) ||
	     ieee80211_is_nullfunc(hdr->frame_control) ||
	     ieee80211_is_qos_nullfunc(hdr->frame_control))) {
		ac = 4;
		hw_txqid = 4;
	} else if ((sc->bq4_dtim) &&
		   info->flags & IEEE80211_TX_CTL_SEND_AFTER_DTIM) {
		hw_txqid = 4;
		ac = 4;
	} else {
		ac = skb_get_queue_mapping(skb);
		hw_txqid = sc->tx.hw_txqid[ac];
	}
	tx_drate = &info->control.rates[0];
	ssv6xxx_rc_hw_rate_idx(sc, info, &ssv_rate);
	tx_desc->len = skb->len;
	tx_desc->c_type = M2_TXREQ;
	tx_desc->f80211 = 1;
	tx_desc->qos = (ieee80211_is_data_qos(hdr->frame_control)) ? 1 : 0;
	if (tx_drate->flags & IEEE80211_TX_RC_MCS) {
		if (ieee80211_is_mgmt(hdr->frame_control) &&
		    ieee80211_has_order(hdr->frame_control))
			tx_desc->ht = 1;
	}
	tx_desc->use_4addr = (ieee80211_has_a4(hdr->frame_control)) ? 1 : 0;
	tx_desc->more_data =
	    (ieee80211_has_morefrags(hdr->frame_control)) ? 1 : 0;
	tx_desc->stype_b5b4 = (cpu_to_le16(hdr->frame_control) >> 4) & 0x3;
	tx_desc->frag = (tx_desc->more_data || (hdr->seq_ctrl & 0xf)) ? 1 : 0;
	tx_desc->unicast = (is_multicast_ether_addr(hdr->addr1)) ? 0 : 1;
	tx_desc->tx_burst = (tx_desc->frag) ? 1 : 0;
	tx_desc->wsid = (!sta_info
			 || (sta_info->hw_wsid < 0)) ? 0x0F : sta_info->hw_wsid;
	tx_desc->txq_idx = hw_txqid;
	tx_desc->hdr_offset = TXPB_OFFSET;
	tx_desc->hdr_len = ssv6xxx_frame_hdrlen(hdr, tx_desc->ht);
	tx_desc->payload_offset = tx_desc->hdr_offset + tx_desc->hdr_len;
	if (info->control.use_rts)
		tx_desc->do_rts_cts = IEEE80211_TX_RC_USE_RTS_CTS;
	else if (info->control.use_cts_prot)
		tx_desc->do_rts_cts = IEEE80211_TX_RC_USE_CTS_PROTECT;
	if (tx_desc->do_rts_cts == IEEE80211_TX_RC_USE_CTS_PROTECT)
		tx_desc->do_rts_cts = IEEE80211_TX_RC_USE_RTS_CTS;
	if (tx_desc->do_rts_cts == IEEE80211_TX_RC_USE_CTS_PROTECT) {
		tx_desc->crate_idx = 0;
	} else
		tx_desc->crate_idx = ssv_rate.crate_hw_idx;
	tx_desc->drate_idx = ssv_rate.drate_hw_idx;
	if (tx_desc->unicast == 0)
		tx_desc->ack_policy = 1;
	else if (tx_desc->qos == 1)
		tx_desc->ack_policy = (*ieee80211_get_qos_ctl(hdr) & 0x60) >> 5;
	else if (ieee80211_is_ctl(hdr->frame_control))
		tx_desc->ack_policy = 1;
	tx_desc->security = 0;
	tx_desc->fCmdIdx = 0;
	tx_desc->fCmd = (hw_txqid + M_ENG_TX_EDCA0);
	if (info->flags & IEEE80211_TX_CTL_AMPDU) {
#ifdef AMPDU_HAS_LEADING_FRAME
		tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_CPU;
#else
		tx_desc->RSVD_1 = 1;
#endif
		tx_desc->aggregation = 1;
		tx_desc->ack_policy = 0x01;
		if ((tx_desc->do_rts_cts == 0)
		    && ((sc->hw->wiphy->rts_threshold == (-1))
			|| ((skb->len - sc->sh->tx_desc_len) >
			    sc->hw->wiphy->rts_threshold))) {
			tx_drate->flags |= IEEE80211_TX_RC_USE_RTS_CTS;
			tx_desc->do_rts_cts = 1;
		}
	}
	if (ieee80211_has_protected(hdr->frame_control)
	    && (ieee80211_is_data_qos(hdr->frame_control)
		|| ieee80211_is_data(hdr->frame_control))) {
		if ((tx_desc->unicast && ssv_sta_priv
		     && ssv_sta_priv->has_hw_encrypt)
		    || (!tx_desc->unicast && vif_priv
			&& vif_priv->has_hw_encrypt)) {
			if (!tx_desc->unicast
			    && !list_empty(&vif_priv->sta_list)) {
				struct ssv_sta_priv_data *one_sta_priv;
				int hw_wsid;
				one_sta_priv =
				    list_first_entry(&vif_priv->sta_list,
						     struct ssv_sta_priv_data,
						     list);
				hw_wsid = one_sta_priv->sta_info->hw_wsid;
				if (hw_wsid != (-1)) {
					tx_desc->wsid = hw_wsid;
				}
			}
			tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_ENCRYPT;
		} else if (ssv_sta_priv->need_sw_encrypt) {
		} else {
		}
	} else {
	}
	tx_desc->fCmd = (tx_desc->fCmd << 4) | M_ENG_HWHCI;
	if (tx_desc->aggregation == 1) {
		struct ampdu_hdr_st *ampdu_hdr =
		    (struct ampdu_hdr_st *)skb->head;
		memcpy(&tx_desc->rc_params[0], ampdu_hdr->rates,
		       sizeof(tx_desc->rc_params));
		nav =
		    ssv6xxx_set_frame_duration(info, &ssv_rate,
					       (skb->len + FCS_LEN), tx_desc,
					       &tx_desc->rc_params[0], sc);
#ifdef FW_RC_RETRY_DEBUG
		{
			dev_dbg
			    (sc->dev, "[FW_RC]:param[0]: drate =%d, count =%d, crate=%d, dl_length =%d, frame_consume_time =%d, rts_cts_nav=%d\n",
			     tx_desc->rc_params[0].drate,
			     tx_desc->rc_params[0].count,
			     tx_desc->rc_params[0].crate,
			     tx_desc->rc_params[0].dl_length,
			     tx_desc->rc_params[0].frame_consume_time,
			     tx_desc->rc_params[0].rts_cts_nav);
			dev_dbg
			    (sc->dev, "[FW_RC]:param[1]: drate =%d, count =%d, crate=%d, dl_length =%d, frame_consume_time =%d, rts_cts_nav=%d\n",
			     tx_desc->rc_params[1].drate,
			     tx_desc->rc_params[1].count,
			     tx_desc->rc_params[1].crate,
			     tx_desc->rc_params[1].dl_length,
			     tx_desc->rc_params[1].frame_consume_time,
			     tx_desc->rc_params[1].rts_cts_nav);
			dev_dbg
			    (sc->dev, "[FW_RC]:param[2]: drate =%d, count =%d, crate=%d, dl_length =%d, frame_consume_time =%d, rts_cts_nav=%d\n",
			     tx_desc->rc_params[2].drate,
			     tx_desc->rc_params[2].count,
			     tx_desc->rc_params[2].crate,
			     tx_desc->rc_params[2].dl_length,
			     tx_desc->rc_params[2].frame_consume_time,
			     tx_desc->rc_params[2].rts_cts_nav);
		}
#endif
	} else {
		nav =
		    ssv6xxx_set_frame_duration(info, &ssv_rate,
					       (skb->len + FCS_LEN), tx_desc,
					       NULL, NULL);
	}
	if ((tx_desc->aggregation == 0)) {
		if (tx_desc->tx_burst == 0) {
			if (tx_desc->ack_policy != 0x01)
				hdr->duration_id = nav;
		} else {
		}
	}
}

void ssv6xxx_add_txinfo(struct ssv_softc *sc, struct sk_buff *skb)
{
	struct ssv6200_tx_desc *tx_desc;
	skb_push(skb, sc->sh->tx_desc_len);
	tx_desc = (struct ssv6200_tx_desc *)skb->data;
	memset((void *)tx_desc, 0, sc->sh->tx_desc_len);
	ssv6xxx_update_txinfo(sc, skb);
}

int ssv6xxx_get_real_index(struct ssv_softc *sc, struct sk_buff *skb)
{
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_tx_rate *tx_drate;
	struct ssv_rate_info ssv_rate;
	tx_drate = &info->control.rates[0];
	ssv6xxx_rc_hw_rate_idx(sc, info, &ssv_rate);
	return ssv_rate.drate_hw_idx;
}

static void _ssv6xxx_tx(struct ieee80211_hw *hw, struct sk_buff *skb)
{
	struct ssv_softc *sc = hw->priv;
	struct ieee80211_tx_info *info = IEEE80211_SKB_CB(skb);
	struct ieee80211_vif *vif = info->control.vif;
	struct ieee80211_hdr *hdr = (struct ieee80211_hdr *)skb->data;
	struct ssv6200_tx_desc *tx_desc;
	int ret;
	unsigned long flags;
	bool send_hci = false;
	do {
		if (info->flags & IEEE80211_TX_CTL_ASSIGN_SEQ) {
			if (info->flags & IEEE80211_TX_CTL_FIRST_FRAGMENT)
				sc->tx.seq_no += 0x10;
			hdr->seq_ctrl &= cpu_to_le16(IEEE80211_SCTL_FRAG);
			hdr->seq_ctrl |= cpu_to_le16(sc->tx.seq_no);
		}
		if (info->flags & IEEE80211_TX_CTL_AMPDU) {
			if (ssv6xxx_get_real_index(sc, skb) <
			    SSV62XX_RATE_MCS_INDEX) {
				info->flags &= (~IEEE80211_TX_CTL_AMPDU);
				goto tx_mpdu;
			}
			if (ssv6200_ampdu_tx_handler(hw, skb)) {
				break;
			} else {
				info->flags &= (~IEEE80211_TX_CTL_AMPDU);
			}
		}
 tx_mpdu:
		ssv6xxx_add_txinfo(sc, skb);
		if (vif &&
		    vif->type == NL80211_IFTYPE_AP &&
		    (sc->bq4_dtim) &&
		    info->flags & IEEE80211_TX_CTL_SEND_AFTER_DTIM) {
			struct ssv_vif_priv_data *priv_vif =
			    (struct ssv_vif_priv_data *)vif->drv_priv;
			u8 buffered = 0;
			spin_lock_irqsave(&sc->ps_state_lock, flags);
			if (priv_vif->sta_asleep_mask) {
				buffered =
				    ssv6200_bcast_enqueue(sc, &sc->bcast_txq,
							  skb);
				if (1 == buffered) {
					dev_dbg(sc->dev, "ssv6200_tx:ssv6200_bcast_start\n");
					ssv6200_bcast_start(sc);
				}
			}
			spin_unlock_irqrestore(&sc->ps_state_lock, flags);
			if (buffered)
				break;
		}
		if (info->flags & IEEE80211_TX_CTL_SEND_AFTER_DTIM) {
			struct ssv_vif_priv_data *vif_priv =
			    (struct ssv_vif_priv_data *)vif->drv_priv;
			dev_dbg(sc->dev, "vif[%d] sc->bq4_dtim[%d]\n",
				vif_priv->vif_idx, sc->bq4_dtim);
		}
		tx_desc = (struct ssv6200_tx_desc *)skb->data;
		ret = HCI_SEND(sc->sh, skb, tx_desc->txq_idx);
		send_hci = true;
	} while (0);
	if ((skb_queue_len(&sc->tx_skb_q) < LOW_TX_Q_LEN)
	    ) {
		if (sc->tx.flow_ctrl_status != 0) {
			int ac;
			for (ac = 0; ac < sc->hw->queues; ac++) {
				if ((sc->tx.flow_ctrl_status & BIT(ac)) == 0)
					ieee80211_wake_queue(sc->hw, ac);
			}
		} else {
			ieee80211_wake_queues(sc->hw);
		}
	}
}

static void ssv6200_tx(struct ieee80211_hw *hw,
		       struct ieee80211_tx_control *control,
		       struct sk_buff *skb)
{
	struct ssv_softc *sc = (struct ssv_softc *)hw->priv;
	struct SKB_info_st *skb_info = (struct SKB_info_st *)skb->head;
	skb_info->sta = control ? control->sta : NULL;
#ifdef CONFIG_DEBUG_SKB_TIMESTAMP
	skb_info->timestamp = ktime_get();
#endif
	skb_queue_tail(&sc->tx_skb_q, skb);
#ifdef CONFIG_SSV6XXX_DEBUGFS
	if (sc->max_tx_skb_q_len < skb_queue_len(&sc->tx_skb_q))
		sc->max_tx_skb_q_len = skb_queue_len(&sc->tx_skb_q);
#endif
	wake_up_interruptible(&sc->tx_wait_q);
	do {
		if (skb_queue_len(&sc->tx_skb_q) >= MAX_TX_Q_LEN)
			ieee80211_stop_queues(sc->hw);
	} while (0);
}

int ssv6xxx_tx_task(void *data)
{
	struct ssv_softc *sc = (struct ssv_softc *)data;
	u32 wait_period = SSV_AMPDU_timer_period / 2;
	dev_info(sc->dev, "TX Task started\n");
	while (!kthread_should_stop()) {
		u32 before_timeout = (-1);
		set_current_state(TASK_INTERRUPTIBLE);
		before_timeout = wait_event_interruptible_timeout(sc->tx_wait_q,
								  (skb_queue_len
								   (&sc->
								    tx_skb_q)
								   ||
								   kthread_should_stop
								   ()
								   || sc->
								   tx_q_empty),
								  msecs_to_jiffies
								  (wait_period));
		if (kthread_should_stop()) {
			dev_dbg(sc->dev, "Quit TX task loop...\n");
			break;
		}
		set_current_state(TASK_RUNNING);
		do {
			struct sk_buff *tx_skb = skb_dequeue(&sc->tx_skb_q);
			if (tx_skb == NULL)
				break;
			_ssv6xxx_tx(sc->hw, tx_skb);
		} while (1);
#ifdef CONFIG_DEBUG_SKB_TIMESTAMP
		{
			struct ssv_hw_txq *hw_txq = NULL;
			struct ieee80211_tx_info *tx_info = NULL;
			struct sk_buff *skb = NULL;
			int txqid;
			unsigned int timeout;
			u32 status;
			for (txqid = 0; txqid < SSV_HW_TXQ_NUM; txqid++) {
				hw_txq = &ssv_dbg_ctrl_hci->hw_txq[txqid];
				skb = skb_peek(&hw_txq->qhead);
				if (skb != NULL) {
					tx_info = IEEE80211_SKB_CB(skb);
					if (tx_info->
					    flags & IEEE80211_TX_CTL_AMPDU)
						timeout =
						    cal_duration_of_ampdu(skb,
									  SKB_DURATION_STAGE_IN_HWQ);
					else
						timeout =
						    cal_duration_of_mpdu(skb);
					if (timeout > SKB_DURATION_TIMEOUT_MS) {
						HCI_IRQ_STATUS(ssv_dbg_ctrl_hci,
							       &status);
						dev_dbg(sc->dev, "hci int_mask: %08x\n",
						       ssv_dbg_ctrl_hci->
						       int_mask);
						dev_dbg(sc->dev, "sdio status: %08x\n",
						       status);
						dev_dbg(sc->dev, "hwq%d len: %d\n", txqid,
						       skb_queue_len(&hw_txq->
								     qhead));
					}
				}
			}
		}
#endif
		if (sc->tx_q_empty || (before_timeout == 0)) {
			u32 flused_ampdu = ssv6xxx_ampdu_flush(sc->hw);
			sc->tx_q_empty = false;
			if (flused_ampdu == 0 && before_timeout == 0) {
				wait_period *= 2;
				if (wait_period > 1000)
					wait_period = 1000;
			}
		} else
			wait_period = SSV_AMPDU_timer_period / 2;
	}
	return 0;
}

int ssv6xxx_rx_task(void *data)
{
	struct ssv_softc *sc = (struct ssv_softc *)data;
	unsigned long wait_period = msecs_to_jiffies(200);
	unsigned long last_timeout_check_jiffies = jiffies;
	unsigned long cur_jiffies;
	dev_info(sc->dev, "RX Task started\n");
	while (!kthread_should_stop()) {
		u32 before_timeout = (-1);
		set_current_state(TASK_INTERRUPTIBLE);
		before_timeout = wait_event_interruptible_timeout(sc->rx_wait_q,
								  (skb_queue_len
								   (&sc->
								    rx_skb_q)
								   ||
								   skb_queue_len
								   (&sc->
								    tx_done_q)
								   ||
								   kthread_should_stop
								   ()),
								  wait_period);
		if (kthread_should_stop()) {
			dev_dbg(sc->dev, "Quit RX task loop...\n");
			break;
		}
		set_current_state(TASK_RUNNING);
		cur_jiffies = jiffies;
		if ((before_timeout == 0)
		    || time_before((last_timeout_check_jiffies + wait_period),
				   cur_jiffies)) {
			ssv6xxx_ampdu_check_timeout(sc->hw);
			last_timeout_check_jiffies = cur_jiffies;
		}
		if (skb_queue_len(&sc->rx_skb_q))
			_process_rx_q(sc, &sc->rx_skb_q, NULL);
		if (skb_queue_len(&sc->tx_done_q))
			_process_tx_done(sc);
	}
	return 0;
}

struct ssv6xxx_iqk_cfg init_iqk_cfg = {
	SSV6XXX_IQK_CFG_XTAL_26M,
#ifdef CONFIG_SSV_DPD
	SSV6XXX_IQK_CFG_PA_LI_MPB,
#else
	SSV6XXX_IQK_CFG_PA_DEF,
#endif
	0,
	0,
	26,
	3,
	0x75,
	0x75,
	0x80,
	0x80,
	SSV6XXX_IQK_CMD_INIT_CALI,
	{SSV6XXX_IQK_TEMPERATURE
	 + SSV6XXX_IQK_RXDC
	 + SSV6XXX_IQK_RXRC
	 + SSV6XXX_IQK_TXDC + SSV6XXX_IQK_TXIQ + SSV6XXX_IQK_RXIQ
#ifdef CONFIG_SSV_DPD
	 + SSV6XXX_IQK_PAPD
#endif
	 },
};

static int ssv6200_start(struct ieee80211_hw *hw)
{
	struct ssv_softc *sc = hw->priv;
	struct ssv_hw *sh = sc->sh;
	struct ieee80211_channel *chan;
    int ret;
    
	mutex_lock(&sc->mutex);
    ret = ssv6xxx_init_mac(sc->sh);
	if (ret != 0) {
		dev_err(sc->dev, "Failed to initialize mac, ret=%d\n", ret);
		ssv6xxx_deinit_mac(sc);
		mutex_unlock(&sc->mutex);
		return -1;
	}
#ifdef CONFIG_P2P_NOA
	ssv6xxx_noa_reset(sc);
#endif
	HCI_START(sh);
	ieee80211_wake_queues(hw);
	ssv6200_ampdu_init(hw);
	sc->watchdog_flag = WD_KICKED;
	mutex_unlock(&sc->mutex);
	mod_timer(&sc->watchdog_timeout, jiffies + WATCHDOG_TIMEOUT);
#ifdef CONFIG_SSV_SMARTLINK
	{
		extern int ksmartlink_init(void);
		(void)ksmartlink_init();
	}
#endif
    ret = ssv6xxx_do_iq_calib(sc->sh, &init_iqk_cfg);
    if (ret != 0) {
        dev_err(sc->dev, "IQ Calibration failed, ret=%d\n", ret);
        return ret;
    }

	dev_info(sc->dev, "Calibration successful\n");

	SMAC_REG_WRITE(sc->sh, ADR_PHY_EN_1, 0x217f);
	if ((sh->cfg.chip_identity == SSV6051Z)
	    || (sc->sh->cfg.chip_identity == SSV6051P)) {
		int i;
		for (i = 0; i < sh->ch_cfg_size; i++) {
			SMAC_REG_READ(sh, sh->p_ch_cfg[i].reg_addr,
				      &sh->p_ch_cfg[i].ch1_12_value);
		}
	}
	chan = hw->conf.chandef.chan;
	sc->cur_channel = chan;
	dev_dbg(sc->dev, "%s(): current channel: %d,sc->ps_status=%d\n", __FUNCTION__,
	       sc->cur_channel->hw_value, sc->ps_status);
	ssv6xxx_set_channel(sc, chan->hw_value);
	ssv6xxx_rf_enable(sh);
	return 0;
}

static void ssv6200_stop(struct ieee80211_hw *hw)
{
	struct ssv_softc *sc = hw->priv;
	u32 count = 0;
	struct rssi_res_st *rssi_tmp0, *rssi_tmp1;
	dev_dbg(sc->dev, "%s(): sc->ps_status=%d\n", __FUNCTION__,
	       sc->ps_status);
	mutex_lock(&sc->mutex);
	list_for_each_entry_safe(rssi_tmp0, rssi_tmp1, &rssi_res.rssi_list,
				 rssi_list) {
		list_del(&rssi_tmp0->rssi_list);
		kfree(rssi_tmp0);
	}
	ssv6200_ampdu_deinit(hw);
	ssv6xxx_rf_disable(sc->sh);
	HCI_STOP(sc->sh);
#ifndef NO_USE_RXQ_LOCK
	while (0) {
#else
	while (skb_queue_len(&sc->rx.rxq_head)) {
#endif
		dev_dbg(sc->dev, "sc->rx.rxq_count=%d\n", sc->rx.rxq_count);
		count++;
		if (count > 90000000) {
			dev_err(sc->dev, "Could not empty RX queue during shutdown\n");
			break;
		}
	}
	HCI_TXQ_FLUSH(sc->sh, (TXQ_EDCA_0 | TXQ_EDCA_1 | TXQ_EDCA_2 |
			       TXQ_EDCA_3 | TXQ_MGMT));
	if ((sc->ps_status == PWRSV_PREPARE) || (sc->ps_status == PWRSV_ENABLE)) {
		ssv6xxx_enable_ps(sc);
		ssv6xxx_rf_enable(sc->sh);
	}
	sc->watchdog_flag = WD_SLEEP;
	mutex_unlock(&sc->mutex);
	del_timer_sync(&sc->watchdog_timeout);
#ifdef CONFIG_SSV_SMARTLINK
	{
		extern void ksmartlink_exit(void);
		ksmartlink_exit();
	}
#endif
	dev_dbg(sc->dev, "%s(): leave\n", __FUNCTION__);
}

void inline ssv62xxx_set_bssid(struct ssv_softc *sc, u8 * bssid)
{
	memcpy(sc->bssid, bssid, 6);
	SMAC_REG_WRITE(sc->sh, ADR_BSSID_0, *((u32 *) & sc->bssid[0]));
	SMAC_REG_WRITE(sc->sh, ADR_BSSID_1, *((u32 *) & sc->bssid[4]));
}

struct ssv_vif_priv_data *ssv6xxx_config_vif_res(struct ssv_softc *sc,
						 struct ieee80211_vif *vif)
{
	int i;
	struct ssv_vif_priv_data *priv_vif;
	struct ssv_vif_info *vif_info;
	lockdep_assert_held(&sc->mutex);
	for (i = 0; i < SSV6200_MAX_VIF; i++) {
		if (sc->vif_info[i].vif == NULL)
			break;
	}
	BUG_ON(i >= SSV6200_MAX_VIF);
	dev_dbg(sc->dev, "ssv6xxx_config_vif_res id[%d].\n", i);
	priv_vif = (struct ssv_vif_priv_data *)vif->drv_priv;
	memset(priv_vif, 0, sizeof(struct ssv_vif_priv_data));
	priv_vif->vif_idx = i;
	memset(&sc->vif_info[i], 0, sizeof(sc->vif_info[0]));
	sc->vif_info[i].vif = vif;
	sc->vif_info[i].vif_priv = priv_vif;
	INIT_LIST_HEAD(&priv_vif->sta_list);
	priv_vif->pair_cipher = SSV_CIPHER_NONE;
	priv_vif->group_cipher = SSV_CIPHER_NONE;
	priv_vif->has_hw_decrypt = false;
	priv_vif->has_hw_encrypt = false;
	priv_vif->need_sw_encrypt = false;
	priv_vif->need_sw_decrypt = false;
	priv_vif->use_mac80211_decrypt = false;
	priv_vif->is_security_valid = false;
	priv_vif->force_sw_encrypt = (vif->type == NL80211_IFTYPE_AP);
	vif_info = &sc->vif_info[priv_vif->vif_idx];
	vif_info->if_type = vif->type;
	vif_info->vif = vif;
	return priv_vif;
}

static int ssv6200_add_interface(struct ieee80211_hw *hw,
				 struct ieee80211_vif *vif)
{
	struct ssv_softc *sc = hw->priv;
	int ret = 0;
	struct ssv_vif_priv_data *vif_priv = NULL;
	dev_dbg(sc->dev, "[I] %s(): vif->type = %d, NL80211_IFTYPE_AP=%d\n", __FUNCTION__,
	       vif->type, NL80211_IFTYPE_AP);
	if ((sc->nvif >= SSV6200_MAX_VIF)
	    || (((vif->type == NL80211_IFTYPE_AP)
		 || (vif->p2p))
		&& (sc->ap_vif != NULL))) {
		dev_err(sc->dev, "Add interface of type %d (p2p: %d) failed.\n",
			vif->type, vif->p2p);
		return -EOPNOTSUPP;
	}
	mutex_lock(&sc->mutex);
	vif_priv = ssv6xxx_config_vif_res(sc, vif);
	if ((vif_priv->vif_idx == 0) && (vif->p2p == 0)
	    && (vif->type == NL80211_IFTYPE_AP)) {
		dev_dbg(sc->dev, "VIF[0] set bssid and config opmode to ap\n");
		ssv62xxx_set_bssid(sc, sc->sh->cfg.maddr[0]);
		SMAC_REG_SET_BITS(sc->sh, ADR_GLBLE_SET, SSV6200_OPMODE_AP,
				  OP_MODE_MSK);
	}
	if (vif->type == NL80211_IFTYPE_AP) {
		BUG_ON(sc->ap_vif != NULL);
		sc->ap_vif = vif;
		if (!vif->p2p && (vif_priv->vif_idx == 0)) {
			dev_dbg(sc->dev, "Normal AP mode. Config Q4 to DTIM Q.\n");
			SMAC_REG_SET_BITS(sc->sh, ADR_MTX_BCN_EN_MISC,
					  MTX_HALT_MNG_UNTIL_DTIM_MSK,
					  MTX_HALT_MNG_UNTIL_DTIM_MSK);
			sc->bq4_dtim = true;
		}
	}
	sc->nvif++;
	dev_dbg(sc->dev,
		"VIF %02x:%02x:%02x:%02x:%02x:%02x of type %d is added.\n",
		vif->addr[0], vif->addr[1], vif->addr[2], vif->addr[3],
		vif->addr[4], vif->addr[5], vif->type);
#ifdef CONFIG_SSV6XXX_DEBUGFS
	ssv6xxx_debugfs_add_interface(sc, vif);
#endif
	mutex_unlock(&sc->mutex);
	return ret;
}

static void ssv6200_remove_interface(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif)
{
	struct ssv_softc *sc = hw->priv;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)vif->drv_priv;
	dev_err(sc->dev,
		"Removing interface %02x:%02x:%02x:%02x:%02x:%02x. PS=%d\n",
		vif->addr[0], vif->addr[1], vif->addr[2], vif->addr[3],
		vif->addr[4], vif->addr[5], sc->ps_status);
	mutex_lock(&sc->mutex);
#ifdef CONFIG_SSV6XXX_DEBUGFS
	ssv6xxx_debugfs_remove_interface(sc, vif);
#endif
	if (vif->type == NL80211_IFTYPE_AP) {
		if (sc->bq4_dtim) {
			sc->bq4_dtim = false;
			ssv6200_release_bcast_frame_res(sc, vif);
			SMAC_REG_SET_BITS(sc->sh, ADR_MTX_BCN_EN_MISC,
					  0, MTX_HALT_MNG_UNTIL_DTIM_MSK);
			dev_dbg(sc->dev, "Config Q4 to normal Q \n");
		}
		ssv6xxx_beacon_release(sc);
		sc->ap_vif = NULL;
	}
	memset(&sc->vif_info[vif_priv->vif_idx], 0,
	       sizeof(struct ssv_vif_info));
	sc->nvif--;
	mutex_unlock(&sc->mutex);
}

static int ssv6200_change_interface(struct ieee80211_hw *dev,
				    struct ieee80211_vif *vif,
				    enum nl80211_iftype new_type, bool p2p)
{
    struct ssv_softc *sc = dev->priv;
	int ret = 0;
    
	dev_dbg(sc->dev, "change_interface new: %d (%d), old: %d (%d)\n", new_type,
	       p2p, vif->type, vif->p2p);

	if (new_type != vif->type || vif->p2p != p2p) {
		ssv6200_remove_interface(dev, vif);
		vif->type = new_type;
		vif->p2p = p2p;
		ret = ssv6200_add_interface(dev, vif);
	}

	return ret;
}

void ssv6xxx_ps_callback_func(unsigned long data)
{
	struct ssv_softc *sc = (struct ssv_softc *)data;
	struct sk_buff *skb;
	struct cfg_host_cmd *host_cmd;
	int retry_cnt = 20;
#ifdef SSV_WAKEUP_HOST
	SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_MNG,
		       M_ENG_MACRX | (M_ENG_CPU << 4) | (M_ENG_HWHCI << 8));
	SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_DATA,
		       M_ENG_MACRX | (M_ENG_CPU << 4) | (M_ENG_HWHCI << 8));
	SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_TB0 + 6 * 4,
		       (sc->mac_deci_tbl[6] | 1));
#else
	SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_MNG,
		       M_ENG_MACRX | (M_ENG_TRASH_CAN << 4));
	SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_DATA,
		       M_ENG_MACRX | (M_ENG_TRASH_CAN << 4));
	SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_MNG,
		       M_ENG_MACRX | (M_ENG_TRASH_CAN << 4));
#endif
	skb = ssv_skb_alloc(sizeof(struct cfg_host_cmd));
	skb->data_len = sizeof(struct cfg_host_cmd);
	skb->len = skb->data_len;
	host_cmd = (struct cfg_host_cmd *)skb->data;
	host_cmd->c_type = HOST_CMD;
	host_cmd->RSVD0 = 0;
	host_cmd->h_cmd = (u8) SSV6XXX_HOST_CMD_PS;
	host_cmd->len = skb->data_len;
#ifdef SSV_WAKEUP_HOST
	host_cmd->dummy = sc->ps_aid;
#else
	host_cmd->dummy = 0;
#endif
	sc->ps_aid = 0;
	while ((HCI_SEND_CMD(sc->sh, skb) != 0) && (retry_cnt)) {
		dev_warn(sc->dev, "PS cmd retry=%d!!\n", retry_cnt);
		retry_cnt--;
	}
	ssv_skb_free(skb);
	dev_dbg(sc->dev, "SSV6XXX_HOST_CMD_PS,ps_aid = %d,len=%d,tabl=0x%x\n",
	       host_cmd->dummy, skb->len, (sc->mac_deci_tbl[6] | 1));
}

void ssv6xxx_enable_ps(struct ssv_softc *sc)
{
	sc->ps_status = PWRSV_ENABLE;
}

void ssv6xxx_disable_ps(struct ssv_softc *sc)
{
	sc->ps_status = PWRSV_DISABLE;
	dev_info(sc->dev, "Power saving disabled\n");
}

int ssv6xxx_watchdog_controller(struct ssv_hw *sh, u8 flag)
{
	struct sk_buff *skb;
	struct cfg_host_cmd *host_cmd;
	int ret = 0;
	dev_dbg(sh->sc->dev, "ssv6xxx_watchdog_controller %d\n", flag);
	skb = ssv_skb_alloc(HOST_CMD_HDR_LEN);
	if (skb == NULL) {
		dev_warn(sh->sc->dev, "init ssv6xxx_watchdog_controller fail!!!\n");
		return (-1);
	}
	skb->data_len = HOST_CMD_HDR_LEN;
	skb->len = skb->data_len;
	host_cmd = (struct cfg_host_cmd *)skb->data;
	host_cmd->c_type = HOST_CMD;
	host_cmd->h_cmd = (u8) flag;
	host_cmd->len = skb->data_len;
	sh->hci.hci_ops->hci_send_cmd(skb);
	ssv_skb_free(skb);
	return ret;
}

static int ssv6200_config(struct ieee80211_hw *hw, u32 changed)
{
	struct ssv_softc *sc = hw->priv;
	int ret = 0;
	mutex_lock(&sc->mutex);
	if (changed & IEEE80211_CONF_CHANGE_PS) {
		struct ieee80211_conf *conf = &hw->conf;
		if (conf->flags & IEEE80211_CONF_PS) {
			dev_dbg(sc->dev, "Enable IEEE80211_CONF_PS ps_aid=%d\n",
			       sc->ps_aid);
		} else {
			dev_dbg(sc->dev, "Disable IEEE80211_CONF_PS ps_aid=%d\n",
			       sc->ps_aid);
		}
	}
	if (changed & IEEE80211_CONF_CHANGE_CHANNEL) {
		struct ieee80211_channel *chan;
		chan = hw->conf.chandef.chan;
#ifdef CONFIG_P2P_NOA
		if (sc->p2p_noa.active_noa_vif) {
			dev_dbg(sc->dev, "NOA operating-active vif[%02x] skip scan\n",
			       sc->p2p_noa.active_noa_vif);
			goto out;
		}
#endif
		if (hw->conf.flags & IEEE80211_CONF_OFFCHANNEL) {
			if ((sc->ap_vif == NULL)
			    ||
			    list_empty(&
				       ((struct ssv_vif_priv_data *)sc->ap_vif->
					drv_priv)->sta_list)) {
				HCI_PAUSE(sc->sh,
					  (TXQ_EDCA_0 | TXQ_EDCA_1 | TXQ_EDCA_2
					   | TXQ_EDCA_3 | TXQ_MGMT));
				sc->sc_flags |= SC_OP_OFFCHAN;
				ssv6xxx_set_channel(sc, chan->hw_value);
				sc->hw_chan = chan->hw_value;
				HCI_RESUME(sc->sh, TXQ_MGMT);
			} else {
				dev_dbg(sc->dev,
					"Off-channel to %d is ignored when AP mode enabled.\n",
					chan->hw_value);
			}
		} else {
			if ((sc->cur_channel == NULL)
			    || (sc->sc_flags & SC_OP_OFFCHAN)
			    || (sc->hw_chan != chan->hw_value)) {
				HCI_PAUSE(sc->sh,
					  (TXQ_EDCA_0 | TXQ_EDCA_1 | TXQ_EDCA_2
					   | TXQ_EDCA_3 | TXQ_MGMT));
				ssv6xxx_set_channel(sc, chan->hw_value);
				sc->cur_channel = chan;
				HCI_RESUME(sc->sh,
					   (TXQ_EDCA_0 | TXQ_EDCA_1 | TXQ_EDCA_2
					    | TXQ_EDCA_3 | TXQ_MGMT));
				sc->sc_flags &= ~SC_OP_OFFCHAN;
			} else {
				dev_dbg(sc->dev,
					"Change to the same channel %d\n",
					chan->hw_value);
			}
		}
	}
#ifdef CONFIG_P2P_NOA
 out:
#endif
	mutex_unlock(&sc->mutex);
	return ret;
}

#define SUPPORTED_FILTERS \
    (FIF_ALLMULTI | \
    FIF_CONTROL | \
    FIF_PSPOLL | \
    FIF_OTHER_BSS | \
    FIF_BCN_PRBRESP_PROMISC | \
    FIF_PROBE_REQ | \
    FIF_FCSFAIL)
static void ssv6200_config_filter(struct ieee80211_hw *hw,
				  unsigned int changed_flags,
				  unsigned int *total_flags, u64 multicast)
{
	changed_flags &= SUPPORTED_FILTERS;
	*total_flags &= SUPPORTED_FILTERS;
}

static void ssv6200_bss_info_changed(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif,
				     struct ieee80211_bss_conf *info,
				     u64 changed)
{
	struct ssv_vif_priv_data *priv_vif = (struct ssv_vif_priv_data *)vif->drv_priv;
	struct ssv_softc *sc = hw->priv;
#ifdef CONFIG_P2P_NOA
	u8 null_address[6] = { 0 };
#endif
	mutex_lock(&sc->mutex);
	if (changed & BSS_CHANGED_ERP_PREAMBLE) {
		dev_dbg(sc->dev, "BSS Changed use_short_preamble[%d]\n",
		       info->use_short_preamble);
		if (info->use_short_preamble)
			sc->sc_flags |= SC_OP_SHORT_PREAMBLE;
		else
			sc->sc_flags &= ~SC_OP_SHORT_PREAMBLE;
	}
	if (!priv_vif->vif_idx) {
		if (changed & BSS_CHANGED_BSSID) {
#ifdef CONFIG_P2P_NOA
			struct ssv_vif_priv_data *vif_priv;
			vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
#endif
			ssv62xxx_set_bssid(sc, (u8 *) info->bssid);
			dev_dbg(sc->dev, "BSS_CHANGED_BSSID: %02x:%02x:%02x:%02x:%02x:%02x\n",
			     info->bssid[0], info->bssid[1], info->bssid[2],
			     info->bssid[3], info->bssid[4], info->bssid[5]);
#ifdef CONFIG_P2P_NOA
			if (memcmp(info->bssid, null_address, 6))
				ssv6xxx_noa_hdl_bss_change(sc,
							   MONITOR_NOA_CONF_ADD,
							   vif_priv->vif_idx);
			else
				ssv6xxx_noa_hdl_bss_change(sc,
							   MONITOR_NOA_CONF_REMOVE,
							   vif_priv->vif_idx);
#endif
		}
		if (changed & BSS_CHANGED_ERP_SLOT) {
			u32 regval = 0;
			dev_dbg(sc->dev, "BSS_CHANGED_ERP_SLOT: use_short_slot[%d]\n",
			       info->use_short_slot);
			if (info->use_short_slot) {
				SMAC_REG_READ(sc->sh, ADR_MTX_DUR_IFS, &regval);
				regval = regval & MTX_DUR_SLOT_I_MSK;
				regval |= 9 << MTX_DUR_SLOT_SFT;
				SMAC_REG_WRITE(sc->sh, ADR_MTX_DUR_IFS, regval);
				SMAC_REG_READ(sc->sh, ADR_MTX_DUR_SIFS_G,
					      &regval);
				regval = regval & MTX_DUR_BURST_SIFS_G_I_MSK;
				regval |= 0xa << MTX_DUR_BURST_SIFS_G_SFT;
				regval = regval & MTX_DUR_SLOT_G_I_MSK;
				regval |= 9 << MTX_DUR_SLOT_G_SFT;
				SMAC_REG_WRITE(sc->sh, ADR_MTX_DUR_SIFS_G,
					       regval);
			} else {
				SMAC_REG_READ(sc->sh, ADR_MTX_DUR_IFS, &regval);
				regval = regval & MTX_DUR_SLOT_I_MSK;
				regval |= 20 << MTX_DUR_SLOT_SFT;
				SMAC_REG_WRITE(sc->sh, ADR_MTX_DUR_IFS, regval);
				SMAC_REG_READ(sc->sh, ADR_MTX_DUR_SIFS_G,
					      &regval);
				regval = regval & MTX_DUR_BURST_SIFS_G_I_MSK;
				regval |= 0xa << MTX_DUR_BURST_SIFS_G_SFT;
				regval = regval & MTX_DUR_SLOT_G_I_MSK;
				regval |= 20 << MTX_DUR_SLOT_G_SFT;
				SMAC_REG_WRITE(sc->sh, ADR_MTX_DUR_SIFS_G,
					       regval);
			}
		}
	}
	if (changed & BSS_CHANGED_HT) {
		dev_dbg(sc->dev, "BSS_CHANGED_HT: Untreated!!\n");
	}
	if (changed & BSS_CHANGED_BASIC_RATES) {
		dev_dbg(sc->dev, "ssv6xxx_rc_update_basic_rate!!\n");
		ssv6xxx_rc_update_basic_rate(sc, info->basic_rates);
	}
	if (vif->type == NL80211_IFTYPE_STATION) {
		dev_dbg(sc->dev, "NL80211_IFTYPE_STATION!!\n");
		if ((changed & BSS_CHANGED_ASSOC) && (vif->p2p == 0)) {
			sc->isAssoc = vif->cfg.assoc;
			if (!sc->isAssoc) {
				sc->channel_center_freq = 0;
				sc->ps_aid = 0;
#ifdef CONFIG_SSV_MRX_EN3_CTRL
				SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN3, 0x0400);
#endif
				SMAC_REG_WRITE(sc->sh, ADR_RX_11B_CCA_CONTROL,
					       0x0);
			} else {
				struct ieee80211_channel *curchan;
				curchan = hw->conf.chandef.chan;
				sc->channel_center_freq = curchan->center_freq;
				dev_dbg(sc->dev, "info->aid = %d\n", vif->cfg.aid);
				sc->ps_aid = vif->cfg.aid;
#ifdef CONFIG_SSV_MRX_EN3_CTRL
				SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN3, 0x1000);
#endif
			}
		}
#ifdef CONFIG_SSV_MRX_EN3_CTRL
		else if ((changed & BSS_CHANGED_ASSOC) && vif->p2p == 1) {
			if (info->assoc)
				SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN3, 0x0400);
			else if (sc->ps_aid != 0)
				SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_EN3, 0x1000);
		}
#endif
	}
	if (vif->type == NL80211_IFTYPE_AP) {
		if (changed & (BSS_CHANGED_BEACON
			       | BSS_CHANGED_SSID
			       | BSS_CHANGED_BSSID | BSS_CHANGED_BASIC_RATES)) {
#ifdef BROADCAST_DEBUG
			dev_dbg(sc->dev, "[A] ssv6200_bss_info_changed:beacon changed\n");
#endif
			queue_work(sc->config_wq, &sc->set_tim_work);
		}
		if (changed & BSS_CHANGED_BEACON_INT) {
			dev_dbg(sc->dev, "[A] BSS_CHANGED_BEACON_INT beacon_interval(%d)\n",
			     info->beacon_int);
			if (sc->beacon_interval != info->beacon_int) {
				sc->beacon_interval = info->beacon_int;
				ssv6xxx_beacon_set_info(sc, sc->beacon_interval,
							sc->beacon_dtim_cnt);
			}
		}
		if (changed & BSS_CHANGED_BEACON_ENABLED) {
#ifdef BEACON_DEBUG
			dev_dbg(sc->dev, "[A] BSS_CHANGED_BEACON_ENABLED (0x%x)\n",
			       info->enable_beacon);
#endif
			if (0 != ssv6xxx_beacon_enable(sc, info->enable_beacon)) {
				dev_err(sc->dev, "Beacon enable %d error.\n",
					info->enable_beacon);
			}
		}
	}
	mutex_unlock(&sc->mutex);
	dev_dbg(sc->dev, "[I] %s(): leave\n", __FUNCTION__);
}

static int ssv6200_sta_add(struct ieee80211_hw *hw,
			   struct ieee80211_vif *vif, struct ieee80211_sta *sta)
{
	struct ssv_sta_priv_data *sta_priv_dat = NULL;
	struct ssv_softc *sc = hw->priv;
	struct ssv_sta_info *sta_info;
	u32 reg_wsid[] = { ADR_WSID0, ADR_WSID1 };
	int s, i;
	u32 reg_wsid_tid0[] = { ADR_WSID0_TID0_RX_SEQ, ADR_WSID1_TID0_RX_SEQ };
	u32 reg_wsid_tid7[] = { ADR_WSID0_TID7_RX_SEQ, ADR_WSID1_TID7_RX_SEQ };
	unsigned long flags;
	int ret = 0;
	struct ssv_vif_priv_data *vif_priv =
	    (struct ssv_vif_priv_data *)vif->drv_priv;
	int fw_sec_caps = SSV6XXX_WSID_SEC_NONE;
	bool tdls_use_sw_cipher = false, tdls_link = false;
	dev_dbg(sc->dev, "[I] %s(): vif[%d] ", __FUNCTION__, vif_priv->vif_idx);
	if (sc->force_triger_reset == true) {
		vif_priv->sta_asleep_mask = 0;
		do {
			spin_lock_irqsave(&sc->ps_state_lock, flags);
			for (s = 0; s < SSV_NUM_STA; s++, sta_info++) {
				sta_info = &sc->sta_info[s];
				if ((sta_info->s_flags & STA_FLAG_VALID)) {
					if (sta_info->sta == sta) {
						dev_dbg
						    (sc->dev, "search stat %02x:%02x:%02x:%02x:%02x:%02x to  wsid=%d\n",
						     sta->addr[0], sta->addr[1],
						     sta->addr[2], sta->addr[3],
						     sta->addr[4], sta->addr[5],
						     sta_info->hw_wsid);
						spin_unlock_irqrestore(&sc->
								       ps_state_lock,
								       flags);
						return ret;
					}
				}
			}
			spin_unlock_irqrestore(&sc->ps_state_lock, flags);
			if (s >= SSV_NUM_STA) {
				break;
			}
		} while (0);
	}
	do {
		spin_lock_irqsave(&sc->ps_state_lock, flags);
		if (!list_empty(&vif_priv->sta_list)
		    && vif->type == NL80211_IFTYPE_STATION) {
			tdls_link = true;
		}
		if ((tdls_link) && (vif_priv->pair_cipher != SSV_CIPHER_NONE)
		    && (vif_priv->pair_cipher != SSV_CIPHER_CCMP)
		    && (sc->sh->cfg.use_wpa2_only == false)) {
			tdls_use_sw_cipher = true;
		}
		if (((vif_priv->vif_idx == 0) && (tdls_use_sw_cipher == false))
		    || sc->sh->cfg.use_wpa2_only)
			s = 0;
		else
			s = 2;
		for (; s < SSV_NUM_STA; s++) {
			sta_info = &sc->sta_info[s];
			if ((sta_info->s_flags & STA_FLAG_VALID) == 0) {
				sta_info->aid = sta->aid;
				sta_info->sta = sta;
				sta_info->vif = vif;
				sta_info->s_flags = STA_FLAG_VALID;
				sta_priv_dat =
				    (struct ssv_sta_priv_data *)sta->drv_priv;
				sta_priv_dat->sta_idx = s;
				sta_priv_dat->sta_info = sta_info;
				sta_priv_dat->has_hw_encrypt = false;
				sta_priv_dat->has_hw_decrypt = false;
				sta_priv_dat->need_sw_decrypt = false;
				sta_priv_dat->need_sw_encrypt = false;
				sta_priv_dat->use_mac80211_decrypt = false;
				if ((vif_priv->pair_cipher == SSV_CIPHER_WEP40)
				    || (vif_priv->pair_cipher ==
					SSV_CIPHER_WEP104)) {
					sta_priv_dat->has_hw_encrypt =
					    vif_priv->has_hw_encrypt;
					sta_priv_dat->has_hw_decrypt =
					    vif_priv->has_hw_decrypt;
					sta_priv_dat->need_sw_encrypt =
					    vif_priv->need_sw_encrypt;
					sta_priv_dat->need_sw_decrypt =
					    vif_priv->need_sw_decrypt;
				}
				list_add_tail(&sta_priv_dat->list,
					      &vif_priv->sta_list);
				break;
			}
		}
		spin_unlock_irqrestore(&sc->ps_state_lock, flags);
		if (s >= SSV_NUM_STA) {
			dev_err(sc->dev,
				"Number of STA exceeds driver limitation %d\n.",
				SSV_NUM_STA);
			ret = -1;
			break;
		}
#ifdef CONFIG_SSV6XXX_DEBUGFS
		ssv6xxx_debugfs_add_sta(sc, sta_info);
#endif
		sta_info->hw_wsid = -1;
		if (sta_priv_dat->sta_idx < SSV_NUM_HW_STA) {
			SMAC_REG_WRITE(sc->sh, reg_wsid[s] + 4,
				       *((u32 *) & sta->addr[0]));
			SMAC_REG_WRITE(sc->sh, reg_wsid[s] + 8,
				       *((u32 *) & sta->addr[4]));
			SMAC_REG_WRITE(sc->sh, reg_wsid[s], 1);
			for (i = reg_wsid_tid0[s]; i <= reg_wsid_tid7[s];
			     i += 4)
				SMAC_REG_WRITE(sc->sh, i, 0);
			ssv6xxx_rc_hw_reset(sc, sta_priv_dat->rc_idx, s);
			sta_info->hw_wsid = sta_priv_dat->sta_idx;
		} else if ((vif_priv->vif_idx == 0)
			   || sc->sh->cfg.use_wpa2_only) {
			sta_info->hw_wsid = sta_priv_dat->sta_idx;
		}
		if ((sta_priv_dat->has_hw_encrypt
		     || sta_priv_dat->has_hw_decrypt)
		    && ((vif_priv->pair_cipher == SSV_CIPHER_WEP40)
			|| (vif_priv->pair_cipher == SSV_CIPHER_WEP104))) {
			struct ssv_vif_info *vif_info =
			    &sc->vif_info[vif_priv->vif_idx];
			struct ssv6xxx_hw_sec *sramKey = &vif_info->sramKey;
			_set_wep_hw_crypto_pair_key(sc, vif_info, sta_info,
						    (void *)sramKey);
			if (sramKey->sta_key[0].pair_key_idx != 0) {
				_set_wep_hw_crypto_group_key(sc, vif_info,
							     sta_info,
							     (void *)sramKey);
			}
		}
		ssv6200_ampdu_tx_add_sta(hw, sta);
		if (sta_info->hw_wsid >= SSV_NUM_HW_STA) {
			if (sta_priv_dat->has_hw_decrypt)
				fw_sec_caps = SSV6XXX_WSID_SEC_PAIRWISE;
			if (vif_priv->has_hw_decrypt)
				fw_sec_caps |= SSV6XXX_WSID_SEC_GROUP;
			hw_update_watch_wsid(sc, sta, sta_info,
					     sta_priv_dat->sta_idx, fw_sec_caps,
					     SSV6XXX_WSID_OPS_ADD);
		} else if (SSV6200_USE_HW_WSID(sta_priv_dat->sta_idx)) {
			hw_update_watch_wsid(sc, sta, sta_info,
					     sta_priv_dat->sta_idx,
					     SSV6XXX_WSID_SEC_SW,
					     SSV6XXX_WSID_OPS_HWWSID_PAIRWISE_SET_TYPE);
			hw_update_watch_wsid(sc, sta, sta_info,
					     sta_priv_dat->sta_idx,
					     SSV6XXX_WSID_SEC_SW,
					     SSV6XXX_WSID_OPS_HWWSID_GROUP_SET_TYPE);
		}
		dev_dbg
		    (sc->dev, "Add %02x:%02x:%02x:%02x:%02x:%02x to VIF %d sw_idx=%d, wsid=%d\n",
		     sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
		     sta->addr[4], sta->addr[5], vif_priv->vif_idx,
		     sta_priv_dat->sta_idx, sta_info->hw_wsid);
	} while (0);
	return ret;
}

void ssv6200_rx_flow_check(struct ssv_sta_priv_data *sta_priv_dat,
			   struct ssv_softc *sc)
{
	if (SSV6200_USE_HW_WSID(sta_priv_dat->sta_idx)
	    && (sta_priv_dat->need_sw_decrypt)) {
		int other_hw_wsid = (sta_priv_dat->sta_idx + 1) & 1;
		struct ssv_sta_info *sta_info = &sc->sta_info[other_hw_wsid];
		struct ieee80211_sta *sta = sta_info->sta;
		struct ssv_sta_priv_data *sta_priv =
		    (struct ssv_sta_priv_data *)sta->drv_priv;
		mutex_lock(&sc->mutex);
		if ((sta_info->s_flags == 0)
		    || ((sta_info->s_flags && STA_FLAG_VALID)
			&& (sta_priv->has_hw_decrypt))) {
			SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_DATA,
				       M_ENG_MACRX | (M_ENG_ENCRYPT_SEC << 4) |
				       (M_ENG_HWHCI << 8));
			dev_dbg(sc->dev, "redirect Rx flow for sta %d disconnect\n",
			       sta_priv_dat->sta_idx);
		}
		mutex_unlock(&sc->mutex);
	}
}

static int ssv6200_sta_remove(struct ieee80211_hw *hw,
			      struct ieee80211_vif *vif,
			      struct ieee80211_sta *sta)
{
	u32 reg_wsid[] = { ADR_WSID0, ADR_WSID1 };
	struct ssv_sta_priv_data *sta_priv_dat =
	    (struct ssv_sta_priv_data *)sta->drv_priv;
	struct ssv_softc *sc = hw->priv;
	struct ssv_sta_info *sta_info = sta_priv_dat->sta_info;
	unsigned long flags;
	u32 bit;
	struct ssv_vif_priv_data *priv_vif =
	    (struct ssv_vif_priv_data *)vif->drv_priv;
	u8 hw_wsid = -1;
	BUG_ON(sta_priv_dat->sta_idx >= SSV_NUM_STA);
	dev_notice(sc->dev,
		   "Removing STA %d (%02X:%02X:%02X:%02X:%02X:%02X) from VIF %d\n.",
		   sta_priv_dat->sta_idx, sta->addr[0], sta->addr[1],
		   sta->addr[2], sta->addr[3], sta->addr[4], sta->addr[5],
		   priv_vif->vif_idx);
	ssv6200_rx_flow_check(sta_priv_dat, sc);
	spin_lock_irqsave(&sc->ps_state_lock, flags);
	bit = BIT(sta_priv_dat->sta_idx);
	priv_vif->sta_asleep_mask &= ~bit;
	if (sta_info->hw_wsid != -1) {
		hw_wsid = sta_info->hw_wsid;
	}
	if (sta_info->hw_wsid >= SSV_NUM_HW_STA) {
		spin_unlock_irqrestore(&sc->ps_state_lock, flags);
		hw_update_watch_wsid(sc, sta, sta_info, sta_info->hw_wsid, 0,
				     SSV6XXX_WSID_OPS_DEL);
		spin_lock_irqsave(&sc->ps_state_lock, flags);
	}
#ifdef CONFIG_SSV6XXX_DEBUGFS
	{
		ssv6xxx_debugfs_remove_sta(sc, sta_info);
	}
#endif
	memset(sta_info, 0, sizeof(*sta_info));
	sta_priv_dat->sta_idx = -1;
	list_del(&sta_priv_dat->list);
	if (list_empty(&priv_vif->sta_list)
	    && vif->type == NL80211_IFTYPE_STATION) {
		priv_vif->pair_cipher = 0;
		priv_vif->group_cipher = 0;
	}
	spin_unlock_irqrestore(&sc->ps_state_lock, flags);
	if ((hw_wsid != -1) && (hw_wsid < SSV_NUM_HW_STA))
		SMAC_REG_WRITE(sc->sh, reg_wsid[hw_wsid], 0x00);
	return 0;
}

static void ssv6200_sta_notify(struct ieee80211_hw *hw,
			       struct ieee80211_vif *vif,
			       enum sta_notify_cmd cmd,
			       struct ieee80211_sta *sta)
{
	struct ssv_softc *sc = hw->priv;
	struct ssv_vif_priv_data *priv_vif =
	    (struct ssv_vif_priv_data *)vif->drv_priv;
	struct ssv_sta_priv_data *sta_priv_dat =
	    sta != NULL ? (struct ssv_sta_priv_data *)sta->drv_priv : NULL;
	struct ssv_sta_info *sta_info;
	u32 bit, prev;
	unsigned long flags;
	spin_lock_irqsave(&sc->ps_state_lock, flags);
	if (sta_priv_dat != NULL) {
		bit = BIT(sta_priv_dat->sta_idx);
		prev = priv_vif->sta_asleep_mask & bit;
		sta_info = sta_priv_dat->sta_info;
		switch (cmd) {
		case STA_NOTIFY_SLEEP:
			if (!prev) {
				sta_info->sleeping = true;
				if ((vif->type == NL80211_IFTYPE_AP)
				    && sc->bq4_dtim
				    && !priv_vif->sta_asleep_mask
				    && ssv6200_bcast_queue_len(&sc->
							       bcast_txq)) {
					dev_dbg(sc->dev, "%s(): ssv6200_bcast_start\n", __FUNCTION__);
					ssv6200_bcast_start(sc);
				}
				priv_vif->sta_asleep_mask |= bit;
			}
			break;
		case STA_NOTIFY_AWAKE:
			if (prev) {
				sta_info->sleeping = false;
				priv_vif->sta_asleep_mask &= ~bit;
			}
			break;
		default:
			break;
		}
	}
	spin_unlock_irqrestore(&sc->ps_state_lock, flags);
}

static u64 ssv6200_get_tsf(struct ieee80211_hw *hw, struct ieee80211_vif *vif)
{
	return jiffies * 1000 * 1000 / HZ;
}

static u64 ssv6200_get_systime_us(void)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,19,0)
	struct timespec64 ts;
	ktime_get_boottime_ts64(&ts);
#else
	struct timespec ts;
	get_monotonic_boottime(&ts);
#endif
	return ((u64) ts.tv_sec * 1000000) + ts.tv_nsec / 1000;
}

static u32 pre_11b_cca_control;
static u32 pre_11b_cca_1;
static void ssv6200_sw_scan_start(struct ieee80211_hw *hw,
				  struct ieee80211_vif *vif,
				  const u8 * mac_addr)
{
	((struct ssv_softc *)(hw->priv))->bScanning = true;
	SMAC_REG_READ(((struct ssv_softc *)(hw->priv))->sh,
		      ADR_RX_11B_CCA_CONTROL, &pre_11b_cca_control);
	SMAC_REG_WRITE(((struct ssv_softc *)(hw->priv))->sh,
		       ADR_RX_11B_CCA_CONTROL, 0x0);
	SMAC_REG_READ(((struct ssv_softc *)(hw->priv))->sh, ADR_RX_11B_CCA_1,
		      &pre_11b_cca_1);
	SMAC_REG_WRITE(((struct ssv_softc *)(hw->priv))->sh, ADR_RX_11B_CCA_1,
		       RX_11B_CCA_IN_SCAN);
#ifdef CONFIG_SSV_MRX_EN3_CTRL
	SMAC_REG_WRITE(((struct ssv_softc *)(hw->priv))->sh, ADR_MRX_FLT_EN3,
		       0x0400);
#endif
}

static void ssv6200_sw_scan_complete(struct ieee80211_hw *hw,
				     struct ieee80211_vif *vif)
{

#ifdef CONFIG_SSV_MRX_EN3_CTRL
	bool is_p2p_assoc;
#endif
	((struct ssv_softc *)(hw->priv))->bScanning = false;
	SMAC_REG_WRITE(((struct ssv_softc *)(hw->priv))->sh,
		       ADR_RX_11B_CCA_CONTROL, pre_11b_cca_control);
	SMAC_REG_WRITE(((struct ssv_softc *)(hw->priv))->sh, ADR_RX_11B_CCA_1,
		       pre_11b_cca_1);
#ifdef CONFIG_SSV_MRX_EN3_CTRL
	is_p2p_assoc =
	    ((struct ssv_softc *)(hw->priv))->vif_info[1].vif->bss_conf.assoc;
	if (((struct ssv_softc *)(hw->priv))->ps_aid != 0 && (!is_p2p_assoc))
		SMAC_REG_WRITE(((struct ssv_softc *)(hw->priv))->sh,
			       ADR_MRX_FLT_EN3, 0x1000);
#endif
}

static int ssv6200_set_tim(struct ieee80211_hw *hw, struct ieee80211_sta *sta,
			   bool set)
{
	struct ssv_softc *sc = hw->priv;
	struct ssv_sta_info *sta_info = sta
	    ? ((struct ssv_sta_priv_data *)sta->drv_priv)->sta_info : NULL;
	if (sta_info && (sta_info->tim_set ^ set)) {
        dev_dbg(sc->dev, "[I] [A] ssvcabrio_set_tim");
		sta_info->tim_set = set;
		queue_work(sc->config_wq, &sc->set_tim_work);
	}
	return 0;
}

static int ssv6200_conf_tx(struct ieee80211_hw *hw,
			   struct ieee80211_vif *vif, u32 link_id, u16 queue,
			   const struct ieee80211_tx_queue_params *params)
{
	struct ssv_softc *sc = hw->priv;
	u32 cw;
	u8 hw_txqid = sc->tx.hw_txqid[queue];
	struct ssv_vif_priv_data *priv_vif =
	    (struct ssv_vif_priv_data *)vif->drv_priv;
	dev_dbg
	    (sc->dev, "[I] sv6200_conf_tx vif[%d] qos[%d] queue[%d] aifsn[%d] cwmin[%d] cwmax[%d] txop[%d] \n",
	     priv_vif->vif_idx, vif->bss_conf.qos, queue, params->aifs,
	     params->cw_min, params->cw_max, params->txop);
	if (queue > NL80211_TXQ_Q_BK)
		return 1;
	if (priv_vif->vif_idx != 0) {
		dev_warn(sc->dev,
			 "WMM setting applicable to primary interface only.\n");
		return 1;
	}
	mutex_lock(&sc->mutex);
	SMAC_REG_SET_BITS(sc->sh, ADR_GLBLE_SET,
			  (vif->bss_conf.qos << QOS_EN_SFT), QOS_EN_MSK);
	cw = (params->aifs - 1) & 0xf;
	cw |= ((ilog2(params->cw_min + 1)) & 0xf) << TXQ1_MTX_Q_ECWMIN_SFT;
	cw |= ((ilog2(params->cw_max + 1)) & 0xf) << TXQ1_MTX_Q_ECWMAX_SFT;
	cw |= ((params->txop) & 0xff) << TXQ1_MTX_Q_TXOP_LIMIT_SFT;
	SMAC_REG_WRITE(sc->sh, ADR_TXQ0_MTX_Q_AIFSN + 0x100 * hw_txqid, cw);
	mutex_unlock(&sc->mutex);
	return 0;
}

static int ssv6200_ampdu_action(struct ieee80211_hw *hw,
				struct ieee80211_vif *vif,
				struct ieee80211_ampdu_params *params)
{
	struct ssv_softc *sc = hw->priv;
	int ret = 0;
	struct ieee80211_sta *sta = params->sta;
	enum ieee80211_ampdu_mlme_action action = params->action;
	u16 tid = params->tid;
	u16 *ssn = &(params->ssn);
	u8 buf_size = params->buf_size;
	if (sta == NULL)
		return ret;
#if (!Enable_AMPDU_Rx)
	if (action == IEEE80211_AMPDU_RX_START
	    || action == IEEE80211_AMPDU_RX_STOP) {
		ampdu_db_log("Disable AMPDU_RX for test(1).\n");
		return -EOPNOTSUPP;
	}
#endif
#if (!Enable_AMPDU_Tx)
	if (action == IEEE80211_AMPDU_TX_START
	    || action == IEEE80211_AMPDU_TX_STOP
	    || action == IEEE80211_AMPDU_TX_OPERATIONAL) {
		ampdu_db_log("Disable AMPDU_TX for test(1).\n");
		return -EOPNOTSUPP;
	}
#endif
	if ((action == IEEE80211_AMPDU_RX_START
	     || action == IEEE80211_AMPDU_RX_STOP)
	    && (!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_RX))) {
		ampdu_db_log("Disable AMPDU_RX(2).\n");
		return -EOPNOTSUPP;
	}
	if ((action == IEEE80211_AMPDU_TX_START
	     || action == IEEE80211_AMPDU_TX_STOP_CONT
	     || action == IEEE80211_AMPDU_TX_STOP_FLUSH
	     || action == IEEE80211_AMPDU_TX_STOP_FLUSH_CONT
	     || action == IEEE80211_AMPDU_TX_OPERATIONAL)
	    && (!(sc->sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_TX))) {
		ampdu_db_log("Disable AMPDU_TX(2).\n");
		return -EOPNOTSUPP;
	}
	switch (action) {
	case IEEE80211_AMPDU_RX_START:
#ifdef WIFI_CERTIFIED
		if (sc->rx_ba_session_count >= SSV6200_RX_BA_MAX_SESSIONS) {
			ieee80211_stop_rx_ba_session(vif,
						     (1 << (sc->ba_tid)),
						     sc->ba_ra_addr);
			sc->rx_ba_session_count--;
		}
#else
		if ((sc->rx_ba_session_count >= SSV6200_RX_BA_MAX_SESSIONS)
		    && (sc->rx_ba_sta != sta)) {
			ret = -EBUSY;
			break;
		} else
		    if ((sc->rx_ba_session_count >= SSV6200_RX_BA_MAX_SESSIONS)
			&& (sc->rx_ba_sta == sta)) {
			ieee80211_stop_rx_ba_session(vif, (1 << (sc->ba_tid)),
						     sc->ba_ra_addr);
			sc->rx_ba_session_count--;
		}
#endif
		dev_dbg(sc->dev, "IEEE80211_AMPDU_RX_START %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
		       sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
		       sta->addr[4], sta->addr[5], tid);
		sc->rx_ba_session_count++;
		sc->rx_ba_sta = sta;
		sc->ba_tid = tid;
		sc->ba_ssn = *ssn;
		memcpy(sc->ba_ra_addr, sta->addr, ETH_ALEN);
		queue_work(sc->config_wq, &sc->set_ampdu_rx_add_work);
		break;
	case IEEE80211_AMPDU_RX_STOP:
		sc->rx_ba_session_count--;
		if (sc->rx_ba_session_count == 0)
			sc->rx_ba_sta = NULL;
		queue_work(sc->config_wq, &sc->set_ampdu_rx_del_work);
		break;
	case IEEE80211_AMPDU_TX_START:
		dev_dbg(sc->dev, "AMPDU_TX_START %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
		       sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
		       sta->addr[4], sta->addr[5], tid);
		ssv6200_ampdu_tx_start(tid, sta, hw, ssn);
		ieee80211_start_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
	case IEEE80211_AMPDU_TX_STOP_CONT:
	case IEEE80211_AMPDU_TX_STOP_FLUSH:
	case IEEE80211_AMPDU_TX_STOP_FLUSH_CONT:
		dev_dbg(sc->dev, "AMPDU_TX_STOP %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
		       sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
		       sta->addr[4], sta->addr[5], tid);
		ssv6200_ampdu_tx_stop(tid, sta, hw);
		ieee80211_stop_tx_ba_cb_irqsafe(vif, sta->addr, tid);
		break;
	case IEEE80211_AMPDU_TX_OPERATIONAL:
		dev_dbg(sc->dev, "AMPDU_TX_OPERATIONAL %02X:%02X:%02X:%02X:%02X:%02X %d.\n",
		       sta->addr[0], sta->addr[1], sta->addr[2], sta->addr[3],
		       sta->addr[4], sta->addr[5], tid);
		ssv6200_ampdu_tx_operation(tid, sta, hw, buf_size);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}
	return ret;
}

#ifdef CONFIG_PM
int ssv6xxx_suspend(struct ieee80211_hw *hw, struct cfg80211_wowlan *wowlan)
{
	return 0;
}

int ssv6xxx_resume(struct ieee80211_hw *hw)
{
	return 0;
}
#endif
struct ieee80211_ops ssv6200_ops = {
	.tx = ssv6200_tx,
	.start = ssv6200_start,
	.stop = ssv6200_stop,
	.add_interface = ssv6200_add_interface,
	.remove_interface = ssv6200_remove_interface,
	.change_interface = ssv6200_change_interface,
	.config = ssv6200_config,
	.configure_filter = ssv6200_config_filter,
	.bss_info_changed = ssv6200_bss_info_changed,
	.sta_add = ssv6200_sta_add,
	.sta_remove = ssv6200_sta_remove,
	.sta_notify = ssv6200_sta_notify,
	.set_key = ssv6200_set_key,
	.sw_scan_start = ssv6200_sw_scan_start,
	.sw_scan_complete = ssv6200_sw_scan_complete,
	.get_tsf = ssv6200_get_tsf,
	.set_tim = ssv6200_set_tim,
	.conf_tx = ssv6200_conf_tx,
	.ampdu_action = ssv6200_ampdu_action,
#ifdef CONFIG_PM
	.suspend = ssv6xxx_suspend,
	.resume = ssv6xxx_resume,
#endif
};

int ssv6200_tx_flow_control(void *dev, int hw_txqid, bool fc_en, int debug)
{
	struct ssv_softc *sc = dev;
	int ac;
	BUG_ON(hw_txqid > 4);
	if (hw_txqid == 4)
		return 0;
	ac = sc->tx.ac_txqid[hw_txqid];
	if (fc_en == false) {
		if (sc->tx.flow_ctrl_status & (1 << ac)) {
			ieee80211_wake_queue(sc->hw, ac);
			sc->tx.flow_ctrl_status &= ~(1 << ac);
		} else {
		}
	} else {
		if ((sc->tx.flow_ctrl_status & (1 << ac)) == 0) {
			ieee80211_stop_queue(sc->hw, ac);
			sc->tx.flow_ctrl_status |= (1 << ac);
		} else {
		}
	}
	return 0;
}

void ssv6xxx_tx_q_empty_cb(u32 txq_no, void *cb_data)
{
	struct ssv_softc *sc = cb_data;
	BUG_ON(sc == NULL);
	sc->tx_q_empty = true;
	smp_mb();
	wake_up_interruptible(&sc->tx_wait_q);
}

struct ssv6xxx_b_cca_control {
	u32 down_level;
	u32 upper_level;
	u32 adjust_cca_control;
	u32 adjust_cca_1;
};
struct ssv6xxx_b_cca_control adjust_cci[] = {
	{0, 43, 0x00162000, 0x20380050},
	{40, 48, 0x00161000, 0x20380050},
	{45, 53, 0x00160800, 0x20380050},
	{50, 63, 0x00160400, 0x20380050},
	{60, 68, 0x00160200, 0x20380050},
	{65, 73, 0x00160100, 0x20380050},
	{70, 128, 0x00000000, 0x20300050},
};

#define MAX_CCI_LEVEL 128
static unsigned long last_jiffies = INITIAL_JIFFIES;
static s32 size = sizeof(adjust_cci) / sizeof(adjust_cci[0]);
static u32 current_level = MAX_CCI_LEVEL;
static u32 current_gate = (sizeof(adjust_cci) / sizeof(adjust_cci[0])) - 1;
void mitigate_cci(struct ssv_softc *sc, u32 input_level)
{
	s32 i;
	if (input_level > MAX_CCI_LEVEL) {
		dev_dbg(sc->dev, "mitigate_cci input error[%d]!!\n", input_level);
		return;
	}
	if (time_after(jiffies, last_jiffies + msecs_to_jiffies(3000))) {
		dev_dbg(sc->dev, "jiffies=%lu, input_level=%d\n", jiffies, input_level);
		last_jiffies = jiffies;
		if ((input_level >= adjust_cci[current_gate].down_level)
		    && (input_level <= adjust_cci[current_gate].upper_level)) {
			current_level = input_level;
#ifdef DEBUG_MITIGATE_CCI
			dev_dbg(sc->dev, "Keep the 0xce0020a0[%x] 0xce002008[%x]!!\n",
			       adjust_cci[current_gate].adjust_cca_control,
			       adjust_cci[current_gate].adjust_cca_1);
#endif
		} else {
			if (current_level < input_level) {
				for (i = 0; i < size; i++) {
					if (input_level <=
					    adjust_cci[i].upper_level) {
#ifdef DEBUG_MITIGATE_CCI
						dev_dbg(sc->dev, "gate=%d, input_level=%d, adjust_cci[%d].upper_level=%d, value=%08x\n",
						     current_gate, input_level,
						     i,
						     adjust_cci[i].upper_level,
						     adjust_cci[i].
						     adjust_cca_control);
#endif
						current_level = input_level;
						current_gate = i;
						SMAC_REG_WRITE(sc->sh,
							       ADR_RX_11B_CCA_CONTROL,
							       adjust_cci[i].
							       adjust_cca_control);
						SMAC_REG_WRITE(sc->sh,
							       ADR_RX_11B_CCA_1,
							       adjust_cci[i].
							       adjust_cca_1);
#ifdef DEBUG_MITIGATE_CCI
						dev_dbg(sc->dev, "##Set to the 0xce0020a0[%x] 0xce002008[%x]##!!\n",
						     adjust_cci[current_gate].
						     adjust_cca_control,
						     adjust_cci[current_gate].
						     adjust_cca_1);
#endif
						return;
					}
				}
			} else {
				for (i = (size - 1); i >= 0; i--) {
					if (input_level >=
					    adjust_cci[i].down_level) {
#ifdef DEBUG_MITIGATE_CCI
						dev_dbg(sc->dev, "gate=%d, input_level=%d, adjust_cci[%d].down_level=%d, value=%08x\n",
						     current_gate, input_level,
						     i,
						     adjust_cci[i].down_level,
						     adjust_cci[i].
						     adjust_cca_control);
#endif
						current_level = input_level;
						current_gate = i;
						SMAC_REG_WRITE(sc->sh,
							       ADR_RX_11B_CCA_CONTROL,
							       adjust_cci[i].
							       adjust_cca_control);
						SMAC_REG_WRITE(sc->sh,
							       ADR_RX_11B_CCA_1,
							       adjust_cci[i].
							       adjust_cca_1);
#ifdef DEBUG_MITIGATE_CCI
						dev_dbg(sc->dev, "##Set to the 0xce0020a0[%x] 0xce002008[%x]##!!\n",
						     adjust_cci[current_gate].
						     adjust_cca_control,
						     adjust_cci[current_gate].
						     adjust_cca_1);
#endif
						return;
					}
				}
			}
		}
	}
}

#define RSSI_SMOOTHING_SHIFT 5
#define RSSI_DECIMAL_POINT_SHIFT 6
static void _proc_data_rx_skb(struct ssv_softc *sc, struct sk_buff *rx_skb)
{
	struct ieee80211_rx_status *rxs;
	struct ieee80211_hdr *hdr;
	__le16 fc;
	struct ssv6200_rx_desc *rxdesc;
	struct ssv6200_rxphy_info_padding *rxphypad;
	struct ssv6200_rxphy_info *rxphy;
	struct ieee80211_channel *chan;
	struct ieee80211_vif *vif = NULL;
	struct ieee80211_sta *sta = NULL;
	bool rx_hw_dec = false;
	bool do_sw_dec = false;
	struct ssv_sta_priv_data *sta_priv = NULL;
	struct ssv_vif_priv_data *vif_priv = NULL;
	SKB_info *skb_info = NULL;
	u8 is_beacon;
	u8 is_probe_resp;
	s32 found = 0;
#ifdef CONFIG_SSV_SMARTLINK
	{
		extern int ksmartlink_smartlink_started(void);
		void smartlink_nl_send_msg(struct sk_buff *skb);
		if (unlikely(ksmartlink_smartlink_started())) {
			skb_pull(rx_skb, SSV6XXX_RX_DESC_LEN);
			skb_trim(rx_skb, rx_skb->len - sc->sh->rx_pinfo_pad);
			smartlink_nl_send_msg(rx_skb);
			return;
		}
	}
#endif
	rxdesc = (struct ssv6200_rx_desc *)rx_skb->data;
	rxphy = (struct ssv6200_rxphy_info *)(rx_skb->data + sizeof(*rxdesc));
	rxphypad =
	    (struct ssv6200_rxphy_info_padding *)(rx_skb->data + rx_skb->len -
						  sizeof(struct
							 ssv6200_rxphy_info_padding));
	hdr = (struct ieee80211_hdr *)(rx_skb->data + SSV6XXX_RX_DESC_LEN);
	fc = hdr->frame_control;
	skb_info = (SKB_info *) rx_skb->head;
	if (rxdesc->wsid >= SSV_RC_MAX_HARDWARE_SUPPORT) {
		if ((ieee80211_is_data(hdr->frame_control))
		    && (!(ieee80211_is_nullfunc(hdr->frame_control)))) {
			ssv6xxx_rc_rx_data_handler(sc->hw, rx_skb,
						   rxdesc->rate_idx);
		}
	}
	rxs = IEEE80211_SKB_RXCB(rx_skb);
	memset(rxs, 0, sizeof(struct ieee80211_rx_status));
	ssv6xxx_rc_mac8011_rate_idx(sc, rxdesc->rate_idx, rxs);

	rxs->mactime = *((u32 *) & rx_skb->data[28]);
	chan = sc->hw->conf.chandef.chan;
	rxs->band = chan->band;
	rxs->freq = chan->center_freq;
	rxs->antenna = 1;
	is_beacon = ieee80211_is_beacon(hdr->frame_control);
	is_probe_resp = ieee80211_is_probe_resp(hdr->frame_control);
	if (is_beacon)		//+++
	{
		struct ieee80211_mgmt *mgmt_tmp = NULL;
		mgmt_tmp =
		    (struct ieee80211_mgmt *)(rx_skb->data +
					      SSV6XXX_RX_DESC_LEN);
		mgmt_tmp->u.beacon.timestamp =
		    cpu_to_le64(ssv6200_get_systime_us());
	}
	if (is_probe_resp) {
		struct ieee80211_mgmt *mgmt_tmp = NULL;
		mgmt_tmp =
		    (struct ieee80211_mgmt *)(rx_skb->data +
					      SSV6XXX_RX_DESC_LEN);
		mgmt_tmp->u.probe_resp.timestamp =
		    cpu_to_le64(ssv6200_get_systime_us());
	}

	if (rxdesc->rate_idx < SSV62XX_G_RATE_INDEX && rxphypad->RSVD == 0) {
		if (is_beacon || is_probe_resp) {
			sta = ssv6xxx_find_sta_by_rx_skb(sc, rx_skb);
			if (sta) {
				sta_priv =
				    (struct ssv_sta_priv_data *)sta->drv_priv;
#ifdef SSV_RSSI_DEBUG
				dev_dbg(sc->dev, "b_beacon %02X:%02X:%02X:%02X:%02X:%02X rssi=%d, snr=%d\n",
				       hdr->addr2[0], hdr->addr2[1],
				       hdr->addr2[2], hdr->addr2[3],
				       hdr->addr2[4], hdr->addr2[5],
				       rxphypad->rpci, rxphypad->snr);
#endif
				if (sta_priv->beacon_rssi) {
					sta_priv->beacon_rssi =
					    ((rxphypad->
					      rpci << RSSI_DECIMAL_POINT_SHIFT)
					     +
					     ((sta_priv->
					       beacon_rssi <<
					       RSSI_SMOOTHING_SHIFT) -
					      sta_priv->
					      beacon_rssi)) >>
					    RSSI_SMOOTHING_SHIFT;
					rxphypad->rpci =
					    (sta_priv->
					     beacon_rssi >>
					     RSSI_DECIMAL_POINT_SHIFT);
				} else
					sta_priv->beacon_rssi =
					    (rxphypad->
					     rpci << RSSI_DECIMAL_POINT_SHIFT);
#ifdef SSV_RSSI_DEBUG
				dev_dbg(sc->dev, "Beacon smoothing RSSI %d\n", rxphypad->rpci);
#endif
				mitigate_cci(sc, rxphypad->rpci);
			} else {
				mutex_lock(&sc->mutex);
				list_for_each_entry(p_rssi_res,
						    &rssi_res.rssi_list,
						    rssi_list) {
					if (!memcmp
					    (p_rssi_res->bssid, hdr->addr2,
					     ETH_ALEN)) {
						{
							p_rssi_res->rssi =
							    ((rxphypad->
							      rpci <<
							      RSSI_DECIMAL_POINT_SHIFT)
							     +
							     ((p_rssi_res->
							       rssi <<
							       RSSI_SMOOTHING_SHIFT)
							      -
							      p_rssi_res->
							      rssi)) >>
							    RSSI_SMOOTHING_SHIFT;
							rxphypad->rpci =
							    (p_rssi_res->
							     rssi >>
							     RSSI_DECIMAL_POINT_SHIFT);
						}
						p_rssi_res->cache_jiffies =
						    jiffies;
						found = 1;
						break;
					} else {
						if (p_rssi_res->rssi) {
							if (time_after
							    (jiffies,
							     p_rssi_res->
							     cache_jiffies +
							     msecs_to_jiffies
							     (40000))) {
								p_rssi_res->
								    timeout = 1;
							}
						}
					}
				}
				if (!found) {
					p_rssi_res =
					    kmalloc(sizeof(struct rssi_res_st),
						    GFP_KERNEL);
					memcpy(p_rssi_res->bssid, hdr->addr2,
					       ETH_ALEN);
					p_rssi_res->cache_jiffies = jiffies;
					p_rssi_res->rssi =
					    (rxphypad->
					     rpci << RSSI_DECIMAL_POINT_SHIFT);
					p_rssi_res->timeout = 0;
					INIT_LIST_HEAD(&p_rssi_res->rssi_list);
					list_add_tail_rcu(&
							  (p_rssi_res->
							   rssi_list),
							  &(rssi_res.
							    rssi_list));
				}
				mutex_unlock(&sc->mutex);
			}
			if (rxphypad->rpci > 88)
				rxphypad->rpci = 88;
		}
		if (sc->sh->cfg.rssi_ctl) {
			rxs->signal = (-rxphypad->rpci) + sc->sh->cfg.rssi_ctl;
		} else {
			rxs->signal = (-rxphypad->rpci);
		}
	} else if (rxdesc->rate_idx >= SSV62XX_G_RATE_INDEX
		   && rxphy->service == 0) {
		if (is_beacon || is_probe_resp) {
			sta = ssv6xxx_find_sta_by_rx_skb(sc, rx_skb);
			if (sta) {
				sta_priv =
				    (struct ssv_sta_priv_data *)sta->drv_priv;
#ifdef SSV_RSSI_DEBUG
				dev_dbg(sc->dev, "gn_beacon %02X:%02X:%02X:%02X:%02X:%02X rssi=%d, snr=%d\n",
				     hdr->addr2[0], hdr->addr2[1],
				     hdr->addr2[2], hdr->addr2[3],
				     hdr->addr2[4], hdr->addr2[5], rxphy->rpci,
				     rxphy->snr);
#endif
				if (sta_priv->beacon_rssi) {
					sta_priv->beacon_rssi =
					    ((rxphy->
					      rpci << RSSI_DECIMAL_POINT_SHIFT)
					     +
					     ((sta_priv->
					       beacon_rssi <<
					       RSSI_SMOOTHING_SHIFT) -
					      sta_priv->
					      beacon_rssi)) >>
					    RSSI_SMOOTHING_SHIFT;
					rxphy->rpci =
					    (sta_priv->
					     beacon_rssi >>
					     RSSI_DECIMAL_POINT_SHIFT);
				} else
					sta_priv->beacon_rssi =
					    (rxphy->
					     rpci << RSSI_DECIMAL_POINT_SHIFT);
#ifdef SSV_RSSI_DEBUG
				dev_dbg(sc->dev, "Beacon smoothing RSSI %d\n", rxphy->rpci);
#endif
			}
			if (rxphy->rpci > 88)
				rxphy->rpci = 88;
		}
		if (sc->sh->cfg.rssi_ctl) {
			rxs->signal = (-rxphy->rpci) + sc->sh->cfg.rssi_ctl;
		} else {
			rxs->signal = (-rxphy->rpci);
		}
	} else {
#ifdef SSV_RSSI_DEBUG
		dev_dbg(sc->dev, "########unicast: %d, b_rssi/snr: %d/%d, gn_rssi/snr: %d/%d, rate:%d###############\n",
		     rxdesc->unicast, (-rxphy->rpci), rxphy->snr,
		     (-rxphypad->rpci), rxphypad->snr, rxdesc->rate_idx);
		dev_dbg(sc->dev, "RSSI, %d, rate_idx, %d\n", rxs->signal,
		       rxdesc->rate_idx);
		dev_dbg(sc->dev, "rxdesc->RxResult = %x,rxdesc->wsid = %d\n",
		       rxdesc->RxResult, rxdesc->wsid);
#endif
		sta = ssv6xxx_find_sta_by_rx_skb(sc, rx_skb);
		if (sta) {
			sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
			rxs->signal =
			    -(sta_priv->
			      beacon_rssi >> RSSI_DECIMAL_POINT_SHIFT);
		}
#ifdef SSV_RSSI_DEBUG
		dev_dbg(sc->dev, "Others signal %d\n", rxs->signal);
#endif
	}
//    rxs->flag = RX_FLAG_MACTIME_START;          //+++
	rxs->rx_flags = 0;
	if (rxphy->aggregate)
		rxs->flag |= RX_FLAG_NO_SIGNAL_VAL;
	sc->hw_mng_used = rxdesc->mng_used;
	if ((ieee80211_is_data(fc) || ieee80211_is_data_qos(fc))
	    && ieee80211_has_protected(fc)) {
		sta = ssv6xxx_find_sta_by_rx_skb(sc, rx_skb);
		if (sta == NULL)
			goto drop_rx;
		sta_priv = (struct ssv_sta_priv_data *)sta->drv_priv;
		vif = sta_priv->sta_info->vif;
		if (vif == NULL)
			goto drop_rx;
		if (is_broadcast_ether_addr(hdr->addr1) || is_multicast_ether_addr(hdr->addr1)) {
			vif_priv = (struct ssv_vif_priv_data *)vif->drv_priv;
			rx_hw_dec = vif_priv->has_hw_decrypt;
			do_sw_dec = vif_priv->need_sw_decrypt;
		} else {
			rx_hw_dec = sta_priv->has_hw_decrypt;
			do_sw_dec = sta_priv->need_sw_decrypt;
		}
	}
	skb_pull(rx_skb, SSV6XXX_RX_DESC_LEN);
	skb_trim(rx_skb, rx_skb->len - sc->sh->rx_pinfo_pad);
#ifdef CONFIG_P2P_NOA
	if (is_beacon)
		ssv6xxx_noa_detect(sc, hdr, rx_skb->len);
#endif
	if (rx_hw_dec || do_sw_dec) {
		hdr = (struct ieee80211_hdr *)rx_skb->data;
		rxs = IEEE80211_SKB_RXCB(rx_skb);
		hdr->frame_control =
		    hdr->
		    frame_control & ~(cpu_to_le16(IEEE80211_FCTL_PROTECTED));
		rxs->flag |= (RX_FLAG_DECRYPTED | RX_FLAG_IV_STRIPPED);
	}
#if defined(USE_THREAD_RX) && !defined(IRQ_PROC_RX_DATA)
	local_bh_disable();
	ieee80211_rx(sc->hw, rx_skb);
	local_bh_enable();
#else
	ieee80211_rx_irqsafe(sc->hw, rx_skb);
#endif
	return;
 drop_rx:
	dev_kfree_skb_any(rx_skb);
}

#ifdef IRQ_PROC_RX_DATA
static struct sk_buff *_proc_rx_skb(struct ssv_softc *sc,
				    struct sk_buff *rx_skb)
{
	struct ieee80211_hdr *hdr =
	    (struct ieee80211_hdr *)(rx_skb->data + SSV6XXX_RX_DESC_LEN);
	struct ssv6200_rx_desc *rxdesc = (struct ssv6200_rx_desc *)rx_skb->data;
	if (ieee80211_is_back(hdr->frame_control)
	    || (rxdesc->c_type == HOST_EVENT))
		return rx_skb;
	_proc_data_rx_skb(sc, rx_skb);
	return NULL;
}
#endif
void _process_rx_q(struct ssv_softc *sc, struct sk_buff_head *rx_q,
		   spinlock_t * rx_q_lock)
{
	struct sk_buff *skb;
	struct ieee80211_hdr *hdr;
	struct ssv6200_rx_desc *rxdesc;
	unsigned long flags = 0;
#ifdef USE_FLUSH_RETRY
	bool has_ba_processed = false;
#endif
	while (1) {
		if (rx_q_lock != NULL) {
			spin_lock_irqsave(rx_q_lock, flags);
			skb = __skb_dequeue(rx_q);
		} else
			skb = skb_dequeue(rx_q);
		if (!skb) {
			if (rx_q_lock != NULL)
				spin_unlock_irqrestore(rx_q_lock, flags);
			break;
		}
		sc->rx.rxq_count--;
		if (rx_q_lock != NULL)
			spin_unlock_irqrestore(rx_q_lock, flags);
		rxdesc = (struct ssv6200_rx_desc *)skb->data;
		if (rxdesc->c_type == HOST_EVENT) {
			struct cfg_host_event *h_evt =
			    (struct cfg_host_event *)rxdesc;
			if (h_evt->h_event == SOC_EVT_NO_BA) {
				ssv6200_ampdu_no_BA_handler(sc->hw, skb);
#ifdef USE_FLUSH_RETRY
				has_ba_processed = true;
#endif
			} else if (h_evt->h_event == SOC_EVT_RC_MPDU_REPORT) {
				skb_queue_tail(&sc->rc_report_queue, skb);
				if (sc->rc_sample_sechedule == 0)
					queue_work(sc->rc_sample_workqueue,
						   &sc->rc_sample_work);
			} else if (h_evt->h_event == SOC_EVT_SDIO_TEST_COMMAND) {
				if (h_evt->evt_seq_no == 0) {
					dev_dbg(sc->dev, "SOC_EVT_SDIO_TEST_COMMAND\n");
					sc->sdio_rx_evt_size = h_evt->len;
					sc->sdio_throughput_timestamp = jiffies;
				} else {
					sc->sdio_rx_evt_size += h_evt->len;
					if (time_after
					    (jiffies,
					     sc->sdio_throughput_timestamp +
					     msecs_to_jiffies(1000))) {
						dev_dbg(sc->dev, "data[%ld] SDIO RX throughput %ld Kbps\n",
						     sc->sdio_rx_evt_size,
						     (sc->
						      sdio_rx_evt_size << 3) /
						     jiffies_to_msecs(jiffies -
								      sc->
								      sdio_throughput_timestamp));
						sc->sdio_throughput_timestamp =
						    jiffies;
						sc->sdio_rx_evt_size = 0;
					}
				}
				dev_kfree_skb_any(skb);
			} else if (h_evt->h_event == SOC_EVT_WATCHDOG_TRIGGER) {
				dev_kfree_skb_any(skb);
//              if(sc->watchdog_flag != WD_SLEEP)     //+++
				sc->watchdog_flag = WD_KICKED;
			} else if (h_evt->h_event == SOC_EVT_RESET_HOST) {
				dev_kfree_skb_any(skb);
				if ((sc->ap_vif == NULL)
				    || !(sc->sh->cfg.ignore_reset_in_ap)) {
					ssv6xxx_restart_hw(sc);
				} else {
					dev_warn(sc->dev,
						 "Reset event ignored.\n");
				}
			}
#ifdef CONFIG_P2P_NOA
			else if (h_evt->h_event == SOC_EVT_NOA) {
				ssv6xxx_process_noa_event(sc, skb);
				dev_kfree_skb_any(skb);
			}
#endif
			else if (h_evt->h_event == SOC_EVT_SDIO_TXTPUT_RESULT) {
				dev_dbg(sc->dev, "data SDIO TX throughput %d Kbps\n",
				       h_evt->evt_seq_no);
				dev_kfree_skb_any(skb);
			} else if (h_evt->h_event == SOC_EVT_TXLOOPBK_RESULT) {
				if (h_evt->evt_seq_no == SSV6XXX_STATE_OK) {
					dev_dbg(sc->dev, "FW TX LOOPBACK OK\n");
					sc->iq_cali_done = IQ_CALI_OK;
				} else {
					dev_dbg(sc->dev, "FW TX LOOPBACK FAILED\n");
					sc->iq_cali_done = IQ_CALI_FAILED;
				}
				dev_kfree_skb_any(skb);
				wake_up_interruptible(&sc->fw_wait_q);
			} else {
				dev_warn(sc->dev, "Unkown event %d received\n",
					 h_evt->h_event);
				dev_kfree_skb_any(skb);
			}
			continue;
		}
		hdr = (struct ieee80211_hdr *)(skb->data + SSV6XXX_RX_DESC_LEN);
		if (ieee80211_is_back(hdr->frame_control)) {
			ssv6200_ampdu_BA_handler(sc->hw, skb);
#ifdef USE_FLUSH_RETRY
			has_ba_processed = true;
#endif
			continue;
		}
		_proc_data_rx_skb(sc, skb);
	}
#ifdef USE_FLUSH_RETRY
	if (has_ba_processed) {
		ssv6xxx_ampdu_postprocess_BA(sc->hw);
	}
#endif
}

#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
int ssv6200_rx(struct sk_buff_head *rx_skb_q, void *args)
#else
int ssv6200_rx(struct sk_buff *rx_skb, void *args)
#endif
{
	struct ssv_softc *sc = args;
#ifdef IRQ_PROC_RX_DATA
	struct sk_buff *skb;
	skb = _proc_rx_skb(sc, rx_skb);
	if (skb == NULL)
		return 0;
#endif
#if !defined(USE_THREAD_RX) || defined(USE_BATCH_RX)
	{
		unsigned long flags;
		spin_lock_irqsave(&sc->rx_skb_q.lock, flags);
		while (skb_queue_len(rx_skb_q))
			__skb_queue_tail(&sc->rx_skb_q,
					 __skb_dequeue(rx_skb_q));
		spin_unlock_irqrestore(&sc->rx_skb_q.lock, flags);
	}
#else
	skb_queue_tail(&sc->rx_skb_q, rx_skb);
#endif
	wake_up_interruptible(&sc->rx_wait_q);
	return 0;
}

struct ieee80211_sta *ssv6xxx_find_sta_by_rx_skb(struct ssv_softc *sc,
						 struct sk_buff *skb)
{
	struct ieee80211_hdr *hdr =
	    (struct ieee80211_hdr *)(skb->data + SSV6XXX_RX_DESC_LEN);
	struct ssv6200_rx_desc *rxdesc = (struct ssv6200_rx_desc *)skb->data;;
	if ((rxdesc->wsid >= 0) && (rxdesc->wsid < SSV_NUM_STA))
		return sc->sta_info[rxdesc->wsid].sta;
	else
		return ssv6xxx_find_sta_by_addr(sc, hdr->addr2);
}

struct ieee80211_sta *ssv6xxx_find_sta_by_addr(struct ssv_softc *sc, u8 addr[6])
{
	struct ieee80211_sta *sta;
	int i;
	for (i = 0; i < SSV6200_MAX_VIF; i++) {
		if (sc->vif_info[i].vif == NULL)
			continue;
		sta = ieee80211_find_sta(sc->vif_info[i].vif, addr);
		if (sta != NULL)
			return sta;
	}
	return NULL;
}

void ssv6xxx_foreach_sta(struct ssv_softc *sc,
			 void (*sta_func)(struct ssv_softc *,
					  struct ssv_sta_info *, void *),
			 void *param)
{
	int i;
	BUG_ON(sta_func == NULL);
	for (i = 0; i < SSV_NUM_STA; i++) {
		if ((sc->sta_info[i].s_flags & STA_FLAG_VALID) == 0)
			continue;
		(*sta_func) (sc, &sc->sta_info[i], param);
	}
}

void ssv6xxx_foreach_vif_sta(struct ssv_softc *sc,
			     struct ssv_vif_info *vif_info,
			     void (*sta_func)(struct ssv_softc *,
					      struct ssv_vif_info *,
					      struct ssv_sta_info *,
					      void *), void *param)
{
	struct ssv_vif_priv_data *vif_priv;
	struct ssv_sta_priv_data *sta_priv_iter;
	BUG_ON(vif_info == NULL);
	BUG_ON((size_t)vif_info < 0x30000);
	vif_priv = (struct ssv_vif_priv_data *)vif_info->vif->drv_priv;
	BUG_ON((size_t)vif_info->vif < 0x30000);
	BUG_ON((size_t)vif_priv < 0x30000);
	list_for_each_entry(sta_priv_iter, &vif_priv->sta_list, list) {
		BUG_ON(sta_priv_iter == NULL);
		BUG_ON((size_t)sta_priv_iter < 0x30000);
		BUG_ON(sta_priv_iter->sta_info == NULL);
		BUG_ON((size_t)sta_priv_iter->sta_info < 0x30000);
		if ((sta_priv_iter->sta_info->s_flags & STA_FLAG_VALID) == 0)
			continue;
		(*sta_func) (sc, vif_info, sta_priv_iter->sta_info, param);
	}
}

#ifdef CONFIG_SSV6XXX_DEBUGFS
ssize_t ssv6xxx_tx_queue_status_dump(struct ssv_softc *sc, char *status_buf,
				     ssize_t length)
{
	ssize_t buf_size = length;
	ssize_t prt_size;
	prt_size =
	    snprintf(status_buf, buf_size, "\nSMAC driver queue status:.\n");
	status_buf += prt_size;
	buf_size -= prt_size;
	prt_size = snprintf(status_buf, buf_size, "\tTX queue: %d\n",
			    skb_queue_len(&sc->tx_skb_q));
	status_buf += prt_size;
	buf_size -= prt_size;
	prt_size = snprintf(status_buf, buf_size, "\tMax TX queue: %d\n",
			    sc->max_tx_skb_q_len);
	status_buf += prt_size;
	buf_size -= prt_size;
	return (length - buf_size);
}
#endif
