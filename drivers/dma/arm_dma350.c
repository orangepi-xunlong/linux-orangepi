// SPDX-License-Identifier: GPL-2.0
/*
 *Driver for ARM DMA-350 controller
 *
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/acpi.h>
#include <linux/acpi_dma.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_dma.h>
#include <linux/of_reserved_mem.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>
#include "dma350_ch_drv.h"
#include "virt-dma.h"

#define DRIVER_NAME	"arm-dma350"
#define DMA_ALIGN	4
#define DMA_MAX_SIZE	(0xFFFFFFFF)
#define MAX_BURST_LEN   0xF
#define LLI_BLOCK_SIZE	(4 * PAGE_SIZE)
#define CHANNEL_IDX(id)   (0x1000 + id * 0x100)
#define CMD_LINK_LEN    (512)
#define MAX_DESC_NUM 16
#define MAX_CHAN_NUM 8
#define RCSU_OFFSET 0x10000
#define ARM_DMA350__PM_TIMEOUT_MS 500

#define AUDSS_DMAC_INFO_AP_IRQ 0x54
#define AUDSS_OFFSET  (0x30000000-0xc0000000)
#define DMA350_DYNAMIC_CH(ch) (ch == 0xFF)

enum arm_dma350_burst_width_t {
	DMA350_DMA_WIDTH_8BIT	= 0,
	DMA350_DMA_WIDTH_16BIT	= 1,
	DMA350_DMA_WIDTH_32BIT	= 2,
	DMA350_DMA_WIDTH_64BIT	= 3
};


struct dma350_desc_hw {
	dma_addr_t saddr;
	dma_addr_t daddr;
	u32 x_len;
	u32 dest_addr_inc;
	u32 src_addr_inc;
	u32 trig_sel;
	u32 burst_size;
} __aligned(32);

struct arm_dma350_desc_sw {
	struct virt_dma_desc	vd;
	dma_addr_t		desc_hw_lli;
	size_t			desc_num;
	size_t			size;
	struct dma350_desc_hw	*desc_hw;
};

struct arm_dma350_phy;

struct arm_dma350_chan {
	struct dma_slave_config slave_cfg;
	u32			id; /* Request phy chan id */
	u32			req_line;
	u32			priority;
	u32			cyclic;
	u32                     transize;
	bool			is_dynamic;
	struct virt_dma_chan	vc;
	struct arm_dma350_phy	*phy;
	struct list_head	node;
	dma_addr_t		dev_addr;
	enum dma_status		status;
};

struct arm_dma350_phy {
	u32			idx;
	void __iomem		*base;
	struct arm_dma350_chan	*vchan;
	struct arm_dma350_desc_sw	*ds_run;
	struct arm_dma350_desc_sw	*ds_done;
};

struct host_remote_map {
	u32 reg_host_addr;
	u32 reg_remote_addr;
	u32 ram_host_addr;
	u32 ram_remote_addr;
};

struct arm_dma350_drvdata {
        bool is_exist_pause;
};

struct arm_dma350_dev {
	struct dma_device	slave;
	void __iomem		*base;
	spinlock_t		lock; /* lock for ch and phy */
	struct list_head	chan_pending;
	struct arm_dma350_phy	*phy;
	struct arm_dma350_chan	*chans;
	struct clk		*clk;
	struct dma_pool		*pool;
	u32			max_channels;
	u32			max_requests;
	int			irq;
	struct host_remote_map  map;
	struct regmap           *regmap;
	struct arm_dma350_drvdata *drvdata;
	struct reset_control	*dma_reset;
	bool			is_clk_enable_atomic;
};

#define to_arm_dma350(dmadev) container_of(dmadev, struct arm_dma350_dev, slave)


static u32 dmachan[MAX_CHAN_NUM];
static u32 desnum[MAX_CHAN_NUM];

static struct dma350_cmdlink_gencfg_t cmdlink_cfg[MAX_CHAN_NUM][MAX_DESC_NUM];
static u32 *cmd0[MAX_CHAN_NUM];


static dma_addr_t  phy_cmd0[MAX_CHAN_NUM];

static struct arm_dma350_chan *to_dma350_chan(struct dma_chan *chan)
{
	return container_of(chan, struct arm_dma350_chan, vc.chan);
}

static u32 *dma350_cmdlink_generate(struct dma350_cmdlink_gencfg_t *cmdlink_cfg, u32 *buffer, u32 *bit)
{
	u32 *cfg;
	u32 header_sel;

	cfg = (u32 *)&cmdlink_cfg->cfg;
	*(buffer) = cmdlink_cfg->header;
	buffer++;
	/*
	 *Note: REGCLEAR (Bit 0) has no associated field and Bit 1 is reserved,
	 *cfg starts from Bit 2
	 */
	for (header_sel = (0x1UL << 2); header_sel; header_sel <<= 1) {
		if (cmdlink_cfg->header & header_sel) {
			(*bit)++;
			*(buffer) = *(cfg);
			buffer++;
			cfg++;
		} else {
			cfg++;
		}
	}
	return buffer;
}

static void dma350_cmdlink_init(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
	static const struct dma350_cmdlink_gencfg_t default_cmdlink = {
	.header = 0,
	.cfg = {.intren = DMA350_CH_INTREN_RESET_VALUE,
	.ctrl = DMA350_CH_CTRL_RESET_VALUE,
	.srcaddr = 0,
	.srcaddrhi = 0,
	.desaddr = 0,
	.desaddrhi = 0,
	.xsize = 0,
	.xsizehi = 0,
	.srctranscfg = DMA350_CH_SRCTRANSCFG_RESET_VALUE,
	.destranscfg = DMA350_CH_DESTRANSCFG_RESET_VALUE,
	.xaddrinc = 0,
	.yaddrstride = 0,
	.fillval = 0,
	.ysize = 0,
	.tmpltcfg = 0,
	.srctmplt = 0,
	.destmplt = 0,
	.srctrigincfg = 0,
	.destrigincfg = 0,
	.trigoutcfg = 0,
	.gpoen0 = 0,
	.reserved0 = 0,
	.gpoval0 = 0,
	.reserved1 = 0,
	.streamintcfg = 0,
	.reserved2 = 0,
	.linkattr = 0,
	.autocfg = DMA350_CH_AUTOCFG_RESET_VALUE,
	.linkaddr = DMA350_CH_LINKADDR_RESET_VALUE,
	.linkaddrhi = 0}};
	*cmdlink_cfg = default_cmdlink;
}

static void arm_dma350_terminate_chan(struct arm_dma350_phy *phy)
{
	dma350_ch_cmd(phy->base + DMA350_REG_CMD, DMA350_CH_CMD_STOPCMD);
}

static void arm_dma350_nonsec_intren(struct arm_dma350_dev *d)
{
	writel_relaxed(INTREN_ANYCHINTR_ENABLE, d->base + DMA350_REG_CHINTRSTATUS0 + DMA350_NONSSEC_CTRL);
}


static void arm_dma350_clear_chan(struct arm_dma350_phy *phy)
{
	dma350_ch_cmd(phy->base + DMA350_REG_CMD, DMA350_CH_CMD_CLEARCMD);
}

static void arm_dma350_set_xsize(struct arm_dma350_phy *phy, u32 xsize)
{
	u32 val = 0;

	val = ((xsize & 0x0000FFFFUL) << 16U) | (xsize & 0x0000FFFFUL);
	writel_relaxed(val, phy->base + DMA350_REG_XSIZE);
	val = (xsize & 0xFFFF0000UL) | ((xsize & 0xFFFF0000UL) >> 16U);
	writel_relaxed(val, phy->base + DMA350_REG_XSIZEHI);
}

static void arm_dma350_set_desc(struct arm_dma350_phy *phy, struct dma350_desc_hw *hw)
{
	writel_relaxed(hw->saddr, phy->base + DMA350_REG_SRCADDR);
	writel_relaxed(hw->daddr, phy->base + DMA350_REG_DESADDR);
	arm_dma350_set_xsize(phy, hw->x_len);
	dma350_ch_cmd(phy->base + DMA350_REG_CMD, DMA350_CH_CMD_ENABLECMD);
}

static void arm_dma350_init_state(struct arm_dma350_phy *phy)
{
	dma350_ch_enable_intr(phy->base + DMA350_REG_INTREN, DMA350_CH_INTREN_DONE|     \
	DMA350_CH_INTREN_ERR | DMA350_CH_INTREN_DISABLED | DMA350_CH_INTREN_STOPPED);
}

static int arm_dma350_start_txd(struct arm_dma350_chan *c)
{
	struct virt_dma_desc *vd = vchan_next_desc(&c->vc);
	struct dma_slave_config *cfg = &c->slave_cfg;
	u32 timeout = 0;
	if (!c->phy)
		return -EAGAIN;
	if (vd) {
		struct arm_dma350_desc_sw *ds =
			container_of(vd, struct arm_dma350_desc_sw, vd);
		/*
		 * fetch and remove request from vc->desc_issued
		 * so vc->desc_issued only contains desc pending
		 */
		if (!c->cyclic) {
			list_del(&ds->vd.node);
		}
		c->phy->ds_run = ds;
		c->phy->ds_done = NULL;

		/* start dma */
		if (c->cyclic) {
			dma350_ch_set_xsize32(c->phy->base + DMA350_REG_XSIZE, 0, 0);
			writel_relaxed(0, c->phy->base + DMA350_REG_INTREN);
			dma350_ch_cmd(c->phy->base + DMA350_REG_CMD, DMA350_CH_CMD_ENABLECMD);
			dma350_ch_set_linkaddr32(c->phy->base + DMA350_REG_LINKADDR, (u32)(phy_cmd0[c->id] + AUDSS_OFFSET));
			dma350_ch_enable_linkaddr(c->phy->base + DMA350_REG_LINKADDR);
			dma350_ch_cmd(c->phy->base + DMA350_REG_CMD, DMA350_CH_CMD_ENABLECMD);
			if(cfg->direction == DMA_MEM_TO_DEV) {
				timeout = 0;
				while((!(readl_relaxed(c->phy->base + DMA350_REG_STATUS) & \
					DMA350_CH_STATUS_STAT_DESTRIGINWAIT_Msk))) {
					udelay(10);
					timeout++;
					if(timeout > 10) {
						pr_info("channel %d timeout\n",c->id);
						break;
					}
				}
			}
			if(cfg->direction == DMA_DEV_TO_MEM){
				timeout = 0;
				while((!(readl_relaxed(c->phy->base + DMA350_REG_STATUS) & \
					DMA350_CH_STATUS_STAT_SRCTRIGINWAIT_Msk))) {
					udelay(10);
					timeout++;
					if(timeout > 10) {
						pr_info("channel %d timeout\n",c->id);
						break;
					}
				}
			}
		} else {
			arm_dma350_set_desc(c->phy, ds->desc_hw);
		}
		return 0;
	}
	c->phy->ds_done = NULL;
	c->phy->ds_run = NULL;

	return -EAGAIN;
}

static void arm_dma350_task(struct arm_dma350_dev *d)
{
	struct arm_dma350_phy *p;
	struct arm_dma350_chan *c;
	unsigned int pch, pch_alloc = 0;
	unsigned long flags;

	/* check new channel request in d->chan_pending */
	spin_lock_irqsave(&d->lock, flags);
	while (!list_empty(&d->chan_pending)) {
		c = list_first_entry(&d->chan_pending,
				     struct arm_dma350_chan, node);
		p = &d->phy[c->id];
		/* remove from d->chan_pending */
		list_del_init(&c->node);
		pch_alloc |= 1 << c->id;
		/* Mark this channel allocated */
		p->vchan = c;
		c->phy = p;
	}
	spin_unlock_irqrestore(&d->lock, flags);
	for (pch = 0; pch < d->max_channels; pch++) {

		if (pch_alloc & (1 << pch)) {
			p = &d->phy[pch];
			c = p->vchan;
			if (c) {
				spin_lock_irqsave(&c->vc.lock, flags);
				arm_dma350_start_txd(c);
				spin_unlock_irqrestore(&c->vc.lock, flags);
			}
		}
	}
}

static u32 arm_dma_ch_getstatus(struct arm_dma350_dev *d, u32 ch_id)
{
	return readl_relaxed(d->base + DMA350_REG_STATUS + CHANNEL_IDX(ch_id));
}

static u32 arm_dma_ch_clrstatus(struct arm_dma350_dev *d, u32 ch_id)
{
	u32 val = arm_dma_ch_getstatus(d, ch_id);

	val &= DMA350_CH_INTREN_DONE | DMA350_CH_INTREN_ERR | DMA350_CH_INTREN_DISABLED | DMA350_CH_INTREN_STOPPED;
	writel_relaxed(val << 16, d->base + DMA350_REG_STATUS + CHANNEL_IDX(ch_id));
	return 0;
}

static irqreturn_t arm_dma350_int_handler(int irq, void *dev_id)
{
	struct arm_dma350_dev *d = (struct arm_dma350_dev *)dev_id;
	struct arm_dma350_phy *p;
	struct arm_dma350_chan *c;
	u32 chstatus = 0, ch_idx = 0;

	while (ch_idx < MAX_CHAN_NUM) {
		chstatus = arm_dma_ch_getstatus(d, ch_idx);
		if (chstatus == 0) {
		ch_idx++;
		continue;
	}
	if (chstatus & DMA350_CH_STATUS_INTR_ERR) {
		writel_relaxed(DMA350_CH_STATUS_STAT_ERR, d->base + DMA350_REG_STATUS + CHANNEL_IDX(ch_idx));
		pr_err("transfer error!!!\n");
	}
	if ((chstatus & DMA350_CH_STATUS_INTR_DONE) == 0) {
		ch_idx++;
		continue;
	}

	arm_dma_ch_clrstatus(d, ch_idx);
	p = &d->phy[ch_idx];
	if (p == NULL) {
		ch_idx++;
		continue;
	}
	c = p->vchan;
	if (c) {
		spin_lock(&c->vc.lock);
		if (c->cyclic) {
			desnum[ch_idx]++;
			if (dmachan[ch_idx] == (desnum[ch_idx] - 1))
				desnum[ch_idx] = 1;
			if (p->ds_run != NULL)
				vchan_cyclic_callback(&p->ds_run->vd);
		} else {
			if(p->ds_run != NULL) {
				vchan_cookie_complete(&p->ds_run->vd);
				p->ds_done = p->ds_run;
				p->ds_run = NULL;
			}
		}
		spin_unlock(&c->vc.lock);
	}
	ch_idx++;
	}
	return IRQ_HANDLED;
}

static void arm_dma350_free_chan_resources(struct dma_chan *chan)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	unsigned long flags;

	spin_lock_irqsave(&d->lock, flags);
	list_del_init(&c->node);
	spin_unlock_irqrestore(&d->lock, flags);

	vchan_free_chan_resources(&c->vc);
}


static u32 get_bytes_in_phy_channel(struct arm_dma350_phy *phy)
{
	u32 witdth;
	u32 bytes;
        u32 byteslo,byteshi;
	byteslo = readl_relaxed(phy->base + DMA350_REG_XSIZE) & 0xFFFF;
	byteshi = (readl_relaxed(phy->base + DMA350_REG_XSIZE) & 0xFFFF0000) >> 16;
        if(byteslo > byteshi)
		bytes = byteshi;
	else
		bytes = byteslo;
	witdth = readl_relaxed(phy->base + DMA350_REG_CTRL) & DMA350_CH_CTRL_TRANSIZE_Msk;
	switch (witdth) {
	case DMA350_DMA_WIDTH_8BIT:
		break;
	case DMA350_DMA_WIDTH_16BIT:
		bytes *= 2;
		break;
	case DMA350_DMA_WIDTH_32BIT:
		bytes *= 4;
		break;
	}
	return bytes;
}

static enum dma_status arm_dma350_tx_status(struct dma_chan *chan,
					dma_cookie_t cookie,
					struct dma_tx_state *state)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_phy *p;
	struct virt_dma_desc *vd;
	unsigned long flags;
	enum dma_status ret;
	size_t bytes = 0;
	size_t residue = 0;

	ret = dma_cookie_status(&c->vc.chan, cookie, state);

	if (ret == DMA_COMPLETE || !state)
		return ret;

	spin_lock_irqsave(&c->vc.lock, flags);
	p = c->phy;
	ret = c->status;

	/*
	 * If the cookie is on our issue queue, then the residue is
	 * its total size.
	 */
	if(c->cyclic){
		vd = vchan_find_desc(&c->vc, cookie);
		if (vd) {
			bytes = container_of(vd, struct arm_dma350_desc_sw, vd)->size;
			residue = bytes - bytes / dmachan[c->id] * desnum[c->id];
		}
	} else {
		residue = get_bytes_in_phy_channel(p);
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
	dma_set_residue(state, residue);
	return ret;
}

static void arm_dma350_issue_pending(struct dma_chan *chan)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	unsigned long flags;
	int issue = 0;

	spin_lock_irqsave(&c->vc.lock, flags);
	/* add request to vc->desc_issued */
	if (vchan_issue_pending(&c->vc)) {
		spin_lock(&d->lock);
		if (list_empty(&c->node)) {
			/* if new channel, add chan_pending */
			list_add_tail(&c->node, &d->chan_pending);
			issue = 1;
		}
		spin_unlock(&d->lock);
	} else {
		dev_info(d->slave.dev, "nothing to issue\n");
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
	if (issue)
		arm_dma350_task(d);
}

static void arm_dma350_fill_desc(struct arm_dma350_desc_sw *ds, dma_addr_t dst,
			     dma_addr_t src, size_t len, u32 num)
{
	ds->desc_hw[num].saddr = src;
	ds->desc_hw[num].daddr = dst;
	ds->desc_hw[num].x_len = len;
}

static struct arm_dma350_desc_sw *arm_dma350_alloc_desc_resource(int num,
						     struct dma_chan *chan)
{
	struct arm_dma350_desc_sw *ds;
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	int lli_limit = LLI_BLOCK_SIZE / sizeof(struct dma350_desc_hw);

	if (num > lli_limit) {
		dev_err(chan->device->dev, "sg num %d exceed max %d\n", num, lli_limit);
		return NULL;
	}
	ds = kzalloc(sizeof(*ds), GFP_ATOMIC);
	if (!ds)
		return NULL;

	ds->desc_hw = dma_pool_zalloc(d->pool, GFP_ATOMIC, &ds->desc_hw_lli);
	if (!ds->desc_hw) {
		dev_err(chan->device->dev, "dma alloc fail\n");
		kfree(ds);
		return NULL;
	}
	ds->desc_num = num;
	return ds;
}

static int arm_dma350_pre_config_sg(struct arm_dma350_chan *c, enum dma_transfer_direction dir)
{

	struct arm_dma350_dev *dev = to_arm_dma350(c->vc.chan.device);
	struct arm_dma350_phy *p = &dev->phy[c->id];
	struct dma_slave_config *cfg = &c->slave_cfg;
	u32 maxburst = 0;
	int ret = 0;

	ret = pm_runtime_resume_and_get(dev->slave.dev);
	if (ret < 0) {
		pr_err("%s,pm get err, %d\n",__func__,ret);
		return ret;
	}
	arm_dma350_nonsec_intren(dev);
	dma350_ch_set_xtype(p->base + DMA350_REG_CTRL, DMA350_CH_XTYPE_CONTINUE);

	switch (dir) {
	case DMA_MEM_TO_DEV:
		dma350_ch_enable_intr(p->base + DMA350_REG_INTREN, DMA350_CH_INTREN_DONE | DMA350_CH_INTREN_ERR );
		c->dev_addr = cfg->dst_addr - RCSU_OFFSET;
		c->transize = cfg->dst_addr_width;
		/* dst len is calculated from src width, len and dst width.
		 * We need make sure dst len not exceed MAX LEN.
		 * Trailing single transaction that does not fill a full
		 * burst also require identical src/dst data width.
		 */
		maxburst = cfg->dst_maxburst - 1;
		maxburst = maxburst < MAX_BURST_LEN ? maxburst : MAX_BURST_LEN;
		dma350_ch_set_desmaxburstlen(p->base + DMA350_REG_DESTRANSCFG, maxburst);
		dma350_ch_set_transize(p->base + DMA350_REG_CTRL, ffs(cfg->dst_addr_width) - 1);
		dma350_ch_enable_destrigin(p->base + DMA350_REG_CTRL);
		dma350_ch_src_memattr(p->base + DMA350_REG_SRCTRANSCFG, 4, 4);
		dma350_ch_des_memattr(p->base + DMA350_REG_DESTRANSCFG, 0, 0);
		dma350_ch_set_destrigintype(p->base + DMA350_REG_DESTRIGINCFG, DMA350_CH_DESTRIGINTYPE_HW);
		dma350_ch_set_destriginmode(p->base + DMA350_REG_DESTRIGINCFG, DMA350_CH_DESTRIGINMODE_PERIPH_FLOW_CTRL);
		dma350_ch_set_destriginsel(p->base + DMA350_REG_DESTRIGINCFG, c->req_line);
		dma350_ch_set_xaddr_inc(p->base + DMA350_REG_XADDRINC, 1, 0);

		break;
	case DMA_DEV_TO_MEM:
		dma350_ch_enable_intr(p->base + DMA350_REG_INTREN, DMA350_CH_INTREN_DONE | DMA350_CH_INTREN_ERR );
		c->dev_addr = cfg->src_addr - RCSU_OFFSET;
		c->transize = cfg->src_addr_width;
		maxburst = cfg->src_maxburst - 1;
		maxburst = maxburst < MAX_BURST_LEN ? maxburst : MAX_BURST_LEN;
		dma350_ch_set_srcmaxburstlen(p->base + DMA350_REG_DESTRANSCFG, maxburst);
		dma350_ch_set_transize(p->base + DMA350_REG_CTRL, ffs(cfg->src_addr_width) - 1);
		dma350_ch_enable_srctrigin(p->base + DMA350_REG_CTRL);

		dma350_ch_src_memattr(p->base + DMA350_REG_SRCTRANSCFG, 0, 0);

		/*set des attribute to device memory,to fix the SOCHW-1449 bug*/
		dma350_ch_des_memattr(p->base + DMA350_REG_DESTRANSCFG, 0, 0);
		dma350_ch_set_srctrigintype(p->base + DMA350_REG_SRCTRIGINCFG, DMA350_CH_SRCTRIGINTYPE_HW);
		dma350_ch_set_srctriginmode(p->base + DMA350_REG_SRCTRIGINCFG, DMA350_CH_SRCTRIGINMODE_PERIPH_FLOW_CTRL);
		dma350_ch_set_srctriginsel(p->base + DMA350_REG_SRCTRIGINCFG, c->req_line);
		dma350_ch_set_xaddr_inc(p->base + DMA350_REG_XADDRINC, 0, 1);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int arm_dma350_pre_config_cyclic(struct arm_dma350_chan *c, enum dma_transfer_direction dir)
{

	struct arm_dma350_dev *dev = to_arm_dma350(c->vc.chan.device);
	struct arm_dma350_phy *p = &dev->phy[c->id];
	struct dma_slave_config *cfg = &c->slave_cfg;
	u32 maxburst = 0;

	if (dev->regmap)
		regmap_update_bits(dev->regmap, AUDSS_DMAC_INFO_AP_IRQ,
					1 << c->id, 1 << c->id);

	arm_dma350_clear_chan(p);
	dma350_ch_set_xtype(p->base + DMA350_REG_CTRL, DMA350_CH_XTYPE_CONTINUE);
	dma350_ch_set_chprio(p->base + DMA350_REG_CTRL, c->priority);

	/* Setup first */
	dma350_cmdlink_init(&cmdlink_cfg[c->id][0]);
	/* Clear DMA registers upon loading this command */
	dma350_cmdlink_set_regclear(&cmdlink_cfg[c->id][0]);
	dma350_cmdlink_set_xtype(&cmdlink_cfg[c->id][0], DMA350_CH_XTYPE_CONTINUE);
	switch (dir) {
	case DMA_MEM_TO_DEV:
		c->dev_addr = cfg->dst_addr;
		c->transize = cfg->dst_addr_width;

		/* dst len is calculated from src width, len and dst width.
		* We need make sure dst len not exceed MAX LEN.
		* Trailing single transaction that does not fill a full
		* burst also require identical src/dst data width.
		*/
		maxburst = cfg->dst_maxburst - 1;
		maxburst = maxburst < MAX_BURST_LEN ? maxburst : MAX_BURST_LEN;
		dma350_ch_set_transize(p->base + DMA350_REG_CTRL, ffs(cfg->dst_addr_width) - 1);

		/*generate cmd link list*/
		dma350_cmdlink_set_desaddr32(&cmdlink_cfg[c->id][0], c->dev_addr);
		dma350_cmdlink_set_transize(&cmdlink_cfg[c->id][0], ffs(cfg->dst_addr_width) - 1);
		dma350_cmdlink_set_srcmemattrhi(&cmdlink_cfg[c->id][0], 4);
		dma350_cmdlink_set_srcmemattrlo(&cmdlink_cfg[c->id][0], 4);
		dma350_cmdlink_set_desmemattrhi(&cmdlink_cfg[c->id][0], 0);
		dma350_cmdlink_set_desmemattrlo(&cmdlink_cfg[c->id][0], 0);
		dma350_cmdlink_set_desmaxburstlen(&cmdlink_cfg[c->id][0], maxburst);
		dma350_cmdlink_enable_intr(&cmdlink_cfg[c->id][0], DMA350_CH_INTREN_DONE |
		DMA350_CH_INTREN_ERR | DMA350_CH_INTREN_DISABLED | DMA350_CH_INTREN_STOPPED);
		dma350_cmdlink_enable_destrigin(&cmdlink_cfg[c->id][0]);
		dma350_cmdlink_set_destriginmode(&cmdlink_cfg[c->id][0], DMA350_CH_DESTRIGINMODE_PERIPH_FLOW_CTRL);
		dma350_cmdlink_set_destrigintype(&cmdlink_cfg[c->id][0], DMA350_CH_DESTRIGINTYPE_HW);
		dma350_cmdlink_set_xaddrinc(&cmdlink_cfg[c->id][0], 1, 0);
		dma350_cmdlink_set_destriginsel(&cmdlink_cfg[c->id][0], c->req_line);
		break;
	case DMA_DEV_TO_MEM:
		c->dev_addr = cfg->src_addr;
		c->transize = cfg->src_addr_width;
		maxburst = cfg->src_maxburst - 1;
		maxburst = maxburst < MAX_BURST_LEN ? maxburst : MAX_BURST_LEN;
		dma350_ch_set_transize(p->base + DMA350_REG_CTRL, ffs(cfg->src_addr_width) - 1);

		 /*generate cmd link list*/
		dma350_cmdlink_set_srcaddr32(&cmdlink_cfg[c->id][0], c->dev_addr);
		dma350_cmdlink_set_transize(&cmdlink_cfg[c->id][0], ffs(cfg->src_addr_width) - 1);
		dma350_cmdlink_set_desmemattrhi(&cmdlink_cfg[c->id][0], 4);
		dma350_cmdlink_set_desmemattrlo(&cmdlink_cfg[c->id][0], 4);
		dma350_cmdlink_set_srcmemattrhi(&cmdlink_cfg[c->id][0], 0);
		dma350_cmdlink_set_srcmemattrlo(&cmdlink_cfg[c->id][0], 0);
		dma350_cmdlink_set_srcmaxburstlen(&cmdlink_cfg[c->id][0], maxburst);
		dma350_cmdlink_enable_intr(&cmdlink_cfg[c->id][0],
		DMA350_CH_INTREN_DONE|DMA350_CH_INTREN_ERR | DMA350_CH_INTREN_DISABLED | DMA350_CH_INTREN_STOPPED);
		dma350_cmdlink_enable_srctrigin(&cmdlink_cfg[c->id][0]);
		dma350_cmdlink_set_srctriginmode(&cmdlink_cfg[c->id][0], DMA350_CH_SRCTRIGINMODE_PERIPH_FLOW_CTRL);
		dma350_cmdlink_set_srctrigintype(&cmdlink_cfg[c->id][0], DMA350_CH_SRCTRIGINTYPE_HW);
		dma350_cmdlink_set_xaddrinc(&cmdlink_cfg[c->id][0], 0, 1);
		dma350_cmdlink_set_srctriginsel(&cmdlink_cfg[c->id][0], c->req_line);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct dma_async_tx_descriptor *arm_dma350_prep_memcpy(
	struct dma_chan *chan,	dma_addr_t dst, dma_addr_t src,
	size_t len, unsigned long flags)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_desc_sw *ds;
	struct arm_dma350_dev *dev = to_arm_dma350(c->vc.chan.device);
	struct arm_dma350_phy *p;
	struct dma_slave_config *cfg = &c->slave_cfg;
	size_t copy = 0;
	int num = 0, ret = 0;

	c->id = chan->chan_id;
	p = &dev->phy[c->id];

	if (!cfg->src_addr_width)
		cfg->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;

	if (!len)
		return NULL;

	ret = pm_runtime_resume_and_get(dev->slave.dev);
	if (ret < 0) {
		pr_err("%s,pm get err, %d\n",__func__,ret);
		return ERR_PTR(ret);
	}

	arm_dma350_nonsec_intren(dev);
	arm_dma350_init_state(p);
	c->dev_addr = cfg->src_addr;
	dma350_ch_set_transize(p->base + DMA350_REG_CTRL, ffs(cfg->src_addr_width) - 1);
	dma350_ch_src_memattr(p->base + DMA350_REG_SRCTRANSCFG, 4, 4);
	dma350_ch_des_memattr(p->base + DMA350_REG_DESTRANSCFG, 4, 4);
	dma350_ch_set_xaddr_inc(p->base + DMA350_REG_XADDRINC, 1, 1);
	num = DIV_ROUND_UP(len, DMA_MAX_SIZE);
	ds = arm_dma350_alloc_desc_resource(num, chan);
	if (!ds)
		return NULL;
	ds->size = len;
	num = 0;
	do {
		copy = min_t(size_t, len, DMA_MAX_SIZE);
		arm_dma350_fill_desc(ds, dst, src, copy / cfg->src_addr_width, num++);

		src += copy;
		dst += copy;
		len -= copy;
	} while (len);

	c->cyclic = 0;
	return vchan_tx_prep(&c->vc, &ds->vd, flags);
}

static struct dma_async_tx_descriptor *arm_dma350_prep_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl, unsigned int sglen,
	enum dma_transfer_direction dir, unsigned long flags, void *context)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_desc_sw *ds;
	size_t len, avail, total = 0;
	struct scatterlist *sg;
	dma_addr_t addr, src = 0, dst = 0;
	int num = sglen, i;

	if (!sgl)
		return NULL;
	if (arm_dma350_pre_config_sg(c, dir))
		return NULL;

	for_each_sg(sgl, sg, sglen, i) {
		avail = sg_dma_len(sg);

		if (avail > DMA_MAX_SIZE)
			num += DIV_ROUND_UP(avail, DMA_MAX_SIZE) - 1;
	}
	ds = arm_dma350_alloc_desc_resource(num, chan);
	if (!ds)
		return NULL;
	c->cyclic = 0;
	num = 0;
	for_each_sg(sgl, sg, sglen, i) {
		addr = sg_dma_address(sg);
		avail = sg_dma_len(sg);
		total += avail;
		do {

			len = min_t(size_t, avail, DMA_MAX_SIZE);

			if (dir == DMA_MEM_TO_DEV) {
				src = addr;
				dst = c->dev_addr;
			} else if (dir == DMA_DEV_TO_MEM) {
				src = c->dev_addr;
				dst = addr;
			}
			arm_dma350_fill_desc(ds, dst, src, (len / c->transize), num++);
			addr += len;
			avail -= len;
		} while (avail);
	}
	ds->size = total;
	return vchan_tx_prep(&c->vc, &ds->vd, flags);
}

static int arm_dma350_find_idle_channel(struct dma_chan *chan)
{
	u32 ch_idx = 0;
	u32 enable_status = 0;
	u32 cmdlink_status = 0;
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	struct arm_dma350_phy *phy = NULL;
	for(ch_idx = 0; ch_idx < MAX_CHAN_NUM; ch_idx++) {
		phy = &d->phy[ch_idx];
		/*cyclic mode use cmd link，so should check dma_enable and linkaddr_en*/
		enable_status = readl_relaxed(phy->base + DMA350_REG_CMD) & DMA350_CH_CMD_ENABLECMD;
		cmdlink_status = readl_relaxed(phy->base + DMA350_REG_LINKADDR) & DMA350_CH_LINKADDR_LINKADDREN;
		if((enable_status) || (cmdlink_status))
			continue;
		else {
			dev_info(d->slave.dev, "use dynamic channel-%d \n", ch_idx);
			c->id = ch_idx;
			break;
		}
	}
	if(c->id >= MAX_CHAN_NUM) {
		dev_err(d->slave.dev, "there is no idle channel to alloc\n");
		return -EINVAL;
	}

	return 0;
}

static struct dma_async_tx_descriptor *arm_dma350_prep_dma_cyclic(
		struct dma_chan *chan, dma_addr_t dma_addr, size_t buf_len,
		size_t period_len, enum dma_transfer_direction dir,
		unsigned long flags)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	struct arm_dma350_desc_sw *ds;
	dma_addr_t src = 0, dst = 0;
	int num_periods = buf_len / period_len;
	int buf = 0, cmd_num = 0;
	u32 offset = 0, bit = 0;
	int ret = 0;

	if (period_len > DMA_MAX_SIZE) {
		dev_err(chan->device->dev, "maximum period size exceeded\n");
		return NULL;
	}

        ret = pm_runtime_resume_and_get(d->slave.dev);
        if (ret < 0) {
                pr_err("%s,pm get err, %d\n",__func__,ret);
		return NULL;
        }
	/*if channel id is 0xFF,it means we should use dynamic channel*/
	if((DMA350_DYNAMIC_CH(c->id)) && (arm_dma350_find_idle_channel(chan))) {
		pm_runtime_put_noidle(d->slave.dev);
		return NULL;
	}
	if (arm_dma350_pre_config_cyclic(c, dir)) {
		pm_runtime_put_noidle(d->slave.dev);
		return NULL;
	}
	ds = arm_dma350_alloc_desc_resource(num_periods, chan);
	if (!ds) {
		pm_runtime_put_noidle(d->slave.dev);
		return NULL;
	}
	c->cyclic = 1;
	while (buf < buf_len) {
		if (cmd_num > 0)
			dma350_cmdlink_init(&cmdlink_cfg[c->id][cmd_num]);
		if (dir == DMA_MEM_TO_DEV) {
			src = dma_addr;

			if (d->map.ram_host_addr && d->map.ram_remote_addr) {
				offset = src - d->map.ram_host_addr;
				src = d->map.ram_remote_addr + offset;
			}

			dst = c->dev_addr;
			if (d->map.reg_host_addr && d->map.reg_remote_addr) {
				offset = dst - d->map.reg_host_addr;
				dst = d->map.reg_remote_addr + offset;
			}


		} else if (dir == DMA_DEV_TO_MEM) {
			src = c->dev_addr;
			if (d->map.reg_host_addr && d->map.reg_remote_addr) {
				offset = src - d->map.reg_host_addr;
				src = d->map.reg_remote_addr + offset;
			}

			dst = dma_addr;
			if (d->map.ram_host_addr && d->map.ram_remote_addr) {
				offset = dst - d->map.ram_host_addr;
				dst = d->map.ram_remote_addr + offset;
			}

		}
		dma_addr += period_len;
		buf += period_len;

		dma350_cmdlink_set_desaddr32(&cmdlink_cfg[c->id][cmd_num], dst);
		dma350_cmdlink_set_srcaddr32(&cmdlink_cfg[c->id][cmd_num], src);
		dma350_cmdlink_enable_linkaddr(&cmdlink_cfg[c->id][cmd_num]);
		dma350_cmdlink_set_xsize32(&cmdlink_cfg[c->id][cmd_num], (period_len / c->transize), (period_len / c->transize));
		cmd_num++;
	}
	cmd0[c->id] = dma_alloc_coherent(chan->device->dev, CMD_LINK_LEN * sizeof(u32), &phy_cmd0[c->id], GFP_ATOMIC);
	cmd_num = 0;
	while (cmd_num < num_periods) {

	/* cmd linklist prepare*/
		dma350_cmdlink_generate(&cmdlink_cfg[c->id][cmd_num], cmd0[c->id] + cmd_num+bit, &bit);

	/*
	*if not the last command,then fill the next command address to the linkaddr domain
	*if the last command, then fill the next command address with the first linkaddr
	*/

		if (cmd_num == (num_periods - 1))
			*(cmd0[c->id] + bit + cmd_num) = (u32)(phy_cmd0[c->id] + AUDSS_OFFSET) + DMA350_CH_LINKADDR_LINKADDREN;
		else
			*(cmd0[c->id] + bit + cmd_num) = (u32)(phy_cmd0[c->id] + (bit + cmd_num + 1) * 4 + AUDSS_OFFSET) + DMA350_CH_LINKADDR_LINKADDREN;
		cmd_num++;
	}
	dmachan[c->id] = num_periods;
	ds->size = buf_len;
	return vchan_tx_prep(&c->vc, &ds->vd, flags);
}

static int arm_dma350_config(struct dma_chan *chan,
			 struct dma_slave_config *cfg)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);

	if (!cfg)
		return -EINVAL;

	memcpy(&c->slave_cfg, cfg, sizeof(*cfg));

	return 0;
}

static int arm_dma350_terminate_all(struct dma_chan *chan)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	struct arm_dma350_phy *p = NULL;
	struct dma_slave_config *cfg = &c->slave_cfg;
	bool runtimepm_flag = false;

	unsigned long flags;
	LIST_HEAD(head);

	dev_dbg(d->slave.dev, "channel-%d terminate all\n", c->id);

	/* cannot find an idle channel when use channel dynamicly allocate */
	if(c->id >= d->max_channels) {
		dev_warn(d->slave.dev, "invaild c->id, c->id = %d \n", c->id);
		return 0;
	}

	p = &d->phy[c->id];

	/*if dma prepare not call (which means dma not ready to use), c->phy is null,
	then pm_runtime_resume_and_get should not call*/
	if(c->phy)
		runtimepm_flag = true;
	desnum[c->id] = 0;
	/* Prevent this channel being scheduled */
	spin_lock(&d->lock);
	list_del_init(&c->node);
	spin_unlock(&d->lock);

	/* Clear the tx descriptor lists */
	spin_lock_irqsave(&c->vc.lock, flags);
	vchan_get_all_descriptors(&c->vc, &head);
	if (p) {
		/* vchan is assigned to a pchan - stop the channel */
		arm_dma350_terminate_chan(p);
		dma350_ch_disable_linkaddr(p->base + DMA350_REG_LINKADDR);
		arm_dma350_clear_chan(p);
		if((cfg->direction == DMA_DEV_TO_MEM) && !c->cyclic && p->ds_run)
			vchan_vdesc_fini(&p->ds_run->vd);
		c->phy = NULL;
		p->vchan = NULL;
		p->ds_run = NULL;
		p->ds_done = NULL;
	}
	spin_unlock_irqrestore(&c->vc.lock, flags);
	if (c->cyclic)
		dma_free_coherent(chan->device->dev, CMD_LINK_LEN * sizeof(u32), cmd0[c->id], phy_cmd0[c->id]);
	vchan_dma_desc_free_list(&c->vc, &head);

	/*dynamic channel should mark the flag is_dynamic, at the same time,c->id should clear to 0xFF*/
	if((c->is_dynamic) && (c->cyclic))
		c->id = 0xFF;
	if (runtimepm_flag) {
		pm_runtime_mark_last_busy(d->slave.dev);
		pm_runtime_put_autosuspend(d->slave.dev);
	}

	return 0;
}

static int arm_dma350_transfer_pause(struct dma_chan *chan)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	struct arm_dma350_phy *p = &d->phy[c->id];

	dma350_ch_cmd(p->base + DMA350_REG_CMD, DMA350_CH_CMD_PAUSECMD);

	c->status = DMA_PAUSED;

	return 0;
}

static int arm_dma350_transfer_resume(struct dma_chan *chan)
{
	struct arm_dma350_chan *c = to_dma350_chan(chan);
	struct arm_dma350_dev *d = to_arm_dma350(chan->device);
	struct arm_dma350_phy *p = &d->phy[c->id];

	dma350_ch_cmd(p->base + DMA350_REG_CMD, DMA350_CH_CMD_RESUMECMD);

	c->status = DMA_IN_PROGRESS;

	return 0;
}

static void arm_dma350_free_desc(struct virt_dma_desc *vd)
{
	struct arm_dma350_desc_sw *ds =
	container_of(vd, struct arm_dma350_desc_sw, vd);
	struct arm_dma350_dev *d = to_arm_dma350(vd->tx.chan->device);

	if(ds != NULL) {
		dma_pool_free(d->pool, ds->desc_hw, ds->desc_hw_lli);
		kfree(ds);
	}
}

static const struct arm_dma350_drvdata arm_dma350_no_pause = {
	.is_exist_pause = false,
};


static const struct arm_dma350_drvdata arm_dma350_full = {
        .is_exist_pause = true,
};

static const struct of_device_id arm_dma350_dt_ids[] = {
	{ .compatible = "arm,dma350-no-pause", .data = &arm_dma350_no_pause },
	{ .compatible = "arm,dma350-full", .data = &arm_dma350_full},
	{}
};
MODULE_DEVICE_TABLE(of, arm_dma350_dt_ids);

static const struct acpi_device_id arm_dma350_acpi_ids[] = {
	{ "CIXH1006", .driver_data = (kernel_ulong_t)&arm_dma350_no_pause },
	{ "CIXHA014", .driver_data = (kernel_ulong_t)&arm_dma350_full },
	{ },
};
MODULE_DEVICE_TABLE(acpi, arm_dma350_acpi_ids);


static struct dma_chan *arm_dma350_of_xlate(struct of_phandle_args *dma_spec,
					       struct of_dma *ofdma)
{
	struct arm_dma350_dev *d = ofdma->of_dma_data;
	unsigned int channel_pri = 0;
	unsigned int channel_id = 0;
	unsigned int request = 0;
	struct dma_chan *chan;
	struct arm_dma350_chan *c;

	if (dma_spec->args_count == 1)
		request = dma_spec->args[0];
	else if(dma_spec->args_count == 2) {
		request = dma_spec->args[0];
		channel_id = dma_spec->args[1];
	} else if(dma_spec->args_count == 3) {
		request = dma_spec->args[0];
                channel_id = dma_spec->args[1];
		channel_pri = dma_spec->args[2];
	} else {
		dev_err(d->slave.dev, "para  invalid %s %d\n", __func__,__LINE__);
	}

	if((!DMA350_DYNAMIC_CH(channel_id)) && (channel_id >= d->max_channels)) {
			dev_err(d->slave.dev, "channel id invalid %s %d\n", __func__,__LINE__);
			return NULL;
	} 
	
	if (request >= d->max_requests) {
		dev_err(d->slave.dev, "request id invalid %s %d\n", __func__,__LINE__);
		return NULL;
	}
	chan = dma_get_any_slave_channel(&d->slave);
	if (!chan) {
		dev_err(d->slave.dev, "get channel fail in %s.\n", __func__);
		return NULL;
	}
	c = to_dma350_chan(chan);
	c->id = channel_id;
	c->req_line = request;
	c->priority = channel_pri;
	if (DMA350_DYNAMIC_CH(channel_id))
		c->is_dynamic = true;
	else
		c->is_dynamic = false;
	return chan;
}

static struct dma_chan *arm_dma350_acpi_xlate(struct acpi_dma_spec *dma_spec,
			struct acpi_dma *adma)
{
	struct arm_dma350_dev *d = adma->data;
	unsigned int request, channel_id;
	struct dma_chan *chan;
	struct arm_dma350_chan *c;

	request = dma_spec->slave_id;
	channel_id = dma_spec->chan_id;

	dev_dbg(d->slave.dev, "dma xlate request[%d] channel[%d]\n",
			request, channel_id);

	if (request >= d->max_requests) {
		dev_err(d->slave.dev, "request or channel_id overflow\n");
		return NULL;
	}
	chan = dma_get_any_slave_channel(&d->slave);
	if (!chan) {
		dev_err(d->slave.dev, "get channel fail in %s.\n", __func__);
		return NULL;
	}
	c = to_dma350_chan(chan);
	c->id = channel_id;
	c->req_line = request;
	return chan;
}

static int arm_dma350_probe(struct platform_device *op)
{
	struct arm_dma350_dev *d;
	struct arm_dma350_drvdata *drvdata = NULL;
	int i, ret = 0;
	u32 out_val[2];
	struct reset_control *dma_reset;

	d = devm_kzalloc(&op->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	drvdata = (struct arm_dma350_drvdata *)device_get_match_data(&op->dev);
	if (!drvdata) {
		dev_err(&op->dev, "unable to find driver data\n");
		return -EINVAL;
	}
	d->drvdata = drvdata;
	d->base = devm_platform_ioremap_resource(op, 0);
	if (IS_ERR(d->base))
		return PTR_ERR(d->base);

	device_property_read_u32(&op->dev, "dma-channels", &d->max_channels);
	device_property_read_u32(&op->dev, "dma-requests", &d->max_requests);
	if (!d->max_requests || !d->max_channels)
		return -EINVAL;
	if (!device_property_read_u32_array(&op->dev, "arm,reg-map",
		out_val, ARRAY_SIZE(out_val))) {
		d->map.reg_host_addr = out_val[0];
		d->map.reg_remote_addr = out_val[1];
	}
	if (!device_property_read_u32_array(&op->dev, "arm,ram-map",
		out_val, ARRAY_SIZE(out_val))) {
		d->map.ram_host_addr = out_val[0];
		d->map.ram_remote_addr = out_val[1];
	}

	if (device_property_present(&op->dev, "arm,clk-enable-atomic"))
		d->is_clk_enable_atomic = true;
	else
		d->is_clk_enable_atomic = false;

	d->regmap = device_syscon_regmap_lookup_by_property(&op->dev,
				"arm,remote-ctrl");
	if (PTR_ERR(d->regmap) == -ENODEV)
		d->regmap = NULL;
	else if (IS_ERR(d->regmap))
		return PTR_ERR(d->regmap);

	ret = dma_set_mask_and_coherent(&op->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&op->dev, "Failed to set DMA Mask\n");
		return ret;
	}
	if (!ACPI_COMPANION(&op->dev)) {
		ret = of_reserved_mem_device_init(&op->dev);
		if (ret && ret != -ENODEV) {
			dev_err(&op->dev, "Failed to init reserved mem for DMA\n");
			return ret;
		}
	}
	d->clk = devm_clk_get(&op->dev, NULL);
	if (IS_ERR(d->clk)) {
		dev_err(&op->dev, "no dma clk\n");
		return PTR_ERR(d->clk);
	}

	d->irq = platform_get_irq(op, 0);
	ret = devm_request_irq(&op->dev, d->irq, arm_dma350_int_handler,
			       0, DRIVER_NAME, d);
	if (ret)
		return ret;

	/* A DMA memory pool for LLIs, align on 32-byte boundary */
	d->pool = dmam_pool_create(DRIVER_NAME, &op->dev,
			LLI_BLOCK_SIZE, 32, 0);
	if (!d->pool) {
		dev_err(&op->dev, "alloc dma pool failed\n");
		return -ENOMEM;
	}

	/* init phy channel */
	d->phy = devm_kcalloc(&op->dev,
		d->max_channels, sizeof(struct arm_dma350_phy), GFP_KERNEL);
	if (!d->phy) {
		dev_err(&op->dev, "alloc phy chans failed\n");
		return -ENOMEM;
	}

	/* Enable clock before accessing registers */
	ret = clk_prepare_enable(d->clk);
	if (ret < 0) {
		dev_err(&op->dev, "clk_prepare_enable failed: %d\n", ret);
		return ret;
	}
	dma_reset = devm_reset_control_get(&op->dev, "dma_reset");
	if (IS_ERR(dma_reset)) {
		dev_err(&op->dev, "get dma reset error\n");
		ret = PTR_ERR(dma_reset);
		if (ret != -ENOENT)
			goto clk_dis;
	} else {
		/* reset */
		reset_control_assert(dma_reset);
		/* release reset */
		reset_control_deassert(dma_reset);
	}
	d->dma_reset = dma_reset;

	for (i = 0; i < d->max_channels; i++) {
		struct arm_dma350_phy *p = &d->phy[i];

		p->idx = i;
		p->base = d->base + CHANNEL_IDX(i);
		desnum[i] = 0;
	}

	INIT_LIST_HEAD(&d->slave.channels);
	dma_cap_set(DMA_SLAVE, d->slave.cap_mask);
	dma_cap_set(DMA_MEMCPY, d->slave.cap_mask);
	dma_cap_set(DMA_CYCLIC, d->slave.cap_mask);
	dma_cap_set(DMA_PRIVATE, d->slave.cap_mask);
	d->slave.dev = &op->dev;
	d->slave.device_free_chan_resources = arm_dma350_free_chan_resources;
	d->slave.device_tx_status = arm_dma350_tx_status;
	d->slave.device_prep_dma_memcpy = arm_dma350_prep_memcpy;
	d->slave.device_prep_slave_sg = arm_dma350_prep_slave_sg;
	d->slave.device_prep_dma_cyclic = arm_dma350_prep_dma_cyclic;
	d->slave.device_issue_pending = arm_dma350_issue_pending;
	d->slave.device_config = arm_dma350_config;
	d->slave.device_terminate_all = arm_dma350_terminate_all;
	if(d->drvdata->is_exist_pause == true) {
		d->slave.device_pause = arm_dma350_transfer_pause;
		d->slave.device_resume = arm_dma350_transfer_resume;
	}
	d->slave.copy_align = DMA_ALIGN;
	d->slave.src_addr_widths = ARM_DMA350_BUSWIDTHS;
	d->slave.dst_addr_widths = ARM_DMA350_BUSWIDTHS;
	d->slave.directions = BIT(DMA_MEM_TO_MEM) | BIT(DMA_MEM_TO_DEV)
			| BIT(DMA_DEV_TO_MEM);
	d->slave.residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;
	/* init virtual channel */
	d->chans = devm_kcalloc(&op->dev,
		d->max_requests, sizeof(struct arm_dma350_chan), GFP_KERNEL);
	if (!d->chans) {
		dev_err(&op->dev, "alloc chans failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < d->max_requests; i++) {
		struct arm_dma350_chan *c = &d->chans[i];

		c->status = DMA_IN_PROGRESS;
		INIT_LIST_HEAD(&c->node);
		c->vc.desc_free = arm_dma350_free_desc;
		vchan_init(&c->vc, &d->slave);
	}

	spin_lock_init(&d->lock);
	INIT_LIST_HEAD(&d->chan_pending);
	platform_set_drvdata(op, d);
	ret = dma_async_device_register(&d->slave);
	if (ret)
		goto clk_dis;

	pm_runtime_irq_safe(&op->dev);
	pm_runtime_set_autosuspend_delay(&op->dev, ARM_DMA350__PM_TIMEOUT_MS);
	pm_runtime_use_autosuspend(&op->dev);
	pm_runtime_get_noresume(&op->dev);
	pm_runtime_set_active(&op->dev);
	pm_runtime_enable(&op->dev);

	if (ACPI_COMPANION(&op->dev))
		ret = acpi_dma_controller_register(&op->dev,
					arm_dma350_acpi_xlate, d);
	else
		ret = of_dma_controller_register((&op->dev)->of_node,
					arm_dma350_of_xlate, d);
	if (ret)
		goto of_dma_register_fail;

	pm_runtime_mark_last_busy(&op->dev);
	pm_runtime_put_autosuspend(&op->dev);
	dev_info(&op->dev, "initialized\n");

	return 0;

of_dma_register_fail:
	dma_async_device_unregister(&d->slave);
clk_dis:
	clk_disable_unprepare(d->clk);
	return ret;
}

static int arm_dma350_remove(struct platform_device *op)
{
	struct arm_dma350_chan *c, *cn;
	struct arm_dma350_dev *d = platform_get_drvdata(op);

	/* explicitly free the irq */
	devm_free_irq(&op->dev, d->irq, d);

	dma_async_device_unregister(&d->slave);
	of_dma_controller_free((&op->dev)->of_node);

	list_for_each_entry_safe(c, cn, &d->slave.channels,
				 vc.chan.device_node) {
		list_del(&c->vc.chan.device_node);
	}
	clk_disable_unprepare(d->clk);

	return 0;
}


static int arm_dma350_runtime_suspend_dev(struct device *dev)
{
	struct arm_dma350_dev *d = dev_get_drvdata(dev);

	if (d->is_clk_enable_atomic)
		clk_disable(d->clk);
	else
		clk_disable_unprepare(d->clk);

        return 0;
}

static int arm_dma350_runtime_resume_dev(struct device *dev)
{
        struct arm_dma350_dev *d = dev_get_drvdata(dev);
        int ret = 0;

	if (d->is_clk_enable_atomic)
		ret = clk_enable(d->clk);
	else
		ret = clk_prepare_enable(d->clk);
        if (ret < 0) {
                dev_err(d->slave.dev, "clk_prepare_enable failed: %d\n", ret);
                return ret;
	}

	if(d->drvdata->is_exist_pause == false) {
		reset_control_assert(d->dma_reset);
		udelay(2);
		reset_control_deassert(d->dma_reset);
	}

        return 0;
}

#ifdef CONFIG_PM_SLEEP
static int arm_dma350_suspend_dev(struct device *dev)
{
	int ret = 0;

	ret = pm_runtime_force_suspend(dev);
	if (ret) {
		dev_err(dev, "Force suspend error.\n");
		return ret;
	}

	return 0;
}

static int arm_dma350_resume_dev(struct device *dev)
{
	int ret = 0;
	struct arm_dma350_dev *d = dev_get_drvdata(dev);

	if(d->drvdata->is_exist_pause == true) {
		reset_control_assert(d->dma_reset);
		udelay(2);
		reset_control_deassert(d->dma_reset);
	}

	ret = pm_runtime_force_resume(dev);
	if (ret) {
		dev_err(dev, "Force resume error.\n");
		return ret;
	}

	return 0;
}
#endif

static const struct dev_pm_ops arm_dma350_pmops = {
	SET_SYSTEM_SLEEP_PM_OPS(arm_dma350_suspend_dev,
				      arm_dma350_resume_dev)
	SET_RUNTIME_PM_OPS(arm_dma350_runtime_suspend_dev,
			   arm_dma350_runtime_resume_dev, NULL)
};

static struct platform_driver arm_dma350_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.pm	= &arm_dma350_pmops,
		.of_match_table = arm_dma350_dt_ids,
		.acpi_match_table = ACPI_PTR(arm_dma350_acpi_ids),
	},
	.probe		= arm_dma350_probe,
	.remove		= arm_dma350_remove,
};

module_platform_driver(arm_dma350_driver);

MODULE_AUTHOR("Hongliang yang <hongliang.yang@cixtech.com>");
MODULE_DESCRIPTION("API Driver for ARM DMA350 controller");
MODULE_LICENSE("GPL v2");

