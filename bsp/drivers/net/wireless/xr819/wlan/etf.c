/*
 * ETF interfaces for XRadio drivers
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef XRADIO_ETF_C
#define XRADIO_ETF_C
#include <linux/module.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/firmware.h>
#include <uapi/linux/sched/types.h>

#include "etf.h"
#include "xradio.h"
#include "sbus.h"

/********************* interfaces of adapter layer **********************/
static struct xradio_adapter adapter_priv;

#if (ETF_QUEUEMODE)
static void adapter_rec_handler(struct sk_buff *__skb)
{
	int i = 0;
	struct adapter_item *item;
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	do {
		spin_lock_bh(&adapter_priv.recv_lock);
		if (likely(!list_empty(&adapter_priv.rx_pool))) {
			item = list_first_entry(&adapter_priv.rx_pool, struct adapter_item, head);
			item->skb = (void *)skb_get(__skb);
			list_move_tail(&item->head, &adapter_priv.rx_queue);
			spin_unlock_bh(&adapter_priv.recv_lock);
			etf_printk(XRADIO_DBG_NIY, "%s recv msg to rx_queue!\n", __func__);
			break;
		} else if (++i > ETF_QUEUE_TIMEOUT) { /* about 2s.*/
			spin_unlock_bh(&adapter_priv.recv_lock);
			etf_printk(XRADIO_DBG_ERROR,
				"%s driver is process timeout, drop msg!\n", __func__);
			return;
		} else {
			etf_printk(XRADIO_DBG_NIY,
				"%s rx_pool is empty, wait %d!ms\n", __func__, i);
		}
		spin_unlock_bh(&adapter_priv.recv_lock);
		msleep(i);
	} while (1);

	if (atomic_add_return(1, &adapter_priv.rx_cnt) == 1)
		up(&adapter_priv.proc_sem);
	return;
}
#else
static void adapter_rec_handler(struct sk_buff *skb)
{
	struct nlmsghdr *nlhdr = (struct nlmsghdr *)skb->data;
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
		etf_printk(XRADIO_DBG_ERROR, "Corrupt netlink msg!\n");
	} else {
		int len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
		adapter_priv.send_pid = nlhdr->nlmsg_pid;
		adapter_priv.msg_len  = min(ADAPTER_RX_BUF_LEN, len);
		memcpy(adapter_priv.msg_buf, NLMSG_DATA(nlhdr), adapter_priv.msg_len);
		if (len != adapter_priv.msg_len)
			etf_printk(XRADIO_DBG_WARN, "%s recv len(%d), proc len=%d!\n",
					__func__, len, adapter_priv.msg_len);
		else
			etf_printk(XRADIO_DBG_NIY, "%s recv msg len(%d)!\n",
					__func__, adapter_priv.msg_len);
		if (adapter_priv.handler)
			adapter_priv.handler(adapter_priv.msg_buf, adapter_priv.msg_len);

	}
}
#endif

static int adapter_init_msgbuf(void)
{
#if (ETF_QUEUEMODE)
	int i = 0;
#endif
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	if (adapter_priv.msg_buf) {
		etf_printk(XRADIO_DBG_ERROR, "%s msgbuf already init!\n", __func__);
		return -1;
	}

#if (ETF_QUEUEMODE)
	atomic_set(&adapter_priv.rx_cnt, 0);
	INIT_LIST_HEAD(&adapter_priv.rx_queue);
	INIT_LIST_HEAD(&adapter_priv.rx_pool);
	for (i = 0; i < ADAPTER_ITEM_MAX; i++)
		list_add_tail(&adapter_priv.rx_items[i].head, &adapter_priv.rx_pool);
#endif

	adapter_priv.msg_buf  = xr_kzalloc(ADAPTER_RX_BUF_LEN, false);
	if (!adapter_priv.msg_buf)
		return -ENOMEM;
	return 0;
}

static void adapter_deinit_msgbuf(void)
{
#if (ETF_QUEUEMODE)
	struct  adapter_item *item;
#endif
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	if (adapter_priv.msg_buf) {
		kfree(adapter_priv.msg_buf);
		adapter_priv.msg_buf = NULL;
	}
#if (ETF_QUEUEMODE)
	spin_lock_bh(&adapter_priv.recv_lock);
	list_for_each_entry(item, &adapter_priv.rx_queue, head) {
		if (item->skb) {
			dev_kfree_skb((struct sk_buff *)item->skb);
			item->skb = NULL;
		}
	}
	spin_unlock_bh(&adapter_priv.recv_lock);
#endif
}

#if (ETF_QUEUEMODE)
static int adapter_handle_rx_queue(void)
{
	int len = 0;
	struct nlmsghdr *nlhdr = NULL;
	struct adapter_item *item;
	bool bHandler = false;
	while (1) {
		spin_lock_bh(&adapter_priv.recv_lock);
		if (!list_empty(&adapter_priv.rx_queue)) {
			item = list_first_entry(&adapter_priv.rx_queue, struct adapter_item, head);
			if (item->skb) {
				struct sk_buff *skb = (struct sk_buff *)(item->skb);
				nlhdr = (struct nlmsghdr *)skb->data;
				if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
					etf_printk(XRADIO_DBG_ERROR, "Corrupt netlink msg!\n");
				} else {
					len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
					adapter_priv.send_pid = nlhdr->nlmsg_pid;
					adapter_priv.msg_len  = min(ADAPTER_RX_BUF_LEN, len);
					memcpy(adapter_priv.msg_buf, NLMSG_DATA(nlhdr), len);
					bHandler = true;
					if (len != adapter_priv.msg_len)
						etf_printk(XRADIO_DBG_WARN, "%s recv len(%d), proc len=%d!\n",
								__func__, len, adapter_priv.msg_len);
					else
						etf_printk(XRADIO_DBG_NIY, "%s recv msg len(%d)!\n",
								__func__, adapter_priv.msg_len);
				}
				dev_kfree_skb(skb);
				item->skb = NULL;
				list_move_tail(&item->head, &adapter_priv.rx_pool);
			} else {
				etf_printk(XRADIO_DBG_ERROR, "%s skb is NULL!\n", __func__);
			}
		} else {
			spin_unlock_bh(&adapter_priv.recv_lock);
			break;
		}
		spin_unlock_bh(&adapter_priv.recv_lock);
		if (bHandler && adapter_priv.handler) {
			adapter_priv.handler(adapter_priv.msg_buf, adapter_priv.msg_len);
			bHandler = false;
		}
	}
	return 0;
}

static int adapter_proc(void *param)
{
	int ret  = 0;
	int term = 0;
	int recv = 0;
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	for (;;) {
		ret = down_interruptible(&adapter_priv.proc_sem);
		recv = atomic_xchg(&adapter_priv.rx_cnt, 0);
		term = kthread_should_stop();
		etf_printk(XRADIO_DBG_NIY, "%s term=%d!\n", __func__, term);
		if (term || adapter_priv.exit || ret < 0) {
			break;
		}
		if (recv) {
			adapter_handle_rx_queue();
		}
	}
	etf_printk(XRADIO_DBG_NIY, "%s: exit!\n", __func__);
	return 0;
}
static int adapter_start_thread(void)
{
	int ret = 0;
	struct sched_param param = {.sched_priority = 100 };
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	adapter_priv.thread_tsk = NULL;
	adapter_priv.exit = 0;
	adapter_priv.thread_tsk = kthread_create(&adapter_proc, NULL, XRADIO_ADAPTER);
	if (IS_ERR(adapter_priv.thread_tsk)) {
		ret = PTR_ERR(adapter_priv.thread_tsk);
		adapter_priv.thread_tsk = NULL;
	} else {
		sched_setscheduler(adapter_priv.thread_tsk, SCHED_NORMAL, &param);
		wake_up_process(adapter_priv.thread_tsk);
	}
	return ret;
}
static void adapter_stop_thread(void)
{
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	if (adapter_priv.thread_tsk != NULL) {
		adapter_priv.exit = 1;
		up(&adapter_priv.proc_sem);
		kthread_stop(adapter_priv.thread_tsk);
		adapter_priv.thread_tsk = NULL;
	} else {
		etf_printk(XRADIO_DBG_WARN, "%s: thread_tsk is NULL!\n", __func__);
	}
}
#endif /* ETF_QUEUEMODE */

static int xradio_adapter_send(void *data, int len)
{
	int ret = 0;
	struct sk_buff  *skb = NULL;
	struct nlmsghdr *nlhdr = NULL;
	u8  *payload  = NULL;
	if (0 == len || NULL == data)
		return -EINVAL;

	skb = xr_alloc_skb(NLMSG_SPACE(len + 1));
	if (!skb) {
		etf_printk(XRADIO_DBG_WARN, "%s:xr_alloc_skb failed!\n", __func__);
		return -ENOMEM;
	}
	/* '\0' safe for strcpy, compat for old etf apk, may be removed in furture.*/
	nlhdr = nlmsg_put(skb, 0, 0, 0, len + 1, 0); /* (len+1) is payload */
	payload = (u8 *)NLMSG_DATA(nlhdr);
	memcpy(payload, data, len);
	payload[len] = '\0';

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	NETLINK_CB(skb).pid = 0;
#else
	NETLINK_CB(skb).portid = 0;
#endif
	NETLINK_CB(skb).dst_group = 0;

	down(&adapter_priv.send_lock);
	ret = netlink_unicast(adapter_priv.sock, skb, adapter_priv.send_pid, 0);
	if (ret < 0)
		etf_printk(XRADIO_DBG_ERROR, "%s:netlink_unicast failed(%d),pid=%d!\n",
					__func__, ret, adapter_priv.send_pid);
	else
		etf_printk(XRADIO_DBG_NIY, "%s:netlink_unicast len(%d),pid=%d!\n",
					__func__, ret, adapter_priv.send_pid);
	up(&adapter_priv.send_lock);

	return ret;
}

static int xradio_adapter_send_pkg(void *data1, int len1, void *data2, int len2)
{
	int ret = 0;
	struct sk_buff  *skb = NULL;
	struct nlmsghdr *nlhdr = NULL;
	u8  *payload  = NULL;
	int total_len = len1 + len2;
	if (len1 <= 0 || len2 < 0 || NULL == data1)
		return -EINVAL;

	skb = xr_alloc_skb(NLMSG_SPACE(total_len + 1));
	if (!skb) {
		etf_printk(XRADIO_DBG_WARN, "%s:xr_alloc_skb failed!\n", __func__);
		return -ENOMEM;
	}
	/* '\0' safe for strcpy, compat for old etf apk, may be removed in furture.*/
	nlhdr = nlmsg_put(skb, 0, 0, 0, total_len + 1, 0);
	payload = (u8 *)NLMSG_DATA(nlhdr);
	memcpy(payload, data1, len1);
	if (data2 && len2)
		memcpy((payload + len1), data2, len2);
	payload[total_len] = '\0';

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0)
	NETLINK_CB(skb).pid = 0;
#else
	NETLINK_CB(skb).portid = 0;
#endif
	NETLINK_CB(skb).dst_group = 0;

	down(&adapter_priv.send_lock);
	ret = netlink_unicast(adapter_priv.sock, skb, adapter_priv.send_pid, 0);
	if (ret < 0)
		etf_printk(XRADIO_DBG_ERROR, "%s:netlink_unicast failed(%d),pid=%d!\n",
					__func__, ret, adapter_priv.send_pid);
	else
		etf_printk(XRADIO_DBG_NIY, "%s:netlink_unicast len(%d),pid=%d!\n",
					__func__, ret, adapter_priv.send_pid);
	up(&adapter_priv.send_lock);

	return ret;
}

struct xradio_adapter *xradio_adapter_init(msg_proc func)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0))
	struct netlink_kernel_cfg cfg = {
		.input	= adapter_rec_handler,
	};
#endif
	if (func == NULL) {
		etf_printk(XRADIO_DBG_ERROR, "%s msg_proc is NULL\n", __func__);
		return NULL;
	}
	adapter_priv.handler = func;
	sema_init(&adapter_priv.send_lock, 1);
	if (adapter_init_msgbuf()) {
		etf_printk(XRADIO_DBG_ERROR, "%s adapter_init_msgbuf failed\n", __func__);
		return NULL;
	}

#if (ETF_QUEUEMODE)
	sema_init(&adapter_priv.proc_sem, 0);
	if (adapter_start_thread()) {
		adapter_deinit_msgbuf();
		etf_printk(XRADIO_DBG_ERROR, "%s adapter_start_thread failed\n", __func__);
		return NULL;
	}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0))
	adapter_priv.sock = netlink_kernel_create(&init_net, NL_FOR_XRADIO, 0,
			adapter_rec_handler, NULL, THIS_MODULE);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0))
	adapter_priv.sock = netlink_kernel_create(&init_net, NL_FOR_XRADIO,
			THIS_MODULE, &cfg);
#else
	adapter_priv.sock = netlink_kernel_create(&init_net, NL_FOR_XRADIO, &cfg);
#endif

	if (adapter_priv.sock == NULL) {
#if (ETF_QUEUEMODE)
		adapter_stop_thread();
#endif
		adapter_deinit_msgbuf();
		etf_printk(XRADIO_DBG_ERROR, "%s netlink_kernel_create failed\n", __func__);
		return NULL;
	}

	return &adapter_priv;
}

void xradio_adapter_deinit(void)
{
	if (adapter_priv.sock != NULL) {
		netlink_kernel_release(adapter_priv.sock);
		adapter_priv.sock = NULL;
	}
#if (ETF_QUEUEMODE)
	adapter_stop_thread();
#endif
	adapter_deinit_msgbuf();
}


/********************* interfaces of etf core **********************/
static struct xradio_etf etf_priv;
static struct etf_api_context context_save;

int wsm_etf_cmd(struct xradio_common *hw_priv, struct wsm_hdr *arg);
int xradio_bh_suspend(struct xradio_common *hw_priv);
int xradio_bh_resume(struct xradio_common *hw_priv);

void etf_adapter_version_init(struct xradio_etf *priv)
{
	priv->version = 0;
	priv->version |= ADAPTER_MAIN_VER << 24;
	priv->version |= ADAPTER_SUB_VER  << 16;
	priv->version |= ADAPTER_REV_VER  << 0;
}

static int  etf_check_version(u32 ver)
{
	u16 rev_ver  = (u16)((ver >> 0)  & 0xffff);
	u8  sub_ver  = (u8)((ver >> 16) & 0xff);
	u8  main_ver = (u8)((ver >> 24) & 0xff);

	etf_printk(XRADIO_DBG_NIY, "[%s] version: %d.%d.%d\n",
				    __func__, main_ver, sub_ver, rev_ver);
	if (main_ver >= 128 || sub_ver >= 200 || rev_ver >= 512)
		goto out;

	if (main_ver > REF_ETF_MAIN_VER) {
		return 1;
	} else if (main_ver == REF_ETF_MAIN_VER && sub_ver > REF_ETF_SUB_VER) {
		return 1;
	} else if (main_ver == REF_ETF_MAIN_VER && sub_ver >= REF_ETF_SUB_VER &&
				    rev_ver >= REF_ETF_REV_VER) {
		return 1;
	}

out:
	etf_printk(XRADIO_DBG_WARN, "*************************************\n");
	etf_printk(XRADIO_DBG_WARN, "     etf cli version is too low\n");
	etf_printk(XRADIO_DBG_WARN, "    %d.%d.%d or above is recommended.\n",
			    REF_ETF_MAIN_VER, REF_ETF_SUB_VER, REF_ETF_REV_VER);
	etf_printk(XRADIO_DBG_WARN, "*************************************\n");

	return 0;
}

static int etf_alloc_cli_buffer(u32 req_len, struct xradio_etf *priv)
{
	int ret = 0;

	if (req_len && req_len > priv->cli_data_len) {
		if (priv->cli_data) {
			kfree(priv->cli_data);
			priv->cli_data = NULL;
		}

		priv->cli_data = kmalloc(req_len, GFP_KERNEL);
		if (!priv->cli_data) {
			ret = EBUSY;
			etf_printk(XRADIO_DBG_ERROR,
			    "%s: can't allocate cli_data buffer!\n", __func__);
		}
		memset(priv->cli_data, 0, req_len);
		priv->cli_data_len = req_len;
		ret = 0;
	} else {
		if (req_len && req_len <= priv->cli_data_len)
			etf_printk(XRADIO_DBG_ALWY,
				"%s: cli_data buffer is ready.\n", __func__);
		else
			etf_printk(XRADIO_DBG_WARN,
				"%s: req_len = %d, priv->cli_data_len = %d.\n",
				__func__, req_len, priv->cli_data_len);
	}

	return ret;
}

static void etf_free_cli_buffer(struct xradio_etf *priv)
{
	if (priv->cli_data) {
		kfree(priv->cli_data);
		priv->cli_data = NULL;
		priv->cli_data_len = 0;
	}
}

bool etf_is_connect(void)
{
	return (bool)(etf_priv.etf_state != ETF_STAT_NULL);
}

void etf_set_core(void *core_priv)
{
	etf_priv.core_priv = core_priv;
}

const char *etf_get_fwpath(void)
{
	return (const char *)etf_priv.fw_path;
}

const char *etf_get_sddpath(void)
{
	return (const char *)etf_priv.sdd_path;
}

void etf_set_fwpath(const char *fw_path)
{
	etf_priv.fw_path = fw_path;
}

void etf_set_sddpath(const char *sdd_path)
{
	etf_priv.sdd_path = sdd_path;
}


static int etf_get_sdd_param(u8 *sdd, int sdd_len, u8 ies, void **data)
{
	int ie_len = 0;
	int parsedLength = 0;
	struct xradio_sdd *pElement = NULL;
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	/*parse SDD config.*/
	pElement = (struct xradio_sdd *)sdd;

	while (parsedLength < sdd_len) {
		if (pElement->id == ies) {
			*data = (void *)pElement->data;
			ie_len = pElement->length;
		}
		parsedLength += (FIELD_OFFSET(struct xradio_sdd, data) + \
						pElement->length);
		pElement = FIND_NEXT_ELT(pElement);
	}

	xradio_dbg(XRADIO_DBG_MSG, "parse len=%d, ie_len=%d.\n",
				parsedLength, ie_len);
	return ie_len;
}

static int etf_driver_resp(int state, int result)
{
	struct drv_resp drv_ret;
	drv_ret.len    = sizeof(drv_ret);
	drv_ret.id     = ETF_DRIVER_IND_ID;
	drv_ret.state  = state;
	drv_ret.result = result;
	return xradio_adapter_send(&drv_ret, drv_ret.len);
}

static int xradio_etf_connect(void)
{
	int ret = BOOT_SUCCESS;
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	if (etf_priv.etf_state == ETF_STAT_NULL) {
		etf_priv.etf_state = ETF_STAT_CONNECTING;
		ret = xradio_core_init();
		if (ret) {
			etf_printk(XRADIO_DBG_ERROR, "%s:xradio_core_init failed(%d).\n",
						__func__, ret);
			goto err;
		}
		etf_priv.etf_state = ETF_STAT_CONNECTED;
	} else {
		etf_printk(XRADIO_DBG_WARN, "%s etf is already connect.\n", __func__);
		return ETF_ERR_CONNECTED;
	}
	etf_printk(XRADIO_DBG_ALWY, "%s:ETF connect success.\n", __func__);
	return 0;

err:
	etf_priv.etf_state = ETF_STAT_NULL;
	return ret;
}

static void xradio_etf_disconnect(void)
{
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	if (etf_priv.etf_state != ETF_STAT_NULL) {
		xradio_core_deinit();
		etf_priv.etf_state = ETF_STAT_NULL;
	}
	etf_printk(XRADIO_DBG_ALWY, "%s end.\n", __func__);
}

static int xradio_etf_proc(void *data, int len)
{
	int ret = BOOT_SUCCESS;
	struct wsm_hdr *hdr_rev = (struct wsm_hdr *)data;
	struct drv_download *download;
	int    datalen;
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);

	down(&etf_priv.etf_lock);
	/* Don't process ETF cmd in wlan mode */
	if (etf_priv.is_wlan) {
		etf_printk(XRADIO_DBG_WARN, "%s is in wlan mode.\n", __func__);
		ret = ETF_ERR_WLAN_MODE;
		etf_driver_resp(BOOT_STATE_NULL, ret);
		goto out;
	}

	/* Process msg from ETF API */
	switch (MSG_ID(hdr_rev->id)) {
	case ETF_SOFT_RESET_REQ_ID:
		etf_printk(XRADIO_DBG_ALWY, "SOFT_RESET\n");
		xradio_etf_disconnect();
		etf_priv.fw_path   = NULL;
		etf_priv.sdd_path  = NULL;
		etf_priv.seq_send  = 0;
		etf_priv.seq_recv  = 0;
		memset(&context_save, 0, sizeof(context_save));
		etf_driver_resp(BOOT_STATE_NULL, BOOT_SUCCESS);
		break;
	case ETF_GET_API_CONTEXT_ID: /* for ETF tools which cannot save context*/
		{
			struct etf_api_context_result *ctxt_ret = NULL;
			struct xradio_common *hw_priv = etf_priv.core_priv;
			ctxt_ret = (struct etf_api_context_result *)hdr_rev;
			if (hw_priv) {
				ctxt_ret->is_etf_fw_run = hw_priv->wsm_caps.firmwareReady;
				memcpy(ctxt_ret->fw_label, hw_priv->wsm_caps.fw_label,
					min(HI_SW_LABEL_MAX, WSM_FW_LABEL));
				ctxt_ret->fw_api_ver = hw_priv->wsm_caps.firmwareApiVer;
				ctxt_ret->mib_baseaddr = context_save.config[1];
			} else {
				ctxt_ret->is_etf_fw_run = 0;
				memset(ctxt_ret->fw_label, 0, HI_SW_LABEL_MAX);
			}
			ctxt_ret->len = sizeof(*ctxt_ret);
			ctxt_ret->id |= ETF_CNF_BASE;
			ctxt_ret->result = 0;
			xradio_adapter_send(ctxt_ret, sizeof(*ctxt_ret));
		}
		break;
	case ETF_DOWNLOAD_ID:
		etf_printk(XRADIO_DBG_NIY, "DOWNLOAD_FW, size=%d\n", hdr_rev->len);
		download = (struct drv_download *)(hdr_rev + 1);
		datalen = hdr_rev->len - sizeof(*download) - sizeof(*hdr_rev);
		if (datalen <= 0) {
			/*it means old version APK, TODO:remove it in future */
			if (hdr_rev->len <= sizeof(*hdr_rev)) {
				etf_driver_resp(BOOT_COMPLETE, BOOT_SUCCESS);
				ret = xradio_etf_connect();
				if (ret)
					etf_driver_resp(BOOT_STATE_NULL, ret);
			}
			break;
		}
		if (download->flags & DOWNLOAD_F_PATH_ONLY) {
			etf_printk(XRADIO_DBG_WARN, "fw path=%s\n", (char *)(download+1));
			/*TODO: set etf_priv.fw_path*/

		} else if (download->flags & DOWNLOAD_F_START) {
			etf_printk(XRADIO_DBG_NIY, "fw first block\n");
			/*TODO: open a tmp file to save firmware */
		}

		/*TODO: put data into a tmp file */

		/*TODO: put data into a tmp file */
		if ((download->flags & DOWNLOAD_F_END)) {
			etf_printk(XRADIO_DBG_NIY, "fw last block\n");
			etf_driver_resp(BOOT_COMPLETE, BOOT_SUCCESS);
			/*TODO: save data tmp file and update etf_priv.fw_path*/
			ret = xradio_etf_connect();
			if (ret)
				etf_driver_resp(BOOT_STATE_NULL, ret);
		}
		break;
	case ETF_DOWNLOAD_SDD_ID:
		etf_printk(XRADIO_DBG_NIY, "DOWNLOAD_SDD, size=%d\n", hdr_rev->len);
		download = (struct drv_download *)(hdr_rev + 1);
		datalen = hdr_rev->len - sizeof(*download) - sizeof(*hdr_rev);
		if (datalen <= 0) {
			/*it means old version APK, TODO:remove it in future */
			if (hdr_rev->len <= sizeof(*hdr_rev)) {
				etf_driver_resp(BOOT_SDD_COMPLETE, BOOT_SUCCESS);
			}
			break;
		}

		if (download->flags & DOWNLOAD_F_PATH_ONLY) {
			etf_printk(XRADIO_DBG_WARN, "sdd path=%s\n", (char *)(download+1));
			/*TODO: set etf_priv.fw_path*/

		} else if ((download->flags & DOWNLOAD_F_START)) {
			etf_printk(XRADIO_DBG_NIY, "sdd first block\n");
			/*TODO: open a tmp file to save firmware */
		}

		/*TODO: put data into a tmp file */

		if ((download->flags & DOWNLOAD_F_END)) {
			etf_printk(XRADIO_DBG_NIY, "sdd last block\n");
			etf_driver_resp(BOOT_SDD_COMPLETE, BOOT_SUCCESS);
			/*TODO: save data tmp file and update etf_priv.sdd_path*/
		}
		break;
	case ETF_CONNECT_ID:
		{
			ETF_CONNECT_REQ_T *connect_req = (ETF_CONNECT_REQ_T *)hdr_rev;

			etf_printk(XRADIO_DBG_ALWY, "ETF_CONNECT_ID!\n");
			etf_driver_resp(BOOT_COMPLETE, BOOT_SUCCESS);
			ret = xradio_etf_connect();
			if (ret)
				etf_driver_resp(BOOT_STATE_NULL, ret);

			if (etf_check_version(connect_req->version)) {
				ret = etf_alloc_cli_buffer(connect_req->data_len, &etf_priv);
				if (ret)
					etf_driver_resp(BOOT_STATE_NULL, ret);
			}
		}
		break;
	case ETF_DISCONNECT_ID:
		etf_printk(XRADIO_DBG_ALWY, "ETF_DISCONNECT_ID!\n");
		xradio_etf_disconnect();
		etf_free_cli_buffer(&etf_priv);
		etf_driver_resp(BOOT_STATE_NULL, BOOT_SUCCESS);
		break;
	case ETF_RECONNECT_ID:
		{
			ETF_CONNECT_REQ_T *reconnect_req = (ETF_CONNECT_REQ_T *)hdr_rev;

			etf_printk(XRADIO_DBG_ALWY, "ETF_RECONNECT_ID!\n");
			xradio_etf_disconnect();
			etf_driver_resp(BOOT_COMPLETE, BOOT_SUCCESS);
			ret = xradio_etf_connect();
			if (ret)
				etf_driver_resp(BOOT_STATE_NULL, ret);

			if (etf_check_version(reconnect_req->version)) {
				ret = etf_alloc_cli_buffer(reconnect_req->data_len, &etf_priv);
				if (ret)
					etf_driver_resp(BOOT_STATE_NULL, ret);
			}
		}
		break;
	case ETF_GET_SDD_POWER_DEFT:  /* get default tx power */
		{
			const struct firmware *sdd = NULL;
			etf_printk(XRADIO_DBG_NIY, "%s ETF_GET_SDD_POWER_DEFT!\n", __func__);

			ret = request_firmware(&sdd, etf_get_sddpath(), NULL);
			/* get sdd data */
			if (likely(sdd) && likely(sdd->data)) {
				int i;
				u16  *outpower = (u16 *)(hdr_rev + 1);
				void *ie_data = NULL;
				int ie_len = etf_get_sdd_param((u8 *)sdd->data, sdd->size,
								SDD_MAX_OUTPUT_POWER_2G4_ELT_ID, &ie_data);
				if (ie_len > 0 && ie_data &&
				   ie_len < (ADAPTER_RX_BUF_LEN - sizeof(*hdr_rev))) {
					memcpy(outpower, ie_data, ie_len);
					for (i = 0; i < (ie_len>>1); i++)
						outpower[i] -= 48; /*default pwr = max power - 48*/
				}
				hdr_rev->len = sizeof(*hdr_rev) + ie_len;
				xradio_adapter_send(hdr_rev, hdr_rev->len);
				release_firmware(sdd);
			} else {
				etf_printk(XRADIO_DBG_ERROR, "%s: can't load sdd file %s.\n",
							__func__, etf_get_sddpath());
				ret = -ENOENT;
				hdr_rev->len = sizeof(*hdr_rev);
				xradio_adapter_send(hdr_rev, sizeof(*hdr_rev));
			}
		}
		break;
	case ETF_GET_SDD_PARAM_ID:
		{
			struct xradio_common *hw_priv = etf_priv.core_priv;
			struct get_sdd_param_req *sddreq = (struct get_sdd_param_req *)hdr_rev;
			struct get_sdd_result *sddret = (struct get_sdd_result *)sddreq;
			void *ie_data = NULL;
			int ie_len = 0;
			const struct firmware *sdd = NULL;
			etf_printk(XRADIO_DBG_NIY, "%s ETF_GET_SDD_PARAM_ID!\n", __func__);

			if (hw_priv && hw_priv->sdd) {
				etf_printk(XRADIO_DBG_NIY, "%s use xradio_common sdd!\n", __func__);
				sdd = hw_priv->sdd;
			} else {
				ret = request_firmware(&sdd, etf_get_sddpath(), NULL);
			}

			hdr_rev->id  = hdr_rev->id + ETF_CNF_BASE;
			if (likely(sdd) && likely(sdd->data)) {
				if (sddreq->flags & FLAG_GET_SDD_ALL) {
					sddret->result  = 0;
					sddret->length  = sdd->size;
					xradio_adapter_send_pkg(sddret, sizeof(*sddret), (u8 *)sdd->data,
							sddret->length);
				} else {
					ie_len = etf_get_sdd_param((u8 *)sdd->data, sdd->size,
								sddreq->ies, &ie_data);
					if (ie_data && ie_len > 0) {
						sddret->result  = 0;
						sddret->length  = ie_len;
						hdr_rev->len = sizeof(*sddret) + sddret->length;
						xradio_adapter_send_pkg(sddret, sizeof(*sddret), ie_data,
								sddret->length);
					} else {
						sddret->result = -ENOMSG;
						sddret->length = 0;
						hdr_rev->len = sizeof(*sddret) + sddret->length;
						xradio_adapter_send(sddret, sizeof(*sddret));
					}
				}
				if (hw_priv && hw_priv->sdd) {
					sdd = NULL;
				} else {
					release_firmware(sdd);
				}
			} else {
				etf_printk(XRADIO_DBG_ERROR, "%s: can't load sdd file %s.\n",
							__func__, etf_get_sddpath());
				sddret->result = -ENOENT;
				sddret->length = 0;
				hdr_rev->len = sizeof(*sddret) + sddret->length;
				xradio_adapter_send(sddret, sizeof(*sddret));
			}
		}
		break;
	case ETF_SET_CLI_PAR_DEFT:
		{
			CLI_PARAM_SAVE_T *hdr_data = (CLI_PARAM_SAVE_T *)hdr_rev;
			ETF_PARAM0 hdr_ret;

			etf_printk(XRADIO_DBG_NIY, "%s ETF_SET_CLI_PAR_DEFT!\n", __func__);
			hdr_ret.MsgId = hdr_rev->id + ETF_CNF_BASE;
			hdr_ret.MsgLen = sizeof(ETF_PARAM0);

			if (etf_check_version(hdr_data->version)) {
				if (etf_priv.cli_data && hdr_data->data_len == etf_priv.cli_data_len) {
					memset(etf_priv.cli_data, 0, hdr_data->data_len);
					memcpy(etf_priv.cli_data,
						    hdr_data->data, hdr_data->data_len);
					hdr_ret.result = BOOT_SUCCESS;
				} else {
					etf_printk(XRADIO_DBG_ALWY,
						"%s support_len = %d req_len = %d\n",
						__func__, etf_priv.cli_data_len, hdr_data->data_len);
					hdr_ret.result = BOOT_ERR_BAD_OP;
				}
			} else {
					hdr_ret.result = ETF_ERR_CHECK_VERSION;
			}

			xradio_adapter_send(&hdr_ret, hdr_ret.MsgLen);
		}
		break;
	case ETF_GET_CLI_PAR_DEFT:
		{
			struct get_cli_data_s *hdr_data;

			hdr_data = (struct get_cli_data_s *)hdr_rev;
			etf_printk(XRADIO_DBG_NIY, "%s ETF_GET_CLI_PAR_DEFT!\n", __func__);
			hdr_rev->id  = hdr_rev->id + ETF_CNF_BASE;
			if (etf_check_version(hdr_data->version)) {
				hdr_data->result   = BOOT_SUCCESS;
				hdr_data->data_len = etf_priv.cli_data_len;
				hdr_data->version  = etf_priv.version;
				hdr_data->msg_len  = sizeof(struct get_cli_data_s) + hdr_data->data_len;
				xradio_adapter_send_pkg(hdr_data, sizeof(struct get_cli_data_s),
						(void *)etf_priv.cli_data, hdr_data->data_len);
			} else {
				hdr_data->result = ETF_ERR_CHECK_VERSION;
				hdr_data->data_len = 0;
				hdr_data->version  = etf_priv.version;
				hdr_data->msg_len  = sizeof(struct get_cli_data_s);
				xradio_adapter_send(hdr_data, hdr_data->msg_len);
			}
		}
		break;
	case ETF_GET_SDD_PATH_ID:
		{
			struct get_sdd_patch_req *param_req;
			struct get_sdd_patch_ret *param_ret;
			const char *sdd_path = etf_get_sddpath();

			param_req = (struct get_sdd_patch_req *)hdr_rev;
			param_ret = (struct get_sdd_patch_ret *)param_req;
			etf_printk(XRADIO_DBG_NIY, "%s ETF_GET_SDD_PATH_ID!\n", __func__);

			hdr_rev->id  = hdr_rev->id + ETF_CNF_BASE;
			param_ret->result = BOOT_SUCCESS;
			param_ret->length = strlen(sdd_path);
			hdr_rev->len = sizeof(*param_ret) + param_ret->length;
			xradio_adapter_send_pkg(param_ret, sizeof(*param_ret),
					(void *)sdd_path, param_ret->length);
		}
		break;
	default:
		etf_printk(XRADIO_DBG_NIY, "%s: passed cmd=0x%04x, len=%d",
					__func__, MSG_ID(hdr_rev->id), hdr_rev->len);
		if (etf_priv.etf_state != ETF_STAT_CONNECTED) {
			etf_printk(XRADIO_DBG_WARN, "%s etf Not connect.\n", __func__);
			ret = ETF_ERR_NOT_CONNECT;
			etf_driver_resp(BOOT_STATE_NULL, ret);
			goto out;
		}
		if (SEQ_MASK(etf_priv.seq_send) != MSG_SEQ(hdr_rev->id)) {
			etf_printk(XRADIO_DBG_NIY, "%s:seq err, drv_seq=%d, seq=%d\n",
						__func__, etf_priv.seq_send, MSG_SEQ(hdr_rev->id));
			etf_priv.seq_send = MSG_SEQ(hdr_rev->id);
		}
		etf_priv.seq_send++;
		if (ETF_HWT_REQ == MSG_ID(hdr_rev->id)) {
			/*TODO: HW test*/
			etf_printk(XRADIO_DBG_ERROR, "%s HW test unsupport\n", __func__);
		} else if (etf_priv.core_priv) {
			if (ETF_DOWNLOAD_SDD == MSG_ID(hdr_rev->id) &&
			    hdr_rev->len <= sizeof(struct etf_sdd_req)) {
				struct xradio_common *hw_priv = etf_priv.core_priv;
				struct etf_sdd_req *sdd_req = (struct etf_sdd_req *)hdr_rev;
				u32 sdd_cmd = sdd_req->sdd_cmd;

				etf_printk(XRADIO_DBG_NIY, "%s add sdd data, cmd=0x%08x!\n",
							__func__, sdd_cmd);
				if (hw_priv->sdd) {
					u8 *sdd_data = (u8 *)(hdr_rev + 1);
					int sdd_len  = hw_priv->sdd->size;
					if (sdd_len + sizeof(*hdr_rev) + 4 < ADAPTER_RX_BUF_LEN) {
						memcpy(sdd_data, hw_priv->sdd->data, sdd_len);
						memcpy(sdd_data + sdd_len, &sdd_cmd, 4);
						hdr_rev->len += sdd_len;
					} else {
						etf_printk(XRADIO_DBG_ERROR,
									"ADAPTER_RX_BUF_LEN=%d, sdd_len=%d\n",
									 ADAPTER_RX_BUF_LEN, sdd_len);
					}
				} else
					etf_printk(XRADIO_DBG_ERROR, "%s core_priv sdd is NULL\n",
							__func__);
			}
			/* send to device*/
			ret = wsm_etf_cmd(etf_priv.core_priv, (struct wsm_hdr *)data);
			if (ret < 0) {
				etf_driver_resp(BOOT_STATE_NULL, ret);
				etf_printk(XRADIO_DBG_ERROR, "%s wsm_etf_cmd failed(%d)\n",
							__func__, ret);
			}
		} else {
			etf_printk(XRADIO_DBG_ERROR, "%s core_priv is NULL\n",
						__func__);
		}
		break;
	}
	up(&etf_priv.etf_lock);
	etf_printk(XRADIO_DBG_NIY, "%s success\n", __func__);
	return 0;

out:
	up(&etf_priv.etf_lock);
	if (ret)
		etf_printk(XRADIO_DBG_ERROR, "%s proc failed(%d)\n", __func__, ret);
	return ret;
}

int xradio_etf_from_device(struct sk_buff **skb)
{
	int ret = 0;
	struct wsm_hdr *wsm = (struct wsm_hdr *)((*skb)->data);
	etf_printk(XRADIO_DBG_NIY, "%s MsgId=0x%04x, len=%d\n", __func__,
			   wsm->id, wsm->len);
	ret = xradio_adapter_send((void *)wsm, wsm->len);
	if (ret < 0) {
		etf_printk(XRADIO_DBG_ERROR, "%s xradio_adapter_send failed(%d).\n",
					__func__, ret);
	} else {
		etf_priv.seq_recv++;
	}
	return 0;
}

void xradio_etf_save_context(void *buf, int len)
{
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	if (len == sizeof(context_save)) {
		etf_printk(XRADIO_DBG_NIY, "%s, len=%d\n", __func__, len);
		memcpy(&context_save, buf, sizeof(context_save));
	} else if (len > sizeof(context_save)) {
		etf_printk(XRADIO_DBG_WARN, "%s, len=%d\n", __func__, len);
		memcpy(&context_save, buf, sizeof(context_save));
	} else {
		etf_printk(XRADIO_DBG_ERROR, "%s,len too short=%d\n", __func__, len);
	}
}

void xradio_etf_to_wlan(u32 change)
{
	down(&etf_priv.etf_lock);
	etf_priv.is_wlan = change;
	if (change && etf_is_connect()) {
		etf_printk(XRADIO_DBG_NIY, "%s change to wlan.\n", __func__);
		xradio_etf_disconnect();
	}
	up(&etf_priv.etf_lock);
}
EXPORT_SYMBOL_GPL(xradio_etf_to_wlan);

int xradio_etf_suspend(void)
{
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	if (down_trylock(&etf_priv.etf_lock)) {
		etf_printk(XRADIO_DBG_WARN,
			"%s etf_lock is locked\n", __func__);
		return -EBUSY; /*etf is locked.*/
	}
	if (etf_is_connect()) {
		etf_printk(XRADIO_DBG_WARN,
			"%s etf is running, don't suspend!\n", __func__);
		up(&etf_priv.etf_lock);
		return -EBUSY; /*etf is running*/
	}
	up(&etf_priv.etf_lock);
	return 0;
}

int xradio_etf_resume(void)
{
	etf_printk(XRADIO_DBG_NIY, "%s\n", __func__);
	return 0;
}

int xradio_etf_init(void)
{
	int ret = 0;
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	sema_init(&etf_priv.etf_lock, 1);
	etf_priv.etf_state = ETF_STAT_NULL;
	etf_priv.fw_path  = NULL;
	etf_priv.sdd_path = NULL;
	etf_priv.adapter = xradio_adapter_init(&xradio_etf_proc);
	etf_adapter_version_init(&etf_priv);
	return ret;
}

void xradio_etf_deinit(void)
{
	etf_printk(XRADIO_DBG_TRC, "%s\n", __func__);
	/*ensure in disconnect state when etf deinit*/
	xradio_etf_to_wlan(1);
	etf_free_cli_buffer(&etf_priv);
	xradio_adapter_deinit();
	memset(&etf_priv, 0, sizeof(etf_priv));
}

#endif /*XRADIO_ETF_C*/
