/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Emma <liujuan1@reuuimllatech.com>
 *
 * Driver of Allwinner UART controller.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2013.6.6 Mintow <duanmintao@allwinnertech.com>
 *    Adapt to support sun8i/sun9i of Allwinner.
 */

#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>

#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>

#include <linux/clk.h>
#include <linux/clk-provider.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/version.h>
#include "sunxi-uart-ng.h"

void sunxi_uart_stop_dma_rx(struct sunxi_uart_port *uart_port)
{
	struct sunxi_uart_dma *uart_dma = uart_port->dma;

	if (uart_dma && uart_dma->rx_dma_used) {
		dmaengine_terminate_all(uart_dma->dma_chan_rx);
		uart_dma->rb_tail = 0;
		uart_dma->rx_dma_used = 0;
	}
}

void sunxi_uart_stop_dma_tx(struct sunxi_uart_port *uart_port)
{
	struct sunxi_uart_dma *uart_dma = uart_port->dma;

	if (uart_dma && uart_dma->tx_dma_used) {
		dmaengine_terminate_all(uart_dma->dma_chan_tx);
		uart_dma->tx_dma_used = 0;
	}
}

void sunxi_uart_dma_rx_callback(void *arg)
{
	int count, flip, total;
	struct dma_tx_state state;
	struct sunxi_uart_port *uart_port = arg;
	struct uart_port *port = &uart_port->port;
	struct sunxi_uart_dma *uart_dma = uart_port->dma;

	if (!uart_dma->rx_dma_used || !port->state->port.tty) {
		SERIAL_DBG(port->dev, "!uart_dma->rx_dma_used || !port->state->port.tty\n");
		return;
	}
	SERIAL_DBG(port->dev, "uart_rx callback\n");
	/* TODO: add trace event */
	dmaengine_tx_status(uart_dma->dma_chan_rx,
			uart_dma->rx_cookie, &state);

	SERIAL_DBG(port->dev, "dma left data:%d\n", state.residue);
	uart_dma->rb_head = uart_dma->rb_size - state.residue;

	total = CIRC_CNT(uart_dma->rb_head, uart_dma->rb_tail,
			uart_dma->rb_size);
	count = CIRC_CNT_TO_END(uart_dma->rb_head, uart_dma->rb_tail,
			uart_dma->rb_size);
	SERIAL_DBG(port->dev, "total data:%d count data:%d\n", total, count);
	if (!total) {
		SERIAL_DBG(port->dev, "dma data is zero\n");
		return;
	}
	flip = tty_insert_flip_string(&port->state->port,
			uart_dma->rx_buffer + uart_dma->rb_tail, count);

	if (total > count) {
		flip += tty_insert_flip_string(&port->state->port,
				uart_dma->rx_buffer, total - count);
	}

	if (unlikely(flip != total))
		sunxi_err(port->dev, "flip data cnt not equel to rev data\n");

	tty_flip_buffer_push(&port->state->port);
	uart_dma->rb_tail =
		(uart_dma->rb_tail + total) & (uart_dma->rb_size - 1);

	port->icount.rx += total;
	return;
}

void sunxi_uart_release_dma_tx(struct sunxi_uart_port *uart_port)
{
	struct sunxi_uart_dma *uart_dma = uart_port->dma;

	if (uart_dma && uart_dma->tx_dma_inited) {
		sunxi_uart_stop_dma_tx(uart_port);
		dma_free_coherent(uart_port->port.dev, uart_port->dma->tb_size,
				uart_port->dma->tx_buffer, uart_port->dma->tx_phy_addr);
		uart_port->port.state->xmit.buf = NULL;
		dma_release_channel(uart_dma->dma_chan_tx);
		uart_dma->dma_chan_tx = NULL;
		uart_dma->tx_dma_inited = 0;
	}
}

int sunxi_uart_init_dma_tx(struct sunxi_uart_port *uart_port)
{
	struct dma_slave_config slave_config;
	struct uart_port *port = &uart_port->port;
	struct sunxi_uart_dma *uart_dma = uart_port->dma;
	int ret;

	sunxi_info(uart_port->port.dev, "sunxi_uart_init_dma_tx begin\n");

	if (!uart_dma) {
		sunxi_info(uart_port->port.dev, "sunxi_uart_init_dma_tx fail\n");
		return -1;
	}

	if (uart_dma->tx_dma_inited) {
		sunxi_info(uart_port->port.dev, "uart_dma->tx_dma_inited\n");
		return 0;
	}

	uart_dma->dma_chan_tx = dma_request_chan(uart_port->port.dev, "tx");
	if (!uart_dma->dma_chan_tx) {
		sunxi_err(port->dev, "cannot get the TX DMA channel!\n");
		ret = -EINVAL;
	}

	slave_config.direction = DMA_MEM_TO_DEV;
	slave_config.dst_addr = port->mapbase + SUNXI_UART_THR;
	slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_config.src_maxburst = 1;
	slave_config.dst_maxburst = 1;
	ret = dmaengine_slave_config(uart_dma->dma_chan_tx, &slave_config);
	if (ret) {
		sunxi_err(port->dev, "error in TX dma configuration.");
		return ret;
	}

	uart_dma->tx_dma_inited = 1;
	sunxi_info(port->dev, "sunxi_uart_init_dma_tx sucess\n");
	return 0;
}

void dma_tx_callback(void *data)
{
	struct uart_port *port = data;
	struct sunxi_uart_port *uart_port = container_of(port,
			struct sunxi_uart_port, port);
	struct circ_buf *xmit = &port->state->xmit;
	struct sunxi_uart_dma *uart_dma = uart_port->dma;
	struct scatterlist *sgl = &uart_dma->tx_sgl;
	unsigned long flags;

	sunxi_info(port->dev, "dma_tx_callback\n");

	spin_lock_irqsave(&port->lock, flags);
	dma_unmap_sg(uart_port->port.dev, sgl, 1, DMA_TO_DEVICE);

	xmit->tail = (xmit->tail + uart_dma->tx_bytes) & (UART_XMIT_SIZE - 1);
	port->icount.tx += uart_dma->tx_bytes;
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	sunxi_uart_enable_ier_thri(port);
	uart_dma->tx_dma_used = 0;
	spin_unlock_irqrestore(&port->lock, flags);
}

int sunxi_uart_start_dma_tx(struct sunxi_uart_port *uart_port)
{
	int count = 0;
	struct uart_port *port = &uart_port->port;
	struct circ_buf *xmit = &port->state->xmit;
	struct sunxi_uart_dma *uart_dma = uart_port->dma;
	struct scatterlist *sgl = &uart_dma->tx_sgl;
	struct dma_async_tx_descriptor *desc;
	int ret;

	if (!uart_dma->use_dma)
		goto err_out;

	if (-1 == sunxi_uart_init_dma_tx(uart_port))
		goto err_out;

	if (1 == uart_dma->tx_dma_used)
		return 1;

	uart_dma->tx_dma_used = 1;
	isb();
	/**********************************/
	/* mask the stop now */
	sunxi_uart_disable_ier_thri(port);

	count = SERIAL_CIRC_CNT_TO_END(xmit);
	uart_dma->tx_bytes = count;
	sg_init_one(sgl, phys_to_virt(uart_dma->tx_phy_addr) + xmit->tail, count);
	ret = dma_map_sg(port->dev, sgl, 1, DMA_TO_DEVICE);

	if (ret == 0) {
		sunxi_err(port->dev, "DMA mapping error for TX.\n");
		return -1;
	}
	desc = dmaengine_prep_slave_sg(uart_dma->dma_chan_tx, sgl, 1,
			DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT);

	if (!desc) {
		sunxi_err(port->dev, "We cannot prepare for the TX slave dma!\n");
		return -1;
	}
	desc->callback = dma_tx_callback;
	desc->callback_param = port;
	dmaengine_submit(desc);
	dma_async_issue_pending(uart_dma->dma_chan_tx);
	return 1;
err_out:
	sunxi_info(uart_port->port.dev, "-sunxi_uart_start_dma_tx-error-\n");
	return -1;
}

void sunxi_uart_release_dma_rx(struct sunxi_uart_port *uart_port)
{
	struct sunxi_uart_dma *uart_dma;
	uart_dma = uart_port->dma;

	if (uart_dma && uart_dma->rx_dma_inited) {
		sunxi_uart_stop_dma_rx(uart_port);
		dma_free_coherent(uart_port->port.dev, uart_port->dma->rb_size,
				uart_port->dma->rx_buffer, uart_port->dma->rx_phy_addr);
		dma_release_channel(uart_dma->dma_chan_rx);
		uart_dma->dma_chan_rx = NULL;
		uart_dma->rx_dma_inited = 0;
	}
}

int sunxi_uart_init_dma_rx(struct sunxi_uart_port *uart_port)
{
	int ret;
	struct uart_port *port = &uart_port->port;
	struct dma_slave_config slave_config;
	struct sunxi_uart_dma *uart_dma = uart_port->dma;
	struct sunxi_dma_desc *sunxi_desc = &uart_dma->sunxi_desc;

	SERIAL_DBG(port->dev, "sunxi_uart_init_dma_rx\n");

	if (!uart_dma) {
		sunxi_info(port->dev, "sunxi_uart_init_dma_rx: port fail\n");
		return -1;
	}

	if (uart_dma->rx_dma_inited) {
		SERIAL_DBG(port->dev, "uart_dma->rx_dma_inited\n");
		return 0;
	}

	uart_dma->dma_chan_rx = dma_request_chan(uart_port->port.dev, "rx");
	if (IS_ERR_OR_NULL(uart_dma->dma_chan_rx)) {
		sunxi_err(port->dev, "cannot get the DMA channel.\n");
		return -1;
	}

	sunxi_desc->is_bmode = 1;
	sunxi_desc->is_timeout = 1;
	sunxi_desc->timeout_steps = 100;
	sunxi_desc->timeout_fun = 0x0; /* dma pending, no other option */
	sunxi_desc->callback = sunxi_uart_dma_rx_callback;
	sunxi_desc->callback_param = uart_port;

	uart_dma->dma_chan_rx->private = sunxi_desc;

	slave_config.direction = DMA_DEV_TO_MEM;
	slave_config.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_config.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	slave_config.src_maxburst = 1;
	slave_config.dst_maxburst = 1;
	slave_config.src_addr = port->mapbase + SUNXI_UART_RBR;

	ret = dmaengine_slave_config(uart_dma->dma_chan_rx, &slave_config);
	if (ret) {
		sunxi_err(port->dev, "error in RX dma configuration.\n");
		return ret;
	}

	uart_dma->rx_dma_inited = 1;
	sunxi_info(port->dev, "sunxi_uart_init_dma_rx sucess\n");
	return 0;
}

int sunxi_uart_start_dma_rx(struct sunxi_uart_port *uart_port)
{
	struct uart_port *port = &uart_port->port;
	struct sunxi_uart_dma *uart_dma = uart_port->dma;
	struct dma_async_tx_descriptor *desc;

	SERIAL_DBG(port->dev, "enter  sunxi_uart_start_dma_rx\n");

	if (!uart_dma->use_dma) {
		SERIAL_DBG(port->dev, "uart_dma->use_dma not set\n");
		return 0;
	}

	if (uart_dma->rx_dma_used == 1) {
		SERIAL_DBG(port->dev, "uart_dma->rx_dma_used == 1\n");
		return 0;
	}

	if (-1 == sunxi_uart_init_dma_rx(uart_port)) {
		sunxi_info(uart_port->port.dev, "sunxi_uart_init_dma_rx error!\n");
		return -1;
	}
	desc = dmaengine_prep_dma_cyclic(uart_dma->dma_chan_rx,
			uart_dma->rx_phy_addr,
			uart_dma->rb_size, 1024,
			DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT);

	if (!desc) {
		sunxi_err(port->dev, "get rx dma descriptor failed!\n");
		return -EINVAL;
	}

	sunxi_debug(port->dev, "RX: prepare for the DMA.\n");
	desc->callback = sunxi_uart_dma_rx_callback;
	desc->callback_param = uart_port;
	uart_dma->rx_cookie = dmaengine_submit(desc);
	dma_async_issue_pending(uart_dma->dma_chan_rx);

	uart_dma->rx_dma_used = 1;

	return 1;
}

int sunxi_uart_init_dma(struct sunxi_uart_port *uart_port)
{
	if (uart_port->dma->use_dma & RX_DMA) {
		/* rx buffer */
		uart_port->dma->rb_size = DMA_SERIAL_BUFFER_SIZE;
		uart_port->dma->rx_buffer = dma_alloc_coherent(
				uart_port->port.dev, uart_port->dma->rb_size,
				&uart_port->dma->rx_phy_addr, GFP_KERNEL);
		uart_port->dma->rb_tail = 0;

		if (!uart_port->dma->rx_buffer) {
			sunxi_err(uart_port->port.dev,
				"dmam_alloc_coherent dma_rx_buffer fail\n");
			return -ENOMEM;
		} else {
			sunxi_info(uart_port->port.dev,
				"dma_rx_buffer %p\n", uart_port->dma->rx_buffer);
			sunxi_info(uart_port->port.dev,
		"dma_rx_phy 0x%08x\n", (unsigned)uart_port->dma->rx_phy_addr);
		}
		sunxi_uart_init_dma_rx(uart_port);
	}

	if (uart_port->dma->use_dma & TX_DMA) {
		/* tx buffer */
		uart_port->dma->tb_size = UART_XMIT_SIZE;
		uart_port->dma->tx_buffer = dma_alloc_coherent(
				uart_port->port.dev, uart_port->dma->tb_size,
				&uart_port->dma->tx_phy_addr, GFP_KERNEL);
		if (!uart_port->dma->tx_buffer) {
			sunxi_info(uart_port->port.dev,
				"dmam_alloc_coherent dma_tx_buffer fail\n");
			return -ENOMEM;
		} else {
			sunxi_info(uart_port->port.dev, "dma_tx_buffer %p\n",
						uart_port->dma->tx_buffer);
			sunxi_info(uart_port->port.dev, "dma_tx_phy 0x%08x\n",
					(unsigned) uart_port->dma->tx_phy_addr);
		}
		sunxi_uart_init_dma_tx(uart_port);
	}
	return 0;
}
