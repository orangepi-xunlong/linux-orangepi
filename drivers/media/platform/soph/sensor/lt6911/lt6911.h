/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SENSOR_REG_H_
#define _SENSOR_REG_H_

struct lt6911_reg {
	u16 address;
	u8 val;
};

#define MAX_SENSOR_DEVICE   6
#define MAX_I2C_BUS_NUM     7

/* Menu items for LINK_FREQ V4L2 control */
/* See V4L2_SNS_CFG_TYPE*/
static s64 lt6911_link_cif_menu[MAX_SENSOR_DEVICE][SNS_CFG_TYPE_MAX] = {
	{//s0 linear mode
		SNS_CFG_TYPE_MAX,
		CAMPLL_FREQ_NONE,        //1:mclk freq
		0,                      //2:mclk num
		RX_MAC_CLK_600M,        //3:mac clk
		INPUT_MODE_MIPI,        //4:input mode
		MIPI_WDR_MODE_NONE, //5:wdr mode
		YUV422_8BIT,		    //6:data type
		0,                      //7:mipi_dev
		1,                      //8:dphy.enable
		18,                      //9:dphy.hs_settle
		0,                      //10:cif phy mode
		2,                      //LANE_0
		4,                      //LANE_1
		3,                      //LANE_2
		1,                      //LANE_3
		0,                      //LANE_4
		1,                      //SWAP_0
		1,                      //SWAP_1
		1,                      //SWAP_2
		1,                      //SWAP_3
		1,                      //SWAP_4
	},
};

static const struct lt6911_reg mode_3840x2160_8bit_regs[] = {
};

#endif //_SENSOR_REG_H_
