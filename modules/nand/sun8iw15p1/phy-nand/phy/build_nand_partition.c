/*
 * build_nand_partition.c
 * (C) Copyright 2007-2011
 * Allwinnertech Technology Co., Ltd. <www.allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#define _BUILD_PARTITION_C_
/*****************************************************************************/

#include "../nand_osal.h"
#include "phy.h"
/*****************************************************************************/


extern int is_factory_bad_block(struct _nand_info *nand_info,
			 unsigned short nDieNum, unsigned short nBlkNum);
extern int is_new_bad_block(struct _nand_info *nand_info,
		     unsigned short nDieNum, unsigned short nBlkNum);
extern int add_new_bad_block(struct _nand_info *nand_info,
		      unsigned short nDieNum, unsigned short nBlkNum);
extern unsigned int PHY_VirtualBadBlockMark(unsigned int nDieNum, unsigned int nBlkNum);
extern int free_phy_partition(struct _nand_phy_partition *phy_partition);
extern void WaitAllRb(void);
extern int free_nand_partition(struct _nand_partition *nand_partition);

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
void change_block_addr(struct _nand_partition *nand,
		       struct _nand_super_block *super_block,
		       unsigned short nBlkNum)
{
	super_block->Chip_NO = nand->phy_partition->StartBlock.Chip_NO;

	super_block->Block_NO =
	    nand->phy_partition->StartBlock.Block_NO + nBlkNum;

	while (super_block->Block_NO >=
	       nand->phy_partition->nand_info->BlkPerChip) {
		super_block->Chip_NO++;
		super_block->Block_NO -=
		    nand->phy_partition->nand_info->BlkPerChip;
	}
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_erase_superblk(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = nand->phy_partition->block_erase(super_block.Chip_NO,
					       super_block.Block_NO);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_read_page(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = nand->phy_partition->page_read(
	    super_block.Chip_NO, super_block.Block_NO, p->phy_page.Page_NO,
	    p->page_bitmap, p->main_data_addr, p->spare_data_addr);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_write_page(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	if (nand->phy_partition->CrossTalk != 0) {
		// wait RB
		WaitAllRb();
	}

	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = nand->phy_partition->page_write(
	    super_block.Chip_NO, super_block.Block_NO, p->phy_page.Page_NO,
	    p->page_bitmap, p->main_data_addr, p->spare_data_addr);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_is_blk_good(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;

	ret = 0;
	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	ret = is_factory_bad_block(nand->phy_partition->nand_info,
				   super_block.Chip_NO, super_block.Block_NO);
	if (ret != 0)
		return 0;
	ret = is_new_bad_block(nand->phy_partition->nand_info,
			       super_block.Chip_NO, super_block.Block_NO);
	if (ret != 0)
		return 0;

	return 1;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int nand_mark_bad_blk(struct _nand_partition *nand, struct _physic_par *p)
{
	int ret;
	struct _nand_super_block super_block;
	unsigned char spare[BYTES_OF_USER_PER_PAGE];

	MEMSET(spare, 0, BYTES_OF_USER_PER_PAGE);
	change_block_addr(nand, &super_block, p->phy_page.Block_NO);

	//	ret =
	// nand->phy_partition->page_write(super_block.Chip_NO,
	// super_block.Block_NO,0,nand->full_bitmap,
	// nand->phy_partition->new_bad_block,spare);
	//	if(ret != 0)
	//	{
	//		PHY_ERR("[NE]nand_mark_bad_blk error!\n");
	//	}
	NAND_PhysicLock();
	ret =
	    PHY_VirtualBadBlockMark(super_block.Chip_NO, super_block.Block_NO);
	NAND_PhysicUnLock();

	add_new_bad_block(nand->phy_partition->nand_info, super_block.Chip_NO,
			  super_block.Block_NO);

	return ret;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
struct _nand_partition *
build_nand_partition(struct _nand_phy_partition *phy_partition)
{
	struct _nand_partition *partition;

	partition = MALLOC(sizeof(struct _nand_partition));
	if (partition == NULL) {
		PHY_ERR("[NE]%s:malloc fail for partition\n", __func__);
		return NULL;
	}
	partition->phy_partition = phy_partition;
	partition->sectors_per_page = phy_partition->SectorNumsPerPage;
	partition->spare_bytes = phy_partition->BytesUserData;
	partition->pages_per_block = phy_partition->PageNumsPerBlk;
	partition->bytes_per_page = partition->sectors_per_page;
	partition->bytes_per_page <<= 9;
	partition->bytes_per_block =
	    partition->bytes_per_page * partition->pages_per_block;
	partition->full_bitmap = phy_partition->FullBitmapPerPage;

	partition->cap_by_sectors = phy_partition->TotalSectors;
	partition->cap_by_bytes = partition->cap_by_sectors << 9;
	partition->total_blocks =
	    phy_partition->TotalBlkNum - phy_partition->FreeBlock;

	partition->total_by_bytes = partition->total_blocks;
	partition->total_by_bytes *= partition->bytes_per_block;

	partition->nand_erase_superblk = nand_erase_superblk;
	partition->nand_read_page = nand_read_page;
	partition->nand_write_page = nand_write_page;
	partition->nand_is_blk_good = nand_is_blk_good;
	partition->nand_mark_bad_blk = nand_mark_bad_blk;

	//	snprintf(partition->name, 31,"nand_partition%d", phy_no);

	MEMCPY(partition->name, "nand_partition0", 31);
	partition->name[14] += phy_partition->PartitionNO;
	PHY_DBG("[ND]%s\n", partition->name);

	return partition;
}

/*****************************************************************************
 *Name         :
 *Description  :
 *Parameter    :
 *Return       :
 *Note         :
*****************************************************************************/
int free_nand_partition(struct _nand_partition *nand_partition)
{
	free_phy_partition(nand_partition->phy_partition);
	FREE((void *)(nand_partition), 0);
	return 0;
}
