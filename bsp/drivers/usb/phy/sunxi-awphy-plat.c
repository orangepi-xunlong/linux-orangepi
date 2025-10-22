// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner USB2.0 (AW) Phy driver
 *
 * Copyright (C) 2024 Allwinner Electronics Co., Ltd.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <dt-bindings/phy/phy.h>

/* USB2.0 PHY Control Register */
#define USBC_REG_o_PHY_CTRL(n)			(0x0010 + (0x20 * (n)))
#define   BIST_EN_A				BIT(16)
#define   VC_ADDR				GENMASK(15, 8)
#define   VC_DI					BIT(7)
#define   SIDDQ					BIT(3) /* No need for common */
#define   VC_EN					BIT(1)
#define   VC_CLK				BIT(0)

/* USB2.0 PHY Control Register */
#define USBC_REG_o_PHY_STATUS(n)		(0x0024 + (0x20 * (n)))
#define   BIST_ERR				BIT(17)
#define   BIST_DONE				BIT(16)
#define   VC_DO					BIT(0)

/* USB2.0 PHY Reset Control Register */
#define USBC_REG_o_RST_CTRL(n)			(0x0028 + (0x20 * (n)))
#define   PHY_RST				BIT(0)

/* USB2.0 PHY Version Number Register */
#define USBC_REG_o_PHY_VER			(0x00F8)
#define   PHY_GEN_VER				GENMASK(31, 24)
#define   PHY_SUB_VER				GENMASK(23, 16)
#define   PHY_PRJ_VER				GENMASK(15, 8)

/* Registers */
#define  USBC_REG_PHY_CTRL(phy_base_addr, n)	\
				((phy_base_addr) + USBC_REG_o_PHY_CTRL(n))
#define  USBC_REG_PHY_STATUS(phy_base_addr, n)	\
				((phy_base_addr) + USBC_REG_o_PHY_STATUS(n))
#define  USBC_REG_RST_CTRL(phy_base_addr, n)	\
				((phy_base_addr) + USBC_REG_o_RST_CTRL(n))
#define  USBC_REG_PHY_VER(phy_base_addr)	\
				((phy_base_addr) + USBC_REG_o_PHY_VER)

struct sunxi_phy {
	const char		*name;
	struct phy		*phy;
	struct clk		*ref_clk;
	struct clk		*u2_only_clk;
	struct sunxi_phy_plat	*sunxi_phy;

	__u8			num; /* phy number */
	enum phy_mode		mode;
};

struct sunxi_phy_plat {
	struct device		*dev;
	void __iomem		*phy_reg;
	void __iomem		*top_reg;

	struct clk		*pclk;
	struct clk		*mclk;
	struct reset_control	*reset;
	struct reset_control	*usb_reset;

	struct sunxi_phy	*uphy0;
	struct sunxi_phy	*uphy1;
	struct sunxi_phy	*uphy2;

	__u32			vernum; /* PHY Version number */

	bool			u3_only_quirk;
};

enum sunxi_phy_type_e {
	SUNXI_PHY0_TYPE = 0,
	SUNXI_PHY1_TYPE,
	SUNXI_PHY2_TYPE,
};

/* The AW PHY support x_p0_transceiver, x_p1_transceiver, x_p2_transceiver and x_common module */
#define  USB_XCVR0_PHY_NO		(0)
#define  USB_XCVR1_PHY_NO		(1)
#define  USB_XCVR2_PHY_NO		(2)
#define  USB_COMMON_PHY_NO		(3)

/* Sub-System USB2.0 PHY Support. */
static u32 phy_ver_get(struct sunxi_phy_plat *sunxi_phy)
{
	u32 reg;

	reg = readl(USBC_REG_PHY_VER(sunxi_phy->phy_reg));

	return reg;
}

static void sunxi_phy_set(struct sunxi_phy *phy, bool enable)
{
	struct sunxi_phy_plat *sunxi_phy = phy->sunxi_phy;
	u32 val;

	val = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy->num));
	if (enable)
		val &= ~SIDDQ; /* write 0 to enable phy */
	else
		val |= SIDDQ; /* write 1 to disable phy */
	writel(val, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy->num));

	val = readl(USBC_REG_RST_CTRL(sunxi_phy->phy_reg, phy->num));
	if (enable)
		val |= PHY_RST;
	else
		val &= ~PHY_RST;
	writel(val, USBC_REG_RST_CTRL(sunxi_phy->phy_reg, phy->num));
}

static int sunxi_phy_VCbus_write(struct sunxi_phy *phy, u32 addr, u32 data, u32 len)
{
	u32 j = 0;
	u32 temp = 0;
	u32 dtmp = data;
	u32 phy_offset = phy->num; /* transciver register 0-2 */
	struct sunxi_phy_plat *sunxi_phy = phy->sunxi_phy;

	if (addr < 0x60) /* common register */
		phy_offset = 3;

	/*VC_EN enable*/
	temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
	temp |= VC_EN;
	writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

	for (j = 0; j < len; j++) {
		/*ensure VC_CLK low*/
		temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
		temp &= ~VC_CLK;
		writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

		/*set write address*/
		temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
		temp &= ~VC_ADDR;//clear
		temp |= FIELD_PREP(VC_ADDR, addr + j);  // write
		writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

		/*write data to VC_DI*/
		temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
		temp &= ~VC_DI;//clear
		temp |= FIELD_PREP(VC_DI, dtmp & 0x01);  // write
		writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

		/*set VC_CLK high*/
		temp |= VC_CLK;
		writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

		/*right move one bit*/
		dtmp >>= 1;
	}

	/*set VC_CLK low*/
	temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
	temp &= ~VC_CLK;
	writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

	/*VC_EN disable*/
	temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
	temp &= ~VC_EN;
	writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

	return 0;
}

static u32 sunxi_phy_VCbus_read(struct sunxi_phy *phy, u32 addr, u32 len)
{
	u32 j = 0;
	u32 ret = 0;
	u32 temp = 0;
	u32 phy_offset = phy->num; /* transciver register 0-2 */
	struct sunxi_phy_plat *sunxi_phy = phy->sunxi_phy;

	if (addr < 0x60) /* common register */
		phy_offset = 3;

	/*VC_EN enable*/
	temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
	temp |= VC_EN;
	writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

	for (j = len; j > 0; j--) {
		/*set write address*/
		temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
		temp &= ~VC_ADDR;//clear
		temp |= FIELD_PREP(VC_ADDR, addr + j - 1);  // write
		writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

		/*delsy 1us*/
		udelay(1);

		/*read data from VC_DO*/
		temp = readl(USBC_REG_PHY_STATUS(sunxi_phy->phy_reg, phy_offset));
		ret <<= 1;
		ret |= temp & VC_DO;
	}

	/*VC_EN disable*/
	temp = readl(USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));
	temp &= ~VC_EN;
	writel(temp, USBC_REG_PHY_CTRL(sunxi_phy->phy_reg, phy_offset));

	return ret;
}

static int sunxi_phy_set_calibrate(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);

	switch (uphy->mode) {
	case PHY_MODE_USB_DEVICE:
	case PHY_MODE_USB_DEVICE_LS:
	case PHY_MODE_USB_DEVICE_FS:
	case PHY_MODE_USB_DEVICE_HS:
	case PHY_MODE_USB_DEVICE_SS:
	{
		sunxi_phy_VCbus_write(uphy, 0xb2, 0, 1); //rpd_dis_en
		sunxi_phy_VCbus_write(uphy, 0xb8, 1, 1); //tm_dp_rdp_en
		sunxi_phy_VCbus_write(uphy, 0xb9, 1, 1); //se_dp_rdp_en
		sunxi_phy_VCbus_write(uphy, 0xba, 0, 1); //tm_dm_rdp_en
		sunxi_phy_VCbus_write(uphy, 0xbb, 1, 1); //se_dm_rdp_en

		dev_info(uphy->sunxi_phy->dev, "[phy%d] PHY calibrate set "
			"%x:%x %x:%x %x:%x %x:%x %x:%x\n", uphy->num,
			0xb2, sunxi_phy_VCbus_read(uphy, 0xb2, 1),
			0xb8, sunxi_phy_VCbus_read(uphy, 0xb8, 1),
			0xb9, sunxi_phy_VCbus_read(uphy, 0xb9, 1),
			0xba, sunxi_phy_VCbus_read(uphy, 0xba, 1),
			0xbb, sunxi_phy_VCbus_read(uphy, 0xbb, 1));

		break;
	}
	default:
		/* TODO */
		break;
	}
	return 0;
}

static int sunxi_phy_set_mode(struct phy *_phy, enum phy_mode mode, int submode)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);

	uphy->mode = mode;
	sunxi_phy_set_calibrate(_phy);

	dev_dbg(uphy->sunxi_phy->dev, "[phy%d] PHY mode set %x\n", uphy->num, mode);

	return 0;
}


static int sunxi_phy0_init(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);
	struct sunxi_phy_plat *sunxi_phy = uphy->sunxi_phy;
	int ret;

	if (uphy->ref_clk) {
		ret = clk_prepare_enable(uphy->ref_clk);
		if (ret) {
			dev_err(sunxi_phy->dev, "%s enable ref_clk err, return %d\n",
				uphy->name, ret);
			return ret;
		}
	}

	sunxi_phy_set(uphy, true);

	return 0;
}

static int sunxi_phy0_exit(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);

	sunxi_phy_set(uphy, false);

	if (uphy->ref_clk)
		clk_disable_unprepare(uphy->ref_clk);

	return 0;
}

static const struct phy_ops sunxi_phy0_ops = {
	.init		= sunxi_phy0_init,
	.exit		= sunxi_phy0_exit,
	.set_mode	= sunxi_phy_set_mode,
	.owner		= THIS_MODULE,
};

static int sunxi_phy1_init(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);
	struct sunxi_phy_plat *sunxi_phy = uphy->sunxi_phy;
	int ret;

	if (uphy->ref_clk) {
		ret = clk_prepare_enable(uphy->ref_clk);
		if (ret) {
			dev_err(sunxi_phy->dev, "%s enable ref_clk err, return %d\n",
				uphy->name, ret);
			return ret;
		}
	}

	sunxi_phy_set(uphy, true);

	return 0;
}

static int sunxi_phy1_exit(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);

	sunxi_phy_set(uphy, false);

	if (uphy->ref_clk)
		clk_disable_unprepare(uphy->ref_clk);

	return 0;
}

static const struct phy_ops sunxi_phy1_ops = {
	.init		= sunxi_phy1_init,
	.exit		= sunxi_phy1_exit,
	.set_mode	= sunxi_phy_set_mode,
	.owner		= THIS_MODULE,
};

static int sunxi_phy2_init(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);
	struct sunxi_phy_plat *sunxi_phy = uphy->sunxi_phy;
	int ret;

	if (uphy->ref_clk) {
		ret = clk_prepare_enable(uphy->ref_clk);
		if (ret) {
			dev_err(sunxi_phy->dev, "%s enable ref_clk err, return %d\n",
				uphy->name, ret);
			return ret;
		}
	}

	if (!sunxi_phy->u3_only_quirk)
		sunxi_phy_set(uphy, true);

	if (uphy->u2_only_clk) {
		ret = clk_set_rate(uphy->u2_only_clk, 120000000);
		if (ret) {
			dev_err(sunxi_phy->dev, "set u2_only_clk rate 120MHz err, return %d\n", ret);
			return ret;
		}

		ret = clk_prepare_enable(uphy->u2_only_clk);
		if (ret) {
			dev_err(sunxi_phy->dev, "%s enable u2_only_clk err, return %d\n",
				uphy->name, ret);
			return ret;
		}
	}

	return 0;
}

static int sunxi_phy2_exit(struct phy *_phy)
{
	struct sunxi_phy *uphy = phy_get_drvdata(_phy);

	sunxi_phy_set(uphy, false);

	if (uphy->u2_only_clk)
		clk_disable_unprepare(uphy->u2_only_clk);

	if (uphy->ref_clk)
		clk_disable_unprepare(uphy->ref_clk);

	return 0;
}

static const struct phy_ops sunxi_phy2_ops = {
	.init		= sunxi_phy2_init,
	.exit		= sunxi_phy2_exit,
	.set_mode	= sunxi_phy_set_mode,
	.owner		= THIS_MODULE,
};

int sunxi_phy_plat_create(struct device *dev, struct device_node *np,
			  struct sunxi_phy *uphy, enum sunxi_phy_type_e type)
{
	struct sunxi_phy_plat *sunxi_phy = dev_get_drvdata(dev);
	const struct phy_ops *ops;
	int ret;

	switch (type) {
	case SUNXI_PHY0_TYPE:
		uphy->num = USB_XCVR0_PHY_NO;
		uphy->name = kstrdup_const("u2-phy0", GFP_KERNEL);
		ops = &sunxi_phy0_ops;
		break;
	case SUNXI_PHY1_TYPE:
		uphy->num = USB_XCVR1_PHY_NO;
		uphy->name = kstrdup_const("u2-phy1", GFP_KERNEL);
		ops = &sunxi_phy1_ops;
		break;
	case SUNXI_PHY2_TYPE:
		uphy->num = USB_XCVR2_PHY_NO;
		uphy->name = kstrdup_const("u2-phy2", GFP_KERNEL);
		ops = &sunxi_phy2_ops;
		break;
	default:
		pr_err("not support phy type (%d)\n", type);
		return -EINVAL;
	}

	uphy->ref_clk = devm_get_clk_from_child(dev, np, "ref_clk");
	if (IS_ERR(uphy->ref_clk)) {
		uphy->ref_clk = NULL;
		pr_debug("Maybe there is no ref clk for phy (%s)\n", uphy->name);
	}

	uphy->u2_only_clk = devm_get_clk_from_child(dev, np, "u2_only_clk");
	if (IS_ERR(uphy->u2_only_clk)) {
		uphy->u2_only_clk = NULL;
		pr_debug("Maybe there is no u2 only pipe clk for phy (%s)\n", uphy->name);
	}

	uphy->phy = devm_phy_create(dev, np, ops);
	if (IS_ERR(uphy->phy)) {
		ret = PTR_ERR(uphy->phy);
		dev_err(dev, "failed to create phy for %s, ret: %d\n", uphy->name, ret);
		return ret;
	}

	uphy->sunxi_phy = sunxi_phy;
	phy_set_drvdata(uphy->phy, uphy);

	return 0;
}

static struct phy *sunxi_phy_plat_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct phy *phy = NULL;

	phy = of_phy_simple_xlate(dev, args);
	if (IS_ERR(phy)) {
		pr_err("%s fail\n", __func__);
		return phy;
	}

	/* TODO: if need */

	return phy;
}

static int sunxi_phy_plat_init(struct sunxi_phy_plat *sunxi_phy)
{
	int ret;

	if (sunxi_phy->pclk) {
		ret = clk_prepare_enable(sunxi_phy->pclk);
		if (ret) {
			dev_err(sunxi_phy->dev, "enable ahb master clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_phy->reset) {
		ret = reset_control_deassert(sunxi_phy->reset);
		if (ret) {
			dev_err(sunxi_phy->dev, "reset bus err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_phy->mclk) {
		ret = clk_prepare_enable(sunxi_phy->mclk);
		if (ret) {
			dev_err(sunxi_phy->dev, "enable ahb clk err, return %d\n", ret);
			return ret;
		}
	}

	if (sunxi_phy->usb_reset) {
		ret = reset_control_deassert(sunxi_phy->usb_reset);
		if (ret) {
			dev_err(sunxi_phy->dev, "reset usb err, return %d\n", ret);
			return ret;
		}
	}

	sunxi_phy->vernum = phy_ver_get(sunxi_phy);

	return 0;
}

static void sunxi_phy_plat_exit(struct sunxi_phy_plat *sunxi_phy)
{
	if (sunxi_phy->usb_reset)
		reset_control_assert(sunxi_phy->usb_reset);

	if (sunxi_phy->mclk)
		clk_disable_unprepare(sunxi_phy->mclk);

	if (sunxi_phy->reset)
		reset_control_assert(sunxi_phy->reset);

	if (sunxi_phy->pclk)
		clk_disable_unprepare(sunxi_phy->pclk);
}

static int sunxi_phy_plat_parse_dt(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_phy_plat *sunxi_phy = dev_get_drvdata(dev);
	struct device_node *child;
	int ret;

	/* parse top register, which determide general configuration such as mode */
	sunxi_phy->phy_reg = devm_platform_ioremap_resource_byname(pdev, "phy_base");
	if (IS_ERR(sunxi_phy->phy_reg))
		return PTR_ERR(sunxi_phy->phy_reg);

	sunxi_phy->top_reg = devm_platform_ioremap_resource_byname(pdev, "top_base");
	if (IS_ERR(sunxi_phy->top_reg))
		return PTR_ERR(sunxi_phy->top_reg);

	sunxi_phy->pclk = devm_clk_get(dev, "pclk");
	if (IS_ERR(sunxi_phy->pclk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_phy->pclk),
				     "failed to get ahb master clock for sunxi phy\n");
	}

	sunxi_phy->mclk = devm_clk_get(dev, "mclk");
	if (IS_ERR(sunxi_phy->mclk)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_phy->mclk),
				     "failed to get ahb clock for sunxi phy\n");
	}

	sunxi_phy->reset = devm_reset_control_get_shared(dev, "rst");
	if (IS_ERR(sunxi_phy->reset)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_phy->reset),
				     "failed to get reset for sunxi phy\n");
	}

	sunxi_phy->usb_reset = devm_reset_control_get_shared(dev, "usb_rst");
	if (IS_ERR(sunxi_phy->usb_reset)) {
		return dev_err_probe(dev, PTR_ERR(sunxi_phy->usb_reset),
				     "failed to get usb reset for sunxi phy\n");
	}

	sunxi_phy->u3_only_quirk = device_property_read_bool(dev, "aw,u3-only-quirk");


	for_each_available_child_of_node(dev->of_node, child) {
		if (of_node_name_eq(child, "u2-phy0")) {
			sunxi_phy->uphy0 = devm_kzalloc(dev, sizeof(*sunxi_phy->uphy0), GFP_KERNEL);
			if (!sunxi_phy->uphy0)
				return -ENOMEM;

			/* create u2 phy0 */
			ret = sunxi_phy_plat_create(dev, child, sunxi_phy->uphy0, SUNXI_PHY0_TYPE);
			if (ret) {
				dev_err(dev, "failed to create u2phy0, ret:%d\n", ret);
				goto err_node_put;
			}

		} else if (of_node_name_eq(child, "u2-phy1")) {
			sunxi_phy->uphy1 = devm_kzalloc(dev, sizeof(*sunxi_phy->uphy1), GFP_KERNEL);
			if (!sunxi_phy->uphy1)
				return -ENOMEM;

			/* create u2 phy1 */
			ret = sunxi_phy_plat_create(dev, child, sunxi_phy->uphy1, SUNXI_PHY1_TYPE);
			if (ret) {
				dev_err(dev, "failed to create u2phy1, ret:%d\n", ret);
				goto err_node_put;
			}

		} else if (of_node_name_eq(child, "u2-phy2")) {
			sunxi_phy->uphy2 = devm_kzalloc(dev, sizeof(*sunxi_phy->uphy2), GFP_KERNEL);
			if (!sunxi_phy->uphy2)
				return -ENOMEM;

			/* create u2 phy2 */
			ret = sunxi_phy_plat_create(dev, child, sunxi_phy->uphy2, SUNXI_PHY2_TYPE);
			if (ret) {
				dev_err(dev, "failed to create u2phy2, ret:%d\n", ret);
				goto err_node_put;
			}
		}
	}

	return 0;

err_node_put:
	of_node_put(child);

	return ret;
}

static int sunxi_phy_plat_probe(struct platform_device *pdev)
{
	struct sunxi_phy_plat *sunxi_phy;
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	int ret;

	sunxi_phy = devm_kzalloc(dev, sizeof(*sunxi_phy), GFP_KERNEL);
	if (!sunxi_phy)
		return -ENOMEM;

	sunxi_phy->dev = dev;
	dev_set_drvdata(dev, sunxi_phy);

	ret = sunxi_phy_plat_parse_dt(pdev);
	if (ret)
		return -EINVAL;

	phy_provider = devm_of_phy_provider_register(dev, sunxi_phy_plat_xlate);
	if (IS_ERR(phy_provider))
		return PTR_ERR_OR_ZERO(phy_provider);

	ret = sunxi_phy_plat_init(sunxi_phy);
	if (ret)
		return -EINVAL;

	dev_info(dev, "Allwinner USB2.0 PHY Version v%lu.%lu.%lu\n",
		 FIELD_GET(PHY_GEN_VER, sunxi_phy->vernum),
		 FIELD_GET(PHY_SUB_VER, sunxi_phy->vernum),
		 FIELD_GET(PHY_PRJ_VER, sunxi_phy->vernum));

	return 0;
}

static int sunxi_phy_plat_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_phy_plat *sunxi_phy = dev_get_drvdata(dev);

	sunxi_phy_plat_exit(sunxi_phy);

	return 0;
}

static int __maybe_unused sunxi_phy_plat_suspend(struct device *dev)
{
	struct sunxi_phy_plat *sunxi_phy = dev_get_drvdata(dev);

	sunxi_phy_plat_exit(sunxi_phy);

	return 0;
}

static int __maybe_unused sunxi_phy_plat_resume(struct device *dev)
{
	struct sunxi_phy_plat *sunxi_phy = dev_get_drvdata(dev);
	int ret;

	ret = sunxi_phy_plat_init(sunxi_phy);
	if (ret) {
		dev_err(dev, "failed to resume awphy\n");
		return ret;
	}

	return 0;
}

static struct dev_pm_ops sunxi_phy_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(sunxi_phy_plat_suspend, sunxi_phy_plat_resume)
};

static const struct of_device_id sunxi_phy_plat_of_match_table[] = {
	{ .compatible = "allwinner,sunxi-awphy-v100", },
	{ /* Sentinel */ }
};

static struct platform_driver sunxi_phy_plat_driver = {
	.probe		= sunxi_phy_plat_probe,
	.remove		= sunxi_phy_plat_remove,
	.driver = {
		.name	= "sunxi-plat-awphy",
		.pm	= &sunxi_phy_plat_pm_ops,
		.of_match_table = sunxi_phy_plat_of_match_table,
	},
};
module_platform_driver(sunxi_phy_plat_driver);

MODULE_ALIAS("platform:sunxi-plat-awphy");
MODULE_DESCRIPTION("Allwinner Platform USB2.0 AW PHY driver");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.4");
