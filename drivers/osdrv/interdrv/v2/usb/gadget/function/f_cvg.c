/*
 * f_cvg.c - Cvitek USB gadget driver
 *
 * Copyright (C) 2003-2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/usb/composite.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/eventfd.h>
#include <linux/security.h>
#include <linux/cred.h>
#include <linux/version.h>
#include "f_cvg.h"
#include "u_f.h"
#include <linux/dma-buf.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#include <linux/sched/signal.h>
#endif
#define CVG_MAX_TRANSFER_LENGTH	65536u
/*
 * BMUSB FUNCTION ... a testing vehicle for USB peripherals,
 *
 * This takes messages of various sizes written OUT to a device, and loops
 * them back so they can be read IN from it.  It has been used by certain
 * test applications.  It supports limited testing of data queueing logic.
 */
struct f_cvg {
	struct usb_function	function;

	struct usb_ep		*in_ep;
	struct usb_ep		*out_ep;

	unsigned int		num_ep_in;
	unsigned int		num_ep_out;
	unsigned int		connect;
	spinlock_t		lock;
	struct miscdevice	*dev;
	void			*zlp_buf;
	struct list_head	ps_list;
};

/* For configfs usage. */
struct f_cvg_opts {
	struct usb_function_instance func_inst;
	unsigned int		num_ep_in;
	unsigned int		num_ep_out;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This is to protect the data from concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

struct usb_dev_state {
	struct list_head list;      /* state list */
	struct f_cvg *cvg;
	struct file *file;
	spinlock_t lock;            /* protects the async urb lists */
	struct list_head async_tx_pending;
	struct list_head async_tx_completed;
	struct list_head async_rx_pending;
	struct list_head async_rx_completed;
	struct list_head memory_list;
	wait_queue_head_t wait_tx;     /* wake up if a request completed */
	wait_queue_head_t wait_rx;     /* wake up if a request completed */
	unsigned int connsignr;
	struct pid *conn_pid;
	const struct cred *cred;
	void __user *conncontext;
	u32 secid;
	bool privileges_dropped;
};

struct usb_memory {
	struct list_head memlist;
	int vma_use_count;
	int urb_use_count;
	u32 size;
	void *mem;
	dma_addr_t dma;
	unsigned long vm_start;
	struct usb_dev_state *ps;
	struct dma_buf *dma_buf;
};

struct async {
	struct list_head asynclist;
	struct usb_dev_state *ps;
	struct usb_ep *ep;
	struct pid *pid;
	const struct cred *cred;
	unsigned int signr;
	unsigned int ifnum;
	void __user *userbuffer;
	void __user *userurb;
	struct usb_request *urb;
	struct usb_memory *usbm;
	unsigned int mem_usage;
	int status;
	u32 secid;
	int buffer_length;
	int actual_length;
	int zero;
};


static struct f_cvg *cvg_gadget;

static inline struct f_cvg *func_to_cvg(struct usb_function *f)
{
	return container_of(f, struct f_cvg, function);
}

/*-------------------------------------------------------------------------*/

static struct usb_interface_descriptor cvg_intf = {
	.bLength =		sizeof(cvg_intf),
	.bDescriptorType =	USB_DT_INTERFACE,

	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor fs_cvg_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor fs_cvg_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *fs_cvg_descs[] = {
	(struct usb_descriptor_header *) &cvg_intf,
	(struct usb_descriptor_header *) &fs_cvg_sink_desc,
	(struct usb_descriptor_header *) &fs_cvg_source_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor hs_cvg_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor hs_cvg_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_descriptor_header *hs_cvg_descs[] = {
	(struct usb_descriptor_header *) &cvg_intf,
	(struct usb_descriptor_header *) &hs_cvg_source_desc,
	(struct usb_descriptor_header *) &hs_cvg_sink_desc,
	NULL,
};

/* super speed support: */

static struct usb_endpoint_descriptor ss_cvg_source_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_cvg_source_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_endpoint_descriptor ss_cvg_sink_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,

	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor ss_cvg_sink_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst =		0,
	.bmAttributes =		0,
	.wBytesPerInterval =	0,
};

static struct usb_descriptor_header *ss_cvg_descs[] = {
	(struct usb_descriptor_header *) &cvg_intf,
	(struct usb_descriptor_header *) &ss_cvg_source_desc,
	(struct usb_descriptor_header *) &ss_cvg_source_comp_desc,
	(struct usb_descriptor_header *) &ss_cvg_sink_desc,
	(struct usb_descriptor_header *) &ss_cvg_sink_comp_desc,
	NULL,
};

/* function-specific strings: */

static struct usb_string strings_cvg[] = {
	[0].s = "cvitek usb",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_cvg = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_cvg,
};

static struct usb_gadget_strings *cvg_strings[] = {
	&stringtab_cvg,
	NULL,
};

/*-------------------------------------------------------------------------*/

static void dec_usb_memory_use_count(struct usb_memory *usbm, int *count)
{
	struct usb_dev_state *ps = usbm->ps;
	unsigned long flags;

	spin_lock_irqsave(&ps->lock, flags);
	--*count;
	if (usbm->urb_use_count == 0 && usbm->vma_use_count == 0) {
		list_del(&usbm->memlist);
		spin_unlock_irqrestore(&ps->lock, flags);
		/* ion buffer shall be freed by user mode. */
		if (!usbm->dma_buf)
			kfree(usbm->mem);
		kfree(usbm);
	} else {
		spin_unlock_irqrestore(&ps->lock, flags);
	}
}

static void cvg_vm_open(struct vm_area_struct *vma)
{
	struct usb_memory *usbm = vma->vm_private_data;
	unsigned long flags;

	spin_lock_irqsave(&usbm->ps->lock, flags);
	++usbm->vma_use_count;
	spin_unlock_irqrestore(&usbm->ps->lock, flags);
}

static void cvg_vm_close(struct vm_area_struct *vma)
{
	struct usb_memory *usbm = vma->vm_private_data;

	dec_usb_memory_use_count(usbm, &usbm->vma_use_count);
}

const struct vm_operations_struct cvg_vm_ops = {
	.open = cvg_vm_open,
	.close = cvg_vm_close
};

static int cvg_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct usb_memory *usbm = NULL;
	struct usb_dev_state *ps = file->private_data;
	struct f_cvg *cvg = ps->cvg;
	size_t size = vma->vm_end - vma->vm_start;
	void *mem;
	unsigned long flags;
	int ret;

	usbm = kzalloc(sizeof(struct usb_memory), GFP_KERNEL);
	if (!usbm) {
		ret = -ENOMEM;
		goto error;
	}

	mem = kzalloc(size, GFP_KERNEL);
	if (!mem) {
		ret = -ENOMEM;
		goto error_free_usbm;
	}
	dev_dbg(cvg->dev->this_device, "mmap %p\n", mem);
	usbm->mem = mem;
	usbm->size = size;
	usbm->ps = ps;
	usbm->vm_start = vma->vm_start;
	usbm->vma_use_count = 1;
	INIT_LIST_HEAD(&usbm->memlist);

	if (remap_pfn_range(vma, vma->vm_start,
			virt_to_phys(usbm->mem) >> PAGE_SHIFT,
			size, vma->vm_page_prot) < 0) {
		dec_usb_memory_use_count(usbm, &usbm->vma_use_count);
		return -EAGAIN;
	}

	vma->vm_flags |= VM_IO;
	vma->vm_flags |= (VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_ops = &cvg_vm_ops;
	vma->vm_private_data = usbm;

	spin_lock_irqsave(&ps->lock, flags);
	list_add_tail(&usbm->memlist, &ps->memory_list);
	spin_unlock_irqrestore(&ps->lock, flags);

	return 0;

error_free_usbm:
	kfree(usbm);
error:
	return ret;
}

static int _cvg_queue_ion(struct usb_dev_state *ps, struct cvg_ion_queue *queue)
{
	struct usb_memory *usbm = NULL;
	struct dma_buf *db;
	struct f_cvg *cvg = ps->cvg;
	struct usb_composite_dev *cdev = cvg->function.config->cdev;
	struct dma_buf_attachment *attach;
	struct sg_table *table;
	unsigned long flags;
	struct scatterlist *sg;
	void *vaddr;
	int ret = 0;
	int fd;

	fd = (int)queue->fd;

	if (fd >= 0) {
		db = dma_buf_get(fd);
		if (IS_ERR(db)) {
			ERROR(cdev, "dma_buf get fail %p\n", db);
			return PTR_ERR(db);
		}
	} else {
		ERROR(cdev, "no proper mapping id\n");
		return -EFAULT;
	}

	usbm = kzalloc(sizeof(struct usb_memory), GFP_KERNEL);
	if (!usbm) {
		ret = -ENOMEM;
		ERROR(cdev, "memory insufficient\n");
		goto err1;
	}

	attach = dma_buf_attach(db, cvg->dev->this_device);
	if (IS_ERR(attach)) {
		ret = PTR_ERR(attach);
		ERROR(cdev, "dma buf attach fail\n");
		goto err1;
	}

	/* we don't care the direction because the driver would handle
	 * the dma mapping later.
	 */
	table = dma_buf_map_attachment(attach, 0);
	if (IS_ERR(table)) {
		ret = PTR_ERR(table);
		ERROR(cdev, "dma buf map attach fail\n");
		goto err2;
	}

	if (table->nents != 1) {
		ERROR(cdev, "memory is inconsistent\n");
		ret = -EINVAL;
		goto err3;
	}

	vaddr = queue->vbase;
	if (!vaddr) {
		ret = -EINVAL;
		ERROR(cdev, "vbase is not set\n");
		goto err3;
	}
	dev_dbg(cvg->dev->this_device, "ion queue %p\n", vaddr);
	sg = table->sgl;
	usbm->dma_buf = db;
	usbm->ps = ps;
	usbm->vm_start = (unsigned long)vaddr;
	usbm->vma_use_count = 1;
	usbm->size = sg_dma_len(sg);
	usbm->dma = sg_dma_address(sg);
	INIT_LIST_HEAD(&usbm->memlist);

	dev_dbg(cvg->dev->this_device, "ion queue phy %llx, vmem %lx, size = %d\n",
			usbm->dma, usbm->vm_start, usbm->size);

	spin_lock_irqsave(&ps->lock, flags);
	list_add_tail(&usbm->memlist, &ps->memory_list);
	spin_unlock_irqrestore(&ps->lock, flags);
err3:
	dma_buf_unmap_attachment(attach, table, 0);
err2:
	dma_buf_detach(db, attach);
err1:
	dma_buf_put(db);

	return ret;
}

static int cvg_queue_ion(struct usb_dev_state *ps, void __user *ion_queue)
{
	struct f_cvg *cvg = ps->cvg;
	struct usb_composite_dev *cdev = cvg->function.config->cdev;
	int ret = 0;
	struct cvg_ion_queue queue;

	if (copy_from_user(&queue, ion_queue, sizeof(queue))) {
		ERROR(cdev, "copy user fail\n");
		return -EFAULT;
	}

	ret = _cvg_queue_ion(ps, &queue);

	return ret;
}

/*
 * async list handling
 */

static void async_completed(struct usb_ep *ep, struct usb_request *req)
{
	struct async *as = req->context;
	struct usb_dev_state *ps = as->ps;
	struct f_cvg *cvg = ps->cvg;
	struct siginfo sinfo;
	struct pid *pid = NULL;
	u32 secid = 0;
	const struct cred *cred = NULL;
	int signr;
	int dir = ep->desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	sigval_t addr;
#endif

	dev_dbg(cvg->dev->this_device,
			"%s: status : %d, uurb req %d, uurb actual %d, urb req %d, urb actual %d\n",
			__func__, req->status, as->buffer_length, as->actual_length,
			req->length, req->actual);
	if (!req->status) {
		/*
		 * We treat the status 0 as transfer success and
		 * the acutal length is the request length for TX.
		 */
		int actual = dir ? req->length : req->actual;

		as->actual_length += actual;
		if ((as->actual_length <= as->buffer_length) &&
		    actual && !(req->actual % ep->maxpacket)) {
			/* append a zero-length packet*/
			if ((as->actual_length == as->buffer_length) &&
			    as->zero) {
				as->zero = 0;
				req->length = 0;
				req->buf = cvg->zlp_buf;
				usb_ep_queue(ep, req, GFP_ATOMIC);
				return;
			}
		}
	}
	spin_lock(&ps->lock);
	if (dir)
		list_move_tail(&as->asynclist, &ps->async_tx_completed);
	else
		list_move_tail(&as->asynclist, &ps->async_rx_completed);
	as->status = req->status;
	signr = as->signr;
	if (signr) {
		memset(&sinfo, 0, sizeof(sinfo));
		sinfo.si_signo = as->signr;
		sinfo.si_errno = as->status;
		sinfo.si_code = SI_ASYNCIO;
		sinfo.si_addr = as->userurb;
		pid = get_pid(as->pid);
		cred = get_cred(as->cred);
		secid = as->secid;
	}

	spin_unlock(&ps->lock);

	if (signr) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		addr.sival_ptr = sinfo.si_addr;
		kill_pid_usb_asyncio(sinfo.si_signo, sinfo.si_errno, addr, pid, cred);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
		kill_pid_info_as_cred(sinfo.si_signo, &sinfo, pid, cred);
#else
		kill_pid_info_as_cred(sinfo.si_signo, &sinfo, pid, cred, secid);
#endif
		put_pid(pid);
		put_cred(cred);
	}

	if (dir)
		wake_up(&ps->wait_tx);
	else
		wake_up(&ps->wait_rx);
}

static struct async *alloc_async(struct usb_ep *ep)
{
	struct async *as;

	as = kzalloc(sizeof(struct async), GFP_KERNEL);
	if (!as)
		return NULL;

	as->urb = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (!as->urb) {
		kfree(as);
		return NULL;
	}
	as->ep = ep;
	return as;
}

static void free_async(struct async *as)
{
	put_pid(as->pid);
	if (as->cred)
		put_cred(as->cred);

	if (as->usbm == NULL)
		kfree(as->urb->buf);
	else
		dec_usb_memory_use_count(as->usbm, &as->usbm->urb_use_count);

	usb_ep_free_request(as->ep, as->urb);
	kfree(as);
}

static void async_newpending(struct async *as)
{
	struct usb_dev_state *ps = as->ps;
	struct usb_ep *ep = as->ep;
	int dir = ep->desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK;
	unsigned long flags;

	spin_lock_irqsave(&ps->lock, flags);
	if (dir)
		list_add_tail(&as->asynclist, &ps->async_tx_pending);
	else
		list_add_tail(&as->asynclist, &ps->async_rx_pending);

	spin_unlock_irqrestore(&ps->lock, flags);
}

static void async_removepending(struct async *as)
{
	struct usb_dev_state *ps = as->ps;
	unsigned long flags;

	spin_lock_irqsave(&ps->lock, flags);
	list_del_init(&as->asynclist);
	spin_unlock_irqrestore(&ps->lock, flags);
}

static struct async *async_gettxcompleted(struct usb_dev_state *ps)
{
	unsigned long flags;
	struct async *as = NULL;

	spin_lock_irqsave(&ps->lock, flags);
	if (!list_empty(&ps->async_tx_completed)) {
		as = list_entry(ps->async_tx_completed.next, struct async,
				asynclist);
		list_del_init(&as->asynclist);
	}
	spin_unlock_irqrestore(&ps->lock, flags);
	return as;
}

static struct async *async_getrxcompleted(struct usb_dev_state *ps)
{
	unsigned long flags;
	struct async *as = NULL;

	spin_lock_irqsave(&ps->lock, flags);
	if (!list_empty(&ps->async_rx_completed)) {
		as = list_entry(ps->async_rx_completed.next, struct async,
				asynclist);
		list_del_init(&as->asynclist);
	}
	spin_unlock_irqrestore(&ps->lock, flags);
	return as;
}

static struct async *async_getpending(struct usb_dev_state *ps,
					     void __user *userurb)
{
	struct async *as;
	struct cvg_uurb __user *uurb = userurb;

	if ((uurb->flags & CVG_UURB_DIR_MASK) == CVG_UURB_DIR_IN) {
		list_for_each_entry(as, &ps->async_rx_pending, asynclist)
			if (as->userurb == userurb) {
				list_del_init(&as->asynclist);
				return as;
			}
	} else {
		list_for_each_entry(as, &ps->async_tx_pending, asynclist)
			if (as->userurb == userurb) {
				list_del_init(&as->asynclist);
				return as;
			}
	}

	return NULL;
}

static void destroy_async(struct usb_dev_state *ps, struct list_head *list)
{
	struct usb_request *urb;
	struct f_cvg *cvg = ps->cvg;
	struct usb_composite_dev *cdev = cvg->function.config->cdev;
	struct async *as;
	unsigned long flags;

	DBG(cdev, "destry async %p\n", list);
	spin_lock_irqsave(&ps->lock, flags);
	while (!list_empty(list)) {
		as = list_entry(list->next, struct async, asynclist);
		list_del_init(&as->asynclist);
		urb = as->urb;

		/* drop the spinlock so the completion handler can run */
		spin_unlock_irqrestore(&ps->lock, flags);
		/* dequeue the request here with block-waiting. */
		usb_ep_dequeue(as->ep, as->urb);
		spin_lock_irqsave(&ps->lock, flags);
	}
	spin_unlock_irqrestore(&ps->lock, flags);
}

static void destroy_all_async(struct usb_dev_state *ps)
{
	destroy_async(ps, &ps->async_tx_pending);
	destroy_async(ps, &ps->async_rx_pending);
}

static void destroy_all_ps(struct f_cvg *cvg)
{
	struct usb_dev_state *ps;
	unsigned long flags;

	spin_lock_irqsave(&cvg->lock, flags);
	while (!list_empty(&cvg->ps_list)) {
		ps = list_entry(cvg->ps_list.next, struct usb_dev_state, list);
		destroy_all_async(ps);
		wake_up_all(&ps->wait_tx);
		wake_up_all(&ps->wait_rx);
		list_del_init(&ps->list);
	}
	spin_unlock_irqrestore(&cvg->lock, flags);
}

static void signal_connect(struct f_cvg *cvg)
{
	struct usb_dev_state *ps;
	struct siginfo sinfo;
	unsigned long flags;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	sigval_t addr;
#endif

	spin_lock_irqsave(&cvg->lock, flags);
	list_for_each_entry(ps, &cvg->ps_list, list) {
		if (ps->connsignr) {
			memset(&sinfo, 0, sizeof(sinfo));
			sinfo.si_signo = ps->connsignr;
			sinfo.si_errno = cvg->connect ? 0 : EPIPE;
			sinfo.si_code = SI_ASYNCIO;
			sinfo.si_addr = ps->conncontext;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			addr.sival_ptr = sinfo.si_addr;
			kill_pid_usb_asyncio(ps->connsignr, sinfo.si_errno, addr,
					ps->conn_pid, ps->cred);
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
			kill_pid_info_as_cred(ps->connsignr, &sinfo,
					ps->conn_pid, ps->cred);
#else
			kill_pid_info_as_cred(ps->connsignr, &sinfo,
					ps->conn_pid, ps->cred, ps->secid);
#endif
		}
	}
	spin_unlock_irqrestore(&cvg->lock, flags);
}

static struct usb_memory *
find_memory_area(struct usb_dev_state *ps, const struct cvg_uurb *uurb)
{
	struct usb_memory *usbm = NULL, *iter;
	unsigned long flags;
	unsigned long uurb_start = (unsigned long)uurb->buffer;

	dev_dbg(cvg_gadget->dev->this_device, "uurb buf = [%p, %lx]\n",
			uurb->buffer, uurb_start);
	spin_lock_irqsave(&ps->lock, flags);
	list_for_each_entry(iter, &ps->memory_list, memlist) {
		if (uurb_start >= iter->vm_start &&
				uurb_start < iter->vm_start + iter->size) {
			if (uurb->buffer_length > iter->vm_start + iter->size -
					uurb_start) {
				usbm = ERR_PTR(-EINVAL);
			} else {
				usbm = iter;
				usbm->urb_use_count++;
			}
			break;
		}
	}
	spin_unlock_irqrestore(&ps->lock, flags);
	return usbm;
}

static int copy_urb_data_to_user(u8 __user *userbuffer,
				 struct usb_request *urb, unsigned int len)
{
	if (copy_to_user(userbuffer, urb->buf, len))
		return -EFAULT;

	return 0;
}

static int cvg_unlinkurb(struct usb_dev_state *ps, void __user *arg)
{
	struct usb_request *urb;
	struct async *as;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&ps->lock, flags);
	as = async_getpending(ps, arg);
	if (!as) {
		spin_unlock_irqrestore(&ps->lock, flags);
		return -EINVAL;
	}

	urb = as->urb;
	spin_unlock_irqrestore(&ps->lock, flags);

	ret = usb_ep_dequeue(as->ep, as->urb);

	return ret;
}

static int cvg_processcompl(struct async *as, void __user * __user *arg)
{
	struct usb_request *urb = as->urb;
	struct cvg_uurb __user *uurb = as->userurb;
	void __user *addr = as->userurb;

	if (as->userbuffer && as->actual_length) {
		if (copy_urb_data_to_user(as->userbuffer, urb, as->actual_length))
			goto err_out;
	}
	if (put_user(as->status, &uurb->status))
		goto err_out;
	if (put_user(as->actual_length, &uurb->actual_length))
		goto err_out;

	if (put_user(addr, (void __user * __user *)arg))
		return -EFAULT;
	return 0;

err_out:
	return -EFAULT;
}

static int cvg_reaptxurbnonblock(struct usb_dev_state *ps, void __user *arg)
{
	int retval;
	struct async *as;

	as = async_gettxcompleted(ps);
	if (as) {
		retval = cvg_processcompl(as, (void __user * __user *)arg);
		free_async(as);
	} else {
		retval = -EAGAIN;
	}
	return retval;
}

static int cvg_reaprxurbnonblock(struct usb_dev_state *ps, void __user *arg)
{
	int retval;
	struct async *as;

	as = async_getrxcompleted(ps);
	if (as) {
		retval = cvg_processcompl(as, (void __user * __user *)arg);
		free_async(as);
	} else {
		retval = -EAGAIN;
	}
	return retval;
}

static int cvg_do_submiturb(struct usb_dev_state *ps, struct cvg_uurb *uurb,
			void __user *arg)
{
	struct f_cvg *cvg = ps->cvg;
	struct usb_ep *ep;
	struct async *as = NULL;
	unsigned int u;
	int ret, is_in;

	if (uurb->buffer_length > 0 && !uurb->buffer)
		return -EINVAL;

	if (!cvg)
		return -ENODEV;

	if ((uurb->flags & CVG_UURB_DIR_MASK) == CVG_UURB_DIR_IN) {
		ep = cvg->out_ep;
		is_in = 1;
	} else {
		ep = cvg->in_ep;
		is_in = 0;
	}

	if (!ep)
		return -ENOENT;

	if (!ep->enabled)
		return -ENOENT;

	switch (uurb->type) {
	case CVG_UURB_TYPE_BULK:
		break;
	default:
		return -EINVAL;
	}

	if (uurb->buffer_length >= CVG_XFER_MAX) {
		ret = -EINVAL;
		goto error;
	}

	if (uurb->buffer_length > 0 &&
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
			!access_ok(uurb->buffer, uurb->buffer_length))
#else
			!access_ok(is_in ? VERIFY_WRITE : VERIFY_READ,
				uurb->buffer, uurb->buffer_length))
#endif
	{
		ret = -EFAULT;
		goto error;
	}

	as = alloc_async(ep);
	if (!as) {
		ret = -ENOMEM;
		goto error;
	}

	as->usbm = find_memory_area(ps, uurb);
	if (IS_ERR(as->usbm)) {
		ret = PTR_ERR(as->usbm);
		as->usbm = NULL;
		goto error;
	}

	u = sizeof(struct async) + sizeof(struct usb_request) + uurb->buffer_length;
	as->mem_usage = u;

	if (uurb->buffer_length > 0) {
		if (as->usbm) {
			unsigned long uurb_start = (unsigned long)uurb->buffer;
			/* When using ION, dma mapping is handled by user mode. */
			if (as->usbm->dma_buf) {
				as->urb->dma = as->usbm->dma +
					(uurb_start - as->usbm->vm_start);
				dev_dbg(cvg->dev->this_device,
					"uurb buf %lx, kmem %llx\n",
					uurb_start, as->urb->dma);
			} else {
				as->urb->buf = as->usbm->mem +
					(uurb_start - as->usbm->vm_start);
				dev_dbg(cvg->dev->this_device,
					"uurb buf %lx, kmem %p\n",
					uurb_start, as->urb->buf);
			}
		} else {
			as->urb->buf = kmalloc(uurb->buffer_length,
					GFP_KERNEL);
			if (!as->urb->buf) {
				ret = -ENOMEM;
				goto error;
			}
			if (!is_in) {
				if (copy_from_user(as->urb->buf,
						   uurb->buffer,
						   uurb->buffer_length)) {
					ret = -EFAULT;
					goto error;
				}
			}
		}
		as->zero = !!(uurb->flags & CVG_UURB_APPEND_ZERO);
	} else if (!uurb->buffer_length) {
		as->urb->buf = cvg->zlp_buf;
	}

	as->urb->complete = async_completed;
	as->urb->actual = 0;
	as->urb->length = uurb->buffer_length > CVG_MAX_TRANSFER_LENGTH ?
		CVG_MAX_TRANSFER_LENGTH : uurb->buffer_length;

	as->urb->context = as;
	as->ps = ps;
	as->userurb = arg;
	if (!as->usbm && is_in && uurb->buffer_length)
		as->userbuffer = uurb->buffer;
	as->signr = uurb->signr;
	as->pid = get_pid(task_pid(current));
	as->cred = get_current_cred();
	as->buffer_length = uurb->buffer_length;
	as->actual_length = 0;
	security_task_getsecid(current, &as->secid);

	async_newpending(as);

	spin_lock_irq(&ps->lock);
	ret = usb_ep_queue(ep, as->urb, GFP_ATOMIC);
	spin_unlock_irq(&ps->lock);

	if (ret) {
		dev_dbg(cvg->dev->this_device,
			   "usbfs: usb_submit_urb returned %d\n", ret);
		async_removepending(as);
		goto error;
	}
	dev_dbg(cvg->dev->this_device, "submit urb success!!\n");
	return 0;

 error:
	if (as && as->usbm)
		dec_usb_memory_use_count(as->usbm, &as->usbm->urb_use_count);
	if (as)
		free_async(as);
	return ret;
}

static int cvg_submiturb(struct usb_dev_state *ps, void __user *arg)
{
	struct cvg_uurb uurb;

	if (copy_from_user(&uurb, arg, sizeof(uurb)))
		return -EFAULT;

	return cvg_do_submiturb(ps, &uurb, arg);
}

static int proc_connectsignal(struct usb_dev_state *ps, void __user *arg)
{
	struct f_cvg *cvg = ps->cvg;
	struct cvg_connectsignal ds;

	if (copy_from_user(&ds, arg, sizeof(ds)))
		return -EFAULT;
	ps->connsignr = ds.signr;
	ps->conncontext = ds.context;

	signal_connect(cvg);
	return 0;
}

#ifdef CONFIG_COMPAT

static int get_urb32(struct cvg_uurb *kuurb,
		     struct cvg_uurb32 __user *uurb)
{
	__u32  uptr;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok(uurb, sizeof(*uurb)) ||
	    __get_user(kuurb->type, &uurb->type) ||
	    __get_user(kuurb->status, &uurb->status) ||
	    __get_user(kuurb->flags, &uurb->flags) ||
	    __get_user(kuurb->buffer_length, &uurb->buffer_length) ||
	    __get_user(kuurb->actual_length, &uurb->actual_length) ||
	    __get_user(kuurb->error_count, &uurb->error_count) ||
	    __get_user(kuurb->signr, &uurb->signr))
		return -EFAULT;
#else
	if (!access_ok(VERIFY_READ, uurb, sizeof(*uurb)) ||
	    __get_user(kuurb->type, &uurb->type) ||
	    __get_user(kuurb->status, &uurb->status) ||
	    __get_user(kuurb->flags, &uurb->flags) ||
	    __get_user(kuurb->buffer_length, &uurb->buffer_length) ||
	    __get_user(kuurb->actual_length, &uurb->actual_length) ||
	    __get_user(kuurb->error_count, &uurb->error_count) ||
	    __get_user(kuurb->signr, &uurb->signr))
		return -EFAULT;
#endif

	if (__get_user(uptr, &uurb->buffer))
		return -EFAULT;
	kuurb->buffer = compat_ptr(uptr);
	if (__get_user(uptr, &uurb->usercontext))
		return -EFAULT;
	kuurb->usercontext = compat_ptr(uptr);

	return 0;
}

static int cvg_submiturb_compat(struct usb_dev_state *ps, void __user *arg)
{
	struct cvg_uurb uurb;

	if (get_urb32(&uurb, (struct cvg_uurb32 __user *)arg))
		return -EFAULT;

	return cvg_do_submiturb(ps, &uurb, arg);
}

static int cvg_processcompl_compat(struct async *as, void __user * __user *arg)
{
	struct usb_request *urb = as->urb;
	struct cvg_uurb32 __user *uurb = as->userurb;
	void __user *addr = as->userurb;

	if (as->userbuffer && as->actual_length) {
		if (copy_urb_data_to_user(as->userbuffer, urb, as->actual_length))
			return -EFAULT;
	}
	if (put_user(as->status, &uurb->status))
		return -EFAULT;
	if (put_user(as->actual_length, &uurb->actual_length))
		return -EFAULT;

	if (put_user(ptr_to_compat(addr), (u32 __user *)arg))
		return -EFAULT;
	return 0;
}

static int cvg_reaptxurbnonblock_compat(struct usb_dev_state *ps, void __user *arg)
{
	int retval;
	struct async *as;

	as = async_gettxcompleted(ps);
	if (as) {
		retval = cvg_processcompl_compat(as, (void __user * __user *)arg);
		free_async(as);
	} else {
		retval = -ENODEV;
	}
	return retval;
}

static int cvg_reaprxurbnonblock_compat(struct usb_dev_state *ps, void __user *arg)
{
	int retval;
	struct async *as;

	as = async_getrxcompleted(ps);
	if (as) {
		retval = cvg_processcompl_compat(as, (void __user * __user *)arg);
		free_async(as);
	} else {
		retval = -ENODEV;
	}
	return retval;
}

static int proc_connectsignal_compat(struct usb_dev_state *ps, void __user *arg)
{
	struct f_cvg *cvg = ps->cvg;
	struct cvg_connectsignal32 ds;

	if (copy_from_user(&ds, arg, sizeof(ds)))
		return -EFAULT;
	ps->connsignr = ds.signr;
	ps->conncontext = compat_ptr(ds.context);
	signal_connect(cvg);
	return 0;
}

static int cvg_queue_ion32(struct usb_dev_state *ps, void __user *arg)
{
	struct f_cvg *cvg = ps->cvg;
	struct usb_composite_dev *cdev = cvg->function.config->cdev;
	struct cvg_ion_queue32 q;
	struct cvg_ion_queue queue;
	int ret;

	if (copy_from_user(&q, arg, sizeof(q))) {
		ERROR(cdev, "copy user fail\n");
		return -EFAULT;
	}

	queue.fd = q.fd;
	queue.vbase = compat_ptr(q.vbase);

	ret = _cvg_queue_ion(ps, &queue);

	return ret;
}

#endif

static long cvg_do_ioctl(struct file *file, unsigned int cmd,
				void __user *p)
{
	struct usb_dev_state *ps = file->private_data;
	struct inode *inode = file_inode(file);
	struct miscdevice *dev = NULL;
	struct f_cvg *cvg = NULL;
	int ret = -ENOTTY;

	if (!(file->f_mode & FMODE_WRITE))
		return -EPERM;

	if (!ps)
		return -ENODEV;

	cvg = ps->cvg;
	if (!cvg)
		return -ENODEV;

	dev = ps->cvg->dev;
	if (!dev)
		return -ENODEV;

	device_lock(dev->this_device);

	/* Reap operations are allowed even after disconnection */
	switch (cmd) {
	case CVG_IOCTL_REAPTXURB:
		dev_dbg(cvg->dev->this_device, "%s: REAPURB\n", __func__);
		ret = cvg_reaptxurbnonblock(ps, p);
		break;
	case CVG_IOCTL_REAPRXURB:
		dev_dbg(cvg->dev->this_device, "%s: REAPURB\n", __func__);
		ret = cvg_reaprxurbnonblock(ps, p);
		break;
	case CVG_IOCTL_SUBMITURB:
		dev_dbg(cvg->dev->this_device, "%s: SUBMITURB\n", __func__);
		ret = cvg_submiturb(ps, p);
		if (ret >= 0)
			inode->i_mtime = current_time(inode);
		break;
#ifdef CONFIG_COMPAT
	case CVG_IOCTL_REAPTXURB32:
		dev_dbg(cvg->dev->this_device, "%s: REAPURB32\n", __func__);
		ret = cvg_reaptxurbnonblock_compat(ps, p);
		break;
	case CVG_IOCTL_REAPRXURB32:
		dev_dbg(cvg->dev->this_device, "%s: REAPURB32\n", __func__);
		ret = cvg_reaprxurbnonblock_compat(ps, p);
		break;

	case CVG_IOCTL_SUBMITURB32:
		ret = cvg_submiturb_compat(ps, p);
		if (ret >= 0)
			inode->i_mtime = current_time(inode);
		break;
	case CVG_IOCTL_CONNSIG32:
		dev_dbg(cvg->dev->this_device, "%s: CONNSIG32\n", __func__);
		ret = proc_connectsignal_compat(ps, p);
		break;
	case CVG_IOCTL_QUEUEION32:
		dev_dbg(cvg->dev->this_device, "%s: QUEUE ION BUFFER32\n", __func__);
		ret = cvg_queue_ion32(ps, p);
		break;
#endif
	case CVG_IOCTL_DISCARDURB:
		dev_dbg(cvg->dev->this_device, "%s: DISCARDURB %pK\n",
			__func__, p);
		ret = cvg_unlinkurb(ps, p);
		break;
	case CVG_IOCTL_CONNSIG:
		dev_dbg(cvg->dev->this_device, "%s: CONNSIG\n", __func__);
		ret = proc_connectsignal(ps, p);
		break;
	case CVG_IOCTL_QUEUEION:
		dev_dbg(cvg->dev->this_device, "%s: QUEUE ION BUFFER\n", __func__);
		ret = cvg_queue_ion(ps, p);
		break;
	default:
		dev_dbg(cvg->dev->this_device, "%s: Unsupport ioctl %d\n",
			__func__, cmd);
		break;
	}
	device_unlock(dev->this_device);
	if (ret >= 0)
		inode->i_atime = current_time(inode);
	return ret;
}

static long cvg_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	int ret;

	ret = cvg_do_ioctl(file, cmd, (void __user *)arg);

	return ret;
}

static int cvg_open(struct inode *inode, struct file *file)
{
	struct f_cvg *cvg = cvg_gadget;
	struct usb_composite_dev *cdev;
	struct miscdevice *dev = NULL;
	struct usb_dev_state *ps;
	int ret;

	if (!cvg)
		return -ENODEV;

	cdev = cvg->function.config->cdev;
	ps = kzalloc(sizeof(struct usb_dev_state), GFP_KERNEL);
	if (!ps) {
		ret = -ENOMEM;
		goto out_free_ps;
	}

	dev = cvg->dev;
	if (!dev) {
		ret = -ENODEV;
		goto out_free_ps;
	}

	device_lock(dev->this_device);

	ps->cvg = cvg;
	ps->file = file;
	spin_lock_init(&ps->lock);
	INIT_LIST_HEAD(&ps->async_tx_pending);
	INIT_LIST_HEAD(&ps->async_tx_completed);
	INIT_LIST_HEAD(&ps->async_rx_pending);
	INIT_LIST_HEAD(&ps->async_rx_completed);
	INIT_LIST_HEAD(&ps->memory_list);
	INIT_LIST_HEAD(&ps->list);
	init_waitqueue_head(&ps->wait_tx);
	init_waitqueue_head(&ps->wait_rx);
	ps->conn_pid = get_pid(task_pid(current));
	ps->cred = get_current_cred();
	security_task_getsecid(current, &ps->secid);
	/* memory barrier in smp case. */
	smp_wmb();
	file->private_data = ps;
	list_add_tail(&ps->list, &cvg->ps_list);

	device_unlock(dev->this_device);
	dev_dbg(cvg->dev->this_device, "cvg open ok\n");
	return 0;

 out_free_ps:
	kfree(ps);
	return ret;
}

#ifdef CONFIG_COMPAT
static long cvg_compat_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	int ret;

	ret = cvg_do_ioctl(file, cmd, compat_ptr(arg));

	return ret;
}
#endif

static unsigned int cvg_poll(struct file *file,
				struct poll_table_struct *wait)
{
	struct usb_dev_state *ps = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &ps->wait_tx, wait);
	poll_wait(file, &ps->wait_rx, wait);
	if (file->f_mode & FMODE_WRITE && !list_empty(&ps->async_tx_completed))
		mask |= POLLOUT | POLLWRNORM;
	if (file->f_mode & FMODE_WRITE && !list_empty(&ps->async_rx_completed))
		mask |= POLLIN | POLLWRNORM;
	return mask;
}

static int cvg_release(struct inode *inode, struct file *file)
{
	struct usb_dev_state *ps = file->private_data;
	struct miscdevice *dev = ps->cvg->dev;
	struct async *as;

	device_lock(dev->this_device);

	list_del_init(&ps->list);
	destroy_all_async(ps);
	device_unlock(dev->this_device);
	put_pid(ps->conn_pid);
	put_cred(ps->cred);

	as = async_gettxcompleted(ps);
	while (as) {
		free_async(as);
		as = async_gettxcompleted(ps);
	}
	as = async_getrxcompleted(ps);
	while (as) {
		free_async(as);
		as = async_getrxcompleted(ps);
	}

	kfree(ps);
	return 0;
}

static const struct file_operations cvg_fops = {
	.owner = THIS_MODULE,
	.open = cvg_open,
	.poll = cvg_poll,
	.mmap = cvg_mmap,
	.unlocked_ioctl = cvg_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl =   cvg_compat_ioctl,
#endif
	.release = cvg_release,
};

static struct miscdevice cvg_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cvi_gadget",
	.fops = &cvg_fops
};

static int cvg_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_cvg	*cvg = func_to_cvg(f);
	int			id;
	int ret;

	/* register misc device */
	ret = misc_register(&cvg_device);
	if (ret)
		return ret;
	cvg->dev = &cvg_device;
#if defined(__aarch64__)
	arch_setup_dma_ops(cvg->dev->this_device, 0, DMA_BIT_MASK(64), NULL, false);
	dma_coerce_mask_and_coherent(cvg->dev->this_device, DMA_BIT_MASK(64));
#else
	dma_coerce_mask_and_coherent(cvg->dev->this_device, DMA_BIT_MASK(32));
#endif
	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	cvg_intf.bInterfaceNumber = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_cvg[0].id = id;
	cvg_intf.iInterface = id;

	/* allocate endpoints */

	cvg->in_ep = usb_ep_autoconfig(cdev->gadget, &fs_cvg_source_desc);
	if (!cvg->in_ep) {
autoconf_fail:
		ERROR(cdev, "%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}

	cvg->out_ep = usb_ep_autoconfig(cdev->gadget, &fs_cvg_sink_desc);
	if (!cvg->out_ep)
		goto autoconf_fail;

	/* support high speed hardware */
	hs_cvg_source_desc.bEndpointAddress =
		fs_cvg_source_desc.bEndpointAddress;
	hs_cvg_sink_desc.bEndpointAddress = fs_cvg_sink_desc.bEndpointAddress;

	/* support super speed hardware */
	ss_cvg_source_desc.bEndpointAddress =
		fs_cvg_source_desc.bEndpointAddress;
	ss_cvg_sink_desc.bEndpointAddress = fs_cvg_sink_desc.bEndpointAddress;

	ret = usb_assign_descriptors(f, fs_cvg_descs, hs_cvg_descs,
			ss_cvg_descs, NULL);
	if (ret)
		return ret;

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
	    (gadget_is_superspeed(c->cdev->gadget) ? "super" :
	     (gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full")),
			f->name, cvg->in_ep->name, cvg->out_ep->name);
	return 0;
}

static void cvg_free_func(struct usb_function *f)
{
	struct f_cvg_opts *opts;
	struct f_cvg *cvg = func_to_cvg(f);

	destroy_all_ps(cvg);

	opts = container_of(f->fi, struct f_cvg_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt--;
	mutex_unlock(&opts->lock);

	usb_free_all_descriptors(f);
	misc_deregister(&cvg_device);
	cvg_gadget = NULL;
	kfree(cvg->zlp_buf);
	kfree(cvg);
}

static void disable_cvg(struct f_cvg *cvg)
{
	struct usb_composite_dev	*cdev;
	int value;

	cdev = cvg->function.config->cdev;
	value = usb_ep_disable(cvg->in_ep);
	if (value < 0)
		VDBG(cdev, "disable %s --> %d\n", cvg->in_ep->name, value);
	value = usb_ep_disable(cvg->out_ep);
	if (value < 0)
		VDBG(cdev, "disable %s --> %d\n", cvg->out_ep->name, value);
	VDBG(cdev, "%s disabled\n", cvg->function.name);
}

static int enable_endpoint(struct usb_composite_dev *cdev,
			   struct f_cvg *cvg, struct usb_ep *ep)
{
	int					result;

	result = config_ep_by_speed(cdev->gadget, &(cvg->function), ep);
	if (result)
		goto out;

	result = usb_ep_enable(ep);
	if (result < 0)
		goto out;
	ep->driver_data = cvg;
	result = 0;

out:
	return result;
}

static int
enable_cvg(struct usb_composite_dev *cdev, struct f_cvg *cvg)
{
	int result = 0;

	result = enable_endpoint(cdev, cvg, cvg->in_ep);
	if (result)
		goto out;

	result = enable_endpoint(cdev, cvg, cvg->out_ep);
	if (result)
		goto disable_in;

	DBG(cdev, "%s enabled\n", cvg->function.name);
	return 0;

disable_in:
	usb_ep_disable(cvg->in_ep);
out:
	return result;
}

static int cvg_set_alt(struct usb_function *f,
		unsigned int intf, unsigned int alt)
{
	struct f_cvg	*cvg = func_to_cvg(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	cvg->connect = 1;
	signal_connect(cvg);
	/* we know alt is zero */
	disable_cvg(cvg);
	return enable_cvg(cdev, cvg);
}

static void cvg_disable(struct usb_function *f)
{
	struct f_cvg	*cvg = func_to_cvg(f);

	cvg->connect = 0;
	signal_connect(cvg);
	disable_cvg(cvg);
}

static struct usb_function *cvg_alloc(struct usb_function_instance *fi)
{
	struct f_cvg	*cvg;
	struct f_cvg_opts	*opts;

	cvg = kzalloc(sizeof(*cvg), GFP_KERNEL);
	if (!cvg)
		return ERR_PTR(-ENOMEM);

	cvg->zlp_buf = kzalloc(1024, GFP_KERNEL);
	if (!cvg->zlp_buf)
		return ERR_PTR(-ENOMEM);

	opts = container_of(fi, struct f_cvg_opts, func_inst);

	mutex_lock(&opts->lock);
	opts->refcnt++;
	mutex_unlock(&opts->lock);

	cvg->num_ep_in = opts->num_ep_in;
	cvg->num_ep_out = opts->num_ep_out;

	cvg->function.name = "cvg";
	cvg->function.bind = cvg_bind;
	cvg->function.set_alt = cvg_set_alt;
	cvg->function.disable = cvg_disable;
	cvg->function.strings = cvg_strings;
	cvg->function.free_func = cvg_free_func;
	spin_lock_init(&cvg->lock);
	INIT_LIST_HEAD(&cvg->ps_list);
	cvg_gadget = cvg;

	return &cvg->function;
}

static inline struct f_cvg_opts *to_f_cvg_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_cvg_opts,
			    func_inst.group);
}

static void cvg_attr_release(struct config_item *item)
{
	struct f_cvg_opts *opts = to_f_cvg_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations cvg_item_ops = {
	.release		= cvg_attr_release,
};

static ssize_t f_cvg_opts_epout_show(struct config_item *item, char *page)
{
	struct f_cvg_opts *opts = to_f_cvg_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf(page, "%d\n", opts->num_ep_out);
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_cvg_opts_epout_store(struct config_item *item,
				    const char *page, size_t len)
{
	struct f_cvg_opts *opts = to_f_cvg_opts(item);
	int ret;
	u32 num;

	mutex_lock(&opts->lock);
	if (opts->refcnt) {
		ret = -EBUSY;
		goto end;
	}

	ret = kstrtou32(page, 0, &num);
	if (ret)
		goto end;

	opts->num_ep_out = num;
	ret = len;
end:
	mutex_unlock(&opts->lock);
	return ret;
}

CONFIGFS_ATTR(f_cvg_opts_, epout);

static ssize_t f_cvg_opts_epin_show(struct config_item *item, char *page)
{
	struct f_cvg_opts *opts = to_f_cvg_opts(item);
	int result;

	mutex_lock(&opts->lock);
	result = sprintf(page, "%d\n", opts->num_ep_in);
	mutex_unlock(&opts->lock);

	return result;
}

static ssize_t f_cvg_opts_epin_store(struct config_item *item,
				    const char *page, size_t len)
{
	struct f_cvg_opts *opts = to_f_cvg_opts(item);
	int ret;
	u32 num;

	mutex_lock(&opts->lock);
	if (opts->refcnt) {
		ret = -EBUSY;
		goto end;
	}

	ret = kstrtou32(page, 0, &num);
	if (ret)
		goto end;

	opts->num_ep_in = num;
	ret = len;
end:
	mutex_unlock(&opts->lock);
	return ret;
}

CONFIGFS_ATTR(f_cvg_opts_, epin);

static struct configfs_attribute *cvg_attrs[] = {
	&f_cvg_opts_attr_epout,
	&f_cvg_opts_attr_epin,
	NULL,
};

static struct config_item_type cvg_func_type = {
	.ct_item_ops    = &cvg_item_ops,
	.ct_attrs	= cvg_attrs,
	.ct_owner       = THIS_MODULE,
};

static void cvg_free_instance(struct usb_function_instance *fi)
{
	struct f_cvg_opts *opts;

	opts = container_of(fi, struct f_cvg_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *cvg_alloc_instance(void)
{
	struct f_cvg_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = cvg_free_instance;
	opts->num_ep_in = CVG_DEF_IN_EP_NUM;
	opts->num_ep_out = CVG_DEF_OUT_EP_NUM;

	config_group_init_type_name(&opts->func_inst.group, "",
				    &cvg_func_type);

	return  &opts->func_inst;
}
DECLARE_USB_FUNCTION_INIT(cvg, cvg_alloc_instance, cvg_alloc);

MODULE_LICENSE("GPL");
