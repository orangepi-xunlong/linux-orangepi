/*
 * The driver of SUNXI SecuritySystem controller.
 *
 * Copyright (C) 2013 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/highmem.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/rng.h>
#include <crypto/des.h>

#include "../sunxi_ce.h"
#include "../sunxi_ce_proc.h"
#include "sunxi_ce_reg.h"

void ss_task_desc_init(ce_task_desc_t *task, u32 flow)
{
	memset(task, 0, sizeof(ce_task_desc_t));
	task->chan_id = flow;
	task->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
}

static int ss_sg_len(struct scatterlist *sg, int total)
{
	int nbyte = 0;
	struct scatterlist *cur = sg;

	while (cur != NULL) {
		SS_DBG("cur: %p, len: %d, is_last: %ld\n",
			cur, cur->length, sg_is_last(cur));
		nbyte += cur->length;

		cur = sg_next(cur);
	}

	return nbyte;
}

static int ss_aes_align_size(int type, int mode)
{
	if ((type == SS_METHOD_ECC) || CE_METHOD_IS_HMAC(type)
		|| (CE_IS_AES_MODE(type, mode, CTS))
		|| (CE_IS_AES_MODE(type, mode, XTS)))
		return 4;
	else if ((type == SS_METHOD_DES) || (type == SS_METHOD_3DES))
		return DES_BLOCK_SIZE;
	else
		return AES_BLOCK_SIZE;
}

static int ss_copy_from_user(void *to, struct scatterlist *from, u32 size)
{
	void *vaddr = NULL;
	struct page *ppage = sg_page(from);

	vaddr = kmap(ppage);
	if (vaddr == NULL) {
		WARN(1, "Fail to map the last sg 0x%p (%d).\n", from, size);
		return -1;
	}

	SS_DBG("vaddr = 0x%p, sg_addr = 0x%p, size = %d\n", vaddr, from, size);
	memcpy(to, vaddr + from->offset, size);
	kunmap(ppage);
	return 0;
}

static int ss_copy_to_user(struct scatterlist *to, void *from, u32 size)
{
	void *vaddr = NULL;
	struct page *ppage = sg_page(to);

	vaddr = kmap(ppage);
	if (vaddr == NULL) {
		WARN(1, "Fail to map the last sg: 0x%p (%d).\n", to, size);
		return -1;
	}

	SS_DBG("vaddr = 0x%p, sg_addr = 0x%p, size = %d\n", vaddr, to, size);
	memcpy(vaddr+to->offset, from, size);
	kunmap(ppage);
	return 0;
}

static int ss_sg_config(ce_scatter_t *scatter,
	ss_dma_info_t *info, int type, int mode, int tail)
{
	int cnt = 0;
	int last_sg_len = 0;
	struct scatterlist *cur = info->sg;

	while (cur != NULL) {
		if (cnt >= CE_SCATTERS_PER_TASK) {
			WARN(1, "Too many scatter: %d\n", cnt);
			return -1;
		}
		info->mapping[cnt].virt_addr = (void *)sg_dma_address(cur);
		scatter[cnt].addr = (sg_dma_address(cur) >> WORD_ALGIN) & 0xffffffff;
		scatter[cnt].len = sg_dma_len(cur) >> 2;
		info->last_sg = cur;
		last_sg_len = sg_dma_len(cur);
		SS_DBG("%d cur: 0x%p, scatter: addr 0x%x, len %d (%d)\n",
				cnt, cur, (scatter[cnt].addr << WORD_ALGIN),
				scatter[cnt].len, sg_dma_len(cur));
		cnt++;
		cur = sg_next(cur);
	}

#ifdef SS_HASH_HW_PADDING
	if (CE_METHOD_IS_HMAC(type)) {
		scatter[cnt-1].len += (tail+3) >> 2;
		info->has_padding = 0;
		return 0;
	}
#endif

	info->nents = cnt;
	if (tail == 0) {
		info->has_padding = 0;
		return 0;
	}

	if (CE_METHOD_IS_HASH(type)) {
		scatter[cnt-1].len -= tail >> 2;
		return 0;
	}

	/* CTS/CTR/CFB/OFB need algin with word/block, so replace the last sg.*/

	last_sg_len += ss_aes_align_size(0, mode) - tail;
	info->padding = kzalloc(last_sg_len, GFP_KERNEL);
	if (info->padding == NULL) {
		SS_ERR("Failed to kmalloc(%d)!\n", last_sg_len);
		return -ENOMEM;
	}
	SS_DBG("AES(%d)-%d padding: 0x%p, tail = %d/%d, cnt = %d\n",
		type, mode, info->padding, tail, last_sg_len, cnt);
	info->mapping[cnt - 1].virt_addr = info->padding;
	scatter[cnt-1].addr = (virt_to_phys(info->padding) >> WORD_ALGIN) & 0xffffffff;
	ss_copy_from_user(info->padding,
		info->last_sg, last_sg_len - ss_aes_align_size(0, mode) + tail);
	scatter[cnt-1].len = last_sg_len >> 2;

	info->has_padding = 1;
	return 0;
}

static void ss_aes_unpadding(ce_scatter_t *scatter,
	ss_dma_info_t *info, int mode, int tail)
{
	int last_sg_len = 0;
	int index = info->nents - 1;

	if (info->has_padding == 0)
		return;

	/* Only the dst sg need to be recovered. */
	if (info->dir == DMA_DEV_TO_MEM) {
		last_sg_len = scatter[index].len * 4;
		last_sg_len -= ss_aes_align_size(0, mode) - tail;
		ss_copy_to_user(info->last_sg, info->padding, last_sg_len);
	}

	kfree(info->padding);
	info->padding = NULL;
	info->has_padding = 0;
}

static void ss_aes_map_padding(ce_scatter_t *scatter,
	ss_dma_info_t *info, int mode, int dir)
{
	int len = 0;
	int index = info->nents - 1;

	if (info->has_padding == 0)
		return;

	len = scatter[index].len * 4;
	SS_DBG("AES padding: 0x%p, len: %d, dir: %d\n",
		info->mapping[index].virt_addr, len, dir);
	dma_map_single(&ss_dev->pdev->dev, info->mapping[index].virt_addr, len, dir);
	info->dir = dir;
}

static void ss_aes_unmap_padding(ce_scatter_t *scatter,
	ss_dma_info_t *info, int mode, int dir)
{
	int len = 0;
	int index = info->nents - 1;

	if (info->has_padding == 0)
		return;

	len = scatter[index].len * 4;
	SS_DBG("AES padding: 0x%x, len: %d, dir: %d\n",
		scatter[index].addr, len, dir);
	dma_unmap_single(&ss_dev->pdev->dev, scatter[index].addr, len, dir);
}

void ss_change_clk(int type)
{
#ifdef SS_RSA_CLK_ENABLE
	if ((type == SS_METHOD_RSA) || (type == SS_METHOD_ECC))
		ss_clk_set(ss_dev->rsa_clkrate);
	else
		ss_clk_set(ss_dev->gen_clkrate);
#endif
}
#ifdef CE_USE_WALK
void ss_task_chan_init(ce_task_desc_t *task, u32 flow)
{
	task->chan_id = flow;
}

static phys_addr_t ss_task_iv_init(struct ablkcipher_walk *walk, struct ablkcipher_request *req, ce_task_desc_t *task)
{
	ce_task_desc_t *p_task;
	phys_addr_t nextiv_phys = 0;
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	ss_aes_ctx_t *ctx =	crypto_ablkcipher_ctx(tfm);
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);
	u32 iv_size = crypto_ablkcipher_ivsize(tfm);

	SS_DBG("iv len =%d\n", crypto_ablkcipher_ivsize(tfm));
	if (iv_size) {
		dma_map_single(&ss_dev->pdev->dev, req->info, iv_size, DMA_BIDIRECTIONAL);
		dma_map_single(&ss_dev->pdev->dev, ctx->next_iv, iv_size, DMA_BIDIRECTIONAL);
		nextiv_phys = virt_to_phys(req->info);
		SS_DBG("walk_iv = 0x%p req_info = 0x%p\n", (void *)nextiv_phys, (void *)virt_to_phys(req->info));
		ss_printf_hex(req->info, iv_size, req->info);
		ss_iv_set(req->info, iv_size, task);
	}

	for (p_task = task; p_task != NULL; p_task = p_task->next) {
		if (p_task != task) {
			p_task->chan_id  = task->chan_id;
			p_task->comm_ctl = task->comm_ctl;
			p_task->sym_ctl  = task->sym_ctl;
			p_task->asym_ctl = task->asym_ctl;
			p_task->key_addr = task->key_addr;
		}

		if (iv_size) {
			if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CTR)) {
				SS_ERR("ctr\n");
				if (p_task != task) {
					ss_iv_set(req->info, iv_size, p_task);
				}
				ss_cnt_set(req->info, iv_size, p_task);
			} else if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CBC)) {
				ss_iv_set_phys(nextiv_phys, iv_size, p_task);
				if (req_ctx->dir == SS_DIR_ENCRYPT) {
					nextiv_phys = (p_task->dst_phys[p_task->dst_cnt - 1].addr) +
						(p_task->dst[p_task->dst_cnt - 1].len << 2) - iv_size;
				} else {
					nextiv_phys = (p_task->src_phys[p_task->src_cnt - 1].addr) +
					(p_task->src[p_task->src_cnt - 1].len << 2) - iv_size;
				}
			}
		}

		/*The last entry should enable interrupt.*/
		if (p_task->next == NULL) {
			if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CTS)) {
				ss_cts_last(p_task);
			} else if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, XTS)) {
				ss_xts_first(p_task);
				ss_xts_last(p_task);
			}
			p_task->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
		}

		SS_DBG("Task addr, vir = 0x%p, phy = 0x%lx\n", p_task, p_task->pthis);
		SS_DBG("preCE, COMM: 0x%08x, SYM: 0x%08x, ASYM: 0x%08x,"
				"key_addr: 0x%p, iv_addr: 0x%p\n",
				p_task->comm_ctl, p_task->sym_ctl, p_task->asym_ctl,
				(void *)p_task->key_addr, (void *)p_task->iv_addr);
		SS_DBG("preCE, data_len:%d\n", p_task->data_len);
	}
	return nextiv_phys;
}

static void ss_task_data_len_set(struct ablkcipher_request *req,
									ce_task_desc_t *task)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);
	u32 word_len = 0;

	if (CE_METHOD_IS_AES(req_ctx->type)) {
		if (task->data_len & (AES_BLOCK_SIZE - 1)) {
				WARN(1, "data len not aes block align: %d\n", task->data_len);
		}
	} else if (CE_METHOD_IS_DES(req_ctx->type)) {
		if (task->data_len & (DES_BLOCK_SIZE - 1)) {
				WARN(1, "data len not block align: %d\n", task->data_len);
		}
	}

#ifdef SS_SUPPORT_CE_V3_1
	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CTS)) {
		;
	} else {
		word_len = task->data_len >> 2;
		ss_data_len_set(word_len, task);
	}
#else
	if (CE_METHOD_IS_HMAC(req_ctx->type)) {
		ss_data_len_set(task->data_len * 8, task);
		task->ctr_addr = task->key_addr;
		task->reserved[0] = task->data_len * 8;
		task->key_addr = (virt_to_phys(&task->reserved[0]) >> WORD_ALGIN) & 0xffffffff;
	} else if (req_ctx->type == SS_METHOD_RSA) {
		ss_data_len_set(task->data_len * 3, task);
	}
#endif
	SS_ERR("task->data_len =%d\n", task->data_len);
	return;
}

static void ss_task_destroy(ce_task_desc_t *task)
{
	ce_task_desc_t *prev;

	while (task != NULL) {
		prev = task;
		task = task->next;
		dma_pool_free(ss_dev->task_pool, prev, prev->pthis);
	}
	return;
}

static ce_task_desc_t *ss_task_create(struct ablkcipher_walk *walk,
					struct ablkcipher_request *req)
{
	int err, nbytes;
	ce_task_desc_t *first = NULL, *prev = NULL, *task;

	dma_map_sg(&ss_dev->pdev->dev, req->src,
			sg_nents(req->src), DMA_TO_DEVICE);

	dma_map_sg(&ss_dev->pdev->dev, req->dst,
			sg_nents(req->dst), DMA_FROM_DEVICE);

	ablkcipher_walk_init(walk, req->dst, req->src, req->nbytes);
	err = ablkcipher_walk_phys(req, walk);
	if (err)
		return NULL;

	while (walk->nbytes) {

		dma_addr_t ptask = 0;

		task = dma_pool_alloc(ss_dev->task_pool, GFP_KERNEL, &ptask);
		if (IS_ERR_OR_NULL(task))
			goto err;

		/*
		 * Clear the dirty data of new task
		 */
		memset(task, 0, sizeof(ce_task_desc_t));
		task->pthis = ptask;
		task->next  = NULL;
		SS_DBG("Task addr, vir = 0x%p, phy = 0x%p\n", task, (void *)task->pthis);
		if (prev != NULL) {
			prev->pnext = (u32)task->pthis;
			prev->next  = task;
		} else {
			first = task;
		}

		while ((nbytes = walk->nbytes) != 0) {
			phys_addr_t dst_paddr, src_paddr;
			SS_ERR("#nbytes = %d\n", nbytes);
			if (nbytes & 0x3) {
				SS_ERR("#nbytes = %d\n", nbytes);
				goto err;
			}
			src_paddr = (page_to_phys(walk->src.page) +
					walk->src.offset);
			dst_paddr = (page_to_phys(walk->dst.page) +
					walk->dst.offset);

			task->src_phys[task->src_cnt].addr = src_paddr;
			task->dst_phys[task->dst_cnt].addr = dst_paddr;

			task->src[task->src_cnt].addr = (src_paddr >> WORD_ALGIN) & 0xffffffff;
			task->src[task->src_cnt].len = nbytes >> 2;
			SS_DBG("task->src[%d] = 0x%lx, len = %d\n", task->src_cnt,
					task->src[task->src_cnt].addr,
					task->src[task->src_cnt].len << 2);

			task->src_cnt++;
			task->dst[task->dst_cnt].addr = (dst_paddr >> WORD_ALGIN) & 0xffffffff;
			task->dst[task->dst_cnt].len = nbytes >> 2;
			SS_DBG("task->dst[%d] = 0x%lx, len = %d\n", task->dst_cnt,
					task->dst[task->dst_cnt].addr,
					task->dst[task->dst_cnt].len << 2);

			task->dst_cnt++;
			task->data_len += nbytes;
			err = ablkcipher_walk_done(req, walk, 0);
			if (err) {
				break;
			}
			if ((task->src_cnt >= CE_SCATTERS_PER_TASK)
				|| (task->dst_cnt >= CE_SCATTERS_PER_TASK))
				goto out;
		}

out:
		ss_task_data_len_set(req, task);
		prev = task;
	}

	return first;
err:
	if (first != NULL)
		ss_task_destroy(first);

	return NULL;
}

static int ss_aes_start_by_walk(struct ablkcipher_request *req)
{
	int ret = 0;
	phys_addr_t nextiv_phys = 0;
	u32 len = req->nbytes;
	int align_size = 0;
	phys_addr_t phy_addr = 0;
	ce_task_desc_t *task = NULL;
	void *nextiv_virt = NULL;
	struct ablkcipher_walk walk;
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);
	ss_aes_ctx_t *ctx = crypto_ablkcipher_ctx(tfm);
	u32 flow = ctx->comm.flow;
	u32 iv_size = crypto_ablkcipher_ivsize(tfm);

	ss_change_clk(req_ctx->type);

	task = ss_task_create(&walk, req);
	if (task == NULL)
		return -ENOMEM;

	/*ss_dev->flows[ctx->comm.flow].task = task;*/
	/*configure the first entry*/
	ss_task_chan_init(task, flow);
#ifdef SS_XTS_MODE_ENABLE
	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, XTS))
		ss_method_set(req_ctx->dir, SS_METHOD_RAES, task);
	else
#endif
	ss_method_set(req_ctx->dir, req_ctx->type, task);

	ss_aes_mode_set(req_ctx->mode, task);

#ifdef SS_CFB_MODE_ENABLE
	if (CE_METHOD_IS_AES(req_ctx->type)
		&& (req_ctx->mode == SS_AES_MODE_CFB))
		ss_cfb_bitwidth_set(req_ctx->bitwidth, task);
#endif

	//phy_addr = virt_to_phys(ctx->key);
	//SS_DBG("ctx->key addr, vir = 0x%p, phy = 0x%lx\n", ctx->key, phy_addr);

#ifdef SS_XTS_MODE_ENABLE
	SS_DBG("The current Key:\n");
	ss_print_hex(ctx->key, ctx->key_size, ctx->key);

	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, XTS))
		ss_key_set(ctx->key, ctx->key_size/2, task);
	else
#endif
	ss_key_set(ctx->key, ctx->key_size, task);
	ctx->comm.flags &= ~SS_FLAG_NEW_KEY;
	dma_map_single(&ss_dev->pdev->dev,
		ctx->key, ctx->key_size, DMA_TO_DEVICE);

	nextiv_phys = ss_task_iv_init(&walk, req, task);
	ss_pending_clear(flow);
	ss_irq_enable(flow);

	/* Start CE controller. */
	init_completion(&ss_dev->flows[flow].done);

	ss_ctrl_start(task);
	SS_ERR("After CE, TSK: 0x%08x, ICR: 0x%08x TLR: 0x%08x\n",
	ss_reg_rd(CE_REG_TSK), ss_reg_rd(CE_REG_ICR), ss_reg_rd(CE_REG_TLR));
	ret = wait_for_completion_timeout(&ss_dev->flows[flow].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ss_reset();
		ret = -ETIMEDOUT;
	}
	SS_DBG("After CE, TSK: 0x%08x, ICR: 0x%08x ,ISR: 0x%08x TLR: 0x%08x\n",
			ss_reg_rd(CE_REG_TSK), ss_reg_rd(CE_REG_ICR), ss_reg_rd(CE_REG_ISR), ss_reg_rd(CE_REG_TLR));

	ss_irq_disable(flow);

	ablkcipher_walk_complete(&walk);

	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(ctx->key),
			ctx->key_size, DMA_MEM_TO_DEV);

	dma_unmap_sg(&ss_dev->pdev->dev, req->src,
			sg_nents(req->src), DMA_TO_DEVICE);

	dma_unmap_sg(&ss_dev->pdev->dev, req->dst,
			sg_nents(req->dst), DMA_FROM_DEVICE);

	/*Backup the next IV*/
	if (iv_size) {
		dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(req->info),
							iv_size, DMA_BIDIRECTIONAL);
		/*ctr already in req->info*/
		if (CE_METHOD_IS_AES(req_ctx->type)
			&& (req_ctx->mode != SS_AES_MODE_CTR)) {
			nextiv_virt = kmap(phys_to_page(nextiv_phys)) + offset_in_page(nextiv_phys);
			SS_DBG("nextiv_phys = 0x%lx virt = 0x%p\n", nextiv_phys, nextiv_virt);
			memcpy(req->info, nextiv_virt, iv_size);
			ss_printf_hex(req->info, iv_size, req->info);
			kunmap(virt_to_page(nextiv_virt));
		}
	}

	ss_task_destroy(task);

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
		ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));
	if (ss_flow_err(flow)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(flow));
		return -EINVAL;
	}

	return 0;
}
#endif

static int ss_aes_start(ss_aes_ctx_t *ctx, ss_aes_req_ctx_t *req_ctx, int len)
{
	int ret = 0;
	int src_len = len;
	int align_size = 0;
	u32 flow = ctx->comm.flow;
	phys_addr_t phy_addr = 0;
	ce_task_desc_t *task = &ss_dev->flows[flow].task;

	ss_change_clk(req_ctx->type);
	ss_task_desc_init(task, flow);

	ss_pending_clear(flow);
	ss_irq_enable(flow);

#ifdef SS_XTS_MODE_ENABLE
	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, XTS))
		ss_method_set(req_ctx->dir, SS_METHOD_RAES, task);
	else
#endif
	ss_method_set(req_ctx->dir, req_ctx->type, task);

	if ((req_ctx->type == SS_METHOD_RSA)
		|| (req_ctx->type == SS_METHOD_DH)) {
#ifdef SS_SUPPORT_CE_V3_1
		if (req_ctx->mode == CE_RSA_OP_M_MUL)
			ss_rsa_width_set(ctx->iv_size, task);
		else
			ss_rsa_width_set(ctx->key_size, task);
#else
		ss_rsa_width_set(len, task);
#endif
		ss_rsa_op_mode_set(req_ctx->mode, task);
	} else if (req_ctx->type == SS_METHOD_ECC) {
#ifdef SS_SUPPORT_CE_V3_1
		ss_ecc_width_set(ctx->key_size, task);
#else
		ss_ecc_width_set(len>>1, task);
#endif
		ss_ecc_op_mode_set(req_ctx->mode, task);
	} else if (CE_METHOD_IS_HMAC(req_ctx->type))
		ss_hmac_sha1_last(task);
	else
		ss_aes_mode_set(req_ctx->mode, task);

#ifdef SS_CFB_MODE_ENABLE
	if (CE_METHOD_IS_AES(req_ctx->type)
		&& (req_ctx->mode == SS_AES_MODE_CFB))
		ss_cfb_bitwidth_set(req_ctx->bitwidth, task);
#endif

	SS_ERR("Flow: %d, Dir: %d, Method: %d, Mode: %d, len: %d\n", flow,
		req_ctx->dir, req_ctx->type, req_ctx->mode, len);

	phy_addr = virt_to_phys(ctx->key);
	SS_DBG("ctx->key addr, vir = 0x%p, phy = 0x%p\n", ctx->key, (void *)phy_addr);
	phy_addr = virt_to_phys(task);
	SS_DBG("Task addr, vir = 0x%p, phy = 0x%p\n", task, (void *)phy_addr);

#ifdef SS_XTS_MODE_ENABLE
	SS_DBG("The current Key:\n");
	ss_print_hex(ctx->key, ctx->key_size, ctx->key);

	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, XTS))
		ss_key_set(ctx->key, ctx->key_size/2, task);
	else
#endif
	ss_key_set(ctx->key, ctx->key_size, task);
	ctx->comm.flags &= ~SS_FLAG_NEW_KEY;
	dma_map_single(&ss_dev->pdev->dev,
		ctx->key, ctx->key_size, DMA_MEM_TO_DEV);

	if (ctx->iv_size > 0) {
		phy_addr = virt_to_phys(ctx->iv);
		SS_DBG("ctx->iv vir = 0x%p phy = 0x%p\n", ctx->iv, (void *)phy_addr);
		ss_iv_set(ctx->iv, ctx->iv_size, task);
		dma_map_single(&ss_dev->pdev->dev,
			ctx->iv, ctx->iv_size, DMA_MEM_TO_DEV);

		phy_addr = virt_to_phys(ctx->next_iv);
		SS_DBG("ctx->next_iv addr, vir = 0x%p, phy = 0x%p\n",
			ctx->next_iv, (void *)phy_addr);
		ss_cnt_set(ctx->next_iv, ctx->iv_size, task);
		dma_map_single(&ss_dev->pdev->dev,
			ctx->next_iv, ctx->iv_size, DMA_DEV_TO_MEM);
	}

	align_size = ss_aes_align_size(req_ctx->type, req_ctx->mode);

	/* Prepare the src scatterlist */
	req_ctx->dma_src.nents = ss_sg_cnt(req_ctx->dma_src.sg, src_len);
	if ((req_ctx->type == SS_METHOD_ECC)
		|| CE_METHOD_IS_HMAC(req_ctx->type)
		|| ((req_ctx->type == SS_METHOD_RSA) &&
			(req_ctx->mode == CE_RSA_OP_M_MUL)))
		src_len = ss_sg_len(req_ctx->dma_src.sg, len);

	dma_map_sg(&ss_dev->pdev->dev,
		req_ctx->dma_src.sg, req_ctx->dma_src.nents, DMA_MEM_TO_DEV);
	ss_sg_config(task->src,	&req_ctx->dma_src,
		req_ctx->type, req_ctx->mode, src_len%align_size);
	ss_aes_map_padding(task->src,
		&req_ctx->dma_src, req_ctx->mode, DMA_MEM_TO_DEV);

	/* Prepare the dst scatterlist */
	req_ctx->dma_dst.nents = ss_sg_cnt(req_ctx->dma_dst.sg, len);
	dma_map_sg(&ss_dev->pdev->dev,
		req_ctx->dma_dst.sg, req_ctx->dma_dst.nents, DMA_DEV_TO_MEM);
	ss_sg_config(task->dst,	&req_ctx->dma_dst,
		req_ctx->type, req_ctx->mode, len%align_size);
	ss_aes_map_padding(task->dst,
		&req_ctx->dma_dst, req_ctx->mode, DMA_DEV_TO_MEM);

#ifdef SS_SUPPORT_CE_V3_1
	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CTS)) {
		ss_data_len_set(len, task);
/*if (len < SZ_4K)  A bad way to determin the last packet of CTS mode. */
			ss_cts_last(task);
	} else
		ss_data_len_set(
			DIV_ROUND_UP(src_len, align_size)*align_size/4, task);
#else
	if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CTS)) {
		/* A bad way to determin the last packet. */
		/* if (len < SZ_4K) */
		ss_cts_last(task);
		ss_data_len_set(src_len, task);
	} else if (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, XTS)) {
		ss_xts_first(task);
		ss_xts_last(task);
		ss_data_len_set(src_len, task);
	} else if (CE_METHOD_IS_HMAC(req_ctx->type)) {
		ss_data_len_set(src_len * 8, task);
		task->ctr_addr = task->key_addr;
		task->reserved[0] = src_len * 8;
		task->key_addr = (virt_to_phys(&task->reserved[0]) >> WORD_ALGIN) & 0xffffffff;
	} else if (req_ctx->type == SS_METHOD_RSA)
		ss_data_len_set(len*3, task);
	else
		ss_data_len_set(DIV_ROUND_UP(src_len, align_size)*align_size,
			task);
#endif

	/* Start CE controller. */
	init_completion(&ss_dev->flows[flow].done);
	dma_map_single(&ss_dev->pdev->dev, task, sizeof(ce_task_desc_t),
		DMA_MEM_TO_DEV);

	SS_ERR("preCE, COMM: 0x%08x, SYM: 0x%08x, ASYM: 0x%08x, data_len:%d\n",
		task->comm_ctl, task->sym_ctl, task->asym_ctl, task->data_len);
	/*ss_print_task_info(task);*/
	ss_ctrl_start(task);

	ret = wait_for_completion_timeout(&ss_dev->flows[flow].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ss_reset();
		ret = -ETIMEDOUT;
	}
	ss_irq_disable(flow);
	/*ss_print_task_info(task);*/
	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(task),
		sizeof(ce_task_desc_t), DMA_MEM_TO_DEV);

	/* Unpadding and unmap the dst sg. */
	ss_aes_unpadding(task->dst,
		&req_ctx->dma_dst, req_ctx->mode, len%align_size);
	ss_aes_unmap_padding(task->dst,
		&req_ctx->dma_dst, req_ctx->mode, DMA_DEV_TO_MEM);
	dma_unmap_sg(&ss_dev->pdev->dev,
		req_ctx->dma_dst.sg, req_ctx->dma_dst.nents, DMA_DEV_TO_MEM);

	/* Unpadding and unmap the src sg. */
	ss_aes_unpadding(task->src,
		&req_ctx->dma_src, req_ctx->mode, src_len%align_size);
	ss_aes_unmap_padding(task->src,
		&req_ctx->dma_src, req_ctx->mode, DMA_MEM_TO_DEV);
	dma_unmap_sg(&ss_dev->pdev->dev,
		req_ctx->dma_src.sg, req_ctx->dma_src.nents, DMA_MEM_TO_DEV);

	if (ctx->iv_size > 0) {
		dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(ctx->iv),
			ctx->iv_size, DMA_MEM_TO_DEV);
		dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(ctx->next_iv),
			ctx->iv_size, DMA_DEV_TO_MEM);
	}
	/* Backup the next IV from ctr_descriptor, except CBC/CTS/XTS mode. */
	if (CE_METHOD_IS_AES(req_ctx->type)
		&& (req_ctx->mode != SS_AES_MODE_CBC)
		&& (req_ctx->mode != SS_AES_MODE_CTS)
		&& (req_ctx->mode != SS_AES_MODE_XTS))
		memcpy(ctx->iv, ctx->next_iv, ctx->iv_size);

	dma_unmap_single(&ss_dev->pdev->dev,
		virt_to_phys(ctx->key), ctx->key_size, DMA_MEM_TO_DEV);

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
		ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));
	if (ss_flow_err(flow)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(flow));
		return -EINVAL;
	}

	return 0;
}

/* verify the key_len */
int ss_aes_key_valid(struct crypto_ablkcipher *tfm, int len)
{
	if (unlikely(len > SS_RSA_MAX_SIZE)) {
		SS_ERR("Unsupported key size: %d\n", len);
		tfm->base.crt_flags |= CRYPTO_TFM_RES_BAD_KEY_LEN;
		return -EINVAL;
	}
	return 0;
}

#ifdef SS_RSA_PREPROCESS_ENABLE
static void ss_rsa_preprocess(ss_aes_ctx_t *ctx,
	ss_aes_req_ctx_t *req_ctx, int len)
{
	struct scatterlist sg = {0};
	ss_aes_req_ctx_t *tmp_req_ctx = NULL;

	if (!((req_ctx->type == SS_METHOD_RSA) &&
		(req_ctx->mode != CE_RSA_OP_M_MUL)))
		return;

	tmp_req_ctx = kmalloc(sizeof(ss_aes_req_ctx_t), GFP_KERNEL);
	if (tmp_req_ctx == NULL) {
		SS_ERR("Failed to malloc(%d)\n", sizeof(ss_aes_req_ctx_t));
		return;
	}

	memcpy(tmp_req_ctx, req_ctx, sizeof(ss_aes_req_ctx_t));
	tmp_req_ctx->mode = CE_RSA_OP_M_MUL;

	sg_init_one(&sg, ctx->key, ctx->iv_size*2);
	tmp_req_ctx->dma_src.sg = &sg;

	ss_aes_start(ctx, tmp_req_ctx, len);

	SS_DBG("The preporcess of RSA complete!\n\n");
	kfree(tmp_req_ctx);
}
#endif

static int ss_rng_start(ss_aes_ctx_t *ctx, u8 *rdata, u32 dlen, u32 trng)
{
	int ret = 0;
	int flow = ctx->comm.flow;
	int rng_len = 0;
	char *buf = NULL;
	phys_addr_t phy_addr = 0;
	ce_task_desc_t *task = &ss_dev->flows[flow].task;

	if (trng)
		rng_len = DIV_ROUND_UP(dlen, 32)*32; /* align with 32 Bytes */
	else
		rng_len = DIV_ROUND_UP(dlen, 20)*20; /* align with 20 Bytes */
	if (rng_len > SS_RNG_MAX_LEN) {
		SS_ERR("The RNG length is too large: %d\n", rng_len);
		rng_len = SS_RNG_MAX_LEN;
	}

	buf = kmalloc(rng_len, GFP_KERNEL);
	if (buf == NULL) {
		SS_ERR("Failed to malloc(%d)\n", rng_len);
		return -ENOMEM;
	}

	ss_change_clk(SS_METHOD_PRNG);

	ss_task_desc_init(task, flow);

	ss_pending_clear(flow);
	ss_irq_enable(flow);

	if (trng)
		ss_method_set(SS_DIR_ENCRYPT, SS_METHOD_TRNG, task);
	else
		ss_method_set(SS_DIR_ENCRYPT, SS_METHOD_PRNG, task);

	phy_addr = virt_to_phys(ctx->key);
	SS_DBG("ctx->key addr, vir = 0x%p, phy = 0x%p\n", ctx->key, (void *)phy_addr);

	if (trng == 0) {
		/* Must set the seed addr in PRNG. */
		ss_key_set(ctx->key, ctx->key_size, task);
		ctx->comm.flags &= ~SS_FLAG_NEW_KEY;
		dma_map_single(&ss_dev->pdev->dev,
			ctx->key, ctx->key_size, DMA_MEM_TO_DEV);
	}
	phy_addr = virt_to_phys(buf);
	SS_DBG("buf addr, vir = 0x%p, phy = 0x%p\n", buf, (void *)phy_addr);

	/* Prepare the dst scatterlist */
	task->dst[0].addr = (virt_to_phys(buf) >> WORD_ALGIN) & 0xffffffff;
	task->dst[0].len  = rng_len >> 2;
	dma_map_single(&ss_dev->pdev->dev, buf, rng_len, DMA_DEV_TO_MEM);
	SS_DBG("task->dst_addr = 0x%x\n", task->dst[0].addr);
#ifdef SS_SUPPORT_CE_V3_1
	ss_data_len_set(rng_len/4, task);
#else
	ss_data_len_set(rng_len, task);
#endif

	SS_DBG("Flow: %d, Request: %d, Aligned: %d\n", flow, dlen, rng_len);

	phy_addr = virt_to_phys(task);
	SS_DBG("Task addr, vir = 0x%p, phy = 0x%p\n", task, (void *)phy_addr);

	/* Start CE controller. */
	init_completion(&ss_dev->flows[flow].done);
	dma_map_single(&ss_dev->pdev->dev, task,
		sizeof(ce_task_desc_t), DMA_MEM_TO_DEV);

	ss_ctrl_start(task);
	SS_DBG("Before CE, COMM_CTL: 0x%08x, TSK: 0x%08x ICR: 0x%08x TLR: 0x%08x\n",
		task->comm_ctl, ss_reg_rd(CE_REG_TSK), ss_reg_rd(CE_REG_ICR), ss_reg_rd(CE_REG_TLR));
	ret = wait_for_completion_timeout(&ss_dev->flows[flow].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ss_reset();
		ret = -ETIMEDOUT;
	}
	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
		ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));
	SS_DBG("After CE, dst data:\n");

	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(task),
		sizeof(ce_task_desc_t), DMA_MEM_TO_DEV);
	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(buf),
		rng_len, DMA_DEV_TO_MEM);
	if (trng == 0)
		dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(ctx->key),
			ctx->key_size, DMA_MEM_TO_DEV);
	memcpy(rdata, buf, dlen);
	ss_irq_disable(flow);
	ret = dlen;

	return ret;
}

int ss_rng_get_random(struct crypto_rng *tfm, u8 *rdata, u32 dlen, u32 trng)
{
	int ret = 0;
	u8 *data = rdata;
	u32 len = dlen;
	ss_aes_ctx_t *ctx = crypto_rng_ctx(tfm);

	SS_DBG("flow = %d, data = %p, len = %d, trng = %d\n",
		ctx->comm.flow, data, len, trng);
	if (ss_dev->suspend) {
		SS_ERR("SS has already suspend.\n");
		return -EAGAIN;
	}

#ifdef SS_TRNG_POSTPROCESS_ENABLE
	if (trng) {
		len = DIV_ROUND_UP(dlen, SHA256_DIGEST_SIZE)*SHA256_BLOCK_SIZE;
		data = kzalloc(len, GFP_KERNEL);
		if (data == NULL) {
			SS_ERR("Failed to malloc(%d)\n", len);
			return -ENOMEM;
		}
		SS_DBG("In fact, flow = %d, data = %p, len = %d\n",
			ctx->comm.flow, data, len);
	}
#endif

	ss_dev_lock();
	ret = ss_rng_start(ctx, data, len, trng);
	ss_dev_unlock();

	SS_DBG("Get %d byte random.\n", ret);

#ifdef SS_TRNG_POSTPROCESS_ENABLE
	if (trng) {
		ss_trng_postprocess(rdata, dlen, data, len);
		ret = dlen;
		kfree(data);
	}
#endif

	return ret;
}

u32 ss_hash_start(ss_hash_ctx_t *ctx,
		ss_aes_req_ctx_t *req_ctx, u32 len, u32 last)
{
	int ret = 0;
	int flow = ctx->comm.flow;
	u32 blk_size = ss_hash_blk_size(req_ctx->type);
	char *digest = NULL;
	phys_addr_t phy_addr = 0;
	ce_task_desc_t *task = &ss_dev->flows[flow].task;

	/* Total len is too small, so process it in the padding data later. */
	if ((last == 0) && (len > 0) && (len < blk_size)) {
		ctx->cnt += len;
		return 0;
	}
	ss_change_clk(req_ctx->type);

	digest = kzalloc(SHA512_DIGEST_SIZE, GFP_KERNEL);
	if (digest == NULL) {
		SS_ERR("Failed to kmalloc(%d)\n", SHA512_DIGEST_SIZE);
		return -ENOMEM;
	}

	ss_task_desc_init(task, flow);

	ss_pending_clear(flow);
	ss_irq_enable(flow);

	ss_method_set(req_ctx->dir, req_ctx->type, task);

	SS_DBG("Flow: %d, Dir: %d, Method: %d, Mode: %d, len: %d / %d\n", flow,
		req_ctx->dir, req_ctx->type, req_ctx->mode, len, ctx->cnt);
	SS_DBG("IV address = 0x%p, size = %d\n", ctx->md, ctx->md_size);
	phy_addr = virt_to_phys(task);
	SS_DBG("Task addr, vir = 0x%p, phy = 0x%p\n", task, (void *)phy_addr);

	ss_iv_set(ctx->md, ctx->md_size, task);
	ss_iv_mode_set(CE_HASH_IV_INPUT, task);
	dma_map_single(&ss_dev->pdev->dev,
		ctx->md, ctx->md_size, DMA_MEM_TO_DEV);

#ifdef SS_SUPPORT_CE_V3_1
	ss_data_len_set((len - len%blk_size)/4, task);
#else
	if (last == 1) {
		ss_hmac_sha1_last(task);
		ss_data_len_set(ctx->tail_len*8, task);
	} else
		ss_data_len_set((len - len%blk_size)*8, task);
#endif

	/* Prepare the src scatterlist */
	req_ctx->dma_src.nents = ss_sg_cnt(req_ctx->dma_src.sg, len);
	dma_map_sg(&ss_dev->pdev->dev, req_ctx->dma_src.sg,
		req_ctx->dma_src.nents, DMA_MEM_TO_DEV);
	ss_sg_config(task->src,
		&req_ctx->dma_src, req_ctx->type, 0, len%blk_size);

#ifdef SS_HASH_HW_PADDING
	if (last == 1) {
		task->src[0].len = (ctx->tail_len + 3)/4;
		SS_DBG("cnt %d, tail_len %d.\n", ctx->cnt, ctx->tail_len);
		ctx->cnt <<= 3; /* Translate to bits in the last pakcket */
		dma_map_single(&ss_dev->pdev->dev, &ctx->cnt, 4,
			DMA_MEM_TO_DEV);
		task->key_addr = (virt_to_phys(&ctx->cnt) >> WORD_ALGIN) & 0xffffffff;
	}
#endif

	/* Prepare the dst scatterlist */
	task->dst[0].addr = (virt_to_phys(digest) >> WORD_ALGIN) & 0xffffffff;
	task->dst[0].len  = ctx->md_size  >> 2;
	dma_map_single(&ss_dev->pdev->dev,
		digest, SHA512_DIGEST_SIZE, DMA_DEV_TO_MEM);
	phy_addr = virt_to_phys(digest);
	SS_DBG("digest addr, vir = 0x%p, phy = 0x%p\n", digest, (void *)phy_addr);

	/* Start CE controller. */
	init_completion(&ss_dev->flows[flow].done);
	dma_map_single(&ss_dev->pdev->dev, task,
		sizeof(ce_task_desc_t), DMA_MEM_TO_DEV);

	SS_DBG("Before CE, COMM_CTL: 0x%08x, ICR: 0x%08x\n",
		task->comm_ctl, ss_reg_rd(CE_REG_ICR));
	ss_ctrl_start(task);

	ret = wait_for_completion_timeout(&ss_dev->flows[flow].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ss_reset();
		ret = -ETIMEDOUT;
	}
	ss_irq_disable(flow);

	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(task),
		sizeof(ce_task_desc_t), DMA_MEM_TO_DEV);
	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(digest),
		SHA512_DIGEST_SIZE, DMA_DEV_TO_MEM);
	dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(ctx->md),
		ctx->md_size, DMA_MEM_TO_DEV);
	dma_unmap_sg(&ss_dev->pdev->dev, req_ctx->dma_src.sg,
		req_ctx->dma_src.nents, DMA_MEM_TO_DEV);
#ifdef SS_HASH_HW_PADDING
	if (last == 1) {
		dma_unmap_single(&ss_dev->pdev->dev, virt_to_phys(&ctx->cnt), 4,
			DMA_MEM_TO_DEV);
		ctx->cnt >>= 3;
	}
#endif

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
			ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));
	SS_DBG("After CE, dst data:\n");
	ss_print_hex(digest, SHA512_DIGEST_SIZE, digest);

	if (ss_flow_err(flow)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(flow));
		kfree(digest);
		return -EINVAL;
	}

	/* Backup the MD to ctx->md. */
	memcpy(ctx->md, digest, ctx->md_size);

	if (last == 0)
		ctx->cnt += len;
	kfree(digest);
	return 0;
}


void ss_load_iv(ss_aes_ctx_t *ctx, ss_aes_req_ctx_t *req_ctx,
	char *buf, int size)
{
	if (buf == NULL)
		return;

	/* Only AES/DES/3DES-ECB don't need IV. */
	if (CE_METHOD_IS_AES(req_ctx->type) &&
		(req_ctx->mode == SS_AES_MODE_ECB))
		return;

	/* CBC/CTS need update the IV eachtime. */
	if ((ctx->cnt == 0)
		|| (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CBC))
		|| (CE_IS_AES_MODE(req_ctx->type, req_ctx->mode, CTS))) {
		SS_DBG("IV address = 0x%p, size = %d\n", buf, size);
		ctx->iv_size = size;
		memcpy(ctx->iv, buf, ctx->iv_size);
	}

	SS_DBG("The current IV:\n");
	ss_print_hex(ctx->iv, ctx->iv_size, ctx->iv);
}

int ss_aes_one_req(sunxi_ss_t *sss, struct ablkcipher_request *req)
{
	int ret = 0;
	struct crypto_ablkcipher *tfm = NULL;
	ss_aes_ctx_t *ctx = NULL;
	ss_aes_req_ctx_t *req_ctx = NULL;

	SS_ENTER();
	if (!req->src || !req->dst) {
		SS_ERR("Invalid sg: src = 0x%p, dst = 0x%p\n", req->src, req->dst);
		return -EINVAL;
	}

	ss_dev_lock();

	tfm = crypto_ablkcipher_reqtfm(req);
	req_ctx = ablkcipher_request_ctx(req);
	ctx = crypto_ablkcipher_ctx(tfm);

	ss_load_iv(ctx, req_ctx, req->info, crypto_ablkcipher_ivsize(tfm));

	req_ctx->dma_src.sg = req->src;
	req_ctx->dma_dst.sg = req->dst;

#ifdef SS_RSA_PREPROCESS_ENABLE
	ss_rsa_preprocess(ctx, req_ctx, req->nbytes);
#endif
#ifdef CE_USE_WALK
	if (CE_METHOD_IS_AES(req_ctx->type) ||
		CE_METHOD_IS_DES(req_ctx->type)) {
		ret = ss_aes_start_by_walk(req);
	} else {
		ret = ss_aes_start(ctx, req_ctx, req->nbytes);
	}
#else
	ret = ss_aes_start(ctx, req_ctx, req->nbytes);
#endif
	if (ret < 0)
		SS_ERR("ss_aes_start fail(%d)\n", ret);

	ss_dev_unlock();

#ifdef SS_CTR_MODE_ENABLE
	if (req_ctx->mode == SS_AES_MODE_CTR) {
		SS_DBG("CNT: %08x %08x %08x %08x\n",
			*(int *)&ctx->iv[0], *(int *)&ctx->iv[4],
			*(int *)&ctx->iv[8], *(int *)&ctx->iv[12]);
	}
#endif

	ctx->cnt += req->nbytes;
	return ret;
}

irqreturn_t sunxi_ss_irq_handler(int irq, void *dev_id)
{
	int i;
	int pending = 0;
	sunxi_ss_t *sss = (sunxi_ss_t *)dev_id;
	unsigned long flags = 0;

	spin_lock_irqsave(&sss->lock, flags);

	pending = ss_pending_get();
	SS_DBG("pending: %#x\n", pending);
	for (i = 0; i < SS_FLOW_NUM; i++) {
		if (pending & (CE_CHAN_PENDING<<i)) {
			SS_DBG("Chan %d completed. pending: %#x\n", i, pending);
			ss_pending_clear(i);
			complete(&sss->flows[i].done);
		}
	}

	spin_unlock_irqrestore(&sss->lock, flags);
	return IRQ_HANDLED;
}
