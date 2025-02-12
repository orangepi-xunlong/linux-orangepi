// SPDX-License-Identifier: GPL-2.0
/*
 * CE engine for ky
 *
 * Copyright (C) 2023 Ky
 */
#include <linux/clk.h>
#include <linux/reset.h>

#ifndef SPAECMIT_SECENG_H
#define KY_SECENG_H

#define KY_AES_BUFFER_LEN		1024 * 256

#define WORK_BUF_SIZE			2048
#define CTR_COUNTER_LITTLE_ENDIAN	0x0000
#define CTR_COUNTER_BIG_ENDIAN		0x1000
#define BYTES_TO_BITS			8
#define	KY_SECENG_SIZE		0x3000

#define ENGINE_DMA_ADDR_HIGH_OFFSET 	0x14c
#define SW_RESETN 			BIT(0)
#define MASTER_CLK_EN 			BIT(1)
#define SLAVE_CLK_EN			BIT(2)
#define WADDR_BIT32 			BIT(4)
#define RADDR_BIT32 			BIT(5)

#define CE_BIU_REG_OFFSET		0x00000000L
#define CE_ADEC_REG_OFFSET		0x00000400L
#define CE_DMA_REG_OFFSET		0x00000800L
#define CE_ABUS_REG_OFFSET		0x00000C00L
#define CE_CRYPTO_REG_OFFSET		0x00001000L
#define CE_HASH_REG_OFFSET		0x00001800L

#define CE_ADEC_CTRL			0x0000
#define CE_ADEC_CTRL2			0x0004
#define CE_AXI_SL_CTRL			0x0008
#define CE_ADEC_INT			0x000C
#define CE_ADEC_INT_MSK			0x0010
#define CE_ADEC_ACC_ERR_ADR		0x0014
#define CE_ADEC_MP_FIFO_ERR_ADR		0x0018

#define CE_ABUS_BUS_CTRL		0x0000

#define SP_HST_INTERRUPT_MASK		0x0cc
#define SP_INTERRUPT_RST		0x218
#define SP_INTERRUPT_MASK		0x21c
#define SP_CONTROL			0x220

#define CE_HASH_CONFIG_REG			0x0000
#define CE_HASH_CONTROL_REG			0x0004
#define CE_HASH_COMMAND_REG			0x0008
#define CE_HASH_STATUS_REG			0x000C
#define CE_HASH_INCOME_SEG_SZ_REG		0x0010
#define CE_HASH_TOTAL_MSG_SZ_L_REG		0x0018
#define CE_HASH_TOTAL_MSG_SZ_H_REG		0x001C
#define CE_HASH_DIGEST_BASE			0x0020
#define CE_HASH_DIGEST_REG(a) 		\
			(CE_HASH_DIGEST_BASE + (a << 2))
#define CE_HASH_DIGEST_H_BASE			0x0040
#define CE_HASH_DIGEST_H_REG(a)		\
			(CE_HASH_DIGEST_H_BASE + (a << 2))
#define CE_HASH_CONTEXTO_BASE			0x0064
#define CE_HASH_CONTEXTO_REG(a)		\
			(CE_HASH_CONTEXTO_BASE + (a << 2))
#define CE_HASH_CONTEXTO_H_BASE			0x0080
#define CE_HASH_CONTEXTO_H_REG(a)	\
			(CE_HASH_CONTEXTO_H_BASE + (a << 2))
#define CE_HASH_KEY_BASE			0x00A4
#define CE_HASH_KEY_REG(a)		\
			(CE_HASH_KEY_BASE + (a << 2))

#define CE_DMA_IN_CTRL				0x0000
#define CE_DMA_IN_STATUS			0x0004
#define CE_DMA_IN_SRC_ADR			0x0008
#define CE_DMA_IN_XFER_CNTR			0x000C
#define CE_DMA_IN_NX_LL_ADR			0x0010
#define CE_DMA_IN_INT				0x0014
#define CE_DMA_IN_INT_MASK			0x0018
#define CE_DMA_OUT_CTRL				0x001C
#define CE_DMA_OUT_STATUS			0x0020
#define CE_DMA_OUT_DEST_ADR			0x0024
#define CE_DMA_OUT_XFER_CNTR			0x0028
#define CE_DMA_OUT_NX_LL_ADR			0x002C
#define CE_DMA_OUT_INT				0x0030
#define CE_DMA_OUT_INT_MASK			0x0034
#define CE_DMA_AXI_CTRL				0x0038
#define CE_DMA_IF_RCOUNT			0x003C
#define CE_DMA_IF_RD_PTR_ERR			0x0040
#define CE_DMA_OF_SPACE				0x0044
#define CE_DMA_OF_RD_PTR_ERR			0x0048
#define CE_DMA_IF_RAM_BASE			0x0100
#define CE_DMA_IF_RAM_REG(a) 		\
			(CE_DMA_IF_RAM_BASE + a*0x4)
#define CE_DMA_OF_RAM_BASE			0x0300
#define CE_DMA_OF_RAM_REG(a)		\
			(CE_DMA_OF_RAM_BASE + a*0x4)

#define CE_BIU_HST_INTERRUPT_MASK		0x00CC
#define CE_BIU_SP_INTERRUPT_MASK		0x021C
#define CE_BIU_SP_CONTROL			0x0220

#define CE_CRYPTO_AES_CONFIG_REG		0x0000
#define CE_CRYPTO_AES_CONTROL_REG		0x0004
#define CE_CRYPTO_AES_COMMAND_REG		0x0008
#define CE_CRYPTO_AES_STATUS_REG		0x000C
#define CE_CRYPTO_AES_INTRPT_SRC_REG		0x0010
#define CE_CRYPTO_AES_INTRPT_SRC_EN_REG		0x0014
#define CE_CRYPTO_AES_STREAM_SIZE_REG		0x0018
#define CE_CRYPTO_ENGINE_SEL_REG		0x00A8

#define CE_CRYPTO_K2_BASE			0x0058
#define CE_CRYPTO_K2_W_REG(a)		\
			(CE_CRYPTO_K2_BASE + a*0x4)
#define CE_CRYPTO_K1_BASE			0x0078
#define CE_CRYPTO_K1_W_REG(a)		\
			(CE_CRYPTO_K1_BASE + a*0x4)
#define CE_CRYPTO_IV_BASE			0x0098
#define CE_CRYPTO_IV_REG(a)		\
			(CE_CRYPTO_IV_BASE + a*0x4)

#define BIT0 1<<0
#define BIT1 1<<1
#define BIT2 1<<2
#define BIT3 1<<3
#define BIT4 1<<4
#define BIT5 1<<5

#define AES_INTERRUPT_FLAG BIT0
#define AES_ERR1_INTERRUPT_FLAG BIT1
#define AES_ERR2_INTERRUPT_FLAG BIT2
#define AES_INTERRUPT_MASK (AES_INTERRUPT_FLAG | AES_ERR1_INTERRUPT_FLAG | AES_ERR2_INTERRUPT_FLAG)

#define BIT_DMA_INOUT_DONE BIT0
#define BIT_DMA_INOUT_BUS_ERR BIT1
#define BIT_DMA_INOUT_LL_ERR BIT2
#define BIT_DMA_INOUT_PAR_ERR BIT3
#define BIT_DMA_INOUT_PAUSE_CMPL_ERR BIT4
#define BIT_DMA_INOUT_DATA_PAR_ERR BIT5
#define DMA_INTERRUPT_MASK (BIT_DMA_INOUT_DONE | BIT_DMA_INOUT_BUS_ERR | BIT_DMA_INOUT_LL_ERR \
		| BIT_DMA_INOUT_PAR_ERR | BIT_DMA_INOUT_PAUSE_CMPL_ERR | BIT_DMA_INOUT_DATA_PAR_ERR)

#define BIU_MASK BIT0
#define ADEC_MASK (BIT1 | BIT5)

typedef enum {
	/* reset bit */
	E_ACC_ENG_DMA = 1,
	E_ACC_ENG_HASH = 5,
	E_ACC_ENG_CRYPTO = 3,
	E_ACC_ENG_ALL,
} ADEC_ACC_ENG_T;

typedef enum {
	E_ABUS_GRP_A_HASH = 0x0,
} ABUS_GRP_A_T;

typedef enum {
	E_ABUS_GRP_B_AES = 0x0,
	E_ABUS_GRP_B_BYPASS = 0x2,
} ABUS_GRP_B_T;

typedef enum {
	E_ABUS_STRAIGHT = 0,
	E_ABUS_CROSS,
} ABUS_CROSS_BAR_T;

typedef enum {
	E_HASH_INIT = 0x1,
	E_HASH_UPDATE = 0x2,
	E_HASH_FINAL = 0x3,
} HASH_OP_MODE_T;

typedef enum {
	E_HASH_LEN_SHA1 = 20,
	E_HASH_LEN_SHA256 = 32,
	E_HASH_LEN_SHA224 = 28,
	E_HASH_LEN_MD5 = 16,
	E_HASH_LEN_SHA512 = 64,
	E_HASH_LEN_SHA384 = 48,
} HASH_LEN_T;

typedef enum {
	E_HASH_SIMPLE = 0,
	E_HASH_HMAC,
} HASH_MODE_T;

typedef enum {
	E_HASH_SHA1 = 0x0,
	E_HASH_SHA256 = 0x1,
	E_HASH_SHA224 = 0x2,
	E_HASH_MD5 = 0x3,
	E_HASH_SHA512 = 0x4,
	E_HASH_SHA384 = 0x5,
} HASH_ALGO_T;

typedef struct {
	uint32_t addr;
	uint32_t size;
	uint32_t next_desc;
	uint32_t reserved;
} DMA_DESC_T;

typedef enum {
	E_AES_128 = 128/8,
	E_AES_192 = 192/8,
	E_AES_256 = 256/8,
} AES_KEY_LEN_T;

typedef enum {
	E_AES_ECB = 0,
	E_AES_CBC,
	E_AES_CTR,
	E_AES_XTS,
} AES_MODE_T;

typedef enum {
	E_AES_DECRYPT = 0,
	E_AES_ENCRYPT,
} AES_OP_MODE_T;

typedef enum {
	E_ENG_AES = 0,
} CRYPTO_ENG_SEL_T;


struct rijndael_key {
	uint32_t eK[60], dK[60];
	int Nr;
};

typedef union Symmetric_key {
	struct rijndael_key rijndael;
	void   *data;
} symmetric_key;

struct md5_state {
	uint64_t length;
	uint32_t state[4], curlen;
	unsigned char buf[64];
};
struct sha512_state {
	uint64_t  length, state[8];
	unsigned long curlen;
	unsigned char buf[128];
};
struct sha256_state {
	uint64_t length;
	uint32_t state[8], curlen;
	unsigned char buf[64];
};
struct sha1_state {
	uint64_t length;
	uint32_t state[5], curlen;
	unsigned char buf[64];
};

typedef union Hash_state {
	char dummy[1];

	struct sha512_state sha512;
	struct sha256_state sha256;
	struct sha1_state   sha1;
	struct md5_state    md5;
	void *data;
} hash_state;

enum engine_index{
	ENGINE_1,
	ENGINE_2,
	ENGINE_MAX,
};
enum crypto_aes_status{
	AES_INVALID,
	AES_DONE,
	AES_ERROR
};
enum crypto_dma_status{
	DMA_INVALID,
	DMA_INOUT_DONE,
	DMA_INOUT_ERROR
};

enum engine_ddr_type{
	NORMAL_DDR,
	RESERVED_DDR,
	RESERVED_SRAM
};

struct engine_info{
	unsigned long engine_base;
	struct completion aes_done;
	struct completion dma_output_done;
	struct completion dma_input_done;
	struct mutex eng_mutex;
	enum crypto_aes_status aes_status;
	enum crypto_dma_status dma_in_status;
	enum crypto_dma_status dma_out_status;
	enum engine_ddr_type ddr_type;
	irqreturn_t (*handler)(int irq, void *nouse);
	unsigned char internal_working_buffer[WORK_BUF_SIZE + WORK_BUF_SIZE]  __attribute__ ((aligned(32)));
};

#ifdef KY_CRYPTO_DEBUG
#define SRAM_NUM 2
#define SUBSYS_MAX 2
#define SRAM_MAP_SIZE 0x1000
struct sram_area{
	unsigned long sram_start;
	unsigned long sram_end;
	unsigned long sram_size;
};
static struct sram_area sram_reserved[SRAM_NUM * SUBSYS_MAX];
static unsigned long  sram_phy_base_src,sram_phy_base_dst;
#endif
struct aes_clk_reset_ctrl {
	struct clk *clk;
	struct reset_control *reset;
};

#endif
