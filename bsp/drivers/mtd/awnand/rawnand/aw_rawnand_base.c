/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * SPDX-License-Identifier: GPL-2.0+
 * aw_rawnand_base.c
 *
 * (C) Copyright 2020 - 2021
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * cuizhikui <cuizhikui@allwinnertech.com>
 *
 */

#include <linux/mtd/aw-rawnand.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include "aw_rawnand_nfc.h"

#define DRIVER_NAME	"awrawnand-mtd"

#define SUNXI_RAWNAND_MODULE_VERSION	"1.0.3"
#define VERSION "v1.32 2023-09-23 17:15"
#define DISPLAY_VERSION	printk("awnand-mtd-version:%s(%s)\n", VERSION, SUNXI_RAWNAND_MODULE_VERSION)

struct aw_nand_chip awnand_chip;
extern struct aw_nand_host aw_host;

struct class *rawnand_class;
struct device *rawnand_device;
static int major_number;

enum ota_type {
    BOOT0,
    UBOOT,
    SECURE_STORAGE,
};

struct burn_param_t {
	enum ota_type type;
	size_t length;
	void *buffer;
};

extern int aw_rawnand_get_ecc_mode(int sparesize, int pagesize);
extern int aw_rawnand_get_ecc_bits(int bch_mode);

void dump_data(void *data, int len)
{
	int i = 0;
	uint8_t *p = (uint8_t *)data;


	for (i = 0; i < len; i++) {
		if (len % 16 == 0)
			printk("%08x: ", i);
		printk("%02x ", p[i]);
		if (len % 16 == 15)
			printk("\n");

	}
	printk("\nlen@%d\n", len);
}

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

/*get ecc mode interger operation may be cause 1 level ecc mode different,
 * so test and correct*/
static int aw_rawnand_ecc_mode_test(int sparesize, int pagesize, int ecc_mode, int *ret_ecc_mode)
{
	int ecc_bits = ecc_bits_tbl[ecc_mode];
	int val = 0;
	int res = 0;
	int res2 = 0;
	int ret = 0;

	val = ((14 * ecc_bits) >> 3);
	val *= B_TO_KB(pagesize);

	res = sparesize - val;
	/*at least reserve 8 Byte avalid spare*/
	if (res > 8 && ecc_mode < (sizeof(ecc_bits_tbl) - 1)) {
		val = ((14 * ecc_bits_tbl[ecc_mode+1]) >> 3);
		val *= B_TO_KB(pagesize);
		res2 = sparesize - val;
		if (res2 >= 8) {
			*ret_ecc_mode = ecc_mode + 1;
		} else
			*ret_ecc_mode = ecc_mode;
	} else if (res < 0)
		ret = -EINVAL;
	else
		*ret_ecc_mode = ecc_mode;

	return ret;
}

void aw_rawnand_select_chip(struct mtd_info *mtd, int c)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	awrawnand_chip_trace("Enter %s c@%d\n", __func__, c);

	if (unlikely(c > MAX_CHIPS) && unlikely(c != -1)) {
		awrawnand_err("chip@%d exceed\n", c);
	}

	if (c != -1) {
		chip->selected_chip.chip_no = c;
		chip->selected_chip.ceinfo[c].ce_no = c;
		chip->selected_chip.ceinfo[c].relate_rb_no = c;
		aw_host_nfc_chip_select(&host->nfc_reg, c);
	} else {
		chip->selected_chip.chip_no = c;
		/*select invalid CE to selected chip*/
		aw_host_nfc_chip_select(&host->nfc_reg, 0xf);
	}
	awrawnand_chip_trace("Exit %s c@%d\n", __func__, c);
}

static int aw_rawnand_chip_reset(struct mtd_info *mtd, int c)
{
	int ret = 0;
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);


	NORMAL_REQ_CMD(req, RAWNAND_CMD_RESET);

	awrawnand_chip_trace("Enter %s c@%d\n", __func__, c);

	chip->select_chip(mtd, c);

	ret = host->normal_op(chip, &req);
	if (!ret)
		ret = chip->dev_ready_wait(mtd);
	chip->select_chip(mtd, -1);

	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret ? 0 : -EIO;
}

/*
 * it judges wheter device is busy or ready by RB line
 * timeout 60s
 * */
static bool aw_rawnand_dev_ready_wait(struct mtd_info *mtd)
{
	int ret = -ETIMEDOUT;
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	unsigned long timeout = 0;

	awrawnand_chip_trace("Enter %s\n", __func__);

	timeout = jiffies + msecs_to_jiffies(60000);

	do {
		if (chip->selected_chip.chip_no != -1) {
			ret = host->rb_ready(chip, host);
			if (ret)
				goto out;
		}
	} while (time_before(jiffies, timeout));

	if (ret == false)
		awrawnand_err("dev is busy(rb#)\n");

	awrawnand_chip_trace("Exit %s\n", __func__);
out:
	return ret;
}

static int aw_rawnand_chip_status(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	uint8_t status = 0;
	int ret = 0;

	NORMAL_REQ_CMD_WITH_ADDR0_DATA_IN(req, RAWNAND_CMD_STATUS, &status, 1);

	awrawnand_chip_trace("Enter %s\n", __func__);

	if (chip->selected_chip.chip_no != -1) {
		ret = host->rb_ready(chip, host);
		if (!ret) {
			awrawnand_err("dev is busy, to get status fail\n");
			goto out;
		}
		ret = host->normal_op(chip, &req);
		if (ret)
			awrawnand_err("%s cmd@%d fail\n", __func__, RAWNAND_CMD_STATUS);
	}

out:
	awrawnand_chip_trace("Exit %s status@%x\n", __func__, status);
	return status;
}

static int aw_rawnand_read_id(struct mtd_info *mtd, struct aw_nand_chip *chip, uint8_t *id)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	int ret = 0;

	/*send read id command@90h*/
	NORMAL_REQ_CMD_WITH_ADDR1_DATA_IN(req, RAWNAND_CMD_READID, 0x00, id, RAWNAND_MAX_ID_LEN);

	awrawnand_chip_trace("Enter %s \n", __func__);

	ret = host->normal_op(chip, &req);
	if (ret) {
		awrawnand_err("read id fail\n");
	}

	awrawnand_chip_trace("Exit %s \n", __func__);

	return ret;
}

int aw_rawnand_chip_erase(struct mtd_info *mtd, int page)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;

	uint8_t row1 = page & 0xff;
	uint8_t row2 = (page >> 8) & 0xff;
	uint8_t row3 = (page >> 16) & 0xff;

	NORMAL_REQ_CMD_WITH_ADDR_N3(req, RAWNAND_CMD_ERASE1, row1, row2, row3);
	NORMAL_REQ_CMD(req2, RAWNAND_CMD_ERASE2);

	awrawnand_chip_trace("Enter %s block@%d\n", __func__, page >> chip->pages_per_blk_shift);

	ret = host->normal_op(chip, &req);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE1, page);

	ret = host->normal_op(chip, &req2);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE2, page);

	if (!chip->dev_ready_wait(mtd))
		awrawnand_err("device is busy after erase cmd@%x page@%d\n", RAWNAND_CMD_ERASE2, page);

	status = chip->dev_status(mtd);

	if (status & RAWNAND_STATUS_FAIL) {
		ret = -EIO;
		awrawnand_err("%s erase block@%d fail\n", __func__, page >> chip->pages_per_blk_shift);
	}

	awrawnand_chip_trace("Exit %s \n", __func__);

	return ret;
}

int aw_rawnand_chip_real_multi_erase(struct mtd_info *mtd, int page)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;

	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = (blkA << chip->pages_per_blk_shift);
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));

	uint8_t rowA_1 = pageA & 0xff;
	uint8_t rowA_2 = (pageA >> 8) & 0xff;
	uint8_t rowA_3 = (pageA >> 16) & 0xff;

	uint8_t rowB_1 = pageB & 0xff;
	uint8_t rowB_2 = (pageB >> 8) & 0xff;
	uint8_t rowB_3 = (pageB >> 16) & 0xff;

	NORMAL_REQ_CMD_WITH_ADDR_N3(reqA, RAWNAND_CMD_ERASE1, rowA_1, rowA_2, rowA_3);

	NORMAL_REQ_CMD_WITH_ADDR_N3(reqB, RAWNAND_CMD_ERASE1, rowB_1, rowB_2, rowB_3);

	NORMAL_REQ_CMD(req2, RAWNAND_CMD_ERASE2);

	awrawnand_chip_trace("Enter %s block@[%d:%d]\n", __func__,
			blkA, blkA + 1);

	ret = host->normal_op(chip, &reqA);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE1, page);

	ret = host->normal_op(chip, &reqB);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE1, page);

	ret = host->normal_op(chip, &req2);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE2, page);

	if (!chip->dev_ready_wait(mtd))
		awrawnand_err("device is busy after erase cmd@%x page@%d\n", RAWNAND_CMD_ERASE2, page);

	status = chip->dev_status(mtd);

	if (status & RAWNAND_STATUS_FAIL) {
		ret = -EIO;
		awrawnand_err("%s erase block@%d fail\n", __func__, page >> chip->pages_per_blk_shift);
	}

	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);

	return ret;
}

int aw_rawnand_chip_real_onfi_multi_erase(struct mtd_info *mtd, int page)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;

	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = (blkA << chip->pages_per_blk_shift);
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));

	uint8_t rowA_1 = pageA & 0xff;
	uint8_t rowA_2 = (pageA >> 8) & 0xff;
	uint8_t rowA_3 = (pageA >> 16) & 0xff;

	uint8_t rowB_1 = pageB & 0xff;
	uint8_t rowB_2 = (pageB >> 8) & 0xff;
	uint8_t rowB_3 = (pageB >> 16) & 0xff;

	NORMAL_REQ_CMD_WITH_ADDR_N3(reqA, RAWNAND_CMD_ERASE1, rowA_1, rowA_2, rowA_3);

	NORMAL_REQ_CMD(req2, RAWNAND_CMD_MULTIERASE);

	NORMAL_REQ_CMD_WITH_ADDR_N3(reqB, RAWNAND_CMD_ERASE1, rowB_1, rowB_2, rowB_3);

	NORMAL_REQ_CMD(req3, RAWNAND_CMD_ERASE2);

	awrawnand_chip_trace("Enter %s block@[%d:%d]\n", __func__,
			blkA, blkA + 1);

	ret = host->normal_op(chip, &reqA);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE1, page);

	ret = host->normal_op(chip, &req2);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE2, page);

	if (!chip->dev_ready_wait(mtd))
		awrawnand_err("device is busy after erase cmd@%x page@%d\n", RAWNAND_CMD_ERASE2, page);

	ret = host->normal_op(chip, &reqB);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE1, page);

	ret = host->normal_op(chip, &req3);
	if (ret)
		awrawnand_err("%s cmd@%d page@%d fail\n", __func__, RAWNAND_CMD_ERASE2, page);

	if (!chip->dev_ready_wait(mtd))
		awrawnand_err("device is busy after erase cmd@%x page@%d\n", RAWNAND_CMD_ERASE2, page);

	status = chip->dev_status(mtd);

	if (status & RAWNAND_STATUS_FAIL) {
		ret = -EIO;
		awrawnand_err("%s erase block@%d fail\n", __func__, page >> chip->pages_per_blk_shift);
	}

	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);

	return ret;
}

int aw_rawnand_chip_simu_multi_erase(struct mtd_info *mtd, int page)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = (blkA << chip->pages_per_blk_shift);
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));
	int ret = 0;

	awrawnand_chip_trace("Enter %s block@[%d:%d]\n", __func__,
			blkA, blkA + 1);

	ret = aw_rawnand_chip_erase(mtd, pageA);
	if (!ret) {
		ret = aw_rawnand_chip_erase(mtd, pageB);
	}

	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);

	return ret;
}

int aw_rawnand_chip_multi_erase(struct mtd_info *mtd, int page)
{
	int ret = 0;
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	if (RAWNAND_HAS_MULTI_ERASE(chip))
		ret = aw_rawnand_chip_real_multi_erase(mtd, page);
	else if (RAWNAND_HAS_MULTI_ONFI_ERASE(chip))
		ret = aw_rawnand_chip_real_onfi_multi_erase(mtd, page);
	else
		ret = aw_rawnand_chip_simu_multi_erase(mtd, page);

	return ret;
}

int aw_rawnand_chip_write_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;
	int row_cycles = chip->row_cycles;


	BATCH_REQ_WRITE(req, page, row_cycles, mdata, mlen, sdata, slen);

	awrawnand_chip_trace("Enter %s page@%d\n", __func__, page);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = host->batch_op(chip, &req);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	status = chip->dev_status(mtd);
	if (status & RAWNAND_STATUS_FAIL) {
		awrawnand_err("write page@%d fail\n", page);
		ret = -EIO;
	}

out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}

int aw_rawnand_chip_cache_write_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	int row_cycles = chip->row_cycles;


	BATCH_REQ_CACHE_WRITE(req, page, row_cycles, mdata, mlen, sdata, slen);

	awrawnand_chip_trace("Enter %s page@%d\n", __func__, page);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = host->batch_op(chip, &req);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}

/**
 * aw_rawnand_chip_real_multi_write_page - multi plane write
 * @mtd: MTD structure
 * @chip: aw nand chip strucutre
 * @mdata: data
 * @mlen: mdatalen, is equal to 2*chip->pagesize
 * @sdata: spare data
 * @slen: spare len(chip->avalid_sparesize)
 * @page: pageno
 * **/
int aw_rawnand_chip_real_multi_write_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;
	int row_cycles = chip->row_cycles;
	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = ((blkA << chip->pages_per_blk_shift) + (page & chip->pages_per_blk_mask));
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));

	BATCH_REQ_MULTI_WRITE(reqA, pageA, row_cycles, mdata, chip->pagesize, sdata, slen, PLANE_A);

	BATCH_REQ_WRITE(reqB, pageB, row_cycles, (mdata + chip->pagesize),
			chip->pagesize, sdata, slen);

	awrawnand_chip_trace("Enter %s page@[%d:%d]\n", __func__, pageA, pageB);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = host->batch_op(chip, &reqA);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}


	ret = host->batch_op(chip, &reqB);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	status = chip->dev_status(mtd);
	if (status & RAWNAND_STATUS_FAIL) {
		awrawnand_err("write page@%d fail\n", page);
		ret = -EIO;
	}

out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;

}

/**
 * aw_rawnand_chip_real_jedec_multi_write_page - jedec multi plane write
 * @mtd: MTD structure
 * @chip: aw nand chip strucutre
 * @mdata: data
 * @mlen: mdatalen, is equal to 2*chip->pagesize
 * @sdata: spare data
 * @slen: spare len(chip->avalid_sparesize)
 * @page: pageno
 * **/
int aw_rawnand_chip_real_jedec_multi_write_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;
	uint8_t status = 0;
	int row_cycles = chip->row_cycles;
	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = ((blkA << chip->pages_per_blk_shift) + (page & chip->pages_per_blk_mask));
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));

	BATCH_REQ_MULTI_WRITE(reqA, pageA, row_cycles, mdata, chip->pagesize, sdata, slen, PLANE_A);

	BATCH_REQ_JEDEC_WRITE(reqB, pageB, row_cycles, (mdata + chip->pagesize),
			chip->pagesize, sdata, slen);

	awrawnand_chip_trace("Enter %s page@[%d:%d]\n", __func__, pageA, pageB);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = host->batch_op(chip, &reqA);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}


	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = host->batch_op(chip, &reqB);
	if (ret == ECC_ERR) {
		awrawnand_err("%s write page@%d fail\n", __func__, 0);
		goto out;
	}

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy write page@%d fail\n", page);
		ret = -ETIMEDOUT;
		goto out;
	}

	status = chip->dev_status(mtd);
	if (status & RAWNAND_STATUS_FAIL) {
		awrawnand_err("write page@%d fail\n", page);
		ret = -EIO;
	}

out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;

}

int aw_rawnand_chip_simu_multi_write_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	int ret = 0;

	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = ((blkA << chip->pages_per_blk_shift) + (page & chip->pages_per_blk_mask));
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));

	awrawnand_chip_trace("Enter %s page@[%d:%d]\n", __func__, pageA, pageB);

	ret = aw_rawnand_chip_write_page(mtd, chip, mdata, (mlen >> 1), sdata, slen, pageA);
	if (!ret) {
		ret = aw_rawnand_chip_write_page(mtd, chip, (mdata + chip->pagesize),
				chip->pagesize, sdata, slen, pageB);
	}

	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);

	return ret;
}

int aw_rawnand_chip_multi_write_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	int ret = 0;
	if (RAWNAND_HAS_MULTI_WRITE(chip))
		ret = aw_rawnand_chip_real_multi_write_page(mtd, chip, mdata, mlen, sdata, slen, page);
	else if (RAWNAND_HAS_JEDEC_MULTI_WRITE(chip))
		ret = aw_rawnand_chip_real_jedec_multi_write_page(mtd, chip, mdata, mlen, sdata, slen, page);
	else
		ret = aw_rawnand_chip_simu_multi_write_page(mtd, chip, mdata, mlen, sdata, slen, page);

	return ret;
}

int aw_rawnand_chip_read_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	int row_cycles = chip->row_cycles;

	int ret = 0;

	BATCH_REQ_READ(req, page, row_cycles, mdata, mlen, sdata, slen);

	awrawnand_chip_trace("Enter %s page@%d\n", __func__, page);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy read page@%d fail\n", page);
		ret = -EIO;
		goto out;
	}

	ret = host->batch_op(chip, &req);
	if (ret == ECC_ERR)
		awrawnand_err("read page@%d fail\n", page);

out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}

int aw_rawnand_chip_read_page_spare(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	int row_cycles = chip->row_cycles;

	int ret = 0;

	BATCH_REQ_READ_ONLY_SPARE(req, page, row_cycles, sdata, slen);

	awrawnand_chip_trace("Enter %s page@%d\n", __func__, page);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy read page@%d spare fail\n", page);
		ret = -EIO;
		goto out;
	}

	ret = host->batch_op(chip, &req);
	if (ret == ECC_ERR)
		awrawnand_err("read page@%d spare fail\n", page);

out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}

int aw_rawnand_chip_simu_multi_read_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	int row_cycles = chip->row_cycles;

	int ret = 0;
	int blkA = ((page >> chip->pages_per_blk_shift) << 1);
	int pageA = ((blkA << chip->pages_per_blk_shift) + (page & chip->pages_per_blk_mask));
	int pageB = (pageA + (1 << chip->pages_per_blk_shift));

	BATCH_REQ_READ(reqA, pageA, row_cycles, mdata, chip->pagesize, sdata, chip->avalid_sparesize);

	BATCH_REQ_READ(reqB, pageB, row_cycles, (mdata + chip->pagesize),
			chip->pagesize, (sdata + chip->avalid_sparesize), chip->avalid_sparesize);
	awrawnand_chip_trace("Enter %s page@[%d:%d]\n", __func__, pageA, pageB);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy read page@%d fail\n", pageA);
		ret = -EIO;
		goto out;
	}

	ret = host->batch_op(chip, &reqA);
	if (ret == ECC_ERR)
		awrawnand_err("read pageA@%d fail\n", pageA);

	if (!chip->dev_ready_wait(mtd)) {
		awrawnand_err("dev is busy read page@%d fail\n", pageB);
		ret = -EIO;
		goto out;
	}

	ret = host->batch_op(chip, &reqB);
	if (ret == ECC_ERR)
		awrawnand_err("read pageB@%d fail\n", pageB);
out:
	awrawnand_chip_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}


int aw_rawnand_chip_multi_read_page(struct mtd_info *mtd, struct aw_nand_chip *chip,
		uint8_t *mdata, int mlen, uint8_t *sdata, int slen, int page)
{
	int ret = 0;

	ret =  aw_rawnand_chip_simu_multi_read_page(mtd, chip,
			mdata, mlen, sdata, slen, page);

	return ret;
}

int aw_rawnand_setup_read_retry(struct mtd_info *mtd, struct aw_nand_chip *chip)
{
	int ret = 0;

	/*to do*/

	return ret;
}
/*
int aw_rawnand_setup_data_interface(struct mtd_info *mtd, struct aw_nand_chip *chip,
		int chipnr, const struct nand_data_interface *conf)
{
	int ret = 0;

	int c = 0;

	chip->select(chip, c);
	ret = aw_host_set_interface(&chip->host);
	chip->select(chip, -1);

	return ret;
}
*/

int aw_rawnand_set_feature(struct aw_nand_chip *chip,
		int feature_addr, uint8_t *feature_para)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int ret = 0;
	int retry = 0;

	/*set feature*/
	NORMAL_REQ_CMD_WITH_ADDR1_DATA_OUT(req, RAWNAND_CMD_SET_FEATURES, feature_addr, feature_para,
			FEATURES_PARA_LEN);

	awrawnand_chip_trace("Enter %s [%x:%x]\n", __func__, feature_addr, *feature_para);
retry0:
	if (!chip->dev_ready_wait(mtd)) {
		retry++;
		if (retry < 3)
			goto retry0;
		else
			goto out_ready_fail;
	}
	aw_host_nfc_repeat_mode_enable(&host->nfc_reg);
	aw_host_nfc_randomize_disable(&host->nfc_reg);

	ret = host->normal_op(chip, &req);
	if (ret)
		goto out_set_feature_fail;

	aw_host_nfc_repeat_mode_disable(&host->nfc_reg);

	awrawnand_chip_trace("Exit %s-%d ret@%d\n", __func__, __LINE__, ret);
	return ret;

out_ready_fail:
	awrawnand_err("device is busy\n");
	awrawnand_chip_trace("Exit %s-%d ret@%d\n", __func__, __LINE__, ret);
	return ret;
out_set_feature_fail:
	aw_host_nfc_repeat_mode_disable(&host->nfc_reg);
	awrawnand_err("set feature fail\n");
	awrawnand_chip_trace("Exit %s-%d ret@%d\n", __func__, __LINE__, ret);
	return ret;
}

int aw_rawnand_get_feature(struct aw_nand_chip *chip,
		int feature_addr, uint8_t *feature_para)
{
	struct aw_nand_host *host = awnand_chip_to_host(chip);
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	int ret = 0;
	int retry = 0;

	/*get feature*/
	NORMAL_REQ_CMD_WITH_ADDR1_DATA_IN(req, RAWNAND_CMD_GET_FEATURES, feature_addr, feature_para,
			FEATURES_PARA_LEN);

	awrawnand_chip_trace("Enter %s [%x:%x]\n", __func__, feature_addr, *feature_para);
retry0:
	if (!chip->dev_ready_wait(mtd)) {
		retry++;
		if (retry < 3)
			goto retry0;
		else
			goto out_ready_fail;
	}

	aw_host_nfc_repeat_mode_enable(&host->nfc_reg);
	aw_host_nfc_randomize_disable(&host->nfc_reg);

	ret = host->normal_op(chip, &req);
	if (ret)
		goto out_get_feature_fail;

	aw_host_nfc_repeat_mode_disable(&host->nfc_reg);
	awrawnand_chip_trace("Enter %s [%x:%x]\n", __func__, feature_addr, *feature_para);
	return ret;

out_ready_fail:
	awrawnand_err("device is busy\n");
	awrawnand_chip_trace("Enter %s [%x:%x]\n", __func__, feature_addr, *feature_para);
	return ret;

out_get_feature_fail:
	aw_host_nfc_repeat_mode_disable(&host->nfc_reg);
	awrawnand_err("get feature fail\n");
	awrawnand_chip_trace("Enter %s [%x:%x]\n", __func__, feature_addr, *feature_para);
	return ret;
}

void aw_rawnand_chip_init_pre(struct aw_nand_chip *chip)
{

	struct rawnand_data_interface *itf = &chip->data_interface;

	if (!chip->select_chip)
		chip->select_chip = aw_rawnand_select_chip;

	if (!chip->dev_ready_wait)
		chip->dev_ready_wait = aw_rawnand_dev_ready_wait;

	if (!chip->dev_status)
		chip->dev_status = aw_rawnand_chip_status;

	if (!chip->block_bad)
		chip->block_bad = aw_rawnand_chip_block_bad;

	if (!chip->simu_block_bad)
		chip->simu_block_bad = aw_rawnand_chip_simu_block_bad;

	if (!chip->block_markbad)
		chip->block_markbad = aw_rawnand_chip_block_markbad;

	if (!chip->simu_block_markbad)
		chip->simu_block_markbad = aw_rawnand_chip_simu_block_markbad;

	if (!chip->scan_bbt)
		chip->scan_bbt = aw_rawnand_chip_scan_bbt;

	if (!chip->erase)
		chip->erase = aw_rawnand_chip_erase;

	if (!chip->multi_erase)
		chip->multi_erase = aw_rawnand_chip_multi_erase;

	if (!chip->write_page)
		chip->write_page = aw_rawnand_chip_write_page;

	if (!chip->multi_write_page)
		chip->multi_write_page = aw_rawnand_chip_multi_write_page;

	if (!chip->cache_write_page)
		chip->cache_write_page = aw_rawnand_chip_cache_write_page;

	if (!chip->read_page)
		chip->read_page = aw_rawnand_chip_read_page;

	if (!chip->multi_read_page)
		chip->multi_read_page = aw_rawnand_chip_multi_read_page;

	if (!chip->read_page_spare)
		chip->read_page_spare = aw_rawnand_chip_read_page_spare;

	if (!chip->setup_read_retry)
		chip->setup_read_retry = aw_rawnand_setup_read_retry;
/*
	if (!chip->setup_data_interface)
		chip->setup_data_interface = aw_rawnand_setup_data_interface;
*/
	if (!itf->set_feature)
		chip->data_interface.set_feature = aw_rawnand_set_feature;

	if (!itf->get_feature)
		chip->data_interface.get_feature = aw_rawnand_get_feature;
/*
	if (!itf->onfi_set_feature)
		chip->data_interface.onfi_set_feature = aw_rawnand_onfi_set_feature;

	if (!itf->onfi_get_feature)
		chip->data_interface.onfi_get_feature = aw_rawnand_onfi_get_feature;

	if (!itf->toggle_set_feature)
		chip->data_interface.toggle_set_feature = aw_rawnand_toggle_set_feature;

	if (!itf->toggle_get_feature)
		chip->data_interface.toggle_get_feature = aw_rawnand_toggle_get_feature;
*/
	chip->selected_chip.chip_no = -1;
}

static bool aw_rawnand_is_valid_id(uint8_t id)
{
	bool ret = false;
	switch (id) {
	case RAWNAND_MFR_TOSHIBA:
	case RAWNAND_MFR_SAMSUNG:
	case RAWNAND_MFR_FUJITSU:
	case RAWNAND_MFR_NATIONAL:
	case RAWNAND_MFR_RENESAS:
	case RAWNAND_MFR_STMICRO:
	case RAWNAND_MFR_HYNIX:
	case RAWNAND_MFR_MICRON:
	case RAWNAND_MFR_AMD:
	case RAWNAND_MFR_MACRONIX:
	case RAWNAND_MFR_EON:
	case RAWNAND_MFR_SANDISK:
	case RAWNAND_MFR_INTEL:
	case RAWNAND_MFR_ATO:
	case RAWNAND_MFR_GIGA:
	/*case RAWNAND_MFR_SPANSION:*/ /*the same AMD*/
	/*case RAWNAND_MFR_ESMT:*/ /*the same GIGA*/
	/*case RAWNAND_MFR_MXIC:*/ /*the same MACRONIX*/
	/*case RAWNAND_MFR_FORESEE:*/ /*the same SAMSUNG*/
	case RAWNAND_MFR_WINBOND:
	case RAWNAND_MFR_DOSILICON:
	case RAWNAND_MFR_FORESEE_1:
	case RAWNAND_MFR_DOSILICON_1:
		ret = true;
		break;
	default:
		pr_warn("Unrecognized id@%02x\n", id);
		break;
	}
	return ret;
}

static inline void aw_rawnand_display_chip_id(uint8_t *id, int chip)
{
	awrawnand_print("detect the chip@%d:%02x %02x %02x %02x %02x %02x %02x %02x\n", chip,
			id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
}

static bool aw_rawnand_detect_device(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int c = 0;
	int ret = 0;
	uint8_t id[RAWNAND_MAX_ID_LEN];

	for (c = 0; c < MAX_CHIPS; c++) {
		/*chip should be the same*/
		memset(id, 0, RAWNAND_MAX_ID_LEN);
		ret = aw_rawnand_chip_reset(mtd, c);
		chip->select_chip(mtd, c);
		ret = aw_rawnand_read_id(mtd, chip, id);
		if (ret)
			goto out_read_id_fail;
		if (aw_rawnand_is_valid_id(id[0])) {
			chip->chips++;
			chip->ceinfo[c].ce_no = c;
			chip->ceinfo[c].relate_rb_no = c;
			memcpy(chip->id, id, RAWNAND_MAX_ID_LEN);
			aw_rawnand_display_chip_id(chip->id, c);
		}
		chip->select_chip(mtd, -1);
	}

	return true;

out_read_id_fail:
	awrawnand_err("read id fail\n");
	return false;

}

static bool aw_rawnand_device_match_idtab(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_flash_dev *dev = NULL;
	int m = 0;
	int d = 0;
	for (m = 0; m < aw_nand_manufs.nm; m++) {
		struct rawnand_manufacture *manuf = &aw_nand_manufs.manuf[m];
		if (chip->id[0] != manuf->id)
			continue;
		for (d = 0; d < manuf->ndev; d++) {
			dev = &manuf->dev[d];
			if (chip->id[1] == dev->dev_id) {
				if (!memcmp(chip->id, dev->id, dev->id_len)) {
					chip->dev = &manuf->dev[d];
					return true;
				}
			}
		}
	}
	return false;
}

static int  aw_rawnand_scan_device(struct mtd_info *mtd)
{
	int ret = -ENODEV;

	if (aw_rawnand_detect_device(mtd)) {
		if (aw_rawnand_device_match_idtab(mtd))
			ret = 0;
		else
			awrawnand_err("dev can't found in idtab\n");
	}

	return ret;
}

static int aw_rawnand_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int page = 0, c = 0, block = 0;
	int pages_per_block = 1 << chip->simu_pages_per_blk_shift;
	int page_in_chip = 0;

	uint64_t len = instr->len;

	awrawnand_mtd_trace("Enter %s addr@%llx\n", __func__, instr->addr);

	if (check_offs_len(mtd, instr->addr, instr->len)) {
		return -EINVAL;
	}

	page = instr->addr >> chip->simu_pagesize_shift;
	page_in_chip = page & chip->simu_chip_pages_mask;
	c = instr->addr >> chip->simu_chip_shift;

	mutex_lock(&chip->lock);

	chip->select_chip(mtd, c);
	do {
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
		ret = chip->multi_erase(mtd, page_in_chip);
#else
		ret = chip->erase(mtd, page_in_chip);
#endif
		if (ret) {
			awrawnand_err("erase block@%d fail\n", block);
			instr->fail_addr = instr->addr;
			goto out;
		}

		len -= chip->simu_erasesize;

		page += pages_per_block;
		page_in_chip = page & chip->simu_chip_pages_mask;

		/*check, if we cross next chip*/
		if (!page_in_chip) {
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, ++c);
		}

	} while (len > 0);

out:
	chip->select_chip(mtd, -1);

	mutex_unlock(&chip->lock);

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);

	return ret;
}

static int aw_rawnand_mtd_panic_write (struct mtd_info *mtd, loff_t to, size_t len,
			     size_t *retlen, const u_char *buf)
{
	int ret = 0;


	return ret;
}


/**
 * aw_rawnand_mtd_read - read data without oob
 * @mtd: mtd device structure
 * @from: offset to read from
 * @ops: oob operation descrition structure
 * */
static int aw_rawnand_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
		      size_t *retlen, u_char *buf)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int chipnr = 0, page = 0, page_in_chip = 0;
	int had_readlen = 0;
	uint8_t *mdata = (uint8_t *)buf;
	unsigned int max_bitflips = 0;
	bool ecc_failed = false;
	struct aw_nand_chip_cache *buffer = &chip->simu_chip_buffer;
	uint8_t *pagebuf = buffer->simu_pagebuf;
	int col = from & chip->simu_pagesize_mask;
	int mlen = min_t(unsigned int, (mtd->writesize - col), len);

	awrawnand_mtd_trace("Enter %s [%llx:%lx]\n", __func__, from, len);

	*retlen = 0;

	chipnr = (int)(from >> chip->simu_chip_shift);
	page = (int)(from >> chip->simu_pagesize_shift);
	page_in_chip = page & chip->simu_chip_pages_mask;

	mutex_lock(&chip->lock);

	chip->select_chip(mtd, chipnr);

	do {
		if ((page_in_chip == buffer->simu_pageno) &&
			(col >= buffer->valid_col) &&
			((col + len) <= (buffer->valid_col + buffer->valid_len))) {

			memcpy(mdata, pagebuf + col, mlen);
			mtd->ecc_stats.failed = 0;
			max_bitflips = buffer->bitflips;

		} else {

			/*
			 *int len_to_read = (B_TO_KB(mlen) + (MOD(mlen, 1024) ? 1 : 0));
			 *[>ret = chip->read_page(mtd, chip, pagebuf, buffer->page_len, NULL, 0,<]
			 *len_to_read = (len_to_read << 10);
			 */
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
			ret = chip->multi_read_page(mtd, chip, pagebuf, buffer->simu_page_len, NULL, 0,
					page_in_chip);
#else
			ret = chip->read_page(mtd, chip, pagebuf, buffer->simu_page_len, NULL, 0,
					page_in_chip);
#endif
			if (ret == ECC_LIMIT) {
				ret = mtd->bitflip_threshold;
				mtd->ecc_stats.corrected += chip->bitflips;
				max_bitflips = max_t(unsigned int, max_bitflips, ret);
				awrawnand_info("ecc limit from@0x%llx len@0x%zx in page@%d\n",
						from, len, page);

			} else if (ret == ECC_ERR) {
				ecc_failed = true;
				mtd->ecc_stats.failed++;
				awrawnand_err("ecc err from@0x%llx len@0x%zx in page@%d\n",
						from, len, page);
				buffer->simu_pageno = INVALID_CACHE;
				buffer->simu_oobno = INVALID_CACHE;
				buffer->chipno = INVALID_CACHE;
			} else if (ret < 0) {
				awrawnand_err("read from @0x%llx len@0x%zx fail in page@%d\n",
						from, len, page);

				buffer->simu_pageno = INVALID_CACHE;
				buffer->simu_oobno = INVALID_CACHE;
				buffer->chipno = INVALID_CACHE;
				break;
			}
			/*dump_data(pagebuf, buffer->page_len);*/

			buffer->simu_pageno = page_in_chip;
			buffer->valid_col = col;
			buffer->valid_len = mlen;
			buffer->chipno = chipnr;

			mtd->ecc_stats.failed = 0;
			buffer->bitflips = max_bitflips;
			memcpy(mdata, pagebuf + col, mlen);

		}

		had_readlen += mlen;
		len -= mlen;
		mdata += mlen;
		/*buf += mlen;*/
		mlen = min_t(unsigned int, len, mtd->writesize);

		col = 0;
		page++;
		page_in_chip = page & chip->simu_chip_pages_mask;

		if (!page_in_chip) {
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, ++chipnr);
		}

	} while (mlen);


	/*buffer->pageno = INVALID_CACHE;*/

	chip->select_chip(mtd, -1);

	*retlen = had_readlen;

	mutex_unlock(&chip->lock);

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);

	if (ret < 0)
		return ret;

	if (ecc_failed)
		return -EBADMSG;

	return max_bitflips;
}




/**
 * aw_rawnand_mtd_read_oob - read data with oob
 * @mtd: MTD device structure
 * @from: offset to read from
 * @ops: oob operation descrition structure
 * */
static int aw_rawnand_mtd_read_oob(struct mtd_info *mtd, loff_t from,
			  struct mtd_oob_ops *ops)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int chipnr = 0, page = 0, page_in_chip = 0;
	int len = ops->len;
	int ooblen = ops->ooblen;
	int had_readlen = 0, had_read_ooblen = 0;
	int slen = min_t(uint32_t, ooblen, chip->avalid_sparesize);
	uint8_t *mdata = ops->datbuf;
	uint8_t *sdata = ops->oobbuf;
	unsigned int max_bitflips = 0;
	bool ecc_failed = false;
	struct aw_nand_chip_cache *buffer = &chip->simu_chip_buffer;
	uint8_t *pagebuf = buffer->simu_pagebuf;
	uint8_t *oobbuf = buffer->simu_oobbuf;
	loff_t col = from & chip->simu_pagesize_mask;
	/*int mlen = mtd->writesize - col;*/
	/*int mlen = min((mtd->writesize - col), len);*/
	int mlen = min_t(unsigned int, (mtd->writesize - col), len);

	awrawnand_mtd_trace("Enter %s [%llx:%lx]\n", __func__, from, ops->len);

	ops->retlen = ops->oobretlen = 0;

	chipnr = (int)(from >> chip->simu_chip_shift);
	page = (int)(from >> chip->simu_pagesize_shift);
	page_in_chip = page & chip->simu_chip_pages_mask;


	if (likely(!sdata))
		slen = 0;

	mutex_lock(&chip->lock);

	chip->select_chip(mtd, chipnr);

	do {

		/*if (page == buffer->pageno && mlen <= buffer->valid_page_len && slen == 0) {*/
		if ((page_in_chip == buffer->simu_oobno) &&
			(page_in_chip == buffer->simu_pageno) &&
			(col >= buffer->valid_col) &&
			((col + len) <= (buffer->valid_col + buffer->valid_len)) &&
			(ops->ooboffs >= buffer->valid_oob_col) &&
			((ops->ooboffs + ops->ooblen) <=
			 (buffer->valid_oob_col + buffer->valid_oob_len))) {

			memcpy(mdata, pagebuf + col, mlen);
			memcpy(sdata, oobbuf, slen);
			mtd->ecc_stats.failed = 0;
			max_bitflips = buffer->bitflips;

		} else {
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
			ret = chip->multi_read_page(mtd, chip, pagebuf, buffer->simu_page_len,
					oobbuf, buffer->simu_oob_len, page_in_chip);

#else
			ret = chip->read_page(mtd, chip, pagebuf, buffer->simu_page_len,
					oobbuf, buffer->simu_oob_len, page_in_chip);
#endif
			if (ret == ECC_LIMIT) {
				ret = mtd->bitflip_threshold;
				mtd->ecc_stats.corrected += chip->bitflips;
				max_bitflips = max_t(unsigned int, max_bitflips, ret);
				awrawnand_info("ecc limit from@%llx len@%zx in page@%d\n",
						from, ops->len, page);

			} else if (ret == ECC_ERR) {
				ecc_failed = true;
				mtd->ecc_stats.failed++;
				awrawnand_err("ecc err from@%llx len@%zx in page@%d &&\n",
						from, ops->len, page);
				awrawnand_err("main data len@%d\n", buffer->simu_page_len);
				dump_data(pagebuf, buffer->simu_page_len);
				awrawnand_err("spare len@%d\n", buffer->simu_oob_len);
				dump_data(oobbuf, buffer->simu_oob_len);
				buffer->simu_pageno = INVALID_CACHE;
				buffer->simu_oobno = INVALID_CACHE;
				buffer->chipno = INVALID_CACHE;

			} else if (ret < 0) {
				awrawnand_err("read from @%llx len@%zx fail in page@%d $$\n",
						from, ops->len, page);
				buffer->simu_pageno = INVALID_CACHE;
				buffer->simu_oobno = INVALID_CACHE;
				buffer->chipno = INVALID_CACHE;
				break;
			}

			mtd->ecc_stats.failed = 0;
			buffer->bitflips = max_bitflips;
			memcpy(mdata, pagebuf + col, mlen);
			memcpy(sdata, oobbuf, slen);

			buffer->simu_pageno = page_in_chip;
			buffer->simu_oobno = page_in_chip;
			buffer->valid_col = col;
			buffer->valid_len = mlen;
			buffer->valid_oob_col = 0;
			buffer->valid_oob_len = slen;
			buffer->chipno = chipnr;

		}

		had_readlen += mlen;
		len -= mlen;
		mdata += mlen;
		mlen = min_t(unsigned int, len, mtd->writesize);

		awrawnand_ubi_trace("had read len@%u\n", had_readlen);

		if (unlikely(slen)) {
			had_read_ooblen += slen;
			ooblen -= slen;
			if (unlikely(ooblen > 0))
				sdata += slen;
			else {
				slen = 0;
				sdata = NULL;
			}
		}

		awrawnand_ubi_trace("had read oob len@%u\n", had_read_ooblen);
		col = 0;
		page++;
		page_in_chip = page & chip->simu_chip_pages_mask;

		if (!page_in_chip) {
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, ++chipnr);
		}

	} while (mlen);

	/*
	 *buffer->pageno = INVALID_CACHE;
	 *buffer->oobno = INVALID_CACHE;
	 */

	chip->select_chip(mtd, -1);

	mutex_unlock(&chip->lock);

	ops->retlen = had_readlen;
	if (ops->oobbuf)
		ops->oobretlen = had_read_ooblen;

	/*mutex_unlock(&chip->lock);*/

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);

	if (ret < 0)
		return ret;

	if (ecc_failed)
		return -EBADMSG;

	return max_bitflips;
}

/**
 * aw_rawnand_mtd_write_oob - write data
 * @mtd: MTD device structure
 * @to: offset to write to
 * @ops: oob operation descrition structure
 * */
static int aw_rawnand_mtd_write_oob(struct mtd_info *mtd, loff_t to,
			   struct mtd_oob_ops *ops)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int chipnr = 0;
	int block = 0;
	int page = 0;
	int npage = 0;
	int page_end = 0;
	int page_in_chip = 0;
	int page_in_chip_pre = 0;
	int sub_page = 0;
	int sub_page_in_chip = 0;
	int write_sub_page = 0;
	int len = ops->len;
	int ooblen = ops->ooblen;
	uint8_t *mdata = ops->datbuf;
	uint8_t *sdata = ops->oobbuf;
	int slen = min_t(unsigned int, ops->ooblen, mtd->oobavail);
	struct aw_nand_chip_cache *buffer = &chip->simu_chip_buffer;
	uint8_t *pagebuf = buffer->simu_pagebuf;
	uint8_t *oobbuf = buffer->simu_oobbuf;
	int mlen = min_t(unsigned int, ops->len, mtd->writesize);
	int blockA = 0;
	int col = to & chip->simu_pagesize_mask;

	awrawnand_mtd_trace("Enter %s [%llx:%lx]\n", __func__, to, ops->len);
	if (check_ofs_mtd_oob_ops(mtd, to, ops, 1)) {
		awrawnand_mtd_trace("Exit %s \n", __func__);
		return -EINVAL;
	}

	block = to >> mtd->erasesize_shift;
	blockA = block << 1;
	if (to & mtd->erasesize_mask)
		sub_page = ((blockA + 1) << chip->pages_per_blk_shift);
	else
		sub_page = blockA << chip->pages_per_blk_shift;

	if (ops->len < chip->simu_pagesize)
		write_sub_page = 1;
	else {

		page = (int)(to >> chip->simu_pagesize_shift);
		page_in_chip_pre = page & chip->simu_chip_pages_mask;

		npage = (ops->len >> chip->simu_pagesize_shift);
		if (ops->len & chip->simu_pagesize_mask) {
			awrawnand_err("write len@%zu not align to simu pagesize\n", ops->len);
			npage++;
		}

		page_end = npage + page;
	}

	mutex_lock(&chip->lock);

	memcpy(pagebuf + col, mdata, mlen);

	if (likely(!sdata))
		slen = 0;
	else
		memcpy(oobbuf, sdata, slen);

	chipnr = (int)(to >> chip->simu_chip_shift);


	chip->select_chip(mtd, chipnr);


	if (write_sub_page) {
		sub_page_in_chip = sub_page & chip->chip_pages_mask;
		buffer->simu_pageno = (to >> chip->simu_pagesize_shift);
		buffer->simu_oobno = buffer->simu_pageno;
		buffer->valid_col = col;
		buffer->valid_len = mlen;
		buffer->valid_oob_col = 0;
		buffer->valid_oob_len = slen;
		buffer->chipno = chipnr;
		ret = chip->write_page(mtd, chip, pagebuf + col, buffer->sub_page_len,
				oobbuf, slen, sub_page_in_chip);
	} else {
		/*last page use program(0x10)*/
		for (; page < page_end - 1; page++) {

			page_in_chip = page & chip->simu_chip_pages_mask;

			/*check, if we cross next chip*/
			if ((!page_in_chip) && (page_in_chip != page_in_chip_pre)) {
				chip->select_chip(mtd, -1);
				chip->select_chip(mtd, ++chipnr);
			}

			page_in_chip++;
			/*block last page should use program instr 0x10
			 * (ONFI4.2: also can use 0x15(cache program instr), but
			 * most flash datasheet only describle 0x10)*/
			if (!(page_in_chip & chip->pages_per_blk_mask)) {
				page_in_chip--;

#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
				ret = chip->multi_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
						oobbuf, slen, page_in_chip);
#else
				ret = chip->write_page(mtd, chip, pagebuf, buffer->simu_page_len,
						oobbuf, slen, page_in_chip);
#endif
				if (ret) {
					awrawnand_err("write page@%d fail\n", page);
					ops->retlen = 0;
					goto out;
				}

			} else {

				page_in_chip--;
				if (RAWNAND_HAS_CACHEPROG(chip)) {
					ret = chip->cache_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
							oobbuf, slen, page_in_chip);
				} else {
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
					ret = chip->multi_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
							oobbuf, slen, page_in_chip);
#else
					ret = chip->write_page(mtd, chip, pagebuf, buffer->simu_page_len,
							oobbuf, slen, page_in_chip);
#endif
				}
				if (ret) {
					awrawnand_err("write page@%d fail\n", page);
					ops->retlen = 0;
					goto out;
				}
			}

			/*len -= mlen;*/
			col = 0;
			mdata += buffer->simu_page_len;
			/*mlen = min_t(unsigned int, len, mtd->writesize);*/
			memcpy(pagebuf, mdata, buffer->simu_page_len);

			if (unlikely(sdata)) {
				ooblen -= slen;
				if (likely(ooblen))
					sdata += slen;
				slen = min_t(uint32_t, ooblen, chip->avalid_sparesize);
				sdata = slen ? sdata : NULL;
				if (sdata)
					memcpy(oobbuf, sdata, slen);
				buffer->simu_oobno = page;
			}
		}

		page_in_chip = page & chip->simu_chip_pages_mask;

		if ((!page_in_chip) && (page_in_chip != page_in_chip_pre)) {
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, ++chipnr);
		}
		/*write last page, use program(0x10)*/
		buffer->simu_pageno = page_in_chip;
		buffer->valid_col = 0;
		buffer->valid_len = buffer->simu_page_len;
		buffer->simu_oobno = buffer->simu_pageno;
		buffer->valid_oob_col = 0;
		buffer->valid_oob_len = slen;
		buffer->chipno = chipnr;
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
		ret = chip->multi_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
				oobbuf, slen, page_in_chip);
#else
		ret = chip->write_page(mtd, chip, pagebuf, buffer->simu_page_len,
				oobbuf, slen, page_in_chip);

#endif

	}

out:

	/*
	 *buffer->pageno = INVALID_CACHE;
	 *buffer->oobno = INVALID_CACHE;
	 */

	chip->select_chip(mtd, -1);

	mutex_unlock(&chip->lock);

	if (ret) {
		awrawnand_err("write page@%d fail\n", page);
		ops->retlen = 0;
		ops->oobretlen = 0;
		buffer->simu_pageno = INVALID_CACHE;
		buffer->simu_oobno = INVALID_CACHE;
	} else {
		ops->retlen = len;
		if (unlikely(sdata))
			ops->oobretlen = slen;
	}

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}

/**
 * aw_rawnand_mtd_write - write data without oob
 * @mtd: mtd device structure
 * @to: offset to write to
 * @len: want to write data len
 * @retlen: had writen len
 * @buf: data buffer
 * */
static int aw_rawnand_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
		       size_t *retlen, const u_char *buf)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int chipnr = 0;
	int block = 0;
	int page = 0;
	int npage = 0;
	int page_end = 0;
	int page_in_chip = 0;
	int page_in_chip_pre = 0;
	int sub_page = 0;
	int sub_page_in_chip = 0;
	int write_sub_page = 0;
	int wlen = len;
	uint8_t *mdata = (uint8_t *)buf;
	struct aw_nand_chip_cache *buffer = &chip->simu_chip_buffer;
	uint8_t *pagebuf = buffer->simu_pagebuf;
	int mlen = min_t(unsigned int, wlen, mtd->writesize);
	int blockA = 0;
	int col = (to & chip->simu_pagesize_mask);


	awrawnand_mtd_trace("Enter %s [%llx:%lx]\n", __func__, to, len);

	if (check_to_len(mtd, to, len)) {
		return -EINVAL;
	}


	block = to >> mtd->erasesize_shift;
	blockA = block << 1;
	if (to & mtd->erasesize_mask)
		sub_page = ((blockA + 1) << chip->pages_per_blk_shift);
	else
		sub_page = blockA << chip->pages_per_blk_shift;

	if (wlen < chip->simu_pagesize)
		write_sub_page = 1;
	else {

		page = (int)(to >> chip->simu_pagesize_shift);
		page_in_chip_pre = page & chip->simu_chip_pages_mask;

		npage = (wlen >> chip->simu_pagesize_shift);
		if (wlen & chip->simu_pagesize_mask) {
			awrawnand_err("write len@%d not align to simu pagesize\n", wlen);
			npage++;
		}

		page_end = npage + page;
	}

	chipnr = (int)(to >> chip->simu_chip_shift);

	mutex_lock(&chip->lock);

	chip->select_chip(mtd, chipnr);

	memcpy(pagebuf + col, mdata, mlen);

	if (write_sub_page) {
		sub_page_in_chip = sub_page & chip->chip_pages_mask;
		/*buffer->pageno = INVALID_CACHE;*/
		buffer->simu_pageno = (to >> chip->simu_pagesize_shift);
		buffer->simu_oobno = INVALID_CACHE;
		buffer->valid_col = col;
		buffer->valid_len = mlen;
		buffer->chipno = chipnr;
		ret = chip->write_page(mtd, chip, pagebuf + col, buffer->sub_page_len,
				NULL, 0, sub_page_in_chip);
	} else {
		/*last page use program(0x10)*/
		for (; page < page_end - 1; page++) {

			page_in_chip = page & chip->simu_chip_pages_mask;

			/*check, if we cross next chip*/
			if ((!page_in_chip) && (page_in_chip != page_in_chip_pre)) {
				chip->select_chip(mtd, -1);
				chip->select_chip(mtd, ++chipnr);
			}

			page_in_chip++;
			/*block last page should use program instr 0x10
			 * (ONFI4.2: also can use 0x15(cache program instr), but
			 * most flash datasheet only describle 0x10)*/
			if (!(page_in_chip & chip->pages_per_blk_mask)) {
				page_in_chip--;
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
				ret = chip->multi_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
						NULL, 0, page_in_chip);
#else
				ret = chip->write_page(mtd, chip, pagebuf, buffer->simu_page_len,
						NULL, 0, page_in_chip);
#endif
				if (ret) {
					awrawnand_err("write page@%d fail\n", page);
					*retlen = 0;
					goto out;
				}

			} else {

				page_in_chip--;
				if (RAWNAND_HAS_CACHEPROG(chip)) {
					ret = chip->cache_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
							NULL, 0, page_in_chip);
				} else {
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
					ret = chip->multi_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
							NULL, 0, page_in_chip);
#else
					ret = chip->write_page(mtd, chip, pagebuf, buffer->simu_page_len,
							NULL, 0, page_in_chip);
#endif
				}
				if (ret) {
					awrawnand_err("write page@%d fail\n", page);
					*retlen = 0;
					goto out;
				}
			}

			col = 0;
			mdata += buffer->simu_page_len;
			memcpy(pagebuf, mdata, buffer->simu_page_len);

		}

		page_in_chip = page & chip->simu_chip_pages_mask;

		if ((!page_in_chip) && (page_in_chip != page_in_chip_pre)) {
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, ++chipnr);
		}
		/*write last page, use program(0x10)*/
		buffer->simu_pageno = page_in_chip;
		buffer->valid_col = 0;
		buffer->valid_len = buffer->simu_page_len;
		buffer->chipno = chipnr;
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
		ret = chip->multi_write_page(mtd, chip, pagebuf, buffer->simu_page_len,
				NULL, 0, page_in_chip);
#else
		ret = chip->write_page(mtd, chip, pagebuf, buffer->simu_page_len,
				NULL, 0, page_in_chip);

#endif

	}

out:
	/*buffer->pageno = INVALID_CACHE;*/

	chip->select_chip(mtd, -1);

	mutex_unlock(&chip->lock);

	if (ret) {
		awrawnand_err("write page@%d fail\n", page);
		*retlen = 0;
		buffer->simu_pageno = INVALID_CACHE;
		buffer->simu_oobno = INVALID_CACHE;
	} else {
		*retlen = wlen;
	}

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;

}

static void aw_rawnand_mtd_sync(struct mtd_info *mtd)
{

}

#ifdef TODO
static int aw_rawnand_mtd_block_isreserved(struct mtd_info *mtd, loff_t ofs)
{
	int ret = 0;

	return ret;
}
#endif

/**
 * aw_rawnand_mtd_block_isbad - check block is badblock or not
 * @mtd: MTD device structure
 * @ofs: offset the device start
 */
static int aw_rawnand_mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int c = 0;
	int b = 0;


	awrawnand_mtd_trace("Enter %s [%llx]\n", __func__, ofs);

	if (ofs < chip->uboot_end) {
		awrawnand_mtd_trace("Exit %s\n", __func__);
		return 0;
	}

	c = (int)(ofs >> chip->simu_chip_shift);
	b = (int)(ofs >> chip->simu_erase_shift);


	mutex_lock(&chip->lock);
	chip->select_chip(mtd, c);

	ret = chip->simu_block_bad(mtd, b);

	chip->select_chip(mtd, -1);
	mutex_unlock(&chip->lock);

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}


/**
 * aw_rawnand_mtd_block_markbad - mark block at the given offset as bad block
 * @mtd: MTD device structure
 * @ofs: offset the device start
 * */
static int aw_rawnand_mtd_block_markbad(struct mtd_info *mtd, loff_t ofs)
{

	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	int ret = 0;
	int c = 0;
	int b = 0;

	awrawnand_mtd_trace("Enter %s [%llx]\n", __func__, ofs);

	c = (int)(ofs >> chip->simu_chip_shift);
	b = (int)(ofs >> chip->simu_erase_shift);

	mutex_lock(&chip->lock);
	chip->select_chip(mtd, c);

	ret = chip->simu_block_markbad(mtd, b);

	chip->select_chip(mtd, -1);
	mutex_unlock(&chip->lock);

	awrawnand_mtd_trace("Exit %s ret@%d\n", __func__, ret);
	return ret;
}

/*
 *#ifdef CONFIG_PM_SLEEP
 *static int aw_rawnand_mtd_suspend (struct mtd_info *mtd)
 *{
 *        int ret = 0;
 *
 *        return ret;
 *}
 *
 *static void aw_rawnand_mtd_resume(struct mtd_info *mtd)
 *{
 *
 *}
 *
 *static void aw_rawnand_mtd_reboot(struct mtd_info *mtd)
 *{
 *
 *
 *}
 *#endif
 */


static enum rawnand_data_interface_type aw_rawnand_get_itf_type(struct aw_nand_chip *chip)
{
	if (RAWNAND_HAS_ITF_SDR(chip))
		return RAWNAND_SDR_IFACE;

	if (RAWNAND_HAS_ITF_ONFI_DDR(chip) || RAWNAND_HAS_ITF_ONFI_DDR2(chip))
		return RAWNAND_ONFI_DDR;

	if (RAWNAND_HAS_ITF_TOGGLE_DDR(chip) || RAWNAND_HAS_ITF_TOGGLE_DDR2(chip))
		return RAWNAND_TOGGLE_DDR;

	return RAWNAND_SDR_IFACE;
}

static int aw_rawnand_chip_data_init(struct mtd_info *mtd)
{

	/*initialize chip data that not yet been completed */
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_flash_dev *dev = chip->dev;
	int i = 0;
	int ecc_bits = 0;
	int bbtsize = 0;
	int uboot_end = 0;

	chip->type = SLC_NAND;

	chip->ecc_mode = aw_rawnand_get_ecc_mode(dev->sparesize, dev->pagesize);
	if (chip->ecc_mode == BCH_NO) {
		awrawnand_err("get ecc mode err pls check id table\n");
		goto out;
	}

	if (aw_rawnand_ecc_mode_test(dev->sparesize, dev->pagesize, chip->ecc_mode,
				&chip->ecc_mode)) {
		awrawnand_err("ecc mode test fail\n");
		goto out;
	}


	chip->dies = dev->dies_per_chip;
	for (i = 0; i < chip->dies; i++) {
		chip->diesize[i] = dev->blks_per_die * dev->pages_per_blk;
		chip->diesize[i] *= dev->pagesize;
		chip->chipsize += chip->diesize[i];
	}

	chip->simu_chipsize = chip->chipsize;

	if (chip->chipsize <= 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else {
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32));
		chip->chip_shift += 32 - 1;
	}
	chip->simu_chip_shift = chip->chip_shift;
	chip->pages_per_blk_shift = ffs(dev->pages_per_blk) -1;
	chip->simu_pages_per_blk_shift = chip->pages_per_blk_shift;
	chip->pages_per_blk_mask = ((1 << chip->pages_per_blk_shift) - 1);
	chip->simu_pages_per_blk_mask = chip->pages_per_blk_mask;
	chip->real_pagesize = dev->pagesize + dev->sparesize;
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
	chip->simu_pagesize = (dev->pagesize << 1);
#else
	chip->simu_pagesize = dev->pagesize;
#endif
	chip->simu_pagesize_mask = chip->simu_pagesize - 1;
	chip->simu_pagesize_shift = ffs(chip->simu_pagesize) - 1;
	chip->simu_erase_shift = ffs(chip->simu_pagesize << chip->simu_pages_per_blk_shift) - 1;
	chip->simu_erasesize = 1 << chip->simu_erase_shift;
	chip->simu_erasesize_mask = (1 << chip->simu_erase_shift) - 1;
	chip->simu_chip_pages = (1 << (chip->simu_chip_shift - chip->simu_pagesize_shift));
	chip->simu_chip_pages_mask = ((1 << (chip->simu_chip_shift - chip->simu_pagesize_shift)) - 1);

	chip->pagesize = dev->pagesize;
	chip->pagesize_mask = chip->pagesize - 1;
	chip->pagesize_shift = ffs(chip->pagesize) - 1;
	chip->erase_shift = ffs((chip->pagesize << chip->pages_per_blk_shift)) - 1;
	chip->erasesize = 1 << chip->erase_shift;
	chip->erasesize_mask = (1 << chip->erase_shift) - 1;
	chip->chip_pages = (1 << (chip->chip_shift - chip->pagesize_shift));
	chip->chip_pages_mask = ((1 << (chip->chip_shift - chip->pagesize_shift)) - 1);

	ecc_bits = aw_rawnand_get_ecc_bits(chip->ecc_mode);
	if (ecc_bits == -1)
		goto out;


	chip->avalid_sparesize = dev->sparesize - ((B_TO_KB(1 << chip->pagesize_shift) * ecc_bits * 14) / 8);
	/*4 is a ecc block user data len minimum multiple*/
	chip->avalid_sparesize = round_down(chip->avalid_sparesize, 4);
	chip->avalid_sparesize = min(chip->avalid_sparesize, MAX_SPARE_SIZE);
	chip->options = dev->options;
	chip->clk_rate = dev->access_freq;
	chip->data_interface.type = aw_rawnand_get_itf_type(chip);

	/*chip->random = RAWNAND_NFC_NEED_RANDOM(chip);*/
	/*random data would be friendly to flash, default enable*/
	chip->random = 1;

	if (RAWNAND_HAS_ONLY_TWO_ADDR(chip))
		chip->row_cycles = 2;
	else
		chip->row_cycles = 3;
	chip->badblock_mark_pos = dev->badblock_flag_pos;

	bbtsize = ((chip->chips * chip->chipsize) >> chip->erase_shift);
	bbtsize = round_up(bbtsize, 8);
	bbtsize = bbtsize >> 3;

	chip->bbt = kzalloc(bbtsize, GFP_KERNEL);
	if (!chip->bbt) {
		awrawnand_err("kzalloc bbt fail\n");
		return -ENOMEM;
	}

	chip->bbtd = kzalloc(bbtsize, GFP_KERNEL);
	if (!chip->bbtd) {
		kfree(chip->bbt);
		awrawnand_err("kzalloc bbtd fail\n");
		return -ENOMEM;
	}
#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
	chip->simu_chip_buffer.simu_page_len = (chip->pagesize << 1);
#else
	chip->simu_chip_buffer.simu_page_len = chip->pagesize;

#endif
	chip->simu_chip_buffer.sub_page_len = chip->pagesize;

	chip->simu_chip_buffer.simu_pagebuf = kmalloc(chip->simu_chip_buffer.simu_page_len, GFP_KERNEL);
	if (!chip->simu_chip_buffer.simu_pagebuf) {
		awrawnand_err("kzalloc pagebuf fail\n");
		kfree(chip->bbt);
		kfree(chip->bbtd);
		return -ENOMEM;
	}
	memset(chip->simu_chip_buffer.simu_pagebuf, 0, chip->simu_chip_buffer.simu_page_len);

#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
	chip->simu_chip_buffer.simu_oob_len = chip->avalid_sparesize << 1;
#else
	chip->simu_chip_buffer.simu_oob_len = chip->avalid_sparesize;
#endif
	chip->simu_chip_buffer.simu_oobbuf = kmalloc(chip->simu_chip_buffer.simu_oob_len, GFP_KERNEL);
	if (!chip->simu_chip_buffer.simu_oobbuf) {
		awrawnand_err("kzalloc oobbuf fail\n");
		kfree(chip->bbt);
		kfree(chip->bbtd);
		kfree(chip->simu_chip_buffer.simu_pagebuf);
		return -ENOMEM;
	}

	memset(chip->simu_chip_buffer.simu_oobbuf, 0, chip->simu_chip_buffer.simu_oob_len);
	chip->simu_chip_buffer.simu_pageno = INVALID_CACHE;
	chip->simu_chip_buffer.simu_oobno = INVALID_CACHE;
	chip->simu_chip_buffer.bitflips = 0;

	if (chip->type == SLC_NAND) {
		chip->write_boot0_page = rawslcnand_write_boot0_page;
		chip->read_boot0_page = rawslcnand_read_boot0_page;
	}

	rawnand_uboot_blknum(NULL, &uboot_end);
	chip->uboot_end = uboot_end << chip->erase_shift;

	mutex_init(&chip->lock);

	awrawnand_info("chip: chip_shift@%d\n", chip->chip_shift);
	awrawnand_info("chip: pagesize_mask@%d\n", chip->pagesize_mask);
	awrawnand_info("chip: chip_pages@%d\n", chip->chip_pages);
	awrawnand_info("chip: pagesize_shift@%d\n", chip->pagesize_shift);
	awrawnand_info("chip: erase_shift@%d\n", chip->erase_shift);
	awrawnand_info("chip: ecc_mode@%d\n", chip->ecc_mode);
	awrawnand_info("chip: clk_rate@%d(MHz)\n", chip->clk_rate);
	awrawnand_info("chip: avalid_sparesize@%d\n", chip->avalid_sparesize);
	awrawnand_info("chip: pages_per_blk_shift_shift@%d\n", chip->pages_per_blk_shift);
	return 0;
out:
	return -EINVAL;
}

static void aw_rawnand_chip_data_destroy(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);

	kfree(chip->bbt);
	kfree(chip->bbtd);
	kfree(chip->simu_chip_buffer.simu_pagebuf);
	kfree(chip->simu_chip_buffer.simu_oobbuf);
}

static int aw_rawnand_mtd_info_init(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	/*initialize mtd data*/
	mtd->type = MTD_NANDFLASH; /*SLCNAND*/
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->owner = THIS_MODULE;

	mtd->erasesize = 1 << chip->simu_erase_shift;
	mtd->erasesize_shift = ffs(mtd->erasesize) - 1;
	mtd->erasesize_mask = mtd->erasesize - 1;

	mtd->writesize = 1 << chip->simu_pagesize_shift;
	mtd->writesize_shift = ffs(mtd->writesize) - 1;
	mtd->writesize_mask = mtd->writesize - 1;
	mtd->writebufsize = mtd->writesize;

	mtd->oobsize = chip->avalid_sparesize;
	mtd->oobavail = chip->avalid_sparesize;


#if IS_ENABLED(CONFIG_SIMULATE_MULTIPLANE)
	/*In order to don't change user application(e.g. ubi), keep the same with aw spi nand*/
	mtd->subpage_sft = 1;
#else
	mtd->subpage_sft = 0;
#endif

	/*mtd->subpage_sft = 0;*/

	mtd->size = (uint64_t)chip->chips << chip->chip_shift;
	mtd->bitflip_threshold = ecc_limit_tab[chip->ecc_mode];
	mtd->name = "nand";
	mtd->ecc_strength = ecc_bits_tbl[chip->ecc_mode];


	mtd->_erase = aw_rawnand_mtd_erase;
	mtd->_read = aw_rawnand_mtd_read;
	mtd->_read_oob = aw_rawnand_mtd_read_oob;
	mtd->_write = aw_rawnand_mtd_write;
	mtd->_write_oob = aw_rawnand_mtd_write_oob;
	mtd->_sync = aw_rawnand_mtd_sync;
#ifdef TODO
	/*can use to do boot0&uboot0 img region*/
	mtd->_block_isreserved = aw_rawnand_mtd_block_isreserved;
#endif
	mtd->_block_isbad = aw_rawnand_mtd_block_isbad;
	mtd->_block_markbad = aw_rawnand_mtd_block_markbad;
	mtd->_panic_write = aw_rawnand_mtd_panic_write;
/*
 *#ifdef CONFIG_PM_SLEEP
 *        mtd->_suspend = aw_rawnand_mtd_suspend;
 *        mtd->_resume = aw_rawnand_mtd_resume;
 *        mtd->_reboot = aw_rawnand_mtd_reboot;
 *#endif
 */

	awrawnand_info("mtd : type@%s\n", mtd->type == MTD_MLCNANDFLASH ? "MLCNAND" : "SLCNAND");
	awrawnand_info("mtd : flags@nand flash\n");
	awrawnand_info("mtd : writesize@%u\n", mtd->writesize);
	awrawnand_info("mtd : oobsize@%d\n", mtd->oobsize);
	awrawnand_info("mtd : size@%llu\n", mtd->size);
	awrawnand_info("mtd : bitflips_threshold@%u\n", mtd->bitflip_threshold);
	awrawnand_info("mtd : ecc_strength@%u\n", mtd->ecc_strength);

	return 0;
}


static int aw_rawnand_scan_tail(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	struct aw_nand_host *host = awnand_chip_to_host(chip);

	int ret = 0;

	ret = aw_rawnand_chip_data_init(mtd);
	if (ret)
		goto out_chip_data_fail;

	ret = aw_rawnand_mtd_info_init(mtd);
	if (ret)
		goto out_mtd_info_fail;

	ret = aw_host_init_tail(host);

	return ret;

out_mtd_info_fail:
	awrawnand_err("mtd info init fail\n");
	return ret;

out_chip_data_fail:
	awrawnand_err("chip data init fail\n");
	return ret;
}

/*return @0: not protected
 *	@1: protected
 **/
static int aw_rawnand_check_wp(struct mtd_info *mtd)
{
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);
	uint8_t status = 0;

	chip->select_chip(mtd, 0);

	if (chip->dev_ready_wait(mtd)) {
		status = chip->dev_status(mtd);

	} else
		awrawnand_err("dev is busy, check wp fail\n");

	chip->select_chip(mtd, -1);

	return (status & RAWNAND_STATUS_WP) ? 0 : 1;
}

static int aw_rawnand_scan(struct mtd_info *mtd)
{
	int ret = 0;
	struct aw_nand_chip *chip = awnand_mtd_to_chip(mtd);


	aw_rawnand_chip_init_pre(chip);

	ret = aw_rawnand_scan_device(&chip->mtd);
	if (ret)
		awrawnand_err("aw rawnand scan fail@%d\n", ret);
	else
		ret = aw_rawnand_scan_tail(&chip->mtd);

	if (aw_rawnand_check_wp(mtd)) {
		awrawnand_err("device is in wp status,err@%d\n", -EIO);
		ret = -EIO;
	}

	return ret;
}

static struct mtd_partition aw_rawnand_parts[] = {
	/* .size is set by @aw_rawnand_mtd_update_mtd_parts */
	{ .name = "boot0", .offset = 0 },
	{ .name = "uboot", .offset = MTDPART_OFS_APPEND },
	{ .name = "secure_storage", .offset = MTDPART_OFS_APPEND },
#if IS_ENABLED(CONFIG_AW_RAWNAND_PSTORE_MTD_PART)
	{ .name = "pstore", .offset = MTDPART_OFS_APPEND },
#endif
#if IS_ENABLED(CONFIG_RAW_KERNEL)
	{ .name = "kernel", .offset = MTDPART_OFS_APPEND },
#endif
	{ .name = "sys", .offset = MTDPART_OFS_APPEND},
};

static void aw_rawnand_mtd_update_mtd_parts(struct aw_nand_chip *chip,
		struct mtd_partition *mtdparts)
{
	unsigned int uboot_start, uboot_end;
	int index = 0;
	int phy_blk_bytes = chip->erasesize;

	rawnand_uboot_blknum(&uboot_start, &uboot_end);
	chip->uboot_end = uboot_end * phy_blk_bytes;

	/* boot0 */
	mtdparts[index++].size = uboot_start * phy_blk_bytes;
	/* uboot */
	mtdparts[index++].size = (uboot_end - uboot_start) * phy_blk_bytes;
	/* secure storage */
	mtdparts[index++].size = PHY_BLKS_FOR_SECURE_STORAGE * phy_blk_bytes;
#if IS_ENABLED(CONFIG_AW_RAWNAND_PSTORE_MTD_PART)
	/* pstore */
	mtdparts[index++].size = PSTORE_SIZE_KB * SZ_1K;
#endif

#if IS_ENABLED(CONFIG_RAW_KERNEL)
	/* kernel */
	if (CONFIG_KERNEL_SIZE_BYTE)
		mtdparts[index++].size =
			ALIGN(CONFIG_KERNEL_SIZE_BYTE, phy_blk_bytes * 2);
	else
		mtdparts[index++].size = phy_blk_bytes * 2;
#endif
	/* user data */
	mtdparts[index++].size = MTDPART_SIZ_FULL;

#if IS_ENABLED(CONFIG_AW_RAWNAND_SECURE_STORAGE)
	struct aw_nand_sec_sto *sec_sto = get_rawnand_sec_sto();
	sec_sto->chip = chip;
	sec_sto->startblk = uboot_end;
	sec_sto->endblk = uboot_end + PHY_BLKS_FOR_SECURE_STORAGE;
#endif
}

static int aw_rawnand_ota_open(struct inode *inode, struct file *file)
{
	awrawnand_dbg("Rawnand cdev opened\n");
	return 0;
}

static int aw_rawnand_ota_release(struct inode *inode, struct file *file)
{
	awrawnand_dbg("Rawnand cdev released\n");
	return 0;
}

static ssize_t aw_rawnand_ota_boot(struct file *file, const char __user *buffer,
		size_t length, loff_t *offset)
{
	struct burn_param_t burn_param;
	char *ota_buf = NULL;
	ssize_t ret = 0;

	ret = copy_from_user(&burn_param, buffer, length);
	if (ret) {
		awrawnand_err("nand ioctl input buffer err\n");
		return -EFAULT;
	}

	ota_buf = vmalloc(burn_param.length);
	if (IS_ERR_OR_NULL(ota_buf)) {
		awrawnand_err("Error: vmalloc failed.\n");
		return -ENOMEM;
	}

	ret = copy_from_user(ota_buf, burn_param.buffer, burn_param.length);
	if (ret) {
		awrawnand_err("nand ioctl input uer buffer err\n");
		vfree(ota_buf);
		return -EFAULT;
	}

	switch (burn_param.type) {
	case BOOT0:
		aw_rawnand_mtd_download_boot0(burn_param.length, (void *)ota_buf);
		break;
	case UBOOT:
		aw_rawnand_mtd_download_uboot(burn_param.length, (void *)ota_buf);
		break;
	case SECURE_STORAGE:
		awrawnand_err("OTA SECURE_STORAGE is not implemented\n");
		break;
	default:
		awrawnand_err("Not aware of other ota types\n");
	}

	vfree(ota_buf);

	return length;
}

static struct file_operations ota_fops = {
	.write = aw_rawnand_ota_boot,
	.open = aw_rawnand_ota_open,
	.release = aw_rawnand_ota_release,
};

static int aw_rawnand_create_cdev(void)
{
	int ret = 0;

	major_number = register_chrdev(0, "rawnand_cdev", &ota_fops);
	if (major_number < 0) {
		awrawnand_err("Failed to register the device@%d\n", major_number);
		ret = major_number;
		goto out;
	}

	rawnand_class = class_create(THIS_MODULE, "rawnand_class");
	if (IS_ERR(rawnand_class)) {
		awrawnand_err("Failed to create the class\n");
		ret = PTR_ERR(rawnand_class);
		goto unregister_chrdev;
	}

	rawnand_device = device_create(rawnand_class, NULL,
			MKDEV(major_number, 0), NULL, "rawnand_cdev");
	if (IS_ERR(rawnand_device)) {
		awrawnand_err("Failed to create the device\n");
		ret = PTR_ERR(rawnand_device);
		goto class_destroy;
	}

	return ret;

class_destroy:
	class_destroy(rawnand_class);
unregister_chrdev:
	unregister_chrdev(major_number, "rawnand_cdev");
out:
	return ret;
}

static void aw_rawnand_remove_cdev(void)
{
	device_destroy(rawnand_class, MKDEV(major_number, 0));
	class_destroy(rawnand_class);
	unregister_chrdev(major_number, "rawnand_cdev");
}

static int aw_rawnand_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct aw_nand_chip *chip = get_rawnand();
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	struct device *dev = &pdev->dev;
	struct device_node *chosen_node;
	const char *bootargs;

	memset(&awnand_chip, 0, sizeof(struct aw_nand_chip));
	awnand_chip.priv = &aw_host;

	DISPLAY_VERSION;

	ret = aw_host_init(dev);
	if (ret) {
		awrawnand_err("host init fail@%d\n", ret);
		goto out;
	}

	ret = aw_rawnand_scan(mtd);
	if (ret) {
		awrawnand_err("scan fail\n");
		goto out;
	}

	aw_rawnand_mtd_update_mtd_parts(chip, aw_rawnand_parts);

	chosen_node = of_find_node_by_path("/chosen");
	if (chosen_node) {
		ret = of_property_read_string(chosen_node, "bootargs", &bootargs);
		if (ret) {
			awrawnand_err("Failed to get bootargs\n");
			ret = 1;
		}
	} else {
			awrawnand_err("Failed to get choosen node\n");
			ret = 1;
	}
#if IS_ENABLED(CONFIG_MTD_CMDLINE_PARTS)
	if (!ret && get_para_from_cmdline(bootargs, "mtdparts") > 0)
		ret = mtd_device_register(mtd, NULL, 0);
	else
#else
		ret = mtd_device_register(mtd, aw_rawnand_parts,
				ARRAY_SIZE(aw_rawnand_parts));
#endif
	if (ret)
		goto out;

	platform_set_drvdata(pdev, chip);

	aw_rawnand_create_cdev();

	return ret;

out:
	aw_host_exit(awnand_chip.priv);
	memset(&awnand_chip, 0, sizeof(struct aw_nand_chip));
	awrawnand_err("init fail\n");
	return ret;
}

static int aw_rawnand_remove(struct platform_device *pdev)
{

	struct aw_nand_chip *chip = platform_get_drvdata(pdev);
	struct mtd_info *mtd = awnand_chip_to_mtd(chip);
	struct aw_nand_host *host = awnand_chip_to_host(chip);

#ifndef __UBOOT__
	if (mtd_device_unregister(mtd))
		return -1;
#endif

	aw_host_exit(host);

	aw_rawnand_chip_data_destroy(mtd);

	aw_rawnand_remove_cdev();

	return 0;
}

static const struct of_device_id of_nand_id[] = {
    { .compatible = "allwinner,sun50iw9-nand"},
    { .compatible = "allwinner,sun50iw10-nand"},
    { .compatible = "allwinner,sun50iw11-nand"},
    { .compatible = "allwinner,sun8iw19-nand"},
    { .compatible = "allwinner,sun8iw11-nand"},
    { .compatible = "allwinner,sun55iw3-nand"},
    {/* sentinel */},
};

static struct platform_driver awrawnand_driver = {
    .probe = aw_rawnand_probe,
    .remove = aw_rawnand_remove,
    .driver = {
	.name = "aw_rawnand",
	.owner = THIS_MODULE,
	.of_match_table = of_nand_id,
    },
};

static int __init awnand_init(void)
{
	return platform_driver_register(&awrawnand_driver);
}

static void __exit awnand_exit(void)
{
	platform_driver_unregister(&awrawnand_driver);
}


module_init(awnand_init);
module_exit(awnand_exit);

MODULE_ALIAS(DRIVER_NAME);
MODULE_DESCRIPTION("Allwinner rawnand driver");
MODULE_AUTHOR("cuizhikui <cuizhikui@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(SUNXI_RAWNAND_MODULE_VERSION);
