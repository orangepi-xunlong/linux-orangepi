/*
 * drivers/usb/sunxi_usb/udc/usb3/gadget.c
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

#include <linux/kernel.h>
#include <linux/delay.h>
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
#include <linux/mfd/axp-mfd.h>

#include "core.h"
#include "gadget.h"
#include "io.h"
#include "osal.h"

/**
 * sunxi_gadget_resize_tx_fifos - reallocate fifo spaces for current use-case
 * @otgc: pointer to our context structure
 *
 * This function will a best effort FIFO allocation in order
 * to improve FIFO usage and throughput, while still allowing
 * us to enable as many endpoints as possible.
 *
 * Keep in mind that this operation will be highly dependent
 * on the configured size for RAM1 - which contains TxFifo -,
 * the amount of endpoints enabled on coreConsultant tool, and
 * the width of the Master Bus.
 *
 * In the ideal world, we would always be able to satisfy the
 * following equation:
 *
 * ((512 + 2 * MDWIDTH-Bytes) + (Number of IN Endpoints - 1) * \
 * (3 * (1024 + MDWIDTH-Bytes) + MDWIDTH-Bytes)) / MDWIDTH-Bytes
 *
 * Unfortunately, due to many variables that's not always the case.
 */
int sunxi_gadget_resize_tx_fifos(struct sunxi_otgc *otgc)
{
	int		last_fifo_depth = 0;
	int		ram1_depth;
	int		fifo_size;
	int		mdwidth;
	int		num;

	if (!otgc->needs_fifo_resize)
		return 0;

	ram1_depth = sunxi_get_ram1_depth(otgc->hwparams.hwparams7);
	mdwidth = sunxi_get_mdwidth(otgc->hwparams.hwparams0);

	/* MDWIDTH is represented in bits, we need it in bytes */
	mdwidth >>= 3;

	/*
	 * FIXME For now we will only allocate 1 wMaxPacketSize space
	 * for each enabled endpoint, later patches will come to
	 * improve this algorithm so that we better use the internal
	 * FIFO space
	 */
	for (num = 0; num < SUNXI_ENDPOINTS_NUM; num++) {
		struct sunxi_otgc_ep	*dep = otgc->eps[num];
		int		fifo_number = dep->number >> 1;
		int		mult = 1;
		int		tmp;

		if (!(dep->number & 1))
			continue;

		if (!(dep->flags & SUNXI_EP_ENABLED))
			continue;

		if (usb_endpoint_xfer_bulk(dep->desc)
				|| usb_endpoint_xfer_isoc(dep->desc))
			mult = 3;

		/*
		 * REVISIT: the following assumes we will always have enough
		 * space available on the FIFO RAM for all possible use cases.
		 * Make sure that's true somehow and change FIFO allocation
		 * accordingly.
		 *
		 * If we have Bulk or Isochronous endpoints, we want
		 * them to be able to be very, very fast. So we're giving
		 * those endpoints a fifo_size which is enough for 3 full
		 * packets
		 */
		tmp = mult * (dep->endpoint.maxpacket + mdwidth);
		tmp += mdwidth;

		fifo_size = DIV_ROUND_UP(tmp, mdwidth);

		fifo_size |= (last_fifo_depth << 16);

		dev_vdbg(otgc->dev, "%s: Fifo Addr %04x Size %d\n",
				dep->name, last_fifo_depth, fifo_size & 0xffff);

		sunxi_set_gtxfifosiz(otgc->regs, fifo_number, fifo_size);

		last_fifo_depth += (fifo_size & 0xffff);
	}

	return 0;
}

void sunxi_gadget_giveback(struct sunxi_otgc_ep *dep, struct sunxi_otgc_request *req,
		int status)
{
	struct sunxi_otgc			*otgc = dep->otgc;

	if (req->queued) {
		if (req->request.num_mapped_sgs)
			dep->busy_slot += req->request.num_mapped_sgs;
		else
			dep->busy_slot++;

		/*
		 * Skip LINK TRB. We can't use req->trb and check for
		 * SUNXI_TRBCTL_LINK_TRB because it points the TRB we just
		 * completed (not the LINK TRB).
		 */
		if (((dep->busy_slot & SUNXI_TRB_MASK) == SUNXI_TRB_NUM - 1) &&
				usb_endpoint_xfer_isoc(dep->desc))
			dep->busy_slot++;
	}
	list_del(&req->list);
	req->trb = NULL;

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;

	if (otgc->ep0_bounced && dep->number == 0)
		otgc->ep0_bounced = false;
	else
		usb_gadget_unmap_request(&otgc->gadget, &req->request,
				req->direction);

	dev_dbg(otgc->dev, "request %p from %s completed %d/%d ===> %d\n",
			req, dep->name, req->request.actual,
			req->request.length, status);

	spin_unlock(&otgc->lock);
	req->request.complete(&dep->endpoint, &req->request);
	spin_lock(&otgc->lock);
}


int sunxi_send_gadget_ep_cmd(struct sunxi_otgc *otgc, unsigned ep,
		unsigned cmd, struct sunxi_otgc_gadget_ep_cmd_params *params)
{
	struct sunxi_otgc_ep		*dep = otgc->eps[ep];

	dev_vdbg(otgc->dev, "%s: cmd '%s' params %08x %08x %08x\n",
			dep->name,
			sunxi_gadget_ep_cmd_string(cmd), params->param0,
			params->param1, params->param2);

	sunxi_set_param(otgc->regs, ep, params->param0, params->param1, params->param2);
	sunxi_send_gadget_ep_cmd_ex(otgc->regs, ep, cmd);
	return 0;
}

static dma_addr_t sunxi_trb_dma_offset(struct sunxi_otgc_ep *dep,
		struct sunxi_otgc_trb *trb)
{
	u32		offset = (char *) trb - (char *) dep->trb_pool;

	return dep->trb_pool_dma + offset;
}

static int sunxi_alloc_trb_pool(struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc		*otgc = dep->otgc;

	if (dep->trb_pool)
		return 0;

	if (dep->number == 0 || dep->number == 1)
		return 0;

	dep->trb_pool = dma_alloc_coherent(otgc->dev,
			sizeof(struct sunxi_otgc_trb) * SUNXI_TRB_NUM,
			&dep->trb_pool_dma, GFP_KERNEL);
	if (!dep->trb_pool) {
		dev_err(dep->otgc->dev, "failed to allocate trb pool for %s\n",
				dep->name);
		return -ENOMEM;
	}

	return 0;
}

static void sunxi_free_trb_pool(struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc		*otgc = dep->otgc;

	dma_free_coherent(otgc->dev, sizeof(struct sunxi_otgc_trb) * SUNXI_TRB_NUM,
			dep->trb_pool, dep->trb_pool_dma);

	dep->trb_pool = NULL;
	dep->trb_pool_dma = 0;
}

static int sunxi_gadget_start_config(struct sunxi_otgc *otgc, struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc_gadget_ep_cmd_params params;
	u32			cmd;

	memset(&params, 0x00, sizeof(params));

	if (dep->number != 1) {
		cmd = SUNXI_DEPCMD_DEPSTARTCFG;
		/* XferRscIdx == 0 for ep0 and 2 for the remaining */
		if (dep->number > 1) {
			if (otgc->start_config_issued)
				return 0;
			otgc->start_config_issued = true;
			cmd |= SUNXI_DEPCMD_PARAM(2);
		}

		return sunxi_send_gadget_ep_cmd(otgc, 0, cmd, &params);
	}

	return 0;
}

static int sunxi_gadget_set_ep_config(struct sunxi_otgc *otgc, struct sunxi_otgc_ep *dep,
		const struct usb_endpoint_descriptor *desc,
		const struct usb_ss_ep_comp_descriptor *comp_desc)
{
	struct sunxi_otgc_gadget_ep_cmd_params params;

	memset(&params, 0x00, sizeof(params));

	params.param0 = sunxi_get_param0(usb_endpoint_type(desc), usb_endpoint_maxp(desc), dep->endpoint.maxburst);

	params.param1 = sunxi_set_param1_en();

	if (usb_ss_max_streams(comp_desc) && usb_endpoint_xfer_bulk(desc)) {
		params.param1 |= sunxi_set_param1_bulk();
		dep->stream_capable = true;
	}

	if (usb_endpoint_xfer_isoc(desc))
		params.param1 |= sunxi_set_param1_iso();

	/*
	 * We are doing 1:1 mapping for endpoints, meaning
	 * Physical Endpoints 2 maps to Logical Endpoint 2 and
	 * so on. We consider the direction bit as part of the physical
	 * endpoint number. So USB endpoint 0x81 is 0x03.
	 */
	params.param1 |= sunxi_depcfg_ep_number(dep->number);

	/*
	 * We must use the lower 16 TX FIFOs even though
	 * HW might have more
	 */
	if (dep->direction)
		params.param0 |= sunxi_depcfg_fifo_number(dep->number >> 1);

	if (desc->bInterval) {
		params.param1 |= sunxi_depcfg_binterval(desc->bInterval - 1);
		dep->interval = 1 << (desc->bInterval - 1);
	}

	return sunxi_send_gadget_ep_cmd(otgc, dep->number,
			SUNXI_DEPCMD_SETEPCONFIG, &params);
}

static int sunxi_gadget_set_xfer_resource(struct sunxi_otgc *otgc, struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc_gadget_ep_cmd_params params;

	memset(&params, 0x00, sizeof(params));

	params.param0 = sunxi_depcfg_num_xfer_res(1);

	return sunxi_send_gadget_ep_cmd(otgc, dep->number,
			SUNXI_DEPCMD_SETTRANSFRESOURCE, &params);
}

/**
 * __sunxi_gadget_ep_enable - Initializes a HW endpoint
 * @dep: endpoint to be initialized
 * @desc: USB Endpoint Descriptor
 *
 * Caller should take care of locking
 */
static int __sunxi_gadget_ep_enable(struct sunxi_otgc_ep *dep,
		const struct usb_endpoint_descriptor *desc,
		const struct usb_ss_ep_comp_descriptor *comp_desc)
{
	struct sunxi_otgc		*otgc = dep->otgc;
	int			ret = -ENOMEM;

	if (!(dep->flags & SUNXI_EP_ENABLED)) {
		ret = sunxi_gadget_start_config(otgc, dep);
		if (ret)
			return ret;
	}

	ret = sunxi_gadget_set_ep_config(otgc, dep, desc, comp_desc);
	if (ret)
		return ret;

	if (!(dep->flags & SUNXI_EP_ENABLED)) {
		struct sunxi_otgc_trb	*trb_st_hw;
		struct sunxi_otgc_trb	*trb_link;

		ret = sunxi_gadget_set_xfer_resource(otgc, dep);
		if (ret)
			return ret;

		dep->desc = desc;
		dep->comp_desc = comp_desc;
		dep->type = usb_endpoint_type(desc);
		dep->flags |= SUNXI_EP_ENABLED;

		sunxi_enable_dalepena_ep(otgc->regs, dep->number);

		if (!usb_endpoint_xfer_isoc(desc))
			return 0;

		memset(&trb_link, 0, sizeof(trb_link));

		/* Link TRB for ISOC. The HWO bit is never reset */
		trb_st_hw = &dep->trb_pool[0];

		trb_link = &dep->trb_pool[SUNXI_TRB_NUM - 1];

		trb_link->bpl = lower_32_bits(sunxi_trb_dma_offset(dep, trb_st_hw));
		trb_link->bph = upper_32_bits(sunxi_trb_dma_offset(dep, trb_st_hw));
		trb_link->ctrl |= SUNXI_TRBCTL_LINK_TRB;
		trb_link->ctrl |= SUNXI_TRB_CTRL_HWO;
	}

	return 0;
}

static void sunxi_stop_active_transfer(struct sunxi_otgc *otgc, u32 epnum);
static void sunxi_remove_requests(struct sunxi_otgc *otgc, struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc_request		*req;

	if (!list_empty(&dep->req_queued)){
		sunxi_stop_active_transfer(otgc, dep->number);
		/* - giveback all requests to gadget driver */
		while (!list_empty(&dep->req_queued)) {
			req = next_request(&dep->req_queued);
			sunxi_gadget_giveback(dep, req, -ESHUTDOWN);
		}
	}

	while (!list_empty(&dep->request_list)) {
		req = next_request(&dep->request_list);

		sunxi_gadget_giveback(dep, req, -ESHUTDOWN);
	}
}

/**
 * __sunxi_gadget_ep_disable - Disables a HW endpoint
 * @dep: the endpoint to disable
 *
 * This function also removes requests which are currently processed ny the
 * hardware and those which are not yet scheduled.
 * Caller should take care of locking.
 */
static int __sunxi_gadget_ep_disable(struct sunxi_otgc_ep *dep)
{
	struct sunxi_otgc		*otgc = dep->otgc;

	sunxi_remove_requests(otgc, dep);

	sunxi_disanble_dalepena_ep(otgc->regs, dep->number);

	dep->stream_capable = false;
	dep->desc = NULL;
	dep->endpoint.desc = NULL;
	dep->comp_desc = NULL;
	dep->type = 0;
	dep->flags = 0;

	return 0;
}

/* -------------------------------------------------------------------------- */

static struct usb_endpoint_descriptor sunxi_gadget_ep0_desc = {
	.bLength	= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes	= USB_ENDPOINT_XFER_CONTROL,
};
static int sunxi_gadget_ep0_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	return -EINVAL;
}

static int sunxi_gadget_ep0_disable(struct usb_ep *ep)
{
	return -EINVAL;
}
static int sunxi_gadget_ep0_disable_ex(struct sunxi_otgc *otgc)
{

	__sunxi_gadget_ep_disable(otgc->eps[0]);
	__sunxi_gadget_ep_disable(otgc->eps[1]);

	return -EINVAL;
}

static int sunxi_gadget_ep0_enable_ex(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_ep		*dep;
	int			ret = 0;

	sunxi_set_speed(otgc->regs, otgc->maximum_speed);

	otgc->start_config_issued = false;

	/* Start with SuperSpeed Default */
	sunxi_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	dep = otgc->eps[0];
	ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
	if (ret) {
		dev_err(otgc->dev, "failed to enable %s\n", dep->name);
		return -EINVAL;
	}


	dep = otgc->eps[1];
	ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
	if (ret) {
		dev_err(otgc->dev, "failed to enable %s\n", dep->name);
		return -EINVAL;
	}


	if(otgc->eps[0]->flags & SUNXI_EP_BUSY){
		sunxi_gadget_ep0_disable_ex(otgc);

		dep = otgc->eps[0];
		ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
		if (ret) {
			dev_err(otgc->dev, "failed to enable %s\n", dep->name);
			return -EINVAL;
		}

		dep = otgc->eps[1];

		ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
		if (ret) {
			dev_err(otgc->dev, "failed to enable %s\n", dep->name);
			return -EINVAL;
		}
	}

	/* begin to receive SETUP packets */
	otgc->ep0state = EP0_SETUP_PHASE;

	sunxi_ep0_out_start(otgc);

	return -EINVAL;
}

/* -------------------------------------------------------------------------- */

static int sunxi_gadget_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct sunxi_otgc_ep			*dep;
	struct sunxi_otgc			*otgc;
	unsigned long			flags;
	int				ret;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT) {
		pr_debug("sunxi: invalid parameters\n");
		return -EINVAL;
	}

	if (!desc->wMaxPacketSize) {
		pr_debug("sunxi: missing wMaxPacketSize\n");
		return -EINVAL;
	}

	dep = to_sunxi_ep(ep);
	otgc = dep->otgc;

	if (dep->flags & SUNXI_EP_ENABLED) {
		dev_WARN_ONCE(otgc->dev, true, "%s is already enabled\n",
				dep->name);
		return 0;
	}

	switch (usb_endpoint_type(desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		strlcat(dep->name, "-control", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_ISOC:
		strlcat(dep->name, "-isoc", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_BULK:
		strlcat(dep->name, "-bulk", sizeof(dep->name));
		break;
	case USB_ENDPOINT_XFER_INT:
		strlcat(dep->name, "-int", sizeof(dep->name));
		break;
	default:
		dev_err(otgc->dev, "invalid endpoint transfer type\n");
	}


	dev_vdbg(otgc->dev, "Enabling %s\n", dep->name);

	spin_lock_irqsave(&otgc->lock, flags);
	ret = __sunxi_gadget_ep_enable(dep, desc, ep->comp_desc);
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static int sunxi_gadget_ep_disable(struct usb_ep *ep)
{
	struct sunxi_otgc_ep			*dep;
	struct sunxi_otgc			*otgc;
	unsigned long			flags;
	int				ret;

	if (!ep) {
		pr_debug("sunxi: invalid parameters\n");
		return -EINVAL;
	}

	dep = to_sunxi_ep(ep);
	otgc = dep->otgc;

	if (!(dep->flags & SUNXI_EP_ENABLED)) {
		dev_WARN_ONCE(otgc->dev, true, "%s is already disabled\n",
				dep->name);
		return 0;
	}

	snprintf(dep->name, sizeof(dep->name), "ep%d%s",
			dep->number >> 1,
			(dep->number & 1) ? "in" : "out");

	spin_lock_irqsave(&otgc->lock, flags);
	ret = __sunxi_gadget_ep_disable(dep);
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static struct usb_request *sunxi_gadget_ep_alloc_request(struct usb_ep *ep,
	gfp_t gfp_flags)
{
	struct sunxi_otgc_request		*req;
	struct sunxi_otgc_ep			*dep = to_sunxi_ep(ep);
	struct sunxi_otgc			*otgc = dep->otgc;

	req = kzalloc(sizeof(*req), gfp_flags);
	if (!req) {
		dev_err(otgc->dev, "not enough memory\n");
		return NULL;
	}

	req->epnum	= dep->number;
	req->dep	= dep;

	return &req->request;
}

static void sunxi_gadget_ep_free_request(struct usb_ep *ep,
		struct usb_request *request)
{
	struct sunxi_otgc_request		*req = to_sunxi_request(request);

	kfree(req);
}

/**
 * sunxi_prepare_one_trb - setup one TRB from one request
 * @dep: endpoint for which this request is prepared
 * @req: sunxi_request pointer
 */
static void sunxi_prepare_one_trb(struct sunxi_otgc_ep *dep,
		struct sunxi_otgc_request *req, dma_addr_t dma,
		unsigned length, unsigned last, unsigned chain)
{
	struct sunxi_otgc		*otgc = dep->otgc;
	struct sunxi_otgc_trb		*trb;

	unsigned int		cur_slot;

	dev_vdbg(otgc->dev, "%s: req %p dma %08llx length %d%s%s\n",
			dep->name, req, (unsigned long long) dma,
			length, last ? " last" : "",
			chain ? " chain" : "");

	trb = &dep->trb_pool[dep->free_slot & SUNXI_TRB_MASK];
	cur_slot = dep->free_slot;
	dep->free_slot++;

	/* Skip the LINK-TRB on ISOC */
	if (((cur_slot & SUNXI_TRB_MASK) == SUNXI_TRB_NUM - 1) &&
			usb_endpoint_xfer_isoc(dep->desc))
		return;

	if (!req->trb) {
		sunxi_gadget_move_request_queued(req);
		req->trb = trb;
		req->trb_dma = sunxi_trb_dma_offset(dep, trb);
	}

	trb->size = SUNXI_TRB_SIZE_LENGTH(length);
	trb->bpl = lower_32_bits(dma);
	trb->bph = upper_32_bits(dma);

	switch (usb_endpoint_type(dep->desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		trb->ctrl = SUNXI_TRBCTL_CONTROL_SETUP;
		break;

	case USB_ENDPOINT_XFER_ISOC:
		trb->ctrl = SUNXI_TRBCTL_ISOCHRONOUS_FIRST;

		/* IOC every SUNXI_TRB_NUM / 4 so we can refill */
		if (!(cur_slot % (SUNXI_TRB_NUM / 4)))
			trb->ctrl |= SUNXI_TRB_CTRL_IOC;
		break;

	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		trb->ctrl = SUNXI_TRBCTL_NORMAL;
		break;
	default:
		/*
		 * This is only possible with faulty memory because we
		 * checked it already :)
		 */
		BUG();
	}

	if (usb_endpoint_xfer_isoc(dep->desc)) {
		trb->ctrl |= SUNXI_TRB_CTRL_ISP_IMI;
		trb->ctrl |= SUNXI_TRB_CTRL_CSP;
	} else {
		if (chain)
			trb->ctrl |= SUNXI_TRB_CTRL_CHN;

		if (last)
			trb->ctrl |= SUNXI_TRB_CTRL_LST;
	}

	if (usb_endpoint_xfer_bulk(dep->desc) && dep->stream_capable)
		trb->ctrl |= SUNXI_TRB_CTRL_SID_SOFN(req->request.stream_id);

	trb->ctrl |= SUNXI_TRB_CTRL_HWO;
}

/*
 * sunxi_prepare_trbs - setup TRBs from requests
 * @dep: endpoint for which requests are being prepared
 * @starting: true if the endpoint is idle and no requests are queued.
 *
 * The function goes through the requests list and sets up TRBs for the
 * transfers. The function returns once there are no more TRBs available or
 * it runs out of requests.
 */
static void sunxi_prepare_trbs(struct sunxi_otgc_ep *dep, bool starting)
{
	struct sunxi_otgc_request	*req, *n;
	u32			trbs_left;
	u32			max;
	unsigned int		last_one = 0;

	BUILD_BUG_ON_NOT_POWER_OF_2(SUNXI_TRB_NUM);

	/* the first request must not be queued */
	trbs_left = (dep->busy_slot - dep->free_slot) & SUNXI_TRB_MASK;

	/* Can't wrap around on a non-isoc EP since there's no link TRB */
	if (!usb_endpoint_xfer_isoc(dep->desc)) {
		max = SUNXI_TRB_NUM - (dep->free_slot & SUNXI_TRB_MASK);
		if (trbs_left > max)
			trbs_left = max;
	}

	/*
	 * If busy & slot are equal than it is either full or empty. If we are
	 * starting to process requests then we are empty. Otherwise we are
	 * full and don't do anything
	 */
	if (!trbs_left) {
		if (!starting)
			return;
		trbs_left = SUNXI_TRB_NUM;
		/*
		 * In case we start from scratch, we queue the ISOC requests
		 * starting from slot 1. This is done because we use ring
		 * buffer and have no LST bit to stop us. Instead, we place
		 * IOC bit every TRB_NUM/4. We try to avoid having an interrupt
		 * after the first request so we start at slot 1 and have
		 * 7 requests proceed before we hit the first IOC.
		 * Other transfer types don't use the ring buffer and are
		 * processed from the first TRB until the last one. Since we
		 * don't wrap around we have to start at the beginning.
		 */
		if (usb_endpoint_xfer_isoc(dep->desc)) {
			dep->busy_slot = 1;
			dep->free_slot = 1;
		} else {
			dep->busy_slot = 0;
			dep->free_slot = 0;
		}
	}

	/* The last TRB is a link TRB, not used for xfer */
	if ((trbs_left <= 1) && usb_endpoint_xfer_isoc(dep->desc))
		return;

	list_for_each_entry_safe(req, n, &dep->request_list, list) {
		unsigned	length;
		dma_addr_t	dma;

		if (req->request.num_mapped_sgs > 0) {
			struct usb_request *request = &req->request;
			struct scatterlist *sg = request->sg;
			struct scatterlist *s;
			int		i;

			for_each_sg(sg, s, request->num_mapped_sgs, i) {
				unsigned chain = true;

				length = sg_dma_len(s);
				dma = sg_dma_address(s);

				if (i == (request->num_mapped_sgs - 1) ||
						sg_is_last(s)) {
					last_one = true;
					chain = false;
				}

				trbs_left--;
				if (!trbs_left)
					last_one = true;

				if (last_one)
					chain = false;

				sunxi_prepare_one_trb(dep, req, dma, length,
						last_one, chain);

				if (last_one)
					break;
			}
		} else {
			dma = req->request.dma;
			length = req->request.length;
			trbs_left--;

			if (!trbs_left)
				last_one = 1;

			/* Is this the last request? */
			if (list_is_last(&req->list, &dep->request_list))
				last_one = 1;

			sunxi_prepare_one_trb(dep, req, dma, length,
					last_one, false);

			if (last_one)
				break;
		}
	}
}

static int __sunxi_gadget_kick_transfer(struct sunxi_otgc_ep *dep, u16 cmd_param,
		int start_new)
{
	struct sunxi_otgc_gadget_ep_cmd_params params;
	struct sunxi_otgc_request		*req;
	struct sunxi_otgc			*otgc = dep->otgc;
	int				ret;
	u32				cmd;

	if (start_new && (dep->flags & SUNXI_EP_BUSY)) {
		dev_vdbg(otgc->dev, "%s: endpoint busy\n", dep->name);
		return -EBUSY;
	}
	dep->flags &= ~SUNXI_EP_PENDING_REQUEST;

	/*
	 * If we are getting here after a short-out-packet we don't enqueue any
	 * new requests as we try to set the IOC bit only on the last request.
	 */
	if (start_new) {
		if (list_empty(&dep->req_queued))
			sunxi_prepare_trbs(dep, start_new);

		/* req points to the first request which will be sent */
		req = next_request(&dep->req_queued);
	} else {
		sunxi_prepare_trbs(dep, start_new);

		/*
		 * req points to the first request where HWO changed from 0 to 1
		 */
		req = next_request(&dep->req_queued);
	}
	if (!req) {
		dep->flags |= SUNXI_EP_PENDING_REQUEST;
		return 0;
	}

	memset(&params, 0, sizeof(params));
	params.param0 = upper_32_bits(req->trb_dma);
	params.param1 = lower_32_bits(req->trb_dma);

	if (start_new)
		cmd = SUNXI_DEPCMD_STARTTRANSFER;
	else
		cmd = SUNXI_DEPCMD_UPDATETRANSFER;

	cmd |= SUNXI_DEPCMD_PARAM(cmd_param);
	ret = sunxi_send_gadget_ep_cmd(otgc, dep->number, cmd, &params);
	if (ret < 0) {
		dev_dbg(otgc->dev, "failed to send STARTTRANSFER command\n");

		/*
		 * FIXME we need to iterate over the list of requests
		 * here and stop, unmap, free and del each of the linked
		 * requests instead of what we do now.
		 */
		usb_gadget_unmap_request(&otgc->gadget, &req->request,
				req->direction);
		list_del(&req->list);
		return ret;
	}

	dep->flags |= SUNXI_EP_BUSY;
	dep->res_trans_idx = sunxi_gadget_ep_get_transfer_index(otgc->regs,
			dep->number);

	WARN_ON_ONCE(!dep->res_trans_idx);

	return 0;
}

static int __sunxi_gadget_ep_queue(struct sunxi_otgc_ep *dep, struct sunxi_otgc_request *req)
{
	struct sunxi_otgc		*otgc = dep->otgc;
	int			ret;

	req->request.actual	= 0;
	req->request.status	= -EINPROGRESS;
	req->direction		= dep->direction;
	req->epnum		= dep->number;

	/*
	 * We only add to our list of requests now and
	 * start consuming the list once we get XferNotReady
	 * IRQ.
	 *
	 * That way, we avoid doing anything that we don't need
	 * to do now and defer it until the point we receive a
	 * particular token from the Host side.
	 *
	 * This will also avoid Host cancelling URBs due to too
	 * many NAKs.
	 */
	ret = usb_gadget_map_request(&otgc->gadget, &req->request,
			dep->direction);
	if (ret)
		return ret;

	list_add_tail(&req->list, &dep->request_list);

	/*
	 * There is one special case: XferNotReady with
	 * empty list of requests. We need to kick the
	 * transfer here in that situation, otherwise
	 * we will be NAKing forever.
	 *
	 * If we get XferNotReady before gadget driver
	 * has a chance to queue a request, we will ACK
	 * the IRQ but won't be able to receive the data
	 * until the next request is queued. The following
	 * code is handling exactly that.
	 */
	if (dep->flags & SUNXI_EP_PENDING_REQUEST) {
		int ret;
		int start_trans;

		start_trans = 1;
		if (usb_endpoint_xfer_isoc(dep->desc) &&
				(dep->flags & SUNXI_EP_BUSY))
			start_trans = 0;

		ret = __sunxi_gadget_kick_transfer(dep, 0, start_trans);
		if (ret && ret != -EBUSY) {
			struct sunxi_otgc	*otgc = dep->otgc;

			dev_dbg(otgc->dev, "%s: failed to kick transfers\n",
					dep->name);
		}
	};

	return 0;
}

static int sunxi_gadget_ep_queue(struct usb_ep *ep, struct usb_request *request,
	gfp_t gfp_flags)
{
	struct sunxi_otgc_request		*req = to_sunxi_request(request);
	struct sunxi_otgc_ep			*dep = to_sunxi_ep(ep);
	struct sunxi_otgc			*otgc = dep->otgc;

	unsigned long			flags;

	int				ret;

	if (!dep->desc) {
		dev_dbg(otgc->dev, "trying to queue request %p to disabled %s\n",
				request, ep->name);
		return -ESHUTDOWN;
	}

	dev_vdbg(otgc->dev, "queing request %p to %s length %d\n",
			request, ep->name, request->length);

	spin_lock_irqsave(&otgc->lock, flags);
	ret = __sunxi_gadget_ep_queue(dep, req);
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static int sunxi_gadget_ep_dequeue(struct usb_ep *ep,
		struct usb_request *request)
{
	struct sunxi_otgc_request		*req = to_sunxi_request(request);
	struct sunxi_otgc_request		*r = NULL;

	struct sunxi_otgc_ep			*dep = to_sunxi_ep(ep);
	struct sunxi_otgc			*otgc = dep->otgc;

	unsigned long			flags;
	int				ret = 0;

	spin_lock_irqsave(&otgc->lock, flags);

	list_for_each_entry(r, &dep->request_list, list) {
		if (r == req)
			break;
	}

	if (r != req) {
		list_for_each_entry(r, &dep->req_queued, list) {
			if (r == req)
				break;
		}
		if (r == req) {
			/* wait until it is processed */
			sunxi_stop_active_transfer(otgc, dep->number);
			goto out1;
		}
		dev_err(otgc->dev, "request %p was not queued to %s\n",
				request, ep->name);
		ret = -EINVAL;
		goto out0;
	}

out1:
	/* giveback the request */
	sunxi_gadget_giveback(dep, req, -ECONNRESET);

out0:
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

int __sunxi_gadget_ep_set_halt(struct sunxi_otgc_ep *dep, int value)
{
	struct sunxi_otgc_gadget_ep_cmd_params	params;
	struct sunxi_otgc				*otgc = dep->otgc;
	int					ret;

	memset(&params, 0x00, sizeof(params));

	if (value) {
		if (dep->number == 0 || dep->number == 1) {
			/*
			 * Whenever EP0 is stalled, we will restart
			 * the state machine, thus moving back to
			 * Setup Phase
			 */
			otgc->ep0state = EP0_SETUP_PHASE;
		}

		ret = sunxi_send_gadget_ep_cmd(otgc, dep->number,
			SUNXI_DEPCMD_SETSTALL, &params);
		if (ret)
			dev_err(otgc->dev, "failed to %s STALL on %s\n",
					value ? "set" : "clear",
					dep->name);
		else
			dep->flags |= SUNXI_EP_STALL;
	} else {
		if (dep->flags & SUNXI_EP_WEDGE)
			return 0;

		ret = sunxi_send_gadget_ep_cmd(otgc, dep->number,
			SUNXI_DEPCMD_CLEARSTALL, &params);
		if (ret)
			dev_err(otgc->dev, "failed to %s STALL on %s\n",
					value ? "set" : "clear",
					dep->name);
		else
			dep->flags &= ~SUNXI_EP_STALL;
	}

	return ret;
}

static int sunxi_gadget_ep_set_halt(struct usb_ep *ep, int value)
{
	struct sunxi_otgc_ep			*dep = to_sunxi_ep(ep);
	struct sunxi_otgc			*otgc = dep->otgc;

	unsigned long			flags;

	int				ret;

	spin_lock_irqsave(&otgc->lock, flags);

	if (usb_endpoint_xfer_isoc(dep->desc)) {
		dev_err(otgc->dev, "%s is of Isochronous type\n", dep->name);
		ret = -EINVAL;
		goto out;
	}

	ret = __sunxi_gadget_ep_set_halt(dep, value);
out:
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static int sunxi_gadget_ep_set_wedge(struct usb_ep *ep)
{
	struct sunxi_otgc_ep			*dep = to_sunxi_ep(ep);
	struct sunxi_otgc			*otgc = dep->otgc;
	unsigned long			flags;

	spin_lock_irqsave(&otgc->lock, flags);
	dep->flags |= SUNXI_EP_WEDGE;
	spin_unlock_irqrestore(&otgc->lock, flags);

	return sunxi_gadget_ep_set_halt(ep, 1);
}


static const struct usb_ep_ops sunxi_gadget_ep0_ops = {
	.enable		= sunxi_gadget_ep0_enable,
	.disable	= sunxi_gadget_ep0_disable,
	.alloc_request	= sunxi_gadget_ep_alloc_request,
	.free_request	= sunxi_gadget_ep_free_request,
	.queue		= sunxi_gadget_ep0_queue,
	.dequeue	= sunxi_gadget_ep_dequeue,
	.set_halt	= sunxi_gadget_ep_set_halt,
	.set_wedge	= sunxi_gadget_ep_set_wedge,
};

static const struct usb_ep_ops sunxi_gadget_ep_ops = {
	.enable		= sunxi_gadget_ep_enable,
	.disable	= sunxi_gadget_ep_disable,
	.alloc_request	= sunxi_gadget_ep_alloc_request,
	.free_request	= sunxi_gadget_ep_free_request,
	.queue		= sunxi_gadget_ep_queue,
	.dequeue	= sunxi_gadget_ep_dequeue,
	.set_halt	= sunxi_gadget_ep_set_halt,
	.set_wedge	= sunxi_gadget_ep_set_wedge,
};

/* -------------------------------------------------------------------------- */

static int sunxi_gadget_get_frame(struct usb_gadget *g)
{
	struct sunxi_otgc		*otgc = gadget_to_otgc(g);
	return sunxi_gadget_get_frame_ex(otgc->regs);
}

static int sunxi_gadget_wakeup(struct usb_gadget *g)
{
	struct sunxi_otgc		*otgc = gadget_to_otgc(g);

	unsigned long		timeout;
	unsigned long		flags;

	u32			reg;

	int			ret = 0;

	u8			link_state;

	spin_lock_irqsave(&otgc->lock, flags);

	/*
	 * According to the Databook Remote wakeup request should
	 * be issued only when the device is in early suspend state.
	 *
	 * We can check that via USB Link State bits in DSTS register.
	 */
	reg = sunxi_get_dsts(otgc->regs);

	if (sunxi_is_superspeed(otgc->regs)) {
		dev_dbg(otgc->dev, "no wakeup on SuperSpeed\n");
		ret = -EINVAL;
		goto out;
	}

	link_state = sunxi_get_usblnkst(otgc->regs);

	switch (link_state) {
	case SUNXI_LINK_STATE_RX_DET:	/* in HS, means Early Suspend */
	case SUNXI_LINK_STATE_U3:	/* in HS, means SUSPEND */
		break;
	default:
		dev_dbg(otgc->dev, "can't wakeup from link state %d\n",
				link_state);
		ret = -EINVAL;
		goto out;
	}

	ret = sunxi_gadget_set_link_state(otgc->regs, SUNXI_LINK_STATE_RECOV);
	if (ret < 0) {
		dev_err(otgc->dev, "failed to put link in Recovery\n");
		goto out;
	}

	/* write zeroes to Link Change Request */
	sunxi_clear_ulstchngreq(otgc->regs, reg);

	/* poll until Link State changes to ON */
	timeout = jiffies + msecs_to_jiffies(100);

	while (!time_after(jiffies, timeout)) {
		reg = sunxi_get_dsts(otgc->regs);

		/* in HS, means ON */
		if (sunxi_dsts_usblnkst(reg) == SUNXI_LINK_STATE_U0)
			break;
	}

	if (sunxi_dsts_usblnkst(reg) != SUNXI_LINK_STATE_U0) {
		dev_err(otgc->dev, "failed to send remote wakeup\n");
		ret = -EINVAL;
	}

out:
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static int sunxi_gadget_set_selfpowered(struct usb_gadget *g,
		int is_selfpowered)
{
	struct sunxi_otgc		*otgc = gadget_to_otgc(g);
	unsigned long		flags;

	spin_lock_irqsave(&otgc->lock, flags);
	otgc->is_selfpowered = !!is_selfpowered;
	spin_unlock_irqrestore(&otgc->lock, flags);

	return 0;
}

static void sunxi_gadget_run_stop(struct sunxi_otgc *otgc, int is_on)
{

	sunxi_gadget_run_stop_ex(otgc->regs, is_on);

	dev_vdbg(otgc->dev, "gadget %s data soft-%s\n",
			otgc->gadget_driver
			? otgc->gadget_driver->function : "no-function",
			is_on ? "connect" : "disconnect");
}

static int sunxi_gadget_pullup(struct usb_gadget *g, int is_on)
{
	struct sunxi_otgc		*otgc = gadget_to_otgc(g);
	unsigned long		flags;

	is_on = !!is_on;

	spin_lock_irqsave(&otgc->lock, flags);
	sunxi_gadget_run_stop(otgc, is_on);
	spin_unlock_irqrestore(&otgc->lock, flags);

	return 0;
}

static int sunxi_gadget_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct sunxi_otgc		*otgc = gadget_to_otgc(g);
	struct sunxi_otgc_ep		*dep;
	unsigned long		flags;
	int			ret = 0;

	spin_lock_irqsave(&otgc->lock, flags);

	if (otgc->gadget_driver) {
		dev_err(otgc->dev, "%s is already bound to %s\n",
				otgc->gadget.name,
				otgc->gadget_driver->driver.name);
		ret = -EBUSY;
		goto err0;
	}

	otgc->gadget_driver	= driver;
	otgc->gadget.dev.driver	= &driver->driver;


	sunxi_set_speed(otgc->regs, otgc->maximum_speed);


	otgc->start_config_issued = false;

	/* Start with SuperSpeed Default */
	sunxi_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	dep = otgc->eps[0];
	ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
	if (ret) {
		dev_err(otgc->dev, "failed to enable %s\n", dep->name);
		goto err0;
	}

	dep = otgc->eps[1];
	ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
	if (ret) {
		dev_err(otgc->dev, "failed to enable %s\n", dep->name);
		goto err1;
	}

	/* begin to receive SETUP packets */
	otgc->ep0state = EP0_SETUP_PHASE;
	sunxi_ep0_out_start(otgc);

	spin_unlock_irqrestore(&otgc->lock, flags);

	return 0;

err1:
	__sunxi_gadget_ep_disable(otgc->eps[0]);

err0:
	spin_unlock_irqrestore(&otgc->lock, flags);

	return ret;
}

static int sunxi_gadget_stop(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	struct sunxi_otgc		*otgc = gadget_to_otgc(g);
	unsigned long		flags;

	spin_lock_irqsave(&otgc->lock, flags);

	__sunxi_gadget_ep_disable(otgc->eps[0]);
	__sunxi_gadget_ep_disable(otgc->eps[1]);

	otgc->gadget_driver	= NULL;
	otgc->gadget.dev.driver	= NULL;

	spin_unlock_irqrestore(&otgc->lock, flags);

	return 0;
}
static const struct usb_gadget_ops sunxi_gadget_ops = {
	.get_frame		= sunxi_gadget_get_frame,
	.wakeup			= sunxi_gadget_wakeup,
	.set_selfpowered	= sunxi_gadget_set_selfpowered,
	.pullup			= sunxi_gadget_pullup,
	.udc_start		= sunxi_gadget_start,
	.udc_stop		= sunxi_gadget_stop,
};

/* -------------------------------------------------------------------------- */

static int __devinit sunxi_gadget_init_endpoints(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_ep			*dep;
	u8				epnum;

	INIT_LIST_HEAD(&otgc->gadget.ep_list);

	for (epnum = 0; epnum < SUNXI_ENDPOINTS_NUM; epnum++) {
		dep = kzalloc(sizeof(*dep), GFP_KERNEL);
		if (!dep) {
			dev_err(otgc->dev, "can't allocate endpoint %d\n",
					epnum);
			return -ENOMEM;
		}

		dep->otgc = otgc;
		dep->number = epnum;
		otgc->eps[epnum] = dep;

		snprintf(dep->name, sizeof(dep->name), "ep%d%s", epnum >> 1,
				(epnum & 1) ? "in" : "out");
		dep->endpoint.name = dep->name;
		dep->direction = (epnum & 1);

		if (epnum == 0 || epnum == 1) {
			dep->endpoint.maxpacket = 512;
			dep->endpoint.maxburst = 1;
			dep->endpoint.ops = &sunxi_gadget_ep0_ops;
			if (!epnum)
				otgc->gadget.ep0 = &dep->endpoint;
		} else {
			int		ret;

			dep->endpoint.maxpacket = 1024;
			dep->endpoint.max_streams = 15;
			dep->endpoint.ops = &sunxi_gadget_ep_ops;
			list_add_tail(&dep->endpoint.ep_list,
					&otgc->gadget.ep_list);

			ret = sunxi_alloc_trb_pool(dep);
			if (ret)
				return ret;
		}

		INIT_LIST_HEAD(&dep->request_list);
		INIT_LIST_HEAD(&dep->req_queued);
	}

	return 0;
}

static void sunxi_gadget_free_endpoints(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_ep			*dep;
	u8				epnum;

	for (epnum = 0; epnum < SUNXI_ENDPOINTS_NUM; epnum++) {
		dep = otgc->eps[epnum];
		sunxi_free_trb_pool(dep);

		if (epnum != 0 && epnum != 1)
			list_del(&dep->endpoint.ep_list);

		kfree(dep);
	}
}

static void sunxi_gadget_release(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
}

/* -------------------------------------------------------------------------- */
static int sunxi_cleanup_done_reqs(struct sunxi_otgc *otgc, struct sunxi_otgc_ep *dep,
		const struct sunxi_otgc_event_depevt *event, int status)
{
	struct sunxi_otgc_request	*req;
	struct sunxi_otgc_trb		*trb;
	unsigned int		count;
	unsigned int		s_pkt = 0;

	do {
		req = next_request(&dep->req_queued);
		if (!req) {
			pr_debug("request was not queued\n");
			return 1;
		}

		trb = req->trb;

		if ((trb->ctrl & SUNXI_TRB_CTRL_HWO) && status != -ESHUTDOWN)
			/*
			 * We continue despite the error. There is not much we
			 * can do. If we don't clean it up we loop forever. If
			 * we skip the TRB then it gets overwritten after a
			 * while since we use them in a ring buffer. A BUG()
			 * would help. Lets hope that if this occurs, someone
			 * fixes the root cause instead of looking away :)
			 */
			dev_err(otgc->dev, "%s's TRB (%p) still owned by HW\n",
					dep->name, req->trb);
		count = trb->size & SUNXI_TRB_SIZE_MASK;

		if (dep->direction) {
			if (count) {
				dev_err(otgc->dev, "incomplete IN transfer %s\n",
						dep->name);
				status = -ECONNRESET;
			}
		} else {
			if (count && (event->status & DEPEVT_STATUS_SHORT))
				s_pkt = 1;
		}

		/*
		 * We assume here we will always receive the entire data block
		 * which we should receive. Meaning, if we program RX to
		 * receive 4K but we receive only 2K, we assume that's all we
		 * should receive and we simply bounce the request back to the
		 * gadget driver for further processing.
		 */
		req->request.actual += req->request.length - count;
		sunxi_gadget_giveback(dep, req, status);
		if (s_pkt)
			break;
		if ((event->status & DEPEVT_STATUS_LST) &&
				(trb->ctrl & SUNXI_TRB_CTRL_LST))
			break;
		if ((event->status & DEPEVT_STATUS_IOC) &&
				(trb->ctrl & SUNXI_TRB_CTRL_IOC))
			break;
	} while (1);

	if ((event->status & DEPEVT_STATUS_IOC) &&
			(trb->ctrl & SUNXI_TRB_CTRL_IOC))
		return 0;
	return 1;
}

static void sunxi_endpoint_transfer_complete(struct sunxi_otgc *otgc,
		struct sunxi_otgc_ep *dep, const struct sunxi_otgc_event_depevt *event,
		int start_new)
{
	unsigned		status = 0;
	int			clean_busy;

	if (event->status & DEPEVT_STATUS_BUSERR)
		status = -ECONNRESET;

	clean_busy = sunxi_cleanup_done_reqs(otgc, dep, event, status);
	if (clean_busy)
		dep->flags &= ~SUNXI_EP_BUSY;

	/*
	 * WORKAROUND: This is the 2nd half of U1/U2 -> U0 workaround.
	 * See sunxi_gadget_linksts_change_interrupt() for 1st half.
	 */
	if (sunxi_is_revision_183A(otgc->revision)) {
		int		i;

		for (i = 0; i < SUNXI_ENDPOINTS_NUM; i++) {
			struct sunxi_otgc_ep	*dep = otgc->eps[i];

			if (!(dep->flags & SUNXI_EP_ENABLED))
				continue;

			if (!list_empty(&dep->req_queued))
				return;
		}

		sunxi_set_dctl(otgc->regs, otgc->u1u2);

		otgc->u1u2 = 0;
	}
}

static void sunxi_gadget_start_isoc(struct sunxi_otgc *otgc,
		struct sunxi_otgc_ep *dep, const struct sunxi_otgc_event_depevt *event)
{
	u32 uf, mask;

	if (list_empty(&dep->request_list)) {
		dev_vdbg(otgc->dev, "ISOC ep %s run out for requests.\n",
			dep->name);
		return;
	}

	mask = ~(dep->interval - 1);
	uf = event->parameters & mask;
	/* 4 micro frames in the future */
	uf += dep->interval * 4;

	__sunxi_gadget_kick_transfer(dep, uf, 1);
}

static void sunxi_process_ep_cmd_complete(struct sunxi_otgc_ep *dep,
		const struct sunxi_otgc_event_depevt *event)
{
	struct sunxi_otgc *otgc = dep->otgc;
	struct sunxi_otgc_event_depevt mod_ev = *event;

	/*
	 * We were asked to remove one request. It is possible that this
	 * request and a few others were started together and have the same
	 * transfer index. Since we stopped the complete endpoint we don't
	 * know how many requests were already completed (and not yet)
	 * reported and how could be done (later). We purge them all until
	 * the end of the list.
	 */
	mod_ev.status = DEPEVT_STATUS_LST;
	sunxi_cleanup_done_reqs(otgc, dep, &mod_ev, -ESHUTDOWN);
	dep->flags &= ~SUNXI_EP_BUSY;
	/* pending requests are ignored and are queued on XferNotReady */
}

static void sunxi_ep_cmd_compl(struct sunxi_otgc_ep *dep,
		const struct sunxi_otgc_event_depevt *event)
{
	u32 param = event->parameters;
	u32 cmd_type = (param >> 8) & ((1 << 5) - 1);

	switch (cmd_type) {
	case SUNXI_DEPCMD_ENDTRANSFER:
		sunxi_process_ep_cmd_complete(dep, event);
		break;
	case SUNXI_DEPCMD_STARTTRANSFER:
		dep->res_trans_idx = param & 0x7f;
		break;
	default:
		pr_err(KERN_ERR "%s() unknown /unexpected type: %d\n",
				__func__, cmd_type);
		break;
	};
}

static void sunxi_endpoint_interrupt(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_depevt *event)
{
	struct sunxi_otgc_ep		*dep;
	u8			epnum = event->endpoint_number;

	dep = otgc->eps[epnum];

	dev_vdbg(otgc->dev, "%s: %s\n", dep->name,
			sunxi_ep_event_string(event->endpoint_event));

	if (epnum == 0 || epnum == 1) {
		sunxi_ep0_interrupt(otgc, event);
		return;
	}

	switch (event->endpoint_event) {
	case SUNXI_DEPEVT_XFERCOMPLETE:
		dep->res_trans_idx = 0;

		if (usb_endpoint_xfer_isoc(dep->desc)) {
			dev_dbg(otgc->dev, "%s is an Isochronous endpoint\n",
					dep->name);
			return;
		}

		sunxi_endpoint_transfer_complete(otgc, dep, event, 1);
		break;
	case SUNXI_DEPEVT_XFERINPROGRESS:
		if (!usb_endpoint_xfer_isoc(dep->desc)) {
			dev_dbg(otgc->dev, "%s is not an Isochronous endpoint\n",
					dep->name);
			return;
		}

		sunxi_endpoint_transfer_complete(otgc, dep, event, 0);
		break;
	case SUNXI_DEPEVT_XFERNOTREADY:
		if (usb_endpoint_xfer_isoc(dep->desc)) {
			sunxi_gadget_start_isoc(otgc, dep, event);
		} else {
			int ret;

			dev_vdbg(otgc->dev, "%s: reason %s\n",
					dep->name, event->status &
					DEPEVT_STATUS_TRANSFER_ACTIVE
					? "Transfer Active"
					: "Transfer Not Active");

			ret = __sunxi_gadget_kick_transfer(dep, 0, 1);
			if (!ret || ret == -EBUSY)
				return;

			dev_dbg(otgc->dev, "%s: failed to kick transfers\n",
					dep->name);
		}

		break;
	case SUNXI_DEPEVT_STREAMEVT:
		if (!usb_endpoint_xfer_bulk(dep->desc)) {
			dev_err(otgc->dev, "Stream event for non-Bulk %s\n",
					dep->name);
			return;
		}

		switch (event->status) {
		case DEPEVT_STREAMEVT_FOUND:
			dev_vdbg(otgc->dev, "Stream %d found and started\n",
					event->parameters);

			break;
		case DEPEVT_STREAMEVT_NOTFOUND:
			/* FALLTHROUGH */
		default:
			dev_dbg(otgc->dev, "Couldn't find suitable stream\n");
		}
		break;
	case SUNXI_DEPEVT_RXTXFIFOEVT:
		dev_dbg(otgc->dev, "%s FIFO Overrun\n", dep->name);
		break;
	case SUNXI_DEPEVT_EPCMDCMPLT:
		sunxi_ep_cmd_compl(dep, event);
		break;
	}
}

static void sunxi_disconnect_gadget(struct sunxi_otgc *otgc)
{
	if (otgc->gadget_driver && otgc->gadget_driver->disconnect) {
		spin_unlock(&otgc->lock);
		otgc->gadget_driver->disconnect(&otgc->gadget);
		spin_lock(&otgc->lock);
	}
}

static void sunxi_stop_active_transfer(struct sunxi_otgc *otgc, u32 epnum)
{
	struct sunxi_otgc_ep *dep;
	struct sunxi_otgc_gadget_ep_cmd_params params;
	u32 cmd;
	int ret;

	dep = otgc->eps[epnum];


	if (!dep->res_trans_idx) {
		return;
	}

	cmd = SUNXI_DEPCMD_ENDTRANSFER;
	cmd |= SUNXI_DEPCMD_HIPRI_FORCERM | SUNXI_DEPCMD_CMDIOC;
	cmd |= SUNXI_DEPCMD_PARAM(dep->res_trans_idx);
	memset(&params, 0, sizeof(params));
	ret = sunxi_send_gadget_ep_cmd(otgc, dep->number, cmd, &params);
	WARN_ON_ONCE(ret);
	dep->res_trans_idx = 0;
	dep->flags &= ~SUNXI_EP_BUSY;

	udelay(100);
}

static void sunxi_stop_active_transfers(struct sunxi_otgc *otgc)
{
	u32 epnum;

	for (epnum = 2; epnum < SUNXI_ENDPOINTS_NUM; epnum++) {
		struct sunxi_otgc_ep *dep;

		dep = otgc->eps[epnum];
		if (!(dep->flags & SUNXI_EP_ENABLED))
			continue;

		sunxi_remove_requests(otgc, dep);
	}
}

static void sunxi_clear_stall_all_ep(struct sunxi_otgc *otgc)
{
	u32 epnum;

	for (epnum = 1; epnum < SUNXI_ENDPOINTS_NUM; epnum++) {
		struct sunxi_otgc_ep *dep;
		struct sunxi_otgc_gadget_ep_cmd_params params;
		int ret;

		dep = otgc->eps[epnum];

		if (!(dep->flags & SUNXI_EP_STALL))
			continue;

		dep->flags &= ~SUNXI_EP_STALL;

		memset(&params, 0, sizeof(params));
		ret = sunxi_send_gadget_ep_cmd(otgc, dep->number,
				SUNXI_DEPCMD_CLEARSTALL, &params);
		WARN_ON_ONCE(ret);
	}
}

static void sunxi_gadget_disconnect_interrupt(struct sunxi_otgc *otgc)
{
	dev_vdbg(otgc->dev, "%s\n", __func__);

	sunxi_stop_active_transfers(otgc);
	sunxi_disconnect_gadget(otgc);
	otgc->start_config_issued = false;

	otgc->gadget.speed = USB_SPEED_UNKNOWN;
	otgc->setup_packet_pending = false;
}

static void sunxi_gadget_usb3_phy_power(struct sunxi_otgc *otgc, int on)
{
	sunxi_gadget_usb3_phy_power_ex(otgc->regs, on);
}

static void sunxi_gadget_usb2_phy_power(struct sunxi_otgc *otgc, int on)
{
	sunxi_gadget_usb2_phy_power_ex(otgc->regs, on);

}

static void sunxi_gadget_reset_interrupt(struct sunxi_otgc *otgc)
{
	dev_vdbg(otgc->dev, "%s\n", __func__);

	if (sunxi_is_revision_188A(otgc->revision)) {
		if (otgc->setup_packet_pending)
			sunxi_gadget_disconnect_interrupt(otgc);
	}

	/* after reset -> Default State */
	otgc->dev_state = SUNXI_DEFAULT_STATE;

	/* Enable PHYs */
	sunxi_gadget_usb2_phy_power(otgc, true);
	sunxi_gadget_usb3_phy_power(otgc, true);

	if (otgc->gadget.speed != USB_SPEED_UNKNOWN)
		sunxi_disconnect_gadget(otgc);

	sunxi_clear_dcfg_tstctrl_mask(otgc->regs);

	otgc->test_mode = false;

	sunxi_stop_active_transfers(otgc);
	sunxi_clear_stall_all_ep(otgc);
	otgc->start_config_issued = false;

	/* Reset device address to zero */
	sunxi_clear_dcfg_devaddr_mask(otgc->regs);
}

static void sunxi_update_ram_clk_sel(struct sunxi_otgc *otgc, u32 speed)
{
	sunxi_update_ram_clk_sel_ex(otgc->regs, speed);
}

static void sunxi_gadget_disable_phy(struct sunxi_otgc *otgc, u8 speed)
{
	switch (speed) {
	case USB_SPEED_SUPER:
		sunxi_gadget_usb2_phy_power(otgc, false);
#if defined(CONFIG_AW_AXP)
		axp_usbvol(CHARGE_USB_30);
		axp_usbcur(CHARGE_USB_30);
#endif

		break;
	case USB_SPEED_HIGH:
	case USB_SPEED_FULL:
	case USB_SPEED_LOW:
#if defined(CONFIG_AW_AXP)
		axp_usbvol(CHARGE_USB_20);
		axp_usbcur(CHARGE_USB_20);
#endif
		sunxi_gadget_usb3_phy_power(otgc, false);
		break;
	}
}

static void sunxi_gadget_conndone_interrupt(struct sunxi_otgc *otgc)
{
	struct sunxi_otgc_gadget_ep_cmd_params params;
	struct sunxi_otgc_ep		*dep;
	int			ret;
	u8			speed;

	dev_vdbg(otgc->dev, "%s\n", __func__);

	memset(&params, 0x00, sizeof(params));

	speed = sunxi_get_speed(otgc->regs);
	otgc->speed = speed;

	sunxi_update_ram_clk_sel(otgc, speed);

	switch (speed) {
	case SUNXI_DCFG_SUPERSPEED:
		if (sunxi_is_revision_190A(otgc->revision))
			sunxi_gadget_reset_interrupt(otgc);

		sunxi_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);
		otgc->gadget.ep0->maxpacket = 512;
		otgc->gadget.speed = USB_SPEED_SUPER;
		break;
	case SUNXI_DCFG_HIGHSPEED:
		sunxi_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		otgc->gadget.ep0->maxpacket = 64;
		otgc->gadget.speed = USB_SPEED_HIGH;
		break;
	case SUNXI_DCFG_FULLSPEED2:
	case SUNXI_DCFG_FULLSPEED1:
		sunxi_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		otgc->gadget.ep0->maxpacket = 64;
		otgc->gadget.speed = USB_SPEED_FULL;
		break;
	case SUNXI_DCFG_LOWSPEED:
		sunxi_gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(8);
		otgc->gadget.ep0->maxpacket = 8;
		otgc->gadget.speed = USB_SPEED_LOW;
		break;
	}

	/* Disable unneded PHY */
	sunxi_gadget_disable_phy(otgc, otgc->gadget.speed);

	dep = otgc->eps[0];
	ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
	if (ret) {
		dev_err(otgc->dev, "failed to enable %s\n", dep->name);
		return;
	}

	dep = otgc->eps[1];
	ret = __sunxi_gadget_ep_enable(dep, &sunxi_gadget_ep0_desc, NULL);
	if (ret) {
		dev_err(otgc->dev, "failed to enable %s\n", dep->name);
		return;
	}
}

static void sunxi_gadget_wakeup_interrupt(struct sunxi_otgc *otgc)
{
	dev_vdbg(otgc->dev, "%s\n", __func__);

	/*
	 * TODO take core out of low power mode when that's
	 * implemented.
	 */

	otgc->gadget_driver->resume(&otgc->gadget);
}

static void sunxi_gadget_linksts_change_interrupt(struct sunxi_otgc *otgc,
		unsigned int evtinfo)
{
	enum sunxi_link_state	next = evtinfo & SUNXI_LINK_STATE_MASK;

	if (sunxi_is_revision_183A(otgc->revision)) {
		if (next == SUNXI_LINK_STATE_U0) {

			switch (otgc->link_state) {
			case SUNXI_LINK_STATE_U1:
			case SUNXI_LINK_STATE_U2:
				otgc->u1u2 = sunxi_link_state_u1u2(otgc->regs, otgc->u1u2);
				break;
			default:
				/* do nothing */
				break;
			}
		}
	}

	otgc->link_state = next;

	dev_vdbg(otgc->dev, "%s link %d\n", __func__, otgc->link_state);
}

static void sunxi_gadget_interrupt(struct sunxi_otgc *otgc,
		const struct sunxi_otgc_event_devt *event)
{
	switch (event->type) {
	case SUNXI_DEVICE_EVENT_DISCONNECT:
		sunxi_gadget_disconnect_interrupt(otgc);
		break;
	case SUNXI_DEVICE_EVENT_RESET:
		sunxi_gadget_reset_interrupt(otgc);
		break;
	case SUNXI_DEVICE_EVENT_CONNECT_DONE:
		sunxi_gadget_conndone_interrupt(otgc);
		break;
	case SUNXI_DEVICE_EVENT_WAKEUP:
		sunxi_gadget_wakeup_interrupt(otgc);
		break;
	case SUNXI_DEVICE_EVENT_LINK_STATUS_CHANGE:
		sunxi_gadget_linksts_change_interrupt(otgc, event->event_info);
		break;
	case SUNXI_DEVICE_EVENT_EOPF:
		dev_vdbg(otgc->dev, "End of Periodic Frame\n");
		break;
	case SUNXI_DEVICE_EVENT_SOF:
		dev_vdbg(otgc->dev, "Start of Periodic Frame\n");
		break;
	case SUNXI_DEVICE_EVENT_ERRATIC_ERROR:
		dev_vdbg(otgc->dev, "Erratic Error\n");
		break;
	case SUNXI_DEVICE_EVENT_CMD_CMPL:
		dev_vdbg(otgc->dev, "Command Complete\n");
		break;
	case SUNXI_DEVICE_EVENT_OVERFLOW:
		dev_vdbg(otgc->dev, "Overflow\n");
		break;
	default:
		dev_dbg(otgc->dev, "UNKNOWN IRQ %d\n", event->type);
	}
}

static void sunxi_process_event_entry(struct sunxi_otgc *otgc,
		const union sunxi_event *event)
{
	/* Endpoint IRQ, handle it and return early */
	if (event->type.is_devspec == 0) {
		/* depevt */
		return sunxi_endpoint_interrupt(otgc, &event->depevt);
	}

	switch (event->type.type) {
	case SUNXI_EVENT_TYPE_DEV:
		sunxi_gadget_interrupt(otgc, &event->devt);
		break;
	/* REVISIT what to do with Carkit and I2C events ? */
	default:
		dev_err(otgc->dev, "UNKNOWN IRQ type %d\n", event->raw);
	}
}

static irqreturn_t sunxi_process_event_buf(struct sunxi_otgc *otgc, u32 buf)
{
	struct sunxi_otgc_event_buffer *evt;
	int left;
	u32 count ;

	count = sunxi_get_gevntcount(otgc->regs, buf);
	if (!count)
		return IRQ_NONE;

	evt = otgc->ev_buffs[buf];
	left = count;

	while (left > 0) {
		union sunxi_event event;

		event.raw = *(u32 *) (evt->buf + evt->lpos);

		sunxi_process_event_entry(otgc, &event);
		evt->lpos = (evt->lpos + 4) % SUNXI_EVENT_BUFFERS_SIZE;
		left -= 4;

		sunxi_set_gevntcount(otgc->regs, buf, 4);
	}

	return IRQ_HANDLED;
}

static irqreturn_t sunxi_interrupt(int irq, void *_otgc)
{
	struct sunxi_otgc			*otgc = _otgc;
	int				i;
	irqreturn_t			ret = IRQ_NONE;

	spin_lock(&otgc->lock);

	for (i = 0; i < otgc->num_event_buffers; i++) {
		irqreturn_t status;

		status = sunxi_process_event_buf(otgc, i);
		if (status == IRQ_HANDLED)
			ret = status;
	}

	spin_unlock(&otgc->lock);

	return ret;
}

int sunxi_gadget_disable(struct sunxi_otgc *otgc){

	int 				irq;
	unsigned long		flags;

	spin_lock_irqsave(&otgc->lock, flags);

	sunxi_gadget_run_stop(otgc, 0);
	sunxi_gadget_ep0_disable_ex(otgc);

	sunxi_disconnect_gadget(otgc);

	irq = platform_get_irq(to_platform_device(otgc->dev), 0);
	disable_sunxi_all_irq(otgc->regs);

	free_irq(irq, otgc);

	spin_unlock_irqrestore(&otgc->lock, flags);
	
	return 0;

}
int sunxi_gadget_enable(struct sunxi_otgc *otgc){

	int 				irq;
	int 				ret;

	irq = platform_get_irq(to_platform_device(otgc->dev), 0);
	ret = request_irq(irq, sunxi_interrupt, IRQF_SHARED,
			"sunxi", otgc);
	if (ret) {
		pr_err("failed to request irq #%d --> %d\n",
				irq, ret);
	}

	sunxi_gadget_enable_all_irq(otgc->regs);

	sunxi_gadget_ep0_enable_ex(otgc);
	sunxi_gadget_run_stop(otgc, 1);

	return 0;

}

/**
 * sunxi_gadget_init - Initializes gadget related registers
 * @otgc: pointer to our controller context structure
 *
 * Returns 0 on success otherwise negative errno.
 */
int __devinit sunxi_gadget_init(struct sunxi_otgc *otgc)
{
	int					ret;
	int					irq;

	otgc->ctrl_req = dma_alloc_coherent(otgc->dev, sizeof(*otgc->ctrl_req),
			&otgc->ctrl_req_addr, GFP_KERNEL);
	if (!otgc->ctrl_req) {
		dev_err(otgc->dev, "failed to allocate ctrl request\n");
		ret = -ENOMEM;
		goto err0;
	}

	otgc->ep0_trb = dma_alloc_coherent(otgc->dev, sizeof(*otgc->ep0_trb),
			&otgc->ep0_trb_addr, GFP_KERNEL);
	if (!otgc->ep0_trb) {
		dev_err(otgc->dev, "failed to allocate ep0 trb\n");
		ret = -ENOMEM;
		goto err1;
	}

	otgc->setup_buf = kzalloc(sizeof(*otgc->setup_buf) * 2,
			GFP_KERNEL);
	if (!otgc->setup_buf) {
		dev_err(otgc->dev, "failed to allocate setup buffer\n");
		ret = -ENOMEM;
		goto err2;
	}

	otgc->ep0_bounce = dma_alloc_coherent(otgc->dev,
			512, &otgc->ep0_bounce_addr, GFP_KERNEL);
	if (!otgc->ep0_bounce) {
		dev_err(otgc->dev, "failed to allocate ep0 bounce buffer\n");
		ret = -ENOMEM;
		goto err3;
	}

	dev_set_name(&otgc->gadget.dev, "gadget");

	otgc->gadget.ops			= &sunxi_gadget_ops;
	otgc->gadget.max_speed		= USB_SPEED_SUPER;
	otgc->gadget.speed		= USB_SPEED_UNKNOWN;
	otgc->gadget.dev.parent		= otgc->dev;
	otgc->gadget.sg_supported	= true;

	dma_set_coherent_mask(&otgc->gadget.dev, otgc->dev->coherent_dma_mask);

	otgc->gadget.dev.dma_parms	= otgc->dev->dma_parms;
	otgc->gadget.dev.dma_mask	= otgc->dev->dma_mask;
	otgc->gadget.dev.release		= sunxi_gadget_release;
	otgc->gadget.name		= "sunxi-gadget";

	/*
	 * REVISIT: Here we should clear all pending IRQs to be
	 * sure we're starting from a well known location.
	 */

	ret = sunxi_gadget_init_endpoints(otgc);
	if (ret)
		goto err4;

	irq = platform_get_irq(to_platform_device(otgc->dev), 0);

	ret = request_irq(irq, sunxi_interrupt, IRQF_SHARED,
			"sunxi", otgc);
	if (ret) {
		dev_err(otgc->dev, "failed to request irq #%d --> %d\n",
				irq, ret);
		goto err5;
	}

	ret = device_register(&otgc->gadget.dev);
	if (ret) {
		dev_err(otgc->dev, "failed to register gadget device\n");
		put_device(&otgc->gadget.dev);
		goto err6;
	}

	ret = usb_add_gadget_udc(otgc->dev, &otgc->gadget);
	if (ret) {
		dev_err(otgc->dev, "failed to register udc\n");
		goto err7;
	}

	return 0;

err7:
	device_unregister(&otgc->gadget.dev);

err6:
	disable_sunxi_all_irq(otgc->regs);
	free_irq(irq, otgc);

err5:
	sunxi_gadget_free_endpoints(otgc);

err4:
	dma_free_coherent(otgc->dev, 512, otgc->ep0_bounce,
			otgc->ep0_bounce_addr);

err3:
	kfree(otgc->setup_buf);

err2:
	dma_free_coherent(otgc->dev, sizeof(*otgc->ep0_trb),
			otgc->ep0_trb, otgc->ep0_trb_addr);

err1:
	dma_free_coherent(otgc->dev, sizeof(*otgc->ctrl_req),
			otgc->ctrl_req, otgc->ctrl_req_addr);

err0:
	return ret;
}

void sunxi_gadget_exit(struct sunxi_otgc *otgc)
{
	int			irq;

	usb_del_gadget_udc(&otgc->gadget);
	irq = platform_get_irq(to_platform_device(otgc->dev), 0);

	disable_sunxi_all_irq(otgc->regs);

	free_irq(irq, otgc);

	sunxi_gadget_free_endpoints(otgc);

	dma_free_coherent(otgc->dev, 512, otgc->ep0_bounce,
			otgc->ep0_bounce_addr);

	kfree(otgc->setup_buf);

	dma_free_coherent(otgc->dev, sizeof(*otgc->ep0_trb),
			otgc->ep0_trb, otgc->ep0_trb_addr);

	dma_free_coherent(otgc->dev, sizeof(*otgc->ctrl_req),
			otgc->ctrl_req, otgc->ctrl_req_addr);

	device_unregister(&otgc->gadget.dev);
}

int sunxi_gadget_suspend(struct sunxi_otgc *otgc)
{
	unsigned long		flags;

	spin_lock_irqsave(&otgc->lock, flags);

	sunxi_gadget_run_stop(otgc, 0);
	sunxi_gadget_ep0_disable_ex(otgc);
	disable_sunxi_all_irq(otgc->regs);

	spin_unlock_irqrestore(&otgc->lock, flags);

	return 0;
}

int sunxi_gadget_resume(struct sunxi_otgc *otgc)
{
	/* Enable all but Start and End of Frame IRQs */
	sunxi_gadget_enable_all_irq(otgc->regs);
	sunxi_gadget_ep0_enable_ex(otgc);
	sunxi_gadget_run_stop(otgc, 1);

	return 0;
}
