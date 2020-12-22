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
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#ifdef CONFIG_DMA_ENGINE
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dma/sunxi-dma.h>
#endif
#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>

#include <linux/clk/sunxi.h>
#include <linux/clk/sunxi_name.h>

#include "spi-sunxi.h"

/* For debug */
#define SPI_EXIT()  		pr_debug("%s()%d - %s \n", __func__, __LINE__, "Exit")
#define SPI_ENTER() 		pr_debug("%s()%d - %s \n", __func__, __LINE__, "Enter ...")
#define SPI_DBG(fmt, arg...)	pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SPI_INF(fmt, arg...)	pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SPI_ERR(fmt, arg...)	pr_warn("%s()%d - "fmt, __func__, __LINE__, ##arg)

#define SUNXI_SPI_OK   0
#define SUNXI_SPI_FAIL -1

enum spi_mode_type {
    SINGLE_HALF_DUPLEX_RX,		//single mode, half duplex read
    SINGLE_HALF_DUPLEX_TX,		//single mode, half duplex write
    SINGLE_FULL_DUPLEX_RX_TX,	//single mode, full duplex read and write
	DUAL_HALF_DUPLEX_RX,		//dual mode, half duplex read
	DUAL_HALF_DUPLEX_TX,		//dual mode, half duplex write
    MODE_TYPE_NULL,
};

enum spi_dma_dir {
	SPI_DMA_RWNULL,
	SPI_DMA_WDEV = DMA_TO_DEVICE,
	SPI_DMA_RDEV = DMA_FROM_DEVICE,
};

#ifdef CONFIG_DMA_ENGINE

#define SPI_MAX_PAGES	32

typedef struct {
	enum spi_dma_dir dir;
	struct dma_chan *chan;
	int nents;
	struct scatterlist sg[SPI_MAX_PAGES];
	struct page *pages[SPI_MAX_PAGES];
} spi_dma_info_t;
#endif

struct sunxi_spi {
    struct platform_device *pdev;
	struct spi_master *master;/* kzalloc */
	void __iomem *base_addr; /* register */
	u32 base_addr_phy;

#ifdef CONFIG_EVB_PLATFORM
	struct clk *pclk;  /* PLL clock */	
	struct clk *mclk;  /* spi module clock */
#endif

#ifdef CONFIG_DMA_ENGINE
	spi_dma_info_t dma_rx;
	spi_dma_info_t dma_tx;
#endif

	enum spi_mode_type mode_type;

	unsigned int irq; /* irq NO. */
	char dev_name[48];

	int busy;
#define SPI_FREE   (1<<0)
#define SPI_SUSPND (1<<1)
#define SPI_BUSY   (1<<2)

	int result; /* 0: succeed -1:fail */

	struct workqueue_struct *workqueue;
	struct work_struct work;

	struct list_head queue; /* spi messages */
	spinlock_t lock;

	struct completion done;  /* wakup another spi transfer */

	/* keep select during one message */
	void (*cs_control)(struct spi_device *spi, bool on);

/*
 * (1) enable cs1,    cs_bitmap = SPI_CHIP_SELECT_CS1;
 * (2) enable cs0&cs1,cs_bitmap = SPI_CHIP_SELECT_CS0|SPI_CHIP_SELECT_CS1;
 *
 */
#define SPI_CHIP_SELECT_CS0 (0x01)
#define SPI_CHIP_SELECT_CS1 (0x02)

	int cs_bitmap;/* cs0- 0x1; cs1-0x2, cs0&cs1-0x3. */

	struct pinctrl		 *pctrl;
};

static int spi_used_mask = 0;

/* config chip select */
static s32 spi_set_cs(u32 chipselect, void __iomem *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);

    if (chipselect < 4) {
        reg_val &= ~SPI_TC_SS_MASK;/* SS-chip select, clear two bits */
        reg_val |= chipselect << SPI_TC_SS_BIT_POS;/* set chip select */
        writel(reg_val, base_addr + SPI_TC_REG);
        return SUNXI_SPI_OK;
    }
    else {
        SPI_ERR("Chip Select set fail! cs = %d\n", chipselect);
        return SUNXI_SPI_FAIL;
    }
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
        reg_val |= SPI_TC_SPOL;/*default SSPOL = 1,Low level effective */

    /*4. LMTF--LSB/MSB transfer first select */
    if (config & SPI_LSB_FIRST_ACTIVE_)
        reg_val |= SPI_TC_FBS;
    else
        reg_val &= ~SPI_TC_FBS;/*default LMTF =0, MSB first */

    /*master mode: set DDB,DHB,SMC,SSCTL*/
    if(master == 1) {
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
    }
    else {
        /* tips for slave mode config */
        SPI_INF("slave mode configurate control register.\n");
    }
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* set spi clock */
static void spi_set_clk(u32 spi_clk, u32 ahb_clk, void __iomem *base_addr)
{
    u32 reg_val = 0;
    u32 div_clk = ahb_clk / (spi_clk << 1);

    SPI_DBG("set spi clock %d, mclk %d\n", spi_clk, ahb_clk);
    reg_val = readl(base_addr + SPI_CLK_CTL_REG);

    /* CDR2 */
    if(div_clk <= SPI_CLK_SCOPE) {
        if (div_clk != 0) {
            div_clk--;
        }
        reg_val &= ~SPI_CLK_CTL_CDR2;
        reg_val |= (div_clk | SPI_CLK_CTL_DRS);
        SPI_DBG("CDR2 - n = %d \n", div_clk);
    }/* CDR1 */
    else {
		div_clk = 0;
		while(ahb_clk > spi_clk){
			div_clk++;
			ahb_clk >>= 1;
		}
        reg_val &= ~(SPI_CLK_CTL_CDR1 | SPI_CLK_CTL_DRS);
        reg_val |= (div_clk << 8);
        SPI_DBG("CDR1 - n = %d \n", div_clk);
    }
    writel(reg_val, base_addr + SPI_CLK_CTL_REG);
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

#ifdef CONFIG_DMA_ENGINE
/* enable dma irq */
static void spi_enable_dma_irq(u32 bitmap, void __iomem *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);
    bitmap &= SPI_FIFO_CTL_DRQEN_MASK;
    reg_val |= bitmap;
    writel(reg_val, base_addr + SPI_FIFO_CTL_REG);

    spi_set_dma_mode(base_addr);
}
#endif

/* disable dma irq */
static void spi_disable_dma_irq(u32 bitmap, void __iomem *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_FIFO_CTL_REG);
    bitmap &= SPI_FIFO_CTL_DRQEN_MASK;
    reg_val &= ~bitmap;
    writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
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
#ifdef CONFIG_ARCH_SUN8IW7P1
	reg_val |= (0x20<<16) | 0x20;
#else
	reg_val |= (0x40<<16) | 0x1;
#endif
    writel(reg_val, base_addr + SPI_FIFO_CTL_REG);
}

/* set transfer total length BC, transfer length TC and single transmit length STC */
static void spi_set_bc_tc_stc(u32 tx_len, u32 rx_len, u32 stc_len, u32 dummy_cnt, void __iomem *base_addr)
{
    u32 reg_val = readl(base_addr + SPI_BURST_CNT_REG);

	SPI_DBG("tx: %d, rx: %d, stc: %d, dummy: %d\n", tx_len, rx_len, stc_len, dummy_cnt);

    reg_val &= ~SPI_BC_CNT_MASK;
    reg_val |= (SPI_BC_CNT_MASK & (tx_len + rx_len + dummy_cnt));
    writel(reg_val, base_addr + SPI_BURST_CNT_REG);
    SPI_DBG("REG BC = %#x --\n", readl(base_addr + SPI_BURST_CNT_REG));

    reg_val = readl(base_addr + SPI_TRANSMIT_CNT_REG);
    reg_val &= ~SPI_TC_CNT_MASK;
    reg_val |= (SPI_TC_CNT_MASK & tx_len);
    writel(reg_val, base_addr + SPI_TRANSMIT_CNT_REG);
    SPI_DBG("REG TC = %#x --\n", readl(base_addr + SPI_TRANSMIT_CNT_REG));

	reg_val = readl(base_addr + SPI_BCC_REG);
    reg_val &= ~(SPI_BCC_STC_MASK|SPI_BCC_DBC_MASK);
	reg_val |= (dummy_cnt << 24)&SPI_BCC_DBC_MASK;
    reg_val |= (SPI_BCC_STC_MASK & stc_len);
    writel(reg_val, base_addr + SPI_BCC_REG);
    SPI_DBG("REG BCC = %#x --\n", readl(base_addr + SPI_BCC_REG));
}

/* set ss control */
static void spi_ss_ctrl(void __iomem *base_addr, u32 on_off)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);
    on_off &= 0x1;
    if(on_off)
        reg_val |= SPI_TC_SS_OWNER;
    else
        reg_val &= ~SPI_TC_SS_OWNER;
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* set ss level */
static void spi_ss_level(void __iomem *base_addr, u32 hi_lo)
{
    u32 reg_val = readl(base_addr + SPI_TC_REG);
    hi_lo &= 0x1;
    if(hi_lo)
        reg_val |= SPI_TC_SS_LEVEL;
    else
        reg_val &= ~SPI_TC_SS_LEVEL;
    writel(reg_val, base_addr + SPI_TC_REG);
}

/* set dhb, 1: discard unused spi burst; 0: receiving all spi burst */
static void spi_set_all_burst_received(void __iomem *base_addr)
{
    u32 reg_val = readl(base_addr+SPI_TC_REG);
	reg_val &= ~SPI_TC_DHB;
    writel(reg_val, base_addr + SPI_TC_REG);
}

static void spi_clear_dual_read(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);
	reg_val &= ~SPI_BCC_DUAL_MOD_RX_EN;
	writel(reg_val, base_addr + SPI_BCC_REG);
}

static void spi_set_dual_read(void __iomem  *base_addr)
{
	u32 reg_val = readl(base_addr+SPI_BCC_REG);
	reg_val |= SPI_BCC_DUAL_MOD_RX_EN;
	writel(reg_val, base_addr + SPI_BCC_REG);
}

static int spi_regulator_request(struct sunxi_spi_platform_data *pdata)
{
	struct regulator *regu = NULL;

	if (pdata->regulator != NULL)
		return 0;

	/* Consider "n***" as nocare. Support "none", "nocare", "null", "" etc. */
	if ((pdata->regulator_id[0] == 'n') || (pdata->regulator_id[0] == 0))
		return 0;

	regu = regulator_get(NULL, pdata->regulator_id);
	if (IS_ERR(regu)) {
		SPI_ERR("get regulator %s failed!\n", pdata->regulator_id);
		return -1;
	}
	pdata->regulator = regu;
	return 0;
}

static void spi_regulator_release(struct sunxi_spi_platform_data *pdata)
{
	if (pdata->regulator == NULL)
		return;

	regulator_put(pdata->regulator);
	pdata->regulator = NULL;
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

#ifdef CONFIG_DMA_ENGINE

/* ------------------------------- dma operation start----------------------------- */
/* dma full done callback for spi rx */
static void sunxi_spi_dma_cb_rx(void *data)
{
	struct sunxi_spi *sspi = (struct sunxi_spi *)data;
    unsigned long flags = 0;
	void __iomem *base_addr = sspi->base_addr;

    spin_lock_irqsave(&sspi->lock, flags);
	spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, sspi->base_addr);
	SPI_DBG("[spi-%d]: dma -read data end!\n", sspi->master->bus_num);

	if (spi_query_rxfifo(base_addr) > 0) {
		SPI_ERR("[spi-%d]: DMA end, but RxFIFO isn't empty! FSR: %#x\n",
			sspi->master->bus_num, spi_query_rxfifo(base_addr));
		sspi->result = -1;// failed
	}
	else
		sspi->result = 0;

	complete(&sspi->done);
    spin_unlock_irqrestore(&sspi->lock, flags);
}

/* dma full done callback for spi tx */
static void sunxi_spi_dma_cb_tx(void *data)
{
	struct sunxi_spi *sspi = (struct sunxi_spi *)data;
    unsigned long flags = 0;

    spin_lock_irqsave(&sspi->lock, flags);
	spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, sspi->base_addr);
	SPI_DBG("[spi-%d]: dma -write data end!\n", sspi->master->bus_num);
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

static int sunxi_spi_dma_init_sg(spi_dma_info_t *info, void *addr, int len, int write)
{
	int i;
	int npages = 0;
	void *bufp = addr;
	int mapbytes = 0;
	int bytesleft = len;

	if (!access_ok(write ? VERIFY_READ : VERIFY_WRITE, (void __user *)addr, len))
		return -EFAULT;

	npages = sunxi_spi_dmg_sg_cnt(addr, len);
	WARN_ON(npages == 0);
	SPI_DBG("npages = %d, len = %d \n", npages, len);
	if (npages > SPI_MAX_PAGES)
		npages = SPI_MAX_PAGES;

	sg_init_table(info->sg, npages);
	for (i=0; i<npages; i++) {
		/*
		 * If there are less bytes left than what fits
		 * in the current page (plus page alignment offset)
		 * we just feed in this, else we stuff in as much
		 * as we can.
		 */
		if (bytesleft < (PAGE_SIZE - offset_in_page(bufp)))
			mapbytes = bytesleft;
		else
			mapbytes = PAGE_SIZE - offset_in_page(bufp);

		SPI_DBG("%d: len %d, offset %ld, addr %p(%d)\n", i, mapbytes, 
				offset_in_page(bufp), bufp, virt_addr_valid(bufp));
		if (virt_addr_valid(bufp))
			sg_set_page(&info->sg[i], virt_to_page(bufp), mapbytes, offset_in_page(bufp));
		else
			sg_set_page(&info->sg[i], vmalloc_to_page(bufp), mapbytes, offset_in_page(bufp));
		
		bufp += mapbytes;
		bytesleft -= mapbytes;
	}

	BUG_ON(bytesleft);
	info->nents = npages;
	return 0;
}

/* request dma channel and set callback function */
static int sunxi_spi_prepare_dma(spi_dma_info_t *_info, enum spi_dma_dir _dir)
{
	dma_cap_mask_t mask;

	SPI_DBG("Init DMA, dir %d \n", _dir);

	/* Try to acquire a generic DMA engine slave channel */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	if (_info->chan == NULL) {
		_info->chan = dma_request_channel(mask, NULL, NULL);
	    if (_info->chan == NULL) {
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

	SPI_DBG("t->rx_buf = %p, t->len = %d \n", t->rx_buf, t->len);
	ret = sunxi_spi_dma_init_sg(&sspi->dma_rx, t->rx_buf, t->len, VERIFY_WRITE);
	if (ret != 0)
		return ret;

	dma_conf.direction = DMA_DEV_TO_MEM;
	dma_conf.src_addr = sspi->base_addr_phy + SPI_RXDATA_REG;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_conf.src_maxburst = 1;
	dma_conf.dst_maxburst = 1;
	dma_conf.slave_id = sunxi_slave_id(DRQDST_SDRAM, SUNXI_SPI_DRQ_RX(sspi->master->bus_num));
	dmaengine_slave_config(sspi->dma_rx.chan, &dma_conf);

	nents = dma_map_sg(&sspi->pdev->dev, sspi->dma_rx.sg, sspi->dma_rx.nents, DMA_FROM_DEVICE);
	if (!nents) {
		SPI_ERR("dma_map_sg(%d) failed! return %d\n", sspi->dma_rx.nents, nents);
		return -ENOMEM;
	}
	SPI_DBG("npages = %d, nents = %d \n", sspi->dma_rx.nents, nents);

	dma_desc = dmaengine_prep_slave_sg(sspi->dma_rx.chan, sspi->dma_rx.sg, nents, 
				DMA_FROM_DEVICE, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		SPI_ERR("[spi-%d]dmaengine_prep_slave_sg() failed!\n", sspi->master->bus_num);
		dma_unmap_sg(&sspi->pdev->dev, sspi->dma_rx.sg, sspi->dma_rx.nents, DMA_FROM_DEVICE);
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

	SPI_DBG("t->tx_buf = %p, t->len = %d \n", t->tx_buf, t->len);
	ret = sunxi_spi_dma_init_sg(&sspi->dma_tx, (void *)t->tx_buf, t->len, VERIFY_READ);
	if (ret != 0)
		return ret;

	dma_conf.direction = DMA_MEM_TO_DEV;
	dma_conf.dst_addr = sspi->base_addr_phy + SPI_TXDATA_REG;
	dma_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	dma_conf.src_maxburst = 1;
	dma_conf.dst_maxburst = 1;
	dma_conf.slave_id = sunxi_slave_id(SUNXI_SPI_DRQ_TX(sspi->master->bus_num), DRQSRC_SDRAM);
	dmaengine_slave_config(sspi->dma_tx.chan, &dma_conf);

	nents = dma_map_sg(&sspi->pdev->dev, sspi->dma_tx.sg, sspi->dma_tx.nents, DMA_FROM_DEVICE);
	if (!nents) {
		SPI_ERR("dma_map_sg(%d) failed! return %d\n", sspi->dma_tx.nents, nents);
		return -ENOMEM;
	}
	SPI_DBG("npages = %d, nents = %d \n", sspi->dma_tx.nents, nents);

	dma_desc = dmaengine_prep_slave_sg(sspi->dma_tx.chan, sspi->dma_tx.sg, nents, 
				DMA_TO_DEVICE, DMA_PREP_INTERRUPT|DMA_CTRL_ACK);
	if (!dma_desc) {
		SPI_ERR("[spi-%d]dmaengine_prep_slave_sg() failed!\n", sspi->master->bus_num);
		dma_unmap_sg(&sspi->pdev->dev, sspi->dma_tx.sg, sspi->dma_tx.nents, DMA_FROM_DEVICE);
		return -1;
	}

	dma_desc->callback = sunxi_spi_dma_cb_tx;
	dma_desc->callback_param = (void *)sspi;
	dmaengine_submit(dma_desc);
	return 0;
}

/*
 * config dma src and dst address,
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

/*	Never release the DMA channel. Duanmintao
	dma_release_channel(info->chan);
    info->chan = NULL;
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
/* ------------------------------dma operation end----------------------------- */

/* check the valid of cs id */
static int sunxi_spi_check_cs(int cs_id, struct sunxi_spi *sspi)
{
    int ret = SUNXI_SPI_FAIL;

    switch(cs_id)
    {
        case 0:
            ret = (sspi->cs_bitmap & SPI_CHIP_SELECT_CS0) ? SUNXI_SPI_OK : SUNXI_SPI_FAIL;
            break;
        case 1:
            ret = (sspi->cs_bitmap & SPI_CHIP_SELECT_CS1) ? SUNXI_SPI_OK : SUNXI_SPI_FAIL;
            break;
        default:
            SPI_ERR("[spi-%d]: chip select not support! cs = %d \n", sspi->master->bus_num, cs_id);
            break;
    }

    return ret;
}

/* spi device on or off control */
static void sunxi_spi_cs_control(struct spi_device *spi, bool on)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	unsigned int cs = 0;
	if (sspi->cs_control) {
		if(on) {
			/* set active */
			cs = (spi->mode & SPI_CS_HIGH) ? 1 : 0;
		}
		else {
			/* set inactive */
			cs = (spi->mode & SPI_CS_HIGH) ? 0 : 1;
		}
		spi_ss_level(sspi->base_addr, cs);
	}
}

/*
 * change the properties of spi device with spi transfer.
 * every spi transfer must call this interface to update the master to the excute transfer
 * set clock frequecy, bits per word, mode etc...
 * return:  >= 0 : succeed;    < 0: failed.
 */
static int sunxi_spi_xfer_setup(struct spi_device *spi, struct spi_transfer *t)
{
	/* get at the setup function, the properties of spi device */
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	struct sunxi_spi_config *config = spi->controller_data; //allocate in the setup,and free in the cleanup
    void __iomem *base_addr = sspi->base_addr;

	config->max_speed_hz  = (t && t->speed_hz) ? t->speed_hz : spi->max_speed_hz;
	config->bits_per_word = (t && t->bits_per_word) ? t->bits_per_word : spi->bits_per_word;
	config->bits_per_word = ((config->bits_per_word + 7) / 8) * 8;

	if(config->bits_per_word != 8) {
	    SPI_ERR("[spi-%d]: just support 8bits per word... \n", spi->master->bus_num);
	    return -EINVAL;
	}

	if(spi->chip_select >= spi->master->num_chipselect) {
	    SPI_ERR("[spi-%d]: spi device's chip select = %d exceeds the master supported cs_num[%d] \n",
	                    spi->master->bus_num, spi->chip_select, spi->master->num_chipselect);
	    return -EINVAL;
	}

	/* check again board info */
	if( SUNXI_SPI_OK != sunxi_spi_check_cs(spi->chip_select, sspi) ) {
	    SPI_ERR("sunxi_spi_check_cs failed! spi_device cs =%d ...\n", spi->chip_select);
	    return -EINVAL;
	}

	/* set cs */
	spi_set_cs(spi->chip_select, base_addr);
    /*
     *  master: set spi module clock;
     *  set the default frequency	10MHz
     */
    spi_set_master(base_addr);
   	if(config->max_speed_hz > SPI_MAX_FREQUENCY) {
	    return -EINVAL;
	}
#ifdef CONFIG_EVB_PLATFORM
	spi_set_clk(config->max_speed_hz, clk_get_rate(sspi->mclk), base_addr);
#else
	spi_set_clk(config->max_speed_hz, 24000000, base_addr);
#endif
    /*
     *  master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     *  set bit width-default: 8 bits
     */
    spi_config_tc(1, spi->mode, base_addr);
	spi_enable_tp(base_addr);

	return 0;
}

static int sunxi_spi_mode_check(struct sunxi_spi *sspi, struct spi_device *spi, struct spi_transfer *t)
{
	unsigned long flags = 0;
	static int dual_mode_enable = 0;
	struct sunxi_dual_mode_dev_data *dual_mode_cfg = (struct sunxi_dual_mode_dev_data *)spi->dev.platform_data;

	if (sspi->mode_type != MODE_TYPE_NULL)
        return -EINVAL;

	SPI_DBG("dual_mode_cfg = %p \n", dual_mode_cfg);

	/* dual spi mode */
	if ((dual_mode_cfg != NULL) && (dual_mode_cfg->dual_mode == 1)) {
		SPI_DBG("in dual SPI mode\n");
		if (t->tx_buf && t->rx_buf) {
			SPI_ERR("full duplex is not support in dual spi mode\n");
			return -1;
		}

		/* half duplex transmit(dual mode) */
		if (t->tx_buf) {
			/* If the command is Dual Mode Read. */
			if (*(const u8 *)t->tx_buf == 0x3b) {
				dual_mode_enable = 1;
				spi_set_dual_read(sspi->base_addr);
			}
			else {
				dual_mode_enable = 0;
				spi_clear_dual_read(sspi->base_addr);
			}

			spin_lock_irqsave(&sspi->lock, flags);
			spi_set_bc_tc_stc(t->len, 0, t->len, 0, sspi->base_addr);
			sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
			spin_unlock_irqrestore(&sspi->lock, flags);
		}/* half duplex receive(dual mode) */
		else if (t->rx_buf) {
			spin_lock_irqsave(&sspi->lock, flags);
			if (dual_mode_enable == 1) {
				spi_set_bc_tc_stc(dual_mode_cfg->single_cnt, t->len,
					dual_mode_cfg->single_cnt, dual_mode_cfg->dummy_cnt, sspi->base_addr);
				sspi->mode_type = DUAL_HALF_DUPLEX_RX;
			}
			else {
				spi_set_bc_tc_stc(0, t->len, 0, 0, sspi->base_addr);
				sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
			}
			spin_unlock_irqrestore(&sspi->lock, flags);
		}
		return 0;
	}

	/* single spi mode */
	SPI_DBG("in single SPI mode\n");
	spin_lock_irqsave(&sspi->lock, flags);
	if (t->tx_buf && t->rx_buf) {
		/* full duplex */
		spi_set_all_burst_received(sspi->base_addr);
		spi_set_bc_tc_stc(t->len, 0, t->len, 0, sspi->base_addr);
		sspi->mode_type = SINGLE_FULL_DUPLEX_RX_TX;
	}
	else {
		/* half duplex transmit(single mode) */
		if (t->tx_buf){
			spi_set_bc_tc_stc(t->len, 0, t->len, 0, sspi->base_addr);
			sspi->mode_type = SINGLE_HALF_DUPLEX_TX;
		}/* half duplex receive(single mode) */
		else if (t->rx_buf){
			spi_set_bc_tc_stc(0, t->len, 0, 0, sspi->base_addr);
			sspi->mode_type = SINGLE_HALF_DUPLEX_RX;
		}
	}
	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;
}
/*
 * < 64 : cpu ;  >= 64 : dma
 * wait for done completion in this function, wakup in the irq hanlder
 */
static int sunxi_spi_xfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	void __iomem* base_addr = sspi->base_addr;
	unsigned long flags = 0;
	unsigned tx_len = t->len;	/* number of bytes receieved */
	unsigned rx_len = t->len;	/* number of bytes sent */
	unsigned char *rx_buf = (unsigned char *)t->rx_buf;
	unsigned char *tx_buf = (unsigned char *)t->tx_buf;
	int ret = 0;

    SPI_DBG("[spi-%d]: begin transfer, txbuf %p, rxbuf %p, len %d, mode %d\n", 
		spi->master->bus_num, tx_buf, rx_buf, t->len, sspi->mode_type);
    if ((!t->tx_buf && !t->rx_buf) || !t->len)
		return -EINVAL;

    /* write 1 to clear 0 */
    spi_clr_irq_pending(SPI_INT_STA_MASK, base_addr);
    /* disable all DRQ */
    spi_disable_dma_irq(SPI_FIFO_CTL_DRQEN_MASK, base_addr);
    /* reset tx/rx fifo */
    spi_reset_fifo(base_addr);

    if (sunxi_spi_mode_check(sspi, spi, t))
        return -EINVAL;

    /*
     * 1. Tx/Rx error irq,process in IRQ;
     * 2. Transfer Complete Interrupt Enable
     */
    spi_enable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);

    /* >64 use DMA transfer, or use cpu */
    if(t->len > BULK_DATA_BOUNDARY) {
#ifdef CONFIG_DMA_ENGINE
        switch(sspi->mode_type)
        {
			case DUAL_HALF_DUPLEX_RX:
            case SINGLE_HALF_DUPLEX_RX:
            {
                SPI_DBG(" rx -> by dma\n");
				/* For Rx mode, the DMA end(not TC flag) is real end. */
				spi_disable_irq(SPI_INTEN_TC, base_addr);

                /* rxFIFO reday dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
                ret = sunxi_spi_prepare_dma(&sspi->dma_rx, SPI_DMA_RDEV);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sunxi_spi_config_dma(sspi, SPI_DMA_RDEV, t);
                sunxi_spi_start_dma(&sspi->dma_rx);

                spi_start_xfer(base_addr);
                break;
            }
            case SINGLE_HALF_DUPLEX_TX:
            {
                SPI_DBG(" tx -> by dma\n");
                spi_start_xfer(base_addr);
                /* txFIFO empty dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
                ret = sunxi_spi_prepare_dma(&sspi->dma_tx, SPI_DMA_WDEV);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sunxi_spi_config_dma(sspi, SPI_DMA_WDEV, t);
                sunxi_spi_start_dma(&sspi->dma_tx);
                break;
            }
            case SINGLE_FULL_DUPLEX_RX_TX:
            {
                SPI_DBG(" rx and tx -> by dma\n");
				/* For Rx mode, the DMA end(not TC flag) is real end. */
				spi_disable_irq(SPI_INTEN_TC, base_addr);

                /* rxFIFO ready dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
                ret = sunxi_spi_prepare_dma(&sspi->dma_rx, SPI_DMA_RDEV);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_RX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sunxi_spi_config_dma(sspi, SPI_DMA_RDEV, t);
                sunxi_spi_start_dma(&sspi->dma_rx);

                spi_start_xfer(base_addr);

                /* txFIFO empty dma request enable */
                spi_enable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
                ret = sunxi_spi_prepare_dma(&sspi->dma_tx, SPI_DMA_WDEV);
                if(ret < 0) {
                    spi_disable_dma_irq(SPI_FIFO_CTL_TX_DRQEN, base_addr);
                    spi_disable_irq(SPI_INTEN_TC|SPI_INTEN_ERR, base_addr);
                    return -EINVAL;
                }
                sunxi_spi_config_dma(sspi, SPI_DMA_WDEV, t);
                sunxi_spi_start_dma(&sspi->dma_tx);
                break;
            }
			case DUAL_HALF_DUPLEX_TX:
			{
				SPI_ERR("dual half duplex tx -> by dma (not support now)\n");
				break;
			}
            default:
                return -1;
        }
#endif
    }
    else {
        switch(sspi->mode_type)
        {
			case DUAL_HALF_DUPLEX_RX:
            case SINGLE_HALF_DUPLEX_RX:
            {
                unsigned int poll_time = 0x7ffff;
                SPI_DBG(" rx -> by ahb\n");
                /* SMC=1,XCH trigger the transfer */
                spi_start_xfer(base_addr);
                while(rx_len && (--poll_time >0)) {
                    /* rxFIFO counter */
                    if(spi_query_rxfifo(base_addr)){
                        *rx_buf++ =  readb(base_addr + SPI_RXDATA_REG);//fetch data
                        --rx_len;
                    }
                }
                if(poll_time <= 0) {
                    SPI_ERR("cpu receive data time out!\n");
					return -1;
                }
                break;
            }
            case SINGLE_HALF_DUPLEX_TX:
            {
                unsigned int poll_time = 0xfffff;
                SPI_DBG(" tx -> by ahb\n");
                spi_start_xfer(base_addr);

                spin_lock_irqsave(&sspi->lock, flags);
                for(; tx_len > 0; --tx_len) {
                    writeb(*tx_buf++, base_addr + SPI_TXDATA_REG);
                }
                spin_unlock_irqrestore(&sspi->lock, flags);

                while(spi_query_txfifo(base_addr)&&(--poll_time > 0) );/* txFIFO counter */
                if(poll_time <= 0) {
                    SPI_ERR("cpu transfer data time out!\n");
					return -1;
                }
                break;
            }
            case SINGLE_FULL_DUPLEX_RX_TX:
            {
                unsigned int poll_time_tx = 0xfffff;
                unsigned int poll_time_rx = 0x7ffff;
                SPI_DBG(" rx and tx -> by ahb\n");
                if((rx_len == 0) || (tx_len == 0))
                    return -EINVAL;

                spi_start_xfer(base_addr);

                spin_lock_irqsave(&sspi->lock, flags);
                for(; tx_len > 0; --tx_len) {
                    writeb(*tx_buf++, base_addr + SPI_TXDATA_REG);
                }
                spin_unlock_irqrestore(&sspi->lock, flags);

                while(spi_query_txfifo(base_addr)&&(--poll_time_tx > 0) );/* txFIFO counter */
                if(poll_time_tx <= 0) {
                    SPI_ERR("cpu transfer data time out!\n");
					return -1;
                }

                while(rx_len && (--poll_time_rx >0)) {
                    /* rxFIFO counter */
                    if(spi_query_rxfifo(base_addr)){
                        *rx_buf++ =  readb(base_addr + SPI_RXDATA_REG);//fetch data
                        --rx_len;
                    }
                }
                if(poll_time_rx <= 0) {
                    SPI_ERR("cpu receive data time out!\n");
					return -1;
                }
                break;
            }
			case DUAL_HALF_DUPLEX_TX:
			{
				SPI_ERR("dual half duplex tx -> by ahb (not support now)\n");
				break;
			}
            default:
                return -1;
        }
    }
	/* wait for xfer complete in the isr. */
	wait_for_completion(&sspi->done);
    /* get the isr return code */
    if(sspi->result != 0) {
        SPI_ERR("[spi-%d]: xfer failed... \n", spi->master->bus_num);
        ret = -1;
    }

#ifdef CONFIG_DMA_ENGINE
    /* release dma resource if neccessary */
	sunxi_spi_release_dma(sspi, t);
#endif

    if(sspi->mode_type != MODE_TYPE_NULL)
        sspi->mode_type = MODE_TYPE_NULL;

	return ret;
}

/* spi core xfer process */
static void sunxi_spi_work(struct work_struct *work)
{
	struct sunxi_spi *sspi = container_of(work, struct sunxi_spi, work);
	unsigned long flags = 0;
	
	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy = SPI_BUSY;
	/*
     * get from messages queue, and then do with them,
	 * if message queue is empty ,then return and set status to free,
	 * otherwise process them.
	 */
	while (!list_empty(&sspi->queue)) {
		struct spi_message *msg = NULL;
		struct spi_device  *spi = NULL;
		struct spi_transfer *t  = NULL;
		unsigned int cs_change = 0;
		int status;
		/* get message from message queue in sunxi_spi. */
		msg = container_of(sspi->queue.next, struct spi_message, queue);
		/* then delete from the message queue,now it is alone.*/
		list_del_init(&msg->queue);
		spin_unlock_irqrestore(&sspi->lock, flags);
		/* get spi device from this message */
		spi = msg->spi;
		/* set default value,no need to change cs,keep select until spi transfer require to change cs. */
		cs_change = 1;
		/* set message status to succeed. */
		status = 0;
		/* search the spi transfer in this message, deal with it alone. */
		list_for_each_entry (t, &msg->transfers, transfer_list) {
			if (t->bits_per_word || t->speed_hz) { /* if spi transfer is zero,use spi device value. */
				status = sunxi_spi_xfer_setup(spi, t);/* set the value every spi transfer */
				if (status < 0)
					break;/* fail, quit */
				SPI_DBG("[spi-%d]: xfer setup \n", sspi->master->bus_num);
			}
			/* first active the cs */
			if (cs_change)
				sspi->cs_control(spi, 1);
			/* update the new cs value */
			cs_change = t->cs_change;
			/*
             * do transfer
			 * > 64 : dma ;  <= 64 : cpu
			 * wait for done completion in this function, wakup in the irq hanlder
			 */
			status = sunxi_spi_xfer(spi, t);
			if (status)
				break;/* fail quit, zero means succeed */
			/* accmulate the value in the message */
			msg->actual_length += t->len;
			/* may be need to delay */
			if (t->delay_usecs)
				udelay(t->delay_usecs);
			/* if zero ,keep active,otherwise deactived. */
			if (cs_change)
				sspi->cs_control(spi, 0);
		}
		/*
		 * spi message complete,succeed or failed
		 * return value
		 */
		msg->status = status;
		/* wakup the uplayer caller,complete one message */
		msg->complete(msg->context);
		/* fail or need to change cs */
		if (status || !cs_change) {
			sspi->cs_control(spi, 0);
		}
        /* restore default value. */
		sunxi_spi_xfer_setup(spi, NULL);
		spin_lock_irqsave(&sspi->lock, flags);
	}
	/* set spi to free */
	sspi->busy = SPI_FREE;
	spin_unlock_irqrestore(&sspi->lock, flags);
}

/* wake up the sleep thread, and give the result code */
static irqreturn_t sunxi_spi_handler(int irq, void *dev_id)
{
	struct sunxi_spi *sspi = (struct sunxi_spi *)dev_id;
	void __iomem *base_addr = sspi->base_addr;
    unsigned int status = 0;
    unsigned long flags = 0;

    spin_lock_irqsave(&sspi->lock, flags);

	status = spi_qry_irq_pending(base_addr);	
    spi_clr_irq_pending(status, base_addr);//write 1 to clear 0.
    SPI_DBG("[spi-%d]: irq status = %x \n", sspi->master->bus_num, status);

    sspi->result = 0; /* assume succeed */
    /* master mode, Transfer Complete Interrupt */
    if(status & SPI_INT_STA_TC) {
        SPI_DBG("[spi-%d]: SPI TC comes\n", sspi->master->bus_num);
        spi_disable_irq(SPI_INT_STA_TC | SPI_INT_STA_ERR, base_addr);

        /*wakup uplayer, by the sem */
        complete(&sspi->done);
		spin_unlock_irqrestore(&sspi->lock, flags);
        return IRQ_HANDLED;
    }/* master mode:err */
	else if (status & SPI_INT_STA_ERR) {
        SPI_ERR("[spi-%d]: SPI ERR %#x comes\n", sspi->master->bus_num, status);
        /* error process, release dma in the workqueue,should not be here */
        spi_disable_irq(SPI_INT_STA_TC | SPI_INT_STA_ERR, base_addr);
        //spi_restore_state(1, base_addr);
		spi_soft_reset(base_addr);
        sspi->result = -1;
        complete(&sspi->done);
		spin_unlock_irqrestore(&sspi->lock, flags);
        return IRQ_HANDLED;
    }
    SPI_DBG("[spi-%d]: SPI NONE comes\n", sspi->master->bus_num);
    spin_unlock_irqrestore(&sspi->lock, flags);
    return IRQ_NONE;
}

/* interface 1 */
static int sunxi_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	unsigned long flags;
	msg->actual_length = 0;
	msg->status = -EINPROGRESS;

	spin_lock_irqsave(&sspi->lock, flags);
    /* add msg to the sunxi_spi queue */
	list_add_tail(&msg->queue, &sspi->queue);
    /* add work to the workqueue,schedule the cpu. */
	queue_work(sspi->workqueue, &sspi->work);
	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;
}

/* interface 2, setup the frequency and default status */
static int sunxi_spi_setup(struct spi_device *spi)
{
	struct sunxi_spi *sspi = spi_master_get_devdata(spi->master);
	struct sunxi_spi_config *config = spi->controller_data;/* general is null. */
	unsigned long flags;

    /* just support 8 bits per word */
	if (spi->bits_per_word != 8)
		return -EINVAL;
    /* first check its valid,then set it as default select,finally set its */
    if(SUNXI_SPI_FAIL == sunxi_spi_check_cs(spi->chip_select, sspi)) {
        SPI_ERR("[spi-%d]: not support cs-%d \n", spi->master->bus_num, spi->chip_select);
        return -EINVAL;
    }
    if(spi->max_speed_hz > SPI_MAX_FREQUENCY)
	    return -EINVAL;
	if (!config) {
		config = kzalloc(sizeof *config, GFP_KERNEL);
		if (!config)
			return -ENOMEM;
		spi->controller_data = config;
	}
    /*
     * set the default vaule with spi device
     * can change by every spi transfer
     */
	config->bits_per_word = spi->bits_per_word;
	config->max_speed_hz  = spi->max_speed_hz;
	config->mode		  = spi->mode;

	spin_lock_irqsave(&sspi->lock, flags);
	/* if spi is free, then deactived the spi device */
	if (sspi->busy & SPI_FREE) {
		/* set chip select number */
	    spi_set_cs(spi->chip_select, sspi->base_addr);
	    /* deactivate chip select */
		sspi->cs_control(spi, 0);
	}
	spin_unlock_irqrestore(&sspi->lock, flags);

	return 0;
}

/* interface 3 */
static void sunxi_spi_cleanup(struct spi_device *spi)
{
    if(spi->controller_data) {
        kfree(spi->controller_data);
        spi->controller_data = NULL;
    }
}

/* get configuration in the script */
static void sunxi_spi_chan_cfg(struct sunxi_spi_platform_data *pdata)
{
    int i;
    script_item_u item = {0};
    script_item_value_type_e type = 0;
    char spi_para[16] = {0};

    for (i=0; i<SUNXI_SPI_NUM; i++) {
        sprintf(spi_para, SUNXI_SPI_DEV_NAME"%d", i);
		type = script_get_item(spi_para, "spi_used", &item);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("[spi-%d] has no spi_used!\n", i);
			continue;
		}
		if (item.val)
			spi_used_mask |= SUNXI_SPI_CHAN_MASK(i);

		type = script_get_item(spi_para, "spi_regulator", &item);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
			SPI_ERR("[spi-%d] has no spi_regulator.\n", i);
			continue;
		}
		strncpy(pdata[i].regulator_id, item.str, 16);
    }
}

static int sunxi_spi_chan_is_enable(int _ch)
{
	return spi_used_mask & SUNXI_SPI_CHAN_MASK(_ch);
}

static int sunxi_spi_select_gpio_state(struct pinctrl *pctrl, char *name, u32 no)
{
	int ret = 0;
	struct pinctrl_state *pctrl_state = NULL;

	pctrl_state = pinctrl_lookup_state(pctrl, name);
	if (IS_ERR(pctrl_state)) {
		SPI_ERR("SPI%d pinctrl_lookup_state(%s) failed! return %p \n", no, name, pctrl_state);
		return -1;
	}

	ret = pinctrl_select_state(pctrl, pctrl_state);
	if (ret < 0)
		SPI_ERR("SPI%d pinctrl_select_state(%s) failed! return %d \n", no, name, ret);

	return ret;
}

static int sunxi_spi_request_gpio(struct sunxi_spi *sspi)
{
	int bus_no = sspi->pdev->id;

	if (sspi->pctrl != NULL)
		return sunxi_spi_select_gpio_state(sspi->pctrl, PINCTRL_STATE_DEFAULT, bus_no);

	if (!sunxi_spi_chan_is_enable(bus_no))
		return -1;
	
	SPI_DBG("Pinctrl init %d ... [%s]\n", bus_no, sspi->pdev->dev.init_name);

	sspi->pctrl = devm_pinctrl_get(&sspi->pdev->dev);
	if (IS_ERR(sspi->pctrl)) {
		SPI_ERR("SPI%d devm_pinctrl_get() failed! return %ld\n", bus_no, PTR_ERR(sspi->pctrl));
		return -1;
	}

	return sunxi_spi_select_gpio_state(sspi->pctrl, PINCTRL_STATE_DEFAULT, bus_no);
}

static void sunxi_spi_release_gpio(struct sunxi_spi *sspi)
{
	devm_pinctrl_put(sspi->pctrl);
	sspi->pctrl = NULL;
}

#ifdef CONFIG_EVB_PLATFORM

static int sunxi_spi_get_cfg_csbitmap(int bus_num)
{
    script_item_u cs_bitmap;
    script_item_value_type_e type;
    char main_name[8] = {0};

	snprintf(main_name, 8, SUNXI_SPI_DEV_NAME"%d", bus_num);
    type = script_get_item(main_name, "spi_cs_bitmap", &cs_bitmap);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
        SPI_ERR("[spi-%d] get spi_cs_bitmap from sysconfig failed\n", bus_num);
        return 0;
    }

    return cs_bitmap.val;
}

static int sunxi_spi_clk_init(struct sunxi_spi *sspi, u32 mod_clk)
{
	int ret = 0;
	long rate = 0;

    sspi->pclk = clk_get(&sspi->pdev->dev, SPI_PLL_CLK);
	if (IS_ERR_OR_NULL(sspi->pclk)) {
		SPI_ERR("[spi-%d] Unable to acquire module clock '%s', return %x\n", 
				sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->pclk));
		return -1;
	}

    sspi->mclk = clk_get(&sspi->pdev->dev, sspi->dev_name);
	if (IS_ERR_OR_NULL(sspi->mclk)) {
		SPI_ERR("[spi-%d] Unable to acquire module clock '%s', return %x\n", 
				sspi->master->bus_num, sspi->dev_name, PTR_RET(sspi->mclk));
		return -1;
	}

	ret = clk_set_parent(sspi->mclk, sspi->pclk);
	if (ret != 0) {
		SPI_ERR("[spi-%d] clk_set_parent() failed! return %d\n", 
				sspi->master->bus_num, ret);
		return -1;
	}

	rate = clk_round_rate(sspi->mclk, mod_clk);
    if (clk_set_rate(sspi->mclk, rate)) {
        SPI_ERR("[spi-%d] spi clk_set_rate failed\n", sspi->master->bus_num);
        return -1;
    }

    SPI_INF("[spi-%d] mclk %u\n", sspi->master->bus_num,
           (unsigned)clk_get_rate(sspi->mclk));

	if (clk_prepare_enable(sspi->mclk)) {
		SPI_ERR("[spi-%d] Couldn't enable module clock 'spi'\n", sspi->master->bus_num);
		return -EBUSY;
	}

//	sunxi_periph_reset_deassert(sspi->mclk);

    return clk_get_rate(sspi->mclk);
}

static int sunxi_spi_clk_exit(struct sunxi_spi *sspi)
{
	if (IS_ERR_OR_NULL(sspi->mclk)) {
		SPI_ERR("[spi-%d] SPI mclk handle is invalid!\n", sspi->master->bus_num);
		return -1;
	}
	
//	sunxi_periph_reset_assert(sspi->mclk);
	clk_disable_unprepare(sspi->mclk);
	clk_put(sspi->mclk);
	clk_put(sspi->pclk);
	sspi->mclk = NULL;
	sspi->pclk = NULL;
	return 0;
}

#else

static int sunxi_spi_clk_init(struct sunxi_spi *sspi, u32 mod_clk)
{
	return 24000000;
}

static int sunxi_spi_clk_exit(struct sunxi_spi *sspi)
{
	return 0;
}

#endif

static int sunxi_spi_hw_init(struct sunxi_spi *sspi, struct sunxi_spi_platform_data *pdata)
{
	void __iomem *base_addr = sspi->base_addr;
	int sclk_freq = 0;

	if (spi_regulator_request(pdata) < 0) {
		SPI_ERR("[spi-%d] request regulator failed!\n", sspi->master->bus_num);
		return -1;
	}
	spi_regulator_enable(pdata);

	if (sunxi_spi_request_gpio(sspi) < 0) {
        SPI_ERR("[spi-%d] Request GPIO failed!\n", sspi->master->bus_num);
        return -1;
	}
	
	sclk_freq = sunxi_spi_clk_init(sspi, 100000000);
    if (sclk_freq < 0) {
        SPI_ERR("[spi-%d] sunxi_spi_clk_init(%s) failed!\n", sspi->master->bus_num, sspi->dev_name);
        return -1;
    }

	/* 1. enable the spi module */
	spi_enable_bus(base_addr);
	/* 2. set the default chip select */
	if(SUNXI_SPI_OK == sunxi_spi_check_cs(0, sspi)) {
	    spi_set_cs(0, base_addr);
	}
	else{
        spi_set_cs(1, base_addr);
	}
    /*
     * 3. master: set spi module clock;
     * 4. set the default frequency	10MHz
     */
    spi_set_master(base_addr);
    spi_set_clk(10000000, sclk_freq, base_addr);
    /*
     * 5. master : set POL,PHA,SSOPL,LMTF,DDB,DHB; default: SSCTL=0,SMC=1,TBW=0.
     * 6. set bit width-default: 8 bits
     */
    spi_config_tc(1, SPI_MODE_0, base_addr);
	spi_enable_tp(base_addr);
	/* 7. manual control the chip select */
	spi_ss_ctrl(base_addr, 1);
	/* 8. reset fifo */
	spi_reset_fifo(base_addr);
#ifdef CONFIG_ARCH_SUN8IW8
	spi_clear_dual_read(base_addr);
#endif
	return 0;
}

static int sunxi_spi_hw_exit(struct sunxi_spi *sspi, struct sunxi_spi_platform_data *pdata)
{
	/* disable the spi controller */
    spi_disable_bus(sspi->base_addr);

	/* disable module clock */
	sunxi_spi_clk_exit(sspi);
	sunxi_spi_release_gpio(sspi);
	
	spi_regulator_disable(pdata);
	spi_regulator_release(pdata);
	
	return 0;
}

static int __devinit sunxi_spi_probe(struct platform_device *pdev)
{
	struct resource	*mem_res;
	struct sunxi_spi *sspi;
	struct sunxi_spi_platform_data *pdata;
	struct spi_master *master;
	int ret = 0, err = 0, irq;
	int cs_bitmap = 0;

	if (pdev->id < 0) {
		SPI_ERR("Invalid platform device id-%d\n", pdev->id);
		return -ENODEV;
	}

	if (pdev->dev.platform_data == NULL) {
		SPI_ERR("platform_data missing!\n");
		return -ENODEV;
	}

	pdata = pdev->dev.platform_data;

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res == NULL) {
		SPI_ERR("Unable to get spi MEM resource\n");
		return -ENXIO;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		SPI_ERR("No spi IRQ specified\n");
		return -ENXIO;
	}

    /* create spi master */
	master = spi_alloc_master(&pdev->dev, sizeof(struct sunxi_spi));
	if (master == NULL) {
		SPI_ERR("Unable to allocate SPI Master\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);
	sspi = spi_master_get_devdata(master);
    memset(sspi, 0, sizeof(struct sunxi_spi));

	sspi->master        = master;
	sspi->irq           = irq;

#ifdef CONFIG_DMA_ENGINE
	sspi->dma_rx.dir	= SPI_DMA_RWNULL;
	sspi->dma_tx.dir	= SPI_DMA_RWNULL;
#endif
	sspi->cs_control	= sunxi_spi_cs_control;
	sspi->cs_bitmap		= pdata->cs_bitmap; /* cs0-0x1; cs1-0x2; cs0&cs1-0x3. */
	sspi->busy			= SPI_FREE;
	sspi->mode_type		= MODE_TYPE_NULL;

	master->bus_num         = pdev->id;
	master->setup           = sunxi_spi_setup;
	master->cleanup         = sunxi_spi_cleanup;
	master->transfer        = sunxi_spi_transfer;
	master->num_chipselect  = pdata->cs_num;
	/* the spi->mode bits understood by this driver: */
	master->mode_bits       = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH| SPI_LSB_FIRST;
    /* update the cs bitmap */
#ifdef CONFIG_EVB_PLATFORM
    cs_bitmap = sunxi_spi_get_cfg_csbitmap(pdev->id);
#else
	cs_bitmap = 0x1;
#endif
	if(cs_bitmap & 0x3){
	    sspi->cs_bitmap = cs_bitmap & 0x3;
	    SPI_INF("[spi-%d]: cs bitmap from cfg = 0x%x \n", master->bus_num, cs_bitmap);
	}

	snprintf(sspi->dev_name, sizeof(sspi->dev_name), SUNXI_SPI_DEV_NAME"%d", pdev->id);
	err = request_irq(sspi->irq, sunxi_spi_handler, IRQF_DISABLED, sspi->dev_name, sspi);
	if (err) {
		SPI_ERR("Cannot request IRQ\n");
		goto err0;
	}

	if (request_mem_region(mem_res->start,
			resource_size(mem_res), pdev->name) == NULL) {
		SPI_ERR("Req mem region failed\n");
		ret = -ENXIO;
		goto err1;
	}

	sspi->base_addr = ioremap(mem_res->start, resource_size(mem_res));
	if (sspi->base_addr == NULL) {
		SPI_ERR("Unable to remap IO\n");
		ret = -ENXIO;
		goto err2;
	}
	sspi->base_addr_phy = mem_res->start;

	sspi->workqueue = create_singlethread_workqueue(dev_name(master->dev.parent));
	if (sspi->workqueue == NULL) {
		SPI_ERR("Unable to create workqueue\n");
		ret = -EPERM;
		goto err5;
	}

    sspi->pdev = pdev;
	pdev->dev.init_name = sspi->dev_name;

	/* Setup Deufult Mode */
	ret = sunxi_spi_hw_init(sspi, pdata);
	if (ret != 0) {
		SPI_ERR("spi hw init failed!\n");
		goto err6;
	}

	spin_lock_init(&sspi->lock);
	init_completion(&sspi->done);
	INIT_WORK(&sspi->work, sunxi_spi_work);/* banding the process handler */
	INIT_LIST_HEAD(&sspi->queue);

	if (spi_register_master(master)) {
		SPI_ERR("cannot register SPI master\n");
		ret = -EBUSY;
		goto err7;
	}

	SPI_INF("allwinners SoC SPI Driver loaded for Bus SPI-%d with %d Slaves at most\n",
            pdev->id, master->num_chipselect);
	SPI_INF("[spi-%d]: driver probe succeed, base %p, irq %d!\n", master->bus_num, sspi->base_addr, sspi->irq);
	return 0;

err7:
	sunxi_spi_hw_exit(sspi, pdata);
err6:
	destroy_workqueue(sspi->workqueue);
err5:
	iounmap(sspi->base_addr);
err2:
	release_mem_region(mem_res->start, resource_size(mem_res));
err1:
	free_irq(sspi->irq, sspi);
err0:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return ret;
}

static int __devexit sunxi_spi_remove(struct platform_device *pdev)
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

	sunxi_spi_hw_exit(sspi, pdev->dev.platform_data);
	spi_unregister_master(master);
	destroy_workqueue(sspi->workqueue);

	iounmap(sspi->base_addr);
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem_res != NULL)
		release_mem_region(mem_res->start, resource_size(mem_res));

	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_spi_suspend(struct device *dev)
{
#ifdef CONFIG_EVB_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spi *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy |= SPI_SUSPND;
	spin_unlock_irqrestore(&sspi->lock, flags);

	while (sspi->busy & SPI_BUSY)
		msleep(10);

	spi_disable_bus(sspi->base_addr);
	sunxi_spi_clk_exit(sspi);

	sunxi_spi_select_gpio_state(sspi->pctrl, PINCTRL_STATE_SUSPEND, master->bus_num);
	spi_regulator_disable(dev->platform_data);

	SPI_INF("[spi-%d]: suspend okay.. \n", master->bus_num);
#endif

	return 0;
}

static int sunxi_spi_resume(struct device *dev)
{
#ifdef CONFIG_EVB_PLATFORM
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spi  *sspi = spi_master_get_devdata(master);
	unsigned long flags;

	sunxi_spi_hw_init(sspi, pdev->dev.platform_data);

	spin_lock_irqsave(&sspi->lock, flags);
	sspi->busy = SPI_FREE;
	spin_unlock_irqrestore(&sspi->lock, flags);
	SPI_INF("[spi-%d]: resume okay.. \n", master->bus_num);
#endif
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

static struct platform_driver sunxi_spi_driver = {
	.probe   = sunxi_spi_probe,
	.remove  = sunxi_spi_remove,
	.driver = {
        .name	= SUNXI_SPI_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm		= SUNXI_SPI_DEV_PM_OPS,
	},
};


/* ---------------- spi resouce and platform data start ---------------------- */

static struct resource sunxi_spi_resources[SUNXI_SPI_NUM * SUNXI_SPI_RES_NUM];
static struct sunxi_spi_platform_data sunxi_spi_pdata[SUNXI_SPI_NUM];
static struct platform_device sunxi_spi_device[SUNXI_SPI_NUM];

static void __init sunxi_spi_device_scan(void)
{
	int i;

	memset(sunxi_spi_device, 0, sizeof(sunxi_spi_device));
	memset(sunxi_spi_pdata, 0, sizeof(sunxi_spi_pdata));
	memset(sunxi_spi_resources, 0, sizeof(sunxi_spi_resources));

	for (i=0; i<SUNXI_SPI_NUM; i++) {
		sunxi_spi_resources[i * SUNXI_SPI_RES_NUM].start = SUNXI_SPI_MEM_START(i);
		sunxi_spi_resources[i * SUNXI_SPI_RES_NUM].end   = SUNXI_SPI_MEM_END(i);
		sunxi_spi_resources[i * SUNXI_SPI_RES_NUM].flags = IORESOURCE_MEM;

		sunxi_spi_resources[i * SUNXI_SPI_RES_NUM + 1].start = SUNXI_SPI_IRQ(i);
		sunxi_spi_resources[i * SUNXI_SPI_RES_NUM + 1].end   = SUNXI_SPI_IRQ(i);
		sunxi_spi_resources[i * SUNXI_SPI_RES_NUM + 1].flags = IORESOURCE_IRQ;

		sunxi_spi_pdata[i].cs_bitmap = SUNXI_CS_BITMAP(i);
		sunxi_spi_pdata[i].cs_num    = SUNXI_CS_NUM(i);
		
		sunxi_spi_device[i].name = SUNXI_SPI_DEV_NAME;
		sunxi_spi_device[i].id   = i;
		sunxi_spi_device[i].resource = &sunxi_spi_resources[i * SUNXI_SPI_RES_NUM];
		sunxi_spi_device[i].num_resources = SUNXI_SPI_RES_NUM;
		sunxi_spi_device[i].dev.platform_data = &sunxi_spi_pdata[i];		
	}
}

/* ---------------- spi resource and platform data end ----------------------- */

static ssize_t sunxi_spi_info_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct sunxi_spi_platform_data *pdata = dev->platform_data;

	return snprintf(buf, PAGE_SIZE, 
		"pdev->id   = %d \n"
		"pdev->name = %s \n"
		"pdev->num_resources = %u \n"
		"pdev->resource.mem = [0x%08x, 0x%08x] \n"
		"pdev->resource.irq = %d \n"
		"pdev->dev.platform_data.cs_bitmap = %d \n"
		"pdev->dev.platform_data.cs_num    = %d \n"
		"pdev->dev.platform_data.regulator = 0x%p \n"
		"pdev->dev.platform_data.regulator_id = %s \n",
		pdev->id, pdev->name, pdev->num_resources,
		pdev->resource[0].start, pdev->resource[0].end, pdev->resource[1].start,
		pdata->cs_bitmap, pdata->cs_num, pdata->regulator, pdata->regulator_id);
}
static struct device_attribute sunxi_spi_info_attr =
	__ATTR(info, S_IRUGO, sunxi_spi_info_show, NULL);

static ssize_t sunxi_spi_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct spi_master *master = dev_get_drvdata(dev);
	struct sunxi_spi *sspi = (struct sunxi_spi *)&master[1];
	char *spi_mode[] = {"Single mode, half duplex read", 
						"Single mode, half duplex write", 
						"Single mode, full duplex read and write", 
						"Dual mode, half duplex read",
						"Dual mode, half duplex write",
						"Null"};
	char *busy_state[] = {"Unknown", "Free", "Suspend", "Busy"};
	char *result_str[] = {"Success", "Fail"};
#ifdef CONFIG_DMA_ENGINE
	char *dma_dir[] = {"DMA NULL", "DMA read", "DMA write"};
#endif

	if (master == NULL)
		return snprintf(buf, PAGE_SIZE, "%s\n", "spi_master is NULL!");

	return snprintf(buf, PAGE_SIZE, 
		"master->bus_num = %d \n"
		"master->num_chipselect = %d \n"
		"master->dma_alignment  = %d \n"
		"master->mode_bits = %d \n"
		"master->flags = 0x%x, ->bus_lock_flag = 0x%x \n"
		"master->busy = %d, ->running = %d, ->rt = %d \n"
		"sspi->mode_type = %d [%s] \n"
		"sspi->irq = %d [%s] \n"
		"sspi->cs_bitmap = %d \n"
#ifdef CONFIG_DMA_ENGINE
		"sspi->dma_tx.dir = %d [%s] \n"
		"sspi->dma_rx.dir = %d [%s] \n"
#endif		
		"sspi->busy = %d [%s] \n"
		"sspi->result = %d [%s] \n"
		"sspi->base_addr = 0x%p, the SPI control register: \n"
		"[VER] 0x%02x = 0x%08x, [GCR] 0x%02x = 0x%08x, [TCR] 0x%02x = 0x%08x \n"
		"[ICR] 0x%02x = 0x%08x, [ISR] 0x%02x = 0x%08x, [FCR] 0x%02x = 0x%08x \n"
		"[FSR] 0x%02x = 0x%08x, [WCR] 0x%02x = 0x%08x, [CCR] 0x%02x = 0x%08x \n"
		"[BCR] 0x%02x = 0x%08x, [TCR] 0x%02x = 0x%08x, [BCC] 0x%02x = 0x%08x \n"
		"[DMA] 0x%02x = 0x%08x, [TXR] 0x%02x = 0x%08x, [RXD] 0x%02x = 0x%08x \n",
		master->bus_num, master->num_chipselect, master->dma_alignment, 
		master->mode_bits, master->flags, master->bus_lock_flag,	   	
		master->busy, master->running, master->rt,
		sspi->mode_type, spi_mode[sspi->mode_type],
		sspi->irq, sspi->dev_name, sspi->cs_bitmap, 
#ifdef CONFIG_DMA_ENGINE
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

static void sunxi_spi_sysfs(struct platform_device *_pdev)
{
	device_create_file(&_pdev->dev, &sunxi_spi_info_attr);
	device_create_file(&_pdev->dev, &sunxi_spi_status_attr);
}

static struct spi_board_info *spi_boards = NULL;
static int __init sunxi_spi_register_spidev(void)
{
    script_item_u spi_dev_num, temp_info;
    script_item_value_type_e type;
    int i, spidev_num;
    char spi_board_name[32] = {0};
    struct spi_board_info* board;

	type = script_get_item("spi_devices", "spi_dev_num", &spi_dev_num);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        SPI_ERR("Get spi devices number failed\n");
        return -1;
    }
	spidev_num = spi_dev_num.val;

    SPI_INF("[spi]: Found %d spi devices in config files\n", spidev_num);

    /* alloc spidev board information structure */
    spi_boards = (struct spi_board_info*)kzalloc(sizeof(struct spi_board_info) * spidev_num, GFP_KERNEL);
    if (spi_boards == NULL) {
        SPI_ERR("Alloc spi board information failed \n");
        return -1;
    }

    SPI_INF("%-16s %-16s %-16s %-8s %-4s %-4s\n", "boards_num", "modalias", "max_spd_hz", "bus_num", "cs", "mode");

    for (i=0; i<spidev_num; i++) {
        board = &spi_boards[i];
        sprintf(spi_board_name, "spi_board%d", i);

		type = script_get_item(spi_board_name, "modalias", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
			SPI_ERR("Get spi devices modalias failed\n");
            goto fail;
		}
		sprintf(board->modalias, "%s", temp_info.str);

		type = script_get_item(spi_board_name, "max_speed_hz", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices max_speed_hz failed\n");
            goto fail;
		}
		board->max_speed_hz = temp_info.val;

		type = script_get_item(spi_board_name, "bus_num", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices bus_num failed\n");
            goto fail;
		}
		board->bus_num = temp_info.val;

		type = script_get_item(spi_board_name, "chip_select", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices chip_select failed\n");
            goto fail;
		}
		board->chip_select = temp_info.val;

		type = script_get_item(spi_board_name, "mode", &temp_info);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			SPI_ERR("Get spi devices mode failed\n");
            goto fail;
		}
		board->mode = temp_info.val;

        SPI_INF("%-16d %-16s %-16d %-8d %-4d %-4d\n", i, board->modalias, board->max_speed_hz,
                board->bus_num, board->chip_select, board->mode);
    }

    /* register boards */
    if (spi_register_board_info(spi_boards, spidev_num)) {
        SPI_ERR("Register board information failed\n");
        goto fail;
    }

    return 0;

fail:
    if (spi_boards)
    {
        kfree(spi_boards);
        spi_boards = NULL;
    }

    return -1;
}

static int __init sunxi_spi_init(void)
{
    int i, ret = 0;

    sunxi_spi_device_scan();
    sunxi_spi_chan_cfg(sunxi_spi_pdata);

	ret = sunxi_spi_register_spidev();
	if (ret)
		SPI_ERR("register spi devices board info failed \n");

#ifdef CONFIG_EVB_PLATFORM
	for (i=0; i<SUNXI_SPI_NUM; i++)
#else
	i = CONFIG_SPI_CHAN_NUM; /* In FPGA, only one channel is available. */
#endif
	{
		if (sunxi_spi_chan_is_enable(i)) {
			SPI_DBG("Sunxi SPI init channel %d \n", i);
			ret = platform_device_register(&sunxi_spi_device[i]);
			if (ret < 0) {
				SPI_ERR("platform_device_register(%d) failed, return %d\n", i, ret);
				return ret;
			}
			sunxi_spi_sysfs(&sunxi_spi_device[i]);
		}
	}

    if (spi_used_mask)
		return platform_driver_register(&sunxi_spi_driver);

    SPI_INF("cannot find any using configuration for all spi controllers!\n");

    return 0;
}

static void __exit sunxi_spi_exit(void)
{
    if (spi_used_mask)
	    platform_driver_unregister(&sunxi_spi_driver);

    if (spi_boards)
		kfree(spi_boards);
}

module_init(sunxi_spi_init);
module_exit(sunxi_spi_exit);

MODULE_AUTHOR("pannan");
MODULE_DESCRIPTION("SUNXI SPI BUS Driver");
MODULE_ALIAS("platform:"SUNXI_SPI_DEV_NAME);
MODULE_LICENSE("GPL");
