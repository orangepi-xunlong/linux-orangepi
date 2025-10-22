/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPIF Controller Driver
 *
 * Copyright (c) 2021-2028 Allwinnertech Co., Ltd.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * 2022.12.22 lujianliang <lujianliang@allwinnertech.com>
 *    creat thie file and support sun55i of Allwinner.
 */

#include <sunxi-log.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/dmapool.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>
#include <boot_param.h>
#include <linux/memblock.h>
#include "sunxi-spif.h"
#include "../core.h"

/* For debug */
#define SPIF_DEBUG 0

#define SUNXI_SPIF_DEV_NAME		"sunxi_spif"
#define XFER_TIMEOUT			5000
#define SPIF_DEFAULT_SPEED_HZ		100000000

#define SUNXI_SPIF_MODULE_VERSION	"1.0.1"

static bool double_clk_flag;

#if SPIF_DEBUG
bool spif_debug_flag;
static void sunxi_spif_dump_reg(struct sunxi_spif *sspi)
{
	char buf[1024] = {0};
	snprintf(buf, sizeof(buf)-1,
			"spif->base_addr = 0x%p, the SPIF control register:\n"
			"[VER] 0x%02x = 0x%08x, [GC]  0x%02x = 0x%08x, [GCA] 0x%02x = 0x%08x\n"
			"[TCR] 0x%02x = 0x%08x, [TDS] 0x%02x = 0x%08x, [INT] 0x%02x = 0x%08x\n"
			"[STA] 0x%02x = 0x%08x, [CSD] 0x%02x = 0x%08x, [PHC] 0x%02x = 0x%08x\n"
			"[TCF] 0x%02x = 0x%08x, [TCS] 0x%02x = 0x%08x, [TNM] 0x%02x = 0x%08x\n"
			"[PSR] 0x%02x = 0x%08x, [PSA] 0x%02x = 0x%08x, [PEA] 0x%02x = 0x%08x\n"
			"[PMA] 0x%02x = 0x%08x, [DMA] 0x%02x = 0x%08x, [DSC] 0x%02x = 0x%08x\n"
			"[DFT] 0x%02x = 0x%08x, [CFT] 0x%02x = 0x%08x, [CFS] 0x%02x = 0x%08x\n"
			"[BAT] 0x%02x = 0x%08x, [BAC] 0x%02x = 0x%08x, [TB]  0x%02x = 0x%08x\n"
			"[RB]  0x%02x = 0x%08x\n",
			sspi->base_addr,
			SPIF_VER_REG, readl(sspi->base_addr + SPIF_VER_REG),
			SPIF_GC_REG, readl(sspi->base_addr + SPIF_GC_REG),
			SPIF_GCA_REG, readl(sspi->base_addr + SPIF_GCA_REG),

			SPIF_TC_REG, readl(sspi->base_addr + SPIF_TC_REG),
			SPIF_TDS_REG, readl(sspi->base_addr + SPIF_TDS_REG),
			SPIF_INT_EN_REG, readl(sspi->base_addr + SPIF_INT_EN_REG),

			SPIF_INT_STA_REG, readl(sspi->base_addr + SPIF_INT_STA_REG),
			SPIF_CSD_REG, readl(sspi->base_addr + SPIF_CSD_REG),
			SPIF_PHC_REG, readl(sspi->base_addr + SPIF_PHC_REG),

			SPIF_TCF_REG, readl(sspi->base_addr + SPIF_TCF_REG),
			SPIF_TCS_REG, readl(sspi->base_addr + SPIF_TCS_REG),
			SPIF_TNM_REG, readl(sspi->base_addr + SPIF_TNM_REG),

			SPIF_PS_REG, readl(sspi->base_addr + SPIF_PS_REG),
			SPIF_PSA_REG, readl(sspi->base_addr + SPIF_PSA_REG),
			SPIF_PEA_REG, readl(sspi->base_addr + SPIF_PEA_REG),

			SPIF_PMA_REG, readl(sspi->base_addr + SPIF_PMA_REG),
			SPIF_DMA_CTL_REG, readl(sspi->base_addr + SPIF_DMA_CTL_REG),
			SPIF_DSC_REG, readl(sspi->base_addr + SPIF_DSC_REG),

			SPIF_DFT_REG, readl(sspi->base_addr + SPIF_DFT_REG),
			SPIF_CFT_REG, readl(sspi->base_addr + SPIF_CFT_REG),
			SPIF_CFS_REG, readl(sspi->base_addr + SPIF_CFS_REG),

			SPIF_BAT_REG, readl(sspi->base_addr + SPIF_BAT_REG),
			SPIF_BAC_REG, readl(sspi->base_addr + SPIF_BAC_REG),
			SPIF_TB_REG, readl(sspi->base_addr + SPIF_TB_REG),

			SPIF_RB_REG, readl(sspi->base_addr + SPIF_RB_REG));
			printk("%s\n\n", buf);
}

void sunxi_spif_dump_descriptor(struct sunxi_spif *sspi)
{
	char buf[512] = {0};
	int desc_num = 0;

	while (desc_num < DESC_PER_DISTRIBUTION_MAX_NUM) {
		snprintf(buf, sizeof(buf)-1,
			"hburst_rw_flag        : 0x%x\n"
			"block_data_len        : 0x%x\n"
			"data_addr             : 0x%x\n"
			"next_des_addr         : 0x%x\n"
			"trans_phase	       : 0x%x\n"
			"flash_addr	       : 0x%x\n"
			"cmd_mode_buswidth     : 0x%x\n"
			"addr_dummy_data_count : 0x%x\n",
			sspi->dma_desc[desc_num]->hburst_rw_flag,
			sspi->dma_desc[desc_num]->block_data_len,
			sspi->dma_desc[desc_num]->data_addr << 2,
			sspi->dma_desc[desc_num]->next_des_addr,
			sspi->dma_desc[desc_num]->trans_phase,
			sspi->dma_desc[desc_num]->flash_addr,
			sspi->dma_desc[desc_num]->cmd_mode_buswidth,
			sspi->dma_desc[desc_num]->addr_dummy_data_count);
			printk("%s", buf);
		printk("sspi->dma_desc addr [%llx]...\n\n", sspi->desc_phys[desc_num]);
		if (sspi->dma_desc[desc_num]->next_des_addr)
			desc_num++;
		else
			break;
	}
}

void dump_spinor_info(boot_spinor_info_t *spinor_info)
{
	sunxi_info(NULL, "\n"
		"----------------------\n"
		"magic:%s\n"
		"readcmd:%x\n"
		"read_mode:%d\n"
		"write_mode:%d\n"
		"flash_size:%dM\n"
		"addr4b_opcodes:%d\n"
		"erase_size:%d\n"
		"frequency:%d\n"
		"sample_mode:%x\n"
		"sample_delay:%x\n"
		"read_proto:%x\n"
		"write_proto:%x\n"
		"read_dummy:%d\n"
		"----------------------\n",
		spinor_info->magic, spinor_info->readcmd,
		spinor_info->read_mode, spinor_info->write_mode,
		spinor_info->flash_size, spinor_info->addr4b_opcodes,
		spinor_info->erase_size, spinor_info->frequency,
		spinor_info->sample_mode, spinor_info->sample_delay,
		spinor_info->read_proto, spinor_info->write_proto,
		spinor_info->read_dummy);
}
#endif

static struct sunxi_spif *g_sspi;
static struct sunxi_spif *get_sspi(void)
{
	return g_sspi;
}

static int sunxi_spif_regulator_request(struct sunxi_spif *sspi)
{
	struct regulator *regu = NULL;

	/* Consider "n*" as nocare. Support "none", "nocare", "null", "" etc. */
	if ((sspi->regulator_id[0] == 'n') || (sspi->regulator_id[0] == 0))
		return 0;

	regu = devm_regulator_get(NULL, sspi->regulator_id);
	if (IS_ERR(regu)) {
		sunxi_err(&sspi->pdev->dev, "get regulator %s failed!\n", sspi->regulator_id);
		sspi->regulator = NULL;
		return -EINVAL;
	}

	sspi->regulator = regu;

	return 0;
}

static void sunxi_spif_regulator_release(struct sunxi_spif *sspi)
{
	if (!sspi->regulator)
		return;

	regulator_put(sspi->regulator);
	sspi->regulator = NULL;
}

static int sunxi_spif_regulator_enable(struct sunxi_spif *sspi)
{
	// fpga don't support
	if (!sspi->regulator)
		return 0;

	if (regulator_enable(sspi->regulator)) {
		sunxi_err(&sspi->pdev->dev, "enable regulator %s failed!\n", sspi->regulator_id);
		return -EINVAL;
	}

	return 0;
}

static void sunxi_spif_regulator_disable(struct sunxi_spif *sspi)
{
	if (!sspi->regulator)
		return;

	regulator_disable(sspi->regulator);
}

static int sunxi_spif_select_gpio_state(struct sunxi_spif *sspi, char *name)
{
	struct pinctrl_state *pctrl_state = NULL;
	int ret = 0;

	pctrl_state = pinctrl_lookup_state(sspi->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		sunxi_err(&sspi->pdev->dev, "spif pinctrl_lookup_state(%s)\n", name);
		return PTR_ERR(pctrl_state);
	}

	ret = pinctrl_select_state(sspi->pctrl, pctrl_state);
	if (ret) {
		sunxi_err(&sspi->pdev->dev, "spif pinctrl_select_state(%s) failed\n", name);
		return ret;
	}

	return 0;
}

static int sunxi_spif_pinctrl_init(struct sunxi_spif *sspi)
{
	return sunxi_spif_select_gpio_state(sspi, PINCTRL_STATE_DEFAULT);
}

static int sunxi_spif_pinctrl_exit(struct sunxi_spif *sspi)
{
	/* susend will use this function to set pin sleep
	 * driver remove will use this function to, and then devm related functions
	 * will auto recover resource
	 */
	return sunxi_spif_select_gpio_state(sspi, PINCTRL_STATE_SLEEP);
}

static void sunxi_spif_set_clk(u32 spif_clk, u32 mode_clk, struct sunxi_spif *sspi)
{
	clk_set_rate(sspi->mclk, spif_clk);
	if (clk_get_rate(sspi->mclk) != spif_clk) {
		clk_set_rate(sspi->mclk, mode_clk);
		sunxi_err(&sspi->pdev->dev,
				"set spif clock %d failed, use clk:%d\n", spif_clk, mode_clk);
	} else {
		sunxi_debug(&sspi->pdev->dev, "set spif clock %d success\n", spif_clk);
	}
}

static int sunxi_spif_clk_init(struct sunxi_spif *sspi)
{
	int ret = 0;
	long rate = 0;

	ret = clk_set_parent(sspi->mclk, sspi->pclk);
	if (ret != 0) {
		sunxi_err(&sspi->pdev->dev,
			"[spi-flash%d] clk_set_parent() failed! return %d\n",
			sspi->data->bus_num, ret);
		return -1;
	}

	ret = clk_prepare_enable(sspi->pclk);
	if (ret) {
		sunxi_err(&sspi->pdev->dev, "Couldn't enable AHB clock\n");
		goto err0;
	}

	ret = clk_prepare_enable(sspi->mclk);
	if (ret) {
		sunxi_err(&sspi->pdev->dev, "Couldn't enable module clock\n");
		goto err1;
	}

	ret = clk_prepare_enable(sspi->bus);
	if (ret) {
		sunxi_err(&sspi->pdev->dev, "Couldn't enable bus clock\n");
		goto err2;
	}

	/* To ensure that the read ID & status is normal,
	 * the maximum operating frequency is set to the minimum value
	 */
	rate = clk_round_rate(sspi->mclk, sspi->data->min_speed_hz);
	if (clk_set_rate(sspi->mclk, rate)) {
		sunxi_err(&sspi->pdev->dev,
			"[spi-flash%d] spi clk_set_rate failed\n",
			sspi->data->bus_num);
		return -1;
	}
	sunxi_info(&sspi->pdev->dev, "[spi-flash%d] mclk %u\n", sspi->data->bus_num,
			(unsigned)clk_get_rate(sspi->mclk));

//	/* linux-4.9 need't do reset and assert working(ccmu do that) */
//	ret = reset_control_reset(sspi->rstc);
//	if (ret) {
//		sunxi_err(dev, "Couldn't deassert the device from reset\n");
//		goto err2;
//	}

	return ret;

	clk_disable_unprepare(sspi->bus);
err2:
	clk_disable_unprepare(sspi->mclk);

err1:
	clk_disable_unprepare(sspi->pclk);

err0:
	return ret;
}

static void sunxi_spif_clk_exit(struct sunxi_spif *sspi)
{
	clk_disable_unprepare(sspi->bus);

	clk_disable_unprepare(sspi->mclk);

	clk_disable_unprepare(sspi->pclk);
}

static void sunxi_spif_soft_reset(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GCA_REG);
	reg_val |= SPIF_RESET;
	writel(reg_val, sspi->base_addr + SPIF_GCA_REG);
}

static void sunxi_spif_fifo_reset(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GCA_REG);
	reg_val |= SPIF_CDC_WF_RST;
	reg_val |= SPIF_CDC_RF_RST;

	if (sspi->working_mode & DQS)
		reg_val |= SPIF_DQS_RF_SRST;

	writel(reg_val, sspi->base_addr + SPIF_GCA_REG);
}

static void sunxi_spif_fifo_init(struct sunxi_spif *sspi)
{
	u32 reg_val;

	sunxi_spif_fifo_reset(sspi);

	/* set fifo water level */
	reg_val = readl(sspi->base_addr + SPIF_CFT_REG);

	reg_val &= ~(0xff << SPIF_RF_EMPTY_TRIG_LEVEL);
	reg_val |= (0x10 << SPIF_RF_EMPTY_TRIG_LEVEL);

	/*rf_fifo should less than 104*/
	reg_val &= ~(0xff << SPIF_RF_FULL_TRIG_LEVEL);
	reg_val |= (0x64 << SPIF_RF_FULL_TRIG_LEVEL);

	reg_val &= ~(0xff << SPIF_WF_EMPTY_TRIG_LEVEL);
	reg_val |= (0x10 << SPIF_WF_EMPTY_TRIG_LEVEL);

	/*ef_fifo should less than 104*/
	reg_val &= ~(0xff << SPIF_WF_FULL_TRIG_LEVEL);
	reg_val |= (0x64 << SPIF_WF_FULL_TRIG_LEVEL);

	writel(reg_val, sspi->base_addr + SPIF_CFT_REG);

	/* dqs mode fifo init */
	if (sspi->working_mode & DQS) {
		/* set fifo water level */
		reg_val = readl(sspi->base_addr + SPIF_DFT_REG);
		reg_val |= 0x0 << SPIF_DQS_EMPTY_TRIG_LEVEL;
		reg_val |= 0x64 << SPIF_DQS_FULL_TRIG_LEVEL;
		writel(reg_val, sspi->base_addr + SPIF_DFT_REG);
	}
}

static void sunxi_spif_wp_en(struct sunxi_spif *sspi, bool enable)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG);

	if (enable)
		reg_val |= SPIF_GC_WP_EN;
	else
		reg_val &= ~SPIF_GC_WP_EN;
	writel(reg_val, sspi->base_addr + SPIF_GC_REG);
}

static void sunxi_spif_hold_en(struct sunxi_spif *sspi, bool enable)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG);

	if (enable)
		reg_val |= SPIF_GC_HOLD_EN;
	else
		reg_val &= ~SPIF_GC_HOLD_EN;
	writel(reg_val, sspi->base_addr + SPIF_GC_REG);
}

static void sunxi_spif_set_clock_mode(struct sunxi_spif *sspi, u32 mode)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG) & ~SPIF_MODE_MASK;

	reg_val |= mode;
	writel(reg_val, sspi->base_addr + SPIF_GC_REG);
}

static void sunxi_spif_set_output_clk(struct sunxi_spif *sspi, u32 status)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_TC_REG);

	if (status)
		reg_val |= SPIF_CLK_SCKOUT_SRC_SEL;
	else
		reg_val &= ~SPIF_CLK_SCKOUT_SRC_SEL;
	writel(reg_val, sspi->base_addr + SPIF_TC_REG);
}

static void sunxi_spif_set_dtr(struct sunxi_spif *sspi, u32 status)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG);

	if (status)
		reg_val |= SPIF_GC_DTR_EN;
	else
		reg_val &= ~SPIF_GC_DTR_EN;
	writel(reg_val, sspi->base_addr + SPIF_GC_REG);
}

static void sunxi_spif_samp_mode(struct sunxi_spif *sspi, unsigned int status)
{
	unsigned int rval = readl(sspi->base_addr + SPIF_TC_REG);

	if (status)
		rval |= SPIF_DIGITAL_ANALOG_EN;
	else
		rval &= ~SPIF_DIGITAL_ANALOG_EN;

	writel(rval, sspi->base_addr + SPIF_TC_REG);
}

static void sunxi_spif_samp_dl_sw_rx_status(struct sunxi_spif *sspi,
		unsigned int status)
{
	unsigned int rval = readl(sspi->base_addr + SPIF_TC_REG);

	if (status)
		rval |= SPIF_ANALOG_DL_SW_RX_EN;
	else
		rval &= ~SPIF_ANALOG_DL_SW_RX_EN;

	writel(rval, sspi->base_addr +SPIF_TC_REG);
}

static void sunxi_spif_set_sample_mode(struct sunxi_spif *sspi,
		unsigned int mode)
{
	unsigned int rval = readl(sspi->base_addr + SPIF_TC_REG);

	rval &= (~SPIF_DIGITAL_DELAY_MASK);
	rval |= mode << SPIF_DIGITAL_DELAY;
	writel(rval, sspi->base_addr + SPIF_TC_REG);
}

static void sunxi_spif_set_sample_delay(struct sunxi_spif *sspi,
		unsigned int sample_delay)
{
	unsigned int rval = readl(sspi->base_addr + SPIF_TC_REG);

	rval &= (~SPIF_ANALOG_DELAY_MASK);
	rval |= sample_delay << SPIF_ANALOG_DELAY;
	writel(rval, sspi->base_addr + SPIF_TC_REG);
	mdelay(1);
}

static void sunxi_spif_config_tc(struct sunxi_spif *sspi)
{
	if (sspi->data->sample_mode != SAMP_MODE_DL_DEFAULT) {
		sunxi_spif_set_clk(sspi->data->max_speed_hz,
			clk_get_rate(sspi->mclk), sspi);
		double_clk_flag = 0;
		sunxi_info(&sspi->pdev->dev, "[spi-flash%d] working clk %u\n",
				sspi->data->bus_num,
				sspi->data->max_speed_hz);

		sunxi_spif_samp_mode(sspi, 1);
		sunxi_spif_samp_dl_sw_rx_status(sspi, 1);
		sunxi_spif_set_sample_mode(sspi, sspi->data->sample_mode);
		sunxi_spif_set_sample_delay(sspi, sspi->data->sample_delay);
	}
}

static int update_boot_param(struct mtd_info *mtd, struct sunxi_spif *sspi)
{
	struct spi_nor *nor = mtd->priv;
	u8 erase_opcode = nor->erase_opcode;
	uint32_t erasesize = mtd->erasesize;
	size_t retlen = 0;
	int ret;
	struct erase_info instr;
	boot_spinor_info_t *boot_info = NULL;
	struct sunxi_boot_param_region *boot_param = NULL;
	boot_param = kmalloc(BOOT_PARAM_SIZE, GFP_KERNEL);
	memset(boot_param, 0, BOOT_PARAM_SIZE);

	strncpy((char *)boot_param->header.magic,
			(const char *)BOOT_PARAM_MAGIC,
			sizeof(boot_param->header.magic));

	boot_param->header.check_sum = CHECK_SUM;

	boot_info = (boot_spinor_info_t *)boot_param->spiflash_info;

	strncpy((char *)boot_info->magic, (const char *)SPINOR_BOOT_PARAM_MAGIC,
			sizeof(boot_info->magic));
	boot_info->readcmd = nor->read_opcode;
	boot_info->flash_size = mtd->size / 1024 / 1024;
	boot_info->erase_size = mtd->erasesize;
	boot_info->read_proto = nor->read_proto;
	boot_info->write_proto = nor->write_proto;
	boot_info->read_dummy = nor->read_dummy;

	boot_info->frequency = sspi->data->max_speed_hz;
	boot_info->sample_mode = sspi->data->sample_mode;
	boot_info->sample_delay = sspi->data->sample_delay;

	if (nor->read_proto == SNOR_PROTO_1_1_4)
		boot_info->read_mode = 4;
	else if (nor->read_proto == SNOR_PROTO_1_1_2)
		boot_info->read_mode = 2;
	else
		boot_info->read_mode = 1;

	if (nor->write_proto == SNOR_PROTO_1_1_4)
		boot_info->write_mode = 4;
	else if (nor->write_proto == SNOR_PROTO_1_1_2)
		boot_info->write_mode = 2;
	else
		boot_info->write_mode = 1;

	if (nor->flags & SNOR_F_4B_OPCODES)
		boot_info->addr4b_opcodes = 1;

	/*
	 * To not break boot0, switch bits 4K erasing
	 */
	if (nor->addr_width == 4)
		nor->erase_opcode = SPINOR_OP_BE_4K_4B;
	else
		nor->erase_opcode = SPINOR_OP_BE_4K;
	mtd->erasesize = 4096;

	instr.addr = (CONFIG_SPINOR_UBOOT_OFFSET << 9) - BOOT_PARAM_SIZE;
	instr.len = BOOT_PARAM_SIZE;
	mtd->_erase(mtd, &instr);
	nor->erase_opcode = erase_opcode;
	mtd->erasesize = erasesize;

	ret = mtd->_write(mtd, (CONFIG_SPINOR_UBOOT_OFFSET << 9) - BOOT_PARAM_SIZE,
			BOOT_PARAM_SIZE, &retlen, (u_char *)boot_param);
	if (ret < 0)
		return -1;

	//dump_spinor_info(boot_info);
	kfree(boot_param);
	return BOOT_PARAM_SIZE == retlen ? 0 : -1;
}

static void sunxi_spif_try_sample_param(struct sunxi_spif *sspi, struct mtd_info *mtd,
				boot_spinor_info_t *boot_info)
{
	unsigned int start_ok = 0, end_ok = 0, len_ok = 0, mode_ok = 0;
	unsigned int start_backup = 0, end_backup = 0, len_backup = 0;
	unsigned int mode = 0, startry_mode = 0, endtry_mode = 1;
	unsigned int sample_delay = 0;
	size_t retlen = 0, len = 512;
	boot0_file_head_t *boot0_head;
	boot0_head = kmalloc(len, GFP_KERNEL);

	sunxi_spif_set_clk(sspi->data->max_speed_hz, clk_get_rate(sspi->mclk), sspi);
	double_clk_flag = 0;

	sunxi_spif_samp_mode(sspi, 1);
	sunxi_spif_samp_dl_sw_rx_status(sspi, 1);
	for (mode = startry_mode; mode <= endtry_mode; mode++) {
		sspi->data->sample_mode = mode;
		sunxi_spif_set_sample_mode(sspi, mode);
		for (sample_delay = 0; sample_delay < 64; sample_delay++) {
			sspi->data->sample_delay = sample_delay;
			sunxi_spif_set_sample_delay(sspi, sample_delay);
			memset(boot0_head, 0, len);
			mtd->_read(mtd, 0, len, &retlen, (u_char *)boot0_head);

			if (strncmp((char *)boot0_head->boot_head.magic,
				(char *)BOOT0_MAGIC,
				sizeof(boot0_head->boot_head.magic)) == 0) {
				sunxi_debug(&sspi->pdev->dev, "mode:%d delay:%d [OK]\n",
						mode, sample_delay);
				if (!len_backup) {
					start_backup = sample_delay;
					end_backup = sample_delay;
				} else
					end_backup = sample_delay;
				len_backup++;
			} else {
				sunxi_debug(&sspi->pdev->dev, "mode:%d delay:%d [ERROR]\n",
						mode, sample_delay);
				if (!start_backup)
					continue;
				else {
					if (len_backup > len_ok) {
						len_ok = len_backup;
						start_ok = start_backup;
						end_ok = end_backup;
						mode_ok = mode;
					}

					len_backup = 0;
					start_backup = 0;
					end_backup = 0;
				}
			}
		}
		if (len_backup > len_ok) {
			len_ok = len_backup;
			start_ok = start_backup;
			end_ok = end_backup;
			mode_ok = mode;
		}
		len_backup = 0;
		start_backup = 0;
		end_backup = 0;
	}

	if (!len_ok) {
		sspi->data->sample_delay = SAMP_MODE_DL_DEFAULT;
		sspi->data->sample_mode = SAMP_MODE_DL_DEFAULT;
		sunxi_spif_samp_mode(sspi, 0);
		sunxi_spif_samp_dl_sw_rx_status(sspi, 0);
		/* default clock */
		sunxi_spif_set_clk(25000000, clk_get_rate(sspi->mclk), sspi);
		double_clk_flag = 0;

		sunxi_err(&sspi->pdev->dev, "spif update delay param error\n");
	} else {
		sspi->data->sample_delay = (start_ok + end_ok) / 2;
		sspi->data->sample_mode = mode_ok;
		sunxi_spif_set_sample_mode(sspi, sspi->data->sample_mode);
		sunxi_spif_set_sample_delay(sspi, sspi->data->sample_delay);
	}
	sunxi_info(&sspi->pdev->dev,
			"Sample mode:%d start:%d end:%d right_sample_delay:0x%x\n",
			mode_ok, start_ok, end_ok, sspi->data->sample_delay);

	boot_info->sample_mode = sspi->data->sample_mode;
	boot_info->sample_delay = sspi->data->sample_delay;
	kfree(boot0_head);
	return;
}

void sunxi_spif_update_sample_delay_para(struct mtd_info *mtd)
{
	struct sunxi_spif *sspi = get_sspi();
	boot_spinor_info_t *boot_info = NULL;
	struct sunxi_boot_param_region *boot_param = NULL;

	if (!sspi) {
		sunxi_err(&sspi->pdev->dev, "spi-flash controller is not initialized\n");
		return;
	}

	if (sspi->data->sample_mode == SAMP_MODE_DL_DEFAULT) {
#if IS_ENABLED(CONFIG_SUNXI_FASTBOOT)
		int boot_param_addr = 0x42FFF000;
		boot_param = (struct sunxi_boot_param_region *)__va(boot_param_addr);
		boot_info = (boot_spinor_info_t *)boot_param->spiflash_info;
#else
		size_t retlen;
		sunxi_spif_set_clk(25000000, clk_get_rate(sspi->mclk), sspi);
		double_clk_flag = 0;

		boot_param = kmalloc(BOOT_PARAM_SIZE, GFP_KERNEL);
		mtd->_read(mtd, (CONFIG_SPINOR_UBOOT_OFFSET << 9) - BOOT_PARAM_SIZE,
			BOOT_PARAM_SIZE, &retlen, (u_char *)boot_param);
#endif

		if (strncmp((const char *)boot_param->header.magic,
					(const char *)BOOT_PARAM_MAGIC,
					sizeof(boot_param->header.magic)) ||
			strncmp((const char *)boot_info->magic,
					(const char *)SPINOR_BOOT_PARAM_MAGIC,
					sizeof(boot_info->magic))) {
			sunxi_err(&sspi->pdev->dev, "boot param magic abnormity go ot retey\n");
			sunxi_spif_try_sample_param(sspi, mtd, boot_info);
			if (update_boot_param(mtd, sspi))
				sunxi_err(&sspi->pdev->dev, "update boot param error\n");
		}

		if (boot_info->sample_delay == SAMP_MODE_DL_DEFAULT) {
			sunxi_err(&sspi->pdev->dev, "boot smple delay abnormity go ot retey\n");
			sunxi_spif_try_sample_param(sspi, mtd, boot_info);
			if (update_boot_param(mtd, sspi))
				sunxi_err(&sspi->pdev->dev, "update boot param error\n");
		} else {
			sspi->data->sample_mode = boot_info->sample_mode;
			sspi->data->sample_delay = boot_info->sample_delay;
			sunxi_info(&sspi->pdev->dev, "Read boot param[mode:%x delay:%x]\n",
					sspi->data->sample_mode, sspi->data->sample_delay);
		}
#if IS_ENABLED(CONFIG_SUNXI_FASTBOOT)
		memblock_free(boot_param_addr, BOOT_PARAM_SIZE);
		free_reserved_area(__va(boot_param_addr),
				__va(boot_param_addr + BOOT_PARAM_SIZE), -1, "boot_param");
#else
		kfree(boot_param);
#endif
	}

	sunxi_spif_config_tc(sspi);

	return;
}
EXPORT_SYMBOL_GPL(sunxi_spif_update_sample_delay_para);

static void sunxi_spif_set_cs_delay(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_CSD_REG);

	reg_val &= ~(0xff << SPIF_CSSOT);
	reg_val |= SPIF_CSSOT_DEFAULT << SPIF_CSSOT;

	reg_val &= ~(0xff << SPIF_CSEOT);
	reg_val |= SPIF_CSEOT_DEFAULT << SPIF_CSEOT;

	reg_val &= ~(0xff << SPIF_CSDA);
	reg_val |= SPIF_CSDA_DEFAULT << SPIF_CSDA;

	writel(reg_val, sspi->base_addr + SPIF_CSD_REG);
}

static void sunxi_spif_enable_irq(struct sunxi_spif *sspi, u32 bitmap)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_INT_EN_REG);

	reg_val &= 0x0;
	reg_val |= bitmap;

	writel(reg_val, sspi->base_addr + SPIF_INT_EN_REG);
}

static void sunxi_spif_disable_irq(struct sunxi_spif *sspi, u32 bitmap)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_INT_EN_REG);
	reg_val &= ~bitmap;
	writel(reg_val, sspi->base_addr + SPIF_INT_EN_REG);
}

static inline u32 sunxi_spif_query_irq_pending(struct sunxi_spif *sspi)
{
	u32 STA_MASK = SPIF_INT_STA_TC_EN | SPIF_INT_STA_ERR_EN;
	return (STA_MASK & readl(sspi->base_addr + SPIF_INT_STA_REG));
}

static void sunxi_spif_clear_irq_pending(struct sunxi_spif *sspi, u32 bitmap)
{
	writel(bitmap, sspi->base_addr + SPIF_INT_STA_REG);
}

static irqreturn_t sunxi_spif_handler(int irq, void *dev_id)
{
	struct sunxi_spif *sspi = (struct sunxi_spif *)dev_id;
	u32 irq_sta;
	unsigned long flags = 0;

	spin_lock_irqsave(&sspi->lock, flags);

	irq_sta = readl(sspi->base_addr + SPIF_INT_STA_REG);
	sunxi_debug(&sspi->pdev->dev, "irq is coming, and status is %x", irq_sta);

	if (irq_sta & SPIF_INT_STA_TC) {
		sunxi_debug(&sspi->pdev->dev, "SPI TC comes\n");
		/*wakup uplayer, by the sem */
		complete(&sspi->done);
		sunxi_spif_clear_irq_pending(sspi, irq_sta);
		spin_unlock_irqrestore(&sspi->lock, flags);
		return IRQ_HANDLED;
	} else if (irq_sta & SPIF_INT_STA_ERR) {
		sunxi_err(&sspi->pdev->dev, " SPI ERR %#x comes\n", irq_sta);
		sspi->result = -1;
		complete(&sspi->done);
		sunxi_spif_clear_irq_pending(sspi, irq_sta);
		spin_unlock_irqrestore(&sspi->lock, flags);
		return IRQ_HANDLED;
	}

	writel(irq_sta, sspi->base_addr + SPIF_INT_STA_REG);
	spin_unlock_irqrestore(&sspi->lock, flags);

	return IRQ_NONE;
}

/*
* spif controller config chip select
* spif cs can only control by controller self
*/
static u32 sunxi_spif_ss_select(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG);

	if (sspi->data->chip_select < 4) {
		reg_val &= ~SPIF_GC_SS_MASK;/* SS-chip select, clear two bits */
		reg_val |= sspi->data->chip_select << SPIF_GC_SS_BIT_POS;/* set chip select */
		reg_val |= SPIF_GC_CS_POL;/* active low polarity */
		writel(reg_val, sspi->base_addr + SPIF_GC_REG);
		sunxi_debug(&sspi->pdev->dev, "use ss : %d\n", sspi->data->chip_select);
		return 0;
	} else {
		sunxi_err(&sspi->pdev->dev, "cs set fail! cs = %d\n", sspi->data->chip_select);
		return -EINVAL;
	}
}

static void sunxi_spif_set_trans_mode(struct sunxi_spif *sspi, u8 mode)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG);

	if (mode)
		reg_val |= SPIF_GC_CFG_MODE;
	else
		reg_val &= ~SPIF_GC_CFG_MODE;
	writel(reg_val, sspi->base_addr + SPIF_GC_REG);
}

static void sunxi_spif_trans_type_enable(struct sunxi_spif *sspi,
				u32 type_phase)
{
	writel(type_phase, sspi->base_addr + SPIF_PHC_REG);
}

static void sunxi_spif_set_flash_addr(struct sunxi_spif *sspi, u32 flash_addr)
{
	writel(flash_addr, sspi->base_addr + SPIF_TCF_REG);
}

static void sunxi_spif_set_buswidth(struct sunxi_spif *sspi,
				u32 cmd_mode_buswidth)
{
	writel(cmd_mode_buswidth, sspi->base_addr + SPIF_TCS_REG);
}

static void sunxi_spif_set_data_count(struct sunxi_spif *sspi,
				u32 addr_dummy_data_count)
{
	writel(addr_dummy_data_count, sspi->base_addr + SPIF_TNM_REG);
}

/* set first descriptor start addr */
static void sunxi_spif_set_des_start_addr(struct sunxi_spif *sspi)
{
	/* addr word alignment */
	writel(sspi->desc_phys[0] >> 2, sspi->base_addr + SPIF_DSC_REG);
}

static void sunxi_spif_cpu_start_transfer(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GC_REG);

	reg_val |= SPIF_GC_NMODE_EN;
	writel(reg_val, sspi->base_addr + SPIF_GC_REG);
}

static void sunxi_spif_start_dma_xfer(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_DMA_CTL_REG);

	/* the dma descriptor default len is 8*4=32 bit */
	reg_val &= ~(0xff << SPIF_DMA_DESCRIPTOR_LEN);
	reg_val |= (0x20 << SPIF_DMA_DESCRIPTOR_LEN);

	/* start transfer, this bit will be quickly clear to 0 after set to 1 */
	reg_val |= SPIF_CFG_DMA_START;

	writel(reg_val, sspi->base_addr + SPIF_DMA_CTL_REG);
}

static int
sunxi_spif_prefetch_xfer(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	sunxi_err(&sspi->pdev->dev, "now don't support\n");

	//sunxi_spif_enable_irq(sspi, (SPIF_PREFETCH_READ_EN | SPIF_INT_STA_ERR_EN));

	return 0;
}

/* sunxi_spif_hw_init : config the spif controller's public configration
 * return 0 on success, reutrn err num on failed
 */
static int sunxi_spif_hw_init(struct sunxi_spif *sspi)
{
	int err;

	err = sunxi_spif_regulator_enable(sspi);
	if (err) {
		sunxi_err(&sspi->pdev->dev, "sunxi_spif regulator enable error\n");
		goto err0;
	}

	err = sunxi_spif_pinctrl_init(sspi);
	if (err) {
		sunxi_err(&sspi->pdev->dev, "sunxi_spif pinctrl init error\n");
		goto err1;
	}

	err = sunxi_spif_clk_init(sspi);
	if (err) {
		sunxi_err(&sspi->pdev->dev, "sunxi_spif clk init error\n");
		goto err2;
	}

	/* 1. reset all tie logic & fifo */
	sunxi_spif_soft_reset(sspi);
	sunxi_spif_fifo_init(sspi);

	/* 2. disable wp & hold */
	sunxi_spif_wp_en(sspi, 0);
	sunxi_spif_hold_en(sspi, 0);

	/* 3. set spi transfer clock mode */
	sunxi_spif_set_clock_mode(sspi, SPIF_MODE0);

	/* 3. disable DTR */
	sunxi_spif_set_output_clk(sspi, 0);
	sunxi_spif_set_dtr(sspi, 0);

	/* 4. set sample delay timing */
	//sunxi_spif_config_tc(sspi);

	/* 5. set the dedault vaule */
	sunxi_spif_set_cs_delay(sspi);

	return 0;

err2:
	sunxi_spif_pinctrl_exit(sspi);
err1:
	sunxi_spif_regulator_disable(sspi);
err0:
	return err;
}

static int sunxi_spif_hw_deinit(struct sunxi_spif *sspi)
{
	sunxi_spif_clk_exit(sspi);
	sunxi_spif_pinctrl_exit(sspi);
	sunxi_spif_regulator_disable(sspi);

	return 0;
}

static void sunxi_spif_ctr_recover(struct sunxi_spif *sspi)
{
	/* aw1886 Soft reset does not reset dma's state machine */
	sunxi_spif_hw_deinit(sspi);
	sunxi_spif_hw_init(sspi);
}

static int sunxi_spif_select_buswidth(u32 buswidth)
{
	int width = 0;

	switch (buswidth) {
	case SPIF_SINGLE_MODE:
		width = 0;
		break;
	case SPIF_DUEL_MODE:
		width = 1;
		break;
	case SPIF_QUAD_MODE:
		width = 2;
		break;
	case SPIF_OCTAL_MODE:
		width = 3;
		break;
	default:
		sunxi_err(NULL, "Parameter error with buswidth:%d\n", buswidth);
	}
	return width;
}

static char *sunxi_spif_get_data_buf(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	if (op->data.nbytes > CONFIG_SYS_MAXDATA_SIZE) {
		if (!sspi->temporary_data_cache)
			sspi->temporary_data_cache = dma_alloc_coherent(&sspi->pdev->dev,
				op->data.nbytes, &sspi->temporary_data_phys, GFP_KERNEL);
		if (!sspi->temporary_data_cache) {
			sunxi_err(&sspi->pdev->dev, "Failed to alloc cache buf memory\n");
			return NULL;
		}
		return sspi->temporary_data_cache;
	} else
		return sspi->data_buf;;
}

static dma_addr_t sunxi_spif_get_data_paddr(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	if (op->data.nbytes > CONFIG_SYS_MAXDATA_SIZE) {
		if (!sspi->temporary_data_cache)
			sspi->temporary_data_cache = dma_alloc_coherent(&sspi->pdev->dev,
				op->data.nbytes, &sspi->temporary_data_phys, GFP_KERNEL);
		if (!sspi->temporary_data_cache) {
			sunxi_err(&sspi->pdev->dev, "Failed to alloc cache buf memory\n");
			return PTR_ERR(sspi->temporary_data_cache);
		}
		return sspi->temporary_data_phys;
	} else
		return sspi->data_phys;
}

static int sunxi_spif_handler_descline(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	bool vmalloced_buf = NULL;
	struct page *vm_page;
	bool kmap_buf = false;
	size_t op_len = 0;
	void *op_buf = NULL;
	unsigned int desc_num = 1;
	unsigned int total_data_len = op->data.nbytes < CONFIG_SYS_MAXDATA_SIZE ?
					0 : (op->data.nbytes - CONFIG_SYS_MAXDATA_SIZE);
	struct dma_descriptor *dma_desc = sspi->dma_desc[desc_num];
	struct dma_descriptor *last_desc = sspi->dma_desc[0];
	dma_addr_t desc_phys;
	struct dma_desc_cache *desc_cache;

	last_desc->hburst_rw_flag &= ~DMA_FINISH_FLASG;

	if (op->data.dir == SPI_MEM_DATA_IN)
		op_buf = op->data.buf.in;
	else
		op_buf = (void *)op->data.buf.out;

	while (total_data_len) {
		if (desc_num >= DESC_PER_DISTRIBUTION_MAX_NUM) {
			dma_desc = dma_pool_zalloc(sspi->pool, GFP_NOWAIT, &desc_phys);
			if (!dma_desc) {
				sunxi_err(&sspi->pdev->dev,
					"Failed to alloc dma descriptor memory\n");
				return PTR_ERR(dma_desc);;
			}
			/* addr word alignment */
			last_desc->next_des_addr = desc_phys >> 2;

			desc_cache = kzalloc(sizeof(struct dma_desc_cache), GFP_KERNEL);
			desc_cache->dma_desc = dma_desc;
			desc_cache->desc_phys = desc_phys;
			list_add(&desc_cache->desc_list, &sspi->desc_cache_list);
		} else {
			dma_desc = sspi->dma_desc[desc_num];
			/* addr word alignment */
			last_desc->next_des_addr = sspi->desc_phys[desc_num] >> 2;
		}

		memcpy(dma_desc, last_desc, sizeof(struct dma_descriptor));

		op_len = min_t(size_t, CONFIG_SYS_MAXDATA_SIZE, total_data_len);

		dma_desc->addr_dummy_data_count &= ~DMA_DATA_LEN;
		dma_desc->block_data_len &= ~DMA_DATA_LEN;

		dma_desc->addr_dummy_data_count |=
				op_len << SPIF_DATA_NUM_POS;
		dma_desc->block_data_len |=
				op_len << SPIF_DATA_NUM_POS;

		op_buf += CONFIG_SYS_MAXDATA_SIZE;

		vmalloced_buf = is_vmalloc_addr(op_buf);
#if IS_ENABLED(CONFIG_HIGHMEM)
		kmap_buf = ((unsigned long)op_buf >= PKMAP_BASE &&
				(unsigned long)op_buf < (PKMAP_BASE +
					(LAST_PKMAP * PAGE_SIZE)));
#endif
		if (vmalloced_buf) {
			vm_page = vmalloc_to_page(op_buf);
			dma_desc->data_addr = page_to_phys(vm_page);
			dma_desc->data_addr += offset_in_page(op_buf);
		} else if (kmap_buf) {
			vm_page = kmap_to_page(op_buf);
			dma_desc->data_addr = page_to_phys(vm_page);
			dma_desc->data_addr += offset_in_page(op_buf);
		} else {
			dma_desc->data_addr = virt_to_phys(op_buf);
		}
		if (dma_desc->data_addr % CONFIG_SYS_CACHELINE_SIZE) {
			dma_desc->data_addr = sunxi_spif_get_data_paddr(sspi, op) +
					(desc_num * CONFIG_SYS_MAXDATA_SIZE);
			if (op->data.dir == SPI_MEM_DATA_OUT) {
				memcpy((void *)sunxi_spif_get_data_buf(sspi, op) +
					(desc_num * CONFIG_SYS_MAXDATA_SIZE),
					(const void *)op_buf, op_len);
			}
		}

		/* addr word alignment */
		dma_desc->data_addr = dma_desc->data_addr >> 2;

		dma_desc->flash_addr += CONFIG_SYS_MAXDATA_SIZE;
		total_data_len -= op_len;
		last_desc = dma_desc;
		desc_num++;
	}

	dma_desc->hburst_rw_flag |= DMA_FINISH_FLASG;
	dma_desc->next_des_addr = 0;

	return 0;
}

static int sunxi_spif_mem_exec_op(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	bool vmalloced_buf = NULL;
	struct page *vm_page;
	bool kmap_buf = false;
	size_t op_len = 0;
	void *op_buf = NULL;
	//struct spi_nor *nor = sspi->nor;

	if (op->data.dir == SPI_MEM_DATA_IN)
		op_buf = op->data.buf.in;
	else
		op_buf = (void *)op->data.buf.out;

	/* set hburst type */
	sspi->dma_desc[0]->hburst_rw_flag &= ~HBURST_TYPE;
	sspi->dma_desc[0]->hburst_rw_flag |= HBURST_INCR16_TYPE;

	/* the last one descriptor */
	sspi->dma_desc[0]->hburst_rw_flag |= DMA_FINISH_FLASG;
	/* set next des addr */

	/* set DMA block len mode */
	sspi->dma_desc[0]->block_data_len &= ~DMA_BLK_LEN;
	sspi->dma_desc[0]->block_data_len |= DMA_BLK_LEN_64B;

	sspi->dma_desc[0]->addr_dummy_data_count |= SPIF_DES_NORMAL_EN;

	/* dispose cmd */
	if (op->cmd.opcode) {
		sspi->dma_desc[0]->trans_phase |= SPIF_CMD_TRANS_EN;
		sspi->dma_desc[0]->cmd_mode_buswidth |= op->cmd.opcode << SPIF_CMD_OPCODE_POS;
		/* set cmd buswidth */
		sspi->dma_desc[0]->cmd_mode_buswidth |=
			sunxi_spif_select_buswidth(op->cmd.buswidth) << SPIF_CMD_TRANS_POS;
		if (op->cmd.buswidth != 1)
			sspi->dma_desc[0]->cmd_mode_buswidth |=
				sunxi_spif_select_buswidth(op->cmd.buswidth) <<
				SPIF_DATA_TRANS_POS;
	}

	/* dispose addr */
	if (op->addr.nbytes) {
		sspi->dma_desc[0]->trans_phase |= SPIF_ADDR_TRANS_EN;
		sspi->dma_desc[0]->flash_addr = op->addr.val;
		if (op->addr.nbytes == 4) //set 4byte addr mode
			sspi->dma_desc[0]->addr_dummy_data_count |= SPIF_ADDR_SIZE_MODE;
		/* set addr buswidth */
		sspi->dma_desc[0]->cmd_mode_buswidth |=
			sunxi_spif_select_buswidth(op->addr.buswidth) <<
			SPIF_ADDR_TRANS_POS;
	}

	/* dispose mode */
	/*if ((nor->info->flags & USE_IO_MODE) && CHECK_IO_OP(op->cmd.opcode)) {
		sspi->dma_desc[0]->trans_phase |= SPIF_MODE_TRANS_EN;
		sspi->dma_desc[0]->cmd_mode_buswidth |=
				0x0 << SPIF_MODE_OPCODE_POS;
		// set addr buswidth
		sspi->dma_desc[0]->cmd_mode_buswidth |=
			sunxi_spif_select_buswidth(op->mode.buswidth) <<
			SPIF_MODE_TRANS_POS;
	}*/

	/* dispose dummy */
	if (op->dummy.nbytes) {
		sspi->dma_desc[0]->trans_phase |= SPIF_DUMMY_TRANS_EN;
		sspi->dma_desc[0]->addr_dummy_data_count |=
			(op->dummy.nbytes << SPIF_DUMMY_NUM_POS);
	}

	/* dispose data */
	if (op->data.nbytes) {
		/* set data buswidth */
		sspi->dma_desc[0]->cmd_mode_buswidth |=
			sunxi_spif_select_buswidth(op->data.buswidth) << SPIF_DATA_TRANS_POS;

		op_len = min_t(size_t, CONFIG_SYS_MAXDATA_SIZE, op->data.nbytes);
		sspi->dma_desc[0]->addr_dummy_data_count |= op_len << SPIF_DATA_NUM_POS;
		sspi->dma_desc[0]->block_data_len |= op_len << SPIF_DATA_NUM_POS;

		if (op->data.dir == SPI_MEM_DATA_IN) {
			/* Flash read:1 DMA Write to dram */
			sspi->dma_desc[0]->hburst_rw_flag |= DMA_RW_PROCESS;
			sspi->dma_desc[0]->trans_phase |= SPIF_RX_TRANS_EN;
		} else {
			/* Flash write:0 DMA read for dram */
			sspi->dma_desc[0]->hburst_rw_flag &= ~DMA_RW_PROCESS;
			sspi->dma_desc[0]->trans_phase |= SPIF_TX_TRANS_EN;
		}

		vmalloced_buf = is_vmalloc_addr(op_buf);
#if IS_ENABLED(CONFIG_HIGHMEM)
		kmap_buf = ((unsigned long)op_buf >= PKMAP_BASE &&
				(unsigned long)op_buf < (PKMAP_BASE +
					(LAST_PKMAP * PAGE_SIZE)));
#endif
		if (vmalloced_buf) {
			vm_page = vmalloc_to_page(op_buf);
			sspi->dma_desc[0]->data_addr = page_to_phys(vm_page);
			sspi->dma_desc[0]->data_addr += offset_in_page(op_buf);
		} else if (kmap_buf) {
			vm_page = kmap_to_page(op_buf);
			sspi->dma_desc[0]->data_addr = page_to_phys(vm_page);
			sspi->dma_desc[0]->data_addr += offset_in_page(op_buf);
		} else {
			sspi->dma_desc[0]->data_addr = virt_to_phys(op_buf);
		}
		if (sspi->dma_desc[0]->data_addr % CONFIG_SYS_CACHELINE_SIZE) {
			sspi->dma_desc[0]->data_addr = sunxi_spif_get_data_paddr(sspi, op);
			if (op->data.dir == SPI_MEM_DATA_OUT) {
				memcpy((void *)sunxi_spif_get_data_buf(sspi, op),
					(const void *)op_buf, op_len);
			}
		}

		/* addr word alignment */
		sspi->dma_desc[0]->data_addr = sspi->dma_desc[0]->data_addr >> 2;
	}

	if (op->data.nbytes > op_len)
		sunxi_spif_handler_descline(sspi, op);

	return 0;
}

static int sunxi_spif_xfer_last(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	int desc_num = 0;
	struct dma_desc_cache *desc_cache;
	dma_addr_t data_addr;

	if (!sspi->dma_desc[desc_num]->data_addr)
		return 0;

	/* Invalid cache */
	while (desc_num < DESC_PER_DISTRIBUTION_MAX_NUM) {
		data_addr = sspi->dma_desc[desc_num]->data_addr << 2;
		dma_sync_single_for_cpu(&sspi->pdev->dev, data_addr,
					(sspi->dma_desc[desc_num]->block_data_len & DMA_DATA_LEN),
					DMA_FROM_DEVICE);
		if (sspi->dma_desc[desc_num]->next_des_addr)
			desc_num++;
		else
			break;
	}

	list_for_each_entry(desc_cache, &sspi->desc_cache_list, desc_list) {
		data_addr = desc_cache->dma_desc->data_addr << 2;
		dma_sync_single_for_cpu(&sspi->pdev->dev, data_addr,
					(desc_cache->dma_desc->block_data_len & DMA_DATA_LEN),
					DMA_FROM_DEVICE);
	}

	if (((size_t)op->data.buf.in % CONFIG_SYS_CACHELINE_SIZE) &&
			op->data.dir == SPI_MEM_DATA_IN) {
		memcpy((void *)op->data.buf.in,
				(const void *)sunxi_spif_get_data_buf(sspi, op),
				op->data.nbytes);
	}

	if (sspi->temporary_data_cache) {
		dma_free_coherent(&sspi->pdev->dev, op->data.nbytes,
			sspi->temporary_data_cache, sspi->temporary_data_phys);
		sspi->temporary_data_cache = NULL;
	}

	/* Release the excess descriptors */
	while (1) {
		if (list_empty(&sspi->desc_cache_list))
			break;

		desc_cache = list_first_entry(&sspi->desc_cache_list,
				struct dma_desc_cache, desc_list);

		list_del(&desc_cache->desc_list);
		dma_pool_free(sspi->pool, desc_cache->dma_desc, desc_cache->desc_phys);
		kfree(desc_cache);
	}

	return 0;
}

static int sunxi_spif_normal_xfer(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	int timeout = 0xfffff;
	int ret = 0;
	unsigned long flags;
	int desc_num = 0;
	struct dma_desc_cache *desc_cache;
	dma_addr_t data_addr;

	spin_lock_irqsave(&sspi->lock, flags);
	/* clear the dma descriptor */
	for (; desc_num < DESC_PER_DISTRIBUTION_MAX_NUM; desc_num++)
		memset(sspi->dma_desc[desc_num], 0, sizeof(struct dma_descriptor));

	if (sunxi_spif_mem_exec_op(sspi, op)) {
		sunxi_err(&sspi->pdev->dev, "SPIF transfer param abnormity\n");
		ret = -EINVAL;
		goto err;
	}

	sunxi_spif_fifo_reset(sspi);
	if (!op->data.nbytes) {
		sunxi_spif_set_trans_mode(sspi, SPIF_GC_CPU_MODE);

		sunxi_spif_trans_type_enable(sspi, sspi->dma_desc[0]->trans_phase);

		sunxi_spif_set_flash_addr(sspi, sspi->dma_desc[0]->flash_addr);

		sunxi_spif_set_buswidth(sspi, sspi->dma_desc[0]->cmd_mode_buswidth);

		sunxi_spif_set_data_count(sspi, sspi->dma_desc[0]->addr_dummy_data_count);

		sunxi_spif_cpu_start_transfer(sspi);

		while ((readl(sspi->base_addr + SPIF_GC_REG) & SPIF_GC_NMODE_EN)) {
			timeout--;
			if (!timeout) {
				sunxi_spif_ctr_recover(sspi);
				sunxi_err(&sspi->pdev->dev, "SPIF DMA transfer time_out\n");
				ret = -EINVAL;
				goto err;
			}
		}

		complete(&sspi->done);

#if SPIF_DEBUG
		if (spif_debug_flag)
			sunxi_spif_dump_reg(sspi);
#endif
	} else {
		sunxi_spif_enable_irq(sspi, SPIF_DMA_TRANS_DONE_EN | SPIF_INT_STA_ERR_EN);
/*
//CONFIG_DMA_ENGINE
		sunxi_spif_set_trans_mode(sspi, SPIF_GC_CPU_MODE);

		sunxi_spif_trans_type_enable(sspi, sspi->dma_desc->trans_phase);

		suxi_spif_set_flash_addr(sspi, sspi->dma_desc->flash_addr);

		sunxi_spif_set_buswidth(sspi, sspi->dma_desc->cmd_mode_buswidth);

		sunxi_spif_set_data_count(sspi, sspi->dma_desc->addr_dummy_data_count);
*/
		sunxi_spif_set_trans_mode(sspi, SPIF_GC_DMA_MODE);
		sunxi_spif_set_des_start_addr(sspi);

		/* flush data addr */
		desc_num = 0;
		while (desc_num < DESC_PER_DISTRIBUTION_MAX_NUM) {
			data_addr = sspi->dma_desc[desc_num]->data_addr << 2;
			dma_sync_single_for_device(&sspi->pdev->dev, data_addr,
					(sspi->dma_desc[desc_num]->block_data_len & DMA_DATA_LEN),
					DMA_TO_DEVICE);
			if (sspi->dma_desc[desc_num]->next_des_addr)
				desc_num++;
			else
				break;
		}

		list_for_each_entry(desc_cache, &sspi->desc_cache_list, desc_list) {
			data_addr = desc_cache->dma_desc->data_addr << 2;
			dma_sync_single_for_device(&sspi->pdev->dev, data_addr,
					(desc_cache->dma_desc->block_data_len & DMA_DATA_LEN),
					DMA_TO_DEVICE);
		}

		sunxi_spif_start_dma_xfer(sspi);

		/*
		 *  The SPIF move data through DMA, and DMA and CPU modes
		 *  differ only between actively configuring registers and
		 *  configuring registers through the DMA descriptor
		 */
/*
//CONFIG_DMA_ENGINE
		sunxi_spif_cpu_start_transfer(sspi);
*/

		/*
		 * Since the dma transfer completion interrupt is triggered when
		 * THE DRAM moves to the FIFO, first check whether
		 * the SPI transfer is complete
		 */
		if (op->data.dir == SPI_MEM_DATA_OUT) {
			while ((readl(sspi->base_addr + SPIF_GC_REG) & SPIF_GC_NMODE_EN)) {
				timeout--;
				if (!timeout) {
					sunxi_spif_ctr_recover(sspi);
					sunxi_err(&sspi->pdev->dev,
							"SPIF DMA transfer time_out\n");
					ret = -EINVAL;
					goto err;
				}
			}
		}
#if SPIF_DEBUG
		if (spif_debug_flag) {
			sunxi_spif_dump_descriptor(sspi);
			sunxi_spif_dump_reg(sspi);
		}
#endif
	}

err:
	spin_unlock_irqrestore(&sspi->lock, flags);
	return ret;
}

static void sunxi_spif_dtr_enable(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	unsigned int clk = sspi->data->max_speed_hz;
	unsigned int dtr_double_clk = clk * 2;

	if (CHECK_DTR_OP(op->cmd.opcode)) {
		if (!double_clk_flag) {
			sunxi_spif_set_output_clk(sspi, 1);
			sunxi_spif_set_dtr(sspi, 1);
			sunxi_spif_set_clk(dtr_double_clk, clk_get_rate(sspi->mclk), sspi);
			double_clk_flag = 1;
		}
	} else {
		if (double_clk_flag) {
			sunxi_spif_set_output_clk(sspi, 0);
			sunxi_spif_set_dtr(sspi, 0);
			sunxi_spif_set_clk(clk, clk_get_rate(sspi->mclk), sspi);
			double_clk_flag = 0;
		}
	}
}

/*
 * setup the spif controller according to the characteristics of the transmission
 * return 0:succeed, < 0:failed.
 */
static int sunxi_spif_xfer_setup(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	sunxi_spif_ss_select(sspi);

	sunxi_spif_dtr_enable(sspi, op);

	return 0;
}

static int sunxi_spif_xfer(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	u32 err;

	if (sspi->working_mode & PREFETCH_READ_MODE) {
		err = sunxi_spif_prefetch_xfer(sspi, op);
		if (err) {
			sunxi_err(&sspi->pdev->dev, "prefetch mode xfer failed\n");
			return err;
		}
	} else {
		err = sunxi_spif_normal_xfer(sspi, op);
		if (err) {
			sunxi_err(&sspi->pdev->dev, "normal mode xfer failed\n");
			return err;
		}
	}

	return 0;
}

/*
 * the interface to connect spi framework
 * wait for done completion in this function, wakup in the irq hanlder
 */
static int sunxi_spif_transfer_one(struct sunxi_spif *sspi, struct spi_mem_op *op)
{
	unsigned long timeout;
	int err;

	err = sunxi_spif_xfer_setup(sspi, op);
	if (err)
		return -EINVAL;

	sunxi_spif_xfer(sspi, op);

	/* wait for xfer complete in the isr. */
	timeout = wait_for_completion_timeout(&sspi->done,
			msecs_to_jiffies(XFER_TIMEOUT));
	if (timeout == 0) {
		sunxi_spif_ctr_recover(sspi);
		sunxi_err(&sspi->pdev->dev, "xfer timeout\n");
		err = -EINVAL;
	} else if (sspi->result < 0) {
		sunxi_spif_ctr_recover(sspi);
		/* after transfer error, must reset the clk and fifo */
		//sunxi_spif_soft_reset(sspi);
		//sunxi_spif_fifo_reset(sspi);
		sunxi_err(&sspi->pdev->dev, "xfer failed...\n");
		err = -EINVAL;
	} else
		sunxi_spif_xfer_last(sspi, op);

	sunxi_spif_disable_irq(sspi, (SPIF_DMA_TRANS_DONE_EN | SPIF_INT_STA_ERR_EN));

	return err;
}

static int sunxi_spif_resource_get(struct sunxi_spif *sspi)
{
	struct device_node *np = sspi->pdev->dev.of_node;
	struct platform_device *pdev = sspi->pdev;
	struct resource *mem_res;
	int ret = 0;

	pdev->id = of_alias_get_id(np, "spif");
	if (pdev->id < 0) {
		sunxi_err(&pdev->dev, "failed to get alias id\n");
		return pdev->id;
	}
	snprintf(sspi->dev_name, sizeof(sspi->dev_name), SUNXI_SPIF_DEV_NAME"%d", pdev->id);

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR_OR_NULL(mem_res)) {
		sunxi_err(&sspi->pdev->dev, "resource get failed\n");
		return -EINVAL;
	}
	sspi->base_addr_phy = mem_res->start;
	sspi->base_addr = devm_ioremap_resource(&pdev->dev, mem_res);
	if (IS_ERR_OR_NULL(sspi->base_addr)) {
		sunxi_err(&sspi->pdev->dev, "spif unable to ioremap\n");
		return -EINVAL;
	}

	sspi->pclk = devm_clk_get(&sspi->pdev->dev, "pclk");
	if (IS_ERR(sspi->pclk)) {
		sunxi_err(&sspi->pdev->dev, "spif unable to acquire parent clock\n");
		return -EINVAL;
	}

	sspi->mclk = devm_clk_get(&sspi->pdev->dev, "mclk");
	if (IS_ERR(sspi->mclk)) {
		sunxi_err(&sspi->pdev->dev, "spif unable to acquire mode clock\n");
		return -EINVAL;
	}

	sspi->bus = devm_clk_get(&sspi->pdev->dev, "bus");
	if (IS_ERR(sspi->mclk)) {
		sunxi_err(&sspi->pdev->dev, "spif unable to acquire bus clock\n");
		return -EINVAL;
	}

	sspi->irq = platform_get_irq(sspi->pdev, 0);
	if (sspi->irq < 0) {
		sunxi_err(&sspi->pdev->dev, "get irq failed\n");
		return sspi->irq;
	}

	sspi->pctrl = devm_pinctrl_get(&sspi->pdev->dev);
	if (IS_ERR(sspi->pctrl)) {
		sunxi_err(&sspi->pdev->dev, "pin get failed!\n");
		return PTR_ERR(sspi->pctrl);
	} else {
		sunxi_spif_select_gpio_state(sspi, PINCTRL_STATE_DEFAULT);
	}

	/* linux-4.9 need't do reset and assert working(ccmu do that) */
/*
	sspi->rstc = devm_reset_control_get(&sspi->pdev->dev, NULL);
	if (IS_ERR_OR_NULL(sspi->rstc)) {
		sunxi_err(&sspi->pdev->dev, "Couldn't get reset controller\n");
		return PTR_ERR(sspi->rstc);
	}
*/

	ret = sunxi_spif_regulator_request(sspi);
	if (ret) {
		sunxi_err(&pdev->dev, "request regulator failed\n");
		return ret;
	}

	ret = of_property_read_u32(np, "clock-frequency", &sspi->data->max_speed_hz);
	if (ret) {
		sspi->data->max_speed_hz = SPIF_DEFAULT_SPEED_HZ;
		sunxi_err(&pdev->dev, "get spif controller working speed_hz failed,\
				set default spped_hz %d\n", SPIF_DEFAULT_SPEED_HZ);
	}

	/* Get sampling delay parameters */
	ret = of_property_read_u32(np, "sample_mode", &sspi->data->sample_mode);
	if (ret) {
		sunxi_err(&pdev->dev, "Failed to get sample mode\n");
		sspi->data->sample_mode = SAMP_MODE_DL_DEFAULT;
	}
	ret = of_property_read_u32(np, "sample_delay", &sspi->data->sample_delay);
	if (ret) {
		sunxi_err(&pdev->dev, "Failed to get sample delay\n");
		sspi->data->sample_delay = SAMP_MODE_DL_DEFAULT;
	}
	sunxi_info(&sspi->pdev->dev, "sample_mode:%x sample_delay:%x\n",
				sspi->data->sample_mode, sspi->data->sample_delay);

	/* AW SPIF controller self working mode */
	if (of_property_read_bool(np, "prefetch_read_mode_enabled")) {
		sunxi_info(&sspi->pdev->dev, "prefetch read mode enabled");
		sspi->working_mode |= PREFETCH_READ_MODE;
	}
	if (of_property_read_bool(np, "dqs_mode_enabled")) {
		sunxi_debug(&sspi->pdev->dev, "DQS mode enabled");
		sspi->working_mode |= DQS;
	}

	return 0;
}

static void sunxi_spif_resource_put(struct sunxi_spif *sspi)
{
	sunxi_spif_regulator_release(sspi);
}

static void sunxi_spif_init_hwcaps(struct sunxi_spif *sspi, struct spi_nor_hwcaps *hwcaps)
{
	struct device_node *np = sspi->pdev->dev.of_node;
	u32 bus_width;

	if (!of_property_read_u32(np, "spif-rx-bus-width", &bus_width)) {
		switch (bus_width) {
		case 1:
			break;
		case 2:
			hwcaps->mask |= SNOR_HWCAPS_READ_DUAL;
			break;
		case 4:
			hwcaps->mask |= SNOR_HWCAPS_READ_QUAD;
			break;
		case 8:
			hwcaps->mask |= SNOR_HWCAPS_READ_OCTAL;
			break;
		default:
			sunxi_warn(&sspi->pdev->dev,
				"spi-tx-bus-width %d not supported\n",
				bus_width);
			break;
		}
	}

	if (!of_property_read_u32(np, "spif-tx-bus-width", &bus_width)) {
		switch (bus_width) {
		case 1:
			break;
		case 4:
			hwcaps->mask |= SNOR_HWCAPS_PP_QUAD;
			break;
		case 8:
			hwcaps->mask |= SNOR_HWCAPS_PP_OCTAL;
			break;
		default:
			sunxi_warn(&sspi->pdev->dev,
				"spi-tx-bus-width %d not supported\n",
				bus_width);
			break;
		}
	}

	return;
}

/*
static int sunxi_spif_nor_prep(struct spi_nor *nor)
{
	struct sunxi_spif *sspi = nor->priv;
	int ret = 0;

	mutex_lock(&sspi->lock);

	ret = sunxi_spif_clk_init(sspi);
	if (ret)
		goto out;

	return 0;

out:
	mutex_unlock(&sspi->lock);
	return ret;
}

static void sunxi_spif_nor_unprep(struct spi_nor *nor)
{
	struct sunxi_spif *sspi = nor->priv;

	sunxi_spif_clk_exit(sspi);
	mutex_unlock(&sspi->lock);
}
*/

static int sunxi_spif_nor_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf, size_t len)
{
	struct sunxi_spif *sspi = nor->priv;
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(opcode, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_IN(len, buf, 1));

	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(nor->read_proto);
	op.dummy.buswidth = 1;
	op.data.buswidth = op.cmd.buswidth;

	return sspi->transfer_one(sspi, &op);
}

static int sunxi_spif_nor_write_reg(struct spi_nor *nor, u8 opcode, const u8 *buf,
				size_t len)
{
	struct sunxi_spif *sspi = nor->priv;
	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(opcode, 1),
				   SPI_MEM_OP_NO_ADDR,
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_DATA_OUT(len, buf, 1));

	/* get transfer protocols. */
	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(nor->write_proto);

	return sspi->transfer_one(sspi, &op);
}

/**
 * sunxi_spif_nor_read() - read data from flash memory
 * @nor:        pointer to 'struct spi_nor'
 * @from:       offset to read from
 * @len:        number of bytes to read
 * @buf:        pointer to dst buffer
 *
 * Return: number of bytes read successfully, -errno otherwise
 */
static ssize_t sunxi_spif_nor_read(struct spi_nor *nor, loff_t from, size_t len, u8 *buf)
{
	struct sunxi_spif *sspi = nor->priv;
	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(nor->read_opcode, 1),
				   SPI_MEM_OP_ADDR(nor->addr_width, from, 1),
				   SPI_MEM_OP_DUMMY(nor->read_dummy, 1),
				   SPI_MEM_OP_DATA_IN(len, buf, 1));
	size_t remaining = len;
	ssize_t ret;

	/* get transfer protocols. */
	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(nor->read_proto);
	op.addr.buswidth = spi_nor_get_protocol_addr_nbits(nor->read_proto);
	op.dummy.buswidth = op.addr.buswidth;
	op.data.buswidth = spi_nor_get_protocol_data_nbits(nor->read_proto);

	while (remaining) {
		op.data.nbytes = remaining < UINT_MAX ? remaining : UINT_MAX;

		ret = sspi->transfer_one(sspi, &op);
		if (ret)
			return ret;

		op.addr.val += op.data.nbytes;
		remaining -= op.data.nbytes;
		op.data.buf.in += op.data.nbytes;
	}

	return len;
}

/**
 * sunxi_spif_nor_write() - write data to flash memory
 * @nor:        pointer to 'struct spi_nor'
 * @to:         offset to write to
 * @len:        number of bytes to write
 * @buf:        pointer to src buffer
 *
 * Return: number of bytes written successfully, -errno otherwise
 */
static ssize_t sunxi_spif_nor_write(struct spi_nor *nor, loff_t to, size_t len,
				  const u8 *buf)
{
	struct sunxi_spif *sspi = nor->priv;
	struct spi_mem_op op =
			SPI_MEM_OP(SPI_MEM_OP_CMD(nor->program_opcode, 1),
				   SPI_MEM_OP_ADDR(nor->addr_width, to, 1),
				   SPI_MEM_OP_NO_DUMMY,
				   SPI_MEM_OP_DATA_OUT(len, buf, 1));
	ssize_t ret;

	/* get transfer protocols. */
	op.cmd.buswidth = spi_nor_get_protocol_inst_nbits(nor->write_proto);
	op.addr.buswidth = spi_nor_get_protocol_addr_nbits(nor->write_proto);
	op.data.buswidth = spi_nor_get_protocol_data_nbits(nor->write_proto);

	if (nor->program_opcode == SPINOR_OP_AAI_WP && nor->sst_write_second)
		op.addr.nbytes = 0;

	ret = sspi->transfer_one(sspi, &op);
	if (ret)
		return ret;

	return op.data.nbytes;
}

static const struct spi_nor_controller_ops sunxi_controller_ops = {
	//.prepare = sunxi_spif_nor_prep,
	//.unprepare = sunxi_spif_nor_unprep,
	.read_reg = sunxi_spif_nor_read_reg,
	.write_reg = sunxi_spif_nor_write_reg,
	.read = sunxi_spif_nor_read,
	.write = sunxi_spif_nor_write,
};

/*
 * Get spi flash device information and register it as a mtd device.
 */
static int sunxi_spif_nor_register(struct sunxi_spif *sspi)
{
	struct spi_nor *nor;
	struct spi_nor_hwcaps hwcaps = {
		.mask = SNOR_HWCAPS_READ |
			SNOR_HWCAPS_READ_FAST |
			SNOR_HWCAPS_PP,
	};
	int ret;
	const char * const type[] = {
		"sunxipart",
		NULL
	};

	sunxi_spif_init_hwcaps(sspi, &hwcaps);

	nor = devm_kzalloc(&sspi->pdev->dev, sizeof(*nor), GFP_KERNEL);
	if (!nor)
		return -ENOMEM;

	sspi->nor = nor;
	nor->priv = sspi;
	nor->controller_ops = &sunxi_controller_ops;
	nor->dev = &sspi->pdev->dev;
	spi_nor_set_flash_node(nor, sspi->pdev->dev.of_node);

	nor->mtd.name = sspi->pdev->dev.of_node->name;

	printk("nor->mtd.name:%s \n", nor->mtd.name);
	ret = spi_nor_scan(nor, NULL, &hwcaps);
	if (ret)
		return ret;

	/*
	 * None of the existing parts have > 512B pages, but let's play safe
	 * and add this logic so that if anyone ever adds support for such
	 * a NOR we don't end up with buffer overflows.
	 */
	if (nor->page_size > PAGE_SIZE) {
		nor->bouncebuf_size = nor->page_size;
		devm_kfree(nor->dev, nor->bouncebuf);
		nor->bouncebuf = devm_kmalloc(nor->dev,
					      nor->bouncebuf_size,
					      GFP_KERNEL);
		if (!nor->bouncebuf)
			return -ENOMEM;
	}

	return mtd_device_parse_register(&nor->mtd, type, NULL, NULL, 0);
}

static struct sunxi_spif_data sun55iw3_spif_data = {
	.max_speed_hz	= 100000000, /* 150M */
	.min_speed_hz	= 25000000,	/* 25M */
	/* SPIF don't support customize gpio as cs */
	.bus_num	= 0,
	.chip_select	= 0,
	.sample_mode	= SAMP_MODE_DL_DEFAULT,
	.sample_delay	= SAMP_MODE_DL_DEFAULT,
};

static const struct of_device_id sunxi_spif_dt_ids[] = {
	{.compatible = "allwinner,sun55i-spif", .data = &sun55iw3_spif_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_spif_dt_ids);

static int sunxi_spif_probe(struct platform_device *pdev)
{
	struct sunxi_spif *sspi;
	const struct of_device_id *of_id;
	int err = 0, i;

	g_sspi = sspi = devm_kzalloc(&pdev->dev, sizeof(struct sunxi_spif), GFP_KERNEL);
	if (!sspi) {
		sunxi_err(&pdev->dev, "devm_kzalloc() failed\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, sspi);

	of_id = of_match_device(sunxi_spif_dt_ids, &pdev->dev);
	if (!of_id) {
		sunxi_err(&pdev->dev, "of_match_device() failed\n");
		return PTR_ERR(of_id);
	}

	sspi->pdev = pdev;
	sspi->data = (struct sunxi_spif_data *)(of_id->data);

	sspi->pool = dmam_pool_create(dev_name(&pdev->dev), &pdev->dev,
		sizeof(struct dma_descriptor), 4, 0);
	if (!sspi->pool) {
		sunxi_err(&pdev->dev, "No memory for descriptors dma pool\n");
		err = ENOMEM;
		goto err0;
	}

	for (i = 0; i < DESC_PER_DISTRIBUTION_MAX_NUM; i++) {
		sspi->dma_desc[i] = dma_pool_zalloc(sspi->pool, GFP_NOWAIT,
				&sspi->desc_phys[i]);
		if (!sspi->dma_desc[i]) {
			sunxi_err(&pdev->dev, "Failed to alloc dma descriptor memory\n");
			err = PTR_ERR(sspi->dma_desc[i]);
			if (i)
				goto err2;
			else
				goto err1;
		}
		sunxi_debug(&pdev->dev, "dma descriptor phys addr is %llx\n", sspi->desc_phys[i]);
	}

	INIT_LIST_HEAD(&sspi->desc_cache_list);

	sspi->cache_buf = dma_alloc_coherent(&pdev->dev, CONFIG_SYS_CACHELINE_SIZE,
			&sspi->cache_phys, GFP_KERNEL);
	if (!sspi->cache_buf) {
		sunxi_err(&pdev->dev, "Failed to alloc cache buf memory\n");
		err = PTR_ERR(sspi->cache_buf);
		goto err2;
	}

	sspi->data_buf = dma_alloc_coherent(&pdev->dev, CONFIG_SYS_MAXDATA_SIZE,
			&sspi->data_phys, GFP_KERNEL);
	if (!sspi->data_buf) {
		sunxi_err(&pdev->dev, "Failed to alloc cache buf memory\n");
		err = PTR_ERR(sspi->data_buf);
		goto err3;
	}

	sspi->temporary_data_cache = NULL;

	err = sunxi_spif_resource_get(sspi);
	if (err)
		goto err4;

	sspi->data->bus_num = pdev->id;
	sspi->transfer_one = sunxi_spif_transfer_one;

	err = sunxi_spif_hw_init(sspi);
	if (err)
		goto err5;

	err = devm_request_irq(&pdev->dev, sspi->irq, sunxi_spif_handler, 0,
			sspi->dev_name, sspi);
	if (err) {
		sunxi_err(&pdev->dev, " Cannot request irq %d\n", sspi->irq);
		goto err6;
	}

	spin_lock_init(&sspi->lock);
	init_completion(&sspi->done);

	err = sunxi_spif_nor_register(sspi);
	if (err) {
		sunxi_err(&pdev->dev, " sunxi spif nor register error:%d\n", err);
		goto err6;
	}

	sunxi_info(&pdev->dev, "probe succeed (Version %s)\n", SUNXI_SPIF_MODULE_VERSION);
	return 0;

err6:
	sunxi_spif_hw_deinit(sspi);
err5:
	sunxi_spif_resource_put(sspi);
err4:
	dma_free_coherent(&pdev->dev, CONFIG_SYS_MAXDATA_SIZE,
			sspi->data_buf, sspi->data_phys);
err3:
	dma_free_coherent(&pdev->dev, CONFIG_SYS_CACHELINE_SIZE,
			sspi->cache_buf, sspi->cache_phys);
err2:
	for (; i > 0; i--)
		dma_pool_free(sspi->pool, sspi->dma_desc[i - 1], sspi->desc_phys[i - 1]);
err1:
	dmam_pool_destroy(sspi->pool);
err0:
	platform_set_drvdata(pdev, NULL);
	return err;
}

static int sunxi_spif_remove(struct platform_device *pdev)
{
	struct sunxi_spif *sspi = platform_get_drvdata(pdev);
	int i;

	sunxi_spif_hw_deinit(sspi);
	sunxi_spif_resource_put(sspi);
	for (i = 0; i < DESC_PER_DISTRIBUTION_MAX_NUM; i++)
		dma_pool_free(sspi->pool, sspi->dma_desc[i], sspi->desc_phys[i]);
	dmam_pool_destroy(sspi->pool);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static void sunxi_spif_save_register(struct sunxi_spif *sspi)
{
	int i;
	for (i = 0; i < 24; i++)
		sspi->spif_register[i] = readl(sspi->base_addr + 0x04 * i);
}

static void sunxi_spif_restore_register(struct sunxi_spif *sspi)
{
	int i;
	for (i = 0; i < 24; i++)
		writel((size_t)sspi->base_addr + 0x04 * i, sspi->spif_register + i);
}

static int sunxi_spif_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_spif *sspi = platform_get_drvdata(pdev);

	sunxi_spif_save_register(sspi);

	sunxi_spif_hw_deinit(sspi);

	sunxi_info(&pdev->dev, "[spi-flash%d] suspend finish\n", sspi->data->bus_num);

	return 0;
}

static int sunxi_spif_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct sunxi_spif  *sspi = platform_get_drvdata(pdev);
	int ret;

	ret = sunxi_spif_hw_init(sspi);
	if (ret) {
		sunxi_err(&pdev->dev, "spif resume error\n");
		return ret;
	}

	sunxi_spif_restore_register(sspi);

	sunxi_info(&pdev->dev, "[spi-flash%d] resume finish\n", sspi->data->bus_num);

	return ret;
}

static const struct dev_pm_ops sunxi_spif_dev_pm_ops = {
	.suspend = sunxi_spif_suspend,
	.resume  = sunxi_spif_resume,
};

#define SUNXI_SPIF_DEV_PM_OPS (&sunxi_spif_dev_pm_ops)
#else
#define SUNXI_SPIF_DEV_PM_OPS NULL
#endif /* CONFIG_PM */

static struct platform_driver sunxi_spif_driver = {
	.probe   = sunxi_spif_probe,
	.remove  = sunxi_spif_remove,
	.driver = {
		.name		= SUNXI_SPIF_DEV_NAME,
		.owner		= THIS_MODULE,
		.pm		= SUNXI_SPIF_DEV_PM_OPS,
		.of_match_table = sunxi_spif_dt_ids,
	},
};
module_platform_driver(sunxi_spif_driver);

MODULE_AUTHOR("lujianliang@allwinnertech.com");
MODULE_DESCRIPTION("Sunxi SPI Nor Flash Controller Driver");
MODULE_ALIAS("platform:"SUNXI_SPIF_DEV_NAME);
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_SPIF_MODULE_VERSION);
