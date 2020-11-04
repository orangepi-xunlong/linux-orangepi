
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

#ifndef __ND_MBR_H__
#define __ND_MBR_H__

#define MAX_PART_COUNT_PER_FTL 24
#define MAX_PARTITION 4
#define PARTITION_NAME_SIZE 16

#define ND_MAX_PARTITION_COUNT (MAX_PART_COUNT_PER_FTL * MAX_PARTITION)

struct _NAND_CRC32_DATA {
	unsigned int CRC;
	unsigned int CRC_32_Tbl[256];
};

/* part info */
typedef struct _NAND_PARTITION {
	unsigned char classname[PARTITION_NAME_SIZE];
	unsigned int addr;
	unsigned int len;
	unsigned int user_type;
	unsigned int keydata;
	unsigned int ro;
} NAND_PARTITION; // 36bytes

/* mbr info */
typedef struct _PARTITION_MBR {
	unsigned int CRC;
	unsigned int PartCount;
	NAND_PARTITION array[ND_MAX_PARTITION_COUNT]; //
} PARTITION_MBR;

#define FTL_PARTITION_TYPE 0x8000
#define FTL_CROSS_TALK 0x4000

#endif //__MBR_H__
