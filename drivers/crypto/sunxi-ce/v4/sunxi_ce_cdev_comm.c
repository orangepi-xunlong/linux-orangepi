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

irqreturn_t sunxi_ce_irq_handler(int irq, void *dev_id)
{
	int i;
	int pending = 0;
	sunxi_ce_cdev_t *p_cdev = (sunxi_ce_cdev_t *)dev_id;
	unsigned long flags = 0;

	spin_lock_irqsave(&p_cdev->lock, flags);

	pending = ss_pending_get();
	SS_DBG("pending: %#x\n", pending);
	for (i = 0; i < SS_FLOW_NUM; i++) {
		if (pending & (CE_CHAN_PENDING << (2 * i))) {
			SS_DBG("Chan %d completed. pending: %#x\n", i, pending);
			ss_pending_clear(i);
			complete(&p_cdev->flows[i].done);
		}
	}

	spin_unlock_irqrestore(&p_cdev->lock, flags);
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
	ss_method_set(req->dir, SS_METHOD_AES, task);
	ss_aes_mode_set(req->aes_mode, task);
}

static void task_iv_init(crypto_aes_req_ctx_t *req, ce_task_desc_t *task, int flag)
{
	if (req->iv_length) {
		if (flag == DMA_MEM_TO_DEV) {
			ss_iv_set(req->iv_buf, req->iv_length, task);
			req->iv_phy = dma_map_single(ce_cdev->pdevice, req->iv_buf,
									req->iv_length, DMA_MEM_TO_DEV);
			SS_DBG("iv = %px, iv_phy_addr = 0x%lx\n", req->iv_buf, req->iv_phy);
		} else if (flag == DMA_DEV_TO_MEM) {
			dma_unmap_single(ce_cdev->pdevice,
				req->iv_phy, req->iv_length, DMA_DEV_TO_MEM);
		} else if (flag == NO_DMA_MAP) {
			task->iv_addr = (req->iv_phy >> WORD_ALGIN);
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
	u32 block_size = 127 * 1024;
	u32 block_size_word = (block_size >> 2);
	u32 block_num, alloc_flag = 0;
	u32 last_data_len, last_size;
	u32 data_len_offset = 0;
	u32 i = 0, n;
	dma_addr_t ptask_phy;
	dma_addr_t next_iv_phy;
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
			ptask->key_addr = (prev->key_addr >> WORD_ALGIN);
			ptask->iv_addr = (prev->iv_addr >> WORD_ALGIN);
			ptask->data_len = 0;
			prev->next_task_addr = (ptask_phy >> WORD_ALGIN);
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
				ptask->src[i].addr = ((src_phy + data_len_offset) >> WORD_ALGIN);
				ptask->src[i].len = block_size_word;
				ptask->dst[i].addr = ((dst_phy + data_len_offset) >> WORD_ALGIN);
				ptask->dst[i].len = block_size_word;
				ptask->data_len += block_size;
				data_len_offset += block_size;
			}
			block_num = block_num - n;
		}

		SS_DBG("block_num =%d i =%d\n", block_num, i);

		/*the last no engure block size*/
		if ((block_num == 0) && (last_size == 0)) {	/*block size aglin */
			ptask->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
			alloc_flag = 0;
			ptask->next_task_addr = 0;
			break;
		} else if ((block_num == 0) && (last_size != 0)) {
			SS_DBG("last_size =%d data_len_offset= %d\n", last_size, data_len_offset);
			/* not block size aglin */
			if ((i < CE_SCATTERS_PER_TASK) && (data_len_offset < length)) {
				last_data_len = length - data_len_offset;

				ptask->src[i].addr = ((src_phy + data_len_offset) >> WORD_ALGIN);
				ptask->src[i].len = (last_data_len >> 2);
				ptask->dst[i].addr = ((dst_phy + data_len_offset) >> WORD_ALGIN);
				ptask->dst[i].len = (last_data_len >> 2);
				ptask->data_len += last_data_len;
				ptask->comm_ctl |= CE_COMM_CTL_TASK_INT_MASK;
				ptask->next_task_addr = 0;
				break;
			}
		}

		if (req->dir == SS_DIR_ENCRYPT) {
			next_iv_phy = ptask->dst[7].addr + (ptask->dst[7].len << 2) - 16;
		} else {
			next_iv_phy = ptask->src[7].addr + (ptask->src[7].len << 2) - 16;
		}
		alloc_flag = 1;
		prev = ptask;

	}
	return 0;
}

static int aes_crypto_start(crypto_aes_req_ctx_t *req, u8 *src_buffer,
							u32 src_length, u8 *dst_buffer)
{
	int ret = 0;
	int channel_id = req->channel_id;
	u32 padding_flag = ce_cdev->flows[req->channel_id].buf_pendding;
	phys_addr_t key_phy = 0;
	phys_addr_t src_phy = 0;
	phys_addr_t dst_phy = 0;
	ce_task_desc_t *task = NULL;

	task = ce_alloc_task();
	if (!task) {
		return -1;
	}

	/*task_mode_set*/
	ce_aes_config(req, task);

	/*task_key_set*/
	if (req->key_length) {
		key_phy = dma_map_single(ce_cdev->pdevice,
					req->key_buffer, req->key_length, DMA_MEM_TO_DEV);
		SS_DBG("key = 0x%px, key_phy_addr = 0x%px\n", req->key_buffer, (void *)key_phy);
		ss_key_set(req->key_buffer, req->key_length, task);
	}

	SS_DBG("ion_flag = %d padding_flag =%d", req->ion_flag, padding_flag);
	/*task_iv_set*/
	if (req->ion_flag && padding_flag) {
		task_iv_init(req, task, NO_DMA_MAP);
	} else {
		task_iv_init(req, task, DMA_MEM_TO_DEV);
	}

	/*task_data_set*/
	/*only the last src_buf is malloc*/
	if (req->ion_flag && (!padding_flag)) {
		src_phy = req->src_phy;
	} else {
		src_phy = dma_map_single(ce_cdev->pdevice, src_buffer, src_length, DMA_MEM_TO_DEV);
	}
	SS_DBG("src = 0x%px, src_phy_addr = 0x%px\n", src_buffer, (void *)src_phy);

	/*the dst_buf is from user*/
	if (req->ion_flag) {
		dst_phy = req->dst_phy;
	} else {
		dst_phy = dma_map_single(ce_cdev->pdevice, dst_buffer, src_length, DMA_MEM_TO_DEV);
	}
	SS_DBG("dst = 0x%px, dst_phy_addr = 0x%px\n", dst_buffer, (void *)dst_phy);

	ce_task_data_init(req, src_phy, dst_phy, src_length, task);
	/*ce_print_task_info(task);*/

	/*start ce*/
	ss_pending_clear(channel_id);
	ss_irq_enable(channel_id);

	init_completion(&ce_cdev->flows[channel_id].done);
	ce_ctrl_start(task, task->task_phy_addr);
	/*ce_reg_print();*/


	ret = wait_for_completion_timeout(&ce_cdev->flows[channel_id].done,
		msecs_to_jiffies(SS_WAIT_TIME));
	if (ret == 0) {
		SS_ERR("Timed out\n");
		ce_task_destroy(task);
		ce_reset();
		ret = -ETIMEDOUT;
	}

	ss_irq_disable(channel_id);
	ce_task_destroy(task);

	/*key*/
	if (req->key_length) {
		dma_unmap_single(ce_cdev->pdevice,
			key_phy, req->key_length, DMA_DEV_TO_MEM);
	}

	/*iv*/
	if (req->ion_flag && padding_flag) {
		;
	} else {
		task_iv_init(req, task, DMA_DEV_TO_MEM);
	}

	/*data*/
	if (req->ion_flag && (!padding_flag)) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice,
				src_phy, src_length, DMA_DEV_TO_MEM);
	}

	if (req->ion_flag) {
		;
	} else {
		dma_unmap_single(ce_cdev->pdevice,
				dst_phy, src_length, DMA_DEV_TO_MEM);
	}

	SS_DBG("After CE, TSR: 0x%08x, ERR: 0x%08x\n",
		ss_reg_rd(CE_REG_TSR), ss_reg_rd(CE_REG_ERR));

	if (ss_flow_err(channel_id)) {
		SS_ERR("CE return error: %d\n", ss_flow_err(channel_id));
		return -EINVAL;
	}

	return 0;
}

int do_aes_crypto(crypto_aes_req_ctx_t *req_ctx)
{
	u32 last_block_size = 0;
	u32 block_num = 0;
	u32 padding_size = 0;
	u32 first_encypt_size = 0;
	u8 data_block[AES_BLOCK_SIZE];
	int channel_id = req_ctx->channel_id;
	int ret;

	ret = check_aes_ctx_vaild(req_ctx);
	if (ret) {
		return -1;
	}

	memset(data_block, 0x0, AES_BLOCK_SIZE);
	ce_cdev->flows[channel_id].buf_pendding = 0;

	if (req_ctx->dir == SS_DIR_DECRYPT) {
		ret = aes_crypto_start(req_ctx, req_ctx->src_buffer,
								req_ctx->src_length, req_ctx->dst_buffer);
		if (ret) {
			SS_ERR("aes decrypt fail\n");
			return -2;
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
				return -2;
			}
			req_ctx->dst_length = block_num * AES_BLOCK_SIZE;
			/*not align 16byte*/
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
					return -2;
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
				return -2;
			}

			req_ctx->dst_length = (block_num + 1) * AES_BLOCK_SIZE;
		}
	}
	SS_ERR("do_aes_crypto sucess\n");
	return 0;
}


