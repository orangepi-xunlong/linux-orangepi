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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <crypto/hash.h>
#include <crypto/md5.h>
#include <crypto/des.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/rng.h>

#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>
#include <linux/clk/sunxi_name.h>

#include "sunxi_ss.h"
#include "sunxi_ss_proc.h"
#include "sunxi_ss_reg.h"

sunxi_ss_t *ss_dev = NULL;

static DEFINE_MUTEX(ss_lock);

void ss_dev_lock(void)
{
	mutex_lock(&ss_lock);
}

void ss_dev_unlock(void)
{
	mutex_unlock(&ss_lock);
}

void __iomem *ss_membase(void)
{
	return ss_dev->base_addr;
}

void ss_reset(void)
{
	SS_ENTER();
	sunxi_periph_reset_assert(ss_dev->mclk);
	sunxi_periph_reset_deassert(ss_dev->mclk);
}

void ss_clk_set(u32 rate)
{
#ifdef CONFIG_EVB_PLATFORM
	int ret = 0;

	ret = clk_get_rate(ss_dev->mclk);
	if (ret == rate)
		return;

	SS_DBG("Change the SS clk to %d MHz. \n", rate/1000000);
	ret = clk_set_rate(ss_dev->mclk, rate);
	if (ret != 0)
		SS_ERR("clk_set_rate(%d) failed! return %d\n", rate, ret);
#endif
}

static int ss_aes_setkey(struct crypto_ablkcipher *tfm, const u8 *key, 
				unsigned int keylen)
{
	int ret = 0;
	ss_aes_ctx_t *ctx = crypto_ablkcipher_ctx(tfm);

	SS_DBG("keylen = %d\n", keylen);
	if (ctx->comm.flags & SS_FLAG_NEW_KEY) {
		SS_ERR("The key has already update.\n");
		return -EBUSY;
	}

	ret = ss_aes_key_valid(tfm, keylen);
	if (ret != 0)
		return ret;

	ctx->key_size = keylen;
	memcpy(ctx->key, key, keylen);
	if (keylen < AES_KEYSIZE_256)
		memset(&ctx->key[keylen], 0, AES_KEYSIZE_256 - keylen);

	ctx->comm.flags |= SS_FLAG_NEW_KEY;
	return 0;
}

static int ss_aes_ecb_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_ECB);
}

static int ss_aes_ecb_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_ECB);
}

static int ss_aes_cbc_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CBC);
}

static int ss_aes_cbc_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CBC);
}

#ifdef SS_CTR_MODE_ENABLE
static int ss_aes_ctr_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CTR);
}

static int ss_aes_ctr_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CTR);
}
#endif

#ifdef SS_CTS_MODE_ENABLE
static int ss_aes_cts_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CTS);
}

static int ss_aes_cts_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CTS);
}
#endif

#ifdef SS_OFB_MODE_ENABLE
static int ss_aes_ofb_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_OFB);
}

static int ss_aes_ofb_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_OFB);
}
#endif

#ifdef SS_CFB_MODE_ENABLE
static int ss_aes_cfb1_encrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 1;
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb1_decrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 1;
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb8_encrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 8;
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb8_decrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 8;
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb64_encrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 64;
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb64_decrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 64;
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb128_encrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 128;
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}

static int ss_aes_cfb128_decrypt(struct ablkcipher_request *req)
{
	ss_aes_req_ctx_t *req_ctx = ablkcipher_request_ctx(req);

	req_ctx->bitwidth = 128;
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CFB);
}
#endif

#ifdef SS_CBC_MAC_MODE_ENABLE
static int ss_aes_cbc_mac_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_AES, SS_AES_MODE_CBC_MAC);
}

static int ss_aes_cbc_mac_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_AES, SS_AES_MODE_CBC_MAC);
}
#endif

static int ss_des_ecb_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_DES, SS_AES_MODE_ECB);
}

static int ss_des_ecb_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_DES, SS_AES_MODE_ECB);
}

static int ss_des_cbc_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_DES, SS_AES_MODE_CBC);
}

static int ss_des_cbc_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_DES, SS_AES_MODE_CBC);
}

static int ss_des3_ecb_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_3DES, SS_AES_MODE_ECB);
}

static int ss_des3_ecb_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_3DES, SS_AES_MODE_ECB);
}

static int ss_des3_cbc_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_3DES, SS_AES_MODE_CBC);
}

static int ss_des3_cbc_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_3DES, SS_AES_MODE_CBC);
}

#ifdef SS_RSA_ENABLE
static int ss_rsa_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_RSA, SS_AES_MODE_CBC);
}

static int ss_rsa_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_RSA, SS_AES_MODE_CBC);
}
#endif

#ifdef SS_DH_ENABLE
static int ss_dh_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_DH, CE_RSA_OP_M_EXP);
}

static int ss_dh_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_DH, CE_RSA_OP_M_EXP);
}
#endif

#ifdef SS_ECC_ENABLE
static int ss_ecdh_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_ECC, CE_ECC_OP_POINT_MUL);
}

static int ss_ecdh_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_ECC, CE_ECC_OP_POINT_MUL);
}

static int ss_ecc_sign_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_ECC, CE_ECC_OP_SIGN);
}

static int ss_ecc_sign_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_ECC, CE_ECC_OP_SIGN);
}

static int ss_ecc_verify_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_RSA, CE_RSA_OP_M_MUL);
}

static int ss_ecc_verify_decrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_DECRYPT, SS_METHOD_RSA, CE_RSA_OP_M_MUL);
}

#endif

#ifdef SS_HMAC_SHA1_ENABLE
static int ss_hmac_sha1_encrypt(struct ablkcipher_request *req)
{
	return ss_aes_crypt(req, SS_DIR_ENCRYPT, SS_METHOD_HMAC_SHA1, SS_AES_MODE_ECB);
}
#endif

int ss_rng_reset(struct crypto_rng *tfm, u8 *seed, unsigned int slen)
{
	int len = slen > SS_PRNG_SEED_LEN ? SS_PRNG_SEED_LEN : slen;
	ss_aes_ctx_t *ctx = crypto_rng_ctx(tfm);

	SS_DBG("Seed len: %d/%d, ctx->flags = %#x \n", len, slen, ctx->comm.flags);
	ctx->key_size = len;
	memset(ctx->key, 0, SS_PRNG_SEED_LEN);
	memcpy(ctx->key, seed, len);
	ctx->comm.flags |= SS_FLAG_NEW_KEY;

	return 0;
}

static int ss_flow_request(ss_comm_ctx_t *comm)
{
	int i;
	unsigned long flags = 0;

	spin_lock_irqsave(&ss_dev->lock, flags);
	for (i=0; i<SS_FLOW_NUM; i++) {
		if (ss_dev->flows[i].available == SS_FLOW_AVAILABLE) {
			comm->flow = i;
			ss_dev->flows[i].available = SS_FLOW_UNAVAILABLE;
			SS_DBG("The flow %d is available. \n", i);
			break;
		}
	}
	spin_unlock_irqrestore(&ss_dev->lock, flags);

	if (i == SS_FLOW_NUM) {
		SS_ERR("Failed to get an available flow. \n");
		i = -1;
	}
	return i;
}

static void ss_flow_release(ss_comm_ctx_t *comm)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&ss_dev->lock, flags);
	ss_dev->flows[comm->flow].available = SS_FLOW_AVAILABLE;
	spin_unlock_irqrestore(&ss_dev->lock, flags);
}

static int sunxi_ss_cra_init(struct crypto_tfm *tfm)
{
	if (ss_flow_request(crypto_tfm_ctx(tfm)) < 0)
		return -1;

	tfm->crt_ablkcipher.reqsize = sizeof(ss_aes_req_ctx_t);
	SS_DBG("reqsize = %d \n", tfm->crt_u.ablkcipher.reqsize);
	return 0;
}

static int sunxi_ss_cra_rng_init(struct crypto_tfm *tfm)
{
	if (ss_flow_request(crypto_tfm_ctx(tfm)) < 0)
		return -1;

	return 0;
}

static int sunxi_ss_cra_hash_init(struct crypto_tfm *tfm)
{
	if (ss_flow_request(crypto_tfm_ctx(tfm)) < 0)
		return -1;

	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm),
				 sizeof(ss_aes_req_ctx_t));

	SS_DBG("reqsize = %d \n", sizeof(ss_aes_req_ctx_t));
	return 0;
}

static void sunxi_ss_cra_exit(struct crypto_tfm *tfm)
{
	SS_ENTER();
	ss_flow_release(crypto_tfm_ctx(tfm));

	/* sun8iw6 and sun9iw1 need reset SS controller after each operation. */
#ifdef SS_IDMA_ENABLE
	ss_reset();
#endif
}

static int ss_hash_init(struct ahash_request *req, int type, int size, char *iv)
{
	ss_aes_req_ctx_t *req_ctx = ahash_request_ctx(req);
	ss_hash_ctx_t *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));

	SS_DBG("Method: %d \n", type);

	memset(req_ctx, 0, sizeof(ss_aes_req_ctx_t));
	req_ctx->type = type;

	ctx->md_size = size;
	memcpy(ctx->md, iv, size);

	ctx->cnt = 0;
	memset(ctx->pad, 0, SS_HASH_PAD_SIZE);
	return 0;
}

static int ss_md5_init(struct ahash_request *req)
{
	int iv[MD5_DIGEST_SIZE/4] = {SHA1_H0, SHA1_H1, SHA1_H2, SHA1_H3};

	return ss_hash_init(req, SS_METHOD_MD5, MD5_DIGEST_SIZE, (char *)iv);
}

static int ss_sha1_init(struct ahash_request *req)
{
	int iv[SHA1_DIGEST_SIZE/4] = {SHA1_H0, SHA1_H1, SHA1_H2, SHA1_H3, SHA1_H4};

#ifdef SS_SHA_SWAP_PRE_ENABLE
#ifdef SS_SHA_NO_SWAP_IV4
	ss_hash_swap((char *)iv, SHA1_DIGEST_SIZE - 4);
#else
	ss_hash_swap((char *)iv, SHA1_DIGEST_SIZE);
#endif
#endif

	return ss_hash_init(req, SS_METHOD_SHA1, SHA1_DIGEST_SIZE, (char *)iv);
}

#ifdef SS_SHA224_ENABLE
static int ss_sha224_init(struct ahash_request *req)
{
	int iv[SHA256_DIGEST_SIZE/4] = {SHA224_H0, SHA224_H1, SHA224_H2, SHA224_H3,
									SHA224_H4, SHA224_H5, SHA224_H6, SHA224_H7};

#ifdef SS_SHA_SWAP_PRE_ENABLE
	ss_hash_swap((char *)iv, SHA256_DIGEST_SIZE);
#endif

	return ss_hash_init(req, SS_METHOD_SHA224, SHA256_DIGEST_SIZE, (char *)iv);
}
#endif

#ifdef SS_SHA256_ENABLE
static int ss_sha256_init(struct ahash_request *req)
{
	int iv[SHA256_DIGEST_SIZE/4] = {SHA256_H0, SHA256_H1, SHA256_H2, SHA256_H3,
									SHA256_H4, SHA256_H5, SHA256_H6, SHA256_H7};

#ifdef SS_SHA_SWAP_PRE_ENABLE
	ss_hash_swap((char *)iv, SHA256_DIGEST_SIZE);
#endif

	return ss_hash_init(req, SS_METHOD_SHA256, SHA256_DIGEST_SIZE, (char *)iv);
}
#endif

#define GET_U64_HIGH(data64)	(int)(data64 >> 32)
#define GET_U64_LOW(data64)		(int)(data64 & 0xFFFFFFFF)

#ifdef SS_SHA384_ENABLE
static int ss_sha384_init(struct ahash_request *req)
{
	int iv[SHA512_DIGEST_SIZE/4] = {GET_U64_HIGH(SHA384_H0), GET_U64_LOW(SHA384_H0),
									GET_U64_HIGH(SHA384_H1), GET_U64_LOW(SHA384_H1),
									GET_U64_HIGH(SHA384_H2), GET_U64_LOW(SHA384_H2),
									GET_U64_HIGH(SHA384_H3), GET_U64_LOW(SHA384_H3),
									GET_U64_HIGH(SHA384_H4), GET_U64_LOW(SHA384_H4),
									GET_U64_HIGH(SHA384_H5), GET_U64_LOW(SHA384_H5),
									GET_U64_HIGH(SHA384_H6), GET_U64_LOW(SHA384_H6),
									GET_U64_HIGH(SHA384_H7), GET_U64_LOW(SHA384_H7)};

#ifdef SS_SHA_SWAP_PRE_ENABLE
	ss_hash_swap((char *)iv, SHA512_DIGEST_SIZE);
#endif

	return ss_hash_init(req, SS_METHOD_SHA384, SHA512_DIGEST_SIZE, (char *)iv);
}
#endif

#ifdef SS_SHA512_ENABLE
static int ss_sha512_init(struct ahash_request *req)
{
	int iv[SHA512_DIGEST_SIZE/4] = {GET_U64_HIGH(SHA512_H0), GET_U64_LOW(SHA512_H0),
									GET_U64_HIGH(SHA512_H1), GET_U64_LOW(SHA512_H1),
									GET_U64_HIGH(SHA512_H2), GET_U64_LOW(SHA512_H2),
									GET_U64_HIGH(SHA512_H3), GET_U64_LOW(SHA512_H3),
									GET_U64_HIGH(SHA512_H4), GET_U64_LOW(SHA512_H4),
									GET_U64_HIGH(SHA512_H5), GET_U64_LOW(SHA512_H5),
									GET_U64_HIGH(SHA512_H6), GET_U64_LOW(SHA512_H6),
									GET_U64_HIGH(SHA512_H7), GET_U64_LOW(SHA512_H7)};

#ifdef SS_SHA_SWAP_PRE_ENABLE
	ss_hash_swap((char *)iv, SHA512_DIGEST_SIZE);
#endif

	return ss_hash_init(req, SS_METHOD_SHA512, SHA512_DIGEST_SIZE, (char *)iv);
}
#endif

#define DES_MIN_KEY_SIZE	DES_KEY_SIZE
#define DES_MAX_KEY_SIZE	DES_KEY_SIZE
#define DES3_MIN_KEY_SIZE	DES3_EDE_KEY_SIZE
#define DES3_MAX_KEY_SIZE	DES3_EDE_KEY_SIZE

#define DECLARE_SS_AES_ALG(utype, ltype, lmode, block_size, iv_size) \
{ \
	.cra_name		 = #lmode"("#ltype")", \
	.cra_driver_name = "ss-"#lmode"-"#ltype, \
	.cra_flags		 = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC, \
	.cra_type		 = &crypto_ablkcipher_type, \
	.cra_blocksize	 = block_size, \
	.cra_alignmask	 = 3, \
	.cra_u.ablkcipher = { \
		.setkey 	 = ss_aes_setkey, \
		.encrypt	 = ss_##ltype##_##lmode##_encrypt, \
		.decrypt	 = ss_##ltype##_##lmode##_decrypt, \
		.min_keysize = utype##_MIN_KEY_SIZE, \
		.max_keysize = utype##_MAX_KEY_SIZE, \
		.ivsize		 = iv_size, \
	} \
}

#define DECLARE_SS_ASYM_ALG(type, bitwidth, key_size, iv_size) \
{ \
	.cra_name 	 	 = #type"("#bitwidth")", \
	.cra_driver_name = "ss-"#type"-"#bitwidth, \
	.cra_flags		 = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC, \
	.cra_type		 = &crypto_ablkcipher_type, \
	.cra_blocksize	 = key_size%AES_BLOCK_SIZE == 0 ? AES_BLOCK_SIZE : 4, \
	.cra_alignmask	 = key_size%AES_BLOCK_SIZE == 0 ? 31 : 3, \
	.cra_u.ablkcipher = { \
		.setkey 	 = ss_aes_setkey, \
		.encrypt	 = ss_##type##_encrypt, \
		.decrypt	 = ss_##type##_decrypt, \
		.min_keysize = key_size, \
		.max_keysize = key_size, \
		.ivsize 	 = iv_size, \
	}, \
}
#define DECLARE_SS_RSA_ALG(type, bitwidth)	DECLARE_SS_ASYM_ALG(type, bitwidth, (bitwidth/8), (bitwidth/8))
#define DECLARE_SS_DH_ALG(type, bitwidth)	DECLARE_SS_RSA_ALG(type, bitwidth)
#define DECLARE_SS_ECC_ALG					DECLARE_SS_ASYM_ALG

#define DECLARE_SS_RNG_ALG(ltype) \
{ \
	.cra_name = #ltype, \
	.cra_driver_name = "ss-"#ltype, \
	.cra_flags = CRYPTO_ALG_TYPE_RNG, \
	.cra_type = &crypto_rng_type, \
	.cra_u.rng = { \
		.rng_make_random = ss_##ltype##_get_random, \
		.rng_reset = ss_rng_reset, \
		.seedsize = SS_SEED_SIZE, \
	} \
}

static struct crypto_alg sunxi_ss_algs[] =
{
	DECLARE_SS_AES_ALG(AES, aes, ecb, AES_BLOCK_SIZE, 0),
	DECLARE_SS_AES_ALG(AES, aes, cbc, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
#ifdef SS_CTR_MODE_ENABLE
	DECLARE_SS_AES_ALG(AES, aes, ctr, 1, AES_MIN_KEY_SIZE),
#endif
#ifdef SS_CTS_MODE_ENABLE
	DECLARE_SS_AES_ALG(AES, aes, cts, 1, AES_MIN_KEY_SIZE),
#endif
#ifdef SS_OFB_MODE_ENABLE
	DECLARE_SS_AES_ALG(AES, aes, ofb, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
#endif
#ifdef SS_CFB_MODE_ENABLE
	DECLARE_SS_AES_ALG(AES, aes, cfb1, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
	DECLARE_SS_AES_ALG(AES, aes, cfb8, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
	DECLARE_SS_AES_ALG(AES, aes, cfb64, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
	DECLARE_SS_AES_ALG(AES, aes, cfb128, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
#endif
#ifdef SS_CBC_MAC_MODE_ENABLE
	DECLARE_SS_AES_ALG(AES, aes, cbc_mac, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
#endif
	DECLARE_SS_AES_ALG(DES, des, ecb, DES_BLOCK_SIZE, 0),
	DECLARE_SS_AES_ALG(DES, des, cbc, DES_BLOCK_SIZE, DES_KEY_SIZE),
	DECLARE_SS_AES_ALG(DES3, des3, ecb, DES3_EDE_BLOCK_SIZE, 0),
	DECLARE_SS_AES_ALG(DES3, des3, cbc, DES3_EDE_BLOCK_SIZE, DES_KEY_SIZE),
#ifdef SS_RSA512_ENABLE
	DECLARE_SS_RSA_ALG(rsa, 512),
#endif
#ifdef SS_RSA1024_ENABLE
	DECLARE_SS_RSA_ALG(rsa, 1024),
#endif
#ifdef SS_RSA2048_ENABLE
	DECLARE_SS_RSA_ALG(rsa, 2048),
#endif
#ifdef SS_RSA3072_ENABLE
	DECLARE_SS_RSA_ALG(rsa, 3072),
#endif
#ifdef SS_RSA4096_ENABLE
	DECLARE_SS_RSA_ALG(rsa, 4096),
#endif
#ifdef SS_DH_ENABLE
	DECLARE_SS_DH_ALG(dh, 512),
	DECLARE_SS_DH_ALG(dh, 1024),
	DECLARE_SS_DH_ALG(dh, 2048),
	DECLARE_SS_DH_ALG(dh, 3072),
	DECLARE_SS_DH_ALG(dh, 4096),
#endif
#ifdef SS_ECC_ENABLE
	DECLARE_SS_ASYM_ALG(ecdh, 160, 160/8, 160/8),
	DECLARE_SS_ASYM_ALG(ecdh, 224, 224/8, 224/8),
	DECLARE_SS_ASYM_ALG(ecdh, 256, 256/8, 256/8),
	DECLARE_SS_ASYM_ALG(ecdh, 521, ((521+31)/32)*4, ((521+31)/32)*4),
	DECLARE_SS_ASYM_ALG(ecc_sign, 160, 160/8, (160/8)*2),
	DECLARE_SS_ASYM_ALG(ecc_sign, 224, 224/8, (224/8)*2),
	DECLARE_SS_ASYM_ALG(ecc_sign, 256, 256/8, (256/8)*2),
	DECLARE_SS_ASYM_ALG(ecc_sign, 521, ((521+31)/32)*4, ((521+31)/32)*4*2),
	DECLARE_SS_RSA_ALG(ecc_verify, 512),
	DECLARE_SS_RSA_ALG(ecc_verify, 1024),
#endif
#ifdef SS_TRNG_ENABLE
	DECLARE_SS_RNG_ALG(trng),
#endif
	DECLARE_SS_RNG_ALG(prng),

#ifdef SS_HMAC_SHA1_ENABLE
	{
		.cra_name		 = "hmac-sha1",
		.cra_driver_name = "ss-hmac-sha1",
		.cra_flags		 = CRYPTO_ALG_TYPE_ABLKCIPHER | CRYPTO_ALG_ASYNC,
		.cra_type		 = &crypto_ablkcipher_type,
		.cra_blocksize	 = 4,
		.cra_alignmask	 = 3,
		.cra_u.ablkcipher = {
			.setkey 	 = ss_aes_setkey,
			.encrypt	 = ss_hmac_sha1_encrypt,
			.decrypt	 = NULL,
			.min_keysize = SHA1_BLOCK_SIZE,
			.max_keysize = SHA1_BLOCK_SIZE,
			.ivsize	 	 = 0,
		}
	}
#endif
};

#define MD5_BLOCK_SIZE	MD5_HMAC_BLOCK_SIZE

#define DECLARE_SS_AHASH_ALG(ltype, utype) \
	{ \
		.init		= ss_##ltype##_init, \
		.update		= ss_hash_update, \
		.final		= ss_hash_final, \
		.finup		= ss_hash_finup, \
		.halg.digestsize	= utype##_DIGEST_SIZE, \
		.halg.base	= { \
			.cra_name		= #ltype, \
			.cra_driver_name= "ss-"#ltype, \
			.cra_flags		= CRYPTO_ALG_TYPE_AHASH|CRYPTO_ALG_ASYNC, \
			.cra_blocksize	= utype##_BLOCK_SIZE, \
			.cra_ctxsize	= sizeof(ss_hash_ctx_t), \
			.cra_alignmask	= 3, \
			.cra_module		= THIS_MODULE, \
			.cra_init		= sunxi_ss_cra_hash_init, \
			.cra_exit		= sunxi_ss_cra_exit, \
		} \
	}

static struct ahash_alg sunxi_ss_algs_hash[] = {
	DECLARE_SS_AHASH_ALG(md5, MD5),
	DECLARE_SS_AHASH_ALG(sha1, SHA1),
#ifdef SS_SHA224_ENABLE
	DECLARE_SS_AHASH_ALG(sha224, SHA224),
#endif
#ifdef SS_SHA256_ENABLE
	DECLARE_SS_AHASH_ALG(sha256, SHA256),
#endif
#ifdef SS_SHA384_ENABLE
	DECLARE_SS_AHASH_ALG(sha384, SHA384),
#endif
#ifdef SS_SHA512_ENABLE
	DECLARE_SS_AHASH_ALG(sha512, SHA512),
#endif
};

/* Requeset the resource: IRQ, mem */
static int __devinit sunxi_ss_res_request(struct platform_device *pdev)
{
	int irq = 0;
	int ret = 0;
	struct resource	*mem_res = NULL;
	sunxi_ss_t *sss = platform_get_drvdata(pdev);

#ifdef SS_IDMA_ENABLE
	int i;
	for (i=0; i<SS_FLOW_NUM; i++) {
		sss->flows[i].buf_src = (char *)kmalloc(SS_DMA_BUF_SIZE, GFP_KERNEL);
		if (sss->flows[i].buf_src == NULL) {
			SS_ERR("Can not allocate DMA source buffer\n");
			return -ENOMEM;
		}
		sss->flows[i].buf_src_dma = virt_to_phys(sss->flows[i].buf_src);

		sss->flows[i].buf_dst = (char *)kmalloc(SS_DMA_BUF_SIZE, GFP_KERNEL);
		if (sss->flows[i].buf_dst == NULL) {
			SS_ERR("Can not allocate DMA source buffer\n");
			return -ENOMEM;
		}
		sss->flows[i].buf_dst_dma = virt_to_phys(sss->flows[i].buf_dst);
		init_completion(&sss->flows[i].done);
	}
#endif

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		SS_ERR("Unable to get SS MEM resource\n");
		return -ENXIO;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		SS_ERR("No SS IRQ specified\n");
		return irq;
	}

	sss->irq = irq;

	ret = request_irq(sss->irq, sunxi_ss_irq_handler, IRQF_DISABLED, sss->dev_name, sss);
	if (ret != 0) {
		SS_ERR("Cannot request IRQ\n");
		return ret;
	}

	if (request_mem_region(mem_res->start,
			resource_size(mem_res), pdev->name) == NULL) {
		SS_ERR("Req mem region failed\n");
		return -ENXIO;
	}

	sss->base_addr = ioremap(mem_res->start, resource_size(mem_res));
	if (sss->base_addr == NULL) {
		SS_ERR("Unable to remap IO\n");
		return -ENXIO;
	}
	sss->base_addr_phy = mem_res->start;

	return 0;
}

/* Release the resource: IRQ, mem */
static int __devexit sunxi_ss_res_release(sunxi_ss_t *sss)
{
#ifdef SS_IDMA_ENABLE
	int i;
#endif
	struct resource	*mem_res = NULL;

	iounmap(sss->base_addr);
	mem_res = platform_get_resource(sss->pdev, IORESOURCE_MEM, 0);
	if (mem_res != NULL)
		release_mem_region(mem_res->start, resource_size(mem_res));

#ifdef SS_IDMA_ENABLE
	for (i=0; i<SS_FLOW_NUM; i++) {
		kfree(sss->flows[i].buf_src);
		kfree(sss->flows[i].buf_dst);
	}
#endif

	free_irq(sss->irq, sss);
	return 0;
}

static int sunxi_ss_hw_init(sunxi_ss_t *sss)
{
#ifdef CONFIG_EVB_PLATFORM
	int ret = 0;
#endif
	struct clk *pclk = NULL;

	pclk = clk_get(&sss->pdev->dev, SS_PLL_CLK);
	if (IS_ERR_OR_NULL(pclk)) {
		SS_ERR("Unable to acquire module clock '%s', return %x\n",
				SS_PLL_CLK, PTR_RET(pclk));
		return PTR_RET(pclk);
	}

	sss->mclk = clk_get(&sss->pdev->dev, sss->dev_name);
	if (IS_ERR_OR_NULL(sss->mclk)) {
		SS_ERR("Unable to acquire module clock '%s', return %x\n",
				sss->dev_name, PTR_RET(sss->mclk));
		return PTR_RET(sss->mclk);
	}

#ifdef CONFIG_EVB_PLATFORM
	ret = clk_set_parent(sss->mclk, pclk);
	if (ret != 0) {
		SS_ERR("clk_set_parent() failed! return %d\n", ret);
		return ret;
	}

	ret = clk_set_rate(sss->mclk, SS_CLK_RATE);
	if (ret != 0) {
		SS_ERR("clk_set_rate(%d) failed! return %d\n", SS_CLK_RATE, ret);
		return ret;
	}
#endif
	SS_DBG("SS mclk %luMHz, pclk %luMHz\n", clk_get_rate(sss->mclk)/1000000,
			clk_get_rate(pclk)/1000000);

	if (clk_prepare_enable(sss->mclk)) {
		SS_ERR("Couldn't enable module clock\n");
		return -EBUSY;
	}

	clk_put(pclk);
	return 0;
}

static int sunxi_ss_hw_exit(sunxi_ss_t *sss)
{
	clk_disable_unprepare(sss->mclk);
	clk_put(sss->mclk);
	sss->mclk = NULL;
	return 0;
}

static int sunxi_ss_alg_register(void)
{
	int i;
	int ret = 0;

	for (i=0; i<ARRAY_SIZE(sunxi_ss_algs); i++) {
		INIT_LIST_HEAD(&sunxi_ss_algs[i].cra_list);

		sunxi_ss_algs[i].cra_priority = 300;
		sunxi_ss_algs[i].cra_ctxsize = sizeof(ss_aes_ctx_t);
		sunxi_ss_algs[i].cra_module = THIS_MODULE;
		sunxi_ss_algs[i].cra_exit = sunxi_ss_cra_exit;
		if (strncmp(sunxi_ss_algs[i].cra_name, "prng", 4) == 0)
			sunxi_ss_algs[i].cra_init = sunxi_ss_cra_rng_init;
		else
			sunxi_ss_algs[i].cra_init = sunxi_ss_cra_init;

		ret = crypto_register_alg(&sunxi_ss_algs[i]);
		if (ret != 0) {
			SS_ERR("crypto_register_alg(%s) failed! return %d \n",
				sunxi_ss_algs[i].cra_name, ret);
			return ret;
		}
	}

	for (i=0; i<ARRAY_SIZE(sunxi_ss_algs_hash); i++) {
		sunxi_ss_algs_hash[i].halg.base.cra_priority = 300;
		ret = crypto_register_ahash(&sunxi_ss_algs_hash[i]);
		if (ret != 0) {
			SS_ERR("crypto_register_ahash(%s) failed! return %d \n",
				sunxi_ss_algs_hash[i].halg.base.cra_name, ret);
			return ret;
		}
	}

	return 0;
}

static void sunxi_ss_alg_unregister(void)
{
	int i;

	for (i=0; i<ARRAY_SIZE(sunxi_ss_algs); i++)
		crypto_unregister_alg(&sunxi_ss_algs[i]);

	for (i=0; i<ARRAY_SIZE(sunxi_ss_algs_hash); i++)
		crypto_unregister_ahash(&sunxi_ss_algs_hash[i]);
}

static ssize_t sunxi_ss_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	sunxi_ss_t *sss = platform_get_drvdata(pdev);

	return snprintf(buf, PAGE_SIZE,
		"pdev->id   = %d \n"
		"pdev->name = %s \n"
		"pdev->num_resources = %u \n"
		"pdev->resource.mem = [0x%08x, 0x%08x] \n"
		"pdev->resource.irq = %d \n"
		"SS module clk rate = %ld Mhz \n"
		"IO membase = 0x%p \n",
		pdev->id, pdev->name, pdev->num_resources,
		pdev->resource[0].start, pdev->resource[0].end,
		sss->irq,
		(clk_get_rate(sss->mclk)/1000000), sss->base_addr);
}
static struct device_attribute sunxi_ss_info_attr =
	__ATTR(info, S_IRUGO, sunxi_ss_info_show, NULL);

static ssize_t sunxi_ss_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	sunxi_ss_t *sss = platform_get_drvdata(pdev);
	char *avail[] = {"Available", "Unavailable"};

	if (sss == NULL)
		return snprintf(buf, PAGE_SIZE, "%s\n", "sunxi_ss_t is NULL!");

	buf[0] = 0;
	for (i=0; i<SS_FLOW_NUM; i++) {
		snprintf(buf+strlen(buf), PAGE_SIZE-strlen(buf),
			"The flow %d state: %s \n"
#ifdef SS_IDMA_ENABLE
			"    Src: 0x%p / 0x%08x \n"
			"    Dst: 0x%p / 0x%08x \n"
#endif
			, i, avail[sss->flows[i].available]
#ifdef SS_IDMA_ENABLE
			, sss->flows[i].buf_src, sss->flows[i].buf_src_dma
			, sss->flows[i].buf_dst, sss->flows[i].buf_dst_dma
#endif
		);
	}

	return strlen(buf) + ss_reg_print(buf + strlen(buf), PAGE_SIZE - strlen(buf));
}

static struct device_attribute sunxi_ss_status_attr =
	__ATTR(status, S_IRUGO, sunxi_ss_status_show, NULL);

static void sunxi_ss_sysfs_create(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_ss_info_attr);
	device_create_file(&_pdev->dev, &sunxi_ss_status_attr);
}

static void sunxi_ss_sysfs_remove(struct platform_device *_pdev)
{
	device_remove_file(&_pdev->dev, &sunxi_ss_info_attr);
	device_remove_file(&_pdev->dev, &sunxi_ss_status_attr);
}

static int __devinit sunxi_ss_probe(struct platform_device *pdev)
{
	int ret = 0;
	sunxi_ss_t *sss = NULL;

	sss = devm_kzalloc(&pdev->dev, sizeof(sunxi_ss_t), GFP_KERNEL);
	if (sss == NULL) {
		SS_ERR("Unable to allocate sunxi_ss_t\n");
		return -ENOMEM;
	}

	snprintf(sss->dev_name, sizeof(sss->dev_name), SUNXI_SS_DEV_NAME);
	platform_set_drvdata(pdev, sss);

	ret = sunxi_ss_res_request(pdev);
	if (ret != 0) {
		goto err0;
	}

    sss->pdev = pdev;

	ret = sunxi_ss_hw_init(sss);
	if (ret != 0) {
		SS_ERR("SS hw init failed!\n");
		goto err1;
	}

	ret = sunxi_ss_alg_register();
	if (ret != 0) {
		SS_ERR("sunxi_ss_alg_register() failed! return %d \n", ret);
		goto err2;
	}

	sunxi_ss_sysfs_create(pdev);

	ss_dev = sss;
	SS_DBG("SS driver probe succeed, base 0x%p, irq %d!\n", sss->base_addr, sss->irq);
	return 0;

err2:
	sunxi_ss_hw_exit(sss);
err1:
	sunxi_ss_res_release(sss);
err0:
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int __devexit sunxi_ss_remove(struct platform_device *pdev)
{
	sunxi_ss_t *sss = platform_get_drvdata(pdev);

	ss_wait_idle();
	sunxi_ss_sysfs_remove(pdev);

	sunxi_ss_alg_unregister();
	sunxi_ss_hw_exit(sss);
	sunxi_ss_res_release(sss);

	platform_set_drvdata(pdev, NULL);
	ss_dev = NULL;
	return 0;
}

static void sunxi_ss_release(struct device *dev)
{
	SS_ENTER();
}

#ifdef CONFIG_PM
static int sunxi_ss_suspend(struct device *dev)
{
#ifdef CONFIG_EVB_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	sunxi_ss_t *sss = platform_get_drvdata(pdev);
	unsigned long flags = 0;

	SS_ENTER();

	/* Wait for the completion of SS operation. */
	ss_dev_lock();

	spin_lock_irqsave(&ss_dev->lock, flags);
	sss->suspend = 1;
	spin_unlock_irqrestore(&sss->lock, flags);

	sunxi_ss_hw_exit(sss);
	ss_dev_unlock();
#endif

	return 0;
}

static int sunxi_ss_resume(struct device *dev)
{
	int ret = 0;
#ifdef CONFIG_EVB_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	sunxi_ss_t *sss = platform_get_drvdata(pdev);
	unsigned long flags = 0;

	SS_ENTER();
	ret = sunxi_ss_hw_init(sss);
	spin_lock_irqsave(&ss_dev->lock, flags);
	sss->suspend = 0;
	spin_unlock_irqrestore(&sss->lock, flags);
#endif
	return ret;
}

static const struct dev_pm_ops sunxi_ss_dev_pm_ops = {
	.suspend = sunxi_ss_suspend,
	.resume  = sunxi_ss_resume,
};

#define SUNXI_SS_DEV_PM_OPS (&sunxi_ss_dev_pm_ops)
#else
#define SUNXI_SS_DEV_PM_OPS NULL
#endif /* CONFIG_PM */

static struct platform_driver sunxi_ss_driver = {
	.probe   = sunxi_ss_probe,
	.remove  = sunxi_ss_remove,
	.driver = {
        .name	= SUNXI_SS_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm		= SUNXI_SS_DEV_PM_OPS,
	},
};

static struct resource sunxi_ss_resources[] = {
	{
		.start	= SUNXI_SS_MEM_START,
		.end 	= SUNXI_SS_MEM_END,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= SUNXI_SS_IRQ,
		.end 	= SUNXI_SS_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device sunxi_ss_device = {
	.name = SUNXI_SS_DEV_NAME,
	.resource = sunxi_ss_resources,
	.num_resources = 2,
	.dev.release = sunxi_ss_release,
};

static int __init sunxi_ss_init(void)
{
    int ret = 0;

	SS_DBG("[%s %s]Sunxi SS init ... \n", __DATE__, __TIME__);

	ret = platform_driver_register(&sunxi_ss_driver);
	if (ret < 0) {
		SS_ERR("platform_driver_register() failed, return %d\n", ret);
		return ret;
	}

	ret = platform_device_register(&sunxi_ss_device);
	if (ret < 0) {
		SS_ERR("platform_device_register() failed, return %d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit sunxi_ss_exit(void)
{
	platform_device_unregister(&sunxi_ss_device);
    platform_driver_unregister(&sunxi_ss_driver);
}

module_init(sunxi_ss_init);
module_exit(sunxi_ss_exit);

MODULE_AUTHOR("mintow");
MODULE_DESCRIPTION("SUNXI SS Controller Driver");
MODULE_ALIAS("platform:"SUNXI_SS_DEV_NAME);
MODULE_LICENSE("GPL");

