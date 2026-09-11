// SPDX-License-Identifier: GPL-2.0
/*
 * Rockchip cpuinfo support
 *
 * Reads the SoC unique ID out of the OTP and folds it into the two CRC32
 * words that /proc/cpuinfo prints as "Serial", so a board can be identified
 * by reading a single file.
 *
 * Derived from the vendor kernel's rockchip-cpuinfo.c. Only the ID half is
 * kept here: the original also fills in CPU type, version and leakage, but
 * that leans on <linux/rockchip/cpu.h> and <linux/rockchip/rockchip_sip.h>,
 * two vendor headers this tree does not carry, and none of it is needed for
 * the serial number.
 *
 * The byte interleave below is deliberately the same one the vendor driver
 * does, so a board reports the same Serial here as it does on the vendor
 * kernel. The OTP stores the ID as 16-bit little-endian words and
 * nvmem_cell_read() hands back the raw bytes.
 *
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 */

#include <linux/crc32.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <asm/system_info.h>

#define RK_CPUINFO_ID_LEN	16

static int rockchip_cpuinfo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct nvmem_cell *cell;
	unsigned char *efuse_buf, buf[RK_CPUINFO_ID_LEN];
	size_t len = 0;
	int i;

	cell = nvmem_cell_get(dev, "id");
	if (IS_ERR(cell))
		return dev_err_probe(dev, PTR_ERR(cell), "failed to get id cell\n");

	efuse_buf = nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);

	if (IS_ERR(efuse_buf))
		return PTR_ERR(efuse_buf);

	if (len != RK_CPUINFO_ID_LEN) {
		kfree(efuse_buf);
		return dev_err_probe(dev, -EINVAL, "invalid id len: %zu\n", len);
	}

	for (i = 0; i < RK_CPUINFO_ID_LEN / 2; i++) {
		buf[i] = efuse_buf[1 + (i << 1)];
		buf[i + RK_CPUINFO_ID_LEN / 2] = efuse_buf[i << 1];
	}

	kfree(efuse_buf);

	system_serial_low = crc32(0, buf, RK_CPUINFO_ID_LEN / 2);
	system_serial_high = crc32(system_serial_low,
				   buf + RK_CPUINFO_ID_LEN / 2,
				   RK_CPUINFO_ID_LEN / 2);

	dev_info(dev, "Serial\t\t: %08x%08x\n",
		 system_serial_high, system_serial_low);

	return 0;
}

static const struct of_device_id rockchip_cpuinfo_of_match[] = {
	{ .compatible = "rockchip,cpuinfo", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rockchip_cpuinfo_of_match);

static struct platform_driver rockchip_cpuinfo_driver = {
	.probe = rockchip_cpuinfo_probe,
	.driver = {
		.name = "rockchip-cpuinfo",
		.of_match_table = rockchip_cpuinfo_of_match,
	},
};
module_platform_driver(rockchip_cpuinfo_driver);

MODULE_DESCRIPTION("Rockchip cpuinfo support");
MODULE_LICENSE("GPL");
