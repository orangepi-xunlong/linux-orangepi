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

#include <crypto/aes.h>
#include <crypto/sha.h>
#include <crypto/hash.h>

#include <linux/scatterlist.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>

#define SUNXI_SS_DEV_NAME		"ce"
#define SUNXI_CE_DEV_NODE_NAME	"allwinner,sunxi-ce"

/* flag for sunxi_ss_t.flags */
#define SS_FLAG_MODE_MASK		0xFF
#define SS_FLAG_NEW_KEY			BIT(0)
#define SS_FLAG_NEW_IV			BIT(1)
#define SS_FLAG_INIT			BIT(2)
#define SS_FLAG_FAST			BIT(3)
#define SS_FLAG_BUSY			BIT(4)
#define SS_FLAG_TRNG			BIT(8)

/* flag for crypto_async_request.flags */
#define SS_FLAG_AES				BIT(16)
#define SS_FLAG_HASH			BIT(17)

/* Define the capability of SS controller. */
#ifdef CONFIG_ARCH_SUN8IW6
#define SS_CTR_MODE_ENABLE		1
#define SS_CTS_MODE_ENABLE		1
#define SS_SHA224_ENABLE		1
#define SS_SHA256_ENABLE		1
#define SS_TRNG_ENABLE			1
#define SS_TRNG_POSTPROCESS_ENABLE	1
#define SS_RSA512_ENABLE		1
#define SS_RSA1024_ENABLE		1
#define SS_RSA2048_ENABLE		1
#define SS_RSA3072_ENABLE		1
#define SS_IDMA_ENABLE			1
#define SS_MULTI_FLOW_ENABLE	1

#define SS_SHA_SWAP_PRE_ENABLE	1 /* The initial IV need to be converted. */

#define SS_DMA_BUF_SIZE			SZ_8K

#define SS_RSA_MIN_SIZE			(512/8)  /* in Bytes. 512 bits */
#define SS_RSA_MAX_SIZE			(3072/8) /* in Bytes. 3072 bits */

#define SS_FLOW_NUM				2
#endif

/*define CE_V1_X---->CE_DRIVER_V3_1*/
#if defined(CONFIG_ARCH_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW11)\
	|| defined(CONFIG_ARCH_SUN50IW1) || defined(CONFIG_ARCH_SUN50IW2)\
	|| defined(CONFIG_ARCH_SUN8IW12) || defined(CONFIG_ARCH_SUN8IW15)\
	|| defined(CONFIG_ARCH_SUN8IW7) || defined(CONFIG_ARCH_SUN50IW8)\
	|| defined(CONFIG_ARCH_SUN8IW17) || defined(CONFIG_ARCH_SUN8IW18)\
	|| defined(CONFIG_ARCH_SUN8IW16) || defined(CONFIG_ARCH_SUN8IW19)\
	|| defined(CONFIG_ARCH_SUN8IW20) || defined(CONFIG_ARCH_SUN50IW11)\

#define SS_SUPPORT_CE_V3_1		1
#define SS_SCATTER_ENABLE		1
#define	TASK_DMA_POOL			1
#define SS_SHA_SWAP_PRE_ENABLE	1 /* The initial IV need to be converted. */
#endif

/*define CE_V2_X---->CE_DRIVER_V3_2*/
#if defined(CONFIG_ARCH_SUN50IW3) || defined(CONFIG_ARCH_SUN50IW6) || \
	defined(CONFIG_ARCH_SUN8IW15) || defined(CONFIG_ARCH_SUN50IW8) || \
	defined(CONFIG_ARCH_SUN8IW16) || defined(CONFIG_ARCH_SUN50IW9) || \
	defined(CONFIG_ARCH_SUN8IW19) || defined(CONFIG_ARCH_SUN50IW10) || \
	defined(CONFIG_ARCH_SUN50IW12) || defined(CONFIG_ARCH_SUN8IW21)

#define SS_SUPPORT_CE_V3_2		1
#define SS_SCATTER_ENABLE		1
#define	TASK_DMA_POOL			1

#define SS_SHA_SWAP_PRE_ENABLE	1 /* The initial IV need to be converted. */

#if defined(CONFIG_ARCH_SUN50IW12) || defined(CONFIG_ARCH_SUN8IW21)
#define	CE_BYTE_ADDR_ENABLE
#endif

#endif

#ifdef CONFIG_EVB_PLATFORM
#define SS_TRNG_ENABLE				1
#define SS_TRNG_POSTPROCESS_ENABLE	1
#endif

#if defined(SS_SUPPORT_CE_V3_1) || defined(SS_SUPPORT_CE_V3_2)
#define SS_CTR_MODE_ENABLE		1
#define SS_CTS_MODE_ENABLE		1
#define SS_OFB_MODE_ENABLE		1
#define SS_CFB_MODE_ENABLE		1

#define SS_SHA224_ENABLE		1
#define SS_SHA256_ENABLE		1
#define SS_SHA384_ENABLE		1
#define SS_SHA512_ENABLE		1
#define SS_HMAC_SHA1_ENABLE		1
#define SS_HMAC_SHA256_ENABLE	1

#define SS_RSA512_ENABLE		1
#define SS_RSA1024_ENABLE		1
#define SS_RSA2048_ENABLE		1

#define SS_DH512_ENABLE			1
#define SS_DH1024_ENABLE		1
#define SS_DH2048_ENABLE		1

#define SS_ECC_ENABLE			1
#endif	/*SS_SUPPORT_CE_V3_1*/


#if defined(SS_SUPPORT_CE_V3_2)
#define SS_XTS_MODE_ENABLE		1

#define SS_RSA3072_ENABLE		1
#define SS_RSA4096_ENABLE		1

#define SS_DH3072_ENABLE		1
#define SS_DH4096_ENABLE		1

#define SS_HASH_HW_PADDING		1
#define SS_HASH_HW_PADDING_ALIGN_CASE	1

#if defined(CONFIG_ARCH_SUN50IW8) || defined(CONFIG_ARCH_SUN50IW10) || \
	defined(CONFIG_ARCH_SUN50IW12) || defined(CONFIG_ARCH_SUN8IW21)

#define SS_GCM_MODE_ENABLE	1
#define SS_DRBG_MODE_ENABLE	1
#endif

#undef SS_TRNG_POSTPROCESS_ENABLE

#endif	/*SS_SUPPORT_CE_V3_2*/


#define SS_RSA_MIN_SIZE			(512/8)  /* in Bytes. 512 bits */

#if defined(CONFIG_ARCH_SUN8IW7)
#define SS_RSA_MAX_SIZE			(4096/8) /*in Bytes. 4096 bits*/
#else
#define SS_RSA_MAX_SIZE			(2048/8) /* in Bytes. 2048 bits */
#endif

#ifdef CONFIG_ARCH_SUN8IW7
#define SS_FLOW_NUM			1
#else
#define SS_FLOW_NUM				4
#endif

#if defined(CONFIG_ARCH_SUN8IW7)
#define SS_ECC_ENABLE                   1
#define SS_RSA_PREPROCESS_ENABLE        1
#define SS_RSA_CLK_ENABLE		1
#endif

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

#define SS_PRNG_SEED_LEN		(192/8) /* 192 bits */
#define SS_RNG_MAX_LEN			SZ_8K


#if defined(SS_SHA384_ENABLE) || defined(SS_SHA512_ENABLE)
#define SS_DIGEST_SIZE		SHA512_DIGEST_SIZE
#define SS_HASH_PAD_SIZE	(SHA512_BLOCK_SIZE * 2)
#else
#define SS_DIGEST_SIZE		SHA256_DIGEST_SIZE
#define SS_HASH_PAD_SIZE	(SHA1_BLOCK_SIZE * 2)
#endif

#ifdef CONFIG_EVB_PLATFORM
#define SS_WAIT_TIME	2000 /* 2s, used in wait_for_completion_timeout() */
#else
#define SS_WAIT_TIME	40000 /* 40s, used in wait_for_completion_timeout() */
#endif

#define SS_ALG_PRIORITY		260

#if defined(CONFIG_ARCH_SUN50IW9) || defined(CONFIG_ARCH_SUN50IW10)
#define WORD_ALGIN		(2)
#else
#define WORD_ALGIN		(0)
#endif


/* For debug */
#define SS_DBG(fmt, arg...) pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SS_ERR(fmt, arg...) pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SS_EXIT()	    SS_DBG("%s\n", "Exit")
#define SS_ENTER()	    SS_DBG("%s\n", "Enter ...")

#define SS_FLOW_AVAILABLE	0
#define SS_FLOW_UNAVAILABLE	1

#define SS_RES_NS_INDEX	0
#define SS_RES_S_INDEX	1
#ifdef CONFIG_EVB_PLATFORM
#define SS_RES_INDEX	SS_RES_NS_INDEX
#else
#define SS_RES_INDEX	SS_RES_NS_INDEX
#endif


#ifdef SS_SCATTER_ENABLE
/* CE: Crypto Engine, start using CE from sun8iw7/sun8iw9 */
#define CE_SCATTERS_PER_TASK		8

#define CE_ADDR_MASK	0xffffffffff

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
	struct ce_task_desc *next_virt;
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

	//struct ce_task_desc *next;
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
#ifdef SS_IDMA_ENABLE
	struct sg_table sgt_for_cp; /* Used to copy data from/to user space. */
#endif
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
#ifdef SS_CFB_MODE_ENABLE
	u32 bitwidth;	/* the bitwidth of CFB mode */
#endif
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

typedef struct {
	ss_comm_ctx_t comm; /* must be in the front. */

	u8  md[SS_DIGEST_SIZE];
	u8  pad[SS_HASH_PAD_SIZE];
	u32 tail_len;
	u32 md_size;
	u32 cnt;	/* in Byte */
} ss_hash_ctx_t;

typedef struct {
#ifdef SS_IDMA_ENABLE
	char *buf_src;
	dma_addr_t buf_src_dma;
	char *buf_dst;
	dma_addr_t buf_dst_dma;
#endif
#ifdef SS_SCATTER_ENABLE
	ce_task_desc_t task;
#endif
	struct completion done;
	u32 available;
} ss_flow_t;

typedef struct {
#ifdef SS_SCATTER_ENABLE
	ce_task_desc_t *task;
#endif
	struct completion done;
	u32 available;
	u32 buf_pendding;
} ce_channel_t;

typedef struct {
	struct cdev *pcdev;
	dev_t devid;
	struct class *pclass;
	struct device *pdevice;
	struct device_node *pnode;
	void __iomem *base_addr;
	ce_channel_t flows[SS_FLOW_NUM];
	struct clk *ce_clk;
	struct reset_control *reset;
	s8  dev_name[8];
	spinlock_t lock;
	u32 flag;
	u32 irq;
	struct dma_pool	*task_pool;
} sunxi_ce_cdev_t;

extern sunxi_ce_cdev_t	*ce_cdev;

typedef struct {
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

/*ioctl cmd*/
#define CE_IOC_MAGIC			'C'
#define CE_IOC_REQUEST			_IOR(CE_IOC_MAGIC, 0, int)
#define CE_IOC_FREE				_IOW(CE_IOC_MAGIC, 1, int)
#define CE_IOC_AES_CRYPTO		_IOW(CE_IOC_MAGIC, 2, crypto_aes_req_ctx_t)

/* Inner functions declaration */
void ce_dev_lock(void);
void ce_dev_unlock(void);
void ce_reset(void);
void __iomem *ss_membase(void);
int do_aes_crypto(crypto_aes_req_ctx_t *req);
irqreturn_t sunxi_ce_irq_handler(int irq, void *dev_id);

#endif /* end of _SUNXI_CE_CDEV_H_ */
