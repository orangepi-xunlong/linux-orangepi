/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller Camera Driver
 *
 */

#define SUNXI_MODNAME "spi"
#include <sunxi-log.h>
#include "spi-sunxi-camera.h"
#include "../spi-sunxi-debug.h"

/* SPI Controller Hardware Register Operation Start */

void sunxi_spi_camera_enable_idlewait(struct sunxi_spi *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);
	reg_val |= SUNXI_SPI_TC_SIWE;
	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

void sunxi_spi_camera_disable_idlewait(struct sunxi_spi *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);
	reg_val &= ~SUNXI_SPI_TC_SIWE;
	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

void sunxi_spi_camera_enable_framehead(struct sunxi_spi *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);
	reg_val |= SUNXI_SPI_TC_SFHE;
	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

void sunxi_spi_camera_disable_framehead(struct sunxi_spi *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);
	reg_val &= ~SUNXI_SPI_TC_SFHE;
	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

void sunxi_spi_camera_enable_vsync(struct sunxi_spi *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);
	reg_val |= SUNXI_SPI_TC_VIE;
	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

void sunxi_spi_camera_disable_vsync(struct sunxi_spi *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);
	reg_val &= ~SUNXI_SPI_TC_VIE;
	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_camera_vsync_input_select(struct sunxi_spi *sspi, bool edge)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_TC_REG);

	if (edge)
		reg_val |= SUNXI_SPI_TC_VIS;
	else
		reg_val &= ~SUNXI_SPI_TC_VIS;

	writel(reg_val, sspi->base_addr + SUNXI_SPI_TC_REG);
}

static void sunxi_spi_camera_set_vsync_freq_relation(struct sunxi_spi *sspi, u32 ahb_clk, u32 sclk)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_SVCN_REG);
	u8 val = (ahb_clk / sclk) - 1;

	reg_val &= ~SUNXI_SPI_SVCN_SFT;
	reg_val |= FIELD_PREP(SUNXI_SPI_SVCN_SFT, val);
	writel(reg_val, sspi->base_addr + SUNXI_SPI_SVCN_REG);
}

static void sunxi_spi_camera_set_vsync_cycle_number(struct sunxi_spi *sspi, u32 num)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_SVCN_REG);
	reg_val &= ~SUNXI_SPI_SVCN_SVCN;
	/* Convert Byte to Cycle -> 1Byte = 8 Cycle */
	reg_val |= FIELD_PREP(SUNXI_SPI_SVCN_SVCN, (num << 3));
	writel(reg_val, sspi->base_addr + SUNXI_SPI_SVCN_REG);
}

static void sunxi_spi_camera_set_frame_head_number(struct sunxi_spi *sspi, u32 num)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_SFHN_REG);
	reg_val &= ~SUNXI_SPI_SFHN;
	reg_val |= FIELD_PREP(SUNXI_SPI_SFHN, num - 1);
	writel(reg_val, sspi->base_addr + SUNXI_SPI_SFHN_REG);
}

static int sunxi_spi_camera_set_frame_head(struct sunxi_spi *sspi, u8 *buf, int len)
{
	u64 reg_val = 0;
	int i;

	if (len > SUNXI_SPI_FRAMEHEAD_MAX)
		return -EINVAL;

	for (i = len; i > 0; i--)
		reg_val |= (buf[len - i] << ((i - 1) * 8));

	writel((reg_val >> 32) & 0xffffffff, sspi->base_addr + SUNXI_SPI_SFHH_REG);
	writel((reg_val >>  0) & 0xffffffff, sspi->base_addr + SUNXI_SPI_SFHL_REG);
	return 0;
}

int sunxi_spi_camera_get_frame_head(struct sunxi_spi *sspi, u8 *buf, int len)
{
	u64 reg_val = 0;
	int i;

	if (len > SUNXI_SPI_FRAMEHEAD_MAX)
		return -EINVAL;

	reg_val |= readl(sspi->base_addr + SUNXI_SPI_SFHHR_REG);
	reg_val <<= 32;
	reg_val |= readl(sspi->base_addr + SUNXI_SPI_SFHLR_REG);

	for (i = len; i > 0; i--)
		buf[len - i] = (reg_val >> ((i - 1) * 8)) & 0xff;
	return 0;
}

static void sunxi_spi_camera_set_idlewait_enable_value(struct sunxi_spi *sspi, u32 ahb_clk, u32 us)
{
	u32 ahb_time = 1000000000UL / ahb_clk;	/* ns per ahb clk cycle */
	u32 value = DIV_ROUND_UP(us * 1000, ahb_time);
	writel(value, sspi->base_addr + SUNXI_SPI_SIWVE_REG);
}

static void sunxi_spi_camera_set_idlewait_reset_value(struct sunxi_spi *sspi, u32 ahb_clk, u32 mclk, u32 cycle)
{
	u32 reg_val = readl(sspi->base_addr + SUNXI_SPI_SIWVR_REG);
	u32 ahb_time = 1000000000UL / ahb_clk;	/* ns per ahb clk cycle */
	u32 clk_time = 1000000000UL / mclk;		/* ns per spi clk cycle */
	u32 value = DIV_ROUND_UP(clk_time * cycle, ahb_time);

	reg_val &= ~SUNXI_SPI_SIWVR;
	reg_val |= FIELD_PREP(SUNXI_SPI_SIWVR, value);

	writel(reg_val, sspi->base_addr + SUNXI_SPI_SIWVR_REG);
}

/* SPI Controller Hardware Register Operation End */

int sunxi_spi_camera_set_mode(struct spi_device *spi, enum sunxi_spi_camera_mode mode)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);

	if (sspi->bus_mode != SUNXI_SPI_BUS_CAMERA) {
		sunxi_err(sspi->dev, "bus mode %#x unsupport camera feature\n", sspi->bus_mode);
		return -EINVAL;
	}

	sspi->camera_mode = mode;

	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_spi_camera_set_mode);

int sunxi_spi_camera_set_vsync(struct spi_device *spi, u32 len)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);
	int ret = 0;

	if (sspi->bus_mode == SUNXI_SPI_BUS_CAMERA && sspi->camera_mode == SUNXI_SPI_CAMERA_VSYNC) {
#ifdef CONFIG_AW_IC_BOARD
		sunxi_spi_camera_set_vsync_freq_relation(sspi, clk_get_rate(sspi->ahb_clk), clk_get_rate(sspi->mclk));
#else
		sunxi_spi_camera_set_vsync_freq_relation(sspi, 24000000, clk_get_rate(sspi->mclk));
#endif
		sunxi_spi_camera_vsync_input_select(sspi, !(spi->mode & SPI_CS_HIGH));
		sunxi_spi_camera_set_vsync_cycle_number(sspi, len);
	} else {
		sunxi_err(sspi->dev, "vsync set unsupport, bus_%#x camera_%d\n", sspi->bus_mode, sspi->camera_mode);
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_spi_camera_set_vsync);

int sunxi_spi_camera_set_framehead_flag(struct spi_device *spi, u8 *buf, int len)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);
	int ret = 0;

	if (sspi->bus_mode == SUNXI_SPI_BUS_CAMERA && sspi->camera_mode == SUNXI_SPI_CAMERA_FRAMEHEAD) {
		if (len > SUNXI_SPI_FRAMEHEAD_MAX) {
			sunxi_err(sspi->dev, "set framehead len %d overflow\n", len);
			ret = -EINVAL;
		} else {
			sspi->camera_framehead_len = len;
			sunxi_spi_camera_set_frame_head_number(sspi, len);
			sunxi_spi_camera_set_frame_head(sspi, buf, len);
		}
	} else {
		sunxi_err(sspi->dev, "framehead set unsupport, bus_%#x camera_%d\n", sspi->bus_mode, sspi->camera_mode);
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_spi_camera_set_framehead_flag);

int sunxi_spi_camera_get_framehead_flag(struct spi_device *spi, u8 *buf, int len)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);
	int ret = 0;

	if (sspi->bus_mode == SUNXI_SPI_BUS_CAMERA && sspi->camera_mode == SUNXI_SPI_CAMERA_FRAMEHEAD) {
		if (len > SUNXI_SPI_FRAMEHEAD_MAX || len != sspi->camera_framehead_len) {
			sunxi_err(sspi->dev, "get framehead len %d not correct\n", len);
			ret = -EINVAL;
		} else {
			memcpy(buf, sspi->camera_framehead, min(len, sspi->camera_framehead_len));
		}
	} else {
		sunxi_err(sspi->dev, "framehead get unsupport, bus_%#x camera_%d\n", sspi->bus_mode, sspi->camera_mode);
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_spi_camera_get_framehead_flag);

int sunxi_spi_camera_set_idlewait_us(struct spi_device *spi, u32 us)
{
	struct sunxi_spi *sspi = spi_controller_get_devdata(spi->controller);
	int ret = 0;

	if (sspi->bus_mode == SUNXI_SPI_BUS_CAMERA && sspi->camera_mode == SUNXI_SPI_CAMERA_IDLEWAIT) {
#if IS_ENABLED(CONFIG_AW_IC_BOARD)
		sunxi_spi_camera_set_idlewait_enable_value(sspi, clk_get_rate(sspi->ahb_clk), us);
		sunxi_spi_camera_set_idlewait_reset_value(sspi, clk_get_rate(sspi->ahb_clk), clk_get_rate(sspi->mclk), 5);
#else
		sunxi_spi_camera_set_idlewait_enable_value(sspi, 24000000, us);
		sunxi_spi_camera_set_idlewait_reset_value(sspi, 24000000, clk_get_rate(sspi->mclk), 5);
#endif
	} else {
		sunxi_err(sspi->dev, "idlewait set unsupport, bus_%#x camera_%d\n", sspi->bus_mode, sspi->camera_mode);
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sunxi_spi_camera_set_idlewait_us);
