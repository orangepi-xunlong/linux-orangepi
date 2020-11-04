/*
 * nand spi interface
 * (C) Copyright 2007-2011
 * Allwinnertech Technology Co., Ltd. <www.allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "../phy/phy.h"
#include "../physic_common/nand_common_interface.h"
#include "nand_format.h"
#include "nand_physic.h"
#include "nand_scan.h"
#include "spinand_drv_cfg.h"
//#define NAND_REG_LENGTH     (0xC4>>2)
#define SPIC_REG_LENGTH (0x40 >> 2)

__u32 MaxBlkEraseTimes;

// struct _nand_info aw_nand_info = {0};
extern struct _nand_info aw_nand_info;
extern struct _boot_info *phyinfo_buf;

__u32 spic_reg_back[SPIC_REG_LENGTH] = {0};

extern struct __NandStorageInfo_t NandStorageInfo;

extern __u32 SuperBlkCntOfDie;

extern int nand_secure_storage_init(int flag);

struct _nand_info *SpiNandHwInit(void)
// void NandHwInit(void)
{
	__s32 ret = 0;

	//	FORMAT_ERR("%s: nand driver physical layer version: %x, %x, %x,
	//%02d:%02d\n", __func__,
	//		NAND_DRV_PHYSIC_LAYER_VERSION_0,
	//NAND_DRV_PHYSIC_LAYER_VERSION_1, NAND_DRV_PHYSIC_LAYER_DATE,
	//		NAND_DRV_PHYSIC_LAYER_DATE_HOUR,
	//NAND_DRV_PHYSIC_LAYER_MINUTE);

	FORMAT_DBG("%s: Start Nand Hardware initializing .....\n", __func__);

	//	val[0] = NAND_DRV_PHYSIC_LAYER_VERSION_0;
	//	val[1] = NAND_DRV_PHYSIC_LAYER_VERSION_1;
	//	val[2] = NAND_DRV_PHYSIC_LAYER_DATE;
	//	val[3] = NAND_DRV_PHYSIC_LAYER_TIME;
	//	NAND_ShowEnv(0, "physical version", 4, val);
	NAND_Print_Version();

	if (PHY_Init()) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: PHY_Init() failed!\n", __func__);
		goto ERR_RET;
	}

	if (SCN_AnalyzeNandSystem()) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: SCN_AnalyzeNandSystem() failed!\n",
			   __func__);
		goto ERR_RET;
	}

// PHY_ChangeMode(1);

#if 0
	ret = PHY_ScanDDRParam();
	if (ret < 0) {
		ret = -1;
		FORMAT_ERR("[ERR]%s: PHY_ScanDDRParam() failed!\n", __func__);
		goto ERR_RET;
	}
#endif

	FMT_Init();

	MEMSET(&aw_nand_info, 0x0, sizeof(struct _nand_info));

	aw_nand_info.type = 0;
	aw_nand_info.SectorNumsPerPage = LogicArchiPar.SectCntPerLogicPage;
	aw_nand_info.BytesUserData = 16;
	aw_nand_info.BlkPerChip = LogicArchiPar.LogicBlkCntPerLogicDie;
	aw_nand_info.ChipNum = LogicArchiPar.LogicDieCnt;
	aw_nand_info.PageNumsPerBlk = LogicArchiPar.PageCntPerLogicBlk;
	aw_nand_info.FullBitmap = FULL_BITMAP_OF_SUPER_PAGE;

#if 0
	if (SUPPORT_USE_MAX_BLK_ERASE_CNT)
		aw_nand_info.MaxBlkEraseTimes = MaxBlkEraseTimes;
	else
		aw_nand_info.MaxBlkEraseTimes = 5000;
#else
	aw_nand_info.MaxBlkEraseTimes = NandStorageInfo.MaxEraseTimes;
#endif
	// if (SUPPORT_READ_RECLAIM)
	if (1)
		aw_nand_info.EnableReadReclaim = 1;
	else
		aw_nand_info.EnableReadReclaim = 0;

	aw_nand_info.boot = phyinfo_buf;

	set_uboot_start_and_end_block();

	nand_secure_storage_init(1);

ERR_RET:

	if (ret < 0)
		FORMAT_ERR("%s: End Nand Hardware initializing ..... FAIL!\n",
			   __func__);
	else
		FORMAT_DBG("%s: End Nand Hardware initializing ..... OK!\n",
			   __func__);

	return (ret < 0) ? (struct _nand_info *)NULL : &aw_nand_info;
	// return ;
}

__s32 SpiNandHwExit(void)
{
	FMT_Exit();
	PHY_Exit();

	return 0;
}

__s32 SpiNandHwNormalStandby(void)
{
	NAND_PhysicLock();
	return 0;
}

__s32 SpiNandHwNormalResume(void)
{
	NAND_PhysicUnLock();
	return 0;
}

__s32 SpiNandHwSuperStandby(void)
{
	__u32 i, j;
	__s32 ret = 0;
	__u32 bank = 0;

	NAND_PhysicLock();

	// sync all chip
	// printk("nand ch %d, chipconnectinfo: 0x%x\n ", j,
	// NAND_GetChipConnect());
	for (i = 0; i < 4; i++) {
		if (NAND_GetChipConnect() & (0x1 << i)) { // chip valid
			// nand_dbg_err("nand reset ch %d, chip %d!\n",j, i);
			ret = PHY_Wait_Status(bank);
			bank++;
			if (ret)
				FORMAT_ERR(
				    "nand reset ch %d, chip %d failed!\n", j,
				    i);
		} else {
			// nand_dbg_err("nand skip ch %d, chip %d!\n",j, i);
		}
	}

	for (i = 0; i < (SPIC_REG_LENGTH); i++) {
		if (i == (SPIC_REG_LENGTH - 1)) {
			spic_reg_back[i] =
			    *(volatile __u32 *)(NAND_GetIOBaseAddr(0) + 0x88);
			// nand_dbg_err("nand ch %d, reg 0x%x, value: 0x%x\n",
			// j, NAND_GetIOBaseAddrCH0() + i*0x04, *(volatile u32
			// *)(NAND_GetIOBaseAddrCH0() + i*0x04));
		} else {
			spic_reg_back[i] = *(
			    volatile __u32 *)(NAND_GetIOBaseAddr(0) + i * 0x04);
			// nand_dbg_err("nand ch %d, reg 0x%x, value: 0x%x\n",
			// j, NAND_GetIOBaseAddrCH1() + i*0x04, *(volatile u32
			// *)(NAND_GetIOBaseAddrCH0() + i*0x04));
		}
	}

	NAND_ClkRelease(0);
	NAND_PIORelease(0);

	NAND_ReleaseVoltage();

	return 0;
}

__s32 SpiNandHwSuperResume(void)
{
	__u32 bank = 0;
	__u32 i, j;
	__s32 ret = 0;

	NAND_GetVoltage();

	NAND_PIORequest(0);
	NAND_ClkRequest(0);

	// process for super standby
	// restore reg state
	for (i = 0; i < (SPIC_REG_LENGTH); i++) {
		if ((0x0 == i) || (0x7 == i)) {
			continue;
		} else if ((SPIC_REG_LENGTH - 1) == i) {
			*(volatile __u32 *)(NAND_GetIOBaseAddr(0) + 0x88) =
			    spic_reg_back[i];
		} else if (2 == i) {
			*(volatile __u32 *)(NAND_GetIOBaseAddr(0) + i * 0x04) =
			    spic_reg_back[i] & (~(0x1 << 31));
		} else {
			*(volatile __u32 *)(NAND_GetIOBaseAddr(0) + i * 0x04) =
			    spic_reg_back[i];
		}
	}

	// reset all chip
	// nand_dbg_err("nand ch %d, chipconnectinfo: 0x%x\n ", j,
	// NAND_GetChipConnect());
	for (i = 0; i < 4; i++) {
		if (NAND_GetChipConnect() & (0x1 << i)) {// chip valid
			// nand_dbg_err("nand reset ch %d, chip %d!\n",j, i);
			ret = PHY_ResetChip(i);
			ret |= PHY_Wait_Status(bank);
			bank++;
			if (ret)
				FORMAT_ERR(
				    "nand reset ch %d, chip %d failed!\n", j,
				    i);
		} else {
			// nand_dbg_err("nand skip ch %d, chip %d!\n",j, i);
		}
	}

	PHY_ChangeMode();

	NAND_PhysicUnLock();
	return 0;
}

__s32 SpiNandHwShutDown(void)
{
	__u32 bank = 0;
	__u32 i, j;
	__s32 ret = 0;

	NAND_PhysicLock();

	// sync all chip
	// nand_dbg_err("nand ch %d, chipconnectinfo: 0x%x\n ", j,
	// NAND_GetChipConnect());
	for (i = 0; i < 4; i++) {
		if (NAND_GetChipConnect() & (0x1 << i)) {// chip valid
			// nand_dbg_err("nand reset ch %d, chip %d!\n",j, i);
			ret = PHY_Wait_Status(bank);
			bank++;
			if (ret)
				FORMAT_ERR(
				    "nand reset ch %d, chip %d failed!\n", j,
				    i);
		} else {
			// nand_dbg_err("nand skip ch %d, chip %d!\n",j, i);
		}
	}
	PHY_Exit();

	return 0;
}

__s32 NandHw_DbgInfo(__u32 type)
{
	// PHY_ERR("%s: nand driver physical layer version: %x, %x, %x, %x\n",
	// __func__,
	//	NAND_DRV_PHYSIC_LAYER_VERSION_0,
	//NAND_DRV_PHYSIC_LAYER_VERSION_1,
	//	NAND_DRV_PHYSIC_LAYER_DATE, NAND_DRV_PHYSIC_LAYER_TIME);

	if (type == 0) {
		/*
		 *NandStorageInfo
		 */
		PHY_ERR("\n\n");
		PHY_ERR(" ==============Nand Architecture "
			"Parameter==============\n");
		PHY_ERR("    Nand Chip ID:         0x%x 0x%x\n",
			(NandStorageInfo.NandChipId[0] << 0) |
			    (NandStorageInfo.NandChipId[1] << 8) |
			    (NandStorageInfo.NandChipId[2] << 16) |
			    (NandStorageInfo.NandChipId[3] << 24),
			(NandStorageInfo.NandChipId[4] << 0) |
			    (NandStorageInfo.NandChipId[5] << 8) |
			    (NandStorageInfo.NandChipId[6] << 16) |
			    (NandStorageInfo.NandChipId[7] << 24));
		PHY_ERR("    Nand Chip Count:      0x%x\n",
			NandStorageInfo.ChipCnt);
		PHY_ERR("    Nand Chip Connect:    0x%x\n",
			NandStorageInfo.ChipConnectInfo);
		PHY_ERR("    Nand Connect Mode:      0x%x\n",
			NandStorageInfo.ConnectMode);
		PHY_ERR("    Sector Count Of Page: 0x%x\n",
			NandStorageInfo.SectorCntPerPage);
		PHY_ERR("    Page Count Of Block:  0x%x\n",
			NandStorageInfo.PageCntPerPhyBlk);
		PHY_ERR("    Block Count Of Die:   0x%x\n",
			NandStorageInfo.BlkCntPerDie);
		PHY_ERR("    Plane Count Of Die:   0x%x\n",
			NandStorageInfo.PlaneCntPerDie);
		PHY_ERR("    Die Count Of Chip:    0x%x\n",
			NandStorageInfo.DieCntPerChip);
		PHY_ERR("    Bank Count Of Chip:   0x%x\n",
			NandStorageInfo.BankCntPerChip);
		PHY_ERR("    Optional Operation:   0x%x\n",
			NandStorageInfo.OperationOpt);
		PHY_ERR("    Access Frequency:     0x%x\n",
			NandStorageInfo.FrequencePar);
		PHY_ERR("    SPI Mode:             0x%x\n",
			NandStorageInfo.SpiMode);
		PHY_ERR("    MaxEraseTimes:      0x%x\n",
			NandStorageInfo.MaxEraseTimes);
		PHY_ERR(" ====================================================="
			"==\n\n");

		PHY_ERR(" ==============Support Optional "
			"Operaion================\n");
		if (SUPPORT_MULTI_READ)
			PHY_ERR("    SUPPORT_MULTI_READ\n");
		if (SUPPORT_MULTI_PROGRAM)
			PHY_ERR("    SUPPORT_MULTI_PROGRAM\n");
		if (SUPPORT_EXT_INTERLEAVE)
			PHY_ERR("    SUPPORT_EXT_INTERLEAVE\n");
		// if (SUPPORT_WRITE_CHECK_SYNC)
		// PHY_ERR("    SUPPORT_WRITE_CHECK_SYNC\n");
		if (SUPPORT_USE_MAX_BLK_ERASE_CNT)
			PHY_ERR("    SUPPORT_USE_MAX_BLK_ERASE_CNT\n");
		if (SUPPORT_READ_RECLAIM)
			PHY_ERR("    SUPPORT_READ_RECLAIM\n");
		PHY_ERR(" ====================================================="
			"==\n");

		PHY_ERR(
		    " ===========Logical Architecture Parameter===========\n");
		PHY_ERR("    Page Count of Logic Block:  0x%x\n",
			LogicArchiPar.PageCntPerLogicBlk);
		PHY_ERR("    Sector Count of Logic Page: 0x%x\n",
			LogicArchiPar.SectCntPerLogicPage);
		PHY_ERR("    Block Count of Die:         0x%x\n",
			SuperBlkCntOfDie);
		PHY_ERR("    Die Count:                  0x%x\n",
			LogicArchiPar.LogicDieCnt);
		PHY_ERR(
		    " ===================================================\n");

	} else {
		PHY_ERR("NandHw_DbgInfo, wrong info type, %d\n", type);
		return -1;
	}

	return 0;
}
