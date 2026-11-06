/*
 * Copyright (C) Cvitek Co., Ltd. 2021-2022. All rights reserved.
 *
 * File Name: ive_reg.h
 * Description: cvitek ive driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef IVE_REG
#define IVE_REG
// include register
//#include "ive_ccl_reg.h"
//#include "ive_dma_reg.h"
//#include "ive_filterop_reg.h"
//#include "ive_gmm_reg.h"
//#include "ive_map_reg.h"
//#include "ive_match_bg_reg.h"
//#include "ive_sad_reg.h"
//#include "ive_top_reg.h"
//#include "ive_update_bg_reg.h"
// include regBuilder
#include "builder/reg_ive_ccl.h"
#include "builder/reg_ive_ccl.struct.h"
#include "builder/reg_ive_dma.h"
#include "builder/reg_ive_dma.struct.h"
#include "builder/reg_ive_filterop.h"
#include "builder/reg_ive_filterop.struct.h"
#include "builder/reg_ive_gmm.h"
#include "builder/reg_ive_gmm.struct.h"
#include "builder/reg_ive_map.h"
#include "builder/reg_ive_map.struct.h"
#include "builder/reg_ive_match_bg.h"
#include "builder/reg_ive_match_bg.struct.h"
#include "builder/reg_ive_sad.h"
#include "builder/reg_ive_sad.struct.h"
#include "builder/reg_ive_top.h"
#include "builder/reg_ive_top.struct.h"
#include "builder/reg_ive_update_bg.h"
#include "builder/reg_ive_update_bg.struct.h"
#include "builder/reg_ive_ncc.h"
#include "builder/reg_ive_ncc.struct.h"
#include "builder/reg_cmdq.h"
#include "builder/reg_cmdq.struct.h"

#include "builder/reg_isp_dma_ctl.h"
#include "builder/reg_isp_dma_ctl.struct.h"
#include "builder/reg_isp_rdma.h"
#include "builder/reg_isp_rdma.struct.h"
#include "builder/reg_isp_wdma.h"
#include "builder/reg_isp_wdma.struct.h"

#include "builder/reg_img_in.h"
#include "builder/reg_img_in.struct.h"
#include "builder/reg_ive_hist.h"
#include "builder/reg_ive_hist.struct.h"
#include "builder/reg_sc_odma.h"
#include "builder/reg_sc_odma.struct.h"
#include "builder/reg_ive_intg.h"
#include "builder/reg_ive_intg.struct.h"


#endif
