/**
 * aicwf_usb.c
 *
 * USB function declarations
 *
 * Copyright (C) AICSemi 2018-2020
 */

#include <linux/usb.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include "aicwf_txrxif.h"
#include "aicwf_usb.h"
#include "rwnx_tx.h"
#include "rwnx_defs.h"
#include "usb_host.h"
#include "rwnx_platform.h"
#include "rwnx_wakelock.h"

struct device_match_entry *aic_matched_ic;
struct aicbsp_feature_t aicwf_feature;

static struct device_match_entry aicdev_match_table[] = {
//	{USB_VENDOR_ID_AIC,		USB_DEVICE_ID_AIC_8800,		PRODUCT_ID_AIC8800D,	"aic8800d",		0, 0}, // 8800d in bootloader mode
	{USB_VENDOR_ID_AIC,		USB_DEVICE_ID_AIC_8801,		PRODUCT_ID_AIC8801,		"aic8801",		0, 0}, // 8801 in bootloader mode
//	{USB_VENDOR_ID_AIC,		USB_DEVICE_ID_AIC_8800D80,	PRODUCT_ID_AIC8800D80,	"aic8800d80",	0, 0}, // 8800d80 in bootloader mode
	{USB_VENDOR_ID_AIC,		USB_DEVICE_ID_AIC_8800D81,	PRODUCT_ID_AIC8800D81,	"aic8800d81",	0, 0}, // 8800d81 in bootloader mode
//	{USB_VENDOR_ID_AIC,		USB_DEVICE_ID_AIC_8800D40,	PRODUCT_ID_AIC8800D80,	"aic8800d40",	0, 0}, // 8800d40 in bootloader mode
	{USB_VENDOR_ID_AIC,		USB_DEVICE_ID_AIC_8800D41,	PRODUCT_ID_AIC8800D81,	"aic8800d41",	0, 0}, // 8800d41 in bootloader mode
//	{USB_VENDOR_ID_AIC_V2,	USB_DEVICE_ID_AIC_8800D80X2,PRODUCT_ID_AIC8800D80X2,"aic8800d80x2",	0, 0}, // 8800d80x2 in bootloader mode
	{USB_VENDOR_ID_AIC_V2,	USB_DEVICE_ID_AIC_8800D81X2,PRODUCT_ID_AIC8800D81X2,"aic8800d81x2",	0, 0}, // 8800d81x2 in bootloader mode
};

atomic_t rx_urb_cnt;
bool rx_urb_sched = false;

bool aicwf_usb_rx_aggr = false;

extern struct semaphore aicwf_deinit_sem;
extern atomic_t aicwf_deinit_atomic;
#define SEM_TIMOUT 2000

void aicwf_usb_tx_flowctrl(struct rwnx_hw *rwnx_hw, bool state)
{
	struct rwnx_vif *rwnx_vif;

	list_for_each_entry(rwnx_vif, &rwnx_hw->vifs, list) {
		if (!rwnx_vif || !rwnx_vif->ndev || !rwnx_vif->up)
			continue;
		if (state)
			netif_tx_stop_all_queues(rwnx_vif->ndev);//netif_stop_queue(rwnx_vif->ndev);
		else
			netif_tx_wake_all_queues(rwnx_vif->ndev);//netif_wake_queue(rwnx_vif->ndev);
	}
}

static struct aicwf_usb_buf *aicwf_usb_tx_dequeue(struct aic_usb_dev *usb_dev,
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

static void aicwf_usb_tx_queue(struct aic_usb_dev *usb_dev,
	struct list_head *q, struct aicwf_usb_buf *usb_buf, int *counter,
	spinlock_t *qlock)
{
	unsigned long flags;

	spin_lock_irqsave(qlock, flags);
	list_add_tail(&usb_buf->list, q);
	(*counter)++;
	spin_unlock_irqrestore(qlock, flags);
}

static struct aicwf_usb_buf *aicwf_usb_rx_buf_get(struct aic_usb_dev *usb_dev)
{
	unsigned long flags;
	struct aicwf_usb_buf *usb_buf;

	spin_lock_irqsave(&usb_dev->rx_free_lock, flags);
	if (list_empty(&usb_dev->rx_free_list)) {
		usb_buf = NULL;
	} else {
		usb_buf = list_first_entry(&usb_dev->rx_free_list, struct aicwf_usb_buf, list);
		list_del_init(&usb_buf->list);
	}
	spin_unlock_irqrestore(&usb_dev->rx_free_lock, flags);
	return usb_buf;
}

static void aicwf_usb_rx_buf_put(struct aic_usb_dev *usb_dev, struct aicwf_usb_buf *usb_buf)
{
	unsigned long flags;

	spin_lock_irqsave(&usb_dev->rx_free_lock, flags);
	list_add_tail(&usb_buf->list, &usb_dev->rx_free_list);
	spin_unlock_irqrestore(&usb_dev->rx_free_lock, flags);
}

#ifdef CONFIG_USB_MSG_IN_EP
static struct aicwf_usb_buf *aicwf_usb_msg_rx_buf_get(struct aic_usb_dev *usb_dev)
{
	unsigned long flags;
	struct aicwf_usb_buf *usb_buf;

	spin_lock_irqsave(&usb_dev->msg_rx_free_lock, flags);
	if (list_empty(&usb_dev->msg_rx_free_list)) {
		usb_buf = NULL;
	} else {
		usb_buf = list_first_entry(&usb_dev->msg_rx_free_list, struct aicwf_usb_buf, list);
		list_del_init(&usb_buf->list);
	}
	spin_unlock_irqrestore(&usb_dev->msg_rx_free_lock, flags);
	return usb_buf;
}

static void aicwf_usb_msg_rx_buf_put(struct aic_usb_dev *usb_dev, struct aicwf_usb_buf *usb_buf)
{
	unsigned long flags;

	spin_lock_irqsave(&usb_dev->msg_rx_free_lock, flags);
	list_add_tail(&usb_buf->list, &usb_dev->msg_rx_free_list);
	spin_unlock_irqrestore(&usb_dev->msg_rx_free_lock, flags);
}
#endif

static void aicwf_usb_tx_complete(struct urb *urb)
{
	unsigned long flags;
	struct aicwf_usb_buf *usb_buf = (struct aicwf_usb_buf *) urb->context;
	struct aic_usb_dev *usb_dev = usb_buf->usbdev;
	struct sk_buff *skb;
	u8 *buf;

#ifdef CONFIG_USB_ALIGN_DATA
	if(usb_buf->usb_align_data) {
		kfree(usb_buf->usb_align_data);
		usb_buf->usb_align_data = NULL;
	}
#endif

	if (usb_buf->cfm == false) {
		skb = usb_buf->skb;
		dev_kfree_skb_any(skb);
	} else {
		buf = (u8 *)usb_buf->skb;
		kfree(buf);
	}
	usb_buf->skb = NULL;

	aicwf_usb_tx_queue(usb_dev, &usb_dev->tx_free_list, usb_buf,
					&usb_dev->tx_free_count, &usb_dev->tx_free_lock);

	spin_lock_irqsave(&usb_dev->tx_flow_lock, flags);
	if (usb_dev->tx_free_count > AICWF_USB_TX_HIGH_WATER) {
		if (usb_dev->tbusy) {
			usb_dev->tbusy = false;
			aicwf_usb_tx_flowctrl(usb_dev->rwnx_hw, false);
		}
	}
	spin_unlock_irqrestore(&usb_dev->tx_flow_lock, flags);
	}

static void aicwf_usb_rx_complete(struct urb *urb)
{
	struct aicwf_usb_buf *usb_buf = (struct aicwf_usb_buf *) urb->context;
	struct aic_usb_dev *usb_dev = usb_buf->usbdev;
	struct aicwf_rx_priv *rx_priv = usb_dev->rx_priv;
	struct sk_buff *skb = NULL;
	unsigned long flags = 0;

	skb = usb_buf->skb;
	usb_buf->skb = NULL;

	atomic_dec(&rx_urb_cnt);
	if(atomic_read(&rx_urb_cnt) < 10){
		usb_dbg("%s %d \r\n", __func__, atomic_read(&rx_urb_cnt));
	}

	if(!usb_dev->rwnx_hw){
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
		usb_err("usb_dev->rwnx_hw is not ready \r\n");
		return;
	}
	if (urb->actual_length > urb->transfer_buffer_length) {
		usb_err("urb_rx len error %u/%u\n", urb->actual_length, urb->transfer_buffer_length);
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
		aicwf_usb_rx_submit_all_urb_(usb_dev);
		return;
	}

	if (urb->status != 0 || !urb->actual_length) {
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
		if(urb->status < 0){
			usb_dbg("%s urb->status:%d \r\n", __func__, urb->status);

			if(g_rwnx_plat->wait_disconnect_cb == false){
				g_rwnx_plat->wait_disconnect_cb = true;
				if(atomic_read(&aicwf_deinit_atomic) > 0){
					atomic_set(&aicwf_deinit_atomic, 0);
					down(&aicwf_deinit_sem);
					usb_info("%s need to wait for disconnect callback \r\n", __func__);
				}else{
					g_rwnx_plat->wait_disconnect_cb = false;
				}
			}

			return;
		}else{
			//schedule_work(&usb_dev->rx_urb_work);
			aicwf_usb_rx_submit_all_urb_(usb_dev);
		return;
		}
	}

	if (usb_dev->state == USB_UP_ST) {
		skb_put(skb, urb->actual_length);

		if (aicwf_usb_rx_aggr) {
			skb->len = urb->actual_length;
		}

		spin_lock_irqsave(&rx_priv->rxqlock, flags);
		if (!aicwf_rxframe_enqueue(usb_dev->dev, &rx_priv->rxq, skb)) {
			spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
			usb_err("rx_priv->rxq is over flow!!!\n");
			aicwf_dev_skb_free(skb);
			aicwf_usb_rx_buf_put(usb_dev, usb_buf);
			aicwf_usb_rx_submit_all_urb_(usb_dev);
			return;
		}
		spin_unlock_irqrestore(&rx_priv->rxqlock, flags);
		atomic_inc(&rx_priv->rx_cnt);
		if(atomic_read(&rx_priv->rx_cnt) == 1){
				complete(&rx_priv->usbdev->bus_if->busrx_trgg);
		}
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
		aicwf_usb_rx_submit_all_urb_(usb_dev);
		//schedule_work(&usb_dev->rx_urb_work);
	} else {
		aicwf_dev_skb_free(skb);
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
	}
}

#ifdef CONFIG_USB_MSG_IN_EP
static void aicwf_usb_msg_rx_complete(struct urb *urb)
{
    struct aicwf_usb_buf *usb_buf = (struct aicwf_usb_buf *) urb->context;
    struct aic_usb_dev *usb_dev = usb_buf->usbdev;
    struct aicwf_rx_priv* rx_priv = usb_dev->rx_priv;
    struct sk_buff *skb = NULL;
    unsigned long flags = 0;

    skb = usb_buf->skb;
    usb_buf->skb = NULL;

    if (urb->actual_length > urb->transfer_buffer_length) {
        usb_err("usb_msg_rx len error %u/%u\n", urb->actual_length, urb->transfer_buffer_length);
        aicwf_dev_skb_free(skb);
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
		aicwf_usb_msg_rx_submit_all_urb_(usb_dev);
        //schedule_work(&usb_dev->msg_rx_urb_work);
        return;
    }

    if (urb->status != 0 || !urb->actual_length) {
        aicwf_dev_skb_free(skb);
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);

		if(urb->status < 0){
			usb_dbg("%s urb->status:%d \r\n", __func__, urb->status);
			return;
		}else{
			aicwf_usb_msg_rx_submit_all_urb_(usb_dev);
			//schedule_work(&usb_dev->msg_rx_urb_work);
        	return;
		}
    }

    if (usb_dev->state == USB_UP_ST) {
        skb_put(skb, urb->actual_length);

#ifdef CONFIG_USB_RX_REASSEMBLE
        bool pkt_check = false;
        if (rx_priv->rx_msg_reassemble_skb) {
            u32 frag_len = skb->len;
            struct sk_buff *reassemble_skb = rx_priv->rx_msg_reassemble_skb;
            bool reassemble_valid = false;
            bool reassemble_done = false;
            if ((rx_priv->rx_msg_reassemble_cur_frags + 1) == rx_priv->rx_msg_reassemble_total_frags) {
                if ((rx_priv->rx_msg_reassemble_cur_len + frag_len) == rx_priv->rx_msg_reassemble_total_len) {
                    reassemble_valid = true;
                    reassemble_done = true;
                }
            } else {
                if (frag_len == AICWF_USB_MSG_MAX_PKT_SIZE) {
                    reassemble_valid = true;
                }
            }

            if (reassemble_valid) {
                memcpy((reassemble_skb->data + reassemble_skb->len), skb->data, frag_len);
                skb_put(reassemble_skb, skb->len);
                rx_priv->rx_msg_reassemble_cur_len += frag_len;
                rx_priv->rx_msg_reassemble_cur_frags++;
                aicwf_dev_skb_free(skb);
                if (reassemble_done) {
                    skb = reassemble_skb;
                    rx_priv->rx_msg_reassemble_skb = NULL;
                    rx_priv->rx_msg_reassemble_total_len = 0;
                    rx_priv->rx_msg_reassemble_cur_len = 0;
                    rx_priv->rx_msg_reassemble_total_frags = 0;
                    rx_priv->rx_msg_reassemble_cur_frags = 0;
                } else {
                    aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
                    aicwf_usb_msg_rx_submit_all_urb_(usb_dev);
                    return;
                }
            } else {
                usb_err("invalid frag msg pkt, len=%u/%u/%u, frags=%u/%u\n", frag_len,
                    rx_priv->rx_msg_reassemble_cur_len, rx_priv->rx_msg_reassemble_cur_len,
                    rx_priv->rx_msg_reassemble_cur_frags, rx_priv->rx_msg_reassemble_total_frags);
                aicwf_dev_skb_free(reassemble_skb);
                rx_priv->rx_msg_reassemble_skb = NULL;
                rx_priv->rx_msg_reassemble_total_len = 0;
                rx_priv->rx_msg_reassemble_cur_len = 0;
                rx_priv->rx_msg_reassemble_total_frags = 0;
                rx_priv->rx_msg_reassemble_cur_frags = 0;
                pkt_check = true;
            }
        } else {
            pkt_check = true;
        }

        if (pkt_check) {
            bool pkt_drop = false;
            u8 type = skb->data[2];
            u32 pkt_len = skb->data[0] | (skb->data[1] << 8);
            if ((type & USB_TYPE_CFG) != USB_TYPE_CFG) {
                usb_err("invalid msg pkt, type=0x%x, len=%u/%u\n", type, pkt_len, skb->len);;
                pkt_drop = true;
            } else {
                if (type == USB_TYPE_CFG_CMD_RSP) {
                    u32 pkt_total_len = ALIGN((pkt_len + 4), 4);
                    if ((pkt_total_len > AICWF_USB_MSG_MAX_PKT_SIZE) && (skb->len == AICWF_USB_MSG_MAX_PKT_SIZE)) {
                        AICWFDBG(LOGINFO, "reassemble msg pkt, len=%u\n", pkt_total_len);
                        struct sk_buff *reassemble_skb = __dev_alloc_skb(pkt_total_len, GFP_ATOMIC/*GFP_KERNEL*/);
                        if (reassemble_skb) {
                            memcpy(reassemble_skb->data, skb->data, skb->len);
                            skb_put(reassemble_skb, skb->len);
                            rx_priv->rx_msg_reassemble_skb = reassemble_skb;
                            rx_priv->rx_msg_reassemble_total_len = pkt_total_len;
                            rx_priv->rx_msg_reassemble_cur_len = skb->len;
                            rx_priv->rx_msg_reassemble_total_frags = ALIGN(pkt_total_len, AICWF_USB_MSG_MAX_PKT_SIZE) / AICWF_USB_MSG_MAX_PKT_SIZE;
                            rx_priv->rx_msg_reassemble_cur_frags = 1;
                        } else {
                            usb_err("reassemble msg pkt alloc fail, len=%u\n", pkt_total_len);
                        }
                        aicwf_dev_skb_free(skb);
                        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
                        aicwf_usb_msg_rx_submit_all_urb_(usb_dev);
                        return;
                    } else if (pkt_total_len != skb->len) {
                        usb_err("invalid CMD_RSP, len=%u/%u\n", pkt_len, skb->len);
                        pkt_drop = true;
                    }
                } else if (type == USB_TYPE_CFG_DATA_CFM) {
                    if (!((pkt_len == 8) && (skb->len == 12))) {
                        usb_err("invalid DATA_CFM, len=%u/%u\n", pkt_len, skb->len);
                        pkt_drop = true;
                    }
                } else {
                    usb_err("invalid msg pkt, type=0x%x, len=%u/%u\n", type, pkt_len, skb->len);
                    pkt_drop = true;
                }
            }
            if (pkt_drop) {
                aicwf_dev_skb_free(skb);
                aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
                aicwf_usb_msg_rx_submit_all_urb_(usb_dev);
                return;
            }
        }
#endif

        spin_lock_irqsave(&rx_priv->msg_rxqlock, flags);
        if(!aicwf_rxframe_enqueue(usb_dev->dev, &rx_priv->msg_rxq, skb)){
            spin_unlock_irqrestore(&rx_priv->msg_rxqlock, flags);
            usb_err("rx_priv->rxq is over flow!!!\n");
            aicwf_dev_skb_free(skb);
            return;
        }
        spin_unlock_irqrestore(&rx_priv->msg_rxqlock, flags);
        atomic_inc(&rx_priv->msg_rx_cnt);
        complete(&rx_priv->usbdev->bus_if->msg_busrx_trgg);
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
        aicwf_usb_msg_rx_submit_all_urb_(usb_dev);
        //schedule_work(&usb_dev->msg_rx_urb_work);
    } else {
        aicwf_dev_skb_free(skb);
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
    }
}
#endif

static int aicwf_usb_submit_rx_urb(struct aic_usb_dev *usb_dev,
				struct aicwf_usb_buf *usb_buf)
{
	struct sk_buff *skb;
	int ret;

	if (!usb_buf || !usb_dev)
		return -1;

	if (usb_dev->state != USB_UP_ST) {
		usb_err("usb state is not up!\n");
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
		return -1;
	}

	if(aicwf_usb_rx_aggr){
		skb = __dev_alloc_skb(AICWF_USB_AGGR_MAX_PKT_SIZE, GFP_ATOMIC/*GFP_KERNEL*/);
	} else {
		 skb = __dev_alloc_skb(AICWF_USB_MAX_PKT_SIZE, GFP_ATOMIC/*GFP_KERNEL*/);
	}
	if (!skb) {
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);
		return -1;
	}

	usb_buf->skb = skb;

	if (aicwf_usb_rx_aggr) {
		usb_fill_bulk_urb(usb_buf->urb,
			usb_dev->udev,
			usb_dev->bulk_in_pipe,
			skb->data, AICWF_USB_AGGR_MAX_PKT_SIZE, aicwf_usb_rx_complete, usb_buf);
	} else {
		usb_fill_bulk_urb(usb_buf->urb,
			usb_dev->udev,
			usb_dev->bulk_in_pipe,
			skb->data, AICWF_USB_MAX_PKT_SIZE, aicwf_usb_rx_complete, usb_buf);
	}

	usb_buf->usbdev = usb_dev;

	usb_anchor_urb(usb_buf->urb, &usb_dev->rx_submitted);
	ret = usb_submit_urb(usb_buf->urb, GFP_ATOMIC);
	if (ret) {
		usb_err("usb submit rx urb fail:%d\n", ret);
		usb_unanchor_urb(usb_buf->urb);
		aicwf_dev_skb_free(usb_buf->skb);
		usb_buf->skb = NULL;
		aicwf_usb_rx_buf_put(usb_dev, usb_buf);

		msleep(100);
		return -1;
	}else{
		atomic_inc(&rx_urb_cnt);
	}
	return 0;
}

static void aicwf_usb_rx_submit_all_urb(struct aic_usb_dev *usb_dev)
{
	struct aicwf_usb_buf *usb_buf;

	if (usb_dev->state != USB_UP_ST) {
		usb_err("bus is not up=%d\n", usb_dev->state);
		return;
	}

	while ((usb_buf = aicwf_usb_rx_buf_get(usb_dev)) != NULL) {
		if (aicwf_usb_submit_rx_urb(usb_dev, usb_buf)) {
            usb_err("sub rx fail\n");
            return;
            #if 0
			usb_err("usb rx refill fail\n");
			if (usb_dev->state != USB_UP_ST)
				return;
            #endif
		}
	}
    usb_dev->rx_prepare_ready = true;
}

#ifdef CONFIG_USB_MSG_IN_EP
static int aicwf_usb_submit_msg_rx_urb(struct aic_usb_dev *usb_dev,
                struct aicwf_usb_buf *usb_buf)
{
    struct sk_buff *skb;
    int ret;

    if (!usb_buf || !usb_dev)
        return -1;

    if (usb_dev->state != USB_UP_ST) {
        usb_err("usb state is not up!\n");
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
        return -1;
    }

    skb = __dev_alloc_skb(AICWF_USB_MSG_MAX_PKT_SIZE, GFP_ATOMIC);
    if (!skb) {
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);
        return -1;
    }

    usb_buf->skb = skb;

    usb_fill_bulk_urb(usb_buf->urb,
        usb_dev->udev,
        usb_dev->msg_in_pipe,
        skb->data, AICWF_USB_MSG_MAX_PKT_SIZE, aicwf_usb_msg_rx_complete, usb_buf);

    usb_buf->usbdev = usb_dev;

    usb_anchor_urb(usb_buf->urb, &usb_dev->msg_rx_submitted);
    ret = usb_submit_urb(usb_buf->urb, GFP_ATOMIC);
    if (ret) {
        usb_err("usb submit msg rx urb fail:%d\n", ret);
        usb_unanchor_urb(usb_buf->urb);
        aicwf_dev_skb_free(usb_buf->skb);
        usb_buf->skb = NULL;
        aicwf_usb_msg_rx_buf_put(usb_dev, usb_buf);

        msleep(100);
    }
    return 0;
}


static void aicwf_usb_msg_rx_submit_all_urb(struct aic_usb_dev *usb_dev)
{
    struct aicwf_usb_buf *usb_buf;

    if (usb_dev->state != USB_UP_ST) {
        usb_err("bus is not up=%d\n", usb_dev->state);
        return;
    }

    while((usb_buf = aicwf_usb_msg_rx_buf_get(usb_dev)) != NULL) {
        if (aicwf_usb_submit_msg_rx_urb(usb_dev, usb_buf)) {
            usb_err("usb msg rx refill fail\n");
            if (usb_dev->state != USB_UP_ST)
                return;
        }
    }
}
#endif

#ifdef CONFIG_USB_MSG_IN_EP
void aicwf_usb_msg_rx_submit_all_urb_(struct aic_usb_dev *usb_dev){
	aicwf_usb_msg_rx_submit_all_urb(usb_dev);
}

static void aicwf_usb_msg_rx_prepare(struct aic_usb_dev *usb_dev)
{
    aicwf_usb_msg_rx_submit_all_urb(usb_dev);
}

#endif

void aicwf_usb_rx_submit_all_urb_(struct aic_usb_dev *usb_dev){
	aicwf_usb_rx_submit_all_urb(usb_dev);
}
static void aicwf_usb_rx_prepare(struct aic_usb_dev *usb_dev)
{
	aicwf_usb_rx_submit_all_urb(usb_dev);
}

static void aicwf_usb_tx_prepare(struct aic_usb_dev *usb_dev)
{
	struct aicwf_usb_buf *usb_buf;

	while (!list_empty(&usb_dev->tx_post_list)) {
		usb_buf = aicwf_usb_tx_dequeue(usb_dev, &usb_dev->tx_post_list,
			&usb_dev->tx_post_count, &usb_dev->tx_post_lock);
		if (usb_buf->skb) {
			dev_kfree_skb(usb_buf->skb);
			usb_buf->skb = NULL;
		}
		aicwf_usb_tx_queue(usb_dev, &usb_dev->tx_free_list, usb_buf,
				&usb_dev->tx_free_count, &usb_dev->tx_free_lock);
	}
}
static void aicwf_usb_tx_process(struct aic_usb_dev *usb_dev)
{
	struct aicwf_usb_buf *usb_buf;
	int ret = 0;
	u8 *data = NULL;

	while (!list_empty(&usb_dev->tx_post_list)) {
		if (usb_dev->state != USB_UP_ST) {
			usb_err("usb state is not up!\n");
			return;
		}

		usb_buf = aicwf_usb_tx_dequeue(usb_dev, &usb_dev->tx_post_list,
						&usb_dev->tx_post_count, &usb_dev->tx_post_lock);
		if (!usb_buf) {
			usb_err("can not get usb_buf from tx_post_list!\n");
			return;
		}
		data = usb_buf->skb->data;

		ret = usb_submit_urb(usb_buf->urb, GFP_KERNEL);
		if (ret) {
			usb_err("aicwf_usb_bus_tx usb_submit_urb FAILED\n");
			goto fail;
		}

		continue;
fail:
		dev_kfree_skb(usb_buf->skb);
		usb_buf->skb = NULL;
		aicwf_usb_tx_queue(usb_dev, &usb_dev->tx_free_list, usb_buf,
					&usb_dev->tx_free_count, &usb_dev->tx_free_lock);
	}
}

static inline void aic_thread_wait_stop(void)
{
#if 1// PLATFORM_LINUX
	#if 0
	while (!kthread_should_stop())
		rtw_msleep_os(10);
	#else
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	#endif
#endif
}

int usb_bustx_thread(void *data)
{
	struct aicwf_bus *bus = (struct aicwf_bus *)data;
	struct aic_usb_dev *usbdev = bus->bus_priv.usb;

	while (1) {
		#if 0
		if (kthread_should_stop()) {
			usb_err("usb bustx thread stop\n");
			break;
		}
		#endif
		if (!wait_for_completion_interruptible(&bus->bustx_trgg)) {
			if (usbdev->bus_if->state == BUS_DOWN_ST) {
				usb_info("usb bustx thread will to stop\n");
				break;
			}
			rwnx_wakeup_lock(usbdev->rwnx_hw->ws_tx);
			if (usbdev->tx_post_count > 0)
				aicwf_usb_tx_process(usbdev);
			rwnx_wakeup_unlock(usbdev->rwnx_hw->ws_tx);
		}
	}

	aic_thread_wait_stop();
	usb_info("usb bustx thread stop\n");

	return 0;
}

int usb_busrx_thread(void *data)
{
	struct aicwf_rx_priv *rx_priv = (struct aicwf_rx_priv *)data;
	struct aicwf_bus *bus_if = rx_priv->usbdev->bus_if;
	struct aic_usb_dev *usbdev = rx_priv->usbdev;

	while (1) {
		#if 0
		if (kthread_should_stop()) {
			usb_err("usb busrx thread stop\n");
			break;
		}
		#endif
		if (!wait_for_completion_interruptible(&bus_if->busrx_trgg)) {
			if (bus_if->state == BUS_DOWN_ST) {
				usb_info("usb busrx thread will to stop\n");
				break;
			}
			rwnx_wakeup_lock(usbdev->rwnx_hw->ws_rx);
			aicwf_process_rxframes(rx_priv);
			rwnx_wakeup_unlock(usbdev->rwnx_hw->ws_rx);
		}
	}

	aic_thread_wait_stop();
	usb_info("usb busrx thread stop\n");

	return 0;
}

#ifdef CONFIG_USB_MSG_IN_EP
int usb_msg_busrx_thread(void *data)
{
    struct aicwf_rx_priv *rx_priv = (struct aicwf_rx_priv *)data;
    struct aicwf_bus *bus_if = rx_priv->usbdev->bus_if;

#ifdef CONFIG_TXRX_THREAD_PRIO
	if (busrx_thread_prio > 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0))
        sched_set_fifo_low(current);
#else
        struct sched_param param;
        param.sched_priority = (busrx_thread_prio < MAX_RT_PRIO)?busrx_thread_prio:(MAX_RT_PRIO-1);
        sched_setscheduler(current, SCHED_FIFO, &param);
#endif
	}
#endif
	usb_info("%s the policy of current thread is:%d\n", __func__, current->policy);
	usb_info("%s the rt_priority of current thread is:%d\n", __func__, current->rt_priority);
	usb_info("%s the current pid is:%d\n", __func__, current->pid);

    while (1) {
		#if 0
		if(kthread_should_stop()) {
			usb_err("usb msg busrx thread stop\n");
			break;
		}
		#endif
		if (!wait_for_completion_interruptible(&bus_if->msg_busrx_trgg)) {
			if(bus_if->state == BUS_DOWN_ST){
				usb_info("usb msg busrx thread will to stop\n");
				break;
			}
			aicwf_process_msg_rxframes(rx_priv);
		}
	}

	aic_thread_wait_stop();
	usb_info("usb msg busrx thread stop\n");

    return 0;
}
#endif

static void aicwf_usb_send_msg_complete(struct urb *urb)
{
	struct aic_usb_dev *usb_dev = (struct aic_usb_dev *) urb->context;

	usb_dev->msg_finished = true;
	if (waitqueue_active(&usb_dev->msg_wait))
		wake_up(&usb_dev->msg_wait);
}

static int aicwf_usb_bus_txmsg(struct device *dev, u8 *buf, u32 len)
{
	int ret = 0;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_usb_dev *usb_dev = bus_if->bus_priv.usb;

	if (usb_dev->state != USB_UP_ST)
		return -EIO;

	if (buf == NULL || len == 0 || usb_dev->msg_out_urb == NULL)
		return -EINVAL;

	usb_dev->msg_finished = false;

#ifdef CONFIG_USB_MSG_OUT_EP
	if (usb_dev->msg_out_pipe) {
		usb_fill_bulk_urb(usb_dev->msg_out_urb,
			usb_dev->udev,
			usb_dev->msg_out_pipe,
			buf, len, (usb_complete_t) aicwf_usb_send_msg_complete, usb_dev);
	} else {
		usb_fill_bulk_urb(usb_dev->msg_out_urb,
			usb_dev->udev,
			usb_dev->bulk_out_pipe,
			buf, len, (usb_complete_t) aicwf_usb_send_msg_complete, usb_dev);
	}
#else
	usb_fill_bulk_urb(usb_dev->msg_out_urb,
		usb_dev->udev,
		usb_dev->bulk_out_pipe,
		buf, len, (usb_complete_t) aicwf_usb_send_msg_complete, usb_dev);
	usb_dev->msg_out_urb->transfer_flags |= URB_ZERO_PACKET;
#endif

	ret = usb_submit_urb(usb_dev->msg_out_urb, GFP_ATOMIC);
	if (ret) {
		usb_err("usb_submit_urb failed %d\n", ret);
		goto exit;
	}

	ret = wait_event_timeout(usb_dev->msg_wait,
		usb_dev->msg_finished, msecs_to_jiffies(CMD_TX_TIMEOUT));
	if (!ret) {
		if (usb_dev->msg_out_urb)
			usb_kill_urb(usb_dev->msg_out_urb);
		usb_err("Txmsg wait timed out\n");
		ret = -EIO;
		goto exit;
	}

	if (usb_dev->msg_finished == false) {
		usb_err("Txmsg timed out\n");
		ret = -ETIMEDOUT;
		goto exit;
	}
exit:
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
			usb_err("bad usb_buf\n");
			spin_lock_irqsave(qlock, flags);
			break;
		}
		usb_free_urb(usb_buf->urb);
		list_del_init(&usb_buf->list);
		spin_lock_irqsave(qlock, flags);
	}
	spin_unlock_irqrestore(qlock, flags);
}

static int aicwf_usb_alloc_rx_urb(struct aic_usb_dev *usb_dev)
{
	int i;

	usb_info("%s AICWF_USB_RX_URBS:%d \r\n", __func__, AICWF_USB_RX_URBS);
    for (i = 0; i < AICWF_USB_RX_URBS; i++) {
        struct aicwf_usb_buf *usb_buf = &usb_dev->usb_rx_buf[i];

		usb_buf->usbdev = usb_dev;
		usb_buf->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!usb_buf->urb) {
			usb_err("could not allocate rx data urb\n");
			goto err;
		}
		list_add_tail(&usb_buf->list, &usb_dev->rx_free_list);
	}
	return 0;

err:
	aicwf_usb_free_urb(&usb_dev->rx_free_list, &usb_dev->rx_free_lock);
	return -ENOMEM;
}

static int aicwf_usb_alloc_tx_urb(struct aic_usb_dev *usb_dev)
{
	int i;

	usb_info("%s AICWF_USB_TX_URBS:%d \r\n", __func__, AICWF_USB_TX_URBS);
	for (i = 0; i < AICWF_USB_TX_URBS; i++) {
		struct aicwf_usb_buf *usb_buf = &usb_dev->usb_tx_buf[i];

		usb_buf->usbdev = usb_dev;
		usb_buf->urb = usb_alloc_urb(0, GFP_KERNEL);
		if (!usb_buf->urb) {
			usb_err("could not allocate tx data urb\n");
			goto err;
		}
		list_add_tail(&usb_buf->list, &usb_dev->tx_free_list);
		(usb_dev->tx_free_count)++;
	}
	return 0;

err:
	aicwf_usb_free_urb(&usb_dev->tx_free_list, &usb_dev->tx_free_lock);
	return -ENOMEM;
}

#ifdef CONFIG_USB_MSG_IN_EP
static int aicwf_usb_alloc_msg_rx_urb(struct aic_usb_dev *usb_dev)
{
    int i;

    usb_info("%s AICWF_USB_MSG_RX_URBS:%d \r\n", __func__, AICWF_USB_MSG_RX_URBS);

    for (i = 0; i < AICWF_USB_MSG_RX_URBS; i++) {
        struct aicwf_usb_buf *usb_buf = &usb_dev->usb_msg_rx_buf[i];

        usb_buf->usbdev = usb_dev;
        usb_buf->urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!usb_buf->urb) {
            usb_err("could not allocate rx data urb\n");
            goto err;
        }
        list_add_tail(&usb_buf->list, &usb_dev->msg_rx_free_list);
    }
    return 0;

err:
    aicwf_usb_free_urb(&usb_dev->msg_rx_free_list, &usb_dev->msg_rx_free_lock);
    return -ENOMEM;
}
#endif

static void aicwf_usb_state_change(struct aic_usb_dev *usb_dev, int state)
{
	int old_state;

	if (usb_dev->state == state)
		return;

	old_state = usb_dev->state;
	usb_dev->state = state;

	if (state == USB_DOWN_ST) {
		usb_dev->bus_if->state = BUS_DOWN_ST;
	}
	if (state == USB_UP_ST) {
		usb_dev->bus_if->state = BUS_UP_ST;
	}
}

#ifdef CONFIG_USB_ALIGN_DATA
int align_param = 8;
module_param(align_param, int, 0660);
#endif
static int aicwf_usb_bus_txdata(struct device *dev, struct sk_buff *skb)
{
	u8 *buf;
	u16 buf_len = 0;
	u16 adjust_len = 0;
	struct aicwf_usb_buf *usb_buf;
	int ret = 0;
	unsigned long flags;
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_usb_dev *usb_dev = bus_if->bus_priv.usb;
	struct rwnx_txhdr *txhdr = (struct rwnx_txhdr *)skb->data;
	struct rwnx_hw *rwnx_hw = usb_dev->rwnx_hw;
	u8 usb_header[4];
	u8 adj_buf[4] = {0};
	u16 index = 0;
	bool need_cfm = false;
#ifdef CONFIG_USB_ALIGN_DATA
	u8 *buf_align = NULL;
	int align;
#endif

	if (usb_dev->state != USB_UP_ST) {
		usb_err("usb state is not up!\n");
		kmem_cache_free(rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);
		dev_kfree_skb_any(skb);
		return -EIO;
	}

	usb_buf = aicwf_usb_tx_dequeue(usb_dev, &usb_dev->tx_free_list,
						&usb_dev->tx_free_count, &usb_dev->tx_free_lock);
	if (!usb_buf) {
		usb_err("free:%d, post:%d\n", usb_dev->tx_free_count, usb_dev->tx_post_count);
		kmem_cache_free(rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);
		dev_kfree_skb_any(skb);
		ret = -ENOMEM;
		goto flow_ctrl;
	}

	if (txhdr->sw_hdr->need_cfm) {
		need_cfm = true;
		buf = kmalloc(skb->len + 1, GFP_ATOMIC/*GFP_KERNEL*/);
		index += sizeof(usb_header);
		memcpy(&buf[index], (u8 *)(long)&txhdr->sw_hdr->desc, sizeof(struct txdesc_api));
		index += sizeof(struct txdesc_api);
		memcpy(&buf[index], &skb->data[txhdr->sw_hdr->headroom], skb->len - txhdr->sw_hdr->headroom);
		index += skb->len - txhdr->sw_hdr->headroom;
		buf_len = index;
		if (buf_len & (TX_ALIGNMENT - 1)) {
			adjust_len = roundup(buf_len, TX_ALIGNMENT)-buf_len;
			memcpy(&buf[buf_len], adj_buf, adjust_len);
			buf_len += adjust_len;
		}
		usb_header[0] = ((buf_len) & 0xff);
		usb_header[1] = (((buf_len) >> 8)&0x0f);
		usb_header[2] = 0x01; //data
		usb_header[3] = 0; //reserved
		memcpy(&buf[0], usb_header, sizeof(usb_header));
		usb_buf->skb = (struct sk_buff *)buf;
	} else {
		skb_pull(skb, txhdr->sw_hdr->headroom);
		skb_push(skb, sizeof(struct txdesc_api));
		memcpy(&skb->data[0], (u8 *)(long)&txhdr->sw_hdr->desc, sizeof(struct txdesc_api));
		kmem_cache_free(rwnx_hw->sw_txhdr_cache, txhdr->sw_hdr);

		skb_push(skb, sizeof(usb_header));
		usb_header[0] = ((skb->len) & 0xff);
		usb_header[1] = (((skb->len) >> 8)&0x0f);
		usb_header[2] = 0x01; //data
		usb_header[3] = 0; //reserved
		memcpy(&skb->data[0], usb_header, sizeof(usb_header));

		buf = skb->data;
		buf_len = skb->len;

		usb_buf->skb = skb;
	}
	usb_buf->usbdev = usb_dev;
	if (need_cfm)
		usb_buf->cfm = true;
	else
		usb_buf->cfm = false;

#ifdef CONFIG_USB_ALIGN_DATA
#if 0
		usb_buf->usb_align_data = (u8*)kmalloc(sizeof(u8) * buf_len + align_param, GFP_ATOMIC);
	
		align = ((unsigned long)(usb_buf->usb_align_data)) & (align_param - 1);
		memcpy(usb_buf->usb_align_data + (align_param - align), buf, buf_len);
	
		usb_fill_bulk_urb(usb_buf->urb, usb_dev->udev, usb_dev->bulk_out_pipe,
					usb_buf->usb_align_data + (align_param - align), buf_len, aicwf_usb_tx_complete, usb_buf);
#else
		if (!IS_ALIGNED((unsigned long)buf, align_param)) {
			usb_buf->usb_align_data = (u8*)kmalloc(sizeof(u8) * buf_len + align_param, GFP_ATOMIC);
			if (usb_buf->usb_align_data) {
				align = ((unsigned long)(usb_buf->usb_align_data)) & (align_param - 1);
				buf_align = usb_buf->usb_align_data + (align_param - align);
				memcpy(buf_align, buf, buf_len);
			}
		} else {
			buf_align = buf;
		}
	
		usb_fill_bulk_urb(usb_buf->urb, usb_dev->udev, usb_dev->bulk_out_pipe,
					buf_align, buf_len, aicwf_usb_tx_complete, usb_buf);
#endif
#else
	usb_fill_bulk_urb(usb_buf->urb, usb_dev->udev, usb_dev->bulk_out_pipe,
				buf, buf_len, aicwf_usb_tx_complete, usb_buf);
#endif

	usb_buf->urb->transfer_flags |= URB_ZERO_PACKET;

	aicwf_usb_tx_queue(usb_dev, &usb_dev->tx_post_list, usb_buf,
					&usb_dev->tx_post_count, &usb_dev->tx_post_lock);
	complete(&bus_if->bustx_trgg);
	ret = 0;

flow_ctrl:
	spin_lock_irqsave(&usb_dev->tx_flow_lock, flags);
	if (usb_dev->tx_free_count < AICWF_USB_TX_LOW_WATER) {
		usb_dbg("usb_dev->tx_free_count < AICWF_USB_TX_LOW_WATER:%d\r\n",
			usb_dev->tx_free_count);
		usb_dev->tbusy = true;
		aicwf_usb_tx_flowctrl(usb_dev->rwnx_hw, true);
	}
	spin_unlock_irqrestore(&usb_dev->tx_flow_lock, flags);

	return ret;
}

static int aicwf_usb_bus_start(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_usb_dev *usb_dev = bus_if->bus_priv.usb;

	if (usb_dev->state == USB_UP_ST)
		return 0;

	aicwf_usb_state_change(usb_dev, USB_UP_ST);
	usb_dev->rx_prepare_ready = false;
	aicwf_usb_rx_prepare(usb_dev);
	aicwf_usb_tx_prepare(usb_dev);
#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		aicwf_usb_msg_rx_prepare(usb_dev);
	}
#endif
	if(!usb_dev->rx_prepare_ready){
		usb_err("%s rx prepare fail\r\n", __func__);
		return -1;
	}else{
		return 0;
	}
}

static void aicwf_usb_cancel_all_urbs_(struct aic_usb_dev *usb_dev)
{
	struct aicwf_usb_buf *usb_buf, *tmp;
	unsigned long flags;

	if (usb_dev->msg_out_urb)
		usb_kill_urb(usb_dev->msg_out_urb);

	spin_lock_irqsave(&usb_dev->tx_post_lock, flags);
	list_for_each_entry_safe(usb_buf, tmp, &usb_dev->tx_post_list, list) {
		spin_unlock_irqrestore(&usb_dev->tx_post_lock, flags);
		if (!usb_buf->urb) {
			usb_err("bad usb_buf\n");
			spin_lock_irqsave(&usb_dev->tx_post_lock, flags);
			break;
		}
		usb_kill_urb(usb_buf->urb);
		spin_lock_irqsave(&usb_dev->tx_post_lock, flags);
	}
	spin_unlock_irqrestore(&usb_dev->tx_post_lock, flags);

	usb_kill_anchored_urbs(&usb_dev->rx_submitted);
#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		usb_kill_anchored_urbs(&usb_dev->msg_rx_submitted);
	}
#endif
}

void aicwf_usb_cancel_all_urbs(struct aic_usb_dev *usb_dev){
	aicwf_usb_cancel_all_urbs_(usb_dev);
}

static void aicwf_usb_bus_stop(struct device *dev)
{
	struct aicwf_bus *bus_if = dev_get_drvdata(dev);
	struct aic_usb_dev *usb_dev = bus_if->bus_priv.usb;

	usb_dbg("%s\r\n", __func__);
	if (usb_dev == NULL)
		return;

	if (usb_dev->state == USB_DOWN_ST)
		return;

    if(g_rwnx_plat && g_rwnx_plat->wait_disconnect_cb == true){
        atomic_set(&aicwf_deinit_atomic, 1);
        up(&aicwf_deinit_sem);
    }
	aicwf_usb_state_change(usb_dev, USB_DOWN_ST);
    //aicwf_usb_cancel_all_urbs(usb_dev);//AIDEN
}

static void aicwf_usb_deinit(struct aic_usb_dev *usbdev)
{
	cancel_work_sync(&usbdev->rx_urb_work);
	aicwf_usb_free_urb(&usbdev->rx_free_list, &usbdev->rx_free_lock);
	aicwf_usb_free_urb(&usbdev->tx_free_list, &usbdev->tx_free_lock);
#ifdef CONFIG_USB_MSG_IN_EP
	if(usbdev->msg_in_pipe){
		cancel_work_sync(&usbdev->msg_rx_urb_work);
		aicwf_usb_free_urb(&usbdev->msg_rx_free_list, &usbdev->msg_rx_free_lock);
	}
#endif

	usb_free_urb(usbdev->msg_out_urb);
}

static void aicwf_usb_rx_urb_work(struct work_struct *work)
{
	struct aic_usb_dev *usb_dev = container_of(work, struct aic_usb_dev, rx_urb_work);

	aicwf_usb_rx_submit_all_urb(usb_dev);
}

#ifdef CONFIG_USB_MSG_IN_EP
static void aicwf_usb_msg_rx_urb_work(struct work_struct *work)
{
    struct aic_usb_dev *usb_dev = container_of(work, struct aic_usb_dev, msg_rx_urb_work);

    aicwf_usb_msg_rx_submit_all_urb(usb_dev);
}
#endif

static int aicwf_usb_init(struct aic_usb_dev *usb_dev)
{
	int ret = 0;

	usb_dev->tbusy = false;
	usb_dev->state = USB_DOWN_ST;

	init_waitqueue_head(&usb_dev->msg_wait);
	init_usb_anchor(&usb_dev->rx_submitted);

#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		init_usb_anchor(&usb_dev->msg_rx_submitted);
	}
#endif

	spin_lock_init(&usb_dev->tx_free_lock);
	spin_lock_init(&usb_dev->tx_post_lock);
	spin_lock_init(&usb_dev->rx_free_lock);
	spin_lock_init(&usb_dev->tx_flow_lock);
#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		spin_lock_init(&usb_dev->msg_rx_free_lock);
	}
#endif

	INIT_LIST_HEAD(&usb_dev->rx_free_list);
	INIT_LIST_HEAD(&usb_dev->tx_free_list);
	INIT_LIST_HEAD(&usb_dev->tx_post_list);
#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		INIT_LIST_HEAD(&usb_dev->msg_rx_free_list);
	}
#endif

	atomic_set(&rx_urb_cnt, 0);
	usb_dev->tx_free_count = 0;
	usb_dev->tx_post_count = 0;

	ret =  aicwf_usb_alloc_rx_urb(usb_dev);
	if (ret) {
		goto error;
	}
	ret =  aicwf_usb_alloc_tx_urb(usb_dev);
	if (ret) {
		goto error;
	}
#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		ret =  aicwf_usb_alloc_msg_rx_urb(usb_dev);
		if (ret) {
			goto error;
		}
	}
#endif

	usb_dev->msg_out_urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!usb_dev->msg_out_urb) {
		usb_err("usb_alloc_urb (msg out) failed\n");
		ret = ENOMEM;
		goto error;
	}

	INIT_WORK(&usb_dev->rx_urb_work, aicwf_usb_rx_urb_work);

#ifdef CONFIG_USB_MSG_IN_EP
	if(usb_dev->msg_in_pipe){
		INIT_WORK(&usb_dev->msg_rx_urb_work, aicwf_usb_msg_rx_urb_work);
	}
#endif

	return ret;
	error:
	usb_err("failed!\n");
	aicwf_usb_deinit(usb_dev);
	return ret;
}


static int aicwf_parse_usb(struct aic_usb_dev *usb_dev, struct usb_interface *interface)
{
	struct usb_interface_descriptor *interface_desc;
	struct usb_host_interface *host_interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *usb = usb_dev->udev;
	int i, endpoints;
	u8 endpoint_num;
	int ret = 0;

	usb_dev->bulk_in_pipe = 0;
	usb_dev->bulk_out_pipe = 0;

#ifdef CONFIG_USB_MSG_OUT_EP
	usb_dev->msg_out_pipe = 0;
#endif
#ifdef CONFIG_USB_MSG_IN_EP
	usb_dev->msg_in_pipe = 0;
#endif

	host_interface = &interface->altsetting[0];
	interface_desc = &host_interface->desc;
	endpoints = interface_desc->bNumEndpoints;
	usb_info("%s endpoints = %d\n", __func__, endpoints);

	/* Check device configuration */
	if (usb->descriptor.bNumConfigurations != 1) {
		usb_err("Number of configurations: %d not supported\n",
						usb->descriptor.bNumConfigurations);
		ret = -ENODEV;
		goto exit;
	}

	/* Check deviceclass */
#ifndef CONFIG_USB_BT
	if (usb->descriptor.bDeviceClass != 0x00) {
		usb_err("DeviceClass %d not supported\n",
		usb->descriptor.bDeviceClass);
		ret = -ENODEV;
		goto exit;
	}
#endif

	/* Check interface number */
#ifdef CONFIG_USB_BT
	if (usb->actconfig->desc.bNumInterfaces != 3) {
#else
	if (usb->actconfig->desc.bNumInterfaces != 1) {
#endif
	usb_err("Number of interfaces: %d not supported\n",
		usb->actconfig->desc.bNumInterfaces);
	ret = -ENODEV;
	goto exit;
	}

	if ((interface_desc->bInterfaceClass != USB_CLASS_VENDOR_SPEC) ||
		(interface_desc->bInterfaceSubClass != 0xff) ||
		(interface_desc->bInterfaceProtocol != 0xff)) {
		usb_err("non WLAN interface %d: 0x%x:0x%x:0x%x\n",
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
			if (!usb_dev->bulk_in_pipe) {
				usb_dev->bulk_in_pipe = usb_rcvbulkpipe(usb, endpoint_num);
			}
#ifdef CONFIG_USB_MSG_IN_EP
			else if (!usb_dev->msg_in_pipe) {
				if(aicwf_feature.chipinfo->chipid != PRODUCT_ID_AIC8801){
					usb_dev->msg_in_pipe = usb_rcvbulkpipe(usb, endpoint_num);
				}
			}
#endif
		}

		if (usb_endpoint_dir_out(endpoint) &&
			usb_endpoint_xfer_bulk(endpoint)) {
			if (!usb_dev->bulk_out_pipe) {
				usb_dev->bulk_out_pipe = usb_sndbulkpipe(usb, endpoint_num);
#ifdef CONFIG_USB_MSG_OUT_EP
			} else if (!usb_dev->msg_out_pipe) {
				usb_dev->msg_out_pipe = usb_sndbulkpipe(usb, endpoint_num);
#endif
			}
		}
	}

	if (usb_dev->bulk_in_pipe == 0) {
		usb_err("No RX (in) Bulk EP found\n");
		ret = -ENODEV;
		goto exit;
	}
	if (usb_dev->bulk_out_pipe == 0) {
		usb_err("No TX (out) Bulk EP found\n");
		ret = -ENODEV;
		goto exit;
	}

#ifdef CONFIG_USB_MSG_OUT_EP
	if (usb_dev->msg_out_pipe == 0) {
		usb_err("No TX Msg (out) Bulk EP found\n");
	}
#endif
#ifdef CONFIG_USB_MSG_IN_EP
	if(aicwf_feature.chipinfo->chipid != PRODUCT_ID_AIC8801){
		if (usb_dev->msg_in_pipe == 0) {
			usb_info("No RX Msg (in) Bulk EP found\n");
		}
	}
#endif

	switch (usb->speed) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
	case USB_SPEED_SUPER_PLUS:
		usb_info("Aic super plus speed USB device detected\n");
		break;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
	case USB_SPEED_SUPER:
		usb_info("Aic super speed USB device detected\n");
		break;
#endif
	case USB_SPEED_HIGH:
		usb_info("Aic high speed USB device detected\n");
		break;
	case USB_SPEED_FULL:
		usb_info("Aic full speed USB device detected\n");
		break;
	default:
		usb_info("Aic unknown speed(%d) USB device detected\n", usb->speed);
		break;
	}

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
    struct aicwf_bus *bus_if = NULL;
	struct device *dev = NULL;
	struct aicwf_rx_priv *rx_priv = NULL;
	struct aic_usb_dev *usb_dev = NULL;
	int i = 0;

	usb_dev = kzalloc(sizeof(struct aic_usb_dev), GFP_ATOMIC);
	if (!usb_dev) {
		usb_err("%s usb_dev kzalloc fail\r\n", __func__);
		return -ENOMEM;
	}

	usb_dbg("%s usb_dev:%d usb_tx_buf:%d usb_rx_buf:%d\r\n", 
		__func__, (int)sizeof(struct aic_usb_dev), (int)sizeof(struct aicwf_usb_buf) * AICWF_USB_TX_URBS, (int)sizeof(struct aicwf_usb_buf) * AICWF_USB_RX_URBS);

	usb_dev->usb_tx_buf = vmalloc(sizeof(struct aicwf_usb_buf) * AICWF_USB_TX_URBS);
    usb_dev->usb_rx_buf = vmalloc(sizeof(struct aicwf_usb_buf) * AICWF_USB_RX_URBS);

    if(!usb_dev->usb_tx_buf || !usb_dev->usb_rx_buf){
        if(usb_dev->usb_tx_buf){
            vfree(usb_dev);
        }
        
        if(usb_dev->usb_tx_buf){
            vfree(usb_dev);
        }
        
        if(usb_dev){
            kfree(usb_dev);
        }
        usb_err("%s usb_tx_buf or usb_rx_buf vmalloc fail\r\n", __func__);
        return -ENOMEM;
    }

    memset(usb_dev->usb_tx_buf, 0, (int)(sizeof(struct aicwf_usb_buf) * AICWF_USB_TX_URBS));
    memset(usb_dev->usb_rx_buf, 0, (int)(sizeof(struct aicwf_usb_buf) * AICWF_USB_RX_URBS));

	usb_dev->udev = usb;
	usb_dev->dev = &usb->dev;
	usb_set_intfdata(intf, usb_dev);

	aic_matched_ic = NULL;
	for (i = 0; i < sizeof(aicdev_match_table) / sizeof(aicdev_match_table[0]); i++) {
		if (id->idVendor == aicdev_match_table[i].vid && id->idProduct == aicdev_match_table[i].pid) {
			aic_matched_ic = &aicdev_match_table[i];
			break;
		}
	}

	usb_dbg("%s, matched chip: %s\n", __func__, aic_matched_ic ? aic_matched_ic->name : "none");
	if (aic_matched_ic == NULL) {
		usb_dbg("%s device is not support, exit...\n", __func__);
		return -1;
	}

#ifdef AICWF_BSP_CTRL
	aicbsp_get_feature(&aicwf_feature);
	aicwf_feature.chipinfo = aic_matched_ic;
#endif

	if(aicwf_feature.chipinfo->chipid == PRODUCT_ID_AIC8800D81){
		aicwf_usb_rx_aggr = true;
	}

	ret = aicwf_parse_usb(usb_dev, intf);
	if (ret) {
		usb_err("aicwf_parse_usb err %d\n", ret);
		goto out_free;
	}

	ret = aicwf_usb_init(usb_dev);
	if (ret) {
		usb_err("aicwf_usb_init err %d\n", ret);
		goto out_free;
	}

	bus_if = kzalloc(sizeof(struct aicwf_bus), GFP_ATOMIC);
	if (!bus_if) {
		ret = -ENOMEM;
		goto out_free_usb;
	}

	dev = usb_dev->dev;
	bus_if->dev = dev;
	usb_dev->bus_if = bus_if;
	bus_if->bus_priv.usb = usb_dev;
	dev_set_drvdata(dev, bus_if);

	bus_if->ops = &aicwf_usb_bus_ops;

	rx_priv = aicwf_rx_init(usb_dev);
	if (!rx_priv) {
		txrx_err("rx init failed\n");
		ret = -1;
		goto out_free_bus;
	}
	usb_dev->rx_priv = rx_priv;


	ret = aicwf_bus_init(0, dev);
	if (ret < 0) {
		usb_err("aicwf_bus_init err %d\n", ret);
		goto out_free_bus;
	}

	ret = aicwf_bus_start(bus_if);
	if (ret < 0) {
		usb_err("aicwf_bus_start err %d\n", ret);
		goto out_free_bus;
	}

	ret = aicwf_rwnx_usb_platform_init(usb_dev);
	if (ret < 0) {
		usb_err("aicwf_rwnx_usb_platform_init err %d\n", ret);
		goto out_free_bus;
	}
	aicwf_hostif_ready();
	return 0;

out_free_bus:
	aicwf_bus_deinit(dev);
	kfree(bus_if);
out_free_usb:
	aicwf_usb_deinit(usb_dev);
out_free:
	usb_err("failed with errno %d\n", ret);
	vfree(usb_dev->usb_tx_buf);
	vfree(usb_dev->usb_rx_buf);
	kfree(usb_dev);
	usb_set_intfdata(intf, NULL);
	return ret;
}

static void aicwf_usb_disconnect(struct usb_interface *intf)
{
	struct aic_usb_dev *usb_dev =
			(struct aic_usb_dev *) usb_get_intfdata(intf);
    usb_info("%s Enter\r\n", __func__);

	if(g_rwnx_plat->wait_disconnect_cb == false){
		atomic_set(&aicwf_deinit_atomic, 0);
		down(&aicwf_deinit_sem);
	}
    if (!usb_dev){
		usb_err("%s usb_dev is null \r\n", __func__);
        return;
    }

	aicwf_bus_deinit(usb_dev->dev);
	aicwf_usb_deinit(usb_dev);

	kfree(usb_dev->bus_if);
    vfree(usb_dev->usb_tx_buf);
    vfree(usb_dev->usb_rx_buf);
	kfree(usb_dev);
	usb_info("%s exit\r\n", __func__);
	up(&aicwf_deinit_sem);
	atomic_set(&aicwf_deinit_atomic, 1);
}

static int aicwf_usb_suspend(struct usb_interface *intf, pm_message_t state)
{
	struct aic_usb_dev *usb_dev =
		(struct aic_usb_dev *) usb_get_intfdata(intf);

	printk("%s enter\r\n", __func__);
	aicwf_usb_state_change(usb_dev, USB_SLEEP_ST);
	aicwf_bus_stop(usb_dev->bus_if);
	return 0;
}

static int aicwf_usb_resume(struct usb_interface *intf)
{
	struct aic_usb_dev *usb_dev =
		(struct aic_usb_dev *) usb_get_intfdata(intf);
	printk("%s enter\r\n", __func__);

	if (usb_dev->state == USB_UP_ST)
		return 0;

	aicwf_bus_start(usb_dev->bus_if);
	return 0;
}

static int aicwf_usb_reset_resume(struct usb_interface *intf)
{
	return aicwf_usb_resume(intf);
}

static struct usb_device_id aicwf_usb_id_table[] = {
#ifndef CONFIG_USB_BT
	{USB_DEVICE(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8800)},
#else
	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8801, 0xff, 0xff, 0xff)},
	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8800D81, 0xff, 0xff, 0xff)},
	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8800D41, 0xff, 0xff, 0xff)},
//	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_PRODUCT_ID_AIC8800DC, 0xff, 0xff, 0xff)},
//	{USB_DEVICE(USB_VENDOR_ID_AIC, USB_PRODUCT_ID_AIC8800DW)},
//	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC_V2, USB_PRODUCT_ID_AIC8800D81X2, 0xff, 0xff, 0xff)},
//	{USB_DEVICE(USB_VENDOR_ID_AIC_V2, USB_PRODUCT_ID_AIC8800D89X2)},
#endif
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	.disable_hub_initiated_lpm = 1,
#endif
};

void aicwf_usb_register(void)
{
	if (usb_register(&aicwf_usbdrvr) < 0) {
		usb_err("usb_register failed\n");
	}
}

void aicwf_usb_exit(void)
{
    int retry = 5;
    usb_info("%s Enter\r\n", __func__);

    usb_dbg("%s in_interrupt:%d in_softirq:%d in_atomic:%d\r\n", __func__, (int)in_interrupt(), (int)in_softirq(), (int)in_atomic());

    do{
        usb_info("aicwf_deinit_atomic is busy. waiting for 500ms retry:%d \r\n",
            retry);
        mdelay(500);
        retry--;
        if(retry == 0){
            break;
        }
    }while(atomic_read(&aicwf_deinit_atomic) == 0);

	atomic_set(&aicwf_deinit_atomic, 0);
	if(down_timeout(&aicwf_deinit_sem, msecs_to_jiffies(SEM_TIMOUT)) != 0){
		usb_err("%s semaphore waiting timeout\r\n", __func__);
	}

	if(g_rwnx_plat){
		g_rwnx_plat->wait_disconnect_cb = false;
	}
	

	if (!g_rwnx_plat || !g_rwnx_plat->enabled) {
		usb_info("g_rwnx_plat is not ready. waiting for 500ms\r\n");
		mdelay(500);
	}
	if (g_rwnx_plat && g_rwnx_plat->enabled)
		rwnx_platform_deinit(g_rwnx_plat->usbdev->rwnx_hw);
	up(&aicwf_deinit_sem);
	atomic_set(&aicwf_deinit_atomic, 1);

	usb_info("%s usb_deregister \r\n", __func__);
	usb_deregister(&aicwf_usbdrvr);
	if (g_rwnx_plat) {
		kfree(g_rwnx_plat);
	}
	usb_info("%s exit\r\n", __func__);
}
