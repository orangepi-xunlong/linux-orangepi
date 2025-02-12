// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Ky Co., Ltd.
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/nvmem-consumer.h>


/*
 * soc information
 */
static struct ky_soc_info {
	u32 die_id;	/* die/wafer id */
	u32 ver_id;	/* die/wafer version */
	u32 pack_id;	/* package id */
	u32 svtdro;	/* dro vaule */
	u64 chipid;	/* chip serial number */
	u32 soc_id;	/* soc id */
	char *name;	/* chip name */
} soc_info;


static const struct ky_soc_id {
	const char *name;
	unsigned int id;
} soc_ids[] = {
	{ "unknown", 0x00000000 },

	/* x1 serial soc */
	{ "M1-8571", 0x36070000 },
	{ "K1-6370", 0x36070009 },
	{ "K1-6350", 0x36070012 },
	{ "K1-6371", 0x36070040 },
	{ "M103-6370", 0x36070109 },
	{ "M103-6371", 0x36070140 },
};


static const char *soc_id_to_chip_name(u32 die_id, u32 ver_id, u32 pack_id)
{
	int i;
	u32 soc_id;

	soc_id = (die_id << 16) | (pack_id);
	for (i = 0; i < ARRAY_SIZE(soc_ids); i++)
		if (soc_id == soc_ids[i].id)
			return soc_ids[i].name;

	/* the soc id is unkown */
	return soc_ids[0].name;
}

static int socinfo_get_nvparam(struct device *dev, char *cell_name, char *val, size_t size)
{
	size_t bytes;
	void *buf;
	struct nvmem_cell *cell;

	cell = devm_nvmem_cell_get(dev, cell_name);
	if (IS_ERR(cell)) {
		dev_err(dev, "devm_nvmem_cell_get %s failed\n", cell_name);
		return PTR_ERR(cell);
	}
	buf = nvmem_cell_read(cell, &bytes);
	if (IS_ERR(buf)) {
		dev_err(dev, "nvmem_cell_read %s failed\n", cell_name);
		return PTR_ERR(buf);
	}

	WARN_ON(size < bytes);
	if (bytes > size) {
		dev_err(dev, "buffer size is not enough for get %s\n", cell_name);
		bytes = size;
	}
	memcpy(val, buf, bytes);

	kfree(buf);

	return bytes;
}

static int ky_get_soc_info(struct device *dev)
{
	int size;

	memset(&soc_info, 0, sizeof(soc_info));
	size = socinfo_get_nvparam(dev, "soc_die_id",
			(char *)&soc_info.die_id, sizeof(soc_info.die_id));
	if (size <= 0) {
		dev_err(dev, "try to get soc_die_id from efuse failed\n");
	}

	size = socinfo_get_nvparam(dev, "soc_ver_id",
			(char *)&soc_info.ver_id, sizeof(soc_info.ver_id));
	if (size <= 0) {
		dev_err(dev, "try to get soc_ver_id from efuse failed\n");
	}

	size = socinfo_get_nvparam(dev, "soc_pack_id",
			(char *)&soc_info.pack_id, sizeof(soc_info.pack_id));
	if (size <= 0) {
		dev_err(dev, "try to get soc_pack_id from efuse failed\n");
	}

	size = socinfo_get_nvparam(dev, "soc_svt_dro",
			(char *)&soc_info.svtdro, sizeof(soc_info.svtdro));
	if (size <= 0) {
		dev_err(dev, "try to get soc_svt_dro from efuse failed\n");
	}

	size = socinfo_get_nvparam(dev, "soc_chip_id",
			(char *)&soc_info.chipid, sizeof(soc_info.chipid));
	if (size <= 0) {
		dev_err(dev, "try to get soc_chip_id from efuse failed\n");
	}

	soc_info.soc_id = (soc_info.die_id << 16) | soc_info.pack_id;

	return 0;
}


static int ky_socinfo_probe(struct platform_device *pdev)
{
	struct soc_device_attribute *soc_dev_attr;
	struct device *dev = &pdev->dev;
	struct soc_device *soc_dev;
	struct device_node *root;
	int ret;

	ret = ky_get_soc_info(dev);
	if (ret) {
		dev_err(dev, "try to get soc info failed!\n");
		return -EINVAL;
	}

	soc_dev_attr = devm_kzalloc(&pdev->dev, sizeof(*soc_dev_attr),
				    GFP_KERNEL);
	if (!soc_dev_attr) {
		return -ENOMEM;
	}

	/* setup soc information */
	root = of_find_node_by_path("/");
	of_property_read_string(root, "model", &soc_dev_attr->machine);
	of_node_put(root);

	soc_dev_attr->family = "ky socs";
	soc_dev_attr->revision = devm_kasprintf(&pdev->dev, GFP_KERNEL,
				"%c", (soc_info.ver_id + 'A') & 0xff);
	soc_dev_attr->soc_id = soc_id_to_chip_name(soc_info.die_id,
				soc_info.ver_id, soc_info.pack_id);
	soc_dev_attr->serial_number = kasprintf(GFP_KERNEL, "%016llX",
				soc_info.chipid);

	/* please note that the actual registration will be deferred */
	soc_dev = soc_device_register(soc_dev_attr);
	if (IS_ERR(soc_dev)) {
		return PTR_ERR(soc_dev);
	}
	platform_set_drvdata(pdev, soc_dev);

	dev_info(&pdev->dev, "Ky: CPU[%s] REV[%s] DRO[%u] Detected\n",
		 soc_dev_attr->soc_id, soc_dev_attr->revision, soc_info.svtdro);

	return 0;
}

static int ky_socinfo_remove(struct platform_device *pdev)
{
	struct soc_device *soc_dev = platform_get_drvdata(pdev);

	soc_device_unregister(soc_dev);

	return 0;
}

static const struct of_device_id ky_socinfo_dt_match[] = {
	{ .compatible = "ky,socinfo-x1",  },
	{ },
};

static struct platform_driver ky_socinfo_driver = {
	.driver = {
		.name   = "ky-socinfo",
		.of_match_table = ky_socinfo_dt_match,
	},

	.probe = ky_socinfo_probe,
	.remove = ky_socinfo_remove,
};
module_platform_driver(ky_socinfo_driver);

MODULE_DESCRIPTION("Ky soc information driver");
MODULE_LICENSE("GPL v2");