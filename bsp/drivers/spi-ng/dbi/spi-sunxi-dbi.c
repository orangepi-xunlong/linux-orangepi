/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller DBI Driver
 *
 */

#define SUNXI_MODNAME "spi"
#include <sunxi-log.h>
#include "spi-sunxi-dbi.h"
#include "../spi-sunxi-debug.h"

static int sunxi_dbi_debug_mask;
module_param_named(dbi_debug_mask, sunxi_dbi_debug_mask, int, 0664);

/* SPI Controller Hardware Register Operation Start */

void sunxi_dbi_disable_dma(struct sunxi_spi *sspi)
{
	u32 reg_old, reg_new;
	reg_new = reg_old = readl(sspi->base_addr + SUNXI_DBI_CTL2_REG);
	reg_new &= ~(SUNXI_DBI_CTL2_FIFO_DRQ_EN);
	if (reg_new != reg_old)
		writel(reg_new, sspi->base_addr + SUNXI_DBI_CTL2_REG);
}

void sunxi_dbi_enable_dma(struct sunxi_spi *sspi)
{
	u32 reg_old, reg_new;
	reg_new = reg_old = readl(sspi->base_addr + SUNXI_DBI_CTL2_REG);
	reg_new |= SUNXI_DBI_CTL2_FIFO_DRQ_EN;
	if (reg_new != reg_old)
		writel(reg_new, sspi->base_addr + SUNXI_DBI_CTL2_REG);
}

inline u32 sunxi_dbi_qry_irq_pending(struct sunxi_spi *sspi)
{
	return (SUNXI_DBI_INT_STA_MASK & readl(sspi->base_addr + SUNXI_DBI_INT_REG));
}

void sunxi_dbi_clr_irq_pending(struct sunxi_spi *sspi, u32 bitmap)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_DBI_INT_REG);
	bitmap &= SUNXI_DBI_INT_STA_MASK;
	reg_val |= bitmap;
	writel(reg_val, sspi->base_addr + SUNXI_DBI_INT_REG);
}

inline u32 sunxi_dbi_qry_irq_enable(struct sunxi_spi *sspi)
{
	return (SUNXI_DBI_INT_EN_MASK & readl(sspi->base_addr + SUNXI_DBI_INT_REG));
}

void sunxi_dbi_disable_irq(struct sunxi_spi *sspi, u32 bitmap)
{
	u32 reg_old, reg_new;
	reg_new = reg_old = readl(sspi->base_addr + SUNXI_DBI_INT_REG);
	bitmap &= SUNXI_DBI_INT_EN_MASK;
	reg_new &= ~bitmap;
	if (reg_new != reg_old)
		writel(reg_new, sspi->base_addr + SUNXI_DBI_INT_REG);
}

void sunxi_dbi_enable_irq(struct sunxi_spi *sspi, u32 bitmap)
{
	u32 reg_old, reg_new;
	reg_new = reg_old = readl(sspi->base_addr + SUNXI_DBI_INT_REG);
	bitmap &= SUNXI_DBI_INT_EN_MASK;
	reg_new |= bitmap;
	if (reg_new != reg_old)
		writel(reg_new, sspi->base_addr + SUNXI_DBI_INT_REG);
}

static s32 sunxi_dbi_set_timer_param(struct sunxi_spi *sspi)
{
	return -1;
}

void sunxi_dbi_config(struct sunxi_spi *sspi)
{
	u32 reg0_val = 0x0;
	u32 reg1_val = 0x0;
	u32 reg2_val = 0x4000;
	u32 reg_tmp = 0;
	u32 dbi_mode = sspi->dbi_config->dbi_mode;

	sspi->dbi_irqbit = 0;

	/* DBI_CTL_0 */
	if (dbi_mode & SUNXI_DBI_COMMAND_READ)
		reg0_val |= SUNXI_DBI_CTL0_CMDT;
	else
		reg0_val &= ~SUNXI_DBI_CTL0_CMDT;

	if (dbi_mode & SUNXI_DBI_LSB_FIRST)
		reg0_val |= SUNXI_DBI_CTL0_DAT_SEQ;
	else
		reg0_val &= ~SUNXI_DBI_CTL0_DAT_SEQ;

	reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_RGB_SEQ, sspi->dbi_config->dbi_out_sequence);

	if (dbi_mode & SUNXI_DBI_TRANSMIT_VIDEO) {
		reg0_val |= SUNXI_DBI_CTL0_TRAN_MOD;
		reg_tmp = FIELD_PREP(SUNXI_DBI_VIDEO_SIZE_V, sspi->dbi_config->dbi_video_v) | FIELD_PREP(SUNXI_DBI_VIDEO_SIZE_H, sspi->dbi_config->dbi_video_h);
		writel(reg_tmp, sspi->base_addr + SUNXI_DBI_VIDEO_SIZE_REG);
		if (sspi->dbi_config->dbi_te_en)
			sspi->dbi_irqbit |= SUNXI_DBI_TE_INT_EN;
		else
			sspi->dbi_irqbit |= SUNXI_DBI_FRAM_DONE_INT_EN;
	} else {
		reg0_val &= ~SUNXI_DBI_CTL0_TRAN_MOD;
		writel(0x0, sspi->base_addr + SUNXI_DBI_VIDEO_SIZE_REG);
	}

	reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_DAT_FMT, sspi->dbi_config->dbi_format);
	reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_INTF, sspi->dbi_config->dbi_interface);
	reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_RGB_FMT, sspi->dbi_config->dbi_src_sequence);
	reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_RGB_BO, sspi->dbi_config->dbi_rgb_bit_order);
	reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_ELEMENT_A_POS, sspi->dbi_config->dbi_rgb32_alpha_pos);

	if (sspi->dbi_config->dbi_format <= SUNXI_DBI_FMT_RGB565)
		reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_VI_SRC_TYPE, SUNXI_DBI_RGB_SRC_TYPE_RGB16);
	else
		reg0_val |= FIELD_PREP(SUNXI_DBI_CTL0_VI_SRC_TYPE, SUNXI_DBI_RGB_SRC_TYPE_RGB32);

	/* DBI_CTL_1 */
	if ((sspi->dbi_config->dbi_te_en == SUNXI_DBI_TE_DISABLE) || !(dbi_mode & SUNXI_DBI_TRANSMIT_VIDEO)) {
		if (!sunxi_dbi_set_timer_param(sspi))
			reg1_val |= FIELD_PREP(SUNXI_DBI_CTL1_EN_MOD_SEL, SUNXI_DBI_MODE_TIMER_TRIG); /* timer trigger mode */
		else
			reg1_val |= FIELD_PREP(SUNXI_DBI_CTL1_EN_MOD_SEL, SUNXI_DBI_MODE_ALWAYS_ON); /* always on mode */
	} else {
		reg1_val |= FIELD_PREP(SUNXI_DBI_CTL1_EN_MOD_SEL, SUNXI_DBI_MODE_TE_TRIG); /* te trigger mode */
	}

	if (sspi->dbi_config->dbi_clk_out_mode == SUNXI_DBI_CLK_ALWAYS_ON)
		reg1_val &= ~SUNXI_DBI_CTL1_CLKO_MOD;
	else
		reg1_val |= SUNXI_DBI_CTL1_CLKO_MOD;

	if (dbi_mode & SUNXI_DBI_DCX_DATA)
		reg1_val |= SUNXI_DBI_CTL1_DCX_DATA;
	else
		reg1_val &= ~SUNXI_DBI_CTL1_DCX_DATA;

	reg1_val |= FIELD_PREP(SUNXI_DBI_CTL1_RGB16_SEL, sspi->dbi_config->dbi_rgb16_pixel_endian);

	if (dbi_mode & SUNXI_DBI_COMMAND_READ) {
		reg1_val |= FIELD_PREP(SUNXI_DBI_CTL1_RCDC, sspi->dbi_config->dbi_read_dummy);
		reg1_val |= FIELD_PREP(SUNXI_DBI_CTL1_RDBN, sspi->dbi_config->dbi_read_bytes);
		sspi->dbi_irqbit |= SUNXI_DBI_RD_DONE_INT_EN;
	}

	/* DBI_CTL_2 */
	if (sspi->dbi_config->dbi_interface == SUNXI_DBI_INTF_D2LI) {
		reg2_val |= SUNXI_DBI_CTL2_DCX_SEL;
		reg2_val &= ~SUNXI_DBI_CTL2_SDI_OUT_SEL;
	} else {
		reg2_val &= ~SUNXI_DBI_CTL2_DCX_SEL;
		reg2_val |= SUNXI_DBI_CTL2_SDI_OUT_SEL;
	}

	if ((sspi->dbi_config->dbi_te_en == SUNXI_DBI_TE_DISABLE) || !(dbi_mode & SUNXI_DBI_TRANSMIT_VIDEO)) {
		reg2_val &= ~SUNXI_DBI_CTL2_TE_EN;
	} else {
		reg2_val |= SUNXI_DBI_CTL2_TE_EN;
		if (sspi->dbi_config->dbi_te_en == SUNXI_DBI_TE_FALLING_EDGE)
			reg2_val |= SUNXI_DBI_CTL2_TE_TRIG_SEL;
		else
			reg2_val &= ~SUNXI_DBI_CTL2_TE_TRIG_SEL;
	}

	writel(reg0_val, sspi->base_addr + SUNXI_DBI_CTL0_REG);
	writel(reg1_val, sspi->base_addr + SUNXI_DBI_CTL1_REG);
	writel(reg2_val, sspi->base_addr + SUNXI_DBI_CTL2_REG);
	sunxi_debug(sspi->dev, "dbi config ctl0(%#x) ctl1(%#x) ctl2(%#x)\n", reg0_val, reg1_val, reg2_val);
	if (sunxi_dbi_debug_mask & SUNXI_SPI_DEBUG_DUMP_REG)
		sunxi_spi_dump_reg(sspi->dev, sspi->base_addr + 0x100, sspi->base_addr_phy + 0x100, 0x10);
}

/* SPI Controller Hardware Register Operation End */

int sunxi_dbi_mode_check_dbi(struct sunxi_spi *sspi, struct spi_transfer *t)
{
	int ret = 0;

	if (t->tx_buf && t->rx_buf) {
		sunxi_err(sspi->dev, "dbi mode not support full duplex mode\n");
		ret = -EINVAL;
	} else {
		if (t->tx_buf)
			sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
		else if (t->rx_buf)
			sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
	}
	sunxi_debug(sspi->dev, "dbi mode type %d\n", sspi->mode_type);

	return ret;
}

void sunxi_dbi_handler(struct sunxi_spi *sspi)
{
	u32 status = 0, enable = 0, irq = 0;
	bool compelte = false;

	enable = sunxi_dbi_qry_irq_enable(sspi);
	status = sunxi_dbi_qry_irq_pending(sspi);
	sunxi_dbi_clr_irq_pending(sspi, status);
	sunxi_debug(sspi->dev, "dbi irq handler enable(%#x) status(%#x)\n", enable, status);

	if ((enable & SUNXI_DBI_FIFO_EMPTY_INT_EN) && (status & SUNXI_DBI_FIFO_EMPTY_INT) && !(sspi->dbi_config->dbi_mode & SUNXI_DBI_TRANSMIT_VIDEO)) {
		sunxi_debug(sspi->dev, "dbi irq fifo empty\n");
		irq |= SUNXI_DBI_FIFO_EMPTY_INT_EN;
		compelte = true;
	}
	if ((enable & SUNXI_DBI_RD_DONE_INT_EN) && (status & SUNXI_DBI_RD_DONE_INT)) {
		sunxi_debug(sspi->dev, "dbi irq read done\n");
		irq |= SUNXI_DBI_RD_DONE_INT_EN;
		compelte = true;
	}
	if ((enable & SUNXI_DBI_TE_INT_EN) && (status & SUNXI_DBI_TE_INT)) {
		sunxi_debug(sspi->dev, "dbi irq te signal has been changed\n");
		if (sspi->dbi_config->dbi_vsync_handle && sspi->dbi_dev)
			sspi->dbi_config->dbi_vsync_handle((unsigned long)sspi->dbi_dev);
		compelte = true;
	}
	if ((enable & SUNXI_DBI_FRAM_DONE_INT_EN) && (status & SUNXI_DBI_FRAM_DONE_INT) && (sspi->dbi_config->dbi_mode & SUNXI_DBI_TRANSMIT_VIDEO)) {
		sunxi_debug(sspi->dev, "dbi irq frame done\n");
		irq |= SUNXI_DBI_FRAM_DONE_INT_EN;
		compelte = true;
	}

	sunxi_dbi_disable_irq(sspi, irq);
	if (compelte)
		complete(&sspi->done);
}

int sunxi_dbi_get_config(const struct spi_device *spi, struct sunxi_dbi_config *dbi_config)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);

	if (sspi->bus_mode != SUNXI_SPI_BUS_DBI) {
		sunxi_err(sspi->dev, "bus was not dbi mode, dbi config get failed\n");
		return -EINVAL;
	}

	memcpy(dbi_config, sspi->dbi_config, sizeof(*dbi_config));

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_dbi_get_config);

int sunxi_dbi_set_config(struct spi_device *spi, const struct sunxi_dbi_config *dbi_config)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);

	if (sspi->bus_mode != SUNXI_SPI_BUS_DBI) {
		sunxi_err(sspi->dev, "bus was not dbi mode, dbi config set failed\n");
		return -EINVAL;
	}

	memcpy(sspi->dbi_config, dbi_config, sizeof(*dbi_config));

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_dbi_set_config);
