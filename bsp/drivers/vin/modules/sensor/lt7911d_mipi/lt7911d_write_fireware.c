// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for lt7911d write fireware.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  zhengzequn <zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <linux/v4l2-dv-timings.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <linux/io.h>
#include "../camera.h"
#include "../sensor_helper.h"
#include "lt7911d_fireware.h"

unsigned char HdcpKey[286];

#define V4L2_IDENT_SENSOR  0x1605
#define SENSOR_NAME "lt7911d_mipi"

static bool lt7911d_checkchipid(struct v4l2_subdev *sd)
{
	unsigned int SENSOR_ID = 0;
	data_type rdval = 0;
	int cnt = 0;

	sensor_write(sd, 0xff, 0xa0); /* FF save Bank addr */
	sensor_read(sd, 0x00, &rdval);
	SENSOR_ID |= (rdval << 8);
	sensor_read(sd, 0x01, &rdval);
	SENSOR_ID |= (rdval);
	sensor_print("V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(sd, 0x00, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(sd, 0x01, &rdval);
		SENSOR_ID |= (rdval);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt, SENSOR_ID);
		cnt++;
	}

	if (SENSOR_ID != V4L2_IDENT_SENSOR)
		return false;

	return true;
}

static void lt7911d_configpara(struct v4l2_subdev *sd)
{
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0x5A, 0x82);
	sensor_write(sd, 0x5E, 0xC0);
	sensor_write(sd, 0x58, 0x00);
	sensor_write(sd, 0x59, 0x51);
	sensor_write(sd, 0x5A, 0x92);
	sensor_write(sd, 0x5A, 0x82);
}

static void lt7911d_blockerase(struct v4l2_subdev *sd)
{
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0x5A, 0x86);
	sensor_write(sd, 0x5A, 0x82);
	sensor_write(sd, 0x5B, 0x00);
	sensor_write(sd, 0x5C, 0x00);
	sensor_write(sd, 0x5D, 0x00);
	sensor_write(sd, 0x5A, 0x83);
	sensor_write(sd, 0x5A, 0x82);
	usleep_range(1000000, 1000100);
}

static bool lt7911d_checkversion(struct v4l2_subdev *sd)
{
#if VIN_TRUE
	unsigned char version_reg[6], version_firmware[6];
	unsigned int version_firmware_position = 0x200b;
	unsigned int i;
	data_type rdval = 0;
	bool ret = true;

	sensor_write(sd, 0xFF, 0xd2);
	usleep_range(5000, 5100);
	sensor_read(sd, 0x0C, &rdval);
	version_reg[0] = rdval;
	sensor_read(sd, 0x0D, &rdval);
	version_reg[1] = rdval;
	sensor_read(sd, 0x0E, &rdval);
	version_reg[2] = rdval;
	sensor_read(sd, 0x0F, &rdval);
	version_reg[3] = rdval;
	sensor_read(sd, 0x10, &rdval);
	version_reg[4] = rdval;
	sensor_read(sd, 0x11, &rdval);
	version_reg[5] = rdval;

	memcpy(version_firmware, lt7911d_firmwaredata + version_firmware_position, 6);

	for (i = 0; i < 6; i++) {
		//vin_print("%d: 0x%.2x/0x%.2x\n", i, version_reg[i], version_firmware[i]);
		if (version_reg[i] != version_firmware[i])
			ret = false;
	}

	return ret;
#else
	unsigned char version_flash[6], version_firmware[6];
	unsigned char firmware_page[32];
	unsigned int version_firmware_position = 0x200b;
	bool ret = true;
	unsigned int startaddr;
	unsigned short npage, j, i;
	unsigned char npagelen = 0;
	unsigned char addr[3] = {0};
	data_type rval;

	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0xFF, 0x90);
	sensor_write(sd, 0x02, 0xdf);
	sensor_write(sd, 0x02, 0xff);
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0x5a, 0x86);
	sensor_write(sd, 0x5a, 0x82);

	startaddr = 0x002000;
	addr[0] = (startaddr & 0xFF0000) >> 16;
	addr[1] = (startaddr & 0xFF00) >> 8;
	addr[2] = startaddr & 0xFF;

	npagelen = 16;
	npage = 2;

	for (i = 0; i < npage; i++) {
		sensor_write(sd, 0x5E, 0x6f);
		sensor_write(sd, 0x5A, 0xA2);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x5B, addr[0]);
		sensor_write(sd, 0x5C, addr[1]);
		sensor_write(sd, 0x5D, addr[2]);
		sensor_write(sd, 0x5A, 0x92);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x58, 0x01);

		for (j = 0; j < npagelen; j++) {
			 sensor_read(sd, 0x5F, &rval);
			 firmware_page[i*16 + j] = rval;
		}
		startaddr += 16;
		addr[0] = (startaddr & 0xFF0000) >> 16;
		addr[1] = (startaddr & 0xFF00) >> 8;
		addr[2] = startaddr & 0xFF;
	}
	sensor_write(sd, 0x5a, 0x8a);
	sensor_write(sd, 0x5a, 0x82);

	memcpy(version_firmware, lt7911d_firmwaredata + version_firmware_position, 6);
	memcpy(version_flash, firmware_page + 0xb, 6);

	for (i = 0; i < 6; i++) {
		//vin_print("%d: 0x%.2x/0x%.2x\n", i, version_flash[i], version_firmware[i]);
		if (version_flash[i] != version_firmware[i])
			ret = false;
	}

	return ret;
#endif
}

static void lt7911d_savehdcpkeyfromflash(struct v4l2_subdev *sd)
{
	unsigned int startaddr;
	unsigned short npage, i, j;
	unsigned char npagelen = 0;
	unsigned char addr[3] = {0};
	data_type rval;

	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0xFF, 0x90);
	sensor_write(sd, 0x02, 0xdf);
	sensor_write(sd, 0x02, 0xff);
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0x5a, 0x86);
	sensor_write(sd, 0x5a, 0x82);

	startaddr = 0x006000;
	addr[0] = (startaddr & 0xFF0000) >> 16;
	addr[1] = (startaddr & 0xFF00) >> 8;
	addr[2] = startaddr & 0xFF;
	npage = 18;
	npagelen = 16;

	for (i = 0; i < npage; i++) {
		sensor_write(sd, 0x5E, 0x6f);
		sensor_write(sd, 0x5A, 0xA2);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x5B, addr[0]);
		sensor_write(sd, 0x5C, addr[1]);
		sensor_write(sd, 0x5D, addr[2]);
		sensor_write(sd, 0x5A, 0x92);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x58, 0x01);

		if (i == 17) {
			npagelen = 14;
		}

		for (j = 0; j < npagelen; j++) {
			sensor_read(sd, 0x5F, &rval);
			HdcpKey[i*16 + j] = rval;
			//vin_print("read HdcpKey%d is 0x%x\n", i*16 + j, rval);
		}
		startaddr += 16;
		addr[0] = (startaddr & 0xFF0000) >> 16;
		addr[1] = (startaddr & 0xFF00) >> 8;
		addr[2] = startaddr & 0xFF;
	}
	sensor_write(sd, 0x5a, 0x8a);
	sensor_write(sd, 0x5a, 0x82);
}

static void lt7911d_writefirmwaretoflash(struct v4l2_subdev *sd)
{
	unsigned int startaddr;
	unsigned short npage, i, j;
	unsigned char npagelen = 0;
	unsigned char addr[3] = {0};
	unsigned int firmware_len;
	data_type wval;

	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0xFF, 0x90);
	sensor_write(sd, 0x02, 0xdf);
	sensor_write(sd, 0x02, 0xff);
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0x5a, 0x86);
	sensor_write(sd, 0x5a, 0x82);

	startaddr = 0x000000;
	addr[0] = (startaddr & 0xFF0000) >> 16;
	addr[1] = (startaddr & 0xFF00) >> 8;
	addr[2] = startaddr & 0xFF;

	firmware_len = ARRAY_SIZE(lt7911d_firmwaredata);

	if (firmware_len%16) {
		npage = firmware_len/16 + 1;//firmware_len is the lenth of the firmware.
	} else {
		npage = firmware_len/16;
	}
	npagelen = 16;

	for (i = 0; i < npage; i++) {
		sensor_write(sd, 0x5a, 0x86);
		sensor_write(sd, 0x5a, 0x82);
		sensor_write(sd, 0x5E, 0xef);
		sensor_write(sd, 0x5A, 0xA2);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x58, 0x01);

		if ((firmware_len - i * 16) < 16) {
			npagelen = firmware_len - i * 16;
		}

		for (j = 0; j < npagelen; j++) {
			wval = lt7911d_firmwaredata[i*16 + j];
			sensor_write(sd, 0x59, wval);//use 0xff as insufficient data if datelen%16 is not zero.
		}
		sensor_write(sd, 0x5B, addr[0]);
		sensor_write(sd, 0x5C, addr[1]);
		sensor_write(sd, 0x5D, addr[2]);
		sensor_write(sd, 0x5E, 0xE0);
		sensor_write(sd, 0x5A, 0x92);
		sensor_write(sd, 0x5A, 0x82);

		startaddr += 16;
		addr[0] = (startaddr & 0xFF0000) >> 16;
		addr[1] = (startaddr & 0xFF00) >> 8;
		addr[2] = startaddr & 0xFF;
	}
	sensor_write(sd, 0x5a, 0x8a);
	sensor_write(sd, 0x5a, 0x82);
}

static void lt7911d_writehdcpkeytoflash(struct v4l2_subdev *sd)
{
	unsigned int startAddr;
	unsigned short npage, i, j;
	unsigned char npagelen = 0;
	unsigned char addr[3] = {0};

	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0xFF, 0x90);
	sensor_write(sd, 0x02, 0xdf);
	sensor_write(sd, 0x02, 0xff);
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0x5a, 0x86);
	sensor_write(sd, 0x5a, 0x82);

	startAddr = 0x006000;
	addr[0] = (startAddr & 0xFF0000) >> 16;
	addr[1] = (startAddr & 0xFF00) >> 8;
	addr[2] = startAddr & 0xFF;

	npage = 18;
	npagelen = 16;

	for (i = 0; i < npage; i++) {
		sensor_write(sd, 0x5a, 0x86);
		sensor_write(sd, 0x5a, 0x82);
		sensor_write(sd, 0x5E, 0xef);
		sensor_write(sd, 0x5A, 0xA2);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x58, 0x01);

		if (i == 17) {
			npagelen = 14;
		}

		for (j = 0; j < npagelen; j++) {
			sensor_write(sd, 0x59, HdcpKey[i*16 + j]);//use 0xff as insufficient data if datelen%16 is not zero.
		}

		if (npagelen == 14) {
			sensor_write(sd, 0x59, 0xFF);
			sensor_write(sd, 0x59, 0xFF);
		}

		sensor_write(sd, 0x5B, addr[0]);
		sensor_write(sd, 0x5C, addr[1]);
		sensor_write(sd, 0x5D, addr[2]);
		sensor_write(sd, 0x5E, 0xE0);
		sensor_write(sd, 0x5A, 0x92);
		sensor_write(sd, 0x5A, 0x82);

		startAddr += 16;
		addr[0] = (startAddr & 0xFF0000) >> 16;
		addr[1] = (startAddr & 0xFF00) >> 8;
		addr[2] = startAddr & 0xFF;
	}
	sensor_write(sd, 0x5a, 0x8a);
	sensor_write(sd, 0x5a, 0x82);
}

static void lt7911d_readfirmwarefromflash(struct v4l2_subdev *sd, unsigned char *readfirmware, unsigned int firmware_len)
{
	unsigned int startaddr;
	unsigned short npage, i, j;
	unsigned char npagelen = 0;
	unsigned char addr[3] = {0};
	data_type rval;

	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0xEE, 0x01);
	sensor_write(sd, 0xFF, 0x90);
	sensor_write(sd, 0x02, 0xdf);
	sensor_write(sd, 0x02, 0xff);
	sensor_write(sd, 0xFF, 0x80);
	sensor_write(sd, 0x5a, 0x86);
	sensor_write(sd, 0x5a, 0x82);

	startaddr = 0x000000;
	addr[0] = (startaddr & 0xFF0000) >> 16;
	addr[1] = (startaddr & 0xFF00) >> 8;
	addr[2] = startaddr & 0xFF;

	if (firmware_len%16) {
		npage = firmware_len/16 + 1;//firmware_len is the lenth of the firmware .
	} else {
		npage = firmware_len/16;
	}

	npagelen = 16;

	for (i = 0; i < npage; i++) {
		sensor_write(sd, 0x5E, 0x6f);
		sensor_write(sd, 0x5A, 0xA2);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x5B, addr[0]);
		sensor_write(sd, 0x5C, addr[1]);
		sensor_write(sd, 0x5D, addr[2]);
		sensor_write(sd, 0x5A, 0x92);
		sensor_write(sd, 0x5A, 0x82);
		sensor_write(sd, 0x58, 0x01);

		if ((firmware_len - i * 16) < 16) {
			npagelen = firmware_len - i * 16;
		}

		for (j = 0; j < npagelen; j++) {
			 sensor_read(sd, 0x5F, &rval);
			 readfirmware[i*16 + j] = rval;
		}
		startaddr += 16;
		addr[0] = (startaddr & 0xFF0000) >> 16;
		addr[1] = (startaddr & 0xFF00) >> 8;
		addr[2] = startaddr & 0xFF;
	}
	sensor_write(sd, 0x5a, 0x8a);
	sensor_write(sd, 0x5a, 0x82);
}

static bool lt7911d_compare_fireware(unsigned char *readfirmware, unsigned int firmware_len)
{
	unsigned short len;
	bool ret = true;
	for (len = 0; len < firmware_len; len++) {
		if (readfirmware[len] != lt7911d_firmwaredata[len]) {
			ret = false;
		}
	}
	return ret;
}

int lt7911d_fireware_upgrade(struct v4l2_subdev *sd)
{
	unsigned char *readfirmware;
	unsigned int firmware_len;

	if (lt7911d_checkchipid(sd)) {
		sensor_print("%s online\n", __func__);

		/* check if need to update firmware */
#if VIN_FALSE
		lt7911d_configpara(sd);

		firmware_len = ARRAY_SIZE(lt7911d_firmwaredata);
		vin_print("fireware len is %d\n", firmware_len);
		readfirmware = kzalloc(firmware_len * sizeof(unsigned char), GFP_KERNEL);
		if (!readfirmware) {
			sensor_err("malloc readfirmware failed!\n");
			return -ENOMEM;
		}

		lt7911d_readfirmwarefromflash(sd, readfirmware, firmware_len);
		if (lt7911d_compare_fireware(readfirmware, firmware_len)) {
			sensor_print("%s no need to update firmware\n", sd->name);
			kfree(readfirmware);
			return -1;
		} else {
			sensor_print("%s need to update firmware\n", sd->name);
		}
#else
		if (lt7911d_checkversion(sd)) {
			sensor_print("%s no need to update firmware\n", sd->name);
			return -1;
		} else {
			sensor_print("%s need to update firmware\n", sd->name);
		}

		lt7911d_configpara(sd);

		firmware_len = ARRAY_SIZE(lt7911d_firmwaredata);
		vin_print("fireware len is %d\n", firmware_len);
		readfirmware = kzalloc(firmware_len * sizeof(unsigned char), GFP_KERNEL);
		if (!readfirmware) {
			sensor_err("malloc readfirmware failed!\n");
			return -ENOMEM;
		}
#endif
		/* update firmware */
		lt7911d_savehdcpkeyfromflash(sd);
		lt7911d_blockerase(sd);
		lt7911d_writefirmwaretoflash(sd);
		lt7911d_writehdcpkeytoflash(sd);

		/* check if update firmware success */
		lt7911d_readfirmwarefromflash(sd, readfirmware, firmware_len);
		if (lt7911d_compare_fireware(readfirmware, firmware_len)) {
			sensor_print("%s success\n", __func__);
		} else {
			sensor_err("%s Fail\n", __func__);
		}

		kfree(readfirmware);
		return 0;
	} else {
		sensor_err("%s offline\n", __func__);
		return -1;
	}
}
