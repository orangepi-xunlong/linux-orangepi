
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


#define _NCF_C_

#include "../../physic_common/nand_common_interface.h"
#include "../nand_physic_inc.h"
#include <linux/sched.h>

int nand_read_scan_data(unsigned int chip, unsigned int block,
			unsigned int page, unsigned int bitmap,
			unsigned char *mbuf, unsigned char *sbuf)
{
	return nand_physic_read_page(chip, block, page, bitmap, mbuf, sbuf);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void nand_enable_chip(struct _nand_chip_info *nci)
{
	struct _nand_controller_info *nctri = nci->nctri;
	unsigned int chip_no = nci->nctri_chip_no;

	ndfc_select_chip(nctri, nctri->ce[chip_no]);
	ndfc_select_rb(nctri, nctri->rb[chip_no]);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void nand_disable_chip(struct _nand_chip_info *nci)
{
	// select invalid CE signal, disable all chips
	ndfc_select_chip(nci->nctri, 0xf);
	// select invalid RB signal, no any more RB busy to ready interrupt
	ndfc_select_rb(nci->nctri, 0x3);
	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_reset_chip(struct _nand_chip_info *nci)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	nand_enable_chip(nci);

	ndfc_clean_cmd_seq(cmd_seq);
	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd =
	    CMD_RESET; //??? don't support onfi ddr interface
	cmd_seq->nctri_cmd[0].cmd_send = 1;
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;
	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, reset failed!\n");
	}

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
int nand_sync_reset_chip(struct _nand_chip_info *nci)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	nand_enable_chip(nci);

	ndfc_clean_cmd_seq(cmd_seq);
	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd = CMD_SYNC_RESET; // support onfi ddr
						    // interface
	cmd_seq->nctri_cmd[0].cmd_send = 1;
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;
	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, reset failed!\n");
	}

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
int nand_first_reset_chip(struct _nand_chip_info *nci, unsigned int chip_no)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	// enable chip, don't select rb signal
	ndfc_select_chip(nci->nctri, chip_no);

	ndfc_clean_cmd_seq(cmd_seq);
	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd =
	    CMD_RESET; //??? don't support onfi ddr interface
	cmd_seq->nctri_cmd[0].cmd_send = 1;
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;
	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_first_reset_chip, reset failed!\n");
		goto ERROR;
	}

	// wait all rb ready, because we don't know which rb sigal connect to
	// current chip
	ret = ndfc_wait_all_rb_ready(nci->nctri);
	if (ret) {
		PHY_ERR(
		    "nand_first_reset_chip, ndfc_wait_all_rb_ready timeout\n");
		goto ERROR;
	}

	nand_disable_chip(nci);
	return NAND_OP_TRUE;

ERROR:
	nand_disable_chip(nci);
	return NAND_OP_FALSE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_chip_status(struct _nand_chip_info *nci, u8 *pstatus)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	nand_enable_chip(nci);
	ndfc_repeat_mode_enable(nci->nctri);

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd = CMD_READ_STA;
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	cmd_seq->nctri_cmd[0].cmd_direction = 0; // read
	cmd_seq->nctri_cmd[0].cmd_mdata_len = 1;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = pstatus;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, read chip status failed!\n");
	}

	ndfc_repeat_mode_disable(nci->nctri);
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
int nand_read_chip_status_ready(struct _nand_chip_info *nci)
{
	int ret;
	int timeout = 0xffff;
	u8 status;

	while (1) {
		ret = nand_read_chip_status(nci, &status);
		if (ret) {
			PHY_ERR("read chip status failed!\n");
			ret = -NAND_OP_FALSE;
			break;
		}

		if (status & NAND_STATUS_READY)
			break;

		if (timeout-- < 0) {
			PHY_ERR("wait nand status ready timeout,chip=%x, "
				"status=%x\n",
				nci->chip_no, status);
			ret = ERR_TIMEOUT;
			break;
		}
		//cond_resched();
	}
	if (status & NAND_OPERATE_FAIL) {
		PHY_ERR("read chip status failed %x %x!\n", nci->chip_no,
			status);
		ret = -NAND_OP_FALSE;
		// ret = 0;
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
int is_chip_rb_ready(struct _nand_chip_info *nci)
{
	struct _nand_controller_info *nctri = nci->nctri;
	unsigned int chip_no = nci->chip_no;

	return ndfc_get_rb_sta(nctri, nctri->rb[chip_no]);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_id(struct _nand_chip_info *nci, unsigned char *id)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	nand_enable_chip(nci);
	ndfc_repeat_mode_enable(nci->nctri);
	ndfc_disable_randomize(nci->nctri);

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd = CMD_READ_ID;
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	cmd_seq->nctri_cmd[0].cmd_acnt = 1;
	cmd_seq->nctri_cmd[0].cmd_addr[0] = 0x0;

	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	cmd_seq->nctri_cmd[0].cmd_direction = 0; // read
	cmd_seq->nctri_cmd[0].cmd_mdata_len = 8;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = id;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, read chip id failed!\n");
	}

	ndfc_repeat_mode_disable(nci->nctri);
	nand_disable_chip(nci);

	return ret;
}

int nand_first_read_id(struct _nand_chip_info *nci, unsigned int chip_no,
		       char *id)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	// enable chip, don't select rb signal
	ndfc_select_chip(nci->nctri, chip_no);

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd = CMD_READ_ID;
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	cmd_seq->nctri_cmd[0].cmd_acnt = 1;
	cmd_seq->nctri_cmd[0].cmd_addr[0] = 0x0;

	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	cmd_seq->nctri_cmd[0].cmd_direction = 0; // read
	cmd_seq->nctri_cmd[0].cmd_mdata_len = 8;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = id;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, read chip id failed!\n");
	}

	nand_disable_chip(nci);
	if (ret)
		return NAND_OP_FALSE;
	else
		return NAND_OP_TRUE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_get_feature(struct _nand_chip_info *nci, u8 *addr, u8 *feature)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	nand_enable_chip(nci);
	ndfc_repeat_mode_enable(nci->nctri);
	ndfc_disable_randomize(nci->nctri);

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd = CMD_GET_FEATURE;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	cmd_seq->nctri_cmd[0].cmd_acnt = 1;
	cmd_seq->nctri_cmd[0].cmd_addr[0] = addr[0];
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;

	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	cmd_seq->nctri_cmd[0].cmd_direction = 0; // read
	cmd_seq->nctri_cmd[0].cmd_mdata_len = 4;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = feature;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, read chip id failed!\n");
	}

	ndfc_repeat_mode_disable(nci->nctri);
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
int nand_set_feature(struct _nand_chip_info *nci, u8 *addr, u8 *feature)
{
	int ret = 0;
	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	nand_enable_chip(nci);
	ndfc_repeat_mode_enable(nci->nctri);
	ndfc_disable_randomize(nci->nctri);

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;
	cmd_seq->nctri_cmd[0].cmd = CMD_SET_FEATURE;
	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[0].cmd_send = 1;

	cmd_seq->nctri_cmd[0].cmd_acnt = 1;
	cmd_seq->nctri_cmd[0].cmd_addr[0] = addr[0];
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;

	cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
	cmd_seq->nctri_cmd[0].cmd_swap_data_dma = 0;
	cmd_seq->nctri_cmd[0].cmd_direction = 1; // write
	cmd_seq->nctri_cmd[0].cmd_mdata_len = 4;
	cmd_seq->nctri_cmd[0].cmd_mdata_addr = feature;

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);
	if (ret) {
		PHY_ERR("nand_reset_chip, read chip id failed!\n");
	}

	ndfc_repeat_mode_disable(nci->nctri);
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
unsigned int get_row_addr(unsigned int page_offset_for_next_blk,
			  unsigned int block, unsigned int page)
{
	unsigned int maddr;

	if (page_offset_for_next_blk == 32)
		maddr = (block << 5) + page;
	else if (page_offset_for_next_blk == 64)
		maddr = (block << 6) + page;
	else if (page_offset_for_next_blk == 128)
		maddr = (block << 7) + page;
	else if (page_offset_for_next_blk == 256)
		maddr = (block << 8) + page;
	else if (page_offset_for_next_blk == 512)
		maddr = (block << 9) + page;
	else if (page_offset_for_next_blk == 1024)
		maddr = (block << 10) + page;
	else {
		maddr = 0xffffffff;
		PHY_ERR("error page per block %d!\n", page_offset_for_next_blk);
	}
	return maddr;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int fill_cmd_addr(u32 col_addr, u32 col_cycle, u32 row_addr, u32 row_cycle,
		  u8 *abuf)
{
	s32 i;

	if (col_cycle) {
		for (i = 0; i < col_cycle; i++)
			abuf[i] = (col_addr >> (i * 8)) & 0xff;
	}

	if (row_cycle) {
		for (i = 0 + col_cycle; i < col_cycle + row_cycle; i++)
			abuf[i] = (row_addr >> ((i - col_cycle) * 8)) & 0xff;
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
void set_default_batch_read_cmd_seq(struct _nctri_cmd_seq *cmd_seq)
{
	cmd_seq->cmd_type = CMD_TYPE_BATCH;
	cmd_seq->ecc_layout = ECC_LAYOUT_INTERLEAVE;

	cmd_seq->nctri_cmd[0].cmd = CMD_READ_PAGE_CMD1;
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;

	cmd_seq->nctri_cmd[1].cmd = CMD_READ_PAGE_CMD2;
	cmd_seq->nctri_cmd[2].cmd = CMD_CHANGE_READ_ADDR_CMD1;
	cmd_seq->nctri_cmd[3].cmd = CMD_CHANGE_READ_ADDR_CMD2;

	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[1].cmd_valid = 1;
	cmd_seq->nctri_cmd[2].cmd_valid = 1;
	cmd_seq->nctri_cmd[3].cmd_valid = 1;
}

void set_default_batch_write_cmd_seq(struct _nctri_cmd_seq *cmd_seq,
				     u32 write_cmd1, u32 write_cmd2)
{
	cmd_seq->cmd_type = CMD_TYPE_BATCH;
	cmd_seq->ecc_layout = ECC_LAYOUT_INTERLEAVE;

	cmd_seq->nctri_cmd[0].cmd = write_cmd1;
	cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;

	cmd_seq->nctri_cmd[1].cmd = write_cmd2;
	cmd_seq->nctri_cmd[2].cmd = CMD_CHANGE_WRITE_ADDR_CMD;

	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	cmd_seq->nctri_cmd[1].cmd_valid = 1;
	cmd_seq->nctri_cmd[2].cmd_valid = 1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 _cal_nand_onfi_timing_mode(u32 mode, u32 dclk)
{
	s32 tmode = -1;
	if (mode == 0x0) {// SDR/Async mode
		if (dclk < 15)
			tmode = 0;
		else if (dclk < 24)
			tmode = 1;
		else if (dclk < 30)
			tmode = 2; // 30.5->30
		else if (dclk < 36)
			tmode = 3; // 36.5->36
		else if (dclk < 45)
			tmode = 4;
		else if (dclk <= 50)
			tmode = 5;
		else
			PHY_ERR("wrong dclk(%d) in mode(%d)\n", dclk, mode);
	} else if (mode == 0x2) {// nv-ddr
		if (dclk < 26)
			tmode = 0; // 26.5->26
		else if (dclk < 41)
			tmode = 1; // 41.5->41
		else if (dclk < 58)
			tmode = 2; // 58.5->58
		else if (dclk < 75)
			tmode = 3;
		else if (dclk < 91)
			tmode = 4; // 91.5->91
		else if (dclk <= 100)
			tmode = 5;
		else
			PHY_ERR("wrong dclk(%d) in mode(%d)\n", dclk, mode);
	} else if (mode == 0x12) {// nv-ddr2
		if (dclk < 36)
			tmode = 0; // 36.5->36
		else if (dclk < 53)
			tmode = 1;
		else if (dclk < 74)
			tmode = 2; // 74.5->74
		else if (dclk < 91)
			tmode = 3; // 91.5->91
		else if (dclk < 116)
			tmode = 4; // 116.5->116
		else if (dclk < 149)
			tmode = 5; // 149.5->149
		else if (dclk < 183)
			tmode = 6;
		else if (dclk < 200)
			tmode = 7;
		else
			PHY_ERR("wrong dclk(%d) in mode(%d)\n", dclk, mode);
	} else {
		tmode = 0;
	}
	return tmode;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 _setup_nand_onfi_ddr2_para(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	u8 pr[4]; // read back value

	addr = 0x02; // feature address 02h, NV-DDR2 Configuration

	p[0] = 0x0;
	p[0] |= (nci->nfc_init_ddr_info->en_ext_verf & 0x1);
	p[0] |= ((nci->nfc_init_ddr_info->en_dqs_c & 0x1) << 1);
	p[0] |= ((nci->nfc_init_ddr_info->en_re_c & 0x1) << 2);
	p[0] |= ((nci->nfc_init_ddr_info->odt & 0x7) << 4);
	p[1] = 0x0;
	p[1] |= (nci->nfc_init_ddr_info->dout_re_warmup_cycle & 0x7);
	p[1] |= ((nci->nfc_init_ddr_info->din_dqs_warmup_cycle & 0x7) << 4);
	p[2] = 0x0;
	p[3] = 0x0;

	nand_set_feature(nci, &addr, p);
	nand_get_feature(nci, &addr, pr);
	if ((pr[0] != p[0]) || (pr[1] != p[1])) {
		PHY_ERR(
		    "set feature(addr 0x02) NV-DDR2 Configuration failed!\n");
		return ERR_NO_113;
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
s32 _setup_nand_onfi_driver_strength(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	u8 pr[4]; // read back value
	u32 drive;

	drive = nci->nfc_init_ddr_info->output_driver_strength;
	if (drive > 3) {
		PHY_ERR("wrong onfi nand flash driver strength value: %d. keep "
			"default value\n",
			drive);
		return 0;
	}

	addr = 0x10; // feature address 10h, Programmable output driver strength
	p[0] = 0x0;
	p[0] |= ((nci->nfc_init_ddr_info->output_driver_strength & 0x3) << 0);
	p[1] = 0x0;
	p[2] = 0x0;
	p[3] = 0x0;
	nand_set_feature(nci, &addr, p);
	nand_get_feature(nci, &addr, pr);
	if (pr[0] != p[0]) {
		PHY_ERR("set feature(addr 0x10) Programmable output driver "
			"strength failed!\n");
		return ERR_NO_112;
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
s32 _setup_nand_onfi_vendor_specific_feature(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	u8 pr[4]; // read back value

	if (nci->id[0] == 0x2c) {// micron nand flash
		if (nci->nfc_init_ddr_info->output_driver_strength <= 3) {
			addr = 0x80; // feature address 80h, Programmable output
				     // driver strength
			p[0] = 0x0;
			p[0] |=
			    ((nci->nfc_init_ddr_info->output_driver_strength &
			      0x3)
			     << 0);
			p[1] = 0x0;
			p[2] = 0x0;
			p[3] = 0x0;
			nand_set_feature(nci, &addr, p);
			nand_get_feature(nci, &addr, pr);
			if (pr[0] != p[0]) {
				PHY_ERR("set feature(addr 0x80) Programmable "
					"output driver strength failed!\n");
				return ERR_NO_111;
			}
		}

		if (nci->nfc_init_ddr_info->rb_pull_down_strength <= 3) {
			addr = 0x81; // feature address 81h, Programmable R/B#
				     // Pull-Down strength
			p[0] = 0x0;
			p[0] |=
			    ((nci->nfc_init_ddr_info->rb_pull_down_strength &
			      0x3)
			     << 0);
			p[1] = 0x0;
			p[2] = 0x0;
			p[3] = 0x0;
			nand_set_feature(nci, &addr, p);
			nand_get_feature(nci, &addr, pr);
			if (pr[0] != p[0]) {
				PHY_ERR("set feature(addr 0x80) Programmable "
					"R/B# Pull-Down strength failed!\n");
				return ERR_NO_110;
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
s32 _change_nand_onfi_timing_mode(struct _nand_chip_info *nci, u32 if_type,
				  u32 timing_mode)
{
	u8 addr;
	u8 p[4];

	if (!SUPPORT_CHANGE_ONFI_TIMING_MODE) {
		PHY_ERR("don't support change onfi timing mode. if_type: %d\n",
			if_type);
		return ERR_NO_71;
	}

	if ((if_type != SDR) && (if_type != ONFI_DDR) &&
	    (if_type != ONFI_DDR2)) {
		PHY_ERR("wrong onfi interface type: %d\n", if_type);
		return ERR_NO_70;
	}

	if ((if_type == SDR) && (timing_mode > 5)) {
		PHY_ERR("wrong onfi timing mode(%d) in interface type(%d)\n",
			if_type, timing_mode);
		return ERR_NO_69;
	}
	if ((if_type == ONFI_DDR) && (timing_mode > 5)) {
		PHY_ERR("wrong onfi timing mode(%d) in interface type(%d)\n",
			if_type, timing_mode);
		return ERR_NO_68;
	}
	if ((if_type == ONFI_DDR2) && (timing_mode > 7)) {
		PHY_ERR("wrong onfi timing mode(%d) in interface type(%d)\n",
			if_type, timing_mode);
		return ERR_NO_67;
	}

	addr = 0x01; // feature address 01h, Timing Mode
	p[0] = 0;
	if (if_type == ONFI_DDR)
		p[0] = (0x1U << 4) | (timing_mode & 0xf);
	else if (if_type == ONFI_DDR2)
		p[0] = (0x2U << 4) | (timing_mode & 0xf);
	else
		p[0] = (timing_mode & 0xf);

	p[1] = 0x0;
	p[2] = 0x0;
	p[3] = 0x0;
	nand_set_feature(nci, &addr, p);
	// aw_delay(0x100); //max tITC is 1us

	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 _setup_nand_toggle_ddr2_para(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	u8 pr[4]; // read back value

	addr = 0x02; // feature address 02h, Toggle 2.0-specific Configuration
	p[0] = 0x0;
	p[0] |= (nci->nfc_init_ddr_info->en_ext_verf & 0x1);
	p[0] |= ((nci->nfc_init_ddr_info->en_dqs_c & 0x1) << 1);
	p[0] |= ((nci->nfc_init_ddr_info->en_re_c & 0x1) << 2);
	p[0] |= ((nci->nfc_init_ddr_info->odt & 0x7) << 4);
	p[1] = 0x0;
	p[1] |= ((nci->nfc_init_ddr_info->dout_re_warmup_cycle & 0x7) << 0);
	p[1] |= ((nci->nfc_init_ddr_info->din_dqs_warmup_cycle & 0x7) << 4);
	p[2] = 0x0;
	p[3] = 0x0;
	nand_set_feature(nci, &addr, p);
	nand_get_feature(nci, &addr, pr);
	if ((pr[0] != p[0]) || (pr[1] != p[1])) {
		PHY_ERR("set feature(addr 0x02) Toggle 2.0-specific "
			"Configuration failed!\n");
		return ERR_NO_66;
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
s32 _setup_nand_toggle_driver_strength(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	u8 pr[4]; // read back value
	u32 drive;

	drive = nci->nfc_init_ddr_info->output_driver_strength;
	if ((drive != 2) && (drive != 4) && (drive != 6)) {
		PHY_ERR("wrong toggle nand flash driver strength value: %d. "
			"keep default value.\n",
			drive);
		return 0;
	}

	addr = 0x10; // feature address 10h, Programmable output driver strength
	p[0] = 0x0;
	p[0] |= ((nci->nfc_init_ddr_info->output_driver_strength & 0x7)
		 << 0); // 2, 4, 6
	p[1] = 0x0;
	p[2] = 0x0;
	p[3] = 0x0;
	nand_set_feature(nci, &addr, p);
	nand_get_feature(nci, &addr, pr);
	if (pr[0] != p[0]) {
		PHY_ERR("set feature(addr 0x10) Programmable output driver "
			"strength failed!\n");
		return ERR_NO_65;
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
s32 _setup_nand_toggle_vendor_specific_feature(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	// u8 pr[4]; //read back value

	if ((nci->id[0] == 0x45) ||
	    (nci->id[0] == 0x98)) {// sandisk/toshiba nand flash
		addr = 0x80; // Sandisk: This address (80h) is a vendor-specific
			     // setting used to turn on or turn off Toggle Mode
		p[0] = 0x0;
		if ((nci->interface_type == TOG_DDR) ||
		    (nci->interface_type == TOG_DDR2))
			p[0] = 0x0; // enable toggle mode
		else
			p[0] = 0x1; // disable toggle mode
		p[1] = 0x0;
		p[2] = 0x0;
		p[3] = 0x0;
		nand_set_feature(nci, &addr, p);
#if 0
		nfc_get_feature(addr, pr);
		if (pr[0] != p[0]) {
			PHY_ERR("set feature(addr 0x80) Programmable output driver strength failed!\n");
			return ERR_NO_64;
		}
#endif
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
s32 _setup_ddr_nand_force_to_sdr_para(struct _nand_chip_info *nci)
{
	u8 addr;
	u8 p[4];
	u8 pr[4]; // read back value

	if ((nci->id[0] == 0x45) ||
	    (nci->id[0] == 0x98)) {// sandisk/toshiba nand flash
		addr = 0x80; // Sandisk: This address (80h) is a vendor-specific
			     // setting used to turn on or turn off Toggle Mode

#if 1
		ndfc_set_legacy_interface(nci->nctri);
		nand_get_feature(nci, &addr, pr);
		// PHY_ERR("get feature(addr 0x80) 0x%x 0x%x 0x%x
		// 0x%x!\n",pr[0],pr[1],pr[2],pr[3]);
		if (pr[0] == 0x00) {
			ndfc_set_toogle_interface(nci->nctri);
		} else {
			ndfc_set_legacy_interface(nci->nctri);
		}
#endif

		p[0] = 0x1; // disable toggle mode
		p[1] = 0x0;
		p[2] = 0x0;
		p[3] = 0x0;
		nand_set_feature(nci, &addr, p);
		ndfc_set_legacy_interface(nci->nctri);
#if 1
		nand_get_feature(nci, &addr, pr);
// PHY_ERR("get feature(addr 0x80) 0x%x 0x%x 0x%x
// 0x%x!\n",pr[0],pr[1],pr[2],pr[3]);
#endif
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
s32 _check_scan_data(u32 first_check, u32 chip, u32 *scan_good_blk_no,
		     u8 *main_buf)
{
	s32 ret;
	u32 b, start_blk = 4, blk_cnt = 5;
	u8 oob_buf[64];
	u32 buf_size = 32768;
	u32 buf_flag = 0;

	if (main_buf == NULL) {
		main_buf = nand_get_temp_buf(buf_size);
		if (!main_buf) {
			PHY_ERR("check scan data, main_buf 0x%x is null!\n",
				main_buf);
			return ERR_NO_63;
		}
		buf_flag = 1;
	}

	if (first_check) {
		scan_good_blk_no[chip] = 0xffff;

		for (b = start_blk; b < start_blk + blk_cnt; b++) {
			// ret = nand_read_scan_data(0, b, 0,
			// g_nsi->nci->sector_cnt_per_page, main_buf, oob_buf);
			ret = nand_read_scan_data(
			    0, b, 0, g_nsi->nci->sector_cnt_per_page, NULL,
			    oob_buf);
			if ((oob_buf[0] == 0xff) && (oob_buf[1] == 0xff) &&
			    (oob_buf[2] == 0xff) && (oob_buf[3] == 0xff)) {
				scan_good_blk_no[chip] = b;
				ret = 1;
				break;
			}

			if (ret >= 0) {
				ret = 0;
				scan_good_blk_no[chip] = b;
				break;
			}
		}
		PHY_DBG("check scan data, first_check %d %d!\n",
			scan_good_blk_no[chip], ret);
	} else {
		// ret = nand_read_scan_data(0, scan_good_blk_no[chip], 0,
		// g_nsi->nci->sector_cnt_per_page, main_buf, oob_buf);
		ret = nand_read_scan_data(0, scan_good_blk_no[chip], 0,
					  g_nsi->nci->sector_cnt_per_page, NULL,
					  oob_buf);
		if (ret >= 0) {
			ret = 0;
		}

		if ((oob_buf[0] == 0xff) && (oob_buf[1] == 0xff) &&
		    (oob_buf[2] == 0xff) && (oob_buf[3] == 0xff)) {
			ret = 1;
		}
	}

	if (buf_flag != 0) {
		nand_free_temp_buf(main_buf, buf_size);
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
s32 _change_all_nand_parameter(struct _nand_controller_info *nctri,
			       u32 ddr_type, u32 pre_ddr_type, u32 dclk)
{
	struct _nand_chip_info *nci = nctri->nci;
	//__s32 ret;
	//__u32 bank;
	//__u8  chip, rb;
	__u32 ddr_change_mode = 0;
	__u32 tmode;
	s32 ret;
	// NFC_CMD_LIST reset_cmd;

	/*check parameter*/
	if (((ddr_type == ONFI_DDR) || (ddr_type == ONFI_DDR2)) &&
	    (pre_ddr_type == SDR)) // Async => ONFI DDR/DDR2
		ddr_change_mode = 1;
	else if ((ddr_type == SDR) &&
		 ((pre_ddr_type == ONFI_DDR) ||
		  (pre_ddr_type == ONFI_DDR2))) // ONFI DDR/DDR2 => Async
		ddr_change_mode = 2;
	else if (((ddr_type == TOG_DDR) || (ddr_type == TOG_DDR2)) &&
		 (pre_ddr_type == SDR)) // Async => Toggle DDR/DDR2
		ddr_change_mode = 3;
	else if ((ddr_type == SDR) &&
		 ((pre_ddr_type == TOG_DDR) ||
		  (pre_ddr_type == TOG_DDR2))) // Toggle DDR/DDR2 => Async
		ddr_change_mode = 4;
	else if (((ddr_type == TOG_DDR) && (pre_ddr_type == TOG_DDR2)) ||
		 ((ddr_type == TOG_DDR2) &&
		  (pre_ddr_type == TOG_DDR))) // Toggle DDR2 <=> Toggle DDR
		ddr_change_mode = 5;
	else if (ddr_type == pre_ddr_type)
		ddr_change_mode = 6;
	else {
		PHY_ERR("_change_nand_parameter: wrong input para, "
			"ddr_type %d, pre_ddr_type %d\n",
			ddr_type, pre_ddr_type);
		return ERR_NO_62;
	}

	tmode = _cal_nand_onfi_timing_mode(ddr_type, dclk);
	nci->timing_mode = tmode;

	/*change nand flash parameter*/
	while (nci) {
		nand_enable_chip(nci);

		// PHY_DBG("%s: ch: %d  chip: %d  rb: %d\n", __func__,
		// NandIndex, chip, rb);

		if (ddr_change_mode == 1) {
			/* Async => ONFI DDR/DDR2 */
			PHY_DBG("mode 1 : Async => ONFI DDR/DDR2\n");
			if ((nci->support_ddr2_specific_cfg) &&
			    (ddr_type == ONFI_DDR2)) {
				ret = _setup_nand_onfi_ddr2_para(nci);
				if (ret) {
					PHY_ERR("_setup_nand_onfi_ddr2_para() "
						"failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_io_driver_strength) {
				ret = _setup_nand_onfi_driver_strength(nci);
				if (ret) {
					PHY_ERR("_setup_nand_onfi_driver_"
						"strength() failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_vendor_specific_cfg) {
				ret = _setup_nand_onfi_vendor_specific_feature(
				    nci);
				if (ret) {
					PHY_ERR("_setup_nand_onfi_vendor_"
						"specific_feature() failed! "
						"%d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_change_onfi_timing_mode) {
				ret = _change_nand_onfi_timing_mode(
				    nci, ddr_type, nci->timing_mode);
				if (ret) {
					PHY_ERR("_change nand onfi timing mode "
						"failed\n");
					PHY_ERR("nand flash switch to nv-ddr "
						"or nv-ddr2 failed!  %d\n",
						ddr_change_mode);
					return ret;
				}
			}
		} else if (ddr_change_mode == 2) {
			/* ONFI DDR/DDR2 => Async */
			PHY_DBG("mode 2 : ONFI DDR/DDR2 => Async\n");
			if ((nci->support_ddr2_specific_cfg) &&
			    (pre_ddr_type == ONFI_DDR2)) {
				ret = _setup_nand_onfi_ddr2_para(nci);
				if (ret != 0) {
					PHY_ERR("_setup_nand_onfi_ddr2_para() "
						"failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_io_driver_strength) {
				ret = _setup_nand_onfi_driver_strength(nci);
				if (ret != 0) {
					PHY_ERR("_setup_nand_onfi_driver_"
						"strength() failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_vendor_specific_cfg) {
				ret = _setup_nand_onfi_vendor_specific_feature(
				    nci);
				if (ret) {
					PHY_ERR("_setup_nand_onfi_vendor_"
						"specific_feature() failed! "
						"%d\n",
						ddr_change_mode);
					return ret;
				}
			}

			/* use async reset to aysnc interface */
			nand_reset_chip(nci);

			/* change to proper timing mode in async interface */
			if (nci->support_change_onfi_timing_mode) {
				ret = _change_nand_onfi_timing_mode(
				    nci, ddr_type, nci->timing_mode);
				if (ret) {
					PHY_ERR("_change_nand onfi timing_mode "
						" failed\n");
					PHY_ERR("nand flash change timing mode "
						"at async interface failed!  "
						"%d \n",
						ddr_change_mode);
					return ret;
				}
			}

		} else if (ddr_change_mode == 3) {
			/* Async => Toggle DDR/DDR2 */
			PHY_DBG("mode 3 : Async => Toggle DDR/DDR2\n");
			nand_reset_chip(nci);
			if (nci->support_ddr2_specific_cfg) {
				if (ddr_type == TOG_DDR2) {
					ret = _setup_nand_toggle_ddr2_para(nci);
					if (ret) {
						PHY_ERR("set nand onfi ddr2 "
							"parameter failed! "
							"%d\n",
							ddr_change_mode);
						return ret;
					}
				}
			}

			if (nci->support_io_driver_strength) {
				ret = _setup_nand_toggle_driver_strength(nci);
				if (ret) {
					PHY_ERR("_setup_nand_toggle_driver_"
						"strength() failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_vendor_specific_cfg) {
				ret =
				    _setup_nand_toggle_vendor_specific_feature(
					nci);
				if (ret) {
					PHY_ERR("_setup_nand_toggle_vendor_"
						"specific_feature() failed! "
						"%d\n",
						ddr_change_mode);
					return ret;
				}
			}

		} else if (ddr_change_mode == 4) {
			/* Toggle DDR/DDR2 => Async */
			PHY_DBG("mode 4 : Toggle DDR/DDR2 => Async\n");
			if ((nci->support_ddr2_specific_cfg) &&
			    (pre_ddr_type == TOG_DDR2)) {
				// clear ddr2 parameter
				ret = _setup_nand_toggle_ddr2_para(nci);
				if (ret != 0) {
					PHY_ERR("set nand onfi ddr2 parameter "
						"failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_io_driver_strength) {
				ret = _setup_nand_toggle_driver_strength(nci);
				if (ret) {
					PHY_ERR("_setup_nand_toggle_driver_"
						"strength() failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_vendor_specific_cfg) {
				ret =
				    _setup_nand_toggle_vendor_specific_feature(
					nci);
				if (ret) {
					PHY_ERR("_setup_nand_toggle_vendor_"
						"specific_feature() failed! "
						"%d\n",
						ddr_change_mode);
					return ret;
				}
			}
		} else if (ddr_change_mode == 5) {
			/* Toggle DDR2 <=> Toggle DDR */
			PHY_DBG("mode 5 : Toggle DDR2 <=> Toggle DDR\n");
			if (nci->support_ddr2_specific_cfg) {
				ret = _setup_nand_toggle_ddr2_para(nci);
				if (ret) {
					PHY_ERR("set nand onfi ddr2 parameter "
						"failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_io_driver_strength) {
				ret = _setup_nand_toggle_driver_strength(nci);
				if (ret) {
					PHY_ERR("_setup_nand_toggle_driver_"
						"strength() failed! %d\n",
						ddr_change_mode);
					return ret;
				}
			}

			if (nci->support_vendor_specific_cfg) {
				ret =
				    _setup_nand_toggle_vendor_specific_feature(
					nci);
				if (ret) {
					PHY_ERR("_setup_nand_toggle_vendor_"
						"specific_feature() failed! "
						"%d\n",
						ddr_change_mode);
					return ret;
				}
			}

		} else if (ddr_change_mode == 6) {
			PHY_DBG("mode 6\n");
			if (ddr_type == SDR) {
				if (nci->support_change_onfi_timing_mode) {
					ret = _change_nand_onfi_timing_mode(
					    nci, ddr_type, nci->timing_mode);
					if (ret) {
						PHY_ERR("nand flash change "
							"timing mode at async "
							"interface failed! %d  "
							"continue...\n",
							ddr_change_mode);
						return ret;
					}
				}
			} else if ((ddr_type == ONFI_DDR) ||
				   (ddr_type == ONFI_DDR2)) {
				if ((nci->support_ddr2_specific_cfg) &&
				    (ddr_type == ONFI_DDR2)) {
					ret = _setup_nand_onfi_ddr2_para(nci);
					if (ret) {
						PHY_ERR("set nand onfi ddr2 "
							"parameter failed! %d "
							"\n",
							ddr_change_mode);
						return ret;
					}
				}

				if (nci->support_io_driver_strength) {
					ret = _setup_nand_onfi_driver_strength(
					    nci);
					if (ret) {
						PHY_ERR("_setup_nand_onfi_"
							"driver_strength() "
							"failed! %d \n",
							ddr_change_mode);
						return ret;
					}
				}

				if (nci->support_vendor_specific_cfg) {
					ret =
					    _setup_nand_onfi_vendor_specific_feature(
						nci);
					if (ret) {
						PHY_ERR("_setup_nand_onfi_"
							"vendor_specific_"
							"feature() failed! %d "
							"\n",
							ddr_change_mode);
						return ret;
					}
				}

				if (nci->support_change_onfi_timing_mode) {
					ret = _change_nand_onfi_timing_mode(
					    nci, ddr_type, nci->timing_mode);
					if (ret) {
						PHY_ERR("nand flash change "
							"timing mode at "
							"nvddr/nvddr2 failed! "
							"%d ",
							ddr_change_mode);
						return ret;
					}
				}
			} else if ((ddr_type == TOG_DDR) ||
				   (ddr_type == TOG_DDR2)) {
				if ((nci->support_ddr2_specific_cfg) &&
				    (ddr_type == TOG_DDR2)) {
					ret = _setup_nand_toggle_ddr2_para(nci);
					if (ret) {
						PHY_ERR("set nand toggle ddr2 "
							"parameter failed! %d "
							"\n",
							ddr_change_mode);
						return ret;
					}
				}

				if (nci->support_io_driver_strength) {
					ret =
					    _setup_nand_toggle_driver_strength(
						nci);
					if (ret) {
						PHY_ERR("_setup_nand_toggle_"
							"driver_strength() "
							"failed! %d \n",
							ddr_change_mode);
						return ret;
					}
				}

				if (nci->support_vendor_specific_cfg) {
					ret =
					    _setup_nand_toggle_vendor_specific_feature(
						nci);
					if (ret) {
						PHY_ERR("_setup_nand_toggle_"
							"vendor_specific_"
							"feature() failed! %d "
							"\n",
							ddr_change_mode);
						return ret;
					}
				}
			} else {
				PHY_ERR("wrong nand interface type!\n");
			}

		} else {
			;
		}
		nci = nci->nctri_next;
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
s32 _get_right_timing_para(struct _nand_controller_info *nctri, u32 ddr_type,
			   u32 *good_sdr_edo, u32 *good_ddr_edo,
			   u32 *good_ddr_delay)
{
	u32 chip = 0;
	struct _nand_chip_info *nci = nctri->nci; // chip 0
	u32 *scan_blk_no = nctri->ddr_scan_blk_no;
	u32 edo, delay, edo_cnt, delay_cnt;
	u32 edo_delay[2][3], tmp_edo_delay[3];
	u32 good_cnt, tmp_good_cnt, store_index;
	s32 err_flag;
	u32 good_flag;
	u32 index, i, j;
	u8 tmpChipID[16];
	u32 param[2];
	s32 ret;
	u32 sclk0_bak, sclk1_bak;
	u32 sdr_edo, ddr_edo, ddr_delay, tmp;
	u8 *main_buf;

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 3; j++)
			edo_delay[i][j] = 0xffffffff;
	}
	for (j = 0; j < 3; j++)
		tmp_edo_delay[j] = 0xffffffff;

	good_flag = 0;
	index = 0;
	store_index = 0;
	good_cnt = 0;
	tmp_good_cnt = 0;
	edo_cnt = 0;
	delay_cnt = 0;
	param[0] = 0xffffffff;
	param[1] = 0xffffffff;

	if (ddr_type == SDR) {
		edo_cnt = 3;
		delay_cnt = 1;
		if (SUPPORT_SCAN_EDO_FOR_SDR_NAND == 0) {
			PHY_DBG("_get right timing para, set edo to 1 for sdr "
				"nand.\n");
			*good_sdr_edo = 1;
			return 0;
		}
	} else {
		delay_cnt = 64;
		if (NDFC_VERSION_V2 == nctri->type)
			edo_cnt = 32;
		else if (NDFC_VERSION_V1 == nctri->type)
			edo_cnt = 16; // 32
		else {
			PHY_ERR("wrong ndfc version, %d\n", nctri->type);
			return ERR_NO_61;
		}
	}

	main_buf = nand_get_temp_buf(32768);
	if (!main_buf) {
		PHY_ERR("main_buf 0x%x is null!\n", main_buf);
		return ERR_NO_60;
	}

	for (edo = 0; edo < edo_cnt; edo++) {
		good_flag = 0;
		for (delay = 0; delay < delay_cnt; delay += 2) {
			if (ddr_type == SDR) {
				sdr_edo = edo;
				ddr_edo = edo;
				tmp = sdr_edo << 8;
				ddr_delay = delay;
			} else {
				sdr_edo = edo;
				ddr_edo = edo;
				ddr_delay = delay;
				tmp = (ddr_edo << 8) | ddr_delay;
			}
			ndfc_change_nand_interface(nctri, ddr_type, sdr_edo,
						   ddr_edo, ddr_delay);
			nctri->ddr_timing_ctl[0] = tmp;

			// read id
			ret = nand_read_id(nci, tmpChipID); // chip 0
			if (ret) {
				PHY_ERR("read id failed! ---> continue\n");
				continue;
			}

			if ((nctri->nci->id[0] == tmpChipID[0]) &&
			    (nctri->nci->id[1] == tmpChipID[1]) &&
			    (nctri->nci->id[2] == tmpChipID[2]) &&
			    (nctri->nci->id[3] == tmpChipID[3])) {
				PHY_DBG("ddr edo:0x%x, delay:0x%x is ", edo,
					ddr_delay);
				err_flag = _check_scan_data(0, 0, scan_blk_no,
							    main_buf);
				if (err_flag == 0) {
					PHY_DBG("good\n");

					good_flag = 1;
					if (ddr_type == SDR) {// sdr mode
						if (edo == 0) {
							NAND_GetClk(
							    nctri->channel_id,
							    &sclk0_bak,
							    &sclk1_bak);
							PHY_DBG("sclk0 %d MHz, "
								"edo %d\n",
								sclk0_bak, edo);
							if (sclk0_bak <
							    12) // less 12MHz
								break;
							else
								good_flag = 0;
						}
						break;
					}

					if ((index != 0) && (index != 1))
						PHY_ERR("wrong index!\n");
					if (store_index == 0) {
						if (edo_delay[index][0] ==
						    0xffffffff) {// first found
							edo_delay[index][0] =
							    edo; // k;
							edo_delay[index][1] =
							    delay; // m;
							edo_delay[index][2] =
							    delay; // m;
						} else {
							edo_delay[index][2] =
							    delay; // m;
						}
						good_cnt++;
					} else if (store_index == 1) {
						if (tmp_edo_delay[0] ==
						    0xffffffff) {// first found
							tmp_edo_delay[0] =
							    edo; // k;
							tmp_edo_delay[1] =
							    delay; // m;
							tmp_edo_delay[2] =
							    delay; // m;
						} else {
							tmp_edo_delay[2] =
							    delay; // m;
						}
						tmp_good_cnt++;
					}
				} else {
					/* id is ok, but data is wrong */
					PHY_DBG("wrong %d\n", err_flag);
					if (good_cnt == 0) {
						store_index = 0;
					} else if (tmp_good_cnt == 0) {
						// store good {edo, delay} to
						// tmp_edo_delay[]
						store_index = 1;
					} else if (tmp_good_cnt > good_cnt) {
						// move tmp_edo_delay[] to
						// edo_delay[][]
						edo_delay[index][0] =
						    tmp_edo_delay[0];
						edo_delay[index][1] =
						    tmp_edo_delay[1];
						edo_delay[index][2] =
						    tmp_edo_delay[2];
						good_cnt = tmp_good_cnt;

						// clear tmp_edo_delay[] for
						// next valid group
						store_index = 1;
						tmp_good_cnt = 0;
						for (j = 0; j < 3; j++)
							tmp_edo_delay[j] =
							    0xffffffff;
					} else {
						store_index = 1;
						tmp_good_cnt = 0;
						for (j = 0; j < 3; j++)
							tmp_edo_delay[j] =
							    0xffffffff;
					}
				}
			} else {
				/* read id wrong */
				// PHY_ERR("timing_para read id wrong!\n");
				if (good_cnt == 0) {
					store_index = 0;
				} else if (tmp_good_cnt == 0) {
					// store good {edo, delay} to
					// tmp_edo_delay[]
					store_index = 1;
				} else if (tmp_good_cnt > good_cnt) {
					// move tmp_edo_delay[] to edo_delay[][]
					edo_delay[index][0] = tmp_edo_delay[0];
					edo_delay[index][1] = tmp_edo_delay[1];
					edo_delay[index][2] = tmp_edo_delay[2];
					good_cnt = tmp_good_cnt;

					// clear tmp_edo_delay[] for next valid
					// group
					store_index = 1;
					tmp_good_cnt = 0;
					for (j = 0; j < 3; j++)
						tmp_edo_delay[j] = 0xffffffff;
				} else {
					store_index = 1;
					tmp_good_cnt = 0;
					for (j = 0; j < 3; j++)
						tmp_edo_delay[j] = 0xffffffff;
				}
			}
		}

		if (good_flag) {
			if (ddr_type == SDR) // sdr mode
				break;

			if (index == 0) {
				if (good_cnt >=
				    GOOD_DDR_EDO_DELAY_CHAIN_TH) // 8 groups of
								 // {edo, delay}
								 // is enough
					break;
				index = 1;
				store_index = 0;
				good_cnt = 0;
				tmp_good_cnt = 0;
				for (j = 0; j < 3; j++)
					tmp_edo_delay[j] = 0xffffffff;
			} else {
				break;
			}
		}
	}

	if (ddr_type == SDR) {
		if (good_flag) {
			*good_sdr_edo = edo;
		}
		goto RET;
	}

	if ((edo_delay[0][0] == 0xffffffff) &&
	    (edo_delay[1][0] == 0xffffffff)) {
		good_flag = 0;
		PHY_ERR(
		    "can't find a good edo, delay chain. index %d:  %d %d %d\n",
		    index, edo_delay[index][0], edo_delay[index][1],
		    edo_delay[index][2]);
	} else if ((edo_delay[0][0] != 0xffffffff) &&
		   (edo_delay[1][0] != 0xffffffff)) {
		i = edo_delay[0][2] - edo_delay[0][1];
		j = edo_delay[1][2] - edo_delay[1][1];
		if (j > i) {
			param[0] = edo_delay[1][0];
			param[1] = (edo_delay[1][1] + edo_delay[1][2]) / 2 + 1;
			if (j >= GOOD_DDR_EDO_DELAY_CHAIN_TH)
				good_flag = 1;
			else
				good_flag = 0;
		} else {
			param[0] = edo_delay[0][0];
			param[1] = (edo_delay[0][1] + edo_delay[0][2]) / 2 + 1;
			if (j >= GOOD_DDR_EDO_DELAY_CHAIN_TH)
				good_flag = 1;
			else
				good_flag = 0;
		}
		PHY_DBG("(0x%x, 0x%x - 0x%x), (0x%x, 0x%x - 0x%x)\n",
			edo_delay[0][0], edo_delay[0][1], edo_delay[0][2],
			edo_delay[1][0], edo_delay[1][1], edo_delay[1][2]);
		if (good_flag)
			PHY_DBG("good edo: 0x%x, good delay chain: 0x%x\n",
				param[0], param[1]);
		else
			PHY_ERR("can't find a good edo, delay chain !\n");
	} else if ((edo_delay[0][0] != 0xffffffff) &&
		   (edo_delay[1][0] == 0xffffffff)) {
		i = edo_delay[0][2] - edo_delay[0][1];
		param[0] = edo_delay[0][0];
		param[1] = (edo_delay[0][1] + edo_delay[0][2]) / 2 + 1;
		PHY_DBG("(0x%x, 0x%x - 0x%x) \n", edo_delay[0][0],
			edo_delay[0][1], edo_delay[0][2]);
		if (i >= GOOD_DDR_EDO_DELAY_CHAIN_TH)
			good_flag = 1;
		else
			good_flag = 0;

		if (good_flag)
			PHY_DBG("good edo: 0x%x, good delay chain: 0x%x\n",
				param[0], param[1]);
		else
			PHY_ERR("can't find a good edo, delay chain!!\n");
	} else {
		good_flag = 0;
		PHY_ERR("scan error!!!!!!!\n");
	}

	*good_ddr_edo = param[0];
	*good_ddr_delay = param[1];

RET:
	nand_free_temp_buf(main_buf, 32768);
	if (good_flag == 0) {
		return ERR_NO_59;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  : for read retry
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         : one cmd; one addr; some data
*****************************************************************************/
int set_cmd_with_nand_bus(struct _nand_chip_info *nci, u8 *cmd, u32 wait_rb,
			  u8 *addr, u8 *dat, u32 dat_len, u32 counter)
{
	int ret, i, j;
	u8 *p_dat = dat;

	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	ndfc_repeat_mode_enable(nci->nctri);

	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;

	// PHY_DBG("set cmd with nand bus:");

	for (i = 0; i < counter; i++) {
		cmd_seq->nctri_cmd[i].cmd_valid = 1;
		if (cmd != NULL) {
			cmd_seq->nctri_cmd[i].cmd = cmd[i];
			cmd_seq->nctri_cmd[i].cmd_send = 1;
			if (wait_rb != 0) {
				cmd_seq->nctri_cmd[i].cmd_wait_rb = 1;
			}
		}

		if ((addr == NULL) && (dat == NULL)) {
			// PHY_DBG("cmd:0x%x;",cmd[i]);
		}

		if (addr != NULL) {
			cmd_seq->nctri_cmd[i].cmd_acnt = 1;
			cmd_seq->nctri_cmd[i].cmd_addr[0] = addr[i];

			// PHY_DBG("addr:0x%x;",addr[i]);
		}

		if (dat != NULL) {
			cmd_seq->nctri_cmd[i].cmd_trans_data_nand_bus = 1;
			cmd_seq->nctri_cmd[i].cmd_swap_data = 1;
			cmd_seq->nctri_cmd[i].cmd_direction = 1;
			cmd_seq->nctri_cmd[i].cmd_mdata_len = dat_len;
			cmd_seq->nctri_cmd[i].cmd_mdata_addr = p_dat;

			//            PHY_DBG("data:");
			//            for(j=0;j<dat_len;j++)
			//            {
			//                PHY_DBG("0x%x,",p_dat[j]);
			//            }

			p_dat += dat_len;
		}
	}

	// PHY_DBG("\n");

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);

	ndfc_repeat_mode_disable(nci->nctri);

	if (ret != 0) {
		PHY_ERR("set cmd with nand bus fail, "
			"cmd:0x%x;dat_len:0x%x;counter:0x%x;!\n",
			cmd[0], dat_len, counter);
		return ret;
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  : for read retry
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         : one cmd; one addr; some data
*****************************************************************************/
int get_data_with_nand_bus_one_cmd(struct _nand_chip_info *nci, u8 *cmd,
				   u8 *addr, u8 *dat, u32 dat_len)
{
	int ret, j;
	u8 *p_dat = dat;

	struct _nctri_cmd_seq *cmd_seq = &nci->nctri->nctri_cmd_seq;

	if (dat_len > 1024) {
		PHY_ERR("dat_len is too long %d!\n", dat_len);
		return ERR_NO_133;
	}

	ndfc_repeat_mode_enable(nci->nctri);

	if (nci->randomizer) {
		ndfc_disable_randomize(nci->nctri);
	}

	ndfc_clean_cmd_seq(cmd_seq);

	cmd_seq->cmd_type = CMD_TYPE_NORMAL;

	// PHY_DBG("set cmd with nand bus:");

	cmd_seq->nctri_cmd[0].cmd_valid = 1;
	if (cmd != NULL) {
		cmd_seq->nctri_cmd[0].cmd = *cmd;
		cmd_seq->nctri_cmd[0].cmd_send = 1;
		cmd_seq->nctri_cmd[0].cmd_wait_rb = 1;
	}

	if ((addr == NULL) && (dat == NULL)) {
		PHY_DBG("cmd:0x%x;", *cmd);
	}

	if (addr != NULL) {
		cmd_seq->nctri_cmd[0].cmd_acnt = 1;
		cmd_seq->nctri_cmd[0].cmd_addr[0] = *addr;

		// PHY_DBG("addr:0x%x;",addr[i]);
	}

	if (dat != NULL) {
		cmd_seq->nctri_cmd[0].cmd_trans_data_nand_bus = 1;
		cmd_seq->nctri_cmd[0].cmd_swap_data = 1;
		cmd_seq->nctri_cmd[0].cmd_direction = 0;
		cmd_seq->nctri_cmd[0].cmd_mdata_len = dat_len;
		cmd_seq->nctri_cmd[0].cmd_mdata_addr = p_dat;

		// PHY_DBG("data:");
		for (j = 0; j < dat_len; j++) {
			// PHY_DBG("0x%x,",p_dat[j]);
		}
	}

	// PHY_DBG("\n");

	ret = ndfc_execute_cmd(nci->nctri, cmd_seq);

	ndfc_repeat_mode_disable(nci->nctri);

	if (ret != 0) {
		PHY_ERR("get_data_with nand_bus_one_cmd fail, "
			"cmd:0x%x;dat_len:0x%x!\n",
			cmd[0], dat_len);
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
int set_one_cmd(struct _nand_chip_info *nci, u8 cmd, u32 wait_rb)
{
	int ret;
	u8 cmd_8[1];

	cmd_8[0] = cmd;
	ret = set_cmd_with_nand_bus(nci, cmd_8, wait_rb, NULL, NULL, 0, 1);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int set_one_addr(struct _nand_chip_info *nci, u8 addr)
{
	int ret;
	u8 addr_8[1];

	addr_8[0] = addr;
	ret = set_cmd_with_nand_bus(nci, NULL, 0, addr_8, NULL, 0, 1);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int switch_ddrtype_from_ddr_to_sdr(struct _nand_controller_info *nctri)
{
	int cfg, ddr_type, ddr_type_bak;

	ddr_type = SDR;

	if ((nctri->nci->support_toggle_only == 0) &&
	    (nctri->nci->interface_type > 0)) {
		//		PHY_DBG("switch_ddrtype_from_ddr_to_sdr start
		//!\n");
		ddr_type_bak = nctri->nci->interface_type;
		nctri->nci->interface_type = SDR;

		nctri->nreg_bak.reg_timing_ctl = 0x100;

		cfg = nctri->nreg_bak.reg_ctl;
		cfg &= ~(0x3U << 18);
		cfg |= (ddr_type & 0x3) << 18;
		if (nctri->type == NDFC_VERSION_V2) {
			cfg &= ~(0x1 << 28);
			cfg |= ((ddr_type >> 4) & 0x1) << 28;
		}
		nctri->nreg_bak.reg_ctl = cfg;

		_change_all_nand_parameter(nctri, ddr_type, ddr_type_bak, 40);

		nctri->nci->interface_type = ddr_type_bak;
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
int get_dummy_byte(int physic_page_size, int ecc_mode, int ecc_block_cnt,
		   int user_data_size)
{
	int ecc_code_size_per_1K, valid_size, dummy_byte;
	u8 ecc_tab[16] = {16, 24, 28, 32, 40, 44, 48, 52,
			  56, 60, 64, 68, 72, 76, 80};
	int ecc_bit;

	ecc_bit = ecc_tab[ecc_mode];
	ecc_code_size_per_1K = 14 * ecc_bit / 8;
	valid_size =
	    (1024 + ecc_code_size_per_1K) * ecc_block_cnt + user_data_size;
	dummy_byte = physic_page_size - valid_size;

	return dummy_byte;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int get_random_cmd2(struct _nand_physic_op_par *npo)
{
	return (npo->page % 3);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int get_data_block_cnt(unsigned int sect_bitmap)
{
	int i, count = 0;

	for (i = 0; i < 32; i++) {
		if ((sect_bitmap >> i) & 0x1)
			count++;
	}

	return count;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int get_data_block_cnt_for_boot0_ecccode(struct _nand_chip_info *nci,
					 u8 ecc_mode)
{
	int size_increased, cnt, i;
	u8 ecc_tab[16] = {16, 24, 28, 32, 40, 44, 48, 52,
			  56, 60, 64, 68, 72, 76, 80};
	int ecc_bit_boot0, ecc_bit_normal;

	ecc_bit_boot0 = ecc_tab[ecc_mode];
	ecc_bit_normal = ecc_tab[nci->ecc_mode];
	size_increased = 14 * (ecc_bit_boot0 - ecc_bit_normal) / 8;

	cnt = 1;
	for (i = (nci->sector_cnt_per_page / 2 - 1); i > 0; i--) {
		if ((size_increased * i) < (1024 * cnt))
			break;
		cnt++;
	}

	return cnt;
}
