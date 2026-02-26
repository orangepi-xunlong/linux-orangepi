// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2013 Allwinner.
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUNXI TWI Controller Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2013.5.3 Mintow <duanmintao@allwinnertech.com>
 *    Adapt to all the new chip of Allwinner. Support sun8i/sun9i
 *
 * 2021.8.15 Lewis <liuyu@allwinnertech.com>
 *	Optimization the probe function and all the function involved code
 *
 * 2021.10.13 Lewis <liuyu@allwinnertec.com>
 *	Refactored twi xfer code
 */

#include <sunxi-log.h>
#include <sunxi-common.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
#include <linux/rpmsg.h>
#include <linux/aw_rpmsg.h>
#include <linux/rpbuf.h>
#endif /* CONFIG_AW_TWI_DELAYINIT */

#define SUNXI_TWI_VERSION	"2.6.9"

/* TWI Register Offset */
/* 31:8bit reserved,7-1bit for slave addr,0 bit for GCE */
#define TWI_ADDR		(0x00)
/* 31:8bit reserved,7-0bit for second addr in 10bit addr */
#define TWI_XADDR		(0x04)
/* 31:8bit reserved, 7-0bit send or receive data byte */
#define TWI_DATA		(0x08)
/* INT_EN,BUS_EN,M_STA,INT_FLAG,A_ACK */
#define TWI_CNTR		(0x0C)
/* 28 interrupt types + 0xF8 normal type = 29  */
#define TWI_STAT		(0x10)
/* 31:7bit reserved,6-3bit,CLK_M,2-0bit CLK_N */
#define TWI_CCR			(0x14)
/* 31:1bit reserved;0bit,write 1 to clear 0. */
#define TWI_SRST		(0x18)
/* 31:2bit reserved,1:0 bit data byte follow read command */
#define TWI_EFR			(0x1C)
/* 31:6bits reserved  5:0bit for sda&scl control */
#define TWI_LCR			(0x20)
/* 23:16 VER_BIG 7:0:VER_SMALL */
#define TWI_VERSION		(0xFC)

#if IS_ENABLED(CONFIG_I2C_SLAVE)
/* TWI_ADDR */
#define TWI_ADDR_SHIFT		(0x1)
#define TWI_ADDR_10_BIT_SHIFT	(0x8)
#define TWI_ADDR_10_BIT_WIDTH	(0x3)
#define TWI_ADDR_10_BIT_MASK	(0x78)

#define TWI_SLAVE_7_BIT_MASK	(0x7f)
#define TWI_SLAVE_10_BIT_MASK	(0xff)
#endif

/* TWI_DATA */
#define TWI_DATA_MASK	(0xff << 0)

/* TWI_CNTR */
#define CLK_COUNT_MODE	(0x1 << 0)
/* set 1 to send A_ACK,then low level on SDA */
#define A_ACK		(0x1 << 2)
/* INT_FLAG,interrupt status flag: set '1' when interrupt coming */
#define INT_FLAG	(0x1 << 3)
/* M_STP,Automatic clear 0 */
#define M_STP		(0x1 << 4)
/* M_STA,atutomatic clear 0 */
#define M_STA		(0x1 << 5)
/* BUS_EN, master mode should be set 1 */
#define BUS_EN		(0x1 << 6)
/* INT_EN, set 1 to enable interrupt */
#define INT_EN		(0x1 << 7)	/* INT_EN */
/* 31:8 bit reserved */

/* TWI_STAT */
/*
 * -------------------------------------------------------------------
 * Code   Status
 * 00h    Bus error
 * 08h    START condition transmitted
 * 10h    Repeated START condition transmitted
 * 18h    Address + Write bit transmitted, ACK received
 * 20h    Address + Write bit transmitted, ACK not received
 * 28h    Data byte transmitted in master mode, ACK received
 * 30h    Data byte transmitted in master mode, ACK not received
 * 38h    Arbitration lost in address or data byte
 * 40h    Address + Read bit transmitted, ACK received
 * 48h    Address + Read bit transmitted, ACK not received
 * 50h    Data byte received in master mode, ACK transmitted
 * 58h    Data byte received in master mode, not ACK transmitted
 * 60h    Slave address + Write bit received, ACK transmitted
 * 68h    Arbitration lost in address as master,
 *	  slave address + Write bit received, ACK transmitted
 * 70h    General Call address received, ACK transmitted
 * 78h    Arbitration lost in address as master,
 *	  General Call address received, ACK transmitted
 * 80h    Data byte received after slave address received, ACK transmitted
 * 88h    Data byte received after slave address received, not ACK transmitted
 * 90h    Data byte received after General Call received, ACK transmitted
 * 98h    Data byte received after General Call received, not ACK transmitted
 * A0h    STOP or repeated START condition received in slave mode
 * A8h    Slave address + Read bit received, ACK transmitted
 * B0h    Arbitration lost in address as master,
 *	  slave address + Read bit received, ACK transmitted
 * B8h    Data byte transmitted in slave mode, ACK received
 * C0h    Data byte transmitted in slave mode, ACK not received
 * C8h    Last byte transmitted in slave mode, ACK received
 * D0h    Second Address byte + Write bit transmitted, ACK received
 * D8h    Second Address byte + Write bit transmitted, ACK not received
 * F8h    No relevant status information or no interrupt
 *--------------------------------------------------------------------------
 */
#define TWI_STAT_MASK		(0xff)
/* 7:0 bits use only,default is 0xF8 */
#define TWI_STAT_BUS_ERR	(0x00)	/* BUS ERROR */
/* timeout when sending 9th scl clk */
#define TWI_STAT_TIMEOUT_9CLK  (0x01)
/* start can't send out */
#define TWI_STAT_TX_NSTA	(0x02)	/* defined by us not spec */
/* Master mode use only */
#define TWI_STAT_TX_STA		(0x08)	/* START condition transmitted */
/* Repeated START condition transmitted */
#define TWI_STAT_TX_RESTA	(0x10)
/* Address+Write bit transmitted, ACK received */
#define TWI_STAT_TX_AW_ACK	(0x18)
/* Address+Write bit transmitted, ACK not received */
#define TWI_STAT_TX_AW_NAK	(0x20)
/* data byte transmitted in master mode,ack received */
#define TWI_STAT_TXD_ACK	(0x28)
/* data byte transmitted in master mode ,ack not received */
#define TWI_STAT_TXD_NAK	(0x30)
/* arbitration lost in address or data byte */
#define TWI_STAT_ARBLOST	(0x38)
/* Address+Read bit transmitted, ACK received */
#define TWI_STAT_TX_AR_ACK	(0x40)
/* Address+Read bit transmitted, ACK not received */
#define TWI_STAT_TX_AR_NAK	(0x48)
/* data byte received in master mode ,ack transmitted */
#define TWI_STAT_RXD_ACK	(0x50)
/* date byte received in master mode,not ack transmitted */
#define TWI_STAT_RXD_NAK	(0x58)
/* Slave mode use only */
/* Slave address+Write bit received, ACK transmitted */
#define TWI_STAT_RXWS_ACK	(0x60)
#define TWI_STAT_ARBLOST_RXWS_ACK	(0x68)
/* General Call address received, ACK transmitted */
#define TWI_STAT_RXGCAS_ACK		(0x70)
#define TWI_STAT_ARBLOST_RXGCAS_ACK	(0x78)
#define TWI_STAT_RXDS_ACK		(0x80)
#define TWI_STAT_RXDS_NAK		(0x88)
#define TWI_STAT_RXDGCAS_ACK		(0x90)
#define TWI_STAT_RXDGCAS_NAK		(0x98)
#define TWI_STAT_RXSTPS_RXRESTAS	(0xA0)
#define TWI_STAT_RXRS_ACK		(0xA8)
#define TWI_STAT_ARBLOST_SLAR_ACK	(0xB0)
#define TWI_STAT_SLV_TXD_ACK		(0xB8)
#define TWI_STAT_SLV_TXD_NACK		(0xC0)
#define TWI_STAT_SLV_TX_LAST_ACK	(0xC8)
/* 10bit Address, second part of address */
/* Second Address byte+Write bit transmitted,ACK received */
#define TWI_STAT_TX_SAW_ACK		(0xD0)
/* Second Address byte+Write bit transmitted,ACK not received */
#define TWI_STAT_TX_SAW_NAK		(0xD8)
/* No relevant status information,INT_FLAG = 0 */
#define TWI_STAT_IDLE			(0xF8)
/* status erro */
#define TWI_STAT_ERROR			(0xF9)

/* TWI_CCR */
/*
 * Fin is APB CLOCK INPUT;
 * Fsample = F0 = Fin/2^CLK_N;
 *	F1 = F0/(CLK_M+1);
 *
 * Foscl = F1/10 = Fin/(2^CLK_N * (CLK_M+1)*10);
 * Foscl is clock SCL;standard mode:100KHz or fast mode:400KHz
 */
#define TWI_CLK_DUTY		(0x1 << 7)	/* 7bit  */
#define TWI_CLK_DUTY_30		(0x1 << 8)	/* 8bit  */
#define TWI_CLK_DIV_M_OFFSET	3
#define TWI_CLK_DIV_M		(0xf << TWI_CLK_DIV_M_OFFSET)	/* 6:3bit  */
#define TWI_CLK_DIV_N_OFFSET	0
#define TWI_CLK_DIV_N		(0x7 << TWI_CLK_DIV_N_OFFSET)	/* 2:0bit */

/* TWI_SRST */
/* write 1 to clear 0, when complete soft reset clear 0 */
#define TWI_SOFT_RST		(0x1 << 0)

/* TWI_EFR */
/* default -- 0x0 */
/* 00:no,01: 1byte, 10:2 bytes, 11: 3bytes */
#define TWI_EFR_MASK		(0x3 << 0)
#define NO_DATA_WROTE		(0x0 << 0)
#define BYTE_DATA_WROTE		(0x1 << 0)
#define BYTES_DATA1_WROTE	(0x2 << 0)
#define BYTES_DATA2_WROTE	(0x3 << 0)

/* TWI_LCR */
#define SCL_STATE		(0x1 << 5)
#define SDA_STATE		(0x1 << 4)
#define SCL_CTL			(0x1 << 3)
#define SCL_CTL_EN		(0x1 << 2)
#define SDA_CTL			(0x1 << 1)
#define SDA_CTL_EN		(0x1 << 0)

#define TWI_DRV_CTRL	(0x200)
#define TWI_DRV_CFG		(0x204)
#define TWI_DRV_SLV		(0x208)
#define TWI_DRV_FMT		(0x20C)
#define TWI_DRV_BUS_CTRL	(0x210)
#define TWI_DRV_INT_CTRL	(0x214)
#define TWI_DRV_DMA_CFG		(0x218)
#define TWI_DRV_FIFO_CON	(0x21C)
#define TWI_DRV_SEND_FIFO_ACC	(0x300)
#define TWI_DRV_RECV_FIFO_ACC	(0x304)

/* TWI_DRV_CTRL */
/* 0:module disable; 1:module enable; only use in TWI master Mode */
#define TWI_DRV_EN		(0x01 << 0)
/* 0:normal; 1:reset */
#define SOFT_RESET		(0x01 << 1)
#define TIMEOUT_N_OFFSET	8
#define TIMEOUT_N		(0xff << TIMEOUT_N_OFFSET)
#define TWI_DRV_STAT_OFFSET	16
#define TWI_DRV_STAT_MASK	(0xff << TWI_DRV_STAT_OFFSET)

#define TRAN_RESULT	(0x07 << 24)
/* 0:send slave_id + W; 1:do not send slave_id + W */
#define READ_TRAN_MODE	(0x01 << 28)
/* 0:restart; 1:STOP + START */
#define RESTART_MODE	(0x01 << 29)
/* 0:transmission idle; 1:start transmission */
#define START_TRAN	(0x01 << 31)

/* TWI_DRV_CFG */
#define PACKET_CNT_OFFSET	0
#define PACKET_CNT	(0xffff << PACKET_CNT_OFFSET)
#define PACKET_INTERVAL_OFFSET	16
#define PACKET_INTERVAL	(0xffff << PACKET_INTERVAL_OFFSET)

/* TWI_DRV_SLV */
#define SLV_ID_X_OFFSET	0
#define SLV_ID_X	(0xff << SLV_ID_X_OFFSET)
#define CMD		(0x01 << 8)
#define SLV_ID_OFFSET	9
#define SLV_ID		(0x7f << SLV_ID_OFFSET)

/* TWI_DRV_FMT */
/* how many bytes be sent/received as data */
#define DATA_BYTE_OFFSET 0
#define DATA_BYTE	(0xffff << DATA_BYTE_OFFSET)
/* how many btyes be sent as slave device reg address */
#define ADDR_BYTE_OFFSET 16
#define ADDR_BYTE	(0xff << ADDR_BYTE_OFFSET)

/* TWI_DRV_BUS_CTRL */
/* SDA manual output en */
#define SDA_MOE		(0x01 << 0)
/* SCL manual output en */
#define SCL_MOE		(0x01 << 1)
/* SDA manual output value */
#define SDA_MOV		(0x01 << 2)
/* SCL manual output value */
#define SCL_MOV		(0x01 << 3)
/* SDA current status */
#define SDA_STA		(0x01 << 6)
/* SCL current status */
#define SCL_STA		(0x01 << 7)
#define TWI_DRV_CLK_M_OFFSET	8
#define TWI_DRV_CLK_M		(0x0f << TWI_DRV_CLK_M_OFFSET)
#define TWI_DRV_CLK_N_OFFSET	12
#define TWI_DRV_CLK_N		(0x07 << TWI_DRV_CLK_N_OFFSET)
#define TWI_DRV_CLK_DUTY	(0x01 << 15)
#define TWI_DRV_COUNT_MODE	(0x01 << 16)
#define TWI_DRV_CLK_DUTY_30	(0x01 << 17)

/* TWI_DRV_INT_CTRL */
#define TRAN_COM_PD	(0x1 << 0)
#define TRAN_ERR_PD	(0x1 << 1)
#define TX_REQ_PD	(0x1 << 2)
#define RX_REQ_PD	(0x1 << 3)
#define TRAN_COM_INT_EN	(0x1 << 16)
#define TRAN_ERR_INT_EN	(0x1 << 17)
#define TX_REQ_INT_EN	(0x1 << 18)
#define RX_REQ_INT_EN	(0x1 << 19)
#define TWI_DRV_INT_EN_MASK	(0x0f << 16)
#define TWI_DRV_INT_STA_MASK	(0x0f << 0)

/* TWI_DRV_DMA_CFG */
#define TX_TRIG_OFFSET 0
#define TX_TRIG		(0x3f << TX_TRIG_OFFSET)
#define DMA_TX_EN	(0x01 << 8)
#define RX_TRIG_OFFSET	16
#define RX_TRIG		(0x3f << RX_TRIG_OFFSET)
#define DMA_RX_EN	(0x01 << 24)
#define TWI_DRQEN_MASK	(DMA_TX_EN | DMA_RX_EN)

/* TWI_DRV_FIFO_CON */
/* the number of data in SEND_FIFO */
#define SEND_FIFO_CONTENT_OFFSET	0
#define SEND_FIFO_CONTENT	(0x3f << SEND_FIFO_CONTENT_OFFSET)
/* Set this bit to clear SEND_FIFO pointer, and this bit cleared automatically */
#define SEND_FIFO_CLEAR		(0x01 << 5)
#define RECV_FIFO_CONTENT_OFFSET	16
#define RECV_FIFO_CONTENT	(0x3f << RECV_FIFO_CONTENT_OFFSET)
#define RECV_FIFO_CLEAR		(0x01 << 22)

/* TWI_DRV_SEND_FIFO_ACC */
#define SEND_DATA_FIFO	(0xff << 0)
/* TWI_DRV_RECV_FIFO_ACC */
#define RECV_DATA_FIFO	(0xff << 0)
/* end of twi regiter offset */

#define LOOP_TIMEOUT	1024
#define TWI_FREQ_100K	100000
#define TWI_FREQ_200K	200000
#define TWI_FREQ_400K	400000
#define AUTOSUSPEND_TIMEOUT 5000
#define HEXADECIMAL		(0x10)
#define REG_INTERVAL	(0x04)
#define REG_CL			(0x0c)
#define DMA_THRESHOLD	32
#define MAX_FIFO		32
#define DMA_TIMEOUT		1000
#define TWI_READ	true
#define TWI_WRITE	false
#define TWI_ATOMIC_TIMEOUT_US	2000
#define TWI_DRV_IRQ			1
#define TWI_ENGINE_IRQ			0

#define WAIT_TIME_CHANGE_RATIO	2

#define TWI_DEBUG
unsigned int debug_chan_flags; /* Represents a global variable that allows dynamic printing */
#if defined(TWI_DEBUG)
#define TWI_DBG(twi, fmt...) {		\
	if ((0x1 << twi->bus_num) & debug_chan_flags) {     \
		sunxi_debug(twi->dev, fmt);      \
	}}
#else
#define TWI_DBG(twi, fmt...)
#endif /* endif defined(TWI_DEBUG)  */

/* twi transfer status twi->status */
enum SUNXI_TWI_XFER_STATUS {
	/* For master mode */
	SUNXI_TWI_XFER_STATUS_ERROR	= -1,
	SUNXI_TWI_XFER_STATUS_IDLE	= 0,
	SUNXI_TWI_XFER_STATUS_RUNNING,
	SUNXI_TWI_XFER_STATUS_SHUTDOWN,
	SUNXI_TWI_XFER_STATUS_COMPLETE,
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	/* For slave mode */
	SUNXI_TWI_XFER_STATUS_SLAVE_IDLE,
	SUNXI_TWI_XFER_STATUS_SLAVE_SADDR,
	SUNXI_TWI_XFER_STATUS_SLAVE_WDATA,
	SUNXI_TWI_XFER_STATUS_SLAVE_RDATA,
	SUNXI_TWI_XFER_STATUS_SLAVE_ERROR,
#endif
};

static char const *twi_status[] = {
	"idle",
	"running",
	"shutdown",
	"complete"
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	, "slave_idle",
	"slave_saddr",
	"slave_wdata",
	"slave_rdata"
#endif
	};

struct sunxi_twi_dma {
	struct dma_chan *chan;
	dma_addr_t dma_buf;
	unsigned int dma_len;
	enum dma_transfer_direction dma_transfer_dir;
	enum dma_data_direction dma_data_dir;
};

struct sunxi_twi_hw_data {
	bool has_clk_duty_30; /* duty cycle 30% of Clock as Master */
	bool slave_func_fixed; /* fixed the sda/sck issue under slave mode */
};

struct sunxi_twi {
	/* twi framework datai */
	struct i2c_adapter adap;
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	struct i2c_client *slave;
#endif
	struct platform_device *pdev;
	struct device *dev;
	struct i2c_msg *msg;
	/* the total num of msg */
	unsigned int msg_num;
	/* the current msg index -> msg[msg_idx] */
	unsigned int msg_idx;
	/* the current msg's buf data index -> msg->buf[buf_idx] */
	unsigned int buf_idx;
	/* for twi core bus lock */
	struct mutex bus_lock;

	/* dts data */
	struct resource *res;
	void __iomem *base_addr;
	struct clk *bus_clk;
	struct reset_control    *reset;
	unsigned int bus_freq;
	struct regulator *regulator;
	struct pinctrl *pctrl;
	int irq;
	int irq_flag;
	unsigned int twi_drv_used;
	unsigned int no_suspend;
	unsigned int pkt_interval;
	struct sunxi_twi_dma *dma_tx;
	struct sunxi_twi_dma *dma_rx;
	u8 *dma_buf;
	u32 vol;  /* the twi io voltage */

	/* other data */
	int bus_num;
	enum SUNXI_TWI_XFER_STATUS status; /* error, idle, running, shutdown */
	unsigned int debug_state; /* log the twi machine state */
	const struct sunxi_twi_hw_data *data;

	spinlock_t lock; /* syn */
	wait_queue_head_t wait;
	struct completion cmd_complete;

	unsigned int reg1[16]; /* store the twi engined mode resigter status */
	unsigned int reg2[16]; /* store the twi drv mode regiter status */
#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	const char *rproc_ser_name;
	char rproc_device_name[16];
	bool delay_init_done;
#endif /* CONFIG_AW_TWI_DELAYINIT */
};

#ifdef TWI_DEBUG
static void sunxi_twi_dump_reg(struct sunxi_twi *twi, u32 offset, u32 len)
{
	u32 i;
	u8 buf[64], cnt = 0;

	for (i = 0; i < len; i = i + REG_INTERVAL) {
		if (i % HEXADECIMAL == 0)
			cnt += sprintf(buf + cnt, "0x%08x: ",
				       (u32)(twi->res->start + offset + i));

		cnt += sprintf(buf + cnt, "%08x ",
			       readl(twi->base_addr + offset + i));

		if (i % HEXADECIMAL == REG_CL) {
			TWI_DBG(twi, "%s\n", buf);
			cnt = 0;
		}
	}
}
#else
static void sunxi_twi_dump_reg(struct sunxi_twi *twi, u32 offset, u32 len) { }
#endif

/* clear the interrupt flag,
 * the twi bus xfer status (register TWI_STAT) will changed as following
 */
static void sunxi_twi_engine_clear_irq(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_CNTR);

	/* start and stop bit should be 0 */
	reg_val |= INT_FLAG;
	reg_val &= ~(M_STA | M_STP);

	writel(reg_val, base_addr + TWI_CNTR);
}

/* only when get the last data, we will clear the flag when stop */
static inline void
sunxi_twi_engine_get_byte(void __iomem *base_addr, unsigned char *buffer)
{
	*buffer = (unsigned char)(TWI_DATA_MASK & readl(base_addr + TWI_DATA));
}

/* write data and clear irq flag to trigger send flow */
static void
sunxi_twi_engine_put_byte(struct sunxi_twi *twi, const unsigned char *buffer)
{
	unsigned int reg_val;

	reg_val = *buffer & TWI_DATA_MASK;
	writel(reg_val, twi->base_addr + TWI_DATA);

	TWI_DBG(twi, "engine-mode: data 0x%x xfered\n", reg_val);
}

static void sunxi_twi_engine_enable_irq(void __iomem *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CNTR);
	/*
	 * 1 when enable irq for next operation, set intflag to 0 to prevent
	 * to clear it by a mistake (intflag bit is write-1-to-clear bit)
	 * 2 Similarly, mask START bit and STOP bit to prevent to set it
	 * twice by a mistake (START bit and STOP bit are self-clear-to-0 bits)
	 */
	reg_val |= INT_EN;
	reg_val &= ~(INT_FLAG | M_STA | M_STP);
	writel(reg_val, base_addr + TWI_CNTR);
}

static void sunxi_twi_engine_disable_irq(void __iomem *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CNTR);

	reg_val &= ~INT_EN;
	reg_val &= ~(INT_FLAG | M_STA | M_STP);
	writel(reg_val, base_addr + TWI_CNTR);
}

static void sunxi_twi_bus_enable(struct sunxi_twi *twi)
{
	unsigned int reg_val;

	if (twi->twi_drv_used) {
		reg_val = readl(twi->base_addr + TWI_DRV_CTRL);
		reg_val |= TWI_DRV_EN;
		writel(reg_val, twi->base_addr + TWI_DRV_CTRL);
	} else {
		reg_val = readl(twi->base_addr + TWI_CNTR);
		reg_val |= BUS_EN;
		writel(reg_val, twi->base_addr + TWI_CNTR);
	}
}

static void sunxi_twi_bus_disable(struct sunxi_twi *twi)
{
	unsigned int reg_val;

	if (twi->twi_drv_used) {
		reg_val = readl(twi->base_addr + TWI_DRV_CTRL);
		reg_val &= ~TWI_DRV_EN;
		writel(reg_val, twi->base_addr + TWI_DRV_CTRL);
	} else {
		reg_val = readl(twi->base_addr + TWI_CNTR);
		reg_val &= ~BUS_EN;
		writel(reg_val, twi->base_addr + TWI_CNTR);
	}
}

/* trigger start signal, the start bit will be cleared automatically */
static void sunxi_twi_engine_set_start(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_CNTR);

	reg_val |= M_STA;
	reg_val &= ~(INT_FLAG);

	writel(reg_val, base_addr + TWI_CNTR);
}

/* trigger stop signal and clear int flag
 * the stop bit will be cleared automatically
 */
static void sunxi_twi_engine_set_stop(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_CNTR);

	reg_val |= M_STP;
	reg_val &= ~INT_FLAG;
	writel(reg_val, base_addr + TWI_CNTR);
}

/* get stop bit status, poll if stop signal is sent */
static inline unsigned int sunxi_twi_engine_get_stop(void __iomem *base_addr)
{
	unsigned int reg_val = readl(base_addr + TWI_CNTR);

	return reg_val & M_STP;
}

/* when sending ack or nack, it will send ack automatically */
static inline void sunxi_twi_engine_enable_ack(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_CNTR);

	reg_val |= A_ACK;

	writel(reg_val, base_addr + TWI_CNTR);
}

static inline void sunxi_twi_engine_disable_ack(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_CNTR);

	reg_val &= ~A_ACK;

	writel(reg_val, base_addr + TWI_CNTR);
}

/* check the engine-mode or drv-mode irq coming or not with irq enable or not
 * return TWI_ENGINE_IRQ on engine-mode interrupt is coming with irq is enabled
 * return TWI_DRV_IRQ on drv-mode interrupt is coming with irq is enabled
 * otherwise return the error num
 */
static unsigned int sunxi_twi_check_irq(struct sunxi_twi *twi)
{
	u32 status;

	if (twi->twi_drv_used) {
		status = readl(twi->base_addr + TWI_DRV_INT_CTRL);
		if ((status & TWI_DRV_INT_EN_MASK) && (status & TWI_DRV_INT_STA_MASK))
			return TWI_DRV_IRQ;
	} else {
		status = readl(twi->base_addr + TWI_CNTR);
		if ((status & INT_EN) && (status & INT_FLAG))
			return TWI_ENGINE_IRQ;
	}

	return -EINVAL;
}

/* get the twi controller current xfer status */
static unsigned int sunxi_twi_get_xfer_sta(struct sunxi_twi *twi)
{
	u32 status;

	if (twi->twi_drv_used) {
		status = readl(twi->base_addr + TWI_DRV_CTRL) & TWI_DRV_STAT_MASK;
		status >>=  TWI_DRV_STAT_OFFSET;
	} else {
		status = readl(twi->base_addr + TWI_STAT) & TWI_STAT_MASK;
	}

	return status;
}

/* set twi controller clock
 * clk_n: clock divider factor n
 * clk_m: clock divider factor m
 */
static void sunxi_twi_set_clock(struct sunxi_twi *twi, u8 clk_m, u8 clk_n)
{
	u32 clk_n_mask, clk_n_offset, clk_m_offset, clk_m_mask;
	u32 reg, duty, duty_30, reg_val;

	/* @IP-TODO
	 * drv-mode set clkm and clkn bit to adjust frequency, finally the
	 * TWI_DRV_BUS_CTRL register will not be changed, the value of the TWI_CCR
	 * register will represent the current drv-mode operating frequency
	 */
	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		clk_n_mask = TWI_DRV_CLK_N;
		clk_n_offset = TWI_DRV_CLK_N_OFFSET;
		clk_m_mask = TWI_DRV_CLK_M;
		clk_m_offset = TWI_DRV_CLK_M_OFFSET;
		duty = TWI_DRV_CLK_DUTY;
		duty_30 = TWI_DRV_CLK_DUTY_30;
	} else {
		reg = TWI_CCR;
		clk_n_mask = TWI_CLK_DIV_N;
		clk_n_offset = TWI_CLK_DIV_N_OFFSET;
		clk_m_mask = TWI_CLK_DIV_M;
		clk_m_offset = TWI_CLK_DIV_M_OFFSET;
		duty = TWI_CLK_DUTY;
		duty_30 = TWI_CLK_DUTY_30;
	}

	reg_val = readl(twi->base_addr + reg);

	reg_val &= ~(clk_m_mask | clk_n_mask);
	reg_val |= ((clk_m << clk_m_offset) & clk_m_mask);
	reg_val |= ((clk_n << clk_n_offset) & clk_n_mask);

	switch (twi->bus_freq) {
	case TWI_FREQ_100K:
		/* Clock duty 50% */
		reg_val &= ~(duty);
		break;
	case TWI_FREQ_200K:
		/* Clock duty 40% */
		reg_val |= duty;
		break;
	case TWI_FREQ_400K:
		if (twi->data->has_clk_duty_30)
			/* Clock duty 30% */
			reg_val |= duty_30;
		else
			/* Clock duty 40% */
			reg_val |= duty;
		break;
	default:
		sunxi_err(twi->dev, "unsupport duty with bus freq %d\n", twi->bus_freq);
	}

	writel(reg_val, twi->base_addr + reg);
}

static int sunxi_twi_set_frequency(struct sunxi_twi *twi)
{
	u8 clk_m = 0, clk_n = 0, _2_pow_clk_n = 1;
	unsigned int clk_in, clk_src, divider, clk_real;

	clk_in = clk_get_rate(twi->bus_clk);
	if (clk_in == 0) {
		sunxi_err(twi->dev, "get clock freq failed\n");
		return -EINVAL;
	}
	TWI_DBG(twi, "get clock freq is %u\n", clk_in);

	clk_src = clk_in / 10;
	divider = clk_src / twi->bus_freq;

	if (!divider) {
		clk_m = 1;
		goto twi_set_clk;
	}

	/*
	 * search clk_n and clk_m,from large to small value so
	 * that can quickly find suitable m & n.
	 */
	while (clk_n < 8) { /* 3bits max value is 8 */
		/* (m+1)*2^n = divider -->m = divider/2^n -1 */
		clk_m = (divider / _2_pow_clk_n) - 1;
		/* clk_m = (divider >> (_2_pow_clk_n>>1))-1 */
		while (clk_m < 16) { /* 4bits max value is 16 */
			/* src_clk/((m+1)*2^n) */
			clk_real = clk_src / (clk_m + 1) / _2_pow_clk_n;
			if (clk_real <= twi->bus_freq)
				goto twi_set_clk;
			else
				clk_m++;
		}
		clk_n++;
		_2_pow_clk_n *= 2; /* mutilple by 2 */
	}

twi_set_clk:
	sunxi_twi_set_clock(twi, clk_m, clk_n);

	return 0;
}

/*
 * twi controller soft_reset can only clear flag bit inside of ip, include the
 * state machine parameters, counters, various flags, fifo, fifo-cnt.
 *
 * But the internal configurations or external register configurations of ip
 * will not be changed.
 */
static void sunxi_twi_soft_reset(struct sunxi_twi *twi)
{
	u32 reg_val, reg, mask;
	int ret;

	if (twi->twi_drv_used) {
		reg = TWI_DRV_CTRL;
		mask = SOFT_RESET;
	} else {
		reg = TWI_SRST;
		mask = TWI_SOFT_RST;
	}

	reg_val = readl(twi->base_addr + reg);
	reg_val |= mask;
	writel(reg_val, twi->base_addr + reg);

	if (twi->twi_drv_used) {
		/*
		 * @IP-TODO
		 * drv-mode soft_reset bit will not clear automatically, write 0 to unreset.
		 * The reset only takes one or two CPU clk cycle.
		 */
		udelay(5);

		reg_val &= (~mask);
		writel(reg_val, twi->base_addr + reg);
	} else {
		/* Hardware will auto clear this bit when soft reset
		 * Before return, driver must wait reset opertion complete */
		ret = readl_poll_timeout_atomic(twi->base_addr + reg, reg_val, !(reg_val & mask), 5, 1000);
		if (ret) {
			sunxi_err(twi->dev, "timeout for waiting soft reset (0x%x)\n", reg_val);
			return ;
		}
	}
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static void sunxi_twi_slave_reset(struct sunxi_twi *twi)
{
	sunxi_twi_engine_clear_irq(twi->base_addr);
	sunxi_twi_engine_disable_ack(twi->base_addr);
	sunxi_twi_soft_reset(twi);
	sunxi_twi_engine_enable_ack(twi->base_addr);
	twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_IDLE;
}
#endif

/* iset the data byte number follow read command control */
static void sunxi_twi_engine_set_efr(void __iomem *base_addr, u32 efr)
{
	u32 reg_val;

	reg_val = readl(base_addr + TWI_EFR);

	reg_val &= ~TWI_EFR_MASK;
	efr &= TWI_EFR_MASK;
	reg_val |= efr;

	writel(reg_val, base_addr + TWI_EFR);
}

static int sunxi_twi_engine_stop(struct sunxi_twi *twi)
{
	int err;
	void __iomem *base_addr = twi->base_addr;

	/* After setting the M_STP position to 1 in TWI_CNTR, the hardware
	 * will automatically send a stop signal and clear the M_STP bit.
	 * The clearing action may occur before reading the register again.
	 * Therefore, no register read-back check is performed when sending
	 * the stop signal.
	 */
	sunxi_twi_engine_set_stop(base_addr);
	TWI_DBG(twi, "Send stop finish\n");
	/* twi bus xfer status will chaned after irq flag cleared */
	sunxi_twi_engine_clear_irq(base_addr);

	err = wait_field_equ((twi->base_addr + TWI_STAT), TWI_STAT_MASK,
				TWI_STAT_IDLE, TWI_ATOMIC_TIMEOUT_US);
	if (err) {
		sunxi_err(twi->dev, "engine-mode: bus state: 0x%0x, isn't idle\n",
			sunxi_twi_get_xfer_sta(twi));
		return -EINVAL;
	}

	TWI_DBG(twi, "engine-mode: stop signal xfered");
	return 0;
}

static void sunxi_twi_set_clk_count_mode(struct sunxi_twi *twi, u8 mode)
{
	u32 reg_val, reg_ctrl, reg;

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		reg_ctrl = TWI_DRV_COUNT_MODE;
		/* @IP-TODO
		 * drv-mode set TWI_DRV_BUS_CTRL register will not be changed.
		 * do not write the value that may destory the normal configure.
		 * Cause this issue, clock stretching cannot be used under drv-mode.
		 */
		return ;
	} else {
		reg = TWI_CNTR;
		reg_ctrl = CLK_COUNT_MODE;
	}

	reg_val = readl(twi->base_addr + reg);

	if (mode)
		reg_val |= reg_ctrl;
	else
		reg_val &= ~reg_ctrl;

	writel(reg_val, twi->base_addr + reg);
}

static void sunxi_twi_scl_control_enable(struct sunxi_twi *twi)
{
	u32 reg_val, reg_ctrl, reg;

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		reg_ctrl = SCL_MOE;
	} else {
		reg = TWI_LCR;
		reg_ctrl = SCL_CTL_EN;
	}

	reg_val = readl(twi->base_addr + reg);

	reg_val |= reg_ctrl;

	writel(reg_val, twi->base_addr + reg);
}

static void sunxi_twi_scl_control_disable(struct sunxi_twi *twi)
{
	u32 reg_val, reg_ctrl, reg;

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		reg_ctrl = SCL_MOE;
	} else {
		reg = TWI_LCR;
		reg_ctrl = SCL_CTL_EN;
	}

	reg_val = readl(twi->base_addr + reg);

	reg_val &= ~(reg_ctrl);

	writel(reg_val, twi->base_addr + reg);
}

static void sunxi_twi_sda_control_enable(struct sunxi_twi *twi)
{
	u32 reg_val, reg_ctrl, reg;

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		reg_ctrl = SDA_MOE;
	} else {
		reg = TWI_LCR;
		reg_ctrl = SDA_CTL_EN;
	}

	reg_val = readl(twi->base_addr + reg);

	reg_val |= reg_ctrl;

	writel(reg_val, twi->base_addr + reg);
}

static void sunxi_twi_sda_control_disable(struct sunxi_twi *twi)
{
	u32 reg_val, reg_ctrl, reg;

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		reg_ctrl = SDA_MOE;
	} else {
		reg = TWI_LCR;
		reg_ctrl = SDA_CTL_EN;
	}

	reg_val = readl(twi->base_addr + reg);

	reg_val &= ~(reg_ctrl);

	writel(reg_val, twi->base_addr + reg);
}

static int sunxi_twi_get_scl(struct i2c_adapter *adap)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adap->algo_data;

	if (twi->twi_drv_used)
		return !!(readl(twi->base_addr + TWI_DRV_BUS_CTRL) & SCL_STA);
	else
		return !!(readl(twi->base_addr + TWI_LCR) & SCL_STATE);
}

static void sunxi_twi_set_scl(struct i2c_adapter *adap, int val)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adap->algo_data;
	u32 reg_val, status, reg;

	sunxi_twi_scl_control_enable(twi);

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		status = SCL_MOV;
	} else {
		reg = TWI_LCR;
		status = SCL_CTL;
	}

	reg_val = readl(twi->base_addr + reg);
	TWI_DBG(twi, "set scl, val:%x, val:%d\n", reg_val, val);

	if (val)
		reg_val |= status;
	else
		reg_val &= ~(status);

	writel(reg_val, twi->base_addr + reg);

	sunxi_twi_scl_control_disable(twi);
}

static int sunxi_twi_get_sda(struct i2c_adapter *adap)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adap->algo_data;

	if (twi->twi_drv_used)
		return !!(readl(twi->base_addr + TWI_DRV_BUS_CTRL) & SDA_STA);
	else
		return !!(readl(twi->base_addr + TWI_LCR) & SDA_STATE);
}

static void sunxi_twi_set_sda(struct i2c_adapter *adap, int val)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adap->algo_data;
	u32 reg_val, status, reg;

	sunxi_twi_sda_control_enable(twi);

	if (twi->twi_drv_used) {
		reg = TWI_DRV_BUS_CTRL;
		status = SDA_MOV;
	} else {
		reg = TWI_LCR;
		status = SDA_CTL;
	}

	reg_val = readl(twi->base_addr + reg);
	TWI_DBG(twi, "set scl, val:%x, val:%d\n", reg_val, val);

	if (val)
		reg_val |= status;
	else
		reg_val &= ~(status);

	writel(reg_val, twi->base_addr + reg);

	sunxi_twi_sda_control_disable(twi);
}

static int sunxi_twi_get_bus_free(struct i2c_adapter *adap)
{
	return sunxi_twi_get_sda(adap);
}

/* get the irq enabled */
static unsigned int sunxi_twi_drv_get_irq(void __iomem *base_addr)
{
	unsigned int sta, irq;

	irq = (readl(base_addr + TWI_DRV_INT_CTRL) & TWI_DRV_INT_EN_MASK) >> 16;
	sta = readl(base_addr + TWI_DRV_INT_CTRL) & TWI_DRV_INT_STA_MASK;

	return irq & sta;
}

static void sunxi_twi_drv_clear_irq(void __iomem *base_addr, u32 bitmap)
{
	unsigned int reg_val = readl(base_addr + TWI_DRV_INT_CTRL);

	reg_val |= (bitmap & TWI_DRV_INT_STA_MASK);

	writel(reg_val, base_addr + TWI_DRV_INT_CTRL);
}

static void sunxi_twi_drv_enable_irq(void __iomem *base_addr, u32 bitmap)
{
	u32 reg_val = readl(base_addr + TWI_DRV_INT_CTRL);

	reg_val |= bitmap;
	reg_val &= ~TWI_DRV_INT_STA_MASK;

	writel(reg_val, base_addr + TWI_DRV_INT_CTRL);
}

static void sunxi_twi_drv_disable_irq(void __iomem *base_addr, u32 bitmap)
{
	u32 reg_val = readl(base_addr + TWI_DRV_INT_CTRL);

	reg_val &= ~bitmap;
	reg_val &= ~TWI_DRV_INT_STA_MASK;

	writel(reg_val, base_addr + TWI_DRV_INT_CTRL);
}

static void sunxi_twi_drv_enable_dma_irq(void __iomem *base_addr, u32 bitmap)
{
	u32 reg_val = readl(base_addr + TWI_DRV_DMA_CFG);

	bitmap &= TWI_DRQEN_MASK;
	reg_val |= bitmap;

	writel(reg_val, base_addr + TWI_DRV_DMA_CFG);
}

static void sunxi_twi_drv_disable_dma_irq(void __iomem *base_addr, u32 bitmap)
{
	u32 reg_val = readl(base_addr + TWI_DRV_DMA_CFG);

	bitmap &= TWI_DRQEN_MASK;
	reg_val &= ~bitmap;

	writel(reg_val, base_addr + TWI_DRV_DMA_CFG);
}

/*
 * The engine-mode bus_en bit is automatically set to 1 after the drv-mode START_TRAN bit set to 1.
 * (because drv-mode rely on engine-mode)
 */
static void sunxi_twi_drv_start_xfer(struct sunxi_twi *twi)
{
	u32 reg_val = readl(twi->base_addr + TWI_DRV_CTRL);

	reg_val |= START_TRAN;

	writel(reg_val, twi->base_addr + TWI_DRV_CTRL);

	TWI_DBG(twi, "drv-mode: start signal xfered\n");
}

static void sunxi_twi_drv_set_tx_trig(void __iomem *base_addr, u32 trig)
{
	u32 reg_val = readl(base_addr + TWI_DRV_DMA_CFG);

	reg_val &= ~TX_TRIG;
	reg_val |= (trig << TX_TRIG_OFFSET);

	writel(reg_val, base_addr + TWI_DRV_DMA_CFG);
}

/* When one of the following conditions is met:
 * 1. The number of data (in bytes) in RECV_FIFO reaches RX_TRIG;
 * 2. Packet read done and RECV_FIFO not empty.
 * If RX_REQ is enabled,  the rx-pending-bit will be set to 1 and the interrupt will be triggered;
 * If RX_REQ is disabled, the rx-pending-bit will be set to 1 but the interrupt will NOT be triggered.
 */
static void sunxi_twi_drv_set_rx_trig(void __iomem *base_addr, u32 trig)
{
	u32 reg_val;

	reg_val = readl(base_addr + TWI_DRV_DMA_CFG);

	reg_val &= ~RX_TRIG;
	reg_val |= (trig << RX_TRIG_OFFSET);

	writel(reg_val, base_addr + TWI_DRV_DMA_CFG);
}


/* bytes be send as slave device reg address */
static void sunxi_twi_drv_set_addr_byte(void __iomem *base_addr, u32 len)
{
	u32 reg_val = readl(base_addr + TWI_DRV_FMT);

	reg_val &= ~ADDR_BYTE;
	reg_val |= (len << ADDR_BYTE_OFFSET);

	writel(reg_val, base_addr + TWI_DRV_FMT);
}

/* bytes be send/received as data */
static void sunxi_twi_drv_set_data_byte(void __iomem *base_addr, u32 len)
{
	u32 val = readl(base_addr + TWI_DRV_FMT);

	val &= ~DATA_BYTE;
	val |= (len << DATA_BYTE_OFFSET);

	writel(val, base_addr + TWI_DRV_FMT);
}

/* interval between each packet in 32*Fscl cycles */
/*
static void sunxi_twi_drv_set_packet_interval(void __iomem *base_addr, u32 val)
{
	u32 reg_val = readl(base_addr + TWI_DRV_CFG);

	reg_val &= ~PACKET_INTERVAL;
	reg_val |= (val << PACKET_INTERVAL_OFFSET);

	writel(reg_val, base_addr + TWI_DRV_CFG);
}
*/

/* FIFO data be transmitted as PACKET_CNT packets in current format */
static void sunxi_twi_drv_set_packet_cnt(void __iomem *base_addr, u32 val)
{
	u32 reg_val = readl(base_addr + TWI_DRV_CFG);

	reg_val &= ~PACKET_CNT;
	reg_val |= (val << PACKET_CNT_OFFSET);

	writel(reg_val, base_addr + TWI_DRV_CFG);
}

/* do not send slave_id +W */
static void sunxi_twi_drv_enable_read_mode(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_DRV_CTRL);

	reg_val |= READ_TRAN_MODE;

	writel(reg_val, base_addr + TWI_DRV_CTRL);
}

/* send slave_id + W */
static void sunxi_twi_drv_disable_read_mode(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_DRV_CTRL);

	reg_val &= ~READ_TRAN_MODE;

	writel(reg_val, base_addr + TWI_DRV_CTRL);
}

static void
sunxi_twi_drv_set_slave_addr(struct sunxi_twi *twi, struct i2c_msg *msgs)
{
	unsigned int reg_val = 0;

	/* read, default value is write */
	if (msgs->flags & I2C_M_RD)
		reg_val |= CMD;
	else
		reg_val &= ~CMD;

	if (msgs->flags & I2C_M_TEN) {
		/* SLV_ID | CMD | SLV_ID_X */
		reg_val |= ((0x78 | ((msgs->addr >> 8) & 0x03)) << 9);
		reg_val |= (msgs->addr & 0xff);
		TWI_DBG(twi, "drv-mode: first 10bit(0x%x) xfered\n", msgs->addr);
	} else {
		reg_val |= ((msgs->addr & 0x7f) << 9);
		TWI_DBG(twi, "drv-mode: 7bits(0x%x) + r/w xfered", msgs->addr);
	}

	writel(reg_val, twi->base_addr + TWI_DRV_SLV);
}

static void sunxi_twi_drv_clear_txfifo(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_DRV_FIFO_CON);

	reg_val |= SEND_FIFO_CLEAR;

	writel(reg_val, base_addr + TWI_DRV_FIFO_CON);
}

static void sunxi_twi_drv_clear_rxfifo(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + TWI_DRV_FIFO_CON);

	reg_val |= RECV_FIFO_CLEAR;

	writel(reg_val, base_addr + TWI_DRV_FIFO_CON);
}

static int sunxi_twi_drv_send_msg(struct sunxi_twi *twi, struct i2c_msg *msg)
{
	u16 i;
	u32 reg_val;
	int ret;

	TWI_DBG(twi, "drv-mode: tx msg len is %d\n", msg->len);

	for (i = 0; i < msg->len; i++) {
		ret = readl_poll_timeout_atomic(twi->base_addr + TWI_DRV_FIFO_CON,
					reg_val, !((reg_val & SEND_FIFO_CONTENT) >= MAX_FIFO),
					5, 100);
		if (ret) {
			sunxi_err(twi->dev, "drv-mode: SEND FIFO overflow, timeout\n");
			return -EINVAL;
		}

		writeb(msg->buf[i], twi->base_addr +  TWI_DRV_SEND_FIFO_ACC);
		TWI_DBG(twi, "drv-mode: write Byte[%u]=0x%x,tx fifo len=%d\n",
			i, msg->buf[i], (reg_val & SEND_FIFO_CONTENT));
	}

	return 0;
}

static u32 sunxi_twi_drv_recv_msg(struct sunxi_twi *twi, struct i2c_msg *msg)
{
	u16 i;
	u32 reg_val;
	int ret;

	TWI_DBG(twi, "drv-mode: rx msg len is %d\n", msg->len);

	for (i = 0; i < msg->len; i++) {
		ret = readl_poll_timeout_atomic(twi->base_addr + TWI_DRV_FIFO_CON,
					reg_val, ((reg_val & RECV_FIFO_CONTENT) >> RECV_FIFO_CONTENT_OFFSET),
					5, 100);
		if (ret) {
			sunxi_err(twi->dev, "drv-mode: rerceive fifo empty. timeout\n");
			return -EINVAL;
		}

		msg->buf[i] = readb(twi->base_addr + TWI_DRV_RECV_FIFO_ACC);
		TWI_DBG(twi, "drv-mode: readb: Byte[%d] = 0x%x\n",
				i, msg->buf[i]);
	}

	return 0;
}

static void sunxi_twi_dma_callback(void *arg)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)arg;

	if (twi->dma_tx) {
		TWI_DBG(twi, "drv-mode: dma write data end\n");
	} else {
		TWI_DBG(twi, "drv-mode: dma read data end\n");
	}
}

/* request dma channel */
static int sunxi_twi_dma_request(struct sunxi_twi *twi, struct sunxi_twi_dma **_info, const char *name)
{
	*_info = kzalloc(sizeof(**_info), GFP_KERNEL);
	if (IS_ERR_OR_NULL(*_info)) {
		sunxi_err(twi->dev, "can't kzalloc dma info\n");
		return -ENOMEM;
	}

	(*_info)->chan = dma_request_chan(twi->dev, name);
	if (IS_ERR((*_info)->chan)) {
		sunxi_err(twi->dev, "can't request DMA channel\n");
		kfree(*_info);
		return PTR_ERR((*_info)->chan);
	}

	return 0;
}

static void sunxi_twi_dma_release(struct sunxi_twi *twi)
{
	if (twi->dma_tx) {
		twi->dma_tx->dma_buf = 0;
		twi->dma_tx->dma_len = 0;
		dma_release_channel(twi->dma_tx->chan);
		twi->dma_tx->chan = NULL;
		kfree(twi->dma_tx);
		twi->dma_tx = NULL;
	}

	if (twi->dma_rx) {
		twi->dma_rx->dma_buf = 0;
		twi->dma_rx->dma_len = 0;
		dma_release_channel(twi->dma_rx->chan);
		twi->dma_rx->chan = NULL;
		kfree(twi->dma_rx);
		twi->dma_rx = NULL;
	}
}

static int sunxi_twi_dma_init(struct sunxi_twi *twi, struct sunxi_twi_dma **_info, bool read)
{
	struct dma_slave_config dma_sconfig;
	struct device *chan_dev;
	struct dma_async_tx_descriptor *dma_desc;
	dma_addr_t phy_addr;
	int err;

	phy_addr = (dma_addr_t)twi->res->start;

	if (read) {
		dma_sconfig.src_addr = phy_addr + TWI_DRV_RECV_FIFO_ACC;
		dma_sconfig.direction = DMA_DEV_TO_MEM;
		(*_info)->dma_transfer_dir = DMA_DEV_TO_MEM;
		(*_info)->dma_data_dir = DMA_FROM_DEVICE;
		(*_info)->dma_len = twi->msg->len;
	} else {
		dma_sconfig.dst_addr = phy_addr + TWI_DRV_SEND_FIFO_ACC;
		dma_sconfig.direction = DMA_MEM_TO_DEV;
		(*_info)->dma_transfer_dir = DMA_MEM_TO_DEV;
		(*_info)->dma_data_dir = DMA_TO_DEVICE;
		(*_info)->dma_len = twi->msg->len;
	}

	dma_sconfig.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_sconfig.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_sconfig.src_maxburst = 16;
	dma_sconfig.dst_maxburst = 16;

	err = dmaengine_slave_config((*_info)->chan, &dma_sconfig);
	if (err < 0) {
		sunxi_err(twi->dev, "can't configure tx channel\n");
		goto err0;
	}

	chan_dev = (*_info)->chan->device->dev;

	/* kzalloc buf to store the dma xfered data */
	twi->dma_buf = i2c_get_dma_safe_msg_buf(twi->msg, DMA_THRESHOLD);
	(*_info)->dma_buf = dma_map_single(chan_dev, twi->dma_buf,
					(*_info)->dma_len, (*_info)->dma_data_dir);
	if (dma_mapping_error(chan_dev, (*_info)->dma_buf)) {
		sunxi_err(twi->dev, "DMA mapping failed\n");
		err = -EINVAL;
		goto err1;
	}

	dma_desc = dmaengine_prep_slave_single((*_info)->chan, (*_info)->dma_buf,
					       (*_info)->dma_len, (*_info)->dma_transfer_dir,
					       DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!dma_desc) {
		sunxi_err(twi->dev, "Not able to get desc for DMA xfer\n");
		err = -EINVAL;
		goto err2;
	}

	dma_desc->callback = sunxi_twi_dma_callback;
	dma_desc->callback_param = twi;
	if (dma_submit_error(dmaengine_submit(dma_desc))) {
		sunxi_err(twi->dev, "DMA submit failed\n");
		err = -EINVAL;
		goto err2;
	}

	return 0;
err2:
	dma_unmap_single(chan_dev, (*_info)->dma_buf, (*_info)->dma_len, (*_info)->dma_data_dir);
err1:
	i2c_put_dma_safe_msg_buf(twi->dma_buf, twi->msg, true);
err0:
	return err;
}

static int sunxi_twi_drv_dma_deinit(struct sunxi_twi *twi, struct sunxi_twi_dma **_info)
{
	struct device *chan_dev = (*_info)->chan->device->dev;

	dma_unmap_single(chan_dev, (*_info)->dma_buf, (*_info)->dma_len, (*_info)->dma_data_dir);
	/* free the buf kzalloc, and copy the data to twi->msg */
	i2c_put_dma_safe_msg_buf(twi->dma_buf, twi->msg, true);

	return 0;
}

/* set dma start flag, if queue, it will auto restart to transfer next queue */
static void sunxi_twi_dma_start(struct sunxi_twi *twi, struct sunxi_twi_dma **_info)
{
	dma_async_issue_pending((*_info)->chan);
	TWI_DBG(twi, "dma issue pending\n");
}

static int sunxi_twi_dma_tx_config(struct sunxi_twi *twi)
{
	int err;

	sunxi_twi_drv_set_tx_trig(twi->base_addr, MAX_FIFO / 2);
	sunxi_twi_drv_enable_dma_irq(twi->base_addr, DMA_TX_EN);

	/*
	 * Only apply for dma when there is more than 32 bytes of communication for the first time,
	 * and release dma when remove to avoid a lot of time overhead
	 */
	if (!twi->dma_tx) {
		err = sunxi_twi_dma_request(twi, &twi->dma_tx, "tx");
		if (err) {
			sunxi_err(twi->dev, "request dma_tx failed\n");
			goto err0;
		}
	}

	err = sunxi_twi_dma_init(twi, &twi->dma_tx, TWI_WRITE);
	if (err) {
		sunxi_err(twi->dev, "dma_tx xfer init failed\n");
		goto err1;
	}

	sunxi_twi_dma_start(twi, &twi->dma_tx);
	sunxi_twi_drv_start_xfer(twi);

	return 0;
err1:
	sunxi_twi_dma_release(twi);
err0:
	sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_TX_EN);
	twi->dma_tx = NULL;
	return err;
}

static int sunxi_twi_dma_rx_config(struct sunxi_twi *twi)
{
	int err;

	sunxi_twi_drv_set_rx_trig(twi->base_addr, MAX_FIFO / 2);
	sunxi_twi_drv_enable_dma_irq(twi->base_addr, DMA_RX_EN);

	/*
	 * Only apply for dma when there is more than 32 bytes of communication for the first time,
	 * and release dma when remove to avoid a lot of time overhead
	 */
	if (!twi->dma_rx) {
		err = sunxi_twi_dma_request(twi, &twi->dma_rx, "rx");
		if (err) {
			sunxi_err(twi->dev, "request dma_rx failed\n");
			goto err0;
		}
	}

	err = sunxi_twi_dma_init(twi, &twi->dma_rx, TWI_READ);
	if (err) {
		sunxi_err(twi->dev, "dma_rx xfer init failed\n");
		goto err1;
	}

	sunxi_twi_dma_start(twi, &twi->dma_rx);
	sunxi_twi_drv_start_xfer(twi);

	return 0;
err1:
	sunxi_twi_dma_release(twi);
err0:
	sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_RX_EN);
	twi->dma_rx = NULL;
	return err;
}

/* Description:
 *	7bits addr: 7-1bits addr+0 bit r/w
 *	10bits addr: 1111_11xx_xxxx_xxxx-->1111_0xx_rw,xxxx_xxxx
 *	send the 7 bits addr,or the first part of 10 bits addr
 **/
static void sunxi_twi_engine_addr_byte(struct sunxi_twi *twi)
{
	unsigned char addr = 0;
	unsigned char tmp  = 0;

	if (twi->msg[twi->msg_idx].flags & I2C_M_TEN) {
		/* 0111_10xx,ten bits address--9:8bits */
		tmp = 0x78 | (((twi->msg[twi->msg_idx].addr)>>8) & 0x03);
		addr = tmp << 1;	/* 1111_0xx0 */
		/* how about the second part of ten bits addr? */
		/* Answer: deal at twi_core_process() */
	} else {
		/* 7-1bits addr, xxxx_xxx0 */
		addr = (twi->msg[twi->msg_idx].addr & 0x7f) << 1;
	}

	/* read, default value is write */
	if (twi->msg[twi->msg_idx].flags & I2C_M_RD)
		addr |= 1;

	if (twi->msg[twi->msg_idx].flags & I2C_M_TEN) {
		TWI_DBG(twi, "first part of 10bits = 0x%x\n", addr);
	} else {
		TWI_DBG(twi, "engine-mode: 7bits+r/w = 0x%x xfered\n", addr);
	}

	/* send 7bits+r/w or the first part of 10bits */
	sunxi_twi_engine_put_byte(twi, &addr);
}

static int sunxi_twi_bus_barrier(struct i2c_adapter *adap)
{
	int i, ret;
	struct sunxi_twi *twi = (struct sunxi_twi *)adap->algo_data;

	for (i = 0; i < LOOP_TIMEOUT; i++) {
		if (sunxi_twi_get_bus_free(adap))
			return 0;

		udelay(1);
	}

	ret = i2c_recover_bus(adap);
	sunxi_twi_soft_reset(twi);

	return ret;
}

/* return 0 on success, otherwise return the negative error num */
static int sunxi_twi_drv_wait_complete(struct sunxi_twi *twi)
{
	unsigned long timeout;

	timeout = wait_event_timeout(twi->wait, twi->status != SUNXI_TWI_XFER_STATUS_RUNNING,
					twi->adap.timeout);
	if (timeout == 0) {
		sunxi_twi_dump_reg(twi, 0x200, 0x20);
		return -ETIME;
	}

	if (twi->status == SUNXI_TWI_XFER_STATUS_ERROR) {
		sunxi_twi_dump_reg(twi, 0x200, 0x20);
		return -EINVAL;
	} else if (twi->status == SUNXI_TWI_XFER_STATUS_COMPLETE) {
		TWI_DBG(twi, "drv-mode: xfer complete\n");
	} else {
		sunxi_err(twi->dev, "drv-mode: result err\n");
		return -EINVAL;
	}

	return 0;
}

/* return the xfer msgs num or the negative error num */
static int sunxi_twi_engine_wait_complete(struct sunxi_twi *twi)
{
	unsigned long timeout;
	int ret;

	timeout = wait_event_timeout(twi->wait, twi->status != SUNXI_TWI_XFER_STATUS_RUNNING,
					twi->adap.timeout);
	if (timeout == 0) {
		sunxi_twi_dump_reg(twi, 0x00, 0x20);
		return -ETIME;
	}

	if (twi->status == SUNXI_TWI_XFER_STATUS_ERROR) {
		sunxi_twi_dump_reg(twi, 0x00, 0x20);
		ret = -EINVAL;
	} else if (twi->status == SUNXI_TWI_XFER_STATUS_COMPLETE) {
		if (twi->msg_idx != twi->msg_num) {
			sunxi_err(twi->dev, "engine-mode: xfer incomplete(dev addr:0x%x\n",
				twi->msg->addr);
			ret = -EINVAL;
		} else {
			TWI_DBG(twi, "engine-mode: xfer complete\n");
			ret = twi->msg_idx;
		}
	} else {
		sunxi_err(twi->dev, "engine-mode: result err\n");
		ret = -EINVAL;
	}

	return ret;
}

static int sunxi_twi_drv_core_process(struct sunxi_twi *twi)
{
	void __iomem *base_addr = twi->base_addr;
	unsigned int irq, err_sta;

	irq = sunxi_twi_drv_get_irq(base_addr);
	sunxi_twi_drv_clear_irq(twi->base_addr, irq);
	TWI_DBG(twi, "drv-mode: the enabled irq 0x%x coming\n", irq);

	if ((irq & TRAN_COM_PD) && (irq &  RX_REQ_PD)) { /* the last RX_IRQ and COMPLETE_IRQ "simultaneous trigger" */
		if (sunxi_twi_drv_recv_msg(twi, twi->msg))
			twi->status = SUNXI_TWI_XFER_STATUS_ERROR;
		else
			twi->status = SUNXI_TWI_XFER_STATUS_COMPLETE;

		sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);
		wake_up(&twi->wait);
		return 0;
	} else if (irq & TRAN_COM_PD) { /* read or write packages is complete */
		/*
		 * current read opration not end, go on and get data by cpu or dma
		 * cpu read tiggered by RX_REQ_PD (RECV_FIFO not empty)
		 * dma read tiggered by DMA_RX_Req(RECV_FIFO not empty)
		 * */
		if (twi->msg->flags & I2C_M_RD) {
			if (twi->msg->len <= MAX_FIFO) /* cpu read */
				return 0; /* current read opration not end, go on and get data by next RX_REQ_PD */
		}

		/* dma read end or write end */
		twi->status = SUNXI_TWI_XFER_STATUS_COMPLETE; /* current write operation is end */
		sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);
		wake_up(&twi->wait);
		return 0;
	} else if (irq & RX_REQ_PD) { /* current read operation is end */
		if (sunxi_twi_drv_recv_msg(twi, twi->msg))
			twi->status = SUNXI_TWI_XFER_STATUS_ERROR;
		else
			twi->status = SUNXI_TWI_XFER_STATUS_COMPLETE;

		sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);
		wake_up(&twi->wait);
		return 0;
	} else if (irq & TRAN_ERR_PD) { /* current read/write packages trasnfer err */
		sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);

		/* The correct way to get the bus status in drv-mode is:
		 * err_sta = sunxi_twi_get_xfer_sta(twi)
		 * however, for the current TWI controller, when using drv-mode,
		 * the bus status is also obtained through the interrupt status register of engine-mode.
		 * therefore, we temporarily use the engine-mode interrupt status register
		 * to get the real bus status. this way is:
		 * err_sta = readl(twi->base_addr + TWI_STAT) & TWI_STAT_MASK
		 */
		err_sta = readl(twi->base_addr + TWI_STAT) & TWI_STAT_MASK;

		switch (err_sta) {
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x20:
			break;
		case 0x30:
			break;
		case 0x38:
			break;
		case 0x48:
			break;
		case 0x58:
			break;
		default:
			sunxi_err(twi->dev, "drv-mode: unknown error\n");
			break;
		}

		twi->status = SUNXI_TWI_XFER_STATUS_ERROR;
		wake_up(&twi->wait);
		return -err_sta;
	} else {
		sunxi_err(twi->dev, "Unknown irq 0x%x", irq);
		twi->status = SUNXI_TWI_XFER_STATUS_ERROR;
		sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);
		wake_up(&twi->wait);
		return -ENXIO;
	}
}

static int sunxi_twi_engine_core_process(struct sunxi_twi *twi)
{
	unsigned long flags;
	unsigned char state;
	unsigned char tmp;
	void __iomem *base_addr = twi->base_addr;

	sunxi_twi_engine_disable_irq(base_addr);
	state = sunxi_twi_get_xfer_sta(twi);
	TWI_DBG(twi, "engine-mode: [slave address:(0x%x),irq state:(0x%x)]\n",
		twi->msg->addr, state);

	spin_lock_irqsave(&twi->lock, flags);
	switch (state) {
	case 0xf8: /* On reset or stop the bus is idle, use only at poll method */
		goto out_break;
	case 0x08: /* A START condition has been transmitted */
	case 0x10: /* A repeated start condition has been transmitted */
		sunxi_twi_engine_addr_byte(twi);/* send slave address */
		break; /* break and goto to normal function, */
	case 0x18: /* slave_addr + write has been transmitted; ACK received */
		/* send second part of 10 bits addr, the remaining 8 bits of address */
		if (twi->msg[twi->msg_idx].flags & I2C_M_TEN) {
			tmp = twi->msg[twi->msg_idx].addr & 0xff;
			sunxi_twi_engine_put_byte(twi, &tmp);
			break;
		}
		/* for 7 bit addr, then directly send data byte--case 0xd0:  */
		/* fall through */
		fallthrough;
	case 0xd0: /* SLA+W has transmitted,ACK received! */
	case 0x28: /* then continue send data or current xfer end */
		/* send register address and the write data  */
		if (twi->buf_idx < twi->msg[twi->msg_idx].len) {
			sunxi_twi_engine_put_byte(twi,
					&(twi->msg[twi->msg_idx].buf[twi->buf_idx]));
			twi->buf_idx++;
		} else { /* the other msg */
			twi->msg_idx++;
			twi->buf_idx = 0;

			/* all the write msgs xfer success, then wakeup */
			if (twi->msg_idx == twi->msg_num) {
				goto out_success;
			} else if (twi->msg_idx < twi->msg_num) {
				/* for restart pattern, read spec, two msgs */
				sunxi_twi_engine_set_start(twi->base_addr);
				TWI_DBG(twi, "Send restart finish\n");
			} else {
				goto out_failed;
			}
		}
		break;
	/* SLA+R has been transmitted; ACK has been received, is ready to receive
	 * with Restart, needn't to send second part of 10 bits addr
	 */
	case 0x40:
		/* refer-"TWI-SPEC v2.1" */
		/* enable A_ACK need it(receive data len) more than 1. */
		if (twi->msg[twi->msg_idx].len > 1)
			sunxi_twi_engine_enable_ack(base_addr);/* then jump to case 0x50 */
		break;
	case 0x50: /* Data bytes has been received; ACK has been transmitted */
		/* receive first data byte */
		if (twi->buf_idx < twi->msg[twi->msg_idx].len) {
			/* get data then clear flag,then next data coming */
			sunxi_twi_engine_get_byte(base_addr,
					&twi->msg[twi->msg_idx].buf[twi->buf_idx]);

			/* more than 2 bytes, the last byte need not to send ACK */
			if ((twi->buf_idx + 2) == twi->msg[twi->msg_idx].len)
				sunxi_twi_engine_disable_ack(base_addr);

			twi->buf_idx++;
			break;
		} else { /* err process, the last byte should be @case 0x58 */
			goto out_failed;
		}
	case 0x58: /* Data byte has been received; NOT ACK has been transmitted */
		/* received the last byte  */
		if (twi->buf_idx == (twi->msg[twi->msg_idx].len - 1)) {
			sunxi_twi_engine_get_byte(base_addr,
					&twi->msg[twi->msg_idx].buf[twi->buf_idx]);
			twi->msg_idx++;
			twi->buf_idx = 0;

			/* all the read mags xfer succeed,wakeup the thread */
			if (twi->msg_idx == twi->msg_num) {
				goto out_success;
			} else if (twi->msg_idx < twi->msg_num) { /* repeat start */
				sunxi_twi_engine_set_start(twi->base_addr);
				TWI_DBG(twi, "Send restart finish\n");
				break;
			} else {
				goto out_failed;
			}
		} else {
			goto out_failed;
		}
	case 0xd8:
		sunxi_err(twi->dev, "second addr has transmitted, ACK not received!");
		goto out_failed;
	case 0x20:
		sunxi_err(twi->dev, "SLA+W has been transmitted; ACK not received\n");
		goto out_failed;
	case 0x30:
		sunxi_err(twi->dev, "DATA byte transmitted, ACK not receive\n");
		goto out_failed;
	case 0x38:
		sunxi_err(twi->dev, "Arbitration lost in SLA+W, SLA+R or data bytes\n");
		goto out_failed;
	case 0x48:
		sunxi_err(twi->dev, "Address + Read bit transmitted, ACK not received\n");
		goto out_failed;
	case 0x00:
		sunxi_err(twi->dev, "Bus error\n");
		goto out_failed;
	default:
		goto out_failed;
	}

	/* just for debug */
	twi->debug_state = state;

/* this time xfer is't completed,return and enable irq
 * then wait new interrupt coming to continue xfer
 */
out_break:
	sunxi_twi_engine_clear_irq(base_addr);
	sunxi_twi_engine_enable_irq(base_addr);
	spin_unlock_irqrestore(&twi->lock, flags);
	return 0;

/* xfer failed, then send stop and wakeup */
out_failed:
	if (sunxi_twi_engine_stop(twi))
		sunxi_err(twi->dev, "STOP failed!\n");

	twi->msg_idx = -state;
	twi->status = SUNXI_TWI_XFER_STATUS_ERROR;
	twi->debug_state = state;
	spin_unlock_irqrestore(&twi->lock, flags);
	wake_up(&twi->wait);
	return -state;

/* xfer success, then send wtop and wakeup */
out_success:
	if (sunxi_twi_engine_stop(twi))
		sunxi_err(twi->dev, "STOP failed!\n");

	twi->status = SUNXI_TWI_XFER_STATUS_COMPLETE;
	twi->debug_state = state;
	spin_unlock_irqrestore(&twi->lock, flags);
	wake_up(&twi->wait);
	return 0;
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static int sunxi_twi_slave_core_process(struct sunxi_twi *twi)
{
	unsigned long flags;
	unsigned char state;
	void __iomem *base_addr = twi->base_addr;
	unsigned char value;
	u32 reg_val;
	int timeout = 0x7ffff;

	state = sunxi_twi_get_xfer_sta(twi);
	TWI_DBG(twi, "slave-mode: addr(0x%x) irq(0x%x)\n", twi->slave->addr, state);

	spin_lock_irqsave(&twi->lock, flags);

	if (!twi->data->slave_func_fixed) {
		if (state == TWI_STAT_RXRS_ACK || state == TWI_STAT_SLV_TXD_ACK) {
			/* wait scl into second half cycle otherwise the 9th ack clk may shorter than normal */
			while (sunxi_twi_get_scl(&twi->adap) && --timeout)
				;
			if (timeout <= 0)
				sunxi_err(twi->dev, "slave-mode: wait scl low timeout!\n");

			reg_val = readl(twi->base_addr + TWI_LCR);
			reg_val &= ~SCL_CTL;
			writel(reg_val, twi->base_addr + TWI_LCR);
			sunxi_twi_scl_control_enable(twi);
		}
	}

	switch (state) {
	case 0x60: /* Slave address + Write bit received, ACK transmitted */
		i2c_slave_event(twi->slave, I2C_SLAVE_WRITE_REQUESTED, &value);
		twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_SADDR;
		break;
	case 0x80: /* Data byte received after slave address received, ACK transmitted */
		if (twi->status == SUNXI_TWI_XFER_STATUS_SLAVE_SADDR) {
			sunxi_twi_engine_get_byte(base_addr, &value);
			i2c_slave_event(twi->slave, I2C_SLAVE_WRITE_RECEIVED, &value);
			twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_WDATA;
		} else if (twi->status == SUNXI_TWI_XFER_STATUS_SLAVE_WDATA) {
			sunxi_twi_engine_get_byte(base_addr, &value);
			i2c_slave_event(twi->slave, I2C_SLAVE_WRITE_RECEIVED, &value);
		} else {
			twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_ERROR;
		}
		break;
	case 0xa0: /* STOP or repeated START condition received in slave mode */
		i2c_slave_event(twi->slave, I2C_SLAVE_STOP, &value);
		twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_IDLE;
		break;
	case 0xa8: /* Slave address + Read bit received, ACK transmitted */
		i2c_slave_event(twi->slave, I2C_SLAVE_READ_REQUESTED, &value);
		sunxi_twi_engine_put_byte(twi, &value);
		twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_RDATA;
		break;
	case 0xb8: /* Data byte transmitted in slave mode, ACK received */
		i2c_slave_event(twi->slave, I2C_SLAVE_READ_PROCESSED, &value);
		sunxi_twi_engine_put_byte(twi, &value);
		twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_RDATA;
		break;
	case 0xc0: /* Data byte transmitted in slave mode, ACK not received */
		i2c_slave_event(twi->slave, I2C_SLAVE_READ_REQUESTED, &value);
		sunxi_twi_engine_put_byte(twi, &value);
		i2c_slave_event(twi->slave, I2C_SLAVE_STOP, &value);
		twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_IDLE;
		break;
	default:
		sunxi_err(twi->dev, "slave-mode: addr 0x%02x error irq(0x%x) status(0x%x), need reset\n",
			twi->slave->addr, state, twi->status);
		twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_ERROR;
		break;
	}

	/* just for debug */
	twi->debug_state = state;

	if (twi->status == SUNXI_TWI_XFER_STATUS_SLAVE_ERROR)
		sunxi_twi_slave_reset(twi);

	sunxi_twi_engine_clear_irq(base_addr);

	if (!twi->data->slave_func_fixed) {
		if (state == TWI_STAT_RXRS_ACK || state == TWI_STAT_SLV_TXD_ACK) {
			/* delay 1us to fix the sda/scl reversal at same time issue */
			udelay(1);
			sunxi_twi_scl_control_disable(twi);
		}
	}


	spin_unlock_irqrestore(&twi->lock, flags);

	return 0;
}
#endif

static irqreturn_t sunxi_twi_handler(int this_irq, void *dev_id)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)dev_id;
	u32 ret = sunxi_twi_check_irq(twi);

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	if (twi->slave) {
		if (ret == TWI_ENGINE_IRQ) {
			sunxi_twi_slave_core_process(twi);
			return IRQ_HANDLED;
		} else {
			sunxi_err(twi->dev, "slave-mode: wrong irq, check irq number!!\n");
			return IRQ_NONE;
		}
	} else
#endif  /* CONFIG_I2C_SLAVE */
	{
		if (ret == TWI_ENGINE_IRQ) {
			sunxi_twi_engine_core_process(twi);
		} else if (ret == TWI_DRV_IRQ) {
			sunxi_twi_drv_core_process(twi);
		} else {
			sunxi_err(twi->dev, "master-mode: wrong irq, check irq number!!\n");
			return IRQ_NONE;
		}
	}

	return IRQ_HANDLED;
}

/* twi controller tx xfer function
 * return the xfered msgs num on success,or the negative error num on failed
 */
static int sunxi_twi_drv_tx_one_msg(struct sunxi_twi *twi, struct i2c_msg *msg)
{
	unsigned long flags;
	int ret;

	TWI_DBG(twi, "drv-mode: one-msg write slave_addr=0x%x, data_len=0x%x\n",
				msg->addr, msg->len);

	spin_lock_irqsave(&twi->lock, flags);
	twi->msg = msg;
	twi->buf_idx = 0;
	spin_unlock_irqrestore(&twi->lock, flags);

	sunxi_twi_drv_disable_read_mode(twi->base_addr);
	sunxi_twi_drv_set_slave_addr(twi, msg);
	if (msg->len == 1) {
		sunxi_twi_drv_set_addr_byte(twi->base_addr, 0);
		sunxi_twi_drv_set_data_byte(twi->base_addr, msg->len);
	} else {
		sunxi_twi_drv_set_addr_byte(twi->base_addr, 1);
		sunxi_twi_drv_set_data_byte(twi->base_addr, msg->len - 1);
	}
	sunxi_twi_drv_set_packet_cnt(twi->base_addr, 1);

	if (msg->len > MAX_FIFO) {
		TWI_DBG(twi, "drv-mode: dma write\n");
		ret = sunxi_twi_dma_tx_config(twi);
		if (ret) {
			sunxi_err(twi->dev, "dma_tx config failed\n");
			goto err_dma;
		}
	} else {
		TWI_DBG(twi, "drv-mode: cpu write\n");
		sunxi_twi_drv_set_tx_trig(twi->base_addr, msg->len);
		 /*
		  * if now fifo can't store all the msg data buf
		  * enable the TX_REQ_INT_EN, and wait TX_REQ_INT,
		  * then continue send the remaining data to fifo
		  */
		if (sunxi_twi_drv_send_msg(twi, twi->msg))
			sunxi_twi_drv_enable_irq(twi->base_addr, TX_REQ_INT_EN);

		sunxi_twi_drv_start_xfer(twi);
	}
	sunxi_twi_drv_enable_irq(twi->base_addr, TRAN_ERR_INT_EN | TRAN_COM_INT_EN);

	ret = sunxi_twi_drv_wait_complete(twi);

	if (twi->dma_tx && (msg->len > MAX_FIFO)) {
		/* check ret value only when use dma xfer */
		if (ret < 0)
			dmaengine_terminate_sync(twi->dma_tx->chan);
		sunxi_twi_drv_dma_deinit(twi, &twi->dma_tx);
		sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_TX_EN);
	}
	sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);

	return ret;

err_dma:
	sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_TX_EN);
	sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);

	return ret;
}

/* read xfer msg function
 * msg : the msg queue
 * num : the number of msg, usually is one or two:
 *
 * num = 1 : some twi slave device support one msg to read, need't reg_addr
 *
 * num = 2 : normally twi reead need two msg, msg example is as following :
 * msg[0]->flag express write, msg[0]->buf store the reg data,
 * msg[1]->flag express read, msg[1]-<buf store the data recived
 * return 0 on succes.
 */
static int
sunxi_twi_drv_rx_msgs(struct sunxi_twi *twi, struct i2c_msg *msgs, int num)
{
	struct i2c_msg *wmsg, *rmsg;
	int ret;

	if (num == 1) {
		wmsg = NULL;
		rmsg = msgs;
		TWI_DBG(twi, "drv-mode: one-msg read slave_addr=0x%x, data_len=0x%x\n",
				rmsg->addr, rmsg->len);
		sunxi_twi_drv_enable_read_mode(twi->base_addr);
		sunxi_twi_drv_set_addr_byte(twi->base_addr, 0);
	} else if (num == 2) {
		wmsg = msgs;
		rmsg = msgs + 1;
		TWI_DBG(twi, "drv-mode: two-msg read slave_addr=0x%x, data_len=0x%x\n",
				rmsg->addr,  rmsg->len);
		if (wmsg->addr != rmsg->addr) {
			sunxi_err(twi->dev, "drv-mode: two msg's addr must be the same\n");
			return -EINVAL;
		}
		sunxi_twi_drv_disable_read_mode(twi->base_addr);
		sunxi_twi_drv_set_addr_byte(twi->base_addr, wmsg->len);
	} else {
		sunxi_err(twi->dev, "twi read xfer can not transfer %d msgs once", num);
		return -EINVAL;
	}

	twi->msg = rmsg;

	sunxi_twi_drv_set_slave_addr(twi, rmsg);
	sunxi_twi_drv_set_packet_cnt(twi->base_addr, 1);
	sunxi_twi_drv_set_data_byte(twi->base_addr, rmsg->len);

	if (num == 2)  /* if (wmsg) */
		sunxi_twi_drv_send_msg(twi, wmsg);

	if (rmsg->len > MAX_FIFO) {
		TWI_DBG(twi, "drv-mode: rx msgs by dma\n");
		ret = sunxi_twi_dma_rx_config(twi);
		if (ret) {
			sunxi_err(twi->dev, "dma_rx config failed\n");
			goto err_dma;
		}
	} else {
		TWI_DBG(twi, "drv-mode: rx msgs by cpu\n");
		/* set the rx_trigger_level max to avoid the RX_REQ com before COM_REQ */
		sunxi_twi_drv_set_rx_trig(twi->base_addr, MAX_FIFO);
		sunxi_twi_drv_start_xfer(twi);
	}

	if (rmsg->len > MAX_FIFO)
		sunxi_twi_drv_enable_irq(twi->base_addr, TRAN_ERR_INT_EN | TRAN_COM_INT_EN);
	else
		sunxi_twi_drv_enable_irq(twi->base_addr, TRAN_ERR_INT_EN | TRAN_COM_INT_EN | RX_REQ_INT_EN);

	ret = sunxi_twi_drv_wait_complete(twi);

	if (twi->dma_rx && (rmsg->len > MAX_FIFO)) {
		/* check ret value only when use dma xfer */
		if (ret < 0)
			dmaengine_terminate_sync(twi->dma_rx->chan);
		sunxi_twi_drv_dma_deinit(twi, &twi->dma_rx);
		sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_RX_EN);
	}
	sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);

	return ret;

err_dma:
	sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_RX_EN);
	sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_EN_MASK);

	return ret;
}

/**
 * @twi: struct of sunxi_twi
 * @msgs: One or more messages to execute before STOP is issued to terminate
 * the operation,and each message begins with a START.
 * @num: the Number of messages to be executed.
 *
 * return the number of xfered or the negative error num
 */
static int
sunxi_twi_drv_xfer(struct sunxi_twi *twi, struct i2c_msg *msgs, int num)
{
	int i = 0;

	sunxi_twi_drv_clear_irq(twi->base_addr, TWI_DRV_INT_STA_MASK);
	sunxi_twi_drv_disable_irq(twi->base_addr, TWI_DRV_INT_STA_MASK);
	sunxi_twi_drv_disable_dma_irq(twi->base_addr, DMA_TX_EN | DMA_RX_EN);

	sunxi_twi_drv_clear_txfifo(twi->base_addr);
	sunxi_twi_drv_clear_rxfifo(twi->base_addr);

	while (i < num) {
		TWI_DBG(twi, "drv-mode: addr: 0x%x, flag:%x, len:%d\n",
			msgs[i].addr, msgs[i].flags, msgs[i].len);
		if (msgs[i].flags & I2C_M_RD) {
			/* one msg read */
			if (sunxi_twi_drv_rx_msgs(twi, &msgs[i], 1))
				return -EINVAL;
			i++;
		} else if (i + 1 < num && msgs[i + 1].flags & I2C_M_RD) {
			/* two msgs read */
			if (sunxi_twi_drv_rx_msgs(twi, &msgs[i], 2))
				return -EINVAL;
			i += 2;
		} else {
			/* one msg write */
			if (sunxi_twi_drv_tx_one_msg(twi, &msgs[i]))
				return -EINVAL;
			i++;
		}
	}

	return i;
}

static int
sunxi_twi_engine_xfer(struct sunxi_twi *twi, struct i2c_msg *msgs, int num)
{
	unsigned long flags;
	void __iomem *base_addr = twi->base_addr;

	sunxi_twi_engine_disable_ack(base_addr);
	sunxi_twi_engine_set_efr(base_addr, NO_DATA_WROTE);
	sunxi_twi_engine_clear_irq(base_addr);

	/* may conflict with xfer_complete */
	spin_lock_irqsave(&twi->lock, flags);
	twi->msg = msgs;
	twi->msg_num = num;
	twi->msg_idx = 0;
	spin_unlock_irqrestore(&twi->lock, flags);

	sunxi_twi_engine_enable_irq(base_addr);
	/* then send START signal, and needn't clear int flag */
	/* After setting the M_STA position to 1 in TWI_CNTR, the hardware
	 * will automatically send a start signal and clear the M_STA bit.
	 * The clearing action may occur before reading the register again.
	 * Therefore, no register read-back check is performed when sending
	 * the start signal.
	 */
	sunxi_twi_engine_set_start(twi->base_addr);
	TWI_DBG(twi, "Send start finish\n");

	/*
	 * sleep and wait, timeput = 5*HZ
	 * then do the transfer at function : sunxi_twi_engine_core_process
	 */
	return sunxi_twi_engine_wait_complete(twi);
}

static int
sunxi_twi_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adap->algo_data;
	int ret;
	unsigned long flags;

#if  IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	if (!twi->delay_init_done) {
		sunxi_err(twi->dev, "twi wait rpmsg and shouldn't be working anymore\n");
		return -EBUSY;
	}
#endif

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	if (twi->slave) {
		sunxi_err(twi->dev, "twi bus is in slave mode and shouldn't be using in master anymore\n");
		return -EBUSY;
	}
#endif

	if (IS_ERR_OR_NULL(msgs) || (num <= 0)) {
		sunxi_err(twi->dev, "invalid argument\n");
		return -EINVAL;
	}

	/* Set twi to RUNNING state when it is in IDLE state */
	spin_lock_irqsave(&twi->lock, flags);
	if (twi->status == SUNXI_TWI_XFER_STATUS_SHUTDOWN || twi->status == SUNXI_TWI_XFER_STATUS_RUNNING) {
		spin_unlock_irqrestore(&twi->lock, flags);
		sunxi_err(twi->dev, "twi bus is %s and shouldn't be working anymore\n", twi_status[twi->status]);
		return -EBUSY;
	} else {
		twi->status = SUNXI_TWI_XFER_STATUS_RUNNING;
		spin_unlock_irqrestore(&twi->lock, flags);
	}

	/* then the sunxi_twi_runtime_reseme() call back */
	ret = pm_runtime_get_sync(twi->dev);
	if (ret < 0)
		goto out;

	sunxi_twi_soft_reset(twi);

	ret = sunxi_twi_bus_barrier(&twi->adap);
	if (ret) {
		sunxi_err(twi->dev, "twi bus barrier failed, sda is still low!\n");
		goto out;
	}

	if (twi->twi_drv_used)
		ret = sunxi_twi_drv_xfer(twi, msgs, num);
	else
		ret = sunxi_twi_engine_xfer(twi, msgs, num);

out:
	/*
	 * xfer indicates that twi transmission is completed and is in idle
	 * state, When there is a race relationship between shutdown and the
	 * next transmission, shutdown will be responded to first.
	 */
	spin_lock_irqsave(&twi->lock, flags);
	twi->status = SUNXI_TWI_XFER_STATUS_IDLE;
	spin_unlock_irqrestore(&twi->lock, flags);

	pm_runtime_mark_last_busy(twi->dev);
	/* asnyc release to ensure other module all suspend */
	pm_runtime_put_autosuspend(twi->dev);

	return ret;
}

static unsigned int sunxi_twi_functionality(struct i2c_adapter *adap)
{
	unsigned int flag = I2C_FUNC_I2C|I2C_FUNC_10BIT_ADDR|I2C_FUNC_SMBUS_EMUL;

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	flag |= I2C_FUNC_SLAVE;
#endif

	return flag;
}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static int sunxi_twi_slave_init(struct sunxi_twi *twi)
{
	struct i2c_client *slave = twi->slave;
	unsigned char addr;

	if (slave->flags & I2C_M_TEN) {
		addr = TWI_ADDR_10_BIT_MASK |
			(((slave->addr) >> TWI_ADDR_10_BIT_SHIFT) & TWI_ADDR_10_BIT_WIDTH);
		writel(addr << TWI_ADDR_SHIFT, twi->base_addr + TWI_ADDR);
		addr = (slave->addr) & TWI_SLAVE_10_BIT_MASK;
		writel(addr, twi->base_addr + TWI_XADDR);
	} else {
		addr = (slave->addr) & TWI_SLAVE_7_BIT_MASK;
		writel(addr << TWI_ADDR_SHIFT, twi->base_addr + TWI_ADDR);
	}

	sunxi_twi_engine_enable_ack(twi->base_addr);
	sunxi_twi_engine_enable_irq(twi->base_addr);
	twi->status = SUNXI_TWI_XFER_STATUS_SLAVE_IDLE;

	return 0;
}

static void sunxi_twi_slave_exit(struct sunxi_twi *twi)
{
	sunxi_twi_engine_disable_irq(twi->base_addr);
	sunxi_twi_engine_disable_ack(twi->base_addr);

	writel(0, twi->base_addr + TWI_ADDR);
	writel(0, twi->base_addr + TWI_XADDR);
}

static int sunxi_twi_reg_slave(struct i2c_client *slave)
{
	struct sunxi_twi *twi;
	int err;

	twi = i2c_get_adapdata(slave->adapter);

	if (twi->twi_drv_used)
		return -EINVAL;

	if (twi->slave)
		return -EBUSY;

	/* Keep device active for slave address detection logic */
	err = pm_runtime_get_sync(twi->dev);
	if (err < 0)
		return err;

	twi->slave = slave;

	sunxi_twi_soft_reset(twi);

	err = sunxi_twi_slave_init(twi);
	if (err)
		goto err0;


	sunxi_info(twi->dev, "slave mode: enter addr(0x%x)\n", slave->addr);

	return 0;

err0:
	twi->slave = NULL;

	pm_runtime_mark_last_busy(twi->dev);
	pm_runtime_put_autosuspend(twi->dev);
	return err;
}

static int sunxi_twi_unreg_slave(struct i2c_client *slave)
{
	struct sunxi_twi *twi = i2c_get_adapdata(slave->adapter);

	WARN_ON(!twi->slave);

	sunxi_info(twi->dev, "slave mode: exit addr(0x%x)\n", slave->addr);

	sunxi_twi_slave_exit(twi);

	twi->slave = NULL;

	sunxi_twi_soft_reset(twi);

	pm_runtime_mark_last_busy(twi->dev);
	pm_runtime_put_autosuspend(twi->dev);

	return 0;
}
#endif

static const struct i2c_algorithm sunxi_twi_algorithm = {
	.master_xfer	  = sunxi_twi_xfer,
	.functionality	  = sunxi_twi_functionality,
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	.reg_slave        = sunxi_twi_reg_slave,
	.unreg_slave      = sunxi_twi_unreg_slave,
#endif
};

static struct i2c_bus_recovery_info sunxi_twi_bus_recovery = {
	.get_scl = sunxi_twi_get_scl,
	.set_scl = sunxi_twi_set_scl,
	.get_sda = sunxi_twi_get_sda,
	.set_sda = sunxi_twi_set_sda,
	.get_bus_free = sunxi_twi_get_bus_free,
	.recover_bus = i2c_generic_scl_recovery,
};

static void sunxi_twi_adap_lock_bus(struct i2c_adapter *adapter, unsigned int flags)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adapter->algo_data;

	mutex_lock(&twi->bus_lock);
}

static int sunxi_twi_adap_trylock_bus(struct i2c_adapter *adapter, unsigned int flags)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adapter->algo_data;
	int ret;

	ret = mutex_trylock(&twi->bus_lock);
	if (ret)
		sunxi_err(twi->dev, "trylock_bus failed!\n");

	return ret;
}

static void sunxi_twi_adap_unlock_bus(struct i2c_adapter *adapter, unsigned int flags)
{
	struct sunxi_twi *twi = (struct sunxi_twi *)adapter->algo_data;

	mutex_unlock(&twi->bus_lock);
}

static const struct i2c_lock_operations sunxi_twi_adap_lock_ops = {
	.lock_bus =    sunxi_twi_adap_lock_bus,
	.trylock_bus = sunxi_twi_adap_trylock_bus,
	.unlock_bus =  sunxi_twi_adap_unlock_bus,
};

static int sunxi_twi_regulator_request(struct sunxi_twi *twi)
{
	if (twi->regulator)
		return 0;

	twi->regulator = regulator_get(twi->dev, "twi");
	if (IS_ERR(twi->regulator)) {
		sunxi_err(twi->dev, "get supply failed!\n");
		return -EPROBE_DEFER;
	}

	return 0;
}

static int sunxi_twi_regulator_release(struct sunxi_twi *twi)
{
	if (!twi->regulator)
		return 0;

	regulator_put(twi->regulator);

	twi->regulator = NULL;

	return 0;
}

static int sunxi_twi_regulator_enable(struct sunxi_twi *twi)
{
	if (!twi->regulator)
		return 0;

	/* set output voltage to the dts config */
	if (twi->vol)
		regulator_set_voltage(twi->regulator, twi->vol, twi->vol);

	if (regulator_enable(twi->regulator)) {
		sunxi_err(twi->dev, "enable regulator failed!\n");
		return -EINVAL;
	}

	return 0;
}

static int sunxi_twi_regulator_disable(struct sunxi_twi *twi)
{
	if (!twi->regulator)
		return 0;

	if (regulator_is_enabled(twi->regulator))
		regulator_disable(twi->regulator);

	return 0;
}

static int sunxi_twi_clk_request(struct sunxi_twi *twi)
{
	twi->bus_clk = devm_clk_get(twi->dev, "bus");
	if (IS_ERR(twi->bus_clk)) {
		sunxi_err(twi->dev, "request clock failed\n");
		return -EINVAL;
	}

	twi->reset = devm_reset_control_get(twi->dev, NULL);
	if (IS_ERR_OR_NULL(twi->reset)) {
		sunxi_err(twi->dev, "request reset failed\n");
		return -EINVAL;
	}
	return 0;
}

static int sunxi_twi_resource_get(struct device_node *np, struct sunxi_twi *twi)
{
	int err;

	twi->bus_num = of_alias_get_id(np, "twi");
	if (twi->bus_num < 0) {
		sunxi_err(twi->dev, "TWI failed to get alias id\n");
		return -EINVAL;
	}
	twi->pdev->id = twi->bus_num;

	twi->res = platform_get_resource(twi->pdev, IORESOURCE_MEM, 0);
	if (!twi->res) {
		sunxi_err(twi->dev, "failed to get MEM res\n");
		return -ENXIO;
	}
	twi->base_addr = devm_ioremap_resource(twi->dev, twi->res);
	if (IS_ERR(twi->base_addr)) {
		sunxi_err(twi->dev, "unable to ioremap\n");
		return PTR_ERR(twi->base_addr);
	}

	err = sunxi_twi_clk_request(twi);
	if (err) {
		sunxi_err(twi->dev, "request twi clk failed!\n");
		return err;
	}

	twi->pctrl = devm_pinctrl_get(twi->dev);
	if (IS_ERR(twi->pctrl)) {
		sunxi_err(twi->dev, "pinctrl_get failed\n");
		return PTR_ERR(twi->pctrl);
	}

	twi->no_suspend = 0;
	of_property_read_u32(np, "no_suspend", &twi->no_suspend);
	twi->irq_flag = (twi->no_suspend) ? IRQF_NO_SUSPEND : 0;

	twi->irq = platform_get_irq(twi->pdev, 0);
	if (twi->irq < 0)
		return twi->irq;

	twi->vol = 0;
	of_property_read_u32(np, "twi_vol", &twi->vol);
	err = sunxi_twi_regulator_request(twi);
	if (err) {
		sunxi_err(twi->dev, "request regulator failed!\n");
		return err;
	}

	err = of_property_read_u32(np, "clock-frequency", &twi->bus_freq);
	if (err) {
		sunxi_err(twi->dev, "failed to get clock frequency\n");
		goto err0;
	}

	twi->pkt_interval = 0;
	of_property_read_u32(np, "twi_pkt_interval", &twi->pkt_interval);
	twi->twi_drv_used = 0;
	of_property_read_u32(np, "twi_drv_used", &twi->twi_drv_used);

	return 0;

err0:
	sunxi_twi_regulator_release(twi);
	return err;
}

static void sunxi_twi_resource_put(struct sunxi_twi *twi)
{
	sunxi_twi_regulator_release(twi);
}

static int sunxi_twi_select_pin_state(struct sunxi_twi *twi, char *name)
{
	int ret;
	struct pinctrl_state *pctrl_state;

	pctrl_state = pinctrl_lookup_state(twi->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		sunxi_err(twi->dev, "pinctrl_lookup_state(%s) failed! return %p\n",
			name, pctrl_state);
		return -EINVAL;
	}

	ret = pinctrl_select_state(twi->pctrl, pctrl_state);
	if (ret < 0)
		sunxi_err(twi->dev, "pinctrl select state(%s) failed! return %d\n",
				name, ret);

	return ret;
}

static int sunxi_twi_clk_init(struct sunxi_twi *twi)
{
	int err;

	/*
	 * twi will be used in the uboot stage, so it must be reset before
	 * configuring the module registers to clean up the configuration
	 * information of the uboot stage
	 */
	if (reset_control_reset(twi->reset)) {
		sunxi_err(twi->dev, "reset control deassert  failed!\n");
		return -EINVAL;
	}

	if (clk_prepare_enable(twi->bus_clk)) {
		sunxi_err(twi->dev, "enable apb_twi clock failed!\n");
		err = -EINVAL;
		goto err0;
	}

	/* twi ctroller module clock is controllerd by self */
	err = sunxi_twi_set_frequency(twi);
	if (err) {
		sunxi_err(twi->dev, "set frequency failed!\n");
		goto err1;
	}

	/* set 1 to support twi clock stretching feature */
	sunxi_twi_set_clk_count_mode(twi, 1);

	return 0;
err1:
	clk_disable_unprepare(twi->bus_clk);
err0:
	reset_control_assert(twi->reset);
	return err;
}

static void sunxi_twi_clk_exit(struct sunxi_twi *twi)
{
	/* disable clk */
	if (!IS_ERR_OR_NULL(twi->bus_clk) && __clk_is_enabled(twi->bus_clk)) {
		clk_disable_unprepare(twi->bus_clk);
	}

	if (!IS_ERR_OR_NULL(twi->reset)) {
		reset_control_assert(twi->reset);
	}
}

static int sunxi_twi_hw_init(struct sunxi_twi *twi)
{
	int err;

	err = sunxi_twi_regulator_enable(twi);
	if (err) {
		sunxi_err(twi->dev, "enable regulator failed!\n");
		goto err0;
	}

	err = sunxi_twi_select_pin_state(twi, PINCTRL_STATE_DEFAULT);
	if (err) {
		sunxi_err(twi->dev, "request twi gpio failed!\n");
		goto err1;
	}

	err = sunxi_twi_clk_init(twi);
	if (err) {
		sunxi_err(twi->dev, "init twi clock failed!\n");
		goto err2;
	}

	sunxi_twi_soft_reset(twi);

	sunxi_twi_bus_enable(twi);

	return 0;

err2:
	sunxi_twi_select_pin_state(twi, PINCTRL_STATE_SLEEP);

err1:
	sunxi_twi_regulator_disable(twi);

err0:
	return err;
}

static void sunxi_twi_hw_exit(struct sunxi_twi *twi)
{
	sunxi_twi_bus_disable(twi);

	sunxi_twi_clk_exit(twi);

	sunxi_twi_select_pin_state(twi, PINCTRL_STATE_SLEEP);

	sunxi_twi_regulator_disable(twi);
}

#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
static int sunxi_twi_rpmsg_callback(void *dev, void *data, int len)
{
	struct sunxi_twi *twi = dev;
	int err;

	if (!data || !strncmp(data, "return", len)) {
		err = devm_request_irq(twi->dev, twi->irq, sunxi_twi_handler,
				twi->irq_flag, dev_name(twi->dev), twi);
		if (err) {
			sunxi_err(twi->dev, "request irq failed!\n");
			return err;
		}
		/*
		 *  After the RTOS uses TWI, the state of the pin is still DEFAULT on the RTOS side.
		 *  When the pin state is set to DEFAULT on the linux side, it will exit directly.
		 *  Therefore, it is necessary to first set the state of the pin to SLEEP.
		 */
		sunxi_twi_select_pin_state(twi, PINCTRL_STATE_SLEEP);

		sunxi_twi_hw_init(twi);

		twi->delay_init_done = true;
		/*
		 *  when rv return the control, reopen the autosuspend funtion.
		 */
		pm_runtime_use_autosuspend(twi->dev);
	} else if (!strncmp(data, "get", len)) {
		/*
		 *  when rv get the control, disable the autosuspend funtion,
		 *  avoid the pm runtime state error.
		 */
		pm_runtime_dont_use_autosuspend(twi->dev);

		if (twi->irq)
			devm_free_irq(twi->dev, twi->irq, twi);

		sunxi_twi_hw_exit(twi);

		twi->delay_init_done = false;
	}

	return err;
}
#endif /* CONFIG_AW_TWI_DELAYINIT */

static ssize_t sunxi_twi_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE,
			"twi->bus_num = %d\n"
			"twi->name = %s\n"
			"twi->irq = %d\n"
			"twi->freqency = %u\n",
			twi->bus_num,
			dev_name(twi->dev),
			twi->irq,
			twi->bus_freq);
}

static ssize_t sunxi_twi_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);

	if (twi == NULL)
		return scnprintf(buf, PAGE_SIZE, "%s\n", "sunxi_twi is NULL!");

	return scnprintf(buf, PAGE_SIZE,
			"twi->bus_num = %d\n"
			"twi->status  = [%d] %s\n"
			"twi->msg_num   = %u, ->msg_idx = %u, ->buf_idx = %u\n"
			"twi->bus_freq  = %u\n"
			"twi->irq       = %d\n"
			"twi->debug_state = %u\n"
			"twi->base_addr = 0x%p, the TWI control register:\n"
			"[ADDR] 0x%02x = 0x%08x, [XADDR] 0x%02x = 0x%08x\n"
			"[DATA] 0x%02x = 0x%08x, [CNTR] 0x%02x = 0x%08x\n"
			"[STAT]  0x%02x = 0x%08x, [CCR]  0x%02x = 0x%08x\n"
			"[SRST] 0x%02x = 0x%08x, [EFR]   0x%02x = 0x%08x\n"
			"[LCR]  0x%02x = 0x%08x\n",
			twi->bus_num, twi->status,  twi_status[twi->status],
			twi->msg_num, twi->msg_idx, twi->buf_idx,
			twi->bus_freq, twi->irq, twi->debug_state,
			twi->base_addr,
			TWI_ADDR,	readl(twi->base_addr + TWI_ADDR),
			TWI_XADDR,	readl(twi->base_addr + TWI_XADDR),
			TWI_DATA,	readl(twi->base_addr + TWI_DATA),
			TWI_CNTR,	readl(twi->base_addr + TWI_CNTR),
			TWI_STAT,	readl(twi->base_addr + TWI_STAT),
			TWI_CCR,	readl(twi->base_addr + TWI_CCR),
			TWI_SRST,	readl(twi->base_addr + TWI_SRST),
			TWI_EFR,	readl(twi->base_addr + TWI_EFR),
			TWI_LCR,	readl(twi->base_addr + TWI_LCR));
}

static ssize_t sunxi_twi_freq_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	unsigned int freq;
	int err;

	if (twi->status != SUNXI_TWI_XFER_STATUS_IDLE) {
		sunxi_err(twi->dev, "Bus busy. Try it later.\n");
		return -EBUSY;
	}

	err = kstrtouint(buf, 10, &freq);
	if (err) {
		sunxi_err(twi->dev, "String conversion failed!\n");
		return -ERANGE;
	}

	if (freq == twi->bus_freq) {
		sunxi_info(twi->dev, "Clock frequncy already is %u\n", freq);
		return count;
	}

	switch (freq) {
	case TWI_FREQ_100K:
	case TWI_FREQ_200K:
	case TWI_FREQ_400K:
		sunxi_info(twi->dev, "Change clock frequncy from %u to %u.\n", twi->bus_freq, freq);
		sunxi_twi_adap_lock_bus(&twi->adap, I2C_LOCK_SEGMENT);
		twi->bus_freq = freq;
		err = sunxi_twi_set_frequency(twi);
		if (err) {
			sunxi_err(twi->dev, "Set frequency failed!\n");
			sunxi_twi_adap_unlock_bus(&twi->adap, I2C_LOCK_SEGMENT);
			return err;
		}
		sunxi_twi_adap_unlock_bus(&twi->adap, I2C_LOCK_SEGMENT);
		break;
	default:
		sunxi_err(twi->dev, "Invalid input %u. Should be 100000/200000/400000.\n", freq);
		break;
	}

	return count;
}

static ssize_t sunxi_twi_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "Current mode is %s\n", twi->twi_drv_used ? "1: drv" : "0: engine");
}

static ssize_t sunxi_twi_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	static char const *twi_mode[] = { "engine", "drv" };
	unsigned int set_twi_drv_used;
	int err;

	if (twi->status != SUNXI_TWI_XFER_STATUS_IDLE) {
		sunxi_err(twi->dev, "Bus busy. Try it later.\n");
		return -EBUSY;
	}

#if IS_ENABLED(CONFIG_I2C_SLAVE)
	if (twi->slave) {
		sunxi_err(twi->dev, "operation is not support in slave mode\n");
		return count;
	}
#endif

	err = kstrtouint(buf, 10, &set_twi_drv_used);
	if (err) {
		sunxi_err(twi->dev, "String conversion failed!\n");
		return -ERANGE;
	}

	if (set_twi_drv_used != 0 && set_twi_drv_used != 1) {
		sunxi_err(twi->dev, "Invalid input %u. Should be 0 or 1.\n", set_twi_drv_used);
		return -EINVAL;
	}

	if (set_twi_drv_used == twi->twi_drv_used) {
		sunxi_info(twi->dev, "Twi already in %s mode\n", twi_mode[set_twi_drv_used]);
	} else {
		sunxi_info(twi->dev, "Change twi mode from %s to %s\n", twi_mode[twi->twi_drv_used], twi_mode[set_twi_drv_used]);
		sunxi_twi_adap_lock_bus(&twi->adap, I2C_LOCK_SEGMENT);
		sunxi_twi_hw_exit(twi);
		twi->twi_drv_used = set_twi_drv_used;
		sunxi_twi_hw_init(twi);
		sunxi_twi_adap_unlock_bus(&twi->adap, I2C_LOCK_SEGMENT);
	}

	return count;
}

#if defined(TWI_DEBUG)
/*
 * If you want a twi to allow dynamic printing, just set the bit corresponding to debug_chan_flags to 1;
 *
 * | twi  | ... | twi7 | twi6 | twi5 | twi4 | twi3 | twi2 | twi1 | twi0 |
 * | bits | ... | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
 *
 * If debug_chan_flags = 0x30( bit4 = 1 and bit5 = 1), indicates that twi4 and twi5 are allowed to print
 * dynamically; When you want add twi6 dynamic printing, need to set bit6 to 1, so you should echo 0x70
 * to debug_chan sysfs node!
 */
static ssize_t sunxi_debug_chan_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "Current debug_chan_flags = 0x%x\n", debug_chan_flags);
}

static ssize_t sunxi_debug_chan_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	unsigned int set_debug_chan_flags;
	int err;

	err = kstrtouint(buf, 16, &set_debug_chan_flags);
	if (err) {
		sunxi_err(twi->dev, "String conversion failed!\n");
		return -ERANGE;
	}

	debug_chan_flags = set_debug_chan_flags;

	return count;
}
#endif

#if IS_ENABLED(CONFIG_I2C_SLAVE)
static ssize_t sunxi_twi_slave_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);

	sunxi_info(twi->dev, "twi controller soft reset\n");

	sunxi_twi_engine_disable_irq(twi->base_addr);
	disable_irq(twi->irq);
	sunxi_twi_slave_reset(twi);
	enable_irq(twi->irq);
	sunxi_twi_engine_enable_irq(twi->base_addr);

	return count;
}
#endif

static struct device_attribute sunxi_twi_debug_attr[] = {
	__ATTR(info, S_IRUGO, sunxi_twi_info_show, NULL),
	__ATTR(status, S_IRUGO, sunxi_twi_status_show, NULL),
	__ATTR(freq, S_IWUSR, NULL, sunxi_twi_freq_store),
	__ATTR(twi_mode, S_IRUGO|S_IWUSR, sunxi_twi_mode_show, sunxi_twi_mode_store),
#if defined(TWI_DEBUG)
	__ATTR(debug_chan, S_IRUGO|S_IWUSR, sunxi_debug_chan_show, sunxi_debug_chan_store),
#endif
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	__ATTR(slave_reset, S_IWUSR, NULL, sunxi_twi_slave_reset_store),
#endif
};

static void sunxi_twi_create_sysfs(struct platform_device *_pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_twi_debug_attr); i++)
		device_create_file(&_pdev->dev, &sunxi_twi_debug_attr[i]);
}

static void sunxi_twi_remove_sysfs(struct platform_device *_pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_twi_debug_attr); i++)
		device_remove_file(&_pdev->dev, &sunxi_twi_debug_attr[i]);
}

static int sunxi_twi_probe(struct platform_device *pdev)
{
	struct sunxi_twi *twi;
	int err;

	twi = devm_kzalloc(&pdev->dev, sizeof(*twi), GFP_KERNEL);
	if (IS_ERR_OR_NULL(twi))
		return -ENOMEM;  /* Do not print prompts after kzalloc errors */

	twi->pdev = pdev;
	twi->dev = &pdev->dev;

	twi->data = of_device_get_match_data(&pdev->dev);
	if (!twi->data) {
		sunxi_err(twi->dev, "TWI failed to get device data\n");
		return -ENODEV;
	}

	/* get dts resource */
	err = sunxi_twi_resource_get(pdev->dev.of_node, twi);
	if (err) {
		sunxi_err(twi->dev, "TWI failed to get resource\n");
		goto err0;
	}

	/* twi controller hardware init */
#if  IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	twi->delay_init_done = true;
	err = of_property_read_string(pdev->dev.of_node, "rproc-name", &twi->rproc_ser_name);
	if (!err) {
		/*
		 *  After the RTOS part uses TWI, it sends a signal to Linux, and then Linux calls the
		 *  sunxi_twi_rpmsg_callback() function to initialize TWI resources normally, and all
		 *  subsequent operations on TWI are completed on the Linux side.
		 */
		twi->delay_init_done = false;
		sprintf(twi->rproc_device_name, "twi%d", twi->bus_num);
		rpmsg_notify_add(twi->rproc_ser_name, twi->rproc_device_name, sunxi_twi_rpmsg_callback, twi);
	} else
#endif /* CONFIG_AW_TWI_DELAYINIT */
	{
		err = devm_request_irq(twi->dev, twi->irq, sunxi_twi_handler,
				twi->irq_flag, dev_name(twi->dev), twi);
		if (err) {
			sunxi_err(twi->dev, "request irq failed!\n");
			return err;
		}

		err = sunxi_twi_hw_init(twi);
		if (err) {
			sunxi_err(twi->dev, "hw init failed! try again!!\n");
			goto err1;
		}
	}

	spin_lock_init(&twi->lock);
	init_waitqueue_head(&twi->wait);

	sunxi_twi_create_sysfs(pdev);

	pm_runtime_set_active(twi->dev);
	if (twi->no_suspend)
		pm_runtime_get_noresume(twi->dev);
	pm_runtime_set_autosuspend_delay(twi->dev, AUTOSUSPEND_TIMEOUT);
	pm_runtime_use_autosuspend(twi->dev);
	pm_runtime_enable(twi->dev);

	if (!twi->no_suspend) {
		err = pm_runtime_get_sync(twi->dev);
		if (err < 0)
			goto err2;
	}

	scnprintf(twi->adap.name, sizeof(twi->adap.name), "SUNXI TWI(%pa)", &twi->res->start);
	mutex_init(&twi->bus_lock);
	twi->status = SUNXI_TWI_XFER_STATUS_IDLE;
	twi->adap.owner = THIS_MODULE;
	twi->adap.nr = twi->bus_num;
	twi->adap.retries = 3;
	twi->adap.timeout = 3 * HZ;
	twi->adap.class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	twi->adap.algo = &sunxi_twi_algorithm;
	twi->adap.bus_recovery_info = &sunxi_twi_bus_recovery;
	twi->adap.lock_ops = &sunxi_twi_adap_lock_ops;
	twi->adap.algo_data = twi;
	twi->adap.dev.parent = &pdev->dev;
	twi->adap.dev.of_node = pdev->dev.of_node;
	platform_set_drvdata(pdev, twi);
#if IS_ENABLED(CONFIG_I2C_SLAVE)
	i2c_set_adapdata(&twi->adap, twi);
#endif
	/*
	 * register twi adapter should be the ending of probe
	 * before register all the resouce twi controller need be ready
	 * (twi_xfer may occur at any time when register)
	 */
	err = i2c_add_numbered_adapter(&twi->adap);
	if (err) {
		sunxi_err(twi->dev, "failed to add adapter\n");
		goto err3;
	}

	if (!twi->no_suspend) {
		pm_runtime_mark_last_busy(twi->dev);
		pm_runtime_put_autosuspend(twi->dev);
	}

	debug_chan_flags = 0x0; /* Initially set to disallow dynamic printing */
	sunxi_info(twi->dev, "v%s probe success\n", SUNXI_TWI_VERSION);
	return 0;

err3:
	pm_runtime_put_noidle(twi->dev);
err2:
	pm_runtime_disable(twi->dev);
	pm_runtime_dont_use_autosuspend(twi->dev);
	pm_runtime_set_suspended(twi->dev);
	sunxi_twi_remove_sysfs(pdev);
	sunxi_twi_remove_sysfs(pdev);
#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	if (twi->delay_init_done)
#endif /* CONFIG_AW_TWI_DELAYINIT */
		sunxi_twi_hw_exit(twi);
err1:
	sunxi_twi_resource_put(twi);
err0:
	return err;
}

static int sunxi_twi_remove(struct platform_device *pdev)
{
	struct sunxi_twi *twi = platform_get_drvdata(pdev);

	i2c_del_adapter(&twi->adap);

	platform_set_drvdata(pdev, NULL);

	pm_runtime_put_noidle(twi->dev);
	pm_runtime_disable(twi->dev);
	pm_runtime_dont_use_autosuspend(twi->dev);
	pm_runtime_set_suspended(twi->dev);

	sunxi_twi_remove_sysfs(pdev);

#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	if (twi->rproc_ser_name)
		rpmsg_notify_del(twi->rproc_ser_name, twi->rproc_device_name);

	if (twi->delay_init_done)
#endif /* CONFIG_AW_TWI_DELAYINIT */
		sunxi_twi_hw_exit(twi);

	/*
	 * dma_release_channel() uses mutex_lock, it takes 7000us to release dma.
	 * Considering the performance impact, it is more appropriate to release when remove
	 */
	sunxi_twi_dma_release(twi);

	sunxi_twi_resource_put(twi);

	TWI_DBG(twi, "remove\n");

	return 0;
}

static void sunxi_twi_shutdown(struct platform_device *pdev)
{
	struct sunxi_twi *twi = platform_get_drvdata(pdev);
	unsigned long flags, timeout;

	/* The operation of checking whether the twi state is idle and set to SHUTDOWN state cannot be interrupted */
	spin_lock_irqsave(&twi->lock, flags);
	if (twi->status != SUNXI_TWI_XFER_STATUS_IDLE) {
		/* When twi is not idle, twi->lock needs to be released to ensure normal transmission */
		spin_unlock_irqrestore(&twi->lock, flags);
		timeout = wait_event_timeout(twi->wait, twi->status == SUNXI_TWI_XFER_STATUS_IDLE, WAIT_TIME_CHANGE_RATIO * twi->adap.timeout);
		if (timeout == 0)
			sunxi_err(twi->dev, "shutdown twi timeout, should be reinitialized when used in new stage\n");
		/*
		 * When this transmission is completed, the lock needs to be held
		 * immediately to prevent the next round of transmission.
		 */
		spin_lock_irqsave(&twi->lock, flags);
	}
	twi->status = SUNXI_TWI_XFER_STATUS_SHUTDOWN;
	spin_unlock_irqrestore(&twi->lock, flags);

	sunxi_info(twi->dev, "shutdown finish\n");

	return;
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_twi_runtime_suspend(struct device *dev)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);

#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	if (!twi->delay_init_done)
		return 0;
#endif /* CONFIG_AW_TWI_DELAYINIT */

	sunxi_twi_bus_disable(twi);

	sunxi_twi_clk_exit(twi);

	sunxi_twi_select_pin_state(twi, PINCTRL_STATE_SLEEP);

	TWI_DBG(twi, "runtime suspend finish\n");

	return 0;
}

static int sunxi_twi_runtime_resume(struct device *dev)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	int err;

#if IS_ENABLED(CONFIG_AW_TWI_DELAYINIT)
	if (!twi->delay_init_done)
		return 0;
#endif /* CONFIG_AW_TWI_DELAYINIT */

	err = sunxi_twi_select_pin_state(twi, PINCTRL_STATE_DEFAULT);
	if (err) {
		sunxi_err(twi->dev, "request twi gpio failed!\n");
		return err;
	}

	err = sunxi_twi_clk_init(twi);
	if (err) {
		sunxi_err(twi->dev, "init twi clock failed!\n");
		return err;
	}

	sunxi_twi_soft_reset(twi);

	sunxi_twi_bus_enable(twi);

	TWI_DBG(twi, "runtime resume finish\n");

	return 0;
}

/* fake suspend_noirq function, does nothing and is intended to pair with resume_noirq */
static int sunxi_twi_suspend_noirq(struct device *dev)
{
	return 0;
}

/* To address the issue of requiring reinitialization of the twi used by PMU in scenarios without CPUs */
static int sunxi_twi_resume_noirq(struct device *dev)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	int ret = 0;
	unsigned long flags;

	/*
	 * cpus		+ normal standby	: the twi used by pmu dont close it regulator
	 * cpus		+ supper stadby		: the twi used by pmu dont close it regulator
	 * without cpus + normal standby	: the twi used by pmu dont close it regulator
	 * without cpus + supper stadby		: the twi used by pmu will close it regulator
	 *
	 * To address the need for reinitializing the TWI used by the PMU during system wake-up
	 * when there is no CPU, and potential issues with other modules modifying the APB bus
	 * clock, the hardware resource initialization is applied to the TWI used by the PMU
	 * during wake-up.
	 */
	if (twi->no_suspend) {
		spin_lock_irqsave(&twi->lock, flags);
		twi->status = SUNXI_TWI_XFER_STATUS_RUNNING;
		spin_unlock_irqrestore(&twi->lock, flags);

		sunxi_info(twi->dev, "new resume noirq\n");
		ret = sunxi_twi_hw_init(twi);

		spin_lock_irqsave(&twi->lock, flags);
		twi->status = SUNXI_TWI_XFER_STATUS_IDLE;
		spin_unlock_irqrestore(&twi->lock, flags);
	}

	return ret;
}

static int sunxi_twi_suspend_late(struct device *dev)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	unsigned long flags, timeout;

	/* the twi used by pmu should't suspend to keep pmu tranfer success when system standby */
	if (twi->no_suspend) {
		TWI_DBG(twi, "doesn't need to suspend\n");
		return 0;
	}

	/* mark this adapter as suspended, then twi core layer will not invoke sunxi_twi_xfer() */
	i2c_mark_adapter_suspended(&twi->adap);

	/* all the twis except used by pmu must wait for the last xfer complete before suspend */
	spin_lock_irqsave(&twi->lock, flags);
	if (twi->status != SUNXI_TWI_XFER_STATUS_IDLE) {
		/* When twi is not idle, twi->lock needs to be released to ensure normal transmission */
		spin_unlock_irqrestore(&twi->lock, flags);
		timeout = wait_event_timeout(twi->wait, twi->status == SUNXI_TWI_XFER_STATUS_IDLE,
						WAIT_TIME_CHANGE_RATIO * twi->adap.timeout);
		if (timeout == 0)
			sunxi_info(twi->dev, "suspend_noirq twi timeout\n");
	} else
		spin_unlock_irqrestore(&twi->lock, flags);

	sunxi_twi_regulator_disable(twi);

	sunxi_info(twi->dev, "suspend late\n");
	return pm_runtime_force_suspend(dev);
}

static int sunxi_twi_resume_early(struct device *dev)
{
	struct sunxi_twi *twi = dev_get_drvdata(dev);
	int ret;

	/*
	 * When using AXP as the wake-up source, the TWI used by the PMU will be utilized to
	 * read the PMU's registers. In this case, this TWI does not require any configuration.
	 * Otherwise, ongoing TWI communication may be interrupted, leading to a failure in
	 * system wake-up.
	 */
	if (twi->no_suspend)
		return 0;

	ret = sunxi_twi_regulator_enable(twi);
	if (ret) {
		sunxi_err(twi->dev, "enable regulator failed!\n");
		return ret;
	}

	/*
	 * dev->power.needs_force_resume not change in sunxi_twi_suspend_late, so this is a
	 * fake resume, sunxi_twi_runtime_resume really be invoked before twi xfer
	 */
	ret = pm_runtime_force_resume(dev);

	i2c_mark_adapter_resumed(&twi->adap);

	sunxi_info(twi->dev, "resume early\n");
	return ret;
}

static const struct dev_pm_ops sunxi_twi_dev_pm_ops = {
	.suspend_noirq	 = sunxi_twi_suspend_noirq,
	.resume_noirq	 = sunxi_twi_resume_noirq,
	.suspend_late	 = sunxi_twi_suspend_late,
	.resume_early	 = sunxi_twi_resume_early,
	.runtime_suspend = sunxi_twi_runtime_suspend,
	.runtime_resume  = sunxi_twi_runtime_resume,
};

#define SUNXI_TWI_DEV_PM_OPS (&sunxi_twi_dev_pm_ops)
#else
#define SUNXI_TWI_DEV_PM_OPS NULL
#endif

static struct sunxi_twi_hw_data sunxi_twi_v100_data = {
	.has_clk_duty_30 = false,
	.slave_func_fixed = false,
};

static struct sunxi_twi_hw_data sunxi_twi_v101_data = {
	.has_clk_duty_30 = true,
	.slave_func_fixed = false,
};

static const struct of_device_id sunxi_twi_match[] = {
	/* compatible for old name format */
	{ .compatible = "allwinner,sun8i-twi", .data = &sunxi_twi_v100_data },
	{ .compatible = "allwinner,sun20i-twi", .data = &sunxi_twi_v100_data },
	{ .compatible = "allwinner,sun50i-twi", .data = &sunxi_twi_v100_data },
	{ .compatible = "allwinner,sunxi-twi-v100", .data = &sunxi_twi_v100_data },
	/* For 1885/1890 */
	{ .compatible = "allwinner,sunxi-twi-v101", .data = &sunxi_twi_v101_data },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_twi_match);

static struct platform_driver sunxi_twi_driver = {
	.probe		= sunxi_twi_probe,
	.remove		= sunxi_twi_remove,
	.shutdown	= sunxi_twi_shutdown,
	.driver		= {
		.name	= "sunxi-twi",
		.pm		= SUNXI_TWI_DEV_PM_OPS,
		.of_match_table = sunxi_twi_match,
	},
};

static int __init sunxi_twi_adap_init(void)
{
	return platform_driver_register(&sunxi_twi_driver);
}

static void __exit sunxi_twi_adap_exit(void)
{
	platform_driver_unregister(&sunxi_twi_driver);
}

#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
subsys_initcall(sunxi_twi_adap_init);
#else
subsys_initcall_sync(sunxi_twi_adap_init);
#endif  /* CONFIG_AW_TWI_DELAYINIT */
module_exit(sunxi_twi_adap_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:twi-sunxi");
MODULE_VERSION(SUNXI_TWI_VERSION);
MODULE_DESCRIPTION("SUNXI TWI Bus Driver");
MODULE_AUTHOR("pannan");
MODULE_AUTHOR("shaosidi <shaosidi@allwinnertech.com>");
