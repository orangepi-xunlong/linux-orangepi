
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


#define _NSCI_C_

#include "../nand_physic_inc.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_erase_super_block(unsigned int chip, unsigned int block)
{
	int ret;
	struct _nand_super_chip_info *nsci;
	struct _nand_physic_op_par npo;

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = 0;
	npo.mdata = NULL;
	npo.sdata = NULL;
	npo.slen = 0;
	nsci = nsci_get_from_nssi(g_nssi, chip);
	ret = nsci->nand_physic_erase_super_block(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_read_super_page(unsigned int chip, unsigned int block,
				   unsigned int page, unsigned int bitmap,
				   unsigned char *mbuf, unsigned char *sbuf)
{
	int ret;
	struct _nand_super_chip_info *nsci;
	struct _nand_physic_op_par npo;

	nsci = nsci_get_from_nssi(g_nssi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = page;
	npo.sect_bitmap = bitmap;
	npo.mdata = mbuf;
	npo.sdata = sbuf;
	npo.slen = nsci->spare_bytes;
	ret = nsci->nand_physic_read_super_page(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_write_super_page(unsigned int chip, unsigned int block,
				    unsigned int page, unsigned int bitmap,
				    unsigned char *mbuf, unsigned char *sbuf)
{
	int ret;
	struct _nand_super_chip_info *nsci;
	struct _nand_physic_op_par npo;

	nsci = nsci_get_from_nssi(g_nssi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = page;
	npo.sect_bitmap = bitmap;
	npo.mdata = mbuf;
	npo.sdata = sbuf;
	npo.slen = nsci->spare_bytes;
	ret = nsci->nand_physic_write_super_page(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_super_bad_block_check(unsigned int chip, unsigned int block)
{
	int ret;
	struct _nand_super_chip_info *nsci;
	struct _nand_physic_op_par npo;

	nsci = nsci_get_from_nssi(g_nssi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = 0;
	npo.mdata = NULL;
	npo.sdata = NULL;
	npo.slen = 0;
	ret = nsci->nand_physic_super_bad_block_check(&npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int rawnand_physic_super_bad_block_mark(unsigned int chip, unsigned int block)
{
	int ret;
	struct _nand_super_chip_info *nsci;
	struct _nand_physic_op_par npo;

	nsci = nsci_get_from_nssi(g_nssi, chip);

	npo.chip = chip;
	npo.block = block;
	npo.page = 0;
	npo.sect_bitmap = 0;
	npo.mdata = NULL;
	npo.sdata = NULL;
	npo.slen = 0;
	ret = nsci->nand_physic_super_bad_block_mark(&npo);

	return ret;
}
