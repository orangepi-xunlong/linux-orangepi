/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * SPDX-License-Identifier: GPL-2.0+
 * aw_rawnand_bbt.c
 *
 * (C) Copyright 2020 - 2021
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * cuizhikui <cuizhikui@allwinnertech.com>
 *
 */

#include <linux/mtd/aw-rawnand.h>
#include <linux/mtd/mtd.h>
#include <linux/slab.h>


/**
 * aw_rawnand_chip_check_badblock_bbt - check bad block from bbt
 * @mtd: MTD device structure
 * @block: block offset from device start block
 * @return: %BBT_B_BAD the block is bad, %BBT_B_GOOD the block is good,
 *		%BBT_B_INVALID the block in bbt can't to know bad or good
 * */
static int aw_rawnand_chip_check_badblock_bbt(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;

	int pos_byte = 0;
	int pos_bit = 0;

	pos_byte = block >> 3;
	pos_bit = block & 0x7;


	if (chip->bbtd[pos_byte] & (1 << pos_bit)) {
		if (chip->bbt[pos_byte] & (1 << pos_bit))
			ret = BBT_B_BAD;
		else
			ret = BBT_B_GOOD;
	} else
		ret = BBT_B_INVALID;

	return ret;
}

/**
 * aw_rawnand_chip_updata_bbt - update bbt&bbtd
 * @mtd : MTD device structure
 * @block: block offset device start
 * @flag: %BBT_B_GOOD mark the block is good in bbt,
 *	%BBT_B_BAD mark the block is bad in bbt
 * */
void aw_rawnand_chip_update_bbt(struct mtd_info *mtd, int block, int flag)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int pos_byte = 0;
	int pos_bit = 0;

	if (unlikely(!chip->bbt || !chip->bbtd)) {
		awrawnand_warn("bbt or bbtd is null, nothing to do\n");
		return;
	}

	pos_byte = block >> 3;
	pos_bit = block & 0x7;

	chip->bbtd[pos_byte] |= (1 << pos_bit);
	if (flag == BBT_B_GOOD)
		chip->bbt[pos_byte] &= ~(1 << pos_bit);
	else
		chip->bbt[pos_byte] |= (1 << pos_bit);

	return;
}
EXPORT_SYMBOL_GPL(aw_rawnand_chip_update_bbt);

static int aw_rawnand_chip_block_bad_check_first_page(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int ret = 0;
	int page = 0;
	uint8_t *spare;
	uint8_t *mdata;
	mdata = kzalloc(chip->pagesize,  GFP_KERNEL);
	if (mdata == NULL) {
		awrawnand_err("malloc buffer fail\n");
		goto out1;
	}
	spare = kzalloc(chip->avalid_sparesize,  GFP_KERNEL);
	if (spare == NULL) {
		awrawnand_err("malloc spare buffer fail\n");
		goto out2;
	}

	page = block << chip->pages_per_blk_shift;

	/*for used chip, maybe read whole page is better*/
	ret = chip->read_page(mtd, chip, mdata, chip->pagesize, spare, chip->avalid_sparesize, page);
	if (ret != ECC_ERR) {
		if (spare[0] != 0xFF) {
			ret = 1;
			awrawnand_info("page@%d oob:%02x %02x %02x %02x %02x %02x %02x %02x\n", page, spare[0],
					spare[1], spare[2], spare[3], spare[4], spare[5], spare[6], spare[7]);
			ret = BBT_B_BAD;
			goto out;
		}
		ret = BBT_B_GOOD;
	} else {
		ret = BBT_B_BAD;
	}

out:
	if (mdata)
		kfree(mdata);
	if (spare)
		kfree(spare);
	return ret;
out2:
	kfree(mdata);
out1:
	return -ENOMEM;
}

static int aw_rawnand_chip_block_bad_check_first_two_pages(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int ret = 0;
	int page = 0;
	uint8_t *spare;
	uint8_t *mdata;
	mdata = kzalloc(chip->pagesize,  GFP_KERNEL);
	if (mdata == NULL) {
		awrawnand_err("malloc buffer fail\n");
		goto out1;
	}
	spare = kzalloc(chip->avalid_sparesize,  GFP_KERNEL);
	if (spare == NULL) {
		awrawnand_err("malloc spare buffer fail\n");
		goto out2;
	}

	ret = aw_rawnand_chip_block_bad_check_first_page(mtd, block);
	if (ret == BBT_B_BAD)
		goto out;

	page = block << chip->pages_per_blk_shift;
	page++;
	/*for used chip, maybe read whole page is better*/
	ret = chip->read_page(mtd, chip, mdata, chip->pagesize, spare, chip->avalid_sparesize, page);
	if (ret != ECC_ERR) {
		if (spare[0] != 0xFF) {
			ret = 1;
			awrawnand_info("page@%d oob:%02x %02x %02x %02x %02x %02x %02x %02x\n", page, spare[0],
					spare[1], spare[2], spare[3], spare[4], spare[5], spare[6], spare[7]);
			ret = BBT_B_BAD;
			goto out;
		}
		ret = BBT_B_GOOD;
	} else {
		ret = BBT_B_BAD;
	}

out:
	if (mdata)
		kfree(mdata);
	if (spare)
		kfree(spare);
	return ret;
out2:
	kfree(mdata);
out1:
	return -ENOMEM;
}

static int aw_rawnand_chip_block_bad_check_last_page(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int ret = 0;
	int page = 0;
	uint8_t *spare;
	uint8_t *mdata;
	mdata = kzalloc(chip->pagesize,  GFP_KERNEL);
	if (mdata == NULL) {
		awrawnand_err("malloc buffer fail\n");
		goto out1;
	}
	spare = kzalloc(chip->avalid_sparesize,  GFP_KERNEL);
	if (spare == NULL) {
		awrawnand_err("malloc spare buffer fail\n");
		goto out2;
	}

	page = (block << chip->pages_per_blk_shift) + chip->pages_per_blk_mask;
	/*for used chip, maybe read whole page is better*/
	ret = chip->read_page(mtd, chip, mdata, chip->pagesize, spare, chip->avalid_sparesize, page);
	if (ret != ECC_ERR) {
		if (spare[0] != 0xFF) {
			ret = 1;
			awrawnand_info("page@%d oob:%02x %02x %02x %02x %02x %02x %02x %02x\n", page, spare[0],
					spare[1], spare[2], spare[3], spare[4], spare[5], spare[6], spare[7]);
			ret = BBT_B_BAD;
			goto out;
		}
		ret = BBT_B_GOOD;
	} else {
		ret = BBT_B_BAD;
	}

out:
	if (mdata)
		kfree(mdata);
	if (spare)
		kfree(spare);
	return ret;
out2:
	kfree(mdata);
out1:
	return -ENOMEM;
}

static int aw_rawnand_chip_block_bad_check_last_two_pages(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int ret = 0;
	int page = 0;
	uint8_t *spare;
	uint8_t *mdata;
	mdata = kzalloc(chip->pagesize,  GFP_KERNEL);
	if (mdata == NULL) {
		awrawnand_err("malloc buffer fail\n");
		goto out1;
	}
	spare = kzalloc(chip->avalid_sparesize,  GFP_KERNEL);
	if (spare == NULL) {
		awrawnand_err("malloc spare buffer fail\n");
		goto out2;
	}

	ret = aw_rawnand_chip_block_bad_check_last_page(mtd, block);
	if (ret == BBT_B_BAD)
		goto out;

	page = (block << chip->pages_per_blk_shift) + chip->pages_per_blk_mask;
	page--;
	/*for used chip, maybe read whole page is better*/
	ret = chip->read_page(mtd, chip, mdata, chip->pagesize, spare, chip->avalid_sparesize, page);
	if (ret != ECC_ERR) {
		if (spare[0] != 0xFF) {
			ret = 1;
			awrawnand_info("page@%d oob:%02x %02x %02x %02x %02x %02x %02x %02x\n", page, spare[0],
					spare[1], spare[2], spare[3], spare[4], spare[5], spare[6], spare[7]);
			ret = BBT_B_BAD;
			goto out;
		}
		ret = BBT_B_GOOD;
	} else {
		ret = BBT_B_BAD;
	}

out:
	if (mdata)
		kfree(mdata);
	if (spare)
		kfree(spare);
	return ret;
out2:
	kfree(mdata);
out1:
	return -ENOMEM;
}

static int aw_rawnand_chip_block_bad_check_first_and_last_pages(struct mtd_info *mtd, int block)
{
	int ret = 0;

	ret = aw_rawnand_chip_block_bad_check_first_page(mtd, block);
	if (ret == BBT_B_BAD)
		goto out;
	else
		ret = aw_rawnand_chip_block_bad_check_last_page(mtd, block);

out:
	return ret;
}

static int aw_rawnand_chip_block_bad_check_first_two_and_last_pages(struct mtd_info *mtd, int block)
{
	int ret = 0;

	ret = aw_rawnand_chip_block_bad_check_first_two_pages(mtd, block);
	if (ret == BBT_B_BAD)
		goto out;
	else
		ret = aw_rawnand_chip_block_bad_check_last_page(mtd, block);

out:
	return ret;
}
/**
 * aw_rawnand_chip_block_bad - read bad block marker from the chip
 * @mtd: MTD device structure
 * @block: block offset from device start block
 * */
int aw_rawnand_chip_block_bad(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int ret = 0;

	ret = aw_rawnand_chip_check_badblock_bbt(mtd, block);
	if (ret == BBT_B_GOOD) {
		awrawnand_dbg("block@%d bbt good\n", block);
		goto out;
	} else if (ret == BBT_B_BAD) {
		awrawnand_err("block@%d bbt bad\n", block);
		goto out;
	} else {
		awrawnand_dbg(
		"block @%d in bbt is invalid , need to read block mark flag\n", block);
	}


	if (chip->badblock_mark_pos == PST_FIRST_PAGE) {

		ret = aw_rawnand_chip_block_bad_check_first_page(mtd, block);

	} else if (chip->badblock_mark_pos == PST_FIRST_TWO_PAGES) {

		ret = aw_rawnand_chip_block_bad_check_first_two_pages(mtd, block);

	} else if (chip->badblock_mark_pos == PST_LAST_PAGE) {

		ret = aw_rawnand_chip_block_bad_check_last_page(mtd, block);

	} else if (chip->badblock_mark_pos == PST_LAST_TWO_PAGES) {

		ret = aw_rawnand_chip_block_bad_check_last_two_pages(mtd, block);

	} else if (chip->badblock_mark_pos == PST_FIRST_AND_LAST_PAGES) {

		ret = aw_rawnand_chip_block_bad_check_first_and_last_pages(mtd, block);

	} else if (chip->badblock_mark_pos == PST_FIRST_TWO_AND_LAST_PAGES) {

		ret = aw_rawnand_chip_block_bad_check_first_two_and_last_pages(mtd, block);
	} else {
		awrawnand_err("Unknow the block mark use default mark pos(first page)\n");
		goto out;
	}

	if (ret == BBT_B_BAD) {
		aw_rawnand_chip_update_bbt(mtd, block, BBT_B_BAD);
		ret = BBT_B_BAD;
	} else {
		aw_rawnand_chip_update_bbt(mtd, block, BBT_B_GOOD);
		ret = BBT_B_GOOD;
	}

out:

	return ret;
}
EXPORT_SYMBOL_GPL(aw_rawnand_chip_block_bad);

/**
 * aw_rawnand_chip_simu_block_bad - read bad block marker from the chip
 * @mtd: MTD device structure
 * @block: simu block offset from device start simu block
 * */
int aw_rawnand_chip_simu_block_bad(struct mtd_info *mtd, int block)
{
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
	int blockA = block << 1;
	int plane_offset = 1;
	int blockB = blockA + plane_offset;
	int ret = 0;

	ret = aw_rawnand_chip_block_bad(mtd, blockA);
	if (!ret)
		ret = aw_rawnand_chip_block_bad(mtd, blockB);

	return ret;
#else
	return aw_rawnand_chip_block_bad(mtd, block);
#endif
}
EXPORT_SYMBOL_GPL(aw_rawnand_chip_simu_block_bad);

/**
 * aw_rawnand_chip_block_markbad - mark a block bad in mark pos and update bbt&bbtd
 * @mtd: MTD device structure
 * @block: block offset from device start
 * */
int aw_rawnand_chip_block_markbad(struct mtd_info *mtd, int block)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	uint8_t *mdata = NULL;
	uint8_t *spare = NULL;
	int ret = 0;
	int page = 0;
	int bound = (chip->chips * chip->chipsize) >> chip->erase_shift;
	int pagesize = chip->pagesize;
	int sparesize = chip->avalid_sparesize;

	if (unlikely(block > bound)) {
		ret = -EINVAL;
		awrawnand_err("block@%d is exceed boundary@%d\n", block, bound);
		goto out;
	}

	page = block << chip->pages_per_blk_shift;

	mdata = kzalloc(pagesize,  GFP_KERNEL);
	if (!mdata) {
		awrawnand_err("kzalloc mdata fail\n");
		ret = -ENOMEM;
		goto out;
	}

	spare = kzalloc(sparesize, GFP_KERNEL);
	if (!spare) {
		awrawnand_err("kzalloc spare fail\n");
		ret = -ENOMEM;
		goto out;
	}

	ret = chip->erase(mtd, page);
	if (ret) {
		awrawnand_err("erase block@%d fail\n", block);
		ret = -EIO;
		goto out;
	}

	if (chip->badblock_mark_pos == PST_FIRST_PAGE) {

		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);

	} else if (chip->badblock_mark_pos == PST_LAST_PAGE) {

		page += chip->pages_per_blk_mask;
		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);

	} else if (chip->badblock_mark_pos == PST_FIRST_TWO_PAGES) {

		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);
		page++;
		ret |= chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);

	} else if (chip->badblock_mark_pos == PST_LAST_TWO_PAGES) {

		page += chip->pages_per_blk_mask;
		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);
		page--;
		ret |= chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);

	} else if (chip->badblock_mark_pos == PST_FIRST_AND_LAST_PAGES) {

		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);
		page += chip->pages_per_blk_mask;
		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);

	} else if (chip->badblock_mark_pos == PST_FIRST_TWO_AND_LAST_PAGES) {

		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);
		page++;
		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);
		page--;
		page += chip->pages_per_blk_mask;
		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);

	} else {
		awrawnand_err("Unknow the block mark use default mark pos(first page)\n");
		ret = chip->write_page(mtd, chip, mdata, pagesize, spare, sparesize, page);
	}
out:
	aw_rawnand_chip_update_bbt(mtd, block, BBT_B_BAD);

	if (mdata)
		kfree(mdata);
	if (spare)
		kfree(spare);

	return ret;
}
EXPORT_SYMBOL_GPL(aw_rawnand_chip_block_markbad);

/**
 * aw_rawnand_chip_simu_block_markbad - mark a simu block bad in mark pos and update bbt&bbtd
 * @mtd: MTD device structure
 * @block: simu block offset from device start simu block
 * */
int aw_rawnand_chip_simu_block_markbad(struct mtd_info *mtd, int block)
{
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
	int blockA = block << 1;
	int plane_offset = 1;
	int blockB = blockA + plane_offset;
	int ret = 0;

	ret = aw_rawnand_chip_block_markbad(mtd, blockA);
	ret |= aw_rawnand_chip_block_markbad(mtd, blockB);

	return ret;
#else
	return aw_rawnand_chip_block_markbad(mtd, block);
#endif
}
EXPORT_SYMBOL_GPL(aw_rawnand_chip_simu_block_markbad);

int aw_rawnand_chip_scan_bbt(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int total_blocks = chip->chipsize >> chip->erase_shift;

	int b = 0;
	int c = 0;
	int pos_byte = 0;
	int pos_bit = 0;

	for (c = 0; c < chip->chips; c++) {
		chip->select_chip(mtd, c);
		for (b = 0; b < total_blocks; b++) {
			pos_byte = b >> 3;
			pos_bit = b & 0x7;
			if (chip->block_bad(mtd, b)) {
				chip->block_markbad(mtd, b);
			} else {
				chip->bbt[pos_byte] &= ~(1 << pos_bit);
			}
			chip->bbtd[pos_byte] |= (1 << pos_bit);
		}
		chip->select_chip(mtd, -1);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(aw_rawnand_chip_scan_bbt);
