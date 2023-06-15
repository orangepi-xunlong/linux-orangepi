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
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "video.h"
#include "coax_protocol.h"
#include "motion.h"
#include "common.h"
#include "audio.h"
#include "video_auto_detect.h"
#include "video_eq.h"
#include "nvp6158_drv.h"

extern struct v4l2_subdev *gl_sd;
#define SENSOR_NAME "nvp6158"

#define COLORBAR_EN
#define HW_REG(reg)  (*((volatile unsigned int *)(reg)))

int chip_nvp6158_id[4];
int rev_nvp6158_id[4];
unsigned int nvp6158_cnt;
unsigned int nvp6158_iic_addr[4] = {0x60, 0x62, 0x64, 0x66};

/*unsigned int nvp6158_mode = PAL;  //0:ntsc, 1: pal
module_param(nvp6158_mode, uint, S_IRUGO);
unsigned int kthread = 0;
module_param(kthread, uint, S_IRUGO);
static struct task_struct *nvp6158_kt = NULL;*/
struct semaphore nvp6158_lock;
unsigned char det_mode[16] = {NVP6158_DET_MODE_AUTO,};
extern unsigned char ch_mode_status[16];
extern unsigned char ch_vfmt_status[16];
unsigned int gCoaxFirmUpdateFlag[16] = {0,};

u32 gpio_i2c_write(u8 da, u8 reg, u8 val)
{
	u8 ret = 0, cnt = 0;

	ret = cci_write_a8_d8(gl_sd, reg, val);
	while ((ret != 0) && (cnt < 2)) {
		ret = cci_write_a8_d8(gl_sd, reg, val);
		cnt++;
	}
	if (cnt > 0)
		sensor_print("%s sensor read retry = %d\n", gl_sd->name, cnt);

	return ret;
}

 u32 gpio_i2c_read(u8 da, u8 reg)
{
	u8 ret = 0, cnt = 0;
	u8 val = 0;

	ret = cci_read_a8_d8(gl_sd, reg, &val);
	while ((ret != 0) && (cnt < 2)) {
		ret = cci_read_a8_d8(gl_sd, reg, &val);
		cnt++;
	}
	if (cnt > 0)
		sensor_print("%s sensor read retry = %d\n", gl_sd->name, cnt);

	return val;
}

/*******************************************************************************
*	Description		: Get rev ID
*	Argurments		: dec(slave address)
*	Return value	: rev ID
*	Modify			:
*	warning			:
*******************************************************************************/
int check_nvp6158_rev(unsigned int dec)
{
	int ret;

	gpio_i2c_write(dec, 0xFF, 0x00);
	ret = gpio_i2c_read(dec, 0xf5);
	return ret;
}

/*******************************************************************************
*	Description		: Get Device ID
*	Argurments		: dec(slave address)
*	Return value	: Device ID
*	Modify			:
*	warning			:
*******************************************************************************/
int check_nvp6158_id(unsigned int dec)
{
	int ret = 0;

	gpio_i2c_write(dec, 0xFF, 0x00);
	ret = gpio_i2c_read(dec, 0xf4);
	return ret;
}

#if 0
unsigned int g_vloss = 0xFFFF;

long nvp6158_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int __user *argp = (unsigned int __user *)arg;
	int cpy2usr_ret;
	unsigned char i;
	/* unsigned char oCableDistance = 0; */
	video_equalizer_info_s s_eq_dist;
	nvp6158_opt_mode optmode;
	/* nvp6158_video_mode vmode; */
	nvp6158_chn_mode schnmode;
	nvp6158_video_adjust v_adj;
	NVP6158_INFORMATION_S vfmt;
	nvp6158_coax_str coax_val;
	nvp6158_input_videofmt_ch vfmt_ch;
	nvp6124_i2c_mode i2c;
	FIRMWARE_UP_FILE_INFO coax_fw_val;
	motion_mode motion_set;
	//int ret=0;

	/* you must skip other command to improve speed of f/w update
	 * when you are updating cam's f/w up. we need to review and test */
	/* if ( acp_dvr_checkFWUpdateStatus( cmd ) == -1 ) */
	{
		/*sensor_dbg(">>>>> DRV[%s:%d] Now cam f/w update mode. so Skip other command.\n", __func__, __LINE__ ); */
		/*return 0; */
	}
	down(&nvp6158_lock);
	switch (cmd) {
	case IOC_VDEC_SET_I2C: /* nextchip demoboard test */
		if (copy_from_user(&i2c, argp, sizeof(nvp6124_i2c_mode))) {
			up(&nvp6158_lock);
			return -1;
		}

		if (i2c.flag == 0) { /* read */
			gpio_i2c_write(i2c.slaveaddr, 0xFF, i2c.bank);
			i2c.data = gpio_i2c_read(i2c.slaveaddr, i2c.address);
		} else { /*write */
			gpio_i2c_write(i2c.slaveaddr, 0xFF, i2c.bank);
			gpio_i2c_write(i2c.slaveaddr, i2c.address, i2c.data);
		}
		if (copy_to_user(argp, &i2c, sizeof(nvp6124_i2c_mode)))
			sensor_dbg("IOC_VDEC_I2C error\n");
	break;
	case IOC_VDEC_GET_VIDEO_LOSS: /* Not use */
		/*g_vloss = nvp6158_getvideoloss(); */
		if (copy_to_user(argp, &g_vloss, sizeof(unsigned int)))
			sensor_dbg("IOC_VDEC_GET_VIDEO_LOSS error\n");
		break;
	case IOC_VDEC_GET_EQ_DIST:
		if  (copy_from_user(&s_eq_dist, argp, sizeof(video_equalizer_info_s))) {
			up(&nvp6158_lock);
			return -1;
		}

		s_eq_dist.distance = nvp6158_get_eq_dist(&s_eq_dist);
		if (copy_to_user(argp, &s_eq_dist, sizeof(video_equalizer_info_s)))
			sensor_dbg("IOC_VDEC_GET_EQ_DIST error\n");
		break;
	case IOC_VDEC_SET_EQUALIZER:
		if (copy_from_user(&s_eq_dist, argp, sizeof(video_equalizer_info_s))) {
			up(&nvp6158_lock);
			return -1;
		}
		if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_nvp6158_id[0] == NVP6158_R0_ID)
			nvp6158_set_equalizer(&s_eq_dist);
		else
			nvp6168_set_equalizer(&s_eq_dist);
		break;
	case IOC_VDEC_GET_DRIVERVER:
		if (copy_to_user(argp, &DRIVER_VER, sizeof(DRIVER_VER)))
			sensor_dbg("IOC_VDEC_GET_DRIVERVER error\n");
		break;
	case IOC_VDEC_ACP_WRITE:
		/*if (copy_from_user(&ispdata, argp, sizeof(nvp6158_acp_rw_data)))
			return -1;
		if (ispdata.opt == 0)
			acp_isp_write(ispdata.ch, ispdata.addr, ispdata.data);
		else
		{
			ispdata.data = acp_isp_read(ispdata.ch, ispdata.addr);
			if (copy_to_user(argp, &ispdata, sizeof(nvp6158_acp_rw_data)))
				sensor_dbg("IOC_VDEC_ACP_WRITE error\n");
		}*/
		break;
	case IOC_VDEC_ACP_WRITE_EXTENTION:

		break;
	case IOC_VDEC_PTZ_ACP_READ:
		/*
		//if (copy_from_user(&vfmt, argp, sizeof(nvp6158_input_videofmt)))
		//	return -1;
		//for(i=0;i<(4*nvp6158_cnt);i++)
		//{
		//	if (1)
		//	{
				// read A-CP
				//if (((g_vloss>>i)&0x01) == 0x00)
				//	acp_read(&vfmt, i);
		//	}
		//}
		//if (copy_to_user(argp, &vfmt, sizeof(nvp6158_input_videofmt)))
		//	sensor_dbg("IOC_VDEC_PTZ_ACP_READ error\n");
		*/
		break;
	case IOC_VDEC_PTZ_ACP_READ_EACH_CH:
		if (copy_from_user(&vfmt_ch, argp, sizeof(nvp6158_input_videofmt_ch))) {
			up(&nvp6158_lock);
			return -1;
		}
		/* read A-CP */
		if (((g_vloss>>vfmt_ch.ch)&0x01) == 0x00) {
			/* acp_read(&vfmt_ch.vfmt, vfmt_ch.ch); */
		}

		if (copy_to_user(argp, &vfmt_ch, sizeof(nvp6158_input_videofmt_ch)))
			sensor_dbg("IOC_VDEC_PTZ_ACP_READ_EACH_CH error\n");
		break;
	case IOC_VDEC_GET_INPUT_VIDEO_FMT:
		if (copy_from_user(&vfmt, argp, sizeof(NVP6158_INFORMATION_S))) {
			up(&nvp6158_lock);
			return -1;
		}
		if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_nvp6158_id[0] == NVP6158_R0_ID)
			video_fmt_det(vfmt.ch, &vfmt);
		else
			nvp6168_video_fmt_det(vfmt.ch, &vfmt);
		if (copy_to_user(argp, &vfmt, sizeof(NVP6158_INFORMATION_S)))
			sensor_dbg("IOC_VDEC_GET_INPUT_VIDEO_FMT error\n");
		break;
	case IOC_VDEC_SET_CHDETMODE:
			if (copy_from_user(&det_mode, argp, sizeof(unsigned char)*16)) {
				up(&nvp6158_lock);
				return -1;
			}

		for (i = 0; i < (nvp6158_cnt*4); i++) {
			sensor_dbg("IOC_VDEC_SET_CHNMODE det_mode[%d]==%d\n", i, det_mode[i]);
			if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_id[0] == NVP6158_R0_ID)
				nvp6158_set_chnmode(i, NC_VIVO_CH_FORMATDEF_UNKNOWN);
			else
				nvp6168_set_chnmode(i, NC_VIVO_CH_FORMATDEF_UNKNOWN);
		}
		break;
	case IOC_VDEC_SET_CHNMODE:
		if (copy_from_user(&schnmode, argp, sizeof(nvp6158_chn_mode))) {
			up(&nvp6158_lock);
			return -1;
		}
		if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_nvp6158_id[0] == NVP6158_R0_ID) {
			if (0 == nvp6158_set_chnmode(schnmode.ch, schnmode.chmode))
				sensor_dbg("IOC_VDEC_SET_CHNMODE OK\n");
		} else {
			if (0 == nvp6168_set_chnmode(schnmode.ch, schnmode.chmode))
				sensor_dbg("IOC_VDEC_SET_CHNMODE OK\n");
		}
		break;
	case IOC_VDEC_SET_OUTPORTMODE:
		if (copy_from_user(&optmode, argp, sizeof(nvp6158_opt_mode))) {
			up(&nvp6158_lock);
			return -1;
		}
			nvp6158_set_portmode(optmode.chipsel, optmode.portsel, optmode.portmode, optmode.chid);
		break;
	case IOC_VDEC_SET_BRIGHTNESS:
		if (copy_from_user(&v_adj, argp, sizeof(nvp6158_video_adjust))) {
			up(&nvp6158_lock);
			return -1;
		}
		/* nvp6158_video_set_brightness(v_adj.ch, v_adj.value, ch_vfmt_status[v_adj.ch]); */
		break;
	case IOC_VDEC_SET_CONTRAST:
		if (copy_from_user(&v_adj, argp, sizeof(nvp6158_video_adjust))) {
			up(&nvp6158_lock);
			return -1;
		}
		/* nvp6158_video_set_contrast(v_adj.ch, v_adj.value, ch_vfmt_status[v_adj.ch]); */
		break;
	case IOC_VDEC_SET_HUE:
		if (copy_from_user(&v_adj, argp, sizeof(nvp6158_video_adjust))) {
			up(&nvp6158_lock);
			return -1;
		}
		/* nvp6158_video_set_hue(v_adj.ch, v_adj.value, ch_vfmt_status[v_adj.ch]); */
		break;
	case IOC_VDEC_SET_SATURATION:
		if (copy_from_user(&v_adj, argp, sizeof(nvp6158_video_adjust))) {
			up(&nvp6158_lock);
			return -1;
		}
		/* nvp6158_video_set_saturation(v_adj.ch, v_adj.value, ch_vfmt_status[v_adj.ch]); */
		break;
	case IOC_VDEC_SET_SHARPNESS:
		if (copy_from_user(&v_adj, argp, sizeof(nvp6158_video_adjust))) {
			up(&nvp6158_lock);
			return -1;
		}
			nvp6158_video_set_sharpness(v_adj.ch, v_adj.value);
		break;
	/*----------------------- Coaxial Protocol ----------------------*/
	case IOC_VDEC_COAX_TX_INIT: /* SK_CHANGE 170703 */
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg("IOC_VDEC_COAX_TX_INIT error\n");
		coax_tx_init(&coax_val);
			break;
	case IOC_VDEC_COAX_TX_16BIT_INIT: /* SK_CHANGE 170703 */
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg("IOC_VDEC_COAX_TX_INIT error\n");
		coax_tx_16bit_init(&coax_val);
			break;
	case IOC_VDEC_COAX_TX_CMD_SEND: /* SK_CHANGE 170703 */
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg(" IOC_VDEC_COAX_TX_CMD_SEND error\n");
		coax_tx_cmd_send(&coax_val);
			break;
	case IOC_VDEC_COAX_TX_16BIT_CMD_SEND: /* SK_CHANGE 170703 */
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg(" IOC_VDEC_COAX_TX_CMD_SEND error\n");
		coax_tx_16bit_cmd_send(&coax_val);
			break;
	case IOC_VDEC_COAX_TX_CVI_NEW_CMD_SEND: /* SK_CHANGE 170703 */
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
				sensor_dbg(" IOC_VDEC_COAX_TX_CMD_SEND error\n");
		coax_tx_cvi_new_cmd_send(&coax_val);
			break;
	case IOC_VDEC_COAX_RX_INIT:
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg(" IOC_VDEC_COAX_RX_INIT error\n");
		coax_rx_init(&coax_val);
		break;
	case IOC_VDEC_COAX_RX_DATA_READ:
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
		sensor_dbg(" IOC_VDEC_COAX_RX_DATA_READ error\n");
		coax_rx_data_get(&coax_val);
		cpy2usr_ret = copy_to_user(argp, &coax_val, sizeof(nvp6158_coax_str));
		break;
	case IOC_VDEC_COAX_RX_BUF_CLEAR:
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg(" IOC_VDEC_COAX_RX_BUF_CLEAR error\n");
		coax_rx_buffer_clear(&coax_val);
		break;
	case IOC_VDEC_COAX_RX_DEINIT:
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
			sensor_dbg("IOC_VDEC_COAX_RX_DEINIT error\n");
		coax_rx_deinit(&coax_val);
		break;
	/*=============== Coaxial Protocol A-CP Option ===============*/
	case IOC_VDEC_COAX_RT_NRT_MODE_CHANGE_SET:
		if (copy_from_user(&coax_val, argp, sizeof(nvp6158_coax_str)))
		sensor_dbg(" IOC_VDEC_COAX_SHOT_SET error\n");
		coax_option_rt_nrt_mode_change_set(&coax_val);
		cpy2usr_ret = copy_to_user(argp, &coax_val, sizeof(nvp6158_coax_str));
		break;
	/*=========== Coaxial Protocol Firmware Update ==============*/
	case IOC_VDEC_COAX_FW_ACP_HEADER_GET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_READY_CMD_SET error\n");
		coax_fw_ready_header_check_from_isp_recv(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_READY_CMD_SET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_READY_CMD_SET error\n");
		coax_fw_ready_cmd_to_isp_send(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_READY_ACK_GET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_READY_ISP_STATUS_GET error\n");
		coax_fw_ready_cmd_ack_from_isp_recv(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_START_CMD_SET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_START_CMD_SET error\n");
		coax_fw_start_cmd_to_isp_send(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_START_ACK_GET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_START_CMD_SET error\n");
		coax_fw_start_cmd_ack_from_isp_recv(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_SEND_DATA_SET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_START_CMD_SET error\n");
		coax_fw_one_packet_data_to_isp_send(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_SEND_ACK_GET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_START_CMD_SET error\n");
		coax_fw_one_packet_data_ack_from_isp_recv(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_END_CMD_SET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_START_CMD_SET error\n");
		coax_fw_end_cmd_to_isp_send(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	case IOC_VDEC_COAX_FW_END_ACK_GET:
		if (copy_from_user(&coax_fw_val, argp, sizeof(FIRMWARE_UP_FILE_INFO)))
						sensor_dbg("IOC_VDEC_COAX_FW_START_CMD_SET error\n");
		coax_fw_end_cmd_ack_from_isp_recv(&coax_fw_val);
		cpy2usr_ret = copy_to_user(argp, &coax_fw_val, sizeof(FIRMWARE_UP_FILE_INFO));
		break;
	/*=========== Coaxial Protocol Firmware Update END ==============*/
	/*----------------------- MOTION ----------------------*/
	case IOC_VDEC_MOTION_DETECTION_GET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_SET error\n");
		motion_detection_get(&motion_set);
		cpy2usr_ret = copy_to_user(argp, &motion_set, sizeof(motion_mode));
	break;
	case IOC_VDEC_MOTION_SET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_SET error\n");
		motion_onoff_set(&motion_set);
		break;
	case IOC_VDEC_MOTION_PIXEL_SET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_Pixel_SET error\n");
		motion_pixel_onoff_set(&motion_set);
	break;
	case IOC_VDEC_MOTION_PIXEL_GET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_Pixel_SET error\n");
		motion_pixel_onoff_get(&motion_set);
		cpy2usr_ret = copy_to_user(argp, &motion_set, sizeof(motion_mode));
		break;
	case IOC_VDEC_MOTION_ALL_PIXEL_SET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_Pixel_SET error\n");
		motion_pixel_all_onoff_set(&motion_set);
	break;
	case IOC_VDEC_MOTION_TSEN_SET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_TSEN_SET error\n");
		motion_tsen_set(&motion_set);
	break;
	case IOC_VDEC_MOTION_PSEN_SET:
		if (copy_from_user(&motion_set, argp, sizeof(motion_set)))
			sensor_dbg("IOC_VDEC_MOTION_PSEN_SET error\n");
		motion_psen_set(&motion_set);
	break;
	default:
		/* sensor_dbg("drv:invalid nc decoder ioctl cmd[%x]\n", cmd); */
		break;
	}
	up(&nvp6158_lock);
	return 0;
}
#endif

#if 0
/*******************************************************************************
*	Description		: kernel thread for EQ (now, not used)
*	Argurments		: void
*	Return value	: 0
*	Modify			:
*	warning			:
*******************************************************************************/
static int nvp6158_kernel_thread(void *data)
{
	/* int ch; */
	/* nvp6158_input_videofmt videofmt; */
	NVP6158_INFORMATION_S s_nvp6158_vfmts;
	video_equalizer_info_s s_eq_info;
	unsigned char prefmt = 0, curfmt = 0, chvloss = 0;
	unsigned char ch = 0;

	memset(&s_nvp6158_vfmts, 0, sizeof(NVP6158_INFORMATION_S));

	while (!kthread_should_stop()) {
		#if 1  /* standard rutine of a process */
		down(&nvp6158_lock);
		ch = ch % (nvp6158_cnt*4);
		nvp6158_getvideoloss();
		if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_nvp6158_id[0] == NVP6158_R0_ID) {
			video_fmt_det(ch, &s_nvp6158_vfmts);
			curfmt = s_nvp6158_vfmts.curvideofmt[ch];
			prefmt = s_nvp6158_vfmts.prevideofmt[ch];
			chvloss = s_nvp6158_vfmts.curvideoloss[ch];
			/* sensor_dbg(">>>>>>%s CH[%d] chvloss = %d curfmt = %x prefmt = %x\n", __func__, ch, chvloss, curfmt, prefmt); */

			if (chvloss == 0x00) {
				if (ch_mode_status[ch] != prefmt) {
					nvp6158_set_chnmode(ch, curfmt);
					nvp6158_set_portmode(0, ch%4, NVP6158_OUTMODE_1MUX_FHD, ch%4);
					s_eq_info.Ch = ch%4;
					s_eq_info.devnum = ch/4;
					s_eq_info.FmtDef = curfmt;
					nvp6158_get_eq_dist(&s_eq_info);
					s_nvp6158_vfmts.prevideofmt[ch] = curfmt;
					sensor_dbg(">>>>>>%s CH[%d] s_eq_info.distance = %d\n", __func__, ch, s_eq_info.distance);
					nvp6158_set_equalizer(&s_eq_info);
				}
			} else {
				if (ch_mode_status[ch] != NC_VIVO_CH_FORMATDEF_UNKNOWN) {
					nvp6158_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN);
					nvp6158_set_portmode(0, ch%4, NVP6158_OUTMODE_1MUX_FHD, ch%4);
				}
			}
		} else {
			nvp6168_video_fmt_det(ch, &s_nvp6158_vfmts);
			curfmt = s_nvp6158_vfmts.curvideofmt[ch];
			prefmt = s_nvp6158_vfmts.prevideofmt[ch];
			chvloss = s_nvp6158_vfmts.curvideoloss[ch];
			/* sensor_dbg(">>>>>>%s CH[%d] chvloss = %d curfmt = %x prefmt = %x ch_mode_status[%d]=%x\n", __func__, ch, chvloss, curfmt, prefmt, ch, ch_mode_status[ch]); */

			if (chvloss == 0x00) {
				if (ch_mode_status[ch] != prefmt) {
					nvp6168_set_chnmode(ch, curfmt);
					nvp6158_set_portmode(0, ch%4, NVP6158_OUTMODE_1MUX_FHD, ch%4);
					s_eq_info.Ch = ch%4;
					s_eq_info.devnum = ch/4;
					s_eq_info.FmtDef = curfmt;
					nvp6158_get_eq_dist(&s_eq_info);
					s_nvp6158_vfmts.prevideofmt[ch] = curfmt;
					sensor_dbg(">>>>>>%s CH[%d] s_eq_info.distance = %d\n", __func__, ch, s_eq_info.distance);
					nvp6168_set_equalizer(&s_eq_info);
				}
			} else {
				if (ch_mode_status[ch] != NC_VIVO_CH_FORMATDEF_UNKNOWN) {
					nvp6168_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN);
					nvp6158_set_portmode(0, ch%4, NVP6158_OUTMODE_1MUX_FHD, ch%4);
				}
			}
		}
		ch++;
		up(&nvp6158_lock);
		#endif
		schedule_timeout_interruptible(msecs_to_jiffies(200));
		/* sensor_dbg("nvp6158_kernel_thread running\n"); */
	}

	return 0;
}
#endif

/*******************************************************************************
*	Description		: It is called when "insmod nvp61XX_ex.ko" command run
*	Argurments		: void
*	Return value	: -1(could not register nvp61XX device), 0(success)
*	Modify			:
*	warning			:
*******************************************************************************/
int nvp6158_init_hardware(int video_mode)
{
	int chip = 0;
	unsigned char ch = 0;
	video_input_auto_detect vin_auto_det;
	/* char entry[20]; */

	nvp6158_cnt = 1;
	chip_nvp6158_id[0] = NVP6158C_R0_ID;
	//* hip_id[0] = check_nvp6158_id(0x62); */
#if 0
	/* check Device ID of maxium 4chip on the slave address,
	* manage slave address. chip count. */
	for (chip = 0; chip < 4; chip++) {
		chip_nvp6158_id[chip] = check_nvp6158_id(nvp6158_iic_addr[chip]);
		rev_nvp6158_id[chip]  = check_nvp6158_rev(nvp6158_iic_addr[chip]);
		if ((chip_nvp6158_id[chip] != NVP6158_R0_ID) &&
			(chip_nvp6158_id[chip] != NVP6158C_R0_ID) &&
			(chip_nvp6158_id[chip] != NVP6168_R0_ID) &&
			(chip_nvp6158_id[chip] != NVP6168C_R0_ID)) {
			sensor_dbg("Device ID Error... %x\n", chip_nvp6158_id[chip]);
		} else {
			sensor_dbg("Device (0x%x) ID OK... %x\n", nvp6158_iic_addr[chip], chip_nvp6158_id[chip]);
			sensor_dbg("Device (0x%x) REV ... %x\n", nvp6158_iic_addr[chip], rev_nvp6158_id[chip]);
			nvp6158_iic_addr[nvp6158_cnt] = nvp6158_iic_addr[chip];
			if (nvp6158_cnt < chip)
				nvp6158_iic_addr[chip] = 0xFF;
			chip_nvp6158_id[nvp6158_cnt] = chip_id[chip];
			rev_nvp6158_id[nvp6158_cnt]  = rev_nvp6158_id[chip];
			nvp6158_cnt++;
		}
	}

	sensor_dbg("Chip Count = %d, [0x%x][0x%x][0x%x][0x%x]\n", \
		nvp6158_cnt, nvp6158_iic_addr[0], nvp6158_iic_addr[1],\
		nvp6158_iic_addr[2], nvp6158_iic_addr[3]);
#endif
	/* initialize semaphore */
	sema_init(&nvp6158_lock, 1);

	/* initialize common value of AHD */
	for (chip = 0; chip < nvp6158_cnt; chip++) {
		nvp6158_common_init(chip);
		if (chip_nvp6158_id[chip] == NVP6158C_R0_ID || chip_nvp6158_id[chip] == NVP6158_R0_ID)
			nvp6158_additional_for3MoverDef(chip);
		else {
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x01);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x97, 0x00); /* CH_RST ON */
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x97, 0x0f); /* CH_RST OFF */
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x7a, 0x0f); /* Clock Auto ON */
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xca, 0xff); /* VCLK_EN, VDO_EN */

			for (ch = 0; ch < 4; ch++) {
				gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x05 + ch);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x00, 0xd0);

				gpio_i2c_write(nvp6158_iic_addr[chip], 0x05, 0x04);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x08, 0x55);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x47, 0xEE);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x59, 0x00);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x76, 0x00);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x77, 0x80);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x78, 0x00);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x79, 0x11);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0xB8, 0xB8); /* H_PLL_BYPASS */
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x7B, 0x11); /* v_rst_on */
				gpio_i2c_write(nvp6158_iic_addr[chip], 0xb9, 0x72);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0xB8, 0xB8); /* No Video Set */

				gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x00);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x00+ch, 0x10);
				gpio_i2c_write(nvp6158_iic_addr[chip], 0x22+(ch*0x04), 0x0b);
			}
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x13);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x12, 0x04);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x2E, 0x10);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x30, 0x00);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x77, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x3a, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x3b, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x3c, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x3d, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x3e, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x3f, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x70, 0x00);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x72, 0x05);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x7A, 0x10);
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x61, 0x03);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x62, 0x00);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x63, 0x03);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x64, 0x00);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x65, 0x03);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x66, 0x00);*/
			/*gpio_ic_write(nvp6158_iic_addr[chip], 0x67, 0x03);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x68, 0x00);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x60, 0x0f);*/
			/*gpio_i2c_write(nvp6158_iic_addr[chip], 0x60, 0x00);*/
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x07, 0x47);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x59, 0x24);

			/* SAM Range */
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x74, 0x00);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x76, 0x00);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x78, 0x00);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x75, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x77, 0xff);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x79, 0xff);

			gpio_i2c_write(nvp6158_iic_addr[chip], 0x01, 0x0c);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x2f, 0xc8);

			// EQ Stage Get
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x73, 0x23);

			gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x09);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x96, 0x03);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xB6, 0x03);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xD6, 0x03);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xF6, 0x03);

			/********************************************************
			 * Audio Default Setting
			 ********************************************************/
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xff, 0x01);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x05, 0x09);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x58, 0x02);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x59, 0x00);
		}
	}

	for (ch = 0; ch < nvp6158_cnt*4; ch++) {
		/* det_mode[ch] = NVP6158_DET_MODE_AUTO; */
		det_mode[ch] = NVP6158_DET_MODE_AHD;
		vin_auto_det.ch = ch%4;
		vin_auto_det.devnum = ch/4;
		if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_nvp6158_id[0] == NVP6158_R0_ID) {
			video_input_auto_detect_set(&vin_auto_det);
			/*nvp6158_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN);*/
			nvp6158_set_chnmode(ch, video_mode);
		} else {
			nvp6168_video_input_auto_detect_set(&vin_auto_det);
			nvp6168_set_chnmode(ch, NC_VIVO_CH_FORMATDEF_UNKNOWN);
		}
	}

	for (chip = 0; chip < nvp6158_cnt; chip++) {
		if (chip_nvp6158_id[chip] == NVP6158_R0_ID || chip_nvp6158_id[chip] == NVP6168_R0_ID) {
			/*setting nvp6158 four port(0~3) being output mode enable*/
			nvp6158_set_portmode(chip, 0, NVP6158_OUTMODE_1MUX_FHD, 0);
			nvp6158_set_portmode(chip, 1, NVP6158_OUTMODE_1MUX_FHD, 1);
			nvp6158_set_portmode(chip, 2, NVP6158_OUTMODE_1MUX_FHD, 2);
			nvp6158_set_portmode(chip, 3, NVP6158_OUTMODE_1MUX_FHD, 3);
		} else { /*if(chip_nvp6158_id[chip] == NVP6158C_R0_ID) */
			/*setting nvp6158 four port(0~3) being output mode enable*/

			/*nvp6158_set_portmode(chip, 1, NVP6158_OUTMODE_2MUX_FHD, 0);
			nvp6158_set_portmode(chip, 2, NVP6158_OUTMODE_2MUX_FHD, 1);*/
			if (video_mode == AHD20_720P_25P) {
#if 1 /*BT1120*/
				nvp6158_set_portmode(chip, 0, NVP6158_OUTMODE_4MUX_BT1120S, 0);
				nvp6158_set_portmode(chip, 1, NVP6158_OUTMODE_4MUX_BT1120S, 1);
				nvp6158_set_portmode(chip, 2, NVP6158_OUTMODE_4MUX_BT1120S, 2);
				nvp6158_set_portmode(chip, 3, NVP6158_OUTMODE_4MUX_BT1120S, 3);
#else /*BT656--NVP6158_OUTMODE_1MUX_HD / NVP6158_OUTMODE_2MUX_HD / NVP6158_OUTMODE_4MUX_HD*/
				nvp6158_set_portmode(chip, 0, NVP6158_OUTMODE_1MUX_HD, 0)
#endif
			} else if (video_mode == AHD20_1080P_25P || video_mode == AHD20_1080P_30P) {
				nvp6158_set_portmode(chip, 0, NVP6158_OUTMODE_4MUX_MIX, 0);
				nvp6158_set_portmode(chip, 1, NVP6158_OUTMODE_4MUX_MIX, 1);
				nvp6158_set_portmode(chip, 2, NVP6158_OUTMODE_4MUX_MIX, 2);
				nvp6158_set_portmode(chip, 3, NVP6158_OUTMODE_4MUX_MIX, 3);
			}
		}
		for (ch = 0; ch < 4; ch++) {
			printk("nvp6158 reg init chip = %d,ch =  %d\n", chip, ch);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x00);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x23+(ch%4)*4, 0x71);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xFF, 0x05+(ch%4));
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x00, 0xd0);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x01, 0x22);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x03, 0x1f);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x47, 0x02);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x5B, 0x41);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0x50, 0x82);
			gpio_i2c_write(nvp6158_iic_addr[chip], 0xb8, 0xb9);
		}

	}

	/* initialize Audio
	 * recmaster, pbmaster, ch_num, samplerate, bits */
	if (chip_nvp6158_id[0] == NVP6158C_R0_ID || chip_nvp6158_id[0] == NVP6158_R0_ID)
		audio_init(1, 0, 16, 0, 0);
	else
		nvp6168_audio_init(1, 0, 16, 0, 0);

	/* create kernel thread for EQ, But Now not used. */
	/*if (kthread == 1)
	{
		nvp6158_kt = kthread_create(nvp6158_kernel_thread, NULL, "nvp6158_kt");
			if (!IS_ERR(nvp6158_kt))
				wake_up_process(nvp6158_kt);
			else {
				sensor_dbg("create nvp6158 watchdog thread failed!!\n");
				nvp6158_kt = 0;
				return 0;
			}
	}*/
#ifdef COLORBAR_EN
	gpio_i2c_write(nvp6158_iic_addr[0], 0xFF, 0x05);
	gpio_i2c_write(nvp6158_iic_addr[0], 0x2c, 0x08);
	gpio_i2c_write(nvp6158_iic_addr[0], 0xFF, 0x06);
	gpio_i2c_write(nvp6158_iic_addr[0], 0x2c, 0x08);
	gpio_i2c_write(nvp6158_iic_addr[0], 0xFF, 0x07);
	gpio_i2c_write(nvp6158_iic_addr[0], 0x2c, 0x08);
	gpio_i2c_write(nvp6158_iic_addr[0], 0xFF, 0x08);
	gpio_i2c_write(nvp6158_iic_addr[0], 0x2c, 0x08);

	gpio_i2c_write(nvp6158_iic_addr[0], 0xFF, 0x00);
	/* gpio_i2c_write(nvp6158_iic_addr[0], 0x78, 0x42);//ch1:Blue */
	/* ch2:Yellow ch3:Green ch4:Red */
	/* gpio_i2c_write(nvp6158_iic_addr[0], 0x79, 0x76); */
	gpio_i2c_write(nvp6158_iic_addr[0], 0x78,
				0xce); /* ch1:Blue  ch2:Yellow ch3:Green ch4:Red */
	gpio_i2c_write(nvp6158_iic_addr[0], 0x79, 0xba);
#endif
	return 0;
}
/*******************************************************************************
*	End of file
*******************************************************************************/

