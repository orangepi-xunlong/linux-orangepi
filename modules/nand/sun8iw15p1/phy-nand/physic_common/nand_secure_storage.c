
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

/*****************************************************************************/
#define _SECURE_STORAGE_C_
/*****************************************************************************/
#include "nand_common_interface.h"

extern struct _nand_info aw_nand_info;

/*****************************************************************************/

#define MAX_SECURE_STORAGE_ITEM 32

#define MIN_SECURE_STORAGE_BLOCK_NUM 8
#define MAX_SECURE_STORAGE_BLOCK_NUM 100

int nand_secure_storage_block;
int nand_secure_storage_block_bak;
#if 0

int nand_secure_storage_read(int item, unsigned char *buf, unsigned int len);
int nand_secure_storage_write(int item, unsigned char *buf, unsigned int len);
int nand_secure_storage_first_build(unsigned int start_block);
int get_nand_secure_storage_max_item(void);

int nand_secure_storage_write_init(unsigned int block);
unsigned int nand_secure_check_sum(unsigned char *mbuf, unsigned int len);
int nand_secure_storage_read_one(unsigned int block, int item, unsigned char *mbuf, unsigned int len);
int nand_secure_storage_check(void);
int nand_secure_storage_update(void);
int nand_secure_storage_repair(int flag);
#endif

__u32 NAND_Get_SecureBlock_Start(void)
{
	int block_start;

	block_start = aw_nand_info.boot->uboot_next_block;
	return block_start;
}

/*****************************************************************************
*Name         :nand_is_support_secure_storage
*Description  : judge if the flash support secure storage
*Parameter    : null
*Return       : 0:support  -1:don't support
*Note         :
*****************************************************************************/
#if 0
int nand_is_support_secure_storage(void)
{
	__u32 block, page_size;
	__u8 spare[32];
	__u8 *mbuf;

	page_size = NAND_GetPageSize();
	mbuf = (__u8 *)MALLOC(page_size);

	block = 7;
	while (block < 50) {
		nand_physic_read_page(0, block, 0, mbuf, spare);
		if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0xbb)) {
			if (spare[3] == 0x01) {
				PRINT_DBG("nand:found factory_bad_block(new version) table in block:%d!\n", block);
				PRINT_DBG("nand:support secure storage\n");
				FREE((void *)mbuf, page_size);
				return 0;
			} else if (spare[3] == 0xff) {
				PRINT_DBG("found factory_bad_block(old version) table in block:%d!\n", block);
				PRINT_DBG("nand:don't support secure storage\n");
				FREE((void *)mbuf, page_size);
				return -1;
			}
		}
		block++;
	}
	PRINT_DBG("new nand flash\n");
	PRINT_DBG("nand:support secure storage\n");
	FREE((void *)mbuf, page_size);
	return 0;
}
#else
int nand_is_support_secure_storage(void)
{
	return 0;
}

#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
#if 0
int nand_physic_erase_block(unsigned int chip, unsigned int block)
{
	struct boot_physical_param  para;
	int  ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

	NAND_PhysicLock();

	lnpo.chip = chip;
	lnpo.block = block;
	ret = nci->nand_physic_erase_block(&lnpo);

	NAND_PhysicUnLock();

	return ret;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
#if 0
int nand_physic_read_page(unsigned int chip, unsigned int block, unsigned int page, unsigned char *mbuf, unsigned char *sbuf)
{
	struct boot_physical_param  para;
	int ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

	NAND_PhysicLock();

	lnpo.chip = chip;
	lnpo.block = block;
	lnpo.page  = page;
	lnpo.mdata = (void *) mbuf;
	lnpo.sect_bitmap = nci->sector_cnt_per_page;
	lnpo.sdata = sbuf;
	lnpo.slen = 64;

	ret = nci->nand_physic_read_page(&lnpo);

	NAND_PhysicUnLock();

	return ret;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
#if 0
int nand_physic_write_page(unsigned int chip, unsigned int block, unsigned int page, unsigned char *mbuf, unsigned char *sbuf)
{
	struct boot_physical_param  para;
	int  ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;
	NAND_PhysicLock();

	lnpo.chip = chip;
	lnpo.block = block;
	lnpo.page  = page;
	lnpo.mdata = (void *) mbuf;
	lnpo.sect_bitmap = nci->sector_cnt_per_page;
	lnpo.sdata = sbuf;
	lnpo.slen = 64;

	ret = nci->nand_physic_write_page(&lnpo);

	NAND_PhysicUnLock();

	return ret;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:good block  other:bad block
*Note         :
*****************************************************************************/
#if 0
int nand_physic_bad_block_check_secure(unsigned int chip, unsigned int block)
{
	struct boot_physical_param  para;
	int  ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

	lnpo.chip = chip;
	lnpo.block = block;

	ret = nci->nand_physic_bad_block_check(&lnpo);
	if (ret)
		return -1;

	return 0;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
#if 0
int nand_physic_block_copy(unsigned int chip_s, unsigned int block_s, unsigned int chip_d, unsigned int block_d)
{
	int i, ret = 0;
	unsigned char spare[64];
	unsigned char *buf;
	unsigned int page_size, page_cnt_per_blk;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo_s;
	struct _nand_physic_op_par lnpo_d;

	nci = g_nctri->nci;

	page_size = nci->sector_cnt_per_page*512;
	page_cnt_per_blk = nci->page_cnt_per_blk;

	buf = MALLOC(page_size);
	if (!buf) {
		PHY_ERR("nand malloc fail\n");
		return -1;
	}

	for (i = 0; i < page_cnt_per_blk; i++) {
		ret |= nand_physic_read_page(chip_s, block_s, i, buf, spare);
		ret |= nand_physic_write_page(chip_d, block_d, i, buf, spare);
	}

	FREE(buf, page_size);

	return ret;
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_write_init(unsigned int block)
{
	int ret = -1;
	unsigned char *mbuf;
	unsigned int page_size, i, page_cnt_per_blk;
	unsigned char spare[64];

	page_size = NAND_GetPageSize();
	page_cnt_per_blk = NAND_GetPageCntPerBlk();
	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	MEMSET(mbuf, 0, page_size);
	MEMSET(spare, 0xff, 64);
	spare[1] = 0xaa;
	spare[2] = 0x5c;
	spare[3] = 0x00;
	spare[4] = 0x00;
	spare[5] = 0x12;
	spare[6] = 0x34;

	if (nand_physic_erase_block(0, block) == 0) {
		for (i = 0; i < page_cnt_per_blk; i++) {
			nand_physic_write_page(0, block, i, page_size / 512,
					       mbuf, spare);
		}
		mbuf[0] = 0X11;
		ret = nand_secure_storage_read_one(block, 0, mbuf, 2048);
	}

	FREE(mbuf, page_size);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_clean_data(unsigned int num)
{
	if (num == 0)
		nand_physic_erase_block(0, nand_secure_storage_block);
	if (num == 1)
		nand_physic_erase_block(0, nand_secure_storage_block_bak);
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_clean(void)
{
	nand_secure_storage_clean_data(0);
	nand_secure_storage_clean_data(1);
	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_nand_secure_storage_build(void)
{
	if ((nand_secure_storage_block_bak > MIN_SECURE_STORAGE_BLOCK_NUM) &&
	    (nand_secure_storage_block_bak < MAX_SECURE_STORAGE_BLOCK_NUM)) {
		return 1;
	} else {
		return 0;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int is_nand_secure_storage_block(unsigned int block)
{
	unsigned char *mbuf;
	int page_size, ret = 0;
	unsigned char spare[64];

	page_size = NAND_GetPageSize();
	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	nand_physic_read_page(0, block, 0, page_size / 512, mbuf, spare);
	if ((spare[0] == 0xff) && (spare[1] == 0xaa) && (spare[2] == 0x5c)) {
		ret = 1;
	}

	FREE(mbuf, page_size);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_init(int flag)
{
	unsigned int nBlkNum;
	int ret;

	nand_secure_storage_block = 0;
	nand_secure_storage_block_bak = 0;

	if (nand_is_support_secure_storage() != 0)
		return 0;

	nBlkNum = NAND_Get_SecureBlock_Start();

	for (; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		ret = is_nand_secure_storage_block(nBlkNum);
		if (ret == 1) {
			nand_secure_storage_block = nBlkNum;
			break;
		}
	}

	for (nBlkNum += 1; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		ret = is_nand_secure_storage_block(nBlkNum);
		if (ret == 1) {
			nand_secure_storage_block_bak = nBlkNum;
			break;
		}
	}

	if ((nand_secure_storage_block < MIN_SECURE_STORAGE_BLOCK_NUM) ||
	    (nand_secure_storage_block >= nand_secure_storage_block_bak)) {
		nand_secure_storage_block = 0;
		nand_secure_storage_block_bak = 0;
		PHY_ERR("nand secure storage fail: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
		ret = -1;
	} else {
		if (flag != 0) {
			nand_secure_storage_update();
		}
		ret = 0;
		PHY_DBG("nand secure storage ok: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : only called when first build
*****************************************************************************/
int nand_secure_storage_first_build(unsigned int start_block)
{
	int block = -1;
	unsigned int nBlkNum;
	int ret;

	if (nand_is_support_secure_storage() != 0)
		return start_block;

	if ((nand_secure_storage_block_bak > MIN_SECURE_STORAGE_BLOCK_NUM) &&
	    (nand_secure_storage_block_bak < MAX_SECURE_STORAGE_BLOCK_NUM)) {
		PHY_DBG("start block:%d\n", nand_secure_storage_block_bak + 1);
		return nand_secure_storage_block_bak + 1;
	}

	nBlkNum = start_block;

	for (; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		if (nand_physic_bad_block_check(0, nBlkNum) == 0) {
			ret = is_nand_secure_storage_block(nBlkNum);
			if (ret != 1) {
				nand_secure_storage_write_init(nBlkNum);
			}
			nand_secure_storage_block = nBlkNum;
			break;
		}
	}

	for (nBlkNum += 1; nBlkNum < MAX_SECURE_STORAGE_BLOCK_NUM; nBlkNum++) {
		if (nand_physic_bad_block_check(0, nBlkNum) == 0) {
			ret = is_nand_secure_storage_block(nBlkNum);
			if (ret != 1) {
				nand_secure_storage_write_init(nBlkNum);
			}
			nand_secure_storage_block_bak = nBlkNum;
			break;
		}
	}

	if ((nand_secure_storage_block < MIN_SECURE_STORAGE_BLOCK_NUM) ||
	    (nand_secure_storage_block >= nand_secure_storage_block_bak)) {
		PHY_ERR("nand secure storage firsr build  fail: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
		nand_secure_storage_block = 0;
		nand_secure_storage_block_bak = 0;
		block = start_block + 2;
	} else {
		block = nand_secure_storage_block_bak + 1;
		PHY_DBG("nand secure storage firsr build  ok: %d,%d\n",
			nand_secure_storage_block,
			nand_secure_storage_block_bak);
	}

	return block;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         : api
*****************************************************************************/
int get_nand_secure_storage_max_item(void)
{
	unsigned int page_cnt_per_blk = NAND_GetPageCntPerBlk();

	if (MAX_SECURE_STORAGE_ITEM < page_cnt_per_blk) {
		return MAX_SECURE_STORAGE_ITEM;
	}
	return page_cnt_per_blk;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
unsigned int nand_secure_check_sum(unsigned char *mbuf, unsigned int len)
{
	unsigned int check_sum, i;
	unsigned int *p;

	p = (unsigned int *)mbuf;
	check_sum = 0x1234;
	len >>= 2;

	for (i = 0; i < len; i++) {
		check_sum += p[i];
	}
	return check_sum;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_read_one(unsigned int block, int item,
				 unsigned char *mbuf, unsigned int len)
{
	unsigned char spare[64];
	unsigned int check_sum, read_sum, page_cnt_per_blk;
	int page_size;

	page_size = NAND_GetPageSize();

	page_cnt_per_blk = NAND_GetPageCntPerBlk();

	for (; item < page_cnt_per_blk; item += MAX_SECURE_STORAGE_ITEM) {
		MEMSET(spare, 0, 64);
		nand_physic_read_page(0, block, item, page_size / 512, mbuf,
				      spare);
		if ((spare[1] != 0xaa) || (spare[2] != 0x5c)) {
			continue;
		}

		check_sum = nand_secure_check_sum(mbuf, len);

		read_sum = spare[3];
		read_sum <<= 8;
		read_sum |= spare[4];
		read_sum <<= 8;
		read_sum |= spare[5];
		read_sum <<= 8;
		read_sum |= spare[6];

		if (read_sum == check_sum) {
			if (read_sum == 0x1234) {
				return 1;
			}
			return 0;
		} else {
			PHY_DBG("spare_sum:0x%x,check_sum:0x%x\n", read_sum,
				check_sum);
		}
	}
	return -1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :api
*****************************************************************************/
int nand_secure_data_read(int item, unsigned char *buf, unsigned int len)
{

	unsigned char *mbuf;
	int page_size, ret;
	unsigned char spare[64];

	if (nand_is_support_secure_storage())
		return 0;

	NAND_PhysicLock();

	page_size = NAND_GetPageSize();
	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	ret = nand_secure_storage_read_one(nand_secure_storage_block, item,
					   mbuf, len);
	if (ret != 0) {
		ret = nand_secure_storage_read_one(
		    nand_secure_storage_block_bak, item, mbuf, len);
	}

	if (ret == 0) {
		MEMCPY(buf, mbuf, len);
	} else {
		MEMSET(buf, 0, len);
	}

	FREE(mbuf, page_size);

	NAND_PhysicUnLock();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :api
*****************************************************************************/
int nand_secure_data_write(int item, unsigned char *buf, unsigned int len)
{
	int ret, retry = 0;
	unsigned char *mbuf;
	unsigned int page_size, i, check_sum, page, page_cnt_per_blk;
	unsigned char spare[64];

	if (nand_is_support_secure_storage())
		return 0;

	NAND_PhysicLock();

	nand_secure_storage_update();

	page_size = NAND_GetPageSize();
	page_cnt_per_blk = NAND_GetPageCntPerBlk();
	if (len > page_size) {
		NAND_PhysicUnLock();
		return -1;
	}

	PHY_DBG("nand_secure_storage_write: %d,%d\n", nand_secure_storage_block,
		nand_secure_storage_block_bak);

	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	nand_physic_erase_block(0, nand_secure_storage_block_bak);
	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		if (i != item) {
			nand_physic_read_page(0, nand_secure_storage_block, i,
					      page_size / 512, mbuf, spare);
		} else {
			MEMSET(spare, 0xff, 64);
			MEMSET(mbuf, 0, page_size);
			MEMCPY(mbuf, buf, len);
			check_sum = nand_secure_check_sum(mbuf, len);
			// spare[0] = 0x00;
			spare[1] = 0xaa;
			spare[2] = 0x5c;
			spare[3] = (unsigned char)(check_sum >> 24);
			spare[4] = (unsigned char)(check_sum >> 16);
			spare[5] = (unsigned char)(check_sum >> 8);
			spare[6] = (unsigned char)(check_sum);
		}
		nand_physic_write_page(0, nand_secure_storage_block_bak, i,
				       page_size / 512, mbuf, spare);
	}

	for (i = MAX_SECURE_STORAGE_ITEM; i < page_cnt_per_blk; i++) {
		page = i % MAX_SECURE_STORAGE_ITEM;
		nand_physic_read_page(0, nand_secure_storage_block_bak, page,
				      page_size / 512, mbuf, spare);
		nand_physic_write_page(0, nand_secure_storage_block_bak, i,
				       page_size / 512, mbuf, spare);
	}

	nand_secure_storage_repair(2);

	FREE(mbuf, page_size);

	NAND_PhysicUnLock();

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : -1;0;1;2
*Note         :
*****************************************************************************/
int nand_secure_storage_check(void)
{
	unsigned char *mbuf;
	int i, page_size, ret = -1, ret1, ret2;

	page_size = NAND_GetPageSize();
	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		ret1 = nand_secure_storage_read_one(nand_secure_storage_block,
						    i, mbuf, page_size);
		ret2 = nand_secure_storage_read_one(
		    nand_secure_storage_block_bak, i, mbuf, page_size);
		if (ret1 != ret2) {
			break;
		}
		if (ret1 < 0) {
			break;
		}
	}

	if ((ret1 < 0) && (ret2 < 0)) {
		ret = -1;
		PHY_ERR("nand secure storage check fail:%d\n", i);
		goto ss_check_out;
	}

	if (ret1 == ret2) {
		ret = 0;
		goto ss_check_out;
	}

	if ((ret1 == 0) || (ret2 < 0)) {
		ret = 1;
		// block_d = nand_secure_storage_block_bak;
		// block_s = nand_secure_storage_block;
	}

	if ((ret2 == 0) || (ret1 < 0)) {
		ret = 2;
		// block_d = nand_secure_storage_block;
		// block_s = nand_secure_storage_block_bak;
	}

ss_check_out:

	FREE(mbuf, page_size);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_repair(int flag)
{
	int ret = 0;
	unsigned int block_s = 0, block_d = 0;

	if (flag == 0) {
		return 0;
	}

	if (flag == 1) {
		block_s = nand_secure_storage_block;
		block_d = nand_secure_storage_block_bak;
	}

	if (flag == 2) {
		block_s = nand_secure_storage_block_bak;
		block_d = nand_secure_storage_block;
	}

	ret |= nand_physic_erase_block(0, block_d);

	ret |= nand_physic_block_copy(0, block_s, 0, block_d);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_update(void)
{
	int ret, retry = 0;

	while (1) {
		ret = nand_secure_storage_check();
		if (ret == 0) {
			break;
		}

		retry++;
		if (ret < 0) {
			PHY_ERR("secure storage fail 1\n");
			return -1;
		}
		if (ret > 0) {
			PHY_DBG("secure storage repair:%d\n", ret);
			nand_secure_storage_repair(ret);
		}
		if (retry > 3) {
			return -1;
		}
	}
	PHY_DBG("secure storage updata ok!\n");
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_test(unsigned int item)
{
	unsigned char *mbuf;
	int i, page_size, ret = -1, ret1, ret2;

	if (item >= MAX_SECURE_STORAGE_ITEM) {
		return 0;
	}

	page_size = NAND_GetPageSize();
	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		ret = nand_secure_data_read(i, mbuf, 2048);
		PHY_DBG(
		    "read secure storage:%d ret %d buf :0x%x,0x%x,0x%x,0x%x,\n",
		    i, ret, mbuf[0], mbuf[1], mbuf[2], mbuf[3]);
	}

	MEMSET(mbuf, 0, 2048);
	mbuf[0] = 0x00 + item;
	mbuf[1] = 0x11 + item;
	mbuf[2] = 0x22 + item;
	mbuf[3] = 0x33 + item;
	nand_secure_data_write(item, mbuf, 2048);

	for (i = 0; i < MAX_SECURE_STORAGE_ITEM; i++) {
		ret = nand_secure_data_read(i, mbuf, 2048);
		PHY_DBG(
		    "read secure storage:%d ret %d buf :0x%x,0x%x,0x%x,0x%x,\n",
		    i, ret, mbuf[0], mbuf[1], mbuf[2], mbuf[3]);
	}

	FREE(mbuf, page_size);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_secure_storage_test_read(unsigned int item, unsigned int block)
{
	unsigned char *mbuf;
	int page_size, ret;
	unsigned char spare[64];

	page_size = NAND_GetPageSize();
	mbuf = MALLOC(page_size);
	if (mbuf == NULL) {
		PHY_ERR("%s:malloc fail for mbuf\n", __func__);
	}

	if (block == 0)
		block = nand_secure_storage_block;
	if (block == 1)
		block = nand_secure_storage_block_bak;

	ret = nand_secure_storage_read_one(block, item, mbuf, 2048);

	PHY_DBG("nand_secure_storage_test_read item:%d ret:%d "
		"buf:0x%x,0x%x,0x%x,0x%x,\n",
		item, ret, mbuf[0], mbuf[1], mbuf[2], mbuf[3]);

	FREE(mbuf, page_size);
	return 0;
}
