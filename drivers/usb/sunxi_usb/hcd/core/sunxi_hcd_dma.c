/*
 * drivers/usb/sunxi_usb/hcd/core/sunxi_hcd_dma.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * javen, 2010-12-20, create this file
 *
 * usb host contoller driver. dma ops.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include  "../include/sunxi_hcd_core.h"
#include  "../include/sunxi_hcd_dma.h"
#include <asm/cacheflush.h>

/* switch USB bus to DMA */
void sunxi_hcd_switch_bus_to_dma(struct sunxi_hcd_qh *qh, u32 is_in)
{
	DMSG_DBG_DMA("sunxi_hcd_switch_bus_to_dma\n");
}
EXPORT_SYMBOL(sunxi_hcd_switch_bus_to_dma);

/* switch USB bus to PIO */
void sunxi_hcd_switch_bus_to_pio(struct sunxi_hcd_qh *qh, __u32 is_in)
{
}
EXPORT_SYMBOL(sunxi_hcd_switch_bus_to_pio);

/* enable DMA channel irq */
static void sunxi_hcd_enable_dma_channel_irq(struct sunxi_hcd_qh *qh)
{
	DMSG_DBG_DMA("sunxi_hcd_enable_dma_channel_irq\n");
}
EXPORT_SYMBOL(sunxi_hcd_enable_dma_channel_irq);

/* disable DMA channel irq */
static void sunxi_hcd_disable_dma_channel_irq(struct sunxi_hcd_qh *qh)
{
	DMSG_DBG_DMA("sunxi_hcd_disable_dma_channel_irq\n");
}
EXPORT_SYMBOL(sunxi_hcd_disable_dma_channel_irq);

/* config DMA */
void sunxi_hcd_dma_set_config(struct sunxi_hcd_qh *qh, __u32 buff_addr, __u32 len)
{
}
EXPORT_SYMBOL(sunxi_hcd_dma_set_config);

/* start DMA */
void sunxi_hcd_dma_start(struct sunxi_hcd_qh *qh, __u32 fifo, __u32 buffer, __u32 len)
{
}
EXPORT_SYMBOL(sunxi_hcd_dma_start);

/* stop DMA transfer */
void sunxi_hcd_dma_stop(struct sunxi_hcd_qh *qh)
{
}
EXPORT_SYMBOL(sunxi_hcd_dma_stop);

/* query length that has been transferred by DMA */
#if 0
static __u32 sunxi_hcd_dma_left_length(struct sunxi_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{
	return 0;
}

__u32 sunxi_hcd_dma_transmit_length(struct sunxi_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{
	return 0;
}
#else
__u32 sunxi_hcd_dma_transmit_length(struct sunxi_hcd_qh *qh, __u32 is_in, __u32 buffer_addr)
{
	return 0;
}
#endif

EXPORT_SYMBOL(sunxi_hcd_dma_transmit_length);

/* check if dma busy */
__u32 sunxi_hcd_dma_is_busy(struct sunxi_hcd_qh *qh)
{
	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_dma_is_busy);

/* dma probe */
__s32 sunxi_hcd_dma_probe(struct sunxi_hcd *sunxi_hcd)
{
	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_dma_probe);

/* dma remove */
__s32 sunxi_hcd_dma_remove(struct sunxi_hcd *sunxi_hcd)
{
	return 0;
}
EXPORT_SYMBOL(sunxi_hcd_dma_remove);

