// SPDX-License-Identifier: GPL-2.0
/*
 * CE engine for ky-x1
 *
 * Copyright (C) 2023 Ky
 */

#include "linux/dma-direction.h"
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <crypto/aes.h>
#include <linux/crypto.h>
#include <linux/cpufeature.h>
#include <linux/sched/task_stack.h>
#include <asm/cacheflush.h>
#include <asm/mmio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <crypto/algapi.h>
#include <../crypto/internal.h>
#include <linux/notifier.h>
#include <linux/mfd/syscon.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include "ky_engine.h"
#include <linux/pm_runtime.h>
#include <linux/pm_qos.h>
#include <linux/pm.h>

struct device *dev;
unsigned char *in_buffer, *out_buffer;
uint64_t dma_addr_in, dma_addr_out;
static struct regmap *ciu_base;
static struct engine_info engine[ENGINE_MAX];

static void dma_write32(int index, size_t offset, uint32_t value)
{
	tcm_override_writel(value, (void*)(engine[index].engine_base + CE_DMA_REG_OFFSET + offset));
}
static uint32_t dma_read32(int index, size_t offset)
{
	return tcm_override_readl((void*)(engine[index].engine_base + CE_DMA_REG_OFFSET + offset));
}
static void biu_write32(int index, size_t offset, uint32_t value)
{
	tcm_override_writel(value, (void*)(engine[index].engine_base + CE_BIU_REG_OFFSET + offset));
}
static uint32_t biu_read32(int index, size_t offset)
{
	return tcm_override_readl((void*)(engine[index].engine_base + CE_BIU_REG_OFFSET + offset));
}
static void adec_write32(int index, size_t offset, uint32_t value)
{
	tcm_override_writel(value, (void*)(engine[index].engine_base + CE_ADEC_REG_OFFSET + offset));
}
static uint32_t adec_read32(int index, size_t offset)
{
	return tcm_override_readl((void*)(engine[index].engine_base + CE_ADEC_REG_OFFSET + offset));
}
static void abus_write32(int index, size_t offset, uint32_t value)
{
	tcm_override_writel(value, (void*)(engine[index].engine_base + CE_ABUS_REG_OFFSET + offset));
}
static uint32_t abus_read32(int index, size_t offset)
{
	return tcm_override_readl((void*)(engine[index].engine_base + CE_ABUS_REG_OFFSET + offset));
}
static void crypto_write32(int index, size_t offset, uint32_t value)
{
	tcm_override_writel(value, (void*)(engine[index].engine_base + CE_CRYPTO_REG_OFFSET + offset));
}
static uint32_t crypto_read32(int index, size_t offset)
{
	return tcm_override_readl((void*)(engine[index].engine_base + CE_CRYPTO_REG_OFFSET + offset));
}

/* just port from syscon_regmap_lookup_by_compatible */
struct regmap *ky_syscon_regmap_lookup_by_compatible(const char *s)
{
        struct device_node *syscon_np;
        struct regmap *regmap;

        syscon_np = of_find_compatible_node(NULL, NULL, s);
        if (!syscon_np)
                return ERR_PTR(-ENODEV);

        regmap = syscon_node_to_regmap(syscon_np);
        of_node_put(syscon_np);

        return regmap;
}
EXPORT_SYMBOL_GPL(ky_syscon_regmap_lookup_by_compatible);

void dump_data(const unsigned char *tag, const unsigned char *str, unsigned int len)
{
	char *p_addr;
	uint8_t *buff;
	int i = 0;
	uint32_t size = 0;

	p_addr = (char *) kmalloc(len * 2 + 1, GFP_KERNEL);
	if (!p_addr) {
		dev_err(dev, "kmalloc failed!\n");
		return;
	}

	memset(p_addr, 0, len * 2 + 1);
	buff = (uint8_t *)str;
	for (i = 0; i < len; i++) {
		size += sprintf(p_addr + size, "%02x", buff[i]);
	}
	dev_info(dev," %s:%s\n", tag, p_addr);

	kfree((void *)p_addr);
}

static void engine_irq_enable(int index)
{
	uint32_t val;

	/* clear aes INT */
	val = crypto_read32(index, CE_CRYPTO_AES_INTRPT_SRC_REG);
	crypto_write32(index, CE_CRYPTO_AES_INTRPT_SRC_REG, val);

	val = crypto_read32(index, CE_CRYPTO_AES_INTRPT_SRC_EN_REG);
	val |= AES_INTERRUPT_MASK;
	crypto_write32(index, CE_CRYPTO_AES_INTRPT_SRC_EN_REG, val);

	val = dma_read32(index, CE_DMA_OUT_INT_MASK);
	val &=~DMA_INTERRUPT_MASK;
	dma_write32(index, CE_DMA_OUT_INT_MASK, val);

	val = dma_read32(index, CE_DMA_IN_INT_MASK);
	val &=~DMA_INTERRUPT_MASK;
	dma_write32(index, CE_DMA_IN_INT_MASK, val);
}
static void enable_biu_mask(int index)
{
	uint32_t val;
	val = biu_read32(index, SP_INTERRUPT_MASK);
	val &=~BIU_MASK;
	biu_write32(index, SP_INTERRUPT_MASK, val);
}
static void enable_adec_mask(int index)
{
	uint32_t val;
	val = adec_read32(index, CE_ADEC_INT_MSK);
	val &=~ADEC_MASK;
	adec_write32(index, CE_ADEC_INT_MSK, val);
}

static void dma_output_start(int index)
{
	uint32_t val;

	val = dma_read32(index, CE_DMA_OUT_INT);
	dma_write32(index, CE_DMA_OUT_INT, val);

	val = dma_read32(index, CE_DMA_OUT_CTRL);
	val |= 0x1;
	dma_write32(index, CE_DMA_OUT_CTRL, val);

	return;
}

static void dma_output_stop(int index)
{
	uint32_t val;

	val = dma_read32(index, CE_DMA_OUT_CTRL);
	val &= ~0x1;
	dma_write32(index, CE_DMA_OUT_CTRL, val);

	return;
}

static int dma_input_config(int index, int rid_ext, int rid)
{
	uint32_t val;

	val = dma_read32(index, CE_DMA_IN_CTRL);
	val &= 0x0f0f0000;
	val |= (0x7 << 28) |	/* dis error check */
	    ((rid_ext & 0xF) << 20) |	/* rid ext */
	    (0x1 << 18) |	/* dis out-of-order */
	    (0x1 << 17) |	/* data 64 Byte aligned */
	    (0x1 << 15) |	/* FIFO bus size 64bit */
	    (0x1 << 13) |	/* burst type: Inc */
	    (0x8 << 8) |	/* burst len */
	    ((rid & 0xF) << 4);

	dma_write32(index, CE_DMA_IN_CTRL, val);

	return 0;
}

static int dma_input_address(int index, uint32_t src_addr, uint32_t src_size, bool chained)
{
	if (chained == true) {
		dma_write32(index, CE_DMA_IN_NX_LL_ADR, src_addr);
		dma_write32(index, CE_DMA_IN_SRC_ADR, 0x0);
		dma_write32(index, CE_DMA_IN_XFER_CNTR, 0x0);
	} else {
		dma_write32(index, CE_DMA_IN_NX_LL_ADR, 0x0);
		dma_write32(index, CE_DMA_IN_SRC_ADR, src_addr);
		dma_write32(index, CE_DMA_IN_XFER_CNTR, src_size);
	}

	return 0;
}

static void dma_input_start(int index)
{
	uint32_t val;

	val = dma_read32(index, CE_DMA_IN_INT);
	dma_write32(index, CE_DMA_IN_INT, val);

	val = dma_read32(index, CE_DMA_IN_CTRL);
	val |= 0x1;
	dma_write32(index, CE_DMA_IN_CTRL, val);

	return;
}

static void dma_input_stop(int index)
{
	uint32_t val;

	val = dma_read32(index, CE_DMA_IN_CTRL);
	val &= ~0x1;
	dma_write32(index, CE_DMA_IN_CTRL, val);

	return;
}


static int __maybe_unused dma_wait_int_output_finish(int index)
{
	wait_for_completion(&engine[index].dma_output_done);
	if(engine[index].dma_out_status != DMA_INOUT_DONE)
		return -EINVAL;

	return 0;
}

static int __maybe_unused dma_wait_int_input_finish(int index)
{
	wait_for_completion(&engine[index].dma_input_done);
	if(engine[index].dma_in_status != DMA_INOUT_DONE)
		return -EINVAL;

	return 0;
}

static int dma_output_config(int index, int wid_ext, int wid)
{
	uint32_t val;

	val = dma_read32(index, CE_DMA_OUT_CTRL);
	val &= 0x0f0f0000;
	val |= (0x7 << 28) |	/* dis error check */
	    ((wid_ext & 0xF) << 20) |	/* rid ext */
	    (0x1 << 18) |	/* dis out-of-order */
	    (0x1 << 17) |	/* data 64 Byte aligned */
	    (0x1 << 15) |	/* FIFO bus size 64bit */
	    (0x1 << 13) |	/* burst type: Inc */
	    (0x8 << 8) |	/* burst len */
	    ((wid & 0xF) << 4);

	dma_write32(index, CE_DMA_OUT_CTRL, val);

	return 0;
}

static int dma_output_address(int index, uint32_t dst_addr, uint32_t dst_size, bool chained)
{
	if (chained == true) {
		dma_write32(index, CE_DMA_OUT_NX_LL_ADR, dst_addr);
		dma_write32(index, CE_DMA_OUT_DEST_ADR, 0x0);
		dma_write32(index, CE_DMA_OUT_XFER_CNTR, 0x0);
	} else {
		dma_write32(index, CE_DMA_OUT_NX_LL_ADR, 0x0);
		dma_write32(index, CE_DMA_OUT_DEST_ADR, dst_addr);
		dma_write32(index, CE_DMA_OUT_XFER_CNTR, dst_size);
	}

	return 0;
}

static int adec_engine_hw_reset(int index, ADEC_ACC_ENG_T engine)
{
	uint32_t val;
	int tmp;

	if (engine == E_ACC_ENG_ALL)
		tmp = 0xffff;
	else
		tmp = 1 << engine;

	val = adec_read32(index, CE_ADEC_CTRL);
	val |= tmp;
	adec_write32(index, CE_ADEC_CTRL, val);
	val &= ~tmp;
	adec_write32(index, CE_ADEC_CTRL, val);

	if (engine == E_ACC_ENG_DMA) {
		regmap_update_bits(ciu_base, ENGINE_DMA_ADDR_HIGH_OFFSET,
				(WADDR_BIT32 | RADDR_BIT32), 0);
	}

	return 0;
}

static int abus_set_mode(int index, ABUS_GRP_A_T grp_a_mode,
			 ABUS_GRP_B_T grp_b_mode,
			 ABUS_CROSS_BAR_T input_bar,
			 ABUS_CROSS_BAR_T output_bar)
{
	uint32_t val;

	val = abus_read32(index, CE_ABUS_BUS_CTRL);

	val &= ~(0x77 << 0x4);
	val |= (grp_a_mode << 0x4) | (grp_b_mode << 0x8);

	if (input_bar == E_ABUS_STRAIGHT) {
		val &= ~(0x1 << 0x0);
	} else if (input_bar == E_ABUS_CROSS) {
		val |= (0x1 << 0x0);
	} else {
		return -EINVAL;
	}

	if (output_bar == E_ABUS_STRAIGHT) {
		val &= ~(0x1 << 0x2);
	} else if (output_bar == E_ABUS_CROSS) {
		val |= (0x1 << 0x2);
	} else {
		return -EINVAL;
	}

	abus_write32(index, CE_ABUS_BUS_CTRL, val);

	return 0;
}

static void crypto_aes_sw_reset(int index)
{
	uint32_t val;

	val = 0x1;
	crypto_write32(index, CE_CRYPTO_AES_CONTROL_REG, val);
	val = 0x0;
	crypto_write32(index, CE_CRYPTO_AES_CONTROL_REG, val);

	return;
}
static void crypto_aes_start(int index)
{
	uint32_t val;

	val = 0x1;
	crypto_write32(index, CE_CRYPTO_AES_COMMAND_REG, val);

	return;
}


static int crypto_aes_wait(int index)
{
	wait_for_completion(&engine[index].aes_done);
	if(engine[index].aes_status != AES_DONE)
	{
		dev_err_once(dev, "%s : %d : engine[%d].status = %d\n",__func__,__LINE__,index,engine[index].aes_status);
		return -EINVAL;
	}

	return 0;
}

static int crypto_engine_select(int index, CRYPTO_ENG_SEL_T engine)
{
	uint32_t val;

	val = crypto_read32(index, CE_CRYPTO_ENGINE_SEL_REG);
	val &= ~0x3;

	switch (engine) {
	case E_ENG_AES:
		val |= (0x1);
		break;
	default:
		return -EINVAL;
	}

	crypto_write32(index, CE_CRYPTO_ENGINE_SEL_REG, val);

	return 0;
}

static int crypto_aes_set_iv(int index, const uint8_t *iv)
{
	uint32_t val;
	int reg_index;

	if (iv == NULL)
		return -EINVAL;

	for(reg_index = 0; reg_index < 4; reg_index++)
	{
		val = ((iv[(reg_index << 2) + 0] & 0xFF)<< 0)|\
			((iv[(reg_index << 2) + 1] & 0xFF)<< 8)|\
			((iv[(reg_index << 2) + 2] & 0xFF)<< 16)|\
			((iv[(reg_index << 2) + 3] & 0xFF)<< 24);
		crypto_write32(index, CE_CRYPTO_IV_REG(reg_index),val);
	}

	return 0;
}

static int crypto_aes_get_iv(int index, uint8_t *iv)
{
	uint32_t val;
	int reg_index;

	if (iv == NULL)
		return -EINVAL;

	for(reg_index = 0; reg_index < 4; reg_index++)
	{
		val = crypto_read32(index, CE_CRYPTO_IV_REG(reg_index));
		iv[(reg_index << 2) + 0] = val & 0xFF;
		iv[(reg_index << 2) + 1] = (val >> 8) & 0xFF;
		iv[(reg_index << 2) + 2] = (val >> 16) & 0xFF;
		iv[(reg_index << 2) + 3] = (val >> 24) & 0xFF;
	}

	return 0;
}

static int crypto_aes_set_mode(int index, AES_MODE_T mode,
			       AES_OP_MODE_T op_mode,
			       AES_KEY_LEN_T keylen, bool use_rkey)
{
	uint32_t val;

	crypto_engine_select(index, E_ENG_AES);

	val = crypto_read32(index, CE_CRYPTO_AES_CONFIG_REG);
	val &= ~(0x7 << 0x3);
	switch (mode) {
	case E_AES_ECB:
		val |= (0x0 << 0x3);
		break;
	case E_AES_CBC:
		val |= (0x1 << 0x3);
		break;
	case E_AES_XTS:
		val |= (0x3 << 0x3);
		break;
	default:
		return -EINVAL;
	}

	val &= ~(0x3 << 0x1);
	switch (keylen) {
	case E_AES_128:
		val |= (0x0 << 0x1);
		break;
	case E_AES_192:
		val |= (0x2 << 0x1);
		break;
	case E_AES_256:
		val |= (0x1 << 0x1);
		break;
	default:
		return -EINVAL;
	}

	val &= ~(0x1 << 0x0);
	if (op_mode == E_AES_DECRYPT) {
		val |= (0x1 << 0x0);
	} else {
		val |= (0x0 << 0x0);
	}

	val &= ~(0x1 << 0x6);
	if (use_rkey == false) {
		val |= (0x0 << 0x6);
	} else {
		val |= (0x1 << 0x6);
	}

	crypto_write32(index, CE_CRYPTO_AES_CONFIG_REG, val);

	return 0;
}

static int crypto_aes_set_key1(int index, const uint8_t *key, AES_KEY_LEN_T keylen)
{
	uint32_t val;
	int reg_index, key_end;

	if (!key)
		return 0;

	switch (keylen) {
	case E_AES_128:
		key_end = 4;
		break;
	case E_AES_192:
		key_end = 6;
		break;
	case E_AES_256:
		key_end = 8;
		break;
	default:
		key_end = 0;
		return -EINVAL;
	}

	for (reg_index = 0; reg_index < 8; reg_index++) {
		if (reg_index < key_end) {
			val = ((key[0 + (reg_index << 2)] & 0xFF) << 0) |
			    ((key[1 + (reg_index << 2)] & 0xFF) << 8) |
			    ((key[2 + (reg_index << 2)] & 0xFF) << 16) |
			    ((key[3 + (reg_index << 2)] & 0xFF) << 24);
		} else {
			val = 0;
		}
		crypto_write32(index, CE_CRYPTO_K1_W_REG(reg_index), val);
	}

	return 0;
}

static int crypto_aes_set_key2(int index, const uint8_t *key, AES_KEY_LEN_T keylen)
{
	uint32_t val;
	int reg_index, key_end;

	if (!key)
		return 0;

	switch (keylen) {
	case E_AES_128:
		key_end = 4;
		break;
	case E_AES_192:
		key_end = 6;
		break;
	case E_AES_256:
		key_end = 8;
		break;
	default:
		key_end = 0;
		return -EINVAL;
	}

	for (reg_index = 0; reg_index < 8; reg_index++) {
		if (reg_index < key_end) {
			val = ((key[0 + (reg_index << 2)] & 0xFF) << 0) |
			    ((key[1 + (reg_index << 2)] & 0xFF) << 8) |
			    ((key[2 + (reg_index << 2)] & 0xFF) << 16) |
			    ((key[3 + (reg_index << 2)] & 0xFF) << 24);
		} else {
			val = 0;
		}
		crypto_write32(index, CE_CRYPTO_K2_W_REG(reg_index), val);
	}

	return 0;
}

int ce_rijndael_setup_internal(int index, const unsigned char *key, int keylen)
{
	if (!key || keylen <= 0) {
		goto error;
	}

	adec_engine_hw_reset(index, E_ACC_ENG_DMA);
	adec_engine_hw_reset(index, E_ACC_ENG_CRYPTO);
	abus_set_mode(index, E_ABUS_GRP_A_HASH, E_ABUS_GRP_B_AES, E_ABUS_STRAIGHT, E_ABUS_STRAIGHT);
	crypto_aes_sw_reset(index);

	enable_biu_mask(index);
	enable_adec_mask(index);
	engine_irq_enable(index);

	return 0;
error:
	return -ENOKEY;
}

BLOCKING_NOTIFIER_HEAD(ky_crypto_chain);

static struct crypto_alg *ky_crypto_mod_get(struct crypto_alg *alg)
{
	return try_module_get(alg->cra_module) ? crypto_alg_get(alg) : NULL;
}

static void ky_crypto_mod_put(struct crypto_alg *alg)
{
	struct module *module = alg->cra_module;

	crypto_alg_put(alg);
	module_put(module);
}

static void ky_crypto_larval_destroy(struct crypto_alg *alg)
{
	struct crypto_larval *larval = (void *)alg;

	BUG_ON(!crypto_is_larval(alg));
	if (!IS_ERR_OR_NULL(larval->adult))
		ky_crypto_mod_put(larval->adult);
	kfree(larval);
}

static struct crypto_larval *ky_crypto_larval_alloc(const char *name, u32 type, u32 mask)
{
	struct crypto_larval *larval;

	larval = kzalloc(sizeof(*larval), GFP_KERNEL);
	if (!larval)
		return ERR_PTR(-ENOMEM);

	larval->mask = mask;
	larval->alg.cra_flags = CRYPTO_ALG_LARVAL | type;
	larval->alg.cra_priority = -1;
	larval->alg.cra_destroy = ky_crypto_larval_destroy;

	strlcpy(larval->alg.cra_name, name, CRYPTO_MAX_ALG_NAME);
	init_completion(&larval->completion);

	return larval;
}

static struct crypto_alg *__ky_crypto_alg_lookup(const char *name, u32 type,
						u32 mask)
{
	struct crypto_alg *q, *alg = NULL;
	int best = -2;

	list_for_each_entry(q, &crypto_alg_list, cra_list) {
		int exact, fuzzy;

		if (q->cra_flags & (CRYPTO_ALG_DEAD | CRYPTO_ALG_DYING))
			continue;

		if ((q->cra_flags ^ type) & mask)
			continue;

		if ((q->cra_flags & CRYPTO_ALG_LARVAL) &&
			!crypto_is_test_larval((struct crypto_larval *)q) &&
			((struct crypto_larval *)q)->mask != mask)
			continue;

		exact = !strcmp(q->cra_driver_name, name);
		fuzzy = !strcmp(q->cra_name, name);
		if (!exact && !(fuzzy && q->cra_priority > best))
			continue;

		if (unlikely(!ky_crypto_mod_get(q)))
			continue;

		best = q->cra_priority;
		if (alg)
			ky_crypto_mod_put(alg);
		alg = q;

		if (exact)
			break;
	}

	return alg;
}

static struct crypto_alg *ky_crypto_alg_lookup(const char *name, u32 type,
						u32 mask)
{
	struct crypto_alg *alg;
	u32 test = 0;

	if (!((type | mask) & CRYPTO_ALG_TESTED))
		test |= CRYPTO_ALG_TESTED;

	down_read(&crypto_alg_sem);
	alg = __ky_crypto_alg_lookup(name, type | test, mask | test);
	if (!alg && test) {
		alg = __ky_crypto_alg_lookup(name, type, mask);
		if (alg && !crypto_is_larval(alg)) {
			/* Test failed */
			ky_crypto_mod_put(alg);
			alg = ERR_PTR(-ELIBBAD);
		}
	}
	up_read(&crypto_alg_sem);

	return alg;
}

static void ky_crypto_start_test(struct crypto_larval *larval)
{
	if (!larval->alg.cra_driver_name[0])
		return;

	if (larval->test_started)
		return;

	down_write(&crypto_alg_sem);
	if (larval->test_started) {
		up_write(&crypto_alg_sem);
		return;
	}

	larval->test_started = true;
	up_write(&crypto_alg_sem);

	crypto_wait_for_test(larval);
}

static struct crypto_alg *ky_crypto_larval_wait(struct crypto_alg *alg)
{
	struct crypto_larval *larval = (void *)alg;
	long timeout;

	if (!crypto_boot_test_finished())
		ky_crypto_start_test(larval);

	timeout = wait_for_completion_killable_timeout(
		&larval->completion, 60 * HZ);

	alg = larval->adult;
	if (timeout < 0)
		alg = ERR_PTR(-EINTR);
	else if (!timeout)
		alg = ERR_PTR(-ETIMEDOUT);
	else if (!alg)
		alg = ERR_PTR(-ENOENT);
	else if (IS_ERR(alg))
		;
	else if (crypto_is_test_larval(larval) &&
		!(alg->cra_flags & CRYPTO_ALG_TESTED))
		alg = ERR_PTR(-EAGAIN);
	else if (!ky_crypto_mod_get(alg))
		alg = ERR_PTR(-EAGAIN);
	ky_crypto_mod_put(&larval->alg);

	return alg;
}

static struct crypto_alg *ky_crypto_larval_add(const char *name,
						u32 type, u32 mask)
{
	struct crypto_alg *alg;
	struct crypto_larval *larval;

	larval = ky_crypto_larval_alloc(name, type, mask);
	if (IS_ERR(larval))
		return ERR_CAST(larval);

	refcount_set(&larval->alg.cra_refcnt, 2);

	down_write(&crypto_alg_sem);
	alg = __ky_crypto_alg_lookup(name, type, mask);
	if (!alg) {
		alg = &larval->alg;
		list_add(&alg->cra_list, &crypto_alg_list);
	}
	up_write(&crypto_alg_sem);

	if (alg != &larval->alg) {
		kfree(larval);
		if (crypto_is_larval(alg))
			alg = ky_crypto_larval_wait(alg);
	}

	return alg;
}

static void ky_crypto_larval_kill(struct crypto_alg *alg)
{
	struct crypto_larval *larval = (void *)alg;

	down_write(&crypto_alg_sem);
	list_del(&alg->cra_list);
	up_write(&crypto_alg_sem);
	complete_all(&larval->completion);
	crypto_alg_put(alg);
}

static struct crypto_alg *ky_crypto_larval_lookup(const char *name,
						u32 type, u32 mask)
{
	struct crypto_alg *alg;

	if (!name)
		return ERR_PTR(-ENOENT);

	type &= ~(CRYPTO_ALG_LARVAL | CRYPTO_ALG_DEAD);
	mask &= ~(CRYPTO_ALG_LARVAL | CRYPTO_ALG_DEAD);

	alg = ky_crypto_alg_lookup(name, type, mask);
	if (!alg && !(mask & CRYPTO_NOLOAD)) {
		request_module("crypto-%s", name);

		if (!((type ^ CRYPTO_ALG_NEED_FALLBACK) & mask &
				CRYPTO_ALG_NEED_FALLBACK))
			request_module("crypto-%s-all", name);

		alg = ky_crypto_alg_lookup(name, type, mask);
	}

	if (!IS_ERR_OR_NULL(alg) && crypto_is_larval(alg))
		alg = ky_crypto_larval_wait(alg);
	else if (!alg)
		alg = ky_crypto_larval_add(name, type, mask);

	return alg;
}

static int ky_crypto_probing_notify(unsigned long val, void *v)
{
	int ok;

	ok = blocking_notifier_call_chain(&ky_crypto_chain, val, v);
	if (ok == NOTIFY_DONE) {
		request_module("cryptomgr");
		ok = blocking_notifier_call_chain(&ky_crypto_chain, val, v);
	}

	return ok;
}

static struct crypto_alg *ky_crypto_alg_mod_lookup(const char *name,
					u32 type, u32 mask)
{
	struct crypto_alg *alg;
	struct crypto_alg *larval;
	int ok;

	/*
	 * If the internal flag is set for a cipher, require a caller to
	 * to invoke the cipher with the internal flag to use that cipher.
	 * Also, if a caller wants to allocate a cipher that may or may
	 * not be an internal cipher, use type | CRYPTO_ALG_INTERNAL and
	 * !(mask & CRYPTO_ALG_INTERNAL).
	 */
	if (!((type | mask) & CRYPTO_ALG_INTERNAL))
		mask |= CRYPTO_ALG_INTERNAL;

	larval = ky_crypto_larval_lookup(name, type, mask);
	if (IS_ERR(larval) || !crypto_is_larval(larval))
		return larval;

	ok = ky_crypto_probing_notify(CRYPTO_MSG_ALG_REQUEST, larval);

	if (ok == NOTIFY_STOP) {
		alg = ky_crypto_larval_wait(larval);
	} else {
		ky_crypto_mod_put(larval);
		alg = ERR_PTR(-ENOENT);
	}
	ky_crypto_larval_kill(larval);
	return alg;
}

static struct crypto_alg *ky_crypto_find_alg(const char *alg_name,
				const struct crypto_type *frontend,
				u32 type, u32 mask)
{
	if (frontend) {
		type &= frontend->maskclear;
		mask &= frontend->maskclear;
		type |= frontend->type;
		mask |= frontend->maskset;
	}

	return ky_crypto_alg_mod_lookup(alg_name, type, mask);
}

static unsigned int ky_crypto_ctxsize(struct crypto_alg *alg, u32 type, u32 mask)
{
	const struct crypto_type *type_obj = alg->cra_type;
	unsigned int len;

	len = alg->cra_alignmask & ~(crypto_tfm_ctx_alignment() - 1);
	if (type_obj)
		return len + type_obj->ctxsize(alg, type, mask);

	switch (alg->cra_flags & CRYPTO_ALG_TYPE_MASK) {
	default:
		BUG();

	case CRYPTO_ALG_TYPE_CIPHER:
		len += alg->cra_ctxsize;
		break;

	case CRYPTO_ALG_TYPE_COMPRESS:
		len += alg->cra_ctxsize;
		break;
	}

	return len;
}

static unsigned int sw_aes_ce_decrypt(unsigned char *in, unsigned char *out,
				unsigned char *key, unsigned int keylen)
{
	int ret = 0;
	struct crypto_alg *alg;
	struct crypto_tfm *tfm;
	unsigned int tfm_size;
	struct crypto_aes_ctx aes_ctx;

	alg = ky_crypto_find_alg("aes-generic", NULL, 0, 0);
	if (IS_ERR(alg)) {
		dev_err_once(dev, "%s : %d : find crypto sw-aes-ce failed!\n",
					__func__,__LINE__);
		ret = -1;
		goto exit;
	}
	dev_err_once(dev, "%s : %d : algo drv name %s.\n",__func__,__LINE__,
					alg->cra_driver_name);

	tfm_size = sizeof(*tfm) + ky_crypto_ctxsize(alg, 0, 0);
	tfm = kzalloc(tfm_size, GFP_KERNEL);
	if (tfm == NULL) {
		dev_err_once(dev, "%s : %d : alloc tfm failed.\n",__func__,
					__LINE__);
		ret = -1;
		goto exit;
	}
	tfm->__crt_ctx[0] = (void *)&aes_ctx;

	alg->cra_cipher.cia_setkey(tfm, (const uint8_t *)key, keylen);
	alg->cra_cipher.cia_decrypt(tfm, out, in);

	kfree(tfm);
exit:
	return ret;
}

static int ce_aes_process_nblocks(int index, const unsigned char *buf_in, unsigned char *buf_out,
				  unsigned long blocks, symmetric_key * skey1,symmetric_key * skey2,
				  AES_MODE_T mode,uint8_t *inv, AES_OP_MODE_T op)
{
	int ret;
	uint32_t dma_addr_in_low,dma_addr_in_high;
	uint32_t dma_addr_out_low,dma_addr_out_high;
	uint32_t val;

	dma_sync_single_for_device(dev,dma_addr_in,blocks*16,DMA_TO_DEVICE);
	if (dma_mapping_error(dev, dma_addr_in)) {
		dev_err(dev, "failed to map buffer\n");
		return -EFAULT;
	}
	if (dma_mapping_error(dev, dma_addr_out)) {
		dev_err(dev, "failed to map buffer\n");
		return -EFAULT;
	}

	dma_addr_in_high = upper_32_bits(dma_addr_in);
	dma_addr_in_low = lower_32_bits(dma_addr_in);
	dma_addr_out_high = upper_32_bits(dma_addr_out);
	dma_addr_out_low = lower_32_bits(dma_addr_out);

	/*reset the HW before using it*/
	adec_engine_hw_reset(index, E_ACC_ENG_DMA);
	adec_engine_hw_reset(index, E_ACC_ENG_CRYPTO);
	abus_set_mode(index, E_ABUS_GRP_A_HASH, E_ABUS_GRP_B_AES, E_ABUS_STRAIGHT, E_ABUS_STRAIGHT);
	crypto_aes_sw_reset(index);

	/*
	The CIU REGISTER(ENGINE_DMA_ADDR_HIGH_OFFSET,offset=0x70) is
	represent for the high address. The bits' definition:
		BIT4 : the write addr of engine1
		BIT5 : the read addr of engine1
		TODO:change below if had engine2
   		BIT8-11 : the write addr of engine2
   		BIT12-15 : the read addr of engine2
	*/
	regmap_read(ciu_base, ENGINE_DMA_ADDR_HIGH_OFFSET, &val);
	switch (index) {
		case 0:
			val &= ~(WADDR_BIT32 | RADDR_BIT32);
			val |= ((dma_addr_out_high&0x1) << 4 | (dma_addr_in_high&0x1) << 5);
			break;
		case 1:
			val &= ~0xFF00;
			val |= ((dma_addr_out_high&0xF) << 8 | (dma_addr_in_high&0xF) << 12);;
			break;
		default:
			ret = -EINVAL;
			dev_err_once(dev, "%s : %d : index is error!\n",__func__,__LINE__);
			goto error;
	}
	regmap_write(ciu_base, ENGINE_DMA_ADDR_HIGH_OFFSET, val);

	if ((unsigned long) dma_addr_in & 0x3 || (unsigned long) dma_addr_out & 0x3) {
		ret = -EINVAL;
		dev_err_once(dev, "%s : %d : dma_addr_in or dma_addr_out is unaligned!\n",__func__,__LINE__);
		goto error;
	}

	enable_biu_mask(index);
	enable_adec_mask(index);
	engine_irq_enable(index);

	dma_input_config(index, 0, 0);
	dma_output_config(index, 0, 1);

	ret = dma_input_address(index, dma_addr_in_low, blocks << 2, false);
	if (ret != 0)
	{
		ret = -EINVAL;
		dev_err_once(dev, "%s : %d : dma_input_address failed!\n",__func__,__LINE__);
		goto error;
	}

	ret = dma_output_address(index, dma_addr_out_low, blocks << 2, false);
	if (ret != 0)
	{
		ret = -EINVAL;
		dev_err_once(dev, "%s : %d : dma_output_address failed!\n",__func__,__LINE__);
		goto error;
	}

	/* Process KEY*/
	if (skey1 == NULL) {
		ret = -EINVAL;
		dev_err_once(dev, "%s : %d : skey1 is NULL!\n",__func__,__LINE__);
		goto error;
	}
	ret = crypto_aes_set_mode(index, mode, op, skey1->rijndael.Nr , false);
	if (ret) {
		dev_err_once(dev, "%s : %d : crypto_aes_set_mode failed!\n",__func__,__LINE__);
		goto error;
	}
	switch(op) {
		case E_AES_ENCRYPT:
			ret = crypto_aes_set_key1(index, (uint8_t *)skey1->rijndael.eK, skey1->rijndael.Nr );
			break;
		case E_AES_DECRYPT:
			ret = crypto_aes_set_key1(index, (uint8_t *)skey1->rijndael.dK, skey1->rijndael.Nr );
			break;
		default:
			dev_err_once(dev, "%s : %d : cmd(op) is invalid!\n",__func__,__LINE__);
			ret = -EINVAL;
	}
	if (ret) {
		dev_err_once(dev, "%s : %d : set_key1 failed!\n",__func__,__LINE__);
		goto error;
	}

	/* Process IV*/
	switch(mode) {
		case E_AES_XTS:
			if (!skey2) {
				dev_err_once(dev, "%s : %d: skey2 is invalid in xts mode.\n", __func__,__LINE__);
				ret = -EINVAL;
				goto error;
			}
			if (op == E_AES_ENCRYPT)
				ret = crypto_aes_set_key2(index, (uint8_t *)skey2->rijndael.eK, skey2->rijndael.Nr);
			else
				ret = crypto_aes_set_key2(index, (uint8_t *)skey2->rijndael.dK, skey2->rijndael.Nr);
			if (ret != 0) {
				dev_err_once(dev, "%s : %d : crypto_aes_set_key2 failed!\n",__func__,__LINE__);
				goto error;
			}
			ret = crypto_aes_set_iv(index, inv);
			if (ret != 0) {
				dev_err_once(dev, "%s : %d : crypto_aes_set_iv failed!\n",__func__,__LINE__);
				goto error;
			}
			break;
		case E_AES_CBC:
		case E_AES_CTR:
			ret = crypto_aes_set_iv(index, inv);
			if (ret != 0) {
				dev_err_once(dev, "%s : %d : crypto_aes_set_iv failed!\n",__func__,__LINE__);
				goto error;
			}
			break;
		default:
			break;
	}

	crypto_write32(index, CE_CRYPTO_AES_STREAM_SIZE_REG, blocks << 4);

	dma_input_start(index);
	dma_output_start(index);
	crypto_aes_start(index);

	ret = dma_wait_int_input_finish(index);
	if (ret) {
		dev_err_once(dev, "%s : %d : dma_wait_input_finish failed! ret = %d\n",__func__,__LINE__,ret);
		goto error;
	}

	ret = crypto_aes_wait(index);
	if (ret) {
		dev_err_once(dev, "%s : %d : crypto_aes_wait failed! ret = %d\n",__func__,__LINE__,ret);
		goto error;
	}
	ret = dma_wait_int_output_finish(index);
	if (ret) {
		dev_err_once(dev, "%s : %d : dma_wait_output_finish failed! ret = %d\n",__func__,__LINE__,ret);
		goto error;
	}
	dma_sync_single_for_cpu(dev,dma_addr_out,blocks*16,DMA_FROM_DEVICE);

	/* Readback IV after operation*/
	switch(mode) {
		case E_AES_XTS:
		case E_AES_CBC:
		case E_AES_CTR:
			ret = crypto_aes_get_iv(index, inv);
			if (ret != 0) {
				dev_err_once(dev, "%s : %d : crypto_aes_get_iv failed!\n",__func__,__LINE__);
				goto error;
			}
			break;
		default:
			break;
	}
	return 0;
error:
	dev_err_once(dev, "====================failed==============\n");
	dev_err_once(dev, "%s : %d : failed! mode=%s,op=%s,keylen=%d\n",__func__,__LINE__,
		(mode==E_AES_CBC?"cbc":(mode==E_AES_CTR?"ctr":(mode==E_AES_ECB?"ecb":(mode==E_AES_XTS?"xts":"err")))),
		(op==E_AES_ENCRYPT?"encrypt":(op==E_AES_DECRYPT?"decrypt":"err")),
		(skey1==NULL?0:skey1->rijndael.Nr));
	return ret;
}

static int ce_aes_process_nblocks_noalign(int index, const unsigned char *buf_in, unsigned char *buf_out,
				  unsigned long blocks, symmetric_key * skey1, symmetric_key * skey2,
				  AES_MODE_T mode, uint8_t *inv, AES_OP_MODE_T op) {
	int ret;
	int len_bytes = 0;
	int step_bytes = 0;
	unsigned char *in_cpy = NULL, *out_cpy = NULL;
	unsigned char *in_work = NULL, *out_work = NULL;
	unsigned char *aligned_buf_1 = &engine[index].internal_working_buffer[0];
	unsigned char *aligned_buf_2 = &engine[index].internal_working_buffer[WORK_BUF_SIZE];

	if ((unsigned long) buf_in & 0x3 || (unsigned long) buf_out & 0x3) {
		len_bytes = blocks << 4;
		in_cpy = (unsigned char *) buf_in;
		out_cpy = (unsigned char *) buf_out;

		while(len_bytes) {
			step_bytes = len_bytes > WORK_BUF_SIZE ? WORK_BUF_SIZE : len_bytes;
			if((unsigned long) buf_in & 0x3) {
				memcpy(aligned_buf_1, in_cpy, step_bytes);
				in_work = aligned_buf_1;
			} else {
				in_work = in_cpy;
			}
			len_bytes -= step_bytes;
			in_cpy += step_bytes;
			if((unsigned long) buf_out & 0x3) {
				memset(aligned_buf_2, 0x0, WORK_BUF_SIZE);
				out_work = aligned_buf_2;
			} else {
				out_work = out_cpy;
			}
			ret = ce_aes_process_nblocks(index, in_work, out_work, step_bytes >> 4, skey1, skey2, mode, inv, op);
			if (ret != 0)
				goto exit;
			if((unsigned long) buf_out & 0x3) {
				memcpy(out_cpy, aligned_buf_2, step_bytes);
			}
			out_cpy += step_bytes;
			if ((mode == E_AES_XTS) && (len_bytes != 0) && (len_bytes > WORK_BUF_SIZE)) {
				unsigned char key_local[32];
				unsigned int key_len = (skey2->rijndael.Nr < 32) ? skey2->rijndael.Nr : 32;

				if (op == E_AES_ENCRYPT)
					memcpy(key_local, (unsigned char *)skey2->rijndael.eK, key_len);
				else
					memcpy(key_local, (unsigned char *)skey2->rijndael.dK, key_len);
				sw_aes_ce_decrypt(inv, inv, key_local, key_len);
			}
		}
	} else {
		ret = ce_aes_process_nblocks(index, buf_in, buf_out, blocks, skey1, skey2, mode, inv, op);
		if (!ret && (mode == E_AES_XTS)) {
			unsigned char key_local[32];
			unsigned int key_len = (skey2->rijndael.Nr < 32) ? skey2->rijndael.Nr : 32;

			if (op == E_AES_ENCRYPT)
				memcpy(key_local, (unsigned char *)skey2->rijndael.eK, key_len);
			else
				memcpy(key_local, (unsigned char *)skey2->rijndael.dK, key_len);
			sw_aes_ce_decrypt(inv, inv, key_local, key_len);
		}
	}

exit:
	memset(aligned_buf_1, 0x0, WORK_BUF_SIZE);
	memset(aligned_buf_2, 0x0, WORK_BUF_SIZE);
	return ret;
}

//---------------------------------------------------------
int ky_crypto_aes_set_key(int index, struct crypto_tfm *tfm, const u8 *key,unsigned int keylen)
{
	struct crypto_aes_ctx *ctx;

	if (!tfm || keylen <= 0) {
		goto error;
	}

	ctx = crypto_tfm_ctx(tfm);

	if ((!key) || (keylen > (int)(sizeof(ctx->key_enc)))
			|| (keylen > (int)(sizeof(ctx->key_dec)))){
			goto error;
	}

	ctx->key_length = keylen;
	memcpy(ctx->key_enc, key, ctx->key_length);
	memcpy(ctx->key_dec, key, ctx->key_length);

	return 0;
error:
	return -EINVAL;
}
EXPORT_SYMBOL(ky_crypto_aes_set_key);

int ky_aes_ecb_encrypt(int index, const unsigned char *pt,unsigned char *ct, u8 *key, unsigned int len,unsigned int blocks)
{
	symmetric_key skey1;
	skey1.rijndael.Nr=len;
	memcpy(skey1.rijndael.eK,key,min(len, sizeof(skey1.rijndael.eK)));

	return ce_aes_process_nblocks_noalign(index, pt,ct,blocks, &skey1,NULL,E_AES_ECB,NULL, E_AES_ENCRYPT);
}
EXPORT_SYMBOL(ky_aes_ecb_encrypt);

int ky_aes_ecb_decrypt(int index, const unsigned char *ct,unsigned char *pt, u8 *key, unsigned int len,unsigned int blocks)
{
	symmetric_key skey1;
	skey1.rijndael.Nr=len;
	memcpy(skey1.rijndael.dK,key,min(len, sizeof(skey1.rijndael.dK)));

	return ce_aes_process_nblocks_noalign(index, ct,pt,blocks, &skey1,NULL,E_AES_ECB,NULL, E_AES_DECRYPT);
}
EXPORT_SYMBOL(ky_aes_ecb_decrypt);

int ky_aes_cbc_encrypt(int index, const unsigned char *pt,unsigned char *ct, u8 *key, unsigned int len, u8 *IV,unsigned int blocks)
{
	symmetric_key skey1;
	skey1.rijndael.Nr=len;
	memcpy(skey1.rijndael.eK,key,min(len, sizeof(skey1.rijndael.eK)));

	return ce_aes_process_nblocks_noalign(index, pt,ct,blocks, &skey1,NULL,E_AES_CBC,IV,E_AES_ENCRYPT);
}
EXPORT_SYMBOL(ky_aes_cbc_encrypt);

int ky_aes_cbc_decrypt(int index, const unsigned char *ct,unsigned char *pt, u8 *key, unsigned int len, u8 *IV,unsigned int blocks)
{
	symmetric_key skey1;
	skey1.rijndael.Nr=len;
	memcpy(skey1.rijndael.dK,key,min(len, sizeof(skey1.rijndael.dK)));

	return ce_aes_process_nblocks_noalign(index, ct,pt,blocks, &skey1,NULL,E_AES_CBC,IV,E_AES_DECRYPT);
}
EXPORT_SYMBOL(ky_aes_cbc_decrypt);

int ky_aes_xts_encrypt(int index, const unsigned char *pt, unsigned char *ct,
			u8 *key1, u8 *key2, unsigned int len, u8 *IV,
			unsigned int blocks)
{
	symmetric_key skey1, skey2;

	skey1.rijndael.Nr = len;
	memcpy(skey1.rijndael.eK, key1, min(len, sizeof(skey1.rijndael.eK)));

	skey2.rijndael.Nr = len;
	memcpy(skey2.rijndael.eK, key2, min(len, sizeof(skey2.rijndael.eK)));

	return ce_aes_process_nblocks_noalign(index, pt, ct, blocks, &skey1, &skey2,
				E_AES_XTS, IV, E_AES_ENCRYPT);
}
EXPORT_SYMBOL(ky_aes_xts_encrypt);

int ky_aes_xts_decrypt(int index, const unsigned char *ct, unsigned char *pt,
			u8 *key1, u8 *key2, unsigned int len, u8 *IV,
			unsigned int blocks)
{
	symmetric_key skey1, skey2;

	skey1.rijndael.Nr = len;
	memcpy(skey1.rijndael.dK, key1, min(len, sizeof(skey1.rijndael.dK)));

	skey2.rijndael.Nr = len;
	memcpy(skey2.rijndael.dK, key2, min(len, sizeof(skey2.rijndael.dK)));

	return ce_aes_process_nblocks_noalign(index, ct, pt, blocks, &skey1, &skey2,
				E_AES_XTS, IV, E_AES_DECRYPT);
}
EXPORT_SYMBOL(ky_aes_xts_decrypt);

void ky_aes_getaddr(unsigned char **in,unsigned char **out)
{
	mutex_lock(&engine[0].eng_mutex);
	*in = in_buffer;
	*out = out_buffer;
}
EXPORT_SYMBOL(ky_aes_getaddr);

void ky_aes_reladdr(void)
{
	mutex_unlock(&engine[0].eng_mutex);
}
EXPORT_SYMBOL(ky_aes_reladdr);

__maybe_unused static void engine_reg_dump(int index)
{
        uint32_t val;
        printk("======> engine[%d] reg dump start! <======\n", index);

        /*BIU*/
        val = biu_read32(index, SP_HST_INTERRUPT_MASK);
        printk("BIU[%d] SP_HST_INTERRUPT_MASK: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_BIU_REG_OFFSET + SP_HST_INTERRUPT_MASK, val);
        val = biu_read32(index, SP_INTERRUPT_MASK);
        printk("BIU[%d] SP_INTERRUPT_MASK: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_BIU_REG_OFFSET + SP_INTERRUPT_MASK, val);
        val = biu_read32(index, SP_CONTROL);
        printk("BIU[%d] SP_CONTROL: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_BIU_REG_OFFSET + SP_CONTROL, val);

        /*ADEC*/
        val = adec_read32(index, CE_ADEC_CTRL);
        printk("ADEC[%d] CE_ADEC_CTRL: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_ADEC_CTRL, val);
        val = adec_read32(index, CE_ADEC_CTRL2);
        printk("ADEC[%d] CE_ADEC_CTRL2: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_ADEC_CTRL2, val);
        val = adec_read32(index, CE_AXI_SL_CTRL);
        printk("ADEC[%d] CE_AXI_SL_CTRL: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_AXI_SL_CTRL, val);
        val = adec_read32(index, CE_ADEC_INT);
        printk("ADEC[%d] CE_ADEC_INT: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_ADEC_INT, val);
        val = adec_read32(index, CE_ADEC_INT_MSK);
        printk("ADEC[%d] CE_ADEC_INT_MSK: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_ADEC_INT_MSK, val);
        val = adec_read32(index, CE_ADEC_ACC_ERR_ADR);
        printk("ADEC[%d] CE_ADEC_ACC_ERR_ADR: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_ADEC_ACC_ERR_ADR, val);
        val = adec_read32(index, CE_ADEC_MP_FIFO_ERR_ADR);
        printk("ADEC[%d] CE_ADEC_MP_FIFO_ERR_ADR: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ADEC_REG_OFFSET + CE_ADEC_MP_FIFO_ERR_ADR, val);

        /*ABUS*/
        val = abus_read32(index, CE_ABUS_BUS_CTRL);
        printk("ABUS[%d] CE_ABUS_BUS_CTRL: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_ABUS_REG_OFFSET + CE_ABUS_BUS_CTRL, val);

        /*DMA*/
        val = dma_read32(index, CE_DMA_IN_CTRL);
        printk("DMA[%d] CE_DMA_IN_CTRL: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_CTRL, val);
        val = dma_read32(index, CE_DMA_IN_STATUS);
        printk("DMA[%d] CE_DMA_IN_STATUS: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_STATUS, val);
        val = dma_read32(index, CE_DMA_IN_SRC_ADR);
        printk("DMA[%d] CE_DMA_IN_SRC_ADR: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_SRC_ADR, val);
        val = dma_read32(index, CE_DMA_IN_XFER_CNTR);
        printk("DMA[%d] CE_DMA_IN_XFER_CNTR: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_XFER_CNTR, val);
        val = dma_read32(index, CE_DMA_IN_NX_LL_ADR);
        printk("DMA[%d] CE_DMA_IN_NX_LL_ADR: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_NX_LL_ADR, val);
        val = dma_read32(index, CE_DMA_IN_INT);
        printk("DMA[%d] CE_DMA_IN_INT: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_INT, val);
        val = dma_read32(index, CE_DMA_IN_INT_MASK);
        printk("DMA[%d] CE_DMA_IN_INT_MASK: reg = 0x%lx, val = 0x%x\n", index, engine[index].engine_base + CE_DMA_REG_OFFSET + CE_DMA_IN_INT_MASK, val);

        printk("======> engine[%d] reg dump finish! <======\n", index);
}

static inline void clear_adec_biu_int_flag(int index)
{
	volatile uint32_t val;

	val = adec_read32(index, CE_ADEC_INT);
	adec_write32(index, CE_ADEC_INT, val);

	val = biu_read32(index, SP_INTERRUPT_RST);
	biu_write32(index, SP_INTERRUPT_RST, val);
}

static inline void engine_irq_handler(int index)
{
	volatile uint32_t val_aes;

	/* aes */
	val_aes = crypto_read32(index, CE_CRYPTO_AES_INTRPT_SRC_REG);
	if (val_aes & AES_INTERRUPT_MASK)
	{
		crypto_write32(index, CE_CRYPTO_AES_INTRPT_SRC_REG, val_aes);
		clear_adec_biu_int_flag(index);
		engine[index].aes_status = (val_aes & AES_INTERRUPT_FLAG) ? AES_DONE : AES_ERROR;
		if(!(val_aes & AES_INTERRUPT_FLAG))
			dev_info(dev, "%s : %d : complete aes_done (0x%x) !\n",__func__,__LINE__,val_aes);
		complete(&engine[index].aes_done);
		return;
	}

	/* dma output */
	val_aes = dma_read32(index, CE_DMA_OUT_INT);
	if (val_aes & DMA_INTERRUPT_MASK)
	{
		dma_output_stop(index);
		dma_write32(index, CE_DMA_OUT_INT, val_aes);
		clear_adec_biu_int_flag(index);
		engine[index].dma_out_status = (val_aes & BIT_DMA_INOUT_DONE) ? DMA_INOUT_DONE : DMA_INOUT_ERROR;
		complete(&engine[index].dma_output_done);
		return;
	}

	/* dma input */
	val_aes = dma_read32(index, CE_DMA_IN_INT);
	if (val_aes & DMA_INTERRUPT_MASK)
	{
		dma_input_stop(index);
		dma_write32(index, CE_DMA_IN_INT, val_aes);
		clear_adec_biu_int_flag(index);
		engine[index].dma_in_status = (val_aes & BIT_DMA_INOUT_DONE) ? DMA_INOUT_DONE : DMA_INOUT_ERROR;
		complete(&engine[index].dma_input_done);
		return;
	}
}


static irqreturn_t engine_irq_handler_0(int irq, void *nouse)
{
	engine_irq_handler(0);

	return IRQ_HANDLED;
}

static irqreturn_t engine_irq_handler_1(int irq, void *nouse)
{
	engine_irq_handler(1);

	return IRQ_HANDLED;
}

irqreturn_t (* irq_func[ENGINE_MAX])(int, void *) ={
		&engine_irq_handler_0,
		&engine_irq_handler_1
};

#ifdef CONFIG_KY_CRYPTO_SELF_TEST
static struct {
	int keylen;
	unsigned char key[32];
	const unsigned char pt[16];
	unsigned char ct[16];
} tests[] = {
	{
		16, {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
		0x0e, 0x0f}, {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd,
		0xee, 0xff}, {
		0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
		0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4,
		0xc5, 0x5a}
	}, {
		24, {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
		0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17}, {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd,
		0xee, 0xff}, {
		0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
		0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d,
		0x71, 0x91}
	}, {
		32, {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
		0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
		0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f}, {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd,
		0xee, 0xff}, {
		0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
		0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49,
		0x60, 0x89}
	}
};
#define PT_CT_SIZE 4096

static int ce_aes_test(u32 num)
{
	int err;

	unsigned char iv[16];
	int i, y, ret;
	int index = num;
	unsigned char *ct_buf;
	unsigned char *pt_buf;
	unsigned char *ct_buf_tmp;
	unsigned char *pt_buf_tmp;

	ct_buf = kzalloc(PT_CT_SIZE, GFP_KERNEL);
	pt_buf = kzalloc(PT_CT_SIZE, GFP_KERNEL);

	ct_buf_tmp = kzalloc(PT_CT_SIZE, GFP_KERNEL);
	pt_buf_tmp = kzalloc(PT_CT_SIZE, GFP_KERNEL);

	while (--index >= 0) {
		dev_info(dev,"================ aes test(%d) =============\n",index);
		for (i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); i++) {
			ret = ce_rijndael_setup_internal(index, tests[i].key, tests[i].keylen * BYTES_TO_BITS);
			if (ret != 0) {
				goto err;
			}
			memcpy(ct_buf, tests[i].ct , 16);
			memcpy(pt_buf, tests[i].pt , 16);
			ky_aes_ecb_encrypt(index, pt_buf,ct_buf_tmp, tests[i].key, tests[i].keylen, 1);
			if (memcmp(ct_buf_tmp, tests[i].ct, 16)) {
				dev_err(dev,"  (ecb test)failed : tmp[0] != tests[i].ct\n");
				dump_data("(ecb ct)", (const unsigned char *)ct_buf_tmp, 16);
				ret = -EPERM;
				goto err;
			}
			ky_aes_ecb_decrypt(index, ct_buf_tmp, pt_buf_tmp, tests[i].key, tests[i].keylen, 1);
			dump_data("(ecb after encrypt-decrypt)", (const unsigned char *)pt_buf_tmp, 16);
			if (memcmp(pt_buf_tmp, tests[i].pt, 16)) {
				dev_err_once(dev,"  (ecb test)failed : tmp[1] != tests[i].pt\n");
				ret = -EPERM;
				goto err;
			}

			memset(ct_buf_tmp, 0, PT_CT_SIZE);
			memcpy(iv, "1234567890123456", sizeof(iv));
			ky_aes_cbc_encrypt(index, pt_buf, ct_buf_tmp, tests[i].key, tests[i].keylen, iv, 1);
			memset(pt_buf_tmp, 0, PT_CT_SIZE);
			memcpy(iv, "1234567890123456", sizeof(iv));
			ky_aes_cbc_decrypt(index, ct_buf_tmp, pt_buf_tmp, tests[i].key, tests[i].keylen, iv, 1);
			dump_data("(cbc after encrypt-decrypt)", (const unsigned char *)pt_buf_tmp, 16);
			if (memcmp(pt_buf_tmp, tests[i].pt, 16)) {
				dev_err_once(dev,"  (cbc test)failed : tmp[1] != tests[i].pt\n");
				ret = -EPERM;
				goto err;
			}

			/* now see if we can encrypt all zero bytes 1000 times, decrypt and come back where we started */
			memset(ct_buf_tmp, 0, PT_CT_SIZE);
			for (y = 0; y < 100; y++) {
				ky_aes_ecb_encrypt(index, ct_buf_tmp, ct_buf_tmp, tests[i].key, tests[i].keylen, 1);
				memcpy(iv,"1234567890123456", sizeof(iv));
				ky_aes_cbc_encrypt(index, ct_buf_tmp, ct_buf_tmp, tests[i].key, tests[i].keylen, iv, 1);
			}
			for (y = 0; y < 100; y++) {
				memcpy(iv,"1234567890123456", sizeof(iv));
				ky_aes_cbc_decrypt(index, ct_buf_tmp, ct_buf_tmp, tests[i].key, tests[i].keylen, iv, 1);
				ky_aes_ecb_decrypt(index, ct_buf_tmp, ct_buf_tmp, tests[i].key, tests[i].keylen, 1);
			}
			for (y = 0; y < 16; y++) {
				if (ct_buf_tmp[y] != 0) {
					dev_err_once(dev," failed : encrypt & decrypt 100 times failed!\n");
					ret = -EPERM;
					goto err;
				}
			}
		}
		dev_info(dev,"                 successful                 \n");
	}

	return 0;
err:
	kfree(ct_buf);
	kfree(pt_buf);
	kfree(ct_buf_tmp);
	kfree(pt_buf_tmp);
	return ret;
}
#endif

#ifdef CONFIG_KY_CRYPTO_DEBUG
static enum engine_ddr_type check_addr_type(unsigned long pddr)
{
	int i;

	for(i=0;i < sizeof(sram_reserved)/sizeof(struct sram_area);i++)
	{
		if(pddr >= sram_reserved[i].sram_start && pddr <= sram_reserved[i].sram_end)
			return RESERVED_SRAM;
	}
	return RESERVED_DDR;
}

static int aes_test_for_nsaid(unsigned char *pt,unsigned char *ct,unsigned long engine_number)
{
	int err;
	static struct {
		int keylen;
		unsigned char key[32];
	} tests = {
			32,
			{
				0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
				0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
				0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
				0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
			}
	};

	int index;
	index = (int)engine_number;

	dev_info(dev,"================ aes test(%d) =============\n",index);

	if ((err = ce_rijndael_setup_internal(index, tests.key, tests.keylen * BYTES_TO_BITS)) != 0) {
		dev_err_once(dev,"ce_rijndael_setup_internal failed!\n");
		return err;
	}

	ky_aes_ecb_encrypt(index, pt, ct, tests.key, tests.keylen,1);
	dump_data("(ecb after encrypt)===",ct,16);

	dev_info(dev,"================ aes test(%d) end=============\n",index);
	return 0;
}
#endif

static ssize_t engine_store(struct device *dev,
		struct device_attribute *attr, const  char *buf, size_t count)
{
#ifndef CONFIG_KY_CRYPTO_DEBUG
	(void)dev;
	(void)buf;
	(void)count;
	dev_info(dev, "%s : %d : Debugging interface is not open !\n", __func__,__LINE__);
#else
	unsigned long pddr1,pddr2,index;
	enum engine_ddr_type pddr1_type,pddr2_type;
	unsigned char *pt,*ct;
	sscanf(buf,"0x%lx 0x%lx 0x%lx",&pddr1,&pddr2,&index);

	pddr1_type = check_addr_type(pddr1);
	pddr2_type = check_addr_type(pddr2);
	if(pddr1_type == RESERVED_SRAM && pddr2_type == RESERVED_SRAM)
	{
		sram_phy_base_src = pddr1;
		sram_phy_base_dst = pddr2;
		engine[(int)index].ddr_type = RESERVED_SRAM;
		pt = (char *)ioremap(pddr1, SRAM_MAP_SIZE);
		if (!pt)
		{
			dev_err_once(dev,"engine_store ioremap pddr1 failed!\n");
			return -ENOMEM;
		}

		ct = (char *)ioremap(pddr2, SRAM_MAP_SIZE);
		if (!ct)
		{
			dev_err_once(dev,"engine_store ioremap pddr2 failed!\n");
			iounmap(pt);
			return -ENOMEM;
		}
	}
	else if(pddr1_type == RESERVED_DDR && pddr2_type == RESERVED_DDR)
	{
		engine[(int)index].ddr_type = RESERVED_DDR;
		pt = (char *)phys_to_virt((unsigned long)pddr1);
		ct = (char *)phys_to_virt((unsigned long)pddr2);
	}
	else
	{
		dev_err_once(dev,"engine_store pddr bad parameters!\n");
		return count;
	}

	dev_dbg(dev,"engine_store (0x%lx,0x%lx)-->(0x%lx,0x%lx)\n",pddr1,pddr2,(unsigned long)pt,(unsigned long)ct);
	aes_test_for_nsaid(pt,ct,index);
	engine[(int)index].ddr_type = NORMAL_DDR;

	if(pddr1_type == RESERVED_SRAM && pddr2_type == RESERVED_SRAM)
	{
		iounmap(pt);
		iounmap(ct);
	}
#endif
	return count;
}

static DEVICE_ATTR(engine_fun, S_IWUSR | S_IRUGO, NULL, engine_store);

static struct attribute *engine_operation[] = {
	&dev_attr_engine_fun.attr,
	NULL
};

static const struct attribute_group engine_operations = {
	.name = "engine",
	.attrs = engine_operation
};

static const char *eng_names[ENGINE_MAX] = {
	"ky-crypto-engine-0",
};

/* ================================================
probe
===================================================*/
static int crypto_engine_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int i;
	uint32_t addr_range[2];
	unsigned int engine_irq;
	struct aes_clk_reset_ctrl *ctrl;
	char obj_name[32];
	const char *irq_name;
	u32 num_engines;
	dev = &pdev->dev;

	ret = of_property_read_u32(np, "num-engines", &num_engines);
	if(ret){
		dev_err_once(dev, "can't get %s from dts!\n", "num-engines");
		return -ENODEV;
	}

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "Unable to set dma mask\n");
		return ret;
	}

	in_buffer = dma_alloc_noncoherent(dev, KY_AES_BUFFER_LEN, &dma_addr_in, DMA_TO_DEVICE, GFP_KERNEL);
	out_buffer = dma_alloc_noncoherent(dev, KY_AES_BUFFER_LEN, &dma_addr_out, DMA_FROM_DEVICE, GFP_KERNEL);
	ctrl = kmalloc(sizeof(struct aes_clk_reset_ctrl), GFP_KERNEL);
	ctrl->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(ctrl->clk))
		return PTR_ERR(ctrl->clk);
	clk_prepare_enable(ctrl->clk);

	ctrl->reset = devm_reset_control_get_optional_shared(&pdev->dev, NULL);
	if(IS_ERR(ctrl->reset))
		return PTR_ERR(ctrl->reset);
	reset_control_deassert(ctrl->reset);

	pm_runtime_enable(&pdev->dev);
	platform_set_drvdata(pdev, ctrl);

	for(i=0; i < num_engines; i++)
	{
		sprintf(obj_name,"ky-crypto-engine-%d",i);
		init_completion(&engine[i].aes_done);
		init_completion(&engine[i].dma_output_done);
		init_completion(&engine[i].dma_input_done);
		mutex_init(&engine[i].eng_mutex);
		engine[i].ddr_type = NORMAL_DDR;
		engine[i].handler = irq_func[i];

		ret = of_property_read_u32_array(np, obj_name, &addr_range[0], 2);
		if(0 != ret){
			dev_err_once(dev, "can't get %s from dts!\n", obj_name);
			return -ENOMEM;
		}

		engine[i].engine_base = (unsigned long)ioremap(addr_range[0], addr_range[1]);
		if (engine[i].engine_base == 0)
		{
			dev_err_once(dev,"engine_mem ioremap failed. pyh_addr=0x%08x,pyh_size=0x%08x\n",addr_range[0], addr_range[1]);
			goto err_ioremap;
		}
		dev_dbg(dev, "map %s successful. pyh_addr=0x%08x,pyh_size=0x%08x, vir_addr=0x%lx\n", obj_name,addr_range[0], addr_range[1],engine[0].engine_base);

		engine_irq = irq_of_parse_and_map(np, i);
		if (!engine_irq) {
			dev_err_once(dev,"%s: %s irq_of_parse_and_map failed\n",__FILE__,obj_name);
			goto err_ioremap;
		}

		irq_name = eng_names[i];
		ret = request_irq(engine_irq, engine[i].handler,IRQF_TRIGGER_HIGH | IRQF_ONESHOT, irq_name, NULL);
		if (ret) {
			dev_err_once(dev,"failed to request %s IRQ\n",obj_name);
			goto err_ioremap;
		}

	}

#ifdef CONFIG_KY_CRYPTO_DEBUG
	{
		int j;
		int sram_index = 0;
		for (i = 0; i < SRAM_NUM; i++) {
			for (j = 0; j < SRAM_NUM; j++) {
				sprintf(obj_name,"ky-sub%d-sram%d",i,j);
				ret = of_property_read_u32_array(np, obj_name, &addr_range[0], 2);
				if(0 != ret){
					dev_err_once(dev, "can't get %s from dts!\n", obj_name);
					return -ENOMEM;
				}

				sram_reserved[sram_index].sram_start = addr_range[0];
				sram_reserved[sram_index].sram_end = addr_range[1];
				sram_reserved[sram_index].sram_size = addr_range[1] - addr_range[0];
				dev_dbg(dev, "sram_%d : 0x%lx 0x%lx 0x%lx\n", sram_index,sram_reserved[sram_index].sram_start,
					sram_reserved[sram_index].sram_end,sram_reserved[sram_index].sram_size);
				sram_index ++;
			}
		}
	}
#endif

	ciu_base = ky_syscon_regmap_lookup_by_compatible("ky,ciu");
	if (IS_ERR(ciu_base))
	{
		dev_err_once(dev,"ciu_base has not mapped. \n");
		goto err_ioremap;
	}

	regmap_update_bits(ciu_base, ENGINE_DMA_ADDR_HIGH_OFFSET,
			(SW_RESETN | MASTER_CLK_EN | SLAVE_CLK_EN),
			(SW_RESETN | MASTER_CLK_EN | SLAVE_CLK_EN));

	ret = sysfs_create_group(&dev->kobj, &engine_operations);
	if (ret) {
		dev_err_once(dev,"sysfs_create_group failed\n");
		return ret;
	}


	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(dev, "Unable to set dma mask\n");
		return ret;
	}

#ifdef CONFIG_KY_CRYPTO_SELF_TEST
	ce_aes_test(num_engines);
#endif
	return 0;

err_ioremap:
	return -EINVAL;
}

static int crypto_engine_remove(struct platform_device *pdev)
{
	struct aes_clk_reset_ctrl *ctrl = dev_get_drvdata(&pdev->dev);
	dma_free_noncoherent(dev, KY_AES_BUFFER_LEN, in_buffer, dma_addr_in, DMA_TO_DEVICE);
	dma_free_noncoherent(dev, KY_AES_BUFFER_LEN, out_buffer, dma_addr_out, DMA_FROM_DEVICE);
	clk_disable_unprepare(ctrl->clk);
	reset_control_assert(ctrl->reset);
	return 0;
}


static struct of_device_id crypto_engine_of_match[] = {
	{ .compatible = "ky,crypto_engine", },
	{}
};

#ifdef CONFIG_PM_SLEEP
static int ky_aes_suspend_noirq(struct device *dev)
{
	struct aes_clk_reset_ctrl *ctrl = dev_get_drvdata(dev);

	clk_disable_unprepare(ctrl->clk);

	return 0;
}

static int ky_aes_resume_noirq(struct device *dev)
{
	struct aes_clk_reset_ctrl *ctrl = dev_get_drvdata(dev);

	clk_prepare_enable(ctrl->clk);

	return 0;
}

static const struct dev_pm_ops ky_aes_pm_qos = {
	SET_NOIRQ_SYSTEM_SLEEP_PM_OPS(ky_aes_suspend_noirq,
			ky_aes_resume_noirq)
};
#endif

static struct platform_driver crypto_engine_driver = {
	.driver = {
		.name = "crypto_engine",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
		.pm	= &ky_aes_pm_qos,
#endif
		.of_match_table = crypto_engine_of_match,
	},
	.probe = crypto_engine_probe,
	.remove = crypto_engine_remove,

};

static int crypto_engine_init(void)
{
	return platform_driver_register(&crypto_engine_driver);
}

static void crypto_engine_exit(void)
{
	platform_driver_unregister(&crypto_engine_driver);
}


module_init(crypto_engine_init);
module_exit(crypto_engine_exit);

MODULE_LICENSE("GPL v2");
