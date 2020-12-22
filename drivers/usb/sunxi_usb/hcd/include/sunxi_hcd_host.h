/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_host.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_HCD_HOST_H__
#define  __SUNXI_HCD_HOST_H__

enum fifo_style { FIFO_RXTX = 0, FIFO_TX, FIFO_RX } __attribute__ ((packed));
enum buf_mode { BUF_SINGLE = 0, BUF_DOUBLE } __attribute__ ((packed));

struct fifo_cfg {
	u8		hw_ep_num;
	enum fifo_style	style;
	enum buf_mode	mode;
	u16		maxpacket;
};
static inline struct usb_hcd *sunxi_hcd_to_hcd(struct sunxi_hcd *sunxi_hcd)
{
	return container_of((void *) sunxi_hcd, struct usb_hcd, hcd_priv);
}

static inline struct sunxi_hcd *hcd_to_sunxi_hcd(struct usb_hcd *hcd)
{
	return (struct sunxi_hcd *) (hcd->hcd_priv);
}

/* stored in "usb_host_endpoint.hcpriv" for scheduled endpoints */
typedef struct sunxi_hcd_qh{
	struct usb_host_endpoint *hep;  /* usbcore info                 */
	struct usb_device  *dev;        /* usb device                   */
	struct sunxi_hcd_hw_ep  *hw_ep;	/* current binding              */

	struct list_head  ring;		/* of sunxi_hcd_qh              */
	struct sunxi_hcd_qh *next;	/* for periodic tree            */
	u8  mux;		        /* qh multiplexed to hw_ep      */
	unsigned  offset;		/* in urb->transfer_buffer      */
	unsigned  segsize;	        /* current xfer fragment        */

	u8  type_reg;	                /* {rx,tx} type register        */
	u8  intv_reg;	                /* {rx,tx} interval register    */
	u8  addr_reg;	                /* device address register      */
	u8  h_addr_reg;	                /* hub address register         */
	u8  h_port_reg;	                /* hub port register            */

	u8  is_ready;	                /* safe to modify hw_ep         */
	u8  type;		        /* ep type XFERTYPE_*           */
	u8  epnum;                      /* target ep index. the peripheral device's ep */
	u16  maxpacket;                 /* max packet size              */
	u16  frame;		        /* for periodic schedule        */
	unsigned  iso_idx;	        /* in urb->iso_frame_desc[]     */

	u32 dma_working;		/* flag. dma working flag 	*/
	u32 dma_transfer_len;		/* flag. dma transfer length 	*/
	u8  PacketCount;
}sunxi_hcd_qh_t;

/* map from control or bulk queue head to the first qh on that ring */
static inline struct sunxi_hcd_qh *first_qh(struct list_head *q)
{
	if (q == NULL) {
		DMSG_WRN("ERR: invalid argment\n");
		return NULL;
	}

	if (list_empty(q)) {
		DMSG_WRN("ERR: invalid argment\n");
		return NULL;
	}

	return list_entry(q->next, struct sunxi_hcd_qh, ring);
}

/* get next urb */
static inline struct urb *next_urb(struct sunxi_hcd_qh *qh)
{
	struct list_head *queue;

	if (!qh) {
		DMSG_WRN("ERR: invalid argment\n");
		return NULL;
	}

	queue = &qh->hep->urb_list;
	if(queue == NULL || (u32)queue < 0x7fffffff){
		return 0;
	}

	if (list_empty(queue) || queue->next == NULL) {
		DMSG_WRN("ERR: list is empty, queue->next = 0x%p\n", queue->next);
		return NULL;
	}

	return list_entry(queue->next, struct urb, urb_list);
}

irqreturn_t sunxi_hcd_h_ep0_irq(struct sunxi_hcd *sunxi_hcd);

int sunxi_hcd_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags);
int sunxi_hcd_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status);
void sunxi_hcd_h_disable(struct usb_hcd *hcd, struct usb_host_endpoint *hep);
int fifo_setup(struct sunxi_hcd *sunxi_hcd,
                     struct sunxi_hcd_hw_ep *hw_ep,
                     const struct fifo_cfg *cfg,
                     u16 offset);

int sunxi_hcd_h_start(struct usb_hcd *hcd);
void sunxi_hcd_h_stop(struct usb_hcd *hcd);

int sunxi_hcd_h_get_frame_number(struct usb_hcd *hcd);
int sunxi_hcd_bus_suspend(struct usb_hcd *hcd);
int sunxi_hcd_bus_resume(struct usb_hcd *hcd);

void sunxi_hcd_host_rx(struct sunxi_hcd *sunxi_hcd, u8 epnum);
void sunxi_hcd_host_tx(struct sunxi_hcd *sunxi_hcd, u8 epnum);

#define  USB_SET_VBUS_ON                   1
#define  USB_SET_VBUS_OFF                  0

#endif   //__SUNXI_HCD_HOST_H__

