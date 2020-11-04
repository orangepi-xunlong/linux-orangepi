
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
 * spi_hal.c
 * date:    2012/2/12 22:34:41
 * author:  Aaron<leafy.myeh@allwinnertech.com>
 * history: V0.1
 */
//#include "include.h"
#include "nand_type_spinand.h"
#include "spic.h"
#include "spinand_drv_cfg.h"
#include "spinand_interface_for_common.h"

#define MAX_SPI_NUM 2
#if 0
struct spic_info {
	__u32 spi_no;
	__s32 irq;
	__u32 sclk;
	__u32 master;
	u8 *txbuf;
	__u32 txptr;
	__u32 txnum;
	u8 *rxbuf;
	__u32 rxptr;
	__u32 rxnum;
	__u32 usedma;
	__s32 txdma_ch;
	__s32 rxdma_ch;
	__u32 error;
	volatile __u32 done;
	volatile __u32 txdma_done;
	volatile __u32 rxdma_done;
} spicinfo[MAX_SPI_NUM];
#endif
// extern void *SPIC_IO_BASE;

#if 0
__u32 spi_cfg_mclk(__u32 spi_no, __u32 src, __u32 mclk)
{
	struct spic_info *spic = &spicinfo[spi_no];
	__u32 mclk_base = CCM_SPI_CLK_REG + 4 * spi_no;
	__u32 source_clk;
	__u32 rval;
	__u32 m, n, div;

#ifdef FPGA_PLATFORM
	spic->sclk = 24000000;
	return spic->sclk;
#else
	switch (src) {
	case 0:
		source_clk = 24000000;
		break;
	case 1:
		source_clk = ccm_get_pll_periph0_clk();
		break;
	case 2:
		source_clk = ccm_get_pll_periph1_clk();
		break;
	default:
		printk("Wrong SPI clock source :%x\n", src);
		break;
	}

	div = (source_clk + mclk - 1) / mclk;
	div = div == 0 ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		printk("Source clock is too high\n");
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	rval = (1U << 31) | (src << 24) | (n << 16) | (m - 1);
	writew(rval, mclk_base);
	spic->sclk = source_clk / (1 << n) / (m - 1);
	return spic->sclk;
#endif
}

/*
 *__u32 spi_get_mlk(__u32 spi_no)
 *{
 *	return spicinfo[spi_no].sclk;
 *}
 */
void spi_onoff(__u32 spi_no, __u32 onoff)
{
	__u32 clkid[] = {SPI_CKID};

	switch (spi_no) {
	case 0:
		//pc0~3,pc2:cs,muliti3
		gpio_set_cfg(GPIO_C(0), 4, 3);
		gpio_set_pull(GPIO_C(2), 1, 1);
		gpio_set_drv(GPIO_C(0), 4, 1);
		break;
	default:
		break;
	}

	ccm_module_reset(clkid[spi_no]);
	if (onoff)
		ccm_clock_enable(clkid[spi_no]);
	else
		ccm_clock_disable(clkid[spi_no]);
}
#endif

#if 0
void spi_irq_handler(__u32 spi_no)
{
	struct spic_info *spic = &spicinfo[spi_no];
	__u32 isr = readw(SPI_ISR);

	/* check error */
	if (isr & (SPI_TXUR_INT|SPI_TXOF_INT|SPI_RXUR_INT|SPI_RXOF_INT)) {
		printk("spi %d FIFO run error, isr 0x%x\n", spi_no, isr);
		spic->done = 1;
		spic->error = isr & (SPI_TXUR_INT|SPI_TXOF_INT|SPI_RXUR_INT|
				     SPI_RXOF_INT);
		goto out;
	}
	/* check transfer compelete */
	if (isr & SPI_TC_INT) {
		if (!spic->usedma && (isr & SPI_RXREQ_INT)) {
			__u32 rcnt = readw(SPI_FSR) & 0xff;
//			if (rcnt > spic->rxnum - spic->rxptr)
//				rcnt = spic->rxnum - spic->rxptr;
			while (rcnt) {
				spic->rxbuf[spic->rxptr++] = readb(SPI_RXD);
				rcnt--;
			}
			if (spic->rxptr != spic->rxnum)
				printk("Rx number != total number\n");
		}
		writew(SPI_ERROR_INT, SPI_IER);
		spic->done = 1;
		goto out;
	}
	if (!spic->usedma) {
		if (isr & SPI_RXREQ_INT) {
			//__u32 rcnt = SPI_RX_WL;
			__u32 rcnt = readw(SPI_FSR) & 0xff;

			if (rcnt > spic->rxnum - spic->rxptr)
				rcnt = spic->rxnum - spic->rxptr;
			while (rcnt) {
				spic->rxbuf[spic->rxptr++] = readb(SPI_RXD);
				rcnt--;
			}
		}
		if (isr & SPI_TXREQ_INT) {
			__u32 tcnt = SPI_FIFO_SIZE - SPI_TX_WL;

			if (tcnt > spic->txnum - spic->txptr)
				tcnt = spic->txnum - spic->txptr;
			while (tcnt) {
				writeb(spic->txbuf[spic->txptr++], SPI_TXD);
				tcnt--;
			}
		}
	}
out:
	writew(isr, SPI_ISR);
}

#define SPIx_IRQ_DEFINE(_n)                                                    \
	static void spi##_n##_irq_hdle(void)                                   \
	{                                                                      \
		spi_irq_handler(_n);                                           \
	}
SPIx_IRQ_DEFINE(0)
SPIx_IRQ_DEFINE(1)

static void (*spi_irq_hdle[])(void) = {
	spi0_irq_hdle,
	spi1_irq_hdle,

};
#endif

#if 0
#if 0
void spic_set_clk(__u32 spi_no, __u32 clk)
{
//	__u32 mclk = spi_get_mlk(spi_no);
	__u32 mclk = AHBCLK;
	__u32 div;
	__u32 cdr1 = 0;
	__u32 cdr2 = 0;
	__u32 cdr_sel = 0;

	div = mclk/(clk<<1)-1;

	if (mclk < (clk<<1))
		div++;

	if (mclk%(clk<<1))
		div++;

	if (div == 0) {
		cdr1 = 0;
		cdr2 = 0;
		cdr_sel = 0;
	} else if (div <= 0x100) {
		cdr1 = 0;
		cdr2 = div;
		cdr_sel = 1;
	} else {
		div = 0;
		while (mclk > clk) {
		    div++;
		    mclk >>= 1;
		}
		cdr1 = div;
		cdr2 = 0;
		cdr_sel = 0;
	}

	writel((cdr_sel<<12)|(cdr1<<8)|cdr2, SPI_CCR);
}
#else
void spic_set_clk(__u32 spi_no, __u32 clk)
{
//	__u32 mclk = spi_get_mlk(spi_no);
	__u32 mclk = AHBCLK;
	__u32 div;
	__u32 cdr1 = 0;
	__u32 cdr2 = 0;
	__u32 cdr_sel = 0;

	if (mclk < clk) {
		printk("spi %d set_clk failed,%d MHz < %d MHz\n",
			spi_no, (mclk / 1000000), (clk / 1000000));
	}

	div = mclk/(clk<<1);
	if ((mclk < (clk<<1)) && (mclk > clk))
		div++;

	if (div == 0) {
		cdr1 = 0;
		cdr2 = 0;
		cdr_sel = 0;
	} else if (div <= 0x100) {
		cdr1 = 0;
		cdr2 = div-1;
		cdr_sel = 1;
	} else {
		div = 0;
		while (mclk > clk) {
		    div++;
		    mclk >>= 1;
		}
		cdr1 = div;
		cdr2 = 0;
		cdr_sel = 0;
	}

	writew((cdr_sel<<12)|(cdr1<<8)|cdr2, SPI_CCR);
}
#endif

__u32 spic_get_clk(__u32 spi_no)
{
//	__u32 mclk = spi_get_mlk(spi_no);
	__u32 mclk = AHBCLK;
	__u32 temp;
	__u32 temp_clk;

	mclk = mclk / 1000000;
	temp = readw(SPI_CCR);
	if (temp && (1<<12)) {
		temp_clk = mclk / 2 / ((temp & 0xff) + 1);
	} else {
		temp_clk = mclk >> ((temp >> 8) & 0xf);
	}
	return temp_clk;
}
#endif

__s32 Wait_Tc_Complete(void)
{
	__u32 timeout = 0xffffff;

	while (!(readw(SPI_ISR) & (0x1 << 12))) {// wait transfer complete
		timeout--;
		if (!timeout)
			break;
	}
	if (timeout == 0) {
		PHY_ERR("TC Complete wait status timeout!\n");
		return ERR_TIMEOUT;
	}

	return 0;
}

__s32 Spic_init(__u32 spi_no)
{
	__u32 rval;
	//	__s32 irq;

	//	MEMSET(&spicinfo[spi_no], 0, sizeof(struct spic_info));

	// init pin
	if (0 != NAND_PIORequest(spi_no)) {
		PHY_ERR("request spi gpio fail!\n");
		return -1;
	} else
		PHY_DBG("request spi gpio  ok!\n");

	// SPIC_IO_BASE = NAND_GetIOBaseAddr();

	// request general dma channel
	if (0 != spinand_request_tx_dma()) {
		PHY_ERR("request tx dma fail!\n");
		return -1;
	} else
		PHY_DBG("request general tx dma channel ok!\n");

	if (0 != spinand_request_rx_dma()) {
		PHY_ERR("request rx dma fail!\n");
		return -1;
	} else
		PHY_DBG("request general rx dma channel ok!\n");

	// init clk
	NAND_ClkRequest(spi_no);
	NAND_SetClk(spi_no, 20, 20 * 2);

	rval = SPI_SOFT_RST | SPI_TXPAUSE_EN | SPI_MASTER | SPI_ENABLE;
	writew(rval, SPI_GCR);

	rval = SPI_SET_SS_1 | SPI_DHB |
	       SPI_SS_ACTIVE0; // set ss to high,discard unused burst,SPI select
			       // signal polarity(low,1=idle)
	writew(rval, SPI_TCR);

	writew(0x1000,
	       SPI_CCR); // SPI data clk = source clk / 2, Duty Ratio ¡Ö 50%
#if 0
	irq = irq_request(SPI_IRQNO(spi_no), spi_irq_hdle[spi_no]);
	if (irq < 0) {
		printk("Request spi %d irq failed\n", spi_no);
		return -1;
	}
	irq_enable(irq);
	spicinfo[spi_no].irq = irq;
#endif
	//	spicinfo[spi_no].master = 1;
	writew(SPI_TXFIFO_RST | (SPI_TX_WL << 16) | (SPI_RX_WL), SPI_FCR);
	writew(SPI_ERROR_INT, SPI_IER);

	return 0;
}

__s32 Spic_exit(__u32 spi_no)
{
	__u32 rval;
	//	__s32 irq;

	rval = readw(SPI_GCR);
	rval &= (~(SPI_SOFT_RST | SPI_MASTER | SPI_ENABLE));
	writew(rval, SPI_GCR);
#if 0
	irq = spicinfo[spi_no].irq;
	irq_disable(irq);
	irq_free(irq);
#endif

	NAND_ClkRelease(spi_no);

	spinand_releasetxdma();
	spinand_releaserxdma();

	// init pin
	NAND_PIORelease(spi_no);

	//	MEMSET(&spicinfo[spi_no], 0, sizeof(struct spic_info));

	rval = SPI_SET_SS_1 | SPI_DHB |
	       SPI_SS_ACTIVE0; // set ss to high,discard unused burst,SPI select
			       // signal polarity(low,1=idle)
	writew(rval, SPI_TCR);

	return 0;
}

void Spic_set_master_slave(__u32 spi_no, __u32 master)
{
	__u32 rval = readw(SPI_GCR) & (~(1 << 1));

	rval |= master << 1;
	writew(rval, SPI_GCR);
	//	spicinfo[spi_no].master = master;
}

void Spic_sel_ss(__u32 spi_no, __u32 ssx)
{
	__u32 rval = readw(SPI_TCR) & (~(3 << 4));

	rval |= ssx << 4;
	writew(rval, SPI_TCR);
}
// add for aw1650
void Spic_set_transmit_LSB(__u32 spi_no, __u32 tmod)
{
	__u32 rval = readw(SPI_TCR) & (~(1 << 12));

	rval |= tmod << 12;
	writew(rval, SPI_TCR);
}

void Spic_set_ss_level(__u32 spi_no, __u32 level)
{
	__u32 rval = readw(SPI_TCR) & (~(1 << 7));

	rval |= level << 7;
	writew(rval, SPI_TCR);
}

void Spic_set_rapids(__u32 spi_no, __u32 rapids)
{
	__u32 rval = readw(SPI_TCR) & (~(1 << 10));

	rval |= rapids << 10;
	writew(rval, SPI_TCR);
}

void Spic_set_sample_mode(__u32 spi_no, __u32 smod)
{
	__u32 rval = readw(SPI_TCR) & (~(1 << 13));

	rval |= smod << 13;
	writew(rval, SPI_TCR);
}

void Spic_set_sample(__u32 spi_no, __u32 sample)
{
	__u32 rval = readw(SPI_TCR) & (~(1 << 11));

	rval |= sample << 11;
	writew(rval, SPI_TCR);
}

void Spic_set_trans_mode(__u32 spi_no, __u32 mode)
{
	__u32 rval = readw(SPI_TCR) & (~(3 << 0));

	rval |= mode << 0;
	writew(rval, SPI_TCR);
}

void Spic_set_wait_clk(__u32 spi_no, __u32 swc, __u32 wcc)
{
	writew((swc << 16) | (wcc), SPI_WCR);
}
#if 0
void spic_dma_onoff(__u32 spi_no, __u32 dma)
{
	spicinfo[spi_no].usedma = dma;
}

void spic_dma_txcb(__u32 data)
{
	struct spic_info *spic = (struct spic_info *)data;

	spic->txdma_done = 1;
}

void spic_dma_rxcb(__u32 data)
{
	struct spic_info *spic = (struct spic_info *)data;

	spic->rxdma_done = 1;
}

__s32 spic_dma_config(__u32 spi_no, __u32 tx_mode, __u32 buff_addr, __u32 len)
{
	struct spic_info *spic = &spicinfo[spi_no];
	__s32 chan;
	__u32 param = 0;
	__u32 config;
	__u32 dmatype[] = {DMA_TYPE_SPI0};
	__u32 drqno_tx[] = {DMA_CFG_DST_DRQ_SPI0};
	__u32 drqno_rx[] = {DMA_CFG_SRC_DRQ_SPI0};

	if (tx_mode == 1) {
		chan = dma_request(dmatype[spi_no], spic_dma_txcb,
				   DMA_QUEUE_END_IRQFLAG, (__u32)spic);
		if (chan < 0) {
			printk("Request spi %d dma channel failed\n", spi_no);
			return -1;
		}
		spic->txdma_ch = chan;
		param = 0x10;
		config = DMA_CFG_SRC_BST1_WIDTH8 | DMA_CFG_SRC_LINEAR |
			 drqno_tx[spi_no] | DMA_CFG_DST_BST1_WIDTH8 |
			 DMA_CFG_DST_IO;

		config |= DMA_CFG_SRC_DRQ_SDRAM;

		dma_setup(chan, config, param);
		dma_enqueue(chan, (__u32)buff_addr, SPI_TXD, len);

		NAND_CleanFlushDCacheRegion(buff_addr, len);
		nand_dma_config_start(tx_mode, buff_addr, len);
	} else {
		chan = dma_request(dmatype[spi_no], spic_dma_rxcb,
				   DMA_QUEUE_END_IRQFLAG, (__u32)spic);
		if (chan < 0) {
			printk("Request spi %d dma channel failed\n", spi_no);
			return -1;
		}
		spic->rxdma_ch = chan;
		param = 0x10;
		config = DMA_CFG_DST_BST1_WIDTH8  | DMA_CFG_DST_LINEAR |
			 drqno_rx[spi_no] | DMA_CFG_SRC_BST1_WIDTH8  |
			 DMA_CFG_SRC_IO;

		config |= DMA_CFG_DST_DRQ_SDRAM;

		dma_setup(chan, config, param);
		dma_enqueue(chan, SPI_RXD, (__u32)buff_addr, len);
	}
	dma_start(chan);

//	NAND_CleanFlushDCacheRegion(buff_addr, len);
//	nand_dma_config_start( rw, nand_dma_addr[NandIndex][0], len);

	return 0;
}
#endif
void Spic_config_io_mode(__u32 spi_no, __u32 rxmode, __u32 dbc, __u32 stc)
{
	if (rxmode == 0)
		writew((dbc << 24) | (stc), SPI_BCC);
	else if (rxmode == 1)
		writew((1 << 28) | (dbc << 24) | (stc), SPI_BCC);
	else if (rxmode == 2)
		writew((1 << 29) | (dbc << 24) | (stc), SPI_BCC);
}

/*
 * spi txrx
 * _ _______ ______________
 *  |_______|/_/_/_/_/_/_/_|
 */
#if 0
__s32 spic_rw(__u32 spi_no, __u32 txlen, void *txbuff, __u32 rxlen,
	      void *rxbuff, __u32 dummy_cnt)
{
	struct spic_info *spic = &spicinfo[spi_no];
	__u32 ier;
	//__u32 tmp;
	spic->txdma_done = 0;
	spic->rxdma_done = 0;
	spic->done = 0;

	/* config transmit counters */
	ier = readl(SPI_IER)|SPI_TC_INT;  //transmit complete enable
	writel(ier, SPI_IER);
	pattern_goto(20);
	writel(txlen, SPI_MTC);
	pattern_goto(21);
	writel(txlen+rxlen+dummy_cnt, SPI_MBC);
	pattern_goto(22);
	/* start transmit */
	writel(readl(SPI_TCR)|SPI_EXCHANGE, SPI_TCR);
	pattern_goto(23);
	/* config dma */
	if (spic->usedma) {
		__u32 fcr = readl(SPI_FCR);

		if (rxlen) {
			fcr |= SPI_RXDMAREQ_EN;
			writel(fcr, SPI_FCR);
			pattern_goto(24);
			spic_dma_config(spi_no, 0, rxbuff, rxlen);
			pattern_goto(25);
		}
		if (txlen) {
		    /* flush cache */
#ifdef SYS_ENABLE_DCACHE
		    FlushDcacheRegion((void *)txbuff, txlen);
#endif
			fcr |= SPI_TXDMAREQ_EN;
			writel(fcr, SPI_FCR);
			pattern_goto(26);
			spic_dma_config(spi_no, 1, txbuff, txlen);
			pattern_goto(27);
		}

		/* wait dma done */
		while (rxlen && !spic->rxdma_done) {
		};
		while (txlen && !spic->txdma_done) {
		};
		pattern_goto(28);
		if (rxlen) {
		    /* flush cache */
#ifdef SYS_ENABLE_DCACHE
		    FlushDcacheRegion((void *)rxbuff, rxlen);
#endif
		}
		spic->rxdma_done = 0;
		spic->txdma_done = 0;
		fcr &= ~(SPI_TXDMAREQ_EN|SPI_RXDMAREQ_EN);
		writel(fcr, SPI_FCR);
		pattern_goto(29);
	} else {
		spic->txbuf = txbuff;
		spic->txptr = 0;
		spic->txnum = txlen;
		spic->rxbuf = rxbuff;
		spic->rxptr = 0;
		spic->rxnum = rxlen;
		if (rxlen)
			ier |= SPI_RXREQ_INT;
		pattern_goto(30);
		if (txlen)
			ier |= SPI_TXREQ_INT;
		pattern_goto(31);
		writel(ier, SPI_IER);
		pattern_goto(32);
	}
	pattern_goto(33);
	/* wait transfer compelete */
	while (!spic->done) {
	};
	pattern_goto(34);
	if (spic->error) {
		printk("spi %d transfer failed\n", spi_no);
		writel(readl(SPI_FCR)|SPI_TXFIFO_RST, SPI_FCR);
		pattern_goto(35);
		return -1;
	}
	pattern_goto(36);
	writel(readl(SPI_IER)&(~(SPI_RXREQ_INT|SPI_TXREQ_INT|SPI_TC_INT)),
	       SPI_IER);
	pattern_goto(39);
	return 0;
}
#else
__s32 Spic_rw(__u32 spi_no, __u32 tcnt, __u8 *txbuf, __u32 rcnt, __u8 *rxbuf,
	      __u32 dummy_cnt)
{
	__u32 i = 0, fcr;
	__u32 tx_dma_flag = 0;
	__u32 rx_dma_flag = 0;
	__s32 timeout = 0xffff;

	writew(0, SPI_IER);
	writew(0xffffffff, SPI_ISR); // clear status register

	writew(tcnt, SPI_MTC);
	writew(tcnt + rcnt + dummy_cnt, SPI_MBC);

	// read and write by cpu operation
	if (tcnt) {
		if (tcnt < 64) {
			i = 0;
			while (i < tcnt) {
				// send data
				// while ((readw(SPI_FSR)>>16)==SPI_FIFO_SIZE);
				if (((readw(SPI_FSR) >> 16) & 0x7f) ==
				    SPI_FIFO_SIZE)
					PHY_ERR("TX FIFO size error!\n");
				writeb(*(txbuf + i), SPI_TXD);
				i++;
			}
		} else {
			tx_dma_flag = 1;

			writew((readw(SPI_FCR) | SPI_TXDMAREQ_EN), SPI_FCR);

			nand_dma_config_start(1, (__u32)txbuf, tcnt);
#if 0
			timeout = 0xffffff;
			while (readw(DMA_IRQ_PEND_REG)&0x7 != 7) {
				timeout--;
				if (!timeout)
					break;
			}
			if (timeout == 0) {
				PHY_ERR("TX DMA wait status timeout!\n");
				return ERR_TIMEOUT;
			}
#endif
		}
	}
	/* start transmit */
	writew(readw(SPI_TCR) | SPI_EXCHANGE, SPI_TCR);
	if (rcnt) {
		if (rcnt < 64) {
			i = 0;
#if 1
			timeout = 0xfffff;
			while (1) {
				if (((readw(SPI_FSR)) & 0x7f) == rcnt)
					break;
				if (timeout < 0) {
					PHY_ERR(
					    "RX FIFO size error,timeout!\n");
					break;
				}
				timeout--;
			}
#endif
			while (i < rcnt) {
				// receive valid data
				// while(((readw(SPI_FSR))&0x7f)==0);
				// while((((readw(SPI_FSR))&0x7f)!=rcnt)||
				// timeout < 0))
				//	PHY_ERR("RX FIFO size error!\n");
				*(rxbuf + i) = readb(SPI_RXD);
				i++;
			}
		} else {
			rx_dma_flag = 1;

			writew((readw(SPI_FCR) | SPI_RXDMAREQ_EN), SPI_FCR);

			nand_dma_config_start(0, (__u32)rxbuf, rcnt);
#if 0
			timeout = 0xffffff;
			while (readw(DMA_IRQ_PEND_REG)&0x7 != 7) {
				timeout--;
				if (!timeout)
					break;
			}
			if (timeout == 0) {
				PHY_ERR("RX DMA wait status timeout!\n");
				return ERR_TIMEOUT;
			}

			//while (readw(SPI_FSR)&0x7f);
			if (readw(SPI_FSR)&0x7f) {
				PHY_ERR("RX FIFO not empty after DMA finish!\n");
			}
#endif
		}
	}

	if (NAND_WaitDmaFinish(tx_dma_flag, rx_dma_flag)) {
		PHY_ERR("DMA wait status timeout!\n");
		return ERR_TIMEOUT;
	}

	if (Wait_Tc_Complete()) {
		PHY_ERR("wait tc complete timeout!\n");
		return ERR_TIMEOUT;
	}

	if (tx_dma_flag)
		Nand_Dma_End(1, (__u32)txbuf, tcnt);

	if (rx_dma_flag)
		Nand_Dma_End(0, (__u32)rxbuf, rcnt);

	fcr = readw(SPI_FCR);
	fcr &= ~(SPI_TXDMAREQ_EN | SPI_RXDMAREQ_EN);
	writew(fcr, SPI_FCR);
	if (readw(SPI_ISR) &
	    (0xf << 8)) {/* (1U << 11) | (1U << 10) | (1U << 9) | (1U << 8)) */
		PHY_ERR("FIFO status error: 0x%x!\n", readw(SPI_ISR));
		return NAND_OP_FALSE;
	}

	if (readw(SPI_TCR) & SPI_EXCHANGE)
		PHY_ERR("XCH Control Error!!\n");

	writew(0xffffffff, SPI_ISR); /* clear  flag */

	return NAND_OP_TRUE;
}
#endif

__s32 spi_nand_rdid(__u32 spi_no, __u32 chip, __u32 id_addr, __u32 addr_cnt,
		    __u32 id_cnt, void *id)
{
	__s32 ret = NAND_OP_TRUE;
	__u8 sdata[2];
	__u32 txnum;
	__u32 rxnum;

	txnum = 1 + addr_cnt;
	rxnum = id_cnt; // mira nand:rxnum=5

	sdata[0] = SPI_NAND_RDID;
	sdata[1] = id_addr; // add 00H:Maunufation ID,01H:Device ID

	Spic_sel_ss(spi_no, chip);

	Spic_config_io_mode(spi_no, 0, 0, txnum);
	ret = Spic_rw(spi_no, txnum, (void *)sdata, rxnum, (void *)id, 0);
	return ret;
}
