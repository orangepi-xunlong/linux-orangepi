// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Implements pstore backend driver that write to block (or non-block) storage
 * devices, using the pstore/zone API.
 *
 * Reference: fs/pstore/blk.c
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/sunxi_blkpstore.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/init_syscalls.h>
#include <linux/mount.h>
#include <linux/notifier.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/namei.h>

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

struct bdev_info bdev_info;

#ifndef MODULE
static char partitions[1024];
static int __init partitions_setup(char *str)
{
	memcpy(partitions, str, strlen(str));
	return 1;
}
__setup("partitions=", partitions_setup);
#endif

static long kmsg_size = CONFIG_AW_PSTORE_BLK_KMSG_SIZE;
module_param(kmsg_size, long, 0400);
MODULE_PARM_DESC(kmsg_size, "kmsg dump record size in kbytes");

static int max_reason = CONFIG_AW_PSTORE_BLK_MAX_REASON;
module_param(max_reason, int, 0400);
MODULE_PARM_DESC(max_reason,
		 "maximum reason for kmsg dump (default 2: Oops and Panic)");

#if IS_ENABLED(CONFIG_PSTORE_PMSG)
static long pmsg_size = CONFIG_AW_PSTORE_BLK_PMSG_SIZE;
#else
static long pmsg_size = -1;
#endif
module_param(pmsg_size, long, 0400);
MODULE_PARM_DESC(pmsg_size, "pmsg size in kbytes");

#if IS_ENABLED(CONFIG_PSTORE_CONSOLE)
static long console_size = CONFIG_AW_PSTORE_BLK_CONSOLE_SIZE;
#else
static long console_size = -1;
#endif
module_param(console_size, long, 0400);
MODULE_PARM_DESC(console_size, "console size in kbytes");

#if IS_ENABLED(CONFIG_PSTORE_FTRACE)
static long ftrace_size = CONFIG_AW_PSTORE_BLK_FTRACE_SIZE;
#else
static long ftrace_size = -1;
#endif
module_param(ftrace_size, long, 0400);
MODULE_PARM_DESC(ftrace_size, "ftrace size in kbytes");

/*
 * blkdev - the block device to use for pstore storage
 * See Documentation/admin-guide/pstore-blk.rst for details.
 */
static char blkdev[80] = CONFIG_AW_PSTORE_BLK_BLKDEV;
module_param_string(blkdev, blkdev, 80, 0400);
MODULE_PARM_DESC(blkdev, "block device for pstore storage");

/*
 * All globals must only be accessed under the aw_psblk_lock
 * during the register/unregister functions.
 */
static DEFINE_MUTEX(aw_psblk_lock);
static struct file *psblk_file;
static struct pstore_device_info *pstore_device_info;

static struct {
	struct raw_notifier_head chain;
	struct pstore_blk_notifier *pbn;
	bool notifier;
} pstore_blk_panic_notifier = {
	.chain = RAW_NOTIFIER_INIT(pstore_blk_panic_notifier.chain),
};

#define check_size(name, alignsize) ({				\
	long _##name_ = (name);					\
	_##name_ = _##name_ <= 0 ? 0 : (_##name_ * 1024);	\
	if (_##name_ & ((alignsize) - 1)) {			\
		pr_info(#name " must align to %d\n",		\
				(alignsize));			\
		_##name_ = ALIGN(name, (alignsize));		\
	}							\
	_##name_;						\
})

#define verify_size(name, alignsize, enabled) {			\
	long _##name_;						\
	if (enabled)						\
		_##name_ = check_size(name, alignsize);		\
	else							\
		_##name_ = 0;					\
	/* Synchronize module parameters with resuls. */	\
	name = _##name_ / 1024;					\
	dev->zone.name = _##name_;				\
}

#ifndef MODULE
static int aw_pstore_blk_device_parse(const char *part_name, char *device_name)
{
	char *start, *end;
	char pattern[50];

	sprintf((char *)pattern, ":%s@", part_name);
	pattern[strlen(pattern)] = '\0';

	start = strstr(partitions, pattern);
	start = strstr(start, ":");
	start = strstr(start, "@");
	start++;
	end = strstr(start, ":");

	strncpy(device_name, start, end - start);
	device_name[end - start] = '\0';

	return 0;
}
#else
static int aw_pstore_blk_device_parse(const char *part_name, char *device_name)
{
	return 0;
}
#endif

static int aw_pstore_blk_panic_notifier_call(struct notifier_block *nb,
		unsigned long action, void *data)
{
	int ret = 0;
	struct pstore_blk_notifier *pbn =
		container_of(nb, struct pstore_blk_notifier, nb);

	if (pbn)
		ret = pbn->notifier_call(action, data);

	return ret;
}

int aw_register_pstore_blk_panic_notifier(struct pstore_blk_notifier *pbn)
{
	int err = 0;
	struct notifier_block *nb;

	mutex_lock(&aw_psblk_lock);

	if (pstore_blk_panic_notifier.notifier) {
		pr_info("had register panic\n");
		goto unlock;
	}

	nb = &pbn->nb;
	nb->notifier_call = aw_pstore_blk_panic_notifier_call;

	err = raw_notifier_chain_register(&pstore_blk_panic_notifier.chain, nb);
	if (err)
		goto unlock;

	if (pstore_device_info)
		err = nb->notifier_call(nb, PSTORE_BLK_BACKEND_PANIC_DRV_REGISTER,
				pstore_device_info);

	if (!err)
		pstore_blk_panic_notifier.notifier = true;

unlock:
	mutex_unlock(&aw_psblk_lock);

	return err;
}
EXPORT_SYMBOL_GPL(aw_register_pstore_blk_panic_notifier);

void aw_unregister_pstore_blk_panic_notifier(struct pstore_blk_notifier *pbn)
{
	struct notifier_block *nb = &pbn->nb;

	mutex_lock(&aw_psblk_lock);

	raw_notifier_chain_unregister(&pstore_blk_panic_notifier.chain, nb);

	if (pstore_device_info)
		nb->notifier_call(nb, PSTORE_BLK_BACKEND_PANIC_DRV_UNREGISTER,
				pstore_device_info);

	pstore_blk_panic_notifier.notifier = false;

	mutex_unlock(&aw_psblk_lock);
}
EXPORT_SYMBOL_GPL(aw_unregister_pstore_blk_panic_notifier);

static int aw_pstore_blk_panic_notifier_init_call(struct pstore_device_info *pdi)
{
	return raw_notifier_call_chain(&pstore_blk_panic_notifier.chain,
			PSTORE_BLK_BACKEND_REGISTER, pdi);
}

static int aw_pstore_blk_panic_notifier_exit_call(struct pstore_device_info *pdi)
{
	return raw_notifier_call_chain(&pstore_blk_panic_notifier.chain,
			PSTORE_BLK_BACKEND_UNREGISTER, pdi);
}


/**
 * aw_register_pstore_device() - register device to pstore/blk
 *
 * @dev: non-block device information
 *
 * Return:
 * * 0		- OK
 * * Others	- something error.
 */
int aw_register_pstore_device(struct pstore_device_info *dev)
{
	int ret;

	mutex_lock(&aw_psblk_lock);

	if (!dev) {
		pr_err("NULL device info\n");
		return -EINVAL;
	}
	if (!dev->zone.total_size) {
		pr_err("zero sized device\n");
		return -EINVAL;
	}
	if (!dev->zone.read) {
		pr_err("no read handler for device\n");
		return -EINVAL;
	}
	if (!dev->zone.write) {
		pr_err("no write handler for device\n");
		return -EINVAL;
	}

	/* someone already registered before */
	if (pstore_device_info)
		return -EBUSY;

	/* zero means not limit on which backends to attempt to store. */
	if (!dev->flags)
		dev->flags = UINT_MAX;

	/* Copy in module parameters. */
	verify_size(kmsg_size, 4096, dev->flags & PSTORE_FLAGS_DMESG);
	verify_size(pmsg_size, 4096, dev->flags & PSTORE_FLAGS_PMSG);
	verify_size(console_size, 4096, dev->flags & PSTORE_FLAGS_CONSOLE);
	verify_size(ftrace_size, 4096, dev->flags & PSTORE_FLAGS_FTRACE);
	dev->zone.max_reason = max_reason;

	/* Initialize required zone ownership details. */
	dev->zone.name = KBUILD_MODNAME;
	dev->zone.owner = THIS_MODULE;

	ret = register_pstore_zone(&dev->zone);
	if (ret == 0)
		pstore_device_info = dev;

	mutex_unlock(&aw_psblk_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(aw_register_pstore_device);

/**
 * aw_unregister_pstore_device() - unregister device from pstore/blk
 *
 * @dev: non-block device information
 */
void aw_unregister_pstore_device(struct pstore_device_info *dev)
{
	mutex_lock(&aw_psblk_lock);
	if (pstore_device_info && pstore_device_info == dev) {
		unregister_pstore_zone(&dev->zone);
		pstore_device_info = NULL;
	}
	mutex_unlock(&aw_psblk_lock);
}
EXPORT_SYMBOL_GPL(aw_unregister_pstore_device);

static ssize_t aw_psblk_generic_blk_read(char *buf, size_t bytes, loff_t pos)
{
	return kernel_read(psblk_file, buf, bytes, &pos);
}

static ssize_t aw_psblk_generic_blk_write(const char *buf, size_t bytes,
		loff_t pos)
{
	/* Console/Ftrace backend may handle buffer until flush dirty zones */
	if (in_interrupt() || irqs_disabled())
		return -EBUSY;
	return kernel_write(psblk_file, buf, bytes, &pos);
}

/*
 * This takes its configuration only from the module parameters now.
 */
static int register_pstore_blk(struct pstore_device_info *dev,
				 const char *devpath)
{
	struct inode *inode;
	int ret = -ENODEV;

	bdev_info.bdev = blkdev_get_by_path(devpath, FMODE_READ, NULL);
	if (IS_ERR(bdev_info.bdev)) {
		pr_err("fail to access bdev %s errno %ld\n", devpath,
				PTR_ERR(bdev_info.bdev));
		return -EACCES;
	}

	bdev_info.nr_sects = bdev_nr_sectors(bdev_info.bdev);
	bdev_info.start_sect = bdev_info.bdev->bd_start_sect;

	psblk_file = filp_open(devpath, O_RDWR | O_DSYNC | O_NOATIME | O_EXCL, 0);
	if (IS_ERR(psblk_file)) {
		ret = PTR_ERR(psblk_file);
		pr_err("failed to open '%s': %d!\n", devpath, ret);
		goto err;
	}

	inode = file_inode(psblk_file);
	if (!S_ISBLK(inode->i_mode)) {
		pr_err("'%s' is not block device!\n", devpath);
		goto err_fput;
	}

	inode = I_BDEV(psblk_file->f_mapping->host)->bd_inode;
	dev->zone.total_size = i_size_read(inode);

	dev->zone.read = aw_psblk_generic_blk_read;
	dev->zone.write = aw_psblk_generic_blk_write;

	ret = aw_register_pstore_device(dev);
	if (ret)
		goto err_fput;

	return 0;

err_fput:
	dev->zone.read = NULL;
	dev->zone.write = NULL;
	fput(psblk_file);
err:
	blkdev_put(bdev_info.bdev, FMODE_READ);
	psblk_file = NULL;

	return ret;
}

#ifndef MODULE
static const char devname[] = "/dev/pstore-blk";
static __init const char *early_boot_devpath(const char *initial_devname)
{
	/*
	 * During early boot the real root file system hasn't been
	 * mounted yet, and no device nodes are present yet. Use the
	 * same scheme to find the device that we use for mounting
	 * the root file system.
	 */
	dev_t dev = name_to_dev_t(initial_devname);

	if (!dev) {
		pr_err("failed to resolve '%s'!\n", initial_devname);
		return initial_devname;
	}

	init_unlink(devname);
	init_mknod(devname, S_IFBLK | 0600, new_encode_dev(dev));

	return devname;
}
#else
static inline const char *early_boot_devpath(const char *initial_devname)
{
	return initial_devname;
}
#endif

int aw_pstore_blk_get_bdev_info(struct bdev_info *bi)
{

	bdev_info.nr_sects = bdev_nr_sectors(bdev_info.bdev);
	bdev_info.start_sect = bdev_info.bdev->bd_start_sect;
	printk("bdev start %d\n", (int)bdev_info.start_sect);
	printk("bdev end %d\n", (int)(bdev_info.start_sect + bdev_info.nr_sects));
	if (psblk_file != NULL)
		memcpy(bi, &bdev_info, sizeof(*bi));
	else
		return -EACCES;

	return 0;
}
EXPORT_SYMBOL_GPL(aw_pstore_blk_get_bdev_info);

static int __init aw_pstore_blk_init(void)
{
	struct pstore_device_info *aw_psblk_dev;
	int ret;
	static char device_name[80];
	int try = 0;

	/* Reject an empty blkdev. */
	if (!blkdev[0]) {
		pr_err("blkdev empty\n");
		return -EINVAL;
	}

	aw_psblk_dev = kzalloc(sizeof(*aw_psblk_dev), GFP_KERNEL);
	if (!aw_psblk_dev)
		return -ENOMEM;

try:
	/*blkdev is a part name, we should fill the full path*/
	if (blkdev[0] != '/') {
		aw_pstore_blk_device_parse(blkdev, device_name);
		if (try == 0) {
			sprintf(blkdev, "/dev/%s", device_name);
		} else {
			sprintf(blkdev, "/dev/block/%s", device_name);
			try = 1;
		}
	}

	ret = register_pstore_blk(aw_psblk_dev,
				    early_boot_devpath(blkdev));
	if (ret) {
		if (try == 0)
			goto try;
		else
			kfree(aw_psblk_dev);
	} else {
		if (aw_pstore_blk_panic_notifier_init_call(aw_psblk_dev) == NOTIFY_OK)
			pr_info("attached %s (%lu) (dedicated panic_write!)\n",
					blkdev, aw_psblk_dev->zone.total_size);
		else
			pr_info("attached %s (%lu) (no dedicated panic_write!)\n",
					blkdev, aw_psblk_dev->zone.total_size);

	}
	return ret;
}
late_initcall(aw_pstore_blk_init);

static void __exit aw_pstore_blk_exit(void)
{
	mutex_lock(&aw_psblk_lock);
	/*
	 * Currently, the only user of psblk_file is best_effort, so
	 * we can assume that pstore_device_info is associated with it.
	 * Once there are "real" blk devices, there will need to be a
	 * dedicated pstore_blk_info, etc.
	 */
	if (psblk_file) {
		struct pstore_device_info *dev = pstore_device_info;

		aw_pstore_blk_panic_notifier_exit_call(dev);

		aw_unregister_pstore_device(dev);
		kfree(dev);
		fput(psblk_file);
		blkdev_put(bdev_info.bdev, FMODE_READ);
		psblk_file = NULL;
	}
	mutex_unlock(&aw_psblk_lock);
}
module_exit(aw_pstore_blk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cuizhikui<cuizhikui@allwinnertech.com>");
MODULE_DESCRIPTION("pstore backend for block devices");
MODULE_VERSION("1.0.0");
