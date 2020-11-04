
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


#include "nand_scan.h"
#include "../physic_common/nand_common_interface.h"
#include "spic.h"

extern struct __NandStorageInfo_t NandStorageInfo;

extern struct __NandPhyInfoPar_t DefaultNandTbl;
extern struct __NandPhyInfoPar_t GigaDeviceNandTbl;
extern struct __NandPhyInfoPar_t AtoNandTbl;
extern struct __NandPhyInfoPar_t MicronNandTbl;
extern struct __NandPhyInfoPar_t WinbondNandTbl;
extern struct __NandPhyInfoPar_t MxicNandTbl;

extern __u32 storage_type;

extern struct _boot_info *phyinfo_buf;

__u32 NandIDNumber = 0xffffff;
__u32 NandSupportTwoPlaneOp;
__u32 CurrentDriverTwoPlaneOPCfg;

__u32 spinand_get_twoplane_flag(void)
{
	return SUPPORT_MULTI_PROGRAM ? 1 : 0;
}

__u32 SPINAND_GetLsbblksize(void)
{
	return (NandStorageInfo.SectorCntPerPage * 512 *
		NandStorageInfo.PageCntPerPhyBlk);
}

__u32 SPINAND_GetLsbPages(void)
{
	return NandStorageInfo.PageCntPerPhyBlk;
}

__u32 SPINAND_UsedLsbPages(void)
{
	return 0;
}

u32 SPINAND_GetPageNo(u32 lsb_page_no)
{
	return lsb_page_no;
}

__u32 NAND_GetPlaneCnt(void)
{
	return NandStorageInfo.PlaneCntPerDie;
}

__u32 SPINAND_GetPageSize(void)
{
	return (NandStorageInfo.SectorCntPerPage * 512);
}

__u32 SPINAND_GetPhyblksize(void)
{
	return (NandStorageInfo.SectorCntPerPage * 512 *
		NandStorageInfo.PageCntPerPhyBlk);
}

__u32 SPINAND_GetPageCntPerBlk(void)
{
	return NandStorageInfo.PageCntPerPhyBlk;
}

__u32 SPINAND_GetBlkCntPerChip(void)
{
	return NandStorageInfo.BlkCntPerDie * NandStorageInfo.DieCntPerChip;
}

__u32 SPINAND_GetChipCnt(void)
{
	return NandStorageInfo.ChipCnt;
}

__u32 NAND_GetChipConnect(void)
{
	return NandStorageInfo.ChipConnectInfo;
}

__u32 NAND_GetBadBlockFlagPos(void)
{
	return 2;
}

__u32 NAND_GetFrequencePar(void)
{
	return NandStorageInfo.FrequencePar;
}

__s32 NAND_SetFrequencePar(__u32 FrequencePar)
{
	NandStorageInfo.FrequencePar = (__u8)FrequencePar;
	return 0;
}

__s32 NAND_GetBlkCntOfDie(void)
{
	return NandStorageInfo.BlkCntPerDie;
}

__u32 NAND_GetOperationOpt(void)
{
	return NandStorageInfo.OperationOpt;
}

__u32 NAND_GetNandVersion(void)
{
	__u32 nand_version;

	nand_version = 0;
	nand_version |= 0xff;
	nand_version |= 0x00 << 8;
	nand_version |= NAND_VERSION_0 << 16;
	nand_version |= NAND_VERSION_1 << 24;

	return nand_version;
}

__u32 NAND_GetVersion(__u8 *nand_version)
{
	__u32 ret;

	ret = NAND_GetNandVersion();
	*(__u32 *)nand_version = ret;

	return ret;
}

__u32 NAND_GetNandVersionDate(void)
{
	return 0x20121209;
}

#if 0
__s32 NAND_GetParam(boot_spinand_para_t *nand_param)
{
	__u32 i;

	nand_param->ChipCnt            =   NandStorageInfo.ChipCnt;
	nand_param->ChipConnectInfo    =   NandStorageInfo.ChipConnectInfo;
	nand_param->ConnectMode        =   NandStorageInfo.ConnectMode;
	nand_param->BankCntPerChip     =   NandStorageInfo.BankCntPerChip;
	nand_param->DieCntPerChip      =   NandStorageInfo.DieCntPerChip;
	nand_param->PlaneCntPerDie     =   NandStorageInfo.PlaneCntPerDie;
	nand_param->SectorCntPerPage   =   NandStorageInfo.SectorCntPerPage;
	nand_param->PageCntPerPhyBlk   =   NandStorageInfo.PageCntPerPhyBlk;
	nand_param->BlkCntPerDie       =   NandStorageInfo.BlkCntPerDie;
	nand_param->OperationOpt       =   NandStorageInfo.OperationOpt;
	nand_param->FrequencePar       =   NandStorageInfo.FrequencePar;
	nand_param->SpiMode            =   NandStorageInfo.SpiMode;
	nand_param->MaxEraseTimes      =   NandStorageInfo.MaxEraseTimes;
	nand_param->MultiPlaneBlockOffset = NandStorageInfo.MultiPlaneBlockOffset;
	nand_param->pagewithbadflag	   =   NandStorageInfo.pagewithbadflag;
	nand_param->EccLimitBits	   =   NandStorageInfo.EccLimitBits;
	nand_param->MaxEccBits		   =   NandStorageInfo.MaxEccBits;
//	nand_param->spi_nand_function  =   NandStorageInfo.spi_nand_function;

//	spic_set_trans_mode(0, NandStorageInfo.SpiMode);

	for (i = 0; i < 8; i++)
	    nand_param->NandChipId[i]   =   NandStorageInfo.NandChipId[i];

	return 0;
}
#endif

__s32 NAND_GetFlashInfo(boot_flash_info_t *param)
{
	param->chip_cnt = NandStorageInfo.ChipCnt;
	param->blk_cnt_per_chip =
	    NandStorageInfo.BlkCntPerDie * NandStorageInfo.DieCntPerChip;
	param->blocksize = SECTOR_CNT_OF_SINGLE_PAGE * PAGE_CNT_OF_PHY_BLK;
	param->pagesize = SECTOR_CNT_OF_SINGLE_PAGE;
	param->pagewithbadflag = NandStorageInfo.pagewithbadflag;

	return 0;
}
__s32 _GetOldPhysicArch(void *phy_arch, __u32 *good_blk_no)
{
	__s32 ret, ret2 = 0;
	__u32 b, chip = 0;
	__u32 start_blk = 20, blk_cnt = 30;
	__u8 oob[32];
	struct __NandStorageInfo_t *parch;
	struct boot_physical_param nand_op;

	// parch = (struct __NandStorageInfo_t *)PHY_TMP_PAGE_CACHE;
	parch = (struct __NandStorageInfo_t *)MALLOC(32 * 1024);
	if (!parch) {
		PRINT("%s,malloc fail\n", __func__);
		ret2 = -1;
		goto EXIT;
	}

	for (b = start_blk; b < start_blk + blk_cnt; b++) {
		nand_op.chip = chip;
		nand_op.block = b;
		nand_op.page = 0;
		nand_op.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
		nand_op.mainbuf = (void *)parch;
		nand_op.oobbuf = oob;

		ret =
		    PHY_SimpleRead(&nand_op); // PHY_SimpleRead_CurCH(&nand_op);
		PHY_DBG("_GetOldPhysicArch: chip %d, block %d, page 0, oob: "
			"0x%x, 0x%x, 0x%x, 0x%x\n",
			nand_op.chip, nand_op.block, oob[0], oob[1], oob[2],
			oob[3]);
		if (ret >= 0) {
			if (oob[0] == 0x00) {
				if ((oob[1] == 0x50) && (oob[2] == 0x48) &&
				    (oob[3] == 0x59) && (oob[4] == 0x41) &&
				    (oob[5] == 0x52) && (oob[6] == 0x43) &&
				    (oob[7] == 0x48)) {
					//*((struct __NandStorageInfo_t
					//*)phy_arch) = *parch;
					if ((parch->PlaneCntPerDie != 1) &&
					    (parch->PlaneCntPerDie != 2)) {
						PHY_DBG("_GetOldPhysicArch: "
							"get old physic arch "
							"ok,but para error: "
							"0x%x 0x%x!\n",
							parch->OperationOpt,
							parch->PlaneCntPerDie);
					} else {
						MEMCPY(
						    phy_arch, parch,
						    sizeof(
							struct
							__NandStorageInfo_t));
						PHY_DBG("_GetOldPhysicArch: "
							"get old physic arch "
							"ok, 0x%x 0x%x!\n",
							parch->OperationOpt,
							parch->PlaneCntPerDie);
						ret2 = 1;
						break;
					}
				} else {
					PHY_DBG("_GetOldPhysicArch: mark bad "
						"block!\n");
				}
			} else if (oob[0] == 0xff) {
				PHY_DBG("_GetOldPhysicArch: find a good block, "
					"but no physic arch info.\n");
				ret2 = 2; // blank page
				break;
			} else {
				PHY_DBG("_GetOldPhysicArch: unkonwn1!\n");
			}
		} else {
			if (oob[0] == 0xff) {
				PHY_DBG("_GetOldPhysicArch: blank block!\n");
				ret2 = 2;
				break;
			} else if (oob[0] == 0) {
				PHY_DBG("_GetOldPhysicArch: bad block!\n");
			} else {
				PHY_DBG("_GetOldPhysicArch: unkonwn2!\n");
			}
		}
	}

	if (b == (start_blk + blk_cnt)) {
		ret2 = -1;
		*good_blk_no = 0;
	} else
		*good_blk_no = b;

EXIT:
	FREE(parch, 32 * 1024);

	return ret2;
}

__s32 _SetNewPhysicArch(void *phy_arch)
{
	__s32 ret;
	__u32 i, b, p, chip = 0;
	__u32 start_blk = 20, blk_cnt = 30;
	__u8 oob[32];
	__u32 good_blk_no;
	struct __NandStorageInfo_t *parch;
	struct __NandStorageInfo_t arch_tmp = {0};
	struct boot_physical_param nand_op;

	// parch = (struct __NandStorageInfo_t *)PHY_TMP_PAGE_CACHE;
	parch = (struct __NandStorageInfo_t *)MALLOC(32 * 1024);

	/* in order to get good block, get old physic arch info */
	ret = _GetOldPhysicArch(&arch_tmp, &good_blk_no);
	if (ret == -1) {
		/* can not find good block */
		PHY_ERR("_SetNewPhysicArch: can not find good block: 12~112\n");
		FREE(parch, 32 * 1024);
		return ret;
	}

	PHY_DBG("_SetNewPhysicArch: write physic arch to blk %d...\n",
		good_blk_no);

	for (b = good_blk_no; b < start_blk + blk_cnt; b++) {
		nand_op.chip = chip;
		nand_op.block = b;
		nand_op.page = 0;
		nand_op.sectorbitmap = FULL_BITMAP_OF_SINGLE_PAGE;
		nand_op.mainbuf = (void *)parch; // PHY_TMP_PAGE_CACHE;
		nand_op.oobbuf = oob;

		ret = PHY_SimpleErase(
		    &nand_op); // PHY_SimpleErase_CurCH(&nand_op);
		if (ret < 0) {
			PHY_ERR("_SetNewPhysicArch: erase chip %d, block %d "
				"error\n",
				nand_op.chip, nand_op.block);

			for (i = 0; i < 32; i++)
				oob[i] = 0x0;

			for (p = 0; p < NandStorageInfo.PageCntPerPhyBlk; p++) {
				nand_op.page = p;
				ret = PHY_SimpleWrite(
				    &nand_op);//PHY_SimpleWrite_CurCH(&nand_op);
				if (ret < 0) {
					PHY_ERR("_SetNewPhysicArch: mark bad "
						"block, write chip %d, block "
						"%d, page %d error\n",
						nand_op.chip, nand_op.block,
						nand_op.page);
				}
			}
		} else {
			PHY_DBG("_SetNewPhysicArch: erase block %d ok.\n", b);

			for (i = 0; i < 32; i++)
				oob[i] = 0x88;
			oob[0] = 0x00; // bad block flag
			oob[1] = 0x50; // 80; //'P'
			oob[2] = 0x48; // 72; //'H'
			oob[3] = 0x59; // 89; //'Y'
			oob[4] = 0x41; // 65; //'A'
			oob[5] = 0x52; // 82; //'R'
			oob[6] = 0x43; // 67; //'C'
			oob[7] = 0x48; // 72; //'H'

			// MEMSET(parch, 0x0, 1024);
			MEMCPY(parch, phy_arch,
			       sizeof(struct __NandStorageInfo_t));
			//*parch = *((struct __NandStorageInfo_t *)phy_arch);
			for (p = 0; p < NandStorageInfo.PageCntPerPhyBlk; p++) {
				nand_op.page = p;
				ret = PHY_SimpleWrite(
				    &nand_op);//PHY_SimpleWrite_CurCH(&nand_op);
				if (ret < 0) {
					PHY_ERR("_SetNewPhysicArch: write chip "
						"%d, block %d, page %d error\n",
						nand_op.chip, nand_op.block,
						nand_op.page);
					FREE(parch, 32 * 1024);
					return -1;
				}
			}
			break;
		}
	}

	PHY_DBG("_SetNewPhysicArch: ============\n");
	ret = _GetOldPhysicArch(&arch_tmp, &good_blk_no);
	if (ret == -1) {
		/* can not find good block */
		PHY_ERR("_SetNewPhysicArch: can not find good block: 12~112\n");
		FREE(parch, 32 * 1024);
		return ret;
	}

	FREE(parch, 32 * 1024);

	return 0;
}

__s32 _UpdateExtMultiPlanePara(void)
{
	__u32 id_number_ctl;
	__u32 script_twoplane, para;

	id_number_ctl = NAND_GetNandIDNumCtrl();
	if (0x0 != id_number_ctl) {
		if (id_number_ctl & (1U << 1)) {// bit 1, set twoplane para
			para = NAND_GetNandExtPara(1);
			if (0xffffffff != para) {// get script success
				if (((para & 0xffffff) == NandIDNumber) ||
				    ((para & 0xffffff) == 0xeeeeee)) {
					script_twoplane = (para >> 24) & 0xff;
					PHY_DBG("_UpdateExtMultiPlanePara: get "
						"twoplane para from script "
						"success: ",
						script_twoplane);
					if (script_twoplane == 1) {
						PHY_DBG("%d\n",
							script_twoplane);
						if (NandSupportTwoPlaneOp) {
							// PHY_DBG("NAND_UpdatePhyArch:
							// current nand support
							// two plane op!\n");
							NandStorageInfo
							    .PlaneCntPerDie = 2;
							NandStorageInfo
							    .OperationOpt |=
							    SPINAND_MULTI_READ;
							NandStorageInfo
							    .OperationOpt |=
							    SPINAND_MULTI_PROGRAM;
						} else {
							PHY_DBG(
							    "_UpdateExtMultiPla"
							    "nePara: current "
							    "nand do not "
							    "support two plane "
							    "op, set to 0!\n");
							NandStorageInfo
							    .PlaneCntPerDie = 1;
							NandStorageInfo
							    .OperationOpt &=
							    ~SPINAND_MULTI_READ;
							NandStorageInfo
							    .OperationOpt &=
							    ~SPINAND_MULTI_PROGRAM;
						}
					} else if (script_twoplane == 0) {
						PHY_DBG("%d\n",
							script_twoplane);
						NandStorageInfo.PlaneCntPerDie =
						    1;
						NandStorageInfo.OperationOpt &=
						    ~SPINAND_MULTI_READ;
						NandStorageInfo.OperationOpt &=
						    ~SPINAND_MULTI_PROGRAM;
					} else {
						PHY_DBG("%d, wrong "
							"parameter(0,1)\n",
							script_twoplane);
						return -1;
					}
				} else {
					PHY_ERR("_UpdateExtMultiPlanePara: "
						"wrong id number, 0x%x/0x%x\n",
						(para & 0xffffff),
						NandIDNumber);
					return -1;
				}
			} else {
				PHY_ERR("_UpdateExtMultiPlanePara: wrong two "
					"plane para, 0x%x\n",
					para);
				return -1;
			}
		} else {
			PHY_ERR("_UpdateExtMultiPlanePara: wrong id ctrl "
				"number: %d/%d\n",
				id_number_ctl, (1U << 1));
			return -1;
		}
	} else {
		PHY_ERR("_UpdateExtMultiPlanePara: no para.\n");
		return -1;
	}

	return 0;
}

__s32 _UpdateExtAccessFreqPara(void)
{
	__u32 id_number_ctl;
	__u32 script_frequence, para;

	id_number_ctl = NAND_GetNandIDNumCtrl();
	if (0x0 != id_number_ctl) {
		if (id_number_ctl & (1U << 0)) { // bit 0, set freq para
			para = NAND_GetNandExtPara(0);
			if (0xffffffff != para) { // get script success
				if (((para & 0xffffff) == NandIDNumber) ||
				    ((para & 0xffffff) == 0xeeeeee)) {

					script_frequence = (para >> 24) & 0xff;
					if ((script_frequence > 10) &&
					    (script_frequence < 100)) {
						NandStorageInfo.FrequencePar =
						    script_frequence;
						PHY_ERR("_UpdateExtAccessFreqPa"
							"ra: update freq from "
							"script, %d\n",
							script_frequence);
					} else {
						PHY_ERR("_UpdateExtAccessFreqPa"
							"ra: wrong freq, %d\n",
							script_frequence);
						return -1;
					}

				} else {
					PHY_ERR("_UpdateExtAccessFreqPara: "
						"wrong id number, 0x%x/0x%x\n",
						(para & 0xffffff),
						NandIDNumber);
					return -1;
				}
			} else {
				PHY_ERR("_UpdateExtAccessFreqPara: wrong freq "
					"para, 0x%x\n",
					para);
				return -1;
			}
		} else {
			PHY_ERR("_UpdateExtAccessFreqPara: wrong id ctrl "
				"number, %d/%d.\n",
				id_number_ctl, (1U << 0));
			return -1;
		}
	} else {
		PHY_DBG("_UpdateExtAccessFreqPara: no para.\n");
		return -1;
	}

	return 0;
}
#if 0
__s32 NAND_UpdatePhyArch(void)
{
	__s32 ret = 0;

	/*
	 * when erase chip during update firmware, it means that we will ignore
	 * previous physical archtecture, erase all good blocks and write new
	 * data.
	 *
	 * we should write new physical architecture to block 12~12+100 for
	 * next update.
	 */
	ret = _UpdateExtMultiPlanePara();
	if (ret < 0) {
		if (CurrentDriverTwoPlaneOPCfg) {
			NandStorageInfo.PlaneCntPerDie = 2;
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_PROGRAM;
		} else {
			NandStorageInfo.PlaneCntPerDie = 1;
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
		}
		PHY_ERR("NAND_UpdatePhyArch: get script error,"
			"use current driver cfg!\n");
	}
	PHY_ERR("NAND_UpdatePhyArch: before set new arch: 0x%x 0x%x.\n",
		NandStorageInfo.OperationOpt, NandStorageInfo.PlaneCntPerDie);
	ret = _SetNewPhysicArch(&NandStorageInfo);
	if (ret < 0)
		PHY_ERR("NAND_UpdatePhyArch: write physic arch to nand failed!\n");

	return ret;
}
#endif
__s32 NAND_ReadPhyArch(void)
{
	__s32 ret = 0;
	struct __NandStorageInfo_t old_storage_info = {0};
	__u32 good_blk_no;
	struct _spinand_config_para_info config_para;

	if (is_phyinfo_empty(phyinfo_buf) != 1) {
		NandStorageInfo.FrequencePar =
		    phyinfo_buf->storage_info.config.frequence;

		if (phyinfo_buf->storage_info.config.support_two_plane == 1) {
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_PROGRAM;
			NandStorageInfo.PlaneCntPerDie = 2;
		} else {
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
			NandStorageInfo.PlaneCntPerDie = 1;
		}

		if (phyinfo_buf->storage_info.config.support_dual_read == 1)
			NandStorageInfo.OperationOpt |= SPINAND_DUAL_READ;
		else
			NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_READ;

		if (phyinfo_buf->storage_info.config.support_dual_write == 1)
			NandStorageInfo.OperationOpt |= SPINAND_DUAL_PROGRAM;
		else
			NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_PROGRAM;

		if (phyinfo_buf->storage_info.config.support_quad_write == 1)
			NandStorageInfo.OperationOpt |= SPINAND_QUAD_PROGRAM;
		else
			NandStorageInfo.OperationOpt &= ~SPINAND_QUAD_PROGRAM;

		if (phyinfo_buf->storage_info.config.support_quad_read == 1)
			NandStorageInfo.OperationOpt |= SPINAND_QUAD_READ;
		else
			NandStorageInfo.OperationOpt &= ~SPINAND_QUAD_READ;

		return 0;
	}

	ret = _GetOldPhysicArch(&old_storage_info, &good_blk_no);
	if (ret == 1) {
		PHY_ERR("NAND_ReadPhyArch: get old physic arch ok, use old "
			"cfg, now:0x%x 0x%x - old:0x%x 0x%x!\n",
			NandStorageInfo.PlaneCntPerDie,
			NandStorageInfo.OperationOpt,
			old_storage_info.PlaneCntPerDie,
			old_storage_info.OperationOpt);
		NandStorageInfo.PlaneCntPerDie =
		    old_storage_info.PlaneCntPerDie;
		if (NandStorageInfo.PlaneCntPerDie == 1) {
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
		}
		if (NandStorageInfo.PlaneCntPerDie == 2) {
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_READ;
			NandStorageInfo.OperationOpt |= SPINAND_MULTI_PROGRAM;
		}
	} else if (ret == 2) {
		PHY_ERR("NAND_ReadPhyArch: blank page!\n");
	} else {
		PHY_ERR("NAND_ReadPhyArch: get para error!\n");
	}
	return ret;
}

/*
 ******************************************************************************
 *                           SEARCH NAND PHYSICAL ARCHITECTURE PARAMETER
 *
 *Description: Search the nand flash physical architecture parameter from the
 *parameter table
 *             by nand chip ID.
 *
 *Arguments  : pNandID           the pointer to nand flash chip ID;
 *             pNandArchiInfo    the pointer to nand flash physical architecture
 *parameter.
 *
 *Return     : search result;
 *               = 0     search successful, find the parameter in the table;
 *               < 0     search failed, can't find the parameter in the table.
 ******************************************************************************
 */
__s32 _SearchNandArchi(__u8 *pNandID, struct __NandPhyInfoPar_t *pNandArchInfo)
{
	__s32 i = 0, j = 0, k = 0;
	__u32 id_match_tbl[5] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
	__u32 id_bcnt;
	struct __NandPhyInfoPar_t *tmpNandManu;

	// analyze the manufacture of the nand flash
	switch (pNandID[0]) {
	case GD_NAND:
		tmpNandManu = &GigaDeviceNandTbl;
		break;

	case ATO_NAND:
		tmpNandManu = &AtoNandTbl;
		break;

	case MICRON_NAND:
		tmpNandManu = &MicronNandTbl;
		break;

	case WINBOND_NAND:
		tmpNandManu = &WinbondNandTbl;
		break;

	case MXIC_NAND:
		tmpNandManu = &MxicNandTbl;
		break;

	// manufacture is unknown, search parameter from default nand table
	default:
		tmpNandManu = &DefaultNandTbl;
		break;
	}

	MEMCPY(pNandArchInfo, tmpNandManu, sizeof(struct __NandPhyInfoPar_t));

#if 1
	// search the nand architecture parameter from the given manufacture
	// nand table by nand ID
	while (tmpNandManu[i].NandID[0] != 0xff) {
		// compare 6 byte id
		id_bcnt = 1;
		for (j = 1; j < 6; j++) {
			// 0xff is matching all ID value
			if ((pNandID[j] != tmpNandManu[i].NandID[j]) &&
			    (tmpNandManu[i].NandID[j] != 0xff))
				break;

			if (tmpNandManu[i].NandID[j] != 0xff)
				id_bcnt++;
		}

		if (j == 6) {
			/*4 bytes of the nand chip ID are all matching, search
			 * parameter successful
			 * */
			if (id_bcnt == 2)
				id_match_tbl[0] = i;
			else if (id_bcnt == 3)
				id_match_tbl[1] = i;
			else if (id_bcnt == 4)
				id_match_tbl[2] = i;
			else if (id_bcnt == 5)
				id_match_tbl[3] = i;
			else if (id_bcnt == 6)
				id_match_tbl[4] = i;
		}

		// prepare to search the next table item
		i++;
	}

	for (k = 4; k >= 0; k--) {
		if (id_match_tbl[k] != 0xffff) {
			i = id_match_tbl[k];
			MEMCPY(pNandArchInfo, tmpNandManu + i,
			       sizeof(struct __NandPhyInfoPar_t));
			return 0;
		}
	}

#endif

	// search nand architecture parameter failed
	return -1;
}

/*
 ******************************************************************************
 *                           ANALYZE NAND FLASH STORAGE SYSTEM
 *
 *Description: Analyze nand flash storage system, generate the nand flash
 *physical
 *             architecture parameter and connect information.
 *
 *Arguments  : none
 *
 *Return     : analyze result;
 *               = 0     analyze successful;
 *               < 0     analyze failed, can't recognize or some other error.
 ******************************************************************************
 */
// struct __NandPhyInfoPar_t tmpNandPhyInfo;
__s32 SCN_AnalyzeNandSystem(void)
{
	__s32 i, result;
	__u8 tmpChipID[8];
	__u8 status_lock;
	__u8 status_otp;
	//	__u8  uniqueID[32];
	struct __NandPhyInfoPar_t tmpNandPhyInfo;
	//   __u32 val[8];

	// init nand flash storage information to default value
	NandStorageInfo.ChipCnt = 1;
	NandStorageInfo.ChipConnectInfo = 1;
	NandStorageInfo.ConnectMode = 1;
	//    NandStorageInfo.RbCnt= 1;
	//    NandStorageInfo.RbConnectInfo= 1;
	NandStorageInfo.BankCntPerChip = 1;
	NandStorageInfo.DieCntPerChip = 1;
	NandStorageInfo.PlaneCntPerDie = 1;
	NandStorageInfo.SectorCntPerPage = 4;
	NandStorageInfo.PageCntPerPhyBlk = 64;
	NandStorageInfo.BlkCntPerDie = 1024;
	NandStorageInfo.OperationOpt = 0;
	NandStorageInfo.FrequencePar = 10;
	NandStorageInfo.SpiMode = 0;
	NandStorageInfo.pagewithbadflag = 0;
	NandStorageInfo.MultiPlaneBlockOffset = 1;
	NandStorageInfo.MaxEraseTimes = 50000;
	NandStorageInfo.MaxEccBits = 1;
	NandStorageInfo.EccLimitBits = 1;
	//	NandStorageInfo.ReadRetryType= 0;

	// read nand flash chip ID from boot chip
	result = PHY_ReadNandId_0(BOOT_CHIP_SELECT_NUM, tmpChipID);
	if (result) {
		PHY_ERR("read id fail 0\n");
		return -1;
	}

	PHY_DBG("SPI nand ID: %x %x\n", *((__u32 *)tmpChipID),
		*((__u32 *)tmpChipID + 1));

	// search the nand flash physical architecture parameter by nand ID
	result = _SearchNandArchi(tmpChipID, &tmpNandPhyInfo);
	if (result) {
		// read nand flash chip ID from boot chip
		result = PHY_ReadNandId_1(BOOT_CHIP_SELECT_NUM, tmpChipID);
		if (result) {
			PHY_ERR("read id fail 1\n");
			return -1;
		}

		// search the nand flash physical architecture parameter by nand
		// ID
		result = _SearchNandArchi(tmpChipID, &tmpNandPhyInfo);
		if (result) {
			PHY_ERR("_SearchNandArchi fail\n");
			return -1;
		}
	}

	storage_type = 2;

	// set the nand flash physical architecture parameter
	NandStorageInfo.BankCntPerChip = tmpNandPhyInfo.DieCntPerChip;
	NandStorageInfo.DieCntPerChip = tmpNandPhyInfo.DieCntPerChip;
	NandStorageInfo.PlaneCntPerDie = 2;
	NandStorageInfo.SectorCntPerPage = tmpNandPhyInfo.SectCntPerPage;
	NandStorageInfo.PageCntPerPhyBlk = tmpNandPhyInfo.PageCntPerBlk;
	NandStorageInfo.BlkCntPerDie = tmpNandPhyInfo.BlkCntPerDie;
	NandStorageInfo.OperationOpt = tmpNandPhyInfo.OperationOpt;
	NandStorageInfo.FrequencePar = tmpNandPhyInfo.AccessFreq;
	NandStorageInfo.NandChipId[0] = tmpNandPhyInfo.NandID[0];
	NandStorageInfo.NandChipId[1] = tmpNandPhyInfo.NandID[1];
	NandStorageInfo.NandChipId[2] = tmpNandPhyInfo.NandID[2];
	NandStorageInfo.NandChipId[3] = tmpNandPhyInfo.NandID[3];
	NandStorageInfo.NandChipId[4] = tmpNandPhyInfo.NandID[4];
	NandStorageInfo.NandChipId[5] = tmpNandPhyInfo.NandID[5];
	NandStorageInfo.NandChipId[6] = tmpNandPhyInfo.NandID[6];
	NandStorageInfo.NandChipId[7] = tmpNandPhyInfo.NandID[7];
	NandStorageInfo.SpiMode = tmpNandPhyInfo.SpiMode;
	NandStorageInfo.pagewithbadflag = tmpNandPhyInfo.pagewithbadflag;
	NandStorageInfo.MaxEraseTimes = tmpNandPhyInfo.MaxEraseTimes;
	NandStorageInfo.MaxEccBits = tmpNandPhyInfo.MaxEccBits;
	NandStorageInfo.EccLimitBits = tmpNandPhyInfo.EccLimitBits;
	NandStorageInfo.MultiPlaneBlockOffset =
	    tmpNandPhyInfo.MultiPlaneBlockOffset;
	NandStorageInfo.spi_nand_function = tmpNandPhyInfo.spi_nand_function;
	NandStorageInfo.Idnumber = tmpNandPhyInfo.Idnumber;

	Spic_set_trans_mode(0, NandStorageInfo.SpiMode);

	// reset the nand flash chip on boot chip select
	result = PHY_ResetChip(BOOT_CHIP_SELECT_NUM);
	//    result |= PHY_SynchBank(BOOT_CHIP_SELECT_NUM);
	if (result)
		return -1;

	/* set max block erase cnt and enable read reclaim flag */
	//   MaxBlkEraseTimes = tmpNandPhyInfo.MaxEraseTimes;
	/* in order to support to parse external script, record id ctl number */
	NandIDNumber = tmpNandPhyInfo.Idnumber;

	/* record current nand flash whether support two plane program */
	if (NandStorageInfo.OperationOpt & SPINAND_MULTI_PROGRAM)
		NandSupportTwoPlaneOp = 1;
	else
		NandSupportTwoPlaneOp = 0;

	/* record current driver cfg for two plane operation */
	if (CFG_SUPPORT_MULTI_PLANE_PROGRAM == 0)
		CurrentDriverTwoPlaneOPCfg = 0;
	else {
		if (NandSupportTwoPlaneOp)
			CurrentDriverTwoPlaneOPCfg = 1;
		else
			CurrentDriverTwoPlaneOPCfg = 0;
	}
	PHY_DBG(
	    "[SCAN_DBG] NandTwoPlaneOp: %d, DriverTwoPlaneOPCfg: %d, 0x%x\n",
	    NandSupportTwoPlaneOp, CurrentDriverTwoPlaneOPCfg,
	    ((NandIDNumber << 4) ^ 0xffffffff));

	/* update access frequency from script */
	if (SUPPORT_UPDATE_EXTERNAL_ACCESS_FREQ)
		_UpdateExtAccessFreqPara();

	if (!CFG_SUPPORT_READ_RECLAIM)
		NandStorageInfo.OperationOpt &= ~SPINAND_READ_RECLAIM;

	if (!CFG_SUPPORT_DUAL_PROGRAM)
		NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_PROGRAM;

	if (!CFG_SUPPORT_DUAL_READ)
		NandStorageInfo.OperationOpt &= ~SPINAND_DUAL_READ;

	for (i = 1; i < MAX_CHIP_SELECT_CNT; i++) {
		// read the nand chip ID from current nand flash chip
		PHY_ReadNandId_0((__u32)i, tmpChipID);
		// check if the nand flash id same as the boot chip
		if ((tmpChipID[0] == NandStorageInfo.NandChipId[0]) &&
		    (tmpChipID[1] == NandStorageInfo.NandChipId[1]) &&
		    ((tmpChipID[2] == NandStorageInfo.NandChipId[2]) ||
		     (NandStorageInfo.NandChipId[2] == 0xff)) &&
		    ((tmpChipID[4] == NandStorageInfo.NandChipId[3]) ||
		     (NandStorageInfo.NandChipId[3] == 0xff)) &&
		    ((tmpChipID[4] == NandStorageInfo.NandChipId[4]) ||
		     (NandStorageInfo.NandChipId[4] == 0xff)) &&
		    ((tmpChipID[5] == NandStorageInfo.NandChipId[5]) ||
		     (NandStorageInfo.NandChipId[5] == 0xff))) {
			NandStorageInfo.ChipCnt++;
			NandStorageInfo.ChipConnectInfo |= (1 << i);
		} else {
			// reset current nand flash chip
			PHY_ResetChip((__u32)i);

			PHY_ReadNandId_1((__u32)i, tmpChipID);
			if ((tmpChipID[0] == NandStorageInfo.NandChipId[0]) &&
			    (tmpChipID[1] == NandStorageInfo.NandChipId[1]) &&
			    ((tmpChipID[2] == NandStorageInfo.NandChipId[2]) ||
			     (NandStorageInfo.NandChipId[2] == 0xff)) &&
			    ((tmpChipID[4] == NandStorageInfo.NandChipId[3]) ||
			     (NandStorageInfo.NandChipId[3] == 0xff)) &&
			    ((tmpChipID[4] == NandStorageInfo.NandChipId[4]) ||
			     (NandStorageInfo.NandChipId[4] == 0xff)) &&
			    ((tmpChipID[5] == NandStorageInfo.NandChipId[5]) ||
			     (NandStorageInfo.NandChipId[5] == 0xff))) {
				NandStorageInfo.ChipCnt++;
				NandStorageInfo.ChipConnectInfo |= (1 << i);
			}
		}

		// reset current nand flash chip
		PHY_ResetChip((__u32)i);
	}

	// process the rb connect information
	{
		NandStorageInfo.ConnectMode = 0xff;

		if ((NandStorageInfo.ChipCnt == 1) &&
		    (NandStorageInfo.ChipConnectInfo & (1 << 0))) {
			NandStorageInfo.ConnectMode = 1;
		} else if (NandStorageInfo.ChipCnt == 2) {
			if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
			    (NandStorageInfo.ChipConnectInfo & (1 << 1)))
				NandStorageInfo.ConnectMode = 2;
			else if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
				 (NandStorageInfo.ChipConnectInfo & (1 << 2)))
				NandStorageInfo.ConnectMode = 3;
			else if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
				 (NandStorageInfo.ChipConnectInfo & (1 << 3)))
				NandStorageInfo.ConnectMode = 4;
			else if ((NandStorageInfo.ChipConnectInfo & (1 << 0)) &&
				 (NandStorageInfo.ChipConnectInfo & (1 << 4)))
				NandStorageInfo.ConnectMode = 5;

		}

		else if (NandStorageInfo.ChipCnt == 4) {
			NandStorageInfo.ConnectMode = 6;
		}

		if (NandStorageInfo.ConnectMode == 0xff) {
			PHY_ERR("%s : check spi nand connect fail, ChipCnt =  "
				"%x, ChipConnectInfo = %x\n",
				__FUNCTION__, NandStorageInfo.ChipCnt,
				NandStorageInfo.ChipConnectInfo);
			return -1;
		}
	}

	if (!CFG_SUPPORT_MULTI_PLANE_PROGRAM) {
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
	}
	// process the plane count of a die and the bank count of a chip
	if (!SUPPORT_MULTI_PROGRAM)
		NandStorageInfo.PlaneCntPerDie = 1;

	for (i = 0; i < NandStorageInfo.ChipCnt; i++) {
		// NandStorageInfo.spi_nand_function->spi_nand_getblocklock(0,
		// i, &status_lock);
		// NandStorageInfo.spi_nand_function->spi_nand_getotp(0, i,
		// &status_otp);
		NandStorageInfo.spi_nand_function->spi_nand_setblocklock(0, i,
									 0);
		NandStorageInfo.spi_nand_function->spi_nand_setotp(
		    0, i, 0x18); // winbond:0x18,bit3 is BUF mode;
				 // other:0x10,bit3 don't care
		NandStorageInfo.spi_nand_function->spi_nand_getblocklock(
		    0, i, &status_lock);
		NandStorageInfo.spi_nand_function->spi_nand_getotp(0, i,
								   &status_otp);
	}

	// process the external inter-leave operation
	if (CFG_SUPPORT_EXT_INTERLEAVE) {
		if (NandStorageInfo.ChipCnt > 1) {
			NandStorageInfo.OperationOpt |= SPINAND_EXT_INTERLEAVE;
		} else {
			NandStorageInfo.OperationOpt &=
			    (~SPINAND_EXT_INTERLEAVE);
		}
	} else {
		NandStorageInfo.OperationOpt &= (~SPINAND_EXT_INTERLEAVE);
	}
	PHY_ChangeMode();
	PHY_Scan_DelayMode(NAND_ACCESS_FREQUENCE);

	physic_info_read();

	if (SUPPORT_UPDATE_WITH_OLD_PHYSIC_ARCH)
		NAND_ReadPhyArch();

	if ((!CFG_SUPPORT_MULTI_PLANE_PROGRAM) ||
	    (SECTOR_CNT_OF_SINGLE_PAGE > 8)) {
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_READ;
		NandStorageInfo.OperationOpt &= ~SPINAND_MULTI_PROGRAM;
	}
	// process the plane count of a die and the bank count of a chip
	if (!SUPPORT_MULTI_PROGRAM)
		NandStorageInfo.PlaneCntPerDie = 1;

	// print nand flash physical architecture parameter
	SCAN_DBG("\n\n");
	SCAN_DBG("[SCAN_DBG] ==============Nand Architecture "
		 "Parameter==============\n");
	SCAN_DBG("[SCAN_DBG]    Nand Chip ID:         0x%x 0x%x\n",
		 (NandStorageInfo.NandChipId[0] << 0) |
		     (NandStorageInfo.NandChipId[1] << 8) |
		     (NandStorageInfo.NandChipId[2] << 16) |
		     (NandStorageInfo.NandChipId[3] << 24),
		 (NandStorageInfo.NandChipId[4] << 0) |
		     (NandStorageInfo.NandChipId[5] << 8) |
		     (NandStorageInfo.NandChipId[6] << 16) |
		     (NandStorageInfo.NandChipId[7] << 24));
	SCAN_DBG("[SCAN_DBG]    Nand Chip Count:      0x%x\n",
		 NandStorageInfo.ChipCnt);
	SCAN_DBG("[SCAN_DBG]    Nand Chip Connect:    0x%x\n",
		 NandStorageInfo.ChipConnectInfo);
	SCAN_DBG("[SCAN_DBG]    Sector Count Of Page: 0x%x\n",
		 NandStorageInfo.SectorCntPerPage);
	SCAN_DBG("[SCAN_DBG]    Page Count Of Block:  0x%x\n",
		 NandStorageInfo.PageCntPerPhyBlk);
	SCAN_DBG("[SCAN_DBG]    Block Count Of Die:   0x%x\n",
		 NandStorageInfo.BlkCntPerDie);
	SCAN_DBG("[SCAN_DBG]    Plane Count Of Die:   0x%x\n",
		 NandStorageInfo.PlaneCntPerDie);
	SCAN_DBG("[SCAN_DBG]    Die Count Of Chip:    0x%x\n",
		 NandStorageInfo.DieCntPerChip);
	SCAN_DBG("[SCAN_DBG]    Bank Count Of Chip:   0x%x\n",
		 NandStorageInfo.BankCntPerChip);
	SCAN_DBG("[SCAN_DBG]    Optional Operation:   0x%x\n",
		 NandStorageInfo.OperationOpt);
	SCAN_DBG("[SCAN_DBG]    Access Frequence:     0x%x\n",
		 NandStorageInfo.FrequencePar);
	SCAN_DBG("[SCAN_DBG] "
		 "=======================================================\n\n");

	return 0;
}
