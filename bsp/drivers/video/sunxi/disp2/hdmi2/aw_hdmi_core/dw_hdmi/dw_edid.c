/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/of_address.h>
#include "dw_i2cm.h"
#include "dw_access.h"
#include "dw_edid.h"

#define EDID_I2C_ADDR		    0x50
#define EDID_I2C_SEGMENT_ADDR	0x30
#define EDID_BLOCK_SIZE		    (128)

#define TAG_BASE_BLOCK         0x00
#define TAG_CEA_EXT            0x02
#define TAG_VTB_EXT            0x10
#define TAG_DI_EXT             0x40
#define TAG_LS_EXT             0x50
#define TAG_MI_EXT             0x60

const unsigned DTD_SIZE = 0x12;

u16 i2c_min_ss_scl_low_time  = I2C_MIN_SS_SCL_LOW_TIME;
u16 i2c_min_ss_scl_high_time = I2C_MIN_SS_SCL_HIGH_TIME;

static supported_dtd_t _dtd[] = {
/* pclk  I/P Ha   Hb                          Va   Vb */
	{60000, {  1, 0, 0, 0, 25200, 0,  640,  160, 0,  4,   16,  96, 0,  480, 45, 0, 3, 10,  2, 0} },
	{59940, {  1, 0, 0, 0, 25175, 0,  640,  160, 0,  4,   16,  96, 0,  480, 45, 0, 3, 10,  2, 0} },
	{60000, {  2, 0, 0, 0, 27000, 0,  720,  138, 0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0} },
	{59940, {  2, 0, 0, 0, 27000, 0,  720,  138, 0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0} },
	{60000, {  3, 0, 0, 0, 27000, 0,  720,  138, 0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0} },
	{59940, {  3, 0, 0, 0, 27000, 0,  720,  138, 0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0} },
	{60000, {  4, 0, 0, 0, 74250, 0, 1280,  370, 0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{59940, {  4, 0, 0, 0, 74176, 0, 1280,  370, 0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{60000, {  5, 0, 0, 0, 74250, 1, 1920,  280, 0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1} },
	{59940, {  5, 0, 0, 0, 74176, 1, 1920,  280, 0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1} },
	{60000, {  6, 0, 0, 1, 27000, 1, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{59940, {  6, 0, 0, 1, 27000, 1, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{60000, {  7, 0, 0, 1, 27000, 1, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{59940, {  7, 0, 0, 1, 27000, 1, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{60000, {  8, 0, 0, 1, 27000, 0, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{60054, {  8, 0, 0, 1, 27000, 0, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{59826, {  8, 0, 0, 1, 27000, 0, 1440,  276, 0,  4,   38, 124, 0,  240, 23, 0, 3,  5,  3, 0} },
	{60000, {  9, 0, 0, 1, 27000, 0, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{60054, {  9, 0, 0, 1, 27000, 0, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{59826, {  9, 0, 0, 1, 27000, 0, 1440,  276, 0, 16,   38, 124, 0,  240, 23, 0, 9,  5,  3, 0} },
	{60000, { 10, 0, 0, 0, 54054, 1, 2880,  552, 0,  4,   76, 248, 0,  240, 22, 0, 3,  4,  3, 0} },
	{59940, { 11, 0, 0, 0, 54000, 1, 2880,  552, 0, 16,   76, 248, 0,  240, 22, 0, 9,  4,  3, 0} },
	{60000, { 12, 0, 0, 0, 54054, 0, 2880,  552, 0,  4,   76, 248, 0,  240, 22, 0, 3,  4,  3, 0} },
	{60054, { 12, 0, 0, 0, 54000, 0, 2880,  552, 0,  4,   76, 248, 0,  240, 22, 0, 3,  4,  3, 0} },
	{59826, { 12, 0, 0, 0, 54000, 0, 2880,  552, 0,  4,   76, 248, 0,  240, 23, 0, 3,  5,  3, 0} },
	{60000, { 13, 0, 0, 0, 54054, 0, 2880,  552, 0, 16,   76, 248, 0,  240, 22, 0, 9,  4,  3, 0} },
	{60054, { 13, 0, 0, 0, 54000, 0, 2880,  552, 0, 16,   76, 248, 0,  240, 22, 0, 9,  4,  3, 0} },
	{59826, { 13, 0, 0, 0, 54000, 0, 2880,  552, 0, 16,   76, 248, 0,  240, 23, 0, 9,  5,  3, 0} },
	{60000, { 14, 0, 0, 0, 54054, 0, 1440,  276, 0,  4,   32, 124, 0,  480, 45, 0, 3,  9,  6, 0} },
	{59940, { 14, 0, 0, 0, 54000, 0, 1440,  276, 0,  4,   32, 124, 0,  480, 45, 0, 3,  9,  6, 0} },
	{60000, { 15, 0, 0, 0, 54054, 0, 1440,  276, 0, 16,   32, 124, 0,  480, 45, 0, 9,  9,  6, 0} },
	{59940, { 15, 0, 0, 0, 54000, 0, 1440,  276, 0, 16,   32, 124, 0,  480, 45, 0, 9,  9,  6, 0} },
	{60000, { 16, 0, 0, 0, 148500, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{59940, { 16, 0, 0, 0, 148352, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{50000, { 17, 0, 0, 0, 27000, 0,  720,  144, 0,  4,   12,  64, 0,  576, 49, 0, 3,  5,  5, 0} },
	{50000, { 18, 0, 0, 0, 27000, 0,  720,  144, 0, 16,   12,  64, 0,  576, 49, 0, 9,  5,  5, 0} },
	{50000, { 19, 0, 0, 0, 74250, 0, 1280,  700, 0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{50000, { 20, 0, 0, 0, 74250, 1, 1920,  720, 0, 16,  528,  44, 1,  540, 22, 0, 9,  2,  5, 1} },
	{50000, { 21, 0, 0, 1, 27000, 1, 1440,  288, 0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0} },
	{50000, { 22, 0, 0, 1, 27000, 1, 1440,  288, 0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0} },
	{50000, { 23, 0, 0, 1, 27000, 0, 1440,  288, 0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0} },
	{50000, { 23, 0, 0, 1, 27000, 0, 1440,  288, 0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0} },
	{49920, { 23, 0, 0, 1, 27000, 0, 1440,  288, 0,  4,   24, 126, 0,  288, 25, 0, 3,  3,  3, 0} },
	{50000, { 24, 0, 0, 1, 27000, 0, 1440,  288, 0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0} },
	{50000, { 24, 0, 0, 1, 27000, 0, 1440,  288, 0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0} },
	{49920, { 24, 0, 0, 1, 27000, 0, 1440,  288, 0, 16,   24, 126, 0,  288, 25, 0, 9,  3,  3, 0} },
	{50000, { 25, 0, 0, 0, 54000, 1, 2880,  576, 0,  4,   48, 252, 0,  288, 24, 0, 3,  2,  3, 0} },
	{50000, { 26, 0, 0, 0, 54000, 1, 2880,  576, 0, 16,   48, 252, 0,  288, 24, 0, 9,  2,  3, 0} },
	{50000, { 27, 0, 0, 0, 54000, 0, 2880,  576, 0,  4,   48, 252, 0,  288, 24, 0, 3,  2,  3, 0} },
	{49920, { 27, 0, 0, 0, 54000, 0, 2880,  576, 0,  4,   48, 252, 0,  288, 25, 0, 3,  3,  3, 0} },
	{50000, { 27, 0, 0, 0, 54000, 0, 2880,  576, 0,  4,   48, 252, 0,  288, 24, 0, 3,  2,  3, 0} },
	{50000, { 28, 0, 0, 0, 54000, 0, 2880,  576, 0, 16,   48, 252, 0,  288, 24, 0, 9,  2,  3, 0} },
	{49920, { 28, 0, 0, 0, 54000, 0, 2880,  576, 0, 16,   48, 252, 0,  288, 25, 0, 9,  3,  3, 0} },
	{50000, { 28, 0, 0, 0, 54000, 0, 2880,  576, 0, 16,   48, 252, 0,  288, 24, 0, 9,  2,  3, 0} },
	{50000, { 29, 0, 0, 0, 54000, 0, 1440,  288, 0,  4,   24, 128, 0,  576, 49, 0, 3,  5,  5, 0} },
	{50000, { 30, 0, 0, 0, 54000, 0, 1440,  288, 0, 16,   24, 128, 0,  576, 49, 0, 9,  5,  5, 0} },
	{50000, { 31, 0, 0, 0, 148500, 0, 1920,  720, 0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{24000, { 32, 0, 0, 0, 74250, 0, 1920,  830, 0, 16,  638,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{23976, { 32, 0, 0, 0, 74176, 0, 1920,  830, 0, 16,  638,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{25000, { 33, 0, 0, 0, 74250, 0, 1920,  720, 0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{30000, { 34, 0, 0, 0, 74250, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{29970, { 34, 0, 0, 0, 74176, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{60000, { 35, 0, 0, 0, 108108, 0, 2880,  552, 0,  4,   64, 248, 0,  480, 45, 0, 3,  9,  6, 0} },
	{59940, { 35, 0, 0, 0, 108000, 0, 2880,  552, 0,  4,   64, 248, 0,  480, 45, 0, 3,  9,  6, 0} },
	{60000, { 36, 0, 0, 0, 108108, 0, 2880,  552, 0, 16,   64, 248, 0,  480, 45, 0, 9,  9,  6, 0} },
	{59940, { 36, 0, 0, 0, 108100, 0, 2880,  552, 0, 16,   64, 248, 0,  480, 45, 0, 9,  9,  6, 0} },
	{50000, { 37, 0, 0, 0, 108000, 0, 2880,  576, 0,  4,   48, 256, 0,  576, 49, 0, 3,  5,  5, 0} },
	{50000, { 38, 0, 0, 0, 108000, 0, 2880,  576, 0, 16,   48, 256, 0,  576, 49, 0, 9,  5,  5, 0} },
	{50000, { 39, 0, 0, 0, 72000, 1, 1920,  384, 0, 16,   32, 168, 1,  540, 85, 0, 9, 23,  5, 0} },
	{100000, { 40, 0, 0, 0, 148500, 1, 1920,  720, 0, 16,  528,  44, 1,  540, 22, 0, 9,  2,  5, 1} },
	{100000, { 41, 0, 0, 0, 148500, 0, 1280,  700, 0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{100000, { 42, 0, 0, 0, 54000, 0,  720,  144, 0,  4,   12,  64, 0,  576, 49, 0, 3,  5,  5, 0} },
	{100000, { 43, 0, 0, 0, 54000, 0,  720,  144, 0, 16,   12,  64, 0,  576, 49, 0, 9,  5,  5, 0} },
	{100000, { 44, 0, 0, 1, 54000, 1, 1440,  288, 0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0} },
	{100000, { 45, 0, 0, 1, 54000, 1, 1440,  288, 0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0} },
	{120000, { 46, 0, 0, 0, 148500, 1, 1920,  280, 0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1} },
	{119880, { 46, 0, 0, 0, 148352, 1, 1920,  280, 0, 16,   88,  44, 1,  540, 22, 0, 9,  2,  5, 1} },
	{120000, { 47, 0, 0, 0, 148500, 0, 1280,  370, 0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{120000, { 48, 0, 0, 0, 54054, 0,  720,  138, 0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0} },
	{120000, { 49, 0, 0, 0, 54054, 0,  720,  138, 0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0} },
	{119880, { 49, 0, 0, 0, 54000, 0,  720,  138, 0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0} },
	{120000, { 50, 0, 0, 1, 54054, 1, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{119880, { 50, 0, 0, 1, 54000, 1, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{120000, { 51, 0, 0, 1, 54054, 1, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{119880, { 51, 0, 0, 1, 54000, 1, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{200000, { 52, 0, 0, 0, 108000, 0,  720,  144, 0,  4,   12,  64, 0,  576, 49, 0, 3,  5,  5, 0} },
	{200000, { 53, 0, 0, 0, 108000, 0,  720,  144, 0, 16,   12,  64, 0,  576, 49, 0, 9,  5,  5, 0} },
	{200000, { 54, 0, 0, 1, 108000, 1, 1440,  288, 0,  4,   24, 126, 0,  288, 24, 0, 3,  2,  3, 0} },
	{200000, { 55, 0, 0, 1, 108000, 1, 1440,  288, 0, 16,   24, 126, 0,  288, 24, 0, 9,  2,  3, 0} },
	{240000, { 56, 0, 0, 0, 108100, 0,  720,  138, 0,  4,   16,  62, 0,  480, 45, 0, 3,  9,  6, 0} },
	{240000, { 57, 0, 0, 0, 108100, 0,  720,  138, 0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0} },
	{239760, { 57, 0, 0, 0, 108000, 0,  720,  138, 0, 16,   16,  62, 0,  480, 45, 0, 9,  9,  6, 0} },
	{240000, { 58, 0, 0, 1, 108100, 1, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{239760, { 58, 0, 0, 1, 108000, 1, 1440,  276, 0,  4,   38, 124, 0,  240, 22, 0, 3,  4,  3, 0} },
	{240000, { 59, 0, 0, 1, 108100, 1, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{239760, { 59, 0, 0, 1, 108000, 1, 1440,  276, 0, 16,   38, 124, 0,  240, 22, 0, 9,  4,  3, 0} },
	{24000, { 60, 0, 0, 0, 59400, 0, 1280, 2020, 0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{23970, { 60, 0, 0, 0, 59341, 0, 1280, 2020, 0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{25000, { 61, 0, 0, 0, 74250, 0, 1280, 2680, 0, 16, 2420,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{30000, { 62, 0, 0, 0, 74250, 0, 1280, 2020, 0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{29970, { 62, 0, 0, 0, 74176, 0, 1280, 2020, 0, 16, 1760,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{120000, { 63, 0, 0, 0, 297000, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{119880, { 63, 0, 0, 0, 296703, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{100000, { 64, 0, 0, 0, 297000, 0, 1920,  720, 0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{50000, { 68, 0, 0, 0, 74250, 0, 1280,  700, 0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{60000, { 69, 0, 0, 0, 74250, 0, 1280,  370, 0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{50000, { 75, 0, 0, 0, 148500, 0, 1920,  720, 0, 16,  528,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{60000, { 76, 0, 0, 0, 148500, 0, 1920,  280, 0, 16,   88,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{24000, { 93, 0, 0, 0, 297000, 0, 3840, 1660, 0, 16, 1276,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{25000, { 94, 0, 0, 0, 297000, 0, 3840, 1440, 0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{30000, { 95, 0, 0, 0, 297000, 0, 3840,  560, 0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{50000, { 96, 0, 0, 0, 594000, 0, 3840, 1440, 0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{60000, { 97, 0, 0, 0, 594000, 0, 3840,  560, 0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{24000, { 98, 0, 0, 0, 297000, 0, 4096, 1404, 0, 16, 1020,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{25000, { 99, 0, 0, 0, 297000, 0, 4096, 1184, 0, 16, 968,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{30000, { 100, 0, 0, 0, 297000, 0, 4096, 304, 0, 16, 88,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{50000, { 101, 0, 0, 0, 594000, 0, 4096, 1184, 0, 16, 968,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{60000, { 102, 0, 0, 0, 594000, 0, 4096, 304, 0, 16, 88,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{30000, { 103, 0, 0, 0, 297000, 0, 3840, 1660, 0, 16, 1276,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{30000, { 104, 0, 0, 0, 297000, 0, 3840, 1440, 0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{30000, { 105, 0, 0, 0, 297000, 0, 3840,  560, 0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{50000, { 106, 0, 0, 0, 594000, 0, 3840, 1440, 0, 16, 1056,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },
	{60000, { 107, 0, 0, 0, 594000, 0, 3840,  560, 0, 16,  176,  88, 1, 2160, 90, 0, 9,  8, 10, 1} },

	/* For 3D */
	{23976, {(32 + 0x80), 0, 0, 0, 148500, 0, 1920,  830, 0, 16,  638,  44, 1, 1080, 45, 0, 9,  4,  5, 1} },
	{50000, {(19 + 0x80), 0, 0, 0, 148500, 0, 1280,  700, 0, 16,  440,  40, 1,  720, 30, 0, 9,  5,  5, 1} },
	{60000, {(4 + 0x80), 0, 0, 0, 148500, 0, 1280,  370, 0, 16,  110,  40, 1,  720, 30, 0, 9,  5,  5, 1} },

	/* for vesa format */
	{60000, {0x201, 0, 0, 0, 233500, 0, 2560, 928, 0, 16, 192, 272, 0, 1440, 53, 0, 9, 3, 5, 1} },
	{70000, {0x202, 0, 0, 0, 280000, 0, 1440, 110, 0, 9, 68, 16, 1, 2560, 16, 0, 16, 6, 6, 1} },
	{60000, {0x203, 0, 0, 0, 142250, 0, 1080, 132, 0, 9, 64, 4, 1, 1920, 36, 0, 16, 16, 4, 0} },
	{0.0, {  0, 0, 0, 0,      0, 0,    0,    0, 0,  0,    0,   0, 0,    0,  0, 0, 0,  0,  0, 0} },
};

/**
 * @short Get the DTD structure that contains the video parameters
 * @param[in] code VIC code to search for
 * @param[in] rate  1HZ * 1000
 * @return returns a pointer to the DTD structure or NULL if not supported.
 * If rate=0 then the first (default)
 * parameters are returned for the VIC code.
 */
dw_dtd_t *_dtd_data_serach(u32 vic_code, u32 rate)
{
	int i = 0;

	for (i = 0; _dtd[i].dtd.mCode != 0; i++) {
		if (_dtd[i].dtd.mCode == vic_code) {
			if (rate == 0)
				return &_dtd[i].dtd;

			if (rate == _dtd[i].refresh_rate)
				return &_dtd[i].dtd;
		}
	}
	return NULL;
}

int dw_dtd_fill(dw_hdmi_dev_t *dev, dw_dtd_t *dtd, u32 code, u32 rate)
{
	dw_dtd_t *p_dtd = NULL;

	p_dtd = _dtd_data_serach(code, rate);
	if (p_dtd == NULL) {
		hdmi_err("%s: dtd table unsupport vic(%d) with frame rate(%dHz)!\n", __func__, code, rate);
		return false;
	}

	p_dtd->mYcc420 = false;
	p_dtd->mLimitedToYcc420 = false;
	memcpy(dtd, p_dtd, sizeof(dw_dtd_t));

	return true;
}

/* refresh rate unit: 1HZ * 1000 */
u32 dw_dtd_get_rate(dw_dtd_t *dtd)
{
	int i = 0;

	for (i = 0; _dtd[i].dtd.mCode != 0; i++) {
		if (_dtd[i].dtd.mCode == dtd->mCode) {
			if ((dtd->mPixelClock == 0) ||
				(_dtd[i].dtd.mPixelClock == dtd->mPixelClock))
				return _dtd[i].refresh_rate;
		}
	}
	return 0;
}

int _edid_calculate_checksum(u8 *edid)
{
	int i, checksum = 0;

	for (i = 0; i < EDID_BLOCK_SIZE; i++)
		checksum += edid[i];

	return checksum % 256; /* CEA-861 Spec */
}

void _reset_data_block_monitor_range_limits(dw_hdmi_dev_t *dev,
		dw_edid_monitor_descriptior_data_t *mrl)
{
	mrl->mMinVerticalRate = 0;
	mrl->mMaxVerticalRate = 0;
	mrl->mMinHorizontalRate = 0;
	mrl->mMaxHorizontalRate = 0;
	mrl->mMaxPixelClock = 0;
	mrl->mValid = false;
}

void _reset_data_block_colorimetry(dw_hdmi_dev_t *dev,
					dw_edid_colorimetry_data_t *cdb)
{
	cdb->mByte3 = 0;
	cdb->mByte4 = 0;
	cdb->mValid = false;
}

void _reset_data_block_hdr_metadata(dw_hdmi_dev_t *dev,
		dw_edid_hdr_static_metadata_data_t *hdr_metadata)
{
	memset(hdr_metadata, 0, sizeof(dw_edid_hdr_static_metadata_data_t));
}

void _reset_data_block_hdmi_forum(dw_hdmi_dev_t *dev, dw_edid_hdmi_forum_vs_data_t *forumvsdb)
{
	forumvsdb->mValid = false;
	forumvsdb->mIeee_Oui = 0;
	forumvsdb->mVersion = 0;
	forumvsdb->mMaxTmdsCharRate = 0;
	forumvsdb->mSCDC_Present = false;
	forumvsdb->mRR_Capable = false;
	forumvsdb->mLTS_340Mcs_scramble = false;
	forumvsdb->mIndependentView = false;
	forumvsdb->mDualView = false;
	forumvsdb->m3D_OSD_Disparity = false;
	forumvsdb->mDC_30bit_420 = false;
	forumvsdb->mDC_36bit_420 = false;
	forumvsdb->mDC_48bit_420 = false;
}

void _reset_data_block_hdmi(dw_hdmi_dev_t *dev, dw_edid_hdmi_vs_data_t *vsdb)
{
	int i, j = 0;

	vsdb->mPhysicalAddress = 0;
	vsdb->mSupportsAi = false;
	vsdb->mDeepColor30 = false;
	vsdb->mDeepColor36 = false;
	vsdb->mDeepColor48 = false;
	vsdb->mDeepColorY444 = false;
	vsdb->mDviDual = false;
	vsdb->mMaxTmdsClk = 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	vsdb->mId = 0;
	vsdb->mContentTypeSupport = 0;
	vsdb->mHdmiVicCount = 0;
	for (i = 0; i < DW_EDID_MAC_HDMI_VIC; i++)
		vsdb->mHdmiVic[i] = 0;

	vsdb->m3dPresent = false;
	for (i = 0; i < DW_EDID_MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < DW_EDID_MAX_HDMI_3DSTRUCT; j++)
			vsdb->mVideo3dStruct[i][j] = 0;
	}
	for (i = 0; i < DW_EDID_MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < DW_EDID_MAX_HDMI_3DSTRUCT; j++)
			vsdb->mDetail3d[i][j] = ~0;
	}
	vsdb->mValid = false;
}

void _reset_data_block_video_capabilit(dw_hdmi_dev_t *dev,
					dw_edid_video_capabilit_data_t *vcdb)
{
	vcdb->mQuantizationRangeSelectable = false;
	vcdb->mPreferredTimingScanInfo = 0;
	vcdb->mItScanInfo = 0;
	vcdb->mCeScanInfo = 0;
	vcdb->mValid = false;
}

void _reset_data_block_short_video(dw_hdmi_dev_t *dev, dw_edid_block_sad_t *sad)
{
	sad->mFormat = 0;
	sad->mMaxChannels = 0;
	sad->mSampleRates = 0;
	sad->mByte3 = 0;
}

void _reset_data_block_short_audio(dw_hdmi_dev_t *dev, dw_edid_block_svd_t *svd)
{
	svd->mNative = false;
	svd->mCode = 0;
}

void _reset_data_block_speaker_alloction(dw_hdmi_dev_t *dev,
		dw_edid_speaker_allocation_data_t *sadb)
{
	sadb->mByte1 = 0;
	sadb->mValid = false;
}

/**
 * Parses the Detailed Timing Descriptor.
 * Encapsulating the parsing process
 * @param dtd pointer to dw_dtd_t strucutute for the information to be save in
 * @param data a pointer to the 18-byte structure to be parsed.
 * @return true if success
 */
int _parse_data_block_detailed_timing(dw_hdmi_dev_t *dev, dw_dtd_t *dtd, u8 data[18])
{
	dtd->mCode = -1;
	dtd->mPixelRepetitionInput = 0;
	dtd->mLimitedToYcc420 = 0;
	dtd->mYcc420 = 0;

	dtd->mPixelClock = 1000 * byte_to_word(data[1], data[0]);/* [10000Hz] */
	if (dtd->mPixelClock < 0x01) {	/* 0x0000 is defined as reserved */
		return false;
	}

	dtd->mHActive         = concat_bits(data[4], 4, 4, data[2], 0, 8);
	dtd->mHBlanking       = concat_bits(data[4], 0, 4, data[3], 0, 8);
	dtd->mHSyncOffset     = concat_bits(data[11], 6, 2, data[8], 0, 8);
	dtd->mHSyncPulseWidth = concat_bits(data[11], 4, 2, data[9], 0, 8);
	dtd->mHImageSize      = concat_bits(data[14], 4, 4, data[12], 0, 8);
	dtd->mHBorder         = data[15];

	dtd->mVActive         = concat_bits(data[7], 4, 4, data[5], 0, 8);
	dtd->mVBlanking       = concat_bits(data[7], 0, 4, data[6], 0, 8);
	dtd->mVSyncOffset     = concat_bits(data[11], 2, 2, data[10], 4, 4);
	dtd->mVSyncPulseWidth = concat_bits(data[11], 0, 2, data[10], 0, 4);
	dtd->mVImageSize      = concat_bits(data[14], 0, 4, data[13], 0, 8);
	dtd->mVBorder         = data[16];

	if (bit_field(data[17], 4, 1) != 1) {/* if not DIGITAL SYNC SIGNAL DEF */
		hdmi_err("%s: invalid dtd byte[17] bit4 parameters %d\n", __func__,
				bit_field(data[17], 4, 1));
		return false;
	}

	if (bit_field(data[17], 3, 1) != 1) {/* if not DIGITAL SEPATATE SYNC */
		hdmi_err("%s: invalid dtd byte[17] bit3 parameters %d\n", __func__,
				bit_field(data[17], 3, 1));
		return false;
	}

	/* no stereo viewing support in HDMI */
	dtd->mInterlaced    = bit_field(data[17], 7, 1) == 1;
	dtd->mVSyncPolarity = bit_field(data[17], 2, 1) == 1;
	dtd->mHSyncPolarity = bit_field(data[17], 1, 1) == 1;
	return true;
}

int _parse_data_block_short_audio(dw_hdmi_dev_t *dev,
		dw_edid_block_sad_t *sad, u8 *data)
{
	LOG_TRACE();
	_reset_data_block_short_video(dev, sad);
	if (data != 0) {
		sad->mFormat = bit_field(data[0], 3, 4);
		sad->mMaxChannels = bit_field(data[0], 0, 3) + 1;
		sad->mSampleRates = bit_field(data[1], 0, 7);
		sad->mByte3 = data[2];
		return true;
	}
	return false;
}

int _parse_data_block_short_video(dw_hdmi_dev_t *dev,
		dw_edid_block_svd_t *svd, u8 data)
{
	_reset_data_block_short_audio(dev, svd);
	svd->mNative = (bit_field(data, 7, 1) == 1) ? true : false;
	svd->mCode = bit_field(data, 0, 7);
	svd->mLimitedToYcc420 = 0;
	svd->mYcc420 = 0;
	return true;
}

/**
 * Parse an array of data to fill the dw_edid_hdmi_vs_data_t data strucutre
 * @param *vsdb pointer to the structure to be filled
 * @param *data pointer to the 8-bit data type array to be parsed
 * @return Success, or error code:
 * @return 1 - array pointer invalid
 * @return 2 - Invalid datablock tag
 * @return 3 - Invalid minimum length
 * @return 4 - HDMI IEEE registration identifier not valid
 * @return 5 - Invalid length - latencies are not valid
 * @return 6 - Invalid length - Interlaced latencies are not valid
 */
int _parse_data_block_hdmi(dw_hdmi_dev_t *dev,
		dw_edid_hdmi_vs_data_t *vsdb, u8 *data)
{
	u8 blockLength = 0;
	unsigned videoInfoStart = 0;
	unsigned hdmi3dStart = 0;
	unsigned hdmiVicLen = 0;
	unsigned hdmi3dLen = 0;
	unsigned spanned3d = 0;
	unsigned i = 0;
	unsigned j = 0;

	LOG_TRACE();
	_reset_data_block_hdmi(dev, vsdb);
	if (!data) {
		hdmi_err("%s: data pointer is NULL!\n", __func__);
		return false;
	}

	if (bit_field(data[0], 5, 3) != 0x3) {
		hdmi_err("%s: invalid datablock tag! %d\n", __func__, bit_field(data[0], 5, 3));
		return false;
	}
	blockLength = bit_field(data[0], 0, 5);

	if (blockLength < 5) {
		hdmi_err("%s: invalid datablock length! %d\n", __func__, blockLength);
		return false;
	}

	if (byte_to_dword(0x00, data[3], data[2], data[1]) != 0x000C03) {
		hdmi_err("%s: hdmi ieee registration identifier not valid %d\n", __func__,
				byte_to_dword(0x00, data[3], data[2], data[1]));
		return false;
	}

	_reset_data_block_hdmi(dev, vsdb);
	vsdb->mId = 0x000C03;
	vsdb->mPhysicalAddress = byte_to_word(data[4], data[5]);
	/* parse extension fields if they exist */
	if (blockLength > 5) {
		vsdb->mSupportsAi = bit_field(data[6], 7, 1) == 1;
		vsdb->mDeepColor48 = bit_field(data[6], 6, 1) == 1;
		vsdb->mDeepColor36 = bit_field(data[6], 5, 1) == 1;
		vsdb->mDeepColor30 = bit_field(data[6], 4, 1) == 1;
		vsdb->mDeepColorY444 = bit_field(data[6], 3, 1) == 1;
		vsdb->mDviDual = bit_field(data[6], 0, 1) == 1;
	} else {
		vsdb->mSupportsAi = false;
		vsdb->mDeepColor48 = false;
		vsdb->mDeepColor36 = false;
		vsdb->mDeepColor30 = false;
		vsdb->mDeepColorY444 = false;
		vsdb->mDviDual = false;
	}
	vsdb->mMaxTmdsClk = (blockLength > 6) ? data[7] : 0;
	vsdb->mVideoLatency = 0;
	vsdb->mAudioLatency = 0;
	vsdb->mInterlacedVideoLatency = 0;
	vsdb->mInterlacedAudioLatency = 0;
	if (blockLength > 7) {
		if (bit_field(data[8], 7, 1) == 1) {
			if (blockLength < 10) {
				hdmi_err("%s: Invalid length - latencies are not valid %d\n", __func__,
						blockLength);
				return false;
			}
			if (bit_field(data[8], 6, 1) == 1) {
				if (blockLength < 12) {
					hdmi_err("%s: Invalid length - Interlaced latencies are not valid %d\n",
							__func__, blockLength);
					return false;
				} else {
					vsdb->mVideoLatency = data[9];
					vsdb->mAudioLatency = data[10];
					vsdb->mInterlacedVideoLatency
								= data[11];
					vsdb->mInterlacedAudioLatency
								= data[12];
					videoInfoStart = 13;
				}
			} else {
				vsdb->mVideoLatency = data[9];
				vsdb->mAudioLatency = data[10];
				vsdb->mInterlacedVideoLatency = 0;
				vsdb->mInterlacedAudioLatency = 0;
				videoInfoStart = 11;
			}
		} else {	/* no latency data */
			vsdb->mVideoLatency = 0;
			vsdb->mAudioLatency = 0;
			vsdb->mInterlacedVideoLatency = 0;
			vsdb->mInterlacedAudioLatency = 0;
			videoInfoStart = 9;
		}
		vsdb->mContentTypeSupport = bit_field(data[8], 0, 4);
	}
	/* additional video format capabilities are described */
	if (bit_field(data[8], 5, 1) == 1) {
		vsdb->mImageSize = bit_field(data[videoInfoStart], 3, 2);
		hdmiVicLen = bit_field(data[videoInfoStart + 1], 5, 3);
		hdmi3dLen = bit_field(data[videoInfoStart + 1], 0, 5);
		for (i = 0; i < hdmiVicLen; i++)
			vsdb->mHdmiVic[i] = data[videoInfoStart + 2 + i];

		vsdb->mHdmiVicCount = hdmiVicLen;
		if (bit_field(data[videoInfoStart], 7, 1) == 1) {/* 3d present */
			vsdb->m3dPresent = true;
			hdmi3dStart = videoInfoStart + hdmiVicLen + 2;
			/* 3d multi 00 -> both 3d_structure_all
			and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (bit_field(data[videoInfoStart], 5, 2) == 1) {
				/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++) {
					/* j spans 3d structures */
					if (bit_field(data[hdmi3dStart
						+ (j / 8)], (j % 8), 1) == 1) {
						for (i = 0; i < 16; i++)
							vsdb->mVideo3dStruct[i][(j < 8)	? j+8 : j - 8] = 1;
					}
				}
				spanned3d = 2;
				/* hdmi3dStart += 2;
				   hdmi3dLen -= 2; */
			} else if (bit_field(data[videoInfoStart], 5, 2) == 2) {
				/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {
						if (bit_field(data[hdmi3dStart + 2 + (i / 8)], (i % 8), 1) == 1)
							vsdb->mVideo3dStruct[(i < 8) ? i + 8 : i - 8][(j < 8) ? j + 8 : j - 8] = bit_field(data[hdmi3dStart + (j / 8)], (j % 8), 1);
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dLen > spanned3d) {
				hdmi3dStart += spanned3d;
				for (i = 0, j = 0; i < (hdmi3dLen - spanned3d); i++) {
					vsdb->mVideo3dStruct[bit_field(data[hdmi3dStart + i + j], 4, 4)][bit_field(data[hdmi3dStart + i + j], 0, 4)] = 1;
					if (bit_field(data[hdmi3dStart + i + j], 4, 4) > 7) {
						j++;
						vsdb->mDetail3d[bit_field(data[hdmi3dStart + i + j], 4, 4)][bit_field(data[hdmi3dStart + i + j], 4, 4)] = bit_field(data[hdmi3dStart + i + j], 4, 4);
					}
				}
			}
		} else {	/* 3d NOT present */
			vsdb->m3dPresent = false;
		}
	}
	vsdb->mValid = true;
	return true;
}

int _parse_data_block_hdmi_forum(dw_hdmi_dev_t *dev, dw_edid_hdmi_forum_vs_data_t *forumvsdb,
								u8 *data)
{
	u16 blockLength;

	LOG_TRACE();
	_reset_data_block_hdmi_forum(dev, forumvsdb);
	if (!data) {
		hdmi_err("%s: data pointer is NULL!\n", __func__);
		return false;
	}

	if (bit_field(data[0], 5, 3) != 0x3) {
		hdmi_err("%s: Invalid datablock tag %d\n", __func__, bit_field(data[0], 5, 3));
		return false;
	}
	blockLength = bit_field(data[0], 0, 5);
	if (blockLength < 7) {
		hdmi_err("%s: Invalid minimum length %d\n", __func__, blockLength);
		return false;
	}
	if (byte_to_dword(0x00, data[3], data[2], data[1]) !=
	    0xC45DD8) {
		hdmi_err("%s: HDMI IEEE registration identifier not valid %d\n", __func__,
				byte_to_dword(0x00, data[3], data[2], data[1]));
		return false;
	}
	forumvsdb->mVersion = bit_field(data[4], 0, 7);
	forumvsdb->mMaxTmdsCharRate = bit_field(data[5], 0, 7);
	forumvsdb->mSCDC_Present = bit_field(data[6], 7, 1);
	forumvsdb->mRR_Capable = bit_field(data[6], 6, 1);
	forumvsdb->mLTS_340Mcs_scramble = bit_field(data[6], 3, 1);
	forumvsdb->mIndependentView = bit_field(data[6], 2, 1);
	forumvsdb->mDualView = bit_field(data[6], 1, 1);
	forumvsdb->m3D_OSD_Disparity = bit_field(data[6], 0, 1);
	forumvsdb->mDC_48bit_420 = bit_field(data[7], 2, 1);
	forumvsdb->mDC_36bit_420 = bit_field(data[7], 1, 1);
	forumvsdb->mDC_30bit_420 = bit_field(data[7], 0, 1);
	forumvsdb->mValid = true;

#if 1
	EDID_INF("version %d\n", bit_field(data[4], 0, 7));
	EDID_INF("Max_TMDS_Charater_rate %d\n",
		    bit_field(data[5], 0, 7));
	EDID_INF("SCDC_Present %d\n", bit_field(data[6], 7, 1));
	EDID_INF("RR_Capable %d\n", bit_field(data[6], 6, 1));
	EDID_INF("LTE_340Mcsc_scramble %d\n",
		    bit_field(data[6], 3, 1));
	EDID_INF("Independent_View %d\n", bit_field(data[6], 2, 1));
	EDID_INF("Dual_View %d\n", bit_field(data[6], 1, 1));
	EDID_INF("3D_OSD_Disparity %d\n", bit_field(data[6], 0, 1));
	EDID_INF("DC_48bit_420 %d\n", bit_field(data[7], 2, 1));
	EDID_INF("DC_36bit_420 %d\n", bit_field(data[7], 1, 1));
	EDID_INF("DC_30bit_420 %d\n", bit_field(data[7], 0, 1));
#endif
	return true;
}

int _parse_data_block_speaker_allocation(dw_hdmi_dev_t *dev,
				dw_edid_speaker_allocation_data_t *sadb, u8 *data)
{
	LOG_TRACE();
	_reset_data_block_speaker_alloction(dev, sadb);
	if ((data != 0) && (bit_field(data[0], 0, 5) == 0x03) &&
				(bit_field(data[0], 5, 3) == 0x04)) {
		sadb->mByte1 = data[1];
		sadb->mValid = true;
		return true;
	}
	return false;
}

int _parse_data_block_video_capability(dw_hdmi_dev_t *dev,
			dw_edid_video_capabilit_data_t *vcdb, u8 *data)
{
	LOG_TRACE();
	_reset_data_block_video_capabilit(dev, vcdb);
	/* check tag code and extended tag */
	if ((data != 0) && (bit_field(data[0], 5, 3) == 0x7) &&
		(bit_field(data[1], 0, 8) == 0x0) &&
			(bit_field(data[0], 0, 5) == 0x2)) {
		/* so far VCDB is 2 bytes long */
		vcdb->mCeScanInfo = bit_field(data[2], 0, 2);
		vcdb->mItScanInfo = bit_field(data[2], 2, 2);
		vcdb->mPreferredTimingScanInfo = bit_field(data[2], 4, 2);
		vcdb->mQuantizationRangeSelectable =
				(bit_field(data[2], 6, 1) == 1) ? true : false;
		vcdb->mValid = true;
		return true;
	}
	return false;
}

int _parse_data_block_colorimetry(dw_hdmi_dev_t *dev,
				dw_edid_colorimetry_data_t *cdb, u8 *data)
{
	LOG_TRACE();
	_reset_data_block_colorimetry(dev, cdb);
	if ((data != 0) && (bit_field(data[0], 0, 5) == 0x03) &&
		(bit_field(data[0], 5, 3) == 0x07)
			&& (bit_field(data[1], 0, 7) == 0x05)) {
		cdb->mByte3 = data[2];
		cdb->mByte4 = data[3];
		cdb->mValid = true;
		return true;
	}
	return false;
}

int _parse_data_block_hdr_static_metadata(dw_hdmi_dev_t *dev,
		dw_edid_hdr_static_metadata_data_t *hdr_metadata, u8 *data)
{
	LOG_TRACE();
	_reset_data_block_hdr_metadata(dev, hdr_metadata);
	if ((data != 0) && (bit_field(data[0], 0, 5) > 1)
		&& (bit_field(data[0], 5, 3) == 0x07)
		&& (data[1] == 0x06)) {
		hdr_metadata->et_n = bit_field(data[2], 0, 5);
		hdr_metadata->sm_n = data[3];

		if (bit_field(data[0], 0, 5) > 3)
			hdr_metadata->dc_max_lum_data = data[4];
		if (bit_field(data[0], 0, 5) > 4)
			hdr_metadata->dc_max_fa_lum_data = data[5];
		if (bit_field(data[0], 0, 5) > 5)
			hdr_metadata->dc_min_lum_data = data[6];

		return true;
	}
	return false;
}

static void _parse_data_block_ycc420_video(sink_edid_t *edidExt,
		u8 Ycc420All, u8 LimitedToYcc420All)
{
	u32 edid_cnt = 0;

	for (edid_cnt = 0; edid_cnt < edidExt->edid_mSvdIndex; edid_cnt++) {
		switch (edidExt->edid_mSvd[edid_cnt].mCode) {
		case 96:
		case 97:
		case 101:
		case 102:
		case 106:
		case 107:
			Ycc420All == 1 ?
				edidExt->edid_mSvd[edid_cnt].mYcc420 = Ycc420All : 0;
			LimitedToYcc420All == 1 ?
				edidExt->edid_mSvd[edid_cnt].mLimitedToYcc420 =
							LimitedToYcc420All : 0;
			break;
		}
	}
}

static int _parse_cta_data_block(dw_hdmi_dev_t *dev, u8 *data, sink_edid_t *edidExt)
{
	u8 c = 0;
	dw_edid_block_sad_t tmpSad;
	dw_edid_block_svd_t tmpSvd;
	u8 tmpYcc420All = 0;
	u8 tmpLimitedYcc420All = 0;
	u32 ieeeId = 0;
	u8 extendedTag = 0;
	int i = 0;
	int edid_cnt = 0;
	int svdNr = 0;
	int icnt = 0;
	u8 tag = bit_field(data[0], 5, 3);
	u8 length = bit_field(data[0], 0, 5);

	tmpSvd.mLimitedToYcc420 = 0;
	tmpSvd.mYcc420 = 0;

	switch (tag) {
	case 0x1:		/* Audio Data Block */
		EDID_INF("audio datablock parsing\n");
		for (c = 1; c < (length + 1); c += 3) {
			_parse_data_block_short_audio(dev, &tmpSad, data + c);
			if (edidExt->edid_mSadIndex < (sizeof(edidExt->edid_mSad) / sizeof(dw_edid_block_sad_t)))
				edidExt->edid_mSad[edidExt->edid_mSadIndex++] = tmpSad;
			else
				EDID_INF("buffer full - sad ignored\n");
		}
		break;
	case 0x2:		/* Video Data Block */
		EDID_INF("video datablock parsing\n");
		for (c = 1; c < (length + 1); c++) {
			_parse_data_block_short_video(dev, &tmpSvd, data[c]);
			if (edidExt->edid_mSvdIndex < (sizeof(edidExt->edid_mSvd) / sizeof(dw_edid_block_svd_t)))
				edidExt->edid_mSvd[edidExt->edid_mSvdIndex++] = tmpSvd;
			else
				EDID_INF("buffer full - SVD ignored\n");
		}
		break;
	case 0x3:		/* Vendor Specific Data Block HDMI or HF */
		EDID_INF("hdmi vsdb and hdmi forum vsdb parsing\n ");
		ieeeId = byte_to_dword(0x00, data[3], data[2], data[1]);
		if (ieeeId == 0x000C03) {	/* HDMI */
			if (_parse_data_block_hdmi(dev, &edidExt->edid_mHdmivsdb, data) != true) {
				hdmi_err("%s: hdmi vendor specific data dlock parse failed!!!\n", __func__);
				break;
			}
			EDID_INF("hdmi vsdb parsed success.\n");
		} else {
			if (ieeeId == 0xC45DD8) {	/* HDMI-F */
				EDID_INF("sink is hdmi2.0 because haves hf-vsdb\n");
				edidExt->edid_m20Sink = true;
				if (_parse_data_block_hdmi_forum(dev, &edidExt->edid_mHdmiForumvsdb, data) != true) {
					hdmi_err("%s: hdmi forum vendor specific data block parse failed!!!\n",
							__func__);
					break;
				}
			} else {
				EDID_INF("vendor specific data block not parsed ieeeId: 0x%x\n",
					ieeeId);
			}
		}
		EDID_INF("\n");
		break;
	case 0x4:		/* Speaker Allocation Data Block */
		EDID_INF("sad block parsing\n");
		if (_parse_data_block_speaker_allocation(dev, &edidExt->edid_mSpeakerAllocationDataBlock, data) != true)
			hdmi_err("%s: Speaker Allocation Data Block corrupt\n", __func__);
		break;
	case 0x7:{
		EDID_INF("cea extended field 0x07\n");
		extendedTag = data[1];
		switch (extendedTag) {
		case 0x00:	/* Video Capability Data Block */
			EDID_INF("Video Capability Data Block\n");
			if (_parse_data_block_video_capability(dev, &edidExt->edid_mVideoCapabilityDataBlock, data) != true)
				hdmi_err("%s: Video Capability Data Block corrupt\n", __func__);
			break;
		case 0x04:	/* HDMI Video Data Block */
			EDID_INF("HDMI Video Data Block\n");
			break;
		case 0x05:	/* Colorimetry Data Block */
			EDID_INF("Colorimetry Data Block\n");
			if (_parse_data_block_colorimetry(dev, &edidExt->edid_mColorimetryDataBlock, data) != true)
				hdmi_err("%s: Colorimetry Data Block corrupt\n", __func__);
			break;
		case 0x06:	/* HDR Static Metadata Data Block */
			EDID_INF("HDR Static Metadata Data Block\n");
			if (_parse_data_block_hdr_static_metadata(dev, &edidExt->edid_hdr_static_metadata_data_block, data) != true)
				hdmi_err("%s: HDR Static Metadata Data Block corrupt\n", __func__);
			break;
		case 0x12:	/* HDMI Audio Data Block */
			EDID_INF("HDMI Audio Data Block\n");
			break;
		case 0xe:
			/* * If it is a YCC420 VDB then VICs can ONLY be displayed in YCC 4:2:0 */
			EDID_INF("YCBCR 4:2:0 Video Data Block\n");
			/* * If Sink has YCC Datablocks it is HDMI 2.0 */
			edidExt->edid_m20Sink = true;
			edidExt->edid_mYcc420Support = true;
			tmpLimitedYcc420All = (bit_field(data[0], 0, 5) == 1 ? 1 : 0);
			_parse_data_block_ycc420_video(edidExt, tmpYcc420All, tmpLimitedYcc420All);
			for (i = 0; i < (bit_field(data[0], 0, 5) - 1); i++) {
				/* * Length includes the tag byte */
				tmpSvd.mCode = data[2 + i];
				tmpSvd.mNative = 0;
				tmpSvd.mLimitedToYcc420 = 1;
				for (edid_cnt = 0; edid_cnt < edidExt->edid_mSvdIndex; edid_cnt++) {
					if (edidExt->edid_mSvd[edid_cnt].mCode == tmpSvd.mCode) {
						edidExt->edid_mSvd[edid_cnt] =	tmpSvd;
						goto concluded;
					}
				}
				if (edidExt->edid_mSvdIndex <
					(sizeof(edidExt->edid_mSvd) /  sizeof(dw_edid_block_svd_t))) {
					edidExt->edid_mSvd[edidExt->edid_mSvdIndex] = tmpSvd;
					edidExt->edid_mSvdIndex++;
				} else {
					EDID_INF("buffer full - YCC 420 DTD ignored\n");
				}
concluded:;
			}
			break;
		case 0x0f:
			/* * If it is a YCC420 CDB then VIC can ALSO be displayed in YCC 4:2:0 */
			edidExt->edid_m20Sink = true;
			edidExt->edid_mYcc420Support = true;
			EDID_INF("YCBCR 4:2:0 Capability Map Data Block\n");
			/* If YCC420 CMDB is bigger than 1, then there is SVD info to parse */
			if (bit_field(data[0], 0, 5) > 1) {
				for (i = 0; i < (bit_field(data[0], 0, 5) - 1); i++) {
					for (icnt = 0; icnt < 8; icnt++) {
						/* * Lenght includes the tag byte */
						if ((bit_field(data[2 + i], icnt, 1) & 0x01)) {
							svdNr = 8 * i + icnt;
							tmpSvd.mCode = edidExt->edid_mSvd[svdNr].mCode;
							tmpSvd.mYcc420 = 1;
							edidExt->edid_mSvd[svdNr] = tmpSvd;
						}
					}
				}
				/* Otherwise, all SVDs present at the Video Data Block support YCC420 */
			} else {
				tmpYcc420All = (bit_field(data[0], 0, 5) == 1 ? 1 : 0);
				_parse_data_block_ycc420_video(edidExt, tmpYcc420All, tmpLimitedYcc420All);
			}
			break;
		default:
			EDID_INF("Extended Data Block not parsed %d\n",
					extendedTag);
			break;
		}
		break;
	}
	default:
		EDID_INF("Data Block not parsed %d\n", tag);
		break;
	}
	return length + 1;
}

static int _edid_parser_block_base(dw_hdmi_dev_t *dev,
		struct edid *edid, sink_edid_t *sink)
{
	int i;

	if (edid->header[0] != 0) {
		hdmi_err("%s: edid base block is invalid!!! %d\n", __func__, edid->header[0]);
		return -1;
	}

	EDID_INF("\n\nEDID Block0 detailed discriptor:\n");
	for (i = 0; i < 4; i++) {
		struct detailed_timing *detailed_timing = &(edid->detailed_timings[i]);

		if (detailed_timing->pixel_clock == 0) {
			struct detailed_non_pixel *npixel = &(detailed_timing->data.other_data);
			char *end = NULL;

			switch (npixel->type) {
			case EDID_DETAIL_MONITOR_NAME:
				memcpy(sink->edid_mMonitorName, npixel->data.str.str,
						sizeof(sink->edid_mMonitorName)-1);

				/* clear 0x0a and 0x20 */
				end = strchr(sink->edid_mMonitorName, 0x0a);
				if (end != NULL)
					memset(end, 0, strlen(end));

				EDID_INF("Monitor name: %s\n", sink->edid_mMonitorName);
				break;
			case EDID_DETAIL_MONITOR_RANGE:
				break;

			}
		} else { /* Detailed Timing Definition */
			struct detailed_pixel_timing *ptiming = &(detailed_timing->data.pixel_data);
			EDID_INF("\npixel_clock:%d\n", detailed_timing->pixel_clock * 10000);
			EDID_INF("hactive * vactive: %d * %d\n",
			(((ptiming->hactive_hblank_hi >> 4) & 0x0f) << 8) | ptiming->hactive_lo,
			(((ptiming->vactive_vblank_hi >> 4) & 0x0f) << 8) | ptiming->vactive_lo);
		}
	}
	return true;
}

static int _edid_parser_block_cta_861(dw_hdmi_dev_t *dev,
		u8 *buffer, sink_edid_t *edidExt)
{
	int i = 0;
	int c = 0;
	dw_dtd_t tmpDtd;
	u8 offset = buffer[2];

	if (buffer[1] < 0x03) {
		hdmi_err("%s: edid cta 861 block version unsupport! %d\n", __func__, buffer[1]);
		return -1;
	}

	/* CTA 861 header block parse */
	edidExt->edid_mYcc422Support     = bit_field(buffer[3],	4, 1) == 1;
	edidExt->edid_mYcc444Support     = bit_field(buffer[3],	5, 1) == 1;
	edidExt->edid_mBasicAudioSupport = bit_field(buffer[3], 6, 1) == 1;
	edidExt->edid_mUnderscanSupport  = bit_field(buffer[3], 7, 1) == 1;

	/* CTA 861 data block parse */
	if (offset != 4) {
		for (i = 4; i < offset; i += _parse_cta_data_block(dev, buffer + i, edidExt))
			;
	}

	memcpy(edidExt->detailed_timings, buffer + 84, sizeof(edidExt->detailed_timings));

	/* last is checksum */
	for (i = offset, c = 0; ((i + DTD_SIZE) < (EDID_BLOCK_SIZE - 1)) && c < 6; i += DTD_SIZE, c++) {
		if (_parse_data_block_detailed_timing(dev, &tmpDtd, buffer + i) == true) {
			if (edidExt->edid_mDtdIndex < ((sizeof(edidExt->edid_mDtd) / sizeof(dw_dtd_t)))) {
				edidExt->edid_mDtd[edidExt->edid_mDtdIndex] = tmpDtd;
				EDID_INF("edid_mDtd code %d\n", edidExt->edid_mDtd[edidExt->edid_mDtdIndex].mCode);
				EDID_INF("edid_mDtd limited to Ycc420? %d\n", edidExt->edid_mDtd[edidExt->edid_mDtdIndex].mLimitedToYcc420);
				EDID_INF("edid_mDtd supports Ycc420? %d\n", edidExt->edid_mDtd[edidExt->edid_mDtdIndex].mYcc420);
				edidExt->edid_mDtdIndex++;
			} else {
				EDID_INF("buffer full - DTD ignored\n");
			}
		}
	}
	return true;
}

void dw_edid_reset(dw_hdmi_dev_t *dev, sink_edid_t *edidExt)
{
	unsigned i = 0;

	edidExt->edid_m20Sink = false;

	for (i = 0; i < sizeof(edidExt->edid_mMonitorName); i++)
		edidExt->edid_mMonitorName[i] = 0;

	edidExt->edid_mBasicAudioSupport = false;
	edidExt->edid_mUnderscanSupport  = false;
	edidExt->edid_mYcc422Support = false;
	edidExt->edid_mYcc444Support = false;
	edidExt->edid_mYcc420Support = false;
	edidExt->edid_mDtdIndex = 0;
	edidExt->edid_mSadIndex = 0;
	edidExt->edid_mSvdIndex = 0;

	_reset_data_block_hdmi(dev, &edidExt->edid_mHdmivsdb);
	_reset_data_block_hdmi_forum(dev, &edidExt->edid_mHdmiForumvsdb);
	_reset_data_block_monitor_range_limits(dev, &edidExt->edid_mMonitorRangeLimits);
	_reset_data_block_video_capabilit(dev, &edidExt->edid_mVideoCapabilityDataBlock);
	_reset_data_block_colorimetry(dev, &edidExt->edid_mColorimetryDataBlock);
	_reset_data_block_hdr_metadata(dev, &edidExt->edid_hdr_static_metadata_data_block);
	_reset_data_block_speaker_alloction(dev, &edidExt->edid_mSpeakerAllocationDataBlock);
}

int dw_edid_parser(dw_hdmi_dev_t *dev, u8 *buffer, sink_edid_t *edidExt)
{
	int ret = false;

	switch (buffer[0]) {
	case TAG_BASE_BLOCK:
		ret = _edid_parser_block_base(dev, (struct edid *) buffer, edidExt);
		break;
	case TAG_CEA_EXT:
		ret = _edid_parser_block_cta_861(dev, buffer, edidExt);
		break;
	case TAG_VTB_EXT:
	case TAG_DI_EXT:
	case TAG_LS_EXT:
	case TAG_MI_EXT:
	default:
		EDID_INF("edid block 0x%02x not supported\n", buffer[0]);
	}

	return ret;
}

int dw_edid_read_base_block(dw_hdmi_dev_t *dev, struct edid *edid)
{
	int error = 0;
	const u8 header[] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00};

	ctrl_i2cm_ddc_fast_mode(dev, 0);
	dw_i2cm_ddc_clk_config(dev, I2C_SFR_CLK, i2c_min_ss_scl_low_time,
			i2c_min_ss_scl_high_time, I2C_MIN_FS_SCL_LOW_TIME,
			I2C_MIN_FS_SCL_HIGH_TIME);

	error = dw_i2cm_ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR,
						0, 0, 128, (u8 *)edid);
	if (error) {
		hdmi_err("%s: edid ddc read failed!\n", __func__);
		return error;
	}
	error = memcmp((u8 *) edid, (u8 *) header, sizeof(header));
	if (error) {
		hdmi_err("%s: edid header check failed!\n", __func__);
		return error;
	}

	error = _edid_calculate_checksum((u8 *) edid);
	if (error) {
		hdmi_err("%s: edid checksum failed!\n", __func__);
		return error;
	}
	return 0;
}

int dw_edid_read_extenal_block(dw_hdmi_dev_t *dev, int block, u8 *edid_ext)
{
	int error = 0;
	/* to incorporate extensions we have to include the following
	- see VESA E-DDC spec. P 11 */
	u8 start_pointer = block / 2;
	/* pointer to segments of 256 bytes */
	u8 start_address = ((block % 2) * 0x80);
	/* offset in segment; first block 0-127; second 128-255 */

	error = dw_i2cm_ddc_read(dev, EDID_I2C_ADDR, EDID_I2C_SEGMENT_ADDR,
			start_pointer, start_address, 128, edid_ext);
	if (error) {
		hdmi_err("%s: edid extension ddc read failed!\n", __func__);
		return error;
	}

	error = _edid_calculate_checksum(edid_ext);
	if (error) {
		hdmi_err("%s: edid extension checksum failed!\n", __func__);
		return error;
	}
	return 0;
}
