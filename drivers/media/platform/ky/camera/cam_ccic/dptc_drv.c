// SPDX-License-Identifier: GPL-2.0
/*
 * dptc_drv.c - Driver for dptc
 *
 * Copyright(C) 2023 KY Micro Limited.
 */

#ifdef CONFIG_ARCH_ZYNQMP
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "dptc_drv.h"
#include "dptc_pll_setting.h"

#define DPTCDELAY_US(us) usleep_range(us, us)
#define DPTCDELAY_MS(ms) DPTCDELAY_US(ms * 1000)

u8 g_current_function = DPTC_FUNC_LIMIT;

struct s_pll_setting DP_pll_configs[PLL_SSC_LIMIT][PLL_RATE_LIMIT] = {
	{			/*PLL_SSC_0 */
		{ 0x27, 0x76, 0x62, 0x3E, 0x20, 0x44, 0x85, 0x42, 0x0},	/*PLL_RATE_1620 */
		{ 0x76, 0x62, 0x7F, 0x34, 0x20, 0x4B, 0x95, 0x42, 0x0},	/*PLL_RATE_2700 */
		{ 0x9E, 0xD8, 0x61, 0x45, 0x20, 0x5B, 0xA5, 0x42, 0x0},	/*PLL_RATE_5400 */
	},
	{			/*PLL_SSC_5000 */
		{ 0x27, 0x76, 0x62, 0x3E, 0x2A, 0x44, 0x85, 0x42, 0x0},	/*PLL_RATE_1620 */
		{ 0x76, 0x62, 0x7F, 0x34, 0x2A, 0x4B, 0x95, 0x42, 0x0},	/*PLL_RATE_2700 */
		{ 0x9E, 0xD8, 0x61, 0x45, 0x2A, 0x5B, 0xA5, 0x42, 0x0},	/*PLL_RATE_5400 */
	}
};

typedef enum {
	STANDARD_MODE = 0,	/*100Kbps */
	FAST_MODE,		/*400Kbps */
	HS_MODE,		/*3.4 Mbps slave/3.3 Mbps master,standard mode when not doing a high speed transfer */
	HS_MODE_FAST,		/*3.4 Mbps slave/3.3 Mbps master,fast mode when not doing a high speed transfer */
} I2C_FAST_MODE;

struct x1_twsi_data dptc_i2c_data;
static DEFINE_MUTEX(cmd_mutex);

u32 swap_n(u32 addr, unsigned int cmdlen)
{
	u32 res;

	switch (cmdlen) {
	case 1:
		res = addr;
		break;
	case 2:
		res = (addr & 0xff) << 8 | ((addr >> 8) & 0xff);
		break;
	case 3:
		res = (addr & 0xff) << 16 | (addr & 0xff00) | ((addr >> 16) & 0xff);
		break;
	case 4:
		res = (addr & 0xff) << 24 | ((addr & 0xff00) << 8) |
		      ((addr & 0xff0000) >> 8) | ((addr >> 24) & 0xff);
		break;
	default:
		res = addr;
		break;
	}

	return res;
}

int twsi_write_i2c(struct x1_twsi_data *data)
{
	int ret = 0;
	struct i2c_adapter *adapter;
	struct i2c_msg msg;
	u8 val[8];
	int i, j = 0;

	if (!data || !data->addr || !data->reg_len || !data->val_len) {
		pr_err("Error: %s, %d", __func__, __LINE__);
		return -EINVAL;
	}

	msg.addr = data->addr;
	msg.flags = 0;
	msg.len = data->reg_len + data->val_len;
	msg.buf = val;

	adapter = i2c_get_adapter(data->twsi_no);
	if (!adapter)
		return -1;

	mutex_lock(&cmd_mutex);
	for (i = 0; i < data->reg_len; i++)
		val[j++] = ((u8 *) (&data->reg))[i];
	for (i = 0; i < data->val_len; i++)
		val[j++] = ((u8 *) (&data->val))[i];
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(&cmd_mutex);
		return ret;
	}
	mutex_unlock(&cmd_mutex);

	return ret;
}

int twsi_read_i2c(struct x1_twsi_data *data)
{
	struct i2c_adapter *adapter;
	struct i2c_msg msg;
	int ret = 0;
	u8 val[4];

	if (!data || !data->addr || !data->reg_len || !data->val_len) {
		pr_err("%s, error param", __func__);
		return -EINVAL;
	}

	msg.addr = data->addr;
	msg.flags = 0;
	msg.len = data->reg_len;
	msg.buf = val;

	adapter = i2c_get_adapter(data->twsi_no);
	if (!adapter)
		return -1;

	mutex_lock(&cmd_mutex);
	if (data->reg_len == I2C_8BIT) {
		val[0] = data->reg & 0xff;
	} else if (data->reg_len == I2C_16BIT) {
		val[0] = (data->reg >> 8) & 0xff;
		val[1] = data->reg & 0xff;
	}
	msg.len = data->reg_len;
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(&cmd_mutex);
		goto err;
	}

	msg.flags = I2C_M_RD;
	msg.len = data->val_len;
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(&cmd_mutex);
		goto err;
	}

	if (data->val_len == I2C_8BIT)
		data->val = val[0];
	else if (data->val_len == I2C_16BIT)
		data->val = (val[0] << 8) + val[1];
	else if (data->val_len == I2C_32BIT)
		data->val = (val[3] << 24) + (val[2] << 16) + (val[1] << 8) + val[0];
	//pr_info("twsi_read_i2c: val[0]=0x%x,val[1]=0x%x,val[2]=0x%x,val[3]=0x%x\n",val[0],val[1],val[2],val[3]);
	mutex_unlock(&cmd_mutex);

	return 0;

err:
	pr_err("Failed reading register 0x%02x!", data->reg);
	return ret;
}

void TWSI_Init(I2C_FAST_MODE mode, unsigned int i2c_no)
{
	dptc_i2c_data.twsi_no = i2c_no;
	dptc_i2c_data.addr = 0x6c;
	dptc_i2c_data.reg_len = 1;	//I2C_8BIT;
	dptc_i2c_data.val_len = 4;	//I2C_32BIT;
}

int TWSI_REG_READ_DPTC(u8 i2c_no, u8 slaveaddress, u8 addr)
{
	int ret = 0;

	dptc_i2c_data.twsi_no = i2c_no;
	dptc_i2c_data.addr = slaveaddress;
	dptc_i2c_data.reg = addr;
	dptc_i2c_data.val = 0x00;
	ret = twsi_read_i2c(&dptc_i2c_data);
	if (ret == 0)
		return dptc_i2c_data.val;
	else
		return -1;
}

int TWSI_REG_WRITE_DPTC(u8 i2c_no, u8 slaveaddress, u8 addr, u32 data)
{
	int ret = 0;

	dptc_i2c_data.twsi_no = i2c_no;
	dptc_i2c_data.addr = slaveaddress;
	dptc_i2c_data.reg = addr;
	dptc_i2c_data.val = data;
	ret = twsi_write_i2c(&dptc_i2c_data);
	if (ret == 0)
		return 1;
	else
		return -1;
}

int TWSI_REG_WRITE_CAM(u8 i2c_no, u8 slaveaddress, u16 addr, u8 data)
{
#if 0
	u32 twsi_param = 0x21;	/*2 byte address, 1 byte data */
	int ret = 0;

	ret = TWSIput(i2c_no, slaveaddress, addr, data, &twsi_param);
	if (ret == 0)
		return 1;
	else
		return -1;
#else
	return -1;
#endif
}

int TWSI_REG_READ_CAM(u8 i2c_no, u8 slaveaddress, u16 addr)
{
#if 0
	u32 twsi_param = 0x21;	/*2 byte address, 1 byte data */
	int ret = 0;

	ret = TWSIget(i2c_no, slaveaddress, addr, &twsi_param);
	if (ret == 0)
		return twsi_param;
	else
		return -1;
#else
	return -1;
#endif
}

static u32 top_read(u8 reg)
{
	return TWSI_REG_READ_DPTC(DPTC_I2C_PORT, TOP_SLAVE_ADDR, reg >> 2);
}

static void top_write(u8 reg, u32 data)
{
	//pr_info("DPTC:top_write: 0x%x = 0x%x\r\n", reg, data);
	TWSI_REG_WRITE_DPTC(DPTC_I2C_PORT, TOP_SLAVE_ADDR, reg >> 2, data);
}

static void top_set_bits(u8 reg, u32 bits)
{
	top_write(reg, top_read(reg) | bits);
}

static void top_clear_bits(u8 reg, u32 bits)
{
	top_write(reg, top_read(reg) & ~bits);
}

static void top_write_bits(u8 reg, u32 value, u8 shifts, u8 width)
{
	u32 reg_val;
	u32 mask = 0;

	mask = (1 << width) - 1;
	reg_val = top_read(reg);
	reg_val &= ~(mask << shifts);
	reg_val |= (value & mask) << shifts;
	top_write(reg, reg_val);
}

u32 top_getverion(void)
{
	u32 reg = 0;
	u32 version = 0;

	pr_info("DPTC:top_getverion ++\r\n");
	reg = top_read(VERSION_REG);
	pr_info("DPTC:top_getverion 0x0 = 0x%x\r\n", reg);

	version = reg & 0xFF;
	pr_info("DPTC:top_getverion version is 0x%x\r\n", version);

	return version;
}

int32_t top_powerup(void)
{
	u32 reg = 0;
	u32 timeout = 1000;

	pr_info("DPTC:top_powerup ++\r\n");

	reg = top_read(PLL_CTRL);
	while (0 == (reg & (RCAL_DONE | RCAL_TIMEOUT)) && timeout > 0) {
		timeout--;
		DPTCDELAY_US(10);
		reg = top_read(PLL_CTRL);
	}
	if (timeout == 0) {
		pr_err("DPTC:top_powerup timeout(0x%x)!\r\n", reg);
		return -1;
	}
	pr_info("DPTC:top_powerup OK (0x%x)!\r\n", reg);
	return 0;
}

int32_t top_init(u8 func)
{
	pr_info("DPTC:top_init ++ (%d)\r\n", func);

	g_current_function = func;

	switch (func) {
	case DPTC_FUNC_0:	/*DP PHY TEST MODE */
		top_write(FUNCTION_SEL, 0);
		top_clear_bits(PLL_CTRL, PLL_CTRL_SEL);	/*DP control pll */
		top_write(CLK_RESET, 0);	/*reset devices */
		DPTCDELAY_MS(5);
		top_write(CLK_RESET, DPC_SW_RST);	/*release DP */
		break;
	case DPTC_FUNC_1:	/*DP CONTROLLER + PHY TEST MODE */
		top_write(FUNCTION_SEL, 1);
		top_clear_bits(PLL_CTRL, PLL_CTRL_SEL);	/*DP control pll */
		top_write(CLK_RESET, 0);	/*reset devices */
		DPTCDELAY_MS(5);
		top_write(CLK_RESET, DPC_SW_RST);	/*release DP */
		break;
	case DPTC_FUNC_2:	/*CSI + DP TEST MODE */
		top_write(FUNCTION_SEL, 2);
		top_clear_bits(PLL_CTRL, PLL_CTRL_SEL);	/*DP control pll */
		top_write(CLK_RESET, 0);	/*reset devices */
		DPTCDELAY_MS(5);
		top_write(CLK_RESET, DPC_SW_RST | CSI_SW_RST);	/*release DP & CSI */
		break;
	case DPTC_FUNC_3:	/*CSI PHY TEST MODE */
		top_write(FUNCTION_SEL, 3);
		top_set_bits(PLL_CTRL, PLL_CTRL_SEL);	/*TOP control pll */
		top_write(CLK_RESET, 0);	/*reset devices */
		DPTCDELAY_US(5);
		top_write(CLK_RESET, CSI_SW_RST | (FROM_PLL << 4));	/*release CSI */
		break;
	case DPTC_FUNC_4:	/*DSI PHY TEST MODE */
		top_write(FUNCTION_SEL, 4);
		top_set_bits(PLL_CTRL, PLL_CTRL_SEL);	/*TOP control pll */
		top_write(CLK_RESET, 0);	/*reset devices */
		DPTCDELAY_US(5);
		top_write(CLK_RESET, DSI_SW_RST);	/*release DP & DSI */
		break;
	default:
		g_current_function = DPTC_FUNC_LIMIT;
		pr_err("DPTC:top_sel_func Invalid param(%d)!\r\n", func);
		return -1;
	}
	return 0;
}

static void top_enable_clks(u32 clks, u32 enable)
{
	u32 reg = 0;

	pr_info("top_enable_clks ++ (0x%x, enable=%d)\r\n", clks, enable);

	if (0 != (clks & DP_CLK_DSI))
		reg |= PLL_CLK_DSI;
	if (0 != (clks & DP_CLK_CSI))
		reg |= PLL_CLK_CSI;
	if (0 != (clks & DP_CLK_LS))
		reg |= PLL_CLK_LS;
	if (0 != (clks & DP_CLK_ESC))
		reg |= PLL_CLK_ESC;
	if (0 != (clks & DP_CLK_20X_40X))
		reg |= PLL_CLK_20X_40X;

	if (enable)
		top_write_bits(PLL_CFG2, reg, 8, 8);
	else
		top_clear_bits(PLL_CFG2, reg << 8);
}

static int top_set_pllrate(u8 freq, u8 ssc)
{
	struct s_pll_setting *pll = NULL;
	u32 reg0 = 0, reg1 = 0, reg2 = 0;
	u32 reg = 0;
	u32 timeout = 50;

	pr_info("DP:top_set_pllrate ++ (%d)\r\n", freq);

	if (freq >= PLL_RATE_LIMIT || ssc >= PLL_SSC_LIMIT) {
		pr_err("DP:top_set_pllrate Invalid param(%d)!\r\n", freq);
		return -1;
	}

	pll = &DP_pll_configs[ssc][freq];

	reg0 = (pll->reg3 << 24) | (pll->reg2 << 16) | (pll->reg1 << 8) | pll->reg0;
	reg1 = (pll->reg7 << 24) | (pll->reg6 << 16) | (pll->reg5 << 8) | pll->reg4;
	reg2 = pll->reg8;

	/*power down pll */
	top_clear_bits(PLL_CTRL, PLL_POWER_UP);

	/*config pll */
	top_write(PLL_CFG0, reg0);
	top_write(PLL_CFG1, reg1);
	top_write_bits(PLL_CFG2, reg2, 0, 8);

	/*power up pll */
	top_set_bits(PLL_CTRL, PLL_POWER_UP);

	/*wait for pll_lk */
	reg = top_read(PLL_CTRL);
	while (((reg & PLL_LOCK) != PLL_LOCK) && timeout > 0) {
		timeout--;
		DPTCDELAY_US(10);
		reg = top_read(PLL_CTRL);
	}

	if (timeout == 0) {
		pr_err("DP:top_set_pllrate pll lock timeout(0x%x)!\r\n", reg);
		return -1;
	}
	pr_info("DP:top_set_pllrate pll locked at %d KHz\r\n", freq);

	return 0;
}

int32_t top_pll_init(u32 func)
{
	int ret = 0;

	pr_info("top_pll_init (func: %d)+++!\r\n", func);

	switch (func) {
	case DPTC_FUNC_0:	/*DP PHY TEST MODE */
		break;
	case DPTC_FUNC_1:	/*DP CONTROLLER + PHY TEST MODE */
		break;
	case DPTC_FUNC_2:	/*CSI + DP TEST MODE */
		break;
	case DPTC_FUNC_3:	/*CSI PHY TEST MODE */
		top_enable_clks(DP_CLK_FUNC3, 1);
		ret = top_set_pllrate(PLL_RATE_5400, PLL_SSC_0);	//PLL_SSC_5000
		break;
	case DPTC_FUNC_4:	/*DSI PHY TEST MODE */
		break;
	default:
		g_current_function = DPTC_FUNC_LIMIT;
		pr_err("top_pll_init Invalid param(%d)!\r\n", func);
		return -1;
	}

	if (ret != 0)
		pr_info("top_pll_init fail (%d)!\r\n", ret);

	return ret;
}

static u32 dp_read(u8 reg)
{
	return TWSI_REG_READ_DPTC(DPTC_I2C_PORT, DP_SLAVE_ADDR, reg >> 2);
}

static void dp_write(u8 reg, u32 data)
{
	//pr_info("DPTC:top_write: 0x%x = 0x%x\r\n", reg, data);
	TWSI_REG_WRITE_DPTC(DPTC_I2C_PORT, DP_SLAVE_ADDR, reg >> 2, data);
}

static void dp_set_bits(u8 reg, u32 bits)
{
	dp_write(reg, dp_read(reg) | bits);
}

static void dp_clear_bits(u8 reg, u32 bits)
{
	dp_write(reg, dp_read(reg) & ~bits);
}

static void dp_write_bits(u8 reg, u32 value, u8 shifts, u8 width)
{
	u32 reg_val;
	u32 mask = 0;

	mask = (1 << width) - 1;
	reg_val = dp_read(reg);
	reg_val &= ~(mask << shifts);
	reg_val |= (value & mask) << shifts;
	dp_write(reg, reg_val);
}

static void dp_enable_clks(u32 clks, u32 enable)
{
	u32 reg = 0;

	pr_info("DP:DP_enable_clks ++ (0x%x, enable=%d)\r\n", clks, enable);

	if (0 != (clks & DP_CLK_DSI))
		reg |= PLL_CLK_DSI;
	if (0 != (clks & DP_CLK_CSI))
		reg |= PLL_CLK_CSI;
	if (0 != (clks & DP_CLK_LS))
		reg |= PLL_CLK_LS;
	if (0 != (clks & DP_CLK_ESC))
		reg |= PLL_CLK_ESC;
	if (0 != (clks & DP_CLK_20X_40X))
		reg |= PLL_CLK_20X_40X;

	if (enable)
		dp_write_bits(PHY_CTRL2, reg, 8, 6);
	else
		dp_clear_bits(PHY_CTRL2, reg << 8);
}

static int dp_set_pllrate(u8 freq, u8 ssc)
{
	struct s_pll_setting *pll = NULL;
	u32 reg0 = 0, reg1 = 0, reg2 = 0;
	u32 reg = 0;
	u32 timeout = 50;

	pr_info("DP:DP_set_pllrate ++ (%d)\r\n", freq);

	if (freq >= PLL_RATE_LIMIT || ssc >= PLL_SSC_LIMIT) {
		pr_err("DP:DP_set_pllrate Invalid param(%d)!\r\n", freq);
		return -1;
	}

	pll = &DP_pll_configs[ssc][freq];
	reg0 = (pll->reg3 << 24) | (pll->reg2 << 16) | (pll->reg1 << 8) | pll->reg0;
	reg1 = (pll->reg7 << 24) | (pll->reg6 << 16) | (pll->reg5 << 8) | pll->reg4;
	reg2 = pll->reg8;

	/*power down pll */
	dp_clear_bits(PHY_PU_CTRL, PU_HPD | PU_AUX | PU_LANE1 | PU_LANE0 | PU_PLL);

	/*config pll */
	dp_write(PHY_CTRL0, reg0);
	dp_write(PHY_CTRL1, reg1);
	dp_write_bits(PHY_CTRL2, reg2, 0, 8);

	/*power up pll, aux, hpd */
	dp_set_bits(PHY_PU_CTRL, PU_HPD | PU_AUX | PU_PLL);

	/*wait for pll_lk */
	reg = dp_read(PHY_PU_CTRL);
	while (((reg & PLL_LK) != PLL_LK) && timeout > 0) {
		timeout--;
		DPTCDELAY_US(10);
		reg = dp_read(PHY_PU_CTRL);
	}

	if (timeout == 0) {
		pr_err("DP:DP_set_pllrate pll lock timeout(0x%x)!\r\n", reg);
		return -1;
	}
	pr_info("DP:DP_set_pllrate pll locked at %d KHz\r\n", freq);

	return 0;
}

int32_t dp_init(u32 func)
{
	int ret = 0;

	pr_info("DP:DP_init (func: %d)+++!\r\n", func);

	switch (func) {
	case DPTC_FUNC_0:	/*DP PHY TEST MODE */
		break;
	case DPTC_FUNC_1:	/*DP CONTROLLER + PHY TEST MODE */
		break;
	case DPTC_FUNC_2:	/*CSI + DP TEST MODE */
		dp_enable_clks(DP_CLK_ALL, 1);
		ret = dp_set_pllrate(PLL_RATE_5400, PLL_SSC_5000);
		break;
	case DPTC_FUNC_3:	/*CSI PHY TEST MODE */
		dp_enable_clks(DP_CLK_FUNC3, 1);
		ret = dp_set_pllrate(PLL_RATE_5400, PLL_SSC_5000);
		break;
	case DPTC_FUNC_4:	/*DSI PHY TEST MODE */
		break;
	default:
		g_current_function = DPTC_FUNC_LIMIT;
		pr_err("EDP:top_sel_func Invalid param(%d)!\r\n", func);
		return -1;
	}

	if (ret != 0)
		pr_err("DP:DP_init fail (%d)!\r\n", ret);

	return ret;
}

static u32 dptc_csi_read(u8 reg)
{
	return TWSI_REG_READ_DPTC(DPTC_I2C_PORT, CSI_SLAVE_ADDR, reg >> 2);
}

static void dptc_csi_write(u8 reg, u32 data)
{
	//pr_info("DPTC:top_write: 0x%x = 0x%x\r\n", reg, data);
	TWSI_REG_WRITE_DPTC(DPTC_I2C_PORT, CSI_SLAVE_ADDR, reg >> 2, data);
}

#if 0
static void dptc_csi_set_bits(u8 reg, u32 bits)
{
	dptc_csi_write(reg, dptc_csi_read(reg) | bits);
}

static void dptc_csi_clear_bits(u8 reg, u32 bits)
{
	dptc_csi_write(reg, dptc_csi_read(reg) & ~bits);
}
#endif
static void dptc_csi_write_bits(u8 reg, u32 value, u8 shifts, u8 width)
{
	u32 reg_val;
	u32 mask = 0;

	mask = (1 << width) - 1;
	reg_val = top_read(reg);
	reg_val &= ~(mask << shifts);
	reg_val |= (value & mask) << shifts;
	dptc_csi_write(reg, reg_val);
}

void dptc_csi_dphy_setting(u8 lane_num)
{
	dptc_csi_write_bits(dptc_csi_phy_ctrl_adr, 0x01, 8, 1);	//analog power on:bit8
	//bit[31:30] adjust the 25uA bias current for HSRX.  10b:def, 11b:max
	//bit[18:16] hsrx termination adjustment.
	dptc_csi_write(dptc_csi_phy_ana_cfg0_adr, 0xa28c8888);	//0xa2848888 //0xe2848888 //0xa2848888
	dptc_csi_write(dptc_csi_phy_timing_adr, 0x15001500);	//thomaszhang dptc_csi_write_bits(dptc_csi_phy_timing_adr, 0x00000820, 0, 16);// 0x00001501 bit[15:8]:HS_SETTLE, bit[7:0]:HS_TERM_ENA xUI

	if (lane_num == 1)	//bit[3:0]:Lane Enable,Each bit controls each of the MIPI lane
		dptc_csi_write_bits(dptc_csi_phy_ctrl_adr, 0x11, 0, 8);
	else if (lane_num == 2)
		dptc_csi_write_bits(dptc_csi_phy_ctrl_adr, 0x33, 0, 8);
	else if (lane_num == 4)
		dptc_csi_write_bits(dptc_csi_phy_ctrl_adr, 0xff, 0, 8);
}

void dptc_csi_reg_setting(u8 RAW_type, u32 sensor_width, u32 sensor_height,
			  u8 csi_lanes)
{
	u32 csi2_laneend = 0;
	u32 csi2_eccend = 0;
	u32 csi2_eot = 0;
	u32 csi2_parse_error = 0;
	u32 csi2_vsynclen = 4;
	u32 csi2_hsynclen = 4;
//      u32 csi2_ana_pu = 1;
	u32 csi2_hs_settle = 0x8;
	u32 csi2_hs_termen = 0x20;
	u32 csi2_ck_settle = 0x10;
	u32 csi2_ck_termen = 0x1;
//      u32 csi2_dsi_mode = 0x0; //1: dsi mode; 0:csi mode
//      u32 csi2_packet_code = 0x2b;
//      u32 csi2_hsync_start_code = 0x02;
//      u32 csi2_vsync_start_code = 0x00;

	dptc_csi_write(dptc_csi_ctrl_adr,
		       ((csi2_hsynclen << 10) + (csi2_vsynclen << 6) +
			(csi2_parse_error << 5) + (csi2_eccend << 4) + (csi2_eot << 3) +
			(csi2_laneend << 2) + (csi_lanes - 1)));
	dptc_csi_write(dptc_csi_image_size_adr, (sensor_height << 16) + sensor_width);

	dptc_csi_write(dptc_csi_status_adr, 0xffffffff);

//      dptc_csi_write(dptc_csi_phy_ctrl_adr, ((csi2_ana_pu << 8) + (CSI2_DPHY5_LANE_ENA(csi_lanes) << 4) + CSI2_DPHY5_LANE_ENA(csi_lanes)));
	dptc_csi_write(dptc_csi_phy_timing_adr,
		       ((csi2_ck_settle << 24) + (csi2_ck_termen << 16) +
			(csi2_hs_settle << 8) + csi2_hs_termen));
	dptc_csi_write(dptc_csi_phy_ana_cfg0_adr, 0xa28c8888);
	dptc_csi_write(dptc_csi_phy_ana_cfg1_adr, 0x00000000);
	dptc_csi_write(dptc_csi_status_adr, 0xffffffff);
	dptc_csi_write_bits(dptc_csi_phy_status_adr, 0x7f, 1, 7);	//dptc_csi_write(dptc_csi_phy_status_adr, 0x0000000f);
//      dptc_csi_write(dptc_csi_sync_code_adr, (csi2_dsi_mode << 24) + (csi2_packet_code << 16) + (csi2_hsync_start_code << 8) + csi2_vsync_start_code);
	dptc_csi_dphy_setting(csi_lanes);
}

int DPTC_func3_open(void)
{
	s32 ret = 0;

	pr_info("DPTC: DPTC_func3_open enter!\n");

	TWSI_Init(STANDARD_MODE, DPTC_I2C_PORT);	//STANDARD_MODE
	DPTCDELAY_US(100);	//thomaszhang DPTCDELAY_MS(100);

	ret = top_powerup();
	if (ret != 0) {
		pr_err("DPTC: DPTC_func3_open: top_powerup fail!\r\n");
		return -1;
	}
	top_getverion();
	ret = top_init(DPTC_FUNC_3);
	if (ret != 0) {
		pr_err("DPTC: DPTC_func3_open: top_init fail!\r\n");
		return -1;
	}
	ret = top_pll_init(DPTC_FUNC_3);
	if (ret != 0) {
		pr_err("DPTC: DPTC_func3_open: top_pll_init fail!\r\n");
		return -1;
	}

	return 0;
}

int DPTC_func3_close(void)
{
	return 0;
}

void dptc_csi_status_handler(u32 idx)
{
	u32 irqs, i;

	DPTCDELAY_MS(1000);

	for (i = 0; i <= 0x28; i += 4) {
		irqs = dptc_csi_read(i);
		pr_info("dptc_csi_status_adr: 0x%x = 0x%08x\n", i, irqs);
	}

	for (i = 0; i <= 0x18; i += 4) {
		irqs = top_read(i);
		pr_info("top_read: 0x%x = 0x%08x\n", i, irqs);
	}
	dptc_csi_write(dptc_csi_status_adr, 0xffffffff);
	dptc_csi_write_bits(dptc_csi_phy_status_adr, 0x7f, 1, 7);
}

void dptc_csi_status_read(void)
{
	u32 irqs;

	DPTCDELAY_MS(1000);

	irqs = dptc_csi_read(dptc_csi_status_adr);
	pr_info("dptc_csi_status_adr: 0x%x = 0x%08x\n", dptc_csi_status_adr, irqs);
}
#endif
