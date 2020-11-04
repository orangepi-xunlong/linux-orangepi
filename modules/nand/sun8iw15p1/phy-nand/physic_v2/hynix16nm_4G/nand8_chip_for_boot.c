
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

#define _NCFB8_C_

#include "../nand_physic_inc.h"

// static struct boot_ndfc_cfg {
struct boot_ndfc_cfg {
	u8 page_size_kb;
	u8 ecc_mode;
	u8 sequence_mode;
	u8 res[5];
};

extern int small_nand_seed;

s32 m8_read_boot0_page_cfg_mode(struct _nand_chip_info *nci,
				struct _nand_physic_op_par *npo,
				struct boot_ndfc_cfg cfg)
{
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0, blk_mask, config = 0, i;
	u32 def_spare[32];
	s32 ecc_sta = 0, ret = 0;
	uchar spare[64];
	// u32 ecc_mode_temp,page_size_temp,page_size_set;
	u32 ecc_mode_temp = 0, bitmap = 0;

	bitmap = ((u32)1 << (cfg.page_size_kb - 1)) |
		 (((u32)1 << (cfg.page_size_kb - 1)) - 1);
	blk_mask = (0x0000FFFF >> (16 - (cfg.page_size_kb << 10) / 2048));

	// wait nand ready before read
	nand_read_chip_status_ready(nci);

	ecc_mode_temp = ndfc_get_ecc_mode(nci->nctri);

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	// set ecc and randomizer
	if (nci->nctri->channel_sel == 0) {
		/*BCH*/
		ndfc_channel_select(nci->nctri, 0);
		ndfc_set_ecc_mode(nci->nctri, cfg.ecc_mode);
		ndfc_enable_ecc(nci->nctri, 1, 1);

		ndfc_set_rand_seed(nci->nctri, small_nand_seed); // zzm 20180301
		ndfc_enable_randomize(nci->nctri);
	} else {
		/*LDPC*/
		ndfc_channel_select(nci->nctri, 1);
		ndfc_enable_decode(nci->nctri);
		ndfc_enable_ldpc_ecc(nci->nctri, 1);

		ndfc_enable_randomize(nci->nctri);
		ndfc_set_new_rand_seed(nci->nctri, npo->page);
	}

	// command
	set_default_batch_read_cmd_seq(cmd_seq);

	if (cfg.sequence_mode == 1)
		cmd_seq->ecc_layout = cfg.sequence_mode;

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

	nci->nctri->random_addr_num = nci->random_addr_num;

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
	if (nci->nctri->channel_sel == 0) {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = bitmap;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10;
	} else {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = blk_mask;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10;
	}

	//	PHY_DBG("before send cmd: %d 0x%x 0x%x\n", nci->randomizer,
	//*(nci->nctri->nreg.reg_ecc_ctl), *(nci->nctri->nreg.reg_ecc_sta));
	MEMSET(def_spare, 0x99, 128);
	ndfc_set_spare_data(nci->nctri, (u8 *)def_spare,
			    nci->sdata_bytes_per_page);

	ndfc_set_user_data_len_cfg_4bytesper1k(nci->nctri,
					       nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);

	if (nci->nctri->channel_sel == 1) {
		for (i = 0; i < (cfg.page_size_kb / 2) * 4; i += 4) {
			config |= NDFC_DATA_LEN_DATA_BLOCK << i;
		}
		*nci->nctri->nreg.reg_user_data_len_base = config;
	}

	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m8 read page start, batch cmd io send error!\n");
		nand_disable_chip(nci);
		return ret;
	}
	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m8 read page end, batch cmd io wait error!\n");
		goto ERROR;
	}

	// check ecc
	ecc_sta = ndfc_check_ecc(nci->nctri, cfg.page_size_kb);
	// get spare data
	ndfc_get_spare_data(nci->nctri, (u8 *)spare, npo->slen);

	if (npo->slen != 0) {
		MEMCPY(npo->sdata, spare, npo->slen);
	}
	// don't update ecc status and spare data for boot operation
	// ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, spare);
	ret = ecc_sta;

ERROR:

	ndfc_set_ecc_mode(nci->nctri, ecc_mode_temp);
	//*(nci->nctri->nreg.reg_ctl) = ((*(nci->nctri->nreg.reg_ctl)) &
	//(~NDFC_PAGE_SIZE)) | ((page_size_temp & 0xf) << 8);
	//	ndfc_set_page_size(nci->nctri,page_size_temp);
	// disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);

	ndfc_disable_randomize(nci->nctri);

	nand_disable_chip(nci);

	return ret; // ecc status
}

s32 m8_write_boot0_page_cfg_mode(struct _nand_chip_info *nci,
				 struct _nand_physic_op_par *npo,
				 struct boot_ndfc_cfg cfg)
{
	uchar spare[64];
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0, blk_mask, config = 0, i;
	s32 ret = 0;
	u32 ecc_mode_temp = 0;
	u32 ecc_block_valid, ecc_block_lose, bitmap = 0;

	if (npo->mdata == NULL) {
		PHY_ERR("m8 write page start, input parameter error!\n");
		return ERR_NO_87;
	}

	ecc_block_lose =
	    get_data_block_cnt_for_boot0_ecccode(nci, cfg.ecc_mode);
	ecc_block_valid = nci->sector_cnt_per_page / 2 - ecc_block_lose;
	bitmap = ((u32)1 << (ecc_block_valid - 1)) |
		 (((u32)1 << (ecc_block_valid - 1)) - 1);
	blk_mask = (0x0000FFFF >> (16 - (cfg.page_size_kb << 10) / 2048));

	// wait nand ready before write
	nand_read_chip_status_ready(nci);

	// get spare data
	MEMSET(spare, 0xff, 64);
	if (npo->slen != 0) {
		MEMCPY(spare, npo->sdata, npo->slen);
	}

	nand_enable_chip(nci);
	ndfc_clean_cmd_seq(cmd_seq);

	ndfc_set_spare_data(nci->nctri, (u8 *)spare, nci->sdata_bytes_per_page);

	ecc_mode_temp = ndfc_get_ecc_mode(nci->nctri);

	// set ecc and randomizer
	if (nci->nctri->channel_sel == 0) {
		/*BCH*/
		ndfc_enable_randomize(nci->nctri);
		ndfc_set_rand_seed(nci->nctri, small_nand_seed); // zzm 20180301

		ndfc_channel_select(nci->nctri, 0);
		ndfc_set_ecc_mode(nci->nctri, cfg.ecc_mode);
		ndfc_enable_ecc(nci->nctri, 1, 1);
	} else {
		/*LDPC*/
		ndfc_enable_randomize(nci->nctri);
		ndfc_set_new_rand_seed(nci->nctri, npo->page);

		ndfc_channel_select(nci->nctri, 1);
		ndfc_enable_encode(nci->nctri);
		ndfc_enable_ldpc_ecc(nci->nctri, 1);
	}

	nci->nctri->current_op_type = 1;
	nci->nctri->random_addr_num = nci->random_addr_num;
	nci->nctri->random_cmd2_send_flag = nci->random_cmd2_send_flag;
	if (nci->random_cmd2_send_flag) {
		nci->nctri->random_cmd2 = get_random_cmd2(npo);
	}

	// command
	set_default_batch_write_cmd_seq(cmd_seq, CMD_WRITE_PAGE_CMD1,
					CMD_WRITE_PAGE_CMD2);

	if (cfg.sequence_mode == 1) {
		cmd_seq->ecc_layout = cfg.sequence_mode;
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
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	if (nci->nctri->channel_sel == 0) {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = bitmap;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = (ecc_block_valid << 10);
	} else {
		cmd_seq->nctri_cmd[0].cmd_data_block_mask = blk_mask;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10;
	}

	ndfc_set_user_data_len_cfg_4bytesper1k(nci->nctri,
					       nci->sdata_bytes_per_page);
	ndfc_set_user_data_len(nci->nctri);

	if (nci->nctri->channel_sel == 1) {
		for (i = 0; i < (cfg.page_size_kb / 2) * 4; i += 4) {
			config |= NDFC_DATA_LEN_DATA_BLOCK << i;
		}
		*nci->nctri->nreg.reg_user_data_len_base = config;
	}

	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m8 read page start, batch cmd io send error!\n");

		nand_disable_chip(nci);
		return ret;
	}

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m8 read page end,  batch cmd io wait error!\n");
	}

	ndfc_set_ecc_mode(nci->nctri, ecc_mode_temp);
	ndfc_disable_ecc(nci->nctri);

	ndfc_disable_randomize(nci->nctri);

	nci->nctri->current_op_type = 0;
	nci->nctri->random_cmd2_send_flag = 0;

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
int m8_read_boot0_page(struct _nand_chip_info *nci,
		       struct _nand_physic_op_par *npo)
{
	struct boot_ndfc_cfg cfg;
	struct _nand_physic_op_par lnpo = *npo;

	cfg.ecc_mode = nci->nctri->max_ecc_level;
	if (nci->nctri->channel_sel == 0) // BCH
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 1;
	else // LDPC
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 2;
	cfg.sequence_mode = 1;
	return m8_read_boot0_page_cfg_mode(nci, npo, cfg);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m8_write_boot0_page(struct _nand_chip_info *nci,
			struct _nand_physic_op_par *npo)
{
	struct boot_ndfc_cfg cfg;
	struct _nand_physic_op_par lnpo = *npo;

	cfg.ecc_mode = nci->nctri->max_ecc_level;
	if (nci->nctri->channel_sel == 0) // BCH
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 1;
	else // LDPC
		cfg.page_size_kb = ((nci->sector_cnt_per_page) / 2) - 2;
	cfg.sequence_mode = 1;
	return m8_write_boot0_page_cfg_mode(nci, npo, cfg);
}

int m8_read_boot0_one_pagetab_4k(unsigned char *buf, unsigned int len,
				 unsigned int counter)
{

	__u32 j, k, m, err_flag, count, block;
	__u8 oob_buf[64];
	__u32 data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	unsigned char *ptr;
	struct _nand_physic_op_par lnpo;
	struct _nand_chip_info *nci;

	nci = g_nctri->nci;
	nci->nctri->channel_sel = 1;

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY_2;

	block = NAND_BOOT0_BLK_START + counter;
	ptr = nand_get_temp_buf(data_size_per_page);

	PHY_DBG("read blk %x \n", block);
	for (j = 0; j < copies_per_block; j++) {
		err_flag = 0;
		count = 0;
		for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY_2; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY_2 + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			for (m = 0; m < 64; m++)
				oob_buf[m] = 0x55;

			if (nci->is_lsb_page(lnpo.page)) {
				lnpo.mdata = ptr;

				nand_wait_all_rb_ready();
				if (nci->nand_read_boot0_page(nci, &lnpo) < 0) {
					PHY_ERR("Warning. Fail in read page %d "
						"in block %d.\n",
						lnpo.page, lnpo.block);
					err_flag = 1;
					break;
				}
				if ((oob_buf[0] != 0xff) ||
				    (oob_buf[1] != 0x00)) {
					PHY_ERR(
					    "get flash driver version error!");
					err_flag = 1;
					break;
				}
				MEMCPY(buf + count * data_size_per_page, ptr,
				       data_size_per_page);

				count++;
				if (count == (len / data_size_per_page))
					break;
			}
		}
		if (err_flag == 0)
			break;
	}
	nci->nctri->channel_sel = 0;
	ndfc_channel_select(nci->nctri, 0);

	nand_wait_all_rb_ready();
	nand_free_temp_buf(ptr, data_size_per_page);

	if (err_flag == 1)
		return -1;

	return 0;
}

int m8_read_boot0_one_1k_mode(unsigned char *buf, unsigned int len,
			      unsigned int counter)
{
	__u32 i, j, ret;
	__u8 oob_buf[64];
	__u32 pages_per_block, blocks_per_copy, start_block, count;
	__u32 flag;
	unsigned char *ptr;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

	PHY_DBG("read_boot0_1k mode!\n");

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		goto error;
	}

	pages_per_block = nci->page_cnt_per_blk;

	blocks_per_copy = NAND_BOOT0_PAGE_CNT_PER_COPY_4 / pages_per_block;
	if (NAND_BOOT0_PAGE_CNT_PER_COPY_4 % pages_per_block)
		blocks_per_copy++;

	start_block = blocks_per_copy * counter;
	if ((start_block + blocks_per_copy) >
	    aw_nand_info.boot->uboot_start_block) {
		return 0;
	}

	PHY_DBG("boot0 count %d!\n", counter);

	ptr = nand_get_temp_buf(1024);

	count = 0;
	flag = 0;
	for (i = start_block; i < (start_block + blocks_per_copy); i++) {
		lnpo.chip = 0;
		lnpo.block = i;
		lnpo.page = 0;
		nand_wait_all_rb_ready();

		for (j = 0; j < pages_per_block; j++) {
			lnpo.page = j;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;
			lnpo.mdata = ptr;
			small_nand_seed = count; // zzm 20180301

			nand_wait_all_rb_ready();
			if (nci->nand_read_boot0_page(nci, &lnpo) < 0) {
				PHY_ERR("Warning. Fail in read page %d in "
					"block %d.\n",
					lnpo.page, lnpo.block);
				goto error;
			}
			if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
				PHY_ERR("get flash driver version error!");
				goto error;
			}

			MEMCPY(buf + count * 1024, ptr, 1024);

			count++;
			if (count == (len / 1024)) {
				flag = 1;
				break;
			}
		}
		if (flag == 1)
			break;
	}
	nand_free_temp_buf(ptr, 1024);
	small_nand_seed = 0;
	return 0;

error:
	nand_free_temp_buf(ptr, 1024);
	return -1;
}

int m8_read_boot0_one(unsigned char *buf, unsigned int len,
		      unsigned int counter)
{
	struct _nand_chip_info *nci;
	__u32 pagesize = 0;
	__u32 pagesperblock = 0;

	nci = g_nctri->nci;
	pagesize = nci->sector_cnt_per_page << 9;
	pagesperblock = nci->page_cnt_per_blk;

	PHY_DBG("m0 read boot0 one \n");
	if ((pagesize >= 8192) && (pagesperblock >= 128))
		return m8_read_boot0_one_pagetab_4k(buf, len, counter);
	else
		return m8_read_boot0_one_1k_mode(buf, len, counter);

	return -1;
}

int m8_write_boot0_one(unsigned char *buf, unsigned int len,
		       unsigned int counter)
{
	__u32 i, k, j, count, block;
	__u8 oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL, *rr_tab = NULL;
	__u8 *data_FF_buf = NULL;
	int ret;
	struct boot_ndfc_cfg cfg;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

#if 1
	/*A50 FPGA LDPC encode have bug, must to use SDR interface By zzm*/
	if (nci->support_toggle_only) {
		ret = before_update_nctri_interface(nci->nctri);
		if (ret) {
			PHY_ERR("before_update nctri nand interface fail!\n");
			return ret;
		}
	} else {
		ret = update_boot0_nctri_interface(nci->nctri);
		if (ret) {
			PHY_ERR("update boot0 nctri nand interface fail!\n");
			return ret;
		}
	}
#endif

	nci->nctri->channel_sel = 1;

	PHY_DBG("burn_boot0_lsb_FF_pagetab secure mode!\n");

	pos_data = (__u32 *)MALLOC(128 * 4 * BOOT0_MAX_COPY_CNT);
	if (!pos_data) {
		PHY_ERR(
		    "burn_boot0_lsb_FF_mode, malloc for pos_data failed.\n");
		goto error;
	}

	tab = (__u32 *)MALLOC(8 * 1024);
	if (!tab) {
		PHY_ERR("burn_boot0_lsb_FF_mode, malloc for tab failed.\n");
		goto error;
	}

	rr_tab = (__u32 *)MALLOC(8 * 1024);
	if (!rr_tab) {
		PHY_ERR("burn_boot0_lsb_FF_mode, malloc for rr_tab failed.\n");
		goto error;
	}

	data_FF_buf = MALLOC(20 * 1024);
	if (data_FF_buf == NULL) {
		PHY_ERR("data_FF_buf malloc error!");
		goto error;
	}

	for (i = 0; i < (16384 + 1664); i++)
		*((__u8 *)data_FF_buf + i) = 0xFF;

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		goto error;
	}

	data_size_per_page = 4096;
	pages_per_block = nci->page_cnt_per_blk;
	copies_per_block = pages_per_block / NAND_BOOT0_PAGE_CNT_PER_COPY_2;

	count = 0;
	for (i = NAND_BOOT0_BLK_START;
	     i < (NAND_BOOT0_BLK_START + aw_nand_info.boot->uboot_start_block);
	     i++) {
		for (j = 0; j < copies_per_block; j++) {
			for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY_2; k++) {
				page_addr = i * pages_per_block +
					    j * NAND_BOOT0_PAGE_CNT_PER_COPY_2 +
					    k;
				if (nci->is_lsb_page(
					(page_addr % pages_per_block))) {
					*((__u32 *)pos_data + count) =
					    page_addr;
					count++;
					if (((count %
					      (len / data_size_per_page)) ==
					     0) &&
					    (count != 0))
						break;
				}
			}
		}
	}

	_generate_page_map_tab(
	    data_size_per_page,
	    copies_per_block * aw_nand_info.boot->uboot_start_block,
	    len / data_size_per_page, pos_data, tab, &tab_size);

	// get read retry table
	_get_read_retry_table(nci, (u8 *)rr_tab);

	block = NAND_BOOT0_BLK_START + counter;

	PHY_DBG("pagetab boot0 %x \n", block);

	lnpo.chip = 0;
	lnpo.block = block;
	lnpo.page = 0;

	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		PHY_ERR("Fail in erasing block %d!\n", lnpo.block);
		// return ret;
	}

	for (j = 0; j < copies_per_block; j++) {
		count = 0;
		for (k = 0; k < NAND_BOOT0_PAGE_CNT_PER_COPY_2; k++) {
			lnpo.chip = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY_2 + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			if (nci->is_lsb_page(lnpo.page)) {
				if (k < 4)
					lnpo.mdata = (__u8 *)rr_tab;
				else if (k < 8)
					lnpo.mdata = (__u8 *)tab;
				else {
					if (count < len / data_size_per_page)
						lnpo.mdata =
						    (__u8
							 *)(buf +
							    count *
								data_size_per_page);
					else
						lnpo.mdata = (__u8 *)buf;
					count++;
				}

				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) !=
				    0) {
					PHY_ERR("Warning. Fail in writing page "
						"%d in block %d.\n",
						lnpo.page, lnpo.block);
				}
			} else {
				lnpo.mdata = (__u8 *)data_FF_buf;
				nand_wait_all_rb_ready();
				if (m1_write_page_FF(
					&lnpo,
					(nci->sector_cnt_per_page / 2)) != 0) {
					PHY_ERR("Warning. Fail in writing page "
						"%d in block %d.\n",
						lnpo.page, lnpo.block);
				}
			}
		}
	}

	nci->nctri->channel_sel = 0;
	ndfc_channel_select(nci->nctri, 0);

	if (pos_data)
		FREE(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		FREE(tab, 8 * 1024);
	if (rr_tab)
		FREE(rr_tab, 8 * 1024);
	if (data_FF_buf)
		FREE(data_FF_buf, 18048);
	return 0;

error:
	if (pos_data)
		FREE(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		FREE(tab, 8 * 1024);
	if (rr_tab)
		FREE(rr_tab, 8 * 1024);
	if (data_FF_buf)
		FREE(data_FF_buf, 18048);
	return -1;
}
