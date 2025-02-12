// SPDX-License-Identifier: GPL-2.0
/*
 * ky aes skcipher driver
 *
 * Copyright (C) 2023 Ky
 */

#include <crypto/aes.h>
#include <crypto/internal/skcipher.h>
#include <crypto/algapi.h>
#include <linux/module.h>
#include <linux/cpufeature.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/scatterlist.h>
#include <linux/highmem-internal.h>
#include <asm/cacheflush.h>
#include "crypto/skcipher.h"
#include "ky_engine.h"

int aes_expandkey_nouse(struct crypto_aes_ctx *key, u8 const in[], int size){return 0;}
#define aes_expandkey		aes_expandkey_nouse
#define PRIO			500
#define MODE			"ky-ce1"
char __aligned(8) align[16] = {0};

extern int ky_aes_ecb_encrypt(int index, const unsigned char *pt,unsigned char *ct, u8 *key, unsigned int len, unsigned int blocks);
extern int ky_aes_ecb_decrypt(int index, const unsigned char *ct,unsigned char *pt, u8 *key, unsigned int len, unsigned int blocks);
extern int ky_aes_cbc_encrypt(int index, const unsigned char *pt,unsigned char *ct, u8 *key, unsigned int len, u8 *IV,unsigned int blocks);
extern int ky_aes_cbc_decrypt(int index, const unsigned char *ct,unsigned char *pt, u8 *key, unsigned int len, u8 *IV,unsigned int blocks);
extern int ky_aes_xts_encrypt(int index, const unsigned char *pt, unsigned char *ct,
			u8 *key1, u8 *key2, unsigned int len, u8 *IV,
			unsigned int blocks);
extern int ky_aes_xts_decrypt(int index, const unsigned char *ct, unsigned char *pt,
			u8 *key1, u8 *key2, unsigned int len, u8 *iv,
			unsigned int blocks);
extern int ky_crypto_aes_set_key(int index, struct crypto_tfm *tfm, const u8 *key,unsigned int keylen);
extern void ky_aes_getaddr(unsigned char **in, unsigned char **out);
extern void ky_aes_reladdr(void);

int aes_setkey(struct crypto_skcipher *tfm, const u8 *key,unsigned int keylen)
{
	return ky_crypto_aes_set_key(0, &tfm->base, key, keylen);
}

static int ecb_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;
	unsigned char* map_addr;
	struct scatterlist* sg, *start_srcsg, *start_dstsg;
	int len = 0, sgl_len = 0;
	unsigned char* sg_va,*in_buffer,*out_buffer;
	int total_len = req->cryptlen;
	int page_len = 0, singal_len = 0;

	ky_aes_getaddr(&in_buffer,&out_buffer);
	start_srcsg = req->src;
	start_dstsg = req->dst;
	for(i = 0; total_len > 0; i++){
		if(total_len > KY_AES_BUFFER_LEN)
			page_len = singal_len = KY_AES_BUFFER_LEN;
		else
			page_len = singal_len = total_len;

		if(singal_len % AES_BLOCK_SIZE)
			singal_len = (page_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

		map_addr = in_buffer;
		for(sg = start_srcsg,len = 0;len < page_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(map_addr,sg_va,sg->length);
			sgl_len++;
			map_addr += sg->length;
		}
		if(page_len != singal_len)
			memcpy(map_addr, align, singal_len-page_len);
		start_srcsg = sg_next(sg);

		ky_aes_ecb_encrypt(0,in_buffer, out_buffer,(u8 *)(ctx->key_enc),
				(unsigned int)(ctx->key_length), singal_len / AES_BLOCK_SIZE);

		map_addr = out_buffer;
		for(sg = start_dstsg,len = 0;len<singal_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(sg_va,map_addr,sg->length);
			sgl_len++;
			map_addr += sg->length;
			flush_dcache_page(sg_page(sg));
		}
		start_dstsg = sg_next(sg);

		total_len = total_len - singal_len;

	}
	ky_aes_reladdr();

	return 0;
}

static int ecb_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;
	unsigned char* map_addr;
	struct scatterlist* sg, *start_srcsg, *start_dstsg;
	int len = 0, sgl_len = 0;
	unsigned char* sg_va,*in_buffer,*out_buffer;
	int total_len = req->cryptlen;
	int page_len = 0, singal_len = 0;

	ky_aes_getaddr(&in_buffer,&out_buffer);
	start_srcsg = req->src;
	start_dstsg = req->dst;
	for(i = 0; total_len > 0; i++){
		if(total_len > KY_AES_BUFFER_LEN)
			page_len = singal_len = KY_AES_BUFFER_LEN;
		else
			page_len = singal_len = total_len;
		if(singal_len % AES_BLOCK_SIZE)
			singal_len = (page_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

		map_addr = in_buffer;
		for(sg = start_srcsg,len = 0;len < page_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(map_addr,sg_va,sg->length);
			sgl_len++;
			map_addr += sg->length;
		}
		start_srcsg = sg_next(sg);
		if(page_len != singal_len)
			memcpy(map_addr, align, singal_len-page_len);

		ky_aes_ecb_decrypt(0,in_buffer, out_buffer,(u8 *)(ctx->key_dec),
				(unsigned int)(ctx->key_length), singal_len / AES_BLOCK_SIZE);

		map_addr = out_buffer;
		for(sg = start_dstsg,len = 0;len<singal_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(sg_va,map_addr,sg->length);
			sgl_len++;
			map_addr += sg->length;
			flush_dcache_page(sg_page(sg));
		}
		start_dstsg = sg_next(sg);

		total_len = total_len - singal_len;
	}
	ky_aes_reladdr();

	return 0;
}

static int cbc_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;
	unsigned char* map_addr;
	struct scatterlist* sg, *start_srcsg, *start_dstsg;
	int len = 0, sgl_len = 0;
	unsigned char* sg_va,*in_buffer,*out_buffer;
	int total_len = req->cryptlen;
	int page_len = 0, singal_len = 0;

	ky_aes_getaddr(&in_buffer,&out_buffer);
	start_srcsg = req->src;
	start_dstsg = req->dst;
	for(i = 0; total_len > 0; i++){
		if(total_len > KY_AES_BUFFER_LEN)
			page_len = singal_len = KY_AES_BUFFER_LEN;
		else
			page_len = singal_len = total_len;

		if(singal_len % AES_BLOCK_SIZE)
			singal_len = (page_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

		map_addr = in_buffer;
		for(sg = start_srcsg,len = 0;len < page_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(map_addr,sg_va,sg->length);
			sgl_len++;
			map_addr += sg->length;
		}
		if(page_len != singal_len)
			memcpy(map_addr, align, singal_len-page_len);
		start_srcsg = sg_next(sg);

		ky_aes_cbc_encrypt(0,in_buffer, out_buffer,(u8 *)(ctx->key_enc),
				(unsigned int)(ctx->key_length), (u8 *)req->iv,singal_len / AES_BLOCK_SIZE);

		map_addr = out_buffer;
		for(sg = start_dstsg,len = 0;len<singal_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(sg_va,map_addr,sg->length);
			sgl_len++;
			map_addr += sg->length;
			flush_dcache_page(sg_page(sg));
		}
		start_dstsg = sg_next(sg);

		total_len = total_len - singal_len;

	}
	ky_aes_reladdr();

	return 0;
}

static int cbc_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;
	unsigned char* map_addr;
	struct scatterlist* sg, *start_srcsg, *start_dstsg;
	int len = 0, sgl_len = 0;
	unsigned char* sg_va,*in_buffer,*out_buffer;
	int total_len = req->cryptlen;
	int page_len = 0, singal_len = 0;

	ky_aes_getaddr(&in_buffer,&out_buffer);
	start_srcsg = req->src;
	start_dstsg = req->dst;
	for(i = 0; total_len > 0; i++){
		if(total_len > KY_AES_BUFFER_LEN)
			page_len = singal_len = KY_AES_BUFFER_LEN;
		else
			page_len = singal_len = total_len;
		if(singal_len % AES_BLOCK_SIZE)
			singal_len = (page_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

		map_addr = in_buffer;
		for(sg = start_srcsg,len = 0;len < page_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(map_addr,sg_va,sg->length);
			sgl_len++;
			map_addr += sg->length;
		}
		start_srcsg = sg_next(sg);
		if(page_len != singal_len)
			memcpy(map_addr, align, singal_len-page_len);

		ky_aes_cbc_decrypt(0,in_buffer, out_buffer,(u8 *)(ctx->key_dec),
				(unsigned int)(ctx->key_length), (u8 *)req->iv,singal_len / AES_BLOCK_SIZE);

		map_addr = out_buffer;
		for(sg = start_dstsg,len = 0;len<singal_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(sg_va,map_addr,sg->length);
			sgl_len++;
			map_addr += sg->length;
			flush_dcache_page(sg_page(sg));
		}
		start_dstsg = sg_next(sg);

		total_len = total_len - singal_len;
	}
	ky_aes_reladdr();

	return 0;
}

static int xts_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;
	unsigned char* map_addr;
	struct scatterlist* sg, *start_srcsg, *start_dstsg;
	int len = 0, sgl_len = 0;
	uint32_t xts_key_len = ctx->key_length / 2;
	unsigned char* sg_va,*in_buffer,*out_buffer;
	int total_len = req->cryptlen;
	int page_len = 0, singal_len = 0;

	ky_aes_getaddr(&in_buffer,&out_buffer);
	start_srcsg = req->src;
	start_dstsg = req->dst;
	for(i = 0; total_len > 0; i++){
		if(total_len > KY_AES_BUFFER_LEN)
			page_len = singal_len = KY_AES_BUFFER_LEN;
		else
			page_len = singal_len = total_len;

		if(singal_len % AES_BLOCK_SIZE)
			singal_len = (page_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

		map_addr = in_buffer;
		for(sg = start_srcsg,len = 0;len < page_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(map_addr,sg_va,sg->length);
			sgl_len++;
			map_addr += sg->length;
		}
		if(page_len != singal_len)
			memcpy(map_addr, align, singal_len-page_len);
		start_srcsg = sg_next(sg);

		ky_aes_xts_encrypt(0,in_buffer, out_buffer,(u8 *)(ctx->key_enc),
				((u8 *)ctx->key_enc + xts_key_len), xts_key_len, (u8 *)req->iv,singal_len / AES_BLOCK_SIZE);

		map_addr = out_buffer;
		for(sg = start_dstsg,len = 0;len<singal_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(sg_va,map_addr,sg->length);
			sgl_len++;
			map_addr += sg->length;
			flush_dcache_page(sg_page(sg));
		}
		start_dstsg = sg_next(sg);

		total_len = total_len - singal_len;

	}
	ky_aes_reladdr();

	return 0;
}

static int xts_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct crypto_aes_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;
	unsigned char* map_addr;
	struct scatterlist* sg, *start_srcsg, *start_dstsg;
	int len = 0, sgl_len = 0;
	uint32_t xts_key_len = ctx->key_length / 2;
	unsigned char* sg_va,*in_buffer,*out_buffer;
	int total_len = req->cryptlen;
	int page_len = 0, singal_len = 0;

	ky_aes_getaddr(&in_buffer,&out_buffer);
	start_srcsg = req->src;
	start_dstsg = req->dst;
	for(i = 0; total_len > 0; i++){
		if(total_len > KY_AES_BUFFER_LEN)
			page_len = singal_len = KY_AES_BUFFER_LEN;
		else
			page_len = singal_len = total_len;
		if(singal_len % AES_BLOCK_SIZE)
			singal_len = (page_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

		map_addr = in_buffer;
		for(sg = start_srcsg,len = 0;len < page_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(map_addr,sg_va,sg->length);
			sgl_len++;
			map_addr += sg->length;
		}
		start_srcsg = sg_next(sg);
		if(page_len != singal_len)
			memcpy(map_addr, align, singal_len-page_len);

		ky_aes_xts_decrypt(0,in_buffer, out_buffer,(u8 *)(ctx->key_dec),
				((u8 *)ctx->key_dec + xts_key_len), xts_key_len, (u8 *)req->iv,singal_len / AES_BLOCK_SIZE);

		map_addr = out_buffer;
		for(sg = start_dstsg,len = 0;len<singal_len;len += sg->length)
		{
			if(len != 0)
				sg = sg_next(sg);
			sg_va = (unsigned char*)(PageHighMem(sg_page(sg)) ? kmap_atomic(sg_page(sg)) : page_address(sg_page(sg))) + offset_in_page(sg->offset);
			memcpy(sg_va,map_addr,sg->length);
			sgl_len++;
			map_addr += sg->length;
			flush_dcache_page(sg_page(sg));
		}
		start_dstsg = sg_next(sg);

		total_len = total_len - singal_len;
	}
	ky_aes_reladdr();

	return 0;
}

static struct skcipher_alg aes_algs[] = {
{
	.base.cra_name		= "ecb(aes)",
	.base.cra_driver_name	= "__driver-ecb-aes-" MODE,
	.base.cra_priority	= PRIO,
	.base.cra_flags		= CRYPTO_ALG_ASYNC,
	.base.cra_blocksize	= AES_BLOCK_SIZE,
	.base.cra_ctxsize	= sizeof(struct crypto_aes_ctx),
	.base.cra_alignmask	= 0xf,
	.base.cra_module	= THIS_MODULE,
	.min_keysize		= AES_MIN_KEY_SIZE,
	.max_keysize		= AES_MAX_KEY_SIZE,
	.ivsize			= AES_BLOCK_SIZE,
	.setkey			= aes_setkey,
	.encrypt		= ecb_encrypt,
	.decrypt		= ecb_decrypt,
}, {
	.base.cra_name		= "cbc(aes)",
	.base.cra_driver_name	= "__driver-cbc-aes-" MODE,
	.base.cra_priority	= PRIO,
	.base.cra_flags		= CRYPTO_ALG_ASYNC,
	.base.cra_blocksize	= AES_BLOCK_SIZE,
	.base.cra_ctxsize	= sizeof(struct crypto_aes_ctx),
	.base.cra_alignmask	= 0xf,
	.base.cra_module	= THIS_MODULE,
	.min_keysize		= AES_MIN_KEY_SIZE,
	.max_keysize		= AES_MAX_KEY_SIZE,
	.ivsize			= AES_BLOCK_SIZE,
	.setkey			= aes_setkey,
	.encrypt		= cbc_encrypt,
	.decrypt		= cbc_decrypt,
},{
	.base.cra_name		= "xts(aes)",
	.base.cra_driver_name	= "__driver-xts-aes-" MODE,
	.base.cra_priority	= PRIO,
	.base.cra_flags		= CRYPTO_ALG_ASYNC,
	.base.cra_blocksize	= AES_BLOCK_SIZE,
	.base.cra_ctxsize	= sizeof(struct crypto_aes_ctx),
	.base.cra_alignmask	= 0xf,
	.base.cra_module	= THIS_MODULE,
	.min_keysize		= 2 * AES_MIN_KEY_SIZE,
	.max_keysize		= 2 * AES_MAX_KEY_SIZE,
	.ivsize			= AES_BLOCK_SIZE,
	.setkey			= aes_setkey,
	.encrypt		= xts_encrypt,
	.decrypt		= xts_decrypt,
},
};

static int __init aes_init(void)
{
	return crypto_register_skciphers(aes_algs, ARRAY_SIZE(aes_algs));
}

static void __exit aes_exit(void)
{
	crypto_unregister_skciphers(aes_algs, ARRAY_SIZE(aes_algs));
}

module_init(aes_init);
module_exit(aes_exit);

MODULE_DESCRIPTION("AES-ECB/CBC using Ky CE Engine");
MODULE_ALIAS_CRYPTO("ecb(aes)");
MODULE_ALIAS_CRYPTO("cbc(aes)");
MODULE_LICENSE("GPL v2");
