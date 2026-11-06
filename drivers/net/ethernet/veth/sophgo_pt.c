/*
 * PT is short for package transmitter
 */
#include <linux/io.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "sophgo_pt.h"
#include "sophgo_cdma.h"

struct veth_dev;
u32 sg_eth_cdma_transfer(struct veth_dev *vdev, struct sg_cdma_arg *parg, bool lock_cdma)
{
	u32 src_addr_hi = 0;
	u32 src_addr_lo = 0;
	u32 dst_addr_hi = 0;
	u32 dst_addr_lo = 0;
	u32 reg_val = 0;
	u64 src = parg->src;
	u64 dst = parg->dst;
	u32 count = 3000;
	u32 int_mask_val;
	unsigned long flag;

	if (lock_cdma)
		spin_lock_irqsave(&vdev->lock_cdma, flag);

	/*Disable CDMA*/
	reg_val = sg_read32(vdev->cdma_cfg_reg, CDMA_CHL_CTRL);
	reg_val &= ~(CDMA_CHL_CTRL_DMA_EN | CDMA_CHL_CTRL_IRQ_EN);
	sg_write32(vdev->cdma_cfg_reg, CDMA_CHL_CTRL, reg_val);
	reg_val |= CDMA_CHL_CTRL_DMA_EN;
	sg_write32(vdev->cdma_cfg_reg, CDMA_CHL_CTRL, reg_val);

	int_mask_val = sg_read32(vdev->cdma_cfg_reg, CDMA_INT_MASK);
	int_mask_val &= ~(CDMA_INT_RF_DES_MASK);
	int_mask_val |= CDMA_INT_RF_CMD_EPT_MASK;
	sg_write32(vdev->cdma_cfg_reg, CDMA_INT_MASK, int_mask_val);

	/*set max payload which equal to PCIE bandwidth.*/
	sg_write32(vdev->cdma_cfg_reg, CDMA_MAX_PAYLOAD, 0x9);

	if (parg->dir == CHIP2HOST) {
		dma_sync_single_range_for_device(&vdev->pdev->dev, src, 0, parg->size, DMA_TO_DEVICE);
		dst |= (u64)0x800 << 32;
	} else if (parg->dir == HOST2CHIP) {
		dma_sync_single_range_for_cpu(&vdev->pdev->dev, dst, 0, parg->size, DMA_FROM_DEVICE);
		src |= (u64)0x800 << 32;
	}

	src_addr_hi = src >> 32;
	src_addr_lo = src & 0xffffffff;
	dst_addr_hi = dst >> 32;
	dst_addr_lo = dst & 0xffffffff;
	/* pr_info("src:0x%llx dst:0x%llx size:%lld\n", src, dst, parg->size);*/
	/* pr_info("src_addr_hi 0x%x src_addr_low 0x%x\n", src_addr_hi, src_addr_lo);*/
	/* pr_info("dst_addr_hi 0x%x dst_addr_low 0x%x\n", dst_addr_hi, dst_addr_lo);*/

	sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP3, src_addr_lo);
	sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP4, src_addr_hi);
	sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP5, dst_addr_lo);
	sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP6, dst_addr_hi);
	sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP7, parg->size);
	///sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP2, 0);
	//sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP1, 1);
	sg_write32(vdev->cdma_cfg_reg, CDMA_CMD_ACCP0, CDMA_CMD_NORMAL_CFG);

	while ((sg_read32(vdev->cdma_cfg_reg, CDMA_INT_STAT) & 0x1) != 0x1) {
		udelay(1);
		if (--count == 0) {
			pr_err("veth cdma polling wait timeout\n");
			if (lock_cdma)
				spin_unlock_irqrestore(&vdev->lock_cdma, flag);
			return -EBUSY;
		}
	}
	reg_val = sg_read32(vdev->cdma_cfg_reg, CDMA_INT_STAT);
	reg_val |= (1 << 0x0);
	sg_write32(vdev->cdma_cfg_reg, CDMA_INT_STAT, reg_val);

	if (lock_cdma)
		spin_unlock_irqrestore(&vdev->lock_cdma, flag);
	return 0;
}

int sg_eth_memcpy_s2d(struct veth_dev *vdev, u64 dst, const void *src, u32 size)
{
	struct sg_cdma_arg cdma_arg;

	sg_construct_cdma_arg(&cdma_arg, (u64)src, dst, size, CHIP2HOST);
	if (sg_eth_cdma_transfer(vdev, &cdma_arg, true))
		return -EBUSY;

	return 0;
}

int sg_eth_memcpy_d2s(struct veth_dev *vdev, void *dst, u64 src, u32 size)
{
	struct sg_cdma_arg cdma_arg;

	sg_construct_cdma_arg(&cdma_arg, src, (u64)dst, size, HOST2CHIP);
	if (sg_eth_cdma_transfer(vdev, &cdma_arg, true))
		return -EBUSY;

	return 0;
}

static void host_queue_init(struct host_queue *q, void __iomem *shm)
{
	q->phy = (u64 *)(shm + SHM_HOST_PHY_OFFSET);
	q->len = (u64 *)(shm + SHM_HOST_LEN_OFFSET);
	q->head = (u32 *)(shm + SHM_HOST_HEAD_OFFSET);
	q->tail = (u32 *)(shm + SHM_HOST_TAIL_OFFSET);
	q->vphy = ioread64(q->phy);
}

static inline void host_queue_set_phy(struct host_queue *q, u64 phy)
{
	iowrite64(phy, q->phy);
}

static inline u64 host_queue_get_phy(struct host_queue *q)
{
	return ioread64(q->phy);
}

static inline void host_queue_set_len(struct host_queue *q, u64 len)
{
	iowrite64(len, q->len);
}

static inline u64 host_queue_get_len(struct host_queue *q)
{
	return ioread64(q->len);
}

static inline void host_queue_set_head(struct host_queue *q, u64 head)
{
	iowrite32(head, q->head);
}

static inline u32 host_queue_get_head(struct host_queue *q)
{
	return ioread32(q->head);
}

static inline void host_queue_set_tail(struct host_queue *q, u32 tail)
{
	iowrite32(tail, q->tail);
}

static inline u32 host_queue_get_tail(struct host_queue *q)
{
	return ioread32(q->tail);
}

static void *pt_dma_alloc(struct device *dev, size_t len, dma_addr_t *phy)
{
	void *virt;

	virt = kmalloc(len, GFP_KERNEL);
	if (virt)
		*phy = dma_map_single(dev, virt, len, DMA_BIDIRECTIONAL);

	return virt;
}

static void pt_dma_free(struct device *dev, size_t len,
			void *virt, dma_addr_t phy)
{
	dma_unmap_single(dev, phy, len, DMA_BIDIRECTIONAL);
	kfree(virt);
}

/* bytes aligned */
struct pt *pt_init(struct device *dev, void __iomem *shm, struct veth_dev *vdev)
{
	struct pt *pt;
	struct local_queue *lq;
	void *handle = devm_kzalloc(dev, sizeof(struct pt), GFP_KERNEL);

	if (!handle)
		return NULL;

	pt = handle;

	pt->dev = dev;
	/* setup host buffer */
	pt->shm = shm;
	pt->vdev = vdev;

	host_queue_init(&pt->tx.hq, shm + SHM_SOC_TX_OFFSET);
	host_queue_init(&pt->rx.hq, shm + SHM_SOC_RX_OFFSET);
	pt->tx.lq.len = host_queue_get_len(&pt->tx.hq);
	pt->rx.lq.len = host_queue_get_len(&pt->rx.hq);

	lq = &pt->tx.lq;
	lq->cpu = pt_dma_alloc(dev, lq->len, &lq->phy);
	if (!lq->cpu) {
		pr_err("allocate tx buffer with size %u failed\n", lq->len);
		goto free_handle;
	}

	lq = &pt->rx.lq;
	lq->cpu = pt_dma_alloc(dev, lq->len, &lq->phy);
	if (!lq->cpu) {
		pr_err("allocate rx buffer with size %u failed\n", lq->len);
		goto free_tx;
	}

	pt_info(pt);

	return handle;

free_tx:
	pt_dma_free(dev, pt->tx.lq.len, pt->tx.lq.cpu, pt->tx.lq.phy);
free_handle:
	devm_kfree(dev, handle);

	return NULL;
}

void pt_uninit(struct pt *pt)
{
	struct device *dev = pt->dev;

	pt_dma_free(dev, pt->rx.lq.len, pt->rx.lq.cpu, pt->rx.lq.phy);
	pt_dma_free(dev, pt->tx.lq.len, pt->tx.lq.cpu, pt->tx.lq.phy);
	devm_kfree(dev, pt);
}

static unsigned int local_queue_used(struct local_queue *q)
{
	unsigned int used;

	if (q->head >= q->tail)
		used = q->head - q->tail;
	else
		used = q->len - q->tail + q->head;

	return used;
}

static unsigned int local_queue_free(struct local_queue *q)
{
	return q->len - local_queue_used(q) - 1;
}

static unsigned int __enqueue(void *queue, unsigned int qlen, unsigned int head, void *data, int len)
{
	unsigned int tmp;

	tmp = min_t(unsigned int, len, qlen - head);
	memcpy((u8 *)queue + head, data, tmp);
	memcpy((u8 *)queue, (u8 *)data + tmp, len - tmp);

	head = (head + len) % qlen;

	return head;
}

static unsigned int __dequeue(void *queue, unsigned int qlen, unsigned int tail, void *data, int len)
{
	unsigned int tmp;

	tmp = min_t(unsigned int, len, qlen - tail);
	memcpy(data, (u8 *)queue + tail, tmp);
	memcpy((u8 *)data + tmp, queue, len - tmp);

	tail = (tail + len) % qlen;

	return tail;
}

int pt_send(struct pt *pt, void *data, int len)
{
	unsigned int head;
	struct local_queue *q;
	struct host_queue *hq;
	u32 pkg_len;
	unsigned int total_len = round_up(len + sizeof(pkg_len), PT_ALIGN);
	//int val;

	q = &pt->tx.lq;
	hq = &pt->tx.hq;
	if (local_queue_free(q) < total_len) {
		// pr_info("local_queue_free(q): 0x%x, total_len: 0x%x\n", local_queue_free(q), total_len);

		// pr_info("q->head: 0x%x\n", q->head);
		// pr_info("q->tail: 0x%x\n", q->tail);
		// val = host_queue_get_head(hq);
		// pr_info("host head: 0x%x\n", val);
		// val = host_queue_get_tail(hq);
		// pr_info("host tail: 0x%x\n", val);
		return -ENOMEM;
	}

	pkg_len = cpu_to_le32(len);

	/* load queue head */
	head = q->head;

	head = __enqueue(q->cpu, q->len, head, &pkg_len, sizeof(pkg_len));
	__enqueue(q->cpu, q->len, head, data, len);

	/* store queue head */
	q->head = (q->head + total_len) % q->len;

	return len;
}

int pt_pkg_len(struct pt *pt)
{
	struct local_queue *q = &pt->rx.lq;
	u32 pkg_len;

	if (q->head == q->tail)
		return -ENOMEM;

	pkg_len = ioread32(q->cpu + q->tail);
	// pkg_len = (u32 *)((u8 *)q->cpu + q->tail);
	// WARN((unsigned long)pkg_len & (sizeof(pkg_len) - 1), "rx queue not aligned correctly\n");

	WARN(((pkg_len < 0) || (pkg_len > 0x1000d)), "pkg_len: %d error\n", pkg_len);
	return pkg_len;
}

int pt_recv(struct pt *pt, void *data, int len)
{
	unsigned int tail;
	unsigned int total_len = round_up(len + sizeof(u32), PT_ALIGN);
	struct local_queue *q = &pt->rx.lq;

	if (local_queue_used(q) < total_len)
		return -ENOMEM;

	tail = q->tail;
	/* skip 4 bytes header(package length) */
	tail = (tail + sizeof(u32)) % q->len;
	__dequeue(q->cpu, q->len, tail, data, len);

	q->tail = (q->tail + total_len) % q->len;

	return len;
}

void pt_load_tx(struct pt *pt)
{
	/* sync read pointer */
	pt->tx.lq.tail = host_queue_get_tail(&pt->tx.hq);
}

int pt_store_tx(struct pt *pt)
{
	int ret;
	u64 host_phy;
	u32 host_head;
	struct local_queue *lq;
	struct host_queue *hq;

	lq = &pt->tx.lq;
	hq = &pt->tx.hq;
	host_phy = hq->vphy;
	host_head = host_queue_get_head(hq);

	if (lq->head > host_head) {
		ret = sg_eth_memcpy_s2d(pt->vdev, host_phy + host_head, (const void *)(lq->phy + host_head),
					lq->head - host_head);
	} else {
		ret = sg_eth_memcpy_s2d(pt->vdev, host_phy + host_head, (const void *)(lq->phy + host_head),
					lq->len - host_head);
		ret |= sg_eth_memcpy_s2d(pt->vdev, host_phy, (const void *)lq->phy, lq->head);
	}

	if (ret == 0)
		host_queue_set_head(hq, lq->head);
	else
		pr_info("s2d exec failed! ret: %d\n", ret);

	return ret;
}

bool pt_load_rx(struct pt *pt)
{
	int ret;
	u64 host_phy;
	u32 host_head;
	struct local_queue *lq;
	struct host_queue *hq;

	lq = &pt->rx.lq;
	hq = &pt->rx.hq;

	host_phy = hq->vphy;

	host_head = host_queue_get_head(hq);
	lq->head = host_head;

	if (host_head == lq->tail)
		return false;

	if (host_head > lq->tail) {
		ret = sg_eth_memcpy_d2s(pt->vdev, (void *)(lq->phy + lq->tail), host_phy + lq->tail,
					host_head - lq->tail);
	} else {
		ret = sg_eth_memcpy_d2s(pt->vdev, (void *)(lq->phy + lq->tail), host_phy + lq->tail,
					lq->len - lq->tail);
		ret |= sg_eth_memcpy_d2s(pt->vdev, (void *)lq->phy, host_phy, host_head);
	}

	if (ret != 0) {
		pr_info("d2s exec failed! ret: %d\n", ret);
		return false;
	}

	return true;
}

void pt_store_rx(struct pt *pt)
{
	host_queue_set_tail(&pt->rx.hq, pt->rx.lq.tail);
}

static void local_queue_info(struct device *dev, struct local_queue *q)
{
	dev_info(dev, "local address %p, local phy 0x%llx, local length %u, head %u, tail %u\n",
		 q->cpu, q->phy, q->len, q->head, q->tail);
}

static void host_queue_info(struct device *dev, struct host_queue *q)
{
	dev_info(dev, "host phy 0x%016llx, length %llu, head %u, tail %u\n",
		 host_queue_get_phy(q), host_queue_get_len(q),
		 host_queue_get_head(q), host_queue_get_tail(q));
}

static void queue_info(struct device *dev, struct pt_queue *q)
{
	local_queue_info(dev, &q->lq);
	host_queue_info(dev, &q->hq);
}

void pt_info(struct pt *pt)
{
	dev_info(pt->dev, "------------- TX -------------\n");
	queue_info(pt->dev, &pt->tx);
	dev_info(pt->dev, "------------- RX -------------\n");
	queue_info(pt->dev, &pt->rx);
}
