/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's rpmsg notify driver
 *
 * the driver provides an interface to sync data to the
 * 'isp-parameter' for the bootpackage.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/version.h>
#include "linux/types.h"
#include <linux/remoteproc.h>
#include <linux/debugfs.h>
#include <asm/setup.h>
//#include <rtw_byteorder.h>

#define PACKAGE_ITEM_NAME       "isp-parameter"

#define FIRMWARE_MAGIC				0x89119800
#define ISP_PARA_OFFSET (13312 * 512)
#define ISP_PARA_MAX_LEN (5 * 1024 * 1024)

typedef struct firmware_head_info {
	u8   name[16];
	u32  magic;			/* must is 0x89119800 */
	u32  reserved1[3];
	u32  items_nr;
	u32  reserved2[6];
	u32  end;
} firmware_head_info_t;

typedef struct firmware_item_info {
	char name[64];
	u32  data_offset;
	u32  data_len;
	u32  reserved[74];
} firmware_item_info_t;

struct load_storage {
	const char *name;
	const char *path;
	const int head_off;
	const int offset;
	const u32 flag;
};

#define SUNXI_FW_FLAG_EMMC				2
#define SUNXI_FW_FLAG_NOR				3
#define SUNXI_FW_FLAG_NAND				5

struct msg_head {
	u64  da;
	u32  len;
	u32  offset;
} __attribute__((packed));

struct sync_action {
	struct device *dev;
	struct delayed_work work;
	struct msg_head msg;
};

static const struct load_storage firmware_storages[] = {
	{ "nor",  "/dev/mtd0", 128 * 512,  128 * 512, SUNXI_FW_FLAG_NOR},
	{ "nand",  "/dev/mtd1", 0,  0, SUNXI_FW_FLAG_NAND},
	{ "emmc",  "/dev/mmcblk0", 32800 * 512,  32800 * 512, SUNXI_FW_FLAG_EMMC},
};

static const struct load_storage *cur_storage;
static const struct rproc *rproc;
static struct dentry *parameter_dbg;
static struct dentry *dbg_dir;

static int _read_file(const char *path, void *buf, u32 len, loff_t *pos)
{
	int ret;
	struct file *fp;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	{
		mm_segment_t fs;

		fs = get_fs();
		set_fs(KERNEL_DS);
		fp = filp_open(path, O_RDONLY, 0444);
		set_fs(fs);
	}
#else
	fp = filp_open(path, O_RDONLY, 0444);
#endif
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	ret = kernel_read(fp, buf, len, pos);

	filp_close(fp, NULL);

	return ret;
}

static int _write_file(const char *path, void *buf, u32 len, loff_t *pos)
{
	int ret;
	struct file *fp;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	{
		mm_segment_t fs;
		fs = get_fs();
		set_fs(KERNEL_DS);
		fp = filp_open(path, O_WRONLY, 0);
		set_fs(fs);
	}
#else
	fp = filp_open(path, O_WRONLY, 0);
#endif
	if (IS_ERR(fp))
		return PTR_ERR(fp);

	ret = kernel_write(fp, buf, len, pos);
	vfs_fsync(fp, 1);

	filp_close(fp, NULL);

	return ret;
}

static int sunxi_parse_storage_type(void)
{
	int ret, storage_type, i;
	char *buf;
	loff_t pos = 0;
	char *p;
	static const char *path = "/proc/cmdline";

	buf = kmalloc(COMMAND_LINE_SIZE, GFP_KERNEL);
	if (!buf) {
		pr_err("out of memory, require %d bytes\n", COMMAND_LINE_SIZE);
		return -ENOMEM;
	}

	ret = _read_file(path, buf, COMMAND_LINE_SIZE, &pos);
	if (ret < 0)
		return ret;
	buf[ret] = '\0';

	p = strstr(buf, "boot_type=");
	if (!p) {
		pr_err("Can't find boot_type!\n");
		ret = -EFAULT;
		goto err_out;
	}
	/* boot_type range in [0,6] */
	storage_type = ((int)p[10]) - 0x30;
	if (storage_type < 0 || storage_type > 6) {
		pr_err("Invalid boot_type!\n");
		ret = -EFAULT;
		goto err_out;
	}
	pr_debug("storage_type = %d\n", storage_type);

	for (i = 0; i < ARRAY_SIZE(firmware_storages); i++) {
		if (firmware_storages[i].flag != storage_type)
			continue;
		cur_storage = &firmware_storages[i];
	}

	kfree(buf);

	return storage_type;
err_out:
	kfree(buf);
	return ret;
}

static int sunxi_firmware_parser_head(const char *name, void *buf, u32 *addr, u32 *len)
{
	u8 *pbuf = buf;
	int i;
	struct firmware_head_info *head;
	struct firmware_item_info *item;

	head = (struct firmware_head_info *)pbuf;
	item = (struct firmware_item_info *)(pbuf + sizeof(*head));

	for (i = 0; i < head->items_nr; i++, item++) {
		if (strncmp(item->name, name, strlen(item->name)) == 0) {
			*addr = item->data_offset;
			*len = item->data_len;
			return 0;
		}
	}

	return -ENOENT;
}

static int sunxi_firmware_get_info(const char *name, u32 *img_addr, u32 *img_len)
{
#define TMP_BUF_SIZE			4096
	int ret;
	u8 *head;
	loff_t pos = 0;
	u32 offset, flag;
	const char *path;

	if (!cur_storage)
		sunxi_parse_storage_type();

	if (!cur_storage) {
		pr_err("can't find current storage type.\r\n");
		return -EINVAL;
	}

	head = kmalloc(TMP_BUF_SIZE, GFP_KERNEL);
	if (!head) {
		pr_err("out of memory, require %d bytes\n", TMP_BUF_SIZE);
		return -ENOMEM;
	}

	offset = cur_storage->offset;
	path = cur_storage->path;
	flag = cur_storage->flag;
	pos = cur_storage->head_off;

	ret = _read_file(path, head, TMP_BUF_SIZE, &pos);
	if (ret != TMP_BUF_SIZE) {
		pr_err("read %s failed\n", path);
		ret = -EINVAL;
		goto out;
	}

	ret = sunxi_firmware_parser_head(name, head, img_addr, img_len);
	if (ret < 0) {
		pr_err("failed to parser head (%s) ret=%d\n", name, ret);
		ret = -EFAULT;
		goto out;
	}

	*img_addr += offset;
	ret = 0;
	pr_debug("firmware: addr:0x%08x len:0x%x\n", *img_addr, *img_len);
out:
	kfree(head);
	return ret;
}

static int _read_isp_parameter(void *va, u32 len, loff_t *pos)
{
	int ret;
	u32 flag;
	u32 item_addr, item_len;
	/* int i; */

	flag = cur_storage->flag;

	if (sunxi_firmware_get_info(PACKAGE_ITEM_NAME, &item_addr, &item_len))
		return -EFAULT;

	if (len > item_len) {
		pr_info("want to write %d bytes but the item only has %d bytes,"
						"truncate it\n", len, item_len);
		len = item_len;
	}

	*pos += item_addr;
	ret = _read_file(cur_storage->path, va, len, pos);
	if (ret < 0) {
		pr_err("failed to read firmware data\n");
		ret = -EFAULT;
		goto out;
	}

	/*for (i = 0; i < len/4; i++)*/
	/*	*((unsigned int *)va + i) = le32_to_cpu(*((unsigned int *)va + i));*/

out:
	return ret;
}

static int _sync_isp_parameter(void *va, u32 len, int offset)
{
	int ret;
	loff_t pos;
	u32 flag;

	flag = cur_storage->flag;

	if (!cur_storage)
		sunxi_parse_storage_type();

	if (!cur_storage) {
		pr_err("can't find current storage type.\r\n");
		return -EINVAL;
	}

	if (len > ISP_PARA_MAX_LEN) {
		pr_info("want to write %d bytes but only allow %d bytes,"
						"truncate it\n", len, ISP_PARA_MAX_LEN);
		len = ISP_PARA_MAX_LEN;
	}

	pos = ISP_PARA_OFFSET;
	pos += offset;
	ret = _write_file(cur_storage->path, va, len, &pos);
	if (ret < 0) {
		pr_err("failed to write firmware data\n");
		ret = -EFAULT;
		goto out;
	}

out:
	return ret;
}

static void rpmsg_isp_parameter_work_func(struct work_struct *work)
{
	struct sync_action *action;
	void *va;

	action = container_of(to_delayed_work(work), struct sync_action, work);

	va = rproc_da_to_va((struct rproc *)rproc, action->msg.da,
					action->msg.len, NULL);
	if (!va) {
		dev_err(action->dev, "Invalid da: 0x%lx\r\n",
						(unsigned long)action->msg.da);
		return;
	}

	dev_info(action->dev, "\tpa: 0x%lx, va: %px\r\n", (long)action->msg.da, va);
	dev_info(action->dev, "\toffset: %d, len: %d\r\n", action->msg.offset, action->msg.len);
	_sync_isp_parameter(va, action->msg.len, action->msg.offset);

	devm_kfree(action->dev, action);
}

static int rpmsg_isp_parameter_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct sync_action *action;

	if (len != sizeof(action->msg)) {
		dev_err(&rpdev->dev, "Invalid length,rx:%d expect:%zu\n",
						len, sizeof(action->msg));
		return -EINVAL;
	}

	dev_info(&rpdev->dev, "sync bootpackage.isp_parameter\n");

	action = devm_kmalloc(&rpdev->dev, sizeof(*action) + len, GFP_KERNEL);
	if (!action) {
		dev_err(&rpdev->dev, "out of memory, require %d bytes\n", (int)sizeof(*action));
		return -ENOENT;
	}

	action->dev = &rpdev->dev;
	memcpy(&action->msg, data, len);
	INIT_DELAYED_WORK(&action->work, rpmsg_isp_parameter_work_func);

	schedule_delayed_work(&action->work, 0);

	return 0;
}

static ssize_t para_dump_read(struct file *filp, char __user *userbuf,
			       size_t count, loff_t *ppos)
{
	const char *buf = "finish\n";
	u32 item_addr, item_len;
	loff_t pos = 0;
	void *data;

	if (sunxi_firmware_get_info(PACKAGE_ITEM_NAME, &item_addr, &item_len))
		return -EFAULT;

	data = vmalloc(item_len);
	if (!data)
		return -ENOMEM;

	if (_read_isp_parameter(data, item_len, &pos) < 0)
		return -ENOENT;

	print_hex_dump(KERN_INFO, " ", DUMP_PREFIX_OFFSET,
					16, 1, data, item_len, false);

	vfree(data);

	return simple_read_from_buffer(userbuf, count, ppos, buf, sizeof(buf));
}

static const struct file_operations para_dump_ops = {
	.read = para_dump_read,
	.open = simple_open,
	.llseek	= generic_file_llseek,
};

static ssize_t para_sync_write(struct file *filp,
				    const char __user *user_buf, size_t count,
				    loff_t *ppos)

{
	char buf[32];
	u32 item_addr, item_len;
	void *data;
	int ret;

	if (count < 1 || count > sizeof(buf))
		return -EINVAL;

	ret = copy_from_user(buf, user_buf, count);
	if (ret)
		return -EFAULT;

	if (sunxi_firmware_get_info(PACKAGE_ITEM_NAME, &item_addr, &item_len))
		return -EFAULT;

	data = vmalloc(item_len);
	if (!data)
		return -ENOMEM;

	memset(data, buf[0], item_len);
	if (_sync_isp_parameter(data, item_len, 0) < 0)
		return -ENOENT;
	vfree(data);

	return count;
}

static const struct file_operations para_sync_ops = {
	.write = para_sync_write,
	.open = simple_open,
	.llseek	= generic_file_llseek,
};

void parameter_delete_debug_dir(void)
{
	if (dbg_dir)
		debugfs_remove_recursive(dbg_dir);
}

void parameter_create_debug_dir(struct device *dev)
{
	if (!parameter_dbg)
		return;

	dbg_dir = debugfs_create_dir(dev_name(dev), parameter_dbg);
	if (!dbg_dir)
		return;

	debugfs_create_file("dump", 0400, dbg_dir,
			    dev, &para_dump_ops);
	debugfs_create_file("sync", 0600, dbg_dir,
			    dev, &para_sync_ops);
}

static int rpmsg_isp_parameter_probe(struct rpmsg_device *rpdev)
{
	if (rproc) {
		dev_err(&rpdev->dev, "already exist.\n");
		return -EBUSY;
	}
	/*
	 * Normally, the rpmsg device is created from:
	 *     remoteproc device    <--- defined by vendors, usually in device tree
	 *     |--- rproc     <--- rproc_alloc()
	 *          |--- rproc_vdev    <---- rproc_handle_vdev()
	 *               |--- virtio    <--- register_virtio_device()
	 *                    |--- rpmsg    <--- rpmsg_register_device()
	 *
	 * Therefore the 4th parent of rpmsg device is the underlying remoteproc
	 * device.
	 */
	rproc = container_of(rpdev->dev.parent->parent->parent, struct rproc, dev);

	parameter_create_debug_dir(&rpdev->dev);

	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	return 0;
}

static void rpmsg_isp_parameter_remove(struct rpmsg_device *rpdev)
{
	parameter_delete_debug_dir();
	rproc = NULL;
}

static struct rpmsg_device_id rpmsg_driver_id_table[] = {
	{ .name	= "sunxi,isp_parameter" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_id_table);

static struct rpmsg_driver rpmsg_isp_parameter = {
	.drv.name	= "rpmsg_isp_parameter",
	.id_table	= rpmsg_driver_id_table,
	.probe		= rpmsg_isp_parameter_probe,
	.callback	= rpmsg_isp_parameter_cb,
	.remove		= rpmsg_isp_parameter_remove,
};

int sunxi_vin_isp_parameter_init(void)
{
	if (debugfs_initialized()) {
		parameter_dbg = debugfs_create_dir(KBUILD_MODNAME, NULL);
		if (!parameter_dbg)
			pr_err("can't create debugfs dir\n");
	}

	return register_rpmsg_driver(&rpmsg_isp_parameter);
}

void sunxi_vin_isp_parameter_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_isp_parameter);
}

MODULE_DESCRIPTION("isp parameter update driver");
MODULE_AUTHOR("lijiajian@allwinnertech.com");
