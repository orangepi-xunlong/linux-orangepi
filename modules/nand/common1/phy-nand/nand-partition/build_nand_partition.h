/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BUILD_NAND_PARTITION_H__
#define __BUILD_NAND_PARTITION_H__

#include "phy.h"

void change_block_addr(struct _nand_partition *nand, struct _nand_super_block *super_block, unsigned short nBlkNum);
int nand_erase_superblk(struct _nand_partition *nand, struct _physic_par *p);
int nand_read_page(struct _nand_partition *nand, struct _physic_par *p);
int nand_write_page(struct _nand_partition *nand, struct _physic_par *p);
int nand_is_blk_good(struct _nand_partition *nand, struct _physic_par *p);
int nand_mark_bad_blk(struct _nand_partition *nand, struct _physic_par *p);
struct _nand_partition *build_nand_partition(struct _nand_phy_partition *phy_partition);
int free_nand_partition(struct _nand_partition *nand_partition);
#endif /*BUILD_NAND_PARTITION_H*/
