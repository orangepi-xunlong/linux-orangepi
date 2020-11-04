/***************************************************************************
 * nand_class.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ***************************************************************************/

#define BLK_INFO_MSG_ON
#include "nand_base.h"
#include "nand_class.h"
#include "nand_dev.h"
#include <linux/device.h>
#include <linux/sunxi-boot.h>
#include "../phy-nand/nand_nftl.h"
#include "../phy-nand/nand.h"
#include "../phy-nand/rawnand/controller/ndfc_base.h"
#include <asm/page.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "../phy-nand/nand_physic_interface.h"

struct nand_kobject *nand_debug_kobj;

static ssize_t nand_sysfs_show(struct kobject *kobject, struct attribute *attr,
			       char *buf);
static ssize_t nand_sysfs_store(struct kobject *kobject, struct attribute *attr,
				const char *buf, size_t count);
static ssize_t nand_show_debug(struct _nftl_blk *nftl_blk, char *buf);
extern void get_nftl_version(char *version, char *patchlevel, char *sublevel,
		char *date, char *time);
/*
 *static ssize_t nand_store_debug(struct _nftl_blk *nftl_blk, const char *buf,
 *                                size_t count);
 */
static ssize_t nand_show_arch(struct _nftl_blk *nftl_blk, char *buf);
static ssize_t nand_show_gcinfo(struct _nftl_blk *nftl_blk, char *buf);
static ssize_t nand_show_version(struct _nftl_blk *nftl_blk, char *buf);
static ssize_t nand_show_badblk(struct _nftl_blk *nftl_blk, char *buf);
static ssize_t nand_store_gcone(struct _nftl_blk *nftl_blk, const char *buf,
				size_t count);
static ssize_t nand_store_gcall(struct _nftl_blk *nftl_blk, const char *buf,
				size_t count);
static struct attribute attr_debug = {
    .name = "nand_debug",
    .mode = S_IRUGO | S_IWUSR | S_IWGRP,
};
static struct attribute attr_arch = {
    .name = "arch",
    .mode = S_IRUGO,
};
static struct attribute attr_gcinfo = {
    .name = "gcinfo",
    .mode = S_IRUGO,
};
static struct attribute attr_badblk = {
    .name = "badblock",
    .mode = S_IRUGO,
};
static struct attribute attr_gc_all = {
    .name = "gc_all",
    .mode = S_IWUSR | S_IWGRP,
};
static struct attribute attr_gc_one = {
    .name = "gc_one",
    .mode = S_IWUSR | S_IWGRP,
};
static struct attribute attr_version = {
    .name = "version",
    .mode = S_IRUGO,
};
static struct attribute *sysfs_attrs[] = {
    &attr_debug,
    &attr_arch,
    &attr_gcinfo,
    &attr_badblk,
    &attr_gc_all,
    &attr_gc_one,
    &attr_version,
    NULL,
};
static const struct sysfs_ops sysfs_ops = {
    .show = nand_sysfs_show,
    .store = nand_sysfs_store,
};
static struct kobj_type sysfs_type = {
    .sysfs_ops = &sysfs_ops,
    .default_attrs = sysfs_attrs,
};
int nand_debug_init(struct _nftl_blk *nftl_blk, int part_num)
{
	int ret;
	nand_debug_kobj = kzalloc(sizeof(struct nand_kobject), GFP_KERNEL);
	if (!nand_debug_kobj) {
		nand_dbg_err("kzalloc failed\n");
		return -ENOMEM;
	}
	nand_debug_kobj->nftl_blk = nftl_blk;
	ret = kobject_init_and_add(&nand_debug_kobj->kobj, &sysfs_type, NULL,
				   "nand_driver%d", part_num);
	if (ret) {
		nand_dbg_err("init nand sysfs fail!\n");
		return ret;
	}

	return 0;
}

struct nand_attr_ops {
	struct attribute *attr;
	ssize_t (*show)(struct _nftl_blk *blk, char *buf);
	ssize_t (*store)(struct _nftl_blk *blk, const char *buf, size_t cnt);
};
static struct nand_attr_ops attr_ops_array[] = {
    {
	.attr = &attr_debug,
	.show = nand_show_debug,
	/*.store = nand_store_debug,*/
    },
    {
	.attr = &attr_arch,
	.show = nand_show_arch,
    },
    {
	.attr = &attr_gcinfo,
	.show = nand_show_gcinfo,
    },
    {
	.attr = &attr_version,
	.show = nand_show_version,
    },
    {
	.attr = &attr_badblk,
	.show = nand_show_badblk,
    },
    {
	.attr = &attr_gc_all,
	.store = nand_store_gcall,
    },
    {
	.attr = &attr_gc_one,
	.store = nand_store_gcone,
    },
};
static ssize_t nand_sysfs_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	int index;
	struct nand_attr_ops *attr_ops;
	struct _nftl_blk *nftl_blk = ((struct nand_kobject *)kobj)->nftl_blk;
	ssize_t ret;
	//struct nand_kobject *nand_kobj;

	for (index = 0; index < ARRAY_SIZE(attr_ops_array); index++) {
		attr_ops = &attr_ops_array[index];
		if (attr == attr_ops->attr)
			break;
	}
	if (unlikely(index == ARRAY_SIZE(attr_ops_array))) {
		nand_dbg_err("not found attr_ops for %s\n", attr->name);
		return -EINVAL;
	}
	if (attr_ops->show) {
		mutex_lock(nftl_blk->blk_lock);
		ret = attr_ops->show(nftl_blk, buf);
		mutex_unlock(nftl_blk->blk_lock);
		return ret;
	}
	return -EINVAL;
}

/****************************************************************************
*Name         :
*Description  :receive testcase num from echo command
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t nand_sysfs_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	int index;
	struct nand_attr_ops *attr_ops;
	struct _nftl_blk *nftl_blk = ((struct nand_kobject *)kobj)->nftl_blk;
	ssize_t ret;
	for (index = 0; index < ARRAY_SIZE(attr_ops_array); index++) {
		attr_ops = &attr_ops_array[index];
		if (attr == attr_ops->attr)
			break;
	}
	if (unlikely(index == ARRAY_SIZE(attr_ops_array))) {
		nand_dbg_err("not found attr_ops for %s\n", attr->name);
		return -EINVAL;
	}
	if (attr_ops->store) {
		mutex_lock(nftl_blk->blk_lock);
		ret = attr_ops->store(nftl_blk, buf, count);
		mutex_unlock(nftl_blk->blk_lock);
		return ret;
	}
	return -EINVAL;
}
static ssize_t nand_show_debug(struct _nftl_blk *nftl_blk, char *buf)
{
	ssize_t ret = 0;
	/*spinand debug achieve in sunxi-debug.c in driver/mtd/aw-spinand*/
	/*
	int tmp[4] = {0};

	ret += PHY_GetArchInfo_Str(buf);
	ret += sprintf(buf + ret, "BadBlkCnt: %d\n",
			nftl_get_bad_block_cnt(nftl_blk));
	nand_get_drv_version(&tmp[0], &tmp[1], &tmp[2], &tmp[3]);
	ret += sprintf(buf + ret, "NandVersion: %x.%x %x %x\n",
			tmp[0], tmp[1], tmp[2], tmp[3]);
*/
	return ret;
}
/*
 *static ssize_t nand_store_debug(struct _nftl_blk *nftl_blk, const char *buf,
 *                                size_t count)
 *{
 *        int ret, i;
 *        int arg_num = 0;
 *        char cmd[32] = {0};
 *        unsigned int arg0_int = 0;
 *        unsigned int arg1_int = 0;
 *        unsigned int arg2_int = 0;
 *        char arg3_str[16] = {0};
 *
 *        arg_num = sscanf(buf, "%31s %u %u %u %15s", cmd, &arg0_int, &arg1_int,
 *                         &arg2_int, arg3_str);
 *
 *        if (-1 == arg_num || 0 == arg_num) {
 *                nand_dbg_err("cmd format err!");
 *                return -EINVAL;
 *        }
 *
 *        if (strcmp(cmd, "flush") == 0) {
 *                if (2 != arg_num) {
 *                        nand_dbg_err("err: %s needs 1 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
 *                ret = nftl_blk->flush_write_cache(nftl_blk, arg0_int);
 *                return count;
 *        } else if (strcmp(cmd, "engcall") == 0) {
 *                nand_dbg_err("NAND DEBUG: %s\n", cmd);
 *                [> gc with invalid_page_count equal to (page_per_block / 2) <]
 *                ret = gc_all_enhance(nftl_blk->nftl_zone);
 *                if (!ret)
 *                        return count;
 *                else
 *                        return -EIO;
 *        } else if (strcmp(cmd, "gcall") == 0) {
 *                if (2 != arg_num) {
 *                        nand_dbg_err("err: %s needs 1 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
 *                ret = gc_all(nftl_blk->nftl_zone, arg0_int);
 *                return count;
 *        } else if (strcmp(cmd, "gcone") == 0) {
 *                if (2 != arg_num) {
 *                        nand_dbg_err("err: %s needs 1 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
 *                ret = gc_one(nftl_blk->nftl_zone, arg0_int);
 *                return count;
 *        } else if (strcmp(cmd, "priogc") == 0) {
 *                if (3 != arg_num) {
 *                        nand_dbg_err("err: %s needs 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
 *                ret = prio_gc_one(nftl_blk->nftl_zone, arg0_int,
 *                                  arg1_int);
 *                return count;
 *        } else if (strcmp(cmd, "test") == 0) {
 *                if (2 != arg_num) {
 *                        nand_dbg_err("err: %s needs 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
 *                ret = nftl_set_zone_test((void *)nftl_blk->nftl_zone,
 *                                         arg0_int);
 *                return count;
 *        } else if (strcmp(cmd, "showall") == 0) {
 *                nand_dbg_err("NAND DEBUG: %s\n", cmd);
 *                print_free_list(nftl_blk->nftl_zone);
 *                print_block_invalid_list(nftl_blk->nftl_zone);
 *                return count;
 *        } else if (strcmp(cmd, "showinfo") == 0) {
 *                nand_dbg_err("NAND DEBUG: %s\n", cmd);
 *                print_nftl_zone(nftl_blk->nftl_zone);
 *                return count;
 *        } else if (strcmp(cmd, "blkdebug") == 0) {
 *                if (2 != arg_num) {
 *                        nand_dbg_err("err: %s needs 1 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
 *                debug_data = arg0_int;
 *                return count;
 *        } else if (strcmp(cmd, "smart") == 0) {
 *                nand_dbg_err("NAND DEBUG: %s\n", cmd);
 *                print_smart(nftl_blk->nftl_zone);
 *                return count;
 *        } else if (strcmp(cmd, "read1") == 0) {
 *                if (4 != arg_num) {
 *                        nand_dbg_err("err: %s needs 3 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u %u\n", cmd, arg0_int,
 *                             arg1_int, arg2_int);
 *                nand_dbg_phy_read(arg0_int, arg1_int, arg2_int);
 *                return count;
 *        } else if (strcmp(cmd, "read2") == 0) {
 *                if (3 != arg_num) {
 *                        nand_dbg_err("  %s need 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
 *                nand_dbg_zone_phy_read(nftl_blk->nftl_zone, arg0_int,
 *                                       arg1_int);
 *                return count;
 *        } else if (strcmp(cmd, "erase1") == 0) {
 *                if (3 != arg_num) {
 *                        nand_dbg_err("err: %s needs 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
 *                nand_dbg_phy_erase(arg0_int, arg1_int);
 *                return count;
 *        } else if (strcmp(cmd, "erase2") == 0) {
 *                if (3 != arg_num) {
 *                        nand_dbg_err("err: %s needs 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
 *                nand_dbg_zone_erase(nftl_blk->nftl_zone, arg0_int,
 *                                    arg1_int);
 *                return count;
 *        } else if (strcmp(cmd, "erase3") == 0) {
 *                if (3 != arg_num) {
 *                        nand_dbg_err("err: %s needs 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
 *                nand_dbg_single_phy_erase(arg0_int, arg1_int);
 *                return count;
 *        } else if (strcmp(cmd, "write1") == 0) {
 *                if (4 != arg_num) {
 *                        nand_dbg_err("err: %s needs 3 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u %u\n", cmd, arg0_int,
 *                             arg1_int, arg2_int);
 *                nand_dbg_phy_write(arg0_int, arg1_int, arg2_int);
 *                return count;
 *        } else if (strcmp(cmd, "write2") == 0) {
 *                if (3 != arg_num) {
 *                        nand_dbg_err("  %s need 2 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
 *                nand_dbg_zone_phy_write(nftl_blk->nftl_zone, arg0_int,
 *                                        arg1_int);
 *                return count;
 *        } else if (strcmp(cmd, "checktable") == 0) {
 *                nand_dbg_err("NAND DEBUG: %s\n", cmd);
 *
 *                nand_check_table(nftl_blk->nftl_zone);
 *                return count;
 *
 *        } else if (strcmp(cmd, "readdev") == 0) {
 *                char *tempbuf;
 *                if (5 != arg_num) {
 *                        nand_dbg_err("err: %s needs 4 argument\n", cmd);
 *                        return -EINVAL;
 *                }
 *                nand_dbg_err("NAND DEBUG: %s %u %u %u %s\n", cmd, arg0_int,
 *                             arg1_int, arg2_int, arg3_str);
 *                if (arg1_int > 16) {
 *                        nand_dbg_err("arg1: max len is 16!\n");
 *                        return -EINVAL;
 *                }
 *
 *                tempbuf = kmalloc(8192, GFP_KERNEL);
 *                if (!tempbuf)
 *                        return PTR_ERR(tempbuf);
 *                _dev_nand_read2(arg3_str, arg0_int, arg1_int, tempbuf);
 *                for (i = 0; i < (arg1_int << 9); i += 4) {
 *                        nand_dbg_inf("%8x ", *((int *)&tempbuf[i]));
 *                        if (((i + 4) % 64) == 0)
 *                                nand_dbg_inf("\n");
 *                }
 *                kfree(tempbuf);
 *                return count;
 *
 *        } else {
 *                nand_dbg_err("NAND DEBUG: undefined cmd: %s\n", cmd);
 *                return -EINVAL;
 *        }
 *
 *        return count;
 *}
 */

/****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static ssize_t nand_show_arch(struct _nftl_blk *nftl_blk, char *buf)
{
	/*spinand achieve in sunxi-debug.c in linux/mtd/aw-spinad*/
	return 0;
}

static ssize_t nand_show_gcinfo(struct _nftl_blk *nftl_blk, char *buf)
{
	return nftl_get_gc_info(nftl_blk->nftl_zone, buf, PAGE_SIZE);
}

static ssize_t nand_show_version(struct _nftl_blk *nftl_blk, char *buf)
{
	int tmp[4] = {0};
	nand_get_drv_version(&tmp[0], &tmp[1], &tmp[2], &tmp[3]);
	return sprintf(buf, "%x.%x %x %x\n", tmp[0], tmp[1], tmp[2], tmp[3]);
}

static ssize_t nand_show_badblk(struct _nftl_blk *nftl_blk, char *buf)
{
	return sprintf(buf, "cnt: %d\n", nftl_get_bad_block_cnt(nftl_blk));
}

static ssize_t nand_store_gcone(struct _nftl_blk *nftl_blk, const char *buf,
				size_t count)
{
	int ret;
	int invalid_page_count;

	ret = sscanf(buf, "%u", &invalid_page_count);
	if (1 != ret) {
		nand_dbg_err("invalid parameter %s\n", buf);
		nand_dbg_err("please enter invalid_page_count for gcone\n");
		return -EINVAL;
	}

	nand_dbg_inf("ctl: gcone %u\n", invalid_page_count);
	ret = gc_one(nftl_blk->nftl_zone, invalid_page_count);
	if (ret == 1)
		return -EIO;
	return count;
}

static ssize_t nand_store_gcall(struct _nftl_blk *nftl_blk, const char *buf,
				size_t count)
{
	int ret;
	int invalid_page_count;

	ret = sscanf(buf, "%u", &invalid_page_count);
	if (1 != ret) {
		nand_dbg_err("invalid parameter %s\n", buf);
		nand_dbg_err("please enter invalid_page_count for gcall\n");
		return -EINVAL;
	}

	nand_dbg_inf("ctl: gcall %u\n", invalid_page_count);
	ret = gc_all(nftl_blk->nftl_zone, invalid_page_count);
	if (ret == 1)
		return -EIO;
	return count;
}

/*
 *extern void get_nftl_version(char *version, char *patchlevel, char *sublevel,
 *                char *data, char *time); //awa1543
 */

static ssize_t nand_nftl_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	char version[20] = {};
	char patchlevel[20] = {};
	char sublevel[20] = {};
	char date[20] = {};
	char time[20] = {};

	get_nftl_version(version, patchlevel, sublevel, date, time);

	ret = sprintf(buf, "%s.%s.%s  %s-%s\n", version, patchlevel, sublevel,
			date, time);

	return ret;
}
char *ecc_mode_to_string(unsigned int ecc_mode)
{
	switch (ecc_mode) {
	case 0x0:
		return "BCH-16";
	case 0x01:
		return "BCH-24";
	case 0x02:
		return "BCH-28";
	case 0x03:
		return "BCH-32";
	case 0x04:
		return "BCH-40";
	case 0x05:
		return "BCH-44";
	case 0x06:
		return "BCH-48";
	case 0x07:
		return "BCH-52";
	case 0x08:
		return "BCH-56";
	case 0x09:
		return "BCH-60";
	case 0x0A:
		return "BCH-64";
	case 0x0B:
		return "BCH-68";
	case 0x0C:
		return "BCH-72";
	case 0x0D:
		return "BCH-76";
	case 0x0E:
		return "BCH-80";
	}
	return NULL;
}
static ssize_t nand_arch_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	char *name = NULL;
	uint8_t id[8] = {0};
	unsigned int die_cnt = 0;
	int pagesize = 0;
	int blocksize = 0;
	int diesize = 0;
	uint8_t freq = 0;
	unsigned int ecc_mode = 0;

	struct nand_chip_info *chip = aw_ndfc.nctri->nci;

	name = rawnand_get_chip_name(chip);
	ret = snprintf(buf, 100, "\033[47;31mmodel      :\033[0m %s\n", name);

	rawnand_get_chip_id(chip, id, sizeof(id));
	ret += snprintf(buf + ret, 100, "\033[47;31mnand     id:\033[0m %02x %02x %02x %02x %02x %02x %02x %02x\n",
			chip->id[0], chip->id[1], chip->id[2], chip->id[3],
			chip->id[4], chip->id[5], chip->id[6], chip->id[7]);

	pagesize = rawnand_get_chip_page_size(chip, SECTOR);
	ret += snprintf(buf + ret, 100, "\033[47;31mpage   size:\033[0m %d sectors\n", pagesize);


	blocksize = rawnand_get_chip_block_size(chip, PAGE);
	ret += snprintf(buf + ret, 100, "\033[47;31mblock  size:\033[0m %d pages\n", blocksize);

	diesize = rawnand_get_chip_die_size(chip, BLOCK);
	ret += snprintf(buf + ret, 100, "\033[47;31mdie    size:\033[0m %d blocks\n", diesize);

	die_cnt = rawnand_get_chip_die_cnt(chip);
	ret += snprintf(buf + ret, 100, "\033[47;31mdie     cnt:\033[0m %d %s\n", die_cnt,
			die_cnt > 1 ? "dies" : "die");

	freq = rawnand_get_chip_freq(chip);
	ret += snprintf(buf + ret, 100, "\033[47;31mfreqence   :\033[0m %dM\n", freq);

	ecc_mode = rawnand_get_chip_ecc_mode(chip);
	ret += snprintf(buf + ret, 100, "\033[47;31mecc    mode:\033[0m %s\n", ecc_mode_to_string(ecc_mode));


	return ret;
}


static ssize_t nand_badblock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct _nftl_blk *nftl_blk = aw_ndfc.nftl_blk;

	return sprintf(buf, "badblock  cnt: %d\n", nftl_get_bad_block_cnt(nftl_blk));
}


static ssize_t nand_gcinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct _nftl_blk *nftl_blk = aw_ndfc.nftl_blk;

	return nftl_get_gc_info(nftl_blk->nftl_zone, buf, PAGE_SIZE);
}

static ssize_t nand_gcone_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret;

	ret = snprintf(buf, PAGE_SIZE, "nand_gcone explain: use it can gc one block,"
			"\nwhich invalid page count large than the input "
			"value;\nusage: echo xxx > nand_gcone\n");

	return ret;
}

static ssize_t nand_gcone_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int invalid_page_count;
	struct _nftl_blk *nftl_blk = aw_ndfc.nftl_blk;

	ret = sscanf(buf, "%u", &invalid_page_count);
	if (1 != ret) {
		nand_dbg_err("invalid parameter %s\n", buf);
		nand_dbg_err("please enter invalid_page_count for gcone\n");
		return -EINVAL;
	}

	if (invalid_page_count <= 0) {
		nand_dbg_inf("do nothing for gc %d\n", invalid_page_count);
		return count;
	}

	nand_dbg_inf("ctl: gcone %u\n", invalid_page_count);
	ret = gc_one(nftl_blk->nftl_zone, invalid_page_count);
	if (ret == 1)
		return -EIO;

	return count;
}

static ssize_t nand_gcall_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret;

	ret = snprintf(buf, PAGE_SIZE, "nand_gcone explain: use it can gc all block,"
			"\nwhich invalid page count large than the input "
			"value;\nusage: echo xxx > nand_gcall\n");

	return ret;
}
static ssize_t nand_gcall_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int invalid_page_count;
	struct _nftl_blk *nftl_blk = aw_ndfc.nftl_blk;

	ret = sscanf(buf, "%u", &invalid_page_count);
	if (1 != ret) {
		nand_dbg_err("invalid parameter %s\n", buf);
		nand_dbg_err("please enter invalid_page_count for gcall\n");
		return -EINVAL;
	}

	if (invalid_page_count <= 0) {
		nand_dbg_inf("do nothing for gc %d\n", invalid_page_count);
		return count;
	}

	nand_dbg_inf("ctl: gcall %u\n", invalid_page_count);
	ret = gc_all(nftl_blk->nftl_zone, invalid_page_count);
	if (ret == 1)
		return -EIO;

	return count;
}

static ssize_t nand_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret = snprintf(buf + ret, PAGE_SIZE - ret, "nand_debug explain:\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo flush > nand_debug\n"
			"flush cache\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo engcall > nand_debug\n"
			"gcc all invalid page count equal or large than "
			"page_per_blk / 2 for samll slc nand(128M)\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo gcall xxx > nand_debug\n"
			"gcc all block of invalid page count equal or large than xxx\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo gcone xxx > nand_debug\n"
			"gcc one block of invalid page count equal or large than xxx\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo priogc xxx_b xxx_p> nand_debug\n"
			"prioritize to gcc the xxx_p pages in xxx_b ,when next time gc\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo test 1 > nand_debug\n\n"
			"open some debug gate\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo showall > nand_debug\n"
			"output the free block and invalid block information\n\n"
			"after echo test 1 > nand_debug after echo test 1 > nand_debug\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo showinfo > nand_debug\n"
			"output the zone information,after echo test 1 > nand_debug\n\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret, "eg: echo smart > nand_debug\n"
			"output the smart information,after echo test 1 > nand_debug\n\n");

	return ret;
}
static ssize_t nand_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, i;
	struct _nftl_blk *nftl_blk = aw_ndfc.nftl_blk;
	int arg_num = 0;
	char cmd[32] = {0};
	unsigned int arg0_int = 0;
	unsigned int arg1_int = 0;
	unsigned int arg2_int = 0;
	char arg3_str[16] = {0};

	arg_num = sscanf(buf, "%31s %u %u %u %15s", cmd, &arg0_int, &arg1_int,
			 &arg2_int, arg3_str);

	if (-1 == arg_num || 0 == arg_num) {
		nand_dbg_err("cmd format err!");
		return -EINVAL;
	}

	if (strcmp(cmd, "flush") == 0) {
		if (2 != arg_num) {
			nand_dbg_err("err: %s needs 1 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
		ret = nftl_blk->flush_write_cache(nftl_blk, arg0_int);
		return count;
	} else if (strcmp(cmd, "engcall") == 0) {
		nand_dbg_err("NAND DEBUG: %s\n", cmd);
		/* gc with invalid_page_count equal to (page_per_block / 2) */
		ret = gc_all_enhance(nftl_blk->nftl_zone);
		if (!ret)
			return count;
		else
			return -EIO;
	} else if (strcmp(cmd, "gcall") == 0) {
		if (2 != arg_num) {
			nand_dbg_err("err: %s needs 1 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
		ret = gc_all(nftl_blk->nftl_zone, arg0_int);
		return count;
	} else if (strcmp(cmd, "gcone") == 0) {
		if (2 != arg_num) {
			nand_dbg_err("err: %s needs 1 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
		ret = gc_one(nftl_blk->nftl_zone, arg0_int);
		return count;
	} else if (strcmp(cmd, "priogc") == 0) {
		if (3 != arg_num) {
			nand_dbg_err("err: %s needs 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
		ret = prio_gc_one(nftl_blk->nftl_zone, arg0_int,
				  arg1_int);
		return count;
	} else if (strcmp(cmd, "test") == 0) {
		if (2 != arg_num) {
			nand_dbg_err("err: %s needs 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
		ret = nftl_set_zone_test((void *)nftl_blk->nftl_zone,
					 arg0_int);
		return count;
	} else if (strcmp(cmd, "showall") == 0) {
		nand_dbg_err("NAND DEBUG: %s\n", cmd);
		print_free_list(nftl_blk->nftl_zone);
		print_block_invalid_list(nftl_blk->nftl_zone);
		return count;
	} else if (strcmp(cmd, "showinfo") == 0) {
		nand_dbg_err("NAND DEBUG: %s\n", cmd);
		print_nftl_zone(nftl_blk->nftl_zone);
		return count;
	} else if (strcmp(cmd, "blkdebug") == 0) {
		if (2 != arg_num) {
			nand_dbg_err("err: %s needs 1 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u\n", cmd, arg0_int);
		debug_data = arg0_int;
		return count;
	} else if (strcmp(cmd, "smart") == 0) {
		nand_dbg_err("NAND DEBUG: %s\n", cmd);
		print_smart(nftl_blk->nftl_zone);
		return count;
	} else if (strcmp(cmd, "read1") == 0) {
		if (4 != arg_num) {
			nand_dbg_err("err: %s needs 3 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u %u\n", cmd, arg0_int,
			     arg1_int, arg2_int);
		nand_dbg_phy_read(arg0_int, arg1_int, arg2_int);
		return count;
	} else if (strcmp(cmd, "read2") == 0) {
		if (3 != arg_num) {
			nand_dbg_err("  %s need 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
		nand_dbg_zone_phy_read(nftl_blk->nftl_zone, arg0_int,
				       arg1_int);
		return count;
	} else if (strcmp(cmd, "erase1") == 0) {
		if (3 != arg_num) {
			nand_dbg_err("err: %s needs 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
		nand_dbg_phy_erase(arg0_int, arg1_int);
		return count;
	} else if (strcmp(cmd, "erase2") == 0) {
		if (3 != arg_num) {
			nand_dbg_err("err: %s needs 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
		nand_dbg_zone_erase(nftl_blk->nftl_zone, arg0_int,
				    arg1_int);
		return count;
	} else if (strcmp(cmd, "erase3") == 0) {
		if (3 != arg_num) {
			nand_dbg_err("err: %s needs 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
		nand_dbg_single_phy_erase(arg0_int, arg1_int);
		return count;
	} else if (strcmp(cmd, "write1") == 0) {
		if (4 != arg_num) {
			nand_dbg_err("err: %s needs 3 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u %u\n", cmd, arg0_int,
			     arg1_int, arg2_int);
		nand_dbg_phy_write(arg0_int, arg1_int, arg2_int);
		return count;
	} else if (strcmp(cmd, "write2") == 0) {
		if (3 != arg_num) {
			nand_dbg_err("  %s need 2 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u\n", cmd, arg0_int, arg1_int);
		nand_dbg_zone_phy_write(nftl_blk->nftl_zone, arg0_int,
					arg1_int);
		return count;
	} else if (strcmp(cmd, "checktable") == 0) {
		nand_dbg_err("NAND DEBUG: %s\n", cmd);

		nand_check_table(nftl_blk->nftl_zone);
		return count;

	} else if (strcmp(cmd, "readdev") == 0) {
		char *tempbuf;
		if (5 != arg_num) {
			nand_dbg_err("err: %s needs 4 argument\n", cmd);
			return -EINVAL;
		}
		nand_dbg_err("NAND DEBUG: %s %u %u %u %s\n", cmd, arg0_int,
			     arg1_int, arg2_int, arg3_str);
		if (arg1_int > 16) {
			nand_dbg_err("arg1: max len is 16!\n");
			return -EINVAL;
		}

		tempbuf = kmalloc(8192, GFP_KERNEL);
		if (!tempbuf)
			return PTR_ERR(tempbuf);
		_dev_nand_read2(arg3_str, arg0_int, arg1_int, tempbuf);
		for (i = 0; i < (arg1_int << 9); i += 4) {
			nand_dbg_inf("%8x ", *((int *)&tempbuf[i]));
			if (((i + 4) % 64) == 0)
				nand_dbg_inf("\n");
		}
		kfree(tempbuf);
		return count;

	} else {
		nand_dbg_err("NAND DEBUG: undefined cmd: %s\n", cmd);
		return -EINVAL;
	}

	return count;
}
#if 0
/**
 * Parses a string into a number.  The number stored at ptr is
 * potentially suffixed with K (for kilobytes, or 1024 bytes),
 * M (for megabytes, or 1048576 bytes), or G (for gigabytes, or
 * 1073741824).  If the number is suffixed with K, M, or G, then
 * the return value is the number multiplied by one kilobyte, one
 * megabyte, or one gigabyte, respectively.
 *
 * @param ptr where parse begins
 * @param retptr output pointer to next char after parse completes (output)
 * @return resulting unsigned int
 */
static u64 memsize_parse (const char *const ptr, const char **retptr)
{
	u64 ret = simple_strtoull(ptr, (char **)retptr, 0);

	switch (**retptr) {
	case 'G':
	case 'g':
		ret <<= 10;
	case 'M':
	case 'm':
		ret <<= 10;
	case 'K':
	case 'k':
		ret <<= 10;
		(*retptr)++;
	default:
		break;
	}

	return ret;
}

#endif
#if 0
static ssize_t nand_sequence_speed_test_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	struct _nftl_blk *nftl_blk = aw_ndfc.nftl_blk;
	/*unsigned int size = nftl_blk->nftl_logic_size;*/
	unsigned char *buffer = NULL;
	struct timeval tv;
	struct timeval tv1;

	int free_block_num;
	u32 logic_page_size;
	u64 free_size;
	u64 test_size;
	u64 size;
	int i = 0;
	unsigned long long time;


	test_size = memsize_parse((const char *)buf, (const char **)&buf);
	size = test_size;

	printk("test_size:%llu\n", test_size);
	logic_page_size = nand_get_logic_page_size();
	free_block_num = get_free_block_num(nftl_blk->nftl_zone);
	free_size = free_block_num * nand_get_logic_block_size();

	buffer = nand_malloc(PAGE_SIZE);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	memset(buffer, 0xa5, PAGE_SIZE);


	test_size = (test_size / PAGE_SIZE) * PAGE_SIZE;

	/*write test*/
	do_gettimeofday(&tv);
	while (test_size) {

		test_size -= PAGE_SIZE;
	}

	do_gettimeofday(&tv1);

	/*ms*/
	time = (tv1.tv_sec * 1000000 + tv1.tv_usec - (tv.tv_sec * 1000000 + tv.tv_usec)) / 1000;


	test_size = size;
	printk("test_size:%llu time:%llu\n", test_size, time);
	printk("write: %dM\n", (test_size * 1000 / time) / 1024 / 1024);
out:
	printk("file:%x\n", file);
	nand_free(buffer);
	return ret;
	return 0;
}
#endif
static ssize_t nand_ndfc_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int index = 0;
	int i = 0;
	unsigned long size;

	void __iomem *mem = NULL;
	volatile unsigned char *base = NULL;
	struct resource res;
	int len;

	if (of_address_to_resource(dev->of_node, index, &res)) {
		nand_dbg_inf("get resource fail\n");
		return -ENXIO;
	}

	size = resource_size(&res);
	mem = ioremap(res.start, size);
	base = (volatile unsigned char *)mem;

	ret = snprintf(buf + ret, PAGE_SIZE - ret,
			"=========ndfc==========\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"=register=--====val===\n");

	/*skip reserve register*/
	len = 0x120;
	if (len > size)
		len = size;
	for (i = 0; i < len + 1; i += 4) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "0x%08x  0x%08x\n",
				(unsigned int)(res.start + i), readl(base + i));
	}

	len = 0x214;
	if (len > size)
		len = size;
	for (i = 0x200; i < len + 1; i += 4) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "0x%08x  0x%08x\n",
				(unsigned int)(res.start + i), readl(base + i));
	}

	ret += snprintf(buf + ret,  PAGE_SIZE - ret, "0x%08x  0x%08x\n",
			(unsigned int)(res.start + 0x2f0), readl(base + 0x2f0));
	ret += snprintf(buf + ret,  PAGE_SIZE - ret, "0x%08x  0x%08x\n",
			(unsigned int)(res.start + 0x300), readl(base + 0x300));
	ret += snprintf(buf + ret,  PAGE_SIZE - ret, "0x%08x  0x%08x\n",
			(unsigned int)(res.start + 0x400), readl(base + 0x400));
	ret += snprintf(buf + ret,  PAGE_SIZE - ret, "0x%08x  0x%08x\n",
			(unsigned int)(res.start + 0x800), readl(base + 0x800));

	iounmap(mem);

	return ret;
}

static ssize_t sunxi_nand_clk_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct device_node *np = NULL;
	struct resource res;
	unsigned long size;
	void __iomem *mem = NULL;
	volatile unsigned char *base = NULL;
	int i;

	np = of_find_node_by_type(NULL, "clocks");
	if (np == NULL) {
		nand_print("ERROR! get clocks node failed!\n");
		return -1;
	}

	if (of_address_to_resource(np, 0, &res)) {
		nand_dbg_inf("get resource fail\n");
		return -ENXIO;
	}

	/*ret = snprintf(buf, "%x %x %x %x\n", val[0], val[1], val[2], val[3]);*/
	size = resource_size(&res);
	mem = ioremap(res.start, size);
	base = (volatile unsigned char *)mem;

	ret = snprintf(buf + ret, PAGE_SIZE - ret,
			"=========clock=========\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"=register=--====val===\n");
#define PLL_GPU0_BASE 0x30

	for (i = 0; i < PLL_GPU0_BASE; i += 4) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "0x%08x  0x%08x\n",
				(unsigned int)(res.start + i), readl(base + i));
	}

#define SMHC0_CLK_BASE 0x834
	/* MBUS_MAT_CLK_GATING_REG 0x804
	 * NAND0_0_CLK_REG 0x810
	 * NAND0_1_CLK_REG 0x814
	 * */
	for (i = 0x800; i < SMHC0_CLK_BASE; i += 4) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "0x%08x  0x%08x\n",
				(unsigned int)(res.start + i), readl(base + i));
	}

	iounmap(mem);
	return ret;
}

static ssize_t sunxi_nand_pio_reg_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct device_node *np = NULL;
	struct resource res;
	unsigned long size;
	void __iomem *mem = NULL;
	volatile unsigned char *base = NULL;
	int i;

	np = of_find_node_by_type(NULL, "pio");
	if (np == NULL) {
		nand_print("ERROR! get clocks node failed!\n");
		return -1;
	}

	if (of_address_to_resource(np, 0, &res)) {
		nand_dbg_inf("get resource fail\n");
		return -ENXIO;
	}

	size = resource_size(&res);
	mem = ioremap(res.start, size);
	base = (volatile unsigned char *)mem;


	ret = snprintf(buf + ret, PAGE_SIZE - ret,
			"==========pio==========\n");
	ret += snprintf(buf + ret, PAGE_SIZE - ret,
			"=register=--====val===\n");
#define PC_BASE 0x48
#define PD_BASE 0x6C

	for (i = PC_BASE; i < PD_BASE; i += 4) {
		ret += snprintf(buf + ret, PAGE_SIZE - ret, "0x%08x  0x%08x\n",
				(unsigned int)(res.start + i), readl(base + i));
	}


	iounmap(mem);
	return ret;
}
static DEVICE_ATTR(sunxi_nftl_version, S_IRUGO, nand_nftl_version_show, NULL);
static DEVICE_ATTR(nand_device_arch, S_IRUGO, nand_arch_show, NULL);
static DEVICE_ATTR(nand_badblock, S_IRUGO, nand_badblock_show, NULL);
static DEVICE_ATTR(nand_gcinfo, S_IRUGO, nand_gcinfo_show, NULL);
static DEVICE_ATTR(nand_gcone, S_IRUGO | S_IWUSR, nand_gcone_show, nand_gcone_store);
static DEVICE_ATTR(nand_gcall, S_IRUGO | S_IWUSR, nand_gcall_show, nand_gcall_store);
static DEVICE_ATTR(nand_debug, S_IRUGO | S_IWUSR, nand_debug_show, nand_debug_store);
/*static DEVICE_ATTR(nand_speed_test, S_IWUSR, NULL, nand_sequence_speed_test_store);*/
static DEVICE_ATTR(sunxi_ndfc_reg, S_IRUGO, nand_ndfc_reg_show, NULL);
static DEVICE_ATTR(sunxi_nand_clk_reg, S_IRUGO, sunxi_nand_clk_reg_show, NULL);
static DEVICE_ATTR(sunxi_nand_pio_reg, S_IRUGO, sunxi_nand_pio_reg_show, NULL);

int nand_create_sys_fs(struct sunxi_ndfc *ndfc, struct platform_device  *pdev)
{
	int ret;

	/*create nftl_version node*/
	ret = device_create_file(&pdev->dev, &dev_attr_sunxi_nftl_version);
	if (ret < 0) {
		printk("device create file sunxi_nftl_version fail\n");
		return ret;
	}
	/*create arch node*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_device_arch);
	if (ret < 0) {
		printk("device create file nand_device fail\n");
		return ret;
	}
	/*create badblock node*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_badblock);
	if (ret < 0) {
		printk("device create file nand_badblock fail\n");
		return ret;
	}

	/*create gcinfo node*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_gcinfo);
	if (ret < 0) {
		printk("device create file nand_gcinfo fail\n");
		return ret;
	}

	/*create gcone node*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_gcone);
	if (ret < 0) {
		printk("device create file nand_gcone fail\n");
		return ret;
	}


	/*create gcall node*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_gcall);
	if (ret < 0) {
		printk("device create file nand_gcall fail\n");
		return ret;
	}

	/*create sunxi_ndfc_reg node*/
	ret = device_create_file(&pdev->dev, &dev_attr_sunxi_ndfc_reg);
	if (ret < 0) {
		printk("device create file sunxi_ndfc_reg fail\n");
		return ret;
	}

	/*create sunxi_nand_clk_reg node*/
	ret = device_create_file(&pdev->dev, &dev_attr_sunxi_nand_clk_reg);
	if (ret < 0) {
		printk("device create file sunxi_nand_clk_reg fail\n");
		return ret;
	}

	/*create sunxi_nand_pio_reg node*/
	ret = device_create_file(&pdev->dev, &dev_attr_sunxi_nand_pio_reg);
	if (ret < 0) {
		printk("device create file sunxi_nand_pio_reg fail\n");
		return ret;
	}

	/*create nand_debug*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_debug);
	if (ret < 0) {
		printk("device create file sunxi_nand_pio_reg fail\n");
		return ret;
	}
#if 0
	/*create speed test*/
	ret = device_create_file(&pdev->dev, &dev_attr_nand_speed_test);
	if (ret < 0) {
		printk("device create file sunxi_nand_pio_reg fail\n");
		return ret;
	}
#endif
	return ret;
}

