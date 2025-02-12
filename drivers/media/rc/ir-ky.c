//*SPDX-License-Identifier: GPL-2.0 */
/*
 * ky ir-rx controller driver
 *
 * Copyright (C) 2023 KY Limited
 */
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/reset.h>
#include <media/rc-core.h>
#include <linux/rpmsg.h>

#define KY_IR_DEV "ky-ir"
#define KY_RIR_DEV "ky-rir"
#define IRQUP_MSG	"irqon"
#define STARTUP_MSG		"startup"
#define STARTUP_OK_MSG		"startup-ok"

/*Registers*/
#define KY_IRC_EN 0x0
#define KY_IR_CLKDIV 0x4
#define KY_IR_NOTHR 0x8
#define KY_IR_IDSTATE 0xc
#define KY_IR_FIFO 0x10
#define KY_IR_STS 0x14
#define KY_IR_CMP 0x18
#define KY_IR_INTEN 0x1c
#define KY_IR_INTSTA 0x20

#define KY_IR_ENABLE BIT(0)
#define STATE_IDLE BIT(0)
#define FIFO_EMPTY BIT(30)
#define FIFO_FULL BIT(31)
#define IR_IRQ_EN_PULLDOWN BIT(0)
#define IR_IRQ_EN_PULLUP BIT(1)
#define IR_IRQ_EN_COUNT BIT(2)
#define IR_IRQ_EN_MATCH BIT(3)
#define IR_FIFO_DATA(val) (val&GENMASK(5,0))
#define KY_IR_NOISE 1

/*ir interrupt status*/
#define IR_IRQ_PULLUP IR_IRQ_EN_PULLUP
#define IR_IRQ_PULLDOWN IR_IRQ_EN_PULLDOWN
#define IR_IRQ_COUNT IR_IRQ_EN_COUNT
#define IR_IRQ_MATCH IR_IRQ_EN_MATCH
#define IRQ_CLEAR_ALL      \
		(IR_IRQ_PULLUP   | \
		 IR_IRQ_PULLDOWN | \
		 IR_IRQ_COUNT    | \
		 IR_IRQ_MATCH)
#define IR_IRQ_ENALL IRQ_CLEAR_ALL

#define IR_FREQ 100000
#define CYCLE_TIME 1000000/IR_FREQ
#define KY_IR_TIMEOUT 10000
#define IRFIFO_DEF_CMP 1

static void *private_data[1];
static const struct of_device_id ky_rir_match[];

struct ky_ir {
	struct rc_dev *rc;
	void __iomem *base;
	int irq;
	int fifo_size;
	int clkdiv;
	int freq;
	const char      *map_name;
	struct clk *clk;
	struct reset_control *rst;
};

struct instance_data {
	struct rpmsg_device *rpdev;
	struct ky_ir *ir;
};

struct ky_ir_data {
	int fifo_size;
	void **pdate;
};

static irqreturn_t ky_ir_irq(int irqno,void *dev_id)
{
	u32 irq_flag,i,rc,fifo_status;
	u8 info;
	struct ky_ir *ir = dev_id;
	struct ir_raw_event rawir = {};

	irq_flag = readl(ir->base + KY_IR_INTSTA);

	/*clear all interrupt*/
	writel(IRQ_CLEAR_ALL,ir->base + KY_IR_INTSTA);

	/*store data from fifo*/
	fifo_status = readl(ir->base + KY_IR_STS);

	if(!(fifo_status & FIFO_EMPTY))
	{
		rc = IR_FIFO_DATA(fifo_status);
		for(i = 0 ; i < rc ; i++)
		{
			info = readb(ir->base + KY_IR_FIFO);
			rawir.pulse = !(info & 0x80);
			rawir.duration = (info & 0x7f)*ir->rc->rx_resolution;
			ir_raw_event_store_with_filter(ir->rc, &rawir);
		}
	}

	/*wake up ir-event thread*/
	ir_raw_event_handle(ir->rc);

	return IRQ_HANDLED;
}

static int ky_ir_config(struct rc_dev *rc_dev)
{
	struct ky_ir *ir = rc_dev->priv;

	/*set clkdiv*/
	ir->clkdiv = DIV_ROUND_UP(ir->freq, IR_FREQ) - 1;
	writel(ir->clkdiv, ir->base + KY_IR_CLKDIV);

	writel(IRQ_CLEAR_ALL,ir->base + KY_IR_INTSTA);

	/*set noise threshold*/
	writeb(KY_IR_NOISE, ir->base + KY_IR_NOTHR);

	writel(IRFIFO_DEF_CMP, ir->base + KY_IR_CMP);

	rc_dev->s_idle(rc_dev, true);

	return 0;
}

static void ky_ir_set_idle(struct rc_dev *rc_dev, bool idle)
{
	struct ky_ir *ir = rc_dev->priv;
	writel(idle, ir->base + KY_IR_IDSTATE);
}

static int ky_ir_hw_init(struct device *dev)
{
	struct ky_ir *ir = dev_get_drvdata(dev);
	int ret;

	ret = reset_control_deassert(ir->rst);
	if(ret)
		return ret;

	ret = clk_prepare_enable(ir->clk);
	if(ret){
		dev_err(dev,"failed to enable ir clk\n");
		goto exit;
	}

	ky_ir_config(ir->rc);
	writel(IR_IRQ_ENALL, ir->base + KY_IR_INTEN);
	writel(KY_IR_ENABLE, ir->base + KY_IRC_EN);
	return 0;

exit:
	reset_control_assert(ir->rst);

	return ret;
}

static void ky_ir_hw_exit(struct device *dev)
{
	struct ky_ir *ir = dev_get_drvdata(dev);

	clk_disable_unprepare(ir->clk);
	reset_control_assert(ir->rst);
}

static int ky_ir_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *dn = dev->of_node;
	const struct of_device_id *of_id;
	struct instance_data *idata;
	struct rpmsg_device *rpdev;
	struct ky_ir *ir;

	ir = devm_kzalloc(dev, sizeof(struct ky_ir), GFP_KERNEL);
	if (!ir)
		return -ENOMEM;

	ir->freq = 0;
	ir->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(ir->clk)) {
		dev_err(dev, "failed to get a ir clock.\n");
		return PTR_ERR(ir->clk);
	}
	ir->rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(ir->rst))
		return PTR_ERR(ir->rst);

	/* Base clock frequency (optional) */
	of_property_read_u32(dn, "clock-frequency", &ir->freq);
	if(!ir->freq) {
		ir->freq = clk_get_rate(ir->clk);
		if(!ir->freq) {
			dev_err(dev, "failed to get ir function clk-freq\n");
			return -ENODEV;
		}
	}

	dev_dbg(dev, "ir base clock frequency : %d Hz.\n", ir->freq);

	ir->base = devm_platform_ioremap_resource(pdev,0);
	if(IS_ERR(ir->base)){
		return PTR_ERR(ir->base);
	}

	ir->rc = rc_allocate_device(RC_DRIVER_IR_RAW);
	if (!ir->rc) {
		dev_err(dev, "failed to allocate device\n");
		return -ENOMEM;
	}

	ir->rc->priv = ir;
	ir->rc->device_name = KY_IR_DEV;
	ir->rc->input_phys = "ky-ir/input0";
	ir->rc->input_id.bustype = BUS_HOST;
	ir->rc->input_id.vendor = 0x0001;
	ir->rc->input_id.product = 0x0001;
	ir->rc->input_id.version = 0x0100;
	ir->map_name = of_get_property(dn, "linux,rc-map-name", NULL);
	ir->rc->map_name = ir->map_name ?: RC_MAP_EMPTY;
	ir->rc->dev.parent = dev;
	ir->rc->allowed_protocols = RC_PROTO_BIT_ALL_IR_DECODER;
	ir->rc->rx_resolution = CYCLE_TIME;
	ir->rc->timeout = KY_IR_TIMEOUT;
	ir->rc->driver_name = KY_IR_DEV;
	ir->rc->s_idle = ky_ir_set_idle;

	ret = rc_register_device(ir->rc);
	if (ret) {
		dev_err(dev, "failed to register rc device\n");
		goto free_dev;
	}

	platform_set_drvdata(pdev, ir);

	//ir->irq = platform_get_irq(pdev, 0);
	if (of_get_property(pdev->dev.of_node, "rcpu-ir", NULL)) {
		/* rcpu service */
		of_id = of_match_device(ky_rir_match, &pdev->dev);
		if (!of_id) {
			pr_err("Unable to match OF ID\n");
			ret = -ENODEV;
			goto free_dev;
		}

		idata = (struct instance_data *)(((struct ky_ir_data *)of_id->data)->pdate)[0];
		rpdev = idata->rpdev;
		idata->ir = ir;

		ret = rpmsg_send(rpdev->ept, STARTUP_MSG, strlen(STARTUP_MSG));
		if (ret) {
			dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
			goto free_dev;
		}

	} else {
		ir->irq = platform_get_irq(pdev, 0);
		ret = devm_request_irq(dev, ir->irq, ky_ir_irq, 0, KY_IR_DEV, ir);
		if (ret) {
			dev_err(dev, "failed request irq\n");
			goto free_dev;
		}
	}

	ret = ky_ir_hw_init(dev);

free_dev:
	rc_free_device(ir->rc);

	return ret;
}

static int ky_ir_remove(struct platform_device *pdev)
{
	struct ky_ir *ir = platform_get_drvdata(pdev);

	rc_unregister_device(ir->rc);
	ky_ir_hw_exit(&pdev->dev);

	return 0;
}

static void ky_ir_shutdown(struct platform_device *pdev)
{
	ky_ir_hw_exit(&pdev->dev);
}

static int __maybe_unused ky_ir_suspend(struct device *dev)
{
	ky_ir_hw_exit(dev);
	return 0;
}

static int __maybe_unused ky_ir_resume(struct device *dev)
{
	return ky_ir_hw_init(dev);
}

static SIMPLE_DEV_PM_OPS(ky_ir_pm_ops, ky_ir_suspend, ky_ir_resume);

static const struct ky_ir_data ky_x1_ir_data = {
	.fifo_size = 32,
};

static const struct of_device_id ky_ir_match[] = {
	{
		.compatible = "ky,x1-irc",
		.data = &ky_x1_ir_data,
	},
	{ /* end of list */ }
};

static struct platform_driver ky_ir_driver = {
	.probe = ky_ir_probe,
	.remove = ky_ir_remove,
	.shutdown = ky_ir_shutdown,
	.driver = {
		.name = KY_IR_DEV,
		.of_match_table = ky_ir_match,
		.pm = &ky_ir_pm_ops,
	},
};

module_platform_driver(ky_ir_driver);

static struct ky_ir_data ky_x1_rir_data = {
	.fifo_size = 32,
	.pdate = &private_data[0],
};

static const struct of_device_id ky_rir_match[] = {
	{
		.compatible = "ky,x1-rirc",
		.data = &ky_x1_rir_data,
	},
	{ /* end of list */ }
};

static struct platform_driver ky_rir_driver = {
	.probe = ky_ir_probe,
	.remove = ky_ir_remove,
	.shutdown = ky_ir_shutdown,
	.driver = {
		.name = KY_RIR_DEV,
		.of_match_table = ky_rir_match,
		.pm = &ky_ir_pm_ops,
	},
};

static struct rpmsg_device_id rpmsg_driver_rir_id_table[] = {
	{ .name	= "rir-service", .driver_data = 0, },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_rir_id_table);

static int rpmsg_rir_client_cb(struct rpmsg_device *rpdev, void *data,
		int len, void *priv, u32 src)
{
	struct instance_data *idata = dev_get_drvdata(&rpdev->dev);
	struct ky_ir *dev = idata->ir;
	int ret;

	ky_ir_irq(0, (void *)dev);

	ret = rpmsg_send(rpdev->ept, IRQUP_MSG, strlen(IRQUP_MSG));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int rpmsg_rir_client_probe(struct rpmsg_device *rpdev)
{
	struct instance_data *idata;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
					rpdev->src, rpdev->dst);

	idata = devm_kzalloc(&rpdev->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	dev_set_drvdata(&rpdev->dev, idata);
	idata->rpdev = rpdev;

	private_data[0] = (void *)idata;

	platform_driver_register(&ky_rir_driver);

	return 0;
}

static void rpmsg_rir_client_remove(struct rpmsg_device *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg rcan client driver is removed\n");
	platform_driver_unregister(&ky_rir_driver);
}

static struct rpmsg_driver rpmsg_rir_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_rir_id_table,
	.probe		= rpmsg_rir_client_probe,
	.callback	= rpmsg_rir_client_cb,
	.remove		= rpmsg_rir_client_remove,
};
module_rpmsg_driver(rpmsg_rir_client);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ky");
