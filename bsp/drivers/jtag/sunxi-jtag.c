/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
* Allwinner JTAG Master driver.
*
* Copyright(c) 2022-2027 Allwinnertech Co., Ltd.
*
* This file is licensed under the terms of the GNU General Public
* License version 2.  This program is licensed "as is" without any
* warranty of any kind, whether express or implied.
*/

/* #define DEBUG */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>

#define JTAG_VERSION		"0.0.1"
#define SCAN_IR			_IOWR('X', 1, unsigned int)
#define SCAN_DR			_IOWR('X', 2, unsigned int)
#define RESET			_IOWR('X', 3, unsigned int)

#define JTAGAP_CSW		0xd00
#define JTAGAP_PSEL		0xd04
#define JTAGAP_PSTA		0xd08
#define JTAGAP_BFIFO1		0xd10
#define JTAGAP_BFIFO2		0xd14
#define JTAGAP_BFIFO3		0xd18
#define JTAGAP_BFIFO4		0xd1c
#define JTAGAP_BFIFO(i)		(0xd10 + (0x4 * (i - 1)))
#define JTAGAP_IDR		0xdfc
#define	JTAGAP_ITSTATUS		0xefc
#define JTAGAP_ITCTRL		0xf00
#define JTAGAP_CLAIMSET		0xfa0
#define JTAGAP_CLAIMCLR		0xfa4
#define JTAGAP_DEVARCH		0xfbc
#define JTAGAP_DEVTYPE		0xfcc
#define	JTAGAP_PIDR4		0xfd0
#define JTAGAP_PIDR5		0xfd4
#define JTAGAP_PIDR6		0xfd8
#define	JTAGAP_PIDR7		0xfdc
#define JTAGAP_PIDR0		0xfe0
#define JTAGAP_PIDR1		0xfe4
#define JTAGAP_PIDR2		0xfe8
#define JTAGAP_PIDR3		0xfec
#define JTAGAP_CIDR0		0xff0
#define JTAGAP_CIDR1		0xff4
#define JTAGAP_CIDR2		0xff8
#define JTAGAP_CIDR3		0xffc

#define JTAGAP_IDLE		0xf0000000
#define JTAGAP_PSEL_PORT0	BIT(0)
#define JTAGAP_PORT_CONNECTED	BIT(3)
#define JTAGAP_TRST		BIT(1)
#define JTAGAP_RFIFOCNT_OFFSET	24
#define JTAGAP_RFIFOCNT_MASK	0x7
#define JTAGAP_TIMEOUT		100  /* ms */
#define JTAGAP_CLK_RATIO	2

/* TDI_TDO Packet: first byte */
#define TDI_TDO_FIRST_FLAG	(0x8 << 4)
#define TMS_HIGH_ON_LAST_CYCLE	(0x1 << 3)
#define TMS_LOW_ON_LAST_CYCLE	(0x0 << 3)
#define READ_TDO		(0x1 << 2)
#define NOT_READ_TDO		(0x0 << 2)
#define HOLD_TDI_HIGH		(0x1 << 1)
#define HOLD_TDI_LOW		(0x0 << 1)
#define USE_TDI			(0x1 << 0)
#define NOT_USE_TDI		(0x0 << 1)

/* TDI_TDO Packet: second byte */
#define TDI_TDO_SECOND_FLAG	(0x0 << 7)

/* TMS packet */
#define TMS_SINGLE_MAX_BITS	5
#define TMS_SINGLE_MASK		0x1f
#define TMS_TOTAL_MAX_BITS	32

#define MAX_TDI_BITS		(4 * 32)
#define JTAG_U32_MAX		4

struct sunxi_jtag {
	struct device *dev;
	struct clk *clk;
	struct reset_control *rst;
	struct miscdevice mdev;
	void __iomem *base;

	u32 jtag_freq;
};

struct jtag_user_data {
	u32 len;
	u32 val[JTAG_U32_MAX];
};

static int sunxi_jtag_is_idle(struct sunxi_jtag *chip)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(JTAGAP_TIMEOUT);
	u32 val;

	/* confirm jtag port in idle state */
	do {
		val = readl(chip->base + JTAGAP_CSW);
		if ((val & JTAGAP_IDLE) == 0)
			return 0;
	} while (time_before(jiffies, timeout));

	dev_err(chip->dev, "jtag port is busy\n");

	return -EBUSY;
}

static int sunxi_jtag_has_sufficient_data(struct sunxi_jtag *chip, u8 bytes)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(JTAGAP_TIMEOUT);
	u32 val;

	/* confirm jtag port has sufficient data */
	do {
		val = readl(chip->base + JTAGAP_CSW);
		if (((val >> JTAGAP_RFIFOCNT_OFFSET) & JTAGAP_RFIFOCNT_MASK) >= bytes)
			return 0;
	} while (time_before(jiffies, timeout));

	dev_err(chip->dev, "jtag not has sufficient data\n");

	return -EIO;
}

static int sunxi_jtag_tms_xmit(struct sunxi_jtag *chip, u32 tms, u8 bits)
{
	int ret;

	if (bits > TMS_TOTAL_MAX_BITS) {
		dev_err(chip->dev, "Error: tms bits must below 32\n");
		return -EINVAL;
	}

	/*
	 * The TMS packet is a single byte. The payload of the packet holds:
	 *
	 * 1.between one and five data bits to be send on TMS
	 * 2.an indication of whether TDI is held at 0 or 1 while these bits are sent
	 * 3.the highest bit must be 1
	 */
	while (bits > TMS_SINGLE_MAX_BITS) {
		writel((tms & TMS_SINGLE_MASK) | BIT(TMS_SINGLE_MAX_BITS), chip->base + JTAGAP_BFIFO1);
		bits -= TMS_SINGLE_MAX_BITS;
		tms >>= TMS_SINGLE_MAX_BITS;
	}

	/* send remaining bits */
	if (bits)
		writel((tms & TMS_SINGLE_MASK) | BIT(bits), chip->base + JTAGAP_BFIFO1);

	ret = sunxi_jtag_is_idle(chip);

	return ret;
}

static int sunxi_jtag_rw_fifo(struct sunxi_jtag *chip, u32 *val, u8 bits)
{
	int ret;
	void __iomem *jtag_fifo;
	const u32 max_bytes = 4;
	u32 bytes;

	/* one packet must below 4 bytes */
	bytes = min((u32)BITS_TO_BYTES(bits), max_bytes);
	jtag_fifo = chip->base + JTAGAP_BFIFO(bytes);

	dev_dbg(chip->dev, "tdi = 0x%x\n", *val);
	writel(*val, jtag_fifo);

	ret = sunxi_jtag_has_sufficient_data(chip, bytes);
	if (ret)
		return ret;

	*val = readl(jtag_fifo);
	dev_dbg(chip->dev, "tdo = 0x%x\n", *val);

	return 0;
}

static int sunxi_jtag_shift(struct sunxi_jtag *chip, u32 *val, u8 bits)
{
	int ret;
	u8 opcode, len;
	u32 bytes;

	while (bits > 0) {
		if (bits > MAX_TDI_BITS) {
			bytes = BITS_TO_BYTES(MAX_TDI_BITS);

			/* TDI_TDO First byte: opcode byte */
			opcode = (u8)(TDI_TDO_FIRST_FLAG |	/* TDI_TDO Packet Header */
				TMS_LOW_ON_LAST_CYCLE |		/* TMS low on last shift */
				READ_TDO |			/* Read TDO */
				HOLD_TDI_LOW |			/* TDI value (unused) */
				NOT_USE_TDI);			/* Don't use TDI bit */

			/* TDI_TDO Second byte: length byte */
			len = (u8)((TDI_TDO_SECOND_FLAG |	/* Normal format */
				(MAX_TDI_BITS - 1)));		/* 128 bit */
		} else {
			/* Create a numbits bit header */
			bytes = BITS_TO_BYTES(bits);

			opcode = (u8)(TDI_TDO_FIRST_FLAG |	/* TDI_TDO Packet Header */
				TMS_HIGH_ON_LAST_CYCLE |	/* TMS high on last shift */
				READ_TDO |			/* Read TDO */
				HOLD_TDI_LOW |			/* TDI value (unused) */
				NOT_USE_TDI);			/* Don't use TDI bit */

			len = (u8)((TDI_TDO_SECOND_FLAG |	/* Normal format */
				(bits - 1)));			/* numbits bits */
		}

		/* firts bytes and second bytes */
		writel((len << 8 | opcode), chip->base + JTAGAP_BFIFO2);

		while (bytes > 0) {
			/* More than four bytes */
			if (bytes >= 4) {
				ret = sunxi_jtag_rw_fifo(chip, val, bits);
				if (ret)
					return ret;

				val++;
				/* Minus already send bytes/bits, and continue... */
				bytes -= 4;
				bits -= 32;
			} else {
				/*
				 * Handle the final bits in this packet
				 * Ensure unused top bits are masked to zero
				 */
				*val &= ~((u32)(0xfffffffful << bits));

				ret = sunxi_jtag_rw_fifo(chip, val, bits);
				if (ret)
					return ret;

				bytes = 0;
				bits = 0;
			}
		}
	}

	return 0;
}

static int sunxi_jtag_scan_ir(struct sunxi_jtag *chip, u32 *inst, u8 bits)
{
	int ret;

	/* Run-Test-Idle -> Scan-IR: 0011 */
	ret = sunxi_jtag_tms_xmit(chip, 0x3, 4);
	if (ret)
		return ret;

	ret = sunxi_jtag_shift(chip, inst, bits);
	if (ret)
		return ret;

	/* Exit1-IR -> Run-Test-Idle: 01 */
	ret = sunxi_jtag_tms_xmit(chip, 0x1, 2);
	if (ret)
		return ret;

	return ret;
}

static int sunxi_jtag_scan_dr(struct sunxi_jtag *chip, u32 *val, u8 bits)
{
	int ret;

	/* Run-Test-Idle -> Scan-DR: 001 */
	ret = sunxi_jtag_tms_xmit(chip, 0x1, 3);
	if (ret)
		return ret;

	ret = sunxi_jtag_shift(chip, val, bits);
	if (ret)
		return ret;

	/* Exit1-DR -> Run-Test-Idle: 01 */
	ret = sunxi_jtag_tms_xmit(chip, 0x1, 2);
	if (ret)
		return ret;

	return ret;
}

static int sunxi_jtag_clk_init(struct sunxi_jtag *chip)
{
	int ret;

	ret = reset_control_reset(chip->rst);
	if (ret) {
		dev_err(chip->dev, "Error: failed to deassert apb2jtag rst\n");
		goto err0;
	}

	ret = clk_prepare_enable(chip->clk);
	if (ret) {
		dev_err(chip->dev, "Error: failed to enable apb2jtag rst\n");
		goto err1;
	}

	if (chip->jtag_freq) {
		ret = clk_set_rate(chip->clk, chip->jtag_freq * JTAGAP_CLK_RATIO);
		if (ret) {
			dev_err(chip->dev, "Error: failed to set apb2jtag rate");
			goto err2;
		}
	}

	return 0;

err2:
	clk_disable_unprepare(chip->clk);
err1:
	reset_control_assert(chip->rst);
err0:
	return ret;
}

static void sunxi_jtag_clk_exit(struct sunxi_jtag *chip)
{
	reset_control_assert(chip->rst);
	clk_disable_unprepare(chip->clk);
}

static int sunxi_jtag_reset(struct sunxi_jtag *chip)
{
	int ret;
	u32 val;

	ret = sunxi_jtag_is_idle(chip);
	if (ret)
		return ret;

	/* select jtag port 0 */
	val = readl(chip->base + JTAGAP_PSEL);
	writel(val | JTAGAP_PSEL_PORT0, chip->base + JTAGAP_PSEL);

	/* confirm port connected */
	val = readl(chip->base + JTAGAP_CSW);
	if (!(val & JTAGAP_PORT_CONNECTED)) {
		dev_err(chip->base, "Error: jtag port not connected\n");
		return -EINVAL;
	}

	val = readl(chip->base + JTAGAP_CSW);
	writel(val | JTAGAP_TRST, chip->base + JTAGAP_CSW);

	/* Any State -> Test-Logic-Rst: 11111 */
	ret = sunxi_jtag_tms_xmit(chip, 0x1f, 5);
	if (ret)
		return ret;

	/* clear nTRST */
	writel(0, chip->base + JTAGAP_CSW);

	/* Test-Logic-Rst -> Run-Test-Idle: 0 */
	ret = sunxi_jtag_tms_xmit(chip, 0x0, 1);
	if (ret)
		return ret;

	return 0;
}

static int sunxi_jtag_open(struct inode *inode, struct file *file)
{
	struct miscdevice *mdev = file->private_data;
	struct sunxi_jtag *chip = container_of(mdev, struct sunxi_jtag, mdev);

	file->private_data = chip;

	return 0;
}

static int sunxi_jtag_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long sunxi_jtag_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sunxi_jtag *chip = file->private_data;
	struct jtag_user_data user_data;
	int ret = 0;

	switch (cmd) {
	case SCAN_IR:
		ret = copy_from_user(&user_data, (void __user *)arg, sizeof(user_data));
		if (ret) {
			dev_err(chip->dev, "inst copy from user err\n");
			return -EFAULT;
		}

		dev_dbg(chip->dev, "inst = 0x%x, len = 0x%x\n", user_data.val[0], user_data.len);
		sunxi_jtag_scan_ir(chip, user_data.val, user_data.len);

		break;
	case SCAN_DR:
		ret = copy_from_user(&user_data, (void __user *)arg, sizeof(user_data));
		if (ret) {
			dev_err(chip->dev, "data copy from user err\n");
			return -EFAULT;
		}

		dev_dbg(chip->dev, "data[0] = 0x%x, data[1] = 0x%x, len = 0x%x\n",
				user_data.val[0], user_data.val[1], user_data.len);
		sunxi_jtag_scan_dr(chip, user_data.val, user_data.len);

		dev_dbg(chip->dev, "data[0] = 0x%x, data[1] = 0x%x\n",
				user_data.val[0], user_data.val[1]);
		ret = copy_to_user((void __user *)arg, &user_data, sizeof(user_data));
		if (ret) {
			dev_err(chip->dev, "tdo copy to user err\n");
			return -EFAULT;
		}
		break;
	case RESET:
		sunxi_jtag_reset(chip);
		break;
	default:
		ret = -EFAULT;
		dev_err(chip->dev, "Unspported cmd\n");
		break;
	}

	return ret;
}

struct file_operations sunxi_jtag_fops = {
	.owner = THIS_MODULE,
	.open = sunxi_jtag_open,
	.release = sunxi_jtag_release,
	.unlocked_ioctl = sunxi_jtag_ioctl,
};

static int sunxi_jtag_resource_get(struct sunxi_jtag *chip, struct platform_device *pdev)
{
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	chip->base = devm_ioremap_resource(&pdev->dev, res);
	if (!chip->base)
		return -ENOMEM;

	chip->clk = devm_clk_get(&pdev->dev, "apb2jtag-clk");
	if (IS_ERR(chip->clk)) {
		dev_err(chip->dev, "Error: failed to get apb2jtag clk\n");
		return PTR_ERR(chip->clk);
	}

	chip->rst = devm_reset_control_get(&pdev->dev, "apb2jtag-rst");
	if (IS_ERR(chip->rst)) {
		dev_err(chip->dev, "Error: failed to get apb2jtag rst\n");
		return PTR_ERR(chip->rst);
	}

	ret = of_property_read_u32(chip->dev->of_node, "jtag-freq", &chip->jtag_freq);
	if (ret)
		dev_warn(chip->dev, "Warning: failed to get jtag-freq, use default 24M\n");

	return 0;
}

static int sunxi_jtag_class_init(struct sunxi_jtag *chip)
{
	chip->mdev.parent	= chip->dev;
	chip->mdev.minor	= MISC_DYNAMIC_MINOR;
	chip->mdev.name		= "jtag";
	chip->mdev.fops		= &sunxi_jtag_fops;
	return misc_register(&chip->mdev);
}

static void sunxi_jtag_class_exit(struct sunxi_jtag *chip)
{
	misc_deregister(&chip->mdev);
}

static const struct of_device_id sunxi_jtag_match[] = {
	{ .compatible = "allwinner,jtag-master"},
	{ }
};
MODULE_DEVICE_TABLE(of, sunxi_jtag_match);

static int sunxi_jtag_probe(struct platform_device *pdev)
{
	struct sunxi_jtag *chip;
	int ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	platform_set_drvdata(pdev, chip);

	ret = sunxi_jtag_resource_get(chip, pdev);
	if (ret)
		return -EINVAL;

	ret = sunxi_jtag_class_init(chip);
	if (ret)
		return -EINVAL;

	ret = sunxi_jtag_clk_init(chip);
	if (ret)
		return ret;

	dev_info(&pdev->dev, "sunxi jtag probe success\n");
	return 0;
}

static int sunxi_jtag_remove(struct platform_device *pdev)
{
	struct sunxi_jtag *chip = platform_get_drvdata(pdev);

	sunxi_jtag_clk_exit(chip);
	sunxi_jtag_class_exit(chip);

	return 0;
}

static struct platform_driver sunxi_jtag_driver = {
	.probe = sunxi_jtag_probe,
	.remove = sunxi_jtag_remove,
	.driver = {
		.name = "sunxi-jtag",
		.of_match_table = sunxi_jtag_match,
	},
};
module_platform_driver(sunxi_jtag_driver);

MODULE_AUTHOR("xuminghui <xuminghui@allwinnertech.com");
MODULE_DESCRIPTION("Allwinner jtag master driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(JTAG_VERSION);
