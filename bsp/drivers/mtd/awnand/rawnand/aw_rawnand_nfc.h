/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * SPDX-License-Identifier: GPL-2.0+
 * aw_rawnand_nfc.h
 *
 * (C) Copyright 2020 - 2021
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * cuizhikui <cuizhikui@allwinnertech.com>
 *
 */
#ifndef __AW_RAWNAND_NFC_H__
#define __AW_RAWNAND_NFC_H__

#include <linux/sizes.h>
#include <linux/dma-direction.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/io.h>

#define NFC_DEFAULT_TIMEOUT_MS	1000

#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif


#ifndef SZ_2K
#define SZ_2K				0x00000800
#endif



/* define bit use in NFC_CTL */
#define NFC_EN			BIT(0)
#define NFC_RESET		BIT(1)
#define NFC_BUS_WIDTH_MSK	BIT(2)
#define NFC_BUS_WIDTH_8		(0 << 2)
#define NFC_BUS_WIDTH_16	(1 << 2)
#define NFC_RB_SEL_MSK		(0x3 << 3)
#define NFC_RB_SEL(x)		((x) << 3)
#define NFC_CE_SEL_MSK		(0xf << 24)
#define NFC_CE_SEL(x)		((x) << 24)
#define NFC_CE_CTL		BIT(6)
#define NFC_PAGE_SHIFT_MSK	(0xf << 8)
#define NFC_SAM			BIT(12)
#define NFC_DMA_TYPE		BIT(15)
#define NFC_RAM_METHOD_DMA	BIT(14)
#define NFC_DATA_INTERFACE_TYPE_MSK (0x3 << 18)
#define NFC_DATA_INTERFACE_TYPE_SDR (0x0 << 18)
#define NFC_DATA_INTERFACE_TYPE_ONFI_DDR (0x2 << 18)
#define NFC_DATA_INTERFACE_TYPE_TOGGLE_DDR (0x3 << 18)
#define NFC_DATA_INTERFACE_TYPE_IS_DDR(reg_val)	(reg_val & (1 << 19))
#define NFC_DDR_REPEAT_ENABLE	BIT(20)
#define NFC_NAND_INTERFACE_DDR_TYPE_DDR2 BIT(28)
#define NFC_DEBUG_CTL		BIT(31)

#define NFC_PAGE_SIZE_1KB	(0 << 8)
#define NFC_PAGE_SIZE_2KB	(1 << 8)
#define NFC_PAGE_SIZE_4KB	(2 << 8)
#define NFC_PAGE_SIZE_8KB	(3 << 8)
#define NFC_PAGE_SIZE_16KB	(4 << 8)
#define NFC_PAGE_SIZE_32KB	(5 << 8)

/* define bit use in NFC_STATUS*/
#define NFC_RB_B2R		BIT(0)
#define NFC_CMD_INT_FLAG	BIT(1)
#define NFC_DMA_INT_FLAG	BIT(2)
#define NFC_CMD_FIFO_STATUS	BIT(3)
#define NFC_STA			BIT(4)
#define NFC_RB_STATE(x)		BIT(x + 8)

/* define bit use in NFC_INT */
#define NFC_B2R_INT_ENABLE	BIT(0)
#define NFC_CMD_INT_ENABLE	BIT(1)
#define NFC_DMA_INT_ENABLE	BIT(2)
#define NFC_INT_MASK		(0x7 << 0)

/* define bit use in NFC_CMD */
#define NFC_CMD_LOW_BYTE_MSK	0xff
#define NFC_CMD_HIGH_BYTE_MSK	(0xff << 8)
#define NFC_CMD(x)		(x)
#define NFC_ADR_NUM_MSK		(0x7 << 16)
#define NFC_ADR_NUM(x)		((((x) - 1)&0x7) << 16)
#define NFC_SEND_ADR		BIT(19)
#define NFC_ACCESS_DIR		BIT(20)
#define NFC_DATA_TRANS		BIT(21)
#define NFC_SEND_CMD1		BIT(22)
#define NFC_WAIT_FLAG		BIT(23)
#define NFC_SEND_CMD2		BIT(24)
#define NFC_SEQ			BIT(25)
#define NFC_DATA_SWAP_METHOD	BIT(26)
#define NFC_SEND_RAN_CMD2	BIT(27)
#define NFC_SEND_CMD3		BIT(28)
#define NFC_SEND_CMD4		BIT(29)
#define NFC_CMD_TYPE_MSK	(0x3 << 30)
#define NFC_NORMAL_OP		(0 << 30)
#define NFC_ECC_OP		(1 << 30)
#define NFC_BATCH_OP		(2 << 30)

/* define bit use in NFC_RCMD_SET */
#define NFC_READ_CMD_MSK	0xff
#define NFC_RND_READ_CMD0_MSK	(0xff << 8)
#define NFC_RND_READ_CMD1_MSK	(0xff << 16)

/* define bit use in NFC_WCMD_SET */
#define NFC_PROGRAM_CMD_MSK	0xff
#define NFC_RND_WRITE_CMD_MSK	(0xff << 8)
#define NFC_READ_CMD0_MSK	(0xff << 16)
#define NFC_READ_CMD1_MSK	(0xff << 24)

/* define bit use in NFC_EFR*/
#define NFC_ECC_DEBUG		(0x3f << 0)
#define NFC_WP_CTL		BIT(8)
#define NFC_DUMMY_BYTE_MSK	(0xff << 16)
#define NFC_DUMMY_BYTE_SET(x)	(((x)&0xff) << 16)
#define NFC_DUMMY_BYTE_EN	BIT(24)


/* define bit use in NFC_ECC_CTL*/
#define NFC_ECC_EN		BIT(0)
#define NFC_ECC_PIPELINE	BIT(3)
#define NFC_ECC_EXCEPTION	BIT(4)

#ifdef CONFIG_ARCH_SUN8IW11
#define NFC_ECC_BLOCK_SIZE	BIT(5)
#define NFC_RANDOM_EN		BIT(9)
#define NFC_RANDOM_DIR		BIT(10)
#define NFC_RANDOM_SIZE		BIT(11)
#define NFC_ECC_MODE_MSK	(0xf << 12)
#define NFC_ECC_SEL(x)		((x) << 12)
#define NFC_ECC_GET(x)		(((x) >> 12)&0xf)
#else
#define NFC_RANDOM_EN		BIT(5)
#define NFC_ECC_MODE_MSK	(0xff << 8)
#define NFC_ECC_SEL(x)		((x) << 8)
#define NFC_ECC_GET(x)		(((x) >> 8)&0xff)
#endif

#define NFC_RANDOM_SEED_MSK	(0x7fff << 16)
#define NFC_RANDOM_SEED_SEL(x)	((x) << 16)

#define NFC_RANDOM_SEED_DEFAULT	(0x4a80 << 16)

/* define bit use in NFC_TIMING_CTL*/
#define NFC_TIMING_CTL_PIPE_MSK (0xf << 8)
#define NFC_TIMING_CTL_DC_MSK (0x3f << 0)
#define NFC_TIMING_SDR_EDO (1 << 8)
#define NFC_TIMING_SDR_EEDO (2 << 8)
#define NFC_TIMING_DC_SEL(x)	((x) << 0)
#define NFC_TIMING_DDR_PIPE_SEL(x)	((x) << 8)


/*define bit use in NFC_SPARE_AREA*/
#define NFC_SPARE_AREA_MSK	(0xffff << 0)



#define ECC_BLOCKC_SIZE	(1024)


static const uint32_t random_seed[128] = {
    //0        1      2       3        4      5        6       7       8       9
    0x2b75, 0x0bd0, 0x5ca3, 0x62d1, 0x1c93, 0x07e9, 0x2162, 0x3a72, 0x0d67, 0x67f9,
    0x1be7, 0x077d, 0x032f, 0x0dac, 0x2716, 0x2436, 0x7922, 0x1510, 0x3860, 0x5287,
    0x480f, 0x4252, 0x1789, 0x5a2d, 0x2a49, 0x5e10, 0x437f, 0x4b4e, 0x2f45, 0x216e,
    0x5cb7, 0x7130, 0x2a3f, 0x60e4, 0x4dc9, 0x0ef0, 0x0f52, 0x1bb9, 0x6211, 0x7a56,
    0x226d, 0x4ea7, 0x6f36, 0x3692, 0x38bf, 0x0c62, 0x05eb, 0x4c55, 0x60f4, 0x728c,
    0x3b6f, 0x2037, 0x7f69, 0x0936, 0x651a, 0x4ceb, 0x6218, 0x79f3, 0x383f, 0x18d9,
    0x4f05, 0x5c82, 0x2912, 0x6f17, 0x6856, 0x5938, 0x1007, 0x61ab, 0x3e7f, 0x57c2,
    0x542f, 0x4f62, 0x7454, 0x2eac, 0x7739, 0x42d4, 0x2f90, 0x435a, 0x2e52, 0x2064,
    0x637c, 0x66ad, 0x2c90, 0x0bad, 0x759c, 0x0029, 0x0986, 0x7126, 0x1ca7, 0x1605,
    0x386a, 0x27f5, 0x1380, 0x6d75, 0x24c3, 0x0f8e, 0x2b7a, 0x1418, 0x1fd1, 0x7dc1,
    0x2d8e, 0x43af, 0x2267, 0x7da3, 0x4e3d, 0x1338, 0x50db, 0x454d, 0x764d, 0x40a3,
    0x42e6, 0x262b, 0x2d2e, 0x1aea, 0x2e17, 0x173d, 0x3a6e, 0x71bf, 0x25f9, 0x0a5d,
    0x7c57, 0x0fbe, 0x46ce, 0x4939, 0x6b17, 0x37bb, 0x3e91, 0x76db
};

#ifdef CONFIG_ARCH_SUN8IW11
static uint8_t ecc_bits_tbl[9] = {16, 24, 28, 32, 40, 48, 56, 60, 64};
static uint8_t ecc_limit_tab[9] = {13, 20, 23, 27, 35, 42, 50, 54, 58};
#else
static uint8_t ecc_bits_tbl[15] = {16, 24, 28, 32, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80};
static uint8_t ecc_limit_tab[15] = {13, 20, 23, 27, 35, 39, 42, 46, 50, 54, 58, 62, 66, 68, 72};
#endif

#define MAX_ECC_BCH_80	((sizeof(ecc_bits_tbl)/sizeof(ecc_bits_tbl[0])) - 1)


static inline int aw_host_nfc_wait_status(volatile uint32_t *reg, uint32_t mark,
		uint32_t val, uint32_t timeout_ms)
{
	unsigned long timeout = 0;
	int ret = -ETIMEDOUT;

	timeout = jiffies + msecs_to_jiffies(timeout_ms);
	do {
		if ((readl(reg) & mark) == val) {
			ret = 0;
			break;
		} else
			cond_resched();
	} while (time_before(jiffies, timeout));

	return ret;

}

static inline int aw_host_nfc_reset(struct nfc_reg *nfc)
{
	uint32_t val = 0;
	int ret = -ETIMEDOUT;
	unsigned long timeout = 0;

	val = readl(nfc->ctl);
	val |= NFC_RESET;
	writel(val, nfc->ctl);

	/*ms*/
	timeout = jiffies + msecs_to_jiffies(30);

	do {
		if (!(readl(nfc->ctl) & NFC_RESET)) {
			ret = 0;
			break;
		} else
			cond_resched();
	} while (time_before(jiffies, timeout));

	awrawnand_info("nfc rest %s\n", ret ? "fail" : "ok");
	return ret;
}

static inline void aw_host_nfc_ctl_init(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;


	cfg |= NFC_EN;
	cfg |= NFC_BUS_WIDTH_8;
	cfg &= ~NFC_CE_CTL;
	cfg |= NFC_PAGE_SIZE_2KB;
	cfg &= ~NFC_DATA_INTERFACE_TYPE_MSK;
	cfg |= NFC_DATA_INTERFACE_TYPE_SDR;

	writel(cfg, nfc->ctl);
}
static inline void aw_host_nfc_timing_init(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;


	cfg = readl(nfc->timing_ctl);
	cfg &= ~NFC_TIMING_CTL_PIPE_MSK;
	cfg &= ~NFC_TIMING_CTL_DC_MSK;
	cfg |= NFC_TIMING_SDR_EDO;

	writel(cfg, nfc->timing_ctl);

	/*1.default value 0x95
	 *2. bit16 tCCS=1 for micron l85a, NVDDR-100mhz*/
	cfg = 0x10095;
	writel(cfg, nfc->timing_cfg);
}

static inline void aw_host_nfc_spare_area_init(struct nfc_reg *nfc)
{
	writel((SZ_2K & NFC_SPARE_AREA_MSK), nfc->spare_area);
}


static inline void aw_host_nfc_efr_init(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg |= NFC_WP_CTL;
	writel(cfg, nfc->efr);
}

static inline void aw_host_nfc_randomize_disable(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->ecc_ctl);
	cfg &= ~NFC_RANDOM_EN;
	cfg &= ~NFC_RANDOM_SEED_MSK;
	cfg |= NFC_RANDOM_SEED_DEFAULT;
	writel(cfg, nfc->ecc_ctl);

}

/*enable randomizer and set random seed*/
#ifdef CONFIG_ARCH_SUN8IW11
static inline void aw_host_nfc_randomize_enable(struct nfc_reg *nfc, uint32_t page)
{
	uint32_t cfg = 0, seed = 0;
	struct aw_nand_chip *chip = get_rawnand();

	seed = random_seed[page % 128];

	cfg = readl(nfc->ecc_ctl);

	if (!chip->operate_boot0) {
		cfg &= 0x0000ffff;
		cfg |= NFC_RANDOM_SEED_SEL(seed);
	} else {
		cfg &= 0x0000ffff;
		cfg |= NFC_RANDOM_SEED_DEFAULT;
	}
	cfg |= NFC_RANDOM_EN;

	writel(cfg, nfc->ecc_ctl);
}
#else
static inline void aw_host_nfc_randomize_enable(struct nfc_reg *nfc, uint32_t page)
{
	uint32_t cfg = 0, seed = 0;

	seed = random_seed[page % 128];

	cfg = readl(nfc->ecc_ctl);

	cfg &= ~NFC_RANDOM_SEED_MSK;
	cfg |= NFC_RANDOM_SEED_SEL(seed);
	cfg |= NFC_RANDOM_EN;

	writel(cfg, nfc->ecc_ctl);
}
#endif

static inline void aw_host_nfc_set_ecc_mode(struct nfc_reg *nfc, uint8_t ecc_mode)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->ecc_ctl);
	cfg &= ~NFC_ECC_MODE_MSK;
	cfg |= (NFC_ECC_SEL(ecc_mode) & NFC_ECC_MODE_MSK);
	writel(cfg, nfc->ecc_ctl);
}

static inline void aw_host_nfc_ecc_enable(struct nfc_reg *nfc, uint8_t pipline)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->ecc_ctl);

	if (pipline)
		cfg |= NFC_ECC_PIPELINE;
	else
		cfg &= ~NFC_ECC_PIPELINE;

	cfg |= NFC_ECC_EXCEPTION;
	cfg |= NFC_ECC_EN;

	writel(cfg, nfc->ecc_ctl);
}

static inline void aw_host_nfc_ecc_disable(struct nfc_reg *nfc)
{
	writel((readl(nfc->ecc_ctl) & (~NFC_ECC_EN)), nfc->ecc_ctl);
}


static inline void aw_host_nfc_chip_select(struct nfc_reg *nfc, int chip)
{
	uint32_t cfg = 0;

	/*ce <==> rb*/
	cfg = readl(nfc->ctl);
	cfg &= ~NFC_CE_SEL_MSK;
	cfg |= NFC_CE_SEL(chip);
	cfg &= ~NFC_RB_SEL_MSK;
	if (chip != 0xf)
		cfg |= NFC_RB_SEL(chip);
	writel(cfg, nfc->ctl);
}

static inline int aw_host_nfc_get_selected_rb_no(struct nfc_reg *nfc)
{
	return ((readl(nfc->ctl) & NFC_RB_SEL_MSK) >> 3);
}

static inline void aw_host_nfc_set_addr(struct nfc_reg *nfc, uint8_t *addr, int addr_num)
{
	uint32_t low = 0, high = 0;
	int i = 0;

	for (i = 0; i < addr_num; i++) {
		if (i < 4)
			low |= addr[i] << (i * 8);
		else
			high |= addr[i] << ((i - 4) * 8);
	}

	writel(low, nfc->addr_low);
	writel(high, nfc->addr_high);
}

static inline void aw_host_nfc_repeat_mode_enable(struct nfc_reg *nfc)
{
	uint32_t val = 0;

	val = readl(nfc->ctl);

	if (NFC_DATA_INTERFACE_TYPE_IS_DDR(val)) {
		val |= NFC_DDR_REPEAT_ENABLE;
		writel(val, nfc->ctl);
	}
}

static inline void aw_host_nfc_repeat_mode_disable(struct nfc_reg *nfc)
{
	uint32_t val = 0;

	val = readl(nfc->ctl);

	if (NFC_DATA_INTERFACE_TYPE_IS_DDR(val)) {
		val &= ~NFC_DDR_REPEAT_ENABLE;
		writel(val, nfc->ctl);
	}
}

static inline void aw_host_nfc_dma_int_enable(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->int_ctl);
	cfg |= NFC_DMA_INT_ENABLE;
	writel(cfg, nfc->int_ctl);
}

static inline void aw_host_nfc_dma_int_disable(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->int_ctl);
	cfg &= ~NFC_DMA_INT_ENABLE;
	writel(cfg, nfc->int_ctl);
}

static inline void aw_host_nfc_dma_intstatus_clear(struct nfc_reg *nfc)
{

	writel(NFC_DMA_INT_FLAG, nfc->sta);
}

static inline bool aw_host_nfc_dma_int_occur_check(struct nfc_reg *nfc)
{
	return ((readl(nfc->sta) & NFC_DMA_INT_ENABLE) &&
			(readl(nfc->int_ctl) & NFC_DMA_INT_ENABLE));
}

static inline void aw_host_nfc_rb_b2r_int_enable(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->int_ctl);
	cfg |= NFC_B2R_INT_ENABLE;
	writel(cfg, nfc->int_ctl);
}

static inline void aw_host_nfc_rb_b2r_int_disable(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->int_ctl);
	cfg &= ~NFC_B2R_INT_ENABLE;
	writel(cfg, nfc->int_ctl);
}
static inline void aw_host_nfc_rb_b2r_intstatus_clear(struct nfc_reg *nfc)
{
	uint32_t cfg = 0;

	cfg = readl(nfc->sta);
	cfg |= NFC_RB_B2R;
	writel(cfg, nfc->sta);
}

static inline bool aw_host_nfc_rb_b2r_int_occur_check(struct nfc_reg *nfc)
{
	return ((readl(nfc->sta) & NFC_RB_B2R) && (readl(nfc->int_ctl) & NFC_B2R_INT_ENABLE));
}

static inline void aw_host_nfc_set_user_data_len(struct nfc_reg *nfc, uint32_t user_data_len)
{
	int i = 0;
	int j = 0;
	uint32_t cfg = 0;
	/*In order to ndfc spec, one ecc block can attach user data len*/
	uint8_t ecc_block_user_len[9] = {0, 4, 8, 12, 16, 20, 24, 28, 32};
	uint8_t ecc_block_cnt = (user_data_len + 32 - 1) / 32;
	uint8_t last_ecc_block_user_len = user_data_len % 32;
	uint8_t last_cfg = 0;
	uint8_t user_data_len_reg_cnt = 0;

	AWRAWNAND_TRACE_NFC("Enter %s user_data_len@%d\n", __func__, user_data_len);

	/*find the last_ecc_block_user_len should configure what*/
	for (i = 0; i < 9; i++) {
		if (ecc_block_user_len[i] == last_ecc_block_user_len) {
			last_cfg = i;
			break;
		}
	}

	if (user_data_len == 32)
		last_cfg = 0x8;

	/*user_data_len register per 4bits indicate
	 * one ecc block user data len configure,
	 * one user data len register can indicate 8 ecc block user len setting*/
	user_data_len_reg_cnt = ecc_block_cnt / 8;
	if (ecc_block_cnt % 8)
		user_data_len_reg_cnt++;


	for (i = 0; i < user_data_len_reg_cnt; i++) {
		/*ecc block user data len configure to maximum(32B, 0x8 indicate),
		 * except the last ecc block*/
		cfg = 0;
		if (i == (user_data_len_reg_cnt - 1)) {
			for (j = 0; j < (ecc_block_cnt % 8); j++) {
				if (j == ((ecc_block_cnt % 8) - 1)) {
					cfg |= (last_cfg << (j * 4));
					break;
				} else
					cfg |= 0x8 << (j * 4);
			}
		} else {
			cfg = 0x88888888;
		}
		writel(cfg, (nfc->user_data_len_base + i));
	}
	AWRAWNAND_TRACE_NFC("Exit %s\n", __func__);
}

/*aw_host_nfc_set_boot0_user_data_len - boot0 user len by one ecc block attach 4Byte*/
static inline void aw_host_nfc_set_boot0_user_data_len(struct nfc_reg *nfc, uint32_t user_data_len)
{
	int i = 0;
	int j = 0;
	uint32_t cfg = 0;
	uint8_t user_data_len_reg_cnt = 0;
	/*In order to ndfc spec, one ecc block can attach user data len*/

/*boot0 user data manage*/
#define ONE_ECC_BLOCK_ATTACH_4BYTE (4)

	uint8_t ecc_block_cnt = (user_data_len + ONE_ECC_BLOCK_ATTACH_4BYTE - 1) / ONE_ECC_BLOCK_ATTACH_4BYTE;

	AWRAWNAND_TRACE_NFC("Enter %s user_data_len@%d\n", __func__, user_data_len);

	/*user_data_len register per 4bits indicate
	 * one ecc block user data len configure,
	 * one user data len register can indicate 8 ecc block user len setting*/
	user_data_len_reg_cnt = ecc_block_cnt / 8;
	if (ecc_block_cnt % 8)
		user_data_len_reg_cnt++;


	/*one ecc block attach 4Bytes*/
	for (i = 0; i < user_data_len_reg_cnt; i++) {
		cfg = 0;
		/*one regisetr can configure 8 ecc block*/
		if (i == (user_data_len_reg_cnt - 1)) {
			for (j = 0; j < ecc_block_cnt % 8; j++) {
				/*4bits indicate one ecc block*/
				cfg |= (1 << (j * 4));
			}
		} else {
			cfg = 0x11111111;
		}
		writel(cfg, (nfc->user_data_len_base + i));
	}
	AWRAWNAND_TRACE_NFC("Exit %s\n", __func__);
}

static inline void aw_host_nfc_set_user_data(struct nfc_reg *nfc, uint8_t *data, int len)
{
	int i = 0;
	uint32_t val = 0;

	AWRAWNAND_TRACE_NFC("Enter %s data@%p len@%d\n", __func__, data, len);
	if (!data)
		return;
	for (i = 0; i < len; i += 4) {
		val = (data[i + 3] << 24 | data[i + 2] << 16 | data[i + 1] << 8 | data[i + 0]);
		writel(val, nfc->user_data_base + (i >> 2));
	}

	AWRAWNAND_TRACE_NFC("Exit %s\n", __func__);
}


static inline void aw_host_nfc_set_dummy_byte(struct nfc_reg *nfc, int dummy_byte)
{
	uint32_t cfg = 0;

	AWRAWNAND_TRACE_NFC("Enter %s\n", __func__);
	cfg = readl(nfc->efr);
	cfg &= ~NFC_DUMMY_BYTE_MSK;
	cfg |= NFC_DUMMY_BYTE_SET(dummy_byte);
	if (dummy_byte != 0)
		cfg |= NFC_DUMMY_BYTE_EN;
	writel(cfg, nfc->efr);

	AWRAWNAND_TRACE_NFC("Exit %s\n", __func__);
}


#endif /*AW_RAWNAND_NFC*/
