/*
 * A V4L2 driver for nvp6158c yuv cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn<zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/string.h>
#include <linux/delay.h>

//#include "eq_common.h"
#include "video.h"
#include "video_auto_detect.h"
#include "coax_protocol.h"
//#include "acp.h"
#include "video_eq.h"
#include "../sensor_helper.h"
#define _ENABLE_DET_DEBOUNCE_
#define SENSOR_NAME "nvp6158"
/*******************************************************************************
 * extern variable
 *******************************************************************************/
extern unsigned int nvp6158_cnt;
extern int chip_nvp6158_id[4];
extern unsigned int g_vloss;
extern unsigned int nvp6158_iic_addr[4];
unsigned char g_ch_video_fmt[16] = {[0 ... 15] = 0xFF};	// save user's video format
extern unsigned char det_mode[16];
extern unsigned int gCoaxFirmUpdateFlag[16];

unsigned char nvp6158_motion_sens_tbl[8] = {0xe0, 0xc8, 0xa0, 0x98, 0x78, 0x68, 0x50, 0x48};
unsigned char ch_mode_status[16] = {[0 ... 15] = 0xff};
unsigned char ch_vfmt_status[16] = {[0 ... 15] = 0xff};
#ifdef _ENABLE_DET_DEBOUNCE_
NVP6158_INFORMATION_S   s_raptor3_vfmts;
#endif

void nvp6158_dump_reg(unsigned char ch, unsigned char bank)
{
	int tmp = 0;
	int i = 0, j = 0;

	sensor_dbg("***************IIC ADDR 0x%02x - CH[%02d] *****************\r\n", nvp6158_iic_addr[ch/4], ch);
	sensor_dbg("***************Chip[0x%02x] Bank[0x%x]*****************\r\n", nvp6158_iic_addr[ch/4], bank);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, bank);
	for (i = 0; i  <=  0xF; i++) {
		if (i == 0)
			sensor_dbg("     0x%02x ", i);
		else if (i == 0xF)
			sensor_dbg("0x%02x\r\n", i);
		else
			sensor_dbg("0x%02x ", i);
	}
	for (i = 0; i  <=  0xF; i++) {
		for (j = 0; j  <=  0xF; j++) {
			tmp = gpio_i2c_read(nvp6158_iic_addr[ch/4], (i<<4) | j);
			if (j == 0)
				sensor_dbg("0x%02x-0x%02x ", (i<<4) | j, tmp);
			else if (j == 0xF)
				sensor_dbg("0x%02x\r\n", tmp);
			else
				sensor_dbg("0x%02x ", tmp);
		}
	}
}

unsigned char nvp6158_video_get_adcclk(unsigned char ch)
{
	unsigned char adc_value;

	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x01);
	adc_value = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x84+ch%4);
	sensor_dbg(">>>>> DRV[%s:%d] CH:%d, Bank:0x%02x, ADC clock delay:0x%x\n", __func__, __LINE__, ch, nvp6158_iic_addr[ch/4], adc_value);
	return adc_value;
}

void nvp6158_video_set_adcclk(unsigned char ch, unsigned char value)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x01);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x84+ch%4, value);
	sensor_dbg(">>>>> DRV[%s:%d] CH:%d, Bank:0x%02x, ADC clock delay:0x%x\n", __func__, __LINE__, ch, nvp6158_iic_addr[ch/4], value);
}

void NVP6158_set_afe(unsigned char ch, unsigned char onoff)
{
	unsigned char afe_value;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	afe_value = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x00+ch%4);
	if (onoff == 1)
		_CLE_BIT(afe_value, 0);
	else
		_SET_BIT(afe_value, 0);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x00+ch%4, afe_value);
	msleep(10);
	sensor_dbg("NVP6158_set_afe ch[%d] [%s] done\n", ch, onoff?"ON":"OFF");
}

void nvp6158_datareverse(unsigned char chip, unsigned char port)
{
/*
BANK1 0xCB[3:0],每个bit控制一个bt656的数据顺序，1为反序，0为正序。
*/
	unsigned char tmp;
	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x01);
	tmp = gpio_i2c_read(nvp6158_iic_addr[chip], 0xCB);
	_SET_BIT(tmp, port);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0xCB, tmp);
	sensor_dbg("nvp6158[%d] port[%d] data reversed\n", chip, port);
}

void nvp6158_pll_bypass(unsigned char chip, int flag)
{
	unsigned char val_1x81;
	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x01);
	val_1x81 = gpio_i2c_read(nvp6158_iic_addr[chip], 0x81);
	if (flag == 1)
		val_1x81 |= 0x02;
	else
		val_1x81 &= 0xFD;
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x81, val_1x81);
}


void nvp6158_system_init(unsigned char chip)
{
	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x80, 0x0F);

	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x01);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x80, 0x40);
	msleep(30);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x80, 0x61);
	msleep(30);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x80, 0x60);

	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x01);
	if (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6168C_R0_ID)
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xCA, 0x66); /* NVP6158C/6158B ONLY HAS 2 PORTS */
	else
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xCA, 0xFF); /* NVP6158 HAS 4 PORTS */

	sensor_dbg("nvp6158[C]_system_init\n");
}

/*******************************************************************************
*	Description		: Initialize common value of AHD
*	Argurments		: dec(slave address)
*	Return value	: rev ID
*	Modify			:
*	warning			:
*******************************************************************************/
void nvp6158_common_init(unsigned char chip)
{
	int ch;
	/* initialize chip */
	nvp6158_system_init(chip);

	for (ch = 0; ch < 4; ch++) {
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x00);
		/* gpio_i2c_write(nvp6158_iic_addr[chip], 0x00+ch,    0x10); */
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x22+4*ch, 0x0B);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x23+4*ch, 0x41);

		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x05+ch%4);
		/* gpio_i2c_write(nvp6158_iic_addr[chip], 0x00, 0xD0); // Clamp speed */
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xA9, 0x80);
	}
}

#define MAX_DEBOUNCE_CNT 5
int nvp6158_AutoDebouceCheck(unsigned char ch, NVP6158_INFORMATION_S *pInformation)
{
	int i;
	int ret = 0;
	/* unsigned char oDevNum = 0; */
	unsigned char oDebncIdx = 0;
	unsigned char oVfc = 0;
	NC_VIVO_CH_FORMATDEF oFmtB5Def;
	video_input_vfc	sVFC;
	/* decoder_dev_ch_info_s sDevChInfo; */


	sVFC.ch = ch % 4;
	sVFC.devnum = ch / 4;
	video_input_onvideo_check_data(&sVFC);

	oDebncIdx = pInformation->debounceidx[ch];
	pInformation->debounce[ch][oDebncIdx%MAX_DEBOUNCE_CNT] = sVFC.vfc;

	/* For Debug Ch1 Only */
	/*
	if (ch == 0)
		sensor_dbg("debunce:0x%02X, debncIdx:%d\n", pInformation->debounce[ch][pInformation->debounceidx[ch]], pInformation->debounceidx[ch]);
	*/
	pInformation->debounceidx[ch]++;
	pInformation->debounceidx[ch] = ((pInformation->debounceidx[ch] % MAX_DEBOUNCE_CNT) == 0) ? 0 : pInformation->debounceidx[ch];

	oVfc = pInformation->debounce[ch][pInformation->debounceidx[ch]];
	for (i = 0; i < MAX_DEBOUNCE_CNT; i++) {
		if (oVfc != pInformation->debounce[ch][i])
			break;
	}
	if (i == MAX_DEBOUNCE_CNT) {
		oFmtB5Def = NC_VD_AUTO_VFCtoFMTDEF(ch, oVfc);
		/* if ((oFmtB5Def != AHD30_5M_20P) && (oFmtB5Def != pInformation->prevideofmt[ch])) */
		if (((oFmtB5Def != AHD30_5M_20P) && (oFmtB5Def != CVI_8M_15P) &&
			(oFmtB5Def != CVI_8M_12_5P) && (oFmtB5Def != CVI_HD_30P_EX) &&
			(oFmtB5Def != AHD20_1080P_25P) && (oFmtB5Def != AHD20_1080P_30P) &&
			(oFmtB5Def != CVI_FHD_25P))
			&& (oFmtB5Def != pInformation->prevideofmt[ch])) {
			sensor_dbg("\n\n\n>>>>>>WATCH OUT<<<<<<ch[%d] oVfc[%2x]oFmtB5Def[%2x] != pInformation->prevideofmt[%2x]\n\n\n", ch, oVfc, oFmtB5Def, pInformation->prevideofmt[ch]);
			ret = -1;
		}
	}

	return ret;
}

void nvp6158_channel_reset(unsigned char ch)
{
	unsigned char reg_1x97, bank_save;
	bank_save = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0xFF);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x01);
	reg_1x97 = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x97);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x97, reg_1x97&(~(1<<(ch%4))));
	msleep(30);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x97, reg_1x97|0x0F);
	msleep(30);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, bank_save);
	sensor_dbg("CH[%d] channel been resetted\n", ch);
}
void nvp6158_set_colorpattern(void)
{
	int chip;
	for (chip = 0; chip < nvp6158_cnt; chip++) {
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x00);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x78, 0xaa);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x79, 0xaa);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x05);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x2c, 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x6a, 0x90);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x06);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x2c, 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x6a, 0x90);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x07);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x2c, 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x6a, 0x90);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x2c, 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x6a, 0x90);
	}
}

void nvp6158_adc_reset(unsigned char ch)
{
	unsigned char bank_save;
	bank_save = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0xFF);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x0B, 0xF0);
	msleep(30);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x0B, 0x0F);
	msleep(30);
	sensor_dbg("CH[%d] adc been resetted\n", ch);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, bank_save);
}

int nvp6158_GetFormatEachCh(unsigned char ch, NVP6158_INFORMATION_S *pInformation)
{
	video_input_vfc sVFC;
	video_input_vfc svin_vfc_bak;
	video_input_novid sNoVideo;
	NC_VIVO_CH_FORMATDEF oCurVidFmt;
	/* NC_VIDEO_ONOFF oCurVideoloss; */

	/* initialize current video format - pInformation structure is for app */
	pInformation->curvideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;
	pInformation->curvideoloss[ch] = VIDEO_LOSS_OFF;
	pInformation->vfc[ch] = 0xff;

	/* initialize vfc(B5xF0) and videoloss information(B0xA8) */
	sVFC.ch = ch%4;
	sVFC.devnum = ch/4;
	sNoVideo.ch = ch%4;
	sNoVideo.devnum = ch/4;

	/* get vfc and videoloss */
	if (chip_nvp6158_id[ch/4] == NVP6158C_R0_ID || chip_nvp6158_id[ch/4] == NVP6158_R0_ID)
		video_input_vfc_read(&sVFC);
	else
		nvp6168_video_input_vfc_read(&sVFC);
	video_input_novid_read(&sNoVideo);
	svin_vfc_bak.ch = ch%4;
	svin_vfc_bak.devnum = ch/4;
	if (chip_nvp6158_id[ch/4] == NVP6158C_R0_ID || chip_nvp6158_id[ch/4] == NVP6158_R0_ID)
		video_input_onvideo_check_data(&svin_vfc_bak);

	/* check vfc&videoloss and run debounce  */
	if (((((sVFC.vfc >> 4) & 0xF) != 0xF) && ((sVFC.vfc & 0x0F) != 0xF)) && !sNoVideo.novid) { /* OnVideo */
		/* convert vfc to formatDefine for APP and save videoloss information */
		oCurVidFmt = NC_VD_AUTO_VFCtoFMTDEF(ch, sVFC.vfc);

		/* debouce */
		pInformation->curvideofmt[ch] = oCurVidFmt;
		pInformation->vfc[ch] = sVFC.vfc;
	} else if (((((sVFC.vfc >> 4) & 0xF) == 0xF) && ((sVFC.vfc & 0x0F) == 0xF)) && !sNoVideo.novid) {
		if (chip_nvp6158_id[ch/4] == NVP6158C_R0_ID || chip_nvp6158_id[ch/4] == NVP6158_R0_ID) {
			if (svin_vfc_bak.vfc == 0xFF) {
				/* nvp6158_channel_reset(ch); */
				/* nvp6158_adc_reset(ch); */
			}
		}
	}

	/* check novideo option */
	if (!sNoVideo.novid)
		pInformation->curvideoloss[ch] = VIDEO_LOSS_ON;

	return 0;
}
/*******************************************************************************
*	Description		: get videoloss information and get video format.
*	Argurments		: pvideofmt(video format buffer point)
*	Return value	: vloss(video loss information)
*	Modify			:
*	warning			:
*******************************************************************************/
static int CVI_720P30[16] = {0,};
unsigned int video_fmt_det(const unsigned char ch, NVP6158_INFORMATION_S *ps_nvp6158_vfmts)
{
	int ret;
	unsigned char oCurVideofmt = 0x00;
	unsigned char oPreVideofmt = 0x00;
	NC_VIVO_CH_FORMATDEF oFmtDef;
	decoder_dev_ch_info_s sDevChInfo;
	video_input_vfc sVFC_B13;
	video_input_vfc sVFC_B5;

	/* for (ch=0; ch<nvp6158_cnt*4; ch++)  */
	{
		/* get video format */
		nvp6158_GetFormatEachCh(ch, ps_nvp6158_vfmts);
		/* process video format on/off */
		oCurVideofmt  = ps_nvp6158_vfmts->curvideofmt[ch];
		oPreVideofmt  = ps_nvp6158_vfmts->prevideofmt[ch];

		if (ps_nvp6158_vfmts->curvideoloss[ch] == VIDEO_LOSS_ON) {
			/* on video */
			if ((oCurVideofmt != NC_VIVO_CH_FORMATDEF_UNKNOWN) && (oPreVideofmt == NC_VIVO_CH_FORMATDEF_UNKNOWN)) {
				oFmtDef = NC_VD_AUTO_VFCtoFMTDEF(ch, ps_nvp6158_vfmts->vfc[ch]);
				sDevChInfo.ch = ch%4;
				sDevChInfo.devnum = ch/4;
				sDevChInfo.fmt_def = oFmtDef;
				if (oFmtDef == AHD30_5M_20P) {
					sensor_dbg("[CH:%d] >> finding format: %x....\n", ch, oFmtDef);

					video_input_ahd_tvi_distinguish(&sDevChInfo);
					oFmtDef = sDevChInfo.fmt_def;

					ps_nvp6158_vfmts->curvideofmt[ch] = oFmtDef;
				} else if (oFmtDef == CVI_8M_15P || oFmtDef == CVI_8M_12_5P) {
					if (oFmtDef == CVI_8M_15P)
						sensor_dbg("[CH:%d] >> finding format:CVI 8M 15P....\n", ch);
					else
						sensor_dbg("[CH:%d] >> finding format:CVI 8M 12.5P....\n", ch);

					video_input_cvi_tvi_distinguish(&sDevChInfo);
					oFmtDef = sDevChInfo.fmt_def;

					if (oFmtDef == TVI_8M_15P) {
						sensor_dbg("[CH:%d] >> changing format:TVI 8M 15P....\n", ch);
						ps_nvp6158_vfmts->curvideofmt[ch] = TVI_8M_15P;
					} else if (oFmtDef == TVI_8M_12_5P) {
						sensor_dbg("[CH:%d] >> changing format:TVI 8M 12_5P....\n", ch);
						ps_nvp6158_vfmts->curvideofmt[ch] = TVI_8M_12_5P;
					}
				} else if (oFmtDef == AHD20_720P_30P_EX_Btype/* || oFmtDef == CVI_HD_30P_EX*/) {
					if (CVI_720P30[ch] == 0) {
						oFmtDef = CVI_HD_30P_EX;
						ps_nvp6158_vfmts->curvideofmt[ch] = CVI_HD_30P_EX;
						CVI_720P30[ch] = 1;
					}
				} else if (oFmtDef == CVI_FHD_25P) {
					sensor_dbg("[CH:%d] >> finding format: %x....\n", ch, oFmtDef);

					video_input_cvi_ahd_1080p_distinguish(&sDevChInfo);
					oFmtDef = sDevChInfo.fmt_def;

					if (oFmtDef == AHD20_1080P_25P) {
						sensor_dbg("[CH:%d] >> changing format:AHD 2M 25P....\n", ch);

						ps_nvp6158_vfmts->curvideofmt[ch] = AHD20_1080P_25P;
					}
				}
				if (ps_nvp6158_vfmts->vfc[ch] == 0x2B) {
					sDevChInfo.ch = ch%4;
					sDevChInfo.devnum = ch/4;
					sDevChInfo.fmt_def = ps_nvp6158_vfmts->vfc[ch];
					video_input_ahd_tvi_distinguish(&sDevChInfo);
					oFmtDef = sDevChInfo.fmt_def;

					if (oFmtDef == TVI_4M_15P) {
						if ((det_mode[ch] == NVP6158_DET_MODE_AUTO) || (det_mode[ch] == NVP6158_DET_MODE_TVI)) {
							sensor_dbg("[CH:%d] >> changing format:TVI 4M 15P....\n", ch);

							ps_nvp6158_vfmts->curvideofmt[ch] = TVI_4M_15P;
						} else
							ps_nvp6158_vfmts->curvideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;
					}
				}

				ps_nvp6158_vfmts->prevideofmt[ch] = ps_nvp6158_vfmts->curvideofmt[ch];
				#ifdef _ENABLE_DET_DEBOUNCE_
				s_raptor3_vfmts.debounce[ch][0] = 0; /* clear debounce param status */
				s_raptor3_vfmts.debounce[ch][1] = 0;
				s_raptor3_vfmts.debounce[ch][2] = 0;
				s_raptor3_vfmts.debounce[ch][3] = 0;
				s_raptor3_vfmts.debounce[ch][4] = 0;
				s_raptor3_vfmts.debounceidx[ch] = 0;
				s_raptor3_vfmts.prevideofmt[ch] = ps_nvp6158_vfmts->curvideofmt[ch];  /* information for debounce. */
				#endif
				/* nvp6158_set_chnmode(ch, ps_nvp6158_vfmts->prevideofmt[ch]); */
				sensor_dbg(">>>>> CH[%d], Set video format : 0x%02X\n", ch, oCurVideofmt);
			} else if ((oCurVideofmt == NC_VIVO_CH_FORMATDEF_UNKNOWN) && (oPreVideofmt == NC_VIVO_CH_FORMATDEF_UNKNOWN)) {
				int ii = 0;
				int retry_cnt = 0;

				/* AHD 1080P, 720P NRT Detection Part */
						/*
				   1. Check Bank13 0xF0
						   2. Check NoVideo Register (Bank0 0xA8)
						   3. Set Value 0x7f to Bank5 0x82
						   4. Read Bank13 0xf0
						   5. Read Bank5 0xf0
				   6. Check H Count
				   7. AHD 1080P or 720P Set
				   8. Set value 0x00 to bank5 0x82
						   */

				sVFC_B13.ch = ch%4;
				sVFC_B13.devnum = ch / 4;
				sVFC_B5.ch = ch%4;
				sVFC_B5.devnum = ch / 4;

				sDevChInfo.ch = ch%4;
				sDevChInfo.devnum = ch / 4;

				/* video_input_manual_agc_stable_endi(&sDevChInfo, 1); */

				for (ii = 0; ii < 20; ii++) {
					video_input_vfc_read(&sVFC_B13);
					video_input_onvideo_check_data(&sVFC_B5);

					if (((sVFC_B5.vfc >> 4) & 0xf) < 0x2)
						break;

					if (sVFC_B13.vfc == 0x2b && sVFC_B5.vfc == 0x3f) {
						sensor_dbg("[DRV] CH[%d] Bank13 0xF0 [%02x], Bank5 0xF0[%02x]\n", ch, sVFC_B13.vfc, sVFC_B5.vfc);
						sensor_dbg("[DRV] CH[%d] AFHD 15P or 12.5P [%d]\n", ch, retry_cnt);
						break;
					} else if ((sVFC_B5.vfc != 0x2f && sVFC_B5.vfc != 0x3f) &&  sVFC_B13.vfc != 0x2b) {
						sensor_dbg("[DRV] CH[%d] Bank13 0xF0 [%02x], Bank5 0xF0[%02x]\n", ch, sVFC_B13.vfc, sVFC_B5.vfc);
						sensor_dbg("[DRV] CH[%d] Unknown Status [%d] \n", ch, retry_cnt);
					}

					if (retry_cnt  >=  20) {
						sensor_dbg("CH[%d] Unknown Status  Disitinguish Finished ...\n", ch);
						break;
					}

					retry_cnt++;
					msleep(33);
				}

				if (((sVFC_B5.vfc >> 4) & 0xf) < 0x2)
					return 0;

				video_input_ahd_nrt_distinguish(&sDevChInfo);

				if (sDevChInfo.fmt_def == NC_VIVO_CH_FORMATDEF_UNKNOWN) {
					sensor_dbg("[DRV] CH[%d] unknown format \n", ch);
					return 0;
				}

				oFmtDef = sDevChInfo.fmt_def;
				/* set video format(DEC) */
				ps_nvp6158_vfmts->curvideofmt[ch] = oFmtDef;
				ps_nvp6158_vfmts->prevideofmt[ch] = ps_nvp6158_vfmts->curvideofmt[ch];
#ifdef _ENABLE_DET_DEBOUNCE_
				s_raptor3_vfmts.debounce[ch][0] = 0; /* clear debounce param status */
				s_raptor3_vfmts.debounce[ch][1] = 0;
				s_raptor3_vfmts.debounce[ch][2] = 0;
				s_raptor3_vfmts.debounce[ch][3] = 0;
				s_raptor3_vfmts.debounce[ch][4] = 0;
				s_raptor3_vfmts.debounceidx[ch] = 0;
				s_raptor3_vfmts.prevideofmt[ch] = ps_nvp6158_vfmts->curvideofmt[ch];  //information for debounce.
#endif

				/* save onvideo to prevideofmt */
				/* s_raptor3_vfmts.prevideofmt[ch] = s_raptor3_vfmts.curvideofmt[ch]; */

				/* video_input_manual_agc_stable_endi(&sDevChInfo, 0); */
				sensor_dbg(">>>>> CH[%d], Auto, Set video format : 0x%02X\n", ch, oCurVideofmt);

			} else {
#ifdef _ENABLE_DET_DEBOUNCE_
				ret = nvp6158_AutoDebouceCheck(ch, &s_raptor3_vfmts);  /* note!!!! */
				if ((ret == -1) && (gCoaxFirmUpdateFlag[ch] == 0)) {
					sDevChInfo.ch = ch % 4;
					sDevChInfo.devnum = ch/4;
					/* hide decoder */
					nvp6158_hide_ch(ch);

					/* decoder afe power down */
					video_input_vafe_control(&sDevChInfo, 0);
					/* set no video- first(i:channel, raptor3_vfmts:information */
					/* nvp6158_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN); */

					video_input_vafe_control(&sDevChInfo, 1);

					/* for forced agc stable */
					/* video_input_manual_agc_stable_endi(&sDevChInfo, 0); */
					/* msleep(50); */

					/* save onvideo to prevideofmt */
					ps_nvp6158_vfmts->prevideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;
					s_raptor3_vfmts.prevideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;
					sensor_dbg(">>>>> CH[%d], Reset, Set No video : 0x%02X\n", ch, oCurVideofmt);
				}
			}
#endif
		} else {
			/* no video */
			if (oPreVideofmt != NC_VIVO_CH_FORMATDEF_UNKNOWN) {
				//nvp6158_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN);
				ps_nvp6158_vfmts->prevideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;
				CVI_720P30[ch] = 0;
#ifdef _ENABLE_DET_DEBOUNCE_
				s_raptor3_vfmts.prevideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;
#endif
				sensor_dbg(">>>>> CH[%d], Set No video : 0x%02X\n", ch, oCurVideofmt);
			}
		}
	}

	return ps_nvp6158_vfmts->prevideofmt[ch];
}

unsigned int nvp6168_video_fmt_det(const unsigned char ch, NVP6158_INFORMATION_S *ps_nvp6158_vfmts)
{
	/* int ret; */
	unsigned char oCurVideofmt = 0x00;
	unsigned char oPreVideofmt = 0x00;
	NC_VIVO_CH_FORMATDEF oFmtDef;
	decoder_dev_ch_info_s sDevChInfo;
	/* video_input_vfc sVFC_B13; */
	/* video_input_vfc sVFC_B5; */

	/* for (ch=0; ch<nvp6158_cnt*4; ch++) */
	{
		/* get video format */
		nvp6158_GetFormatEachCh(ch, ps_nvp6158_vfmts);
		/* process video format on/off */
		oCurVideofmt  = ps_nvp6158_vfmts->curvideofmt[ch];
		oPreVideofmt  = ps_nvp6158_vfmts->prevideofmt[ch];

		if (ps_nvp6158_vfmts->curvideoloss[ch] == VIDEO_LOSS_ON) {
			/* on video */
			if ((oCurVideofmt != NC_VIVO_CH_FORMATDEF_UNKNOWN) && (oPreVideofmt == NC_VIVO_CH_FORMATDEF_UNKNOWN)) {
				oFmtDef = NC_VD_AUTO_VFCtoFMTDEF(ch, ps_nvp6158_vfmts->vfc[ch]);
				sDevChInfo.ch = ch%4;
				sDevChInfo.devnum = ch/4;
				sDevChInfo.fmt_def = oFmtDef;

				if (oFmtDef == TVI_5M_20P) { /* needs 2nd identify */
					nvp6168_video_input_cvi_tvi_5M20p_distinguish(&sDevChInfo);
					oFmtDef = sDevChInfo.fmt_def;

					ps_nvp6158_vfmts->curvideofmt[ch] = oFmtDef;
				}

				ps_nvp6158_vfmts->prevideofmt[ch] = ps_nvp6158_vfmts->curvideofmt[ch];

				/* nvp6158_set_chnmode(ch, ps_nvp6158_vfmts->prevideofmt[ch]); */
				sensor_dbg(">>>>> CH[%d], Set video format : 0x%02X\n", ch, oCurVideofmt);
			}

		} else {
			/* no video */
			if (oPreVideofmt != NC_VIVO_CH_FORMATDEF_UNKNOWN) {
				//nvp6158_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN);
				ps_nvp6158_vfmts->prevideofmt[ch] = NC_VIVO_CH_FORMATDEF_UNKNOWN;

				sensor_dbg(">>>>> CH[%d], Set No video : 0x%02X\n", ch, oCurVideofmt);
			}
		}
	}

	return ps_nvp6158_vfmts->prevideofmt[ch];
}

unsigned int nvp6158_getvideoloss(void)
{
	unsigned int vloss = 0, i;
	unsigned char vlossperchip[4];

	for (i = 0; i < nvp6158_cnt; i++) {
		gpio_i2c_write(nvp6158_iic_addr[i], 0xFF, 0x00);
		vlossperchip[i] = (gpio_i2c_read(nvp6158_iic_addr[i], 0xA8)&0x0F);
		vloss |= (vlossperchip[i]<<(4*i));
	}

	return vloss;
}

void nvp6158_vd_chnreset(unsigned char ch)
{
	unsigned char reg_1x97;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x01);
	reg_1x97 = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x97);
	_CLE_BIT(reg_1x97, (ch%4));
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x97, reg_1x97);
	msleep(10);
	_SET_BIT(reg_1x97, (ch%4));
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x97, reg_1x97);
}

/*0:agc unlocked; 1:agc locked*/
int nvp6158_GetAgcLockStatus(unsigned char ch)
{
	int agc_lock, ret;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	agc_lock = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0xE0);
	ret = ((agc_lock>>(ch%4))&0x01);

	return ret;
}

/*0:fsc unlocked; 1:fsc locked*/
int nvp6158_GetFSCLockStatus(unsigned char ch)
{
	int fsc_lock, ret;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	fsc_lock = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0xE8+(ch%4));
	ret = ((fsc_lock>>1)&0x01);

	return ret;
}

void nvp6158_ResetFSCLock(unsigned char ch)
{
	unsigned char acc_ref = 0;
	unsigned char check_cnt = 4;
	do {
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x05+(ch%4));
		acc_ref = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x27);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x23, 0x80);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x27, 0x10);
		msleep(35);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x23, 0x00);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x27, acc_ref);
		msleep(300);
	} while ((nvp6158_GetFSCLockStatus(ch) == 0) && ((check_cnt--) > 0));

	sensor_dbg("%s, %d\n", __FUNCTION__, __LINE__);
}

void nvp6158_chn_killcolor(unsigned char ch, unsigned char onoff)
{
	unsigned char colorkill;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	colorkill = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x22+(ch%4)*4);
	if (onoff == 1)
		_SET_BIT(colorkill, 4);
	else
		_CLE_BIT(colorkill, 4);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x22+(ch%4)*4, colorkill);
	sensor_dbg("%s, %d %x %x\n", __FUNCTION__, __LINE__, onoff, colorkill);
}

void nvp6158_hide_ch(unsigned char ch)
{
	unsigned char reg_0x7a;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	reg_0x7a = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x7A+((ch%4)/2));
	reg_0x7a &= (ch%2 == 0?0xF0:0x0F);
	reg_0x7a |= (ch%2 == 0?0x0F:0xF0);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x7A+((ch%4)/2), reg_0x7a);
	/* sensor_dbg("%s, %d\n", __FUNCTION__, __LINE__); */
}

void nvp6158_show_ch(unsigned char ch)
{
	unsigned char reg_0x7a;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	reg_0x7a = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x7A+((ch%4)/2));
	reg_0x7a &= (ch%2 == 0?0xF0:0x0F);
	reg_0x7a |= (ch%2 == 0?0x01:0x10);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x7A+((ch%4)/2), reg_0x7a);
	/* sensor_dbg("%s, %d\n", __FUNCTION__, __LINE__); */
}


/*
support AHD 3M/4M real-time camera switch between NTSC and PAL
*/
int nvp6158_acp_SetVFmt(unsigned char ch, const unsigned char vfmt)
{
	/*nvp6158_acp_rw_data_extention acpdata;

	if ((vfmt!=NTSC) && (vfmt!=PAL)) {
		sensor_dbg("%s vfmt[%d] out of range!!!\n", __FUNCTION__, vfmt);
		return -1;
	}
	if (ch_vfmt_status[ch] == vfmt) {
		sensor_dbg("%s vfmt is %d now!!!\n", __FUNCTION__, vfmt);
		return -2;
	}

	acpdata.ch = ch;
	acpdata.data[0] = 0x60; // register write
	acpdata.data[1] = 0x82; // Output mode command
	acpdata.data[2] = 0x19; // Output Format Change mode
	acpdata.data[3] = 0x00; // Output Mode value
	acpdata.data[4] = 0x00;
	acpdata.data[5] = 0x00;
	acpdata.data[6] = 0x00;
	acpdata.data[7] = 0x00;
	if ((ch_mode_status[ch] == NVP6158_VI_3M ||
		 ch_mode_status[ch] == NVP6158_VI_3M_NRT ||
		 ch_mode_status[ch] == NVP6158_VI_4M_NRT ||
		 ch_mode_status[ch] == NVP6158_VI_4M) &&
		nvp6158_GetAgcLockStatus(ch)==1) {
		acpdata.data[3] = vfmt^1;   //CAUTION!!! IN CAMERA SIDE 0:PAL, 1:NTSC.
		acp_isp_write_extention(ch, &acpdata);
		msleep(100);
		sensor_dbg("%s change ch[%d] to %s!!!\n", __FUNCTION__, ch, vfmt==NTSC?"NTSC":"PAL");
	}
	*/
	return 0;
}

void nvp6158_video_set_contrast(unsigned char ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x10+(ch%4)), value);
}

void nvp6158_video_set_brightness(unsigned char ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x0C+(ch%4)), value);
}

void nvp6158_video_set_saturation(unsigned char ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x3C+(ch%4)), value);
}

void nvp6158_video_set_hue(unsigned char ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x40+(ch%4)), value);
}

void nvp6158_video_set_sharpness(unsigned char ch, unsigned int value)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x14+(ch%4)), (0x90+value-100));
}

//u-gain value B0 0x44~0x47
void nvp6158_video_set_ugain(unsigned char ch, unsigned int value)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x44+(ch%4)), value);
}

//v-gain value B0 0x48~0x4b
void nvp6158_video_set_vgain(unsigned char ch, unsigned int value)
{
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], (0x48+(ch%4)), value);
}

void video_input_new_format_set(const unsigned char ch, const unsigned char chnmode)
{
	unsigned char val_9x44;


	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x11);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x00 + ((ch%4) * 0x20), 0x00);

	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x09);
	val_9x44 = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x44);
	val_9x44 &= ~(1 << (ch%4));

	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x44, val_9x44);

	/* CVI HD 30P PN Value Set */
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x50 + ((ch%4) * 4), 0x30);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x51 + ((ch%4) * 4), 0x6F);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x52 + ((ch%4) * 4), 0x67);
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x53 + ((ch%4) * 4), 0x48);

}

void nvp6158_set_chn_ycmerge(const unsigned char ch, unsigned char onoff)
{
	unsigned char YCmerge, val5x69;
	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x01);
	YCmerge = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0xed);
	_CLE_BIT(YCmerge, (ch%4));
	if (onoff == 1)
		_SET_BIT(YCmerge, (ch%4));

	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xed, YCmerge);

	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x05+ch%4);
	val5x69 = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x69);
	_CLE_BIT(val5x69, 4);
	if (onoff == 1)
		_SET_BIT(val5x69, 4);

	gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x69, val5x69);
}

/*******************************************************************************
*	Description		: set this value
*	Argurments		: ch(channel)
*	Return value	: void
*	Modify			:
*	warning			: You don't have to change these values.
*******************************************************************************/
void nvp6158_set_chn_commonvalue(const unsigned char ch, const unsigned char chnmode)
{
	decoder_dev_ch_info_s decoder_info;
	unsigned char val_0x54;
	unsigned char vfmt = chnmode%2;

	if ((chnmode  <=  AHD20_SD_H960_2EX_Btype_PAL) && (chnmode  >=  AHD20_SD_H960_NT)) {
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
		val_0x54 = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x54);
		_CLE_BIT(val_0x54, (ch%4+4));
		if (vfmt != PAL)
			_SET_BIT(val_0x54, (ch%4+4));
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x54, val_0x54);

		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x69, 0x01);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xB8, 0xB8);
	} else {
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x00);
		val_0x54 = gpio_i2c_read(nvp6158_iic_addr[ch/4], 0x54);
		_CLE_BIT(val_0x54, (ch%4+4));
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x54, val_0x54);

		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xFF, 0x05+ch%4);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x69, 0x00);
		gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xB8, 0x39);
	}

	decoder_info.ch = ch%4;
	decoder_info.devnum = ch/4;
	decoder_info.fmt_def = chnmode;
	if (__IsOver3MRTVideoFormat(&decoder_info))
		nvp6158_set_chn_ycmerge(ch, 1);
	else
		nvp6158_set_chn_ycmerge(ch, 0);
	if (chip_nvp6158_id[decoder_info.devnum] == NVP6158C_R0_ID || chip_nvp6158_id[decoder_info.devnum] == NVP6158_R0_ID)
		video_input_onvideo_set(&decoder_info);
	else
		nvp6168_video_input_onvideo_set(&decoder_info);
}


/*
设置通道模式
变量
ch: 通道号，取值范围0~(nvp6158_cnt*4-1)
vfmt: 0:NTSC, 1:PAL
chnmode:通道模式，参考NVP6158_VI_MODE.
*/
int nvp6158_set_chnmode(const unsigned char ch, const unsigned char chnmode)
{
	//unsigned char tmp;
	video_equalizer_info_s vin_eq_set;
	video_input_novid auto_novid;
	nvp6158_coax_str s_coax_str;

	if (ch  >=  (nvp6158_cnt*4)) {
		sensor_dbg("func[nvp6158_set_chnmode] Channel %d is out of range!!!\n", ch);
		return -1;
	}

	/* set video format each format */
	if (chnmode < NC_VIVO_CH_FORMATDEF_MAX)  {
		if (NC_VIVO_CH_FORMATDEF_UNKNOWN != chnmode) {
			nvp6158_set_chn_commonvalue(ch, chnmode);

			video_input_new_format_set(ch, chnmode);

			s_coax_str.ch = ch;
			s_coax_str.fmt_def = chnmode;
			coax_nvp6158_tx_init(&s_coax_str);
			coax_nvp6158_tx_16bit_init(&s_coax_str); /* for ahd 720P and CVI 4M */
			coax_nvp6158_rx_init(&s_coax_str);

			vin_eq_set.Ch = ch%4;
			vin_eq_set.devnum = ch/4;
			vin_eq_set.distance = 0;
			vin_eq_set.FmtDef = chnmode;
			nvp6158_set_equalizer(&vin_eq_set);

			gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xff, 0x09);
			gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x40+ch%4, 0x61);
			msleep(35);
			if (AHD20_SD_H960_2EX_Btype_PAL  >=  chnmode)
				gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x40+ch%4, 0x60); /* for comet setting */
			else
				gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x40+ch%4, 0x00);
			nvp6158_show_ch(ch);
		} else {
			nvp6158_hide_ch(ch);
			auto_novid.ch = ch%4;
			auto_novid.devnum = ch/4;
			video_input_no_video_set(&auto_novid);
			nvp6158_set_chn_ycmerge(ch, 0);
		}
		ch_mode_status[ch] = chnmode;
		/* ch_vfmt_status[ch] = chnmode%2; */

		sensor_dbg(">>>>%s CH[%d] been setted to %2x mode\n", __func__, ch, chnmode);
	}

	return 0;
}

int nvp6168_set_chnmode(const unsigned char ch, const unsigned char chnmode)
{
	/* unsigned char tmp; */
	video_equalizer_info_s vin_eq_set;
	video_input_novid auto_novid;
	nvp6158_coax_str s_coax_str;

	if (ch  >=  (nvp6158_cnt*4)) {
		sensor_dbg("func[nvp6168_set_chnmode] Channel %d is out of range!!!\n", ch);
		return -1;
	}

	/* set video format each format */
	if (chnmode < NC_VIVO_CH_FORMATDEF_MAX) {
		if (NC_VIVO_CH_FORMATDEF_UNKNOWN != chnmode) {
			nvp6158_set_chn_commonvalue(ch, chnmode);

			/* video_input_new_format_set(ch, chnmode); */

			s_coax_str.ch = ch;
			s_coax_str.fmt_def = chnmode;
			coax_nvp6158_tx_init(&s_coax_str);
			coax_nvp6158_tx_16bit_init(&s_coax_str); /* for ahd 720P and CVI 4M */
			coax_nvp6158_rx_init(&s_coax_str);

			vin_eq_set.Ch = ch%4;
			vin_eq_set.devnum = ch/4;
			vin_eq_set.distance = 0;
			vin_eq_set.FmtDef = chnmode;
			nvp6168_set_equalizer(&vin_eq_set);

			gpio_i2c_write(nvp6158_iic_addr[ch/4], 0xff, 0x09);
			gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x40+ch%4, 0x61);
			msleep(35);
			if (AHD20_SD_H960_2EX_Btype_PAL  >=  chnmode)
				gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x40+ch%4, 0x60); /* for comet setting */
			else
				gpio_i2c_write(nvp6158_iic_addr[ch/4], 0x40+ch%4, 0x00);

			nvp6158_show_ch(ch);
		} else {
			nvp6158_hide_ch(ch);
			auto_novid.ch = ch%4;
			auto_novid.devnum = ch/4;
			nvp6168_video_input_no_video_set(&auto_novid);
			nvp6158_set_chn_ycmerge(ch, 0);
		}
		ch_mode_status[ch] = chnmode;
		/* ch_vfmt_status[ch] = chnmode%2; */

		sensor_dbg(">>>>%s CH[%d] been setted to %2x mode\n", __func__, ch, chnmode);
	}

	return 0;
}


/*
nvp6158和nvp6158c共用同一函数
portsel上有差异，nvp6158c只能使用1和2，nvp6158有4个port，可以使用0~3。
chip:chip select[0,1,2,3];
portsel: port select->6158c[1,2],6158[0,1,2,3];
portmode: port mode select[1mux,2mux,4mux]
chid:  channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0]
*/
/*******************************************************************************
*	Description		: select port
*	Argurments		: chip(chip select[0,1,2,3]),
*					  portsel(port select->6158c[1,2],6158[0,1,2,3];)
*					  portmode(port mode select[1mux,2mux,4mux]),
*					  chid(channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0])
*	Return value	: 0
*	Modify			:
*	warning			:
*******************************************************************************/
int nvp6158_set_portmode(const unsigned char chip, const unsigned char portsel, const unsigned char portmode, const unsigned char chid)
{
	unsigned char chipaddr = nvp6158_iic_addr[chip];
	unsigned char tmp = 0, tmp1 = 0, reg1 = 0, reg2 = 0;

	if ((portsel != 1) && (portsel != 2) && (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6168C_R0_ID)) {
		sensor_dbg("nvp6158C_set_portmode portsel[%d] error!!!\n", portsel);
		//return -1;
	}

	switch (portmode) {
	case NVP6158_OUTMODE_1MUX_SD:
		/*输出720H或者960H单通道,数据37.125MHz,时钟37.125MHz,单沿采样.*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x86);
		break;
	case NVP6158_OUTMODE_1MUX_HD:
		/*输出720P或者1280H或者1440H单通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
		/*gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x16);*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC2, 0x10);
		gpio_i2c_write(chipaddr, 0xC3, 0x32);
		gpio_i2c_write(chipaddr, 0xC4, 0x10);
		gpio_i2c_write(chipaddr, 0xC5, 0x32);
		gpio_i2c_write(chipaddr, 0xC8, 0x00);
		gpio_i2c_write(chipaddr, 0xC9, 0x00);
		gpio_i2c_write(chipaddr, 0xCD, 0x06);
		gpio_i2c_write(chipaddr, 0xCE, 0x06);
		break;
	case NVP6158_OUTMODE_1MUX_FHD:
		/*输出720P@5060或者1080P单通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x58);
		break;
	case NVP6158_OUTMODE_2MUX_SD:
		/*输出720H或者960H 2通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid == 0?0x10:0x32);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid == 0?0x10:0x32);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x20:0x02);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x16);
		break;
	case NVP6158_OUTMODE_2MUX_HD:
		/*输出HD 2通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
		/*gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid == 0?0x10:0x32);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid == 0?0x10:0x32);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x20:0x02);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x58);*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x55, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC2, 0x10);
		gpio_i2c_write(chipaddr, 0xC3, 0x32);
		gpio_i2c_write(chipaddr, 0xC4, 0x10);
		gpio_i2c_write(chipaddr, 0xC5, 0x32);
		gpio_i2c_write(chipaddr, 0xC8, 0x22);
		gpio_i2c_write(chipaddr, 0xC9, 0x22);
		gpio_i2c_write(chipaddr, 0xCD, 0x16);
		gpio_i2c_write(chipaddr, 0xCE, 0x16);
		break;
	case NVP6158_OUTMODE_4MUX_SD:
		/*输出720H或者960H 4通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x32);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x80:0x08);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x58);
		break;
	case NVP6158_OUTMODE_4MUX_HD:
		/*输出720P 4通道,数据297MHz,时钟297MHz,单沿采样.*/
		/*gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x32);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x98);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0xba);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x80:0x08);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x58);*/
		/* gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up */
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x55, 0x10);
		gpio_i2c_write(chipaddr, 0x56, 0x32);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC2, 0x10);
		gpio_i2c_write(chipaddr, 0xC3, 0x32);
		gpio_i2c_write(chipaddr, 0xC4, 0x10);
		gpio_i2c_write(chipaddr, 0xC5, 0x32);
		gpio_i2c_write(chipaddr, 0xC8, 0x88);
		gpio_i2c_write(chipaddr, 0xC9, 0x88);
		gpio_i2c_write(chipaddr, 0xCD, 0x45);
		gpio_i2c_write(chipaddr, 0xCE, 0x45);
		break;
	case NVP6158_OUTMODE_2MUX_FHD:
		/*5M_20P,5M_12P,4M_RT,4M_15P,3M_RT/NRT,FHD,3840H,HDEX 2mux任意混合,数据297MHz,时钟148.5MHz,双沿采样.
		SOC VI端通过丢点，实现3840H->960H, HDEX->720P  */
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
#if 1
		/* CHANNEL 1 JUDGE */
		tmp  = gpio_i2c_read(chipaddr, 0x81)&0x0F;
		tmp1 = gpio_i2c_read(chipaddr, 0x85)&0x0F;
		if (((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
			reg1 |= 0x08; /* 3M_RT, THEN OUTPUT 3M_CIF DATA */
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
			reg1 |= 0x08; /* 4M, THEN OUTPUT 4M_CIF DATA */
		else if ((tmp == 0x01)  && (tmp1 == 0x05))  /* ahd 5m20p */
			reg1 |= 0x08;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && ((tmp1 == 0x02) || (tmp1 == 0x03)))  /* tvi/cvi 4m rt */
			reg1 |= 0x08;
		else if (((tmp == 0x01) || (tmp == 0x02)) && ((tmp1 == 0x08) || (tmp1 == 0x09) || (tmp1 == 0x0a)))  /* 8M */
			reg1 |= 0x08;
		else
			reg1 &= 0xF0;
		/* CHANNEL 2 JUDGE */
		tmp  = gpio_i2c_read(chipaddr, 0x82)&0x0F;
		tmp1 = gpio_i2c_read(chipaddr, 0x86)&0x0F;
		if (((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
			reg1 |= 0x80;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
			reg1 |= 0x80;
		else if ((tmp == 0x01)  && (tmp1 == 0x05))
			reg1 |= 0x80;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && ((tmp1 == 0x02) || (tmp1 == 0x03)))  /* tvi/cvi 4m rt */
			reg1 |= 0x80;
		else if (((tmp == 0x01) || (tmp == 0x02)) && ((tmp1 == 0x08) || (tmp1 == 0x09) || (tmp1 == 0x0a)))  /* 8M */
			reg1 |= 0x80;
		else
			reg1 &= 0x0F;
		/* CHANNEL 3 JUDGE */
		tmp  = gpio_i2c_read(chipaddr, 0x83)&0x0F;
		tmp1 = gpio_i2c_read(chipaddr, 0x87)&0x0F;
		if (((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
			reg2 |= 0x08;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
			reg2 |= 0x08;
		else if ((tmp == 0x01)  && (tmp1 == 0x05))
			reg2 |= 0x08;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && ((tmp1 == 0x02) || (tmp1 == 0x03)))  /* tvi/cvi 4m rt*/
			reg2 |= 0x08;
		else if (((tmp == 0x01) || (tmp == 0x02)) && ((tmp1 == 0x08) || (tmp1 == 0x09) || (tmp1 == 0x0a)))  /* 8M*/
			reg2 |= 0x08;
		else
			reg2 &= 0xF0;
		/* CHANNEL 4 JUDGE */
		tmp  = gpio_i2c_read(chipaddr, 0x84)&0x0F;
		tmp1 = gpio_i2c_read(chipaddr, 0x88)&0x0F;
		if (((tmp == 0x02) || (tmp == 0x03)) && (tmp1 == 0x04))
			reg2 |= 0x80;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && (tmp1 == 0x00))
			reg2 |= 0x80;
		else if ((tmp == 0x01)  && (tmp1 == 0x05))
			reg2 |= 0x80;
		else if (((tmp == 0x0E) || (tmp == 0x0F)) && ((tmp1 == 0x02) || (tmp1 == 0x03)))  /* tvi/cvi 4m rt*/
			reg2 |= 0x80;
		else if (((tmp == 0x01) || (tmp == 0x02)) && ((tmp1 == 0x08) || (tmp1 == 0x09) || (tmp1 == 0x0a)))  /* ahd 8M */
			reg2 |= 0x80;
		else
			reg2 &= 0x0F;
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid == 0?(0x10|reg1):(0x32|reg2));
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid == 0?(0x10|reg1):(0x32|reg2));
#else
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid == 0?0x10:0x32);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid == 0?0x10:0x32);
#endif
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x20:0x02);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x56);
		/* gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up */
		break;
	case NVP6158_OUTMODE_4MUX_MIX:
		/* HD,1920H,FHD-X 4mux任意混合,数据297MHz,时钟148.5MHz,双沿采样
		SOC VI端通过丢点，实现1920H->960H */
#if 0
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x32);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x98);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0xba);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x80:0x08);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x58);
		/* gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);  //single up */
#else
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x32);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC2, 0x54); /* ch1/2/3/4 Y data */
		gpio_i2c_write(chipaddr, 0xC3, 0x76);
		gpio_i2c_write(chipaddr, 0xC4, 0xDC); /* ch1/2/3/4 C data */
		gpio_i2c_write(chipaddr, 0xC5, 0xFE);
		gpio_i2c_write(chipaddr, 0xC8, 0x88);
		gpio_i2c_write(chipaddr, 0xC9, 0x88);
		gpio_i2c_write(chipaddr, 0xCD, 0x48);
		gpio_i2c_write(chipaddr, 0xCE, 0x48); /* 时钟 */
		gpio_i2c_write(chipaddr, 0xCA, 0x66);
#endif
		break;
	case NVP6158_OUTMODE_2MUX_MIX:
		/*HD,1920H,FHD-X 2MUX任意混合,数据148.5MHz,时钟148.5MHz,单沿采样
		SOC VI端通过丢点，实现1920H->960H  */
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid == 0?0x98:0xba);
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid == 0?0x98:0xba);
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		tmp |= (portsel%2?0x20:0x02);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x58);
		break;
	case NVP6158_OUTMODE_1MUX_BT1120S:
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		if (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6168C_R0_ID) {
			/* 6158C makes 2 bt656 ports to 1 bt1120 port.  portsel=[1,2] to choose clock. */
			gpio_i2c_write(chipaddr, 0xC2, (((chid%4)+0x0C)<<4)|((chid%4)+0x0C));
			gpio_i2c_write(chipaddr, 0xC3, (((chid%4)+0x0C)<<4)|((chid%4)+0x0C));
			gpio_i2c_write(chipaddr, 0xC4, (((chid%4)+0x04)<<4)|((chid%4)+0x04));
			gpio_i2c_write(chipaddr, 0xC5, (((chid%4)+0x04)<<4)|((chid%4)+0x04));
			gpio_i2c_write(chipaddr, 0xC8, 0x00);
			gpio_i2c_write(chipaddr, 0xC9, 0x00);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x06); /* 74.25MHz clock */
		} else {
			/* 6158 makes 4 bt656 ports to 2 bt1120 port.   portsel=[0,1] to choose clock. */
			gpio_i2c_write(chipaddr, 0xC0+portsel*4, (((chid%4)+0x0C)<<4)|((chid%4)+0x0C));
			gpio_i2c_write(chipaddr, 0xC1+portsel*4, (((chid%4)+0x0C)<<4)|((chid%4)+0x0C));
			gpio_i2c_write(chipaddr, 0xC2+portsel*4, (((chid%4)+0x04)<<4)|((chid%4)+0x04));
			gpio_i2c_write(chipaddr, 0xC3+portsel*4, (((chid%4)+0x04)<<4)|((chid%4)+0x04));
			gpio_i2c_write(chipaddr, 0xC8+(portsel), 0x00);
			gpio_i2c_write(chipaddr, 0xCC+portsel*2, 0x86); /* 37.125MHz clock */
		}
		break;
	case NVP6158_OUTMODE_2MUX_BT1120S:
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		if (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6168C_R0_ID) {
			/* 6158C makes 2 bt656 ports to 1 bt1120 port.  portsel=[1,2] to choose clock. */
			gpio_i2c_write(chipaddr, 0xC2, 0xdc);
			gpio_i2c_write(chipaddr, 0xC3, 0xdc);
			gpio_i2c_write(chipaddr, 0xC4, 0x54);
			gpio_i2c_write(chipaddr, 0xC5, 0x54);
			gpio_i2c_write(chipaddr, 0xC8, 0x22);
			gpio_i2c_write(chipaddr, 0xC9, 0x22);
			gpio_i2c_write(chipaddr, 0xCD, 0x06); //74.25MHz clock*/
			gpio_i2c_write(chipaddr, 0xCE, 0x06); //74.25MHz clock*/
		} else {
			/* 6158 makes 4 bt656 ports to 2 bt1120 port.   portsel=[0,1] to choose clock. */
			gpio_i2c_write(chipaddr, 0xC0+portsel*4, 0xdc);
			gpio_i2c_write(chipaddr, 0xC1+portsel*4, 0xdc);
			gpio_i2c_write(chipaddr, 0xC2+portsel*4, 0x54);
			gpio_i2c_write(chipaddr, 0xC3+portsel*4, 0x54);
			gpio_i2c_write(chipaddr, 0xC8+(portsel), 0x00);
			gpio_i2c_write(chipaddr, 0xCC+portsel*2, 0x06); /* 74.25MHz clock */
		}
		break;
	case NVP6158_OUTMODE_4MUX_BT1120S:
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x32);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		if (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6168C_R0_ID) {
			/* 6158C makes 2 bt656 ports to 1 bt1120 port.  portsel=[1,2] to choose clock. */
			gpio_i2c_write(chipaddr, 0xC2, 0x54);
			gpio_i2c_write(chipaddr, 0xC3, 0x76);
			gpio_i2c_write(chipaddr, 0xC4, 0xdc);
			gpio_i2c_write(chipaddr, 0xC5, 0xfe);
			gpio_i2c_write(chipaddr, 0xC8, 0x88);
			gpio_i2c_write(chipaddr, 0xC9, 0x88);
			gpio_i2c_write(chipaddr, 0xCD, 0x58); /* 148.5MHz clock */
			gpio_i2c_write(chipaddr, 0xCE, 0x58); /* 148.5MHz clock */
		} else {
			/* 6158 makes 4 bt656 ports to 2 bt1120 port.   portsel=[0,1] to choose clock. */
			gpio_i2c_write(chipaddr, 0xC0+portsel*4, 0xdc);
			gpio_i2c_write(chipaddr, 0xC1+portsel*4, 0xfe);
			gpio_i2c_write(chipaddr, 0xC2+portsel*4, 0x54);
			gpio_i2c_write(chipaddr, 0xC3+portsel*4, 0x76);
			gpio_i2c_write(chipaddr, 0xC8+(portsel), 0x88);
			gpio_i2c_write(chipaddr, 0xCC+portsel*2, 0x58); /* 148.5MHz clock */
		}
		break;
	case NVP6158_OUTMODE_1MUX_297MHz:
		/*1MUX数据输出，时钟是297MHZ输出*/
		gpio_i2c_write(chipaddr, 0xFF, 0x00);
		gpio_i2c_write(chipaddr, 0x56, 0x10);
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);   /* Port selection */
		gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);   /* Port selection */
		tmp = gpio_i2c_read(chipaddr, 0xC8+(portsel/2)) & (portsel%2?0x0F:0xF0);
		gpio_i2c_write(chipaddr, 0xC8+(portsel/2), tmp);
		gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
		break;
	default:
		sensor_dbg("portmode %d not supported yet\n", portmode);
		break;
	}

	sensor_dbg("nvp6158(b)_set_portmode portsel %d portmode %d setting\n", portsel, portmode);

	if (portmode == NVP6158_OUTMODE_2MUX_SD ||\
		portmode == NVP6158_OUTMODE_4MUX_SD ||\
		portmode == NVP6158_OUTMODE_2MUX_HD ||\
		portmode == NVP6158_OUTMODE_2MUX_MIX ||\
		portmode == NVP6158_OUTMODE_2MUX_BT1120S ||\
		portmode == NVP6158_OUTMODE_4MUX_BT1120S) {
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xA0+portsel, 0x20);  /* TM clock mode sel manual */
		sensor_dbg("TM clock mode sel manual mode \n");
	} else {
		gpio_i2c_write(chipaddr, 0xFF, 0x01);
		gpio_i2c_write(chipaddr, 0xA0+portsel, 0x00);  /* TM clock mode sel auto */
		sensor_dbg("TM clock mode sel auto mode \n");
	}

	return 0;
}


/*
chip:0~3
portsel: 6158b/c->1/2, 6158->0~3
enclk: enable clock pin,  1:enable,0:disable;
endata: enable data port, 1:enable,0:disable;
*/
void nvp6158_set_portcontrol(const unsigned char chip, const unsigned char portsel, const unsigned char enclk, const unsigned char endata)
{
	unsigned char reg_portctl;
	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x01);
	reg_portctl = gpio_i2c_read(nvp6158_iic_addr[chip], 0xCA);
	if (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6168C_R0_ID) {
		if (enclk == 1)
			_SET_BIT(reg_portctl, (portsel+5));
		else
			_CLE_BIT(reg_portctl, (portsel+5));

		if (endata == 1)
			_SET_BIT(reg_portctl, portsel);
		else
			_CLE_BIT(reg_portctl, portsel);
	} else if (chip_nvp6158_id[chip] == NVP6158_R0_ID) {
		if (enclk == 1)
			_SET_BIT(reg_portctl, (portsel+4));
		else
			_CLE_BIT(reg_portctl, (portsel+4));

		if (endata == 1)
			_SET_BIT(reg_portctl, portsel);
		else
			_CLE_BIT(reg_portctl, portsel);
	}
}

NC_FORMAT_STANDARD NVP6158_GetFmtStd_from_Fmtdef(NC_VIVO_CH_FORMATDEF vivofmt)
{
	NC_FORMAT_STANDARD vformat_std = FMT_STD_UNKNOWN;
	if ((vivofmt >= AHD20_SD_H960_NT) && (vivofmt <= AHD20_SD_H960_2EX_Btype_PAL))
		vformat_std = FMT_SD;
	else if ((vivofmt >= CVI_FHD_30P) && (vivofmt <= CVI_8M_12_5P))
		vformat_std = FMT_CVI;
	else if ((vivofmt >= TVI_FHD_30P) && (vivofmt <= TVI_8M_12_5P))
		vformat_std = FMT_TVI;
	else if ((vivofmt >= AHD20_720P_25P_EX_Btype) && (vivofmt <= AHD20_1080P_60P))
		vformat_std = FMT_AHD20;
	else if ((vivofmt >= AHD30_4M_30P) && (vivofmt <= AHD30_8M_15P))
		vformat_std = FMT_AHD30;
	else
		vformat_std = FMT_STD_UNKNOWN;
	return vformat_std;
}

void nvp6158_additional_for3MoverDef(unsigned char chip)
{
	unsigned char ch = 0;

	for (ch = 0; ch < 4; ch++) {
		gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x0a + (ch / 2));

		gpio_i2c_write(nvp6158_iic_addr[chip], 0x00 + (0x80 * (ch % 2)), 0x80);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x01 + (0x80 * (ch % 2)), 0x02);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x02 + (0x80 * (ch % 2)), 0x04);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x03 + (0x80 * (ch % 2)), 0x80);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x04 + (0x80 * (ch % 2)), 0x06);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x05 + (0x80 * (ch % 2)), 0x07);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x06 + (0x80 * (ch % 2)), 0x80);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x07 + (0x80 * (ch % 2)), 0x07);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x08 + (0x80 * (ch % 2)), 0x03);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x09 + (0x80 * (ch % 2)), 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x0a + (0x80 * (ch % 2)), 0x04);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x0b + (0x80 * (ch % 2)), 0x10);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x0c + (0x80 * (ch % 2)), 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x0d + (0x80 * (ch % 2)), 0x1f);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x0e + (0x80 * (ch % 2)), 0x2e);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x0f + (0x80 * (ch % 2)), 0x08);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x10 + (0x80 * (ch % 2)), 0x38);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x11 + (0x80 * (ch % 2)), 0x35);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x12 + (0x80 * (ch % 2)), 0x00);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x13 + (0x80 * (ch % 2)), 0x20);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x14 + (0x80 * (ch % 2)), 0x0d);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x15 + (0x80 * (ch % 2)), 0x80);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x16 + (0x80 * (ch % 2)), 0x54);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x17 + (0x80 * (ch % 2)), 0xb1);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x18 + (0x80 * (ch % 2)), 0x91);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x19 + (0x80 * (ch % 2)), 0x1c);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x1a + (0x80 * (ch % 2)), 0x87);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x1b + (0x80 * (ch % 2)), 0x92);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x1c + (0x80 * (ch % 2)), 0xe2);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x1d + (0x80 * (ch % 2)), 0x20);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x1e + (0x80 * (ch % 2)), 0xd0);
		gpio_i2c_write(nvp6158_iic_addr[chip], 0x1f + (0x80 * (ch % 2)), 0xcc);
		}
}

void nvp6158_video_powerdown(unsigned char ch)
{
	unsigned char val_0x00;   /* video afe; */
	unsigned char val_1x97;   /* clock; */
	unsigned char val_1x98;   /* channel; */
	unsigned char chip = ch / 4;

	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x00);
	val_0x00 = gpio_i2c_read(nvp6158_iic_addr[chip], 0x00+(ch%4));
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x00+(ch%4), (val_0x00|0x01));

	gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x01);
	val_1x97 = gpio_i2c_read(nvp6158_iic_addr[chip], 0x97);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x97, (val_1x97&(~(0x01<<(ch%4)))));

	val_1x98 = gpio_i2c_read(nvp6158_iic_addr[chip], 0x98);
	gpio_i2c_write(nvp6158_iic_addr[chip], 0x98, (val_1x98|(0x01<<(ch%4))));
	sensor_dbg(">>>>%s CH[%d] been setted to powerdown mode\n", __func__, ch);
}

/********************************************************************************
* End of file
********************************************************************************/
