// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/auxiliary_bus.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/media-bus-format.h>
#include <linux/atomic.h>
#include <linux/workqueue.h>
#include <asm/unaligned.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_bridge.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_of.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_bridge.h>
#include <linux/delay.h>
#include <drm/display/drm_dp_aux_bus.h>
#include <drm/display/drm_dp_helper.h>

#define IT8911_DSI_DRIVER_NAME "ky-edp-drv"
#define EDID_SEG_SIZE	256
#define EDID_LEN	16

#define MIPI_DSI_1920x1080  1
#define MIPI_DSI_1920x1200  0

// #define _test_pattern_
// #define _read_edid_
// #define _uart_debug_
#define _eDP_2G7_
#define _link_train_enable_

#define _MIPI_Lane_ 4	// 4 3 2 1
#define _MIPI_data_PN_Swap_En	0xF0
#define _MIPI_data_PN_Swap_Dis	0x00
#define _MIPI_data_PN_ _MIPI_data_PN_Swap_Dis
#define _Nvid 0	// default 0: 0x0080
static int Nvid_Val[] = {0x0080, 0x0800};

#define _No_swap_		0x00	// default
#define _MIPI_data_3210_	0	// default
#define _MIPI_data_0123_	21
#define _MIPI_data_2103_	20
#define _MIPI_data_sequence_ _No_swap_

#define eDP_lane	2
#define PCR_PLL_PREDIV	0x40

#define LT8911_LOW 1
#define LT8911_HIGH 0

static int MIPI_Timing[] =
// hfp,	hs,	hbp,	hact,	htotal,	vfp,	vs,	vbp,	vact,	vtotal,	pixel_CLK/10000

// 1920x1080
#if MIPI_DSI_1920x1080
// {48, 32, 200, 1920, 2200, 3, 6, 31, 1080, 1120, 14784};     // boe config for linux
{48, 32, 200, 1920, 2200, 3, 6, 31, 1080, 1120, 14285};     // boe config for linux
#endif

// 1920x1200
#if MIPI_DSI_1920x1200
// {16, 16, 298, 1920, 2250, 3, 14, 19, 1200, 1236, 16684};     // boe config for linux
{16, 16, 298, 1920, 2250, 3, 14, 19, 1200, 1236, 15360};     // boe config for linux
// {16, 16, 298, 1920, 2250, 3, 14, 19, 1200, 1236, 14285};     // boe config for linux
#endif

#define _8bit_

enum {
	hfp = 0,
	hs,
	hbp,
	hact,
	htotal,
	vfp,
	vs,
	vbp,
	vact,
	vtotal,
	pclk_10khz
};

u32 EDID_DATA[128] = { 0 };
u32 EDID_Timing[11] = { 0 };
bool EDID_Reply = 0;

bool	ScrambleMode = 0;

static const struct drm_display_mode lt8911exb_panel_modes[] = {
// 1920x1080
#if MIPI_DSI_1920x1080
	{
		.clock = 142857143 / 1000,
		.hdisplay = 1920,
		.hsync_start = 1920 + 48,
		.hsync_end = 1920 + 48 + 200,
		.htotal = 1920 + 48 + 200 + 32,
		.vdisplay = 1080,
		.vsync_start = 1080 + 3,
		.vsync_end = 1080 + 3 + 31,
		.vtotal = 1080 + 3 + 31 + 6,
	},
#endif

// 1920x1200
#if MIPI_DSI_1920x1200

	{
		.clock = 142857143 / 1000,
		.hdisplay = 1920,
		.hsync_start = 1920 + 16,
		.hsync_end = 1920 + 16 + 298,
		.htotal = 1920 + 16 + 298 + 16,
		.vdisplay = 1200,
		.vsync_start = 1200 + 3,
		.vsync_end = 1200 + 3 + 19,
		.vtotal = 1200 + 3 + 19 + 14,
	},
#endif

};

enum
{
	_Level0_ = 0,	// 27.8 mA	0x83/0x00
	_Level1_,	// 26.2 mA	0x82/0xe0
	_Level2_,	// 24.6 mA	0x82/0xc0
	_Level3_,	// 23 mA	0x82/0xa0
	_Level4_,	// 21.4 mA	0x82/0x80
	_Level5_,	// 18.2 mA	0x82/0x40
	_Level6_,	// 16.6 mA	0x82/0x20
	_Level7_,	// 15 mA	0x82/0x00	// level 1
	_Level8_,	// 12.8mA	0x81/0x00	// level 2
	_Level9_,	// 11.2mA	0x80/0xe0	// level 3
	_Level10_,	// 9.6mA	0x80/0xc0	// level 4
	_Level11_,	// 8mA		0x80/0xa0	// level 5
	_Level12_,	// 6mA		0x80/0x80	// level 6
};

u8	Swing_Setting1[] = {0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x81, 0x80, 0x80, 0x80, 0x80};
u8	Swing_Setting2[] = {0x00, 0xe0, 0xc0, 0xa0, 0x80, 0x40, 0x20, 0x00, 0x00, 0xe0, 0xc0, 0xa0, 0x80};

u8	Level = _Level7_;	// normal

struct lt8911exb {
	struct device *dev;
	struct drm_bridge bridge;
	struct drm_connector connector;

	struct regmap *regmap;

	struct device_node *dsi0_node;
	struct device_node *dsi1_node;
	struct mipi_dsi_device *dsi0;
	struct mipi_dsi_device *dsi1;

	struct gpio_desc *reset_gpio;	//reset
	struct gpio_desc *enable_gpio;	//power
	struct gpio_desc *standby_gpio;	//standby
	struct gpio_desc *bl_gpio;	//backlight

	bool power_on;
	bool sleep;

	struct regulator_bulk_data supplies[2];

	struct i2c_client *client;
	struct drm_panel base;
	struct mipi_dsi_device *dsi;

	enum drm_connector_status status;

	u8 edid_buf[EDID_SEG_SIZE];
	u32 vic;

	struct delayed_work init_work;
	bool init_work_pending;
};

static const struct regmap_config lt8911exb_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
};

static struct lt8911exb *panel_to_lt8911exb(struct drm_panel *panel)
{
	return container_of(panel, struct lt8911exb, base);
}

void lt8911exb_mipi_video_timing(struct lt8911exb *lt8911exb)
{
	__maybe_unused unsigned int tmp;

	regmap_write(lt8911exb->regmap, 0xff, 0xd0);
	regmap_write(lt8911exb->regmap, 0x0d, (MIPI_Timing[vtotal] / 256));
	regmap_write(lt8911exb->regmap, 0x0e, (MIPI_Timing[vtotal] % 256));	//vtotal
	regmap_write(lt8911exb->regmap, 0x0f, (MIPI_Timing[vact] / 256));
	regmap_write(lt8911exb->regmap, 0x10, (MIPI_Timing[vact] % 256));	//vactive
	regmap_write(lt8911exb->regmap, 0x11, (MIPI_Timing[htotal] / 256));
	regmap_write(lt8911exb->regmap, 0x12, (MIPI_Timing[htotal] % 256));	//htotal
	regmap_write(lt8911exb->regmap, 0x13, (MIPI_Timing[hact] / 256));
	regmap_write(lt8911exb->regmap, 0x14, (MIPI_Timing[hact] % 256));	//hactive
	regmap_write(lt8911exb->regmap, 0x15, (MIPI_Timing[vs] % 256));		//vsa
	regmap_write(lt8911exb->regmap, 0x16, (MIPI_Timing[hs] % 256));		//hsa
	regmap_write(lt8911exb->regmap, 0x17, (MIPI_Timing[vfp] / 256));
	regmap_write(lt8911exb->regmap, 0x18, (MIPI_Timing[vfp] % 256));	//vfp
	regmap_write(lt8911exb->regmap, 0x19, (MIPI_Timing[hfp] / 256));
	regmap_write(lt8911exb->regmap, 0x1a, (MIPI_Timing[hfp] % 256));	//hfp

#ifdef _uart_debug_
	DRM_INFO("------\n");
	DRM_INFO("MIPI_Timing[vtotal] / 256 = %d\n", MIPI_Timing[vtotal] / 256);
	DRM_INFO("MIPI_Timing[vtotal]  256 = %d\n", MIPI_Timing[vtotal] % 256);
	DRM_INFO("MIPI_Timing[vact] / 256 = %d\n", MIPI_Timing[vact] / 256);
	DRM_INFO("MIPI_Timing[vact]  256 = %d\n", MIPI_Timing[vact] % 256);
	DRM_INFO("MIPI_Timing[htotal] / 256 = %d\n", MIPI_Timing[htotal] / 256);
	DRM_INFO("MIPI_Timing[htotal]  256 = %d\n", MIPI_Timing[htotal] % 256);
	DRM_INFO("MIPI_Timing[hact] / 256 = %d\n", MIPI_Timing[hact] / 256);
	DRM_INFO("MIPI_Timing[hact]  256 = %d\n", MIPI_Timing[hact] % 256);

	DRM_INFO("MIPI_Timing[vs]  256 = %d\n", MIPI_Timing[vs] % 256);
	DRM_INFO("MIPI_Timing[hs]  256 = %d\n", MIPI_Timing[hs] % 256);

	DRM_INFO("MIPI_Timing[vfp] / 256 = %d\n", MIPI_Timing[vfp] / 256);
	DRM_INFO("MIPI_Timing[vfp]  256 = %d\n", MIPI_Timing[vfp] % 256);
	DRM_INFO("MIPI_Timing[hfp] / 256 = %d\n", MIPI_Timing[hfp] / 256);
	DRM_INFO("MIPI_Timing[hfp]  256 = %d\n", MIPI_Timing[hfp] % 256);
	DRM_INFO("------\n");

	regmap_read(lt8911exb->regmap, 0x0d, &tmp);
	DRM_DEBUG_ATOMIC("0x0d = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x0e, &tmp);
	DRM_DEBUG_ATOMIC("0x0e = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x0f, &tmp);
	DRM_DEBUG_ATOMIC("0x0f = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x10, &tmp);
	DRM_DEBUG_ATOMIC("0x10 = %d\n",tmp);

	regmap_read(lt8911exb->regmap, 0x11, &tmp);
	DRM_DEBUG_ATOMIC("0x11 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x12, &tmp);
	DRM_DEBUG_ATOMIC("0x12 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x13, &tmp);
	DRM_DEBUG_ATOMIC("0x13 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x14, &tmp);
	DRM_DEBUG_ATOMIC("0x14 = %d\n",tmp);


	regmap_read(lt8911exb->regmap, 0x15, &tmp);
	DRM_DEBUG_ATOMIC("0x15 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x16, &tmp);
	DRM_DEBUG_ATOMIC("0x16 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x17, &tmp);
	DRM_DEBUG_ATOMIC("0x17 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x18, &tmp);
	DRM_DEBUG_ATOMIC("0x18 = %d\n",tmp);

	regmap_read(lt8911exb->regmap, 0x19, &tmp);
	DRM_DEBUG_ATOMIC("0x19 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x1a, &tmp);
	DRM_DEBUG_ATOMIC("0x1a = %d\n",tmp);
#endif
}

void lt8911exb_edp_video_cfg(struct lt8911exb *lt8911exb)
{
	__maybe_unused unsigned int tmp;

	regmap_write(lt8911exb->regmap, 0xff, 0xa8);
	regmap_write(lt8911exb->regmap, 0x2d, 0x88);	// MSA from register
	regmap_write(lt8911exb->regmap, 0x05, (MIPI_Timing[htotal] / 256));
	regmap_write(lt8911exb->regmap, 0x06, (MIPI_Timing[htotal] % 256));
	regmap_write(lt8911exb->regmap, 0x07, ((MIPI_Timing[hs] + MIPI_Timing[hbp]) / 256 ));
	regmap_write(lt8911exb->regmap, 0x08, ((MIPI_Timing[hs] + MIPI_Timing[hbp]) % 256));
	regmap_write(lt8911exb->regmap, 0x09, (MIPI_Timing[hs] / 256));
	regmap_write(lt8911exb->regmap, 0x0a, (MIPI_Timing[hs] % 256));
	regmap_write(lt8911exb->regmap, 0x0b, (MIPI_Timing[hact] / 256));
	regmap_write(lt8911exb->regmap, 0x0c, (MIPI_Timing[hact] % 256));
	regmap_write(lt8911exb->regmap, 0x0d, (MIPI_Timing[vtotal] / 256));
	regmap_write(lt8911exb->regmap, 0x0e, (MIPI_Timing[vtotal] % 256));
	regmap_write(lt8911exb->regmap, 0x11, ((MIPI_Timing[vs] + MIPI_Timing[vbp]) / 256));
	regmap_write(lt8911exb->regmap, 0x12, ((MIPI_Timing[vs] + MIPI_Timing[vbp]) % 256));
	regmap_write(lt8911exb->regmap, 0x14, (MIPI_Timing[vs] % 256));
	regmap_write(lt8911exb->regmap, 0x15, (MIPI_Timing[vact] / 256));
	regmap_write(lt8911exb->regmap, 0x16, (MIPI_Timing[vact] % 256));

#ifdef _uart_debug_
	DRM_INFO("------\n");
	DRM_INFO("(u8)( MIPI_Timing[htotal] / 256 ) = %d\n", (MIPI_Timing[htotal] / 256));
	DRM_INFO("(u8)( MIPI_Timing[htotal]  256 ) = %d\n", (MIPI_Timing[htotal] % 256));
	DRM_INFO("(u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] ) / 256 )  = %d\n", ((MIPI_Timing[hs] + MIPI_Timing[hbp]) / 256));
	DRM_INFO("(u8)( ( MIPI_Timing[hs] + MIPI_Timing[hbp] )  256 ) = %d\n", ((MIPI_Timing[hs] + MIPI_Timing[hbp]) % 256));
	DRM_INFO("(u8)( MIPI_Timing[hs] / 256 ) = %d\n", (MIPI_Timing[hs] / 256));
	DRM_INFO(" (u8)( MIPI_Timing[hs]  256 ) = %d\n", (MIPI_Timing[hs] % 256));
	DRM_INFO(" (u8)( MIPI_Timing[hact] / 256 )  = %d\n", (MIPI_Timing[hact] / 256));
	DRM_INFO("(u8)( MIPI_Timing[hact]  256 ) = %d\n", (MIPI_Timing[hact] % 256));

	DRM_INFO("(u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] ) / 256 ) = %d\n", ((MIPI_Timing[vs] + MIPI_Timing[vbp]) / 256));
	DRM_INFO(" (u8)( ( MIPI_Timing[vs] + MIPI_Timing[vbp] )  256 ) = %d\n", ((MIPI_Timing[vs] + MIPI_Timing[vbp]) % 256));

	DRM_INFO(" (u8)( MIPI_Timing[vs]  256 ) = %d\n", (MIPI_Timing[vs] % 256));
	DRM_INFO(" (u8)( MIPI_Timing[vact] / 256 )  = %d\n", (MIPI_Timing[vact] / 256));
	DRM_INFO("(u8)( MIPI_Timing[vact]  256 ) = %d\n", (MIPI_Timing[vact] % 256));
	DRM_INFO("------\n");

	regmap_read(lt8911exb->regmap, 0x05, &tmp);
	DRM_DEBUG_ATOMIC("0x05 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x06, &tmp);
	DRM_DEBUG_ATOMIC("0x06 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x07, &tmp);
	DRM_DEBUG_ATOMIC("0x07 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x08, &tmp);
	DRM_DEBUG_ATOMIC("0x08 = %d\n",tmp);

	regmap_read(lt8911exb->regmap, 0x09, &tmp);
	DRM_DEBUG_ATOMIC("0x09 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x0a, &tmp);
	DRM_DEBUG_ATOMIC("0x0a = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x0b, &tmp);
	DRM_DEBUG_ATOMIC("0x0b = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x0c, &tmp);
	DRM_DEBUG_ATOMIC("0x0c = %d\n",tmp);


	regmap_read(lt8911exb->regmap, 0x0d, &tmp);
	DRM_DEBUG_ATOMIC("0x0d = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x0e, &tmp);
	DRM_DEBUG_ATOMIC("0x0e = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x11, &tmp);
	DRM_DEBUG_ATOMIC("0x11 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x12, &tmp);
	DRM_DEBUG_ATOMIC("0x12 = %d\n",tmp);

	regmap_read(lt8911exb->regmap, 0x14, &tmp);
	DRM_DEBUG_ATOMIC("0x14 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x15, &tmp);
	DRM_DEBUG_ATOMIC("0x15 = %d\n",tmp);
	regmap_read(lt8911exb->regmap, 0x16, &tmp);
	DRM_DEBUG_ATOMIC("0x16 = %d\n",tmp);
#endif
}

void lt8911exb_read_edid(struct lt8911exb *lt8911exb)
{
#ifdef _read_edid_
	u8 i, j;
	unsigned int reg;

	DRM_INFO("%s()\n", __func__);

	regmap_write(lt8911exb->regmap, 0xff, 0xac );
	regmap_write(lt8911exb->regmap, 0x00, 0x20 ); //Soft Link train
	regmap_write(lt8911exb->regmap, 0xff, 0xa6 );
	regmap_write(lt8911exb->regmap, 0x2a, 0x01 );

	/*set edid offset addr*/
	regmap_write(lt8911exb->regmap, 0x2b, 0x40 ); //CMD
	regmap_write(lt8911exb->regmap, 0x2b, 0x00 ); //addr[15:8]
	regmap_write(lt8911exb->regmap, 0x2b, 0x50 ); //addr[7:0]
	regmap_write(lt8911exb->regmap, 0x2b, 0x00 ); //data lenth
	regmap_write(lt8911exb->regmap, 0x2b, 0x00 ); //data lenth
	regmap_write(lt8911exb->regmap, 0x2c, 0x00 ); //start Aux read edid

	mdelay(15);                         //more than 10ms
	regmap_read(lt8911exb->regmap, 0x25, &reg);
	DRM_INFO("lt8911exb_read_edid 0x25 0x%x\n", reg);

	if( ( reg & 0x0f ) == 0x0c )
	{
		for( j = 0; j < 8; j++ )
		{
			if( j == 7 )
			{
				regmap_write(lt8911exb->regmap, 0x2b, 0x10 ); //MOT
			}else
			{
				regmap_write(lt8911exb->regmap, 0x2b, 0x50 );
			}

			regmap_write(lt8911exb->regmap, 0x2b, 0x00 );
			regmap_write(lt8911exb->regmap, 0x2b, 0x50 );
			regmap_write(lt8911exb->regmap, 0x2b, 0x0f );
			regmap_write(lt8911exb->regmap, 0x2c, 0x00 ); //start Aux read edid
			mdelay(50);                                   //more than 50ms

			regmap_read(lt8911exb->regmap, 0x39, &reg);
			DRM_INFO("lt8911exb_read_edid 0x39 0x%x\n", reg);
			if (reg == 0x31)
			{
				regmap_read(lt8911exb->regmap, 0x2b, &reg);
				DRM_INFO("lt8911exb_read_edid 0x2b 0x%x\n", reg);
				for( i = 0; i < 16; i++ )
				{
					regmap_read(lt8911exb->regmap, 0x2b, &reg);
					EDID_DATA[j * 16 + i] = reg;
				}

				EDID_Reply = 1;
			}else
			{
				EDID_Reply = 0;
				return;
			}
		}

		// for( i = 0; i < 128; i++ ) //print edid data
		// {
		// 	if( ( i % 16 ) == 0 )
		// 	{
		// 		DRM_INFO( "\n" );
		// 	}
		// 	DRM_INFO( "%x", EDID_DATA[i] );
		// }


		EDID_Timing[hfp] = (EDID_DATA[0x41] & 0xC0) * 4 + EDID_DATA[0x3e];
		EDID_Timing[hs] = (EDID_DATA[0x41] & 0x30) * 16 + EDID_DATA[0x3f];
		EDID_Timing[hbp] = ((EDID_DATA[0x3a] & 0x0f) * 0x100 + EDID_DATA[0x39]) - ((EDID_DATA[0x41] & 0x30) * 16 + EDID_DATA[0x3f]) - ((EDID_DATA[0x41] & 0xC0 ) * 4 + EDID_DATA[0x3e]);
		EDID_Timing[hact] = (EDID_DATA[0x3a] & 0xf0) * 16 + EDID_DATA[0x38];
		EDID_Timing[htotal] = (EDID_DATA[0x3a] & 0xf0) * 16 + EDID_DATA[0x38] + ((EDID_DATA[0x3a] & 0x0f) * 0x100 + EDID_DATA[0x39]);
		EDID_Timing[vfp] = (EDID_DATA[0x41] & 0x0c) * 4 + (EDID_DATA[0x40] & 0xf0 ) / 16;
		EDID_Timing[vs] = (EDID_DATA[0x41] & 0x03) * 16 + (EDID_DATA[0x40] & 0x0f );
		EDID_Timing[vbp] = ((EDID_DATA[0x3d] & 0x03) * 0x100 + EDID_DATA[0x3c]) - ((EDID_DATA[0x41] & 0x03) * 16 + (EDID_DATA[0x40] & 0x0f)) - ((EDID_DATA[0x41] & 0x0c) * 4 + (EDID_DATA[0x40] & 0xf0 ) / 16);
		EDID_Timing[vact] = (EDID_DATA[0x3d] & 0xf0) * 16 + EDID_DATA[0x3b];
		EDID_Timing[vtotal] = (EDID_DATA[0x3d] & 0xf0 ) * 16 + EDID_DATA[0x3b] + ((EDID_DATA[0x3d] & 0x03) * 0x100 + EDID_DATA[0x3c]);
		EDID_Timing[pclk_10khz] = EDID_DATA[0x37] * 0x100 + EDID_DATA[0x36];
		DRM_INFO( "eDP Timing = { H_FP / H_pluse / H_BP / H_act / H_tol / V_FP / V_pluse / V_BP / V_act / V_tol / D_CLK };" );
		DRM_INFO( "eDP Timing = { %d       %d       %d     %d     %d     %d      %d       %d     %d    %d    %d };\n",
				(u32)EDID_Timing[hfp],(u32)EDID_Timing[hs],(u32)EDID_Timing[hbp],(u32)EDID_Timing[hact],(u32)EDID_Timing[htotal],
				(u32)EDID_Timing[vfp],(u32)EDID_Timing[vs],(u32)EDID_Timing[vbp],(u32)EDID_Timing[vact],(u32)EDID_Timing[vtotal],(u32)EDID_Timing[pclk_10khz]);
	}

	return;

#endif
}

void lt8911exb_setup(struct lt8911exb *lt8911exb)
{
	u8	i;
	u8	pcr_pll_postdiv;
	u8	pcr_m;
	u16 Temp16;
	u32 chip_read = 0x00;

	DRM_DEBUG("\r\n lt8911exb_setup");

	/* init */
	regmap_write(lt8911exb->regmap, 0xff, 0x81); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x08, 0x7f); // i2c over aux issue
	regmap_write(lt8911exb->regmap, 0x49, 0xff); // enable 0x87xx

	regmap_write(lt8911exb->regmap, 0xff, 0x82); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x5a, 0x0e); // GPIO test output

	//for power consumption//
	regmap_write(lt8911exb->regmap, 0xff, 0x81);
	regmap_write(lt8911exb->regmap, 0x05, 0x06);
	regmap_write(lt8911exb->regmap, 0x43, 0x00);
	regmap_write(lt8911exb->regmap, 0x44, 0x1f);
	regmap_write(lt8911exb->regmap, 0x45, 0xf7);
	regmap_write(lt8911exb->regmap, 0x46, 0xf6);
	regmap_write(lt8911exb->regmap, 0x49, 0x7f);

	regmap_write(lt8911exb->regmap, 0xff, 0x82);
#if (eDP_lane == 2)
	{
		regmap_write(lt8911exb->regmap, 0x12, 0x33);
	}
#elif (eDP_lane == 1)
	{
		regmap_write(lt8911exb->regmap, 0x12, 0x11);
	}
#endif

	/* mipi Rx analog */
	regmap_write(lt8911exb->regmap, 0xff, 0x82); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x32, 0x51);
	regmap_write(lt8911exb->regmap, 0x35, 0x22); //EQ current 0x22/0x42/0x62/0x82/0xA2/0xC2/0xe2
	regmap_write(lt8911exb->regmap, 0x3a, 0x77); //EQ 12.5db
	regmap_write(lt8911exb->regmap, 0x3b, 0x77); //EQ 12.5db

	regmap_write(lt8911exb->regmap, 0x4c, 0x0c);
	regmap_write(lt8911exb->regmap, 0x4d, 0x00);

	/* dessc_pcr  pll analog */
	regmap_write(lt8911exb->regmap, 0xff, 0x82); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x6a, 0x40);
	regmap_write(lt8911exb->regmap, 0x6b, PCR_PLL_PREDIV);

	Temp16 = MIPI_Timing[pclk_10khz];

	if (MIPI_Timing[pclk_10khz] < 8800) {
		regmap_write(lt8911exb->regmap, 0x6e, 0x82); //0x44:pre-div = 2 ,pixel_clk=44~ 88MHz
		pcr_pll_postdiv = 0x08;
	} else if (MIPI_Timing[pclk_10khz] < 17600){
		regmap_write(lt8911exb->regmap, 0x6e, 0x81); //0x40:pre-div = 1, pixel_clk =88~176MHz
		pcr_pll_postdiv = 0x04;
	} else {
		regmap_write(lt8911exb->regmap, 0x6e, 0x80); //0x40:pre-div = 0, pixel_clk =176~200MHz
		pcr_pll_postdiv = 0x02;
	}

	pcr_m = (u8)(Temp16 * pcr_pll_postdiv / 25 / 100);

	/* dessc pll digital */
	regmap_write(lt8911exb->regmap, 0xff, 0x85);	// Change Reg bank
	regmap_write(lt8911exb->regmap, 0xa9, 0x31);
	regmap_write(lt8911exb->regmap, 0xaa, 0x17);
	regmap_write(lt8911exb->regmap, 0xab, 0xba);
	regmap_write(lt8911exb->regmap, 0xac, 0xe1);
	regmap_write(lt8911exb->regmap, 0xad, 0x47);
	regmap_write(lt8911exb->regmap, 0xae, 0x01);
	regmap_write(lt8911exb->regmap, 0xae, 0x11);

	/* Digital Top */
	regmap_write(lt8911exb->regmap, 0xff, 0x85);	// Change Reg bank
	regmap_write(lt8911exb->regmap, 0xc0, 0x01);	//select mipi Rx
#ifdef _6bit_
	regmap_write(lt8911exb->regmap, 0xb0, 0xd0);	//enable dither
#else
	regmap_write(lt8911exb->regmap, 0xb0, 0x00);	// disable dither
#endif

	/* mipi Rx Digital */
	regmap_write(lt8911exb->regmap, 0xff, 0xd0);	// Change Reg bank
	regmap_write(lt8911exb->regmap, 0x00, _MIPI_data_PN_ + _MIPI_Lane_ % 4); // 0: 4 Lane / 1: 1 Lane / 2 : 2 Lane / 3: 3 Lane
	regmap_write(lt8911exb->regmap, 0x02, 0x08);	//settle
	regmap_write(lt8911exb->regmap, 0x03, _MIPI_data_sequence_);	// default is 0x00
	regmap_write(lt8911exb->regmap, 0x08, 0x00);
//	regmap_write(lt8911exb->regmap, 0x0a, 0x12);	//pcr mode

	regmap_write(lt8911exb->regmap, 0x0c, 0x80);	//fifo position
	regmap_write(lt8911exb->regmap, 0x1c, 0x80);	//fifo position

	//	hs mode:MIPI行采样；vs mode:MIPI帧采样
	regmap_write(lt8911exb->regmap, 0x24, 0x70);	// 0x30  [3:0]  line limit	  //pcr mode( de hs vs)

	regmap_write(lt8911exb->regmap, 0x31, 0x0a);

	/*stage1 hs mode*/
	regmap_write(lt8911exb->regmap, 0x25, 0x90);	// 0x80	// line limit
	regmap_write(lt8911exb->regmap, 0x2a, 0x3a);	// 0x04	// step in limit
	regmap_write(lt8911exb->regmap, 0x21, 0x4f);	// hs_step
	regmap_write(lt8911exb->regmap, 0x22, 0xff);

	/*stage2 de mode*/
	regmap_write(lt8911exb->regmap, 0x0a, 0x02);	//de adjust pre line
	regmap_write(lt8911exb->regmap, 0x38, 0x02);	//de_threshold 1
	regmap_write(lt8911exb->regmap, 0x39, 0x04);	//de_threshold 2
	regmap_write(lt8911exb->regmap, 0x3a, 0x08);	//de_threshold 3
	regmap_write(lt8911exb->regmap, 0x3b, 0x10);	//de_threshold 4

	regmap_write(lt8911exb->regmap, 0x3f, 0x04);	//de_step 1
	regmap_write(lt8911exb->regmap, 0x40, 0x08);	//de_step 2
	regmap_write(lt8911exb->regmap, 0x41, 0x10);	//de_step 3
	regmap_write(lt8911exb->regmap, 0x42, 0x60);	//de_step 4

	/*stage2 hs mode*/
	// regmap_write(lt8911exb->regmap, 0x1e, 0x0A); //regmap_write(lt8911exb->regmap, 0x1e, 0x01 );                             // 0x11
	DRM_DEBUG("\r\n lt8911exb_setup 0X1e 0x0a");
	regmap_write(lt8911exb->regmap, 0x1e, 0x0a);
	regmap_write(lt8911exb->regmap, 0x23, 0xf0);	// 0x80

	regmap_write(lt8911exb->regmap, 0x2b, 0x80);	// 0xa0

#ifdef _test_pattern_
	regmap_write(lt8911exb->regmap, 0x26, (pcr_m | 0x80));
#else

	regmap_write(lt8911exb->regmap, 0x26, pcr_m);

//	regmap_write(lt8911exb->regmap, 0x27, Read_0xD095);
//	regmap_write(lt8911exb->regmap, 0x28, Read_0xD096);
#endif

	lt8911exb_mipi_video_timing(lt8911exb);	//defualt setting is 1080P

	regmap_write(lt8911exb->regmap, 0xff, 0x81); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x03, 0x7b); //PCR reset
	regmap_write(lt8911exb->regmap, 0x03, 0xff);

#ifdef _eDP_2G7_
	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x19, 0x31);
	regmap_write(lt8911exb->regmap, 0x1a, 0x36); // sync m
	regmap_write(lt8911exb->regmap, 0x1b, 0x00); // sync_k [7:0]
	regmap_write(lt8911exb->regmap, 0x1c, 0x00); // sync_k [13:8]

	// txpll Analog
	regmap_write(lt8911exb->regmap, 0xff, 0x82);
	regmap_write(lt8911exb->regmap, 0x09, 0x00); // div hardware mode, for ssc.

//	regmap_write(lt8911exb->regmap, 0x01, 0x18);// default : 0x18
	regmap_write(lt8911exb->regmap, 0x02, 0x42);
	regmap_write(lt8911exb->regmap, 0x03, 0x00); // txpll en = 0
	regmap_write(lt8911exb->regmap, 0x03, 0x01); // txpll en = 1
//	regmap_write(lt8911exb->regmap, 0x04, 0x3a);// default : 0x3A

	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x0c, 0x10); // cal en = 0

	regmap_write(lt8911exb->regmap, 0xff, 0x81);
	regmap_write(lt8911exb->regmap, 0x09, 0xfc);
	regmap_write(lt8911exb->regmap, 0x09, 0xfd);

	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x0c, 0x11); // cal en = 1

	// ssc
	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x13, 0x83);
	regmap_write(lt8911exb->regmap, 0x14, 0x41);
	regmap_write(lt8911exb->regmap, 0x16, 0x0a);
	regmap_write(lt8911exb->regmap, 0x18, 0x0a);
	regmap_write(lt8911exb->regmap, 0x19, 0x33);
#endif

#ifdef _eDP_1G62_
	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x19, 0x31);
	regmap_write(lt8911exb->regmap, 0x1a, 0x20); // sync m
	regmap_write(lt8911exb->regmap, 0x1b, 0x19); // sync_k [7:0]
	regmap_write(lt8911exb->regmap, 0x1c, 0x99); // sync_k [13:8]

	// txpll Analog
	regmap_write(lt8911exb->regmap, 0xff, 0x82);
	regmap_write(lt8911exb->regmap, 0x09, 0x00); // div hardware mode, for ssc.
	//	regmap_write(lt8911exb->regmap, 0x01, 0x18);// default : 0x18
	regmap_write(lt8911exb->regmap, 0x02, 0x42);
	regmap_write(lt8911exb->regmap, 0x03, 0x00); // txpll en = 0
	regmap_write(lt8911exb->regmap, 0x03, 0x01); // txpll en = 1
	//	regmap_write(lt8911exb->regmap, 0x04, 0x3a);// default : 0x3A

	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x0c, 0x10); // cal en = 0

	regmap_write(lt8911exb->regmap, 0xff, 0x81);
	regmap_write(lt8911exb->regmap, 0x09, 0xfc);
	regmap_write(lt8911exb->regmap, 0x09, 0xfd);

	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x0c, 0x11); // cal en = 1

	//ssc
	regmap_write(lt8911exb->regmap, 0xff, 0x87);
	regmap_write(lt8911exb->regmap, 0x13, 0x83);
	regmap_write(lt8911exb->regmap, 0x14, 0x41);
	regmap_write(lt8911exb->regmap, 0x16, 0x0a);
	regmap_write(lt8911exb->regmap, 0x18, 0x0a);
	regmap_write(lt8911exb->regmap, 0x19, 0x33);
#endif

	regmap_write(lt8911exb->regmap, 0xff, 0x87);

	for (i = 0; i < 5; i++) {//Check Tx PLL
		mdelay(2);
		regmap_read(lt8911exb->regmap, 0x37, &chip_read);
		if (chip_read & 0x02) {
			DRM_DEBUG("\r\n LT8911 tx pll locked");
			break;
		} else {
			DRM_DEBUG("\r\n LT8911 tx pll unlocked");
			regmap_write(lt8911exb->regmap, 0xff, 0x81);
			regmap_write(lt8911exb->regmap, 0x09, 0xfc);
			regmap_write(lt8911exb->regmap, 0x09, 0xfd);

			regmap_write(lt8911exb->regmap, 0xff, 0x87);
			regmap_write(lt8911exb->regmap, 0x0c, 0x10);
			regmap_write(lt8911exb->regmap, 0x0c, 0x11);
		}
	}

	// AUX reset
	regmap_write(lt8911exb->regmap, 0xff, 0x81); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x07, 0xfe);
	regmap_write(lt8911exb->regmap, 0x07, 0xff);
	regmap_write(lt8911exb->regmap, 0x0a, 0xfc);
	regmap_write(lt8911exb->regmap, 0x0a, 0xfe);

	/* tx phy */
	regmap_write(lt8911exb->regmap, 0xff, 0x82); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x11, 0x00);
	regmap_write(lt8911exb->regmap, 0x13, 0x10);
	regmap_write(lt8911exb->regmap, 0x14, 0x0c);
	regmap_write(lt8911exb->regmap, 0x14, 0x08);
	regmap_write(lt8911exb->regmap, 0x13, 0x20);

	regmap_write(lt8911exb->regmap, 0xff, 0x82); // Change Reg bank
	regmap_write(lt8911exb->regmap, 0x0e, 0x35);
//	regmap_write(lt8911exb->regmap, 0x12, 0xff);
//	regmap_write(lt8911exb->regmap, 0xff, 0x80);
//	regmap_write(lt8911exb->regmap, 0x40, 0x22);

	/*eDP Tx Digital */
	regmap_write(lt8911exb->regmap, 0xff, 0xa8); // Change Reg bank

#ifdef _test_pattern_

	regmap_write(lt8911exb->regmap, 0x24, 0x50); // bit2 ~ bit 0 : test panttern image mode
	regmap_write(lt8911exb->regmap, 0x25, 0x70); // bit6 ~ bit 4 : test Pattern color
	regmap_write(lt8911exb->regmap, 0x27, 0x50); //0x50:Pattern; 0x10:mipi video

//	regmap_write(lt8911exb->regmap, 0x2d, 0x00); //  pure color setting
//	regmap_write(lt8911exb->regmap, 0x2d, 0x84); // black color
	regmap_write(lt8911exb->regmap, 0x2d, 0x88); //  block

#else
	regmap_write(lt8911exb->regmap, 0x27, 0x10); //0x50:Pattern; 0x10:mipi video
#endif

#ifdef _6bit_
	regmap_write(lt8911exb->regmap, 0x17, 0x00);
	regmap_write(lt8911exb->regmap, 0x18, 0x00);
#else
	// _8bit_
	regmap_write(lt8911exb->regmap, 0x17, 0x10);
	regmap_write(lt8911exb->regmap, 0x18, 0x20);
#endif

	/* nvid */
	regmap_write(lt8911exb->regmap, 0xff, 0xa0);	// Change Reg bank
	regmap_write(lt8911exb->regmap, 0x00, (u8)(Nvid_Val[_Nvid] / 256));	// 0x08
	regmap_write(lt8911exb->regmap, 0x01, (u8)(Nvid_Val[_Nvid] % 256));	// 0x00
}

void lt8911exb_video_check(struct lt8911exb *lt8911exb)
{
	unsigned int reg;
	unsigned int temp2;

	/* mipi byte clk check*/
	regmap_write(lt8911exb->regmap, 0xff, 0x85);	// Change Reg bank
	regmap_write(lt8911exb->regmap, 0x1d, 0x00);	//FM select byte clk
	regmap_write(lt8911exb->regmap, 0x40, 0xf7);
	regmap_write(lt8911exb->regmap, 0x41, 0x30);

	if (ScrambleMode) {
		regmap_write(lt8911exb->regmap, 0xa1, 0x82 ); //eDP scramble mode;
	} else {
		regmap_write(lt8911exb->regmap, 0xa1, 0x02 ); // DP scramble mode;
	}

	//regmap_write(lt8911exb->regmap, 0x17, 0xf0 ); // 0xf0:Close scramble; 0xD0 : Open scramble

	regmap_write(lt8911exb->regmap, 0xff, 0x81);
	regmap_write(lt8911exb->regmap, 0x09, 0x7d);
	regmap_write(lt8911exb->regmap, 0x09, 0xfd);
	regmap_write(lt8911exb->regmap, 0xff, 0x85);
	mdelay(40);

	regmap_read(lt8911exb->regmap, 0x50, &temp2);
	if (temp2 == 0x03) {
		//reg	   = LT8911EXB_IIC_Read_byte( 0x4d );
		//reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x4e );
		//reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x4f );

		regmap_read(lt8911exb->regmap, 0x4d, &reg);
		DRM_DEBUG_ATOMIC("1 0x4d = %d\n",reg);
		regmap_read(lt8911exb->regmap, 0x4e, &temp2);
		DRM_DEBUG_ATOMIC("1 0x4e = %d\n",temp2);
		reg = reg * 256 + temp2;
		DRM_DEBUG_ATOMIC("1-1 reg = %d\n",reg);
		regmap_read(lt8911exb->regmap, 0x4f, &temp2);
		DRM_DEBUG_ATOMIC("1-1 0x4f = %d\n",temp2);
		reg = reg * 256 + temp2;
		DRM_DEBUG_ATOMIC("1-2 reg = %d\n",reg);


		DRM_DEBUG( "\r\nvideo check: mipi byteclk = %d ", reg ); // mipi byteclk = reg * 1000
		DRM_DEBUG( "\r\nvideo check: mipi bitrate = %d ", reg * 8); // mipi byteclk = reg * 1000
		DRM_DEBUG( "\r\nvideo check: mipi pclk = %d ", reg /3 * 4 * 1000); // mipi byteclk = reg * 1000
	} else {
		DRM_DEBUG( "\r\nvideo check: mipi clk unstable" );
	}

	/* mipi vtotal check*/
	//reg	   = LT8911EXB_IIC_Read_byte( 0x76 );
	//reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x77 );
	regmap_read(lt8911exb->regmap, 0x76, &reg);
	DRM_DEBUG_ATOMIC("2 0x76 = %d\n",reg);
	regmap_read(lt8911exb->regmap, 0x77, &temp2);
	DRM_DEBUG_ATOMIC("2 0x77 = %d\n",temp2);
	reg = reg * 256 + temp2;
	DRM_DEBUG_ATOMIC("2-1 reg = %d\n",reg);

	DRM_DEBUG( "\r\nvideo check: Vtotal =  %d", reg);

	/* mipi word count check*/
	regmap_write(lt8911exb->regmap, 0xff, 0xd0);
	//reg	   = LT8911EXB_IIC_Read_byte( 0x82 );
	//reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x83 );
	//reg	   = reg / 3;
	regmap_read(lt8911exb->regmap, 0x82, &reg);
	DRM_DEBUG_ATOMIC("3 0x82 = %d\n",reg);
	regmap_read(lt8911exb->regmap, 0x83, &temp2);
	DRM_DEBUG_ATOMIC("3 0x83 = %d\n",reg);
	reg = reg * 256 + temp2;
	reg	   = reg / 3;
	DRM_DEBUG_ATOMIC( "\r\n3-1 reg  = %d ", reg );


	DRM_DEBUG( "\r\nvideo check: Hact(word counter) =  %d", reg);

	/* mipi Vact check*/
	//reg	   = LT8911EXB_IIC_Read_byte( 0x85 );
	//reg	   = reg * 256 + LT8911EXB_IIC_Read_byte( 0x86 );
	regmap_read(lt8911exb->regmap, 0x85, &reg);
	DRM_DEBUG_ATOMIC("4 0x85 = %d\n",reg);
	regmap_read(lt8911exb->regmap, 0x86, &temp2);
	DRM_DEBUG_ATOMIC("4 0x86 = %d\n",temp2);
	reg = reg * 256 + temp2;
	DRM_DEBUG_ATOMIC( "\r\n4-1 reg  = %d ", reg );

	DRM_DEBUG( "\r\nvideo check: Vact = %d", reg);

}

void DpcdWrite(struct lt8911exb *lt8911exb, u32 Address, u8 Data)
{
	/***************************
	   注意大小端的问题!
	   这里默认是大端模式

	   Pay attention to the Big-Endian and Little-Endian!
	   The default mode is Big-Endian here.

	 ****************************/
	u8	AddressH = 0x0f & (Address >> 16);
	u8	AddressM = 0xff & (Address >> 8);
	u8	AddressL = 0xff & Address;

	unsigned int reg;

	regmap_write(lt8911exb->regmap, 0xff, 0xa6);
	regmap_write(lt8911exb->regmap, 0x2b, (0x80 | AddressH));	//CMD
	regmap_write(lt8911exb->regmap, 0x2b, AddressM);	//addr[15:8]
	regmap_write(lt8911exb->regmap, 0x2b, AddressL);	//addr[7:0]
	regmap_write(lt8911exb->regmap, 0x2b, 0x00);	//data lenth
	regmap_write(lt8911exb->regmap, 0x2b, Data);	//data
	regmap_write(lt8911exb->regmap, 0x2c, 0x00);	//start Aux

	mdelay(20);	//more than 10ms
	regmap_read(lt8911exb->regmap, 0x25, &reg);
	if ((reg & 0x0f) == 0x0c) {
		return;
	}
}

unsigned int DpcdRead( struct lt8911exb *lt8911exb, u32 Address )
{
	/***************************
	   注意大小端的问题!
	   这里默认是大端模式

	   Pay attention to the Big-Endian and Little-Endian!
	   The default mode is Big-Endian here.

	 ****************************/

	unsigned int DpcdValue = 0x00;
	unsigned int AddressH = 0x0f & (Address >> 16);
	unsigned int AddressM = 0xff & (Address >> 8);
	unsigned int AddressL = 0xff & Address;
	unsigned int reg;
	unsigned int temp;

	regmap_write(lt8911exb->regmap, 0xff, 0xac);
	regmap_write(lt8911exb->regmap, 0x00, 0x20);	//Soft Link train
	regmap_write(lt8911exb->regmap, 0xff, 0xa6);
	regmap_write(lt8911exb->regmap, 0x2a, 0x01);

	regmap_write(lt8911exb->regmap, 0xff, 0xa6);
	regmap_write(lt8911exb->regmap, 0x2b, (0x90 | AddressH));	//CMD
	regmap_write(lt8911exb->regmap, 0x2b, AddressM);	//addr[15:8]
	regmap_write(lt8911exb->regmap, 0x2b, AddressL);	//addr[7:0]
	regmap_write(lt8911exb->regmap, 0x2b, 0x00);	//data lenth
	regmap_write(lt8911exb->regmap, 0x2c, 0x00);	//start Aux read edid

	mdelay(20);	//more than 10ms
	regmap_read(lt8911exb->regmap, 0x25, &reg);
	if ((reg & 0x0f) == 0x0c) {
		regmap_read(lt8911exb->regmap, 0x39, &temp);
		if (temp == 0x22) {
			//LT8911EXB_IIC_Read_byte( 0x2b );
			//DpcdValue = LT8911EXB_IIC_Read_byte( 0x2b );
			regmap_read(lt8911exb->regmap, 0x2b, &DpcdValue);
			regmap_read(lt8911exb->regmap, 0x2b, &DpcdValue);
		}
	} else {
		regmap_write(lt8911exb->regmap, 0xff, 0x81); // change bank
		regmap_write(lt8911exb->regmap, 0x07, 0xfe);
		regmap_write(lt8911exb->regmap, 0x07, 0xff);
		regmap_write(lt8911exb->regmap, 0x0a, 0xfc);
		regmap_write(lt8911exb->regmap, 0x0a, 0xfe);
	}

	return DpcdValue;
}



void lt8911exb_link_train(struct lt8911exb *lt8911exb)
{
	regmap_write(lt8911exb->regmap, 0xff, 0x81);
	regmap_write(lt8911exb->regmap, 0x06, 0xdf); // rset VID TX
	regmap_write(lt8911exb->regmap, 0x06, 0xff);

	regmap_write(lt8911exb->regmap, 0xff, 0x85);

	//regmap_write(lt8911exb->regmap, 0x17, 0xf0 ); // turn off scramble

	if (ScrambleMode) {
		regmap_write(lt8911exb->regmap, 0xa1, 0x82); // eDP scramble mode;
		regmap_write(lt8911exb->regmap, 0xff, 0xac);
		regmap_write(lt8911exb->regmap, 0x00, 0x20); //Soft Link train
		regmap_write(lt8911exb->regmap, 0xff, 0xa6);
		regmap_write(lt8911exb->regmap, 0x2a, 0x01);

		DpcdWrite(lt8911exb, 0x010a, 0x01);
		mdelay(10);
		DpcdWrite(lt8911exb, 0x0102, 0x00);
		mdelay(10);
		DpcdWrite(lt8911exb, 0x010a, 0x01);

		mdelay(20);
		//*/
	} else {
		regmap_write(lt8911exb->regmap, 0xa1, 0x02); // DP scramble mode;
	}
	/* Aux setup */
	regmap_write(lt8911exb->regmap, 0xff, 0xac);
	regmap_write(lt8911exb->regmap, 0x00, 0x60);     //Soft Link train
	regmap_write(lt8911exb->regmap, 0xff, 0xa6);
	regmap_write(lt8911exb->regmap, 0x2a, 0x00);

	regmap_write(lt8911exb->regmap, 0xff, 0x81);
	regmap_write(lt8911exb->regmap, 0x07, 0xfe);
	regmap_write(lt8911exb->regmap, 0x07, 0xff);
	regmap_write(lt8911exb->regmap, 0x0a, 0xfc);
	regmap_write(lt8911exb->regmap, 0x0a, 0xfe);

	/* link train */

	regmap_write(lt8911exb->regmap, 0xff, 0x85);
	regmap_write(lt8911exb->regmap, 0x1a, eDP_lane);

#ifdef _link_train_enable_
	regmap_write(lt8911exb->regmap, 0xff, 0xac);
	regmap_write(lt8911exb->regmap, 0x00, 0x64);
	regmap_write(lt8911exb->regmap, 0x01, 0x0a);
	regmap_write(lt8911exb->regmap, 0x0c, 0x85);
	regmap_write(lt8911exb->regmap, 0x0c, 0xc5);
#else
	regmap_write(lt8911exb->regmap, 0xff, 0xac);
	regmap_write(lt8911exb->regmap, 0x00, 0x00);
	regmap_write(lt8911exb->regmap, 0x01, 0x0a);
	regmap_write(lt8911exb->regmap, 0x14, 0x80);
	regmap_write(lt8911exb->regmap, 0x14, 0x81);
	mdelay(10);
	regmap_write(lt8911exb->regmap, 0x14, 0x84);
	mdelay(10);
	regmap_write(lt8911exb->regmap, 0x14, 0xc0);
#endif
}

void lt8911exb_reset(struct lt8911exb *lt8911exb)
{
	if (!lt8911exb->reset_gpio) {
		DRM_DEBUG_ATOMIC("no gpio, no reset\n");
		return;
	}

	gpiod_direction_output(lt8911exb->reset_gpio, 1);
	usleep_range(5*1000, 10*1000);
	gpiod_direction_output(lt8911exb->reset_gpio, 0);
	usleep_range(40*1000, 50*1000);
	gpiod_direction_output(lt8911exb->reset_gpio, 1);
	usleep_range(40*1000, 50*1000);
}

void LT8911EX_TxSwingPreSet(struct lt8911exb *lt8911exb)
{
	regmap_write(lt8911exb->regmap, 0xFF, 0x82);
	regmap_write(lt8911exb->regmap, 0x22, Swing_Setting1[Level]);	//lane 0 tap0
	regmap_write(lt8911exb->regmap, 0x23, Swing_Setting2[Level]);
	regmap_write(lt8911exb->regmap, 0x24, 0x80);	//lane 0 tap1
	regmap_write(lt8911exb->regmap, 0x25, 0x00);

#if ( eDP_lane == 2 )
	regmap_write(lt8911exb->regmap, 0x26, Swing_Setting1[Level]);	//lane 1 tap0
	regmap_write(lt8911exb->regmap, 0x27, Swing_Setting2[Level]);
	regmap_write(lt8911exb->regmap, 0x28, 0x80);	//lane 1 tap1
	regmap_write(lt8911exb->regmap, 0x29, 0x00);
#endif
}

void LT8911EXB_LinkTrainResultCheck( struct lt8911exb *lt8911exb)
{
#ifdef _link_train_enable_
	u8	i;
	unsigned int val;

	regmap_write(lt8911exb->regmap, 0xff, 0xac);
	for (i = 0; i < 10; i++) {
		regmap_read(lt8911exb->regmap, 0x82, &val);
		DRM_DEBUG("\r\n0x82: 0x%x", val);
		if (val & 0x20) {
			if ((val & 0x1f) == 0x1e) {
#ifdef _uart_debug_
				DRM_INFO("\r\nedp link train successed: 0x%x", val);
#endif
				return;
			} else {
#ifdef _uart_debug_
				DRM_INFO("\r\nedp link train failed: 0x%x", val);
#endif
				regmap_write(lt8911exb->regmap, 0xff, 0xac);
				regmap_write(lt8911exb->regmap, 0x00, 0x00);
				regmap_write(lt8911exb->regmap, 0x01, 0x0a);
				regmap_write(lt8911exb->regmap, 0x14, 0x80);
				regmap_write(lt8911exb->regmap, 0x14, 0x81);
				mdelay(10);
				regmap_write(lt8911exb->regmap, 0x14, 0x84);
				mdelay(10);
				regmap_write(lt8911exb->regmap, 0x14, 0xc0);
			}

#ifdef _uart_debug_

			regmap_read(lt8911exb->regmap, 0x83, &val);
			//DRM_DEBUG_ATOMIC("\r\nLT8911_LinkTrainResultCheck: panel link rate: 0x%bx",val);
			DRM_INFO( "\r\npanel link rate: 0x%x", val );
			regmap_read(lt8911exb->regmap, 0x84, &val);
			//DRM_DEBUG_ATOMIC("\r\nLT8911_LinkTrainResultCheck: panel link count: 0x%bx",val);
			DRM_INFO( "\r\npanel link count:0x%x ", val);
#endif
			mdelay(10); // return;
		} else {
			//DRM_DEBUG_ATOMIC("\r\nLT8911_LinkTrainResultCheck: link trian on going...");
			mdelay(20);
		}
	}
#endif
}


void LT8911EX_link_train_result(struct lt8911exb *lt8911exb)
{
	u8 i;
	unsigned int reg;
	unsigned int temp;
	regmap_write(lt8911exb->regmap, 0xff, 0xac);
	for (i = 0; i < 10; i++) {
		regmap_read(lt8911exb->regmap, 0x82, &reg);
		DRM_DEBUG( "\r\n0x82 = 0x%x", reg );
		if (reg & 0x20) {
			if((reg & 0x1f) == 0x1e) {
				DRM_DEBUG("\r\nLink train success, 0x82 = 0x%x", reg);
			} else {
				DRM_DEBUG("\r\nLink train fail, 0x82 = 0x%x", reg);
			}

			regmap_read(lt8911exb->regmap, 0x83, &temp);
			DRM_DEBUG("\r\npanel link rate: 0x%x", temp);
			regmap_read(lt8911exb->regmap, 0x84, &temp);
			DRM_DEBUG("\r\npanel link count: 0x%x", temp);
			return;
		} else {
			DRM_DEBUG("\r\nlink trian on going...");
		}
		mdelay(10);
	}
}

void PCR_Status(struct lt8911exb *lt8911exb)	// for debug
{
#ifdef _uart_debug_
	unsigned int reg;

	regmap_write(lt8911exb->regmap, 0xff, 0xd0);
	//reg = LT8911EXB_IIC_Read_byte( 0x87 );
	regmap_read(lt8911exb->regmap, 0x87, &reg);

	DRM_INFO("\r\nReg0xD087 =	");
	DRM_INFO(" 0x%x ", reg);
	DRM_INFO("\r\n ");
	if (reg & 0x10) {
		DRM_INFO( "\r\nPCR Clock stable" );
	} else {
		DRM_INFO( "\r\nPCR Clock unstable" );
	}
	DRM_INFO("\r\n ");
#endif
}

static int lt8911exb_chip_id(struct lt8911exb *lt8911exb)
{
    u8 retry = 0;
    unsigned int chip_id_h = 0, chip_id_m = 0, chip_id_l = 0;
    int ret = -EAGAIN;

    while(retry++ < 3) {
        ret = regmap_write(lt8911exb->regmap, 0xff, 0x81);

        if(ret < 0) {
            dev_err(lt8911exb->dev, "LT8911EXB i2c test write addr:0xff failed\n");
            continue;
        }

        ret = regmap_write(lt8911exb->regmap, 0x08, 0x7f);

        if(ret < 0) {
            dev_err(lt8911exb->dev, "LT8911EXB i2c test write addr:0x08 failed\n");
            continue;
        }

        regmap_read(lt8911exb->regmap, 0x00, &chip_id_l);
        regmap_read(lt8911exb->regmap, 0x01, &chip_id_m);
        regmap_read(lt8911exb->regmap, 0x02, &chip_id_h);
        // LT8911EXB i2c test success chipid: 0xe0517
        DRM_DEBUG("LT8911EXB i2c test success chipid: 0x%x%x%x\n", chip_id_h, chip_id_m, chip_id_l);

        ret = 0;
        break;
    }

    return ret;
}

static int lt8911exb_panel_enable(struct drm_panel *panel)
{
	struct lt8911exb *lt8911exb = panel_to_lt8911exb(panel);

	DRM_INFO("%s()\n", __func__);
	if (!IS_ERR_OR_NULL(lt8911exb->enable_gpio)) {
		gpiod_direction_output(lt8911exb->enable_gpio, 1);
	}
	schedule_delayed_work(&lt8911exb->init_work,
				msecs_to_jiffies(100));
	lt8911exb->init_work_pending = true;

	return 0;
}

static int lt8911exb_panel_disable(struct drm_panel *panel)
{
	struct lt8911exb *lt8911exb = panel_to_lt8911exb(panel);

	DRM_INFO("%s()\n", __func__);

	gpiod_direction_output(lt8911exb->bl_gpio, 0);
	if (!IS_ERR_OR_NULL(lt8911exb->enable_gpio)) {
		gpiod_direction_output(lt8911exb->enable_gpio, 0);
	}
	usleep_range(50*1000, 100*1000); //100ms

	gpiod_direction_output(lt8911exb->standby_gpio, 0);

	if (lt8911exb->init_work_pending) {
		cancel_delayed_work_sync(&lt8911exb->init_work);
		lt8911exb->init_work_pending = false;
	}

	return 0;
}

static int lt8911exb_panel_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	unsigned int i, num = 0;
	static const u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;

	DRM_DEBUG("%s()\n", __func__);

	for (i = 0; i < ARRAY_SIZE(lt8911exb_panel_modes); i++) {
		const struct drm_display_mode *m = &lt8911exb_panel_modes[i];
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(connector->dev, m);
		if (!mode) {
			dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
				m->hdisplay, m->vdisplay,
				drm_mode_vrefresh(m));
			continue;
		}

		mode->type |= DRM_MODE_TYPE_DRIVER;

		if (i == 0)
			mode->type |= DRM_MODE_TYPE_PREFERRED;

		drm_mode_set_name(mode);
		drm_mode_probed_add(connector, mode);
		num++;
	}

	connector->display_info.bpc = 8;
	// 1920x1080
	#if MIPI_DSI_1920x1080
	connector->display_info.width_mm = 309;
	connector->display_info.height_mm = 174;
	#endif

	// 1920x1200
	#if MIPI_DSI_1920x1200
	connector->display_info.width_mm = 301;
	connector->display_info.height_mm = 188;
	#endif
	drm_display_info_set_bus_formats(&connector->display_info,
				&bus_format, 1);
	return num;
}


static const struct drm_panel_funcs lt8911exb_panel_funcs = {
	.disable = lt8911exb_panel_disable,
	.enable = lt8911exb_panel_enable,
	.get_modes = lt8911exb_panel_get_modes,
};

static void init_work_func(struct work_struct *work)
{
	struct lt8911exb *lt8911exb = container_of(work, struct lt8911exb,
						init_work.work);

	DRM_DEBUG(" %s() \n", __func__);

	gpiod_direction_output(lt8911exb->standby_gpio, 1);
	// usleep_range(5*1000,10*1000); //10ms

	lt8911exb_reset(lt8911exb);
	lt8911exb_chip_id(lt8911exb);

	lt8911exb_edp_video_cfg(lt8911exb);
	lt8911exb_setup(lt8911exb);
	LT8911EX_TxSwingPreSet(lt8911exb);

	lt8911exb_read_edid(lt8911exb);

	ScrambleMode = 0;
	lt8911exb_link_train(lt8911exb);
	LT8911EXB_LinkTrainResultCheck(lt8911exb);
	LT8911EX_link_train_result(lt8911exb);
	lt8911exb_video_check(lt8911exb); //just for Check MIPI Input

	DRM_DEBUG("\r\nDpcdRead(0x0202) = 0x%x\r\n",DpcdRead(lt8911exb, 0x0202));
	mdelay(80);
	PCR_Status(lt8911exb);

	gpiod_direction_output(lt8911exb->bl_gpio, 1);
}
static int lt8911exb_probe(struct i2c_client *client)
{
	struct lt8911exb *lt8911exb;
	struct device *dev = &client->dev;
	int ret;

	struct device_node *endpoint, *dsi_host_node;
	struct mipi_dsi_host *host;

	struct device_node *lcd_node;
	int rc;
	const char *str;
	char lcd_path[60];
	const char *lcd_name;

	struct mipi_dsi_device_info info = {
		.type = IT8911_DSI_DRIVER_NAME,
		.channel = 0,
		.node = NULL,
	};

	DRM_DEBUG("%s()\n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "Failed check I2C functionality");
		return -ENODEV;
	}

	lt8911exb = devm_kzalloc(&client->dev, sizeof(*lt8911exb), GFP_KERNEL);
	if (!lt8911exb)
		return -ENOMEM;

	lt8911exb->dev = &client->dev;
	lt8911exb->client = client;

	//regmap i2c , maybe useless
	lt8911exb->regmap = devm_regmap_init_i2c(client, &lt8911exb_regmap_config);
	if (IS_ERR(lt8911exb->regmap)) {
		dev_err(lt8911exb->dev, "regmap i2c init failed\n");
		return PTR_ERR(lt8911exb->regmap);
	}

	lt8911exb->enable_gpio = devm_gpiod_get_optional(dev, "enable",
						GPIOD_IN);
	if (IS_ERR_OR_NULL(lt8911exb->enable_gpio)) {
		DRM_DEBUG("%s() failed get enable gpio\n", __func__);
		// return PTR_ERR(lt8911exb->enable_gpio);
	}

	lt8911exb->standby_gpio = devm_gpiod_get_optional(dev, "standby",
						GPIOD_IN);
	if (IS_ERR(lt8911exb->standby_gpio)) {
		dev_err(&client->dev, "Failed get standby gpio\n");
		return PTR_ERR(lt8911exb->standby_gpio);
	}

	lt8911exb->bl_gpio = devm_gpiod_get_optional(dev, "bl",
						GPIOD_IN);
	if (IS_ERR(lt8911exb->bl_gpio)) {
		dev_err(&client->dev, "Failed get bl gpio\n");
		return PTR_ERR(lt8911exb->bl_gpio);
	}
	gpiod_direction_output(lt8911exb->bl_gpio, 0);
	if (!IS_ERR_OR_NULL(lt8911exb->enable_gpio)) {
		gpiod_direction_output(lt8911exb->enable_gpio, 0);
	}
	usleep_range(50*1000, 100*1000); //100ms

	//disable firstly
	gpiod_direction_output(lt8911exb->standby_gpio, 0);
	usleep_range(50*1000, 100*1000); //100ms
	gpiod_direction_output(lt8911exb->standby_gpio, 1);
	usleep_range(50*1000, 100*1000); //100ms

	lt8911exb->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						GPIOD_IN);
	if (IS_ERR(lt8911exb->reset_gpio)) {
		dev_err(&client->dev, "Failed get reset gpio\n");
		return PTR_ERR(lt8911exb->reset_gpio);
	}

	lt8911exb_reset(lt8911exb);

	i2c_set_clientdata(client, lt8911exb);

	//check i2c communicate
	ret = lt8911exb_chip_id(lt8911exb);
	if (ret < 0) {
		dev_err(&client->dev, "Failed communicate with IC use I2C\n");
		return ret;
	}

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint)
		return -ENODEV;

	dsi_host_node = of_graph_get_remote_port_parent(endpoint);
	if (!dsi_host_node)
		goto error;

	rc = of_property_read_string(dsi_host_node, "force-attached", &str);
	if (!rc)
		lcd_name = str;

	sprintf(lcd_path, "/lcds/%s", lcd_name);
	lcd_node = of_find_node_by_path(lcd_path);
	if (!lcd_node) {
		DRM_ERROR("%pOF: could not find %s node\n", dsi_host_node, lcd_name);
		of_node_put(endpoint);
		return -ENODEV;
	}

	DRM_INFO("%pOF: find %s node\n", dsi_host_node, lcd_name);

	host = of_find_mipi_dsi_host_by_node(dsi_host_node);
	of_node_put(dsi_host_node);
	if (!host) {
		of_node_put(endpoint);
		return -EPROBE_DEFER;
	}

	drm_panel_init(&lt8911exb->base, dev, &lt8911exb_panel_funcs,
			DRM_MODE_CONNECTOR_DSI);

	/* This appears last, as it's what will unblock the DSI host
	 * driver's component bind function.
	 */
	drm_panel_add(&lt8911exb->base);

	lt8911exb->base.dev->of_node = lcd_node;
	info.node = of_node_get(of_graph_get_remote_port(endpoint));
	if (!info.node)
		goto error;
	of_node_put(endpoint);

	lt8911exb->dsi = mipi_dsi_device_register_full(host, &info);
	if (IS_ERR(lt8911exb->dsi)) {
		dev_err(dev, "DSI device registration failed: %ld\n",
			PTR_ERR(lt8911exb->dsi));
		return PTR_ERR(lt8911exb->dsi);
	}

	INIT_DELAYED_WORK(&lt8911exb->init_work, init_work_func);

	return 0;
error:
	of_node_put(endpoint);
	return -ENODEV;
}

static void lt8911exb_remove(struct i2c_client *client)
{
	struct lt8911exb *lt8911exb = i2c_get_clientdata(client);

	DRM_DEBUG("%s()\n", __func__);

	mipi_dsi_detach(lt8911exb->dsi);

	drm_panel_remove(&lt8911exb->base);

	mipi_dsi_device_unregister(lt8911exb->dsi);
}

static struct i2c_device_id lt8911exb_id[] = {
	{ "lontium,lt8911exb", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, lt8911exb_id);

static const struct of_device_id lt8911exb_match_table[] = {
	{ .compatible = "lontium,lt8911exb" },
	{ }
};
MODULE_DEVICE_TABLE(of, lt8911exb_match_table);

static struct i2c_driver lt8911exb_driver = {
	.driver = {
		.name = "lt8911exb",
		.of_match_table = lt8911exb_match_table,
	},
	.probe = lt8911exb_probe,
	.remove = lt8911exb_remove,
	.id_table = lt8911exb_id,
};

static int lt8911exb_dsi_probe(struct mipi_dsi_device *dsi)
{
	int ret;

	DRM_DEBUG("%s()\n", __func__);

	dsi->lanes = 4;
	dsi->mode_flags = (MIPI_DSI_MODE_VIDEO |
				MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
				MIPI_DSI_MODE_LPM);
	dsi->format = MIPI_DSI_FMT_RGB888;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(&dsi->dev, "failed to attach dsi to host\n");
		mipi_dsi_device_unregister(dsi);
	}

	return ret;
}

static const struct of_device_id ky_edp_of_match[] = {
	{ .compatible = "ky,edp-panel" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ky_edp_of_match);

static struct mipi_dsi_driver lt8911exb_dsi_driver = {
	.probe = lt8911exb_dsi_probe,
	.driver = {
		.name = IT8911_DSI_DRIVER_NAME,
		.of_match_table = ky_edp_of_match,
	},
};


static int __init init_lt8911exb(void)
{
	int err;

	DRM_DEBUG("%s()\n", __func__);
	mipi_dsi_driver_register(&lt8911exb_dsi_driver);
	err = i2c_add_driver(&lt8911exb_driver);

	return err;
}

module_init(init_lt8911exb);

static void __exit exit_lt8911exb(void)
{
	DRM_DEBUG("%s()\n", __func__);
	i2c_del_driver(&lt8911exb_driver);
	mipi_dsi_driver_unregister(&lt8911exb_dsi_driver);
}
module_exit(exit_lt8911exb);

MODULE_DESCRIPTION("LT8911EXB_MIPI to eDP Reg Setting driver");
MODULE_LICENSE("GPL v2");
