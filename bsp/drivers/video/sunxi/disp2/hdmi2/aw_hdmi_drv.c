/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/dma-buf.h>
#include <linux/kthread.h>
#include <video/drv_hdmi.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <video/sunxi_metadata.h>
#include <linux/regulator/consumer.h>

#if IS_ENABLED(CONFIG_EXTCON)
#include <linux/extcon.h>
#include <linux/extcon-provider.h>
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
#include <media/cec.h>
#include <media/cec-notifier.h>
#endif

#include "aw_hdmi_define.h"
#include "aw_hdmi_drv.h"

struct aw_hdmi_driver *g_hdmi_drv;
struct disp_video_timings g_disp_info;

/**
 * 0x10: force unplug;
 * 0x11: force plug;
 * 0x1xx: unreport hpd state
 * 0x1xxx: mask hpd
 */
u32 gHdmi_hpd_force;
u32 gHdmi_hpd_force_pre;
u32 gHdmi_log_level;
u32 gHdmi_plugout_count;
u32 gHdmi_edid_repeat_count;

bool bHdmi_clk_enable;
bool bHdmi_pin_enable;
bool bHdmi_drv_enable;
bool bHdmi_suspend_enable;
bool bHdmi_boot_enable;
bool bHdmi_video_enable;
bool bHdmi_hpd_state;
bool bHdmi_cec_enable;
bool bHdmi_cec_suspend_enable;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
u8 gHdmi_hdcp_state;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
char *gHdcp_esm_fw_vir_addr;
u32 gHdcp_esm_fw_size;
#endif /* CONFIG_AW_HDMI2_HDCP22_SUNXI */
#endif /* CONFIG_AW_HDMI2_HDCP_SUNXI */

/**
 * @short List of the devices
 * Linked list that contains the installed devices
 */
static LIST_HEAD(devlist_global);

#if IS_ENABLED(CONFIG_EXTCON)
static const unsigned int aw_hdmi_cable[] = {
	EXTCON_DISP_HDMI,
	EXTCON_NONE,
};
static struct extcon_dev *gHdmi_extcon;
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
struct aw_hdmi_cec {
	struct device *dev;
	struct cec_adapter *adap;
	struct cec_notifier *notify;

	struct cec_msg rx_msg;
	unsigned int tx_status;
	bool tx_done;
	bool rx_done;
	u32 logical_addr;
};
#endif

#if IS_ENABLED(CONFIG_HDMI2_FREQ_SPREAD_SPECTRUM)

u32 freq_ss_amp = 10;
bool freq_ss_old;

static void _aw_freq_ss_set_tcon_tv_clock(u32 pixel_clk)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	switch (pixel_clk) {
	case 74250000:
		writel(0x84000009, ccmu_base + CCMU_TCON_TV0);
		break;
	case 148500000:
		writel(0x84000004, ccmu_base + CCMU_TCON_TV0);
		break;
	case 297000000:
		writel(0x85000104, ccmu_base + CCMU_TCON_TV0);
		break;
	default:
		hdmi_inf("[%s] freq ss unspport pixel clock %dHz for %s!\n",
				__func__, pixel_clk);
		break;
	}

	iounmap(ccmu_base);
}

static void _aw_freq_ss_set_pll_video_clock(u32 pixel_clk)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);
	writel(0xa0007a00 | (1 << 27) | (1 << 24),
		ccmu_base + CCMU_PLL_VIDEO_CONTROL);
	iounmap(ccmu_base);
}

static void _aw_freq_ss_set_pll_video_bias(u32 pixel_clk)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	switch (pixel_clk) {
	case 74250000:
		writel(0x00080000, ccmu_base + CCMU_PLL_VIDEO_BIAS);
		break;
	case 148500000:
		writel(0x00020000, ccmu_base + CCMU_PLL_VIDEO_BIAS);
		break;
	case 297000000:
		writel(0x0, ccmu_base + CCMU_PLL_VIDEO_BIAS);
		break;
	default:
		hdmi_inf("[%s] freq ss unspport pixel clock %dHz for %s!\n",
				__func__, pixel_clk);
		break;
	}

	iounmap(ccmu_base);

}

/* write SDM wavedata into PLL SDM sram */
static u32 _aw_freq_ss_set_sram_pat_cfg(u32 *cfg_table, u32 num)
{
	u32 i;
	u32 rdata;
	volatile void __iomem *sram_test_reg;
	volatile void __iomem *ccmu_base;

	sram_test_reg = ioremap(SRAM_TEST_REG0, 4);
	writel(readl(sram_test_reg) | (1 << 20), sram_test_reg);

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);
	/* reset sram */
	for (i = 0; i < 382; i++)
		writel(0, ccmu_base + i * 0x4);

	for (i = 0; i < num; i++)
		writel(cfg_table[i], ccmu_base + i * 4);

	/* check data */
	for (i = 0; i < 382; i++) {
		rdata = readl(ccmu_base + i * 0x4);
		if (i < num) {
			if (rdata != cfg_table[i]) {
				hdmi_wrn("[%s] w_data 0x%x unmatch r_data 0x%x!\n",
						__func__, cfg_table[i], rdata);
				break;
			}

		} else {
			if (rdata != 0) {
				hdmi_wrn("[%s] i = %d, num = %d, r_data = 0x%x!\n",
						__func__, i, num, rdata);
				break;
			}
		}
	}

	iounmap(ccmu_base);
	writel(readl(sram_test_reg) & 0xffefffff, sram_test_reg);
	iounmap(sram_test_reg);

	return 0;
}

/* write SDM wavedata into PLL SDM sram */
static u32 _aw_freq_ss_set_sram_pat_uncfg(void)
{
	u32 i;

	void __iomem *sram_test_reg;
	void __iomem *ccmu_base;

	sram_test_reg = ioremap(SRAM_TEST_REG0, 4);
	writel(readl(sram_test_reg) | (1 << 20), sram_test_reg);

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);
	/* reset sram */
	for (i = 0; i < 382; i++)
		writel(0, ccmu_base + i * 0x4);

	iounmap(ccmu_base);
	writel(readl(sram_test_reg) & 0xffefffff, sram_test_reg);
	iounmap(sram_test_reg);

	return 0;
}

static u32 _aw_freq_ss_set_sdm_mode(u8 hershey)
{
	u32 rvalue;
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	if (hershey) {
		rvalue = readl(ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		rvalue &= ~(0x1 << 31);
		writel(rvalue, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);

		udelay(1000);

		rvalue = readl(ccmu_base + CCMU_PLL_VIDEO_PATTERN1_CONTROL);
		rvalue &= ~(0x1 << 31);
		rvalue |= (0x1 << 31);
		writel(rvalue, ccmu_base + CCMU_PLL_VIDEO_PATTERN1_CONTROL);

		udelay(1000);
	} else {
		rvalue = readl(ccmu_base + CCMU_PLL_VIDEO_PATTERN1_CONTROL);
		rvalue &= ~(0x1 << 31);
		writel(rvalue, ccmu_base + CCMU_PLL_VIDEO_PATTERN1_CONTROL);

		udelay(1000);

		rvalue = readl(ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		rvalue &= ~(0x1 << 31);
		writel(rvalue, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);

		udelay(1000);
	}

	iounmap(ccmu_base);

	return 0;
}

static void _aw_freq_ss_set_pll_video_pattern(u32 amp)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	switch (amp) {
	case 2:/* sdm amp = 0.02 */
		writel(0xC0717AE1, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 4:/* sdm amp = 0.04 */
		writel(0xC0E175C3, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 6:/* sdm amp = 0.06 */
		writel(0xC15170A4, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 8:/* sdm amp = 0.08 */
		writel(0xC1C16B85, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 10:/* sdm amp = 0.10 */
		writel(0xC2216666, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 12:/* sdm amp = 0.12 */
		writel(0xC2916148, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 14:/* sdm amp = 0.14 */
		writel(0xC3015C29, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 16:/* sdm amp = 0.16 */
		writel(0xC371570A, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 18:/* sdm amp = 0.18 */
		writel(0xC3E151EC, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 20:/* sdm amp = 0.20 */
		writel(0xC4514CCD, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 22:/* sdm amp = 0.22 */
		writel(0xC4C147AE, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 24:/* sdm amp = 0.24 */
		writel(0xC531428F, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 26:/* sdm amp = 0.26 */
		writel(0xC5913D71, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 28:/* sdm amp = 0.28 */
		writel(0xC6013852, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	case 30:/* sdm amp = 0.30 */
		writel(0xC6713333, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;

	default:/* sdm amp = 0.10 */
		writel(0xC2216666, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);
		break;
	}

	iounmap(ccmu_base);
}

static void _aw_freq_ss_set_pll_video_pattern_unset_amp(void)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	writel(0x0, ccmu_base + CCMU_PLL_VIDEO_PATTERN0_CONTROL);

	iounmap(ccmu_base);
}

static u32 _aw_freq_ss_set_pll_sdm_Fs_24M_f_31500(u32 amp, bool old)
{
	u32 *table;
	u32 table_size = 0;

	switch (amp) {
	case 2:/* sdm amp = 0.02 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_02;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_02);
		break;

	case 4:/* sdm amp = 0.04 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_04;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_04);
		break;

	case 6:/* sdm amp = 0.06 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_06;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_06);
		break;

	case 8:/* sdm amp = 0.08 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_08;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_08);
		break;

	case 10:/* sdm amp = 0.10 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_10;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_10);
		break;

	case 12:/* sdm amp = 0.12 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_12;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_12);
		break;

	case 14:/* sdm amp = 0.14 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_14;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_14);
		break;

	case 16:/* sdm amp = 0.16 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_16;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_16);
		break;

	case 18:/* sdm amp = 0.18 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_18;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_18);
		break;

	case 20:/* sdm amp = 0.20 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_20;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_20);
		break;

	case 22:/* sdm amp = 0.22 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_22;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_22);
		break;

	case 24:/* sdm amp = 0.24 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_24;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_24);
		break;

	case 26:/* sdm amp = 0.26 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_26;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_26);
		break;

	case 28:/* sdm amp = 0.28 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_28;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_28);
		break;

	case 30:/* sdm amp = 0.30 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_30;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_30);
		break;

	default:/* sdm amp = 0.10 */
		table = kiss_data_Fs_24M_f_31500_AMP_0_10;
		table_size = sizeof(kiss_data_Fs_24M_f_31500_AMP_0_10);
		break;
	}

	if (old) {
		_aw_freq_ss_set_sdm_mode(0);/* freq old spectrum */
	} else {
		_aw_freq_ss_set_sdm_mode(1);/* enable new hershey kiss sdm */
		_aw_freq_ss_set_sram_pat_cfg(table, table_size / 4);/* write sdm wavedata into sdm sram */
	}

	_aw_freq_ss_set_pll_video_pattern(amp);

	return table_size;
}

u32 _aw_freq_ss_set_pll_video2_sdm(u32 Fs, u32 f, u32 amp, bool old)
{
	/* SDM Clk source is 24MHz */
	/* SDM wave freq is 31.5KHz */
	if (Fs == 24000000) {
		if (f == 31500)
			_aw_freq_ss_set_pll_sdm_Fs_24M_f_31500(amp, old);
		else if (f == 32000)  /* SDM wave freq is 32KHz */
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else if (f == 32500)  /* SDM wave freq is 32.5KHz */
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else if (f == 33000)  /* SDM wave freq is 33KHz */
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
	} else {/* SDM Clk source is 12MHz */
		if (f == 31500)
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else if (f == 32000)  /* SDM wave freq is 32KHz */
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else if (f == 32500)  /* SDM wave freq is 32.5KHz */
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else if (f == 33000)  /* SDM wave freq is 33KHz */
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
		else
			hdmi_err("%s: Fs %d f %d not support now!\n", __func__, Fs, f);
	}

	return 0;
}

void _aw_freq_ss_unset(void)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	writel(0x000f0000, ccmu_base + CCMU_PLL_VIDEO_BIAS);
	_aw_freq_ss_set_pll_video_pattern_unset_amp();
	_aw_freq_ss_set_sram_pat_uncfg();

	iounmap(ccmu_base);
}

void _aw_freq_spread_sprectrum_disabled(u32 pixel_clk)
{
	void __iomem *ccmu_base;

	ccmu_base = ioremap(CCMU_BASE, CCMU_SIZE);

	switch (pixel_clk) {
	case 13500000:
		break;
	case 27000000:
		writel(0x88006213, ccmu_base + CCMU_PLL_VIDEO_CONTROL);
		writel(0x8400000a, ccmu_base + CCMU_TCON_TV0);
		writel(0x8200000a, ccmu_base + CCMU_HDMI);
		break;
	case 74250000:
		writel(0xa8006200, ccmu_base + CCMU_PLL_VIDEO_CONTROL);
		writel(0x85000303, ccmu_base + CCMU_TCON_TV0);
		writel(0x82000003, ccmu_base + CCMU_HDMI);
		break;
	case 148500000:
		writel(0xa8006200, ccmu_base + CCMU_PLL_VIDEO_CONTROL);
		writel(0x85000301, ccmu_base + CCMU_TCON_TV0);
		writel(0x82000001, ccmu_base + CCMU_HDMI);
		break;
	case 297000000:
		writel(0xa8006200, ccmu_base + CCMU_PLL_VIDEO_CONTROL);
		writel(0x85000007, ccmu_base + CCMU_TCON_TV0);
		writel(0x82000000, ccmu_base + CCMU_HDMI);
		break;
	case 594000000:
		writel(0xa8006200, ccmu_base + CCMU_PLL_VIDEO_CONTROL);
		writel(0x85000003, ccmu_base + CCMU_TCON_TV0);
		writel(0x82000000, ccmu_base + CCMU_HDMI);
		break;
	default:
		hdmi_err("%s: pixel_clk(%d) not supported yet!!!\n",
				__func__, pixel_clk);
		break;
	}

	iounmap(ccmu_base);
}

u32 _aw_freq_spread_sprectrum_enabled(u32 pixel_clk)
{
	u32 sdm_src_clk_rate = 24000000;
	u32 sdm_wave_rate = 31500;

	if (!freq_ss_amp) {
		hdmi_wrn("freq_ss_amp is zero, NOT set spread spectrun!\n");
		_aw_freq_ss_unset();
		_aw_freq_spread_sprectrum_disabled(pixel_clk);
		return 0;
	}

	if (pixel_clk != 74250000
		&& pixel_clk != 148500000
		&& pixel_clk != 297000000) {
		hdmi_wrn("NOT set spread spectrun, pixel clk:%d\n",
				pixel_clk);
		_aw_freq_ss_unset();
		_aw_freq_spread_sprectrum_disabled(pixel_clk);
		return 0;
	}

	hdmi_inf("set spread spectrun, pixel clk:%d\n", pixel_clk);

	_aw_freq_ss_set_pll_video_clock(pixel_clk);
	_aw_freq_ss_set_tcon_tv_clock(pixel_clk);
	_aw_freq_ss_set_pll_video_bias(pixel_clk);

	_aw_freq_ss_set_pll_video2_sdm(sdm_src_clk_rate, sdm_wave_rate,
		freq_ss_amp, freq_ss_old);

	return 0;
}
#endif

unsigned char _aw_hdmi_get_tcon_pad(void)
{
#ifdef TCON_PAN_SEL
	return (unsigned char)disp_hdmi_pad_get();
#else
	return 0;
#endif
}

static void _aw_hdmi_set_tcon_pad(unsigned char set)
{
#ifdef TCON_PAN_SEL
	mutex_lock(&g_hdmi_drv->aw_mutex.lock_tcon);
	if (set)
		disp_hdmi_pad_sel(1);
	else
		disp_hdmi_pad_release();
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_tcon);
#endif
}

static int _aw_hdmi_drv_dfs_set_state_sync(bool en)
{
#if IS_ENABLED(CONFIG_AW_DMC_DEVFREQ) && (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2))
	int ret = 0;
	char *envp[3] = {
		"SYSTEM=HDMI_DISP",
		NULL,
		NULL};

	if (en)
		envp[1] = "EVENT=ON";
	else
		envp[1] = "EVENT=OFF";

	if (g_hdmi_drv->parent_dev)
		ret = kobject_uevent_env(&g_hdmi_drv->parent_dev->kobj,
			KOBJ_CHANGE, envp);
	else
		hdmi_wrn("hdmi parent_dev is Null!\n");

	return ret;
#else
	return 0;
#endif
}

u32 _aw_hdmi_drv_get_hpd(void)
{
#ifdef __FPGA_PLAT__
	hdmi_err("%s: fpga mode and force hpd in\n", __func__);
	return 1;
#else
	return bHdmi_hpd_state;
#endif
}

static int _aw_hdmi_drv_power_enable(struct regulator *regu)
{
	int ret = -1;

	if (!regu) {
		hdmi_err("%s: regulator is NULL!\n", __func__);
		return -1;
	}

	/* enalbe regulator */
	ret = regulator_enable(regu);
	if (ret != 0) {
		hdmi_err("%s: some error happen, fail to enable regulator!\n", __func__);
	} else {
		VIDEO_INF("suceess to enable regulator!\n");
	}

	return ret;
}

static int _aw_hdmi_drv_power_disable(struct regulator *regu)
{
	int ret = 0;

	/* disalbe regulator */
	ret = regulator_disable(regu);
	if (ret != 0) {
		hdmi_err("%s: some error happen, fail to disable regulator!\n", __func__);
	} else {
		VIDEO_INF("suceess to disable regulator!\n");
	}

	return ret;
}

static void _aw_hdmi_drv_clock_enable(void)
{
	if (bHdmi_clk_enable) {
		hdmi_wrn("hdmi clk has been enable!\n");
		return;
	}

#if (!IS_ENABLED(CONFIG_ARCH_SUN55IW3))
	if (g_hdmi_drv->aw_clock.rst_bus_sub) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_sub)) {
			hdmi_err("%s: clock enable to de-assert sub bus is failed!\n", __func__);
			return;
		}
	}
	if (g_hdmi_drv->aw_clock.rst_bus_main) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_main)) {
			hdmi_err("%s: clock enable to de-assert main bus is failed!\n", __func__);
			return;
		}
	}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_clock.rst_bus_hdcp) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_hdcp)) {
			hdmi_err("%s: clock enable to de-assert hdcp bus is failed!\n", __func__);
			return;
		}
	}
#endif /* CONFIG_AW_HDMI2_HDCP22_SUNXI */
#endif /* CONFIG_ARCH_SUN55IW3 */

	if (g_hdmi_drv->aw_clock.clk_hdmi_bus != NULL) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_bus) != 0) {
			hdmi_err("%s: hdmi bus clk enable failed!\n", __func__);
			return;
		}
	}

	if (g_hdmi_drv->aw_clock.clk_hdmi != NULL) {
		if (g_hdmi_drv->aw_clock.clk_tcon_tv == NULL)
			hdmi_err("%s: tcon_tv clk get failed\n", __func__);
		else
			clk_set_rate(g_hdmi_drv->aw_clock.clk_hdmi, clk_get_rate(g_hdmi_drv->aw_clock.clk_tcon_tv));

		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi) != 0)
			hdmi_err("%s: hdmi clk enable failed!\n", __func__);
	}

	if (g_hdmi_drv->aw_clock.clk_hdmi_24M != NULL) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_24M) != 0)
			hdmi_err("%s: hdmi 24M clk enable failed!\n", __func__);
	}

    if (g_hdmi_drv->aw_clock.clk_hdmi_sfr != NULL) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_sfr) != 0)
			hdmi_err("%s: hdmi sfr enable failed!\n", __func__);
	}

#if (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2))
#else
	if (g_hdmi_drv->aw_clock.clk_hdmi_ddc != NULL) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_ddc) != 0)
			hdmi_err("%s: hdmi ddc clk enable failed!\n", __func__);
	} else {
		hdmi_wrn("hdmi ddc clk is NULL!\n");
	}
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_clock.clk_hdcp_bus) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdcp_bus) != 0)
			hdmi_err("%s: hdmi hdcp bus clk enable failed!\n", __func__);
	}

	if (g_hdmi_drv->aw_clock.clk_hdcp)
		clk_set_rate(g_hdmi_drv->aw_clock.clk_hdcp, 300000000);

	if (g_hdmi_drv->aw_clock.clk_hdcp != NULL)
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdcp) != 0)
			hdmi_err("%s: hdmi ddc clk enable failed!\n", __func__);
#endif

	bHdmi_clk_enable = true;
}

static void _aw_hdmi_drv_clock_enable_for_resume(void)
{
	if (bHdmi_clk_enable) {
		hdmi_wrn("hdmi clk has been enable when resume.\n");
		return;
	}

#if (!IS_ENABLED(CONFIG_ARCH_SUN55IW3))
	if (g_hdmi_drv->aw_clock.rst_bus_sub) {
		hdmi_inf("config assert sub bus.\n");
		if (reset_control_assert(g_hdmi_drv->aw_clock.rst_bus_sub)) {
			hdmi_err("%s: assert bus sub failed!\n", __func__);
			return;
		}
	}

	if (g_hdmi_drv->aw_clock.rst_bus_main) {
		hdmi_inf("config assert main bus.\n");
		if (reset_control_assert(g_hdmi_drv->aw_clock.rst_bus_main)) {
			hdmi_err("%s: assert bus main failed!\n", __func__);
			return;
		}
	}
#endif

	if (g_hdmi_drv->aw_clock.rst_bus_sub) {
		hdmi_inf("config de-assert sub bus.\n");
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_sub)) {
			hdmi_err("%s: deassert bus sub failed!\n", __func__);
			return;
		}
	}

	if (g_hdmi_drv->aw_clock.rst_bus_main) {
		hdmi_inf("config de-assert main bus.\n");
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_main)) {
			hdmi_err("%s: deassert bus main failed!\n", __func__);
			return;
		}
	}

	if (g_hdmi_drv->aw_clock.clk_hdmi_bus) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_bus) != 0)
			hdmi_err("%s: hdmi bus clk enable failed!\n", __func__);
	}

	if (g_hdmi_drv->aw_clock.clk_hdmi != NULL) {
		if (g_hdmi_drv->aw_clock.clk_tcon_tv == NULL)
			hdmi_err("%s: tcon_tv clk get failed\n", __func__);
		else
			clk_set_rate(g_hdmi_drv->aw_clock.clk_hdmi,
					clk_get_rate(g_hdmi_drv->aw_clock.clk_tcon_tv));

		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi) != 0)
			hdmi_err("%s: hdmi clk enable failed!\n", __func__);
	}

#if (IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2))
#else
	if ((g_hdmi_drv->aw_clock.clk_hdmi_ddc != NULL) && (!g_hdmi_drv->aw_dts.cec_super_standby))
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_ddc) != 0)
			hdmi_err("%s: hdmi ddc clk enable failed!\n", __func__);
#endif

	if (g_hdmi_drv->aw_clock.clk_hdmi_24M != NULL) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_24M) != 0)
			hdmi_err("%s: hdmi 24M clk enable failed!\n", __func__);
	}

        if (g_hdmi_drv->aw_clock.clk_hdmi_sfr != NULL) {
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdmi_sfr) != 0)
			hdmi_err("%s: hdmi sfr clk enable failed!\n", __func__);
	}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_clock.clk_hdcp)
		clk_set_rate(g_hdmi_drv->aw_clock.clk_hdcp, 300000000);

	if (g_hdmi_drv->aw_clock.clk_hdcp != NULL)
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_hdcp) != 0)
			hdmi_err("%s: hdmi ddc clk enable failed!\n", __func__);
#endif

	bHdmi_clk_enable = true;
}

static void _aw_hdmi_drv_clock_disable(void)
{
	if (!bHdmi_clk_enable) {
		hdmi_wrn("hdmi clk has been disable.\n");
		return;
	}

	bHdmi_clk_enable = false;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_clock.clk_hdcp != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdcp);
	if (g_hdmi_drv->aw_clock.clk_hdcp_bus != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdcp_bus);
#endif

	if (g_hdmi_drv->aw_clock.clk_hdmi_ddc != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_ddc);

	if (g_hdmi_drv->aw_clock.clk_hdmi != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi);

	if (g_hdmi_drv->aw_clock.clk_hdmi_bus != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_bus);

    if (g_hdmi_drv->aw_clock.clk_hdmi_sfr != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_sfr);

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_clock.rst_bus_hdcp) {
		if (reset_control_assert(g_hdmi_drv->aw_clock.rst_bus_hdcp)) {
			hdmi_err("%s: assert bus hdcp failed!\n", __func__);
			return;
		}
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)

#else
	if (g_hdmi_drv->aw_clock.rst_bus_sub) {
		hdmi_inf("config assert sub bus.\n");
		if (reset_control_assert(g_hdmi_drv->aw_clock.rst_bus_sub)) {
			hdmi_err("%s: assert bus sub failed!\n", __func__);
			return;
		}
	}

	if (g_hdmi_drv->aw_clock.rst_bus_main) {
		hdmi_inf("config assert main bus.\n");
		if (reset_control_assert(g_hdmi_drv->aw_clock.rst_bus_main)) {
			hdmi_err("%s: assert bus main failed!\n", __func__);
			return;
		}
	}
#endif
}

static void _aw_hdmi_drv_clock_disable_for_suspend(void)
{
	if (!bHdmi_clk_enable) {
		hdmi_wrn("hdmi clk has been disable when suspend.\n");
		return;
	}

	bHdmi_clk_enable = false;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_clock.clk_hdcp != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdcp);
#endif

	if (g_hdmi_drv->aw_clock.clk_hdmi_24M != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_24M);
	if ((g_hdmi_drv->aw_clock.clk_hdmi_ddc != NULL)
			&& (!g_hdmi_drv->aw_dts.cec_super_standby))
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_ddc);
	if (g_hdmi_drv->aw_clock.clk_hdmi != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi);

	if (g_hdmi_drv->aw_clock.clk_hdmi_bus != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_bus);

	if (g_hdmi_drv->aw_clock.clk_hdmi_sfr != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_hdmi_sfr);

}

static void _aw_hdmi_drv_pin_config(void)
{
	s32 ret = 0;
	struct pinctrl_state *state;

	if (bHdmi_pin_enable) {
		hdmi_wrn("hdmi pin has been enable.\n");
		return;
	}

	/* pin configuration for ddc */
	if (!IS_ERR(g_hdmi_drv->hdmi_pctl)) {
		state = pinctrl_lookup_state(g_hdmi_drv->hdmi_pctl, AW_DDC_PIN_ACTIVE);
		if (IS_ERR(state)) {
			hdmi_err("%s: pinctrl_lookup_state for HDMI2.0 SCL failed!\n",
					__func__);
			return;
		}

		ret = pinctrl_select_state(g_hdmi_drv->hdmi_pctl, state);
		if (ret < 0) {
			hdmi_err("%s: pinctrl_select_state for HDMI2.0 DDC failed!\n",
					__func__);
			return;
		}
	}

	bHdmi_pin_enable = true;
}

static void _aw_hdmi_drv_pin_config_for_resume(void)
{
	s32 ret = 0;
	struct pinctrl_state *state;

	if (bHdmi_pin_enable) {
		hdmi_inf("hdmi pin has been enable when resume.\n");
		return;
	}

	/* pin configuration for ddc */
	if (!IS_ERR(g_hdmi_drv->hdmi_pctl)) {
		state = pinctrl_lookup_state(g_hdmi_drv->hdmi_pctl, AW_DDC_PIN_ACTIVE);
		if (IS_ERR(state)) {
			hdmi_err("%s: pinctrl_lookup_state for HDMI2.0 SCL failed!\n",
					__func__);
			return;
		}

		ret = pinctrl_select_state(g_hdmi_drv->hdmi_pctl, state);
		if (ret < 0) {
			hdmi_err("%s: pinctrl_select_state for HDMI2.0 DDC failed!\n",
					__func__);
			return;
		}
	}

	bHdmi_pin_enable = true;
}

static void _aw_hdmi_drv_pin_release(void)
{
	s32 ret = 0;
	struct pinctrl_state *state;

	if (!bHdmi_pin_enable) {
		hdmi_wrn("hdmi pin has been disable.\n");
		return;
	}

	/* pin configuration for ddc */
	if (!IS_ERR(g_hdmi_drv->hdmi_pctl)) {
		state = pinctrl_lookup_state(g_hdmi_drv->hdmi_pctl, AW_DDC_PIN_SLEEP);
		if (IS_ERR(state)) {
			hdmi_err("%s: pinctrl_lookup_state for HDMI2.0 SCL failed!\n",
					__func__);
			return;
		}

		ret = pinctrl_select_state(g_hdmi_drv->hdmi_pctl, state);
		if (ret < 0) {
			hdmi_err("%s: pinctrl_select_state for HDMI2.0 DDC failed!\n",
					__func__);
			return;
		}
	}

	bHdmi_pin_enable = false;
}

static void _aw_hdmi_drv_pin_release_for_suspend(void)
{
	s32 ret = 0;
	struct pinctrl_state *state;

	LOG_TRACE();
	if (!bHdmi_pin_enable) {
		hdmi_wrn("hdmi pin has been disable when suspend.\n");
		return;
	}

	/* pin configuration for ddc */
	if (!IS_ERR(g_hdmi_drv->hdmi_pctl)) {
		state = pinctrl_lookup_state(g_hdmi_drv->hdmi_pctl, AW_DDC_PIN_SLEEP);
		if (IS_ERR(state)) {
			hdmi_err("%s: pinctrl_lookup_state for HDMI2.0 SCL failed!\n",
					__func__);
			return;
		}

		ret = pinctrl_select_state(g_hdmi_drv->hdmi_pctl, state);
		if (ret < 0) {
			hdmi_err("%s: pinctrl_select_state for HDMI2.0 DDC failed!\n",
					__func__);
			return;
		}
	}

	bHdmi_pin_enable = false;
}

static void _aw_hdmi_drv_resource_config(void)
{
	LOG_TRACE();

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	if (g_hdmi_drv->aw_clock.rst_bus_sub) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_sub)) {
			hdmi_err("%s: clock enable to de-assert sub bus is failed!\n", __func__);
			return;
		}
	}
	if (g_hdmi_drv->aw_clock.rst_bus_main) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_main)) {
			hdmi_err("%s: clock enable to de-assert main bus is failed!\n", __func__);
			return;
		}
	}
#endif

	_aw_hdmi_drv_clock_enable();
	_aw_hdmi_drv_pin_config();
}

static void _aw_hdmi_drv_resource_config_for_resume(void)
{
	LOG_TRACE();
	_aw_hdmi_drv_clock_enable_for_resume();
	_aw_hdmi_drv_pin_config_for_resume();
}

static void _aw_hdmi_drv_resource_release(void)
{
	LOG_TRACE();
	_aw_hdmi_drv_clock_disable();
	_aw_hdmi_drv_pin_release();
}

static void _aw_hdmi_drv_resource_release_for_suspend(void)
{
	LOG_TRACE();
	_aw_hdmi_drv_clock_disable_for_suspend();
	_aw_hdmi_drv_pin_release_for_suspend();
}

static void _aw_hdmi_drv_clock_reset(void)
{
	_aw_hdmi_drv_clock_disable();
	udelay(10);
	_aw_hdmi_drv_clock_enable();

	_aw_hdmi_drv_clock_disable();
	udelay(10);
	_aw_hdmi_drv_clock_enable();
}

void _aw_hdmi_drv_hpd_notify(u32 status)
{
#if IS_ENABLED(CONFIG_EXTCON)
	/* hpd mask not-notify */
	if (aw_hdmi_drv_get_hpd_mask() & 0x100)
		return;

	extcon_set_state_sync(gHdmi_extcon, EXTCON_DISP_HDMI,
		(status == 1) ? STATUE_OPEN : STATUE_CLOSE);
#else
	hdmi_wrn("not config extcon node for hpd notify.\n");
#endif	
}

bool _aw_hdmi_drv_get_boot_info(void)
{
#ifdef __FPGA_PLAT__
	return false;
#else
	unsigned int value;
	unsigned int output_type0, output_mode0;
	unsigned int output_type1, output_mode1;

	const char *name = "boot_disp";
	/* Read video booting params from disp device tree */
	value = disp_boot_para_parse(name);

	/* To check if hdmi has been configured in uboot */
	output_type0 = (value >> 8) & 0xff;
	output_mode0 = (value) & 0xff;
	output_type1 = (value >> 24) & 0xff;
	output_mode1 = (value >> 16) & 0xff;
	if ((output_type0 == DISP_OUTPUT_TYPE_HDMI) ||
		(output_type1 == DISP_OUTPUT_TYPE_HDMI))
		return true;
	else
		return false;
#endif
}

void _aw_hdmi_init_boot_mode(void)
{
	bHdmi_boot_enable = _aw_hdmi_drv_get_boot_info();
	hdmi_inf("boot hdmi is %s\n", bHdmi_boot_enable ? "on" : "off");

	/* Check if hdmi has been configured during booting */
	if (bHdmi_boot_enable) {
		bHdmi_video_enable = true;
		bHdmi_pin_enable = true;
		bHdmi_drv_enable = true;
#if (!defined(CONFIG_COMMON_CLK_ENABLE_SYNCBOOT))
		bHdmi_clk_enable = false;
		_aw_hdmi_drv_clock_enable();
		bHdmi_clk_enable = true;
#endif
	} else {
		bHdmi_video_enable = false;
		bHdmi_clk_enable   = false;
		bHdmi_pin_enable   = false;
		_aw_hdmi_drv_resource_config();
	}
}

int _aw_hdmi_init_notify_node(void)
{
#if IS_ENABLED(CONFIG_EXTCON)
	gHdmi_extcon = devm_extcon_dev_allocate(g_hdmi_drv->parent_dev, aw_hdmi_cable);
/* 	gHdmi_extcon->name = "hdmi"; */ /* fix me */
	devm_extcon_dev_register(g_hdmi_drv->parent_dev, gHdmi_extcon);

	_aw_hdmi_drv_hpd_notify(false);
	if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
		_aw_hdmi_drv_dfs_set_state_sync(0);
	return 0;
#else
	hdmi_wrn("not init hdmi notify node!\n");
	return -1;
#endif
}

static void _aw_hdmi_drv_edid_recheck(void)
{
	if (!g_hdmi_drv->hdmi_core->mode.edid_done) {
		g_hdmi_drv->hdmi_core->edid_ops.main_release();

		mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
		g_hdmi_drv->hdmi_core->edid_ops.main_read();
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);

		_aw_hdmi_drv_hpd_notify(0x1);
		if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
			_aw_hdmi_drv_dfs_set_state_sync(1);

		g_hdmi_drv->hdmi_core->edid_ops.correct_hw_config();

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
		/* after rechecking edid, physical address needs to be updated. */
		if (g_hdmi_drv->aw_dts.cec_support) {
			mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
			cec_notifier_set_phys_addr(g_hdmi_drv->cec_notifier,
					g_hdmi_drv->hdmi_core->cec_ops.get_pa());
			mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
		}
#endif
	}
}

static void _aw_hdmi_drv_video_config(struct aw_hdmi_driver *drv)
{
	g_hdmi_drv->hdmi_core->dev_ops.dev_config();
}

/* sys_config.fex setting for hpd */
static void _aw_hdmi_drv_ddc_gpio_config(void)
{
	/* To enhance the ddc ability */
	if (g_hdmi_drv->ddc_ctrl_en == 1) {
		if (gpio_direction_output(g_hdmi_drv->ddc_gpio.gpio, 1) != 0) {
			hdmi_err("%s: ddc ctrl gpio set 1 error!\n", __func__);
			return;
		}
	}
}

/* release sys_config.fex setting for hpd */
static void _aw_hdmi_drv_ddc_gpio_release(void)
{
	if (g_hdmi_drv->ddc_ctrl_en == 1) {
		if (gpio_direction_output(g_hdmi_drv->ddc_gpio.gpio, 0) != 0) {
			hdmi_err("%s: ddc ctrl gpio set 0 error!\n", __func__);
			return;
		}
	}
}

static void _aw_hdmi_drv_plugin_proc(void)
{
	LOG_TRACE();
	hdmi_inf("HDMI cable is connected\n");
	if (!(aw_hdmi_drv_get_hpd_mask() & 0x10))
		bHdmi_hpd_state = 1;

	_aw_hdmi_set_tcon_pad(true);

	_aw_hdmi_drv_ddc_gpio_config();

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
	g_hdmi_drv->hdmi_core->edid_ops.main_read();
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);

	if (g_hdmi_drv->hdmi_core->mode.edid_done) {
		g_hdmi_drv->hdmi_core->edid_ops.set_prefered_video();

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
		if (g_hdmi_drv->aw_dts.cec_support) {
			mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
			cec_notifier_set_phys_addr(g_hdmi_drv->cec_notifier,
					g_hdmi_drv->hdmi_core->cec_ops.get_pa());
			mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
		}
#endif
	}

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_ctrl);
	if ((!bHdmi_video_enable) && (bHdmi_drv_enable)) {
		_aw_hdmi_drv_video_config(g_hdmi_drv);
		bHdmi_video_enable = true;
	}
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);

	_aw_hdmi_drv_hpd_notify(0x1);
	if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
		_aw_hdmi_drv_dfs_set_state_sync(1);

	g_hdmi_drv->hdmi_core->edid_ops.correct_hw_config();
}

static void _aw_hdmi_drv_plugout_proc(void)
{
	LOG_TRACE();
	hdmi_inf("HDMI cable is disconnected\n");
	if (!(aw_hdmi_drv_get_hpd_mask() & 0x10))
		bHdmi_hpd_state = 0;
	bHdmi_video_enable = false;

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	if (g_hdmi_drv->aw_dts.cec_support) {
		mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
		cec_notifier_phys_addr_invalidate(g_hdmi_drv->cec_notifier);
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
	}
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	if (g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_on) {
		g_hdmi_drv->hdmi_core->hdcp_ops.hdcp_disconfigure();
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
	}
#endif

	if (g_hdmi_drv->hdmi_core->mode.pHdcp.use_hdcp)
		g_hdmi_drv->hdmi_core->hdcp_ops.hdcp_close();

	g_hdmi_drv->hdmi_core->dev_ops.dev_close();

	_aw_hdmi_drv_clock_reset();
	g_hdmi_drv->hdmi_core->phy_ops.set_hpd(true);

	g_hdmi_drv->hdmi_core->edid_ops.main_release();
	_aw_hdmi_set_tcon_pad(false);

	_aw_hdmi_drv_hpd_notify(0x0);
	if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
		_aw_hdmi_drv_dfs_set_state_sync(0);
}

/*
 * hpd_mask:
 * 1. 0x1x: force to plug in or out
 * 0x10-force to hotplug_out  0x11-force to hotplug_in
 *
 * 2. 0x1xx: do NOT report plug event to user-space
 * 0x110: force to hotplug_out and do NOT report plug event to user-space
 * 0x111: force to hotplug_in and do NOT report plug event to user-space
 *
 * 3. 0x1xxx: disable plugin or out
 *  0x1000: disable plugin or out
 */
static void _aw_hdmi_drv_hpd_mask_process(void)
{
	u32 mask = aw_hdmi_drv_get_hpd_mask();
	if (mask == 0x10 || mask == 0x110 || mask == 0x1010) {
		_aw_hdmi_drv_plugout_proc();
	} else if (mask == 0x11 || mask == 0x111 || mask == 0x1011) {
		msleep(400);
		_aw_hdmi_drv_plugin_proc();
	} else {
		hdmi_err("%s: Unknow hpd event\n", __func__);
	}
}

static int _aw_hdmi_drv_run_thread(void *parg)
{
	u32 hpd_state_now = 0;
	u32 i;
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	u32 hdcp_status = 0;
#endif

	while (1) {
		if (kthread_should_stop())
			break;

		if (aw_hdmi_drv_get_hpd_mask() & 0x1000) {
			msleep(1);
			continue;
		}

		if (aw_hdmi_drv_get_hpd_mask() & 0x10) {
			if (aw_hdmi_drv_get_hpd_mask() != gHdmi_hpd_force_pre) {
				gHdmi_hpd_force_pre = aw_hdmi_drv_get_hpd_mask();
				_aw_hdmi_drv_hpd_mask_process();
			} else if ((aw_hdmi_drv_get_hpd_mask() & 0x11) == 0x11) {
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
				mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
				hdcp_status = g_hdmi_drv->hdmi_core->hdcp_ops.hdcp_get_status();
				if (hdcp_status == 0)
					aw_hdmi_drv_set_hdcp_state(AW_HDCP_SUCCESS);
				else if (hdcp_status == -1)
					aw_hdmi_drv_set_hdcp_state(AW_HDCP_FAILED);
				else if (hdcp_status == 1)
					aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
				else
					;/* hdmi_inf("Unkown hdcp status\n"); */
				mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);
#endif
			}
			msleep(1);
			continue;
		}

		if (!bHdmi_clk_enable) {
			msleep(1);
			continue;
		}

		hpd_state_now = g_hdmi_drv->hdmi_core->phy_ops.get_hpd();
		if (hpd_state_now != bHdmi_hpd_state) {
			/* HPD Event Happen */
			if (!hpd_state_now) {
				gHdmi_edid_repeat_count = 20;
				_aw_hdmi_drv_plugout_proc();
				gHdmi_plugout_count = 0;
			} else {
				for (i = 0; i < 2; i++) {
					mdelay(100);
					hpd_state_now = g_hdmi_drv->hdmi_core->phy_ops.get_hpd();
					if (hpd_state_now == bHdmi_hpd_state)
						break;/* it's not a real hpd event */
				}

				if (i >= 2) {
					gHdmi_plugout_count = 41;
					/* it's a real hpd event */
					_aw_hdmi_drv_plugin_proc();
					gHdmi_edid_repeat_count = 0;
				}
			}
		} else {
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
			if (hpd_state_now && (bHdmi_hpd_state || (aw_hdmi_drv_get_hpd_mask() == 0x1)) &&
					g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_on &&
					g_hdmi_drv->hdmi_core->mode.pHdcp.use_hdcp) {
				mutex_lock(&g_hdmi_drv->aw_mutex.lock_hdcp);
				hdcp_status = g_hdmi_drv->hdmi_core->hdcp_ops.hdcp_get_status();
				if (hdcp_status == 0)
					aw_hdmi_drv_set_hdcp_state(AW_HDCP_SUCCESS);
				else if (hdcp_status == -1)
					aw_hdmi_drv_set_hdcp_state(AW_HDCP_FAILED);
				else if (hdcp_status == 1)
					aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
				else
					;/* hdmi_inf("Unkown hdcp status\n"); */
				mutex_unlock(&g_hdmi_drv->aw_mutex.lock_hdcp);
			}
#endif
		}

		gHdmi_plugout_count++;
		if ((!bHdmi_hpd_state) && (gHdmi_plugout_count == 40))
			_aw_hdmi_drv_ddc_gpio_release();

		gHdmi_edid_repeat_count++;
		if (bHdmi_hpd_state && (gHdmi_edid_repeat_count == 19))
			_aw_hdmi_drv_edid_recheck();

		msleep(1);
	}

	return 0;
}

int _aw_hpd_thread_init(void)
{
	g_hdmi_drv->hdmi_task = kthread_create(_aw_hdmi_drv_run_thread, (void *)0, "hdmi proc");
	if (IS_ERR(g_hdmi_drv->hdmi_task)) {
		hdmi_err("%s: Unable to start kernel thread %s.\n",
				__func__, "hdmi proc");
		g_hdmi_drv->hdmi_task = NULL;
		return -1;
	}
	wake_up_process(g_hdmi_drv->hdmi_task);
	return 0;
}

void _aw_hpd_thread_exit(void)
{
	if (g_hdmi_drv->hdmi_task) {
		kthread_stop(g_hdmi_drv->hdmi_task);
		g_hdmi_drv->hdmi_task = NULL;
	}
}

static int _aw_hdmi_dts_parse_basic_info(struct platform_device *pdev)
{
	uintptr_t reg_base = 0x0;
	u32 cts_status = 0x0;

	/* iomap hdmi register base address */
	reg_base = (uintptr_t __force)of_iomap(pdev->dev.of_node, 0);
	if (reg_base == 0) {
		hdmi_err("%s: Unable to map hdmi registers\n", __func__);
		return -EINVAL;
	}
	g_hdmi_drv->reg_base = reg_base;

	/* get cts config */
	if (of_property_read_u32(pdev->dev.of_node, "hdmi_cts_compatibility",
			&cts_status)) {
		hdmi_err("%s: can not get hdmi cts compatibility\n", __func__);
		g_hdmi_drv->aw_dts.hdmi_cts = 0x0;
	} else
		g_hdmi_drv->aw_dts.hdmi_cts = cts_status;

	return 0;
}

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
static int _aw_hdmi_dts_parse_cec(struct platform_device *pdev)
{
	int ret = 0;
	u32 temp_val = 0x0;

	ret = of_property_read_u32(pdev->dev.of_node, "hdmi_cec_support", &temp_val);
	if (ret) {
		hdmi_err("%s: can not get hdmi_cec_support node\n", __func__);
		g_hdmi_drv->aw_dts.cec_support = 0x0;
	} else
		g_hdmi_drv->aw_dts.cec_support = temp_val;


	ret = of_property_read_u32(pdev->dev.of_node, "hdmi_cec_super_standby", &temp_val);
	if (ret) {
		hdmi_err("%s: can not get hdmi_cec_super_standby node\n", __func__);
		g_hdmi_drv->aw_dts.cec_super_standby = 0x0;
	} else
		g_hdmi_drv->aw_dts.cec_super_standby = temp_val;

	return 0;
}
#endif

static void _aw_hdmi_dts_parse_pin_config(struct platform_device *pdev)
{
	/* Get DDC GPIO */
	g_hdmi_drv->hdmi_pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(g_hdmi_drv->hdmi_pctl))
		hdmi_err("%s: ddc can not get pinctrl\n", __func__);

	/* get ddc control gpio enable config */
	if (of_property_read_u32(pdev->dev.of_node,
			"ddc_en_io_ctrl", &g_hdmi_drv->ddc_ctrl_en))
		hdmi_err("%s: can not get ddc_en_io_ctrl\n", __func__);

	if (g_hdmi_drv->ddc_ctrl_en) {
		g_hdmi_drv->ddc_gpio.gpio =
			of_get_named_gpio_flags(pdev->dev.of_node,
				"ddc_io_ctrl", 0,
				(enum of_gpio_flags *)(&(g_hdmi_drv->ddc_gpio)));

		if (gpio_request(g_hdmi_drv->ddc_gpio.gpio, NULL) != 0) {
			hdmi_err("%s: ddc ctrl gpio_request is failed\n",
					__func__);
		}
	}
}

static int _aw_hdmi_dts_parse_clk(struct platform_device *pdev)
{
	/* get tcon tv clock */
	g_hdmi_drv->aw_clock.clk_tcon_tv = devm_clk_get(&pdev->dev, "clk_tcon_tv");
	if (IS_ERR(g_hdmi_drv->aw_clock.clk_tcon_tv)) {
		hdmi_err("%s: fail to get clk for tcon tv\n", __func__);
		g_hdmi_drv->aw_clock.clk_tcon_tv = NULL;
	}

#if (IS_ENABLED(CONFIG_ARCH_SUN8IW20) || IS_ENABLED(CONFIG_ARCH_SUN20IW1))
	g_hdmi_drv->aw_clock.clk_hdmi = NULL;
#else
	g_hdmi_drv->aw_clock.clk_hdmi = devm_clk_get(&pdev->dev, "clk_hdmi");
	if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdmi)) {
		hdmi_err("%s: fail to get clk for hdmi\n", __func__);
		return -1;
	}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	g_hdmi_drv->aw_clock.clk_hdmi_bus = NULL;
	g_hdmi_drv->aw_clock.clk_hdmi_ddc = NULL;
	g_hdmi_drv->aw_clock.clk_hdmi_24M = devm_clk_get(&pdev->dev, "clk_hdmi_24M");
	if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdmi_24M)) {
		hdmi_err("%s: fail to get clk_hdmi_24M for hdmi inno phy\n",
				__func__);
		return -1;
	}
#elif defined(CONFIG_ARCH_SUN60IW2)
	g_hdmi_drv->aw_clock.clk_hdmi_bus = NULL;
	g_hdmi_drv->aw_clock.clk_hdmi_ddc = NULL;
	g_hdmi_drv->aw_clock.clk_hdmi_24M = NULL;
#else
	g_hdmi_drv->aw_clock.clk_hdmi_bus = devm_clk_get(&pdev->dev, "clk_bus_hdmi");
	if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdmi_bus)) {
		hdmi_err("%s: fail to get bus clk for hdmi\n", __func__);
		return -1;
	}

	g_hdmi_drv->aw_clock.clk_hdmi_ddc = devm_clk_get(&pdev->dev, "clk_ddc");
	if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdmi_ddc)) {
		hdmi_err("%s: fail to get clk for hdmi ddc\n", __func__);
		return -1;
	}
	g_hdmi_drv->aw_clock.clk_hdmi_24M = NULL;
#endif

#if defined(CONFIG_ARCH_SUN60IW2)
	g_hdmi_drv->aw_clock.clk_hdmi_sfr = devm_clk_get(&pdev->dev, "clk_hdmi_sfr");
	if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdmi_sfr)) {
		hdmi_err("%s: fail to get clk_hdmi_sfr for hdmi phy\n",
				__func__);
		return -1;
	}
#else
	g_hdmi_drv->aw_clock.clk_hdmi_sfr = NULL;
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (g_hdmi_drv->aw_dts.support_hdcp) {
		g_hdmi_drv->aw_clock.clk_hdcp = devm_clk_get(&pdev->dev, "clk_hdcp");
		if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdcp)) {
			hdmi_err("%s: fail to get clk for hdmi hdcp\n", __func__);
			/* return -1; */
		}

#if defined(CONFIG_ARCH_SUN60IW2)
	g_hdmi_drv->aw_clock.clk_hdcp_bus = NULL;
#else
		g_hdmi_drv->aw_clock.clk_hdcp_bus = devm_clk_get(&pdev->dev, "clk_bus_hdcp");
		if (IS_ERR(g_hdmi_drv->aw_clock.clk_hdcp_bus)) {
			hdmi_err("%s: fail to get clk for hdmi hdcp bus\n", __func__);
			/* return -1; */
		}
#endif

	}
#else
	g_hdmi_drv->aw_clock.clk_hdcp = NULL;
	g_hdmi_drv->aw_clock.clk_hdcp_bus = NULL;
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	if (g_hdmi_drv->aw_dts.cec_support) {
		g_hdmi_drv->aw_clock.clk_cec = devm_clk_get(&pdev->dev, "clk_cec");
		if (IS_ERR(g_hdmi_drv->aw_clock.clk_cec)) {
			hdmi_err("%s: fail to get clk for hdmi cec\n", __func__);
			/* return -1; */
		}
		g_hdmi_drv->aw_clock.clk_cec_parent = clk_get_parent(g_hdmi_drv->aw_clock.clk_cec);
		if (IS_ERR(g_hdmi_drv->aw_clock.clk_cec_parent)) {
			hdmi_err("%s: fail to get clk parent for hdmi cec\n", __func__);
			/* return -1; */
		}
	}
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	g_hdmi_drv->aw_clock.rst_bus_hdcp = devm_reset_control_get(&pdev->dev, "rst_bus_hdcp");
	if (IS_ERR(g_hdmi_drv->aw_clock.rst_bus_hdcp)) {
		hdmi_err("%s: fail to get clk for hdmi rst_bus_hdcp\n", __func__);
		return -1;
	}
#else
	g_hdmi_drv->aw_clock.rst_bus_hdcp = NULL;
#endif
	g_hdmi_drv->aw_clock.rst_bus_sub = devm_reset_control_get(&pdev->dev, "rst_bus_sub");
	if (IS_ERR(g_hdmi_drv->aw_clock.rst_bus_sub)) {
		hdmi_err("%s: fail to get clk for hdmi rst_bus_sub\n", __func__);
		return -1;
	}

	g_hdmi_drv->aw_clock.rst_bus_main = devm_reset_control_get(&pdev->dev, "rst_bus_main");
	if (IS_ERR(g_hdmi_drv->aw_clock.rst_bus_main)) {
		hdmi_err("%s: fail to get clk for hdmi rst_bus_main\n", __func__);
		return -1;
	}

#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
#else
	if (g_hdmi_drv->aw_clock.rst_bus_sub) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_sub)) {
			hdmi_err("%s: deassert bus sub failed!\n", __func__);
			return -1;
		}
	}

	if (g_hdmi_drv->aw_clock.rst_bus_main) {
		if (reset_control_deassert(g_hdmi_drv->aw_clock.rst_bus_main)) {
			hdmi_err("%s: deassert bus main failed!\n", __func__);
			return -1;
		}
	}
#endif

	return 0;
}

static int _aw_hdmi_dts_parse_power(struct platform_device *pdev)
{
	int ret = 0, i;
	const char *hdmi_power;
	char power_name[20];
	struct regulator *regulator;

	if (of_property_read_u32(pdev->dev.of_node, "hdmi_power_cnt",
			&g_hdmi_drv->aw_power.power_count)) {
		hdmi_err("%s: can not get hdmi power count from dts\n", __func__);
		return -1;
	}

	for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++) {
		sprintf(power_name, "hdmi_power%d", i);
		if (of_property_read_string(pdev->dev.of_node, power_name,
				&hdmi_power)) {
			hdmi_err("%s: failed get %s from dts\n", __func__, power_name);
			ret = -1;
		} else {
			hdmi_inf("get hdmi power%d: %s is success.\n", i, hdmi_power);
			memcpy((void *)g_hdmi_drv->aw_power.power_name[i], hdmi_power,
				strlen(hdmi_power) + 1);
			regulator = regulator_get(&g_hdmi_drv->pdev->dev, g_hdmi_drv->aw_power.power_name[i]);
			if (!regulator) {
				hdmi_wrn("regulator get for %s failed\n", g_hdmi_drv->aw_power.power_name[i]);
				continue;
			}
			g_hdmi_drv->aw_power.power_regu[i] = regulator;
			_aw_hdmi_drv_power_enable(g_hdmi_drv->aw_power.power_regu[i]);
		}
	}

	return ret;
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
static int _aw_hdmi_dts_parse_hdcp(struct platform_device *pdev,
		dw_hdcp_param_t *hdcp)
{
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	u32 dts_esm_buff_phy_addr = 0;
	u32 dts_esm_size_phy_addr = 0;
	void *dts_esm_buff_vir_addr = NULL;
	void *dts_esm_size_vir_addr = NULL;
	struct device_node *esm_np = NULL;
#endif

	/* get hdcp configs */
	if (of_property_read_u32_array(pdev->dev.of_node,
			"hdmi_hdcp_enable", (u32 *)&hdcp->use_hdcp, 1))
		hdmi_err("%s: can not get hdmi_hdcp_enable\n", __func__);
	else
		g_hdmi_drv->aw_dts.support_hdcp = hdcp->use_hdcp;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	if (of_property_read_u32_array(pdev->dev.of_node, "hdmi_hdcp22_enable",
			(u32 *)&hdcp->use_hdcp22, 1))
		hdmi_err("%s: can not get hdmi_hdcp22_enable\n", __func__);
	else
		g_hdmi_drv->aw_dts.support_hdcp22 = hdcp->use_hdcp22;

	esm_np = of_find_node_by_name(NULL, "esm");
	if (esm_np == NULL) {
		hdmi_wrn("Can not find the node of esm\n");
	} else if (hdcp->use_hdcp22) {
		if (!of_property_read_u32_array(esm_np,
				"esm_img_size_addr", &dts_esm_size_phy_addr, 1)) {

			/* obtain esm firmware size */
			dts_esm_size_vir_addr = __va(dts_esm_size_phy_addr);
			memcpy((void *)(&hdcp->esm_firm_size), dts_esm_size_vir_addr, 4);

			if (!of_property_read_u32_array(esm_np,
					"esm_img_buff_addr", &dts_esm_buff_phy_addr, 0x1)) {

				/* obtain esm firmware */
				dts_esm_buff_vir_addr = __va(dts_esm_buff_phy_addr);
				if (hdcp->esm_firm_size <= HDCP22_FIRMWARE_SIZE)
					memcpy((void *)hdcp->esm_firm_vir_addr,
						dts_esm_buff_vir_addr, hdcp->esm_firm_size);
				else
					hdmi_err("%s: esm firm size is too big\n", __func__);
			} else {
				hdmi_err("%s: can not read esm_img_buff_addr\n", __func__);
			}
		} else {
			hdmi_err("%s: can not read esm_img_size_addr\n", __func__);
		}
	}
#endif
	return 0;
}

int _aw_hdmi_drv_hdcp_init(struct platform_device *pdev,
		dw_hdcp_param_t *hdcp)
{
#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP22_SUNXI)
	hdcp->esm_firm_vir_addr = (unsigned long)dma_alloc_coherent(&pdev->dev,
		HDCP22_FIRMWARE_SIZE, &hdcp->esm_firm_phy_addr,
		GFP_KERNEL | __GFP_ZERO);
	hdcp->esm_firm_size = HDCP22_FIRMWARE_SIZE;

	gHdcp_esm_fw_vir_addr = (char *)hdcp->esm_firm_vir_addr;
	gHdcp_esm_fw_size = hdcp->esm_firm_size;

	hdcp->esm_data_vir_addr =  (unsigned long)dma_alloc_coherent(&pdev->dev,
		HDCP22_DATA_SIZE, &hdcp->esm_data_phy_addr,
		GFP_KERNEL | __GFP_ZERO);
	hdcp->esm_data_size = HDCP22_DATA_SIZE;

	hdcp->esm_hpi_base = g_hdmi_drv->reg_base + ESM_REG_BASE_OFFSET;

	hdcp->esm_firm_phy_addr -= 0x40000000;
	hdcp->esm_data_phy_addr -= 0x40000000;
#endif
	return 0;
}
#endif

/* ******************* SND_HDMI for sunxi_v2 begain ************************** */

/**
 * @desc: sound hdmi audio enable
 * @enable: 1 - enable hdmi audio
 *          0 - disable hdmi audio
 * @channel:
 * @return: 0 - success
 *         -1 - failed
*/
static s32 _snd_hdmi_audio_enable(u8 enable, u8 channel)
{
	struct aw_hdmi_core_s *p_core = g_hdmi_drv->hdmi_core;

	LOG_TRACE();

	if (enable == 0)
		return 0;

	if (!p_core->audio_ops.audio_config()) {
		hdmi_err("%s: HDMI Audio Configure failed\n", __func__);
		return -1;
	}

	hdmi_inf("HDMI Audio enable successfully\n");

	return 0;
}

/**
 * @desc: sound hdmi audio param config
 * @audio_para: audio params
 * @return: 0 - success
 *         -1 - failed
*/
static s32 _snd_hdmi_set_audio_para(hdmi_audio_t *audio_para)
{
	struct aw_hdmi_core_s *p_core = g_hdmi_drv->hdmi_core;
	dw_audio_param_t *pAudio = &p_core->mode.pAudio;

	LOG_TRACE();

	memset(pAudio, 0, sizeof(dw_audio_param_t));
	if (audio_para->hw_intf < 2)
		pAudio->mInterfaceType = audio_para->hw_intf;
	else
		hdmi_wrn("HDMI Audio unknow hardware interface type %d\n",
				audio_para->hw_intf);

	pAudio->mCodingType = audio_para->data_raw;
	if (pAudio->mCodingType < 1)
		pAudio->mCodingType = DW_AUD_CODING_PCM;

	pAudio->mSamplingFrequency = audio_para->sample_rate;
	pAudio->mChannelAllocation = audio_para->ca;
	pAudio->mChannelNum = audio_para->channel_num;
	pAudio->mSampleSize = audio_para->sample_bit;
	pAudio->mClockFsFactor = audio_para->fs_between;

	hdmi_inf("HDMI Audio set params successfully\n");

	return 0;
}

int snd_hdmi_get_func(__audio_hdmi_func *hdmi_func)
{
	if (!hdmi_func) {
		hdmi_err("%s: HDMI Audio func is NULL!\n", __func__);
		return -1;
	}

	hdmi_func->hdmi_audio_enable   = _snd_hdmi_audio_enable;
	hdmi_func->hdmi_set_audio_para = _snd_hdmi_set_audio_para;

	return 0;
}

/* ******************* SND_HDMI for sunxi_v2 end ***************************** */

static void _aw_disp_hdmi_blacklist_process(struct disp_device_config *config)
{
	u8 issue = 0;

	issue = g_hdmi_drv->hdmi_core->dev_ops.dev_get_blacklist_issue((u32)config->mode);

	if ((issue & 0x02) &&
			((config->format == (enum disp_csc_type)DW_COLOR_FORMAT_YCC444) ||
			 (config->format == (enum disp_csc_type)DW_COLOR_FORMAT_YCC422))) {
		/* Sink not support yuv on this mode */
		config->format = (enum disp_csc_type)DW_COLOR_FORMAT_RGB;
		VIDEO_INF("Sink is on blacklist and not support YUV on mode %d\n", config->mode);
	}
}

static u32 _aw_disp_hdmi_config_check_update(struct aw_hdmi_core_s *core,
							struct disp_device_config *config)
{
	u32 ret = DISP_CONFIG_UPDATE_NULL;

	if (config->mode != core->config.mode)
		ret |= DISP_CONFIG_UPDATE_MODE;
	if ((config->format != core->config.format))
		ret |= DISP_CONFIG_UPDATE_FORMAT;
	if (config->bits != core->config.bits)
		ret |= DISP_CONFIG_UPDATE_BITS;
	if (config->eotf != core->config.eotf)
		ret |= DISP_CONFIG_UPDATE_EOTF;
	if (config->cs != core->config.cs)
		ret |= DISP_CONFIG_UPDATE_CS;
	if (config->dvi_hdmi != core->config.dvi_hdmi)
		ret |= DISP_CONFIG_UPDATE_DVI;
	if (config->range != core->config.range)
		ret |= DISP_CONFIG_UPDATE_RANGE;
	if (config->scan != core->config.scan)
		ret |= DISP_CONFIG_UPDATE_SCAN;
	if (config->aspect_ratio != core->config.aspect_ratio)
		ret |= DISP_CONFIG_UPDATE_RATIO;

	return ret;
}

static s32 _aw_disp_hdmi_enable(void)
{
	s32 ret = 0;
#if IS_ENABLED(CONFIG_HDMI2_FREQ_SPREAD_SPECTRUM)
	struct disp_video_timings *video_info;
	u32 clk_rate = 0;
	dw_video_param_t *pVideo = &g_hdmi_drv->hdmi_core->mode.pVideo;
#endif

	hdmi_inf("%s start.\n", __func__);

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_ctrl);

	if (bHdmi_drv_enable) {
		hdmi_wrn("disp set hdmi enable but hdmi already enable!\n");
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		return 0;
	}

#if (!IS_ENABLED(CONFIG_ARCH_SUN55IW3))
	if (!bHdmi_hpd_state) {
		hdmi_wrn("disp set hdmi enable but hdmi not detect hpd!\n");
		bHdmi_drv_enable = true;
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		return 0;
	}
#endif

	if (bHdmi_video_enable) {
		hdmi_wrn("disp set hdmi enable but hdmi is video on!\n");
		bHdmi_drv_enable = true;
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		return 0;
	}

	if (g_hdmi_drv->aw_clock.clk_hdmi != NULL) {
		if (g_hdmi_drv->aw_clock.clk_tcon_tv != NULL) {
			clk_set_rate(g_hdmi_drv->aw_clock.clk_hdmi,
					clk_get_rate(g_hdmi_drv->aw_clock.clk_tcon_tv));
		} else {
			hdmi_err("%s: tcon_tv clk get failed!\n", __func__);
		}
#if IS_ENABLED(CONFIG_HDMI2_FREQ_SPREAD_SPECTRUM)
		_aw_disp_hdmi_get_video_timming_info(&video_info);
		clk_rate = video_info->pixel_clk
			* (video_info->pixel_repeat + 1);

		if (pVideo->mEncodingOut == DW_COLOR_FORMAT_YCC420)
			clk_rate /= 2;
		_aw_freq_spread_sprectrum_enabled(clk_rate);
#endif
	}

	_aw_hdmi_set_tcon_pad(true);

	ret = g_hdmi_drv->hdmi_core->dev_ops.dev_config();
	if (!ret) {
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		hdmi_err("%s: disp enable hdmi main config failed!\n", __func__);
		return -1;
	}
	hdmi_inf("disp enable hdmi main config success.\n");

	ret = g_hdmi_drv->hdmi_core->audio_ops.audio_config();
	if (!ret) {
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		hdmi_err("%s: disp enable hdmi audio config failed!\n", __func__);
		return -1;
	}

	hdmi_inf("disp enable hdmi success.\n");
	bHdmi_video_enable = true;
	bHdmi_drv_enable = true;

	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
	return 0;
}

static s32 _aw_disp_hdmi_smooth_enable(void)
{
	s32 ret = 0;

	hdmi_inf("%s start.\n", __func__);

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_ctrl);

	if (bHdmi_hpd_state && bHdmi_video_enable)
		ret = g_hdmi_drv->hdmi_core->dev_ops.dev_smooth_enable();

	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);

	return ret;
}

static s32 _aw_disp_hdmi_disable(void)
{
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	hdmi_inf("%s start.\n", __func__);

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_ctrl);
	if (!bHdmi_drv_enable) {
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		return 0;
	}

	/* Stand by */
	core->video_ops.set_avmute(1);
	mdelay(100);
	if (core->mode.pHdcp.hdcp_on)
		core->hdcp_ops.hdcp_disconfigure();
	if (!g_hdmi_drv->aw_dts.cec_super_standby)
		core->dev_ops.dev_close();
	else
		core->dev_ops.dev_standby();

#if !IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	_aw_hdmi_set_tcon_pad(false);
#endif

	bHdmi_video_enable = false;
	bHdmi_drv_enable = false;

	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
	return 0;
}

static s32 _aw_disp_hdmi_suspend(void)
{
	int i = 0;

	hdmi_inf("%s start.\n", __func__);

	if (bHdmi_suspend_enable) {
		hdmi_wrn("hdmi has been suspend!\n");
		return 0;
	}

	_aw_hpd_thread_exit();

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	if (g_hdmi_drv->hdmi_core->mode.pHdcp.hdcp_on)
		aw_hdmi_drv_set_hdcp_state(AW_HDCP_ING);
	if (g_hdmi_drv->hdmi_core->mode.pHdcp.use_hdcp)
		g_hdmi_drv->hdmi_core->hdcp_ops.hdcp_close();
#endif

#ifdef SUNXI_HDMI20_PHY_AW
	g_hdmi_drv->hdmi_core->phy_ops.phy_reset();
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	/* the suspend function that run last is responsible for releasing resources */
	if (bHdmi_cec_suspend_enable) {
		hdmi_inf("release resources in %s\n", __func__);
#else
	{
#endif
		_aw_hdmi_drv_resource_release_for_suspend();

		for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++)
			_aw_hdmi_drv_power_disable(g_hdmi_drv->aw_power.power_regu[i]);

		_aw_hdmi_set_tcon_pad(false);
		disp_hdmi_clk_disable();

		if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
			_aw_hdmi_drv_dfs_set_state_sync(0);
	}

	bHdmi_suspend_enable = true;
	hdmi_inf("%s finish.\n", __func__);
	return 0;
}

/* extern int sunxi_smc_refresh_hdcp(void); */
static s32 _aw_disp_hdmi_resume(void)
{
	s32 ret = 0, i = 0;

	hdmi_inf("%s start.\n", __func__);

#if IS_ENABLED(CONFIG_AW_SMC) && IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	if (sunxi_smc_refresh_hdcp())
		hdmi_wrn("can not refresh hdcp key when hdmi resume!\n");
#endif

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_ctrl);
	if (!bHdmi_suspend_enable) {
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		hdmi_wrn("hdmi has been resume!\n");
		return 0;
	}

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	/* the resume function that run first is responsible for configure resources */
	if (bHdmi_cec_suspend_enable) {
		hdmi_inf("config resource in %s\n", __func__);
#else
	{
#endif
		_aw_hdmi_set_tcon_pad(true);

		for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++)
			_aw_hdmi_drv_power_enable(g_hdmi_drv->aw_power.power_regu[i]);

		_aw_hdmi_drv_resource_config_for_resume();

		_aw_hdmi_drv_clock_reset();
	}

#ifdef SUNXI_HDMI20_PHY_AW
	g_hdmi_drv->hdmi_core->phy_ops.phy_resume();
#endif

	/* enable hpd sense */
	g_hdmi_drv->hdmi_core->phy_ops.set_hpd(true);

	bHdmi_hpd_state = 0;
	_aw_hdmi_drv_hpd_notify(0x0);
	if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
		_aw_hdmi_drv_dfs_set_state_sync(0);

	ret = _aw_hpd_thread_init();
	if (ret != 0) {
		hdmi_err("%s: aw hdmi thread init failed when resume!!!\n",
				__func__);
		goto exit;
	}

	bHdmi_suspend_enable = false;

exit:
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);

	hdmi_inf("%s finish.\n", __func__);

	return ret;
}

static s32 _aw_disp_hdmi_mode_check(u32 mode)
{
	return g_hdmi_drv->hdmi_core->dev_ops.dev_tv_mode_check(mode);
}

static s32 _aw_disp_hdmi_get_hpd_state(void)
{
	return g_hdmi_drv->hdmi_core->phy_ops.get_hpd();
}

static s32 _aw_disp_hdmi_get_csc_type(void)
{
	return g_hdmi_drv->hdmi_core->video_ops.get_color_format();
}

static s32 _aw_disp_hdmi_get_color_range(void)
{
	enum disp_color_range range;
	u8 drv_range = g_hdmi_drv->hdmi_core->video_ops.get_color_range();

	switch (drv_range) {
	case 0:
		range = DISP_COLOR_RANGE_DEFAULT;
		break;
	case 2:
		range = DISP_COLOR_RANGE_0_255;
		break;
	case 1:
		range = DISP_COLOR_RANGE_16_235;
		break;
	default:
		range = DISP_COLOR_RANGE_0_255;
		hdmi_wrn("driver color range is unkonw! %d\n", drv_range);
		break;
	}

	return (s32)range;
}

static s32 _aw_disp_hdmi_get_video_timming_info(struct disp_video_timings **video_info)
{
	dw_dtd_t *dtd = NULL;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	hdmi_inf("%s: start.\n", __func__);

	if (!core) {
		hdmi_err("%s: param core is null!!!\n", __func__);
		return -1;
	}

	dtd = &core->mode.pVideo.mDtd;
	if (!dtd) {
		hdmi_err("%s: param dtd is null!!!\n", __func__);
		return -1;
	}

	g_disp_info.vic       = dtd->mCode;
	g_disp_info.tv_mode   = 0;
	g_disp_info.pixel_clk = (dtd->mPixelClock) * 1000 / (dtd->mPixelRepetitionInput + 1);

	if ((g_disp_info.vic == 6) || (g_disp_info.vic == 7) ||
			(g_disp_info.vic == 21) || (g_disp_info.vic == 22))
		g_disp_info.pixel_clk = (dtd->mPixelClock) * 1000 / (dtd->mPixelRepetitionInput + 1) / (dtd->mInterlaced + 1);

	g_disp_info.pixel_repeat = dtd->mPixelRepetitionInput;
	g_disp_info.x_res = (dtd->mHActive) / (dtd->mPixelRepetitionInput+1);

	if (dtd->mInterlaced == 1)
		g_disp_info.y_res = (dtd->mVActive) * 2;
	else if (dtd->mInterlaced == 0)
		g_disp_info.y_res = dtd->mVActive;

	g_disp_info.hor_total_time  = (dtd->mHActive + dtd->mHBlanking) / (dtd->mPixelRepetitionInput+1);
	g_disp_info.hor_back_porch  = (dtd->mHBlanking - dtd->mHSyncOffset - dtd->mHSyncPulseWidth) / (dtd->mPixelRepetitionInput + 1);
	g_disp_info.hor_front_porch = (dtd->mHSyncOffset) / (dtd->mPixelRepetitionInput+1);
	g_disp_info.hor_sync_time   = (dtd->mHSyncPulseWidth) / (dtd->mPixelRepetitionInput+1);

	if (dtd->mInterlaced == 1)
		g_disp_info.ver_total_time = (dtd->mVActive + dtd->mVBlanking) * 2 + 1;
	else if (dtd->mInterlaced == 0)
		g_disp_info.ver_total_time = dtd->mVActive + dtd->mVBlanking;

	g_disp_info.ver_back_porch  = dtd->mVBlanking - dtd->mVSyncOffset - dtd->mVSyncPulseWidth;
	g_disp_info.ver_front_porch = dtd->mVSyncOffset;
	g_disp_info.ver_sync_time   = dtd->mVSyncPulseWidth;

	g_disp_info.hor_sync_polarity = dtd->mHSyncPolarity;
	g_disp_info.ver_sync_polarity = dtd->mVSyncPolarity;
	g_disp_info.b_interlace       = dtd->mInterlaced;

	if (dtd->mCode == HDMI_1080P_24_3D_FP) {
		g_disp_info.y_res = (dtd->mVActive) * 2;
		g_disp_info.vactive_space = 45;
		g_disp_info.trd_mode = 1;
	} else if (dtd->mCode == HDMI_720P_50_3D_FP) {
		g_disp_info.y_res = (dtd->mVActive) * 2;
		g_disp_info.vactive_space = 30;
		g_disp_info.trd_mode = 1;
	} else if (dtd->mCode == HDMI_720P_60_3D_FP) {
		g_disp_info.y_res = (dtd->mVActive) * 2;
		g_disp_info.vactive_space = 30;
		g_disp_info.trd_mode = 1;
	} else {
		g_disp_info.vactive_space = 0;
		g_disp_info.trd_mode = 0;
	}

	*video_info = &g_disp_info;
	return 0;
}

static s32 _aw_disp_hdmi_set_static_config(struct disp_device_config *config)
{
	u32 data_bit = 0;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;
	dw_video_param_t *pVideo = &core->mode.pVideo;

	hdmi_inf("%s: start.\n", __func__);

	_aw_disp_hdmi_blacklist_process(config);

	hdmi_inf("[HDMI receive params]: tv mode: 0x%x format:0x%x data bits:0x%x eotf:0x%x cs:0x%x dvi_hdmi:%x range:%x scan:%x aspect_ratio:%x\n",
				config->mode, config->format, config->bits, config->eotf, config->cs,
				config->dvi_hdmi, config->range, config->scan, config->aspect_ratio);

	/* prevent mem leak */
	if (pVideo->pb) {
		kfree(pVideo->pb);
		pVideo->pb = NULL;
	}
	memset(pVideo, 0, sizeof(dw_video_param_t));
	pVideo->update = _aw_disp_hdmi_config_check_update(core, config);

	/* set vic mode and dtd */
	g_hdmi_drv->hdmi_core->dev_ops.dev_tv_mode_update_dtd((u32)config->mode);

	/* set encoding mode */
	pVideo->mEncodingIn  = (dw_color_format_t)config->format;
	pVideo->mEncodingOut = (dw_color_format_t)config->format;
#ifdef USE_CSC
	if (config->format == (u8)DW_COLOR_FORMAT_YCC422) {
		pVideo->mEncodingIn  = DW_COLOR_FORMAT_YCC444;
		pVideo->mEncodingOut = DW_COLOR_FORMAT_YCC422;
	}
#endif

	/* set data bits */
	if ((config->bits >= 0) && (config->bits < 3))
		data_bit = 8 + 2 * config->bits;
	if (config->bits == 3)
		data_bit = 16;
	pVideo->mColorResolution = (u8)data_bit;

	/* set eotf */
	if (config->eotf) {
		if (core->mode.pVideo.pb == NULL) {
			pVideo->pb = kmalloc(sizeof(dw_fc_drm_pb_t), GFP_KERNEL);
			if (pVideo->pb) {
				memset(pVideo->pb, 0, sizeof(dw_fc_drm_pb_t));
			} else {
				hdmi_err("%s: Can not alloc memory for dynamic range and mastering infoframe\n",
						__func__);
				return -1;
			}
		}
		if (pVideo->pb) {
			pVideo->pb->r_x = 0x33c2;
			pVideo->pb->r_y = 0x86c4;
			pVideo->pb->g_x = 0x1d4c;
			pVideo->pb->g_y = 0x0bb8;
			pVideo->pb->b_x = 0x84d0;
			pVideo->pb->b_y = 0x3e80;
			pVideo->pb->w_x = 0x3d13;
			pVideo->pb->w_y = 0x4042;
			pVideo->pb->luma_max = 0x03e8;
			pVideo->pb->luma_min = 0x1;
			pVideo->pb->mcll = 0x03e8;
			pVideo->pb->mfll = 0x0190;
		}


		switch (config->eotf) {
		case DISP_EOTF_GAMMA22:
			pVideo->mHdr = 0;
			pVideo->pb->eotf = DW_EOTF_SDR;
			break;
		case DISP_EOTF_SMPTE2084:
			pVideo->mHdr = 1;
			pVideo->pb->eotf = DW_EOTF_SMPTE2084;
			break;
		case DISP_EOTF_ARIB_STD_B67:
			pVideo->mHdr = 1;
			pVideo->pb->eotf = DW_EOTF_HLG;
			break;
		default:
			hdmi_wrn("Unknow ouput eotf!\n");
			break;
		}

	}
	/* set color space */
	switch (config->cs) {
	case DISP_UNDEF:
		pVideo->mColorimetry = 0;
		pVideo->mExtColorimetry = 0;
		break;
	case DISP_BT601:
		pVideo->mColorimetry = DW_METRY_ITU601;
		pVideo->mExtColorimetry = 0;
		break;
	case DISP_BT709:
		pVideo->mColorimetry = DW_METRY_ITU709;
		pVideo->mExtColorimetry = 0;
		break;
	case DISP_BT2020NC:
		pVideo->mColorimetry = DW_METRY_EXTENDED;
		pVideo->mExtColorimetry = DW_METRY_EXT_BT2020_Y_CB_CR;
		break;
	default:
		pVideo->mColorimetry = 0;
		hdmi_wrn("Unknow ouput color space!\n");
		break;
	}

	/* set output mode: hdmi or avi */
	switch (config->dvi_hdmi) {
	case DISP_DVI_HDMI_UNDEFINED:
		pVideo->mHdmi = DW_TMDS_MODE_HDMI;
		break;
	case DISP_DVI:
		pVideo->mHdmi = DW_TMDS_MODE_DVI;
		break;
	case DISP_HDMI:
		pVideo->mHdmi = DW_TMDS_MODE_HDMI;
		break;
	default:
		pVideo->mHdmi = DW_TMDS_MODE_HDMI;
		hdmi_wrn("Unknow ouput dvi_hdmi!\n");
		break;
	}

	/* set clor range: defult/limited/full */
	switch (config->range) {
	case DISP_COLOR_RANGE_DEFAULT:
		pVideo->mRgbQuantizationRange = 0;
		break;
	case DISP_COLOR_RANGE_0_255:
		pVideo->mRgbQuantizationRange = 2;
		break;
	case DISP_COLOR_RANGE_16_235:
		pVideo->mRgbQuantizationRange = 1;
		break;
	default:
		hdmi_wrn("Unknow color range!\n");
		break;
	}

	/* set scan info */
	pVideo->mScanInfo = config->scan;

	/* set aspect ratio */
	if (config->aspect_ratio == 0)
		pVideo->mActiveFormatAspectRatio = 8;
	else
		pVideo->mActiveFormatAspectRatio = config->aspect_ratio;

	memcpy(&core->config, config, sizeof(struct disp_device_config));
	return 0;
}

static s32 _aw_disp_hdmi_get_static_config(struct disp_device_config *config)
{
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;

	hdmi_inf("%s: start.\n", __func__);

	config->mode = (enum disp_tv_mode)g_hdmi_drv->hdmi_core->dev_ops.dev_tv_mode_get();
	config->format = (enum disp_csc_type)core->mode.pVideo.mEncodingIn;

	if ((core->mode.pVideo.mColorResolution >= 8) && (core->mode.pVideo.mColorResolution < 16))
		config->bits = (core->mode.pVideo.mColorResolution - 8)/2;
	if (core->mode.pVideo.mColorResolution == 16)
		config->bits  = 4;

	if (core->mode.pVideo.pb != NULL) {

		switch (core->mode.pVideo.pb->eotf) {
		case DW_EOTF_SDR:
			config->eotf = DISP_EOTF_GAMMA22;
			break;
		case DW_EOTF_SMPTE2084:
			config->eotf = DISP_EOTF_SMPTE2084;
			break;
		case DW_EOTF_HLG:
			config->eotf = DISP_EOTF_ARIB_STD_B67;
			break;
		default:
			hdmi_wrn("Unknow eotf! %d\n", core->mode.pVideo.pb->eotf);
			break;
		}

		config->cs = DISP_BT2020NC;
	} else {
		if (config->mode < 4)
			config->cs = DISP_BT601;
		else
			config->cs = DISP_BT709;

	}

	switch (core->mode.pVideo.mRgbQuantizationRange) {
	case 0:
		config->range = DISP_COLOR_RANGE_DEFAULT;
		break;
	case 2:
		config->range = DISP_COLOR_RANGE_0_255;
		break;
	case 1:
		config->range = DISP_COLOR_RANGE_16_235;
		break;
	default:
		config->range = DISP_COLOR_RANGE_0_255;
		hdmi_wrn("Unknow mRgbQuantizationRange! %d\n", core->mode.pVideo.mRgbQuantizationRange);
		break;
	}

	return 0;
}

static s32 _aw_disp_hdmi_set_dynamic_config(struct disp_device_dynamic_config *config)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) &&\
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	struct dma_buf_map map;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
	struct iosys_map map;
#endif
	void *hdr_buff_addr = NULL;
	int ret = 0;
	struct dma_buf *dmabuf = NULL;
	struct aw_hdmi_core_s *core = g_hdmi_drv->hdmi_core;
	dw_video_param_t *pVideo = &core->mode.pVideo;
	struct hdr_static_metadata *hdr_smetadata = NULL;

	u8 *temp = NULL;
	dw_fc_drm_pb_t *dynamic_pb = NULL;

	hdmi_inf("%s: start.\n", __func__);

	temp = kmalloc(config->metadata_size, GFP_KERNEL);
	if (!temp) {
		hdmi_err("%s: kmalloc metadata memory failed!\n", __func__);
		return -1;
	}
	dynamic_pb = kmalloc(sizeof(dw_fc_drm_pb_t), GFP_KERNEL);
	if (!dynamic_pb) {
		hdmi_err("%s: kmalloc dynamic_pb memory failed!\n", __func__);
		kfree(temp);
		return -1;
	}

	LOG_TRACE();

	if ((_aw_hdmi_drv_get_hpd() == 0) || (bHdmi_clk_enable == false)) {
		hdmi_err("%s: hdmi not ready!\n", __func__);
		kfree(temp);
		kfree(dynamic_pb);
		return -1;
	}

	if (!((pVideo->pb->eotf == DW_EOTF_SMPTE2084)
	   || (pVideo->pb->eotf == DW_EOTF_HLG))) {
		kfree(temp);
		kfree(dynamic_pb);
		return -1;
	}

	/* get the virtual addr of metadata */
	dmabuf = dma_buf_get(config->metadata_fd);
	if (IS_ERR(dmabuf)) {
		hdmi_err("%s: dma_buf_get failed\n", __func__);
		kfree(temp);
		kfree(dynamic_pb);
		return -1;
	}

	ret = dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
	if (ret) {
		dma_buf_put(dmabuf);
		hdmi_err("%s: dmabuf cpu aceess failed\n", __func__);
		kfree(temp);
		kfree(dynamic_pb);
		return ret;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	ret = dma_buf_vmap(dmabuf, &map);
	hdr_buff_addr = map.vaddr;
	if (ret) {
		hdr_buff_addr = NULL;
	}
#else
	hdr_buff_addr = dma_buf_vmap(dmabuf);
#endif
	if (!hdr_buff_addr) {
		hdmi_err("%s: dma_buf_vmap failed\n", __func__);
		dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
		dma_buf_put(dmabuf);
		dmabuf = NULL;
		kfree(temp);
		kfree(dynamic_pb);
		return -1;
	}


	/* obtain metadata */
	memcpy((void *)temp, hdr_buff_addr, config->metadata_size);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		dma_buf_vunmap(dmabuf, &map);
#else
		dma_buf_vunmap(dmabuf, hdr_buff_addr);
#endif

	dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
	dma_buf_put(dmabuf);
	hdr_buff_addr = NULL;
	dmabuf = NULL;

	dynamic_pb->eotf = core->mode.pVideo.pb->eotf;
	dynamic_pb->metadata = core->mode.pVideo.pb->metadata;

	hdr_smetadata = (struct hdr_static_metadata *)temp;
	dynamic_pb->r_x =
		hdr_smetadata->disp_master.display_primaries_x[0];
	dynamic_pb->r_y =
		hdr_smetadata->disp_master.display_primaries_y[0];
	dynamic_pb->g_x =
		hdr_smetadata->disp_master.display_primaries_x[1];
	dynamic_pb->g_y =
		hdr_smetadata->disp_master.display_primaries_y[1];
	dynamic_pb->b_x =
		hdr_smetadata->disp_master.display_primaries_x[2];
	dynamic_pb->b_y =
		hdr_smetadata->disp_master.display_primaries_y[2];
	dynamic_pb->w_x =
		hdr_smetadata->disp_master.white_point_x;
	dynamic_pb->w_y =
		hdr_smetadata->disp_master.white_point_y;
	dynamic_pb->luma_max =
		hdr_smetadata->disp_master.max_display_mastering_luminance
		/ 10000;
	dynamic_pb->luma_min =
		hdr_smetadata->disp_master.min_display_mastering_luminance;
	dynamic_pb->mcll =
		hdr_smetadata->maximum_content_light_level;
	dynamic_pb->mfll =
		hdr_smetadata->maximum_frame_average_light_level;

	/* send metadata */
	core->video_ops.set_drm_up(dynamic_pb);

	kfree(temp);
	kfree(dynamic_pb);

	return 0;
}

s32 _aw_disp_hdmi_get_dynamic_config(struct disp_device_dynamic_config *config)
{
	hdmi_inf("%s: start.\n", __func__);
	return 0;
}

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
inline void aw_hdmi_drv_set_hdcp_state(u8 status)
{
	gHdmi_hdcp_state = status;
}

inline u8 aw_hdmi_drv_get_hdcp_state(void)
{
	return gHdmi_hdcp_state;
}

int aw_hdmi_drv_get_hdcp_type(struct aw_hdmi_core_s *core)
{
	if (core == NULL) {
		hdmi_err("%s: param is null!!!\n", __func__);
		return -1;
	}

	if (core->mode.pHdcp.use_hdcp == 0) {
		hdmi_err("%s: get hdcp type failed because not config use hdcp!\n", __func__);
		return -1;
	}

	if (core->mode.pHdcp.hdcp_on == 0) {
		hdmi_err("%s: get hdcp type failed because not set hdcp!\n", __func__);
		return -1;
	}

	return  core->hdcp_ops.hdcp_get_type();
}

void aw_hdmi_drv_hdcp_enable(struct aw_hdmi_core_s *core, u8 enable)
{
	if (enable) {
		if (core->mode.pHdcp.hdcp_on) {
			hdmi_wrn("hdcp has been enable!\n");
			return;
		}
		core->mode.pHdcp.hdcp_on = 1;
		if (_aw_hdmi_drv_get_hpd())
			core->hdcp_ops.hdcp_configure();
	} else {
		if (!core->mode.pHdcp.hdcp_on) {
			hdmi_wrn("hdcp has been disable!\n");
			return;
		}

		core->mode.pHdcp.hdcp_on = 0;
		core->hdcp_ops.hdcp_disconfigure();
	}
}
#endif

void aw_hdmi_drv_write(uintptr_t addr, u32 data)
{
	writeb((u8)data, (volatile void __iomem *)(
		g_hdmi_drv->reg_base + (addr >> 2)));
}

u32 aw_hdmi_drv_read(uintptr_t addr)
{
	return (u32)readb((volatile void __iomem *)(
		g_hdmi_drv->reg_base + (addr >> 2)));
}

void _aw_hdmi_init_disp_func(void)
{
	struct disp_device_func disp_func;

	memset(&disp_func, 0, sizeof(struct disp_device_func));

	disp_func.enable                 = _aw_disp_hdmi_enable;
	disp_func.smooth_enable          = _aw_disp_hdmi_smooth_enable;
	disp_func.disable                = _aw_disp_hdmi_disable;
	disp_func.suspend                = _aw_disp_hdmi_suspend;
	disp_func.resume                 = _aw_disp_hdmi_resume;
	disp_func.mode_support           = _aw_disp_hdmi_mode_check;
	disp_func.get_HPD_status         = _aw_disp_hdmi_get_hpd_state;
	disp_func.get_input_csc          = _aw_disp_hdmi_get_csc_type;
	disp_func.get_input_color_range  = _aw_disp_hdmi_get_color_range;
	disp_func.get_video_timing_info  = _aw_disp_hdmi_get_video_timming_info;
	disp_func.set_static_config      = _aw_disp_hdmi_set_static_config;
	disp_func.get_static_config      = _aw_disp_hdmi_get_static_config;
	disp_func.set_dynamic_config     = _aw_disp_hdmi_set_dynamic_config;
	disp_func.get_dynamic_config     = _aw_disp_hdmi_get_dynamic_config;

	disp_set_hdmi_func(&disp_func);
}

int _aw_hdmi_init_thread(void)
{
	if (_aw_hpd_thread_init() != 0) {
		hdmi_err("%s: hdmi hpd thread init failed!!!\n", __func__);
		return -1;
	}
	return 0;
}

int _aw_hdmi_init_mutex_lock(void)
{
	if (!g_hdmi_drv) {
		hdmi_err("%s: g_hdmi_drv is null!!!\n", __func__);
		return -1;
	}

	mutex_init(&g_hdmi_drv->aw_mutex.lock_tcon);
	mutex_init(&g_hdmi_drv->aw_mutex.lock_ctrl);
	mutex_init(&g_hdmi_drv->aw_mutex.lock_hdcp);
#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	mutex_init(&g_hdmi_drv->aw_mutex.lock_cec);
	mutex_init(&g_hdmi_drv->aw_mutex.lock_cec_notifier);
#endif
	return 0;
}

int _aw_hdmi_init_resoure(struct platform_device *pdev, dw_hdcp_param_t *hdcp)
{
	int ret = 0x0;
	ret = _aw_hdmi_dts_parse_basic_info(pdev);
	if (ret != 0) {
		hdmi_err("%s: aw dts parse basic info failed!!!\n", __func__);
		return -1;
	}

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	ret = _aw_hdmi_dts_parse_cec(pdev);
	if (ret != 0)
		hdmi_wrn("aw dts parse cec info failed!!!\n");
#endif

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	_aw_hdmi_dts_parse_hdcp(pdev, hdcp);
#endif

	_aw_hdmi_dts_parse_pin_config(pdev);

	ret = _aw_hdmi_dts_parse_clk(pdev);
	if (ret) {
		hdmi_err("%s: aw dts parse clock info failed!!!\n", __func__);
		return -1;
	}

	_aw_hdmi_dts_parse_power(pdev);

	return 0;
}

inline void aw_hdmi_drv_set_hpd_mask(u32 mask)
{
	gHdmi_hpd_force = mask;
}

inline u32 aw_hdmi_drv_get_hpd_mask(void)
{
	return gHdmi_hpd_force;
}

inline u32 aw_hdmi_drv_get_clk_enable(void)
{
	return bHdmi_clk_enable;
}

inline u32 aw_hdmi_drv_get_enable_mask(void)
{
	return bHdmi_drv_enable;
}

inline u32 aw_hdmi_drv_get_suspend_mask(void)
{
	return bHdmi_suspend_enable;
}

inline u32 aw_hdmi_drv_get_boot_enable(void)
{
	return bHdmi_boot_enable;
}

inline u32 aw_hdmi_drv_get_video_enable(void)
{
	return bHdmi_video_enable;
}

void _aw_hdmi_init_global_value(void)
{
	gHdmi_log_level = 0;
	gHdmi_hpd_force = 0;
	gHdmi_hpd_force_pre = 0;
	gHdmi_plugout_count = 0;
	gHdmi_edid_repeat_count = 0;

	bHdmi_clk_enable     = false;
	bHdmi_pin_enable     = false;
	bHdmi_drv_enable     = false;
	bHdmi_suspend_enable = false;
	bHdmi_cec_suspend_enable = false;

	bHdmi_boot_enable  = false;
	bHdmi_video_enable = false;
	bHdmi_hpd_state = 0;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	aw_hdmi_drv_set_hdcp_state(AW_HDCP_DISABLE);
#endif
}

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
static void _aw_hdmi_cec_drv_clock_enable(void)
{
	if (g_hdmi_drv->aw_clock.clk_cec != NULL) {
		if (g_hdmi_drv->aw_clock.clk_cec_parent) {
			clk_set_parent(g_hdmi_drv->aw_clock.clk_cec,
					g_hdmi_drv->aw_clock.clk_cec_parent);
		}
		if (clk_prepare_enable(g_hdmi_drv->aw_clock.clk_cec) != 0)
			hdmi_err("%s: hdmi cec clk enable failed!\n", __func__);
	}
}

static void _aw_hdmi_cec_drv_clock_disable(void)
{
	if (g_hdmi_drv->aw_clock.clk_cec != NULL)
		clk_disable_unprepare(g_hdmi_drv->aw_clock.clk_cec);
}

static void _aw_hdmi_cec_drv_pin_config(void)
{
	s32 ret = 0;
	struct pinctrl_state *state;

	if (!IS_ERR(g_hdmi_drv->hdmi_pctl)) {
		state = pinctrl_lookup_state(g_hdmi_drv->hdmi_pctl, AW_CEC_PIN_ACTIVE);
		if (IS_ERR(state)) {
			hdmi_err("%s: pinctrl_lookup_state for HDMI2.0 CEC active fail\n",
					__func__);
			return;
		}

		ret = pinctrl_select_state(g_hdmi_drv->hdmi_pctl, state);
		if (ret < 0) {
			hdmi_err("%s: pinctrl_select_state for HDMI2.0 CEC active fail\n",
					__func__);
			return;
		}
	}
}

static void _aw_hdmi_cec_drv_pin_release(void)
{
	s32 ret = 0;
	struct pinctrl_state *state;

	if (!IS_ERR(g_hdmi_drv->hdmi_pctl)) {
		state = pinctrl_lookup_state(g_hdmi_drv->hdmi_pctl, AW_CEC_PIN_SLEEP);
		if (IS_ERR(state)) {
			hdmi_err("%s: pinctrl_lookup_state for HDMI2.0 CEC SLEEP fail\n",
					__func__);
			return;
		}

		ret = pinctrl_select_state(g_hdmi_drv->hdmi_pctl, state);
		if (ret < 0) {
			hdmi_err("%s: pinctrl_select_state for HDMI2.0 CEC SLEEP fail\n",
					__func__);
			return;
		}
	}
}

static void _aw_hdmi_cec_drv_resouce_config_for_resume(void)
{
	if (!bHdmi_cec_enable)
		return ;
	if (g_hdmi_drv->aw_dts.cec_super_standby)
		return ;

	LOG_TRACE();

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec);
	_aw_hdmi_cec_drv_clock_enable();
	_aw_hdmi_cec_drv_pin_config();
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec);
}

static void _aw_hdmi_cec_drv_resouce_release_for_suspend(void)
{
	if (!bHdmi_cec_enable)
		return ;
	if (g_hdmi_drv->aw_dts.cec_super_standby)
		return ;

	LOG_TRACE();

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec);
	_aw_hdmi_cec_drv_pin_release();
	_aw_hdmi_cec_drv_clock_disable();
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec);
}

static int _aw_hdmi_cec_enable(struct cec_adapter *adap, bool enable)
{
	int ret = 0;

	LOG_TRACE1(enable);

	if (enable) {
		mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec);
		_aw_hdmi_cec_drv_clock_enable();
		_aw_hdmi_cec_drv_pin_config();
		g_hdmi_drv->hdmi_core->cec_ops.enable();
		bHdmi_cec_enable = true;
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec);
	} else {
		mutex_lock(&g_hdmi_drv->aw_mutex.lock_cec);
		g_hdmi_drv->hdmi_core->cec_ops.disable();
		_aw_hdmi_cec_drv_pin_release();
		_aw_hdmi_cec_drv_clock_disable();
		bHdmi_cec_enable = false;
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_cec);
	}

	return ret;
}

static int _aw_hdmi_cec_log_addr(struct cec_adapter *adap, u8 logical_addr)
{
	struct aw_hdmi_cec *cec = cec_get_drvdata(adap);

	if (logical_addr == CEC_LOG_ADDR_INVALID)
		cec->logical_addr = 0;
	else
		cec->logical_addr |= BIT(logical_addr);

	g_hdmi_drv->hdmi_core->cec_ops.set_la(cec->logical_addr);

	return 0;
}

static int _aw_hdmi_cec_transmit(struct cec_adapter *adap, u8 attempts,
				u32 signal_free_time, struct cec_msg *msg)
{
	unsigned int frame_type;

	switch (signal_free_time) {
	case CEC_SIGNAL_FREE_TIME_RETRY:
		frame_type = AW_HDMI_CEC_FRAME_TYPE_RETRY;

		/* The reason for failed retry may be that other devices are sending,
		 * so you need to give some time to receive before sending.
		 */
		msleep(50);
		break;
	case CEC_SIGNAL_FREE_TIME_NEW_INITIATOR:
	default:
		frame_type = AW_HDMI_CEC_FRAME_TYPE_NORMAL;
		break;
	case CEC_SIGNAL_FREE_TIME_NEXT_XFER:
		frame_type = AW_HDMI_CEC_FRAME_TYPE_IMMED;
		break;
	}

	g_hdmi_drv->hdmi_core->cec_ops.send(msg->msg, msg->len, frame_type);

	return 0;
}

static irqreturn_t _aw_hdmi_cec_irq_handler(int irq, void *data)
{
	struct cec_adapter *adap = data;
	struct aw_hdmi_cec *cec = cec_get_drvdata(adap);

	irqreturn_t ret = IRQ_HANDLED;
	unsigned int stat = g_hdmi_drv->hdmi_core->cec_ops.get_ir_state();

	if (stat == 0)
		return IRQ_NONE;

	g_hdmi_drv->hdmi_core->cec_ops.clear_ir_state(stat);

	if (stat & AW_HDMI_CEC_STAT_ERROR_INIT) {
		cec->tx_status = CEC_TX_STATUS_ERROR;
		cec->tx_done = true;
		ret = IRQ_WAKE_THREAD;
	} else if (stat & AW_HDMI_CEC_STAT_DONE) {
		cec->tx_status = CEC_TX_STATUS_OK;
		cec->tx_done = true;
		ret = IRQ_WAKE_THREAD;
	} else if (stat & AW_HDMI_CEC_STAT_NACK) {
		cec->tx_status = CEC_TX_STATUS_NACK;
		cec->tx_done = true;
		ret = IRQ_WAKE_THREAD;
	}

	if (stat & AW_HDMI_CEC_STAT_EOM) {
		unsigned int len;

		g_hdmi_drv->hdmi_core->cec_ops.receive(cec->rx_msg.msg, &len);

		cec->rx_msg.len = len;
		smp_wmb();
		cec->rx_done = true;

		ret = IRQ_WAKE_THREAD;
	}

	return ret;
}

static irqreturn_t _aw_hdmi_cec_irq_thread(int irq, void *data)
{
	struct cec_adapter *adap = data;
	struct aw_hdmi_cec *cec = cec_get_drvdata(adap);

	if (cec->tx_done) {
		cec->tx_done = false;
		cec_transmit_attempt_done(adap, cec->tx_status);
	}
	if (cec->rx_done) {
		cec->rx_done = false;
		smp_rmb();
		cec_received_msg(adap, &cec->rx_msg);
	}
	return IRQ_HANDLED;
}

static const struct cec_adap_ops aw_hdmi_cec_ops = {
	.adap_enable   = _aw_hdmi_cec_enable,
	.adap_log_addr = _aw_hdmi_cec_log_addr,
	.adap_transmit = _aw_hdmi_cec_transmit,
};

static void _aw_hdmi_cec_del(void *data)
{
	struct aw_hdmi_cec *cec = data;
	cec_delete_adapter(cec->adap);
}

int aw_hdmi_cec_probe(struct platform_device *pdev)
{
	struct aw_hdmi_cec *cec;
	int ret;

	hdmi_inf("%s: start!!!\n", __func__);

	cec = devm_kzalloc(&pdev->dev, sizeof(*cec), GFP_KERNEL);
	if (!cec) {
		hdmi_err("%s: cec kzalloc failed!\n", __func__);
		return -ENOMEM;
	}

	cec->dev = &pdev->dev;
	bHdmi_cec_enable = false;

	platform_set_drvdata(pdev, cec);

	cec->adap = cec_allocate_adapter(&aw_hdmi_cec_ops, cec, "aw_hdmi",
					 CEC_CAP_DEFAULTS |
					 CEC_CAP_CONNECTOR_INFO,
					 CEC_MAX_LOG_ADDRS);
	if (IS_ERR(cec->adap)) {
		hdmi_err("%s: cec_allocate_adapter error!\n", __func__);
		return PTR_ERR(cec->adap);
	}

	cec->adap->owner = THIS_MODULE;

	ret = devm_add_action(&pdev->dev, _aw_hdmi_cec_del, cec);
	if (ret) {
		hdmi_err("%s: devm_add_action error! ret: %d\n", __func__, ret);
		cec_delete_adapter(cec->adap);
		return ret;
	}

	g_hdmi_drv->cec_notifier = cec_notifier_cec_adap_register(pdev->dev.parent,
			NULL, cec->adap);
	if (!g_hdmi_drv->cec_notifier) {
		hdmi_err("%s: cec_notifier_cec_adap_register error!\n", __func__);
		return -ENOMEM;
	}

	ret = cec_register_adapter(cec->adap, pdev->dev.parent);
	if (ret < 0) {
		hdmi_err("%s: cec_register_adapter error! ret: %d\n", __func__, ret);
		cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
		return ret;
	}

	ret = devm_request_threaded_irq(&pdev->dev, g_hdmi_drv->aw_dts.irq,
					_aw_hdmi_cec_irq_handler,
					_aw_hdmi_cec_irq_thread,
					IRQF_SHARED,
					"hdmi-cec", cec->adap);
	if (ret < 0) {
		hdmi_err("%s: devm_request_threaded_irq error! ret: %d\n", __func__, ret);
		return ret;
	}

	devm_remove_action(&pdev->dev, _aw_hdmi_cec_del, cec);

	hdmi_inf("%s finish!\n", __func__);

	return 0;
}

int aw_hdmi_cec_remove(struct platform_device *pdev)
{
	struct aw_hdmi_cec *cec = platform_get_drvdata(pdev);

	cec_notifier_cec_adap_unregister(cec->notify, cec->adap);
	cec_unregister_adapter(cec->adap);
	devm_kfree(&pdev->dev, cec);

	return 0;
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
int aw_hdmi_cec_suspend(struct device *dev)
{
	int i;

	hdmi_inf("%s start.\n", __func__);

	if (bHdmi_cec_suspend_enable) {
		hdmi_wrn("hdmi cec has been suspend!\n");
		return 0;
	}

	_aw_hdmi_cec_drv_resouce_release_for_suspend();

	if (g_hdmi_drv->aw_dts.cec_support
			&& g_hdmi_drv->aw_dts.cec_super_standby) {
		/* enable_wakeup_src(CPUS_HDMICEC_SRC, 0);
		   scene_lock(&hdmi_standby_lock); */
	}

	/* the suspend function that run last is responsible for releasing resources */
	if (bHdmi_suspend_enable) {
		hdmi_inf("release resources in %s\n", __func__);

		_aw_hdmi_drv_resource_release_for_suspend();

		for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++)
			_aw_hdmi_drv_power_disable(g_hdmi_drv->aw_power.power_regu[i]);

		_aw_hdmi_set_tcon_pad(false);
		disp_hdmi_clk_disable();

		if (!(aw_hdmi_drv_get_hpd_mask() & 0x100))
			_aw_hdmi_drv_dfs_set_state_sync(0);
	}

	bHdmi_cec_suspend_enable = true;
	hdmi_inf("%s finish.\n", __func__);
	return 0;
}

int aw_hdmi_cec_resume(struct device *dev)
{
	int i;

	hdmi_inf("%s start.\n", __func__);

	mutex_lock(&g_hdmi_drv->aw_mutex.lock_ctrl);
	if (!bHdmi_cec_suspend_enable) {
		mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);
		hdmi_wrn("hdmi cec has been resume!\n");
		return 0;
	}

	if (g_hdmi_drv->aw_dts.cec_support && g_hdmi_drv->aw_dts.cec_super_standby) {
		/* scene_unlock(&hdmi_standby_lock);
		disable_wakeup_src(CPUS_HDMICEC_SRC, 0); */
	}

	/* the resume function that run first is responsible for configure resources */
	if (bHdmi_suspend_enable) {
		_aw_hdmi_set_tcon_pad(true);

		for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++)
			_aw_hdmi_drv_power_enable(g_hdmi_drv->aw_power.power_regu[i]);

		_aw_hdmi_drv_resource_config_for_resume();

		_aw_hdmi_drv_clock_reset();
	}

	_aw_hdmi_cec_drv_resouce_config_for_resume();

	bHdmi_cec_suspend_enable = false;
	mutex_unlock(&g_hdmi_drv->aw_mutex.lock_ctrl);

	hdmi_inf("%s finish.\n", __func__);
	return 0;
}
#endif

static int _aw_hdmi_cec_init(void)
{
	struct platform_device_info pdevinfo;

	hdmi_inf("%s start!\n", __func__);

	if (!g_hdmi_drv->aw_dts.cec_support) {
		hdmi_wrn("cec not set support in dts!\n");
		return 0;
	}

	memset(&pdevinfo, 0, sizeof(pdevinfo));
	pdevinfo.parent = g_hdmi_drv->parent_dev;
	pdevinfo.id = PLATFORM_DEVID_AUTO;
	pdevinfo.name = "aw-hdmi-cec";
	pdevinfo.dma_mask = 0;

	g_hdmi_drv->cec = platform_device_register_full(&pdevinfo);

	hdmi_inf("%s finish!\n", __func__);
	return 0;
}

static void _aw_hdmi_cec_exit(void)
{
	platform_device_unregister(g_hdmi_drv->cec);
}
#endif /* CONFIG_AW_HDMI2_CEC_SUNXI */

int aw_hdmi_drv_probe(struct platform_device *pdev)
{
	int ret = 0;
	int phy_model = 301;
	dw_hdcp_param_t hdcp;

#if IS_ENABLED(CONFIG_AW_HDMI2_LOG_BUFFER)
	aw_hdmi_log_init(pdev->dev.of_node);
#endif

	hdmi_inf("%s: start.\n", __func__);

	if (g_hdmi_drv) {
		kfree(g_hdmi_drv);
		g_hdmi_drv = NULL;
	}

	g_hdmi_drv = kzalloc(sizeof(struct aw_hdmi_driver), GFP_KERNEL);
	if (!g_hdmi_drv) {
		hdmi_err("%s: aw hdmi alloc hdmi drv memory failed!!!\n", __func__);
		return -1;
	}

	g_hdmi_drv->hdmi_core = kzalloc(sizeof(struct aw_hdmi_core_s), GFP_KERNEL);
	if (!g_hdmi_drv->hdmi_core) {
		hdmi_err("%s: aw hdmi alloc hdmi core memory failed!!!\n", __func__);
		goto free_mem;
	}

	g_hdmi_drv->pdev = pdev;
	g_hdmi_drv->parent_dev = &pdev->dev;

	/* 1. base global value init */
	_aw_hdmi_init_global_value();

	if (_aw_hdmi_init_mutex_lock() != 0)
		goto free_mem;

	/* 2. base use reosurce init and malloc */
	if (_aw_hdmi_init_resoure(pdev, &hdcp) != 0)
		goto free_mem;

	g_hdmi_drv->hdmi_core->acs_ops.read  = aw_hdmi_drv_read;
	g_hdmi_drv->hdmi_core->acs_ops.write = aw_hdmi_drv_write;
	g_hdmi_drv->hdmi_core->reg_base = g_hdmi_drv->reg_base;

#if IS_ENABLED(CONFIG_AW_HDMI2_HDCP_SUNXI)
	_aw_hdmi_drv_hdcp_init(pdev, &hdcp);
#endif

	if (_aw_hdmi_init_notify_node() != 0)
		goto free_mem;

	_aw_hdmi_init_boot_mode();

	/* Init hdmi core and core params */
	ret = aw_hdmi_core_init(g_hdmi_drv->hdmi_core, phy_model, &hdcp);
	if (ret) {
		hdmi_err("%s: aw hdmi core init failed!!!\n", __func__);
		goto free_mem;
	}

	if (!bHdmi_boot_enable)
		g_hdmi_drv->hdmi_core->phy_ops.set_hpd(true);

	_aw_hdmi_set_tcon_pad(true);

	/* Now that everything is fine, let's add it to device list */
	list_add_tail(&g_hdmi_drv->devlist, &devlist_global);

	/* if (g_hdmi_drv->aw_dts.cec_super_standby)
		scene_lock_init(&hdmi_standby_lock, SCENE_HDMI_CEC_STANDBY, "hdmi_cec_standby"); */

	ret = _aw_hdmi_init_thread();
	if (ret != 0) {
		hdmi_err("%s: aw hdmi initial thread failed!!!\n", __func__);
		goto free_mem;
	}

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	g_hdmi_drv->aw_dts.irq = platform_get_irq(pdev, 0);
	if (g_hdmi_drv->aw_dts.irq < 0) {
		hdmi_err("%s: platform_get_irq error!\n", __func__);
	} else {
		ret = _aw_hdmi_cec_init();
		if (ret != 0) {
			hdmi_err("%s: aw hdmi initial cec failed!!!\n", __func__);
			goto free_mem;
		}
	}
#endif

	_aw_hdmi_init_disp_func();

	hdmi_inf("%s: finish.\n", __func__);
	return ret;

free_mem:
	kfree(g_hdmi_drv->hdmi_core);
	kfree(g_hdmi_drv);
	hdmi_err("%s: check log and free core and drv memory!!!\n", __func__);
	return -1;
}

int aw_hdmi_drv_remove(struct platform_device *pdev)
{
	struct aw_hdmi_driver *dev;
	struct list_head *list;
	int i = 0;

#if IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	_aw_hdmi_cec_exit();
#endif

	while (!list_empty(&devlist_global)) {
		list = devlist_global.next;
		list_del(list);
		dev = list_entry(list, struct aw_hdmi_driver, devlist);

		if (dev == NULL)
			continue;
	}

	_aw_hdmi_set_tcon_pad(false);

	_aw_hpd_thread_exit();

	/* scene_lock_destroy(&hdmi_standby_lock); */
	_aw_hdmi_drv_resource_release();

	for (i = 0; i < g_hdmi_drv->aw_power.power_count; i++) {
		if (g_hdmi_drv->aw_power.power_regu[i]) {
			_aw_hdmi_drv_power_disable(g_hdmi_drv->aw_power.power_regu[i]);
			regulator_put(g_hdmi_drv->aw_power.power_regu[i]);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(snd_hdmi_get_func);
