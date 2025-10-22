/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Sunxi SD/MMC panic host driver
 *
 * Copyright (C) 2019 AllWinnertech Ltd.
 * Author: lixiang <lixiang@allwinnertech>
 *         cuizhikui<cuizhikui@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/reset.h>

#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>

#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>

#include "sunxi-mmc.h"
#include "sunxi-mmc-export.h"
#include <linux/sunxi_blkpstore.h>


#if IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
#define NCAT
#endif

struct bdev_info part;

#define SUNXI_TEST_SIZE	(512*4)

#define MWR_TO_NS		(250*1000*1000)
#define MWR_RFAIL	-1
#define MWR_ROK	0


#define mmc_mreadl(reg_base, reg) \
	({\
	 int val = readl(reg_base + SDXC_##reg);\
	 val;\
	 })
#define mmc_mwritel(reg_base, reg, value) \
	({\
	 int val = 0;\
	 writel((value), reg_base + SDXC_##reg);\
	 val;\
	 })

#define SDXC_REG_EDSD		(0x010C)
#define SDXC_REG_CSDC		(0x0054)
#define SDXC_REG_THLD		(0x0100)
#define SDXC_REG_DRV_DL		(0x0140)
#define SDXC_REG_SAMP_DL	(0x0144)
#define SDXC_REG_DS_DL		(0x0148)


#define SDXC_DAT_STARV_ERR	SDXC_VOLTAGE_CHANGE_DONE


#ifdef NCAT
#define SUNXI_MMC_GATR		(0xd2c)
#define SUNXI_MMC_MODR		(0xd20)
#define SUNXI_MMC_RST		(0xd2c)
#elif OCAT
#define SUNXI_MMC_GATR         (0x60)
#define SUNXI_MMC_MODR         (0x90)
#define SUNXI_MMC_RST          (0x2C0)
#else
#define SUNXI_MMC_GATR		(0x84c)
#define SUNXI_MMC_MODR		(0x838)
#define SUNXI_MMC_RST		(0x84c)
#endif

#ifdef MMC_DEBUG
#define mmcinfo(fmt...) printk(KERN_INFO "[mmc]: "fmt)
#define mmcdbg(fmt...)  printk(KERN_DEBUG "[mmc]: "fmt)
#define mmcerr(fmt...)  printk(KERN_ERR "[mmc]: "fmt)
#else
#define mmcinfo(fmt...) printk(KERN_INFO "[mmc]: "fmt)
#define mmcdbg(fmt...)
#define mmcerr(fmt...)	printk(KERN_ERR "[mmc]: "fmt)
#endif



struct sunxi_mmc_mbak_regs {
	u32 gctrl;
	u32 clkc;
	u32 timeout;
	u32 buswid;
	u32 waterlvl;
	u32 funcsel;
	u32 debugc;
	u32 idmacc;
	u32 dlba;
	u32 imask;
	u32 drv_dl;
	u32 samp_dl;
	u32 ds_dl;
	u32 edsd;
	u32 csdc;
	u32 sd_ntsr;
};

static struct sunxi_mmc_mbak_regs gmbak_regs;
static char *gccmu_base_reg;
static char *ghost_base_reg;
static bool need_dis_cmdq;


#ifdef NCAT
static void sunxi_mmc_mbusrst_host(char *host)
{
	char *ccmu_reg = gccmu_base_reg;

	u32 rval = 0;
	rval = readl(ccmu_reg + SUNXI_MMC_GATR);
	rval &= ~((1u<<0)|(1u<<16));
	writel(rval, ccmu_reg + SUNXI_MMC_GATR);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval &= ~((1<<31));
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval |= (1<<31);
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	rval = readl(ccmu_reg + SUNXI_MMC_GATR);
	rval |= ((1u<<0)|(1u<<16));
	writel(rval, ccmu_reg + SUNXI_MMC_GATR);
}
#elif OCAT
static void sunxi_mmc_mbusrst_host(char *host)
{
	char *ccmu_reg = gccmu_base_reg;
	u32 rval = 0;

	rval = readl(ccmu_reg + SUNXI_MMC_GATR);
	rval &= ~(1u<<10);
	writel(rval, ccmu_reg + SUNXI_MMC_GATR);

	rval = readl(ccmu_reg + SUNXI_MMC_RST);
	rval &= ~(1u<<10);
	writel(rval, ccmu_reg + SUNXI_MMC_RST);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval &= ~((1<<31));
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval |= (1<<31);
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	rval = readl(ccmu_reg + SUNXI_MMC_RST);
	rval |= (1u<<10);
	writel(rval, ccmu_reg + SUNXI_MMC_RST);

	rval = readl(ccmu_reg + SUNXI_MMC_GATR);
	rval |= (1u<<10);
	writel(rval, ccmu_reg + SUNXI_MMC_GATR);
}
#else
static void sunxi_mmc_mbusrst_host(char *host)
{
	char *ccmu_reg = gccmu_base_reg;

	u32 rval = 0;
	rval = readl(ccmu_reg + SUNXI_MMC_GATR);
	rval &= ~((1u<<2)|(1u<<18));
	writel(rval, ccmu_reg + SUNXI_MMC_GATR);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval &= ~((1<<31));
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval |= (1<<31);
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	rval = readl(ccmu_reg + SUNXI_MMC_GATR);
	rval |= ((1u<<2)|(1u<<18));
	writel(rval, ccmu_reg + SUNXI_MMC_GATR);
}
#endif

static const char mtsdat[512] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,

	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,

	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,

	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static void buf_dumphex32(char *name, const char *base, int len)
{
	u32 i;

	pr_cont("dump %s\n", name);
	for (i = 0; i < len; i += 4) {
		if (!(i&0xf))
			pr_cont("\n0x%px : ", base + i);
		pr_cont("0x%08x ", *((u32 *)&base[i]));
	}
	pr_cont("\n");
}

static void sunxi_mmc_regs_save(char *host)
{
	struct sunxi_mmc_mbak_regs *bak_regs = &gmbak_regs;

	/* save public register */
	bak_regs->gctrl = mmc_mreadl(host, REG_GCTRL);
	bak_regs->clkc = mmc_mreadl(host, REG_CLKCR);
	bak_regs->timeout = mmc_mreadl(host, REG_TMOUT);
	bak_regs->buswid = mmc_mreadl(host, REG_WIDTH);
	bak_regs->debugc = 0xdeb;

	bak_regs->drv_dl = mmc_mreadl(host, REG_DRV_DL);
	bak_regs->samp_dl = mmc_mreadl(host, REG_SAMP_DL);
	bak_regs->ds_dl = mmc_mreadl(host, REG_DS_DL);
	bak_regs->sd_ntsr = mmc_mreadl(host, REG_SD_NTSR);
	bak_regs->edsd = mmc_mreadl(host, REG_EDSD);
	bak_regs->csdc = mmc_mreadl(host, REG_CSDC);
}

static void sunxi_mmc_regs_restore(char *host)
{
	struct sunxi_mmc_mbak_regs *bak_regs = &gmbak_regs;
	char *ccmu_reg = gccmu_base_reg;
	u32 rval = 0;

	/* restore public register */
	mmc_mwritel(host, REG_GCTRL, bak_regs->gctrl);
	mmc_mwritel(host, REG_CLKCR, bak_regs->clkc);
	mmc_mwritel(host, REG_TMOUT, bak_regs->timeout);
	mmc_mwritel(host, REG_WIDTH, bak_regs->buswid);
	mmc_mwritel(host, REG_DBGC, bak_regs->debugc);

	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval &= ~((1<<31));
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);
	mmc_mwritel(host, REG_DRV_DL, bak_regs->drv_dl);
	rval = readl(ccmu_reg + SUNXI_MMC_MODR);
	rval |= (1<<31);
	writel(rval, ccmu_reg + SUNXI_MMC_MODR);

	mmc_mwritel(host, REG_SAMP_DL, bak_regs->samp_dl);
	mmc_mwritel(host, REG_DS_DL, bak_regs->ds_dl);
	mmc_mwritel(host, REG_SD_NTSR, bak_regs->sd_ntsr);
	mmc_mwritel(host, REG_EDSD, bak_regs->edsd);
	mmc_mwritel(host, REG_CSDC, bak_regs->csdc);
}

static int sunxi_mmc_mupdate_clk(char *host)
{
	u32 rval;
	int i = 0;

	rval = mmc_mreadl(host, REG_CLKCR);
	rval &= ~(SDXC_CARD_CLOCK_ON | SDXC_LOW_POWER_ON | SDXC_MASK_DATA0);

	rval |= SDXC_CARD_CLOCK_ON;
	rval |= SDXC_LOW_POWER_ON;
	rval |= SDXC_MASK_DATA0;

	mmc_mwritel(host, REG_CLKCR, rval);

	mmcdbg("%s REG_CLKCR:%x\n", __func__,
			mmc_mreadl(host, REG_CLKCR));

	rval = SDXC_START | SDXC_UPCLK_ONLY | SDXC_WAIT_PRE_OVER;
	mmc_mwritel(host, REG_CMDR, rval);

	do {
		rval = mmc_mreadl(host, REG_CMDR);
		ndelay(1);
	} while (((i++) < MWR_TO_NS) && (rval & SDXC_START));

	/* clear irq status bits set by the command */
	mmc_mwritel(host, REG_RINTR,
			mmc_mreadl(host, REG_RINTR) & ~SDXC_SDIO_INTERRUPT);

	if (rval & SDXC_START) {
		mmcerr("fatal err update clk timeout\n");
		buf_dumphex32("mmc host", host, 1024);
		return -EIO;
	}

	mmc_mwritel(host, REG_CLKCR,
			mmc_mreadl(host, REG_CLKCR) & ~SDXC_MASK_DATA0);

	return 0;
}



static void sunxi_mmc_rcover_host(char *host)
{
	sunxi_mmc_regs_save(host);
	sunxi_mmc_mbusrst_host(host);
	sunxi_mmc_regs_restore(host);
	sunxi_mmc_mupdate_clk(host);
}

static int sunxi_mmc_mchk_r1_rdy(char *reg, int to_ns)
{
	int i = 0;
	/* wait busy over */
	for (i = 0; i < to_ns; i++) {
		if (!(mmc_mreadl(reg, REG_STAS) & SDXC_CARD_DATA_BUSY))
			break;
		ndelay(1);
	}

	if ((mmc_mreadl(reg, REG_STAS) & SDXC_CARD_DATA_BUSY)) {
		printk("dead Wait r1 rdy failed\n");
		return MWR_RFAIL;
	}
	return MWR_ROK;
}


static int sunxi_mmc_raw_write(char *reg, u32 sec_addr,
		u32 sec_cnt, const char *inbuf)
{
	u32 cmd_val, ri;
	u32 rval;
	int to = 0;
	int wcnt = (sec_cnt<<9)>>2;
	int i = 0;
	u32 *buf = (u32 *)inbuf;

	rval = mmc_mreadl(reg, REG_GCTRL);
	rval |= SDXC_ACCESS_BY_AHB | SDXC_FIFO_RESET;
	rval &= ~SDXC_DMA_ENABLE_BIT;
	mmc_mwritel(reg, REG_GCTRL, rval);
	for (to = 0; to < MWR_TO_NS; to++) {
		rval = mmc_mreadl(reg, REG_GCTRL);
		if (!(rval & SDXC_FIFO_RESET))
			break;
		ndelay(1);
	}

	if (to == MWR_TO_NS) {
		mmcerr("wait fifo rest timout\n");
		goto eout;
	}

	/* cmd seting */
	cmd_val = SDXC_START | SDXC_RESP_EXPECT \
			  | SDXC_CHECK_RESPONSE_CRC | SDXC_DATA_EXPECT\
			  | SDXC_WRITE | SDXC_SEND_AUTO_STOP
			  | SDXC_WAIT_PRE_OVER | MMC_WRITE_MULTIPLE_BLOCK;
	mmc_mwritel(reg, REG_CARG, sec_addr);
	mmc_mwritel(reg, REG_A12A, 0);

	/* data setting */
	mmc_mwritel(reg, REG_BLKSZ, 512);
	mmc_mwritel(reg, REG_BCNTR, sec_cnt<<9);


	mmc_mwritel(reg, REG_THLD, (512<<16)|(1<<2)|(1<<0));
	mmcdbg("thld %x\n", readl(reg + 0x100));

	/* int seting */
	rval = mmc_mreadl(reg, REG_GCTRL);
	rval &= ~SDXC_INTERRUPT_ENABLE_BIT;
	mmc_mwritel(reg, REG_GCTRL, rval);
	mmc_mwritel(reg, REG_MISTA, 0);
	mmc_mwritel(reg, REG_RINTR, 0xffffffff);

	/* exe cmd */
	mmc_mwritel(reg, REG_CMDR, cmd_val);

	/* write data */
	for (i = 0; i < wcnt; i++) {
		/* wait data not full */
		for (to = 0; to < MWR_TO_NS; to++) {
			if (!(mmc_mreadl(reg, REG_STAS) & SDXC_FIFO_FULL))
				break;
			ri = mmc_mreadl(reg, REG_RINTR);
			if (ri & (SDXC_INTERRUPT_ERROR_BIT)) {
				mmcerr("trans err %x\n", ri);
				goto eout;
			}
			ndelay(1);
		}
		if (to == MWR_TO_NS) {
			mmcerr("wait fifo not full timeout\n");
			goto eout;
		}

		mmc_mwritel(reg, REG_FIFO, buf[i]);
	}

	/* wait busy over */
	for (i = 0; i < MWR_TO_NS; i++) {
		if (!(mmc_mreadl(reg, REG_STAS) & SDXC_CARD_DATA_BUSY))
			break;
		ndelay(1);
	}

	if ((mmc_mreadl(reg, REG_STAS) & SDXC_CARD_DATA_BUSY)) {
		mmcerr("dead Wait r1 rdy failed\n");
		goto eout;
	}

	for (to = 0; to < MWR_TO_NS; to++) {
		ri = mmc_mreadl(reg, REG_RINTR);
		if (ri & (SDXC_AUTO_COMMAND_DONE))
			break;
		ndelay(1);
	}
	if (to == MWR_TO_NS) {
		mmcerr("wait auto cmd done timout\n");
		goto eout;
	}

	mmc_mwritel(reg, REG_RINTR, 0xffff);
	mmcdbg("manul write ok\n");
	return MWR_ROK;

eout:
	mmcerr("mau write failed\n");
	return MWR_RFAIL;
}

static int sunxi_mmc_raw_wcmd_clr(char *reg, int *out_cmd_val)
{
	int i = 0;
	u32 cmd_val = 0;

	do {
		cmd_val = mmc_mreadl(reg, REG_CMDR);
		if (!(cmd_val & SDXC_START)) {
			*out_cmd_val = cmd_val;
			return MWR_ROK;
		}
		ndelay(1);
	} while ((i++) < MWR_TO_NS);

	mmcerr("Wait cmd over timout\n");
	return MWR_RFAIL;
}

static void sunxi_mmc_raw_send_cmd(struct mmc_command *cmd, char *reg)
{
	u32 cmd_val = SDXC_START | (cmd->opcode & 0x3f), ri;
	u32 imask = SDXC_INTERRUPT_ERROR_BIT;
	int rval = 0, i = 0;

	if (cmd->opcode == MMC_GO_IDLE_STATE) {
		cmd_val |= SDXC_SEND_INIT_SEQUENCE;
		imask |= SDXC_COMMAND_DONE;
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		cmd_val |= SDXC_RESP_EXPECT;
		if (cmd->flags & MMC_RSP_136)
			cmd_val |= SDXC_LONG_RESPONSE;
		if (cmd->flags & MMC_RSP_CRC)
			cmd_val |= SDXC_CHECK_RESPONSE_CRC;

		imask |= SDXC_COMMAND_DONE;
	} else {
		imask |= SDXC_COMMAND_DONE;
	}

	/* int seting */
	rval = mmc_mreadl(reg, REG_GCTRL);
	rval &= ~SDXC_INTERRUPT_ENABLE_BIT;
	mmc_mwritel(reg, REG_GCTRL, rval);
	mmc_mwritel(reg, REG_MISTA, 0);
	mmc_mwritel(reg, REG_RINTR, 0xffff);
	/* exe_cmd */
	mmc_mwritel(reg, REG_IMASK, imask);
	mmc_mwritel(reg, REG_CARG, cmd->arg);
	/*********************************************************/
	wmb();
	mmc_mwritel(reg, REG_CMDR, cmd_val);

	do {
		ri = mmc_mreadl(reg, REG_RINTR);
		if (ri & (SDXC_COMMAND_DONE |
					(SDXC_INTERRUPT_ERROR_BIT|SDXC_DAT_STARV_ERR)))
			break;
		ndelay(1);
	} while ((i++) < MWR_TO_NS);

	if (!(ri & SDXC_COMMAND_DONE) ||
			(ri & (SDXC_INTERRUPT_ERROR_BIT|SDXC_DAT_STARV_ERR))) {
		ri = mmc_mreadl(reg, REG_RINTR);
		if (!(ri & SDXC_COMMAND_DONE) ||
				(ri & (SDXC_INTERRUPT_ERROR_BIT|SDXC_DAT_STARV_ERR))) {
			mmcerr("send command (%u) failed, %xi\n", cmd->opcode, ri);
		} else {
			mmcdbg("send command (%u) ok\n", cmd->opcode);
		}
	} else
		mmcdbg("send cmd(%u) ok\n", cmd->opcode);


	mmc_mwritel(reg, REG_RINTR, 0xffff);
}

static void sunxi_mmc_raw_disable_cmdq(char *reg)
{
	struct mmc_command cmd;

	sunxi_mmc_rcover_host(reg);

	/* cmd12 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = MMC_STOP_TRANSMISSION;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;
	cmd.flags       &= ~MMC_RSP_CRC; /* Ignore CRC */
	sunxi_mmc_raw_send_cmd(&cmd, reg);
	sunxi_mmc_mchk_r1_rdy(reg, MWR_TO_NS);

	/* cmd48 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode       = MMC_CMDQ_TASK_MGMT;
	cmd.arg		= 1;
	cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;
	cmd.flags       &= ~MMC_RSP_CRC; /* Ignore CRC */
	sunxi_mmc_raw_send_cmd(&cmd, reg);
	sunxi_mmc_mchk_r1_rdy(reg, MWR_TO_NS);

	/* cmd6 disable cmdq */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode       = MMC_SWITCH;
	cmd.arg	= (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
			(EXT_CSD_CMDQ_MODE_EN << 16) |
			(0 << 8) |
			EXT_CSD_CMD_SET_NORMAL;
	sunxi_mmc_raw_send_cmd(&cmd, reg);

	sunxi_mmc_mchk_r1_rdy(reg, MWR_TO_NS);
}

static void sunxi_mmc_raw_stop(char *reg)
{
	u32 arg, cmd_val, ri;
	int i = 0;
	int rval = 0;

	cmd_val = SDXC_START | SDXC_RESP_EXPECT
		|SDXC_STOP_ABORT_CMD | SDXC_CHECK_RESPONSE_CRC
		|MMC_STOP_TRANSMISSION;
	arg = 0;

	/* int seting */
	rval = mmc_mreadl(reg, REG_GCTRL);
	rval &= ~SDXC_INTERRUPT_ENABLE_BIT;
	mmc_mwritel(reg, REG_GCTRL, rval);
	mmc_mwritel(reg, REG_MISTA, 0);
	mmc_mwritel(reg, REG_RINTR, 0xffff);

	mmc_mwritel(reg, REG_CARG, arg);
	mmc_mwritel(reg, REG_CMDR, cmd_val);

	do {
		ri = mmc_mreadl(reg, REG_RINTR);
		if (ri & (SDXC_COMMAND_DONE |
					(SDXC_INTERRUPT_ERROR_BIT|SDXC_DAT_STARV_ERR)))
			break;
		ndelay(1);
	} while ((i++) < MWR_TO_NS);

	if (!(ri & SDXC_COMMAND_DONE) ||
			(ri & (SDXC_INTERRUPT_ERROR_BIT|SDXC_DAT_STARV_ERR))) {
		ri = mmc_mreadl(reg, REG_RINTR);
		if (!(ri & SDXC_COMMAND_DONE) ||
				(ri & (SDXC_INTERRUPT_ERROR_BIT|SDXC_DAT_STARV_ERR))) {
			mmcdbg("send  manual stop command failed, %x\n", ri);
		} else {
			mmcdbg("send manual stop command ok\n");
		}
	} else
		mmcdbg("send manual stop command ok\n");


	mmc_mwritel(reg, REG_RINTR, 0xffff);
}

/* use for panic situation,no irq,no lock,no dma */
int sunxi_mmc_panic_write_do(u32 sec_addr, u32 sec_cnt, const char *inbuf)
{

	int ret = 0;
	u32 cmd_val = 0;
	char *reg = ghost_base_reg;

	cmd_val = mmc_mreadl(reg, REG_CMDR);
	BUG_ON(inbuf == NULL);
	if (!ghost_base_reg || !gccmu_base_reg) {
		mmcerr("host,ccmu reg has not init\n");
		return MWR_RFAIL;
	}

	ret = sunxi_mmc_raw_wcmd_clr(reg, &cmd_val);
	if (ret)
		return ret;

	if (cmd_val & SDXC_DATA_EXPECT)
		sunxi_mmc_raw_stop(reg);

	sunxi_mmc_rcover_host(reg);

	if (cmd_val & SDXC_DATA_EXPECT)
		sunxi_mmc_raw_stop(reg);

	if (cmd_val & SDXC_WRITE)
		ret = sunxi_mmc_mchk_r1_rdy(reg, MWR_TO_NS);
	if (ret)
		return ret;

	return sunxi_mmc_raw_write(reg, sec_addr, sec_cnt, inbuf);
}

static int sunxi_mmc_panic_resource_init(void)
{
	struct device_node *dn;
	struct resource res;

	need_dis_cmdq = true;
	dn = of_find_node_by_type(NULL, "sdc2");
	if (dn) {
		if (of_address_to_resource(dn, 0, &res)) {
			mmcerr("*get sdc2 host base address fail*\n");
			return MWR_RFAIL;
		}

		ghost_base_reg = ioremap(res.start, resource_size(&res));
		if (!ghost_base_reg) {
			mmcerr("*iormap host failed*\n");
			return MWR_RFAIL;
		}
	}

	dn = of_find_node_by_name(NULL, "ccu");
	if (dn) {
		if (of_address_to_resource(dn, 0, &res)) {
			mmcerr("*get ccu host base address fail*\n");
			return -ENXIO;
		}

		gccmu_base_reg = ioremap(res.start, resource_size(&res));
		if (!gccmu_base_reg) {
			mmcerr("*iormap ccmu failed*\n");
			return MWR_RFAIL;
		}

	}

	return MWR_ROK;
}

static void sunxi_mmc_panic_resource_release(void)
{
	iounmap(gccmu_base_reg);
	iounmap(ghost_base_reg);
}


static ssize_t sunxi_mmc_panic_write(const char *buf, size_t size, loff_t off)
{
	int ret = 0;
	struct mmc_command cmd;

	if (need_dis_cmdq) {
		need_dis_cmdq = false;
		sunxi_mmc_raw_disable_cmdq(ghost_base_reg);
	}

	mmcdbg("%s size:%llu off:%llu\n", __func__, (unsigned long long)size, (unsigned long long)off);

	if (((off + size) >> SECTOR_SHIFT) > part.nr_sects) {
		mmcerr("%s size + off (%llu + %llu) exceed (%llu)\n", __func__,
				(unsigned long long)size, (unsigned long long)off, part.nr_sects);
		return -EOVERFLOW;
	}

	ret = sunxi_mmc_panic_write_do(part.start_sect + (off >> SECTOR_SHIFT), size >> SECTOR_SHIFT, buf);
	if (ret)
		return ret;

	/* cmd6 flush cache */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode       = MMC_SWITCH;
	cmd.arg	= (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
			(EXT_CSD_FLUSH_CACHE << 16) |
			(1 << 8) |
			EXT_CSD_CMD_SET_NORMAL;
	sunxi_mmc_raw_send_cmd(&cmd, ghost_base_reg);
	sunxi_mmc_mchk_r1_rdy(ghost_base_reg, MWR_TO_NS);

	return 0;
}

static int sunxi_mmc_register_pstore_panic(enum pstore_blk_notifier_type type,
		struct pstore_device_info *pdi)
{
	switch (type) {
	case PSTORE_BLK_BACKEND_REGISTER:
	case PSTORE_BLK_BACKEND_PANIC_DRV_REGISTER:
		if (sunxi_mmc_panic_resource_init() != MWR_ROK)
			goto err;

		pdi->zone.panic_write = sunxi_mmc_panic_write;

		if (aw_pstore_blk_get_bdev_info(&part)) {
			pr_err("get bdev info fail\n");
			goto err;
		}


		pr_info("register pstore backend %s panic write\n", pdi->zone.name);
		break;

	case PSTORE_BLK_BACKEND_UNREGISTER:
	case PSTORE_BLK_BACKEND_PANIC_DRV_UNREGISTER:
		pdi->zone.panic_write = NULL;

		break;
	default:
		goto err;

	}

	return NOTIFY_OK;

err:
	return NOTIFY_BAD;
}

static struct pstore_blk_notifier sunxi_mmc_panic = {
	.notifier_call = sunxi_mmc_register_pstore_panic,
};

static int __init sunxi_mmc_panic_init(void)
{
	if (aw_register_pstore_blk_panic_notifier(&sunxi_mmc_panic) == NOTIFY_OK)
		return 0;
	else
		return -1;
}


static void __exit sunxi_mmc_panic_exit(void)
{
	aw_unregister_pstore_blk_panic_notifier(&sunxi_mmc_panic);

	sunxi_mmc_panic_resource_release();
}


module_init(sunxi_mmc_panic_init);
module_exit(sunxi_mmc_panic_exit);

MODULE_DESCRIPTION("Allwinner's SD/MMC Card Panic Pstore backend Support");
MODULE_AUTHOR("lixiang <lixiang@allwinnertech.com>");
MODULE_AUTHOR("cuizhikui <cuizhikui@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
