/*
 * SUNXI MTD partitioning
 *
 * Copyright © 2016 WimHuang <huangwei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt)	"sunxipart: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#define MBR_OFFSET		((512-16)*1024)
#define MBR_SIZE		(16 * 1024)
#define DL_SIZE			(16 * 1024)
#define MBR_MAGIC		"softw411"
#define MBR_MAX_PART_COUNT	120
#define MBR_RESERVED		(MBR_SIZE - 32 - (MBR_MAX_PART_COUNT * sizeof(struct sunxi_partition)))
#define NOR_BLK_SIZE		512

/* partition information */
struct sunxi_partition {
	unsigned  int		addrhi;
	unsigned  int		addrlo;
	unsigned  int		lenhi;
	unsigned  int		lenlo;
	unsigned  char		classname[16];
	unsigned  char		name[16];
	unsigned  int		user_type;
	unsigned  int		keydata;
	unsigned  int		ro;
	unsigned  char		reserved[68];
} __packed;

/* mbr information */
struct sunxi_mbr {
	unsigned  int		crc32;
	unsigned  int		version;
	unsigned  char		magic[8];
	unsigned  int		copy;
	unsigned  int		index;
	unsigned  int		PartCount;
	unsigned  int		stamp[1];
	struct sunxi_partition	array[MBR_MAX_PART_COUNT];
	unsigned  char		res[MBR_RESERVED];
} __packed;

/* save partition's name */
static char partition_name[MBR_MAX_PART_COUNT][16];

static void sunxipart_add_part(struct mtd_partition *part, char *name,
				uint64_t size, uint64_t offset)
{
	part->name = name;
	part->size = size;
	part->offset = offset;
}

static int sunxipart_parse(struct mtd_info *master,
				const struct mtd_partition **pparts,
				struct mtd_part_parser_data *data)
{
	int i, ret, nrparts;
	size_t bytes_read;
	struct sunxi_mbr *sunxi_mbr;
	struct mtd_partition *parts;

	sunxi_mbr = kzalloc(MBR_SIZE, GFP_KERNEL);
	if (sunxi_mbr == NULL) {
		pr_err("failed to alloc sunxi_mbr\n");
		return -ENOMEM;
	}

	ret = mtd_read(master, MBR_OFFSET, MBR_SIZE,
		       &bytes_read, (uint8_t *)sunxi_mbr);
	if ((ret < 0)) {
		pr_err("failed to read sunxi_mbr!\n");
		kfree(sunxi_mbr);
		return -EIO;
	}

	if ((sunxi_mbr->PartCount == 0)
	     || (sunxi_mbr->PartCount > MBR_MAX_PART_COUNT)) {
		pr_err("failed to parse sunxi_mbr)!\n");
		kfree(sunxi_mbr);
		return -EINVAL;
	}

	nrparts = sunxi_mbr->PartCount + 1;
	parts = kzalloc(nrparts * sizeof(*parts), GFP_KERNEL);
	if (parts == NULL) {
		pr_err("failed to alloc %d patitions\n", nrparts);
		kfree(sunxi_mbr);
		return -ENOMEM;
	}

	strncpy(partition_name[0], "uboot", 16);
	sunxipart_add_part(&parts[0], partition_name[0],
					MBR_OFFSET + MBR_SIZE, 0);
	for (i = 0; i < nrparts; i++) {
		strncpy(partition_name[i+1],
			sunxi_mbr->array[i].name, 16);

		sunxipart_add_part(&parts[i+1],
			partition_name[i+1],
			sunxi_mbr->array[i].lenlo * NOR_BLK_SIZE,
			sunxi_mbr->array[i].addrlo * NOR_BLK_SIZE + MBR_OFFSET);
	}

	kfree(sunxi_mbr);
	*pparts = parts;
	return nrparts;
}

static struct mtd_part_parser sunxipart_mtd_parser = {
	.owner = THIS_MODULE,
	.parse_fn = sunxipart_parse,
	.name = "sunxipart",
};

static int __init sunxipart_init(void)
{
	register_mtd_parser(&sunxipart_mtd_parser);

	return 0;
}

static void __exit sunxipart_exit(void)
{
	deregister_mtd_parser(&sunxipart_mtd_parser);
}

module_init(sunxipart_init);
module_exit(sunxipart_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MTD partitioning for SUNXI flash memories");
