/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI NSI ECC driver
 *
 * Copyright (C) 2015 AllWinnertech Ltd.
 * Author: panzhijian <panzhijian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <sunxi-sid.h>
#include <asm/io.h>
#include "sunxi-nsi.h"

#define TAG_N_ECC_REG_OFFSET            (0x200 * MBUS_PMU_TAG)

#define TAG_N_ECC_ERR                   (0x18 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_ROB_ECC_ERR               BIT(0)
#define TAG_N_DMX_ECC_ERR               BIT(3)

#define TAG_N_MISC                      (0xA8 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_INJECT_EN_ECO             BIT(7)

#define TAG_N_ROB_ECC_CTRL              (0x150 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_ROB_ECC_EN                BIT(8)
#define TAG_N_ROB_ECC_INJECT_EN         BIT(0)
#define TAG_N_ROB_ECC_INJECT_TYPE_MASK  (0x3 << 6)
#define TAG_N_ROB_ECC_INJECT_MODE_MASK  (0x3 << 4)
#define TAG_N_ROB_ECC_IRQ_EN            BIT(16)

#define TAG_N_ROB_ECC_IRQ_CLR           (0x154 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_ROB_ECC_IRQ_CLR_CTRL      BIT(0)

#define TAG_N_ROB_ECC_INT_STA           (0x158 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_ROB_ECC_SINGLE_BIT_ERR    BIT(21)
#define TAG_N_ROB_ECC_DOUB_BIT_ERR      BIT(22)
#define TAG_N_ROB_ECC_CHECK_CODE_ERR    BIT(23)
#define TAG_N_ROB_ECC_ERR_TYPE_MASK     (0x7 << 21)

#define TAG_N_ROB_ECC_DATA_NUM          (4)
#define TAG_N_ROB_ECC_INJ_DATA_BASE     (0x160 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_ROB_ECC_ORI_DATA_BASE     (0x180 + TAG_N_ECC_REG_OFFSET)

#define TAG_N_DMX_ECC_CTRL              (0x1A0 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_DMX_ECC_EN                BIT(8)
#define TAG_N_DMX_ECC_INJECT_EN         BIT(0)
#define TAG_N_DMX_ECC_INJECT_TYPE_MASK  (0x3 << 6)
#define TAG_N_DMX_ECC_INJECT_MODE_MASK  (0x3 << 4)
#define TAG_N_DMX_ECC_IRQ_EN            BIT(16)

#define TAG_N_DMX_ECC_IRQ_CLR           (0x1A4 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_DMX_ECC_IRQ_CLR_CTRL      BIT(0)

#define TAG_N_DMX_ECC_INT_STA           (0x1A8 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_DMX_ECC_SINGLE_BIT_ERR    BIT(21)
#define TAG_N_DMX_ECC_DOUB_BIT_ERR      BIT(22)
#define TAG_N_DMX_ECC_CHECK_CODE_ERR    BIT(23)
#define TAG_N_DMX_ECC_ERR_TYPE_MASK     (0x7 << 21)

#define TAG_N_DMX_ECC_DATA_NUM          (5)
#define TAG_N_DMX_ECC_INJ_DATA_BASE     (0x1B0 + TAG_N_ECC_REG_OFFSET)
#define TAG_N_DMX_ECC_ORI_DATA_BASE     (0x1D0 + TAG_N_ECC_REG_OFFSET)

static irqreturn_t sunxi_nsi_handler(int irq, void *dev_id)
{
	struct nsi_bus *p_sunxi_nsi = (struct nsi_bus *)dev_id;
	volatile u32 err_module = 0x0;
	volatile u32 val = 0x0;
	static u32 test_cnt;

	++test_cnt;
	err_module = readl_relaxed(p_sunxi_nsi->base + TAG_N_ECC_ERR);
	pr_debug("nsi isr, test cnt: %u, 0x%x\n", test_cnt, err_module);
	if (err_module & TAG_N_ROB_ECC_ERR) {
		val = readl_relaxed(p_sunxi_nsi->base + TAG_N_ROB_ECC_INT_STA);
		pr_debug("ROB ecc ini sta reg: 0x%08x\n", val);

		if (val & TAG_N_ROB_ECC_SINGLE_BIT_ERR)
			pr_debug("It is single bit err for rob!\n");
		else if (val & TAG_N_ROB_ECC_DOUB_BIT_ERR)
			pr_debug("It is doub bit err for rob!\n");
		else
			pr_warn("Unknow err for rob!\n");

		if (test_cnt > 10) {
			pr_debug("rob disable inject\n");
			val = readl_relaxed(sunxi_nsi.base + TAG_N_ROB_ECC_CTRL);
			val &= (~TAG_N_ROB_ECC_INJECT_EN);
			writel_relaxed(val, sunxi_nsi.base + TAG_N_ROB_ECC_CTRL);
		}

		val = readl_relaxed(p_sunxi_nsi->base + TAG_N_ROB_ECC_IRQ_CLR);
		val |= TAG_N_ROB_ECC_IRQ_CLR_CTRL;
		writel_relaxed(val, p_sunxi_nsi->base + TAG_N_ROB_ECC_IRQ_CLR);
		val &= (~TAG_N_ROB_ECC_IRQ_CLR_CTRL);
		writel_relaxed(val, p_sunxi_nsi->base + TAG_N_ROB_ECC_IRQ_CLR);

		val = readl_relaxed(p_sunxi_nsi->base + TAG_N_ECC_ERR);
		val |= TAG_N_ROB_ECC_ERR;
		writel_relaxed(val, p_sunxi_nsi->base + TAG_N_ECC_ERR);
	}

	if (err_module & TAG_N_DMX_ECC_ERR) {
		val = readl_relaxed(p_sunxi_nsi->base + TAG_N_DMX_ECC_INT_STA);
		pr_debug("DMX ecc ini sta reg: 0x%08x\n", val);
		if (val & TAG_N_DMX_ECC_SINGLE_BIT_ERR)
			pr_debug("It is single bit err for dmx!\n");
		else if (val & TAG_N_DMX_ECC_DOUB_BIT_ERR)
			pr_debug("It is doub bits err for dmx!\n");
		else
			pr_warn("Unknow err for dmx!\n");

		if (test_cnt > 10) {
			pr_debug("dmx disable inject\n");
			val = readl_relaxed(sunxi_nsi.base + TAG_N_DMX_ECC_CTRL);
			val &= (~TAG_N_DMX_ECC_INJECT_EN);
			writel_relaxed(val, sunxi_nsi.base + TAG_N_DMX_ECC_CTRL);
		}

		val = readl_relaxed(p_sunxi_nsi->base + TAG_N_DMX_ECC_IRQ_CLR);
		val |= TAG_N_DMX_ECC_IRQ_CLR_CTRL;
		writel_relaxed(val, p_sunxi_nsi->base + TAG_N_DMX_ECC_IRQ_CLR);
		val &= (~TAG_N_DMX_ECC_IRQ_CLR_CTRL);
		writel_relaxed(val, p_sunxi_nsi->base + TAG_N_DMX_ECC_IRQ_CLR);

		val = readl_relaxed(p_sunxi_nsi->base + TAG_N_ECC_ERR);
		val |= TAG_N_DMX_ECC_ERR;
		writel_relaxed(val, p_sunxi_nsi->base + TAG_N_ECC_ERR);
	}

	return IRQ_HANDLED;
}

__weak int sunxi_sid_get_ecc_status(void)
{
	return 0;
}

static bool sunxi_is_support_ecc(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	bool is_support_ecc = false;
	u32 support_ecc = 0;
	int ret  = 0;

	ret = of_property_read_u32(np, "support-ecc", &support_ecc);
	if (ret) {
		dev_warn(&pdev->dev, "Get support-ecc failed\n");
		is_support_ecc = false;
		return is_support_ecc;
	}

	is_support_ecc = support_ecc ? true : false;
	if (!is_support_ecc) {
		dev_warn(&pdev->dev, "nsi not support ecc\n");
		return is_support_ecc;
	}

	is_support_ecc = sunxi_sid_get_ecc_status() ? true : false;
	if (!is_support_ecc) {
		dev_warn(&pdev->dev, "nsi not support ecc(efuse)\n");
		return is_support_ecc;
	}

	return is_support_ecc;
}

int sunxi_nsi_ecc_init(struct platform_device *pdev)
{
	volatile u32 val  = 0x0;
	int ret = 0;

	if (!sunxi_is_support_ecc(pdev))
		return -ENODEV;

	sunxi_nsi.irq = platform_get_irq(pdev, 0);
	if (sunxi_nsi.irq < 0) {
		ret = sunxi_nsi.irq;
		return ret;
	}

	ret = devm_request_irq(&pdev->dev, sunxi_nsi.irq, sunxi_nsi_handler, 0,
		dev_name(&pdev->dev), &sunxi_nsi);
	if (ret) {
		dev_err(&pdev->dev, "nsi could not request irq!\n");
		return ret;
	}

	val = readl_relaxed(sunxi_nsi.base + TAG_N_ROB_ECC_CTRL);
	val |= TAG_N_ROB_ECC_IRQ_EN;
	writel_relaxed(val, sunxi_nsi.base + TAG_N_ROB_ECC_CTRL);

	val = readl_relaxed(sunxi_nsi.base + TAG_N_DMX_ECC_CTRL);
	val |= TAG_N_DMX_ECC_IRQ_EN;
	writel_relaxed(val, sunxi_nsi.base + TAG_N_DMX_ECC_CTRL);

	val = readl_relaxed(sunxi_nsi.base + TAG_N_MISC);
	val |= TAG_N_INJECT_EN_ECO;
	writel_relaxed(val, sunxi_nsi.base + TAG_N_MISC);
	dev_dbg(&pdev->dev, "nsi ecc init succeed\n");

	return ret;
}

void sunxi_nsi_ecc_exit(struct platform_device *pdev)
{
	if (!sunxi_is_support_ecc(pdev))
		return;

	if (sunxi_nsi.irq >= 0) {
		devm_free_irq(&pdev->dev, sunxi_nsi.irq, &sunxi_nsi);
	}
}
