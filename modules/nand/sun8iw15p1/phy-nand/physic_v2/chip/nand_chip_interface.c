
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

#define _NCI_C_

#include "../../physic_common/nand_common_interface.h"
#include "../nand_physic_inc.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_erase_block(unsigned int chip, unsigned int block)
{
	int ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;

	nci = nci_get_from_nsi(g_nsi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = 0;
	npo.mdata = NULL;
	npo.sdata = NULL;
	npo.slen = 0;
	ret = nci->nand_physic_erase_block(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_read_page(unsigned int chip, unsigned int block,
			     unsigned int page, unsigned int bitmap,
			     unsigned char *mbuf, unsigned char *sbuf)
{
	int ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;

	nci = nci_get_from_nsi(g_nsi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = page;
	// npo.sect_bitmap = bitmap;
	npo.sect_bitmap = g_nsi->nci->sector_cnt_per_page;
	npo.mdata = mbuf;
	npo.sdata = sbuf;
	npo.slen = nci->sdata_bytes_per_page;

	ret = nci->nand_physic_read_page(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_write_page(unsigned int chip, unsigned int block,
			      unsigned int page, unsigned int bitmap,
			      unsigned char *mbuf, unsigned char *sbuf)
{
	int ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;

	nci = nci_get_from_nsi(g_nsi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = page;
	// npo.sect_bitmap = bitmap;
	npo.sect_bitmap = g_nsi->nci->sector_cnt_per_page;
	npo.mdata = mbuf;
	npo.sdata = sbuf;
	npo.slen = nci->sdata_bytes_per_page;
	ret = nci->nand_physic_write_page(&npo);

#if 0

	unsigned char *buf;
	unsigned char spare[64];
	int page_size, i;

	page_size = g_nsi->nci->sector_cnt_per_page<<9;
	buf = nand_get_temp_buf(page_size);

	npo.chip = chip;
	npo.block = block;
	npo.page = page;
	npo.sect_bitmap = bitmap;
	npo.mdata = buf;
	npo.sdata = spare;
	npo.slen = nci->sdata_bytes_per_page;
	nci->nand_physic_read_page(&npo);

	for (i = 0; i < page_size; i++) {
		if (buf[i] != mbuf[i]) {
			PHY_ERR("data error chip:0x%x block:0x%x page:0x%x \n", chip, block, page);
			break;
		}
	}
	nand_free_temp_buf(buf, page_size);

#endif

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_bad_block_check(unsigned int chip, unsigned int block)
{
	int ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = 0;
	npo.mdata = NULL;
	npo.sdata = NULL;
	nci = nci_get_from_nsi(g_nsi, chip);
	ret = nci->nand_physic_bad_block_check(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_bad_block_mark(unsigned int chip, unsigned int block)
{
	int ret;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = 0;
	npo.mdata = NULL;
	npo.sdata = NULL;
	nci = nci_get_from_nsi(g_nsi, chip);
	ret = nci->nand_physic_bad_block_mark(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_write_data_in_whole_block(unsigned int chip, unsigned int block,
				   unsigned char *mbuf, unsigned int mlen,
				   unsigned char *sbuf, unsigned int slen)
{
	int ret, i, flag;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;
	unsigned char spare[64];

	nci = nci_get_from_nsi(g_nsi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = nci->sector_cnt_per_page;
	npo.mdata = NULL;
	npo.sdata = NULL;
	npo.slen = 0;

	ret = nci->nand_physic_erase_block(&npo);
	if (ret != 0) {
		PHY_ERR(
		    "nand_write_data_in block error1 chip:0x%x block:0x%x \n",
		    chip, block);
		return ret;
	}
	nand_wait_all_rb_ready();

	MEMCPY(spare, sbuf, nci->sdata_bytes_per_page);

	flag = ERR_NO_132;
	for (i = 0; i < nci->page_cnt_per_blk; i++) {
		npo.chip = chip;
		npo.block = block;
		npo.page = i;
		npo.sect_bitmap = nci->sector_cnt_per_page;
		npo.mdata = mbuf;
		npo.sdata = spare;
		npo.slen = nci->sdata_bytes_per_page;
		ret = nci->nand_physic_write_page(&npo);
		nand_wait_all_rb_ready();
		if (ret == 0) {
			flag = 0;
		} else {
			PHY_ERR("nand_write_data_in block error2 chip:0x%x "
				"block:0x%x page:0x%x \n",
				chip, block, i);
		}
	}

	return flag;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_data_in_whole_block(unsigned int chip, unsigned int block,
				  unsigned char *mbuf, unsigned int mlen,
				  unsigned char *sbuf, unsigned int slen)
{
	int ret, i, flag, page_size;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par npo;
	unsigned char spare[64];
	unsigned char *buf;

	page_size = g_nsi->nci->sector_cnt_per_page << 9;

	if (mlen < page_size) {
		buf = nand_get_temp_buf(page_size);
	} else {
		buf = mbuf;
	}

	nci = nci_get_from_nsi(g_nsi, chip);

	flag = ERR_NO_132;
	for (i = 0; i < nci->page_cnt_per_blk; i++) {
		npo.chip = chip;
		npo.block = block;
		npo.page = i;
		npo.sect_bitmap = nci->sector_cnt_per_page;
		npo.mdata = buf;
		npo.sdata = spare;
		npo.slen = nci->sdata_bytes_per_page;
		ret = nci->nand_physic_read_page(&npo);
		if (ret >= 0) {
			flag = 0;
			break;
		} else {
			PHY_ERR("nand_read_data_in block error2 chip:0x%x "
				"block:0x%x page:0x%x \n",
				chip, block, i);
		}
	}

	MEMCPY(sbuf, spare, slen);
	if (mlen < page_size) {
		MEMCPY(mbuf, buf, mlen);
		nand_free_temp_buf(buf, page_size);
	}

	return flag;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_block_copy(unsigned int chip_s, unsigned int block_s,
			      unsigned int chip_d, unsigned int block_d)
{
	int i, ret = 0;
	unsigned char spare[64];
	unsigned char *buf;

	buf = nand_get_temp_buf(g_nsi->nci->sector_cnt_per_page << 9);

	for (i = 0; i < g_nsi->nci->page_cnt_per_blk; i++) {
		ret |= nand_physic_read_page(chip_s, block_s, i,
					     g_nsi->nci->sector_cnt_per_page,
					     buf, spare);
		ret |= nand_physic_write_page(chip_d, block_d, i,
					      g_nsi->nci->sector_cnt_per_page,
					      buf, spare);
	}

	nand_free_temp_buf(buf, g_nsi->nci->sector_cnt_per_page << 9);
	return ret;
}
