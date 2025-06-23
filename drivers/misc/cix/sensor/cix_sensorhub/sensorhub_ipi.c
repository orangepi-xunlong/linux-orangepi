// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#define pr_fmt(fmt)	"[sensorhub_ipi] " fmt

#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include <linux/kernel.h>
#include <linux/virtio.h>
#include <linux/rpmsg.h>

#include "sensorhub_ipi.h"
#include "cix_sensorhub.h"
#include "hf_sensor_type.h"

#define MSG		"hello, Sensor Fusion!"

struct rpmsg_device *g_sfh_rpdev;

struct rpmsg_device *sfh_get_rpmsg(void)
{
	return g_sfh_rpdev;
}
EXPORT_SYMBOL_GPL(sfh_get_rpmsg);

struct ipi_hw_master {
	spinlock_t lock;
	bool running;
	struct list_head head;
	struct workqueue_struct *workqueue;
	struct work_struct work;
};

struct ipi_hw_transfer {
	struct completion done;
	int count;
	/* data buffers */
	const unsigned char *tx;
	unsigned char *rx;
	unsigned int tx_len;
	unsigned int rx_len;
	void *context;
};

static struct ipi_hw_master hw_master;
static struct ipi_hw_transfer hw_transfer;
static DEFINE_SPINLOCK(hw_transfer_lock);

static int ipi_txrx_bufs(struct ipi_transfer *t)
{
	int status = 0, retry = 0;
	int timeout;
	unsigned long flags;
	struct ipi_hw_transfer *hw = &hw_transfer;

	spin_lock_irqsave(&hw_transfer_lock, flags);
	hw->tx = t->tx_buf;
	hw->rx = t->rx_buf;
	hw->tx_len = t->tx_len;
	hw->rx_len = t->rx_len;

	reinit_completion(&hw->done);
	hw->context = &hw->done;
	spin_unlock_irqrestore(&hw_transfer_lock, flags);

	do {
		status = sfh_ipi_send(IPI_SENSOR,
			(unsigned char *)hw->tx, hw->tx_len, 0, SCP_A_ID);
		if (status == SCP_IPI_ERROR) {
			pr_err("IPI_SENSOR send fail\n");
			return -1;
		}
		if (status == SCP_IPI_BUSY) {
			if (retry++ == 1000) {
				pr_err("IPI_SENSOR send retry fail\n");
				return -1;
			}
			if (retry % 100 == 0)
				usleep_range(1000, 2000);
		}
	} while (status == SCP_IPI_BUSY);

	if (retry >= 100)
		pr_debug("IPI_SENSOR send retry time:%d\n", retry);

	timeout = wait_for_completion_timeout(&hw->done,
			msecs_to_jiffies(500));

	spin_lock_irqsave(&hw_transfer_lock, flags);
	if (!timeout) {
		pr_err("IPI_SENSOR transfer timeout!");
		hw->count = -1;
	}
	hw->context = NULL;
	spin_unlock_irqrestore(&hw_transfer_lock, flags);
	return hw->count;
}

static void ipi_complete(void *arg)
{
	complete(arg);
}

static void ipi_transfer_messages(void)
{
	struct ipi_message *m;
	struct ipi_transfer *t = NULL;
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw_master.lock, flags);
	if (list_empty(&hw_master.head) || hw_master.running)
		goto out;
	hw_master.running = true;
	while (!list_empty(&hw_master.head)) {
		m = list_first_entry(&hw_master.head,
			struct ipi_message, list);
		list_del(&m->list);
		spin_unlock_irqrestore(&hw_master.lock, flags);
		list_for_each_entry(t, &m->transfers, transfer_list) {
			if (!t->tx_buf && t->tx_len) {
				status = -EINVAL;
				pr_err("transfer param wrong, null tx buf\n");
				break;
			}
			if (t->tx_len)
				status = ipi_txrx_bufs(t);
			if (status < 0) {
				status = -EREMOTEIO;
				/* pr_err("transfer err :%d\n", status); */
				break;
			} else if (status != t->rx_len) {
				pr_err("ack err :%d %d\n", status, t->rx_len);
				status = -EREMOTEIO;
				break;
			}
			status = 0;
		}
		m->status = status;
		m->complete(m->context);
		spin_lock_irqsave(&hw_master.lock, flags);
	}
	hw_master.running = false;
out:
	spin_unlock_irqrestore(&hw_master.lock, flags);
}

static void ipi_prefetch_messages(void)
{
	ipi_transfer_messages();
}

static void ipi_work(struct work_struct *work)
{
	ipi_transfer_messages();
}

static int __ipi_transfer(struct ipi_message *m)
{
	unsigned long flags;

	m->status = -EINPROGRESS;

	spin_lock_irqsave(&hw_master.lock, flags);
	list_add_tail(&m->list, &hw_master.head);
	queue_work(hw_master.workqueue, &hw_master.work);
	spin_unlock_irqrestore(&hw_master.lock, flags);
	return 0;
}

static int __ipi_xfer(struct ipi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = ipi_complete;
	message->context = &done;

	status = __ipi_transfer(message);

	if (status == 0) {
		ipi_prefetch_messages();
		wait_for_completion(&done);
		status = message->status;
	}
	message->context = NULL;
	return status;
}

static int ipi_sync(const unsigned char *txbuf, unsigned int n_tx,
		unsigned char *rxbuf, unsigned int n_rx)
{
	struct ipi_transfer t;
	struct ipi_message m;

	t.tx_buf = txbuf;
	t.tx_len = n_tx;
	t.rx_buf = rxbuf;
	t.rx_len = n_rx;

	ipi_message_init(&m);
	ipi_message_add_tail(&t, &m);

	return __ipi_xfer(&m);
}

int cix_nanohub_ipi_sync(unsigned char *buffer, unsigned int len)
{
	return ipi_sync(buffer, len, buffer, len);
}

void cix_nanohub_ipi_complete(unsigned char *buffer, unsigned int len)
{
	struct ipi_hw_transfer *hw = &hw_transfer;

	spin_lock(&hw_transfer_lock);
	if (!hw->context) {
		pr_err("after ipi timeout ack occur then dropped this\n");
		goto out;
	}
	/* only copy hw->rx_len bytes to hw->rx to avoid memory corruption */
	memcpy(hw->rx, buffer, hw->rx_len);
	/* hw->count give real len */
	hw->count = len;
	complete(hw->context);
out:
	spin_unlock(&hw_transfer_lock);
}

/* A common API to send message to SCP */
enum scp_ipi_status sfh_ipi_send(enum ipi_id id, void *buf,
	unsigned int len, unsigned int wait, enum scp_core_id scp_id)
{
	int ret;
	struct ipi_hw_transfer *hw = &hw_transfer;

	ret = rpmsg_send(g_sfh_rpdev->ept, buf, len);
	if (ret < 0) {
		pr_err("failed to send message\n");
		return SCP_IPI_ERROR;
	}

	if (hw->context)
		complete(hw->context);

	hw->count = len;

	return SCP_IPI_DONE;
}

/* An API to get scp status */
enum scp_ipi_status is_sfh_ready(enum scp_core_id scp_id) {
	// TBD
	return SCP_IPI_DONE;
}

int sensorhub_external_write(const char *buffer, int length)
{
	return rpmsg_send(g_sfh_rpdev->ept, (void *)buffer, length);
}

void sfh_register_feature(enum feature_id id) {}

enum scp_ipi_status sfh_ipi_registration(enum ipi_id id,
	void (*ipi_handler)(int id, void *data, unsigned int len),
	const char *name) {
	return SCP_IPI_DONE;
}

enum scp_ipi_status sfh_ipi_unregistration(enum ipi_id id) {
	return SCP_IPI_DONE;
}

void sfh_A_register_notify(struct notifier_block *nb)  {}
void sfh_A_unregister_notify(struct notifier_block *nb) {}

static struct rpmsg_device_id sfh_rpmsg_driver_id_table[] = {
	{ .name	= "sfh-rpmsg" },
	{ },
};

static int sfh_rpmsg_probe(struct rpmsg_device *rpdev)
{
	int err;

	err = rpmsg_send(rpdev->ept, (void *)MSG, strlen(MSG));
	if (err) {
		pr_err("rpmsg send failed\n");
		return err;
	}

	g_sfh_rpdev = rpdev;
	if (!g_sfh_rpdev || !g_sfh_rpdev->ept)
		pr_err("rpmsg device is NULL\n");

	return 0;
}

static void sfh_rpmsg_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "sfh rpmsg driver is removed\n");
}

static int sfh_rpmsg_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	if ((*(unsigned long *)data) == SCP_EVENT_READY) {
		cix_nanohub_ready_event(SCP_EVENT_READY);
		return 0;
	} else if (((union SCP_SENSOR_HUB_DATA*)data)->notify_rsp.sensorType == 0) {
		if(((union SCP_SENSOR_HUB_DATA*)data)->notify_rsp.action == SENSOR_HUB_NOTIFY) {
			cix_nanohub_ipi_handler(0, data, len);
		}
	}

	return 0;
}


static struct rpmsg_driver cix_sfh_rpmsg_driver = {
	.drv.name	= "sfh-rpmsg",
	.drv.owner	= THIS_MODULE,
	.id_table	= sfh_rpmsg_driver_id_table,
	.probe		= sfh_rpmsg_probe,
	.callback	= sfh_rpmsg_cb,
	.remove		= sfh_rpmsg_remove,
};

int cix_nanohub_ipi_init(void)
{
	int ret;

	init_completion(&hw_transfer.done);
	INIT_WORK(&hw_master.work, ipi_work);
	INIT_LIST_HEAD(&hw_master.head);
	spin_lock_init(&hw_master.lock);
	hw_master.workqueue = create_singlethread_workqueue("ipi_master");
	if (hw_master.workqueue == NULL) {
		pr_err("workqueue fail\n");
		return -1;
	}

	ret = register_rpmsg_driver(&cix_sfh_rpmsg_driver);
	if (ret)
		pr_err("register rpmsg driver failed\n");

	return 0;
}