/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_VEP_TAB__
#define __DE_VEP_TAB__

extern int r2r[2][16];
extern int r2y[14][16];
extern int y2yl2l[21][16];
extern int y2yf2l[21][16];
extern int y2yf2f[21][16];
extern int y2yl2f[21][16];
extern int y2rl2l[7][16];
extern int y2rl2f[7][16];
extern int y2rf2l[7][16];
extern int y2rf2f[7][16];
extern int bypass_csc[12];
extern unsigned int sin_cos[128];
extern int fcc_range_gain[6];
extern int fcc_hue_gain[6];
extern short fcc_hue_tab[512];
extern unsigned short ce_bypass_lut[256];
extern unsigned short ce_constant_lut[256];

#endif
