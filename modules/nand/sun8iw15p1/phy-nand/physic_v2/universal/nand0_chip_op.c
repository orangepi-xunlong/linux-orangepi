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

#define _NCO0_C_

#include "../nand_physic_inc.h"

#define ALIGN_SIZE(s, a) (((s) + ((a)-1)) & (~((a)-1)))
/* read_mapping_page will be set during reading mapping data @ read_mapping_page */
extern int read_mapping_page;
int (*function_read_page_end)(struct _nand_physic_op_par *npo) = NULL;

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_erase_block_start(struct _nand_physic_op_par *npo)
{
	int ret;
	unsigned int row_addr = 0, col_addr = 0;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nand_controller_info *nctri = nci->nctri;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	// PHY_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d \n", __func__,
	// nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block,
	// nci->blk_cnt_per_chip);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) ||
	    (npo->block >= nci->blk_cnt_per_chip)) {
		PHY_ERR("fatal err -0, wrong input parameter, ch: %d  chip: "
			"%d/%d  block: %d/%d \n",
			nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
			npo->block, nci->blk_cnt_per_chip);
		return ERR_NO_10;
	}
	// wait nand ready before erase
	nand_read_chip_status_ready(nci);

	nand_enable_chip(nci);

	ndfc_clean_cmd_seq(cmd_seq);

	// cmd1: 0x60
	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd = CMD_ERASE_CMD1;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	row_addr =
	    get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 2;
		fill_cmd_addr(col_addr, 0, row_addr, 2,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 3;
		fill_cmd_addr(col_addr, 0, row_addr, 3,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	}

	// cmd2: 0xD0
	cmd_seq->nctri_cmd[1].cmd = CMD_ERASE_CMD2;
	cmd_seq->nctri_cmd[1].cmd_valid = 1;
	cmd_seq->nctri_cmd[1].cmd_send = 1;
	cmd_seq->nctri_cmd[1].cmd_wait_rb = 0;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);

	nand_disable_chip(nci);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_erase_block(struct _nand_physic_op_par *npo)
{
	int ret;
	// unsigned char status;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);

	ret = m0_erase_block_start(npo);
	if (ret != 0) {
		PHY_ERR("erase_block wrong1\n");
		return ret;
	}

	ret = nand_read_chip_status_ready(nci);
	if (ret != 0) {
		PHY_ERR("erase_block wrong2\n");
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
int m0_read_page_start(struct _nand_physic_op_par *npo)
{
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;
	u32 def_spare[32];
	int ret;
	u32 ecc_block, full_bitmap = 0;

	ecc_block = nci->sector_cnt_per_page / 2;
	full_bitmap = ((unsigned int)1 << (ecc_block - 1)) |
		      (((unsigned int)1 << (ecc_block - 1)) - 1);

	// wait nand ready before read
	nand_read_chip_status_ready(nci);

	// set ecc mode & randomize
	ndfc_set_ecc_mode(nci->nctri, nci->ecc_mode);
	ndfc_enable_ecc(nci->nctri, 1, nci->randomizer);
	if (nci->randomizer) {
		ndfc_set_rand_seed(nci->nctri, npo->page);
		ndfc_enable_randomize(nci->nctri);
	}

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	// command
	set_default_batch_read_cmd_seq(cmd_seq);
	nci->nctri->random_addr_num = nci->random_addr_num;
	nci->nctri->random_cmd2_send_flag = nci->random_cmd2_send_flag;

	// address
	row_addr =
	    get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 4;
		fill_cmd_addr(col_addr, 2, row_addr, 2,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 5;
		fill_cmd_addr(col_addr, 2, row_addr, 3,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	}

	// data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	if (npo->mdata != NULL) {
		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	} else {
		// don't swap main data with host memory
		cmd_seq->nctri_cmd[0].cmd_swap_data = 0;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	}
	cmd_seq->nctri_cmd[0].cmd_direction = 0; // read
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	/* optimize for reading mapping data during second_scan_all_blocks */
	if (!read_mapping_page) {
		cmd_seq->nctri_cmd[0].cmd_mdata_len = nci->sector_cnt_per_page << 9;
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = full_bitmap;
	} else {
		cmd_seq->nctri_cmd[0].cmd_mdata_len = ALIGN_SIZE(g_nssi->nsci->page_cnt_per_super_blk << 2, 1 << 10);
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = (1 << (cmd_seq->nctri_cmd[0].cmd_mdata_len >> 10)) - 1;
	}

	if ((npo->mdata == NULL) && (npo->sdata != NULL) &&
	    (npo->sect_bitmap == 0)) {
		if (nci->sector_cnt_per_page > 4) {
			cmd_seq->nctri_cmd[0].cmd_mdata_len = 2 << 9; /* 12 byte spare data in 1K */
			cmd_seq->nctri_cmd[0].cmd_data_block_mask = 0x1;
		} else {
			cmd_seq->nctri_cmd[0].cmd_mdata_len = 4 << 9;
			cmd_seq->nctri_cmd[0].cmd_data_block_mask = 0x3;
		}
	}
	ndfc_set_user_data_len_cfg(nci->nctri, nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);
	MEMSET(def_spare, 0x99, nci->sdata_bytes_per_page);
	ndfc_set_spare_data(nci->nctri, (u8 *)def_spare, nci->sdata_bytes_per_page);
	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("read page start, batch cmd io send error:chip:%d "
			"block:%d page:%d sect_bitmap:%d mdata:%x slen:%d: !\n",
			npo->chip, npo->block, npo->page, npo->sect_bitmap,
			npo->mdata, npo->slen);

		nand_disable_chip(nci);

		return ret;
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
int m0_read_page_end_not_retry(struct _nand_physic_op_par *npo)
{
	s32 ecc_sta = 0, ret = 0;
	uchar spare[64];
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("read page end, batch cmd io wait error:chip:%d "
			"block:%d page:%d sect_bitmap:%d mdata:%x slen:%d: !\n",
			npo->chip, npo->block, npo->page, npo->sect_bitmap,
			npo->mdata, npo->slen);
		goto ERROR;
	}

	// check ecc
	ecc_sta = ndfc_check_ecc(nci->nctri,
				 nci->sector_cnt_per_page >> nci->ecc_sector);
	// get spare data
	ndfc_get_spare_data(nci->nctri, (u8 *)spare, nci->sdata_bytes_per_page);

	if (npo->slen != 0) {
		MEMCPY(npo->sdata, spare, npo->slen);
	}
	// update ecc status and spare data
	// PHY_DBG("npo: 0x%x %d 0x%x\n",npo, npo->slen, npo->sdata);
	ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, spare);

ERROR:
	// disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);
	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}
	nand_disable_chip(nci);

	return ret; // ecc status
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_read_page_end(struct _nand_physic_op_par *npo)
{
	int ret;

	ret = function_read_page_end(npo);

	if (ret == ECC_LIMIT) {
		PHY_DBG("read page ecc limit,chip=%d block=%d page=%d\n",
			npo->chip, npo->block, npo->page);
	} else if (ret != 0) {
		PHY_ERR("ecc err!read page, read page end error %d,chip=%d "
			"block=%d page=%d\n",
			ret, npo->chip, npo->block, npo->page);
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
int m0_read_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nand_controller_info *nctri = nci->nctri;

	// PHY_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d  page: %d/%d\n",
	// __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
	// npo->block, nci->blk_cnt_per_chip, npo->page, nci->page_cnt_per_blk);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) ||
	    (npo->block >= nci->blk_cnt_per_chip) ||
	    (npo->page >= nci->page_cnt_per_blk)) {
		PHY_ERR("fatal err -0, wrong input parameter, ch: %d  chip: "
			"%d/%d  block: %d/%d  page: %d/%d\n",
			nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
			npo->block, nci->blk_cnt_per_chip, npo->page,
			nci->page_cnt_per_blk);
		return ERR_NO_11;
	}

	if ((npo->mdata == NULL) && (npo->sdata == NULL) &&
	    (npo->sect_bitmap)) {
		PHY_ERR("fatal err -1, wrong input parameter, mdata: 0x%08x  "
			"sdata: 0x%08x  sect_bitmap: 0x%x\n",
			npo->mdata, npo->sdata, npo->sect_bitmap);
		return ERR_NO_12;
	}

	if ((npo->mdata == NULL) && (npo->sdata == NULL) &&
	    (npo->sect_bitmap == 0)) {
		// PHY_DBG("warning -0, mdata: 0x%08x  sdata: 0x%08x
		// sect_bitmap: 0x%x\n",npo->mdata, npo->sdata,
		// npo->sect_bitmap);
		return 0;
	}

	ret = m0_read_page_start(npo);
	if (ret) {
		PHY_ERR("read page, read page start error %d\n", ret);
		return ret;
	}

	ret = m0_read_page_end(npo);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_write_page_start(struct _nand_physic_op_par *npo, int plane_no)
{
	uchar spare[64];
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;
	int dummy_byte;
	unsigned int ecc_block, full_bitmap = 0;
	int ret;

	if (npo->mdata == NULL) {
		PHY_ERR("write page start, input parameter error!\n");
		return ERR_NO_13;
	}

	ecc_block = nci->sector_cnt_per_page / 2;
	full_bitmap = ((unsigned int)1 << (ecc_block - 1)) |
		      (((unsigned int)1 << (ecc_block - 1)) - 1);
	if ((plane_no == 0) || (plane_no == 1))
		// wait nand ready before write
		nand_read_chip_status_ready(nci);
	if ((nci->driver_no == 1) && (m1_read_retry_mode == 0x4) &&
	    (nci->retry_count != 0)) {
		m1_setdefaultparam(nci);
		nci->retry_count = 0;
	}

	//    PHY_DBG("chip:%d block:%d page: %d buf:0x%x%x spare:0x%x%x%x
	//    \n",npo->chip,npo->block,npo->page,npo->mdata[0],npo->mdata[1],npo->sdata[2],npo->sdata[3],npo->sdata[4]);

	//    if((nci->driver_no == 2)&&((0x2 == m2_read_retry_mode)||(0x3 ==
	//    m2_read_retry_mode))&&(nci->retry_count != 0))
	//    {
	//        m2_setdefaultparam(nci);
	//        nci->retry_count = 0;
	//	}

	// get spare data
	MEMSET(spare, 0xff, 64);
	if (npo->slen != 0) {
		MEMCPY(spare, npo->sdata, npo->slen);
	}

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	ndfc_set_spare_data(nci->nctri, (u8 *)spare, nci->sdata_bytes_per_page);

	// set ecc mode & randomize
	if (nci->randomizer) {
		ndfc_set_rand_seed(nci->nctri, npo->page);
		ndfc_enable_randomize(nci->nctri);
	}
	ndfc_set_ecc_mode(nci->nctri, nci->ecc_mode);
	ndfc_enable_ecc(nci->nctri, 1, nci->randomizer);

	nci->nctri->current_op_type = 1;
	dummy_byte = get_dummy_byte(nci->nand_real_page_size, nci->ecc_mode,
				    nci->sector_cnt_per_page / 2,
				    nci->sdata_bytes_per_page);
	if (dummy_byte > 0) {
		ndfc_set_dummy_byte(nci->nctri, dummy_byte);
		ndfc_enable_dummy_byte(nci->nctri);
	}

	nci->nctri->random_addr_num = nci->random_addr_num;
	nci->nctri->random_cmd2_send_flag = nci->random_cmd2_send_flag;
	if (nci->random_cmd2_send_flag) {
		nci->nctri->random_cmd2 = get_random_cmd2(npo);
	}

	// command
	if (plane_no == 0) {
		set_default_batch_write_cmd_seq(cmd_seq, CMD_WRITE_PAGE_CMD1,
						CMD_WRITE_PAGE_CMD2);
	} else if (plane_no == 1) {
		// set_default_batch_write_cmd_seq(cmd_seq,CMD_WRITE_PAGE_CMD1,0x11);
		set_default_batch_write_cmd_seq(
		    cmd_seq, CMD_WRITE_PAGE_CMD1,
		    nci->opt_phy_op_par->multi_plane_write_cmd[0]);
		// set_default_batch_write_cmd_seq(cmd_seq,CMD_WRITE_PAGE_CMD1,CMD_WRITE_PAGE_CMD2);
	} else {
		// set_default_batch_write_cmd_seq(cmd_seq,0x81,CMD_WRITE_PAGE_CMD2);
		set_default_batch_write_cmd_seq(
		    cmd_seq, nci->opt_phy_op_par->multi_plane_write_cmd[1],
		    CMD_WRITE_PAGE_CMD2);
		// set_default_batch_write_cmd_seq(cmd_seq,CMD_WRITE_PAGE_CMD1,CMD_WRITE_PAGE_CMD2);
	}

	// address
	row_addr =
	    get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 4;
		fill_cmd_addr(col_addr, 2, row_addr, 2,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 5;
		fill_cmd_addr(col_addr, 2, row_addr, 3,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	}

	// data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	cmd_seq->nctri_cmd[0].cmd_direction = 1; // write
	//   cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	cmd_seq->nctri_cmd[0].cmd_data_block_mask = full_bitmap;
	cmd_seq->nctri_cmd[0].cmd_mdata_len = nci->sector_cnt_per_page << 9;

	ndfc_set_user_data_len_cfg(nci->nctri, nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);

	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("read2 page start, batch cmd io send error:chip:%d "
			"block:%d page:%d sect_bitmap:%d mdata:%x slen:%d: !\n",
			npo->chip, npo->block, npo->page, npo->sect_bitmap,
			npo->mdata, npo->slen);
		nand_disable_chip(nci);
		return ret;
	}

	return 0;
}

int m0_write_page_end(struct _nand_physic_op_par *npo)
{
	s32 ret = 0;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("write page end, batch cmd io wait error:chip:%d "
			"block:%d page:%d sect_bitmap:%d mdata:%x slen:%d: !\n",
			npo->chip, npo->block, npo->page, npo->sect_bitmap,
			npo->mdata, npo->slen);
	}

	// disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);
	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}

	ndfc_set_dummy_byte(nci->nctri, 0);
	ndfc_disable_dummy_byte(nci->nctri);

	nci->nctri->random_cmd2_send_flag = 0;
	nci->nctri->current_op_type = 0;

	nand_disable_chip(nci);
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_write_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nand_controller_info *nctri = nci->nctri;

	// PHY_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d  page: %d/%d\n",
	// __func__, nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
	// npo->block, nci->blk_cnt_per_chip, npo->page, nci->page_cnt_per_blk);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) ||
	    (npo->block >= nci->blk_cnt_per_chip) ||
	    (npo->page >= nci->page_cnt_per_blk)) {
		PHY_ERR("wfatal err -0, wrong input parameter, ch: %d  chip: "
			"%d/%d  block: %d/%d  page: %d/%d\n",
			nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
			npo->block, nci->blk_cnt_per_chip, npo->page,
			nci->page_cnt_per_blk);
		return ERR_NO_14;
	}

	if (npo->sect_bitmap != nci->sector_cnt_per_page) {
		PHY_ERR("wFatal err -2, wrong input parameter, unaligned write "
			"page SectBitmap: 0x%x/0x%x\n",
			npo->sect_bitmap, nci->sector_cnt_per_page);
		return ERR_NO_15;
	}

	ret = m0_write_page_start(npo, 0);
	if (ret) {
		PHY_ERR("write page, write page start error %d\n", ret);
		return ret;
	}

	ret = m0_write_page_end(npo);
	if (ret) {
		PHY_ERR("write page, write page end error %d\n", ret);
		return ret;
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
int m0_read_two_plane_page_start(struct _nand_physic_op_par *npo,
				 struct _nand_physic_op_par *npo2)
{
	int ret;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;

	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd =
	    nci->opt_phy_op_par->multi_plane_read_cmd[0];
	cmd_seq->nctri_cmd[0].cmd_send = 1;
	row_addr =
	    get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 2;
		fill_cmd_addr(col_addr, 0, row_addr, 2,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[0].cmd_acnt = 3;
		fill_cmd_addr(col_addr, 0, row_addr, 3,
			      cmd_seq->nctri_cmd[0].cmd_addr);
	}

	cmd_seq->nctri_cmd[1].cmd_valid = 1;
	cmd_seq->nctri_cmd[1].cmd =
	    nci->opt_phy_op_par->multi_plane_read_cmd[0];
	cmd_seq->nctri_cmd[1].cmd_send = 1;
	row_addr = get_row_addr(nci->page_offset_for_next_blk, npo2->block,
				npo2->page);
	if (nci->npi->operation_opt & NAND_WITH_TWO_ROW_ADR) {
		cmd_seq->nctri_cmd[1].cmd_acnt = 2;
		fill_cmd_addr(col_addr, 0, row_addr, 2,
			      cmd_seq->nctri_cmd[1].cmd_addr);
	} else {
		cmd_seq->nctri_cmd[1].cmd_acnt = 3;
		fill_cmd_addr(col_addr, 0, row_addr, 3,
			      cmd_seq->nctri_cmd[1].cmd_addr);
	}

	cmd_seq->nctri_cmd[2].cmd_valid = 1;
	cmd_seq->nctri_cmd[2].cmd =
	    nci->opt_phy_op_par->multi_plane_read_cmd[1];
	cmd_seq->nctri_cmd[2].cmd_send = 1;
	cmd_seq->nctri_cmd[2].cmd_wait_rb = 1;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("read_two_plane_page_start failed!\n");
	}

	nand_disable_chip(nci);

	return ret;
}

int m0_read_two_plane_page_end(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	//    int ecc_sta = 0;
	//    struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	//    struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	//    u32 row_addr = 0, col_addr = 0;
	//    u32 def_spare[32];
	//
	//    nand_enable_chip(nci);
	//
	//    //set ecc mode & randomize
	//    ndfc_set_ecc_mode(nci->nctri, nci->ecc_mode);
	//    ndfc_enable_ecc(nci->nctri, 1, nci->randomizer);
	//    if (nci->randomizer)
	//    {
	//    	ndfc_set_rand_seed(nci->nctri, npo->page);
	//    	ndfc_enable_randomize(nci->nctri);
	//    }
	//
	//    ndfc_clean_cmd_seq(cmd_seq);
	//    cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	//
	//    cmd_seq->nctri_cmd[0].cmd_valid = 1;
	//    cmd_seq->nctri_cmd[0].cmd = 0x00;
	//    cmd_seq->nctri_cmd[0].cmd_send = 1;
	//    row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block,
	//    npo->page);
	//    cmd_seq->nctri_cmd[0].cmd_acnt = 5;
	//    fill_cmd_addr(col_addr, 2, row_addr, 3,
	//    cmd_seq->nctri_cmd[0].cmd_addr);
	//    ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	//	if (ret)
	//    {
	//		PHY_ERR("read_two_plane_page_start failed!\n");
	//		nand_disable_chip(nci);
	//		return ret;
	//	}
	//
	//    //command
	//    ndfc_clean_cmd_seq(cmd_seq);
	//
	//	cmd_seq->cmd_type = CMD_TYPE_BATCH;
	//	cmd_seq->ecc_layout = ECC_LAYOUT_INTERLEAVE;
	//
	//	cmd_seq->nctri_cmd[0].cmd = 0x05;
	//	cmd_seq->nctri_cmd[1].cmd = 0xe0;
	//	cmd_seq->nctri_cmd[2].cmd = 0x05;
	//	cmd_seq->nctri_cmd[3].cmd = 0xe0;
	//
	//    //address
	//    row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block,
	//    npo->page);
	//    cmd_seq->nctri_cmd[0].cmd_acnt = 2;
	//    fill_cmd_addr(col_addr, 2, row_addr, 0,
	//    cmd_seq->nctri_cmd[0].cmd_addr);
	//
	//    //data
	//	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	//	if (npo->mdata != NULL)
	//	{
	//		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	//		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	//	}
	//	else
	//    {
	//		//don't swap main data with host memory
	//		cmd_seq->nctri_cmd[0].cmd_swap_data = 0;
	//		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	//	}
	//	cmd_seq->nctri_cmd[0].cmd_direction = 0; //read
	//	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	//	cmd_seq->nctri_cmd[0].cmd_mdata_len = nci->sector_cnt_per_page
	//<< 9;
	//
	//	MEMSET(def_spare, 0x99, 128);
	//	ndfc_set_spare_data(nci->nctri, (u32*)def_spare,
	//MAX_ECC_BLK_CNT);
	//	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	//	if(ret)
	//	{
	//		PHY_ERR("read3 page start, batch cmd io send error:chip:%d
	//block:%d page:%d sect_bitmap:%d mdata:%x slen:%d:
	//!\n",npo->chip,npo->block,npo->page,npo->sect_bitmap,npo->mdata,npo->slen);
	//		nand_disable_chip(nci);
	//		return ret;
	//	}
	//    ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	//	if (ret)
	//	{
	//		PHY_ERR("read2 page end, batch cmd io wait error:chip:%d
	//block:%d page:%d sect_bitmap:%d mdata:%x slen:%d:
	//!\n",npo->chip,npo->block,npo->page,npo->sect_bitmap,npo->mdata,npo->slen);
	//	}
	//
	//	//check ecc
	//	ecc_sta = ndfc_check_ecc(nci->nctri,
	//nci->sector_cnt_per_page>>nci->ecc_sector);
	//	//get spare data
	//	ndfc_get_spare_data(nci->nctri, (u32*)def_spare,
	//nci->sector_cnt_per_page>>nci->ecc_sector);
	//
	//	if (npo->slen != 0)
	//	{
	//		MEMCPY(npo->sdata, def_spare, npo->slen);
	//	}
	//    //update ecc status and spare data
	//    //PHY_DBG("npo: 0x%x %d 0x%x\n",npo, npo->slen, npo->sdata);
	//    ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, def_spare);
	//
	//    //disable ecc mode & randomize
	//    ndfc_disable_ecc(nci->nctri);
	//    if (nci->randomizer)
	//    {
	//    	ndfc_disable_randomize(nci->nctri);
	//    }
	//
	//    nand_disable_chip(nci);

	return ret;
}

int m0_read_two_plane_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	int ret0, ret1 = 0;
	uchar spare[64];
	int i;
	struct _nand_physic_op_par npo1;
	struct _nand_physic_op_par npo2;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	// struct _nctri_cmd_seq* cmd_seq = &nci->nctri->nctri_cmd_seq;

	npo1.chip = npo->chip;
	npo1.block = (npo->block << 1);
	npo1.page = npo->page;
	npo1.sect_bitmap = nci->sector_cnt_per_page;
	npo1.mdata = npo->mdata;
	npo1.sdata = npo->sdata;
	npo1.slen = npo->slen;
	if ((npo1.slen == 0) || (npo1.sdata == NULL)) {
		npo1.sdata = spare;
		npo1.slen = nci->sdata_bytes_per_page;
	}

	npo2.chip = npo->chip;
	npo2.block = (npo->block << 1) + 1;
	npo2.page = npo->page;
	npo2.sect_bitmap = nci->sector_cnt_per_page;
	if (npo->mdata != NULL) {
		npo2.mdata = npo->mdata + (nci->sector_cnt_per_page << 9);
	} else {
		npo2.mdata = NULL;
	}
	npo2.sdata = spare;
	npo2.slen = npo->slen;
	if ((npo2.slen == 0) || (npo2.sdata == NULL)) {
		npo2.sdata = spare;
		npo2.slen = nci->sdata_bytes_per_page;
	}

	if ((npo->mdata == NULL) && (npo->sect_bitmap == 0)) {
		npo1.sect_bitmap = 0;
		npo2.sect_bitmap = 0;
		npo2.sdata = NULL;
		npo2.slen = 0;
	}

	if (nci->sector_cnt_per_page == 4) {
		npo1.sdata = spare;
		npo2.sdata = &spare[8];
		npo2.slen = nci->sdata_bytes_per_page;
	}

	//    ret |= m0_read_two_plane_page_start(&npo1,&npo2);
	//    ret |= m0_read_two_plane_page_end(&npo1);
	//    ret |= m0_read_two_plane_page_end(&npo2);

	ret0 = m0_read_page(&npo1);
	/* optimize for reading mapping data during second_scan_all_blocks */
	if (!read_mapping_page)
		ret1 = m0_read_page(&npo2);
	else if ((g_nssi->nsci->page_cnt_per_super_blk << 2) > (nci->sector_cnt_per_page << 9))
		ret1 = m0_read_page(&npo2);

	if (nci->sector_cnt_per_page == 4) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				*((unsigned char *)npo->sdata + i) = spare[i];
			} else if (i == 16) {
				*((unsigned char *)npo->sdata + i) = 0xff;
			} else {
				*((unsigned char *)npo->sdata + i) =
				    spare[i + 1];
			}
		}
	}

	if ((ret0 == ERR_ECC) || (ret1 == ERR_ECC))
		ret = ERR_ECC;
	else if ((ret0 == ECC_LIMIT) || (ret1 == ECC_LIMIT))
		ret = ECC_LIMIT;
	else
		ret = ret0 | ret1;

	return ret;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m0_write_two_plane_page(struct _nand_physic_op_par *npo)
{
	int ret = 0;
	uchar spare[64];
	int i;
	struct _nand_physic_op_par npo1;
	struct _nand_physic_op_par npo2;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	// struct _nctri_cmd_seq* cmd_seq = &nci->nctri->nctri_cmd_seq;

	npo1.chip = npo->chip;
	npo1.block = npo->block << 1;
	npo1.page = npo->page;
	npo1.sect_bitmap = nci->sector_cnt_per_page;
	npo1.mdata = npo->mdata;
	npo1.sdata = npo->sdata;
	npo1.slen = npo->slen;

	npo2.chip = npo->chip;
	npo2.block = (npo->block << 1) + 1;
	npo2.page = npo->page;
	npo2.sect_bitmap = nci->sector_cnt_per_page;
	npo2.mdata = npo->mdata + (nci->sector_cnt_per_page << 9);
	npo2.sdata = npo->sdata;
	npo2.slen = npo->slen;

	if (nci->sector_cnt_per_page == 4) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				spare[i] = *((unsigned char *)npo->sdata + i);
			} else if (i == 8) {
				spare[i] = 0xff;
			} else {
				spare[i] =
				    *((unsigned char *)npo->sdata + i - 1);
			}
		}
		npo1.sdata = spare;
		npo2.sdata = &spare[8];
	}
	//    if(nci->driver_no == 2)
	//    {
	//        ret |= m0_write_page_start(&npo1,0);
	//        ret |= m0_write_page_end(&npo1);
	//
	//        ret |= m0_write_page_start(&npo2,0);
	//        ret |= m0_write_page_end(&npo2);
	//    }
	//    else
	{
		ret |= m0_write_page_start(&npo1, 1);
		ret |= m0_write_page_end(&npo1);

		ret |= m0_write_page_start(&npo2, 2);
		ret |= m0_write_page_end(&npo2);
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:good block  -1:bad block
*Note         :
*****************************************************************************/
int m0_bad_block_check(struct _nand_physic_op_par *npo)
{
	int num, start_page, i;
	unsigned char spare[64];
	struct _nand_physic_op_par lnpo;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nand_controller_info *nctri = nci->nctri;

	// PHY_DBG("%s: ch: %d  chip: %d/%d  block: %d/%d \n", __func__,
	// nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block,
	// nci->blk_cnt_per_chip);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) ||
	    (npo->block >= nci->blk_cnt_per_chip)) {
		PHY_ERR("cfatal err -0, wrong input parameter, ch: %d  chip: "
			"%d/%d  block: %d/%d \n",
			nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
			npo->block, nci->blk_cnt_per_chip);
		return ERR_NO_16;
	}

	lnpo.chip = npo->chip;
	lnpo.block = npo->block;
	lnpo.mdata = NULL;
	lnpo.sdata = spare;
	lnpo.slen = nci->sdata_bytes_per_page;
	if (nci->opt_phy_op_par->bad_block_flag_position == 0) {
		// the bad block flag is in the first page, same as the logical
		// information, just read 1st page is ok
		start_page = 0;
		num = 1;
	} else if (nci->opt_phy_op_par->bad_block_flag_position == 1) {
		// the bad block flag is in the first page or the second page,
		// need read the first page and the second page
		start_page = 0;
		num = 2;
	} else if (nci->opt_phy_op_par->bad_block_flag_position == 2) {
		// the bad block flag is in the last page, need read the first
		// page and the last page
		start_page = nci->page_cnt_per_blk - 1;
		num = 1;
	} else if (nci->opt_phy_op_par->bad_block_flag_position == 3) {
		// the bad block flag is in the last 2 page, so, need read the
		// first page, the last page and the last-1 page
		start_page = nci->page_cnt_per_blk - 2;
		num = 2;
	} else {
		PHY_ERR("bad block check, unknown bad block flag position\n");
		return ERR_NO_17;
	}

	// read and check 1st page
	lnpo.page = 0;
	m0_read_page(&lnpo);
	if (lnpo.sdata[0] != 0xff) {
		PHY_ERR("find a bad block: %d %d %d ", lnpo.chip, lnpo.block,
			lnpo.page);
		PHY_ERR("sdata: %02x %02x %02x %02x \n", lnpo.sdata[0],
			lnpo.sdata[1], lnpo.sdata[2], lnpo.sdata[3]);
		return -1;
	}

	// read and check other pages
	for (i = 0, lnpo.page = start_page; i < num; i++) {
		m0_read_page(&lnpo);
		if (lnpo.sdata[0] != 0xff) {
			PHY_ERR("find a bad block: %d %d %d ", lnpo.chip,
				lnpo.block, lnpo.page);
			PHY_ERR("sdata: %02x %02x %02x %02x \n", lnpo.sdata[0],
				lnpo.sdata[1], lnpo.sdata[2], lnpo.sdata[3]);
			return -1;
		}
		lnpo.page++;
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
int m0_bad_block_mark(struct _nand_physic_op_par *npo)
{
	int num, start_page, i, ret;
	unsigned char spare[64];
	struct _nand_physic_op_par lnpo;
	struct _nand_chip_info *nci = nci_get_from_nsi(g_nsi, npo->chip);
	struct _nand_controller_info *nctri = nci->nctri;
	unsigned char *mbuf =
	    nand_get_temp_buf(nctri->nci->sector_cnt_per_page << 9);
	if (mbuf == NULL) {
		PHY_ERR("bad block mark no memory chip: %d block:%d\n",
			npo->chip, npo->block);
		return ERR_NO_18;
	}
	// PHY_ERR("%s: ch: %d  chip: %d/%d  block: %d/%d \n", __func__,
	// nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt, npo->block,
	// nci->blk_cnt_per_chip);

	if ((nci->nctri_chip_no >= nctri->chip_cnt) ||
	    (npo->block >= nci->blk_cnt_per_chip)) {
		PHY_ERR("Mfatal err -0, wrong input parameter, chip: %d/%d  "
			"block: %d/%d \n",
			nctri->channel_id, nci->nctri_chip_no, nctri->chip_cnt,
			npo->block, nci->blk_cnt_per_chip);
		return ERR_NO_18;
	}

	PHY_DBG("bad block mark chip: %d block:%d\n", npo->chip, npo->block);

	lnpo.chip = npo->chip;
	lnpo.block = npo->block;
	lnpo.page = 0;
	lnpo.sect_bitmap = nci->sector_cnt_per_page;
	lnpo.mdata = mbuf;
	lnpo.sdata = spare;
	lnpo.slen = nci->sdata_bytes_per_page;

	if (nci->opt_phy_op_par->bad_block_flag_position == 0) {
		start_page = 0;
		num = 1;
	} else if (nci->opt_phy_op_par->bad_block_flag_position == 1) {
		start_page = 0;
		num = 2;
	} else if (nci->opt_phy_op_par->bad_block_flag_position == 2) {
		start_page = nci->page_cnt_per_blk - 1;
		num = 1;
	} else if (nci->opt_phy_op_par->bad_block_flag_position == 3) {
		start_page = nci->page_cnt_per_blk - 2;
		num = 2;
	} else {
		nand_free_temp_buf(mbuf, nctri->nci->sector_cnt_per_page << 9);
		PHY_ERR("bad block mark, unknown bad block flag position\n");
		return ERR_NO_19;
	}

	ret = m0_erase_block(&lnpo);
	if (ret) {
		nand_free_temp_buf(mbuf, nctri->nci->sector_cnt_per_page << 9);
		PHY_ERR("bad block mark, erase block failed, blk %d, chip %d, "
			"ch %d\n",
			lnpo.block, lnpo.chip, nci->nctri->channel_id);
		return ret;
	}

	MEMSET(lnpo.sdata, 0, 64);
	m0_write_page(&lnpo);

	for (i = 0, lnpo.page = start_page; i < num; i++) {
		if (lnpo.page != 0) {
			lnpo.chip = npo->chip;
			lnpo.block = npo->block;
			lnpo.sect_bitmap = nci->sector_cnt_per_page;
			lnpo.mdata = mbuf;
			lnpo.sdata = spare;
			lnpo.slen = nci->sdata_bytes_per_page;
			m0_write_page(&lnpo);
		}
		lnpo.page++;
	}

	// check bad block flag
	MEMSET(lnpo.sdata, 0xff, 64);

	lnpo.chip = npo->chip;
	lnpo.block = npo->block;
	lnpo.page = 0;
	lnpo.sect_bitmap = nci->sector_cnt_per_page;
	lnpo.mdata = NULL;
	lnpo.sdata = spare;
	lnpo.slen = nci->sdata_bytes_per_page;
	m0_read_page(&lnpo);

	if (lnpo.sdata[0] != 0xff) {
		nand_free_temp_buf(mbuf, nctri->nci->sector_cnt_per_page << 9);
		return 0;
	}

	for (i = 0, lnpo.page = start_page; i < num; i++) {
		lnpo.chip = npo->chip;
		lnpo.block = npo->block;
		lnpo.sect_bitmap = nci->sector_cnt_per_page;
		lnpo.mdata = NULL;
		lnpo.sdata = spare;
		lnpo.slen = nci->sdata_bytes_per_page;
		m0_read_page(&lnpo);

		if (lnpo.sdata[0] != 0xff) {
			nand_free_temp_buf(
			    mbuf, nctri->nci->sector_cnt_per_page << 9);
			return 0;
		}
		lnpo.page++;
	}

	nand_free_temp_buf(mbuf, nctri->nci->sector_cnt_per_page << 9);
	return ERR_NO_20;
}
