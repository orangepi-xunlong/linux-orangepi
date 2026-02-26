/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __BUILD_PHY_PARTITION_V2_H__
#define __BUILD_PHY_PARTITION_V2_H__

#include "../nand-partition/phy.h"

int get_max_free_block_num(struct _nand_info *nand_info, struct _nand_phy_partition *phy_partition);
struct _nand_phy_partition *mp_build_phy_partition(struct _nand_info *nand_info, struct _partition *part);
struct _nand_phy_partition *launch_build_phy_partition(struct _nand_info *nand_info, struct _partition *part);
struct _nand_phy_partition *build_phy_partition_v2(struct _nand_info *nand_info, struct _partition *part);

void print_partition(struct _partition *partition);
#endif /*BUILD_PHY_PARTITION_H*/
