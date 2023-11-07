// SPDX-License-Identifier: GPL-2.0
/*
 * hardware cryptographic offloader for RK3568/RK3588 SoC
 *
 * Copyright (c) 2022-2023 Corentin Labbe <clabbe@baylibre.com>
 */
#include <crypto/scatterwalk.h>
#include "rk2_crypto.h"

static void rk2_print(struct rk2_crypto_dev *rkc)
{
	u32 v;

	v = readl(rkc->reg + RK2_CRYPTO_DMA_ST);
	dev_info(rkc->dev, "DMA_ST %x\n", v);
	switch (v) {
	case 0:
		dev_info(rkc->dev, "DMA_ST: DMA IDLE\n");
		break;
	case 1:
		dev_info(rkc->dev, "DMA_ST: DMA BUSY\n");
		break;
	default:
		dev_err(rkc->dev, "DMA_ST: invalid value\n");
	}

	v = readl(rkc->reg + RK2_CRYPTO_DMA_STATE);
	dev_info(rkc->dev, "DMA_STATE %x\n", v);

	switch (v & 0x3) {
	case 0:
		dev_info(rkc->dev, "DMA_STATE: DMA DST IDLE\n");
		break;
	case 1:
		dev_info(rkc->dev, "DMA_STATE: DMA DST LOAD\n");
		break;
	case 2:
		dev_info(rkc->dev, "DMA_STATE: DMA DST WORK\n");
		break;
	default:
		dev_err(rkc->dev, "DMA DST invalid\n");
		break;
	}
	switch (v & 0xC) {
	case 0:
		dev_info(rkc->dev, "DMA_STATE: DMA SRC IDLE\n");
		break;
	case 1:
		dev_info(rkc->dev, "DMA_STATE: DMA SRC LOAD\n");
		break;
	case 2:
		dev_info(rkc->dev, "DMA_STATE: DMA SRC WORK\n");
		break;
	default:
		dev_err(rkc->dev, "DMA_STATE: DMA SRC invalid\n");
		break;
	}
	switch (v & 0x30) {
	case 0:
		dev_info(rkc->dev, "DMA_STATE: DMA LLI IDLE\n");
		break;
	case 1:
		dev_info(rkc->dev, "DMA_STATE: DMA LLI LOAD\n");
		break;
	case 2:
		dev_info(rkc->dev, "DMA LLI WORK\n");
		break;
	default:
		dev_err(rkc->dev, "DMA LLI invalid\n");
		break;
	}

	v = readl(rkc->reg + RK2_CRYPTO_DMA_LLI_RADDR);
	dev_info(rkc->dev, "DMA_LLI_RADDR %x\n", v);
	v = readl(rkc->reg + RK2_CRYPTO_DMA_SRC_RADDR);
	dev_info(rkc->dev, "DMA_SRC_RADDR %x\n", v);
	v = readl(rkc->reg + RK2_CRYPTO_DMA_DST_WADDR);
	dev_info(rkc->dev, "DMA_LLI_WADDR %x\n", v);
	v = readl(rkc->reg + RK2_CRYPTO_DMA_ITEM_ID);
	dev_info(rkc->dev, "DMA_LLI_ITEMID %x\n", v);

	v = readl(rkc->reg + RK2_CRYPTO_CIPHER_ST);
	dev_info(rkc->dev, "CIPHER_ST %x\n", v);
	if (v & BIT(0))
		dev_info(rkc->dev, "CIPHER_ST: BLOCK CIPHER BUSY\n");
	else
		dev_info(rkc->dev, "CIPHER_ST: BLOCK CIPHER IDLE\n");
	if (v & BIT(2))
		dev_info(rkc->dev, "CIPHER_ST: HASH BUSY\n");
	else
		dev_info(rkc->dev, "CIPHER_ST: HASH IDLE\n");
	if (v & BIT(2))
		dev_info(rkc->dev, "CIPHER_ST: OTP KEY VALID\n");
	else
		dev_info(rkc->dev, "CIPHER_ST: OTP KEY INVALID\n");

	v = readl(rkc->reg + RK2_CRYPTO_CIPHER_STATE);
	dev_info(rkc->dev, "CIPHER_STATE %x\n", v);
	switch (v & 0x3) {
	case 0:
		dev_info(rkc->dev, "serial: IDLE state\n");
		break;
	case 1:
		dev_info(rkc->dev, "serial: PRE state\n");
		break;
	case 2:
		dev_info(rkc->dev, "serial: BULK state\n");
		break;
	default:
		dev_info(rkc->dev, "serial: reserved state\n");
		break;
	}
	switch (v & 0xC) {
	case 0:
		dev_info(rkc->dev, "mac_state: IDLE state\n");
		break;
	case 1:
		dev_info(rkc->dev, "mac_state: PRE state\n");
		break;
	case 2:
		dev_info(rkc->dev, "mac_state: BULK state\n");
		break;
	default:
		dev_info(rkc->dev, "mac_state: reserved state\n");
		break;
	}
	switch (v & 0x30) {
	case 0:
		dev_info(rkc->dev, "parallel_state: IDLE state\n");
		break;
	case 1:
		dev_info(rkc->dev, "parallel_state: PRE state\n");
		break;
	case 2:
		dev_info(rkc->dev, "parallel_state: BULK state\n");
		break;
	default:
		dev_info(rkc->dev, "parallel_state: reserved state\n");
		break;
	}
	switch (v & 0xC0) {
	case 0:
		dev_info(rkc->dev, "ccm_state: IDLE state\n");
		break;
	case 1:
		dev_info(rkc->dev, "ccm_state: PRE state\n");
		break;
	case 2:
		dev_info(rkc->dev, "ccm_state: NA state\n");
		break;
	default:
		dev_info(rkc->dev, "ccm_state: reserved state\n");
		break;
	}
	switch (v & 0xF00) {
	case 0:
		dev_info(rkc->dev, "gcm_state: IDLE state\n");
		break;
	case 1:
		dev_info(rkc->dev, "gcm_state: PRE state\n");
		break;
	case 2:
		dev_info(rkc->dev, "gcm_state: NA state\n");
		break;
	case 3:
		dev_info(rkc->dev, "gcm_state: PC state\n");
		break;
	}
	switch (v & 0xC00) {
	case 0x1:
		dev_info(rkc->dev, "hash_state: IDLE state\n");
		break;
	case 0x2:
		dev_info(rkc->dev, "hash_state: IPAD state\n");
		break;
	case 0x4:
		dev_info(rkc->dev, "hash_state: TEXT state\n");
		break;
	case 0x8:
		dev_info(rkc->dev, "hash_state: OPAD state\n");
		break;
	case 0x10:
		dev_info(rkc->dev, "hash_state: OPAD EXT state\n");
		break;
	default:
		dev_info(rkc->dev, "hash_state: invalid state\n");
		break;
	}

	v = readl(rkc->reg + RK2_CRYPTO_DMA_INT_ST);
	dev_info(rkc->dev, "RK2_CRYPTO_DMA_INT_ST %x\n", v);
}

static int rk2_cipher_need_fallback(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	struct rk2_crypto_template *algt = container_of(alg, struct rk2_crypto_template, alg.skcipher.base);
	struct scatterlist *sgs, *sgd;
	unsigned int stodo, dtodo, len;
	unsigned int bs = crypto_skcipher_blocksize(tfm);

	if (!req->cryptlen)
		return true;

	if (algt->is_xts) {
		if (sg_nents_for_len(req->src, req->cryptlen) > 1)
			return true;
		if (sg_nents_for_len(req->dst, req->cryptlen) > 1)
			return true;
	}

	len = req->cryptlen;
	sgs = req->src;
	sgd = req->dst;
	while (sgs && sgd) {
		if (!IS_ALIGNED(sgs->offset, sizeof(u32))) {
			algt->stat_fb_align++;
			return true;
		}
		if (!IS_ALIGNED(sgd->offset, sizeof(u32))) {
			algt->stat_fb_align++;
			return true;
		}
		stodo = min(len, sgs->length);
		if (stodo % bs) {
			algt->stat_fb_len++;
			return true;
		}
		dtodo = min(len, sgd->length);
		if (dtodo % bs) {
			algt->stat_fb_len++;
			return true;
		}
		if (stodo != dtodo) {
			algt->stat_fb_sgdiff++;
			return true;
		}
		len -= stodo;
		sgs = sg_next(sgs);
		sgd = sg_next(sgd);
	}
	return false;
}

static int rk2_cipher_fallback(struct skcipher_request *areq)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(areq);
	struct rk2_cipher_ctx *op = crypto_skcipher_ctx(tfm);
	struct rk2_cipher_rctx *rctx = skcipher_request_ctx(areq);
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	struct rk2_crypto_template *algt = container_of(alg, struct rk2_crypto_template, alg.skcipher.base);
	int err;

	algt->stat_fb++;

	skcipher_request_set_tfm(&rctx->fallback_req, op->fallback_tfm);
	skcipher_request_set_callback(&rctx->fallback_req, areq->base.flags,
				      areq->base.complete, areq->base.data);
	skcipher_request_set_crypt(&rctx->fallback_req, areq->src, areq->dst,
				   areq->cryptlen, areq->iv);
	if (rctx->mode & RK2_CRYPTO_DEC)
		err = crypto_skcipher_decrypt(&rctx->fallback_req);
	else
		err = crypto_skcipher_encrypt(&rctx->fallback_req);
	return err;
}

static int rk2_cipher_handle_req(struct skcipher_request *req)
{
	struct rk2_cipher_rctx *rctx = skcipher_request_ctx(req);
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct rk2_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct rk2_crypto_dev *rkc;
	struct crypto_engine *engine;

	if (ctx->keylen == AES_KEYSIZE_192 * 2)
		return rk2_cipher_fallback(req);

	if (rk2_cipher_need_fallback(req))
		return rk2_cipher_fallback(req);

	rkc = get_rk2_crypto();

	engine = rkc->engine;
	rctx->dev = rkc;

	return crypto_transfer_skcipher_request_to_engine(engine, req);
}

int rk2_aes_xts_setkey(struct crypto_skcipher *cipher, const u8 *key,
		       unsigned int keylen)
{
	struct crypto_tfm *tfm = crypto_skcipher_tfm(cipher);
	struct rk2_cipher_ctx *ctx = crypto_tfm_ctx(tfm);
	int err;

	err = xts_verify_key(cipher, key, keylen);
	if (err)
		return err;

	ctx->keylen = keylen;
	memcpy(ctx->key, key, keylen);

	return crypto_skcipher_setkey(ctx->fallback_tfm, key, keylen);
}

int rk2_aes_setkey(struct crypto_skcipher *cipher, const u8 *key,
		   unsigned int keylen)
{
	struct crypto_tfm *tfm = crypto_skcipher_tfm(cipher);
	struct rk2_cipher_ctx *ctx = crypto_tfm_ctx(tfm);

	if (keylen != AES_KEYSIZE_128 && keylen != AES_KEYSIZE_192 &&
	    keylen != AES_KEYSIZE_256)
		return -EINVAL;
	ctx->keylen = keylen;
	memcpy(ctx->key, key, keylen);

	return crypto_skcipher_setkey(ctx->fallback_tfm, key, keylen);
}

int rk2_skcipher_encrypt(struct skcipher_request *req)
{
	struct rk2_cipher_rctx *rctx = skcipher_request_ctx(req);
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	struct rk2_crypto_template *algt = container_of(alg, struct rk2_crypto_template, alg.skcipher.base);

	rctx->mode = algt->rk2_mode;
	return rk2_cipher_handle_req(req);
}

int rk2_skcipher_decrypt(struct skcipher_request *req)
{
	struct rk2_cipher_rctx *rctx = skcipher_request_ctx(req);
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	struct rk2_crypto_template *algt = container_of(alg, struct rk2_crypto_template, alg.skcipher.base);

	rctx->mode = algt->rk2_mode | RK2_CRYPTO_DEC;
	return rk2_cipher_handle_req(req);
}

int rk2_cipher_run(struct crypto_engine *engine, void *async_req)
{
	struct skcipher_request *areq = container_of(async_req, struct skcipher_request, base);
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(areq);
	struct rk2_cipher_rctx *rctx = skcipher_request_ctx(areq);
	struct rk2_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct scatterlist *sgs, *sgd;
	int err = 0;
	int ivsize = crypto_skcipher_ivsize(tfm);
	unsigned int len = areq->cryptlen;
	unsigned int todo;
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	struct rk2_crypto_template *algt = container_of(alg, struct rk2_crypto_template, alg.skcipher.base);
	struct rk2_crypto_dev *rkc = rctx->dev;
	struct rk2_crypto_lli *dd = &rkc->tl[0];
	u32 m, v;
	u32 *rkey = (u32 *)ctx->key;
	u32 *riv = (u32 *)areq->iv;
	int i;
	unsigned int offset;

	algt->stat_req++;
	rkc->nreq++;

	m = rctx->mode | RK2_CRYPTO_ENABLE;
	if (algt->is_xts) {
		switch (ctx->keylen) {
		case AES_KEYSIZE_128 * 2:
			m |= RK2_CRYPTO_AES_128BIT_key;
			break;
		case AES_KEYSIZE_256 * 2:
			m |= RK2_CRYPTO_AES_256BIT_key;
			break;
		default:
			dev_err(rkc->dev, "Invalid key length %u\n", ctx->keylen);
			return -EINVAL;
		}
	} else {
		switch (ctx->keylen) {
		case AES_KEYSIZE_128:
			m |= RK2_CRYPTO_AES_128BIT_key;
			break;
		case AES_KEYSIZE_192:
			m |= RK2_CRYPTO_AES_192BIT_key;
			break;
		case AES_KEYSIZE_256:
			m |= RK2_CRYPTO_AES_256BIT_key;
			break;
		default:
			dev_err(rkc->dev, "Invalid key length %u\n", ctx->keylen);
			return -EINVAL;
		}
	}

	err = pm_runtime_resume_and_get(rkc->dev);
	if (err)
		return err;

	/* the upper bits are a write enable mask, so we need to write 1 to all
	 * upper 16 bits to allow write to the 16 lower bits
	 */
	m |= 0xffff0000;

	dev_dbg(rkc->dev, "%s %s len=%u keylen=%u mode=%x\n", __func__,
		crypto_tfm_alg_name(areq->base.tfm),
		areq->cryptlen, ctx->keylen, m);
	sgs = areq->src;
	sgd = areq->dst;

	while (sgs && sgd && len) {
		ivsize = crypto_skcipher_ivsize(tfm);
		if (areq->iv && crypto_skcipher_ivsize(tfm) > 0) {
			if (rctx->mode & RK2_CRYPTO_DEC) {
				offset = sgs->length - ivsize;
				scatterwalk_map_and_copy(rctx->backup_iv, sgs,
							 offset, ivsize, 0);
			}
		}

		dev_dbg(rkc->dev, "SG len=%u mode=%x ivsize=%u\n", sgs->length, m, ivsize);

		if (sgs == sgd) {
			err = dma_map_sg(rkc->dev, sgs, 1, DMA_BIDIRECTIONAL);
			if (err != 1) {
				dev_err(rkc->dev, "Invalid sg number %d\n", err);
				err = -EINVAL;
				goto theend;
			}
		} else {
			err = dma_map_sg(rkc->dev, sgs, 1, DMA_TO_DEVICE);
			if (err != 1) {
				dev_err(rkc->dev, "Invalid sg number %d\n", err);
				err = -EINVAL;
				goto theend;
			}
			err = dma_map_sg(rkc->dev, sgd, 1, DMA_FROM_DEVICE);
			if (err != 1) {
				dev_err(rkc->dev, "Invalid sg number %d\n", err);
				err = -EINVAL;
				dma_unmap_sg(rkc->dev, sgs, 1, DMA_TO_DEVICE);
				goto theend;
			}
		}
		err = 0;
		writel(m, rkc->reg + RK2_CRYPTO_BC_CTL);

		if (algt->is_xts) {
			for (i = 0; i < ctx->keylen / 8; i++) {
				v = cpu_to_be32(rkey[i]);
				writel(v, rkc->reg + RK2_CRYPTO_KEY0 + i * 4);
			}
			for (i = 0; i < (ctx->keylen / 8); i++) {
				v = cpu_to_be32(rkey[i + ctx->keylen / 8]);
				writel(v, rkc->reg + RK2_CRYPTO_CH4_KEY0 + i * 4);
			}
		} else {
			for (i = 0; i < ctx->keylen / 4; i++) {
				v = cpu_to_be32(rkey[i]);
				writel(v, rkc->reg + RK2_CRYPTO_KEY0 + i * 4);
			}
		}

		if (ivsize) {
			for (i = 0; i < ivsize / 4; i++)
				writel(cpu_to_be32(riv[i]),
				       rkc->reg + RK2_CRYPTO_CH0_IV_0 + i * 4);
			writel(ivsize, rkc->reg + RK2_CRYPTO_CH0_IV_LEN);
		}
		if (!sgs->length) {
			sgs = sg_next(sgs);
			sgd = sg_next(sgd);
			continue;
		}

		/* The hw support multiple descriptor, so why this driver use
		 * only one descriptor ?
		 * Using one descriptor per SG seems the way to do and it works
		 * but only when doing encryption.
		 * With decryption it always fail on second descriptor.
		 * Probably the HW dont know how to use IV.
		 */
		todo = min(sg_dma_len(sgs), len);
		len -= todo;
		dd->src_addr = sg_dma_address(sgs);
		dd->src_len = todo;
		dd->dst_addr = sg_dma_address(sgd);
		dd->dst_len = todo;
		dd->iv = 0;
		dd->next = 1;

		dd->user = RK2_LLI_CIPHER_START | RK2_LLI_STRING_FIRST | RK2_LLI_STRING_LAST;
		dd->dma_ctrl |= RK2_LLI_DMA_CTRL_DST_INT | RK2_LLI_DMA_CTRL_LAST;

		writel(RK2_CRYPTO_DMA_INT_LISTDONE | 0x7F, rkc->reg + RK2_CRYPTO_DMA_INT_EN);

		/*writel(0x00030000, rkc->reg + RK2_CRYPTO_FIFO_CTL);*/
		writel(rkc->t_phy, rkc->reg + RK2_CRYPTO_DMA_LLI_ADDR);

		reinit_completion(&rkc->complete);
		rkc->status = 0;

		writel(RK2_CRYPTO_DMA_CTL_START | 1 << 16, rkc->reg + RK2_CRYPTO_DMA_CTL);

		wait_for_completion_interruptible_timeout(&rkc->complete,
							  msecs_to_jiffies(10000));
		if (sgs == sgd) {
			dma_unmap_sg(rkc->dev, sgs, 1, DMA_BIDIRECTIONAL);
		} else {
			dma_unmap_sg(rkc->dev, sgs, 1, DMA_TO_DEVICE);
			dma_unmap_sg(rkc->dev, sgd, 1, DMA_FROM_DEVICE);
		}

		if (!rkc->status) {
			dev_err(rkc->dev, "DMA timeout\n");
			rk2_print(rkc);
			err = -EFAULT;
			goto theend;
		}
		if (areq->iv && ivsize > 0) {
			offset = sgd->length - ivsize;
			if (rctx->mode & RK2_CRYPTO_DEC) {
				memcpy(areq->iv, rctx->backup_iv, ivsize);
				memzero_explicit(rctx->backup_iv, ivsize);
			} else {
				scatterwalk_map_and_copy(areq->iv, sgd, offset,
							 ivsize, 0);
			}
		}
		sgs = sg_next(sgs);
		sgd = sg_next(sgd);
	}
theend:
	writel(0xffff0000, rkc->reg + RK2_CRYPTO_BC_CTL);
	pm_runtime_put_autosuspend(rkc->dev);

	local_bh_disable();
	crypto_finalize_skcipher_request(engine, areq, err);
	local_bh_enable();
	return 0;
}

int rk2_cipher_tfm_init(struct crypto_skcipher *tfm)
{
	struct rk2_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);
	const char *name = crypto_tfm_alg_name(&tfm->base);
	struct skcipher_alg *alg = crypto_skcipher_alg(tfm);
	struct rk2_crypto_template *algt = container_of(alg, struct rk2_crypto_template, alg.skcipher.base);

	ctx->fallback_tfm = crypto_alloc_skcipher(name, 0, CRYPTO_ALG_NEED_FALLBACK);
	if (IS_ERR(ctx->fallback_tfm)) {
		dev_err(algt->dev->dev, "ERROR: Cannot allocate fallback for %s %ld\n",
			name, PTR_ERR(ctx->fallback_tfm));
		return PTR_ERR(ctx->fallback_tfm);
	}

	dev_info(algt->dev->dev, "Fallback for %s is %s\n",
		 crypto_tfm_alg_driver_name(&tfm->base),
		 crypto_tfm_alg_driver_name(crypto_skcipher_tfm(ctx->fallback_tfm)));

	tfm->reqsize = sizeof(struct rk2_cipher_rctx) +
		crypto_skcipher_reqsize(ctx->fallback_tfm);

	return 0;
}

void rk2_cipher_tfm_exit(struct crypto_skcipher *tfm)
{
	struct rk2_cipher_ctx *ctx = crypto_skcipher_ctx(tfm);

	memzero_explicit(ctx->key, ctx->keylen);
	crypto_free_skcipher(ctx->fallback_tfm);
}
