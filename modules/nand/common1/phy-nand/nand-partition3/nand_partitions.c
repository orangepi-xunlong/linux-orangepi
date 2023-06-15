/*
 * nand_partitions.c
 *
 * Copyright (C) 2019 Allwinner.
 *
 * cuizhikui <cuizhikui@allwinnertech.com>
 * 2021-09-24
 *
 * SPDX-License-Identifier: GPL-2.0
 */
#include "sunxi_nand.h"
#include "sunxi_nand_boot.h"
#include "sunxi_nand_partitions.h"
#include <linux/kernel.h>
#include "../nand_nftl.h"
#include <linux/types.h>
#include <linux/printk.h>


extern struct nand_partitions *get_nand_parts(void);
extern int nand_physic_lock(void);
extern int nand_physic_unlock(void);
extern int virtual_badblock_check(unsigned int nDieNum, unsigned int nBlkNum);
extern int read_virtual_page(unsigned int nDieNum, unsigned int nBlkNum,
		unsigned int nPage, uint64_t SectBitmap, void *pBuf, void *pSpare);
extern int write_virtual_page(unsigned int nDieNum, unsigned int nBlkNum,
		unsigned int nPage, uint64_t SectBitmap, void *pBuf, void *pSpare);
extern int erase_virtual_block(unsigned int nDieNum, unsigned int nBlkNum);
extern int nand_wait_all_rb_ready(void);
extern int virtual_badblock_mark(unsigned int nDieNum, unsigned int nBlkNum);
extern int nand_secure_storage_first_build(unsigned int start_block);
extern __u32 nand_get_twoplane_flag(void);
extern unsigned int nand_get_super_chip_page_size(void);
extern unsigned int nand_get_super_chip_spare_size(void);
extern unsigned int nand_get_super_chip_block_size(void);
extern unsigned int nand_get_super_chip_size(void);
extern unsigned int nand_get_super_chip_cnt(void);

struct kernel_status kernelsta;
struct nand_partitions nand_parts;

int free_phy_partition(struct _nand_phy_partition *phy_partition)
{
	nand_free(phy_partition->factory_bad_block);
	nand_free(phy_partition->new_bad_block);
	nand_free(phy_partition);
	return 0;
}
int free_nand_partition(struct _nand_partition *nand_partition)
{
	free_phy_partition(nand_partition->phy_partition);
	nand_free(nand_partition);

	return 0;
}
struct nand_partitions *get_nand_parts(void)
{
	return &nand_parts;
}
struct _nand_phy_partition *get_head_phy_partition_from_nand_info(struct _nand_info *nand_info)
{
	return nand_info->phy_partition_head;
}

void set_cache_level(struct _nand_info *nand_info, unsigned short cache_level)
{
	nand_info->cache_level = cache_level;
}

struct _nand_disk *get_disk_from_phy_partition(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->disk;
}

struct _nand_phy_partition *get_next_phy_partition(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->next_phy_partition;
}

uint16 get_partitionNO(struct _nand_phy_partition *phy_partition)
{
	return phy_partition->PartitionNO;
}

uint64_t bitmap_change(unsigned short SectBitmap)
{
	uint64_t bitmap = 0;
	int i;

	if (get_storage_type() == 1) {
		bitmap = SectBitmap & 0xff;
	} else if (get_storage_type() == 2) {
		for (i = 0, bitmap = 0; i < SectBitmap; i++) {
			bitmap <<= 1;
			bitmap |= 0x01;
		}
	}

	return bitmap;
}

void wait_all_rb(void)
{
	nand_physic_lock();
	nand_wait_all_rb_ready();
	nand_physic_unlock();
}

int page_read(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare)
{
	uint64_t bitmap;
	int ret;
	nand_physic_lock();
	bitmap = bitmap_change(SectBitmap);
	ret = read_virtual_page(nDieNum, nBlkNum, nPage, bitmap, pBuf, pSpare);
	nand_physic_unlock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int page_write(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare)
{
	uint64_t bitmap;
	int ret;
	nand_physic_lock();
	bitmap = bitmap_change(SectBitmap);
	ret = write_virtual_page(nDieNum, nBlkNum, nPage, bitmap, pBuf, pSpare);
	nand_physic_unlock();

	return ret;
}

int block_erase(unsigned short nDieNum, unsigned short nBlkNum)
{
	int ret;
	nand_physic_lock();
	ret = erase_virtual_block(nDieNum, nBlkNum);
	nand_physic_unlock();

	return ret;
}

void change_block_addr(struct _nand_partition *nand, struct _nand_super_block *super_block, unsigned short nBlkNum)
{
	super_block->Chip_NO = nand->phy_partition->StartBlock.Chip_NO;

	super_block->Block_NO = nand->phy_partition->StartBlock.Block_NO + nBlkNum;

	while (super_block->Block_NO >= nand->phy_partition->nand_info->BlkPerChip) {
		super_block->Chip_NO++;
		super_block->Block_NO -= nand->phy_partition->nand_info->BlkPerChip;
	}
}

int nand_erase_superblk(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);
	cur_e_lb_no = p->phy_page.Block_NO;
	ret = nand->phy_partition->block_erase(super_block.Chip_NO, super_block.Block_NO);

	return ret;
}

int nand_read_page(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = nand->phy_partition->page_read(super_block.Chip_NO, super_block.Block_NO, p->phy_page.Page_NO, p->page_bitmap, p->main_data_addr, p->spare_data_addr);

	return ret;
}

int nand_write_page(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	if (nand->phy_partition->CrossTalk != 0) {
		//wait RB
		wait_all_rb();
	}

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);
	cur_w_lb_no = p->phy_page.Block_NO;
	cur_w_pb_no = super_block.Block_NO;
	cur_w_p_no = p->phy_page.Page_NO;
	ret = nand->phy_partition->page_write(super_block.Chip_NO, super_block.Block_NO, p->phy_page.Page_NO, p->page_bitmap, p->main_data_addr, p->spare_data_addr);

	return ret;
}

int is_factory_bad_block(struct _nand_info *nand_info, unsigned short nDieNum, unsigned short nBlkNum)
{
	uint32_t num, i;

	num = FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if ((nand_info->factory_bad_block[i].Chip_NO == nDieNum) && (nand_info->factory_bad_block[i].Block_NO == nBlkNum)) {
			return 1;
		}
		if (nand_info->factory_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	return 0;
}

int is_new_bad_block(struct _nand_info *nand_info, unsigned short nDieNum, unsigned short nBlkNum)
{
	uint32_t num, i;

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if ((nand_info->new_bad_block[i].Chip_NO == nDieNum) && (nand_info->new_bad_block[i].Block_NO == nBlkNum)) {
			return 1;
		}
		if (nand_info->new_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	return 0;
}
int nand_is_blk_good(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	ret = 0;
	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = is_factory_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO, super_block.Block_NO);
	if (ret != 0) {
		return NFTL_NO;
	}
	ret = is_new_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO, super_block.Block_NO);
	if (ret != 0) {
		return NFTL_NO;
	}

	return NFTL_YES;
}

int add_new_bad_block(struct _nand_info *nand_info, unsigned short nDieNum, unsigned short nBlkNum)
{
	uint32 num, i;

	num = PHY_PARTITION_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block);
	for (i = 0; i < num; i++) {
		if (nand_info->new_bad_block[i].Chip_NO == 0xffff) {
			break;
		}
	}
	if (i < num) {
		nand_info->new_bad_block[i].Chip_NO = nDieNum;
		nand_info->new_bad_block[i].Block_NO = nBlkNum;

	} else {
		NFTL_ERR("[NE]Too much new bad block\n");
	}

	return 0;
}

int nand_mark_bad_blk(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];

	memset(spare, 0, BYTES_OF_USER_PER_PAGE);
	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	nand_physic_lock();
	ret = virtual_badblock_mark(super_block.Chip_NO, super_block.Block_NO);
	nand_physic_unlock();

	add_new_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO, super_block.Block_NO);

	return ret;
}

int nand_get_logic_start_block(struct _nand_info *nand_info, int state)
{
	/*boot0 area | uboot area | secure storage area | reserved area |logic area*/
	unsigned int block_temp, logic_start_block;
	struct _boot_info *boot_info = nand_info->boot;


	if (state == KERNEL_IN_BOOT)
		return boot_info->logic_start_block;

	block_temp = nand_info->boot->uboot_next_block;

	block_temp = nand_secure_storage_first_build(block_temp);

	block_temp = block_temp + nand_info->boot->physic_block_reserved;

	logic_start_block = block_temp;
	if (nand_get_twoplane_flag() == 1) {
		/*super block unit*/
		logic_start_block = logic_start_block / 2;
		if (block_temp % 2)
			logic_start_block++;
	}

	pr_info("support two plane:%d , logic start block@%d\n",
			nand_get_twoplane_flag(), logic_start_block);

	return logic_start_block;
}

/*
 *int nand_info_partitions_init(struct _nand_info *nand_info, unsigned char *mbr_data)
 *{
 *
 *        int last_num = 0, nums = 0, new_partition;
 *        PARTITION_MBR *mbr;
 *        unsigned int part_cnt, i, m;
 *        int p = 0;
 *
 *        mbr = (PARTITION_MBR *)mbr_data;
 *
 *        memset(nand_info->partition, 0xff, sizeof(nand_info->partition));
 *
 *        nand_info->partition[0].cross_talk = 0;
 *        nand_info->partition[0].attribute = 0;
 *        nand_info->partition_nums = 1;
 *
 *        return 0;
 *}
 */

int BlockCheck(unsigned short nDieNum, unsigned short nBlkNum)
{
	int ret = 0;

	nand_physic_lock();
	ret = virtual_badblock_check(nDieNum, nBlkNum);
	nand_physic_unlock();

	return ret;
}

void print_bad_blocks(struct _nand_super_block *badblocks, int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		if (badblocks[i].Chip_NO != 0xffff && badblocks[i].Block_NO != 0xffff)
			printk("%d-%d ", badblocks[i].Chip_NO, badblocks[i].Block_NO);
	}
	printk("\n");
}

int nand_scan_phy_partitions_bad_blocks(struct _nand_info *nand_info)
{
	int die = 0;
	int block = 0;
	int factory_bad_blocks = 0;
	int new_bad_blocks = 0;
	int bad_blocks = 0;

	if (nand_info->FirstBuild == 1) {
		for (die = nand_info->no_used_block_addr.Chip_NO;
				die < nand_info->ChipNum; die++) {
			for (block = nand_info->no_used_block_addr.Block_NO;
					block < nand_info->BlkPerChip; block++) {

				if (kernelsta.nand_bootinfo_state.state == BOOT_INFO_OK) {
					/*not erase whole chip to burn would be this case*/
					if (is_factory_bad_block(nand_info, die, block) == 1) {

						factory_bad_blocks++;

						if (factory_bad_blocks == FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
							pr_err("[NE]too much factory bad block %d\n", new_bad_blocks);
							return -1;
						}
					} else if (BlockCheck(die, block) != 0) {

						nand_info->new_bad_block[new_bad_blocks].Chip_NO = die;
						nand_info->new_bad_block[new_bad_blocks].Block_NO = block;
						new_bad_blocks++;
						if (new_bad_blocks == PHY_PARTITION_BAD_BLOCK_SIZE  / sizeof(struct _nand_super_block)) {
							pr_err("\n[NE]too much new bad block %d\n", factory_bad_blocks);
							return -1;
						}
					}
				} else {
					if (BlockCheck(die, block) != 0) {

						nand_info->factory_bad_block[factory_bad_blocks].Chip_NO = die;
						nand_info->factory_bad_block[factory_bad_blocks].Block_NO = block;
						factory_bad_blocks++;
						if (factory_bad_blocks == FACTORY_BAD_BLOCK_SIZE / sizeof(struct _nand_super_block)) {
							pr_err("\n[NE]too much bad block %d\n", factory_bad_blocks);
							return -1;
						}
					}
				}

			}
		}
		bad_blocks = factory_bad_blocks + new_bad_blocks;
		if (bad_blocks != 0) {
			printk("factory bad block:\n");
			print_bad_blocks(nand_info->factory_bad_block, bad_blocks);
		}

		if (new_bad_blocks != 0) {
			printk("new bad block:\n");
			print_bad_blocks(nand_info->new_bad_block, new_bad_blocks);
		}
	} else {

		/*scan bad block in nftl when normal boot*/
	}

	return bad_blocks;
}

static int get_max_free_block_num(struct _nand_info *nand_info)
{
	int max_free_block_num, sector_per_block;
	uint32_t total_sector;

	sector_per_block = nand_info->SectorNumsPerPage * nand_info->PageNumsPerBlk;
	total_sector = sector_per_block * nand_info->BlkPerChip;
	total_sector *= nand_info->ChipNum;

	if (total_sector <= 0x64000) { //less than 200MB {

		max_free_block_num = 40;
	} else if (total_sector <= 0x96000) { //less than 300MB {

		max_free_block_num = 85;
	} else if (total_sector <= 0x12c000) { //less than 600MB {

		if (sector_per_block >= 2048) {
			max_free_block_num = 40;
		} else if (sector_per_block >= 1024) {
			max_free_block_num = 80;
		} else {
			max_free_block_num = 180;
		}
	} else if (total_sector <= 0x258000) { //less than 1200MB{

		if (sector_per_block >= 1024) {
			max_free_block_num = 170;
		} else {
			max_free_block_num = 320;
		}
	} else if (total_sector <= 0xa00000) { //less than 5GB {

		if (sector_per_block >= 16384) {
			max_free_block_num = 50;
		} else if (sector_per_block >= 8182) {
			max_free_block_num = 75;
		} else if (sector_per_block >= 4096) {
			max_free_block_num = 170;
		} else {
			max_free_block_num = 300;
		}
	} else if (total_sector <= 0x1400000) { //less than 10GB{

		if (sector_per_block >= 16384) {
			max_free_block_num = 75;
		} else if (sector_per_block >= 8182) {
			max_free_block_num = 160;
		} else if (sector_per_block >= 4096) {
			max_free_block_num = 300;
		} else {
			max_free_block_num = 600;
		}
	} else if (total_sector <= 0x2800000) { //less than 20GB {

		if (sector_per_block >= 32768) {
			max_free_block_num = 70;
		} else if (sector_per_block >= 16384) {
			max_free_block_num = 150;
		} else if (sector_per_block >= 8182) {
			max_free_block_num = 160;
		} else {
			max_free_block_num = 240;
		}
	} else { //32G {

		if (sector_per_block >= 32768) {
			max_free_block_num = 140;
		} else if (sector_per_block >= 16384) {
			max_free_block_num = 250;
		} else {
			max_free_block_num = 500;
		}
	}

	return max_free_block_num;
}


unsigned int nand_reserve_blocks(struct _nand_info *nand_info,
		unsigned int total_good_blocks)
{
	unsigned int resv = 0;
	unsigned int total_block_nums = 0;
	int max_free_block_num = 0;

	total_block_nums = total_good_blocks;


	resv = total_block_nums / (NORM_RESERVED_BLOCK_RATIO + nand_info->capacity_level * 3);

	if (resv < nand_info->mini_free_block_first_reserved) {
		resv = nand_info->mini_free_block_first_reserved;
	}

	max_free_block_num = get_max_free_block_num(nand_info);
	if (resv > max_free_block_num) {
		resv = max_free_block_num;
	}

	pr_debug("nand reserve block nums@%u\n", resv);
	return resv;
}

/*predefine from sys_partition.fex*/
uint32_t nand_predefine_parts_size(struct nand_partitions *nparts)
{
	int i = 0;
	uint32_t size = 0;
	struct nand_part *np = nparts->parts;

	for (i = 0; i < nparts->nc; i++) {
		size += np[i].size;
	}

	return size;
}

uint32_t nand_calc_phy_partitions_size(struct _nand_info *nand_info, int total_blocks, int bad_block_nums)
{
	uint32_t partitions_size = 0; /*sector as unit*/
	uint32_t total_good_blocks = 0;
	uint32_t partitions_blocks = 0;

	pr_debug("cal partitions size:[%d-%d]\n", total_blocks, bad_block_nums);
	total_good_blocks = total_blocks - bad_block_nums;
	partitions_blocks = total_good_blocks - nand_reserve_blocks(nand_info,
			total_good_blocks);

	partitions_size = partitions_blocks * nand_info->PageNumsPerBlk;
	partitions_size *= nand_info->SectorNumsPerPage;

	return partitions_size;
}

uint32_t nand_get_phy_partitions_size(struct _nand_info *nand_info, struct _boot_info *boot)
{
	uint32_t size = 0;

	struct _partition *parts = boot->partition.data;

	if (nand_info->bootinfo_is_old == BOOTINFO_OLD) {
		size = parts->size;
		pr_info("boot info is old, the parts size@%u\n", size);
	} else
		size = boot->parts.size;

	return size;
}

uint32_t nand_calc_udisk_capacity(struct _nand_info *nand_info)
{
	struct _nand_phy_partition *phy_partition = NULL;
	struct nand_partitions *predef_nand_parts = NULL;
	uint32_t defsize = 0, total_avalid_size = 0;

	if (nand_info == NULL || nand_info->phy_partition_head == NULL) {
		pr_err("%s nand_info or nand info phy_partition_head is null\n",
				__func__);
		return NOSPACE;
	}
	phy_partition = nand_info->phy_partition_head;

	predef_nand_parts = get_nand_parts();

	defsize = nand_predefine_parts_size(predef_nand_parts);
	total_avalid_size = phy_partition->TotalSectors;

	if (defsize > total_avalid_size) {
		pr_err("[NPE] define partitons size@%u greater than avalidable space@%u\n",
				defsize, total_avalid_size);
		return NOSPACE;
	}

	return total_avalid_size - defsize;
}

void nand_partitions_phy_range(struct nand_partitions *parts, struct _nand_super_block start,
		struct _nand_super_block end)
{
	parts->start = start;
	parts->end = end;
}
void nand_partitions_append_udisk(struct nand_partitions *parts, unsigned int udisk_size)
{
	int p = 0;


	for (p = 0; p < parts->nc; p++) {
		parts->parts[parts->nc].from += parts->parts[p].size;
	}
	if (parts->nc == 0)
		parts->nc = 1;

	parts->parts[parts->nc - 1].size = udisk_size;

	/*update total size*/
	parts->size += udisk_size;

	pr_info("parts size:%u (sect)\n", parts->size);
}

int nand_check_partitions_consistent_legal(struct nand_partitions *new, struct nand_partitions *old)
{
	int p = 0;
	/*check partitions count*/
	pr_debug("new nc:%d old nc:%d\n", new->nc, old->nc);

	/*some a100 a133 boards's parts force to one parts*/
	if (old->nc == 1 && old->size == new->size) {
		pr_info("special machine: old parts count is one, old part size%u is the same with new size@%u\n", old->size, new->size);
		return 0;
	}
	if (new->nc != old->nc) {
		pr_err("update parts count is inconsistent [%d-%d]\n", new->nc, old->nc);
		return NAND_PARTS_COUNT_INCONSISTENT;
	}

	for (p = 0; p < new->nc; p++) {
		pr_debug("new->parts[%d].from:%u ,old->parts[%d].from:%u\n", p, new->parts[p].from, p, old->parts[p].from);
		pr_debug("new->parts[%d].size:%u ,old->parts[%d].size:%u\n", p, new->parts[p].size, p, old->parts[p].size);
		if (new->parts[p].from != old->parts[p].from ||
				new->parts[p].size != old->parts[p].size) {
			pr_err("update parts size is inconsitent\n");
			return NAND_PARTS_SIZE_INCONSISTENT;
		}
	}

	return 0;
}

/*for update new version(parts2.0 --> parts3.0)*/
void nand_parts2_to_parts3(struct nand_partitions *parts3, PARTITION_MBR *parts2_mbr, _PARTITION *parts2_parts, unsigned int old_parts_size)
{
	int p = 0;
	unsigned int size = 0;

	parts3->size = old_parts_size;
	parts3->nc = parts2_mbr->PartCount;
	parts3->cross_talk = parts2_parts->data[0].cross_talk;
	parts3->attribute = parts2_parts->data[0].attribute;
	parts3->start = parts2_parts->data[0].start;
	parts3->end = parts2_parts->data[0].end;

	for (p = 0; p < parts2_mbr->PartCount; p++) {
		parts3->parts[p].from = parts2_mbr->array[p].addr;
		parts3->parts[p].size = parts2_mbr->array[p].len;
		size += parts3->parts[p].size;
	}

	/*udisk size*/
	parts3->parts[p - 1].size = parts3->size - size;

	pr_info("change parts2 to parts3\n");
}

/*for back to old version(parts3.o --> parts2.0)*/
void nand_parts3_to_parts2(struct nand_partitions *parts3, PARTITION_MBR *parts2_mbr,
		_PARTITION *parts2_parts, unsigned int new_parts_size)
{
	int p = 0;

	if (parts3->nc > MAX_PART_COUNT_PER_FTL) {
		parts2_mbr->PartCount = 1;
		parts2_parts->data[0].size = new_parts_size;
		parts2_parts->data[0].cross_talk = parts3->cross_talk;
		parts2_parts->data[0].attribute = parts3->attribute;
		parts2_parts->data[0].start = parts3->start;
		parts2_parts->data[0].end = parts3->end;
		parts2_parts->data[0].nand_disk[0].size = parts2_parts->data[0].size;

		return;
	}

	/*build 2.0 mbr*/
	parts2_mbr->PartCount = parts3->nc;
	for (p = 0; p < parts3->nc; p++) {
		parts2_mbr->array[p].addr = parts3->parts[p].from;
		parts2_mbr->array[p].len = parts3->parts[p].size;
	}

	/*build 2.0 partition*/
	parts2_parts->data[0].size = new_parts_size;
	parts2_parts->data[0].cross_talk = parts3->cross_talk;
	parts2_parts->data[0].attribute = parts3->attribute;
	parts2_parts->data[0].start = parts3->start;
	parts2_parts->data[0].end = parts3->end;
	for (p = 0; p < parts3->nc; p++) {
		parts2_parts->data[0].nand_disk[p].size = parts3->parts[p].size;
	}
}

int nand_build_boot_info_f(struct _nand_info *nand_info, int state)
{
	unsigned int uboot_start_block = 0, uboot_next_block = 0;
	int ret;

	if (nand_info == NULL) {
		pr_err("%s nand_info is null\n", __func__);
		return -1;
	}

	ret = nand_physic_info_read();
	if (ret == -ENOMEM) {
		pr_err("nand build boot info fail\n");
		return -1;
	}

	nand_info->boot = phyinfo_buf;

	if (nand_info->boot->append_new_magic != PARTS3P0_MAGIC) {
		nand_info->bootinfo_is_old = BOOTINFO_OLD;
	}

	nand_info->boot->len = PHY_INFO_SIZE;
	nand_info->boot->magic = PHY_INFO_MAGIC;
	nand_info->boot->append_new_magic = PARTS3P0_MAGIC; /*parts3p0 -> 0x7061727473337030*/

	if (kernelsta.nand_bootinfo_state.state == BOOT_INFO_NOEXIST)
		return 0;

	if (nand_info->bootinfo_is_old == BOOTINFO_OLD &&
			kernelsta.nand_bootinfo_state.state == BOOT_INFO_OK) {
		nand_parts2_to_parts3(&nand_info->boot->parts, &nand_info->boot->mbr.data,
				&nand_info->boot->partition, nand_info->boot->partition.data[0].size);
	} else {
		nand_info->bootinfo_is_old = BOOTINFO_NULL;
		nand_info->boot->physic_block_reserved = PHYSIC_RECV_BLOCK;
	}

	if (nand_info->boot->uboot_start_block == 0) {
		get_default_uboot_block(&uboot_start_block, &uboot_next_block);
		nand_info->boot->uboot_start_block = uboot_start_block;
		nand_info->boot->uboot_next_block = uboot_next_block;
	}

	return 0;
}
int nand_build_boot_info_r(struct _nand_info *nand_info, int state)
{

	struct _boot_info *boot_info = nand_info->boot;

	if (nand_info == NULL || nand_info->boot == NULL) {
		pr_err("%s nand_info or nand_info boot is null\n", __func__);
		return -1;
	}

	if (state == KERNEL_IN_BOOT) {
		/*for kernel compatible with old version (parts 2.0) boot*/
		nand_parts3_to_parts2(&boot_info->parts, &boot_info->mbr.data,
				&boot_info->partition, boot_info->parts.size);
		return 0;
	}

	boot_info->logic_start_block = nand_get_logic_start_block(nand_info, state);
	boot_info->no_use_block = boot_info->logic_start_block;

	memcpy(boot_info->factory_block.ndata, nand_info->factory_bad_block, FACTORY_BAD_BLOCK_SIZE);

	memcpy(&boot_info->parts, &nand_parts, sizeof(struct nand_partitions));

	/*for kernel compatible with old version (parts 2.0) boot*/
	if (nand_info->bootinfo_is_old == BOOTINFO_NULL) {
		nand_parts3_to_parts2(&boot_info->parts, &boot_info->mbr.data,
				&boot_info->partition, boot_info->parts.size);
	}

	return 0;
}

unsigned int get_phy_partition_num(struct _nand_info *nand_info)
{
	unsigned int num = 0;

	struct _nand_phy_partition *p;

	p = nand_info->phy_partition_head;
	while (p != NULL) {
		num++;
		p = p->next_phy_partition;
	}
	return num;
}
int nand_build_phy_partitions(struct _nand_phy_partition **partition_head, int state)
{
	struct _nand_info *nand_info = container_of(partition_head, struct _nand_info, phy_partition_head);

	struct _nand_phy_partition *phy_partition = NULL;
	struct _partition *part = NULL;
	unsigned int total_blocks = 0;
	uint32_t partitions_size = 0; /*sector as unit*/
	int bad_block_nums = 0;

	if (partition_head == NULL) {
		pr_err("%s partition_head is null", __func__);
		return -1;
	}

	bad_block_nums = nand_scan_phy_partitions_bad_blocks(nand_info);
	if (bad_block_nums == -1) {
		pr_err("[NE] too much bad block, build phy partitions fail\n");
		return -1;
	}

	phy_partition = (struct _nand_phy_partition *)nand_malloc(sizeof(struct _nand_phy_partition));
	if (phy_partition == NULL) {
		pr_err("[NE]%s: malloc fail for phy_partition\n", __func__);
		return -ENOMEM;
	}

	memset(phy_partition, 0, sizeof(struct _nand_phy_partition));
	phy_partition->PartitionNO = 0;
	part = &nand_info->boot->partition.data[phy_partition->PartitionNO];

	total_blocks = nand_info->ChipNum * (nand_info->BlkPerChip - nand_info->no_used_block_addr.Block_NO);

	if (state == KERNEL_IN_PRODUCT) {

		partitions_size = nand_calc_phy_partitions_size(nand_info, total_blocks, bad_block_nums);

		phy_partition->CrossTalk = 0;
		phy_partition->Attribute = 0;
		phy_partition->StartBlock.Chip_NO = nand_info->no_used_block_addr.Chip_NO;
		phy_partition->StartBlock.Block_NO = nand_info->no_used_block_addr.Block_NO;
		phy_partition->EndBlock.Chip_NO = nand_info->ChipNum - 1;
		phy_partition->EndBlock.Block_NO = nand_info->BlkPerChip - 1;
	} else {
		partitions_size = nand_get_phy_partitions_size(nand_info, nand_info->boot);

		phy_partition->CrossTalk = part->cross_talk;
		phy_partition->Attribute = part->attribute;
		phy_partition->StartBlock.Chip_NO = part->start.Chip_NO;
		phy_partition->StartBlock.Block_NO = part->start.Block_NO;
		phy_partition->EndBlock.Chip_NO = part->end.Chip_NO;
		phy_partition->EndBlock.Block_NO = part->end.Block_NO;
	}

	phy_partition->SectorNumsPerPage = nand_info->SectorNumsPerPage;
	phy_partition->BytesUserData = nand_info->BytesUserData;
	phy_partition->PageNumsPerBlk = nand_info->PageNumsPerBlk;
	phy_partition->TotalBlkNum = total_blocks;
	phy_partition->GoodBlockNum = total_blocks - bad_block_nums;
	phy_partition->FullBitmapPerPage = phy_partition->SectorNumsPerPage;
	phy_partition->FreeBlock = 0;
	phy_partition->TotalSectors = partitions_size;

	phy_partition->page_read = page_read;
	phy_partition->page_write = page_write;
	phy_partition->block_erase = block_erase;

	phy_partition->disk = nand_info->partition[phy_partition->PartitionNO].nand_disk;
	phy_partition->disk->size = phy_partition->TotalSectors;
	memcpy(phy_partition->disk->name, "nand0", strlen("nand0"));
	phy_partition->disk->name[strlen("nand0")] = '\0';
	phy_partition->disk->type = 0;
	phy_partition->nand_info = nand_info;

	*partition_head = phy_partition;

	pr_debug("phy partition: page size: %u(sec)\n", phy_partition->SectorNumsPerPage);
	pr_debug("phy partition: user size: %u(B)\n", phy_partition->BytesUserData);
	pr_debug("phy partition: blks size: %u(page)\n", phy_partition->PageNumsPerBlk);
	pr_debug("phy partition: full bitmap per page: %u\n", phy_partition->FullBitmapPerPage);
	pr_debug("phy partition: attribute: %d\n", phy_partition->Attribute);
	pr_debug("phy partition: crosstalk: %d\n", phy_partition->CrossTalk);
	pr_debug("phy partition: free block: %d\n", phy_partition->FreeBlock);
	pr_info("phy partition: start block: %d-%d\n", phy_partition->StartBlock.Chip_NO, phy_partition->StartBlock.Block_NO);
	pr_info("phy partition: end block: %d.%d\n", phy_partition->EndBlock.Chip_NO, phy_partition->EndBlock.Block_NO);
	pr_info("phy partition: total sectors: %u\n", phy_partition->TotalSectors);

	return 0;
}

struct _nand_partition *build_nand_partition(struct _nand_phy_partition *phy_partition)
{
	struct _nand_partition *partition;

	if (phy_partition == NULL) {
		pr_err("%s phy_partition is null", __func__);
		return NULL;
	}

	partition = nand_malloc(sizeof(struct _nand_partition));
	if (partition == NULL) {
		pr_err("[NE]%s:malloc fail for partition\n", __func__);
		return NULL;
	}
	partition->phy_partition = phy_partition;
	partition->sectors_per_page = phy_partition->SectorNumsPerPage;
	partition->spare_bytes = phy_partition->BytesUserData;
	partition->pages_per_block = phy_partition->PageNumsPerBlk;
	partition->bytes_per_page = partition->sectors_per_page;
	partition->bytes_per_page <<= 9;
	partition->bytes_per_block = partition->bytes_per_page * partition->pages_per_block;
	partition->full_bitmap = phy_partition->FullBitmapPerPage;

	/*partitions size include udisk part*/
	partition->cap_by_sectors = phy_partition->TotalSectors;
	partition->cap_by_bytes = partition->cap_by_sectors << 9;
	partition->total_blocks = phy_partition->TotalBlkNum - phy_partition->FreeBlock;

	/*total blocks include good and bad block*/
	partition->total_by_bytes = partition->total_blocks;
	partition->total_by_bytes *= partition->bytes_per_block;

	partition->nand_erase_superblk = nand_erase_superblk;
	partition->nand_read_page = nand_read_page;
	partition->nand_write_page = nand_write_page;
	partition->nand_is_blk_good = nand_is_blk_good;
	partition->nand_mark_bad_blk = nand_mark_bad_blk;

	memcpy(partition->name, "nand_partition0", sizeof("nand_partition0"));
	partition->name[14] += phy_partition->PartitionNO;

	pr_debug("[ND]partitions: name: %s\n", partition->name);
	pr_info("partitions: sectors_per_page: %d\n", partition->sectors_per_page);
	pr_info("partitions: spare_bytes: %d\n", partition->spare_bytes);
	pr_info("partitions: capacity size: %llu(sec)\n", partition->cap_by_sectors);
	pr_info("partitions: total size: %llu(sec)\n", partition->total_by_bytes >> 9);
	return partition;
}


void set_capacity_level(struct _nand_info *nand_info, unsigned short capacity_level)
{
	if (nand_info == NULL) {
		pr_err("%s nand_info is null", __func__);
		return;
	}

	if (capacity_level != 0)
		nand_info->capacity_level = 1;
	else
		nand_info->capacity_level = 0;
}

/*@state: boot@0, product@1*/
int nand_info_init(struct _nand_info *nand_info, int state)
{
	unsigned int bytes_per_page = 0;
	int ret = 0;
	unsigned int udisk_size = 0;

	if (nand_info == NULL) {
		pr_err("%s nand_info is null\n", __func__);
		return -1;
	}

	nand_info->type = MLC_NAND;
	nand_info->SectorNumsPerPage = nand_get_super_chip_page_size();
	nand_info->BytesUserData = nand_get_super_chip_spare_size();
	nand_info->PageNumsPerBlk = nand_get_super_chip_block_size();
	nand_info->BlkPerChip = nand_get_super_chip_size();
	nand_info->ChipNum = nand_get_super_chip_cnt();
	nand_info->FirstBuild = (state == KERNEL_IN_PRODUCT) ? 1 : 0;

	nand_info->mini_free_block_first_reserved = MIN_NAND_Free_BLOCK_NUM_V2;
	nand_info->mini_free_block_reserved = MIN_NAND_Free_BLOCK_REMAIN;
	nand_info->new_bad_page_addr = 0xffff;

	memset(nand_info->partition, 0xff, sizeof(nand_info->partition));


	nand_info->partition[0].cross_talk = 0;
	nand_info->partition[0].attribute = 0;
	nand_info->partition_nums = 1;
	bytes_per_page = nand_info->SectorNumsPerPage;
	bytes_per_page <<= 9;


	nand_info->temp_page_buf = nand_malloc(bytes_per_page);
	if (nand_info->temp_page_buf == NULL) {
		pr_err("%s: malloc fail for temp_page_buf!\n", __func__);
		ret = -ENOMEM;
		goto out_nomem;
	}

	nand_info->factory_bad_block = nand_malloc(FACTORY_BAD_BLOCK_SIZE);
	if (nand_info->factory_bad_block == NULL) {
		pr_err("%s: malloc fail for factory_bad_block!\n", __func__);
		ret = -ENOMEM;
		goto out_nomem;
	}

	nand_info->new_bad_block = nand_malloc(PHY_PARTITION_BAD_BLOCK_SIZE);
	if (nand_info->new_bad_block == NULL) {
		pr_err("%s: malloc fail for new_bad_block!\n", __func__);
		ret = -ENOMEM;
		goto out_nomem;
	}

	nand_info->mbr_data = nand_malloc(sizeof(PARTITION_MBR));
	if (nand_info->mbr_data == NULL) {
		pr_err("%s: malloc fail for mbr_data!\n", __func__);
		ret = -ENOMEM;
		goto out_nomem;
	}

	if (nand_build_boot_info_f(nand_info, state) != 0) {
		pr_err("%s: build boot info f fail\n", __func__);
		goto out;
	}

	if (kernelsta.nand_bootinfo_state.state == BOOT_INFO_NOEXIST)
		state = KERNEL_IN_PRODUCT;



	nand_info->no_used_block_addr.Chip_NO = 0;
	nand_info->no_used_block_addr.Block_NO = nand_get_logic_start_block(nand_info, state);



	if (kernelsta.nand_bootinfo_state.state == BOOT_INFO_OK) {
		memcpy(nand_info->factory_bad_block, &nand_info->boot->factory_block, FACTORY_BAD_BLOCK_SIZE);
	} else {
		memset(nand_info->factory_bad_block, 0xff, FACTORY_BAD_BLOCK_SIZE);
		memset(nand_info->new_bad_block, 0xff, PHY_PARTITION_BAD_BLOCK_SIZE);
		memset(nand_info->mbr_data, 0xff, sizeof(PARTITION_MBR));
	}

	if (nand_build_phy_partitions(&(nand_info->phy_partition_head), state) != 0) {
		pr_err("%s: build phy partitions fail\n", __func__);
		goto out;
	}

	if ((state == KERNEL_IN_PRODUCT) && ((udisk_size = nand_calc_udisk_capacity(nand_info)) == NOSPACE))
		goto out;
	else if (state == KERNEL_IN_PRODUCT) {
		struct _nand_phy_partition *phy_partition = nand_info->phy_partition_head;

		nand_partitions_append_udisk(&nand_parts, udisk_size);
		nand_partitions_phy_range(&nand_parts, phy_partition->StartBlock, phy_partition->EndBlock);
	}

	nand_build_boot_info_r(nand_info, state);

	return 0;
out:
out_nomem:
	if (nand_info->temp_page_buf)
		nand_free(nand_info->temp_page_buf);
	if (nand_info->factory_bad_block)
		nand_free(nand_info->factory_bad_block);
	if (nand_info->new_bad_block)
		nand_free(nand_info->new_bad_block);
	if (nand_info->mbr_data)
		nand_free(nand_info->mbr_data);
	return ret;
}

