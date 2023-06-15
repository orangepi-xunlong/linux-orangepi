/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __DE_HAL_H__
#define __DE_HAL_H__
#include "de_rtmx.h"
#include "de_scaler.h"
#include "de_csc.h"
#include "../disp_private.h"

extern int de_al_mgr_apply(unsigned int screen_id,
			   struct disp_manager_data *data);
extern int de_al_init(struct disp_bsp_init_para *para);
extern int de_al_lyr_apply(unsigned int screen_id,
			   struct disp_layer_config_data *data,
			   unsigned int layer_num);
extern int de_al_mgr_sync(unsigned int screen_id);
extern int de_al_mgr_update_regs(unsigned int screen_id);
extern int de_al_query_irq(unsigned int screen_id);
extern int de_al_enable_irq(unsigned int screen_id, unsigned en);
int de_update_device_fps(unsigned int sel, u32 fps);

extern int de_enhance_set_size(unsigned int screen_id,
			       struct disp_rectsz *size);

#endif
