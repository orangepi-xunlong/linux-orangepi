
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

#if 0

#define _NCFB1_C_

#include "../../nand_physic_inc.h"

static struct boot_ndfc_cfg {
	u8 page_size_kb;
	u8 ecc_mode;
	u8 sequence_mode;
	u8 res[5];
};

s32 m1_read_boot0_page_cfg_mode(struct _nand_chip_info *nci, struct _nand_physic_op_par *npo, struct boot_ndfc_cfg cfg)
{
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;
	u32 def_spare[32];
	s32 ecc_sta = 0, ret = 0;
	uchar spare[64];
	u32 ecc_mode_temp, page_size_temp, page_size_set;

	ecc_mode_temp = ndfc_get_ecc_mode(nci->nctri);
	//set ecc mode & randomize
	ndfc_set_ecc_mode(nci->nctri, cfg.ecc_mode);
	ndfc_enable_ecc(nci->nctri, 1, 1);

	ndfc_set_default_rand_seed(nci->nctri);
	ndfc_enable_randomize(nci->nctri);

	nand_enable_chip(nci);

	//page_size_temp = (*(nci->nctri->nreg.reg_ctl) >> 8) & 0xf;
	page_size_temp = ndfc_get_page_size(nci->nctri);
	if (cfg.page_size_kb  ==  1)
		page_size_set = 0x0;   //1K
	else if (cfg.page_size_kb  ==  2)
		page_size_set = 0x1;   //2K
	else if (cfg.page_size_kb  ==  4)
		page_size_set = 0x2;   //4K
	else if (cfg.page_size_kb  ==  8)
		page_size_set = 0x3;   //8K
	else if (cfg.page_size_kb  == 16)
		page_size_set = 0x4;   //16K
	else {
		return ERR_NO_115;
	}

	//*(nci->nctri->nreg.reg_ctl) = ((*(nci->nctri->nreg.reg_ctl)) & (~NDFC_PAGE_SIZE)) | ((page_size_set & 0xf) << 8);
	ndfc_set_page_size(nci->nctri, page_size_set);

	//command
	set_default_batch_read_cmd_seq(cmd_seq);

	if (cfg.sequence_mode == 1)
		cmd_seq->ecc_layout = cfg.sequence_mode;

	//address
	row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	cmd_seq->nctri_cmd[0].cmd_acnt = 5;
	fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);

	//data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	if (npo->mdata != NULL) {
		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	} else {
		//don't swap main data with host memory
		cmd_seq->nctri_cmd[0].cmd_swap_data = 0;
		cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	}
	cmd_seq->nctri_cmd[0].cmd_direction = 0; //read
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10; //nci->sector_cnt_per_page << 9;

	//PHY_DBG("before send cmd: %d 0x%x 0x%x\n", nci->randomizer, *(nci->nctri->nreg.reg_ecc_ctl), *(nci->nctri->nreg.reg_ecc_sta));
	MEMSET(def_spare, 0x99, 128);
	ndfc_set_spare_data(nci->nctri, def_spare, MAX_ECC_BLK_CNT);
	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m1 read page start, batch cmd io send error!\n");
		nand_disable_chip(nci);
		return ret;
	}
	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m1 read page end, batch cmd io wait error!\n");
		goto ERROR;
	}

	//check ecc
	ecc_sta = ndfc_check_ecc(nci->nctri, cfg.page_size_kb);
	//get spare data
	ndfc_get_spare_data(nci->nctri, (u32 *)spare, cfg.page_size_kb);

	if (npo->slen != 0) {
		MEMCPY(npo->sdata, spare, npo->slen);
	}
	//don't update ecc status and spare data for boot operation
	//ret = ndfc_update_ecc_sta_and_spare_data(npo, ecc_sta, spare);
	ret = ecc_sta;

ERROR:

	ndfc_set_ecc_mode(nci->nctri, ecc_mode_temp);
	//*(nci->nctri->nreg.reg_ctl) = ((*(nci->nctri->nreg.reg_ctl)) & (~NDFC_PAGE_SIZE)) | ((page_size_temp & 0xf) << 8);
	ndfc_set_page_size(nci->nctri, page_size_temp);
	//disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);
	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}
	nand_disable_chip(nci);

	return ret; //ecc status
}

s32 m1_write_boot0_page_cfg_mode(struct _nand_chip_info *nci, struct _nand_physic_op_par *npo, struct boot_ndfc_cfg cfg)
{
	uchar spare[64];
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;
	u32 row_addr = 0, col_addr = 0;
	s32 ret = 0;
	u32 ecc_mode_temp, page_size_temp, page_size_set;

	if (npo->mdata == NULL) {
		PHY_ERR("m1 write page start, input parameter error!\n");
		return ERR_NO_87;
	}

	//get spare data
	MEMSET(spare, 0xff, 64);
	if (npo->slen != 0) {
		MEMCPY(spare, npo->sdata, npo->slen);
	}
	ndfc_set_spare_data(nci->nctri, (u32 *)spare, cfg.page_size_kb);

	ecc_mode_temp = ndfc_get_ecc_mode(nci->nctri);
	//set ecc mode & randomize
	ndfc_set_ecc_mode(nci->nctri, cfg.ecc_mode);
	ndfc_enable_ecc(nci->nctri, 1, 1);
	//alway enable randomize with default random seed
	ndfc_set_default_rand_seed(nci->nctri);
	ndfc_enable_randomize(nci->nctri);

	nand_enable_chip(nci);

	//page_size_temp = (*(nci->nctri->nreg.reg_ctl) >> 8) & 0xf;
	page_size_temp = ndfc_get_page_size(nci->nctri);
	if (cfg.page_size_kb  ==  1)
		page_size_set = 0x0;   //1K
	else if (cfg.page_size_kb  ==  2)
		page_size_set = 0x1;   //2K
	else if (cfg.page_size_kb  ==  4)
		page_size_set = 0x2;   //4K
	else if (cfg.page_size_kb  ==  8)
		page_size_set = 0x3;   //8K
	else if (cfg.page_size_kb  == 16)
		page_size_set = 0x4;   //16K
	else {
		return ERR_NO_118;
	}
	//*(nci->nctri->nreg.reg_ctl) = ((*(nci->nctri->nreg.reg_ctl)) & (~NDFC_PAGE_SIZE)) | ((page_size_set & 0xf) << 8);
	ndfc_set_page_size(nci->nctri, page_size_set);

	nci->nctri->current_op_type = 1;

	//command
	set_default_batch_write_cmd_seq(cmd_seq, CMD_WRITE_PAGE_CMD1, CMD_WRITE_PAGE_CMD2);

	if (cfg.sequence_mode == 1)
		cmd_seq->ecc_layout = cfg.sequence_mode;

	//address
	row_addr = get_row_addr(nci->page_offset_for_next_blk, npo->block, npo->page);
	cmd_seq->nctri_cmd[0].cmd_acnt = 5;
	fill_cmd_addr(col_addr, 2, row_addr, 3, cmd_seq->nctri_cmd[0].cmd_addr);

	//data
	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 1;
	cmd_seq->nctri_cmd[0].cmd_direction = 1; //write
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = npo->mdata;
	cmd_seq->nctri_cmd[0].cmd_mdata_len = cfg.page_size_kb << 10; //nci->sector_cnt_per_page <<9;

	ret = _batch_cmd_io_send(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m1 read page start, batch cmd io send error!\n");

		nand_disable_chip(nci);
		return ret;
	}

	ret = _batch_cmd_io_wait(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("m1 read page end,  batch cmd io wait error!\n");
	}

	ndfc_set_ecc_mode(nci->nctri, ecc_mode_temp);
	//*(nci->nctri->nreg.reg_ctl) = ((*(nci->nctri->nreg.reg_ctl)) & (~NDFC_PAGE_SIZE)) | ((page_size_temp & 0xf) << 8);
	ndfc_set_page_size(nci->nctri, page_size_temp);
	//disable ecc mode & randomize
	ndfc_disable_ecc(nci->nctri);
	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}

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
int m1_read_boot0_page(struct _nand_chip_info *nci, struct _nand_physic_op_par *npo)
{
	struct boot_ndfc_cfg cfg;
	cfg.ecc_mode = 0x6;
	cfg.page_size_kb = 16;
	cfg.sequence_mode = 1;
	return m1_read_boot0_page_cfg_mode(nci, npo, cfg);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :每个page只写16K
*****************************************************************************/
int m1_write_boot0_page(struct _nand_chip_info *nci, struct _nand_physic_op_par *npo)
{
	struct boot_ndfc_cfg cfg;
	cfg.ecc_mode = 0x6;
	cfg.page_size_kb = 16;
	cfg.sequence_mode = 1;
	return m1_write_boot0_page_cfg_mode(nci, npo, cfg);
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :每个page只写16K
*****************************************************************************/
int m1_read_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	unsigned int j, err_flag;
	unsigned char  oob_buf[64];
	struct _nand_physic_op_par lnpo;
	struct _nand_chip_info *nci;

	nci = g_nctri->nci;

	nand_wait_all_rb_ready();

	err_flag = 0;
	for (j = 0; j < 2; j++) {
		lnpo.chip  = 0;
		lnpo.block = NAND_BOOT0_BLK_START + counter;
		lnpo.page = j;
		lnpo.sdata = oob_buf;
		lnpo.slen = 64;

		lnpo.mdata = buf + j*16384;

		MEMSET(oob_buf, 0xff, 64);

		if (nci->nand_read_boot0_page(nci, &lnpo) != 0) {
			PHY_ERR("Warning. Fail in read page %d in block %d.\n", j, lnpo.block);
			err_flag = 1;
		}
		if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
			PHY_ERR("get flash driver version error!");
			err_flag = 1;
		}

		if (j == 2) {
			if (err_flag == 1) {
			return ERR_NO_86;
			} else {
				return 0;
			}
		}
	}
	return ERR_NO_85;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int m1_write_boot0_one_nonsecure(unsigned char *buf, unsigned int len, unsigned int counter)
{
	unsigned int j;
	unsigned char  oob_buf[64];
	struct _nand_physic_op_par lnpo;
	struct _nand_chip_info *nci;
	u8i *data_ff_buf;
	int ret;

	data_ff_buf = nand_get_temp_buf(18048);
	if (!data_ff_buf) {
		PHY_ERR("[PHY_GetDefaultParam]:data_ff_buf malloc fail\n");
		return ERR_NO_84;
	}
	for (j = 0; j < 18048; j++) {
		*((__u8 *)data_ff_buf + j) = 0xFF;
	}

	nci = g_nctri->nci;

	MEMSET(oob_buf, 0xff, 64);

	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		ret = ERR_NO_83;
		goto m1_w_boot0_end;
	}

	lnpo.chip  = 0;
	lnpo.block = NAND_BOOT0_BLK_START + counter;

	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		PHY_ERR("Fail in erasing block %d!\n", lnpo.block);
		goto m1_w_boot0_end;
	}

	for (j = 0; j < nci->page_cnt_per_blk; j++) {
		lnpo.chip  = 0;
		lnpo.block = NAND_BOOT0_BLK_START + counter;
		lnpo.sdata = oob_buf;
		lnpo.slen = 64;
		lnpo.page = j;
		if (j < 2) {
			lnpo.mdata = buf + j*16384;

			nand_wait_all_rb_ready();

			if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
				PHY_ERR("Warning. Fail in writing page %d in block %d.\n", j, lnpo.block);
			}
		} else if ((j > 2) && (j%2) && (j != 255)) {
			lnpo.mdata = buf;

			nand_wait_all_rb_ready();

			if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
				PHY_ERR("Warning. Fail in writing page %d in block %d.\n", j, lnpo.block);
			}
		} else {
			lnpo.mdata = data_ff_buf;

			nand_wait_all_rb_ready();

			m1_write_page_FF(&lnpo);
		}

	}

	nand_wait_all_rb_ready();

	ret = 0;

m1_w_boot0_end:

	nand_free_temp_buf(data_ff_buf, 18048);

	return ret;
}

int m1_write_boot0_one_secure(unsigned char *buf, unsigned int len, unsigned int counter)
{
	__u32 i, k, j, count, block;
	__u8  oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL;
	__u8 *data_FF_buf = NULL;
	int ret;
	struct boot_ndfc_cfg cfg;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

	PHY_DBG("burn_boot0_hynix_f16_8G secure mode!\n");

	pos_data = (__u32 *)MALLOC(128*4*BOOT0_MAX_COPY_CNT);
	if (!pos_data) {
		PHY_ERR("burn_boot0_lsb_FF_mode, malloc for pos_data failed.\n");
		goto error;
	}

	tab = (__u32 *)MALLOC(8*1024);
	if (!tab) {
		PHY_ERR("burn_boot0_lsb_FF_mode, malloc for tab failed.\n");
		goto error;
	}

	data_FF_buf = MALLOC(18048);
	if (data_FF_buf == NULL) {
		MALLOC("data_FF_buf malloc error!");
		goto error;
	}

	for (i = 0; i < (16384+1664); i++)
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
	for (i = NAND_BOOT0_BLK_START; i < (NAND_BOOT0_BLK_START + NAND_BOOT0_BLK_CNT); i++) {
		for (j = 0; j < copies_per_block; j++) {
			for (k = 8; k < NAND_BOOT0_PAGE_CNT_PER_COPY_2; k++) {
				page_addr = i * pages_per_block + j * NAND_BOOT0_PAGE_CNT_PER_COPY_2 + k;
				if (m1_is_lsb_page((page_addr % pages_per_block))) {
					*((__u32 *)pos_data + count) = page_addr;
					count++;
					if (((count % (len/data_size_per_page)) == 0) && (count != 0))
						break;
				}
			}
		}
	}

	_generate_page_map_tab(data_size_per_page, copies_per_block * NAND_BOOT0_BLK_CNT, len/data_size_per_page, pos_data, tab, &tab_size);

	block = NAND_BOOT0_BLK_START + counter;

	PHY_DBG("pagetab boot0 %x \n", block);

	lnpo.chip  = 0;
	lnpo.block = block;

	nand_wait_all_rb_ready();

	ret = nci->nand_physic_erase_block(&lnpo);
	if (ret) {
		PHY_ERR("Fail in erasing block %d!\n", lnpo.block);
		return ret;
	}

	for (j = 0; j < copies_per_block; j++) {
		count = 0;
		for (k = 0; k < NAND_BOOT0_PAGE_CNT_PER_COPY_2; k++) {
			lnpo.chip  = 0;
			lnpo.block = block;
			lnpo.page = j * NAND_BOOT0_PAGE_CNT_PER_COPY + k;
			lnpo.sdata = oob_buf;
			lnpo.slen = 64;

			if (m1_is_lsb_page(lnpo. page)) {
				if (k < 8)
					lnpo.mdata = (void *) tab;
				else {
					lnpo.mdata = (void *) (buf + count * data_size_per_page);
					count++;
				}

				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
					PHY_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
			} else {
				lnpo.mdata = (void *) data_FF_buf ;
				nand_wait_all_rb_ready();
				if (nci->nand_write_boot0_page(nci, &lnpo) != 0) {
					PHY_ERR("Warning. Fail in writing page %d in block %d.\n", lnpo.page, lnpo.block);
				}
			}
		}
	}

	if (pos_data)
		FREE(pos_data, 128*4*BOOT0_MAX_COPY_CNT);
	if (tab)
		FREE(tab, 1024);
	if (data_FF_buf)
		FREE(data_FF_buf, 18048);
	return 0;

error:
	if (pos_data)
		FREE(pos_data, 128*4*BOOT0_MAX_COPY_CNT);
	if (tab)
		FREE(tab, 1024);
	if (data_FF_buf)
		FREE(data_FF_buf, 18048);
    return -1;
}


int m1_write_boot0_one(unsigned char *buf, unsigned int len, unsigned int counter)
{
	//int m1_write_boot0_one_nonsecure(unsigned char *buf, unsigned int len, unsigned int counter);

}

#endif
