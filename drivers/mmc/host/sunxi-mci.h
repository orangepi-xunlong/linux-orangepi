/*
 * drivers/mmc/host/sunxi-mci.h
 * (C) Copyright 2007-2011
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Aaron.Maoye <leafy.myeh@reuuimllatech.com>
 *
 * description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SUNXI_HOST_OP_H_
#define _SUNXI_HOST_OP_H_ "host_op.h"




#if defined CONFIG_FPGA_V4_PLATFORM || defined CONFIG_FPGA_V7_PLATFORM
#define MMC_FPGA
#endif

#define DRIVER_NAME "sunxi-mmc"
#define DRIVER_RIVISION "v1.111 2015-4-13 15:24"
#define DRIVER_VERSION "SD/MMC/SDIO Host Controller Driver(" DRIVER_RIVISION ")" \
			" Compiled in " __DATE__ " at " __TIME__""

/*========== platform define ==========*/
/*---------- for sun8iw1 and sun8iw3 ----------*/
#if defined CONFIG_ARCH_SUN8IW1 || defined CONFIG_ARCH_SUN8IW3
#define REG_FIFO_OS	(0x200)
#define SMC_IRQNO(x)	(SUNXI_IRQ_MMC0 + (x))

#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL   "pll6"

#define	SUNXI_CCM_BASE	 	0x01c20000
#define SUNXI_PIO_BASE		0x01c20800

#define MMC_MODCLK_PREFIX "sdmmc"
#define MMC3_DMA_TL       (0x2007000f)

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_MMC2 : SUNXI_IRQ_MMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 2 || (x) == 0)
#endif
#endif

/*---------- for sun8iw2 ----------*/
#ifdef CONFIG_ARCH_SUN8IW2
#define REG_FIFO_OS	(0x100)
#define SMC_IRQNO(x)	(SUNXI_IRQ_MMC0 + (x))

#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL5   "sdram_pll_p"
#define MMC_SRCCLK_PLL6   "sata_pll_2"
#define MMC_AHBCLK_PREFIX "ahb_sdc"
#define MMC_MODCLK_PREFIX "sdc"
#define MMC3_DMA_TL       (0x20070008)

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_MMC2 : SUNXI_IRQ_MMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 0 || (x) == 2)
#endif
#endif

/*---------- for sun9iw1----------*/
#ifdef CONFIG_ARCH_SUN9IW1

//secure storage relate
#define MAX_SECURE_STORAGE_MAX_ITEM			32
#define SDMMC_SECURE_STORAGE_START_ADD	(6*1024*1024/512)//6M
#define SDMMC_ITEM_SIZE									(4*1024/512)//4K

#define REG_FIFO_OS	(0x200)
#define SMC_IRQNO(x)	(SUNXI_IRQ_SDMMC0 + (x))


#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL   "pll4"

#define	SUNXI_CCM_BASE	 	0x06000400
#define SUNXI_PIO_BASE		0x06000800


#define MMC_MODCLK_PREFIX "sdmmc"
#define MMC3_DMA_TL       (0x2007000f)

#define SMC_COMM_BASE		(0x01c13000)//for smc reset switch and config clock switch

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_SDMMC2 : SUNXI_IRQ_SDMMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 2 || (x) == 0)
#endif
#endif

/*---------- for sun8iw5 sun8iw6 ----------*/
#if defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW6

#if defined CONFIG_ARCH_SUN8IW6
//secure storage relate
#define MAX_SECURE_STORAGE_MAX_ITEM			32
#define SDMMC_SECURE_STORAGE_START_ADD	(6*1024*1024/512)//6M
#define SDMMC_ITEM_SIZE									(4*1024/512)//4K
#endif

#define REG_FIFO_OS	(0x200)
#define SMC_IRQNO(x)	(SUNXI_IRQ_MMC0 + (x))

#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL   "pll_periph"
#define MMC_2MOD_CLK     "sdmmc2mod"

#define	SUNXI_CCM_BASE	 	0x01c20000
#define SUNXI_PIO_BASE		0x01c20800

#define MMC_MODCLK_PREFIX "sdmmc"
#define MMC3_DMA_TL       (0x2007000f)

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_MMC2 : SUNXI_IRQ_MMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 2 || (x) == 0)
#endif
#endif

/*---------- for sun8iw7----------*/
#if defined (CONFIG_ARCH_SUN8IW7)
#define REG_FIFO_OS	(0x200)
#define SMC_IRQNO(x)	(SUNXI_IRQ_MMC0 + (x))

#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL   "pll_periph1" //"pll_periph0"
#define MMC_1MOD_CLK     "sdmmc1mod"
#define MMC_2MOD_CLK     "sdmmc2mod"

//secure storage relate
#define MAX_SECURE_STORAGE_MAX_ITEM			32
#define SDMMC_SECURE_STORAGE_START_ADD	(6*1024*1024/512)//6M
#define SDMMC_ITEM_SIZE									(4*1024/512)//4K

#define	SUNXI_CCM_BASE	 	0x01c20000
#define SUNXI_PIO_BASE		0x01c20800

#define MMC_MODCLK_PREFIX "sdmmc"
#define MMC3_DMA_TL       (0x2007000f)

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_MMC2 : SUNXI_IRQ_MMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 2 || (x) == 0)
#endif
#endif

/*---------- for sun8iw8  ----------*/
#if defined CONFIG_ARCH_SUN8IW8
#define REG_FIFO_OS	(0x200)
#define SMC_IRQNO(x)	(SUNXI_IRQ_MMC0 + (x))

#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL   "pll_periph0"
#define MMC_2MOD_CLK     "sdmmc2mod"

#define	SUNXI_CCM_BASE	 	0x01c20000
#define SUNXI_PIO_BASE		0x01c20800

#define MMC_MODCLK_PREFIX "sdmmc"
#define MMC3_DMA_TL       (0x2007000f)

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_MMC2 : SUNXI_IRQ_MMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 2 || (x) == 0)
#endif
#endif


/*---------- for sun8iw9  ----------*/
#if defined CONFIG_ARCH_SUN8IW9
#define REG_FIFO_OS	(0x200)
#define SMC_IRQNO(x)	(SUNXI_IRQ_MMC0 + (x))

#define MMC_SRCCLK_HOSC   "hosc"
#define MMC_SRCCLK_PLL   "pll_periph1"  //"pll_periph0"
#define MMC_2MOD_CLK     "sdmmc2mod"

#define	SUNXI_CCM_BASE	 	0x01c20000
#define SUNXI_PIO_BASE		0x01c20800

#define MMC_MODCLK_PREFIX "sdmmc"
#define MMC3_DMA_TL       (0x2007000f)

#ifdef MMC_FPGA
#undef SMC_IRQNO
#define SMC_IRQNO(x)	((x) & 2 ? SUNXI_IRQ_MMC2 : SUNXI_IRQ_MMC0)
#define SMC_FPGA_MMC_PREUSED(x)	((x) == 2 || (x) == 0)
#endif
#endif




/* SDXC register operation */
#define SMC0_BASE	(0x01C0f000)
#define SMC_BASE_OS	(0x1000)
#define SMC_BASE(x)	(SMC0_BASE + 0x1000 * (x))

/* register offset define */
#define SDXC_REG_GCTRL	( 0x00 ) // SMC Global Control Register
#define SDXC_REG_CLKCR	( 0x04 ) // SMC Clock Control Register
#define SDXC_REG_TMOUT	( 0x08 ) // SMC Time Out Register
#define SDXC_REG_WIDTH	( 0x0C ) // SMC Bus Width Register
#define SDXC_REG_BLKSZ	( 0x10 ) // SMC Block Size Register
#define SDXC_REG_BCNTR	( 0x14 ) // SMC Byte Count Register
#define SDXC_REG_CMDR	( 0x18 ) // SMC Command Register
#define SDXC_REG_CARG	( 0x1C ) // SMC Argument Register
#define SDXC_REG_RESP0	( 0x20 ) // SMC Response Register 0
#define SDXC_REG_RESP1	( 0x24 ) // SMC Response Register 1
#define SDXC_REG_RESP2	( 0x28 ) // SMC Response Register 2
#define SDXC_REG_RESP3	( 0x2C ) // SMC Response Register 3
#define SDXC_REG_IMASK	( 0x30 ) // SMC Interrupt Mask Register
#define SDXC_REG_MISTA	( 0x34 ) // SMC Masked Interrupt Status Register
#define SDXC_REG_RINTR	( 0x38 ) // SMC Raw Interrupt Status Register
#define SDXC_REG_STAS	( 0x3C ) // SMC Status Register
#define SDXC_REG_FTRGL	( 0x40 ) // SMC FIFO Threshold Watermark Registe
#define SDXC_REG_FUNS	( 0x44 ) // SMC Function Select Register
#define SDXC_REG_CBCR	( 0x48 ) // SMC CIU Byte Count Register
#define SDXC_REG_BBCR	( 0x4C ) // SMC BIU Byte Count Register
#define SDXC_REG_DBGC	( 0x50 ) // SMC Debug Enable Register
#if defined CONFIG_ARCH_SUN8IW3 || defined CONFIG_ARCH_SUN9IW1 || defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW6 || defined CONFIG_ARCH_SUN8IW8 \
	|| defined (CONFIG_ARCH_SUN8IW7) || defined CONFIG_ARCH_SUN8IW9
#define SDXC_REG_A12A	(0x58 )		//auto cmd12 arg
#endif
#if defined CONFIG_ARCH_SUN8IW5 || defined CONFIG_ARCH_SUN8IW6 || defined CONFIG_ARCH_SUN8IW8 \
	|| defined (CONFIG_ARCH_SUN8IW7) || defined CONFIG_ARCH_SUN8IW9
#define SDXC_REG_NTSR       ( 0x5C )//SMC Newtiming Set Register
#endif
#define SDXC_REG_HWRST	( 0x78 ) // SMC Card Hardware Reset for Register
#define SDXC_REG_DMAC	( 0x80 ) // SMC IDMAC Control Register
#define SDXC_REG_DLBA	( 0x84 ) // SMC IDMAC Descriptor List Base Addre
#define SDXC_REG_IDST	( 0x88 ) // SMC IDMAC Status Register
#define SDXC_REG_IDIE	( 0x8C ) // SMC IDMAC Interrupt Enable Register
#define SDXC_REG_CHDA	( 0x90 )
#define SDXC_REG_CBDA	( 0x94 )
#define SDXC_REG_FIFO	( REG_FIFO_OS ) // SMC FIFO Access Address


#define mci_readl(host, reg) \
	__raw_readl((host)->reg_base + SDXC_##reg)
#define mci_writel(host, reg, value) \
	__raw_writel((value), (host)->reg_base + SDXC_##reg)
#define mci_readw(host, reg) \
	__raw_readw((host)->reg_base + SDXC_##reg)
#define mci_writew(host, reg, value) \
	__raw_writew((value), (host)->reg_base + SDXC_##reg)

/* bit field for registers */
/* global control register */
#define SDXC_SoftReset		BIT(0)
#define SDXC_FIFOReset		BIT(1)
#define SDXC_DMAReset		BIT(2)
#define SDXC_HWReset		(SDXC_SoftReset|SDXC_FIFOReset|SDXC_DMAReset)
#define SDXC_INTEnb		BIT(4)
#define SDXC_DMAEnb		BIT(5)
#define SDXC_DebounceEnb	BIT(8)
#define SDXC_DDR_MODE		BIT(10)
#define SDXC_MemAccessDone	BIT(29)
#define SDXC_AccessDoneDirect	BIT(30)
#define SDXC_ACCESS_BY_AHB	BIT(31)
#define SDXC_ACCESS_BY_DMA	(0U << 31)
/* Clock control */
#define SDXC_CardClkOn		BIT(16)
#define SDXC_LowPowerOn		BIT(17)
/* bus width */
#define SDXC_WIDTH1		(0)
#define SDXC_WIDTH4		(1)
#define SDXC_WIDTH8		(2)
/* Struct for SMC Commands */
#define SDXC_RspExp		BIT(6) //0x40
#define SDXC_LongRsp		BIT(7) //0x80
#define SDXC_CheckRspCRC	BIT(8) //0x100
#define SDXC_DataExp		BIT(9) //0x200
#define SDXC_Write		BIT(10) //0x400
#define SDXC_Seqmod		BIT(11) //0x800
#define SDXC_SendAutoStop	BIT(12) //0x1000
#define SDXC_WaitPreOver	BIT(13) //0x2000
#define SDXC_StopAbortCMD	BIT(14) //0x4000
#define SDXC_SendInitSeq	BIT(15) //0x8000
#define SDXC_UPCLKOnly		BIT(21) //0x200000
#define SDXC_RdCEATADev		BIT(22) //0x400000
#define SDXC_CCSExp		BIT(23) //0x800000
#define SDXC_EnbBoot		BIT(24) //0x1000000
#define SDXC_AltBootOpt		BIT(25) //0x2000000
#define SDXC_BootACKExp		BIT(26) //0x4000000
#define SDXC_BootAbort		BIT(27) //0x8000000
#define SDXC_VolSwitch	        BIT(28) //0x10000000
#define SDXC_UseHoldReg	        BIT(29) //0x20000000
#define SDXC_Start	        BIT(31) //0x80000000
/* Struct for Intrrrupt Information */
#define SDXC_RespErr		BIT(1) //0x2
#define SDXC_CmdDone		BIT(2) //0x4
#define SDXC_DataOver		BIT(3) //0x8
#define SDXC_TxDataReq		BIT(4) //0x10
#define SDXC_RxDataReq		BIT(5) //0x20
#define SDXC_RespCRCErr		BIT(6) //0x40
#define SDXC_DataCRCErr		BIT(7) //0x80
#define SDXC_RespTimeout	BIT(8) //0x100
#define SDXC_ACKRcv		BIT(8)	//0x100
#define SDXC_DataTimeout	BIT(9)	//0x200
#define SDXC_BootStart		BIT(9)	//0x200
#define SDXC_DataStarve		BIT(10) //0x400
#define SDXC_VolChgDone		BIT(10) //0x400
#define SDXC_FIFORunErr		BIT(11) //0x800
#define SDXC_HardWLocked	BIT(12)	//0x1000
#define SDXC_StartBitErr	BIT(13)	//0x2000
#define SDXC_AutoCMDDone	BIT(14)	//0x4000
#define SDXC_EndBitErr		BIT(15)	//0x8000
#define SDXC_SDIOInt		BIT(16)	//0x10000
#define SDXC_CardInsert		BIT(30) //0x40000000
#define SDXC_CardRemove		BIT(31) //0x80000000
#define SDXC_IntErrBit		(SDXC_RespErr | SDXC_RespCRCErr | SDXC_DataCRCErr \
				| SDXC_RespTimeout | SDXC_DataTimeout | SDXC_FIFORunErr \
				| SDXC_HardWLocked | SDXC_StartBitErr | SDXC_EndBitErr)  //0xbfc2
/* status */
#define SDXC_RXWLFlag		BIT(0)
#define SDXC_TXWLFlag		BIT(1)
#define SDXC_FIFOEmpty		BIT(2)
#define SDXC_FIFOFull		BIT(3)
#define SDXC_CardPresent	BIT(8)
#define SDXC_CardDataBusy	BIT(9)
#define SDXC_DataFSMBusy	BIT(10)
#define SDXC_DMAReq		BIT(31)
#define SDXC_FIFO_SIZE		(16)
/* Function select */
#define SDXC_CEATAOn		(0xceaaU << 16)
#define SDXC_SendIrqRsp		BIT(0)
#define SDXC_SDIORdWait		BIT(1)
#define SDXC_AbtRdData		BIT(2)
#define SDXC_SendCCSD		BIT(8)
#define SDXC_SendAutoStopCCSD	BIT(9)
#define SDXC_CEATADevIntEnb	BIT(10)
/* IDMA controller bus mod bit field */
#define SDXC_IDMACSoftRST	BIT(0)
#define SDXC_IDMACFixBurst	BIT(1)
#define SDXC_IDMACIDMAOn	BIT(7)
#define SDXC_IDMACRefetchDES	BIT(31)
/* IDMA status bit field */
#define SDXC_IDMACTransmitInt	BIT(0)
#define SDXC_IDMACReceiveInt	BIT(1)
#define SDXC_IDMACFatalBusErr	BIT(2)
#define SDXC_IDMACDesInvalid	BIT(4)
#define SDXC_IDMACCardErrSum	BIT(5)
#define SDXC_IDMACNormalIntSum	BIT(8)
#define SDXC_IDMACAbnormalIntSum BIT(9)
#define SDXC_IDMACHostAbtInTx	BIT(10)
#define SDXC_IDMACHostAbtInRx	BIT(10)
#define SDXC_IDMACIdle		(0U << 13)
#define SDXC_IDMACSuspend	(1U << 13)
#define SDXC_IDMACDESCRd	(2U << 13)
#define SDXC_IDMACDESCCheck	(3U << 13)
#define SDXC_IDMACRdReqWait	(4U << 13)
#define SDXC_IDMACWrReqWait	(5U << 13)
#define SDXC_IDMACRd		(6U << 13)
#define SDXC_IDMACWr		(7U << 13)
#define SDXC_IDMACDESCClose	(8U << 13)
/*0x5c ntsr new mode select*/
#define NEWMODE_ENABLE   (1U << 31)

#ifdef CONFIG_ARCH_SUN9IW1
#define SDXC_RET_SW	(1<<18) //SDC reset switch
#define SDXC_CLK_SW	(1<<16) //SDC clock switch
#endif

#define SDXC_IDMA_OVER (SDXC_IDMACTransmitInt|SDXC_IDMACReceiveInt|SDXC_IDMACNormalIntSum)
#define SDXC_IDMA_ERR (SDXC_IDMACFatalBusErr|SDXC_IDMACDesInvalid \
			|SDXC_IDMACCardErrSum|SDXC_IDMACAbnormalIntSum)

#define SDXC_DES_NUM_SHIFT	(15)
#define SDXC_DES_BUFFER_MAX_LEN	(1U << SDXC_DES_NUM_SHIFT)
struct sunxi_mmc_idma_des {
	u32	config;
#define SDXC_IDMAC_DES0_DIC	BIT(1) // disable interrupt on completion
#define SDXC_IDMAC_DES0_LD	BIT(2) // last descriptor
#define SDXC_IDMAC_DES0_FD	BIT(3) // first descriptor
#define SDXC_IDMAC_DES0_CH	BIT(4) // chain mode
#define SDXC_IDMAC_DES0_ER	BIT(5) // end of ring
#define SDXC_IDMAC_DES0_CES	BIT(30) // card error summary
#define SDXC_IDMAC_DES0_OWN	BIT(31) // des owner:1-idma owns it, 0-host owns it

	u32	data_buf1_sz    :16,
		data_buf2_sz    :16;
	u32	buf_addr_ptr1;
	u32	buf_addr_ptr2;
};

struct sunxi_mmc_ctrl_regs {
	u32 gctrl;
	u32 clkc;
	u32 timeout;
	u32 buswid;
	u32 waterlvl;
	u32 funcsel;
	u32 debugc;
	u32 idmacc;
	u32 ntsr;
 
};

/* UHS-I Operation Modes
 * DS		25MHz	12.5MB/s	3.3V
 * HS		50MHz	25MB/s		3.3V
 * SDR12	25MHz	12.5MB/s	1.8V
 * SDR25	50MHz	25MB/s		1.8V
 * SDR50	100MHz	50MB/s		1.8V
 * SDR104	208MHz	104MB/s		1.8V
 * DDR50	50MHz	50MB/s		1.8V
 * MMC Operation Modes
 * DS		26MHz	26MB/s		3/1.8/1.2V
 * HS		52MHz	52MB/s		3/1.8/1.2V
 * HSDDR	52MHz	104MB/s		3/1.8/1.2V
 * HS200	200MHz	200MB/s		1.8/1.2V
 *
 * Spec. Timing
 * SD3.0
 * Fcclk    Tcclk   Fsclk   Tsclk   Tis     Tih     odly  RTis     RTih
 * 400K     2.5us   24M     41ns    5ns     5ns     1     2209ns   41ns
 * 25M      40ns    600M    1.67ns  5ns     5ns     3     14.99ns  5.01ns
 * 50M      20ns    600M    1.67ns  6ns     2ns     3     14.99ns  5.01ns
 * 50MDDR   20ns    600M    1.67ns  6ns     0.8ns   2     6.67ns   3.33ns
 * 104M     9.6ns   600M    1.67ns  3ns     0.8ns   1     7.93ns   1.67ns
 * 208M     4.8ns   600M    1.67ns  1.4ns   0.8ns   1     3.33ns   1.67ns

 * 25M      40ns    300M    3.33ns  5ns     5ns     2     13.34ns   6.66ns
 * 50M      20ns    300M    3.33ns  6ns     2ns     2     13.34ns   6.66ns
 * 50MDDR   20ns    300M    3.33ns  6ns     0.8ns   1     6.67ns    3.33ns
 * 104M     9.6ns   300M    3.33ns  3ns     0.8ns   0     7.93ns    1.67ns
 * 208M     4.8ns   300M    3.33ns  1.4ns   0.8ns   0     3.13ns    1.67ns

 * eMMC4.5
 * 400K     2.5us   24M     41ns    3ns     3ns     1     2209ns    41ns
 * 25M      40ns    600M    1.67ns  3ns     3ns     3     14.99ns   5.01ns
 * 50M      20ns    600M    1.67ns  3ns     3ns     3     14.99ns   5.01ns
 * 50MDDR   20ns    600M    1.67ns  2.5ns   2.5ns   2     6.67ns    3.33ns
 * 200M     5ns     600M    1.67ns  1.4ns   0.8ns   1     3.33ns    1.67ns
 */
#define MMC_CLK_400K					0
#define MMC_CLK_25M						1
#define MMC_CLK_50M						2
#define MMC_CLK_50MDDR				3
#define MMC_CLK_50MDDR_8BIT		4
#define MMC_CLK_100M					5
#define MMC_CLK_200M					6
#define MMC_CLK_MOD_NUM				7

struct sunxi_mmc_clk_dly {
	u32 mode;
	u32 oclk_dly;
	u32 sclk_dly;
};


struct sunxi_mmc_platform_data {
	/* predefine information */
	u32 ocr_avail;
	u32 caps;
	u32 caps2;
	u32 platform_cap;
	u32 f_min;
	u32 f_max;
	u32 f_ddr_max;
	u32 dma_tl;
	char* regulator;
	char* power_supply;
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		||defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	u32  used_2xmod;
	u32  used_ddrmode;
#endif
	/* sys config information */
	u32 used:8,
	    cdmode:8,
	    width:4,
	    wpmode:4,
	    has_hwrst:4,
	    isiodev:4;
#ifndef MMC_USE_RAW_SYS
	struct gpio_config mmcio[10];
	struct gpio_config hwrst;
	struct gpio_config cd;
	struct gpio_config wp;
#endif
	struct pinctrl *pinctrl;

		/*sample delay and output deley setting*/
	struct sunxi_mmc_clk_dly mmc_clk_dly[MMC_CLK_MOD_NUM];
};

struct sunxi_mmc_host {

	struct platform_device  *pdev;
	struct mmc_host *mmc;
	struct sunxi_mmc_platform_data  *pdata;

	/* IO mapping base */
	void __iomem 	*reg_base;
	void __iomem 	*comm_reg;

	spinlock_t 	lock;
	struct tasklet_struct tasklet;
	//for dat3 detect
	struct tasklet_struct d3_det_tasklet;

	/* clock management */
	struct clk 	*hclk;
	struct clk 	*mclk;

	/* ios information */
	u32 		mod_clk;
	u32 		card_clk;
	u32 		oclk_dly;
	u32 		sclk_dly;
	u32 		bus_width;
	u32 		ddr;
	u32 		voltage;
#define SDC_WOLTAGE_3V3 (0)
#define SDC_WOLTAGE_1V8 (1)
#define SDC_WOLTAGE_1V2 (2)
#define SDC_WOLTAGE_OFF (3)
#define SDC_WOLTAGE_ON  (4)
	u32 		voltage_switching;
	struct regulator *regulator;
	u32         regulator_voltage:8; //record the initial io voltage
	u32 		present;

	/* irq */
	int 		irq;
	volatile u32	int_sum;

	volatile u32 	trans_done:1;
	volatile u32 	dma_done:1;
	dma_addr_t	sg_dma;
	void		*sg_cpu;

	struct mmc_request *mrq;
	volatile u32	error;
	volatile u32	ferror;
	volatile u32	wait;
#define SDC_WAIT_NONE		(1<<0)
#define SDC_WAIT_CMD_DONE	(1<<1)
#define SDC_WAIT_DATA_OVER	(1<<2)
#define SDC_WAIT_AUTOCMD_DONE	(1<<3)
#define SDC_WAIT_DMA_DONE	(1<<4)
#define SDC_WAIT_RXDATA_OVER	(SDC_WAIT_DATA_OVER|SDC_WAIT_DMA_DONE)
#define SDC_WAIT_RXAUTOCMD_DONE	(SDC_WAIT_AUTOCMD_DONE|SDC_WAIT_DMA_DONE)
#define SDC_WAIT_ERROR		(1<<6)
#define SDC_WAIT_SWITCH1V8	(1<<7)
#define SDC_WAIT_FINALIZE	(1<<8)
	volatile u32	state;
#define SDC_STATE_IDLE		(0)
#define SDC_STATE_SENDCMD	(1)
#define SDC_STATE_CMDDONE	(2)

	struct timer_list cd_timer;
	u32 pio_hdle;
	s32 cd_hdle;
	s32 cd_mode;
#define CARD_DETECT_BY_GPIO_POLL (1)	/* mmc detected by gpio check */
#define CARD_DETECT_BY_GPIO_IRQ  (2)	/* mmc detected by gpio irq */
#define CARD_ALWAYS_PRESENT      (3)	/* mmc always present, without detect pin */
#define CARD_DETECT_BY_FS        (4)	/* mmc insert/remove by fs, /proc/sunxi-mmc.x/insert node */
#define CARD_DETECT_BY_D3				 (5)	/* mmc detected by data3*/
#define CARD_DETECT_BY_GPIO_IRQ_UP	(6)	//hp t-card detect
	u32 power_on:8;
	u32 read_only:8;
	u32 io_flag:8;
	u32 suspend:8;

	u32 power_for_card;/*used for card power on flag*/

	u32 debuglevel;
	u32 dump_ctl;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *proc_root;
	struct proc_dir_entry *proc_drvver;
	struct proc_dir_entry *proc_hostinfo;
	struct proc_dir_entry *proc_dbglevel;
	struct proc_dir_entry *proc_regs;
	struct proc_dir_entry *proc_insert;
	struct proc_dir_entry *proc_cdmode;
	struct proc_dir_entry *proc_iodrive;
#endif

	/* backup register structrue */
	struct sunxi_mmc_ctrl_regs bak_regs;
};

#define SMC_MSG(d, ...)    do { printk("[mmc]: "__VA_ARGS__); } while(0)
#define SMC_ERR(d, ...)    do { printk("[mmc]: *** %s(L%d): ", __FUNCTION__, __LINE__); \
					printk(__VA_ARGS__);} while(0)

#define SMC_DEBUG_INFO	BIT(0)
#define SMC_DEBUG_DBG	BIT(1)
#ifdef CONFIG_MMC_DEBUG_SUNXI
#define SMC_INFO(d, ...)   do {if ((d)->debuglevel & SMC_DEBUG_INFO) 	\
				SMC_MSG(d, __VA_ARGS__); } while(0)
#define SMC_DBG(d, ...)    do {if ((d)->debuglevel & SMC_DEBUG_DBG) 	\
				SMC_MSG(d, __VA_ARGS__); } while(0)
#else
#define SMC_INFO(d, ...)
#define SMC_DBG(d, ...)
#endif

#ifndef	__io_address
#define __io_address(n)     IOMEM(IO_ADDRESS(n))
#endif

#endif
