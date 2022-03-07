/*
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <wcn_bus.h>

#include "loopcheck.h"
#include "wcn_glb.h"
#include "wcn_procfs.h"

int mdbg_log_read(int channel, struct mbuf_t *head,
		  struct mbuf_t *tail, int num);

static struct ring_device *ring_dev;
static unsigned long long rx_count;
static unsigned long long rx_count_last;

bool mdbg_rx_count_change(void)
{
	rx_count = sprdwcn_bus_get_rx_total_cnt();

	WCN_INFO("rx_count:0x%llx rx_count_last:0x%llx\n",
		rx_count, rx_count_last);

	if ((rx_count == 0) && (rx_count_last == 0)) {
		return true;
	} else if (rx_count != rx_count_last) {
		rx_count_last = rx_count;
		return true;
	} else {
		return false;
	}
}

static long int mdbg_comm_write(char *buf,
				long int len, unsigned int subtype)
{
	unsigned char *send_buf = NULL;
	char *str = NULL;
	struct mbuf_t *head, *tail;
	int num = 1;

	if (unlikely(marlin_get_module_status() != true)) {
		WCN_ERR("WCN module have not open\n");
		return -EIO;
	}
	send_buf = kzalloc(len + PUB_HEAD_RSV + 1, GFP_KERNEL);
	if (!send_buf)
		return -ENOMEM;
	memcpy(send_buf + PUB_HEAD_RSV, buf, len);

	str = strstr(send_buf + PUB_HEAD_RSV, SMP_HEAD_STR);
	if (!str)
		str = strstr(send_buf + PUB_HEAD_RSV + ARMLOG_HEAD,
			     SMP_HEAD_STR);

	if (str) {
		int ret;

		/* for arm log to pc */
		WCN_INFO("smp len:%ld,str:%s\n", len, str);
		str[sizeof(SMP_HEAD_STR)] = 0;
		ret = kstrtol(&str[sizeof(SMP_HEAD_STR) - 1], 10,
							&ring_dev->flag_smp);
		WCN_INFO("smp ret:%d, flag_smp:%ld\n", ret,
			 ring_dev->flag_smp);
		kfree(send_buf);
	} else {
		if (!sprdwcn_bus_list_alloc(
				mdbg_proc_ops[MDBG_AT_TX_OPS].channel,
				&head, &tail, &num)) {
			head->buf = send_buf;
			head->len = len;
			head->next = NULL;
			sprdwcn_bus_push_list(
				mdbg_proc_ops[MDBG_AT_TX_OPS].channel,
				head, tail, num);
		}
	}

	return len;
}

int mdbg_log_read(int channel, struct mbuf_t *head,
		  struct mbuf_t *tail, int num)
{
	struct ring_rx_data *rx;

	if (ring_dev) {
		mutex_lock(&ring_dev->mdbg_read_mutex);
		rx = kmalloc(sizeof(*rx), GFP_KERNEL);
		if (!rx) {
			WCN_ERR("mdbg ring low memory\n");
			mutex_unlock(&ring_dev->mdbg_read_mutex);
			sprdwcn_bus_push_list(channel, head, tail, num);
			return 0;
		}
		mutex_unlock(&ring_dev->mdbg_read_mutex);
		spin_lock_bh(&ring_dev->rw_lock);
		rx->channel = channel;
		rx->head = head;
		rx->tail = tail;
		rx->num = num;
		list_add_tail(&rx->entry, &ring_dev->rx_head);
		spin_unlock_bh(&ring_dev->rw_lock);
		schedule_work(&ring_dev->rx_task);
	}

	return 0;
}

long int mdbg_send(char *buf, long int len, unsigned int subtype)
{
	long int sent_size = 0;

	WCN_DEBUG("BYTE MODE");
	__pm_stay_awake(ring_dev->rw_wake_lock);
	sent_size = mdbg_comm_write(buf, len, subtype);
	__pm_relax(ring_dev->rw_wake_lock);

	return sent_size;
}
EXPORT_SYMBOL_GPL(mdbg_send);

int mdbg_tx_cb(int channel, struct mbuf_t *head,
	       struct mbuf_t *tail, int num)
{
	struct mbuf_t *mbuf_node;
	int i;

	mbuf_node = head;
	for (i = 0; i < num; i++, mbuf_node = mbuf_node->next) {
		kfree(mbuf_node->buf);
		mbuf_node->buf = NULL;
	}

	sprdwcn_bus_list_free(channel, head, tail, num);

	return 0;
}

int mdbg_tx_power_notify(int chn, int flag)
{
	if (flag) {
		WCN_DEBUG("%s resume\n", __func__);
#ifdef CONFIG_WCN_LOOPCHECK
		start_loopcheck();
#endif
	} else {
		WCN_DEBUG("%s suspend\n", __func__);
#ifdef CONFIG_WCN_LOOPCHECK
		stop_loopcheck();
#endif
	}
	return 0;
}
