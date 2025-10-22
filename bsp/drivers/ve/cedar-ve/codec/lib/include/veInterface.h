/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : ve_interface.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2016/04/13
*   Comment :
*
*
*/


#ifndef VE_INTERFACE_H
#define VE_INTERFACE_H

#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
#include <linux/types.h>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define VE_ENCPP_REGISTRE_LEN (256)
#define VE_ENCVE0_REGISTRE_LEN (256)
#define VE_ENCVE1_REGISTRE_LEN (256)      /* only lbc 0x88 - 0xa4 */
// #define VE_VCU_ENCODER_CORE_INDEX (1) //* 0: core0, 1: core1

enum VE_INTERRUPT_RESULT_TYPE {
	VE_INT_RESULT_TYPE_TIMEOUT    = 0,
	VE_INT_RESULT_TYPE_NORMAL     = 1,
	VE_INT_RESULT_TYPE_CSI_RESET  = 2,
};

//typedef enum eVeLbcMode {
//	LBC_MODE_DISABLE  = 0,
//	LBC_MODE_1_5X     = 1,
//	LBC_MODE_2_0X     = 2,
//	LBC_MODE_2_5X     = 3,
//	LBC_MODE_NO_LOSSY = 4,
//	LBC_MODE_1_0X     = 5,
//} eVeLbcMode;

typedef enum VE_REGISTER_GROUP {
	REG_GROUP_VETOP             = 0,
	REG_GROUP_MPEG_DECODER      = 1,
	REG_GROUP_H264_DECODER      = 2,
	REG_GROUP_VC1_DECODER       = 3,
	REG_GROUP_RV_DECODER        = 4,
	REG_GROUP_H265_DECODER      = 5,
	REG_GROUP_JPEG_DECODER      = 6,
	REG_GROUP_AVS_DECODER       = 7,

	REG_GROUP_VCU_ENCODER       = 8,
	REG_GROUP_ENCPP_ENCODER     = 9,
	REG_GROUP_JPEG_ENCODER      = 10,
	REG_GROUP_H264_ENCODER      = 11,
	REG_GROUP_H265_ENCODER      = 12,
	REG_GROUP_H265_ENCODER_EXT  = 13,
	REG_GROUP_ENCVE0            = 14,
	REG_GROUP_ENCVE1            = 15,
} ve_register_group_e;


enum EVEOPSTYPE{
	VE_OPS_TYPE_NORMAL = 0,
	VE_OPS_TYPE_VP9    = 1,
};

enum DRAMTYPE {
	DDRTYPE_DDR1_16BITS = 0,
	DDRTYPE_DDR1_32BITS = 1,
	DDRTYPE_DDR2_16BITS = 2,
	DDRTYPE_DDR2_32BITS = 3,
	DDRTYPE_DDR3_16BITS = 4,
	DDRTYPE_DDR3_32BITS = 5,
	DDRTYPE_DDR3_64BITS = 6,
	DDRTYPE_DDR4_16BITS = 7,
	DDRTYPE_DDR4_32BITS = 8,
	DDRTYPE_DDR4_64BITS = 9,
	DDRTYPE_LPDDR4      = 10,

	DDRTYPE_MIN = DDRTYPE_DDR1_16BITS,
	DDRTYPE_MAX = DDRTYPE_LPDDR4,
};

enum RESET_VE_MODE {
	RESET_VE_NORMAL = 0,
	RESET_VE_SPECIAL = 1,  // for dtmb, we should reset ve not reset decode
};

#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
#include "UserKernelAdapter.h"
#else
enum VE_MODE {
	VE_MODE_NULL = -1,
	VE_MODE_ENCPP = 0,
	VE_MODE_ENC,
	VE_MODE_DE,
	VE_MODE_VCUENC,
	VE_MODE_CNT,
};

struct user_iommu_param {
	int            fd;
	unsigned int   iommu_addr;
};
typedef struct ve_channel_proc_info {
	unsigned char *base_info_data;
	unsigned int   base_info_size;
	unsigned char *advance_info_data;
	unsigned int   advance_info_size;
	unsigned int   channel_id;
} ve_channel_proc_info;
#endif

enum VE_WORK_MODE {
	VE_NORMAL_MODE = 0,
	VE_DEC_MODE = 1,
	VE_ENC_MODE = 2,
	VE_JPG_DEC_MODE = 3,
};

typedef struct VeConfig {
	int nDecoderFlag;
	int nEncoderFlag;
	int nFormat;
	int nWidth;
	int nEnableAfbcFlag;
	int nResetVeMode;
	unsigned int nVeFreq;
	int bJustRunIspFlag;
	unsigned int bEnableVcuFuncFlag;
} VeConfig;

/* the struct must be same with lichee/linux-xx/drivers/media/cedar-ve/cedar-ve.h*/

/* v853
typedef struct CsiOnlineRelatedInfo
{
	unsigned int       csi_frame_start_cnt;
	unsigned int       csi_frame_done_cnt;
	unsigned int       csi_cur_frame_addr;
	unsigned int       csi_pre_frame_addr;
	unsigned int       csi_line_start_cnt;
	unsigned int       csi_line_done_cnt;
}CsiOnlineRelatedInfo;
*/

/* v821 */
typedef struct CsiOnlineRelatedInfo {
	/* target */
	unsigned int sensor_id;
	unsigned int bk_id;

	/* info */
	unsigned int csi_frame_start_cnt;
	unsigned int csi_sensor_id;
	unsigned int csi_bk_done;
	unsigned int csi_bk_error;
	unsigned int csi_cur_frame_addr;
	unsigned int csi_last_frame_addr;
} CsiOnlineRelatedInfo;

typedef struct CsiOnlineFrameCnt {
	int frame_cnt0;
	int frame_cnt1;
    unsigned int csi_sensor0_cur_frame_addr;
    unsigned int csi_sensor1_cur_frame_addr;
} CsiOnlineFrameCnt;

typedef struct page_buf_info {
	/* total_size = header + data + ext */
	unsigned int header_size;
	unsigned int data_size;
	unsigned int ext_size;
	unsigned int phy_addr_0;
	unsigned int phy_addr_1;
	unsigned int buf_id;
} page_buf_info;

typedef struct VeOpsS {
	void *(*init)(VeConfig *);
	void (*release)(void *);
	int (*lock)(void *);
	int (*unlock)(void *);
	void (*reset)(void *);
	void (*resetForce)(void *);
	int  (*waitInterrupt)(void *);
	int  (*waitInterruptVcu)(void*,  unsigned int);

#if CEDARC_DEBUG
	void (*WriteValue)(void*, unsigned int);
	unsigned int (*ReadValue)(void*, unsigned int);
	void (*CleanValue)(void *);
#endif

	int          (*getChipId)(void *);
	uint64_t     (*getIcVeVersion)(void *);
	void *(*getGroupRegAddr)(void*, int);
	int          (*getDramType)(void *);
	unsigned int (*getPhyOffset)(void *);

	void (*setDramType)(void *);
	void (*setDdrMode)(void*, int);
	int  (*setSpeed)(void*, unsigned int);
	void (*setEnableAfbcFlag)(void*, int);
	void (*setAdjustDramSpeedFlag)(void*, int);

	void (*enableVe)(void *);
	void (*disableVe)(void *);

	void (*initEncoderPerformance)(void*, int);
	void (*unInitEncoderPerformance)(void*, int);

	int  (*getIommuAddr)(void *, struct user_iommu_param *);
	int  (*freeIommuAddr)(void *, struct user_iommu_param *);

	void (*flush_cache)(void*, void *, int);

	int  (*setProcInfo)(void*, struct ve_channel_proc_info *);
	int  (*stopProcInfo)(void*, unsigned char);

	int  (*getCsiOnlineInfo)(void *, CsiOnlineRelatedInfo *);
	int  (*getCsiBkCurInfo)(void *, CsiOnlineRelatedInfo *, CsiOnlineFrameCnt *);
	int  (*setLbcParameter)(void *, unsigned int, unsigned int, unsigned int);

	int  (*allocPageBuf)(void*, page_buf_info *);
	int  (*recPageBuf)(void*, page_buf_info *);
	int  (*freePageBuf)(void*, page_buf_info *);

	void (*setOnlineChannel)(void*, unsigned int);

	int (*setMode)(void *, enum VE_MODE);
} VeOpsS;

static inline void *CdcVeInit(VeOpsS *veops, VeConfig *pVeConfig)
{
	return veops->init(pVeConfig);
}

static inline void CdcVeRelease(VeOpsS *veops, void *p)
{
	veops->release(p);
}

static inline int CdcVeLock(VeOpsS *veops, void *p)
{
	return veops->lock(p);
}

static inline int CdcVeUnLock(VeOpsS *veops, void *p)
{
	return veops->unlock(p);
}

static inline void CdcVeReset(VeOpsS *veops, void *p)
{
	veops->reset(p);
}

static inline void CdcVeResetForce(VeOpsS *veops, void *p)
{
	veops->resetForce(p);
}

static inline int CdcVeWaitInterrupt(VeOpsS *veops, void *p)
{
	return veops->waitInterrupt(p);
}

static inline int CdcVeWaitInterruptVcu(VeOpsS *veops, void *p, unsigned int coreIndex)
{
	return veops->waitInterruptVcu(p, coreIndex);
}

#if CEDARC_DEBUG
static inline void CdcVeWriteValue(VeOpsS *veops, void *p, unsigned int value)
{
	return veops->WriteValue(p, value);
}
static inline unsigned int CdcVeReadValue(VeOpsS *veops, void *p, unsigned int value)
{
	return veops->ReadValue(p, value);
}
static inline void CdcClearnValue(VeOpsS *veops, void *p)
{
	return veops->CleanValue(p);
}
#endif

static inline int CdcVeGetChipId(VeOpsS *veops, void *p)
{
	return veops->getChipId(p);
}

static inline uint64_t CdcVeGetIcVeVersion(VeOpsS *veops, void *p)
{
	return veops->getIcVeVersion(p);
}

static inline void *CdcVeGetGroupRegAddr(VeOpsS *veops, void *p, int nGroupId)
{
	return veops->getGroupRegAddr(p, nGroupId);
}

static inline int CdcVeGetDramType(VeOpsS *veops, void *p)
{
	return veops->getDramType(p);
}

static inline unsigned int  CdcVeGetPhyOffset(VeOpsS *veops, void *p)
{
	return veops->getPhyOffset(p);
}

static inline void CdcVeSetDramType(VeOpsS *veops, void *p)
{
	veops->setDramType(p);
}

static inline void CdcVeSetDdrMode(VeOpsS *veops, void *p, int ddr_mode)
{
	veops->setDdrMode(p, ddr_mode);
}

static inline int CdcVeSetSpeed(VeOpsS *veops, void *p, int nSpeedMHz)
{
	return veops->setSpeed(p, nSpeedMHz);
}

static inline void CdcVeSetEnableAfbcFlag(VeOpsS *veops, void *p, int bEnableFlag)
{
	veops->setEnableAfbcFlag(p, bEnableFlag);
}

static inline void CdcVeSetAdjustDramSpeedFlag(VeOpsS *veops, void *p, int bEnableFlag)
{
	veops->setAdjustDramSpeedFlag(p, bEnableFlag);
}


static inline void CdcVeEnableVe(VeOpsS *veops, void *p)
{
	veops->enableVe(p);
}

static inline void CdcVeDisableVe(VeOpsS *veops, void *p)
{
	veops->disableVe(p);
}

static inline void CdcVeInitEncoderPerformance(VeOpsS *veops, void *p, int nMode)
{
	veops->initEncoderPerformance(p, nMode);
}

static inline void CdcVeUnInitEncoderPerformance(VeOpsS *veops, void *p, int nMode)
{
	veops->unInitEncoderPerformance(p, nMode);
}

static inline int CdcVeGetIommuAddr(VeOpsS *veops, void *p, struct user_iommu_param *iommu_buffer)
{
	return veops->getIommuAddr(p, iommu_buffer);
}

static inline int CdcVeFreeIommuAddr(VeOpsS *veops, void *p, struct user_iommu_param *iommu_buffer)
{
	return veops->freeIommuAddr(p, iommu_buffer);
}

static inline int CdcVeSetProcInfo(VeOpsS *veops, void *p, struct ve_channel_proc_info *ch_proc_info)
{
	return veops->setProcInfo(p, ch_proc_info);
}

static inline int CdcVeStopProcInfo(VeOpsS *veops, void *p, unsigned char cChannelNum)
{
	return veops->stopProcInfo(p, cChannelNum);
}

static inline int CdcGetCsiBkCurInfo(VeOpsS *veops, void *p, CsiOnlineRelatedInfo *pCsiOnlineInfo, CsiOnlineFrameCnt *pCsiOnlineFramecnt)
{
    return veops->getCsiBkCurInfo(p, pCsiOnlineInfo, pCsiOnlineFramecnt);
}

static inline int CdcGetCsiOnlineInfo(VeOpsS *veops, void *p, CsiOnlineRelatedInfo *pCsiOnlineInfo)
{
	return veops->getCsiOnlineInfo(p, pCsiOnlineInfo);
}

static inline int CdcSetLbcParameter(VeOpsS *veops, void *p, unsigned int nLbcMode, unsigned int nWidht, unsigned int bReduceBuf)
{
	return veops->setLbcParameter(p, nLbcMode, nWidht, bReduceBuf);
}

static inline unsigned int CdcVeAllocPageBuf(VeOpsS *veops, void *p, page_buf_info *page_buf)
{
	return veops->allocPageBuf(p, page_buf);
}

static inline unsigned int CdcVeRecPageBuf(VeOpsS *veops, void *p, page_buf_info *page_buf)
{
	return veops->recPageBuf(p, page_buf);
}

static inline unsigned int CdcVeFreePageBuf(VeOpsS *veops, void *p, page_buf_info *page_buf)
{
	return veops->freePageBuf(p, page_buf);
}

static inline void CdcVeSetOnlineChannel(VeOpsS *veops, void *p, unsigned int bIsOnlineChannel)
{
	return veops->setOnlineChannel(p, bIsOnlineChannel);
}

static inline void CdcVeFlushCache(VeOpsS *veops, void *p, void *startAddr, int size)
{
	veops->flush_cache(p, startAddr, size);
}

static inline int CdcVeMode(VeOpsS *veops, void *p, enum VE_MODE mode)
{
	return veops->setMode(p, mode);
}

#ifdef __cplusplus
}
#endif

#endif

