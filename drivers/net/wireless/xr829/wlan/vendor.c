/*
 * Main code of XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*Linux version 3.4.0 compilation*/
#include <net/genetlink.h>
#include <net/cfg80211.h>
#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/timer.h>

#include "vendor.h"
#include "../umac/ieee80211_i.h"
#include "xradio.h"


static int xradio_vendor_do_acs(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

static int xradio_vendor_get_features(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

static int xradio_vendor_start_mkeep_alive(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

static int xradio_vendor_stop_mkeep_alive(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len);

static const struct wiphy_vendor_command xradio_nl80211_vendor_commands[] = {
	[NL80211_VENDOR_SUBCMD_DO_ACS_INDEX] = {
		.info.vendor_id = XRADIO_NL80211_VENDOR_ID,
		.info.subcmd = NL80211_VENDOR_SUBCMD_DO_ACS,
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = xradio_vendor_do_acs
	},
	[NL80211_VENDOR_SUBCMD_GET_FEATURES_INDEX] = {
		.info.vendor_id = XRADIO_NL80211_VENDOR_ID,
		.info.subcmd = NL80211_VENDOR_SUBCMD_GET_FEATURES,
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = xradio_vendor_get_features
	},
	[NL80211_WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE_INDEX] = {
		.info.vendor_id = XRADIO_NL80211_ANDROID_ID,
		.info.subcmd = NL80211_WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE,
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = xradio_vendor_start_mkeep_alive
	},
	[NL80211_WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE_INDEX] = {
		.info.vendor_id = XRADIO_NL80211_ANDROID_ID,
		.info.subcmd = NL80211_WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE,
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV |
			 WIPHY_VENDOR_CMD_NEED_RUNNING,
		.doit = xradio_vendor_stop_mkeep_alive
	},
};

/* vendor specific events */
static const struct nl80211_vendor_cmd_info xradio_nl80211_vendor_events[] = {
	[NL80211_VENDOR_SUBCMD_DO_ACS_INDEX] = {
			.vendor_id = XRADIO_NL80211_VENDOR_ID,
			.subcmd = NL80211_VENDOR_SUBCMD_DO_ACS
	},
};

static const struct nla_policy
xradio_cfg80211_do_acs_policy[WLAN_VENDOR_ATTR_ACS_MAX+1] = {
	[WLAN_VENDOR_ATTR_ACS_HW_MODE] = { .type = NLA_U8 },
	[WLAN_VENDOR_ATTR_ACS_HT_ENABLED] = { .type = NLA_FLAG },
	[WLAN_VENDOR_ATTR_ACS_HT40_ENABLED] = { .type = NLA_FLAG },
	[WLAN_VENDOR_ATTR_ACS_VHT_ENABLED] = { .type = NLA_FLAG },
	[WLAN_VENDOR_ATTR_ACS_CHWIDTH] = { .type = NLA_U16 },
	[WLAN_VENDOR_ATTR_ACS_CH_LIST] = { .type = NLA_UNSPEC },
	[WLAN_VENDOR_ATTR_ACS_FREQ_LIST] = { .type = NLA_UNSPEC },
};

static unsigned int xradio_acs_calc_channel(struct wiphy *wiphy)
{
	/*TODO: Find a better ACS algorithm*/
	return 5;
}


static void xradio_acs_report_channel(struct wiphy *wiphy, struct wireless_dev *wdev)
{

	struct sk_buff *vendor_event;
	int ret_val;
	struct nlattr *nla;
	u8 channel = xradio_acs_calc_channel(wiphy);

	vendor_event = cfg80211_vendor_event_alloc(wiphy, NULL,
						   2 + 4 + NLMSG_HDRLEN,
						   NL80211_VENDOR_SUBCMD_DO_ACS_INDEX,
						   GFP_KERNEL);
	if (!vendor_event) {
		printk("cfg80211_vendor_event_alloc failed\n");
		return;
	}

	/* Send the IF INDEX to differentiate the ACS event for each interface
	 * TODO: To be update once cfg80211 APIs are updated to accept if_index
	 */
	nla_nest_cancel(vendor_event, ((void **)vendor_event->cb)[2]);

	ret_val = nla_put_u32(vendor_event, NL80211_ATTR_IFINDEX,
				  wdev->netdev->ifindex);
	if (ret_val) {
		printk("NL80211_ATTR_IFINDEX put fail\n");
		kfree_skb(vendor_event);
		return;
	}

	nla = nla_nest_start(vendor_event, NL80211_ATTR_VENDOR_DATA);
	((void **)vendor_event->cb)[2] = nla;

	/* channel indices used by fw are zero based and those used upper
	 * layers are 1 based: must add 1
	 */
	ret_val = nla_put_u8(vendor_event,
				 WLAN_VENDOR_ATTR_ACS_PRIMARY_CHANNEL,
				 channel + 1);
	if (ret_val) {
		printk(
			"WLAN_VENDOR_ATTR_ACS_PRIMARY_CHANNEL put fail\n");
		kfree_skb(vendor_event);
		return;
	}

	/* must report secondary channel always, 0 is harmless */
	ret_val = nla_put_u8(vendor_event,
				 WLAN_VENDOR_ATTR_ACS_SECONDARY_CHANNEL, 0);
	if (ret_val) {
		printk(
			"WLAN_VENDOR_ATTR_ACS_SECONDARY_CHANNEL put fail\n");
		kfree_skb(vendor_event);
		return;
	}

	cfg80211_vendor_event(vendor_event, GFP_KERNEL);

}

static int xradio_vendor_do_acs(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	struct sk_buff *temp_skbuff;
	int res = 0;
	//u8 hw_mode;
	struct nlattr *tb[WLAN_VENDOR_ATTR_ACS_MAX + 1];
	//struct ieee80211_channel reg_channels[ARRAY_SIZE(xradio_2ghz_chantable)];
	//int num_channels;


	res = nla_parse(tb, WLAN_VENDOR_ATTR_ACS_MAX, data, data_len,
						xradio_cfg80211_do_acs_policy);
	if (res) {
		printk("Invalid ATTR");
		goto out;
	}

/*
	if (!tb[WLAN_VENDOR_ATTR_ACS_HW_MODE]) {
		printk("%s: Attr hw_mode failed\n", __func__);
		goto out;
	}

	hw_mode = nla_get_u8(tb[WLAN_VENDOR_ATTR_ACS_HW_MODE]);
*/


	if (tb[WLAN_VENDOR_ATTR_ACS_CH_LIST]) {

	} else if (tb[WLAN_VENDOR_ATTR_ACS_FREQ_LIST]) {

	} else {
		res = -EINVAL;
		goto out;
	}


	xradio_acs_report_channel(wiphy, wdev);


out:
	if (0 == res) {
		temp_skbuff = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
								  NLMSG_HDRLEN);
		if (temp_skbuff)
			return cfg80211_vendor_cmd_reply(temp_skbuff);
	}

	return res;
}

#define NUM_BITS_IN_BYTE	8
void xradio_vendor_set_features(uint8_t *feature_flags, uint8_t feature)
{
	uint32_t index;
	uint8_t bit_mask;

	index = feature / NUM_BITS_IN_BYTE;
	bit_mask = 1 << (feature % NUM_BITS_IN_BYTE);
	feature_flags[index] |= bit_mask;
}

static int xradio_vendor_get_features(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	struct sk_buff *msg;
	uint8_t feature_flags[(NUM_WLAN_VENDOR_FEATURES + 7) / 8] = {0};

	xradio_vendor_set_features(feature_flags,
					WLAN_VENDOR_FEATURE_SUPPORT_HW_MODE_ANY);

	msg = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(feature_flags) +
									NLMSG_HDRLEN);
	if (!msg)
		return -ENOMEM;

	if (nla_put(msg,
		WLAN_VENDOR_ATTR_FEATURE_FLAGS,
		sizeof(feature_flags), feature_flags))
		goto nla_put_failure;

	return cfg80211_vendor_cmd_reply(msg);

nla_put_failure:
	kfree_skb(msg);
	return -EINVAL;

}

struct keepalivenode {
	struct list_head list;
	u8 *pkt;
	u16 pkt_len;
	u8 id;
	u32 period_msec;
	unsigned long next_jiffies;
	struct net_device *netdev;
};

static struct list_head keepalivelist;
static struct timer_list keepalivetimer;

static void keep_alive_send(struct ieee80211_local *local, struct net_device *dev, u8 *frame_8023,
						u16 pkt_len)
{
	struct sk_buff *skb;
	skb = dev_alloc_skb(local->hw.extra_tx_headroom + pkt_len);
	if (!skb)
		return;

	skb_reserve(skb, local->hw.extra_tx_headroom);

	skb_put(skb, pkt_len);
	memcpy(skb->data, frame_8023, pkt_len);

	mac80211_subif_start_xmit(skb, dev);

}


static int
keep_alive_cmp(void *priv,
	struct list_head *a, struct list_head *b)
{
	struct keepalivenode *ap = container_of(a, struct keepalivenode, list);
	struct keepalivenode *bp = container_of(b, struct keepalivenode, list);
	long diff;

	diff = ap->next_jiffies - bp->next_jiffies;

	if (diff < 0)
		return -1;
	if (diff > 0)
		return 1;
	return 0;
}

static void keep_alive_run(unsigned long data)
{
	struct keepalivenode *node;
	struct ieee80211_local *local = (struct ieee80211_local *)data;

	if (list_empty(&keepalivelist))
		return;

	list_for_each_entry(node, &keepalivelist, list) {
		if (time_before(jiffies, node->next_jiffies))
			break;
		keep_alive_send(local, node->netdev, node->pkt, node->pkt_len);
		node->next_jiffies += msecs_to_jiffies(node->period_msec);
	}
	list_sort(NULL, &keepalivelist, keep_alive_cmp);

	//reset timer;
	node = container_of(keepalivelist.next, struct keepalivenode, list);
	mod_timer(&keepalivetimer, node->next_jiffies);
}

static void keep_alive_queue_clear(void)
{
	struct keepalivenode *node;
	struct keepalivenode *tmp;

	if (list_empty(&keepalivelist))
		return;

	list_for_each_entry_safe(node, tmp, &keepalivelist, list) {
		if (node->pkt != NULL) {
			kfree(node->pkt);
			node->pkt = NULL;
		}
		list_del(&node->list);
		kfree(node);
	}

	//delete timer
	del_timer(&keepalivetimer);
}

static int keep_alive_queue_put(u8 *dst_mac, u8 *src_mac, struct net_device *netdev,
				u8 *ip_pkt, u16 ip_pkt_len, u32 period_msec, u8 mkeep_alive_id)
{
	u8 *frame_8023;
	int len = 0;
	int ret = 0;
	struct keepalivenode *node;

	frame_8023 = kzalloc(ip_pkt_len + 14, GFP_KERNEL);
	if (frame_8023 == NULL) {
		ret = -ENOMEM;
		printk("Failed to allocate mem for frame_8023\n");
		return ret;
	}

	node = (struct keepalivenode *)kzalloc(sizeof(struct keepalivenode), GFP_KERNEL);
	if (node == NULL) {
		kfree(frame_8023);
		ret = -ENOMEM;
		printk("Failed to allocate mem for keepalivenode\n");
		return ret;
	}

	/*
	 * This is the IP packet, add 14 bytes Ethernet (802.3) header
	 * ------------------------------------------------------------
	 * | 14 bytes Ethernet (802.3) header | IP header and payload |
	 * ------------------------------------------------------------
	 */

	/* Mapping dest mac addr */
	memcpy(&frame_8023[len], dst_mac, ETH_ALEN);
	len += ETH_ALEN;

	/* Mapping src mac addr */
	memcpy(&frame_8023[len], src_mac, ETH_ALEN);
	len += ETH_ALEN;

	/* Mapping Ethernet type (ETHERTYPE_IP: 0x0800) */
	frame_8023[len] = 0x08;
	frame_8023[len+1] = 0x00;
	len += 2;

	/* Mapping IP pkt */
	memcpy(&frame_8023[len], ip_pkt, ip_pkt_len);
	len += ip_pkt_len;

	node->pkt = frame_8023;
	node->pkt_len = len;
	node->period_msec = period_msec;
	node->id = mkeep_alive_id;
	node->next_jiffies = jiffies;
	node->netdev = netdev;

	list_add(&node->list, &keepalivelist);

	//run timer;
	mod_timer(&keepalivetimer, jiffies);

	return ret;

}

static void keep_alive_queue_remove(u8 mkeep_alive_id)
{
	struct keepalivenode *node;
	struct keepalivenode *tmp;

	list_for_each_entry_safe(node, tmp, &keepalivelist, list) {
		if (node->id == mkeep_alive_id) {
			if (node->pkt != NULL) {
				kfree(node->pkt);
				node->pkt = NULL;
			}
			list_del(&node->list);
			kfree(node);
		}
	}
	if (list_empty(&keepalivelist)) {
		//delete timer;
		del_timer(&keepalivetimer);
	}
}

static int xradio_vendor_start_mkeep_alive(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	/* max size of IP packet for keep alive */
	const int MKEEP_ALIVE_IP_PKT_MAX = 256;

	int ret = 0, rem, type;
	u8 mkeep_alive_id = 0;
	u8 *ip_pkt = NULL;
	u16 ip_pkt_len = 0;
	u8 src_mac[ETH_ALEN];
	u8 dst_mac[ETH_ALEN];
	u32 period_msec = 0;
	const struct nlattr *iter;
	struct ieee80211_sub_if_data *sdata = IEEE80211_WDEV_TO_SUB_IF(wdev);
	struct xradio_vif *priv = xrwl_get_vif_from_ieee80211(&sdata->vif);

	nla_for_each_attr(iter, data, data_len, rem) {
		type = nla_type(iter);
		switch (type) {
		case MKEEP_ALIVE_ATTRIBUTE_ID:
			mkeep_alive_id = nla_get_u8(iter);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
			ip_pkt_len = nla_get_u16(iter);
			if (ip_pkt_len > MKEEP_ALIVE_IP_PKT_MAX) {
				ret = -EINVAL;
				goto exit;
			}
			break;
		case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
			if (!ip_pkt_len) {
				ret = -EINVAL;
				printk("ip packet length is 0\n");
				goto exit;
			}
			ip_pkt = (u8 *)kzalloc(ip_pkt_len, GFP_KERNEL);
			if (ip_pkt == NULL) {
				ret = -ENOMEM;
				printk("Failed to allocate mem for ip packet\n");
				goto exit;
			}
			memcpy(ip_pkt, (u8 *)nla_data(iter), ip_pkt_len);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
			memcpy(src_mac, nla_data(iter), ETH_ALEN);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
			memcpy(dst_mac, nla_data(iter), ETH_ALEN);
			break;
		case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
			period_msec = nla_get_u32(iter);
			break;
		default:
			printk("Unknown type: %d\n", type);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (ip_pkt == NULL) {
		ret = -EINVAL;
		printk(("ip packet is NULL\n"));
		goto exit;
	}

	if (priv->join_status == XRADIO_JOIN_STATUS_PASSIVE) {
		ret = -EINVAL;
		goto exit;
	}

	ret = keep_alive_queue_put(dst_mac, src_mac, wdev->netdev,
			ip_pkt, ip_pkt_len, period_msec, mkeep_alive_id);
	if (ret) {
		ret = -EINVAL;
		goto exit;
	}

exit:
	if (ip_pkt) {
		kfree(ip_pkt);
	}

	return ret;
}

static int xradio_vendor_stop_mkeep_alive(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 const void *data, int data_len)
{
	int ret = 0, rem, type;
	u8 mkeep_alive_id = 0;
	const struct nlattr *iter;

	nla_for_each_attr(iter, data, data_len, rem) {
		type = nla_type(iter);
		switch (type) {
		case MKEEP_ALIVE_ATTRIBUTE_ID:
			mkeep_alive_id = nla_get_u8(iter);
			break;
		default:
			printk("Unknown type: %d\n", type);
			ret = -EINVAL;
			break;
		}
	}
	if (ret < 0) {
		printk("stop_mkeep_alive is failed ret: %d\n", ret);
		goto exit;
	}

	keep_alive_queue_remove(mkeep_alive_id);

exit:
	return ret;

}

void xradio_vendor_close_mkeep_alive(void)
{
	keep_alive_queue_clear();
}

void xradio_vendor_init(struct wiphy *wiphy)
{
	struct ieee80211_local *local = wiphy_priv(wiphy);

	wiphy->n_vendor_commands = ARRAY_SIZE(xradio_nl80211_vendor_commands);
	wiphy->vendor_commands = xradio_nl80211_vendor_commands;
	wiphy->n_vendor_events = ARRAY_SIZE(xradio_nl80211_vendor_events);
	wiphy->vendor_events = xradio_nl80211_vendor_events;

	INIT_LIST_HEAD(&keepalivelist);
	setup_timer(&keepalivetimer, keep_alive_run,
			(unsigned long)local);

	return;
}

