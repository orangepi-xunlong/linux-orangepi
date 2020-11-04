
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef SPINAND_PHYSIC_INTERFACE_H
#define SPINAND_PHYSIC_INTERFACE_H

// struct physic_nand_info{
//	unsigned short                  type;
//    unsigned short                  SectorNumsPerPage;
//    unsigned short                  BytesUserData;
//    unsigned short                  PageNumsPerBlk;
//	unsigned short                  BlkPerChip;
//    unsigned short                  ChipNum;
//	__u64                           FullBitmap;
//};
#include "../nand_osal.h"

typedef struct {
	__u8 ChipCnt; // the count of the total nand flash chips are currently
		      // connecting on the CE pin
	__u8 ConnectMode;    // the rb connect  mode
	__u8 BankCntPerChip; // the count of the banks in one nand chip,
			     // multiple banks can support Inter-Leave
	__u8 DieCntPerChip; // the count of the dies in one nand chip, block
			    // management is based on Die
	__u8 PlaneCntPerDie; // the count of planes in one die, multiple planes
			     // can support multi-plane operation
	__u8 SectorCntPerPage; // the count of sectors in one single physic
			       // page, one sector is 0.5k
	__u16 ChipConnectInfo; // chip connect information, bit == 1 means there
			       // is a chip connecting on the CE pin
	__u32 PageCntPerPhyBlk; // the count of physic pages in one physic block
	__u32 BlkCntPerDie;     // the count of the physic blocks in one die,
			    // include valid block and invalid block
	__u32 OperationOpt; // the mask of the operation types which current
			    // nand flash can support support
	__u32 FrequencePar; // the parameter of the hardware access clock, based
			    // on 'MHz'
	__u32 SpiMode;      // spi nand mode, 0:mode 0, 3:mode 3
	__u8 NandChipId[8]; // the nand chip id of current connecting nand chip
	__u32 pagewithbadflag; // bad block flag was written at the first byte
			       // of spare area of this page
	__u32 MultiPlaneBlockOffset; // the value of the block number offset
				     // between the two plane block
	__u32 MaxEraseTimes; // the max erase times of a physic block
	__u32 MaxEccBits;    // the max ecc bits that nand support
	__u32 EccLimitBits;  // the ecc limit flag for tne nand
	__u32 uboot_start_block;
	__u32 uboot_next_block;
	__u32 logic_start_block;
	__u32 nand_specialinfo_page;
	__u32 nand_specialinfo_offset;
	__u32 physic_block_reserved;
	__u32 Reserved[4];
} boot_spinand_para_t;

typedef struct boot_flash_info {
	__u32 chip_cnt;
	__u32 blk_cnt_per_chip;
	__u32 blocksize;
	__u32 pagesize;
	__u32 pagewithbadflag; //bad block flag was written at the first byte of
			       //spare area of this page
} boot_flash_info_t;

struct boot_physical_param {
	__u32 chip;  // chip no
	__u32 block; // block no within chip
	__u32 page;  // apge no within block
	__u32 sectorbitmap;
	void *mainbuf; // data buf
	void *oobbuf;  // oob buf
};

// for simple nand operation (boot0)
extern __s32 PHY_SimpleErase(struct boot_physical_param *eraseop);
extern __s32 PHY_SimpleRead(struct boot_physical_param *readop);
extern __s32 PHY_SimpleWrite(struct boot_physical_param *writeop);

// for param get&set
extern __u32 NAND_GetFrequencePar(void);
extern __s32 NAND_SetFrequencePar(__u32 FrequencePar);
extern __u32 NAND_GetNandVersion(void);
// extern __s32 NAND_GetParam(boot_spinand_para_t * nand_param);
extern __s32 NAND_GetFlashInfo(boot_flash_info_t *info);
extern __u32 NAND_GetChipConnect(void);

// general operation based no nand super block
struct _nand_info *SpiNandHwInit(void);
// void NandHwInit(void);
__s32 SpiNandHwExit(void);
__s32 SpiNandHwSuperStandby(void);
__s32 SpiNandHwSuperResume(void);
__s32 SpiNandHwNormalStandby(void);
__s32 SpiNandHwNormalResume(void);
__s32 SpiNandHwShutDown(void);
__s32 NandHw_DbgInfo(__u32 type);

__s32 spinand_super_block_read(__u32 nDieNum, __u32 nBlkNum, __u32 nPage,
			       __u64 SectBitmap, void *pBuf, void *pSpare);
__s32 spinand_super_block_write(__u32 nDieNum, __u32 nBlkNum, __u32 nPage,
				__u64 SectBitmap, void *pBuf, void *pSpare);
__s32 spinand_super_block_erase(__u32 nDieNum, __u32 nBlkNum);
__s32 spinand_super_badblock_check(__u32 nDieNum, __u32 nBlkNum);
__s32 spinand_super_badblock_mark(__u32 nDieNum, __u32 nBlkNum);

int spinand_physic_erase_block(unsigned int chip, unsigned int block);
int spinand_physic_read_page(unsigned int chip, unsigned int block,
			     unsigned int page, unsigned int bitmap,
			     unsigned char *mbuf, unsigned char *sbuf);
int spinand_physic_write_page(unsigned int chip, unsigned int block,
			      unsigned int page, unsigned int bitmap,
			      unsigned char *mbuf, unsigned char *sbuf);
int spinand_physic_bad_block_check(unsigned int chip, unsigned int block);
int spinand_physic_bad_block_mark(unsigned int chip, unsigned int block);
int spinand_physic_block_copy(unsigned int chip_s, unsigned int block_s,
			      unsigned int chip_d, unsigned int block_d);

#endif /*SPINAND_PHYSIC_INTERFACE_H*/
