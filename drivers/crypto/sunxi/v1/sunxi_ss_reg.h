/*
 * The register macro of SUNXI SecuritySystem controller.
 *
 * Copyright (C) 2013 Allwinner.
 *
 * Mintow <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _SUNXI_SECURITY_SYSTEM_REG_H_
#define _SUNXI_SECURITY_SYSTEM_REG_H_

#include <mach/platform.h>

#define SS_REG_CTL			0x00
#define SS_REG_KEY_BASE		0x04
#define SS_REG_KEY(n)		(SS_REG_KEY_BASE + 4*n)
#define SS_REG_KEY_NUM		8
#define SS_REG_IV_BASE		0x24
#define SS_REG_IV(n)		(SS_REG_IV_BASE + 4*n)
#define SS_REG_IV_NUM		4
#define SS_REG_FCSR			0x44
#define SS_REG_ICSR			0x48
#define SS_REG_MD_BASE		0x4C
#define SS_REG_MD(n)		(SS_REG_MD_BASE + 4*n)
#define SS_REG_MD_NUM		5
#define SS_REG_CNT_BASE		0x34
#define SS_REG_CNT(n)		(SS_REG_CNT_BASE + 4*n)
#define SS_REG_CNT_NUM		4
#if defined(CONFIG_ARCH_SUN8IW5) || defined(CONFIG_ARCH_SUN8IW8)
#define SS_REG_CTS_LEN		0x60
#endif
#define SS_REG_RXFIFO		0x200
#define SS_REG_TXFIFO		0x204

#define SS_KEY_SELECT_INPUT			0
#define SS_KEY_SELECT_SID_R			1
#define SS_KEY_SELECT_SID_B			2
#define SS_KEY_SELECT_INTER			3
#define SS_REG_CTL_KEY_SELECT_SHIFT	24
#define SS_REG_CTL_KEY_SELECT_MASK	(0x4 << SS_REG_CTL_KEY_SELECT_SHIFT)

#define SS_RNG_MODE_ONESHOT			0
#define SS_RNG_MODE_CONTINUE		1
#define SS_REG_CTL_PRNG_MODE_SHIFT	15
#define SS_REG_CTL_PRNG_MODE_MASK	(1 << SS_REG_CTL_PRNG_MODE_SHIFT)

#define SS_IV_MODE_CONSTANT			0
#define SS_IV_MODE_ARBITRARY		1
#define SS_REG_CTL_IV_MODE_SHIFT	14
#define SS_REG_CTL_IV_MODE_MASK		(1 << SS_REG_CTL_IV_MODE_SHIFT)

#define SS_AES_MODE_ECB				0
#define SS_AES_MODE_CBC				1
#define SS_AES_MODE_CTR				2
#if defined(CONFIG_ARCH_SUN8IW5) || defined(CONFIG_ARCH_SUN8IW8)
#define SS_AES_MODE_CTS				3
#endif
#define SS_REG_CTL_OP_MODE_SHIFT	12
#define SS_REG_CTL_OP_MODE_MASK		(0x3 << SS_REG_CTL_OP_MODE_SHIFT)

#define SS_CTR_SIZE_16				0
#define SS_CTR_SIZE_32				1
#define SS_CTR_SIZE_64				2
#define SS_REG_CTL_CTR_SIZE_SHIFT	10
#define SS_REG_CTL_CTR_SIZE_MASK	(0x3 << SS_REG_CTL_CTR_SIZE_SHIFT)

#define SS_AES_KEY_SIZE_128			0
#define SS_AES_KEY_SIZE_192			1
#define SS_AES_KEY_SIZE_256			2
#define SS_REG_CTL_KEY_SIZE_SHIFT	8
#define SS_REG_CTL_KEY_SIZE_MASK	(0x3 << SS_REG_CTL_KEY_SIZE_SHIFT)

#define SS_DIR_ENCRYPT				0
#define SS_DIR_DECRYPT				1
#define SS_REG_CTL_OP_DIR_SHIFT		7
#define SS_REG_CTL_OP_DIR_MASK		(0x1 << SS_REG_CTL_OP_DIR_SHIFT)

#define SS_METHOD_AES				0
#define SS_METHOD_DES				1
#define SS_METHOD_3DES				2
#define SS_METHOD_SHA1				3
#define SS_METHOD_MD5				4
#define SS_METHOD_PRNG				5
#define SS_REG_CTL_METHOD_SHIFT		4
#define SS_REG_CTL_METHOD_MASK		(0x7 << SS_REG_CTL_METHOD_SHIFT)

#define SS_METHOD_IS_HASH(type) ((type == SS_METHOD_MD5) \
								|| (type == SS_METHOD_SHA1))

#define SS_REG_CTL_SHA_END_SHIFT	2
#define SS_REG_CTL_SHA_END_MASK		(0x1 << SS_REG_CTL_SHA_END_SHIFT)

#define SS_CTL_START				1
#define SS_REG_CTL_START_MASK		0x1

#define SS_REG_FCSR_RXFIFO_STATUS			BIT(30)
#define SS_REG_FCSR_RXFIFO_EMP_CNT_SHIFT	24
#define SS_REG_FCSR_RXFIFO_EMP_CNT_MASK		(0x3F << SS_REG_FCSR_RXFIFO_EMP_CNT_SHIFT)
#define SS_REG_FCSR_TXFIFO_STATUS			BIT(22)
#define SS_REG_FCSR_TXFIFO_AVA_CNT_SHIFT	16
#define SS_REG_FCSR_TXFIFO_AVA_CNT_MASK		(0x3F << SS_REG_FCSR_TXFIFO_AVA_CNT_SHIFT)
#define SS_REG_FCSR_RXFIFO_TRIG_LEVEL_SHIFT	8
#define SS_REG_FCSR_RXFIFO_TRIG_LEVEL_MASK	(0x1F << SS_REG_FCSR_RXFIFO_TRIG_LEVEL_SHIFT)
#define SS_REG_FCSR_TXFIFO_TRIG_LEVEL_SHIFT	0
#define SS_REG_FCSR_TXFIFO_TRIG_LEVEL_MASK	(0x1F << SS_REG_FCSR_TXFIFO_TRIG_LEVEL_SHIFT)

#define SS_REG_ICSR_RXFIFO_EMP_PENDING		BIT(10)
#define SS_REG_ICSR_TXFIFO_AVA_PENDING		BIT(8)
#define SS_REG_ICSR_DRA_ENABLE				BIT(4)
#define SS_REG_ICSR_RXFIFO_EMP_INT_ENABLE	BIT(2)
#define SS_REG_ICSR_TXFIFO_AVA_INT_ENABLE	BIT(0)

#define SS_SEED_SIZE			24

/* Function declaration */

u32 ss_reg_rd(u32 offset);
void ss_reg_wr(u32 offset, u32 val);

void ss_keyselect_set(int select);
void ss_keysize_set(int size);
void ss_key_set(char *key, int size);

void ss_fifo_init(void);
void ss_flow_enable(int flow);
void ss_flow_mode_set(int mode);

int ss_pending_get(void);
void ss_pending_clear(int flow);
void ss_dma_enable(int flow);
void ss_dma_disable(int flow);

void ss_iv_set(char *iv, int size);
void ss_cntsize_set(int size);
void ss_cnt_set(char *cnt, int size);
void ss_cnt_get(int flow, char *cnt, int size);

void ss_md_get(char *dst, char *src, int size);
void ss_sha_final(void);
void ss_check_sha_end(void);

void ss_iv_mode_set(int mode);

void ss_method_set(int dir, int type);
void ss_aes_mode_set(int mode);
void ss_rng_mode_set(int mode);
int ss_random_rd(char *data, int len);

void ss_wait_idle(void);
void ss_ctrl_start(void);
void ss_ctrl_stop(void);

int ss_reg_print(char *buf, int len);

#endif /* end of _SUNXI_SECURITY_SYSTEM_REG_H_ */

