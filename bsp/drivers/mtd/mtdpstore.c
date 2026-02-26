// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#define dev_fmt(fmt) "mtdoops-pstore: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/kmsg_dump.h>
#include <linux/pstore.h>
#include <linux/pstore_zone.h>

static long kmsg_size = CONFIG_AW_MTD_PSTORE_KMSG_SIZE;
module_param(kmsg_size, long, 0400);
MODULE_PARM_DESC(kmsg_size, "kmsg dump record size in kbytes");

static int max_reason = CONFIG_AW_MTD_PSTORE_MAX_REASON;
module_param(max_reason, int, 0400);
MODULE_PARM_DESC(max_reason,
		 "maximum reason for kmsg dump (default 2: Oops and Panic)");

#if IS_ENABLED(CONFIG_PSTORE_PMSG)
static long pmsg_size = CONFIG_AW_MTD_PSTORE_PMSG_SIZE;
#else
static long pmsg_size = -1;
#endif
module_param(pmsg_size, long, 0400);
MODULE_PARM_DESC(pmsg_size, "pmsg size in kbytes");

#if IS_ENABLED(CONFIG_PSTORE_CONSOLE)
static long console_size = CONFIG_AW_MTD_PSTORE_CONSOLE_SIZE;
#else
static long console_size = -1;
#endif
module_param(console_size, long, 0400);
MODULE_PARM_DESC(console_size, "console size in kbytes");

#if IS_ENABLED(CONFIG_PSTORE_FTRACE)
static long ftrace_size = CONFIG_AW_MTD_PSTORE_FTRACE_SIZE;
#else
static long ftrace_size = -1;
#endif
module_param(ftrace_size, long, 0400);
MODULE_PARM_DESC(ftrace_size, "ftrace size in kbytes");

static char mtddev[80] = CONFIG_AW_MTD_PSTORE_MTDDEV;
module_param_string(mtddev, mtddev, 80, 0400);
MODULE_PARM_DESC(mtddev, "mtd device for pstore storage");

/**
 * struct mtdpstore_config - the mtdpstore backend configuration
 *
 * @device:		Name of the desired mtd device
 * @max_reason:		Maximum kmsg dump reason to store to mtd device
 * @kmsg_size:		Total size of for kmsg dumps
 * @pmsg_size:		Total size of the pmsg storage area
 * @console_size:	Total size of the console storage area
 * @ftrace_size:	Total size for ftrace logging data (for all CPUs)
 */
struct mtdpstore_config {
	char device[80];
	enum kmsg_dump_reason max_reason;
	unsigned long kmsg_size;
	unsigned long pmsg_size;
	unsigned long console_size;
	unsigned long ftrace_size;
};

/**
 * struct pstore_device_info - back-end pstore/mtddev driver structure.
 *
 * @flags:	Refer to macro starting with PSTORE_FLAGS defined in
 *		linux/pstore.h. It means what front-ends this device support.
 *		Zero means all backends for compatible.
 * @zone:	The struct pstore_zone_info details.
 *
 */
struct pstore_device_info {
	unsigned int flags;
	struct pstore_zone_info zone;
};

static struct mtdpstore_context {
	int index;
	struct mtdpstore_config info;
	struct pstore_device_info dev;
	struct mtd_info *mtd;
	unsigned long *rmmap;		/* removed bit map */
	unsigned long *usedmap;		/* used bit map */
	/*
	 * used for panic write
	 * As there are no block_isbad for panic case, we should keep this
	 * status before panic to ensure panic_write not failed.
	 */
	unsigned long *badmap;		/* bad block bit map */
} oops_cxt;

static struct pstore_device_info *pstore_device_info;
/*
 * All globals must only be accessed under the aw_psmtd_lock
 * during the register/unregister functions.
 */
static DEFINE_MUTEX(aw_psmtd_lock);

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

static int mtdpstore_block_isbad(struct mtdpstore_context *cxt, loff_t off)
{
	int ret;
	struct mtd_info *mtd = cxt->mtd;
	u64 blknum;

	off = ALIGN_DOWN(off, mtd->erasesize);
	blknum = div_u64(off, mtd->erasesize);

	if (test_bit(blknum, cxt->badmap))
		return true;
	ret = mtd_block_isbad(mtd, off);
	if (ret < 0) {
		dev_err(&mtd->dev, "mtd_block_isbad failed, aborting\n");
		return ret;
	} else if (ret > 0) {
		set_bit(blknum, cxt->badmap);
		return true;
	}
	return false;
}

static inline int mtdpstore_panic_block_isbad(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u64 blknum;

	off = ALIGN_DOWN(off, mtd->erasesize);
	blknum = div_u64(off, mtd->erasesize);
	return test_bit(blknum, cxt->badmap);
}

static inline void mtdpstore_mark_used(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u64 zonenum = div_u64(off, cxt->info.kmsg_size);

	dev_dbg(&mtd->dev, "mark zone %llu used\n", zonenum);
	set_bit(zonenum, cxt->usedmap);
}

static inline void mtdpstore_mark_unused(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u64 zonenum = div_u64(off, cxt->info.kmsg_size);

	dev_dbg(&mtd->dev, "mark zone %llu unused\n", zonenum);
	clear_bit(zonenum, cxt->usedmap);
}

static inline void mtdpstore_block_mark_unused(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u32 zonecnt = mtd->erasesize / cxt->info.kmsg_size;
	u64 zonenum;

	off = ALIGN_DOWN(off, mtd->erasesize);
	zonenum = div_u64(off, cxt->info.kmsg_size);
	while (zonecnt > 0) {
		dev_dbg(&mtd->dev, "mark zone %llu unused\n", zonenum);
		clear_bit(zonenum, cxt->usedmap);
		zonenum++;
		zonecnt--;
	}
}

static inline int mtdpstore_is_used(struct mtdpstore_context *cxt, loff_t off)
{
	u64 zonenum = div_u64(off, cxt->info.kmsg_size);
	u64 blknum = div_u64(off, cxt->mtd->erasesize);

	if (test_bit(blknum, cxt->badmap))
		return true;
	return test_bit(zonenum, cxt->usedmap);
}

static int mtdpstore_block_is_used(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u32 zonecnt = mtd->erasesize / cxt->info.kmsg_size;
	u64 zonenum;

	off = ALIGN_DOWN(off, mtd->erasesize);
	zonenum = div_u64(off, cxt->info.kmsg_size);
	while (zonecnt > 0) {
		if (test_bit(zonenum, cxt->usedmap))
			return true;
		zonenum++;
		zonecnt--;
	}
	return false;
}

static int mtdpstore_is_empty(struct mtdpstore_context *cxt, char *buf,
		size_t size)
{
	struct mtd_info *mtd = cxt->mtd;
	size_t sz;
	int i;

	sz = min_t(uint32_t, size, mtd->writesize / 4);
	for (i = 0; i < sz; i++) {
		if (buf[i] != (char)0xFF)
			return false;
	}
	return true;
}

static void mtdpstore_mark_removed(struct mtdpstore_context *cxt, loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u64 zonenum = div_u64(off, cxt->info.kmsg_size);

	dev_dbg(&mtd->dev, "mark zone %llu removed\n", zonenum);
	set_bit(zonenum, cxt->rmmap);
}

static void mtdpstore_block_clear_removed(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u32 zonecnt = mtd->erasesize / cxt->info.kmsg_size;
	u64 zonenum;

	off = ALIGN_DOWN(off, mtd->erasesize);
	zonenum = div_u64(off, cxt->info.kmsg_size);
	while (zonecnt > 0) {
		clear_bit(zonenum, cxt->rmmap);
		zonenum++;
		zonecnt--;
	}
}

static int mtdpstore_block_is_removed(struct mtdpstore_context *cxt,
		loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	u32 zonecnt = mtd->erasesize / cxt->info.kmsg_size;
	u64 zonenum;

	off = ALIGN_DOWN(off, mtd->erasesize);
	zonenum = div_u64(off, cxt->info.kmsg_size);
	while (zonecnt > 0) {
		if (test_bit(zonenum, cxt->rmmap))
			return true;
		zonenum++;
		zonecnt--;
	}
	return false;
}

static int mtdpstore_erase_do(struct mtdpstore_context *cxt, loff_t off)
{
	struct mtd_info *mtd = cxt->mtd;
	struct erase_info erase;
	int ret;

	off = ALIGN_DOWN(off, cxt->mtd->erasesize);
	dev_dbg(&mtd->dev, "try to erase off 0x%llx\n", off);
	erase.len = cxt->mtd->erasesize;
	erase.addr = off;
	ret = mtd_erase(cxt->mtd, &erase);
	if (!ret)
		mtdpstore_block_clear_removed(cxt, off);
	else
		dev_err(&mtd->dev, "erase of region [0x%llx, 0x%llx] on \"%s\" failed\n",
		       (unsigned long long)erase.addr,
		       (unsigned long long)erase.len, cxt->info.device);
	return ret;
}

/*
 * called while removing file
 *
 * Avoiding over erasing, do erase block only when the whole block is unused.
 * If the block contains valid log, do erase lazily on flush_removed() when
 * unregister.
 */
static ssize_t mtdpstore_erase(size_t size, loff_t off)
{
	struct mtdpstore_context *cxt = &oops_cxt;

	if (mtdpstore_block_isbad(cxt, off))
		return -EIO;

	mtdpstore_mark_unused(cxt, off);

	/* If the block still has valid data, mtdpstore do erase lazily */
	if (likely(mtdpstore_block_is_used(cxt, off))) {
		mtdpstore_mark_removed(cxt, off);
		return 0;
	}

	/* all zones are unused, erase it */
	return mtdpstore_erase_do(cxt, off);
}

/*
 * What is security for mtdpstore?
 * As there is no erase for panic case, we should ensure at least one zone
 * is writable. Otherwise, panic write will fail.
 * If zone is used, write operation will return -ENOMSG, which means that
 * pstore/blk will try one by one until gets an empty zone. So, it is not
 * needed to ensure the next zone is empty, but at least one.
 */
static int mtdpstore_security(struct mtdpstore_context *cxt, loff_t off)
{
	int ret = 0, i;
	struct mtd_info *mtd = cxt->mtd;
	u32 zonenum = (u32)div_u64(off, cxt->info.kmsg_size);
	u32 zonecnt = (u32)div_u64(cxt->mtd->size, cxt->info.kmsg_size);
	u32 blkcnt = (u32)div_u64(cxt->mtd->size, cxt->mtd->erasesize);
	u32 erasesize = cxt->mtd->erasesize;

	for (i = 0; i < zonecnt; i++) {
		u32 num = (zonenum + i) % zonecnt;

		/* found empty zone */
		if (!test_bit(num, cxt->usedmap))
			return 0;
	}

	/* If there is no any empty zone, we have no way but to do erase */
	while (blkcnt--) {
		div64_u64_rem(off + erasesize, cxt->mtd->size, (u64 *)&off);

		if (mtdpstore_block_isbad(cxt, off))
			continue;

		ret = mtdpstore_erase_do(cxt, off);
		if (!ret) {
			mtdpstore_block_mark_unused(cxt, off);
			break;
		}
	}

	if (ret)
		dev_err(&mtd->dev, "all blocks bad!\n");
	dev_dbg(&mtd->dev, "end security\n");
	return ret;
}

static ssize_t mtdpstore_write(const char *buf, size_t size, loff_t off)
{
	struct mtdpstore_context *cxt = &oops_cxt;
	struct mtd_info *mtd = cxt->mtd;
	size_t retlen;
	int ret;

	if (mtdpstore_block_isbad(cxt, off))
		return -ENOMSG;

	/* zone is used, please try next one */
	if (mtdpstore_is_used(cxt, off))
		return -ENOMSG;

	dev_dbg(&mtd->dev, "try to write off 0x%llx size %zu\n", off, size);
	ret = mtd_write(cxt->mtd, off, size, &retlen, (u_char *)buf);
	if (ret < 0 || retlen != size) {
		dev_err(&mtd->dev, "write failure at %lld (%zu of %zu written), err %d\n",
				off, retlen, size, ret);
		return -EIO;
	}
	mtdpstore_mark_used(cxt, off);

	mtdpstore_security(cxt, off);
	return retlen;
}

static inline bool mtdpstore_is_io_error(int ret)
{
	return ret < 0 && !mtd_is_bitflip(ret) && !mtd_is_eccerr(ret);
}

/*
 * All zones will be read as pstore/blk will read zone one by one when do
 * recover.
 */
static ssize_t mtdpstore_read(char *buf, size_t size, loff_t off)
{
	struct mtdpstore_context *cxt = &oops_cxt;
	struct mtd_info *mtd = cxt->mtd;
	size_t retlen, done;
	int ret;

	if (mtdpstore_block_isbad(cxt, off))
		return -ENOMSG;

	dev_dbg(&mtd->dev, "try to read off 0x%llx size %zu\n", off, size);
	for (done = 0, retlen = 0; done < size; done += retlen) {
		retlen = 0;

		ret = mtd_read(cxt->mtd, off + done, size - done, &retlen,
				(u_char *)buf + done);
		if (mtdpstore_is_io_error(ret)) {
			dev_err(&mtd->dev, "read failure at %lld (%zu of %zu read), err %d\n",
					off + done, retlen, size - done, ret);
			/* the zone may be broken, try next one */
			return -ENOMSG;
		}

		/*
		 * ECC error. The impact on log data is so small. Maybe we can
		 * still read it and try to understand. So mtdpstore just hands
		 * over what it gets and user can judge whether the data is
		 * valid or not.
		 */
		if (mtd_is_eccerr(ret)) {
			dev_err(&mtd->dev, "ecc error at %lld (%zu of %zu read), err %d\n",
					off + done, retlen, size - done, ret);
			/* driver may not set retlen when ecc error */
			retlen = retlen == 0 ? size - done : retlen;
		}
	}

	if (mtdpstore_is_empty(cxt, buf, size))
		mtdpstore_mark_unused(cxt, off);
	else
		mtdpstore_mark_used(cxt, off);

	mtdpstore_security(cxt, off);
	return retlen;
}

static ssize_t mtdpstore_panic_write(const char *buf, size_t size, loff_t off)
{
	struct mtdpstore_context *cxt = &oops_cxt;
	struct mtd_info *mtd = cxt->mtd;
	size_t retlen;
	int ret;

	if (mtdpstore_panic_block_isbad(cxt, off))
		return -ENOMSG;

	/* zone is used, please try next one */
	if (mtdpstore_is_used(cxt, off))
		return -ENOMSG;

	ret = mtd_panic_write(cxt->mtd, off, size, &retlen, (u_char *)buf);
	/*if low-level mtd driver not support ->_panic_write, we try ->_write*/
	if (ret == -EOPNOTSUPP)
		ret = mtd_write(cxt->mtd, off, size, &retlen, (u_char *)buf);
	if (ret < 0 || size != retlen) {
		dev_err(&mtd->dev, "panic write failure at %lld (%zu of %zu read), err %d\n",
				off, retlen, size, ret);
		return -EIO;
	}
	mtdpstore_mark_used(cxt, off);

	return retlen;
}

/* get information of pstore/mtdev */
int mtdpstore_get_config(struct mtdpstore_config *info)
{
	strncpy(info->device, mtddev, 80);
	info->max_reason = max_reason;
	info->kmsg_size = check_size(kmsg_size, 4096);
	info->pmsg_size = check_size(pmsg_size, 4096);
	info->ftrace_size = check_size(ftrace_size, 4096);
	info->console_size = check_size(console_size, 4096);

	return 0;
}

static int mtdpstore_register_device(struct pstore_device_info *dev)
{
	int ret;

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

	/* Initialize required zone ownership details. */
	dev->zone.name = KBUILD_MODNAME;
	dev->zone.owner = THIS_MODULE;

	/* Copy in module parameters. */
	verify_size(kmsg_size, 4096, dev->flags & PSTORE_FLAGS_DMESG);
	verify_size(pmsg_size, 4096, dev->flags & PSTORE_FLAGS_PMSG);
	verify_size(console_size, 4096, dev->flags & PSTORE_FLAGS_CONSOLE);
	verify_size(ftrace_size, 4096, dev->flags & PSTORE_FLAGS_FTRACE);
	dev->zone.max_reason = max_reason;
	if (!kmsg_size) {
		pr_err("no backend enabled (kmsg_size is 0)\n");
		return -EINVAL;
	}

	ret = register_pstore_zone(&dev->zone);
	if (ret == 0)
		pstore_device_info = dev;

	return ret;
}

static void mtdpstore_notify_add(struct mtd_info *mtd)
{
	int ret;
	struct mtdpstore_context *cxt = &oops_cxt;
	struct mtdpstore_config *info = &cxt->info;
	unsigned long longcnt;

	mutex_lock(&aw_psmtd_lock);

	if (!strcmp(mtd->name, info->device))
		cxt->index = mtd->index;

	if (mtd->index != cxt->index || cxt->index < 0)
		return;

	dev_dbg(&mtd->dev, "found matching MTD device %s\n", mtd->name);

	if (mtd->size < info->kmsg_size * 2) {
		dev_err(&mtd->dev, "MTD partition %d not big enough\n",
				mtd->index);
		return;
	}
	/*
	 * kmsg_size must be aligned to 4096 Bytes, which is limited by
	 * psblk. The default value of kmsg_size is 64KB. If kmsg_size
	 * is larger than erasesize, some errors will occur since mtdpsotre
	 * is designed on it.
	 */
	if (mtd->erasesize < info->kmsg_size) {
		dev_err(&mtd->dev, "eraseblock size of MTD partition %d too small\n",
				mtd->index);
		return;
	}
	if (unlikely(info->kmsg_size % mtd->writesize)) {
		dev_err(&mtd->dev, "record size %lu KB must align to write size %d KB\n",
				info->kmsg_size / 1024,
				mtd->writesize / 1024);
		return;
	}

	longcnt = BITS_TO_LONGS(div_u64(mtd->size, info->kmsg_size));
	cxt->rmmap = kcalloc(longcnt, sizeof(long), GFP_KERNEL);
	cxt->usedmap = kcalloc(longcnt, sizeof(long), GFP_KERNEL);

	longcnt = BITS_TO_LONGS(div_u64(mtd->size, mtd->erasesize));
	cxt->badmap = kcalloc(longcnt, sizeof(long), GFP_KERNEL);

	/* just support dmesg right now */
	cxt->dev.flags = PSTORE_FLAGS_DMESG;
	cxt->dev.zone.read = mtdpstore_read;
	cxt->dev.zone.write = mtdpstore_write;
	cxt->dev.zone.erase = mtdpstore_erase;
	cxt->dev.zone.panic_write = mtdpstore_panic_write;
	cxt->dev.zone.total_size = mtd->size;

	ret = mtdpstore_register_device(&cxt->dev);
	if (ret) {
		dev_err(&mtd->dev, "mtd%d register to psblk failed\n",
				mtd->index);
		return;
	}
	cxt->mtd = mtd;
	dev_info(&mtd->dev, "Attached to MTD device %d\n", mtd->index);

	mutex_unlock(&aw_psmtd_lock);
}

static int mtdpstore_flush_removed_do(struct mtdpstore_context *cxt,
		loff_t off, size_t size)
{
	struct mtd_info *mtd = cxt->mtd;
	u_char *buf;
	int ret;
	size_t retlen;
	struct erase_info erase;

	buf = kmalloc(mtd->erasesize, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* 1st. read to cache */
	ret = mtd_read(mtd, off, mtd->erasesize, &retlen, buf);
	if (mtdpstore_is_io_error(ret))
		goto free;

	/* 2nd. erase block */
	erase.len = mtd->erasesize;
	erase.addr = off;
	ret = mtd_erase(mtd, &erase);
	if (ret)
		goto free;

	/* 3rd. write back */
	while (size) {
		unsigned int zonesize = cxt->info.kmsg_size;

		/* there is valid data on block, write back */
		if (mtdpstore_is_used(cxt, off)) {
			ret = mtd_write(mtd, off, zonesize, &retlen, buf);
			if (ret)
				dev_err(&mtd->dev, "write failure at %lld (%zu of %u written), err %d\n",
						off, retlen, zonesize, ret);
		}

		off += zonesize;
		size -= min_t(unsigned int, zonesize, size);
	}

free:
	kfree(buf);
	return ret;
}

/*
 * What does mtdpstore_flush_removed() do?
 * When user remove any log file on pstore filesystem, mtdpstore should do
 * something to ensure log file removed. If the whole block is no longer used,
 * it's nice to erase the block. However if the block still contains valid log,
 * what mtdpstore can do is to erase and write the valid log back.
 */
static int mtdpstore_flush_removed(struct mtdpstore_context *cxt)
{
	struct mtd_info *mtd = cxt->mtd;
	int ret;
	loff_t off;
	u32 blkcnt = (u32)div_u64(mtd->size, mtd->erasesize);

	for (off = 0; blkcnt > 0; blkcnt--, off += mtd->erasesize) {
		ret = mtdpstore_block_isbad(cxt, off);
		if (ret)
			continue;

		ret = mtdpstore_block_is_removed(cxt, off);
		if (!ret)
			continue;

		ret = mtdpstore_flush_removed_do(cxt, off, mtd->erasesize);
		if (ret)
			return ret;
	}
	return 0;
}

/**
 * aw_unregister_pstore_device() - unregister device from pstore/blk(mtd)
 *
 * @dev: non-block device information
 */
void mtdpstore_unregister_device(struct pstore_device_info *dev)
{
	if (pstore_device_info && pstore_device_info == dev) {
		unregister_pstore_zone(&dev->zone);
		pstore_device_info = NULL;
	}
}

static void mtdpstore_notify_remove(struct mtd_info *mtd)
{
	struct mtdpstore_context *cxt = &oops_cxt;

	mutex_lock(&aw_psmtd_lock);

	if (mtd->index != cxt->index || cxt->index < 0)
		return;

	mtdpstore_flush_removed(cxt);

	mtdpstore_unregister_device(&cxt->dev);
	kfree(cxt->badmap);
	kfree(cxt->usedmap);
	kfree(cxt->rmmap);
	cxt->mtd = NULL;
	cxt->index = -1;

	mutex_unlock(&aw_psmtd_lock);
}

static struct mtd_notifier mtdpstore_notifier = {
	.add	= mtdpstore_notify_add,
	.remove	= mtdpstore_notify_remove,
};

static int __init mtdpstore_init(void)
{
	int ret;
	struct mtdpstore_context *cxt = &oops_cxt;
	struct mtdpstore_config *info = &cxt->info;

	/* Reject an empty mtddev. */
	if (!mtddev[0]) {
		pr_err("mtd device must be supplied (device name is empty)\n");
		return -EINVAL;
	}

	mtdpstore_get_config(info);

	/* Setup the MTD device to use */
	ret = kstrtoint((char *)info->device, 0, &cxt->index);
	if (ret)
		cxt->index = -1;

	register_mtd_user(&mtdpstore_notifier);
	return 0;
}
module_init(mtdpstore_init);

static void __exit mtdpstore_exit(void)
{
	unregister_mtd_user(&mtdpstore_notifier);
}
module_exit(mtdpstore_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WeiXiong Liao <liaoweixiong@allwinnertech.com>");
MODULE_DESCRIPTION("MTD backend for pstore/blk");
MODULE_VERSION("1.0.0");
