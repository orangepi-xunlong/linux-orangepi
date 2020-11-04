
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

#define _NSCO0_C_

#include "../nand_physic_inc.h"

#define NAND_OPEN_BLOCK_CNT (8)

struct nand_phy_write_lsb_cache {
	unsigned int cache_use_status;
	struct _nand_physic_op_par tmp_npo;
};

struct nand_phy_write_lsb_cache nand_phy_w_cache[NAND_OPEN_BLOCK_CNT] = {
    0, {0},
};

int nand_phy_get_page_type(unsigned int page)
{
	/*
	    for SPECTEK L04a 3D nand
	    page type:1-> independent page,
	    page type:2-> lsb page
	    page type:3-> msb page
	*/
	if ((page <= 15) || (page >= 496))
		return 1;

	if (page % 2 == 0)
		return 2;

	return 3;
}

int nand_phy_low_page_write_cache_set(struct _nand_physic_op_par *npo,
				      unsigned int two_plane)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if (nand_phy_w_cache[i].cache_use_status == 0) {
			/*get a new lsb page cache and backup for L04a*/
			nand_phy_w_cache[i].cache_use_status = 1;
			nand_phy_w_cache[i].tmp_npo.chip = npo->chip;
			nand_phy_w_cache[i].tmp_npo.block = npo->block;
			nand_phy_w_cache[i].tmp_npo.page = npo->page;
			nand_phy_w_cache[i].tmp_npo.sect_bitmap =
			    npo->sect_bitmap;
			nand_phy_w_cache[i].tmp_npo.slen = npo->slen;

			if (nand_phy_w_cache[i].tmp_npo.mdata == NULL) {
				if (two_plane == 1)
					nand_phy_w_cache[i].tmp_npo.mdata =
					    NAND_Malloc(
						2 * (npo->sect_bitmap << 9));
				else
					nand_phy_w_cache[i].tmp_npo.mdata =
					    NAND_Malloc(npo->sect_bitmap << 9);
			}
			if (nand_phy_w_cache[i].tmp_npo.sdata == NULL) {
				nand_phy_w_cache[i].tmp_npo.sdata =
				    NAND_Malloc(npo->slen);
				NAND_Memset(nand_phy_w_cache[i].tmp_npo.sdata,
					    0xff, npo->slen);
			}

			if (npo->mdata) {
				if (two_plane == 1)
					NAND_Memcpy(
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    npo->mdata,
					    2 * (npo->sect_bitmap << 9));
				else
					NAND_Memcpy(
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    npo->mdata,
					    (npo->sect_bitmap << 9));
			}

			if (npo->sdata)
				NAND_Memcpy(nand_phy_w_cache[i].tmp_npo.sdata,
					    npo->sdata, npo->slen);

			return 0;
		}
	}

	PHY_ERR("ERR! no cache buf for lsb page!\n");

	return -1;
}

struct _nand_physic_op_par *
nand_phy_low_page_cache_get_for_write(struct _nand_physic_op_par *npo)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if ((nand_phy_w_cache[i].cache_use_status == 1) &&
		    (nand_phy_w_cache[i].tmp_npo.chip == npo->chip) &&
		    ((nand_phy_w_cache[i].tmp_npo.page + 1) == npo->page) &&
		    (nand_phy_w_cache[i].tmp_npo.block == npo->block) &&
		    (nand_phy_w_cache[i].tmp_npo.sect_bitmap ==
		     npo->sect_bitmap) &&
		    (nand_phy_w_cache[i].tmp_npo.slen == npo->slen) &&
		    (nand_phy_w_cache[i].tmp_npo.mdata != NULL) &&
		    (nand_phy_w_cache[i].tmp_npo.sdata != NULL)) {

			return &(nand_phy_w_cache[i].tmp_npo);
		}
	}
	PHY_ERR("ERR! Not get LSB write cache!\n");

	return NULL;
}

int nand_phy_low_page_write_cache_cancle(struct _nand_physic_op_par *npo)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if ((nand_phy_w_cache[i].cache_use_status == 1) &&
		    (nand_phy_w_cache[i].tmp_npo.chip == npo->chip) &&
		    (nand_phy_w_cache[i].tmp_npo.block == npo->block) &&
		    (nand_phy_w_cache[i].tmp_npo.page == npo->page)) {

			nand_phy_w_cache[i].cache_use_status = 0;

			return 0;
		}
	}
	PHY_ERR("ERR! Fail to cancle LSB write cache!\n");

	return -1;
}

int nand_phy_low_page_cache_get_for_read(struct _nand_physic_op_par *npo,
					 unsigned int two_plane)
{
	int i = 0;

	for (i = 0; i < NAND_OPEN_BLOCK_CNT; i++) {
		if ((nand_phy_w_cache[i].tmp_npo.chip == npo->chip) &&
		    (nand_phy_w_cache[i].tmp_npo.block == npo->block) &&
		    (nand_phy_w_cache[i].tmp_npo.page == npo->page) &&
		    (nand_phy_w_cache[i].tmp_npo.mdata != NULL) &&
		    (nand_phy_w_cache[i].tmp_npo.sdata != NULL)) {

			if (npo->mdata) {
				if (two_plane == 1)
					NAND_Memcpy(
					    npo->mdata,
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    2 * (npo->sect_bitmap << 9));
				else
					NAND_Memcpy(
					    npo->mdata,
					    nand_phy_w_cache[i].tmp_npo.mdata,
					    (npo->sect_bitmap << 9));
			}
			if (npo->sdata)
				NAND_Memcpy(npo->sdata,
					    nand_phy_w_cache[i].tmp_npo.sdata,
					    npo->slen);

			return 0;
		}
	}

	return -1;
}

int m0_rw_page(struct _nand_physic_op_par *npo, unsigned int function,
	       unsigned int two_plane)
{
	int ret = 0;

	if ((function == 0) && (two_plane == 0)) {
		ret |= m0_read_page(npo);
	} else if ((function == 0) && (two_plane == 1)) {
		ret |= m0_read_two_plane_page(npo);
	} else if ((function == 1) && (two_plane == 0)) {
		ret |= m0_write_page(npo);
	} else if ((function == 1) && (two_plane == 1)) {
		ret |= m0_write_two_plane_page(npo);
	} else {
		;
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
int m0_use_chip_function(struct _nand_physic_op_par *npo, unsigned int function)
{
	int ret, i;
	struct _nand_physic_op_par lnpo;
	struct _nand_chip_info *nci;
	struct _nand_super_chip_info *nsci =
	    nsci_get_from_nssi(g_nssi, npo->chip);
	unsigned int chip[8];
	unsigned int block[8];
	unsigned int block_num;

	if (nsci->two_plane == 0) {
		if ((nsci->vertical_interleave == 1) &&
		    (nsci->dual_channel == 1)) {
			block_num = 4;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block;

			chip[1] = nsci->d_channel_nci_2->chip_no;
			block[1] = npo->block;

			chip[2] = nsci->v_intl_nci_2->chip_no;
			block[2] = npo->block;

			nci = nci_get_from_nctri(nsci->d_channel_nci_2->nctri,
						 nsci->v_intl_nci_1->chip_no);
			chip[3] = nci->chip_no;
			block[3] = npo->block;
		} else if (nsci->vertical_interleave == 1) {
			block_num = 2;
			chip[0] = nsci->v_intl_nci_1->chip_no;
			block[0] = npo->block;

			chip[1] = nsci->v_intl_nci_2->chip_no;
			block[1] = npo->block;
		} else if (nsci->dual_channel == 1) {
			block_num = 2;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block;

			chip[1] = nsci->d_channel_nci_2->chip_no;
			block[1] = npo->block;
		} else {
			block_num = 1;
			chip[0] = npo->chip;
			block[0] = npo->block;
		}
	} else {
		if ((nsci->vertical_interleave == 1) &&
		    (nsci->dual_channel == 1)) {
			block_num = 8;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block << 1;
			chip[1] = nsci->d_channel_nci_1->chip_no;
			block[1] = (npo->block << 1) + 1;

			chip[2] = nsci->d_channel_nci_2->chip_no;
			block[2] = npo->block << 1;
			chip[3] = nsci->d_channel_nci_2->chip_no;
			block[3] = (npo->block << 1) + 1;

			chip[4] = nsci->v_intl_nci_2->chip_no;
			block[4] = npo->block << 1;
			chip[5] = nsci->v_intl_nci_2->chip_no;
			block[5] = (npo->block << 1) + 1;

			nci = nci_get_from_nctri(nsci->d_channel_nci_2->nctri,
						 nsci->v_intl_nci_1->chip_no);
			chip[6] = nci->chip_no;
			block[6] = npo->block << 1;
			chip[7] = nci->chip_no;
			block[7] = (npo->block << 1) + 1;
		} else if (nsci->vertical_interleave == 1) {
			block_num = 4;
			chip[0] = nsci->v_intl_nci_1->chip_no;
			block[0] = npo->block << 1;
			chip[1] = nsci->v_intl_nci_1->chip_no;
			block[1] = (npo->block << 1) + 1;

			chip[2] = nsci->v_intl_nci_2->chip_no;
			block[2] = npo->block << 1;
			chip[3] = nsci->v_intl_nci_2->chip_no;
			block[3] = (npo->block << 1) + 1;
		} else if (nsci->dual_channel == 1) {
			block_num = 4;
			chip[0] = nsci->d_channel_nci_1->chip_no;
			block[0] = npo->block << 1;
			chip[1] = nsci->d_channel_nci_1->chip_no;
			block[1] = (npo->block << 1) + 1;

			chip[2] = nsci->d_channel_nci_2->chip_no;
			block[2] = npo->block << 1;
			chip[3] = nsci->d_channel_nci_2->chip_no;
			block[3] = (npo->block << 1) + 1;
		} else {
			block_num = 2;
			chip[0] = npo->chip;
			block[0] = npo->block << 1;
			chip[1] = npo->chip;
			block[1] = (npo->block << 1) + 1;
		}
	}

	for (i = 0, ret = 0; i < block_num; i++) {
		lnpo.chip = chip[i];
		lnpo.block = block[i];
		lnpo.page = 0;
		if (function == 0) {
			ret |= m0_erase_block(&lnpo);
			// ret |= m0_erase_block_start(&lnpo);
		} else if (function == 1) {
			ret |= m0_bad_block_check(&lnpo);
			if (ret != 0) {
				break;
			}
		} else if (function == 2) {
			ret |= m0_bad_block_mark(&lnpo);
		} else {
			;
		}
	}

	nand_wait_all_rb_ready();

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_rw_use_chip_function(struct _nand_physic_op_par *npo,
			    unsigned int function)
{
	int ret = 0;
	struct _nand_physic_op_par lnpo;
	struct _nand_physic_op_par *low_npo;
	struct _nand_super_chip_info *nsci =
	    nsci_get_from_nssi(g_nssi, npo->chip);
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);

	unsigned int chip[4];
	unsigned int block[4];
	unsigned int page[4];
	unsigned char *mdata[4];
	unsigned char *sdata[4];
	unsigned int slen[4];
	unsigned int sect_bitmap[4];
	unsigned int block_num;
	unsigned char oob_temp[64];
	unsigned int i;

	if ((nsci->dual_channel == 1) && (nsci->two_plane == 1) &&
	    (function == 1)) {
		block_num = 4;
		chip[0] = nsci->d_channel_nci_1->chip_no;
		block[0] = npo->block << 1;
		page[0] = npo->page;
		mdata[0] = npo->mdata;
		sect_bitmap[0] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[0] = npo->sdata;
		slen[0] = npo->slen;

		chip[1] = nsci->d_channel_nci_2->chip_no;
		block[1] = npo->block << 1;
		page[1] = npo->page;
		mdata[1] = npo->mdata +
			   (nsci->d_channel_nci_1->sector_cnt_per_page << 10);
		sect_bitmap[1] = nsci->d_channel_nci_2->sector_cnt_per_page;
		sdata[1] = npo->sdata;
		slen[1] = npo->slen;

		chip[2] = nsci->d_channel_nci_1->chip_no;
		block[2] = (npo->block << 1) + 1;
		page[2] = npo->page;
		mdata[2] = npo->mdata +
			   (nsci->d_channel_nci_1->sector_cnt_per_page << 9);
		sect_bitmap[2] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[2] = npo->sdata;
		slen[2] = npo->slen;

		chip[3] = nsci->d_channel_nci_2->chip_no;
		block[3] = (npo->block << 1) + 1;
		page[3] = npo->page;
		mdata[3] = npo->mdata +
			   (nsci->d_channel_nci_2->sector_cnt_per_page << 10) +
			   (nsci->d_channel_nci_2->sector_cnt_per_page << 9);
		sect_bitmap[3] = nsci->d_channel_nci_2->sector_cnt_per_page;
		sdata[3] = npo->sdata;
		slen[3] = npo->slen;

		if (nsci->d_channel_nci_1->sector_cnt_per_page == 4) {
			for (i = 0; i < 16; i++) {
				if (i < 8) {
					oob_temp[i] =
					    *((unsigned char *)npo->sdata + i);
				} else if (i == 8) {
					oob_temp[i] = 0xff;
				} else {
					oob_temp[i] =
					    *((unsigned char *)npo->sdata + i -
					      1);
				}
			}
			sdata[0] = &oob_temp[0];
			sdata[1] = &oob_temp[0];
			sdata[2] = &oob_temp[8];
			sdata[3] = &oob_temp[8];
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= m0_write_page_start(&lnpo, 1);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		ret |= m0_write_page_start(&lnpo, 1);

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= m0_write_page_end(&lnpo);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[2];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		ret |= m0_write_page_start(&lnpo, 2);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		ret |= m0_write_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[3];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		ret |= m0_write_page_start(&lnpo, 2);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[2];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		ret |= m0_write_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[3];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		ret |= m0_write_page_end(&lnpo);

		return ret;
	}

	if ((nsci->dual_channel == 1) && (nsci->two_plane == 1) &&
	    (function == 0)) {
		block_num = 4;
		chip[0] = nsci->d_channel_nci_1->chip_no;
		block[0] = npo->block << 1;
		page[0] = npo->page;
		mdata[0] = npo->mdata;
		sect_bitmap[0] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[0] = npo->sdata;
		slen[0] = npo->slen;

		chip[1] = nsci->d_channel_nci_2->chip_no;
		block[1] = npo->block << 1;
		page[1] = npo->page;
		mdata[1] = npo->mdata +
			   (nsci->d_channel_nci_1->sector_cnt_per_page << 10);
		sect_bitmap[1] = sect_bitmap[0];
		sdata[1] = NULL;
		slen[1] = 0;

		chip[2] = nsci->d_channel_nci_1->chip_no;
		block[2] = (npo->block << 1) + 1;
		page[2] = npo->page;
		mdata[2] = npo->mdata +
			   (nsci->d_channel_nci_1->sector_cnt_per_page << 9);
		sect_bitmap[2] = sect_bitmap[0];
		sdata[2] = NULL;
		slen[2] = 0;

		chip[3] = nsci->d_channel_nci_2->chip_no;
		block[3] = (npo->block << 1) + 1;
		page[3] = npo->page;
		mdata[3] = npo->mdata +
			   (nsci->d_channel_nci_2->sector_cnt_per_page << 10) +
			   (nsci->d_channel_nci_2->sector_cnt_per_page << 9);
		sect_bitmap[3] = sect_bitmap[0];
		sdata[3] = NULL;
		slen[3] = 0;

		if (nsci->d_channel_nci_1->sector_cnt_per_page == 4) {
			sdata[0] = &oob_temp[0];
			sdata[1] = NULL;
			sdata[2] = &oob_temp[8];
			sdata[3] = NULL;
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= m0_read_page_start(&lnpo);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (mdata[0] != NULL)
			ret |= m0_read_page_start(&lnpo);

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		ret |= m0_read_page_end(&lnpo);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		if (mdata[0] != NULL)
			ret |= m0_read_page_start(&lnpo);

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (mdata[0] != NULL)
			ret |= m0_read_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		if (mdata[0] != NULL)
			ret |= m0_read_page_start(&lnpo);

		lnpo.chip = chip[2];
		lnpo.block = block[2];
		lnpo.page = page[2];
		lnpo.mdata = mdata[2];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[2];
		lnpo.slen = slen[2];
		if (mdata[0] != NULL)
			ret |= m0_read_page_end(&lnpo);

		lnpo.chip = chip[3];
		lnpo.block = block[3];
		lnpo.page = page[3];
		lnpo.mdata = mdata[3];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[3];
		lnpo.slen = slen[3];
		if (mdata[0] != NULL)
			ret |= m0_read_page_end(&lnpo);

		if (nsci->d_channel_nci_1->sector_cnt_per_page == 4) {
			for (i = 0; i < 16; i++) {
				if (i < 8) {
					*((unsigned char *)npo->sdata + i) =
					    oob_temp[i];
				} else if (i == 16) {
					*((unsigned char *)npo->sdata + i) =
					    0xff;
				} else {
					*((unsigned char *)npo->sdata + i) =
					    oob_temp[i + 1];
				}
			}
		}

		return ret;
	}

	if (nsci->dual_channel == 1) {
		block_num = 2;
		chip[0] = nsci->d_channel_nci_1->chip_no;
		block[0] = npo->block;
		page[0] = npo->page;
		mdata[0] = npo->mdata;
		sect_bitmap[0] = nsci->d_channel_nci_1->sector_cnt_per_page;
		sdata[0] = npo->sdata;
		slen[0] = npo->slen;

		chip[1] = nsci->d_channel_nci_2->chip_no;
		block[1] = npo->block;
		page[1] = npo->page;
		mdata[1] = npo->mdata +
			   (nsci->d_channel_nci_1->sector_cnt_per_page << 9);
		sect_bitmap[1] = sect_bitmap[0];
		sdata[1] = npo->sdata;
		slen[1] = npo->slen;
		if (function == 0) {
			sdata[1] = NULL;
			slen[1] = 0;
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		if (function == 1) {
			ret |= m0_write_page_start(&lnpo, 0);
		} else {
			ret |= m0_read_page_start(&lnpo);
		}

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (function == 1) {
			ret |= m0_write_page_start(&lnpo, 0);
		} else {
			if (mdata[0] != NULL)
				ret |= m0_read_page_start(&lnpo);
		}

		lnpo.chip = chip[0];
		lnpo.block = block[0];
		lnpo.page = page[0];
		lnpo.mdata = mdata[0];
		lnpo.sect_bitmap = sect_bitmap[0];
		lnpo.sdata = sdata[0];
		lnpo.slen = slen[0];
		if (function == 1) {
			ret |= m0_write_page_end(&lnpo);
		} else {
			ret |= m0_read_page_end(&lnpo);
		}

		lnpo.chip = chip[1];
		lnpo.block = block[1];
		lnpo.page = page[1];
		lnpo.mdata = mdata[1];
		lnpo.sect_bitmap = sect_bitmap[1];
		lnpo.sdata = sdata[1];
		lnpo.slen = slen[1];
		if (function == 1) {
			ret |= m0_write_page_end(&lnpo);
		} else {
			if (mdata[0] != NULL)
				ret |= m0_read_page_end(&lnpo);
		}

		return ret;
	}

	if (nsci->vertical_interleave == 1) {
		if (npo->page & 0x01) {
			lnpo.chip = nsci->v_intl_nci_2->chip_no;
		} else {
			lnpo.chip = nsci->v_intl_nci_1->chip_no;
		}
		lnpo.page = npo->page >> 1;
	} else {
		lnpo.chip = nsci->nci_first->chip_no;
		lnpo.page = npo->page;
	}
	lnpo.block = npo->block;
	lnpo.mdata = npo->mdata;
	lnpo.sect_bitmap = nsci->nci_first->sector_cnt_per_page;
	lnpo.sdata = npo->sdata;
	lnpo.slen = npo->slen;

	if (lnpo.mdata == NULL)
		lnpo.sect_bitmap = 0;

	if ((nci->npi->operation_opt & NAND_PAIRED_PAGE_SYNC) &&
	    (function == 1)) {
		if (nand_phy_get_page_type(lnpo.page) == 2) {
			ret = nand_phy_low_page_write_cache_set(
			    &lnpo, nsci->two_plane);
			return ret;
		} else if (nand_phy_get_page_type(lnpo.page) == 3) {
			low_npo = nand_phy_low_page_cache_get_for_write(&lnpo);

			if (low_npo) {
				ret |= m0_rw_page(low_npo, function,
						  nsci->two_plane);
				nand_phy_low_page_write_cache_cancle(low_npo);
			} else {
				PHY_ERR("ERR! Not get low page cache when "
					"write uper page\n");
			}
		} else {
			;
		}
	} else if ((nci->npi->operation_opt & NAND_PAIRED_PAGE_SYNC) &&
		   (function == 0)) {
		if ((nand_phy_get_page_type(lnpo.page) == 2) &&
		    (nand_phy_low_page_cache_get_for_read(
			 &lnpo, nsci->two_plane) == 0))
			return 0;
	} else {
		;
	}

	ret |= m0_rw_page(&lnpo, function, nsci->two_plane);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_erase_super_block(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = m0_use_chip_function(npo, 0);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_read_super_page(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = m0_rw_use_chip_function(npo, 0);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_write_super_page(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = m0_rw_use_chip_function(npo, 1);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_super_bad_block_check(struct _nand_physic_op_par *npo)
{

	int ret;

	ret = m0_use_chip_function(npo, 1);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_super_bad_block_mark(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = m0_use_chip_function(npo, 2);

	return ret;
}
