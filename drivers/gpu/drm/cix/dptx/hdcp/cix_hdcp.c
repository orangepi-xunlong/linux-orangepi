// SPDX-License-Identifier: GPL-2.0
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/nospec.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include "cix_hdcp_ioctl.h"
#include "cix_hdcp_ioctl_cmd.h"

static LIST_HEAD(cix_hdcp_list);

static unsigned int cix_hdcp_ioctl_cmds[] = {
	HDCP2_IOCTL_RXSTATE,
	HDCP2_IOCTL_TIMER_START,
	HDCP2_IOCTL_TIMER_STOP,
	HDCP2_IOCTL_DPCD_ACCESS
};

int cix_hdcp_hpd_event_process(struct cix_hdcp *hdcp,
		bool plugged)
{
	struct hdcp_event *e;

	e = kmalloc(sizeof(struct hdcp_event), GFP_KERNEL);

	if (plugged == true) {
		e->event = EV2_TX_RX_CONNECT;
		hdcp->state = ST2_H1;
		dev_info(hdcp->aux->dev, "report event EV2_TX_RX_CONNECT\n");
	} else {
		e->event = EV2_TX_RX_DISCONNECT;
		hdcp->state = ST2_H0;
		dev_info(hdcp->aux->dev, "report event EV2_TX_RX_DISCONNECT\n");
	}

	if (!hdcp->opened)
		return 0;

	spin_lock_irq(&hdcp->event_lock);
	list_add_tail(&e->list, &hdcp->event_list);
	spin_unlock_irq(&hdcp->event_lock);

	wake_up_interruptible_poll(&hdcp->event_wait,
		EPOLLIN | EPOLLRDNORM);

	return 0;
}

int cix_hdcp_timer_process(struct cix_hdcp *hdcp)
{
	struct hdcp_event *e;

	hdcp->timer_in_use = false;

	e = kmalloc(sizeof(struct hdcp_event), GFP_KERNEL);
	e->event = EV2_TX_TIMER;

	dev_info(hdcp->aux->dev, "report event timer\n");

	spin_lock_irq(&hdcp->event_lock);
	list_add_tail(&e->list, &hdcp->event_list);
	spin_unlock_irq(&hdcp->event_lock);

	wake_up_interruptible_poll(&hdcp->event_wait,
		EPOLLIN | EPOLLRDNORM);

	return 0;
}

int cix_hdcp_cp_irq_process(struct cix_hdcp *hdcp, u8 rx_status)
{
	struct hdcp_event *e;

	if (rx_status & 0x1f) {
		e = kmalloc(sizeof(struct hdcp_event), GFP_KERNEL);
		if (rx_status & 0x1) {
			e->event = EV2_TX_READY;
			dev_info(hdcp->aux->dev, "report event EV2_TX_READY\n");
		}

		if ((rx_status >> 0x1) & 0x1) {
			e->event = EV2_TX_HPRIME_AVAILABLE;
			dev_info(hdcp->aux->dev, "report event EV2_TX_HPRIME_AVAILABLE\n");
		}

		if ((rx_status >> 0x2) & 0x1) {
			e->event = EV2_TX_PAIRING_AVAILABLE;
			dev_info(hdcp->aux->dev, "report event EV2_TX_PAIRING_AVAILABLE\n");
		}

		if ((rx_status >> 0x3) & 0x1) {
			e->event = EV2_TX_REAUTH_REQ;
			dev_info(hdcp->aux->dev, "report event EV2_TX_REAUTH_REQ\n");
		}

		if ((rx_status >> 0x4) & 0x1) {
			e->event = EV2_TX_INTEGRITY_FAILURE;
			dev_info(hdcp->aux->dev, "report event EV2_TX_INTEGRITY_FAILURE\n");
		}

		spin_lock_irq(&hdcp->event_lock);
		list_add_tail(&e->list, &hdcp->event_list);
		spin_unlock_irq(&hdcp->event_lock);

		wake_up_interruptible_poll(&hdcp->event_wait,
			EPOLLIN | EPOLLRDNORM);
	}

	return 0;
}

static int cix_hdcp_open(struct inode *inode, struct file *filp)
{
	int ret;
	struct cix_hdcp *pos, *hdcp = NULL;

	list_for_each_entry(pos, &cix_hdcp_list, list) {
		if (pos->misc.minor == iminor(inode)) {
			hdcp = pos;
			break;
		}
	}

	if (hdcp) {
		hdcp->opened = true;
		filp->private_data = hdcp;
		dev_info(hdcp->aux->dev, "succeed to open hdcp file\n");
		ret = 0;
	} else {
		dev_info(hdcp->aux->dev, "failed to open hdcp file\n");
		ret = -EINVAL;
	}

	return ret;
}

static int cix_hdcp_close(struct inode *inode, struct file *filp)
{
	struct cix_hdcp *hdcp = filp->private_data;

	for (;;) {
		struct hdcp_event *e = NULL;

		spin_lock_irq(&hdcp->event_lock);
		if (!list_empty(&hdcp->event_list)) {
			e = list_first_entry(&hdcp->event_list,
						struct hdcp_event, list);
			list_del(&e->list);
			kfree(e);
		} else {
			dev_info(hdcp->aux->dev, "event list is null and close\n");
			spin_unlock_irq(&hdcp->event_lock);
			break;
		}
		spin_unlock_irq(&hdcp->event_lock);
	}

	hdcp->opened = false;

	return 0;
}

static ssize_t cix_hdcp_read(struct file *filp, char __user *buffer,
         size_t count, loff_t *offset)
{
	struct cix_hdcp *hdcp = filp->private_data;
	ssize_t ret;

	ret = mutex_lock_interruptible(&hdcp->mutex);
	if (ret)
		return ret;

	for (;;) {
		struct hdcp_event *e = NULL;

		spin_lock_irq(&hdcp->event_lock);
		if (!list_empty(&hdcp->event_list)) {
			e = list_first_entry(&hdcp->event_list,
						struct hdcp_event, list);
			list_del(&e->list);
		}
		spin_unlock_irq(&hdcp->event_lock);

		if (e == NULL) {
			if (ret)
				break;

			if (filp->f_flags & O_NONBLOCK) {
					ret = -EAGAIN;
					break;
			}

			mutex_unlock(&hdcp->mutex);
			ret = wait_event_interruptible(hdcp->event_wait,
							!list_empty(&hdcp->event_list));
			if (ret >= 0)
				ret = mutex_lock_interruptible(&hdcp->mutex);
			if (ret)
				return ret;
		} else {
			unsigned length = sizeof(unsigned int);

			if (length > count - ret) {
put_back_event:
				spin_lock_irq(&hdcp->event_lock);
				list_add(&e->list, &hdcp->event_list);
				spin_unlock_irq(&hdcp->event_lock);
				wake_up_interruptible_poll(&hdcp->event_wait,
								EPOLLIN | EPOLLRDNORM);
				break;
			}

			if (copy_to_user(buffer + ret, &e->event, length)) {
				if (ret == 0)
					ret = -EFAULT;
				goto put_back_event;
			}

			ret += length;
			kfree(e);
		}
	}
	mutex_unlock(&hdcp->mutex);

	return ret;
}

static __poll_t cix_hdcp_poll(struct file *filp, poll_table *wait)
{
	struct cix_hdcp *hdcp = filp->private_data;
	__poll_t mask = 0;

	poll_wait(filp, &hdcp->event_wait, wait);
	mutex_lock(&hdcp->mutex);

	if (!list_empty(&hdcp->event_list))
		mask |= EPOLLIN | EPOLLRDNORM;

	mutex_unlock(&hdcp->mutex);

	return mask;
}

static long cix_hdcp_ioctl(struct file *file, unsigned int ucmd,
               unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	unsigned int kcmd;
	unsigned int in_size, out_size, drv_size, ksize;
	int nr = _IOC_NR(ucmd);
	int ret = 0;
	struct cix_hdcp *hdcp = file->private_data;

	if (nr >= ARRAY_SIZE(cix_hdcp_ioctl_cmds))
		return -EINVAL;

	nr = array_index_nospec(nr, ARRAY_SIZE(cix_hdcp_ioctl_cmds));
	/* Get the kernel ioctl cmd that matches */
	kcmd = cix_hdcp_ioctl_cmds[nr];

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(kcmd);
	out_size = _IOC_SIZE(ucmd);
	in_size = out_size;
	if ((ucmd & kcmd & IOC_IN) == 0)
		in_size = 0;
	if ((ucmd & kcmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}

	if (copy_from_user(kdata, (void __user *)arg, in_size) != 0) {
		ret = -EFAULT;
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (kcmd) {
	case HDCP2_IOCTL_RXSTATE:
		ret = cix_hdcp2_ioctl_get_rx_state(hdcp, kdata);
		break;
	case HDCP2_IOCTL_TIMER_START:
		ret = cix_hdcp2_ioctl_timer_start(hdcp, kdata);
		break;
	case HDCP2_IOCTL_TIMER_STOP:
		ret = cix_hdcp2_ioctl_timer_stop(hdcp);
		break;
	case HDCP2_IOCTL_DPCD_ACCESS:
		ret = cix_hdcp2_ioctl_dpcd_access(hdcp, kdata);
		break;
	default:
		ret = -ENOTTY;
	}

	if (ret)
		goto err;

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;
err:
	if (kdata != stack_kdata)
		kfree(kdata);
	return ret;
}

static const struct file_operations hdcp_fops = {
	.owner = THIS_MODULE,
	.open = cix_hdcp_open,
	.release = cix_hdcp_close,
	.read = cix_hdcp_read,
	.poll = cix_hdcp_poll,
	.unlocked_ioctl = cix_hdcp_ioctl,
};

int cix_hdcp_init(struct cix_hdcp *hdcp)
{
	int ret;
	struct drm_dp_aux *aux = hdcp->aux;
	struct device *dev = aux->dev;

	snprintf(hdcp->name, 14, "hdcp-%s\n", dev_name(dev));
	mutex_init(&hdcp->mutex);
	spin_lock_init(&hdcp->event_lock);
	INIT_LIST_HEAD(&hdcp->event_list);
	init_waitqueue_head(&hdcp->event_wait);
	hdcp->misc.minor  = MISC_DYNAMIC_MINOR;
	hdcp->misc.name  = hdcp->name;
	hdcp->misc.fops  = &hdcp_fops;

	ret = misc_register(&hdcp->misc);
	if (!ret) {
		list_add(&hdcp->list, &cix_hdcp_list);
		dev_info(dev, "succeed register hdcp misc device.\n");
	} else {
		dev_err(dev, "cannot register hdcp misc device, ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

int cix_hdcp_uninit(struct cix_hdcp *hdcp)
{
	misc_deregister(&hdcp->misc);
	list_del(&hdcp->list);
	hdcp->aux = NULL;

	return 0;
}
