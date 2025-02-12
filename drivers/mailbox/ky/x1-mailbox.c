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
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include "../mailbox.h"
#include "x1_mailbox.h"

#define mbox_dbg(mbox, ...)	dev_dbg((mbox)->controller.dev, __VA_ARGS__)

#define DEV_PM_QOS_CLK_GATE            1
#define DEV_PM_QOS_REGULATOR_GATE      2
#define DEV_PM_QOS_PM_DOMAIN_GATE      4
#define DEV_PM_QOS_DEFAULT             7

static struct dev_pm_qos_request greq;

static irqreturn_t ky_mbox_irq(int irq, void *dev_id)
{
	struct ky_mailbox *mbox = dev_id;
	struct mbox_chan *chan;
	unsigned int status, msg = 0;
	int i;

	writel(0, (void __iomem *)&mbox->regs->ipc_iir);

	status = readl((void __iomem *)&mbox->regs->ipc_iir);

	if (!(status & 0xff))
		return IRQ_NONE;

	for (i = KY_TX_ACK_OFFSET; i < KY_NUM_CHANNELS + KY_TX_ACK_OFFSET; ++i) {
		chan = &mbox->controller.chans[i - KY_TX_ACK_OFFSET];
		/* new msg irq */
		if (!(status & (1 << i)))
			continue;

		/* clear the irq pending */
		writel(1 << i, (void __iomem *)&mbox->regs->ipc_icr);

		if (chan->txdone_method & TXDONE_BY_IRQ)
			mbox_chan_txdone(chan, 0);
	}

	for (i = 0; i < KY_NUM_CHANNELS; ++i) {
		chan = &mbox->controller.chans[i];

		/* new msg irq */
		if (!(status & (1 << i)))
			continue;

		mbox_chan_received_data(chan, &msg);

		/* clear the irq pending */
		writel(1 << i, (void __iomem *)&mbox->regs->ipc_icr);
	}

	return IRQ_HANDLED;
}

static int ky_chan_send_data(struct mbox_chan *chan, void *data)
{
	unsigned long flag;
	struct ky_mailbox *mbox = ((struct ky_mb_con_priv *)chan->con_priv)->smb;
	unsigned int chan_num = chan - mbox->controller.chans;

	spin_lock_irqsave(&mbox->lock, flag);

	writel(1 << chan_num, (void __iomem *)&mbox->regs->ipc_isrw);

	spin_unlock_irqrestore(&mbox->lock, flag);

	mbox_dbg(mbox, "Channel %d sent 0x%08x\n", chan_num, *((unsigned int *)data));

	return 0;
}

static int ky_chan_startup(struct mbox_chan *chan)
{
	return 0;
}

static void ky_chan_shutdown(struct mbox_chan *chan)
{
	unsigned int j;
	unsigned long flag;
	struct ky_mailbox *mbox = ((struct ky_mb_con_priv *)chan->con_priv)->smb;
	unsigned int chan_num = chan - mbox->controller.chans;

	spin_lock_irqsave(&mbox->lock, flag);

	/* clear pending */
	j = 0;
	j |= (1 << chan_num);
	writel(j, (void __iomem *)&mbox->regs->ipc_icr);

	spin_unlock_irqrestore(&mbox->lock, flag);
}

static bool ky_chan_last_tx_done(struct mbox_chan *chan)
{
	return 0;
}

static bool ky_chan_peek_data(struct mbox_chan *chan)
{
	struct ky_mailbox *mbox = ((struct ky_mb_con_priv *)chan->con_priv)->smb;
	unsigned int chan_num = chan - mbox->controller.chans;

	return readl((void __iomem *)&mbox->regs->ipc_rdr) & (1 << chan_num);
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
	struct ky_mb_con_priv *con_priv;
	int i, ret;

	mbox = devm_kzalloc(dev, sizeof(*mbox), GFP_KERNEL);
	if (!mbox)
		return -ENOMEM;

	con_priv = devm_kzalloc(dev, sizeof(*con_priv) * KY_NUM_CHANNELS, GFP_KERNEL);
	if (!con_priv)
		return -ENOMEM;

	chans = devm_kcalloc(dev, KY_NUM_CHANNELS, sizeof(*chans), GFP_KERNEL);
	if (!chans)
		return -ENOMEM;

	for (i = 0; i < KY_NUM_CHANNELS; ++i) {
		con_priv[i].smb = mbox;
		chans[i].con_priv = con_priv + i;
	}

	mbox->dev = dev;

	mbox->regs = (mbox_reg_desc_t *)devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mbox->regs)) {
		ret = PTR_ERR(mbox->regs);
		dev_err(dev, "Failed to map MMIO resource: %d\n", ret);
		return -EINVAL;
	}

	mbox->reset = devm_reset_control_get_exclusive(dev, "core_reset");
	if (IS_ERR(mbox->reset)) {
		ret = PTR_ERR(mbox->reset);
		dev_err(dev, "Failed to get reset: %d\n", ret);
		return -EINVAL;
	}

	/* deasser clk  */
	ret = reset_control_deassert(mbox->reset);
	if (ret) {
		dev_err(dev, "Failed to deassert reset: %d\n", ret);
		return -EINVAL;
	}

	pm_runtime_enable(dev);
	dev_pm_qos_add_request(dev, &greq, DEV_PM_QOS_MAX_FREQUENCY,
			DEV_PM_QOS_CLK_GATE | DEV_PM_QOS_PM_DOMAIN_GATE);
	pm_runtime_get_sync(dev);
	/* do not disable the clk of mailbox when suspend */
	dev_pm_qos_update_request(&greq, DEV_PM_QOS_PM_DOMAIN_GATE);
	pm_runtime_get_noresume(dev);

	/* request irq */
	ret = devm_request_irq(dev, platform_get_irq(pdev, 0),
			       ky_mbox_irq, IRQF_NO_SUSPEND, dev_name(dev), mbox);
	if (ret) {
		dev_err(dev, "Failed to register IRQ handler: %d\n", ret);
		return -EINVAL;
	}

	/* register the mailbox controller */
	mbox->controller.dev           = dev;
	mbox->controller.ops           = &ky_chan_ops;
	mbox->controller.chans         = chans;
	mbox->controller.num_chans     = KY_NUM_CHANNELS;
	mbox->controller.txdone_irq    = true;
	mbox->controller.txdone_poll   = false;
	mbox->controller.txpoll_period = 5;

	spin_lock_init(&mbox->lock);
	platform_set_drvdata(pdev, mbox);

	ret = mbox_controller_register(&mbox->controller);
	if (ret) {
		dev_err(dev, "Failed to register controller: %d\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int ky_mailbox_remove(struct platform_device *pdev)
{
	struct ky_mailbox *mbox = platform_get_drvdata(pdev);

	mbox_controller_unregister(&mbox->controller);

	pm_runtime_put_noidle(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);

	return 0;
}

static const struct of_device_id ky_mailbox_of_match[] = {
	{ .compatible = "ky,x1-mailbox", },
	{},
};
MODULE_DEVICE_TABLE(of, ky_mailbox_of_match);

#ifdef CONFIG_PM_SLEEP
static int x1_mailbox_suspend_noirq(struct device *dev)
{
#if 0
	int ret;
	struct ky_mailbox *mbox = dev_get_drvdata(dev);

	ret = reset_control_assert(mbox->reset);
#endif

	return 0;
}

static int x1_mailbox_resume_noirq(struct device *dev)
{
#if 0
	int ret;
	struct ky_mailbox *mbox = dev_get_drvdata(dev);

	ret = reset_control_deassert(mbox->reset);
#endif

	return 0;
}

static const struct dev_pm_ops x1_mailbox_pm_qos = {
	.suspend_noirq = x1_mailbox_suspend_noirq,
	.resume_noirq = x1_mailbox_resume_noirq,
};
#endif

static struct platform_driver ky_mailbox_driver = {
	.driver = {
		.name = "ky-mailbox",
#ifdef CONFIG_PM_SLEEP
		.pm	= &x1_mailbox_pm_qos,
#endif
		.of_match_table = ky_mailbox_of_match,
	},
	.probe  = ky_mailbox_probe,
	.remove = ky_mailbox_remove,
};
module_platform_driver(ky_mailbox_driver);

MODULE_DESCRIPTION("ky Message Box driver");
MODULE_LICENSE("GPL v2");
