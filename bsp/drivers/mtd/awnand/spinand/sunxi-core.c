// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

//#define pr_fmt(fmt) "sunxi-spinand: " fmt
#define SUNXI_MODNAME "sunxi-spinand"
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/mtd.h>
#include <linux/uaccess.h>
#include <sunxi-sid.h>
#include "sunxi-spinand.h"

struct aw_spinand *g_spinand;

static int ubootblks = -1;
module_param(ubootblks, int, 0400);
MODULE_PARM_DESC(ubootblks, "block count for uboot");

#if IS_ENABLED(CONFIG_MTD_CMDLINE_PARTS)
static int get_para_from_cmdline(const char *cmdline, const char *name)
{
	if (!cmdline || !name)
		return -1;

	for (; *cmdline != 0;) {
		if (*cmdline++ == ' ') {
			if (0 == strncmp(cmdline, name, strlen(name))) {
				printk("cmdline:%s \n", cmdline);
				return 1;
			}
		}
	}

	return 0;
}
#endif

struct aw_spinand *get_aw_spinand(void)
{
	return g_spinand;
}

struct mtd_info *get_aw_spinand_mtd(void)
{
	return &g_spinand->mtd;
}
EXPORT_SYMBOL(get_aw_spinand_mtd);

static void aw_spinand_cleanup(struct aw_spinand *spinand)
{
	aw_spinand_chip_exit(&spinand->chip);
}

static int aw_spinand_erase(struct mtd_info *mtd, struct erase_info *einfo)
{
	unsigned int len, block_size;
	int ret;
	struct aw_spinand_chip_request req = {0};
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;

	sunxi_debug(NULL, "calling erase\n");

	len = (unsigned int)einfo->len;
	ret = addr_to_req(chip, &req, einfo->addr);
	if (ret)
		return ret;

	/*
	 * For historical reasons
	 * The SYS partition uses a superblock layout,
	 * while the other physical partitions do not.
	 * Added judgment to enable MTD devices to access the entire Flash
	 */
	if (einfo->addr >= get_sys_part_offset())
		block_size = info->block_size(chip);
	else
		block_size = info->phy_block_size(chip);

	while (len) {
		mutex_lock(&spinand->lock);

		if (einfo->addr >= get_sys_part_offset())
			ret = ops->erase_block(chip, &req);
		else
			ret = ops->phy_erase_block(chip, &req);

		mutex_unlock(&spinand->lock);

		if (ret) {
			einfo->fail_addr = einfo->addr;
			sunxi_err(NULL, "erase block %u in addr 0x%x failed: %d\n",
					req.block, (unsigned int)einfo->addr,
					ret);
			return ret;
		}

		req.block++;
		len -= block_size;
	}

	return 0;
}

static void aw_spinand_init_mtd_info(struct aw_spinand_chip *chip,
		struct mtd_info *mtd)
{
	struct aw_spinand_info *info = chip->info;

	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->owner = THIS_MODULE;
	mtd->erasesize = info->block_size(chip);
	mtd->writesize = info->page_size(chip);
	mtd->oobsize = info->oob_size(chip);
#if IS_ENABLED(CONFIG_AW_SPINAND_SIMULATE_MULTIPLANE)
	mtd->oobavail = AW_OOB_SIZE_PER_PHY_PAGE * 2;
	mtd->subpage_sft = 1;
#else
	mtd->oobavail = AW_OOB_SIZE_PER_PHY_PAGE;
	mtd->subpage_sft = 0;
#endif

#if IS_ENABLED(CONFIG_AW_SPINAND_OOB_RAW_SPARE)
	mtd->oobavail = mtd->oobsize;
#endif
	mtd->writebufsize = info->page_size(chip);
	mtd->size = info->total_size(chip);
	mtd->name = "nand";
	mtd->ecc_strength = ECC_ERR;
	mtd->bitflip_threshold = ECC_LIMIT;
}

static inline void aw_spinand_req_init(struct aw_spinand *spinand,
		loff_t offs, struct mtd_oob_ops *mtd_ops,
		struct aw_spinand_chip_request *req)
{
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	struct mtd_info *mtd = spinand_to_mtd(spinand);

	addr_to_req(chip, req, offs);
	req->databuf = mtd_ops->datbuf;
	req->oobbuf = mtd_ops->oobbuf;

	/*
	 * For historical reasons
	 * The SYS partition uses a superblock layout,
	 * while the other physical partitions do not.
	 * Added judgment to enable MTD devices to access the entire Flash
	 */
	if (offs >= get_sys_part_offset()) {
		req->pageoff = offs & (info->page_size(chip) - 1);
		req->datalen = min(info->page_size(chip) - req->pageoff,
				(unsigned int)(mtd_ops->len));
	} else {
		req->pageoff = offs & (info->phy_page_size(chip) - 1);
		req->datalen = min(info->phy_page_size(chip) - req->pageoff,
				(unsigned int)(mtd_ops->len));
	}

	/* read size once */
	req->ooblen = min_t(unsigned int, mtd_ops->ooblen, mtd->oobavail);

	/* the total size to read */
	req->dataleft = mtd_ops->len;
	req->oobleft = mtd_ops->ooblen;
}

static inline void aw_spinand_req_next(struct aw_spinand *spinand,
		loff_t offs, struct aw_spinand_chip_request *req)
{
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	unsigned int pages_per_blk;
	const char *str;

	/*
	 * For historical reasons
	 * The SYS partition uses a superblock layout,
	 * while the other physical partitions do not.
	 * Added judgment to enable MTD devices to access the entire Flash
	 */
	if (offs >= get_sys_part_offset())
		pages_per_blk = info->block_size(chip) >>
			spinand->page_shift;
	else
		pages_per_blk = info->phy_block_size(chip) >>
			spinand->phy_page_shift;

	req->page++;
	if (req->page >= pages_per_blk) {
		req->page = 0;
		req->block++;
	}

	if (req->dataleft < req->datalen) {
		str = "[phy]request: dataleft < datalen";
		goto bug;
	}

	if (req->oobleft < req->ooblen) {
		str = "[phy]request: oobleft < ooblen";
		goto bug;
	}

	req->dataleft -= req->datalen;
	req->databuf += req->datalen;
	req->oobleft -= req->ooblen;
	req->oobbuf += req->ooblen;
	req->pageoff = 0;

	if (offs >= get_sys_part_offset())
		req->datalen = min(info->page_size(chip), req->dataleft);
	else
		req->datalen = min(info->phy_page_size(chip), req->dataleft);

	req->ooblen = min_t(unsigned int, req->oobleft, mtd->oobavail);
	return;

bug:
	aw_spinand_reqdump(pr_err, str, req);
	WARN_ON(1);
	return;
}

static inline bool aw_spinand_req_end(struct aw_spinand *spinand,
		struct aw_spinand_chip_request *req)
{
	if (req->dataleft || req->oobleft)
		return false;
	return true;
}

/**
 * aw_spinand_for_each_req - Iterate over all NAND pages contained in an
 *				MTD I/O request
 * @spinand: SPINAND device
 * @start: start address to read/write from
 * @mtd_ops: MTD I/O request
 * @req: sunxi chip request
 *
 * Should be used for iterate over pages that are contained in an MTD request
 */
#define aw_spinand_for_each_req(spinand, start, mtd_ops, req)	\
	for (aw_spinand_req_init(spinand, start, mtd_ops, req);	\
		!aw_spinand_req_end(spinand, req);			\
		aw_spinand_req_next(spinand, start, req))

static int aw_spinand_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	int ret = 0;
	unsigned int max_bitflips = 0;
	struct aw_spinand_chip_request req = {0};
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;
	bool ecc_failed = false;

	if (from < 0 || from >= mtd->size || ops->len > mtd->size - from)
		return -EINVAL;
	if (!ops->len && !ops->ooblen)
		return 0;

	mutex_lock(&spinand->lock);

	sunxi_debug(NULL, "calling read with oob: from 0x%llx datalen %lu ooblen %lu\n",
			from, (unsigned long)ops->len, (unsigned long)ops->ooblen);

	aw_spinand_for_each_req(spinand, from, ops, &req) {
		aw_spinand_reqdump(pr_debug, "do super read", &req);

		if (from >= get_sys_part_offset())
			ret = chip_ops->read_page(chip, &req);
		else
			ret = chip_ops->phy_read_page(chip, &req);

		if (ret < 0) {
			sunxi_err(NULL, "read single page failed: %d\n", ret);
			break;
		} else if (ret == ECC_LIMIT) {
			ret = mtd->bitflip_threshold;
			mtd->ecc_stats.corrected += ret;
			max_bitflips = max_t(unsigned int, max_bitflips, ret);
			sunxi_debug(NULL, "ecc limit: block: %u page: %u\n",
					req.block, req.page);
		} else if (ret == ECC_ERR) {
			ecc_failed = true;
			mtd->ecc_stats.failed++;
			sunxi_err(NULL, "ecc err: block: %u page: %u\n",
					req.block, req.page);
		}

		ret = 0;
		ops->retlen += req.datalen;
		ops->oobretlen += req.ooblen;
	}
	mutex_unlock(&spinand->lock);

	if (ecc_failed && !ret)
		ret = -EBADMSG;

	sunxi_debug(NULL, "exitng read with oob\n");
	return ret ? ret : max_bitflips;
}

static int aw_spinand_read(struct mtd_info *mtd,
	loff_t from, size_t len, size_t *retlen, uint8_t *buf)
{
	struct mtd_oob_ops ops = {0};
	int ret;

	sunxi_debug(NULL, "calling read\n");

	ops.len = len;
	ops.datbuf = buf;
	ops.oobbuf = NULL;
	ops.ooblen = ops.ooboffs = ops.oobretlen = 0;
	ret = aw_spinand_read_oob(mtd, from, &ops);
	*retlen = ops.retlen;
	sunxi_debug(NULL, "exiting read\n");
	return ret;
}

static int aw_spinand_write_oob(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
	int ret = 0;
	struct aw_spinand_chip_request req = {0};
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	if (to < 0 || to >= mtd->size || ops->len > mtd->size - to)
		return -EOVERFLOW;
	if (!ops->len && !ops->ooblen)
		return 0;
	/*
	 * if enable CONFIG_AW_SPINAND_SIMULATE_MULTIPLANE, will eanble
	 * subpage too. This means that ubi will write header to physic block
	 * page. So, we should check alignment for physic page rather super page.
	 */
	if (ops->len & (info->phy_page_size(chip) - 1))
		return -EINVAL;

	mutex_lock(&spinand->lock);

	sunxi_debug(NULL, "calling write with oob: to 0x%llx datalen %lu ooblen %lu\n",
			to, (unsigned long)ops->len, (unsigned long)ops->ooblen);

	aw_spinand_for_each_req(spinand, to, ops, &req) {
		aw_spinand_reqdump(pr_debug, "do super write", &req);

		if (to >= get_sys_part_offset())
			ret = chip_ops->write_page(chip, &req);
		else
			ret = chip_ops->phy_write_page(chip, &req);

		if (ret < 0) {
			sunxi_err(NULL, "write single page failed: block %d, page %d, ret %d\n",
					req.block, req.page, ret);
			break;
		}

		ops->retlen += req.datalen;
		ops->oobretlen += req.ooblen;
	}
	mutex_unlock(&spinand->lock);

	sunxi_debug(NULL, "exiting write with oob\n");
	return ret;
}

static int aw_spinand_write(struct mtd_info *mtd, loff_t to,
	size_t len, size_t *retlen, const u_char *buf)
{
	struct mtd_oob_ops ops = {0};
	int ret;

	sunxi_debug(NULL, "calling write\n");

	ops.len = len;
	ops.datbuf = (uint8_t *)buf;
	ops.oobbuf = NULL;
	ops.ooblen = ops.ooboffs = ops.oobretlen = 0;
	ret = aw_spinand_write_oob(mtd, to, &ops);
	*retlen = ops.retlen;

	sunxi_debug(NULL, "exiting write\n");
	return ret;
}

static int aw_spinand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	int ret;
	struct aw_spinand_chip_request req = {0};
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;

	sunxi_debug(NULL, "calling isbad\n");

	ret = addr_to_req(chip, &req, offs);
	if (ret)
		return ret;

	mutex_lock(&spinand->lock);

	if (offs >= get_sys_part_offset())
		ret = ops->is_bad(chip, &req);
	else
		ret = ops->phy_is_bad(chip, &req);

	mutex_unlock(&spinand->lock);
	sunxi_debug(NULL, "exting isbad: block %u is %s\n", req.block,
			ret == true ? "bad" : "good");
	return ret;
}

static int aw_spinand_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	int ret;
	struct aw_spinand_chip_request req = {0};
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;

	sunxi_debug(NULL, "calling markbad\n");

	ret = addr_to_req(chip, &req, offs);
	if (ret)
		return ret;

	mutex_lock(&spinand->lock);

	if (offs >= get_sys_part_offset())
		ret = ops->mark_bad(chip, &req);
	else
		ret = ops->phy_mark_bad(chip, &req);

	mutex_unlock(&spinand->lock);
	sunxi_info(NULL, "exting markbad: mark block %d as bad\n", req.block);
	return ret;
}

#if IS_ENABLED(CONFIG_AW_SPINAND_PSTORE_PANIC_WRITE)
/* write when on panic */
static int aw_spinand_panic_write(struct mtd_info *mtd, loff_t to,
		size_t len, size_t *retlen, const u_char *buf)
{
	int ret = 0;
	struct aw_spinand_chip_request req = {0};
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_chip_ops *chip_ops = chip->ops;
	struct mtd_oob_ops ops = {0};

	sunxi_info(NULL, "calling panic write: to 0x%llx datalen %lu\n", to,
			(unsigned long)len);


	if (to < 0 || to >= mtd->size || len > mtd->size - to)
		return -EOVERFLOW;

	if (!len)
		return 0;
	/*
	 * if enable CONFIG_AW_SPINAND_SIMULATE_MULTIPLANE, will eanble
	 * subpage too. This means that ubi will write header to physic block
	 * page. So, we should check alignment for physic page rather super page.
	 */
	if (len & (info->phy_page_size(chip) - 1))
		return -EINVAL;

	mutex_lock(&spinand->lock);

	ops.len = len;
	ops.datbuf = (uint8_t *)buf;
	ops.oobbuf = NULL;
	ops.ooblen = ops.ooboffs = ops.oobretlen = 0;

	aw_spinand_for_each_req(spinand, to, &ops, &req) {
		aw_spinand_reqdump(pr_debug, "do super panic write", &req);

		req.type = AW_SPINAND_MTD_REQ_PANIC;

		ret = chip_ops->write_page(chip, &req);
		if (ret < 0) {
			sunxi_err(NULL, "panic write single page failed: block %d, page %d, ret %d\n",
					req.block, req.page, ret);
			break;
		}

		ops.retlen += req.datalen;
		ops.oobretlen += req.ooblen;
	}

	*retlen = ops.retlen;
	mutex_unlock(&spinand->lock);

	sunxi_info(NULL, "exiting panic write\n");
	return ret;
}
#endif

/* mtd resume handler */
static void aw_spinand_resume(struct mtd_info *mtd)
{
	int ret;
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);

	/* update spinand register */
	ret = aw_spinand_chip_update_cfg(chip);
	if (ret)
		sunxi_err(NULL, "spinand resume() failed\n");
}

static int aw_spinand_suspend(struct mtd_info *mtd)
{
	int ret;
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *chip_ops = chip->ops;

	ret = chip_ops->reset(chip);
	if (ret) {
		sunxi_err(NULL, "spinand suspend() failed\n");
		return ret;
	}

    return 0;
}

static int aw_spinand_mtd_init(struct aw_spinand *spinand)
{
	struct mtd_info *mtd = &spinand->mtd;
	struct aw_spinand_chip *chip = &spinand->chip;
	struct spi_device *spi = chip->spi;

	spi_set_drvdata(spi, spinand);
	mtd = spinand_to_mtd(spinand);
	mtd_set_of_node(mtd, spi->dev.of_node);
	mtd->dev.parent = &spi->dev;

	aw_spinand_init_mtd_info(chip, mtd);

	mtd->_erase = aw_spinand_erase;
	mtd->_read = aw_spinand_read;
	mtd->_read_oob = aw_spinand_read_oob;
	mtd->_write = aw_spinand_write;
	mtd->_write_oob = aw_spinand_write_oob;
	mtd->_block_isbad = aw_spinand_block_isbad;
	mtd->_block_markbad = aw_spinand_block_markbad;
#if IS_ENABLED(CONFIG_AW_SPINAND_PSTORE_PANIC_WRITE)
	mtd->_panic_write = aw_spinand_panic_write;
#endif

	mtd->_resume = aw_spinand_resume;
	mtd->_suspend = aw_spinand_suspend;

	return 0;
}

void aw_spinand_uboot_blknum(struct aw_spinand *spinand,
		unsigned int *start, unsigned int *end)
{
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	unsigned int blksize = info->phy_block_size(chip);
	unsigned int pagecnt = blksize / info->phy_page_size(chip);
	unsigned int _start, _end;

	/* small nand:block size < 1MB;  reserve 4M for uboot */
	if (blksize <= SZ_128K) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 32;
	} else if (blksize <= SZ_256K) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 16;
	} else if (blksize <= SZ_512K) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 8;
	} else if (blksize <= SZ_1M && pagecnt <= 128) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 4;
	/* big nand;  reserve at least 20M for uboot */
	} else if (blksize <= SZ_1M && pagecnt > 128) {
		_start = UBOOT_START_BLOCK_BIGNAND;
		_end = _start + 20;
	} else if (blksize <= SZ_2M) {
		_start = UBOOT_START_BLOCK_BIGNAND;
		_end = _start + 10;
	} else {
		_start = UBOOT_START_BLOCK_BIGNAND;
		_end = _start + 8;
	}

	if (ubootblks > 0)
		_end = _start + ubootblks;
	else
		/* update parameter to /sys */
		ubootblks = _end - _start;

	if (start)
		*start = _start;
	if (end)
		*end = _end;
}

int aw_spinand_fill_phy_info(struct aw_spinand_chip *chip,
		void *data);

/*
 * this function is used to fix for aw boot0
 * we do not make a new way for ubi on boot0, to fix for the old way, we
 * have to fill boot_spinand_para_t for boot0 header
 */
int aw_spinand_mtd_get_flash_info(struct aw_spinand *spinand,
		void *data, unsigned int len)
{
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	boot_spinand_para_t *boot_info = data;
	unsigned int uboot_start, uboot_end;

	aw_spinand_uboot_blknum(spinand, &uboot_start, &uboot_end);

	aw_spinand_fill_phy_info(chip, data);

	boot_info->uboot_start_block = uboot_start;
	boot_info->uboot_next_block = uboot_end;
	boot_info->logic_start_block = uboot_end;
	boot_info->nand_specialinfo_page = uboot_end;
	boot_info->nand_specialinfo_offset = uboot_end;
	boot_info->physic_block_reserved = 0;

	return 0;
}

/**
 * do download boot data to flash
 *
 * @startblk: the block to downlaod
 * @endblk: the end block to downlaod [start, end)
 * @pagesize: data size to write to each page, 0 means the whole page
 * @buf: data buffer
 * @len: length of buf
 *
 * return the blocks count written including the bad blocks.
 * return negative number if error.
 */
static int download_boot(struct mtd_info *mtd,
		unsigned int startblk, unsigned int endblk,
		unsigned int pagesize, void *buf, unsigned int len)
{
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_chip_request req = {0};
	unsigned int blksize = info->phy_block_size(chip);
	unsigned int pagecnt = blksize / info->phy_page_size(chip);
	unsigned int written_blks = 0;
	int ret, pgindex = 0;

	/* check boundary */
	pagesize = pagesize ? pagesize : info->phy_page_size(chip);
	if (pagesize > info->phy_page_size(chip)) {
		pagesize = info->phy_page_size(chip);
		sunxi_warn(NULL, "reset download boot pagesize to %d\n", pagesize);
	}
	/* download */
	req.block = startblk;
	req.page = 0;
	do {
		if (req.page == 0) {
			/* must check bad and do erase for new block */
			mutex_lock(&spinand->lock);
			ret = ops->phy_is_bad(chip, &req);
			mutex_unlock(&spinand->lock);
			if (ret == true) {
				sunxi_info(NULL, "skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				written_blks++;
				if (req.block >= endblk) {
					sunxi_info(NULL, "achieve maximum blocks %d\n",
							endblk);
					return written_blks;
				}
				/* continue to check bad blk before erase */
				continue;
			}
			mutex_lock(&spinand->lock);
			ret = ops->phy_erase_block(chip, &req);
			mutex_unlock(&spinand->lock);
			if (ret) {
				sunxi_err(NULL, "erase blk %d failed\n", req.block);
				return ret;
			}
		}

		req.datalen = min(len, pagesize);
		/*
		 * only on the last time, datalen may be less than pagesize,
		 * to calculate the offset, we should use pagesize * pgindex
		 */
		req.databuf = buf + pgindex * pagesize;

		mutex_lock(&spinand->lock);
		ret = ops->phy_write_page(chip, &req);
		mutex_unlock(&spinand->lock);
		if (ret) {
			sunxi_err(NULL, "write boot to blk %d page %d failed\n",
					req.block, req.page);
			return ret;
		}
		if (req.page == 0)
			written_blks++;

		pgindex++;
		len -= req.datalen;
		req.page++;
		if (req.page >= pagecnt) {
			req.page = 0;
			req.block++;
		}
	} while (len > 0);

	return written_blks;
}

static int gen_check_sum(void *boot_buf)
{
	standard_boot_file_head_t *head_p;
	unsigned int length;
	unsigned int *buf;
	unsigned int loop;
	unsigned int i;
	unsigned int sum;
	unsigned int *p;
	toc0_private_head_t *toc0_head;

	if (sunxi_soc_is_secure()) {
		/* secure */
		toc0_head = (toc0_private_head_t *) boot_buf;
		length = toc0_head->length;
		p = &(toc0_head->check_sum);
	} else {
		head_p = (standard_boot_file_head_t *) boot_buf;
		length = head_p->length;
		p = &(head_p->check_sum);
	}

	if ((length & 0x3) != 0)	/* must 4-byte-aligned */
		return -1;

	buf = (unsigned int *)boot_buf;
	*p = STAMP_VALUE;	/* fill stamp */
	loop = length >> 2;

	for (i = 0, sum = 0; i < loop; i++)
		sum += buf[i];

	*p = sum;
	return 0;
}

static int get_nand_para(struct aw_spinand *spinand, void *boot_buf)
{
	boot0_file_head_t *boot0_buf;
	char *data_buf;
	void *nand_para;
	sbrom_toc0_config_t *secure_toc0_buf;

	if (sunxi_soc_is_secure()) {
		/* secure */
		secure_toc0_buf =
		    (sbrom_toc0_config_t *) (boot_buf + SBROM_TOC0_HEAD_SPACE);
		data_buf = secure_toc0_buf->storage_data;
		nand_para = (void *)data_buf;
	} else {
		/* nonsecure */
		boot0_buf = (boot0_file_head_t *) boot_buf;
		data_buf = boot0_buf->prvt_head.storage_data;
		nand_para = (void *)data_buf;
	}

	aw_spinand_mtd_get_flash_info(spinand, nand_para, STORAGE_BUFFER_SIZE);

	return 0;
}

#if IS_ENABLED(CONFIG_AW_SPINAND_SECURE_STORAGE)
int aw_spinand_mtd_read_secure_storage(struct mtd_info *mtd,
		int item, void *buf, unsigned int len)
{
	int ret;
	struct aw_spinand *spinand = mtd_to_spinand(mtd);

	mutex_lock(&spinand->lock);
	ret = aw_spinand_secure_storage_read(&spinand->sec_sto,
		item, buf, len);
	mutex_unlock(&spinand->lock);

	return ret;
}

int aw_spinand_mtd_write_secure_storage(struct mtd_info *mtd,
		int item, void *buf, unsigned int len)
{
	int ret;
	struct aw_spinand *spinand = mtd_to_spinand(mtd);

	mutex_lock(&spinand->lock);
	ret = aw_spinand_secure_storage_write(&spinand->sec_sto,
		item, buf, len);
	mutex_unlock(&spinand->lock);

	return ret;
}
#endif

static int upload_boot(struct mtd_info *mtd,
		unsigned int startblk, unsigned int endblk,
		unsigned int pagesize, void *buf, unsigned int len)
{
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_chip_request req = {0};
	unsigned int blksize = info->phy_block_size(chip);
	unsigned int pagecnt = blksize / info->phy_page_size(chip);
	unsigned int read_blks = 0;
	int ret, pgindex = 0;

	/* check boundary */
	pagesize = pagesize ? pagesize : info->phy_page_size(chip);
	if (pagesize > info->phy_page_size(chip)) {
		pagesize = info->phy_page_size(chip);
		sunxi_warn(NULL, "reset download boot size for each page to %d\n", pagesize);
	}
	/* download */
	req.block = startblk;
	req.page = 0;
	do {
		if (req.page == 0) {
			/* must check bad and do erase for new block */
			ret = ops->phy_is_bad(chip, &req);
			if (ret == true) {
				sunxi_info(NULL, "skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				read_blks++;
				if (req.block >= endblk) {
					sunxi_info(NULL, "achieve maximum blocks %d\n", endblk);
					return read_blks;
				}
				/* continue to check bad blk before erase */
				continue;
			}
		}

		req.datalen = min(len, pagesize);
		/*
		 * only on the last time, datalen may be less than pagesize,
		 * to calculate the offset, we should use pagesize * pgindex
		 */
		req.databuf = buf + pgindex * pagesize;

		ret = ops->phy_read_page(chip, &req);
		if (ret) {
			sunxi_err(NULL, "read boot to blk %d page %d failed\n",
					req.block, req.page);
			return ret;
		}
		if (req.page == 0)
			read_blks++;

		pgindex++;
		len -= req.datalen;
		req.page++;
		if (req.page >= pagecnt) {
			req.page = 0;
			req.block++;
		}
	} while (len > 0);

	return read_blks;
}

int spinand_mtd_upload_boot0(struct mtd_info *mtd, unsigned int len, void *buf)
{
	unsigned start, end;
	int ret = 0;
	struct aw_spinand *spinand = mtd_to_spinand(mtd);

	start = NAND_BOOT0_BLK_START;
	/* start addr of uboot is the end addr of boot0 */
	aw_spinand_uboot_blknum(spinand, &end, NULL);

	/* In general, size of boot0 is less than a block */
	while (start < end) {
		sunxi_info(NULL, "upload boot0 to block %d len %dK\n", start, len / SZ_1K);
		ret = upload_boot(mtd, start, end, VALID_PAGESIZE_FOR_BOOT0,
				buf, len);
		if (ret <= 0) {
			sunxi_err(NULL, "download boot0 to blk %d failed\n", start);
			break;
		} else if (ret > 0) {
			/* if grater than zero, means had written @ret blks */
			start += ret;
		} else {
			start++;
		}
	}

	return 0;
}

int spinand_mtd_upload_uboot(struct mtd_info *mtd, unsigned int len, void *buf)
{
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	unsigned int phy_blk_size;
	unsigned int start, end, blks_per_uboot;
	int blks_written = 0;

	/* [start, end) */
	aw_spinand_uboot_blknum(spinand, &start, &end);
	phy_blk_size = info->phy_block_size(chip);
	blks_per_uboot = (len + phy_blk_size - 1) / phy_blk_size;

	sunxi_info(NULL, "uboot blk range [%d-%d)\n", start, end);
	if (len % info->page_size(chip)) {
		sunxi_err(NULL, "len (%d) of uboot must align to pagesize %d\n",
				len, info->page_size(chip));
		return -EINVAL;
	}

	while (start + blks_per_uboot <= end) {
		sunxi_info(NULL, "download uboot to block %d (%d blocks) len %dK\n",
				start, blks_per_uboot, len / SZ_1K);
		blks_written = upload_boot(mtd, start, end, 0, buf, len);
		if (blks_written <= 0) {
			sunxi_err(NULL, "download uboot to blk %d failed\n", start);
			return blks_written;
		}
		if (blks_written < blks_per_uboot) {
			sunxi_err(NULL, "something error, written %d blks but wanted %d blks\n",
					blks_written, blks_per_uboot);
			return -EINVAL;
		}
		start += blks_written;
	}

	return 0;
}

int spinand_mtd_upload_bootpackage(struct mtd_info *mtd, loff_t from,
		unsigned int len, void *buf)
{
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_chip_ops *ops = chip->ops;
	unsigned int phy_blk_size = info->phy_block_size(chip);
	unsigned int pagecnt = phy_blk_size / info->phy_page_size(chip);
	unsigned int pagesize = info->phy_page_size(chip);
	unsigned int start, end;
	struct aw_spinand_chip_request req = {0};
	int ret, pgindex = 0;

	/* [start, end) */
	aw_spinand_uboot_blknum(spinand, &start, &end);

	if (from > ((end - start) * phy_blk_size))
		sunxi_err(NULL, "bootpackage size is:0x%x, over read:0x%llx\n",
		       ((end - start) * phy_blk_size),
		       (start * phy_blk_size + from));
	else if (from + len > ((end - start) * phy_blk_size))
		len = ((end - start) * phy_blk_size) - from;

	req.block = start + (from >> 17);
	req.page = (from & (phy_blk_size - 1)) >> 11;
	do {
		if (req.page == 0) {
			/* must check bad and do erase for new block */
			ret = ops->phy_is_bad(chip, &req);
			if (ret == true) {
				sunxi_info(NULL, "skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				if (req.block >= end) {
					sunxi_info(NULL, "achieve maximum blocks %d\n", end);
					return -1;
				}
				/* continue to check bad blk before erase */
				continue;
			}
		}

		req.datalen = min(len, pagesize);
		/*
		 * only on the last time, datalen may be less than pagesize,
		 * to calculate the offset, we should use pagesize * pgindex
		 */
		req.databuf = buf + pgindex * pagesize;

		ret = ops->phy_read_page(chip, &req);
		if (ret) {
			sunxi_err(NULL, "read boot to blk %d page %d failed\n",
					req.block, req.page);
			return ret;
		}

		pgindex++;
		len -= req.datalen;
		req.page++;
		if (req.page >= pagecnt) {
			req.page = 0;
			req.block++;
		}
	} while (len > 0);

	return 0;
}
EXPORT_SYMBOL(spinand_mtd_upload_bootpackage);

int aw_spinand_mtd_download_boot0(struct mtd_info *mtd,
		unsigned int len, void *buf)
{
	unsigned int start, end;
	int ret = 0;
	struct aw_spinand *spinand = mtd_to_spinand(mtd);

	get_nand_para(spinand, buf);
	gen_check_sum(buf);

	start = NAND_BOOT0_BLK_START;
	/* start addr of uboot is the end addr of boot0 */
	aw_spinand_uboot_blknum(spinand, &end, NULL);

	/* In general, size of boot0 is less than a block */
	while (start < end) {
		sunxi_info(NULL, "download boot0 to block %d len %dK\n",
				start, len / SZ_1K);
		ret = download_boot(mtd, start, end,
				VALID_PAGESIZE_FOR_BOOT0,
				buf, len);
		if (ret <= 0) {
			sunxi_err(NULL, "download boot0 to blk %d failed\n",
					start);
			break;
		} else if (ret > 0) {
			/* means had written @ret blks */
			start += ret;
		} else {
			start++;
		}
	}

	return 0;
}

int aw_spinand_mtd_download_uboot(struct mtd_info *mtd,
		unsigned int len, void *buf)
{
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	unsigned int phy_blk_size;
	unsigned int start, end, blks_per_uboot;
	int blks_written = 0;

	/* [start, end) */
	aw_spinand_uboot_blknum(spinand, &start, &end);
	phy_blk_size = info->phy_block_size(chip);
	blks_per_uboot = (len + phy_blk_size - 1) / phy_blk_size;

	if (end - start < blks_per_uboot) {
		sunxi_err(NULL, "no enough space for at least one uboot\n");
		sunxi_err(NULL, "block per uboot is %u, however the uboot blks range is only [%u,%u)\n",
				blks_per_uboot, start, end);
		return -ENOSPC;
	}

	sunxi_info(NULL, "uboot blk range [%u-%u)\n", start, end);
	if (len % info->page_size(chip)) {
		sunxi_err(NULL, "len (%u) of uboot must align to pagesize %u\n",
				len, info->page_size(chip));
		return -EINVAL;
	}

	while (start + blks_per_uboot <= end) {
		sunxi_info(NULL, "download uboot to block %u (%u blocks) len %uK\n",
				start, blks_per_uboot, len / SZ_1K);
		blks_written = download_boot(mtd, start, end, 0, buf, len);
		if (blks_written <= 0) {
			sunxi_err(NULL, "download uboot to blk %u failed\n", start);
			return blks_written;
		}
		if (blks_written < blks_per_uboot) {
			sunxi_err(NULL, "something error, written %u blks but wanted %u blks\n",
					blks_written, blks_per_uboot);
			return -EINVAL;
		}
		start += blks_written;
	}

	return 0;
}

static struct mtd_partition aw_spinand_parts[] = {
	/* .size is set by @aw_spinand_mtd_update_mtd_parts */
	{ .name = "boot0", .offset = 0 },
	{ .name = "uboot", .offset = MTDPART_OFS_APPEND },
#if IS_ENABLED(CONFIG_AW_MTD_SPINAND_FASTBOOT)
	{ .name = "boot", .offset = MTDPART_OFS_APPEND },
	{ .name = "boot_backup", .offset = MTDPART_OFS_APPEND },
#endif
	{ .name = "secure_storage", .offset = MTDPART_OFS_APPEND },
#if IS_ENABLED(CONFIG_AW_SPINAND_PSTORE_MTD_PART)
	{ .name = "pstore", .offset = MTDPART_OFS_APPEND },
#endif
#if IS_ENABLED(CONFIG_RAW_KERNEL)
	{ .name = "kernel", .offset = MTDPART_OFS_APPEND },
#endif
	{ .name = "sys", .offset = MTDPART_OFS_APPEND},
};

static void aw_spinand_mtd_update_mtd_parts(struct aw_spinand *spinand,
		struct mtd_partition *mtdparts)
{
	struct aw_spinand_chip *chip = &spinand->chip;
	struct aw_spinand_info *info = chip->info;
	unsigned int blk_bytes = info->phy_block_size(chip);
	unsigned int uboot_start, uboot_end;
	int index = 0;
#if IS_ENABLED(CONFIG_AW_MTD_SPINAND_FASTBOOT)
	unsigned int bootsize = CONFIG_BIMAGE_SIZE * 1024 * 1024;
#endif

	aw_spinand_uboot_blknum(spinand, &uboot_start, &uboot_end);
	/* boot0 */
	mtdparts[index++].size = uboot_start * blk_bytes;
	/* uboot */
	mtdparts[index++].size = (uboot_end - uboot_start) * blk_bytes;
#if IS_ENABLED(CONFIG_AW_MTD_SPINAND_FASTBOOT)
	/* boot */
	mtdparts[index++].size = bootsize;
	/* boot_backup */
	mtdparts[index++].size = bootsize;
#endif
	/* secure storage */
	mtdparts[index++].size = PHY_BLKS_FOR_SECURE_STORAGE * blk_bytes;

#if IS_ENABLED(CONFIG_AW_SPINAND_PSTORE_MTD_PART)
	/* pstore */
	mtdparts[index++].size = PSTORE_SIZE_KB * SZ_1K;
#endif

#if IS_ENABLED(CONFIG_RAW_KERNEL)
	/* kernel */
	printk("kernel size:%d\n", CONFIG_KERNEL_SIZE_BYTE);
	if (CONFIG_KERNEL_SIZE_BYTE)
		mtdparts[index++].size =
			ALIGN(CONFIG_KERNEL_SIZE_BYTE, info->block_size(chip));
	else
		mtdparts[index++].size = info->block_size(chip);
#endif
	/* user data */
	mtdparts[index++].size = MTDPART_SIZ_FULL;

#if IS_ENABLED(CONFIG_AW_SPINAND_SECURE_STORAGE)
	spinand->sec_sto.chip = spinand_to_chip(spinand);
	spinand->sec_sto.startblk = uboot_end;
	spinand->sec_sto.endblk = uboot_end + PHY_BLKS_FOR_SECURE_STORAGE;
#endif
}

uint64_t get_sys_part_offset(void)
{
	uint64_t offset = aw_spinand_parts[0].offset;
	int count = sizeof(aw_spinand_parts) / sizeof(struct mtd_partition);
	int index;

	for (index = 0; index < count - 1; index++)
		offset += aw_spinand_parts[index].size;

	return offset;
}

static int aw_spinand_probe(struct spi_device *spi)
{
	struct aw_spinand *spinand;
	struct aw_spinand_chip *chip;
	int ret;
	struct device_node *chosen_node;
	const char *bootargs;

	if (g_spinand) {
		sunxi_info(NULL, "AW Spinand already initialized\n");
		return -EBUSY;
	}

	sunxi_info(NULL, "AW SPINand MTD Layer Version: %x.%x %x\n",
			AW_MTD_SPINAND_VER_MAIN, AW_MTD_SPINAND_VER_SUB,
			AW_MTD_SPINAND_VER_DATE);

	spinand = devm_kzalloc(&spi->dev, sizeof(*spinand), GFP_KERNEL);
	if (!spinand)
		return -ENOMEM;
	chip = spinand_to_chip(spinand);
	mutex_init(&spinand->lock);

	ret = aw_spinand_chip_init(spi, chip);
	if (ret)
		return ret;

	spinand->sector_shift = ffs(chip->info->sector_size(chip)) - 1;
	spinand->page_shift = ffs(chip->info->page_size(chip)) - 1;
	spinand->block_shift = ffs(chip->info->block_size(chip)) - 1;
	spinand->phy_page_shift = ffs(chip->info->phy_page_size(chip)) - 1;
	spinand->phy_block_shift = ffs(chip->info->phy_block_size(chip)) - 1;

	ret = aw_spinand_mtd_init(spinand);
	if (ret)
		goto err_spinand_cleanup;

	aw_spinand_mtd_update_mtd_parts(spinand, aw_spinand_parts);

	chosen_node = of_find_node_by_path("/chosen");
	if (chosen_node) {
		ret = of_property_read_string(chosen_node, "bootargs", &bootargs);
		if (ret) {
			sunxi_err(NULL, "Failed to get bootargs\n");
			ret = 1;
		}
	} else {
			sunxi_err(NULL, "Failed to get choosen node\n");
			ret = 1;
	}
#if IS_ENABLED(CONFIG_MTD_CMDLINE_PARTS)
	if (!ret && get_para_from_cmdline(bootargs, "mtdparts") > 0)
		ret = mtd_device_register(&spinand->mtd, NULL, 0);
	else
#else
		ret = mtd_device_register(&spinand->mtd, aw_spinand_parts,
				ARRAY_SIZE(aw_spinand_parts));
#endif
	if (ret)
		goto err_spinand_cleanup;

	g_spinand = spinand;
	return 0;

err_spinand_cleanup:
	aw_spinand_cleanup(spinand);
	return ret;
}

static int aw_spinand_remove(struct spi_device *spi)
{
	int ret;
	struct aw_spinand *spinand;
	struct mtd_info *mtd;

	spinand = spi_to_spinand(spi);
	mtd = spinand_to_mtd(spinand);

	ret = mtd_device_unregister(mtd);
	if (ret)
		return ret;

	aw_spinand_cleanup(spinand);
	return 0;
}

static const struct spi_device_id aw_spinand_ids[] = {
	{ .name = "spi-nand" },
	{ /* sentinel */ },
};

static const struct of_device_id aw_spinand_of_ids[] = {
	{ .compatible = "spi-nand" },
	{ /* sentinel */ },
};

static struct spi_driver aw_spinand_drv = {
	.driver = {
		.name = "spi-nand",
		.of_match_table = of_match_ptr(aw_spinand_of_ids),
	},
	.id_table = aw_spinand_ids,
	.probe = aw_spinand_probe,
	.remove = aw_spinand_remove,
};
module_spi_driver(aw_spinand_drv);

MODULE_AUTHOR("liaoweixiong <liaoweixiong@allwinnertech.com>");
MODULE_DESCRIPTION("Allwinner's spinand driver");
MODULE_VERSION("1.0.0");
