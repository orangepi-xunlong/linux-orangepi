/*
 *file : nand_phy.c
 *description : this file creates some physic optimize access function for
 *system .
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

#include "nand_physic.h"
#include "nand_physic_interface_spinand.h"
#include "nand_type_spinand.h"
#include "spic.h"

struct __NandStorageInfo_t NandStorageInfo = {0};
struct __NandPageCachePool_t PageCachePool = {0};

__u32 _cal_addr_in_chip(__u32 block, __u32 page)
{
	return block * PAGE_CNT_OF_PHY_BLK + page;
}

__u8 _cal_real_chip(__u32 global_bank)
{
	__u8 chip = 0;

	if ((CONNECT_MODE == 1) && (global_bank <= 1)) {
		chip = global_bank;
		return chip;
	}
	if ((CONNECT_MODE == 2) && (global_bank <= 2)) {
		chip = global_bank;
		return chip;
	}
	if ((CONNECT_MODE == 3) && (global_bank <= 2)) {
		chip = global_bank * 2;
		return chip;
	}
	if ((CONNECT_MODE == 4) && (global_bank <= 2)) {
		chip = global_bank * 3;
		return chip;
	}
	if ((CONNECT_MODE == 5) && (global_bank <= 4)) {
		switch (global_bank) {
		case 0:
			chip = 0;
			break;
		case 1:
			chip = 1;
			break;
		case 2:
			chip = 2;
			break;
		case 3:
			chip = 3;
			break;
		default:
			chip = 0;
			break;
		}
		return chip;
	}

	PHY_ERR("wrong chip number ,connect_mode = %d, bank = %d, chip = %d, "
		"chip info = %x\n",
		CONNECT_MODE, global_bank, chip, CHIP_CONNECT_INFO);

	return 0xff;
}

__u32 _cal_first_valid_bit(__u32 secbitmap)
{
	__u32 firstbit = 0;
	__u8 i = 0;

	// while(!(secbitmap & 0x1))
	while (1) {
		if (secbitmap & (0x1 << i)) {
			firstbit = i;
			break;
		}
		i++;
		if (i == 32)
			break;
	}

	return firstbit;
}

__u32 _cal_valid_bits(__u32 secbitmap)
{
	__u32 validbit = 0;

	while (secbitmap) {
		if (secbitmap & 0x1)
			validbit++;
		secbitmap >>= 1;
	}

	return validbit;
}

__u32 _check_continuous_bits(__u32 secbitmap)
{
	__u32 ret = 1; // 1: bitmap is continuous, 0: bitmap is not continuous
	__u32 first_valid_bit = 0;
	__u32 flag = 0;

	while (secbitmap) {
		if (secbitmap & 0x1) {
			if (first_valid_bit == 0)
				first_valid_bit = 1;

			if (first_valid_bit && flag) {
				ret = 0;
				break;
			}
		} else {
			if (first_valid_bit == 1)
				flag = 1;
		}
		secbitmap >>= 1;
	}

	return ret;
}

__u32 _cal_block_in_chip(__u32 global_bank, __u32 super_blk_within_bank)
{
	__u32 blk_within_chip;
	__u32 single_blk_within_bank;
	__u32 bank_base;

	/*translate block 0 within  bank into blcok number within chip*/
	bank_base = global_bank % BNK_CNT_OF_CHIP * BLOCK_CNT_OF_DIE;

	if (SUPPORT_MULTI_PROGRAM) {
		single_blk_within_bank =
		    super_blk_within_bank * PLANE_CNT_OF_DIE -
		    super_blk_within_bank % MULTI_PLANE_BLOCK_OFFSET;
	}

	else {
		single_blk_within_bank = super_blk_within_bank;
	}

	blk_within_chip = bank_base + single_blk_within_bank;

	if (blk_within_chip >= DIE_CNT_OF_CHIP * BLOCK_CNT_OF_DIE)
		blk_within_chip = 0xffffffff;

	return blk_within_chip;
}

__s32 PHY_Init(void)
{

	Spic_init(0);

	// spic_dma_onoff(0, 1);

	return 0;
}

__s32 PHY_Exit(void)
{

	if (PageCachePool.PageCache0) {
		FREE(PageCachePool.PageCache0, 512 * SECTOR_CNT_OF_SINGLE_PAGE);
		PageCachePool.PageCache0 = NULL;
	}
#if 0
	if (PageCachePool.PageCache1) {
		FREE(PageCachePool.PageCache0, 512 * SECTOR_CNT_OF_SINGLE_PAGE);
		PageCachePool.PageCache0 = NULL;
	}
	if (PageCachePool.PageCache2) {
		FREE(PageCachePool.PageCache0, 512 * SECTOR_CNT_OF_SINGLE_PAGE);
		PageCachePool.PageCache0 = NULL;
	}
#endif
	if (PageCachePool.SpiPageCache) {
		FREE(PageCachePool.SpiPageCache,
		     512 * SECTOR_CNT_OF_SINGLE_PAGE + 32);
		PageCachePool.SpiPageCache = NULL;
	}
	if (PageCachePool.SpareCache) {
		FREE(PageCachePool.SpareCache, 512);
		PageCachePool.SpareCache = NULL;
	}
	if (PageCachePool.TmpPageCache) {
		FREE(PageCachePool.TmpPageCache,
		     512 * SECTOR_CNT_OF_SINGLE_PAGE);
		PageCachePool.TmpPageCache = NULL;
	}

	Spic_exit(0);

	// spic_dma_onoff(0, 0);

	return 0;
}

__s32 PHY_ResetChip(__u32 nChip)
{
	return NandStorageInfo.spi_nand_function->spi_nand_reset(0, nChip);
}

__s32 PHY_ReadNandId_0(__s32 nChip, void *pChipID)
{
	__s32 ret = 0;

	ret = spi_nand_rdid(0, nChip, 0x00, 1, 8, pChipID);

	return ret;
}

__s32 PHY_ReadNandId_1(__s32 nChip, void *pChipID)
{
	__s32 ret = 0;

	ret = spi_nand_rdid(0, nChip, 0x00, 0, 8, pChipID);

	return ret;
}

__s32 PHY_ChangeMode(void)
{

	MEMSET(&PageCachePool, 0, sizeof(struct __NandPageCachePool_t));

	if (!PageCachePool.PageCache0) {
		PageCachePool.PageCache0 =
		    (__u8 *)MALLOC(512 * SECTOR_CNT_OF_SINGLE_PAGE);
		if (!PageCachePool.PageCache0) {
			PHY_ERR("PageCache0 malloc fail\n");
			return -1;
		}
	}
#if 0
	if (!PageCachePool.PageCache1) {
		PageCachePool.PageCache1 = (__u8 *)MALLOC(512 * SECTOR_CNT_OF_SINGLE_PAGE);
		if (!PageCachePool.PageCache1) {
			PHY_ERR("PageCache1 malloc fail\n");
			return -1;
		}
	}

	if (!PageCachePool.PageCache2) {
		PageCachePool.PageCache2 = (__u8 *)MALLOC(512 * SECTOR_CNT_OF_SINGLE_PAGE);
		if (!PageCachePool.PageCache2) {
			PHY_ERR("PageCache2 malloc fail\n");
			return -1;
		}
	}
#endif
	if (!PageCachePool.SpiPageCache) {
		PageCachePool.SpiPageCache =
		    (__u8 *)MALLOC(512 * SECTOR_CNT_OF_SINGLE_PAGE + 32);
		if (!PageCachePool.SpiPageCache) {
			PHY_ERR("SpiPageCache malloc fail\n");
			return -1;
		}
	}
	if (!PageCachePool.SpareCache) {
		PageCachePool.SpareCache = (__u8 *)MALLOC(512);
		if (!PageCachePool.SpareCache) {
			PHY_ERR("SpareCache malloc fail\n");
			return -1;
		}
	}
	if (!PageCachePool.TmpPageCache) {
		PageCachePool.TmpPageCache =
		    (__u8 *)MALLOC(512 * SECTOR_CNT_OF_SINGLE_PAGE);
		if (!PageCachePool.TmpPageCache) {
			PHY_ERR("TmpPageCache malloc fail\n");
			return -1;
		}
	}

	return 0;
}

__s32 PHY_SimpleRead(struct boot_physical_param *readop)
{
	__s32 ret = 0;

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//readop->chip);//clear status register

	ret = NandStorageInfo.spi_nand_function->read_single_page(readop, 0);

	if (ret == ERR_TIMEOUT)
		PHY_ERR(
		    "PHY_SimpleRead : chip %d block %d page %d read timeout\n",
		    readop->chip, readop->block, readop->page);

	if (ret == ERR_ECC)
		PHY_ERR("PHY_SimpleRead: chip %d blk %d pg %d ecc err\n",
			readop->chip, readop->block, readop->page);

	if (ret == ECC_LIMIT) {
		PHY_DBG("PHY_SimpleRead: chip %d blk %d pg %d ecc correct\n",
			readop->chip, readop->block, readop->page);
	}

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//readop->chip);//clear status register

	return ret;
}

__s32 PHY_SimpleWrite(struct boot_physical_param *writeop)
{
	__s32 ret = 0;
	//	__u8 status;

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//writeop->chip);//clear status register

	ret = NandStorageInfo.spi_nand_function->write_single_page(writeop);
	//	ret = NandStorageInfo.spi_nand_function->spi_nand_read_status(0,
	//writeop->chip, (__u8)status, 1);

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//writeop->chip);//clear status register

	if (ret == ERR_TIMEOUT)
		PHY_ERR("PHY_SimpleWrite : chip %d block %d page %d write "
			"timeout\n",
			writeop->chip, writeop->block, writeop->page);

	if (ret == NAND_OP_FALSE)
		PHY_ERR(
		    "PHY_SimpleWrite : erase fail,chip %d block %d page %d\n",
		    writeop->chip, writeop->block, writeop->page);

	return ret;
}

__s32 PHY_SimpleErase(struct boot_physical_param *eraseop)
{
	__s32 ret = 0;
	//	__u8 status;

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//eraseop->chip);//clear status register

	ret = NandStorageInfo.spi_nand_function->erase_single_block(eraseop);
	// ret = NandStorageInfo.spi_nand_function->spi_nand_read_status(0,
	// eraseop->chip, (__u8)status, 1);

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//eraseop->chip);//clear status register

	if (ret == ERR_TIMEOUT)
		PHY_ERR("PHY_SimpleErase : chip %d block %d erase timeout\n",
			eraseop->chip, eraseop->block);

	if (ret == NAND_OP_FALSE)
		PHY_ERR("PHY_SimpleErase : erase fail,chip %d block %d\n",
			eraseop->chip, eraseop->block);

	return ret;
}

__s32 PHY_BlockErase(struct __PhysicOpPara_t *pBlkAdr)
{
	__s32 ret = 0;
	__u32 block_in_chip, chip;
	__u32 plane_cnt;
	__u32 i;
	struct boot_physical_param eraseop;

	plane_cnt = SUPPORT_MULTI_PROGRAM ? PLANE_CNT_OF_DIE : 1;

	chip = _cal_real_chip(pBlkAdr->BankNum);
	if (0xff == chip) {
		PHY_ERR("PHY_BlockErase : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}

	block_in_chip = _cal_block_in_chip(pBlkAdr->BankNum, pBlkAdr->BlkNum);
	if (0xffffffff == block_in_chip) {
		PHY_ERR("PHY_BlockErase : beyond block of per chip  count\n");
		return -ERR_INVALIDPHYADDR;
	}

	for (i = 0; i < plane_cnt; i++) {
		eraseop.chip = chip;
		eraseop.block = block_in_chip + i * MULTI_PLANE_BLOCK_OFFSET;
		eraseop.page = pBlkAdr->PageNum;
		//	ret = spi_nand_block_erase(0,addr);
		ret |= NandStorageInfo.spi_nand_function->erase_single_block(
		    &eraseop);
	}

	if (ret == ERR_TIMEOUT)
		PHY_ERR("PHY_BlockErase : bank %d block %d erase timeout\n",
			pBlkAdr->BankNum, pBlkAdr->BlkNum);

	if (ret == NAND_OP_FALSE)
		PHY_ERR("PHY_BlockErase : erase fail,bank %d block %d\n",
			pBlkAdr->BankNum, pBlkAdr->BlkNum);
	//	m0_erase_single_block(&eraseop);
	return ret;
}

__s32 PHY_PageRead(struct __PhysicOpPara_t *pPageAdr)
{
	__s32 ret = 0;
	__u32 block_in_chip;
	__u32 chip;
	__u32 plane_cnt, i;
	__u64 bitmap_in_single_page;
	__u8 oob[2][16] = {0};
	struct boot_physical_param readop;

	plane_cnt = SUPPORT_MULTI_PROGRAM ? PLANE_CNT_OF_DIE : 1;

	chip = _cal_real_chip(pPageAdr->BankNum);
	if (0xff == chip) {
		PHY_ERR("PHY_PageRead : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}

	block_in_chip = _cal_block_in_chip(pPageAdr->BankNum, pPageAdr->BlkNum);
	if (0xffffffff == block_in_chip) {
		PHY_ERR("PHY_PageRead : beyond block of per chip  count\n");
		return -ERR_INVALIDPHYADDR;
	}

	for (i = 0; i < plane_cnt; i++) {
		readop.chip = chip;
		readop.block = block_in_chip + i * MULTI_PLANE_BLOCK_OFFSET;
		readop.page = pPageAdr->PageNum;
		// readop.oobbuf = (void *)(pPageAdr->SDataPtr);
		readop.oobbuf = (void *)oob[i];

		bitmap_in_single_page =
		    FULL_BITMAP_OF_SINGLE_PAGE &
		    (pPageAdr->SectBitmap >> (i * SECTOR_CNT_OF_SINGLE_PAGE));
		readop.sectorbitmap = bitmap_in_single_page;

		if (bitmap_in_single_page) {
			readop.mainbuf =
			    (void *)((__u8 *)pPageAdr->MDataPtr +
				     i * 512 * SECTOR_CNT_OF_SINGLE_PAGE);
			ret |=
			    NandStorageInfo.spi_nand_function->read_single_page(
				&readop, 0);
			//		m0_read_single_page(&readop);
		}
	}

	if ((oob[0][0] != 0xff) || (oob[1][0] != 0xff)) {
		oob[0][0] = 0x0;
		PHY_DBG("PHY_PageRead bad flag: bank 0x%x  block 0x%x  page "
			"0x%x\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);
	}
	MEMCPY((__u8 *)pPageAdr->SDataPtr, &oob[0][0], 16);

	if (ret == ERR_TIMEOUT)
		PHY_ERR("PHY_PageRead : bank %d blk %d pg %d read timeout\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);

	if (ret == ERR_ECC)
		PHY_ERR("PHY_PageRead: bank %d blk %d pg %d ecc err\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);

	if (ret == ECC_LIMIT) {
		PHY_DBG("PHY_PageRead: bank %d blk %d pg %d ecc correct\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);
	}

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//chip);//clear status register

	return ret;
}

__s32 PHY_PageReadSpare(struct __PhysicOpPara_t *pPageAdr)
{
	__s32 ret = 0;
	__u32 block_in_chip;
	__u32 chip, plane_cnt;
	__u32 i;
	__u64 bitmap_in_single_page;
	__u8 oob[2][16] = {0};
	struct boot_physical_param readop;

	plane_cnt = SUPPORT_MULTI_PROGRAM ? PLANE_CNT_OF_DIE : 1;

	chip = _cal_real_chip(pPageAdr->BankNum);
	if (0xff == chip) {
		PHY_ERR("PHY_PageRead : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}

	block_in_chip = _cal_block_in_chip(pPageAdr->BankNum, pPageAdr->BlkNum);
	if (0xffffffff == block_in_chip) {
		PHY_ERR("PHY_PageRead : beyond block of per chip  count\n");
		return -ERR_INVALIDPHYADDR;
	}

	for (i = 0; i < plane_cnt; i++) {
		readop.chip = chip;
		readop.block = block_in_chip + i * MULTI_PLANE_BLOCK_OFFSET;
		readop.page = pPageAdr->PageNum;
		readop.mainbuf = (void *)(pPageAdr->MDataPtr);
		readop.oobbuf = (void *)oob[i];

		bitmap_in_single_page =
		    FULL_BITMAP_OF_SINGLE_PAGE &
		    (pPageAdr->SectBitmap >> (i * SECTOR_CNT_OF_SINGLE_PAGE));
		readop.sectorbitmap = bitmap_in_single_page;

		ret = NandStorageInfo.spi_nand_function->read_single_page(
		    &readop, 1);
	}

	if ((oob[0][0] != 0xff) || (oob[1][0] != 0xff)) {
		oob[0][0] = 0x0;
		PHY_DBG("PHY_PageReadSpare bad flag: bank 0x%x  block 0x%x  "
			"page 0x%x\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);
	}
	MEMCPY((__u8 *)pPageAdr->SDataPtr, &oob[0][0], 16);

	if (ret == ERR_TIMEOUT)
		PHY_ERR(
		    "PHY_PageReadSpare : bank %d blk %d pg %d read timeout\n",
		    (__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
		    (__u32)pPageAdr->PageNum);

	if (ret == ERR_ECC)
		PHY_ERR("PHY_PageReadSpare: bank %d blk %d pg %d ecc err\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);

	if (ret == ECC_LIMIT) {
		PHY_DBG("PHY_PageReadSpare: bank %d blk %d pg %d ecc correct\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);
	}

	//	NandStorageInfo.spi_nand_function->spi_nand_reset(0,
	//chip);//clear status register

	return ret;
}

__s32 PHY_PageWrite(struct __PhysicOpPara_t *pPageAdr)
{
	__s32 ret = 0;
	__u32 block_in_chip;
	__u32 chip, plane_cnt;
	__u32 i;
	struct boot_physical_param writeop;

	plane_cnt = SUPPORT_MULTI_PROGRAM ? PLANE_CNT_OF_DIE : 1;

	chip = _cal_real_chip(pPageAdr->BankNum);
	if (0xff == chip) {
		PHY_ERR("PHY_PageWrite : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}

	block_in_chip = _cal_block_in_chip(pPageAdr->BankNum, pPageAdr->BlkNum);
	if (0xffffffff == block_in_chip) {
		PHY_ERR("PHY_PageWrite : beyond block of per chip  count\n");
		return -ERR_INVALIDPHYADDR;
	}

	for (i = 0; i < plane_cnt; i++) {
		writeop.chip = chip;
		writeop.block = block_in_chip + i * MULTI_PLANE_BLOCK_OFFSET;
		writeop.page = pPageAdr->PageNum;
		writeop.mainbuf = (void *)((__u8 *)pPageAdr->MDataPtr +
					   i * 512 * SECTOR_CNT_OF_SINGLE_PAGE);
		writeop.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;

		if (pPageAdr->SDataPtr)
			writeop.oobbuf = (void *)(pPageAdr->SDataPtr);
		else
			writeop.oobbuf = NULL;

		ret |= NandStorageInfo.spi_nand_function->write_single_page(
		    &writeop);
	}

	if (ret == ERR_TIMEOUT)
		PHY_ERR("PHY_PageWrite : bank %d blk %d pg %d write timeout\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);

	if (ret == NAND_OP_FALSE)
		PHY_ERR("PHY_PageWrite : bank %d blk %d pg %d write fail\n",
			(__u32)pPageAdr->BankNum, (__u32)pPageAdr->BlkNum,
			(__u32)pPageAdr->PageNum);
	//	m0_write_single_page(&writeop);

	return ret;
}

__s32 PHY_PageCopyback(struct __PhysicOpPara_t *pSrcPage,
		       struct __PhysicOpPara_t *pDstPage)
{
	__s32 ret = 0;
	__u32 chip;

	pSrcPage->SectBitmap = pDstPage->SectBitmap = FULL_BITMAP_OF_SUPER_PAGE;
	pSrcPage->MDataPtr = pDstPage->MDataPtr = PHY_TMP_PAGE_CACHE;
	pSrcPage->SDataPtr = pDstPage->SDataPtr = PHY_TMP_SPARE_CACHE;

	chip = _cal_real_chip(pSrcPage->BankNum);
	if (0xff == chip) {
		PHY_ERR("PHY_PageCopyback : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}
	PHY_ResetChip(chip); // clear status register

	ret = PHY_PageRead(pSrcPage);

	if (ret == -ERR_ECC)
		ret = 0;

	if (ret < 0)
		return ret;

	chip = _cal_real_chip(pDstPage->BankNum);
	if (0xff == chip) {
		PHY_ERR("PHY_PageCopyback : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}
	PHY_ResetChip(chip); // clear status register

	ret = PHY_PageWrite(pDstPage);

	PHY_ResetChip(chip); // clear status register

	return 0;
}

__s32 PHY_Wait_Status(__u32 nBank)
{
	__s32 ret = 0, status = 0;
	__u32 chip;

	ret = 0;
	/*get chip no*/
	chip = _cal_real_chip(nBank);

	if (0xff == chip) {
		PHY_ERR("PHY_SynchBank : beyond chip count\n");
		return -ERR_INVALIDPHYADDR;
	}

	ret = NandStorageInfo.spi_nand_function->spi_nand_read_status(
	    0, chip, (__u8)status, 1);

	NandStorageInfo.spi_nand_function->spi_nand_reset(
	    0, chip); // clear status register

	return ret;
}

__s32 PHY_Scan_DelayMode(__u32 clk)
{
	__u32 mode;
	__u32 mode_select[4] = {0};
	__u32 m, i, b, start_blk, blk_cnt;
	__u8 *rmain_buf = NULL;
	__u8 roob_buf[16];
	__s32 ret = 0;
	__u8 status = 0;
	//	__u8 err_flag = 0;
	__u32 clk_real, clk1_real;
	struct boot_physical_param readop;

	start_blk = 8;
	blk_cnt = 10;

	rmain_buf = (__u8 *)MALLOC(NandStorageInfo.SectorCntPerPage * 512);
	if (!rmain_buf) {
		PHY_ERR("PHY_Scan_DelayMode, rmain_buf 0x%x is null!\n",
			rmain_buf);
		return -1;
	}

	for (b = start_blk; b < start_blk + blk_cnt; b++) {
		NAND_SetClk(0, clk, clk * 2);
		for (m = 0; m < 4; m++) {
			//			err_flag = 0;
			if (m == 0) {
				Spic_set_sample(0, 0);
				Spic_set_sample_mode(0, 0);
				mode = 0;
			} else if (m == 1) {
				Spic_set_sample(0, 1);
				Spic_set_sample_mode(0, 0);
				mode = 0x1 << 11;
			} else if (m == 2) {
				Spic_set_sample(0, 0);
				Spic_set_sample_mode(0, 1);
				mode = 0x1 << 13;
			} else {
				Spic_set_sample(0, 1);
				Spic_set_sample_mode(0, 1);
				mode = 0x5 << 11;
			}

			readop.chip = 0;
			readop.block = b;
			readop.page = 0;
			readop.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
			readop.mainbuf = rmain_buf;
			readop.oobbuf = roob_buf;

			ret = PHY_SimpleRead(&readop);
			if (ret == 0) {
				if ((roob_buf[0] == 0xff) &&
				    (roob_buf[1] == 0xff) &&
				    (roob_buf[2] == 0xff) &&
				    (roob_buf[3] == 0xff)) {
					PHY_DBG("PHY_Scan_DelayMode, it is a "
						"free page(type 0), block %d\n",
						b);
					break;
				} else if ((roob_buf[0] == 0x0) &&
					   (roob_buf[1] == 0x0) &&
					   (roob_buf[2] == 0x0) &&
					   (roob_buf[3] == 0x0)) {
					PHY_DBG("PHY_Scan_DelayMode, bad block "
						"%d !\n",
						b);
					break;
				} else {
					PHY_DBG("PHY_Scan_DelayMode: right "
						"delay mode 0x%x\n",
						mode);
					mode_select[m] = 1;
				}
			} else {
#if 1
				if ((roob_buf[0] == 0xff) &&
				    (roob_buf[1] == 0xff) &&
				    (roob_buf[2] == 0xff) &&
				    (roob_buf[3] == 0xff)) {
					PHY_DBG("PHY_Scan_DelayMode, it is a "
						"free page(type 1), block %d\n",
						b);
					break;
				}
#endif
				PHY_DBG("PHY_Scan_DelayMode, read error, block "
					"%d !\n",
					b);
			}

			if (((mode_select[0] == 1) || (mode_select[1] == 1)) &&
			    (m == 1))
				break;
		}
		if ((mode_select[0] == 1) || (mode_select[1] == 1) ||
		    (mode_select[2] == 1) || (mode_select[3] == 1))
			break;
	}

	NAND_GetClk(0, &clk_real, &clk1_real);

	if ((mode_select[0] == 1) && (mode_select[1] == 1)) {
		Spic_set_sample_mode(0, 0);
		if (clk_real > 50) {
			Spic_set_sample(0, 1);
			PHY_DBG("PHY_Scan_DelayMode: right delay mode,clk %d "
				"MHz, bit[13]=0,bit[11]=1\n",
				clk_real);
		} else {
			Spic_set_sample(0, 0);
			PHY_DBG("PHY_Scan_DelayMode: right delay mode,clk %d "
				"MHz, bit[13]=0,bit[11]=0\n",
				clk_real);
		}
	} else if (mode_select[1] == 1) {
		Spic_set_sample(0, 1);
		Spic_set_sample_mode(0, 0);
		PHY_DBG("PHY_Scan_DelayMode: right delay mode,clk %d MHz, "
			"bit[13]=0,bit[11]=1\n",
			clk_real);
	} else if (mode_select[0] == 1) {
		Spic_set_sample(0, 0);
		Spic_set_sample_mode(0, 0);
		PHY_DBG("PHY_Scan_DelayMode: right delay mode,clk %d MHz, "
			"bit[13]=0,bit[11]=0\n",
			clk_real);
	} else if (mode_select[2] == 1) {
		Spic_set_sample(0, 0);
		Spic_set_sample_mode(0, 1);
		PHY_DBG("PHY_Scan_DelayMode: right delay mode,clk %d MHz, "
			"bit[13]=1,bit[11]=0\n",
			clk_real);
	} else if (mode_select[3] == 1) {
		Spic_set_sample(0, 1);
		Spic_set_sample_mode(0, 1);
		PHY_DBG("PHY_Scan_DelayMode: right delay mode,clk %d MHz, "
			"bit[13]=1,bit[11]=1\n",
			clk_real);
	} else {
		NAND_SetClk(0, 30, 30 * 2);
		Spic_set_sample(0, 0);
		Spic_set_sample_mode(0, 0);
		PHY_DBG("PHY_Scan_DelayMode: no right delay mode,set default "
			"clk 30MHz\n");
		//		return -1;
	}

	return 0;
}
