// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2024 Ky X1x Adma Driver
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/genalloc.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/of.h>
#include <linux/delay.h>
#include "dmaengine.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/of_device.h>

#define BCR	0x0
#define SAR	0x10
#define DAR	0x20
#define NDR	0x30
#define DCR	0x40
#define IER	0x80
#define ADMA_SAMPLE_BITS_MASK	(0x7 << 22)
#define ADMA_SAMPLE_BITS(x)	(((x) << 22) & ADMA_SAMPLE_BITS_MASK)
#define ADMA_CH_ABORT	(1 << 20)
#define ADMA_CLOSE_DESC_EN	(1 << 17)
#define ADMA_UNPACK_SAMPLES	(1 << 16)
#define ADMA_CH_ACTIVE	(1 << 14)
#define ADMA_FETCH_NEXT_DESC	(1 << 13)
#define ADMA_CH_EN	(1 << 12)
#define ADMA_INTRRUPT_MODE	(1 << 10)

#define ADMA_BURST_LIMIT_MASK      (0x7 << 6)
#define ADMA_BURST_LIMIT(x)        (((x) << 6) & ADMA_BURST_LIMIT_MASK)

#define ADMA_DEST_ADDR_DIR_MASK    (0x3 << 4)
#define ADMA_DEST_ADDR_INCREMENT   (0x0 << 4)
#define ADMA_DEST_ADDR_DECREMENT   (0x1 << 4)
#define ADMA_DEST_ADDR_HOLD        (0x2 << 4)

#define ADMA_SRC_ADDR_DIR_MASK     (0x3 << 2)
#define ADMA_SRC_ADDR_INCREMENT    (0x0 << 2)
#define ADMA_SRC_ADDR_DECREMENT    (0x1 << 2)
#define ADMA_SRC_ADDR_HOLD         (0x2 << 2)

/* current descriptor register */
#define ADMA_CH_CUR_DESC_REG       0x70

/* interrupt mask register */
#define ADMA_CH_INTR_MASK_REG      0x80
#define ADMA_FINISH_INTR_EN        (0x1 << 0)

/* interrupt status register */
#define ADMA_CH_INTR_STATUS_REG    0xa0
#define ADMA_FINISH_INTR_DONE      (0x1 << 0)

#define HDMI_ADMA                   0x50
#define HDMI_ENABLE                 (1 << 0)
#define HDMI_DISABLE                (0 << 0)

#define DESC_BUF_BASE	0xc08d0000
#define DESC_BUF_SIZE	0x400

#define tx_to_adma_desc(tx)	\
		container_of(tx, struct adma_desc_sw, async_tx)
#define to_adma_chan(dchan)	\
		container_of(dchan, struct adma_ch, chan)
#define to_adma_dev(dmadev)	\
		container_of(dmadev, struct adma_dev, device)

#define STARTUP_MSG		"startup"
#define STARTUP_OK_MSG		"startup-ok"
//#define DESC_BUFFER_ADDR

enum {
	AUDIO_SAMPLE_WORD_8BITS = 0x0,
	AUDIO_SAMPLE_WORD_12BITS,
	AUDIO_SAMPLE_WORD_16BITS,
	AUDIO_SAMPLE_WORD_20BITS,
	AUDIO_SAMPLE_WORD_24BITS,
	AUDIO_SAMPLE_WORD_32BITS,
};

struct adma_desc_hw {
	u32 byte_cnt;
	u32 src_addr;
	u32 dst_addr;
	u32 nxt_desc;
};

struct adma_desc_sw {
	struct adma_desc_hw desc;
	struct list_head node;
	struct list_head tx_list;
	struct dma_async_tx_descriptor async_tx;
};

struct adma_pchan;

struct adma_ch {
	struct device	*dev;
	struct dma_chan	chan;
	struct dma_async_tx_descriptor	desc;
	struct adma_pchan	*phy;
	struct dma_slave_config	slave_config;
	enum dma_transfer_direction	dir;
	struct adma_desc_sw *cyclic_first;
	bool unpack_sample;

	struct tasklet_struct	tasklet;
	u32	dev_addr;

	spinlock_t desc_lock;
	struct list_head chain_pending;
	struct list_head chain_running;
	enum dma_status	status;

	struct gen_pool *desc_pool;
};

struct adma_pchan {
	void __iomem *base;
	void __iomem *ctrl_base;
	struct adma_ch *vchan;
};

struct adma_dev {
	int	max_burst_size;
	void __iomem	*base;
	void __iomem	*ctrl_base;
	void __iomem	*desc_base;
	struct dma_device	device;
	struct device	*dev;
	spinlock_t	phy_lock;
};

static unsigned long long private_data[2];

struct instance_data {
	struct rpmsg_device *rpdev;
	struct adma_ch *achan;
};

static void adma_ch_write_reg(struct adma_pchan *phy, u32 reg_offset, u32 value)
{
	writel(value, phy->base + reg_offset);
}

static u32 adma_ch_read_reg(struct adma_pchan *phy, u32 reg_offset)
{
	u32 val;
	return val = readl(phy->base + reg_offset);
}

/*define adma-controller driver*/
static dma_cookie_t adma_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct adma_ch *achan = to_adma_chan(tx->chan);
	struct adma_desc_sw *desc = tx_to_adma_desc(tx);
	struct adma_desc_sw *child;
	unsigned long flags;
	dma_cookie_t cookie = -EBUSY;

	spin_lock_irqsave(&achan->desc_lock, flags);
	list_for_each_entry(child, &desc->tx_list, node) {
		cookie = dma_cookie_assign(&child->async_tx);
	}

	list_splice_tail_init(&desc->tx_list, &achan->chain_pending);
	spin_unlock_irqrestore(&achan->desc_lock, flags);

	return cookie;
}

static int adma_alloc_chan_resources(struct dma_chan *dchan)
{
	struct adma_ch *achan = to_adma_chan(dchan);
	struct adma_dev *adev = to_adma_dev(achan->chan.device);
	if(achan->desc_pool)
		return 1;
	achan->desc_pool = gen_pool_create(7, -1);
	if (!achan->desc_pool) {
		pr_err("unable to allocate descriptor pool\n");
		return -ENOMEM;
	}
	if(gen_pool_add_virt(achan->desc_pool, (long)adev->desc_base, DESC_BUF_BASE,
		DESC_BUF_SIZE, -1) != 0) {
		pr_err("gen_pool_add mem error!\n");
		gen_pool_destroy(achan->desc_pool);
		return -ENOMEM;
	}

	achan->status = DMA_COMPLETE;
	achan->dir = 0;
	achan->dev_addr = 0;
	return 1;
}

static void adma_free_desc_list(struct adma_ch *chan,
					struct list_head *list)
{
	struct adma_desc_sw *desc, *_desc;

	list_for_each_entry_safe(desc, _desc, list, node) {
		list_del(&desc->node);
		gen_pool_free(chan->desc_pool, (long)desc, sizeof(struct adma_desc_sw));
	}
}

static void adma_free_chan_resources(struct dma_chan *dchan)
{
	struct adma_ch *achan = to_adma_chan(dchan);
	struct adma_dev *adev = to_adma_dev(achan->chan.device);
	unsigned long flags;

	spin_lock_irqsave(&achan->desc_lock, flags);
	adma_free_desc_list(achan, &achan->chain_pending);
	adma_free_desc_list(achan, &achan->chain_running);
	spin_unlock_irqrestore(&achan->desc_lock, flags);
	gen_pool_destroy(achan->desc_pool);
	achan->desc_pool = NULL;
	achan->status = DMA_COMPLETE;
	achan->dir = 0;
	achan->dev_addr = 0;
	spin_lock_irqsave(&adev->phy_lock, flags);
	spin_unlock_irqrestore(&adev->phy_lock, flags);
	return;
}

static struct adma_desc_sw *alloc_descriptor(struct adma_ch *achan)
{
	struct adma_desc_sw *desc;
	dma_addr_t pdesc;

	desc = (struct adma_desc_sw*)gen_pool_alloc(achan->desc_pool, sizeof(struct adma_desc_sw));
	if (!desc) {
		dev_err(achan->dev, "out of memory for link descriptor\n");
		return NULL;
	}
	memset(desc, 0, sizeof(struct adma_desc_sw));
	pdesc = (dma_addr_t)gen_pool_virt_to_phys(achan->desc_pool, (long)desc);

	INIT_LIST_HEAD(&desc->tx_list);
	dma_async_tx_descriptor_init(&desc->async_tx, &achan->chan);
	desc->async_tx.tx_submit = adma_tx_submit;
	desc->async_tx.phys = pdesc;
	return desc;
}

static struct dma_async_tx_descriptor *
adma_prep_cyclic(struct dma_chan *dchan, dma_addr_t buf_addr,
				size_t len, size_t period_len,
				enum dma_transfer_direction direction,
				unsigned long flags)
{
	struct adma_ch *achan;
	struct adma_desc_sw *first = NULL, *prev = NULL, *new;
	dma_addr_t adma_src, adma_dst;

	achan = to_adma_chan(dchan);

	switch(direction) {
	case DMA_MEM_TO_DEV:
		adma_src = buf_addr & 0xffffffff;
		achan->dev_addr = achan->slave_config.dst_addr;
		adma_dst = achan->dev_addr;
		break;
	case DMA_DEV_TO_MEM:
		adma_dst = buf_addr & 0xffffffff;
		achan->dev_addr = achan->slave_config.src_addr;
		adma_src = achan->dev_addr;
		break;
	default:
		dev_err(achan->dev, "Unsupported direction for cyclic DMA\n");
		return NULL;
	}
	achan->dir = direction;
	do {
		new = alloc_descriptor(achan);
		if(!new) {
			dev_err(achan->dev, "no memory for desc\n");

		}
		new->desc.byte_cnt = period_len;
		new->desc.src_addr = adma_src;
		new->desc.dst_addr = adma_dst;
		if(!first)
			first = new;
		else
			prev->desc.nxt_desc = new->async_tx.phys;
		new->async_tx.cookie = 0;
		prev = new;
		len -= period_len;

		if(achan->dir == DMA_MEM_TO_DEV)
			adma_src += period_len;
		else
			adma_dst += period_len;
		list_add_tail(&new->node, &first->tx_list);
	}while(len);

	first->async_tx.flags = flags;
	first->async_tx.cookie = -EBUSY;
	new->desc.nxt_desc = first->async_tx.phys;
	achan->cyclic_first = first;
	return &first->async_tx;
}

static int adma_config(struct dma_chan *dchan,
						struct dma_slave_config *cfg)
{
	struct adma_ch *achan = to_adma_chan(dchan);

	memcpy(&achan->slave_config, cfg, sizeof(*cfg));
	return 0;
}

static void set_desc(struct adma_pchan *phy, dma_addr_t addr)
{
	adma_ch_write_reg(phy, NDR, addr);
}

static void set_ctrl_reg(struct adma_pchan *phy)
{
	u32 ctrl_reg_val = 0;
	u32 maxburst = 0, sample_bits = 0;
	enum dma_slave_buswidth width = DMA_SLAVE_BUSWIDTH_UNDEFINED;
	struct adma_ch *achan = phy->vchan;

	if(achan->dir == DMA_MEM_TO_DEV) {
		maxburst = achan->slave_config.dst_maxburst;
		width = achan->slave_config.dst_addr_width;
		ctrl_reg_val |= ADMA_DEST_ADDR_HOLD | ADMA_SRC_ADDR_INCREMENT;
	}
	else if(achan->dir == DMA_DEV_TO_MEM) {
		maxburst = achan->slave_config.src_maxburst;
		width = achan->slave_config.src_addr_width;
		ctrl_reg_val |= ADMA_SRC_ADDR_HOLD | ADMA_DEST_ADDR_INCREMENT;
	}
	else
		ctrl_reg_val |= ADMA_SRC_ADDR_HOLD | ADMA_DEST_ADDR_HOLD;

	if(width == DMA_SLAVE_BUSWIDTH_1_BYTE)
		sample_bits = AUDIO_SAMPLE_WORD_8BITS;
	else if(width == DMA_SLAVE_BUSWIDTH_2_BYTES)
		sample_bits = AUDIO_SAMPLE_WORD_16BITS;
	else if(width == DMA_SLAVE_BUSWIDTH_3_BYTES)
		sample_bits = AUDIO_SAMPLE_WORD_24BITS;
	else if(width == DMA_SLAVE_BUSWIDTH_4_BYTES)
		sample_bits = AUDIO_SAMPLE_WORD_32BITS;
	ctrl_reg_val |= ADMA_SAMPLE_BITS(sample_bits);

	/*no burst function information,default 0*/
	ctrl_reg_val |= ADMA_BURST_LIMIT(0);
	ctrl_reg_val |= ADMA_CH_ABORT;
	if(achan->unpack_sample)
		ctrl_reg_val |= ADMA_UNPACK_SAMPLES;
	adma_ch_write_reg(phy, DCR, ctrl_reg_val);
	if(!achan->unpack_sample)
		writel(HDMI_ENABLE, phy->ctrl_base);
}

static void enable_chan(struct adma_pchan *phy)
{
	u32 ctrl_val;
	struct adma_ch *achan = phy->vchan;

	if(achan->dir == DMA_MEM_TO_DEV)
		adma_ch_write_reg(phy, DAR, achan->dev_addr);
	else if(achan->dir == DMA_DEV_TO_MEM)
		adma_ch_write_reg(phy, SAR, achan->dev_addr);
	adma_ch_write_reg(phy, IER, 1);
	ctrl_val = adma_ch_read_reg(phy, DCR);
	ctrl_val |= ADMA_FETCH_NEXT_DESC;
	ctrl_val |= ADMA_CH_EN;
	adma_ch_write_reg(phy, DCR, ctrl_val);
}

static void start_pending_queue(struct adma_ch *achan)
{
	struct adma_dev *adev = to_adma_dev(achan->chan.device);
	struct adma_pchan *phy;
	struct adma_desc_sw *desc;
	unsigned long flags;

	if(achan->status == DMA_IN_PROGRESS) {
		dev_dbg(achan->dev, "DMA controller still busy\n");
		return;
	}
	spin_lock_irqsave(&adev->phy_lock, flags);
	phy = achan->phy;
	desc = list_first_entry(&achan->chain_pending,
							struct adma_desc_sw, node);
	list_splice_tail_init(&achan->chain_pending, &achan->chain_running);
	set_desc(phy, desc->async_tx.phys);
	set_ctrl_reg(phy);
	enable_chan(phy);
	spin_unlock_irqrestore(&adev->phy_lock, flags);
	achan->status = DMA_IN_PROGRESS;
}

static void adma_issue_pending(struct dma_chan *dchan)
{
	struct adma_ch *achan = to_adma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&achan->desc_lock, flags);
	start_pending_queue(achan);
	spin_unlock_irqrestore(&achan->desc_lock, flags);
}

static enum dma_status adma_tx_status(struct dma_chan *dchan,
							dma_cookie_t cookie,
							struct dma_tx_state *txstate)
{
	/*struct adma_ch *chan = to_adma_chan(dchan);
	enum dma_status ret;
	unsigned long flags;
	spin_lock_irqsave(&chan->desc_lock, flags);
	ret = dma_cookie_status(dchan, cookie, txstate);
	if (likely(ret != DMA_ERROR))
		dma_set_residue(txstate, mmp_pdma_residue(chan, cookie));
	spin_unlock_irqrestore(&chan->desc_lock, flags);
	if (ret == DMA_COMPLETE)
		return ret;
	else
		return chan->status;*/
	return 0;
}

static void disable_chan(struct adma_pchan *phy)
{
	u32 reg_val = adma_ch_read_reg(phy,DCR);
	reg_val |= ADMA_CH_ABORT;
	adma_ch_write_reg(phy, DCR, reg_val);

	udelay(500);
	reg_val = adma_ch_read_reg(phy, DCR);
	reg_val &= ~ADMA_CH_EN;
	adma_ch_write_reg(phy, DCR, reg_val);
	adma_ch_write_reg(phy, IER, 0);
	if((!phy->vchan->unpack_sample) && ((readl(phy->ctrl_base) & HDMI_ENABLE) == 0x1))
		writel(HDMI_DISABLE, phy->ctrl_base);
}

static int adma_terminate_all(struct dma_chan *dchan)
{
	struct adma_ch *achan = to_adma_chan(dchan);
	struct adma_dev *adev = to_adma_dev(achan->chan.device);
	unsigned long flags;

	spin_lock_irqsave(&achan->desc_lock, flags);
	disable_chan(achan->phy);
	achan->status = DMA_COMPLETE;
	spin_lock_irqsave(&adev->phy_lock, flags);
	spin_unlock_irqrestore(&adev->phy_lock, flags);

	adma_free_desc_list(achan, &achan->chain_pending);
	adma_free_desc_list(achan, &achan->chain_running);
	//achan->bytes_residue = 0;

	spin_unlock_irqrestore(&achan->desc_lock, flags);
	return 0;
}

static struct dma_chan *adma_dma_xlate(struct of_phandle_args *dma_spec,
				struct of_dma *ofdma)
{
	struct adma_dev *d = ofdma->of_dma_data;
	struct dma_chan *chan;

	chan = dma_get_any_slave_channel(&d->device);
	if (!chan)
		return NULL;
	return chan;
}

static const struct of_device_id adma_id_table[] = {
	{ .compatible = "ky,x1-adma", .data =(void *)&private_data[0] },
	{},
};

static int adma_probe(struct platform_device *pdev)
{
	struct adma_dev *adev;
	struct device *dev;
	const struct of_device_id *of_id;
	struct rpmsg_device *rpdev;
	struct instance_data *idata;
	struct adma_pchan *phy;
	struct adma_ch *achan;
	int ret;
	const enum dma_slave_buswidth widths =
		DMA_SLAVE_BUSWIDTH_1_BYTE | DMA_SLAVE_BUSWIDTH_2_BYTES |
		DMA_SLAVE_BUSWIDTH_3_BYTES | DMA_SLAVE_BUSWIDTH_4_BYTES;

	of_id = of_match_device(adma_id_table, &pdev->dev);
	if (!of_id) {
		pr_err("Unable to match OF ID\n");
		return -ENODEV;
	}
	idata = (struct instance_data *)((unsigned long long *)(of_id->data))[0];
	rpdev = idata->rpdev;
	ret = rpmsg_send(rpdev->ept, STARTUP_MSG, strlen(STARTUP_MSG));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	/*get controller dts info*/
	dev = &pdev->dev;
	adev = devm_kzalloc(dev, sizeof(*adev), GFP_KERNEL);
	adev->dev = dev;
	adev->base = devm_platform_ioremap_resource_byname(pdev, "adma_reg");
	if(IS_ERR(adev->base))
		return PTR_ERR(adev->base);
	adev->ctrl_base = devm_platform_ioremap_resource_byname(pdev, "ctrl_reg");
	if(IS_ERR(adev->ctrl_base))
		return PTR_ERR(adev->ctrl_base);
	adev->desc_base = devm_platform_ioremap_resource_byname(pdev, "buf_addr");
	if(IS_ERR(adev->desc_base))
		return PTR_ERR(adev->desc_base);
	/*if(of_property_read_u32(pdev->dev->of_node, "max-burst-size", &adev->max_burst_size))
		adev->max_burst_size = DEFAULT_MAX_BURST_SIZE;*/

	/*init adma-chan*/
	INIT_LIST_HEAD(&adev->device.channels);
	achan = devm_kzalloc(dev, sizeof(struct adma_ch), GFP_KERNEL);
	if(achan == NULL)
		return -ENOMEM;
	phy = devm_kzalloc(dev, sizeof(struct adma_pchan), GFP_KERNEL);
	phy->base = adev->base;
	phy->ctrl_base = adev->ctrl_base;
	phy->vchan = achan;
	achan->phy = phy;
	achan->dev = adev->dev;
	achan->chan.device = &adev->device;
	spin_lock_init(&achan->desc_lock);
	spin_lock_init(&adev->phy_lock);
	INIT_LIST_HEAD(&achan->chain_pending);
	INIT_LIST_HEAD(&achan->chain_running);
	achan->status = DMA_COMPLETE;
	achan->unpack_sample = !of_property_read_bool(pdev->dev.of_node, "hdmi-sample");

	/* register virt channel to dma engine */
	list_add_tail(&achan->chan.device_node, &adev->device.channels);
	idata->achan = achan;

	dma_cap_set(DMA_SLAVE, adev->device.cap_mask);
	dma_cap_set(DMA_CYCLIC, adev->device.cap_mask);
	adev->device.dev = dev;
	adev->device.device_tx_status = adma_tx_status;
	adev->device.device_alloc_chan_resources = adma_alloc_chan_resources;
	adev->device.device_free_chan_resources = adma_free_chan_resources;
	adev->device.device_prep_dma_cyclic = adma_prep_cyclic;
	adev->device.device_issue_pending = adma_issue_pending;
	adev->device.device_config = adma_config;
	adev->device.device_terminate_all = adma_terminate_all;
	adev->device.copy_align = DMAENGINE_ALIGN_8_BYTES;
	adev->device.src_addr_widths = widths;
	adev->device.dst_addr_widths = widths;
	adev->device.directions = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);

	dma_set_mask(adev->dev, adev->dev->coherent_dma_mask);

	ret = dma_async_device_register(&adev->device);
	if(ret) {
		dev_err(adev->device.dev, "unable to register\n");
		return ret;
	}

	if(pdev->dev.of_node) {
		ret = of_dma_controller_register(pdev->dev.of_node,
										adma_dma_xlate,adev);
		if(ret < 0){
			dev_err(dev, "of_dma_controller_register failed\n");
			dma_async_device_unregister(&adev->device);
			return ret;
		}
	}

	platform_set_drvdata(pdev, adev);
	return 0;
}

static int adma_remove(struct platform_device *pdev)
{
	struct adma_dev *adev = platform_get_drvdata(pdev);;

	if(pdev->dev.of_node)
		of_dma_controller_free(pdev->dev.of_node);
	dma_async_device_unregister(&adev->device);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver adma_driver = {
	.driver	= {
		.name	= "x1-adma",
		.of_match_table	= adma_id_table,
	},
	.probe	= adma_probe,
	.remove	= adma_remove,
};

static struct rpmsg_device_id rpmsg_driver_adma_id_table[] = {
	{ .name	= "adma-service", .driver_data = 0 },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_adma_id_table);

static int rpmsg_adma_client_cb(struct rpmsg_device *rpdev, void *data,
		int len, void *priv, u32 src)
{
	struct instance_data *idata = dev_get_drvdata(&rpdev->dev);
	struct adma_ch *chan = idata->achan;

#if 0
	if (strcmp(data, STARTUP_OK_MSG) == 0) {
		dev_info(&rpdev->dev, "channel: 0x%x -> 0x%x startup ok!\n",
					rpdev->src, rpdev->dst);
	}

	if (strcmp(data, "#") == 0) {
#endif
		/* adma irq happend */
		struct adma_desc_sw *desc;
		LIST_HEAD(chain_cleanup);
		unsigned long flags;
		struct dmaengine_desc_callback cb;

		spin_lock_irqsave(&chan->desc_lock, flags);
		if (chan->status == DMA_COMPLETE) {
			spin_unlock_irqrestore(&chan->desc_lock, flags);
			return 0;
		}
		spin_unlock_irqrestore(&chan->desc_lock, flags);

		spin_lock_irqsave(&chan->desc_lock, flags);
		desc = chan->cyclic_first;
		dmaengine_desc_get_callback(&desc->async_tx, &cb);
		spin_unlock_irqrestore(&chan->desc_lock, flags);

		dmaengine_desc_callback_invoke(&cb, NULL);
//	}

	return 0;
}

static int rpmsg_adma_client_probe(struct rpmsg_device *rpdev)
{
	struct instance_data *idata;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
					rpdev->src, rpdev->dst);

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	dev_set_drvdata(&rpdev->dev, idata);
	idata->rpdev = rpdev;

	((unsigned long long *)(adma_id_table[0].data))[0] = (unsigned long long)idata;

	platform_driver_register(&adma_driver);

	return 0;
}

static void rpmsg_adma_client_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg adma client driver is removed\n");
	platform_driver_unregister(&adma_driver);
}

static struct rpmsg_driver rpmsg_adma_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_adma_id_table,
	.probe		= rpmsg_adma_client_probe,
	.callback	= rpmsg_adma_client_cb,
	.remove		= rpmsg_adma_client_remove,
};
module_rpmsg_driver(rpmsg_adma_client);
