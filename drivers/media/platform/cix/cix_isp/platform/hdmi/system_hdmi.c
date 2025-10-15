/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <asm/io.h>
#include "system_logger.h"
#include "types_utils.h"
#include "system_hdmi.h"
#include "system_vdma.h"

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_COMMON
#endif

#define ARMCB_HDMI_I2CSEND_BUFLENS_MAX 255

extern void armcb_hwchnnel_select(unsigned char hwchnl);
extern unsigned char armcb_get_sensor_hwchnl(void);

static struct armcb_hdmi_info *p_hdmi_info = NULL;
static struct i2c_client *pclient_i2c1 = NULL;

static u64 DO_DIV64(u64 mol, u64 den)
{
	u64 res = mol;
	if (den != 0) {
		do_div(res, den);
	}
	return res;
}

static s32 armcb_hdmi_i2c_read(struct armcb_i2c_transfer *pi2c_transfer, u8 *ucData)
{
	struct i2c_client *pi2c_client;
	s32 res = 0, bufcnt = 2;
	u8 buf[2] = {0};

	if (!pi2c_transfer) {
		LOG(LOG_ERR, "Read pi2c_transfer is NULL");
		return -1;
	}


	armcb_hwchnnel_select(pi2c_transfer->ch );
	if (pi2c_transfer->datatype == ARMCN_I2C_TRANS_DATATYPE_REG8DATA8) {
		bufcnt = 1;
		buf[0] = (pi2c_transfer->usAddr & 0xff);
	} else {
		bufcnt = 2;
		buf[0] = (pi2c_transfer->usAddr >> 8);
		buf[1] = (pi2c_transfer->usAddr & 0xff);
	}

	pi2c_client = pclient_i2c1;
	pi2c_client->addr = pi2c_transfer->slvaddr;
	res = i2c_master_send(pi2c_client, (s8 *)buf, bufcnt);
	if (res < 0) {
		LOG(LOG_ERR, "i2c_master_send usAddr(%x) faied", pi2c_transfer->usAddr);
		return -1;
	}
	mdelay(1);
	bufcnt = 1;
	res = i2c_master_recv(pi2c_client, (char *)ucData, bufcnt);
	if (res <= 0) {
		LOG(LOG_ERR, "i2c_master_recv ucAddr(%x) faied", pi2c_transfer->usAddr);
		return -1;
	}

	return res;
}

static s32 armcb_hdmi_i2c_write(struct armcb_i2c_transfer *pi2c_transfer)
{
	struct i2c_client *pi2c_client;
	s32 res = 0, bufcnt = 2;
	u8 buf[3] = {0};
	if (!pi2c_transfer) {
		LOG(LOG_ERR, "Read pi2c_transfer is NULL");
		return -1;
	}

	armcb_hwchnnel_select(pi2c_transfer->ch);
	if (pi2c_transfer->datatype == ARMCN_I2C_TRANS_DATATYPE_REG8DATA8) {
		bufcnt = 2;
		buf[0] = (pi2c_transfer->usAddr & 0xff);
		buf[1] = (pi2c_transfer->usData & 0xff);
	} else {
		bufcnt = 3;
		buf[0] = (pi2c_transfer->usAddr >> 8);
		buf[1] = (pi2c_transfer->usAddr & 0xff);
		buf[2] = (pi2c_transfer->usData & 0xff);
	}

	pi2c_client = pclient_i2c1;
	if (!pi2c_client) {
		LOG(LOG_ERR, "pi2c_client is NULL");
		return -1;
	}
	pi2c_client->addr = pi2c_transfer->slvaddr;
	res = i2c_master_send(pi2c_client, (s8 *)buf, bufcnt);
	if (res < 0) {
		LOG(LOG_ERR, "i2c_master_send ch=%d, usAddr(%x) failed",
			pi2c_transfer->ch, pi2c_transfer->usAddr);
		return -1;
	}

	return res;
}

static s32 armcb_i2c_bufferwrite(struct armcb_i2c_transfer *pi2c_transfer, u8 *buff, s32 bufcnt)
{
	struct i2c_client *pi2c_client;
	s32 res = 0, offset = 0;
	u8 buf[ARMCB_HDMI_I2CSEND_BUFLENS_MAX] = {0};
	if (!pi2c_transfer || !buff) {
		LOG(LOG_ERR, "pi2c_transfer or buff is NULL");
		return -1;
	}


	armcb_hwchnnel_select(pi2c_transfer->ch);
	if (pi2c_transfer->datatype == ARMCN_I2C_TRANS_DATATYPE_REG8DATA8) {
		buf[0] = (pi2c_transfer->usAddr & 0xff);
		offset = 1;
	} else {
		bufcnt = 3;
		buf[0] = (pi2c_transfer->usAddr >> 8);
		buf[1] = (pi2c_transfer->usAddr & 0xff);
		offset = 2;
	}

	bufcnt += offset;
	memcpy(&buf[offset], buff, bufcnt);

	pi2c_client = pclient_i2c1;
	pi2c_client->addr = pi2c_transfer->slvaddr;
	res = i2c_master_send(pi2c_client, (s8 *)buf, bufcnt);
	if (res < 0) {
		LOG(LOG_ERR, "i2c_master_send usAddr(%x) failed", pi2c_transfer->usAddr);
		return -1;
	}

	return res;
}

/* HDMI I2C_TRANSFER FUNCTION */
s32 armcb_hdmi_i2c_write_8(u8 slvaddr, u8 ucAddr, u8 ucWdat, u8 ch)
{
	s32 res = 0;
	struct armcb_i2c_transfer i2c_transfer;
	i2c_transfer.ch = ch;
	i2c_transfer.slvaddr = slvaddr;
	i2c_transfer.usAddr = (u16)ucAddr;
	i2c_transfer.usData = (u16)ucWdat;
	i2c_transfer.datatype = ARMCN_I2C_TRANS_DATATYPE_REG8DATA8;
	res = armcb_hdmi_i2c_write(&i2c_transfer);
	if (res < 0) {
		LOG(LOG_ERR, "ucAddr[%x] failed", ucAddr);
	}
	return res;
}

s32 armcb_hdmi_i2c_bufferwrite(u8 slvaddr, u8 ucAddr, s32 bufcnt, u8* buff, u8 ch)
{
	s32 res = 0;
	struct armcb_i2c_transfer i2c_transfer;

	if (!buff) {
		LOG(LOG_ERR, "buff is NULL");
		return -1;
	}
	i2c_transfer.ch = ch;
	i2c_transfer.slvaddr = slvaddr;
	i2c_transfer.usAddr = (u16)ucAddr;
	i2c_transfer.usData = (u16)ucAddr;
	i2c_transfer.datatype = ARMCN_I2C_TRANS_DATATYPE_REG8DATA8;
	res = armcb_i2c_bufferwrite(&i2c_transfer, buff, bufcnt);
	if (res < 0) {
		LOG(LOG_ERR, "ucAddr[%x] failed", ucAddr);
	}
	return res;
}

s32 armcb_hdmi_i2c_read_8(u8 slvaddr, u8 ucAddr, u8 *ucData, u8 ch)
{
	s32 res = 0;
	struct armcb_i2c_transfer i2c_transfer;
	i2c_transfer.ch = ch;
	i2c_transfer.slvaddr = slvaddr;
	i2c_transfer.usAddr = (u16)ucAddr;
	i2c_transfer.datatype = ARMCN_I2C_TRANS_DATATYPE_REG8DATA8;
	res = armcb_hdmi_i2c_read(&i2c_transfer, ucData);
	if (res < 0) {
		LOG(LOG_ERR, "ucAddr[%x] failed", ucAddr);
	}
	return res;
}

#define hdmi_i2c_write(id, addr, wdat)             armcb_hdmi_i2c_write_8(id, addr, wdat, armcb_get_sensor_hwchnl())
#define hdmi_i2c_writem(id, start, count, pdat)    armcb_hdmi_i2c_bufferwrite(id, start, count, pdat, armcb_get_sensor_hwchnl())
#define hdmi_i2c_read(id, addr)                    \
    ({u8 ucData = 0;                               \
    armcb_hdmi_i2c_read_8(id, addr, &ucData, armcb_get_sensor_hwchnl()); \
    ucData;})

void hdmi_i2c_regdump(u8 ucId, u8 ucStartAddr, s32 n)
{
	s32 i;
	u8 ucTmp;

	for (i = 0; i < n; i++)	{
		ucTmp = hdmi_i2c_read(ucId, ucStartAddr++);
		LOG(LOG_INFO, "Reg[0x%x]=0x%x on slave 0x%x.", ucStartAddr, ucTmp, (ucId << 1));
	}
}

//---------------------------------------------------------------------------------
// Function name: hdmi_ahb_read()
// Description  : Read ahb bus through I2C of 8-bit data on 16-bit address.
// Parameters   :
//   u8  ucId:        I2C slave Id.
//   u16 usRegAddr  : Register address.
// Return       : None.
// Notes        :
//---------------------------------------------------------------------------------
u8 hdmi_ahb_read(u8 ucId, u16 usRegAddr)
{
	u8  ucAhbRdBase = 0x18;  // Write data base for AHB access.
	u8  ucTmp = 0;

	hdmi_i2c_writem(ucId, 0x10, 2, (u8 *)(&usRegAddr)); // Address base for AHB access.

	hdmi_i2c_write(ucId, 0x1c, 0x21); // Command reg address.
	hdmi_i2c_write(ucId, 0x1c, 0x20); // Command reg address.

	if ((usRegAddr & 0x03) < 4 ) {
		ucAhbRdBase += (usRegAddr & 0x03);
		ucTmp = hdmi_i2c_read(ucId, ucAhbRdBase);
	}

	return ucTmp;
}

//---------------------------------------------------------------------------------
// Function name: hdmi_ahb_write()
// Description  : Write ahb bus through I2C with 8-bit data on 16-bit address.
// Parameters   :
//   u8  ucId     : I2C slave Id.
//   u16 usRegAddr: Register address.
//   u8  ucWrData : Write data.
// Return       : None.
// Notes        :
//---------------------------------------------------------------------------------
void hdmi_ahb_write(u8 ucId, u16 usRegAddr, u8 ucWrData)
{
	u8  ucAhbWrBase = 0x14;  // Write data base for AHB access.

	hdmi_i2c_writem(ucId, 0x10, 2, (u8 *)(&usRegAddr)); // Address base for AHB access.

	if ((usRegAddr & 0x03) < 4) {
		ucAhbWrBase += (usRegAddr & 0x03);
		hdmi_i2c_write(ucId, ucAhbWrBase, ucWrData);
	}

	hdmi_i2c_write(ucId, 0x1c, 0x23); // Command reg address.
	hdmi_i2c_write(ucId, 0x1c, 0x20); // Command reg address.
}

#ifdef HDMI_DEBUG
void hdmi_debug_dumpreg(u8 slvaddr, u32 idx)
{
	if (idx == 0) {
		LOG(LOG_DEBUG, "read 0x300d = 0x%x", hdmi_ahb_read(slvaddr, 0x300d));
		LOG(LOG_DEBUG, "read 0x300b = 0x%x", hdmi_ahb_read(slvaddr, 0x300b));
		LOG(LOG_DEBUG, "read 0x3013 = 0x%x", hdmi_ahb_read(slvaddr, 0x3013));
		LOG(LOG_DEBUG, "read 0x3079 = 0x%x", hdmi_ahb_read(slvaddr, 0x3079));
		LOG(LOG_DEBUG, "read 0x3065 = 0x%x", hdmi_ahb_read(slvaddr, 0x3065));
		LOG(LOG_DEBUG, "read 0x30f7 = 0x%x", hdmi_ahb_read(slvaddr, 0x30f7));
		LOG(LOG_DEBUG, "read 0x3224 = 0x%x", hdmi_ahb_read(slvaddr, 0x3224));
		LOG(LOG_DEBUG, "read 0x33f6 = 0x%x", hdmi_ahb_read(slvaddr, 0x33f6));
		LOG(LOG_DEBUG, "read 0x3609 = 0x%x", hdmi_ahb_read(slvaddr, 0x3609));
		LOG(LOG_DEBUG, "read 0x360a = 0x%x", hdmi_ahb_read(slvaddr, 0x360a));
		LOG(LOG_DEBUG, "read 0x3900 = 0x%x", hdmi_ahb_read(slvaddr, 0x3900));
		LOG(LOG_DEBUG, "read 0x3909 = 0x%x", hdmi_ahb_read(slvaddr, 0x3909));
		LOG(LOG_DEBUG, "read 0x3b1c = 0x%x", hdmi_ahb_read(slvaddr, 0x3b1c));
		LOG(LOG_DEBUG, "read 0x3d20 = 0x%x", hdmi_ahb_read(slvaddr, 0x3d20));
		LOG(LOG_DEBUG, "read 0x3d21 = 0x%x", hdmi_ahb_read(slvaddr, 0x3d21));
		LOG(LOG_DEBUG, "read 0x3da0 = 0x%x", hdmi_ahb_read(slvaddr, 0x3da0));
		LOG(LOG_DEBUG, "read 0x3da1 = 0x%x", hdmi_ahb_read(slvaddr, 0x3da1));
	} else {
		LOG(LOG_DEBUG, "read 0x36bf = 0x%x", hdmi_ahb_read(slvaddr,0x36bf));
		LOG(LOG_DEBUG, "read 0x36c0 = 0x%x", hdmi_ahb_read(slvaddr,0x36c0));
		LOG(LOG_DEBUG, "read 0x36c1 = 0x%x", hdmi_ahb_read(slvaddr,0x36c1));
		LOG(LOG_DEBUG, "read 0x36c2 = 0x%x", hdmi_ahb_read(slvaddr,0x36c2));
		LOG(LOG_DEBUG, "read 0x36c3 = 0x%x", hdmi_ahb_read(slvaddr,0x36c3));
		LOG(LOG_DEBUG, "read 0x36c4 = 0x%x", hdmi_ahb_read(slvaddr,0x36c4));
		LOG(LOG_DEBUG, "read 0x36c5 = 0x%x", hdmi_ahb_read(slvaddr,0x36c5));
		LOG(LOG_DEBUG, "read 0x36c6 = 0x%x", hdmi_ahb_read(slvaddr,0x36c6));
		LOG(LOG_DEBUG, "read 0x36c7 = 0x%x", hdmi_ahb_read(slvaddr,0x36c7));
		LOG(LOG_DEBUG, "read 0x36c8 = 0x%x", hdmi_ahb_read(slvaddr,0x36c8));
		LOG(LOG_DEBUG, "read 0x36c9 = 0x%x", hdmi_ahb_read(slvaddr,0x36c9));
		LOG(LOG_DEBUG, "read 0x36ca = 0x%x", hdmi_ahb_read(slvaddr,0x36ca));
		LOG(LOG_DEBUG, "read 0x36cb = 0x%x", hdmi_ahb_read(slvaddr,0x36cb));
		LOG(LOG_DEBUG, "read 0x36cc = 0x%x", hdmi_ahb_read(slvaddr,0x36cc));
		LOG(LOG_DEBUG, "read 0x36cd = 0x%x", hdmi_ahb_read(slvaddr,0x36cd));
		LOG(LOG_DEBUG, "read 0x36ce = 0x%x", hdmi_ahb_read(slvaddr,0x36ce));
		LOG(LOG_DEBUG, "read 0x36cf = 0x%x", hdmi_ahb_read(slvaddr,0x36cf));
		LOG(LOG_DEBUG, "read 0x36df = 0x%x", hdmi_ahb_read(slvaddr,0x36df));
	}
}
#endif

static int hdmi_phyhw_init(u32 unHdmiOutFmt)
{
	u8 ucId2 = FPGA_HDMITX_IIC;

	hdmi_ahb_write(ucId2,   0x300d,   0x04); // [2]: PCLK enable.
	hdmi_ahb_write(ucId2,   0x300b,   0x01); // [0]: Enable HDMI for TX
	hdmi_ahb_write(ucId2,   0x3013,   0x00); // [7:2]: clks bypass. [1]: miso edge, 0/1=falling/rising edge. [0]: mosi edge, 0/1=falling/rising edge.
	hdmi_ahb_write(ucId2,   0x3079,   0x06); // [2]: 1=audio split en. [1]: 1=audio pass through.
	hdmi_ahb_write(ucId2,   0x3065,   0x05); // [3:2]: 2'b01=pclknx same as pclk. [1:0]: 2'b01=idclk same as pclk.
	hdmi_ahb_write(ucId2,   0x30f7,   0x02); // [1]: 1=hmdi mode, 0= DVI mode.
	hdmi_ahb_write(ucId2,   0x3224,   0x04); // [3:2]: =2'b01, hdmi_clk is 1x pxl_clk(default); =2'b00, hdmi clk is 0.5*pxl_clk.

	hdmi_ahb_write(ucId2,   0x3900,   0x00);

	hdmi_ahb_write(ucId2,   0x3b1c,   0x01); // [0]: Configure sync polarity adjustment for datapath 0. ?
	hdmi_ahb_write(ucId2,   0x3d20,   0x00); // Color space converter setting0.
	hdmi_ahb_write(ucId2,   0x3d21,   0x08); // [3]: Disable saturation.
	hdmi_ahb_write(ucId2,   0x3da0,   0x00); // Color space converter setting1.
	hdmi_ahb_write(ucId2,   0x3da1,   0x08);

	hdmi_ahb_write(ucId2,   0x3909,   0x00); // [1:0]: TXC divided by 1<<[1:0]. 2'b10= divided by 4.
	hdmi_ahb_write(ucId2,   0x33f6,   0x30);

	switch (unHdmiOutFmt) {
		case HDMIOUT_RGB444_8BIT:
		{
			hdmi_ahb_write(ucId2,   0x3609, 0x00);  // Input
			hdmi_ahb_write(ucId2,   0x360a, 0x00);  // Output
			LOG(LOG_INFO, "HDMI output format: RGB444, 8-bit.");
			break;
		}
		case HDMIOUT_YUV444_8BIT:
		{
			hdmi_ahb_write(ucId2,   0x3609, 0x01);
			hdmi_ahb_write(ucId2,   0x360a, 0x01);
			LOG(LOG_INFO, "HDMI output format: YUV444, 8-bit.");
			break;
		}
		case HDMIOUT_YUV422_8BIT:
		{
			hdmi_ahb_write(ucId2,   0x3609, 0x02);
			hdmi_ahb_write(ucId2,   0x360a, 0x02);
			LOG(LOG_INFO, "HDMI output format: YUV422, 8-bit.");
			break;
		}
		default:
		{
			LOG(LOG_INFO, "Unsupported HDMI output format %d!", (u32)unHdmiOutFmt);
			return -1;
		}
	}
#ifdef HDMI_DEBUG
	hdmi_debug_dumpreg(ucId2, 0);
#endif
	msleep(1);

	return 0;
}

//---------------------------------------------------------------------------------
// Function name: hdmiphy_aviinfo720p60()
// Description  : Tx avi information.
// Parameters   : None.
// Return       : None.
// Notes        :
//---------------------------------------------------------------------------------
s32 hdmi_phy_set_fmt_avi720p60(u32 unHdmiOutFmt)
{
	u8 ucId2=FPGA_HDMITX_IIC;

	hdmi_ahb_write(ucId2,  0x36bf, 0x00); //  Buffer 0 select

	hdmi_ahb_write(ucId2,  0x36c0, 0x82); //  AVI Packet type 0x82
	hdmi_ahb_write(ucId2,  0x36c1, 0x02); //  Version     0x02
	hdmi_ahb_write(ucId2,  0x36c2, 0x0d); //  Length      0x0d

	switch (unHdmiOutFmt) {
		case HDMIOUT_RGB444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x63); //  Check sum
			hdmi_ahb_write(ucId2,  0x36c4, 0x00); //  [7:5]: 001=YUV422, 010=YUV444, 000=RGB. [4]: 1=Active aspect ratio information included(0x360e[3:0] is valid)
			break;
		}
		case HDMIOUT_YUV444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x23);
			hdmi_ahb_write(ucId2,  0x36c4, 0x40);
			break;
		}
		case HDMIOUT_YUV422_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x43);
			hdmi_ahb_write(ucId2,  0x36c4, 0x20);
			break;
		}
		default:
		{
			LOG(LOG_INFO, "Unsupported HDMI output format %d!", (u32)unHdmiOutFmt);
			return -1;
		}
	}

	hdmi_ahb_write(ucId2,  0x36c5, 0x08); //  [7:6]: c1c0, 01=SMPTE 170M, 10=ITU-R BT.709. [5:4]: M1M0, 00=NoData, 01=4:3, 10=16:9. [3:0]: 1000=Same as picture aspect ratio. 1001=4:3, 1010=16:9
	hdmi_ahb_write(ucId2,  0x36c6, 0x00); //
	hdmi_ahb_write(ucId2,  0x36c7, 0x04); //  vic 4: 1280x720p60

	hdmi_ahb_write(ucId2,  0x36c8, 0x00);
	hdmi_ahb_write(ucId2,  0x36c9, 0x00);
	hdmi_ahb_write(ucId2,  0x36ca, 0x00);
	hdmi_ahb_write(ucId2,  0x36cb, 0x00);
	hdmi_ahb_write(ucId2,  0x36cc, 0x00);
	hdmi_ahb_write(ucId2,  0x36cd, 0x00);
	hdmi_ahb_write(ucId2,  0x36ce, 0x00);
	hdmi_ahb_write(ucId2,  0x36cf, 0x00);

	hdmi_ahb_write(ucId2,  0x36df, 0xc0); //  bit[7]: Enable, bit[6]: Repeat.

	return 0;
}

s32 hdmi_phy_set_fmt_avi1080p60(u32 unHdmiOutFmt)
{
	u8 ucId2=FPGA_HDMITX_IIC;

	hdmi_ahb_write(ucId2,  0x36bf, 0x00); //  Buffer 0 select
	hdmi_ahb_write(ucId2,  0x36c0, 0x82); //  AVI Packet type 0x82
	hdmi_ahb_write(ucId2,  0x36c1, 0x02); //  Version     0x02
	hdmi_ahb_write(ucId2,  0x36c2, 0x0d); //  Length      0x0d

	switch (unHdmiOutFmt) {
		case HDMIOUT_RGB444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x57); //  Check sum
			hdmi_ahb_write(ucId2,  0x36c4, 0x00); //  [7:5]: 001=YUV422, 010=YUV444, 000=RGB. [4]: 1=Active aspect ratio information included(0x360e[3:0] is valid)
			break;
		}

		case HDMIOUT_YUV444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x17);
			hdmi_ahb_write(ucId2,  0x36c4, 0x40);
			break;
		}
		case HDMIOUT_YUV422_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x37);
			hdmi_ahb_write(ucId2,  0x36c4, 0x20);
			break;
		}
		default:
		{
			LOG(LOG_INFO, "Unsupported HDMI output format %d!", (u32)unHdmiOutFmt);
			return -1;
		}
	}

	hdmi_ahb_write(ucId2,  0x36c5, 0x08); //  [7:6]: c1c0, 01=SMPTE 170M, 10=ITU-R BT.709. [5:4]: M1M0, 00=NoData, 01=4:3, 10=16:9. [3:0]: 1000=Same as picture aspect ratio. 1001=4:3, 1010=16:9
	hdmi_ahb_write(ucId2,  0x36c6, 0x00); //
	hdmi_ahb_write(ucId2,  0x36c7, 0x10); //  vic 16: 1920x1080p60. SMPTE 274M [2]

	///-- Below setting for this AVI not necessary.
	hdmi_ahb_write(ucId2,  0x36c8, 0x00);
	hdmi_ahb_write(ucId2,  0x36c9, 0x00);
	hdmi_ahb_write(ucId2,  0x36ca, 0x00);
	hdmi_ahb_write(ucId2,  0x36cb, 0x00);
	hdmi_ahb_write(ucId2,  0x36cc, 0x00);
	hdmi_ahb_write(ucId2,  0x36cd, 0x00);
	hdmi_ahb_write(ucId2,  0x36ce, 0x00);
	hdmi_ahb_write(ucId2,  0x36cf, 0x00);

	hdmi_ahb_write(ucId2,  0x36df, 0xc0); //  bit[7]: Enable, bit[6]: Repeat.

#ifdef HDMI_DEBUG
	hdmi_debug_dumpreg(ucId2, 1);
#endif

	return 0;
}

//---------------------------------------------------------------------------------
// Function name: hdmi_phy_set_fmt_avi720p30()
// Description  : Tx avi information.
// Parameters   : None.
// Return       : None.
// Notes        :
//---------------------------------------------------------------------------------
s32 hdmi_phy_set_fmt_avi720p30(u32 unHdmiOutFmt)
{
	u8 ucId2=FPGA_HDMITX_IIC;

	hdmi_ahb_write(ucId2,  0x36bf, 0x00); //  Buffer 0 select
	hdmi_ahb_write(ucId2,  0x36c0, 0x82); //  AVI Packet type 0x82
	hdmi_ahb_write(ucId2,  0x36c1, 0x02); //  Version     0x02
	hdmi_ahb_write(ucId2,  0x36c2, 0x0d); //  Length      0x0d

	switch (unHdmiOutFmt) {
		case HDMIOUT_RGB444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x29); //  Check sum
			hdmi_ahb_write(ucId2,  0x36c4, 0x00); //  [7:5]: 001=YUV422, 010=YUV444, 000=RGB. [4]: 1=Active aspect ratio information included(0x360e[3:0] is valid)
			break;
		}
		case HDMIOUT_YUV444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0xe9);
			hdmi_ahb_write(ucId2,  0x36c4, 0x40);
			break;
		}
		case HDMIOUT_YUV422_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x09);
			hdmi_ahb_write(ucId2,  0x36c4, 0x20);
			break;
		}
		default:
		{
			LOG(LOG_INFO, "Unsupported HDMI output format %d!", (u32)unHdmiOutFmt);
			return -1;
		}
	}

	hdmi_ahb_write(ucId2,  0x36c5, 0x08); //  [7:6]: c1c0, 01=SMPTE 170M, 10=ITU-R BT.709. [5:4]: M1M0, 00=NoData, 01=4:3, 10=16:9. [3:0]: 1000=Same as picture aspect ratio. 1001=4:3, 1010=16:9
	hdmi_ahb_write(ucId2,  0x36c6, 0x00); //
	hdmi_ahb_write(ucId2,  0x36c7, 0x3e); //  vic 62/67: 1280x720p30

	hdmi_ahb_write(ucId2,  0x36c8, 0x00);
	hdmi_ahb_write(ucId2,  0x36c9, 0x00);
	hdmi_ahb_write(ucId2,  0x36ca, 0x00);
	hdmi_ahb_write(ucId2,  0x36cb, 0x00);
	hdmi_ahb_write(ucId2,  0x36cc, 0x00);
	hdmi_ahb_write(ucId2,  0x36cd, 0x00);
	hdmi_ahb_write(ucId2,  0x36ce, 0x00);
	hdmi_ahb_write(ucId2,  0x36cf, 0x00);

	hdmi_ahb_write(ucId2,  0x36df, 0xc0); //  bit[7]: Enable, bit[6]: Repeat.

	return 0;
}

//---------------------------------------------------------------------------------
// Function name: hdmi_phy_set_fmt_avi1080i50()
// Description  : Tx avi information.
// Parameters   : None.
// Return       : None.
// Notes        :
//---------------------------------------------------------------------------------
s32 hdmi_phy_set_fmt_avi1080i50(u32 unHdmiOutFmt)
{
	u8 ucId2=FPGA_HDMITX_IIC;

	hdmi_ahb_write(ucId2,  0x36bf, 0x00); //  Buffer 0 select

	hdmi_ahb_write(ucId2,  0x36c0, 0x82); //  AVI Packet type 0x82
	hdmi_ahb_write(ucId2,  0x36c1, 0x02); //  Version     0x02
	hdmi_ahb_write(ucId2,  0x36c2, 0x0d); //  Length      0x0d

	switch (unHdmiOutFmt) {
		case HDMIOUT_RGB444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x53); //  Check sum
			hdmi_ahb_write(ucId2,  0x36c4, 0x00); //  [7:5]: 001=YUV422, 010=YUV444, 000=RGB. [4]: 1=Active aspect ratio information included(0x360e[3:0] is valid)
			break;
		}
		case HDMIOUT_YUV444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x13);
			hdmi_ahb_write(ucId2,  0x36c4, 0x40);
			break;
		}
		case HDMIOUT_YUV422_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x33);
			hdmi_ahb_write(ucId2,  0x36c4, 0x20);
			break;
		}
		default:
		{
			LOG(LOG_INFO, "Unsupported HDMI output format %d!", (u32)unHdmiOutFmt);
			return -1;
		}
	}

	hdmi_ahb_write(ucId2,  0x36c5, 0x08); //  [7:6]: c1c0, 01=SMPTE 170M, 10=ITU-R BT.709. [5:4]: M1M0, 00=NoData, 01=4:3, 10=16:9. [3:0]: 1000=Same as picture aspect ratio. 1001=4:3, 1010=16:9
	hdmi_ahb_write(ucId2,  0x36c6, 0x00); //
	hdmi_ahb_write(ucId2,  0x36c7, 0x14); //  vic 20: 1080I50

	hdmi_ahb_write(ucId2,  0x36c8, 0x00);
	hdmi_ahb_write(ucId2,  0x36c9, 0x00);
	hdmi_ahb_write(ucId2,  0x36ca, 0x00);
	hdmi_ahb_write(ucId2,  0x36cb, 0x00);
	hdmi_ahb_write(ucId2,  0x36cc, 0x00);
	hdmi_ahb_write(ucId2,  0x36cd, 0x00);
	hdmi_ahb_write(ucId2,  0x36ce, 0x00);
	hdmi_ahb_write(ucId2,  0x36cf, 0x00);

	hdmi_ahb_write(ucId2,  0x36df, 0xc0); //  bit[7]: Enable, bit[6]: Repeat.

	return 0;
}

//---------------------------------------------------------------------------------
// Function name: hdmi_phy_set_fmt_avi1080p50()
// Description  : Tx avi information.
// Parameters   : None.
// Return       : None.
// Notes        :
//---------------------------------------------------------------------------------
s32 hdmi_phy_set_fmt_avi1080p50(u32 unHdmiOutFmt)
{
	u8 ucId2=FPGA_HDMITX_IIC;

	hdmi_ahb_write(ucId2,  0x36bf, 0x00); //  Buffer 0 select

	hdmi_ahb_write(ucId2,  0x36c0, 0x82); //  AVI Packet type 0x82
	hdmi_ahb_write(ucId2,  0x36c1, 0x02); //  Version     0x02
	hdmi_ahb_write(ucId2,  0x36c2, 0x0d); //  Length      0x0d

	switch (unHdmiOutFmt) {
		case HDMIOUT_RGB444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x48); //  Check sum
			hdmi_ahb_write(ucId2,  0x36c4, 0x00); //  [7:5]: 001=YUV422, 010=YUV444, 000=RGB. [4]: 1=Active aspect ratio information included(0x360e[3:0] is valid)
			break;
		}
		case HDMIOUT_YUV444_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x08);
			hdmi_ahb_write(ucId2,  0x36c4, 0x40);
			break;
		}
		case HDMIOUT_YUV422_8BIT:
		{
			hdmi_ahb_write(ucId2,  0x36c3, 0x28);
			hdmi_ahb_write(ucId2,  0x36c4, 0x20);
			break;
		}
		default:
		{
			LOG(LOG_INFO, "Unsupported HDMI output format %d!", (u32)unHdmiOutFmt);
			return -1;
		}
	}

	hdmi_ahb_write(ucId2,  0x36c5, 0x08); //  [7:6]: c1c0, 01=SMPTE 170M, 10=ITU-R BT.709. [5:4]: M1M0, 00=NoData, 01=4:3, 10=16:9. [3:0]: 1000=Same as picture aspect ratio. 1001=4:3, 1010=16:9
	hdmi_ahb_write(ucId2,  0x36c6, 0x00); //
	hdmi_ahb_write(ucId2,  0x36c7, 0x1f); //  vic 31/75: 1080P50. 31 for 16:9, 75 for 4:3. SMPTE 274M [2]

	hdmi_ahb_write(ucId2,  0x36c8, 0x00);
	hdmi_ahb_write(ucId2,  0x36c9, 0x00);
	hdmi_ahb_write(ucId2,  0x36ca, 0x00);
	hdmi_ahb_write(ucId2,  0x36cb, 0x00);
	hdmi_ahb_write(ucId2,  0x36cc, 0x00);
	hdmi_ahb_write(ucId2,  0x36cd, 0x00);
	hdmi_ahb_write(ucId2,  0x36ce, 0x00);
	hdmi_ahb_write(ucId2,  0x36cf, 0x00);

	hdmi_ahb_write(ucId2,  0x36df, 0xc0); //  bit[7]: Enable, bit[6]: Repeat.

	return 0;
}

//-------------------------------------------------------------------------
// Function name: hdmi_cal_freq()
// Description: Calculate the register setting for new freq.
//
//-------------------------------------------------------------------------
s32 hdmi_cal_freq(tFREQSET *ptCurSet, u8 slaveid)
{
	long long llFdcoMax = 5650000;  // Max of fDCO.
	long long llFdcoMin = 4860000;  // Min of fDCO.
	u32       llFreqOld = 156250;   // Default freq.
	u8        pucHsDivTbl [] = {11, 9, 7, 6, 5, 4};

	long long llFreqDiv, llFreqDivNew, llFreqDivHigh, llFreqDivLow;
	u64       fFreqRatio;

	u8        ucHsDiv, ucN1, ucHsDivNew, ucN1New;
	u32       unDivMax, unDivCurrent, i, llTmp, fN1Tmp;

	u32       unFreqDivHighOld, unFreqDivLowOld, unFreqKHzNew;
	u32       unFreqDivHigh32, unFreqDivLow32;

	const u32 bVerbose = 0;
	u32       bDataValid;

	llFreqOld        = (slaveid != 0x67) ?  297000 : 156250;
	ucHsDiv          = ptCurSet->ucHsDiv;
	ucN1             = ptCurSet->ucN1;
	unFreqDivHighOld = ptCurSet->unFreqHigh;
	unFreqDivLowOld  = ptCurSet->unFreqLow;
	unFreqKHzNew     = ptCurSet->unTargetFreqKHZ;

	if (ucN1 == 0) {
		ucN1 = 1;
	} else if ((ucN1 & 0x01) != 0) {
		ucN1 = ucN1 + 1;
	}

	llFreqDiv = (((long long)unFreqDivHighOld) << 32) | ((long long)unFreqDivLowOld);

	unDivMax = DO_DIV64(llFdcoMax, unFreqKHzNew);
	unDivCurrent = DO_DIV64(llFdcoMin, unFreqKHzNew) + 1;

	bDataValid   = 0;
	if (bVerbose) {
		LOG(LOG_INFO, "  HSDV_old =%2d",     ucHsDiv);
		LOG(LOG_INFO, "  N_old    =%2d",     ucN1);
		LOG(LOG_INFO, "  llFreqDiv=0x%llx",  llFreqDiv);
		LOG(LOG_INFO, "  unDivMax=%4d",      (u32)unDivMax);
		LOG(LOG_INFO, "  unDivCurrent=%4d",  (u32)unDivCurrent);
	}

	while (unDivCurrent <= unDivMax) {
		for (i = 0; i < 6; i++) {
			ucHsDivNew = pucHsDivTbl[i];
			llTmp = unDivCurrent/ucHsDivNew;
			fN1Tmp = unDivCurrent % ucHsDivNew;

			if (fN1Tmp == 0) {
				ucN1New = (u8)llTmp;
				if ((ucN1New == 1) | ((ucN1New % 2) == 0)) {
					bDataValid = 1;
				}
			}

			if (bDataValid == 1) {
				break;
			}
		}

		if (bDataValid == 1) {
			break;
		}

		unDivCurrent = unDivCurrent + 1;
	}

	fFreqRatio = DO_DIV64(unFreqKHzNew*10, llFreqOld);
	fFreqRatio = DO_DIV64(fFreqRatio*ucN1New*10, ucN1);
	fFreqRatio = DO_DIV64(fFreqRatio*ucHsDivNew*10, ucHsDiv);

	if (bVerbose) {
		LOG(LOG_INFO, "  llFreqOld=0x%llx", llFreqDiv);
		LOG(LOG_INFO, "punch @6 fFreqRatio=%lld", fFreqRatio);
		LOG(LOG_INFO, "  ucHsDivNew=%2d", ucHsDivNew);
		LOG(LOG_INFO, "  ucN1New=%2d", ucN1New);
	}


	llFreqDivNew = DO_DIV64(llFreqDiv*fFreqRatio, (1000));

	if (bVerbose) {
		LOG(LOG_INFO, "  llFreqDivNew=0x%llx", llFreqDivNew);
	}
	ucHsDivNew    = ucHsDivNew - 4;
	if (ucN1New == 1) {
		ucN1New = 0;
	} else if ((ucN1New & 1) == 0) {
		ucN1New = ucN1New - 1;
	}

	llFreqDivHigh   = llFreqDivNew >> 32;
	llFreqDivLow    = llFreqDivNew & (0xffffffff);
	unFreqDivHigh32 = (u32)llFreqDivHigh;
	unFreqDivLow32  = (u32)llFreqDivLow;

	if (bVerbose) {
		LOG(LOG_INFO, "  llFreqDivNew   =0x%lld", llFreqDivNew );
		LOG(LOG_INFO, "  ucHsDivNew     =%2d",    ucHsDivNew   );
		LOG(LOG_INFO, "  ucN1New        =%2d",    ucN1New      );
		LOG(LOG_INFO, "  unFreqDivHigh32=0x%x",   (unsigned int)unFreqDivHigh32);
		LOG(LOG_INFO, "  unFreqDivLow32 =0x%x",   (unsigned int)unFreqDivLow32);
	}

	ptCurSet->ucHsDiv         = ucHsDivNew;
	ptCurSet->ucN1            = ucN1New;
	ptCurSet->unFreqHigh      = unFreqDivHigh32;
	ptCurSet->unFreqLow       = unFreqDivLow32;

	return 0;
}

//-------------------------------------------------------------------------
// Function name: hdmi_set_phyfreq()
// Description:
// Parameters:
//   U32 unFreqKHz: Target frequency in KHz.
// Return:
//   Return positives if no error, otherwise, return negatives.
//
// Notes:
//   1. Si570 registers, 7-12 for 50ppm/20ppm,
//      13~18 for 7ppm devices freqdiv
//      (HS_DIV[2:0], N1[7:0], and RFREQ[37:0]).
//        Reg[7/13]: bit[7:5]=HS_DIV[2:0], bit[4:0]=N1[7:2]
//        Reg[8/14]: bit[7:6]=N1[1:0], bit[5:0]=RFREQ[37:32]
//        {Reg[9/15], Reg[10/16], Reg[11/17], Reg[12/18]}: RFREQ[31:0]
//      Refer to spec of si570 P.18 for the program procedure:
//        Finit=156.25MHz.
//        Read div, calc Fxtal, calc new div, re-program.
//      N1 should be 1, 2, 4, 6, ..., 128(1 or even values no more than 128).
//-------------------------------------------------------------------------
s32 hdmi_set_phyfreq(u32 unFreqKHz)
{
	u32 unI2CM1to, i, unDivHigh, unDivLow;
	u8  ucHsDiv, ucN1, ucSlaveId = 0x67, ucTmp;
	tFREQSET curFreqParams;
	const u32 bVerbose = 0;
	s32 res = 0;

	/// Route I2CM1 to HDMI GTH clock source(Si570).
	unI2CM1to = armcb_reg_get_int32(0x43C0001C);
	armcb_reg_set_int32(0x43C0001C, 0x0);

	/// Load current setting in NVM to RAM.
	res = hdmi_i2c_write(ucSlaveId, 135, 0x1);
	if (res < 0) {
		ucSlaveId = 0x55;
		res = hdmi_i2c_write(ucSlaveId, 135, 0x1);
		LOG(LOG_INFO, "ucSlaveId(%x) new res(%d)", ucSlaveId, res);
	}

	/// Read out current setting first.
	if (bVerbose) {
		LOG(LOG_INFO, "Previous setting:");
	}
	ucTmp  = hdmi_i2c_read(ucSlaveId, 13);
	if (bVerbose) {
		LOG(LOG_INFO, "Reg[013]=0x%x:", ucTmp);
	}
	ucHsDiv = ((ucTmp >> 5) & 0x7) + 4; // + 4: See spec of si570.
	ucN1      = (ucTmp << 2) & 0x7c;

	ucTmp  = hdmi_i2c_read(ucSlaveId, 14);
	if (bVerbose) {
		LOG(LOG_INFO, "Reg[14]=0x%x:", ucTmp);
	}
	ucN1 = ucN1 + ((ucTmp >> 6) & 0x03);
	unDivHigh = (u32)(ucTmp & 0x3f);

	unDivLow = 0;
	ucTmp    = hdmi_i2c_read(ucSlaveId, 15);
	if (bVerbose) {
		LOG(LOG_INFO, "Reg[15]=0x%x:", ucTmp);
	}
	unDivLow = unDivLow | ((((u32)ucTmp) << 24) & 0xff000000);

	ucTmp    = hdmi_i2c_read(ucSlaveId, 16);
	if (bVerbose) {
		LOG(LOG_INFO, "Reg[16]=0x%x:", ucTmp);
	}
	unDivLow = unDivLow | ((((u32)ucTmp) << 16) & 0x00ff0000);

	ucTmp    = hdmi_i2c_read(ucSlaveId, 17);
	if (bVerbose) {
		LOG(LOG_INFO, "Reg[17]=0x%x:", ucTmp);
	}
	unDivLow = unDivLow | ((((u32)ucTmp) << 8) & 0x0000ff00);

	ucTmp = hdmi_i2c_read(ucSlaveId, 18);
	if (bVerbose) {
		LOG(LOG_INFO, "Reg[18]=0x%x:", ucTmp);
	}
	unDivLow = unDivLow | ((((u32)ucTmp) << 0) & 0x000000ff);

	hdmi_i2c_write(ucSlaveId, 135, 0x0);

	/// Calculate the new settings.
	curFreqParams.ucHsDiv        = ucHsDiv;
	curFreqParams.ucN1           = ucN1;
	curFreqParams.unFreqHigh     = unDivHigh;
	curFreqParams.unFreqLow      = unDivLow;
	curFreqParams.unTargetFreqKHZ= unFreqKHz;

	hdmi_cal_freq(&curFreqParams, ucSlaveId);

	LOG(LOG_DEBUG, "ucHsDiv=%d, ucN1=%d, unFreqHigh=%d, unFreqLow=0x%x, unTargetFreqKHZ=%d",
		curFreqParams.ucHsDiv, curFreqParams.ucN1, curFreqParams.unFreqHigh, curFreqParams.unFreqLow, curFreqParams.unTargetFreqKHZ);

	if (curFreqParams.unFreqHigh > 0x3F) {
		LOG(LOG_ERR, "Calculation for new freq setting error! HS_DIV or N1 should be fine tuned. unFreqHigh=%d", curFreqParams.unFreqHigh);
		return -1;
	}

	ucHsDiv   = curFreqParams.ucHsDiv;
	ucN1      = curFreqParams.ucN1;
	unDivHigh = curFreqParams.unFreqHigh;
	unDivLow  = curFreqParams.unFreqLow;
	/// Set new values to change frequency.
	ucTmp = hdmi_i2c_read(ucSlaveId, 137);
	hdmi_i2c_write(ucSlaveId, 137, ucTmp | 0x10);

	msleep(1); // Delay time enough?

	ucTmp = (u8)(((ucHsDiv << 5) & 0xE0) | ((ucN1 >> 2) & 0x1F));
	hdmi_i2c_write(ucSlaveId, 13, ucTmp);

	ucTmp = (u8)((((ucN1 & 0x3) << 6) & 0xC0) | (unDivHigh & 0x3F));
	hdmi_i2c_write(ucSlaveId, 14, ucTmp);

	ucTmp = (u8)(unDivLow >> 24);
	hdmi_i2c_write(ucSlaveId, 15, ucTmp);

	ucTmp = (u8)(unDivLow >> 16);
	hdmi_i2c_write(ucSlaveId, 16, ucTmp);

	ucTmp = (u8)(unDivLow >> 8);
	hdmi_i2c_write(ucSlaveId, 17, ucTmp);

	ucTmp = (u8)(unDivLow >> 0);
	hdmi_i2c_write(ucSlaveId, 18, ucTmp);

	if (bVerbose) {
		LOG(LOG_INFO, "New setting:");
		for (i = 13; i <= 18; i++) {
			ucTmp = hdmi_i2c_read(ucSlaveId, i);
			LOG(LOG_INFO, "Reg[%2d]=0x%2x", (u32)i, (u32)ucTmp);
		}
	}

	/// Launch new setting to work.
	ucTmp = hdmi_i2c_read(ucSlaveId, 137);
	hdmi_i2c_write(ucSlaveId, 137, (ucTmp&0xEF));

	hdmi_i2c_write(ucSlaveId, 135, 0x40);

	/// Re-check the setting.
	if (bVerbose) {
		msleep(1);
		LOG(LOG_INFO, "  Final setting(for check with new setting):");
		for (i = 13; i <= 18; i++) {
			ucTmp = hdmi_i2c_read(ucSlaveId, i);
			LOG(LOG_INFO, "Reg[%2d]=0x%2x", (u32)i, (u32)ucTmp);
		}
	}

	/// Route I2CM1 to original slave.
	armcb_reg_set_int32(0x43C0001C, unI2CM1to);
	return 0;
}

/***************************************************************************
* Function name: armcb_hdmi_init()
* Description  : HDMI output path initialization.
* Parameters   :
*   u32 unVinMode:  Video input mode, 1280x720 or 1920x1080.
*   u32 unDispMode: Video output mode, 1280x720 or 1920x1080.
* Return       : None.
* Notes        : Originated from hdmiphy_1080p60_en().
***************************************************************************/
s32 armcb_hdmi_init(u32 uiVinMode, u32 uiDispMode, u32 unHdmiOutFmt)
{
	u8  ucId1 = FPGA_HDMIPHY_IIC, ucTmp;
	u32 unVinHActive, unVinVActive;
	const u32 bVerbose = 0;

	/// 1. Set htotal/vtotal for video input.
	if ((uiVinMode == RES720P30FPS) || (uiVinMode == RES720P60FPS)) {
		unVinHActive = 1280;
		unVinVActive = 720;
	} else if ((uiVinMode == RES1080P30FPS) ||
			(uiVinMode == RES1080P60FPS) ||
			(uiVinMode == RES1080I50FPS) ||
			(uiVinMode == RES1080P50FPS)) {
		unVinHActive = 1920;
		unVinVActive = 1080;
	} else {
		LOG(LOG_INFO, "Unsupported Video input mode! %d", uiVinMode);
		return -1;
	}

	disp_vdma_reset();
	/// Reset sn65dp159 before changing HDMI PHY clock.
	hdmi_i2c_write(ucId1, 0x28, 0x00); // Enable i2c access from PC.
	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0x09, 0x0a);
	msleep(1);
	/// 3. Set proper clock for HDMI PHY according to display resolution.
	if ((uiDispMode == RES720P60FPS) || (uiDispMode == RES720P30FPS) || (uiDispMode == RES1080I50FPS)) {
		hdmi_set_phyfreq(74250);
		LOG(LOG_INFO, "HDMI clock set to 74.25MHz!");
	} else if ((uiDispMode == RES1080P60FPS) || (uiDispMode == RES1080P50FPS)) {
		hdmi_set_phyfreq(148500);
		LOG(LOG_INFO, "HDMI clock set to 148.5MHz!");
	} else {
		LOG(LOG_INFO, "Unsupported display mode! %d", uiDispMode);
		return -1;
	}
	msleep(1);  // For clock to settle down.

	/// 4. Reset HDMI PHY after clock changed.
	hdmi_i2c_write(ucId1, 0x01, 0x20);      // Set gth rx reset.
	hdmi_i2c_write(ucId1, 0x01, 0x60);      // Set gth tx reset.
	hdmi_i2c_write(ucId1, 0x01, 0x61);      // Reset whole system.

	hdmi_i2c_write(ucId1, 0x08, 0x55);      // Output CK of 1/2 Fbit on data channels. Output CK of 1/40 Fbit on clock channel.
	hdmi_i2c_write(ucId1, 0x09, 0x55);
	hdmi_i2c_write(ucId1, 0x0a, 0x55);
	hdmi_i2c_write(ucId1, 0x0b, 0x55);
	hdmi_i2c_write(ucId1, 0x03, 0xd5);

	msleep(1);

	hdmi_i2c_write(ucId1, 0x01, 0x60);       // Release reset.
	msleep(1);
	hdmi_i2c_write(ucId1, 0x01, 0x20);       // Release gth tx reset.
	msleep(1);
	hdmi_i2c_write(ucId1, 0x01, 0x00);       // Release gth rx reset.
	msleep(1);                      // Wait internal reset to complete.

	hdmi_i2c_write(ucId1, 0x28, 0x00);       // Enable i2c access from PC.

	video_io_enable(unVinHActive, unVinVActive, (u32)uiDispMode);
	hdmi_phyhw_init(unHdmiOutFmt);
	if (uiDispMode == RES1080P60FPS) {
		hdmi_phy_set_fmt_avi1080p60(unHdmiOutFmt);
	} else if (uiDispMode == RES720P60FPS) {
		hdmi_phy_set_fmt_avi720p60(unHdmiOutFmt);
	} else if (uiDispMode == RES720P30FPS) {
		hdmi_phy_set_fmt_avi720p30(unHdmiOutFmt);
	} else if (uiDispMode == RES1080I50FPS) {
		hdmi_phy_set_fmt_avi1080i50(unHdmiOutFmt);
	} else if (uiDispMode == RES1080P50FPS) {
		hdmi_phy_set_fmt_avi1080p50(unHdmiOutFmt);
	} else {
		LOG(LOG_INFO, "Unsupported display mode! %d", uiDispMode);
		return -2;
	}

	hdmi_i2c_write(ucId1, 0x00,  0x1e);   // Select HDMI-TX data source.

	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0x09,  0x06);  // Bit2=1: Disable auto powndown mode when HPD_SNK is low.
	msleep(1);
	/// Setting of SN65DP159.
	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0xff, 0x00);
	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0x0b, 0x99);   // Reg[0x0b]: [7:6]=2'b10, SLEW_CTRL. [4:3]=2'b11, TX_TERM_CTL, 75~150.   [1]=1'b1, TMDS clk 1/40 of bit clk. [0]=1'b1, Disable DDC training.

	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0x0a, 0x3d);   // Auto redriver/retimer mode.

	hdmi_i2c_write(ucId1, 0x03,  0xff);               // Select output from buffering logic.
	/// Setting of TV.
	hdmi_i2c_write(ucId1, 0x28, 0x00);    // Enable I2C access to TV CSDC.

	if (bVerbose) {
		hdmi_i2c_regdump(FPGA_SN65DP159_IIC, 0x09, 7);
		hdmi_i2c_regdump(FPGA_SN65DP159_IIC, 0x10, 8);
	}

	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0x15, 0x10);   // Clear BERT_CNT
	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0x16, 0xe0);   // Enable TMDS disparity check on ch1-3(ch0 is clock, excluded)
	if (bVerbose) {
		hdmi_i2c_regdump(FPGA_SN65DP159_IIC, 0x1a, 6);
	}

	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0xff, 0x01);
	if (bVerbose) {
		ucTmp = hdmi_i2c_read(FPGA_SN65DP159_IIC, 0x00);
		LOG(LOG_INFO, " Reg[0x00]=0x%x on 0x%x", ucTmp, (FPGA_SN65DP159_IIC << 1)); // bit0: 1=Scrambled data captured.
	}
	hdmi_i2c_write(FPGA_SN65DP159_IIC, 0xff, 0x00);

	msleep(1);
	return 0;
}
EXPORT_SYMBOL(armcb_hdmi_init);

void __iomem *armcb_hdmi_get_apb2_base(void)
{
	return p_hdmi_info->apb2_base_addr;
}

void __iomem *armcb_hdmi_get_vdma_base(void)
{
	return p_hdmi_info->vdma_base_addr;
}

void __iomem *armcb_hdmi_get_vdmasensor_base(void)
{
	return p_hdmi_info->vdmasensor_base_addr;
}

void __iomem *armcb_hdmi_get_vdmaisp_base(void)
{
	return p_hdmi_info->vdmaisp_base_addr;
}

void __iomem *armcb_hdmi_get_xvtc_base(void)
{
	return p_hdmi_info->xvtc_base_addr;
}

u32 armcb_reg_get_int32(u32 phy_addr)
{
	void __iomem *virt_addr;
	u32 reg_val = 0;

	/* ioremap: convert phy addr to virtual addr*/
	virt_addr = (void __iomem *)ioremap(phy_addr, 4);
	reg_val = *(u32 *)virt_addr;
	iounmap((void __iomem*)virt_addr);

	return reg_val;
}

void armcb_reg_set_int32(u32 phy_addr, u32 value)
{
	void __iomem *virt_addr;

	/* ioremap: convert phy addr to virtual addr*/
	virt_addr = (void __iomem *)ioremap(phy_addr, 4);
	/* write value to virtual addr */
	*(u32 *)virt_addr = value;
	/*read back the value of virtual addr */
	iounmap((void __iomem*)virt_addr);
}


u32 armcb_hdmi_apb2_reg_get(u32 offset)
{
	volatile void __iomem *g_virt_addr = NULL;
	u32 regval = 0;

	g_virt_addr = armcb_hdmi_get_apb2_base();
	regval = readl(g_virt_addr + offset);

	return regval;
}

void armcb_hdmi_apb2_reg_set(u32 offset, u32 value)
{
	volatile void __iomem *g_virt_addr = NULL;

	g_virt_addr = armcb_hdmi_get_apb2_base();
	writel(value, g_virt_addr + offset);
}


static const struct i2c_device_id armcb_hdmi_id[] = {
	{"hdmi-i2c", FPGA_SN65DP159_IIC},
	{},
};

static const struct of_device_id armcb_hdmi_of_match[] = {
	{.compatible = "armcb,hdmi"},
	{},
};

static int armcb_hdmi_open(struct inode *node, struct file *file)
{
	return 0;
}

static long  armcb_hdmi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int  armcb_hdmi_release(struct inode *node, struct file *file)
{
	return 0;
}

static struct file_operations  hdmi_dev_fops = {
	.owner          = THIS_MODULE,
	.open           = armcb_hdmi_open,
	.unlocked_ioctl = armcb_hdmi_ioctl,
	.release        = armcb_hdmi_release,
};

static struct miscdevice  hdmi_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "armcb-hdmi",
	.fops = &hdmi_dev_fops,
};

static int armcb_hdmi_i2c_probe(struct i2c_client *client)
{

	int  res          = 0;

	if (!client) {
		LOG(LOG_ERR, "armcb hdmi client is NULL");
		return -EINVAL;
	}

	pclient_i2c1 = client;

	if (misc_register(& hdmi_misc)) {
		LOG(LOG_ERR, "failed to register ispmem driver.");
		return -ENODEV;
	}

	p_hdmi_info = devm_kzalloc(&client->dev, sizeof(*p_hdmi_info), GFP_KERNEL);
	if (!p_hdmi_info) {
		LOG(LOG_ERR, "failed to alloc p_hdmi_info.");
		misc_deregister(&hdmi_misc);
		return -ENOMEM;
	}

	p_hdmi_info->apb2_base_addr = devm_ioremap(&client->dev, APB2_REG_BASE, APB2_REG_SIZE);
	if (!p_hdmi_info->apb2_base_addr) {
		LOG(LOG_WARN, "failed to ioremap apb2 register region.");
	}

	p_hdmi_info->vdma_base_addr = devm_ioremap(&client->dev, VDMA_REG_DISP_BASE, VDMA_REG_DISP_SIZE);
	if (!p_hdmi_info->vdma_base_addr) {
		LOG(LOG_WARN, "failed to ioremap vdma register region.");
	}

	p_hdmi_info->vdmasensor_base_addr = devm_ioremap(&client->dev, VDMA_REG_SENSOR_IN_BASE, VDMA_REG_SENSOR_IN_SIZE);
	if (!p_hdmi_info->vdmasensor_base_addr) {
		LOG(LOG_WARN, "failed to ioremap vdma_sensor register region.");
	}

	p_hdmi_info->vdmaisp_base_addr = devm_ioremap(&client->dev, VDMA_REG_ISP_OUT_BASE, VDMA_REG_ISP_OUT_SIZE);
	if (!p_hdmi_info->vdmaisp_base_addr) {
		LOG(LOG_WARN, "failed to ioremap vdma_isp register region.");
	}

	p_hdmi_info->xvtc_base_addr = devm_ioremap(&client->dev, XVTC_REG_BASE, XVTC_REG_SIZE);
	if (!p_hdmi_info->xvtc_base_addr) {
		LOG(LOG_WARN, "failed to ioremap xvtc register region.");
	}

	p_hdmi_info->pddev = &client->dev;
	LOG(LOG_INFO, "armcb hdmi probe success, pclient = 0x%p", client);

	return res;

}

void armcb_hdmi_i2c_remove(struct i2c_client *client)
{
	struct armcb_hdmi_subdev *phdmi_sd = i2c_get_clientdata(client);

	if (phdmi_sd) {
		kfree(phdmi_sd);
	} else {
		LOG(LOG_ERR, "phdmi_sd is NULL !");
	}

	misc_deregister(&hdmi_misc);
	return;
}

static struct i2c_driver armcb_hdmi_i2c_driver = {
	.probe    = armcb_hdmi_i2c_probe,
	.remove   = armcb_hdmi_i2c_remove,
	.id_table = armcb_hdmi_id,
	.driver = {
		.name = "armcb-hdmi",
		.of_match_table = of_match_ptr(armcb_hdmi_of_match),
	},
};

#ifndef ARMCB_CAM_KO
static int __init armcb_hdmi_subdev_init(void)
{
	int res = 0;

	res = i2c_add_driver(&armcb_hdmi_i2c_driver);
	if (res) {
		LOG(LOG_ERR, "i2c_add_driver failed res(%d)", res);
	}

	return res;
}

static void __exit armcb_hdmi_subdev_exit(void)
{
	i2c_del_driver(&armcb_hdmi_i2c_driver);
}

module_init(armcb_hdmi_subdev_init);
module_exit(armcb_hdmi_subdev_exit);

MODULE_DEVICE_TABLE(i2c, armcb_hdmi_id);
MODULE_DEVICE_TABLE(of, armcb_hdmi_of_match);
MODULE_AUTHOR("Armchina Inc.");
MODULE_DESCRIPTION("Armchina hdmi driver");
MODULE_LICENSE("GPL v2");
#else
static void *g_instance = NULL;

void *armcb_get_system_hdmi_driver_instance(void)
{
	if (i2c_add_driver(&armcb_hdmi_i2c_driver) < 0) {
		LOG(LOG_ERR, "register hdmi driver failed.\n");
		return NULL;
	}

	g_instance = (void *)&armcb_hdmi_i2c_driver;

	return g_instance;
}

void armcb_system_hdmi_driver_destroy(void)
{
	if (g_instance) {
		i2c_del_driver((struct i2c_driver *)g_instance);
	}
}
#endif

