/*
 * drivers/usb/sunxi_usb/hcd/include/sunxi_hcd_dma.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver, dma ops.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef  __SUNXI_HCD_DMA_H__
#define  __SUNXI_HCD_DMA_H__

#if 1
#define  is_hcd_support_dma(usbc_no)   0
#else
#define  is_hcd_support_dma(usbc_no)    (usbc_no == 0)
#endif

/* the condition to use DMA: 1 - more than one packet, 2 - DMA idle, 3 - not ep0 */
#define  is_sunxi_hcd_dma_capable(usbc_no, len, maxpacket, epnum)	(is_hcd_support_dma(usbc_no) \
										&& (len > maxpacket) \
										&& epnum)

typedef struct sunxi_hcd_dma{
	char name[32];
	//struct sunxi_dma_client dma_client;

	int dma_hdle;	/* dma handle */
}sunxi_hcd_dma_t;

void sunxi_hcd_switch_bus_to_dma(struct sunxi_hcd_qh *qh, u32 is_in);
void sunxi_hcd_switch_bus_to_pio(struct sunxi_hcd_qh *qh, __u32 is_in);

void sunxi_hcd_dma_set_config(struct sunxi_hcd_qh *qh, __u32 buff_addr, __u32 len);
__u32 sunxi_hcd_dma_is_busy(struct sunxi_hcd_qh *qh);

void sunxi_hcd_dma_start(struct sunxi_hcd_qh *qh, __u32 fifo, __u32 buffer, __u32 len);
void sunxi_hcd_dma_stop(struct sunxi_hcd_qh *qh);
__u32 sunxi_hcd_dma_transmit_length(struct sunxi_hcd_qh *qh, __u32 is_in, __u32 buffer_addr);

__s32 sunxi_hcd_dma_probe(struct sunxi_hcd *sunxi_hcd);
__s32 sunxi_hcd_dma_remove(struct sunxi_hcd *sunxi_hcd);

#endif   //__SUNXI_HCD_DMA_H__

