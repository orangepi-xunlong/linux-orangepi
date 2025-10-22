/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DISP_MANAGER_H__
#define __DISP_MANAGER_H__

#include "disp_private.h"

#define MAX_LAYERS 24

s32 disp_init_mgr(struct disp_bsp_init_para *para);

#if IS_ENABLED(CONFIG_AW_IOMMU) || IS_ENABLED(CONFIG_ARM_SMMU_V3)
#define DE_MASTOR_ID 0
extern void sunxi_enable_device_iommu(unsigned int mastor_id, bool flag);
#endif

extern s32 __disp_config_transfer2inner(
	struct disp_layer_config_inner *config_inner,
	struct disp_layer_config *config);
extern s32 __disp_config2_transfer2inner(
	struct disp_layer_config_inner *config_inner,
	struct disp_layer_config2 *config);

s32 disp_mgr_set_rtwb_layer(struct disp_manager *mgr,
			    struct disp_layer_config2 *config,
			    struct disp_capture_info2 *p_cptr_info,
			    unsigned int layer_num);
void disp_composer_proc(u32 sel);
int disp_composer_update_timeline(int disp, unsigned int cnt);

void amp_preview_init(void);
void amp_preview_start_ack(void);
void amp_preview_stop_ack(void);

#endif
