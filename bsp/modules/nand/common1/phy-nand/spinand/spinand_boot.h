/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __UBOOT_TAIL_SPINAND_H__
#define __UBOOT_TAIL_SPINAND_H__

#include "../nand-partition/phy.h"

int spinand_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter);
int spinand_read_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter);
int spinand_write_uboot_one_in_block(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter);
int spinand_write_uboot_one_in_many_block(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter);
int spinand_write_uboot_one(unsigned char *buf, unsigned int len, struct _boot_info *info_buf, unsigned int info_len, unsigned int counter);
int spinand_read_uboot_one_in_block(unsigned char *buf, unsigned int len, unsigned int counter);
int spinand_read_uboot_one_in_many_block(unsigned char *buf, unsigned int len, unsigned int counter);
int spinand_read_uboot_one(unsigned char *buf, unsigned int len, unsigned int counter);
int spinand_get_param(void *nand_param);
int spinand_get_param_for_uboottail(void *nand_param);
__s32 SPINAND_SetPhyArch_V3(struct _boot_info *ram_arch, void *phy_arch);
int SPINAND_UpdatePhyArch(void);
int spinand_erase_chip(unsigned int chip, unsigned int start_block, unsigned int end_block, unsigned int force_flag);
void spinand_erase_special_block(void);
int spinand_uboot_erase_all_chip(UINT32 force_flag);
int spinand_dragonborad_test_one(unsigned char *buf, unsigned char *oob, unsigned int blk_num);
int spinand_physic_info_get_one_copy(unsigned int start_block, unsigned int pages_offset, unsigned int *block_per_copy, unsigned int *buf);
int spinand_add_len_to_uboot_tail(unsigned int uboot_size);
extern __u32 nand_get_nand_id_number_ctrl(struct sunxi_ndfc *ndfc);
extern __u32 nand_get_nand_extern_para(struct sunxi_ndfc *ndfc, __u32 para_num);

#endif /*UBOOT_TAIL_SPINAND_H*/
