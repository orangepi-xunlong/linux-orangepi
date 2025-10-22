/* SPDX-License-Identifier: GPL-2.0-or-later*/
/* Copyright(c) 2024 - 2027 Allwinner Technology Co.Ltd. All rights reserved.*/
/*
 * Driver for sunxi SD/MMC host controllers
* (C) Copyright 2024-2026 lixiang <lixiang@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 18, 0))
#include "../../../drivers/ufs/host/ufshcd-pltfrm.h"
#else
#include "../../../drivers/scsi/ufs/ufshcd-pltfrm.h"
#endif
#include "ufshcd-sunxi.h"
#include "tc-dwc.h"
#include "ufshci-sunxi.h"
#include "sunxi_ufs_unipro.h"
#include "sunxi-ufs.h"
#include "sunxi-sid.h"

#define SUNXI_UFS_DRIVER_VESION "0.0.16 2025.1.20 10:40"
//#define PHY_DEBUG_DUMP
//#define CCU_DBG
#define SUNXI_UFS_AXI_CLK		(200*1000*1000)
#define SUNXI_UFS_CAL_WORDS_EFUSE_ALIGN_LOW         (0x60)
#define SUNXI_UFS_CAL_WORDS_EFUSE_ALIGN_HIGH        (0x64)


#define SUNXI_UFS_CARD_INT_BIT  (0x7 << 29)

static int ufs_sunxi_common_init(struct ufs_hba *hba);
static int sunxi_ufs_host_top_init(struct ufs_hba *hba);
static int sunxi_ufs_sys_clk_deinit(struct ufs_hba *hba);
static int sunxi_ufs_sys_clk_init(struct ufs_hba *hba);

static inline int sunxi_ufs_dump_ccu_reg(void)
{
#ifdef CCU_DBG
	void __iomem *v_addr =  NULL;
	int i = 0;
	u32 phy_addr = 0x2002d80;
	v_addr = ioremap(phy_addr, 24);
	if (v_addr == NULL) {
		pr_err("Can not map addr %x resource\n", (unsigned int)phy_addr);
		return -EIO;
	}

	printk("ccu dump start\n");
	for (i = 0; i < 24; i += 4) {
		printk("addr %x, val %x\n", (unsigned int)(phy_addr + i), readl(v_addr + i));
	}

	iounmap(v_addr);
	phy_addr = 0x7090000 +0x160;
	v_addr = ioremap(phy_addr, 24);
	for (i = 0; i < 24; i += 4) {
		printk("addr %x, val %x\n", (unsigned int)(phy_addr + i), readl(v_addr + i));
	}
	iounmap(v_addr);

	return 0;
#else
	return 0;
#endif
}
static inline int sunxi_ufs_get_cal_words(struct ufs_hba *hba, u32 *pll_rate_a, u32 *pll_rate_b, \
										u32 *att_lane0, u32 *ctle_lane0, \
										u32 *att_lane1, u32 *ctle_lane1)
{
	u32 rval_l  = 0;
	u32 rval_h  = 0;
	int ret = 0;

	dev_dbg(hba->dev, "Get ufs_cal_word_l\n");

	ret = sunxi_get_module_param_from_sid(&rval_l, SUNXI_UFS_CAL_WORDS_EFUSE_ALIGN_LOW, 4);
	if (ret) {
		dev_err(hba->dev, "Get ufs_cal_word_l failed\n");
		return -1;
	}

	dev_dbg(hba->dev, "Get ufs_cal_word_h\n");
	ret = sunxi_get_module_param_from_sid(&rval_h, SUNXI_UFS_CAL_WORDS_EFUSE_ALIGN_HIGH, 4);
	if (ret) {
		dev_err(hba->dev, "Get ufs_cal_word_h failed\n");
		return -1;
	}

	dev_info(hba->dev, "Cal words efuse addr 0x%x value 0x%08x, addr 0x%x value 0x%08x\n",\
						SUNXI_UFS_CAL_WORDS_EFUSE_ALIGN_LOW, rval_l,\
						SUNXI_UFS_CAL_WORDS_EFUSE_ALIGN_HIGH, rval_h);
	*pll_rate_a = (rval_h >> 16) & 0xff;
	*pll_rate_b = (rval_h >> 24) & 0xff;

	if (*pll_rate_a && *pll_rate_b) {
		if ((*pll_rate_a < 0x14) \
			|| (*pll_rate_a > 0x4b)\
			|| (*pll_rate_b < 0x4b)\
			|| (*pll_rate_b > 0x73)) {
			dev_err(hba->dev, "pll cal words over valid range"
					"pll rate a 0x%08x, pll rate b 0x%08x\n", *pll_rate_a, *pll_rate_b);

			if ((*pll_rate_b < 0x4b) ||\
				(*pll_rate_b > 0x73))
				*pll_rate_b = 0;
			if ((*pll_rate_a < 0x14) \
				|| (*pll_rate_a > 0x4b))\
				*pll_rate_a = 0;
		}
	}


	*att_lane0 = (rval_l >> 16) & 0xff;
	*ctle_lane0 = (rval_l >> 24) & 0xff;
	if ((*att_lane0) \
		&& (*ctle_lane0)) {
		if ((*att_lane0 == 0) \
			|| (*att_lane0 == 255)\
			|| (*ctle_lane0 == 0)\
			|| (*ctle_lane0 == 255)) {
			dev_err(hba->dev, "afe cal words over valid range"
					"att lane0 0x%08x, ctle_lane0 0x%08x\n",
					*att_lane0, *ctle_lane0);
			return -1;
		}
	}

	*att_lane1 = (rval_h >> 0) & 0xff;
	*ctle_lane1 = (rval_h >> 8) & 0xff;
	if ((*att_lane1) \
		&& (*ctle_lane1)) {
		if ((*att_lane1 == 0) \
			|| (*att_lane1 == 255)\
			|| (*ctle_lane1 == 0)\
			|| (*ctle_lane1 == 255)) {
			dev_err(hba->dev, "afe cal words over valid range"
					"att lane1 0x%08x, ctle_lane1 0x%08x\n",
					*att_lane1, *ctle_lane1);
			return -1;
		}
	}

	dev_dbg(hba->dev, "pll rate a 0x%08x, pll rate b 0x%08x\n",\
						*pll_rate_a, *pll_rate_b);
	dev_dbg(hba->dev, "att lane0 0x%08x, ctle_lane0 0x%08x\n",\
						*att_lane0, *ctle_lane0);
	dev_dbg(hba->dev, "att lane1 0x%08x, ctle_lane1 0x%08x\n",
						*att_lane1, *ctle_lane1);

	if (!(*pll_rate_a) && !(*pll_rate_b)\
		&& !(*att_lane0) && !(*ctle_lane0)\
		&& !(*att_lane1) && !(*ctle_lane1)) {
		dev_info(hba->dev, "Phy PLL Use auto mode and default afe value\n");
	}

	return 0;
}

/**
 * ufshcd_wait_for_register - wait for register value to change
 * @hba: per-adapter interface
 * @reg: mmio register offset
 * @mask: mask to apply to the read register value
 * @val: value to wait for
 * @interval_us: polling interval in microseconds
 * @timeout_ms: timeout in milliseconds
 *
 * Return: -ETIMEDOUT on error, zero on success.
 */
static int sunxi_ufshcd_wait_for_register(struct ufs_hba *hba, u32 reg, u32 mask,
				u32 val, unsigned long interval_us,
				unsigned long timeout_ms)
{
	int err = 0;
	unsigned long timeout = jiffies + msecs_to_jiffies(timeout_ms);

	/* ignore bits that we don't intend to wait on */
	val = val & mask;

	while ((ufshcd_readl(hba, reg) & mask) != val) {
		usleep_range(interval_us, interval_us + 50);
		if (time_after(jiffies, timeout)) {
			if ((ufshcd_readl(hba, reg) & mask) != val)
				err = -ETIMEDOUT;
			break;
		}
	}

	return err;
}

/**
 * ufshcd_sunxi_link_is_up() - check if link is up.
 * @hba: private structure pointer
 *
 * Return: 0 on success, non-zero value on failure.
 */
static int ufshcd_sunxi_link_is_up(struct ufs_hba *hba)
{
	int dme_result = 0;

	ufshcd_dme_get(hba, UIC_ARG_MIB(VS_POWERSTATE), &dme_result);

	if (dme_result == UFSHCD_LINK_IS_UP) {
		ufshcd_set_link_active(hba);
		return 0;
	}

	return 1;
}

int ufshcd_sunxi_dme_set_attrs(struct ufs_hba *hba,
				const struct ufshcd_dme_attr_val *v, int n)
{
	int ret = 0;
	int attr_node = 0;

	for (attr_node = 0; attr_node < n; attr_node++) {
		ret = ufshcd_dme_set_attr(hba, v[attr_node].attr_sel,
			ATTR_SET_NOR, v[attr_node].mib_val, v[attr_node].peer);
		if (ret)
			return ret;
	}

	return 0;
}

int ufshcd_sunxi_dme_dump_attrs(struct ufs_hba *hba,
				const struct ufshcd_dme_attr_val *v, int n)
{
	int ret = 0;
	int attr_node = 0;
	u32 value = 0;

	for (attr_node = 0; attr_node < n; attr_node++) {
		ret = ufshcd_dme_get_attr(hba, v[attr_node].attr_sel,
			&value, v[attr_node].peer);
		if (ret)
			return ret;
		dev_err(hba->dev, "dme attr sel 0x%08x, value 0x%08x\n",\
					v[attr_node].attr_sel, value);
	}

	return 0;
}

/**
 * tc_sunxi_g240_c10_read()
 *
 * This function reads from the c10 interface in Synopsys G240 TC
 * @hba: Pointer to driver structure
 * @c10_reg: c10 register to read from
 * @c10_val: c10 value read
 *
 * Returns 0 on success or non-zero value on failure
 */
int tc_sunxi_cr_read(struct ufs_hba *hba, u16 reg, u16 *val)
{
	struct ufshcd_dme_attr_val data[] = {
		{ UIC_ARG_MIB(TC_CBC_REG_ADDR_LSB), TC_LSB(0), DME_LOCAL },
		{ UIC_ARG_MIB(TC_CBC_REG_ADDR_MSB), TC_MSB(0), DME_LOCAL },
/*		{ UIC_ARG_MIB(TC_CBC_REG_WR_LSB), 0xff, DME_LOCAL },
		{ UIC_ARG_MIB(TC_CBC_REG_WR_MSB), 0xff, DME_LOCAL },
*/
		{ UIC_ARG_MIB(TC_CBC_REG_RD_WR_SEL), 0x0, DME_LOCAL },
		{ UIC_ARG_MIB(VS_MPHYCFGUPDT), 0x1, DME_LOCAL },
	};
	u32 tmp_rd_lsb = 0;
	u32 tmp_rd_msb = 0;
	int ret = 0;

	data[0].mib_val = TC_LSB(reg);
	data[1].mib_val = TC_MSB(reg);


	ret = ufshcd_sunxi_dme_set_attrs(hba, data, ARRAY_SIZE(data));

/*	ret |= ufshcd_dme_set_attr(hba, UIC_ARG_MIB(TC_CBC_REG_RD_LSB), 0x00,
					0x00, DME_LOCAL);*/
	ret |= ufshcd_dme_get_attr(hba, UIC_ARG_MIB(TC_CBC_REG_RD_LSB),
					&tmp_rd_lsb, DME_LOCAL);
/*	ret |= ufshcd_dme_set_attr(hba, UIC_ARG_MIB(TC_CBC_REG_RD_MSB), 0x00,
					0x00, DME_LOCAL);*/
	ret |= ufshcd_dme_get_attr(hba, UIC_ARG_MIB(TC_CBC_REG_RD_MSB),
					&tmp_rd_msb, DME_LOCAL);

	if (ret)
		return ret;

	*val = (tmp_rd_msb << 8) | tmp_rd_lsb;

	return ret;
}


/**
 * tc_sunxi_c10_write()
 *
 * This function writes to the c10 interface in Synopsys G240 TC
 * @hba: Pointer to driver structure
 * @c10_reg: c10 register to write into
 * @c10_val: c10 value to be written
 *
 * Returns 0 on success or non-zero value on failure
 */
int tc_sunxi_cr_write(struct ufs_hba *hba, u16 reg, u16 val)
{
	struct ufshcd_dme_attr_val data[] = {
		{ UIC_ARG_MIB(TC_CBC_REG_ADDR_LSB), REG_16_LSB(0),
			DME_LOCAL },
		{ UIC_ARG_MIB(TC_CBC_REG_ADDR_MSB), REG_16_MSB(0),
			DME_LOCAL },
		{ UIC_ARG_MIB(TC_CBC_REG_WR_LSB), REG_16_LSB(0), DME_LOCAL },
		{ UIC_ARG_MIB(TC_CBC_REG_WR_MSB), REG_16_MSB(0), DME_LOCAL },
		{ UIC_ARG_MIB(CBC_REG_RD_WR_SEL), CBC_REG_WRITE, DME_LOCAL },
		{ UIC_ARG_MIB(VS_MPHYCFGUPDT), 0x1, DME_LOCAL },
	};

	data[0].mib_val = REG_16_LSB(reg);
	data[1].mib_val = REG_16_MSB(reg);
	data[2].mib_val = REG_16_LSB(val);
	data[3].mib_val = REG_16_MSB(val);


	return ufshcd_sunxi_dme_set_attrs(hba, data, ARRAY_SIZE(data));
}



static int sunxi_ufs_rmmi_config(struct ufs_hba *hba)
{
	u16 phy_val = 0;
	int ret = 0;
	int i = 0;
	//u32 v = 0;
	u32 pll_rate_a = 0;
	u32 pll_rate_b = 0;
	u32 att_lane0 = 0;
	u32 ctle_lane0 = 0;
	u32 att_lane1 = 0;
	u32 ctle_lane1 = 0;
	struct ufs_sunxi_priv *priv = hba->priv;


	struct ufshcd_dme_attr_val rmmi_config[] = {
		/*Configure CB attribute CBRATESEL (0x8114) with value 0x0 for Rate A and 0x1 for Rate B*/
		{ UIC_ARG_MIB(CBRATSEL), 0x1, DME_LOCAL },
		/*Because RMMI_CBREFCLKCTRL2 bit 4 defualt value is 1,
		 * here bit 4 set to 1 too
		 *according to SNPS ip
		 * */
		{ UIC_ARG_MIB(RMMI_CBREFCLKCTRL2), 0x90, DME_LOCAL },
		{ UIC_ARG_MIB_SEL(RMMI_RXSQCONTROL, SELIND_LN0_RX), 0x01,
					DME_LOCAL },
		{ UIC_ARG_MIB_SEL(RMMI_RXSQCONTROL, SELIND_LN1_RX), 0x01,
					DME_LOCAL },
		{ UIC_ARG_MIB_SEL(RMMI_RXRHOLDCTRLOPT, SELIND_LN0_RX), 0x02,
					DME_LOCAL },
		{ UIC_ARG_MIB_SEL(RMMI_RXRHOLDCTRLOPT, SELIND_LN1_RX), 0x02,
					DME_LOCAL },
#if 1
		{ UIC_ARG_MIB(EXT_COARSE_TUNE_RATEA), 0x2,
					DME_LOCAL },/*reset value 0x2*/
		{ UIC_ARG_MIB(EXT_COARSE_TUNE_RATEB), 0x80,
					DME_LOCAL },/*reset value 0x80*/
#endif
		{ UIC_ARG_MIB(RMMI_CBCRCTRL), 0x01, DME_LOCAL },
		{ UIC_ARG_MIB(VS_MPHYCFGUPDT), 0x01, DME_LOCAL },
	};

	static const struct ufshcd_dme_attr_val rmmi_config_1[] = {
		{ UIC_ARG_MIB(VS_MPHYCFGUPDT), 0x1, DME_LOCAL },
		};


	struct pair_addr phy_cr_data_coarse_tune[] = {
		{RAWAONLANEN_DIG_MPLLA_COARSE_TUNE, 0x0},/*reg default value**/
		{MPLL_SKIPCAL_COARSE_TUNE, 0x0},/*reg default value**/
	};


	static const struct pair_addr phy_cr_data_afe_flags[] = {
		{RAWAONLANEN_DIG_FAST_FLAGS0, 0x0004},
		{RAWAONLANEN_DIG_FAST_FLAGS1, 0x0004},
	};

	static const struct pair_addr phy_cr_data_pll_auto_cal[] = {
		//	{SUP_DIG_MPLLA_MPLL_PWR_CTL_CAL_CTRL, 0x0001},//PLL calibration 4.10.2 step1 for Rate A
		{MPLL_PWR_CTL_CAL_CTRL, 0x0008},//PLL calibration 4.10.2 step1 for Rate A
	};


	struct pair_addr phy_cr_data_afe_cal_words[] = {
		{RAWAONLANEN_DIG_AFE_ATT_IDAC_OFST0, 0x80},/*reg default value**/
		{RAWAONLANEN_DIG_AFE_ATT_IDAC_OFST1, 0x80},/*reg default value**/
		{RAWAONLANEN_DIG_AFE_CTLE_IDAC_OFST0, 0x80},/*reg default value**/
		{RAWAONLANEN_DIG_AFE_CTLE_IDAC_OFST1, 0x80},/*reg default value**/
		{RAWAONLANEN_DIG_RX_ADPT_DFE_TAP3_0, 0xa00},
		{RAWAONLANEN_DIG_RX_ADPT_DFE_TAP3_1, 0xa00},
	};

	ret = sunxi_ufs_get_cal_words(hba, &pll_rate_a, &pll_rate_b, \
										&att_lane0, &ctle_lane0, \
										&att_lane1, &ctle_lane1);
	if (ret)
		return ret;

	/*Force use auto pll cal,for the value in efuse will cause uic error*/
	//pll_rate_a = pll_rate_b = 0;

	/*update ext coarse tune rate from efuse**/
	if (pll_rate_a || pll_rate_b) {
		if ((rmmi_config[6].attr_sel != UIC_ARG_MIB(EXT_COARSE_TUNE_RATEA))
		   && (rmmi_config[7].attr_sel != UIC_ARG_MIB(EXT_COARSE_TUNE_RATEB))) {
				dev_err(hba->dev, "ext coarse tune arrary setting failed\n");
				return -1;
		   }
		if (pll_rate_a)
			rmmi_config[6].mib_val = pll_rate_a;
		if (pll_rate_b)
			rmmi_config[7].mib_val = pll_rate_b;
		dev_dbg(hba->dev, "update ext coarse tune rate ok,"
				"rate a(0x%08x) 0x%08x, rate b(0x%08x) 0x%08x\n",\
				rmmi_config[6].attr_sel, rmmi_config[6].mib_val,\
				rmmi_config[7].attr_sel, rmmi_config[7].mib_val);

	}


	if (!pll_rate_b && pll_rate_a) {
		rmmi_config[0].mib_val = 0;//rate a
		priv->phy_hs_rate = PA_HS_MODE_A;
	}
/*
 *startup sequence_ufs_mphy_812a.docx step 8.b, step 9 to 10,13 to 14
 */
	ret = ufshcd_sunxi_dme_set_attrs(hba, rmmi_config,
				       ARRAY_SIZE(rmmi_config));
	if (ret) {
		dev_err(hba->dev, "set rmmi_config failed\n");
		return ret;
	}

#ifdef PHY_DEBUG_DUMP
	ufshcd_sunxi_dme_dump_attrs(hba, rmmi_config, ARRAY_SIZE(rmmi_config));
#endif


/*
 *15.De-assert.phy_reset
 */
#ifdef SUNXI_UFS_RAW_CCU_SETTING
	v = readl(SUNXI_UFS_BGR_REG);
	v |= SUNXI_UFS_PHY_RST_BIT;
	writel(v, SUNXI_UFS_BGR_REG);
#else
	{
		struct ufs_sunxi_priv *priv = NULL;
		priv = hba->priv;
		reset_control_deassert(priv->rst[SUNXI_UFS_PHY_RST].urst);
		dev_dbg(hba->dev, "phy rset deassert\n");
		//sunxi_ufs_dump_ccu_reg();

	}
#endif
	/**16.Wait for sram_init_done signal from MPHY.**/
	/*test_top.ufs_rtl.u_mphy_top.sram_init_done 1*/
	ret = sunxi_ufshcd_wait_for_register(hba, REG_UFS_CFG, MPHY_SRAM_INIT_DONE_BIT, MPHY_SRAM_INIT_DONE_BIT, 10, 10);
	if (ret) {
		dev_err(hba->dev, "%s: wait sram init done timeout\n", __func__);
		return ret;
	}

	/*17.Access SRAM contents through indirect register access if needed.**/
	/*test sram*/
	ret = tc_sunxi_cr_read(hba, RAWMEM_DIG_RAM_CMN0_B0_R0 + 2, &phy_val);
	if (ret) {
			dev_err(hba->dev,
				"Failed to read  RAWMEM_DIG_RAM_CMN0_B0_R0 + 2results\n");
			return ret;
	}

	ret = tc_sunxi_cr_write(hba, RAWMEM_DIG_RAM_CMN0_B0_R0 + 2, phy_val);
	if (ret) {
			dev_err(hba->dev,
				"Failed to write  RAWMEM_DIG_RAM_CMN0_B0_R0 +2 results\n");
			return ret;
	}




/*
 *startup sequence_ufs_mphy_812a.docx step 18,19
 */
	/*update coarse tune rate from efuse**/
	if (pll_rate_b || pll_rate_a) {
		phy_cr_data_coarse_tune[0].value = pll_rate_b?pll_rate_b:pll_rate_a;
		phy_cr_data_coarse_tune[1].value = pll_rate_b?pll_rate_b:pll_rate_a;
		dev_dbg(hba->dev, "update coarse tune for rate b ok,"
				"addr 0x%08x value 0x%08x,"
				"addr 0x%08x, value 0x%08x\n",\
				phy_cr_data_coarse_tune[0].addr, phy_cr_data_coarse_tune[0].value, \
				phy_cr_data_coarse_tune[1].addr, phy_cr_data_coarse_tune[1].value);

		for (i = 0; i < ARRAY_SIZE(phy_cr_data_coarse_tune); i++) {
			const struct pair_addr *data = &phy_cr_data_coarse_tune[i];
			ret = tc_sunxi_cr_write(hba, data->addr, data->value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 write failed\n", __func__);
				return ret;
			}
		}
#ifdef PHY_DEBUG_DUMP
		for (i = 0; i < ARRAY_SIZE(phy_cr_data_coarse_tune); i++) {
			const struct pair_addr *data = &phy_cr_data_coarse_tune[i];
			u16 value = 0;
			ret = tc_sunxi_cr_read(hba, data->addr, &value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 write failed\n", __func__);
				return ret;
			}
			dev_err(hba->dev, "phy cr addr 0x%08x, value 0x%08x\n", data->addr, value);
		}
#endif
	}


/*
 *startup sequence_ufs_mphy_812a.docx step 20
 */
	for (i = 0; i < ARRAY_SIZE(phy_cr_data_afe_flags); i++) {
		const struct pair_addr *data = &phy_cr_data_afe_flags[i];
		ret = tc_sunxi_cr_write(hba, data->addr, data->value);
		if (ret) {
			dev_err(hba->dev, "%s: c10 write failed\n", __func__);
			return ret;
		}
	}

	if (att_lane0 && ctle_lane0 && att_lane1 && ctle_lane1) {
		phy_cr_data_afe_cal_words[0].value = att_lane0;
		phy_cr_data_afe_cal_words[1].value = att_lane1;
		phy_cr_data_afe_cal_words[2].value = ctle_lane0;
		phy_cr_data_afe_cal_words[3].value = ctle_lane1;
		dev_dbg(hba->dev, "update pll cal words, att_lane0(0x%08x) 0x%x,att_lane1(0x%08x) 0x%08x,"
				"ctle_lane0(0x%08x) 0x%08x, ctle_lane1(0x%08x) 0x%08x\n", \
				phy_cr_data_afe_cal_words[0].addr, phy_cr_data_afe_cal_words[0].value,
				phy_cr_data_afe_cal_words[1].addr, phy_cr_data_afe_cal_words[1].value,\
				phy_cr_data_afe_cal_words[2].addr, phy_cr_data_afe_cal_words[2].value,\
				phy_cr_data_afe_cal_words[3].addr, phy_cr_data_afe_cal_words[3].value);

		/*
		*startup sequence_ufs_mphy_812a.docx step 21,22,23
		 */
		for (i = 0; i < ARRAY_SIZE(phy_cr_data_afe_cal_words); i++) {
			const struct pair_addr *data = &phy_cr_data_afe_cal_words[i];
			ret = tc_sunxi_cr_write(hba, data->addr, data->value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 write failed\n", __func__);
				return ret;
			}
		}

#ifdef PHY_DEBUG_DUMP
		for (i = 0; i < ARRAY_SIZE(phy_cr_data_afe_cal_words); i++) {
			const struct pair_addr *data = &phy_cr_data_afe_cal_words[i];
			u16 value = 0;
			ret = tc_sunxi_cr_read(hba, data->addr, &value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 read failed\n", __func__);
				return ret;
			}
			dev_err(hba->dev, "phy cr addr 0x%08x, value 0x%08x\n", data->addr, value);
		}

		for (i = 0; i < ARRAY_SIZE(phy_cr_data_pll_auto_cal); i++) {
			const struct pair_addr *data = &phy_cr_data_pll_auto_cal[i];
			u16 value = 0;
			ret = tc_sunxi_cr_read(hba, data->addr, &value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 read failed\n", __func__);
				return ret;
			}
			dev_err(hba->dev, "phy cr addr 0x%08x, value 0x%08x\n", data->addr, value);
		}

#endif

	}
	if (!pll_rate_b && !pll_rate_a) {
		/*use auto pll cal**/
		for (i = 0; i < ARRAY_SIZE(phy_cr_data_pll_auto_cal); i++) {
			const struct pair_addr *data = &phy_cr_data_pll_auto_cal[i];
			ret = tc_sunxi_cr_write(hba, data->addr, data->value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 write failed\n", __func__);
				return ret;
			}
		}
		dev_info(hba->dev, "auto pll cal\n");

#ifdef PHY_DEBUG_DUMP
		for (i = 0; i < ARRAY_SIZE(phy_cr_data_pll_auto_cal); i++) {
			const struct pair_addr *data = &phy_cr_data_pll_auto_cal[i];
			u16 value = 0;
			ret = tc_sunxi_cr_read(hba, data->addr, &value);
			if (ret) {
				dev_err(hba->dev, "%s: c10 read failed\n", __func__);
				return ret;
			}
			dev_err(hba->dev, "phy cr addr 0x%08x, value 0x%08x\n", data->addr, value);
		}
#endif
	}



	/**24.Trigger FW execution start according to FW usage scenario, for detailed
	 * information see the (PHY databook) "Firmware Usage Scenario"**/
	/*force test_top.ufs_rtl.u_mphy_top.sram_ext_ld_done*/
	ufshcd_writel(hba, ufshcd_readl(hba, REG_UFS_CFG) | MPHY_SRAM_EXT_LD_DONE_BIT, REG_UFS_CFG);

/*
 * 25.Configure the VS_MphyCfgUpdt (0xD085) attribute to 1. The Write access
 * to this attribute triggers a configuration update of all connected TX
 * and RX M-PHY modules. It returns 0 upon read.
 *
*/
	ret = ufshcd_sunxi_dme_set_attrs(hba, rmmi_config_1,
				       ARRAY_SIZE(rmmi_config_1));
	if (ret) {
		dev_err(hba->dev, "set rmmi_config1 failed\n");
		return ret;
	}


	return ret;
}


static int sunxi_ufs_phy_config(struct ufs_hba *hba)
{
	int ret = 0;
#ifdef USE_UNUSED_CODE
	/*default value is 1,no need to set ,here only for safe*/
	ret = ufshcd_dme_set(hba, UIC_ARG_MIB(VS_MPHY_DISABLE), 0x01);
	if (ret) {
		dev_err(hba->dev, "VS_MPHY_ENABLE failed\n");
		return ret;
	}
#endif
	ret = sunxi_ufs_rmmi_config(hba);
	if (ret) {
		dev_err(hba->dev, "Failed to config rmmi");
		return ret;
	}

/*
 *26.Configure VS_mphy_disable(0xD0C1) with the value 0x0
 *
 */
	ret = ufshcd_dme_set(hba, UIC_ARG_MIB(VS_MPHY_DISABLE), 0x00);
	if (ret) {
		dev_err(hba->dev, "VS_MPHY_DISABLE failed\n");
		return ret;
	}

/*
 *28.Configure VS_DebugSaveConfigTime(0xD0A0). Set 0xD0A0[1:0] = 0x3.
 *29.Configure VS_ClkMuxSwitchingTimer (0xD0FB) with the value 0xA.
 */
	ret = ufshcd_dme_set(hba, UIC_ARG_MIB(VS_DEBUG_SAVE_CONFIG_TIME), 0x1b);
	if (ret) {
		dev_err(hba->dev, "VS_DEBUG_SAVE_CONFIG_TIME failed\n");
		return ret;
	} else {
		dev_dbg(hba->dev, "%s:VS_DEBUG_SAVE_CONFIG_TIME 1b\n", __FUNCTION__);
	}

	ret = ufshcd_dme_set(hba, UIC_ARG_MIB(VS_CLK_MUX_SWITCHING_TIMER), 0xa);
	if (ret) {
		dev_err(hba->dev, "VS_CLK_MUX_SWITCHING_TIMER failed\n");
		return ret;
	}

	return ret;
}

/**
 * ufshcd_sunxi_connection_setup()
 * This function configures both the local side (host) and the peer side
 * (device) unipro attributes to establish the connection to application/
 * cport.
 * This function is not required if the hardware is properly configured to
 * have this connection setup on reset. But invoking this function does no
 * harm and should be fine even working with any ufs device.
 *
 * @hba: pointer to drivers private data
 *
 * Returns 0 on success non-zero value on failure
 */
static int ufshcd_sunxi_connection_setup(struct ufs_hba *hba)
{
	int ret = 0;
	u32 val = 0;
	static const struct ufshcd_dme_attr_val setup_attrs[] = {
		{ UIC_ARG_MIB(T_CONNECTIONSTATE), 0, DME_LOCAL },
/*
		{ UIC_ARG_MIB(N_DEVICEID), 0, DME_LOCAL },
		{ UIC_ARG_MIB(N_DEVICEID_VALID), 0, DME_LOCAL },
		{ UIC_ARG_MIB(T_PEERDEVICEID), 1, DME_LOCAL },
		{ UIC_ARG_MIB(T_PEERCPORTID), 0, DME_LOCAL },
		{ UIC_ARG_MIB(T_TRAFFICCLASS), 0, DME_LOCAL },
*/
		{ UIC_ARG_MIB(T_CPORTFLAGS), 0x6, DME_LOCAL },
/*
		{ UIC_ARG_MIB(T_CPORTMODE), 1, DME_LOCAL },
*/
		{ UIC_ARG_MIB(T_CONNECTIONSTATE), 1, DME_LOCAL },
/*
		{ UIC_ARG_MIB(T_CONNECTIONSTATE), 0, DME_PEER },
		{ UIC_ARG_MIB(N_DEVICEID), 1, DME_PEER },
		{ UIC_ARG_MIB(N_DEVICEID_VALID), 1, DME_PEER },
		{ UIC_ARG_MIB(T_PEERDEVICEID), 1, DME_PEER },
		{ UIC_ARG_MIB(T_PEERCPORTID), 0, DME_PEER },
		{ UIC_ARG_MIB(T_TRAFFICCLASS), 0, DME_PEER },
		{ UIC_ARG_MIB(T_CPORTFLAGS), 0x6, DME_PEER },
		{ UIC_ARG_MIB(T_CPORTMODE), 1, DME_PEER },
		{ UIC_ARG_MIB(T_CONNECTIONSTATE), 1, DME_PEER }
*/
	};

	ret = ufshcd_sunxi_dme_set_attrs(hba, setup_attrs, ARRAY_SIZE(setup_attrs));
	if (ret) {
		dev_err(hba->dev, "connection set up failed\n");
		return ret;
	}

	ret = ufshcd_dme_get_attr(hba, UIC_ARG_MIB(T_CONNECTIONSTATE),
					&val, DME_LOCAL);
	if (ret) {
		dev_err(hba->dev, "read  T_CONNECTIONSTATE)failed\n");
		return ret;
	}
	if (val != 1) {
		dev_err(hba->dev, "read  T_CONNECTIONSTATE not 1,unipro init failed\n");
		return ret;
	}

	return ret;
}




static int sunxi_ufs_link_startup_notify(struct ufs_hba *hba,
					enum ufs_notify_change_status status)
{
	int err = 0;
	switch (status) {
	case PRE_CHANGE:
		if (hba->vops->phy_initialization) {
			err = hba->vops->phy_initialization(hba);
			if (err) {
				dev_err(hba->dev, "Phy setup failed (%d)\n",
									err);
				goto out;
			}
		}
#ifdef USE_UNUSED_CODE
		if (hba->hwv_params.tc_rule == 2) {
			/* Necessary for HSG4 FPGA environment */
			err = ufshcd_dme_set_attr(hba,
						UIC_ARG_MIB(DL_AFC0CREDITTHRESHOLD),
						ATTR_SET_NOR, 0x6a, DME_LOCAL);
		}
#endif
		break;
	case POST_CHANGE:
		err = ufshcd_sunxi_link_is_up(hba);
		if (err) {
			dev_err(hba->dev, "Link is not up\n");
			goto out;
		}

		err = ufshcd_sunxi_connection_setup(hba);
		if (err)
			dev_err(hba->dev, "Connection setup failed (%d)\n",
									err);
		break;
	}

out:
	return err;
}


static int sunxi_ufs_pre_pwr_change(struct ufs_hba *hba,
				  struct ufs_pa_layer_attr *dev_max_params,
				  struct ufs_pa_layer_attr *dev_req_params)
{
	struct ufs_dev_params host_cap;
	int ret;
	struct ufs_sunxi_priv *priv = hba->priv;

	ufshcd_init_pwr_dev_param(&host_cap);
	host_cap.hs_rx_gear = UFS_HS_G4;
	host_cap.hs_tx_gear = UFS_HS_G4;
	host_cap.hs_rate = priv->phy_hs_rate;

	ret = ufshcd_get_pwr_dev_param(&host_cap,
				       dev_max_params,
				       dev_req_params);
	if (ret) {
		pr_info("%s: failed to determine capabilities\n",
			__func__);
		goto out;
	}


	ret = ufshcd_dme_configure_adapt(hba,
					   dev_req_params->gear_tx,
					   PA_INITIAL_ADAPT);
	if (ret) {
		pr_err("%s: failed to set PA INIT ADAPT\n",
			__func__);
	}
out:
	return ret;
}

static int sunxi_ufs_pwr_change_notify(struct ufs_hba *hba,
				     enum ufs_notify_change_status stage,
				     struct ufs_pa_layer_attr *dev_max_params,
				     struct ufs_pa_layer_attr *dev_req_params)
{
	int ret = 0;

	/*use pwr change notify to set system stanby state to ufs power down*/
	hba->spm_lvl = UFS_PM_LVL_5;
	dev_dbg(hba->dev, "pm lvl 5:ufs power down and link off\n");

	switch (stage) {
	case PRE_CHANGE:
		ret = sunxi_ufs_pre_pwr_change(hba, dev_max_params,
					     dev_req_params);
		break;
	case POST_CHANGE:
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

void sunxi_ufs_register_dump(struct ufs_hba *hba)
{
	sunxi_ufs_dump_ccu_reg();
}

void  sunxi_ufs_hibern8_notify(struct ufs_hba *hba, enum uic_cmd_dme cmd,
		enum ufs_notify_change_status status)
{
	if (status == PRE_CHANGE && cmd == UIC_CMD_DME_HIBER_ENTER) {
		dev_dbg(hba->dev, "hibern8 enter pre");
	//	ufshcd_dump_regs(hba, 0, UFSHCI_REG_SPACE_SIZE, "host_regs: ");
	}
	if (status == POST_CHANGE && cmd == UIC_CMD_DME_HIBER_ENTER) {
		dev_dbg(hba->dev, "hibern8 enter post");
	//	ufshcd_dump_regs(hba, 0, UFSHCI_REG_SPACE_SIZE, "host_regs: ");
	//	ufshcd_dump_regs(hba, 0x200, 64, "top host_regs: ");
	}

	if (status == PRE_CHANGE && cmd == UIC_CMD_DME_HIBER_EXIT) {
		dev_dbg(hba->dev, "hibern8 exit pre");
	//	ufshcd_dump_regs(hba, 0, UFSHCI_REG_SPACE_SIZE, "host_regs: ");
	//	ufshcd_dump_regs(hba, 0x200, 64, "top host_regs: ");

	}
	if (status == POST_CHANGE && cmd == UIC_CMD_DME_HIBER_EXIT) {
		dev_dbg(hba->dev, "hibern8 exit post");
	//	ufshcd_dump_regs(hba, 0, UFSHCI_REG_SPACE_SIZE, "host_regs: ");
	//	ufshcd_dump_regs(hba, 0x200, 64, "top host_regs: ");
	}
}

static inline struct scsi_device *sunxi_hba_to_wlun(struct ufs_hba *hba)
{
	struct scsi_device *sdp;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 19, 0))
	sdp = hba->ufs_device_wlun;
#else
	sdp = hba->sdev_ufs_device;
#endif
	return sdp;
}

#if defined(CONFIG_AW_KERNEL_ORIGIN)
int  sunxi_ufs_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret = 0;
	if (pm_op == UFS_SYSTEM_PM) {
		struct scsi_device *sdp;
		unsigned long flags;

		spin_lock_irqsave(hba->host->host_lock, flags);
		sdp = sunxi_hba_to_wlun(hba);
		if (sdp && scsi_device_online(sdp))
			ret = scsi_device_get(sdp);
		else
			ret = -ENODEV;
		spin_unlock_irqrestore(hba->host->host_lock, flags);

		if (ret) {
			dev_err(hba->dev, "sunxi ufs suspend scsi device get faile\n");
			goto out;
		}

		/*disable uevent to avoid netlink(cause by ufs device UAC) to resmue systme**/
		dev_set_uevent_suppress(&sdp->sdev_gendev, true);
		dev_dbg(hba->dev, "disable uevent\n");
		scsi_device_put(sdp);
		sunxi_ufs_sys_clk_deinit(hba);
	} else {
		dev_err(hba->dev, "Unsupport pm_op %x\nr", pm_op);
		ret = -EINVAL;
	}

out:
	return ret;
}
#else
int  sunxi_ufs_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op,
					enum ufs_notify_change_status status)
{
	int ret = 0;
	if (pm_op == UFS_SYSTEM_PM) {
		if (status == POST_CHANGE) {
			sunxi_ufs_sys_clk_deinit(hba);
		} else if (status == PRE_CHANGE) {
			struct scsi_device *sdp;
			unsigned long flags;

			spin_lock_irqsave(hba->host->host_lock, flags);
			sdp = sunxi_hba_to_wlun(hba);
			if (sdp && scsi_device_online(sdp))
				ret = scsi_device_get(sdp);
			else
				ret = -ENODEV;
			spin_unlock_irqrestore(hba->host->host_lock, flags);

			if (ret) {
				dev_err(hba->dev, "sunxi ufs suspend scsi device get faile\n");
				goto out;
			}

			/*disable uevent to avoid netlink(cause by ufs device UAC) to resmue systme**/
			dev_set_uevent_suppress(&sdp->sdev_gendev, true);
			dev_dbg(hba->dev, "disable uevent\n");
			scsi_device_put(sdp);
		} else {
			dev_err(hba->dev, "Unsupport status %x, pm_op %x\n", status, pm_op);
			ret = -EINVAL;
		}
	} else {
		dev_err(hba->dev, "Unsupport pm_op %x status %x\nr", pm_op, status);
		ret = -EINVAL;
	}

out:
	return ret;
}
#endif

int sunxi_ufs_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	int ret = 0;

	if (pm_op == UFS_SYSTEM_PM) {
			struct scsi_device *sdp;
			unsigned long flags;

			spin_lock_irqsave(hba->host->host_lock, flags);
			sdp = sunxi_hba_to_wlun(hba);
			if (sdp && scsi_device_online(sdp))
				ret = scsi_device_get(sdp);
			else
					ret = -ENODEV;
			spin_unlock_irqrestore(hba->host->host_lock, flags);

			if (ret) {
					dev_err(hba->dev, "sunxi ufs resume scsi device get faile\n");
			goto out;
		}

		dev_set_uevent_suppress(&sdp->sdev_gendev, false);
		dev_dbg(hba->dev, "enable uevent\n");
		scsi_device_put(sdp);

		ret = sunxi_ufs_sys_clk_init(hba);
		if (ret)
			goto out;
		ret = sunxi_ufs_host_top_init(hba);
		if (ret)
			goto out;

		/*Avoid:ufshcd_intr: Unhandled interrupt 0x00000000 (0xa0000000, 0xa0000000)*/
		ufshcd_writel(hba,  ufshcd_readl(hba, REG_INTERRUPT_ENABLE) & ~SUNXI_UFS_CARD_INT_BIT,
					  REG_INTERRUPT_ENABLE);
		ufshcd_writel(hba, SUNXI_UFS_CARD_INT_BIT, REG_INTERRUPT_STATUS);
	} else {
		dev_err(hba->dev, "Unsupport pm_op %x", pm_op);
		ret = -EINVAL;
	}
out:
	return ret;
}


static int sunxi_ufs_device_reset(struct ufs_hba *hba)
{
    u32 rv = 0;

	rv = ufshcd_readl(hba, REG_UFS_CFG);
	rv &= ~HW_RST_N;
	ufshcd_writel(hba, rv, REG_UFS_CFG);
    /*
	 ** The reset signal is active low. UFS devices shall detect
	 ** more than or equal to 1us of positive or negative RST_n
	 ** pulse width.
	 **
	 ** To be on safe side, keep the reset low for at least 10us.
	 * */
	udelay(15);

	rv |= HW_RST_N;
	ufshcd_writel(hba, rv, REG_UFS_CFG);
    //ufs_mtk_device_reset_ctrl(1, res);

    /* Some devices may need time to respond to rst_n */
	udelay(15000);
//	dev_dbg(hba->dev,"dev rst\n");
	return 0;
}

static int sunxi_ufs_hce_enable_notify(struct ufs_hba *hba,
				enum ufs_notify_change_status status)
{
	switch (status) {
	case PRE_CHANGE:
		break;
	case POST_CHANGE:
		break;
	}

	return 0;
}
#ifdef SUNXI_UFS_RAW_CCU_SETTING

static void sunxi_ufs_cfg_clk_init(void)
{
	u32 v = readl(SUNXI_UFS_CFG_CLK_REG);
	v &= ~SUNXI_UFS_CFG_CLK_GATING_BIT;
	writel(v, SUNXI_UFS_CFG_CLK_REG);

	v = readl(SUNXI_UFS_CFG_CLK_REG);
	v &= ~SUNXI_UFS_CFG_FACTOR_M_MASK;
	/*use defult value,that is cfg clk is19.2M*/
	v |= 0x18;
	writel(v, SUNXI_UFS_CFG_CLK_REG);
	udelay(10);

	v = readl(SUNXI_UFS_CFG_CLK_REG);
	v &= ~SUNXI_UFS_CFG_CLK_SRC_SEL_MASK;
	v |= SUNXI_UFS_CFG_SRC_PERI0_480M;
	writel(v, SUNXI_UFS_CFG_CLK_REG);
	udelay(10);

	v = readl(SUNXI_UFS_CFG_CLK_REG);
	v |= SUNXI_UFS_CFG_CLK_GATING_BIT;
	writel(v, SUNXI_UFS_CFG_CLK_REG);
	/*only for safe*/
	udelay(10);
	dev_dbg(hba->dev, "SUNXI_UFS_CFG_CLK_REG %x\n", readl(SUNXI_UFS_CFG_CLK_REG));
}
/*for cfg clk use default value,so it deinit is the same with init*/
static void sunxi_ufs_cfg_clk_deinit(void)
{
	u32 v = readl(SUNXI_UFS_CFG_CLK_REG);
	v &= ~SUNXI_UFS_CFG_CLK_GATING_BIT;
	writel(v, SUNXI_UFS_CFG_CLK_REG);

	v = readl(SUNXI_UFS_CFG_CLK_REG);
	v &= ~SUNXI_UFS_CFG_FACTOR_M_MASK;
	/*use defult value,that is cfg clk is19.2M*/
	v |= 0x18;
	writel(v, SUNXI_UFS_CFG_CLK_REG);
	udelay(10);

	v = readl(SUNXI_UFS_CFG_CLK_REG);
	v &= ~SUNXI_UFS_CFG_CLK_SRC_SEL_MASK;
	v |= SUNXI_UFS_CFG_SRC_PERI0_480M;
	writel(v, SUNXI_UFS_CFG_CLK_REG);
	udelay(10);

	v = readl(SUNXI_UFS_CFG_CLK_REG);
	v |= SUNXI_UFS_CFG_CLK_GATING_BIT;
	writel(v, SUNXI_UFS_CFG_CLK_REG);
	/*only for safe*/
	udelay(10);
	dev_dbg(hba->dev, "SUNXI_UFS_CFG_CLK_REG\n", readl(SUNXI_UFS_CFG_CLK_REG));
}



static void sunxi_ufs_sys_ref_clk_init(u32 *ref_val)
{
	u32 reg_val = 0;
//	int ret = 0;
	u32 ref_ext = 0;
	u32 ref_cfg_val = 0;

	switch (get_hosc_type_ufs()) {
	case SUNXI_UFS_EXT_REF_26M:
		ref_cfg_val |= REF_CLK_UNIPRO_SEL_BIT | REF_CLK_APP_SEL_BIT | REF_CLK_FREQ_SEL_26M;
		ref_ext = 1;
		sunxi_ufs_trace_point(SUNXI_UFS_EXT_REF_26M);
		break;
	case SUNXI_UFS_INT_REF_19_2M:
		ref_cfg_val |= REF_CLK_FREQ_SEL_19_2M;
		ref_ext  = 0;
		sunxi_ufs_trace_point(SUNXI_UFS_INT_REF_19_2M);
		break;
	case SUNXI_UFS_INT_REF_26M:
		ref_cfg_val |= REF_CLK_FREQ_SEL_26M;
		ref_ext = 0;
		sunxi_ufs_trace_point(SUNXI_UFS_INT_REF_26M);
		break;
	case SUNXI_UFS_EXT_REF_19_2M:
		ref_cfg_val |= REF_CLK_UNIPRO_SEL_BIT | REF_CLK_APP_SEL_BIT | REF_CLK_FREQ_SEL_19_2M;
		ref_ext = 1;
		sunxi_ufs_trace_point(SUNXI_UFS_EXT_REF_19_2M);
		break;
	default:
		ref_cfg_val |= REF_CLK_FREQ_SEL_26M;
		ref_ext = 0;
		sunxi_ufs_trace_point(SUNXI_UFS_INT_REF_26M);
		break;
	}

	/*dcxo ufs gating should set according to ref clock*/
	reg_val = readl(SUNXI_XO_CTRL_WP_REG) & (~0xffff);
	writel(reg_val | 0x16aa, SUNXI_XO_CTRL_WP_REG);
	dev_dbg(hba->dev, "XO_CTRL_WP %x\n", readl(SUNXI_XO_CTRL_WP_REG));

	if (ref_ext == 1) {
		/*disable refclk_out to ufs device*/
		writel(readl(SUNXI_XO_CTRL_REG) | SUNXI_CLK_REQ_EN, SUNXI_XO_CTRL_REG);

		reg_val = readl(SUNXI_XO_CTRL1_REG);
		reg_val &= ~SUNXI_DCXO_UFS_GATING;
		writel(reg_val, SUNXI_XO_CTRL1_REG);

	} else {
		/*enable ref clk to host controller*/
		reg_val = readl(SUNXI_XO_CTRL1_REG);
		reg_val |= SUNXI_DCXO_UFS_GATING;
		writel(reg_val, SUNXI_XO_CTRL1_REG);

		/*enable refclk_out to ufs device*/
		writel(readl(SUNXI_XO_CTRL_REG) & (~SUNXI_CLK_REQ_EN), SUNXI_XO_CTRL_REG);

	}
	dev_dbg(hba->dev, "XO_CTRL %x\n", readl(SUNXI_XO_CTRL_REG));
	dev_dbg(hba->dev, "XO_CTRL1 %x\n", readl(SUNXI_XO_CTRL1_REG));
	dev_dbg(hba->dev, "XO_CTRL_WP %x\n", readl(SUNXI_XO_CTRL_WP_REG));
	//dev_info(hba->dev,"U3U2_PHY_MUX_CTRL %x\n", readl(SUNXI_U3U2_PHY_MUX_CTRL_REG));
//	dev_info(hba->dev,"SUNXI_UFS_REF_CLK_EN_REG %x\n", readl(SUNXI_UFS_REF_CLK_EN_REG));
	*ref_val =  ref_cfg_val;
}

static void sunxi_ufs_sys_ref_clk_deinit(void)
{
	u32 reg_val = readl(SUNXI_XO_CTRL_WP_REG) & (~0xffff);
	writel(reg_val | 0x16aa, SUNXI_XO_CTRL_WP_REG);
	/*set refclk_out to default*/
	writel(readl(SUNXI_XO_CTRL_REG) | SUNXI_CLK_REQ_EN, SUNXI_XO_CTRL_REG);

	/*set dcxo ufs gating to default value*/
	reg_val = readl(SUNXI_XO_CTRL1_REG);
	reg_val &= ~SUNXI_DCXO_UFS_GATING;
	writel(reg_val, SUNXI_XO_CTRL1_REG);
}
static void sunxi_ufs_axi_onoff(u32 on)
{
	u32 reg_val = 0;

	if (!on) {
		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val &= ~SUNXI_UFS_AXI_CLK_GATING_BIT;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);

		reg_val = readl(SUNXI_UFS_BGR_REG);
		reg_val &= ~SUNXI_UFS_AXI_RST_BIT;
		writel(reg_val, SUNXI_UFS_BGR_REG);

		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val &= ~SUNXI_UFS_AXI_FACTOR_M_MASK;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);
		udelay(10);

		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val &= ~SUNXI_UFS_CLK_SRC_SEL_MASK;
		reg_val |= SUNXI_UFS_SRC_PERI0_300M;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);
		udelay(10);
	} else  {
		/*close clk gate before change clock according to ccmu spec
		 *although we will call sunxi_ufs_axi_onoff(0) before sunxi_ufs_axi_onoff(1)
		 *here close clk gate for safe.
		 * */
		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val &= ~SUNXI_UFS_AXI_CLK_GATING_BIT;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);

		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val &= ~SUNXI_UFS_CLK_SRC_SEL_MASK;
		reg_val |= SUNXI_UFS_SRC_PERI0_200M;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);
		udelay(10);

		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val &= ~SUNXI_UFS_AXI_FACTOR_M_MASK;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);
		udelay(10);

		reg_val = readl(SUNXI_UFS_BGR_REG);
		reg_val |= SUNXI_UFS_AXI_RST_BIT;
		writel(reg_val, SUNXI_UFS_BGR_REG);

		reg_val = readl(SUNXI_UFS_AXI_CLK_REG);
		reg_val |= SUNXI_UFS_AXI_CLK_GATING_BIT;
		writel(reg_val, SUNXI_UFS_AXI_CLK_REG);
	}
}
static void sunxi_ufs_ahb_onoff(u32 on)
{
	u32 reg_val = 0;

	/******ahb*******/
	if (!on) {
		reg_val = readl(SUNXI_UFS_BGR_REG);
		reg_val &= ~SUNXI_UFS_GATING_BIT;
		writel(reg_val, SUNXI_UFS_BGR_REG);

		reg_val = readl(SUNXI_UFS_BGR_REG);
		reg_val &= ~SUNXI_UFS_RST_BIT;
		writel(reg_val, SUNXI_UFS_BGR_REG);
	} else {
		reg_val = readl(SUNXI_UFS_BGR_REG);
		reg_val |= SUNXI_UFS_RST_BIT;
		writel(reg_val, SUNXI_UFS_BGR_REG);

		reg_val = readl(SUNXI_UFS_BGR_REG);
		reg_val |= SUNXI_UFS_GATING_BIT;
		writel(reg_val, SUNXI_UFS_BGR_REG);
	}
}
#endif
static int sunxi_ufs_sys_clk_init(struct ufs_hba *hba)
{
	struct ufs_sunxi_priv *priv = NULL;
	int rval = 0;
	u32 rate = 0;
	priv = hba->priv;
#if 1
	/*Only to avoid waring when disable clk and rst if no enable first*
	 *Start
	 * */
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	reset_control_deassert(priv->rst[SUNXI_UFS_RST].urst);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	rval = clk_prepare_enable(priv->clk[SUNXI_UFS_GATING].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable ahb clk err %d\n", rval);
		return -1;
	}
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	reset_control_deassert(priv->rst[SUNXI_UFS_AXI_RST].urst);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);
	rval = clk_prepare_enable(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable axi clk err %d\n", rval);
		return -1;
	}
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	dev_dbg(hba->dev, "ahb axi clk rst deassert\n");
	//sunxi_ufs_dump_ccu_reg();
	/*End**/

	/*Reset all clk and reset for safe**/
	reset_control_assert(priv->rst[SUNXI_UFS_PHY_RST].urst);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	reset_control_assert(priv->rst[SUNXI_UFS_CORE_RST].urst);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	clk_disable_unprepare(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	reset_control_assert(priv->rst[SUNXI_UFS_AXI_RST].urst);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	clk_disable_unprepare(priv->clk[SUNXI_UFS_GATING].uclk);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	reset_control_assert(priv->rst[SUNXI_UFS_RST].urst);
	//dev_err(hba->dev, "%s,%d\n", __FUNCTION__, __LINE__);

	//sunxi_ufs_dump_ccu_reg();
#endif
	dev_dbg(hba->dev, "sys clk init\n");

	rval = clk_prepare_enable(priv->clk[SUNXI_UFS_CFG_CLK_GATING].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable cfg clk gate %d\n", rval);
		return -1;
	}


	reset_control_deassert(priv->rst[SUNXI_UFS_RST].urst);

	rval = clk_set_parent(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk, priv->clk[SUNXI_PLL_PERI].uclk);
	if (rval) {
		dev_err(hba->dev, "set parent failed\n");
		return -1;
	}

	rate = clk_round_rate(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk, SUNXI_UFS_AXI_CLK);
	//Here we dump register,and find ufs axi clk gating has disable,so no disable here to avoid warining in clk
	//clk_disable_unprepare(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk);
	rval = clk_set_rate(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk, rate);
	if (rval) {
		dev_err(hba->dev, "set mclk rate error, rate %dHz\n",
			rate);
		return -1;
	}
	//sunxi_ufs_dump_ccu_reg();


	/*special purpose bus*/
	rval = clk_prepare_enable(priv->clk[SUNXI_MSI_LITE_GATE].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable msi lite gate err %d\n", rval);
		return -1;
	}

	rval = clk_prepare_enable(priv->clk[SUNXI_STORE_AHB_GATE].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable store ahb err %d\n", rval);
		return -1;
	}

	rval = clk_prepare_enable(priv->clk[SUNXI_STORE_MBUS_GATE].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable store mbus gate %d\n", rval);
		return -1;
	}




	rval = clk_prepare_enable(priv->clk[SUNXI_UFS_GATING].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable ahb clk err %d\n", rval);
		return -1;
	}

	reset_control_deassert(priv->rst[SUNXI_UFS_AXI_RST].urst);
	rval = clk_prepare_enable(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable axi clk err %d\n", rval);
		return -1;
	}
	dev_dbg(hba->dev, "ahb axi clk rst deassert\n");

	rval = clk_prepare_enable(priv->clk[SUNXI_RTC_DCXO_WAKEUP].uclk);
	if (rval) {
		dev_err(hba->dev, "Enable rtc dcxo wakeup %d\n", rval);
		return -1;
	}
	dev_dbg(hba->dev, "enable rtc dcxo wakeup\n");
	//sunxi_ufs_dump_ccu_reg();

	return 0;
}


static int sunxi_ufs_host_init(struct ufs_hba *hba)
{
//	u32 reg_val = 0;
	int ret = 0;
//	u32 ref_cfg_val = 0;
//	struct ufs_sunxi_priv *priv = NULL;

#ifdef SUNXI_UFS_RAW_CCU_SETTING
	sunxi_ufs_ahb_onoff(0);
	sunxi_ufs_axi_onoff(0);
	sunxi_ufs_axi_onoff(1);
	sunxi_ufs_ahb_onoff(1);
#else

	ret = ufs_sunxi_common_init(hba);
	if (ret)
		return ret;

	ret = sunxi_ufs_sys_clk_init(hba);
	if (ret)
		return ret;
#endif
	ret = sunxi_ufs_host_top_init(hba);
	return ret;

}

static int sunxi_ufs_host_top_init(struct ufs_hba *hba)
{
	u32 reg_val = 0;
	//int ret = 0;
	u32 ref_cfg_val = 0;
	struct ufs_sunxi_priv *priv = NULL;

/*
 * 1.Issue Power on Reset(assert Reset_n and phy_reset)
 */
#ifdef SUNXI_UFS_RAW_CCU_SETTING
	reg_val = readl(SUNXI_UFS_BGR_REG);
	reg_val &= ~(SUNXI_UFS_CORE_RST_BIT | SUNXI_UFS_PHY_RST_BIT);
	writel(reg_val, SUNXI_UFS_BGR_REG);
#else
	priv = hba->priv;
	reset_control_assert(priv->rst[SUNXI_UFS_PHY_RST].urst);
	reset_control_assert(priv->rst[SUNXI_UFS_CORE_RST].urst);
	dev_dbg(hba->dev, "core/phy rset assert\n");
	//sunxi_ufs_dump_ccu_reg();

#endif
/*
*a.Set sram_bypass/sram_ext_ld_done according to FW usage scenario,
*for detailed information see the (PHY databook) Firmware Usage Scenario
* here brom use sram mode
*/
	reg_val = ufshcd_readl(hba, REG_UFS_CFG);
	reg_val &= ~(MPHY_SRAM_BYPASS_BIT | MPHY_SRAM_EXT_LD_DONE_BIT);
	ufshcd_writel(hba, reg_val, REG_UFS_CFG);
/*
*b.Provide stable config clock and reference clock (required in host mode, optional in device mode)
*to MPHY before reset release. See Device mode support of RMMI databook for reference
*clock requirement in device mode
*/
#ifdef SUNXI_UFS_RAW_CCU_SETTING
	sunxi_ufs_cfg_clk_init();
	sunxi_ufs_sys_ref_clk_init(&ref_cfg_val);
#else
	//ref_cfg_val = SUNXI_UFS_INT_REF_26M;
	ref_cfg_val =  REF_CLK_FREQ_SEL_26M;
	dev_info(hba->dev, "fore use int ref 26m\n");
#endif
	/*open cfg clk gate in host, and open clk24m too*/
	reg_val = ufshcd_readl(hba, REG_UFS_CLK_GATE);
	reg_val &= ~(MPHY_CFGCLK_GATE_BIT | CLK24M_GATE_BIT);
	ufshcd_writel(hba, reg_val, REG_UFS_CLK_GATE);

/*
 *c.Set the cfg_clock_freq port according to the cfg_clock frequency
 */
	reg_val = ufshcd_readl(hba, REG_UFS_CFG);
	reg_val &=  ~(CFG_CLK_FREQ_MASK);
	reg_val |= CFG_CLK_19_2M;
	ufshcd_writel(hba, reg_val, REG_UFS_CFG);

/*
 *d.Set the ref_clock_sel port value according to the ref_clock selection
 */
	reg_val = ufshcd_readl(hba, REG_UFS_CFG);
	reg_val &=  ~(REF_CLK_FREQ_SEL_MASK);
	reg_val |= ref_cfg_val & REF_CLK_FREQ_SEL_MASK;
	ufshcd_writel(hba, reg_val, REG_UFS_CFG);
/*
 *e.For host mode, set ref_clk_en_unipro/ref_clk_en_app at top level input ports.
 *Application needs to set ref_clk_en_app to expected value.
 *For more details, please refer to section Setting up M-PHY for Device mode vs Host mode of RMMI databook.
 */
#ifdef SUNXI_UFS_RAW_CCU_SETTING
	if (efuse_cfg_ufs_use_ccu_en_app()) {
		u32 rval = ufshcd_readl(hba, REG_UFS_CFG);
		rval &= ~SUNXI_UFS_REF_CLK_EN_APP_EN;
		ufshcd_writel(hba, rval, REG_UFS_CFG);
		if (ref_cfg_val & REF_CLK_APP_SEL_BIT) {
			rval = readl(SUNXI_UFS_REF_CLK_EN_REG);
			rval |= SUNXI_UFS_CCU_REF_CLK_EN_APP_BIT;
			writel(rval, SUNXI_UFS_REF_CLK_EN_REG);
		} else {
			rval = readl(SUNXI_UFS_REF_CLK_EN_REG);
			rval &= ~SUNXI_UFS_CCU_REF_CLK_EN_APP_BIT;
			writel(rval, SUNXI_UFS_REF_CLK_EN_REG);
		}
	} else
#endif
	{
		u32 rval = ufshcd_readl(hba, REG_UFS_CFG);
		rval |= SUNXI_UFS_REF_CLK_EN_APP_EN;
		ufshcd_writel(hba, rval, REG_UFS_CFG);
	}

	reg_val = ufshcd_readl(hba, REG_UFS_CFG);
	reg_val &= ~(REF_CLK_UNIPRO_SEL_BIT | REF_CLK_APP_SEL_BIT);
	reg_val |= ref_cfg_val & (REF_CLK_UNIPRO_SEL_BIT | REF_CLK_APP_SEL_BIT) ;
	ufshcd_writel(hba, reg_val, REG_UFS_CFG);

	/*
	 * have to keep power on the hibernate, otherwise, UFS will cause the bus to freeze
	 * when reading  ufs register.
	 */
	reg_val = ufshcd_readl(hba, REG_UFS_PPU_ACTIVE);
	reg_val &= ~PPU_ACTIVE_EN;
	ufshcd_writel(hba, reg_val, REG_UFS_PPU_ACTIVE);
 #ifdef USE_UNUSED_CODE
	/*ic deisigned as power on defautl, so no need to power on again*/
	/*******power on,must power on before operation other register*******/
	reg_val = ufshcd_readl(hba, REG_UFS_PD_PSW_DLY) & (~PD_PSWON_EN);
	ufshcd_writel(hba, reg_val, REG_UFS_PD_PSW_DLY);
	reg_val = ufshcd_readl(hba, REG_UFS_PD_PSW_DLY);
	reg_val &= ~PD_PSWON_DLY_MASK;
	reg_val |= 0xf;
	ufshcd_writel(hba, reg_val, REG_UFS_PD_PSW_DLY);

	reg_val = ufshcd_readl(hba, REG_UFS_PD_CTRL);
	reg_val &= ~PD_POLICY_MASK;
	reg_val |= 0x1;
	ufshcd_writel(hba, reg_val, REG_UFS_PD_CTRL);

	ret = ufshcd_wait_for_register(hba, REG_UFS_PD_STAT, \
			TRANS_CPT, TRANS_CPT, 20);
	if (ret) {
		dev_err(hba->dev, "%s: wait powr on timeout\n", __func__);
		return ret;
	}
	reg_val = ufshcd_readl(hba, REG_UFS_PD_STAT);
	ufshcd_writel(hba, reg_val, REG_UFS_PD_STAT);
#endif

	/* Ufsch psw power down by defautl.
	 * If power on is not finish,We can't give clock to psw
	 * So disable auto gate after powe on,not before
	 * */
	ufshcd_writel(hba, ufshcd_readl(hba, REG_UFS_CLK_GATE) | 0x800003FF, REG_UFS_CLK_GATE);
	dev_dbg(hba->dev, "REG_UFS_CFG %x\n", ufshcd_readl(hba, REG_UFS_CFG));
	dev_dbg(hba->dev, " REG_UFS_CLK_GATE %x\n", ufshcd_readl(hba,  REG_UFS_CLK_GATE));

/*
 * 3.De-assert Reset_n
 */
#ifdef SUNXI_UFS_RAW_CCU_SETTING
	reg_val = readl(SUNXI_UFS_BGR_REG);
	reg_val |= SUNXI_UFS_CORE_RST_BIT;
	writel(reg_val, SUNXI_UFS_BGR_REG);
#else
	reset_control_deassert(priv->rst[SUNXI_UFS_CORE_RST].urst);
	dev_dbg(hba->dev, "core rst clk deassert\n");
	//sunxi_ufs_dump_ccu_reg();

#endif
	//ufshcd_dump_regs(hba, 0, UFSHCI_REG_SPACE_SIZE, "host_regs: ");

	return 0;
}
#ifdef SUNXI_UFS_RAW_CCU_SETTING
void sunxi_ufs_host_exit(struct ufs_hba *hba)
{
	u32 reg_val = 0;
//	int ret = 0;

 #ifdef USE_UNUSED_CODE
	/*ic deisigned as power on defautl, so no need to power on again, no need power off*/
	/*power off*/
	reg_val = ufshcd_readl(hba, REG_UFS_PD_CTRL);
	reg_val &= ~PD_POLICY_MASK;
	reg_val |= 0x2;
	ufshcd_writel(hba, reg_val, REG_UFS_PD_CTRL);

	ret = ufshcd_wait_for_register(hba, REG_UFS_PD_STAT, \
				TRANS_CPT, TRANS_CPT, 20);
	if (ret) {
		dev_err(hba->dev, "%s: wait powr off timeout\n", __func__);
		//return ret;
	}
	reg_val = ufshcd_readl(hba, REG_UFS_PD_STAT);
	ufshcd_writel(hba, reg_val, REG_UFS_PD_STAT);
#endif

	reg_val = readl(SUNXI_UFS_BGR_REG);
	reg_val &= ~SUNXI_UFS_PHY_RST_BIT;
	writel(reg_val, SUNXI_UFS_BGR_REG);

	reg_val = readl(SUNXI_UFS_BGR_REG);
	reg_val &= ~SUNXI_UFS_CORE_RST_BIT;
	writel(reg_val, SUNXI_UFS_BGR_REG);

	if (efuse_cfg_ufs_use_ccu_en_app()) {
		reg_val = readl(SUNXI_UFS_REF_CLK_EN_REG);
		reg_val &= ~SUNXI_UFS_CCU_REF_CLK_EN_APP_BIT;
		writel(reg_val, SUNXI_UFS_REF_CLK_EN_REG);
	}

	sunxi_ufs_sys_ref_clk_deinit();
	sunxi_ufs_cfg_clk_deinit();
	sunxi_ufs_ahb_onoff(0);
	sunxi_ufs_axi_onoff(0);
}
#else

static int sunxi_ufs_sys_clk_deinit(struct ufs_hba *hba)
{
	struct ufs_sunxi_priv *priv = NULL;
	//int rval = 0;
	priv = hba->priv;

	dev_dbg(hba->dev, "sys clk deinit\n");

	reset_control_assert(priv->rst[SUNXI_UFS_PHY_RST].urst);
	reset_control_assert(priv->rst[SUNXI_UFS_CORE_RST].urst);

	clk_disable_unprepare(priv->clk[SUNXI_RTC_DCXO_WAKEUP].uclk);
	clk_disable_unprepare(priv->clk[SUNXI_UFS_AXI_CLK_GATING].uclk);
	reset_control_assert(priv->rst[SUNXI_UFS_AXI_RST].urst);

	clk_disable_unprepare(priv->clk[SUNXI_UFS_GATING].uclk);

	/*special purpose bus*/
	clk_disable_unprepare(priv->clk[SUNXI_STORE_MBUS_GATE].uclk);
	clk_disable_unprepare(priv->clk[SUNXI_STORE_AHB_GATE].uclk);
	clk_disable_unprepare(priv->clk[SUNXI_MSI_LITE_GATE].uclk);

	reset_control_assert(priv->rst[SUNXI_UFS_RST].urst);

	clk_disable_unprepare(priv->clk[SUNXI_UFS_CFG_CLK_GATING].uclk);
	return 0;
}

void sunxi_ufs_host_exit(struct ufs_hba *hba)
{
	sunxi_ufs_sys_clk_deinit(hba);
}

#endif

static int ufs_sunxi_get_reset_ctrl(struct device *dev, struct ufs_sunxi_rst *rst)

{
	rst->urst = devm_reset_control_get(dev, rst->name);
	if (IS_ERR(rst->urst)) {
		dev_err(dev, "failed to get reset ctrl:%s\n", rst->name);
		return PTR_ERR(rst->urst);
	}

	return 0;
}

static int ufs_sunxi_get_clk_ctrl(struct device *dev, struct ufs_sunxi_clk *clk)

{
	clk->uclk = devm_clk_get(dev, clk->name);
	if (IS_ERR(clk->uclk)) {
		dev_err(dev, "failed to get clk:%s\n", clk->name);
		return PTR_ERR(clk->uclk);
	}
	return 0;
}





static int ufs_ufs_parse_dt(struct device *dev, struct ufs_hba *hba)
{
	u32 i;
	struct ufs_sunxi_priv *priv = hba->priv;
	int ret = 0;

	/* Parse UFS reset ctrl info */
	for (i = 0; i < SUNXI_UFS_RST_MAX; i++) {
		if (!priv->rst[i].name)
			continue;
		ret = ufs_sunxi_get_reset_ctrl(dev, &priv->rst[i]);
		if (ret)
			goto out;
	}

	/* Parse UFS clke ctrl info */
	for (i = 0; i < SUNXI_UFS_CLK_MAX; i++) {
		if (!priv->clk[i].name)
			continue;
		ret = ufs_sunxi_get_clk_ctrl(dev, &priv->clk[i]);
		if (ret)
			goto out;
	}

#ifdef USE_UNUSED_CODE
	/* Parse UFS syscon reg info */
	for (i = 0; i < SPRD_UFS_SYSCON_MAX; i++) {
		if (!priv->sysci[i].name)
			continue;
		ret = ufs_sprd_get_syscon_reg(dev, &priv->sysci[i]);
		if (ret)
			goto out;
	}

	/* Parse UFS vreg info */
	for (i = 0; i < SPRD_UFS_VREG_MAX; i++) {
		if (!priv->vregi[i].name)
			continue;
		ret = ufs_sprd_get_vreg(dev, &priv->vregi[i]);
		if (ret)
			goto out;
	}
#endif
out:
	return ret;
}

static struct ufs_sunxi_priv sunxi_ufs_host_priv = {
	.rst[SUNXI_UFS_CORE_RST] = { .name = "controller_rst", },
	.rst[SUNXI_UFS_PHY_RST] = { .name = "phy_rst", },
	.rst[SUNXI_UFS_AXI_RST] = { .name = "axi_rst", },
	.rst[SUNXI_UFS_RST] = { .name = "ahb_rst", },
	.clk[SUNXI_UFS_GATING] = { .name = "ahb_gate", },
	.clk[SUNXI_UFS_AXI_CLK_GATING] = { .name = "axi_clk_gate", },
	.clk[SUNXI_UFS_CFG_CLK_GATING] = { .name = "cfg_clk_gate", },
	.clk[SUNXI_PLL_PERI] = {.name = "pll_periph"},

	.clk[SUNXI_MSI_LITE_GATE] = { .name = "msi_lite", },
	.clk[SUNXI_STORE_AHB_GATE] = { .name = "store_ahb", },
	.clk[SUNXI_STORE_MBUS_GATE] = { .name = "store_mbus", },
	.clk[SUNXI_RTC_DCXO_WAKEUP] = { .name = "dcxo_wakeup", },
	.phy_hs_rate = PA_HS_MODE_B,
};




static int ufs_sunxi_common_init(struct ufs_hba *hba)
{
	struct device *dev = hba->dev;
	//struct ufs_sprd_host *host;
	struct platform_device __maybe_unused *pdev = to_platform_device(dev);
	//const struct of_device_id *of_id;
	int ret = 0;

/*
	host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
	if (!host)
		return -ENOMEM;

	of_id = of_match_node(ufs_sprd_of_match, pdev->dev.of_node);
	if (of_id->data != NULL)
		host->priv = container_of(of_id->data, struct ufs_sprd_priv,
					  ufs_hba_sprd_vops);

	host->hba = hba;
	ufshcd_set_variant(hba, host);

	hba->caps |= UFSHCD_CAP_CLK_GATING |
		UFSHCD_CAP_CRYPTO |
		UFSHCD_CAP_WB_EN;
	hba->quirks |= UFSHCD_QUIRK_DELAY_BEFORE_DME_CMDS;

	ret = ufs_sprd_parse_dt(dev, hba, host);
*/
	//hba->quirks |= UFSHCD_QUIRK_BROKEN_AUTO_HIBERN8;
	ufshcd_set_variant(hba, &sunxi_ufs_host_priv);
	ret = ufs_ufs_parse_dt(dev, hba);

	return ret;
}



/*
 * UFS sunxi specific variant operations
 */
static struct ufs_hba_variant_ops sunxi_ufs_v0_pltfm_hba_vops = {
	.name                   = "sunxi-ufs-v0-pltfm",
	.init = sunxi_ufs_host_init,
	.exit = sunxi_ufs_host_exit,
	.hce_enable_notify = sunxi_ufs_hce_enable_notify,
	.link_startup_notify = sunxi_ufs_link_startup_notify,
	.pwr_change_notify = sunxi_ufs_pwr_change_notify,
	.phy_initialization = sunxi_ufs_phy_config,
	.device_reset = sunxi_ufs_device_reset,
	.dbg_register_dump = sunxi_ufs_register_dump,
	.hibern8_notify = sunxi_ufs_hibern8_notify,
	.suspend = sunxi_ufs_suspend,
	.resume = sunxi_ufs_resume,
};


static const struct of_device_id sunxi_ufs_pltfm_match[] = {
	{
		.compatible = "allwinner,sunxi-ufs-v0",
		.data = &sunxi_ufs_v0_pltfm_hba_vops,
	},
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_ufs_pltfm_match);

/**
 * sunxi_ufs_pltfm_probe()
 * @pdev: pointer to platform device structure
 *
 */
static int sunxi_ufs_pltfm_probe(struct platform_device *pdev)
{
	int err;
	const struct of_device_id *of_id;
	struct ufs_hba_variant_ops *vops;
	struct device *dev = &pdev->dev;

	dev_info(dev, "Sunxi ufs driver version %s\n", SUNXI_UFS_DRIVER_VESION);
	of_id = of_match_node(sunxi_ufs_pltfm_match, dev->of_node);
	vops = (struct ufs_hba_variant_ops *)of_id->data;

	//ufshcd_set_variant(hba, priv);
	/* Perform generic probe */
	err = ufshcd_pltfrm_init(pdev, vops);
	if (err)
		dev_err(dev, "ufshcd_pltfrm_init() failed %d\n", err);

	return err;
}

/**
 * sunxi_ufs_pltfm_remove()
 * @pdev: pointer to platform device structure
 *
 */
static int sunxi_ufs_pltfm_remove(struct platform_device *pdev)
{
	struct ufs_hba *hba =  platform_get_drvdata(pdev);

	pm_runtime_get_sync(&(pdev)->dev);
	ufshcd_remove(hba);

	return 0;
}

static const struct dev_pm_ops sunxi_ufs_pltfm_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ufshcd_system_suspend, ufshcd_system_resume)
	SET_RUNTIME_PM_OPS(ufshcd_runtime_suspend, ufshcd_runtime_resume, NULL)
	.prepare	 = ufshcd_suspend_prepare,
	.complete	 = ufshcd_resume_complete,
};

static struct platform_driver sunxi_ufs_pltfm_driver = {
	.probe		= sunxi_ufs_pltfm_probe,
	.remove		= sunxi_ufs_pltfm_remove,
	.driver		= {
		.name	= "sunxi-ufs-pltfm",
		.pm	= &sunxi_ufs_pltfm_pm_ops,
		.of_match_table	= of_match_ptr(sunxi_ufs_pltfm_match),
	},
};

module_platform_driver(sunxi_ufs_pltfm_driver);

MODULE_ALIAS("platform:sunxi-ufs-pltfm");
MODULE_DESCRIPTION("allwinnertech platform glue driver");
MODULE_AUTHOR("lxiang <lixiang@allwinnerte.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(SUNXI_UFS_DRIVER_VESION);
