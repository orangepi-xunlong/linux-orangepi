/*
 * drivers/usb/sunxi_usb/udc/usb3/ep0.c
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller ep0
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/list.h>
#include <linux/dma-mapping.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include "core.h"
#include "gadget.h"
#include "io.h"
#include "osal.h"

static void sunxi_ep0_do_control_status(struct sunxi_otgc *otgc, u32 epnum);

static const char *sunxi_ep0_state_string(enum sunxi_ep0_state state)
{
	switch (state) {
	case EP0_UNCONNECTED:
		return "Unconnected";
	case EP0_SETUP_PHASE:
		return "Setup Phase";
	case EP0_DATA_PHASE:
		return "Data Phase";
	case EP0_STATUS_PHASE:
		return "Status Phase";
	default:
		return "UNKNOWN";
	}
}

static int sunxi_ep0_start_trans(struct sunxi_otgc *otgc, u8 epnum, dma_addr_t buf_dma,
		u32 len, u32 type)
{
	struct sunxi_otgc_gadget_ep_cmd_params params;
	struct sunxi_otgc_trb			*trb;
	struct sunxi_otgc_ep			*dep;

	int				ret;

	dep = otgc->eps[epnum];
	if (dep->flags & SUNXI_EP_BUSY) {
		dev_vdbg(otgc->dev, "%s: still busy\n", dep->name);
		return 0;
	}

	trb = otgc->ep0_trb;

	trb->bpl = lower_32_bits(buf_dma);
	trb->bph = upper_32_bits(buf_dma);
	trb->size = len;
	trb->ctrl = type;

	trb->ctrl |= (SUNXI_TRB_CTRL_HWO
			| SUNXI_TRB_CTRL_LST
			| SUNXI_TRB_CTRL_IOC
			| SUNXI_TRB_CTRL_ISP_IMI);

	memset(&params, 0, sizeof(params));
	params.param0 = upper_32_bits(otgc->ep0_trb_addr);
	params.param1 = lower_32_bits(otgc->ep0_trb_addr);

	ret = sunxi_send_gadget_ep_cmd(otgc, dep->number,
			SUNXI_DEPCMD_STARTTRANSFER, &params);
	if (ret < 0) {
		dev_dbg(otgc->dev, "failed to send STARTTRANSFER command\n");
		return ret;
	}

	dep->flags |= SUNXI_EP_BUSY;
	dep->res_trans_idx = sunxi_gadget_ep_get_transfer_index(otgc->regs,
			dep->number);

	otgc->ep0_next_event = SUNXI_EP0_COMPLETE;

	return 0;
}

static int __sunxi_gadget_ep0_queue(struct sunxi_otgc_ep *dep,
		struct sunxi_otgc_request *req)
{
	struct sunxi_otgc		*otgc = dep->otgc;
	int			ret = 0;

	req->request.actual	= 0;
	req->request.status	= -EINPROGRESS;
	req->epnum		= dep->number;

	list_add_tail(&req->list, &dep->request_list);

	if (dep->flags & SUNXI_EP_PENDING_REQUEST) {
		unsigned	direction;

		direction = !!(dep->flags & SUNXI_EP0_DIR_IN);

		if (otgc->ep0state != EP0_DATA_PHASE) {
			dev_WARN(otgc->dev, "Unexpected pending request\n");
			return 0;
		}

		ret = sunxi_ep0_start_trans(otgc, direction,
				req->request.dma, req->request.length,
				SUNXI_TRBCTL_CONTROL_DATA);
		dep->flags &= ~(SUNXI_EP_PENDING_REQUEST |
				SUNXI_EP0_DIR_IN);
	} else if (otgc->delayed_status) {
		otgc->delayed_status = false;

		if (otgc->ep0state == EP0_STATUS_PHASE)
			sunxi_ep0_do_control_status(otgc, 1);
		else
			dev_dbg(otgc->dev, "too early for delayed status\n");
	}

	return ret;
}

int sunxi_gadget_ep0_queue(struct usb_ep *ep, struct usb_request *request,
		gfp_t gfp_flags)
{
	struct sunxi_otgc_request		*req = to_sunxi_request(request);
	struct sunxi_otgc_ep			*dep = to_sunxi_ep(ep);
	struct sunxi_otgc			*otgc = dep->otgc;

	unsigned long			flags;

	int				ret;

	spin_lock_irqsave(&otgc->lock, flags);
	if (!dep->desc) {
		dev_dbg(otgc->dev, "trying to queue request %p to disabled %s\n",
				request, dep->name);
		ret = -ESHUTDOWN;
		goto out;
	}

	/* we share one TRB for ep0/1 */
	if (!list_empty(&dep->request_list)) {
		ret = -EBUSY;
		goto out;
	}

	dev_vdbg(otgc->dev, "queueing request %p to %s length %d, state '%s'\n",
			request, dep->name, request->length,
			sunxi_ep0_state_string(otgc->ep0state));

	ret = __sunxi_gadget_ep0_queue(dep, req);

out:
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static void sunxi_ep0_stall_and_restart(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_ep		*dep = otgc->eps[0];

	/* stall is always issued on EP0 */
	__sunxi_gadget_ep_set_halt(dep, 1);
	dep->flags = SUNXI_EP_ENABLED;
	otgc->delayed_status = false;

	if (!list_empty(&dep->request_list)) {
		struct sunxi_otgc_request	*req;

		req = next_request(&dep->request_list);
		sunxi_gadget_giveback(dep, req, -ECONNRESET);
	}

	otgc->ep0state = EP0_SETUP_PHASE;
	sunxi_ep0_out_start(otgc);
}

void sunxi_ep0_out_start(struct sunxi_otgc *otgc)
{
	int				ret;

	ret = sunxi_ep0_start_trans(otgc, 0, otgc->ctrl_req_addr, 8,
			SUNXI_TRBCTL_CONTROL_SETUP);
	WARN_ON(ret < 0);
}

static struct sunxi_otgc_ep *sunxi_wIndex_to_dep(struct sunxi_otgc *otgc, __le16 wIndex_le)
{
	struct sunxi_otgc_ep		*dep;
	u32			windex = le16_to_cpu(wIndex_le);
	u32			epnum;

	epnum = (windex & USB_ENDPOINT_NUMBER_MASK) << 1;
	if ((windex & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN)
		epnum |= 1;

	dep = otgc->eps[epnum];
	if (dep->flags & SUNXI_EP_ENABLED)
		return dep;

	return NULL;
}

static void sunxi_ep0_status_cmpl(struct usb_ep *ep, struct usb_request *req)
{
}
/*
 * ch 9.4.5
 */
static int sunxi_ep0_handle_status(struct sunxi_otgc *otgc,
		struct usb_ctrlrequest *ctrl)
{
	struct sunxi_otgc_ep		*dep;
	u32			recip;
	u16			usb_status = 0;
	__le16			*response_pkt;

	recip = ctrl->bRequestType & USB_RECIP_MASK;
	switch (recip) {
	case USB_RECIP_DEVICE:
		/*
		 * We are self-powered. U1/U2/LTM will be set later
		 * once we handle this states. RemoteWakeup is 0 on SS
		 */
		usb_status |= otgc->is_selfpowered << USB_DEVICE_SELF_POWERED;
		break;

	case USB_RECIP_INTERFACE:
		/*
		 * Function Remote Wake Capable	D0
		 * Function Remote Wakeup	D1
		 */
		break;

	case USB_RECIP_ENDPOINT:
		dep = sunxi_wIndex_to_dep(otgc, ctrl->wIndex);
		if (!dep)
			return -EINVAL;

		if (dep->flags & SUNXI_EP_STALL)
			usb_status = 1 << USB_ENDPOINT_HALT;
		break;
	default:
		return -EINVAL;
	};

	response_pkt = (__le16 *) otgc->setup_buf;
	*response_pkt = cpu_to_le16(usb_status);

	dep = otgc->eps[0];
	otgc->ep0_usb_req.dep = dep;
	otgc->ep0_usb_req.request.length = sizeof(*response_pkt);
	otgc->ep0_usb_req.request.buf = otgc->setup_buf;
	otgc->ep0_usb_req.request.complete = sunxi_ep0_status_cmpl;

	return __sunxi_gadget_ep0_queue(dep, &otgc->ep0_usb_req);
}

static int sunxi_ep0_handle_feature(struct sunxi_otgc *otgc,
		struct usb_ctrlrequest *ctrl, int set)
{
	struct sunxi_otgc_ep		*dep;
	u32			recip;
	u32			wValue;
	u32			wIndex;
	int			ret;

	wValue = le16_to_cpu(ctrl->wValue);
	wIndex = le16_to_cpu(ctrl->wIndex);
	recip = ctrl->bRequestType & USB_RECIP_MASK;
	switch (recip) {
	case USB_RECIP_DEVICE:

		/*
		 * 9.4.1 says only only for SS, in AddressState only for
		 * default control pipe
		 */
		switch (wValue) {
		case USB_DEVICE_U1_ENABLE:
		case USB_DEVICE_U2_ENABLE:
		case USB_DEVICE_LTM_ENABLE:
			if (otgc->dev_state != SUNXI_CONFIGURED_STATE)
				return -EINVAL;
			if (otgc->speed != SUNXI_DSTS_SUPERSPEED)
				return -EINVAL;
		}

		/* XXX add U[12] & LTM */
		switch (wValue) {
		case USB_DEVICE_REMOTE_WAKEUP:
			break;
		case USB_DEVICE_U1_ENABLE:
			break;
		case USB_DEVICE_U2_ENABLE:
			break;
		case USB_DEVICE_LTM_ENABLE:
			break;

		case USB_DEVICE_TEST_MODE:
			if ((wIndex & 0xff) != 0)
				return -EINVAL;
			if (!set)
				return -EINVAL;

			otgc->test_mode_nr = wIndex >> 8;
			otgc->test_mode = true;
			break;
		default:
			return -EINVAL;
		}
		break;

	case USB_RECIP_INTERFACE:
		switch (wValue) {
		case USB_INTRF_FUNC_SUSPEND:
			if (wIndex & USB_INTRF_FUNC_SUSPEND_LP)
				/* XXX enable Low power suspend */
				;
			if (wIndex & USB_INTRF_FUNC_SUSPEND_RW)
				/* XXX enable remote wakeup */
				;
			break;
		default:
			return -EINVAL;
		}
		break;

	case USB_RECIP_ENDPOINT:
		switch (wValue) {
		case USB_ENDPOINT_HALT:
			dep = sunxi_wIndex_to_dep(otgc, wIndex);
			if (!dep)
				return -EINVAL;
			ret = __sunxi_gadget_ep_set_halt(dep, set);
			if (ret)
				return -EINVAL;
			break;
		default:
			return -EINVAL;
		}
		break;

	default:
		return -EINVAL;
	};

	return 0;
}

static int sunxi_ep0_set_address(struct sunxi_otgc *otgc, struct usb_ctrlrequest *ctrl)
{
	u32 addr;

	addr = le16_to_cpu(ctrl->wValue);
	if (addr > 127) {
		dev_dbg(otgc->dev, "invalid device address %d\n", addr);
		return -EINVAL;
	}

	if (otgc->dev_state == SUNXI_CONFIGURED_STATE) {
		dev_dbg(otgc->dev, "trying to set address when configured\n");
		return -EINVAL;
	}

	sunxi_set_dcfg_devaddr(otgc->regs, addr);

	if (addr)
		otgc->dev_state = SUNXI_ADDRESS_STATE;
	else
		otgc->dev_state = SUNXI_DEFAULT_STATE;

	return 0;
}

static int sunxi_ep0_delegate_req(struct sunxi_otgc *otgc, struct usb_ctrlrequest *ctrl)
{
	int ret;

	spin_unlock(&otgc->lock);
	ret = otgc->gadget_driver->setup(&otgc->gadget, ctrl);
	spin_lock(&otgc->lock);
	return ret;
}

static int sunxi_ep0_set_config(struct sunxi_otgc *otgc, struct usb_ctrlrequest *ctrl)
{
	u32 cfg;
	int ret;

	otgc->start_config_issued = false;
	cfg = le16_to_cpu(ctrl->wValue);

	switch (otgc->dev_state) {
	case SUNXI_DEFAULT_STATE:
		return -EINVAL;
		break;

	case SUNXI_ADDRESS_STATE:
		ret = sunxi_ep0_delegate_req(otgc, ctrl);
		/* if the cfg matches and the cfg is non zero */
		if (cfg && (!ret || (ret == USB_GADGET_DELAYED_STATUS))) {
			otgc->dev_state = SUNXI_CONFIGURED_STATE;
			otgc->resize_fifos = true;
			dev_dbg(otgc->dev, "resize fifos flag SET\n");
		}
		break;

	case SUNXI_CONFIGURED_STATE:
		ret = sunxi_ep0_delegate_req(otgc, ctrl);
		if (!cfg)
			otgc->dev_state = SUNXI_ADDRESS_STATE;
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int sunxi_ep0_std_request(struct sunxi_otgc *otgc, struct usb_ctrlrequest *ctrl)
{
	int ret;

	switch (ctrl->bRequest) {
	case USB_REQ_GET_STATUS:
		dev_vdbg(otgc->dev, "USB_REQ_GET_STATUS\n");
		ret = sunxi_ep0_handle_status(otgc, ctrl);
		break;
	case USB_REQ_CLEAR_FEATURE:
		dev_vdbg(otgc->dev, "USB_REQ_CLEAR_FEATURE\n");
		ret = sunxi_ep0_handle_feature(otgc, ctrl, 0);
		break;
	case USB_REQ_SET_FEATURE:
		dev_vdbg(otgc->dev, "USB_REQ_SET_FEATURE\n");
		ret = sunxi_ep0_handle_feature(otgc, ctrl, 1);
		break;
	case USB_REQ_SET_ADDRESS:
		dev_vdbg(otgc->dev, "USB_REQ_SET_ADDRESS\n");
		ret = sunxi_ep0_set_address(otgc, ctrl);
		break;
	case USB_REQ_SET_CONFIGURATION:
		dev_vdbg(otgc->dev, "USB_REQ_SET_CONFIGURATION\n");
		ret = sunxi_ep0_set_config(otgc, ctrl);
		break;
	default:
		dev_vdbg(otgc->dev, "Forwarding to gadget driver\n");
		ret = sunxi_ep0_delegate_req(otgc, ctrl);
		break;
	};

	return ret;
}

static void sunxi_ep0_inspect_setup(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	struct usb_ctrlrequest *ctrl = otgc->ctrl_req;
	int ret;
	u32 len;

	if (!otgc->gadget_driver)
		goto err;

	len = le16_to_cpu(ctrl->wLength);
	if (!len) {
		otgc->three_stage_setup = false;
		otgc->ep0_expect_in = false;
		otgc->ep0_next_event = SUNXI_EP0_NRDY_STATUS;
	} else {
		otgc->three_stage_setup = true;
		otgc->ep0_expect_in = !!(ctrl->bRequestType & USB_DIR_IN);
		otgc->ep0_next_event = SUNXI_EP0_NRDY_DATA;
	}

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		ret = sunxi_ep0_std_request(otgc, ctrl);
	else
		ret = sunxi_ep0_delegate_req(otgc, ctrl);

	if (ret == USB_GADGET_DELAYED_STATUS)
		otgc->delayed_status = true;

	if (ret >= 0)
		return;

err:
	sunxi_ep0_stall_and_restart(otgc);
}

static void sunxi_ep0_complete_data(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	struct sunxi_otgc_request	*r = NULL;
	struct usb_request	*ur;
	struct sunxi_otgc_trb		*trb;
	struct sunxi_otgc_ep		*ep0;
	u32			transferred;
	u32			length;
	u8			epnum;

	epnum = event->endpoint_number;
	ep0 = otgc->eps[0];

	otgc->ep0_next_event = SUNXI_EP0_NRDY_STATUS;

	r = next_request(&ep0->request_list);
	ur = &r->request;

	trb = otgc->ep0_trb;
	length = trb->size & SUNXI_TRB_SIZE_MASK;

	if (otgc->ep0_bounced) {
		unsigned transfer_size = ur->length;
		unsigned maxp = ep0->endpoint.maxpacket;

		transfer_size += (maxp - (transfer_size % maxp));
		transferred = min_t(u32, ur->length,
				transfer_size - length);
		memcpy(ur->buf, otgc->ep0_bounce, transferred);
	} else {
		transferred = ur->length - length;
	}

	ur->actual += transferred;

	if ((epnum & 1) && ur->actual < ur->length) {
		/* for some reason we did not get everything out */

		sunxi_ep0_stall_and_restart(otgc);
	} else {
		/*
		 * handle the case where we have to send a zero packet. This
		 * seems to be case when req.length > maxpacket. Could it be?
		 */
		if (r)
			sunxi_gadget_giveback(ep0, r, 0);
	}
}

static void sunxi_ep0_complete_req(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	struct sunxi_otgc_request	*r;
	struct sunxi_otgc_ep		*dep;

	dep = otgc->eps[0];

	if (!list_empty(&dep->request_list)) {
		r = next_request(&dep->request_list);

		sunxi_gadget_giveback(dep, r, 0);
	}

	if (otgc->test_mode) {
		int ret;

		ret = sunxi_gadget_set_test_mode(otgc->regs, otgc->test_mode_nr);
		if (ret < 0) {
			dev_dbg(otgc->dev, "Invalid Test #%d\n",
					otgc->test_mode_nr);
			sunxi_ep0_stall_and_restart(otgc);
		}
	}

	otgc->ep0state = EP0_SETUP_PHASE;
	sunxi_ep0_out_start(otgc);
}

static void sunxi_ep0_xfer_complete(struct sunxi_otgc *otgc,
			const struct sunxi_otgc_event_depevt *event)
{
	struct sunxi_otgc_ep		*dep = otgc->eps[event->endpoint_number];

	dep->flags &= ~SUNXI_EP_BUSY;
	dep->res_trans_idx = 0;
	otgc->setup_packet_pending = false;

	switch (otgc->ep0state) {
	case EP0_SETUP_PHASE:
		dev_vdbg(otgc->dev, "Inspecting Setup Bytes\n");
		sunxi_ep0_inspect_setup(otgc, event);
		break;

	case EP0_DATA_PHASE:
		dev_vdbg(otgc->dev, "Data Phase\n");
		sunxi_ep0_complete_data(otgc, event);
		break;

	case EP0_STATUS_PHASE:
		dev_vdbg(otgc->dev, "Status Phase\n");
		sunxi_ep0_complete_req(otgc, event);
		break;
	default:
		WARN(true, "UNKNOWN ep0state %d\n", otgc->ep0state);
	}
}

static void sunxi_ep0_do_control_setup(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	sunxi_ep0_out_start(otgc);
}

static void sunxi_ep0_do_control_data(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	struct sunxi_otgc_ep		*dep;
	struct sunxi_otgc_request	*req;
	int			ret;

	dep = otgc->eps[0];

	if (list_empty(&dep->request_list)) {
		dev_vdbg(otgc->dev, "pending request for EP0 Data phase\n");
		dep->flags |= SUNXI_EP_PENDING_REQUEST;

		if (event->endpoint_number)
			dep->flags |= SUNXI_EP0_DIR_IN;
		return;
	}

	req = next_request(&dep->request_list);
	req->direction = !!event->endpoint_number;

	if (req->request.length == 0) {
		ret = sunxi_ep0_start_trans(otgc, event->endpoint_number,
				otgc->ctrl_req_addr, 0,
				SUNXI_TRBCTL_CONTROL_DATA);
	} else if ((req->request.length % dep->endpoint.maxpacket)
			&& (event->endpoint_number == 0)) {
		ret = usb_gadget_map_request(&otgc->gadget, &req->request,
				event->endpoint_number);
		if (ret) {
			dev_dbg(otgc->dev, "failed to map request\n");
			return;
		}

		WARN_ON(req->request.length > dep->endpoint.maxpacket);

		otgc->ep0_bounced = true;

		/*
		 * REVISIT in case request length is bigger than EP0
		 * wMaxPacketSize, we will need two chained TRBs to handle
		 * the transfer.
		 */
		ret = sunxi_ep0_start_trans(otgc, event->endpoint_number,
				otgc->ep0_bounce_addr, dep->endpoint.maxpacket,
				SUNXI_TRBCTL_CONTROL_DATA);
	} else {
		ret = usb_gadget_map_request(&otgc->gadget, &req->request,
				event->endpoint_number);
		if (ret) {
			dev_dbg(otgc->dev, "failed to map request\n");
			return;
		}

		ret = sunxi_ep0_start_trans(otgc, event->endpoint_number,
				req->request.dma, req->request.length,
				SUNXI_TRBCTL_CONTROL_DATA);
	}

	WARN_ON(ret < 0);
}

static int sunxi_ep0_start_control_status(struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc		*otgc = dep->otgc;
	u32			type;

	type = otgc->three_stage_setup ? SUNXI_TRBCTL_CONTROL_STATUS3
		: SUNXI_TRBCTL_CONTROL_STATUS2;

	return sunxi_ep0_start_trans(otgc, dep->number,
			otgc->ctrl_req_addr, 0, type);
}

static void sunxi_ep0_do_control_status(struct sunxi_otgc *otgc, u32 epnum)
{
	struct sunxi_otgc_ep		*dep = otgc->eps[epnum];

	if (otgc->resize_fifos) {
		dev_dbg(otgc->dev, "starting to resize fifos\n");
		sunxi_gadget_resize_tx_fifos(otgc);
		otgc->resize_fifos = 0;
	}

	WARN_ON(sunxi_ep0_start_control_status(dep));
}

static void sunxi_ep0_xfernotready(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	otgc->setup_packet_pending = true;

	if (otgc->ep0_next_event == SUNXI_EP0_COMPLETE) {
		switch (event->status) {
		case DEPEVT_STATUS_CONTROL_SETUP:
			dev_vdbg(otgc->dev, "Unexpected XferNotReady(Setup)\n");
			sunxi_ep0_stall_and_restart(otgc);
			break;
		case DEPEVT_STATUS_CONTROL_DATA:
			/* FALLTHROUGH */
		case DEPEVT_STATUS_CONTROL_STATUS:
			/* FALLTHROUGH */
		default:
			dev_vdbg(otgc->dev, "waiting for XferComplete\n");
		}

		return;
	}

	switch (event->status) {
	case DEPEVT_STATUS_CONTROL_SETUP:
		dev_vdbg(otgc->dev, "Control Setup\n");

		otgc->ep0state = EP0_SETUP_PHASE;

		sunxi_ep0_do_control_setup(otgc, event);
		break;

	case DEPEVT_STATUS_CONTROL_DATA:
		dev_vdbg(otgc->dev, "Control Data\n");

		otgc->ep0state = EP0_DATA_PHASE;

		if (otgc->ep0_next_event != SUNXI_EP0_NRDY_DATA) {
			dev_vdbg(otgc->dev, "Expected %d got %d\n",
					otgc->ep0_next_event,
					SUNXI_EP0_NRDY_DATA);

			sunxi_ep0_stall_and_restart(otgc);
			return;
		}

		/*
		 * One of the possible error cases is when Host _does_
		 * request for Data Phase, but it does so on the wrong
		 * direction.
		 *
		 * Here, we already know ep0_next_event is DATA (see above),
		 * so we only need to check for direction.
		 */
		if (otgc->ep0_expect_in != event->endpoint_number) {
			dev_vdbg(otgc->dev, "Wrong direction for Data phase\n");
			sunxi_ep0_stall_and_restart(otgc);
			return;
		}

		sunxi_ep0_do_control_data(otgc, event);
		break;

	case DEPEVT_STATUS_CONTROL_STATUS:
		dev_vdbg(otgc->dev, "Control Status\n");

		otgc->ep0state = EP0_STATUS_PHASE;

		if (otgc->ep0_next_event != SUNXI_EP0_NRDY_STATUS) {
			dev_vdbg(otgc->dev, "Expected %d got %d\n",
					otgc->ep0_next_event,
					SUNXI_EP0_NRDY_STATUS);

			sunxi_ep0_stall_and_restart(otgc);
			return;
		}

		if (otgc->delayed_status) {
			WARN_ON_ONCE(event->endpoint_number != 1);
			dev_vdbg(otgc->dev, "Mass Storage delayed status\n");
			return;
		}

		sunxi_ep0_do_control_status(otgc, event->endpoint_number);
	}
}

void sunxi_ep0_interrupt(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	u8			epnum = event->endpoint_number;

	dev_dbg(otgc->dev, "%s while ep%d%s in state '%s'\n",
			sunxi_ep_event_string(event->endpoint_event),
			epnum >> 1, (epnum & 1) ? "in" : "out",
			sunxi_ep0_state_string(otgc->ep0state));

	switch (event->endpoint_event) {
	case SUNXI_DEPEVT_XFERCOMPLETE:
		sunxi_ep0_xfer_complete(otgc, event);
		break;

	case SUNXI_DEPEVT_XFERNOTREADY:
		sunxi_ep0_xfernotready(otgc, event);
		break;

	case SUNXI_DEPEVT_XFERINPROGRESS:
	case SUNXI_DEPEVT_RXTXFIFOEVT:
	case SUNXI_DEPEVT_STREAMEVT:
	case SUNXI_DEPEVT_EPCMDCMPLT:
		break;
	}
}
