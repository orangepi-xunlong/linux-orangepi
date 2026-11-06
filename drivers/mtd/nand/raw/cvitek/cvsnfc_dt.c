/*
 * SPI NAND Flash Controller Device Driver for DT
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/err.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "cvsnfc.h"

bool otp_en = false;
struct cvsnfc_dt {
	struct cvsnfc_host cvsnfc;
};

static const struct of_device_id cvsnfc_dt_ids[] = {
	{ .compatible = "cvitek,cv1835-spinf"},
	{/* */}
};

MODULE_DEVICE_TABLE(of, cvsnfc_dt_ids);

int spi_general_execute(struct cvsnfc_host *host)
{
        cvsfc_write(host, REG_SPI_NAND_TRX_CTRL2, 0x0);
        cvsfc_write(host, REG_SPI_NAND_TRX_CTRL3, 0x0);
        cvsfc_write(host, REG_SPI_NAND_TRX_CMD0, SPI_NAND_CMD_PROGRAM_EXECUTE);

        cvsfc_write(host, REG_SPI_NAND_TRX_CTRL0,
                        cvsfc_read(host, REG_SPI_NAND_TRX_CTRL0) |
                        BIT_REG_TRX_START);

        cvsnfc_send_nondata_cmd_and_wait(host);
        return 0;
}

static int nand_show_sr2(struct seq_file *m, void *v)
{
        struct cvsnfc_host *host = m->private;
        uint32_t reg = 0;

	spi_feature_op(host, GET_OP, FEATURE_ADDR, &reg);

	seq_printf(m, "\tOTP read/write is %s\n", ((reg & STATUS_OTP_E_MASK) ? "enable" : "disable"));
	seq_printf(m, "\tOTP area lock is %s\n", ((reg & STATUS_OTP_L_MASK) ? "enable" : "disable"));

	return 0;
}

static int seq_otp_open(struct inode *inode, struct file *file)
{
        return single_open(file, nand_show_sr2, PDE_DATA(inode));
}

static ssize_t nand_otp_set(struct file *file, const char __user *data, size_t len, loff_t *off)
{
        int val;
        int ret = 0;
	uint32_t reg = 0;
	struct cvsnfc_host *host = PDE_DATA(file_inode(file));
	struct cvsnfc_chip_info *spi_nand = &host->spi_nand;
	struct spi_nand_driver *spi_driver = spi_nand->driver;

        ret = kstrtouint_from_user(data, len, 10, &val);

        switch (val) {
        case 0:
		spi_feature_op(host, GET_OP, FEATURE_ADDR, &reg);
		reg &=~(STATUS_OTP_E_MASK);
		spi_feature_op(host, SET_OP, FEATURE_ADDR, &reg);
		otp_en = false;
                break;
        case 1:
		spi_feature_op(host, GET_OP, FEATURE_ADDR, &reg);
		reg |= (STATUS_OTP_E_MASK);
		reg &= ~(STATUS_OTP_L_MASK);
		spi_feature_op(host, SET_OP, FEATURE_ADDR, &reg);
		otp_en = true;
                break;

	case 2:
		spi_feature_op(host, GET_OP, FEATURE_ADDR, &reg);
		if (reg & STATUS_OTP_E_MASK) {
			reg |= STATUS_OTP_L_MASK;
			spi_feature_op(host, SET_OP, FEATURE_ADDR, &reg);
			spi_driver->write_enable(host);
			spi_general_execute(host);
			ret = spi_driver->wait_ready(host);
			if (ret) {
				pr_err("wait ready fail! status:%d\n", ret);
				return -1;
			}
			otp_en = true;
		}
		break;
	case 3:
		spi_feature_op(host, GET_OP, FEATURE_ADDR, &reg);
		reg &= ~(STATUS_OTP_E_MASK | STATUS_OTP_L_MASK);
		spi_feature_op(host, SET_OP, FEATURE_ADDR, &reg);
		otp_en = true;
                break;

        default:
                dev_info(host->dev, "Incorrect input %d for /proc/nand_otp, please input 0 - 3\n", val);
                break;
        }

        return len;
}

const struct proc_ops nand_otp_proc_ops = {
	.proc_open = seq_otp_open,
	.proc_read = seq_read,
	.proc_write = nand_otp_set,
	.proc_release = single_release,
};

static int cvsnfc_dt_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	struct cvsnfc_dt *dt;
	struct cvsnfc_host *host;
	const struct of_device_id *of_id;
	struct mtd_info *mtd;
	struct proc_dir_entry *proc_otp = NULL;

	of_id = of_match_device(cvsnfc_dt_ids, &pdev->dev);

	if (of_id) {
		pdev->id_entry = of_id->data;
	} else {
		pr_err("Failed to find the right device id.\n");
		return -ENOMEM;
	}

	dt = devm_kzalloc(&pdev->dev, sizeof(*dt), GFP_KERNEL);
	if (!dt)
		return -ENOMEM;

	host = &dt->cvsnfc;
	host->dev = &pdev->dev;
	mtd = nand_to_mtd(&host->nand);
	mtd->priv = host;

	mtd->dev.of_node = pdev->dev.of_node;
	host->irq = platform_get_irq(pdev, 0);

	if (host->irq < 0) {
		dev_err(&pdev->dev, "no irq defined\n");
		return host->irq;
	}

	dev_info(host->dev, "IRQ: nr %d\n", host->irq);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->regbase = devm_ioremap_resource(host->dev, res);

	if (IS_ERR(host->regbase)) {
		dev_err(&pdev->dev, "devm_ioremap_resource res 0 failed\n");
		return PTR_ERR(host->regbase);
	}

	host->io_base_phy = res->start;
	host->nand.priv = host;

	cvsnfc_nand_init(&host->nand);

	ret = cvsnfc_host_init(host);
	if (ret) {
		pr_err("cvsnfc dt probe error\n");
		return ret;
	}

	ret = cvsnfc_scan_nand(host);
	if (ret) {
		pr_err("cvsnfc scan nand error\n");
		return ret;
	}

	cvsnfc_spi_nand_init(host);
	ret = mtd_device_register(mtd, NULL, 0);
	if (ret) {
		dev_err(host->dev, "mtd parse partition error\n");
		nand_cleanup(&host->nand);
		return ret;
	}

	proc_otp = proc_create_data("nand_otp", 0664, NULL, &nand_otp_proc_ops, (void *)host);
	if (!proc_otp)
		dev_err(host->dev, "Create cvsnfc nand otp proc failed!\n");

	platform_set_drvdata(pdev, dt);
	return 0;
}

static int cvsnfc_dt_remove(struct platform_device *pdev)
{
	struct cvsnfc_dt *dt = platform_get_drvdata(pdev);

	cvsnfc_remove(&dt->cvsnfc);

	return 0;
}

static struct platform_driver cvsnfc_dt_driver = {
	.probe          = cvsnfc_dt_probe,
	.remove         = cvsnfc_dt_remove,
	.driver         = {
		.name   = "cvsnfc",
		.of_match_table = cvsnfc_dt_ids,
	},
};

module_platform_driver(cvsnfc_dt_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CV");
MODULE_DESCRIPTION("DT driver for SPI NAND flash controller");

