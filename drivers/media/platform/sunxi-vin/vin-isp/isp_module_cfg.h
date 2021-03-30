/*
 ******************************************************************************
 *
 * isp_module_cfg.h
 *
 * Hawkview ISP - isp_module_cfg.h module
 *
 * Copyright (c) 2013 by Allwinnertech Co., Ltd.  http:
 *
 * Version		  Author         Date		    Description
 *
 *   1.0		Yang Feng   	2013/11/07	    First Version
 *
 ******************************************************************************
 */

#ifndef __ISP__MODULE__CFG__H
#define __ISP__MODULE__CFG__H

#include <linux/kernel.h>
#include "bsp_isp_comm.h"

/* For debug */
#define ISP_DGB

#ifdef ISP_DGB
#define ISP_DBG(lev, dbg_level, x, arg...) do \
	{ \
		if (lev <= dbg_level) \
			printk("[ISP_DEBUG]"x, ##arg); \
	} while (0)
#else
#define  ISP_DBG(lev, dbg_level, x, arg...)
#endif

#ifdef ISP_DGB_FL
#define FUNCTION_LOG do \
	{ \
		printk("%s, line: %d\n", __FUNCTION__, __LINE__); \
	} while (0)
#else
#define  FUNCTION_LOG
#endif

#define ISP_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* ISP module config */
/* TABLE */

#define ISP_LUT_TBL_SIZE	256
#define ISP_LENS_TBL_SIZE	256
#define ISP_GAMMA_TBL_SIZE	256
#define ISP_DRC_TBL_SIZE	256

#define MAX_PIC_RESOLUTION_NUM	10

/*
 *
 *  struct isp_module_config - .
 *
 */
struct isp_module_config {
	unsigned int isp_platform_id;
	unsigned int module_enable_flag;
	unsigned int isp_module_update_flags;

	unsigned int table_update;
	/* AFS config */

	void *lut_src0_table;
	void *lut_src1_table;
	void *gamma_table;
	void *lens_table;
	void *drc_table;
	void *linear_table;
	void *disc_table;
};

enum isp_features_flags {
	ISP_FEATURES_AFS = (1 << 0),
	ISP_FEATURES_SAP = (1 << 1),
	ISP_FEATURES_CONTRAST = (1 << 2),
	ISP_FEATURES_BDNF = (1 << 3),
	ISP_FEATURES_RGB_DRC = (1 << 4),
	ISP_FEATURES_LUT_DPC = (1 << 5),
	ISP_FEATURES_LSC = (1 << 6),

	ISP_FEATURES_GAMMA = (1 << 7),
	ISP_FEATURES_RGB2RGB = (1 << 8),
	ISP_FEATURES_AE = (1 << 9),
	ISP_FEATURES_AF = (1 << 10),
	ISP_FEATURES_AWB = (1 << 11),
	ISP_FEATURES_HIST = (1 << 12),
	ISP_FEATURES_BAYER_GAIN_OFFSET = (1 << 13),
	ISP_FEATURES_WB = (1 << 14),
	ISP_FEATURES_OTF_DPC = (1 << 15),
	ISP_FEATURES_TG = (1 << 16),
	ISP_FEATURES_YCbCr_DRC = (1 << 17),
	ISP_FEATURES_OBC = (1 << 18),
	ISP_FEATURES_CFA = (1 << 19),
	ISP_FEATURES_SPRITE = (1 << 20),
	ISP_FEATURES_YCBCR_GAIN_OFFSET = (1 << 21),
	ISP_FEATURES_OUTPUT_SPEED_CTRL = (1 << 22),
	ISP_FEATURES_3D_DENOISE = (1 << 23),
	ISP_FEATURES_CNR = (1 << 24),
	ISP_FEATURES_SATU = (1 << 25),
	ISP_FEATURES_LINEAR = (1 << 26),
	ISP_FEATURES_DISC = (1 << 27),
	ISP_FEATURES_MAX,

	/* all possible flags raised */
	ISP_FEATURES_All = (((ISP_FEATURES_MAX - 1) << 1) - 1),
};

void isp_module_platform_init(struct isp_module_config *module_cfg);
void isp_setup_module_hw(struct isp_module_config *module_cfg);

#endif
