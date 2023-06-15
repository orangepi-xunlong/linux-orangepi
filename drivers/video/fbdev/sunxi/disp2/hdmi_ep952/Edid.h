/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef EDID_H
#define EDID_H

#define EDID_BLOCK_SIZE  128

//
extern unsigned char EDID_GetHDMICap(unsigned char *pTarget);
extern unsigned char EDID_GetPCMFreqCap(unsigned char *pTarget);
extern unsigned char EDID_GetPCMChannelCap(unsigned char *pTarget);
extern unsigned char EDID_GetDataBlockAddr(unsigned char *pTarget,
					   unsigned char Tag);

#endif // EDID_H
