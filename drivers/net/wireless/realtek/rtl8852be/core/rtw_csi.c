
/******************************************************************************
 *
 * Copyright(c) 2007 - 2022 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#define _RTW_CSI_C_

#include <drv_types.h>

#ifdef CONFIG_RTW_CSI_CHANNEL_INFO

#ifdef CONFIG_RTW_CSI_NETLINK
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

#define NETLINK_CSI_FAMILY 19

static void csi_nl_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh;
	int pid;
	struct sk_buff *skb_out;
	int msg_size;
	char *msg = "Hello from kernel";
	int res;

	RTW_INFO("Entering: %s\n", __FUNCTION__);

	msg_size = strlen(msg);

	nlh = (struct nlmsghdr *)skb->data;
	RTW_INFO("Kernel: Netlink received msg payload:%s\n", (char *)nlmsg_data(nlh));
#if 1
	pid = nlh->nlmsg_pid; /*pid of sending process */
#else
	usr_pid = nlh->nlmsg_pid; /*pid of sending process */
#endif

#if 0//no send back
	skb_out = nlmsg_new(msg_size, 0);
	if (!skb_out) {
		RTW_ERR("Failed to allocate new skb\n");
		return;
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	strncpy(nlmsg_data(nlh), msg, msg_size);

	res = nlmsg_unicast(nl_sk, skb_out, pid);
	if (res < 0)
		RTW_ERR("Error while sending bak to user\n");
#endif
}

int rtw_csi_nl_init(struct dvobj_priv *dvobj)
{
	struct sock *nl_sk;
	struct netlink_kernel_cfg cfg;

	RTW_INFO("Entering: %s\n", __func__);

	nl_sk = NULL;
	dvobj->csi_nl_sk = NULL;
	cfg.input = csi_nl_recv_msg;

	nl_sk = netlink_kernel_create(&init_net, NETLINK_CSI_FAMILY, &cfg);
	if (!nl_sk) {
		RTW_ERR("%s: Error creating socket.\n", __func__);
		return -10;
	}
	dvobj->csi_nl_sk = nl_sk;

	return 0;
}

void rtw_csi_nl_exit(struct dvobj_priv *dvobj)
{
	RTW_INFO("Exiting: %s\n", __func__);
	netlink_kernel_release(dvobj->csi_nl_sk);
	dvobj->csi_nl_sk = NULL;
}

#endif /*CONFIG_RTW_CSI_NETLINK*/

#ifdef CONFIG_CSI_TIMER_POLLING

/******CSI header for OWI******/

static int ow_pid = 123;

#define IEEE80211_ADDR_LEN  6

struct ow_csi_netlink_header {
    u8    phynetdevidx;
    u32   sequence_number;
    u16   fragment_index;
} __attribute__ ((__packed__));

/*
  src_macaddr         received frame source MAC address
  csi_capture_status  1 - CSI capture successful; 0 - CSI capture not successful
  csi_timestamp       received frame TSF timestamp
  csi_nc	      Nrx
  csi_nr	      Nsts
  csi_bw	      received frame bandwidth
  csi_rx_rate	      received frame date rate
  csi_dump_length     Length of the CSI dump, it excludes the header size
*/
struct ow_csi_frame_header {
	u32 rsvd1[4];
	u8 src_macaddr[IEEE80211_ADDR_LEN];
	u8 csi_capture_status;
	u8 rsvd2[13];
	u32 csi_timestamp;
	u8 csi_nc;
	u8 csi_nr;
	u8 csi_bw;
	u16 csi_rx_rate;
	u8 csi_rssi[2];
	u8 csi_subc_num;
	u8 csi_tone_bit;
	u32 csi_data_length;
};
/******************************/


#define CSI_MSG_LEN 1000
#define CSI_POLL_TIME 30 /*msec*/
static u8 msg_buf[CSI_MSG_LEN] = {0};
static u8 buff[CSI_MSG_LEN] = {0};

void rtw_csi_poll_timer_set(struct dvobj_priv *dvobj, u32 delay)
{
	_timer *timer = NULL;

	timer = &dvobj->csi_poll_timer;
	_set_timer(timer, delay);
}

void rtw_csi_poll_timer_cancel(struct dvobj_priv *dvobj)
{
	_timer *timer = NULL;

	timer = &dvobj->csi_poll_timer;
	_cancel_timer_ex(timer);
	timer->function = NULL;
}

void rtw_nl_send_csi(struct dvobj_priv* dvobj, u8* buffer, u32 buf_len, struct csi_header_t *csi_header)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size = 0;
	//char *msg = "NL msg send from kernel";
	//char *msg_send;
	int res;
	struct ow_csi_netlink_header ow_csi_nlh;
	struct ow_csi_frame_header ow_csi_h;

	if ((buf_len + sizeof(struct csi_header_t)) > CSI_MSG_LEN) {
		RTW_ERR("%s: CSI Buffer len is not enought.\n", __func__);
		return;
	}
	else {
		//RTW_INFO("%s: CSI total len = %d\n", __func__, buf_len + sizeof(struct csi_header_t));
	}

	/* Fill ow_csi_netlink_header */
	RTW_DBG("%s: len=%ld\n", __func__, sizeof(struct ow_csi_netlink_header));
	_rtw_memset(&ow_csi_nlh, 0, sizeof(struct ow_csi_netlink_header));
	ow_csi_nlh.phynetdevidx = 0; // ToDo
	ow_csi_nlh.sequence_number = 0; //ToDo
	ow_csi_nlh.fragment_index = 0;

	/* Fill ow_csi_frame_header */
	RTW_DBG("%s: len=%ld\n", __func__, sizeof(struct ow_csi_frame_header));
	_rtw_memset(&ow_csi_h, 0, sizeof(struct ow_csi_frame_header));
	_rtw_memcpy(ow_csi_h.src_macaddr, csi_header->mac_addr, 6);
	ow_csi_h.csi_capture_status = csi_header->csi_valid;
	ow_csi_h.csi_timestamp = csi_header->hw_assigned_timestamp;
	ow_csi_h.csi_data_length = csi_header->csi_data_length;
	ow_csi_h.csi_nc = csi_header->nc;
	ow_csi_h.csi_nr = csi_header->nr;
	ow_csi_h.csi_bw = csi_header->bandwidth;
	ow_csi_h.csi_rx_rate = csi_header->rx_data_rate;
	ow_csi_h.csi_subc_num = csi_header->num_sub_carrier;
	ow_csi_h.csi_tone_bit = csi_header->num_bit_per_tone;
	ow_csi_h.csi_rssi[0] = csi_header->rssi[0];
	ow_csi_h.csi_rssi[1] = csi_header->rssi[1];

	/* Prepare NL msg */
	_rtw_memcpy(msg_buf, &ow_csi_nlh, sizeof(struct ow_csi_netlink_header));
	msg_size += sizeof(struct ow_csi_netlink_header);

	_rtw_memcpy(msg_buf + msg_size, &ow_csi_h, sizeof(struct ow_csi_frame_header));
	msg_size += sizeof(struct ow_csi_frame_header);

	/* Fill csi_raw */
#if 1
	RTW_DBG(":%s: len=%d\n", __func__, csi_header->csi_data_length);
	_rtw_memcpy(msg_buf + msg_size, buffer, csi_header->csi_data_length);
	msg_size += csi_header->csi_data_length;
#else
	RTW_DBG(":%s: len=%d\n", __func__, buf_len);
	_rtw_memcpy(msg_buf + msg_size, buffer, buf_len);
	msg_size += buf_len;
#endif
	RTW_DBG("%s: msg_size = %d\n", __func__, msg_size);

	skb_out = nlmsg_new(msg_size, 0);

	if (!skb_out) {
		RTW_ERR("%s: Failed to allocate new skb\n", __func__);
		return;
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */

	_rtw_memcpy(nlmsg_data(nlh), msg_buf, msg_size);

	if (!dvobj->csi_nl_sk) {
		RTW_ERR("%s fail: csi_nl_sk is NULL\n", __func__);
		return;
	}

	res = nlmsg_unicast(dvobj->csi_nl_sk, skb_out, ow_pid);
	if (res < 0) {
		if (printk_ratelimit())
			RTW_ERR("%s: Error while sending bak to user\n", __func__);
	}
}

void rtw_csi_poll_timeout_handler(void *FunctionContext)
{
	struct dvobj_priv *d;
	struct csi_header_t csi_header;
	void *phl;
	u32 buf_len;
	enum rtw_phl_status status = RTW_PHL_STATUS_FAILURE;
	int i;

	d = (struct dvobj_priv *)FunctionContext;
	phl = GET_PHL_INFO(d);

	rtw_csi_poll_timer_set(d, CSI_POLL_TIME);

	_rtw_memset(&csi_header, 0, sizeof(csi_header));
	status = rtw_phl_query_chan_info(phl, CSI_MSG_LEN, buff, &buf_len, &csi_header);
	if (status == RTW_PHL_STATUS_SUCCESS) {

		/* send to user app by NL */
		rtw_nl_send_csi(d, buff, buf_len, &csi_header);

#if 0 //debug dump
		//RTW_DUMP_SEL(m, buff, buf_len);
		RTW_INFO("csi_valid=%d\n", csi_header.csi_valid);
		RTW_INFO("csi_data_length=%d\n", csi_header.csi_data_length);
		RTW_INFO("rxsc=%d\n", csi_header.rxsc);
		RTW_INFO("nc=%d\n", csi_header.nc);
		RTW_INFO("nr=%d\n", csi_header.nr);
		RTW_INFO("avg_idle_noise_pwr=%d\n", csi_header.avg_idle_noise_pwr);
		RTW_INFO("evm[0]=%d\n", csi_header.evm[0]);
		RTW_INFO("evm[1]=%d\n", csi_header.evm[1]);
		RTW_INFO("rssi[0]=%d%%\n", csi_header.rssi[0] >> 1);
		RTW_INFO("rssi[1]=%d%%\n", csi_header.rssi[1] >> 1);
		RTW_INFO("timestamp=%u\n", csi_header.hw_assigned_timestamp);
		RTW_INFO("rx_data_rate=%d\n", csi_header.rx_data_rate);
		RTW_INFO("channel=%d\n", csi_header.channel);
		RTW_INFO("bandwidth=%d\n", csi_header.bandwidth);
		RTW_INFO("num_sub_carrier=%d\n", csi_header.num_sub_carrier);
		RTW_INFO("num_bit_per_tone=%d\n", csi_header.num_bit_per_tone);
		printk(KERN_CONT "srcmac= ");
		for (i = 0; i < 6; i++)
			printk(KERN_CONT "%x ", csi_header.mac_addr[i]);
		RTW_INFO("\n");

#if 0
		RTW_INFO("CSI RAW(len=%d):\n", buf_len);
		for (i = 0; i < buf_len; i++) {
			printk(KERN_CONT "%x ", buff[i]);
			if ((i%8)==7)
				printk(KERN_CONT "\n");
		}
		printk("\n");
#endif
#endif
	}
}

void rtw_csi_poll_init(struct dvobj_priv *dvobj)
{
	_timer *timer = &dvobj->csi_poll_timer;

	if (timer->function != NULL) {
		RTW_INFO("CSI polling timer has been init.\n");
		return;
	}

	rtw_init_timer(timer, rtw_csi_poll_timeout_handler, dvobj);
	rtw_csi_poll_timer_set(dvobj, CSI_POLL_TIME);
	RTW_INFO("CSI polling timer init!\n");
}
#endif /*CONFIG_CSI_TIMER_POLLING*/
#endif /*CONFIG_RTW_CSI_CHANNEL_INFO*/

