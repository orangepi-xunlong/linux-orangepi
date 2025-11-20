/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 - 2028 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2024-2028 allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/spinlock.h>

#define BOOT_MAGIC		(0x91287346)

#define NUM_OS			(4)
#define NUM_CHAN		(4)
#define NUM_FIFO		(8)
#define NUM_SLOT		(NUM_OS * NUM_OS * NUM_CHAN)

/* gic macro */
#define IPC_IRQ_NONE       (0)
#define GICD_ISPENDR_BASE  (0x200)
#define GICD_ICPENDR_BASE  (0X280)

#define OFFSET(base, irq)  (base + (irq >> 5) * 4)
#define BIT_VAL(irq)       (1 << (irq % 32))

#define mbox_dbg(mbox, ...)	dev_dbg((mbox)->controller.dev, __VA_ARGS__)

enum direction {
	SEND = 0,
	RECV
};

/*
 * os_state: local os if data need recv, local os read, remote os writel;
 * chan_state: tx rx is chan fifo ptr, tx update by remote os, rx update by local os;
 * chan_fifo[slot_id][fifo_id]: use for data buffer
 */

typedef struct {
	u32 state[NUM_OS];

	u32 boot[NUM_OS];

	struct {
		u32 tx;
		u32 rx;
	} slot_state[NUM_SLOT];

	u32 fifo[NUM_SLOT][NUM_FIFO];
} chan_buffer_t;


struct sunxi_mbox_chan {
	struct sunxi_irq_mbox *parent;
	int chan_id;
	int remote_id;
	int local_id;
	int tx_irq;
	int rx_irq;
	int slot_rx;
	int slot_tx;
	bool used;
};

struct sunxi_irq_mbox {
	struct mbox_controller controller;
	struct mbox_chan *chan;
	struct sunxi_mbox_chan *mchan;
	int num_chans;
	spinlock_t lock;
	void __iomem *mem_virt;
	unsigned long mem_phys;
	unsigned long mem_size;

	void __iomem *gic_dbase;
	int recv_irqs[NUM_OS];
	int irq_num;
	int rx_irq;
	int local_id;
};

static bool sunxi_irq_mbox_last_tx_done(struct mbox_chan *mchan);
static bool sunxi_irq_mbox_peek_data(struct mbox_chan *chan);

/* I/O memory map interrupt controller register space */
static int sunxi_intc_map(struct sunxi_irq_mbox *mbox)
{
	struct device_node *gic_node = NULL;
	int err = 0;
	struct resource res;

	/* get gic node from DT */
	gic_node = of_parse_phandle(mbox->controller.dev->of_node, "gic-node", 0);
	if (!gic_node) {
		pr_err("Unable to find gic-node in device tree\n");
		err = -ENXIO;
		goto ret_err;
	}

	/* get base address from DT node*/
	err = of_address_to_resource(gic_node, 0, &res);
	if (err) {
		pr_err("Unable to get resource from gic node \n");
		err = -ENOMEM;
		goto ret_err;
	}

	/* map gic distributer register space */
	mbox->gic_dbase = (void __iomem *)ioremap(res.start, resource_size(&res));
	if (!mbox->gic_dbase) {
		pr_err("Unable to ioremap gic distributer register space \n");
		err = -ENOMEM;
		goto ret_err;
	}

ret_err:
	/* release refcount to gic DT node */
	of_node_put(gic_node);

	return err;
}

/* I/O memory unmap interrupt controller register space */
static inline void sunxi_intc_unmap(struct sunxi_irq_mbox *mbox)
{
	iounmap((void *)mbox->gic_dbase);
}

static inline void sunxi_mbox_irq_notify(struct sunxi_mbox_chan *mchan)
{
	int irq = mchan->tx_irq;
	struct sunxi_irq_mbox *mbox = mchan->parent;

	chan_buffer_t *chan_buf = (chan_buffer_t *)mbox->mem_virt;

	/* TODO */
	while (BOOT_MAGIC != readl((void *)&chan_buf->boot[mchan->remote_id]))
		;;

	writel(BIT_VAL(irq), mbox->gic_dbase + OFFSET(GICD_ISPENDR_BASE, irq));
}

static inline void sunxi_mbox_irq_clear(struct sunxi_mbox_chan *mchan)
{
	struct sunxi_irq_mbox *mbox = mchan->parent;
	int irq = mbox->rx_irq;

	writel(BIT_VAL(irq), mbox->gic_dbase + OFFSET(GICD_ICPENDR_BASE, irq));
}

static inline void *os_state_addr(struct sunxi_mbox_chan *mchan, enum direction d)
{
	struct sunxi_irq_mbox *mbox = mchan->parent;

	chan_buffer_t *buffer = (chan_buffer_t *)mbox->mem_virt;

	if (SEND == d)
		return (void *)&buffer->state[mchan->remote_id];
	else
		return (void *)&buffer->state[mchan->local_id];
}

static inline void *chan_state_tx_addr(struct sunxi_mbox_chan *mchan, enum direction d)
{
	struct sunxi_irq_mbox *mbox = mchan->parent;

	chan_buffer_t *buffer = (chan_buffer_t *)mbox->mem_virt;

	if (SEND == d)
		return (void *)&buffer->slot_state[mchan->slot_tx];
	else
		return (void *)&buffer->slot_state[mchan->slot_rx];
}

static inline void *chan_state_rx_addr(struct sunxi_mbox_chan *mchan, enum direction d)
{
	return (void *)((int *)chan_state_tx_addr(mchan, d) + 1);
}

static unsigned long chan_fifo_addr(struct sunxi_mbox_chan *mchan, enum direction d)
{
	unsigned long base;
	unsigned long offset;
	struct sunxi_irq_mbox *mbox = mchan->parent;

	chan_buffer_t *buffer = (chan_buffer_t *)mbox->mem_virt;
	int tx_idx = readl(chan_state_tx_addr(mchan, d));
	int rx_idx = readl(chan_state_rx_addr(mchan, d));

	pr_debug("fifo tx index = %d, rx index = %d \n", tx_idx, rx_idx);

	if (SEND == d) {
		/* addr = (unsigned long)&buffer->fifo[mchan->slot_tx][tx_idx]; */
		offset = (mchan->slot_tx * NUM_FIFO + tx_idx) * sizeof(u32);
		tx_idx = (tx_idx + 1) % NUM_FIFO;
		writel(tx_idx, chan_state_tx_addr(mchan, d));
	} else {
		/* addr = (unsigned long)&buffer->fifo[mchan->slot_rx][rx_idx]; */
		offset = (mchan->slot_rx * NUM_FIFO + rx_idx) * sizeof(u32);
		rx_idx = (rx_idx + 1) % NUM_FIFO;
		writel(rx_idx, chan_state_rx_addr(mchan, d));
	}

	base = (unsigned long)buffer->fifo;

	return (base + offset);
}

static struct mbox_chan *sunxi_mbox_xlate(struct mbox_controller *controller,
					   const struct of_phandle_args *spec)
{
	struct sunxi_irq_mbox *mbox = dev_get_drvdata(controller->dev);
	struct sunxi_mbox_chan *mchan;
	struct mbox_chan *chan;
	int idx = 0;
	unsigned int os_id = spec->args[0];
	unsigned int chan_id = spec->args[1];

	/* Bounds checking */
	if (os_id >= NUM_OS || chan_id >= NUM_CHAN) {
		pr_err("Invalid os idx %d channel %d \n", os_id, chan_id);
		return ERR_PTR(-EINVAL);
	}

	/* Is requested channel free? */
	idx = (mbox->local_id * NUM_OS + os_id) * NUM_CHAN + chan_id;
	chan = &mbox->chan[idx];
	mchan = chan->con_priv;

	if (mchan->used) {
		pr_err("os(%d) Channel(%d) in use\n", os_id,  chan_id);
		return ERR_PTR(-EBUSY);
	}

	mchan->remote_id	= os_id;
	mchan->local_id		= mbox->local_id;
	mchan->chan_id		= chan_id;
	mchan->tx_irq		= mbox->recv_irqs[os_id];
	mchan->rx_irq		= mbox->recv_irqs[mbox->local_id];
	mchan->slot_tx		= (os_id * NUM_OS + mbox->local_id) * NUM_CHAN + chan_id;

	mchan->used		= 1;

	pr_info("request local_os(%d) <==> remote_os(%d), chan_id(%d) \n",
			mchan->local_id, mchan->remote_id, mchan->chan_id);

	pr_debug("rx slot(%d), tx slot(%d) \n", mchan->slot_rx, mchan->slot_tx);

	return chan;
}

static irqreturn_t sunxi_irq_mbox_handler(int irq, void *dev_id)
{

	struct sunxi_irq_mbox *mbox = dev_id;
	chan_buffer_t *buffer = NULL;
	struct mbox_chan *chan = NULL;
	struct sunxi_mbox_chan *mchan = NULL;
	uint32_t status;
	int n, s, e;
	uint32_t msg;
	void *addr = NULL;

	s = mbox->local_id * NUM_OS * NUM_CHAN;
	e = s + (NUM_OS * NUM_CHAN);

	buffer = (chan_buffer_t *)mbox->mem_virt;

	/* Only examine channels that are currently enabled. */
	status = readl((void *)&buffer->state[mbox->local_id]);
	if (!(status))
		goto ret;

	for (n = s; n < e; ++n) {
		chan = &mbox->chan[n];
		mchan = (struct sunxi_mbox_chan *)chan->con_priv;
		if (!mchan->used) /* not register, will continue */
			continue;

		while (sunxi_irq_mbox_peek_data(chan)) { /* Todo: if always data avaliable, how to do */
			addr = (void *)chan_fifo_addr(mchan, RECV);
			msg = readl(addr);

			pr_debug("--- [recv] os(%d) ==> 0s(%d) chan_id %d data 0x%x\n",
					mchan->remote_id, mchan->local_id,
					mchan->chan_id, msg);

			mbox_chan_received_data(chan, &msg);
		}
		writel(0, os_state_addr(mchan, RECV));
	}

ret:
	/* clear irq pending. */
	sunxi_mbox_irq_clear(chan->con_priv);

	return IRQ_HANDLED;
}

static int sunxi_irq_mbox_send_data(struct mbox_chan *chan, void *data)
{

	struct sunxi_mbox_chan *mchan = chan->con_priv;

	uint32_t msg = *(uint32_t *)data;
	void *addr = NULL;

	/* send data to tx chan fifo */
	pr_debug("--- [send] os(%d) os(%d) chan_id(%d) msg = %d\n",
			mchan->local_id, mchan->remote_id,
			mchan->chan_id, msg);

	addr = (void *)chan_fifo_addr(mchan, SEND);
	writel(msg, addr);

	writel(1, os_state_addr(mchan, SEND));

	/* raise irq to remote */
	sunxi_mbox_irq_notify(mchan); /* TODO */

	return 0;
}

static int sunxi_irq_mbox_startup(struct mbox_chan *chan)
{
	/* Todo */
	return 0;
}

static void sunxi_irq_mbox_shutdown(struct mbox_chan *chan)
{
	/* Todo */
}

static bool sunxi_irq_mbox_last_tx_done(struct mbox_chan *chan)
{
	struct sunxi_mbox_chan *mchan = chan->con_priv;

	int tx_idx = readl(chan_state_tx_addr(mchan, SEND));
	int rx_idx = readl(chan_state_rx_addr(mchan, SEND));

	return ((tx_idx + 1) % NUM_FIFO != rx_idx);
}

static bool sunxi_irq_mbox_peek_data(struct mbox_chan *chan)
{
	struct sunxi_mbox_chan *mchan = chan->con_priv;

	int tx_idx = readl(chan_state_tx_addr(mchan, RECV));
	int rx_idx = readl(chan_state_rx_addr(mchan, RECV));

	return (tx_idx != rx_idx);
}

static const struct mbox_chan_ops sunxi_irq_mbox_chan_ops = {
	.send_data    = sunxi_irq_mbox_send_data,
	.startup      = sunxi_irq_mbox_startup,
	.shutdown     = sunxi_irq_mbox_shutdown,
	.last_tx_done = sunxi_irq_mbox_last_tx_done,
	.peek_data    = sunxi_irq_mbox_peek_data,
};

static int sunxi_irq_mbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource res;
	struct sunxi_irq_mbox *mbox;
	struct device_node *node;
	void *zero_buffer = NULL;
	unsigned int zero_size = 0;
	int i;
	int ret = 0;

	mbox = devm_kzalloc(dev, sizeof(*mbox), GFP_KERNEL);
	if (!mbox)
		return -ENOMEM;

	mbox->num_chans = NUM_SLOT;

	mbox->chan = devm_kcalloc(dev, mbox->num_chans, sizeof(*mbox->chan), GFP_KERNEL);
	if (!mbox->chan)
		return -ENOMEM;

	mbox->mchan = devm_kcalloc(dev,
		mbox->num_chans, sizeof(*mbox->mchan), GFP_KERNEL);
	if (!mbox->mchan)
		return -ENOMEM;

	for (i = 0; i < mbox->num_chans; i++) {
		mbox->chan[i].con_priv	= &mbox->mchan[i];
		mbox->mchan[i].parent	= mbox;
		mbox->mchan[i].slot_rx	= i;
		mbox->mchan[i].used	= 0;
	}

	ret = of_property_read_variable_u32_array(dev->of_node, "recv-irqs", mbox->recv_irqs, 2, NUM_OS);
	if (ret < 0) {
		pr_err("Unable to find recv-irqs in node \n");
		return ret;
	}

	for (i = 0; i < NUM_OS; i++) {
		pr_debug("OS(%d) recv_irq is %d \n", i, mbox->recv_irqs[i]);
	}

	ret = of_property_read_u32_index(dev->of_node, "local-id", 0, &mbox->local_id);
	if (ret) {
		pr_err("Unable to find local_id in node \n");
		return -EINVAL;
	}

	mbox->rx_irq = mbox->recv_irqs[mbox->local_id];

	node = of_parse_phandle(dev->of_node, "memory-mbox", 0);
	if (!node) {
		dev_err(dev, "no memory-mbox specified \n");
		return -EINVAL;
	}

	ret = of_address_to_resource(node, 0, &res);
	of_node_put(node);
	if (ret)
		return ret;

	mbox->mem_phys = res.start;
	mbox->mem_size = resource_size(&res);
	mbox->mem_virt = devm_ioremap_wc(dev, mbox->mem_phys, mbox->mem_size);
	if (IS_ERR(mbox->mem_virt)) {
		dev_err(dev, "Failed to map memory fifo: %pa+%zx\n",
				&mbox->mem_phys, mbox->mem_size);
		return -EBUSY;
	}

	zero_size = sizeof(u32) * (NUM_OS + NUM_SLOT) * 2;
	zero_buffer = kzalloc(zero_size, GFP_KERNEL);
	if (!zero_buffer) {
		dev_err(dev, "Failed to get memory \n");
		return -ENOMEM;
	}

	memcpy_toio(mbox->mem_virt, zero_buffer, zero_size);

	kfree(zero_buffer);

	pr_debug("---- buffer phys addr : %lx ----\n", mbox->mem_phys);
	pr_debug("---- buffer virt addr : %lx ----\n", (long)mbox->mem_virt);

	mbox->controller.dev           = dev;
	mbox->controller.ops           = &sunxi_irq_mbox_chan_ops;
	mbox->controller.chans         = mbox->chan;
	mbox->controller.num_chans     = mbox->num_chans;
	mbox->controller.txdone_irq    = false;
	mbox->controller.txdone_poll   = true;
	mbox->controller.txpoll_period = 5;
	mbox->controller.of_xlate = sunxi_mbox_xlate;

	if (sunxi_intc_map(mbox)) {
		pr_err("Failed to map intc MMIO resource: %d\n", ret);
		goto ioremap_err;
	}

	ret = devm_request_irq(dev, irq_of_parse_and_map(dev->of_node, 0),
			       sunxi_irq_mbox_handler, 0, dev_name(dev), mbox);
	if (ret) {
		dev_err(dev, "Failed to register IRQ handler: %d\n", ret);
		return ret;
	}

	spin_lock_init(&mbox->lock);

	platform_set_drvdata(pdev, mbox);

	ret = mbox_controller_register(&mbox->controller);
	if (ret) {
		dev_err(dev, "Failed to register controller: %d\n", ret);
		return ret;
	}

	pr_info("----- irq msgbox probe success -----\n");

ioremap_err:
	return ret;
}

static int sunxi_irq_mbox_remove(struct platform_device *pdev)
{
	struct sunxi_irq_mbox *mbox = platform_get_drvdata(pdev);

	mbox_controller_unregister(&mbox->controller);

	return 0;
}

static const struct of_device_id sunxi_irq_mbox_of_match[] = {
	{ .compatible = "allwinner,irq-msgbox", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_irq_mbox_of_match);

static struct platform_driver sunxi_irq_mbox_driver = {
	.driver = {
		.name = "sunxi-soft-msgbox",
		.of_match_table = sunxi_irq_mbox_of_match,
	},
	.probe  = sunxi_irq_mbox_probe,
	.remove = sunxi_irq_mbox_remove,
};

module_platform_driver(sunxi_irq_mbox_driver);

MODULE_AUTHOR("sw1@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi irq Message Box driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
