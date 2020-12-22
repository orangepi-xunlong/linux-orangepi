/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 * Copyright 2005-2008 Pierre Ossman
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/string_helpers.h>
#include <linux/delay.h>
#include <linux/capability.h>
#include <linux/compat.h>

#define CREATE_TRACE_POINTS
#include <trace/events/mmc.h>

#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <asm/uaccess.h>

#ifdef eMMC_Magician_SDK_Changes
#include <linux/delay.h>
#endif
#include "queue.h"

MODULE_ALIAS("mmc:block");
#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "mmcblk."

#define INAND_CMD38_ARG_EXT_CSD  113
#define INAND_CMD38_ARG_ERASE    0x00
#define INAND_CMD38_ARG_TRIM     0x01
#define INAND_CMD38_ARG_SECERASE 0x80
#define INAND_CMD38_ARG_SECTRIM1 0x81
#define INAND_CMD38_ARG_SECTRIM2 0x88

//#define MMC_BLK_TIMEOUT_MS  (10 * 60 * 1000)        /* 10 minute timeout */
#define MMC_SANITIZE_REQ_TIMEOUT 240000
#define MMC_EXTRACT_INDEX_FROM_ARG(x) ((x & 0x00FF0000) >> 16)






static DEFINE_MUTEX(block_mutex);

/*
 * The defaults come from config options but can be overriden by module
 * or bootarg options.
 */
static int perdev_minors = CONFIG_MMC_BLOCK_MINORS;

/*
 * We've only got one major, so number of mmcblk devices is
 * limited to 256 / number of minors per device.
 */
static int max_devices;

/* 256 minors, so at most 256 separate devices */
static DECLARE_BITMAP(dev_use, 256);
static DECLARE_BITMAP(name_use, 256);

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct mmc_queue queue;
	struct list_head part;

	unsigned int	flags;
#define MMC_BLK_CMD23	(1 << 0)	/* Can do SET_BLOCK_COUNT for multiblock */
#define MMC_BLK_REL_WR	(1 << 1)	/* MMC Reliable write support */

	unsigned int	usage;
	unsigned int	read_only;
	unsigned int	part_type;
	unsigned int	name_idx;
	unsigned int	reset_done;
#define MMC_BLK_READ		BIT(0)
#define MMC_BLK_WRITE		BIT(1)
#define MMC_BLK_DISCARD		BIT(2)
#define MMC_BLK_SECDISCARD	BIT(3)

	/*
	 * Only set in main mmc_blk_data associated
	 * with mmc_card with mmc_set_drvdata, and keeps
	 * track of the current selected device partition.
	 */
	unsigned int	part_curr;
	struct device_attribute force_ro;
	struct device_attribute power_ro_lock;
	int	area_type;
};

static DEFINE_MUTEX(open_lock);

enum mmc_blk_status {
	MMC_BLK_SUCCESS = 0,
	MMC_BLK_PARTIAL,
	MMC_BLK_CMD_ERR,
	MMC_BLK_RETRY,
	MMC_BLK_ABORT,
	MMC_BLK_DATA_ERR,
	MMC_BLK_ECC_ERR,
	MMC_BLK_NOMEDIUM,
};

module_param(perdev_minors, int, 0444);
MODULE_PARM_DESC(perdev_minors, "Minors numbers to allocate per device");

static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
	struct mmc_blk_data *md;

	mutex_lock(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	mutex_unlock(&open_lock);

	return md;
}

static inline int mmc_get_devidx(struct gendisk *disk)
{
#ifndef eMMC_Magician_SDK_Changes
	int devidx = disk->first_minor / perdev_minors;
#else
	int devmaj = MAJOR(disk_devt(disk));
	int devidx = MINOR(disk_devt(disk)) / perdev_minors;
	if (!devmaj)
		devidx = disk->first_minor / perdev_minors;
#endif
	return devidx;
}

static void mmc_blk_put(struct mmc_blk_data *md)
{
	mutex_lock(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		int devidx = mmc_get_devidx(md->disk);
		blk_cleanup_queue(md->queue.queue);

		__clear_bit(devidx, dev_use);

		put_disk(md->disk);
		kfree(md);
	}
	mutex_unlock(&open_lock);
}

static ssize_t power_ro_lock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	struct mmc_card *card = md->queue.card;
	int locked = 0;

	if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PERM_WP_EN)
		locked = 2;
	else if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_EN)
		locked = 1;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", locked);

	mmc_blk_put(md);

	return ret;
}

static ssize_t power_ro_lock_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	struct mmc_blk_data *md, *part_md;
	struct mmc_card *card;
	unsigned long set;

	if (kstrtoul(buf, 0, &set))
		return -EINVAL;

	if (set != 1)
		return count;

	md = mmc_blk_get(dev_to_disk(dev));
	card = md->queue.card;

	mmc_claim_host(card->host);

	ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_WP,
				card->ext_csd.boot_ro_lock |
				EXT_CSD_BOOT_WP_B_PWR_WP_EN,
				card->ext_csd.part_time);
	if (ret)
		pr_err("%s: Locking boot partition ro until next power on failed: %d\n", md->disk->disk_name, ret);
	else
		card->ext_csd.boot_ro_lock |= EXT_CSD_BOOT_WP_B_PWR_WP_EN;

	mmc_release_host(card->host);

	if (!ret) {
		pr_info("%s: Locking boot partition ro until next power on\n",
			md->disk->disk_name);
		set_disk_ro(md->disk, 1);

		list_for_each_entry(part_md, &md->part, part)
			if (part_md->area_type == MMC_BLK_DATA_AREA_BOOT) {
				pr_info("%s: Locking boot partition ro until next power on\n", part_md->disk->disk_name);
				set_disk_ro(part_md->disk, 1);
			}
	}

	mmc_blk_put(md);
	return count;
}

static ssize_t force_ro_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	int ret;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));

	ret = snprintf(buf, PAGE_SIZE, "%d",
		       get_disk_ro(dev_to_disk(dev)) ^
		       md->read_only);
	mmc_blk_put(md);
	return ret;
}

static ssize_t force_ro_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	char *end;
	struct mmc_blk_data *md = mmc_blk_get(dev_to_disk(dev));
	unsigned long set = simple_strtoul(buf, &end, 0);
	if (end == buf) {
		ret = -EINVAL;
		goto out;
	}

	set_disk_ro(dev_to_disk(dev), set || md->read_only);
	ret = count;
out:
	mmc_blk_put(md);
	return ret;
}

static int mmc_blk_open(struct block_device *bdev, fmode_t mode)
{
	struct mmc_blk_data *md = mmc_blk_get(bdev->bd_disk);
	int ret = -ENXIO;

	mutex_lock(&block_mutex);
	if (md) {
		if (md->usage == 2)
			check_disk_change(bdev);
		ret = 0;

		if ((mode & FMODE_WRITE) && md->read_only) {
			mmc_blk_put(md);
			ret = -EROFS;
		}
	}
	mutex_unlock(&block_mutex);

	return ret;
}

static int mmc_blk_release(struct gendisk *disk, fmode_t mode)
{
	struct mmc_blk_data *md = disk->private_data;

	mutex_lock(&block_mutex);
	mmc_blk_put(md);
	mutex_unlock(&block_mutex);
	return 0;
}

static int
mmc_blk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->cylinders = get_capacity(bdev->bd_disk) / (4 * 16);
	geo->heads = 4;
	geo->sectors = 16;
	return 0;
}

struct mmc_blk_ioc_data {
	struct mmc_ioc_cmd ic;
	unsigned char *buf;
	u64 buf_bytes;
};

static struct mmc_blk_ioc_data *mmc_blk_ioctl_copy_from_user(
	struct mmc_ioc_cmd __user *user)
{
	struct mmc_blk_ioc_data *idata;
	int err;

	idata = kzalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata) {
		err = -ENOMEM;
		goto out;
	}

	if (copy_from_user(&idata->ic, user, sizeof(idata->ic))) {
		err = -EFAULT;
		goto idata_err;
	}

	idata->buf_bytes = (u64) idata->ic.blksz * idata->ic.blocks;
	if (idata->buf_bytes > MMC_IOC_MAX_BYTES) {
		err = -EOVERFLOW;
		goto idata_err;
	}

	if (!idata->buf_bytes)
		return idata;

	idata->buf = kzalloc(idata->buf_bytes, GFP_KERNEL);
	if (!idata->buf) {
		err = -ENOMEM;
		goto idata_err;
	}

	if (copy_from_user(idata->buf, (void __user *)(unsigned long)
					idata->ic.data_ptr, idata->buf_bytes)) {
		err = -EFAULT;
		goto copy_err;
	}

	return idata;

copy_err:
	kfree(idata->buf);
idata_err:
	kfree(idata);
out:
	return ERR_PTR(err);
}

static int ioctl_do_sanitize(struct mmc_card *card)
{
	int err;

	if (!mmc_can_sanitize(card)) {
			pr_warn("%s: %s - SANITIZE is not supported\n",
				mmc_hostname(card->host), __func__);
			err = -EOPNOTSUPP;
			goto out;
	}

	pr_info("%s: %s - SANITIZE IN PROGRESS...\n",
		mmc_hostname(card->host), __func__);

	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_SANITIZE_START, 1,
					MMC_SANITIZE_REQ_TIMEOUT);

	if (err)
		pr_err("%s: %s - EXT_CSD_SANITIZE_START failed. err=%d\n",
		       mmc_hostname(card->host), __func__, err);

	pr_info("%s: %s - SANITIZE COMPLETED\n", mmc_hostname(card->host),
					     __func__);
out:
	return err;
}

static int mmc_blk_ioctl_cmd(struct block_device *bdev,
	struct mmc_ioc_cmd __user *ic_ptr)
{
	struct mmc_blk_ioc_data *idata;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {NULL};
	struct scatterlist sg;
	int err;

	/*
	 * The caller must have CAP_SYS_RAWIO, and must be calling this on the
	 * whole block device, not on a partition.  This prevents overspray
	 * between sibling partitions.
	 */
	if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
		return -EPERM;

	idata = mmc_blk_ioctl_copy_from_user(ic_ptr);
	if (IS_ERR(idata))
		return PTR_ERR(idata);

	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		err = -EINVAL;
		goto cmd_done;
	}

	card = md->queue.card;
	if (IS_ERR(card)) {
		err = PTR_ERR(card);
		goto cmd_done;
	}

	cmd.opcode = idata->ic.opcode;
	cmd.arg = idata->ic.arg;
	cmd.flags = idata->ic.flags;

	if (idata->buf_bytes) {
		data.sg = &sg;
		data.sg_len = 1;
		data.blksz = idata->ic.blksz;
		data.blocks = idata->ic.blocks;

		sg_init_one(data.sg, idata->buf, idata->buf_bytes);

		if (idata->ic.write_flag)
			data.flags = MMC_DATA_WRITE;
		else
			data.flags = MMC_DATA_READ;

		/* data.flags must already be set before doing this. */
		mmc_set_data_timeout(&data, card);

		/* Allow overriding the timeout_ns for empirical tuning. */
		if (idata->ic.data_timeout_ns)
			data.timeout_ns = idata->ic.data_timeout_ns;

		if ((cmd.flags & MMC_RSP_R1B) == MMC_RSP_R1B) {
			/*
			 * Pretend this is a data transfer and rely on the
			 * host driver to compute timeout.  When all host
			 * drivers support cmd.cmd_timeout for R1B, this
			 * can be changed to:
			 *
			 *     mrq.data = NULL;
			 *     cmd.cmd_timeout = idata->ic.cmd_timeout_ms;
			 */
			data.timeout_ns = idata->ic.cmd_timeout_ms * 1000000;
		}

		mrq.data = &data;
	}

	mrq.cmd = &cmd;

	mmc_claim_host(card->host);

	if (idata->ic.is_acmd) {
		err = mmc_app_cmd(card->host, card);
		if (err)
			goto cmd_rel_host;
	}

	if ((MMC_EXTRACT_INDEX_FROM_ARG(cmd.arg) == EXT_CSD_SANITIZE_START) &&
	    (cmd.opcode == MMC_SWITCH)) {
		err = ioctl_do_sanitize(card);

		if (err)
			pr_err("%s: ioctl_do_sanitize() failed. err = %d",
			       __func__, err);

		goto cmd_rel_host;
	}

	mmc_wait_for_req(card->host, &mrq);

	if (cmd.error) {
		dev_err(mmc_dev(card->host), "%s: cmd error %d\n",
						__func__, cmd.error);
		err = cmd.error;
		goto cmd_rel_host;
	}
	if (data.error) {
		dev_err(mmc_dev(card->host), "%s: data error %d\n",
						__func__, data.error);
		err = data.error;
		goto cmd_rel_host;
	}

	/*
	 * According to the SD specs, some commands require a delay after
	 * issuing the command.
	 */
	if (idata->ic.postsleep_min_us)
		usleep_range(idata->ic.postsleep_min_us, idata->ic.postsleep_max_us);

	if (copy_to_user(&(ic_ptr->response), cmd.resp, sizeof(cmd.resp))) {
		err = -EFAULT;
		goto cmd_rel_host;
	}

	if (!idata->ic.write_flag) {
		if (copy_to_user((void __user *)(unsigned long) idata->ic.data_ptr,
						idata->buf, idata->buf_bytes)) {
			err = -EFAULT;
			goto cmd_rel_host;
		}
	}

cmd_rel_host:
	mmc_release_host(card->host);

cmd_done:
	mmc_blk_put(md);
	kfree(idata->buf);
	kfree(idata);
	return err;
}


static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md);
static int mmc_blk_reset(struct mmc_blk_data *md, struct mmc_host *host,
			 int type);
static inline void mmc_blk_reset_success(struct mmc_blk_data *md, int type);


static int mmc_blk_ioctl_erase_cmd(struct block_device *bdev,
	struct mmc_ioc_erase_cmd __user *ic_ptr)
{
	int err;
	struct mmc_ioc_erase_cmd erase_cmd;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	int arg = 0;
	struct gendisk *disk = bdev->bd_disk;
    __u64 offset, size;
    int part_num  = 0;



    part_num = MINOR(bdev->bd_dev)-disk->first_minor;
    size = (disk->part_tbl->part[part_num]->nr_sects<<9);
    offset = (disk->part_tbl->part[part_num]->start_sect<<9);

    pr_err("\n%s, %d part_num:%d, size:%012llx, offset:%012llx, disk->disk_name:%s\n",
        __func__, __LINE__, part_num, size, offset, disk->disk_name);

	// The caller must have CAP_SYS_RAWIO, and must be calling this on the
	// whole block device, not on a partition.  This prevents overspray
	// between sibling partitions.


	//if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
	//	return -EPERM;

	if (copy_from_user(&erase_cmd, ic_ptr, sizeof(erase_cmd))) {
		err = -EFAULT;
		goto cpy_err;
	}

	//clear all data in the part
	if(erase_cmd.flags == 0xa){
		erase_cmd.start_sec = offset>>9;
		erase_cmd.size_sec = size>>9;
		pr_info("erase whole part, start: %d, size: %d\n",
			erase_cmd.start_sec,erase_cmd.size_sec);
	}else if((erase_cmd.start_sec<(offset>>9))||(erase_cmd.size_sec>(size>>9))) {
		pr_err("start sec or size is over the range of disk\n");
		return -EPERM;
	}

	pr_info("%s start sec %d,size %d sec\n",
		bdev->bd_disk->disk_name,erase_cmd.start_sec,erase_cmd.size_sec);
	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		pr_err("%s, failed to get mmc blk\n", __func__);
		err = -EINVAL;
		goto blk_get_err;
	}

	card = md->queue.card;

	if(!mmc_card_mmc(card)){
		pr_err("No mmc, do nothing\n");
		err = 0;
		goto dev_card_err;
	}


	if (!mmc_can_erase(card)) {
		pr_err("device do not support erase, do nothing\n");
		err = 0;
		goto dev_card_err;
	}

	mmc_claim_host(card->host);

	err = mmc_blk_part_switch(card, md);
	if (err){
		pr_err("Part switch failed\n");
		goto cmd_rel_host;
	 }

	//force erase here
	//	if (mmc_can_discard(card))
	//		arg = MMC_DISCARD_ARG;
	//	else if (mmc_can_trim(card))
	//		arg = MMC_TRIM_ARG;
	//	else
			arg = MMC_ERASE_ARG;
	pr_err("%s %d: erase arg: 0x%x\n", __FUNCTION__, __LINE__, arg);

retry:

	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
				pr_err("checked MMC_QUIRK_INAND_CMD38 here\n");
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_TRIM_ARG ?
				 INAND_CMD38_ARG_TRIM :
				 INAND_CMD38_ARG_ERASE,
				 0);
		if (err)
			goto out;
	}


	err = mmc_erase(card, erase_cmd.start_sec, erase_cmd.size_sec, arg);
out:
	if (err == -EIO && !mmc_blk_reset(md, card->host, MMC_BLK_DISCARD)){
		pr_err("Erase failed and retry here\n");
		goto retry;
	}

	if (!err){
		mmc_blk_reset_success(md, MMC_BLK_DISCARD);
	}
	else{
		pr_err("Erase failed and err:%d\n", err);
	}

cmd_rel_host:
	mmc_release_host(card->host);

dev_card_err:
	mmc_blk_put(md);

blk_get_err:
	pr_err("%s completed, err:%d \n", __func__, err);

cpy_err:

	return err;

}

static int mmc_do_secure_wipe(struct mmc_card *card, unsigned int from,
	unsigned int nr)
{
	int err;
	unsigned int arg = 0;

	pr_info("%s: start %s...\n", mmc_hostname(card->host), __func__);
	if (nr == 0) {
		pr_info("%s:%s: on space need to erase, nr %d\n",
			mmc_hostname(card->host), __func__, nr);
		return 0;
	}

	if (card->ext_csd.rev <= 5) /* ver 4.41 or older */
	{
		if (mmc_can_secure_erase_trim(card) && mmc_can_trim(card))
		{
			/* support secure trim operation */
			pr_info("%s:%s: start secure trim...\n",
				mmc_hostname(card->host), __func__);
			arg = MMC_SECURE_TRIM1_ARG;
			if (card->quirks & MMC_QUIRK_INAND_CMD38) {
				err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
						 INAND_CMD38_ARG_EXT_CSD,
						 arg == MMC_SECURE_TRIM1_ARG ?
						 INAND_CMD38_ARG_SECTRIM1 :
						 INAND_CMD38_ARG_SECERASE,
						 0);
				if (err) {
					pr_err("%s:%s: switch for secure trim 1 fail.\n",
						mmc_hostname(card->host), __func__);
					goto out;
				}
			}
			err = mmc_erase(card, from, nr, arg);
			if (err) {
				pr_info("%s:%s: secure trim 1 fail\n",
					mmc_hostname(card->host), __func__);
				goto out;
			}

			if (!err && arg == MMC_SECURE_TRIM1_ARG) {
				if (card->quirks & MMC_QUIRK_INAND_CMD38) {
					err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
							 INAND_CMD38_ARG_EXT_CSD,
							 INAND_CMD38_ARG_SECTRIM2,
							 0);
					if (err) {
						pr_err("%s:%s: switch for secure trim 2 fail.\n",
							mmc_hostname(card->host), __func__);
						goto out;
					}
				}
				err = mmc_erase(card, from, nr, MMC_SECURE_TRIM2_ARG);
				if (err) {
					pr_err("%s:%s: secure trim 2 fail\n",
						mmc_hostname(card->host), __func__);
					goto out;
				}
			}
		}
		else if (mmc_can_secure_erase_trim(card) && mmc_can_erase(card))
		{
			/* support secure erase operation */
			pr_info("%s:%s: start secure erase...\n",
				mmc_hostname(card->host), __func__);
			if (!mmc_erase_group_aligned(card, from, nr)) {
				pr_info("%s:%s: not erase group aligned, group:%d!!\n",
					mmc_hostname(card->host), __func__, card->erase_size);
			}
			arg = MMC_SECURE_ERASE_ARG;
			if (card->quirks & MMC_QUIRK_INAND_CMD38) {
				err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
						 INAND_CMD38_ARG_EXT_CSD,
						 arg == MMC_SECURE_TRIM1_ARG ?
						 INAND_CMD38_ARG_SECTRIM1 :
						 INAND_CMD38_ARG_SECERASE,
						 0);
				if (err) {
					pr_err("%s:%s: switch for secure erase fail.\n",
						mmc_hostname(card->host), __func__);
					goto out;
				}
			}
			err = mmc_erase(card, from, nr, arg);
			if (err) {
				pr_err("%s:%s: secure erase fail\n",
					mmc_hostname(card->host), __func__);
				goto out;
			}

		}
		else if (mmc_can_trim(card))
		{
			/* support insecure trim operation */
			pr_info("%s:%s: start trim...\n",
				mmc_hostname(card->host), __func__);
			arg = MMC_TRIM_ARG;
			if (card->quirks & MMC_QUIRK_INAND_CMD38) {
				err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 arg == MMC_TRIM_ARG ?
					 INAND_CMD38_ARG_TRIM :
					 INAND_CMD38_ARG_ERASE,
					 0);
				if (err) {
					pr_err("%s:%s: switch for trim fail.\n",
						mmc_hostname(card->host), __func__);
					goto out;
				}
			}
			err = mmc_erase(card, from, nr, arg);
			if (err) {
				pr_err("%s:%s: trim fail\n",
					mmc_hostname(card->host), __func__);
				goto out;
			}
		}
		else
		{
			pr_err("%s:%s: no method to wipe data for current emmc %d(<=v4.41)!\n",
				mmc_hostname(card->host), __func__, card->ext_csd.rev);
			err = -EOPNOTSUPP;
			goto out;
		}
	}
	else if (card->ext_csd.rev <= 7) /* v4.5 or newer */
	{
		if (!mmc_can_sanitize(card)
			|| (!mmc_can_trim(card) && !mmc_can_erase(card)))
		{
			pr_err("%s:%s: no method to wipe data for current emmc %d(>v4.5)!\n",
				mmc_hostname(card->host), __func__, card->ext_csd.rev);
			err = -EOPNOTSUPP;
			goto out;
		}

		if (mmc_can_trim(card))
		{
			pr_info("%s:%s: start trim...\n",
				mmc_hostname(card->host), __func__);
			arg = MMC_TRIM_ARG;
			if (card->quirks & MMC_QUIRK_INAND_CMD38) {
				err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 arg == MMC_TRIM_ARG ?
					 INAND_CMD38_ARG_TRIM :
					 INAND_CMD38_ARG_ERASE,
					 0);
				if (err) {
					pr_err("%s:%s: switch for trim fail.\n",
						mmc_hostname(card->host), __func__);
					goto out;
				}
			}
			err = mmc_erase(card, from, nr, arg);
			if (err) {
				pr_err("%s:%s: trim fail\n", mmc_hostname(card->host),
					__func__);
				goto out;
			}
		}
		else // if (mmc_can_erase(card))
		{
			pr_info("%s:%s: start erase...\n",
				mmc_hostname(card->host), __func__);
			if (!mmc_erase_group_aligned(card, from, nr)) {
				pr_info("%s:%s: not erase group aligned, group:%d!!\n",
					mmc_hostname(card->host), __func__, card->erase_size);
			}
			arg = MMC_ERASE_ARG;
			if (card->quirks & MMC_QUIRK_INAND_CMD38) {
				err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 arg == MMC_TRIM_ARG ?
					 INAND_CMD38_ARG_TRIM :
					 INAND_CMD38_ARG_ERASE,
					 0);
				if (err) {
					pr_err("%s:%s: switch for erase fail.\n",
						mmc_hostname(card->host), __func__);
					goto out;
				}
			}
			err = mmc_erase(card, from, nr, arg);
			if (err) {
				pr_err("%s:%s: erase fail\n",
					mmc_hostname(card->host), __func__);
				goto out;
			}
		}

		err = ioctl_do_sanitize(card);
		if (err) {
			pr_err("%s:%s: do sanitize failed!!\n",
				mmc_hostname(card->host), __func__);
			goto out;
		}
	}
	else
	{
		pr_err("%s:%s: Unknown mmc version %d\n",
			mmc_hostname(card->host), __func__, card->ext_csd.rev);
		err = -EOPNOTSUPP;
		goto out;
	}

out:
	pr_info("%s: end %s ret: %d\n", mmc_hostname(card->host), __func__, err);
	return err;
}

static int mmc_blk_ioctl_secure_wipe_cmd(struct block_device *bdev,
	struct mmc_ioc_erase_cmd __user *ic_ptr)
{
	int err;
	struct mmc_ioc_erase_cmd erase_cmd;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	struct gendisk *disk = bdev->bd_disk;
	__u64 offset, size;
	int part_num = 0;
	int type = MMC_BLK_SECDISCARD;

	part_num = MINOR(bdev->bd_dev) - disk->first_minor;
	size = (disk->part_tbl->part[part_num]->nr_sects <<9);
	offset = (disk->part_tbl->part[part_num]->start_sect <<9);

	pr_err("%s: part_num:%d, size:0x%012llx, offset:0x%012llx, disk->disk_name:%s\n",
		__func__, part_num, size, offset, disk->disk_name);

	if (copy_from_user(&erase_cmd, ic_ptr, sizeof(erase_cmd))) {
		err = -EFAULT;
		goto cpy_err;
	}

	/* 0xA is a special cmd flag defined by ourselves to erase whole partition */
	if (erase_cmd.flags == 0xA) {
		erase_cmd.start_sec = offset>>9;
		erase_cmd.size_sec = size>>9;
		pr_info("wipe the whole part start 0x%x size 0x%x\n",
			erase_cmd.start_sec, erase_cmd.size_sec);
	} else if ((erase_cmd.start_sec >= (offset>>9))
				&& (erase_cmd.start_sec <= ((offset+size)>>9))
				&& ((erase_cmd.start_sec+erase_cmd.size_sec) <= ((offset+size)>>9))) {
		pr_err("start or size is out of the range of disk\n");
		return -EPERM;
	}

	pr_info("start wipe %s: start: 0x%x sect, size 0x%x sect\n",
		bdev->bd_disk->disk_name, erase_cmd.start_sec,erase_cmd.size_sec);
	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		pr_err("%s, failed to get mmc blk\n", __func__);
		err = -EINVAL;
		goto blk_get_err;
	}
	card = md->queue.card;
	pr_info("%s: erase group size: %d\n",
		mmc_hostname(card->host), card->erase_size);
	if (card->host->platform_cap & MMC_HOST_PLATFORM_CAP_DIS_SECURE_WIPE_OP) {
		pr_err("%s: don't support secure wipe operation!!\n", mmc_hostname(card->host));
		err = -EINVAL;
		goto dev_card_err;
	}

	if (!mmc_card_mmc(card)) {
		pr_err("%s: not mmc, do nothing\n", __func__);
		err = 0;
		goto dev_card_err;
	}

	mmc_claim_host(card->host);

	err = mmc_blk_part_switch(card, md);
	if (err) {
		pr_err("%s: part switch failed\n", __func__);
		goto cmd_rel_host;
	 }

retry:
	 err = mmc_do_secure_wipe(card, erase_cmd.start_sec, erase_cmd.size_sec);
	 if (err) {
		pr_err("%s: secure wipe %s part fail\n",
			__func__, bdev->bd_disk->disk_name);
	 }
	if (err == -EIO && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);

cmd_rel_host:
	mmc_release_host(card->host);

dev_card_err:
	mmc_blk_put(md);

blk_get_err:
	pr_err("%s completed, err:%d \n", __func__, err);

cpy_err:

	return err;

}

#ifdef eMMC_Magician_SDK_Changes
struct mmc_blk_ioc_mag_sdk_data {
	struct mmc_ioc_mag_sdk_cmd ic[MAX_MMC_IOC_MAG_SDK_CMD];
	unsigned char *buf;
	u64 buf_bytes;
};

struct scatterlist *mmc_blk_get_sg(struct mmc_card *card,
		unsigned char *buf, int *sg_len, int size)
{
	struct scatterlist *sg;
	struct scatterlist *sl;
	int total_sec_cnt, sec_cnt;
	int max_seg_size, len;

	total_sec_cnt = size;
	max_seg_size = card->host->max_seg_size;
	len = (size - 1 + max_seg_size) / max_seg_size;
	sl = kmalloc(sizeof(struct scatterlist) * len, GFP_KERNEL);

	if (!sl) {
		return NULL;
	}
	sg = (struct scatterlist *)sl;
	sg_init_table(sg, len);

	while (total_sec_cnt) {
		if (total_sec_cnt < max_seg_size)
			sec_cnt = total_sec_cnt;
		else
			sec_cnt = max_seg_size;
			sg_set_page(sg, virt_to_page(buf), sec_cnt, offset_in_page(buf));
			buf = buf + sec_cnt;
			total_sec_cnt = total_sec_cnt - sec_cnt;
			if (total_sec_cnt == 0)
				break;
			sg = sg_next(sg);
	}

	if (sg)
		sg_mark_end(sg);
	*sg_len = len;
	return sl;
}


static struct mmc_blk_ioc_mag_sdk_data *mmc_blk_mag_sdk_ioctl_copy_from_user(
	struct mmc_ioc_mag_sdk_cmd __user (*user)[], int *ioc_len)
{
	struct mmc_blk_ioc_mag_sdk_data *idata;
	int err;
	int ic_idx;
	int buf_alloc_flag = 0;

	idata = kzalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata)
	{
		err = -ENOMEM;
		MagRelPr("MAG_SDK::Memory allocation failed for idata\n");
		goto out;
	}
	for (ic_idx = 0; ic_idx < MAX_MMC_IOC_MAG_SDK_CMD ; ic_idx++)
	{
		if (copy_from_user(&(idata->ic[ic_idx]), &((*user)[ic_idx]), sizeof(struct mmc_ioc_mag_sdk_cmd)))
		{
			err = -EFAULT;
			MagRelPr("MAG_SDK::copy_from_user failed\n");
			goto idata_err;
		}
		if (idata->ic[ic_idx].opcode == 255)
		{
			*ioc_len = ic_idx;
			MagDbgPr("MAG_SDK::Number of commands passed = %d\n", ic_idx);
			break;
		}
		if ((buf_alloc_flag==0) && (idata->ic[ic_idx].data_ptr != 0)
			&& (idata->ic[ic_idx].flags & MMC_CMD_ADTC))
		{
			buf_alloc_flag = 1;
			idata->buf_bytes = (u64) idata->ic[ic_idx].blksz * idata->ic[ic_idx].blocks;
			MagDbgPr("MAG_SDK::Data length passed in bytes %llu", idata->buf_bytes);

			if (idata->buf_bytes > MMC_IOC_MAG_SDK_MAX_BYTES)
			{
				err = -EOVERFLOW;
				MagRelPr("MAG_SDK::Data length exceeds MMC_IOC_MAG_SDK_MAX_BYTES");
				goto idata_err;
			}

			if (!idata->buf_bytes)
			{
				return idata;
			}

			idata->buf = kzalloc(idata->buf_bytes, GFP_KERNEL);
			if (!idata->buf)
			{
				err = -ENOMEM;
				MagRelPr("MAG_SDK::Memory allocation failed data buffer\n");
				goto idata_err;
			}
			if (idata->ic[ic_idx].write_flag)
			{
				if (copy_from_user(idata->buf, (void __user *)(unsigned long)idata->ic[ic_idx].data_ptr, idata->buf_bytes))
				{
					MagRelPr("MAG_SDK::copy_from_user failed for write data\n");
					err = -EFAULT;
					goto copy_err;
				}
			}
		}
	}

	return idata;

copy_err:
	kfree(idata->buf);
idata_err:
	kfree(idata);
out:
	return ERR_PTR(err);
}

static int mmc_blk_mag_sdk_ioctl_cmd(struct block_device *bdev,struct mmc_ioc_mag_sdk_cmd __user (*ic_ptr)[])
{
	struct mmc_blk_ioc_mag_sdk_data *idata;
	struct mmc_blk_data *md;
	struct mmc_card *card;
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {0};
	struct scatterlist *sg = 0;
	int err = 0;
	int sg_len;
	int ic_len = 0;
	int ic_idx = 0;
	int is_need_to_reset = 0;
	int response = 0;
	int ibytesNotCopied = 0;
	int previous_cmd = 0;
	__u32 execution_status = 0; // 0 - Not executed 1 - success 2- failure

	MagDbgPr("MAG_SDK::Entered %s ...\n", __func__);

	mutex_lock(&block_mutex);
	/*
	 * The caller must have CAP_SYS_RAWIO, and must be calling this on the
	 * whole block device, not on a partition.  This prevents overspray
	 * between sibling partitions.
	 */
	if ((!capable(CAP_SYS_RAWIO)) || (bdev != bdev->bd_contains))
	{
		mutex_unlock(&block_mutex);
		return -EPERM;
	}

	idata = mmc_blk_mag_sdk_ioctl_copy_from_user(ic_ptr, &ic_len);
	if (IS_ERR(idata))
	{
		MagRelPr("MAG_SDK::Error in mmc_blk_mag_sdk_ioctl_copy_from_user\n");
		mutex_unlock(&block_mutex);
		return PTR_ERR(idata);
	}

	md = mmc_blk_get(bdev->bd_disk);
	if (!md)
	{
		MagRelPr("MAG_SDK::Error in getting md\n");
		err = -EINVAL;
		goto cmd_done;
	}

	card = md->queue.card;
	if (IS_ERR(card))
	{
		MagRelPr("MAG_SDK::Error in getting reference to card\n");
		err = PTR_ERR(card);
		goto cmd_done;
	}

	mmc_claim_host(card->host);

	for (ic_idx = 0; ic_idx < ic_len; ic_idx++)
	{
		execution_status = 0;
		sg = NULL;
		memset(&mrq, 0, sizeof(struct mmc_request));
		memset(&cmd, 0, sizeof(struct mmc_command));
		memset(&data, 0, sizeof(struct mmc_data));
		memset(&stop, 0, sizeof(struct mmc_command));
		cmd.opcode = idata->ic[ic_idx].opcode;
		cmd.arg = idata->ic[ic_idx].arg;
		cmd.flags = idata->ic[ic_idx].flags;

		MagDbgPr("MAG_SDK::CMD = %d ARG = 0x%08X\n", cmd.opcode, cmd.arg);

		if (cmd.flags & MMC_CMD_ADTC)
		{
			sg = (struct scatterlist *)mmc_blk_get_sg(card, idata->buf, &sg_len, idata->buf_bytes);

			if (sg == NULL)
			{
				MagRelPr("MAG_SDK::Error in sg\n");
				err = -ENOMEM;
				goto cmd_rel_host;
			}

			data.sg = sg;
			data.sg_len = sg_len;
			data.blksz = idata->ic[ic_idx].blksz;
			data.blocks = idata->ic[ic_idx].blocks;

			if (idata->ic[ic_idx].write_flag)
			{
				data.flags = MMC_DATA_WRITE;
			}
			else
			{
				data.flags = MMC_DATA_READ;
			}

			if (cmd.opcode == MMC_WRITE_MULTIPLE_BLOCK)
			{
				if(previous_cmd != 23)
				{
					MagDbgPr("MAG_SDK::Packing CMD12 as it is open ended transfer\n");
					stop.opcode = MMC_STOP_TRANSMISSION;
					stop.arg = 0;
					stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
					mrq.stop = &stop;
				}
				else
					MagDbgPr("MAG_SDK::Dont pack CMD12 as it is predefined transfer\n");
			}
			else if(cmd.opcode == MMC_READ_MULTIPLE_BLOCK)
			{
				/*
				  WJQ, 20141116
				  must add some delay here!!!

				  magician tool sends serval vendor cmd 62 followed send a cmd 18.
				  during debug, we found if these is no delay before cmd 18, the mmc will have no reponse
				  or reponse timeout error.
				  the delay time value is just a try value. maybe it should be changed on different samsung emmc.
				*/
				mdelay(10000);
				if(previous_cmd != 23)
				{
					MagDbgPr("MAG_SDK::Packing CMD12 as it is open ended transfer\n");
					stop.opcode = MMC_STOP_TRANSMISSION;
					stop.arg = 0;
					stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
					mrq.stop = &stop;
				}
				else
				MagDbgPr("MAG_SDK::Dont pack CMD12 as it is predefined transfer\n");
			}
			mrq.data = &data;
			mmc_set_data_timeout(&data, card);
		}

		if (idata->ic[ic_idx].data_timeout_ns > 0)
		{
			data.timeout_ns = idata->ic[ic_idx].data_timeout_ns;
			data.timeout_clks = 0;
		}

		if (idata->ic[ic_idx].cmd_timeout_ms > 0)
		{
			cmd.cmd_timeout_ms = idata->ic[ic_idx].cmd_timeout_ms;
		}

		mrq.cmd = &cmd;
		mrq.sbc = NULL; // not to use cmd23

		if (idata->ic[ic_idx].is_acmd)
		{
			err = mmc_app_cmd(card->host, card);
			if (err)
			{
				goto cmd_rel_host;
			}
		}

		mmc_wait_for_req(card->host, &mrq);

		if (cmd.error)
		{
			dev_err(mmc_dev(card->host), "MAG_SDK::%s: cmd error %d\n",
							__func__, cmd.error);
			err = cmd.error;
			execution_status = 2;
			ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[4]), &execution_status, sizeof(__u32));
			//goto cmd_rel_host;
			continue;
		}

		if (data.error)
		{
			dev_err(mmc_dev(card->host), "MAG_SDK::%s: data error %d\n",
							__func__, data.error);
			err = data.error;
			execution_status = 2;
			ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[4]), &execution_status, sizeof(__u32));
			//goto cmd_rel_host;
			continue;
		}

		/*
		 * According to the SD specs, some commands require a delay after
		 * issuing the command.
		 */
		if (idata->ic[ic_idx].postsleep_min_us)
		{
			usleep_range(idata->ic[ic_idx].postsleep_min_us, idata->ic[ic_idx].postsleep_max_us);
		}

		if (copy_to_user(&((*ic_ptr)[ic_idx].response), cmd.resp, sizeof(cmd.resp)))
		{
			err = -EFAULT;
			execution_status = 2;
			ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[4]), &execution_status, sizeof(__u32));
			//goto cmd_rel_host;
			continue;
		}

		MagDbgPr("MAG_SDK::Response = 0x%08X\n", (unsigned int)(*ic_ptr)[ic_idx].response[0]);

		if(cmd.opcode == MMC_WRITE_MULTIPLE_BLOCK || cmd.opcode == MMC_READ_MULTIPLE_BLOCK)
		{
			if(previous_cmd != 23)
			{
				MagDbgPr("MAG_SDK::CMD12 Response = 0x%08X\n", stop.resp[0]);
				ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[1]), cmd.resp, sizeof(cmd.resp));
			}
		}

		if (cmd.flags & MMC_CMD_ADTC)
		{
			if (!idata->ic[ic_idx].write_flag)
			{
				ibytesNotCopied = copy_to_user((void __user *)(unsigned long) idata->ic[ic_idx].data_ptr, 
				                           idata->buf, idata->buf_bytes);

				if (ibytesNotCopied != 0)
				{
					MagRelPr ("MAG_SDK::Bytes not copied = %d\n", ibytesNotCopied);
					execution_status = 2;
					ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[4]), &execution_status, sizeof(__u32));
					err = -EFAULT;
					//goto cmd_rel_host;
					continue;
				}
			}
		}

		if (((cmd.opcode == MMC_SET_WRITE_PROT && cmd.arg != 0x000000FA) || (cmd.opcode == MMC_WRITE_MULTIPLE_BLOCK) ||
			(cmd.opcode == MMC_ERASE) || (cmd.opcode == MMC_READ_MULTIPLE_BLOCK)) && (idata->ic[ic_idx].is_not_need_to_cmd13 != 1))
		{
			do
			{
				memset(&cmd, 0, sizeof(struct mmc_command));

				cmd.opcode = MMC_SEND_STATUS;
				if (!mmc_host_is_spi(card->host))
				{
					cmd.arg = card->rca << 16;
				}
				cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
				err = mmc_wait_for_cmd(card->host, &cmd, 0);
				if (err)
				{
					MagRelPr("MAG_SDK::Error %d sending status command", err);
				}
				else
					MagDbgPr("MAG_SDK::CMD13 Response = 0x%08X\n", cmd.resp[0]);

				response = cmd.resp[0] & 0x1F00;

			} while (response != 0x900);

			if (err)
			{
				execution_status = 2;
				ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[4]), &execution_status, sizeof(__u32));
				//goto cmd_rel_host;
				continue;
			}
		}

		if (idata->ic[ic_idx].is_need_to_reset == 1)
		{
			is_need_to_reset = 1;
		}

		if (idata->ic[ic_idx].is_need_to_delay == 1)
		{
			mdelay(2000);
		}
		execution_status = 1;
		ibytesNotCopied = copy_to_user(&((*ic_ptr)[ic_idx].response[4]), &execution_status, sizeof(__u32));
		MagDbgPr("MAG_SDK::execution status = %d\n", (unsigned int)(*ic_ptr)[ic_idx].response[4]);
		previous_cmd = idata->ic[ic_idx].opcode;
	}

	MagDbgPr("MAG_SDK::IOCTL Succeeded\n");

	if (is_need_to_reset)
	{
		MagDbgPr("MAG_SDK::mmc_release_host\n");
		mmc_release_host(card->host);
#ifdef CONFIG_PM
		MagDbgPr("MAG_SDK::mmc_resume_host\n");
		mmc_resume_host(card->host);
#endif 
		MagDbgPr("MAG_SDK::mmc_claim_host\n");
		mmc_claim_host(card->host);
	}

cmd_rel_host:
	mmc_release_host(card->host);

cmd_done:
	if (md)
	{
		mmc_blk_put(md);
	}
	if (sg)
	{
		kfree(sg);
	}

	kfree(idata->buf);
	kfree(idata);
	mutex_unlock(&block_mutex);

	MagDbgPr("MAG_SDK::exit %s, err: %d\n", __func__, err);
	return err;
}

#endif

static int mmc_blk_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	if (cmd == MMC_IOC_CMD)
		ret = mmc_blk_ioctl_cmd(bdev, (struct mmc_ioc_cmd __user *)arg);
	if (cmd == MMC_IOC_ERASE_CMD)
		ret = mmc_blk_ioctl_erase_cmd(bdev, (struct mmc_ioc_erase_cmd __user *)arg);
	if (cmd == MMC_IOC_SECURE_WIPE_CMD)
		ret = mmc_blk_ioctl_secure_wipe_cmd(bdev, (struct mmc_ioc_erase_cmd __user *)arg);
#ifdef eMMC_Magician_SDK_Changes
	if (cmd == MMC_IOC_MAG_SDK_CMD)
		ret = mmc_blk_mag_sdk_ioctl_cmd(bdev, (struct mmc_ioc_mag_sdk_cmd __user (*)[])arg);
#endif

	return ret;
}

#ifdef CONFIG_COMPAT
static int mmc_blk_compat_ioctl(struct block_device *bdev, fmode_t mode,
	unsigned int cmd, unsigned long arg)
{
	return mmc_blk_ioctl(bdev, mode, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static const struct block_device_operations mmc_bdops = {
	.open			= mmc_blk_open,
	.release		= mmc_blk_release,
	.getgeo			= mmc_blk_getgeo,
	.owner			= THIS_MODULE,
	.ioctl			= mmc_blk_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl		= mmc_blk_compat_ioctl,
#endif
};

static inline int mmc_blk_part_switch(struct mmc_card *card,
				      struct mmc_blk_data *md)
{
	int ret;
	struct mmc_blk_data *main_md = mmc_get_drvdata(card);

	if (main_md->part_curr == md->part_type)
		return 0;

	if (mmc_card_mmc(card)) {
		u8 part_config = card->ext_csd.part_config;

		part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		part_config |= md->part_type;

		ret = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_PART_CONFIG, part_config,
				 card->ext_csd.part_time);
		if (ret)
			return ret;

		card->ext_csd.part_config = part_config;
	}

	main_md->part_curr = md->part_type;
	return 0;
}

static u32 mmc_sd_num_wr_blocks(struct mmc_card *card)
{
	int err;
	u32 result;
	__be32 *blocks;

	struct mmc_request mrq = {NULL};
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	unsigned int timeout_us;

	struct scatterlist sg;

	cmd.opcode = MMC_APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_AC;

	err = mmc_wait_for_cmd(card->host, &cmd, 0);
	if (err)
		return (u32)-1;
	if (!mmc_host_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return (u32)-1;

	memset(&cmd, 0, sizeof(struct mmc_command));

	cmd.opcode = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	data.timeout_ns = card->csd.tacc_ns * 100;
	data.timeout_clks = card->csd.tacc_clks * 100;

	timeout_us = data.timeout_ns / 1000;
	timeout_us += data.timeout_clks * 1000 /
		(card->host->ios.clock / 1000);

	if (timeout_us > 100000) {
		data.timeout_ns = 100000000;
		data.timeout_clks = 0;
	}

	data.blksz = 4;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.sg = &sg;
	data.sg_len = 1;

	mrq.cmd = &cmd;
	mrq.data = &data;

	blocks = kmalloc(4, GFP_KERNEL);
	if (!blocks)
		return (u32)-1;

	sg_init_one(&sg, blocks, 4);

	mmc_wait_for_req(card->host, &mrq);

	result = ntohl(*blocks);
	kfree(blocks);

	if (cmd.error || data.error)
		result = (u32)-1;

	return result;
}

static int send_stop(struct mmc_card *card, u32 *status)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_STOP_TRANSMISSION;
	cmd.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, 5);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

static int get_card_status(struct mmc_card *card, u32 *status, int retries)
{
	struct mmc_command cmd = {0};
	int err;

	cmd.opcode = MMC_SEND_STATUS;
	if (!mmc_host_is_spi(card->host))
		cmd.arg = card->rca << 16;
	cmd.flags = MMC_RSP_SPI_R2 | MMC_RSP_R1 | MMC_CMD_AC;
	err = mmc_wait_for_cmd(card->host, &cmd, retries);
	if (err == 0)
		*status = cmd.resp[0];
	return err;
}

#define ERR_NOMEDIUM	3
#define ERR_RETRY	2
#define ERR_ABORT	1
#define ERR_CONTINUE	0

static int mmc_blk_cmd_error(struct request *req, const char *name, int error,
	bool status_valid, u32 status)
{
	switch (error) {
	case -EILSEQ:
		/* response crc error, retry the r/w cmd */
		pr_err("%s: %s sending %s command, card status %#x\n",
			req->rq_disk->disk_name, "response CRC error",
			name, status);
		return ERR_RETRY;

	case -ETIMEDOUT:
		pr_err("%s: %s sending %s command, card status %#x\n",
			req->rq_disk->disk_name, "timed out", name, status);

		/* If the status cmd initially failed, retry the r/w cmd */
		if (!status_valid) {
			pr_err("%s: status not valid, retrying timeout\n", req->rq_disk->disk_name);
			return ERR_RETRY;
		}
		/*
		 * If it was a r/w cmd crc error, or illegal command
		 * (eg, issued in wrong state) then retry - we should
		 * have corrected the state problem above.
		 */
		if (status & (R1_COM_CRC_ERROR | R1_ILLEGAL_COMMAND)) {
			pr_err("%s: command error, retrying timeout\n", req->rq_disk->disk_name);
			return ERR_RETRY;
		}

		/* Otherwise abort the command */
		pr_err("%s: not retrying timeout\n", req->rq_disk->disk_name);
		return ERR_ABORT;

	default:
		/* We don't understand the error code the driver gave us */
		pr_err("%s: unknown error %d sending read/write command, card status %#x\n",
		       req->rq_disk->disk_name, error, status);
		return ERR_ABORT;
	}
}

/*
 * Initial r/w and stop cmd error recovery.
 * We don't know whether the card received the r/w cmd or not, so try to
 * restore things back to a sane state.  Essentially, we do this as follows:
 * - Obtain card status.  If the first attempt to obtain card status fails,
 *   the status word will reflect the failed status cmd, not the failed
 *   r/w cmd.  If we fail to obtain card status, it suggests we can no
 *   longer communicate with the card.
 * - Check the card state.  If the card received the cmd but there was a
 *   transient problem with the response, it might still be in a data transfer
 *   mode.  Try to send it a stop command.  If this fails, we can't recover.
 * - If the r/w cmd failed due to a response CRC error, it was probably
 *   transient, so retry the cmd.
 * - If the r/w cmd timed out, but we didn't get the r/w cmd status, retry.
 * - If the r/w cmd timed out, and the r/w cmd failed due to CRC error or
 *   illegal cmd, retry.
 * Otherwise we don't understand what happened, so abort.
 */
static int mmc_blk_cmd_recovery(struct mmc_card *card, struct request *req,
	struct mmc_blk_request *brq, int *ecc_err, int *gen_err)
{
	bool prev_cmd_status_valid = true;
	u32 status, stop_status = 0;
	int err, retry;

	if (mmc_card_removed(card))
		return ERR_NOMEDIUM;

	/*
	 * Try to get card status which indicates both the card state
	 * and why there was no response.  If the first attempt fails,
	 * we can't be sure the returned status is for the r/w command.
	 */
	for (retry = 2; retry >= 0; retry--) {
		err = get_card_status(card, &status, 0);
		if (!err)
			break;

		prev_cmd_status_valid = false;
		pr_err("%s: error %d sending status command, %sing\n",
		       req->rq_disk->disk_name, err, retry ? "retry" : "abort");
	}

	/* We couldn't get a response from the card.  Give up. */
	if (err) {
		/* Check if the card is removed */
		if (mmc_detect_card_removed(card->host))
			return ERR_NOMEDIUM;
		return ERR_ABORT;
	}

	/* Flag ECC errors */
	if ((status & R1_CARD_ECC_FAILED) ||
	    (brq->stop.resp[0] & R1_CARD_ECC_FAILED) ||
	    (brq->cmd.resp[0] & R1_CARD_ECC_FAILED))
		*ecc_err = 1;

	/* Flag General errors */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ)
		if ((status & R1_ERROR) ||
			(brq->stop.resp[0] & R1_ERROR)) {
			pr_err("%s: %s: general error sending stop or status command, stop cmd response %#x, card status %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0], status);
			*gen_err = 1;
		}

	/*
	 * Check the current card state.  If it is in some data transfer
	 * mode, tell it to stop (and hopefully transition back to TRAN.)
	 */
	if (R1_CURRENT_STATE(status) == R1_STATE_DATA ||
	    R1_CURRENT_STATE(status) == R1_STATE_RCV) {
		err = send_stop(card, &stop_status);
		if (err)
			pr_err("%s: error %d sending stop command\n",
			       req->rq_disk->disk_name, err);

		/*
		 * If the stop cmd also timed out, the card is probably
		 * not present, so abort.  Other errors are bad news too.
		 */
		if (err)
			return ERR_ABORT;
		if (stop_status & R1_CARD_ECC_FAILED)
			*ecc_err = 1;
		if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ)
			if (stop_status & R1_ERROR) {
				pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
				       req->rq_disk->disk_name, __func__,
				       stop_status);
				*gen_err = 1;
			}
	}

	/* Check for set block count errors */
	if (brq->sbc.error)
		return mmc_blk_cmd_error(req, "SET_BLOCK_COUNT", brq->sbc.error,
				prev_cmd_status_valid, status);

	/* Check for r/w command errors */
	if (brq->cmd.error)
		return mmc_blk_cmd_error(req, "r/w cmd", brq->cmd.error,
				prev_cmd_status_valid, status);

	/* Data errors */
	if (!brq->stop.error)
		return ERR_CONTINUE;

	/* Now for stop errors.  These aren't fatal to the transfer. */
	pr_err("%s: error %d sending stop command, original cmd response %#x, card status %#x\n",
	       req->rq_disk->disk_name, brq->stop.error,
	       brq->cmd.resp[0], status);

	/*
	 * Subsitute in our own stop status as this will give the error
	 * state which happened during the execution of the r/w command.
	 */
	if (stop_status) {
		brq->stop.resp[0] = stop_status;
		brq->stop.error = 0;
	}
	return ERR_CONTINUE;
}

static int mmc_blk_reset(struct mmc_blk_data *md, struct mmc_host *host,
			 int type)
{
	int err;

	if (md->reset_done & type)
		return -EEXIST;

	md->reset_done |= type;
	err = mmc_hw_reset(host);
	/* Ensure we switch back to the correct partition */
	if (err != -EOPNOTSUPP) {
		struct mmc_blk_data *main_md = mmc_get_drvdata(host->card);
		int part_err;

		main_md->part_curr = main_md->part_type;
		part_err = mmc_blk_part_switch(host->card, md);
		if (part_err) {
			/*
			 * We have failed to get back into the correct
			 * partition, so we need to abort the whole request.
			 */
			return -ENODEV;
		}
	}
	return err;
}

static inline void mmc_blk_reset_success(struct mmc_blk_data *md, int type)
{
	md->reset_done &= ~type;
}

static int mmc_blk_issue_discard_rq(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int from, nr, arg;
	int err = 0, type = MMC_BLK_DISCARD;

	pr_info("+%s,%d\n",__FUNCTION__,__LINE__);

	if (!mmc_can_erase(card)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_discard(card))
		arg = MMC_DISCARD_ARG;
	else if (mmc_can_trim(card))
		arg = MMC_TRIM_ARG;
	else
		arg = MMC_ERASE_ARG;
retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_TRIM_ARG ?
				 INAND_CMD38_ARG_TRIM :
				 INAND_CMD38_ARG_ERASE,
				 0);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
out:
	if (err == -EIO && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
	spin_lock_irq(&md->lock);
	__blk_end_request(req, err, blk_rq_bytes(req));
	spin_unlock_irq(&md->lock);
	pr_info("-%s,%d\n",__FUNCTION__,__LINE__);
	return err ? 0 : 1;
}

static int mmc_blk_issue_secdiscard_rq(struct mmc_queue *mq,
				       struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	unsigned int from = 0, nr = 0, arg = 0;
	int err = 0, type = MMC_BLK_SECDISCARD;

	pr_info("+%s,%d\n",__FUNCTION__,__LINE__);

	if (!(mmc_can_secure_erase_trim(card) /*|| mmc_can_sanitize(card)*/)) {
		err = -EOPNOTSUPP;
		goto out;
	}

	/* The sanitize operation is supported at v4.5 only */
	/*
	if (mmc_can_sanitize(card)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_SANITIZE_START, 1, 0);
		goto out;
	}
	*/

	from = blk_rq_pos(req);
	nr = blk_rq_sectors(req);

	if (mmc_can_trim(card) && !mmc_erase_group_aligned(card, from, nr))
		arg = MMC_SECURE_TRIM1_ARG;
	else
		arg = MMC_SECURE_ERASE_ARG;
retry:
	if (card->quirks & MMC_QUIRK_INAND_CMD38) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 INAND_CMD38_ARG_EXT_CSD,
				 arg == MMC_SECURE_TRIM1_ARG ?
				 INAND_CMD38_ARG_SECTRIM1 :
				 INAND_CMD38_ARG_SECERASE,
				 0);
		if (err)
			goto out;
	}
	err = mmc_erase(card, from, nr, arg);
	if (!err && arg == MMC_SECURE_TRIM1_ARG) {
		if (card->quirks & MMC_QUIRK_INAND_CMD38) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					 INAND_CMD38_ARG_EXT_CSD,
					 INAND_CMD38_ARG_SECTRIM2,
					 0);
			if (err)
				goto out;
		}
		err = mmc_erase(card, from, nr, MMC_SECURE_TRIM2_ARG);
	}
out:
	if (err == -EIO && !mmc_blk_reset(md, card->host, type))
		goto retry;
	if (!err)
		mmc_blk_reset_success(md, type);
	spin_lock_irq(&md->lock);
	__blk_end_request(req, err, blk_rq_bytes(req));
	spin_unlock_irq(&md->lock);
	pr_info("-%s,%d\n",__FUNCTION__,__LINE__);
	return err ? 0 : 1;
}

static int mmc_blk_issue_flush(struct mmc_queue *mq, struct request *req)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	int ret = 0;

	ret = mmc_flush_cache(card);
	if (ret)
		ret = -EIO;

	spin_lock_irq(&md->lock);
	__blk_end_request_all(req, ret);
	spin_unlock_irq(&md->lock);

	return ret ? 0 : 1;
}

/*
 * Reformat current write as a reliable write, supporting
 * both legacy and the enhanced reliable write MMC cards.
 * In each transfer we'll handle only as much as a single
 * reliable write can handle, thus finish the request in
 * partial completions.
 */
static inline void mmc_apply_rel_rw(struct mmc_blk_request *brq,
				    struct mmc_card *card,
				    struct request *req)
{
	if (!(card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN)) {
		/* Legacy mode imposes restrictions on transfers. */
		if (!IS_ALIGNED(brq->cmd.arg, card->ext_csd.rel_sectors))
			brq->data.blocks = 1;

		if (brq->data.blocks > card->ext_csd.rel_sectors)
			brq->data.blocks = card->ext_csd.rel_sectors;
		else if (brq->data.blocks < card->ext_csd.rel_sectors)
			brq->data.blocks = 1;
	}
}

#define CMD_ERRORS							\
	(R1_OUT_OF_RANGE |	/* Command argument out of range */	\
	 R1_ADDRESS_ERROR |	/* Misaligned address */		\
	 R1_BLOCK_LEN_ERROR |	/* Transferred block length incorrect */\
	 R1_WP_VIOLATION |	/* Tried to write to protected block */	\
	 R1_CC_ERROR |		/* Card controller error */		\
	 R1_ERROR)		/* General/unknown error */

static int mmc_blk_err_check(struct mmc_card *card,
			     struct mmc_async_req *areq)
{
	struct mmc_queue_req *mq_mrq = container_of(areq, struct mmc_queue_req,
						    mmc_active);
	struct mmc_blk_request *brq = &mq_mrq->brq;
	struct request *req = mq_mrq->req;
	int ecc_err = 0, gen_err = 0;

	/*
	 * sbc.error indicates a problem with the set block count
	 * command.  No data will have been transferred.
	 *
	 * cmd.error indicates a problem with the r/w command.  No
	 * data will have been transferred.
	 *
	 * stop.error indicates a problem with the stop command.  Data
	 * may have been transferred, or may still be transferring.
	 */
	if (brq->sbc.error || brq->cmd.error || brq->stop.error ||
	    brq->data.error) {
		switch (mmc_blk_cmd_recovery(card, req, brq, &ecc_err, &gen_err)) {
		case ERR_RETRY:
			return MMC_BLK_RETRY;
		case ERR_ABORT:
			return MMC_BLK_ABORT;
		case ERR_NOMEDIUM:
			return MMC_BLK_NOMEDIUM;
		case ERR_CONTINUE:
			break;
		}
	}

	/*
	 * Check for errors relating to the execution of the
	 * initial command - such as address errors.  No data
	 * has been transferred.
	 */
	if (brq->cmd.resp[0] & CMD_ERRORS) {
		pr_err("%s: r/w command failed, status = %#x\n",
		       req->rq_disk->disk_name, brq->cmd.resp[0]);
		return MMC_BLK_ABORT;
	}

	/*
	 * Everything else is either success, or a data error of some
	 * kind.  If it was a write, we may have transitioned to
	 * program mode, which we have to wait for it to complete.
	 */
	if (!mmc_host_is_spi(card->host) && rq_data_dir(req) != READ) {
		u32 status;

		/* Check stop command response */
		if (brq->stop.resp[0] & R1_ERROR) {
			pr_err("%s: %s: general error sending stop command, stop cmd response %#x\n",
			       req->rq_disk->disk_name, __func__,
			       brq->stop.resp[0]);
			gen_err = 1;
		}

		do {
			int err = get_card_status(card, &status, 5);
			if (err) {
				pr_err("%s: error %d requesting status\n",
				       req->rq_disk->disk_name, err);
				return MMC_BLK_CMD_ERR;
			}

			if (status & R1_ERROR) {
				pr_err("%s: %s: general error sending status command, card status %#x\n",
				       req->rq_disk->disk_name, __func__,
				       status);
				gen_err = 1;
			}

			/*
			 * Some cards mishandle the status bits,
			 * so make sure to check both the busy
			 * indication and the card state.
			 */
		} while (!(status & R1_READY_FOR_DATA) ||
			 (R1_CURRENT_STATE(status) == R1_STATE_PRG));
	}

	/* if general error occurs, retry the write operation. */
	if (gen_err) {
		pr_warning("%s: retrying write for general error\n",
				req->rq_disk->disk_name);
		return MMC_BLK_RETRY;
	}

	if (brq->data.error) {
		pr_err("%s: error %d transferring data, sector %u, nr %u, cmd response %#x, card status %#x\n",
		       req->rq_disk->disk_name, brq->data.error,
		       (unsigned)blk_rq_pos(req),
		       (unsigned)blk_rq_sectors(req),
		       brq->cmd.resp[0], brq->stop.resp[0]);

		if (rq_data_dir(req) == READ) {
			if (ecc_err)
				return MMC_BLK_ECC_ERR;
			return MMC_BLK_DATA_ERR;
		} else {
			return MMC_BLK_CMD_ERR;
		}
	}

	if (!brq->data.bytes_xfered)
		return MMC_BLK_RETRY;

	if (blk_rq_bytes(req) != brq->data.bytes_xfered)
		return MMC_BLK_PARTIAL;

	return MMC_BLK_SUCCESS;
}

static void mmc_blk_rw_rq_prep(struct mmc_queue_req *mqrq,
			       struct mmc_card *card,
			       int disable_multi,
			       struct mmc_queue *mq)
{
	u32 readcmd, writecmd;
	struct mmc_blk_request *brq = &mqrq->brq;
	struct request *req = mqrq->req;
	struct mmc_blk_data *md = mq->data;
	bool do_data_tag;

	/*
	 * Reliable writes are used to implement Forced Unit Access and
	 * REQ_META accesses, and are supported only on MMCs.
	 *
	 * XXX: this really needs a good explanation of why REQ_META
	 * is treated special.
	 */
	bool do_rel_wr = ((req->cmd_flags & REQ_FUA) ||
			  (req->cmd_flags & REQ_META)) &&
		(rq_data_dir(req) == WRITE) &&
		(md->flags & MMC_BLK_REL_WR);

	memset(brq, 0, sizeof(struct mmc_blk_request));
	brq->mrq.cmd = &brq->cmd;
	brq->mrq.data = &brq->data;

	brq->cmd.arg = blk_rq_pos(req);
	if (!mmc_card_blockaddr(card))
		brq->cmd.arg <<= 9;
	brq->cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	brq->data.blksz = 512;
	brq->stop.opcode = MMC_STOP_TRANSMISSION;
	brq->stop.arg = 0;
	brq->stop.flags = MMC_RSP_SPI_R1B | MMC_RSP_R1B | MMC_CMD_AC;
	brq->data.blocks = blk_rq_sectors(req);

	/*
	 * The block layer doesn't support all sector count
	 * restrictions, so we need to be prepared for too big
	 * requests.
	 */
	if (brq->data.blocks > card->host->max_blk_count)
		brq->data.blocks = card->host->max_blk_count;

	if (brq->data.blocks > 1) {
		/*
		 * After a read error, we redo the request one sector
		 * at a time in order to accurately determine which
		 * sectors can be read successfully.
		 */
		if (disable_multi)
			brq->data.blocks = 1;

		/* Some controllers can't do multiblock reads due to hw bugs */
		if (card->host->caps2 & MMC_CAP2_NO_MULTI_READ &&
		    rq_data_dir(req) == READ)
			brq->data.blocks = 1;
	}

	if (brq->data.blocks > 1 || do_rel_wr) {
		/* SPI multiblock writes terminate using a special
		 * token, not a STOP_TRANSMISSION request.
		 */
		if (!mmc_host_is_spi(card->host) ||
		    rq_data_dir(req) == READ)
			brq->mrq.stop = &brq->stop;
		readcmd = MMC_READ_MULTIPLE_BLOCK;
		writecmd = MMC_WRITE_MULTIPLE_BLOCK;
	} else {
		brq->mrq.stop = NULL;
		readcmd = MMC_READ_SINGLE_BLOCK;
		writecmd = MMC_WRITE_BLOCK;
	}
	if (rq_data_dir(req) == READ) {
		brq->cmd.opcode = readcmd;
		brq->data.flags |= MMC_DATA_READ;
	} else {
		brq->cmd.opcode = writecmd;
		brq->data.flags |= MMC_DATA_WRITE;
	}

	if (do_rel_wr)
		mmc_apply_rel_rw(brq, card, req);

	/*
	 * Data tag is used only during writing meta data to speed
	 * up write and any subsequent read of this meta data
	 */
	do_data_tag = (card->ext_csd.data_tag_unit_size) &&
		(req->cmd_flags & REQ_META) &&
		(rq_data_dir(req) == WRITE) &&
		((brq->data.blocks * brq->data.blksz) >=
		 card->ext_csd.data_tag_unit_size);

	/*
	 * Pre-defined multi-block transfers are preferable to
	 * open ended-ones (and necessary for reliable writes).
	 * However, it is not sufficient to just send CMD23,
	 * and avoid the final CMD12, as on an error condition
	 * CMD12 (stop) needs to be sent anyway. This, coupled
	 * with Auto-CMD23 enhancements provided by some
	 * hosts, means that the complexity of dealing
	 * with this is best left to the host. If CMD23 is
	 * supported by card and host, we'll fill sbc in and let
	 * the host deal with handling it correctly. This means
	 * that for hosts that don't expose MMC_CAP_CMD23, no
	 * change of behavior will be observed.
	 *
	 * N.B: Some MMC cards experience perf degradation.
	 * We'll avoid using CMD23-bounded multiblock writes for
	 * these, while retaining features like reliable writes.
	 */
	if ((md->flags & MMC_BLK_CMD23) && mmc_op_multi(brq->cmd.opcode) &&
	    (do_rel_wr || !(card->quirks & MMC_QUIRK_BLK_NO_CMD23) ||
	     do_data_tag)) {
		brq->sbc.opcode = MMC_SET_BLOCK_COUNT;
		brq->sbc.arg = brq->data.blocks |
			(do_rel_wr ? (1 << 31) : 0) |
			(do_data_tag ? (1 << 29) : 0);
		brq->sbc.flags = MMC_RSP_R1 | MMC_CMD_AC;
		brq->mrq.sbc = &brq->sbc;
	}

	mmc_set_data_timeout(&brq->data, card);

	brq->data.sg = mqrq->sg;
	brq->data.sg_len = mmc_queue_map_sg(mq, mqrq);

	/*
	 * Adjust the sg list so it is the same size as the
	 * request.
	 */
	if (brq->data.blocks != blk_rq_sectors(req)) {
		int i, data_size = brq->data.blocks << 9;
		struct scatterlist *sg;

		for_each_sg(brq->data.sg, sg, brq->data.sg_len, i) {
			data_size -= sg->length;
			if (data_size <= 0) {
				sg->length += data_size;
				i++;
				break;
			}
		}
		brq->data.sg_len = i;
	}

	mqrq->mmc_active.mrq = &brq->mrq;
	mqrq->mmc_active.err_check = mmc_blk_err_check;

	mmc_queue_bounce_pre(mqrq);
}

static int mmc_blk_cmd_err(struct mmc_blk_data *md, struct mmc_card *card,
			   struct mmc_blk_request *brq, struct request *req,
			   int ret)
{
	/*
	 * If this is an SD card and we're writing, we can first
	 * mark the known good sectors as ok.
	 *
	 * If the card is not SD, we can still ok written sectors
	 * as reported by the controller (which might be less than
	 * the real number of written sectors, but never more).
	 */
	if (mmc_card_sd(card)) {
		u32 blocks;

		blocks = mmc_sd_num_wr_blocks(card);
		if (blocks != (u32)-1) {
			spin_lock_irq(&md->lock);
			ret = __blk_end_request(req, 0, blocks << 9);
			spin_unlock_irq(&md->lock);
		}
	} else {
		spin_lock_irq(&md->lock);
		ret = __blk_end_request(req, 0, brq->data.bytes_xfered);
		spin_unlock_irq(&md->lock);
	}
	return ret;
}

static int mmc_blk_issue_rw_rq(struct mmc_queue *mq, struct request *rqc)
{
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;
	struct mmc_blk_request *brq = &mq->mqrq_cur->brq;
	int ret = 1, disable_multi = 0, retry = 0, type;
	enum mmc_blk_status status;
	struct mmc_queue_req *mq_rq;
	struct request *req;
	struct mmc_async_req *areq;

	if (!rqc && !mq->mqrq_prev->req)
		return 0;

	do {
		if (rqc) {
			mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
			areq = &mq->mqrq_cur->mmc_active;
		} else
			areq = NULL;
		areq = mmc_start_req(card->host, areq, (int *) &status);
		if (!areq)
			return 0;

		mq_rq = container_of(areq, struct mmc_queue_req, mmc_active);
		brq = &mq_rq->brq;
		req = mq_rq->req;
		type = rq_data_dir(req) == READ ? MMC_BLK_READ : MMC_BLK_WRITE;
		mmc_queue_bounce_post(mq_rq);

		switch (status) {
		case MMC_BLK_SUCCESS:
		case MMC_BLK_PARTIAL:
			/*
			 * A block was successfully transferred.
			 */
			mmc_blk_reset_success(md, type);
			spin_lock_irq(&md->lock);
			ret = __blk_end_request(req, 0,
						brq->data.bytes_xfered);
			spin_unlock_irq(&md->lock);
			/*
			 * If the blk_end_request function returns non-zero even
			 * though all data has been transferred and no errors
			 * were returned by the host controller, it's a bug.
			 */
			if (status == MMC_BLK_SUCCESS && ret) {
				pr_err("%s BUG rq_tot %d d_xfer %d\n",
				       __func__, blk_rq_bytes(req),
				       brq->data.bytes_xfered);
				rqc = NULL;
				goto cmd_abort;
			}
			break;
		case MMC_BLK_CMD_ERR:
			ret = mmc_blk_cmd_err(md, card, brq, req, ret);
			if (mmc_blk_reset(md, card->host, type))
				goto cmd_abort;
			if (!ret)
				goto start_new_req;
			break;
		case MMC_BLK_RETRY:
			if (retry++ < 5)
				break;
			/* Fall through */
		case MMC_BLK_ABORT:
			if (!mmc_blk_reset(md, card->host, type))
				break;
			goto cmd_abort;
		case MMC_BLK_DATA_ERR: {
			int err;

			err = mmc_blk_reset(md, card->host, type);
			if (!err)
				break;
			if (err == -ENODEV)
				goto cmd_abort;
			/* Fall through */
		}
		case MMC_BLK_ECC_ERR:
			if (brq->data.blocks > 1) {
				/* Redo read one sector at a time */
				pr_warning("%s: retrying using single block read\n",
					   req->rq_disk->disk_name);
				disable_multi = 1;
				break;
			}
			/*
			 * After an error, we redo I/O one sector at a
			 * time, so we only reach here after trying to
			 * read a single sector.
			 */
			spin_lock_irq(&md->lock);
			ret = __blk_end_request(req, -EIO,
						brq->data.blksz);
			spin_unlock_irq(&md->lock);
			if (!ret)
				goto start_new_req;
			break;
		case MMC_BLK_NOMEDIUM:
			goto cmd_abort;
		}

		if (ret) {
			/*
			 * In case of a incomplete request
			 * prepare it again and resend.
			 */
			mmc_blk_rw_rq_prep(mq_rq, card, disable_multi, mq);
			mmc_start_req(card->host, &mq_rq->mmc_active, NULL);
		}
	} while (ret);

	return 1;

 cmd_abort:
	spin_lock_irq(&md->lock);
	if (mmc_card_removed(card))
		req->cmd_flags |= REQ_QUIET;
	while (ret)
		ret = __blk_end_request(req, -EIO, blk_rq_cur_bytes(req));
	spin_unlock_irq(&md->lock);

 start_new_req:
	if (rqc) {
		mmc_blk_rw_rq_prep(mq->mqrq_cur, card, 0, mq);
		mmc_start_req(card->host, &mq->mqrq_cur->mmc_active, NULL);
	}

	return 0;
}

#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
static int mmc_blk_set_blksize(struct mmc_blk_data *md, struct mmc_card *card);
#endif

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
	int ret;
	struct mmc_blk_data *md = mq->data;
	struct mmc_card *card = md->queue.card;

#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	if (mmc_bus_needs_resume(card->host)) {
		mmc_resume_bus(card->host);
		mmc_blk_set_blksize(md, card);
	}
#endif

	if (req && !mq->mqrq_prev->req)
		/* claim host only for the first request */
		mmc_claim_host(card->host);

	ret = mmc_blk_part_switch(card, md);
	if (ret) {
		if (req) {
			spin_lock_irq(&md->lock);
			__blk_end_request_all(req, -EIO);
			spin_unlock_irq(&md->lock);
		}
		ret = 0;
		goto out;
	}

	if (req && req->cmd_flags & REQ_DISCARD) {
		/* complete ongoing async transfer before issuing discard */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		if (req->cmd_flags & REQ_SECURE &&
			!(card->quirks & MMC_QUIRK_SEC_ERASE_TRIM_BROKEN))
			ret = mmc_blk_issue_secdiscard_rq(mq, req);
		else
			ret = mmc_blk_issue_discard_rq(mq, req);
	} else if (req && req->cmd_flags & REQ_FLUSH) {
		/* complete ongoing async transfer before issuing flush */
		if (card->host->areq)
			mmc_blk_issue_rw_rq(mq, NULL);
		ret = mmc_blk_issue_flush(mq, req);
	} else {
		ret = mmc_blk_issue_rw_rq(mq, req);
	}

out:
	if (!req)
		/* release host only when there are no more requests */
		mmc_release_host(card->host);
	return ret;
}

static inline int mmc_blk_readonly(struct mmc_card *card)
{
	return mmc_card_readonly(card) ||
	       !(card->csd.cmdclass & CCC_BLOCK_WRITE);
}

static struct mmc_blk_data *mmc_blk_alloc_req(struct mmc_card *card,
					      struct device *parent,
					      sector_t size,
					      bool default_ro,
					      const char *subname,
					      int area_type)
{
	struct mmc_blk_data *md;
	int devidx, ret;

	devidx = find_first_zero_bit(dev_use, max_devices);
	if (devidx >= max_devices)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kzalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
	if (!md) {
		ret = -ENOMEM;
		goto out;
	}

	/*
	 * !subname implies we are creating main mmc_blk_data that will be
	 * associated with mmc_card with mmc_set_drvdata. Due to device
	 * partitions, devidx will not coincide with a per-physical card
	 * index anymore so we keep track of a name index.
	 */
	if (!subname) {
		md->name_idx = find_first_zero_bit(name_use, max_devices);
		__set_bit(md->name_idx, name_use);
	} else
		md->name_idx = ((struct mmc_blk_data *)
				dev_to_disk(parent)->private_data)->name_idx;

	md->area_type = area_type;

	/*
	 * Set the read-only status based on the supported commands
	 * and the write protect switch.
	 */
	md->read_only = mmc_blk_readonly(card);

	md->disk = alloc_disk(perdev_minors);
	if (md->disk == NULL) {
		ret = -ENOMEM;
		goto err_kfree;
	}

	spin_lock_init(&md->lock);
	INIT_LIST_HEAD(&md->part);
	md->usage = 1;

	ret = mmc_init_queue(&md->queue, card, &md->lock, subname);
	if (ret)
		goto err_putdisk;

	md->queue.issue_fn = mmc_blk_issue_rq;
	md->queue.data = md;

	md->disk->major	= MMC_BLOCK_MAJOR;
	md->disk->first_minor = devidx * perdev_minors;
	md->disk->fops = &mmc_bdops;
	md->disk->private_data = md;
	md->disk->queue = md->queue.queue;
	md->disk->driverfs_dev = parent;
	set_disk_ro(md->disk, md->read_only || default_ro);
	md->disk->flags = GENHD_FL_EXT_DEVT;

	/*
	 * As discussed on lkml, GENHD_FL_REMOVABLE should:
	 *
	 * - be set for removable media with permanent block devices
	 * - be unset for removable block devices with permanent media
	 *
	 * Since MMC block devices clearly fall under the second
	 * case, we do not set GENHD_FL_REMOVABLE.  Userspace
	 * should use the block device creation/destruction hotplug
	 * messages to tell when the card is present.
	 */

	snprintf(md->disk->disk_name, sizeof(md->disk->disk_name),
		 "mmcblk%d%s", md->name_idx, subname ? subname : "");

	blk_queue_logical_block_size(md->queue.queue, 512);
	set_capacity(md->disk, size);

	if (mmc_host_cmd23(card->host)) {
		if (mmc_card_mmc(card) ||
		    (mmc_card_sd(card) &&
		     card->scr.cmds & SD_SCR_CMD23_SUPPORT))
			md->flags |= MMC_BLK_CMD23;
	}

	if (mmc_card_mmc(card) &&
	    md->flags & MMC_BLK_CMD23 &&
	    ((card->ext_csd.rel_param & EXT_CSD_WR_REL_PARAM_EN) ||
	     card->ext_csd.rel_sectors)) {
		md->flags |= MMC_BLK_REL_WR;
		blk_queue_flush(md->queue.queue, REQ_FLUSH | REQ_FUA);
	}

	return md;

 err_putdisk:
	put_disk(md->disk);
 err_kfree:
	kfree(md);
 out:
	return ERR_PTR(ret);
}

static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
	sector_t size;
	struct mmc_blk_data *md;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		size = card->csd.capacity << (card->csd.read_blkbits - 9);
	}

	md = mmc_blk_alloc_req(card, &card->dev, size, false, NULL,
					MMC_BLK_DATA_AREA_MAIN);
	return md;
}

static int mmc_blk_alloc_part(struct mmc_card *card,
			      struct mmc_blk_data *md,
			      unsigned int part_type,
			      sector_t size,
			      bool default_ro,
			      const char *subname,
			      int area_type)
{
	char cap_str[10];
	struct mmc_blk_data *part_md;

	part_md = mmc_blk_alloc_req(card, disk_to_dev(md->disk), size, default_ro,
				    subname, area_type);
	if (IS_ERR(part_md))
		return PTR_ERR(part_md);
	part_md->part_type = part_type;
	list_add(&part_md->part, &md->part);

	string_get_size((u64)get_capacity(part_md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s partition %u %s\n",
	       part_md->disk->disk_name, mmc_card_id(card),
	       mmc_card_name(card), part_md->part_type, cap_str);
	return 0;
}

/* MMC Physical partitions consist of two boot partitions and
 * up to four general purpose partitions.
 * For each partition enabled in EXT_CSD a block device will be allocatedi
 * to provide access to the partition.
 */

static int mmc_blk_alloc_parts(struct mmc_card *card, struct mmc_blk_data *md)
{
	int idx, ret = 0;

	if (!mmc_card_mmc(card))
		return 0;

	for (idx = 0; idx < card->nr_parts; idx++) {
		if (card->part[idx].size) {
			ret = mmc_blk_alloc_part(card, md,
				card->part[idx].part_cfg,
				card->part[idx].size >> 9,
				card->part[idx].force_ro,
				card->part[idx].name,
				card->part[idx].area_type);
			if (ret)
				return ret;
		}
	}

	return ret;
}

static void mmc_blk_remove_req(struct mmc_blk_data *md)
{
	struct mmc_card *card;

	if (md) {
		card = md->queue.card;
		if (md->disk->flags & GENHD_FL_UP) {
			device_remove_file(disk_to_dev(md->disk), &md->force_ro);
			if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
					card->ext_csd.boot_ro_lockable)
				device_remove_file(disk_to_dev(md->disk),
					&md->power_ro_lock);

			/* Stop new requests from getting into the queue */
			del_gendisk(md->disk);
		}

		/* Then flush out any already in there */
		mmc_cleanup_queue(&md->queue);
		mmc_blk_put(md);
	}
}

static void mmc_blk_remove_parts(struct mmc_card *card,
				 struct mmc_blk_data *md)
{
	struct list_head *pos, *q;
	struct mmc_blk_data *part_md;

	__clear_bit(md->name_idx, name_use);
	list_for_each_safe(pos, q, &md->part) {
		part_md = list_entry(pos, struct mmc_blk_data, part);
		list_del(pos);
		mmc_blk_remove_req(part_md);
	}
}

static int mmc_add_disk(struct mmc_blk_data *md)
{
	int ret;
	struct mmc_card *card = md->queue.card;

	add_disk(md->disk);
	md->force_ro.show = force_ro_show;
	md->force_ro.store = force_ro_store;
	sysfs_attr_init(&md->force_ro.attr);
	md->force_ro.attr.name = "force_ro";
	md->force_ro.attr.mode = S_IRUGO | S_IWUSR;
	ret = device_create_file(disk_to_dev(md->disk), &md->force_ro);
	if (ret)
		goto force_ro_fail;

	if ((md->area_type & MMC_BLK_DATA_AREA_BOOT) &&
	     card->ext_csd.boot_ro_lockable) {
		umode_t mode;

		if (card->ext_csd.boot_ro_lock & EXT_CSD_BOOT_WP_B_PWR_WP_DIS)
			mode = S_IRUGO;
		else
			mode = S_IRUGO | S_IWUSR;

		md->power_ro_lock.show = power_ro_lock_show;
		md->power_ro_lock.store = power_ro_lock_store;
		sysfs_attr_init(&md->power_ro_lock.attr);
		md->power_ro_lock.attr.mode = mode;
		md->power_ro_lock.attr.name =
					"ro_lock_until_next_power_on";
		ret = device_create_file(disk_to_dev(md->disk),
				&md->power_ro_lock);
		if (ret)
			goto power_ro_lock_fail;
	}
	return ret;

power_ro_lock_fail:
	device_remove_file(disk_to_dev(md->disk), &md->force_ro);
force_ro_fail:
	del_gendisk(md->disk);

	return ret;
}

#define CID_MANFID_SANDISK	0x2
#define CID_MANFID_TOSHIBA	0x11
#define CID_MANFID_MICRON	0x13
#define CID_MANFID_SAMSUNG	0x15

static const struct mmc_fixup blk_fixups[] =
{
	MMC_FIXUP("SEM02G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM04G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM08G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM16G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),
	MMC_FIXUP("SEM32G", CID_MANFID_SANDISK, 0x100, add_quirk,
		  MMC_QUIRK_INAND_CMD38),

	/*
	 * Some MMC cards experience performance degradation with CMD23
	 * instead of CMD12-bounded multiblock transfers. For now we'll
	 * black list what's bad...
	 * - Certain Toshiba cards.
	 *
	 * N.B. This doesn't affect SD cards.
	 */
	MMC_FIXUP("MMC08G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC16G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),
	MMC_FIXUP("MMC32G", CID_MANFID_TOSHIBA, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_BLK_NO_CMD23),

	/*
	 * Some Micron MMC cards needs longer data read timeout than
	 * indicated in CSD.
	 */
	MMC_FIXUP(CID_NAME_ANY, CID_MANFID_MICRON, 0x200, add_quirk_mmc,
		  MMC_QUIRK_LONG_READ_TIME),

	/*
	 * On these Samsung MoviNAND parts, performing secure erase or
	 * secure trim can result in unrecoverable corruption due to a
	 * firmware bug.
	 */
	MMC_FIXUP("M8G2FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MAG4FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MBG8FA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("MCGAFA", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VAL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("KYL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),
	MMC_FIXUP("VZL00M", CID_MANFID_SAMSUNG, CID_OEMID_ANY, add_quirk_mmc,
		  MMC_QUIRK_SEC_ERASE_TRIM_BROKEN),

	END_FIXUP
};

#ifdef CONFIG_FATFS_FS_SUPPORT
	extern int mmc_fatfs_probe(card);
	extern void mmc_fatfs_remove(card);
	extern int mmc_fatfs_init(void);
	extern void mmc_fatfs_exit(void);
#endif

static int mmc_blk_probe(struct mmc_card *card)
{
	struct mmc_blk_data *md, *part_md;
	char cap_str[10];
	/*
	 * Check that the card supports the command class(es) we need.
	 */
	if (!(card->csd.cmdclass & CCC_BLOCK_READ))
		return -ENODEV;

	md = mmc_blk_alloc(card);
	if (IS_ERR(md))
		return PTR_ERR(md);

	string_get_size((u64)get_capacity(md->disk) << 9, STRING_UNITS_2,
			cap_str, sizeof(cap_str));
	pr_info("%s: %s %s %s %s\n",
		md->disk->disk_name, mmc_card_id(card), mmc_card_name(card),
		cap_str, md->read_only ? "(ro)" : "");

	if (mmc_blk_alloc_parts(card, md))
		goto out;

	mmc_set_drvdata(card, md);
	mmc_fixup_device(card, blk_fixups);

#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	mmc_set_bus_resume_policy(card->host, 1);
#endif
	if (mmc_add_disk(md))
		goto out;

	list_for_each_entry(part_md, &md->part, part) {
		if (mmc_add_disk(part_md))
			goto out;
	}

#ifdef CONFIG_FATFS_FS_SUPPORT
	mmc_fatfs_probe(card);			// add for fatfs
#endif

	return 0;

 out:
	mmc_blk_remove_parts(card, md);
	mmc_blk_remove_req(md);
	return 0;
}

static void mmc_blk_remove(struct mmc_card *card)
{
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	mmc_blk_remove_parts(card, md);
	mmc_claim_host(card->host);

#ifdef CONFIG_FATFS_FS_SUPPORT
	mmc_fatfs_remove(card);			// add for fatfs
#endif
	
	mmc_blk_part_switch(card, md);
	mmc_release_host(card->host);
	mmc_blk_remove_req(md);
	mmc_set_drvdata(card, NULL);
#ifdef CONFIG_MMC_BLOCK_DEFERRED_RESUME
	mmc_set_bus_resume_policy(card->host, 0);
#endif
}

#ifdef CONFIG_PM
static int mmc_blk_suspend(struct mmc_card *card)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		mmc_queue_suspend(&md->queue);
		list_for_each_entry(part_md, &md->part, part) {
			mmc_queue_suspend(&part_md->queue);
		}
	}
	return 0;
}

static int mmc_blk_resume(struct mmc_card *card)
{
	struct mmc_blk_data *part_md;
	struct mmc_blk_data *md = mmc_get_drvdata(card);

	if (md) {
		/*
		 * Resume involves the card going into idle state,
		 * so current partition is always the main one.
		 */
		md->part_curr = md->part_type;
		mmc_queue_resume(&md->queue);
		list_for_each_entry(part_md, &md->part, part) {
			mmc_queue_resume(&part_md->queue);
		}
	}
	return 0;
}
#else
#define	mmc_blk_suspend	NULL
#define mmc_blk_resume	NULL
#endif

static struct mmc_driver mmc_driver = {
	.drv		= {
		.name	= "mmcblk",
	},
	.probe		= mmc_blk_probe,
	.remove		= mmc_blk_remove,
	.suspend	= mmc_blk_suspend,
	.resume		= mmc_blk_resume,
};

static int __init mmc_blk_init(void)
{
	int res;

	if (perdev_minors != CONFIG_MMC_BLOCK_MINORS)
		pr_info("mmcblk: using %d minors per device\n", perdev_minors);

	max_devices = 256 / perdev_minors;

	res = register_blkdev(MMC_BLOCK_MAJOR, "mmc");
	if (res)
		goto out;

#ifdef CONFIG_FATFS_FS_SUPPORT
	mmc_fatfs_init();
#endif

	res = mmc_register_driver(&mmc_driver);
	if (res)
		goto out2;

	return 0;
 out2:
	unregister_blkdev(MMC_BLOCK_MAJOR, "mmc");
 out:
	return res;
}

static void __exit mmc_blk_exit(void)
{
#ifdef CONFIG_FATFS_FS_SUPPORT
	mmc_fatfs_exit();
#endif

	mmc_unregister_driver(&mmc_driver);
	unregister_blkdev(MMC_BLOCK_MAJOR, "mmc");
}

module_init(mmc_blk_init);
module_exit(mmc_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) block device driver");

