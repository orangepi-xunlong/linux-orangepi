/*
*
* Copyright 2024 Cix Technology Group Co., Ltd.
*
*/
#include "csi_dma_cap.h"
#include "csi_bridge_hw.h"
#include "csi_rcsu_hw.h"

#ifdef CONFIG_VI_BBOX
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_public_interface.h>
#endif

#ifdef CONFIG_VI_BBOX
#define CSI_DMA_RDR_NUM 36
static int rdr_writen_num = 0;
static u32 rdr_register_probe;
u64 g_csidma_addr;

enum RDR_CSIDMA_MODID {
	RDR_CSIDMA_MODID_START = PLAT_BB_MOD_CSIDMA_START,
	RDR_CSIDMA_SOC_ISR_ERR_MODID,
	RDR_CSIDMA_MODID_END = PLAT_BB_MOD_CSIDMA_END,
};

static struct rdr_register_module_result g_current_info;

static struct rdr_exception_info_s g_csidma_einfo[] = {
	{ { 0, 0 }, RDR_CSIDMA_SOC_ISR_ERR_MODID, RDR_CSIDMA_SOC_ISR_ERR_MODID, RDR_ERR,
	 RDR_REBOOT_NO, RDR_CSIDMA, RDR_CSIDMA, RDR_CSIDMA,
	 (u32)RDR_REENTRANT_DISALLOW, CSIDMA_S_EXCEPTION, 0, (u32)RDR_UPLOAD_YES,
	 "csidma", "csidma isr proc", 0, 0, 0 },
};

static struct completion g_rdr_dump_comp;

/*
 * Description : Dump function of the AP when an exception occurs
 */
static void cix_csidma_rproc_rdr_dump(u32 modid, u32 etype,
				   u64 coreid, char *log_path,
				   pfn_cb_dump_done pfn_cb)
{
	if (pfn_cb)
		pfn_cb(modid, coreid);
}

/*
 * Description : register exception with the rdr
 */
static void cix_csidma_rproc_rdr_register_exception(void)
{
	unsigned int i;
	int ret;

	for (i = 0; i < sizeof(g_csidma_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("register exception:%u", g_csidma_einfo[i].e_exce_type);
		ret = rdr_register_exception(&g_csidma_einfo[i]);
		if (ret == 0) {
			pr_err("rdr_register_exception fail, ret = [%d]\n", ret);
			return;
		}
	}
}

static void cix_csidma_rproc_rdr_unregister_exception(void)
{
	unsigned int i;

	for (i = 0; i < sizeof(g_csidma_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		pr_debug("unregister exception:%u", g_csidma_einfo[i].e_exce_type);
		rdr_unregister_exception(g_csidma_einfo[i].e_modid);
	}
}

/*
 * Description : Register the dump and reset functions to the rdr
 */
static int cix_csidma_rproc_rdr_register_core(void)
{
	struct rdr_module_ops_pub s_csidma_ops;
	struct rdr_register_module_result retinfo;
	u64 coreid = RDR_CSIDMA;
	int ret;

	s_csidma_ops.ops_dump = cix_csidma_rproc_rdr_dump;
	s_csidma_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(coreid, &s_csidma_ops, &retinfo);
	if (ret < 0) {
		pr_err("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	g_current_info.log_addr = retinfo.log_addr;
	g_current_info.log_len = retinfo.log_len;
	g_current_info.nve = retinfo.nve;

	g_csidma_addr =
		(uintptr_t)rdr_bbox_map(g_current_info.log_addr, g_current_info.log_len);
	if (!g_csidma_addr) {
		pr_err("hisi_bbox_map g_hisiap_addr fail\n");
		return -1;
	}

	pr_debug("%s,%d: addr=0x%llx   [0x%llx], len=0x%x\n",
		__func__, __LINE__, g_current_info.log_addr, g_csidma_addr, g_current_info.log_len);

	return ret;
}

static void cix_csidma_rproc_rdr_unregister_core(void)
{
	u64 coreid = RDR_CSIDMA;

	rdr_unregister_module_ops(coreid);
}

#endif

enum cix_dphy_pads {
	CIX_CSI_DMA_PAD_SINK,
	CIX_CSI_DMA_PAD_MAX,
};

static irqreturn_t csi_dma_irq_handler(int irq, void *priv)
{
	struct csi_dma_dev *csi_dma = priv;
	unsigned long flags;
	u32 status;
#ifdef CONFIG_VI_BBOX
	const u32 registers[CSI_DMA_RDR_NUM] = {
		0x00,  0x04,  0x08,  0x0C,  0x10,  0x14,  0x18,  0x1C,  0x20,
		0x24,  0x28,  0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x118,
		0x11C, 0x120, 0x124, 0x128, 0x12C, 0x130, 0x134, 0x138, 0x13C,
		0x200, 0x204, 0x208, 0x20C, 0x210, 0x214, 0x300, 0x304, 0x308};
	const char start_flag_str[] = "csiDmaDumpStart";
	int start_flag_len          = sizeof(start_flag_str) - 1;
	int start_flag_num          = (start_flag_len % 4 ? 1 : 0) + start_flag_len / 4;
	u64 write_shift             = g_csidma_addr + rdr_writen_num * 4;
	u32 val, i;
#endif
	spin_lock_irqsave(&csi_dma->slock, flags);

	status = csi_dma_get_irq_status(csi_dma);
	csi_dma->status = status;
	csi_dma_clean_irq_status(csi_dma, status);

	/* frame start interrupt */
	if (status & FRAME_START_INT_EN_MASK) {
		// TODO
	}

	/* frame end interrupt */
	if (status & FRAME_END_INT_EN_MASK) {
		csi_dma_cap_frame_write_done(csi_dma);
	}

	/* specific line number interrupt */
	if (status & LINE_CNT_INT_EN_MASK) {
		// TODO
	}

	/* every N lines interrupt */
	if (status & LINE_MODE_INT_EN_MASK) {
		// TODO
	}

	if (status & PIXEL_ERR_INT_EN_MAKS) {
		dev_err(csi_dma->dev, "csi_dma pixsel err \n");
	}

	if (status & ASYNC_FIFO_OVF_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma async FIFO overflow \n");
	}

	if (status & ASYNC_FIFO_UNDF_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma async FIFO underrun \n");
	}

	if (status & DMA_OVF_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma overflow \n");
	}

	if (status & DMA_UNDF_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma underrun \n");
	}

	if (status & FRAME_STOP_INT_EN_MASK) {
		// TODO
	}

	if (status & DMA_ERR_RST_INT_EN_MASK) {
		// TODO
	}

	if (status & UNSUPPORT_DT_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma unsupport date type \n");
	}

	if (status & UNSUPPORT_STRIDE_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma unsupport stride \n");
	}

	if (status & LINE_MISMATCH_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma line mismatch \n");
	}

	if (status & PIXEL_MISMATCH_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma pixsel mismatch \n");
	}

	if (status & TIMEOUT_INT_EN_MASK) {
		dev_err(csi_dma->dev, "csi_dma timeout \n");
	}
#ifdef CONFIG_VI_BBOX
	if (status & UNSUPPORT_DT_INT_EN_MASK \
		|| status & UNSUPPORT_STRIDE_INT_EN_MASK \
		|| status & LINE_MISMATCH_INT_EN_MASK \
		|| status & PIXEL_MISMATCH_INT_EN_MASK \
		|| status & TIMEOUT_INT_EN_MASK) {
		if (((rdr_writen_num + CSI_DMA_RDR_NUM + start_flag_num) * 4 <  g_current_info.log_len)) {
			memcpy((void*)write_shift, start_flag_str, start_flag_len);
			write_shift += start_flag_num * 4;
			rdr_writen_num += start_flag_num;

			for (i = 0; i < CSI_DMA_RDR_NUM; i++) {
				val = readl(csi_dma->regs + registers[i]);
				writel(val, (volatile void __iomem *)write_shift);
				write_shift += 0x4;
			}
			rdr_writen_num += CSI_DMA_RDR_NUM;
		}

		/* asynchronous api */
		rdr_system_error(RDR_CSIDMA_SOC_ISR_ERR_MODID, 0, 0);
	}
#endif

	spin_unlock_irqrestore(&csi_dma->slock, flags);

	return IRQ_HANDLED;
}
static int csi_dma_clk_get(struct csi_dma_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;

	csi_dma->sclk = devm_clk_get_optional(dev, "dma_sclk");
	if (IS_ERR(csi_dma->sclk)) {
		dev_err(dev, "failed to get csi bridge clk\n");
		return PTR_ERR(csi_dma->sclk);
	}

	csi_dma->apbclk = devm_clk_get_optional(dev, "dma_pclk");
	if (IS_ERR(csi_dma->apbclk)) {
		dev_err(dev, "failed to get csi bridge apbclk\n");
		return PTR_ERR(csi_dma->apbclk);
	}

	return 0;
}

static struct csi_dma_chan_src cix_sky1_chan_src = {
	.src_csi0 = 0,
	.src_csi2 = 2,
};

static struct csi_dma_dev_ops cix_sky1_clk_ops = {
	.clk_get     = csi_dma_clk_get,
	.clk_enable  = csi_dma_clk_enable,
	.clk_disable = csi_dma_clk_disable,
};

int csi_dma_parse_resets(struct csi_dma_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;
	struct reset_control *reset;

	reset = devm_reset_control_get_optional(dev, "csibridge_reset");
	if (IS_ERR(reset)) {
		if (PTR_ERR(reset) != -EPROBE_DEFER)
			dev_err(dev, "Failed to get sky1  reset control\n");
		return PTR_ERR(reset);
	}
	csi_dma->csibridge_reset = reset;
	return 0;
}

static struct csi_dma_rst_ops cix_sky1_rst_ops = {
	.parse  = csi_dma_parse_resets,
	.assert = csi_dma_resets_assert,
	.deassert = csi_dma_resets_deassert,
};

static struct csi_dma_plat_data csi_dma_data = {
	.ops      = &cix_sky1_clk_ops,
	.chan_src = &cix_sky1_chan_src,
	.rst_ops  = &cix_sky1_rst_ops,
};

static int csi_dma_pipeline_set_stream(struct csi_dma_pipeline *p, bool on)
{
	int loop;
	int ret;

	/*csi_dma -> sensor */
	for (loop = 0; loop < p->num_subdevs; loop++) {
		ret = v4l2_subdev_call(p->subdevs[loop], video, s_stream, on);
		if (ret) {
			//debug info
			break;
		}
	}

	return ret;
}

static int csi_dma_pipeline_prepare(struct csi_dma_pipeline *p)
{
	struct v4l2_subdev *sd;
	struct csi_dma_dev *csi_dma = container_of(p,struct csi_dma_dev,pipe);
	struct media_entity *me = &csi_dma->dma_cap->vdev.entity;
	struct media_pad *pad = NULL;
	struct device *dev = &csi_dma->pdev->dev;
	struct device *supply = dev;
	struct device *consumer = dev;
	int loop;

	p->num_subdevs = 0;
	memset(p->subdevs, 0, sizeof(p->subdevs));

	while (1) {

		/* Find remote source pad */
		for (loop = 0; loop < me->num_pads; loop++) {
			struct media_pad *s_pad = &me->pads[loop];
			if (!(s_pad->flags & MEDIA_PAD_FL_SINK))
				continue;

			pad = media_pad_remote_pad_unique(s_pad);
			if (pad)
				break;
		}

		if (!pad) {
			return -1;
		}

		sd = media_entity_to_v4l2_subdev(pad->entity);
		dev_info(dev,"get the subdev name %s entity \n",sd->name);
		p->subdevs[p->num_subdevs++] = sd;
		me = &sd->entity;

		/*add device link for pm */
		supply = sd->dev;
		device_link_add(consumer,supply,DL_FLAG_STATELESS);
		consumer = sd->dev;

		/*is the terminor subdev */
		if (me->function == MEDIA_ENT_F_CAM_SENSOR) {
			csi_dma->sensor_sd = sd;
			dev_info(dev, "pipeline get the sensor subdev\n");
			break;
		}
	}

	return 0;
}

static int csi_dma_pipeline_open(struct csi_dma_pipeline *p,
		struct v4l2_subdev **sensor_sd,bool prepare)
{
	return 0;
}

static int csi_dma_pipeline_close(struct csi_dma_pipeline *p)
{
	/*clean the all subdev information*/
	p->num_subdevs = 0;
	memset(p->subdevs, 0, sizeof(p->subdevs));
        return 0;
}

static int subdev_notifier_bound(struct v4l2_async_notifier *notifier,
                                 struct v4l2_subdev *subdev,
                                 struct v4l2_async_subdev *asd)
{
	struct csi_dma_dev *csi_dma = container_of(notifier,struct csi_dma_dev, notifier);
	struct device *dev = csi_dma->dev;
	int ret;

	dev_info(dev,"csi_dma bound enter\n");

	/*find remote entity source pad */
	csi_dma->dma_cap->source_pad = media_entity_get_fwnode_pad(&subdev->entity,subdev->fwnode,MEDIA_PAD_FL_SOURCE);

	if (csi_dma->dma_cap->source_pad < 0) {
		dev_err(dev, "Couldn't find output pad for subdev %s\n",
				subdev->name);
		return -1;
	}

	csi_dma->dma_cap->source_subdev = subdev;

	dev_info(dev,"csi_dma source name %s index %d sink name %s index %d \n",subdev->name,csi_dma->dma_cap->source_pad,csi_dma->dma_cap->vdev.name,CIX_CSI_DMA_PAD_SINK);
	/*create the link*/
	ret = media_create_pad_link(&subdev->entity,csi_dma->dma_cap->source_pad,
					&csi_dma->dma_cap->vdev.entity,CIX_CSI_DMA_PAD_SINK,
					MEDIA_LNK_FL_ENABLED | MEDIA_LNK_FL_IMMUTABLE);
	return ret;
}

static int subdev_notifier_complete(struct v4l2_async_notifier *notifier)
{
	struct csi_dma_dev *csi_dma = container_of(notifier,struct csi_dma_dev, notifier);
	struct device *dev = csi_dma->dev;
	int ret;
	dev_info(dev,"csi_dma complete enter\n");
	ret = v4l2_device_register_subdev_nodes(&csi_dma->v4l2_dev);
	csi_dma_pipeline_prepare(&csi_dma->pipe);

	return ret;
}

static const struct v4l2_async_notifier_operations subdev_notifier_ops = {
	.bound = subdev_notifier_bound,
	.complete = subdev_notifier_complete,
};

static int csi_dma_subdev_notifier(struct csi_dma_dev *csi_dma)
{
	struct v4l2_async_notifier *ntf = &csi_dma->notifier;
	struct device *dev = csi_dma->dev;
	int ret;

	v4l2_async_nf_init(ntf);

	ret = v4l2_async_nf_parse_fwnode_endpoints(
			dev, ntf,sizeof(struct v4l2_async_subdev),NULL);
	if (ret < 0) {
		dev_err(dev,"%s: parse fwnode failed\n", __func__);
		return ret;
	}

	ntf->ops = &subdev_notifier_ops;

	return v4l2_async_nf_register(&csi_dma->v4l2_dev, ntf);
}

static struct csi_rcsu_dev *rcsu_hw_attach(struct csi_dma_dev *csi_dma)
{
	struct platform_device *plat_dev;
	struct fwnode_handle *np;
	struct csi_rcsu_dev *rcsu_hw;
	struct device *dev = csi_dma->dev;
	struct device *tdev = NULL;

	np = fwnode_find_reference(dev->fwnode, "cix,hw", 0);
	if (IS_ERR(np) || !np->ops->device_is_available(np)) {
		dev_err(dev,"failed to get dphy%d hw node\n", csi_dma->id);
		return NULL;
	}

	tdev = bus_find_device_by_fwnode(&platform_bus_type, np);
	plat_dev = tdev ? to_platform_device(tdev) : NULL;
	
	fwnode_handle_put(np);
	if (!plat_dev) {
		dev_err(dev,"failed to get dphy%d hw from node\n",csi_dma->id);
		return NULL;
	}

	rcsu_hw = platform_get_drvdata(plat_dev);
	if (!rcsu_hw) {
		dev_err(dev,"failed attach dphy%d hw\n",csi_dma->id);
		return NULL;
	}

	dev_info(dev,"attach dphy%d hardware success \n", csi_dma->id);

	return rcsu_hw;
}

static int csi_dma_parse(struct csi_dma_dev *csi_dma)
{
	struct device *dev = &csi_dma->pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *res;
	struct reset_control *reset;
	int irq = 0;
	int ret = 0;

	if (has_acpi_companion(dev)) {
		ret = device_property_read_u32(dev, "csi-dma-id", &csi_dma->id);
	} else {
		ret = csi_dma->id = of_alias_get_id(node, CIX_BRIDGE_OF_NODE_NAME);
	}
	
	if ((ret < 0) || (csi_dma->id >= CIX_BRIDGE_MAX_DEVS)) {
		dev_err(dev, "Invalid driver data or device id (%d)\n",
			csi_dma->id);
		return -EINVAL;
	}

	dev_info(dev,"csi dma id %d \n",csi_dma->id);

	res = platform_get_resource(csi_dma->pdev, IORESOURCE_MEM, 0);
	csi_dma->regs = devm_ioremap_resource(dev, res);

	if (IS_ERR(csi_dma->regs)) {
		dev_err(dev, "Failed to get csi-bridge register map\n");
		return PTR_ERR(csi_dma->regs);
	}

	ret = device_property_read_u32(dev, "axi-uid", &csi_dma->axi_uid);
	if (ret < 0) {
		dev_err(dev, "%s failed to get axi user id property\n",
				__func__);
		return ret;
	}

	/*clk & reset*/
	csi_dma->sclk = devm_clk_get_optional(dev, "dma_sclk");
	if (IS_ERR(csi_dma->sclk)) {
		dev_err(dev, "failed to get csi bridge dma_sclk\n");
		return PTR_ERR(csi_dma->sclk);
	}

	csi_dma->apbclk = devm_clk_get_optional(dev, "dma_pclk");
	if (IS_ERR(csi_dma->apbclk)) {
		dev_err(dev, "failed to get csi bridge dma_pclk\n");
		return PTR_ERR(csi_dma->apbclk);
	}

	reset = devm_reset_control_get_optional(dev, "csibridge_reset");
	if (IS_ERR(reset)) {
		if (PTR_ERR(reset) != -EPROBE_DEFER)
			dev_err(dev, "Failed to get sky1  reset control\n");
		return PTR_ERR(reset);
	}

	csi_dma->csibridge_reset = reset;

	irq = platform_get_irq(csi_dma->pdev, 0);
	if (irq < 0) {
		dev_err(dev, ":irq = %d failed to get IRQ resource\n", irq);
		return -1;
	}

	ret = devm_request_irq(dev, irq, csi_dma_irq_handler,
			       IRQF_ONESHOT|IRQF_SHARED, dev_name(dev), csi_dma);
	if (ret) {
		dev_err(dev, "failed to install irq (%d)\n", ret);
		return -1;
	}

	return 0;
}

static const struct of_device_id csi_dma_of_match[] = {
	{.compatible = "cix,cix-bridge", .data = &csi_dma_data },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, csi_dma_of_match);

static const struct acpi_device_id csi_dma_acpi_match[] = {
	{ .id = "CIXH3028", .driver_data = (long unsigned int)&csi_dma_data },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, csi_dma_acpi_match);

static int csi_dma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct csi_dma_dev *csi_dma;
	struct v4l2_device *v4l2_dev;
	int ret = 0;

	csi_dma = devm_kzalloc(dev, sizeof(*csi_dma), GFP_KERNEL);
	if (!csi_dma)
		return -ENOMEM;

	csi_dma->pdev = pdev;
	csi_dma->dev = dev;
	csi_dma->pdata = (struct csi_dma_plat_data *)device_get_match_data(dev);

	if (!csi_dma->pdata) {
		dev_err(dev, "Can't get platform device data\n");
		return -EINVAL;
	}

	ret = csi_dma_parse(csi_dma);
	if (ret < 0)
		return ret;

	csi_dma->rcsu_dev = rcsu_hw_attach(csi_dma);
	if (!csi_dma->rcsu_dev) {
		dev_err(dev, "Can't attach rcsu hw device\n");
		return -EINVAL;
	}

	spin_lock_init(&csi_dma->slock);
	mutex_init(&csi_dma->lock);
	atomic_set(&csi_dma->usage_count, 0);

	/*csi_dma pipeline init*/
	csi_dma->pipe.open = csi_dma_pipeline_open;
	csi_dma->pipe.close = csi_dma_pipeline_close;
	csi_dma->pipe.set_stream = csi_dma_pipeline_set_stream;

	strlcpy(csi_dma->media_dev.model, dev_name(dev),sizeof(csi_dma->media_dev.model));	
	csi_dma->media_dev.dev = dev;

	v4l2_dev = &csi_dma->v4l2_dev;
	v4l2_dev->mdev = &csi_dma->media_dev;
	strlcpy(v4l2_dev->name, dev_name(dev),sizeof(v4l2_dev->name));
	ret = v4l2_device_register(dev, &csi_dma->v4l2_dev);
	if (ret < 0)
		return ret;

	media_device_init(&csi_dma->media_dev);
	ret = media_device_register(&csi_dma->media_dev);
	if (ret < 0) {
		dev_err(dev, "Failed to register media device: %d\n",ret);
		goto err_unreg_v4l2_dev;
	}

	ret = csi_dma_register_stream(csi_dma);
	if (ret) {
		dev_err(dev, "registered the stream failed\n");
		goto err_unreg_media_dev;
	}

	ret = csi_dma_subdev_notifier(csi_dma);
	if (ret) {
		dev_err(dev, "registered notifier failed\n");
		goto err_unreg_media_dev;
	}

	platform_set_drvdata(pdev, csi_dma);

	pm_runtime_enable(dev);

#ifdef CONFIG_VI_BBOX
	if (rdr_register_probe == 0) {

		init_completion(&g_rdr_dump_comp);

		cix_csidma_rproc_rdr_register_exception();

		ret = cix_csidma_rproc_rdr_register_core();
		if (ret) {
			dev_err(dev, "cix_csi_rproc_rdr_register_core fail, ret = [%d]\n", ret);
			return ret;
		}
		rdr_register_probe = 1;
	}
#endif

	dev_info(dev,"csi_dma.%d registered successfully\n",csi_dma->id);
	return 0;

err_unreg_media_dev:
	media_device_unregister(&csi_dma->media_dev);
err_unreg_v4l2_dev:
	v4l2_device_unregister(&csi_dma->v4l2_dev);

	return ret;
}

static int csi_dma_remove(struct platform_device *pdev)
{
	struct csi_dma_dev *csi_dma = platform_get_drvdata(pdev);

	csi_dma_unregister_stream(csi_dma);

	media_device_unregister(&csi_dma->media_dev);
	v4l2_async_nf_unregister(&csi_dma->notifier);
	v4l2_async_nf_cleanup(&csi_dma->notifier);
	v4l2_device_unregister(&csi_dma->v4l2_dev);

#ifdef CONFIG_VI_BBOX
	if (rdr_register_probe == 1) {
		cix_csidma_rproc_rdr_unregister_core();
		cix_csidma_rproc_rdr_unregister_exception();
		rdr_register_probe = 0;
	}
#endif
	pm_runtime_disable(&pdev->dev);
	dev_info(csi_dma->dev,"csi_dma remove \n");

	return 0;
}

#ifdef CONFIG_PM
static int csi_dma_rpm_suspend(struct device *dev)
{
	struct csi_dma_dev *csi_dma = dev_get_drvdata(dev);

	dev_info(dev, "%s() enter!\n", __func__);

	csi_dma_clk_disable(csi_dma);
	csi_dma_resets_assert(csi_dma);

	return 0;
}

static int csi_dma_rpm_resume(struct device *dev)
{
	struct csi_dma_dev *csi_dma = dev_get_drvdata(dev);
	int ret;
	u64 freq;

	dev_info(dev, "%s enter\n", __func__);
	ret = csi_dma_clk_enable(csi_dma);
	if (ret < 0) {
		dev_err(dev, "CSI_DMA_%d enable clocks fail\n", csi_dma->id);
		return ret;
	}

	ret = csi_dma_resets_deassert(csi_dma);
	if (ret < 0) {
		dev_err(dev, "CSI_DMA_%d deassert resets fail\n", csi_dma->id);
		return ret;
	}

	/*Convert frequency to HZ*/
	freq = csi_dma->sys_clk_freq * 1000 * 1000;
#ifdef CIX_VI_SET_RATE
	clk_set_rate(csi_dma->sclk,freq);
#endif
	dev_info(dev, "CSI_DMA sckl is %lld\n", freq);

	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP

static int csi_dma_suspend(struct device *dev)
{
	struct csi_dma_dev *csi_dma = dev_get_drvdata(dev);

	dev_info(dev, "%s enter!\n", __func__);

	if (csi_dma->stream_on == 1) {
		csi_dma_disable_irq(csi_dma);
		v4l2_subdev_call(csi_dma->sensor_sd, video,s_stream, 0);
		csi_dma_store(csi_dma);
	}

	return pm_runtime_force_suspend(dev);
}

static int csi_dma_resume(struct device *dev)
{
	struct csi_dma_dev *csi_dma = dev_get_drvdata(dev);

	dev_info(dev, "%s enter!\n", __func__);

	pm_runtime_force_resume(dev);

	if (csi_dma->stream_on == 1) {
		csi_dma_restore(csi_dma);
		csi_dma_bridge_start(csi_dma, &csi_dma->dma_cap->src_f);
		csi_dma_config_rcsu(csi_dma->dma_cap);
		v4l2_subdev_call(csi_dma->sensor_sd, video,s_stream, 1);
	}

	return 0;
}
#endif

static const struct dev_pm_ops csi_dma_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	SET_SYSTEM_SLEEP_PM_OPS(csi_dma_suspend, csi_dma_resume)
#endif

#ifdef CONFIG_PM
	SET_RUNTIME_PM_OPS(csi_dma_rpm_suspend, csi_dma_rpm_resume, NULL)
#endif
};

static struct platform_driver csi_dma_driver = {
	.probe		= csi_dma_probe,
	.remove		= csi_dma_remove,
	.driver = {
		.of_match_table = csi_dma_of_match,
		.acpi_match_table = ACPI_PTR(csi_dma_acpi_match),
		.name		= CSI_DMA_DRIVER_NAME,
		.pm = &csi_dma_pm_ops,
	}
};
module_platform_driver(csi_dma_driver);

MODULE_AUTHOR("CIX Tech.");
MODULE_DESCRIPTION("CIX CSI Bridge driver");
MODULE_LICENSE("GPL");
