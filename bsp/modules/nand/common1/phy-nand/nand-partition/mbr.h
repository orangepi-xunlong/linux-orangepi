/*
 *================================================================================================
 *
 *                                            Aone project ---- tools
 *
 *                             Copyright(C), 2006-2008, Microelectronic Co., Ltd.
 *											       All Rights Reserved
 *
 * File Name :  MBR.h
 *
 * Author : javen
 *
 * Version : 1.0
 *
 * Date : 2008.12.02
 *
 * Description :
 *
 * History :
 *================================================================================================
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
} NAND_PARTITION; //36bytes

/* mbr info */
typedef struct _PARTITION_MBR {
	unsigned int CRC;
	unsigned int PartCount;
	NAND_PARTITION array[ND_MAX_PARTITION_COUNT]; //
} PARTITION_MBR;

#define FTL_PARTITION_TYPE 0x8000
#define FTL_CROSS_TALK 0x4000

#endif //__MBR_H__
