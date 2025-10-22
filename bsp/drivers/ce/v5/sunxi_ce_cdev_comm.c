/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * The driver of SUNXI SecuritySystem controller.
 *
 * Copyright (C) 2014 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/rng.h>
#include <crypto/internal/aead.h>
#include <crypto/hash.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>

#include "../sunxi_ce_cdev.h"
#include "sunxi_ce_reg.h"

#define NO_DMA_MAP		(0xE7)
#define SRC_DATA_DIR	(0)
#define DST_DATA_DIR	(0x1)

extern sunxi_ce_cdev_t	*ce_cdev;

void ce_print_task_desc(ce_task_desc_t *task)
{
	int i;
	u64 phy_addr;

#ifndef SUNXI_CE_DEBUG
	return;
#endif

	pr_info("---------------------task_info--------------------\n");
	pr_info("[channel_id] 0x%x\n", task->chan_id);
	pr_info("task->comm_ctl = 0x%x\n", task->comm_ctl);
	pr_info("task->sym_ctl = 0x%x\n", task->sym_ctl);
	pr_info("task->asym_ctl = 0x%x\n", task->asym_ctl);
	pr_info("task->key_addr = 0x%llx\n", (u64)ce_task_addr_get(task->key_addr));
	pr_info("task->iv_addr = 0x%llx\n", (u64)ce_task_addr_get(task->iv_addr));
	pr_info("task->ctr_addr = 0x%llx\n", (u64)ce_task_addr_get(task->ctr_addr));
	pr_info("task->data_len = 0x%x\n", task->data_len);


	for (i = 0; i < 8; i++) {
		phy_addr = (u64)ce_task_addr_get(task->ce_sg[i].src_addr);
		if (phy_addr) {
			pr_info("task->src[%d].addr = 0x%llx\n", i, phy_addr);
			pr_info("task->src[%d].len = 0x%x\n", i, task->ce_sg[i].src_len);
		}
	}

	for (i = 0; i < 8; i++) {
		phy_addr = (u64)ce_task_addr_get(task->ce_sg[i].dst_addr);
		if (phy_addr) {
			pr_info("task->dst[%d].addr = 0x%llx\n", i, phy_addr);
			pr_info("task->dst[%d].len = 0x%x\n", i, task->ce_sg[i].dst_len);
		}
	}
	pr_info("task->task_phy_addr = 0x%llx\n", (u64)task->task_phy_addr);
}


irqreturn_t sunxi_ce_irq_handler(int irq, void *dev_id)
{
	int i;
	int pending = 0;
	sunxi_ce_cdev_t *p_cdev = (sunxi_ce_cdev_t *)dev_id;

	pending = ss_pending_get();
	SS_DBG("pending: %#x\n", pending);
	for (i = 0; i < SS_FLOW_NUM; i++) {
		if (pending & (CE_CHAN_PENDING << (2 * i))) {
			SS_DBG("Chan %d completed. pending: %#x\n", i, pending);
			ss_pending_clear(i);
			complete(&p_cdev->flows[i].done);
		}
	}

	return IRQ_HANDLED;
}

static int check_aes_ctx_vaild(crypto_aes_req_ctx_t *req)
{
	if (!req->src_buffer || !req->dst_buffer || !req->key_buffer) {
		SS_ERR("Invalid para: src = 0x%px, dst = 0x%px key = 0x%p\n",
				req->src_buffer, req->dst_buffer, req->key_buffer);
		return -EINVAL;
	}

	if (req->iv_length) {
		if (!req->iv_buf) {
			SS_ERR("Invalid para: iv_buf = 0x%px\n", req->iv_buf);
			return -EINVAL;
		}
	}

	SS_DBG("key_length = %d\n", req->key_length);
	if (req->key_length > AES_MAX_KEY_SIZE) {
		SS_ERR("Invalid para: key_length = %d\n", req->key_length);
		return -EINVAL;
	} else if (req->key_length < AES_MIN_KEY_SIZE) {
		SS_ERR("Invalid para: key_length = %d\n", req->key_length);
		return -EINVAL;
	}

	return 0;
}

static void ce_aes_config(crypto_aes_req_ctx_t *req, ce_task_desc_t *task)
{
	task->chan_id = req->channel_id;
	ss_method_set(req->dir, req->method, task);
	ss_aes_mode_set(req->aes_mode, task);

	if (req->aes_mode == SS_AES_MODE_CFB)
		ss_cfb_bitwidth_set(req->bit_width, task);
	else if (req->aes_mode == SS_AES_MODE_CTR)
		ss_ctr_bitwidth_set(req->bit_width, task);
	else
		SS_DBG("the current mode does not need to set bit_width\n");
}

static void task_iv_init(crypto_aes_req_ctx_t *req, ce_task_desc_t *task, int flag)
{
	if (req->iv_length) {
		if (flag == DMA_TO_DEVICE) {
			ss_iv_set(req->iv_buf, req->iv_length, task);
			req->iv_phy = dma_map_single(ce_cdev->pdevice, req->iv_buf,
									req->iv_length, DMA_TO_DEVICE);
			SS_DBG("iv = %px, iv_phy_addr = 0x%lx\n", req->iv_buf, req->iv_phy);
		} else if (flag == DMA_FROM_DEVICE) {
			dma_unmap_single(ce_cdev->pdevice,
				req->iv_phy, req->iv_length, DMA_FROM_DEVICE);
		} else if (flag == NO_DMA_MAP) {
			ce_iv_phyaddr_set(req->iv_phy, task);
			/* task->iv_addr = (req->iv_phy >> WORD_ALGIN); */
			SS_DBG("iv_phy_addr = 0x%lx\n", req->iv_phy);
		}
	}
	return;
}

static ce_task_desc_t *ce_alloc_task(void)
{
	dma_addr_t task_phy_addr;
	ce_task_desc_t *task;

	task = dma_pool_zalloc(ce_cdev->task_pool, GFP_KERNEL, &task_phy_addr);
	if (task == NULL) {
		SS_ERR("Failed to alloc for task\n");
		return NULL;
	} else {
		task->next_virt = NULL;
		task->task_phy_addr = task_phy_addr;
		SS_DBG("task = 0x%px task_phy = 0x%px\n", task, (void *)task_phy_addr);
	}

	return task;
}

static void ce_task_destroy(ce_task_desc_t *task)
{
	ce_task_desc_t *prev;

	while (task != NULL) {
		prev = task;
		task = task->next_virt;
		SS_DBG("prev = 0x%px, prev_phy = 0x%px\n", prev, (void *)prev->task_phy_addr);
		dma_pool_free(ce_cdev->task_pool, prev, prev->task_phy_addr);
	}
	return;
}

static int ce_task_data_init(crypto_aes_req_ctx_t *req, phys_addr_t src_phy,
						phys_addr_t dst_phy, u32 length, ce_task_desc_t *task)
{
	u32 block_size = TASK_MAX_DATA_SIZE;
	u32 block_num, alloc_flag = 0;
	u32 last_data_len, last_size;
	u32 data_len_offset = 0;
	u32 i = 0, n;
	dma_addr_t ptask_phy;
	dma_addr_t tmp_addr;
	dma_addr_t next_iv_phy = 0;
	ce_task_desc_t *ptask = task, *prev;

	block_num = length / block_size;
	last_size = length % block_size;
	ptask->data_len = 0;
	SS_DBG("total_len = 0x%x block_num =%d last_size =%d\n", length, block_num, last_size);
	while (length) {
		if (alloc_flag) {
			ptask = dma_pool_zalloc(ce_cdev->task_pool, GFP_KERNEL, &ptask_phy);
			if (ptask == NULL) {
				SS_ERR("Failed to alloc for ptask\n");
				return -ENOMEM;
			}
			ptask->chan_id  = prev->chan_id;
			ptask->comm_ctl = prev->comm_ctl;
			ptask->sym_ctl  = prev->sym_ctl;
			ptask->asym_ctl = prev->asym_ctl;
			ce_task_addr_set(NULL, ce_task_addr_get(prev->key_addr), ptask->key_addr);
			ce_task_addr_set(NULL, ce_task_addr_get(prev->iv_addr), ptask->iv_addr);
			ptask->data_len = 0;
			ce_task_addr_set(NULL, ce_task_addr_get(prev->next_task_addr), ptask->iv_addr);
			/* prev->next = (ce_task_desc_t *)(ptask_phy >> WORD_ALGIN); */
			prev->next_virt = ptask;
			ptask->task_phy_addr = ptask_phy;

			SS_DBG("ptask = 0x%px, ptask_phy = 0x%px\n", ptask, (void *)ptask_phy);

			if (SS_AES_MODE_CBC == req->aes_mode) {
				req->iv_phy = next_iv_phy;
				task_iv_init(req, ptask, NO_DMA_MAP);
			}
			i = 0;
		}

		if (block_num) {
			n = (block_num > 8) ? CE_SCATTERS_PER_TASK : block_num;
			for (i = 0; i < n; i++) {
				ce_task_addr_set(NULL, (src_phy + data_len_offset), ptask->ce_sg[i].src_addr);
				ptask->ce_sg[i].src_len = block_size;
				ce_task_addr_set(NULL, (dst_phy + data_len_offset), ptask->ce_sg[i].dst_addr);
				ptask->ce_sg[i].dst_len = block_size;
				ptask->data_len += block_size;
				data_len_offset += block_size;
			}
			block_num = block_num - n;
		}

		SS_DBG("block_num =%d i =%d\n", block_num, i);

		/* the last no engure block size */
		if ((block_num == 0) && (last_size == 0)) {  /* block size aglin */
			ptask->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
			alloc_flag = 0;
			/* ptask->next = NULL; */
			break;
		} else if ((block_num == 0) && (last_size != 0)) {
			SS_DBG("last_size =%d data_len_offset= %d\n", last_size, data_len_offset);
			/* not block size aglin */
			if ((i < CE_SCATTERS_PER_TASK) && (data_len_offset < length)) {
				last_data_len = length - data_len_offset;
				ce_task_addr_set(0, (src_phy + data_len_offset), ptask->ce_sg[i].src_addr);
				ptask->ce_sg[i].src_len = last_data_len;
				ce_task_addr_set(0, (dst_phy + data_len_offset), ptask->ce_sg[i].dst_addr);
				ptask->ce_sg[i].dst_len = last_data_len;
				ptask->data_len += last_data_len;
				ptask->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
				/* ptask->next = NULL; */
				break;
			}
		}

		if (req->dir == SS_DIR_ENCRYPT) {
			SS_DBG("next_iv_phy = %pa\n", &next_iv_phy);
			tmp_addr = ce_task_addr_get(ptask->ce_sg[7].dst_addr);
			tmp_addr = tmp_addr + ptask->ce_sg[i].dst_len - 16;
			ce_task_addr_set(0, tmp_addr, (u8 *)&next_iv_phy);
			SS_DBG("next_iv_phy = %pa\n", &next_iv_phy);
		} else {
			tmp_addr = ce_task_addr_get(ptask->ce_sg[7].src_addr);
			tmp_addr = tmp_addr + ptask->ce_sg[7].src_len - 16;
			ce_task_addr_set(0, tmp_addr, (u8 *)&next_iv_phy);
		}
		alloc_flag = 1;
		prev = ptask;

	}
	return 0;
}

static int aes_crypto_start(crypto_aes_req_ctx_t *req, u8 *src_buffer,
							u32 src_length, u8 *dst_buffer)
{
	int ret;
	int channel_id = req->channel_id;
	u32 padding_flag = ce_cdev->flows[req->channel_id].buf_pendding;
	phys_addr_t key_phy = 0;
	phys_addr_t src_phy = 0;
	phys_addr_t dst_phy = 0;
	ce_task_desc_t *task;

	task = ce_alloc_task();
	if (!task) {
		return -ENOMEM;
	}

	/* task_mode_set */
	ce_aes_config(req, task);

	/* task_key_set */
	if (req->key_length) {
		key_phy = dma_map_single(ce_cdev->pdevice,
					req->key_buffer, req->key_length, DMA_TO_DEVICE);
		SS_DBG("key = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer, (void *)key_phy);
		ss_key_set(req->key_buffer, req->key_length, task);
	}

	SS_DBG("ion_flag = %d padding_flag =%d", req->ion_flag, padding_flag);
	/* task_iv_set */
	if (req->ion_flag && padding_flag) {
		task_iv_init(req, task, NO_DMA_MAP);
	} else {
		task_iv_init(req, task, DMA_TO_DEVICE);
	}

	/* task_data_set */
	/* only the last src_buf is malloc */
	if (req->ion_flag && (!padding_flag)) {
		src_phy = req->src_phy;
	} else {
		src_phy = dma_map_single(ce_cdev->pdevice, src_buffer, src_length, DMA_TO_DEVICE);
	}
	SS_DBG("src = 0x%px, src_phy_addr = 0x%px\n", src_buffer, (void *)src_phy);

	/* the dst_buf is from user */
	if (req->ion_flag) {
		dst_phy = req->dst_phy;
	} else {
		dst_phy = dma_map_single(ce_cdev->pdevice, dst_buffer, src_length, DMA_FROM_DEVICE);
	}
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", dst_buffer, (void *)dst_phy);

	ce_task_data_init(req, src_phy, dst_phy, src_length, task);
	ce_print_task_desc(task);

	/* start ce */
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ss_ctrl_start(task, req->method, req->aes_mode);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_reg_print();
		ce_task_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
		goto out;
	}

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
		ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		ret = -EINVAL;
		goto out;
	}

	ret = 0;
out:
	ss_irq_disable(channel_id);
	ce_task_destroy(task);

	/* key */
	if (req->key_length) {
		dma_unmap_single(ce_cdev->pdevice,
			key_phy, req->key_length, DMA_TO_DEVICE);
	}

	/* iv */
	if (req->ion_flag && padding_flag) {
		;
	} else {
		task_iv_init(req, task, DMA_TO_DEVICE);
	}

	/* data */
	if (req->ion_flag && (!padding_flag)) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice,
				src_phy, src_length, DMA_TO_DEVICE);
	}

	if (req->ion_flag) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice,
				dst_phy, src_length, DMA_FROM_DEVICE);
	}

	return ret;
}

int do_aes_crypto(crypto_aes_req_ctx_t *req_ctx)
{
	u32 last_block_size;
	u32 block_num;
	u32 padding_size;
	u32 first_encypt_size;
	u8 data_block[AES_BLOCK_SIZE];
	int channel_id = req_ctx->channel_id;
	int ret;

	ret = check_aes_ctx_vaild(req_ctx);
	if (ret) {
		return ret;
	}

	memset(data_block, 0x0, AES_BLOCK_SIZE);
	ce_cdev->flows[channel_id].buf_pendding = 0;

	if (req_ctx->dir == SS_DIR_DECRYPT) {
		ret = aes_crypto_start(req_ctx, req_ctx->src_buffer,
								req_ctx->src_length, req_ctx->dst_buffer);
		if (ret) {
			SS_ERR("aes decrypt fail\n");
			return ret;
		}
		req_ctx->dst_length = req_ctx->src_length;
	} else {
		block_num = req_ctx->src_length / AES_BLOCK_SIZE;
		last_block_size = req_ctx->src_length % AES_BLOCK_SIZE;
		padding_size = AES_BLOCK_SIZE - last_block_size;

		if (block_num > 0) {
			SS_DBG("block_num = %d\n", block_num);
			first_encypt_size = block_num * AES_BLOCK_SIZE;
			SS_DBG("src_phy = 0x%lx, dst_phy = 0x%lx\n", req_ctx->src_phy, req_ctx->dst_phy);
			ret = aes_crypto_start(req_ctx, req_ctx->src_buffer,
						first_encypt_size,
						req_ctx->dst_buffer
						);
			if (ret) {
				SS_ERR("aes encrypt fail\n");
				return ret;
			}
			req_ctx->dst_length = block_num * AES_BLOCK_SIZE;
			/* not align 16byte */
			if (last_block_size) {
				SS_DBG("last_block_size = %d\n", last_block_size);
				SS_DBG("padding_size = %d\n", padding_size);
				ce_cdev->flows[channel_id].buf_pendding = padding_size;
				if (req_ctx->ion_flag) {
					SS_ERR("ion memery must be 16 byte algin\n");
				} else {
					memcpy(data_block, req_ctx->src_buffer + first_encypt_size, last_block_size);
					memset(data_block + last_block_size, padding_size, padding_size);
				}
				if (SS_AES_MODE_CBC == req_ctx->aes_mode) {
					if (req_ctx->ion_flag) {
						req_ctx->iv_phy = req_ctx->dst_phy + first_encypt_size - AES_BLOCK_SIZE;
					} else {
						req_ctx->iv_buf = req_ctx->dst_buffer + first_encypt_size - AES_BLOCK_SIZE;
					}
				}

				ret = aes_crypto_start(req_ctx, data_block, AES_BLOCK_SIZE,
										req_ctx->dst_buffer + first_encypt_size
										);
				if (ret) {
					SS_ERR("aes encrypt fail\n");
					return ret;
				}
				req_ctx->dst_length = req_ctx->dst_length + AES_BLOCK_SIZE;
			}
		} else {
			SS_DBG("padding_size = %d\n", padding_size);
			ce_cdev->flows[channel_id].buf_pendding = padding_size;
			if (req_ctx->ion_flag) {
				SS_ERR("ion memery must be 16 byte algin\n");
			} else {
				memcpy(data_block, req_ctx->src_buffer, req_ctx->src_length);
				memset(data_block + last_block_size, padding_size, padding_size);
			}
			ret = aes_crypto_start(req_ctx, data_block, AES_BLOCK_SIZE,
									req_ctx->dst_buffer
									);
			if (ret) {
				SS_ERR("aes encrypt fail\n");
				return ret;
			}

			req_ctx->dst_length = (block_num + 1) * AES_BLOCK_SIZE;
		}
	}
	SS_DBG("do_aes_crypto sucess\n");
	return 0;
}

static int check_rsa_ctx_vaild(crypto_rsa_req_ctx_t *req)
{
	if (!req->sign_buffer || !req->pkey_buffer || !req->dst_buffer) {
		SS_ERR("Invalid para: src = 0x%px, dst = 0x%px key = 0x%p\n",
		       req->sign_buffer, req->pkey_buffer, req->dst_buffer);
		return -EINVAL;
	}

	if ((!req->sign_length) || (!req->pkey_length)) {
		SS_ERR("Invalid len: sign = 0x%x, pkey = 0x%x\n",
		       req->sign_length, req->pkey_length);
		return -EINVAL;
	}
	return 0;
}

static void ce_rsa_config(crypto_rsa_req_ctx_t *req, ce_task_desc_t *task)
{
	task->chan_id = req->channel_id;
	ss_method_set(req->dir, SS_METHOD_RSA, task);
	ss_rsa_width_set(req->rsa_width, task);
	task->comm_ctl |= 1 << 31;
}

static void ce_task_rsa_init(crypto_rsa_req_ctx_t *req, phys_addr_t pkey_phy,
			     phys_addr_t sign_phy, phys_addr_t dst_phy,
			     ce_task_desc_t *ptask)
{
	ulong data_len = 0;
	ce_task_addr_set(0, pkey_phy, ptask->ce_sg[0].src_addr);
	ptask->ce_sg[0].src_len = req->pkey_length;

	data_len += req->pkey_length;

	ce_task_addr_set(0, sign_phy, ptask->ce_sg[1].src_addr);
	ptask->ce_sg[1].src_len = req->sign_length;
	data_len += req->sign_length;

	ce_task_addr_set(0, dst_phy, ptask->ce_sg[0].dst_addr);
	ptask->ce_sg[0].dst_len = req->dst_length;
	ss_data_len_set(data_len, ptask);
}

void ce_rsa_task_print(crypto_rsa_req_ctx_t *rsa_ctx)
{
	crypto_rsa_req_ctx_t *rsa_req_ctx = rsa_ctx;
	pr_err("the rsa task");

	pr_err("[sign address] 0x%px\n", rsa_req_ctx->sign_buffer);
	pr_err("[sign length] %d\n", rsa_req_ctx->sign_length);

	pr_err("[pkey address] 0x%px\n", rsa_req_ctx->pkey_buffer);
	pr_err("[pkey length] %d\n", rsa_req_ctx->pkey_length);

	pr_err("[dst address] 0x%px\n", rsa_req_ctx->dst_buffer);
	pr_err("[dst length] %d\n", rsa_req_ctx->dst_length);

	pr_err("[dir] %d\n", rsa_req_ctx->dir);
	pr_err("[rsa width] %d\n", rsa_req_ctx->rsa_width);
	pr_err("[flag] %d\n", rsa_req_ctx->flag);
	pr_err("[channel_id] %d\n", rsa_req_ctx->channel_id);
}

void ce_rsa_task_description_print(ce_task_desc_t *task)
{
	int i;
	ce_task_desc_t *ptask = task;

#ifndef SUNXI_CE_DEBUG
	return;
#endif
	pr_err("the rsa task description");

	pr_err("[channel_id] 0x%x\n", ptask->chan_id);
	pr_err("[comm_ctl] 0x%x\n", ptask->comm_ctl);
	pr_err("[sym_ctl] 0x%x\n", ptask->sym_ctl);
	pr_err("[asym_ctl] 0x%x\n", ptask->asym_ctl);

	pr_err("[data_len] 0x%x\n", ptask->data_len);

	for (i = 0; i < 8; i++) {
		pr_err("[ce_sg%d src_addr] 0x%pa\n", i,
		       ((phys_addr_t *)(ptask->ce_sg[i].src_addr)));
		pr_err("[ce_sg%d dst_addr] 0x%pa\n", i,
		       ((phys_addr_t *)(ptask->ce_sg[i].dst_addr)));
		pr_err("[ce_sg%d src_len] 0x%x\n", i,
		       (ptask->ce_sg[i].src_len));
		pr_err("[ce_sg%d dst_len] 0x%x\n", i,
		       (ptask->ce_sg[i].dst_len));
	}
}

static int rsa_crypto_start(crypto_rsa_req_ctx_t *req)
{
	int ret;
	int channel_id = req->channel_id;
	phys_addr_t pkey_phy;
	phys_addr_t sign_phy;
	phys_addr_t dst_phy;
	ce_task_desc_t *task;

	task = ce_alloc_task();
	if (!task) {
		return -ENOMEM;
	}

	/* task_set */
	ce_rsa_config(req, task);
	pkey_phy = dma_map_single(ce_cdev->pdevice, req->pkey_buffer,
				  req->pkey_length, DMA_TO_DEVICE);
	SS_DBG("pkey = 0x%px, pkey_phy_addr = 0x%px\n", req->pkey_buffer,
	       (void *)pkey_phy);

	sign_phy = dma_map_single(ce_cdev->pdevice, req->sign_buffer,
				  req->sign_length, DMA_TO_DEVICE);
	SS_DBG("sign = 0x%px, sign_phy_addr = 0x%px\n", req->sign_buffer,
	       (void *)sign_phy);

	dst_phy = dma_map_single(ce_cdev->pdevice, req->dst_buffer,
				 req->dst_length, DMA_FROM_DEVICE);
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", req->dst_buffer,
	       (void *)dst_phy);

	ce_task_rsa_init(req, pkey_phy, sign_phy, dst_phy, task);
	/* start ce */
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);
	ce_rsa_task_description_print(task);

	init_completion(&ce_cdev->flows[channel_id].done);
	ss_ctrl_start(task, SS_METHOD_RSA, 0);
	ss_pending_clear(channel_id);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
					  msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_rsa_task_description_print(task);
		ce_rsa_task_print(req);
		ce_reg_print();
		ce_task_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
		goto out;
	}

	ss_irq_disable(channel_id);
	ce_task_destroy(task);

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n", ss_reg_rd(CE_REG_TSR),
	       ss_reg_rd(CE_REG_ERR));

	if (ss_flow_err(channel_id)) {
		ce_rsa_task_print(req);
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		ret = -EINVAL;
		goto out;
	}

	ret = 0;
out:
	/* pkey */
	dma_unmap_single(ce_cdev->pdevice, pkey_phy, req->pkey_length,
			 DMA_TO_DEVICE);
	/* data */
	dma_unmap_single(ce_cdev->pdevice, sign_phy, req->sign_length,
			 DMA_TO_DEVICE);

	dma_unmap_single(ce_cdev->pdevice, dst_phy, req->dst_length,
			 DMA_FROM_DEVICE);

	return ret;
}

int do_rsa_crypto(crypto_rsa_req_ctx_t *req_ctx)
{
	int ret;

	ret = check_rsa_ctx_vaild(req_ctx);
	if (ret) {
		return ret;
	}

	if (req_ctx->dir == SS_DIR_ENCRYPT) {
		SS_ERR("rsa decrypt fail\n");
		return -EINVAL;
	} else {
		ret = rsa_crypto_start(req_ctx);
		if (ret) {
			SS_ERR("rsa encrypt fail\n");
			return -EINVAL;
		}
	}
	SS_ERR("do_rsa_crypto sucess\n");
	return ret;
}

static void ce_ecc_config(crypto_ecc_req_ctx_t *req, ce_task_desc_t *task)
{
	task->chan_id = req->channel_id;
	ss_method_set(req->dir, SS_METHOD_ECC, task);
	ss_ecc_width_set(req->width / 8, task);
	ss_ecc_op_mode_set(req->mode, task);
	task->comm_ctl |= 1 << 31;
}

int do_ecc_crypto(crypto_ecc_req_ctx_t *req)
{
	int err;
	u32 src_data_len, dst_data_len, ecc_byte_size;
	int channel_id = req->channel_id;
	phys_addr_t key_phy;
	phys_addr_t iv_phy;
	phys_addr_t dst_phy;
	phys_addr_t src_phy;
	ce_task_desc_t *task;

	ecc_byte_size = req->width / 8;

	task = ce_alloc_task();
	if (!task) {
		return -ENOMEM;
	}

	ce_ecc_config(req, task);

	key_phy = dma_map_single(ce_cdev->pdevice, req->key_buffer,
				  req->key_length, DMA_TO_DEVICE);
	SS_DBG("pkey = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer,
	       (void *)key_phy);

	iv_phy = dma_map_single(ce_cdev->pdevice, req->iv_buffer,
				  req->iv_length, DMA_TO_DEVICE);
	SS_DBG("sign = 0x%px, iv_phy_addr = 0x%px\n", req->iv_buffer,
	       (void *)iv_phy);

	dst_phy = dma_map_single(ce_cdev->pdevice, req->dst_buffer,
				 req->dst_length, DMA_FROM_DEVICE);
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", req->dst_buffer,
	       (void *)dst_phy);

	src_phy = dma_map_single(ce_cdev->pdevice, req->src_buffer,
				  req->src_length, DMA_TO_DEVICE);
	SS_DBG("src = 0x%px, src_phy_addr = 0x%px\n", req->src_buffer,
	       (void *)src_phy);

	switch (req->mode) {
	case CE_ECC_OP_POINT_ADD:
		src_data_len = ecc_byte_size * 5;
		dst_data_len = ecc_byte_size * 2;
		break;
	case CE_ECC_OP_POINT_DBL:
		src_data_len = ecc_byte_size * 4;
		dst_data_len = ecc_byte_size * 2;
		break;
	case CE_ECC_OP_POINT_MUL:
		src_data_len = ecc_byte_size * 5;
		dst_data_len = ecc_byte_size * 2;
		/* config task src */
		ce_task_addr_set(0, src_phy, task->ce_sg[0].src_addr);
		task->ce_sg[0].src_len = src_data_len;
		task->data_len = src_data_len;

		/* config task dst */
		ce_task_addr_set(0, dst_phy, task->ce_sg[0].dst_addr);
		task->ce_sg[0].dst_len = dst_data_len;
		break;
	case CE_ECC_OP_POINT_VER:
		src_data_len = ecc_byte_size * 5;
		dst_data_len = ecc_byte_size;
		break;
	case CE_ECC_OP_ENC:
		src_data_len = ecc_byte_size * 8;
		dst_data_len = ecc_byte_size * 3;
		/* config task src */
		ce_task_addr_set(0, key_phy, task->ce_sg[0].src_addr);
		task->ce_sg[0].src_len = ecc_byte_size;

		ce_task_addr_set(0, iv_phy, task->ce_sg[1].src_addr);
		task->ce_sg[1].src_len = ecc_byte_size;

		ce_task_addr_set(0, src_phy, task->ce_sg[2].src_addr);
		task->ce_sg[2].src_len = src_data_len - 2 * ecc_byte_size;

		task->data_len = src_data_len;

		/* config task dst */
		ce_task_addr_set(0, dst_phy, task->ce_sg[0].dst_addr);
		task->ce_sg[0].dst_len = dst_data_len;
		break;
	case CE_ECC_OP_DEC:
		src_data_len = ecc_byte_size * 6;
		dst_data_len = ecc_byte_size;
		/* config task src */
		ce_task_addr_set(0, key_phy, task->ce_sg[0].src_addr);
		task->ce_sg[0].src_len = ecc_byte_size;

		ce_task_addr_set(0, iv_phy, task->ce_sg[1].src_addr);
		task->ce_sg[1].src_len = ecc_byte_size;

		ce_task_addr_set(0, src_phy, task->ce_sg[2].src_addr);
		task->ce_sg[2].src_len = src_data_len - 2 * ecc_byte_size;

		task->data_len = src_data_len;

		/* config task dst */
		ce_task_addr_set(0, dst_phy, task->ce_sg[0].dst_addr);
		task->ce_sg[0].dst_len = dst_data_len;
		break;
	case CE_ECC_OP_SIGN:
		src_data_len = ecc_byte_size * 8;
		dst_data_len = ecc_byte_size * 2;
		/* config task src */
		ce_task_addr_set(0, src_phy, task->ce_sg[0].src_addr);
		task->ce_sg[0].src_len = src_data_len;
		task->data_len = src_data_len;

		/* config task dst */
		ce_task_addr_set(0, dst_phy, task->ce_sg[0].dst_addr);
		task->ce_sg[0].dst_len = dst_data_len;
		break;
	case CE_ECC_OP_VERIFY:
		src_data_len = ecc_byte_size * 12;
		dst_data_len = ecc_byte_size;

		/* config task src */
		ce_task_addr_set(0, src_phy, task->ce_sg[0].src_addr);
		task->ce_sg[0].src_len = src_data_len;
		task->data_len = src_data_len;

		/* config task dst */
		ce_task_addr_set(0, dst_phy, task->ce_sg[0].dst_addr);
		task->ce_sg[0].dst_len = dst_data_len;
		break;
	default:
		pr_err("ecc mode config error\n");
		return -EINVAL;
	}

	/* start ce */
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);
	ce_print_task_desc(task);

	init_completion(&ce_cdev->flows[channel_id].done);
	ss_ctrl_start(task, SS_METHOD_ECC, 0);
	ss_pending_clear(channel_id);

	err = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
					  msecs_to_jiffies(SS_WAIT_TIME));
	if (err == 0) {
		SS_ERR("Timed out\n");
		ce_print_task_desc(task);
		ce_reg_print();
		ce_task_destroy(task);
		ce_reset();
		err = -ETIMEDOUT;
		goto out;
	}

	ss_irq_disable(channel_id);
	ce_task_destroy(task);

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n", ss_reg_rd(CE_REG_TSR),
	       ss_reg_rd(CE_REG_ERR));

	err = 0;
out:
	/* key */
	dma_unmap_single(ce_cdev->pdevice, key_phy, req->key_length,
			 DMA_TO_DEVICE);
	/* iv */
	dma_unmap_single(ce_cdev->pdevice, iv_phy, req->iv_length,
			 DMA_TO_DEVICE);

	/* dst */
	dma_unmap_single(ce_cdev->pdevice, dst_phy, req->dst_length,
			 DMA_FROM_DEVICE);

	/* src */
	dma_unmap_single(ce_cdev->pdevice, src_phy, req->src_length,
			 DMA_TO_DEVICE);

	if (err)
		return err;

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		return -EINVAL;
	}

	/*
	 * Verify is to substitute the hash into the ECC curve for verification.
	 * If the verification is successful, output 1; otherwise, output 0.
	 */
	if (req->mode == CE_ECC_OP_VERIFY) {
		SS_DBG("the verify is %d\n", req->dst_buffer[0]);
		if (req->dst_buffer[0] != 1) {
			SS_ERR("ecc verify failed\n");
			return -2;
		}
	}

	return 0;

}

static int check_hash_ctx_vaild(crypto_hash_req_ctx_t *req)
{
	if (!req->text_buffer || !req->dst_buffer) {
		SS_ERR("Invalid para: text = 0x%px, dst = 0x%px\n",
		       req->text_buffer, req->dst_buffer);
		return -EINVAL;
	}

	if (!req->text_length) {
		SS_ERR("Invalid len: text = 0x%x\n", req->text_length);
		return -EINVAL;
	}
	if (req->key_length) {
		if (!req->key_buffer) {
			SS_ERR("Invalid para: key = 0x%px\n", req->key_buffer);
			return -EINVAL;
		}
	}
	if (req->iv_length) {
		if (!req->iv_buffer) {
			SS_ERR("Invalid para: key = 0x%px\n", req->iv_buffer);
			return -EINVAL;
		}
	}
	return 0;
}

static void ce_hash_config(crypto_hash_req_ctx_t *req,
			   ce_new_task_desc_t *task)
{
	if (CE_METHOD_IS_HMAC(req->hash_mode)  && (req->hash_mode == SS_METHOD_HMAC_SHA1))
		ss_hmac_method_set(SS_METHOD_SHA1, task);
	else if (CE_METHOD_IS_HMAC(req->hash_mode)  && (req->hash_mode == SS_METHOD_HMAC_SHA256))
		ss_hmac_method_set(SS_METHOD_SHA256, task);
	else
		ss_hash_method_set(req->hash_mode, task);

	ss_hash_cmd_set(req->channel_id, task);
}

static void ce_task_hash_init(crypto_hash_req_ctx_t *req, phys_addr_t key_phy,
			      phys_addr_t text_phy, phys_addr_t dst_phy,
			      phys_addr_t iv_phy, ce_new_task_desc_t *ptask)
{
	u64 total_bit_len = req->text_length * 8;
	ce_task_addr_set(0, text_phy, ptask->ce_sg[0].src_addr);
	ptask->ce_sg[0].src_len = req->text_length;

	ce_task_addr_set(0, dst_phy, ptask->ce_sg[0].dst_addr);
	ptask->ce_sg[0].dst_len = req->dst_length;

	memcpy(ptask->data_len, &total_bit_len, 4);
#ifdef SS_TOTAL_DATALEN_ENABLE
	/* muilt_packets hash should config total_datalen */
	ss_hash_total_data_len_set(total_bit_len, ptask);
#endif

	if (req->key_length) {
		ce_task_addr_set(0, key_phy, ptask->key_addr);
	}
	if (req->key_length) {
		ce_task_addr_set(0, iv_phy, ptask->iv_addr);
	}
}

static ce_new_task_desc_t *ce_alloc_hash_rng_task(void)
{
	dma_addr_t task_phy_addr;
	ce_new_task_desc_t *task;

	task = dma_pool_zalloc(ce_cdev->task_pool, GFP_KERNEL, &task_phy_addr);
	if (task == NULL) {
		SS_ERR("Failed to alloc for task\n");
		return NULL;
	} else {
		task->next_virt = NULL;
		task->task_phy_addr = task_phy_addr;
		SS_DBG("task = 0x%px task_phy = 0x%px\n", task,
		       (void *)task_phy_addr);
	}

	return task;
}

static void ce_task_hash_rng_destroy(ce_new_task_desc_t *task)
{
	ce_new_task_desc_t *prev;

	while (task != NULL) {
		prev = task;
		task = task->next_virt;
		SS_DBG("prev = 0x%px, prev_phy = 0x%px\n", prev,
		       (void *)prev->task_phy_addr);
		dma_pool_free(ce_cdev->task_pool, prev, prev->task_phy_addr);
	}
	return;
}

void ce_hash_rng_task_desc_print(ce_new_task_desc_t *task)
{
	int i;
	ce_new_task_desc_t *ptask = task;

#ifndef SUNXI_CE_DEBUG
	return;
#endif
	pr_err("the hash task description");

	pr_err("[comm_ctl] 0x%lx\n", ((ulong)ptask->comm_ctl));
	pr_err("[main_cmd] 0x%lx\n", ((ulong)ptask->main_cmd));
	pr_err("[data_len] = 0x%llx\n", (u64)ce_task_addr_get(ptask->data_len));
	pr_err("[total data_len] = %u\n", ptask->reserved[2]);

	for (i = 0; i < 8; i++) {
		pr_err("[ce_sg%d src_addr] 0x%llx\n", i,
			(u64)ce_task_addr_get(ptask->ce_sg[i].src_addr));
		pr_err("[ce_sg%d dst_addr] 0x%llx\n", i,
			(u64)ce_task_addr_get(ptask->ce_sg[i].dst_addr));
		pr_err("[ce_sg%d src_len] 0x%x\n", i,
		       (ptask->ce_sg[i].src_len));
		pr_err("[ce_sg%d dst_len] 0x%x\n", i,
		       (ptask->ce_sg[i].dst_len));
	}
}

int hash_crypto_start(crypto_hash_req_ctx_t *req)
{
	int ret;
	phys_addr_t key_phy = 0;
	phys_addr_t text_phy = 0;
	phys_addr_t dst_phy = 0;
	phys_addr_t iv_phy = 0;
	ce_new_task_desc_t *task;
	int channel_id = req->channel_id;
	task = ce_alloc_hash_rng_task();
	if (!task) {
		return -ENOMEM;
	}

	/* task_mode_set */
	ce_hash_config(req, task);
	/* task_key_set */
	if (req->key_length) {
		key_phy = dma_map_single(ce_cdev->pdevice, req->key_buffer,
					 req->key_length, DMA_TO_DEVICE);
		SS_DBG("key = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer,
		       (void *)key_phy);
	}

	if (req->iv_length) {
		iv_phy = dma_map_single(ce_cdev->pdevice, req->iv_buffer,
					req->iv_length, DMA_TO_DEVICE);
		SS_DBG("iv = %px, iv_phy_addr = %pa\n", req->iv_buffer,
		       &iv_phy);
	}

	/* task_data_set */
	/* only the last src_buf is malloc */
	if (req->ion_flag) {
		text_phy = req->text_phy;
	} else {
		text_phy = dma_map_single(ce_cdev->pdevice, req->text_buffer,
					  req->text_length, DMA_TO_DEVICE);
	}
	SS_DBG("src = 0x%px, src_phy_addr = 0x%px\n", req->text_buffer,
	       (void *)text_phy);

	/* the dst_buf is from user */
	dst_phy = dma_map_single(ce_cdev->pdevice, req->dst_buffer,
					 req->dst_length, DMA_FROM_DEVICE);
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", req->dst_buffer,
	       (void *)dst_phy);

	ce_task_hash_init(req, key_phy, text_phy, dst_phy, iv_phy, task);

	/* start ce */
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ce_hash_rng_task_desc_print(task);
	ss_ctrl_hash_start(task, req->hash_mode, 0);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
					  msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_reg_print();
		ce_task_hash_rng_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
		ce_hash_rng_task_desc_print(task);
		goto out;
	}

	ss_irq_disable(channel_id);
	ce_task_hash_rng_destroy(task);

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		ret = -EINVAL;
		goto out;
	}
	ret = 0;
out:
	/* key */
	if (req->key_length) {
		dma_unmap_single(ce_cdev->pdevice, key_phy, req->key_length,
				 DMA_TO_DEVICE);
	}

	/* iv */
	if (req->iv_length) {
		dma_unmap_single(ce_cdev->pdevice, iv_phy, req->iv_length,
				 DMA_TO_DEVICE);
	}

	/* data */
	if (req->ion_flag) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice, text_phy, req->text_length,
				 DMA_TO_DEVICE);
	}

	dma_unmap_single(ce_cdev->pdevice, dst_phy, req->dst_length,
			 DMA_FROM_DEVICE);

	return ret;
}

int do_hash_crypto(crypto_hash_req_ctx_t *req_ctx)
{
	int ret;
	ret = check_hash_ctx_vaild(req_ctx);
	if (ret) {
		return ret;
	}

	ret = hash_crypto_start(req_ctx);
	if (ret) {
		SS_ERR("hash encrypt fail\n");
		return ret;
	}

	return 0;
}

static int check_rng_ctx_vaild(crypto_rng_req_ctx_t *req)
{
	if (!req->dst_buffer) {
		SS_ERR("Invalid para: dst = 0x%px\n", req->dst_buffer);
		return -EINVAL;
	}

	if (req->key_length) {
		if (!req->key_buffer) {
			SS_ERR("Invalid para: key = 0x%px\n", req->key_buffer);
			return -EINVAL;
		}
	}

	return 0;
}

int do_rng_crypto(crypto_rng_req_ctx_t *req)
{
	int ret;
	phys_addr_t key_phy;
	phys_addr_t dst_phy;
	ce_new_task_desc_t *task;
	int channel_id = req->channel_id;

	ret = check_rng_ctx_vaild(req);
	if (ret)
		return ret;

	task = ce_alloc_hash_rng_task();
	if (!task)
		return -ENOMEM;

	/* task_mode_set, prng key set */
	if (req->trng)
		ss_rng_method_set(SS_METHOD_SHA256, SS_METHOD_TRNG, task);
	else {
		ss_rng_method_set(SS_METHOD_SHA1, SS_METHOD_PRNG, task);
		/* Must set the seed addr in PRNG, key_len 5 words stable */
		req->key_length = 5 * sizeof(int);
		/* ce_task_data_len_set(0, task->data_len); */
		key_phy = dma_map_single(ce_cdev->pdevice, req->key_buffer,
					 req->key_length, DMA_TO_DEVICE);
		SS_DBG("key = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer,
								(void *)key_phy);
		ss_rng_key_set(req->key_buffer, req->key_length, task);
	}

	/* channel set */
	task->comm_ctl |= req->channel_id;

	/* reload and reload_offset set */
	if (req->reload_offset)
		task->main_cmd |= (1 << CE_SUB_CMD_RELOAD_SHIFT) |
			(req->reload_offset << CE_SUB_CMD_RELOAD_OFFSET_SHIFT);
	else
		task->main_cmd |= (0 << CE_SUB_CMD_RELOAD_SHIFT);

	/* the dst_buf is from user */
	dst_phy = dma_map_single(ce_cdev->pdevice, req->dst_buffer,
					 req->dst_length, DMA_FROM_DEVICE);
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", req->dst_buffer,
							(void *)dst_phy);

	ce_task_addr_set(0, dst_phy, task->ce_sg[0].dst_addr);
	task->ce_sg[0].dst_len = req->dst_length;

	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ce_hash_rng_task_desc_print(task);
	/* start ce */
	ss_hash_rng_ctrl_start(task);

	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
					  msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_reg_print();
		ce_task_hash_rng_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
		ce_hash_rng_task_desc_print(task);
		goto out;
	}

	ss_irq_disable(channel_id);
	ce_task_hash_rng_destroy(task);

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		ret = -EINVAL;
		goto out;
	}

	ret = 0;

out:
	/* key */
	if (!req->trng) {
		dma_unmap_single(ce_cdev->pdevice, key_phy, req->key_length,
								DMA_TO_DEVICE);
	}

	dma_unmap_single(ce_cdev->pdevice, dst_phy, req->dst_length,
							DMA_FROM_DEVICE);
	return ret;
}
