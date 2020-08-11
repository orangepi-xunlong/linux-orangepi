/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _DISP_VDPO_H
#define _DISP_VDPO_H

#include "disp_private.h"

#if defined(SUPPORT_VDPO)
s32 disp_init_vdpo(struct disp_bsp_init_para *para);


s32 disp_vdpo_set_config(struct disp_device *p_vdpo,
			 struct disp_vdpo_config *p_cfg);
#endif /*endif SUPPORT_VDPO */

#endif /*End of file*/
