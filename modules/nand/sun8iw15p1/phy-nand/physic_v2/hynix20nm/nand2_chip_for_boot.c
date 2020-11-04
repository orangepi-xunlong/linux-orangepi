
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

#define _NCFB2_C_

#include "../nand_physic_inc.h"

struct boot_ndfc_cfg {
	u8 page_size_kb;
	u8 ecc_mode;
	u8 sequence_mode;
	u8 res[5];
};

int m2_write_boot0_one(unsigned char *buf, unsigned int len,
		       unsigned int counter)
{
	__u32 i, k, j, count, block, chip_no;
	__u8 oob_buf[64];
	__u32 tab_size, data_size_per_page;
	__u32 pages_per_block, copies_per_block;
	__u32 page_addr;
	__u32 *pos_data = NULL, *tab = NULL, *rr_tab = NULL;
	//	__u8 *data_FF_buf=NULL;
	int ret;
	struct boot_ndfc_cfg cfg;
	struct _nand_chip_info *nci;
	struct _nand_physic_op_par lnpo;

	nci = g_nctri->nci;

#if 0
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

	PHY_DBG("burn_boot0_lsb_enable_pagetab mode!\n");

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

#if 0
	data_FF_buf = MALLOC(18048);
	if (data_FF_buf == NULL) {
		MALLOC("data_FF_buf malloc error!");
		goto error;
	}

	for (i = 0; i < (16384+1664); i++)
		*((__u8 *)data_FF_buf + i) = 0xFF;
#endif

	for (i = 0; i < 64; i++)
		oob_buf[i] = 0xff;

	/* get nand driver version */
	nand_get_version(oob_buf);
	if ((oob_buf[0] != 0xff) || (oob_buf[1] != 0x00)) {
		PHY_ERR("get flash driver version error!");
		goto error;
	}

	// lsb enable
	PHY_DBG("lsb enalbe \n");
	PHY_DBG("read retry mode: 0x%x\n", m2_read_retry_mode);

	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	m2_lsb_init(nci);
	m2_lsb_enable(nci);
	nci->nctri_chip_no = chip_no;

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
			}
		}
	}

	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	m2_lsb_disable(nci);
	m2_lsb_exit(nci);
	nci->nctri_chip_no = chip_no;

	PHY_DBG("lsb disalbe \n");

	nci->nctri->channel_sel = 0;
	ndfc_channel_select(nci->nctri, 0);

	if (pos_data)
		FREE(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		FREE(tab, 8 * 1024);
	if (rr_tab)
		FREE(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		FREE(data_FF_buf,18048);
	return 0;

error:
	chip_no = nci->nctri_chip_no;
	nci->nctri_chip_no = 0;
	m2_lsb_disable(nci);
	m2_lsb_exit(nci);
	nci->nctri_chip_no = chip_no;

	PHY_DBG("lsb disalbe \n");

	if (pos_data)
		FREE(pos_data, 128 * 4 * BOOT0_MAX_COPY_CNT);
	if (tab)
		FREE(tab, 8 * 1024);
	if (rr_tab)
		FREE(rr_tab, 8 * 1024);
	//	if(data_FF_buf)
	//		FREE(data_FF_buf,18048);
	return -1;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
