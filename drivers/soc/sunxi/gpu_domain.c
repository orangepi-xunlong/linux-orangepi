/*
 * Allwinner GPU power domain support.
 *
 * Copyright (C) 2019 Allwinner Technology, Inc.
 *	fanqinghua <fanqinghua@allwinnertech.com>
 *
 * Implementation of gpu specific power domain control which is used in
 * conjunction with runtime-pm. Support for both device-tree and non-device-tree
 * based power domain support is included.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/reset.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/pm_domain.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/sched.h>
#include <linux/mfd/syscon.h>

#define DRIVER_NAME	"GPU power domain driver"
#define GPU_RSTN_REG			0x0
#define GPU_CLK_GATE_REG		0x4
#define GPU_POWEROFF_GATE_REG	0x8
#define GPU_PSWON_REG			0xC

#define GPU_PSWOFF_REG			0x10
#define GPU_PDA_OFF				0xff
#define GPU_PDB_OFF				0xff0000

#define GPU_PSW_DLY_REG			0x14
#define GPU_PDOFFCTL_DLY_REG	0x18
#define GPU_PDONCTL_DLY_REG		0x1C

#define GPU_PD_STAT_REG			0x20
#define GPU_PD_STAT_MASK		0x3
#define GPU_PD_STAT_3D			0x0
#define GPU_PD_STAT_HOST		0x1
#define GPU_PD_STAT_ALLOFF		0x2

#define PPU_CFG_REG				0x24
#define POWER_CTRL_MODE_MASK	0x3
#define POWER_CTRL_MODE_SW		0x1
#define POWER_CTRL_MODE_HW		0x2
#define PPU_POWER_ON_COMPLETE	0x10

#define PPU_STAT_REG			0x28
#define GPU_POWER_REQ			0x1
#define GPU_STAT_3D_2_HOST		0x2
#define GPU_STAT_HOST_2_3D		0x4
#define GPU_STAT_POWER_MASK		0x7

#define PPU_IRQ_MASK_REG		0x2C
#define PSW_OFF_DLY_REG			0x30

#define to_gpu_pd(gpd) container_of(gpd, struct gpu_pm_domain, genpd)

/*
 * gpu specific wrapper around the generic power domain
 */
struct gpu_pm_domain {
	struct device *dev;
	struct regmap *regmap;
	char const *name;
	struct clk *clk;
	struct reset_control *reset;
	int irq;
	struct generic_pm_domain genpd;
	spinlock_t ppu_lock;
};

static inline void gpu_rstn(struct gpu_pm_domain *pd, bool on)
{
	udelay(1);
	regmap_write(pd->regmap, GPU_RSTN_REG, on ? 1 : 0);
}

static inline void gpu_clk_gate(struct gpu_pm_domain *pd, bool on)
{
	regmap_write(pd->regmap, GPU_CLK_GATE_REG, on ? 1 : 0);
	udelay(1);
}

static inline void gpu_iso_b(struct gpu_pm_domain *pd, bool on)
{
	regmap_update_bits(pd->regmap,
		GPU_POWEROFF_GATE_REG, BIT(1), on ? -1U : 0);
}

static inline void gpu_iso_a(struct gpu_pm_domain *pd, bool on)
{
	regmap_update_bits(pd->regmap,
		GPU_POWEROFF_GATE_REG, BIT(0), on ? -1U : 0);
}

static inline void gpu_pwra_on(struct gpu_pm_domain *pd)
{
	unsigned int i;

	for (i = 0; i < 8; i++) {
		regmap_write(pd->regmap, GPU_PSWON_REG, 1 << i);
		udelay(1);
	}
}

static inline void gpu_pwra_off(struct gpu_pm_domain *pd)
{
	regmap_write(pd->regmap, GPU_PSWOFF_REG, GPU_PDA_OFF);
}

static inline void gpu_pwrb_on(struct gpu_pm_domain *pd)
{
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(23));
	udelay(10);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(18));
	udelay(10);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(16));
	udelay(5);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(19));
	udelay(5);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(20));
	udelay(5);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(21));
	udelay(5);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(22));
	udelay(5);
	regmap_write(pd->regmap, GPU_PSWON_REG, BIT(17));
	udelay(5);
}

static inline void gpu_pwrb_off(struct gpu_pm_domain *pd)
{
	regmap_write(pd->regmap, GPU_PSWOFF_REG, GPU_PDB_OFF);
}

static inline void domain_b_on_flow(struct gpu_pm_domain *pd)
{
	gpu_pwrb_on(pd);
	gpu_iso_b(pd, false);
	regmap_write(pd->regmap, GPU_PD_STAT_REG, GPU_PD_STAT_3D);
}

static inline void domain_b_off_flow(struct gpu_pm_domain *pd)
{
	gpu_iso_b(pd, true);
	gpu_pwrb_off(pd);
	regmap_write(pd->regmap, GPU_PD_STAT_REG, GPU_PD_STAT_HOST);
}

static irqreturn_t ppu_irq_handler(int irq, void *dev_id)
{
	struct gpu_pm_domain *pd = dev_id;
	unsigned int rdata;

	regmap_read(pd->regmap, PPU_STAT_REG, &rdata);


	/* gpu power req */
	if (rdata & GPU_POWER_REQ) {
		spin_lock(&(pd->ppu_lock));
		if (((rdata >> 16) & GPU_STAT_POWER_MASK)
			== GPU_STAT_3D_2_HOST)
			domain_b_off_flow(pd);
		else if (((rdata >> 16) & GPU_STAT_POWER_MASK)
			== GPU_STAT_HOST_2_3D)
			domain_b_on_flow(pd);

		/* clear gpu power req pending */
		regmap_write(pd->regmap, PPU_STAT_REG, 0x1);
		/* send complete signal to gpu*/
		regmap_read(pd->regmap, PPU_CFG_REG, &rdata);
		regmap_write(pd->regmap, PPU_CFG_REG,
			rdata | PPU_POWER_ON_COMPLETE);
		spin_unlock(&(pd->ppu_lock));
	}

	return IRQ_HANDLED;
}

static int gpu_pd_power_on(struct generic_pm_domain *domain)
{
	struct gpu_pm_domain *pd = to_gpu_pd(domain);
	unsigned int rdata;
	unsigned long irqflags;

	regmap_read(pd->regmap, GPU_PD_STAT_REG, &rdata);
	WARN_ON((rdata & GPU_PD_STAT_MASK) != GPU_PD_STAT_ALLOFF);

	spin_lock_irqsave(&(pd->ppu_lock), irqflags);

	gpu_pwra_on(pd);
	gpu_clk_gate(pd, true);
	gpu_iso_a(pd, false);
	gpu_pwrb_on(pd);
	gpu_iso_b(pd, false);
	gpu_rstn(pd, true);
	udelay(10);
	regmap_write(pd->regmap, GPU_PD_STAT_REG, GPU_PD_STAT_3D);
	regmap_write(pd->regmap, PPU_STAT_REG, 0x1);
	regmap_write(pd->regmap, PPU_IRQ_MASK_REG, 0x1);

	spin_unlock_irqrestore(&(pd->ppu_lock), irqflags);

	return 0;
}

static int gpu_pd_power_off(struct generic_pm_domain *domain)
{
	struct gpu_pm_domain *pd = to_gpu_pd(domain);
	unsigned int rdata;
	unsigned long irqflags;

	regmap_read(pd->regmap, GPU_PD_STAT_REG, &rdata);
	WARN_ON((rdata & GPU_PD_STAT_MASK) == GPU_PD_STAT_ALLOFF);

	spin_lock_irqsave(&(pd->ppu_lock), irqflags);

	gpu_iso_b(pd, true);
	gpu_iso_a(pd, true);
	gpu_clk_gate(pd, false);
	gpu_rstn(pd, false);
	gpu_pwrb_off(pd);
	gpu_pwra_off(pd);
	regmap_write(pd->regmap, GPU_PD_STAT_REG, GPU_PD_STAT_ALLOFF);
	regmap_write(pd->regmap, PPU_STAT_REG, 0x1);
	regmap_write(pd->regmap, PPU_IRQ_MASK_REG, 0x0);

	spin_unlock_irqrestore(&(pd->ppu_lock), irqflags);

	return 0;
}

static inline int sunxi_get_ic_version(char *version)
{
#define SYS_CFG_BASE 0x03000000
#define VER_REG_OFFS 0x00000024
	void __iomem *io = NULL;
	static char ver = 0xff;
	/* IC version:
	 * A/B/C: 0
	 *     D: 3
	 *     E: 4
	 *     F: 5
	 *        6/7/1/2 to be used in future.
	 */
	*version = 0;
	if (ver == 0xff) {
		io = ioremap(SYS_CFG_BASE, 0x100);
		if (io == NULL) {
			pr_err("%s: ioremap of sys_cfg register failed!\n",
				__func__);
			return -1;
		}
		*version = (char)(readl(io + VER_REG_OFFS) & 0x7);
		iounmap(io);
		ver = *version;
	} else {
		*version = ver;
	}
	return 0;
}

static bool sunxi_ic_version_ctrl(void)
{
	char ic_version = 0;
	sunxi_get_ic_version(&ic_version);
	/*
	 * The flow of jtag reset before gpu reset will cause MIPS crash, so we
	 * will put the domainA, domainB and gpu reset operations to boot0
	 * stage. And in kernel stage, we will always keep domainA poweron.
	 */
	if (ic_version == 0 || ic_version == 3 || ic_version == 4 ||
		ic_version == 5)
		return false;
	return true;
}

static const struct of_device_id gpu_pm_domain_of_match[] = {
	{
		.compatible = "allwinner,gpu-pd",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, gpu_pm_domain_of_match);

static int gpu_domain_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct gpu_pm_domain *pd;
	int ret = 0;

	if (!np) {
		dev_err(&pdev->dev, "failed to match gpu power domain\n");
		return -ENODEV;
	}

	pd = devm_kzalloc(&pdev->dev, sizeof(*pd), GFP_KERNEL);
	if (!pd) {
		dev_err(&pdev->dev, "failed to malloc pd space\n");
		return -ENOMEM;
	}

	pd->dev = &pdev->dev;
	platform_set_drvdata(pdev, pd);

	pd->genpd.name = np->full_name;
	pd->name = pd->genpd.name;
	pd->regmap = syscon_node_to_regmap(np);
	if (IS_ERR(pd->regmap)) {
		dev_err(&pdev->dev, "no regmap available\n");
		return PTR_ERR(pd->regmap);
	}

	/*
	 * Gpu driver will not set and unset ppu runtime counter, it
	 * can make sure the poweron and poweroff function of ppu
	 * must not be called by kernel stage.
	 * Besides the ppu driver will only recept event to control domain B
	 * power status in kernel stage.
	 */
	if (sunxi_ic_version_ctrl()) {
		pd->genpd.power_off = gpu_pd_power_off;
		pd->genpd.power_on = gpu_pd_power_on;
	}
	pd->genpd.flags = GENPD_FLAG_PM_CLK;

	pd->irq = of_irq_get(np, 0);
	if (pd->irq == -EPROBE_DEFER) {
		dev_err(&pdev->dev, "failed to find gpu domain IRQ number\n");
		return -EPROBE_DEFER;
	}

	ret = devm_request_irq(&pdev->dev, pd->irq, ppu_irq_handler,
				0, "ppu_interrupt", (void *)pd);
	if (ret) {
		dev_err(&pdev->dev, "failed to request gpu domain interrupt\n");
		return ret;
	}

	pd->reset = devm_reset_control_get(&pdev->dev, "ppu_rst");
	if (IS_ERR(pd->reset))
		return PTR_ERR(pd->reset);

	pd->clk = devm_clk_get(&pdev->dev, "ppu");
	if (IS_ERR(pd->clk)) {
		dev_err(&pdev->dev, "failed to find clock\n");
		return -ENOMEM;
	}

	ret = reset_control_deassert(pd->reset);
	if (ret)
		return ret;

	ret = clk_prepare_enable(pd->clk);
	if (ret)
		goto assert_reset;

	spin_lock_init(&(pd->ppu_lock));

	regmap_write(pd->regmap, PPU_CFG_REG, POWER_CTRL_MODE_SW);
	regmap_write(pd->regmap, PPU_IRQ_MASK_REG, 0x1);

	if (IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)) {
		pm_genpd_init(&pd->genpd, NULL, true);
		of_genpd_add_provider_simple(np, &pd->genpd);
	}

	return 0;

assert_reset:
	reset_control_assert(pd->reset);
	return ret;
}

static int gpu_domain_remove(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct gpu_pm_domain *pd = platform_get_drvdata(pdev);

	if (IS_ENABLED(CONFIG_PM_GENERIC_DOMAINS)) {
		of_genpd_del_provider(np);
		pm_genpd_remove(&pd->genpd);
	}
	clk_disable_unprepare(pd->clk);
	reset_control_assert(pd->reset);
	return 0;
}

static struct platform_driver gpu_domain_driver = {
	.probe   = gpu_domain_probe,
	.remove  = gpu_domain_remove,
	.driver  = {
		.name  = DRIVER_NAME,
		.of_match_table = gpu_pm_domain_of_match,
	},
};

module_platform_driver(gpu_domain_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner GPU power domain driver");
MODULE_ALIAS("platform:" DRIVER_NAME);
MODULE_AUTHOR("fanqinghua <fanqinghua@allwinnertech.com>");
MODULE_VERSION("1.0.1");
