// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.
#include "card-utils.h"

static int cix_asoc_card_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cix_asoc_card *priv;
	struct snd_soc_card *card;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	card = devm_kzalloc(dev, sizeof(*card), GFP_KERNEL);
	if (!card)
		return -ENOMEM;
	card->owner = THIS_MODULE;
	card->dev = dev;
	priv->card = card;

	platform_set_drvdata(pdev, card);
	snd_soc_card_set_drvdata(card, priv);

	if (ACPI_COMPANION(dev))
		ret = cix_card_parse_acpi(priv);
	else
		ret = cix_card_parse_of(priv);
	if (ret) {
		dev_err(dev, "cix_card_parse_of failed (%d)\n", ret);
		goto err;
	}

	ret = devm_snd_soc_register_card(dev, card);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "snd_soc_register_card failed (%d)\n", ret);
		goto err;
	}

	return 0;
err:
	return ret;
}

static const struct acpi_device_id sky1_acpi_match[] = {
	{ "CIXH6070", },
	{ },
};
MODULE_DEVICE_TABLE(acpi, sky1_acpi_match);

static const struct of_device_id sky1_of_match[] = {
	{
		.compatible = "cix,sky1-sound-card",
	}, {}
};
MODULE_DEVICE_TABLE(of, sky1_of_match);

static struct platform_driver sky1_card_pdrv = {
	.driver = {
		.name = "sky1-asoc-card",
		.of_match_table = sky1_of_match,
		.acpi_match_table = sky1_acpi_match,
		.pm = &snd_soc_pm_ops,
	},
	.probe = cix_asoc_card_probe,
};
module_platform_driver(sky1_card_pdrv);

MODULE_DESCRIPTION("Sky1 ALSA machine driver for Cix Technology");
MODULE_AUTHOR("Xing.Wang <xing.wang@cixtech.com>");
MODULE_LICENSE("GPL v2");
