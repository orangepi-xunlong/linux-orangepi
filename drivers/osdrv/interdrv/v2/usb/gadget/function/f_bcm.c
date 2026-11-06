/*
 * f_acm.c -- USB CDC serial (ACM) function driver
 *
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 by David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2009 by Samsung Electronics
 * Author: Michal Nazarewicz (mina86@mina86.com)
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/poll.h>

#include "bm_serial.h"


/*
 * This CDC ACM function support just wraps control functions and
 * notifications around the generic serial-over-usb code.
 *
 * Because CDC ACM is standardized by the USB-IF, many host operating
 * systems have drivers for it.  Accordingly, ACM is the preferred
 * interop solution for serial-port type connections.  The control
 * models are often not necessary, and in any case don't do much in
 * this bare-bones implementation.
 *
 * Note that even MS-Windows has some support for ACM.  However, that
 * support is somewhat broken because when you use ACM in a composite
 * device, having multiple interfaces confuses the poor OS.  It doesn't
 * seem to understand CDC Union descriptors.  The new "association"
 * descriptors (roughly equivalent to CDC Unions) may sometimes help.
 */

struct f_bcm {
	struct bmserial			port;
	u8				data_id;
	u8				port_num;

	u8				pending;

	/* lock is mostly for pending and notify_req ... they get accessed
	 * by callbacks both from tty (open/close/break) under its spinlock,
	 * and notify_req.complete() which can't use that lock.
	 */
	spinlock_t			lock;

	struct usb_cdc_line_coding	port_line_coding;	/* 8-N-1 etc */

	u16				port_handshake_bits;

	u16				serial_state;
};

static inline struct f_bcm *func_to_bcm(struct usb_function *f)
{
	return container_of(f, struct f_bcm, port.func);
}

static inline struct f_bcm *port_to_bcm(struct bmserial *p)
{
	return container_of(p, struct f_bcm, port);
}

/*-------------------------------------------------------------------------*/

/* notification endpoint uses smallish and infrequent fixed-size messages */

#define GS_NOTIFY_INTERVAL_MS		32
#define GS_NOTIFY_MAXPACKET		10	/* notification + 2 bytes */

/* interface and class descriptors: */

static struct usb_interface_descriptor bcm_data_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass =	USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor bcm_fs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor bcm_fs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *bcm_fs_function[] = {
	(struct usb_descriptor_header *) &bcm_data_interface_desc,
	(struct usb_descriptor_header *) &bcm_fs_in_desc,
	(struct usb_descriptor_header *) &bcm_fs_out_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor bcm_hs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor bcm_hs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_descriptor_header *bcm_hs_function[] = {
	(struct usb_descriptor_header *) &bcm_data_interface_desc,
	(struct usb_descriptor_header *) &bcm_hs_in_desc,
	(struct usb_descriptor_header *) &bcm_hs_out_desc,
	NULL,
};

static struct usb_endpoint_descriptor bcm_ss_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor bcm_ss_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor bcm_ss_bulk_comp_desc = {
	.bLength =              sizeof(bcm_ss_bulk_comp_desc),
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,
	.bMaxBurst       =      3,
};

static struct usb_descriptor_header *bcm_ss_function[] = {
	(struct usb_descriptor_header *) &bcm_data_interface_desc,
	(struct usb_descriptor_header *) &bcm_ss_in_desc,
	(struct usb_descriptor_header *) &bcm_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &bcm_ss_out_desc,
	(struct usb_descriptor_header *) &bcm_ss_bulk_comp_desc,
	NULL,
};

/* string descriptors: */
#define BCM_DATA_IDX	0

/* static strings, in UTF-8 */
static struct usb_string bcm_string_defs[] = {
	[BCM_DATA_IDX].s = "Generic Serial",
	{  } /* end of list */
};

static struct usb_gadget_strings bcm_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		bcm_string_defs,
};

static struct usb_gadget_strings *bcm_strings[] = {
	&bcm_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/

/* ACM control ... data handling is delegated to tty library code.
 * The main task of this function is to activate and deactivate
 * that code based on device state; track parameters like line
 * speed, handshake state, and so on; and issue notifications.
 */

static void bcm_complete_set_line_coding(struct usb_ep *ep,
		struct usb_request *req)
{
	struct f_bcm	*bcm = ep->driver_data;
	struct usb_composite_dev *cdev = bcm->port.func.config->cdev;

	if (req->status != 0) {
		dev_dbg(&cdev->gadget->dev, "bcm ttyGS%d completion, err %d\n",
			bcm->port_num, req->status);
		return;
	}

	/* normal completion */
	if (req->actual != sizeof(bcm->port_line_coding)) {
		dev_dbg(&cdev->gadget->dev, "bcm ttyGS%d short resp, len %d\n",
			bcm->port_num, req->actual);
		usb_ep_set_halt(ep);
	} else {
		struct usb_cdc_line_coding	*value = req->buf;

		/* REVISIT:  we currently just remember this data.
		 * If we change that, (a) validate it first, then
		 * (b) update whatever hardware needs updating,
		 * (c) worry about locking.  This is information on
		 * the order of 9600-8-N-1 ... most of which means
		 * nothing unless we control a real RS232 line.
		 */
		bcm->port_line_coding = *value;
	}
}

static int bcm_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_bcm		*bcm = func_to_bcm(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 *
	 * Note CDC spec table 4 lists the ACM request profile.  It requires
	 * encapsulated command support ... we don't handle any, and respond
	 * to them by stalling.  Options include get/set/clear comm features
	 * (not that useful) and SEND_BREAK.
	 */
	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {

	/* SET_LINE_CODING ... just read and save what the host sends */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_LINE_CODING:
		if (w_length != sizeof(struct usb_cdc_line_coding))
			goto invalid;

		value = w_length;
		cdev->gadget->ep0->driver_data = bcm;
		req->complete = bcm_complete_set_line_coding;
		break;

	/* GET_LINE_CODING ... return what host sent, or initial value */
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_GET_LINE_CODING:

		value = min_t(unsigned int, w_length,
				sizeof(struct usb_cdc_line_coding));
		memcpy(req->buf, &bcm->port_line_coding, value);
		break;

	/* SET_CONTROL_LINE_STATE ... save what the host sent */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_CONTROL_LINE_STATE:

		value = 0;

		/* FIXME we should not allow data to flow until the
		 * host sets the ACM_CTRL_DTR bit; and when it clears
		 * that bit, we should return to that no-flow state.
		 */
		bcm->port_handshake_bits = w_value;
		break;

	default:
invalid:
		dev_vdbg(&cdev->gadget->dev,
			 "invalid control req%02x.%02x v%04x i%04x l%d\n",
			 ctrl->bRequestType, ctrl->bRequest,
			 w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		dev_dbg(&cdev->gadget->dev,
			"bcm ttyGS%d req%02x.%02x v%04x i%04x l%d\n",
			bcm->port_num, ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "bcm response on ttyGS%d, err %d\n",
					bcm->port_num, value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static int bcm_set_alt(struct usb_function *f, unsigned int intf, unsigned int alt)
{
	struct f_bcm		*bcm = func_to_bcm(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	/* we know alt == 0, so this is an activation or a reset */

		if (bcm->port.in->enabled)
			bmserial_disconnect(&bcm->port);

		if (!bcm->port.in->desc || !bcm->port.out->desc) {
			if (config_ep_by_speed(cdev->gadget, f,
					       bcm->port.in) ||
			    config_ep_by_speed(cdev->gadget, f,
					       bcm->port.out)) {
				bcm->port.in->desc = NULL;
				bcm->port.out->desc = NULL;
				return -EINVAL;
			}
		}
		bmserial_connect(&bcm->port, bcm->port_num);


	return 0;
}

static void bcm_disable(struct usb_function *f)
{
	struct f_bcm	*bcm = func_to_bcm(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	dev_dbg(&cdev->gadget->dev, "bcm ttyGS%d deactivated\n", bcm->port_num);
	bmserial_disconnect(&bcm->port);
}

/*-------------------------------------------------------------------------*/

/* connect == the TTY link is open */

static void bcm_connect(struct bmserial *port)
{

}

static void bcm_disconnect(struct bmserial *port)
{

}

/*-------------------------------------------------------------------------*/

/* ACM function driver setup/binding */
static int
bcm_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_bcm		*bcm = func_to_bcm(f);
	struct usb_string	*us;
	int			status;
	struct usb_ep		*ep;

	/* REVISIT might want instance-specific strings to help
	 * distinguish instances ...
	 */

	/* maybe allocate device-global string IDs, and patch descriptors */
	us = usb_gstrings_attach(cdev, bcm_strings,
			ARRAY_SIZE(bcm_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);

	bcm_data_interface_desc.iInterface = us[BCM_DATA_IDX].id;

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	bcm->data_id = status;

	bcm_data_interface_desc.bInterfaceNumber = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &bcm_fs_in_desc);
	if (!ep)
		goto fail;
	bcm->port.in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &bcm_fs_out_desc);
	if (!ep)
		goto fail;
	bcm->port.out = ep;

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	bcm_hs_in_desc.bEndpointAddress = bcm_fs_in_desc.bEndpointAddress;
	bcm_hs_out_desc.bEndpointAddress = bcm_fs_out_desc.bEndpointAddress;

	bcm_ss_in_desc.bEndpointAddress = bcm_fs_in_desc.bEndpointAddress;
	bcm_ss_out_desc.bEndpointAddress = bcm_fs_out_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, bcm_fs_function, bcm_hs_function,
			bcm_ss_function, NULL);
	if (status)
		goto fail;

	dev_dbg(&cdev->gadget->dev,
		"bcm ttyGS%d: %s speed IN/%s OUT/%s\n",
		bcm->port_num,
		gadget_is_superspeed(c->cdev->gadget) ? "super" :
		gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
		bcm->port.in->name, bcm->port.out->name);
	return 0;

fail:
	ERROR(cdev, "%s/%p: can't bind, err %d\n", f->name, f, status);

	return status;
}

static void bcm_unbind(struct usb_configuration *c, struct usb_function *f)
{
	bcm_string_defs[0].id = 0;
	usb_free_all_descriptors(f);
}

static void bcm_free_func(struct usb_function *f)
{
	struct f_bcm		*bcm = func_to_bcm(f);

	kfree(bcm);
}

static struct usb_function *bcm_alloc_func(struct usb_function_instance *fi)
{
	struct bm_serial_opts *opts;
	struct f_bcm *bcm;

	bcm = kzalloc(sizeof(*bcm), GFP_KERNEL);
	if (!bcm)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&bcm->lock);

	bcm->port.connect = bcm_connect;
	bcm->port.disconnect = bcm_disconnect;

	bcm->port.func.name = "bcm";
	bcm->port.func.strings = bcm_strings;
	/* descriptors are per-instance copies */
	bcm->port.func.bind = bcm_bind;
	bcm->port.func.set_alt = bcm_set_alt;
	bcm->port.func.setup = bcm_setup;
	bcm->port.func.disable = bcm_disable;

	opts = container_of(fi, struct bm_serial_opts, func_inst);
	bcm->port_num = opts->port_num;
	bcm->port.func.unbind = bcm_unbind;
	bcm->port.func.free_func = bcm_free_func;

	return &bcm->port.func;
}

static inline struct bm_serial_opts *to_bm_serial_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct bm_serial_opts,
			func_inst.group);
}

static void bcm_attr_release(struct config_item *item)
{
	struct bm_serial_opts *opts = to_bm_serial_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations bcm_item_ops = {
	.release                = bcm_attr_release,
};

static ssize_t f_bcm_port_num_show(struct config_item *item, char *page)
{
	return sprintf(page, "%u\n", to_bm_serial_opts(item)->port_num);
}

CONFIGFS_ATTR_RO(f_bcm_, port_num);

static struct configfs_attribute *bcm_attrs[] = {
	&f_bcm_attr_port_num,
	NULL,
};

static struct config_item_type bcm_func_type = {
	.ct_item_ops    = &bcm_item_ops,
	.ct_attrs	= bcm_attrs,
	.ct_owner       = THIS_MODULE,
};

static void bcm_free_instance(struct usb_function_instance *fi)
{
	struct bm_serial_opts *opts;

	opts = container_of(fi, struct bm_serial_opts, func_inst);
	bmserial_free_line(opts->port_num);
	kfree(opts);
}

static struct usb_function_instance *bcm_alloc_instance(void)
{
	struct bm_serial_opts *opts;
	int ret;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	opts->func_inst.free_func_inst = bcm_free_instance;
	ret = bmserial_alloc_line(&opts->port_num);
	if (ret) {
		kfree(opts);
		return ERR_PTR(ret);
	}
	config_group_init_type_name(&opts->func_inst.group, "",
			&bcm_func_type);
	return &opts->func_inst;
}
DECLARE_USB_FUNCTION_INIT(bcm, bcm_alloc_instance, bcm_alloc_func);
MODULE_LICENSE("GPL");
