/*
 * drivers/media/video/tsc/tscdrv.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * csjamesdeng <csjamesdeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __TSC_DRV_H__
#define __TSC_DRV_H__

#define DRV_VERSION                 "0.01alpha"
#define SUNXI_IRQ_TS                 (32 + 81)
#define TS_IRQ_NO                   (SUNXI_IRQ_TS)
#define NEW

#ifndef TSCDEV_MAJOR
#define TSCDEV_MAJOR                (225)
#endif

#ifndef TSCDEV_MINOR
#define TSCDEV_MINOR                (0)
#endif

#ifdef NEW
struct tsc_port_config {
	unsigned int port0config:1;
	unsigned int port1config:1;
	unsigned int port2config:1;
	unsigned int port3config:1;
};
#endif

struct iomap_para {
	volatile char *regs_macc;
	volatile char *regs_ccmu;
};

struct tsc_dev {
	struct cdev cdev;                 /* char device struct                 */
	struct device *dev;              /* ptr to class device struct         */
	struct class  *tsc_class;            /* class for auto create device node  */
	struct semaphore sem;            /* mutual exclusion semaphore         */
	spinlock_t lock;                /* spinlock to protect ioclt access */
	wait_queue_head_t wq;            /* wait queue for poll ops            */
	char   name[16];

	struct iomap_para iomap_addrs;   /* io remap addrs                     */
	volatile unsigned int *clk_base_vir ;

	unsigned int irq;               /* tsc driver irq number */
	unsigned int irq_flag;          /* flag of tsc driver irq generated */
	unsigned int ref_count;
	struct clk *tsc_parent_pll_clk;
	struct clk *tsc_clk;            /* ts  clock */
	struct intrstatus intstatus;    /* save interrupt status */
	struct pinctrl *pinctrl;
#ifdef NEW
	struct pinctrl_state  *ts0_pinstate;
	struct pinctrl_state  *ts1_pinstate;
	struct pinctrl_state  *ts2_pinstate;
	struct pinctrl_state  *ts3_pinstate;
	/* for sleep */
	struct pinctrl_state  *ts0sleep_pinstate;
	struct pinctrl_state  *ts1sleep_pinstate;
	struct pinctrl_state  *ts2sleep_pinstate;
	struct pinctrl_state  *ts3sleep_pinstate;
	struct tsc_port_config port_config;
#endif
};

#endif

