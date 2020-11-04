/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_VEP_TAB_
#define _DE_VEP_TAB_

#include <linux/types.h>

extern s32 fcc_range_gain[6];
extern s32 fcc_hue_gain[6];
extern s16 fcc_hue_tab[512];
extern u16 ce_bypass_lut[256];
extern u16 ce_constant_lut[256];

#endif /* #ifndef _DE_VEP_TAB_ */
