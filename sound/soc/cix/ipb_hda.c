// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/acpi.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_runtime.h>
#include <linux/mfd/syscon.h>

#include "hdac.h"
#include "hda_pcm.h"
#include "hdacodec.h"

#define HDA_DRV_NAME "ipbloq-hda"

#define SKY1_AUDSS_CRU_INFO_REGMAP               0x24
#define SKY1_AUDSS_CRU_INFO_REGMAP_STEP          0x10000000

static const struct regmap_config ipb_hda_regmap_cfg = {
	.reg_bits	= 32,
	.val_bits	= 32,
	.reg_stride	= 4,
	.max_register	= 0xfffffff,
};

static int hda_dai_setfmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

static int hda_dai_startup(struct snd_pcm_substream *substream,
							struct snd_soc_dai *dai)
{
	return 0;
}

static void hda_dai_shutdown(struct snd_pcm_substream *substream,
							 struct snd_soc_dai *dai)
{

}

static int hda_dai_hw_params(struct snd_pcm_substream *substream,
							 struct snd_pcm_hw_params *params,
							 struct snd_soc_dai *dai)
{
	struct hda_ipbloq *p_ipb_hda = dev_get_drvdata(dai->dev);
	struct hdac *hdac = substream->private_data;
	struct hdac_bus *bus;

	hdac = &p_ipb_hda->hdac;
	bus = hdac_bus(hdac);

	return 0;
}

static int hda_dai_hw_free(struct snd_pcm_substream *substream,
						   struct snd_soc_dai *dai)
{
	return 0;
}

static int hda_dai_trigger(struct snd_pcm_substream *substream,
						   int cmd, struct snd_soc_dai *dai)
{
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* Enable HDA */
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* disable HDA */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int hda_clks_enable(struct hda_ipbloq *p_ipb_hda)
{
	int ret;

	ret = clk_prepare_enable(p_ipb_hda->sysclk);
	if (ret) {
		dev_err(p_ipb_hda->dev, "enabling sysclk failed\n");
		return ret;
	}

	ret = clk_prepare_enable(p_ipb_hda->clk48m);
	if (ret) {
		dev_err(p_ipb_hda->dev, "enabling clk48m failed\n");
		goto err_sysclk;
	}

	return 0;

err_sysclk:
	clk_disable_unprepare(p_ipb_hda->sysclk);

	return 0;
}

static int hda_clks_disable(struct hda_ipbloq *p_ipb_hda)
{
	clk_disable_unprepare(p_ipb_hda->sysclk);
	clk_disable_unprepare(p_ipb_hda->clk48m);

	return 0;
}

static void hda_sw_rst(struct hda_ipbloq *p_ipb_hda)
{
	/* reset */
	reset_control_assert(p_ipb_hda->hda_rst);

	usleep_range(1, 2);

	/* release reset */
	reset_control_deassert(p_ipb_hda->hda_rst);
}

static struct snd_soc_dai_ops hda_dai_ops = {
	.set_fmt   = hda_dai_setfmt,
	.startup   = hda_dai_startup,
	.shutdown  = hda_dai_shutdown,
	.hw_params = hda_dai_hw_params,
	.hw_free   = hda_dai_hw_free,
	.trigger   = hda_dai_trigger,
};

static struct snd_soc_dai_driver ipb_hda_dai_drv = {
	.name = HDA_DRV_NAME,
	.playback = {
		.stream_name	= "HDA Playback",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= HDA_RATES,
		.formats	= HDA_FORMATS,
	},
	.capture = {
		.stream_name	= "HDA Capture",
		.channels_min	= 1,
		.channels_max	= 16,
		.rates		= HDA_RATES,
		.formats	= HDA_FORMATS,
	},
	.id = 0,
	.ops = &hda_dai_ops,
};

static const struct snd_soc_component_driver ipb_hda_component = {
	.name = HDA_DRV_NAME,
	.open = hda_pcm_open,
	.close = hda_pcm_close,
	.hw_params = hda_pcm_hw_params,
	.hw_free = hda_pcm_hw_free,
	.prepare = hda_pcm_prepare,
	.trigger = hda_pcm_trigger,
	.pointer = hda_pcm_pointer,
	.pcm_construct  = hda_pcm_new,
	.pcm_destruct   = hda_pcm_free,
};

static const struct hda_ipbloq ipb_hda_data = {
	.comp_drv = &ipb_hda_component,
	.regmap_cfg = &ipb_hda_regmap_cfg,
};

static int phb_hda_init(struct hda_ipbloq *p_ipb_hda)
{
	struct hdac *hdac;
	struct hdac_bus *bus;
	unsigned short gcap;
	int cp_streams, pb_streams, start_idx;
	int err;

	if (!p_ipb_hda)
		return -EINVAL;

	hdac = &p_ipb_hda->hdac;
	bus = hdac_bus(hdac);

	hdac->single_cmd = 1;

	gcap = snd_hdac_chip_readw(bus, GCAP);
	dev_info(bus->dev, "chipset global capabilities = 0x%x\n", gcap);

	/* read number of streams from GCAP register */
	cp_streams = (gcap >> 8) & 0x0f;
	pb_streams = (gcap >> 12) & 0x0f;

	if (!pb_streams && !cp_streams) {
		dev_err(bus->dev, "no streams found in GCAP definitions?\n");
		return -EIO;
	}

	hdac->num_streams = cp_streams + pb_streams;
	bus->num_streams = cp_streams + pb_streams;

	/* initialize streams */
	snd_hdac_ext_stream_init_all(bus, 0, cp_streams, SNDRV_PCM_STREAM_CAPTURE);
	start_idx = cp_streams;
	snd_hdac_ext_stream_init_all(bus, start_idx, pb_streams, SNDRV_PCM_STREAM_PLAYBACK);

	cix_init_streams(hdac);
	err = snd_hdac_bus_alloc_stream_pages(bus);
	if (err < 0) {
		pr_err("hdac bus alloc stream failed\n");
		return err;
	}

	/* fix remap offset for hda dma */
	if (p_ipb_hda->cru_regmap) {
		int val;

		regmap_read(p_ipb_hda->cru_regmap,
			    SKY1_AUDSS_CRU_INFO_REGMAP, &val);
		p_ipb_hda->remap_offset =
			val * SKY1_AUDSS_CRU_INFO_REGMAP_STEP;
		hdac_bus_stream_fixedup_remap(bus, p_ipb_hda->remap_offset);
	}

	/* reset and start the controller */
	snd_hdac_bus_init_chip(bus, true);

	/* disable cmd io,  use pio now */
	snd_hdac_bus_stop_cmd_io(bus);

	if (!bus->codec_mask) {
		usleep_range(10000, 12000);
		bus->codec_mask = snd_hdac_chip_readw(bus, STATESTS);
		dev_info(bus->dev, "codecs retry: 0x%lx!\n", bus->codec_mask);
	}

	/* codec detection */
	if (!bus->codec_mask) {
		dev_err(bus->dev, "no codecs found!\n");
		return -ENODEV;
	}

	hdac->running = 1;

	if (hdac_acquire_irq(hdac) < 0) {
		dev_err(bus->dev, "failed to register hda irq");
		return -EINVAL;
	}

	/* Accept unsolicited responses */
	snd_hdac_chip_updatel(bus, GCTL, AZX_GCTL_UNSOL, AZX_GCTL_UNSOL);
	snd_hdac_chip_writeb(bus, RIRBCTL, AZX_RBCTL_DMA_EN | AZX_RBCTL_IRQ_EN);

	return err;
}

extern const struct hdac_bus_ops bus_core_ops;

//	TODO: to improve it that to pass hdac bus to codec by platform data
struct hdac_bus *ipb_hda_bus;
struct hdac_bus *get_ipb_hda_bus(void)
{
	return ipb_hda_bus;
}
EXPORT_SYMBOL_GPL(get_ipb_hda_bus);

struct snd_card *get_hda_card(void)
{
	struct hdac *hdac = bus_to_hdac(ipb_hda_bus);
	return hdac->card;
}
EXPORT_SYMBOL_GPL(get_hda_card);

static int ipb_hda_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hda_ipbloq *p_ipb_hda, *drvdata;
	struct hdac *hdac;
	struct hdac_bus *bus;
	struct resource *res;
	int ret;

	drvdata = (struct hda_ipbloq *)device_get_match_data(dev);
	if (!drvdata)
		return -EINVAL;

	p_ipb_hda = devm_kzalloc(dev, sizeof(struct hda_ipbloq), GFP_KERNEL);
	if (!p_ipb_hda)
		return -ENOMEM;
	p_ipb_hda->comp_drv = drvdata->comp_drv;
	p_ipb_hda->regmap_cfg = drvdata->regmap_cfg;
	p_ipb_hda->dev = dev;

	hdac = &(p_ipb_hda->hdac);
	bus = hdac_bus(hdac);
	hdac->dev = dev;
	mutex_init(&hdac->open_mutex);

	snd_hdac_ext_bus_init(bus, dev, &bus_core_ops, NULL);
	bus->use_posbuf = 1;
	bus->aligned_mmio = 1;

	platform_set_drvdata(pdev, p_ipb_hda);

	hdac->regs = devm_platform_get_and_ioremap_resource(pdev, 0, &res);
	if (IS_ERR(hdac->regs))
		return PTR_ERR(hdac->regs);

	hdac->regmap = devm_regmap_init_mmio(&pdev->dev, hdac->regs,
					     p_ipb_hda->regmap_cfg);
	if (IS_ERR(hdac->regmap)) {
			return dev_err_probe(dev, PTR_ERR(hdac->regmap),
					     "Failed to initialise regmap\n");
	}

	bus->remap_addr = hdac->regs;
	bus->addr = res->start;

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (!ACPI_COMPANION(dev)) {
		ret = of_reserved_mem_device_init(dev);
		if (ret && ret != -ENODEV) {
			dev_err(dev, "Failed to init reserved mem for DMA\n");
			return ret;
		}
	}

	hdac->irq = platform_get_irq(pdev, 0);
	if (hdac->irq < 0) {
		dev_err(dev, "failed to get irq:%d", hdac->irq);
		return hdac->irq;
	}

	p_ipb_hda->sysclk = devm_clk_get(dev, "sysclk");
	if (IS_ERR(p_ipb_hda->sysclk)) {
		dev_err(dev, "failed to get sysclk: %ld\n",
			PTR_ERR(p_ipb_hda->sysclk));
		return PTR_ERR(p_ipb_hda->sysclk);
	}

	p_ipb_hda->clk48m = devm_clk_get(dev, "clk48m");
	if (IS_ERR(p_ipb_hda->clk48m)) {
		dev_err(dev, "failed to get clk48m: %ld\n",
			PTR_ERR(p_ipb_hda->clk48m));
		return PTR_ERR(p_ipb_hda->clk48m);
	}

	hda_clks_enable(p_ipb_hda);

	p_ipb_hda->pdb_gpiod = devm_gpiod_get_optional(dev, "pdb", GPIOD_OUT_HIGH);
	if (IS_ERR(p_ipb_hda->pdb_gpiod)) {
		ret = PTR_ERR(p_ipb_hda->pdb_gpiod);
		dev_err(dev, "failed to pdb gpio, ret: %d\n", ret);
		return ret;
	}
	msleep(20);

	p_ipb_hda->cru_regmap =
		device_syscon_regmap_lookup_by_property(dev, "cru-ctrl");
	if (PTR_ERR(p_ipb_hda->cru_regmap) == -ENODEV)
			p_ipb_hda->cru_regmap = NULL;
	else if (IS_ERR(p_ipb_hda->cru_regmap))
			ret = PTR_ERR(p_ipb_hda->cru_regmap);

	p_ipb_hda->hda_rst = devm_reset_control_get(dev, "hda");
	if (IS_ERR(p_ipb_hda->hda_rst))
		return PTR_ERR(p_ipb_hda->hda_rst);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	if (!pm_runtime_enabled(dev))
		goto err_pm_runtime;

	pm_runtime_get_sync(dev);

	hda_sw_rst(p_ipb_hda);

	phb_hda_init(p_ipb_hda);

	pm_runtime_put(dev);

	ipb_hda_bus = bus;

	ret = devm_snd_soc_register_component(dev, &ipb_hda_component,
					       &ipb_hda_dai_drv, 1);

	dev_info(dev, "IPB_HDA driver probed\n");

	return 0;

err_pm_runtime:
	pm_runtime_disable(dev);

	return ret;
}

static int ipb_hda_remove(struct platform_device *pdev)
{
	struct hda_ipbloq *p_ipb_hda = dev_get_drvdata(&pdev->dev);
	struct hdac *hdac;
	struct hdac_bus *bus;
	int ret = 0;

	hdac = &(p_ipb_hda->hdac);
	bus = hdac_bus(hdac);

	hdac_clear_irq(hdac);
	cix_free_streams(hdac);

	pm_runtime_disable(&pdev->dev);

	return ret;
}

/*
 * power management
 */
static int __maybe_unused ipb_hda_runtime_suspend(struct device *dev)
{
	struct hda_ipbloq *p_ipb_hda = dev_get_drvdata(dev);
	struct hdac *hdac = &(p_ipb_hda->hdac);
	struct hdac_bus *bus = hdac_bus(hdac);

	gpiod_set_value(p_ipb_hda->pdb_gpiod, 0);

	if (hdac->running) {
		snd_hdac_bus_stop_chip(bus);
		snd_hdac_bus_enter_link_reset(bus);

		hdac->running = 0;
	}

	hda_clks_disable(p_ipb_hda);

	return 0;
}

static int __maybe_unused ipb_hda_runtime_resume(struct device *dev)
{
	struct hda_ipbloq *p_ipb_hda = dev_get_drvdata(dev);
	struct hdac *hdac = &(p_ipb_hda->hdac);
	struct hdac_bus *bus = hdac_bus(hdac);
	int ret = 0;

	gpiod_set_value(p_ipb_hda->pdb_gpiod, 1);

	msleep(20);

	ret = hda_clks_enable(p_ipb_hda);

	hda_sw_rst(p_ipb_hda);

	if (!hdac->running) {
		/* reset and start the controller */
		snd_hdac_bus_init_chip(bus, true);

		/* disable cmd io,  use pio now */
		snd_hdac_bus_stop_cmd_io(bus);

		/* Accept unsolicited responses */
		snd_hdac_chip_updatel(bus, GCTL, AZX_GCTL_UNSOL, AZX_GCTL_UNSOL);
		snd_hdac_chip_writeb(bus, RIRBCTL, AZX_RBCTL_DMA_EN | AZX_RBCTL_IRQ_EN);

		hdac->running = 1;
	}

	return ret;
}

static const struct dev_pm_ops ipb_hda_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend, pm_runtime_force_resume)
	SET_RUNTIME_PM_OPS(ipb_hda_runtime_suspend, ipb_hda_runtime_resume, NULL)
};

static const struct of_device_id ipb_hda_of_match[] = {
	{
		.compatible = "ipbloq,hda",
		.data = &ipb_hda_data,
	},
	{}
};
MODULE_DEVICE_TABLE(of, ipb_hda_of_match);

static const struct acpi_device_id cix_ipb_hda_acpi_match[] = {
	{ "CIXH6020", (kernel_ulong_t)&ipb_hda_data },
	{},
};
MODULE_DEVICE_TABLE(acpi, cix_ipb_hda_acpi_match);

static struct platform_driver ipb_hda_drv = {
	.driver = {
		.name = HDA_DRV_NAME,
		.of_match_table = of_match_ptr(ipb_hda_of_match),
		.acpi_match_table = ACPI_PTR(cix_ipb_hda_acpi_match),
		.pm = &ipb_hda_pm_ops,
	},
	.probe = ipb_hda_probe,
	.remove = ipb_hda_remove,
};
module_platform_driver(ipb_hda_drv);

MODULE_DESCRIPTION("IPBloq HDA driver for Cix Technology");
MODULE_AUTHOR("Xing.Wang <xing.wang@cixtech.com>");
MODULE_LICENSE("GPL v2");
