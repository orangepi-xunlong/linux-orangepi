// SPDX-License-Identifier: GPL-2.0

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include "x1pro_mailbox.h"

#define mbox_dbg(mbox, ...)	dev_dbg((mbox)->controller.dev, __VA_ARGS__)

static irqreturn_t ky_mbox_irq(int irq, void *dev_id)
{
	struct ky_mailbox *mbox = dev_id;
	struct mbox_chan *chan;
	unsigned int status, msg;
	int i, j;

	/* we fixed user0 as the richos */
	/* and we are fixed using new_msg irq */
	status = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_status)
		& readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_en_set);

	if (!(status & 0xff))
		return IRQ_NONE;

	for (i = 0; i < KY_NUM_CHANNELS; ++i) {
		chan = &mbox->controller.chans[i];

		/* new msg irq */
		if (!(status & (1 << (i * 2))))
			continue;

		/* clear the fifo */
		while (readl((void __iomem *)&mbox->regs->msg_status[i])) {
			msg = readl((void __iomem *)&mbox->regs->mbox_msg[i]);
			mbox_dbg(mbox, "Channel %d received 0x%08x\n", i, msg);
			mbox_chan_received_data(chan, &msg);
		}

		/* clear the irq pending */
		j = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_status_clr);
		j |= (1 << (i * 2));
		writel(j, (void __iomem *)&mbox->regs->mbox_irq[0].irq_status_clr);
	}

	return IRQ_HANDLED;
}

static int ky_chan_send_data(struct mbox_chan *chan, void *data)
{
	struct ky_mailbox *mbox = chan->con_priv;
	unsigned int chan_num = chan - mbox->controller.chans;

	/* send data */
	writel(*((unsigned int *)data), (void __iomem *)&mbox->regs->mbox_msg[chan_num]);

	mbox_dbg(mbox, "Channel %d sent 0x%08x\n", chan_num, *((unsigned int *)data));

	return 0;
}

static int ky_chan_startup(struct mbox_chan *chan)
{

	struct ky_mailbox *mbox = chan->con_priv;
	unsigned int chan_num = chan - mbox->controller.chans;
	unsigned int msg, j;

	/* if this channel is tx, we should not enable the interrupt */
	if (chan->cl->tx_prepare != NULL)
		return 0;

	/* clear the fifo */
	while (readl((void __iomem *)&mbox->regs->msg_status[chan_num])) {
		msg = readl((void __iomem *)&mbox->regs->mbox_msg[chan_num]);
	}

	spin_lock(&mbox->lock);

	/* clear pending */
	j = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_status_clr);
	j |= (1 << (chan_num * 2));
	writel(j, (void __iomem *)&mbox->regs->mbox_irq[0].irq_status_clr);

	/* enable new msg irq */
	j = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_en_set);
	j |= (1 << (chan_num * 2));
	writel(j, (void __iomem *)&mbox->regs->mbox_irq[0].irq_en_set);

	spin_unlock(&mbox->lock);

	return 0;
}

static void ky_chan_shutdown(struct mbox_chan *chan)
{
	struct ky_mailbox *mbox = chan->con_priv;
	unsigned int chan_num = chan - mbox->controller.chans;
	unsigned int msg, j;

	if (chan->cl->tx_prepare != NULL)
		return ;

	spin_lock(&mbox->lock);

	/* disable new msg irq */
	j = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_en_clr);
	j |= (1 << (chan_num * 2));
	writel(j, (void __iomem *)&mbox->regs->mbox_irq[0].irq_en_clr);

	/* flush the fifo */
	while (readl((void __iomem *)&mbox->regs->msg_status[chan_num])) {
		msg = readl((void __iomem *)&mbox->regs->mbox_msg[chan_num]);
	}

	/* clear pending */
	j = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_status_clr);
	j |= (1 << (chan_num * 2));
	writel(j, (void __iomem *)&mbox->regs->mbox_irq[0].irq_status_clr);

	spin_unlock(&mbox->lock);
}

static bool ky_chan_last_tx_done(struct mbox_chan *chan)
{
	unsigned int j;
	struct ky_mailbox *mbox = chan->con_priv;
	unsigned int chan_num = chan - mbox->controller.chans;

	spin_lock(&mbox->lock);

	j = readl((void __iomem *)&mbox->regs->mbox_irq[0].irq_status);

	spin_unlock(&mbox->lock);

	return !((j >> (2 * chan_num)) & 0x1);
}

static bool ky_chan_peek_data(struct mbox_chan *chan)
{
	struct ky_mailbox *mbox = chan->con_priv;
	unsigned int chan_num = chan - mbox->controller.chans;

	return readl((void __iomem *)&mbox->regs->msg_status[chan_num]);
}

static const struct mbox_chan_ops ky_chan_ops = {
	.send_data    = ky_chan_send_data,
	.startup      = ky_chan_startup,
	.shutdown     = ky_chan_shutdown,
	.last_tx_done = ky_chan_last_tx_done,
	.peek_data    = ky_chan_peek_data,
};

static int ky_mailbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mbox_chan *chans;
	struct ky_mailbox *mbox;
	int i, ret;

	mbox = devm_kzalloc(dev, sizeof(*mbox), GFP_KERNEL);
	if (!mbox)
		return -ENOMEM;

	chans = devm_kcalloc(dev, KY_NUM_CHANNELS, sizeof(*chans), GFP_KERNEL);
	if (!chans)
		return -ENOMEM;

	for (i = 0; i < KY_NUM_CHANNELS; ++i)
		chans[i].con_priv = mbox;

	mbox->regs = (mbox_reg_desc_t *)devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mbox->regs)) {
		ret = PTR_ERR(mbox->regs);
		dev_err(dev, "Failed to map MMIO resource: %d\n", ret);
		return -EINVAL;
	}

	mbox->clk = devm_clk_get(dev, "core");
	if (IS_ERR(mbox->clk)) {
		ret = PTR_ERR(mbox->clk);
		dev_err(dev, "Failed to get clock: %d\n", ret);
		return -EINVAL;
	}

	mbox->reset = devm_reset_control_get_exclusive(dev, "core_reset");
	if (IS_ERR(mbox->reset)) {
		ret = PTR_ERR(mbox->reset);
		dev_err(dev, "Failed to get reset: %d\n", ret);
		return -EINVAL;
	}

	/* enable clk */
	ret = clk_prepare_enable(mbox->clk);
	if (ret) {
		dev_err(dev, "Failed to enable clock: %d\n", ret);
		return ret;
	}

	/* deasser clk  */
	ret = reset_control_deassert(mbox->reset);
	if (ret) {
		dev_err(dev, "Failed to deassert reset: %d\n", ret);
		clk_disable_unprepare(mbox->clk);
		return -EINVAL;
	}

	/* deassert module */
	ret = readl((void __iomem *)&mbox->regs->mbox_sysconfig);
	ret |= 0x1;
	writel(ret, (void __iomem *)&mbox->regs->mbox_sysconfig);

	/* request irq */
	ret = devm_request_irq(dev, platform_get_irq(pdev, 0),
			       ky_mbox_irq, 0, dev_name(dev), mbox);
	if (ret) {
		dev_err(dev, "Failed to register IRQ handler: %d\n", ret);
		reset_control_assert(mbox->reset);
		clk_disable_unprepare(mbox->clk);
	}

	/* register the mailbox controller */
	mbox->controller.dev           = dev;
	mbox->controller.ops           = &ky_chan_ops;
	mbox->controller.chans         = chans;
	mbox->controller.num_chans     = KY_NUM_CHANNELS;
	mbox->controller.txdone_irq    = false;
	mbox->controller.txdone_poll   = true;
	mbox->controller.txpoll_period = 5;

	spin_lock_init(&mbox->lock);
	platform_set_drvdata(pdev, mbox);

	ret = mbox_controller_register(&mbox->controller);
	if (ret) {
		dev_err(dev, "Failed to register controller: %d\n", ret);
		reset_control_assert(mbox->reset);
		clk_disable_unprepare(mbox->clk);
		return -EINVAL;
	}

	return 0;
}

static int ky_mailbox_remove(struct platform_device *pdev)
{
	struct ky_mailbox *mbox = platform_get_drvdata(pdev);

	mbox_controller_unregister(&mbox->controller);
	clk_disable_unprepare(mbox->clk);

	return 0;
}

static const struct of_device_id ky_mailbox_of_match[] = {
	{ .compatible = "ky,x1-pro-mailbox", },
	{},
};
MODULE_DEVICE_TABLE(of, ky_mailbox_of_match);

static struct platform_driver ky_mailbox_driver = {
	.driver = {
		.name = "ky-mailbox",
		.of_match_table = ky_mailbox_of_match,
	},
	.probe  = ky_mailbox_probe,
	.remove = ky_mailbox_remove,
};
module_platform_driver(ky_mailbox_driver);

MODULE_DESCRIPTION("ky Message Box driver");
MODULE_LICENSE("GPL v2");
