/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *  File name   :   de_enhance.h
 *
 *  Description :   display engine 2.0 enhance basic function declaration
 *
 *  History     :   2014/04/02  vito cheng  v0.1  Initial version
 *                  2014/04/29  vito cheng  v0.2  Add disp_enhance_config_data
 *                                                struct delcaration
 ******************************************************************************/

#ifndef _DE_ENHANCE_H_
#define _DE_ENHANCE_H_

#include "../../include.h"
#include "de_fce_type.h"
#include "de_peak_type.h"
#include "de_lti_type.h"
#include "de_dns_type.h"
#include "de_fcc_type.h"
#include "de_bls_type.h"
#include "de_csc_table.h"
#include "de_vsu.h"

/* 8K for enahnce reg mem */
#define DE_DNS_REG_MEM_SIZE (0x400)
#define DE_FCC_REG_MEM_SIZE (0x800)
#define DE_BLS_REG_MEM_SIZE (0x400)
#define DE_LTI_REG_MEM_SIZE (0x400)
#define DE_PEAK_REG_MEM_SIZE (0x400)
#define DE_FCE_REG_MEM_SIZE (0x800)
#define DE_ENHANCE_REG_MEM_SIZE \
	(DE_DNS_REG_MEM_SIZE + DE_FCC_REG_MEM_SIZE \
	+ DE_BLS_REG_MEM_SIZE + DE_LTI_REG_MEM_SIZE \
	+ DE_PEAK_REG_MEM_SIZE + DE_FCE_REG_MEM_SIZE)

#define MODE_NUM 4
#define PARA_NUM 6
#define FORMAT_NUM 2
#define ENHANCE_MODE_NUM 3

#define ENHANCE_MIN_OVL_WIDTH  32
#define ENHANCE_MIN_OVL_HEIGHT 32
#define ENHANCE_MIN_BLD_WIDTH 32
#define ENHANCE_MIN_BLD_HEIGHT 4

#define FCE_OFST  0x70000	/* FCE offset based on RTMX */
#define PEAK_OFST 0x70800	/* PEAKING offset based on RTMX */
#define LTI_OFST  0x71000	/* LTI offset based on RTMX */
#define BLS_OFST  0x71800	/* BLS offset based on RTMX */
#define FCC_OFST  0x72000	/* FCC offset based on RTMX */
#define DNS_OFST  0x80000	/* DNS offset based on RTMX */
#define PEAK2D_OFST  0x30	/* PEAK2D offset based on VSU */

/* vep bypass mask */
#define USER_BYPASS 0x00000001
#define USER_BYPASS_MASK 0xfffffffe
#define SIZE_BYPASS 0x00000002
#define SIZE_BYPASS_MASK 0xfffffffd
#define LAYER_BYPASS 0x00000004
#define LAYER_BYPASS_MASK 0xfffffffb

#define DE_CLIP(x, low, high) max(low, min(x, high))

struct disp_enhance_layer_info {
	/* layer framebuffer size width/height */
	struct disp_rectsz fb_size;
	/* layer framebuffer crop retangle, don't care w/h at all */
	struct disp_rect fb_crop;
	/* layer enable */
	u32 en;
	/* layer format */
	u32 format;
};

struct disp_enhance_chn_info {
	/* overlay size (scale in size) */
	/* CAUTION: overlay size can be the output size of */
	/* video overlay/ de-interlace overlay/ FBD overlay */
	struct disp_rectsz ovl_size;
	/* bld size (scale out size) */
	struct disp_rectsz bld_size;
	/* layer info */
	struct disp_enhance_layer_info layer_info[MAX_LAYER_NUM_PER_CHN];
};

/* parameters of vep module */
struct vep_para {
	enum disp_manager_dirty_flags flags;
	struct __fce_config_data fce_para;
	struct __lti_config_data lti_para;
	struct __peak_config_data peak_para;
	struct __bls_config_data bls_para;
	struct __fcc_config_data fcc_para;
	struct __dns_config_data dns_para;
	struct de_vsu_sharp_config peak2d_para;
	u32 demo_enable;
	u32 bypass;
	u32 dev_type;
	u32 fmt;
};

/* system configuration of vep */
struct vep_config_info {
	u32 device_num;
	u32 vep_num[DE_NUM];
	u32 dns_exist[DE_NUM][VI_CHN_NUM];
	u32 peak2d_exist[DE_NUM][VI_CHN_NUM];
};

/* algorithm variable */
struct vep_alg_var {
	u32 frame_cnt;
};

/* peak function declaration */
s32 de_peak_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_peak_exit(u32 disp, u32 chn);
s32 de_peak_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_peak_enable(u32 disp, u32 chn, u32 en);
s32 de_peak_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_peak_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_peak_init_para(u32 disp, u32 chn);
s32 de_peak_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct __peak_config_data *para, u32 bypass);

/* /LTI function declaration */
s32 de_lti_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_lti_exit(u32 disp, u32 chn);
s32 de_lti_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_lti_enable(u32 disp, u32 chn, u32 en);
s32 de_lti_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_lti_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_lti_init_para(u32 disp, u32 chn);
s32 de_lti_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct __lti_config_data *para, u32 bypass);

/* FCE function declaration */
s32 de_fce_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_fce_exit(u32 disp, u32 chn);
s32 de_fce_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_fce_enable(u32 disp, u32 chn, u32 en);
s32 de_fce_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_fce_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_fce_init_para(u32 disp, u32 chn);
s32 de_fce_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct __fce_config_data *para, u32 bypass);
s32 de_hist_tasklet(u32 disp, u32 chn,
	u32 frame_cnt);
s32 de_ce_tasklet(u32 disp, u32 chn,
		  u32 frame_cnt);
s32 de_fce_set_icsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info);

/* BLS function declaration */
s32 de_bls_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_bls_enable(u32 disp, u32 chn, u32 en);
s32 de_bls_exit(u32 disp, u32 chn);
s32 de_bls_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_bls_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_bls_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_bls_init_para(u32 disp, u32 chn);
s32 de_bls_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct __bls_config_data *para, u32 bypass);

/* FCC function declaration */
s32 de_fcc_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_fcc_exit(u32 disp, u32 chn);
s32 de_fcc_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_fcc_enable(u32 disp, u32 chn, u32 en);
s32 de_fcc_set_size(u32 disp, u32 chn, u32 width, u32 height);
s32 de_fcc_set_window(u32 disp, u32 chn, u32 win_en,
	struct de_rect_o window);
s32 de_fcc_init_para(u32 disp, u32 chn);
s32 de_fcc_info2para(u32 disp, u32 chn, u32 fmt,
	u32 dev_type, struct __fcc_config_data *para,
	u32 bypass);
s32 de_fcc_set_icsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info);
s32 de_fcc_set_ocsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info);


/* DNS function declaration */
s32 de_dns_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_dns_exit(u32 disp, u32 chn);
s32 de_dns_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_dns_enable(u32 disp, u32 chn, u32 en);
s32 de_dns_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_dns_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_dns_init_para(u32 disp, u32 chn);
s32 de_dns_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct __dns_config_data *para, u32 bypass);
s32 de_dns_tasklet(u32 disp, u32 chn,
	u32 frame_cnt);

/* ENHANCE function declaration */
s32 de_enhance_update_regs(u32 disp);
s32 de_enhance_init(struct disp_bsp_init_para *para);
s32 de_enhance_exit(void);
s32 de_enhance_get_reg_blocks(u32 disp,
	struct de_reg_block **blk, u32 *blk_num);
s32 de_enhance_layer_apply(u32 disp,
	struct disp_enhance_chn_info *info);
s32 de_enhance_apply(u32 disp,
	struct disp_enhance_config *config);
s32 de_enhance_sync(u32 disp);
s32 de_enhance_tasklet(u32 disp);

/* module public function declaration */
void de_set_bits(u32 *reg_addr, u32 bits_val,
	u32 shift, u32 width);

#endif /* #ifndef _DE_ENHANCE_H_ */
