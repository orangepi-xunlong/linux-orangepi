/*SPDX-Licensen-Identifier: GPL-2.0*/

#ifndef __PHY2NFTL_H__
#define __PHY2NFTL_H__

#include "aw_nand_type.h"

typedef struct nand_phyinfo_for_nftl {
	__u32 die_cnt_per_chip;
	__u32 blk_cnt_per_die;
	__u16 page_cnt_per_blk;
	__u8 sector_cnt_per_page;
	__u8 nand_chip_id[8];
} nand_phyinfo_for_nftl_t;

void nftl_get_nand_phyinfo(nand_phyinfo_for_nftl_t *info);

#endif /*PHY2NFTL_H*/
