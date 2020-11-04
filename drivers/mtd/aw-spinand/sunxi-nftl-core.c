/*
 * sunxi-nftl-core.c for  sunxi spi nand base on nftl.
 *
 * Copyright (C) 2019 Allwinner.
 * SPDX-License-Identifier: GPL-2.0
 */

#define pr_fmt(fmt) "sunxi-spinand: " fmt

#include "sunxi-spinand.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/aw-spinand-nftl.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/sunxi-sid.h>
#include <linux/uaccess.h>

struct aw_spinand *g_spinand;

struct aw_spinand *get_aw_spinand(void)
{
	return g_spinand;
}

#define OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, pagenum)              \
	spinand_nftl_ops_flash_memory_legality_check(g_spinand, dienum,        \
			blocknum, pagenum)

static int spinand_nftl_ops_flash_memory_legality_check(
		struct aw_spinand *spinand, unsigned short dienum,
		unsigned short blocknum, unsigned short pagenum)
{
	unsigned int die_cnt = 0, block_cnt = 0, page_cnt = 0;

	if (!unlikely(spinand) || !unlikely(spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	die_cnt = spinand->chip.info->die_cnt(&spinand->chip);
	block_cnt = spinand->chip.info->total_size(&spinand->chip) >>
		spinand->block_shift;
	page_cnt = spinand->chip.info->block_size(&spinand->chip) >>
		spinand->page_shift;

	if (unlikely(dienum >= die_cnt)) {
		pr_err("nftl ops input dieno.%d/%d err\n", dienum, die_cnt);
		return -EOVERFLOW;
	}

	if (unlikely(blocknum >= block_cnt)) {
		pr_err("nftl ops input blockno.%d/%d err\n", blocknum,
				block_cnt);
		return -EOVERFLOW;
	}

	if (unlikely(pagenum >= page_cnt)) {
		pr_err("nftl ops input pageno.%d/%d err\n", pagenum, page_cnt);
		return -EOVERFLOW;
	}

	return 0;
}

#define OP_BUF_LEGALITY_CHECK(mbuf, spare)                                     \
	spinand_nftl_ops_buf_legality_check(mbuf, spare)
static int spinand_nftl_ops_buf_legality_check(void *mbuf, void *spare)
{
	if (!unlikely(mbuf) && !unlikely(spare)) {
		pr_err("nftl ops input mbuf and spare is null\n");
		return -EINVAL;
	}

	return 0;
}

#define NFTL_OPS_REQUEST(name, blocknum, pagenum, len, oobsize, mbuf, spare)   \
{                                                                      \
	(name).block = blocknum;                                       \
	(name).page = pagenum;                                         \
	(name).pageoff = 0;                                            \
	(name).ooblen = oobsize;                                       \
	(name).datalen = len;                                          \
	(name).databuf = mbuf;                                         \
	(name).oobbuf = spare;                                         \
}

/*
 * calc_valid_bits - calculate valid bit
 * @secbitmap: record valid bits
 *
 * Returns: valid bit count;
 */

unsigned int calc_valid_bits(unsigned int secbitmap)
{
	unsigned int validbit = 0;

	while (secbitmap) {
		if (secbitmap & 0x1)
			validbit++;
		secbitmap >>= 1;
	}

	return validbit;
}

/*
 * spinand_nftl_read_super_page - nftl layer to read data
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * @pagenum: which mean you want to operation that page in blocknum block,
 *	0 means the first page in one block
 * @sectorbitmap: operation page bitmap in sector
 * @rmbuf: read main data buf
 * @rspare: read spare data buf
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_read_super_page(unsigned short dienum, unsigned short blocknum,
		unsigned short pagenum, unsigned short sectorbitmap,
		void *rmbuf, void *rspare)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;
	struct aw_spinand_chip_request req = {0};
	unsigned int req_data_len = 0;

	pr_debug("calling nftl read page: die num:%d blocknum:%d pagenum:%d "
			"sectorbitmap:%d rbuf:%p rspare:%p\n",
			dienum, blocknum, pagenum, sectorbitmap, rmbuf, rspare);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, pagenum))
			|| unlikely(OP_BUF_LEGALITY_CHECK(rmbuf, rspare)))
		return -EINVAL;

	if (rmbuf == NULL && rspare != NULL)
		req_data_len = 0;

	req_data_len = calc_valid_bits(sectorbitmap) << SECTOR_SHIFT;
	NFTL_OPS_REQUEST(req, blocknum, pagenum, req_data_len, AW_NFTL_OOB_LEN,
			rmbuf, rspare);

	mutex_lock(&spinand->lock);

	ret = chip_ops->read_page(chip, &req);
	if (ret < 0)
		pr_err("read single page failed: %d\n", ret);
	else if (ret == ECC_LIMIT) {
		ret = AW_NFTL_ECC_LIMIT;
		pr_debug("ecc limit: block: %u page: %u\n", req.block,
				req.page);
	} else if (ret == ECC_ERR) {
		ret = AW_NFTL_ECC_ERR;
		pr_debug("ecc err: block: %u page: %u\n", req.block, req.page);
	}

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl read\n");

	return ret;
}

/*
 * spinand_nftl_write_super_page - nftl layer to write data
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * @pagenum: which mean you want to operation that page in blocknum block,
 *	0 means the first page in one block
 * @sectorbitmap: operation page bitmap in sector
 * @wmbuf: write main data buf
 * @wspare: write spare data buf
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_write_super_page(unsigned short dienum, unsigned short blocknum,
		unsigned short pagenum, unsigned short sectorbitmap,
		void *wmbuf, void *wspare)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;
	struct aw_spinand_chip_request req = {0};
	unsigned int write_mdata_len = 0;

	unsigned char *oob = NULL;


	pr_debug("calling nftl write page: die num:%d blocknum:%d pagenum:%d "
			"sectorbitmap:%d rbuf:%p wspare:%p\n",
			dienum, blocknum, pagenum, sectorbitmap, wmbuf, wspare);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, pagenum))
			|| unlikely(OP_BUF_LEGALITY_CHECK(wmbuf, wspare)))
		return -EINVAL;

	if (wmbuf == NULL && wspare != NULL)
		write_mdata_len = 0;

	if (wspare != NULL) {
		oob = kmalloc(AW_NFTL_OOB_LEN, GFP_KERNEL);
		if (oob == NULL) {
			pr_err("%s kmalloc oob failed!\n", __func__);
			return false;
		}
		/*---------------------------------------------------
		 * nftl oob |0xff-1-2-3-4-5-6-7-8-9-10-11-12-13-14-reserve|
		 *----------------------------------------------------
		 *super page|0xff-1-2-3-4-5-6-7-8-0xff-9-10-11-12-13-14|
		 *           ____________________ _____________________
		 *            physic pagen        physic pagem
		 *
		 * ---------------------------*/
		memcpy(oob, (unsigned char *)wspare, 8);
		oob[8] = 0xff;
		memcpy(oob+9, wspare+8, 7);
	}



	write_mdata_len = calc_valid_bits(sectorbitmap) << SECTOR_SHIFT;
	NFTL_OPS_REQUEST(req, blocknum, pagenum, write_mdata_len,
			AW_NFTL_OOB_LEN, wmbuf, oob);

	mutex_lock(&spinand->lock);

	ret = chip_ops->write_page(chip, &req);
	if (ret < 0)
		pr_err("write single page failed: %d\n", ret);
	else if (ret == ECC_LIMIT) {
		ret = AW_NFTL_ECC_LIMIT;
		pr_debug("ecc limit: block: %u page: %u\n", req.block,
				req.page);
	} else if (ret == ECC_ERR) {
		ret = AW_NFTL_ECC_ERR;
		pr_debug("ecc err: block: %u page: %u\n", req.block, req.page);
	}

	mutex_unlock(&spinand->lock);

	if (oob != NULL)
		kfree(oob);

	pr_debug("exitng nftl write block@%d page@%d\n", req.block, req.page);

	return ret;
}

/*
 * spinand_nftl_erase_super_block - nftl layer to erase block
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_erase_super_block(unsigned short dienum, unsigned short blocknum)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	struct aw_spinand_chip_request req = {0};

	pr_debug("calling nftl erase block: die num:%d blocknum:%d ", dienum,
			blocknum);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, 0)))
		return -EINVAL;

	NFTL_OPS_REQUEST(req, blocknum, 0, 0, 0, NULL, NULL);

	mutex_lock(&spinand->lock);

	ret = chip_ops->erase_block(chip, &req);
	if (ret < 0)
		pr_err("erase block@%d failed: %d\n", req.block, ret);

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl erase block@%d\n", req.block);

	return ret;
}

/*
 * spinand_nftl_super_badblock_check - nftl layer to erase block
 * @chipnum:  the block in chip'number
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_super_badblock_check(unsigned short chipnum, unsigned short blocknum)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	struct aw_spinand_chip_request req = {0};

	pr_debug("calling nftl check block is bad : chipnum num:%d blocknum:%d",
			chipnum, blocknum);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(0, blocknum, 0)))
		return -EINVAL;

	NFTL_OPS_REQUEST(req, blocknum, 0, 0, 0, NULL, NULL);

	mutex_lock(&spinand->lock);

	ret = chip_ops->is_bad(chip, &req);

	mutex_unlock(&spinand->lock);

	pr_debug("exit check bad block: block %u is %s\n",
			req.block, ret == true ? "bad" : "good");

	return ret;
}


/*
 * spinand_nftl_super_badblock_mark - nftl layer to erase block
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_super_badblock_mark(unsigned short dienum, unsigned short blocknum)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	struct aw_spinand_chip_request req = {0};

	pr_debug("calling nftl mark bad block : die num:%d blocknum:%d ", dienum,
			blocknum);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, 0)))
		return -EINVAL;

	NFTL_OPS_REQUEST(req, blocknum, 0, 0, 0, NULL, NULL);

	mutex_lock(&spinand->lock);

	ret = chip_ops->mark_bad(chip, &req);

	mutex_unlock(&spinand->lock);

	pr_debug("exit mark bad block: block %u\n", req.block);

	return ret;
}

/*
 * spinand_nftl_read_single_page - nftl layer to read data
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * @pagenum: which mean you want to operation that page in blocknum block,
 *	0 means the first page in one block
 * @sectorbitmap: operation page bitmap in sector
 * @rmbuf: read main data buf
 * @rspare: read spare data buf
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_read_single_page(unsigned short dienum, unsigned short blocknum,
		unsigned short pagenum, unsigned short sectorbitmap,
		void *rmbuf, void *rspare)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;
	struct aw_spinand_chip_request req = {0};
	unsigned int req_data_len = 0;

	pr_debug("calling nftl read page: die num:%d blocknum:%d pagenum:%d "
			"sectorbitmap:%d rbuf:%p rspare:%p\n",
			dienum, blocknum, pagenum, sectorbitmap, rmbuf, rspare);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, pagenum))
			|| unlikely(OP_BUF_LEGALITY_CHECK(rmbuf, rspare)))
		return -EINVAL;

	if (rmbuf == NULL && rspare != NULL)
		req_data_len = 0;

	req_data_len = calc_valid_bits(sectorbitmap) << SECTOR_SHIFT;
	NFTL_OPS_REQUEST(req, blocknum, pagenum, req_data_len, AW_NFTL_OOB_LEN,
			rmbuf, rspare);

	mutex_lock(&spinand->lock);

	ret = chip_ops->phy_read_page(chip, &req);
	if (ret < 0)
		pr_err("read single page failed: %d\n", ret);
	else if (ret == ECC_LIMIT) {
		ret = AW_NFTL_ECC_LIMIT;
		pr_debug("ecc limit: block: %u page: %u\n", req.block,
				req.page);
	} else if (ret == ECC_ERR) {
		ret = AW_NFTL_ECC_ERR;
		pr_debug("ecc err: block: %u page: %u\n", req.block, req.page);
	}

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl read\n");

	return ret;
}

/*
 * spinand_nftl_write_single_page - nftl layer to write data
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * @pagenum: which mean you want to operation that page in blocknum block,
 *	0 means the first page in one block
 * @sectorbitmap: operation page bitmap in sector
 * @wmbuf: write main data buf
 * @wspare: write spare data buf
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_write_single_page(unsigned short dienum, unsigned short blocknum,
		unsigned short pagenum, unsigned short sectorbitmap,
		void *wmbuf, void *wspare)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;
	struct aw_spinand_chip_request req = {0};
	unsigned int write_mdata_len = 0;

	pr_debug("calling nftl write page: die num:%d blocknum:%d pagenum:%d "
			"sectorbitmap:%d rbuf:%p wspare:%p\n",
			dienum, blocknum, pagenum, sectorbitmap, wmbuf, wspare);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, pagenum))
			|| unlikely(OP_BUF_LEGALITY_CHECK(wmbuf, wspare)))
		return -EINVAL;

	if (wmbuf == NULL && wspare != NULL)
		write_mdata_len = 0;

	write_mdata_len = calc_valid_bits(sectorbitmap) << SECTOR_SHIFT;
	NFTL_OPS_REQUEST(req, blocknum, pagenum, write_mdata_len,
			AW_NFTL_OOB_LEN, wmbuf, wspare);

	mutex_lock(&spinand->lock);

	ret = chip_ops->phy_write_page(chip, &req);
	if (ret < 0)
		pr_err("write single page failed: %d\n", ret);
	else if (ret == ECC_LIMIT) {
		ret = AW_NFTL_ECC_LIMIT;
		pr_debug("ecc limit: block: %u page: %u\n", req.block,
				req.page);
	} else if (ret == ECC_ERR) {
		ret = AW_NFTL_ECC_ERR;
		pr_debug("ecc err: block: %u page: %u\n", req.block, req.page);
	}

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl write block@%d page@%d\n", req.block, req.page);

	return ret;
}

/*
 * spinand_nftl_erase_single_block - nftl layer to erase block
 * @dienum: a chip maybe have multiple die,which mean you want to operate that
 *	die,0 means the first die
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_erase_single_block(unsigned short dienum, unsigned short blocknum)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	struct aw_spinand_chip_request req = {0};

	pr_debug("calling nftl write page: die num:%d blocknum:%d ", dienum,
			blocknum);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(dienum, blocknum, 0)))
		return -EINVAL;

	NFTL_OPS_REQUEST(req, blocknum, 0, 0, 0, NULL, NULL);

	mutex_lock(&spinand->lock);

	ret = chip_ops->phy_erase_block(chip, &req);
	if (ret < 0)
		pr_err("erase block@%d failed: %d\n", req.block, ret);

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl erase block@%d\n", req.block);

	return ret;
}

/*
 * spinand_nftl_single_badblock_check - nftl layer to check phy block is bad
 * @chipnum:  the physic block in which chip
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * Returns: true is bad block; false is good block
 */
int spinand_nftl_single_badblock_check(unsigned short chipnum, unsigned short blocknum)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	struct aw_spinand_chip_request req = {0};

	pr_debug("calling nftl check physic block is bad : chip num:%d blocknum"
			":%d ", chipnum, blocknum);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(0, blocknum, 0)))
		return -EINVAL;

	NFTL_OPS_REQUEST(req, blocknum, 0, 0, 0, NULL, NULL);

	mutex_lock(&spinand->lock);

	ret = chip_ops->phy_is_bad(chip, &req);

	mutex_unlock(&spinand->lock);

	pr_debug("exit check bad block: block %u is %s\n",
			req.block, ret == true ? "bad" : "good");

	return ret;
}

/*
 * spinand_nftl_single_badblock_mark - nftl layer to mark block
 * @chipnum: chip number
 * @blocknum: which mean you want to operation that block,0 means the first
 *	block
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_single_badblock_mark(unsigned short chipnum, unsigned short blocknum)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	struct aw_spinand_chip_request req = {0};

	pr_debug("calling nftl single badblock mark: chip num:%d blocknum:%d ",
			chipnum, blocknum);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(0, blocknum, 0)))
		return -EINVAL;

	NFTL_OPS_REQUEST(req, blocknum, 0, 0, 0, NULL, NULL);

	mutex_lock(&spinand->lock);

	ret = chip_ops->phy_mark_bad(chip, &req);
	if (ret < 0)
		pr_err("mark block@%d failed: %d\n", req.block, ret);

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl single badblock mark block@%d\n", req.block);

	return ret;
}


/*
 * spinand_nftl_single_block_copy - nftl layer to copy data between blocks
 * @from_chip: data src chip number
 * @from_block: data src block number
 * @to_chip: dst chip num
 * @to_block: dst block num
 * Returns: 0 success;otherwise fail
 */
int spinand_nftl_single_block_copy(unsigned int from_chip,
		unsigned int from_block, unsigned int to_chip,
		unsigned int to_block)
{

	int ret = 0;
	struct aw_spinand *spinand = g_spinand;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;


	pr_debug("calling nftl single block copy: from chip:%u from block:%u "
			"to chip:%u to block:%u",
			from_chip, from_block, to_chip, to_block);

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(0, from_block, 0)))
		return -EINVAL;

	if (unlikely(OP_FLASH_MEMORY_LEGALITY_CHECK(0, to_block, 0)))
		return -EINVAL;


	mutex_lock(&spinand->lock);

	ret = chip_ops->phy_copy_block(chip, from_block, to_block);
	if (ret < 0)
		pr_err("copy block@%u to block@%ufailed: %d\n",
				from_block, to_block);

	mutex_unlock(&spinand->lock);

	pr_debug("exitng nftl single block copy\n");

	return ret;
}

/**
 * spinand_nftl_get_super_page_size - nftl layer to get page size
 * @type: page size in sector or byte,0 in byte,1 sector
 *
 * Returns the size of page
 */
unsigned int spinand_nftl_get_super_page_size(int type)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	if (likely(type == SECTOR)) {
		return info->page_size(&g_spinand->chip) >>
			g_spinand->sector_shift;
	} else if (unlikely(type == BYTE)) {
		return info->page_size(&g_spinand->chip);
	} else {
		pr_err("no this type:%d super page size in BYTE(0)@byte "
				"SECTOR(1)@sector", type);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(spinand_nftl_get_super_page_size);
/**
 * spinand_nftl_get_super_block_size - nftl layer to get block size
 * @type: page size in sector or byte,0 in byte,1 sector,2 page
 *
 * Returns the size of block
 */
unsigned int spinand_nftl_get_super_block_size(int type)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	if (likely(type == PAGE)) {
		return info->block_size(&g_spinand->chip) >>
			g_spinand->page_shift;
	} else if (unlikely(type == SECTOR)) {
		return info->block_size(&g_spinand->chip) >>
			g_spinand->sector_shift;
	} else if (unlikely(type == BYTE)) {
		return info->block_size(&g_spinand->chip);
	} else {
		pr_err("no this type:%d super block size in BYTE(0)@byte "
				"SECTOR(1)@sector PAGE(2)@page", type);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(spinand_nftl_get_super_block_size);

/**
 * spinand_nftl_get_single_page_size - nftl layer to get page size
 * @type: page size in sector or byte,0 in byte,1 sector
 *
 * Returns the size of page
 */
unsigned int spinand_nftl_get_single_page_size(int type)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	if (likely(type == SECTOR)) {
		return info->phy_page_size(&g_spinand->chip) >>
			g_spinand->sector_shift;
	} else if (unlikely(type == BYTE)) {
		return info->phy_page_size(&g_spinand->chip);
	} else {
		pr_err("no this type:%d single page size in BYTE(0)@byte "
				"SECTOR(1)@sector", type);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(spinand_nftl_get_single_page_size);
/**
 * spinand_nftl_get_super_block_size - nftl layer to get block size
 * @type: page size in sector or byte,0 in byte,1 sector,2 page
 *
 * Returns the size of block
 */
unsigned int spinand_nftl_get_single_block_size(int type)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	if (likely(type == PAGE)) {
		return info->phy_block_size(&g_spinand->chip) >>
			g_spinand->page_shift;
	} else if (unlikely(type == SECTOR)) {
		return info->phy_block_size(&g_spinand->chip) >>
			g_spinand->sector_shift;
	} else if (unlikely(type == BYTE)) {
		return info->phy_block_size(&g_spinand->chip);
	} else {
		pr_err("no this type:%d single block size in BYTE(0)@byte "
				"SECTOR(1)@sector PAGE(2)@page", type);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(spinand_nftl_get_single_block_size);
/**
 * spinand_nftl_get_die_size - nftl layer to get die size
 * @type: page size in sector or byte,0 in byte,1 sector,2 page,3 block
 *
 * Returns the size of die
 */
unsigned int spinand_nftl_get_die_size(int type)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	if (likely(type == BLOCK))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->block_shift >> g_spinand->die_shift;
	else if (unlikely(type == BYTE))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->die_shift;
	else if (unlikely(type == SECTOR))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->sector_shift >> g_spinand->die_shift;
	else if (unlikely(type == PAGE))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->page_shift >> g_spinand->die_shift;
	else {
		pr_err("no this type:%d die size in BYTE(0)@byte "
			"SECTOR(1)@sector PAGE(2)@page BLOCK(3)@block", type);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(spinand_nftl_get_die_size);
/**
 * spinand_nftl_get_chip_size - nftl layer to get die size
 * @type: page size in sector or byte,0 in byte,1 sector,2 page,3 block
 *
 * Returns the size of chip
 */
unsigned int spinand_nftl_get_chip_size(int type)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	if (likely(type == BLOCK))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->block_shift;
	else if (unlikely(type == BYTE))
		return info->total_size(&g_spinand->chip);
	else if (unlikely(type == SECTOR))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->sector_shift;
	else if (unlikely(type == PAGE))
		return info->total_size(&g_spinand->chip) >>
			g_spinand->page_shift;
	else {
		pr_err("no this type:%d die size in BYTE(0)@byte "
			"SECTOR(1)@sector PAGE(2)@page BLOCK(3)@block", type);
		return -EINVAL;
	}
}
EXPORT_SYMBOL(spinand_nftl_get_chip_size);

#define DIE_NUM_IN_OTHER_CHIP (0)
/**
 * spinand_nftl_get_die_cnt - nftl layer to get die count
 *
 * Returns the count of die in a chip
 */
unsigned int spinand_nftl_get_die_cnt(void)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	return info->die_cnt(&g_spinand->chip) + DIE_NUM_IN_OTHER_CHIP;
}
EXPORT_SYMBOL(spinand_nftl_get_die_cnt);

/**
 * spinand_nftl_get_chip_cnt - nftl layer to get chip count
 *
 * Returns the count of chip
 */
unsigned int spinand_nftl_get_chip_cnt(void)
{
	/*now only support one chip*/
	return 1;
}
EXPORT_SYMBOL(spinand_nftl_get_chip_cnt);
/**
 * spinand_nftl_get_max_erase_times - nftl layer to get chip endurace
 *
 * Returns the program/erase cycles
 */
unsigned int spinand_nftl_get_max_erase_times(void)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	return info->max_erase_times(&g_spinand->chip);
}
EXPORT_SYMBOL(spinand_nftl_get_max_erase_times);
/**
 * @spinand_nftl_get_multi_plane_flag - nftl layer to get flash is support multi
 *	plane
 *
 * @Returns 1 support , 0 not support
 */
unsigned int spinand_nftl_get_multi_plane_flag(void)
{
	return SIMULATE_MULTIPLANE;
}
EXPORT_SYMBOL(spinand_nftl_get_multi_plane_flag);

/**
 * spinand_nftl_get_operation_opt
 *
 * Returns the flash operation_opt
 */
unsigned int spinand_nftl_get_operation_opt(void)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	return info->operation_opt(&(g_spinand->chip));
}
EXPORT_SYMBOL(spinand_nftl_get_operation_opt);


/**
 * spinand_nftl_get_chip_id
 *
 * Returns
 */
void spinand_nftl_get_chip_id(unsigned char *id)
{
	struct aw_spinand_info *info = NULL;

	if (!unlikely(g_spinand) || !unlikely(g_spinand->chip.info)) {
		pr_err("nftl spinand driver not init\n");
		return false;
	}

	info = g_spinand->chip.info;

	info->nandid(&(g_spinand->chip), id, MAX_ID_LEN);
}
static void aw_spinand_cleanup(struct aw_spinand *spinand)
{
	aw_spinand_chip_exit(&spinand->chip);
}

static int aw_nftl_spinand_remove(struct spi_device *spi)
{
	struct aw_spinand *spinand;

	spinand = spi_to_spinand(spi);

	aw_spinand_cleanup(spinand);

	return 0;
}

static int aw_nftl_spinand_probe(struct spi_device *spi)
{
	struct aw_spinand *spinand;
	struct aw_spinand_chip *chip;
	int ret;

	if (g_spinand) {
		pr_info("AW nftl-spinand already initialized\n");
		return -EBUSY;
	}

	pr_info("AW SPINand NFTL Layer Version: %x.%x %x\n",
			AW_NFTL_SPINAND_VER_MAIN, AW_NFTL_SPINAND_VER_SUB,
			AW_NFTL_SPINAND_VER_DATE);

	spinand = devm_kzalloc(&spi->dev, sizeof(*spinand), GFP_KERNEL);
	if (!spinand)
		return -ENOMEM;
	chip = spinand_to_chip(spinand);
	mutex_init(&spinand->lock);

	ret = aw_spinand_chip_init(spi, chip);
	if (ret)
		goto err_spinand_cleanup;

	spinand->sector_shift = ffs(chip->info->sector_size(chip)) - 1;
	spinand->page_shift = ffs(chip->info->page_size(chip)) - 1;
	spinand->block_shift = ffs(chip->info->block_size(chip)) - 1;
	spinand->die_shift = ffs(chip->info->die_cnt(chip)) - 1;

	g_spinand = spinand;

	return 0;

err_spinand_cleanup:
	devm_kfree(&spi->dev, spinand);
	return ret;
}

static const struct spi_device_id aw_spinand_ids[] = {
	{.name = "spi-nand"},
	{/* sentinel */},
};

static const struct of_device_id aw_spinand_of_ids[] = {
	{.compatible = "spi-nand"},
	{/* sentinel */},
};

static struct spi_driver aw_spinand_drv = {
	.driver = {
		.name = "spi-nand",
		.of_match_table = of_match_ptr(aw_spinand_of_ids),
	},
	.id_table = aw_spinand_ids,
	.probe = aw_nftl_spinand_probe,
	.remove = aw_nftl_spinand_remove,
};
module_spi_driver(aw_spinand_drv);

MODULE_AUTHOR("cuizhikui <cuizhikui@allwinnertech.com>");
MODULE_DESCRIPTION(
		"Allwinner's spinand driver base on allwinner's nftl system");
