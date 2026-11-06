/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SENSOR_REG_H_
#define _SENSOR_REG_H_

struct nc021_reg {
	u16 address;
	u8 val;
};
#define SNS_CFG_TYPE_MAX	21
#define MAX_SENSOR_DEVICE   6
#define MAX_I2C_BUS_NUM     7

/* Menu items for LINK_FREQ V4L2 control */
/* See V4L2_SNS_CFG_TYPE*/
static s64 nc021_link_cif_menu[MAX_SENSOR_DEVICE][SNS_CFG_TYPE_MAX] = {
	{//s0 linear mode
		SNS_CFG_TYPE_MAX,
		CAMPLL_FREQ_24M,        //1:mclk freq
		0,                      //2:mclk num
		RX_MAC_CLK_900M,        //3:mac clk
		INPUT_MODE_MIPI,        //4:input mode
		MIPI_WDR_MODE_NONE,     //5:wdr mode
		YUV422_8BIT,            //6:data type
		0,                      //7:mipi_dev
		1,                      //8:dphy.enable
		8,                     //9:dphy.hs_settle
		0,                      //10:cif phy mode
		2,                      //LANE_0
		0,                      //LANE_1
		1,                      //LANE_2
		-1,                     //LANE_3
		-1,                     //LANE_4
		0,                      //SWAP_0
		0,                      //SWAP_1
		0,                      //SWAP_2
		0,                      //SWAP_3
		0,                      //SWAP_4
	},
};

static const struct nc021_reg mode_1920x1080_regs[] = {
};

#endif //_SENSOR_REG_H_
