/*
 * drivers/spi/spi-sunxi.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUNXI SPI Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2013.5.7 Mintow <duanmintao@allwinnertech.com>
 *    Adapt to support sun8i/sun9i of Allwinner.
 *
 * 2021-3-2 liuyu <liuyu@allwinnertech.com>
 *	1 : use the new kernel framework to transfer
 *	2 : support soft cs gpio
 *	3 : delet unused dts node : cs_bitmap
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/pinctrl/consumer.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi_bitbang.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/signal.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/regulator/consumer.h>
#include "spi-sunxi.h"
#include "spi-slave-protocol.h"

/* For debug */
#define SPI_ERR(fmt, arg...)	pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)

static u32 debug_mask = 1;
#define dprintk(level_mask, fmt, arg...)				\
do {									\
	if (unlikely(debug_mask & level_mask))				\
		pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg);	\
} while (0)

#define SUNXI_SPI_OK   0
#define SUNXI_SPI_FAIL -1

#define XFER_TIMEOUT	5000

enum spi_mode_type {
	SINGLE_HALF_DUPLEX_RX,		/* single mode, half duplex read */
	SINGLE_HALF_DUPLEX_TX,		/* single mode, half duplex write */
	SINGLE_FULL_DUPLEX_RX_TX,	/* single mode, full duplex read and write */
	DUAL_HALF_DUPLEX_RX,		/* dual mode, half duplex read */
	DUAL_HALF_DUPLEX_TX,		/* dual mode, half duplex write */
	QUAD_HALF_DUPLEX_RX,		/* quad mode, half duplex read */
	QUAD_HALF_DUPLEX_TX,		/* quad mode, half duplex write */
	MODE_TYPE_NULL,
};

#if IS_ENABLED(CONFIG_DMA_ENGINE)

#define SPI_MAX_PAGES	100
enum spi_dma_dir {
	SPI_DMA_RWNULL,
	SPI_DMA_WDEV = DMA_TO_DEVICE,
	SPI_DMA_RDEV = DMA_FROM_DEVICE,
};

typedef struct {
	enum spi_dma_dir dir;
	struct dma_chan *chan;
	int nents;
	struct scatterlist sg[SPI_MAX_PAGES];
	struct page *pages[SPI_MAX_PAGES];
} spi_dma_info_t;

u64 sunxi_spi_dma_mask = DMA_BIT_MASK(32);

#endif

struct sunxi_spi {
	#define SPI_FREE   (1<<0)
	#define SPI_SUSPND (1<<1)
	#define SPI_BUSY   (1<<2)

#if IS_ENABLED(CONFIG_DMA_ENGINE)
	spi_dma_info_t dma_rx;
	spi_dma_info_t dma_tx;
#endif

	struct platform_device *pdev;
	struct spi_master *master;/* kzalloc */
	struct spi_device *spi;
	struct spi_dbi_config *dbi_config;
	struct sunxi_slave *slave;
	struct pinctrl		 *pctrl;

	struct clk *pclk;  /* PLL clock */
	struct clk *mclk;  /* spi module clock */
	struct clk *bus_clk; /*spi bus clock*/
	struct reset_control *reset; /*reset clock*/

	struct task_struct *task;

	struct completion done;  /* wakup another spi transfer */
	spinlock_t lock;

	char dev_name[48];
	enum spi_mode_type mode_type;
	void __iomem *base_addr; /* register */

	u32 base_addr_phy;
	u32 mode; /* 0: master mode, 1: slave mode */
	u32 irq; /* irq NO. */
	int busy;
	int result; /* 0: succeed -1:fail */
	int task_flag;
	int dbi_enabled;
};

void __spi_get_dbi_config(struct spi_device *spi, struct spi_dbi_config *dbi_config)
{
	struct sunxi_spi *sspi = spi->master->dev.driver_data;
	sspi->dbi_config = kzalloc(sizeof(struct spi_dbi_config), GFP_KERNEL);
	sspi->dbi_config = dbi_config;
}
EXPORT_SYMBOL_GPL(__spi_get_dbi_config);

void spi_dump_reg(struct sunxi_spi *sspi, u32 offset, u32 len)
{
	u32 i;
	u8 buf[64], cnt = 0;

	for (i = 0; i < len; i = i + REG_INTERVAL) {
		if (i%HEXADECIMAL == 0)
			cnt += sprintf(buf + cnt, "0x%08x: ",
					(u32)(sspi->base_addr_phy  + offset + i));

		cnt += sprintf(buf + cnt, "%08x ",
				readl(sspi->base_addr + offset + i));

		if (i%HEXADECIMAL == REG_CL) {
			pr_warn("%s\n", buf);
			cnt = 0;
		}
	}
}

void spi_dump_data(u8 *buf, u32 len)
{
	u32 i, cnt = 0;
	u8 *tmp;

	tmp = kzalloc(len, GFP_KERNEL);
	if (!tmp)
		return;

	for (i = 0; i < len; i++) {
		if (i%HEXADECIMAL == 0)
			cnt += sprintf(tmp + cnt, "0x%08x: ", i);

		cnt += sprintf(tmp + cnt, "%02x ", buf[i]);

		if ((i%HEXADECIMAL == REG_END) || (i == (len - 1))) {
			pr_warn("%s\n", tmp);
			cnt = 0;
		}
	}

	kfree(tmp);
}

static void dbi_disable_irq(u32 bitmap, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_DBI_INT_REG);

	bitmap &= DBI_INT_STA_MASK;
	reg_val &= ~bitmap;
	writel(reg_val, base_addr + SPI_DBI_INT_REG);
}

static void dbi_enable_irq(u32 bitmap, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_DBI_INT_REG);


	bitmap &= DBI_INT_STA_MASK;
	reg_val |= bitmap;
	writel(reg_val, base_addr + SPI_DBI_INT_REG);
}

static s32 set_dbi_timer_param(struct sunxi_spi *sspi, struct spi_device *spi)
{
	u32 timer_val = 0, pixel_cycle = 0;
	s32 ret = -1;
	void __iomem *base_addr = sspi->base_addr;

	if (!sspi || !base_addr)
		goto OUT;

	goto OUT; /*not use */

	if (sspi->dbi_config->dbi_te_en || !sspi->dbi_config->dbi_fps) {
		writel(0x0, base_addr + SPI_DBI_TIMER_REG);
		goto OUT;
	}

	if (sspi->dbi_config->dbi_interface == D2LI) {
		switch (sspi->dbi_config->dbi_format) {
		case DBI_RGB111:
			pixel_cycle = 8;
			break;
		case DBI_RGB444:
		case DBI_RGB565:
			pixel_cycle = 9;
			break;
		case DBI_RGB666:
			pixel_cycle = 10;
			break;
		case DBI_RGB888:
			pixel_cycle = 13;
			break;
		default:
			break;
		}
	} else {
		switch (sspi->dbi_config->dbi_format) {
		case DBI_RGB111:
			pixel_cycle = 8;
			break;
		case DBI_RGB444:
			pixel_cycle = 12;
			break;
		case DBI_RGB565:
			pixel_cycle = 16;
			break;
		case DBI_RGB666:
		case DBI_RGB888:
			pixel_cycle = 24;
			break;
		default:
			break;
		}
	}
	timer_val = spi->max_speed_hz / sspi->dbi_config->dbi_fps -
		    pixel_cycle * sspi->dbi_config->dbi_video_h * sspi->dbi_config->dbi_video_v;

	timer_val |= 0x80000000;
	writel(timer_val, base_addr + SPI_DBI_TIMER_REG);
	ret = 0;

OUT:
	return ret;
}

/* config dbi */
static void spi_config_dbi(struct sunxi_spi *sspi, struct spi_device *spi)
{
	u32 reg_val = 0;
	u32 reg_tmp = 0;
	u32 config = sspi->dbi_config->dbi_mode;
	void __iomem *base_addr = sspi->base_addr;

	/*1. command type */
	if (config & SPI_DBI_COMMAND_READ_) {
		reg_val |= DBI_CR_READ;
		reg_tmp = readl(base_addr + SPI_DBI_CR_REG1);
		writel(reg_tmp | sspi->dbi_config->dbi_read_bytes,
			base_addr + SPI_DBI_CR_REG1);
	} else
		reg_val &= ~DBI_CR_READ;

	/*3. output data sequence */
	if (config & SPI_DBI_LSB_FIRST_)
		reg_val |= DBI_CR_LSB_FIRST;
	else
		reg_val &= ~DBI_CR_LSB_FIRST;

	/*4. transmit data type */
	if (config & SPI_DBI_TRANSMIT_VIDEO_) {
		reg_val |= DBI_CR_TRANSMIT_MODE;
		writel((sspi->dbi_config->dbi_video_v << 16)|(sspi->dbi_config->dbi_video_h),
			base_addr + SPI_DBI_VIDEO_SIZE);
		if (sspi->dbi_config->dbi_te_en)
			dbi_enable_irq(DBI_TE_INT_EN, base_addr);
		else
			dbi_enable_irq(DBI_FRAM_DONE_INT_EN, base_addr);
	} else {
		reg_val &= ~DBI_CR_TRANSMIT_MODE;

		writel(0x0, base_addr + SPI_DBI_VIDEO_SIZE);
		dbi_disable_irq(DBI_FRAM_DONE_INT_EN | DBI_TE_INT_EN, base_addr);
		dbi_enable_irq(DBI_FIFO_EMPTY_INT_EN, base_addr);
	}


	/*5. output data format */
	reg_val &= ~(DBI_CR_FORMAT_MASK);
	if (sspi->dbi_config->dbi_format == DBI_RGB111)
		reg_val &= ~(0x7 << DBI_CR_FORMAT);
	else
		reg_val |= ((sspi->dbi_config->dbi_format) << DBI_CR_FORMAT);

	/*6. dbi interface select */
	reg_val &= ~(DBI_CR_INTERFACE_MASK);

	if (sspi->dbi_config->dbi_interface == L3I1)
		reg_val &= ~((0x7) << DBI_CR_INTERFACE);
	else
		reg_val |= ((sspi->dbi_config->dbi_interface) << DBI_CR_INTERFACE);

	if (sspi->dbi_config->dbi_format <= DBI_RGB565)
		reg_val |= 0x1;
	else
		reg_val &= ~0x1;

	if (sspi->dbi_config->dbi_out_sequence == DBI_OUT_RGB)
		reg_val &= ~((0x7) << 16);
	else
		reg_val |= ((sspi->dbi_config->dbi_out_sequence) << 16);

	if (sspi->dbi_config->dbi_src_sequence == DBI_SRC_RGB)
		reg_val &= ~((0xf) << 4);
	else
		reg_val |= ((sspi->dbi_config->dbi_src_sequence) << 4);

	if (sspi->dbi_config->dbi_rgb_bit_order == 1)
		reg_val |= ((0x1) << 2);
	else
		reg_val &= ~((0x1) << 2);

	if (sspi->dbi_config->dbi_rgb32_alpha_pos == 1)
		reg_val |= ((0x1) << 1);
	else
		reg_val &= ~((0x1) << 1);

	writel(reg_val, base_addr + SPI_DBI_CR_REG);

	reg_val = 0;

	if (sspi->dbi_config->dbi_interface == D2LI) {
		reg_val |= DBI_CR2_DCX_PIN;
		reg_val &= ~DBI_CR2_SDI_PIN;
	} else {
		reg_val |= DBI_CR2_SDI_PIN;
		reg_val &= ~DBI_CR2_DCX_PIN;
	}

	if ((sspi->dbi_config->dbi_te_en == DBI_TE_DISABLE) ||
	    !(config & SPI_DBI_TRANSMIT_VIDEO_)) {
		reg_val &= ~(0x3 << 0); // te disable
	} else {
		/*te enable*/
		reg_val |= 0x1;
		if (sspi->dbi_config->dbi_te_en == DBI_TE_FALLING_EDGE)
			reg_val |= (0x1 << 1);
		else
			reg_val &= ~(0x1 << 1);
	}

	writel(reg_val, base_addr + SPI_DBI_CR_REG2);


	dprintk(DEBUG_INFO, "DBI mode configurate : %x\n", reg_val);

	reg_val = 0;
	if (config & SPI_DBI_DCX_DATA_)
		reg_val |= DBI_CR1_DCX_DATA;
	else
		reg_val &= ~DBI_CR1_DCX_DATA;

	if (sspi->dbi_config->dbi_rgb16_pixel_endian == 1)
		reg_val |= ((0x1) << 21);
	else
		reg_val &= ~((0x1) << 21);

	/* dbi en mode sel */
	if ((sspi->dbi_config->dbi_te_en == DBI_TE_DISABLE) ||
	    !(config & SPI_DBI_TRANSMIT_VIDEO_)) {
		if (!set_dbi_timer_param(sspi, spi))
			reg_val |= (0x2 << 29); // timer trigger mode
		else
			reg_val &= ~(0x3 << 29); // always on mode
	} else {
		/*te trigger mode */
		reg_val |= ((0x3) << 29);
	}

	/* config dbi clock mode: auto gating */
	if (sspi->dbi_config->dbi_clk_out_mode == SPI_DBI_CLK_ALWAYS_ON)
		reg_val &= ~(DBI_CR1_CLK_AUTO);
	else
		reg_val |= DBI_CR1_CLK_AUTO;

	writel(reg_val, base_addr + SPI_DBI_CR_REG1);

	if ((debug_mask & DEBUG_INIT) && (debug_mask & DEBUG_DATA)) {
		dprintk(DEBUG_DATA, "[spi%d] dbi register dump reg:\n", sspi->master->bus_num);
		spi_dump_reg(sspi, 0x100, 0x30);
	}
}

/* enable spi dbi */
static void spi_enable_dbi(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val |= SPI_GC_DBI_EN;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* set dbi mode */
static void spi_set_dbi(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val |= SPI_GC_DBI_MODE_SEL;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* spi controller config chip select
 * only spi controller cs mode can use this function
 * */
static s32 sunxi_spi_ss_select(u32 chipselect, void __iomem *base_addr)
{
	char ret;
	u32 reg_val = readl(base_addr + SPI_TC_REG);

	if (chipselect < 4) {
		reg_val &= ~SPI_TC_SS_MASK;/* SS-chip select, clear two bits */
		reg_val |= chipselect << SPI_TC_SS_BIT_POS;/* set chip select */
		writel(reg_val, base_addr + SPI_TC_REG);
		ret = SUNXI_SPI_OK;
	} else {
		SPI_ERR("Chip Select set fail! cs = %d\n", chipselect);
		ret = SUNXI_SPI_FAIL;
	}

	return ret;
}

/* config spi */
static void spi_config_tc(u32 master, u32 config, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_TC_REG);

	/*1. POL */
	if (config & SPI_POL_ACTIVE_)
		reg_val |= SPI_TC_POL;/*default POL = 1 */
	else
		reg_val &= ~SPI_TC_POL;

	/*2. PHA */
	if (config & SPI_PHA_ACTIVE_)
		reg_val |= SPI_TC_PHA;/*default PHA = 1 */
	else
		reg_val &= ~SPI_TC_PHA;

	/*3. SSPOL,chip select signal polarity */
	if (config & SPI_CS_HIGH_ACTIVE_)
		reg_val &= ~SPI_TC_SPOL;
	else
		reg_val |= SPI_TC_SPOL; /*default SSPOL = 1,Low level effect */

	/*4. LMTF--LSB/MSB transfer first select */
	if (config & SPI_LSB_FIRST_ACTIVE_)
		reg_val |= SPI_TC_FBS;
	else
		reg_val &= ~SPI_TC_FBS;/*default LMTF =0, MSB first */

	/*master mode: set DDB,DHB,SMC,SSCTL*/
	if (master == 1) {
		/*5. dummy burst type */
		if (config & SPI_DUMMY_ONE_ACTIVE_)
			reg_val |= SPI_TC_DDB;
		else
			reg_val &= ~SPI_TC_DDB;/*default DDB =0, ZERO */

		/*6.discard hash burst-DHB */
		if (config & SPI_RECEIVE_ALL_ACTIVE_)
			reg_val &= ~SPI_TC_DHB;
		else
			reg_val |= SPI_TC_DHB;/*default DHB =1, discard unused burst */

		/*7. set SMC = 1 , SSCTL = 0 ,TPE = 1 */
		reg_val &= ~SPI_TC_SSCTL;
	} else {
		/* tips for slave mode config */
		dprintk(DEBUG_INFO, "slave mode configurate control register\n");
	}

	writel(reg_val, base_addr + SPI_TC_REG);
}

/* set spi clock */
static void spi_set_clk(u32 spi_clk, u32 ahb_clk, struct sunxi_spi *sspi)
{
	dprintk(DEBUG_INFO, "set spi clock %d, mclk %d\n", spi_clk, ahb_clk);

	clk_set_rate(sspi->mclk, spi_clk);
	if (clk_get_rate(sspi->mclk) != spi_clk) {
		clk_set_rate(sspi->mclk, ahb_clk);
		SPI_ERR("[spi%d] set spi clock failed, use clk:%d\n",
				sspi->master->bus_num, ahb_clk);
	}
}

/* delay internal read sample point*/
static void spi_sample_delay(u32 sdm, u32 sdc, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_TC_REG);
	u32 org_val = reg_val;

	if (sdm)
		reg_val |= SPI_TC_SDM;
	else
		reg_val &= ~SPI_TC_SDM;

	if (sdc)
		reg_val |= SPI_TC_SDC;
	else
		reg_val &= ~SPI_TC_SDC;

	if (reg_val != org_val)
		writel(reg_val, base_addr + SPI_TC_REG);
}

/* start spi transfer */
static void spi_start_xfer(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_TC_REG);

	reg_val |= SPI_TC_XCH;
	writel(reg_val, base_addr + SPI_TC_REG);
}

/* enable spi bus */
static void spi_enable_bus(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val |= SPI_GC_EN;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* disbale spi bus */
static void spi_disable_bus(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val &= ~SPI_GC_EN;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* set master mode */
static void spi_set_master(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val |= SPI_GC_MODE;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* set slaev mode */
static void spi_set_slave(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);
	u32 val = SPI_GC_MODE;

	reg_val &= ~val;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* enable transmit pause */
static void spi_enable_tp(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val |= SPI_GC_TP_EN;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* soft reset spi controller */
static void spi_soft_reset(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_GC_REG);

	reg_val |= SPI_GC_SRST;
	writel(reg_val, base_addr + SPI_GC_REG);
}

/* enable irq type */
static void spi_enable_irq(u32 bitmap, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_INT_CTL_REG);

	bitmap &= SPI_INTEN_MASK;
	reg_val |= bitmap;
	writel(reg_val, base_addr + SPI_INT_CTL_REG);
}

/* disable irq type */
static void spi_disable_irq(u32 bitmap, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_INT_CTL_REG);

	bitmap &= SPI_INTEN_MASK;
	reg_val &= ~bitmap;
	writel(reg_val, base_addr + SPI_INT_CTL_REG);
}

#if IS_ENABLED(CONFIG_DMA_ENGINE)
/* enable dma irq */
static void spi_enable_dma_irq(u32 bitmap, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);

	bitmap &= SPI_FIFO_CTL_DRQEN_MASK;
	reg_val |= bitmap;
	writel(reg_val, base_addr + SPI_FIFO_CTL_REG);

	spi_set_dma_mode(base_addr);
}

/* disable dma irq */
static void spi_disable_dma_irq(u32 bitmap, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);

	bitmap &= SPI_FIFO_CTL_DRQEN_MASK;
	reg_val &= ~bitmap;
	writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
}

static void spi_enable_dbi_dma(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_DBI_CR_REG2);

	reg_val |= DBI_CR2_DMA_ENABLE;
	writel(reg_val, base_addr + SPI_DBI_CR_REG2);
}

static void spi_disable_dbi_dma(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_DBI_CR_REG2);

	reg_val &= ~(DBI_CR2_DMA_ENABLE);
	writel(reg_val, base_addr + SPI_DBI_CR_REG2);
}
#endif

/* query irq enable */
static u32 spi_qry_irq_enable(void __iomem *base_addr)
{
	return (SPI_INTEN_MASK & readl(base_addr + SPI_INT_CTL_REG));
}

/* query dbi irq pending */
static u32 dbi_qry_irq_pending(void __iomem *base_addr)
{
	return (DBI_INT_STA_MASK & readl(base_addr + SPI_DBI_INT_REG));
}

/* clear irq pending */
static void dbi_clr_irq_pending(u32 pending_bit, void __iomem *base_addr)
{
	pending_bit &= DBI_INT_STA_MASK;
	writel(pending_bit, base_addr + SPI_DBI_INT_REG);
}

/* query irq pending */
static u32 spi_qry_irq_pending(void __iomem *base_addr)
{
	return (SPI_INT_STA_MASK & readl(base_addr + SPI_INT_STA_REG));
}

/* clear irq pending */
static void spi_clr_irq_pending(u32 pending_bit, void __iomem *base_addr)
{
	pending_bit &= SPI_INT_STA_MASK;
	writel(pending_bit, base_addr + SPI_INT_STA_REG);
}

/* query txfifo bytes */
static u32 spi_query_txfifo(void __iomem *base_addr)
{
	u32 reg_val = (SPI_FIFO_STA_TX_CNT & readl(base_addr + SPI_FIFO_STA_REG));

	reg_val >>= SPI_TXCNT_BIT_POS;
	return reg_val;
}

/* query rxfifo bytes */
static u32 spi_query_rxfifo(void __iomem *base_addr)
{
	u32 reg_val = (SPI_FIFO_STA_RX_CNT & readl(base_addr + SPI_FIFO_STA_REG));

	reg_val >>= SPI_RXCNT_BIT_POS;
	return reg_val;
}

/* reset fifo */
static void spi_reset_fifo(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);

	reg_val |= (SPI_FIFO_CTL_RX_RST|SPI_FIFO_CTL_TX_RST);
	/* Set the trigger level of RxFIFO/TxFIFO. */
	reg_val &= ~(SPI_FIFO_CTL_RX_LEVEL|SPI_FIFO_CTL_TX_LEVEL);
	reg_val |= (0x20<<16) | 0x20;
	writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
}

static void spi_set_rx_trig(u32 val, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);

	reg_val &= ~SPI_FIFO_CTL_RX_LEVEL;
	reg_val |= val & SPI_FIFO_CTL_RX_LEVEL;
	writel(reg_val, base_addr + SPI_FIFO_CTL_REG);

}

/* set transfer total length BC, transfer length TC and single transmit length STC */
static void spi_set_bc_tc_stc(u32 tx_len, u32 rx_len, u32 stc_len, u32 dummy_cnt, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPI_BURST_CNT_REG);

	/* set MBC(0x30) = tx_len + rx_len + dummy_cnt */
	reg_val &= ~SPI_BC_CNT_MASK;
	reg_val |= (SPI_BC_CNT_MASK & (tx_len + rx_len + dummy_cnt));
	writel(reg_val, base_addr + SPI_BURST_CNT_REG);

	/* set MTC(0x34) = tx_len */
	reg_val = readl(base_addr + SPI_TRANSMIT_CNT_REG);
	reg_val &= ~SPI_TC_CNT_MASK;
	reg_val |= (SPI_TC_CNT_MASK & tx_len);
	writel(reg_val, base_addr + SPI_TRANSMIT_CNT_REG);

	/* set BBC(0x38) = dummy cnt & single mode transmit counter */
	reg_val = readl(base_addr + SPI_BCC_REG);
	reg_val &= ~SPI_BCC_STC_MASK;
	reg_val |= (SPI_BCC_STC_MASK & stc_len);
	reg_val &= ~(0xf << 24);
	reg_val |= (dummy_cnt << 24);
	writel(reg_val, base_addr + SPI_BCC_REG);
}

/* sunxi_spi_ss_ctrl : software control or spi controller control
 * owner = 1 : software contorl
 * owner = 0 : spi controller control
 * */
static void sunxi_spi_ss_ctrl(void __iomem *base_addr, bool owner)
{
	u32 reg_val = readl(base_addr + SPI_TC_REG);

	owner &= 0x1;
	if (owner)
		reg_val |= SPI_TC_SS_OWNER;
	else
		reg_val &= ~SPI_TC_SS_OWNER;
	writel(reg_val, base_addr + SPI_TC_REG);
}

/* set dhb, 1: discard unused spi burst; 0: receiving all spi burst */
static void spi_set_all_burst_received(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_TC_REG);

	reg_val &= ~SPI_TC_DHB;
	writel(reg_val, base_addr + SPI_TC_REG);
}

static void spi_disable_dual(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);
	reg_val &= ~SPI_BCC_DUAL_MODE;
	writel(reg_val, base_addr + SPI_BCC_REG);
}

static void spi_enable_dual(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);
	reg_val &= ~SPI_BCC_QUAD_MODE;
	reg_val |= SPI_BCC_DUAL_MODE;
	writel(reg_val, base_addr + SPI_BCC_REG);
}

static void spi_disable_quad(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);

	reg_val &= ~SPI_BCC_QUAD_MODE;
	writel(reg_val, base_addr + SPI_BCC_REG);
}

static void spi_enable_quad(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);

	reg_val |= SPI_BCC_QUAD_MODE;
	writel(reg_val, base_addr + SPI_BCC_REG);
}
static int spi_regulator_request(struct sunxi_spi_platform_data *pdata,
			struct device *dev)
{
	struct regulator *regu = NULL;

	if (pdata->regulator != NULL)
		return 0;

	regu = devm_regulator_get(dev, "spi");
	if (IS_ERR(regu)) {
		SPI_ERR("%s: spi get supply failed!\n", __func__);
		return -1;
	}

	pdata->regulator = regu;
	return 0;
}

static int spi_regulator_enable(struct sunxi_spi_platform_data *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_enable(pdata->regulator) != 0) {
		SPI_ERR("enable regulator %s failed!\n", pdata->regulator_id);
		return -1;
	}
	return 0;
}

static int spi_regulator_disable(struct sunxi_spi_platform_data *pdata)
{
	if (pdata->regulator == NULL)
		return 0;

	if (regulator_disable(pdata->regulator) != 0) {
		SPI_ERR("enable regulator %s failed!\n", pdata->regulator_id);
		return -1;
	}
	return 0;
}

#if IS_ENABLED(CONFIG_DMA_ENGINE)

/* ------------------------------- dma operation start----------------------- */
/* dma full done callback for spi rx */
static void sunxi_spi_dma_cb_rx(void *data)
{
	struct sunxi_spi *sspi = (struct sunxi_spi *)data;
	unsigned long flags = 0;
	void __iomem *base_addr = sspi->base_addr;

	spin_lock_irqsave(&sspi->lock, flags);
	dprintk(DEBUG_INFO, "[spi%d] dma read data end\n", sspi->master->bus_num);

	if (spi_query_rxfifo(base_addr) > 0) {
		SPI_ERR("[spi%d] DMA end, but RxFIFO isn't empty! FSR: %#x\n",
			sspi->master->bus_num, spi_query_rxfifo(base_addr));
		sspi->result = -1; /* failed */
	} else {
		sspi->result = 0;
	}

	complete(&sspi->done);
	spin_unlock_irqrestore(&sspi->lock, flags);
}

/* dma full done callback for spi tx */
static void sunxi_spi_dma_cb_tx(void *data)
{
	struct sunxi_spi *sspi = (struct sunxi_spi *)data;
	unsigned long flags = 0;

	spin_lock_irqsave(&sspi->lock, flags);
	dprintk(DEBUG_INFO, "[spi%d] dma write data end\n", sspi->master->bus_num);
	spin_unlock_irqrestore(&sspi->lock, flags);
}

static int sunxi_spi_dmg_sg_cnt(void *addr, int len)
{
	int npages = 0;
	char *bufp = (char *)addr;
	int mapbytes = 0;
	int bytesleft = len;

	while (bytesleft > 0) {
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);

		npages++;
		bufp += mapbytes;
		bytesleft -= mapbytes;
	}
	return npages;
}

static int sunxi_spi_dma_init_sg(spi_dma_info_t *info, void *addr,
				 int len)
{
	int i;
	int npages = 0;
	void *bufp = addr;
	int mapbytes = 0;
	int bytesleft = len;

	npages = sunxi_spi_dmg_sg_cnt(addr, len);
	WARN_ON(npages == 0);
	dprintk(DEBUG_INFO, "npages = %d, len = %d\n", npages, len);
	if (npages > SPI_MAX_PAGES)
		npages = SPI_MAX_PAGES;

	sg_init_table(info->sg, npages);
	for (i = 0; i < npages; i++) {
		/* If there are less bytes left than what fits
		 * in the current page (plus page alignment offset)
		 * we just feed in this, else we stuff in as much
		 * as we can.
		 */
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);

		dprintk(DEBUG_INFO, "%d: len %d, offset %ld, addr %p(%d)\n", i, mapbytes,
			offset_in_page(bufp), bufp, virt_addr_valid(bufp));
		if (virt_addr_valid(bufp))
			sg_set_page(&info->sg[i], virt_to_page(bufp),
				    mapbytes, offset_in_page(bufp));
		else
			sg_set_page(&info->sg[i], vmalloc_to_page(bufp),
				    mapbytes, offset_in_page(bufp));

		bufp += mapbytes;
		bytesleft -= mapbytes;
	}

	WARN_ON(bytesleft);
	info->nents = npages;
	return 0;
}

/* request dma channel and set callback function */
static int sunxi_spi_prepare_dma(struct device *dev, spi_dma_info_t *_info,
				enum spi_dma_dir _dir, const char *name)
{
	dprintk(DEBUG_INFO, "Init DMA, dir %d\n", _dir);

	if (_info->chan == NULL) {
		_info->chan = dma_request_chan(dev, name);
		if (IS_ERR(_info->chan)) {
			SPI_ERR("Request DMA(dir %d) failed!\n", _dir);
			return -EINVAL;
		}
	}

	_info->dir = _dir;
	return 0;
}

static int sunxi_spi_config_dma_rx(struct sunxi_spi *sspi, struct spi_transfer *t)
{
	int ret = 0;
	int nents = 0;
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;
	unsigned int i, j;
	u8 buf[64], cnt = 0;

	if (debug_mask & DEBUG_INFO3) {
		dprintk(DEBUG_INIT, "t->len = %d\n", t->len);
		if (debug_mask & DEBUG_DATA) {
			for (i = 0; i < t->len; i += 16) {
				cnt = 0;
				cnt += sprintf(buf + cnt, "%03x: ", i);
				for (j = 0; ((i + j) < t->len) && (j < 16); j++)
					cnt += sprintf(buf + cnt, "%02x ",
							((unsigned char *)(t->rx_buf))[i+j]);
				pr_warn("%s\n", buf);
			}
		}
	}

	ret = sunxi_spi_dma_init_sg(&sspi->dma_rx, t->rx_buf, t->len);
	if (ret != 0)
		return ret;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = sspi->base_addr_phy + SPI_RXDATA_REG;
	if (t->len%DMA_SLAVE_BUSWIDTH_4_BYTES) {
		dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	} else {
		dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	}
	dma_conf.src_maxburst = 4;
	dma_conf.dst_maxburst = 4;
	dmaengine_slave_config(sspi->dma_rx.chan, &dma_conf);

	nents = dma_map_sg(&sspi->pdev->dev, sspi->dma_rx.sg,
			   sspi->dma_rx.nents, DMA_FROM_DEVICE);
	if (!nents) {
		SPI_ERR("[spi%d] dma_map_sg(%d) failed! return %d\n",
				sspi->master->bus_num, sspi->dma_rx.nents, nents);
		return -ENOMEM;
	}
	dprintk(DEBUG_INFO, "[spi%d] npages = %d, nents = %d\n",
			sspi->master->bus_num, sspi->dma_rx.nents, nents);

	dma_desc = dmaengine_prep_slave_sg(sspi->dma_rx.chan, sspi->dma_rx.sg,
					   nents, DMA_DEV_TO_MEM,
					   DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		SPI_ERR("[spi%d] dmaengine_prep_slave_sg() failed!\n",
				sspi->master->bus_num);
		dma_unmap_sg(&sspi->pdev->dev, sspi->dma_rx.sg,
			     sspi->dma_rx.nents, DMA_FROM_DEVICE);
		return -1;
	}

	dma_desc->callback = sunxi_spi_dma_cb_rx;
	dma_desc->callback_param = (void *)sspi;
	dmaengine_submit(dma_desc);

	return 0;
}

static int sunxi_spi_config_dma_tx(struct sunxi_spi *sspi, struct spi_transfer *t)
{
	int ret = 0;
	int nents = 0;
	struct dma_slave_config dma_conf = {0};
	struct dma_async_tx_descriptor *dma_desc = NULL;
	unsigned int i, j;
	u8 buf[64], cnt = 0;

	if (debug_mask & DEBUG_INFO4) {
		dprintk(DEBUG_INIT, "t->len = %d\n", t->len);
		if (debug_mask & DEBUG_DATA) {
			for (i = 0; i < t->len; i += 16) {
				cnt = 0;
				cnt += sprintf(buf + cnt, "%03x: ", i);
				for (j = 0; ((i + j) < t->len) && (j < 16); j++)
					cnt += sprintf(buf + cnt, "%02x ",
							((unsigned char *)(t->tx_buf))[i+j]);
				pr_warn("%s\n", buf);
			}
		}
	}

	ret = sunxi_spi_dma_init_sg(&sspi->dma_tx, (void *)t->tx_buf,
				    t->len);
	if (ret != 0)
		return ret;

	dma_conf.direction = DMA_MEM_TO_DEV;
	dma_conf.dst_addr = sspi->base_addr_phy + SPI_TXDATA_REG;
	if (t->len%DMA_SLAVE_BUSWIDTH_4_BYTES) {
		dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	} else {
		dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
		dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	}
	dma_conf.src_maxburst = 4;
	dma_conf.dst_maxburst = 4;
	dmaengine_slave_config(sspi->dma_tx.chan, &dma_conf);

	nents = dma_map_sg(&sspi->pdev->dev, sspi->dma_tx.sg, sspi->dma_tx.nents, DMA_TO_DEVICE);
	if (!nents) {
		SPI_ERR("[spi%d] dma_map_sg(%d) failed! return %d\n",
				sspi->master->bus_num, sspi->dma_tx.nents, nents);
		return -ENOMEM;
	}
	dprintk(DEBUG_INFO, "[spi%d] npages = %d, nents = %d\n",
			sspi->master->bus_num, sspi->dma_tx.nents, nents);

	dma_desc = dmaengine_prep_slave_sg(sspi->dma_tx.chan, sspi->dma_tx.sg,
					   nents, DMA_MEM_TO_DEV,
					   DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		SPI_ERR("[spi%d] dmaengine_prep_slave_sg() failed!\n",
				sspi->master->bus_num);
		dma_unmap_sg(&sspi->pdev->dev, sspi->dma_tx.sg,
			     sspi->dma_tx.nents, DMA_TO_DEVICE);
		return -1;
	}

	dma_desc->callback = sunxi_spi_dma_cb_tx;
	dma_desc->callback_param = (void *)sspi;
	dmaengine_submit(dma_desc);
	return 0;
}

/* config dma src and dst address,
 * io or linear address,
 * drq type,
 * then enqueue
 * but not trigger dma start
 */
static int sunxi_spi_config_dma(struct sunxi_spi *sspi, enum spi_dma_dir dma_dir, struct spi_transfer *t)
{
	if (dma_dir == SPI_DMA_RDEV)
		return sunxi_spi_config_dma_rx(sspi, t);
	else
		return sunxi_spi_config_dma_tx(sspi, t);
}

/* set dma start flag, if queue, it will auto restart to transfer next queue */
static int sunxi_spi_start_dma(spi_dma_info_t *_info)
{
	dma_async_issue_pending(_info->chan);
	return 0;
}

/* Unmap and free the SG tables */
static void sunxi_spi_dma_free_sg(struct sunxi_spi *sspi, spi_dma_info_t *info)
{
	if (info->dir == SPI_DMA_RWNULL)
		return;

	dma_unmap_sg(&sspi->pdev->dev, info->sg, info->nents, (enum dma_data_direction)info->dir);
	info->dir = SPI_DMA_RWNULL;

	/* Never release the DMA channel. Duanmintao
	 * dma_release_channel(info->chan);
	 * info->chan = NULL;
	 */
}

/* release dma channel, and set queue status to idle. */
static int sunxi_spi_release_dma(struct sunxi_spi *sspi, struct spi_transfer *t)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&sspi->lock, flags);

	sunxi_spi_dma_free_sg(sspi, &sspi->dma_rx);
	sunxi_spi_dma_free_sg(sspi, &sspi->dma_tx);

	spin_unlock_irqrestore(&sspi->lock, flags);
	return 0;
}
#endif

/* sunxi_spi_set_cs : spi control set cs to connect device
 * enable : 1, working mode : set ss to connect device
 * enable : 0, default mode : set ss to do not connect device
 *
 * spi controller cs mode use this funtion to set cs
 * software cs mode use kernel code to set cs
 * */
static void sunxi_spi_set_cs(struct spi_device *spi, bool enable)
{
	u32 reg_val;
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);

	sunxi_spi_ss_select(spi->chip_select, sspi->base_addr);

	reg_val = readl(sspi->base_addr + SPI_TC_REG);
	enable &= 0x01;
	if (enable)	//set cs to connect device
		reg_val |= SPI_TC_SS_LEVEL;
	else		//set cs to default mode
		reg_val &= ~SPI_TC_SS_LEVEL;
	writel(reg_val, sspi->base_addr + SPI_TC_REG);
}

/* change the properties of spi device with spi transfer.
 * every spi transfer must call this interface to update
 * the master to the excute transfer set clock.
 * return:  >= 0 : succeed;    < 0: failed.
 */
static int sunxi_spi_xfer_setup(struct spi_device *spi, struct spi_transfer *t)
{
	/* get at the setup function, the properties of spi device */
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	u32 spi_speed_hz;
	void __iomem *base_addr = sspi->base_addr;

	spi_speed_hz  = (t && t->speed_hz) ? t->speed_hz : spi->max_speed_hz;

	if (spi_speed_hz >= SPI_HIGH_FREQUENCY)
		spi_sample_delay(0, 1, base_addr);
	else if (spi_speed_hz <= SPI_LOW_FREQUENCY)
		spi_sample_delay(1, 0, base_addr);
	else
		spi_sample_delay(0, 0, base_addr);

#if IS_ENABLED(CONFIG_EVB_PLATFORM)
	spi_set_clk(spi_speed_hz, clk_get_rate(sspi->mclk), sspi);
#else
	spi_set_clk(spi_speed_hz, 24000000, sspi);
#endif

	spi_config_tc(1, spi->mode, sspi->base_addr);
	return 0;
}

static int sunxi_spi_mode_check(struct sunxi_spi *sspi, struct spi_device *spi, struct spi_transfer *t)
{
	unsigned long flags = 0;

	if (sspi->mode_type != MODE_TYPE_NULL)
		return -EINVAL;

	/* full duplex */
	spin_lock_irqsave(&sspi->lock, flags);
	if (t->tx_buf && t->rx_buf) {
		spi_set_all_burst_received(sspi->base_addr);
		spi_set_bc_tc_stc(t->len, 0, t->len, 0, sspi->base_addr);
		sspi->mode_type = SINGLE_FULL_DUPLEX_RX_TX;
		dprintk(DEBUG_INFO, "[spi%d] Single mode Full duplex tx & rx\n", sspi->master->bus_num);
	} /* half duplex transmit */
	else if (t->tx_buf) {
		if (t->tx_nbits == SPI_NBITS_QUAD) {
			spi_disable_dual(sspi->base_addr);
			spi_enable_quad(sspi->base_addr);
			spi_set_bc_tc_stc(t->len, 0, 0, 0, sspi->base_addr);
			sspi->mode_type = QUAD_HALF_DUPLEX_TX;
			dprintk(DEBUG_INFO, "[spi%d] Quad mode Half duplex tx\n", sspi->master->bus_num);
		} else if (t->tx_nbits == SPI_NBITS_DUAL) {
			spi_disable_quad(sspi->base_addr);
			spi_enable_dual(sspi->base_addr);
			spi_set_bc_tc_stc(t->len, 0, 0, 0, sspi->base_addr);
			sspi->mode_type = DUAL_HALF_DUPLEX_TX;
			dprintk(DEBUG_INFO, "[spi%d] Dual mode Half duplex tx\n", sspi->master->bus_num);
		} else {
			spi_disable_quad(sspi->base_addr);
			spi_disable_dual(sspi->base_addr);
			spi_set_bc_tc_stc(t->len, 0, t->len, 0, sspi->base_addr);
			sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
			dprintk(DEBUG_INFO, "[spi%d] Single mode Half duplex tx\n", sspi->master->bus_num);
		}
	} /* half duplex receive */
	else if (t->rx_buf) {
		if (t->rx_nbits == SPI_NBITS_QUAD) {
			spi_disable_dual(sspi->base_addr);
			spi_enable_quad(sspi->base_addr);
			spi_set_bc_tc_stc(0, t->len, 0, 0, sspi->base_addr);
			sspi->mode_type = QUAD_HALF_DUPLEX_RX;
			dprintk(DEBUG_INFO, "[spi%d] Quad mode Half duplex rx\n", sspi->master->bus_num);
		} else if (t->rx_nbits == SPI_NBITS_DUAL) {
			spi_disable_quad(sspi->base_addr);
			spi_enable_dual(sspi->base_addr);
			spi_set_bc_tc_stc(0, t->len, 0, 0, sspi->base_addr);
			sspi->mode_type = DUAL_HALF_DUPLEX_RX;
			dprintk(DEBUG_INFO, "[spi%d] Dual mode Half duplex rx\n", sspi->master->bus_num);
		} else {
			spi_disable_quad(sspi->base_addr);
			spi_disable_dual(sspi->base_addr);
			spi_set_bc_tc_stc(0, t->len, 0, 0, sspi->base_addr);
			sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
			dprintk(DEBUG_INFO, "[spi%d] Single mode Half duplex rx\n", sspi->master->bus_num);
		}
	}
	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;

}

static int sunxi_spi_cpu_readl(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	unsigned rx_len = t->len;	/* number of bytes sent */
	unsigned char *rx_buf = (unsigned char *)t->rx_buf;
	unsigned int poll_time = 0x7ffffff;
	unsigned int i, j;
	u8 buf[64], cnt = 0;

	while (rx_len) {
	/* rxFIFO counter */
		if (spi_query_rxfifo(base_addr) && (--poll_time > 0)) {
			*rx_buf++ =  readb(base_addr + SPI_RXDATA_REG);
			--rx_len;
		}
	}
	if (poll_time <= 0) {
		SPI_ERR("[spi%d] cpu receive data time out!\n", sspi->master->bus_num);
		return -1;
	}

	if (debug_mask & DEBUG_INFO1) {
		dprintk(DEBUG_INIT, "t->len = %d\n", t->len);
		if (debug_mask & DEBUG_DATA) {
			for (i = 0; i < t->len; i += 16) {
				cnt = 0;
				cnt += sprintf(buf + cnt, "%03x: ", i);
				for (j = 0; ((i + j) < t->len) && (j < 16); j++)
					cnt += sprintf(buf + cnt, "%02x ",
							((unsigned char *)(t->rx_buf))[i+j]);
				pr_warn("%s\n", buf);
			}
		}
	}

	return 0;
}

static int sunxi_spi_cpu_writel(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	unsigned long flags = 0;
#if IS_ENABLED(CONFIG_DMA_ENGINE)
	unsigned char time;
#endif
	unsigned tx_len = t->len;	/* number of bytes receieved */
	unsigned char *tx_buf = (unsigned char *)t->tx_buf;
	unsigned int poll_time = 0x7ffffff;
	unsigned int i, j;
	u8 buf[64], cnt = 0;

	if (debug_mask & DEBUG_INFO2) {
		dprintk(DEBUG_INIT, "t->len = %d\n", t->len);
		if (debug_mask & DEBUG_DATA) {
			for (i = 0; i < t->len; i += 16) {
				cnt = 0;
				cnt += sprintf(buf + cnt, "%03x: ", i);
				for (j = 0; ((i + j) < t->len) && (j < 16); j++)
					cnt += sprintf(buf + cnt, "%02x ",
							((unsigned char *)(t->tx_buf))[i+j]);
				pr_warn("%s\n", buf);
			}
		}
	}

	spin_lock_irqsave(&sspi->lock, flags);
	for (; tx_len > 0; --tx_len) {
		writeb(*tx_buf++, base_addr + SPI_TXDATA_REG);
#if IS_ENABLED(CONFIG_DMA_ENGINE)
		if (spi_query_txfifo(base_addr) >= MAX_FIFU)
			for (time = 2; 0 < time; --time)
				;
#endif
	}
	spin_unlock_irqrestore(&sspi->lock, flags);
	while (spi_query_txfifo(base_addr) && (--poll_time > 0))
		;
	if (poll_time <= 0) {
		SPI_ERR("[spi%d] cpu transfer data time out!\n", sspi->master->bus_num);
		return -1;
	}

	return 0;
}

#if IS_ENABLED(CONFIG_DMA_ENGINE)
static int sunxi_spi_dma_rx_config(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	int ret = 0;

	/* rxFIFO reday dma request enable */
	spi_enable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
	ret = sunxi_spi_prepare_dma(&sspi->pdev->dev, &sspi->dma_rx,
				SPI_DMA_RDEV, "rx");
	if (ret < 0) {
		spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
		spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
		return -EINVAL;
	}
	sunxi_spi_config_dma(sspi, SPI_DMA_RDEV, t);
	sunxi_spi_start_dma(&sspi->dma_rx);

	return ret;
}

static int sunxi_spi_dma_tx_config(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	int ret = 0;

	spi_enable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
	ret = sunxi_spi_prepare_dma(&sspi->pdev->dev, &sspi->dma_tx,
				SPI_DMA_WDEV, "tx");
	if (ret < 0) {
		spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
		spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
		return -EINVAL;
	}
	sunxi_spi_config_dma(sspi, SPI_DMA_WDEV, t);
	sunxi_spi_start_dma(&sspi->dma_tx);

	return ret;
}
static int sunxi_spi_dma_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	unsigned tx_len = t->len;	/* number of bytes receieved */
	unsigned rx_len = t->len;	/* number of bytes sent */

	switch (sspi->mode_type) {
	case SINGLE_HALF_DUPLEX_RX:
	case DUAL_HALF_DUPLEX_RX:
	case QUAD_HALF_DUPLEX_RX:
	{
		/* >64 use DMA transfer, or use cpu */
		if (t->len > BULK_DATA_BOUNDARY) {
			dprintk(DEBUG_INFO, "[spi%d] rx -> by dma\n", sspi->master->bus_num);
			/* For Rx mode, the DMA end(not TC flag) is real end. */
			spi_disable_irq(SPI_INTEN_TC, base_addr);
			sunxi_spi_dma_rx_config(spi, t);
			if (!sspi->dbi_enabled)
				spi_start_xfer(base_addr);
		} else {
			dprintk(DEBUG_INFO, "[spi%d] rx -> by ahb\n", sspi->master->bus_num);
			/* SMC=1,XCH trigger the transfer */
			if (!sspi->dbi_enabled)
				spi_start_xfer(base_addr);
			sunxi_spi_cpu_readl(spi, t);
		}
		break;
	}
	case SINGLE_HALF_DUPLEX_TX:
	case DUAL_HALF_DUPLEX_TX:
	case QUAD_HALF_DUPLEX_TX:
	{
		/* >64 use DMA transfer, or use cpu */
		if (t->len > BULK_DATA_BOUNDARY) {
			dprintk(DEBUG_INFO, "[spi%d] tx -> by dma\n", sspi->master->bus_num);
			if (!sspi->dbi_enabled)
				spi_start_xfer(base_addr);
			/* txFIFO empty dma request enable */
			sunxi_spi_dma_tx_config(spi, t);
		} else {
			dprintk(DEBUG_INFO, "[spi%d] tx -> by ahb\n", sspi->master->bus_num);
			if (!sspi->dbi_enabled)
				spi_start_xfer(base_addr);
			sunxi_spi_cpu_writel(spi, t);
		}
		break;
	}
	case SINGLE_FULL_DUPLEX_RX_TX:
	{
		/* >64 use DMA transfer, or use cpu */
		if (t->len > BULK_DATA_BOUNDARY) {
			dprintk(DEBUG_INFO, "[spi%d] rx and tx -> by dma\n", sspi->master->bus_num);
			/* For Rx mode, the DMA end(not TC flag) is real end. */
			spi_disable_irq(SPI_INTEN_TC, base_addr);
			sunxi_spi_dma_rx_config(spi, t);
			if (!sspi->dbi_enabled)
				spi_start_xfer(base_addr);
			sunxi_spi_dma_tx_config(spi, t);
		} else {
			dprintk(DEBUG_INFO, "[spi%d] rx and tx -> by ahb\n", sspi->master->bus_num);
			if ((rx_len == 0) || (tx_len == 0))
				return -EINVAL;

			if (!sspi->dbi_enabled)
				spi_start_xfer(base_addr);
			sunxi_spi_cpu_writel(spi, t);
			sunxi_spi_cpu_readl(spi, t);
		}
		break;
	}
	default:
		return -1;
	}

	return 0;
}
#else
static int sunxi_spi_cpu_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	unsigned tx_len = t->len;	/* number of bytes receieved */
	unsigned rx_len = t->len;	/* number of bytes sent */

	switch (sspi->mode_type) {
	case SINGLE_HALF_DUPLEX_RX:
	case DUAL_HALF_DUPLEX_RX:
	case QUAD_HALF_DUPLEX_RX:
	{
		dprintk(DEBUG_INFO, "[spi%d] rx -> by ahb\n", sspi->master->bus_num);
		/* SMC=1,XCH trigger the transfer */
		if (!sspi->dbi_enabled)
			spi_start_xfer(base_addr);
		sunxi_spi_cpu_readl(spi, t);
		break;
	}
	case SINGLE_HALF_DUPLEX_TX:
	case DUAL_HALF_DUPLEX_TX:
	case QUAD_HALF_DUPLEX_TX:
	{
		dprintk(DEBUG_INFO, "[spi%d] tx -> by ahb\n", sspi->master->bus_num);
		if (!sspi->dbi_enabled)
			spi_start_xfer(base_addr);
		sunxi_spi_cpu_writel(spi, t);
		break;
	}
	case SINGLE_FULL_DUPLEX_RX_TX:
	{
		dprintk(DEBUG_INFO, "[spi%d] rx and tx -> by ahb\n", sspi->master->bus_num);
		if ((rx_len == 0) || (tx_len == 0))
			return -EINVAL;

		if (!sspi->dbi_enabled)
			spi_start_xfer(base_addr);
		sunxi_spi_cpu_writel(spi, t);
		sunxi_spi_cpu_readl(spi, t);
		break;
	}
	default:
		return -1;
	}

	return 0;
}
#endif

/*
 * <= 64 : cpu mode transt
 * > 64  : dma mode transt
 * wait for done completion in this function, wakup in the irq hanlder
 * transt one message->transfer to slave devices
 */
static int sunxi_spi_transfer_one(struct spi_controller *master,
					struct spi_device *spi,
					struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem *base_addr = sspi->base_addr;
	unsigned char *tx_buf = (unsigned char *)t->tx_buf;
	unsigned char *rx_buf = (unsigned char *)t->rx_buf;
	unsigned long timeout = 0;
	int ret = 0;

	dprintk(DEBUG_INFO, "[spi%d] begin transfer, txbuf %p, rxbuf %p, len %d\n",
		spi->master->bus_num, tx_buf, rx_buf, t->len);
	if ((!t->tx_buf && !t->rx_buf) || !t->len)
		return -EINVAL;

	if (sunxi_spi_xfer_setup(spi, t) < 0)
		return -EINVAL;

	/* write 1 to clear 0 */
	spi_clr_irq_pending(SPI_INT_STA_MASK, base_addr);
	/* disable all DRQ */
	spi_disable_dma_irq(SPI_FIFO_CTL_DRQEN_MASK, base_addr);
	/* reset tx/rx fifo */
	//spi_reset_fifo(base_addr);

	if (sunxi_spi_mode_check(sspi, spi, t))
		return -EINVAL;

	if (sspi->dbi_enabled) {
		spi_config_dbi(sspi, spi);
		spi_enable_dbi_dma(base_addr);
	} else {
		/* reset tx/rx fifo */
		spi_reset_fifo(base_addr);
		/*	1. Tx/Rx error irq,process in IRQ;
			2. Transfer Complete Interrupt Enable
		*/
		spi_enable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
	}

	if ((debug_mask & DEBUG_INIT) && (debug_mask & DEBUG_DATA)) {
		dprintk(DEBUG_DATA, "[spi%d] dump reg:\n", sspi->master->bus_num);
		spi_dump_reg(sspi, 0, 0x40);
	}

#if IS_ENABLED(CONFIG_DMA_ENGINE)
	sunxi_spi_dma_transfer(spi, t);
#else
	sunxi_spi_cpu_transfer(spi, t);
#endif

	if ((debug_mask & DEBUG_INIT) && (debug_mask & DEBUG_DATA)) {
		dprintk(DEBUG_DATA, "[spi%d] dump reg:\n", sspi->master->bus_num);
		spi_dump_reg(sspi, 0, 0x40);
	}

	/* wait for xfer complete in the isr. */
	timeout = wait_for_completion_timeout(
				&sspi->done,
				msecs_to_jiffies(XFER_TIMEOUT));
	if (timeout == 0) {
		SPI_ERR("[spi%d] xfer timeout\n", spi->master->bus_num);
		ret = -1;
	} else if (sspi->result < 0) {
		SPI_ERR("[spi%d] xfer failed...\n", spi->master->bus_num);
		ret = -1;
	}

#if IS_ENABLED(CONFIG_DMA_ENGINE)
	/* release dma resource if necessary */
	sunxi_spi_release_dma(sspi, t);
#endif

	if (sspi->dbi_enabled)
		spi_disable_dbi_dma(base_addr);

	if (sspi->mode_type != MODE_TYPE_NULL)
		sspi->mode_type = MODE_TYPE_NULL;

	return ret;
}

/* wake up the sleep thread, and give the result code */
static irqreturn_t sunxi_spi_handler(int irq, void *dev_id)
{
	struct sunxi_spi *sspi = (struct sunxi_spi *)dev_id;
	void __iomem *base_addr = sspi->base_addr;
	unsigned int status = 0, enable = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&sspi->lock, flags);

	enable = spi_qry_irq_enable(base_addr);
	status = spi_qry_irq_pending(base_addr);
	spi_clr_irq_pending(status, base_addr);
	dprintk(DEBUG_INFO, "[spi%d] irq status = %x\n", sspi->master->bus_num, status);

	sspi->result = 0; /* assume succeed */

	if (sspi->mode) {
		if ((enable & SPI_INTEN_RX_RDY) && (status & SPI_INT_STA_RX_RDY)) {
			dprintk(DEBUG_INFO, "[spi%d] spi data is ready\n", sspi->master->bus_num);
			spi_disable_irq(SPI_INT_STA_RX_RDY, base_addr);
			wake_up_process(sspi->task);
			spin_unlock_irqrestore(&sspi->lock, flags);
			return IRQ_HANDLED;
		}
	}

	/* master mode, Transfer Complete Interrupt */
	if (status & SPI_INT_STA_TC) {
		dprintk(DEBUG_INFO, "[spi%d] SPI TC comes\n", sspi->master->bus_num);
		spi_disable_irq(SPI_INT_STA_TC | SPI_INT_STA_ERR, base_addr);

		/*wakup uplayer, by the sem */
		complete(&sspi->done);
		spin_unlock_irqrestore(&sspi->lock, flags);
		return IRQ_HANDLED;
	} else if (status & SPI_INT_STA_ERR) { /* master mode:err */
		SPI_ERR("[spi%d]  SPI ERR %#x comes\n", sspi->master->bus_num, status);
		/* error process, release dma in the workqueue,should not be here */
		spi_disable_irq(SPI_INT_STA_TC | SPI_INT_STA_ERR, base_addr);
		spi_soft_reset(base_addr);
		sspi->result = -1;
		complete(&sspi->done);
		spin_unlock_irqrestore(&sspi->lock, flags);
		return IRQ_HANDLED;
	}
	if (sspi->dbi_enabled) {
		status = dbi_qry_irq_pending(base_addr);
		dbi_clr_irq_pending(status, base_addr);
		dprintk(DEBUG_INFO, "[dbi%d] irq status = %x\n",
				sspi->master->bus_num, status);
		if ((status & DBI_INT_FIFO_EMPTY) && !(sspi->dbi_config->dbi_mode
					& SPI_DBI_TRANSMIT_VIDEO_)) {
			dprintk(DEBUG_INFO, "[spi%d] DBI Fram TC comes\n",
					sspi->master->bus_num);
			dbi_disable_irq(DBI_FIFO_EMPTY_INT_EN, base_addr);
			/*wakup uplayer, by the sem */
			complete(&sspi->done);
			spin_unlock_irqrestore(&sspi->lock, flags);
			return IRQ_HANDLED;
		} else if (((status & DBI_INT_TE_INT) ||
			    (status & DBI_INT_STA_FRAME)) &&
			   (sspi->dbi_config->dbi_mode & SPI_DBI_TRANSMIT_VIDEO_)) {
			if (sspi->dbi_config->dbi_vsync_handle &&
			    (status & DBI_INT_TE_INT))
				sspi->dbi_config->dbi_vsync_handle(
				    (unsigned long)sspi->spi);
			else
				dbi_disable_irq(DBI_FRAM_DONE_INT_EN, base_addr);
			complete(&sspi->done);
			spin_unlock_irqrestore(&sspi->lock, flags);
			return IRQ_HANDLED;
		} else {
			//TODO: Adapt to other states
			spin_unlock_irqrestore(&sspi->lock, flags);
			return IRQ_HANDLED;
		}
	}
	dprintk(DEBUG_INFO, "[spi%d] SPI NONE comes\n", sspi->master->bus_num);
	spin_unlock_irqrestore(&sspi->lock, flags);
	return IRQ_NONE;
}

static int sunxi_spi_setup(struct spi_device *spi)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);

	sspi->spi = spi;

	return 0;
}

static bool sunxi_spi_can_dma(struct spi_master *master, struct spi_device *spi,
				 struct spi_transfer *xfer)
{
	return (xfer->len > BULK_DATA_BOUNDARY);
}

static struct device_data *sunxi_spi_slave_set_txdata(struct sunxi_spi_slave_head *head)
{
	u8 *buf, i;
	struct device_data *data;

	buf = kzalloc(head->len, GFP_KERNEL);
	if (IS_ERR_OR_NULL(buf)) {
		SPI_ERR("failed to alloc mem\n");
		goto err0;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(data)) {
		SPI_ERR("failed to alloc mem\n");
		goto err1;
	}

	for (i = 0; i < head->len; i++) {
		buf[i] = i + SAMPLE_NUMBER;
	}
	udelay(100);

	dprintk(DEBUG_DATA, "[debugging only] send data:\n");
	if (debug_mask & DEBUG_DATA)
		spi_dump_data(buf, head->len);

	data->tx_buf = buf;
	data->len = head->len;

	return data;

err1:
	kfree(buf);
err0:
	return NULL;
}

static int sunxi_spi_slave_cpu_tx_config(struct sunxi_spi *sspi)
{
	int ret = 0, i;
	u32 poll_time = 0x7ffffff;
	unsigned long timeout = 0;
	unsigned long flags = 0;

	dprintk(DEBUG_INFO, "[spi%d] receive pkt head ok\n", sspi->master->bus_num);
	if (sspi->slave->set_up_txdata) {
		sspi->slave->data = sspi->slave->set_up_txdata(sspi->slave->head);
		if (IS_ERR_OR_NULL(sspi->slave->data)) {
			SPI_ERR("[spi%d] null data\n", sspi->master->bus_num);
			ret = -1;
			goto err;
		}
	} else {
		SPI_ERR("[spi%d] none define set_up_txdata\n", sspi->master->bus_num);
		ret = -1;
		goto err;
	}

	sspi->done.done = 0;
	spi_clr_irq_pending(SPI_INT_STA_MASK, sspi->base_addr);
	spi_disable_irq(SPI_INTEN_RX_RDY, sspi->base_addr);
	spi_reset_fifo(sspi->base_addr);
	spi_set_bc_tc_stc(0, 0, 0, 0, sspi->base_addr);
	spi_enable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, sspi->base_addr);

	dprintk(DEBUG_INFO, "[spi%d] to be send data init ok\n", sspi->master->bus_num);
	spin_lock_irqsave(&sspi->lock, flags);
	for (i = 0; i < sspi->slave->head->len; i++) {
		while ((spi_query_txfifo(sspi->base_addr) >= MAX_FIFU) && (--poll_time))
			;
		if (poll_time == 0) {
			dprintk(DEBUG_INFO, "[spi%d]cpu send data timeout\n", sspi->master->bus_num);
			goto err;
		}

		writeb(sspi->slave->data->tx_buf[i], sspi->base_addr + SPI_TXDATA_REG);
	}
	spin_unlock_irqrestore(&sspi->lock, flags);

	dprintk(DEBUG_INFO, "[spi%d] already send data to fifo\n", sspi->master->bus_num);

	/* wait for xfer complete in the isr. */
	timeout = wait_for_completion_timeout(
				&sspi->done,
				msecs_to_jiffies(XFER_TIMEOUT));
	if (timeout == 0) {
		SPI_ERR("[spi%d] xfer timeout\n", sspi->master->bus_num);
		ret = -1;
		goto err;
	} else if (sspi->result < 0) {
		SPI_ERR("[spi%d] xfer failed...\n", sspi->master->bus_num);
		ret = -1;
		goto err;
	}

err:
	spi_clr_irq_pending(SPI_INT_STA_MASK, sspi->base_addr);
	spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, sspi->base_addr);
	spi_disable_dma_irq(SPI_FIFO_CTL_DRQEN_MASK, sspi->base_addr);
	spi_reset_fifo(sspi->base_addr);
	kfree(sspi->slave->data->tx_buf);
	kfree(sspi->slave->data);

	return ret;
}

static int sunxi_spi_slave_cpu_rx_config(struct sunxi_spi *sspi)
{
	int ret = 0, i;
	u32 poll_time = 0x7ffffff;

	dprintk(DEBUG_INFO, "[spi%d] receive pkt head ok\n", sspi->master->bus_num);
	sspi->slave->data = kzalloc(sizeof(struct device_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sspi->slave->data)) {
		SPI_ERR("failed to alloc mem\n");
		ret = -ENOMEM;
		goto err0;
	}

	sspi->slave->data->len = sspi->slave->head->len;
	sspi->slave->data->rx_buf = kzalloc(sspi->slave->data->len, GFP_KERNEL);
	if (IS_ERR_OR_NULL(sspi->slave->data->rx_buf)) {
		SPI_ERR("failed to alloc mem\n");
		ret = -ENOMEM;
		goto err1;
	}

	sspi->done.done = 0;

	spi_set_rx_trig(sspi->slave->data->len/2, sspi->base_addr);
	spi_enable_irq(SPI_INTEN_ERR|SPI_INTEN_RX_RDY, sspi->base_addr);
	spi_set_bc_tc_stc(0, 0, 0, 0, sspi->base_addr);

	dprintk(DEBUG_INFO, "[spi%d] to be receive data init ok\n", sspi->master->bus_num);
	for (i = 0; i < sspi->slave->data->len; i++) {
		while (!spi_query_rxfifo(sspi->base_addr) && (--poll_time > 0))
			;
		sspi->slave->data->rx_buf[i] =  readb(sspi->base_addr + SPI_RXDATA_REG);
	}


	if (poll_time <= 0) {
		SPI_ERR("[spi%d] cpu receive pkt head time out!\n", sspi->master->bus_num);
		spi_reset_fifo(sspi->base_addr);
		ret = -1;
		goto err2;
	} else if (sspi->result < 0) {
		SPI_ERR("[spi%d] xfer failed...\n", sspi->master->bus_num);
		spi_reset_fifo(sspi->base_addr);
		ret = -1;
		goto err2;
	}

	dprintk(DEBUG_DATA, "[debugging only] receive data:\n");
	if (debug_mask & DEBUG_DATA)
		spi_dump_data(sspi->slave->data->rx_buf, sspi->slave->data->len);

err2:
	spi_clr_irq_pending(SPI_INT_STA_MASK, sspi->base_addr);
	spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, sspi->base_addr);
	spi_disable_dma_irq(SPI_FIFO_CTL_DRQEN_MASK, sspi->base_addr);
	spi_reset_fifo(sspi->base_addr);
	kfree(sspi->slave->data->rx_buf);
err1:
	kfree(sspi->slave->data);
err0:
	return ret;
}
static int sunxi_spi_slave_handle_head(struct sunxi_spi *sspi, u8 *buf)
{
	struct sunxi_spi_slave_head *head;
	int ret = 0;

	head = kzalloc(sizeof(*head), GFP_KERNEL);
	if (IS_ERR_OR_NULL(head)) {
		SPI_ERR("failed to alloc mem\n");
		ret = -ENOMEM;
		goto err0;
	}

	head->op_code = buf[OP_MASK];
	head->addr = (buf[ADDR_MASK_0] << 16) | (buf[ADDR_MASK_1] << 8) | buf[ADDR_MASK_2];
	head->len = buf[LENGTH_MASK];

	dprintk(DEBUG_INFO, "[spi%d] op=0x%x addr=0x%x len=0x%x\n",
			sspi->master->bus_num, head->op_code, head->addr, head->len);

	sspi->slave->head = head;

	if (head->len > 64) {
		dprintk(DEBUG_INFO, "[spi%d] length must less than 64 bytes\n", sspi->master->bus_num);
		ret = -1;
		goto err1;
	}

	if (head->op_code == SUNXI_OP_WRITE) {
		sunxi_spi_slave_cpu_rx_config(sspi);
	} else if (head->op_code == SUNXI_OP_READ) {
		sunxi_spi_slave_cpu_tx_config(sspi);
	} else {
		dprintk(DEBUG_INFO, "[spi%d] pkt head opcode err\n", sspi->master->bus_num);
		ret = -1;
		goto err1;
	}
err1:
	kfree(head);
err0:
	spi_clr_irq_pending(SPI_INT_STA_MASK, sspi->base_addr);
	spi_disable_irq(SPI_INTEN_RX_RDY, sspi->base_addr);
	spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, sspi->base_addr);
	spi_reset_fifo(sspi->base_addr);

	return ret;
}

static int sunxi_spi_slave_task(void *data)
{
	u8 *pkt_head, i;
	u32 poll_time = 0x7ffffff;
	unsigned long flags = 0;
	struct sunxi_spi *sspi = (struct sunxi_spi *)data;

	pkt_head = kzalloc(HEAD_LEN, GFP_KERNEL);
	if (IS_ERR_OR_NULL(pkt_head)) {
		SPI_ERR("[spi%d] failed to alloc mem\n", sspi->master->bus_num);
		return -ENOMEM;
	}

	allow_signal(SIGKILL);

	while (!kthread_should_stop()) {
		spi_reset_fifo(sspi->base_addr);
		spi_clr_irq_pending(SPI_INT_STA_MASK, sspi->base_addr);
		spi_disable_dma_irq(SPI_FIFO_CTL_DRQEN_MASK, sspi->base_addr);
		spi_enable_irq(SPI_INTEN_ERR|SPI_INTEN_RX_RDY, sspi->base_addr);
		spi_set_rx_trig(HEAD_LEN, sspi->base_addr);
		spi_set_bc_tc_stc(0, 0, 0, 0, sspi->base_addr);

		dprintk(DEBUG_INFO, "[spi%d] receive pkt head init ok, sleep and wait for data\n", sspi->master->bus_num);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		dprintk(DEBUG_INFO, "[spi%d] data is come, wake up and receive pkt head\n", sspi->master->bus_num);

		for (i = 0; i < HEAD_LEN ; i++) {
			while (!spi_query_rxfifo(sspi->base_addr) && (--poll_time > 0))
				;
			pkt_head[i] =  readb(sspi->base_addr + SPI_RXDATA_REG);
		}

		if (poll_time <= 0) {
			SPI_ERR("[spi%d] cpu receive pkt head time out!\n", sspi->master->bus_num);
			spi_reset_fifo(sspi->base_addr);
			continue;
		} else if (sspi->result < 0) {
			SPI_ERR("[spi%d] xfer failed...\n", sspi->master->bus_num);
			spi_reset_fifo(sspi->base_addr);
			continue;
		}

		sunxi_spi_slave_handle_head(sspi, pkt_head);
	}

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->task_flag = 1;
	spin_unlock_irqrestore(&sspi->lock, flags);

	kfree(pkt_head);

	return 0;
}

static int sunxi_spi_select_gpio_state(struct pinctrl *pctrl, char *name, u32 no)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		SPI_ERR("[spi%d] pinctrl_lookup_state(%s) failed! return %p\n", no, name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		SPI_ERR("[spi%d] pinctrl_select_state(%s) failed! return %d\n", no, name, ret);

	return ret;
}

static int sunxi_spi_request_gpio(struct sunxi_spi *sspi)
{
	int bus_no = sspi->pdev->id;

	sspi->pctrl = devm_pinctrl_get(&sspi->pdev->dev);
	if (IS_ERR(sspi->pctrl)) {
		SPI_ERR("[spi%d] devm_pinctrl_get() failed! return %ld\n",
				sspi->master->bus_num, PTR_ERR(sspi->pctrl));
		return -1;
	}

	return sunxi_spi_select_gpio_state(sspi->pctrl, PINCTRL_STATE_DEFAULT, bus_no);
}

/*
static void sunxi_spi_release_gpio(struct sunxi_spi *sspi)
{
	devm_pinctrl_put(sspi->pctrl);
	sspi->pctrl = NULL;
}
*/

static int sunxi_spi_resource_get(struct sunxi_spi *sspi)
{
	int ret;
	struct device_node *np = sspi->pdev->dev.of_node;
	struct sunxi_spi_platform_data *pdata = sspi->pdev->dev.platform_data;
	struct device *dev = &(sspi->pdev->dev);

	ret = spi_regulator_request(pdata, dev);
	if (ret < 0) {
		SPI_ERR("[spi%d] request regulator failed!\n", sspi->master->bus_num);
		return ret;
	}

	ret = of_property_read_u32(sspi->pdev->dev.of_node, "clock-frequency",
			&pdata->sclk_freq_def);
	if (ret) {
		SPI_ERR("[spi%d] Get clock-frequency property failed\n", sspi->master->bus_num);
		return -1;
	}

	ret = of_property_read_u32(np, "spi_slave_mode", &sspi->mode);
	if (sspi->mode)
		dprintk(DEBUG_INIT, "[spi%d] SPI SLAVE MODE\n", sspi->master->bus_num);
	else
		dprintk(DEBUG_INIT, "[spi%d] SPI MASTER MODE\n", sspi->master->bus_num);

	if (sunxi_spi_request_gpio(sspi) < 0) {
		SPI_ERR("[spi%d] Request GPIO failed!\n", sspi->master->bus_num);
		return -1;
	}
	sspi->pclk = devm_clk_get(&sspi->pdev->dev, "pll");
	if (IS_ERR_OR_NULL(sspi->pclk)) {
		SPI_ERR("[spi%d] Unable to acquire module clock '%s', return %x\n",
			sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->pclk));
		return -ENXIO;
	}

	sspi->mclk = devm_clk_get(&sspi->pdev->dev, "mod");
	if (IS_ERR_OR_NULL(sspi->mclk)) {
		SPI_ERR("[spi%d] Unable to acquire module clock '%s', return %x\n",
			sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->mclk));
		return -ENXIO;
	}

	sspi->bus_clk = devm_clk_get(&sspi->pdev->dev, "bus");
	if (IS_ERR_OR_NULL(sspi->bus_clk)) {
		SPI_ERR("[spi%d] Unable to acquire bus clock '%s', return %x\n",
			sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->bus_clk));
		return -ENXIO;
	}

	if (!sspi->reset) {
		sspi->reset = devm_reset_control_get(&sspi->pdev->dev, NULL);
		if (IS_ERR_OR_NULL(sspi->reset)) {
			SPI_ERR("[spi%d] Unable to acquire reset clock '%s', return %x\n",
				sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->reset));
			return -ENXIO;
		}
	}

	return 0;
}

static int sunxi_spi_clk_init(struct sunxi_spi *sspi, u32 mod_clk)
{
	int ret = 0;
	long rate = 0;

	/*assert and deassert constitute a complete hardware reset operation*/
	ret = reset_control_assert(sspi->reset);
	if (ret != 0) {
		SPI_ERR("[spi%d] Unable to assert reset clock '%s', return %x\n",
			sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->mclk));
		return -ENXIO;
	}
	ret = reset_control_deassert(sspi->reset);
	if (ret != 0) {
		SPI_ERR("[spi%d] Unable to deassert reset clock '%s', return %x\n",
			sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->mclk));
		return -ENXIO;
	}

	if (clk_prepare_enable(sspi->pclk)) {
		SPI_ERR("[spi%d] Couldn't enable pll clock 'spi'\n", sspi->master->bus_num);
		goto err1;
	}

	ret = clk_set_parent(sspi->mclk, sspi->pclk);
	if (ret != 0) {
		SPI_ERR("[spi%d] clk_set_parent() failed! return %d\n",
			sspi->master->bus_num, ret);
		goto err2;
	}

	rate = clk_round_rate(sspi->mclk, mod_clk);
	if (clk_set_rate(sspi->mclk, rate)) {
		SPI_ERR("[spi%d] spi clk_set_rate failed\n", sspi->master->bus_num);
		goto err2;
	}

	dprintk(DEBUG_INIT, "[spi%d] mclk %u\n", sspi->master->bus_num, (unsigned)clk_get_rate(sspi->mclk));

	if (clk_prepare_enable(sspi->mclk)) {
		SPI_ERR("[spi%d] Couldn't enable module clock 'spi'\n", sspi->master->bus_num);
		goto err2;
	}

	if (clk_prepare_enable(sspi->bus_clk)) {
		SPI_ERR("[spi%d] Couldn't enable bus clock 'spi'\n", sspi->master->bus_num);
		goto err3;
	}

	return clk_get_rate(sspi->mclk);

err3:
	clk_disable_unprepare(sspi->bus_clk);
err2:
	clk_disable_unprepare(sspi->mclk);
err1:
	clk_disable_unprepare(sspi->pclk);

	return -1;
}

static int sunxi_spi_clk_exit(struct sunxi_spi *sspi)
{
	if (IS_ERR_OR_NULL(sspi->mclk)) {
		SPI_ERR("[spi%d] SPI mclk handle is invalid!\n", sspi->master->bus_num);
		return -1;
	}

	clk_disable_unprepare(sspi->bus_clk);
	clk_disable_unprepare(sspi->mclk);
	return 0;
}

static int sunxi_spi_hw_init(struct sunxi_spi *sspi,
		struct sunxi_spi_platform_data *pdata, struct device *dev)
{
	void __iomem *base_addr = sspi->base_addr;
	u32 sclk_freq_def = 0;
	int sclk_freq = 0;
	int bus_no = sspi->pdev->id;

	spi_regulator_enable(pdata);

	sclk_freq = sunxi_spi_clk_init(sspi, pdata->sclk_freq_def);
	if (sclk_freq < 0) {
		SPI_ERR("[spi%d] sunxi_spi_clk_init(%s) failed!\n", sspi->master->bus_num, sspi->dev_name);
		return -1;
	}

	if (!sspi->dbi_enabled) {
		/* enable the spi module */
		spi_enable_bus(base_addr);
	}

	sunxi_spi_select_gpio_state(sspi->pctrl, PINCTRL_STATE_DEFAULT, bus_no);

	if (sspi->dbi_enabled) {
		spi_set_slave(base_addr);
		spi_set_dbi(base_addr);
		spi_enable_dbi(base_addr);
		spi_set_clk(10000000, sclk_freq, sspi);
		spi_enable_tp(base_addr);
	}
	if (!sspi->mode) {
		/* master: set spi module clock;
		 * set the default frequency	10MHz
		 */
		spi_set_master(base_addr);
		spi_set_clk(10000000, sclk_freq, sspi);
		/* master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0. */
		spi_config_tc(1, SPI_MODE_0, base_addr);
		spi_enable_tp(base_addr);
		/* manual control the chip select */
		sunxi_spi_ss_ctrl(base_addr, true);
	} else {
		//slave
		spi_set_slave(base_addr);
		/* master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0. */
		spi_config_tc(1, SPI_MODE_0, base_addr);
		spi_set_clk(sclk_freq_def, sclk_freq, sspi);

		if (sclk_freq_def >= SPI_HIGH_FREQUENCY)
			spi_sample_delay(0, 1, base_addr);
		else if (sclk_freq_def <= SPI_LOW_FREQUENCY)
			spi_sample_delay(1, 0, base_addr);
		else
			spi_sample_delay(0, 0, base_addr);
	}

	/* reset fifo */
	spi_reset_fifo(base_addr);

	return 0;
}

static int sunxi_spi_hw_exit(struct sunxi_spi *sspi, struct sunxi_spi_platform_data *pdata)
{
	struct spi_master *master = sspi->master;

	/* release the  gpio */
	//sunxi_spi_release_gpio(sspi);
	sunxi_spi_select_gpio_state(sspi->pctrl, PINCTRL_STATE_SLEEP, master->bus_num);

	/* disable the spi controller */
	spi_disable_bus(sspi->base_addr);

	/* disable module clock */
	sunxi_spi_clk_exit(sspi);

	/* disable regulator */
	spi_regulator_disable(pdata);

	return 0;
}

static ssize_t sunxi_spi_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct sunxi_spi_platform_data *pdata = dev->platform_data;

	return snprintf(buf, PAGE_SIZE,
		"pdev->id   = %d\n"
		"pdev->name = %s\n"
		"pdev->num_resources = %u\n"
		"pdev->resource.mem = [%pa, %pa]\n"
		"pdev->resource.irq = %pa\n"
		"pdev->dev.platform_data.cs_num    = %d\n"
		"pdev->dev.platform_data.regulator = 0x%p\n"
		"pdev->dev.platform_data.regulator_id = %s\n",
		pdev->id, pdev->name, pdev->num_resources,
		&pdev->resource[0].start, &pdev->resource[0].end,
		&pdev->resource[1].start, pdata->cs_num, pdata->regulator,
		pdata->regulator_id);
}
static struct device_attribute sunxi_spi_info_attr =
	__ATTR(info, S_IRUGO, sunxi_spi_info_show, NULL);

static ssize_t sunxi_spi_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct sunxi_spi *sspi = (struct sunxi_spi *)&master[1];
	char const *spi_mode[] = {"Single mode, half duplex read",
				  "Single mode, half duplex write",
				  "Single mode, full duplex read and write",
				  "Dual mode, half duplex read",
				  "Dual mode, half duplex write",
				  "Null"};
	char const *busy_state[] = {"Unknown", "Free", "Suspend", "Busy"};
	char const *result_str[] = {"Success", "Fail"};
#if IS_ENABLED(CONFIG_DMA_ENGINE)
	char const *dma_dir[] = {"DMA NULL", "DMA read", "DMA write"};
#endif

	if (master == NULL)
		return snprintf(buf, PAGE_SIZE, "%s\n", "spi_master is NULL!");

	return snprintf(buf, PAGE_SIZE,
			"master->bus_num = %d\n"
			"master->num_chipselect = %d\n"
			"master->dma_alignment  = %d\n"
			"master->mode_bits = %d\n"
			"master->flags = 0x%x, ->bus_lock_flag = 0x%x\n"
			"master->busy = %d, ->running = %d, ->rt = %d\n"
			"sspi->mode_type = %d [%s]\n"
			"sspi->irq = %d [%s]\n"
#if IS_ENABLED(CONFIG_DMA_ENGINE)
			"sspi->dma_tx.dir = %d [%s]\n"
			"sspi->dma_rx.dir = %d [%s]\n"
#endif
			"sspi->busy = %d [%s]\n"
			"sspi->result = %d [%s]\n"
			"sspi->base_addr = 0x%p, the SPI control register:\n"
			"[VER] 0x%02x = 0x%08x, [GCR] 0x%02x = 0x%08x, [TCR] 0x%02x = 0x%08x\n"
			"[ICR] 0x%02x = 0x%08x, [ISR] 0x%02x = 0x%08x, [FCR] 0x%02x = 0x%08x\n"
			"[FSR] 0x%02x = 0x%08x, [WCR] 0x%02x = 0x%08x, [CCR] 0x%02x = 0x%08x\n"
			"[BCR] 0x%02x = 0x%08x, [TCR] 0x%02x = 0x%08x, [BCC] 0x%02x = 0x%08x\n"
			"[DMA] 0x%02x = 0x%08x, [TXR] 0x%02x = 0x%08x, [RXD] 0x%02x = 0x%08x\n",
			master->bus_num, master->num_chipselect, master->dma_alignment,
			master->mode_bits, master->flags, master->bus_lock_flag,
			master->busy, master->running, master->rt,
			sspi->mode_type, spi_mode[sspi->mode_type],
			sspi->irq, sspi->dev_name,
#if IS_ENABLED(CONFIG_DMA_ENGINE)
			sspi->dma_tx.dir, dma_dir[sspi->dma_tx.dir],
			sspi->dma_rx.dir, dma_dir[sspi->dma_rx.dir],
#endif
			sspi->busy, busy_state[sspi->busy],
			sspi->result, result_str[sspi->result],
			sspi->base_addr,
			SPI_VER_REG, readl(sspi->base_addr + SPI_VER_REG),
			SPI_GC_REG, readl(sspi->base_addr + SPI_GC_REG),
			SPI_TC_REG, readl(sspi->base_addr + SPI_TC_REG),
			SPI_INT_CTL_REG, readl(sspi->base_addr + SPI_INT_CTL_REG),
			SPI_INT_STA_REG, readl(sspi->base_addr + SPI_INT_STA_REG),

			SPI_FIFO_CTL_REG, readl(sspi->base_addr + SPI_FIFO_CTL_REG),
			SPI_FIFO_STA_REG, readl(sspi->base_addr + SPI_FIFO_STA_REG),
			SPI_WAIT_CNT_REG, readl(sspi->base_addr + SPI_WAIT_CNT_REG),
			SPI_CLK_CTL_REG, readl(sspi->base_addr + SPI_CLK_CTL_REG),
			SPI_BURST_CNT_REG, readl(sspi->base_addr + SPI_BURST_CNT_REG),

			SPI_TRANSMIT_CNT_REG, readl(sspi->base_addr + SPI_TRANSMIT_CNT_REG),
			SPI_BCC_REG, readl(sspi->base_addr + SPI_BCC_REG),
			SPI_DMA_CTL_REG, readl(sspi->base_addr + SPI_DMA_CTL_REG),
			SPI_TXDATA_REG, readl(sspi->base_addr + SPI_TXDATA_REG),
			SPI_RXDATA_REG, readl(sspi->base_addr + SPI_RXDATA_REG));
}
static struct device_attribute sunxi_spi_status_attr =
	__ATTR(status, S_IRUGO, sunxi_spi_status_show, NULL);

static void sunxi_spi_create_sysfs(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_spi_info_attr);
	device_create_file(&_pdev->dev, &sunxi_spi_status_attr);
}

static void sunxi_spi_remove_sysfs(struct platform_device *_pdev)
{
	device_remove_file(&_pdev->dev, &sunxi_spi_info_attr);
	device_remove_file(&_pdev->dev, &sunxi_spi_status_attr);
}

static int sunxi_spi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource	*mem_res;
	struct sunxi_spi *sspi;
	struct sunxi_spi_platform_data *pdata;
	struct spi_master *master;
	struct sunxi_slave *slave = NULL;
	char spi_para[16] = {0};
	int ret = 0, err = 0, irq;

	if (np == NULL) {
		SPI_ERR("SPI failed to get of_node\n");
		return -ENODEV;
	}

	pdev->id = of_alias_get_id(np, "spi");
	if (pdev->id < 0) {
		SPI_ERR("SPI failed to get alias id\n");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_DMA_ENGINE)
	pdev->dev.dma_mask = &sunxi_spi_dma_mask;
	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
#endif

	pdata = kzalloc(sizeof(struct sunxi_spi_platform_data), GFP_KERNEL);
	if (pdata == NULL) {
		SPI_ERR("SPI failed to alloc mem\n");
		return -ENOMEM;
	}
	pdev->dev.platform_data = pdata;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		SPI_ERR("Unable to get spi MEM resource\n");
		ret = -ENXIO;
		goto err0;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		SPI_ERR("No spi IRQ specified\n");
		ret = -ENXIO;
		goto err0;
	}

	snprintf(spi_para, sizeof(spi_para), "spi%d_cs_number", pdev->id);
	ret = of_property_read_u32(np, spi_para, &pdata->cs_num);
	if (ret) {
		SPI_ERR("Failed to get cs_number property\n");
		ret = -EINVAL;
		goto err0;
	}

	/* create spi master */
	master = spi_alloc_master(&pdev->dev, sizeof(struct sunxi_spi));
	if (master == NULL) {
		SPI_ERR("Unable to allocate SPI Master\n");
		ret = -ENOMEM;
		goto err0;
	}

	platform_set_drvdata(pdev, master);
	sspi = spi_master_get_devdata(master);
	memset(sspi, 0, sizeof(struct sunxi_spi));

	sspi->master        = master;
#if IS_ENABLED(CONFIG_DMA_ENGINE)
	sspi->dma_rx.dir		= SPI_DMA_RWNULL;
	sspi->dma_tx.dir		= SPI_DMA_RWNULL;
#endif
	sspi->busy			= SPI_FREE;
	sspi->mode_type			= MODE_TYPE_NULL;
	sspi->irq			= irq;

	master->max_speed_hz		= SPI_MAX_FREQUENCY;
	master->dev.of_node		= pdev->dev.of_node;
	master->bus_num			= pdev->id;
	master->setup			= sunxi_spi_setup;
	master->can_dma			= sunxi_spi_can_dma;
	master->transfer_one		= sunxi_spi_transfer_one;
	master->use_gpio_descriptors	= true;
	master->set_cs			= sunxi_spi_set_cs;
	master->num_chipselect		= pdata->cs_num;
	master->bits_per_word_mask	= SPI_BPW_MASK(8);
	/* the spi->mode bits understood by this driver: */
	master->mode_bits	= SPI_CPOL | SPI_CPHA | SPI_CS_HIGH
				| SPI_LSB_FIRST | SPI_TX_DUAL | SPI_TX_QUAD
				| SPI_RX_DUAL | SPI_RX_QUAD;

	ret = of_property_read_u32(np, "spi_dbi_enable", &sspi->dbi_enabled);
	if (ret == 0)
		dprintk(DEBUG_INIT, "[spi%d] SPI DBI INTERFACE\n",
				sspi->master->bus_num);
	else
		sspi->dbi_enabled = 0;

	snprintf(sspi->dev_name, sizeof(sspi->dev_name), SUNXI_SPI_DEV_NAME"%d", pdev->id);

	err = devm_request_irq(&pdev->dev, irq, sunxi_spi_handler, 0,
			sspi->dev_name, sspi);
	if (err) {
		SPI_ERR("[spi%d] Cannot request IRQ\n", sspi->master->bus_num);
		ret = -EINVAL;
		goto err1;
	}

	if (request_mem_region(mem_res->start,
			resource_size(mem_res), pdev->name) == NULL) {
		SPI_ERR("[spi%d] Req mem region failed\n", sspi->master->bus_num);
		ret = -ENXIO;
		goto err2;
	}

	sspi->base_addr = ioremap(mem_res->start, resource_size(mem_res));
	if (sspi->base_addr == NULL) {
		SPI_ERR("[spi%d] Unable to remap IO\n", sspi->master->bus_num);
		ret = -ENXIO;
		goto err3;
	}

	sspi->base_addr_phy = mem_res->start;
	sspi->pdev = pdev;
	pdev->dev.init_name = sspi->dev_name;

	err = sunxi_spi_resource_get(sspi);
	if (err) {
		SPI_ERR("[spi%d] resource get error\n", sspi->master->bus_num);
		ret = -EINVAL;
		goto err1;
	}

	/* Setup Deufult Mode */
	ret = sunxi_spi_hw_init(sspi, pdata, &pdev->dev);
	if (ret != 0) {
		SPI_ERR("[spi%d] spi hw init failed!\n", sspi->master->bus_num);
		ret = -EINVAL;
		goto err4;
	}

	spin_lock_init(&sspi->lock);
	init_completion(&sspi->done);

	if (sspi->mode) {
		slave = kzalloc(sizeof(*slave), GFP_KERNEL);
		if (IS_ERR_OR_NULL(slave)) {
			SPI_ERR("[spi%d] failed to alloc mem\n", sspi->master->bus_num);
			ret = -ENOMEM;
			goto err5;
		}
		sspi->slave = slave;
		sspi->slave->set_up_txdata = sunxi_spi_slave_set_txdata;
		sspi->task = kthread_create(sunxi_spi_slave_task, sspi, "spi_slave");
		if (IS_ERR(sspi->task)) {
			SPI_ERR("[spi%d] unable to start kernel thread.\n", sspi->master->bus_num);
			ret = PTR_ERR(sspi->task);
			sspi->task = NULL;
			ret = -EINVAL;
			goto err6;
		}

		wake_up_process(sspi->task);
	} else {
		if (spi_register_master(master)) {
			SPI_ERR("[spi%d] cannot register SPI master\n", sspi->master->bus_num);
			ret = -EBUSY;
			goto err6;
		}
	}

	sunxi_spi_create_sysfs(pdev);

	dprintk(DEBUG_INFO, "[spi%d] loaded for Bus with %d Slaves at most\n",
		master->bus_num, master->num_chipselect);
	dprintk(DEBUG_INIT, "[spi%d]: driver probe succeed, base %px, irq %d\n",
		master->bus_num, sspi->base_addr, sspi->irq);
	return 0;

err6:
	if (sspi->mode)
		if (!IS_ERR_OR_NULL(slave))
			kfree(slave);
err5:
	sunxi_spi_hw_exit(sspi, pdata);

err4:
	iounmap(sspi->base_addr);
err3:
	release_mem_region(mem_res->start, resource_size(mem_res));
err2:
	free_irq(sspi->irq, sspi);
err1:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);
err0:
	kfree(pdata);

	return ret;
}

static int sunxi_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spi *sspi = spi_master_get_devdata(master);
	struct resource	*mem_res;
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy |= SPI_FREE;
	spin_unlock_irqrestore(&sspi->lock, flags);

	while (sspi->busy & SPI_BUSY)
		msleep(10);

	if (sspi->mode)
		if (!sspi->task_flag)
			if (!IS_ERR(sspi->task))
				kthread_stop(sspi->task);

	sunxi_spi_remove_sysfs(pdev);
	sunxi_spi_hw_exit(sspi, pdev->dev.platform_data);
	iounmap(sspi->base_addr);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res != NULL)
		release_mem_region(mem_res->start, resource_size(mem_res));
	free_irq(sspi->irq, sspi);
	spi_unregister_master(master);
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);
	kfree(pdev->dev.platform_data);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_spi_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spi *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy |= SPI_SUSPND;
	spin_unlock_irqrestore(&sspi->lock, flags);

	while (sspi->busy & SPI_BUSY)
		msleep(10);

	sunxi_spi_hw_exit(sspi, pdev->dev.platform_data);

	dprintk(DEBUG_SUSPEND, "[spi%d] suspend finish\n", master->bus_num);

	return 0;
}

static int sunxi_spi_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spi  *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	sunxi_spi_hw_init(sspi, pdev->dev.platform_data, dev);

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy = SPI_FREE;
	spin_unlock_irqrestore(&sspi->lock, flags);
	dprintk(DEBUG_SUSPEND, "[spi%d] resume finish\n", master->bus_num);

	return 0;
}

static const struct dev_pm_ops sunxi_spi_dev_pm_ops = {
	.suspend = sunxi_spi_suspend,
	.resume  = sunxi_spi_resume,
};

#define SUNXI_SPI_DEV_PM_OPS (&sunxi_spi_dev_pm_ops)
#else
#define SUNXI_SPI_DEV_PM_OPS NULL
#endif /* CONFIG_PM */

static const struct of_device_id sunxi_spi_match[] = {
	{ .compatible = "allwinner,sun8i-spi", },
	{ .compatible = "allwinner,sun20i-spi", },
	{ .compatible = "allwinner,sun50i-spi", },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_spi_match);


static struct platform_driver sunxi_spi_driver = {
	.probe   = sunxi_spi_probe,
	.remove  = sunxi_spi_remove,
	.driver = {
		.name	= SUNXI_SPI_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm		= SUNXI_SPI_DEV_PM_OPS,
		.of_match_table = sunxi_spi_match,
	},
};

static int __init sunxi_spi_init(void)
{
	return platform_driver_register(&sunxi_spi_driver);
}

static void __exit sunxi_spi_exit(void)
{
	platform_driver_unregister(&sunxi_spi_driver);
}

fs_initcall_sync(sunxi_spi_init);
module_exit(sunxi_spi_exit);
module_param_named(debug, debug_mask, int, 0664);

MODULE_AUTHOR("pannan");
MODULE_DESCRIPTION("SUNXI SPI BUS Driver");
MODULE_ALIAS("platform:"SUNXI_SPI_DEV_NAME);
MODULE_LICENSE("GPL");
