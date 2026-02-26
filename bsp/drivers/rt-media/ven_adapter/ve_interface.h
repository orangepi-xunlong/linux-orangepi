/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * =====================================================================================
 *
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 *       Filename:  ve_interface.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020-04-15
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  jilinglin, <jilinglin@allwinnertech.com>
 *        Company:
 *
 * =====================================================================================
 */

#ifndef VE_INTERFACE_H
#define VE_INTERFACE_H

//#include "cdc_log.h"

enum VE_REGISTER_GROUP {
	REG_GROUP_VETOP	= 0,
	REG_GROUP_MPEG_DECODER = 1,
	REG_GROUP_H264_DECODER = 2,
	REG_GROUP_VC1_DECODER  = 3,
	REG_GROUP_RV_DECODER   = 4,
	REG_GROUP_H265_DECODER = 5,
	REG_GROUP_JPEG_DECODER = 6,
	REG_GROUP_AVS_DECODER  = 7,
	REG_GROUP_VP9_DECODER  = 8,
	REG_GROUP_AVS2_DECODER = 9,
};

enum VE_TYPE {
	VE_TYPE_AW = 0,
	VE_TYPE_GOOGLE,
};

enum CODEC_TYPE {
	VIDEO_CODEC_H264,
	VIDEO_CODEC_VP8,
	VIDEO_CODEC_AVS,
	VIDEO_CODEC_WMV3,
	VIDEO_CODEC_RX,
	VIDEO_CODEC_H265,
	VIDEO_CODEC_MJPEG,
	VIDEO_CODEC_AVS2,
	VIDEO_CODEC_VP9,
};

enum DRAMTYPE {
	DDRTYPE_DDR1_16BITS = 0,
	DDRTYPE_DDR1_32BITS = 1,
	DDRTYPE_DDR2_16BITS = 2,
	DDRTYPE_DDR2_32BITS = 3,
	DDRTYPE_DDR3_16BITS = 4,
	DDRTYPE_DDR3_32BITS = 5,
	DDRTYPE_DDR3_64BITS = 6,

	DDRTYPE_MIN = DDRTYPE_DDR1_16BITS,
	DDRTYPE_MAX = DDRTYPE_DDR3_64BITS,
};

enum RESET_VE_MODE {
	RESET_VE_NORMAL  = 0,
	RESET_VE_SPECIAL = 1, // for dtmb, we should reset ve not reset decode
};

enum VE_WORK_MODE {
	VE_NORMAL_MODE  = 0,
	VE_DEC_MODE     = 1,
	VE_ENC_MODE     = 2,
	VE_JPG_DEC_MODE = 3,
};

struct ve_config {
	int dec_flag;
	int enc_flag;
	int codec_format;
	int just_isp_enable;
};

enum VE_INTERRUPT_RESULT_TYPE {
	VE_INT_RESULT_TYPE_TIMEOUT   = 0,
	VE_INT_RESULT_TYPE_NORMAL    = 1,
	VE_INT_RESULT_TYPE_CSI_RESET = 2,
};

typedef enum eVeLbcMode {
	LBC_MODE_DISABLE  = 0,
	LBC_MODE_1_5X     = 1,
	LBC_MODE_2_0X     = 2,
	LBC_MODE_2_5X     = 3,
	LBC_MODE_NO_LOSSY = 4,
} eVeLbcMode;

typedef struct CsiOnlineRelatedInfo {
	unsigned int csi_frame_start_cnt;
	unsigned int csi_frame_done_cnt;
	unsigned int csi_cur_frame_addr;
	unsigned int csi_pre_frame_addr;
	unsigned int csi_line_start_cnt;
	unsigned int csi_line_done_cnt;
} CsiOnlineRelatedInfo;

typedef struct ve_channel_proc_info {
	unsigned char *base_info_data;
	unsigned int   base_info_size;
	unsigned char *advance_info_data;
	unsigned int   advance_info_size;
	unsigned int   channel_id;
} ve_channel_proc_info;

typedef struct page_buf_info {
    /* total_size = header + data + ext */
    unsigned int header_size;
    unsigned int data_size;
    unsigned int ext_size;
    unsigned int phy_addr_0;
    unsigned int phy_addr_1;
    unsigned int buf_id;
} page_buf_info;

struct ve_interface {
	void (*reset)(struct ve_interface *);
	int (*wait_interrupt)(struct ve_interface *);

	void (*set_ddr_mode)(struct ve_interface *, int);
	int (*set_ve_freq)(struct ve_interface *, int);
	void *(*get_reg_group_addr)(struct ve_interface *, int);
	unsigned long long (*get_ic_version)(struct ve_interface *);
	unsigned int (*get_phy_offset)(struct ve_interface *);

	void (*enable_ve)(struct ve_interface *);
	void (*disable_ve)(struct ve_interface *);

	void (*lock)(struct ve_interface *);
	void (*unlock)(struct ve_interface *);

	void (*force_reset)(struct ve_interface *);
	int (*get_csi_online_info)(struct ve_interface *, CsiOnlineRelatedInfo *);
	int (*set_lbc_parameter)(struct ve_interface *, unsigned int, unsigned int);
	int (*set_proc_info)(void *, struct ve_channel_proc_info *);
	int (*stop_proc_info)(struct ve_interface *, unsigned char);

    int  (*allocPageBuf)(struct ve_interface *, struct page_buf_info *);
    int  (*recPageBuf)(struct ve_interface *, struct page_buf_info *);
    int  (*freePageBuf)(struct ve_interface *, struct page_buf_info *);
};

static inline void ve_reset(struct ve_interface *p)
{
	p->reset(p);
}
static inline void ve_force_reset(struct ve_interface *p)
{
	p->reset(p);
}
static inline int ve_get_csi_online_info(struct ve_interface *p, CsiOnlineRelatedInfo *csi_online_info)
{
	return p->get_csi_online_info(p, csi_online_info);
}
static inline void ve_set_lbc_parameter(struct ve_interface *p, unsigned int lbc_mode, unsigned int w)
{
	p->set_lbc_parameter(p, lbc_mode, w);
}

static inline int ve_set_proc_info(struct ve_interface *p, struct ve_channel_proc_info *ch_proc_info)
{
	return p->set_proc_info(p, ch_proc_info);
}

static inline int ve_stop_proc_info(struct ve_interface *p, unsigned char cChannelNum)
{
	return p->stop_proc_info(p, cChannelNum);
}

static inline void ve_lock(struct ve_interface *p)
{
	p->lock(p);
}
static inline void ve_unlock(struct ve_interface *p)
{
	p->unlock(p);
}

static inline int ve_wait_interrupt(struct ve_interface *p)
{
	return p->wait_interrupt(p);
}

static inline void *ve_get_group_reg_addr(struct ve_interface *p, int id)
{
	if (p && p->get_reg_group_addr)
		return p->get_reg_group_addr(p, id);
	printk("error, fatal error! p %px", p);
	return NULL;
}

static inline void ve_set_ddr_mode(struct ve_interface *p, int ddr_mode)
{
	p->set_ddr_mode(p, ddr_mode);
}

static inline unsigned long long ve_get_ic_version(struct ve_interface *p)
{
	return p->get_ic_version(p);
}

static inline unsigned int ve_get_phy_offset(struct ve_interface *p)
{
	return p->get_phy_offset(p);
}

static inline void ve_enable_ve(struct ve_interface *p)
{
	p->enable_ve(p);
}

static inline void ve_disable_ve(struct ve_interface *p)
{
	p->disable_ve(p);
}

static inline int ve_set_ve_freq(struct ve_interface *p, int freq)
{
	return p->set_ve_freq(p, freq);
}

static inline unsigned int CdcVeAllocPageBuf(struct ve_interface *p, struct page_buf_info *page_buf)
{
    return p->allocPageBuf(p, page_buf);
}

static inline unsigned int CdcVeRecPageBuf(struct ve_interface *p, struct page_buf_info *page_buf)
{
    return p->recPageBuf(p, page_buf);
}

static inline unsigned int CdcVeFreePageBuf(struct ve_interface *p, struct page_buf_info *page_buf)
{
    return p->freePageBuf(p, page_buf);
}

struct ve_interface *ve_create(int type, struct ve_config config);
void ve_destory(int type, struct ve_interface *p);

#endif
