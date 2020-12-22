/*
 * drivers/tty/serial/sunxi-uart.h
 * (C) Copyright 2007-2013
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Aaron.Maoye <leafy.myeh@reuuimllatech.com>
 *
 * Some macro and struct of Allwinner UART controller.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2013.6.6 Mintow <duanmintao@allwinnertech.com>
 *    Adapt to support sun8i/sun9i of Allwinner.
 */

#ifndef _SUNXI_UART_H_
#define _SUNXI_UART_H_

#include <linux/regulator/consumer.h>

struct sw_uart_pdata {
	unsigned int used;
	unsigned int base;
	unsigned int irq;
	unsigned int max_ios;
	unsigned int io_num;
	unsigned int port_no;
	char			  regulator_id[16];
	struct regulator *regulator;
};

struct sw_uart_port {
	struct uart_port port;
	char   name[16];
	struct clk *mclk;
	unsigned char id;
	unsigned char ier;
	unsigned char lcr;
	unsigned char mcr;
	unsigned char fcr;
	unsigned char dll;
	unsigned char dlh;
	unsigned char msr_saved_flags;
	unsigned int lsr_break_flag;
	struct sw_uart_pdata *pdata;

	/* for debug */
#define MAX_DUMP_SIZE	1024
	unsigned int dump_len;
	char* dump_buff;
	struct proc_dir_entry *proc_root;
	struct proc_dir_entry *proc_info;

	struct pinctrl		 *pctrl;
};

/* register offset define */
#define SW_UART_RBR (0x00)
#define SW_UART_THR (0x00)
#define SW_UART_DLL (0x00)
#define SW_UART_DLH (0x04)
#define SW_UART_IER (0x04)
#define SW_UART_IIR (0x08)
#define SW_UART_FCR (0x08)
#define SW_UART_LCR (0x0c)
#define SW_UART_MCR (0x10)
#define SW_UART_LSR (0x14)
#define SW_UART_MSR (0x18)
#define SW_UART_SCH (0x1c)
#define SW_UART_USR (0x7c)
#define SW_UART_TFL (0x80)
#define SW_UART_RFL (0x84)
#define SW_UART_HALT (0xa4)

/* register bit field define */
/* Interrupt Enable Register */
#define SW_UART_IER_PTIME (BIT(7))
#define SW_UART_IER_MSI   (BIT(3))
#define SW_UART_IER_RLSI  (BIT(2))
#define SW_UART_IER_THRI  (BIT(1))
#define SW_UART_IER_RDI   (BIT(0))
/* Interrupt ID Register */
#define SW_UART_IIR_FEFLAG_MASK (BIT(6)|BIT(7))
#define SW_UART_IIR_IID_MASK    (BIT(0)|BIT(1)|BIT(2)|BIT(3))
 #define SW_UART_IIR_IID_MSTA    (0)
 #define SW_UART_IIR_IID_NOIRQ   (1)
 #define SW_UART_IIR_IID_THREMP  (2)
 #define SW_UART_IIR_IID_RXDVAL  (4)
 #define SW_UART_IIR_IID_LINESTA (6)
 #define SW_UART_IIR_IID_BUSBSY  (7)
 #define SW_UART_IIR_IID_CHARTO  (12)
/* FIFO Control Register */
#define SW_UART_FCR_RXTRG_MASK  (BIT(6)|BIT(7))
 #define SW_UART_FCR_RXTRG_1CH   (0 << 6)
 #define SW_UART_FCR_RXTRG_1_4   (1 << 6)
 #define SW_UART_FCR_RXTRG_1_2   (2 << 6)
 #define SW_UART_FCR_RXTRG_FULL  (3 << 6)
#define SW_UART_FCR_TXTRG_MASK  (BIT(4)|BIT(5))
 #define SW_UART_FCR_TXTRG_EMP   (0 << 4)
 #define SW_UART_FCR_TXTRG_2CH   (1 << 4)
 #define SW_UART_FCR_TXTRG_1_4   (2 << 4)
 #define SW_UART_FCR_TXTRG_1_2   (3 << 4)
#define SW_UART_FCR_TXFIFO_RST  (BIT(2))
#define SW_UART_FCR_RXFIFO_RST  (BIT(1))
#define SW_UART_FCR_FIFO_EN     (BIT(0))
/* Line Control Register */
#define SW_UART_LCR_DLAB        (BIT(7))
#define SW_UART_LCR_SBC         (BIT(6))
#define SW_UART_LCR_PARITY_MASK (BIT(5)|BIT(4))
 #define SW_UART_LCR_EPAR        (1 << 4)
 #define SW_UART_LCR_OPAR        (0 << 4)
#define SW_UART_LCR_PARITY      (BIT(3))
#define SW_UART_LCR_STOP        (BIT(2))
#define SW_UART_LCR_DLEN_MASK   (BIT(1)|BIT(0))
 #define SW_UART_LCR_WLEN5       (0)
 #define SW_UART_LCR_WLEN6       (1)
 #define SW_UART_LCR_WLEN7       (2)
 #define SW_UART_LCR_WLEN8       (3)
/* Modem Control Register */
#define SW_UART_MCR_SIRE       (BIT(6))
#define SW_UART_MCR_AFE        (BIT(5))
#define SW_UART_MCR_LOOP       (BIT(4))
#define SW_UART_MCR_RTS        (BIT(1))
#define SW_UART_MCR_DTR        (BIT(0))
/* Line Status Rigster */
#define SW_UART_LSR_RXFIFOE    (BIT(7))
#define SW_UART_LSR_TEMT       (BIT(6))
#define SW_UART_LSR_THRE       (BIT(5))
#define SW_UART_LSR_BI         (BIT(4))
#define SW_UART_LSR_FE         (BIT(3))
#define SW_UART_LSR_PE         (BIT(2))
#define SW_UART_LSR_OE         (BIT(1))
#define SW_UART_LSR_DR         (BIT(0))
#define SW_UART_LSR_BRK_ERROR_BITS 0x1E /* BI, FE, PE, OE bits */
/* Modem Status Register */
#define SW_UART_MSR_DCD        (BIT(7))
#define SW_UART_MSR_RI         (BIT(6))
#define SW_UART_MSR_DSR        (BIT(5))
#define SW_UART_MSR_CTS        (BIT(4))
#define SW_UART_MSR_DDCD       (BIT(3))
#define SW_UART_MSR_TERI       (BIT(2))
#define SW_UART_MSR_DDSR       (BIT(1))
#define SW_UART_MSR_DCTS       (BIT(0))
#define SW_UART_MSR_ANY_DELTA  0x0F
#define MSR_SAVE_FLAGS SW_UART_MSR_ANY_DELTA
/* Status Register */
#define SW_UART_USR_RFF        (BIT(4))
#define SW_UART_USR_RFNE       (BIT(3))
#define SW_UART_USR_TFE        (BIT(2))
#define SW_UART_USR_TFNF       (BIT(1))
#define SW_UART_USR_BUSY       (BIT(0))
/* Halt Register */
#define SW_UART_HALT_LCRUP     (BIT(2))
#define SW_UART_HALT_FORCECFG  (BIT(1))
#define SW_UART_HALT_HTX       (BIT(0))

/* The global infor of UART channel. */

#ifdef CONFIG_ARCH_SUN8IW1P1
#define SUNXI_UART_NUM			6
#endif
#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9)
#define SUNXI_UART_NUM			5
#endif
#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) \
		|| defined(CONFIG_ARCH_SUN8IW7)
#define SUNXI_UART_NUM			4
#endif
#if defined(CONFIG_ARCH_SUN8IW8)
#define SUNXI_UART_NUM			3
#endif
#if defined(CONFIG_ARCH_SUN9IW1P1)
#define SUNXI_UART_NUM			6
#endif

#ifndef SUNXI_UART_NUM
#define SUNXI_UART_NUM			1
#endif

/* In 50/39 FPGA, two UART is available, but they share one IRQ.
   So we define the number of UART port as 1. */
#ifndef CONFIG_EVB_PLATFORM
#undef SUNXI_UART_NUM
#define SUNXI_UART_NUM			1
#endif

#define SUNXI_UART_DEV_NAME			"uart"

/* Memory mapping. */

#define SUNXI_UART_MEM_BASE			SUNXI_UART0_PBASE
#define SUNXI_UART_MEM_RANGE		0x400
#define SUNXI_UART_MEM_START(ch)	(SUNXI_UART_MEM_BASE + ch * SUNXI_UART_MEM_RANGE)

/* The IRQ number. */

#define SUNXI_UART_IRQ(ch)	    (SUNXI_IRQ_UART0 + ch)

/* Support to use r_uart on sun8iw5 */
#ifdef CONFIG_ARCH_SUN8IW5P1
#define SUNXI_S_UART
#undef	SUNXI_UART_NUM
#define SUNXI_UART_NUM			5
#endif

#ifdef SUNXI_S_UART
#define SUNXI_S_UART_DEV_NAME		"s_uart"
#define SUNXI_S_UART_MEM_BASE		SUNXI_R_UART_PBASE
#define SUNXI_S_UART_MEM_RANGE		0x400
#define SUNXI_S_UART_MEM_START		(SUNXI_S_UART_MEM_BASE)
#define SUNXI_S_UART_MEM_END		(SUNXI_S_UART_MEM_START + SUNXI_S_UART_MEM_RANGE - 1)
#define SUNXI_S_UART_IRQ	    	SUNXI_IRQ_RUART
#endif

/* About the number of IO */

#ifdef CONFIG_EVB_PLATFORM

#ifdef CONFIG_ARCH_SUN8IW1P1
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 8, 4, 4, 2, 2};
#elif defined(CONFIG_ARCH_SUN8IW5P1)
#ifdef SUNXI_S_UART
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 4, 4, 4, 2};
#else
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 4, 4, 4};
#endif
#elif defined(CONFIG_ARCH_SUN8IW6P1)
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 4, 4, 4, 4};
#elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW7P1)
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 4, 4, 4};
#elif defined(CONFIG_ARCH_SUN8IW8P1)
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 4, 4};
#elif defined(CONFIG_ARCH_SUN8IW9P1)
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 4, 4, 4, 4};
#elif defined(CONFIG_ARCH_SUN9IW1P1)
static int gs_uart_io_num[SUNXI_UART_NUM] = {2, 8, 4, 4, 4, 4};
#else
static int gs_uart_io_num[SUNXI_UART_NUM] = {2};
#endif

#else /* #ifdef CONFIG_EVB_PLATFORM */

static int gs_uart_io_num[SUNXI_UART_NUM] = {2};

#endif

struct platform_device *sw_uart_get_pdev(int uart_id);

#endif /* end of _SUNXI_UART_H_ */

