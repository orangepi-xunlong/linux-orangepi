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
#ifndef __SUNXI_NAND_PARTITIONS_H_
#define __SUNXI_NAND_PARTITIONS_H_

#include "sunxi_nand.h"
#include "../rawnand/rawnand.h"
#include "../rawnand/rawnand_cfg.h"

#define MAX_PART_COUNT_PER_FTL 24
#define MAX_PARTITION 4
#define PARTITION_NAME_SIZE 16

#define ND_MAX_PARTITION_COUNT (MAX_PART_COUNT_PER_FTL * MAX_PARTITION)

#define FACTORY_BAD_BLOCK_SIZE 2048
#define PHY_PARTITION_BAD_BLOCK_SIZE 4096
#define NORM_RESERVED_BLOCK_RATIO 10

/* current logical write block */
extern unsigned short cur_w_lb_no;
/* current physical write block */
extern unsigned short cur_w_pb_no;
/* current physical write page */
extern unsigned short cur_w_p_no;
/* current logical erase block */
extern unsigned short cur_e_lb_no;

#define NAND_PARTS_COUNT_INCONSISTENT (-1)
#define NAND_PARTS_SIZE_INCONSISTENT (-2)

struct nand_part {
	unsigned char classname[PARTITION_NAME_SIZE];
	unsigned int user_type;
	unsigned int from;
	unsigned int size;
};

struct nand_partitions {
	int nc;
	unsigned int size;
	unsigned int cross_talk;
	unsigned int attribute;
	struct _nand_super_block start;
	struct _nand_super_block end;
	int resv[512 - 8];
#define SUNXI_DISK_MAX_PARTS (256)
	struct nand_part parts[SUNXI_DISK_MAX_PARTS];
};
extern struct nand_partitions nand_parts;
/* part info */
typedef struct _NAND_PARTITION {
	unsigned char classname[PARTITION_NAME_SIZE];
	unsigned int addr;
	unsigned int len;
	unsigned int user_type;
	unsigned int keydata;
	unsigned int ro;
} NAND_PARTITION; //36bytes

typedef struct _PARTITION_MBR {
	unsigned int CRC;
	unsigned int PartCount;
	NAND_PARTITION array[ND_MAX_PARTITION_COUNT]; //
} PARTITION_MBR;

typedef union {
	unsigned char ndata[4096];
	PARTITION_MBR data;
} _MBR;

struct _nand_disk {
	unsigned int size;
	//unsigned int offset;
	unsigned int type;
	unsigned char name[PARTITION_NAME_SIZE];
};

struct _partition {
	struct _nand_disk nand_disk[MAX_PART_COUNT_PER_FTL];
	unsigned int size;
	unsigned int cross_talk;
	unsigned int attribute;
	struct _nand_super_block start;
	struct _nand_super_block end;
	//unsigned int offset;
};
typedef union {
	unsigned char ndata[2048 + 512];
	struct _partition data[MAX_PARTITION];
} _PARTITION;

struct _spinand_config_para_info {
	unsigned int super_chip_cnt;
	unsigned int super_block_nums;
	unsigned int support_two_plane;
	unsigned int support_v_interleave;
	unsigned int support_dual_channel;
	unsigned int plane_cnt;
	unsigned int support_dual_read;
	unsigned int support_dual_write;
	unsigned int support_quad_read;
	unsigned int support_quad_write;
	unsigned int frequence;
};

typedef union {
	unsigned char ndata[512];
	struct _nand_super_storage_info data;
	struct _spinand_config_para_info config;
} _NAND_STORAGE_INFO;

typedef union {
	unsigned char ndata[2048];
	struct _nand_super_block data[512];
} _FACTORY_BLOCK;

typedef union {
	unsigned char ndata[1024];
	unsigned char data[1024];
} _NAND_SPECIAL_INFO;
/*
 *struct _nand_partition {
 *        char name[32];
 *        unsigned short sectors_per_page;
 *        unsigned short spare_bytes;
 *        unsigned short pages_per_block;
 *        unsigned short total_blocks;
 *        unsigned short bytes_per_page;
 *        unsigned int bytes_per_block;
 *        unsigned short full_bitmap;
 *        unsigned long long cap_by_sectors;
 *        unsigned long long cap_by_bytes;
 *        unsigned long long total_by_bytes;
 *        struct _nand_phy_partition *phy_partition;
 *
 *        int (*nand_erase_superblk)(struct _nand_partition *nand, struct _physic_par *p);
 *        int (*nand_read_page)(struct _nand_partition *nand, struct _physic_par *p);
 *        int (*nand_write_page)(struct _nand_partition *nand, struct _physic_par *p);
 *        int (*nand_is_blk_good)(struct _nand_partition *nand, struct _physic_par *p);
 *        int (*nand_mark_bad_blk)(struct _nand_partition *nand, struct _physic_par *p);
 *};
 */
//nand phy partition 访问接口
struct nand_phy_partition {
	unsigned short PartitionNO;
	unsigned short CrossTalk;
	unsigned short SectorNumsPerPage;
	unsigned short BytesUserData;
	unsigned short PageNumsPerBlk;
	unsigned short TotalBlkNum; //include bad block
	unsigned short GoodBlockNum;
	unsigned short FullBitmapPerPage;
	unsigned short FreeBlock;
	unsigned int Attribute;
	unsigned int TotalSectors;
	struct _nand_super_block StartBlock;
	struct _nand_super_block EndBlock;
	struct _nand_info *nand_info;
	struct _nand_super_block *factory_bad_block;
	struct _nand_super_block *new_bad_block;
	struct nand_phy_partition *next_phy_partition;
	struct _nand_disk *disk;

	int (*page_read)(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare);
	int (*page_write)(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare);
	int (*block_erase)(unsigned short nDieNum, unsigned short nBlkNum);
};

struct _nand_phy_partition {
	unsigned short PartitionNO;
	unsigned short CrossTalk;
	unsigned short SectorNumsPerPage;
	unsigned short BytesUserData;
	unsigned short PageNumsPerBlk;
	unsigned short TotalBlkNum; //include bad block
	unsigned short GoodBlockNum;
	unsigned short FullBitmapPerPage;
	unsigned short FreeBlock;
	unsigned int Attribute;
	unsigned int TotalSectors;
	struct _nand_super_block StartBlock;
	struct _nand_super_block EndBlock;
	struct _nand_info *nand_info;
	struct _nand_super_block *factory_bad_block;
	struct _nand_super_block *new_bad_block;
	struct _nand_phy_partition *next_phy_partition;
	struct _nand_disk *disk;

	int (*page_read)(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare);
	int (*page_write)(unsigned short nDieNum, unsigned short nBlkNum, unsigned short nPage, unsigned short SectBitmap, void *pBuf, void *pSpare);
	int (*block_erase)(unsigned short nDieNum, unsigned short nBlkNum);
};

struct _nand_partition {
	char name[32];
	unsigned short sectors_per_page;
	unsigned short spare_bytes;
	unsigned short pages_per_block;
	unsigned short total_blocks;
	unsigned short bytes_per_page;
	unsigned int bytes_per_block;
	unsigned short full_bitmap;
	unsigned long long cap_by_sectors;
	unsigned long long cap_by_bytes;
	unsigned long long total_by_bytes;
	struct _nand_phy_partition *phy_partition;

	int (*nand_erase_superblk)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_read_page)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_write_page)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_is_blk_good)(struct _nand_partition *nand,
			struct _physic_par *p);
	int (*nand_mark_bad_blk)(struct _nand_partition *nand,
			struct _physic_par *p);
};

struct nand_cfg;
//全局flash属性
/*
 *struct _nand_info {
 *        unsigned short type;
 *        unsigned short SectorNumsPerPage;
 *        unsigned short BytesUserData;
 *        unsigned short PageNumsPerBlk;
 *        unsigned short BlkPerChip;
 *        unsigned short ChipNum;
 *        unsigned short FirstBuild;
 *        unsigned short new_bad_page_addr;
 *        unsigned long long FullBitmap;
 *        struct _nand_super_block mbr_block_addr;
 *        struct _nand_super_block bad_block_addr;
 *        struct _nand_super_block new_bad_block_addr;
 *        struct _nand_super_block no_used_block_addr;
 *        struct _nand_super_block *factory_bad_block;
 *        struct _nand_super_block *new_bad_block;
 *        unsigned char *temp_page_buf;
 *        unsigned char *mbr_data;
 *        struct nand_phy_partition *phy_partition_head;
 *        struct _partition partition[MAX_PARTITION];
 *        struct nand_cfg nand_cfg;
 *        unsigned short partition_nums;
 *        unsigned short cache_level;
 *        unsigned short capacity_level;
 *
 *        unsigned short mini_free_block_first_reserved;
 *        unsigned short mini_free_block_reserved;
 *
 *        unsigned int MaxBlkEraseTimes;
 *        unsigned int EnableReadReclaim;
 *
 *        unsigned int read_claim_interval;
 *
 *        struct _boot_info *boot;
 *};
 */

struct _nand_info {
#define SLC_NAND (0)
#define MLC_NAND (1)
#define UNKOWN_NAND (2)
	unsigned short type;
	unsigned short SectorNumsPerPage;
	unsigned short BytesUserData;
	unsigned short PageNumsPerBlk;
	unsigned short BlkPerChip;
	unsigned short ChipNum;
	unsigned short FirstBuild;
	unsigned short new_bad_page_addr;
	unsigned long long FullBitmap;
	struct _nand_super_block mbr_block_addr;
	struct _nand_super_block bad_block_addr;
	struct _nand_super_block new_bad_block_addr;
	struct _nand_super_block no_used_block_addr;
	//struct _bad_block_list*         new_bad_block_addr;
	struct _nand_super_block *factory_bad_block;
	struct _nand_super_block *new_bad_block;
	unsigned char *temp_page_buf;
	unsigned char *mbr_data;
	struct _nand_phy_partition *phy_partition_head;
	struct _partition partition[MAX_PARTITION];
	struct nand_cfg nand_cfg;
	unsigned short partition_nums;
	unsigned short cache_level;
	unsigned short capacity_level;

	unsigned short mini_free_block_first_reserved;
	unsigned short mini_free_block_reserved;

	unsigned int MaxBlkEraseTimes;
	unsigned int EnableReadReclaim;

	unsigned int read_claim_interval;

#define BOOTINFO_NULL (0)
#define BOOTINFO_OLD (1)
#define BOOTINFO_NEW (2)
	unsigned int bootinfo_is_old;
	struct _boot_info *boot;
};

#endif /*__SUNXI_NAND_PARTITIONS_H_*/
