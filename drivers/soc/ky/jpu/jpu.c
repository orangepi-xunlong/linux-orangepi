// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/pm.h>
//#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/pm_runtime.h>
#include <asm/io.h>
#include <linux/pm_qos.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include "jpuconfig.h"
#include "regdefine.h"
#include "jpu.h"
#include "jpu_export.h"

#define JPU_TBU_BASE_VA (0x80000000)
#define JPU_TBU_VA_STEP (0x2000000)
#define DDR_QOS_ENABLE
#ifdef ENABLE_DEBUG_MSG
#define JLOG(dev, fmt, args...) dev_info(dev, "JPU: " fmt,  ## args);
#else
#define JLOG(args...)
#endif
jpu_dma_buf_info buf_inf[2];
#if defined (CONFIG_PM) && defined (DDR_QOS_ENABLE)
//static struct freq_qos_request jpu_ddrfreq_qos_rreq_sum;
//static struct freq_qos_request jpu_ddrfreq_qos_wreq_sum;
#endif
typedef struct jpu_drv_context_t {
	struct fasync_struct *async_queue;
	u32 open_count;
	u32 interrupt_reason[MAX_NUM_INSTANCE];
} jpu_drv_context_t;

enum ky_iommu_type {
	TBU_INPUT = 0,
	TBU_OUTPUT = 1,
};

typedef struct jpudrv_buffer_pool_t {
	struct list_head list;
	struct jpudrv_buffer_t jb;
	struct file *filp;
} jpudrv_buffer_pool_t;

typedef struct jpudrv_instance_list_t {
	struct list_head list;
	unsigned long inst_idx;
	struct file *filp;
} jpudrv_instance_list_t;

typedef struct jpudrv_instance_pool_t {
	unsigned char codecInstPool[MAX_NUM_INSTANCE][MAX_INST_HANDLE_SIZE];
} jpudrv_instance_pool_t;

struct jpu_device {
	struct device *jdev;

	struct device *jpu_device;
	struct cdev s_jpu_cdev;
	struct class *jpu_class;
	dev_t s_jpu_major;

	jpudrv_buffer_t s_instance_pool;
	jpu_drv_context_t s_jpu_drv_context;

	int s_jpu_open_ref_count;
	int s_jpu_irq;

	struct clk *cclk;
	atomic_t cclk_enable_count;
	int32_t cclk_max_frequency;
	int32_t cclk_min_frequency;
	int32_t cclk_cur_frequency;
	int32_t cclk_default_frequency;
	struct clk *aclk;
	atomic_t aclk_enable_count;
	int64_t aclk_max_frequency;
	int32_t aclk_min_frequency;
	int32_t aclk_cur_frequency;
	int32_t aclk_default_frequency;

	struct clk *iclk;
	atomic_t iclk_enable_count;
	int64_t iclk_max_frequency;
	int32_t iclk_min_frequency;
	int32_t iclk_cur_frequency;
	int32_t iclk_default_frequency;

	struct reset_control *jpg_reset;
	atomic_t jpg_reset_enable_count;

	struct reset_control *lcd_mclk_reset;
	atomic_t lcd_mclk_reset_enable_count;

	struct reset_control *isp_ci_reset;
	atomic_t isp_ci_reset_enable_count;

	struct reset_control *freset;
	atomic_t freset_enable_count;

	struct reset_control *sreset;
	atomic_t sreset_enable_count;

	jpudrv_buffer_t s_jpu_register;
	void __iomem *reg;

	int s_interrupt_flag[MAX_NUM_INSTANCE];
	wait_queue_head_t s_interrupt_wait_q[MAX_NUM_INSTANCE];

	spinlock_t s_jpu_lock;
	struct semaphore s_jpu_sem;
	struct list_head s_jbp_head;
	struct list_head s_inst_list_head;
	u32 time_out_cycs;
	u32 page_size;
	u64 va_base;
	u64 va_end;
	struct tbu_instance tbu_ins[TBU_INSTANCES_NUM];
	unsigned long tbu_ins_bitmap;
	spinlock_t tbu_ins_bitmap_lock;
	int tbu_ins_map;
	bool is_hw_enable;
	spinlock_t hw_access_lock;
	struct semaphore tbu_ins_free_cnt;
#ifdef CONFIG_PM
#ifdef DDR_QOS_ENABLE
	struct freq_constraints *ddr_qos_cons;
	struct freq_qos_request *ddr_qos_rreq;
	struct freq_qos_request *ddr_qos_wreq;
#endif
#endif
};

static void jpu_writel(struct jpu_device *dev, int offset, u32 val)
{
	writel(val, dev->reg + offset);
}

static u32 jpu_readl(struct jpu_device *dev, int offset)
{
	return readl(dev->reg + offset);
}

static void jpu_set_reg_bits(struct jpu_device *dev, u64 offset, u32 bits)
{
	jpu_writel(dev, offset, (jpu_readl(dev, offset) | bits));
}

static void jpu_clear_reg_bits(struct jpu_device *dev, u64 offset, u32 bits)
{
	jpu_writel(dev, offset, (jpu_readl(dev, offset) & ~bits));
}

static int jpu_jpg_reset_deassert(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->freset)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->jpg_reset_enable_count);
	JLOG(jdev->jdev, "deassert jpg_reset\n");
	return reset_control_deassert(jdev->jpg_reset);
}

static int jpu_lcd_mclk_reset_deassert(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->lcd_mclk_reset)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->lcd_mclk_reset_enable_count);
	JLOG(jdev->jdev, "deassert lcd_mclk_reset\n");
	return reset_control_deassert(jdev->lcd_mclk_reset);
}

static int jpu_isp_ci_reset_deassert(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->isp_ci_reset)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->isp_ci_reset_enable_count);
	JLOG(jdev->jdev, "deassert isp_ci_reset\n");
	return reset_control_deassert(jdev->isp_ci_reset);
}

static int jpu_freset_deassert(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->freset)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->freset_enable_count);
	JLOG(jdev->jdev, "deassert freset\n");
	return reset_control_deassert(jdev->freset);
}

static int jpu_sreset_deassert(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->sreset)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->sreset_enable_count);
	JLOG(jdev->jdev, "deassert sreset\n");
	return reset_control_deassert(jdev->sreset);
}

static void jpu_jpg_reset_assert(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->jpg_reset)
	    && atomic_read(&jdev->jpg_reset_enable_count) >= 1) {
		JLOG(jdev->jdev, "assert jpg_reset\n");
		atomic_dec(&jdev->jpg_reset_enable_count);
		reset_control_assert(jdev->jpg_reset);
	}
}

static void jpu_lcd_mclk_reset_assert(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->lcd_mclk_reset)
	    && atomic_read(&jdev->lcd_mclk_reset_enable_count) >= 1) {
		JLOG(jdev->jdev, "assert lcd_mclk_reset\n");
		atomic_dec(&jdev->lcd_mclk_reset_enable_count);
		reset_control_assert(jdev->lcd_mclk_reset);
	}
}

static void jpu_isp_ci_reset_assert(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->isp_ci_reset)
	    && atomic_read(&jdev->isp_ci_reset_enable_count) >= 1) {
		JLOG(jdev->jdev, "assert isp_ci_reset\n");
		atomic_dec(&jdev->isp_ci_reset_enable_count);
		reset_control_assert(jdev->isp_ci_reset);
	}
}

static void jpu_freset_assert(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->freset)
	    && atomic_read(&jdev->freset_enable_count) >= 1) {
		JLOG(jdev->jdev, "assert freset\n");
		atomic_dec(&jdev->freset_enable_count);
		reset_control_assert(jdev->freset);
	}
}

static void jpu_sreset_assert(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->sreset)
	    && atomic_read(&jdev->sreset_enable_count) >= 1) {
		JLOG(jdev->jdev, "assert sreset\n");
		atomic_dec(&jdev->sreset_enable_count);
		reset_control_assert(jdev->sreset);
	}
}


#ifndef CONFIG_SOC_KY_X1_FPGA
static int jpu_aclk_enable(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->aclk)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->aclk_enable_count);
	JLOG(jdev->jdev, "enable aclk\n");
	return clk_prepare_enable(jdev->aclk);
}

static int jpu_iclk_enable(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->iclk)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->iclk_enable_count);
	JLOG(jdev->jdev, "enable iclk\n");
	return clk_prepare_enable(jdev->iclk);
}

static int jpu_cclk_enable(struct jpu_device *jdev)
{
	if (IS_ERR_OR_NULL(jdev->cclk)) {
		return -EINVAL;
	}
	atomic_inc(&jdev->cclk_enable_count);
	JLOG(jdev->jdev, "enable cclk\n");
	return clk_prepare_enable(jdev->cclk);
}

static void jpu_aclk_disable(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->aclk) && __clk_is_enabled(jdev->aclk)
	    && atomic_read(&jdev->aclk_enable_count) >= 1) {
		JLOG(jdev->jdev, "disable aclk\n");
		atomic_dec(&jdev->aclk_enable_count);
		clk_disable_unprepare(jdev->aclk);
	}
}

static void jpu_cclk_disable(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->cclk) && __clk_is_enabled(jdev->cclk)
	    && atomic_read(&jdev->cclk_enable_count) >= 1) {
		JLOG(jdev->jdev, "disable cclk\n");
		atomic_dec(&jdev->cclk_enable_count);
		clk_disable_unprepare(jdev->cclk);
	}
}

static void jpu_iclk_disable(struct jpu_device *jdev)
{
	if (!IS_ERR_OR_NULL(jdev->iclk) && __clk_is_enabled(jdev->iclk)
	    && atomic_read(&jdev->iclk_enable_count) >= 1) {
		JLOG(jdev->jdev, "disable iclk\n");
		atomic_dec(&jdev->iclk_enable_count);
		clk_disable_unprepare(jdev->iclk);
	}
}

static int jpu_clk_enable(struct jpu_device *jdev)
{
	int ret;
#if 0
	ret = clk_set_rate(jdev->aclk, jdev->aclk_cur_frequency);
	if (ret) {
		return ret;
	}
#endif

#if 0
	ret = clk_set_rate(jdev->cclk, jdev->cclk_cur_frequency);
	if (ret) {
		return ret;
	}
#endif

	ret = jpu_cclk_enable(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_aclk_enable(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_iclk_enable(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_jpg_reset_deassert(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_lcd_mclk_reset_deassert(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_isp_ci_reset_deassert(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_freset_deassert(jdev);
	if (ret) {
		return ret;
	}
	ret = jpu_sreset_deassert(jdev);
	if (ret) {
		return ret;
	}

	return ret;
}

static void jpu_clk_disable(struct jpu_device *jdev)
{
	jpu_cclk_disable(jdev);
	jpu_aclk_disable(jdev);
	jpu_iclk_disable(jdev);
	jpu_jpg_reset_assert(jdev);
	jpu_lcd_mclk_reset_assert(jdev);
	jpu_isp_ci_reset_assert(jdev);
	jpu_freset_assert(jdev);
	jpu_sreset_assert(jdev);
}

static int jpu_hw_reset(struct jpu_device *jdev)
{
	JLOG(jdev->jdev, "request jpu reset from application.\n");

	jpu_cclk_disable(jdev);
	jpu_aclk_disable(jdev);
	jpu_iclk_disable(jdev);
	jpu_jpg_reset_assert(jdev);
	jpu_lcd_mclk_reset_assert(jdev);
	jpu_isp_ci_reset_assert(jdev);
	jpu_freset_assert(jdev);
	jpu_sreset_assert(jdev);

	return 0;
}
#endif

static int jpu_enable_jpu_mmu_hw(struct jpu_device *jpu_device)
{
	int i;
	struct tbu_instance *tbu;

	for (i = 0; i < TBU_INSTANCES_NUM; i++) {
		tbu = &jpu_device->tbu_ins[i];
		tbu->ttb_size = 0;
		tbu->always_preload = false;
		tbu->enable_preload = true;
		tbu->nsaid = 0;
		tbu->qos = 2;
		tbu->secure_enable = false;
	}
	jpu_device->tbu_ins_map = -1;
	//jpu_device->tbu_ins_bitmap = 0;
	sema_init(&jpu_device->tbu_ins_free_cnt, TBU_INSTANCES_NUM);

	/* Set MJPEG_MMU iova base */
	jpu_writel(jpu_device, MJPEG_MMU_BVA_LO, jpu_device->va_base & 0xFFFFFFFF);
	jpu_writel(jpu_device, MJPEG_MMU_BVA_HI, jpu_device->va_base >> 32);

	/* Set MJPEG_MMU timeout cycles */
	jpu_writel(jpu_device, MJPEG_MMU_TIMEOUT_VALUE, jpu_device->time_out_cycs);

	/* Enable MJPEG_MMU irq */
	jpu_set_reg_bits(jpu_device, MJPEG_MMU_IRQ_ENABLE, 0);

	jpu_device->is_hw_enable = true;
	return 0;
}

static void jpu_disable_jpu_mmu_hw(struct jpu_device *jpu_device)
{
	int i;
	struct tbu_instance *tbu;

	/* Waiting for post done. */
	spin_lock(&jpu_device->hw_access_lock);
	jpu_device->is_hw_enable = false;
	spin_unlock(&jpu_device->hw_access_lock);

	for (i = 0; i < TBU_INSTANCES_NUM; i++) {
		tbu = &jpu_device->tbu_ins[i];
		tbu->ttb_size = 0;
	}
	/* Disable all TBUs. */
	for (i = 0; i < TBU_NUM; i++)
		jpu_writel(jpu_device, MJPEG_MMU_TCR0_BASE + MJPEG_MMU_TBUx_STEP * i, 0);

	/* Disable MJPEG_MMU irq. */
	jpu_clear_reg_bits(jpu_device, MJPEG_MMU_IRQ_ENABLE, 0x1FF);

}

static void jpu_write_tbu_table(struct jpu_device *jpu_device, struct tbu_instance *tbu,
				unsigned long iova, phys_addr_t paddr, size_t size)
{
	u32 *ttb_entry;
	uint64_t mask = 0;
	uint32_t val;

	mask = (jpu_device->page_size == 4096) ? 0xFFFFFFFFFFFFF000 : 0xFFFFFFFFFFFF0000;
	ttb_entry = tbu->ttb_va + (iova - tbu->va_base) / jpu_device->page_size;
	while (size != 0) {
		paddr = paddr & 0xFFFFFFFF;
		val = ((paddr & mask) >> TTB_ENTRY_SHIFT) & 0x1FFFFF;
		*ttb_entry = val;
		size -= jpu_device->page_size;
		ttb_entry++;
		paddr += jpu_device->page_size;
	}
}

static void jpu_mmu_post(struct jpu_device *jpu_device, int *ins_id, int num)
{
	u32 reg;
	struct tbu_instance *tbu;
	int i, tbu_slot[TBU_NUM];

	for (i = 0; i < TBU_NUM; i++)
		tbu_slot[i] = -1;

	for (i = 0; i < num; i++) {
		int index;
		tbu = &jpu_device->tbu_ins[ins_id[i]];
		index = (tbu->va_base - jpu_device->va_base) / VA_STEP_PER_TBU;
		tbu_slot[index] = ins_id[i];
	}

	spin_lock(&jpu_device->hw_access_lock);
	if (!jpu_device->is_hw_enable) {
		spin_unlock(&jpu_device->hw_access_lock);
		return;
	}

	for (i = 0; i < TBU_NUM; i++) {
		if (tbu_slot[i] != -1) {
			tbu = &jpu_device->tbu_ins[tbu_slot[i]];
			if (tbu->ttb_size == 0) {
				jpu_writel(jpu_device,
					   MJPEG_MMU_TCR0_BASE + i * MJPEG_MMU_TBUx_STEP, 0);
			} else {
				jpu_writel(jpu_device,
					   MJPEG_MMU_TTBLR_BASE +
					   i * MJPEG_MMU_TBUx_STEP, tbu->ttb_pa & 0xFFFFFFFF);
				jpu_writel(jpu_device,
					   MJPEG_MMU_TTBHR_BASE +
					   i * MJPEG_MMU_TBUx_STEP, tbu->ttb_pa >> 32);

				reg = (tbu->ttb_size - 1) << 16;
				if (tbu->always_preload)
					reg |= BIT(3);
				if (tbu->enable_preload)
					reg |= BIT(2);
				if (jpu_device->page_size == SZ_64K)
					reg |= BIT(1);
				reg |= BIT(0);
				jpu_writel(jpu_device,
					   MJPEG_MMU_TCR0_BASE + i * MJPEG_MMU_TBUx_STEP, reg);
			}
		}
	}
	spin_unlock(&jpu_device->hw_access_lock);
}

static int jpu_mmu_map_sg(struct jpu_device *jpu_device, unsigned long iova,
			  struct scatterlist *sg, unsigned int nents, size_t *mapped,
			  u32 data_size, u32 append_buf_size, u32 need_append)
{
	struct tbu_instance *tbu;
	struct scatterlist *s;
	unsigned int i;
	unsigned int j;
	phys_addr_t paddr;
	size_t size;
	unsigned long orig_iova = iova;
	unsigned int offset = 0;
	unsigned int find = 0;
	unsigned int bottom_buf_size = 0;

	int invaild_data_size = data_size - append_buf_size;
	if ((iova >= jpu_device->va_end) && (nents == 1))
		return sg->length;

	jpu_device->tbu_ins_map = (iova - BASE_VIRTUAL_ADDRESS) / VA_STEP_PER_TBU;

	if (jpu_device->tbu_ins_map < 0 || jpu_device->tbu_ins_map >= TBU_INSTANCES_NUM)
		goto out_id_err;
	tbu = &jpu_device->tbu_ins[jpu_device->tbu_ins_map];

	if (tbu->ttb_size == 0) {
		int index;
		if (iova < jpu_device->va_base || iova >= jpu_device->va_end)
			goto out_iova_err;
		index = (iova - jpu_device->va_base) / VA_STEP_PER_TBU;
		tbu->va_base = jpu_device->va_base + index * VA_STEP_PER_TBU;
		tbu->va_end = tbu->va_base + VA_STEP_PER_TBU;
	}

	if (iova < tbu->va_base || iova >= tbu->va_end)
		goto out_iova_err;
	if (append_buf_size && need_append) {
		for_each_sg(sg, s, nents, i) {
			paddr = page_to_phys(sg_page(s)) + s->offset;
			size = s->length;
			if (!IS_ALIGNED(s->offset, jpu_device->page_size)) {
				dev_warn(jpu_device->jdev,
					 "paddr not aligned: iova %lx, paddr %llx, size %lx\n",
					 iova, paddr, size);
				goto out_region_err;
			}
			invaild_data_size -= size;
			if (invaild_data_size < 0) {
				if (!find) {
					find = 1;
				}
				if (find) {
					bottom_buf_size += size;
				}
				if (iova + size > tbu->va_end || size == 0)
					goto out_region_err;
				jpu_write_tbu_table(jpu_device, tbu, iova, paddr, size);
				iova += size;
			}
		}
		if (append_buf_size)
			offset = bottom_buf_size - append_buf_size;
	}
	for_each_sg(sg, s, nents, j) {
		paddr = page_to_phys(sg_page(s)) + s->offset;
		size = s->length;
		if (!IS_ALIGNED(s->offset, jpu_device->page_size)) {
			dev_warn(jpu_device->jdev,
				 "paddr not aligned: iova %lx, paddr %llx, size %lx\n",
				 iova, paddr, size);
			goto out_region_err;
		}
		if (iova + size > tbu->va_end || size == 0)
			goto out_region_err;

		jpu_write_tbu_table(jpu_device, tbu, iova, paddr, size);
		iova += size;
	}

	if (iova > tbu->va_base + jpu_device->page_size * tbu->ttb_size)
		tbu->ttb_size = (iova - tbu->va_base) / jpu_device->page_size;

	*mapped = iova - orig_iova;

	jpu_mmu_post(jpu_device, &jpu_device->tbu_ins_map, 1);
	return offset;

out_region_err:
	dev_err(jpu_device->jdev, "Map_sg is wrong: iova %lx, paddr %llx, size %lx\n",
		iova, paddr, size);
	return 0;
out_iova_err:
	dev_err(jpu_device->jdev, "Map_sg is wrong: iova %lx", iova);
	return 0;
out_id_err:
	dev_err(jpu_device->jdev, "TBU ins_id is wrong: %d\n", jpu_device->tbu_ins_map);
	return 0;
}

static dma_addr_t get_addr_from_fd(struct jpu_device *jpu_dev, int fd,
				   struct jpu_dma_buf_info *pInfo, u32 data_size,
				   u32 append_buf_size)
{
	struct device *dev = jpu_dev->jdev;
	struct sg_table *sgt;
	dma_addr_t addr;
	int offset = 0;
	size_t mapped_size = 0;
	u32 need_append = 0;

	pInfo->buf_fd = fd;
	pInfo->dmabuf = dma_buf_get(fd);
	if (IS_ERR(pInfo->dmabuf)) {
		pr_err("jpu get dmabuf fail fd:%d\n", fd);
		return 0;
	}
	pInfo->attach = dma_buf_attach(pInfo->dmabuf, dev);
	if (IS_ERR(pInfo->attach)) {
		pr_err("jpu get dma buf attach fail\n");
		goto err_dmabuf_put;
	}
	pInfo->sgtable = dma_buf_map_attachment_unlocked(pInfo->attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(pInfo->sgtable)) {
		pr_err("jpu get dma buf map attachment fail\n");
		goto err_dmabuf_detach;
	}
	sgt = pInfo->sgtable;
	if (sgt->nents == 1) {
		addr = sg_dma_address(sgt->sgl);
	} else {
		if (!jpu_dev->is_hw_enable)
			jpu_enable_jpu_mmu_hw(jpu_dev);
		addr = JPU_TBU_BASE_VA + (pInfo->tbu_id) * JPU_TBU_VA_STEP;
		if (pInfo->tbu_id == TBU_INPUT) {
			need_append = 1;
		}
		offset =
		    jpu_mmu_map_sg(jpu_dev, addr, sgt->sgl, sgt->nents, &mapped_size,
				   data_size, append_buf_size, need_append);
		if (!mapped_size) {
			pr_err("jpu iommu map sgtable fail\n");
			goto err_dmabuf_unmap;
		}
	}
	return addr + offset;
err_dmabuf_unmap:
	dma_buf_unmap_attachment_unlocked(pInfo->attach, pInfo->sgtable, DMA_BIDIRECTIONAL);
err_dmabuf_detach:
	dma_buf_detach(pInfo->dmabuf, pInfo->attach);
err_dmabuf_put:
	dma_buf_put(pInfo->dmabuf);
	return 0;
}

static int jpu_alloc_dma_buffer(struct jpu_device *jdev, jpudrv_buffer_t *jb)
{
	if (!jb) {
		return -EINVAL;
	}

	jb->base = (unsigned long)dma_alloc_coherent(jdev->jdev, PAGE_ALIGN(jb->size),
						     (dma_addr_t *) (&jb->phys_addr),
						     GFP_DMA | GFP_KERNEL);
	if ((void *)(jb->base) == NULL) {
		dev_err(jdev->jdev, "Physical memory allocation error size=%d\n", jb->size);
		return -ENOMEM;
	}

	return 0;
}

static void jpu_free_dma_buffer(struct jpu_device *jdev, jpudrv_buffer_t *jb)
{
	if (!jb) {
		return;
	}

	if (jb->base) {
		dma_free_coherent(jdev->jdev, PAGE_ALIGN(jb->size), (void *)jb->base,
				  jb->phys_addr);
	}
}

static int jpu_free_instances(struct file *filp)
{
	struct jpu_device *jdev = filp->private_data;
	jpudrv_instance_list_t *vil, *n;
	jpudrv_instance_pool_t *vip;
	void *vip_base;
	int instance_pool_size_per_core;
	void *jdi_mutexes_base;
	const int PTHREAD_MUTEX_T_DESTROY_VALUE = 0xdead10cc;

	JLOG(jdev->jdev, "free instance\n");

	// s_instance_pool.size  assigned to the size of all core once call JDI_IOCTL_GET_INSTANCE_POOL by user.
	instance_pool_size_per_core = (jdev->s_instance_pool.size / MAX_NUM_JPU_CORE);

	list_for_each_entry_safe(vil, n, &jdev->s_inst_list_head, list) {
		if (vil->filp == filp) {
			vip_base =
			    (void *)(jdev->s_instance_pool.base + instance_pool_size_per_core);
			JLOG(jdev->jdev,
			     "jpu_free_instances detect instance crash instIdx=%d, vip_base=%p, instance_pool_size_per_core=%d\n",
			     (int)vil->inst_idx, vip_base, (int)instance_pool_size_per_core);
			vip = (jpudrv_instance_pool_t *) vip_base;
			if (vip) {
				// only first 4 byte is key point(inUse of CodecInst in jpuapi) to free the corresponding instance
				memset(&vip->codecInstPool[vil->inst_idx], 0x00, 4);
#define PTHREAD_MUTEX_T_HANDLE_SIZE 4
				jdi_mutexes_base =
				    (vip_base + (instance_pool_size_per_core - PTHREAD_MUTEX_T_HANDLE_SIZE * 4));
				JLOG(jdev->jdev,
				     "force to destroy jdi_mutexes_base=%p in userspace \n", jdi_mutexes_base);
				if (jdi_mutexes_base) {
					int i;
					for (i = 0; i < 4; i++) {
						memcpy(jdi_mutexes_base,
						       &PTHREAD_MUTEX_T_DESTROY_VALUE, PTHREAD_MUTEX_T_HANDLE_SIZE);
						jdi_mutexes_base += PTHREAD_MUTEX_T_HANDLE_SIZE;
					}
				}
			}

			jdev->s_jpu_open_ref_count--;
			list_del(&vil->list);
			kfree(vil);
		}
	}

	return 0;
}

static int jpu_free_buffers(struct file *filp)
{
	struct jpu_device *jdev = filp->private_data;
	jpudrv_buffer_pool_t *pool, *n;
	jpudrv_buffer_t jb;

	JLOG(jdev->jdev, "jpu free buffers\n");

	list_for_each_entry_safe(pool, n, &jdev->s_jbp_head, list) {
		if (pool->filp == filp) {
			jb = pool->jb;
			if (jb.base) {
				jpu_free_dma_buffer(jdev, &jb);
				list_del(&pool->list);
				kfree(pool);
			}
		}
	}

	return 0;
}

static irqreturn_t jpu_irq_handler(int irq, void *dev_id)
{
	struct jpu_device *jdev = (struct jpu_device *)dev_id;
	jpu_drv_context_t *dev = &jdev->s_jpu_drv_context;
	int i = 0;
	unsigned long flags;
	u32 int_reason;
	u64 last_va, last_pa;
	u32 mmu_irq_status;
	u32 reg;
	int j;

	spin_lock_irqsave(&jdev->s_jpu_lock, flags);
	// suppose instance 0 irq handle
	int_reason = jpu_readl(jdev, MJPEG_PIC_STATUS_REG);
	if (int_reason != 0) {
		jdev->s_interrupt_flag[i] = 1;
		if (int_reason & (1 << INT_JPU_DONE)) {
			jpu_writel(jdev, MJPEG_BBC_FLUSH_CMD_REG, 0);
		}
		jpu_writel(jdev, MJPEG_PIC_STATUS_REG, int_reason);	// clear JPEG register
	}
	mmu_irq_status = jpu_readl(jdev, MJPEG_MMU_IRQ_STATUS);

	if (mmu_irq_status != 0) {
		reg = jpu_readl(jdev, MJPEG_MMU_LAST_PA_ADDR_HI);
		last_pa = reg & 0x1;
		reg = jpu_readl(jdev, MJPEG_MMU_LAST_PA_ADDR_LO);
		last_pa = (last_pa << 32) | reg;
		reg = jpu_readl(jdev, MJPEG_MMU_LAST_VA_ADDR_HI);
		last_va = reg & 0x1;
		reg = jpu_readl(jdev, MJPEG_MMU_LAST_VA_ADDR_LO);
		last_va = (last_va << 32) | reg;

		/* Print IRQ status. */
		dev_err_ratelimited(jdev->jdev,
				    "Unexpected fault: IRQ status 0x%x, last PA 0x%09llx, last VA 0x%09llx\n",
				    mmu_irq_status, last_pa, last_va);

		if (mmu_irq_status & BIT(8)) {
			u64 timeout_va_addr;
			reg = jpu_readl(jdev, MJPEG_MMU_TIMEOUT_VA_ADDR_HI);
			timeout_va_addr = reg & 0x1;
			reg = jpu_readl(jdev, MJPEG_MMU_TIMEOUT_VA_ADDR_LO);
			timeout_va_addr = (timeout_va_addr << 32) | reg;
			dev_err_ratelimited(jdev->jdev,
					    "timeout error: timeout_va 0x%09llx\n",
					    timeout_va_addr);
		}

		for (j = 0; j < TBU_NUM; j++) {
			if (mmu_irq_status & BIT(i)) {
				reg = jpu_readl(jdev, MJPEG_MMU_TBU_STATUS_BASE + j * MJPEG_MMU_TBUx_STEP);
				dev_err_ratelimited(jdev->jdev,
						    "TBU%d error: read addr 0x%08x, write addr 0x%08x\n",
						    j, ((reg >> 16) & 0xFFF), reg & 0x1FFF);
			}
		}

		/* clear DMA error */
		if (mmu_irq_status & 0xFF)
			jpu_set_reg_bits(jdev, MJPEG_MMU_ERROR_CLEAR, BIT(1));

		/* reset IRQ status */
		jpu_writel(jdev, MJPEG_MMU_IRQ_STATUS, mmu_irq_status);
	}
	dev->interrupt_reason[i] = int_reason;
	spin_unlock_irqrestore(&jdev->s_jpu_lock, flags);

	JLOG(jdev->jdev, "JPU: instance no %d, INTERRUPT FLAG: %08x, %08x\n",
	     i, dev->interrupt_reason[i], MJPEG_PIC_STATUS_REG);

	if (dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);	// notify the interrupt to userspace

	wake_up_interruptible(&jdev->s_interrupt_wait_q[i]);
	return IRQ_HANDLED;
}

static int jpu_open(struct inode *inode, struct file *filp)
{
	struct jpu_device *jdev = container_of(inode->i_cdev, struct jpu_device, s_jpu_cdev);

	spin_lock(&jdev->s_jpu_lock);

	if (jdev->s_jpu_drv_context.open_count) {
		spin_unlock(&jdev->s_jpu_lock);
		return -EBUSY;
	} else {
		jdev->s_jpu_drv_context.open_count++;
	}

	filp->private_data = jdev;
	spin_unlock(&jdev->s_jpu_lock);
	pm_runtime_get_sync(jdev->jdev);
	return 0;
}

static ssize_t jpu_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	return 0;
}

static ssize_t jpu_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
	return 0;
}

static long jpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	struct jpu_device *jdev = filp->private_data;
	jpudrv_buffer_pool_t *jbp, *n;
	jpudrv_buffer_t jb;
	jpudrv_intr_info_t info;
	jpudrv_inst_info_t inst_info;
	struct jpu_drv_context_t *dev_ctx;
	u32 instance_no;
	JPU_DMA_CFG cfg;
	int i;
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sg_table;
	jpu_dma_buf_info pInfo;
#ifndef CONFIG_SOC_KY_X1_FPGA
	u32 clkgate;
#endif
	int ret = 0;

	switch (cmd) {
	case JDI_IOCTL_ALLOCATE_PHYSICAL_MEMORY:
		ret = down_interruptible(&jdev->s_jpu_sem);
		if (ret) {
			return -EAGAIN;
		}

		jbp = kzalloc(sizeof(jpudrv_buffer_pool_t), GFP_KERNEL);
		if (!jbp) {
			up(&jdev->s_jpu_sem);
			return -ENOMEM;
		}

		ret = copy_from_user(&(jbp->jb), (jpudrv_buffer_t *) arg, sizeof(jpudrv_buffer_t));
		if (ret) {
			kfree(jbp);
			up(&jdev->s_jpu_sem);
			return -EFAULT;
		}

		ret = jpu_alloc_dma_buffer(jdev, &(jbp->jb));
		if (ret) {
			kfree(jbp);
			up(&jdev->s_jpu_sem);
			return -ENOMEM;
		}

		ret = copy_to_user((void __user *)arg, &(jbp->jb), sizeof(jpudrv_buffer_t));
		if (ret) {
			kfree(jbp);
			up(&jdev->s_jpu_sem);
			return -EFAULT;
		}

		jbp->filp = filp;

		spin_lock(&jdev->s_jpu_lock);
		list_add(&jbp->list, &jdev->s_jbp_head);
		spin_unlock(&jdev->s_jpu_lock);

		up(&jdev->s_jpu_sem);

		break;
	case JDI_IOCTL_FREE_PHYSICALMEMORY:
		ret = down_interruptible(&jdev->s_jpu_sem);
		if (ret) {
			return -EAGAIN;
		}

		ret = copy_from_user(&jb, (jpudrv_buffer_t *) arg, sizeof(jpudrv_buffer_t));
		if (ret) {
			up(&jdev->s_jpu_sem);
			return -EACCES;
		}

		if (jb.base) {
			jpu_free_dma_buffer(jdev, &jb);
		}

		spin_lock(&jdev->s_jpu_lock);

		list_for_each_entry_safe(jbp, n, &jdev->s_jbp_head, list) {
			if (jbp->jb.base == jb.base) {
				list_del(&jbp->list);
				kfree(jbp);
				break;
			}
		}

		spin_unlock(&jdev->s_jpu_lock);

		up(&jdev->s_jpu_sem);

		break;
	case JDI_IOCTL_GET_RESERVED_VIDEO_MEMORY_INFO:
		ret = -EFAULT;
		break;
	case JDI_IOCTL_WAIT_INTERRUPT:
		dev_ctx = &jdev->s_jpu_drv_context;
		ret = copy_from_user(&info, (jpudrv_intr_info_t *) arg, sizeof(jpudrv_intr_info_t));
		if (ret) {
			return -EFAULT;
		}

		instance_no = info.inst_idx;
		ret =
		    wait_event_interruptible_timeout(jdev->s_interrupt_wait_q[instance_no],
						     jdev->s_interrupt_flag[instance_no] != 0,
						     msecs_to_jiffies(info.timeout));
		if (!ret) {
			dev_err(jdev->jdev,
				"instance no %d ETIME, s_interrupt_flag(%d), reason(0x%08x)\n",
				instance_no, jdev->s_interrupt_flag[instance_no],
				dev_ctx->interrupt_reason[instance_no]);
			ret = -ETIME;
			break;
		}
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			dev_err(jdev->jdev, "instance no: %d ERESTARTSYS\n", instance_no);
			break;
		}

		JLOG(jdev->jdev, "INST(%d) s_interrupt_flag(%d), reason(0x%08x)\n",
		     instance_no, jdev->s_interrupt_flag[instance_no],
		     dev_ctx->interrupt_reason[instance_no]);

		spin_lock(&jdev->s_jpu_lock);
		info.intr_reason = dev_ctx->interrupt_reason[instance_no];
		jdev->s_interrupt_flag[instance_no] = 0;
		dev_ctx->interrupt_reason[instance_no] = 0;
		spin_unlock(&jdev->s_jpu_lock);

		for (i = 0; i < TBU_INSTANCES_NUM; i++) {
			pInfo = buf_inf[i];
			dmabuf = pInfo.dmabuf;
			attach = pInfo.attach;
			sg_table = pInfo.sgtable;

			if (dmabuf && attach && sg_table) {
				dma_buf_unmap_attachment_unlocked(attach, sg_table, DMA_BIDIRECTIONAL);
				dma_buf_detach(dmabuf, attach);
				dma_buf_put(dmabuf);
			}
		}
		if (jdev->is_hw_enable) {
			jpu_disable_jpu_mmu_hw(jdev);
		}
		ret = copy_to_user((void __user *)arg, &info, sizeof(jpudrv_intr_info_t));
		if (ret) {
			return -EFAULT;
		}
#if defined (CONFIG_PM) && defined (DDR_QOS_ENABLE)
		//freq_qos_update_request(jdev->ddr_qos_rreq, PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
		//freq_qos_update_request(jdev->ddr_qos_wreq, PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
#endif
		break;
	case JDI_IOCTL_SET_CLOCK_GATE:;
#ifndef CONFIG_SOC_KY_X1_FPGA
		ret = down_interruptible(&jdev->s_jpu_sem);
		if (ret) {
			return -EAGAIN;
		}

		if (get_user(clkgate, (u32 __user *) arg)) {
			up(&jdev->s_jpu_sem);
			return -EFAULT;
		}

		if (clkgate) {
			pm_runtime_get_sync(jdev->jdev);
			jpu_clk_enable(jdev);
		} else {
			jpu_clk_disable(jdev);
			pm_runtime_put_sync(jdev->jdev);
		}

		up(&jdev->s_jpu_sem);
#endif
		break;
	case JDI_IOCTL_GET_INSTANCE_POOL:
		ret = down_interruptible(&jdev->s_jpu_sem);
		if (ret) {
			return -EAGAIN;
		}

		if (jdev->s_instance_pool.base) {
			ret = copy_to_user((void __user *)arg, &jdev->s_instance_pool,
					   sizeof(jpudrv_buffer_t));
		} else {
			ret = copy_from_user(&jdev->s_instance_pool,
					     (jpudrv_buffer_t *) arg, sizeof(jpudrv_buffer_t));
			if (!ret) {
				jdev->s_instance_pool.size = PAGE_ALIGN(jdev->s_instance_pool.size);
				jdev->s_instance_pool.base =
				    (unsigned long)vmalloc(jdev->s_instance_pool.size);
				jdev->s_instance_pool.phys_addr = jdev->s_instance_pool.base;

				if (jdev->s_instance_pool.base != 0) {
					memset((void *)jdev->s_instance_pool.base, 0x0, jdev->s_instance_pool.size);	/*clearing memory */
					ret = copy_to_user((void __user *)arg,
							   &jdev->s_instance_pool, sizeof(jpudrv_buffer_t));
					if (ret == 0) {
						/* success to get memory for instance pool */
						up(&jdev->s_jpu_sem);
						break;
					}
				}
				ret = -EFAULT;
			}

		}

		up(&jdev->s_jpu_sem);

		JLOG(jdev->jdev,
		     "JDI_IOCTL_GET_INSTANCE_POOL: %s base: %lx, size: %d\n",
		     (ret == 0 ? "OK" : "NG"), jdev->s_instance_pool.base,
		     jdev->s_instance_pool.size);

		break;
	case JDI_IOCTL_OPEN_INSTANCE:
		if (copy_from_user(&inst_info, (jpudrv_inst_info_t *) arg, sizeof(jpudrv_inst_info_t)))
			return -EFAULT;

		spin_lock(&jdev->s_jpu_lock);
		jdev->s_jpu_open_ref_count++;	/* flag just for that jpu is in opened or closed */
		inst_info.inst_open_count = jdev->s_jpu_open_ref_count;
		spin_unlock(&jdev->s_jpu_lock);

		if (copy_to_user((void __user *)arg, &inst_info, sizeof(jpudrv_inst_info_t))) {
			return -EFAULT;
		}

		JLOG(jdev->jdev,
		     "JDI_IOCTL_OPEN_INSTANCE inst_idx=%d, s_jpu_open_ref_count=%d, inst_open_count=%d\n",
		     (int)inst_info.inst_idx, jdev->s_jpu_open_ref_count, inst_info.inst_open_count);

		break;
	case JDI_IOCTL_CLOSE_INSTANCE:
		if (copy_from_user(&inst_info, (jpudrv_inst_info_t *) arg, sizeof(jpudrv_inst_info_t)))
			return -EFAULT;

		spin_lock(&jdev->s_jpu_lock);
		jdev->s_jpu_open_ref_count--;	/* flag just for that jpu is in opened or closed */
		inst_info.inst_open_count = jdev->s_jpu_open_ref_count;
		spin_unlock(&jdev->s_jpu_lock);

		if (copy_to_user((void __user *)arg, &inst_info, sizeof(jpudrv_inst_info_t)))
			return -EFAULT;

		JLOG(jdev->jdev,
		     "JDI_IOCTL_CLOSE_INSTANCE inst_idx=%d, s_jpu_open_ref_count=%d, inst_open_count=%d\n",
		     (int)inst_info.inst_idx, jdev->s_jpu_open_ref_count, inst_info.inst_open_count);

		break;
	case JDI_IOCTL_GET_INSTANCE_NUM:
		ret = copy_from_user(&inst_info, (jpudrv_inst_info_t *) arg, sizeof(jpudrv_inst_info_t));
		if (ret != 0)
			break;

		spin_lock(&jdev->s_jpu_lock);
		inst_info.inst_open_count = jdev->s_jpu_open_ref_count;
		spin_unlock(&jdev->s_jpu_lock);

		ret = copy_to_user((void __user *)arg, &inst_info, sizeof(jpudrv_inst_info_t));

		JLOG(jdev->jdev,
		     "JDI_IOCTL_GET_INSTANCE_NUM inst_idx=%d, open_count=%d\n",
		     (int)inst_info.inst_idx, inst_info.inst_open_count);
		break;
	case JDI_IOCTL_RESET:
#ifndef CONFIG_SOC_KY_X1_FPGA

		ret = down_interruptible(&jdev->s_jpu_sem);
		if (ret) {
			return -EAGAIN;
		}

		jpu_hw_reset(jdev);

		up(&jdev->s_jpu_sem);
#endif
		break;
	case JDI_IOCTL_GET_REGISTER_INFO:
		ret = copy_to_user((void __user *)arg, &jdev->s_jpu_register, sizeof(jpudrv_buffer_t));
		if (ret != 0) {
			ret = -EFAULT;
		}
		JLOG(jdev->jdev, "JDI_IOCTL_GET_REGISTER_INFO s_jpu_register.phys_addr=0x%lx, s_jpu_register.virt_addr=0x%lx, s_jpu_register.size=%d\n",
				 jdev->s_jpu_register.phys_addr, jdev->s_jpu_register.virt_addr, jdev->s_jpu_register.size);
		break;
	case JDI_IOCTL_CFG_MMU:
		//JLOG(jdev->jdev, "JDI_IOCTL_CFG_MMU \n");
		ret = copy_from_user(&cfg, (JPU_DMA_CFG *) arg, sizeof(JPU_DMA_CFG));
		if (ret != 0) {
			ret = -EFAULT;
		}
		//JLOG(jdev->jdev, "JDI_IOCTL_CFG_MMU input fd:%d output:%d\n",cfg.intput_buf_fd,cfg.output_buf_fd);
		buf_inf[0].tbu_id = TBU_INPUT;
		buf_inf[1].tbu_id = TBU_OUTPUT;
		cfg.intput_virt_addr =
		    get_addr_from_fd(jdev, cfg.intput_buf_fd, &buf_inf[0], cfg.data_size, cfg.append_buf_size);
		cfg.output_virt_addr =
		    get_addr_from_fd(jdev, cfg.output_buf_fd, &buf_inf[1], cfg.data_size, cfg.append_buf_size);
		ret = copy_to_user((void __user *)arg, &cfg, sizeof(JPU_DMA_CFG));
		if (ret != 0) {
			ret = -EFAULT;
		}
		jpu_writel(jdev, JPU_MMU_TRI, 0x01);
#if defined (CONFIG_PM) && defined (DDR_QOS_ENABLE)
		//_update_request(jdev->ddr_qos_rreq,160000);
		//freq_qos_update_request(jdev->ddr_qos_wreq,8000);
#endif
		JLOG(jdev->jdev, "JDI_IOCTL_CFG_MMU DONE!\n");
		break;
	default:
		dev_err(jdev->jdev, "No such IOCTL, cmd is %d\n", cmd);
		break;
	}

	return ret;
}

static int jpu_fasync(int fd, struct file *filp, int mode)
{
	struct jpu_device *jdev = filp->private_data;

	return fasync_helper(fd, filp, mode, &jdev->s_jpu_drv_context.async_queue);
}

static int jpu_release(struct inode *inode, struct file *filp)
{
	struct jpu_device *jdev = filp->private_data;
	int ret = 0;
	u32 open_count;

	ret = down_interruptible(&jdev->s_jpu_sem);
	if (ret) {
		return -EAGAIN;
	}

	/* found and free the not handled buffer by user applications */
	jpu_free_buffers(filp);

	/* found and free the not closed instance by user applications */
	jpu_free_instances(filp);
	JLOG(jdev->jdev, "open_count: %d\n", jdev->s_jpu_drv_context.open_count);

	spin_lock(&jdev->s_jpu_lock);
	jdev->s_jpu_drv_context.open_count--;
	open_count = jdev->s_jpu_drv_context.open_count;
	spin_unlock(&jdev->s_jpu_lock);

	if (open_count == 0) {
		if (jdev->s_instance_pool.base) {
			JLOG(jdev->jdev, "free instance pool\n");
			vfree((const void *)jdev->s_instance_pool.base);
			jdev->s_instance_pool.base = 0;
		}
#ifndef CONFIG_SOC_KY_X1_FPGA
		jpu_clk_disable(jdev);
		pm_runtime_put_sync(jdev->jdev);

#endif
	}

	up(&jdev->s_jpu_sem);

	JLOG(jdev->jdev, "released\n");

	return 0;
}

static int jpu_map_to_register(struct file *fp, struct vm_area_struct *vm)
{
	struct jpu_device *jdev = fp->private_data;
	unsigned long pfn;

	vm_flags_set(vm, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);
	pfn = jdev->s_jpu_register.phys_addr >> PAGE_SHIFT;

	return remap_pfn_range(vm, vm->vm_start, pfn, vm->vm_end - vm->vm_start,
			       vm->vm_page_prot) ? -EAGAIN : 0;
}

static int jpu_map_to_physical_memory(struct file *fp, struct vm_area_struct *vm)
{
	vm_flags_set(vm, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
	vm->vm_page_prot = pgprot_noncached(vm->vm_page_prot);

	return remap_pfn_range(vm, vm->vm_start, vm->vm_pgoff,
			       vm->vm_end - vm->vm_start, vm->vm_page_prot) ? -EAGAIN : 0;
}

static int jpu_map_to_instance_pool_memory(struct file *fp, struct vm_area_struct *vm)
{
	struct jpu_device *jdev = fp->private_data;
	int ret;
	long length = vm->vm_end - vm->vm_start;
	unsigned long start = vm->vm_start;
	char *vmalloc_area_ptr = (char *)jdev->s_instance_pool.base;
	unsigned long pfn;

	vm_flags_set(vm, VM_DONTEXPAND | VM_DONTDUMP);

	/* loop over all pages, map it page individually */
	while (length > 0) {
		pfn = vmalloc_to_pfn(vmalloc_area_ptr);
		if ((ret = remap_pfn_range(vm, start, pfn, PAGE_SIZE, PAGE_SHARED)) < 0) {
			return ret;
		}
		start += PAGE_SIZE;
		vmalloc_area_ptr += PAGE_SIZE;
		length -= PAGE_SIZE;
	}

	return 0;
}

static int jpu_mmap(struct file *fp, struct vm_area_struct *vm)
{
	struct jpu_device *jdev = fp->private_data;

	if (vm->vm_pgoff == 0)
		return jpu_map_to_instance_pool_memory(fp, vm);

	if (vm->vm_pgoff == (jdev->s_jpu_register.phys_addr >> PAGE_SHIFT))
		return jpu_map_to_register(fp, vm);

	return jpu_map_to_physical_memory(fp, vm);
}

struct file_operations jpu_fops = {
	.owner = THIS_MODULE,
	.open = jpu_open,
	.read = jpu_read,
	.write = jpu_write,
	.unlocked_ioctl = jpu_ioctl,
	.release = jpu_release,
	.fasync = jpu_fasync,
	.mmap = jpu_mmap,
};

static ssize_t cclk_max_frequency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	return sprintf(buf, "%u\n", jdev->cclk_max_frequency);
}

static ssize_t cclk_min_frequency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	return sprintf(buf, "%u\n", jdev->cclk_min_frequency);
}

static ssize_t cclk_cur_frequency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	return sprintf(buf, "%u\n", jdev->cclk_cur_frequency);
}

static ssize_t cclk_cur_frequency_store(struct device *dev,
					struct device_attribute *attr, const char *buf,
					size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);
	unsigned long cclk_cur_frequency = 0;
	int ret;

	ret = kstrtoul(buf, 0, &cclk_cur_frequency);
	if (ret < 0) {
		return ret;
	}

	if (cclk_cur_frequency < jdev->cclk_min_frequency
	    || cclk_cur_frequency > jdev->cclk_max_frequency) {
		return -EINVAL;
	}

	jdev->cclk_cur_frequency = cclk_cur_frequency;

	return count;
}

static DEVICE_ATTR_RO(cclk_max_frequency);
static DEVICE_ATTR_RO(cclk_min_frequency);
static DEVICE_ATTR_RW(cclk_cur_frequency);

static struct attribute *cclk_frequency_attrs[] = {
	&dev_attr_cclk_max_frequency.attr,
	&dev_attr_cclk_min_frequency.attr,
	&dev_attr_cclk_cur_frequency.attr,
	NULL,
};

static const struct attribute_group cclk_frequency_group = {
	.name = "cclk",
	.attrs = cclk_frequency_attrs,
};

static ssize_t aclk_max_frequency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	return sprintf(buf, "%llu\n", jdev->aclk_max_frequency);
}

static ssize_t aclk_min_frequency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	return sprintf(buf, "%u\n", jdev->aclk_min_frequency);
}

static ssize_t aclk_cur_frequency_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	return sprintf(buf, "%u\n", jdev->aclk_cur_frequency);
}

static ssize_t aclk_cur_frequency_store(struct device *dev,
					struct device_attribute *attr, const char *buf,
					size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct jpu_device *jdev = platform_get_drvdata(pdev);
	unsigned long aclk_cur_frequency = 0;
	int ret;

	ret = kstrtoul(buf, 0, &aclk_cur_frequency);
	if (ret < 0) {
		return ret;
	}

	if (aclk_cur_frequency < jdev->aclk_min_frequency
	    || aclk_cur_frequency > jdev->aclk_max_frequency) {
		return -EINVAL;
	}

	jdev->aclk_cur_frequency = aclk_cur_frequency;

	return count;
}

static DEVICE_ATTR_RO(aclk_max_frequency);
static DEVICE_ATTR_RO(aclk_min_frequency);
static DEVICE_ATTR_RW(aclk_cur_frequency);

static struct attribute *aclk_frequency_attrs[] = {
	&dev_attr_aclk_max_frequency.attr,
	&dev_attr_aclk_min_frequency.attr,
	&dev_attr_aclk_cur_frequency.attr,
	NULL,
};

static const struct attribute_group aclk_frequency_group = {
	.name = "aclk",
	.attrs = aclk_frequency_attrs
};

static const struct attribute_group *jpu_frequency_group[] = {
	&cclk_frequency_group,
	&aclk_frequency_group,
	NULL,
};

static const struct of_device_id jpu_dt_match[] = {
	{
	 .compatible = "chip-media, jpu",
	  },
	{ }
};

static u64 jpu_dmamask = 0xffffffffffUL;

static int jpu_probe(struct platform_device *pdev)
{
	struct jpu_device *jdev;
	struct resource *res;
	const struct of_device_id *of_id;
	uint32_t device_id = 0;
	char cdev_name[32] = { 0 };
	int err, i;
	struct cpumask mask = { CPU_BITS_NONE };
	int cpuid = 0;
	void *va_temp;
	dma_addr_t pa_temp;
	jdev = devm_kzalloc(&pdev->dev, sizeof(*jdev), GFP_KERNEL);
	if (!jdev) {
		return -ENOMEM;
	}

	jdev->jdev = &pdev->dev;
	jdev->jdev->dma_mask = &jpu_dmamask;
	jdev->jdev->coherent_dma_mask = 0xffffffffffull;
	platform_set_drvdata(pdev, jdev);
	INIT_LIST_HEAD(&jdev->s_jbp_head);
	INIT_LIST_HEAD(&jdev->s_inst_list_head);
	sema_init(&jdev->s_jpu_sem, 1);
	spin_lock_init(&jdev->s_jpu_lock);
	for (i = 0; i < MAX_NUM_INSTANCE; i++) {
		init_waitqueue_head(&jdev->s_interrupt_wait_q[i]);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR_OR_NULL(res)) {
		dev_err(jdev->jdev, "No I/O registers defined");
		return -ENXIO;
	}

	jdev->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(jdev->reg)) {
		dev_err(jdev->jdev, "Failed to map JPU registers.\n");
		return PTR_ERR(jdev->reg);
	}

	jdev->s_jpu_register.phys_addr = res->start;
	jdev->s_jpu_register.virt_addr = (unsigned long)jdev->reg;
	jdev->s_jpu_register.size = resource_size(res);

	jdev->s_jpu_irq = platform_get_irq(pdev, 0);
	if (jdev->s_jpu_irq < 0) {
		dev_err(jdev->jdev, "No irq defined\n");
		return -ENXIO;
	}
	err = of_property_read_u32(pdev->dev.of_node, "page-size", &jdev->page_size);
	if (err == 0)
		jdev->page_size *= SZ_1K;
	else
		jdev->page_size = SZ_4K;

	jdev->is_hw_enable = false;
	va_temp = dma_alloc_coherent(jdev->jdev, MAX_SIZE_PER_TTB * TBU_INSTANCES_NUM,
				     &pa_temp, GFP_KERNEL | GFP_DMA);
	if (!va_temp) {
		dev_err(jdev->jdev, "No memory for %d tbu_ins!\n", TBU_INSTANCES_NUM);
		goto err0;
	}
	for (i = 0; i < TBU_INSTANCES_NUM; i++) {
		struct tbu_instance *tbu = &jdev->tbu_ins[i];
		tbu->ttb_va = va_temp + i * MAX_SIZE_PER_TTB;
		tbu->ttb_pa = pa_temp + i * MAX_SIZE_PER_TTB;
		tbu->ins_id = i;
	}
	spin_lock_init(&jdev->tbu_ins_bitmap_lock);

	jdev->va_base = BASE_VIRTUAL_ADDRESS;
	jdev->va_end = BASE_VIRTUAL_ADDRESS + TBU_NUM * VA_STEP_PER_TBU;
	jdev->time_out_cycs = DEFAULT_TIMEOUT_CYCS;

	spin_lock_init(&jdev->hw_access_lock);
	err = devm_request_irq(&pdev->dev, jdev->s_jpu_irq, jpu_irq_handler, 0, "JPU", jdev);
	if (err) {
		dev_err(jdev->jdev, "irq not be registered\n");
		return err;
	}
#ifndef CONFIG_SOC_KY_X1_FPGA

	jdev->aclk = devm_clk_get(&pdev->dev, "aclk");
	if (IS_ERR_OR_NULL(jdev->aclk)) {
		dev_err(jdev->jdev, "not found axi clk\n");
		return PTR_ERR(jdev->aclk);
	}
	atomic_set(&jdev->aclk_enable_count, 0);
	jdev->cclk = devm_clk_get(&pdev->dev, "cclk");
	if (IS_ERR_OR_NULL(jdev->cclk)) {
		dev_err(jdev->jdev, "not found core clock\n");
		return PTR_ERR(jdev->cclk);
	}
	atomic_set(&jdev->cclk_enable_count, 0);
	if (of_property_read_u32(pdev->dev.of_node, "jpu,cclk-max-frequency", &jdev->cclk_max_frequency)) {
		dev_err(jdev->jdev, "not read cclk max frequency.\n");
		return -ENXIO;
	}

	if (of_property_read_u32(pdev->dev.of_node, "jpu,cclk-min-frequency", &jdev->cclk_min_frequency)) {
		dev_err(jdev->jdev, "not read cclk min frequency.\n");
		return -ENXIO;
	}

	if (of_property_read_u32(pdev->dev.of_node, "jpu,cclk-default-frequency", &jdev->cclk_default_frequency)) {
		dev_err(jdev->jdev, "not read cclk default frequency.\n");
		return -ENXIO;
	} else {
		jdev->cclk_cur_frequency = jdev->cclk_default_frequency;
	}

	jdev->iclk = devm_clk_get(&pdev->dev, "iclk");
	if (IS_ERR_OR_NULL(jdev->iclk)) {
		dev_err(jdev->jdev, "not found core clock\n");
		return PTR_ERR(jdev->iclk);
	}
	atomic_set(&jdev->iclk_enable_count, 0);
#endif
	jdev->jpg_reset = devm_reset_control_get_optional_shared(&pdev->dev, "jpg_reset");
	if (IS_ERR_OR_NULL(jdev->jpg_reset)) {
		dev_err(jdev->jdev, "not found core jpg_reset\n");
		return PTR_ERR(jdev->jpg_reset);
	}
	atomic_set(&jdev->jpg_reset_enable_count, 0);
	jdev->lcd_mclk_reset = devm_reset_control_get_optional_shared(&pdev->dev, "lcd_mclk_reset");
	if (IS_ERR_OR_NULL(jdev->lcd_mclk_reset)) {
		dev_err(jdev->jdev, "not found core lcd_mclk_reset\n");
		return PTR_ERR(jdev->lcd_mclk_reset);
	}
	atomic_set(&jdev->lcd_mclk_reset_enable_count, 0);
	jdev->isp_ci_reset = devm_reset_control_get_optional_shared(&pdev->dev, "isp_ci_reset");
	if (IS_ERR_OR_NULL(jdev->isp_ci_reset)) {
		dev_err(jdev->jdev, "not found core isp_ci_reset\n");
		return PTR_ERR(jdev->isp_ci_reset);
	}
	atomic_set(&jdev->isp_ci_reset_enable_count, 0);
	jdev->freset = devm_reset_control_get_optional_shared(&pdev->dev, "freset");
	if (IS_ERR_OR_NULL(jdev->freset)) {
		dev_err(jdev->jdev, "not found core freset\n");
		return PTR_ERR(jdev->freset);
	}
	atomic_set(&jdev->freset_enable_count, 0);
	jdev->sreset = devm_reset_control_get_optional_shared(&pdev->dev, "sreset");
	if (IS_ERR_OR_NULL(jdev->sreset)) {
		dev_err(jdev->jdev, "not found core sreset\n");
		return PTR_ERR(jdev->sreset);
	}
	atomic_set(&jdev->sreset_enable_count, 0);

	of_id = of_match_device(jpu_dt_match, &pdev->dev);
	if (!of_id) {
		dev_err(jdev->jdev, "No matching device to of_node: %p.\n", pdev->dev.of_node);
		return -EINVAL;
	}

	if (of_property_read_u32(pdev->dev.of_node, "jpu,chip-id", &device_id)) {
		dev_err(jdev->jdev, "not found device id defined.\n");
		return -ENXIO;
	}

	snprintf(cdev_name, sizeof(cdev_name), "%s%d", "jpu", device_id);
	JLOG(jdev->jdev, "cdev name %s\n", cdev_name);
	if ((alloc_chrdev_region(&jdev->s_jpu_major, 0, 1, cdev_name)) < 0) {
		dev_err(jdev->jdev, "could not allocate major number\n");
		return -EBUSY;
	}

	/* initialize the device structure and register the device with the kernel */
	cdev_init(&jdev->s_jpu_cdev, &jpu_fops);
	jdev->s_jpu_cdev.owner = THIS_MODULE;

	/* Register char dev with the kernel */
	if ((cdev_add(&jdev->s_jpu_cdev, jdev->s_jpu_major, 1)) < 0) {
		dev_err(jdev->jdev, "could not allocate chrdev\n");
		return -EBUSY;
	}

	JLOG(jdev->jdev, "cdev major %d, minor %d\n", MAJOR(jdev->s_jpu_major),
	     MINOR(jdev->s_jpu_major));

	/* Create class for device driver. */
	jdev->jpu_class = class_create(cdev_name);

	/* Create a device node. */
	jdev->jpu_device = device_create(jdev->jpu_class, NULL, jdev->s_jpu_major, NULL, cdev_name);

	err = sysfs_create_groups(&pdev->dev.kobj, jpu_frequency_group);
	if (err < 0) {
		return err;
	}
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#if defined (CONFIG_PM) && defined (DDR_QOS_ENABLE)
#if 0
	jdev->ddr_qos_cons = ddr_get_freq_constraints();
	jdev->ddr_qos_rreq = &jpu_ddrfreq_qos_rreq_sum;
	jdev->ddr_qos_wreq = &jpu_ddrfreq_qos_wreq_sum;
	freq_qos_add_request(jdev->ddr_qos_cons, jdev->ddr_qos_rreq, FREQ_QOS_RSUM,
			     PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
	freq_qos_add_request(jdev->ddr_qos_cons, jdev->ddr_qos_wreq, FREQ_QOS_WSUM,
			     PM_QOS_CPUIDLE_BLOCK_DEFAULT_VALUE);
#endif
#endif
	cpuid = 0;
	cpumask_set_cpu(cpuid, &mask);
	irq_set_affinity(jdev->s_jpu_irq, &mask);
	dev_dbg(jdev->jdev, "driver probe successfully\n");
	return 0;
err0:
	return -1;
}

static int jpu_remove(struct platform_device *pdev)
{
	struct jpu_device *jdev = platform_get_drvdata(pdev);

	if (jdev->s_instance_pool.base) {
		vfree((const void *)jdev->s_instance_pool.base);
		jdev->s_instance_pool.base = 0;
	}

	if (jdev->jpu_device) {
		device_destroy(jdev->jpu_class, jdev->s_jpu_major);
	}

	if (jdev->jpu_class) {
		class_destroy(jdev->jpu_class);
	}

	if (jdev->s_jpu_major) {
		cdev_del(&jdev->s_jpu_cdev);
		unregister_chrdev_region(jdev->s_jpu_major, 1);
		jdev->s_jpu_major = 0;
	}
#ifndef CONFIG_SOC_KY_X1_FPGA
	jpu_clk_disable(jdev);
	pm_runtime_put_sync(jdev->jdev);
	pm_runtime_disable(&pdev->dev);
#endif
	sysfs_remove_groups(&pdev->dev.kobj, jpu_frequency_group);

	dev_dbg(jdev->jdev, "driver removed\n");

	return 0;
}

#ifdef CONFIG_PM
static int jpu_suspend(struct device *dev)
{
#ifndef CONFIG_SOC_KY_X1_FPGA
	struct jpu_device *jdev = dev_get_drvdata(dev);
	jpu_clk_disable(jdev);
#endif

	return 0;
}

static int jpu_resume(struct device *dev)
{
	return 0;
}

static int jpu_runtime_suspend(struct device *dev)
{
	return 0;
}

static int jpu_runtime_resume(struct device *dev)
{
    return 0;
}

static const struct dev_pm_ops jpu_pm_ops = {
	.runtime_suspend = jpu_runtime_suspend,
	.runtime_resume  = jpu_runtime_resume,
	SET_SYSTEM_SLEEP_PM_OPS(jpu_suspend, jpu_resume)
};
#else
#define jpu_suspend NULL
#define jpu_resume NULL
#define jpu_runtime_suspend NULL
#define jpu_runtime_resume NULL
#endif

static struct platform_driver jpu_driver = {
	.driver = {
			.name = "jpu",
			.of_match_table = jpu_dt_match,
#ifdef CONFIG_PM
			.pm             = &jpu_pm_ops
#endif /* CONFIG_PM */
		    },
	.probe = jpu_probe,
	.remove = jpu_remove,
};

static int __init jpu_init(void)
{
	jpu_exp_init();
	return platform_driver_register(&jpu_driver);
}

static void __exit jpu_exit(void)
{
	jpu_exp_exit();
	platform_driver_unregister(&jpu_driver);
}

MODULE_AUTHOR("KY Limited, Inc.");
MODULE_DESCRIPTION("JPU linux driver");
MODULE_LICENSE("GPL v2");

module_init(jpu_init);
module_exit(jpu_exit);
