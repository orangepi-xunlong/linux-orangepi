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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/nl80211.h>
#include <linux/kthread.h>
#include <linux/etherdevice.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0)
#include <crypto/hash.h>
#else
#include <linux/crypto.h>
#endif
#include <ssv6200.h>
#include <hci/hctrl.h>
#include <ssv_firmware_version.h>
#include "dev_tbl.h"
#include "dev.h"
#include "lib.h"
#include "ssv_rc.h"
#include "ap.h"
#include "efuse.h"
#include "sar.h"
#include "ssv_cfgvendor.h"

#include "linux_80211.h"
#ifdef CONFIG_SSV6XXX_DEBUGFS
#include "ssv6xxx_debugfs.h"
#endif

#define WIFI_FIRMWARE_NAME "ssv6051-sw.bin"
static const struct ieee80211_iface_limit ssv6xxx_p2p_limits[] = {
	{
	 .max = 2,
	 .types = BIT(NL80211_IFTYPE_STATION),
	 },
	{
	 .max = 1,
	 .types = BIT(NL80211_IFTYPE_P2P_GO) |
	 BIT(NL80211_IFTYPE_P2P_CLIENT) | BIT(NL80211_IFTYPE_AP),
	 },
};

static const struct ieee80211_iface_combination
 ssv6xxx_iface_combinations_p2p[] = {
	{.num_different_channels = 1,
	 .max_interfaces = SSV6200_MAX_VIF,
	 .beacon_int_infra_match = true,
	 .limits = ssv6xxx_p2p_limits,
	 .n_limits = ARRAY_SIZE(ssv6xxx_p2p_limits),
	 },
};

#define LBYTESWAP(a) ((((a) & 0x00ff00ff) << 8) | \
    (((a) & 0xff00ff00) >> 8))
#define LONGSWAP(a) ((LBYTESWAP(a) << 16) | (LBYTESWAP(a) >> 16))
#define CHAN2G(_freq,_idx) { \
    .band = INDEX_80211_BAND_2GHZ, \
    .center_freq = (_freq), \
    .hw_value = (_idx), \
    .max_power = 20, \
}
#ifndef WLAN_CIPHER_SUITE_SMS4
#define WLAN_CIPHER_SUITE_SMS4 0x00147201
#endif
#define SHPCHECK(__hw_rate,__flags) \
    ((__flags & IEEE80211_RATE_SHORT_PREAMBLE) ? (__hw_rate +3 ) : 0)
#define RATE(_bitrate,_hw_rate,_flags) { \
    .bitrate = (_bitrate), \
    .flags = (_flags), \
    .hw_value = (_hw_rate), \
    .hw_value_short = SHPCHECK(_hw_rate,_flags) \
}
extern struct ssv6xxx_cfg ssv_cfg;
static const struct ieee80211_channel ssv6200_2ghz_chantable[] = {
	CHAN2G(2412, 1),
	CHAN2G(2417, 2),
	CHAN2G(2422, 3),
	CHAN2G(2427, 4),
	CHAN2G(2432, 5),
	CHAN2G(2437, 6),
	CHAN2G(2442, 7),
	CHAN2G(2447, 8),
	CHAN2G(2452, 9),
	CHAN2G(2457, 10),
	CHAN2G(2462, 11),
	CHAN2G(2467, 12),
	CHAN2G(2472, 13),
	CHAN2G(2484, 14),
};

static struct ieee80211_rate ssv6200_legacy_rates[] = {
	RATE(10, 0x00, 0),
	RATE(20, 0x01, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(55, 0x02, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
	RATE(60, 0x07, 0),
	RATE(90, 0x08, 0),
	RATE(120, 0x09, 0),
	RATE(180, 0x0a, 0),
	RATE(240, 0x0b, 0),
	RATE(360, 0x0c, 0),
	RATE(480, 0x0d, 0),
	RATE(540, 0x0e, 0),
};

struct ssv6xxx_ch_cfg ch_cfg_z[] = {
	{ADR_ABB_REGISTER_1, 0, 0x151559fc},
	{ADR_LDO_REGISTER, 0, 0x00eb7c1c},
	{ADR_RX_ADC_REGISTER, 0, 0x20d000d2}
};

struct ssv6xxx_ch_cfg ch_cfg_p[] = {
	{ADR_ABB_REGISTER_1, 0, 0x151559fc},
	{ADR_RX_ADC_REGISTER, 0, 0x20d000d2}
};

int ssv6xxx_do_iq_calib(struct ssv_hw *sh, struct ssv6xxx_iqk_cfg *p_cfg)
{
	struct sk_buff *skb;
	struct cfg_host_cmd *host_cmd;
	int ret = 0;
	dev_dbg(sh->sc->dev, "# Do init_cali (iq)\n");
	skb =
	    ssv_skb_alloc(HOST_CMD_HDR_LEN + IQK_CFG_LEN + PHY_SETTING_SIZE +
			  RF_SETTING_SIZE);
	if (skb == NULL) {
		dev_err(sh->sc->dev, "init ssv6xxx_do_iq_calib failure\n");
		return (-1);
	}
	if ((PHY_SETTING_SIZE > MAX_PHY_SETTING_TABLE_SIZE) ||
	    (RF_SETTING_SIZE > MAX_RF_SETTING_TABLE_SIZE)) {
		dev_warn(sh->sc->dev, "wrong RF or PHY table size\n");
		WARN_ON(1);
		return (-1);
	}
	skb->data_len =
	    HOST_CMD_HDR_LEN + IQK_CFG_LEN + PHY_SETTING_SIZE + RF_SETTING_SIZE;
	skb->len = skb->data_len;
	host_cmd = (struct cfg_host_cmd *)skb->data;
	host_cmd->c_type = HOST_CMD;
	host_cmd->h_cmd = (u8) SSV6XXX_HOST_CMD_INIT_CALI;
	host_cmd->len = skb->data_len;
	p_cfg->phy_tbl_size = PHY_SETTING_SIZE;
	p_cfg->rf_tbl_size = RF_SETTING_SIZE;
	memcpy(host_cmd->dat32, p_cfg, IQK_CFG_LEN);
	memcpy(host_cmd->dat8 + IQK_CFG_LEN, phy_setting, PHY_SETTING_SIZE);
	memcpy(host_cmd->dat8 + IQK_CFG_LEN + PHY_SETTING_SIZE, ssv6200_rf_tbl,
	       RF_SETTING_SIZE);
	sh->hci.hci_ops->hci_send_cmd(skb);
	ssv_skb_free(skb);
	{
		u32 timeout;
		sh->sc->iq_cali_done = IQ_CALI_RUNNING;
		set_current_state(TASK_INTERRUPTIBLE);
		timeout = wait_event_interruptible_timeout(sh->sc->fw_wait_q,
							   sh->sc->iq_cali_done,
							   msecs_to_jiffies
							   (500));
		set_current_state(TASK_RUNNING);
		if (timeout == 0)
			return -ETIME;
		if (sh->sc->iq_cali_done != IQ_CALI_OK)
			return (-1);
	}
	return ret;
}

#define HT_CAP_RX_STBC_ONE_STREAM 0x1
#if defined(CONFIG_PM)
static const struct wiphy_wowlan_support wowlan_support = {
#ifdef SSV_WAKEUP_HOST
	.flags = WIPHY_WOWLAN_ANY,
#else
	.flags = WIPHY_WOWLAN_DISCONNECT,
#endif
	.n_patterns = 0,
	.pattern_max_len = 0,
	.pattern_min_len = 0,
	.max_pkt_offset = 0,
};
#endif
static void ssv6xxx_set_80211_hw_capab(struct ssv_softc *sc)
{
	struct ieee80211_hw *hw = sc->hw;
	struct ssv_hw *sh = sc->sh;
	struct ieee80211_sta_ht_cap *ht_info;
	ieee80211_hw_set(hw, SIGNAL_DBM);
	hw->rate_control_algorithm = "ssv6xxx_rate_control";
	//hw->rate_control_algorithm = NULL; // NULL selects default
	ht_info = &sc->sbands[INDEX_80211_BAND_2GHZ].ht_cap;
	ampdu_db_log("sh->cfg.hw_caps = 0x%x\n", sh->cfg.hw_caps);
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_HT) {
		if (sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_RX) {
			ieee80211_hw_set(hw, AMPDU_AGGREGATION);
			ampdu_db_log("set IEEE80211_HW_AMPDU_AGGREGATION(%d)\n",
				     ieee80211_hw_check(hw, AMPDU_AGGREGATION));
		}
		ht_info->cap = IEEE80211_HT_CAP_SM_PS;
		if (sh->cfg.hw_caps & SSV6200_HW_CAP_GF) {
			ht_info->cap |= IEEE80211_HT_CAP_GRN_FLD;
			ht_info->cap |=
			    HT_CAP_RX_STBC_ONE_STREAM <<
			    IEEE80211_HT_CAP_RX_STBC_SHIFT;
		}
		if (sh->cfg.hw_caps & SSV6200_HT_CAP_SGI_20)
			ht_info->cap |= IEEE80211_HT_CAP_SGI_20;
		ht_info->ampdu_factor = IEEE80211_HT_MAX_AMPDU_32K;
		ht_info->ampdu_density = IEEE80211_HT_MPDU_DENSITY_8;
		memset(&ht_info->mcs, 0, sizeof(ht_info->mcs));
		ht_info->mcs.rx_mask[0] = 0xff;
		ht_info->mcs.tx_params |= IEEE80211_HT_MCS_TX_DEFINED;
		ht_info->mcs.rx_highest = cpu_to_le16(SSV6200_RX_HIGHEST_RATE);
		ht_info->ht_supported = true;
	}
	hw->wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION);
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_P2P) {
		hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_P2P_CLIENT);
		hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_P2P_GO);
		hw->wiphy->iface_combinations = ssv6xxx_iface_combinations_p2p;
		hw->wiphy->n_iface_combinations =
		    ARRAY_SIZE(ssv6xxx_iface_combinations_p2p);
	}
	hw->wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_AP) {
		hw->wiphy->interface_modes |= BIT(NL80211_IFTYPE_AP);
		hw->wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
	}
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_TDLS) {
		hw->wiphy->flags |= WIPHY_FLAG_SUPPORTS_TDLS;
		hw->wiphy->flags |= WIPHY_FLAG_TDLS_EXTERNAL_SETUP;
		dev_info(sc->dev, "TDLS function enabled in sta.cfg\n");
	}
	hw->queues = 4;
	hw->max_rates = 4;
	hw->max_listen_interval = 1;
	hw->max_rate_tries = HW_MAX_RATE_TRIES;
	hw->extra_tx_headroom = TXPB_OFFSET + AMPDU_DELIMITER_LEN;
	if (sizeof(struct ampdu_hdr_st) > SSV_SKB_info_size)
		hw->extra_tx_headroom += sizeof(struct ampdu_hdr_st);
	else
		hw->extra_tx_headroom += SSV_SKB_info_size;
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
		hw->wiphy->bands[INDEX_80211_BAND_2GHZ] =
		    &sc->sbands[INDEX_80211_BAND_2GHZ];
	}
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_AMPDU_TX)
#ifdef PREFER_RX
		hw->max_rx_aggregation_subframes = 64;
#else
		hw->max_rx_aggregation_subframes = 16;
#endif
	else
		hw->max_rx_aggregation_subframes = 12;
	hw->max_tx_aggregation_subframes = 64;
	hw->sta_data_size = sizeof(struct ssv_sta_priv_data);
	hw->vif_data_size = sizeof(struct ssv_vif_priv_data);
	memcpy(sh->maddr[0].addr, &sh->cfg.maddr[0][0], ETH_ALEN);
	hw->wiphy->addresses = sh->maddr;
	hw->wiphy->n_addresses = 1;
	if (sh->cfg.hw_caps & SSV6200_HW_CAP_P2P) {
		int i;
		for (i = 1; i < SSV6200_MAX_HW_MAC_ADDR; i++) {
			memcpy(sh->maddr[i].addr, sh->maddr[i - 1].addr,
			       ETH_ALEN);
			sh->maddr[i].addr[5]++;
			hw->wiphy->n_addresses++;
		}
	}
	if (!is_zero_ether_addr(sh->cfg.maddr[1])) {
		memcpy(sh->maddr[1].addr, sh->cfg.maddr[1], ETH_ALEN);
		if (hw->wiphy->n_addresses < 2)
			hw->wiphy->n_addresses = 2;
	}
#if defined(CONFIG_PM)
	hw->wiphy->wowlan = &wowlan_support;
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 14, 0)) && defined(CONFIG_SSV_VENDOR_EXT_SUPPORT)
    {
        int err = 0;
        struct ssv_softc *softc = (struct ssv_softc *)hw->priv;
        if (softc)
        {
            set_wiphy_dev(hw->wiphy, softc->dev);
            *((struct ssv_softc **)wiphy_priv(hw->wiphy)) = softc;
        }
       	dev_dbg(sc->dev, "Registering Vendor80211\n");
       	err = ssv_cfgvendor_attach(hw->wiphy);
       	if (unlikely(err < 0)) {
       		dev_err(sc->dev, "Couldn not attach vendor commands (%d)\n", err);
       	}
    }
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(3, 14, 0)) || defined(WL_VENDOR_EXT_SUPPORT) */
}

void ssv6xxx_watchdog_restart_hw(struct ssv_softc *sc)
{
	dev_dbg(sc->dev, "%s()\n", __FUNCTION__);
	sc->restart_counter++;
	sc->force_triger_reset = true;
	sc->beacon_info[0].pubf_addr = 0x00;
	sc->beacon_info[1].pubf_addr = 0x00;
	ieee80211_restart_hw(sc->hw);
}

extern struct rssi_res_st rssi_res;
void ssv6200_watchdog_timeout(struct timer_list *t)
{
	static u32 count = 0;
	struct rssi_res_st *rssi_tmp0 = NULL, *rssi_tmp1 = NULL;
	struct ssv_softc *sc = from_timer(sc, t, watchdog_timeout);
	if (sc->watchdog_flag == WD_BARKING) {
		ssv6xxx_watchdog_restart_hw(sc);
		mod_timer(&sc->watchdog_timeout, jiffies + WATCHDOG_TIMEOUT);
		return;
	}
	if (sc->watchdog_flag != WD_SLEEP)
		sc->watchdog_flag = WD_BARKING;
	count++;
	if (count == 6) {
		count = 0;
		if (list_empty(&rssi_res.rssi_list)) {
			return;
		}
		list_for_each_entry_safe(rssi_tmp0, rssi_tmp1,
					 &rssi_res.rssi_list, rssi_list) {
			if (rssi_tmp0->timeout) {
				list_del_rcu(&rssi_tmp0->rssi_list);
				kfree(rssi_tmp0);
			}
		}
	}
	mod_timer(&sc->watchdog_timeout, jiffies + WATCHDOG_TIMEOUT);
	return;
}

static void ssv6xxx_preload_sw_cipher(void)
{
}

static int ssv6xxx_init_softc(struct ssv_softc *sc)
{
	void *channels;
	int ret = 0;
	sc->sc_flags = SC_OP_INVALID;
	mutex_init(&sc->mutex);
	mutex_init(&sc->mem_mutex);
	sc->config_wq = create_singlethread_workqueue("ssv6xxx_cong_wq");
	sc->thermal_wq = create_singlethread_workqueue("ssv6xxx_thermal_wq");
	INIT_DELAYED_WORK(&sc->thermal_monitor_work, thermal_monitor);
	INIT_WORK(&sc->set_tim_work, ssv6200_set_tim_work);
	INIT_WORK(&sc->bcast_start_work, ssv6200_bcast_start_work);
	INIT_DELAYED_WORK(&sc->bcast_stop_work, ssv6200_bcast_stop_work);
	INIT_DELAYED_WORK(&sc->bcast_tx_work, ssv6200_bcast_tx_work);
	INIT_WORK(&sc->set_ampdu_rx_add_work, ssv6xxx_set_ampdu_rx_add_work);
	INIT_WORK(&sc->set_ampdu_rx_del_work, ssv6xxx_set_ampdu_rx_del_work);
	sc->mac_deci_tbl = sta_deci_tbl;
	memset((void *)&sc->tx, 0, sizeof(struct ssv_tx));
	sc->tx.hw_txqid[WMM_AC_VO] = 3;
	sc->tx.ac_txqid[3] = WMM_AC_VO;
	sc->tx.hw_txqid[WMM_AC_VI] = 2;
	sc->tx.ac_txqid[2] = WMM_AC_VI;
	sc->tx.hw_txqid[WMM_AC_BE] = 1;
	sc->tx.ac_txqid[1] = WMM_AC_BE;
	sc->tx.hw_txqid[WMM_AC_BK] = 0;
	sc->tx.ac_txqid[0] = WMM_AC_BK;
	INIT_LIST_HEAD(&sc->tx.ampdu_tx_que);
	spin_lock_init(&sc->tx.ampdu_tx_que_lock);
	memset((void *)&sc->rx, 0, sizeof(struct ssv_rx));
	spin_lock_init(&sc->rx.rxq_lock);
	skb_queue_head_init(&sc->rx.rxq_head);
	sc->rx.rx_buf = ssv_skb_alloc(MAX_FRAME_SIZE);
	if (sc->rx.rx_buf == NULL)
		return -ENOMEM;
	memset(&sc->bcast_txq, 0, sizeof(struct ssv6xxx_bcast_txq));
	spin_lock_init(&sc->bcast_txq.txq_lock);
	skb_queue_head_init(&sc->bcast_txq.qhead);
	spin_lock_init(&sc->ps_state_lock);
#ifdef CONFIG_P2P_NOA
	spin_lock_init(&sc->p2p_noa.p2p_config_lock);
#endif
	if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
		channels = kmemdup(ssv6200_2ghz_chantable,
				   sizeof(ssv6200_2ghz_chantable), GFP_KERNEL);
		if (!channels) {
			kfree(sc->rx.rx_buf);
			return -ENOMEM;
		}
		sc->sbands[INDEX_80211_BAND_2GHZ].channels = channels;
		sc->sbands[INDEX_80211_BAND_2GHZ].band = INDEX_80211_BAND_2GHZ;
		sc->sbands[INDEX_80211_BAND_2GHZ].n_channels =
		    ARRAY_SIZE(ssv6200_2ghz_chantable);
		sc->sbands[INDEX_80211_BAND_2GHZ].bitrates =
		    ssv6200_legacy_rates;
		sc->sbands[INDEX_80211_BAND_2GHZ].n_bitrates =
		    ARRAY_SIZE(ssv6200_legacy_rates);
	}
	sc->cur_channel = NULL;
	sc->hw_chan = (-1);
	ssv6xxx_set_80211_hw_capab(sc);
	ret = ssv6xxx_rate_control_register();
	if (ret != 0) {
		dev_warn(sc->dev, "%s(): Failed to register rc algorithm.\n",__FUNCTION__);
	}
	init_waitqueue_head(&sc->tx_wait_q);
	sc->tx_wait_q_woken = 0;
	skb_queue_head_init(&sc->tx_skb_q);
#ifdef CONFIG_SSV6XXX_DEBUGFS
	sc->max_tx_skb_q_len = 0;
#endif
	sc->tx_task = kthread_run(ssv6xxx_tx_task, sc, "ssv6xxx_tx_task");
	sc->tx_q_empty = false;
	skb_queue_head_init(&sc->tx_done_q);
	init_waitqueue_head(&sc->rx_wait_q);
	sc->rx_wait_q_woken = 0;
	skb_queue_head_init(&sc->rx_skb_q);
	sc->rx_task = kthread_run(ssv6xxx_rx_task, sc, "ssv6xxx_rx_task");
	ssv6xxx_preload_sw_cipher();
	timer_setup(&sc->watchdog_timeout, ssv6200_watchdog_timeout, 0);
	init_waitqueue_head(&sc->fw_wait_q);
	INIT_LIST_HEAD(&rssi_res.rssi_list);
	rssi_res.rssi = 0;
	mod_timer(&sc->watchdog_timeout, jiffies + WATCHDOG_TIMEOUT);
	//add_timer(&sc->watchdog_timeout);
	//if(get_flash_info(sc) == 1)
	sc->is_sar_enabled = get_flash_info(sc);
	if (sc->is_sar_enabled)
		queue_delayed_work(sc->thermal_wq, &sc->thermal_monitor_work,
				   THERMAL_MONITOR_TIME);
	//schedule_delayed_work(&sc->thermal_monitor_work, THERMAL_MONITOR_TIME);
	return ret;
}

static int ssv6xxx_deinit_softc(struct ssv_softc *sc)
{
	void *channels;
	struct sk_buff *skb;
	u8 remain_size;
	dev_dbg(sc->dev, "%s():\n", __FUNCTION__);
	if (sc->sh->cfg.hw_caps & SSV6200_HW_CAP_2GHZ) {
		channels = sc->sbands[INDEX_80211_BAND_2GHZ].channels;
		kfree(channels);
	}
	ssv_skb_free(sc->rx.rx_buf);
	sc->rx.rx_buf = NULL;
	ssv6xxx_rate_control_unregister();
	cancel_delayed_work_sync(&sc->bcast_tx_work);
	//ssv6xxx_watchdog_controller(sc->sh ,(u8)SSV6XXX_HOST_CMD_WATCHDOG_STOP);
	del_timer_sync(&sc->watchdog_timeout);
	cancel_delayed_work(&sc->thermal_monitor_work);
	sc->ps_status = PWRSV_PREPARE;
	flush_workqueue(sc->thermal_wq);
	destroy_workqueue(sc->thermal_wq);
	do {
		skb = ssv6200_bcast_dequeue(&sc->bcast_txq, &remain_size);
		if (skb)
			ssv6xxx_txbuf_free_skb(skb, (void *)sc);
		else
			break;
	} while (remain_size);
	if (sc->tx_task != NULL) {
		dev_dbg(sc->dev, "Stopping TX task...\n");
		kthread_stop(sc->tx_task);
		sc->tx_task = NULL;
		dev_dbg(sc->dev, "Stopped TX task.\n");
	}
	if (sc->rx_task != NULL) {
		dev_dbg(sc->dev, "Stopping RX task...\n");
		kthread_stop(sc->rx_task);
		sc->rx_task = NULL;
		dev_dbg(sc->dev, "Stopped RX task.\n");
	}
	destroy_workqueue(sc->config_wq);
	return 0;
}

static void ssv6xxx_hw_set_replay_ignore(struct ssv_hw *sh, u8 ignore)
{
	u32 temp;
	SMAC_REG_READ(sh, ADR_SCRT_SET, &temp);
	temp = temp & SCRT_RPLY_IGNORE_I_MSK;
	temp |= (ignore << SCRT_RPLY_IGNORE_SFT);
	SMAC_REG_WRITE(sh, ADR_SCRT_SET, temp);
}

int ssv6xxx_init_mac(struct ssv_hw *sh)
{
	struct ssv_softc *sc = sh->sc;
	int i = 0, ret = 0;

	u32 *ptr, id_len, regval, temp[0x8];
	char *chip_id = sh->chip_id;
	SMAC_REG_READ(sh, ADR_IC_TIME_TAG_1, &regval);
	sh->chip_tag = ((u64) regval << 32);
	SMAC_REG_READ(sh, ADR_IC_TIME_TAG_0, &regval);
	sh->chip_tag |= (regval);
	SMAC_REG_READ(sh, ADR_CHIP_ID_3, &regval);
	*((u32 *) & chip_id[0]) = (u32) LONGSWAP(regval);
	SMAC_REG_READ(sh, ADR_CHIP_ID_2, &regval);
	*((u32 *) & chip_id[4]) = (u32) LONGSWAP(regval);
	SMAC_REG_READ(sh, ADR_CHIP_ID_1, &regval);
	*((u32 *) & chip_id[8]) = (u32) LONGSWAP(regval);
	SMAC_REG_READ(sh, ADR_CHIP_ID_0, &regval);
	*((u32 *) & chip_id[12]) = (u32) LONGSWAP(regval);
	chip_id[12 + sizeof(u32)] = 0;
	dev_info(sh->sc->dev, "chip id: %s, tag: %llx\n", chip_id, sh->chip_tag);
	if (sc->ps_status == PWRSV_ENABLE) {
		SMAC_REG_WRITE(sh, ADR_RX_FLOW_DATA,
			       M_ENG_MACRX | (M_ENG_ENCRYPT_SEC << 4) |
			       (M_ENG_HWHCI << 8));
		SMAC_REG_WRITE(sc->sh, ADR_RX_FLOW_MNG,
			       M_ENG_MACRX | (M_ENG_HWHCI << 4));
#if Enable_AMPDU_FW_Retry
		SMAC_REG_WRITE(sh, ADR_RX_FLOW_CTRL,
			       M_ENG_MACRX | (M_ENG_CPU << 4) | (M_ENG_HWHCI <<
								 8));
#else
		SMAC_REG_WRITE(sh, ADR_RX_FLOW_CTRL,
			       M_ENG_MACRX | (M_ENG_HWHCI << 4));
#endif
		SMAC_REG_WRITE(sc->sh, ADR_MRX_FLT_TB0 + 6 * 4,
			       (sc->mac_deci_tbl[6]));
		return ret;
	}
	SMAC_REG_SET_BITS(sh, ADR_PHY_EN_1, (0 << RG_PHY_MD_EN_SFT),
			  RG_PHY_MD_EN_MSK);
	SMAC_REG_WRITE(sh, ADR_BRG_SW_RST, 1 << MAC_SW_RST_SFT);
	do {
		SMAC_REG_READ(sh, ADR_BRG_SW_RST, &regval);
		i++;
		if (i > 10000) {
			dev_err(sh->sc->dev, "MAC reset fail !!!!\n");
			WARN_ON(1);
			ret = 1;
			goto exit;
		}
	} while (regval != 0);
	SMAC_REG_WRITE(sc->sh, ADR_TXQ4_MTX_Q_AIFSN, 0xffff2101);
	SMAC_REG_SET_BITS(sc->sh, ADR_MTX_BCN_EN_MISC, 0,
			  MTX_HALT_MNG_UNTIL_DTIM_MSK);
	SMAC_REG_WRITE(sh, ADR_CONTROL, 0x12000006);
	SMAC_REG_WRITE(sh, ADR_RX_TIME_STAMP_CFG,
		       ((28 << MRX_STP_OFST_SFT) | 0x01));
	SMAC_REG_WRITE(sh, ADR_HCI_TX_RX_INFO_SIZE,
		       ((u32) (TXPB_OFFSET) << TX_PBOFFSET_SFT) |
		       ((u32) (sh->tx_desc_len) << TX_INFO_SIZE_SFT) |
		       ((u32) (sh->rx_desc_len) << RX_INFO_SIZE_SFT) |
		       ((u32) (sh->rx_pinfo_pad) << RX_LAST_PHY_SIZE_SFT)
	    );
	SMAC_REG_READ(sh, ADR_MMU_CTRL, &regval);
	regval |= (0xff << MMU_SHARE_MCU_SFT);
	SMAC_REG_WRITE(sh, ADR_MMU_CTRL, regval);
	SMAC_REG_READ(sh, ADR_MRX_WATCH_DOG, &regval);
	regval &= 0xfffffff0;
	SMAC_REG_WRITE(sh, ADR_MRX_WATCH_DOG, regval);
	SMAC_REG_READ(sh, ADR_TRX_ID_THRESHOLD, &id_len);
	id_len = (id_len & 0xffff0000) |
	    (SSV6200_ID_TX_THRESHOLD << TX_ID_THOLD_SFT) |
	    (SSV6200_ID_RX_THRESHOLD << RX_ID_THOLD_SFT);
	SMAC_REG_WRITE(sh, ADR_TRX_ID_THRESHOLD, id_len);
	SMAC_REG_READ(sh, ADR_ID_LEN_THREADSHOLD1, &id_len);
	id_len = (id_len & 0x0f) |
	    (SSV6200_PAGE_TX_THRESHOLD << ID_TX_LEN_THOLD_SFT) |
	    (SSV6200_PAGE_RX_THRESHOLD << ID_RX_LEN_THOLD_SFT);
	SMAC_REG_WRITE(sh, ADR_ID_LEN_THREADSHOLD1, id_len);
#ifdef CONFIG_SSV_CABRIO_MB_DEBUG
	SMAC_REG_READ(sh, ADR_MB_DBG_CFG3, &regval);
	regval |= (debug_buffer << 0);
	SMAC_REG_WRITE(sh, ADR_MB_DBG_CFG3, regval);
	SMAC_REG_READ(sh, ADR_MB_DBG_CFG2, &regval);
	regval |= (DEBUG_SIZE << 16);
	SMAC_REG_WRITE(sh, ADR_MB_DBG_CFG2, regval);
	SMAC_REG_READ(sh, ADR_MB_DBG_CFG1, &regval);
	regval |= (1 << MB_DBG_EN_SFT);
	SMAC_REG_WRITE(sh, ADR_MB_DBG_CFG1, regval);
	SMAC_REG_READ(sh, ADR_MBOX_HALT_CFG, &regval);
	regval |= (1 << MB_ERR_AUTO_HALT_EN_SFT);
	SMAC_REG_WRITE(sh, ADR_MBOX_HALT_CFG, regval);
#endif
	SMAC_REG_READ(sc->sh, ADR_MTX_BCN_EN_MISC, &regval);
	regval |= (1 << MTX_TSF_TIMER_EN_SFT);
	SMAC_REG_WRITE(sc->sh, ADR_MTX_BCN_EN_MISC, regval);
	SMAC_REG_WRITE(sh, 0xcd010004, 0x1213);
	for (i = 0; i < SSV_RC_MAX_STA; i++) {
		if (i == 0) {
			sh->hw_buf_ptr[i] =
			    ssv6xxx_pbuf_alloc(sc,
					       sizeof(phy_info_tbl) +
					       sizeof(struct ssv6xxx_hw_sec),
					       NOTYPE_BUF);
			if ((sh->hw_buf_ptr[i] >> 28) != 8) {
				dev_err(sh->sc->dev, "opps allocate pbuf error\n");
				WARN_ON(1);
				ret = 1;
				goto exit;
			}
		} else {
			sh->hw_buf_ptr[i] =
			    ssv6xxx_pbuf_alloc(sc,
					       sizeof(struct ssv6xxx_hw_sec),
					       NOTYPE_BUF);
			if ((sh->hw_buf_ptr[i] >> 28) != 8) {
				dev_err(sh->sc->dev, "opps allocate pbuf error\n");
				WARN_ON(1);
				ret = 1;
				goto exit;
			}
		}
	}
	for (i = 0; i < 0x8; i++) {
		temp[i] = 0;
		temp[i] = ssv6xxx_pbuf_alloc(sc, 256, NOTYPE_BUF);
	}
	for (i = 0; i < 0x8; i++) {
		if (temp[i] == 0x800e0000)
			dev_dbg(sh->sc->dev, "Found 0x800e0000 at position %d\n", i);
		else
			ssv6xxx_pbuf_free(sc, temp[i]);
	}
	for (i = 0; i < SSV_RC_MAX_STA; i++)
		sh->hw_sec_key[i] = sh->hw_buf_ptr[i];
	for (i = 0; i < SSV_RC_MAX_STA; i++) {
		int x;
		for (x = 0; x < sizeof(struct ssv6xxx_hw_sec); x += 4) {
			SMAC_REG_WRITE(sh, sh->hw_sec_key[i] + x, 0);
		}
	}
	SMAC_REG_READ(sh, ADR_SCRT_SET, &regval);
	regval &= SCRT_PKT_ID_I_MSK;
	regval |= ((sh->hw_sec_key[0] >> 16) << SCRT_PKT_ID_SFT);
	SMAC_REG_WRITE(sh, ADR_SCRT_SET, regval);
	sh->hw_pinfo = sh->hw_sec_key[0] + sizeof(struct ssv6xxx_hw_sec);
	for (i = 0, ptr = phy_info_tbl; i < PHY_INFO_TBL1_SIZE; i++, ptr++) {
		SMAC_REG_WRITE(sh, ADR_INFO0 + i * 4, *ptr);
		SMAC_REG_CONFIRM(sh, ADR_INFO0 + i * 4, *ptr);
	}
	for (i = 0; i < PHY_INFO_TBL2_SIZE; i++, ptr++) {
		SMAC_REG_WRITE(sh, sh->hw_pinfo + i * 4, *ptr);
		SMAC_REG_CONFIRM(sh, sh->hw_pinfo + i * 4, *ptr);
	}
	for (i = 0; i < PHY_INFO_TBL3_SIZE; i++, ptr++) {
		SMAC_REG_WRITE(sh, sh->hw_pinfo +
			       (PHY_INFO_TBL2_SIZE << 2) + i * 4, *ptr);
		SMAC_REG_CONFIRM(sh, sh->hw_pinfo +
				 (PHY_INFO_TBL2_SIZE << 2) + i * 4, *ptr);
	}
	SMAC_REG_WRITE(sh, ADR_INFO_RATE_OFFSET, 0x00040000);
	SMAC_REG_WRITE(sh, ADR_INFO_IDX_ADDR, sh->hw_pinfo);
	SMAC_REG_WRITE(sh, ADR_INFO_LEN_ADDR,
		       sh->hw_pinfo + (PHY_INFO_TBL2_SIZE) * 4);
	dev_dbg(sh->sc->dev, "ADR_INFO_IDX_ADDR[%08x] ADR_INFO_LEN_ADDR[%08x]\n",
	       sh->hw_pinfo, sh->hw_pinfo + (PHY_INFO_TBL2_SIZE) * 4);
	SMAC_REG_WRITE(sh, ADR_GLBLE_SET,
		       (0 << OP_MODE_SFT) | (0 << SNIFFER_MODE_SFT) | (1 <<
								       DUP_FLT_SFT)
		       | (SSV6200_TX_PKT_RSVD_SETTING << TX_PKT_RSVD_SFT) |
		       ((u32) (RXPB_OFFSET) << PB_OFFSET_SFT)
	    );
	SMAC_REG_WRITE(sh, ADR_STA_MAC_0, *((u32 *) & sh->cfg.maddr[0][0]));
	SMAC_REG_WRITE(sh, ADR_STA_MAC_1, *((u32 *) & sh->cfg.maddr[0][4]));
	SMAC_REG_WRITE(sh, ADR_BSSID_0, *((u32 *) & sc->bssid[0]));
	SMAC_REG_WRITE(sh, ADR_BSSID_1, *((u32 *) & sc->bssid[4]));
	SMAC_REG_WRITE(sh, ADR_TX_ETHER_TYPE_0, 0x00000000);
	SMAC_REG_WRITE(sh, ADR_TX_ETHER_TYPE_1, 0x00000000);
	SMAC_REG_WRITE(sh, ADR_RX_ETHER_TYPE_0, 0x00000000);
	SMAC_REG_WRITE(sh, ADR_RX_ETHER_TYPE_1, 0x00000000);
	SMAC_REG_WRITE(sh, ADR_REASON_TRAP0, 0x7FBC7F87);
	SMAC_REG_WRITE(sh, ADR_REASON_TRAP1, 0x0000003F);
	SMAC_REG_WRITE(sh, ADR_TRAP_HW_ID, M_ENG_CPU);
	SMAC_REG_WRITE(sh, ADR_WSID0, 0x00000000);
	SMAC_REG_WRITE(sh, ADR_WSID1, 0x00000000);
	SMAC_REG_WRITE(sh, ADR_RX_FLOW_DATA,
		       M_ENG_MACRX | (M_ENG_ENCRYPT_SEC << 4) | (M_ENG_HWHCI <<
								 8));
#if defined(CONFIG_P2P_NOA) || defined(CONFIG_RX_MGMT_CHECK)
	SMAC_REG_WRITE(sh, ADR_RX_FLOW_MNG,
		       M_ENG_MACRX | (M_ENG_CPU << 4) | (M_ENG_HWHCI << 8));
#else
	SMAC_REG_WRITE(sh, ADR_RX_FLOW_MNG, M_ENG_MACRX | (M_ENG_HWHCI << 4));
#endif
#if Enable_AMPDU_FW_Retry
	SMAC_REG_WRITE(sh, ADR_RX_FLOW_CTRL,
		       M_ENG_MACRX | (M_ENG_CPU << 4) | (M_ENG_HWHCI << 8));
#else
	SMAC_REG_WRITE(sh, ADR_RX_FLOW_CTRL, M_ENG_MACRX | (M_ENG_HWHCI << 4));
#endif
	ssv6xxx_hw_set_replay_ignore(sh, 1);
	ssv6xxx_update_decision_table(sc);
	SMAC_REG_SET_BITS(sc->sh, ADR_GLBLE_SET, SSV6200_OPMODE_STA,
			  OP_MODE_MSK);
	SMAC_REG_WRITE(sh, ADR_SDIO_MASK, 0xfffe1fff);
	SMAC_REG_WRITE(sh, ADR_TX_LIMIT_INTR, 0x80000000 |
		       SSV6200_TX_LOWTHRESHOLD_ID_TRIGGER << 16 |
		       SSV6200_TX_LOWTHRESHOLD_PAGE_TRIGGER);
#ifdef CONFIG_SSV_SUPPORT_BTCX
	SMAC_REG_WRITE(sh, ADR_BTCX0,
		       COEXIST_EN_MSK | (WIRE_MODE_SZ << WIRE_MODE_SFT)
		       | WIFI_TX_SW_POL_MSK | BT_SW_POL_MSK);
	SMAC_REG_WRITE(sh, ADR_BTCX1,
		       SSV6200_BT_PRI_SMP_TIME | (SSV6200_BT_STA_SMP_TIME <<
						  BT_STA_SMP_TIME_SFT)
		       | (SSV6200_WLAN_REMAIN_TIME << WLAN_REMAIN_TIME_SFT));
	SMAC_REG_WRITE(sh, ADR_SWITCH_CTL, BT_2WIRE_EN_MSK);
	SMAC_REG_WRITE(sh, ADR_PAD7, 1);
	SMAC_REG_WRITE(sh, ADR_PAD8, 0);
	SMAC_REG_WRITE(sh, ADR_PAD9, 1);
	SMAC_REG_WRITE(sh, ADR_PAD25, 1);
	SMAC_REG_WRITE(sh, ADR_PAD27, 8);
	SMAC_REG_WRITE(sh, ADR_PAD28, 8);
#endif
	dev_info(sh->sc->dev, "attempt to load firmware %s\n", WIFI_FIRMWARE_NAME);
	ret = SMAC_LOAD_FW(sh, WIFI_FIRMWARE_NAME, 0);

	SMAC_REG_READ(sh, FW_VERSION_REG, &regval);
	if (regval == ssv_firmware_version) {
		SMAC_REG_SET_BITS(sh, ADR_PHY_EN_1, (1 << RG_PHY_MD_EN_SFT),
				  RG_PHY_MD_EN_MSK);
		dev_info(sh->sc->dev, "Firmware version %d\n", regval);
	} else {
		dev_err(sh->sc->dev, "Firmware version not mapping %d\n", regval);
		ret = -1;
	}
	ssv6xxx_watchdog_controller(sh, (u8) SSV6XXX_HOST_CMD_WATCHDOG_START);
 exit:
	return ret;
}

void ssv6xxx_deinit_mac(struct ssv_softc *sc)
{
	int i;
	for (i = 0; i < SSV_RC_MAX_STA; i++) {
		if (sc->sh->hw_buf_ptr[i])
			ssv6xxx_pbuf_free(sc, sc->sh->hw_buf_ptr[i]);
	}
}

void inline ssv6xxx_deinit_hw(struct ssv_softc *sc)
{
	dev_dbg(sc->dev, "%s(): \n", __FUNCTION__);
	ssv6xxx_deinit_mac(sc);
}

void ssv6xxx_restart_hw(struct ssv_softc *sc)
{
	dev_info(sc->dev, "Software MAC reset\n");
	sc->restart_counter++;
	sc->force_triger_reset = true;
	HCI_STOP(sc->sh);
	SMAC_REG_WRITE(sc->sh, 0xce000004, 0x0);
	sc->beacon_info[0].pubf_addr = 0x00;
	sc->beacon_info[1].pubf_addr = 0x00;
	ieee80211_restart_hw(sc->hw);
}

extern struct ssv6xxx_iqk_cfg init_iqk_cfg;
static int ssv6xxx_init_hw(struct ssv_hw *sh)
{
	int ret = 0, i = 0, x = 0;
	u32 regval;
	sh->tx_desc_len = SSV6XXX_TX_DESC_LEN;
	sh->rx_desc_len = SSV6XXX_RX_DESC_LEN;
	sh->rx_pinfo_pad = 0x04;
	sh->tx_page_available = SSV6200_PAGE_TX_THRESHOLD;
	sh->ampdu_divider = SSV6XXX_AMPDU_DIVIDER;
	memset(sh->page_count, 0, sizeof(sh->page_count));
	if (sh->cfg.force_chip_identity) {
		dev_info(sh->sc->dev, "Force use external RF setting [%08x]\n",
		       sh->cfg.force_chip_identity);
		sh->cfg.chip_identity = sh->cfg.force_chip_identity;
	}
	if (sh->cfg.chip_identity == SSV6051Z) {
		sh->p_ch_cfg = &ch_cfg_z[0];
		sh->ch_cfg_size =
		    sizeof(ch_cfg_z) / sizeof(struct ssv6xxx_ch_cfg);
		memcpy(phy_info_tbl, phy_info_6051z, sizeof(phy_info_6051z));
	} else if (sh->cfg.chip_identity == SSV6051P) {
		sh->p_ch_cfg = &ch_cfg_p[0];
		sh->ch_cfg_size =
		    sizeof(ch_cfg_p) / sizeof(struct ssv6xxx_ch_cfg);
	}
	switch (sh->cfg.chip_identity) {
	case SSV6051Q_P1:
	case SSV6051Q_P2:
	case SSV6051Q:
		dev_info(sh->sc->dev, "Using SSV6051Q setting\n");
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address == 0xCE010008)
				ssv6200_rf_tbl[i].data = 0x008DF61B;
			if (ssv6200_rf_tbl[i].address == 0xCE010014)
				ssv6200_rf_tbl[i].data = 0x3D3E84FE;
			if (ssv6200_rf_tbl[i].address == 0xCE010018)
				ssv6200_rf_tbl[i].data = 0x01457D79;
			if (ssv6200_rf_tbl[i].address == 0xCE01001C)
				ssv6200_rf_tbl[i].data = 0x000103A7;
			if (ssv6200_rf_tbl[i].address == 0xCE010020)
				ssv6200_rf_tbl[i].data = 0x000103A6;
			if (ssv6200_rf_tbl[i].address == 0xCE01002C)
				ssv6200_rf_tbl[i].data = 0x00032CA8;
			if (ssv6200_rf_tbl[i].address == 0xCE010048)
				ssv6200_rf_tbl[i].data = 0xFCCCCF27;
			if (ssv6200_rf_tbl[i].address == 0xCE010050)
				ssv6200_rf_tbl[i].data = 0x0047C000;
		}
		break;
	case SSV6051Z:
		dev_info(sh->sc->dev, "Using SSV6051Z setting\n");
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address == 0xCE010008)
				ssv6200_rf_tbl[i].data = 0x004D561C;
			if (ssv6200_rf_tbl[i].address == 0xCE010014)
				ssv6200_rf_tbl[i].data = 0x3D9E84FE;
			if (ssv6200_rf_tbl[i].address == 0xCE010018)
				ssv6200_rf_tbl[i].data = 0x00457D79;
			if (ssv6200_rf_tbl[i].address == 0xCE01001C)
				ssv6200_rf_tbl[i].data = 0x000103EB;
			if (ssv6200_rf_tbl[i].address == 0xCE010020)
				ssv6200_rf_tbl[i].data = 0x000103EA;
			if (ssv6200_rf_tbl[i].address == 0xCE01002C)
				ssv6200_rf_tbl[i].data = 0x00062CA8;
			if (ssv6200_rf_tbl[i].address == 0xCE010048)
				ssv6200_rf_tbl[i].data = 0xFCCCCF27;
			if (ssv6200_rf_tbl[i].address == 0xCE010050)
				ssv6200_rf_tbl[i].data = 0x0047C000;
		}
		break;
	case SSV6051P:
		dev_info(sh->sc->dev, "Using SSV6051P setting\n");
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address == 0xCE010008)
				ssv6200_rf_tbl[i].data = 0x008B7C1C;
			if (ssv6200_rf_tbl[i].address == 0xCE010014)
				ssv6200_rf_tbl[i].data = 0x3D7E84FE;
			if (ssv6200_rf_tbl[i].address == 0xCE010018)
				ssv6200_rf_tbl[i].data = 0x01457D79;
			if (ssv6200_rf_tbl[i].address == 0xCE01001C)
				ssv6200_rf_tbl[i].data = 0x000103EB;
			if (ssv6200_rf_tbl[i].address == 0xCE010020)
				ssv6200_rf_tbl[i].data = 0x000103EA;
			if (ssv6200_rf_tbl[i].address == 0xCE01002C)
				ssv6200_rf_tbl[i].data = 0x00032CA8;
			if (ssv6200_rf_tbl[i].address == 0xCE010048)
				ssv6200_rf_tbl[i].data = 0xFCCCCC27;
			if (ssv6200_rf_tbl[i].address == 0xCE010050)
				ssv6200_rf_tbl[i].data = 0x0047C000;
			if (ssv6200_rf_tbl[i].address == 0xC0001D00)
				ssv6200_rf_tbl[i].data = 0x5E000040;
		}
		break;
	default:
		dev_err(sh->sc->dev, "No RF setting\n");
		break;
	}
	if (sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_26M) {
		init_iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_26M;
		dev_info(sh->sc->dev, "Crystal frequency: 26 Mhz\n");
	} else if (sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_40M) {
		init_iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_40M;
		dev_info(sh->sc->dev, "Crystal frequency: 40 Mhz\n");
	} else if (sh->cfg.crystal_type == SSV6XXX_IQK_CFG_XTAL_24M) {
		init_iqk_cfg.cfg_xtal = SSV6XXX_IQK_CFG_XTAL_24M;
		dev_info(sh->sc->dev, "Crystal frequency: 24 Mhz\n");
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address == ADR_SX_ENABLE_REGISTER)
				ssv6200_rf_tbl[i].data = 0x0003E07C;
			if (ssv6200_rf_tbl[i].address ==
			    ADR_DPLL_DIVIDER_REGISTER)
				ssv6200_rf_tbl[i].data = 0x00406000;
			if (ssv6200_rf_tbl[i].address ==
			    ADR_DPLL_FB_DIVIDER_REGISTERS_I)
				ssv6200_rf_tbl[i].data = 0x00000028;
			if (ssv6200_rf_tbl[i].address ==
			    ADR_DPLL_FB_DIVIDER_REGISTERS_II)
				ssv6200_rf_tbl[i].data = 0x00000000;
		}
	} else {
		dev_warn(sh->sc->dev, "Illegal crystal setting, using default value of 26 Mhz\n");
	}
	for (i = 0;
	     i < sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
	     i++) {
		if (ssv6200_rf_tbl[i].address ==
		    ADR_SYN_KVCO_XO_FINE_TUNE_CBANK) {
			if (sh->cfg.crystal_frequency_offset) {
				ssv6200_rf_tbl[i].data &=
				    RG_XOSC_CBANK_XO_I_MSK;
				ssv6200_rf_tbl[i].data |=
				    (sh->cfg.
				     crystal_frequency_offset <<
				     RG_XOSC_CBANK_XO_SFT);
			}
		}
	}
	for (i = 0; i < sizeof(phy_setting) / sizeof(struct ssv6xxx_dev_table);
	     i++) {
		if (phy_setting[i].address == ADR_TX_GAIN_FACTOR) {
			switch (sh->cfg.chip_identity) {
			case SSV6051Q_P1:
			case SSV6051Q_P2:
			case SSV6051Q:
				dev_dbg(sh->sc->dev, "SSV6051Q setting [0x5B606C72]\n");
				phy_setting[i].data = 0x5B606C72;
				break;
			case SSV6051Z:
				dev_dbg(sh->sc->dev, "SSV6051Z setting [0x60606060]\n");
				phy_setting[i].data = 0x60606060;
				break;
			case SSV6051P:
				dev_dbg(sh->sc->dev, "SSV6051P setting [0x6C726C72]\n");
				phy_setting[i].data = 0x6C726C72;
				break;
			default:
				dev_dbg(sh->sc->dev, "Use default power setting\n");
				break;
			}
			if (sh->cfg.wifi_tx_gain_level_b) {
				phy_setting[i].data &= 0xffff0000;
				phy_setting[i].data |=
				    wifi_tx_gain[sh->cfg.
						 wifi_tx_gain_level_b] &
				    0x0000ffff;
			}
			if (sh->cfg.wifi_tx_gain_level_gn) {
				phy_setting[i].data &= 0x0000ffff;
				phy_setting[i].data |=
				    wifi_tx_gain[sh->cfg.
						 wifi_tx_gain_level_gn] &
				    0xffff0000;
			}
			dev_dbg(sh->sc->dev, "TX power setting 0x%x\n", phy_setting[i].data);
			init_iqk_cfg.cfg_def_tx_scale_11b =
			    (phy_setting[i].data >> 0) & 0xff;
			init_iqk_cfg.cfg_def_tx_scale_11b_p0d5 =
			    (phy_setting[i].data >> 8) & 0xff;
			init_iqk_cfg.cfg_def_tx_scale_11g =
			    (phy_setting[i].data >> 16) & 0xff;
			init_iqk_cfg.cfg_def_tx_scale_11g_p0d5 =
			    (phy_setting[i].data >> 24) & 0xff;
			break;
		}
	}
	if (sh->cfg.volt_regulator == SSV6XXX_VOLT_LDO_CONVERT) {
		dev_info(sh->sc->dev, "Using LDO voltage regulator\n");
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address == ADR_PMU_2) {
				ssv6200_rf_tbl[i].data &= 0xFFFFFFFE;
				ssv6200_rf_tbl[i].data |= 0x00000000;
			}
		}
	} else if (sh->cfg.volt_regulator == SSV6XXX_VOLT_DCDC_CONVERT) {
		dev_info(sh->sc->dev, "Using DCDC buck regulator\n");
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address == ADR_PMU_2) {
				ssv6200_rf_tbl[i].data &= 0xFFFFFFFE;
				ssv6200_rf_tbl[i].data |= 0x00000001;
			}
		}
	} else {
		dev_warn(sh->sc->dev, "Illegal regulator setting, using DCDC buck as default\n");
	}
	while (ssv_cfg.configuration[x][0]) {
		for (i = 0;
		     i <
		     sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (ssv6200_rf_tbl[i].address ==
			    ssv_cfg.configuration[x][0]) {
				ssv6200_rf_tbl[i].data =
				    ssv_cfg.configuration[x][1];
				break;
			}
		}
		for (i = 0;
		     i < sizeof(phy_setting) / sizeof(struct ssv6xxx_dev_table);
		     i++) {
			if (phy_setting[i].address ==
			    ssv_cfg.configuration[x][0]) {
				phy_setting[i].data =
				    ssv_cfg.configuration[x][1];
				break;
			}
		}
		x++;
	};
	if (ret == 0)
		ret = SSV6XXX_SET_HW_TABLE(sh, ssv6200_rf_tbl);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_PHY_EN_1, 0x00000000);
	SMAC_REG_READ(sh, ADR_PHY_EN_0, &regval);
	if (regval & (1 << RG_RF_BB_CLK_SEL_SFT)) {
		dev_dbg(sh->sc->dev, "already do clock switch\n");
	} else {
		dev_dbg(sh->sc->dev, "reset PLL\n");
		SMAC_REG_READ(sh, ADR_DPLL_CP_PFD_REGISTER, &regval);
		regval |=
		    ((1 << RG_DP_BBPLL_PD_SFT) |
		     (1 << RG_DP_BBPLL_SDM_EDGE_SFT));
		SMAC_REG_WRITE(sh, ADR_DPLL_CP_PFD_REGISTER, regval);
		regval &=
		    ~((1 << RG_DP_BBPLL_PD_SFT) |
		      (1 << RG_DP_BBPLL_SDM_EDGE_SFT));
		SMAC_REG_WRITE(sh, ADR_DPLL_CP_PFD_REGISTER, regval);
		mdelay(10);
	}
	if (ret == 0)
		ret = SSV6XXX_SET_HW_TABLE(sh, ssv6200_phy_tbl);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_TRX_DUMMY_REGISTER, 0xEAAAAAAA);
	SMAC_REG_READ(sh, ADR_TRX_DUMMY_REGISTER, &regval);
	if (regval != 0xEAAAAAAA) {
		dev_warn(sh->sc->dev, "Unexpected register value\n");
		WARN_ON(1);
	}
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_PAD53, 0x21);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_PAD54, 0x3000);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_PIN_SEL_0, 0x4000);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, 0xc0000304, 0x01);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, 0xc0000308, 0x01);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_CLOCK_SELECTION, 0x3);
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_TRX_DUMMY_REGISTER, 0xAAAAAAAA);
	if ((ret = ssv6xxx_set_channel(sh->sc, sh->cfg.def_chan)))
		return ret;
	if (ret == 0)
		ret = SMAC_REG_WRITE(sh, ADR_PHY_EN_1,
				     (RG_PHYRX_MD_EN_MSK | RG_PHYTX_MD_EN_MSK |
				      RG_PHY11GN_MD_EN_MSK | RG_PHY11B_MD_EN_MSK
				      | RG_PHYRXFIFO_MD_EN_MSK |
				      RG_PHYTXFIFO_MD_EN_MSK |
				      RG_PHY11BGN_MD_EN_MSK));
	return ret;
}

static void ssv6xxx_check_mac2(struct ssv_hw *sh)
{
	const u8 addr_mask[6] = { 0xfd, 0xff, 0xff, 0xff, 0xff, 0xfc };
	u8 i;
	bool invalid = false;
	for (i = 0; i < 6; i++) {
		if ((ssv_cfg.maddr[0][i] & addr_mask[i]) !=
		    (ssv_cfg.maddr[1][i] & addr_mask[i])) {
			invalid = true;
			dev_dbg(sh->sc->dev, " i %d , mac1[i] %x, mac2[i] %x, mask %x \n", i,
			       ssv_cfg.maddr[0][i], ssv_cfg.maddr[1][i],
			       addr_mask[i]);
			break;
		}
	}
	if (invalid) {
		memcpy(&ssv_cfg.maddr[1][0], &ssv_cfg.maddr[0][0], 6);
		ssv_cfg.maddr[1][5] ^= 0x01;
		if (ssv_cfg.maddr[1][5] < ssv_cfg.maddr[0][5]) {
			u8 temp;
			temp = ssv_cfg.maddr[0][5];
			ssv_cfg.maddr[0][5] = ssv_cfg.maddr[1][5];
			ssv_cfg.maddr[1][5] = temp;
			sh->cfg.maddr[0][5] = ssv_cfg.maddr[0][5];
		}
		dev_warn(sh->sc->dev, "MAC 2 address invalid!!\n");
		dev_warn(sh->sc->dev, "After modification, MAC1 %pM, MAC2 %pM\n",
		       ssv_cfg.maddr[0], ssv_cfg.maddr[1]);
	}
}

static int ssv6xxx_read_configuration(struct ssv_hw *sh)
{
	extern u32 sdio_sr_bhvr;
	if (is_valid_ether_addr(&ssv_cfg.maddr[0][0]))
		memcpy(&sh->cfg.maddr[0][0], &ssv_cfg.maddr[0][0], ETH_ALEN);
	if (is_valid_ether_addr(&ssv_cfg.maddr[1][0])) {
		ssv6xxx_check_mac2(sh);
		memcpy(&sh->cfg.maddr[1][0], &ssv_cfg.maddr[1][0], ETH_ALEN);
	}
	if (ssv_cfg.hw_caps)
		sh->cfg.hw_caps = ssv_cfg.hw_caps;
	else
		sh->cfg.hw_caps = SSV6200_HW_CAP_HT |
		    SSV6200_HW_CAP_2GHZ |
		    SSV6200_HW_CAP_SECURITY |
		    SSV6200_HW_CAP_P2P |
		    SSV6200_HT_CAP_SGI_20 |
		    SSV6200_HW_CAP_AMPDU_RX |
		    SSV6200_HW_CAP_AMPDU_TX | SSV6200_HW_CAP_AP;
	if (ssv_cfg.def_chan)
		sh->cfg.def_chan = ssv_cfg.def_chan;
	else
		sh->cfg.def_chan = 6;
	sh->cfg.use_wpa2_only = ssv_cfg.use_wpa2_only;
	if (ssv_cfg.crystal_type == 26)
		sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_26M;
	else if (ssv_cfg.crystal_type == 40)
		sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_40M;
	else if (ssv_cfg.crystal_type == 24)
		sh->cfg.crystal_type = SSV6XXX_IQK_CFG_XTAL_24M;
	else {
		dev_warn(sh->sc->dev, "Please redefine xtal_clock(wifi.cfg)!!\n");
		WARN_ON(1);
		return 1;
	}
	if (ssv_cfg.volt_regulator < 2)
		sh->cfg.volt_regulator = ssv_cfg.volt_regulator;
	else {
		dev_warn(sh->sc->dev, "Please redefine volt_regulator(wifi.cfg)!!\n");
		WARN_ON(1);
		return 1;
	}
	sh->cfg.wifi_tx_gain_level_gn = ssv_cfg.wifi_tx_gain_level_gn;
	sh->cfg.wifi_tx_gain_level_b = ssv_cfg.wifi_tx_gain_level_b;
	sh->cfg.rssi_ctl = ssv_cfg.rssi_ctl;
	sh->cfg.sr_bhvr = ssv_cfg.sr_bhvr;
	sdio_sr_bhvr = ssv_cfg.sr_bhvr;
	sh->cfg.force_chip_identity = ssv_cfg.force_chip_identity;
	strncpy(sh->cfg.firmware_path, ssv_cfg.firmware_path,
		sizeof(sh->cfg.firmware_path) - 1);
	strncpy(sh->cfg.flash_bin_path, ssv_cfg.flash_bin_path,
		sizeof(sh->cfg.flash_bin_path) - 1);
	strncpy(sh->cfg.mac_address_path, ssv_cfg.mac_address_path,
		sizeof(sh->cfg.mac_address_path) - 1);
	strncpy(sh->cfg.mac_output_path, ssv_cfg.mac_output_path,
		sizeof(sh->cfg.mac_output_path) - 1);
	sh->cfg.ignore_efuse_mac = ssv_cfg.ignore_efuse_mac;
	sh->cfg.mac_address_mode = ssv_cfg.mac_address_mode;
	return 0;
}

static int ssv6xxx_read_hw_info(struct ssv_softc *sc)
{
	struct ssv_hw *sh;
	sh = kzalloc(sizeof(struct ssv_hw), GFP_KERNEL);
	if (sh == NULL)
		return -ENOMEM;
	memset((void *)sh, 0, sizeof(struct ssv_hw));
	sc->sh = sh;
	sh->sc = sc;
	sh->priv = sc->dev->platform_data;
	if (ssv6xxx_read_configuration(sh))
		return -ENOMEM;
	sh->hci.dev = sc->dev;
	sh->hci.hci_ops = NULL;
	sh->hci.hci_rx_cb = ssv6200_rx;
	sh->hci.rx_cb_args = (void *)sc;
	sh->hci.hci_tx_cb = ssv6xxx_tx_cb;
	sh->hci.tx_cb_args = (void *)sc;
	sh->hci.hci_skb_update_cb = ssv6xxx_tx_rate_update;
	sh->hci.skb_update_args = (void *)sc;
	sh->hci.hci_tx_flow_ctrl_cb = ssv6200_tx_flow_control;
	sh->hci.tx_fctrl_cb_args = (void *)sc;
	sh->hci.hci_tx_q_empty_cb = ssv6xxx_tx_q_empty_cb;
	sh->hci.tx_q_empty_args = (void *)sc;
	sh->hci.if_ops = sh->priv->ops;
	sh->hci.hci_tx_buf_free_cb = ssv6xxx_txbuf_free_skb;
	sh->hci.tx_buf_free_args = (void *)sc;
	return 0;
}

static int ssv6xxx_init_device(struct ssv_softc *sc, const char *name)
{
	struct ieee80211_hw *hw = sc->hw;
	struct ssv_hw *sh;
	int error = 0;
	BUG_ON(!sc->dev->platform_data);
	if ((error = ssv6xxx_read_hw_info(sc)) != 0) {
		return error;
	}
	sh = sc->sh;
	if (sh->cfg.hw_caps == 0)
		return -1;
	ssv6xxx_hci_register(&sh->hci);
	efuse_read_all_map(sh);
	if ((error = ssv6xxx_init_softc(sc)) != 0) {
		ssv6xxx_deinit_softc(sc);
		ssv6xxx_hci_deregister();
		kfree(sh);
		return error;
	}
	if ((error = ssv6xxx_init_hw(sc->sh)) != 0) {
		ssv6xxx_deinit_hw(sc);
		ssv6xxx_deinit_softc(sc);
		ssv6xxx_hci_deregister();
		kfree(sh);
		return error;
	}
	if ((error = ieee80211_register_hw(hw)) != 0) {
		dev_err(sc->dev, "Failed to register ieee80211 wireless device. ret=%d.\n", error);
		ssv6xxx_deinit_hw(sc);
		ssv6xxx_deinit_softc(sc);
		ssv6xxx_hci_deregister();
		kfree(sh);
		return error;
	}
#ifdef CONFIG_SSV6XXX_DEBUGFS
	ssv6xxx_init_debugfs(sc, name);
#endif
	return 0;
}

static void ssv6xxx_deinit_device(struct ssv_softc *sc)
{
	dev_dbg(sc->dev, "%s(): \n", __FUNCTION__);
#ifdef CONFIG_SSV6XXX_DEBUGFS
	ssv6xxx_deinit_debugfs(sc);
#endif
	ssv6xxx_rf_disable(sc->sh);
	ieee80211_unregister_hw(sc->hw);
	ssv6xxx_deinit_hw(sc);
	ssv6xxx_deinit_softc(sc);
	ssv6xxx_hci_deregister();
	kfree(sc->sh);
}

extern struct ieee80211_ops ssv6200_ops;
int ssv6xxx_dev_probe(struct platform_device *pdev)
{
#ifdef CONFIG_SSV6200_CLI_ENABLE
	extern struct ssv_softc *ssv_dbg_sc;
#endif
#ifdef CONFIG_SSV_SMARTLINK
	extern struct ssv_softc *ssv_smartlink_sc;
#endif
	struct ssv_softc *softc;
	struct ieee80211_hw *hw;
	int ret;
	if (!pdev->dev.platform_data) {
		dev_err(&pdev->dev, "no platform data specified!\n");
		return -EINVAL;
	}
	hw = ieee80211_alloc_hw(sizeof(struct ssv_softc), &ssv6200_ops);
	if (hw == NULL) {
		dev_err(&pdev->dev, "Could not allocate memory for ieee80211 wireless device\n");
		return -ENOMEM;
	}
	SET_IEEE80211_DEV(hw, &pdev->dev);
	dev_set_drvdata(&pdev->dev, hw);
	memset((void *)hw->priv, 0, sizeof(struct ssv_softc));
	softc = hw->priv;
	softc->hw = hw;
	softc->dev = &pdev->dev;
	//SET_IEEE80211_PERM_ADDR(hw, (const u8 *)&softc->sh->maddr[0]);
	ret = ssv6xxx_init_device(softc, pdev->name);
	if (ret) {
		dev_err(&pdev->dev, "Failed to initialize device\n");
		ieee80211_free_hw(hw);
		return ret;
	}
#ifdef CONFIG_SSV6200_CLI_ENABLE
	ssv_dbg_sc = softc;
#endif
#ifdef CONFIG_SSV_SMARTLINK
	ssv_smartlink_sc = softc;
#endif
	wiphy_info(hw->wiphy, "%s\n", "SSV6200 of South Silicon Valley");
	return 0;
}

EXPORT_SYMBOL(ssv6xxx_dev_probe);
int ssv6xxx_dev_remove(struct platform_device *pdev)
{
	struct ieee80211_hw *hw = dev_get_drvdata(&pdev->dev);
	struct ssv_softc *softc = hw->priv;
	dev_dbg(&pdev->dev, "ssv6xxx_dev_remove(): pdev=%p, hw=%p\n", pdev, hw);
	ssv6xxx_deinit_device(softc);
	dev_dbg(&pdev->dev, "ieee80211_free_hw(): \n");
	ieee80211_free_hw(hw);
	dev_info(&pdev->dev, "driver unloaded\n");
	return 0;
}

EXPORT_SYMBOL(ssv6xxx_dev_remove);
static const struct platform_device_id ssv6xxx_id_table[] = {
	{
	 .name = "ssv6200",
	 .driver_data = 0x00,
	 },
	{},
};

MODULE_DEVICE_TABLE(platform, ssv6xxx_id_table);
static struct platform_driver ssv6xxx_driver = {
	.probe = ssv6xxx_dev_probe,
	.remove = ssv6xxx_dev_remove,
	.id_table = ssv6xxx_id_table,
	.driver = {
		   .name = "SSV WLAN driver",
		   .owner = THIS_MODULE,
		   }
};

int ssv6xxx_init(void)
{
	extern void *ssv_dbg_phy_table;
	extern u32 ssv_dbg_phy_len;
	extern void *ssv_dbg_rf_table;
	extern u32 ssv_dbg_rf_len;
	ssv_dbg_phy_table = (void *)ssv6200_phy_tbl;
	ssv_dbg_phy_len =
	    sizeof(ssv6200_phy_tbl) / sizeof(struct ssv6xxx_dev_table);
	ssv_dbg_rf_table = (void *)ssv6200_rf_tbl;
	ssv_dbg_rf_len =
	    sizeof(ssv6200_rf_tbl) / sizeof(struct ssv6xxx_dev_table);
	return platform_driver_register(&ssv6xxx_driver);
}

void ssv6xxx_exit(void)
{
	platform_driver_unregister(&ssv6xxx_driver);
}

EXPORT_SYMBOL(ssv6xxx_init);
EXPORT_SYMBOL(ssv6xxx_exit);
