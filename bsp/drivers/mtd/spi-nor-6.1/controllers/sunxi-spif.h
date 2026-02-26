/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2021-2028 Allwinnertech Co., Ltd.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * SUNXI SPIF Register Definition
 *
 * 2021.6.24 liuyu <liuyu@allwinnertech.com>
 * 2022.5.5  lujianliang <lujianliang@allwinnertech.com>
 *    creat thie file and support sun8i of Allwinner.
 */

#ifndef _SUNXI_SPI_H_
#define _SUNXI_SPI_H_

#include <linux/mtd/mtd.h>
#include <linux/spi/spi-mem.h>
#include <linux/mtd/spi-nor.h>

#define CONFIG_SYS_CACHELINE_SIZE	(64)
#define CONFIG_SYS_MAXDATA_SIZE		(4096)

/* SPIF Registers offsets from peripheral base address */
#define SPIF_VER_REG			(0x00)	/* version number register */
#define SPIF_GC_REG			(0x04)	/* global control register */
#define SPIF_GCA_REG			(0x08)	/* global additional control register */
#define SPIF_TC_REG			(0x0C)	/* timing control register */
#define SPIF_TDS_REG			(0x10)	/* timing delay state register */
#define SPIF_INT_EN_REG			(0x14)	/* interrupt enable register */
#define SPIF_INT_STA_REG		(0x18)	/* interrupt status register */
#define SPIF_CSD_REG			(0x1C)	/* chipselect delay register */
#define SPIF_PHC_REG			(0x20)	/* trans phase configure register */
#define SPIF_TCF_REG			(0x24)	/* trans configure1 register */
#define SPIF_TCS_REG			(0x28)	/* trans configure2 register */
#define SPIF_TNM_REG			(0x2C)	/* trans number register */
#define SPIF_PS_REG			(0x30)	/* prefetch state register */
#define SPIF_PSA_REG			(0x34)	/* prefetch start address register */
#define SPIF_PEA_REG			(0x38)	/* prefetch end address register */
#define SPIF_PMA_REG			(0x3C)	/* prefetch map address register */
#define SPIF_DMA_CTL_REG		(0x40)	/* DMA control register */
#define SPIF_DSC_REG			(0x44)	/* DMA descriptor start address register */
#define SPIF_DFT_REG			(0x48)	/* DQS FIFO trigger level register */
#define SPIF_CFT_REG			(0x4C)	/* CDC FIFO trigger level register */
#define SPIF_CFS_REG			(0x50)	/* CDC FIFO status register */
#define SPIF_BAT_REG			(0x54)	/* Bit-Aligned tansfer configure register */
#define SPIF_BAC_REG			(0x58)	/* Bit-Aligned clock configuration register */
#define SPIF_TB_REG			(0x5C)	/* TX Bit register */
#define SPIF_RB_REG			(0x60)	/* RX Bit register */
#define SPIF_RXDATA_REG			(0x200)	/* prefetch RX data register */

/* SPIF global control register bit fields & masks,default value:0x0000_0100 */
#define SPIF_GC_CFG_MODE		(0x1 << 0)
#define SPIF_GC_DMA_MODE		(1)
#define SPIF_GC_CPU_MODE		(0)
#define SPIF_GC_ADDR_MAP_MODE		(0x1 << 1)
#define SPIF_GC_NMODE_EN		(0x1 << 2)
#define SPIF_GC_PMODE_EN		(0x1 << 3)
#define SPIF_GC_CPHA			(0x1 << 4)
#define SPIF_GC_CPOL			(0x1 << 5)
#define SPIF_GC_SS_MASK			(0x3 << 6) /* SPIF chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPIF_GC_CS_POL			(0x1 << 8)
#define SPIF_GC_DUMMY_BIT_POL		(0x1 << 9)
#define SPIF_GC_DQS_RX_EN		(0x1 << 10)
#define SPIF_GC_HOLD_POL		(0x1 << 12)
#define SPIF_GC_HOLD_EN			(0x1 << 13)
#define SPIF_GC_WP_POL			(0x1 << 14)
#define SPIF_GC_WP_EN			(0x1 << 15)
#define SPIF_GC_DTR_EN			(0x1 << 16)
#define SPIF_GC_RX_CFG_FBS		(0x1 << 17)
#define SPIF_GC_TX_CFG_FBS		(0x1 << 18)
#define SPIF_GC_SS_BIT_POS		(6)

//SPIF_GCR:SPIF_MODE
#define SPIF_MODE0			(0)
#define SPIF_MODE1			(SPIF_GC_CPHA)
#define SPIF_MODE2			(SPIF_GC_CPOL)
#define SPIF_MODE3			(SPIF_GC_CPHA | SPIF_GC_CPOL)
#define SPIF_MODE_MASK			(SPIF_GC_CPHA | SPIF_GC_CPOL)

#define SPIF_SCKT_DELAY_MODE		(1U << 21)
#define SPIF_DIGITAL_ANALOG_EN		(1U << 20)
#define SPIF_DIGITAL_DELAY		(16)
#define SPIF_DIGITAL_DELAY_MASK		(7U << 16)
#define SPIF_ANALOG_DL_SW_RX_EN		(1U << 6)
#define SPIF_ANALOG_DELAY		(0)
#define SPIF_ANALOG_DELAY_MASK		(0x3F << 0)

/*0x08*/
#define SPIF_CDC_RF_RST			(0x1 << 0)
#define	SPIF_CDC_WF_RST			(0x1 << 1)
#define	SPIF_DQS_RF_SRST		(0x1 << 2)
#define SPIF_RESET			(0x1 << 3)

/*0x14*/
#define SPIF_RF_RDY_INT_EN		(0x1 << 0)	/* read fifo is ready */
#define SPIF_RF_ENPTY_INT_EN		(0x1 << 1)
#define SPIF_RF_FULL_INT_EN		(0x1 << 2)
#define SPIF_WF_RDY_INT_EN		(0x1 << 4)	/* write fifo is ready */
#define SPIF_WF_ENPTY_INT_EN		(0x1 << 5)
#define SPIF_WF_FULL_INT_EN		(0x1 << 6)
#define SPIF_RF_OVF_INT_EN		(0x1 << 8)
#define SPIF_RF_UDF_INT_EN		(0x1 << 9)
#define SPIF_WF_OVF_INT_EN		(0x1 << 10)
#define SPIF_WF_UDF_INT_EN		(0x1 << 11)
#define SPIF_DMA_TRANS_DONE_EN		(0x1 << 24)
#define SPIF_PREFETCH_READ_EN		(0x1 << 25)
#define SPIF_INT_STA_READY_EN		(SPIF_RF_RDY_INT_EN | SPIF_WF_RDY_INT_EN)
#define SPIF_INT_STA_TC_EN		(SPIF_DMA_TRANS_DONE_EN | SPIF_PREFETCH_READ_EN)
#define	SPIF_INT_STA_ERR_EN		(SPIF_RF_OVF_INT_EN | SPIF_RF_UDF_INT_EN | SPIF_WF_OVF_INT_EN)

/* 0x18 */
#define	SPIF_RF_RDY			BIT(0)
#define	SPIF_RF_EMPTY			BIT(1)
#define	SPIF_RF_FULL			BIT(2)
#define	SPIF_WF_RDY			BIT(4)
#define	SPIF_WF_EMPTY			BIT(5)
#define	SPIF_WF_FULL			BIT(6)
#define SPIF_RF_OVF			BIT(8)
#define SPIF_RF_UDF			BIT(9)
#define SPIF_WF_OVF			BIT(10)
#define SPIF_WF_UDF			BIT(11)
#define	SPIF_DMA_TRANS_DONE		BIT(24)
#define	SPIF_PREFETCH_READ_BACK		BIT(25)
#define SPIF_INT_STA_READY		(SPIF_RF_READY | SPIF_WF_READY)
#define SPIF_INT_STA_TC			(SPIF_DMA_TRANS_DONE | SPIF_PREFETCH_READ_BACK)
#define	SPIF_INT_STA_ERR		(SPIF_RF_OVF | SPIF_RF_UDF | SPIF_WF_OVF)

/* 0x1c */
#define SPIF_CSSOT			(0)	/* chip select start of transfer */
#define SPIF_CSEOT			(8)	/* chip select end of transfer */
#define SPIF_CSDA			(16)	/* chip select de-assert */
#define SPIF_CSSOT_DEFAULT		(0x5)
#define SPIF_CSEOT_DEFAULT		(0x6)
#define SPIF_CSDA_DEFAULT		(0x6)

/* 0x40 */
#define SPIF_CFG_DMA_START		BIT(0)
#define SPIF_DMA_DESCRIPTOR_LEN		(4)

/* 0x48 */
#define SPIF_DQS_EMPTY_TRIG_LEVEL	(0)
#define SPIF_DQS_FULL_TRIG_LEVEL	(8)

/* 0x4c */
#define SPIF_RF_EMPTY_TRIG_LEVEL	(0)
#define SPIF_RF_FULL_TRIG_LEVEL		(8)
#define SPIF_WF_EMPTY_TRIG_LEVEL	(16)
#define SPIF_WF_FULL_TRIG_LEVEL		(24)

/* SPIF Timing Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_SAMP_DL_VAL_TX_POS		(8)
#define SPIF_SAMP_DL_VAL_RX_POS		(0)
#define SPIF_SCKR_DL_MODE_SEL		(0x1 << 20)
#define SPIF_CLK_SCKOUT_SRC_SEL		(0x1 << 26)

/* SPIF Interrupt status Register Bit Fields & Masks,default value:0x0000_0000 */
#define DMA_TRANS_DONE_INT		(0x1 << 24)

/* SPIF Trans Phase Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_RX_TRANS_EN		(0x1 << 8)
#define SPIF_TX_TRANS_EN		(0x1 << 12)
#define SPIF_DUMMY_TRANS_EN		(0x1 << 16)
#define SPIF_MODE_TRANS_EN		(0x1 << 20)
#define SPIF_ADDR_TRANS_EN		(0x1 << 24)
#define SPIF_CMD_TRANS_EN		(0x1 << 28)

/* SPIF Trans Configure2 Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_DATA_TRANS_POS		(0)
#define SPIF_MODE_TRANS_POS		(2)
#define SPIF_ADDR_TRANS_POS		(4)
#define SPIF_CMD_TRANS_POS		(8)
#define SPIF_MODE_OPCODE_POS		(16)
#define SPIF_CMD_OPCODE_POS		(24)

/* SPIF Trans Number Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_ADDR_SIZE_MODE		(0x1 << 24)
#define SPIF_DUMMY_NUM_POS		(16)
#define SPIF_DATA_NUM_POS		(0)

/* SPIF DMA Control Register Bit Fields & Masks,default value:0x0000_0000 */
#define CFG_DMA_START			(1 << 0)
#define DMA_DESCRIPTOR_LEN		(32 << 4)

/* DMA descriptor0 Bit Fields & Masks */
#define HBURST_SINGLE_TYPE		(0x0 << 4)
#define HBURST_INCR4_TYPE		(0x3 << 4)
#define HBURST_INCR8_TYPE		(0x5 << 4)
#define HBURST_INCR16_TYPE		(0x7 << 4)
#define HBURST_TYPE			(0x7 << 4)
#define DMA_RW_PROCESS			(0x1 << 1) /* 0:Read  1:Write */
#define DMA_FINISH_FLASG		(0x1 << 0) /* The Last One Descriptor */

/* DMA descriptor1 Bit Fields & Masks */
#define DMA_BLK_LEN_8B			(0x0 << 24)
#define DMA_BLK_LEN_16B			(0x1 << 24)
#define DMA_BLK_LEN_32B			(0x2 << 24)
#define DMA_BLK_LEN_64B			(0x3 << 24)
#define DMA_BLK_LEN			(0xff << 24)
#define DMA_DATA_LEN_POS		(0)
#define DMA_DATA_LEN			(0x1ffff)
#define DMA_TRANS_NUM			(0xffff)
#define DMA_TRANS_NUM_16BIT		(1 << 31)

/* DMA descriptor7 Bit Fields & Masks */
#define SPIF_DES_NORMAL_EN		(0x1 << 28)

#define SPIF_OCTAL_MODE			(8)
#define SPIF_QUAD_MODE			(4)
#define SPIF_DUEL_MODE			(2)
#define SPIF_SINGLE_MODE		(1)

#define SPIF_MIN_TRANS_NUM		(8)

#define NOR_PAGE_SIZE			(256)

#define DESC_PER_DISTRIBUTION_MAX_NUM	(16)

#define SAMP_MODE_DL_DEFAULT		(0xaaaaffff)
struct sunxi_spif_data {
	u32 min_speed_hz;
	u32 max_speed_hz;
	u32 bus_num;			/* the total num of spi chip select */
	u32 chip_select;
	u32 sample_mode;
	u32 sample_delay;
};

/*
* dma descriptor 0~3 : set the dma config
* you can set the register 0x20 0x24 0x28 0x2c to set too
* dma descriptor 4~7 : set the spif controller transfer config
*/
struct dma_descriptor {
	u32 hburst_rw_flag;
	u32 block_data_len;	   /* dma blk len and dma data len */
	u32 data_addr;		   /* data buff */
	u32 next_des_addr;	   /* the next dma desc */
	u32 trans_phase;	   /* trans en, can be instead by 0x20 */
	u32 flash_addr;		   /* the spi device's addr, can be instead by 0x24 */
	u32 cmd_mode_buswidth;	   /* opcode and trans type, can be instead by 0x28 */
	u32 addr_dummy_data_count; /* can be instead by 0x2c */
};

struct dma_desc_cache {
	struct list_head desc_list;
	struct dma_descriptor *dma_desc;
	dma_addr_t desc_phys;	/* the request dma descriptor phys addr */
};

struct sunxi_spif {
	struct platform_device *pdev;
	struct spi_nor *nor;
	struct completion done;  /* wakup another spi transfer */
	struct task_struct *task;
	struct sunxi_spif_data *data;
	spinlock_t lock;
	void __iomem *base_addr;
	struct dma_pool	*pool;
	struct dma_descriptor *dma_desc[DESC_PER_DISTRIBUTION_MAX_NUM];
	dma_addr_t desc_phys[DESC_PER_DISTRIBUTION_MAX_NUM];	/* the request dma descriptor phys addr */
	struct list_head desc_cache_list;
	u32 spif_register[24];
	int result;			/* the spi trans result */
	char dev_name[48];
	/* @cache buf (aw1886)
	 * The amount of data to be transmitted at a time is greater than 8,
	 * which is used for data cache in the read state
	 */
	char *cache_buf;
	dma_addr_t cache_phys;	/* the request dma descriptor phys addr */
	char *data_buf;
	dma_addr_t data_phys;	/* the request dma descriptor phys addr */
	char *temporary_data_cache;
	dma_addr_t temporary_data_phys;	/* the request dma descriptor phys addr */

	/* dts data */
	struct regulator *regulator;
	struct pinctrl	*pctrl;
	struct clk *pclk;  /* PLL clock */
	struct clk *mclk;  /* spi module clock */
	struct clk *bus;  /* bus clock */
	struct reset_control	*rstc;
	u32 base_addr_phy;
	u32 irq;
	u32 working_mode;
#define PREFETCH_READ_MODE	0x1
#define DQS			0x2
	char regulator_id[16];

	int (*transfer_one)(struct sunxi_spif *sspi, struct spi_mem_op *op);
};

extern void sunxi_spif_update_sample_delay_para(struct mtd_info *mtd);
extern int sunxi_spif_controller_ops(struct spi_device *spi, struct spi_transfer *t);

#define CHECK_DTR_OP(opcode)	((opcode == SPINOR_OP_READ_1_1_1_DTR || \
				opcode == SPINOR_OP_READ_1_1_1_DTR_4B || \
				opcode == SPINOR_OP_READ_1_2_2_DTR || \
				opcode == SPINOR_OP_READ_1_2_2_DTR_4B || \
				opcode == SPINOR_OP_READ_1_4_4_DTR || \
				opcode == SPINOR_OP_READ_1_4_4_DTR_4B) \
				? 1 : 0)
#define CHECK_IO_OP(opcode)	(CHECK_DTR_OP(opcode) || \
				((opcode == SPINOR_OP_READ_1_2_2 || \
				opcode == SPINOR_OP_READ_1_2_2_4B || \
				opcode == SPINOR_OP_READ_1_4_4 || \
				opcode == SPINOR_OP_READ_1_4_4_4B || \
				opcode == SPINOR_OP_READ_1_8_8 || \
				opcode == SPINOR_OP_READ_1_8_8_4B) \
				? 1 : 0))

#endif
