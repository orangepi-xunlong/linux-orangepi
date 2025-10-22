/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* secure no-voliate memory(nand/emmc) driver for sunxi platform
 *
 * Copyright (C) 2014 Allwinner Ltd.
 *
 * Author:
 *	Ryan Chen <yanjianbo@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/crc32.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/blk_types.h>
#include <linux/bio.h>
#include <linux/blkdev.h>
#include <linux/pagemap.h>
#include "secure_storage.h"

#define AW_SECURE_STORAGE_DEBUG		0
#define SEC_BLK_SIZE			(4096)
#define MAX_SECURE_STORAGE_MAX_ITEM	(32)
/*
 * EMMC parameters
 */
#define SDMMC_SECTOR_SIZE		(512)
#define SDMMC_SECURE_STORAGE_START_ADD  (6*1024*1024/512)
#define SDMMC_ITEM_SIZE               	(4*1024/512)

static unsigned int  secure_storage_inited;
static char *sd_oem_path = "/dev/block/mmcblk0";
static char *sd_oem_path_for_linux = "/dev/mmcblk0";

/*
 * Nand parameters
 */
static char *nand_oem_path = "/dev/block/nand0";

#ifdef CONFIG_AW_SECURE_STORAGE_BY_CHAR_DEVICE
static char *nand_oem_path_for_linux = "/dev/mtd2";
#else
static char *nand_oem_path_for_linux = "/dev/nand0";
#endif
static char *nand_oem_path2 = "/dev/block/nanda";
static char *nand_oem_path2_for_linux = "/dev/nanda";

static struct secblc_op_t secblk_op;
static fcry_pt nfcr;
/*
 *  secure storage map
 *
 *  section 0:
 *		name1:length1
 *		name2:length2
 *			...
 *	section 1:
 *		data1 ( name1 data )
 *	section 2 :
 *		data2 ( name2 data )
 *			...
 */

#define FLASH_TYPE_NAND  0
#define FLASH_TYPE_SD1  1
#define FLASH_TYPE_SD2  2
#define FLASH_TYPE_UNKNOW	-1

static int flash_boot_type = FLASH_TYPE_UNKNOW;

#define SST_STORAGE_READ	_IO('V', 1)
#define SST_STORAGE_WRITE	_IO('V', 2)

void mem_dump(void *addr, unsigned int size)
{
	int j;
	char *buf = (char *)addr;

	for (j = 0; j < size; j++) {
		printk("%02x ", buf[j] & 0xff);
		if (j%15 == 0 && j) {
			printk("\n");
		}
	}
	printk("\n");
	return ;
}

int _sst_user_ioctl(char *filename, int ioctl, void *param)
{
	struct file *fd;
	int ret = -1;

	if (!filename) {
		pr_err("- filename NULL\n");
		return -1;
	}
#ifdef CONFIG_AW_SECURE_STORAGE_BY_CHAR_DEVICE
	fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);
#else
	fd = filp_open_block(filename, O_WRONLY|O_CREAT, 0666);
#endif
	if (IS_ERR(fd)) {
		pr_err(" -file open fail %s\n", filename);
		return -1;
	}
	do {
		if ((fd->f_op == NULL) || (fd->f_op->unlocked_ioctl == NULL)) {
			pr_info(" -file can't to write!!\n");
			break;
		}

		ret = fd->f_op->unlocked_ioctl(
				fd,
				ioctl,
				(unsigned long)(param));

	} while (false);

	filp_close(fd, NULL);

	return ret;
}

static int _flush_secure_storage_data(int nr_page, struct bio *bio,
				      struct page *page)
{
	int i;
	for (i = 0; i < nr_page; i++) {
		bio_add_page(bio, page + i * PAGE_SIZE, PAGE_SIZE, 0);
		pr_debug("i:%d, nr_page:%d, page:%px\n", i, nr_page, page);
	}

	return submit_bio_wait(bio);
}

/* Read up to four pages of data */
#define MAX_PAGES   4

static int _sst_user_read_and_write(char *filename, char *buf, ssize_t len,
				    int offset, int op)
{
	struct bio *bio = NULL;
	struct page *page = NULL;
	struct block_device *bdevp = NULL;
	sector_t blk_addr = 0;
	int nr_page, all_page_size;
	int ret = -1;

	blk_addr = (offset + 511) / 512;
	nr_page = (len + PAGE_SIZE - 1) / PAGE_SIZE;
	all_page_size = nr_page * PAGE_SIZE;

	if (nr_page > MAX_PAGES) {
		pr_err("The size of the key should not exceed %d page\n",
		       MAX_PAGES);
		goto err;
	}

	/* try device */
	bdevp = blkdev_get_by_path(filename, FMODE_READ | FMODE_WRITE, NULL);
	if (IS_ERR(bdevp)) {
		pr_debug("\ndevice %s error %ld\n", filename, PTR_ERR(bdevp));
		goto err;
	}

	bio = bio_alloc(GFP_KERNEL, 1);
	if (!bio) {
		pr_err("Couldn't alloc bio\n");
		goto err;
	}
	page = alloc_pages(GFP_KERNEL, get_order(nr_page * PAGE_SIZE));
	if (!page) {
		pr_err("Couldn't alloc page\n");
		goto err;
	}
	bio->bi_iter.bi_sector = blk_addr;
	bio_set_dev(bio, bdevp);

	switch (op) {
	case REQ_OP_READ:
		bio_set_op_attrs(bio, REQ_OP_READ, 0);

		ret = _flush_secure_storage_data(nr_page, bio, page);
		if (ret)
			goto err;

		memcpy(buf, page_address(page), all_page_size);
		break;
	case REQ_OP_WRITE:
		bio_set_op_attrs(bio, REQ_OP_WRITE, REQ_SYNC);
		memcpy(page_address(page), buf, all_page_size);

		ret = _flush_secure_storage_data(nr_page, bio, page);
		if (ret)
			goto err;
		break;
	default:
		pr_err(" There is no such option :%d \n", op);
	}

	ret = all_page_size;
err:
	if (bio)
		bio_put(bio);
	if (page)
		__free_page(page);
	return ret;
}

static int get_para_from_cmdline(const char *cmdline, const char *name, char *value)
{
	char *value_p = value;

	if (!cmdline || !name || !value) {
		return -1;
	}

	for (; *cmdline != 0;) {
		if (*cmdline++ == ' ') {
			if (0 == strncmp(cmdline, name, strlen(name))) {
				cmdline += strlen(name);
				if (*cmdline++ != '=') {
					continue;
				}
				while (*cmdline != 0 && *cmdline != ' ') {
					*value_p++ = *cmdline++;
				}
				return value_p - value;
			}
		}
	}

	return 0;
}

static int get_flash_type(void)
{
	char ctype[16];
	struct device_node *chosen_node;
	const char *bootargs;
	int ret;
	chosen_node = of_find_node_by_path("/chosen");
	if (chosen_node) {
		ret = of_property_read_string(chosen_node, "bootargs",
					      &bootargs);
		if (ret) {
			pr_err("Failed to get bootargs\n");
			return -1;
		}
	} else {
		pr_err("Failed to get choosen node\n");
		return -1;
	}
	pr_debug("bootargs:%s\n", bootargs);
	memset(ctype, 0, 16);
	if (get_para_from_cmdline(bootargs, "boot_type",
	    ctype) <= 0) {
		pr_err("Get boot type cmd line fail\n");
		return -1;
	}

	flash_boot_type = simple_strtol(ctype, NULL, 10);
	pr_info("Boot type %d\n", flash_boot_type);
	return 0;
}

/* nand secure storage read/write */
static int _nand_read(int id, char *buf, ssize_t len)
{
	int ret;

	if (!buf) {
		pr_err("-buf NULL\n");
		return -1;
	}
	if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
		pr_err("out of range id %x\n", id);
		return -1;
	}
	secblk_op.item = id;
	secblk_op.buf = buf;
	secblk_op.len = len;

	ret = _sst_user_ioctl(nand_oem_path, SECBLK_READ, &secblk_op);
	if (ret < 0) {
		ret = _sst_user_ioctl(nand_oem_path_for_linux, SECBLK_READ, &secblk_op);
		if (ret < 0) {
			ret = _sst_user_ioctl(nand_oem_path2, SECBLK_READ, &secblk_op);
			if (ret < 0)
				ret = _sst_user_ioctl(nand_oem_path2_for_linux, SECBLK_READ, &secblk_op);
		}
	}
	return ret;
}

static int _nand_write(int	id, char *buf, ssize_t len)
{
	int ret;

	if (!buf) {
		pr_err("- buf NULL\n");
		return -1;
	}

	if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
		pr_err("out of range id %x\n", id);
		return -1;
	}
	secblk_op.item = id;
	secblk_op.buf = buf;
	secblk_op.len = len;

	ret = _sst_user_ioctl(nand_oem_path, SECBLK_WRITE, &secblk_op);
	if (ret < 0) {
		ret = _sst_user_ioctl(nand_oem_path_for_linux, SECBLK_WRITE, &secblk_op);
		if (ret < 0) {
			ret = _sst_user_ioctl(nand_oem_path2, SECBLK_WRITE, &secblk_op);
			if (ret < 0)
				ret = _sst_user_ioctl(nand_oem_path2_for_linux, SECBLK_WRITE, &secblk_op);
		}
	}
	return ret;
}

/* emmc secure storage read/write */
static int _sd_read(char *buf, int len, int offset)
{
	int ret;

	ret = _sst_user_read_and_write(sd_oem_path, buf, len, offset,
				       REQ_OP_READ);
	if (ret < 0) {
		ret = _sst_user_read_and_write(sd_oem_path_for_linux, buf, len,
					       offset, REQ_OP_READ);
		if (ret < 0) {
			pr_err("sst user read: read request len 0x%x, actual read 0x%x\n",
			       len, ret);
			return -1;
		}
	}
	return 0;
}

static int _sd_write(char *buf, int len, int offset)
{
	int ret;

	ret = _sst_user_read_and_write(sd_oem_path, buf, len, offset,
				       REQ_OP_WRITE);
	if (ret < 0) {
		ret = _sst_user_read_and_write(sd_oem_path_for_linux, buf, len,
					       offset, REQ_OP_WRITE);
		if (ret < 0) {
			pr_err("sst user write: write request len 0x%x, actual write 0x%x\n",
			       len, ret);
			return -1;
		}
	}
	return 0;
}

static int nv_write(int id, char *buf, ssize_t len, ssize_t offset)
{
	int ret ;
	switch (flash_boot_type) {
	case FLASH_TYPE_NAND:
		ret = _nand_write(id, buf, len);
		break;
	case FLASH_TYPE_SD1:
	case FLASH_TYPE_SD2:
		ret = _sd_write(buf, len, offset);
		break;
	default:
		pr_err("Unknown no-volatile device\n");
		ret = -1 ;
		break;
	}

	return ret ;
}

static int nv_read(int id, char *buf, ssize_t len, ssize_t offset)
{
	int ret ;
	switch (flash_boot_type) {
	case FLASH_TYPE_NAND:
		ret = _nand_read(id, buf, len);
		break;
	case FLASH_TYPE_SD1:
	case FLASH_TYPE_SD2:
		ret = _sd_read(buf, len, offset);
		break;
	default:
		pr_err("Unknown no-volatile device\n");
		ret = -1 ;
		break;
	}

	return ret ;
}

static int sst_storage_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	return ret;
}
static int sst_storage_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long sst_storage_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param)
{
	int err = 0;

	mutex_lock(&nfcr->mutex);
	if (copy_from_user(&nfcr->key_store, (void __user *)ioctl_param, sizeof(nfcr->key_store))) {
		err = -EFAULT;
		goto _out;
	}

	if (!nfcr->key_store.buf) {
		pr_err("buf is NULL\n");
		err = -1;
		goto _out;
	}

	if (nfcr->key_store.id > MAX_SECURE_STORAGE_MAX_ITEM) {
		pr_err("out of range id %d, id should be in [1, 32]\n", nfcr->key_store.id);
		err = -1;
		goto _out;
	}

	if (nfcr->key_store.len > SEC_BLK_SIZE) {
		pr_err("user ask for 0x%x data, it more than sst load_data 0x%x\n", nfcr->key_store.len, SEC_BLK_SIZE);
		err = -1;
		goto _out;
	}

	nfcr->cmd = ioctl_num;

#if AW_SECURE_STORAGE_DEBUG
	pr_info("size of struct sst_storage_data %d\n", sizeof(struct sst_storage_data));
	pr_info("sst_storage_ioctl: ioctl_num=%d\n", ioctl_num);
	pr_info("nfcr = %p\n", nfcr);
	pr_info("id = %d\n", nfcr->key_store.id);
	pr_info("len = %d\n", nfcr->key_store.len);
	pr_info("offset = %d\n", nfcr->key_store.offset);
#endif

	switch (ioctl_num) {
	case SST_STORAGE_READ:
		schedule_work(&nfcr->work);
		wait_for_completion(&nfcr->work_end);
		/* sunxi_dump(nfcr->temp_data, 50); */
		err = nfcr->ret;
		if (!err) {
			if (copy_to_user((void __user *)nfcr->key_store.buf, nfcr->temp_data, nfcr->key_store.len)) {
				err = -EFAULT;
				pr_err("copy_to_user: err:%d\n", err);
				goto _out;
			}
		}
		break;
	case SST_STORAGE_WRITE:
		if (copy_from_user(nfcr->temp_data, (void __user *)nfcr->key_store.buf, nfcr->key_store.len)) {
			err = -EFAULT;
			pr_err("copy_from_user: err:%d\n", err);
			goto _out;
		}
		schedule_work(&nfcr->work);
		wait_for_completion(&nfcr->work_end);
		err = nfcr->ret;
		break;
	default:
		pr_err("sst_storage_ioctl: un supported cmd:%d\n", ioctl_num);
		break;
	}
_out:
	memset(nfcr->temp_data, 0, SEC_BLK_SIZE);
	memset(&nfcr->key_store, 0, sizeof(nfcr->key_store));
	nfcr->cmd = -1;
	mutex_unlock(&nfcr->mutex);
	return err;
}

static const struct file_operations sst_storage_ops = {
	.owner 		= THIS_MODULE,
	.open 		= sst_storage_open,
	.release 	= sst_storage_release,
	.unlocked_ioctl = sst_storage_ioctl,
	.compat_ioctl = sst_storage_ioctl,
};

struct miscdevice sst_storage_device = {
	.minor 	= MISC_DYNAMIC_MINOR,
	.name 	= "sst_storage",
	.fops	= &sst_storage_ops,
};

static void sst_storage_work(struct work_struct *data)
{
	fcry_pt fcpt  = container_of(data, struct fcrypt, work);

	switch (fcpt->cmd) {
	case SST_STORAGE_READ:
		fcpt->ret = nv_read(fcpt->key_store.id, fcpt->temp_data, fcpt->key_store.len, fcpt->key_store.offset);
	break;
	case SST_STORAGE_WRITE:
		fcpt->ret = nv_write(fcpt->key_store.id, fcpt->temp_data, fcpt->key_store.len, fcpt->key_store.offset);
	break;
	default:
		fcpt->ret = -1;
		pr_err("sst_storage_work: un supported cmd:%d\n", fcpt->cmd);
		break;
	}

	if ((fcpt->cmd == SST_STORAGE_READ) || (fcpt->cmd == SST_STORAGE_WRITE))
		complete(&fcpt->work_end);
}

static void __exit sunxi_secure_storage_exit(void)
{
	pr_debug("sunxi secure storage driver exit\n");

	misc_deregister(&sst_storage_device);
	kfree(nfcr->temp_data);
	kfree(nfcr);
}

static int __init sunxi_secure_storage_init(void)
{
	int ret;

	if (!secure_storage_inited) {
		get_flash_type();
	}
	secure_storage_inited = 1;

	ret = misc_register(&sst_storage_device);
	if (ret) {
		pr_err("%s: cannot deregister miscdev.(return value-%d)\n", __func__, ret);
		return ret;
	}

	nfcr = kzalloc(sizeof(*nfcr), GFP_KERNEL);
	if (!nfcr) {
		pr_err(" - Malloc failed\n");
		return -1;
	}

	nfcr->temp_data = kzalloc(SEC_BLK_SIZE, GFP_KERNEL);
	if (!nfcr->temp_data) {
		pr_err("sst_storage_ioctl: error to kmalloc\n");
		return -1;
	}

	INIT_WORK(&nfcr->work, sst_storage_work);
	init_completion(&nfcr->work_end);
	mutex_init(&nfcr->mutex);

	return 0;
}

module_init(sunxi_secure_storage_init);
module_exit(sunxi_secure_storage_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yanjianbo");
MODULE_VERSION("1.0.1");
MODULE_DESCRIPTION("secure storage");
