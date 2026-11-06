/*
 * bm_serial.c - utilities for USB gadget "serial port"/TTY support
 *
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This code also borrows from usbserial.c, which is
 * Copyright (C) 1999 - 2002 Greg Kroah-Hartman (greg@kroah.com)
 * Copyright (C) 2000 Peter Berger (pberger@brimson.com)
 * Copyright (C) 2000 Al Borchers (alborchers@steinerpoint.com)
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/kthread.h>
#include "bm_serial.h"
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/eventfd.h>
#include <linux/compat.h>

/*
 * This component encapsulates the TTY layer glue needed to provide basic
 * "serial port" functionality through the USB gadget stack.  Each such
 * port is exposed through a /dev/ttyGS* node.
 *
 * After this module has been loaded, the individual TTY port can be requested
 * (bmserial_alloc_line()) and it will stay available until they are removed
 * (bmserial_free_line()). Each one may be connected to a USB function
 * (bmserial_connect), or disconnected (with bmserial_disconnect) when the USB
 * host issues a config change event. Data can only flow when the port is
 * connected to the host.
 *
 * A given TTY port can be made available in multiple configurations.
 * For example, each one might expose a ttyGS0 node which provides a
 * login application.  In one case that might use CDC ACM interface 0,
 * while another configuration might use interface 3 for that.  The
 * work to handle that (including descriptor management) is not part
 * of this component.
 *
 * Configurations may expose more than one TTY port.  For example, if
 * ttyGS0 provides login service, then ttyGS1 might provide dialer access
 * for a telephone or fax link.  And ttyGS2 might be something that just
 * needs a simple byte stream interface for some messaging protocol that
 * is managed in userspace ... OBEX, PTP, and MTP have been mentioned.
 *
 *
 * bmserial is the lifecycle interface, used by USB functions
 * gs_port is the I/O nexus, used by the tty driver
 * tty_struct links to the tty/filesystem framework
 *
 * bmserial <---> gs_port ... links will be null when the USB link is
 * inactive; managed by bmserial_{connect,disconnect}().  each bmserial
 * instance can wrap its own USB control protocol.
 *	bmserial->ioport == usb_ep->driver_data ... gs_port
 *	gs_port->port_usb ... bmserial
 *
 * gs_port <---> tty_struct ... links will be null when the TTY file
 * isn't opened; managed by gs_open()/gs_close()
 *	bmserial->port_tty ... tty_struct
 *	tty_struct->driver_data ... bmserial
 */

/* RX and TX queues can buffer QUEUE_SIZE packets before they hit the
 * next layer of buffering.  For TX that's a circular buffer; for RX
 * consider it a NOP.  A third layer is provided by the TTY code.
 */

#define PREFIX	"BMUSB"

/*
 * The port structure holds info for each port, one for each minor number
 * (and thus for each /dev/ node).
 */
struct gs_port {
	struct tty_port port;
	spinlock_t port_lock;	/* guard port_* access */

	struct bmserial *port_usb;

	bool openclose;		/* open/close in progress */
	u8 port_num;

	struct list_head read_pool;
	int read_started;
	int read_allocated;
	struct list_head read_queue;
	unsigned int n_read;
	struct tasklet_struct push;

	struct list_head write_pool;
	int write_started;
	int write_allocated;
	//struct gs_buf         port_write_buf;
	wait_queue_head_t drain_wait;	/* wait while writes drain */
	bool write_busy;
	wait_queue_head_t close_wait;

	/* REVISIT this state ... */
	struct usb_cdc_line_coding port_line_coding;	/* 8-N-1 etc */
};

static struct portmaster {
	struct mutex lock;	/* protect open/close */
	struct gs_port *port;
} ports[MAX_U_SERIAL_PORTS];

/*bitmain device node*/
static struct class *bitmain_class;
static struct device *bitmain_device_rx, *bitmain_device_tx;

#define MAX_SIZE (1024UL * 1024UL)	/* max size mmaped to userspace */
#define RX_DEVICE_NAME "bmusb_rx"
#define TX_DEVICE_NAME "bmusb_tx"
#define RX_CLASS_NAME "bitmain_usb_rx"
#define TX_CLASS_NAME "bitmain_usb_tx"

static int rx_major, tx_major;
static char *buffer_rx;
static int create_complete;

static DEFINE_MUTEX(bmusb_rx_mutex);
static DEFINE_MUTEX(bmusb_tx_mutex);

#define MAX_TRANSFER_LENGTH 65536
#define timeout_value 1

struct bm_serial_request {
	struct usb_request *request;
	int actual_length;
	int request_length;
	int complete;
};

struct usb_dev_state {
	struct list_head list;
	spinlock_t lock;
	struct list_head async_pending;
	struct list_head async_completed;
	struct list_head memory_list;
	wait_queue_head_t wait;
	int timeout_done;
	struct timer_list timeout;
	unsigned char val;
	rwlock_t ioctl_lock;
};

struct async {
	struct list_head asynclist;
	struct usb_dev_state *ps;
	int status;
	u8 bulk_addr;
	u8 bulk_status;
};

struct bm_serial_request *bm_request_rx;
struct bm_serial_request *bm_request_tx;
static struct timer_list timeout_rx;
static struct timer_list timeout_tx;
spinlock_t lock_rx;
spinlock_t lock_tx;

static int async_enable_tx;
static int async_enable;

static char flag = 'n';
static char flag_tx = 'n';
static DECLARE_WAIT_QUEUE_HEAD(wq);
static DECLARE_WAIT_QUEUE_HEAD(wq_tx);

#ifdef VERBOSE_DEBUG
#ifndef pr_vdebug
#define pr_vdebug(fmt, arg...) \
	pr_debug(fmt, ##arg)
#endif /* pr_vdebug */
#else
#ifndef pr_vdebug
#define pr_vdebug(fmt, arg...) \
	({ if (0) pr_debug(fmt, ##arg); })
#endif /* pr_vdebug */
#endif

/*-------------------------------------------------------------------------*/

/* Circular Buffer */
#if 0
/*
 * gs_buf_alloc
 *
 * Allocate a circular buffer and all associated memory.
 */
static int gs_buf_alloc(struct gs_buf *gb, unsigned int size)
{
	gb->buf_buf = kmalloc(size, GFP_KERNEL);
	if (gb->buf_buf == NULL)
		return -ENOMEM;

	gb->buf_size = size;
	gb->buf_put = gb->buf_buf;
	gb->buf_get = gb->buf_buf;

	return 0;
}

/*
 * gs_buf_free
 *
 * Free the buffer and all associated memory.
 */
static void gs_buf_free(struct gs_buf *gb)
{
	kfree(gb->buf_buf);
	gb->buf_buf = NULL;
}

/*
 * gs_buf_clear
 *
 * Clear out all data in the circular buffer.
 */
static void gs_buf_clear(struct gs_buf *gb)
{
	gb->buf_get = gb->buf_put;
	/* equivalent to a get of all data available */
}

/*
 * gs_buf_data_avail
 *
 * Return the number of bytes of data written into the circular
 * buffer.
 */
static unsigned int gs_buf_data_avail(struct gs_buf *gb)
{
	return (gb->buf_size + gb->buf_put - gb->buf_get) % gb->buf_size;
}

/*
 * gs_buf_space_avail
 *
 * Return the number of bytes of space available in the circular
 * buffer.
 */
static unsigned int gs_buf_space_avail(struct gs_buf *gb)
{
	return (gb->buf_size + gb->buf_get - gb->buf_put - 1) % gb->buf_size;
}

/*
 * gs_buf_put
 *
 * Copy data data from a user buffer and put it into the circular buffer.
 * Restrict to the amount of space available.
 *
 * Return the number of bytes copied.
 */
static unsigned int
gs_buf_put(struct gs_buf *gb, const char *buf, unsigned int count)
{
	unsigned int len;

	len = gs_buf_space_avail(gb);
	if (count > len)
		count = len;

	if (count == 0)
		return 0;

	len = gb->buf_buf + gb->buf_size - gb->buf_put;
	if (count > len) {
		memcpy(gb->buf_put, buf, len);
		memcpy(gb->buf_buf, buf + len, count - len);
		gb->buf_put = gb->buf_buf + count - len;
	} else {
		memcpy(gb->buf_put, buf, count);
		if (count < len)
			gb->buf_put += count;
		else		/* count == len */
			gb->buf_put = gb->buf_buf;
	}

	return count;
}

/*
 * gs_buf_get
 *
 * Get data from the circular buffer and copy to the given buffer.
 * Restrict to the amount of data available.
 *
 * Return the number of bytes copied.
 */
static unsigned int gs_buf_get(struct gs_buf *gb, char *buf, unsigned int count)
{
	unsigned int len;

	len = gs_buf_data_avail(gb);
	if (count > len)
		count = len;

	if (count == 0)
		return 0;

	len = gb->buf_buf + gb->buf_size - gb->buf_get;
	if (count > len) {
		memcpy(buf, gb->buf_get, len);
		memcpy(buf + len, gb->buf_buf, count - len);
		gb->buf_get = gb->buf_buf + count - len;
	} else {
		memcpy(buf, gb->buf_get, count);
		if (count < len)
			gb->buf_get += count;
		else		/* count == len */
			gb->buf_get = gb->buf_buf;
	}

	return count;
}
#endif
/*-------------------------------------------------------------------------*/

/* I/O glue between TTY (upper) and USB function (lower) driver layers */

/*
 * bm_alloc_req
 *
 * Allocate a usb_request and its buffer.  Returns a pointer to the
 * usb_request or NULL if there is an error.
 */
struct usb_request *bm_alloc_req(struct usb_ep *ep, unsigned int len,
				 gfp_t kmalloc_flags)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, kmalloc_flags);

	if (req != NULL) {
		req->length = len;
		req->buf = kmalloc(len, kmalloc_flags);
		if (req->buf == NULL) {
			usb_ep_free_request(ep, req);
			return NULL;
		}
	}

	return req;
}
EXPORT_SYMBOL_GPL(bm_alloc_req);

/*
 * gs_free_req
 *
 * Free a usb_request and its buffer.
 */
void bm_free_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}
EXPORT_SYMBOL_GPL(bm_free_req);

static void bm_read_complete(struct usb_ep *ep, struct usb_request *req)
{
	memcpy(&buffer_rx[bm_request_rx->actual_length], req->buf, req->actual);
	bm_request_rx->actual_length = bm_request_rx->actual_length +
	    req->actual;
	if ((bm_request_rx->actual_length < bm_request_rx->request_length) &&
	    (req->actual != 0) && (req->actual % ep->maxpacket == 0)) {
		req->length = bm_request_rx->request_length -
		    bm_request_rx->actual_length;
		if (req->length > MAX_TRANSFER_LENGTH)
			req->length = MAX_TRANSFER_LENGTH;
		if (req->status == 0)
			usb_ep_queue(ep, req, GFP_ATOMIC);
	} else {
		flag = 'y';
		wake_up_interruptible(&wq);
	}
}

static void bm_write_complete(struct usb_ep *ep, struct usb_request *req)
{
	req->req_map = req->req_map + 1;
	if (req->actual > 0) {
		if (req->actual > MAX_TRANSFER_LENGTH)
			req->length = MAX_TRANSFER_LENGTH;
		else
			req->length = req->actual;
		if (req->status == 0)
			usb_ep_queue(ep, req, GFP_ATOMIC);
	} else {
		req->req_map = 0;
		flag_tx = 'y';
		wake_up_interruptible(&wq_tx);
	}
}

static void bm_free_requests(struct usb_ep *ep, struct list_head *head,
			     int *allocated)
{
	struct usb_request *req;

	while (!list_empty(head)) {
		req = list_entry(head->next, struct usb_request, list);
		list_del(&req->list);
		bm_free_req(ep, req);
		if (allocated)
			(*allocated)--;
	}
}

static int bm_alloc_requests(struct usb_ep *ep, void (*fn) (struct usb_ep *,
							    struct usb_request
							    *))
{
	struct usb_request *req;

	/* Pre-allocate up to QUEUE_SIZE transfers, but if we can't
	 * do quite that many this time, don't fail ... we just won't
	 * be as speedy as we might otherwise be.
	 */
	req = bm_alloc_req(ep, MAX_SIZE, GFP_ATOMIC);
	req->complete = fn;
	return 0;
}

/**
 * gs_start_io - start USB I/O streams
 * @dev: encapsulates endpoints to use
 * Context: holding port_lock; port_tty and port_usb are non-null
 *
 * We only start I/O when something is connected to both sides of
 * this port.  If nothing is listening on the host side, we may
 * be pointlessly filling up our TX buffers and FIFO.
 */
static int bm_start_io(struct gs_port *port)
{
	struct list_head *head = &port->read_pool;
	struct usb_ep *ep = port->port_usb->out;
	int status;

	/* Allocate RX and TX I/O buffers.  We can't easily do this much
	 * earlier (with GFP_KERNEL) because the requests are coupled to
	 * endpoints, as are the packet sizes we'll be using.  Different
	 * configurations may use different endpoints with a given port;
	 * and high speed vs full speed changes packet sizes too.
	 */
	status = bm_alloc_requests(ep, bm_read_complete);
	if (status)
		return status;

	status = bm_alloc_requests(port->port_usb->in, bm_write_complete);
	if (status) {
		bm_free_requests(ep, head, &port->read_allocated);
		return status;
	}

	return status;
}

/*-------------------------------------------------------------------------*/

#if 0
static struct usb_request *gs_request_new(struct usb_ep *ep)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_ATOMIC);

	if (!req)
		return NULL;

	req->buf = kmalloc(ep->maxpacket, GFP_ATOMIC);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}
#endif

static void bmusb_timeout(unsigned long arg)
{
	struct usb_dev_state *ps = (struct usb_dev_state *)arg;
	unsigned long flags;

	spin_lock_irqsave(&ps->lock, flags);

	ps->timeout_done = 1;
	if (flag == 'n')
		wake_up(&ps->wait);
	spin_unlock_irqrestore(&ps->lock, flags);
}

static void bmusb_timeout_tx(unsigned long arg)
{
	struct usb_dev_state *ps_tx = (struct usb_dev_state *)arg;
	unsigned long flags;

	spin_lock_irqsave(&ps_tx->lock, flags);

	ps_tx->timeout_done = 1;
	wake_up(&ps_tx->wait);
	spin_unlock_irqrestore(&ps_tx->lock, flags);
}

static void bmusb_timeout_read(unsigned long arg)
{
	unsigned long flags;
	struct usb_ep *out = ports[0].port->port_usb->out;

	spin_lock_irqsave(&lock_rx, flags);

	if (flag == 'n') {
		if (bm_request_rx->complete != 1000) {
			usb_ep_dequeue(out, bm_request_rx->request);
			flag = 'y';
			wake_up_interruptible(&wq);
			memset(buffer_rx, 0, MAX_TRANSFER_LENGTH);
		}
	}

	spin_unlock_irqrestore(&lock_rx, flags);
}

static void bmusb_timeout_write(unsigned long arg)
{
	unsigned long flags;
	struct usb_ep *in = ports[0].port->port_usb->in;

	spin_lock_irqsave(&lock_tx, flags);

	if (flag_tx == 'n') {
		if (bm_request_tx->complete != 2000) {
			usb_ep_dequeue(in, bm_request_tx->request);
			flag_tx = 'y';
			wake_up_interruptible(&wq_tx);
		}
	}
	spin_unlock_irqrestore(&lock_tx, flags);
}

/*bitmain usb file operation*/

/*  executed once the device is closed or releaseed by userspace
 *  @param inodep: pointer to struct inode
 *  @param filep: pointer to struct file
 */
static int bmusbrx_release(struct inode *inodep, struct file *filp)
{
	if (async_enable == 1) {
		struct usb_dev_state *ps = filp->private_data;

		if (ps) {
			del_timer_sync(&ps->timeout);
			kfree(ps);
		}
	}
	mutex_unlock(&bmusb_rx_mutex);
	return 0;
}

/* executed once the device is opened.
 *
 */
static int bmusbrx_open(struct inode *inodep, struct file *filp)
{
	int ret = 0;
	struct usb_dev_state *ps;

	ps = kmalloc(sizeof(struct usb_dev_state), GFP_KERNEL);

	if (!mutex_trylock(&bmusb_rx_mutex)) {
		pr_alert("bmusbrx_mmap: device busy!\n");
		ret = -EBUSY;
		goto out;
	}

	spin_lock_init(&ps->lock);
	spin_lock_init(&lock_rx);
	rwlock_init(&ps->ioctl_lock);
	ps->val = 0xFF;
	INIT_LIST_HEAD(&ps->async_completed);
	async_enable = 0;
	filp->private_data = ps;
	pr_info("bmusbrx_mmap: Device opened\n");

out:
	return ret;
}

static unsigned int bmusbrx_poll(struct file *filp, poll_table *wait)
{
	struct usb_dev_state *ps = filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &ps->wait, wait);
	if ((filp->f_mode & FMODE_READ) && (flag == 'n'))
		mask |= POLLIN;	//mask |= POLLIN | POLLWRNORM;
	else
		mask |= POLLERR;
	if (list_empty(&ps->list))
		mask |= POLLERR;
	return mask;

}

static int bmusbrx_flush(struct file *filp, fl_owner_t id)
{
	struct usb_ep *out = ports[0].port->port_usb->out;

	if (bm_request_rx->complete != 1000) {
		if (!bm_request_rx->request) {
			usb_ep_dequeue(out, bm_request_rx->request);
			flag = 'y';
			wake_up_interruptible(&wq);
		}
	}

	return 0;
}

static ssize_t bmusbrx_read(struct file *filp, char *buffer, size_t len, loff_t
			*offset)
{
	int ret;
	struct usb_ep *out = NULL;
	struct usb_dev_state *ps = filp->private_data;

	if (ports[0].port && ports[0].port->port_usb)
		out = ports[0].port->port_usb->out;
	else {
		pr_err("%s port is empty\n", __func__);
		ret = -EFAULT;
		goto out;
	}

	if (len > MAX_SIZE) {
		pr_err("%s len %lu > MAX_SIZE %lu\n", __func__, len, MAX_SIZE);
		ret = -EFAULT;
		goto out;
	}
	bm_request_rx->request->complete = bm_read_complete;
	bm_request_rx->request->actual = 0;
	bm_request_rx->request_length = len;
	bm_request_rx->actual_length = 0;
	bm_request_rx->complete = 0;
	ps->timeout_done = 0;
	if (async_enable == 1)
		mod_timer(&timeout_rx, jiffies + timeout_value * HZ);

	if (bm_request_rx->request->buf == NULL) {
		pr_err("%s request (%p)->buf allocation fail\n", __func__,
		       bm_request_rx->request);
		ret = -ENOMEM;
		goto out;
	}
	memset(bm_request_rx->request->buf, 0, MAX_TRANSFER_LENGTH);
	memset(buffer_rx, 0, MAX_TRANSFER_LENGTH);

	if (len > MAX_TRANSFER_LENGTH)
		bm_request_rx->request->length = MAX_TRANSFER_LENGTH;
	else
		bm_request_rx->request->length = len;

	usb_ep_queue(out, bm_request_rx->request, GFP_ATOMIC);

	wait_event_interruptible(wq, flag == 'y');

	if (copy_to_user(buffer, buffer_rx, len) == 0)
		ret = bm_request_rx->actual_length;
	else
		ret = -EFAULT;

	bm_request_rx->complete = 1000;
	if (flag == 'n')
		usb_ep_dequeue(out, bm_request_rx->request);
	flag = 'n';

out:
	return ret;
}

static ssize_t bmusbrx_write(struct file *filp, const char *buffer, size_t len,
			loff_t *offset)
{
	return 0;
}

static long bmusbrx_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct usb_dev_state *ioctl_data = filp->private_data;
	int retval = 0;
	unsigned char val;
	struct ioctl_arg data;

	memset(&data, 0, sizeof(data));
	switch (cmd) {
	case IOCTL_VALSET:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
			retval = -EFAULT;
			goto done;
		}
		write_lock(&ioctl_data->ioctl_lock);
		ioctl_data->val = data.val;
		write_unlock(&ioctl_data->ioctl_lock);
		break;
	case IOCTL_VALGET:
		read_lock(&ioctl_data->ioctl_lock);
		val = ioctl_data->val;
		read_unlock(&ioctl_data->ioctl_lock);
		data.val = val;

		if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
			retval = -EFAULT;
			goto done;
		}

		break;

	case IOCTL_VALGET_NUM:
		retval = __put_user(async_enable, (int __user *)arg);
		break;
	case IOCTL_VALSET_NUM:
		async_enable = arg;
		if (async_enable == 1) {
			init_waitqueue_head(&ioctl_data->wait);
			init_timer(&ioctl_data->timeout);
			ioctl_data->timeout.function = bmusb_timeout;
			ioctl_data->timeout.data = (unsigned long)ioctl_data;
			ioctl_data->timeout_done = 0;
			mod_timer(&ioctl_data->timeout,
				  jiffies + timeout_value * HZ);
			setup_timer(&timeout_rx, bmusb_timeout_read, 0);
		}

		break;
	default:
		retval = -ENOTTY;
	}

done:
	return retval;
}

#ifdef CONFIG_COMPAT
static long bm_serial_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations bmusbrx_fops = {
	.open = bmusbrx_open,
	.read = bmusbrx_read,
	.write = bmusbrx_write,
	.release = bmusbrx_release,
	.poll = bmusbrx_poll,
	.flush = bmusbrx_flush,
	.unlocked_ioctl = bmusbrx_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = bm_serial_compat_ptr_ioctl,
#endif
	.owner = THIS_MODULE,
};

/*TX File operations*/

/*  executed once the device is closed or releaseed by userspace
 *  @param inodep: pointer to struct inode
 *  @param filep: pointer to struct file
 */
static int bmusbtx_release(struct inode *inodep, struct file *filep)
{
	mutex_unlock(&bmusb_tx_mutex);

	return 0;
}

/* executed once the device is opened.
 *
 */
static int bmusbtx_open(struct inode *inodep, struct file *filp)
{
	int ret = 0;
	struct usb_dev_state *ps_tx;

	ps_tx = kmalloc(sizeof(struct usb_dev_state), GFP_KERNEL);

	if (!mutex_trylock(&bmusb_tx_mutex)) {
		pr_alert("bmusbtx_mmap: device busy!\n");
		ret = -EBUSY;
		goto out;
	}

	spin_lock_init(&ps_tx->lock);
	spin_lock_init(&lock_tx);
	INIT_LIST_HEAD(&ps_tx->async_completed);
	rwlock_init(&ps_tx->ioctl_lock);
	ps_tx->val = 0xFF;
	async_enable_tx = 0;
	filp->private_data = ps_tx;
	pr_info("bmusbtx: Device opened\n");

out:
	return ret;
}

static long bmusbtx_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct usb_dev_state *ioctl_data = filp->private_data;
	int retval = 0;
	unsigned char val;
	struct ioctl_arg data;

	memset(&data, 0, sizeof(data));
	switch (cmd) {
	case IOCTL_VALSET:
		if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
			retval = -EFAULT;
			goto done;
		}
		write_lock(&ioctl_data->ioctl_lock);
		ioctl_data->val = data.val;
		write_unlock(&ioctl_data->ioctl_lock);

		break;
	case IOCTL_VALGET:
		read_lock(&ioctl_data->ioctl_lock);
		val = ioctl_data->val;
		read_unlock(&ioctl_data->ioctl_lock);
		data.val = val;

		if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
			retval = -EFAULT;
			goto done;
		}

		break;

	case IOCTL_VALGET_NUM:
		retval = __put_user(async_enable_tx, (int __user *)arg);
		break;
	case IOCTL_VALSET_NUM:
		async_enable_tx = arg;
		if (async_enable_tx == 1) {
			init_waitqueue_head(&ioctl_data->wait);
			init_timer(&ioctl_data->timeout);
			ioctl_data->timeout.function = bmusb_timeout_tx;
			ioctl_data->timeout.data = (unsigned long)ioctl_data;
			ioctl_data->timeout_done = 0;
			mod_timer(&ioctl_data->timeout,
				  jiffies + timeout_value * HZ);
			setup_timer(&timeout_tx, bmusb_timeout_write, 0);
		}
		break;
	default:
		retval = -ENOTTY;
	}

done:
	return retval;
}

static unsigned int bmusbtx_poll(struct file *filp, poll_table *wait)
{
	struct usb_dev_state *ps_tx = filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &ps_tx->wait, wait);
	if ((filp->f_mode & FMODE_WRITE) && (flag_tx == 'n'))
		mask |= POLLOUT;
	else
		mask |= POLLERR;

	if (list_empty(&ps_tx->list))
		mask |= POLLERR;
	return mask;

}

static int bmusbtx_flush(struct file *filp, fl_owner_t id)
{
	struct usb_ep *in = ports[0].port->port_usb->in;

	if (bm_request_tx->complete != 2000) {
		if (!bm_request_tx->request) {
			usb_ep_dequeue(in, bm_request_tx->request);
			flag_tx = 'y';
			wake_up_interruptible(&wq_tx);
		}
	}

	return 0;
}

static ssize_t bmusbtx_read(struct file *filep, char *buffer, size_t len, loff_t
			*offset)
{
	return 0;
}

static ssize_t bmusbtx_write(struct file *filep, const char *buffer, size_t len,
			loff_t *offset)
{
	int ret;

	struct usb_ep *in;

	if (ports[0].port && ports[0].port->port_usb)
		in = ports[0].port->port_usb->in;
	else {
		pr_err("%s port is empty\n", __func__);
		ret = -EFAULT;
		goto out;
	}

	bm_request_tx->request->complete = bm_write_complete;
	bm_request_tx->complete = 0;

	memset(bm_request_tx->request->buf, 0, MAX_TRANSFER_LENGTH);
	if (copy_from_user(bm_request_tx->request->buf, buffer, len)) {
		pr_err("mchar: write fault!\n");
		ret = -EFAULT;
		goto out;
	}

	ret = len;
	bm_request_tx->request->actual = len;
	bm_request_tx->request->req_map = 0;
	if (async_enable_tx == 1)
		mod_timer(&timeout_tx, jiffies + timeout_value * HZ);

	if (len > MAX_TRANSFER_LENGTH)
		bm_request_tx->request->length = MAX_TRANSFER_LENGTH;
	else
		bm_request_tx->request->length = len;
	usb_ep_queue(in, bm_request_tx->request, GFP_ATOMIC);

	wait_event_interruptible(wq_tx, flag_tx == 'y');

	if (flag_tx == 'n')
		usb_ep_dequeue(in, bm_request_tx->request);

	bm_request_tx->complete = 2000;
	flag_tx = 'n';

out:
	return ret;
}

static const struct file_operations bmusbtx_fops = {
	.open = bmusbtx_open,
	.read = bmusbtx_read,
	.write = bmusbtx_write,
	.release = bmusbtx_release,
	.poll = bmusbtx_poll,
	.flush = bmusbtx_flush,
	.unlocked_ioctl = bmusbtx_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = bm_serial_compat_ptr_ioctl,
#endif
	.owner = THIS_MODULE,
};

static ssize_t rx_state_show(struct device *pdev, struct device_attribute *attr,
			     char *buf)
{
	return 0;
}

static ssize_t tx_state_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return 0;
}

static DEVICE_ATTR(rx_state, 0444, rx_state_show, NULL);
static DEVICE_ATTR(tx_state, 0200, NULL, tx_state_store);

static int bm_device_create(struct bmserial *gser)
{
	int err;

	/* RX device init */
	bitmain_device_rx = device_create(bitmain_class, NULL,
					  MKDEV(rx_major, 0), NULL,
					  RX_DEVICE_NAME);
	if (IS_ERR(bitmain_device_rx))
		return PTR_ERR(bitmain_device_rx);

	dev_set_drvdata(bitmain_device_rx, gser->out->driver_data);

	err = device_create_file(bitmain_device_rx, &dev_attr_rx_state);
	if (err) {
		device_destroy(bitmain_device_rx->class,
			       bitmain_device_rx->devt);
		return err;
	}

	/*TX device init */
	bitmain_device_tx = device_create(bitmain_class, NULL,
					  MKDEV(tx_major, 0), NULL,
					  TX_DEVICE_NAME);
	if (IS_ERR(bitmain_device_tx))
		return PTR_ERR(bitmain_device_tx);

	dev_set_drvdata(bitmain_device_tx, gser->in->driver_data);

	err = device_create_file(bitmain_device_tx, &dev_attr_tx_state);
	if (err) {
		device_destroy(bitmain_device_tx->class,
			       bitmain_device_tx->devt);
		return err;
	}

	return 0;
}

#if 0
static void bitmain_device_destroy(void)
{
	device_remove_file(bitmain_device_rx, &dev_attr_rx_state);
	device_destroy(bitmain_device_rx->class, bitmain_device_rx->devt);

	device_remove_file(bitmain_device_tx, &dev_attr_tx_state);
	device_destroy(bitmain_device_tx->class, bitmain_device_tx->devt);

}
#endif

static int bm_port_alloc(u8 port_num, struct usb_cdc_line_coding *coding)
{
	struct gs_port *port;
	int ret = 0;

	mutex_lock(&ports[port_num].lock);
	if (ports[port_num].port) {
		ret = -EBUSY;
		goto out;
	}

	port = kzalloc(sizeof(struct gs_port), GFP_KERNEL);
	if (port == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	tty_port_init(&port->port);
	spin_lock_init(&port->port_lock);
	//init_waitqueue_head(&port->drain_wait);
	//init_waitqueue_head(&port->close_wait);

	//INIT_LIST_HEAD(&port->read_pool);
	//INIT_LIST_HEAD(&port->read_queue);
	//INIT_LIST_HEAD(&port->write_pool);

	port->port_num = port_num;
	port->port_line_coding = *coding;

	ports[port_num].port = port;
out:
	mutex_unlock(&ports[port_num].lock);
	return ret;
}

static void bm_free_port(struct gs_port *port)
{
	/* wait for old opens to finish */
	WARN_ON(port->port_usb != NULL);
	tty_port_destroy(&port->port);
	kfree(port);
}

int bmserial_alloc_line(unsigned char *line_num)
{
	struct usb_cdc_line_coding coding;
	int ret;
	u8 port_num;

	coding.dwDTERate = cpu_to_le32(9600);
	coding.bCharFormat = 8;
	coding.bParityType = USB_CDC_NO_PARITY;
	coding.bDataBits = USB_CDC_1_STOP_BITS;

	for (port_num = 0; port_num < MAX_U_SERIAL_PORTS; port_num++) {
		ret = bm_port_alloc(port_num, &coding);
		if (ret == -EBUSY)
			continue;
		if (ret)
			return ret;
		break;
	}
	if (ret)
		return ret;

	/* ... and sysfs class devices, so mdev/udev make /dev/ttyGS* */

	*line_num = port_num;
	//gserial_console_init();
	//
	return ret;
}
EXPORT_SYMBOL_GPL(bmserial_alloc_line);

void bmserial_free_line(unsigned char port_num)
{
	struct gs_port *port;

	mutex_lock(&ports[port_num].lock);
	if (WARN_ON(!ports[port_num].port)) {
		mutex_unlock(&ports[port_num].lock);
		return;
	}
	port = ports[port_num].port;
	ports[port_num].port = NULL;
	mutex_unlock(&ports[port_num].lock);

	bm_free_port(port);
}
EXPORT_SYMBOL_GPL(bmserial_free_line);

/**
 * bmserial_connect - notify TTY I/O glue that USB link is active
 * @gser: the function, set up with endpoints and descriptors
 * @port_num: which port is active
 * Context: any (usually from irq)
 *
 * This is called activate endpoints and let the TTY layer know that
 * the connection is active ... not unlike "carrier detect".  It won't
 * necessarily start I/O queues; unless the TTY is held open by any
 * task, there would be no point.  However, the endpoints will be
 * activated so the USB host can perform I/O, subject to basic USB
 * hardware flow control.
 *
 * Caller needs to have set up the endpoints and USB function in @dev
 * before calling this, as well as the appropriate (speed-specific)
 * endpoint descriptors, and also have allocate @port_num by calling
 * @bmserial_alloc_line().
 *
 * Returns negative errno or zero.
 * On success, ep->driver_data will be overwritten.
 */
int bmserial_connect(struct bmserial *gser, u8 port_num)
{
	struct gs_port *port;
	int status;
	int ret;

	if (port_num >= MAX_U_SERIAL_PORTS)
		return -ENXIO;

	port = ports[port_num].port;

	if (!port) {
		pr_err("serial line %d not allocated.\n", port_num);
		return -EINVAL;
	}

	if (port->port_usb) {
		pr_err("serial line %d is in use.\n", port_num);
		return -EBUSY;
	}

	/* activate the endpoints */
	status = usb_ep_enable(gser->in);
	if (status < 0)
		return status;
	gser->in->driver_data = port;

	status = usb_ep_enable(gser->out);
	if (status < 0)
		goto fail_out;
	gser->out->driver_data = port;
/*bmdevice create*/
	if (create_complete == 0) {
		rx_major = register_chrdev(0, RX_DEVICE_NAME, &bmusbrx_fops);
		tx_major = register_chrdev(0, TX_DEVICE_NAME, &bmusbtx_fops);

		if (rx_major < 0) {
			pr_notice("bmusb: fail to register major number!");
			ret = rx_major;
		}

		bitmain_class = class_create(THIS_MODULE, RX_CLASS_NAME);
		if (IS_ERR(bitmain_class))
			return PTR_ERR(bitmain_class);

		if (bm_device_create(gser) < 0)
			ret = -1;

		bm_request_rx = kzalloc(sizeof(struct bm_serial_request),
					GFP_KERNEL);
		bm_request_tx = kzalloc(sizeof(struct bm_serial_request),
					GFP_KERNEL);
		buffer_rx = vmalloc(MAX_SIZE);
		if ((bm_request_tx == NULL) ||
		    (bm_request_rx == NULL) || (buffer_rx == NULL)) {
			pr_err("%s resource allocation fail\n", __func__);
			status = ENOMEM;
			goto fail_out;
		}
		bm_request_tx->request =
		    usb_ep_alloc_request(gser->in, GFP_KERNEL);
		bm_request_tx->request->buf = kmalloc(MAX_SIZE, GFP_KERNEL);

		bm_request_rx->request =
		    usb_ep_alloc_request(gser->out, GFP_KERNEL);
		bm_request_rx->request->buf =
		    kmalloc(MAX_TRANSFER_LENGTH, GFP_KERNEL);

		create_complete = 1;
	}
#if 1
	/* then tell the tty glue that I/O can work */
	//spin_lock_irqsave(&port->port_lock, flags);
	gser->ioport = port;
	port->port_usb = gser;

	/* REVISIT unclear how best to handle this state...
	 * we don't really couple it with the Linux TTY.
	 */
	gser->port_line_coding = port->port_line_coding;

	/* REVISIT if waiting on "carrier detect", signal. */

	/* if it's already open, start I/O ... and notify the serial
	 * protocol about open/close status (connect/disconnect).
	 */
	if (port->port.count) {
		pr_debug("bmserial_connect: start ttyGS%d\n", port->port_num);
		bm_start_io(port);
		if (gser->connect)
			gser->connect(gser);
	} else {
		if (gser->disconnect)
			gser->disconnect(gser);
	}

	//spin_unlock_irqrestore(&port->port_lock, flags);
#endif
	return status;

fail_out:
	usb_ep_disable(gser->in);
	return status;
}
EXPORT_SYMBOL_GPL(bmserial_connect);
/**
 * bmserial_disconnect - notify TTY I/O glue that USB link is inactive
 * @gser: the function, on which bmserial_connect() was called
 * Context: any (usually from irq)
 *
 * This is called to deactivate endpoints and let the TTY layer know
 * that the connection went inactive ... not unlike "hangup".
 *
 * On return, the state is as if bmserial_connect() had never been called;
 * there is no active USB I/O on these endpoints.
 */
void bmserial_disconnect(struct bmserial *gser)
{
	struct gs_port *port = gser->ioport;

	port->port_usb = NULL;
	gser->ioport = NULL;
	/* disable endpoints, aborting down any active I/O */
	usb_ep_disable(gser->out);
	usb_ep_disable(gser->in);
}
EXPORT_SYMBOL_GPL(bmserial_disconnect);

static int bmserial_init(void)
{
	int i;

	for (i = 0; i < MAX_U_SERIAL_PORTS; i++)
		mutex_init(&ports[i].lock);

	create_complete = 0;

	return 0;
}

module_init(bmserial_init);

static void bmserial_cleanup(void)
{
	device_destroy(bitmain_device_rx->class, bitmain_device_rx->devt);
	device_destroy(bitmain_device_tx->class, bitmain_device_tx->devt);
	if (!IS_ERR(bitmain_class))
		class_destroy(bitmain_class);

	if (!IS_ERR(bitmain_class))
		class_destroy(bitmain_class);
	unregister_chrdev(rx_major, RX_DEVICE_NAME);
	unregister_chrdev(tx_major, TX_DEVICE_NAME);
}

module_exit(bmserial_cleanup);

MODULE_LICENSE("GPL");
