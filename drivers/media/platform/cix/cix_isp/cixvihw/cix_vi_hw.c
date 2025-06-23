// SPDX-License-Identifier: GPL-2.0
/*
 * Set hardware register information of VI for Cix sky SOC
 *
 * Copyright (c) 2024 CIX Semiconductor
 *
 */
#include "cix_vi_hw.h"
#include "armcb_register.h"
#include "system_logger.h"
#include <linux/acpi.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/memory.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/reset.h>

#define CIX_VI_HW_NAME "cix-vi-hw"
#define CSI_NUM 4
#define CSI0_PIXEL_NUM 4
#define CSI2_PIXEL_NUM 4
#define DPHY_NUM 2
#define CSIDMA_NUM 4

#define POWER_ON 1
#define POWER_OFF 0

int dphy_power_status[DPHY_NUM] = { 0, 0 };
int csi_power_status[CSI_NUM] = { 0, 0, 0, 0 };
int csidma_power_status[CSIDMA_NUM] = { 0, 0, 0, 0 };

struct cix_vi_hw_dev *cix_vi_hw_info;

void __iomem *cix_ahb_dphy_get_addr_base(int id)
{
	return cix_vi_hw_info->ahb_dphy_base_addrs[id];
}

void __iomem *cix_ahb_csi_get_addr_base(int id)
{
	return cix_vi_hw_info->ahb_csi_base_addrs[id];
}

void __iomem *cix_ahb_csidma_get_addr_base(int id)
{
	return cix_vi_hw_info->ahb_csidma_base_addrs[id];
}

void __iomem *cix_ahb_csircsu_get_addr_base(int id)
{
	return cix_vi_hw_info->ahb_csircsu_base_addrs[id];
}

u32 cix_ahb_dphy_read_reg(u32 reg_addr)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;
	u32 offset;
	u16 phy_id;

	if ((reg_addr >= AHB_DPHY0_REG_BASE) &&
		(reg_addr <= AHB_DPHY0_REG_END)) {
		offset = reg_addr - AHB_DPHY0_REG_BASE;
		phy_id = 0;
	} else if ((reg_addr >= AHB_DPHY1_REG_BASE) &&
		   (reg_addr <= AHB_DPHY1_REG_END)) {
		offset = reg_addr - AHB_DPHY1_REG_BASE;
		phy_id = 1;
	} else {
		LOG(LOG_ERR, "dphy addr :0x%x    is error!", reg_addr);
		return -1;
	}

	virt_addr = cix_ahb_dphy_get_addr_base(phy_id);
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void cix_ahb_dphy_write_reg(u32 reg_addr, u32 value)
{
	void __iomem *virt_addr = NULL;
	u32 offset;
	u16 phy_id;

	if ((reg_addr >= AHB_DPHY0_REG_BASE) &&
		(reg_addr <= AHB_DPHY0_REG_END)) {
		offset = reg_addr - AHB_DPHY0_REG_BASE;
		phy_id = 0;
	} else if ((reg_addr >= AHB_DPHY1_REG_BASE) &&
		   (reg_addr <= AHB_DPHY1_REG_END)) {
		offset = reg_addr - AHB_DPHY1_REG_BASE;
		phy_id = 1;
	} else {
		LOG(LOG_ERR, "dphy addr :0x%x    is error!", reg_addr);
		return;
	}

	virt_addr = cix_ahb_dphy_get_addr_base(phy_id);
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}

u32 cix_ahb_csi_read_reg(u32 reg_addr)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;
	u32 offset;
	u16 csi_id;

	if ((reg_addr >= AHB_CSI0_REG_BASE) && (reg_addr <= AHB_CSI0_REG_END)) {
		offset = reg_addr - AHB_CSI0_REG_BASE;
		csi_id = 0;
	} else if ((reg_addr >= AHB_CSI1_REG_BASE) &&
		   (reg_addr <= AHB_CSI1_REG_END)) {
		offset = reg_addr - AHB_CSI1_REG_BASE;
		csi_id = 1;
	} else if ((reg_addr >= AHB_CSI2_REG_BASE) &&
		   (reg_addr <= AHB_CSI2_REG_END)) {
		offset = reg_addr - AHB_CSI2_REG_BASE;
		csi_id = 2;
	} else if ((reg_addr >= AHB_CSI3_REG_BASE) &&
		   (reg_addr <= AHB_CSI3_REG_END)) {
		offset = reg_addr - AHB_CSI3_REG_BASE;
		csi_id = 3;
	} else {
		LOG(LOG_ERR, "csi addr :0x%x    is error!", reg_addr);
		return -1;
	}

	virt_addr = cix_ahb_csi_get_addr_base(csi_id);
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void cix_ahb_csi_write_reg(u32 reg_addr, u32 value)
{
	void __iomem *virt_addr = NULL;
	u32 offset;
	u16 csi_id;

	if ((reg_addr >= AHB_CSI0_REG_BASE) && (reg_addr <= AHB_CSI0_REG_END)) {
		offset = reg_addr - AHB_CSI0_REG_BASE;
		csi_id = 0;
	} else if ((reg_addr >= AHB_CSI1_REG_BASE) &&
		   (reg_addr <= AHB_CSI1_REG_END)) {
		offset = reg_addr - AHB_CSI1_REG_BASE;
		csi_id = 1;
	} else if ((reg_addr >= AHB_CSI2_REG_BASE) &&
		   (reg_addr <= AHB_CSI2_REG_END)) {
		offset = reg_addr - AHB_CSI2_REG_BASE;
		csi_id = 2;
	} else if ((reg_addr >= AHB_CSI3_REG_BASE) &&
		   (reg_addr <= AHB_CSI3_REG_END)) {
		offset = reg_addr - AHB_CSI3_REG_BASE;
		csi_id = 3;
	} else {
		LOG(LOG_ERR, "csi addr :0x%x    is error!", reg_addr);
		return;
	}

	virt_addr = cix_ahb_csi_get_addr_base(csi_id);
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}

u32 cix_ahb_csidma_read_reg(u32 reg_addr)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;
	u32 offset;
	u16 csidma_id;

	if ((reg_addr >= AHB_CSIDMA0_REG_BASE) &&
		(reg_addr <= AHB_CSIDMA0_REG_END)) {
		offset = reg_addr - AHB_CSIDMA0_REG_BASE;
		csidma_id = 0;
	} else if ((reg_addr >= AHB_CSIDMA1_REG_BASE) &&
		   (reg_addr <= AHB_CSIDMA1_REG_END)) {
		offset = reg_addr - AHB_CSIDMA1_REG_BASE;
		csidma_id = 1;
	} else if ((reg_addr >= AHB_CSIDMA2_REG_BASE) &&
		   (reg_addr <= AHB_CSIDMA2_REG_END)) {
		offset = reg_addr - AHB_CSIDMA2_REG_BASE;
		csidma_id = 2;
	} else if ((reg_addr >= AHB_CSIDMA3_REG_BASE) &&
		   (reg_addr <= AHB_CSIDMA3_REG_END)) {
		offset = reg_addr - AHB_CSIDMA3_REG_BASE;
		csidma_id = 3;
	} else {
		LOG(LOG_ERR, "csidma addr :0x%x    is error!", reg_addr);
		return -1;
	}

	virt_addr = cix_ahb_csidma_get_addr_base(csidma_id);
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void cix_ahb_csidma_write_reg(u32 reg_addr, u32 value)
{
	void __iomem *virt_addr = NULL;
	u32 offset;
	u16 csidma_id;

	if ((reg_addr >= AHB_CSIDMA0_REG_BASE) &&
		(reg_addr <= AHB_CSIDMA0_REG_END)) {
		offset = reg_addr - AHB_CSIDMA0_REG_BASE;
		csidma_id = 0;
	} else if ((reg_addr >= AHB_CSIDMA1_REG_BASE) &&
		   (reg_addr <= AHB_CSIDMA1_REG_END)) {
		offset = reg_addr - AHB_CSIDMA1_REG_BASE;
		csidma_id = 1;
	} else if ((reg_addr >= AHB_CSIDMA2_REG_BASE) &&
		   (reg_addr <= AHB_CSIDMA2_REG_END)) {
		offset = reg_addr - AHB_CSIDMA2_REG_BASE;
		csidma_id = 2;
	} else if ((reg_addr >= AHB_CSIDMA3_REG_BASE) &&
		   (reg_addr <= AHB_CSIDMA3_REG_END)) {
		offset = reg_addr - AHB_CSIDMA3_REG_BASE;
		csidma_id = 3;
	} else {
		LOG(LOG_ERR, "csidma addr :0x%x    is error!", reg_addr);
		return;
	}

	virt_addr = cix_ahb_csidma_get_addr_base(csidma_id);
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}

u32 cix_ahb_csircsu_read_reg(u32 reg_addr)
{
	void __iomem *virt_addr = NULL;
	u32 reg_val = 0;
	u32 offset;
	u16 csircsu_id;

	if ((reg_addr >= AHB_CSIRCSU0_REG_BASE) &&
		(reg_addr <= AHB_CSIRCSU0_REG_END)) {
		offset = reg_addr - AHB_CSIRCSU0_REG_BASE;
		csircsu_id = 0;
	} else if ((reg_addr >= AHB_CSIRCSU1_REG_BASE) &&
		   (reg_addr <= AHB_CSIRCSU1_REG_END)) {
		offset = reg_addr - AHB_CSIRCSU1_REG_BASE;
		csircsu_id = 1;
	} else {
		LOG(LOG_ERR, "csircsu addr :0x%x    is error!", reg_addr);
		return -1;
	}

	virt_addr = cix_ahb_csircsu_get_addr_base(csircsu_id);
	/* Ensure read order to prevent hardware register access reordering */
	rmb();
	reg_val = readl(virt_addr + offset);

	return reg_val;
}

void cix_ahb_csircsu_write_reg(u32 reg_addr, u32 value)
{
	void __iomem *virt_addr = NULL;
	u32 offset;
	u16 csircsu_id;

	if ((reg_addr >= AHB_CSIRCSU0_REG_BASE) &&
		(reg_addr <= AHB_CSIRCSU0_REG_END)) {
		offset = reg_addr - AHB_CSIRCSU0_REG_BASE;
		csircsu_id = 0;
	} else if ((reg_addr >= AHB_CSIRCSU1_REG_BASE) &&
		   (reg_addr <= AHB_CSIRCSU1_REG_END)) {
		offset = reg_addr - AHB_CSIRCSU1_REG_BASE;
		csircsu_id = 1;
	} else {
		LOG(LOG_ERR, "csircsu addr :0x%x    is error!", reg_addr);
		return;
	}

	virt_addr = cix_ahb_csircsu_get_addr_base(csircsu_id);
	/* Ensure write order to prevent hardware register access reordering */
	wmb();
	writel(value, virt_addr + offset);
}

void cix_dphy_resets(int id, int enable)
{
	int ret;

	if (enable) {
		ret = reset_control_deassert(cix_vi_hw_info->rst_dphy[id]);
		if (ret)
			LOG(LOG_ERR, "First: Failed to deassert rst_dphy %d\n",
				id);

		ret = reset_control_deassert(cix_vi_hw_info->cmnrst_phy[id]);
		if (ret)
			LOG(LOG_ERR,
				"First: Failed to deassert cmnrst_phy %d\n", id);

		ret = reset_control_assert(cix_vi_hw_info->rst_dphy[id]);
		if (ret)
			LOG(LOG_ERR, "Second: Failed to assert rst_dphy %d\n",
				id);

		ret = reset_control_assert(cix_vi_hw_info->cmnrst_phy[id]);
		if (ret)
			LOG(LOG_ERR, "Second: Failed to assert cmnrst_phy %d\n",
				id);

		ret = reset_control_deassert(cix_vi_hw_info->rst_dphy[id]);
		if (ret)
			LOG(LOG_ERR, "Last: Failed to deassert rst_dphy %d\n",
				id);

		ret = reset_control_deassert(cix_vi_hw_info->cmnrst_phy[id]);
		if (ret)
			LOG(LOG_ERR, "Last: Failed to deassert cmnrst_phy %d\n",
				id);
	} else {
		ret = reset_control_assert(cix_vi_hw_info->rst_dphy[id]);
		if (ret)
			LOG(LOG_ERR, "Failed to assert rst_dphy %d\n", id);

		ret = reset_control_assert(cix_vi_hw_info->cmnrst_phy[id]);
		if (ret)
			LOG(LOG_ERR, "Failed to assert cmnrst_phy %d\n", id);
	}
}

void cix_csi_resets(int id, int enable)
{
	int ret;

	if (enable) {
		ret = reset_control_deassert(cix_vi_hw_info->csi_reset[id]);
		if (ret)
			LOG(LOG_ERR, "First: Failed to deassert csi_reset %d\n",
				id);

		ret = reset_control_assert(cix_vi_hw_info->csi_reset[id]);
		if (ret)
			LOG(LOG_ERR,
				"Second: Failed to assert csi_reset %d before deassert\n",
				id);

		ret = reset_control_deassert(cix_vi_hw_info->csi_reset[id]);
		if (ret)
			LOG(LOG_ERR, "Last: Failed to deassert csi_reset %d\n",
				id);
	} else {
		ret = reset_control_assert(cix_vi_hw_info->csi_reset[id]);
		if (ret)
			LOG(LOG_ERR, "Failed to assert csi_reset %d\n", id);
	}
}

void cix_csidma_resets(int id, int enable)
{
	int ret;

	if (enable) {
		ret = reset_control_deassert(
			cix_vi_hw_info->csibridge_reset[id]);
		if (ret)
			LOG(LOG_ERR,
				"First: Failed to deassert csibridge_reset %d\n",
				id);

		ret = reset_control_assert(cix_vi_hw_info->csibridge_reset[id]);
		if (ret)
			LOG(LOG_ERR,
				"Second: Failed to assert csibridge_reset %d\n",
				id);

		ret = reset_control_deassert(
			cix_vi_hw_info->csibridge_reset[id]);
		if (ret)
			LOG(LOG_ERR,
				"Last: Failed to deassert csibridge_reset %d\n",
				id);
	} else {
		ret = reset_control_assert(cix_vi_hw_info->csibridge_reset[id]);
		if (ret)
			LOG(LOG_ERR, "Failed to assert csibridge_reset %d\n",
				id);
	}
}

void cix_enable_dphy_clk(u32 reg_addr, u32 value)
{
	u16 dhpy_id;
	int ret;

	dhpy_id = reg_addr - DPHY_POWER_BASE;
	if (value > 0) {
		ret = clk_prepare_enable(cix_vi_hw_info->phy_psm_clk[dhpy_id]);
		if (ret < 0) {
			LOG(LOG_ERR, "enable phy%d_psm_clk error",
				dhpy_id);
			goto err_phy_clks;
		}

		ret = clk_prepare_enable(cix_vi_hw_info->phy_apb_clk[dhpy_id]);
		if (ret < 0) {
			LOG(LOG_ERR, "enable phy%d_apb_clk error",
				dhpy_id);
			goto err_phy_clks;
		}
		cix_dphy_resets(dhpy_id, POWER_ON);
		dphy_power_status[dhpy_id] = POWER_ON;
	} else {
		if (dphy_power_status[dhpy_id] == POWER_ON) {
			clk_disable_unprepare(
				cix_vi_hw_info->phy_psm_clk[dhpy_id]);
			clk_disable_unprepare(
				cix_vi_hw_info->phy_apb_clk[dhpy_id]);
			cix_dphy_resets(dhpy_id, POWER_OFF);
		}
		dphy_power_status[dhpy_id] = POWER_OFF;
	}
	return;
err_phy_clks:
	LOG(LOG_ERR, "enable dphy%d  clk failed", dhpy_id);
}

void cix_set_csi_clk_rate(u32 reg_addr, u32 value)
{
	u16 csi_id;
	int i, ret;

	csi_id = reg_addr - CSI_POWER_BASE;
	if (value > 0) {
		LOG(LOG_INFO, "enable csi%d_sys_clk %d", csi_id, value);
		ret = clk_prepare_enable(cix_vi_hw_info->csi_sys_clk[csi_id]);
		if (ret < 0) {
			LOG(LOG_ERR, "enable csi%d_sys_clk error", csi_id);
			goto err_csi_clks;
		}

		ret = clk_prepare_enable(cix_vi_hw_info->csi_p_clk[csi_id]);
		if (ret < 0) {
			LOG(LOG_ERR, "enable csi%d_p_clk error", csi_id);
			goto err_csi_clks;
		}
		switch (csi_id) {
		case 0:
			for (i = 0; i < 4; i++) {
				if (clk_prepare_enable(
						cix_vi_hw_info->csi0_pixel_clk[i])) {
					LOG(LOG_ERR,
						"csi0_pixel_clk[%d] enable failed\n",
						i);
					goto err_csi_clks;
				}
			}
			break;
		case 1:
			if (clk_prepare_enable(
					cix_vi_hw_info->csi1_pixel_clk)) {
				LOG(LOG_ERR, "csi1_pixel_clk enable failed\n");
				goto err_csi_clks;
			}
			break;
		case 2:
			for (i = 0; i < 4; i++) {
				if (clk_prepare_enable(
						cix_vi_hw_info->csi2_pixel_clk[i])) {
					LOG(LOG_ERR,
						"csi2_pixel_clk[%d] enable failed\n",
						i);
					goto err_csi_clks;
				}
			}
			break;
		case 3:
			if (clk_prepare_enable(
					cix_vi_hw_info->csi3_pixel_clk)) {
				LOG(LOG_ERR,
					"csi3_pixel_clk[%d] enable failed\n");
				goto err_csi_clks;
			}
			break;
		}

		clk_set_rate(cix_vi_hw_info->csi_sys_clk[csi_id], value);
		cix_csi_resets(csi_id, POWER_ON);
		csi_power_status[csi_id] = POWER_ON;
	} else {
		if (csi_power_status[csi_id] == POWER_ON) {
			clk_disable_unprepare(
				cix_vi_hw_info->csi_sys_clk[csi_id]);
			clk_disable_unprepare(
				cix_vi_hw_info->csi_p_clk[csi_id]);
			switch (csi_id) {
			case 0:
				for (i = 0; i < 4; i++)
					clk_disable_unprepare(
						cix_vi_hw_info
							->csi0_pixel_clk[i]);
				break;
			case 1:
				clk_disable_unprepare(
					cix_vi_hw_info->csi1_pixel_clk);
				break;
			case 2:
				for (i = 0; i < 4; i++)
					clk_disable_unprepare(
						cix_vi_hw_info
							->csi2_pixel_clk[i]);
				break;
			case 3:
				clk_disable_unprepare(
					cix_vi_hw_info->csi3_pixel_clk);
				break;
			}
			cix_csi_resets(csi_id, POWER_OFF);
		}
		csi_power_status[csi_id] = POWER_OFF;
	}
	return;
err_csi_clks:
	LOG(LOG_ERR, "enable csi%d  clk failed", csi_id);
}
void cix_enable_csidma_clk(u32 reg_addr, u32 value)
{
	u16 csidma_id;
	int ret;

	csidma_id = reg_addr - CSIDMA_POWER_BASE;
	if (value > 0) {
		ret = clk_prepare_enable(
			cix_vi_hw_info->csidma_apbclk[csidma_id]);
		if (ret < 0)
			LOG(LOG_ERR, "enable csidma%d_apbclk error",
				csidma_id);
		cix_csidma_resets(csidma_id, POWER_ON);
		csidma_power_status[csidma_id] = POWER_ON;
	} else {
		if (csidma_power_status[csidma_id] == POWER_ON) {
			clk_disable_unprepare(
				cix_vi_hw_info->csidma_apbclk[csidma_id]);
			cix_csidma_resets(csidma_id, POWER_OFF);
		}
		csidma_power_status[csidma_id] = POWER_OFF;
	}
}

static irqreturn_t mipi_csi2_irq_handler(int irq, void *base)
{
	unsigned int status;

	status = readl(base + MIPI_INFO_IRQS);
	writel(status, base + MIPI_INFO_IRQS);

	pr_info("csi irq enter status %x\n", status);

	return IRQ_HANDLED;
}

static irqreturn_t mipi_csi2_err_irq_handler(int irq, void *base)
{
	unsigned int csi_status;
	unsigned int dphy_status;

	csi_status = readl(base + MIPI_ERROR_IRQS);
	writel(csi_status, base + MIPI_ERROR_IRQS);

	dphy_status = readl(base + DPHY_ERR_STATUS_IRQ);
	writel(dphy_status, base + DPHY_ERR_STATUS_IRQ);

	pr_info("CSI ERROR status 0x%08x, DPHY error status 0x%08x\n",
		csi_status, dphy_status);

	return IRQ_HANDLED;
}

static const struct of_device_id cix_vi_hw_of_match[] = {
	{
		.compatible = "cix,cix-vi-hw",
	},
	{}
};
MODULE_DEVICE_TABLE(of, cix_vi_hw_of_match);

static const struct acpi_device_id cix_vi_hw_acpi_match[] = {
	{ .id = "CIXH3026", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, cix_vi_hw_acpi_match);

static int cix_vi_hw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	int i;
	char buffer[64], clk_name[32], reset_name[32];
	int irq0, irq1;
	int ret;

	LOG(LOG_INFO, "cix vi hw reg  probe enter\n");

	cix_vi_hw_info =
		devm_kzalloc(dev, sizeof(struct cix_vi_hw_dev), GFP_KERNEL);
	if (!cix_vi_hw_info)
		return -ENOMEM;

	cix_vi_hw_info->dev = dev;

	for (i = 0; i < DPHY_NUM; i++) {
		sprintf(clk_name, "phy%d_psmclk", i);
		cix_vi_hw_info->phy_psm_clk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->phy_psm_clk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->phy_psm_clk[i]);
		}
	}
	for (i = 0; i < DPHY_NUM; i++) {
		sprintf(clk_name, "phy%d_apbclk", i);
		cix_vi_hw_info->phy_apb_clk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->phy_apb_clk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->phy_apb_clk[i]);
		}
	}
	for (i = 0; i < CSI_NUM; i++) {
		sprintf(clk_name, "csi%d_pclk", i);
		cix_vi_hw_info->csi_p_clk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->csi_p_clk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->csi_p_clk[i]);
		}
	}
	for (i = 0; i < CSI_NUM; i++) {
		sprintf(clk_name, "csi%d_sclk", i);
		cix_vi_hw_info->csi_sys_clk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->csi_sys_clk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->csi_sys_clk[i]);
		}
	}
	for (i = 0; i < CSI0_PIXEL_NUM; i++) {
		sprintf(clk_name, "csi0_p%dclk", i);
		cix_vi_hw_info->csi0_pixel_clk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->csi0_pixel_clk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->csi0_pixel_clk[i]);
		}
	}
	cix_vi_hw_info->csi1_pixel_clk =
		devm_clk_get_optional(&pdev->dev, "csi1_p0clk");
	if (IS_ERR(cix_vi_hw_info->csi1_pixel_clk)) {
		LOG(LOG_ERR, "Couldn't get csi1_p0clk\n");
		return PTR_ERR(cix_vi_hw_info->csi1_pixel_clk);
	}
	for (i = 0; i < CSI2_PIXEL_NUM; i++) {
		sprintf(clk_name, "csi2_p%dclk", i);
		cix_vi_hw_info->csi2_pixel_clk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->csi2_pixel_clk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->csi2_pixel_clk[i]);
		}
	}
	cix_vi_hw_info->csi3_pixel_clk =
		devm_clk_get_optional(&pdev->dev, "csi3_p0clk");
	if (IS_ERR(cix_vi_hw_info->csi3_pixel_clk)) {
		LOG(LOG_ERR, "Couldn't get csi3_p0clk\n");
		return PTR_ERR(cix_vi_hw_info->csi3_pixel_clk);
	}
	for (i = 0; i < CSIDMA_NUM; i++) {
		sprintf(clk_name, "dma%d_pclk", i);
		cix_vi_hw_info->csidma_apbclk[i] =
			devm_clk_get_optional(&pdev->dev, clk_name);
		if (IS_ERR(cix_vi_hw_info->csidma_apbclk[i])) {
			LOG(LOG_ERR, "Couldn't get %s\n", clk_name);
			return PTR_ERR(cix_vi_hw_info->csidma_apbclk[i]);
		}
	}

	for (i = 0; i < DPHY_NUM; i++) {
		sprintf(reset_name, "phy%d_prst", i);
		cix_vi_hw_info->rst_dphy[i] =
			devm_reset_control_get_optional_shared(&pdev->dev,
								   reset_name);
		if (IS_ERR(cix_vi_hw_info->rst_dphy[i])) {
			if (PTR_ERR(cix_vi_hw_info->rst_dphy[i]) !=
				-EPROBE_DEFER)
				LOG(LOG_ERR, "Failed to get %s\n", reset_name);
			return PTR_ERR(cix_vi_hw_info->rst_dphy[i]);
		}
		sprintf(reset_name, "phy%d_cmnrst", i);
		cix_vi_hw_info->cmnrst_phy[i] =
			devm_reset_control_get_optional_shared(&pdev->dev,
								   reset_name);
		if (IS_ERR(cix_vi_hw_info->cmnrst_phy[i])) {
			if (PTR_ERR(cix_vi_hw_info->cmnrst_phy[i]) !=
				-EPROBE_DEFER)
				LOG(LOG_ERR, "Failed to get %s\n", reset_name);
			return PTR_ERR(cix_vi_hw_info->cmnrst_phy[i]);
		}
	}

	for (i = 0; i < CSI_NUM; i++) {
		sprintf(reset_name, "csi%d_reset", i);
		cix_vi_hw_info->csi_reset[i] =
			devm_reset_control_get_optional_shared(&pdev->dev,
								   reset_name);
		if (IS_ERR(cix_vi_hw_info->csi_reset[i])) {
			if (PTR_ERR(cix_vi_hw_info->csi_reset[i]) !=
				-EPROBE_DEFER)
				LOG(LOG_ERR, "Failed to get %s\n", reset_name);
			return PTR_ERR(cix_vi_hw_info->csi_reset[i]);
		}
	}
	for (i = 0; i < CSIDMA_NUM; i++) {
		sprintf(reset_name, "csibridge%d_reset", i);
		cix_vi_hw_info->csibridge_reset[i] =
			devm_reset_control_get_optional_shared(&pdev->dev,
								   reset_name);
		if (IS_ERR(cix_vi_hw_info->csibridge_reset[i])) {
			if (PTR_ERR(cix_vi_hw_info->csibridge_reset[i]) !=
				-EPROBE_DEFER)
				LOG(LOG_ERR, "Failed to get %s\n", reset_name);
			return PTR_ERR(cix_vi_hw_info->csibridge_reset[i]);
		}
	}
	for (i = 0; i < 2; i++) {
		sprintf(buffer, "ahb-dphy%d-base", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_dphy_base[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_dphy_base[i] = 0;
			LOG(LOG_ERR, "failed to get ahb-dphy%d-base.", i);
		}
		sprintf(buffer, "ahb-dphy%d-size", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_dphy_size[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_dphy_size[i] = 0;
			LOG(LOG_ERR, "failed to get ahb-dphy%d-size.", i);
		}
	}

	for (i = 0; i < 4; i++) {
		sprintf(buffer, "ahb-csi%d-base", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_csi_base[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_csi_base[i] = 0;
			LOG(LOG_ERR, "failed to get ahb-csi%d-base.", i);
		}
		sprintf(buffer, "ahb-csi%d-size", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_csi_size[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_csi_size[i] = 0;
			LOG(LOG_ERR, "failed to get ahb-csi-size.", i);
		}
	}

	for (i = 0; i < 4; i++) {
		sprintf(buffer, "ahb-csidma%d-base", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_csidma_base[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_csidma_base[i] = 0;
			LOG(LOG_ERR, "failed to get %s.", buffer);
		}
		sprintf(buffer, "ahb-csidma%d-size", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, "ahb-csidma0-size",
			&cix_vi_hw_info->ahb_csidma_size[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_csidma_size[i] = 0;
			LOG(LOG_ERR, "failed to get %s.", buffer);
		}
	}

	for (i = 0; i < 2; i++) {
		sprintf(buffer, "ahb-csircsu%d-base", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_csircsu_base[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_csircsu_base[i] = 0;
			LOG(LOG_ERR, "failed to get ahb-csircsu%d-base.", i);
		}
		sprintf(buffer, "ahb-csircsu%d-size", i);
		ret = fwnode_property_read_u32(
			pdev->dev.fwnode, buffer,
			&cix_vi_hw_info->ahb_csircsu_size[i]);
		if (ret < 0) {
			cix_vi_hw_info->ahb_csircsu_size[i] = 0;
			LOG(LOG_ERR, "failed to get ahb-csircsu%d-size.", i);
		}
	}

	for (i = 0; i < 2; i++) {
		if (cix_vi_hw_info->ahb_dphy_base[i] != 0 &&
			cix_vi_hw_info->ahb_dphy_size[i] != 0) {
			cix_vi_hw_info->ahb_dphy_base_addrs[i] = devm_ioremap(
				&pdev->dev, cix_vi_hw_info->ahb_dphy_base[i],
				cix_vi_hw_info->ahb_dphy_size[i]);
			if (!cix_vi_hw_info->ahb_dphy_base_addrs[i]) {
				cix_vi_hw_info->ahb_dphy_base_addrs[i] = 0;
				LOG(LOG_ERR,
					"failed to ioremap ahb-dphy%d register region.",
					i);
			}
		}
		LOG(LOG_INFO,
			"ahb-dphy%d-base=0x%x, ahb-dphy%d-size=0x%x ahb_dphy%d_base_addr=%p",
			i, cix_vi_hw_info->ahb_dphy_base[i], i,
			cix_vi_hw_info->ahb_dphy_size[i], i,
			cix_vi_hw_info->ahb_dphy_base_addrs[i]);
	}

	for (i = 0; i < 4; i++) {
		if (cix_vi_hw_info->ahb_csi_base[i] != 0 &&
			cix_vi_hw_info->ahb_csi_size[i] != 0) {
			cix_vi_hw_info->ahb_csi_base_addrs[i] = devm_ioremap(
				&pdev->dev, cix_vi_hw_info->ahb_csi_base[i],
				cix_vi_hw_info->ahb_csi_size[i]);
			if (!cix_vi_hw_info->ahb_csi_base_addrs[i]) {
				cix_vi_hw_info->ahb_csi_base_addrs[i] = 0;
				LOG(LOG_ERR,
					"failed to ioremap ahb-csi%d register region.",
					i);
			}
		}

		/*get irq info*/
		irq0 = platform_get_irq(pdev, i * 2);
		if (irq0 < 0)
			LOG(LOG_ERR, "Failed to get IRQ resource");

		ret = devm_request_irq(dev, irq0, mipi_csi2_irq_handler,
					   IRQF_ONESHOT | IRQF_SHARED,
					   dev_name(dev),
					   cix_vi_hw_info->ahb_csi_base_addrs[i]);
		if (ret)
			LOG(LOG_ERR, "failed to install irq (%d)", ret);

		irq1 = platform_get_irq(pdev, i * 2 + 1);
		if (!irq1)
			LOG(LOG_ERR, "Failed to get IRQ error resource");

		ret = devm_request_irq(dev, irq1, mipi_csi2_err_irq_handler,
					   IRQF_ONESHOT | IRQF_SHARED,
					   dev_name(dev),
					   cix_vi_hw_info->ahb_csi_base_addrs[i]);
		if (ret)
			LOG(LOG_ERR, "failed to install irq (%d)", ret);

		LOG(LOG_INFO,
			"ahb-csi%d-base=0x%x, ahb-csi%d-size=0x%x ahb_csi%d_base_addr=%p",
			i, cix_vi_hw_info->ahb_csi_base[i], i,
			cix_vi_hw_info->ahb_csi_size[i], i,
			cix_vi_hw_info->ahb_csi_base_addrs[i]);
	}

	for (i = 0; i < 4; i++) {
		if (cix_vi_hw_info->ahb_csidma_base[i] != 0 &&
			cix_vi_hw_info->ahb_csidma_size[i] != 0) {
			cix_vi_hw_info->ahb_csidma_base_addrs[i] = devm_ioremap(
				&pdev->dev, cix_vi_hw_info->ahb_csidma_base[i],
				cix_vi_hw_info->ahb_csidma_size[i]);
			if (!cix_vi_hw_info->ahb_csidma_base_addrs[i]) {
				cix_vi_hw_info->ahb_csidma_base_addrs[i] = 0;
				LOG(LOG_ERR,
					"failed to ioremap ahb-csidma%d register region.",
					i);
			}
		}
		LOG(LOG_INFO,
			"ahb-csidma%d-base=0x%x, ahb-csidma%d-size=0x%x " \
			"ahb_csidma%d_base_addr=%p",
			i, cix_vi_hw_info->ahb_csidma_base[i], i,
			cix_vi_hw_info->ahb_csidma_size[i], i,
			cix_vi_hw_info->ahb_csidma_base_addrs[i]);
	}

	for (i = 0; i < 2; i++) {
		if (cix_vi_hw_info->ahb_csircsu_base[i] != 0 &&
			cix_vi_hw_info->ahb_csircsu_size[i] != 0) {
			cix_vi_hw_info->ahb_csircsu_base_addrs[i] =
				devm_ioremap(
					&pdev->dev,
					cix_vi_hw_info->ahb_csircsu_base[i],
					cix_vi_hw_info->ahb_csircsu_size[i]);
			if (!cix_vi_hw_info->ahb_csircsu_base_addrs[i]) {
				cix_vi_hw_info->ahb_csircsu_base_addrs[i] = 0;
				LOG(LOG_ERR,
					"failed to ioremap ahb-csircsu% register region.",
					i);
			}
		}
		LOG(LOG_INFO,
			"ahb-csircsu%d-base=0x%x, ahb-csircsu%d-size=0x%x " \
			"ahb_csircsu%d_base_addr=%p",
			i, cix_vi_hw_info->ahb_csircsu_base[i], i,
			cix_vi_hw_info->ahb_csircsu_size[i], i,
			cix_vi_hw_info->ahb_csircsu_base_addrs[i]);
	}

	mutex_init(&cix_vi_hw_info->mutex);

	platform_set_drvdata(pdev, cix_vi_hw_info);

	dev_info(dev, "cix hw reg list probe exit\n");

	return 0;
}

static int cix_vi_hw_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_info(dev, "csi rcsu hw remove enter\n");
	mutex_destroy(&cix_vi_hw_info->mutex);
	return 0;
}

static struct platform_driver cix_vi_hw_driver = {
	.driver = {
		.name = CIX_VI_HW_NAME,
		.of_match_table = cix_vi_hw_of_match,
		.acpi_match_table = ACPI_PTR(cix_vi_hw_acpi_match),
	},
	.probe = cix_vi_hw_probe,
	.remove = cix_vi_hw_remove,
};

static void *g_instance;
void *cix_vi_hw_instance(void)
{
	if (platform_driver_register(&cix_vi_hw_driver) < 0) {
		LOG(LOG_ERR, "register cix vi hw drv failed.\n");
		return NULL;
	}
	g_instance = (void *)&cix_vi_hw_driver;
	return g_instance;
}

void cix_vi_hw_destroy(void)
{
	if (g_instance)
		platform_driver_unregister(
			(struct platform_driver *)g_instance);
}

MODULE_AUTHOR("Cix Semiconductor, Inc.");
MODULE_DESCRIPTION("VI HW register of Cix SOC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" CIX_VI_HW_NAME);
