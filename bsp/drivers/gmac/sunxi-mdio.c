/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner GMAC MDIO interface driver
 *
 * Copyright 2022 Allwinnertech
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/* #define DEBUG */
#define SUNXI_MODNAME "mdio"
#include <sunxi-log.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_address.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/iopoll.h>

#define SUNXI_MDIO_CONFIG		0x0
#define SUNXI_MDIO_DATA			0x4

#define SUNXI_MDIO_BUSY			0x00000001
#define SUNXI_MDIO_WRITE		0x00000002
#define SUNXI_MDIO_PHY_MASK		0x0000FFC0
#define SUNXI_MDIO_CR_MASK		0x0000001
#define SUNXI_MDIO_CLK			0x00000008
#define SUNXI_MDIO_MDC_DIV		0x3

/* bits 4 3 2 | AHB1 Clock	| MDC Clock
 * -------------------------------------------------------
 *      0 0 0 | 60 ~ 100 MHz	| div-42
 *      0 0 1 | 100 ~ 150 MHz	| div-62
 *      0 1 0 | 20 ~ 35 MHz	| div-16
 *      0 1 1 | 35 ~ 60 MHz	| div-26
 *      1 0 0 | 150 ~ 250 MHz	| div-102
 *      1 0 1 | 250 ~ 300 MHz	| div-124
 *      1 1 x | Reserved	|
 */
#define SUNXI_MDIO_MDC_DIV_RATIO_M	0x07
#define SUNXI_MDIO_MDC_DIV_RATIO_M_BIT	20
#define SUNXI_MDIO_PHY_ADDR		0x0001F000
#define SUNXI_MDIO_PHY_ADDR_OFFSET	12
#define SUNXI_MDIO_PHY_REG		0x000007F0
#define SUNXI_MDIO_PHY_REG_OFFSET	4
#define SUNXI_MDIO_RESET		0x4
#define SUNXI_MDIO_RESET_OFFSET		2

#define SUNXI_MDIO_WR_TIMEOUT		10000 /* us */

struct mii_reg_dump {
	u32 addr;
	u16 reg;
	u16 value;
};

struct sunxi_mdio {
	struct device *dev;
	void __iomem *base;
};

struct mii_reg_dump mii_reg;
/**
 * sunxi_parse_read_str - parse the input string for write attri.
 * @str: string to be parsed, eg: "0x00 0x01".
 * @addr: store the reg address. eg: 0x00.
 * @reg: store the expect value. eg: 0x01.
 *
 * return 0 if success, otherwise failed.
 */
static int sunxi_parse_read_str(char *str, u16 *addr, u16 *reg)
{
	char *ptr = str;
	char *tstr = NULL;
	int ret;

	/*
	 * Skip the leading whitespace, find the true split symbol.
	 * And it must be 'address value'.
	 */
	tstr = strim(str);
	ptr = strchr(tstr, ' ');
	if (!ptr)
		return -EINVAL;

	/*
	 * Replaced split symbol with a %NUL-terminator temporary.
	 * Will be fixed at end.
	 */
	*ptr = '\0';
	ret = kstrtos16(tstr, 16, addr);
	if (ret)
		goto out;

	ret = kstrtos16(skip_spaces(ptr + 1), 16, reg);

out:
	return ret;
}

/**
 * sunxi_parse_write_str - parse the input string for compare attri.
 * @str: string to be parsed, eg: "0x00 0x11 0x11".
 * @addr: store the address. eg: 0x00.
 * @reg: store the reg. eg: 0x11.
 * @val: store the value. eg: 0x11.
 *
 * return 0 if success, otherwise failed.
 */
static int sunxi_parse_write_str(char *str, u16 *addr,
			  u16 *reg, u16 *val)
{
	u16 result_addr[3] = { 0 };
	char *ptr = str;
	char *ptr2 = NULL;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(result_addr); i++) {
		ptr = skip_spaces(ptr);
		ptr2 = strchr(ptr, ' ');
		if (ptr2)
			*ptr2 = '\0';

		ret = kstrtou16(ptr, 16, &result_addr[i]);

		if (!ptr2 || ret)
			break;

		ptr = ptr2 + 1;
	}

	*addr = result_addr[0];
	*reg = result_addr[1];
	*val = result_addr[2];

	return ret;
}

/*
 * Wait until any existing MII operation is complete
 * Read 0 indicate finish in read or write operation
 * Read 1 indicate busy
 * */
static int sunxi_mdio_busy_wait(struct sunxi_mdio *chip)
{
	u32 reg;

	if (readl_poll_timeout(chip->base + SUNXI_MDIO_CONFIG, reg, !(reg & SUNXI_MDIO_BUSY), 100, SUNXI_MDIO_WR_TIMEOUT))
		return -EBUSY;

	return 0;
}

/**
 * sunxi_mdio_read - GMAC MII bus read func
 * @bus:	mii bus struct
 * @phyaddr:	phy address
 * @phyreg:	phy register
 *
 * Called when phy_write is used.
 *
 * Returns reg value for specific phy register.
 */
static int sunxi_mdio_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	unsigned int value = 0;
	struct sunxi_mdio *chip = bus->priv;
	int ret;

	/* Mask the MDC_DIV_RATIO */
	value |= ((SUNXI_MDIO_MDC_DIV & SUNXI_MDIO_MDC_DIV_RATIO_M) << SUNXI_MDIO_MDC_DIV_RATIO_M_BIT);
	value |= (((phyaddr << SUNXI_MDIO_PHY_ADDR_OFFSET) & (SUNXI_MDIO_PHY_ADDR)) |
			((phyreg << SUNXI_MDIO_PHY_REG_OFFSET) & (SUNXI_MDIO_PHY_REG)) |
			SUNXI_MDIO_BUSY);

	writel(value, chip->base + SUNXI_MDIO_CONFIG);

	ret = sunxi_mdio_busy_wait(chip);
	if (ret) {
		sunxi_err(chip->dev, "mdio read busy %#x\n", readl(chip->base + SUNXI_MDIO_CONFIG));
		return ret;
	}

	return (int)readl(chip->base + SUNXI_MDIO_DATA);
}

/**
 * sunxi_mdio_write - GMAC MII bus write func
 * @bus:	mii bus struct
 * @phyaddr:	phy address
 * @phyreg:	phy register
 * @data:	the value to be written to the register
 *
 * Called when phy_wirte is used.
 *
 * Returns 0 for a successful open, or appropriate error code
 */
static int sunxi_mdio_write(struct mii_bus *bus, int phyaddr, int phyreg, unsigned short data)
{
	unsigned int value;
	struct sunxi_mdio *chip = bus->priv;
	int ret;

	value = ((SUNXI_MDIO_MDC_DIV_RATIO_M << SUNXI_MDIO_MDC_DIV_RATIO_M_BIT) & readl(chip->base+ SUNXI_MDIO_CONFIG)) |
		 (SUNXI_MDIO_MDC_DIV << SUNXI_MDIO_MDC_DIV_RATIO_M_BIT);
	value |= (((phyaddr << SUNXI_MDIO_PHY_ADDR_OFFSET) & (SUNXI_MDIO_PHY_ADDR)) |
		  ((phyreg << SUNXI_MDIO_PHY_REG_OFFSET) & (SUNXI_MDIO_PHY_REG))) |
		  SUNXI_MDIO_WRITE | SUNXI_MDIO_BUSY;

	ret = sunxi_mdio_busy_wait(chip);
	if (ret) {
		sunxi_err(chip->dev, "mdio write wait busy %#x\n", readl(chip->base + SUNXI_MDIO_CONFIG));
		return ret;
	}

	/* Set the MII address register to write */
	writel(data, chip->base + SUNXI_MDIO_DATA);
	writel(value, chip->base + SUNXI_MDIO_CONFIG);

	ret = sunxi_mdio_busy_wait(chip);
	if (ret) {
		sunxi_err(chip->dev, "mdio write busy %#x\n", readl(chip->base + SUNXI_MDIO_CONFIG));
		return ret;
	}

	return 0;
}

static int sunxi_mdio_reset(struct mii_bus *bus)
{
	struct sunxi_mdio *chip = bus->priv;

	writel((SUNXI_MDIO_RESET << SUNXI_MDIO_RESET_OFFSET), chip->base + SUNXI_MDIO_CONFIG);

	sunxi_mdio_busy_wait(chip);
	return 0;
}

static ssize_t sunxi_mdio_read_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mii_bus *bus = platform_get_drvdata(pdev);

	mii_reg.value = sunxi_mdio_read(bus, mii_reg.addr, mii_reg.reg);
	return sprintf(buf, "ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n",
		       mii_reg.addr, mii_reg.reg, mii_reg.value);
}

static ssize_t sunxi_mdio_read_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	u16 reg, addr;
	char *ptr;

	ptr = (char *)buf;

	if (!dev) {
		pr_err("Argment is invalid\n");
		return count;
	}

	ret = sunxi_parse_read_str(ptr, &addr, &reg);
	if (ret)
		return ret;

	mii_reg.addr = addr;
	mii_reg.reg = reg;

	return count;
}

static ssize_t sunxi_mdio_write_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mii_bus *bus = platform_get_drvdata(pdev);
	u16 bef_val, aft_val;

	bef_val = sunxi_mdio_read(bus, mii_reg.addr, mii_reg.reg);
	sunxi_mdio_write(bus, mii_reg.addr, mii_reg.reg, mii_reg.value);
	aft_val = sunxi_mdio_read(bus, mii_reg.addr, mii_reg.reg);
	return sprintf(buf, "before ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n"
			    "after  ADDR[0x%02x]:REG[0x%02x] = 0x%04x\n",
			    mii_reg.addr, mii_reg.reg, bef_val,
			    mii_reg.addr, mii_reg.reg, aft_val);
}

static ssize_t sunxi_mdio_write_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	u16 reg, addr, val;
	char *ptr;

	ptr = (char *)buf;

	ret = sunxi_parse_write_str(ptr, &addr, &reg, &val);
	if (ret)
		return ret;

	mii_reg.reg = reg;
	mii_reg.addr = addr;
	mii_reg.value = val;

	return count;
}

static DEVICE_ATTR(mii_read, 0664, sunxi_mdio_read_show, sunxi_mdio_read_store);
static DEVICE_ATTR(mii_write, 0664, sunxi_mdio_write_show, sunxi_mdio_write_store);

static void sunxi_mdio_sysfs_create(struct device *dev)
{
	device_create_file(dev, &dev_attr_mii_read);
	device_create_file(dev, &dev_attr_mii_write);
}

static void sunxi_mdio_sysfs_destroy(struct device *dev)
{
	device_remove_file(dev, &dev_attr_mii_read);
	device_remove_file(dev, &dev_attr_mii_write);
}
/**
 * sunxi_mdio_probe - GMAC MII bus probe func
 *
 * sunxi mdio probe must run after sunxi emac probe,
 * because mdio clk was enabled in emac driver.
 */
static int sunxi_mdio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct mii_bus *bus;
	struct sunxi_mdio *chip;
	int ret;
#ifdef DEBUG
	struct phy_device *phy;
	int addr;
#endif

	sunxi_debug(dev, "%s() BEGIN\n", __func__);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	bus = devm_mdiobus_alloc(dev);
	if (!bus) {
		sunxi_err(dev, "Alloc mii bus failed\n");
		return -ENOMEM;
	}

	bus->name = dev_name(dev);
	bus->read = sunxi_mdio_read;
	bus->write = sunxi_mdio_write;
	bus->reset = sunxi_mdio_reset;
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s-mii", dev_name(&pdev->dev));
	bus->parent = &pdev->dev;
	bus->priv = chip;

	chip->dev = dev;
	chip->base = of_iomap(np, 0);
	if (IS_ERR(chip->base)) {
		ret = PTR_ERR(chip->base);
		goto iomap_err;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
	ret = of_mdiobus_register(bus, np);
#else
	ret = devm_of_mdiobus_register(dev, bus, np);
#endif
	if (ret < 0)
		goto mdio_register_err;

	platform_set_drvdata(pdev, bus);

	sunxi_mdio_sysfs_create(dev);

#ifdef DEBUG
	/* scan and dump the bus */
	for (addr = 0; addr < PHY_MAX_ADDR; addr++) {
		phy = mdiobus_get_phy(bus, addr);
		if (phy)
			sunxi_info(dev, "PHY ID: 0x%08x, ADDR: 0x%x, DEVICE: %s, DRIVER: %s\n",
				 phy->phy_id, addr, phydev_name(phy),
				 phy->drv ? phy->drv->name : "Generic PHY");
	}
#endif

	sunxi_debug(dev, "%s() SUCCESS\n", __func__);
	return 0;

mdio_register_err:
	iounmap(chip->base);
iomap_err:
	return ret;
}

static int sunxi_mdio_remove(struct platform_device *pdev)
{
	struct mii_bus *bus = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct sunxi_mdio *chip = bus->priv;

	sunxi_mdio_sysfs_destroy(dev);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 9, 0)
	mdiobus_unregister(bus);
#endif
	iounmap(chip->base);

	return 0;
}

static const struct of_device_id sunxi_mdio_dt_ids[] = {
	{ .compatible = "allwinner,sunxi-mdio" },
	{ }
};
MODULE_DEVICE_TABLE(of, sunxi_mdio_dt_ids);

static struct platform_driver sunxi_mdio_driver = {
	.probe = sunxi_mdio_probe,
	.remove = sunxi_mdio_remove,
	.driver = {
		.name = "sunxi-mdio",
		.of_match_table = sunxi_mdio_dt_ids,
	},
};

static int __init sunxi_mdio_init(void)
{
	return platform_driver_register(&sunxi_mdio_driver);
}
late_initcall(sunxi_mdio_init);

static void __exit sunxi_mdio_exit(void)
{
	platform_driver_unregister(&sunxi_mdio_driver);
}
module_exit(sunxi_mdio_exit);

MODULE_DESCRIPTION("Allwinner GMAC MDIO interface driver");
MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.4");
