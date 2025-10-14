// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2025 Cix Technology Group Co., Ltd.
 *
 */

#include "rdr_print.h"
#include <linux/types.h>
#include <linux/bch.h>
#include <linux/math.h>
#include <linux/bits.h>
#include <linux/soc/cix/rdr_pub.h>

static struct bch_control *rdr_bch;
static struct spinlock rdr_bch_spinlock;
static uint rdr_errloc[RDR_BCH_T];
#define SAFEMEM_POOL_MAGIC 0xdeaeaabe

int rdr_bch_encode(char *data, uint data_len, char *ecc, uint ecc_len)
{
	char *datatmp, *ecctmp;
	uint ecc_times = 0, int_times = 0;

	if (IS_ERR_OR_NULL(rdr_bch))
		return -ENODEV;

	ecc_times = DIV_ROUND_UP(data_len, RDR_BCH_MAX_BYTES);
	if (ecc_len < ecc_times * RDR_BCH_ECC_BYTES)
		return -EINVAL;

	int_times = data_len / RDR_BCH_MAX_BYTES;
	datatmp = data;
	ecctmp = ecc;
	spin_lock(&rdr_bch_spinlock);
	for (uint i = 0; i < int_times; i++) {
		bch_encode(rdr_bch, datatmp, RDR_BCH_MAX_BYTES, ecctmp);
		datatmp += RDR_BCH_MAX_BYTES;
		ecctmp += RDR_BCH_ECC_BYTES;
	}
	if (int_times == ecc_times)
		goto out;

	if (data_len % RDR_BCH_MAX_BYTES) {
		bch_encode(rdr_bch, datatmp, data_len % RDR_BCH_MAX_BYTES,
			   ecctmp);
	}
out:
	spin_unlock(&rdr_bch_spinlock);
	return 0;
}
EXPORT_SYMBOL(rdr_bch_encode);

int rdr_bch_checkout(char *data, uint data_len, char *ecc, uint ecc_len)
{
	char *datatmp, *ecctmp;
	uint ecc_times = 0, int_times = 0;
	int err, num = 0;

	if (IS_ERR_OR_NULL(rdr_bch))
		return -ENODEV;

	ecc_times = DIV_ROUND_UP(data_len, RDR_BCH_MAX_BYTES);
	if (ecc_len < ecc_times * RDR_BCH_ECC_BYTES)
		return -EINVAL;

	spin_lock(&rdr_bch_spinlock);
	int_times = data_len / RDR_BCH_MAX_BYTES;
	datatmp = data;
	ecctmp = ecc;
	for (uint i = 0; i < int_times; i++) {
		err = bch_decode(rdr_bch, datatmp, RDR_BCH_MAX_BYTES, ecctmp,
				 NULL, NULL, rdr_errloc);
		if (err < 0) {
			spin_unlock(&rdr_bch_spinlock);
			return -EIO;
		}
		num += err;
		/*fix bit err*/
		for (int j = 0; j < err; j++) {
			datatmp[rdr_errloc[j] >> 3] ^= BIT(rdr_errloc[j] & 7);
		}
		datatmp += RDR_BCH_MAX_BYTES;
		ecctmp += RDR_BCH_ECC_BYTES;
	}
	if (data_len % RDR_BCH_MAX_BYTES) {
		err = bch_decode(rdr_bch, datatmp, data_len % RDR_BCH_MAX_BYTES,
				 ecctmp, NULL, NULL, rdr_errloc);
		if (err < 0) {
			spin_unlock(&rdr_bch_spinlock);
			return -EIO;
		}
		num += err;
		/*fix bit err*/
		for (int j = 0; j < err; j++)
			datatmp[rdr_errloc[j] >> 3] ^= BIT(rdr_errloc[j] & 7);
	}
	spin_unlock(&rdr_bch_spinlock);
	if (num)
		BB_PN("bch check num: %d\n", num);

	return 0;
}
EXPORT_SYMBOL(rdr_bch_checkout);

int rdr_safemem_pool_show(struct rdr_safemem_pool *pool)
{
	if (IS_ERR_OR_NULL(pool) || pool->magic != SAFEMEM_POOL_MAGIC)
		return -EINVAL;
	BB_PN("pool name: %s\n", pool->name);
	BB_PN("maxnum: %d\n", pool->maxnum);
	BB_PN("curnum: %d\n", pool->curnum);
	BB_PN("low_to_high: %d\n", pool->low_to_high);
	BB_PN("base_alloc_addr: %px\n", pool->base_alloc_addr);
	BB_PN("end_alloc_addr: %px\n", pool->end_alloc_addr);
	BB_PN("cur_alloc_addr: %px\n", pool->cur_alloc_addr);
	BB_PN("pool_size: 0x%llx\n", pool->pool_size);
	return 0;
}

int rdr_safemem_pool_init(struct rdr_safemem_pool *pool, char *name, uint size,
			  bbox_mem *mem, bool low_to_high)
{
	size_t len = 0;

	if (IS_ERR_OR_NULL(pool) || IS_ERR_OR_NULL(name) || IS_ERR_OR_NULL(mem))
		return -EINVAL;

	if (size <
	    sizeof(struct rdr_safemem_pool) + 2 * sizeof(struct rdr_safemem))
		return -EINVAL;

	spin_lock_init(&pool->lock);
	len = strlen(name);
	len = len > (sizeof(pool->name) - 1) ? (sizeof(pool->name) - 1) : len;
	memcpy(pool->name, name, len);
	pool->name[len] = '\0';
	pool->base_alloc_addr = mem->vaddr;
	pool->phyaddr = mem->paddr;
	pool->pool_size = mem->size;
	pool->end_alloc_addr = mem->vaddr + mem->size;
	pool->curnum = 0;
	pool->low_to_high = low_to_high;
	pool->maxnum = ((size - sizeof(struct rdr_safemem_pool)) /
			sizeof(struct rdr_safemem)) -
		       1;
	pool->magic = SAFEMEM_POOL_MAGIC;
	if (pool->low_to_high)
		pool->cur_alloc_addr = pool->base_alloc_addr;
	else
		pool->cur_alloc_addr = pool->end_alloc_addr;

	memset(pool->mem, 0, sizeof(struct rdr_safemem) * pool->maxnum);
	return 0;
}
EXPORT_SYMBOL(rdr_safemem_pool_init);

int rdr_safemem_pool_reinit(struct rdr_safemem_pool *pool)
{
	if (IS_ERR_OR_NULL(pool))
		return -EINVAL;

	spin_lock_init(&pool->lock);
	return 0;
}
EXPORT_SYMBOL(rdr_safemem_pool_reinit);

int rdr_safemem_alloc(struct rdr_safemem_pool *pool, uint id, uint size,
		      bbox_mem *mem)
{
	uint index = 0;
	int check = 0;
	struct rdr_safemem *safemem;

	if (IS_ERR_OR_NULL(pool) || pool->magic != SAFEMEM_POOL_MAGIC ||
	    IS_ERR_OR_NULL(mem) || id == 0)
		return -EINVAL;

	spin_lock(&pool->lock);
	index = pool->curnum;
	if (index == pool->maxnum) {
		spin_unlock(&pool->lock);
		return -ENOMEM;
	}

	for (int i = 0; i < index; i++) {
		if (id == pool->mem[i].safe.id) {
			spin_unlock(&pool->lock);
			BB_ERR("%s: id %d is exist\n", pool->name, id);
			return -EINVAL;
		}
	}

	if (pool->low_to_high) {
		if (pool->cur_alloc_addr + size > pool->end_alloc_addr)
			check = -1;
	} else {
		if (pool->cur_alloc_addr - size < pool->base_alloc_addr)
			check = -1;
	}
	if (check) {
		spin_unlock(&pool->lock);
		BB_ERR("%s: id %d alloc fail\n", pool->name, id);
		return -ENOMEM;
	}

	pool->curnum++;
	safemem = &pool->mem[index];
	safemem->safe.id = id;
	safemem->safe.size = size;
	if (pool->low_to_high) {
		safemem->safe.vaddr = pool->cur_alloc_addr;
		pool->cur_alloc_addr += size;
	} else {
		pool->cur_alloc_addr -= size;
		safemem->safe.vaddr = pool->cur_alloc_addr;
	}
	safemem->safe.paddr =
		safemem->safe.vaddr - pool->base_alloc_addr + pool->phyaddr;
	spin_unlock(&pool->lock);

	check = rdr_bch_encode((void *)safemem->safe.rawdata,
			       sizeof(safemem->safe.rawdata), safemem->ecc,
			       sizeof(safemem->ecc));

	if (check) {
		spin_lock(&pool->lock);
		pool->curnum--;
		spin_unlock(&pool->lock);
		return -ENOMEM;
	}

	mem->paddr = safemem->safe.paddr;
	mem->size = safemem->safe.size;
	mem->vaddr = safemem->safe.vaddr;
	return 0;
}
EXPORT_SYMBOL(rdr_safemem_alloc);

int rdr_safemem_get(struct rdr_safemem_pool *pool, uint id, bbox_mem *mem)
{
	int index = 0, err;

	if (IS_ERR_OR_NULL(pool) || pool->magic != SAFEMEM_POOL_MAGIC ||
	    IS_ERR_OR_NULL(mem) || id == 0)
		return -EINVAL;

	spin_lock(&pool->lock);
	for (index = 0; index < pool->curnum; index++) {
		if (id == pool->mem[index].safe.id)
			break;
	}
	if (index == pool->curnum) {
		spin_unlock(&pool->lock);
		return -ENOMEM;
	}
	spin_unlock(&pool->lock);

	err = rdr_bch_checkout((void *)pool->mem[index].safe.rawdata,
			       sizeof(pool->mem[index].safe.rawdata),
			       pool->mem[index].ecc,
			       sizeof(pool->mem[index].ecc));
	if (err)
		return -EIO;

	mem->paddr = pool->mem[index].safe.paddr;
	mem->size = pool->mem[index].safe.size;
	mem->vaddr = pool->mem[index].safe.vaddr;
	return 0;
}
EXPORT_SYMBOL(rdr_safemem_get);

static int __init rdr_bch_init(void)
{
	struct rdr_safemem mem;

	compiletime_assert(
		sizeof(mem.safe) <= RDR_BCH_MAX_BYTES,
		"safe buff mast be lower than {RDR_BCH_MAX_BYTES}\n");
	rdr_bch = bch_init(RDR_BCH_M, RDR_BCH_T, 0, 0);
	if (IS_ERR_OR_NULL(rdr_bch))
		return -1;
	spin_lock_init(&rdr_bch_spinlock);
	return 0;
}
early_initcall(rdr_bch_init);
