
/* 
 ***************************************************************************************
 * 
 * cci_platform_drv.c
 * 
 * Hawkview ISP - cci_platform_drv.c module
 * 
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * 
 * Version		  Author         Date		    Description
 * 
 *   2.0		  Yang Feng   	2014/06/23	      Second Version
 * 
 ****************************************************************************************
 */
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "bsp_cci.h"
#include "cci_platform_drv.h"
#include "../vfe_os.h"
#include "../platform_cfg.h"
#define CCI_MODULE_NAME "vfe_cci"

//#ifdef USE_SPECIFIC_CCI

static void cci_release(struct device *dev)
{
	//struct vfe_dev *vfe_dev = (struct vfe_dev *)dev_get_drvdata(dev);
	vfe_print("cci_device_release\n");
};

#ifdef CCI_IRQ
static irqreturn_t cci_irq_handler(int this_irq, void * dev)
{
	unsigned long flags = 0;
	struct cci_dev *cci = (struct cci_dev *)dev;
	spin_lock_irqsave(&cci->slock, flags);
	bsp_cci_irq_process(cci->cci_sel);
	spin_unlock_irqrestore(&cci->slock, flags);
	return IRQ_HANDLED;
}
#endif

static int __devinit cci_probe(struct platform_device *pdev)
{
	struct cci_dev *cci = NULL;
	struct resource *res = NULL;
	struct cci_platform_data *pdata = NULL;
	int ret, irq = 0;
	if(pdev->dev.platform_data == NULL)
	{
		return -ENODEV;
	}
	pdata = pdev->dev.platform_data;
	vfe_print("cci probe start cci_sel = %d!\n",pdata->cci_sel);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (res == NULL || irq < 0) {
		return -ENODEV;
	}

	if (!request_mem_region(res->start, resource_size(res), res->name)) {
		return -ENOMEM;
	}
	cci = kzalloc(sizeof(struct cci_dev), GFP_KERNEL);
	if (!cci) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	cci->irq 		  = irq;
	cci->cci_sel = pdata->cci_sel;
	
	spin_lock_init(&cci->slock);
	init_waitqueue_head(&cci->wait);
	cci->base =  ioremap(res->start, resource_size(res));
	if (!cci->base) {
		ret = -EIO;
		goto eremap;
	}
#ifdef CCI_IRQ
	ret = request_irq(irq, cci_irq_handler, IRQF_DISABLED, CCI_MODULE_NAME, cci);
	if (ret) {
		vfe_err("[cci_%d] requeset irq failed!\n", cci->cci_sel);
		goto ereqirq;
	}
#endif
#if defined  (CONFIG_ARCH_SUN9IW1P1)
	ret = bsp_csi_cci_set_base_addr(cci->cci_sel, (unsigned int)cci->base);
	if(ret < 0)
		goto ehwinit;
#else
	ret = bsp_csi_cci_set_base_addr(0, (unsigned int)cci->base);
	if(ret < 0)
		goto ehwinit;

	ret = bsp_csi_cci_set_base_addr(1, (unsigned int)cci->base);
	if(ret < 0)
		goto ehwinit;
#endif
	platform_set_drvdata(pdev, cci);
	vfe_print("cci probe end cci_sel = %d!\n",pdata->cci_sel);
	return 0;
ehwinit:
#ifdef CCI_IRQ
	free_irq(irq, cci);
	ereqirq:
#endif

	iounmap(cci->base);

eremap:
	kfree(cci);

ekzalloc:
	vfe_print("cci probe err!\n");
	return ret;
}


static int __devexit cci_remove(struct platform_device *pdev)
{
	struct cci_dev *cci = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
#ifdef CCI_IRQ
	free_irq(cci->irq, cci);
#endif
	if(cci->base)
		iounmap(cci->base);
	kfree(cci);
	return 0;
}

#if defined  (CONFIG_ARCH_SUN9IW1P1)
static struct resource cci0_resource[] = 
{
	[0] = {
		.name		= "csi_cci",
		.start  = CSI0_CCI_REG_BASE,
		.end    = CSI0_CCI_REG_BASE + CSI0_CCI_REG_SIZE- 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = SUNXI_IRQ_CSI0_CCI,
		.end    = SUNXI_IRQ_CSI0_CCI,
		.flags  = IORESOURCE_IRQ,
	}, 
};

static struct resource cci1_resource[] = 
{
	[0] = {
		.name		= "csi_cci",
		.start  = CSI1_CCI_REG_BASE,
		.end    = CSI1_CCI_REG_BASE + CSI1_CCI_REG_SIZE- 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = SUNXI_IRQ_CSI1_CCI,
		.end    = SUNXI_IRQ_CSI1_CCI,
		.flags  = IORESOURCE_IRQ,
	}, 
};

static struct cci_platform_data cci0_pdata[] = {
	{
		.cci_sel  = 0,
	},
};

static struct cci_platform_data cci1_pdata[] = {
	{
		.cci_sel  = 1,
	},
};
static struct platform_device cci_device[] = {
	[0] = {
		.name  = CCI_MODULE_NAME,
		.id = 0,
		.num_resources = ARRAY_SIZE(cci0_resource),
		.resource = cci0_resource,
		.dev = {
			.platform_data  = cci0_pdata,
			.release        = cci_release,
		},
	},
	[1] = {
		.name = CCI_MODULE_NAME,
		.id = 1,
		.num_resources = ARRAY_SIZE(cci1_resource),
		.resource = cci1_resource,
		.dev = {
			.platform_data = cci1_pdata,
			.release = cci_release,
		},
	},
};

#else
static struct resource cci0_resource[] = 
{
	[0] = {
		.name		= "csi_cci",
		.start  = CSI0_CCI_REG_BASE,
		.end    = CSI0_CCI_REG_BASE + CSI0_CCI_REG_SIZE- 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = SUNXI_IRQ_CSI_CCI,
		.end    = SUNXI_IRQ_CSI_CCI,
		.flags  = IORESOURCE_IRQ,
	}, 
};


static struct cci_platform_data cci0_pdata[] = {
	{
		.cci_sel  = 0,
	},
};

static struct platform_device cci_device[] = {
	[0] = {
		.name  = CCI_MODULE_NAME,
		.id = 0,
		.num_resources = ARRAY_SIZE(cci0_resource),
		.resource = cci0_resource,
		.dev = {
			.platform_data  = cci0_pdata,
			.release        = cci_release,
		},
	},
};
#endif


static struct platform_driver cci_platform_driver = {
	.probe    = cci_probe,
	.remove   = __devexit_p(cci_remove),
	.driver = {
		.name   = CCI_MODULE_NAME,
		.owner  = THIS_MODULE,
	}
};

static int __init cci_init(void)
{
	int ret,i;
	for(i=0; i<ARRAY_SIZE(cci_device); i++) 
	{
		ret = platform_device_register(&cci_device[i]);
		if (ret)
			vfe_err("cci device %d register failed\n",i);
	}
	ret = platform_driver_register(&cci_platform_driver);
	if (ret) {
		vfe_err("platform driver register failed\n");
		return ret;
	}
	vfe_print("cci_init end\n");
	return 0;
}

static void __exit cci_exit(void)
{
	int i;
	vfe_print("cci_exit start\n");
	for(i=0; i<ARRAY_SIZE(cci_device); i++)
	{
		platform_device_unregister(&cci_device[i]);
	}
	platform_driver_unregister(&cci_platform_driver);
	vfe_print("cci_exit end\n");
}
module_init(cci_init);
module_exit(cci_exit);
//#endif

MODULE_AUTHOR("yangfeng");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Camera CCI DRIVER for sunxi");
