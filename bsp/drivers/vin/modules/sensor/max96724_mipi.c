/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* A V4L2 driver for ROHM BU18RM84 serdes sensor.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  GuCheng <gucheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("GC");
MODULE_DESCRIPTION("A low-level driver for maxim serdes sensor");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

/*define module timing*/
#define MCLK              (25*1000*1000)

/*
 * The MAX96724 8bit i2c address
 */
#define I2C_ADDR 0x4E
#define SENSOR_NUM 0x2
#define SENSOR_NAME "max96724_mipi"
#define SENSOR_NAME_2 "max96724_mipi_2"
#define V4L2_IDENT_SENSOR 0xA4		// Device Identifier: MAX96724(0xA2), MAX96724F(0xA3), MAX96724R(0xA4)

#define MAX96724_ADDR 0x27      // Deserializer: 8bit addrress 0x4E, 7bit addrress 0x27
#define MAX96705_ADDR 0x40      // Serializer:   8bit addrress 0x80, 7bit addrress 0x40
#define MAX96705_ADDR_NEW_LINKA 0x41
#define MAX96705_ADDR_NEW_LINKB 0x42
#define MAX96705_ADDR_NEW_LINKC 0x43
#define MAX96705_ADDR_NEW_LINKD 0x44

#define DLY_5MS 5
#define DLY_50MS 50
#define DLY_100MS 100

typedef struct{
    unsigned short reg_addr;
    unsigned short data;
    unsigned short wr16;            // Deserializer: 16bit Register, Serializer: 8bit
    unsigned short device_addr;
    unsigned short delay_time;     // ms
} maxim_reglist;

/*
 * The default register settings
 */
static maxim_reglist sensor_default_regs[] = {

};

static maxim_reglist sensor_720p_30fps_4ch[] = {
    // disable MIPI output
    {0x040B, 0x00, 1, MAX96724_ADDR, 0},

    // disable link
    {0x0006, 0x00, 1, MAX96724_ADDR, 0},

    // Set 3Gbps link Rx rate for GMSL1
    {0x0010, 0x11, 1, MAX96724_ADDR, 0},
    {0x0011, 0x11, 1, MAX96724_ADDR, 0},

    // config linkA/B/C/D as high-HIM mode
    {0x0B06, 0xEF, 1, MAX96724_ADDR, 0},
    {0x0C06, 0xEF, 1, MAX96724_ADDR, 0},
    {0x0D06, 0xEF, 1, MAX96724_ADDR, 0},
    {0x0E06, 0xEF, 1, MAX96724_ADDR, 0},

    // enable linkA/B/C/D local-ack
    {0x0B0D, 0x81, 1, MAX96724_ADDR, 0},
    {0x0C0D, 0x81, 1, MAX96724_ADDR, 0},
    {0x0D0D, 0x81, 1, MAX96724_ADDR, 0},
    {0x0E0D, 0x81, 1, MAX96724_ADDR, 0},

    // One-shot link reset for all
    {0x0018, 0x0F, 1, MAX96724_ADDR, DLY_50MS},

    // Disable linkA/B/C/D HS DE processing
    {0x0B0F, 0x01, 1, MAX96724_ADDR, 0},
    {0x0C0F, 0x01, 1, MAX96724_ADDR, 0},
    {0x0D0F, 0x01, 1, MAX96724_ADDR, 0},
    {0x0E0F, 0x01, 1, MAX96724_ADDR, 0},

    // LinkA
    // config linkA as GMSL1 mode & Enable LinkA
    {0x0006, 0x01, 1, MAX96724_ADDR, DLY_5MS},
    // Disable serialization & Enable configuration link
    {0x04, 0x47, 0, MAX96705_ADDR, 0},
    // Enable HVEN, HIBW, DBL on Ser depends on applications
    {0x07, 0x84, 0, MAX96705_ADDR, 0},
    // GPIO_OUT setting
    {0x0F, 0xBF, 0, MAX96705_ADDR, DLY_50MS},
    // Change I2C address to 0x82 for this Link A serializer
    {0x00, 0x82, 0, MAX96705_ADDR, DLY_50MS},

    // LinkB
    // config linkB as GMSL1 mode & Enable LinkB
    {0x0006, 0x02, 1, MAX96724_ADDR, DLY_5MS},
    // Disable serialization & Enable configuration link
    {0x04, 0x47, 0, MAX96705_ADDR, 0},
    // Enable HVEN, HIBW, DBL on Ser depends on applications
    {0x07, 0x84, 0, MAX96705_ADDR, 0},
    // GPIO_OUT setting
    {0x0F, 0xBF, 0, MAX96705_ADDR, DLY_50MS},
    // Change I2C address to 0x84 for this Link B serializer
    {0x00, 0x84, 0, MAX96705_ADDR, DLY_50MS},

    // LinkC
    // config linkC as GMSL1 mode & Enable LinkC
    {0x0006, 0x04, 1, MAX96724_ADDR, DLY_5MS},
    // Disable serialization & Enable configuration link
    {0x04, 0x47, 0, MAX96705_ADDR, 0},
    // Serializer Enable double and HVEN
    {0x07, 0x84, 0, MAX96705_ADDR, 0},
    // GPIO_OUT setting
    {0x0F, 0xBF, 0, MAX96705_ADDR, DLY_50MS},
    // Change I2C address to 0x86 for this Link C serializer
    {0x00, 0x86, 0, MAX96705_ADDR, DLY_50MS},

    // LinkD
    // config linkD as GMSL1 mode & Enable LinkD
    {0x0006, 0x08, 1, MAX96724_ADDR, DLY_5MS},
    // Disable serialization & Enable configuration link
    {0x04, 0x47, 0, MAX96705_ADDR, 0},
    // Enable HVEN, HIBW, DBL on Ser depends on applications
    {0x07, 0x84, 0, MAX96705_ADDR, 0},
    // GPIO_OUT setting
    {0x0F, 0xBF, 0, MAX96705_ADDR, DLY_50MS},
    // Change I2C address to 0x88 for this Link D serializer
    {0x00, 0x88, 0, MAX96705_ADDR, DLY_50MS},

    // ***************** MAX96724 Setting ****************
    // Enable Pipe 0/1/2/3
    {0x00F4, 0x0F, 1, MAX96724_ADDR, 0},
    // YUYV422 software override for all pipes since connected GMSL1 is under parallel mode
    {0x040B, 0x40, 1, MAX96724_ADDR, 0},    // soft_bpp_0[4:0] & CSI_OUT_EN remains disabled
    {0x040C, 0x00, 1, MAX96724_ADDR, 0},    // soft_vc_1[3:0] & soft_vc_0[3:0]
    {0x040D, 0x00, 1, MAX96724_ADDR, 0},    // soft_vc_3[3:0] & soft_vc_2[3:0]
    {0x040E, 0x5E, 1, MAX96724_ADDR, 0},    // soft_dt_1_h[1:0] & soft_dt_0[5:0]
    {0x040F, 0x7E, 1, MAX96724_ADDR, 0},    // soft_dt_2_h[3:0] & soft_dt_1_l[3:0]
    {0x0410, 0x7A, 1, MAX96724_ADDR, 0},    // soft_dt_3[5:0] & soft_dt_2_l[1:0]
    {0x0411, 0x48, 1, MAX96724_ADDR, 0},    // soft_bpp_2_h[2:0] & soft_bpp_1[4:0]
    {0x0412, 0x20, 1, MAX96724_ADDR, 0},    // soft_bpp_3[4:0] & soft_bpp_2_l[1:0]

    // Enable the Pipe1/2/3/4 8-bit(10 bit) mux
    {0x041A, 0xF0, 1, MAX96724_ADDR, 0},
    // Force all MIPI clocks running & Set Des in 2x4 mode
    {0x08A0, 0x84, 1, MAX96724_ADDR, 0},
    // Set Lane Mapping for 4-lane port A
    {0x08A3, 0xE4, 1, MAX96724_ADDR, 0},
    {0x08A4, 0xE4, 1, MAX96724_ADDR, 0},
    // Set 4 lane D-PHY
    {0x090A, 0xC0, 1, MAX96724_ADDR, 0},
    {0x0944, 0xC0, 1, MAX96724_ADDR, 0},
    {0x0984, 0xC0, 1, MAX96724_ADDR, 0},
    {0x09C4, 0xC0, 1, MAX96724_ADDR, 0},
    // Turn on MIPI PHYs
    {0x08A2, 0xF0, 1, MAX96724_ADDR, 0},

    // Set Data rate to be 800Mbps/lane for port A(PHY0+PHY1) and enable software override
    {0x0415, 0xE8, 1, MAX96724_ADDR, 0},
    {0x0418, 0xE8, 1, MAX96724_ADDR, 0},
    // Enable 2WxH concatenation on controller 1 for port A 4-lane only
    // {0x0971, 0x93, 1, MAX96724_ADDR, 0},

    // Video Pipe to MIPI Controller Mapping
    {0x090B, 0x07, 1, MAX96724_ADDR, 0},    // Enable mapping for pipe 0, mappings enabled for data type, FS and FE
    {0x092D, 0x15, 1, MAX96724_ADDR, 0},    // Map_D-PHY_DST Sets PHY destination to PHY1 for mappings 0-2
    {0x090D, 0x1E, 1, MAX96724_ADDR, 0},    // Map_SRC_0 - YUV422 8bit, VC = 0
    {0x090E, 0x1E, 1, MAX96724_ADDR, 0},    // Map_DST_0 - YUV422 8bit, VC = 0
    {0x090F, 0x00, 1, MAX96724_ADDR, 0},    // Map_SRC_1 - FS, VC = 0
    {0x0910, 0x00, 1, MAX96724_ADDR, 0},    // Map_DST_1 - FS, VC = 0
    {0x0911, 0x01, 1, MAX96724_ADDR, 0},    // Map_SRC_2 - FE, VC = 0
    {0x0912, 0x01, 1, MAX96724_ADDR, 0},    // Map_DST_2 - FE, VC = 0

    {0x094B, 0x07, 1, MAX96724_ADDR, 0},    // Enable mapping for pipe 1, mappings enabled for data type, FS and FE
    {0x096D, 0x15, 1, MAX96724_ADDR, 0},    // Map_D-PHY_DST Sets PHY destination to PHY1 for mappings 0-2
    {0x094D, 0x1E, 1, MAX96724_ADDR, 0},    // Map_SRC_0 - YUV422 8bit, VC = 0
    {0x094E, 0x5E, 1, MAX96724_ADDR, 0},    // Map_DST_0 - YUV422 8bit, VC = 1
    {0x094F, 0x00, 1, MAX96724_ADDR, 0},    // Map_SRC_1 - FS, VC = 0
    {0x0950, 0x40, 1, MAX96724_ADDR, 0},    // Map_DST_1 - FS, VC = 1
    {0x0951, 0x01, 1, MAX96724_ADDR, 0},    // Map_SRC_2 - FE, VC = 0
    {0x0952, 0x41, 1, MAX96724_ADDR, 0},    // Map_DST_2 - FE, VC = 1

    {0x098B, 0x07, 1, MAX96724_ADDR, 0},    // Enable mapping for pipe 2, mappings enabled for data type, FS and FE
    {0x09AD, 0x15, 1, MAX96724_ADDR, 0},    // Map_D-PHY_DST Sets PHY destination to PHY1 for mappings 0-2
    {0x098D, 0x1E, 1, MAX96724_ADDR, 0},    // Map_SRC_0 - YUV422 8bit, VC = 0
    {0x098E, 0x9E, 1, MAX96724_ADDR, 0},    // Map_DST_0 - YUV422 8bit, VC = 2
    {0x098F, 0x00, 1, MAX96724_ADDR, 0},    // Map_SRC_1 - FS, VC = 0
    {0x0990, 0x80, 1, MAX96724_ADDR, 0},    // Map_DST_1 - FS, VC = 2
    {0x0991, 0x01, 1, MAX96724_ADDR, 0},    // Map_SRC_2 - FE, VC = 0
    {0x0992, 0x81, 1, MAX96724_ADDR, 0},    // Map_DST_2 - FE, VC = 2

    {0x09CB, 0x07, 1, MAX96724_ADDR, 0},    // Enable mapping for pipe 3, mappings enabled for data type, FS and FE
    {0x09ED, 0x15, 1, MAX96724_ADDR, 0},    // Map_D-PHY_DST Sets PHY destination to PHY1 for mappings 0-2
    {0x09CD, 0x1E, 1, MAX96724_ADDR, 0},    // Map_SRC_0 - YUV422 8bit, VC = 0
    {0x09CE, 0xDE, 1, MAX96724_ADDR, 0},    // Map_DST_0 - YUV422 8bit, VC = 3
    {0x09CF, 0x00, 1, MAX96724_ADDR, 0},    // Map_SRC_1 - FS, VC = 0
    {0x09D0, 0xC0, 1, MAX96724_ADDR, 0},    // Map_DST_1 - FS, VC = 3
    {0x09D1, 0x01, 1, MAX96724_ADDR, 0},    // Map_SRC_2 - FE, VC = 0
    {0x09D2, 0xC1, 1, MAX96724_ADDR, DLY_50MS},    // Map_DST_2 - FE, VC = 3

    // Deserializer Enable double and HVEN (LinkA/B/C/D)
    {0x0B07, 0x84, 1, MAX96724_ADDR, 0},
    {0x0C07, 0x84, 1, MAX96724_ADDR, 0},
    {0x0D07, 0x84, 1, MAX96724_ADDR, 0},
    {0x0E07, 0x84, 1, MAX96724_ADDR, DLY_50MS},

    // Enable All Links in GMSL1 mode
    {0x0006, 0x0F, 1, MAX96724_ADDR, DLY_5MS},

    // Enable Link A/B/C/D serialization & Disable configuration link
    {0x04, 0x87, 0, MAX96705_ADDR_NEW_LINKA, 0},
    {0x04, 0x87, 0, MAX96705_ADDR_NEW_LINKB, 0},
    {0x04, 0x87, 0, MAX96705_ADDR_NEW_LINKC, 0},
    {0x04, 0x87, 0, MAX96705_ADDR_NEW_LINKD, DLY_50MS},

    // Disable linkA/B/C/D local-ack
    {0x0B0D, 0x00, 1, MAX96724_ADDR, 0},
    {0x0C0D, 0x00, 1, MAX96724_ADDR, 0},
    {0x0D0D, 0x00, 1, MAX96724_ADDR, 0},
    {0x0E0D, 0x00, 1, MAX96724_ADDR, 0},

    // CSI_OUT_EN enabled
    {0x040B, 0x42, 1, MAX96724_ADDR, 0},
};

static maxim_reglist sensor_720p_30fps_2ch[] = {
	// disable MIPI output
    {0x040B, 0x00, 1, MAX96724_ADDR, 0},

    // disable link
    {0x0006, 0x00, 1, MAX96724_ADDR, 0},

    // Set 3Gbps link Rx rate for GMSL1
    {0x0010, 0x11, 1, MAX96724_ADDR, 0},

    // config linkA/B as high-HIM mode
    {0x0B06, 0xEF, 1, MAX96724_ADDR, 0},
    {0x0C06, 0xEF, 1, MAX96724_ADDR, 0},

    // enable linkA/B local-ack
    {0x0B0D, 0x81, 1, MAX96724_ADDR, 0},
    {0x0C0D, 0x81, 1, MAX96724_ADDR, 0},

    // One-shot link reset for all
    {0x0018, 0x0F, 1, MAX96724_ADDR, DLY_50MS},

    // Disable linkA/B HS DE processing
    {0x0B0F, 0x01, 1, MAX96724_ADDR, 0},
    {0x0C0F, 0x01, 1, MAX96724_ADDR, 0},

    // LinkA
    // config linkA as GMSL1 mode & Enable LinkA
    {0x0006, 0x01, 1, MAX96724_ADDR, DLY_5MS},
    // Disable serialization & Enable configuration link
    {0x04, 0x47, 0, MAX96705_ADDR, 0},
    // Enable HVEN, HIBW, DBL on Ser depends on applications
    {0x07, 0x84, 0, MAX96705_ADDR, 0},
    // GPIO_OUT setting
    {0x0F, 0xBF, 0, MAX96705_ADDR, DLY_50MS},
    // Change I2C address to 0x82 for this Link A serializer
    {0x00, 0x82, 0, MAX96705_ADDR, DLY_50MS},

    // LinkB
    // config linkB as GMSL1 mode & Enable LinkB
    {0x0006, 0x02, 1, MAX96724_ADDR, DLY_5MS},
    // Disable serialization & Enable configuration link
    {0x04, 0x47, 0, MAX96705_ADDR, 0},
    // Enable HVEN, HIBW, DBL on Ser depends on applications
    {0x07, 0x84, 0, MAX96705_ADDR, 0},
    // GPIO_OUT setting
    {0x0F, 0xBF, 0, MAX96705_ADDR, DLY_50MS},
    // Change I2C address to 0x84 for this Link B serializer
    {0x00, 0x84, 0, MAX96705_ADDR, DLY_50MS},

    // ***************** MAX96724 Setting ****************
    // Enable Pipe 0/1
    {0x00F4, 0x0F, 1, MAX96724_ADDR, 0},
    // YUYV422 software override for all pipes since connected GMSL1 is under parallel mode
    {0x040B, 0x40, 1, MAX96724_ADDR, 0},    // soft_bpp_0[4:0] & CSI_OUT_EN remains disabled
    {0x040C, 0x00, 1, MAX96724_ADDR, 0},    // soft_vc_1[3:0] & soft_vc_0[3:0]
    {0x040D, 0x00, 1, MAX96724_ADDR, 0},    // soft_vc_3[3:0] & soft_vc_2[3:0]
    {0x040E, 0x5E, 1, MAX96724_ADDR, 0},    // soft_dt_1_h[1:0] & soft_dt_0[5:0]
    {0x040F, 0x7E, 1, MAX96724_ADDR, 0},    // soft_dt_2_h[3:0] & soft_dt_1_l[3:0]
    {0x0410, 0x7A, 1, MAX96724_ADDR, 0},    // soft_dt_3[5:0] & soft_dt_2_l[1:0]
    {0x0411, 0x48, 1, MAX96724_ADDR, 0},    // soft_bpp_2_h[2:0] & soft_bpp_1[4:0]
    {0x0412, 0x20, 1, MAX96724_ADDR, 0},    // soft_bpp_3[4:0] & soft_bpp_2_l[1:0]

    // Enable the Pipe1/2 8-bit(10 bit) mux
    {0x041A, 0xF0, 1, MAX96724_ADDR, 0},
    // Force all MIPI clocks running & Set Des in 2x4 mode
    {0x08A0, 0x84, 1, MAX96724_ADDR, 0},
    // Set Lane Mapping for 4-lane port A
    {0x08A3, 0xE4, 1, MAX96724_ADDR, 0},
    {0x08A4, 0xE4, 1, MAX96724_ADDR, 0},
    // Set 4 lane D-PHY
    {0x090A, 0xC0, 1, MAX96724_ADDR, 0},
    {0x0944, 0xC0, 1, MAX96724_ADDR, 0},
    // Turn on MIPI PHYs
    {0x08A2, 0xF0, 1, MAX96724_ADDR, 0},

    // Set Data rate to be 800Mbps/lane for port A(PHY0+PHY1) and enable software override
    {0x0415, 0xE8, 1, MAX96724_ADDR, 0},
    {0x0418, 0xE8, 1, MAX96724_ADDR, 0},
    // Enable 2WxH concatenation on controller 1 for port A 4-lane only
    // {0x0971, 0x93, 1, MAX96724_ADDR, 0},

    // Video Pipe to MIPI Controller Mapping
    {0x090B, 0x07, 1, MAX96724_ADDR, 0},    	// Enable mapping for pipe 0, mappings enabled for data type, FS and FE
    {0x092D, 0x15, 1, MAX96724_ADDR, 0},    	// Map_D-PHY_DST Sets PHY destination to PHY1 for mappings 0-2
    {0x090D, 0x1E, 1, MAX96724_ADDR, 0},    	// Map_SRC_0 - YUV422 8bit, VC = 0
    {0x090E, 0x1E, 1, MAX96724_ADDR, 0},    	// Map_DST_0 - YUV422 8bit, VC = 0
    {0x090F, 0x00, 1, MAX96724_ADDR, 0},    	// Map_SRC_1 - FS, VC = 0
    {0x0910, 0x00, 1, MAX96724_ADDR, 0},    	// Map_DST_1 - FS, VC = 0
    {0x0911, 0x01, 1, MAX96724_ADDR, 0},    	// Map_SRC_2 - FE, VC = 0
    {0x0912, 0x01, 1, MAX96724_ADDR, 0},    	// Map_DST_2 - FE, VC = 0

    {0x094B, 0x07, 1, MAX96724_ADDR, 0},    	// Enable mapping for pipe 1, mappings enabled for data type, FS and FE
    {0x096D, 0x15, 1, MAX96724_ADDR, 0},    	// Map_D-PHY_DST Sets PHY destination to PHY1 for mappings 0-2
    {0x094D, 0x1E, 1, MAX96724_ADDR, 0},    	// Map_SRC_0 - YUV422 8bit, VC = 0
    {0x094E, 0x5E, 1, MAX96724_ADDR, 0},    	// Map_DST_0 - YUV422 8bit, VC = 1
    {0x094F, 0x00, 1, MAX96724_ADDR, 0},    	// Map_SRC_1 - FS, VC = 0
    {0x0950, 0x40, 1, MAX96724_ADDR, 0},    	// Map_DST_1 - FS, VC = 1
    {0x0951, 0x01, 1, MAX96724_ADDR, 0},    	// Map_SRC_2 - FE, VC = 0
    {0x0952, 0x41, 1, MAX96724_ADDR, DLY_50MS},	// Map_DST_2 - FE, VC = 1

    // Deserializer Enable double and HVEN (LinkA/B)
    {0x0B07, 0x84, 1, MAX96724_ADDR, 0},
    {0x0C07, 0x84, 1, MAX96724_ADDR, DLY_50MS},

    // Enable All Links in GMSL1 mode
    {0x0006, 0x03, 1, MAX96724_ADDR, DLY_5MS},

    // Enable Link A/B serialization & Disable configuration link
    {0x04, 0x87, 0, MAX96705_ADDR_NEW_LINKA, 0},
    {0x04, 0x87, 0, MAX96705_ADDR_NEW_LINKB, DLY_50MS},

    // Disable linkA/B local-ack
    {0x0B0D, 0x00, 1, MAX96724_ADDR, 0},
    {0x0C0D, 0x00, 1, MAX96724_ADDR, 0},

    // CSI_OUT_EN enabled
    {0x040B, 0x42, 1, MAX96724_ADDR, 0},
};

int gpio_write_a16_d8(struct v4l2_subdev *sd, unsigned short chip_addr, addr_type reg,
		data_type value)
{
	int ret = 0, cnt = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	// Replace the i2c address in DTS with the specified I2C address
	client->addr = chip_addr;

	ret = cci_write_a16_d8(sd, reg, (unsigned char)value);
	while ((ret != 0) && cnt < 2) {
		ret = cci_write_a16_d8(sd, reg, (unsigned char)value);
		cnt++;
	}
	if (cnt > 0)
		sensor_err("sensor write retry = %d\n", cnt);

	return ret;
}

int gpio_write_a8_d8(struct v4l2_subdev *sd, unsigned short chip_addr, addr_type reg,
		   data_type value)
{
	int ret = 0, cnt = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	// Replace the i2c address in DTS with the specified I2C address
	client->addr = chip_addr;

	ret = cci_write_a8_d8(sd, (unsigned char)reg, (unsigned char)value);
	if (ret)
		sensor_err("cci_write_a8_d8 failed! device_addr:0x%2x, reg:0x%2x, val:0x%2x\n", chip_addr, reg, value);
	while ((ret != 0) && cnt < 2) {
		ret = cci_write_a8_d8(sd, (unsigned char)reg, (unsigned char)value);
		if (ret)
			sensor_err("cci_write_a8_d8 failed! device_addr:0x%2x, reg:0x%2x, val:0x%2x\n", chip_addr, reg, value);
		cnt++;
	}
	if (cnt > 0)
		sensor_err(" %s sensor write retry = %d\n", sd->name, cnt);

	usleep_range(50000, 55000);

	return ret;
}

__maybe_unused static unsigned int gpio_i2c_read(struct v4l2_subdev *sd, unsigned short chip_addr, int reg, int bit_flag)
{
	int ret;
	unsigned char val;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	// Replace the i2c address in DTS with the specified I2C address
	client->addr = chip_addr;

	if (bit_flag == 0)
		ret = cci_read_a8_d8(sd, (unsigned char)reg, (unsigned char *)&val);
	else
		ret = cci_read_a16_d8(sd, reg, (unsigned char *)&val);

	return (unsigned int)val;
}

static unsigned int maxim_reg_write(struct v4l2_subdev *sd, maxim_reglist *reg)
{
	int ret = 0;

	if (reg->wr16 == 1) {
		ret = gpio_write_a16_d8(sd, reg->device_addr, reg->reg_addr, reg->data);
	} else {
		ret = gpio_write_a8_d8(sd, reg->device_addr, reg->reg_addr, reg->data);
	}

	if (ret) {
		sensor_err("maxim_reg_write failed!!! device_addr:0x%2x, reg:0x%2x, val:0x%2x\n",
				reg->device_addr, reg->reg_addr, reg->data);
		return ret;
	}

	if (reg->delay_time != 0)
			msleep(reg->delay_time);

	return ret;
}

__maybe_unused static unsigned int maxim_reg_init(struct v4l2_subdev *sd, maxim_reglist *reg, int reg_size)
{
	int i = 0, cnt = 0, ret = 0;

	if (0 == reg_size)
		return 0;

	for (i = 0; i < reg_size; i++) {
		ret = maxim_reg_write(sd, &reg[i]);
		if (0 == ret)
			cnt++;
	}

	sensor_print("maxim_reg_init, write count: %d/%d !\r\n", cnt, reg_size);

	return 0;
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	ret = 0;
	switch (on) {
	case STBY_ON:
		sensor_print("STBY_ON!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_print("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(20000, 22000);

		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);

		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, PWDN, 1);

		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);

		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);

		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");

		cci_lock(sd);
		vin_set_mclk(sd, OFF);

		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);

		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);

		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
#if !defined CONFIG_VIN_INIT_MELIS
	data_type rdval = 0;
	int cnt = 0;

	sensor_read(sd, 0x000d, &rdval);
	sensor_print("reg 0x000d (ChipID) = 0x%2x\n", rdval);

	while ((rdval != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(sd, 0x000d, &rdval);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt, rdval);
		cnt++;
	}
	if (rdval != V4L2_IDENT_SENSOR)
		return -ENXIO;
#endif

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");
	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);

	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1280;
	info->height = 720;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->tpf.numerator = 1;
	info->tpf.denominator = 30;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static struct sensor_format_struct sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,  //  MEDIA_BUS_FMT_UYVY8_2X8
		.regs 		= NULL,
		.regs_size = 0,
		.bpp		= 2,
	}
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	{
	  .width = 1280,
	  .height = 720,
	  .hoffset = 0,
	  .voffset = 0,
	  .pclk = 150*1000*1000,
	  .mipi_bps = 600*1000*1000,
	  .fps_fixed = 30,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	if (!strcmp(sd->name, SENSOR_NAME_2))
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNELS;
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
		container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info = container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}

	return 0;
}

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	if (wsize->regs)
		maxim_reg_init(sd, wsize->regs, wsize->regs_size);

	if (!strcmp(sd->name, SENSOR_NAME_2)) {
		maxim_reg_init(sd, sensor_720p_30fps_2ch, ARRAY_SIZE(sensor_720p_30fps_2ch));
	} else {
		maxim_reg_init(sd, sensor_720p_30fps_4ch, ARRAY_SIZE(sensor_720p_30fps_4ch));
	}

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	sensor_dbg("s_fmt set width = %d, height = %d, fps = %d\n", wsize->width,
				wsize->height, wsize->fps_fixed);

	sensor_dbg("sensor_reg_init\n");

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */
static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;

}

static int sensor_dev_id;
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;
	__maybe_unused int err;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x20;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			.owner = THIS_MODULE,
			.name = SENSOR_NAME,
			},
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			.owner = THIS_MODULE,
			.name = SENSOR_NAME_2,
			},
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	}
};

static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);
