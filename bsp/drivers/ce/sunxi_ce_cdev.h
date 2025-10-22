/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Some macro and struct of SUNXI SecuritySystem controller.
 *
 * Copyright (C) 2013 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _SUNXI_CE_CDEV_H_
#define _SUNXI_CE_CDEV_H_

#include <crypto/internal/akcipher.h>
#include <crypto/aes.h>
#include <crypto/sha1.h>
#include <crypto/sha2.h>
#include <crypto/sha3.h>
#include <crypto/hash.h>
#include <crypto/rng.h>
#include <crypto/internal/skcipher.h>

#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>

#define SUNXI_SS_DEV_NAME		"ce"
#define SUNXI_CE_DEV_NODE_NAME		"allwinner,sunxi-ce"

/* flag for sunxi_ss_t.flags */
#define SS_FLAG_MODE_MASK		0xFF
#define SS_FLAG_NEW_KEY			BIT(0)
#define SS_FLAG_NEW_IV			BIT(1)
#define SS_FLAG_INIT			BIT(2)
#define SS_FLAG_FAST			BIT(3)
#define SS_FLAG_BUSY			BIT(4)
#define SS_FLAG_TRNG			BIT(8)

/* flag for crypto_async_request.flags */
#define SS_FLAG_AES			BIT(16)
#define SS_FLAG_HASH			BIT(17)

/* define CE_V1_X---->CE_DRIVER_V3_1 */
#if IS_ENABLED(CONFIG_ARCH_SUN20IW1) || IS_ENABLED(CONFIG_ARCH_SUN8IW11)\
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW1) || IS_ENABLED(CONFIG_ARCH_SUN50IW2)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW12) || IS_ENABLED(CONFIG_ARCH_SUN8IW15)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW7) || IS_ENABLED(CONFIG_ARCH_SUN50IW8)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW17) || IS_ENABLED(CONFIG_ARCH_SUN8IW18)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW16) || IS_ENABLED(CONFIG_ARCH_SUN8IW19)\
	|| IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN50IW11)\

#define SS_SUPPORT_CE_V3_1		1
#define SS_SCATTER_ENABLE		1
#define	TASK_DMA_POOL			1
#define SS_SHA_SWAP_PRE_ENABLE		1 /* The initial IV need to be converted. */
#define TASK_MAX_DATA_SIZE		(1 * 1024 * 1024 *1024)
#endif

/* define CE_V2_X---->CE_DRIVER_V3_2 */
#if IS_ENABLED(CONFIG_ARCH_SUN50IW3) || IS_ENABLED(CONFIG_ARCH_SUN50IW6) || \
	IS_ENABLED(CONFIG_ARCH_SUN8IW15) || IS_ENABLED(CONFIG_ARCH_SUN50IW8) || \
	IS_ENABLED(CONFIG_ARCH_SUN8IW16) || IS_ENABLED(CONFIG_ARCH_SUN50IW9) || \
	IS_ENABLED(CONFIG_ARCH_SUN8IW19) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)

#define SS_SUPPORT_CE_V3_2		1
#define SS_SCATTER_ENABLE		1
#define	TASK_DMA_POOL			1
#define SS_SHA_SWAP_PRE_ENABLE		1 /* The initial IV need to be converted. */
#endif

/* define CE_V2.3---->CE_DRIVER_V5 */
#if IS_ENABLED(CONFIG_ARCH_SUN50IW12) || IS_ENABLED(CONFIG_ARCH_SUN8IW21) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) || \
					IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)

#define SS_SUPPORT_CE_V5		1
#define SS_GCM_MODE_ENABLE		1
#define SS_SCATTER_ENABLE		1
#define	TASK_DMA_POOL			1
#define SS_SHA_SWAP_PRE_ENABLE		1 /* The initial IV need to be converted. */
#define	CE_BYTE_ADDR_ENABLE		1

#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
#define SET_CE_CLKFRE_MODE2		1
#define TASK_MAX_DATA_SIZE		(1 * 1024 * 1024 *1024)
#endif
#endif

/* ce_2.x bug: task must be less 127*1024 */
#ifndef TASK_MAX_DATA_SIZE
#define TASK_MAX_DATA_SIZE		(127 * 1024)
#endif

#if defined(SS_SUPPORT_CE_V3_1) || defined(SS_SUPPORT_CE_V3_2) || defined(SS_SUPPORT_CE_V5)
#define SS_SHA224_ENABLE		1
#define SS_SHA256_ENABLE		1
#define SS_SHA384_ENABLE		1
#define SS_SHA512_ENABLE		1
#define SS_HMAC_ENABLE			1
#define SS_HMAC_SHA1_ENABLE		1
#define SS_HMAC_SHA256_ENABLE		1

#define SS_RSA512_ENABLE		1
#define SS_RSA1024_ENABLE		1
#define SS_RSA2048_ENABLE		1

#define SS_DH512_ENABLE			1
#define SS_DH1024_ENABLE		1
#define SS_DH2048_ENABLE		1

#define SS_ECC_ENABLE			1
#endif	/* SS_SUPPORT_CE_V3_1 */

#define SS_TRNG_ENABLE			1
#define SS_TRNG_POSTPROCESS_ENABLE	1

#if defined(SS_SUPPORT_CE_V3_2)
#if IS_ENABLED(CONFIG_ARCH_SUN50IW10)
#define SS_GCM_MODE_ENABLE		1
#define SS_DRBG_MODE_ENABLE             1
#undef SS_TRNG_POSTPROCESS_ENABLE
#endif
#define SS_HASH_HW_PADDING_ALIGN_CASE	1
#endif  /* SS_SUPPORT_CE_V3_2 */

#if defined(SS_SUPPORT_CE_V3_2) || defined(SS_SUPPORT_CE_V5)
#define SS_RSA3072_ENABLE		1
#define SS_RSA4096_ENABLE		1

#define SS_DH3072_ENABLE		1
#define SS_DH4096_ENABLE		1

#define SS_HASH_HW_PADDING		1
#endif	/* SS_SUPPORT_CE_V3_2 || defined(SS_SUPPORT_CE_V5) */

#define SS_RSA_MIN_SIZE			(512/8)  /* in Bytes. 512 bits */
#define SS_RSA_MAX_SIZE			(4096/8) /* in Bytes. 4096 bits */
#define SS_FLOW_NUM			4

#define SS_XTS_MODE_ENABLE              1

#if defined(SS_RSA512_ENABLE) || defined(SS_RSA1024_ENABLE) \
	|| defined(SS_RSA2048_ENABLE) || defined(SS_RSA3072_ENABLE) \
	|| defined(SS_RSA4096_ENABLE)
#define SS_RSA_ENABLE			1
#endif

#if defined(SS_DH512_ENABLE) || defined(SS_DH1024_ENABLE) \
	|| defined(SS_DH2048_ENABLE) || defined(SS_DH3072_ENABLE) \
	|| defined(SS_DH4096_ENABLE)
#define SS_DH_ENABLE			1
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#define SS_LPKG_KEEP_ON
#undef SS_XTS_MODE_ENABLE
#undef SS_RSA_ENABLE
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN8IW21) \
	|| IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)
#undef SS_RSA_ENABLE
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define SS_TOTAL_DATALEN_ENABLE	1
#undef SS_RSA_ENABLE
#endif

#define SS_PRNG_SEED_LEN		(192/8) /* 192 bits */
#define SS_RNG_MAX_LEN			SZ_8K

#if defined(SS_SHA384_ENABLE) || defined(SS_SHA512_ENABLE)
#define SS_DIGEST_SIZE			SHA512_DIGEST_SIZE
#define SS_HASH_PAD_SIZE		(SHA512_BLOCK_SIZE * 2)
#else
#define SS_DIGEST_SIZE			SHA256_DIGEST_SIZE
#define SS_HASH_PAD_SIZE		(SHA1_BLOCK_SIZE * 2)
#endif

/*
 * 10s, The waittimeout time of ce needs to be longer, because rsa will take
 * time to calculate a large amount of data; and the frequency of fpga stage
 * is much lower than that of ic board, setting too small will lead to wrong timeout.
 */
#define SS_WAIT_TIME			10000

#define SS_ALG_PRIORITY			260

#if IS_ENABLED(CONFIG_ARCH_SUN50IW9) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)
#define WORD_ALGIN			(2)
#else
#define WORD_ALGIN			(0)
#endif

/* For debug */
/* #define SUNXI_CE_DEBUG */
#ifdef SUNXI_CE_DEBUG
#define SS_DBG(fmt, arg...) 	pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)
#else
#define SS_DBG(fmt, arg...) 	pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#endif  /* SUNXI_CE_DEBUG */

#define SS_ERR(fmt, arg...) 	pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SS_EXIT()	    	SS_DBG("%s\n", "Exit")
#define SS_ENTER()	    	SS_DBG("%s\n", "Enter ...")

#define SS_FLOW_AVAILABLE	0
#define SS_FLOW_UNAVAILABLE	1

#define SS_RES_NS_INDEX	0
#define SS_RES_S_INDEX	1
#define SS_RES_INDEX	SS_RES_NS_INDEX

#ifdef SS_SCATTER_ENABLE
/* CE: Crypto Engine, start using CE from sun8iw7/sun8iw9 */
#define CE_SCATTERS_PER_TASK	8

#define CE_ADDR_MASK		0xffffffffff

/* The descriptor of a CE task. */
#ifdef CE_BYTE_ADDR_ENABLE
typedef struct {
	u8 src_addr[5];
	u8 dst_addr[5];
	u8 pad[2];
	u32 src_len;
	u32 dst_len;
} ce_scatter_t;

typedef struct ce_task_desc {
	u32 chan_id;
	u32 comm_ctl;
	u32 sym_ctl;
	u32 asym_ctl;

	u8 key_addr[5];
	u8 iv_addr[5];
	u8 ctr_addr[5];
	u8 pad;
	u32 data_len; /* in word(4 byte). Exception: in byte for AES_CTS */

	ce_scatter_t ce_sg[CE_SCATTERS_PER_TASK];

	u8 next_sg_addr[5];
	u8 next_task_addr[5];
	u8 pad2[2];
	u32 reserved[3];
	struct ce_task_desc *next_virt;
	dma_addr_t task_phy_addr;
} ce_task_desc_t;

/* The descriptor of a CE hash and rng task. */
typedef struct ce_new_task_desc {
	u32 comm_ctl;
	u32 main_cmd;
	u8 data_len[5];
	u8 key_addr[5];
	u8 iv_addr[5];
	u8 pad;

	ce_scatter_t ce_sg[CE_SCATTERS_PER_TASK];

	u8 next_sg_addr[5];
	u8 next_task_addr[5];
	u8 pad2[2];
	u32 reserved[3];
	struct ce_new_task_desc *next_virt;
	dma_addr_t task_phy_addr;
} ce_new_task_desc_t;
#else
typedef struct {
	u32 addr;
	u32 len; /* in word (4 bytes). Exception: in byte for AES_CTS */
} ce_scatter_t;

/* The descriptor of a CE task. */
typedef struct ce_task_desc {
	u32 chan_id;
	u32 comm_ctl;
	u32 sym_ctl;
	u32 asym_ctl;

	u32 key_addr;
	u32 iv_addr;
	u32 ctr_addr;
	u32 data_len; /* in word(4 byte). Exception: in byte for AES_CTS */

	ce_scatter_t src[CE_SCATTERS_PER_TASK];
	ce_scatter_t dst[CE_SCATTERS_PER_TASK];

	/* struct ce_task_desc *next; */
	u32 next_task_addr;
	u32 reserved[3];
	struct ce_task_desc *next_virt;
	dma_addr_t task_phy_addr;
} ce_task_desc_t;

/* The descriptor of a CE hash and rng task. */
typedef struct ce_new_task_desc {
	u32 common_ctl;
	u32 main_cmd;
	u32 data_len;
	u32 key_addr;
	u32 iv_addr;

	ce_scatter_t src[CE_SCATTERS_PER_TASK];
	ce_scatter_t dst[CE_SCATTERS_PER_TASK];
	struct ce_new_task_desc *next;
	u32 reserved[3];
} ce_new_task_desc_t;
#endif
#endif

struct ce_scatter_mapping {
	void *virt_addr;
	u32 data_len;
};

typedef struct {
	u32 dir;
	u32 nents;
	struct dma_chan *chan;
	struct scatterlist *sg;
#ifdef SS_SCATTER_ENABLE
	u32 has_padding;
	u8 *padding;
	struct scatterlist *last_sg;
	struct ce_scatter_mapping mapping[CE_SCATTERS_PER_TASK];
#endif
} ss_dma_info_t;

typedef struct {
	u32 dir;
	u32 type;
	u32 mode;
	u32 bitwidth;	/* the bitwidth of CFB mode */
	struct completion done;
	ss_dma_info_t dma_src;
	ss_dma_info_t dma_dst;
} ss_aes_req_ctx_t;

/* The common context of AES and HASH */
typedef struct {
	u32 flow;
	u32 flags;
} ss_comm_ctx_t;

typedef struct {
	ss_comm_ctx_t comm; /* must be in the front. */

#ifdef SS_RSA_ENABLE
	u8 key[SS_RSA_MAX_SIZE];
	u8 iv[SS_RSA_MAX_SIZE];
#else
	u8 key[AES_MAX_KEY_SIZE];
	u8 iv[AES_MAX_KEY_SIZE];
#endif
#ifdef SS_SCATTER_ENABLE
	u8 next_iv[AES_MAX_KEY_SIZE]; /* the next IV/Counter in continue mode */
#endif

	u32 key_size;
	u32 iv_size;
	u32 cnt;	/* in Byte */
} ss_aes_ctx_t;

#ifdef SS_DRBG_MODE_ENABLE
typedef struct {
	ss_comm_ctx_t comm; /* must be in the front. */

	u8 person[AES_MAX_KEY_SIZE];	/* in drbg, means personalization input */
	u32 person_size;
	u8 nonce[AES_MAX_KEY_SIZE];
	u32 nonce_size;
	u8 entropt[AES_MAX_KEY_SIZE];
	u32 entropt_size;

	u32 cnt;	/* in Byte */
} ss_drbg_ctx_t;

#endif

typedef struct {
	ss_comm_ctx_t comm; /* must be in the front. */

	u8 key[AES_MAX_KEY_SIZE];
	u8 iv[AES_MAX_KEY_SIZE];

#ifdef SS_SCATTER_ENABLE
	u8 next_iv[AES_MAX_KEY_SIZE]; /* the next IV/Counter in continue mode */
#endif

	u8 task_iv[88];	/* max size: pt_iv(4 words) || tag_iv(4 words) ||
					   ghash_y(4 words) || tag(4 words) || iv_size(2 words)
					   || aad_size(2 words) || pt_size(2 words)*/
	u8 task_ctr[48];	/* max size when not last package: pt_iv(4 words) ||
						   tag_iv(4 words) || ghash_y(4 words) */
	u8 tag[16];		/* to store the tag data */

	struct scatterlist *aad_addr;
	u32 key_size;
	u32 iv_size;
	u32 tag_len;
	u32 assoclen;
	u32 cryptlen;

	u32 cnt;	/* in Byte */
} ss_aead_ctx_t;

typedef struct {
	ss_comm_ctx_t comm; /* must be in the front. */

	u8  md[SS_DIGEST_SIZE]; /* message digest data which fixed-length output obtained by applying a hash function to the message */
	u8  pad[SS_HASH_PAD_SIZE]; /* Padding data, adding this to a message to meet the requirements of a hash algorithm */
	u8  key[SHA512_BLOCK_SIZE]; /* for hmac */
	u32 tail_len;
	u32 md_size;
	u32 key_size;	/* for hmac */
	u32 cnt;	/* in Byte */
	u32 npackets;
} ss_hash_ctx_t;

typedef struct {
#ifdef SS_SCATTER_ENABLE
	ce_task_desc_t task;
#endif
	struct completion done;
	u32 available;
} ss_flow_t;

typedef struct {
#ifdef SS_SCATTER_ENABLE
	ce_task_desc_t task;
#endif
	struct completion done;
	u32 available;
	u32 buf_pendding;
} ce_channel_t;

typedef struct {
	struct platform_device *pdev;
	struct cdev *pcdev;
#if IS_ENABLED(CONFIG_AW_HWRNG_DRIVER)
	struct sunxi_hwrng *hwrng;
#endif
	dev_t devid;
	struct class *pclass;
	struct device *pdevice;
	struct device_node *pnode;
	void __iomem *base_addr;
	ce_channel_t flows[SS_FLOW_NUM];
	struct clk *ce_clk;
	struct clk *ce_sys_clk;
	struct clk *pclk;
	struct clk *bus_clk;
	struct clk *mbus_clk;
	struct reset_control *reset;
	s8  dev_name[8];
	spinlock_t lock;
	u32 gen_clkrate;
	u32 rsa_clkrate;
	u32 flag;
	u32 irq;
	s32 suspend;
	struct dma_pool	*task_pool;
} sunxi_ce_cdev_t;

extern sunxi_ce_cdev_t	*ce_cdev;
extern sunxi_ce_cdev_t  *ss_dev;

/* define the ctx for aes requtest */
typedef struct {
	u32 bit_width;  /* the bitwidth of CFB/CTR mode */
	u32 method;  /* symm method, such as AES/DES/3DES ... */
	u8 *src_buffer;
	u32 src_length;
	u8 *dst_buffer;
	u32 dst_length;
	u8 *key_buffer;
	u32 key_length;
	u8 *iv_buf;
	u32 iv_length;
	u32 aes_mode;
	u32 dir;
	u32 ion_flag;
	unsigned long src_phy;
	unsigned long dst_phy;
	unsigned long iv_phy;
	unsigned long key_phy;
	s32 channel_id;
} crypto_aes_req_ctx_t;

/* define the ctx for rsa requtest */
typedef struct {
	u8 *sign_buffer;
	u32 sign_length;
	u8 *pkey_buffer;
	u32 pkey_length;
	u8 *dst_buffer;
	u32 dst_length;
	u32 dir;
	u32 rsa_width;
	u32 flag;
	s32 channel_id;
} crypto_rsa_req_ctx_t;

/* define the ctx for hash requtest */
typedef struct {
	u8 *text_buffer;
	u32 text_length;
	u8 *key_buffer;  /* hmac */
	u32 key_length;
	u8 *dst_buffer;
	u32 dst_length;
	u8 *iv_buffer;
	u32 iv_length;
	u32 hash_mode;
	u32 ion_flag;
	unsigned long text_phy;
	unsigned long dst_phy;
	s32 channel_id;
} crypto_hash_req_ctx_t;

typedef struct {
	u8 *src_buffer;
	u32 src_length;

	u8 *dst_buffer;
	u32 dst_length;

	u8 *key_buffer;
	u32 key_length;

	u8 *iv_buffer;
	u32 iv_length;

	u32 trng;
	u32 reload_offset;

	unsigned long src_phy;
	unsigned long dst_phy;
	unsigned long iv_phy;
	unsigned long key_phy;
	s32 channel_id;
} crypto_rng_req_ctx_t;

typedef struct {
	u8 *src_buffer;
	u32 src_length;

	u8 *dst_buffer;
	u32 dst_length;

	u8 *key_buffer;
	u32 key_length;

	u8 *iv_buffer;
	u32 iv_length;

	u8 *sk_buffer;
	u32 sk_length;

	u8 *p_buffer;
	u32 p_length;

	u32 mode;
	u32 dir;
	u32 width;  /* 160/224/256/384/512 */
	u32 flag;

	s32 channel_id;
} crypto_ecc_req_ctx_t;

enum alg_type {
	ALG_TYPE_HASH,
	ALG_TYPE_CIPHER,
	ALG_TYPE_RNG,
	ALG_TYPE_ASYM,
};

struct sunxi_crypto_tmp {
	union {
		struct skcipher_alg	skcipher;
		struct ahash_alg	hash;
		struct rng_alg		rng;
		struct akcipher_alg     asym;
	} alg;
	enum alg_type			type;
};

/* ioctl cmd */
#define CE_IOC_MAGIC			'C'
#define CE_IOC_REQUEST			_IOR(CE_IOC_MAGIC, 0, int)
#define CE_IOC_FREE			_IOW(CE_IOC_MAGIC, 1, int)
#define CE_IOC_AES_CRYPTO		_IOW(CE_IOC_MAGIC, 2, crypto_aes_req_ctx_t)
#define CE_IOC_RSA_CRYPTO		_IOW(CE_IOC_MAGIC, 3, crypto_rsa_req_ctx_t)
#define CE_IOC_HASH_CRYPTO		_IOW(CE_IOC_MAGIC, 4, crypto_hash_req_ctx_t)
#define CE_IOC_RNG_CRYPTO		_IOW(CE_IOC_MAGIC, 5, crypto_rng_req_ctx_t)
#define CE_IOC_ECC_CRYPTO		_IOW(CE_IOC_MAGIC, 6, crypto_ecc_req_ctx_t)

/* Inner functions declaration */
void ce_dev_lock(void);
void ce_dev_unlock(void);
void ss_dev_lock(void);
void ss_dev_unlock(void);
void ce_reset(void);
void ss_reset(void);
void ss_clk_set(u32 rate);
void print_hex(void *_data, int _len);
void __iomem *ss_membase(void);
int do_aes_crypto(crypto_aes_req_ctx_t *req);
int do_rsa_crypto(crypto_rsa_req_ctx_t *req);
int do_hash_crypto(crypto_hash_req_ctx_t *req_ctx);
int do_rng_crypto(crypto_rng_req_ctx_t *req_ctx);
int do_ecc_crypto(crypto_ecc_req_ctx_t *req_ctx);
irqreturn_t sunxi_ce_irq_handler(int irq, void *dev_id);

#if IS_ENABLED(CONFIG_AW_HWRNG_DRIVER)
int sunxi_register_hwrng(sunxi_ce_cdev_t *ce_dev);
#endif

#endif /* end of _SUNXI_CE_CDEV_H_ */
