/*
 * drivers/usb/sunxi_usb/udc/usb3/gadget.h
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller gadget driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DRIVERS_USB_SUNXI_GADGET_H
#define __DRIVERS_USB_SUNXI_GADGET_H

#include <linux/list.h>
#include <linux/usb/gadget.h>
#include "io.h"

struct sunxi_otgc;
#define to_sunxi_ep(ep)		(container_of(ep, struct sunxi_otgc_ep, endpoint))
#define gadget_to_otgc(g)	(container_of(g, struct sunxi_otgc, gadget))

struct sunxi_otgc_gadget_ep_cmd_params {
	u32	param2;
	u32	param1;
	u32	param0;
};

/* -------------------------------------------------------------------------- */

#define to_sunxi_request(r)	(container_of(r, struct sunxi_otgc_request, request))

static inline struct sunxi_otgc_request *next_request(struct list_head *list)
{
	if (list_empty(list))
		return NULL;

	return list_first_entry(list, struct sunxi_otgc_request, list);
}

static inline void sunxi_gadget_move_request_queued(struct sunxi_otgc_request *req)
{
	struct sunxi_otgc_ep		*dep = req->dep;

	req->queued = true;
	list_move_tail(&req->list, &dep->req_queued);
}

void sunxi_gadget_giveback(struct sunxi_otgc_ep *dep, struct sunxi_otgc_request *req,
		int status);
void sunxi_ep0_interrupt(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event);
void sunxi_ep0_out_start(struct sunxi_otgc *otgc);
int sunxi_gadget_ep0_queue(struct usb_ep *ep, struct usb_request *request,
		gfp_t gfp_flags);
int __sunxi_gadget_ep_set_halt(struct sunxi_otgc_ep *dep, int value);
int sunxi_send_gadget_ep_cmd(struct sunxi_otgc *otgc, unsigned ep,
		unsigned cmd, struct sunxi_otgc_gadget_ep_cmd_params *params);


#endif /* __DRIVERS_USB_SUNXI_GADGET_H */
