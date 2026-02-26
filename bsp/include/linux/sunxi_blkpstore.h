/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __AW_BLKPSTORE_H_
#define __AW_BLKPSTORE_H_

#include <linux/types.h>
#include <linux/pstore.h>
#include <linux/pstore_zone.h>
#include <linux/pstore_blk.h>

struct bdev_info {
	struct block_device *bdev;
	dev_t devt;
	sector_t nr_sects;
	sector_t start_sect;
};

enum pstore_blk_notifier_type {
	PSTORE_BLK_BACKEND_REGISTER = 1,
	PSTORE_BLK_BACKEND_PANIC_DRV_REGISTER,
	PSTORE_BLK_BACKEND_UNREGISTER,
	PSTORE_BLK_BACKEND_PANIC_DRV_UNREGISTER,
};

typedef	int (*pstore_blk_notifier_fn_t)(enum pstore_blk_notifier_type type,
		struct pstore_device_info *dev);

struct pstore_blk_notifier {
	struct notifier_block nb;
	pstore_blk_notifier_fn_t notifier_call;
};

extern int aw_pstore_blk_get_bdev_info(struct bdev_info *bi);
extern int aw_register_pstore_device(struct pstore_device_info *dev);
extern void aw_unregister_pstore_device(struct pstore_device_info *dev);

extern int aw_register_pstore_blk_panic_notifier(struct pstore_blk_notifier *pbn);
extern void aw_unregister_pstore_blk_panic_notifier(struct pstore_blk_notifier *nb);

#endif
