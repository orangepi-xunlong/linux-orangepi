/*
 * A V4L2 driver for TD100 YUV cameras.
 *
 * Copyright (c) 2020 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Lihuiyu<lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _TD100_DRV_H_
#define _TD100_DRV_H_

#include <media/sunxi_camera_v2.h>
#include "../sensor_helper.h"

#define CCI_TO_AUDIO

#define SRC_27MHz
// #define SUPPORT_3D_FILTER
/*set 3d filter channel as 3 for car reserver*/
#define TVD_3D_CH               3       /*0 - 3*/
#define _3D_FILTRE_ID	0x181

enum AGC_STATE{
	AGC_CLOSE = 0,
	AGC_OPEN = 1,
};

enum {
	Set_cb = 0,
	Set_cr,
};

enum {
	TVD_CH0 = 0,
	TVD_CH1 = 1,
	TVD_CH2 = 2,
	TVD_CH3 = 3,
	TVD_CH_MAX = 4,
};

enum {
	ADC_CH0 = 0x01,
	ADC_CH1 = 0x02,
	ADC_CH2 = 0x04,
	ADC_CH3 = 0x08,
};

enum {
	TVD_4CH_CVBS = 0x9100,
	TVD_CVBS_AND_YPRBPR = 0x9200,
};

#define TVD_4CH_CVBS		0x9100
#define TVD_CVBS_AND_YPRBPR	0x9200

#define INPUT_FMT_REG		0x8e01
#define CONTRAST_REG		0x5100
#define BRIGTH_REG		0x5200
#define SATURATION_REG		0x5400
#define CB_REG			0x5800
#define CR_EN_REG		0x5a00

#define TD100_TVD_TOP_BASE              0x7000
#define TD100_TVD0_BASE                 0x8000
#define TD100_PSRAM_BASE		0x2000

/*************TD100 PSRAM CONTROL REG***************/
#define MEM_COM_CFG                      0X000         /*default:0x0000_0081*/
#define CACHE_CFG                        0x008         /*default:0x1100_900d*/
#define MEM_AC_CFG                       0x00C         /*default:0x0100_0505*/
#define C_READ_CFG                       0x010         /*default:0x0300_0000*/
#define C_WRITE_CFG                      0x014         /*default:0x0200_0000*/
#define S_CMD_CFG                        0x02C         /*default:0x0000_0000*/
#define S_ADDR_CFG                       0x030         /*default:0x0000_0000*/
#define S_WR_NUM                         0x040         /*default:0x0000_0000*/
#define S_RD_NUM                         0x044         /*default:0x0000_0000*/
#define START_SEND_REG                   0x048         /*default:0x0000_0000*/
#define FIFO_TRIG_LEV                    0x04C         /*default:0x300f_300f*/
#define FIFO_STA_REG                     0x050         /*default:0x1c00_0000*/
#define INT_EN_REG                       0x054         /*default:0x0000_0000*/
#define INT_STA_REG                      0x058         /*default:0x0000_0000*/
#define DEBUG_STA                        0x060         /*default:0x0000_0000*/
#define DEBUG_CNT_SBUS                   0x064         /*default:0x0000_0000*/
#define DEBUG_CNT_CBUS                   0x068         /*default:0x0000_0000*/
#define PSRAM_FORCE_CFG                  0x06C         /*default:0x0000_0000*/
#define PSRAM_COM_CFG                    0x070         /*default:0xC1C0_2003*/
#define PSRAM_LAT_CFG                    0x074         /*default:0x0020_0000*/
#define PSRAM_TIM_CFG                    0x078         /*default:0x0012_1111*/
#define PSRAM_DQS0_IN_DLY                0x07C         /*default:0x0000_0200*/
#define PSRAM_DQS1_IN_DLY                0x0BC         /*default:0x0000_0200*/
#define PSRAM_CLK_OUT_DLY                0x0C0         /*default:0x0102_0200*/
#define PSRAM_MIS_CFG                    0x0C4         /*default:0x0000_0702*/
#define S_MEM_REG_SEL                    0x0C8         /*default:0x0000_0001*/
#define S_WDATA_REG                      0x100         /*default:0x0000_0000*/
#define S_RDATA_REG                      0x200         /*default:0x0000_0000*/


extern struct v4l2_subdev *td100_sd;
extern unsigned int sensor_type;

int td100_set_ch_fmt(struct v4l2_subdev *sd,
	struct tvin_init_info *info, int ch, int fmt);
int td100_hw_regs_init(struct v4l2_subdev *sd,
	struct tvin_init_info *info);
int __td100_get_input_fmt(struct v4l2_subdev *sd, int ch, int *value);
int td100_agc_control(int ch, enum AGC_STATE agc_state, int w_value);
extern int td100_cci_read_a16_d8(unsigned short addr, unsigned char *value);
extern int td100_cci_write_a16_d8(unsigned short addr, unsigned char value);
extern int tvd_reg_r_a16_d16(unsigned short addr, unsigned short *value);

#endif
