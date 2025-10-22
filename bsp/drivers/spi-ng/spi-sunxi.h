/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * SUNXI SPI Controller Register Definition
 *
 */

#ifndef _SUNXI_SPI_H_
#define _SUNXI_SPI_H_

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/bitfield.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/sunxi-spi.h>
#include <dt-bindings/spi/sunxi-spi.h>

#define SUNXI_SPI_XFER_TIMEOUT	(5000)
#ifdef CONFIG_AW_IC_BOARD
#define SUNXI_SPI_POLL_TIMEOUT	(0x1ffffff)
#else
#define SUNXI_SPI_POLL_TIMEOUT (0x3ffff)
#endif

#define SUNXI_SPI_CS_MAX		(4)			/* SPI Controller support max chip select */
#define SUNXI_SPI_FIFO_DEFAULT	(64)		/* SPI Controller default fifo depth */
#define SUNXI_SPI_MAX_FREQUENCY	(150000000)	/* SPI Controller support max freq 150Mhz */
#define SUNXI_SPI_MIN_FREQUENCY	(187500)	/* SPI Controller support min freq 187.5Khz(24M/8/16) */

/* SPI Version Number Register */
#define SUNXI_SPI_VER_REG		(0x00)
	#define SUNXI_SPI_VER_H			GENMASK(31, 16)	/* Version Number Large Part */
	#define SUNXI_SPI_VER_L			GENMASK(15, 0)	/* Version Number Small Part */

/* SPI Global Control Register */
#define SUNXI_SPI_GC_REG		(0x04)
	#define SUNXI_SPI_GC_SRST			BIT(31)	/* Soft Reset */
	#define SUNXI_SPI_GC_SLV_MODE		GENMASK(10, 9)	/* Slave Mode Write Mode */
	#define SUNXI_SPI_GC_SLV_DIR		BIT(8)	/* Slave Mode Direction */
	#define SUNXI_SPI_GC_TP_EN			BIT(7)	/* Transmit Pause Enable */
	#define SUNXI_SPI_GC_SLV_MODE_SEL	BIT(5)	/* Slave Mode Sample Timing Mode Select */
	#define SUNXI_SPI_GC_DBI_EN			BIT(4)	/* DBI Mode Enable Control */
	#define SUNXI_SPI_GC_DBI_MODE_SEL	BIT(3)	/* SPI DBI Working Mode Select */
	#define SUNXI_SPI_GC_MODE_SEL		BIT(2)	/* Sample Timing Mode Select */
	#define SUNXI_SPI_GC_MODE			BIT(1)	/* SPI Function Mode Select */
	#define SUNXI_SPI_GC_EN				BIT(0)	/* SPI Module Enable Control */

/* SPI Transfer Control Register */
#define SUNXI_SPI_TC_REG		(0x08)
	#define SUNXI_SPI_TC_XCH		BIT(31)	/* Exchange Burst */
	#define SUNXI_SPI_TC_SIWE		BIT(19)	/* Slave Idle Wait Enable */
	#define SUNXI_SPI_TC_SFHE		BIT(18)	/* Slave Mode frame head auto distinguish feature enable */
	#define SUNXI_SPI_TC_VIE		BIT(17)	/* Vsync Input Enable */
	#define SUNXI_SPI_TC_VIS		BIT(16)	/* Vsync Input Select */
	#define SUNXI_SPI_TC_SDC1		BIT(15)	/* Master Sample Data Control Register 1 */
	#define SUNXI_SPI_TC_SDDM		BIT(14)	/* Sending Data Delay Mode */
	#define SUNXI_SPI_TC_SDM		BIT(13)	/* Master Sample Data Mode */
	#define SUNXI_SPI_TC_FBS		BIT(12)	/* First Transmit Bit Select */
	#define SUNXI_SPI_TC_SDC		BIT(11)	/* Master Sample Data Control */
	#define SUNXI_SPI_TC_RPSM		BIT(10)	/* Select Rapids mode for high speed write */
	#define SUNXI_SPI_TC_DDB		BIT(9)	/* Dummy Burst Type */
	#define SUNXI_SPI_TC_DHB		BIT(8)	/* Discard Hash Burst */
	#define SUNXI_SPI_TC_SS_LEVEL	BIT(7)	/* SS Signal Level Output */
	#define SUNXI_SPI_TC_SS_OWNER	BIT(6)	/* SS Output Owner Select */
	#define SUNXI_SPI_TC_SS_SEL		GENMASK(5, 4)	/* SPI Chip Select */
	#define SUNXI_SPI_TC_SSCTL		BIT(3)	/* SS Output Wave Select */
	#define SUNXI_SPI_TC_SPOL		BIT(2)	/* SPI Chip Select Signal Polarity Control */
	#define SUNXI_SPI_TC_CPOL		BIT(1)	/* SPI Clock Polarity Control */
	#define SUNXI_SPI_TC_CPHA		BIT(0)	/* SPI Clock/Data Phase Control */

/* SPI Interrupt Control/Status Register */
#define SUNXI_SPI_INT_CTL_REG	(0x10)
#define SUNXI_SPI_INT_STA_REG	(0x14)
	#define SUNXI_SPI_INT_SVE		BIT(14)	/* Slave Vsync Error Interrupt */
	#define SUNXI_SPI_INT_SSI		BIT(13)	/* SS Invalid Interrupt */
	#define SUNXI_SPI_INT_TC		BIT(12)	/* Transfer Completed Interrupt */
	#define SUNXI_SPI_INT_TX_UDR	BIT(11)	/* TX FIFO Underrun Interrupt */
	#define SUNXI_SPI_INT_TX_OVF	BIT(10)	/* TX FIFO Overflow Interrupt */
	#define SUNXI_SPI_INT_RX_UDR	BIT(9)	/* RX FIFO Underrun Interrupt */
	#define SUNXI_SPI_INT_RX_OVF	BIT(8)	/* RX FIFO Overflow Interrupt */
	#define SUNXI_SPI_INT_TX_FULL	BIT(6)	/* TX FIFO Full Interrupt */
	#define SUNXI_SPI_INT_TX_EMP	BIT(5)	/* TX FIFO Empty Interrupt */
	#define SUNXI_SPI_INT_TX_RDY	BIT(4)	/* TX FIFO Ready Request Interrupt */
	#define SUNXI_SPI_INT_RX_FULL	BIT(2)	/* RX FIFO Full Interrupt */
	#define SUNXI_SPI_INT_RX_EMP	BIT(1)	/* RX FIFO Empty Interrupt */
	#define SUNXI_SPI_INT_RX_RDY	BIT(0)	/* RX FIFO Ready Request Interrupt */
	/* non-Register */
	#define SUNXI_SPI_INT_ERR	(SUNXI_SPI_INT_TX_OVF|SUNXI_SPI_INT_RX_UDR|SUNXI_SPI_INT_RX_OVF)	/* No TX FIFO Underrun */
	#define SUNXI_SPI_INT_MASK	(GENMASK(14, 8) | GENMASK(6, 4) | GENMASK(2, 0))

/* SPI FIFO Control Register */
#define SUNXI_SPI_FIFO_CTL_REG	(0x18)
	#define SUNXI_SPI_FIFO_CTL_TX_RST			BIT(31)			/* TX FIFO Reset */
	#define SUNXI_SPI_FIFO_CTL_TX_TEST_EN		BIT(30)			/* TX Test Mode Enable */
	#define SUNXI_SPI_FIFO_CTL_TX_DRQ_EN		BIT(24)			/* TX FIFO DMA Request Enable */
	#define SUNXI_SPI_FIFO_CTL_TX_TRIG_LEVEL	GENMASK(23, 16)	/* TX FIFO Empty Request Trigger Level */
	#define SUNXI_SPI_FIFO_CTL_RX_RST			BIT(15)			/* RX FIFO Reset */
	#define SUNXI_SPI_FIFO_CTL_RX_TEST_EN		BIT(14)			/* RX Test Mode Enable */
	#define SUNXI_SPI_FIFO_CTL_RX_DRQ_EN		BIT(8)			/* RX FIFO DMA Request Enable */
	#define SUNXI_SPI_FIFO_CTL_RX_TRIG_LEVEL	GENMASK(7, 0)	/* RX FIFO Ready Request Trigger Level */
	/* non-Register */
	#define SUNXI_SPI_FIFO_CTL_RST		(SUNXI_SPI_FIFO_CTL_TX_RST|SUNXI_SPI_FIFO_CTL_RX_RST)
	#define SUNXI_SPI_FIFO_CTL_DRQ_EN	(SUNXI_SPI_FIFO_CTL_TX_DRQ_EN|SUNXI_SPI_FIFO_CTL_RX_DRQ_EN)

/* SPI FIFO Status Register */
#define SUNXI_SPI_FIFO_STA_REG	(0x1C)
	#define SUNXI_SPI_FIFO_STA_TB_WR	BIT(31)			/* TX FIFO Write Buffer Write Enable */
	#define SUNXI_SPI_FIFO_STA_TB_CNT	GENMASK(30, 28)	/* TX FIFO Write Buffer Counter - Unused after sun55iw3 */
	#define SUNXI_SPI_FIFO_STA_TX_CNT	GENMASK(23, 16)	/* TX FIFO Counter */
	#define SUNXI_SPI_FIFO_STA_RB_WR	BIT(15)			/* RX FIFO Read Buffer Write Enable */
	#define SUNXI_SPI_FIFO_STA_RB_CNT	GENMASK(14, 12)	/* RX FIFO Read Buffer Counter - Unused after sun55iw3 */
	#define SUNXI_SPI_FIFO_STA_RX_CNT	GENMASK(7, 0)	/* RX FIFO Counter */

/* SPI Wait Clock Counter Register */
#define SUNXI_SPI_WAIT_CNT_REG	(0x20)
	#define SUNXI_SPI_WAIT_CNT_SWC	GENMASK(19, 16)	/* Dual mode direction switch wait clock counter (for master mode only) */
	#define SUNXI_SPI_WAIT_CNT_WCC	GENMASK(15, 0)	/* Wait Clock Counter (In Master mode) */

/* SPI Master Burst Counter Register */
#define SUNXI_SPI_MBC_REG	(0x30)
	#define SUNXI_SPI_MBC		GENMASK(23, 0)	/* Master Burst Counter */

/* SPI Master Transmit Counter Register */
#define SUNXI_SPI_MTC_REG	(0x34)
	#define SUNXI_SPI_MWTC		GENMASK(23, 0)	/* Master Write Transmit Counter */

/* SPI Master Burst Control Counter Register */
#define SUNXI_SPI_BCC_REG		(0x38)
	#define SUNXI_SPI_BCC_QUAD_EN	BIT(29)	/* Master Quad Mode Enable */
	#define SUNXI_SPI_BCC_DRM		BIT(28)	/* Master Dual Mode Enable */
	#define SUNXI_SPI_BCC_DBC		GENMASK(27, 24)	/* Master Dummy Burst Counter */
	#define SUNXI_SPI_BCC_STC		GENMASK(23, 0)	/* Master Single Mode Transmit Counter */

/* SPI Normal DMA Mode Control Register */
#define SUNXI_SPI_DMA_CTL_REG	(0x88)
	#define SUNXI_SPI_DMA_ACT		GENMASK(7, 6)	/* SPI DMA Active Mode */
	#define SUNXI_SPI_DMA_ACK		BIT(5)			/* SPI DMA Acknowledge Mode */
	#define SUNXI_SPI_DMA_WIAT		GENMASK(4, 0)

/* SPI TX Data Register */
#define SUNXI_SPI_TXDATA_REG	(0x200)

/* SPI RX Data Register */
#define SUNXI_SPI_RXDATA_REG	(0x300)

/* SPI BUF Status Register */
#define SUNXI_SPI_BUF_STA_REG	(0x400)
	#define SUNXI_SPI_BUF_STA_TX_SHIFT_CNT	GENMASK(26, 24)	/* TX SHIFT Buffer Counter */
	#define SUNXI_SPI_BUF_STA_TB_CNT		GENMASK(18, 16)	/* TX FIFO Write Buffer Counter */
	#define SUNXI_SPI_BUF_STA_RB_CNT		GENMASK(7, 0)	/* RX FIFO Read Buffer Counter */

enum sunxi_spi_mode_type {
	MODE_TYPE_NULL,
	SINGLE_HALF_DUPLEX_RX,		/* single mode, half duplex read */
	SINGLE_HALF_DUPLEX_TX,		/* single mode, half duplex write */
	SINGLE_FULL_DUPLEX_TX_RX,	/* single mode, full duplex read and write */
	DUAL_HALF_DUPLEX_RX,		/* dual mode, half duplex read */
	DUAL_HALF_DUPLEX_TX,		/* dual mode, half duplex write */
	QUAD_HALF_DUPLEX_RX,		/* quad mode, half duplex read */
	QUAD_HALF_DUPLEX_TX,		/* quad mode, half duplex write */
};

enum sunxi_spi_quirk_flags {
	DMA_FORCE_FIXED = BIT(0),	/* workaround for spi RX_FIFO underrun issue. fixed on sun8iw21 */
	INDEPENDENT_BSR = BIT(1),	/* identify SPI_BSR register exsit or not. addition on sun55iw3 */
	NEW_SAMPLE_MODE = BIT(2),	/* new mode of sample timing including maximum 3 cycle delay and delay chain control. */
	NEW_SAMPLE_MODE_RST = BIT(3),	/* workaround for invisible fifo under new sample mode synchronize issue. */
};

struct sunxi_spi_hw_data {
	u32 master_mode_extra;	/* flag to identify hardware controller slave mode extra support */
	u32 slave_mode_extra;	/* flag to identify hardware controller master mode extra support */
	u32 hw_bus_mode;		/* flag to identify hardware controller bus mode support */
	u32 quirk_flag;
	u8 rx_fifosize;
	u8 tx_fifosize;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	resource_size_t dma_sel_addr;
	resource_size_t dma_sel_offset;
	u32 dma_sel_bit;
#endif
};

struct sunxi_spi {
	/* spi framework device */
	struct spi_controller *ctlr;
	struct platform_device *pdev;
	struct device *dev;

	/* spi dts data */
	struct resource *mem_res;
	void __iomem *base_addr;
	struct clk *pclk;		/* pll clock */
	struct clk *mclk;		/* spi module clock */
	struct clk *bus_clk;	/* spi bus clock */
	struct clk *ahb_clk;
	struct reset_control *reset;
	struct regulator *regulator;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
	void __iomem *dma_sel_base;
	bool spi_dma_sel;
#endif
	int bus_num;
	u32 bus_mode;
	u32 bus_freq;
	u32 cs_num;
	u32 cs_mode;
	u8 rx_triglevel;
	u8 tx_triglevel;
	int irq;
	bool use_dma;
	u32 bus_sample_type;
	u32 bus_sample_mode;
	u32 spi_sample_mode;
	u32 spi_sample_delay;
	/* For ready logic */
	int ready_gpio;
	int ready_irq;
	char *ready_label;
	bool ready_status;
	struct completion ready_done;
	/* For slave */
	u32 slave_cs;

	/* spi driver data */
	u32 base_addr_phy;
	enum sunxi_spi_mode_type mode_type;
	struct completion done;	/* wakup another spi transfer */
	raw_spinlock_t lock;
	const struct sunxi_spi_hw_data *data;
	bool xfer_setup;
	bool slave_aborted;
	u32 pre_speed_hz;
	int result;	/* 0: succeed -1:fail */

	/* spi dbi function */
	struct spi_device *dbi_dev;
	struct sunxi_dbi_config *dbi_config;
	u32 dbi_irqbit;

	/* spi camera function */
	u32 camera_mode;
	int camera_framehead_len;
	u8 *camera_framehead;
};

#include "dbi/spi-sunxi-dbi.h"
#include "bit/spi-sunxi-bit.h"
#include "camera/spi-sunxi-camera.h"
#include "calibrate/spi-sunxi-calibrate.h"

#endif	/* _SUNXI_SPI_H_ */
