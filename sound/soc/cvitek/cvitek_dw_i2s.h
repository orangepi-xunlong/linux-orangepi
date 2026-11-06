/* SPDX-License-Identifier: GPL-2.0-or-later
 * I2S driver on CVITEK CV1835
 *
 * Copyright 2018 CVITEK
 *
 * Author: EthanChen
 *
 *
 */
#ifndef __CVITEK_DW_I2S_H__
#define __CVITEK_DW_I2S_H__

#define DWI2S_FIFO_FLUSH_VAL    0x1
#define DWI2S_DEFAULT_FIFO_LEVEL    0x3

#define DMAEN_TXBLOCK_MASK 0x00020000
#define DMAEN_RXBLOCK_MASK 0x00010000
#define DMAEN_TXBLOCK 0x1 << 17
#define DMAEN_RXBLOCK 0x1 << 16
#define DMADIS_TXBLOCK 0x0 << 17
#define DMADIS_RXBLOCK 0x0 << 16

#define DW_NO_RESOLUTION   0x0
#define DW_12_RESOLUTION   0x1
#define DW_16_RESOLUTION   0x2
#define DW_20_RESOLUTION   0x3
#define DW_24_RESOLUTION   0x4
#define DW_32_RESOLUTION   0x5

#define CLK_SCLKG_MASK 0x00000007
#define CLK_WSS_MASK 0x00000018
#define SCLKG_SET(x) (((x) << 0) & CLK_SCLKG_MASK)
#define WSS_SET(x)   (((x) << 3) & CLK_WSS_MASK)

/* define value of each configuration of register I2S_INT */
#define DWI2S_INT_RXDA		0x1 << 0 /* RX FIFO data available interrupt status */
#define DWI2S_INT_RXFO		0x1 << 1 /* RX FIFO overflow interrupt status */
#define DWI2S_INT_TXFE		0x1 << 4 /* RX FIFO empty interrupt status */
#define DWI2S_INT_TXFO		0x1 << 5 /* RX FIFO overflow interrupt status */

/* common register offset */
#define DWI2S_EN                    0x00
#define DWI2S_RX_BLOCK_EN           0x04
#define DWI2S_TX_BLOCK_EN           0x08
#define DWI2S_CLK_EN                0x0c
#define DWI2S_CLK_CFG               0x10
#define DWI2S_RX_FIFO_RESET         0x14
#define DWI2S_TX_FIFO_RESET         0x18

//#define DWI2S_LRXTX0_BUFF_REG  0X20
//#define DWI2S_RRXTX0_BUFF_REG  0X24
#define DWI2S_RX0_EN                0x28
#define DWI2S_TX0_EN                0x2c
#define DWI2S_RX0_CFG_WLEN          0x30
#define DWI2S_TX0_CFG_WLEN          0x34

/*read only*/
#define DWI2S_IRQ0_STATUS           0x38
#define DWI2S_IRQ0_MASK             0x3c
#define DWI2S_RX0_OVRUN             0x40
#define DWI2S_TX0_OVRUN             0x44

/* Interrupt trigger and DMA request asserted when FIFO level is 1-16 */
#define DWI2S_RX0_CFG_FIFO_LEVEL    0x48
#define DWI2S_TX0_CFG_FIFO_LEVEL    0x4c
#define DWI2S_RX0_FIFO_FLUSH        0x50
#define DWI2S_TX0_FIFO_FLUSH        0x54

//#define DWI2S_LRXTX1_BUFF_REG  0x60
//#define DWI2S_RRXTX1_BUFF_REG  0x64
#define DWI2S_RX1_EN                0x68
#define DWI2S_TX1_EN                0x6c
#define DWI2S_RX1_CFG_WLEN          0x70
#define DWI2S_TX1_CFG_WLEN          0x74
/*read only*/
#define DWI2S_IRQ1_STATUS           0x78
#define DWI2S_IRQ1_MASK             0x7c
#define DWI2S_RX1_OVRUN             0x80
#define DWI2S_TX1_OVRUN             0x84

#define DWI2S_RX1_CFG_FIFO_LEVEL    0x88
#define DWI2S_TX1_CFG_FIFO_LEVEL    0x8c
#define DWI2S_RX1_FIFO_FLUSH        0x90
#define DWI2S_TX1_FIFO_FLUSH        0x94

#define DWI2S_RX_BLOCK_DMA          0x1c0
#define DWI2S_RESET_RX_DMA          0x1c4
#define DWI2S_TX_BLOCK_DMA          0x1c8
#define DWI2S_RESET_TX_DMA          0x1cc

#define DWI2S_COMP_PARAM2           0x1f0
#define DWI2S_COMP_PARAM1           0x1f4
#define DWI2S_COMP_VERSION          0x1f8
#define DWI2S_COMP_TYPE             0x1fc
#define DWI2S_DMA_CONTROL           0x200




#endif /*__CVITEK_DW_I2S_H__*/
