/**
 * aicwf_usb.c
 *
 * USB function declarations
 *
 * Copyright (C) AICSemi 2018-2020
 */
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/printk.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/completion.h>
#include <linux/semaphore.h>
#include <linux/debugfs.h>
#include <linux/atomic.h>
#include <linux/vmalloc.h>
#include <linux/signal.h>

#include <linux/usb.h>
#include <linux/kthread.h>
#include "aic_bsp_txrxif.h"
#include "aicusb.h"
#include "aic_bsp_driver.h"

static int fw_loaded;

#ifdef CONFIG_PLATFORM_ALLWINNER
extern void sunxi_wlan_set_power(int on);
#endif
static int aicbsp_platform_power_on(void);
static void aicbsp_platform_power_off(void);
static int  aicbsp_usb_init(void);
static void aicbsp_usb_exit(void);
static struct usb_driver aicwf_usbdrvr;

int aicbsp_device_init(void)
{
	if (aicbsp_platform_power_on() < 0)
		return -1;

	return aicbsp_usb_init();
}

void aicbsp_device_exit(void)
{
	aicbsp_usb_exit();
	aicbsp_platform_power_off();
}

static struct device_match_entry aicdev_match_table[] = {
	{0xa69c, 0x8800, PRODUCT_ID_AIC8800D,   "aic8800d",   0, 0}, // 8800d in bootloader mode
};

static struct device_match_entry *aic_matched_ic;

int aicbsp_set_subsys(int subsys, int state)
{
	bsp_dbg("%s, subsys: %d, state to: %d\n", __func__, subsys, state);
	return state;
}
EXPORT_SYMBOL_GPL(aicbsp_set_subsys);

void *aicbsp_get_drvdata(void *args)
{
	return dev_get_drvdata(args);
}

static int aicbsp_platform_power_on(void)
{
#ifdef CONFIG_PLATFORM_ALLWINNER
	sunxi_wlan_set_power(1);
	mdelay(50);
#endif
	return 0;
}

static void aicbsp_platform_power_off(void)
{
#ifdef CONFIG_PLATFORM_ALLWINNER
	sunxi_wlan_set_power(0);
	mdelay(100);
#endif
	bsp_dbg("%s\n", __func__);
}

static int aicbsp_usb_init(void)
{
	if (usb_register(&aicwf_usbdrvr) < 0)
		return -1;
	return 0;
}

static void aicbsp_usb_exit(void)
{
	usb_deregister(&aicwf_usbdrvr);
}

static void aicwf_usb_tx_flowctrl(struct priv_dev *aicdev, bool state)
{
}

static struct aicwf_usb_buf *aicwf_usb_tx_dequeue(struct priv_dev *aicdev,
	struct list_head *q, int *counter, spinlock_t *qlock)
{
	unsigned long flags;
	struct aicwf_usb_buf *usb_buf;

	spin_lock_irqsave(qlock, flags);
	if (list_empty(q)) {
		usb_buf = NULL;
	} else {
		usb_buf = list_first_entry(q, struct aicwf_usb_buf, list);
		list_del_init(&usb_buf->list);
		if (counter)
			(*counter)--;
	}
	spin_unlock_irqrestore(qlock, flags);
	return usb_buf;
}

static void aicwf_usb_tx_queue(struct priv_dev *aicdev,
	struct list_head *q, struct aicwf_usb_buf *usb_buf, int *counter,
	spinlock_t *qlock)
{
	unsigned long flags;

	spin_lock_irqsave(qlock, flags);
	list_add_tail(&usb_buf->list, q);
	(*counter)++;
	spin_unlock_irqrestore(qlock, flags);
}

static struct aicwf_usb_buf *aicwf_usb_rx_buf_get(struct priv_dev *aicdev)
{
	unsigned long flags;
	struct aicwf_usb_buf *usb_buf;

	spin_lock_irqsave(&aicdev->rx_free_lock, flags);
	if (list_empty(&aicdev->rx_free_list)) {
		usb_buf = NULL;
	} else {
		usb_buf = list_first_entry(&aicdev->rx_free_list, struct aicwf_usb_buf, list);
		list_del_init(&usb_buf->list);
	}
	spin_unlock_irqrestore(&aicdev->rx_free_lock, flags);
	return usb_buf;
}

static void aicwf_usb_rx_buf_put(struct priv_dev *aicdev, struct aicwf_usb_buf *usb_buf)
{
	unsigned long flags;

	spin_lock_irqsave(&aicdev->rx_free_lock, flags);
	list_add_tail(&usb_buf->list, &aicdev->rx_free_list);
	spin_unlock_irqrestore(&aicdev->rx_free_lock, flags);
}

static void aicwf_usb_tx_complete(struct urb *urb)
{
	unsigned long flags;
	struct aicwf_usb_buf *usb_buf = (struct aicwf_usb_buf *) urb->context;
	struct priv_dev *aicdev = usb_buf->aicdev;
	struct sk_buff *skb;
	u8 *buf;

	if (usb_buf->cfm == false) {
		skb = usb_buf->skb;
	} else {
		buf = (u8 *)usb_buf->skb;
	}

	if (usb_buf->cfm == false) {
		dev_kfree_skb_any(skb);
	} else {
		kfree(buf);
	}
	usb_buf->skb = NULL;

	aicwf_usb_tx_queue(aicdev, &aicdev->tx_free_list, usb_buf,
					&aicdev->tx_free_count, &aicdev->tx_free_lock);

	spin_lock_irqsave(&aicdev->tx_flow_lock, flags);
	if (aicdev->tx_free_count > AICWF_USB_TX_HIGH_WATER) {
		if (aicdev->tbusy) {
			aicdev->tbusy = false;
			aicwf_usb_tx_flowctrl(aicdev, false);
		}
	}
	spin_unlock_irqrestore(&aicdev->tx_flow_lock, flags);
	}

static void aicwf_usb_rx_complete(struct urb *urb)
{
	struct aicwf_usb_buf *usb_buf = (struct aicwf_usb_buf *) urb->context;
	struct priv_dev *aicdev = usb_buf->aicdev;
	struct aicwf_rx_priv *rx_priv = aicdev->rx_priv;
	struct sk_buff *skb = NULL;
	unsigned long flags = 0;

	skb = usb_buf->skb;
	usb_buf->skb = NULL;

	if (urb->actual_length > urb->transfer_buffer_length) {
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(aicdev, usb_buf);
		schedule_work(&aicdev->rx_urb_work);
		return;
	}

	if (urb->status != 0 || !urb->actual_length) {
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(aicdev, usb_buf);
		schedule_work(&aicdev->rx_urb_work);
		return;
	}

	if (aicdev->state == USB_UP_ST) {
		skb_put(skb, urb->actual_length);

		spin_lock_irqsave(&rx_priv->rxqlock, flags);
		if (!aicwf_rxframe_enqueue(aicdev->dev, &rx_priv->rxq, skb)) {
			spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
			bsp_err("rx_priv->rxq is over flow!!!\n");
			aicwf_dev_skb_free(skb);
			return;
		}
		spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
		atomic_inc(&rx_priv->rx_cnt);
		complete(&rx_priv->aicdev->bus_if->busrx_trgg);
		aicwf_usb_rx_buf_put(aicdev, usb_buf);

		schedule_work(&aicdev->rx_urb_work);
	} else {
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(aicdev, usb_buf);
	}
}

static int aicwf_usb_submit_rx_urb(struct priv_dev *aicdev,
				struct aicwf_usb_buf *usb_buf)
{
	struct sk_buff *skb;
	int ret;

	if (!usb_buf || !aicdev)
		return -1;

	if (aicdev->state != USB_UP_ST) {
		bsp_err("usb state is not up!\n");
		aicwf_usb_rx_buf_put(aicdev, usb_buf);
		return -1;
	}

	skb = __dev_alloc_skb(AICWF_USB_MAX_PKT_SIZE, GFP_KERNEL);
	if (!skb) {
		aicwf_usb_rx_buf_put(aicdev, usb_buf);
		return -1;
	}

	usb_buf->skb = skb;

	usb_fill_bulk_urb(usb_buf->urb,
		aicdev->udev,
		aicdev->bulk_in_pipe,
		skb->data, skb_tailroom(skb), aicwf_usb_rx_complete, usb_buf);

	usb_buf->aicdev = aicdev;

	usb_anchor_urb(usb_buf->urb, &aicdev->rx_submitted);
	ret = usb_submit_urb(usb_buf->urb, GFP_ATOMIC);
	if (ret) {
		bsp_err("usb submit rx urb fail:%d\n", ret);
		usb_unanchor_urb(usb_buf->urb);
		aicwf_dev_skb_free(usb_buf->skb);
		usb_buf->skb = NULL;
		aicwf_usb_rx_buf_put(aicdev, usb_buf);

		msleep(100);
	}
	return 0;
}

static void aicwf_usb_rx_submit_all_urb(struct priv_dev *aicdev)
{
	struct aicwf_usb_buf *usb_buf;

	if (aicdev->state != USB_UP_ST) {
		bsp_err("bus is not up=%d\n", aicdev->state);
		return;
	}
	if (aicdev->app_cmp) {
		return;
	}

	while ((usb_buf = aicwf_usb_rx_buf_get(aicdev)) != NULL) {
		if (aicwf_usb_submit_rx_urb(aicdev, usb_buf)) {
			bsp_err("usb rx refill fail\n");
			if (aicdev->state != USB_UP_ST)
				return;
		}
	}
}

static void aicwf_usb_rx_prepare(struct priv_dev *aicdev)
{
	aicwf_usb_rx_submit_all_urb(aicdev);
}

static void aicwf_usb_tx_prepare(struct priv_dev *aicdev)
{
	struct aicwf_usb_buf *usb_buf;

	while (!list_empty(&aicdev->tx_post_list)) {
		usb_buf = aicwf_usb_tx_dequeue(aicdev, &aicdev->tx_post_list,
			&aicdev->tx_post_count, &aicdev->tx_post_lock);
		if (usb_buf->skb) {
			dev_kfree_skb(usb_buf->skb);
			usb_buf->skb = NULL;
		}
		aicwf_usb_tx_queue(aicdev, &aicdev->tx_free_list, usb_buf,
				&aicdev->tx_free_count, &aicdev->tx_free_lock);
	}
}
static void aicwf_usb_tx_process(struct priv_dev *aicdev)
{
	struct aicwf_usb_buf *usb_buf;
	int ret = 0;
	u8 *data = NULL;

	while (!list_empty(&aicdev->tx_post_list)) {
		if (aicdev->state != USB_UP_ST) {
			bsp_err("usb state is not up!\n");
			return;
		}

		usb_buf = aicwf_usb_tx_dequeue(aicdev, &aicdev->tx_post_list,
						&aicdev->tx_post_count, &aicdev->tx_post_lock);
		if (!usb_buf) {
			bsp_err("can not get usb_buf from tx_post_list!\n");
			return;
		}
		data = usb_buf->skb->data;

		ret = usb_submit_urb(usb_buf->urb, GFP_ATOMIC);
		if (ret) {
			bsp_err("aicwf_usb_bus_tx usb_submit_urb FAILED\n");
			goto fail;
		}

		continue;
fail:
		dev_kfree_skb(usb_buf->skb);
		usb_buf->skb = NULL;
		aicwf_usb_tx_queue(aicdev, &aicdev->tx_free_list, usb_buf,
					&aicdev->tx_free_count, &aicdev->tx_free_lock);
	}
}

int aicwf_bustx_thread(void *data)
{
	struct aicwf_bus *bus = (struct aicwf_bus *)data;
	struct priv_dev *aicdev = bus->bus_priv.dev;

	while (1) {
		if (kthread_should_stop()) {
			bsp_err("usb bustx thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&bus->bustx_trgg)) {
			if (aicdev->app_cmp == false) {
				bsp_err("usb bustx thread will to stop\n");
				mdelay(100);
				continue;
			}
			if (aicdev->tx_post_count > 0)
				aicwf_usb_tx_process(aicdev);
		}
	}

	return 0;
}

int aicwf_busrx_thread(void *data)
{
	struct aicwf_rx_priv *rx_priv = (struct aicwf_rx_priv *)data;
	struct aicwf_bus *bus_if = rx_priv->aicdev->bus_if;

	allow_signal(SIGTERM);
	while (1) {
		if (kthread_should_stop()) {
			bsp_err("usb busrx thread stop\n");
			break;
		}
		if (!wait_for_completion_interruptible(&bus_if->busrx_trgg)) {
			aicwf_process_rxframes(rx_priv);
		} else {
			break;
		}
		if (bus_if->state == BUS_DOWN_ST) {
			bsp_dbg("bus down");
			break;
		}

	}
	bsp_dbg("rx_thread exit\n");

	return 0;
}

static void aicwf_usb_send_msg_complete(struct urb *urb)
{
	struct priv_dev *aicdev = (struct priv_dev *) urb->context;

	aicdev->msg_finished = true;
	if (waitqueue_active(&aicdev->msg_wait))
		wake_up(&aicdev->msg_wait);
}

static int aicwf_usb_bus_txmsg(struct device *dev, u8 *buf, u32 len)
{
	int ret = 0;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;

	if (aicdev->state != USB_UP_ST)
		return -EIO;

	if (buf == NULL || len == 0 || aicdev->msg_out_urb == NULL)
		return -EINVAL;

	if (test_and_set_bit(0, &aicdev->msg_busy)) {
		bsp_err("In a control frame option, can't tx!\n");
		return -EIO;
	}

	aicdev->msg_finished = false;

#ifdef CONFIG_USB_MSG_EP
	if (aicdev->msg_out_pipe && aicdev->use_msg_ep == 1) {
		usb_fill_bulk_urb(aicdev->msg_out_urb,
			aicdev->udev,
			aicdev->msg_out_pipe,
			buf, len, (usb_complete_t) aicwf_usb_send_msg_complete, aicdev);
	} else {
		usb_fill_bulk_urb(aicdev->msg_out_urb,
			aicdev->udev,
			aicdev->bulk_out_pipe,
			buf, len, (usb_complete_t) aicwf_usb_send_msg_complete, aicdev);
	}
#else
	usb_fill_bulk_urb(aicdev->msg_out_urb,
		aicdev->udev,
		aicdev->bulk_out_pipe,
		buf, len, (usb_complete_t) aicwf_usb_send_msg_complete, aicdev);
	aicdev->msg_out_urb->transfer_flags |= URB_ZERO_PACKET;
#endif

	ret = usb_submit_urb(aicdev->msg_out_urb, GFP_ATOMIC);
	if (ret) {
		bsp_err("usb_submit_urb failed %d\n", ret);
		goto exit;
	}

	ret = wait_event_timeout(aicdev->msg_wait,
		aicdev->msg_finished, msecs_to_jiffies(CMD_TX_TIMEOUT));
	if (!ret) {
		if (aicdev->msg_out_urb)
			usb_kill_urb(aicdev->msg_out_urb);
		bsp_err("Txmsg wait timed out\n");
		ret = -EIO;
		goto exit;
	}

	if (aicdev->msg_finished == false) {
		bsp_err("Txmsg timed out\n");
		ret = -ETIMEDOUT;
		goto exit;
	}
exit:
	clear_bit(0, &aicdev->msg_busy);
	return ret;
}

static void aicwf_usb_free_urb(struct list_head *q, spinlock_t *qlock)
{
	struct aicwf_usb_buf *usb_buf, *tmp;
	unsigned long flags;

	spin_lock_irqsave(qlock, flags);
	list_for_each_entry_safe(usb_buf, tmp, q, list) {
	spin_unlock_irqrestore(qlock, flags);
		if (!usb_buf->urb) {
			bsp_err("bad usb_buf\n");
			spin_lock_irqsave(qlock, flags);
			break;
		}
		usb_free_urb(usb_buf->urb);
		list_del_init(&usb_buf->list);
		spin_lock_irqsave(qlock, flags);
	}
	spin_unlock_irqrestore(qlock, flags);
}

static int aicwf_usb_alloc_rx_urb(struct priv_dev *aicdev)
{
	int i;

	for (i = 0; i < AICWF_USB_RX_URBS; i++) {
		struct aicwf_usb_buf *usb_buf = &aicdev->usb_rx_buf[i];

		usb_buf->aicdev = aicdev;
		usb_buf->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!usb_buf->urb) {
			bsp_err("could not allocate rx data urb\n");
			goto err;
		}
		list_add_tail(&usb_buf->list, &aicdev->rx_free_list);
	}
	return 0;

err:
	aicwf_usb_free_urb(&aicdev->rx_free_list, &aicdev->rx_free_lock);
	return -ENOMEM;
}

static int aicwf_usb_alloc_tx_urb(struct priv_dev *aicdev)
{
	int i;

	for (i = 0; i < AICWF_USB_TX_URBS; i++) {
		struct aicwf_usb_buf *usb_buf = &aicdev->usb_tx_buf[i];

		usb_buf->aicdev = aicdev;
		usb_buf->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!usb_buf->urb) {
			bsp_err("could not allocate tx data urb\n");
			goto err;
		}
		list_add_tail(&usb_buf->list, &aicdev->tx_free_list);
		(aicdev->tx_free_count)++;
	}
	return 0;

err:
	aicwf_usb_free_urb(&aicdev->tx_free_list, &aicdev->tx_free_lock);
	return -ENOMEM;
}

static void aicwf_usb_state_change(struct priv_dev *aicdev, int state)
{
	int old_state;

	if (aicdev->state == state)
		return;

	old_state = aicdev->state;
	aicdev->state = state;

	if (state == USB_DOWN_ST) {
		aicdev->bus_if->state = BUS_DOWN_ST;
	}
	if (state == USB_UP_ST) {
		aicdev->bus_if->state = BUS_UP_ST;
	}
}

static int aicwf_usb_bus_txdata(struct device *dev, struct sk_buff *skb)
{
	u8 *buf = NULL;
	u16 buf_len = 0;
	struct aicwf_usb_buf *usb_buf;
	int ret = 0;
	unsigned long flags;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;
	bool need_cfm = false;

	if (aicdev->state != USB_UP_ST) {
		bsp_err("usb state is not up!\n");
		dev_kfree_skb_any(skb);
		return -EIO;
	}

	usb_buf = aicwf_usb_tx_dequeue(aicdev, &aicdev->tx_free_list,
						&aicdev->tx_free_count, &aicdev->tx_free_lock);
	if (!usb_buf) {
		bsp_err("free:%d, post:%d\n", aicdev->tx_free_count, aicdev->tx_post_count);
		dev_kfree_skb_any(skb);
		ret = -ENOMEM;
		goto flow_ctrl;
	}

	usb_buf->aicdev = aicdev;
	if (need_cfm)
		usb_buf->cfm = true;
	else
		usb_buf->cfm = false;
	usb_fill_bulk_urb(usb_buf->urb, aicdev->udev, aicdev->bulk_out_pipe,
				buf, buf_len, aicwf_usb_tx_complete, usb_buf);
	usb_buf->urb->transfer_flags |= URB_ZERO_PACKET;

	aicwf_usb_tx_queue(aicdev, &aicdev->tx_post_list, usb_buf,
					&aicdev->tx_post_count, &aicdev->tx_post_lock);
	complete(&bus_if->bustx_trgg);
	ret = 0;

flow_ctrl:
	spin_lock_irqsave(&aicdev->tx_flow_lock, flags);
	if (aicdev->tx_free_count < AICWF_USB_TX_LOW_WATER) {
		aicdev->tbusy = true;
		aicwf_usb_tx_flowctrl(aicdev, true);
	}
	spin_unlock_irqrestore(&aicdev->tx_flow_lock, flags);

	return ret;
}

static int aicwf_usb_bus_start(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;

	if (aicdev->state == USB_UP_ST)
		return 0;

	aicwf_usb_state_change(aicdev, USB_UP_ST);
	aicwf_usb_rx_prepare(aicdev);
	aicwf_usb_tx_prepare(aicdev);
	return 0;
}

static void aicwf_usb_cancel_all_urbs(struct priv_dev *aicdev)
{
	struct aicwf_usb_buf *usb_buf, *tmp;
	unsigned long flags;

	if (aicdev->msg_out_urb)
		usb_kill_urb(aicdev->msg_out_urb);

	spin_lock_irqsave(&aicdev->tx_post_lock, flags);
	list_for_each_entry_safe(usb_buf, tmp, &aicdev->tx_post_list, list) {
		spin_unlock_irqrestore(&aicdev->tx_post_lock, flags);
		if (!usb_buf->urb) {
			bsp_err("bad usb_buf\n");
			spin_lock_irqsave(&aicdev->tx_post_lock, flags);
			break;
		}
		usb_kill_urb(usb_buf->urb);
		spin_lock_irqsave(&aicdev->tx_post_lock, flags);
	}
	spin_unlock_irqrestore(&aicdev->tx_post_lock, flags);

	usb_kill_anchored_urbs(&aicdev->rx_submitted);
}

static void aicwf_usb_bus_stop(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct priv_dev *aicdev = bus_if->bus_priv.dev;

	if (aicdev == NULL)
		return;

	if (aicdev->state == USB_DOWN_ST)
		return;

	aicwf_usb_state_change(aicdev, USB_DOWN_ST);
	aicwf_usb_cancel_all_urbs(aicdev);
}

static void aicwf_usb_deinit(struct priv_dev *aicdev)
{
	cancel_work_sync(&aicdev->rx_urb_work);
	aicwf_usb_free_urb(&aicdev->rx_free_list, &aicdev->rx_free_lock);
	aicwf_usb_free_urb(&aicdev->tx_free_list, &aicdev->tx_free_lock);
	usb_free_urb(aicdev->msg_out_urb);
}

static void aicwf_usb_rx_urb_work(struct work_struct *work)
{
	struct priv_dev *aicdev = container_of(work, struct priv_dev, rx_urb_work);

	aicwf_usb_rx_submit_all_urb(aicdev);
}

static int aicwf_usb_init(struct priv_dev *aicdev)
{
	int ret = 0;

	aicdev->tbusy = false;
	aicdev->app_cmp = false;
	aicdev->state = USB_DOWN_ST;

	init_waitqueue_head(&aicdev->msg_wait);
	init_usb_anchor(&aicdev->rx_submitted);

	spin_lock_init(&aicdev->tx_free_lock);
	spin_lock_init(&aicdev->tx_post_lock);
	spin_lock_init(&aicdev->rx_free_lock);
	spin_lock_init(&aicdev->tx_flow_lock);

	INIT_LIST_HEAD(&aicdev->rx_free_list);
	INIT_LIST_HEAD(&aicdev->tx_free_list);
	INIT_LIST_HEAD(&aicdev->tx_post_list);

	aicdev->tx_free_count = 0;
	aicdev->tx_post_count = 0;

	ret =  aicwf_usb_alloc_rx_urb(aicdev);
	if (ret) {
		goto error;
	}
	ret =  aicwf_usb_alloc_tx_urb(aicdev);
	if (ret) {
		goto error;
	}

	aicdev->msg_out_urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!aicdev->msg_out_urb) {
		bsp_err("usb_alloc_urb (msg out) failed\n");
		ret = ENOMEM;
		goto error;
	}

	INIT_WORK(&aicdev->rx_urb_work, aicwf_usb_rx_urb_work);
	return ret;

error:
	bsp_err("failed!\n");
	aicwf_usb_deinit(aicdev);
	return ret;
}

static int aicwf_parse_usb(struct priv_dev *aicdev, struct usb_interface *interface)
{
	struct usb_interface_descriptor *interface_desc;
	struct usb_host_interface *host_interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *usb = aicdev->udev;
	int i, endpoints;
	u8 endpoint_num;
	int ret = 0;

	aicdev->bulk_in_pipe = 0;
	aicdev->bulk_out_pipe = 0;

#ifdef CONFIG_USB_MSG_EP
	aicdev->msg_out_pipe = 0;
#endif

	host_interface = &interface->altsetting[0];
	interface_desc = &host_interface->desc;
	endpoints = interface_desc->bNumEndpoints;

	/* Check device configuration */
	if (usb->descriptor.bNumConfigurations != 1) {
		bsp_err("Number of configurations: %d not supported\n",
						usb->descriptor.bNumConfigurations);
		ret = -ENODEV;
		goto exit;
	}

	/* Check deviceclass */
	if (usb->descriptor.bDeviceClass != 0x00 && usb->descriptor.bDeviceClass != 239) {
		bsp_err("DeviceClass %d not supported\n",
			usb->descriptor.bDeviceClass);
		ret = -ENODEV;
		goto exit;
	}

	/* Check interface number */
	if (usb->actconfig->desc.bNumInterfaces != 1 && usb->actconfig->desc.bNumInterfaces != 3) {
		bsp_err("Number of interfaces: %d not supported\n",
			usb->actconfig->desc.bNumInterfaces);
		ret = -ENODEV;
		goto exit;
	}

	if ((interface_desc->bInterfaceClass != USB_CLASS_VENDOR_SPEC) ||
		(interface_desc->bInterfaceSubClass != 0xff) ||
		(interface_desc->bInterfaceProtocol != 0xff)) {
		bsp_err("non WLAN interface %d: 0x%x:0x%x:0x%x\n",
			interface_desc->bInterfaceNumber, interface_desc->bInterfaceClass,
			interface_desc->bInterfaceSubClass, interface_desc->bInterfaceProtocol);
		ret = -ENODEV;
		goto exit;
	}

	for (i = 0; i < endpoints; i++) {
		endpoint = &host_interface->endpoint[i].desc;
		endpoint_num = usb_endpoint_num(endpoint);

		if (usb_endpoint_dir_in(endpoint) &&
			usb_endpoint_xfer_bulk(endpoint)) {
			if (!aicdev->bulk_in_pipe) {
				aicdev->bulk_in_pipe = usb_rcvbulkpipe(usb, endpoint_num);
			}
		}

		if (usb_endpoint_dir_out(endpoint) &&
			usb_endpoint_xfer_bulk(endpoint)) {
			if (!aicdev->bulk_out_pipe) {
				aicdev->bulk_out_pipe = usb_sndbulkpipe(usb, endpoint_num);
#ifdef CONFIG_USB_MSG_EP
			} else if (!aicdev->msg_out_pipe) {
				aicdev->msg_out_pipe = usb_sndbulkpipe(usb, endpoint_num);
#endif
			}
		}
	}

	if (aicdev->bulk_in_pipe == 0) {
		bsp_err("No RX (in) Bulk EP found\n");
		ret = -ENODEV;
		goto exit;
	}
	if (aicdev->bulk_out_pipe == 0) {
		bsp_err("No TX (out) Bulk EP found\n");
		ret = -ENODEV;
		goto exit;
	}

#ifdef CONFIG_USB_MSG_EP
	if (aicdev->msg_out_pipe == 0) {
		bsp_err("No TX Msg (out) Bulk EP found\n");
		aicdev->use_msg_ep = 0;
	} else {
		aicdev->use_msg_ep = 1;
	}
#endif

	if (usb->speed == USB_SPEED_HIGH)
		bsp_dbg("Aic high speed USB device detected\n");
	else
		bsp_dbg("Aic full speed USB device detected\n");

exit:
	return ret;
}

static struct aicwf_bus_ops aicwf_usb_bus_ops = {
	.start = aicwf_usb_bus_start,
	.stop = aicwf_usb_bus_stop,
	.txdata = aicwf_usb_bus_txdata,
	.txmsg = aicwf_usb_bus_txmsg,
};

static int aicwf_usb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	int ret = 0;
	struct usb_device *usb = interface_to_usbdev(intf);
	struct aicwf_bus *bus_if ;
	struct device *dev = NULL;
	struct aicwf_rx_priv *rx_priv = NULL;
	struct priv_dev *aicdev = NULL;
	int i = 0;

	bsp_dbg("%s vid:0x%X pid:0x%X icl:0x%X isc:0x%X ipr:0x%X\n", __func__,
		id->idVendor,
		id->idProduct,
		id->bInterfaceClass,
		id->bInterfaceSubClass,
		id->bInterfaceProtocol);

	aic_matched_ic = NULL;
	for (i = 0; i < sizeof(aicdev_match_table) / sizeof(aicdev_match_table[0]); i++) {
		if (id->idVendor == aicdev_match_table[i].vid && id->idProduct == aicdev_match_table[i].pid) {
			aic_matched_ic = &aicdev_match_table[i];
			break;
		}
	}

	bsp_dbg("%s, matched chip: %s\n", __func__, aic_matched_ic ? aic_matched_ic->name : "none");
	if (aic_matched_ic == NULL) {
		bsp_dbg("%s device not in bootloader mode, exit...\n", __func__);
		return -1;
	}

	aicdev = kzalloc(sizeof(struct priv_dev), GFP_ATOMIC);
	if (!aicdev) {
		return -ENOMEM;
	}

	aicbsp_info.chipinfo = aic_matched_ic;
	aicdev->udev = usb;
	aicdev->dev = &usb->dev;
	usb_set_intfdata(intf, aicdev);

	ret = aicwf_parse_usb(aicdev, intf);
	if (ret) {
		bsp_err("aicwf_parse_usb err %d\n", ret);
		goto out_free;
	}

	ret = aicwf_usb_init(aicdev);
	if (ret) {
		bsp_err("aicwf_usb_init err %d\n", ret);
		goto out_free;
	}

	bus_if = kzalloc(sizeof(struct aicwf_bus), GFP_ATOMIC);
	if (!bus_if) {
		ret = -ENOMEM;
		goto out_free_usb;
	}

	dev = aicdev->dev;
	bus_if->dev = dev;
	aicdev->bus_if = bus_if;
	bus_if->bus_priv.dev = aicdev;
	dev_set_drvdata(dev, bus_if);

	bus_if->ops = &aicwf_usb_bus_ops;

	rx_priv = aicwf_rx_init(aicdev);
	if (!rx_priv) {
		bsp_err("rx init failed\n");
		ret = -1;
		goto out_free_bus;
	}
	aicdev->rx_priv = rx_priv;

	ret = aicwf_bus_init(0, dev);
	if (ret < 0) {
		bsp_err("aicwf_bus_init err %d\n", ret);
		goto out_free_bus;
	}

	ret = aicwf_bus_start(bus_if);
	if (ret < 0) {
		bsp_err("aicwf_bus_start err %d\n", ret);
		goto out_free_bus;
	}

	aicbsp_platform_init(aicdev);

	if (usb->speed != USB_SPEED_HIGH) {
		printk("Aic full speed USB device detected\n");
		aicbsp_system_reboot(aicdev);
		goto out_free_bus;
	}

	if (fw_loaded == 0 && id->idProduct == USB_DEVICE_ID_AIC_8801) {
		rwnx_send_dbg_start_app_req(aicdev, 2000, HOST_START_APP_REBOOT, NULL);
		goto out_free_bus;
	}

	if (aicbsp_driver_fw_init(aicdev))
		goto out_free_bus;

	aicdev->app_cmp = true;
	fw_loaded = 1;

	return 0;

out_free_bus:
	aicwf_bus_deinit(dev);
	kfree(bus_if);
out_free_usb:
	aicwf_usb_deinit(aicdev);
out_free:
	bsp_err("failed with errno %d\n", ret);
	kfree(aicdev);
	usb_set_intfdata(intf, NULL);
	return ret;
}

static void aicwf_usb_disconnect(struct usb_interface *intf)
{
	struct priv_dev *aicdev =
			(struct priv_dev *) usb_get_intfdata(intf);

	if (!aicdev)
		return;

	aicwf_bus_deinit(aicdev->dev);
	aicwf_usb_deinit(aicdev);
	if (aicdev->rx_priv)
		aicwf_rx_deinit(aicdev->rx_priv);
	kfree(aicdev->bus_if);
	kfree(aicdev);
}

static int aicwf_usb_suspend(struct usb_interface *intf, pm_message_t state)
{
	struct priv_dev *aicdev =
		(struct priv_dev *) usb_get_intfdata(intf);

	aicwf_usb_state_change(aicdev, USB_SLEEP_ST);
	aicwf_bus_stop(aicdev->bus_if);
	return 0;
}

static int aicwf_usb_resume(struct usb_interface *intf)
{
	struct priv_dev *aicdev =
		(struct priv_dev *) usb_get_intfdata(intf);

	if (aicdev->state == USB_UP_ST)
		return 0;

	aicwf_bus_start(aicdev->bus_if);
	return 0;
}

static int aicwf_usb_reset_resume(struct usb_interface *intf)
{
	return aicwf_usb_resume(intf);
}

static struct usb_device_id aicwf_usb_id_table[] = {
	{USB_DEVICE(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC)},
	{USB_DEVICE(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8801)},
	{}
};

MODULE_DEVICE_TABLE(usb, aicwf_usb_id_table);

static struct usb_driver aicwf_usbdrvr = {
	.name = KBUILD_MODNAME,
	.probe = aicwf_usb_probe,
	.disconnect = aicwf_usb_disconnect,
	.id_table = aicwf_usb_id_table,
	.suspend = aicwf_usb_suspend,
	.resume = aicwf_usb_resume,
	.reset_resume = aicwf_usb_reset_resume,
	.supports_autosuspend = 0,
	.disable_hub_initiated_lpm = 1,
};

void aicwf_usb_register(void)
{
	if (usb_register(&aicwf_usbdrvr) < 0) {
		bsp_err("usb_register failed\n");
	}
}

void aicwf_usb_exit(void)
{
	usb_deregister(&aicwf_usbdrvr);
}
