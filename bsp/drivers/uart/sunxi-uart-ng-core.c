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

//#define DEBUG
#if IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE) && IS_ENABLED(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

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
#include "sunxi-uart-trace.h"

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

#define SUNXI_RX_MAX_COUNT	256

void sunxi_uart_enable_ier_thri(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (!(uart_port->reg.ier & SUNXI_UART_IER_THRI)) {
		uart_port->reg.ier |= SUNXI_UART_IER_THRI;
		SERIAL_DBG(port->dev, "start tx, ier %x\n", uart_port->reg.ier);
		serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	}
}

void sunxi_uart_disable_ier_thri(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (uart_port->reg.ier & SUNXI_UART_IER_THRI) {
		uart_port->reg.ier &= ~SUNXI_UART_IER_THRI;
		SERIAL_DBG(port->dev, "stop tx, ier %x\n", uart_port->reg.ier);
		serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	}
}

#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
static void sunxi_uart_dump_data(struct sunxi_uart_port *uart_port, char *prompt)
{
	int i, j;
	int head = 0;
	char *buf = uart_port->dump_buff;
	u32 len = uart_port->dump_len;
	static char pbuff[128];
	u32 idx = 0;

	BUG_ON(uart_port->dump_len > MAX_DUMP_SIZE);
	BUG_ON(!uart_port->dump_buff);
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
	uart_port->dump_len = 0;
}
#define SERIAL_DUMP(up, ...) do { \
				if (DEBUG_CONDITION) \
					sunxi_uart_dump_data(up, __VA_ARGS__); \
			} while (0)
#else
#define SERIAL_DUMP(up, ...)	{ up->dump_len = 0; }
#endif

static inline void sunxi_uart_reset(struct sunxi_uart_port *uart_port)
{
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	int ret;
	ret = reset_control_assert(uart_port->reset);
	ret = reset_control_deassert(uart_port->reset);
#endif
}

static unsigned int sunxi_uart_handle_rx(struct sunxi_uart_port *uart_port, unsigned int lsr)
{
	unsigned char ch = 0;
	int max_count = SUNXI_RX_MAX_COUNT;
	char flag;
	struct uart_port *port = &uart_port->port;

	if ((uart_port->dma->use_dma & RX_DMA)) {
		if (lsr & SUNXI_UART_LSR_RXFIFOE) {
			sunxi_info(port->dev, "error:lsr=0x%x\n", lsr);
			lsr = serial_in(&uart_port->port, SUNXI_UART_LSR);
			return lsr;
		}
		return lsr;
	}

	do {
		if (likely(lsr & SUNXI_UART_LSR_DR)) {
			ch = serial_in(&uart_port->port, SUNXI_UART_RBR);
#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
			uart_port->dump_buff[uart_port->dump_len++] = ch;
#endif
		} else
			ch = 0;

		flag = TTY_NORMAL;
		uart_port->port.icount.rx++;

		if (unlikely(lsr & SUNXI_UART_LSR_BRK_ERROR_BITS)) {
			/*
			 * For statistics only
			 */
			if (lsr & SUNXI_UART_LSR_BI) {
				lsr &= ~(SUNXI_UART_LSR_FE | SUNXI_UART_LSR_PE);
				uart_port->port.icount.brk++;
				/*
				 * We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
				if (!ch && uart_handle_break(&uart_port->port))
					goto ignore_char;
			} else if (lsr & SUNXI_UART_LSR_PE)
				uart_port->port.icount.parity++;
			else if (lsr & SUNXI_UART_LSR_FE)
				uart_port->port.icount.frame++;
			if (lsr & SUNXI_UART_LSR_OE)
				uart_port->port.icount.overrun++;

			/*
			 * Mask off conditions which should be ignored.
			 */
			lsr &= uart_port->port.read_status_mask;
#if IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE)
			if (sunxi_is_console_port(&uart_port->port)) {
				/* Recover the break flag from console xmit */
				lsr |= uart_port->lsr_break_flag;
			}
#endif
			if (lsr & SUNXI_UART_LSR_BI)
				flag = TTY_BREAK;
			else if (lsr & SUNXI_UART_LSR_PE)
				flag = TTY_PARITY;
			else if (lsr & SUNXI_UART_LSR_FE)
				flag = TTY_FRAME;
		}
		if (uart_handle_sysrq_char(&uart_port->port, ch))
			goto ignore_char;
		if (uart_port->id != 0)
			trace_uart_data_rx(uart_port, ch, lsr);
		SERIAL_DBG(uart_port->port.dev, "receive data 0x%x\n", ch);
		uart_insert_char(&uart_port->port, lsr, SUNXI_UART_LSR_OE, ch, flag);
ignore_char:
		lsr = serial_in(&uart_port->port, SUNXI_UART_LSR);
	} while ((lsr & (SUNXI_UART_LSR_DR | SUNXI_UART_LSR_BI)) && (max_count-- > 0));

	SERIAL_DUMP(uart_port, "Rx");
	spin_unlock(&uart_port->port.lock);
	trace_uart_count_cpu(uart_port, SUNXI_RX_MAX_COUNT - max_count);
	tty_flip_buffer_push(&uart_port->port.state->port);
	spin_lock(&uart_port->port.lock);

	return lsr;
}

static void sunxi_uart_stop_tx(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	struct sunxi_uart_dma *uart_dma = uart_port->dma;

	if (uart_dma->use_dma & TX_DMA)
		sunxi_uart_stop_dma_tx(uart_port);

	sunxi_uart_disable_ier_thri(port);
}

static void sunxi_uart_start_tx(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (!((uart_port->dma->use_dma & TX_DMA) && uart_port->dma->tx_dma_used))
		sunxi_uart_enable_ier_thri(port);
}

static void sunxi_uart_handle_tx(struct sunxi_uart_port *uart_port)
{
	struct circ_buf *xmit = &uart_port->port.state->xmit;
	int count;

	if (uart_port->port.x_char) {
		serial_out(&uart_port->port, uart_port->port.x_char, SUNXI_UART_THR);
		uart_port->port.icount.tx++;
		uart_port->port.x_char = 0;
#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
		uart_port->dump_buff[uart_port->dump_len++] = uart_port->port.x_char;
		SERIAL_DUMP(uart_port, "Tx");
#endif
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&uart_port->port)) {
		sunxi_uart_stop_tx(&uart_port->port);
		return;
	}

	if (uart_port->dma->use_dma & TX_DMA) {
		if (SERIAL_CIRC_CNT_TO_END(xmit) >= (uart_port->port.fifosize / 2)) {
			sunxi_uart_start_dma_tx(uart_port);
			return;
		}
	}

	count = uart_port->port.fifosize / 2;
	do {
#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
		uart_port->dump_buff[uart_port->dump_len++] = xmit->buf[xmit->tail];
#endif
		serial_out(&uart_port->port, xmit->buf[xmit->tail], SUNXI_UART_THR);
		SERIAL_DBG(uart_port->port.dev, "write tx fifo 0x%x\n", xmit->buf[xmit->tail]);
		if (uart_port->id != 0)
			trace_uart_data_tx(uart_port, xmit->buf[xmit->tail], 0);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		uart_port->port.icount.tx++;
		if (uart_circ_empty(xmit)) {
			break;
		}
	} while (--count > 0);

	SERIAL_DUMP(uart_port, "Tx");
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock(&uart_port->port.lock);
		uart_write_wakeup(&uart_port->port);
		spin_lock(&uart_port->port.lock);
	}
	if (uart_circ_empty(xmit))
		sunxi_uart_stop_tx(&uart_port->port);
}

static unsigned int sunxi_uart_modem_status(struct sunxi_uart_port *uart_port)
{
	struct uart_port *port = &uart_port->port;
	unsigned int status = serial_in(&uart_port->port, SUNXI_UART_MSR);

	status |= uart_port->msr_saved_flags;
	uart_port->msr_saved_flags = 0;

	if (status & SUNXI_UART_MSR_ANY_DELTA && uart_port->reg.ier & SUNXI_UART_IER_MSI &&
	    uart_port->port.state != NULL) {
		if (status & SUNXI_UART_MSR_TERI)
			uart_port->port.icount.rng++;
		if (status & SUNXI_UART_MSR_DDSR)
			uart_port->port.icount.dsr++;
		if (status & SUNXI_UART_MSR_DDCD)
			uart_handle_dcd_change(&uart_port->port, status & SUNXI_UART_MSR_DCD);
		if (!(uart_port->reg.mcr & SUNXI_UART_MCR_AFE) && status & SUNXI_UART_MSR_DCTS)
			uart_handle_cts_change(&uart_port->port, status & SUNXI_UART_MSR_CTS);

		wake_up_interruptible(&uart_port->port.state->port.delta_msr_wait);
	}

	SERIAL_DBG(port->dev, "modem status: %x\n", status);
	return status;
}

static void sunxi_uart_force_lcr(struct sunxi_uart_port *uart_port, unsigned msecs)
{
	unsigned long expire = jiffies + msecs_to_jiffies(msecs);
	struct uart_port *port = &uart_port->port;

	/* hold tx so that uart will update lcr and baud in the gap of rx */
	serial_out(port, SUNXI_UART_HALT_HTX|SUNXI_UART_HALT_FORCECFG, SUNXI_UART_HALT);
	serial_out(port, uart_port->reg.lcr|SUNXI_UART_LCR_DLAB, SUNXI_UART_LCR);
	serial_out(port, uart_port->reg.dll, SUNXI_UART_DLL);
	serial_out(port, uart_port->reg.dlh, SUNXI_UART_DLH);
	serial_out(port, SUNXI_UART_HALT_HTX|SUNXI_UART_HALT_FORCECFG|SUNXI_UART_HALT_LCRUP, SUNXI_UART_HALT);
	while (time_before(jiffies, expire) && (serial_in(port, SUNXI_UART_HALT) & SUNXI_UART_HALT_LCRUP))
		;

	/*
	 * In fact there are two DLABs(DLAB and DLAB_BAK) in the hardware implementation.
	 * The DLAB_BAK is sellected only when SW_UART_HALT_FORCECFG is set to 1,
	 * and this bit can be access no matter uart is busy or not.
	 * So we select the DLAB_BAK always by leaving SW_UART_HALT_FORCECFG to be 1.
	 */
	serial_out(port, uart_port->reg.lcr, SUNXI_UART_LCR);
	serial_out(port, SUNXI_UART_HALT_FORCECFG, SUNXI_UART_HALT);
}

static void sunxi_uart_force_idle(struct sunxi_uart_port *uart_port)
{
	struct uart_port *port = &uart_port->port;

	if (uart_port->reg.fcr & SUNXI_UART_FCR_FIFO_EN) {
		serial_out(port, SUNXI_UART_FCR_FIFO_EN, SUNXI_UART_FCR);
		serial_out(port, SUNXI_UART_FCR_TXFIFO_RST
				| SUNXI_UART_FCR_RXFIFO_RST
				| SUNXI_UART_FCR_FIFO_EN, SUNXI_UART_FCR);
		serial_out(port, 0, SUNXI_UART_FCR);
	}

	serial_out(port, uart_port->reg.fcr, SUNXI_UART_FCR);
	(void)serial_in(port, SUNXI_UART_FCR);
}

/*
 * We should clear busy interupt, busy state and reset lcr,
 * but we should be careful not to introduce a new busy interrupt.
 */
static void sunxi_uart_handle_busy(struct sunxi_uart_port *uart_port)
{
	struct uart_port *port = &uart_port->port;

	(void)serial_in(port, SUNXI_UART_USR);

	/*
	 * Before reseting lcr, we should ensure than uart is not in busy
	 * state. Otherwise, a new busy interrupt will be introduced.
	 * It is wise to set uart into loopback mode, since it can cut down the
	 * serial in, then we should reset fifo(in my test, busy state
	 * (SUNXI_UART_USR_BUSY) can't be cleard until the fifo is empty).
	 */
	serial_out(port, uart_port->reg.mcr | SUNXI_UART_MCR_LOOP, SUNXI_UART_MCR);
	sunxi_uart_force_idle(uart_port);
	serial_out(port, uart_port->reg.lcr, SUNXI_UART_LCR);
	serial_out(port, uart_port->reg.mcr, SUNXI_UART_MCR);
}

static irqreturn_t sunxi_uart_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned int iir = 0, lsr = 0;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	iir = serial_in(port, SUNXI_UART_IIR) & SUNXI_UART_IIR_IID_MASK;
	lsr = serial_in(port, SUNXI_UART_LSR);
	SERIAL_DBG(port->dev, "irq: iir %x lsr %x\n", iir, lsr);
	if (uart_port->id != 0)
		trace_uart_irq_status(uart_port, iir, lsr, serial_in(port, SUNXI_UART_TFL),
				      serial_in(port, SUNXI_UART_RFL));
	if (iir == SUNXI_UART_IIR_IID_BUSBSY) {
		sunxi_uart_handle_busy(uart_port);
	} else {
		if (lsr & (SUNXI_UART_LSR_DR | SUNXI_UART_LSR_BI))
			lsr = sunxi_uart_handle_rx(uart_port, lsr);
		/* has charto irq but no dr lsr? just read and ignore */
		else if (iir & SUNXI_UART_IIR_IID_CHARTO)
			serial_in(&uart_port->port, SUNXI_UART_RBR);
		sunxi_uart_modem_status(uart_port);
#if IS_ENABLED(CONFIG_SW_UART_PTIME_MODE)
		if (iir == SUNXI_UART_IIR_IID_THREMP)
#else
		if (lsr & SUNXI_UART_LSR_THRE)
#endif
			sunxi_uart_handle_tx(uart_port);
	}

	spin_unlock_irqrestore(&port->lock, flags);

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

static int sunxi_uart_check_baudset(struct uart_port *port, unsigned int baud)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
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
				uart_port->id, port->uartclk, baud);
			return -1;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(baud_set); i++) {
			if (baud == baud_set[i].baud)
				break;
		}

		if (i == ARRAY_SIZE(baud_set)) {
			sunxi_info(port->dev, "uart%d, baud %d beyond rance\n", uart_port->id, baud);
			return -1;
		}

		setsel = &baud_set[i];
		if (port->uartclk < setsel->uartclk_min
			|| port->uartclk > setsel->uartclk_max) {
			sunxi_info(port->dev, "uart%d, select set %d, baud %d, uartclk %d beyond rance[%d, %d]\n",
				uart_port->id, i, baud, port->uartclk,
				setsel->uartclk_min, setsel->uartclk_max);
			return -1;
		}
	}
	return 0;
}

#define BOTH_EMPTY    (SUNXI_UART_LSR_TEMT | SUNXI_UART_LSR_THRE)
static void wait_for_xmitr(struct sunxi_uart_port *uart_port)
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
		status = serial_in(&uart_port->port, offs);
		if (serial_in(&uart_port->port, SUNXI_UART_LSR) & SUNXI_UART_LSR_BI)
			uart_port->lsr_break_flag = SUNXI_UART_LSR_BI;
		if (--tmout == 0)
			break;
		udelay(1);
	} while ((status & mask) != mask);

	/* CTS is unsupported by the 2-line UART, so ignore it. */
	if (uart_port->pdata->io_num == 2)
		return;

	/* Wait up to 500ms for flow control if necessary */
	if (uart_port->port.flags & UPF_CONS_FLOW) {
		tmout = 500000;
		for (tmout = 1000000; tmout; tmout--) {
			unsigned int msr = serial_in(&uart_port->port, SUNXI_UART_MSR);

			uart_port->msr_saved_flags |= msr & MSR_SAVE_FLAGS;
			if (msr & SUNXI_UART_MSR_CTS)
				break;

			udelay(1);
		}
	}
}

/* Enable or disable the RS485 support */
static void sunxi_uart_config_rs485(struct uart_port *port, struct serial_rs485 *rs485conf)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	uart_port->rs485conf = *rs485conf;

	uart_port->reg.mcr &= ~SUNXI_UART_MCR_MODE_MASK;
	if (rs485conf->flags & SER_RS485_ENABLED) {
		SERIAL_DBG(port->dev, "setting to rs485\n");
		uart_port->reg.mcr |= SUNXI_UART_MCR_MODE_RS485;

		/*
		 * In NMM mode and no 9th bit(default RS485 mode), uart receive
		 * all the bytes into FIFO before receveing an address byte
		 */
		uart_port->rs485 |= SUNXI_UART_RS485_RXBFA;
	} else {
		SERIAL_DBG(port->dev, "setting to uart\n");
		uart_port->reg.mcr |= SUNXI_UART_MCR_MODE_UART;
		uart_port->rs485 = 0;
	}

	serial_out(port, uart_port->reg.mcr, SUNXI_UART_MCR);
	serial_out(port, uart_port->rs485, SUNXI_UART_RS485);
}

static unsigned int sunxi_uart_tx_empty(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned long flags = 0;
	unsigned int ret = 0;

	spin_lock_irqsave(&uart_port->port.lock, flags);
	ret = (serial_in(port, SUNXI_UART_USR) & SUNXI_UART_USR_TFE) ? TIOCSER_TEMT : 0;
	spin_unlock_irqrestore(&uart_port->port.lock, flags);
	return ret;
}

static void sunxi_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned int mcr = 0;

	if (mctrl & TIOCM_RTS)
		mcr |= SUNXI_UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= SUNXI_UART_MCR_DTR;
	if (mctrl & TIOCM_LOOP)
		mcr |= SUNXI_UART_MCR_LOOP;
	uart_port->reg.mcr &= ~(SUNXI_UART_MCR_RTS|SUNXI_UART_MCR_DTR|SUNXI_UART_MCR_LOOP);
	uart_port->reg.mcr |= mcr;

	if (uart_port->loopback)
		uart_port->reg.mcr |= SUNXI_UART_MCR_LOOP;

	SERIAL_DBG(port->dev, "set mcr %x\n", mcr);
	serial_out(port, uart_port->reg.mcr, SUNXI_UART_MCR);
}

static unsigned int sunxi_uart_get_mctrl(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned int msr;
	unsigned int ret = 0;

	msr = sunxi_uart_modem_status(uart_port);
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

static void sunxi_uart_stop_rx(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	struct sunxi_uart_dma *uart_dma = uart_port->dma;
	if (uart_dma->use_dma & RX_DMA) {
		sunxi_uart_stop_dma_rx(uart_port);
	}

	if (uart_port->reg.ier & SUNXI_UART_IER_RLSI) {
		uart_port->reg.ier &= ~SUNXI_UART_IER_RLSI;
		SERIAL_DBG(port->dev, "stop rx, ier %x\n", uart_port->reg.ier);
		uart_port->port.read_status_mask &= ~SUNXI_UART_LSR_DR;
		serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	}
}

static void sunxi_uart_enable_ms(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (!(uart_port->reg.ier & SUNXI_UART_IER_MSI)) {
		uart_port->reg.ier |= SUNXI_UART_IER_MSI;
		SERIAL_DBG(port->dev, "en msi, ier %x\n", uart_port->reg.ier);
		serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	}
}

static void sunxi_uart_break_ctl(struct uart_port *port, int break_state)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state == -1)
		uart_port->reg.lcr |= SUNXI_UART_LCR_SBC;
	else
		uart_port->reg.lcr &= ~SUNXI_UART_LCR_SBC;
	serial_out(port, uart_port->reg.lcr, SUNXI_UART_LCR);
	spin_unlock_irqrestore(&port->lock, flags);
}

static int sunxi_uart_startup(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	int ret;

	SERIAL_DBG(port->dev, "start up ...\n");

	ret = request_irq(port->irq, sunxi_uart_irq, 0, uart_port->name, port);
	if (unlikely(ret)) {
		sunxi_info(port->dev, "uart%d cannot get irq %d\n", uart_port->id, port->irq);
		return ret;
	}

	uart_port->msr_saved_flags = 0;
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

	if (uart_port->dma->use_dma & TX_DMA) {
		if (uart_port->port.state->xmit.buf !=
						uart_port->dma->tx_buffer){
			free_page((unsigned long)uart_port->port.state->xmit.buf);
			uart_port->port.state->xmit.buf =
						uart_port->dma->tx_buffer;
		}
	} else {
		uart_port->reg.ier = 0;
		serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	}

	sunxi_uart_config_rs485(port, &uart_port->rs485conf);

	return 0;
}

static void sunxi_uart_shutdown(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	SERIAL_DBG(port->dev, "shut down ...\n");
	if (uart_port->dma->use_dma & TX_DMA)
		uart_port->port.state->xmit.buf = NULL;

	uart_port->reg.ier = 0;
	uart_port->reg.lcr = 0;
	uart_port->reg.mcr = 0;
	uart_port->reg.fcr = 0;
	serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	free_irq(port->irq, port);
}

static void sunxi_uart_flush_buffer(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	SERIAL_DBG(port->dev, "flush buffer...\n");
	serial_out(port, uart_port->reg.fcr|SUNXI_UART_FCR_TXFIFO_RST, SUNXI_UART_FCR);
}

static void sunxi_uart_set_termios(struct uart_port *port, struct ktermios *termios,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 12))
			    const struct ktermios *old)
#else
			    struct ktermios *old)
#endif  /* LINUX_VERSION_CODE */
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned long flags;
	unsigned int baud, quot, lcr = 0, dll, dlh;

	/* stop dma tx, which might make the uart be busy while some
	 * registers are set
	 */
	if (uart_port->dma->tx_dma_used)
		sunxi_uart_stop_dma_tx(uart_port);

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
	sunxi_uart_check_baudset(port, baud);
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
	sunxi_uart_reset(uart_port);

	if (baud <= 9600)
		uart_port->reg.fcr = SUNXI_UART_FCR_RXTRG_1CH
				| SUNXI_UART_FCR_TXTRG_1_2
				| SUNXI_UART_FCR_FIFO_EN;
	else
		uart_port->reg.fcr = SUNXI_UART_FCR_RXTRG_1_2
				| SUNXI_UART_FCR_TXTRG_1_2
				| SUNXI_UART_FCR_FIFO_EN;

	serial_out(port, uart_port->reg.fcr, SUNXI_UART_FCR);


	uart_port->reg.lcr = lcr;
	uart_port->reg.dll = dll;
	uart_port->reg.dlh = dlh;
	sunxi_uart_force_lcr(uart_port, 50);

	/* clear rxfifo after set lcr & baud to discard redundant data */
	serial_out(port, uart_port->reg.fcr|SUNXI_UART_FCR_RXFIFO_RST, SUNXI_UART_FCR);
	port->ops->set_mctrl(port, port->mctrl);

	uart_port->reg.ier = SUNXI_UART_IER_RLSI | SUNXI_UART_IER_RDI;
#if IS_ENABLED(CONFIG_SW_UART_PTIME_MODE)
	uart_port->reg.ier |= SUNXI_UART_IER_PTIME;
#endif
	if (uart_port->dma->use_dma & RX_DMA) {
		/* disable the receive data interrupt */
		uart_port->reg.ier &= ~SUNXI_UART_IER_RDI;
		sunxi_uart_start_dma_rx(uart_port);
	} else {
		SERIAL_DBG(port->dev, "uart_port->dma->use_dma not set\n");
	}

	/* flow control */
	uart_port->reg.mcr &= ~SUNXI_UART_MCR_AFE;
	port->status &= ~(UPSTAT_AUTOCTS | UPSTAT_AUTORTS);
	if (termios->c_cflag & CRTSCTS) {
		port->status |= UPSTAT_AUTOCTS | UPSTAT_AUTORTS;
		uart_port->reg.mcr |= SUNXI_UART_MCR_AFE;
	}
	serial_out(port, uart_port->reg.mcr, SUNXI_UART_MCR);

	/*
	 * CTS flow control flag and modem status interrupts
	 */
	uart_port->reg.ier &= ~SUNXI_UART_IER_MSI;
	if (UART_ENABLE_MS(port, termios->c_cflag))
		uart_port->reg.ier |= SUNXI_UART_IER_MSI;
	serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	/* Must save the current config for the resume of console(no tty user). */
	if (sunxi_is_console_port(port))
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
			uart_port->reg.lcr, uart_port->reg.fcr, uart_port->reg.mcr,
			uart_port->reg.dll, uart_port->reg.dlh);
}

static const char *sunxi_uart_type(struct uart_port *port)
{
	return "SUNXI";
}

static int sunxi_uart_select_gpio_state(struct pinctrl *pctrl, char *name, u32 no, struct device *dev)
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

static int sunxi_uart_request_gpio(struct sunxi_uart_port *uart_port)
{
	struct uart_port *port = &uart_port->port;
	if (uart_port->card_print)
		return 0;

	uart_port->pctrl = devm_pinctrl_get(port->dev);

	if (IS_ERR_OR_NULL(uart_port->pctrl)) {
		sunxi_info(port->dev, "UART%d devm_pinctrl_get() failed! return %ld\n", uart_port->id, PTR_ERR(uart_port->pctrl));
		return -1;
	}

	return sunxi_uart_select_gpio_state(uart_port->pctrl, PINCTRL_STATE_DEFAULT, uart_port->id, port->dev);
}

static void sunxi_uart_release_gpio(struct sunxi_uart_port *uart_port)
{
	if (uart_port->card_print)
		return;

	devm_pinctrl_put(uart_port->pctrl);
	uart_port->pctrl = NULL;
}

static void sunxi_uart_release_port(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	struct platform_device *pdev;
	struct resource	*mem_res;

	SERIAL_DBG(port->dev, "release port(iounmap & release io)\n");

	pdev = to_platform_device(port->dev);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		sunxi_info(port->dev, "uart%d, get MEM resource failed\n", uart_port->id);
		return;
	}

	/* release memory resource */
	release_mem_region(mem_res->start, resource_size(mem_res));
	iounmap(port->membase);
	port->membase = NULL;

	/* release io resource */
	sunxi_uart_release_gpio(uart_port);
}

static int sunxi_uart_request_port(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	struct platform_device *pdev;
	struct resource	*mem_res;
	int ret;

	SERIAL_DBG(port->dev, "request port(ioremap & request io) %d\n", uart_port->id);

	pdev = to_platform_device(port->dev);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		sunxi_info(port->dev, "uart%d, get MEM resource failed\n", uart_port->id);
		ret = -ENXIO;
	}

	/* request memory resource */
	if (!request_mem_region(mem_res->start, resource_size(mem_res), SUNXI_UART_DEV_NAME)) {
		sunxi_info(port->dev, "uart%d, request mem region failed\n", uart_port->id);
		return -EBUSY;
	}

	port->membase = ioremap(mem_res->start, resource_size(mem_res));
	if (!port->membase) {
		sunxi_info(port->dev, "uart%d, ioremap failed\n", uart_port->id);
		release_mem_region(mem_res->start, resource_size(mem_res));
		return -EBUSY;
	}

	/* request io resource */
	ret = sunxi_uart_request_gpio(uart_port);
	if (ret < 0) {
		release_mem_region(mem_res->start, resource_size(mem_res));
		return ret;
	}

	return 0;
}

static void sunxi_uart_config_port(struct uart_port *port, int flags)
{
	int ret;

	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_SUNXI;
		ret = sunxi_uart_request_port(port);
		if (ret)
			return;
	}
}

static int sunxi_uart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_SUNXI))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;
	return 0;
}

static int sunxi_uart_ioctl(struct uart_port *port, unsigned int cmd,
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
		if (copy_from_user(&rs485conf, (struct serial_rs485 *)arg,
				   sizeof(rs485conf)))
			return -EFAULT;

		spin_lock_irqsave(&port->lock, flags);
		sunxi_uart_config_rs485(port, &rs485conf);
		spin_unlock_irqrestore(&port->lock, flags);
		break;

	case TIOCGRS485:
		if (copy_to_user((struct serial_rs485 *) arg,
				 &(UART_TO_SPORT(port)->rs485conf),
				 sizeof(rs485conf)))
			return -EFAULT;
		break;

	default:
		return -ENOIOCTLCMD;
	}

	return 0;
}

static void sunxi_uart_pm(struct uart_port *port, unsigned int state,
		      unsigned int oldstate)
{
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	int ret;
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	SERIAL_DBG(port->dev, "PM state %d -> %d\n", oldstate, state);

	switch (state) {
	case UART_PM_STATE_ON: /* Power up */
		ret = clk_prepare_enable(uart_port->mclk);
		if (ret) {
			sunxi_info(port->dev, "uart%d release reset failed\n", uart_port->id);
		}
		break;
	case UART_PM_STATE_OFF: /* Power down */
		clk_disable_unprepare(uart_port->mclk);
		break;
	default:
		sunxi_info(port->dev, "uart%d, Unknown PM state %d\n", uart_port->id, state);
	}
#endif
}

#if IS_ENABLED(CONFIG_CONSOLE_POLL)
/*
 * Console polling routines for writing and reading from the uart while
 * in an interrupt or debug context.
 */

static int sunxi_get_poll_char(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned int lsr = serial_in(port, SUNXI_UART_LSR);

	if (!(lsr & SUNXI_UART_LSR_DR)) {
		return NO_POLL_CHAR;
	}

	return serial_in(port, SUNXI_UART_RBR);
}


static void sunxi_put_poll_char(struct uart_port *port,
			unsigned char c)
{
	unsigned int ier;
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	/*
	 * First save the IER then disable the interrupts.
	 */
	ier = serial_in(port, SUNXI_UART_IER);

	serial_out(port, 0, SUNXI_UART_IER);
	wait_for_xmitr(uart_port);

	serial_out(port, c, SUNXI_UART_THR);
	if (c == 10) {
		wait_for_xmitr(uart_port);
		serial_out(port, 13, SUNXI_UART_THR);
	}
	/*
	 * Finally, wait for transmitter to become empty
	 * and restore the IER
	 */
	wait_for_xmitr(uart_port);
	serial_out(port, ier, SUNXI_UART_IER);
}

#endif /* CONFIG_CONSOLE_POLL */

/* Disable receive interrupts to throttle receive data, which could
 * avoid receive data overrun
 */
static void sunxi_uart_throttle(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	uart_port->reg.ier &= ~(SUNXI_UART_IER_RLSI | SUNXI_UART_IER_RDI);
	serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	uart_port->throttled = true;
	spin_unlock_irqrestore(&port->lock, flags);
}

static void sunxi_uart_unthrottle(struct uart_port *port)
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	uart_port->reg.ier |= SUNXI_UART_IER_RLSI | SUNXI_UART_IER_RDI;
	serial_out(port, uart_port->reg.ier, SUNXI_UART_IER);
	uart_port->throttled = false;
	spin_unlock_irqrestore(&port->lock, flags);
}

static struct uart_ops sunxi_uart_ops = {
	.tx_empty = sunxi_uart_tx_empty,
	.set_mctrl = sunxi_uart_set_mctrl,
	.get_mctrl = sunxi_uart_get_mctrl,
	.stop_tx = sunxi_uart_stop_tx,
	.start_tx = sunxi_uart_start_tx,
	.stop_rx = sunxi_uart_stop_rx,
	.enable_ms = sunxi_uart_enable_ms,
	.break_ctl = sunxi_uart_break_ctl,
	.startup = sunxi_uart_startup,
	.shutdown = sunxi_uart_shutdown,
	.flush_buffer = sunxi_uart_flush_buffer,
	.set_termios = sunxi_uart_set_termios,
	.type = sunxi_uart_type,
	.release_port = sunxi_uart_release_port,
	.request_port = sunxi_uart_request_port,
	.config_port = sunxi_uart_config_port,
	.verify_port = sunxi_uart_verify_port,
	.ioctl = sunxi_uart_ioctl,
	.pm = sunxi_uart_pm,
#if IS_ENABLED(CONFIG_CONSOLE_POLL)
	.poll_get_char = sunxi_get_poll_char,
	.poll_put_char = sunxi_put_poll_char,
#endif
	.throttle = sunxi_uart_throttle,
	.unthrottle = sunxi_uart_unthrottle,
};

static int sunxi_uart_regulator_request(struct sunxi_uart_port *uart_port, struct sunxi_uart_pdata *pdata,
		struct device *dev)
{
	struct regulator *regu = NULL;
	struct uart_port *port = &uart_port->port;

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
		sunxi_info(port->dev, "get regulator %s failed!\n", pdata->regulator_id);
		return -1;
	}
	pdata->regulator = regu;
	return 0;
}

static void sunxi_uart_regulator_release(struct sunxi_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return;

	regulator_put(pdata->regulator);
	pdata->regulator = NULL;
}

static int sunxi_uart_regulator_enable(struct sunxi_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_enable(pdata->regulator) != 0)
		return -1;

	return 0;
}

static int sunxi_uart_regulator_disable(struct sunxi_uart_pdata *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_disable(pdata->regulator) != 0)
		return -1;

	return 0;
}

static struct sunxi_uart_port sunxi_uart_ports[SUNXI_UART_NUM];
static struct sunxi_uart_pdata uart_port_pdata[SUNXI_UART_NUM];

static ssize_t sunxi_uart_dev_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	struct sunxi_uart_pdata *pdata = (struct sunxi_uart_pdata *)dev->platform_data;

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
		uart_port->id, uart_port->name, port->irq,
		uart_port->pdata->io_num,
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
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (!strncmp(buf, "enable", 6))
		enable = 1;

	sunxi_debug(dev, "Set loopback: %d \n", enable);

	if (enable) {
		uart_port->loopback = true;
		writel(mcr|SUNXI_UART_MCR_LOOP, port->membase + SUNXI_UART_MCR);
	} else {
		uart_port->loopback = false;
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
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);
	u32 dl = (u32)uart_port->reg.dlh << 8 | (u32)uart_port->reg.dll;

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
		uart_port->reg.ier, uart_port->reg.lcr, uart_port->reg.mcr,
		uart_port->reg.fcr, uart_port->reg.dll, uart_port->reg.dlh,
		(uart_port->port.uartclk>>4)/dl, dl,
		uart_port->port.icount.tx,
		uart_port->port.icount.rx,
		uart_port->port.icount.parity,
		uart_port->port.icount.frame,
		uart_port->port.icount.overrun,
		uart_port->throttled);
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
static struct uart_port *sunxi_console_get_port(struct console *co)
{
	struct uart_port *port = NULL;
	int i, used;

	for (i = 0; i < SUNXI_UART_NUM; i++) {
		used = uart_port_pdata[i].used;
		port = &sunxi_uart_ports[i].port;
		if ((used == 1) && (port->line == co->index))
			return port;
	}
	return NULL;
}

static void sunxi_console_putchar(struct uart_port *port,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 12))
				unsigned char c)
#else
				int c)
#endif  /* LINUX_VERSION_CODE */
{
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	wait_for_xmitr(uart_port);
	serial_out(port, c, SUNXI_UART_THR);
}

static void sunxi_console_write(struct console *co, const char *s,
			      unsigned int count)
{
	struct uart_port *port = NULL;
	struct sunxi_uart_port *uart_port;
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	BUG_ON(co->index < 0 || co->index >= SUNXI_UART_NUM);

	port = sunxi_console_get_port(co);
	if (port == NULL)
		return;
	uart_port = UART_TO_SPORT(port);

	if (port->sysrq || oops_in_progress)
		locked = spin_trylock_irqsave(&port->lock, flags);
	else
		spin_lock_irqsave(&port->lock, flags);

	ier = serial_in(port, SUNXI_UART_IER);
	serial_out(port, 0, SUNXI_UART_IER);

	uart_console_write(port, s, count, sunxi_console_putchar);
	wait_for_xmitr(uart_port);
	serial_out(port, ier, SUNXI_UART_IER);

	if (locked)
		spin_unlock_irqrestore(&port->lock, flags);
}

static int sunxi_console_setup(struct console *co, char *options)
{
	struct uart_port *port = NULL;
	struct sunxi_uart_port *uart_port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (unlikely(co->index >= SUNXI_UART_NUM || co->index < 0))
		return -ENXIO;

	port = sunxi_console_get_port(co);
	if (port == NULL)
		return -ENODEV;
	uart_port = UART_TO_SPORT(port);
	if (!port->iobase && !port->membase)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	sunxi_info(port->dev, "console setup baud %d parity %c bits %d, flow %c\n",
			baud, parity, bits, flow);
	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver sunxi_uart_driver;
static struct console sunxi_console = {
#if IS_ENABLED(CONFIG_SERIAL_8250)
	.name = "ttyAS",
#else
	.name = "ttyS",
#endif
	.write = sunxi_console_write,
	.device = uart_console_device,
	.setup = sunxi_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &sunxi_uart_driver,
};

#define SW_CONSOLE	(&sunxi_console)
#else
#define SW_CONSOLE	NULL
#endif

static struct uart_driver sunxi_uart_driver = {
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

static int sunxi_uart_request_resource(struct sunxi_uart_port *uart_port, struct sunxi_uart_pdata *pdata,
		struct device *dev)
{
	struct uart_port *port = &uart_port->port;
	SERIAL_DBG(port->dev, "get system resource(clk & IO)\n");

	if (sunxi_uart_regulator_request(uart_port, pdata, dev) < 0) {
		sunxi_info(port->dev, "uart%d request regulator failed!\n", uart_port->id);
		return -ENXIO;
	}
	sunxi_uart_regulator_enable(pdata);

#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
	uart_port->dump_buff = kmalloc(MAX_DUMP_SIZE, GFP_KERNEL);
	if (!uart_port->dump_buff) {
		sunxi_info(port->dev, "uart%d fail to alloc dump buffer\n", uart_port->id);
	}
#endif

	return 0;
}

static int sunxi_uart_release_resource(struct sunxi_uart_port *uart_port, struct sunxi_uart_pdata *pdata)
{
	struct uart_port *port = &uart_port->port;
	SERIAL_DBG(port->dev, "put system resource(clk & IO)\n");

#if IS_ENABLED(CONFIG_SW_UART_DUMP_DATA)
	kfree(uart_port->dump_buff);
	uart_port->dump_buff = NULL;
	uart_port->dump_len = 0;
#endif

	clk_disable_unprepare(uart_port->mclk);
	clk_put(uart_port->mclk);

	sunxi_uart_regulator_disable(pdata);
	sunxi_uart_regulator_release(pdata);

	return 0;
}

struct platform_device *sunxi_uart_get_pdev(int uart_id)
{
	if (sunxi_uart_ports[uart_id].port.dev)
		return to_platform_device(sunxi_uart_ports[uart_id].port.dev);
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

static int sunxi_uart_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct uart_port *port;
	struct sunxi_uart_port *uart_port;
	struct sunxi_uart_pdata *pdata;
	struct resource *res;
	char uart_para[16] = {0};
	const char *uart_string;
	int ret = -1;
	struct device_node *apk_np = of_find_node_by_name(NULL, "auto_print");
	const char *apk_sta = NULL;
	int irq;
	struct dma_chan *dma_chan_rx, *dma_chan_tx;

	pdev->id = of_alias_get_id(np, "serial");
	if (pdev->id < 0 || pdev->id >= SUNXI_UART_NUM) {
		sunxi_info(&pdev->dev, "get alias id err or exceed supported uart controllers\n");
		return -EINVAL;
	}

	port = &sunxi_uart_ports[pdev->id].port;
	port->dev = &pdev->dev;
	pdata = &uart_port_pdata[pdev->id];
	uart_port = UART_TO_SPORT(port);
	uart_port->pdata = pdata;
	uart_port->id = pdev->id;
	uart_port->reg.ier = 0;
	uart_port->reg.lcr = 0;
	uart_port->reg.mcr = 0;
	uart_port->reg.fcr = 0;
	uart_port->reg.dll = 0;
	uart_port->reg.dlh = 0;
	snprintf(uart_port->name, 16, SUNXI_UART_DEV_NAME"%d", pdev->id);
	pdev->dev.init_name = uart_port->name;
	pdev->dev.platform_data = uart_port->pdata;

	snprintf(uart_para, sizeof(uart_para), "uart%d_regulator", pdev->id);
	ret = of_property_read_string(np, uart_para, &uart_string);
	if (!ret)
		strncpy(pdata->regulator_id, uart_string, 16);

	/* request system resource and init them */
	ret = sunxi_uart_request_resource(uart_port, pdev->dev.platform_data, &pdev->dev);
	if (unlikely(ret)) {
		sunxi_info(&pdev->dev, "uart%d error to get resource\n", pdev->id);
		return -ENXIO;
	}


#if IS_ENABLED(CONFIG_AW_IC_BOARD)
	uart_port->reset = devm_reset_control_get(&pdev->dev, NULL);
	if (IS_ERR(uart_port->reset)) {
		printk("get reset clk error\n");
		return -EINVAL;
	}
	uart_port->mclk = of_clk_get(np, 0);
	if (IS_ERR(uart_port->mclk)) {
		sunxi_info(&pdev->dev, "uart%d error to get clk\n", pdev->id);
		return -EINVAL;
	}

	ret = reset_control_deassert(uart_port->reset);
	if (ret) {
		printk("deassert clk error, ret:%d\n", ret);
		return ret;
	}

	/* uart clk come from apb2, apb2 default clk is hosc. if change rate
	 * needed, must switch apb2's source clk first and then set its rate
	 * */
	uart_port->sclk = of_clk_get(np, 1);
	if (!IS_ERR(uart_port->sclk)) {
		uart_port->pclk = of_clk_get(np, 2);
		port->uartclk = clk_get_rate(uart_port->sclk);
		/* config a fixed divider before switch source clk for apb2 */
		clk_set_rate(uart_port->sclk, port->uartclk/6);
		/* switch source clock for apb2 */
		clk_set_parent(uart_port->sclk, uart_port->pclk);
		ret = of_property_read_u32(np, "clock-frequency",
					&port->uartclk);
		if (ret) {
			sunxi_info(&pdev->dev, "uart%d get clock-freq failed\n", pdev->id);
			return -EINVAL;
		}
		/* set apb2 clock frequency now */
		clk_set_rate(uart_port->sclk, port->uartclk);
	}

	port->uartclk = clk_get_rate(uart_port->mclk);
#else
	port->uartclk = 24000000;
#endif

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		sunxi_err(&pdev->dev, "uart%d error to get MEM resource\n", pdev->id);
		return -EINVAL;
	}
	port->mapbase = res->start;

	uart_port->dma = devm_kzalloc(&pdev->dev, sizeof(*uart_port->dma), GFP_KERNEL);
	dma_chan_tx = dma_request_chan(uart_port->port.dev, "tx");
	if (IS_ERR(dma_chan_tx)) {
		sunxi_info(&pdev->dev, "cannot get the TX DMA channel!\n");
	} else {
		uart_port->dma->use_dma = TX_DMA;
		dma_release_channel(dma_chan_tx);
	}

	dma_chan_rx = dma_request_chan(uart_port->port.dev, "rx");
	if (IS_ERR(dma_chan_rx)) {
		sunxi_info(&pdev->dev, "cannot get the RX DMA channel!\n");
	} else {
		uart_port->dma->use_dma |= RX_DMA;
		dma_release_channel(dma_chan_rx);
	}

	uart_port->dma->rx_dma_inited = 0;
	uart_port->dma->rx_dma_used = 0;
	uart_port->dma->tx_dma_inited = 0;
	uart_port->dma->tx_dma_used = 0;

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


	if (of_property_read_bool(np, "linux,rs485-enabled-at-boot-time"))
		uart_port->rs485conf.flags |= SER_RS485_ENABLED;

	if (apk_np && !of_property_read_string(apk_np, "status", &apk_sta)
						&& !strcmp(apk_sta, "okay"))
		uart_port->card_print = true;
	else
		uart_port->card_print = false;

	pdata->used = 1;
	port->iotype = UPIO_MEM;
	port->type = PORT_SUNXI;
	port->flags = UPF_BOOT_AUTOCONF;
	port->ops = &sunxi_uart_ops;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	port->has_sysrq = IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE);
#endif
	ret = of_property_read_u32(np, "sunxi,uart-fifosize", &port->fifosize);
	if (ret) {
		sunxi_err(&pdev->dev, "uart%d error to get fifo size property\n", pdev->id);
		port->fifosize = SUNXI_UART_FIFO_SIZE;
	}

	platform_set_drvdata(pdev, port);

	/* set dma config */
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);

	ret = sunxi_uart_init_dma(uart_port);
	if (ret) {
		sunxi_err(&pdev->dev, "uart%d error to malloc dma buffer!\n", pdev->id);
		return -EINVAL;
	}
	sunxi_uart_sysfs(pdev);

	SERIAL_DBG(port->dev, "add uart%d port, port_type %d, uartclk %d\n",
			pdev->id, port->type, port->uartclk);
	return uart_add_one_port(&sunxi_uart_driver, port);
}

static int sunxi_uart_remove(struct platform_device *pdev)
{
	struct sunxi_uart_port *uart_port = platform_get_drvdata(pdev);
	struct uart_port *port = &uart_port->port;

	SERIAL_DBG(port->dev, "release uart%d port\n", uart_port->id);

	if ((uart_port->dma->use_dma & TX_DMA))
		sunxi_uart_release_dma_tx(uart_port);
	if ((uart_port->dma->use_dma & RX_DMA))
		sunxi_uart_release_dma_rx(uart_port);

	sunxi_uart_release_resource(uart_port, pdev->dev.platform_data);
	return 0;
}

/* UART power management code */
#if IS_ENABLED(CONFIG_PM_SLEEP)

#define SW_UART_NEED_SUSPEND(port) \
	((sunxi_is_console_port(port) && (console_suspend_enabled)) \
		|| !sunxi_is_console_port(port))

static int sunxi_uart_suspend(struct device *dev)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (port) {
		sunxi_info(dev, "uart%d suspend\n", uart_port->id);
		uart_suspend_port(&sunxi_uart_driver, port);

		if (SW_UART_NEED_SUSPEND(port)) {
			if (!uart_port->card_print)
				sunxi_uart_select_gpio_state(uart_port->pctrl,
					PINCTRL_STATE_SLEEP, uart_port->id, port->dev);
			sunxi_uart_regulator_disable(dev->platform_data);
		}
	}

	return 0;
}

static int sunxi_uart_resume(struct device *dev)
{
	struct uart_port *port = dev_get_drvdata(dev);
	struct sunxi_uart_port *uart_port = UART_TO_SPORT(port);

	if (port) {
		if (SW_UART_NEED_SUSPEND(port)) {
			sunxi_uart_regulator_enable(dev->platform_data);
			if (!uart_port->card_print)
				sunxi_uart_select_gpio_state(uart_port->pctrl,
					PINCTRL_STATE_DEFAULT, uart_port->id, uart_port->port.dev);
		}
		uart_resume_port(&sunxi_uart_driver, port);
		sunxi_info(dev, "uart%d resume. DLH: %d, DLL: %d. \n", uart_port->id, uart_port->reg.dlh, uart_port->reg.dll);
	}

	return 0;
}

static const struct dev_pm_ops sunxi_uart_pm_ops = {
	.suspend = sunxi_uart_suspend,
	.resume = sunxi_uart_resume,
};
#define SERIAL_SW_PM_OPS	(&sunxi_uart_pm_ops)

#else /* !CONFIG_PM_SLEEP */

#define SERIAL_SW_PM_OPS	NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id sunxi_uart_match[] = {
	{ .compatible = "allwinner,sun55i-uart", },
	{ .compatible = "allwinner,uart-v100", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_uart_match);

static struct platform_driver uart_port_platform_driver = {
	.probe  = sunxi_uart_probe,
	.remove = sunxi_uart_remove,
	.driver = {
		.name  = SUNXI_UART_DEV_NAME,
		.pm    = SERIAL_SW_PM_OPS,
		.of_match_table = sunxi_uart_match,
	},
};

static int __init sunxi_uart_init(void)
{
	int ret;

	ret = uart_register_driver(&sunxi_uart_driver);
	if (unlikely(ret)) {
		sunxi_info(NULL, "driver initializied\n");
		return ret;
	}

	return platform_driver_register(&uart_port_platform_driver);
}

static void __exit sunxi_uart_exit(void)
{
	sunxi_info(NULL, "driver exit\n");
#if IS_ENABLED(CONFIG_AW_SERIAL_CONSOLE)
	unregister_console(&sunxi_console);
#endif
	platform_driver_unregister(&uart_port_platform_driver);
	uart_unregister_driver(&sunxi_uart_driver);
}

module_init(sunxi_uart_init);
module_exit(sunxi_uart_exit);

MODULE_AUTHOR("Emma<liujuan1@allwinnertech.com>");
MODULE_DESCRIPTION("Driver for Allwinner UART NG controller");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1.1");
