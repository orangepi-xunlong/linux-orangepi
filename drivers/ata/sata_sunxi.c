/*
*************************************************************************************
*                         			      Linux
*					           AHCI SATA platform driver
*
*				        (c) Copyright 2006-2013, Allwinner Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sata_sunxi.c
*
* Author 		: danielwang
*
* Description 	: SATA Host Controller Driver for SUNXI Platform
*
* Notes         : 
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    danielwang        2011-6-29            1.0          create this file
*
* 2013.8.21 Mintow <duanmintao@allwinnertech.com>
*    Adapte to sun9iw1p1.
*
*************************************************************************************
*/

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>
#include <mach/sys_config.h>

#include "ahci.h"
#include "sata_sunxi.h"

enum {
	DBG_ERR  = 1U << 0,
	DBG_INFO = 1U << 1
};
static u32 gs_debug_mask = 1;
#define dprintk(level, fmt, arg...)	\
	do { \
		if (unlikely(gs_debug_mask & level)) { \
			printk("%s()%d - ", __func__, __LINE__); \
			printk(fmt, ##arg); \
		} \
	} while (0)
	
module_param_named(debug_mask, gs_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define AHCI_DBG(fmt, arg...)	dprintk(DBG_INFO, fmt, ##arg)
#define AHCI_ERR(fmt, arg...)	dprintk(DBG_ERR, fmt, ##arg)

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT("sunxi_sata"),
};

static int sunxi_ahci_enable = 0;
static struct pinctrl	    *sunxi_ahci_pctrl = NULL;
static struct pinctrl_state *sunxi_ahci_pctrl_state = NULL;

static struct resource sunxi_ahci_resources[] = {
#ifdef CONFIG_EVB_PLATFORM
	[0] = {
		.start	= SUNXI_SATA_PBASE,
		.end	= SUNXI_SATA_PBASE + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},

	[1] = {
    	.start	= SUNXI_IRQ_SATA,
    	.end	= SUNXI_IRQ_SATA,
    	.flags	= IORESOURCE_IRQ,
    },
#endif
};

static int sunxi_ahci_is_enable(void)
{
	return sunxi_ahci_enable;
}

static int sunxi_ahci_cfg_load(void)
{
	script_item_u used = {0};

	script_get_item(SUNXI_AHCI_DEV_NAME, "sata_used", &used);
	if (!used.val) {
		AHCI_ERR("AHCI is disable\n");
		return -EINVAL;
	}

	sunxi_ahci_enable = 1;
	return 0;
}

static int sunxi_ahci_request_gpio(struct device *dev)
{
	int ret = 0;

	if (sunxi_ahci_pctrl != NULL)
		return 0;

	if (!sunxi_ahci_is_enable())
		return -1;
	
	AHCI_DBG("Pinctrl init ... [%s]\n", dev->init_name);

	sunxi_ahci_pctrl = devm_pinctrl_get(dev);
	if (IS_ERR(sunxi_ahci_pctrl)) {
		AHCI_ERR("devm_pinctrl_get() failed! return %ld\n", PTR_ERR(sunxi_ahci_pctrl));
		return -1;
	}

	sunxi_ahci_pctrl_state = pinctrl_lookup_state(sunxi_ahci_pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(sunxi_ahci_pctrl_state)) {
		AHCI_ERR("pinctrl_lookup_state() failed! return %p \n", sunxi_ahci_pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(sunxi_ahci_pctrl, sunxi_ahci_pctrl_state);
	if (ret < 0)
		AHCI_ERR("pinctrl_select_state() failed! return %d \n", ret);
	
	return ret;
}

static void sunxi_ahci_release_gpio(void)
{
	devm_pinctrl_put(sunxi_ahci_pctrl);
	sunxi_ahci_pctrl = NULL;
}

static int sunxi_ahci_phy_init(unsigned int base)
{
	unsigned int tmp;
	const unsigned int timeout_val = 0x100000;
	unsigned int timeout = timeout_val;
	
	for(tmp=0; tmp<0x1000; tmp++);
	
	SUNXI_AHCI_ACCESS_LOCK(base, 0);
	
	tmp = ahci_readl(base, AHCI_REG_PHYCS1R);
	tmp |= (0x1<<19); 
	ahci_writel(base, AHCI_REG_PHYCS1R, tmp);
		
	tmp = ahci_readl(base, AHCI_REG_PHYCS0R);
	tmp |= 0x1<<23; 
	tmp |= 0x1<<18; 
	tmp &= ~(0x7<<24); 
	tmp |= 0x5<<24; 
	ahci_writel(base, AHCI_REG_PHYCS0R, tmp);
	
	tmp = ahci_readl(base, AHCI_REG_PHYCS1R);
	tmp &= ~(0x3<<16); 
	tmp |= (0x2<<16); 	
	tmp &= ~(0x1f<<8);
	tmp |= (6<<8); 	
	tmp &= ~(0x3<<6); 
	tmp |= (2<<6); 	
	ahci_writel(base, AHCI_REG_PHYCS1R, tmp);
	
	tmp = ahci_readl(base, AHCI_REG_PHYCS1R);
	tmp |= (0x1<<28); 
	tmp |= (0x1<<15); 
	ahci_writel(base, AHCI_REG_PHYCS1R, tmp);
	
	tmp = ahci_readl(base, AHCI_REG_PHYCS1R);
	tmp &= ~(0x1<<19); 
	ahci_writel(base, AHCI_REG_PHYCS1R, tmp);
		
	tmp = ahci_readl(base, AHCI_REG_PHYCS0R);
	tmp &= ~(0x7<<20); 
	tmp |= (0x03<<20);  
	ahci_writel(base, AHCI_REG_PHYCS0R, tmp);
		
	tmp = ahci_readl(base, AHCI_REG_PHYCS2R);
	tmp &= ~(0x1f<<5); 
	tmp |= (0x19<<5);  
	ahci_writel(base, AHCI_REG_PHYCS2R, tmp);
		
	for(tmp=0; tmp<0x1000; tmp++);
	
	tmp = ahci_readl(base, AHCI_REG_PHYCS0R);
	tmp |= 0x1<<19; 
	ahci_writel(base, AHCI_REG_PHYCS0R, tmp);
		
	timeout = timeout_val;
	do{
		tmp = ahci_readl(base, AHCI_REG_PHYCS0R);
		timeout --;
		if (!timeout) {
			AHCI_ERR("SATA AHCI Phy Power Failed!!\n");
			break;
		}
	}while((tmp&(0x7<<28))!=(0x02<<28)); 
	
	tmp = ahci_readl(base, AHCI_REG_PHYCS2R);
	tmp |= 0x1<<24; 
	ahci_writel(base, AHCI_REG_PHYCS2R, tmp);
	
	timeout = timeout_val;
	do{
		tmp = ahci_readl(base, AHCI_REG_PHYCS2R);
		timeout --;
		if (!timeout) {
			AHCI_ERR("SATA AHCI Phy Calibration Failed!!\n");
			break;
		}
	}while(tmp&(0x1<<24)); 
	
	for(tmp=0; tmp<0x3000; tmp++);
	
	SUNXI_AHCI_ACCESS_LOCK(base, 0x07);
	
	return 0;		
}

static int sunxi_ahci_start(struct device *dev, void __iomem *addr)
{
	struct clk *mclk;

	/*Enable mclk and hclk for AHCI*/
	mclk = clk_get(dev, SUNXI_AHCI_DEV_NAME);
	if (IS_ERR(mclk))
    {
    	AHCI_ERR("Error to get module clk for AHCI\n");
    	return -EINVAL;
    }
    
	clk_prepare_enable(mclk);
	
	sunxi_ahci_phy_init((unsigned int)addr);

	if (sunxi_ahci_request_gpio(dev) <= 0)
		return -EINVAL;
	
	clk_put(mclk);	
	return 0;
}

static void sunxi_ahci_stop(struct device *dev)
{
	struct clk *mclk;
		
	mclk = clk_get(dev, SUNXI_AHCI_DEV_NAME);
	if (IS_ERR(mclk))
    {
    	AHCI_ERR("Error to get module clk for AHCI\n");
    	return;
    }

	sunxi_ahci_release_gpio();
	
	/*Disable mclk and hclk for AHCI*/
	clk_disable_unprepare(mclk);
	clk_put(mclk);
}

void sunxi_ahci_release(struct device *dev)
{
	AHCI_DBG("Release. \n");
}

static struct ata_port_info sunxi_ahci_port_info = {
	.flags = AHCI_FLAG_COMMON,
	.pio_mask = ATA_PIO4,
	.udma_mask = ATA_UDMA6,
	.port_ops = &ahci_ops,
	.private_data = (void*)(AHCI_HFLAG_32BIT_ONLY | AHCI_HFLAG_NO_MSI 
							| AHCI_HFLAG_NO_PMP | AHCI_HFLAG_YES_NCQ),
};

static struct ahci_platform_data sunxi_ahci_platform_data = {
	.ata_port_info = &sunxi_ahci_port_info,
	.init = sunxi_ahci_start,
	.exit = sunxi_ahci_stop,
};

static struct platform_device sunxi_ahci_device = {
	.name		= SUNXI_AHCI_DEV_NAME,
	.id			= 0,
	.dev 		= {
		.platform_data = &sunxi_ahci_platform_data,
		.release = sunxi_ahci_release,
	},
	
	.num_resources	= ARRAY_SIZE(sunxi_ahci_resources),
	.resource		= sunxi_ahci_resources,
};

static int __devinit sunxi_ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev->platform_data;
	struct ata_port_info pi = {
		.flags		= AHCI_FLAG_COMMON,
		.pio_mask	= ATA_PIO4,
		.udma_mask	= ATA_UDMA6,
		.port_ops	= &ahci_ops,
	};
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		AHCI_ERR("no mmio space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		AHCI_ERR("no irq\n");
		return -EINVAL;
	}

	if (pdata && pdata->ata_port_info)
		pi = *pdata->ata_port_info;

	hpriv = devm_kzalloc(dev, sizeof(*hpriv), GFP_KERNEL);
	if (!hpriv) {
		AHCI_ERR("can't alloc ahci_host_priv\n");
		return -ENOMEM;
	}

	hpriv->flags |= (unsigned long)pi.private_data;

	hpriv->mmio = devm_ioremap(dev, mem->start, resource_size(mem));
	if (!hpriv->mmio) {
		AHCI_ERR("can't map %pR\n", mem);
		return -ENOMEM;
	}

	/*
	 * Some platforms might need to prepare for mmio region access,
	 * which could be done in the following init call. So, the mmio
	 * region shouldn't be accessed before init (if provided) has
	 * returned successfully.
	 */
	if (pdata && pdata->init) {
		rc = pdata->init(dev, hpriv->mmio);
		if (rc)
			return rc;
	}

	ahci_save_initial_config(dev, hpriv,
		pdata ? pdata->force_port_map : 0,
		pdata ? pdata->mask_port_map  : 0);

	/* prepare host */
	if (hpriv->cap & HOST_CAP_NCQ)
		pi.flags |= ATA_FLAG_NCQ;

	if (hpriv->cap & HOST_CAP_PMP)
		pi.flags |= ATA_FLAG_PMP;

	ahci_set_em_messages(hpriv, &pi);

	/* CAP.NP sometimes indicate the index of the last enabled
	 * port, at other times, that of the last possible port, so
	 * determining the maximum port number requires looking at
	 * both CAP.NP and port_map.
	 */
	n_ports = max(ahci_nr_ports(hpriv->cap), fls(hpriv->port_map));

	host = ata_host_alloc_pinfo(dev, ppi, n_ports);
	if (!host) {
		rc = -ENOMEM;
		goto err0;
	}

	host->private_data = hpriv;

	if (!(hpriv->cap & HOST_CAP_SSS) || ahci_ignore_sss)
		host->flags |= ATA_HOST_PARALLEL_SCAN;
	else
		AHCI_ERR("ahci: SSS flag set, parallel bus scan disabled\n");

	if (pi.flags & ATA_FLAG_EM)
		ahci_reset_em(host);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		ata_port_desc(ap, "mmio %pR", mem);
		ata_port_desc(ap, "port 0x%x", 0x100 + ap->port_no * 0x80);

		/* set enclosure management message type */
		if (ap->flags & ATA_FLAG_EM)
			ap->em_message_type = hpriv->em_msg_type;

		/* disabled/not-implemented port */
		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

	rc = ahci_reset_controller(host);
	if (rc)
		goto err0;

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_platform_sht);
	if (rc)
		goto err0;

	return 0;
err0:
	if (pdata && pdata->exit)
		pdata->exit(dev);
	return rc;
}

static int __devexit sunxi_ahci_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_platform_data *pdata = dev->platform_data;
	struct ata_host *host = dev_get_drvdata(dev);

	ata_host_detach(host);

	if (pdata && pdata->exit)
		pdata->exit(dev);

	return 0;
}

void sunxi_ahci_dump_reg(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data; 
	u32 base = (u32)hpriv->mmio;
	int i = 0;

	if (gs_debug_mask & DBG_INFO)
		for(i=0; i<0x200; i+=0x10) {
			printk("0x%08x: 0x%08x 0x%08x   0x%08x 0x%08x\n",
				base + i, ahci_readl(base, i), ahci_readl(base, i+4), 
				ahci_readl(base, i+8), ahci_readl(base, i+12));
		}
}

static ssize_t sunxi_ahci_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sunxi_ahci_dump_reg(dev);

	return snprintf(buf, PAGE_SIZE, "\n");
}
static struct device_attribute sunxi_ahci_status_attr =
	__ATTR(status, S_IRUGO, sunxi_ahci_status_show, NULL);

static void sunxi_ahci_sysfs(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_ahci_status_attr);
}

#ifdef CONFIG_PM

static int sunxi_ahci_suspend(struct device *dev)
{	
	AHCI_DBG("AHCI suspend.\n");
	//sunxi_ahci_dump_reg(dev);

	sunxi_ahci_stop(dev);
	return 0;
}

extern int ahci_hardware_recover_for_controller_resume(struct ata_host *host);
static int sunxi_ahci_resume(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data; 
		
	AHCI_DBG("AHCI resume.\n");

	sunxi_ahci_start(dev, hpriv->mmio);
	//sunxi_ahci_dump_reg(dev);	

	ahci_hardware_recover_for_controller_resume(host);
	return 0;
}

static const struct dev_pm_ops  sunxi_ahci_pmops = {
	.suspend	= sunxi_ahci_suspend,
	.resume		= sunxi_ahci_resume,
};

#define SUNXI_AHCI_PMOPS &sunxi_ahci_pmops
#else
#define SUNXI_AHCI_PMOPS NULL
#endif

static struct platform_driver sunxi_ahci_driver = {
	.remove = __devexit_p(sunxi_ahci_remove),
	.driver = {
		.name = SUNXI_AHCI_DEV_NAME,
		.owner = THIS_MODULE,
		.pm = SUNXI_AHCI_PMOPS,
	},
};

static int __init sunxi_ahci_init(void)
{
	sunxi_ahci_cfg_load();
	if (sunxi_ahci_is_enable() == 0)
		return -EACCES;
	
	platform_device_register(&sunxi_ahci_device);
	sunxi_ahci_sysfs(&sunxi_ahci_device);
	return platform_driver_probe(&sunxi_ahci_driver, sunxi_ahci_probe);
}
module_init(sunxi_ahci_init);

static void __exit sunxi_ahci_exit(void)
{
	if (sunxi_ahci_is_enable() == 0)
		return;

	platform_device_unregister(&sunxi_ahci_device);
	platform_driver_unregister(&sunxi_ahci_driver);
}
module_exit(sunxi_ahci_exit);

MODULE_DESCRIPTION("SUNXI AHCI driver");
MODULE_AUTHOR("Daniel Wang <danielwang@reuuimllatech.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sunxi_sata");
