// SPDX-License-Identifier: GPL-2.0+
/*
 * Implementation of the DSP IPC interface (host side)
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/firmware/cix/dsp.h>
#include <linux/kernel.h>
#include <linux/mailbox_client.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/platform_device.h>

static const char * const dsp_mbox_ch_names[CIX_DSP_MBOX_NUM] = { "txdb", "rxdb" };

static void __maybe_unused cix_dsp_mbox_dump_regs(struct cix_dsp_ipc *dsp_ipc)
{
	void __iomem *base;
	uint32_t val;
	int i;
#define MBOX_BASE_AP2DSP	(0x070f0000)
#define MBOX_BASE_DSP2AP	(0x07100000)
#define MBOX_REG_SIZE		(0x10000)
#define MBOX_RGE_DUMP_NUM	(20)
#define MBOX_REG_OFFSET		(0x80)

	base = ioremap(MBOX_BASE_AP2DSP, MBOX_REG_SIZE);
	for (i = 0; i < MBOX_RGE_DUMP_NUM; i++) {
		val = readl(base + MBOX_REG_OFFSET + 4*i);
		dev_info(dsp_ipc->dev, "[0x%x]: 0x%x\n",
			 MBOX_REG_OFFSET + 4*i, val);
	}
	iounmap(base);

	base = ioremap(MBOX_BASE_DSP2AP, MBOX_REG_SIZE);
	for (i = 0; i < MBOX_RGE_DUMP_NUM; i++) {
		val = readl(base + MBOX_REG_OFFSET + 4*i);
		dev_info(dsp_ipc->dev, "[0x%x]: 0x%x\n",
			 MBOX_REG_OFFSET + 4*i, val);
	}
	iounmap(base);
}

int cix_dsp_ipc_send(struct cix_dsp_ipc *ipc, unsigned int idx, uint32_t msg)
{
	struct cix_dsp_chan *dsp_chan;
	int ret;

	if (idx >= CIX_DSP_MBOX_NUM)
		return -EINVAL;

	dsp_chan = &ipc->chans[idx];
	ret = mbox_send_message(dsp_chan->ch, &msg);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(cix_dsp_ipc_send);

static void cix_dsp_rx_callback(struct mbox_client *cl, void *msg)
{
	struct cix_dsp_chan *chan = container_of(cl, struct cix_dsp_chan, cl);
	struct device *dev = cl->dev;

	switch (chan->idx) {
	case CIX_DSP_MBOX_REPLY:
		chan->ipc->ops->handle_reply(chan->ipc);
		mbox_client_txdone(chan->ch, 0);
		break;
	case CIX_DSP_MBOX_REQUEST:
		chan->ipc->ops->handle_request(chan->ipc);
		break;
	default:
		dev_err(dev, "wrong mbox chan %d\n", chan->idx);
		break;
	}
}

int cix_dsp_request_mbox(struct cix_dsp_ipc *dsp_ipc)
{
	struct device *dev = dsp_ipc->dev;
	struct cix_dsp_chan *dsp_chan;
	struct mbox_client *cl;
	int i, j;
	int ret;

	/*
	 * AP req -- txdb --> DSP
	 *    AP <-- txdb --  DSP rsp
	 *    AP <-- rxdb --  DSP req
	 * AP rsp -- rxdb --> DSP
	 */
	for (i = 0; i < CIX_DSP_MBOX_NUM; i++) {
		dsp_chan = &dsp_ipc->chans[i];
		cl = &dsp_chan->cl;
		cl->dev = dev;
		cl->tx_block = false;
		cl->knows_txdone = false;
		cl->tx_prepare = NULL;
		cl->rx_callback = cix_dsp_rx_callback;

		dsp_chan->ipc = dsp_ipc;
		dsp_chan->idx = i;
		dsp_chan->ch = mbox_request_channel_byname(cl, dsp_mbox_ch_names[i]);
		if (IS_ERR(dsp_chan->ch)) {
			ret = PTR_ERR(dsp_chan->ch);
			if (ret != -EPROBE_DEFER)
				dev_err(dev, "Failed to request mbox chan %s ret %d\n",
					dsp_mbox_ch_names[i], ret);

			for (j = 0; j < i; j++) {
				dsp_chan = &dsp_ipc->chans[j];
				mbox_free_channel(dsp_chan->ch);
			}

			return ret;
		}
	}

	dsp_ipc->dev = dev;

	dev_info(dev, "CIX DSP IPC Mbox request\n");

	return 0;
}
EXPORT_SYMBOL(cix_dsp_request_mbox);

void cix_dsp_free_mbox(struct cix_dsp_ipc *dsp_ipc)
{
	struct device *dev = dsp_ipc->dev;
	struct cix_dsp_chan *dsp_chan;
	int i;

	for (i = 0; i < CIX_DSP_MBOX_NUM; i++) {
		dsp_chan = &dsp_ipc->chans[i];
		mbox_free_channel(dsp_chan->ch);
	}

	dev_info(dev, "CIX DSP IPC Mbox free\n");
}
EXPORT_SYMBOL(cix_dsp_free_mbox);

static int cix_dsp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cix_dsp_ipc *dsp_ipc;

	device_set_node(&pdev->dev, pdev->dev.parent->fwnode);

	dsp_ipc = devm_kzalloc(dev, sizeof(*dsp_ipc), GFP_KERNEL);
	if (!dsp_ipc)
		return -ENOMEM;

	dsp_ipc->dev = dev;
	dev_set_drvdata(dev, dsp_ipc);

	dev_info(dev, "CIX DSP IPC initialized\n");

	return 0;
}

static struct platform_driver cix_dsp_driver = {
	.driver = {
		.name = "cix-dsp",
	},
	.probe = cix_dsp_probe,
};
builtin_platform_driver(cix_dsp_driver);

MODULE_AUTHOR("Joakim Zhang <joakim.zhang@cixtech.com>");
MODULE_DESCRIPTION("CIX DSP IPC Driver");
MODULE_LICENSE("GPL v2");
