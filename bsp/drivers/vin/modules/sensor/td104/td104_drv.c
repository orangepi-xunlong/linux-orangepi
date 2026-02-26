
/*
 * A V4L2 driver for TD100.
 *
 * Copyright (c) 2020 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Lihuiyu<lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
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

#include "../camera.h"
#include "../sensor_helper.h"
#include "td104_drv.h"
#include "../../../utility/vin_log.h"

#define SENSOR_NAME	"td100c"

#define is_cvbs(fmt)	((fmt == CVBS_NTSC) || (fmt == CVBS_PAL))
#define is_ypbpr(fmt)	((fmt == YCBCR_480I) ||	\
			(fmt == YCBCR_576I) ||	\
			(fmt == YCBCR_480P) ||	\
			(fmt == YCBCR_576P))

unsigned int sensor_type;

int tvd_base_addr[TVD_CH_MAX] = {
	0x8000,
	0x9000,
	0xa000,
	0xb000,
};

void sensor_type_set(unsigned int type)
{
	if (type)
		sensor_type =  type;
}

unsigned int sensor_type_get(unsigned int type)
{
	return sensor_type;
}

unsigned int sensor_support_3d_filter(void)
{
	return sensor_type == _3D_FILTRE_ID;
}

#ifdef CCI_TO_AUDIO
int td100_cci_read_a16_d8(unsigned short addr, unsigned char *value)
{
	if (td100_sd)
		return cci_read_a16_d8(td100_sd, addr, value);
	pr_err("[%s] td100 sd is null!\n", __func__);
	return -EINVAL;

}
EXPORT_SYMBOL_GPL(td100_cci_read_a16_d8);

int td100_cci_write_a16_d8(unsigned short addr, unsigned char value)
{
	if (td100_sd)
		return cci_write_a16_d8(td100_sd, addr, value);
	sensor_err("[%s] td100 sd is null!\n", __func__);
	return -EINVAL;

}
EXPORT_SYMBOL_GPL(td100_cci_write_a16_d8);

#endif

int td100_cci_read_a16_d16(unsigned short addr, unsigned short *value)
{
	if (td100_sd)
		return cci_read_a16_d16(td100_sd, addr, value);
	sensor_err("[%s] td100 sd is null!\n", __func__);
	return -EINVAL;

}

int td100_cci_write_a16_d16(unsigned short addr, unsigned short value)
{
	if (td100_sd)
		return cci_write_a16_d16(td100_sd, addr, value);
	sensor_err("[%s] td100 sd is null!\n", __func__);
	return -EINVAL;

}

/*flip high and low*/
static int tvd_reg_w_a16_d8(unsigned short addr, unsigned char value)
{

	return td100_cci_write_a16_d8(((addr << 8) | (addr >> 8)) & (0xffff), value);

}

static int tvd_reg_r_a16_d8(unsigned short addr, unsigned char *value)
{
	return td100_cci_read_a16_d8(((addr << 8) | (addr >> 8)) & (0xffff), value);
}

int tvd_reg_r_a16_d16(unsigned short addr, unsigned short *value)
{
	unsigned short tmp = 0;
	int ret;

	ret = td100_cci_read_a16_d16(((addr << 8) | (addr >> 8)) & (0xffff), &tmp);
	*value = ((tmp << 8) | (tmp >> 8)) & (0xffff);
	return ret;

}

static int tvd_reg_w_a16_d16(unsigned short addr, unsigned short value)
{
	unsigned short val;

	td100_cci_write_a16_d16(((addr << 8) | (addr >> 8)) & (0xffff),
					((value << 8) | (value >> 8)) & (0xffff));
	// tvd_reg_r_a16_d16(addr, &val);
	// printk("addr :0x%x = 0x%x !\r\n", addr, val);
	return 0;
}

int td100_agc_control(int ch, enum AGC_STATE agc_state, int w_value)
{
	int base_addr;
	unsigned short value0, value1, value2;
	static enum AGC_STATE state = AGC_OPEN;
	static int old_value;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];

	if (state != agc_state || w_value != old_value) {
		if (agc_state == AGC_CLOSE) {
			tvd_reg_w_a16_d16((0x008 | base_addr), 0xd000);
			tvd_reg_w_a16_d16((0x102 | base_addr), 0x0030);
			tvd_reg_w_a16_d16((0x101 | base_addr), w_value);
			printk("agc_state: AGC_CLOSE\r\n");
			tvd_reg_r_a16_d16((0x008 | base_addr), &value0);
			tvd_reg_r_a16_d16((0x102 | base_addr), &value1);
			tvd_reg_r_a16_d16((0x101 | base_addr), &value2);
			printk("reg is: en - 0x%x , auto - 0x%x , value - 0x%x !\r\n", value0, value1, value2);
		} else {
			tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
			tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
			printk("agc_state: AGC_OPEN\r\n");
			tvd_reg_r_a16_d16((0x008 | base_addr), &value0);
			tvd_reg_r_a16_d16((0x102 | base_addr), &value1);
			// printk("reg is: en - 0x%x , auto - 0x%x , value - 0x%x !\r\n", value0, value1);
		}

		state = agc_state;
		old_value = w_value;
	} else
		printk("no change!\r\n");

	return 0;

}

static int init_td100_top(void)
{
#if 0
	/*global reset will affect audio*/
	tvd_reg_w_a16_d16(0x0002, 0x0001);	/*global reset*/
	usleep_range(1000, 1200);         	/*global reset*/
	tvd_reg_w_a16_d16(0x0002, 0x0001);	/*release global reset*/
#endif
	tvd_reg_w_a16_d16(0x0004, 0x8031);	/*release global reset*/
	tvd_reg_w_a16_d16(0x0008, 0x0001);	/*enable irq,enable irq pin*/
	tvd_reg_w_a16_d16(0x000A, 0x00cc);	/*27MHz clock input*/
	tvd_reg_w_a16_d16(0x000C, 0xcc00);	/*enable digital LDO and PSRAM LDO*/
	tvd_reg_w_a16_d16(0x000E, 0x014a);	/* 27MHz/(0+1)*(3+1)*/
	tvd_reg_w_a16_d16(0x0010, 0xcc00);	/*video pll1 control*/
	tvd_reg_w_a16_d16(0x0012, 0x014a);	/* 27MHz/(0+1)*(5+1)*/
	tvd_reg_w_a16_d16(0x0018, 0x0006);	/*psram pll1 control*/
	tvd_reg_w_a16_d16(0x0018, 0x0007);	   /*tvin reset*/
	tvd_reg_w_a16_d16(0x001A, 0x0593);	/*tvd reset 1ms*/
	tvd_reg_w_a16_d16(0x0040, 0x0302);	/*tvin clk gating reset,eanble psram clk*/
	return tvd_reg_w_a16_d16(0x0042, 0x0502);	/*enable ccir data and clock io*/

}

static int init_tvd_top(unsigned char _3d_ch)
{

#ifdef SUPPORT_3D_FILTER
	if ((_3d_ch >= TVD_CH_MAX) && sensor_support_3d_filter())
		tvd_reg_w_a16_d16(0x7008, (_3d_ch << 12) & 0x0300);
	else
		tvd_reg_w_a16_d16(0x7008, 0x0000);
#else
	tvd_reg_w_a16_d16(0x7008, 0x0000);
#endif
	tvd_reg_w_a16_d16(0x7008, 0x0000);
	tvd_reg_w_a16_d16(0x700a, 0x0000);
	tvd_reg_w_a16_d16(0x700c, 0x1000);
	tvd_reg_w_a16_d16(0x700e, 0x0000);
	tvd_reg_w_a16_d16(0x7010, 0x0000);
	tvd_reg_w_a16_d16(0x7012, 0x0000);
	tvd_reg_w_a16_d16(0x7014, 0x0000);
	tvd_reg_w_a16_d16(0x7016, 0x0020);
	tvd_reg_w_a16_d16(0x7018, 0x0000);
	return tvd_reg_w_a16_d16(0x701a, 0x0020);
}

static int init_tvd_top_adc(unsigned char mask)
{
	int ret = 0;

	if ((mask >> 0) & (0x1)) {
		tvd_reg_w_a16_d16(0x702c, 0xaaaa);
		tvd_reg_w_a16_d16(0x702e, 0x1007);
		tvd_reg_w_a16_d16(0x7024, 0x0010);
		tvd_reg_w_a16_d16(0x7026, 0x0000);
		tvd_reg_w_a16_d16(0x7028, 0x0003);
		ret = tvd_reg_w_a16_d16(0x702a, 0x0000);
	}
	if ((mask >> 1) & (0x1)) {
		tvd_reg_w_a16_d16(0x704c, 0xaaaa);
		tvd_reg_w_a16_d16(0x704e, 0x1007);
		tvd_reg_w_a16_d16(0x7044, 0x0010);
		tvd_reg_w_a16_d16(0x7046, 0x0000);
		tvd_reg_w_a16_d16(0x7048, 0x0003);
		ret = tvd_reg_w_a16_d16(0x704a, 0x0000);
	}
	if ((mask >> 2) & (0x1)) {
		tvd_reg_w_a16_d16(0x706c, 0xaaaa);
		tvd_reg_w_a16_d16(0x706e, 0x1007);
		tvd_reg_w_a16_d16(0x7064, 0x0010);
		tvd_reg_w_a16_d16(0x7066, 0x0000);
		tvd_reg_w_a16_d16(0x7068, 0x0003);
		ret = tvd_reg_w_a16_d16(0x706a, 0x0000);
	}
	if ((mask >> 3) & (0x1)) {
		tvd_reg_w_a16_d16(0x708c, 0xaaaa);
		tvd_reg_w_a16_d16(0x708e, 0x1007);
		tvd_reg_w_a16_d16(0x7084, 0x0010);
		tvd_reg_w_a16_d16(0x7086, 0x0000);
		tvd_reg_w_a16_d16(0x7088, 0x0003);
		ret = tvd_reg_w_a16_d16(0x708a, 0x0000);
	}
	return ret;
}
static int init_tvd_to_ntsc(int ch, int _3d_filter_en, int tvd_en)
{

	int base_addr;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];
	if (tvd_en)
		tvd_reg_w_a16_d16((0x000 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa001);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x010 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x012 | base_addr), 0x2000);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x018 | base_addr), 0x502d);
	tvd_reg_w_a16_d16((0x01a | base_addr), 0x0fe9);
	tvd_reg_w_a16_d16((0x01c | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x01e | base_addr), 0x3e3e);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xe103);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa001);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x42d6);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0220);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5082);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0230);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5082);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4222);
	tvd_reg_w_a16_d16((0x028 | base_addr), 0x0070);
	tvd_reg_w_a16_d16((0x02a | base_addr), 0x000e);
	tvd_reg_w_a16_d16((0x030 | base_addr), 0x3201);
	tvd_reg_w_a16_d16((0x032 | base_addr), 0x0446);
	tvd_reg_w_a16_d16((0x034 | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x036 | base_addr), 0x21f0);
#ifdef SUPPORT_3D_FILTER
	if (sensor_support_3d_filter() && _3d_filter_en)
		tvd_reg_w_a16_d16((0x040 | base_addr), 0x4201);
	else
		tvd_reg_w_a16_d16((0x040 | base_addr), 0x4201);
#else
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x4209);
#endif
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x4209);
	tvd_reg_w_a16_d16((0x042 | base_addr), 0x0140);
	tvd_reg_w_a16_d16((0x044 | base_addr), 0x40af);
	tvd_reg_w_a16_d16((0x046 | base_addr), 0x0b14);
	tvd_reg_w_a16_d16((0x050 | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x052 | base_addr), 0x1420);
	tvd_reg_w_a16_d16((0x054 | base_addr), 0x0780);
	tvd_reg_w_a16_d16((0x056 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x058 | base_addr), 0x0811);
	tvd_reg_w_a16_d16((0x05a | base_addr), 0x15b8);
	tvd_reg_w_a16_d16((0x138 | base_addr), 0x0c81);
	tvd_reg_w_a16_d16((0x13a | base_addr), 0x0040);
	tvd_reg_w_a16_d16((0x100 | base_addr), 0x4001);
	tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
	tvd_reg_w_a16_d16((0x104 | base_addr), 0x0450);
	tvd_reg_w_a16_d16((0x106 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x108 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x10a | base_addr), 0x0320);
	tvd_reg_w_a16_d16((0x10c | base_addr), 0x1819);
	tvd_reg_w_a16_d16((0x10e | base_addr), 0xcc9c);
	tvd_reg_w_a16_d16((0x110 | base_addr), 0x1dc0);
	tvd_reg_w_a16_d16((0x112 | base_addr), 0x06e8);
	tvd_reg_w_a16_d16((0x114 | base_addr), 0x09bd);
	tvd_reg_w_a16_d16((0x116 | base_addr), 0xb9a0);
	tvd_reg_w_a16_d16((0x118 | base_addr), 0xc829);
	tvd_reg_w_a16_d16((0x11a | base_addr), 0x007e);
	tvd_reg_w_a16_d16((0x11c | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x11e | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x120 | base_addr), 0x9446);
	tvd_reg_w_a16_d16((0x122 | base_addr), 0x21f6);
	tvd_reg_w_a16_d16((0x124 | base_addr), 0xefe3);
	tvd_reg_w_a16_d16((0x126 | base_addr), 0x21e6);
	tvd_reg_w_a16_d16((0x128 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x12a | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x12c | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x12e | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x130 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x132 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x134 | base_addr), 0xb08c);
	tvd_reg_w_a16_d16((0x136 | base_addr), 0x8021);
	if (tvd_en)
		tvd_reg_w_a16_d16((0x000 | base_addr), 0x0001);
	return tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
}

static int init_tvd_to_pal(int ch, int _3d_filter_en, int tvd_en)
{

	int base_addr;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];
	if (tvd_en)
		tvd_reg_w_a16_d16((0x000 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa001);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x010 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x012 | base_addr), 0x2000);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x018 | base_addr), 0x502d);
	tvd_reg_w_a16_d16((0x01a | base_addr), 0x0fe9);
	tvd_reg_w_a16_d16((0x01c | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x01e | base_addr), 0x3e3e);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xdc03);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0x8601);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x0682);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0001);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0221);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5089);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x02a1);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x00c1);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x508a);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4322);
	tvd_reg_w_a16_d16((0x028 | base_addr), 0x0070);
	tvd_reg_w_a16_d16((0x02a | base_addr), 0x000e);
	tvd_reg_w_a16_d16((0x030 | base_addr), 0x3203);
	tvd_reg_w_a16_d16((0x032 | base_addr), 0x0446);
	tvd_reg_w_a16_d16((0x034 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x036 | base_addr), 0x2a09);
#ifdef SUPPORT_3D_FILTER
	if (sensor_support_3d_filter() && _3d_filter_en)
		tvd_reg_w_a16_d16((0x040 | base_addr), 0x4264);
	else
		tvd_reg_w_a16_d16((0x040 | base_addr), 0x426c);
#else
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x426c);
#endif
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x426c);
	tvd_reg_w_a16_d16((0x042 | base_addr), 0x0140);
	tvd_reg_w_a16_d16((0x044 | base_addr), 0x40af);
	tvd_reg_w_a16_d16((0x046 | base_addr), 0x0b14);
	tvd_reg_w_a16_d16((0x050 | base_addr), 0x7800);
	tvd_reg_w_a16_d16((0x052 | base_addr), 0x141c);
	tvd_reg_w_a16_d16((0x054 | base_addr), 0x0780);
	tvd_reg_w_a16_d16((0x056 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x058 | base_addr), 0x0807);
	tvd_reg_w_a16_d16((0x05a | base_addr), 0x05b1);
	tvd_reg_w_a16_d16((0x138 | base_addr), 0x0c80);
	tvd_reg_w_a16_d16((0x13a | base_addr), 0x001e);
	tvd_reg_w_a16_d16((0x100 | base_addr), 0x4000);
	tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
	tvd_reg_w_a16_d16((0x104 | base_addr), 0x0450);
	tvd_reg_w_a16_d16((0x106 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x108 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x10a | base_addr), 0x0320);
	tvd_reg_w_a16_d16((0x10c | base_addr), 0x1819);
	tvd_reg_w_a16_d16((0x10e | base_addr), 0xcc9c);
	tvd_reg_w_a16_d16((0x110 | base_addr), 0x1dc0);
	tvd_reg_w_a16_d16((0x112 | base_addr), 0x8ae8);
	tvd_reg_w_a16_d16((0x114 | base_addr), 0x09bd);
	tvd_reg_w_a16_d16((0x116 | base_addr), 0xb9a0);
	tvd_reg_w_a16_d16((0x118 | base_addr), 0xc829);
	tvd_reg_w_a16_d16((0x11a | base_addr), 0x005e);
	tvd_reg_w_a16_d16((0x11c | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x11e | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x120 | base_addr), 0x9446);
	tvd_reg_w_a16_d16((0x122 | base_addr), 0x21f6);
	tvd_reg_w_a16_d16((0x124 | base_addr), 0xefe3);
	tvd_reg_w_a16_d16((0x126 | base_addr), 0x21e6);
	tvd_reg_w_a16_d16((0x128 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x12a | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x12c | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x12e | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x130 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x132 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x134 | base_addr), 0xb08c);
	tvd_reg_w_a16_d16((0x136 | base_addr), 0x8021);
	if (tvd_en)
		tvd_reg_w_a16_d16((0x000 | base_addr), 0x0001);
	return tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
}

static int init_tvd_to_480i(int ch, int tvd_en)
{
	int base_addr;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];

	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa000);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x010 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x012 | base_addr), 0x2000);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x018 | base_addr), 0x502d);
	tvd_reg_w_a16_d16((0x01a | base_addr), 0x0fe9);
	tvd_reg_w_a16_d16((0x01c | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x01e | base_addr), 0x3e3e);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0021);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xdc03);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0x8601);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x0682);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0220);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5069);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0200);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5066);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x028 | base_addr), 0x0070);
	tvd_reg_w_a16_d16((0x02a | base_addr), 0x000e);
	tvd_reg_w_a16_d16((0x030 | base_addr), 0x3201);
	tvd_reg_w_a16_d16((0x032 | base_addr), 0x0446);
	tvd_reg_w_a16_d16((0x034 | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x036 | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x4209);
	tvd_reg_w_a16_d16((0x042 | base_addr), 0x0180);
	tvd_reg_w_a16_d16((0x044 | base_addr), 0x43fa);
	tvd_reg_w_a16_d16((0x046 | base_addr), 0x0b15);
	tvd_reg_w_a16_d16((0x050 | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x052 | base_addr), 0x1420);
	tvd_reg_w_a16_d16((0x054 | base_addr), 0x0680);
	tvd_reg_w_a16_d16((0x056 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x058 | base_addr), 0x073a);
	tvd_reg_w_a16_d16((0x05a | base_addr), 0x173a);
	tvd_reg_w_a16_d16((0x138 | base_addr), 0x0c81);
	tvd_reg_w_a16_d16((0x13a | base_addr), 0x0040);
	tvd_reg_w_a16_d16((0x100 | base_addr), 0x4001);
	tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
	tvd_reg_w_a16_d16((0x104 | base_addr), 0x0450);
	tvd_reg_w_a16_d16((0x106 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x108 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x10a | base_addr), 0x0320);
	tvd_reg_w_a16_d16((0x10c | base_addr), 0x1819);
	tvd_reg_w_a16_d16((0x10e | base_addr), 0xcc9c);
	tvd_reg_w_a16_d16((0x110 | base_addr), 0x1dc0);
	tvd_reg_w_a16_d16((0x112 | base_addr), 0x86e8);
	tvd_reg_w_a16_d16((0x114 | base_addr), 0x09bd);
	tvd_reg_w_a16_d16((0x116 | base_addr), 0xb9a0);
	tvd_reg_w_a16_d16((0x118 | base_addr), 0xc829);
	tvd_reg_w_a16_d16((0x11a | base_addr), 0x007e);
	tvd_reg_w_a16_d16((0x11c | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x11e | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x120 | base_addr), 0x9446);
	tvd_reg_w_a16_d16((0x122 | base_addr), 0x21f6);
	tvd_reg_w_a16_d16((0x124 | base_addr), 0xefe3);
	tvd_reg_w_a16_d16((0x126 | base_addr), 0x21e6);
	tvd_reg_w_a16_d16((0x128 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x12a | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x12c | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x12e | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x130 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x132 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x134 | base_addr), 0xb08c);
	tvd_reg_w_a16_d16((0x136 | base_addr), 0x8021);
	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0001);
	return tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
}

static int init_tvd_to_576i(int ch, int tvd_en)
{
	int base_addr;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];

	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa000);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x010 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x012 | base_addr), 0x2000);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x018 | base_addr), 0x502d);
	tvd_reg_w_a16_d16((0x01a | base_addr), 0x0fe9);
	tvd_reg_w_a16_d16((0x01c | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x01e | base_addr), 0x3e3e);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0021);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xdc03);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0x8601);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x0682);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0001);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0221);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5072);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x02a1);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x00c1);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5072);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x028 | base_addr), 0x0070);
	tvd_reg_w_a16_d16((0x02a | base_addr), 0x000e);
	tvd_reg_w_a16_d16((0x030 | base_addr), 0x3203);
	tvd_reg_w_a16_d16((0x032 | base_addr), 0x0446);
	tvd_reg_w_a16_d16((0x034 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x036 | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x420c);
	tvd_reg_w_a16_d16((0x042 | base_addr), 0x0140);
	tvd_reg_w_a16_d16((0x044 | base_addr), 0x43fa);
	tvd_reg_w_a16_d16((0x046 | base_addr), 0x0b15);
	tvd_reg_w_a16_d16((0x050 | base_addr), 0x7800);
	tvd_reg_w_a16_d16((0x052 | base_addr), 0x141b);
	tvd_reg_w_a16_d16((0x054 | base_addr), 0x0694);
	tvd_reg_w_a16_d16((0x056 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x058 | base_addr), 0x05b9);
	tvd_reg_w_a16_d16((0x05a | base_addr), 0x15b9);
	tvd_reg_w_a16_d16((0x138 | base_addr), 0x0c81);
	tvd_reg_w_a16_d16((0x13a | base_addr), 0x0040);
	tvd_reg_w_a16_d16((0x100 | base_addr), 0x4001);
	tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
	tvd_reg_w_a16_d16((0x104 | base_addr), 0x0450);
	tvd_reg_w_a16_d16((0x106 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x108 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x10a | base_addr), 0x0320);
	tvd_reg_w_a16_d16((0x10c | base_addr), 0x1819);
	tvd_reg_w_a16_d16((0x10e | base_addr), 0xcc9c);
	tvd_reg_w_a16_d16((0x110 | base_addr), 0x1dc0);
	tvd_reg_w_a16_d16((0x112 | base_addr), 0x86e8);
	tvd_reg_w_a16_d16((0x114 | base_addr), 0x09bd);
	tvd_reg_w_a16_d16((0x116 | base_addr), 0xb9a0);
	tvd_reg_w_a16_d16((0x118 | base_addr), 0xc829);
	tvd_reg_w_a16_d16((0x11a | base_addr), 0x007e);
	tvd_reg_w_a16_d16((0x11c | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x11e | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x120 | base_addr), 0x9446);
	tvd_reg_w_a16_d16((0x122 | base_addr), 0x21f6);
	tvd_reg_w_a16_d16((0x124 | base_addr), 0xefe3);
	tvd_reg_w_a16_d16((0x126 | base_addr), 0x21e6);
	tvd_reg_w_a16_d16((0x128 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x12a | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x12c | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x12e | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x130 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x132 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x134 | base_addr), 0xb08c);
	tvd_reg_w_a16_d16((0x136 | base_addr), 0x8021);
	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0001);
	return tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
}

static int init_tvd_to_480p(int ch, int tvd_en)
{
	int base_addr;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];

	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa000);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x010 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x012 | base_addr), 0x2000);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x018 | base_addr), 0x502d);
	tvd_reg_w_a16_d16((0x01a | base_addr), 0x0fe9);
	tvd_reg_w_a16_d16((0x01c | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x01e | base_addr), 0x3e3e);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0025);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd803);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0x8600);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x0682);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0220);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0060);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5068);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0220);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0060);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5068);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x028 | base_addr), 0x0070);
	tvd_reg_w_a16_d16((0x02a | base_addr), 0x000e);
	tvd_reg_w_a16_d16((0x030 | base_addr), 0x3201);
	tvd_reg_w_a16_d16((0x032 | base_addr), 0x0446);
	tvd_reg_w_a16_d16((0x034 | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x036 | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x4209);
	tvd_reg_w_a16_d16((0x042 | base_addr), 0x0140);
	tvd_reg_w_a16_d16((0x044 | base_addr), 0x40af);
	tvd_reg_w_a16_d16((0x046 | base_addr), 0x0b14);
	tvd_reg_w_a16_d16((0x050 | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x052 | base_addr), 0x1420);
	tvd_reg_w_a16_d16((0x054 | base_addr), 0x0680);
	tvd_reg_w_a16_d16((0x056 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x058 | base_addr), 0x073a);
	tvd_reg_w_a16_d16((0x05a | base_addr), 0x173a);
	tvd_reg_w_a16_d16((0x138 | base_addr), 0x0c81);
	tvd_reg_w_a16_d16((0x13a | base_addr), 0x0040);
	tvd_reg_w_a16_d16((0x100 | base_addr), 0x4001);
	tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
	tvd_reg_w_a16_d16((0x104 | base_addr), 0x0450);
	tvd_reg_w_a16_d16((0x106 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x108 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x10a | base_addr), 0x0320);
	tvd_reg_w_a16_d16((0x10c | base_addr), 0x1819);
	tvd_reg_w_a16_d16((0x10e | base_addr), 0xcc9c);
	tvd_reg_w_a16_d16((0x110 | base_addr), 0x1dc0);
	tvd_reg_w_a16_d16((0x112 | base_addr), 0x0ae8);
	tvd_reg_w_a16_d16((0x114 | base_addr), 0x09bd);
	tvd_reg_w_a16_d16((0x116 | base_addr), 0xb9a0);
	tvd_reg_w_a16_d16((0x118 | base_addr), 0xc829);
	tvd_reg_w_a16_d16((0x11a | base_addr), 0x007e);
	tvd_reg_w_a16_d16((0x11c | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x11e | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x120 | base_addr), 0x9446);
	tvd_reg_w_a16_d16((0x122 | base_addr), 0x21f6);
	tvd_reg_w_a16_d16((0x124 | base_addr), 0xefe3);
	tvd_reg_w_a16_d16((0x126 | base_addr), 0x21e6);
	tvd_reg_w_a16_d16((0x128 | base_addr), 0x8acd);
	tvd_reg_w_a16_d16((0x12a | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x12c | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x12e | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x130 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x132 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x134 | base_addr), 0xb08c);
	tvd_reg_w_a16_d16((0x136 | base_addr), 0x8021);
	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0001);
	return tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
}

static int init_tvd_to_576p(int ch, int tvd_en)
{
	int base_addr;

	if (ch >= TVD_CH_MAX)
		return -1;
	base_addr = tvd_base_addr[ch];

	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0020);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xd003);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0xa000);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x8682);
	tvd_reg_w_a16_d16((0x010 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x012 | base_addr), 0x2000);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x018 | base_addr), 0x502d);
	tvd_reg_w_a16_d16((0x01a | base_addr), 0x0fe9);
	tvd_reg_w_a16_d16((0x01c | base_addr), 0x8000);
	tvd_reg_w_a16_d16((0x01e | base_addr), 0x3e3e);
	tvd_reg_w_a16_d16((0x004 | base_addr), 0x0025);
	tvd_reg_w_a16_d16((0x006 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x008 | base_addr), 0xdc03);
	tvd_reg_w_a16_d16((0x00a | base_addr), 0x8601);
	tvd_reg_w_a16_d16((0x00c | base_addr), 0x6440);
	tvd_reg_w_a16_d16((0x00e | base_addr), 0x0682);
	tvd_reg_w_a16_d16((0x014 | base_addr), 0x0001);
	tvd_reg_w_a16_d16((0x016 | base_addr), 0x4ed6);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0221);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x0061);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5072);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x024 | base_addr), 0x0281);
	tvd_reg_w_a16_d16((0x026 | base_addr), 0x00c0);
	tvd_reg_w_a16_d16((0x020 | base_addr), 0x5072);
	tvd_reg_w_a16_d16((0x022 | base_addr), 0x4e22);
	tvd_reg_w_a16_d16((0x028 | base_addr), 0x0070);
	tvd_reg_w_a16_d16((0x02a | base_addr), 0x000e);
	tvd_reg_w_a16_d16((0x030 | base_addr), 0x3203);
	tvd_reg_w_a16_d16((0x032 | base_addr), 0x0446);
	tvd_reg_w_a16_d16((0x034 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x036 | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x040 | base_addr), 0x4204);
	tvd_reg_w_a16_d16((0x042 | base_addr), 0x0140);
	tvd_reg_w_a16_d16((0x044 | base_addr), 0x43fa);
	tvd_reg_w_a16_d16((0x046 | base_addr), 0x0b15);
	tvd_reg_w_a16_d16((0x050 | base_addr), 0x7800);
	tvd_reg_w_a16_d16((0x052 | base_addr), 0x141b);
	tvd_reg_w_a16_d16((0x054 | base_addr), 0x0694);
	tvd_reg_w_a16_d16((0x056 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x058 | base_addr), 0x05b9);
	tvd_reg_w_a16_d16((0x05a | base_addr), 0x15b9);
	tvd_reg_w_a16_d16((0x138 | base_addr), 0x0c81);
	tvd_reg_w_a16_d16((0x13a | base_addr), 0x0040);
	tvd_reg_w_a16_d16((0x100 | base_addr), 0x4001);
	tvd_reg_w_a16_d16((0x102 | base_addr), 0x0010);
	tvd_reg_w_a16_d16((0x104 | base_addr), 0x0450);
	tvd_reg_w_a16_d16((0x106 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x108 | base_addr), 0x0000);
	tvd_reg_w_a16_d16((0x10a | base_addr), 0x0320);
	tvd_reg_w_a16_d16((0x10c | base_addr), 0x1819);
	tvd_reg_w_a16_d16((0x10e | base_addr), 0xcc9c);
	tvd_reg_w_a16_d16((0x110 | base_addr), 0x1dc0);
	tvd_reg_w_a16_d16((0x112 | base_addr), 0x0ae8);
	tvd_reg_w_a16_d16((0x114 | base_addr), 0x09bd);
	tvd_reg_w_a16_d16((0x116 | base_addr), 0xb9a0);
	tvd_reg_w_a16_d16((0x118 | base_addr), 0xc829);
	tvd_reg_w_a16_d16((0x11a | base_addr), 0x007e);
	tvd_reg_w_a16_d16((0x11c | base_addr), 0x7c1f);
	tvd_reg_w_a16_d16((0x11e | base_addr), 0x21f0);
	tvd_reg_w_a16_d16((0x120 | base_addr), 0x9446);
	tvd_reg_w_a16_d16((0x122 | base_addr), 0x21f6);
	tvd_reg_w_a16_d16((0x124 | base_addr), 0xefe3);
	tvd_reg_w_a16_d16((0x126 | base_addr), 0x21e6);
	tvd_reg_w_a16_d16((0x128 | base_addr), 0x8acb);
	tvd_reg_w_a16_d16((0x12a | base_addr), 0x2a09);
	tvd_reg_w_a16_d16((0x12c | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x12e | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x130 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x132 | base_addr), 0xffff);
	tvd_reg_w_a16_d16((0x134 | base_addr), 0xb08c);
	tvd_reg_w_a16_d16((0x136 | base_addr), 0x8021);
	tvd_reg_w_a16_d16((0x000 | base_addr), 0x0001);
	return tvd_reg_w_a16_d16((0x002 | base_addr), 0x0000);
}

/*for debug*/
/**
int sensor_read_array(struct v4l2_subdev *sd,
		struct regval_list *regs, int array_size)
{
	int ret = 0, i = 0, len = 1;

	if (!regs)
		return -EINVAL;
	sensor_dbg("%s array size %d\n", __func__, array_size);
	while (i < array_size) {
		len = 1;
		ret = sensor_read(sd, regs->addr, &regs->data);
		sensor_dbg("0x%x%x = 0x%x\n", (regs->addr & 0xff),
				(regs->addr & 0xff00) >> 8,
			regs->data);
		if (ret < 0) {
			sensor_print("%s write array error, array_size %d!\n",
					sd->name, array_size);
			return -EINVAL;
		}
		i += len;
		regs += len;
	}
	return 0;
}
*/

static int td100_write_tvd_top(struct v4l2_subdev *sd, int map,
		unsigned char _3d_ch)
{
	int ret;
	data_type val;

	sensor_dbg("%s init\n", __func__);
	sensor_write(sd, 0x0070, map);
	sensor_read(sd, 0x0070, &val);
	printk("map[%d] is 0x%x!\r\n", map, val);
	ret = init_tvd_top(_3d_ch);
	return ret;

}

static int td100_enable_adc(struct v4l2_subdev *sd, int mask)
{
	int ret;

	sensor_dbg("%s init , ch0x%x\n", __func__, mask);
	ret = init_tvd_top_adc(mask);
	return ret;
}

/*for defult setting*/
int td100_ntsc_4(struct v4l2_subdev *sd)
{
	int ret, i;

	sensor_print("%s init\n", __func__);

	td100_write_tvd_top(sd, TVD_4CH_CVBS, TVD_3D_CH);

	for (i = 0; i < 4; i++)
		init_tvd_to_ntsc(i, i == TVD_3D_CH, 1);

	ret = td100_enable_adc(sd, ADC_CH0 | ADC_CH1 |
			ADC_CH2 | ADC_CH3); /*init 4 ch adc*/
	/*reg init 19ms */
	return ret;
}

/*for defult setting*/
static int td100_pal_4(struct v4l2_subdev *sd)
{
	int ret, i;

	sensor_print("%s init\n", __func__);

	td100_write_tvd_top(sd, TVD_4CH_CVBS, TVD_3D_CH);

	for (i = 0; i < 4; i++)
		init_tvd_to_pal(i, i == TVD_3D_CH, 1);

	ret = td100_enable_adc(sd, ADC_CH0 | ADC_CH1 |
			ADC_CH2 | ADC_CH3); /*init 4 ch adc*/
	return ret;

}

static int __set_ch_fmt(struct v4l2_subdev *sd,
		int ch, int fmt, int init_tvd, int _3d_en)
{
	int ret;

	switch (fmt) {
	case CVBS_NTSC:
		ret = init_tvd_to_ntsc(ch, _3d_en, init_tvd);
		break;
	case CVBS_PAL:
		ret = init_tvd_to_pal(ch, _3d_en, init_tvd);
		break;
	case YCBCR_480I:
		ret = init_tvd_to_480i(ch, init_tvd);
		break;
	case YCBCR_576I:
		ret = init_tvd_to_576i(ch, init_tvd);
		break;
	case YCBCR_480P:
		ret = init_tvd_to_480p(ch, init_tvd);
		break;
	case YCBCR_576P:
		ret = init_tvd_to_576p(ch, init_tvd);
		break;
	default:
		return -1;
	}
	if (ret) {
		sensor_err("%s init tvd ch%d to %d fail\n", __func__, ch, fmt);
		return ret;
	}
	sensor_print("set ch%d to %d, %s 3d filter\n", ch,
			fmt, _3d_en ? "enable" : "disable");
	return 0;
}

/*External initialization of TVD interface*/
int td100_set_ch_fmt(struct v4l2_subdev *sd,
	struct tvin_init_info *info, int ch, int fmt)
{
	int ret;
	int _3d_en;

	if (ch >= TVD_CH_MAX)
		return -1;

	_3d_en = (info->ch_3d_filter == ch);
	if ((info->work_mode == Tvd_Input_Type_YCBCR_CVBS) && ch == 1) {
		ch = 3;
		_3d_en = 1;
	}

	switch (fmt) {
	case CVBS_NTSC:
		ret = init_tvd_to_ntsc(ch, _3d_en, 0);
		break;
	case CVBS_PAL:
		ret = init_tvd_to_pal(ch, _3d_en, 0);
		break;
	case YCBCR_480I:
		ret = init_tvd_to_480i(ch, 0);
		break;
	case YCBCR_576I:
		ret = init_tvd_to_576i(ch, 0);
		break;
	case YCBCR_480P:
		ret = init_tvd_to_480p(ch, 0);
		break;
	case YCBCR_576P:
		ret = init_tvd_to_576p(ch, 0);
		break;
	default:
		return -1;
	}
	if (ret) {
		sensor_err("%s init tvd ch%d to %d fail\n", __func__, ch, fmt);
		return ret;
	}
	sensor_print("set ch%d to %d, %s 3d filter\n", ch,
			fmt, _3d_en ? "enable" : "disable");
	return 0;
}

int td100_cvbs_init(struct v4l2_subdev *sd,
		struct tvin_init_info *info)
{
	int ch_num = info->ch_num;
	int _3d_ch = info->ch_3d_filter;
	int ch_id = info->ch_id;
	int cvbs_fmt;
	int ret, i;

	if (!ch_num) {
		sensor_err("td1000 ch number cannot be set to 0\n");
		return -1;
	}

	sensor_print("%s init, ch_num = %d, 3d ch = %d\n", __func__, ch_num, _3d_ch);
	usleep_range(1000, 1100);
	ret = td100_write_tvd_top(sd, TVD_4CH_CVBS, _3d_ch); /*cvbs mode*/
	if (ret) {
		sensor_err("%s init tvd top fail!\n", __func__);
		return ret;
	}

	/*init tvd ch fmt*/
	if (ch_num != 1) {
		for (i = 0; i < ch_num; i++) {
			cvbs_fmt = info->input_fmt[i];
			ret = __set_ch_fmt(sd, i,
					(is_cvbs(cvbs_fmt) ?  cvbs_fmt : CVBS_PAL),
					1, i == _3d_ch);
			if (ret)
				return ret;
		}
		usleep_range(1000, 1100);
		ret = init_tvd_top_adc(0x0f >> (TVD_CH_MAX - ch_num));
		if (ret) {
			sensor_err("%s init tvd adc fail!\n", __func__);
			return ret;
		}

	} else {
		cvbs_fmt = info->input_fmt[ch_id];
		ret = __set_ch_fmt(sd, 1,
				(is_cvbs(cvbs_fmt) ?  cvbs_fmt : CVBS_PAL),
				1, 1 == _3d_ch);
		if (ret)
			return ret;
		usleep_range(1000, 1100);
		ret = init_tvd_top_adc(ADC_CH0 << ch_id);
		if (ret) {
			sensor_err("%s init tvd adc fail!\n", __func__);
			return ret;
		}
	}

	return 0;
}

int td100_ycbcr_init(struct v4l2_subdev *sd,
		struct tvin_init_info *info)
{
	int ycbcr_fmt = info->input_fmt[0];
	int ret;

	sensor_print("%s init, fmt %d\n", __func__, ycbcr_fmt);

	/*YPRBPR  mode, disable 3dfilter*/
	ret = td100_write_tvd_top(sd, TVD_CVBS_AND_YPRBPR, TVD_CH_MAX);
	if (ret) {
		sensor_err("%s init tvd top fail!\n", __func__);
		return ret;
	}
	/*set tvd0 to YPRBPR  mode*/
	ret = __set_ch_fmt(sd, TVD_CH0,
			(is_ypbpr(ycbcr_fmt) ? ycbcr_fmt : YCBCR_576I), 1, 0);
	if (ret)
		return ret;
	/*enable 0-2 ch adc*/
	ret = td100_enable_adc(sd, ADC_CH0 | ADC_CH1 | ADC_CH2);
	if (ret) {
		sensor_err("%s init adc fail!\n", __func__);
		return ret;
	}
	return ret;
}

int td100_ycbcr_cvbs_init(struct v4l2_subdev *sd,
		struct tvin_init_info *info)
{
	int ycbcr_fmt = info->input_fmt[0];
	int cvbs_fmt = info->input_fmt[1];
	int ret;

	sensor_print("%s init\n", __func__);


	/*YPRBPR and cvbs mode,
	 *if need 3d filter, It must be set to tvd ch 3
	 */
	ret = td100_write_tvd_top(sd, TVD_CVBS_AND_YPRBPR, TVD_CH3);
	if (ret) {
		sensor_err("%s init tvd top fail!\n", __func__);
		return ret;
	}

	/*set tvd0 to YPRBPR  mode*/
	ret = __set_ch_fmt(sd, TVD_CH0,
			(is_ypbpr(ycbcr_fmt) ? ycbcr_fmt : YCBCR_576I), 1, 0);
	if (ret)
		return ret;
	/*set tvd3 to cvbs  mode*/
	ret = __set_ch_fmt(sd, TVD_CH3,
			(is_cvbs(cvbs_fmt) ?  cvbs_fmt : CVBS_PAL), 1, 1);
	if (ret)
		return ret;
	/*enable all ch adc*/
	usleep_range(1000, 1100);
	ret = td100_enable_adc(sd, ADC_CH0 | ADC_CH1 | ADC_CH2 | ADC_CH3);
	if (ret)
		sensor_err("%s init adc fail!\n", __func__);
	return ret;
}

static int status_get_sig_lock(data_type status_reg_l)
{
	return (status_reg_l & 0x0200) && (status_reg_l & 0x0400);
}

static int status_get_sig_detecte(data_type status_reg_l)
{
	return !(status_reg_l & 0x0100);
}

static int status_get_imp_fmt(data_type status_reg_h)
{
	return (status_reg_h >> 10 & 0x1);
}

/*get ch fmt is pal or ntsc*/
int __td100_get_input_fmt(struct v4l2_subdev *sd, int ch, int *value)
{
	int i, offset = INPUT_FMT_REG;
	data_type tvd_status4_reg_low;
	data_type tvd_status4_reg_high;

	/*read 4 ch input fmt*/
	for (i = 0; i < 10; i++) {
		sensor_read(sd, ch * 0x10 + 0x80 + offset,
					&tvd_status4_reg_high);
		sensor_read(sd, ch * 0x10 + 0x80 + offset - 0x200,
					&tvd_status4_reg_low);
		sensor_dbg("ch = %d, reg_H = 0x%x, L = 0x%x\n",
					ch, tvd_status4_reg_high, tvd_status4_reg_low);
		if (status_get_sig_lock(tvd_status4_reg_low)
			&& status_get_sig_detecte(tvd_status4_reg_low)) {
				*value = status_get_imp_fmt(tvd_status4_reg_high);
				sensor_dbg("value = %d\n", *value);
				return 0;
			}
		usleep_range(10000, 10200);
	}
	return -EINVAL;
}


static int td100_psram_trans(void)
{
	unsigned int reg_base = TD100_PSRAM_BASE;
	unsigned char rd = 0;
	unsigned int cnt = 0;

	/*0x48:start send*/
	tvd_reg_w_a16_d8(reg_base + START_SEND_REG, 0x01);
	tvd_reg_w_a16_d8(reg_base + START_SEND_REG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + START_SEND_REG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + START_SEND_REG + 3, 0x00);
	usleep_range(1000, 1100);
	while ((rd & 0x1) && (cnt < 100)) {
		cnt++;
		usleep_range(1000, 1100);
		tvd_reg_r_a16_d8(reg_base+START_SEND_REG, &rd);
	}
	if (cnt >= 100)
		sensor_err("read  START_SEND_REG cnt over %d timce\n", cnt);

	rd = 0;
	cnt = 0;
	usleep_range(1000, 1100);

	while (!(rd & 0x10) && (cnt < 100)) {
		usleep_range(1000, 1100);
		cnt++;
		tvd_reg_r_a16_d8(reg_base+INT_STA_REG+1, &rd);
	}
	if (cnt >= 100)
		sensor_err("read  START_SEND_REG cnt over %d time\n", cnt);
	tvd_reg_w_a16_d8(reg_base + INT_STA_REG, 0x00);
	tvd_reg_w_a16_d8(reg_base + INT_STA_REG + 1, 0x10);
	tvd_reg_w_a16_d8(reg_base + INT_STA_REG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + INT_STA_REG + 3, 0x00);
	return 0;
}

__maybe_unused static int tvd_psram_ap32_fast_init(void)
{
	unsigned int ret = 0;
	unsigned int reg_base = TD100_PSRAM_BASE;
	unsigned char rd[8] = {0};
	unsigned int psram_clk = 133000000; /*Hz*/
	unsigned int tcem = 2500; /*ns*/
	unsigned int tce_low;

#ifdef SRC_27MHz
	psram_clk = 135000000;
#else
	psram_clk = 133000000;
#endif

	tce_low = tcem*(psram_clk/1000000)/1000-16;
	/*start common cfg:0x70, clk_freq*ce_low_time=0x14c,clk=133MHz*/
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG, 0xfe);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 1, 0x61);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 2, (tce_low & 0xff));
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 3, 0xc0|((tce_low >> 8) & 0xf));
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG + 2, 0x20);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_TIM_CFG, 0x11);
	tvd_reg_w_a16_d8(reg_base + PSRAM_TIM_CFG + 1, 0x14);
	tvd_reg_w_a16_d8(reg_base + PSRAM_TIM_CFG + 2, 0x22);
	tvd_reg_w_a16_d8(reg_base + PSRAM_TIM_CFG + 3, 0x00);
	/*0x7c:start dqs0 force calibration*/
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY + 2, 0x0d);
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY + 3, 0x01);
	/*0xc0:start clk  force calibration*/
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY + 2, 0x02);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY + 3, 0x01);
	/*start misc:0xc4*/
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG, 0x12);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 1, 0x07);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 3, 0x00);
	/*enable all interrupt:0x54*/
	tvd_reg_w_a16_d8(reg_base + INT_EN_REG, 0x00);
	tvd_reg_w_a16_d8(reg_base + INT_EN_REG + 1, 0x7f);
	tvd_reg_w_a16_d8(reg_base + INT_EN_REG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + INT_EN_REG + 3, 0x00);

	/*start global reset: first do global reset*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0xff);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM, 0x02);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WDATA_REG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WDATA_REG, 0x00);
	td100_psram_trans();

	/*start global reset:second global reset*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0xff);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM, 0x02);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WDATA_REG, 0xa1);
	tvd_reg_w_a16_d8(reg_base + S_WDATA_REG, 0xa2);
	td100_psram_trans();

	/*start write APS_32_PSRAM_CHIP_REG0 as 0x37*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0xc0);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WDATA_REG, 0x37);
	td100_psram_trans();

	/*start wirte APS_32M_PSRAM_CHIP_REG4 as 0x80*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0xc0);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x04);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WDATA_REG, 0x80);
	td100_psram_trans();
	mdelay(1);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG, 0x02);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG + 1, 0x02);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG + 2, 0x20);
	tvd_reg_w_a16_d8(reg_base + PSRAM_LAT_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG, 0x02);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 1, 0x07);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 3, 0x00);

	/******begin to change freq to 162M*******/
	/*set psram clk, read regs check*/
#ifdef SRC_27MHz
	psram_clk = 162000000;
#else
	psram_clk = 168000000;
#endif
	tcem = 2500; /*ns*/
	tce_low = tcem * (psram_clk/1000000) / 1000 - 16;
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG, 0xfe);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 1, 0x61);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 2, (tce_low & 0xff));
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 3, 0xc0 | ((tce_low >> 8) & 0xf));

#ifdef SRC_27MHz
	tvd_reg_w_a16_d8(0x0043, 0x0b);
#else
	tvd_reg_w_a16_d8(0x0043, 0x0d);
#endif

	/*start read APS_32M_PSRAM_CHIP_REG0*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0x40);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 3, 0x00);
	td100_psram_trans();
	tvd_reg_r_a16_d8(reg_base + S_RDATA_REG, &rd[0]);
	if (rd[0] != 0x37)
		ret++;

	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0x40);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 3, 0x00);
	td100_psram_trans();
	tvd_reg_r_a16_d8(reg_base + S_RDATA_REG, &rd[0]);
	if (rd[0] != 0x8d)
		ret++;

	/*start read APS_32M_PSRAM_CHIP_REG2*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0x40);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x02);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 3, 0x00);
	td100_psram_trans();
	tvd_reg_r_a16_d8(reg_base + S_RDATA_REG, &rd[0]);
	if (rd[0] != 0xc9)
		ret++;

	/*start read APS_32M_PSRAM_CHIP_REG4*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0x40);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x04);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_RD_NUM + 3, 0x00);
	td100_psram_trans();
	tvd_reg_r_a16_d8(reg_base + S_RDATA_REG, &rd[0]);
	if (rd[0] != 0x80)
		ret++;

	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG, 0xfe);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 1, 0x21);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 2, (tce_low & 0xff));
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 3, 0xc0|((tce_low >> 8) & 0xf));

	tvd_reg_w_a16_d8(reg_base + CACHE_CFG, 0x0b);
	tvd_reg_w_a16_d8(reg_base + CACHE_CFG + 1, 0xc0);
	tvd_reg_w_a16_d8(reg_base + CACHE_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + CACHE_CFG + 3, 0x91);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG, 0x02);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 1, 0x07);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_MIS_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG, 0x7e);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 1, 0x21);
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 2, (tce_low & 0xff));
	tvd_reg_w_a16_d8(reg_base + PSRAM_COM_CFG + 3, 0xc0 | ((tce_low >> 8) & 0xf));
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY + 2, 0x0c);
	tvd_reg_w_a16_d8(reg_base + PSRAM_DQS0_IN_DLY + 3, 0x01);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY + 2, 0x02);
	tvd_reg_w_a16_d8(reg_base + PSRAM_CLK_OUT_DLY + 3, 0x01);
	tvd_reg_w_a16_d8(reg_base + C_READ_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_READ_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_READ_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_READ_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_WRITE_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_WRITE_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_WRITE_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + C_WRITE_CFG + 3, 0x80);
	/*start write APS_32M_PSRAM_CHIP_REG*/
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_CMD_CFG + 3, 0xc0);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_ADDR_CFG + 3, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM, 0x01);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 1, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 2, 0x00);
	tvd_reg_w_a16_d8(reg_base + S_WR_NUM + 3, 0x00);
	if (psram_clk > 166000000)
		tvd_reg_w_a16_d8(reg_base+S_WDATA_REG, 0x3b);
	else
		tvd_reg_w_a16_d8(reg_base+S_WDATA_REG, 0x37);
	td100_psram_trans();
	return ret;
}

__maybe_unused static int tvd_en_3d_mode(unsigned int sel_3d,
		unsigned int en, unsigned int addr_3d)
{
	unsigned int tvd_top_addr = 0x7000;
	unsigned int tvd_addr, addr2_3d;
	unsigned char r_data;

	sensor_print("%s en ch%d 3d filter\n", __func__, sel_3d);
	addr2_3d = addr_3d + 0x200000;
	tvd_addr = TD100_TVD0_BASE + 0x1000 * sel_3d;

	tvd_reg_w_a16_d8(tvd_top_addr + 0x10, (addr_3d >> 0) & 0xff);
	tvd_reg_w_a16_d8(tvd_top_addr + 0x11, (addr_3d >> 8) & 0xff);
	tvd_reg_w_a16_d8(tvd_top_addr + 0x12, (addr_3d >> 16) & 0xff);
	/*tvd_reg_w_a16_d8(tvd_top_addr + 0x13, (addr_3d >> 24) & 0xff);*/
	tvd_reg_w_a16_d8(tvd_top_addr + 0x14, (addr2_3d >> 0) & 0xff);
	tvd_reg_w_a16_d8(tvd_top_addr + 0x15, (addr2_3d >> 8) & 0xff);
	tvd_reg_w_a16_d8(tvd_top_addr + 0x16, (addr2_3d >> 16) & 0xff);
	/*tvd_reg_w_a16_d8(tvd_top_addr + 0x17, (addr2_3d >> 24) & 0xff);*/

	tvd_reg_w_a16_d8(tvd_top_addr + 0x18, 0);
	tvd_reg_w_a16_d8(tvd_top_addr + 0x19, 0);
	tvd_reg_w_a16_d8(tvd_top_addr + 0x1a, 0x20);
	/*tvd_reg_w_a16_d8(tvd_top_addr + 0x1b, 0x20);*/
	tvd_reg_w_a16_d8(tvd_top_addr + 0x08, ((sel_3d << 4) | (en << 1) | (en)) & 0xff);
	tvd_reg_r_a16_d8(tvd_addr + 0x40, &r_data);
	return tvd_reg_w_a16_d8(tvd_addr + 0x40, (r_data & 0xf7) | (!en << 3));
}

int td100_hw_regs_init(struct v4l2_subdev *sd,
		struct tvin_init_info *info)
{
	int ret;

	/*td100 top init*/
	ret = init_td100_top();
	if (ret) {
		sensor_err("init_td100_top fail\n");
		return ret;
	}

	/*defult mode*/
	if (info == NULL) {
#ifdef SUPPORT_3D_FILTER
		if (sensor_support_3d_filter())
			tvd_psram_ap32_fast_init();
#endif
		// ret = td100_pal_4(sd);
		ret = td100_ntsc_4(sd);
#ifdef SUPPORT_3D_FILTER
		if (sensor_support_3d_filter())
			tvd_en_3d_mode(TVD_3D_CH, 1, 0);
#endif
		return ret;
	}

	switch (info->work_mode) {
	case Tvd_Input_Type_1CH_CVBS:
	case Tvd_Input_Type_2CH_CVBS:
	case Tvd_Input_Type_4CH_CVBS:
#ifdef SUPPORT_3D_FILTER
		if (sensor_support_3d_filter())
			tvd_psram_ap32_fast_init();
#endif
		ret = td100_cvbs_init(sd, info);
#ifdef SUPPORT_3D_FILTER
		if (sensor_support_3d_filter())
			tvd_en_3d_mode(info->ch_3d_filter, 1, 0);
#endif
		break;
	case Tvd_Input_Type_YCBCR:
		ret = td100_ycbcr_init(sd, info);
		break;
	case Tvd_Input_Type_YCBCR_CVBS:
#ifdef SUPPORT_3D_FILTER
		if (sensor_support_3d_filter())
			tvd_psram_ap32_fast_init();
#endif
		ret = td100_ycbcr_cvbs_init(sd, info);
#ifdef SUPPORT_3D_FILTER
		if (sensor_support_3d_filter())
			tvd_en_3d_mode(3, 1, 0);
#endif
		break;
	default:
		ret = td100_pal_4(sd);
		break;

	}
	return ret;
}
