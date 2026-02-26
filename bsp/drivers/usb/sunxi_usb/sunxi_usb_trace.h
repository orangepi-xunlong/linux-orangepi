/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * lijingpsw <lijingpsw@allwinnertech.com>
 *
 * sunxi_udc_trace.h - sunxi_udc Trace Support
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM sunxi_udc

#if !defined(__SUNXI_UDC_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define __SUNXI_UDC_TRACE_H

#include <linux/types.h>
#include <linux/tracepoint.h>
#include <linux/usb.h>
#include "udc/sunxi_udc_config.h"
#include  "udc/sunxi_udc_dma.h"

DECLARE_EVENT_CLASS(sunxi_udc_req,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep),
	TP_STRUCT__entry(
		__field(struct usb_request *, req)
		__field(u8, is_tx)
		__field(u8, epnum)
		__field(unsigned, actual)
		__field(unsigned, length)
		__field(int, status)
	),
	TP_fast_assign(
		__entry->req = &req->req;
		__entry->is_tx = ep->bEndpointAddress;
		__entry->epnum = ep->num;
		__entry->actual = req->req.actual;
		__entry->length = req->req.length;
		__entry->status = req->req.status;
	),
	TP_printk("req(%p, %d/%d) ep(num:%d, %s), status %d",
			__entry->req, __entry->actual, __entry->length,
			__entry->epnum, __entry->is_tx ? "tx/IN" : "rx/OUT",
			__entry->status
	)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_queue,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_dequeue,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_handle_ep,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_write_fifo,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_read_fifo,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, pio_write_fifo,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, pio_read_fifo,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_done,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DECLARE_EVENT_CLASS(sunxi_udc_dma,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep),
	TP_STRUCT__entry(
		__field(struct usb_request *, req)
		__field(u8, is_tx)
		__field(u8, epnum)
		__field(unsigned, actual)
		__field(unsigned, length)
		__field(int, status)
		__field(u32, dma_working)
		__field(u32, dma_transfer_len)
	),
	TP_fast_assign(
		__entry->req = &req->req;
		__entry->is_tx = ep->bEndpointAddress & USB_DIR_IN;
		__entry->epnum = ep->num;
		__entry->actual = req->req.actual;
		__entry->length = req->req.length;
		__entry->status = req->req.status;
		__entry->dma_working = ep->dma_working;
		__entry->dma_transfer_len = ep->dma_transfer_len;
	),
	TP_printk("req(%p, %d/%d), ep(num:%d, %s), dam(working:%d, len:%d),, status %d",
			__entry->req, __entry->actual, __entry->length,
			__entry->epnum, __entry->is_tx ? "tx/IN" : "rx/OUT",
			__entry->dma_working, __entry->dma_transfer_len,
			__entry->status
	)
);

DEFINE_EVENT(sunxi_udc_dma, dma_write_fifo,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_dma, dma_read_fifo,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);


DEFINE_EVENT(sunxi_udc_dma, dma_got_short_pkt,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DEFINE_EVENT(sunxi_udc_req, sunxi_udc_dma_completion,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep),
	TP_ARGS(req, ep)
);

DECLARE_EVENT_CLASS(sunxi_udc_dma_chan,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep, dma_channel_t *pchan),
	TP_ARGS(req, ep, pchan),
	TP_STRUCT__entry(
		__field(struct usb_request *, req)
		__field(u8, is_tx)
		__field(u8, epnum)
		__field(unsigned, actual)
		__field(unsigned, length)
		__field(int, status)
		__field(u32, channel_num)
		__field(u32, dma_working)
		__field(u32, dma_transfer_len)
	),
	TP_fast_assign(
		__entry->req = &req->req;
		__entry->is_tx = ep->bEndpointAddress;
		__entry->epnum = ep->num;
		__entry->actual = req->req.actual;
		__entry->length = req->req.length;
		__entry->status = req->req.status;
		__entry->channel_num = pchan->channel_num;
		__entry->dma_working = ep->dma_working;
		__entry->dma_transfer_len = ep->dma_transfer_len;
	),
	TP_printk("req(%p, %d/%d), ep(num: %d,  %s), dam(chan:%d, working:%d, len:%d), status %d",
			__entry->req, __entry->actual, __entry->length,
			__entry->epnum, __entry->is_tx ? "tx/IN" : "rx/OUT",
			__entry->channel_num, __entry->dma_working,
			__entry->dma_transfer_len,
			__entry->status
	)
);

DEFINE_EVENT(sunxi_udc_dma_chan, sunxi_udc_dma_set_config,
	TP_PROTO(struct sunxi_udc_request *req, struct sunxi_udc_ep *ep, dma_channel_t *pchan),
	TP_ARGS(req, ep, pchan)
);

TRACE_EVENT(sunxi_udc_irq,
	TP_PROTO(int usb_irq, int tx_irq, int rx_irq, int dma_irq),
	TP_ARGS(usb_irq, tx_irq, rx_irq, dma_irq),
	TP_STRUCT__entry(
		__field(int, usb_irq)
		__field(int, tx_irq)
		__field(int, rx_irq)
		__field(int, dma_irq)
	),
	TP_fast_assign(
		__entry->usb_irq = usb_irq;
		__entry->tx_irq = tx_irq;
		__entry->rx_irq = rx_irq;
		__entry->dma_irq = dma_irq;
	),
	TP_printk("irq:%02x, tx_irq:%02x, rx_irq:%02x, dma_irq:%02x",
		__entry->usb_irq, __entry->tx_irq,
		__entry->rx_irq, __entry->dma_irq
	)
);

#endif /* __SUNXI_UDC_TRACE_H */

/* this part has to be here */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .

#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE sunxi_usb_trace

#include <trace/define_trace.h>
