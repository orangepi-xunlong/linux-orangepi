
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

#define _NPHYI_C_

#include "../physic_common/nand_common_interface.h"
#include "nand_physic_inc.h"

extern int nand_cfg_interface(void);
extern int nand_wait_rb_before(void);
extern int nand_wait_rb_mode(void);
extern int nand_wait_dma_mode(void);
extern int nand_support_two_plane(void);
extern int nand_support_vertical_interleave(void);
extern int nand_support_dual_channel(void);

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void nand_cfg_setting(void)
{
	g_phy_cfg = &aw_nand_info.nand_lib_cfg;
	aw_nand_info.nand_lib_cfg.phy_interface_cfg = nand_cfg_interface();

	aw_nand_info.nand_lib_cfg.phy_support_two_plane =
	    nand_support_two_plane();
	aw_nand_info.nand_lib_cfg.phy_nand_support_vertical_interleave =
	    nand_support_vertical_interleave();
	aw_nand_info.nand_lib_cfg.phy_support_dual_channel =
	    nand_support_dual_channel();

	aw_nand_info.nand_lib_cfg.phy_wait_rb_before = nand_wait_rb_before();
	aw_nand_info.nand_lib_cfg.phy_wait_rb_mode = nand_wait_rb_mode();
	aw_nand_info.nand_lib_cfg.phy_wait_dma_mode = nand_wait_dma_mode();
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
struct _nand_info *RawNandHwInit(void)
{
	int ret;

	nand_cfg_setting();

	ret = nand_physic_init();
	if (ret != 0) {
		PHY_ERR("nand_physic_init error %d\n", ret);
		return NULL;
	}

	aw_nand_info.type = 0;
	aw_nand_info.SectorNumsPerPage =
	    g_nssi->nsci->sector_cnt_per_super_page;
	aw_nand_info.BytesUserData = g_nssi->nsci->spare_bytes;
	aw_nand_info.BlkPerChip = g_nssi->nsci->blk_cnt_per_super_chip;

	aw_nand_info.ChipNum = g_nssi->super_chip_cnt;

	aw_nand_info.PageNumsPerBlk = g_nssi->nsci->page_cnt_per_super_blk;
	// aw_nand_info.FullBitmap = FULL_BITMAP_OF_SUPER_PAGE;

	// aw_nand_info.MaxBlkEraseTimes = 2000;
	aw_nand_info.MaxBlkEraseTimes =
	    g_nssi->nsci->nci_first->max_erase_times;

	// aw_nand_info.EnableReadReclaim = (g_nsi->nci->npi->operation_opt &
	// NAND_READ_RECLAIM) ? 1 : 0;
	aw_nand_info.EnableReadReclaim = 1;

	aw_nand_info.boot = phyinfo_buf;

	set_uboot_start_and_end_block();

	set_hynix_special_info();

	nand_secure_storage_init(1);

	PHY_DBG("RawNandHwInit end\n");

	return &aw_nand_info;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int RawNandHwExit(void)
{
	NAND_PhysicLock();
	nand_wait_all_rb_ready();
	nand_physic_exit();
	NAND_PhysicUnLock();
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int RawNandHwSuperStandby(void)
{
	struct _nand_controller_info *nctri = g_nctri;

	PHY_DBG("RawNandHwSuperStandby start\n");
	NAND_PhysicLock();
	nand_wait_all_rb_ready();

	while (nctri != NULL) {
		save_nctri(nctri);
		// show_nctri(nctri);
		// show_nci(nctri->nci);
		switch_ddrtype_from_ddr_to_sdr(nctri);
		NAND_ClkRelease(nctri->channel_id);
		NAND_PIORelease(nctri->channel_id);
		nctri = nctri->next;
	}

	NAND_ReleaseVoltage();

	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int RawNandHwSuperResume(void)
{
	struct _nand_controller_info *nctri;
	struct _nand_chip_info *nci;
	u8 spare[16];
	u8 *mbuf;
	u32 t;

	PHY_DBG("RawNandHwSuperResume start\n");
	NAND_GetVoltage();
	nctri = g_nctri;
	while (nctri != NULL) {
		NAND_PIORequest(nctri->channel_id);
		NAND_ClkRequest(nctri->channel_id);
		ndfc_soft_reset(nctri);
		recover_nctri(nctri);

		nci = nctri->nci;
		while (nci != NULL) {
			nand_reset_chip(nci);
			nci = nci->nctri_next;
		}
		nctri = nctri->next;
	}

	nctri = g_nctri;
	while (nctri != NULL) {
		update_nctri(nctri);
		// show_nctri(nctri);
		// show_nci(nctri->nci);
		nctri = nctri->next;
	}

	//////////////////////////////////////////
	//    nctri = g_nctri;
	//    while(nctri != NULL)
	//    {
	//        nci = nctri->nci;
	//        while(nci != NULL)
	//        {
	//            nand_reset_chip(nctri->nci);
	//            nci = nci->nctri_next;
	//        }
	//        nctri = nctri->next;
	//    }
	//////////////////////////////////////////
	//    if(g_nsi->nci->driver_no == 6)
	//    {
	//        nand_physic_read_page(0,7,2,g_nsi->nci->sector_cnt_per_page,NULL,spare);
	//    }

	nand_wait_all_rb_ready();
	NAND_PhysicUnLock();

	//    if(g_nsi->nci->driver_no == 6)
	//    {
	//        PHY_DBG("=======================\n");
	//        t = 0xfffff;
	//        while(--t);
	//        mbuf = nand_get_temp_buf(32*1024);
	//        nand_physic_read_page(0,7,2,g_nsi->nci->sector_cnt_per_page,mbuf,spare);
	//        nand_free_temp_buf(mbuf,32*1024);
	//    }

	PHY_DBG("RawNandHwSuperResume end\n");
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int RawNandHwNormalStandby(void)
{
	PHY_DBG("RawNandHwNormalStandby start\n");
	NAND_PhysicLock();
	nand_wait_all_rb_ready();
	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int RawNandHwNormalResume(void)
{
	PHY_DBG("RawNandHwNormalResume start\n");
	NAND_PhysicUnLock();
	return 0;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int RawNandHwShutDown(void)
{
	PHY_DBG("RawNandHwShutDown start\n");
	NAND_PhysicLock();
	nand_physic_exit();
	nand_wait_all_rb_ready();
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void do_nand_interrupt(u32 no)
{
	struct _nand_controller_info *nctri = nctri_get(g_nctri, no);

	u32 rb = ndfc_get_selected_rb_no(nctri);

	if (ndfc_check_rb_b2r_int_occur(nctri) != 0) {
		ndfc_clear_rb_b2r_int(nctri);

		ndfc_disable_rb_b2r_int(nctri);

		nctri->rb_ready_flag = 1;

		//        PHY_DBG("do %d\n",no);

		nand_rb_wake_up(no);
	}

	if (ndfc_check_dma_int_occur(nctri) != 0) {
		ndfc_clear_dma_int(nctri);

		ndfc_disable_dma_int(nctri);

		nctri->dma_ready_flag = 1;

		nand_dma_wake_up(no);
	}
}
