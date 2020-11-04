
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

#define _NC_C_
#include "../nand_physic_inc.h"

extern const u8 rand_factor_1st_spare_data[4][128];

/*****************************************************************************
*Name         : _dma_config_start
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
int ndfc_dma_config_start(struct _nand_controller_info *nctri, u8 rw,
			  void *buff_addr, u32 len)
{
	u32 reg_val;
	u32 config_addr = 0;
	int i, valid_data_dma_cnt;
	u32 data_dma_size_temp[8] = {0};

	NAND_CleanFlushDCacheRegion(buff_addr, len);

	if (nctri->dma_type == 1) {// MBUS DMA
		nctri->dma_addr =
		    (unsigned long)NAND_DMASingleMap(rw, buff_addr, len);
		if (nctri->dma_addr &
		    0x1f) { // zhouk for DMA addr must be 32B aligned
			PHY_ERR("ndfc_dma_config_start: buff addr(0x%x) is not "
				"32Bytes aligned",
				nctri->dma_addr);
		}

		/*BCH*/
		if (nctri->channel_sel == 0) {
			*nctri->nreg.reg_valid_data_dma_cnt = 1;
			*nctri->nreg.reg_data_dma_base = nctri->dma_addr;
			*nctri->nreg.reg_data_dma_size_2x = (len | (8 << 16));
			for (i = 1; i < 8; i++)
				*(nctri->nreg.reg_data_dma_size_2x + i) = (8 | (8 << 16));
		} else {
			/*LDPC*/
			valid_data_dma_cnt = len / 0x800;
			*nctri->nreg.reg_valid_data_dma_cnt = valid_data_dma_cnt;
			for (i = 0; i < 8; i++)
				*(nctri->nreg.reg_data_dma_size_2x + i) = (8 | (8 << 16));
			if (!(valid_data_dma_cnt %
			      2)) { // valid_data_dma_cnt is even
				for (i = 0; i < valid_data_dma_cnt;
				     i++) { // valid_data_dma_cnt
					*(nctri->nreg.reg_data_dma_base + i) = nctri->dma_addr + (i * 0x800);
					if (!(i % 2)) {
						data_dma_size_temp[i / 2] = 0x800 | (0x800 << 16);
						*(nctri->nreg.reg_data_dma_size_2x + (i / 2)) = data_dma_size_temp[i / 2];
					}
				}
			} else { // valid_data_dma_cnt is odd
				for (i = 0; i < valid_data_dma_cnt - 1;
				     i++) { // valid_data_dma_cnt-1
					*(nctri->nreg.reg_data_dma_base + i) =
					    nctri->dma_addr + (i * 0x800);
					if (!(i % 2)) {
						data_dma_size_temp[i / 2] =
						    0x800 | (0x800 << 16);
						*(nctri->nreg
						      .reg_data_dma_size_2x +
						  (i / 2)) =
						    data_dma_size_temp[i / 2];
					}
				} // for(i=0; i<valid_data_dma_cnt; i++)
				// The last DATA_DMA Configuration
				*(nctri->nreg.reg_data_dma_base +
				  (valid_data_dma_cnt - 1)) =
				    nctri->dma_addr +
				    ((valid_data_dma_cnt - 1) * 0x800);
				data_dma_size_temp[(valid_data_dma_cnt - 1) /
						   2] = 0x800 | (0x8 << 16);
				*(nctri->nreg.reg_data_dma_size_2x +
				  ((valid_data_dma_cnt - 1) / 2)) =
				    data_dma_size_temp[(valid_data_dma_cnt -
							1) /
						       2];
			}
		}
	} else if (nctri->dma_type == 0) {// General DMA
		reg_val = *nctri->nreg.reg_ctl;
		reg_val |= 0x1U << 15;
		reg_val |= 0x1U << 14;
		*nctri->nreg.reg_ctl = reg_val;
		*nctri->nreg.reg_dma_cnt = len;

		nctri->dma_addr =
		    (unsigned long)NAND_DMASingleMap(rw, buff_addr, len);
		nand_dma_config_start(rw, nctri->dma_addr, len);
	} else {
		PHY_ERR("_dma_config_start, wrong dma mode, %d\n",
			nctri->dma_type);
		return ERR_NO_51;
	}

	return 0;
}

/*****************************************************************************
*Name         : _wait_dma_end
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_wait_dma_end(struct _nand_controller_info *nctri, u8 rw,
		      void *buff_addr, u32 len)
{
	s32 ret = 0;

	while (!(*nctri->nreg.reg_sta) & 0x4) {
	}
	*nctri->nreg.reg_sta = 0x4; /* songwj for debug */
	/* NOTE: bu neng Or 0x04 Op */

	NAND_DMASingleUnmap(rw, (void *)(unsigned long)nctri->dma_addr, len);
	NAND_InvaildDCacheRegion(rw, nctri->dma_addr, len);

	return ret;
}

/*****************************************************************************
*Name         : ndfc_check_read_data_sta
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_check_read_data_sta(struct _nand_controller_info *nctri,
			     u32 eblock_cnt)
{
	s32 i, flag;

	// check all '1' or all '0' status

	if (*nctri->nreg.reg_data_pattern_sta ==
	    ((0x1U << (eblock_cnt - 1)) | ((0x1U << (eblock_cnt - 1)) - 1))) {
		flag = *nctri->nreg.reg_pat_id & 0x1;
		for (i = 1; i < eblock_cnt; i++) {
			if (flag != ((*nctri->nreg.reg_pat_id >> i) & 0x1))
				break;
		}

		if (i == eblock_cnt) {
			if (flag) {
				// PHY_DBG("read data sta, all '1'\n");
				return 4; // all input bits are '1'
			} else {
				// PHY_DBG("read data sta, all '0'\n");
				return 3; // all input bit are '0'
			}
		} else {
			PHY_DBG("read data sta, some ecc blocks are all '1', "
				"others are all '0' %x %x\n",
				*nctri->nreg.reg_ecc_sta,
				*nctri->nreg.reg_pat_id);
			return 2;
		}
	} else if (*nctri->nreg.reg_data_pattern_sta == 0x0) {
		return 0;
	} else {
		// PHY_DBG("!!!!read data sta, only some ecc blocks are all '1'
		// or all '1', 0x%x 0x%x\n", *nctri->nreg.reg_ecc_sta,
		// *nctri->nreg.reg_pat_id);
		return 1;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
extern int is_nouse_page(u8 *buf);

s32 ndfc_get_bad_blk_flag(u32 page_no, u32 dis_random, u32 slen, u8 *sbuf)
{
	u8 _1st_spare_byte[16];
	u32 i, len, num;

	num = page_no % 128;
	len = slen;

	if (len > 16)
		len = 16;

	// PHY_DBG("ndfc get_bad_blk_flag, %d %d %d 0x%x\n", page_no,
	// dis_random, slen, sbuf);
	if (sbuf == NULL) {
		PHY_ERR("ndfc get_bad_blk_flag, input parameter error!\n");
		return ERR_NO_50;
	}

	for (i = 0; i < 16; i++) {
		_1st_spare_byte[i] = 0xff;
	}

	if (dis_random) {
		//  check some byte in spare data
		len = sizeof(rand_factor_1st_spare_data) /
		      sizeof(rand_factor_1st_spare_data[0]);
		for (i = 0; i < len; i++) {
			_1st_spare_byte[i] =
			    sbuf[i] ^ rand_factor_1st_spare_data[i % 4][num];
		}
	} else {
		for (i = 0; i < len; i++) {
			_1st_spare_byte[i] = sbuf[i];
		}
	}

	// if (_1st_spare_byte[0] == 0xff)
	if (is_nouse_page(_1st_spare_byte) == 1) {
		// PHY_DBG("good page flag, page %d\n", page_no);
		//		if((page_no != 0)&&(page_no != 255))
		//		{
		//		    PHY_DBG("blank page:%d\n",page_no);
		//		}

		return 0;
	} else {
		// PHY_DBG("bad page flag, page %d\n", page_no);
		return ERR_NO_49;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 ndfc_check_ecc(struct _nand_controller_info *nctri, u32 eblock_cnt)
{
	u32 i, ecc_limit, max_ecc_bit_cnt, cfg;
	u32 ecc_cnt_w[8];
	u8 ecc_cnt;
	u8 ecc_tab[16] = {16, 24, 28, 32, 40, 44, 48, 52,
			  56, 60, 64, 68, 72, 76, 80};
	u8 ecc_limit_tab[16] = {13, 20, 23, 27, 35, 39, 42, 46,
				50, 54, 58, 62, 66, 68, 72};

	max_ecc_bit_cnt = ecc_tab[(*nctri->nreg.reg_ecc_ctl >> 8) & 0xff];
	ecc_limit = ecc_limit_tab[(*nctri->nreg.reg_ecc_ctl >> 8) & 0xff];

	// check ecc errors
	// cfg = (1<<eblock_cnt) - 1;
	if (nctri->channel_sel == 0) // BCH
		cfg = (1 << (eblock_cnt - 1)) | ((1 << (eblock_cnt - 1)) - 1);
	else // LDPC
		cfg = (1 << (eblock_cnt / 2 - 1)) |
		      ((1 << (eblock_cnt / 2 - 1)) - 1);
	// PHY_ERR("check ecc: %d 0x%x 0x%x\n", nctri->nci->randomizer,
	// *nctri->nreg.reg_ecc_ctl, *nctri->nreg.reg_ecc_sta);
	if ((*nctri->nreg.reg_ecc_sta & cfg) != 0) {
		return ERR_ECC;
	}

	// check ecc limit
	ecc_cnt_w[0] = *nctri->nreg.reg_err_cnt0;
	ecc_cnt_w[1] = *nctri->nreg.reg_err_cnt1;
	ecc_cnt_w[2] = *nctri->nreg.reg_err_cnt2;
	ecc_cnt_w[3] = *nctri->nreg.reg_err_cnt3;
	ecc_cnt_w[4] = *nctri->nreg.reg_err_cnt4;
	ecc_cnt_w[5] = *nctri->nreg.reg_err_cnt5;
	ecc_cnt_w[6] = *nctri->nreg.reg_err_cnt6;
	ecc_cnt_w[7] = *nctri->nreg.reg_err_cnt7;

	if (nctri->channel_sel == 0) {
		for (i = 0; i < eblock_cnt; i++) {
			ecc_cnt = (u8)(ecc_cnt_w[i >> 2] >> ((i % 4) << 3));

			if (ecc_cnt > ecc_limit) {
				// PHY_DBG("ecc limit: 0x%x 0x%x 0x%x 0x%x   %d
				// %d  %d\n",
				// ecc_cnt_w[0],ecc_cnt_w[1],ecc_cnt_w[2],ecc_cnt_w[3],ecc_cnt,ecc_limit,max_ecc_bit_cnt);
				return ECC_LIMIT;
			}
		}
	} else {
		for (i = 0; i < eblock_cnt; i += 2) {
			if (i % 4 == 0)
				ecc_cnt = (u16)(ecc_cnt_w[i >> 2]);
			else
				ecc_cnt = (u16)(ecc_cnt_w[i >> 2] >> 16);

			if (ecc_cnt > 256) { // LDPC most can fix 511 err bits
				// PHY_DBG("ecc limit : ecc_cnt_w[%d] = 0x%x\n",
				// (i>>2), ecc_cnt_w[i>>2],);
				return ECC_LIMIT;
			}
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
s32 ndfc_update_ecc_sta_and_spare_data(struct _nand_physic_op_par *npo,
				       s32 ecc_sta, unsigned char *sbuf)
{
	unsigned char *buf;
	unsigned int len;
	s32 ret_ecc_sta, ret1 = 0;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);

	if (npo->slen != 0) {
		buf = npo->sdata;
		len = npo->slen;
	} else {
		buf = sbuf;
		len = nci->sdata_bytes_per_page;
	}

	if (npo->slen > nci->sdata_bytes_per_page) {
		len = nci->sdata_bytes_per_page;
	}

	// check input data status
	ret1 = ndfc_check_read_data_sta(
	    nci->nctri, nci->sector_cnt_per_page >> nci->ecc_sector);

	ret_ecc_sta = ecc_sta;

	if (nci->randomizer) {
		if (ecc_sta == ERR_ECC) {
			// PHY_DBG("update ecc, %d %d %d 0x%x\n", npo->page,
			// nci->randomizer, npo->slen, npo->sdata);
			if (ndfc_get_bad_blk_flag(npo->page, nci->randomizer,
						  len, buf) == 0) {
				// when the 1st byte of spare data is 0xff after
				// randomization, this page is blank
				// PHY_DBG("randomizer blank page\n");
				MEMSET(buf, 0xff, len);
				ret_ecc_sta = 0;
			}
		}

		if (ret1 == 3) {
			// all data bits are '0', no ecc error, this page is a
			// bad page
			// PHY_DBG("randomizer all bit 0\n");
			MEMSET(buf, 0x0, len);
			ret_ecc_sta = ERR_ECC;
		}
	} else {
		if (ret1 == 3) {
			// all data bits are '0', don't do ecc(ecc exception),
			// this page is a bad page
			// PHY_DBG("no randomizer all bit 0\n");
			ret_ecc_sta = ERR_ECC;
		} else if (ret1 == 4) {
			// all data bits are '1', don't do ecc(ecc exception),
			// this page is a good page
			// PHY_DBG("no randomizer all bit 1\n");
			ret_ecc_sta = 0;
		}
	}

	return ret_ecc_sta;
}
