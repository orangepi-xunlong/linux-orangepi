// SPDX-License-Identifier: GPL-2.0

#include <linux/limits.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include "remoteproc_internal.h"

struct ky_rproc {
	struct reset_control *rst;
	struct clk *mux_clk;
	struct clk *src_clk;
	struct clk *core_clk;
	struct clk *mbus_clk;
	struct clk *ahb_clk;
	struct clk *apb_clk;
	void __iomem *reg_base;
};

static int ky_rproc_mem_alloc(struct rproc *rproc,
				 struct rproc_mem_entry *mem)
{
	struct device *dev = &rproc->dev;
	void *va;

	dev_dbg(dev, "map memory: %pa+%zx\n", &mem->dma, mem->len);
	va = ioremap(mem->dma, mem->len);
	if (!va) {
		dev_err(dev, "Unable to map memory region: %pa+%zx\n",
			&mem->dma, mem->len);
		return -ENOMEM;
	}

	/* Update memory entry va */
	mem->va = va;

	return 0;
}

static int ky_rproc_mem_release(struct rproc *rproc,
				   struct rproc_mem_entry *mem)
{
	dev_dbg(&rproc->dev, "unmap memory: %pa\n", &mem->dma);
	iounmap(mem->va);

	return 0;
}

static int ky_rproc_prepare(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct rproc_mem_entry *mem;
	struct reserved_mem *rmem;
	u32 da;

	/* Register associated reserved memory regions */
	of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	while (of_phandle_iterator_next(&it) == 0) {

		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			dev_err(&rproc->dev,
				"unable to acquire memory-region\n");
			return -EINVAL;
		}

		if (rmem->base > U64_MAX) {
			dev_err(&rproc->dev,
				"the rmem base is overflow\n");
			return -EINVAL;
		}

		/* No need to translate pa to da, ky use same map */
		da = rmem->base;
		mem = rproc_mem_entry_init(dev, NULL,
					   rmem->base,
					   rmem->size, da,
					   ky_rproc_mem_alloc,
					   ky_rproc_mem_release,
					   it.node->name);

		if (!mem)
			return -ENOMEM;

		rproc_add_carveout(rproc, mem);
	}

	return 0;
}

static int ky_rproc_parse_fw(struct rproc *rproc, const struct firmware *fw)
{
	int ret;

	ret = rproc_elf_load_rsc_table(rproc, fw);
	if (ret)
		dev_info(&rproc->dev, "No resource table in elf\n");

	return 0;
}

static int ky_rproc_start(struct rproc *rproc)
{
	struct ky_rproc *priv = rproc->priv;
	int err;

	if (!rproc->bootaddr)
		return -EINVAL;

	/* set the entry point to the register */
	writel(rproc->bootaddr, priv->reg_base);

	err = reset_control_deassert(priv->rst);
	if (err)
		dev_err(&rproc->dev, "failed to deassert reset\n");

	return err;
}

static int ky_rproc_stop(struct rproc *rproc)
{
	/* TODO */
	return 0;
}

static u64 ky_get_boot_addr(struct rproc *rproc, const struct firmware *fw)
{
	int err;
	unsigned int entry_point;
	struct device *dev = rproc->dev.parent;

	/* get the entry point */
	err = of_property_read_u32(dev->of_node, "esos-entry-point", &entry_point);
	if (err) {
		 dev_err(dev, "failed to get entry point\n");
		 return 0;
	}

	return entry_point;
}

static void *ky_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
{
	struct rproc_mem_entry *carveout;
	void *ptr = NULL;

	list_for_each_entry(carveout, &rproc->carveouts, node) {
		s64 offset = (carveout->dma - /* memory base */0xa000000000) - (da -
			/* dram memory mapping base from rcpu */ - 0x80000000);

		/*  Verify that carveout is allocated */
		if (!carveout->va)
			continue;

		/* try next carveout if da is too small */
		if (offset < 0)
			continue;

		/* try next carveout if da is too large */
		if (offset + len > carveout->len)
			continue;

		ptr = carveout->va + offset;

		if (is_iomem)
			*is_iomem = carveout->is_iomem;

		break;
	}

	return ptr;
}

static struct rproc_ops ky_rproc_ops = {
	.prepare	= ky_rproc_prepare,
	.start		= ky_rproc_start,
	.stop		= ky_rproc_stop,
	.load		= rproc_elf_load_segments,
	.parse_fw	= ky_rproc_parse_fw,
	.find_loaded_rsc_table = rproc_elf_find_loaded_rsc_table,
	.sanity_check	= rproc_elf_sanity_check,
	.get_boot_addr	= ky_get_boot_addr,
	.da_to_va	= ky_da_to_va,

};

static int ky_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct ky_rproc *priv;
	unsigned int clk_frequency;
	struct rproc *rproc;
	const char *fw_name = "esos.elf";
	int ret;

	ret = rproc_of_parse_firmware(dev, 0, &fw_name);
	if (ret < 0 && ret != -EINVAL)
		return ret;

	rproc = devm_rproc_alloc(dev, np->name, &ky_rproc_ops,
				fw_name, sizeof(*priv));
	if (!rproc)
		return -ENOMEM;

	priv = rproc->priv;

	priv->reg_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->reg_base)) {
		ret = PTR_ERR(priv->reg_base);
		dev_err(dev, "failed to get reg base\n");
		return ret;
	}

	priv->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(priv->rst)) {
		ret = PTR_ERR(priv->rst);
		dev_err_probe(dev, ret, "fail to acquire rproc reset\n");
		return ret;
	}

	priv->mux_clk = devm_clk_get(dev, "mux");
	if (IS_ERR(priv->mux_clk)) {
		ret = PTR_ERR(priv->mux_clk);
		dev_err(dev, "failed to acquire rpoc mux\n");
		return ret;
	}

	priv->src_clk = devm_clk_get(dev, "src");
	if (IS_ERR(priv->src_clk)) {
		ret = PTR_ERR(priv->src_clk);
		dev_err(dev, "failed to acquire rpoc src\n");
		return ret;
	}

	priv->ahb_clk = devm_clk_get(dev, "ahb");
	if (IS_ERR(priv->ahb_clk)) {
		ret = PTR_ERR(priv->ahb_clk);
		dev_err(dev, "failed to acquire rpoc ahb\n");
		return ret;
	}

	priv->apb_clk = devm_clk_get(dev, "apb");
	if (IS_ERR(priv->apb_clk)) {
		ret = PTR_ERR(priv->apb_clk);
		dev_err(dev, "failed to acquire rpoc apb\n");
		return ret;
	}

	priv->core_clk = devm_clk_get(dev, "core");
	if (IS_ERR(priv->core_clk)) {
		ret = PTR_ERR(priv->core_clk);
		dev_err(dev, "failed to acquire rpoc core\n");
		return ret;
	}

	priv->mbus_clk = devm_clk_get(dev, "mbus");
	if (IS_ERR(priv->mbus_clk)) {
		ret = PTR_ERR(priv->mbus_clk);
		dev_err(dev, "failed to acquire rpoc mbus\n");
		return ret;
	}

	/* set the clock source */
	ret = clk_set_parent(priv->mux_clk, priv->src_clk);
	if (ret < 0) {
		dev_err(dev, "failed to set parent clk\n");
		return -EINVAL;
	}

	/* get ahb clock rate */
	ret = of_property_read_u32(dev->of_node, "core-ahb-clock-frequency", &clk_frequency);
	if (ret) {
		 dev_err(dev, "failed to get ahb clk frequency\n");
		 return -EINVAL;
	}

	/* set ahb clock rate */
	ret = clk_set_rate(priv->ahb_clk, clk_frequency);
	if (ret) {
		 dev_err(dev, "failed to set ahb clk frequency\n");
		 return -EINVAL;
	}

	/* get apb clock rate */
	ret = of_property_read_u32(dev->of_node, "aph-bus-clock-frequency", &clk_frequency);
	if (ret) {
		 dev_err(dev, "failed to get ahb clk frequency\n");
		 return -EINVAL;
	}

	/* set ahb clock rate */
	ret = clk_set_rate(priv->apb_clk, clk_frequency);
	if (ret) {
		 dev_err(dev, "failed to set apb clk frequency\n");
		 return -EINVAL;
	}

	/* enable clk */
	ret = clk_prepare_enable(priv->mbus_clk);
	if (ret) {
		dev_err(dev, "failed to enable mbus clk\n");
		return -EINVAL;
	}

	ret = clk_prepare_enable(priv->core_clk);
	if (ret) {
		clk_disable_unprepare(priv->mbus_clk);
		dev_err(dev, "failed to enable core clk\n");
		return -EINVAL;
	}

	dev_set_drvdata(dev, rproc);

	/* Manually start the rproc */
	rproc->auto_boot = false;

	ret = devm_rproc_add(dev, rproc);
	if (ret) {
		dev_err(dev, "rproc_add failed\n");
	}

	return ret;
}

static int ky_rproc_remove(struct platform_device *pdev)
{
	/* TODO */
	return 0;
}

static const struct of_device_id ky_rproc_of_match[] = {
	{ .compatible = "ky,x1-pro-rproc" },
	{},
};

MODULE_DEVICE_TABLE(of, ky_rproc_of_match);

static struct platform_driver ky_rproc_driver = {
	.probe = ky_rproc_probe,
	.remove = ky_rproc_remove,
	.driver = {
		.name = "ky-rproc",
		.of_match_table = ky_rproc_of_match,
	},
};

module_platform_driver(ky_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sapcemit remote processor control driver");
