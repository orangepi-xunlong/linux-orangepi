/*
 * drivers/spi/spi-sunxi.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUNXI SPI Register Definition
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2013.5.7 Mintow <duanmintao@allwinnertech.com>
 *    Adapt to support sun8i/sun9i of Allwinner.
 */

#include <linux/regulator/consumer.h>

#ifndef _SUNXI_SPI_H_
#define _SUNXI_SPI_H_

#define HEXADECIMAL	(0x10)
#define REG_END		(0x0f)
#define SAMPLE_NUMBER	(0x80)
#define REG_INTERVAL	(0x04)
#define REG_CL		(0x0c)

#define SPI_MODULE_NUM		(4)
#define SPI_FIFO_DEPTH		(128)
#define MAX_FIFU		64
#define BULK_DATA_BOUNDARY	64	 /* can modify to adapt the application */
#define SPI_MAX_FREQUENCY	100000000 /* spi controller just support 100Mhz */
#define SPI_HIGH_FREQUENCY	60000000 /* sample mode threshold frequency  */
#define SPI_LOW_FREQUENCY	24000000 /* sample mode threshold frequency  */

/* SPI Registers offsets from peripheral base address */
#define SPI_VER_REG		(0x00)	/* version number register */
#define SPI_GC_REG		(0x04)	/* global control register */
#define SPI_TC_REG		(0x08)	/* transfer control register */
#define SPI_INT_CTL_REG		(0x10)	/* interrupt control register */
#define SPI_INT_STA_REG		(0x14)	/* interrupt status register */
#define SPI_FIFO_CTL_REG	(0x18)	/* fifo control register */
#define SPI_FIFO_STA_REG	(0x1C)	/* fifo status register */
#define SPI_WAIT_CNT_REG	(0x20)	/* wait clock counter register */
#define SPI_CLK_CTL_REG		(0x24)	/* clock rate control register */
#define SPI_BURST_CNT_REG	(0x30)	/* burst counter register */
#define SPI_TRANSMIT_CNT_REG	(0x34)	/* transmit counter register */
#define SPI_BCC_REG		(0x38)	/* burst control counter register */
#define SPI_DMA_CTL_REG		(0x88)	/* DMA control register, only for 1639 */
#define SPI_TXDATA_REG		(0x200)	/* tx data register */
#define SPI_RXDATA_REG		(0x300)	/* rx data register */
#define SPI_DBI_CR_REG		(0x100)	/* DBI control register */
#define SPI_DBI_CR_REG1		(0x104)	/* DBI control register */
#define SPI_DBI_CR_REG2		(0x108)	/* DBI control register */
#define SPI_DBI_TIMER_REG       (0x10C)	/* DBI timer register */
#define SPI_DBI_VIDEO_SIZE	(0x110)	/* DBI VIDEO SIZE register */
#define SPI_DBI_INT_REG		(0x120)	/* DBI interrupter status register */

/* SPI Global Control Register Bit Fields & Masks,default value:0x0000_0080 */
#define SPI_GC_EN			(0x1 <<  0) /* SPI module enable control 1:enable; 0:disable; default:0 */
#define SPI_GC_MODE			(0x1 <<  1) /* SPI function mode select 1:master; 0:slave; default:0 */
#define SPI_GC_TP_EN		(0x1 <<  7) /* SPI transmit stop enable 1:stop transmit data when RXFIFO is full; 0:ignore RXFIFO status; default:1 */
#define SPI_GC_SRST			(0x1 << 31) /* soft reset, write 1 will clear SPI control, auto clear to 0 */
#define SPI_GC_DBI_MODE_SEL	(0x1 <<  3) /* SPI interface mode select 1:dbi; 0:spi; default:0 */
#define SPI_GC_DBI_EN		(0x1 <<  4) /* SPI DBI  mode enable 1:enable; 0:disable; default:0 */

/* SPI Transfer Control Register Bit Fields & Masks,default value:0x0000_0087 */
#define SPI_TC_PHA		(0x1 <<  0) /* SPI Clock/Data phase control,0: phase0,1: phase1;default:1 */
#define SPI_TC_POL		(0x1 <<  1) /* SPI Clock polarity control,0:low level idle,1:high level idle;default:1 */
#define SPI_TC_SPOL		(0x1 <<  2) /* SPI Chip select signal polarity control,default: 1,low effective like this:~~|_____~~ */
#define SPI_TC_SSCTL	(0x1 <<  3) /* SPI chip select control,default 0:SPI_SSx remains asserted between SPI bursts,1:negate SPI_SSx between SPI bursts */
#define SPI_TC_SS_MASK		(0x3 <<  4) /* SPI chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPI_TC_SS_OWNER		(0x1 <<  6) /* SS output mode select default is 0:automatic output SS;1:manual output SS */
#define SPI_TC_SS_LEVEL		(0x1 <<  7) /* defautl is 1:set SS to high;0:set SS to low */
#define SPI_TC_DHB		(0x1 <<  8) /* Discard Hash Burst,default 0:receiving all spi burst in BC period 1:discard unused,fectch WTC bursts */
#define SPI_TC_DDB		(0x1 <<  9) /* Dummy burst Type,default 0: dummy spi burst is zero;1:dummy spi burst is one */
#define SPI_TC_RPSM		(0x1 << 10) /* select mode for high speed write,0:normal write mode,1:rapids write mode,default 0 */
#define SPI_TC_SDM		(0x1 << 13) /* master sample data mode, 1: normal sample mode;0:delay sample mode. */
#define SPI_TC_SDC		(0x1 << 11) /* master sample data control, 1: delay--high speed operation;0:no delay. */
#define SPI_TC_FBS		(0x1 << 12) /* LSB/MSB transfer first select 0:MSB,1:LSB,default 0:MSB first */
#define SPI_TC_XCH		(0x1 << 31) /* Exchange burst default 0:idle,1:start exchange;when BC is zero,this bit cleared by SPI controller*/
#define SPI_TC_SS_BIT_POS	(4)

/* SPI Interrupt Control Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_INTEN_RX_RDY	(0x1 <<  0) /* rxFIFO Ready Interrupt Enable,---used for immediately received,0:disable;1:enable */
#define SPI_INTEN_RX_EMP	(0x1 <<  1) /* rxFIFO Empty Interrupt Enable ---used for IRQ received */
#define SPI_INTEN_RX_FULL	(0x1 <<  2) /* rxFIFO Full Interrupt Enable ---seldom used */
#define SPI_INTEN_TX_ERQ	(0x1 <<  4) /* txFIFO Empty Request Interrupt Enable ---seldom used */
#define SPI_INTEN_TX_EMP	(0x1 <<  5) /* txFIFO Empty Interrupt Enable ---used  for IRQ tx */
#define SPI_INTEN_TX_FULL	(0x1 <<  6) /* txFIFO Full Interrupt Enable ---seldom used */
#define SPI_INTEN_RX_OVF	(0x1 <<  8) /* rxFIFO Overflow Interrupt Enable ---used for error detect */
#define SPI_INTEN_RX_UDR	(0x1 <<  9) /* rxFIFO Underrun Interrupt Enable ---used for error detect */
#define SPI_INTEN_TX_OVF	(0x1 << 10) /* txFIFO Overflow Interrupt Enable ---used for error detect */
#define SPI_INTEN_TX_UDR	(0x1 << 11) /* txFIFO Underrun Interrupt Enable ---not happened */
#define SPI_INTEN_TC		(0x1 << 12) /* Transfer Completed Interrupt Enable  ---used */
#define SPI_INTEN_SSI		(0x1 << 13) /* SSI interrupt Enable,chip select from valid state to invalid state,for slave used only */
#define SPI_INTEN_ERR		(SPI_INTEN_TX_OVF|SPI_INTEN_RX_UDR|SPI_INTEN_RX_OVF) /* NO txFIFO underrun */
#define SPI_INTEN_MASK		(0x77|(0x3f<<8))

/* SPI Interrupt Status Register Bit Fields & Masks,default value:0x0000_0022 */
#define SPI_INT_STA_RX_RDY	(0x1 <<  0) /* rxFIFO ready, 0:RX_WL < RX_TRIG_LEVEL,1:RX_WL >= RX_TRIG_LEVEL */
#define SPI_INT_STA_RX_EMP	(0x1 <<  1) /* rxFIFO empty, this bit is set when rxFIFO is empty */
#define SPI_INT_STA_RX_FULL	(0x1 <<  2) /* rxFIFO full, this bit is set when rxFIFO is full */
#define SPI_INT_STA_TX_RDY	(0x1 <<  4) /* txFIFO ready, 0:TX_WL > TX_TRIG_LEVEL,1:TX_WL <= TX_TRIG_LEVEL */
#define SPI_INT_STA_TX_EMP	(0x1 <<  5) /* txFIFO empty, this bit is set when txFIFO is empty */
#define SPI_INT_STA_TX_FULL	(0x1 <<  6) /* txFIFO full, this bit is set when txFIFO is full */
#define SPI_INT_STA_RX_OVF	(0x1 <<  8) /* rxFIFO overflow, when set rxFIFO has overflowed */
#define SPI_INT_STA_RX_UDR	(0x1 <<  9) /* rxFIFO underrun, when set rxFIFO has underrun */
#define SPI_INT_STA_TX_OVF	(0x1 << 10) /* txFIFO overflow, when set txFIFO has overflowed */
#define SPI_INT_STA_TX_UDR	(0x1 << 11) /* fxFIFO underrun, when set txFIFO has underrun */
#define SPI_INT_STA_TC		(0x1 << 12) /* Transfer Completed */
#define SPI_INT_STA_SSI		(0x1 << 13) /* SS invalid interrupt, when set SS has changed from valid to invalid */
#define SPI_INT_STA_ERR		(SPI_INT_STA_TX_OVF|SPI_INT_STA_RX_UDR|SPI_INT_STA_RX_OVF) /* NO txFIFO underrun */
#define SPI_INT_STA_MASK	(0x77|(0x3f<<8))

/* SPI FIFO Control Register Bit Fields & Masks,default value:0x0040_0001 */
#define SPI_FIFO_CTL_RX_LEVEL	(0xFF <<  0) /* rxFIFO reday request trigger level,default 0x1 */
#define SPI_FIFO_CTL_RX_DRQEN	(0x1  <<  8) /* rxFIFO DMA request enable,1:enable,0:disable */
#define SPI_FIFO_CTL_RX_TESTEN	(0x1  << 14) /* rxFIFO test mode enable,1:enable,0:disable */
#define SPI_FIFO_CTL_RX_RST	(0x1  << 15) /* rxFIFO reset, write 1, auto clear to 0 */
#define SPI_FIFO_CTL_TX_LEVEL	(0xFF << 16) /* txFIFO empty request trigger level,default 0x40 */
#define SPI_FIFO_CTL_TX_DRQEN	(0x1  << 24) /* txFIFO DMA request enable,1:enable,0:disable */
#define SPI_FIFO_CTL_TX_TESTEN	(0x1  << 30) /* txFIFO test mode enable,1:enable,0:disable */
#define SPI_FIFO_CTL_TX_RST	(0x1  << 31) /* txFIFO reset, write 1, auto clear to 0 */
#define SPI_FIFO_CTL_DRQEN_MASK	(SPI_FIFO_CTL_TX_DRQEN|SPI_FIFO_CTL_RX_DRQEN)

/* SPI FIFO Status Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_FIFO_STA_RX_CNT	(0xFF <<  0) /* rxFIFO counter,how many bytes in rxFIFO */
#define SPI_FIFO_STA_RB_CNT	(0x7  << 12) /* rxFIFO read buffer counter,how many bytes in rxFIFO read buffer */
#define SPI_FIFO_STA_RB_WR	(0x1  << 15) /* rxFIFO read buffer write enable */
#define SPI_FIFO_STA_TX_CNT	(0xFF << 16) /* txFIFO counter,how many bytes in txFIFO */
#define SPI_FIFO_STA_TB_CNT	(0x7  << 28) /* txFIFO write buffer counter,how many bytes in txFIFO write buffer */
#define SPI_FIFO_STA_TB_WR	(0x1  << 31) /* txFIFO write buffer write enable */
#define SPI_RXCNT_BIT_POS	(0)
#define SPI_TXCNT_BIT_POS	(16)

/* SPI Wait Clock Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPI_WAIT_WCC_MASK	(0xFFFF <<  0) /* used only in master mode: Wait Between Transactions */
#define SPI_WAIT_SWC_MASK	(0xF    << 16) /* used only in master mode: Wait before start dual data transfer in dual SPI mode */

/* SPI Clock Control Register Bit Fields & Masks,default:0x0000_0002 */
#define SPI_CLK_CTL_CDR2	(0xFF <<  0) /* Clock Divide Rate 2,master mode only : SPI_CLK = AHB_CLK/(2*(n+1)) */
#define SPI_CLK_CTL_CDR1	(0xF  <<  8) /* Clock Divide Rate 1,master mode only : SPI_CLK = AHB_CLK/2^n */
#define SPI_CLK_CTL_DRS		(0x1  << 12) /* Divide rate select,default,0:rate 1;1:rate 2 */
#define SPI_CLK_SCOPE		(SPI_CLK_CTL_CDR2+1)

/* SPI Master Burst Counter Register Bit Fields & Masks,default:0x0000_0000 */
/* master mode: when SMC = 1,BC specifies total burst number, Max length is 16Mbytes */
#define SPI_BC_CNT_MASK		(0xFFFFFF << 0) /* Total Burst Counter, tx length + rx length ,SMC=1 */

/* SPI Master Transmit Counter reigster default:0x0000_0000 */
#define SPI_TC_CNT_MASK		(0xFFFFFF << 0) /* Write Transmit Counter, tx length, NOT rx length!!! */

/* SPI Master Burst Control Counter reigster Bit Fields & Masks,default:0x0000_0000 */
#define SPI_BCC_STC_MASK	(0xFFFFFF <<  0) /* master single mode transmit counter */
#define SPI_BCC_DBC_MASK	(0xF	  << 24) /* master dummy burst counter */
#define SPI_BCC_DUAL_MODE	(0x1	  << 28) /* master dual mode RX enable */
#define SPI_BCC_QUAD_MODE	(0x1	  << 29) /* master quad mode RX enable */

#define DBI_CR_READ		(0x1	  << 31)
#define DBI_CR_LSB_FIRST	(0x1	  << 19)
#define DBI_CR_TRANSMIT_MODE	(0x1	  << 15)
#define DBI_CR_FORMAT		(12)
#define DBI_CR_FORMAT_MASK	(0x7	  << DBI_CR_FORMAT)
#define DBI_CR_INTERFACE	(8)
#define DBI_CR_INTERFACE_MASK	(0x7	  << DBI_CR_INTERFACE)
#define DBI_CR1_DCX_DATA	(0x1	  << 22)
#define DBI_CR1_CLK_AUTO	(0x1	  << 24)
#define DBI_CR2_SDI_PIN	(0x1	  << 6)
#define DBI_CR2_DCX_PIN	(0x1	  << 5)
#define DBI_CR2_TE_ENABLE	(0x1	  << 0)
#define DBI_CR2_DMA_ENABLE	(0x1	  << 15)
#define DBI_INT_STA_MASK	(0x7f|(0x7f<<8))
#define DBI_INT_TE_INT          (0x1 << 10) /* te enable*/
#define DBI_INT_TIMER_INT          (0x1 << 12) /* timer enable*/
#define DBI_INT_STA_FRAME	(0x1 << 9) /* fram Transfer Completed */
#define DBI_INT_FIFO_EMPTY	(0x1 << 14)
#define DBI_FRAM_DONE_INT_EN	(0x1 << 1) /* fram Transfer Completed En*/
#define DBI_FIFO_EMPTY_INT_EN	(0x1 << 6) /* fram Transfer Completed En*/
#define DBI_TE_INT_EN	(0x1 << 2) /* TE interrupt*/
#define DBI_TIMER_INT_EN	(0x1 << 4) /* timer interrupt*/

#define SPI_PHA_ACTIVE_		(0x01)
#define SPI_POL_ACTIVE_		(0x02)

#define SPI_MODE_0_ACTIVE_	(0|0)
#define SPI_MODE_1_ACTIVE_	(0|SPI_PHA_ACTIVE_)
#define SPI_MODE_2_ACTIVE_	(SPI_POL_ACTIVE_|0)
#define SPI_MODE_3_ACTIVE_	(SPI_POL_ACTIVE_|SPI_PHA_ACTIVE_)
#define SPI_CS_HIGH_ACTIVE_	(0x04)
#define SPI_LSB_FIRST_ACTIVE_	(0x08)
#define SPI_DUMMY_ONE_ACTIVE_	(0x10)
#define SPI_RECEIVE_ALL_ACTIVE_	(0x20)

#define SPI_DBI_COMMAND_READ_	(0x10)
#define SPI_DBI_LSB_FIRST_	(0x20)
#define SPI_DBI_TRANSMIT_VIDEO_	(0x40)
#define SPI_DBI_DCX_DATA_	(0x80)

/* About SUNXI */
#define SUNXI_SPI_DEV_NAME	"spi"

/* About DMA */
#ifdef CONFIG_ARCH_SUN9IW1P1
#define SPI_DMA_WAIT_MODE	0xA5
#define SPI_DMA_SHAKE_MODE	0xEA
#define spi_set_dma_mode(base)	writel(SPI_DMA_SHAKE_MODE, base + SPI_DMA_CTL_REG)
#else
#define spi_set_dma_mode(base)
#endif

struct sunxi_spi_platform_data {
	int cs_bitmap; /* cs0-0x1,cs1-0x2,cs0&cs1-0x3 */
	int cs_num;    /* number of cs */
	int sclk_freq_def;	/* clk frequence*/
	char regulator_id[16];
	struct regulator *regulator;
};

/* spi device controller state, alloc */
struct sunxi_spi_config {
	int bits_per_word; /* 8bit */
	int max_speed_hz;  /* 80MHz */
	int mode; /* pha,pol,LSB,etc.. */
};

/* spi device data, used in dual spi mode */
struct sunxi_dual_mode_dev_data {
	int dual_mode;	/* dual SPI mode, 0-single mode, 1-dual mode */
	int single_cnt;	/* single mode transmit counter */
	int dummy_cnt;	/* dummy counter should be sent before receive in dual mode */
};

enum {
	DEBUG_INIT    = 1U << 0,
	DEBUG_SUSPEND = 1U << 1,
	DEBUG_DATA    = 1U << 2,
	DEBUG_INFO    = 1U << 3,
	DEBUG_INFO1   = 1U << 4,
	DEBUG_INFO2   = 1U << 5,
	DEBUG_INFO3   = 1U << 6,
	DEBUG_INFO4   = 1U << 7,
};

enum dbi_out_seq {
	DBI_OUT_RGB = 0,
	DBI_OUT_RBG = 1,
	DBI_OUT_GRB = 2,
	DBI_OUT_GBR = 3,
	DBI_OUT_BRG = 4,
	DBI_OUT_BGR = 5,
};

enum dbi_src_seq {
	DBI_SRC_RGB = 0,
	DBI_SRC_RBG = 1,
	DBI_SRC_GRB = 2,
	DBI_SRC_GBR = 3,
	DBI_SRC_BRG = 4,
	DBI_SRC_BGR = 5,
	/* following definition only for rgb565
	 * to change the RGB order in two byte(16 bit).
	 * format:R(5bit)--G_1(3bit)--G_0(3bit)--B(5bit)
	 * G_0 mean the low 3 bit of G component
	 * G_1 mean the high 3 bit of G component
	 *  */
	DBI_SRC_GRBG_0 = 6,
	DBI_SRC_GRBG_1 = 7,
	DBI_SRC_GBRG_0 = 8,
	DBI_SRC_GBRG_1 = 9,
};

enum dbi_te_en {
	DBI_TE_DISABLE = 0,
	DBI_TE_RISING_EDGE = 1,
	DBI_TE_FALLING_EDGE = 2,
};

struct spi_dbi_config {
	enum dbi_src_seq	dbi_src_sequence;
	enum dbi_out_seq	dbi_out_sequence;
	char dbi_rgb_bit_order;
	char dbi_rgb32_alpha_pos;
	char dbi_rgb16_pixel_endian;
	char dbi_format; /*DBI OUT format*/
	char dbi_interface;
	u16	 dbi_mode;
	char dbi_clk_out_mode;
	u16			dbi_video_v;
	u16			dbi_video_h;
	enum dbi_te_en          dbi_te_en;
	unsigned char		dbi_fps;
	void			(*dbi_vsync_handle)(unsigned long data);
	char			dbi_read_bytes;
};

extern void __spi_get_dbi_config(struct spi_device *spi, struct spi_dbi_config *dbi_config);

#define DBI_RGB111		(0x0)
#define DBI_RGB444		(0x1)
#define DBI_RGB565		(0x2)
#define DBI_RGB666		(0x3)
#define DBI_RGB888		(0x4)

#define L3I1		(0x0)
#define L3I2		(0x1)
#define L4I1		(0x2)
#define L4I2		(0x3)
#define D2LI		(0x4)

#define SPI_DBI_READ		(0x10)
#define SPI_DBI_LSB_FIRST	(0x20)
#define SPI_DBI_TRANSMIT_VIDEO	(0x40)
#define SPI_DBI_DCX_DATA	(0x80)

#define SPI_DBI_CLK_AUTO_GATING	 (0x0) /*default*/
#define SPI_DBI_CLK_ALWAYS_ON	(0x1)

#define DBI_READ(dbi_config)			(dbi_config.dbi_mode |= (SPI_DBI_READ))
#define DBI_WRITE(dbi_config)			(dbi_config.dbi_mode &= ~(SPI_DBI_READ))
#define DBI_LSB_FIRST(dbi_config)		(dbi_config.dbi_mode |= SPI_DBI_LSB_FIRST)
#define DBI_MSB_FIRST(dbi_config)		(dbi_config.dbi_mode &= ~SPI_DBI_LSB_FIRST)
#define	DBI_TR_VIDEO(dbi_config)		(dbi_config.dbi_mode |= SPI_DBI_TRANSMIT_VIDEO)
#define	DBI_TR_COMMAND(dbi_config)		(dbi_config.dbi_mode &= ~(SPI_DBI_TRANSMIT_VIDEO))
#define DBI_DCX_DATA(dbi_config)		(dbi_config.dbi_mode |= SPI_DBI_DCX_DATA)
#define DBI_DCX_COMMAND(dbi_config)		(dbi_config.dbi_mode &= ~(SPI_DBI_DCX_DATA))

#endif
