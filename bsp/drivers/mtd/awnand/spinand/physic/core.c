// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

//#define pr_fmt(fmt) "sunxi-spinand-phy: " fmt
#define SUNXI_MODNAME "sunxi-spinand-phy"
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/aw-spinand.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

#include "physic.h"
#include "../sunxi-spinand.h"

struct spinanddbg_data {
	char param[32];
	char value[32];
	char status[512];
};
static struct spinanddbg_data spinanddbg_priv;

/**
 * aw_spinand_chip_update_cfg() - Update the configuration register
 * @chip: spinand chip structure
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int aw_spinand_chip_update_cfg(struct aw_spinand_chip *chip)
{
	int ret;
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;
	u8 reg;

	reg = 0;
	ret = ops->set_block_lock(chip, reg);
	if (ret)
		goto err;
	ret = ops->get_block_lock(chip, &reg);
	if (ret)
		goto err;
	sunxi_info(NULL, "block lock register: 0x%02x\n", reg);

	ret = ops->get_otp(chip, &reg);
	if (ret) {
		sunxi_err(NULL, "get otp register failed: %d\n", ret);
		goto err;
	}
	/* FS35ND01G ECC_EN not on register 0xB0, but on 0x90 */
	if (!strcmp(info->manufacture(chip), "Foresee")) {
		ret = ops->write_reg(chip, SPI_NAND_SETSR, FORESEE_REG_ECC_CFG,
				CFG_ECC_ENABLE);
		if (ret) {
			sunxi_err(NULL, "enable ecc for foresee failed: %d\n", ret);
			goto err;
		}
	} else {
		reg |= CFG_ECC_ENABLE;
	}
	if (!strcmp(info->manufacture(chip), "Winbond"))
		reg |= CFG_BUF_MODE;
	if (info->operation_opt(chip) & SPINAND_QUAD_READ ||
			info->operation_opt(chip) & SPINAND_QUAD_PROGRAM)
		reg |= CFG_QUAD_ENABLE;
	if (info->operation_opt(chip) & SPINAND_QUAD_NO_NEED_ENABLE)
		reg &= ~CFG_QUAD_ENABLE;
	ret = ops->set_otp(chip, reg);
	if (ret) {
		sunxi_err(NULL, "set otp register failed: val %d, ret %d\n", reg, ret);
		goto err;
	}
	ret = ops->get_otp(chip, &reg);
	if (ret) {
		sunxi_err(NULL, "get updated otp register failed: %d\n", ret);
		goto err;
	}
	sunxi_info(NULL, "feature register: 0x%02x\n", reg);

	return 0;
err:
	sunxi_err(NULL, "update config register failed\n");
	return ret;
}
EXPORT_SYMBOL(aw_spinand_chip_update_cfg);

static void aw_spinand_chip_clean(struct aw_spinand_chip *chip)
{
	aw_spinand_chip_cache_exit(chip);
	aw_spinand_chip_bbt_exit(chip);
}

int aw_spinand_fill_phy_info(struct aw_spinand_chip *chip, void *data)
{
	struct aw_spinand *spinand = get_aw_spinand();
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_phy_info *pinfo = info->phy_info;
	struct device_node *node = chip->spi->dev.of_node;
	boot_spinand_para_t *boot_info = data;
	int ret;
	unsigned int max_hz;

	ret = of_property_read_u32(node, "spi-max-frequency", &max_hz);
	if (ret < 0)
		sunxi_err(NULL, "get spi-max-frequency from node of spi-nand failed\n");

	ret = of_property_read_u32(node, "sample_mode",
				&spinand->right_sample_mode);
	if (ret) {
		sunxi_err(NULL, "Failed to get sample mode\n");
		spinand->right_sample_mode = AW_SAMP_MODE_DL_DEFAULT;
	}
	ret = of_property_read_u32(node, "sample_delay",
				&spinand->right_sample_delay);
	if (ret) {
		sunxi_err(NULL, "Failed to get sample delay\n");
		spinand->right_sample_delay = AW_SAMP_MODE_DL_DEFAULT;
	}

	/* nand information */
	boot_info->ChipCnt = 1;
	boot_info->ConnectMode = 1;
	boot_info->BankCntPerChip = 1;
	boot_info->DieCntPerChip = pinfo->DieCntPerChip;
	boot_info->PlaneCntPerDie = 2;
	boot_info->SectorCntPerPage = pinfo->SectCntPerPage;
	boot_info->ChipConnectInfo = 1;
	boot_info->PageCntPerPhyBlk = pinfo->PageCntPerBlk;
	boot_info->BlkCntPerDie = pinfo->BlkCntPerDie;
	boot_info->OperationOpt = pinfo->OperationOpt;
	boot_info->FrequencePar = max_hz / 1000 / 1000;
	boot_info->SpiMode = 0;
	info->nandid(chip, boot_info->NandChipId, 8);
	boot_info->pagewithbadflag = pinfo->BadBlockFlag;
	boot_info->MultiPlaneBlockOffset = 1;
	boot_info->MaxEraseTimes = pinfo->MaxEraseTimes;
	/* there is no metter what max ecc bits is */
	boot_info->MaxEccBits = 4;
	boot_info->EccLimitBits = 4;

	boot_info->sample_mode = spinand->right_sample_mode;
	boot_info->sample_delay = spinand->right_sample_delay;

	return 0;
}

static int aw_spinand_chip_init_last(struct aw_spinand_chip *chip)
{
	int ret;
	struct aw_spinand_info *info = chip->info;
	struct device_node *node = chip->spi->dev.of_node;
	unsigned int val;

	/* initialize from spinand information */
	if (info->operation_opt(chip) & SPINAND_QUAD_PROGRAM)
		chip->tx_bit = SPI_NBITS_QUAD;
	else
		chip->tx_bit = SPI_NBITS_SINGLE;

	if (info->operation_opt(chip) & SPINAND_QUAD_READ)
		chip->rx_bit = SPI_NBITS_QUAD;
	else if (info->operation_opt(chip) & SPINAND_DUAL_READ)
		chip->rx_bit = SPI_NBITS_DUAL;
	else
		chip->rx_bit = SPI_NBITS_SINGLE;

	/* re-initialize from device tree */
	ret = of_property_read_u32(node, "spi-rx-bus-width", &val);
	if (!ret && val < chip->rx_bit) {
		sunxi_info(NULL, "%s reset rx bit width to %u\n",
				info->model(chip), val);
		chip->rx_bit = val;
	}

	ret = of_property_read_u32(node, "spi-tx-bus-width", &val);
	if (!ret && val < chip->tx_bit) {
		if (val == SPI_NBITS_DUAL) {
			pr_info("%s not support tx bit width = 2, reset tx bit width to 1\n",
				info->model(chip));
			chip->tx_bit = SPI_NBITS_SINGLE;
		} else {
			pr_info("%s reset tx bit width to %u\n",
				info->model(chip), val);
			chip->tx_bit = val;
		}
	}

	/* update spinand register */
	ret = aw_spinand_chip_update_cfg(chip);
	if (ret)
		return ret;

	/* do read/write cache init */
	ret = aw_spinand_chip_cache_init(chip);
	if (ret)
		return ret;

	/* do bad block table init */
	ret = aw_spinand_chip_bbt_init(chip);
	if (ret)
		return ret;

	return 0;
}

static int aw_spinand_chip_preinit(struct spi_device *spi,
		struct aw_spinand_chip *chip)
{
	int ret;

	chip->spi = spi;

	ret = aw_spinand_chip_ecc_init(chip);
	if (unlikely(ret))
		return ret;

	ret = aw_spinand_chip_ops_init(chip);
	if (unlikely(ret))
		return ret;

	return 0;
}

static ssize_t spinand_param_read(struct file *file, char __user *buf,
				    size_t count, loff_t *ppos)
{
	int len = strlen(spinanddbg_priv.param);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user
		    ((void __user *)buf, (const void *)spinanddbg_priv.param,
		     (unsigned long)len)) {
			sunxi_warn(NULL, "copy_to_user fail\n");
			return 0;
		}
		*ppos += count;
	} else
		count = 0;
	return count;
}

static ssize_t spinand_param_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *ppos)
{
	if (copy_from_user(spinanddbg_priv.param, buf, count)) {
		sunxi_warn(NULL, "copy_from_user fail\n");
		return 0;
	}
	return count;
}

static ssize_t spinand_status_read(struct file *file, char __user *buf,
				    size_t count, loff_t *ppos)
{
	struct mtd_info *mtd = get_aw_spinand_mtd();
	struct aw_spinand *spinand = mtd_to_spinand(mtd);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_phy_info *pinfo = chip->info->phy_info;
	char tmpstatus[512];
	u32 value;
	int len = 0;
	struct device_node *node = chip->spi->dev.of_node;

	if (!strncmp(spinanddbg_priv.param, "freq", 4)) {
		of_property_read_u32(node, "spi-max-frequency", &value);
		sprintf(tmpstatus, "%u", value);
		strcpy(spinanddbg_priv.status, tmpstatus);

	} else if (!strncmp(spinanddbg_priv.param, "mode", 4)) {
		switch (chip->rx_bit) {
		case SPI_NBITS_QUAD:
			strcpy(spinanddbg_priv.status, "QUAD");
			break;
		case SPI_NBITS_DUAL:
			strcpy(spinanddbg_priv.status, "DUAL");
			break;
		case SPI_NBITS_SINGLE:
			strcpy(spinanddbg_priv.status, "SINGLE");
			break;
		default:
			strcpy(spinanddbg_priv.status, "NONE");
		}

	} else if (!strncmp(spinanddbg_priv.param, "info", 4)) {
		sprintf(tmpstatus,
			"========== arch info ==========\n"
			"Model:               %s\n"
			"DieCntPerChip:       %u\n"
			"BlkCntPerDie:        %u\n"
			"PageCntPerBlk:       %u\n"
			"SectCntPerPage:      %u\n"
			"OobSizePerPage:      %u\n"
			"BadBlockFlag:        0x%x\n"
			"OperationOpt:        0x%x\n"
			"MaxEraseTimes:       %d\n"
			"EccFlag:             0x%x\n"
			"EccType:             %d\n"
			"EccProtectedType:    %d\n"
			"===============================\n",
			pinfo->Model, pinfo->DieCntPerChip,
			pinfo->BlkCntPerDie, pinfo->PageCntPerBlk,
			pinfo->SectCntPerPage, pinfo->OobSizePerPage,
			pinfo->BadBlockFlag, pinfo->OperationOpt, pinfo->MaxEraseTimes,
			pinfo->EccFlag, pinfo->EccType, pinfo->EccProtectedType);
			strcpy(spinanddbg_priv.status, tmpstatus);

	} else {
		strcpy(spinanddbg_priv.status, "please set param before cat status\n \
			freq    ----return spi frequency\n \
			mode    ----return QUAD/DUAL/SINGLE\n \
			info    ----return chip info\n");
	}

	len = strlen(spinanddbg_priv.status);
	spinanddbg_priv.status[len] = 0x0A;
	spinanddbg_priv.status[len + 1] = 0x0;
	len = strlen(spinanddbg_priv.status);
	if (len) {
		if (*ppos >= len)
			return 0;
		if (count >= len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user
		   ((void __user *)buf, (const void *)spinanddbg_priv.status,
		  (unsigned long)len)) {
			sunxi_warn(NULL, "copy_to_user fail\n");
			return 0;
		}
		*ppos += count;
	} else
		count = 0;
	return count;
}

static ssize_t spinand_status_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *ppos)
{
	struct aw_spinand *spinand = get_aw_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);

	if (copy_from_user(spinanddbg_priv.status, buf, count)) {
		sunxi_warn(NULL, "copy_from_user fail\n");
		return 0;
	}

	if (!strncmp(spinanddbg_priv.status, "1-1-1", 5)) {
		chip->rx_bit = SPI_NBITS_SINGLE;
	} else if (!strncmp(spinanddbg_priv.status, "1-1-2", 5)) {
		chip->rx_bit = SPI_NBITS_DUAL;
	} else if (!strncmp(spinanddbg_priv.status, "1-1-4", 5)) {
		chip->rx_bit = SPI_NBITS_QUAD;
	} else {
		sunxi_warn(NULL, "please set param before echo x-x-x > status\n \
			x-x-x => cmd-addr-data line width\n \
			sg: echo 1-1-4 > /sys/kernel/debug/status\n \
			(cmd one line - addr one line - data quad line)\n");

		return count;
	}

	return count;
}

static const struct file_operations param_ops = {
	.write = spinand_param_write,
	.read = spinand_param_read,
};

static const struct file_operations status_ops = {
	.write = spinand_status_write,
	.read = spinand_status_read,
};

int spinand_debug_init(void)
{
	struct dentry *spinand_root;

	spinand_root = debugfs_create_dir("spinand", NULL);
	if (!debugfs_create_file
	    ("status", 0644, spinand_root, NULL, &status_ops))
		goto Fail;
	if (!debugfs_create_file
	    ("param", 0644, spinand_root, NULL, &param_ops))
		goto Fail;

	return 0;

Fail:
	debugfs_remove_recursive(spinand_root);
	spinand_root = NULL;
	return -ENOENT;
}

int aw_spinand_chip_init(struct spi_device *spi, struct aw_spinand_chip *chip)
{
	int ret;

	sunxi_info(NULL, "AW SPINand Phy Layer Version: %x.%x %x\n",
			AW_SPINAND_PHY_VER_MAIN, AW_SPINAND_PHY_VER_SUB,
			AW_SPINAND_PHY_VER_DATE);

	ret = aw_spinand_chip_preinit(spi, chip);
	if (unlikely(ret))
		return ret;

	ret = aw_spinand_chip_detect(chip);
	if (ret)
		return ret;

	ret = aw_spinand_chip_init_last(chip);
	if (ret)
		goto err;

	spinand_debug_init();

	sunxi_info(NULL, "sunxi physic nand init end\n");
	return 0;
err:
	aw_spinand_chip_clean(chip);
	return ret;
}
EXPORT_SYMBOL(aw_spinand_chip_init);

void aw_spinand_chip_exit(struct aw_spinand_chip *chip)
{
	aw_spinand_chip_clean(chip);
}
EXPORT_SYMBOL(aw_spinand_chip_exit);

MODULE_AUTHOR("liaoweixiong <liaoweixiong@allwinnertech.com>");
MODULE_DESCRIPTION("Commond physic layer for Allwinner's spinand driver");
MODULE_VERSION("1.0.0");
