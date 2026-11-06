#include "cif.h"

#define MAX_CIF_PROC_BUF 32
#define CIF_PROC_NAME "v4l2/mipi-rx"

int cif_log_lv = CIF_DEBUG;
module_param(cif_log_lv, int, 0644);

int mclk0 = CAMPLL_FREQ_NONE;
module_param(mclk0, int, 0644);
MODULE_PARM_DESC(mclk0, "cam0 mclk");

int mclk1 = CAMPLL_FREQ_NONE;
module_param(mclk1, int, 0644);
MODULE_PARM_DESC(mclk1, "cam1 mclk");

int mclk2 = CAMPLL_FREQ_NONE;
module_param(mclk2, int, 0644);
MODULE_PARM_DESC(mclk2, "cam2 mclk");

int mclk3 = CAMPLL_FREQ_NONE;
module_param(mclk3, int, 0644);
MODULE_PARM_DESC(mclk3, "cam3 mclk");

int mclk4 = CAMPLL_FREQ_NONE;
module_param(mclk4, int, 0644);
MODULE_PARM_DESC(mclk4, "cam4 mclk");

int mclk5 = CAMPLL_FREQ_NONE;
module_param(mclk5, int, 0644);
MODULE_PARM_DESC(mclk5, "cam5 mclk");

int mclk6 = CAMPLL_FREQ_NONE;
module_param(mclk6, int, 0644);
MODULE_PARM_DESC(mclk6, "cam6 mclk");

int lane_phase[LANE_SKEW_NUM] = {0x00, 0x03, 0x08, 0x00, 0x03};
int count;
module_param_array(lane_phase, int, &count, 0664);

int bypass_mac_clk;
module_param(bypass_mac_clk, int, 0644);
MODULE_PARM_DESC(bypass_mac_clk, "byass mac clk");

unsigned int max_mac_clk = 900;
module_param(max_mac_clk, uint, 0644);
MODULE_PARM_DESC(max_mac_clk, "max mac clk");

static irqreturn_t cif_isr(int irq, void *_link)
{
	struct cvi_link *link = (struct cvi_link *)_link;
	struct cif_ctx *ctx = &link->cif_ctx;

	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_ECC_ERR_MASK))
		link->sts_csi.errcnt_ecc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_CRC_ERR_MASK))
		link->sts_csi.errcnt_crc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_WC_ERR_MASK))
		link->sts_csi.errcnt_wc++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_HDR_ERR_MASK))
		link->sts_csi.errcnt_hdr++;
	if (cif_check_csi_int_sts(ctx, CIF_INT_STS_FIFO_FULL_MASK))
		link->sts_csi.fifo_full++;

	if (link->sts_csi.errcnt_ecc > 0xFFFF ||
		link->sts_csi.errcnt_crc > 0xFFFF ||
		link->sts_csi.errcnt_hdr > 0xFFFF ||
		link->sts_csi.fifo_full > 0xFFFF ||
		link->sts_csi.errcnt_wc > 0xFFFF) {

		cif_mask_csi_int_sts(ctx, 0x1F);
		dev_err(link->dev, "mask the interrupt since err cnt is full\n");
		dev_err(link->dev, "ecc = %u, crc = %u, wc = %u, hdr = %u, fifo_full = %u\n",
				link->sts_csi.errcnt_ecc,
				link->sts_csi.errcnt_crc,
				link->sts_csi.errcnt_wc,
				link->sts_csi.errcnt_hdr,
				link->sts_csi.fifo_full);
	}

	cif_clear_csi_int_sts(ctx);

	return IRQ_HANDLED;
}

static char irq_name[MAX_LINK_NUM][20] = {
	"cif-irq0",
	"cif-irq1",
	"cif-irq2",
	"cif-irq3",
	"cif-irq4",
	"cif-irq5",
	"cif-irq6",
	"cif-irq7"
};

static int _init_resource(struct platform_device *pdev)
{
#if (DEVICE_FROM_DTS)
	struct resource *res = NULL;
	void *reg_base[20];
	struct cvi_cif_dev *dev;
	int i;
	struct cvi_link *link;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata\n");
		return -EINVAL;
	}
#ifndef FPGA_PORTING
	/* cam clk shall not depend on the link so we separate them from link. */
	dev->clk_cam0.clk_o = devm_clk_get(&pdev->dev, "clk_cam0");
	if (!IS_ERR_OR_NULL(dev->clk_cam0.clk_o))
		dev_info(&pdev->dev, "cam0 clk installed\n");
	dev->clk_cam1.clk_o = devm_clk_get(&pdev->dev, "clk_cam1");
	if (!IS_ERR_OR_NULL(dev->clk_cam1.clk_o))
		dev_info(&pdev->dev, "cam1 clk installed\n");
	dev->clk_cam2.clk_o = devm_clk_get(&pdev->dev, "clk_cam2");
	if (!IS_ERR_OR_NULL(dev->clk_cam2.clk_o))
		dev_info(&pdev->dev, "cam2 clk installed\n");
	dev->vip_sys2.clk_o = devm_clk_get(&pdev->dev, "clk_sys_2");
	if (!IS_ERR_OR_NULL(dev->vip_sys2.clk_o))
		dev_info(&pdev->dev, "vip_sys_2 clk installed\n");
	dev->clk_mipimpll.clk_o = devm_clk_get(&pdev->dev, "clk_mipimpll");
	if (!IS_ERR_OR_NULL(dev->clk_mipimpll.clk_o))
		dev_info(&pdev->dev, "clk_mipimpll clk installed %p\n", dev->clk_mipimpll.clk_o);
	dev->clk_disppll.clk_o = devm_clk_get(&pdev->dev, "clk_disppll");
	if (!IS_ERR_OR_NULL(dev->clk_disppll.clk_o))
		dev_info(&pdev->dev, "clk_disppll clk installed %p\n", dev->clk_disppll.clk_o);
	dev->clk_fpll.clk_o = devm_clk_get(&pdev->dev, "clk_fpll");
	if (!IS_ERR_OR_NULL(dev->clk_fpll.clk_o))
		dev_info(&pdev->dev, "clk_fpll clk installed %p\n", dev->clk_fpll.clk_o);
#endif
	for (i = 0; i < (MAX_LINK_NUM * 2 - 1); ++i) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_info(&pdev->dev, "no memsource[%d], break\n", i);
			break;
		}
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		reg_base[i] = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
#else
		reg_base[i] = devm_ioremap_nocache(&pdev->dev, res->start, res->end - res->start);
#endif

		// dev_info(&pdev->dev,
		// 	 "(%d) res-reg: start: 0x%llx, end: 0x%llx.",
		// 	 i, res->start, res->end);
		// dev_info(&pdev->dev, " virt-addr(%p)\n", reg_base[i]);
	}
	if (i > 1)
		cif_set_base_addr(0, reg_base[0], reg_base[1]);
	if (i > 2)
		cif_set_base_addr(1, reg_base[2], reg_base[1]);
	if (i > 3)
		cif_set_base_addr(2, reg_base[3], reg_base[1]);
	if (i > 4)
		cif_set_base_addr(3, reg_base[4], reg_base[1]);
	if (i > 5)
		cif_set_base_addr(4, reg_base[5], reg_base[1]);
	if (i > 6)
		cif_set_base_addr(5, reg_base[6], reg_base[1]);
	if (i > 7)
		cif_set_base_addr(6, reg_base[7], reg_base[1]);
	if (i > 8)
		cif_set_base_addr(7, reg_base[8], reg_base[1]);
	if (i > 9)
		cif_set_base_addr(8, reg_base[9], reg_base[1]);
	/* init pad_ctrl. */
	res = platform_get_resource(pdev, IORESOURCE_MEM, i);
	if (!res) {
		dev_info(&pdev->dev, "no pad_ctrl for res_%d\n", i);
	} else {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
		dev->pad_ctrl = devm_ioremap(&pdev->dev, res->start, res->end - res->start);
#else
		dev->pad_ctrl = devm_ioremap_nocache(&pdev->dev, res->start, res->end - res->start);
#endif
		dev_info(&pdev->dev,
			 "pad-ctrl res-reg: start: 0x%llx, end: 0x%llx.",
			 res->start, res->end);
	}

	/* Init max mac clock. */
	if (max_mac_clk <= 150)
		dev->max_mac_clk = 150;
	else if (max_mac_clk <= 300)
		dev->max_mac_clk = 300;
	else if (max_mac_clk <= 600)
		dev->max_mac_clk = 600;
	else
		dev->max_mac_clk = 900;

	/* Interrupt */
	for (i = 0; i < CIF_MAX_CSI_NUM; ++i) {
		link = &dev->link[i];

		link->irq_num = platform_get_irq(pdev, i);
		if (link->irq_num < 0)
			break;
		if (devm_request_irq(&pdev->dev, link->irq_num, cif_isr, IRQF_SHARED, irq_name[i], link))
			break;
		// dev_info(&pdev->dev, "request irq-%d as %s\n",
		// 	 link->irq_num, irq_name[i]);

		/* set the port id */
		link->cif_ctx.mac_num = i;
	}

	/* reset pin */
	for (i = 0; i < MAX_LINK_NUM; ++i) {
		link = &dev->link[i];
		link->dev = &pdev->dev;
		link->mac_clk = RX_MAC_CLK_400M;
		link->snsr_rst_pin = of_get_named_gpio_flags(pdev->dev.of_node,
				"snsr-reset", i, &link->snsr_rst_pol);
		if (link->snsr_rst_pin < 0)
			break;

		if (gpio_request(link->snsr_rst_pin, "snsr-rst-gpio"))
			return 0;

		dev_info(&pdev->dev, "rst_pin = %d, pol = %d\n",
			link->snsr_rst_pin, link->snsr_rst_pol);
	}
#ifndef FPGA_PORTING
	/* sw reset */
	link = &dev->link[0];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy0");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy0 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb0");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy0 apb reset installed\n");
	link = &dev->link[1];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy1");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy1 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb1");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy1 apb reset installed\n");
	link = &dev->link[2];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy2");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy2 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb2");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy2 apb reset installed\n");
	link = &dev->link[3];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy3");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy3 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb3");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy3 apb reset installed\n");
	link = &dev->link[4];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy4");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy4 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb4");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy4 apb reset installed\n");
	link = &dev->link[5];
	link->phy_reset = devm_reset_control_get(&pdev->dev, "phy5");
	if (link->phy_reset)
		dev_info(&pdev->dev, "phy5 reset installed\n");
	link->phy_apb_reset = devm_reset_control_get(&pdev->dev, "phy-apb5");
	if (link->phy_apb_reset)
		dev_info(&pdev->dev, "phy5 apb reset installed\n");
#endif

#endif

	return 0;
}


static ssize_t cif_proc_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
	struct cvi_cif_dev *dev = PDE_DATA(file_inode(file));
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
	char txt_buff[MAX_CIF_PROC_BUF];

	count = simple_write_to_buffer(txt_buff, MAX_CIF_PROC_BUF, ppos,
					user_buf, count);

	dbg_hdler(dev, txt_buff);
#else

	dbg_hdler(dev, user_buf);
#endif

	return count;
}

static int proc_cif_open(struct inode *inode, struct file *file)
{
	struct cvi_cif_dev *dev = PDE_DATA(inode);

	return single_open(file, proc_cif_show, dev);
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops cif_proc_fops = {
	.proc_open		= proc_cif_open,
	.proc_read		= seq_read,
	.proc_write		= cif_proc_write,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations cif_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_cif_open,
	.read		= seq_read,
	.write		= cif_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static struct proc_dir_entry *cif_proc_entry;

static int cvi_cif_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct cvi_cif_dev *dev;

	/* allocate main cif state structure */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev = &pdev->dev;
	/* initialize locks */
	spin_lock_init(&dev->lock);
	mutex_init(&dev->mutex);
	/* initialize subdev device */
	dev_set_drvdata(&pdev->dev, dev);
	rc = cif_init_subdev(pdev, dev);
	if (rc < 0) {
		dev_err(&pdev->dev, "failed to register subdev\n");
		return rc;
	}

	rc = _init_resource(pdev);
	if (rc < 0) {
		dev_err(&pdev->dev, "Failed to init res for cif, %d\n", rc);
		return rc;
	}

#ifdef CONFIG_PROC_FS
	{
		cif_proc_entry = proc_create_data(CIF_PROC_NAME, 0, NULL,
						&cif_proc_fops, dev);
		if (!cif_proc_entry)
			dev_err(&pdev->dev, "cif: can't init procfs.\n");
	}
#endif

	return 0;
}

static int cvi_cif_remove(struct platform_device *pdev)
{
	struct cvi_cif_dev *dev;

	if (!pdev) {
		dev_err(&pdev->dev, "invalid param");
		return -EINVAL;
	}

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get cvi_cif drvdata");
		return 0;
	}

	media_device_unregister(&dev->media_dev);
	v4l2_async_notifier_unregister(&dev->notifier);
	v4l2_async_notifier_cleanup(&dev->notifier);
	v4l2_device_unregister(&dev->v4l2_dev);
	v4l2_device_unregister_subdev(&dev->sd);
	media_entity_cleanup(&dev->sd.entity);
	media_device_cleanup(&dev->media_dev);
	dev_set_drvdata(&pdev->dev, NULL);

#ifdef CONFIG_PROC_FS
	proc_remove(cif_proc_entry);
#endif
	return 0;
}

static const struct of_device_id cvi_cif_dt_match[] = {
	{.compatible = "cvitek,cif_v4l2"},
	{}
};

#if (!DEVICE_FROM_DTS)
static void cvi_cif_pdev_release(struct device *dev)
{
}

static struct platform_device cvi_cif_pdev = {
	.name		= "cif_v4l2",
	.dev.release	= cvi_cif_pdev_release,
};
#endif

static struct platform_driver cvi_cif_pdrv = {
	.probe      = cvi_cif_probe,
	.remove     = cvi_cif_remove,
	.driver     = {
		.name		= "cif_v4l2",
		.owner		= THIS_MODULE,
#if (DEVICE_FROM_DTS)
		.of_match_table	= cvi_cif_dt_match,
#endif
	},
};

static int __init cvi_cif_init(void)
{
	int rc;

#if (DEVICE_FROM_DTS)
	rc = platform_driver_register(&cvi_cif_pdrv);
#else
	rc = platform_device_register(&cvi_cif_pdev);
	if (rc)
		return rc;

	rc = platform_driver_register(&cvi_cif_pdrv);
	if (rc)
		platform_device_unregister(&cvi_cif_pdev);
#endif

	return rc;
}

static void __exit cvi_cif_exit(void)
{
	platform_driver_unregister(&cvi_cif_pdrv);
#if (!DEVICE_FROM_DTS)
	platform_device_unregister(&cvi_cif_pdev);
#endif
}

MODULE_DESCRIPTION("Cvitek Camera Interface Driver");
MODULE_AUTHOR("Jie Lu");
MODULE_LICENSE("GPL");
module_init(cvi_cif_init);
module_exit(cvi_cif_exit);
