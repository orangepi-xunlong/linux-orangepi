//                           SmartDV Technologies Proprietary
//           Copyright 2007-2023 SmartDV Technologies India Private Limited
//
//                              CONFIDENTIAL INFORMATION
//
//                                 All rights reserved
//
//            The use, modification, or duplication of this product is protected
//            according to SmartDV Technologies's licensing agreement.
//
//            This Intellectual property contains confidential and proprietary
//            information which are the properties of SmartDV Technologies.
//
//            Unauthorized use, disclosure, duplication, or reproduction are prohibited.

#include <linux/platform_device.h>
#include "cvitek_can.h"
extern int sdvt_can_set_mode(struct net_device *dev, enum can_mode mode);


static u32 iomap_read_reg(struct sdvt_can_classdev *cdev, int reg)
{
	struct sdvt_can_plat_priv *priv = cdev->device_data;

	return readl(priv->base + (reg * 4));
}


static int  iomap_write_reg(struct sdvt_can_classdev *cdev, int reg, int val)
{
	struct sdvt_can_plat_priv *priv = cdev->device_data;

	writel(val, priv->base +(reg * 4));

	return 0;
}


static struct sdvt_can_ops sdvt_can_plat_ops = {
	.read_reg = iomap_read_reg,
	.write_reg = iomap_write_reg
};

static int sdvt_can_plat_probe(struct platform_device *pdev)
{
	struct sdvt_can_classdev *sdvt_can_class;
	struct sdvt_can_plat_priv *priv;
	struct resource *res;
	void __iomem *addr;
	void __iomem *sd1_d1_pinmux_addr;
	int irq, ret = 0;
	const char *label;

	sdvt_can_class = sdvt_can_class_allocate_dev(&pdev->dev);
	if (!sdvt_can_class)
		return -ENOMEM;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto probe_fail;
	}

	sdvt_can_class->device_data = priv;
	ret = sdvt_can_class_get_clocks(sdvt_can_class);
	if (ret)
		goto probe_fail;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "can");
	addr = devm_ioremap_resource(&pdev->dev, res);
	dev_info(&pdev->dev,"can addr: %px  res.start:%llx\n",(void *)&addr,res->start);
	irq = platform_get_irq(pdev, 0);
	if (IS_ERR(addr) || irq < 0) {
		ret = -EINVAL;
		goto probe_fail;
	}

	priv->base = addr;

	label = of_get_property(pdev->dev.of_node, "label", NULL);
	if (label) {
		dev_info(&pdev->dev, "label: %s\n", label);
		if (strcmp(label, "SE9") == 0) {
			sd1_d1_pinmux_addr = devm_ioremap(&pdev->dev, SD1_D1_PINMUX, 4);
			if (IS_ERR(sd1_d1_pinmux_addr)) {
				dev_err(&pdev->dev, "Failed to map register address\n");
				goto probe_fail;
			}
			writel(readl(sd1_d1_pinmux_addr) & ~0xf, sd1_d1_pinmux_addr);
			writel(readl(sd1_d1_pinmux_addr) | PULL_DOWN_EN, sd1_d1_pinmux_addr);
		}
	} else {
		dev_info(&pdev->dev, "label: none\n");
	}

	sdvt_can_class->net->irq = irq;
	sdvt_can_class->pm_clock_support = 1;
	sdvt_can_class->can.clock.freq = clk_get_rate(sdvt_can_class->cclk);

	sdvt_can_class->dev = &pdev->dev;

	sdvt_can_class->ops = &sdvt_can_plat_ops;
	memset(&sdvt_can_class->cmd_o, 0, sizeof(sdvt_can_class->cmd_o));
	memset(&sdvt_can_class->cfg_o, 0, sizeof(sdvt_can_class->cfg_o));

	sdvt_can_class->is_peripheral = false;

	platform_set_drvdata(pdev, sdvt_can_class->net);

	// m_can_init_ram(sdvt_can_class);

	return sdvt_can_class_register(sdvt_can_class);

probe_fail:
	sdvt_can_class_free_dev(sdvt_can_class->net);
	return ret;
}

static __maybe_unused int sdvt_can_suspend(struct device *dev)
{
	return sdvt_can_class_suspend(dev);
}

static __maybe_unused int sdvt_can_resume(struct device *dev)
{
	return sdvt_can_class_resume(dev);
}

static int sdvt_can_plat_remove(struct platform_device *pdev)
{
	struct net_device *dev = platform_get_drvdata(pdev);
	struct sdvt_can_classdev *sdvt_can_class = netdev_priv(dev);

	sdvt_can_class_unregister(sdvt_can_class);

	sdvt_can_class_free_dev(sdvt_can_class->net);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int __maybe_unused sdvt_can_runtime_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sdvt_can_classdev *sdvt_can_class = netdev_priv(ndev);

	clk_disable_unprepare(sdvt_can_class->cclk);

	return 0;
}

static int __maybe_unused sdvt_can_runtime_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct sdvt_can_classdev *sdvt_can_class = netdev_priv(ndev);
	int err;


	err = clk_prepare_enable(sdvt_can_class->cclk);
	if (err)
		return err;

	return err;
}

static const struct dev_pm_ops sdvt_can_pmops = {
	SET_RUNTIME_PM_OPS(sdvt_can_runtime_suspend,
			   sdvt_can_runtime_resume, NULL)
	SET_SYSTEM_SLEEP_PM_OPS(sdvt_can_suspend, sdvt_can_resume)
};

static const struct of_device_id sdvt_can_of_table[] = {
	{ .compatible = "smartDV,sdvt_can", .data = NULL },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sdvt_can_of_table);

static struct platform_driver sdvt_can_plat_driver = {
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = sdvt_can_of_table,
		.pm     = &sdvt_can_pmops,
	},
	.probe = sdvt_can_plat_probe,
	.remove = sdvt_can_plat_remove,
};

module_platform_driver(sdvt_can_plat_driver);

MODULE_AUTHOR("Dong Aisheng <b29396@freescale.com>");
MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("M_CAN driver for IO Mapped Bosch controllers");
