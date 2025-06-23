// SPDX-License-Identifier: GPL-2.0
/*
 * phy driver for cdn_sd0803_t7g_typea
 *
 * Author: Matthew MA <Matthew.Ma@cixtech.com>
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include "phy-cix-usbdp.h"

struct cix_u3phy_con_dir {
	u16 offset;
	u8 bit;
};


struct cix_u3phy;

struct cix_u3phy_cfg {
	int (*u3phy_init)(struct cix_u3phy *u3phy);
	int (*u3phy_exit)(struct cix_u3phy *u3phy);
};

static const struct reg_sequence sky1_u3phy_conf[] = {
	{CMN_SSM_BIAS_TMR,                  0x0018},
	{CMN_PLLSM0_PLLPRE_TMR,             0x0030},
	{CMN_PLLSM0_PLLLOCK_TMR,            0x00f0},
	{CMN_PLLSM1_PLLPRE_TMR,             0x0030},
	{CMN_PLLSM1_PLLLOCK_TMR,            0x00f0},
	{CMN_BGCAL_INIT_TMR,                0x0078},
	{CMN_BGCAL_ITER_TMR,                0x0078},
	{CMN_IBCAL_INIT_TMR,                0x0018},
	{CMN_TXPUCAL_INIT_TMR,              0x001d},
	{CMN_TXPDCAL_INIT_TMR,              0x001d},
	{CMN_RXCAL_INIT_TMR,                0x02d0},
	{CMN_SD_CAL_PLLCNT_START,           0x0137},
	{RX_SDCAL0_INIT_TMR_LANE0,          0x0018},
	{RX_SDCAL0_INIT_TMR_LANE1,          0x0018},
	{RX_SDCAL0_ITER_TMR_LANE0,          0x0078},
	{RX_SDCAL0_ITER_TMR_LANE1,          0x0078},
	{RX_SDCAL1_INIT_TMR_LANE0,          0x0018},
	{RX_SDCAL1_INIT_TMR_LANE1,          0x0018},
	{RX_SDCAL1_ITER_TMR_LANE0,          0x0078},
	{RX_SDCAL1_ITER_TMR_LANE1,          0x0078},
	{TX_RCVDET_ST_TMR_LANE0,            0x0960},
	{TX_RCVDET_ST_TMR_LANE1,            0x0960},

	{PHY_PLL_CFG_0803,                  0x0000},
	{CMN_PDIAG_PLL0_CLK_SEL_M0,         0x8600},
	{XCVR_DIAG_HSCLK_SEL_LANE0,         0x0000},
	{XCVR_DIAG_HSCLK_SEL_LANE1,         0x0000},
	{XCVR_DIAG_HSCLK_DIV_LANE0,         0x0001},
	{XCVR_DIAG_HSCLK_DIV_LANE1,         0x0001},
	{XCVR_DIAG_PLLDRC_CTRL_LANE0,       0x0041},
	{XCVR_DIAG_PLLDRC_CTRL_LANE1,       0x0041},
	{CMN_PLL0_DSM_DIAG_M0,              0x0004},
	{CMN_PLL1_DSM_DIAG_M0,              0x0004},
	{CMN_PDIAG_PLL1_ITRIM_M0,           0x003f},
	{CMN_PDIAG_PLL0_CP_PADJ_M0,         0x0b17},
	{CMN_PDIAG_PLL1_CP_PADJ_M0,         0x0b17},
	{CMN_PDIAG_PLL0_CP_IADJ_M0,         0x0e01},
	{CMN_PDIAG_PLL1_CP_IADJ_M0,         0x0e01},
	{CMN_PDIAG_PLL0_FILT_PADJ_M0,       0x0d05},
	{CMN_PDIAG_PLL1_FILT_PADJ_M0,       0x0d05},
	{CMN_PLL0_INTDIV_M0,                0x01a0},
	{CMN_PLL1_INTDIV_M0,                0x01a0},
	{CMN_PLL0_FRACDIVL_M0,              0xaaab},
	{CMN_PLL1_FRACDIVL_M0,              0xaaab},
	{CMN_PLL0_FRACDIVH_M0,              0x0002},
	{CMN_PLL1_FRACDIVH_M0,              0x0002},
	{CMN_PLL0_HIGH_THR_M0,              0x0116},
	{CMN_PLL1_HIGH_THR_M0,              0x0116},
	{CMN_PDIAG_PLL0_CTRL_M0,            0x1002},
	{CMN_PDIAG_PLL1_CTRL_M0,            0x1002},
	{CMN_PLL0_VCOCAL_INIT_TMR,          0x00f0},
	{CMN_PLL1_VCOCAL_INIT_TMR,          0x00f0},
	{CMN_PLL0_VCOCAL_ITER_TMR,          0x0004},
	{CMN_PLL1_VCOCAL_ITER_TMR,          0x0004},
	{CMN_PLL0_VCOCAL_REFTIM_START,      0x02f8},
	{CMN_PLL1_VCOCAL_REFTIM_START,      0x02f8},
	{CMN_PLL0_VCOCAL_PLLCNT_START,      0x02f8},
	{CMN_PLL1_VCOCAL_PLLCNT_START,      0x02f8},
	{CMN_PLL0_VCOCAL_TCTRL,             0x0003},
	{CMN_PLL1_VCOCAL_TCTRL,             0x0003},
	{CMN_PLL0_LOCK_REFCNT_START,        0x00bf},
	{CMN_PLL1_LOCK_REFCNT_START,        0x00bf},
	{CMN_PLL0_LOCK_PLLCNT_START,        0x00bf},
	{CMN_PLL1_LOCK_PLLCNT_START,        0x00bf},
	{CMN_PLL0_LOCK_PLLCNT_THR,          0x0003},
	{CMN_PLL1_LOCK_PLLCNT_THR,          0x0003},
	{PHY_PIPE_USB3_GEN2_PRE_CFG0_0803,  0x0a0a},
	{PHY_PIPE_USB3_GEN2_POST_CFG0_0803, 0x1000},
	{PHY_PIPE_USB3_GEN2_POST_CFG1_0803, 0x0010},
	{CMN_CDIAG_CDB_PWRI_OVRD,           0x8200},
	{CMN_CDIAG_XCVRC_PWRI_OVRD,         0x8200},
	{TX_PSC_A0_LANE0,                   0x02ff},
	{TX_PSC_A0_LANE1,                   0x02ff},
	{TX_PSC_A1_LANE0,                   0x06af},
	{TX_PSC_A1_LANE1,                   0x06af},
	{TX_PSC_A2_LANE0,                   0x06ae},
	{TX_PSC_A2_LANE1,                   0x06ae},
	{TX_PSC_A3_LANE0,                   0x06ae},
	{TX_PSC_A3_LANE1,                   0x06ae},
	{RX_PSC_A0_LANE0,                   0x0d1d},
	{RX_PSC_A0_LANE1,                   0x0d1d},
	{RX_PSC_A1_LANE0,                   0x0d1d},
	{RX_PSC_A1_LANE1,                   0x0d1d},
	{RX_PSC_A2_LANE0,                   0x0d00},
	{RX_PSC_A2_LANE1,                   0x0d00},
	{RX_PSC_A3_LANE0,                   0x0500},
	{RX_PSC_A3_LANE1,                   0x0500},
	{TX_TXCC_CTRL_LANE0,                0x2a82},
	{TX_TXCC_CTRL_LANE1,                0x2a82},
	{TX_TXCC_CPOST_MULT_01_LANE0,       0x0014},
	{TX_TXCC_CPOST_MULT_01_LANE1,       0x0014},
	{TX_TXCC_MGNFS_MULT_000_LANE0,      0x0002}, // TODO checkout xcvr_avdd_h
	{TX_TXCC_MGNFS_MULT_000_LANE1,      0x0002}, // TODO checkout xcvr_avdd_h
	{RX_SIGDET_HL_FILT_TMR_LANE0,       0x0013},
	{RX_SIGDET_HL_FILT_TMR_LANE1,       0x0013},
	{RX_REE_GCSM1_CTRL_LANE0,           0x0000},
	{RX_REE_GCSM1_CTRL_LANE1,           0x0000},
	{RX_REE_ATTEN_THR_LANE0,            0x0c02},
	{RX_REE_ATTEN_THR_LANE1,            0x0c02},
	{RX_REE_SMGM_CTRL1_LANE0,           0x0330},
	{RX_REE_SMGM_CTRL1_LANE1,           0x0330},
	{RX_REE_SMGM_CTRL2_LANE0,           0x0300},
	{RX_REE_SMGM_CTRL2_LANE1,           0x0300},
	{XCVR_DIAG_PSC_OVRD_LANE0,          0x0003},
	{XCVR_DIAG_PSC_OVRD_LANE1,          0x0003},
	{RX_DIAG_SIGDET_TUNE_LANE0,         0x1004},
	{RX_DIAG_SIGDET_TUNE_LANE1,         0x1004},
	{RX_DIAG_NQST_CTRL_LANE0,           0x00f9},
	{RX_DIAG_NQST_CTRL_LANE1,           0x00f9},
	{RX_DIAG_DFE_AMP_TUNE_2_LANE0,      0x0c01},
	{RX_DIAG_DFE_AMP_TUNE_2_LANE1,      0x0c01},
	{RX_DIAG_DFE_AMP_TUNE_3_LANE0,      0x0002},
	{RX_DIAG_DFE_AMP_TUNE_3_LANE1,      0x0002},
	{RX_DIAG_PI_CAP_LANE0,              0x0000},
	{RX_DIAG_PI_CAP_LANE1,              0x0000},
	{RX_DIAG_PI_RATE_LANE0,             0x0031},
	{RX_DIAG_PI_RATE_LANE1,             0x0031},
	{RX_CDRLF_CNFG_LANE0,               0x018c},
	{RX_CDRLF_CNFG_LANE1,               0x018c},
	{RX_CDRLF_CNFG3_LANE0,              0x0003},
	{RX_CDRLF_CNFG3_LANE1,              0x0003}
};

struct cix_u3phy {
	struct device *dev;
	void __iomem *base;
	struct regmap *phy_regmap;
	struct regmap *usbphy_syscon;
	struct reset_control *preset;
	struct reset_control *reset;
	struct clk *apb_clk;
	struct clk *ref_clk;
	struct mutex mutex; /* mutex to protect access to individual PHYs */
	bool init;
	int init_count;
	int id;
	const struct cix_u3phy_cfg *cfg;
};

static int sky1_u3phy_exit(struct cix_u3phy *u3phy)
{
	u3phy->init_count--;
	if (u3phy->init && (u3phy->init_count == 0)) {
		dev_dbg(u3phy->dev, "sky1_u3phy_exit\n");
		reset_control_assert(u3phy->reset);
		reset_control_assert(u3phy->preset);
		clk_disable_unprepare(u3phy->apb_clk);
		clk_disable_unprepare(u3phy->ref_clk);
		u3phy->init = false;
	}
	return 0;
}
static int sky1_u3phy_init(struct cix_u3phy *u3phy)
{
	int ret;

	if (!u3phy->init) {
		//usb rcsu reset is default deassert
		reset_control_assert(u3phy->reset);
		reset_control_assert(u3phy->preset);

		ret = clk_prepare_enable(u3phy->apb_clk);
		if (ret) {
			dev_err(u3phy->dev, "Failed to prepare_enable u3phy apb clock\n");
			goto assert_reset_preset;
		}

		ret = clk_prepare_enable(u3phy->ref_clk);
		if (ret) {
			dev_err(u3phy->dev, "Failed to prepare_enable u3phy ref clock\n");
			goto disable_apb_clk;
		}

		reset_control_deassert(u3phy->preset);

		ret = regmap_multi_reg_write(u3phy->phy_regmap, sky1_u3phy_conf,
						ARRAY_SIZE(sky1_u3phy_conf));

		if (ret) {
			dev_err(u3phy->dev, "Failed to write the reg sequence\n");
			goto disable_ref_clk;
		}
		u3phy->init = true;
		reset_control_deassert(u3phy->reset);
	}
	u3phy->init_count++;
	return 0;

disable_ref_clk:
	clk_disable_unprepare(u3phy->ref_clk);
disable_apb_clk:
	clk_disable_unprepare(u3phy->apb_clk);
assert_reset_preset:
	reset_control_deassert(u3phy->preset);
	reset_control_deassert(u3phy->reset);
	u3phy->init = false;
	return ret;
}


static const struct cix_u3phy_cfg sky1_u3phy_cfg = {
	.u3phy_init = sky1_u3phy_init,
	.u3phy_exit = sky1_u3phy_exit
};
static int u3phy_regmap_write(void *context, unsigned int reg, unsigned int val)
{
#if (!IS_ENABLED(CONFIG_ARCH_CIX_EMU_FPGA))
	struct cix_u3phy *u3phy = context;
	u32 offset = reg << 2;

	writel(val, u3phy->base + offset);
#endif

	return 0;
}

static int u3phy_regmap_read(void *context, unsigned int reg, unsigned int *val)
{
#if (!IS_ENABLED(CONFIG_ARCH_CIX_EMU_FPGA))
	struct cix_u3phy *u3phy = context;
	u32 offset = reg << 2;

	*val = readl(u3phy->base + offset);
#endif
	return 0;
}


static const struct regmap_config cix_u3phy_regmap_cfg = {
	.reg_bits = 32,
	.reg_stride = 1,//register width, if =2, then only reg 0 2 4 ...2^n can access
	.val_bits = 16,
	.fast_io = true,
	.reg_write = u3phy_regmap_write,
	.reg_read = u3phy_regmap_read,
};

static int cix_usb3_phy_power_on(struct cix_u3phy *u3phy)
{
	int ret;

	const struct cix_u3phy_cfg *phy_cfgs = u3phy->cfg;

	if (phy_cfgs->u3phy_init) {
		ret = phy_cfgs->u3phy_init(u3phy);

		if (ret) {
			dev_err(u3phy->dev, "failed to init udphy\n");
			return ret;
		}
	}

	return 0;
}

static int cix_usb3_phy_power_off(struct cix_u3phy *u3phy)
{
	int ret;
	const struct cix_u3phy_cfg *phy_cfgs = u3phy->cfg;

	if (phy_cfgs->u3phy_exit) {
		ret = phy_cfgs->u3phy_exit(u3phy);

		if (ret) {
			dev_err(u3phy->dev, "failed to exit udphy\n");
			return ret;
		}
	}

	return 0;
}


static int cix_u3phy_init(struct phy *phy)
{
	struct cix_u3phy *u3phy = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&u3phy->mutex);
	ret = cix_usb3_phy_power_on(u3phy);
	mutex_unlock(&u3phy->mutex);
	return ret;
}

static int cix_u3phy_exit(struct phy *phy)
{
	struct cix_u3phy *u3phy = phy_get_drvdata(phy);
	int ret = 0;

	mutex_lock(&u3phy->mutex);

	ret = cix_usb3_phy_power_off(u3phy);

	mutex_unlock(&u3phy->mutex);
	return ret;
}

static const struct phy_ops cix_u3phy_ops = {
	.init		= cix_u3phy_init,
	.exit		= cix_u3phy_exit,
	.owner		= THIS_MODULE,
};

static int cix_u3phy_probe(struct platform_device *pdev)
{
	struct cix_u3phy *u3phy;
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct fwnode_handle *child_fn;
	int ret = 0;

	u3phy = devm_kzalloc(dev, sizeof(*u3phy), GFP_KERNEL);
	if (!u3phy)
		return -ENOMEM;

	dev_set_drvdata(dev, u3phy);
	u3phy->dev = dev;

	u3phy->cfg = device_get_match_data(dev);
	if (!u3phy->cfg) {
		dev_err(dev, "no OF data can be matched with %p node\n", np);
		return -EINVAL;
	}

	u3phy->reset = devm_reset_control_get(dev, "reset");
	if (IS_ERR(u3phy->reset)) {
		dev_err(dev, "%s: failed to get reset\n",
			dev->of_node->full_name);
	}

	u3phy->preset = devm_reset_control_get(dev, "preset");
	if (IS_ERR(u3phy->preset)) {
		dev_err(dev, "%s: failed to get preset\n",
			dev->of_node->full_name);
	}

	u3phy->apb_clk = devm_clk_get(dev, "apb_clk");
	if (IS_ERR(u3phy->apb_clk)) {
		dev_err(dev, "phy apb clock not found\n");
		return PTR_ERR(u3phy->apb_clk);
	}

	u3phy->ref_clk = devm_clk_get(dev, "ref_clk");
	if (IS_ERR(u3phy->apb_clk)) {
		dev_err(dev, "phy apb clock not found\n");
		return PTR_ERR(u3phy->apb_clk);
	}

	u3phy->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(u3phy->base))
		return PTR_ERR(u3phy->base);

	u3phy->phy_regmap = devm_regmap_init(dev, NULL, u3phy, &cix_u3phy_regmap_cfg);
	if (IS_ERR(u3phy->phy_regmap)) {
		dev_err(dev, "failed to remap phy register\n");
		return PTR_ERR(u3phy->phy_regmap);
	}

	u3phy->usbphy_syscon =
		device_syscon_regmap_lookup_by_property(&pdev->dev,
					"cix,usbphy_syscon");
	if (IS_ERR(u3phy->usbphy_syscon)) {
		dev_err(dev, "failed get usbphy syscon\n");
		return PTR_ERR(u3phy->usbphy_syscon);
	}

	mutex_init(&u3phy->mutex);

	device_for_each_child_node(dev, child_fn) {
		struct phy *phy;

		child_np = to_of_node(child_fn);

		if (!strncmp(fwnode_get_name(child_fn), "usb-port", 8)
			|| !strncmp(fwnode_get_name(child_fn), "USB", 3))
			phy = devm_phy_create(dev, child_np, &cix_u3phy_ops);
		else
			continue;

		if (IS_ERR(phy)) {
			dev_err(dev, "failed to create phy: %s\n",
						fwnode_get_name(child_fn));
			goto put_child;
		}

		phy_set_drvdata(phy, u3phy);

		phy_create_lookup(phy, fwnode_get_name(child_fn), dev_name(dev));
	}

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		dev_err(dev, "failed to register phy provider\n");
		goto put_child;
	}

	reset_control_assert(u3phy->reset);
	reset_control_assert(u3phy->preset);

	return 0;

put_child:
	of_node_put(child_np);
	return ret;
}

static const struct acpi_device_id cix_udphy_acpi_match[] = {
	{ "CIXH2034", (kernel_ulong_t)&sky1_u3phy_cfg },
	{ },
};
MODULE_DEVICE_TABLE(acpi, cix_udphy_acpi_match);

static const struct of_device_id cix_u3phy_dt_match[] = {
	{
		.compatible = "cix,sky1-usb3-phy",
		.data = &sky1_u3phy_cfg
	},
	{ /* sentinel */ }
};

static struct platform_driver cix_usb3_phy_driver = {
	.probe		= cix_u3phy_probe,
	.driver		= {
		.name	= "cix-usb3-phy",
		.of_match_table = cix_u3phy_dt_match,
		.acpi_match_table = cix_udphy_acpi_match,
		.pm = NULL,
	},
};

module_platform_driver(cix_usb3_phy_driver);

MODULE_AUTHOR("Matthew.ma Ma <matthew.ma@cixteck.com>");
MODULE_DESCRIPTION("Cix USB3 Only PHY driver");
MODULE_LICENSE("GPL v2");

