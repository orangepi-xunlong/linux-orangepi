/*
 * Created on Thu Jun 01 2023
 *
 * Copyright (c) 2023 CVITEK
 */
#include <linux/clk.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <media/rc-core.h>

#include "pinctrl-cv186x.h"

#define IRRX_ENABLE   0x0
#define IRRX_MODE     0x4
#define IRRX_INT_EN   0x10
#define IRRX_INT_CLR  0x14
#define IRRX_INT_MSK  0x18
#define IRRX_INT_STA  0x1c
#define IRRX_NEC_DATA 0xa8

#define RX_DONE	  BIT(0)
#define FRAME_ERR BIT(1)
#define FRAME_OVF BIT(2)
#define RELEASE	  BIT(3)
#define REPEAT	  BIT(4)

#define DRIVER_NAME "cvi-irrx"

struct cvi_irrx {
	int	       irq;
	struct rc_dev *rc;
	void __iomem  *base;
	struct clk    *clk;
};

//	strict ordering	is not necessary, use relaxed api
void irrx_init(struct cvi_irrx *irrx)
{
	void __iomem *base = irrx->base;
	writel_relaxed(0x20, base + 0x0c);
	// 25M clk
	writel_relaxed(0x97918,	base + 0x08);
	// 5M clk
	// writel_relaxed(0x97904, base + 0x08);
	writel_relaxed(0xdc02bc, base + 0x30);
	writel_relaxed(0x2401c1, base + 0x34);
	writel_relaxed(0xb370b37, base + 0x38);
	writel_relaxed(0x1800e0, base + 0x3c);
	writel_relaxed(0x9006f, base + 0x40);
	writel_relaxed(0x2Af0, base + 0x44);
	writel_relaxed(0x480383, base + 0x48);
	writel_relaxed(0x1800e0, base + 0x4c);

	// enable intr
	writel_relaxed(0xff, base + IRRX_INT_EN);
	// clear intr
	writel_relaxed(0xff, base + IRRX_INT_CLR);
	// mask	err, ovf and release intr
	writel_relaxed(0xe, base + IRRX_INT_MSK);
	writel_relaxed(0x1, base + IRRX_ENABLE);
	// pinmux
	PINMUX_CONFIG(PWR_WAKEUP0, PWR_IRRX0, G7);
}

static irqreturn_t irrx_irq(int irq, void *dev_id)
{
	unsigned int	 v, code;
	enum rc_proto	 proto;
	struct cvi_irrx *irrx = dev_id;

	// repeat code
	if (readl_relaxed(irrx->base + IRRX_INT_STA) & REPEAT) {
		rc_repeat(irrx->rc);
		writel_relaxed(0xff, irrx->base + IRRX_INT_CLR);
		return IRQ_HANDLED;
	}

	v = readl_relaxed(irrx->base + IRRX_NEC_DATA);
	dev_dbg(&irrx->rc->dev, "recevie ir nec data: 0x%X\n", v);

	code = ir_nec_bytes_to_scancode(v, v >> 8, v >> 16, v >> 24, &proto);
	rc_keydown(irrx->rc, proto, code, 0);

	writel_relaxed(0xff, irrx->base + IRRX_INT_CLR);
	return IRQ_HANDLED;
}

static int cvi_irrx_probe(struct platform_device *pdev)
{
	int		 ret;
	struct device	*dev = &pdev->dev;
	struct rc_dev	*rc;
	struct cvi_irrx *irrx;
	dev_dbg(dev, "irrx driver probe start!\n");

	irrx = devm_kzalloc(dev, sizeof(*irrx), GFP_KERNEL);
	if (!irrx)
		return -ENOMEM;

	irrx->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(irrx->base)) {
		dev_err(dev, "remap resource failed!\n");
		return PTR_ERR(irrx->base);
	}

	irrx->irq = platform_get_irq(pdev, 0);
	if (irrx->irq < 0) {
		dev_dbg(dev, "get irq failed\n");
		return -EINVAL;
	}

	// irrx->clk = devm_clk_get(dev, "clk_irrx");
	// if (IS_ERR(irrx->clk)) {
	// 	dev_err(dev, "get clk failed!\n");
	// 	return PTR_ERR(irrx->clk);
	// }

	rc = devm_rc_allocate_device(dev, RC_DRIVER_SCANCODE);
	if (!rc)
		return -ENOMEM;
	rc->device_name = DRIVER_NAME;
	rc->driver_name = DRIVER_NAME;
	rc->input_phys = DRIVER_NAME "/input0";
	rc->map_name = "rc-anysee";
	rc->allowed_protocols = RC_PROTO_BIT_NEC;
	// | RC_PROTO_BIT_RC5 |	RC_PROTO_BIT_RC6_0 | RC_PROTO_BIT_SANYO
	rc->priv = irrx;
	irrx->rc = rc;

	ret	= clk_prepare_enable(irrx->clk);
	if (ret)
		return ret;

	ret = devm_request_irq(dev, irrx->irq, irrx_irq, IRQF_SHARED,
			       dev_name(dev), irrx);
	if (ret)
		goto err_clk;

	ret = rc_register_device(rc);
	if (ret)
		goto err_clk;

	irrx_init(irrx);

	platform_set_drvdata(pdev, irrx);
	dev_dbg(dev, "irrx driver registered\n");

	// enable trigger wake-up event
	// device_set_wakeup_capable(&pdev->dev, 1);
	return 0;

err_clk:
	clk_disable_unprepare(irrx->clk);
	return ret;
}

static int cvi_irrx_remove(struct platform_device *pdev)
{
	struct cvi_irrx *irrx = platform_get_drvdata(pdev);
	rc_unregister_device(irrx->rc);
	clk_disable_unprepare(irrx->clk);
	dev_dbg(&pdev->dev, "irrx driver removed!\n");
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int irrx_suspend(struct device *dev)
{
	struct cvi_irrx *irrx = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		enable_irq_wake(irrx->irq);
	else {
		writel_relaxed(0x0, irrx->base + IRRX_ENABLE);
		writel_relaxed(0x0, irrx->base + IRRX_INT_EN);
		clk_disable_unprepare(irrx->clk);
	}

	return 0;
}

static int irrx_resume(struct device *dev)
{
	struct cvi_irrx *irrx = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(irrx->irq);
	else {
		clk_prepare_enable(irrx->clk);
		writel_relaxed(0xff, irrx->base + IRRX_INT_EN);
		writel_relaxed(0x1, irrx->base + IRRX_ENABLE);
	}

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(irrx_dev_pm_ops, irrx_suspend, irrx_resume);

static const struct of_device_id cvi_irrx_of_match[] = {
	{
		.compatible = "cvitek,irrx",
	},
	{},
};

MODULE_DEVICE_TABLE(of, cvi_irrx_of_match);

static struct platform_driver
	cvi_irrx_driver = { .probe = cvi_irrx_probe,
			    .remove = cvi_irrx_remove,
			    .driver = {
				    .name = "cvi-irrx",
				    .pm = &irrx_dev_pm_ops,
				    .of_match_table = cvi_irrx_of_match,
			    } };

module_platform_driver(cvi_irrx_driver);

MODULE_AUTHOR("zixun.li@sophgo.com");
MODULE_DESCRIPTION("cvitek irrx	driver");
MODULE_LICENSE("GPL");
