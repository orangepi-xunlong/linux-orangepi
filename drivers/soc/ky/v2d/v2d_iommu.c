// SPDX-License-Identifier: GPL-2.0
/*
* V2D mmu driver for Ky
* Copyright (C) 2023 Ky Co., Ltd.
*
*/
#include "v2d_priv.h"
#include "v2d_reg.h"

struct v2d_iommu_res sV2dIommuRes;

unsigned long phys_cpu2v2d(unsigned long phys_addr)
{
	if (phys_addr >= 0x100000000UL) {
		phys_addr -= 0x80000000UL;
	}
	return phys_addr;
}

static u32 __read_reg(struct v2d_iommu_res *res, u64 offset)
{
	return readl(res->base + offset + V2D_IOMMU_BASE_OFFSET);
}

static inline void __write_reg(struct v2d_iommu_res *res, u64 offset, u32 data)
{
	writel(data, res->base + offset + V2D_IOMMU_BASE_OFFSET);
}

static void __set_reg_bits(struct v2d_iommu_res *res, u64 offset, u32 bits)
{
	__write_reg(res, offset, (__read_reg(res, offset) | bits));
}

static void __clear_reg_bits(struct v2d_iommu_res *res, u64 offset, u32 bits)
{
	__write_reg(res, offset, (__read_reg(res, offset) & ~bits));
}

static int __enable_ky_iommu_hw(struct v2d_iommu_res *res)
{
	int i;
	struct tbu_instance *tbu;

	if (res->is_hw_enable == false) {
		for (i = 0; i < TBU_INSTANCES_NUM; i++) {
			tbu = &res->tbu_ins[i];
			tbu->ttb_size = 0;
			tbu->always_preload = false;
			tbu->enable_preload = true;
			tbu->nsaid = 0;
			tbu->qos = 2;
			tbu->secure_enable = false;
		}
		res->tbu_ins_map = -1;

		/* Set V2D_MMU iova base */
		__write_reg(res, V2D_MMU_BVA_LO, res->va_base&0xFFFFFFFF);
		__write_reg(res, V2D_MMU_BVA_HI, res->va_base>>32);

		/* Set V2D_MMU timeout cycles */
		__write_reg(res, V2D_MMU_TIMEOUT_VALUE, res->time_out_cycs);

		/* Enable V2D_MMU irq */
		__set_reg_bits(res, V2D_MMU_IRQ_ENABLE, 0x00);

		res->is_hw_enable = true;
	}

	return 0;
}

static void __disable_ky_iommu_hw(struct v2d_iommu_res *res)
{
	int i;
	struct tbu_instance *tbu;

	/* Waiting for post done. */
	res->is_hw_enable = false;
	for (i=0; i<TBU_INSTANCES_NUM; i++) {
		tbu = &res->tbu_ins[i];
		tbu->ttb_size = 0;
	}
	/* Disable all TBUs. */
	for (i = 0; i < TBU_NUM; i++)
		__write_reg(res, V2D_MMU_TCR0_BASE+V2D_MMU_TBUx_STEP*i, 0);

	/* Disable V2D_MMU irq. */
	__clear_reg_bits(res, V2D_MMU_IRQ_ENABLE, 0x1FF);
}

static void __write_tbu_table(struct v2d_iommu_res *res, struct tbu_instance *tbu,
	unsigned long iova, phys_addr_t paddr, size_t size)
{
	u32 *ttb_entry;
	uint64_t mask  = 0;
	uint32_t val;

	mask = (res->page_size == 4096) ? 0xFFFFFFFFFFFFF000 : 0xFFFFFFFFFFFF0000;
	ttb_entry = tbu->ttb_va + (iova - tbu->va_base) / res->page_size;
	while (size != 0) {
		paddr = paddr & 0xFFFFFFFF;
		val = ((paddr & mask) >> TTB_ENTRY_SHIFT) & 0x1FFFFF;
		*ttb_entry = val;
		size -= res->page_size;
		ttb_entry++;
		paddr += res->page_size;
	}
}

void v2d_iommu_map_end(void)
{
	__disable_ky_iommu_hw(&sV2dIommuRes);
}

static void v2d_iommu_post(struct v2d_iommu_res *res, int *ins_id, int num)
{
	u32 reg;
	struct tbu_instance *tbu;
	int i, tbu_slot[TBU_NUM];

	for (i = 0; i < TBU_NUM; i++)
		tbu_slot[i] = -1;

	for (i = 0; i < num; i++) {
		int index;
		tbu = &res->tbu_ins[ins_id[i]];
		index = (tbu->va_base - res->va_base) / VA_STEP_PER_TBU;
		tbu_slot[index] = ins_id[i];
	}

	if (!res->is_hw_enable) {
		return;
	}

	for (i = 0; i < TBU_NUM; i++) {
		if (tbu_slot[i] != -1) {
			tbu = &res->tbu_ins[tbu_slot[i]];
			if (tbu->ttb_size == 0) {
				__write_reg(res, V2D_MMU_TCR0_BASE+i*V2D_MMU_TBUx_STEP, 0);
			} else {
				__write_reg(res, V2D_MMU_TTBLR_BASE+i*V2D_MMU_TBUx_STEP, tbu->ttb_pa & 0xFFFFFFFF);
				__write_reg(res, V2D_MMU_TTBHR_BASE+i*V2D_MMU_TBUx_STEP, tbu->ttb_pa >> 32);

				reg = (tbu->ttb_size - 1) << 16;
				if (tbu->always_preload)
					reg |= BIT(3);
				if (tbu->enable_preload)
					reg |= BIT(2);
				reg |= (tbu->qos << 4);
				if (res->page_size == SZ_64K)
					reg |= BIT(1);
				reg |= BIT(0);
				__write_reg(res, V2D_MMU_TCR0_BASE+i*V2D_MMU_TBUx_STEP, reg);
			}
		}
	}
}

int v2d_iommu_map_sg(unsigned long iova, struct scatterlist *sg, unsigned int nents, int prot)
{
	struct v2d_iommu_res *res = &sV2dIommuRes;
	struct tbu_instance *tbu;
	struct scatterlist *s;
	unsigned int i;
	phys_addr_t paddr;
	size_t size;
	unsigned long orig_iova = iova;

	if ((iova >= res->va_end) && (nents == 1))
		return sg->length;

	__enable_ky_iommu_hw(res);
	res->tbu_ins_map = (iova - BASE_VIRTUAL_ADDRESS) / VA_STEP_PER_TBU;
	pr_debug("tbu ins map:%d\n", res->tbu_ins_map);

	if (res->tbu_ins_map < 0 || res->tbu_ins_map >= TBU_INSTANCES_NUM)
		goto out_id_err;

	tbu = &res->tbu_ins[res->tbu_ins_map];

	if (tbu->ttb_size == 0) {
		int index;
		if (iova < res->va_base || iova >= res->va_end)
			goto out_iova_err;

		index = (iova - res->va_base) / VA_STEP_PER_TBU;
		tbu->va_base = res->va_base + index * VA_STEP_PER_TBU;
		tbu->va_end = tbu->va_base + VA_STEP_PER_TBU;
	}

	if (iova < tbu->va_base || iova >= tbu->va_end)
		goto out_iova_err;

	for_each_sg(sg, s, nents, i) {
		paddr = phys_cpu2v2d(page_to_phys(sg_page(s))) + s->offset;
		size = s->length;
		if (!IS_ALIGNED(s->offset, res->page_size)) {
			pr_err("v2d iommu paddr not aligned: iova %lx, paddr %llx, size %lx\n",
				iova, paddr, size);
			goto out_region_err;
		}

		if (iova+size > tbu->va_end || size == 0)
			goto out_region_err;

		__write_tbu_table(res, tbu, iova, paddr, size);
		iova += size;
	}

	if (iova > tbu->va_base + res->page_size * tbu->ttb_size)
		tbu->ttb_size = (iova - tbu->va_base) / res->page_size;

	v2d_iommu_post(res, &res->tbu_ins_map, 1);

	return (iova - orig_iova);

out_region_err:
	pr_err("v2d map_sg is wrong: iova %lx, paddr %llx, size %lx\n",
			iova, paddr, size);
	return 0;

out_iova_err:
	pr_err("v2d map_sg is wrong: iova %lx", iova);

	return 0;

out_id_err:
	pr_err("v2d tbu ins_id is wrong: %d\n", res->tbu_ins_map);

	return 0;
}

void iommu_irq_reset(void)
{
	u64 last_va, last_pa;
	u32 IRQ_status;
	u32 reg;
	int i;
	struct v2d_iommu_res *res = &sV2dIommuRes;

	IRQ_status = __read_reg(res, V2D_MMU_IRQ_STATUS);

	if (IRQ_status == 0) {
		return;
	}

	reg = __read_reg(res, V2D_MMU_LAST_PA_ADDR_HI);
	last_pa = reg & 0x1;
	reg = __read_reg(res, V2D_MMU_LAST_PA_ADDR_LO);
	last_pa = (last_pa << 32) | reg;
	reg = __read_reg(res, V2D_MMU_LAST_VA_ADDR_HI);
	last_va = reg & 0x1;
	reg = __read_reg(res, V2D_MMU_LAST_VA_ADDR_LO);
	last_va = (last_va << 32) | reg;

	/* Print IRQ status. */
	pr_err("V2d Iommu Unexpected fault: IRQ status 0x%x, last PA 0x%09llx, last VA 0x%09llx\n", IRQ_status, last_pa, last_va);

	if (IRQ_status & BIT(8)) {
		u64 timeout_va_addr;
		reg = __read_reg(res, V2D_MMU_TIMEOUT_VA_ADDR_HI);
		timeout_va_addr = reg & 0x1;
		reg = __read_reg(res, V2D_MMU_TIMEOUT_VA_ADDR_LO);
		timeout_va_addr = (timeout_va_addr << 32) | reg;
		pr_err("v2d iommu timeout error: timeout_va 0x%09llx\n", timeout_va_addr);
	}

	for (i = 0; i < TBU_NUM; i++) {
		if (IRQ_status & BIT(i)) {
			reg = __read_reg(res,
				V2D_MMU_TBU_STATUS_BASE+i*V2D_MMU_TBUx_STEP);
			pr_err("V2d Iommu TBU%d error: read addr 0x%08x, write addr 0x%08x\n",
					i, ((reg >> 16) & 0xFFF), reg &0x1FFF);
		}
	}

	/* clear DMA error */
	if (IRQ_status & 0xFF)
		__set_reg_bits(res, V2D_MMU_ERROR_CLEAR, BIT(1));

	/* reset IRQ status */
	__write_reg(res, V2D_MMU_IRQ_STATUS, IRQ_status);
}

