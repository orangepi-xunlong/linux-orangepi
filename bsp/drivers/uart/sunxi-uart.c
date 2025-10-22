/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * (C) Copyright 2007-2013
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Aaron.Maoye <leafy.myeh@reuuimllatech.com>
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
#if defined(CONFIG_AW_SERIAL_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
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
#include "sunxi-uart.h"

/* #define CONFIG_SW_UART_DUMP_DATA */
/*
 * ********************* Note **********************
 * CONFIG_SW_UART_DUMP_DATA may cause some problems
 * with some commands of 'dmesg', 'logcat', and
 * 'cat /proc/kmsg' in the console. This problem may
 * cause kernel to dead. These commands will work fine
 * in the adb shell. So you must be very clear with
 * this problem if you want define this macro to debug.
 */

/* debug control */

static inline bool sw_is_console_port(struct uart_port *port)
{
	return port->cons && port->cons->index == port->line;
}

#define SERIAL_DBG(dev, fmt, arg...)	\
			do { \
				if (!sw_is_console_port(&sw_uport->port)) \
					sunxi_debug(dev, "%s()%d - "fmt, __func__, __LINE__, ##arg); \
			} while (0)

#define TX_DMA		1
#define RX_DMA		2
#define DMA_SERIAL_BUFFER_SIZE	(PAGE_SIZE)
#define SERIAL_CIRC_CNT_TO_END(xmit) \
	CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE)

enum uart_time_use {
	UART_NO_USE_TIMER = 0,
	UART_USE_TIMER,
};

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
static void sw_uart_stop_dma_tx(struct sw_uart_port *sw_uport);
static void sw_uart_release_dma_tx(struct sw_uart_port *sw_uport);
static int sw_uart_init_dma_tx(struct sw_uart_port *sw_uport);
static void dma_tx_callback(void *data);
static int sw_uart_start_dma_tx(struct sw_uart_port *sw_uport);
static void sw_uart_stop_dma_tx(struct sw_uart_port *sw_uport);
static void sw_uart_release_dma_rx(struct sw_uart_port *sw_uport);
static int sw_uart_init_dma_rx(struct sw_uart_port *sw_uport);
static int sw_uart_start_dma_rx(struct sw_uart_port *sw_uport);
static void sw_uart_update_rb_addr(struct sw_uart_port *sw_uport);
static enum hrtimer_restart  sw_uart_report_dma_rx(struct hrtimer *rx_hrtimer);
#endif

#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
static void sw_uart_dump_data(struct sw_uart_port *sw_uport, char *prompt)
{
	int i, j;
	int head = 0;
	char *buf = sw_uport->dump_buff;
	u32 len = sw_uport->dump_len;
	static char pbuff[128];
	u32 idx = 0;

	BUG_ON(sw_uport->dump_len > MAX_DUMP_SIZE);
	BUG_ON(!sw_uport->dump_buff);
	#define MAX_DUMP_PER_LINE	(16)
	#define MAX_DUMP_PER_LINE_HALF	(MAX_DUMP_PER_LINE >> 1)
	printk(KERN_DEBUG "%s len %d\n", prompt, len);
	for (i = 0; i < len;) {
		if ((i & (MAX_DUMP_PER_LINE-1)) == 0) {
			idx += sprintf(&pbuff[idx], "%04x: ", i);
			head = i;
		}
		idx += sprintf(&pbuff[idx], "%02x ", buf[i]&0xff);
		if ((i & (MAX_DUMP_PER_LINE-1)) == MAX_DUMP_PER_LINE-1 || i == len-1) {
			for (j = i-head+1; j < MAX_DUMP_PER_LINE; j++)
				idx += sprintf(&pbuff[idx], "   ");
			idx += sprintf(&pbuff[idx], " |");
			for (j = head; j <= i; j++) {
				if (isascii(buf[j]) && isprint(buf[j]))
					idx += sprintf(&pbuff[idx], "%c", buf[j]);
				else
					idx += sprintf(&pbuff[idx], ".");
			}
			idx += sprintf(&pbuff[idx], "|\n");
			pbuff[idx] = '\0';
			printk(KERN_DEBUG "%s", pbuff);
			idx = 0;
		}
		i++;
	}
	sw_uport->dump_len = 0;
}
#define SERIAL_DUMP(up, ...) do { \
				if (DEBUG_CONDITION) \
					sw_uart_dump_data(up, __VA_ARGS__); \
			} while (0)
#else
#define SERIAL_DUMP(up, ...)	{ up->dump_len = 0; }
#endif

#define UART_TO_SPORT(port)	((struct sw_uart_port *)port)

static inline unsigned char serial_in(struct uart_port *port, int offs)
{
	return readb_relaxed(port->membase + offs);
}

static inline void serial_out(struct uart_port *port, unsigned char value, int offs)
{
	writeb_relaxed(value, port->membase + offs);
}


static inline void sw_uart_reset(struct sw_uart_port *sw_uport)
{
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	reset_control_assert(sw_uport->reset);
	reset_control_deassert(sw_uport->reset);
#endif
}

static void sw_uart_enable_ier_thri(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!(sw_uport->ier & SUNXI_UART_IER_THRI)) {
		sw_uport->ier |= SUNXI_UART_IER_THRI;
		SERIAL_DBG(port->dev, "start tx, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	}
}

static void sw_uart_disable_ier_thri(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (sw_uport->ier & SUNXI_UART_IER_THRI) {
		sw_uport->ier &= ~SUNXI_UART_IER_THRI;
		SERIAL_DBG(port->dev, "stop tx, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	}
}

static bool sunxi_uart_pe_occur(unsigned int lsr)
{
#ifdef SUNXI_UART_PE_ERRATA
	if ((lsr & SUNXI_UART_LSR_PE) && (lsr & SUNXI_UART_LSR_RXFIFOE)) {
		if ((lsr & SUNXI_UART_LSR_BI) || (lsr & SUNXI_UART_LSR_FE))
			return false;
		return true;
	}
	return false;
#else
	return lsr & SUNXI_UART_LSR_PE;
#endif
}

static unsigned int sw_uart_handle_rx(struct sw_uart_port *sw_uport, unsigned int lsr)
{
	unsigned char ch = 0;
	int max_count = 256;
	char flag;

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	if ((sw_uport->dma->use_dma & RX_DMA)) {
		if (lsr & SUNXI_UART_LSR_RXFIFOE) {
			sunxi_info(sw_uport->port.dev, "error:lsr=0x%x\n", lsr);
			lsr = serial_in(&sw_uport->port, SUNXI_UART_LSR);
			return lsr;
		}
		return lsr;
	}
#endif
	do {
		if (likely(lsr & SUNXI_UART_LSR_DR)) {
			ch = serial_in(&sw_uport->port, SUNXI_UART_RBR);
#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
			sw_uport->dump_buff[sw_uport->dump_len++] = ch;
#endif
		} else
			ch = 0;

		flag = TTY_NORMAL;
		sw_uport->port.icount.rx++;

		if (unlikely(lsr & SUNXI_UART_LSR_BRK_ERROR_BITS)) {
			/*
			 * For statistics only
			 */
			if (lsr & SUNXI_UART_LSR_BI) {
				SERIAL_DBG(sw_uport->port.dev, "break interrupt occur\n");
				lsr &= ~(SUNXI_UART_LSR_FE | SUNXI_UART_LSR_PE);
				sw_uport->port.icount.brk++;
				/*
				 * We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
				if (!ch && uart_handle_break(&sw_uport->port))
					goto ignore_char;
			} else if (sunxi_uart_pe_occur(lsr)) {
				SERIAL_DBG(sw_uport->port.dev, "parity error occur\n");
				sw_uport->port.icount.parity++;
			} else if (lsr & SUNXI_UART_LSR_FE) {
				SERIAL_DBG(sw_uport->port.dev, "framing error occur\n");
				sw_uport->port.icount.frame++;
			}
			if (lsr & SUNXI_UART_LSR_OE) {
				SERIAL_DBG(sw_uport->port.dev, "overrun error occur\n");
				sw_uport->port.icount.overrun++;
			}

			/*
			 * Mask off conditions which should be ignored.
			 */
			lsr &= sw_uport->port.read_status_mask;
#if IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE)
			if (sw_is_console_port(&sw_uport->port)) {
				/* Recover the break flag from console xmit */
				lsr |= sw_uport->lsr_break_flag;
			}
#endif
			if (lsr & SUNXI_UART_LSR_BI)
				flag = TTY_BREAK;
			else if (sunxi_uart_pe_occur(lsr))
				flag = TTY_PARITY;
			else if (lsr & SUNXI_UART_LSR_FE)
				flag = TTY_FRAME;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41))
		if (uart_prepare_sysrq_char(&sw_uport->port, ch))
#else
		if (uart_handle_sysrq_char(&sw_uport->port, ch))
#endif
			goto ignore_char;
		SERIAL_DBG(sw_uport->port.dev, "receive data 0x%x\n", ch);
		uart_insert_char(&sw_uport->port, lsr, SUNXI_UART_LSR_OE, ch, flag);
ignore_char:
		lsr = serial_in(&sw_uport->port, SUNXI_UART_LSR);
	} while ((lsr & (SUNXI_UART_LSR_DR | SUNXI_UART_LSR_BI)) && (max_count-- > 0));

	SERIAL_DUMP(sw_uport, "Rx");
	spin_unlock(&sw_uport->port.lock);
	tty_flip_buffer_push(&sw_uport->port.state->port);
	spin_lock(&sw_uport->port.lock);

	return lsr;
}

static void sw_uart_stop_tx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int poll_time_tx = 0x7ffff;
	unsigned char lsr = 0;

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (uart_dma->use_dma & TX_DMA)
		sw_uart_stop_dma_tx(sw_uport);
#endif
	if (sw_uport->rs485_en) {

		while ((lsr & SUNXI_UART_LSR_BOTH_EMPTY) != SUNXI_UART_LSR_BOTH_EMPTY) {
			lsr = serial_in(port, SUNXI_UART_LSR);
			if (--poll_time_tx <= 0)
				break;
		};

		gpiod_set_value(sw_uport->rs485oe_gpio, sw_uport->rs485_fl);
	}
	sw_uart_disable_ier_thri(port);
}

static void sw_uart_start_tx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct circ_buf *xmit = &sw_uport->port.state->xmit;

	if (sw_uport->rs485_en) {
		if ((sw_uport->port.x_char) && (uart_circ_empty(xmit)))
			return;
		gpiod_set_value(sw_uport->rs485oe_gpio, !sw_uport->rs485_fl);
	}
#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	if (!((sw_uport->dma->use_dma & TX_DMA) && sw_uport->dma->tx_dma_used))
#endif
		sw_uart_enable_ier_thri(port);
}

static void sw_uart_handle_tx(struct sw_uart_port *sw_uport)
{
	struct circ_buf *xmit = &sw_uport->port.state->xmit;
	int count;

	if (sw_uport->port.x_char) {
		serial_out(&sw_uport->port, sw_uport->port.x_char, SUNXI_UART_THR);
		sw_uport->port.icount.tx++;
		sw_uport->port.x_char = 0;
#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
		sw_uport->dump_buff[sw_uport->dump_len++] = sw_uport->port.x_char;
		SERIAL_DUMP(sw_uport, "Tx");
#endif
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&sw_uport->port)) {
		sw_uart_stop_tx(&sw_uport->port);
		return;
	}

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	if (sw_uport->dma->use_dma & TX_DMA) {
		if (SERIAL_CIRC_CNT_TO_END(xmit) >= (sw_uport->port.fifosize / 2)) {
			sw_uart_start_dma_tx(sw_uport);
			return;
		}
	}
#endif

	count = sw_uport->port.fifosize / 2;
	do {
#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
		sw_uport->dump_buff[sw_uport->dump_len++] = xmit->buf[xmit->tail];
#endif
		serial_out(&sw_uport->port, xmit->buf[xmit->tail], SUNXI_UART_THR);
		SERIAL_DBG(sw_uport->port.dev, "write tx fifo 0x%x\n", xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		sw_uport->port.icount.tx++;
		if (uart_circ_empty(xmit)) {
			break;
		}
	} while (--count > 0);

	SERIAL_DUMP(sw_uport, "Tx");
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock(&sw_uport->port.lock);
		uart_write_wakeup(&sw_uport->port);
		spin_lock(&sw_uport->port.lock);
	}

	if (sw_uport->rs485_en)
		return;

	if (uart_circ_empty(xmit))
		sw_uart_stop_tx(&sw_uport->port);
}

static unsigned int sw_uart_modem_status(struct sw_uart_port *sw_uport)
{
	struct uart_port *port = &sw_uport->port;
	unsigned int status = serial_in(&sw_uport->port, SUNXI_UART_MSR);

	status |= sw_uport->msr_saved_flags;
	sw_uport->msr_saved_flags = 0;

	if (status & SUNXI_UART_MSR_ANY_DELTA && sw_uport->ier & SUNXI_UART_IER_MSI &&
	    sw_uport->port.state != NULL) {
		if (status & SUNXI_UART_MSR_TERI)
			sw_uport->port.icount.rng++;
		if (status & SUNXI_UART_MSR_DDSR)
			sw_uport->port.icount.dsr++;
		if (status & SUNXI_UART_MSR_DDCD)
			uart_handle_dcd_change(&sw_uport->port, status & SUNXI_UART_MSR_DCD);
		if (!(sw_uport->mcr & SUNXI_UART_MCR_AFE) && status & SUNXI_UART_MSR_DCTS)
			uart_handle_cts_change(&sw_uport->port, status & SUNXI_UART_MSR_CTS);

		wake_up_interruptible(&sw_uport->port.state->port.delta_msr_wait);
	}

	SERIAL_DBG(port->dev, "modem status: %x\n", status);
	return status;
}

static void sw_uart_force_lcr(struct sw_uart_port *sw_uport, unsigned msecs)
{
	unsigned long expire = jiffies + msecs_to_jiffies(msecs);
	struct uart_port *port = &sw_uport->port;

	/* hold tx so that uart will update lcr and baud in the gap of rx */
	serial_out(port, SUNXI_UART_HALT_HTX|SUNXI_UART_HALT_FORCECFG, SUNXI_UART_HALT);
	serial_out(port, sw_uport->lcr|SUNXI_UART_LCR_DLAB, SUNXI_UART_LCR);
	serial_out(port, sw_uport->dll, SUNXI_UART_DLL);
	serial_out(port, sw_uport->dlh, SUNXI_UART_DLH);
	serial_out(port, SUNXI_UART_HALT_HTX|SUNXI_UART_HALT_FORCECFG|SUNXI_UART_HALT_LCRUP, SUNXI_UART_HALT);
	while (time_before(jiffies, expire) && (serial_in(port, SUNXI_UART_HALT) & SUNXI_UART_HALT_LCRUP))
		;

	/*
	 * In fact there are two DLABs(DLAB and DLAB_BAK) in the hardware implementation.
	 * The DLAB_BAK is sellected only when SW_UART_HALT_FORCECFG is set to 1,
	 * and this bit can be access no matter uart is busy or not.
	 * So we select the DLAB_BAK always by leaving SW_UART_HALT_FORCECFG to be 1.
	 */
	serial_out(port, sw_uport->lcr, SUNXI_UART_LCR);
	serial_out(port, SUNXI_UART_HALT_FORCECFG, SUNXI_UART_HALT);
}

static void sw_uart_force_idle(struct sw_uart_port *sw_uport)
{
	struct uart_port *port = &sw_uport->port;

	if (sw_uport->fcr & SUNXI_UART_FCR_FIFO_EN) {
		serial_out(port, SUNXI_UART_FCR_FIFO_EN, SUNXI_UART_FCR);
		serial_out(port, SUNXI_UART_FCR_TXFIFO_RST
				| SUNXI_UART_FCR_RXFIFO_RST
				| SUNXI_UART_FCR_FIFO_EN, SUNXI_UART_FCR);
		serial_out(port, 0, SUNXI_UART_FCR);
	}

	serial_out(port, sw_uport->fcr, SUNXI_UART_FCR);
	(void)serial_in(port, SUNXI_UART_FCR);
}

/*
 * We should clear busy interupt, busy state and reset lcr,
 * but we should be careful not to introduce a new busy interrupt.
 */
static void sw_uart_handle_busy(struct sw_uart_port *sw_uport)
{
	struct uart_port *port = &sw_uport->port;

	(void)serial_in(port, SUNXI_UART_USR);

	/*
	 * Before reseting lcr, we should ensure than uart is not in busy
	 * state. Otherwise, a new busy interrupt will be introduced.
	 * It is wise to set uart into loopback mode, since it can cut down the
	 * serial in, then we should reset fifo(in my test, busy state
	 * (SUNXI_UART_USR_BUSY) can't be cleard until the fifo is empty).
	 */
	serial_out(port, sw_uport->mcr | SUNXI_UART_MCR_LOOP, SUNXI_UART_MCR);
	sw_uart_force_idle(sw_uport);
	serial_out(port, sw_uport->lcr, SUNXI_UART_LCR);
	serial_out(port, sw_uport->mcr, SUNXI_UART_MCR);
}

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
static void sw_uart_stop_dma_tx(struct sw_uart_port *sw_uport)
{
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (uart_dma && uart_dma->tx_dma_used) {
		dmaengine_terminate_all(uart_dma->dma_chan_tx);
		uart_dma->tx_dma_used = 0;
	}
}

static void sw_uart_release_dma_tx(struct sw_uart_port *sw_uport)
{
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (uart_dma && uart_dma->tx_dma_inited) {
		sw_uart_stop_dma_tx(sw_uport);
		dma_free_coherent(sw_uport->port.dev, sw_uport->dma->tb_size,
			sw_uport->dma->tx_buffer, sw_uport->dma->tx_phy_addr);
		sw_uport->port.state->xmit.buf = NULL;
		dma_release_channel(uart_dma->dma_chan_tx);
		uart_dma->dma_chan_tx = NULL;
		uart_dma->tx_dma_inited = 0;
	}
}

static int sw_uart_init_dma_tx(struct sw_uart_port *sw_uport)
{
	struct dma_slave_config slave_config;
	struct uart_port *port = &sw_uport->port;
	struct sw_uart_dma *uart_dma = sw_uport->dma;
	int ret;

	if (!uart_dma) {
		sunxi_info(sw_uport->port.dev, "sw_uart_init_dma_tx fail\n");
		return -1;
	}

	if (uart_dma->tx_dma_inited)
		return 0;

	uart_dma->dma_chan_tx = dma_request_chan(sw_uport->port.dev, "tx");
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
	sunxi_info(port->dev, "sw_uart_init_dma_tx sucess\n");
	return 0;
}

static void dma_tx_callback(void *data)
{
	struct uart_port *port = data;
	struct sw_uart_port *sw_uport = container_of(port,
						struct sw_uart_port, port);
	struct circ_buf *xmit = &port->state->xmit;
	struct sw_uart_dma *uart_dma = sw_uport->dma;
	struct scatterlist *sgl = &uart_dma->tx_sgl;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	dma_unmap_sg(sw_uport->port.dev, sgl, 1, DMA_TO_DEVICE);

	xmit->tail = (xmit->tail + uart_dma->tx_bytes) & (UART_XMIT_SIZE - 1);
	port->icount.tx += uart_dma->tx_bytes;
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	sw_uart_enable_ier_thri(port);
	uart_dma->tx_dma_used = 0;

	spin_unlock_irqrestore(&port->lock, flags);
}

static int sw_uart_start_dma_tx(struct sw_uart_port *sw_uport)
{
	int count = 0;
	struct uart_port *port = &sw_uport->port;
	struct circ_buf *xmit = &port->state->xmit;
	struct sw_uart_dma *uart_dma = sw_uport->dma;
	struct scatterlist *sgl = &uart_dma->tx_sgl;
	struct dma_async_tx_descriptor *desc;
	int ret;

	if (!uart_dma->use_dma)
		goto err_out;

	if (-1 == sw_uart_init_dma_tx(sw_uport))
		goto err_out;

	if (1 == uart_dma->tx_dma_used)
		return 1;

	uart_dma->tx_dma_used = 1;
	isb();
	/**********************************/
	/* mask the stop now */
	sw_uart_disable_ier_thri(port);

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
	sunxi_info(sw_uport->port.dev, "-sw_uart_start_dma_tx-error-\n");
	return -1;
}

static void sw_uart_stop_dma_rx(struct sw_uart_port *sw_uport)
{
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (uart_dma && uart_dma->rx_dma_used) {
		hrtimer_cancel(&sw_uport->rx_hrtimer);
		dmaengine_terminate_all(uart_dma->dma_chan_rx);
		uart_dma->rb_tail = 0;
		uart_dma->rx_dma_used = 0;
	}
}

static void sw_uart_release_dma_rx(struct sw_uart_port *sw_uport)
{
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (uart_dma && uart_dma->rx_dma_inited) {
		sw_uart_stop_dma_rx(sw_uport);
		dma_free_coherent(sw_uport->port.dev, sw_uport->dma->rb_size,
			sw_uport->dma->rx_buffer, sw_uport->dma->rx_phy_addr);
		dma_release_channel(uart_dma->dma_chan_rx);
		uart_dma->dma_chan_rx = NULL;
		uart_dma->rx_dma_inited = 0;
	}
}

static int sw_uart_init_dma_rx(struct sw_uart_port *sw_uport)
{
	int ret;
	struct uart_port *port = &sw_uport->port;
	struct dma_slave_config slave_config;
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (!uart_dma) {
		sunxi_info(port->dev, "sw_uart_init_dma_rx: port fail\n");
		return -1;
	}

	if (uart_dma->rx_dma_inited)
		return 0;

	uart_dma->dma_chan_rx = dma_request_chan(sw_uport->port.dev, "rx");
	if (!uart_dma->dma_chan_rx) {
		sunxi_err(port->dev, "cannot get the DMA channel.\n");
		return -1;
	}

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
	sunxi_info(port->dev, "sw_uart_init_dma_rx sucess\n");
	return 0;
}

static int sw_uart_start_dma_rx(struct sw_uart_port *sw_uport)
{
	struct uart_port *port = &sw_uport->port;
	struct sw_uart_dma *uart_dma = sw_uport->dma;
	struct dma_async_tx_descriptor *desc;

	if (!uart_dma->use_dma)
		return 0;

	if (uart_dma->rx_dma_used == 1)
		return 0;

	if (-1 == sw_uart_init_dma_rx(sw_uport)) {
		sunxi_info(sw_uport->port.dev, "sw_uart_init_dma_rx error!\n");
		return -1;
	}
	desc = dmaengine_prep_dma_cyclic(uart_dma->dma_chan_rx,
				uart_dma->rx_phy_addr, uart_dma->rb_size, 1,
					DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT);

	if (!desc) {
		sunxi_err(port->dev, "get rx dma descriptor failed!\n");
		return -EINVAL;
	}

	SERIAL_DBG(port->dev, "RX: prepare for the DMA.\n");
	uart_dma->rx_cookie = dmaengine_submit(desc);
	dma_async_issue_pending(uart_dma->dma_chan_rx);

	uart_dma->rx_dma_used = 1;
	if (uart_dma->use_timer == 1) {
		hrtimer_start(&sw_uport->rx_hrtimer,
			ns_to_ktime(uart_dma->rx_timeout), HRTIMER_MODE_REL);
	}
	return 1;
}

static void sw_uart_update_rb_addr(struct sw_uart_port *sw_uport)
{
	struct sw_uart_dma *uart_dma = sw_uport->dma;
	struct dma_tx_state state;
	uart_dma->rx_size = 0;
	if (uart_dma->rx_dma_used == 1) {
		dmaengine_tx_status(uart_dma->dma_chan_rx, uart_dma->rx_cookie,
									&state);
		if ((uart_dma->rb_size - state.residue) !=
						sw_uport->rx_last_pos) {
			uart_dma->rb_head = uart_dma->rb_size - state.residue;
			sw_uport->rx_last_pos = uart_dma->rb_head;
		}
	}
}

static enum hrtimer_restart sw_uart_report_dma_rx(struct hrtimer *rx_hrtimer)
{
	int count, flip = 0;
	struct sw_uart_port *sw_uport = container_of(rx_hrtimer,
						struct sw_uart_port, rx_hrtimer);
	struct uart_port *port = &sw_uport->port;
	struct sw_uart_dma *uart_dma = sw_uport->dma;

	if (!uart_dma->rx_dma_used || !port->state->port.tty)
		return HRTIMER_NORESTART;

	sw_uart_update_rb_addr(sw_uport);
	while (1) {
		count = CIRC_CNT_TO_END(uart_dma->rb_head, uart_dma->rb_tail,
							uart_dma->rb_size);
		if (count <= 0)
			break;
		port->icount.rx += count;
		flip = tty_insert_flip_string(&port->state->port,
				uart_dma->rx_buffer + uart_dma->rb_tail, count);
		tty_flip_buffer_push(&port->state->port);
		uart_dma->rb_tail =
			(uart_dma->rb_tail + count) & (uart_dma->rb_size - 1);
	}

	if (uart_dma->use_timer == 1) {
		hrtimer_forward_now(&sw_uport->rx_hrtimer,
				ns_to_ktime(uart_dma->rx_timeout));
	}

	return HRTIMER_RESTART;
}

#endif

static irqreturn_t sw_uart_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int iir = 0, lsr = 0;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	iir = serial_in(port, SUNXI_UART_IIR) & SUNXI_UART_IIR_IID_MASK;
	lsr = serial_in(port, SUNXI_UART_LSR);
	SERIAL_DBG(port->dev, "irq: iir %x lsr %x\n", iir, lsr);

	if (iir == SUNXI_UART_IIR_IID_BUSBSY) {
		sw_uart_handle_busy(sw_uport);
	} else {
		if (lsr & (SUNXI_UART_LSR_DR | SUNXI_UART_LSR_BI))
			lsr = sw_uart_handle_rx(sw_uport, lsr);
		/* has charto irq but no dr lsr? just read and ignore */
		else if (iir & SUNXI_UART_IIR_IID_CHARTO)
			serial_in(&sw_uport->port, SUNXI_UART_RBR);
		sw_uart_modem_status(sw_uport);
#if IS_ENABLED(CONFIG_SW_UART_PTIME_MODE)
		if (iir == SUNXI_UART_IIR_IID_THREMP)
#else
		if (lsr & SUNXI_UART_LSR_THRE)
#endif
			sw_uart_handle_tx(sw_uport);
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 41))
	uart_unlock_and_check_sysrq_irqrestore(port, flags);
#else
	spin_unlock_irqrestore(&port->lock, flags);
#endif
	return IRQ_HANDLED;
}

/*
 * uart buadrate and apb2 clock config selection
 * We should select an apb2 clock as low as possible
 * for lower power comsumpition, which can satisfy the
 * different baudrates of different ttyS applications.
 *
 * the reference table as follows:
 * pll6 600M
 * apb2div      0        20       19       18       17       16       15       14       13       12       11       10       9        8        7        6         5
 * apbclk       24000000 30000000 31578947 33333333 35294117 37500000 40000000 42857142 46153846 50000000 54545454 60000000 66666666 75000000 85714285 100000000 120000000
 * 115200            *      *         *        *        *        *        *        *        *        *        *        *        *        *       *         *         *
 * 230400                   *         *        *        *        *        *        *        *        *        *        *        *        *       *         *         *
 * 380400            *      *         *                 *        *                 *        *        *        *        *        *        *       *         *         *
 * 460800                   *                                    *                 *        *        *        *        *        *        *       *         *         *
 * 921600                   *                                                      *        *                          *                 *       *         *         *
 * 1000000                            *        *                                            *        *                          *                          *
 * 1500000           *                                                                      *        *                                   *                 *         *
 * 1750000                                                                                                    *                                  *
 * 2000000                            *        *                                                                                *                          *
 * 2500000                                                                *                                                                                          *
 * 3000000                                                                                  *        *                                                     *
 * 3250000                                                                                                    *                                            *
 * 3500000                                                                                                    *
 * 4000000                                                                                                                      *
 */
struct baudset {
	u32 baud;
	u32 uartclk_min;
	u32 uartclk_max;
};

static int sw_uart_check_baudset(struct uart_port *port, unsigned int baud)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	static struct baudset baud_set[] = {
		{115200, 24000000, 120000000},
		{230400, 30000000, 120000000},
		{380400, 24000000, 120000000},
		{460800, 30000000, 120000000},
		{921600, 30000000, 120000000},
		{1000000, 31000000, 120000000}, /* 31578947 */
		{1500000, 24000000, 120000000},
		{1750000, 54000000, 120000000}, /* 54545454 */
		{2000000, 31000000, 120000000}, /* 31578947 */
		{2500000, 40000000, 120000000}, /* 40000000 */
		{3000000, 46000000, 120000000}, /* 46153846 */
		{3250000, 54000000, 120000000}, /* 54545454 */
		{3500000, 54000000, 120000000}, /* 54545454 */
		{4000000, 66000000, 120000000}, /* 66666666 */
	};
	struct baudset *setsel;
	int i;

	if (baud < 115200) {
		if (port->uartclk < 24000000) {
			sunxi_info(port->dev, "uart%d, uartclk(%d) too small for baud %d\n",
				sw_uport->id, port->uartclk, baud);
			return -1;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(baud_set); i++) {
			if (baud == baud_set[i].baud)
				break;
		}

		if (i == ARRAY_SIZE(baud_set)) {
			sunxi_info(port->dev, "uart%d, baud %d beyond rance\n", sw_uport->id, baud);
			return -1;
		}

		setsel = &baud_set[i];
		if (port->uartclk < setsel->uartclk_min
			|| port->uartclk > setsel->uartclk_max) {
			sunxi_info(port->dev, "uart%d, select set %d, baud %d, uartclk %d beyond rance[%d, %d]\n",
				sw_uport->id, i, baud, port->uartclk,
				setsel->uartclk_min, setsel->uartclk_max);
			return -1;
		}
	}
	return 0;
}

#define BOTH_EMPTY    (SUNXI_UART_LSR_TEMT | SUNXI_UART_LSR_THRE)
static void wait_for_xmitr(struct sw_uart_port *sw_uport)
{
	unsigned int status, tmout = 10000;
#if IS_ENABLED(CONFIG_SW_UART_PTIME_MODE)
	unsigned int offs = SUNXI_UART_USR;
	unsigned char mask = SUNXI_UART_USR_TFNF;
#else
	unsigned int offs = SUNXI_UART_LSR;
	unsigned char mask = BOTH_EMPTY;
#endif

	/* Wait up to 10ms for the character(s) to be sent. */
	do {
		status = serial_in(&sw_uport->port, offs);
		if (serial_in(&sw_uport->port, SUNXI_UART_LSR) & SUNXI_UART_LSR_BI)
			sw_uport->lsr_break_flag = SUNXI_UART_LSR_BI;
		if (--tmout == 0)
			break;
		udelay(1);
	} while ((status & mask) != mask);

	/* CTS is unsupported by the 2-line UART, so ignore it. */
	if (sw_uport->pdata->io_num == 2)
		return;

	/* Wait up to 500ms for flow control if necessary */
	if (sw_uport->port.flags & UPF_CONS_FLOW) {
		tmout = 500000;
		for (tmout = 1000000; tmout; tmout--) {
			unsigned int msr = serial_in(&sw_uport->port, SUNXI_UART_MSR);

			sw_uport->msr_saved_flags |= msr & MSR_SAVE_FLAGS;
			if (msr & SUNXI_UART_MSR_CTS)
				break;

			udelay(1);
		}
	}
}

/* Enable or disable the RS485 support */
static void sw_uart_config_rs485(struct uart_port *port, struct serial_rs485 *rs485conf)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	sw_uport->rs485conf = *rs485conf;

	sw_uport->mcr &= ~SUNXI_UART_MCR_MODE_MASK;
	if (rs485conf->flags & SER_RS485_ENABLED) {
		SERIAL_DBG(port->dev, "setting to rs485\n");
		sw_uport->mcr |= SUNXI_UART_MCR_MODE_RS485;

		/*
		 * In NMM mode and no 9th bit(default RS485 mode), uart receive
		 * all the bytes into FIFO before receveing an address byte
		 */
		sw_uport->rs485 |= SUNXI_UART_RS485_RXBFA;
	} else {
		SERIAL_DBG(port->dev, "setting to uart\n");
		sw_uport->mcr |= SUNXI_UART_MCR_MODE_UART;
		sw_uport->rs485 = 0;
	}

	serial_out(port, sw_uport->mcr, SUNXI_UART_MCR);
	serial_out(port, sw_uport->rs485, SUNXI_UART_RS485);
}

static unsigned int sw_uart_tx_empty(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags = 0;
	unsigned int ret = 0;

	spin_lock_irqsave(&sw_uport->port.lock, flags);
	ret = (serial_in(port, SUNXI_UART_USR) & SUNXI_UART_USR_TFE) ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&sw_uport->port.lock, flags);
	return ret;
}

static void sw_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= SUNXI_UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= SUNXI_UART_MCR_DTR;
	if (mctrl & TIOCM_LOOP)
		mcr |= SUNXI_UART_MCR_LOOP;
	sw_uport->mcr &= ~(SUNXI_UART_MCR_RTS|SUNXI_UART_MCR_DTR|SUNXI_UART_MCR_LOOP);
	sw_uport->mcr |= mcr;

	if (sw_uport->loopback)
		sw_uport->mcr |= SUNXI_UART_MCR_LOOP;

	SERIAL_DBG(port->dev, "set mcr %x\n", mcr);
	serial_out(port, sw_uport->mcr, SUNXI_UART_MCR);
}

static unsigned int sw_uart_get_mctrl(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int msr;
	unsigned int ret = 0;

	msr = sw_uart_modem_status(sw_uport);
	if (msr & SUNXI_UART_MSR_DCD)
		ret |= TIOCM_CAR;
	if (msr & SUNXI_UART_MSR_RI)
		ret |= TIOCM_RNG;
	if (msr & SUNXI_UART_MSR_DSR)
		ret |= TIOCM_DSR;
	if (msr & SUNXI_UART_MSR_CTS)
		ret |= TIOCM_CTS;
	SERIAL_DBG(port->dev, "get msr %x\n", msr);
	return ret;
}

static void sw_uart_stop_rx(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	struct sw_uart_dma *uart_dma = sw_uport->dma;
	if (uart_dma->use_dma & RX_DMA) {
		sw_uart_stop_dma_rx(sw_uport);
	}
#endif
	if (sw_uport->ier & SUNXI_UART_IER_RLSI) {
		sw_uport->ier &= ~SUNXI_UART_IER_RLSI;
		SERIAL_DBG(port->dev, "stop rx, ier %x\n", sw_uport->ier);
		sw_uport->port.read_status_mask &= ~SUNXI_UART_LSR_DR;
		serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	}
}

static void sw_uart_enable_ms(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!(sw_uport->ier & SUNXI_UART_IER_MSI)) {
		sw_uport->ier |= SUNXI_UART_IER_MSI;
		SERIAL_DBG(port->dev, "en msi, ier %x\n", sw_uport->ier);
		serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	}
}

static void sw_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state == -1)
		sw_uport->lcr |= SUNXI_UART_LCR_SBC;
	else
		sw_uport->lcr &= ~SUNXI_UART_LCR_SBC;
	serial_out(port, sw_uport->lcr, SUNXI_UART_LCR);
	spin_unlock_irqrestore(&port->lock, flags);
}

static int sw_uart_startup(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG(port->dev, "start up ...\n");

	ret = request_irq(port->irq, sw_uart_irq, 0, sw_uport->name, port);
	if (unlikely(ret)) {
		sunxi_info(port->dev, "uart%d cannot get irq %d\n", sw_uport->id, port->irq);
		return ret;
	}

	sw_uport->msr_saved_flags = 0;
	/*
	 * PTIME mode to select the THRE trigger condition:
	 * if PTIME=1(IER[7]), the THRE interrupt will be generated when the
	 * the water level of the TX FIFO is lower than the threshold of the
	 * TX FIFO. and if PTIME=0, the THRE interrupt will be generated when
	 * the TX FIFO is empty.
	 * In addition, when PTIME=1, the THRE bit of the LSR register will not
	 * be set when the THRE interrupt is generated. You must check the
	 * interrupt id of the IIR register to decide whether some data need to
	 * send.
	 */

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	if (sw_uport->dma->use_dma & TX_DMA) {
		if (sw_uport->port.state->xmit.buf !=
						sw_uport->dma->tx_buffer){
			free_page((unsigned long)sw_uport->port.state->xmit.buf);
			sw_uport->port.state->xmit.buf =
						sw_uport->dma->tx_buffer;
		}
	} else
#endif
	{
		sw_uport->ier = 0;
		serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	}

	sw_uart_config_rs485(port, &sw_uport->rs485conf);

	return 0;
}

static void sw_uart_shutdown(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG(port->dev, "shut down ...\n");
#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	if (sw_uport->dma->use_dma & TX_DMA)
		sw_uport->port.state->xmit.buf = NULL;
#endif
	sw_uport->ier = 0;
	sw_uport->lcr = 0;
	sw_uport->mcr = 0;
	sw_uport->fcr = 0;
	serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	free_irq(port->irq, port);
}

static void sw_uart_flush_buffer(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG(port->dev, "flush buffer...\n");
	serial_out(port, sw_uport->fcr|SUNXI_UART_FCR_TXFIFO_RST, SUNXI_UART_FCR);
}

static void sw_uart_filter_lsr_err(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	static bool uart_lb[SUNXI_UART_NUM] = {false};
	/* note:For some memory fifo, first receive data,LSR will error make data abnormal.
	 * UART config loopback mode self-receiving data can filter this exception.
	 * Only need to config loopback mode once.
	 */
	if (sw_uport->port.line != 0 && !uart_lb[sw_uport->port.line]) {
		uart_lb[sw_uport->port.line] = true;
		/* set loopback mode */
		serial_out(port, sw_uport->mcr | SUNXI_UART_MCR_LOOP,
				SUNXI_UART_MCR);
		serial_out(&sw_uport->port, 0xff, SUNXI_UART_THR); /* write 0xff */
		sw_uart_start_tx(port);
		serial_in(&sw_uport->port, SUNXI_UART_RBR); /* read 0xff */
		sw_uart_stop_tx(port);
		/* disabled loopback mode */
		serial_out(port, sw_uport->mcr, SUNXI_UART_MCR);
	}
}

static void sw_uart_set_termios(struct uart_port *port, struct ktermios *termios,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 12))
			    const struct ktermios *old)
#else
			    struct ktermios *old)
#endif  /* LINUX_VERSION_CODE */
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;
	unsigned int baud, quot, lcr = 0, dll, dlh;

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	/* stop dma tx, which might make the uart be busy while some
	 * registers are set
	 */
	if (sw_uport->dma->tx_dma_used)
		sw_uart_stop_dma_tx(sw_uport);
#endif
	SERIAL_DBG(port->dev, "set termios ...\n");
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr |= SUNXI_UART_LCR_WLEN5;
		break;
	case CS6:
		lcr |= SUNXI_UART_LCR_WLEN6;
		break;
	case CS7:
		lcr |= SUNXI_UART_LCR_WLEN7;
		break;
	case CS8:
	default:
		lcr |= SUNXI_UART_LCR_WLEN8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		lcr |= SUNXI_UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		lcr |= SUNXI_UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		lcr |= SUNXI_UART_LCR_EPAR;

	/* set buadrate */
	baud = uart_get_baud_rate(port, termios, old,
				  port->uartclk / 16 / 0xffff,
				  port->uartclk / 16);
	sw_uart_check_baudset(port, baud);
	quot = uart_get_divisor(port, baud);
	dll = quot & 0xff;
	dlh = quot >> 8;
	SERIAL_DBG(port->dev, "set baudrate %d, quot %d\n", baud, quot);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 154))
	if (uart_console(port))
		console_lock();
#endif

	spin_lock_irqsave(&port->lock, flags);
	uart_update_timeout(port, termios->c_cflag, baud);

	/* Update the per-port timeout. */
	port->read_status_mask = SUNXI_UART_LSR_OE | SUNXI_UART_LSR_THRE | SUNXI_UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= SUNXI_UART_LSR_FE | SUNXI_UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= SUNXI_UART_LSR_BI;

	/* Characteres to ignore */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= SUNXI_UART_LSR_PE | SUNXI_UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= SUNXI_UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= SUNXI_UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= SUNXI_UART_LSR_DR;

	/*
	 * reset controller
	 */
	sw_uart_reset(sw_uport);

	if (baud <= 9600)
		sw_uport->fcr = SUNXI_UART_FCR_RXTRG_1CH
				| SUNXI_UART_FCR_TXTRG_1_2
				| SUNXI_UART_FCR_FIFO_EN;
	else
		sw_uport->fcr = SUNXI_UART_FCR_RXTRG_1_2
				| SUNXI_UART_FCR_TXTRG_1_2
				| SUNXI_UART_FCR_FIFO_EN;

	serial_out(port, sw_uport->fcr, SUNXI_UART_FCR);
	SERIAL_DBG(port->dev, "set fcr %x\n", sw_uport->fcr);

	sw_uport->lcr = lcr;
	sw_uport->dll = dll;
	sw_uport->dlh = dlh;
	sw_uart_force_lcr(sw_uport, 50);

	/* clear rxfifo after set lcr & baud to discard redundant data */
	serial_out(port, sw_uport->fcr|SUNXI_UART_FCR_RXFIFO_RST, SUNXI_UART_FCR);
	port->ops->set_mctrl(port, port->mctrl);

	sw_uport->ier = SUNXI_UART_IER_RLSI | SUNXI_UART_IER_RDI;
#if IS_ENABLED(CONFIG_SW_UART_PTIME_MODE)
	sw_uport->ier |= SUNXI_UART_IER_PTIME;
#endif
#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	if (sw_uport->dma->use_dma & TX_DMA)
		serial_out(port, SUNXI_UART_HALT_PTE | serial_in(port, SUNXI_UART_HALT), SUNXI_UART_HALT);

	if (sw_uport->dma->use_dma & RX_DMA) {
		/* disable the receive data interrupt */
		sw_uport->ier &= ~SUNXI_UART_IER_RDI;
		sw_uart_start_dma_rx(sw_uport);
	}
#endif
	/* flow control */
	sw_uport->mcr &= ~SUNXI_UART_MCR_AFE;
	port->status &= ~(UPSTAT_AUTOCTS | UPSTAT_AUTORTS);
	if (termios->c_cflag & CRTSCTS) {
		port->status |= UPSTAT_AUTOCTS | UPSTAT_AUTORTS;
		sw_uport->mcr |= SUNXI_UART_MCR_AFE;
	}
	serial_out(port, sw_uport->mcr, SUNXI_UART_MCR);
	SERIAL_DBG(port->dev, "set mcr %x\n", sw_uport->mcr);

	/*
	 * CTS flow control flag and modem status interrupts
	 */
	sw_uport->ier &= ~SUNXI_UART_IER_MSI;
	if (UART_ENABLE_MS(port, termios->c_cflag))
		sw_uport->ier |= SUNXI_UART_IER_MSI;
	serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	SERIAL_DBG(port->dev, "set ier %x\n", sw_uport->ier);
	/* Must save the current config for the resume of console(no tty user). */
	if (sw_is_console_port(port))
		port->cons->cflag = termios->c_cflag;

	spin_unlock_irqrestore(&port->lock, flags);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 154))
	if (uart_console(port))
		console_unlock();
#endif

	/* Don't rewrite B0 */
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);
	SERIAL_DBG(port->dev, "termios lcr 0x%x fcr 0x%x mcr 0x%x dll 0x%x dlh 0x%x\n",
			sw_uport->lcr, sw_uport->fcr, sw_uport->mcr,
			sw_uport->dll, sw_uport->dlh);

	sw_uart_filter_lsr_err(port);
}

static const char *sw_uart_type(struct uart_port *port)
{
	return "SUNXI";
}

static int sw_uart_select_gpio_state(struct pinctrl *pctrl, char *name, u32 no, struct device *dev)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		sunxi_info(dev, "UART%d pinctrl_lookup_state(%s) failed! return %p \n", no, name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		sunxi_info(dev, "UART%d pinctrl_select_state(%s) failed! return %d \n", no, name, ret);

	return ret;
}

static int sw_uart_request_gpio(struct sw_uart_port *sw_uport)
{
	if (sw_uport->card_print)
		return 0;

	sw_uport->pctrl = devm_pinctrl_get(sw_uport->port.dev);

	if (IS_ERR_OR_NULL(sw_uport->pctrl)) {
		sunxi_info(sw_uport->port.dev, "UART%d devm_pinctrl_get() failed! return %ld\n", sw_uport->id, PTR_ERR(sw_uport->pctrl));
		return -1;
	}

	return sw_uart_select_gpio_state(sw_uport->pctrl, PINCTRL_STATE_DEFAULT, sw_uport->id, sw_uport->port.dev);
}

static void sw_uart_release_gpio(struct sw_uart_port *sw_uport)
{
	if (sw_uport->card_print)
		return;

	devm_pinctrl_put(sw_uport->pctrl);
	sw_uport->pctrl = NULL;
}

static void sw_uart_release_port(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct platform_device *pdev;
	struct resource	*mem_res;

	SERIAL_DBG(port->dev, "release port(iounmap & release io)\n");

	pdev = to_platform_device(port->dev);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		sunxi_info(port->dev, "uart%d, get MEM resource failed\n", sw_uport->id);
		return;
	}

	/* release memory resource */
	release_mem_region(mem_res->start, resource_size(mem_res));
	iounmap(port->membase);
	port->membase = NULL;

	/* release io resource */
	sw_uart_release_gpio(sw_uport);
}

static int sw_uart_request_port(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct platform_device *pdev;
	struct resource	*mem_res;
	int ret;

	SERIAL_DBG(port->dev, "request port(ioremap & request io) %d\n", sw_uport->id);

	pdev = to_platform_device(port->dev);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		sunxi_info(port->dev, "uart%d, get MEM resource failed\n", sw_uport->id);
		ret = -ENXIO;
	}

	/* request memory resource */
	if (!request_mem_region(mem_res->start, resource_size(mem_res), SUNXI_UART_DEV_NAME)) {
		sunxi_info(port->dev, "uart%d, request mem region failed\n", sw_uport->id);
		return -EBUSY;
	}

	port->membase = ioremap(mem_res->start, resource_size(mem_res));
	if (!port->membase) {
		sunxi_info(port->dev, "uart%d, ioremap failed\n", sw_uport->id);
		release_mem_region(mem_res->start, resource_size(mem_res));
		return -EBUSY;
	}

	/* request io resource */
	ret = sw_uart_request_gpio(sw_uport);
	if (ret < 0) {
		release_mem_region(mem_res->start, resource_size(mem_res));
		return ret;
	}

	return 0;
}

static void sw_uart_config_port(struct uart_port *port, int flags)
{
	int ret;

	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_SUNXI;
		ret = sw_uart_request_port(port);
		if (ret)
			return;
	}
}

static int sw_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_SUNXI))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;
	return 0;
}

static int sw_uart_ioctl(struct uart_port *port, unsigned int cmd,
			 unsigned long arg)
{
	struct serial_rs485 rs485conf;
	unsigned long flags = 0;

	if (!access_ok((void __user *)arg, sizeof(rs485conf))) {
		dev_err(port->dev, "Failed to access memory\n");
		return -EFAULT;
	}
	switch (cmd) {
	case TIOCSRS485:
		if (copy_from_user(&rs485conf, (void __user *)arg,
				   sizeof(rs485conf)))
			return -EFAULT;

		spin_lock_irqsave(&port->lock, flags);
		sw_uart_config_rs485(port, &rs485conf);
		spin_unlock_irqrestore(&port->lock, flags);
		break;

	case TIOCGRS485:
		if (copy_to_user((void __user *)arg,
				 &(UART_TO_SPORT(port)->rs485conf),
				 sizeof(rs485conf)))
			return -EFAULT;
		break;

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static void sw_uart_pm(struct uart_port *port, unsigned int state,
		      unsigned int oldstate)
{
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	int ret;
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	SERIAL_DBG(port->dev, "PM state %d -> %d\n", oldstate, state);

	switch (state) {
	case UART_PM_STATE_ON: /* Power up */
		ret = clk_prepare_enable(sw_uport->mclk);
		if (ret) {
			sunxi_info(port->dev, "uart%d release reset failed\n", sw_uport->id);
		}
		break;
	case UART_PM_STATE_OFF: /* Power down */
		clk_disable_unprepare(sw_uport->mclk);
		break;
	default:
		sunxi_info(port->dev, "uart%d, Unknown PM state %d\n", sw_uport->id, state);
	}
#endif
}

#if IS_ENABLED(CONFIG_CONSOLE_POLL)
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int sw_get_poll_char(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned int lsr = serial_in(port, SUNXI_UART_LSR);

	if (!(lsr & SUNXI_UART_LSR_DR)) {
		return NO_POLL_CHAR;
	}

	return serial_in(port, SUNXI_UART_RBR);
}


static void sw_put_poll_char(struct uart_port *port,
			unsigned char c)
{
	unsigned int ier;
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	/*
	 * First save the IER then disable the interrupts.
	 */
	ier = serial_in(port, SUNXI_UART_IER);

	serial_out(port, 0, SUNXI_UART_IER);
	wait_for_xmitr(sw_uport);

	serial_out(port, c, SUNXI_UART_THR);
	if (c == 10) {
		wait_for_xmitr(sw_uport);
		serial_out(port, 13, SUNXI_UART_THR);
	}
	/*
	 * Finally, wait for transmitter to become empty
	 * and restore the IER
	 */
	wait_for_xmitr(sw_uport);
	serial_out(port, ier, SUNXI_UART_IER);
}

#endif /* CONFIG_CONSOLE_POLL */

/* Disable receive interrupts to throttle receive data, which could
 * avoid receive data overrun
 */
static void sw_uart_throttle(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	sw_uport->ier &= ~(SUNXI_UART_IER_RLSI | SUNXI_UART_IER_RDI);
	serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	sw_uport->throttled = true;
	spin_unlock_irqrestore(&port->lock, flags);
}

static void sw_uart_unthrottle(struct uart_port *port)
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	sw_uport->ier |= SUNXI_UART_IER_RLSI | SUNXI_UART_IER_RDI;
	serial_out(port, sw_uport->ier, SUNXI_UART_IER);
	sw_uport->throttled = false;
	spin_unlock_irqrestore(&port->lock, flags);
}

static struct uart_ops sw_uart_ops = {
	.tx_empty = sw_uart_tx_empty,
	.set_mctrl = sw_uart_set_mctrl,
	.get_mctrl = sw_uart_get_mctrl,
	.stop_tx = sw_uart_stop_tx,
	.start_tx = sw_uart_start_tx,
	.stop_rx = sw_uart_stop_rx,
	.enable_ms = sw_uart_enable_ms,
	.break_ctl = sw_uart_break_ctl,
	.startup = sw_uart_startup,
	.shutdown = sw_uart_shutdown,
	.flush_buffer = sw_uart_flush_buffer,
	.set_termios = sw_uart_set_termios,
	.type = sw_uart_type,
	.release_port = sw_uart_release_port,
	.request_port = sw_uart_request_port,
	.config_port = sw_uart_config_port,
	.verify_port = sw_uart_verify_port,
	.ioctl = sw_uart_ioctl,
	.pm = sw_uart_pm,
#if IS_ENABLED(CONFIG_CONSOLE_POLL)
	.poll_get_char = sw_get_poll_char,
	.poll_put_char = sw_put_poll_char,
#endif
	.throttle = sw_uart_throttle,
	.unthrottle = sw_uart_unthrottle,
};

static int sw_uart_regulator_request(struct sw_uart_port *sw_uport, struct sw_uart_pdata *pdata,
		struct device *dev)
{
	struct regulator *regu = NULL;

	/* Consider "n***" as nocare. Support "none", "nocare", "null", "" etc. */
	if ((pdata->regulator_id[0] == 'n') || (pdata->regulator_id[0] == 0)) {
		/* if regulator_id not exist, use dt way to get regulator */
		regu = regulator_get(dev, "uart");
		if (IS_ERR(regu)) {
			sunxi_warn(dev, "get regulator by dt way failed!\n");
			return 0;
		}
		pdata->regulator = regu;
		return 0;
	}

	regu = regulator_get(NULL, pdata->regulator_id);
	if (IS_ERR(regu)) {
		sunxi_info(sw_uport->port.dev, "get regulator %s failed!\n", pdata->regulator_id);
		return -1;
	}
	pdata->regulator = regu;
	return 0;
}

static void sw_uart_regulator_release(struct sw_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return;

	regulator_put(pdata->regulator);
	pdata->regulator = NULL;
}

static int sw_uart_regulator_enable(struct sw_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_enable(pdata->regulator) != 0)
		return -1;

	return 0;
}

static int sw_uart_regulator_disable(struct sw_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_disable(pdata->regulator) != 0)
		return -1;

	return 0;
}

static struct sw_uart_port sw_uart_ports[SUNXI_UART_NUM];
static struct sw_uart_pdata sw_uport_pdata[SUNXI_UART_NUM];

static ssize_t sunxi_uart_dev_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	struct sw_uart_pdata *pdata = (struct sw_uart_pdata *)dev->platform_data;

	return scnprintf(buf, PAGE_SIZE,
		"id     = %u \n"
		"name   = %s \n"
		"irq    = %u \n"
		"io_num = %u \n"
		"port->mapbase = %pa \n"
		"port->membase = 0x%p \n"
		"port->iobase  = 0x%08lx \n"
		"port->fifosize = %d\n"
		"pdata->regulator    = 0x%p \n"
		"pdata->regulator_id = %s \n",
		sw_uport->id, sw_uport->name, port->irq,
		sw_uport->pdata->io_num,
		&port->mapbase, port->membase, port->iobase,
		port->fifosize, pdata->regulator, pdata->regulator_id);
}

static struct device_attribute sunxi_uart_dev_info_attr =
	__ATTR(dev_info, S_IRUGO, sunxi_uart_dev_info_show, NULL);

static ssize_t sunxi_uart_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE,
		"uartclk = %d \n"
		"The Uart controller register[Base: 0x%p]: \n"
		"[RTX] 0x%02x = 0x%08x, [IER] 0x%02x = 0x%08x, [FCR] 0x%02x = 0x%08x \n"
		"[LCR] 0x%02x = 0x%08x, [MCR] 0x%02x = 0x%08x, [LSR] 0x%02x = 0x%08x \n"
		"[MSR] 0x%02x = 0x%08x, [SCH] 0x%02x = 0x%08x, [USR] 0x%02x = 0x%08x \n"
		"[TFL] 0x%02x = 0x%08x, [RFL] 0x%02x = 0x%08x, [HALT] 0x%02x = 0x%08x \n",
	port->uartclk, port->membase,
		SUNXI_UART_RBR, readl(port->membase + SUNXI_UART_RBR),
		SUNXI_UART_IER, readl(port->membase + SUNXI_UART_IER),
		SUNXI_UART_FCR, readl(port->membase + SUNXI_UART_FCR),
		SUNXI_UART_LCR, readl(port->membase + SUNXI_UART_LCR),
		SUNXI_UART_MCR, readl(port->membase + SUNXI_UART_MCR),
		SUNXI_UART_LSR, readl(port->membase + SUNXI_UART_LSR),
		SUNXI_UART_MSR, readl(port->membase + SUNXI_UART_MSR),
		SUNXI_UART_SCH, readl(port->membase + SUNXI_UART_SCH),
		SUNXI_UART_USR, readl(port->membase + SUNXI_UART_USR),
		SUNXI_UART_TFL, readl(port->membase + SUNXI_UART_TFL),
		SUNXI_UART_RFL, readl(port->membase + SUNXI_UART_RFL),
		SUNXI_UART_HALT, readl(port->membase + SUNXI_UART_HALT));
}
static struct device_attribute sunxi_uart_status_attr =
	__ATTR(status, S_IRUGO, sunxi_uart_status_show, NULL);

static ssize_t sunxi_uart_loopback_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int mcr = 0;
	struct uart_port *port = dev_get_drvdata(dev);

	mcr = readl(port->membase + SUNXI_UART_MCR);
	return scnprintf(buf, PAGE_SIZE,
		"MCR: 0x%08x, Loopback: %d\n", mcr, mcr&SUNXI_UART_MCR_LOOP ? 1 : 0);
}

static ssize_t sunxi_uart_loopback_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int mcr = 0;
	int enable = 0;
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (!strncmp(buf, "enable", 6))
		enable = 1;

	SERIAL_DBG(dev, "Set loopback: %d \n", enable);

	if (enable) {
		sw_uport->loopback = true;
		writel(mcr|SUNXI_UART_MCR_LOOP, port->membase + SUNXI_UART_MCR);
	} else {
		sw_uport->loopback = false;
		writel(mcr&(~SUNXI_UART_MCR_LOOP), port->membase + SUNXI_UART_MCR);
	}

	return count;
}
static struct device_attribute sunxi_uart_loopback_attr =
	__ATTR(loopback, S_IRUGO|S_IWUSR, sunxi_uart_loopback_show, sunxi_uart_loopback_store);

static ssize_t sunxi_uart_ctrl_info_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);
	u32 dl = (u32)sw_uport->dlh << 8 | (u32)sw_uport->dll;

	if (dl == 0)
		dl = 1000;

	return scnprintf(buf, PAGE_SIZE,
		" ier  : 0x%02x\n"
		" lcr  : 0x%02x\n"
		" mcr  : 0x%02x\n"
		" fcr  : 0x%02x\n"
		" dll  : 0x%02x\n"
		" dlh  : 0x%02x\n"
		" last baud : %d (dl = %d)\n\n"
		"TxRx Statistics:\n"
		" tx     : %d\n"
		" rx     : %d\n"
		" parity : %d\n"
		" frame  : %d\n"
		" overrun: %d\n"
		" throttled: %d\n",
		sw_uport->ier, sw_uport->lcr, sw_uport->mcr,
		sw_uport->fcr, sw_uport->dll, sw_uport->dlh,
		(sw_uport->port.uartclk>>4)/dl, dl,
		sw_uport->port.icount.tx,
		sw_uport->port.icount.rx,
		sw_uport->port.icount.parity,
		sw_uport->port.icount.frame,
		sw_uport->port.icount.overrun,
		sw_uport->throttled);
}
static struct device_attribute sunxi_uart_ctrl_info_attr =
	__ATTR(ctrl_info, S_IRUGO, sunxi_uart_ctrl_info_show, NULL);

static void sunxi_uart_sysfs(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_uart_dev_info_attr);
	device_create_file(&_pdev->dev, &sunxi_uart_status_attr);
	device_create_file(&_pdev->dev, &sunxi_uart_loopback_attr);
	device_create_file(&_pdev->dev, &sunxi_uart_ctrl_info_attr);
}

#if IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE)
static struct uart_port *sw_console_get_port(struct console *co)
{
	struct uart_port *port = NULL;
	int i, used;

	for (i = 0; i < SUNXI_UART_NUM; i++) {
		used = sw_uport_pdata[i].used;
		port = &sw_uart_ports[i].port;
		if ((used == 1) && (port->line == co->index))
			return port;
	}
	return NULL;
}

static void sw_console_putchar(struct uart_port *port,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 12))
				unsigned char c)
#else
				int c)
#endif  /* LINUX_VERSION_CODE */
{
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	wait_for_xmitr(sw_uport);
	serial_out(port, c, SUNXI_UART_THR);
}

static void sw_console_write(struct console *co, const char *s,
			      unsigned int count)
{
	struct uart_port *port = NULL;
	struct sw_uart_port *sw_uport;
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	BUG_ON(co->index < 0 || co->index >= SUNXI_UART_NUM);

	port = sw_console_get_port(co);
	if (port == NULL)
		return;
	sw_uport = UART_TO_SPORT(port);

	if (port->sysrq || oops_in_progress)
		locked = spin_trylock_irqsave(&port->lock, flags);
	else
		spin_lock_irqsave(&port->lock, flags);

	ier = serial_in(port, SUNXI_UART_IER);
	serial_out(port, 0, SUNXI_UART_IER);

	uart_console_write(port, s, count, sw_console_putchar);
	wait_for_xmitr(sw_uport);
	serial_out(port, ier, SUNXI_UART_IER);

	if (locked)
		spin_unlock_irqrestore(&port->lock, flags);
}

static int sw_console_setup(struct console *co, char *options)
{
	struct uart_port *port = NULL;
	struct sw_uart_port *sw_uport;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (unlikely(co->index >= SUNXI_UART_NUM || co->index < 0))
		return -ENXIO;

	port = sw_console_get_port(co);
	if (port == NULL)
		return -ENODEV;
	sw_uport = UART_TO_SPORT(port);
	if (!port->iobase && !port->membase)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	sunxi_info(port->dev, "console setup baud %d parity %c bits %d, flow %c\n",
			baud, parity, bits, flow);
	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver sw_uart_driver;
static struct console sw_console = {
#if IS_ENABLED(CONFIG_SERIAL_8250)
	.name = "ttyAS",
#else
	.name = "ttyS",
#endif
	.write = sw_console_write,
	.device = uart_console_device,
	.setup = sw_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &sw_uart_driver,
};

#define SW_CONSOLE	(&sw_console)
#else
#define SW_CONSOLE	NULL
#endif

static struct uart_driver sw_uart_driver = {
	.owner = THIS_MODULE,
	.driver_name = SUNXI_UART_DEV_NAME,
#if IS_ENABLED(CONFIG_SERIAL_8250)
	.dev_name = "ttyAS",
#else
	.dev_name = "ttyS",
#endif
	.nr = SUNXI_UART_NUM,
	.cons = SW_CONSOLE,
};

static int sw_uart_request_resource(struct sw_uart_port *sw_uport, struct sw_uart_pdata *pdata,
		struct device *dev)
{
	struct uart_port *port = &sw_uport->port;
	SERIAL_DBG(port->dev, "get system resource(clk & IO)\n");

	if (sw_uart_regulator_request(sw_uport, pdata, dev) < 0) {
		sunxi_info(port->dev, "uart%d request regulator failed!\n", sw_uport->id);
		return -ENXIO;
	}
	sw_uart_regulator_enable(pdata);

#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
	sw_uport->dump_buff = kmalloc(MAX_DUMP_SIZE, GFP_KERNEL);
	if (!sw_uport->dump_buff) {
		sunxi_info(port->dev, "uart%d fail to alloc dump buffer\n", sw_uport->id);
	}
#endif

	return 0;
}

static int sw_uart_release_resource(struct sw_uart_port *sw_uport, struct sw_uart_pdata *pdata)
{
	struct uart_port *port = &sw_uport->port;
	SERIAL_DBG(port->dev, "put system resource(clk & IO)\n");

#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
	kfree(sw_uport->dump_buff);
	sw_uport->dump_buff = NULL;
	sw_uport->dump_len = 0;
#endif

	clk_disable_unprepare(sw_uport->mclk);
	clk_put(sw_uport->mclk);

	sw_uart_regulator_disable(pdata);
	sw_uart_regulator_release(pdata);

	return 0;
}

struct platform_device *sw_uart_get_pdev(int uart_id)
{
	if (sw_uart_ports[uart_id].port.dev)
		return to_platform_device(sw_uart_ports[uart_id].port.dev);
	else
		return NULL;
}

#if IS_ENABLED(CONFIG_AW_SERIAL_EARLYCON)

#define SUNXI_UART_USR_NF    0x02    /* Tansmit fifo not full */

static void sunxi_serial_console_putchar(struct uart_port *port, int ch)
{
	int value = 0;

	do {
		value = readl_relaxed(port->membase + SUNXI_UART_USR);
	} while (!(value & SUNXI_UART_USR_NF));

	writel_relaxed(ch, port->membase + SUNXI_UART_THR);
}

static __init void sunxi_early_serial_write(struct console *con, const char *s,
					  unsigned int n)
{
	struct earlycon_device *dev = con->data;

	uart_console_write(&dev->port, s, n, sunxi_serial_console_putchar);
}

static int __init sunxi_early_console_setup(struct earlycon_device *dev,
					  const char *opt)
{
	if (!dev->port.membase)
		return -ENODEV;
	dev->con->write = sunxi_early_serial_write;
	return 0;
}
OF_EARLYCON_DECLARE(uart0, "", sunxi_early_console_setup);
#endif	/* CONFIG_AW_SERIAL_EARLYCON */

static int sw_uart_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct uart_port *port;
	struct sw_uart_port *sw_uport;
	struct sw_uart_pdata *pdata;
	struct resource *res;
	char uart_para[16] = {0};
	const char *uart_string;
	int ret = -1;
	struct device_node *apk_np = of_find_node_by_name(NULL, "auto_print");
	const char *apk_sta = NULL;
	int irq;
#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	int use_dma = 0;
#endif

	pdev->id = of_alias_get_id(np, "serial");
	if (pdev->id < 0 || pdev->id >= SUNXI_UART_NUM) {
		sunxi_info(&pdev->dev, "get alias id err or exceed supported uart controllers\n");
		return -EINVAL;
	}

	port = &sw_uart_ports[pdev->id].port;
	port->dev = &pdev->dev;
	pdata = &sw_uport_pdata[pdev->id];
	sw_uport = UART_TO_SPORT(port);
	sw_uport->pdata = pdata;
	sw_uport->id = pdev->id;
	sw_uport->ier = 0;
	sw_uport->lcr = 0;
	sw_uport->mcr = 0;
	sw_uport->fcr = 0;
	sw_uport->dll = 0;
	sw_uport->dlh = 0;
	snprintf(sw_uport->name, 16, SUNXI_UART_DEV_NAME"%d", pdev->id);
	pdev->dev.init_name = sw_uport->name;
	pdev->dev.platform_data = sw_uport->pdata;

	snprintf(uart_para, sizeof(uart_para), "uart%d_regulator", pdev->id);
	ret = of_property_read_string(np, uart_para, &uart_string);
	if (!ret)
		strncpy(pdata->regulator_id, uart_string, 16);

	/* request system resource and init them */
	ret = sw_uart_request_resource(sw_uport, pdev->dev.platform_data, &pdev->dev);
	if (unlikely(ret)) {
		sunxi_info(&pdev->dev, "uart%d error to get resource\n", pdev->id);
		return -ENXIO;
	}


#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	sw_uport->reset = devm_reset_control_get_optional(&pdev->dev, NULL);
	if (IS_ERR(sw_uport->reset)) {
		printk("get reset clk error\n");
		return -EINVAL;
	}

	sw_uport->mclk = of_clk_get(np, 0);
	if (IS_ERR(sw_uport->mclk)) {
		sunxi_info(&pdev->dev, "uart%d error to get clk\n", pdev->id);
		return -EINVAL;
	}

	ret = reset_control_deassert(sw_uport->reset);
	if (ret) {
		printk("deassert clk error, ret:%d\n", ret);
		return ret;
	}

	/* uart clk come from apb2, apb2 default clk is hosc. if change rate
	 * needed, must switch apb2's source clk first and then set its rate
	 * */
	sw_uport->sclk = of_clk_get(np, 1);
	if (!IS_ERR(sw_uport->sclk)) {
		sw_uport->pclk = of_clk_get(np, 2);
		port->uartclk = clk_get_rate(sw_uport->sclk);
		/* config a fixed divider before switch source clk for apb2 */
		clk_set_rate(sw_uport->sclk, port->uartclk/6);
		/* switch source clock for apb2 */
		clk_set_parent(sw_uport->sclk, sw_uport->pclk);
		ret = of_property_read_u32(np, "clock-frequency",
					&port->uartclk);
		if (ret) {
			sunxi_info(&pdev->dev, "uart%d get clock-freq failed\n", pdev->id);
			return -EINVAL;
		}
		/* set apb2 clock frequency now */
		clk_set_rate(sw_uport->sclk, port->uartclk);
	}

	port->uartclk = clk_get_rate(sw_uport->mclk);
#else
	port->uartclk = 24000000;
#endif

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		sunxi_err(&pdev->dev, "uart%d error to get MEM resource\n", pdev->id);
		return -EINVAL;
	}
	port->mapbase = res->start;


#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	sw_uport->dma = devm_kzalloc(&pdev->dev, sizeof(*sw_uport->dma), GFP_KERNEL);
	if (!sw_uport->dma) {
		sunxi_err(&pdev->dev, "unable to allocate mem\n");
		return -ENOMEM;
	}
	ret = of_property_read_u32(np, "use_dma", &use_dma);
	if (ret)
		use_dma = 0;
	sw_uport->dma->use_dma = use_dma;
	sw_uport->dma->rx_dma_inited = 0;
	sw_uport->dma->rx_dma_used = 0;
	sw_uport->dma->tx_dma_inited = 0;
	sw_uport->dma->tx_dma_used = 0;
#endif

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0)
		return -EINVAL;
	port->irq = (unsigned int)irq;

	snprintf(uart_para, sizeof(uart_para), "uart%d_port", pdev->id);
	ret = of_property_read_u32(np, uart_para, &port->line);
	if (ret) {
		sunxi_err(&pdev->dev, "uart%d error to get port property\n", pdev->id);
		return -EINVAL;
	}


	snprintf(uart_para, sizeof(uart_para), "uart%d_type", pdev->id);
	ret = of_property_read_u32(np, uart_para, &pdata->io_num);
	if (ret) {
		sunxi_err(&pdev->dev, "uart%d error to get type property\n", pdev->id);
		return -EINVAL;
	}

	if (apk_np && !of_property_read_string(apk_np, "status", &apk_sta)
						&& !strcmp(apk_sta, "okay"))
		sw_uport->card_print = true;
	else
		sw_uport->card_print = false;

	of_property_read_u32(np, "sunxi,uart-rs485", &sw_uport->rs485_en);
	if (sw_uport->rs485_en) {
		ret = of_property_read_u32(np, "sunxi,uart-485fl", &sw_uport->rs485_fl);
		if (ret) {
			sunxi_info(&pdev->dev, "cannot get 485-fl, use default value: high\n");
			sw_uport->rs485_fl = 1;
		}

		sw_uport->rs485oe_gpio = devm_gpiod_get(&pdev->dev, "sunxi,uart-485oe", GPIOD_OUT_HIGH);
		if (IS_ERR(sw_uport->rs485oe_gpio)) {
			sunxi_err(&pdev->dev, "Error: request rs485oe_gpio failed\n");
			return PTR_ERR(sw_uport->rs485oe_gpio);
		}
		gpiod_set_value(sw_uport->rs485oe_gpio, sw_uport->rs485_fl);
	}

	pdata->used = 1;
	port->iotype = UPIO_MEM;
	port->type = PORT_SUNXI;
	port->flags = UPF_BOOT_AUTOCONF;
	port->ops = &sw_uart_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	port->has_sysrq = IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE);
#endif
	ret = of_property_read_u32(np, "sunxi,uart-fifosize", &port->fifosize);
	if (ret) {
		sunxi_err(&pdev->dev, "uart%d error to get fifo size property\n", pdev->id);
		port->fifosize = SUNXI_UART_FIFO_SIZE;
	}

	platform_set_drvdata(pdev, port);

#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	/* set dma config */
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	if (sw_uport->dma->use_dma & RX_DMA) {
		/* timer */
		sw_uport->dma->use_timer = UART_USE_TIMER;
		sw_uport->dma->rx_timeout = 2000000;
		hrtimer_init(&sw_uport->rx_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		sw_uport->rx_hrtimer.function = sw_uart_report_dma_rx;

		/* rx buffer */
		sw_uport->dma->rb_size = DMA_SERIAL_BUFFER_SIZE;
		sw_uport->dma->rx_buffer = dma_alloc_coherent(
				sw_uport->port.dev, sw_uport->dma->rb_size,
				&sw_uport->dma->rx_phy_addr, GFP_KERNEL);
		sw_uport->dma->rb_tail = 0;

		if (!sw_uport->dma->rx_buffer) {
			sunxi_err(sw_uport->port.dev,
				"dmam_alloc_coherent dma_rx_buffer fail\n");
			return -ENOMEM;
		} else {
			sunxi_info(&pdev->dev,
				"dma_rx_buffer %p\n", sw_uport->dma->rx_buffer);
			sunxi_info(&pdev->dev,
		"dma_rx_phy 0x%08x\n", (unsigned)sw_uport->dma->rx_phy_addr);
		}
		sw_uart_init_dma_rx(sw_uport);
	}

	if (sw_uport->dma->use_dma & TX_DMA) {
		/* tx buffer */
		sw_uport->dma->tb_size = UART_XMIT_SIZE;
		sw_uport->dma->tx_buffer = dma_alloc_coherent(
				sw_uport->port.dev, sw_uport->dma->tb_size,
				&sw_uport->dma->tx_phy_addr, GFP_KERNEL);
		if (!sw_uport->dma->tx_buffer) {
			sunxi_info(&pdev->dev,
				"dmam_alloc_coherent dma_tx_buffer fail\n");
		} else {
			sunxi_info(&pdev->dev, "dma_tx_buffer %p\n",
						sw_uport->dma->tx_buffer);
			sunxi_info(&pdev->dev, "dma_tx_phy 0x%08x\n",
					(unsigned) sw_uport->dma->tx_phy_addr);
		}
		sw_uart_init_dma_tx(sw_uport);
	}

#endif

	sunxi_uart_sysfs(pdev);

	SERIAL_DBG(port->dev, "add uart%d port, port_type %d, uartclk %d\n",
			pdev->id, port->type, port->uartclk);
	return uart_add_one_port(&sw_uart_driver, port);
}

static int sw_uart_remove(struct platform_device *pdev)
{
	struct sw_uart_port *sw_uport = platform_get_drvdata(pdev);

	SERIAL_DBG(&pdev->dev, "release uart%d port\n", sw_uport->id);
#if IS_ENABLED(CONFIG_AW_SERIAL_DMA)
	sw_uart_release_dma_tx(sw_uport);
	sw_uart_release_dma_rx(sw_uport);
#endif
	sw_uart_release_resource(sw_uport, pdev->dev.platform_data);
	return 0;
}

/* UART power management code */
#if IS_ENABLED(CONFIG_PM_SLEEP)

#define SW_UART_NEED_SUSPEND(port) \
	((sw_is_console_port(port) && (console_suspend_enabled)) \
		|| !sw_is_console_port(port))

static int sw_uart_suspend(struct device *dev)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (port) {
		sunxi_info(port->dev, "uart%d suspend\n", sw_uport->id);
		uart_suspend_port(&sw_uart_driver, port);

		if (SW_UART_NEED_SUSPEND(port)) {
			if (!sw_uport->card_print)
				sw_uart_select_gpio_state(sw_uport->pctrl,
					PINCTRL_STATE_SLEEP, sw_uport->id,
					dev);
			sw_uart_regulator_disable(dev->platform_data);
		}
	}

	return 0;
}

static int sw_uart_resume(struct device *dev)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sw_uart_port *sw_uport = UART_TO_SPORT(port);

	if (port) {
		if (SW_UART_NEED_SUSPEND(port)) {
			sw_uart_regulator_enable(dev->platform_data);
			if (!sw_uport->card_print)
				sw_uart_select_gpio_state(sw_uport->pctrl,
					PINCTRL_STATE_DEFAULT, sw_uport->id,
					dev);
		}
		uart_resume_port(&sw_uart_driver, port);
		sunxi_info(port->dev, "uart%d resume. DLH: %d, DLL: %d. \n", sw_uport->id, sw_uport->dlh, sw_uport->dll);
	}

	return 0;
}

static const struct dev_pm_ops sw_uart_pm_ops = {
	.suspend = sw_uart_suspend,
	.resume = sw_uart_resume,
};
#define SERIAL_SW_PM_OPS	(&sw_uart_pm_ops)

#else /* !CONFIG_PM_SLEEP */

#define SERIAL_SW_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id sunxi_uart_match[] = {
	{ .compatible = "allwinner,sun8i-uart", },
	{ .compatible = "allwinner,sun50i-uart", },
	{ .compatible = "allwinner,sun55i-uart", },
	{ .compatible = "allwinner,sun20i-uart", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_uart_match);


static struct platform_driver sw_uport_platform_driver = {
	.probe  = sw_uart_probe,
	.remove = sw_uart_remove,
	.driver = {
		.name  = SUNXI_UART_DEV_NAME,
		.pm    = SERIAL_SW_PM_OPS,
		.of_match_table = sunxi_uart_match,
	},
};

static int __init sunxi_uart_init(void)
{
	int ret;

	ret = uart_register_driver(&sw_uart_driver);
	if (unlikely(ret)) {
		sunxi_info(NULL, "driver initializied\n");
		return ret;
	}

	return platform_driver_register(&sw_uport_platform_driver);
}

static void __exit sunxi_uart_exit(void)
{
	sunxi_info(NULL, "driver exit\n");
#if IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE)
	unregister_console(&sw_console);
#endif
	platform_driver_unregister(&sw_uport_platform_driver);
	uart_unregister_driver(&sw_uart_driver);
}

module_init(sunxi_uart_init);
module_exit(sunxi_uart_exit);

MODULE_AUTHOR("Aaron<leafy.myeh@allwinnertech.com>");
MODULE_DESCRIPTION("Driver for Allwinner UART controller");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1.7");
