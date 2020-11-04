
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * spi_hal.h
 * date:    2012/2/12 22:34:41
 * author:  Aaron<leafy.myeh@allwinnertech.com>
 * history: V0.1
 */

#ifndef __SPI_HAL_H
#define __SPI_HAL_H

#include "../physic_common/nand_common_interface.h"
//extern __u32 SPIC_IO_BASE;
//#define SPI0_A  //PC0-CLK, PC1-CS0, PC2-MISO, PC3-MOSI :Multi2
//#define SPI0_B  //PD18-MOSI, PD19-MISO, PD20-CLK, PD21-CS0:Multi3
//#define SPI1_A  //PE7-CS0, PE8-MOSI, PE9-CLK, PE10-MISO :Multi4
//#define SPI1_B  //PA0-CS0, PA1-MOSI, PA2-CLK, PA3-MISO :Multi6
//#define SPI1_C  //PB0-CS0, PB1-MOSI, PB2-CLK, PB3-MISO :Multi6
/* run time control */
#define TEST_SPI_NO		(0)
#define SPI_DEFAULT_CLK	(40000000)
#define SPI_TX_WL		(32)
#define SPI_RX_WL		(32)
#define SPI_FIFO_SIZE	(64)
#define SPI_CLK_SRC		(1)	//0-24M, 1-PLL6
#define SPI_MCLK		(40000000)

//#define SPIC_BASE_OS	(0x1000)
#define SPI_BASE		(u8 *)(NAND_GetIOBaseAddr(0))
//#define SPI_IRQNO(_n)	(INTC_SRC_SPI0 + (_n))

#define SPI_VAR		(SPI_BASE + 0x00)
#define SPI_GCR		(SPI_BASE + 0x04)
#define SPI_TCR		(SPI_BASE + 0x08)
#define SPI_IER		(SPI_BASE + 0x10)
#define SPI_ISR		(SPI_BASE + 0x14)
#define SPI_FCR		(SPI_BASE + 0x18)
#define SPI_FSR		(SPI_BASE + 0x1c)
#define SPI_WCR		(SPI_BASE + 0x20)
#define SPI_CCR		(SPI_BASE + 0x24)
#define SPI_MBC		(SPI_BASE + 0x30)
#define SPI_MTC		(SPI_BASE + 0x34)
#define SPI_BCC		(SPI_BASE + 0x38)
#define SPI_TXD		(SPI_BASE + 0x200)
#define SPI_RXD		(SPI_BASE + 0x300)

/* bit field of registers */
#define SPI_SOFT_RST	(1U << 31)
#define SPI_TXPAUSE_EN	(1U << 7)
#define SPI_MASTER		(1U << 1)
#define SPI_ENABLE		(1U << 0)

#define SPI_EXCHANGE	(1U << 31)
#define SPI_SAMPLE_MODE	(1U << 13)
#define SPI_LSB_MODE	(1U << 12)
#define SPI_SAMPLE_CTRL	(1U << 11)
#define SPI_RAPIDS_MODE	(1U << 10)
#define SPI_DUMMY_1		(1U << 9)
#define SPI_DHB			(1U << 8)
#define SPI_SET_SS_1	(1U << 7)
#define SPI_SS_MANUAL	(1U << 6)
#define SPI_SEL_SS0		(0U << 4)
#define SPI_SEL_SS1		(1U << 4)
#define SPI_SEL_SS2		(2U << 4)
#define SPI_SEL_SS3		(3U << 4)
#define SPI_SS_N_INBST	(1U << 3)
#define SPI_SS_ACTIVE0	(1U << 2)
#define SPI_MODE0		(0U << 0)
#define SPI_MODE1		(1U << 0)
#define SPI_MODE2		(2U << 0)
#define SPI_MODE3		(3U << 0)

#define SPI_CPHA        (1U << 0)

#define SPI_SS_INT		(1U << 13)
#define SPI_TC_INT		(1U << 12)
#define SPI_TXUR_INT	(1U << 11)
#define SPI_TXOF_INT	(1U << 10)
#define SPI_RXUR_INT	(1U << 9)
#define SPI_RXOF_INT	(1U << 8)
#define SPI_TXFULL_INT	(1U << 6)
#define SPI_TXEMPT_INT	(1U << 5)
#define SPI_TXREQ_INT	(1U << 4)
#define SPI_RXFULL_INT	(1U << 2)
#define SPI_RXEMPT_INT	(1U << 1)
#define SPI_RXREQ_INT	(1U << 0)
#define SPI_ERROR_INT	(SPI_TXUR_INT|SPI_TXOF_INT|SPI_RXUR_INT|SPI_RXOF_INT)

#define SPI_TXFIFO_RST	(1U << 31)
#define SPI_TXFIFO_TST	(1U << 30)
#define SPI_TXDMAREQ_EN	(1U << 24)
#define SPI_RXFIFO_RST	(1U << 15)
#define SPI_RXFIFO_TST	(1U << 14)
#define SPI_RXDMAREQ_EN	(1U << 8)

#define SPI_MASTER_DUAL	(1U << 28)

#define SPI_NAND_READY		  (1U << 0)
#define SPI_NAND_ERASE_FAIL   (1U << 2)
#define SPI_NAND_WRITE_FAIL   (1U << 3)
#define SPI_NAND_ECC_FIRST_BIT   (4)
#define SPI_NAND_ECC_BITMAP		 (0x3)
#define SPI_NAND_QE		  (1U << 0)


#define SPI_NAND_WREN		        0x06
#define SPI_NAND_WRDI		        0x04
#define SPI_NAND_GETSR              0x0f   //get status/features
#define SPI_NAND_SETSR              0x1f   //set status/features
#define SPI_NAND_PAGE_READ          0x13
#define SPI_NAND_FAST_READ_X1		0x0b
#define SPI_NAND_READ_X1		    0x03
#define SPI_NAND_READ_X2            0x3b
#define SPI_NAND_READ_X4            0x6b
#define SPI_NAND_READ_DUAL_IO	    0xbb
#define SPI_NAND_READ_QUAD_IO	    0xeb
#define SPI_NAND_RDID		        0x9f
#define SPI_NAND_PP		        0x02
#define SPI_NAND_PP_X4	        0x32
#define SPI_NAND_RANDOM_PP		0x84
#define SPI_NAND_RANDOM_PP_X4	0x34
#define SPI_NAND_PE                 0x10   //program execute
#define SPI_NAND_BE		        0xd8   //block erase
#define SPI_NAND_RESET		        0xff

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned int *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned int *)(addr)) = (unsigned int)(v))


/* operations of spi controller */
//extern void spi_onoff(u32 spi_no, u32 onoff);
//u32 spi_cfg_mclk(u32 spi_no, u32 src, u32 mclk);
//extern void spic_set_clk(u32 spi_no, u32 clk);
//extern u32 spic_get_clk(u32 spi_no);
extern void Spic_set_master_slave(__u32 spi_no, __u32 master);
extern void Spic_sel_ss(__u32 spi_no, __u32 ssx);
extern void Spic_set_master_sample(__u32 spi_no, __u32 sample);
extern void Spic_set_rapids(__u32 spi_no, __u32 rapids);
extern void Spic_set_trans_mode(__u32 spi_no, __u32 mode);
extern void Spic_set_wait_clk(__u32 spi_no, __u32 swc, __u32 wcc);
//extern void spic_dma_onoff(u32 spi_no, u32 dma);
extern void Spic_config_io_mode(__u32 spi_no, __u32 rxmode, __u32 dbc,
				__u32 stc);
extern void Spic_set_sample(__u32 spi_no, __u32 sample);
extern void Spic_set_sample_mode(__u32 spi_no, __u32 smod);
extern void Spic_set_trans_mode(__u32 spi_no, __u32 mode);

extern __s32 Spic_init(__u32 spi_no);
extern __s32 Spic_exit(__u32 spi_no);
extern __s32 Spic_rw(__u32 spi_no, __u32 tcnt, __u8 *txbuf, __u32 rcnt,
		     __u8 *rxbuf, __u32 dummy_cnt);

/* operations of spi nor flash */
extern __s32 spi_nand_rdid(__u32 spi_no, __u32 chip, __u32 id_addr,
			   __u32 addr_cnt, __u32 id_cnt, void *id);

#endif
