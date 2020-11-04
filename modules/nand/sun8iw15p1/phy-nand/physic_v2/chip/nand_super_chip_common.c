
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

/*
************************************************************************************************************************
*                                                      eNand
*                                           Nand flash driver scan module
*
*                             Copyright(C), 2008-2009, SoftWinners
*Microelectronic Co., Ltd.
*                                                  All Rights Reserved
*
* File Name : nand_chip_function.c
*
* Author :
*
* Version : v0.1
*
* Date : 2013-11-20
*
* Description :
*
* Others : None at present.
*
*
*
************************************************************************************************************************
*/

#define _NSC_C_

#include "../nand_physic_inc.h"

extern int is_phyinfo_empty(struct _boot_info *info);
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nsci_add_to_nssi(struct _nand_super_storage_info *nssi,
		     struct _nand_super_chip_info *node)
{
	struct _nand_super_chip_info *nsci;

	node->chip_no = 0;
	if (nssi->nsci == NULL) {
		nssi->nsci = node;
		return NAND_OP_TRUE;
	}

	node->chip_no = 1;
	nsci = nssi->nsci;
	while (nsci->nssi_next != NULL) {
		nsci = nsci->nssi_next;
		node->chip_no++;
	}
	nsci->nssi_next = node;
	return NAND_OP_TRUE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
struct _nand_super_chip_info *
nsci_get_from_nssi(struct _nand_super_storage_info *nssi, unsigned int num)
{
	int i;
	struct _nand_super_chip_info *nsci;

	for (i = 0, nsci = nssi->nsci; i < num; i++)
		nsci = nsci->nssi_next;

	return nsci;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int get_nand_structure(struct _nand_super_storage_info *nssi)
{
	int good_block, ret;

	struct __RAWNandStorageInfo_t *nand_storage;

	nssi->support_two_plane = 1;
	nssi->support_v_interleave = 1;
	nssi->support_dual_channel = 1;

	if (is_phyinfo_empty(phyinfo_buf) != 1) {
		nssi->support_two_plane =
		    phyinfo_buf->storage_info.data.support_two_plane;
		nssi->support_v_interleave =
		    phyinfo_buf->storage_info.data.support_v_interleave;
		nssi->support_dual_channel =
		    phyinfo_buf->storage_info.data.support_dual_channel;
		return 0;
	}

	ret = read_nand_structure((void *)(&nand_permanent_data),
				  (u32 *)(&good_block));
	if ((ret == 0) || (ret == 2)) {
		PHY_DBG("get nand structure ok!\n");
		if (nand_permanent_data.magic_data ==
		    MAGIC_DATA_FOR_PERMANENT_DATA) {
			PHY_DBG("get nand structure 1!\n");
			nssi->support_two_plane =
			    nand_permanent_data.support_two_plane;
			nssi->support_v_interleave =
			    nand_permanent_data.support_vertical_interleave;
			nssi->support_dual_channel =
			    nand_permanent_data.support_dual_channel;
		} else {
			nand_storage = (struct __RAWNandStorageInfo_t
					    *)(&nand_permanent_data);
			if (nand_storage->PlaneCntPerDie == 2) {
				PHY_DBG("get nand structure 2 %d!\n",
					nand_storage->PlaneCntPerDie);
				nssi->support_two_plane = 1;
				MEMSET((void *)&nand_permanent_data, 0,
				       sizeof(struct _nand_permanent_data));
				nand_permanent_data.magic_data =
				    MAGIC_DATA_FOR_PERMANENT_DATA;
				nand_permanent_data.support_two_plane = 1;
				nand_permanent_data
				    .support_vertical_interleave = 1;
				nand_permanent_data.support_dual_channel = 1;
			} else {
				PHY_DBG("get nand structure 3 0x%x!\n",
					nand_storage->PlaneCntPerDie);
				MEMSET((void *)&nand_permanent_data, 0,
				       sizeof(struct _nand_permanent_data));
				nand_permanent_data.magic_data =
				    MAGIC_DATA_FOR_PERMANENT_DATA;
				nand_permanent_data.support_two_plane = 0;
				nand_permanent_data
				    .support_vertical_interleave = 1;
				nand_permanent_data.support_dual_channel = 1;
			}
		}
	}
	//    else
	//    {
	//        PHY_ERR("get nand structure fail!\n");
	//        return ERR_NO_131;
	//    }

	phyinfo_buf->storage_info.data.support_two_plane =
	    nssi->support_two_plane;
	phyinfo_buf->storage_info.data.support_v_interleave =
	    nssi->support_v_interleave;
	phyinfo_buf->storage_info.data.support_dual_channel =
	    nssi->support_dual_channel;

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int init_nsci_from_nctri(struct _nand_super_storage_info *nssi,
			 struct _nand_super_chip_info *nsci,
			 struct _nand_controller_info *nctri,
			 unsigned int channel_num, unsigned int chip_no,
			 unsigned int nsci_num_in_nctri)
{
	int rb1, rb2, rb3, rb0;
	struct _nand_chip_info *nci;

	if (chip_no >= nsci_num_in_nctri) {
		chip_no -= nsci_num_in_nctri;
		nctri = nctri->next;
	}

	if (channel_num == 1) {
		nsci->channel_num = 1;
		nsci->dual_channel = 0;
		nssi->support_dual_channel = 0;

		nci = nci_get_from_nctri(nctri, chip_no);
		nsci->nci_first = nci;
		nci->nsci = nsci;
	} else {// must be channel_num == 2
		nsci->channel_num = 2;

		nci = nci_get_from_nctri(nctri, chip_no);
		nsci->nci_first = nci;
		nci->nsci = nsci;

		if (nssi->support_dual_channel != 0) {
			nsci->dual_channel = 1;
			nci = nci_get_from_nctri(nctri, chip_no);
			nsci->d_channel_nci_1 = nci;
			nci->nsci = nsci;

			nci = nci_get_from_nctri(nctri->next, chip_no);
			nsci->d_channel_nci_2 = nci;
			nci->nsci = nsci;
		}
	}

	if (nssi->support_v_interleave != 0) {
		nsci->vertical_interleave = 1;

		if (nctri->chip_cnt == 2) {
			nci = nci_get_from_nctri(nctri, chip_no);
			nsci->v_intl_nci_1 = nci;
			nci->nsci = nsci;
			nci = nci_get_from_nctri(nctri, chip_no + 1);
			nsci->v_intl_nci_2 = nci;
			nci->nsci = nsci;
		} else {// must be  nctri->chip_cnt == 4
			rb0 = nctri->rb[0];
			rb1 = nctri->rb[1];
			rb2 = nctri->rb[2];
			rb3 = nctri->rb[3];
			if (rb0 == rb1) {// rb0 == rb1
				nci = nci_get_from_nctri(nctri, chip_no);
				nsci->v_intl_nci_1 = nci;
				nci->nsci = nsci;
				nci = nci_get_from_nctri(nctri, chip_no + 2);
				nsci->v_intl_nci_2 = nci;
				nci->nsci = nsci;
			} else {// must be rb0 == rb2
				nci = nci_get_from_nctri(nctri, (chip_no << 1));
				nsci->v_intl_nci_1 = nci;
				nci->nsci = nsci;
				nci = nci_get_from_nctri(nctri,
							 (chip_no << 1) + 1);
				nsci->v_intl_nci_2 = nci;
				nci->nsci = nsci;
			}
		}
	}

	nsci->driver_no = nsci->nci_first->driver_no;
	nsci->blk_cnt_per_super_chip = nctri->nci->blk_cnt_per_chip;
	nsci->sector_cnt_per_super_page = nctri->nci->sector_cnt_per_page;
	nsci->page_cnt_per_super_blk = nctri->nci->page_cnt_per_blk;
	nsci->page_offset_for_next_super_blk =
	    nctri->nci->page_offset_for_next_blk;

	if (nsci->dual_channel == 1) {
		nsci->sector_cnt_per_super_page <<= 1;
	}

	if ((nssi->support_two_plane != 0) &&
	    (nsci->nci_first->npi->operation_opt & NAND_MULTI_PROGRAM) &&
	    (nsci->nci_first->opt_phy_op_par->multi_plane_block_offset == 1)) {
		nsci->two_plane = 1;
	} else {
		nsci->two_plane = 0;
	}

	if (nsci->sector_cnt_per_super_page >
	    MAX_SECTORS_PER_PAGE_FOR_TWO_PLANE) {
		nsci->two_plane = 0;
	}

	if (nctri->nci->sector_cnt_per_page == 4) {
		nsci->two_plane = 1;
	}

	if (nsci->two_plane == 1) {
		nsci->blk_cnt_per_super_chip >>= 1;
		nsci->sector_cnt_per_super_page <<= 1;
	} else {
		nssi->support_two_plane = 0;
	}

	if (nssi->support_two_plane == 0) {
		nssi->plane_cnt = 1;
	} else {
		nssi->plane_cnt = 2;
	}

	if (nsci->vertical_interleave == 1) {
		nsci->page_cnt_per_super_blk <<= 1;
		nsci->page_offset_for_next_super_blk <<= 1;
	}

	nsci->spare_bytes = 16;

	nsci->nand_physic_erase_super_block =
	    nand_physic_function[nsci->driver_no].nand_physic_erase_super_block;
	nsci->nand_physic_read_super_page =
	    nand_physic_function[nsci->driver_no].nand_physic_read_super_page;
	nsci->nand_physic_write_super_page =
	    nand_physic_function[nsci->driver_no].nand_physic_write_super_page;
	nsci->nand_physic_super_bad_block_check =
	    nand_physic_function[nsci->driver_no]
		.nand_physic_super_bad_block_check;
	nsci->nand_physic_super_bad_block_mark =
	    nand_physic_function[nsci->driver_no]
		.nand_physic_super_bad_block_mark;

	return NAND_OP_TRUE;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
规则：

1.一个通道内需要贴同一种flash
2.两个通道应该贴同样数目和类型的flash

单通道
1.支持 two-plane
2.支持 vertical_interleave
3.如果超级页超过32k，不支持two-plane
4.vertical_interleave 通道内rb不相同的chip配对

双通道
1.支持 two-plane
2.支持dual_channel
3.支持vertical_interleave
4.如果超级页超过32k，不支持two-plane
5.dual_channel 通道间chip0和chip0配对
6.vertical_interleave 通道内rb不相同的chip配对
*****************************************************************************/
int nand_build_nssi(struct _nand_super_storage_info *nssi,
		    struct _nand_controller_info *nctri)
{
	int rb1, rb2, rb3, rb0, ret;
	int i, channel_num, nsci_num, nsci_num_in_nctri;
	struct _nand_controller_info *nctri_temp;
	struct _nand_super_chip_info *nsci;

	ret = get_nand_structure(nssi);
	if (ret != 0) {
		return ret;
	}

	nsci_num = 0;

	for (channel_num = 0, nctri_temp = nctri; nctri_temp;
	     nctri_temp = nctri_temp->next) {
		if (nctri_temp->chip_cnt != 0)
			channel_num++;
	}

	if (nctri->chip_cnt == 1) {
		nssi->support_v_interleave = 0;
	} else if (nctri->chip_cnt == 2) {
		rb0 = nctri->rb[0];
		rb1 = nctri->rb[1];
		if (rb0 == rb1) {
			nssi->support_v_interleave = 0;
		}
	} else {// must be nctri->chip_cnt == 4
		rb0 = nctri->rb[0];
		rb1 = nctri->rb[1];
		rb2 = nctri->rb[2];
		rb3 = nctri->rb[3];
		if ((rb0 == rb1) && (rb0 == rb2)) {
			nssi->support_v_interleave = 0;
		}
	}

	if (channel_num == 1) {
		if ((nctri->chip_cnt == 1) || (nctri->chip_cnt == 2) ||
		    (nctri->chip_cnt == 4)) {
			if (nssi->support_v_interleave != 0) {
				nsci_num = nctri->chip_cnt / 4 + 1;
			} else {
				nsci_num = nctri->chip_cnt;
			}
		} else {
			PHY_ERR("not support chip_cnt1 %d\n", nctri->chip_cnt);
			return NAND_OP_FALSE;
		}
		nsci_num_in_nctri = nsci_num;
	} else if (channel_num == 2) {
		if ((nctri->chip_cnt == 1) || (nctri->chip_cnt == 2) ||
		    (nctri->chip_cnt == 4)) {
			if (nssi->support_dual_channel != 0) {
				nsci_num = nctri->chip_cnt;
			} else {
				nsci_num = nctri->chip_cnt << 1;
			}

			nsci_num_in_nctri = nctri->chip_cnt;

			if (nssi->support_v_interleave != 0) {
				nsci_num >>= 1;
				nsci_num_in_nctri >>= 1;
			}
		} else {
			PHY_ERR("not support chip_cnt2 %d\n", nctri->chip_cnt);
			return NAND_OP_FALSE;
		}
	} else {
		PHY_ERR("not support channel_num %d\n", channel_num);
		return NAND_OP_FALSE;
	}

	for (i = 0; i < nsci_num; i++) {
		// nsci = (struct _nand_super_chip_info *)MALLOC(sizeof(struct
		// _nand_super_chip_info));
		nsci = &nsci_data[i];
		if (nsci == NULL) {
			PHY_ERR("no memory for nssi\n");
		}

		MEMSET(nsci, 0, sizeof(struct _nand_super_chip_info));

		init_nsci_from_nctri(nssi, nsci, nctri, channel_num, i,
				     nsci_num_in_nctri);

		nsci_add_to_nssi(nssi, nsci);

		nsci = nsci->nssi_next;
	}

	nssi->super_chip_cnt = nsci_num;

	if (nsci_num == 0) {
		PHY_ERR("not support chip_cnt %d %d\n", channel_num,
			nctri->chip_cnt);
		return NAND_OP_FALSE;
	} else {
		return NAND_OP_TRUE;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_build_storage_info(void)
{
	g_nand_storage_info->ChannelCnt = g_nssi->nsci->channel_num;
	g_nand_storage_info->ChipCnt = g_nsi->chip_cnt;
	g_nand_storage_info->ChipConnectInfo = g_nctri->chip_connect_info;
	g_nand_storage_info->RbCnt = g_nsi->chip_cnt;
	g_nand_storage_info->RbConnectInfo = g_nctri->rb_connect_info;
	g_nand_storage_info->RbConnectMode = 0;
	g_nand_storage_info->BankCntPerChip = 1;
	g_nand_storage_info->DieCntPerChip = g_nsi->nci->npi->die_cnt_per_chip;
	g_nand_storage_info->PlaneCntPerDie = 1;
	g_nand_storage_info->SectorCntPerPage =
	    g_nsi->nci->npi->sect_cnt_per_page;
	g_nand_storage_info->PageCntPerPhyBlk =
	    g_nsi->nci->npi->page_cnt_per_blk;
	g_nand_storage_info->BlkCntPerDie = g_nsi->nci->npi->blk_cnt_per_die;
	g_nand_storage_info->OperationOpt = g_nsi->nci->npi->operation_opt;
	g_nand_storage_info->FrequencePar = g_nsi->nci->npi->access_freq;
	g_nand_storage_info->EccMode = g_nsi->nci->npi->ecc_mode;
	g_nand_storage_info->ValidBlkRatio = g_nsi->nci->npi->valid_blk_ratio;
	g_nand_storage_info->ReadRetryType = g_nsi->nci->npi->read_retry_type;
	g_nand_storage_info->DDRType = g_nsi->nci->npi->ddr_type;
	g_nand_storage_info->random_addr_num = g_nsi->nci->npi->random_addr_num;
	g_nand_storage_info->random_cmd2_send_flag =
	    g_nsi->nci->npi->random_cmd2_send_flag;
	g_nand_storage_info->nand_real_page_size =
	    g_nsi->nci->npi->nand_real_page_size;

	MEMCPY(g_nand_storage_info->NandChipId, g_nsi->nci->id, 8);
	MEMCPY(&g_nand_storage_info->OptPhyOpPar, g_nsi->nci->opt_phy_op_par,
	       sizeof(struct _optional_phy_op_par));

	return 0;
}
