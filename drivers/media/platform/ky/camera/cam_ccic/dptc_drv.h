/* SPDX-License-Identifier: GPL-2.0 */
/*
 * dptc_drv.h
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __DPTC_DRV_H__
#define __DPTC_DRV_H__

#define	DPTC_I2C_PORT	0	//8
#define	TOP_SLAVE_ADDR	0x30	/*7-bit, 8-bit: 0x60 */
#define	DP_SLAVE_ADDR	0x31	/*7-bit, 8-bit: 0x62 */
#define	CSI_SLAVE_ADDR	0x32	/*7-bit, 8-bit: 0x64 */

/* top reg */
#define	VERSION_REG		0x0
#define	FUNCTION_SEL	0x4
#define	PLL_CTRL		0x8
#define	PLL_CFG0		0xC
#define	PLL_CFG1		0x10
#define	PLL_CFG2		0x14
#define	CLK_RESET		0x18

/*0x8*/
#define	PLL_CTRL_SEL	BIT(0)
#define	PLL_POWER_UP	BIT(1)
#define	RCAL_POWER_UP	BIT(2)
#define	PLL_LOCK		BIT(3)
#define	RCAL_DONE		BIT(4)
#define	RCAL_TIMEOUT	BIT(5)

/*0x18*/
#define	DSI_SW_RST		BIT(2)
#define	CSI_SW_RST		BIT(1)
#define	DPC_SW_RST		BIT(0)

/* DP PLL */
#define	PHY_SWING_CTRL	0x8C
#define	PHY_PU_CTRL		0x90
#define	PHY_CTRL0		0x94
#define	PHY_CTRL1		0x98
#define	PHY_CTRL2		0x9C

/*0x90*/
#define	PHY_PN_SWAP		BIT(30)
#define	PHY_LANE_SWAP	BIT(29)
#define	PLL_LK			BIT(28)
#define	AUX_SINGLE_END	BIT(27)
#define	PU_HPD			BIT(26)
#define	PU_AUX			BIT(25)
#define	PU_PLL			BIT(24)
#define	PU_LANE1		BIT(21)
#define	PU_LANE0		BIT(20)
#define	EN_20B_MODE		BIT(19)

/* csi register */
#define	dptc_csi_ctrl_adr		0x00
#define	dptc_csi_image_size_adr	0x04
#define	dptc_csi_gate_ctrl_adr	0x08
#define	dptc_csi_status_adr		0x0c
#define	dptc_csi_phy_ctrl_adr	0x10
#define	dptc_csi_phy_timing_adr	0x14
#define	dptc_csi_phy_ana_cfg0_adr	0x18
#define	dptc_csi_phy_ana_cfg1_adr	0x1c
#define	dptc_csi_phy_status_adr		0x20
#define	dptc_csi_sync_code_adr		0x24
#define	dptc_csi_mem_cfg_adr		0x28

#define	CSI2_DPHY5_LANE_ENA(n)	((1 << (n)) - 1)

enum {
	DPTC_FUNC_0,		/*DP PHY TEST MODE */
	DPTC_FUNC_1,		/*DP CONTROLLER + PHY TEST MODE */
	DPTC_FUNC_2,		/*CSI + DP TEST MODE */
	DPTC_FUNC_3,		/*CSI PHY TEST MODE */
	DPTC_FUNC_4,		/*DSI PHY TEST MODE */
	DPTC_FUNC_LIMIT,
};

typedef enum {
	FROM_PLL = 0,
	FROM_PAD = 1,
	FROM_REF_CLK = 2,
	FROM_RESERVE,
} CSI_ESC_CLK_SEL;

struct x1_twsi_data {
	u8 twsi_no;
	u8 reg_len;		/* byte num */
	u8 val_len;		/* byte num */
	u8 addr;		/* 7 bit i2c address */
	u16 reg;
	u32 val;
};

enum sensor_i2c_len {
	I2C_8BIT = 1,
	I2C_16BIT = 2,
	I2C_24BIT = 3,
	I2C_32BIT = 4,
};

u32 top_getverion(void);
int DPTC_func3_open(void);
int DPTC_func3_close(void);
void dptc_csi_status_handler(u32 idx);
void dptc_csi_reg_setting(u8 RAW_type, u32 sensor_width, u32 sensor_height,
			  u8 csi_lanes);
#endif /* ifndef __DPTC_DRV_H__ */
