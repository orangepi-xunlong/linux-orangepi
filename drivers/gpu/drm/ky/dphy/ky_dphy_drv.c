// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "../dsi/ky_dsi_hw.h"
#include "../ky_dphy.h"
#include "../dsi/ky_dptc_drv.h"

static unsigned int ky_dphy_lane[5] = {0, 0x1, 0x3, 0x7, 0xf};

static void dphy_ana_reset(void __iomem *base_addr)
{
	dsi_clear_bits(base_addr, DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
	udelay(5);
	dsi_set_bits(base_addr, DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
}

static void dphy_set_power(void __iomem *base_addr, bool poweron)
{
	if(poweron) {
		dsi_set_bits(base_addr, DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
		dsi_set_bits(base_addr, DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_PU);
	} else {
		dsi_clear_bits(base_addr, DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_PU);
		dsi_clear_bits(base_addr, DSI_PHY_ANA_PWR_CTRL, CFG_DPHY_ANA_RESET);
	}
}

static void dphy_set_cont_clk(void __iomem *base_addr, bool cont_clk)
{
#ifdef DPTC_DPHY_TEST
	uint32_t tmp;

	if(cont_clk) {
		tmp = dptc_dsi_read(0x04);
		tmp |= CFG_DPHY_CONT_CLK;
		//dptc_dsi_write(0x04, tmp);
	} else {
		tmp = dptc_dsi_read(0x04);
		tmp &= (~CFG_DPHY_CONT_CLK);
		//dptc_dsi_write(0x04, tmp);
	}
	dptc_dsi_write(0x04, 0x30001);
#else
	if(cont_clk)
		dsi_set_bits(base_addr, DSI_PHY_CTRL_1, CFG_DPHY_CONT_CLK);
	else
		dsi_clear_bits(base_addr, DSI_PHY_CTRL_1, CFG_DPHY_CONT_CLK);

	dsi_set_bits(base_addr, DSI_PHY_CTRL_1, CFG_DPHY_ADD_VALID);
	dsi_set_bits(base_addr, DSI_PHY_CTRL_1, CFG_DPHY_VDD_VALID);
#endif
}

static void dphy_set_lane_num(void __iomem *base_addr, uint32_t lane_num)
{
#ifdef DPTC_DPHY_TEST
	uint32_t tmp;

	tmp = dptc_dsi_read(0x08);
	tmp &= ~CFG_DPHY_LANE_EN_MASK;
	tmp |= ky_dphy_lane[lane_num] << CFG_DPHY_LANE_EN_SHIFT;
	dptc_dsi_write(0x08, 0);
	tmp = dptc_dsi_read(0x08);
	dptc_dsi_write(0x08, 0x30);
#endif
	dsi_write_bits(base_addr, DSI_PHY_CTRL_2,
		CFG_DPHY_LANE_EN_MASK, ky_dphy_lane[lane_num] << CFG_DPHY_LANE_EN_SHIFT);
}

static void dphy_set_bit_clk_src(void __iomem *base_addr, uint32_t bit_clk_src,
	uint32_t half_pll5)
{
#ifdef DPTC_DPHY_TEST
	uint32_t tmp;
#endif

	if(bit_clk_src >= DPHY_BIT_CLK_SRC_MAX) {
		pr_err("%s: Invalid bit clk src (%d)\n", __func__, bit_clk_src);
		return;
	}

#ifdef DPTC_DPHY_TEST
	//if(bit_clk_src == DPHY_BIT_CLK_SRC_MUX) {
	if(0) {
		tmp = dptc_dsi_read(0x68);
		tmp |= CFG_CLK_SEL;
		dptc_dsi_write(0x68,tmp);
	} else {
		tmp = dptc_dsi_read(0x68);
		tmp &= ~CFG_CLK_SEL;
		dptc_dsi_write(0x68,tmp);
	}

	//if(1 == half_pll5) {
	if(0) {
		tmp = dptc_dsi_read(0x68);
		tmp |= CFG_CLK_DIV2;
		dptc_dsi_write(0x68,tmp);
	} else {
		tmp = dptc_dsi_read(0x68);
		tmp &= ~CFG_CLK_DIV2;
		dptc_dsi_write(0x68,tmp);
	}
#else
#if 0
	if(bit_clk_src == DPHY_BIT_CLK_SRC_MUX)
		dsi_set_bits(base_addr, DSI_PHY_ANA_CTRL1, CFG_CLK_SEL);
	else
		dsi_clear_bits(base_addr, DSI_PHY_ANA_CTRL1, CFG_CLK_SEL);

	if(1 == half_pll5)
		dsi_set_bits(base_addr, DSI_PHY_ANA_CTRL1, CFG_CLK_DIV2);
	else
		dsi_clear_bits(base_addr, DSI_PHY_ANA_CTRL1, CFG_CLK_DIV2);
#else
	/*
	dsi_set_bits(base_addr, DSI_PHY_ANA_CTRL1, CFG_CLK_SEL);
	dsi_clear_bits(base_addr, DSI_PHY_ANA_CTRL1, CFG_CLK_DIV2);
	*/
#endif
#endif
}

static void dphy_set_timing(struct ky_dphy_ctx *dphy_ctx)
{
	uint32_t bitclk, lpx_clk, lpx_time, ta_get, ta_go;
	int ui, wakeup, reg;
	int hs_prep, hs_zero, hs_trail, hs_exit, ck_zero, ck_trail, ck_exit;
	int esc_clk, esc_clk_t;
	struct ky_dphy_timing *phy_timing;
	uint32_t value;

	if(NULL == dphy_ctx) {
		pr_err("%s: Invalid param!\n", __func__);
		return;
	}

	phy_timing = &(dphy_ctx->dphy_timing);

	DRM_DEBUG("%s() phy_freq %d esc_clk %d \n", __func__, dphy_ctx->phy_freq, dphy_ctx->esc_clk);

	esc_clk = dphy_ctx->esc_clk/1000;
	esc_clk_t = 1000/esc_clk;

	bitclk = dphy_ctx->phy_freq / 1000;
	ui = 1000/bitclk + 1;

	lpx_clk = (phy_timing->lpx_constant + phy_timing->lpx_ui * ui) / esc_clk_t + 1;
	lpx_time = lpx_clk * esc_clk_t;

	/* Below is for NT35451 */
	ta_get = lpx_time * 5 / esc_clk_t - 1;
	ta_go = lpx_time * 4 / esc_clk_t - 1;

	wakeup = phy_timing->wakeup_constant;
	wakeup = wakeup / esc_clk_t + 1;

	hs_prep = phy_timing->hs_prep_constant + phy_timing->hs_prep_ui * ui;
	hs_prep = hs_prep / esc_clk_t + 1;

	/* Our hardware added 3-byte clk automatically.
	 * 3-byte 3 * 8 * ui.
	 */
	hs_zero = phy_timing->hs_zero_constant + phy_timing->hs_zero_ui * ui -
		(hs_prep + 1) * esc_clk_t;
	hs_zero = (hs_zero - (3 * ui << 3)) / esc_clk_t + 4;
	if (hs_zero < 0)
		hs_zero = 0;

	hs_trail = phy_timing->hs_trail_constant + phy_timing->hs_trail_ui * ui;
	hs_trail = ((8 * ui) >= hs_trail) ? (8 * ui) : hs_trail;
	hs_trail = hs_trail / esc_clk_t + 1;
	if (hs_trail > 3)
		hs_trail -= 3;
	else
		hs_trail = 0;

	hs_exit = phy_timing->hs_exit_constant + phy_timing->hs_exit_ui * ui;
	hs_exit = hs_exit / esc_clk_t + 1;

	ck_zero = phy_timing->ck_zero_constant + phy_timing->ck_zero_ui * ui -
		(hs_prep + 1) * esc_clk_t;
	ck_zero = ck_zero / esc_clk_t + 1;

	ck_trail = phy_timing->ck_trail_constant + phy_timing->ck_trail_ui * ui;
	ck_trail = ck_trail / esc_clk_t + 1;

	ck_exit = hs_exit;

	reg = (hs_exit << CFG_DPHY_TIME_HS_EXIT_SHIFT)
		| (hs_trail << CFG_DPHY_TIME_HS_TRAIL_SHIFT)
		| (hs_zero << CFG_DPHY_TIME_HS_ZERO_SHIFT)
		| (hs_prep << CFG_DPHY_TIME_HS_PREP_SHIFT);

	DRM_DEBUG("%s dphy time0 hs_exit %d hs_trail %d hs_zero %d hs_prep %d reg 0x%x\n", __func__, hs_exit, hs_trail, hs_zero, hs_prep, reg);
#ifdef DPTC_DPHY_TEST
	dptc_dsi_write(0x40 , 0x01000000);
#else
	dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_0, reg);
	// dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_0, 0x06010603);
#endif

	reg = (ta_get << CFG_DPHY_TIME_TA_GET_SHIFT)
		| (ta_go << CFG_DPHY_TIME_TA_GO_SHIFT)
		| (wakeup << CFG_DPHY_TIME_WAKEUP_SHIFT);

	DRM_DEBUG("%s dphy time1 ta_get %d ta_go %d wakeup %d reg 0x%x\n", __func__, ta_get, ta_go, wakeup, reg);
#ifdef DPTC_DPHY_TEST
	dptc_dsi_write(0x44, 0x0403001F);
#else
	dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_1, reg);
	// dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_1, 0x130fcd98);
#endif
	reg = (ck_exit << CFG_DPHY_TIME_CLK_EXIT_SHIFT)
		| (ck_trail << CFG_DPHY_TIME_CLK_TRAIL_SHIFT)
		| (ck_zero << CFG_DPHY_TIME_CLK_ZERO_SHIFT)
		| (lpx_clk << CFG_DPHY_TIME_CLK_LPX_SHIFT);

	DRM_DEBUG("%s dphy time2 ck_exit %d ck_trail %d ck_zero %d lpx_clk %d reg 0x%x\n", __func__, ck_exit, ck_trail, ck_zero, lpx_clk, reg);
#ifdef DPTC_DPHY_TEST
	dptc_dsi_write(0x48, 0x02010500);
#else
	dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_2, reg);
	// dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_2, 0x06040c04);
#endif

	reg = (lpx_clk << CFG_DPHY_TIME_LPX_SHIFT)
		| phy_timing->req_ready << CFG_DPHY_TIME_REQRDY_SHIFT;

	DRM_DEBUG("%s dphy time3 lpx_clk %d req_ready %d reg 0x%x\n", __func__, lpx_clk, phy_timing->req_ready, reg);
#ifdef DPTC_DPHY_TEST
	dptc_dsi_write(0x4c, 0x001F);
#else
	dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_3, reg);
	// dsi_write(dphy_ctx->base_addr, DSI_PHY_TIME_3, 0x43c);
#endif
	/* calculated timing on brownstone:
	 * DSI_PHY_TIME_0 0x06080204
	 * DSI_PHY_TIME_1 0x6d2bfff0
	 * DSI_PHY_TIME_2 0x603130a
	 * DSI_PHY_TIME_3 0xa3c
	 */

	value = dsi_read(dphy_ctx->base_addr, DSI_PHY_TIME_0);
	DRM_DEBUG("%s() DSI_PHY_TIME_0 offset 0x%x value 0x%x\n", __func__, DSI_PHY_TIME_0, value);
	value = dsi_read(dphy_ctx->base_addr, DSI_PHY_TIME_1);
	DRM_DEBUG("%s() DSI_PHY_TIME_1 offset 0x%x value 0x%x\n", __func__, DSI_PHY_TIME_1, value);
	value = dsi_read(dphy_ctx->base_addr, DSI_PHY_TIME_2);
	DRM_DEBUG("%s() DSI_PHY_TIME_2 offset 0x%x value 0x%x\n", __func__, DSI_PHY_TIME_2, value);
	value = dsi_read(dphy_ctx->base_addr, DSI_PHY_TIME_3);
	DRM_DEBUG("%s() DSI_PHY_TIME_3 offset 0x%x value 0x%x\n", __func__, DSI_PHY_TIME_3, value);
}

static void dphy_get_setting(struct ky_dphy_ctx *dphy_ctx, struct device_node *np)
{
	struct ky_dphy_timing *dphy_timing = &dphy_ctx->dphy_timing;
	int ret;

	if(NULL == dphy_timing){
		pr_err("%s: Invalid param\n",__func__);
		return;
	}

	ret = of_property_read_u32(np, "hs_prep_constant", &dphy_timing->hs_prep_constant);
	if(0 != ret)
		dphy_timing->hs_prep_constant = HS_PREP_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "hs_prep_ui", &dphy_timing->hs_prep_ui);
	if(0 != ret)
		dphy_timing->hs_prep_ui = HS_PREP_UI_DEFAULT;

	ret = of_property_read_u32(np, "hs_zero_constant", &dphy_timing->hs_zero_constant);
	if(0 != ret)
		dphy_timing->hs_zero_constant = HS_ZERO_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "hs_zero_ui", &dphy_timing->hs_zero_ui);
	if(0 != ret)
		dphy_timing->hs_zero_ui = HS_ZERO_UI_DEFAULT;

	ret = of_property_read_u32(np, "hs_trail_constant", &dphy_timing->hs_trail_constant);
	if(0 != ret)
		dphy_timing->hs_trail_constant = HS_TRAIL_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "hs_trail_ui", &dphy_timing->hs_trail_ui);
	if(0 != ret)
		dphy_timing->hs_trail_ui = HS_TRAIL_UI_DEFAULT;

	ret = of_property_read_u32(np, "hs_exit_constant", &dphy_timing->hs_exit_constant);
	if(0 != ret)
		dphy_timing->hs_exit_constant = HS_EXIT_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "hs_exit_ui", &dphy_timing->hs_exit_ui);
	if(0 != ret)
		dphy_timing->hs_exit_ui = HS_EXIT_UI_DEFAULT;

	ret = of_property_read_u32(np, "ck_zero_constant", &dphy_timing->ck_zero_constant);
	if(0 != ret)
		dphy_timing->ck_zero_constant = CK_ZERO_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "ck_zero_ui", &dphy_timing->ck_zero_ui);
	if(0 != ret)
		dphy_timing->ck_zero_ui = CK_ZERO_UI_DEFAULT;

	ret = of_property_read_u32(np, "ck_trail_constant", &dphy_timing->ck_trail_constant);
	if(0 != ret)
		dphy_timing->ck_trail_constant = CK_TRAIL_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "ck_zero_ui", &dphy_timing->ck_zero_ui);
	if(0 != ret)
		dphy_timing->ck_zero_ui = CK_TRAIL_UI_DEFAULT;

	ret = of_property_read_u32(np, "req_ready", &dphy_timing->req_ready);
	if(0 != ret)
		dphy_timing->req_ready = REQ_READY_DEFAULT;

	ret = of_property_read_u32(np, "wakeup_constant", &dphy_timing->wakeup_constant);
	if(0 != ret)
		dphy_timing->wakeup_constant = WAKEUP_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "wakeup_ui", &dphy_timing->wakeup_ui);
	if(0 != ret)
		dphy_timing->wakeup_ui = WAKEUP_UI_DEFAULT;

	ret = of_property_read_u32(np, "lpx_constant", &dphy_timing->lpx_constant);
	if(0 != ret)
		dphy_timing->lpx_constant = LPX_CONSTANT_DEFAULT;

	ret = of_property_read_u32(np, "lpx_ui", &dphy_timing->lpx_ui);
	if(0 != ret)
		dphy_timing->lpx_ui = LPX_UI_DEFAULT;
}


void ky_dphy_core_get_status(struct ky_dphy_ctx *dphy_ctx)
{
	pr_debug("%s\n", __func__);

	if(NULL == dphy_ctx){
		pr_err("%s: Invalid param\n", __func__);
		return;
	}

	dphy_ctx->dphy_status0 = dsi_read(dphy_ctx->base_addr, DSI_PHY_STATUS_0);
	dphy_ctx->dphy_status1 = dsi_read(dphy_ctx->base_addr, DSI_PHY_STATUS_1);
	dphy_ctx->dphy_status2 = dsi_read(dphy_ctx->base_addr, DSI_PHY_STATUS_2);
	pr_debug("%s: dphy_status0 = 0x%x\n", __func__, dphy_ctx->dphy_status0);
	pr_debug("%s: dphy_status1 = 0x%x\n", __func__, dphy_ctx->dphy_status1);
	pr_debug("%s: dphy_status2 = 0x%x\n", __func__, dphy_ctx->dphy_status2);
}

void ky_dphy_core_reset(struct ky_dphy_ctx *dphy_ctx)
{
	pr_debug("%s\n", __func__);

	if(NULL == dphy_ctx){
		pr_err("%s: Invalid param\n", __func__);
		return;
	}

	dphy_ana_reset(dphy_ctx->base_addr);
}

/**
 * ky_dphy_core_init - int ky dphy
 *
 * @dphy_ctx: pointer to the ky_dphy_ctx
 *
 * This function will be called by the dsi driver in order to init the dphy
 * This function will do phy power on, enable continous clk, set dphy timing
 * and set lane number.
 *
 * This function has no return value.
 *
 */
void ky_dphy_core_init(struct ky_dphy_ctx *dphy_ctx)
{
	pr_debug("%s\n", __func__);

	if(NULL == dphy_ctx){
		pr_err("%s: Invalid param\n", __func__);
		return;
	}

	if(DPHY_STATUS_UNINIT != dphy_ctx->status){
		pr_warn("%s: dphy_ctx has been initialized (%d)\n",
			__func__, dphy_ctx->status);
		return;
	}

	/*use DPHY_BIT_CLK_SRC_MUX as default clk src*/
	dphy_set_bit_clk_src(dphy_ctx->base_addr, dphy_ctx->clk_src, dphy_ctx->half_pll5);

	/* digital and analog power on */
	dphy_set_power(dphy_ctx->base_addr, true);

	/* turn on DSI continuous clock for HS */
	dphy_set_cont_clk(dphy_ctx->base_addr, true);

	/* set dphy */
	dphy_set_timing(dphy_ctx);

	/* enable data lanes */
	dphy_set_lane_num(dphy_ctx->base_addr, dphy_ctx->lane_num);

	dphy_ctx->status = DPHY_STATUS_INIT;
}

/**
 * ky_dphy_core_uninit - unint ky dphy
 *
 * @dphy_ctx: pointer to the ky_dphy_ctx
 *
 * This function will be called by the dsi driver in order to unint the dphy
 * This function will disable continous clk, reset dphy, power down dphy
 *
 * This function has no return value.
 *
 */
void ky_dphy_core_uninit(struct ky_dphy_ctx *dphy_ctx)
{
	pr_debug("%s\n", __func__);

	if(NULL == dphy_ctx){
		pr_err("%s: Invalid param\n", __func__);
		return;
	}

	if(DPHY_STATUS_INIT != dphy_ctx->status){
		pr_warn("%s: dphy_ctx has not been initialized (%d)\n",
			__func__, dphy_ctx->status);
		return;
	}

	dphy_set_cont_clk(dphy_ctx->base_addr, false);
	dphy_ana_reset(dphy_ctx->base_addr);
	dphy_set_power(dphy_ctx->base_addr, false);

	dphy_ctx->status = DPHY_STATUS_UNINIT;
}

int ky_dphy_core_parse_dt(struct ky_dphy_ctx *dphy_ctx, struct device_node *np)
{
	if (!dphy_ctx) {
		pr_err("%s: Param is NULL\n",__func__);
		return -1;
	}

	dphy_get_setting(dphy_ctx, np);

	return 0;
}


static struct dphy_core_ops dphy_core_ops = {
	.parse_dt = ky_dphy_core_parse_dt,
	.init = ky_dphy_core_init,
	.uninit = ky_dphy_core_uninit,
	.reset = ky_dphy_core_reset,
	.get_status = ky_dphy_core_get_status,
};

static struct ops_entry entry = {
	.ver = "ky-dphy",
	.ops = &dphy_core_ops,
};

static int __init dphy_core_register(void)
{
	return dphy_core_ops_register(&entry);
}

subsys_initcall(dphy_core_register);

MODULE_LICENSE("GPL v2");
