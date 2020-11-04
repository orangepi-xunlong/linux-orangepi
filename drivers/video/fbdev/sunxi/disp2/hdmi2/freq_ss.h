/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __FREQ_SS_H__
#define __FREQ_SS_H__

#define FREE_SDM

#define SRAM_TEST_REG0 0x03000048

#define CCMU_BASE 0x03001000
#define CCMU_SIZE 4096

#define CCMU_PLL_VIDEO_CONTROL 0x50
#define CCMU_PLL_VIDEO_BIAS 0x350

#define CCMU_PLL_VIDEO_PATTERN0_CONTROL 0x150
#define CCMU_PLL_VIDEO_PATTERN1_CONTROL 0x154

#define CCMU_TCON_TV0 0xb80
#define CCMU_HDMI 0xb00

#endif
