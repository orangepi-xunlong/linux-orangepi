
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

#define _UBOOTT_C_

#include "../physic/spinand_interface_for_common.h"
#include "../physic_v2/nand_physic_inc.h"
#include "nand_common_interface.h"

extern int nand_secure_storage_block_bak;
extern int nand_secure_storage_block;



/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_boot0_one(unsigned char *buf, unsigned int len,
			 unsigned int counter)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_write_boot0_one(buf, len, counter);
	else if (get_storage_type() == 2)
		ret = spinand_write_boot0_one(buf, len, counter);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_boot0_one(unsigned char *buf, unsigned int len,
			unsigned int counter)
{
	int ret = 0;

	// if(get_storage_type() == 1)
	ret = rawnand_read_boot0_one(buf, len, counter);
	// else if(get_storage_type() == 2)
	//	ret = spinand_read_boot0_one(buf,len,counter);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_nboot_data(unsigned char *buf, unsigned int len)
{
	int ret = 1;
	unsigned int i;
	unsigned char *mbuf;

	PHY_DBG("burn boot0 normal mode!\n");

	for (i = 0; i < aw_nand_info.boot->uboot_start_block; i++) {
		ret &= nand_write_boot0_one(buf, len, i);
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_nboot_data(unsigned char *buf, unsigned int len)
{
	int ret = 0;
	unsigned int i;

	PHY_DBG("read boot0 normal mode!\n");

	for (i = 0; i < aw_nand_info.boot->uboot_start_block; i++) {
		ret = nand_read_boot0_one(buf, len, i);
		if (ret == 0) {
			return 0;
		} else {
			PHY_ERR("read boot0 one fail %d!\n", i);
		}
	}

	PHY_ERR("read boot0 fail!\n");

	return ERR_NO_106;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int check_nboot_table(unsigned char *buf, unsigned int len,
		      unsigned int counter)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_check_nboot(unsigned char *buf, unsigned int len)
{
	int i;
	int num = 0;
	int flag[NAND_BOOT0_BLK_CNT];
	int table_flag[NAND_BOOT0_BLK_CNT];

	for (i = 0; i < aw_nand_info.boot->uboot_start_block; i++) {
		table_flag[i] = check_nboot_table(buf, len, i);
		flag[i] = nand_read_boot0_one(buf, len, i);
	}

	if (flag[NAND_BOOT0_BLK_CNT - 1] != 0) {
		for (i = 0; i < NAND_BOOT0_BLK_CNT; i++) {
			if (flag[i] == 0) {
				if (nand_read_boot0_one(buf, len, i) == 0) {
					num = 1;
					break;
				}
			}
		}

		if (num == 0) {
			PHY_ERR("no boot0 ok!\n");
			return ERR_NO_102;
		}
	}

	for (i = 0; i < NAND_BOOT0_BLK_CNT; i++) {
		if (table_flag[i] == -1) {
			flag[i] = -1;
		}
	}

	if (flag[0] > 0) {
		flag[0] = 0;
	}

	for (i = 0; i < NAND_BOOT0_BLK_CNT; i++) {
		if (flag[i] != 0) {
			PHY_DBG("recover boot0 %d!\n", i);
			nand_write_boot0_one(buf, len, i);
		}
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_uboot_one(unsigned char *buf, unsigned int len,
			 struct _boot_info *info_buf, unsigned int info_len,
			 unsigned int counter)
{
	int ret = 0;
	int real_len;

	PHY_DBG("burn uboot one!\n");

	cal_sum_physic_info(phyinfo_buf, PHY_INFO_SIZE);

	// print_physic_info(phyinfo_buf);

	if (get_storage_type() == 1)
		ret = rawnand_write_uboot_one(buf, len, phyinfo_buf,
					      PHY_INFO_SIZE, counter);
	else if (get_storage_type() == 2)
		ret = spinand_write_uboot_one(buf, len, phyinfo_buf,
					      PHY_INFO_SIZE, counter);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_uboot_data(unsigned char *buf, unsigned int len)
{
	int ret = 0;
	unsigned int i;
	unsigned int uboot_block_cnt;

	uboot_block_cnt = aw_nand_info.boot->uboot_next_block -
			  aw_nand_info.boot->uboot_start_block;

	for (i = 0; i < uboot_block_cnt; i++) {
		ret |= nand_write_uboot_one(buf, len, phyinfo_buf,
					    PHY_INFO_SIZE, i);
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  <0:fail 1: ecc limit
*Note         :
*****************************************************************************/
int nand_read_uboot_one(unsigned char *buf, unsigned int len,
			unsigned int counter)
{
	int ret = 0;

	//	PHY_DBG("read uboot one %d!\n",counter);

	if (get_storage_type() == 1)
		ret = rawnand_read_uboot_one(buf, len, counter);
	else if (get_storage_type() == 2)
		ret = spinand_read_uboot_one(buf, len, counter);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_uboot_data(unsigned char *buf, unsigned int len)
{
	int ret = 0;
	unsigned int i;
	unsigned int uboot_block_cnt;

	PHY_DBG("read uboot!\n");

	uboot_block_cnt = aw_nand_info.boot->uboot_next_block -
			  aw_nand_info.boot->uboot_start_block;

	for (i = 0; i < uboot_block_cnt; i++) {
		ret = nand_read_uboot_one(buf, len, i);
		if (ret >= 0) {
			return 0;
		}
	}
	return -1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         : must after uboot_info_init
*****************************************************************************/
int nand_check_uboot(unsigned char *buf, unsigned int len)
{
	int i;
	int num = 0;
	int uboot_len;
	int counter, first_boot_flag;
	int flag[30];
	int boot_flag[30];
	char sbuf[64];
	struct _boot_info *boot;

	counter = uboot_info.copys;

	for (i = 0; i < counter; i++) {
		boot_flag[i] = -1;
		flag[i] = nand_read_uboot_one(buf, len, i);
		if (flag[i] >= 0) {
			boot =
			    (struct _boot_info
				 *)(buf + uboot_info.byte_offset_for_nand_info);
			if (boot->magic == PHY_INFO_MAGIC) {
				boot_flag[i] = 0;
			} else {
				PHY_ERR("boot magic error %x\n", boot->magic);
			}
		}
	}

	if (flag[counter - 1] < 0) {
		for (i = 0; i < counter; i++) {
			if (flag[i] >= 0) {
				if (nand_read_uboot_one(buf, len, i) == 0) {
					num = 1;
					break;
				}
			}
		}
		if (num == 0) {
			PHY_ERR("no uboot ok\n");
			return -1;
		}
	}

	if (flag[0] > 0) {
		flag[0] = 0;
	}

	first_boot_flag = 1;
	for (i = 0; i < counter; i++) {
		if (boot_flag[i] == 0) {
			first_boot_flag = 0;
		}
	}

	if (first_boot_flag == 1) {
		PHY_DBG("first boot flag\n");
		boot =
		    (struct _boot_info *)(buf +
					  uboot_info.byte_offset_for_nand_info);
		if (boot->magic != PHY_INFO_MAGIC) {
			PHY_DBG("recover uboot data\n");
			cal_sum_physic_info(aw_nand_info.boot, PHY_INFO_SIZE);
			MEMCPY(boot, aw_nand_info.boot, PHY_INFO_SIZE);
		}
	}

	uboot_len = uboot_info.uboot_len;

	for (i = 0; i < (counter / 2); i++) {// if error; change
		if ((flag[i] < 0) || (first_boot_flag == 1)) {
			PHY_DBG("recover1  uboot %d.\n", i);
			nand_write_uboot_one(buf, uboot_len, aw_nand_info.boot,
					     PHY_INFO_SIZE, i);
		} else if ((boot_flag[i] < 0) && (first_boot_flag == 0)) {
			PHY_DBG("recover2  uboot %d.\n", i);
			nand_write_uboot_one(buf, uboot_len, aw_nand_info.boot,
					     PHY_INFO_SIZE, i);
		} else {
			;
		}
	}

	for (i = (counter / 2); i < counter; i++) {// if ecc limit ; change
		if ((flag[i] != 0) || (first_boot_flag == 1)) {
			PHY_DBG("recover3  uboot %d.\n", i);
			nand_write_uboot_one(buf, uboot_len, aw_nand_info.boot,
					     PHY_INFO_SIZE, i);
		} else if ((boot_flag[i] < 0) && (first_boot_flag == 0)) {
			PHY_DBG("recover4  uboot %d.\n", i);
			nand_write_uboot_one(buf, uboot_len, aw_nand_info.boot,
					     PHY_INFO_SIZE, i);
		} else {
			;
		}
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_get_uboot_total_len(void)
{
	int len;

	uboot_info_init(&uboot_info);

	len = uboot_info.total_len;
	if ((len == 0) || (len >= 8 * 1024 * 1024)) {
		PHY_ERR("not uboot: %d\n", len);
		return 0;
	}
	return len;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_get_nboot_total_len(void)
{
	int len = 0;

	return len;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :NAND_GetParam
*****************************************************************************/
int nand_get_param(void *nand_param)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_get_param(nand_param);
	else if (get_storage_type() == 2)
		ret = spinand_get_param(nand_param);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :NAND_GetParam
*****************************************************************************/
int nand_get_param_for_uboottail(void *nand_param)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_get_param_for_uboottail(nand_param);
	else if (get_storage_type() == 2)
		ret = spinand_get_param_for_uboottail(nand_param);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int NAND_UpdatePhyArch(void)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = RAWNAND_UpdatePhyArch();
	else if (get_storage_type() == 2)
		ret = SPINAND_UpdatePhyArch();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_erase_chip(unsigned int chip, unsigned int start_block,
		    unsigned int end_block, unsigned int force_flag)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_erase_chip(chip, start_block, end_block,
					 force_flag);
	else if (get_storage_type() == 2)
		ret = spinand_erase_chip(chip, start_block, end_block,
					 force_flag);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void nand_erase_special_block(void)
{
	if (get_storage_type() == 1)
		rawnand_erase_special_block();
	else if (get_storage_type() == 2)
		spinand_erase_special_block();
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_uboot_erase_all_chip(UINT32 force_flag)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_uboot_erase_all_chip(force_flag);
	else if (get_storage_type() == 2)
		ret = spinand_uboot_erase_all_chip(force_flag);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_dragonborad_test_one(unsigned char *buf, unsigned char *oob,
			      unsigned int blk_num)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_dragonborad_test_one(buf, oob, blk_num);
	else if (get_storage_type() == 2)
		ret = spinand_dragonborad_test_one(buf, oob, blk_num);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int set_uboot_start_and_end_block(void)
{
	__u32 uboot_block_size;
	__u32 uboot_start_block;
	__u32 uboot_next_block;
	__u32 page_cnt_per_blk;

	page_cnt_per_blk = NAND_GetPageCntPerBlk();
	uboot_block_size = NAND_GetLsbblksize();

	/*small nand:block size < 1MB*/
	if (uboot_block_size <= 0x20000) {// 128K
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		uboot_next_block = uboot_start_block + 50;
	} else if (uboot_block_size <= 0x40000) {// 256k
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		uboot_next_block = uboot_start_block + 34;
	} else if (uboot_block_size <= 0x80000) {// 512k
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		uboot_next_block = uboot_start_block + 26;
	} else if (uboot_block_size <= 0x100000) {// 1M
		if (page_cnt_per_blk <= 128)
			uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		else
			uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
		uboot_next_block = uboot_start_block + 20;
	} else if (uboot_block_size <= 0x200000) {// 2M
		uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
		uboot_next_block = uboot_start_block + 10;
	} else {
		uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
		uboot_next_block = uboot_start_block + 8;
	}

	if (aw_nand_info.boot->uboot_start_block == 0) {
		aw_nand_info.boot->uboot_start_block = uboot_start_block;
		aw_nand_info.boot->uboot_next_block = uboot_next_block;
	}

	if (aw_nand_info.boot->physic_block_reserved == 0) {
		aw_nand_info.boot->physic_block_reserved = PHYSIC_RECV_BLOCK;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int get_uboot_start_block(void)
{
	__u32 uboot_block_size;
	__u32 uboot_start_block;
	__u32 page_cnt_per_blk;

	page_cnt_per_blk = NAND_GetPageCntPerBlk();

	uboot_block_size = NAND_GetLsbblksize();

	/*small nand:block size < 1MB*/
	if (uboot_block_size <= 0x80000) {// 512k
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
	} else if (uboot_block_size <= 0x100000) {// 1M
		if (page_cnt_per_blk <= 128)
			uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		else
			uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
	} else {
		uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
	}

	if (aw_nand_info.boot->uboot_start_block != 0) {
		uboot_start_block = aw_nand_info.boot->uboot_start_block;
	}

	return uboot_start_block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int get_uboot_next_block(void)
{
	__u32 uboot_block_size;
	__u32 uboot_start_block;
	__u32 uboot_next_block;
	__u32 page_cnt_per_blk;

	page_cnt_per_blk = NAND_GetPageCntPerBlk();

	uboot_block_size = NAND_GetLsbblksize();

	/*small nand:block size < 1MB*/
	if (uboot_block_size <= 0x20000) {// 128K
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		uboot_next_block = uboot_start_block + 50;
	} else if (uboot_block_size <= 0x40000) {// 256k
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		uboot_next_block = uboot_start_block + 34;
	} else if (uboot_block_size <= 0x80000) {// 512k
		uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		uboot_next_block = uboot_start_block + 26;
	} else if (uboot_block_size <= 0x100000) { // 1M
		if (page_cnt_per_blk <= 128)
			uboot_start_block = UBOOT_START_BLOCK_SMALLNAND;
		else
			uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
		uboot_next_block = uboot_start_block + 20;
	} else if (uboot_block_size <= 0x200000) { // 2M
		uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
		uboot_next_block = uboot_start_block + 10;
	} else {
		uboot_start_block = UBOOT_START_BLOCK_BIGNAND;
		uboot_next_block = uboot_start_block + 8;
	}

	if (aw_nand_info.boot->uboot_next_block != 0) {
		uboot_next_block = aw_nand_info.boot->uboot_next_block;
	}

	return uboot_next_block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int get_physic_block_reserved(void)
{
	__u32 physic_block_reserved;

	physic_block_reserved = PHYSIC_RECV_BLOCK;

	if (aw_nand_info.boot->physic_block_reserved != 0) {
		physic_block_reserved =
		    aw_nand_info.boot->physic_block_reserved;
	}

	return physic_block_reserved;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
void print_uboot_info(struct _uboot_info *uboot)
{
	int i;
	PHY_DBG("uboot->use_lsb_page %d \n", uboot->use_lsb_page);
	PHY_DBG("uboot->copys %d \n", uboot->copys);
	PHY_DBG("uboot->uboot_len %d \n", uboot->uboot_len);
	PHY_DBG("uboot->total_len %d \n", uboot->total_len);
	PHY_DBG("uboot->uboot_pages %d \n", uboot->uboot_pages);
	PHY_DBG("uboot->total_pages %d \n", uboot->total_pages);
	PHY_DBG("uboot->blocks_per_total %d \n", uboot->blocks_per_total);
	PHY_DBG("uboot->page_offset_for_nand_info %d \n",
		uboot->page_offset_for_nand_info);
	PHY_DBG("uboot->uboot_block:\n");
	for (i = 0; i < 60; i++) {
		if (uboot->uboot_block[i] != 0)
			PHY_DBG("%d ", uboot->uboot_block[i]);
	}
	PHY_DBG("\n");
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static unsigned int is_uboot_block(unsigned int block, char *uboot_buf, unsigned int *pages_offset)
{
	int ret = 0;
	unsigned char sdata[64];
	unsigned int size_per_page;
	unsigned int uboot_size;

	size_per_page = NAND_GetPageSize();

	ret = nand_physic_read_page(0, block, 0, size_per_page / 512, uboot_buf,
				    sdata);
	if (1 == (is_uboot_magic(uboot_buf))) {
		uboot_size = get_uboot_offset(uboot_buf);
		*pages_offset = uboot_size / size_per_page;
		if (uboot_size % size_per_page)
			*pages_offset = (*pages_offset) + 1;
		return 1;
	} else {
		return 0;
	}
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok   1:ecc limit     -1:fail
*Note         :
*****************************************************************************/
int uboot_info_init(struct _uboot_info *uboot)
{
	int ret = 0, blcok_offset;
	char *uboot_buf;
	unsigned int pages, i;
	unsigned int jump_size;
	unsigned int size_per_page;
	unsigned int uboot_size;
	unsigned int lsb_pages;
	unsigned int pages_offset;
	int uboot_find = 0;
	int uboot_ptr = 0;
	__u32 page_cnt_per_blk;

	page_cnt_per_blk = NAND_GetPageCntPerBlk();

	size_per_page = NAND_GetPageSize();
	MEMSET(uboot, 0, sizeof(struct _uboot_info));

	uboot_buf = MALLOC(size_per_page);
	if (uboot_buf == NULL) {
		PHY_ERR("uboot_buf malloc fail\n");
		return 1;
	}

	uboot->use_lsb_page = NAND_UsedLsbPages();
	blcok_offset = 0;

	for (i = aw_nand_info.boot->uboot_start_block;
	     i < aw_nand_info.boot->uboot_next_block; i++) {
		ret = nand_physic_bad_block_check(0, i);
		if (ret == 0) {
			uboot->uboot_block[uboot_ptr] = i;
			uboot_ptr++;
		}

		ret = is_uboot_block(i, uboot_buf, &pages_offset);
		if (ret == 1) {
			if (blcok_offset == 0) {
				uboot_find = 1;
				uboot->uboot_len = get_uboot_offset(uboot_buf);
				uboot->uboot_pages =
				    uboot->uboot_len / size_per_page;
				if (uboot->uboot_len % size_per_page) {
					uboot->uboot_pages++;
				}
				jump_size = size_per_page * uboot->uboot_pages -
					    uboot->uboot_len;

				uboot->total_len = uboot->uboot_len +
						   jump_size + PHY_INFO_SIZE;
				uboot->total_pages =
				    uboot->total_len / size_per_page;
				uboot->page_offset_for_nand_info =
				    uboot->uboot_pages;
				uboot->byte_offset_for_nand_info =
				    size_per_page * uboot->uboot_pages;
				if (uboot->use_lsb_page == 0) {
					uboot->blocks_per_total =
					    uboot->total_pages /
					    page_cnt_per_blk;
					if (uboot->total_pages %
					    page_cnt_per_blk) {
						uboot->blocks_per_total++;
					}
				} else {
					lsb_pages = NAND_GetLsbPages();
					uboot->blocks_per_total =
					    uboot->total_pages / lsb_pages;
					if (uboot->total_pages % lsb_pages) {
						uboot->blocks_per_total++;
					}
				}
			}
			blcok_offset++;
			// uboot->copys++;
			// i += uboot->blocks_per_total - 1;
		}
	}

	if (uboot_find == 1) {
		uboot->copys = uboot_ptr / uboot->blocks_per_total;
	} else {
		uboot->copys = 0;
	}

	print_uboot_info(uboot);

	FREE(uboot_buf, size_per_page);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int init_phyinfo_buf(void)
{
	phyinfo_buf = (struct _boot_info *)MALLOC(PHY_INFO_SIZE);
	if (phyinfo_buf == NULL) {
		PHY_ERR("init_phyinfo_buf malloc fail\n");
		return -1;
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int get_uboot_offset(void *buf)
{
	int *toc;
	toc = (int *)buf;
	return toc[TOC_LEN_OFFSET_BY_INT];
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_uboot_magic(void *buf)
{
	int *toc;
	toc = (int *)buf;

	return (toc[TOC_MAGIC_OFFSET_BY_INT] == TOC_MAIN_INFO_MAGIC) ? 1 : 0;
}

/********************************************************************************
* name	: check_magic
* func	: check the magic of boot0
*
* argu	: @mem_base	: the start address of boot0;
*         @magic	: the standard magic;
*
* return: CHECK_IS_CORRECT 	: magic is ok;
*         CHECK_IS_WRONG	: magic is wrong;
********************************************************************************/
int check_magic_physic_info(unsigned int *mem_base, char *magic)
{
	unsigned int i;
	struct _boot_info *bfh;
	unsigned int sz;
	unsigned char *p;

	bfh = (struct _boot_info *)mem_base;
	p = (unsigned char *)(bfh->magic);
	for (i = 0, sz = sizeof(bfh->magic); i < sz; i++) {
		if (*p++ != *magic++)
			return -1;
	}

	return 0;
}

/********************************************************************************
* name	: check_sum
* func	: check data using check sum.
*
* argu	: @mem_base	: the start address of data, it must be 4-byte aligned;
*         @size		: the size of data;
*
* return: CHECK_IS_CORRECT 	: the data is right;
*         CHECK_IS_WRONG	: the data is wrong;
********************************************************************************/
int check_sum_physic_info(unsigned int *mem_base, unsigned int size)
{
	unsigned int *buf;
	unsigned int count;
	unsigned int src_sum;
	unsigned int sum;
	struct _boot_info *bfh;

	bfh = (struct _boot_info *)mem_base;

	/* generate check sum */
	src_sum = bfh->sum; // get check_sum field from the head of boot0;
	bfh->sum = UBOOT_STAMP_VALUE; // replace the check_sum field of the
				      // boot0 head with STAMP_VALUE

	count = size >> 2; // unit, 4byte
	sum = 0;
	buf = (unsigned int *)mem_base;
	do {
		sum += *buf++; // calculate check sum
		sum += *buf++;
		sum += *buf++;
		sum += *buf++;
	} while ((count -= 4) > (4 - 1));

	while (count-- > 0)
		sum += *buf++;

	bfh->sum = src_sum; // restore the check_sum field of the boot0 head

	// msg("sum:0x%x - src_sum:0x%x\n", sum, src_sum);
	if (sum == src_sum)
		return 0; // ok
	else
		return -1; // err
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
unsigned int cal_sum_physic_info(struct _boot_info *mem_base, unsigned int size)
{
	unsigned int count, sum;
	struct _boot_info *temp_buf;
	unsigned int *buf;

	temp_buf = mem_base;
	temp_buf->sum = UBOOT_STAMP_VALUE;

	count = size >> 2;
	sum = 0;
	buf = (unsigned int *)mem_base;
	do {
		sum += *buf++;
		sum += *buf++;
		sum += *buf++;
		sum += *buf++;
	} while ((count -= 4) > (4 - 1));

	while (count-- > 0)
		sum += *buf++;

	temp_buf->sum = sum;

	return sum;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int physic_info_get_offset(unsigned int *pages_offset)
{
	unsigned int block;
	unsigned int size_per_page;
	unsigned int uboot_size;
	unsigned char sdata[64] = {
	    0xFF,
	};
	int ret = 0;
	unsigned int cnt;
	void *uboot_buf = NULL;

	PHY_DBG("physic_info_get_offset start!!\n");

	size_per_page = NAND_GetPageSize();

	uboot_buf = MALLOC(size_per_page);
	if (uboot_buf == NULL) {
		PHY_ERR("uboot_buf malloc fail\n");
		return -1;
	}

	cnt = 0;
	for (block = UBOOT_SCAN_START_BLOCK; block < UBOOT_MAX_BLOCK_NUM;
	     block++) {
		ret = nand_physic_read_page(0, block, 0, size_per_page / 512,
					    uboot_buf, sdata);
		if (ret == ERR_ECC) {
			PHY_ERR("ecc err:chip 0 block %d page 0\n", block);
			continue;
		}

#if 0
		{
			unsigned int  i;
			for (i = 1; i < 257; i++) {
				if ((i%8) == 0)
					PHY_DBG("\n");
				PHY_DBG(" %08x", *((unsigned int  *)uboot_buf+i-1));
			}
			PHY_DBG("\n");
		}
#endif

		if (1 == (is_uboot_magic(uboot_buf))) {
			cnt++;
			uboot_size = get_uboot_offset(uboot_buf);
			break;
		}
	}

	if (cnt) {
		*pages_offset = uboot_size / size_per_page;
		if (uboot_size % size_per_page)
			*pages_offset = (*pages_offset) + 1;
	}
	//	PHY_DBG("cnt %d pages_offset %d uboot_size
	//%x\n",cnt,*pages_offset,uboot_size);
	if (uboot_buf) {
		FREE(uboot_buf, size_per_page);
	}

	if (cnt)
		return 0;
	else
		return 1;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int physic_info_get_one_copy(unsigned int start_block,
			     unsigned int pages_offset,
			     unsigned int *block_per_copy, unsigned int *buf)
{
	int ret = 0;

	if (get_storage_type() == 1)
		ret = rawnand_physic_info_get_one_copy(
		    start_block, pages_offset, block_per_copy, buf);
	else if (get_storage_type() == 2)
		ret = spinand_physic_info_get_one_copy(
		    start_block, pages_offset, block_per_copy, buf);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int clean_physic_info(void)
{
	MEMSET((char *)phyinfo_buf, 0, PHY_INFO_SIZE);
	phyinfo_buf->len = PHY_INFO_SIZE;
	phyinfo_buf->magic = PHY_INFO_MAGIC;
	PHY_DBG("clean physic info uboot\n");
	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int physic_info_read(void)
{
	unsigned int copy;
	int ret = 0, ret_sum, ret_flag;
	unsigned int pages_offset;
	unsigned int start_block, block_per_copy, size_per_page;
	char *temp_buf;
	static int flag;

	PHY_DBG("physic_info_read start!!\n");

	if (flag == 1) {
		PHY_DBG("physic_info_read already!!\n");
		return 0;
	}

	if (init_phyinfo_buf() != 0) {
		return -1;
	}

	size_per_page = NAND_GetPageSize();

	/*ret = physic_info_get_offset(&pages_offset);
	if (ret) {
		MEMSET((char *)phyinfo_buf, 0, PHY_INFO_SIZE);
		phyinfo_buf->len = PHY_INFO_SIZE;
		phyinfo_buf->magic = PHY_INFO_MAGIC;
		PHY_ERR("can't find uboot head\n");
		flag = 1;
		return ret;
	}*/

	temp_buf = MALLOC(size_per_page);

	ret_flag = -1;

	for (start_block = UBOOT_SCAN_START_BLOCK;
	     start_block <= UBOOT_MAX_BLOCK_NUM; start_block++) {
		ret = is_uboot_block(start_block, temp_buf, &pages_offset);
		if (ret != 1) {
			continue;
		}
		physic_info_get_one_copy(start_block, pages_offset,
					 &block_per_copy,
					 (unsigned int *)phyinfo_buf);
//		PHY_DBG("start_block %d pages_offset %d block_per_copy
//%d\n",start_block,pages_offset,block_per_copy);
#if 0
		{
			unsigned int  i;
			for (i = 1; i < 2049; i++) {
				if ((i%8) == 0)
					PHY_DBG("\n");
				PHY_DBG(" %08x", *((unsigned int  *)phyinfo_buf+i-1));
			}
			PHY_DBG("\n");
			PHY_DBG("block_per_copy %d\n", block_per_copy);
		}
#endif

		ret_sum = check_sum_physic_info((unsigned int *)phyinfo_buf,
						PHY_INFO_SIZE);
		if (ret_sum == 0) {
			PHY_DBG("physic info copy is ok\n");
			ret_flag = 0;
			flag = 1;
			break;
		} else {
			PHY_DBG("physic info copy is bad\n");
			MEMSET((char *)phyinfo_buf, 0, PHY_INFO_SIZE);
			phyinfo_buf->len = PHY_INFO_SIZE;
			phyinfo_buf->magic = PHY_INFO_MAGIC;
			flag = 1;
		}
	}

	FREE(temp_buf, size_per_page);

	return ret_flag;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_phyinfo_empty(struct _boot_info *info)
{
	if (get_storage_type() == 1) {
		if ((info->magic == PHY_INFO_MAGIC) &&
		    (info->len == PHY_INFO_SIZE) &&
		    (info->storage_info.data.plane_cnt == 0) &&
		    (info->sum == 0)) {
			return 1;
		}
	} else if (get_storage_type() == 2) {
		if ((info->magic == PHY_INFO_MAGIC) &&
		    (info->len == PHY_INFO_SIZE) &&
		    (info->storage_info.config.plane_cnt == 0) &&
		    (info->sum == 0)) {
			return 1;
		}
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_len_to_uboot_tail(unsigned int uboot_size)
{
	int size = 0;

	if (get_storage_type() == 1)
		size = rawnand_add_len_to_uboot_tail(uboot_size);
	else if (get_storage_type() == 2)
		size = spinand_add_len_to_uboot_tail(uboot_size);

	return size;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int print_physic_info(struct _boot_info *info)
{

	unsigned int i;
	for (i = 1; i < (PHY_INFO_SIZE / 4) + 1; i++) {
		if ((i % 8) == 0)
			PHY_DBG("\n");
		PHY_DBG(" %08x", *((unsigned int *)info + i - 1));
	}
	PHY_DBG("end!\n");
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int physic_info_add_to_uboot_tail(unsigned int *buf_dst,
				  unsigned int uboot_size)
{
	unsigned int real_len;

	real_len = add_len_to_uboot_tail(uboot_size);

	cal_sum_physic_info(phyinfo_buf, PHY_INFO_SIZE);

#if 0
	{
		unsigned int  i;
		for (i = 1; i < 2049; i++) {
			if ((i%8) == 0)
				PHY_DBG("\n");
			PHY_DBG(" %08x", *((unsigned int  *)phyinfo_buf+i-1));
		}
		PHY_DBG("111\n");
	}
#endif

	PHY_DBG("physic_info_add_to_uboot_tail start2 %x!!\n", phyinfo_buf);

	MEMCPY((char *)buf_dst + (real_len - PHY_INFO_SIZE), phyinfo_buf,
	       PHY_INFO_SIZE);

	return 0;
}

int is_physic_info_enable_crc(void)
{
	return (phyinfo_buf->enable_crc == ENABLE_CRC_MAGIC);
}

