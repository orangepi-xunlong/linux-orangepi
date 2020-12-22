/*
 * drivers/mmc/host/sunxi-mci.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/device.h>


#include <linux/mmc/host.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>

#include <asm/cacheflush.h>
#include <asm/uaccess.h>


#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <mach/sunxi-chip.h>
#include <linux/clk/sunxi.h>
#include <linux/regulator/consumer.h>

#include "sunxi-mci.h"

static struct sunxi_mmc_host* sunxi_host[] = {NULL, NULL, NULL, NULL};

//#define CONFIG_CACULATE_TRANS_TIME
#define USE_AUTO_CMD12_ARG
//#define NO_USE_PIN

#if defined(CONFIG_ARCH_SUN8IW5P1)
	#define KEEP_CARD0_POWER_SUPPLY  /*used for sd card hold power when suspend*/
#endif
//#define USE_SECURE_WRITE_PROTECT 

#ifdef CONFIG_CACULATE_TRANS_TIME
static unsigned long long begin_time, over_time, end_time;
static volatile struct sunxi_mmc_idma_des *last_pdes;
#endif

#if 0
static void uart_put(char c)
{
	void __iomem *base = __io_address(0x01c28000);
	while (readl(base + 0x80) > 48);
	writel(c, base);
}

static void uart_puts(char* s)
{
	u32 i = strlen(s);
	while (i--)
		uart_put(*s++);
}

static void uart_puts_u32hex(u32 c)
{
	char ch = 0;
	uart_put('0');
	uart_put('x');
	ch = (c >> 28) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 24) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 20) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 16) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 12) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 8) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 4) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
	ch = (c >> 0) & 0xf;
	ch = ch > 9 ? 'a' + ch - 10 : '0' + ch;
	uart_put(ch);
}

static void hexdump(struct sunxi_mmc_host* smc_host, char* name, char* base, int len)
{
	u32 i;

	if (smc_host->dump_ctl)
	{
		printk("%s :", name);
		for (i=0; i<len; i++) {
			if (!(i&0x1f))
				printk("\n0x%p : ", base + i);
			if (!(i&0xf))
				printk(" ");
			printk("%02x ", readb(base + i));
		}
		printk("\n");
	}
}

#endif

#if 1
static void dumphex32(struct sunxi_mmc_host* smc_host, char* name, char* base, int len)
{
	u32 i;

	if (smc_host->dump_ctl)
	{
		printk("dump %s registers:", name);
		for (i=0; i<len; i+=4) {
			if (!(i&0xf))
				printk("\n0x%p : ", base + i);
			printk("0x%08x ", readl(base + i));
		}
		printk("\n");
	}
}


#else
#define dumphex32(fmt...)

#endif

#ifdef  USE_SECURE_WRITE_PROTECT
static int sunxi_check_secure_area(u32 start, u32 blkcnt)
{
	u32 sta_add = start;
	u32 end_add = start + blkcnt -1;
	u32 se_sta_add = SDMMC_SECURE_STORAGE_START_ADD;
	u32 se_end_add = SDMMC_SECURE_STORAGE_START_ADD+(SDMMC_ITEM_SIZE*2*MAX_SECURE_STORAGE_MAX_ITEM)-1;
	if(blkcnt<=(SDMMC_ITEM_SIZE*2*MAX_SECURE_STORAGE_MAX_ITEM)){
		if(((sta_add >= se_sta_add)&&(sta_add <= se_end_add))
			||((end_add >= se_sta_add)&&(end_add <= se_end_add))){
			return 1;
		}
	}else{
		if(((se_sta_add >= sta_add)&&(se_sta_add <= end_add))
			||((se_end_add >= sta_add)&&(se_end_add <= end_add))){
			return 1;
		}
	}

	return 0;
}
#endif

static s32 sunxi_mci_init_host(struct sunxi_mmc_host* smc_host)
{
	u32 rval;

	SMC_DBG(smc_host, "MMC Driver init host %d\n", smc_host->pdev->id);

	/* reset controller */
	rval = mci_readl(smc_host, REG_GCTRL) | SDXC_HWReset;
	mci_writel(smc_host, REG_GCTRL, rval);

	mci_writel(smc_host, REG_FTRGL, 0x70008);
	mci_writel(smc_host, REG_TMOUT, 0xffffffff);

	if(smc_host->cd_mode == CARD_DETECT_BY_D3){
		mci_writel(smc_host, REG_IMASK, SDXC_CardInsert|SDXC_CardRemove);
	}else{
		mci_writel(smc_host, REG_IMASK, 0);
	}
	mci_writel(smc_host, REG_RINTR, 0xffffffff);
	mci_writel(smc_host, REG_DBGC, 0xdeb);
	mci_writel(smc_host, REG_FUNS, 0xceaa0000);
	rval = mci_readl(smc_host, REG_GCTRL)|SDXC_INTEnb;
	rval &= ~SDXC_AccessDoneDirect;
	if(smc_host->cd_mode == CARD_DETECT_BY_D3){
		rval |= SDXC_DebounceEnb;
	}
	mci_writel(smc_host, REG_GCTRL, rval);

	smc_host->voltage = SDC_WOLTAGE_OFF;
	return 0;
}

s32 sunxi_mci_exit_host(struct sunxi_mmc_host* smc_host)
{
	u32 rval;

	SMC_DBG(smc_host, "MMC Driver exit host %d\n", smc_host->pdev->id);
	smc_host->ferror = 0;
	smc_host->voltage = SDC_WOLTAGE_OFF;

	rval = mci_readl(smc_host, REG_GCTRL) | SDXC_HWReset;
	mci_writel(smc_host, REG_GCTRL, SDXC_HWReset);
	return 0;
}

#ifdef CONFIG_ARCH_SUN9IW1
/*
	this function is only for card0 no gpio-f now.
*/
void sunxi_mci_change_gpio_bias(struct sunxi_mmc_host* smc_host,u32 vdd)
{
	void __iomem *gpio_bias_reg = NULL;

	switch(smc_host->pdev->id){
		case 0:
			gpio_bias_reg = __io_address(0x6000800+0x314);
			break;
		default:
			SMC_MSG(smc_host,"this card gpio bias setting is not support now\n");
			return;
	}

	SMC_DBG(smc_host,"before set gpio bias %x\n",readl(gpio_bias_reg));
	switch (vdd) {
		case SDC_WOLTAGE_3V3:
			writel(0xA, gpio_bias_reg);//we use 3v in fact,so not use 0xD
			break;
		case SDC_WOLTAGE_1V8:
			writel(0, gpio_bias_reg);
			break;
		case SDC_WOLTAGE_OFF:
			break;
		default:
			SMC_MSG(smc_host,"this card gpio bias setting is not support this voltage now\n");
			break;
	}
	SMC_DBG(smc_host,"after set gpio bias %x\n", readl(gpio_bias_reg));
}
#endif

s32 sunxi_mci_set_vddio(struct sunxi_mmc_host* smc_host, u32 vdd)
{
	char* vddstr[] = {"3.3V", "1.8V", "1.2V", "OFF", "ON"};
	static u32 on[4] = {0};
	u32 id = smc_host->pdev->id;
	s32 ret = 0;

	if (smc_host->regulator == NULL)
		return 0;
	BUG_ON(vdd > SDC_WOLTAGE_ON);
	switch (vdd) {
		case SDC_WOLTAGE_3V3:
			//regulator_set_voltage(smc_host->regulator, 3300000, 3300000);
			ret = regulator_set_voltage(smc_host->regulator, 3000000, 3000000);
			if (IS_ERR(smc_host->regulator)) {
				SMC_ERR(smc_host,"sdc%d regulator set voltage 3.0V failed!\n", id);
				return ret;
			}
			if (!on[id]) {
				SMC_ERR(smc_host, "regulator on\n");
				ret = regulator_enable(smc_host->regulator);
				if (IS_ERR(smc_host->regulator)) {
					SMC_ERR(smc_host,"sdc%d regulator enable failed 3.0V!\n", id);
					return ret;
				} else {
					SMC_DBG(smc_host, "sdc%d regulator enable successful 3.0V\n", id);
					on[id] = 1;
				}
			}
			break;
		case SDC_WOLTAGE_1V8:
			ret = regulator_set_voltage(smc_host->regulator, 1800000, 1800000);
			if (IS_ERR(smc_host->regulator)) {
				SMC_ERR(smc_host,"sdc%d regulator set voltage 1.8V failed!\n", id);
				return ret;
			}
			if (!on[id]) {
				SMC_DBG(smc_host, "regulator on\n");
				ret = regulator_enable(smc_host->regulator);
				if (IS_ERR(smc_host->regulator)) {
					SMC_ERR(smc_host,"sdc%d regulator enable failed! 1.8V\n", id);
					return ret;
				} else {
					SMC_DBG(smc_host, "sdc%d regulator enable successful 1.8V\n", id);
					on[id] = 1;
				}
			}
			break;
		case SDC_WOLTAGE_1V2:
			ret = regulator_set_voltage(smc_host->regulator, 1200000, 1200000);
			if (IS_ERR(smc_host->regulator)) {
				SMC_ERR(smc_host,"sdc%d regulator set voltage 1.2V failed!\n", id);
				return ret;
			}
			if (!on[id]) {
				SMC_DBG(smc_host, "regulator on\n");
				ret = regulator_enable(smc_host->regulator);
				if (IS_ERR(smc_host->regulator)) {
					SMC_ERR(smc_host,"sdc%d regulator enable failed! 1.2V\n", id);
					return ret;
				} else {
					SMC_DBG(smc_host, "sdc%d regulator enable successful 1.2V\n", id);
					on[id] = 1;
				}
			}
			break;
		case SDC_WOLTAGE_OFF:
			if (on[id]) {
				SMC_DBG(smc_host, "regulator off\n");
				ret = regulator_disable(smc_host->regulator);
				if (IS_ERR(smc_host->regulator)) {
					SMC_ERR(smc_host,"sdc%d regulator off failed!\n", id);
					return ret;
				} else {
					SMC_DBG(smc_host,"sdc%d regulator off successful!\n", id);
				}
				on[id] = 0;
			}
			break;
		case SDC_WOLTAGE_ON:
			if (!on[id]) {
				SMC_DBG(smc_host, "regulator on\n");
				ret = regulator_enable(smc_host->regulator);
				if (IS_ERR(smc_host->regulator)) {
					SMC_ERR(smc_host,"sdc%d regulator enable failed! 1.2V\n", id);
					return ret;
				} else {
					SMC_DBG(smc_host, "sdc%d regulator enable successful 1.2V\n", id);
					on[id] = 1;
				}
			}
			break;

	}
#ifdef CONFIG_ARCH_SUN9IW1
	sunxi_mci_change_gpio_bias(smc_host,vdd);
#endif

	SMC_DBG(smc_host, "sdc%d switch io voltage to %s\n", smc_host->pdev->id, vddstr[vdd]);
	return 0;
}

s32 sunxi_mci_get_vddio(struct sunxi_mmc_host* smc_host)
{
	int voltage;
	char *vol_str[4] = {"3.3V","1.8V","1.2V","0V"};

	if (smc_host->regulator == NULL)
		return 0;
	voltage = regulator_get_voltage(smc_host->regulator);
	if (IS_ERR(smc_host->regulator)) {
		SMC_ERR(smc_host, "Get vddio voltage fail !\n");
		return -1;
	}

	if (voltage > 1700000 && voltage < 1950000)
		smc_host->regulator_voltage = SDC_WOLTAGE_1V8;
	if (voltage > 2700000 && voltage < 3600000)
		smc_host->regulator_voltage = SDC_WOLTAGE_3V3;

	SMC_DBG(smc_host, "%s:card io voltage mode:%s\n", __FUNCTION__, vol_str[smc_host->regulator_voltage]);
	return 0 ;
}


#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)
static int card_power_on(struct sunxi_mmc_host* smc_host)
{
	int voltage;
	struct regulator *regu;
	int ret = 0;

	SMC_DBG(smc_host, "sdc%d card_power_on start...\n", smc_host->pdev->id);

	if (smc_host->pdata->power_supply == NULL) {
		SMC_MSG(NULL, "sdc%d power_supply is null\n", smc_host->pdev->id);
		return ret;
	}

	regu = regulator_get(NULL, smc_host->pdata->power_supply);
	if(IS_ERR(regu)) {
		SMC_MSG(smc_host, "sdc%d fail to get regulator\n", smc_host->pdev->id);
		return -1;
	}

	//set output voltage to 3V
	voltage = regulator_get_voltage(regu);
	if (IS_ERR(smc_host->regulator)) {
		SMC_ERR(smc_host, "get power supply voltage fail\n");
		return -1;
	}
	if (!(voltage > 2700000 && voltage < 3600000)) {
		ret = regulator_set_voltage(regu, 3000000, 3300000);
		if(ret) {
			SMC_MSG(NULL, "sdc%d fail to set regulator voltage 2.7~3.6V \n", smc_host->pdev->id);
			return ret;
		}
	}

	//enable regulator
	ret = regulator_enable(regu);
	if(ret) {
		SMC_MSG(NULL, "sdc%d fail to enable regulator\n", smc_host->pdev->id);
		return ret;
	}

	//put regulator when module exit
	regulator_put(regu);

	SMC_DBG(smc_host, "sdc%d card_power_on end ...ok\n", smc_host->pdev->id);
	return ret;
}

static int card_power_off(struct sunxi_mmc_host* smc_host)
{
	struct regulator *regu;
	int ret = 0;

	SMC_DBG(smc_host,"sdc%d card_power_off start...\n", smc_host->pdev->id);

	if(smc_host->pdata->power_supply == NULL) {
		SMC_MSG(NULL, "sdc%d power_supply is null\n", smc_host->pdev->id);
		return ret;
	}

	regu = regulator_get(NULL,smc_host->pdata->power_supply);
	if(IS_ERR(regu)) {
		SMC_MSG(NULL, "sdc%d fail to get regulator\n", smc_host->pdev->id);
		return -1;
	}

	//diable regulator
	ret = regulator_disable(regu);
	if(ret) {
		SMC_MSG(NULL, "sdc%d fail to disable regulator\n", smc_host->pdev->id);
		return ret;
	}

	//put regulator when module exit
	regulator_put(regu);

	SMC_DBG(smc_host, "sdc%d card_power_off end ...ok\n", smc_host->pdev->id);
	return ret;
}
#endif

s32 sunxi_mci_update_clk(struct sunxi_mmc_host* smc_host)
{
  	u32 rval;
	unsigned long expire = jiffies + msecs_to_jiffies(1000); // 1000ms timeout
  	s32 ret = 0;

  	rval = SDXC_Start|SDXC_UPCLKOnly|SDXC_WaitPreOver;
	if (smc_host->voltage_switching)
		rval |= SDXC_VolSwitch;
	mci_writel(smc_host, REG_CMDR, rval);

	do {
		rval = mci_readl(smc_host, REG_CMDR);
	} while (time_before(jiffies, expire) && (rval & SDXC_Start));

	if (rval & SDXC_Start) {
		smc_host->ferror = 1;
		SMC_ERR(smc_host, "update clock timeout, fatal error!!!\n");
		ret = -1;
	}

	if(!ret)
		SMC_INFO(smc_host, "update clock ok\n");
	return ret;
}


s32 sunxi_mci_set_clk_dly(struct sunxi_mmc_host* smc_host, u32 oclk_dly, u32 sclk_dly)
{
	u32 smc_no = smc_host->pdev->id;
#ifndef CONFIG_ARCH_SUN9IW1
	void __iomem *mclk_base = __io_address(0x01c20088 + 0x4 * smc_no);
#else
	void __iomem *mclk_base = __io_address(SUNXI_CCM_BASE + 0x10 + 0x4 * smc_no);
#endif
	u32 rval;
	unsigned long iflags;

	spin_lock_irqsave(&smc_host->lock, iflags);
	rval = readl(mclk_base);
	rval &= ~((0x7U << 8) | (0x7U << 20));
	rval |= (oclk_dly << 8) | (sclk_dly << 20);
	writel(rval, mclk_base);
	spin_unlock_irqrestore(&smc_host->lock, iflags);

	smc_host->oclk_dly = oclk_dly;
	smc_host->sclk_dly = sclk_dly;
	SMC_DBG(smc_host, "oclk_dly %d, sclk_dly %d\n", oclk_dly, sclk_dly);
	return 0;
}

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	||defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)
s32 sunxi_mci_ntsr_onoff(struct sunxi_mmc_host* smc_host, u32 newmode_en)
{
	u32 rval = mci_readl(smc_host, REG_NTSR);

	if (newmode_en)
		rval |= NEWMODE_ENABLE;
	else
		rval &= ~NEWMODE_ENABLE;
	mci_writel(smc_host, REG_NTSR, rval);
	SMC_DBG(smc_host, "ntsr_onoff REG_NTSR %x\n", mci_readl(smc_host, REG_NTSR));

	return 0;
}
#endif

s32 sunxi_mci_oclk_onoff(struct sunxi_mmc_host* smc_host, u32 oclk_en, u32 pwr_save)
{
	u32 rval = mci_readl(smc_host, REG_CLKCR);
	rval &= ~(SDXC_CardClkOn | SDXC_LowPowerOn);
	if (oclk_en)
		rval |= SDXC_CardClkOn;
	if (pwr_save || !smc_host->io_flag)
		rval |= SDXC_LowPowerOn;
	mci_writel(smc_host, REG_CLKCR, rval);
	sunxi_mci_update_clk(smc_host);
	return 0;
}

static void sunxi_mci_send_cmd(struct sunxi_mmc_host* smc_host, struct mmc_command* cmd)
{
	u32 imask = SDXC_IntErrBit;
	u32 cmd_val = SDXC_Start|(cmd->opcode&0x3f);
	unsigned long iflags;
	u32 wait = SDC_WAIT_NONE;

	wait = SDC_WAIT_CMD_DONE;
	if (cmd->opcode == MMC_GO_IDLE_STATE) {
		cmd_val |= SDXC_SendInitSeq;
		imask |= SDXC_CmdDone;
	}

	if (cmd->opcode == SD_SWITCH_VOLTAGE) {
		cmd_val |= SDXC_VolSwitch;
		imask |= SDXC_VolChgDone;
		smc_host->voltage_switching = 1;
		wait = SDC_WAIT_SWITCH1V8;
		/* switch controller to high power mode */
		sunxi_mci_oclk_onoff(smc_host, 1, 0);
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		cmd_val |= SDXC_RspExp;
		if (cmd->flags & MMC_RSP_136)
			cmd_val |= SDXC_LongRsp;
		if (cmd->flags & MMC_RSP_CRC)
			cmd_val |= SDXC_CheckRspCRC;

		if ((cmd->flags & MMC_CMD_MASK) == MMC_CMD_ADTC) {
			cmd_val |= SDXC_DataExp | SDXC_WaitPreOver;
			wait = SDC_WAIT_DATA_OVER;
			if (cmd->data->flags & MMC_DATA_STREAM) {
				imask |= SDXC_AutoCMDDone;
				cmd_val |= SDXC_Seqmod | SDXC_SendAutoStop;
				wait = SDC_WAIT_AUTOCMD_DONE;
			}
			if (cmd->data->stop) {
				imask |= SDXC_AutoCMDDone;
				cmd_val |= SDXC_SendAutoStop;
				wait = SDC_WAIT_AUTOCMD_DONE;
			} else
				imask |= SDXC_DataOver;

			if (cmd->data->flags & MMC_DATA_WRITE)
				cmd_val |= SDXC_Write;
			else
				wait |= SDC_WAIT_DMA_DONE;
		} else
			imask |= SDXC_CmdDone;

	} else
		imask |= SDXC_CmdDone;
	SMC_DBG(smc_host, "smc %d cmd %d(%08x) arg %x ie 0x%08x wt %x len %d\n",
		smc_host->pdev->id, cmd_val&0x3f, cmd->arg, cmd_val, imask, wait,
		smc_host->mrq->data ? smc_host->mrq->data->blksz * smc_host->mrq->data->blocks : 0);
	spin_lock_irqsave(&smc_host->lock, iflags);
	#ifdef CONFIG_CACULATE_TRANS_TIME
	begin_time = over_time = end_time = cpu_clock(smp_processor_id());
	#endif
	smc_host->wait = wait;
	smc_host->state = SDC_STATE_SENDCMD;
	mci_writew(smc_host, REG_IMASK, imask);
#ifdef USE_AUTO_CMD12_ARG
#if defined(CONFIG_ARCH_SUN8IW3) \
	|| defined(CONFIG_ARCH_SUN9IW1) \
	|| defined(CONFIG_ARCH_SUN8IW5) \
	|| defined(CONFIG_ARCH_SUN8IW6) \
	|| defined(CONFIG_ARCH_SUN8IW8) \
	|| defined(CONFIG_ARCH_SUN8IW7) \
	|| defined(CONFIG_ARCH_SUN8IW9)
	if(cmd_val & SDXC_SendAutoStop)
		mci_writel(smc_host, REG_A12A, 0);
	else
		mci_writel(smc_host, REG_A12A, 0xffff);
#endif
#endif
	mci_writel(smc_host, REG_CARG, cmd->arg);
	mci_writel(smc_host, REG_CMDR, cmd_val);
	smp_wmb();
	spin_unlock_irqrestore(&smc_host->lock, iflags);
}

static void sunxi_mci_init_idma_des(struct sunxi_mmc_host* smc_host, struct mmc_data* data)
{
	struct sunxi_mmc_idma_des* pdes = (struct sunxi_mmc_idma_des*)smc_host->sg_cpu;
	struct sunxi_mmc_idma_des* pdes_pa = (struct sunxi_mmc_idma_des*)smc_host->sg_dma;
	u32 des_idx = 0;
	u32 buff_frag_num = 0;
	u32 remain;
	u32 i, j;
	u32 config;

	for (i=0; i<data->sg_len; i++) {
		buff_frag_num = data->sg[i].length >> SDXC_DES_NUM_SHIFT;
		remain = data->sg[i].length & (SDXC_DES_BUFFER_MAX_LEN-1);
		if (remain)
			buff_frag_num ++;
		else
			remain = SDXC_DES_BUFFER_MAX_LEN;

		for (j=0; j < buff_frag_num; j++, des_idx++) {
			memset((void*)&pdes[des_idx], 0, sizeof(struct sunxi_mmc_idma_des));
			config = SDXC_IDMAC_DES0_CH|SDXC_IDMAC_DES0_OWN|SDXC_IDMAC_DES0_DIC;

			if (buff_frag_num > 1 && j != buff_frag_num-1)
				pdes[des_idx].data_buf1_sz = SDXC_DES_BUFFER_MAX_LEN;
			else
				pdes[des_idx].data_buf1_sz = remain;

			pdes[des_idx].buf_addr_ptr1 = sg_dma_address(&data->sg[i])
							+ j * SDXC_DES_BUFFER_MAX_LEN;
			if (i==0 && j==0)
				config |= SDXC_IDMAC_DES0_FD;

			if ((i == data->sg_len-1) && (j == buff_frag_num-1)) {
				config &= ~SDXC_IDMAC_DES0_DIC;
				config |= SDXC_IDMAC_DES0_LD|SDXC_IDMAC_DES0_ER;
				pdes[des_idx].buf_addr_ptr2 = 0;
				#ifdef CONFIG_CACULATE_TRANS_TIME
				last_pdes = &pdes[des_idx];
				#endif
			} else {
				pdes[des_idx].buf_addr_ptr2 = (u32)&pdes_pa[des_idx+1];
			}
			pdes[des_idx].config = config;
			SMC_INFO(smc_host, "sg %d, frag %d, remain %d, des[%d](%08x): "
					"[0] = %08x, [1] = %08x, [2] = %08x, [3] = %08x\n", i, j, remain,
				des_idx, (u32)&pdes[des_idx],
				(u32)((u32*)&pdes[des_idx])[0], (u32)((u32*)&pdes[des_idx])[1],
				(u32)((u32*)&pdes[des_idx])[2], (u32)((u32*)&pdes[des_idx])[3]);
		}
	}
	smp_wmb();
	//__asm("DSB");
	dsb();
	return;
}

static int sunxi_mci_prepare_dma(struct sunxi_mmc_host* smc_host, struct mmc_data* data)
{
	u32 dma_len;
	u32 i;
	u32 temp;
	struct scatterlist *sg;
	unsigned long expire = 0;

	if (smc_host->sg_cpu == NULL)
		return -ENOMEM;

	dma_len = dma_map_sg(mmc_dev(smc_host->mmc), data->sg, data->sg_len,
			(data->flags & MMC_DATA_WRITE) ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	if (dma_len == 0) {
		SMC_ERR(smc_host, "no dma map memory\n");
		return -ENOMEM;
	}

	for_each_sg(data->sg, sg, data->sg_len, i) {
		if (sg->offset & 3 || sg->length & 3) {
			SMC_ERR(smc_host, "unaligned scatterlist: os %x length %d\n",
				sg->offset, sg->length);
			return -EINVAL;
		}
	}

	sunxi_mci_init_idma_des(smc_host, data);
	temp = mci_readl(smc_host, REG_GCTRL);
	temp |= SDXC_DMAEnb;
	mci_writel(smc_host, REG_GCTRL, temp);
	temp |= SDXC_DMAReset;
	mci_writel(smc_host, REG_GCTRL, temp);
     expire = jiffies + msecs_to_jiffies(1);
     do {
		if(!(mci_readl(smc_host,REG_GCTRL)&SDXC_DMAReset)){
			break;
		}
     } while (time_before(jiffies, expire));

     if (mci_readl(smc_host,REG_GCTRL)&SDXC_DMAReset) {
         SMC_ERR(smc_host, "wait idma global rst timeout %d ms\n", 1);
         return -1;
     }

	mci_writel(smc_host, REG_DMAC, SDXC_IDMACSoftRST);
    expire = jiffies + msecs_to_jiffies(1);
     do {
		if(!(mci_readl(smc_host,REG_DMAC)&SDXC_IDMACSoftRST)){
			break;
		}
     } while (time_before(jiffies, expire));

     if (mci_readl(smc_host,REG_DMAC)&SDXC_IDMACSoftRST) {
         SMC_ERR(smc_host, "wait idma soft rst timeout %d ms\n", 1);
         return -1;
     }

	temp = SDXC_IDMACFixBurst|SDXC_IDMACIDMAOn;
	mci_writel(smc_host, REG_DMAC, temp);
	temp = mci_readl(smc_host, REG_IDIE);
	temp &= ~(SDXC_IDMACReceiveInt|SDXC_IDMACTransmitInt);
	if (data->flags & MMC_DATA_WRITE)
		temp |= SDXC_IDMACTransmitInt;
	else
		temp |= SDXC_IDMACReceiveInt;
	mci_writel(smc_host, REG_IDIE, temp);

	//write descriptor address to register
	mci_writel(smc_host, REG_DLBA, smc_host->sg_dma);
	mci_writel(smc_host, REG_FTRGL, smc_host->pdata->dma_tl);

	return 0;
}

int sunxi_mci_send_manual_stop(struct sunxi_mmc_host* smc_host, struct mmc_request* req)
{
	struct mmc_data* data = req->data;
	u32 cmd_val = SDXC_Start | SDXC_RspExp | SDXC_StopAbortCMD
			| SDXC_CheckRspCRC | MMC_STOP_TRANSMISSION;
	u32 iflags = 0;
	u32 imask = 0;
	int ret = 0;
	unsigned long expire = jiffies + msecs_to_jiffies(1000);

	if (!data) {
		SMC_ERR(smc_host, "no data request\n");
		return -1;
	}
	/* disable interrupt */
	imask = mci_readw(smc_host, REG_IMASK);
	mci_writew(smc_host, REG_IMASK, 0);

	mci_writel(smc_host, REG_CARG, 0);
	mci_writel(smc_host, REG_CMDR, cmd_val);
	do {
		iflags = mci_readw(smc_host, REG_RINTR);
	} while(!(iflags & (SDXC_CmdDone | SDXC_IntErrBit)) && time_before(jiffies, expire));

	if (iflags & SDXC_IntErrBit) {
		SMC_ERR(smc_host, "sdc %d send stop command failed\n", smc_host->pdev->id);
		ret = -1;
	}

	if (req->stop)
		req->stop->resp[0] = mci_readl(smc_host, REG_RESP0);

	mci_writew(smc_host, REG_RINTR, iflags);

	/* enable interrupt */
	mci_writew(smc_host, REG_IMASK, imask);

	return ret;
}

void sunxi_mci_dump_errinfo(struct sunxi_mmc_host* smc_host)
{
	void __iomem *mclk_base = NULL;

	SMC_ERR(smc_host, "smc %d err, cmd %d, %s%s%s%s%s%s%s%s%s%s !!\n",
		smc_host->pdev->id, smc_host->mrq->cmd ? smc_host->mrq->cmd->opcode : -1,
		smc_host->int_sum & SDXC_RespErr     ? " RE"     : "",
		smc_host->int_sum & SDXC_RespCRCErr  ? " RCE"    : "",
		smc_host->int_sum & SDXC_DataCRCErr  ? " DCE"    : "",
		smc_host->int_sum & SDXC_RespTimeout ? " RTO"    : "",
		smc_host->int_sum & SDXC_DataTimeout ? " DTO"    : "",
		smc_host->int_sum & SDXC_DataStarve  ? " DS"     : "",
		smc_host->int_sum & SDXC_FIFORunErr  ? " FE"     : "",
		smc_host->int_sum & SDXC_HardWLocked ? " HL"     : "",
		smc_host->int_sum & SDXC_StartBitErr ? " SBE"    : "",
		smc_host->int_sum & SDXC_EndBitErr   ? " EBE"    : ""
		);
		dumphex32(smc_host, "mmc", IOMEM(IO_ADDRESS(SMC_BASE(smc_host->pdev->id))), 0x100);
#ifndef CONFIG_ARCH_SUN9IW1
		mclk_base = __io_address(SUNXI_CCM_BASE + 0x88 + 0x4 * smc_host->pdev->id);
#else
		mclk_base = __io_address(SUNXI_CCM_BASE + 0x10 + 0x4 * smc_host->pdev->id);
#endif
		dumphex32(smc_host, "ccmu", mclk_base, 0x4);
		dumphex32(smc_host, "gpio", IO_ADDRESS(SUNXI_PIO_BASE), 0x120);
}

s32 sunxi_mci_wait_access_done(struct sunxi_mmc_host* smc_host)
{
	s32 own_set = 0;
	unsigned long expire = jiffies + msecs_to_jiffies(5);
	while (!(mci_readl(smc_host, REG_GCTRL) & SDXC_MemAccessDone) && time_before(jiffies, expire));
	if (!(mci_readl(smc_host, REG_GCTRL) & SDXC_MemAccessDone)) {
		SMC_MSG(smc_host, "wait memory access done timeout !!\n");
	}
	#ifdef CONFIG_CACULATE_TRANS_TIME
	expire = jiffies + msecs_to_jiffies(5);
	while (last_pdes->config & SDXC_IDMAC_DES0_OWN && time_before(jiffies, expire)) {
		own_set = 1;
	}
	if (time_after(jiffies, expire)) {
		SMC_MSG(smc_host, "wait last pdes own bit timeout, lc %x, rc %x\n",
			last_pdes->config, mci_readl(smc_host, REG_CHDA));
	}

	end_time = cpu_clock(smp_processor_id());
	if (own_set || (end_time - over_time) > 1000000) {
		SMC_MSG(smc_host, "Own set %x, t1 %llu t2 %llu t3 %llu, d1 %llu d2 %llu, bcnt %x\n",
			own_set, begin_time, over_time, end_time, over_time - begin_time,
			end_time - over_time, mci_readl(smc_host, REG_BCNTR));
		own_set = 0;
	}
	#endif
	return own_set;
}

s32 sunxi_mci_request_done(struct sunxi_mmc_host* smc_host)
{
	struct mmc_request* req = smc_host->mrq;
	u32 temp;
	s32 ret = 0;

	if (smc_host->int_sum & SDXC_IntErrBit) {
		/* if we got response timeout error information, we should check
		   if the command done status has been set. if there is no command
		   done information, we should wait this bit to be set */
		if ((smc_host->int_sum & SDXC_RespTimeout) && !(smc_host->int_sum & SDXC_CmdDone)) {
			u32 rint;
			unsigned long expire = jiffies + 1;
			do {
				rint = mci_readl(smc_host, REG_RINTR);
			} while (time_before(jiffies, expire) && !(rint & SDXC_CmdDone));
		}

		//sunxi_mci_dump_errinfo(smc_host);
		if (req->data)
			SMC_DBG(smc_host, "In data %s operation\n",
				req->data->flags & MMC_DATA_WRITE ? "write" : "read");
		ret = -1;
		goto out;
	}

	if (req->cmd) {
		if (req->cmd->flags & MMC_RSP_136) {
			req->cmd->resp[0] = mci_readl(smc_host, REG_RESP3);
			req->cmd->resp[1] = mci_readl(smc_host, REG_RESP2);
			req->cmd->resp[2] = mci_readl(smc_host, REG_RESP1);
			req->cmd->resp[3] = mci_readl(smc_host, REG_RESP0);
		} else {
			req->cmd->resp[0] = mci_readl(smc_host, REG_RESP0);
		}
	}

out:
	if (req->data) {
		struct mmc_data* data = req->data;
		unsigned long expire = 0;

		sunxi_mci_wait_access_done(smc_host);
		mci_writel(smc_host, REG_IDST, 0x337);
		mci_writel(smc_host, REG_IDIE, 0);
		mci_writel(smc_host, REG_DMAC, 0);
		temp = mci_readl(smc_host, REG_GCTRL);
		mci_writel(smc_host, REG_GCTRL, temp|SDXC_DMAReset);
		temp &= ~SDXC_DMAEnb;
		mci_writel(smc_host, REG_GCTRL, temp);
		expire = jiffies + msecs_to_jiffies(1);
		do {
			if(!(mci_readl(smc_host,REG_GCTRL)&SDXC_DMAReset)){
				break;
			}
		} while (time_before(jiffies, expire));

		if (mci_readl(smc_host,REG_GCTRL)&SDXC_DMAReset) {
			SMC_ERR(smc_host, "wait idma global rst timeout %d ms\n", 1);
			return -1;
		}

		temp |= SDXC_FIFOReset;
		mci_writel(smc_host, REG_GCTRL, temp);

		expire = jiffies + msecs_to_jiffies(1);
		do {
			if(!(mci_readl(smc_host,REG_GCTRL)&SDXC_FIFOReset)){
				break;
			}
		} while (time_before(jiffies, expire));

		if (mci_readl(smc_host,REG_GCTRL)&SDXC_FIFOReset) {
			SMC_ERR(smc_host, "wait idma rst timeout %d ms\n", 1);
			return -1;
		}

		dma_unmap_sg(mmc_dev(smc_host->mmc), data->sg, data->sg_len,
                                data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
	}

	mci_writew(smc_host, REG_IMASK, 0);
	if (smc_host->int_sum & (SDXC_RespErr | SDXC_HardWLocked | SDXC_RespTimeout)) {
		SMC_DBG(smc_host, "sdc %d abnormal status: %s\n", smc_host->pdev->id,
			smc_host->int_sum & SDXC_HardWLocked ? "HardWLocked" : "RespErr");
	}

	mci_writew(smc_host, REG_RINTR, 0xffff);

	SMC_DBG(smc_host, "smc %d done, resp %08x %08x %08x %08x\n", smc_host->pdev->id,
		req->cmd->resp[0], req->cmd->resp[1], req->cmd->resp[2], req->cmd->resp[3]);

	//if (req->data  && (smc_host->int_sum & SDXC_IntErrBit)) {
	//	SMC_MSG(smc_host, "found data error, need to send stop command !!\n");
	//	sunxi_mci_send_manual_stop(smc_host, req);
	//}

	return ret;
}

#ifndef CONFIG_ARCH_SUN9IW1
static void mclk_enable_disable(struct sunxi_mmc_host* smc_host,u32 enable)
{
#ifndef CONFIG_ARCH_SUN9IW1
		void __iomem *mclk_base = __io_address(SUNXI_CCM_BASE + 0x88 + 0x4 * smc_host->pdev->id);
#else
		void __iomem *mclk_base = __io_address(SUNXI_CCM_BASE + 0x10 + 0x4 * smc_host->pdev->id);
#endif
	if (enable) {
		writel(readl(mclk_base) | (1U << 31),mclk_base);
	} else {
		writel(readl(mclk_base) & ~(1U << 31),mclk_base);
	}

	SMC_DBG(smc_host,"mclk 0x%08x 0x%08x\n", (u32)mclk_base, readl(mclk_base));
}
#endif

/* static s32 sunxi_mci_set_clk(struct sunxi_mmc_host* smc_host, u32 clk);
 * set clock and the phase of output/input clock incording on
 * the different timing condition
 */
static int sunxi_mci_set_clk(struct sunxi_mmc_host* smc_host)
{
	struct clk *sclk = NULL;
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		||defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	struct clk* mclk2 = NULL;
	u32 source_rate = 0;
	u32 sdmmc2mod_rate = 0;
#endif
	u32 mod_clk = 0;
	u32 src_clk = 0;
	u32 temp;
	u32 oclk_dly = 2;
	u32 sclk_dly = 2;
	struct sunxi_mmc_clk_dly* dly = NULL;
	struct sunxi_mmc_clk_dly* mmc_clk_dly = smc_host->pdata->mmc_clk_dly;
	s32 err;
	u32 rate;
	void __iomem *mclk_base = NULL;
	u32 clk = 0;
	char* sclk_name = NULL;

	mod_clk = smc_host->mod_clk;
	if (smc_host->card_clk <= 400000) {
		sclk_name = MMC_SRCCLK_HOSC;
		sclk = clk_get(&smc_host->pdev->dev, sclk_name);
	} else {
		sclk_name = MMC_SRCCLK_PLL;
		sclk = clk_get(&smc_host->pdev->dev, sclk_name);
	}
	if (NULL==sclk || IS_ERR(sclk)) {
		SMC_ERR(smc_host, "Error to get source clock %s %d\n", sclk_name, (int)sclk);
		return -1;
	}

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		|| defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	if (smc_host->pdev->id == 2)
	{
		/*  1. MMC_2MOD_CLK's parent can be choosed by calling clk_set_parent();
			2. smc_host->mclk's parent is fixed on MMC_2MOD_CLK;
			3. MMC_2MOD_CLK's frequency can be changed by calling clk_set_rate().
			4. when MMC_2MOD_CLK's frequency is same as its parent, controller uses 1x clock;
			   when MMC_2MOD_CLK's frequency is half of its parent, contorller uses 2x clock;
		*/
		SMC_DBG(smc_host, "2xclk mode %d\n", smc_host->pdata->used_2xmod);
		if (smc_host->pdata->used_2xmod == 1)
		{
			sunxi_mci_ntsr_onoff(smc_host, 1);
		}
		mclk2 = clk_get(&smc_host->pdev->dev, MMC_2MOD_CLK);
		if (NULL== mclk2 || IS_ERR(mclk2)) {
			SMC_ERR(smc_host, "Error to get source clock for clk %dHz\n", clk);
			return -1;
		}
		err = clk_set_parent(mclk2, sclk);
		source_rate = clk_get_rate(sclk);
		if (smc_host->pdata->used_2xmod == 1)
		{
			sdmmc2mod_rate = source_rate/2;
		}
		else
		{
			sdmmc2mod_rate = source_rate;
		}
		clk_set_rate(mclk2, sdmmc2mod_rate); /* in fact, we enable 2x clock mode by call this func. */
		clk_put(mclk2);
	}
	else
#endif
#if defined(CONFIG_ARCH_SUN8IW7P1)
	if (smc_host->pdev->id == 1)
	{
		/*	1. MMC_1MOD_CLK's parent can be choosed by calling clk_set_parent();
			2. smc_host->mclk's parent is fixed on MMC_2MOD_CLK;
			3. MMC_1MOD_CLK's frequency can be changed by calling clk_set_rate().
			4. when MMC_1MOD_CLK's frequency is same as its parent, controller uses 1x clock;
			   when MMC_1MOD_CLK's frequency is half of its parent, contorller uses 2x clock;
		*/
		SMC_DBG(smc_host, "2xclk mode %d\n", smc_host->pdata->used_2xmod);
		if (smc_host->pdata->used_2xmod == 1)
		{
			sunxi_mci_ntsr_onoff(smc_host, 1);
		}
		mclk2 = clk_get(&smc_host->pdev->dev, MMC_1MOD_CLK);
		if (NULL== mclk2 || IS_ERR(mclk2)) {
			SMC_ERR(smc_host, "Error to get source clock for clk %dHz\n", clk);
			return -1;
		}
		err = clk_set_parent(mclk2, sclk);
		source_rate = clk_get_rate(sclk);
		if (smc_host->pdata->used_2xmod == 1)
		{
			sdmmc2mod_rate = source_rate/2;
		}
		else
		{
			sdmmc2mod_rate = source_rate;
		}
		clk_set_rate(mclk2, sdmmc2mod_rate); /* in fact, we enable 2x clock mode by call this func. */
		clk_put(mclk2);
	}
	else
#endif
	{
		err = clk_set_parent(smc_host->mclk, sclk);
	}

	if (err) {
		SMC_ERR(smc_host, "sdc%d set mclk parent error\n", smc_host->pdev->id);
		clk_put(sclk);
		return -1;
	}
	rate = clk_round_rate(smc_host->mclk, mod_clk);

	SMC_DBG(smc_host, "sdc%d before set round clock %d\n", smc_host->pdev->id, rate);
	{
#ifndef CONFIG_ARCH_SUN9IW1
		mclk_base = __io_address(0x01c20088 + 0x4 * smc_host->pdev->id);
#else
		mclk_base = __io_address(SUNXI_CCM_BASE + 0x10 + 0x4 * smc_host->pdev->id);
#endif
		SMC_DBG(smc_host, "mclk 0x%08x 0x%08x\n", (u32)mclk_base, readl(mclk_base));
	}

	//disable mclk first before change mclk
#ifdef CONFIG_ARCH_SUN9IW1
	clk_disable_unprepare(smc_host->mclk);
#else
	mclk_enable_disable(smc_host, 0);
#endif

	err = clk_set_rate(smc_host->mclk, rate);
	if (err) {
		SMC_ERR(smc_host, "sdc%d set mclk rate error, rate %dHz\n",
						smc_host->pdev->id, rate);
		clk_put(sclk);
		return -1;
	}

	//mclk_enable_disable(smc_host,1);
#ifdef CONFIG_ARCH_SUN9IW1
	clk_prepare_enable(smc_host->mclk);
#else
	mclk_enable_disable(smc_host,1);
#endif

	src_clk = clk_get_rate(sclk);
	clk_put(sclk);

	/* update clock info in smc_host */
	clk = smc_host->card_clk = rate;

	SMC_DBG(smc_host, "sdc%d set round clock %d, soure clk is %d\n", smc_host->pdev->id, rate, src_clk);
	{
#ifndef CONFIG_ARCH_SUN9IW1
		mclk_base = __io_address(0x01c20088 + 0x4 * smc_host->pdev->id);
#else
		mclk_base = __io_address(SUNXI_CCM_BASE + 0x10 + 0x4 * smc_host->pdev->id);
#endif
		SMC_DBG(smc_host,"mclk 0x%08x 0x%08x\n", (u32)mclk_base, readl(mclk_base));
	}
	sunxi_mci_oclk_onoff(smc_host, 0, 0);

	/* clear internal divider */
	temp = mci_readl(smc_host, REG_CLKCR);
	temp &= ~0xff;

	/* set internal divider */
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		|| defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	if (smc_host->pdata->used_2xmod)
	{
		if (smc_host->ddr) {
			temp |= 1;
			SMC_DBG(smc_host,"internal div %x\n", temp);
			smc_host->card_clk = smc_host->mod_clk / 2;
			clk = smc_host->card_clk;
			SMC_DBG(smc_host,"card clk%d \n",smc_host->card_clk);
		}
	}
	else
#endif
	{
		if (smc_host->ddr && smc_host->bus_width == 8) {
			temp |= 1;
			SMC_DBG(smc_host,"internal div %x\n", temp);
			smc_host->card_clk = smc_host->mod_clk / 2;
			clk = smc_host->card_clk;
			SMC_DBG(smc_host,"card clk%d \n",smc_host->card_clk);
		}
	}

#ifdef MMC_FPGA
	{
		/* support internal divide clock under fpga environment  */
		temp = mci_readl(smc_host, REG_CLKCR);
		temp &= ~0xff;

		temp |= 24000000 / smc_host->mod_clk / 2; // =24M/400K/2=0x1E
		mci_writel(smc_host, REG_CLKCR, temp);
		SMC_MSG(NULL, "--FPGA REG_CLKCR: 0x%08x \n", mci_readl(smc_host, REG_CLKCR));
	}
#endif

	mci_writel(smc_host, REG_CLKCR, temp);
	sunxi_mci_oclk_onoff(smc_host, 0, 0);

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		|| defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	if ((smc_host->pdev->id == 2) && (smc_host->pdata->used_2xmod == 1))
	{
		//2xmod phase, user default value
	}
	else
#endif
#if defined(CONFIG_ARCH_SUN8IW7P1)
	if ((smc_host->pdev->id == 1) && (smc_host->pdata->used_2xmod == 1))
	{
		//2xmod phase, user default value
	}
	else
#endif
	{
		if (clk <= 400000) {
			dly = &mmc_clk_dly[MMC_CLK_400K];
		} else if (clk <= 25000000) {
			dly = &mmc_clk_dly[MMC_CLK_25M];
		} else if (clk <= 50000000) {
			if (smc_host->ddr) {
				if (smc_host->bus_width == 8)
					dly = &mmc_clk_dly[MMC_CLK_50MDDR_8BIT];
				else
					dly = &mmc_clk_dly[MMC_CLK_50MDDR];
			} else {
				dly = &mmc_clk_dly[MMC_CLK_50M];
			}
		} else if (clk <= 104000000) {
			dly = &mmc_clk_dly[MMC_CLK_100M];
		} else if (clk <= 208000000) {
			dly = &mmc_clk_dly[MMC_CLK_200M];
		} else
			dly = &mmc_clk_dly[MMC_CLK_50M];
		oclk_dly = dly->oclk_dly;
		sclk_dly = dly->sclk_dly;
		sunxi_mci_set_clk_dly(smc_host, oclk_dly, sclk_dly);
	}
	sunxi_mci_oclk_onoff(smc_host, 1, 0);
	return 0;
}

static int sunxi_mci_request_mmcio(struct sunxi_mmc_host *smc_host)
{
#ifndef NO_USE_PIN

	s32 ret = 0;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	u32 smc_no = smc_host->pdev->id;
	struct gpio_config *gpio;
	u32 i;
	char               pin_name[SUNXI_PIN_NAME_MAX_LEN] = {0};
	unsigned long      config;

	SMC_DBG(smc_host,"device [%s] probe enter\n", dev_name(&smc_host->pdev->dev));

	/* signal io */
	for (i=0; i<pdata->width+2; i++) {
		gpio = &pdata->mmcio[i];
		sunxi_gpio_to_name(gpio->gpio, pin_name);

		SMC_DBG(smc_host,"pin name %s,pull%d,drv_level%d,muxsel%d,gpio index %d\n",\
							pin_name,gpio->pull,gpio->drv_level,gpio->mul_sel,gpio->gpio);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d pull failed,ret = %d\n", smc_no, i,ret);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, gpio->drv_level);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d drvlel failed,ret = %d\n", smc_no, i,ret);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d mulsel failed,ret = %d\n", smc_no, i,ret);
			return -1;
		}
	}

	/* emmc hwrst */
	if (pdata->has_hwrst) {
		gpio = &pdata->hwrst;
		sunxi_gpio_to_name(gpio->gpio, pin_name);

		SMC_DBG(smc_host,"pin name %s,pull%d,drv_level%d,muxsel%d,gpio index %d\n",\
							pin_name,gpio->pull,gpio->drv_level,gpio->mul_sel,gpio->gpio);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set io(emmc-rst) pull failed,ret = %d\n", smc_no,ret);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, gpio->drv_level);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set io(emmc-rst) drvlel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set io(emmc-rst) mulsel failed\n", smc_no);
			return -1;
		}
	}

	/* write protect */
	if (pdata->wpmode) {
		gpio = &pdata->wp;

		sunxi_gpio_to_name(gpio->gpio, pin_name);

		SMC_DBG(smc_host,"pin name %s,pull%d,drv_level%d,muxsel%d,gpio index %d\n",\
							pin_name,gpio->pull,gpio->drv_level,gpio->mul_sel,gpio->gpio);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set wp pull failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, gpio->drv_level);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set wp drvlel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set wp mulsel failed\n", smc_no);
			return -1;
		}
	}

	/* cd mode == gpio polling, wangyaliang for hp t-card detect */
	if ((pdata->cdmode == CARD_DETECT_BY_GPIO_POLL) || (pdata->cdmode == CARD_DETECT_BY_GPIO_IRQ || pdata->cdmode == CARD_DETECT_BY_GPIO_IRQ_UP)) {
		gpio_request(smc_host->pdata->cd.gpio, "cd");
		gpio = &pdata->cd;

		sunxi_gpio_to_name(gpio->gpio, pin_name);

		SMC_DBG(smc_host,"pin name %s,pull%d,drv_level%d,muxsel%d,gpio index %d\n",\
							pin_name,gpio->pull,gpio->drv_level,gpio->mul_sel,gpio->gpio);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d request io(cd) failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, gpio->drv_level);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set cd pull failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set cd to input failed\n", smc_no);
			return -1;
		}
	}

#endif
	return 0;
}

static int sunxi_mci_free_mmcio(struct sunxi_mmc_host *smc_host)
{
#ifndef NO_USE_PIN
	u32 smc_no = smc_host->pdev->id;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	struct gpio_config *gpio;
	u32 i;
	s32 ret = 0;
	char               pin_name[SUNXI_PIN_NAME_MAX_LEN];
  unsigned long      config;


	for (i=0; i<pdata->width+2; i++) {
		gpio = &pdata->mmcio[i];
		sunxi_gpio_to_name(gpio->gpio, pin_name);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d mulsel failed\n", smc_no, i);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 1);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d drvlel failed\n", smc_no, i);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d pull failed\n", smc_no, i);
			return -1;
		}
	}

	/* emmc hwrst */
	if (pdata->has_hwrst) {
		gpio = &pdata->hwrst;
		sunxi_gpio_to_name(gpio->gpio, pin_name);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set io(emmc-rst) mulsel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 1);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set io(emmc-rst) drvlel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set io(emmc-rst) pull failed\n", smc_no);
			return -1;
		}
	}

	/* write protect */
	if (pdata->wpmode) {
		gpio = &pdata->wp;

		sunxi_gpio_to_name(gpio->gpio, pin_name);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set wp mulsel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 1);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set wp drvlel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set wp pull failed\n", smc_no);
			return -1;
		}
	}

	/* cd mode == gpio polling */
	if (pdata->cdmode == CARD_DETECT_BY_GPIO_POLL) {
		gpio = &pdata->cd;

		sunxi_gpio_to_name(gpio->gpio, pin_name);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set cd mulsel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 1);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set cd drvlel failed\n", smc_no);
			return -1;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set cd pull failed\n", smc_no);
			return -1;
		}
	}

#endif
	return 0;
}

u32 sunxi_gpio_setdrvlevel(u32	gpio,s32 set_driving)
{
#ifndef NO_USE_PIN
		char	pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long config;
		s32 ret = 0;
    /* get gpio name */
    sunxi_gpio_to_name(gpio, pin_name);
    config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, set_driving);//ma?
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc set mmcio drvlel failed\n");
			return  __LINE__;
		}
#endif
		return 0;
}


static void sunxi_mci_update_io_driving(struct sunxi_mmc_host *smc_host, s32 driving)
{
	u32 smc_no = smc_host->pdev->id;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	struct gpio_config *gpio = NULL;
	u32 set_driving;
	u32 i;
	s32 ret;

	for (i=0; i<pdata->width+2; i++) {
		gpio = &pdata->mmcio[i];
		set_driving = driving == -1 ? gpio->drv_level : driving;
		ret = sunxi_gpio_setdrvlevel(gpio->gpio, set_driving);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d set mmcio%d drvlel failed\n", smc_no, i);
			return;
		}
	}

	SMC_DBG(smc_host, "sdc%d set mmcio driving to %d\n", smc_no, driving);
}


static int sunxi_mci_resource_request(struct sunxi_mmc_host *smc_host)
{
	struct platform_device *pdev = smc_host->pdev;
	u32 smc_no = pdev->id;
	char mclk_name[8] = {0};
	struct resource* res = NULL;
	s32 ret;

	/* request IO */
	ret = sunxi_mci_request_mmcio(smc_host);
	if (ret) {
		SMC_ERR(smc_host, "sdc%d request mmcio failed\n", pdev->id);
		goto release_pin;
	}

	/* io mapping */
	res = request_mem_region(SMC_BASE(smc_no), SMC_BASE_OS, pdev->name);
	if (!res) {
		SMC_ERR(smc_host, "Failed to request io memory region.\n");
		ret = -ENOENT;
		goto release_pin;
	}
	smc_host->reg_base = ioremap(res->start, SMC_BASE_OS);
	if (!smc_host->reg_base) {
		SMC_ERR(smc_host, "Failed to ioremap() io memory region.\n");
		ret = -EINVAL;
		goto free_mem_region;
	}

	/* mclk */
	sprintf(mclk_name, MMC_MODCLK_PREFIX"%d", smc_no);
	smc_host->mclk = clk_get(&pdev->dev, mclk_name);
	if (NULL==smc_host->mclk||IS_ERR(smc_host->mclk)) {
		ret = PTR_ERR(smc_host->mclk);
		SMC_ERR(smc_host, "Error to get clk for %s\n", mclk_name);
		goto iounmap;
	}

	/* alloc idma descriptor structure */
	smc_host->sg_cpu = dma_alloc_coherent(NULL, PAGE_SIZE,
					&smc_host->sg_dma, GFP_KERNEL);
	if (smc_host->sg_cpu == NULL) {
		SMC_ERR(smc_host, "alloc dma des failed\n");
		goto free_mclk;
	}

	/* get power regulator */
	if (smc_host->pdata->regulator != NULL) {
		smc_host->regulator = regulator_get(NULL, smc_host->pdata->regulator);
		if (IS_ERR(smc_host->regulator )) {
			SMC_ERR(smc_host, "Get regulator %s failed\n", smc_host->pdata->regulator);
			goto free_sgbuff;
		} else {
			SMC_INFO(smc_host,"%s: Get regulator suceessful\n",__FUNCTION__);

			/*
			    the aims of calling sunxi_mci_set_vddio() here are as follows:
			    1. although the default state of the regulator is power-on after power cycle, we still call
			        sunxi_mci_set_vddio() -> regulator_enable() to enable regulator.
			        it can increase regulator's "use_count" to make "use_count" be valid to
			        call regulator_disable() if there is no card during initial process.
			    2. set inner static variable on[id] in sunxi_mci_set_vddio() to 1, and avoid to enable
			        regulator again when set voltage at first time.
			*/
			if (sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_ON)) {
				SMC_ERR(smc_host, "1st enable regulator failed!\n");
				goto free_sgbuff;
			}

			sunxi_mci_get_vddio(smc_host);
		}
	} else {
		smc_host->regulator_voltage = SDC_WOLTAGE_3V3;// default 3.3V
	}

	/* if SD/MMC is 1, do not decrease frequency. maybe the device is a sdio v3.0 wifi.
		we should can control sdio wifi's freqiency through " ????? " in [mmc1_para] in sys_config.fex */
	if ((smc_host->regulator_voltage == SDC_WOLTAGE_3V3) && (pdev->id != 1)) {
		//hs200 no use in 3.3v, so disable hs200 when use 3.3v
		smc_host->pdata->caps2 &= ~MMC_CAP2_HS200_1_8V_SDR;

#if defined CONFIG_ARCH_SUN9IW1
		if (smc_host->pdata->f_max > 48000000)
			smc_host->pdata->f_max = 48000000;
#else
		if (smc_host->pdata->f_max > 50000000)
			smc_host->pdata->f_max = 50000000;
#endif
		SMC_DBG(smc_host, "io voltage is not 1.8v, set max freq to %d\n", smc_host->pdata->f_max);
	}

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		|| defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	if (smc_host->pdata->used_ddrmode == 0) {
		smc_host->pdata->caps &= ~MMC_CAP_UHS_DDR50;
		smc_host->pdata->caps &= ~MMC_CAP_1_8V_DDR;
		SMC_DBG(smc_host, "card%d disable host ddr capability: 0x%x\n", smc_host->pdev->id, smc_host->pdata->caps);
	} else
		SMC_DBG(smc_host, "card%d keep host ddr capability: 0x%x\n", smc_host->pdev->id, smc_host->pdata->caps);

	if (smc_host->pdata->used_2xmod == 0) {
		smc_host->pdata->f_ddr_max = 30000000;
	}
#endif

#ifdef CONFIG_ARCH_SUN9IW1
	/* sd/mmc comm regster mapping */
	smc_host->comm_reg = __io_address(SMC_COMM_BASE+0x4*smc_no);

	if (sunxi_get_soc_ver() >= SUN9IW1P1_REV_B) {
		SMC_DBG(smc_host, "--B-sdc %d - %d\n", smc_host->pdev->id, smc_host->pdata->f_max);
	} else {
		if (smc_host->pdata->f_max > 48000000)
			smc_host->pdata->f_max = 48000000;
		SMC_DBG(smc_host, "--A-sdc %d - %d\n", smc_host->pdev->id, smc_host->pdata->f_max);
	}
#endif

	return 0;
free_sgbuff:
	dma_free_coherent(NULL, PAGE_SIZE, smc_host->sg_cpu, smc_host->sg_dma);
	smc_host->sg_cpu = NULL;
	smc_host->sg_dma = 0;
free_mclk:
	clk_put(smc_host->mclk);
	smc_host->mclk = NULL;
iounmap:
	iounmap(smc_host->reg_base);
free_mem_region:
	release_mem_region(SMC_BASE(smc_no), SMC_BASE_OS);
release_pin:
	sunxi_mci_free_mmcio(smc_host);

	return -1;
}


static int sunxi_mci_resource_release(struct sunxi_mmc_host *smc_host)
{

#ifdef CONFIG_ARCH_SUN9IW1
	smc_host->comm_reg = NULL;
#endif

	/* free power regulator */
	if (smc_host->regulator) {
		regulator_put(smc_host->regulator);
		SMC_MSG(smc_host, "release regulator\n");
		smc_host->regulator = NULL;
	}
	/* free idma descriptor structrue */
	if (smc_host->sg_cpu) {
		dma_free_coherent(NULL, PAGE_SIZE,
				  smc_host->sg_cpu, smc_host->sg_dma);
		smc_host->sg_cpu = NULL;
		smc_host->sg_dma = 0;
	}

	clk_put(smc_host->mclk);
	smc_host->mclk = NULL;

	iounmap(smc_host->reg_base);
	release_mem_region(SMC_BASE(smc_host->pdev->id), SMC_BASE_OS);

	sunxi_mci_free_mmcio(smc_host);

	return 0;
}


static void sunxi_mci_hold_io(struct sunxi_mmc_host* smc_host)
{
#ifndef NO_USE_PIN
	int ret = 0;
	u32 i;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	struct gpio_config *gpio;

	for (i=0; i<pdata->width+2; i++) {
		char               pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long      config;
		gpio = &pdata->mmcio[i];
		sunxi_gpio_to_name(gpio->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d hold mmcio%d failed\n",
						smc_host->pdev->id, i);
			return;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d hold mmcio%d failed\n",
						smc_host->pdev->id, i);
			return;
		}
	}

	if(pdata->has_hwrst){
		char               pin_name[SUNXI_PIN_NAME_MAX_LEN];
    	unsigned long      config;
		gpio =  &pdata->hwrst;
		sunxi_gpio_to_name(gpio->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 7);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d hold mmc hardware failed\n",
						smc_host->pdev->id);
			return;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 0);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d hold mmc hardware failed\n",
						smc_host->pdev->id);
			return;
		}
	}

	SMC_DBG(smc_host, "mmc %d suspend pins\n", smc_host->pdev->id);

#endif
	return;
}


static void sunxi_mci_restore_io(struct sunxi_mmc_host* smc_host)
{
#ifndef NO_USE_PIN
	int ret;
	u32 i;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	struct gpio_config *gpio;

	for (i=0; i<pdata->width+2; i++) {
		char               pin_name[SUNXI_PIN_NAME_MAX_LEN];
    unsigned long      config;
		gpio = &pdata->mmcio[i];
		sunxi_gpio_to_name(gpio->gpio, pin_name);

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD,gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d restore mmcio%d failed\n",
						smc_host->pdev->id, i);
			return;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d restore mmcio%d mulsel failed\n",
						smc_host->pdev->id, i);
			return;
		}
	}

	if(pdata->has_hwrst){
		char			   pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long	   config;
		gpio =	&pdata->hwrst;
		sunxi_gpio_to_name(gpio->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, gpio->pull);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d restore mmc hardware failed\n",
					smc_host->pdev->id);
			return;
		}

		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, gpio->mul_sel);
		ret = pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (ret) {
			SMC_ERR(smc_host, "sdc%d restore mmc hardware failed\n",
					smc_host->pdev->id);
			return;
		}
	}

#endif
	SMC_DBG(smc_host, "mmc %d resume pins\n", smc_host->pdev->id);
}


static void sunxi_mci_finalize_request(struct sunxi_mmc_host *smc_host)
{
	struct mmc_request* mrq = smc_host->mrq;
	unsigned long iflags;

	spin_lock_irqsave(&smc_host->lock, iflags);
	if (smc_host->wait != SDC_WAIT_FINALIZE) {
		spin_unlock_irqrestore(&smc_host->lock, iflags);
		SMC_MSG(smc_host, "nothing finalize, wt %x, st %d\n",
				smc_host->wait, smc_host->state);
		return;
	}
	smc_host->wait = SDC_WAIT_NONE;
	smc_host->state = SDC_STATE_IDLE;
	smc_host->trans_done = 0;
	smc_host->dma_done = 0;
	spin_unlock_irqrestore(&smc_host->lock, iflags);

	sunxi_mci_request_done(smc_host);
	if (smc_host->error) {
		mrq->cmd->error = -ETIMEDOUT;
		if (mrq->data)
			mrq->data->error = -ETIMEDOUT;
		if (mrq->stop)
			mrq->stop->error = -ETIMEDOUT;
	} else {
		if (mrq->data)
			mrq->data->bytes_xfered = (mrq->data->blocks * mrq->data->blksz);
	}

	smc_host->mrq = NULL;
	smc_host->error = 0;
	smc_host->int_sum = 0;
	smp_wmb();
	mmc_request_done(smc_host->mmc, mrq);

	if (smc_host->cd_mode == CARD_DETECT_BY_D3) {
		u32 rval = 0;
		rval = mci_readl(smc_host, REG_GCTRL);
		rval |= SDXC_DebounceEnb;
		mci_writel(smc_host, REG_GCTRL, rval);
	}

	return;
}

static s32 sunxi_mci_get_ro(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	u32 wp_val;

	if (pdata->wpmode) {
		struct gpio_config *wp = &pdata->wp;
		wp_val = __gpio_get_value(wp->gpio);
		SMC_DBG(smc_host, "sdc fetch card wp pin status: %d \n", wp_val);
		if (!wp_val) {
			smc_host->read_only = 0;
			return 0;
		} else {
			SMC_MSG(smc_host, "Card is write-protected\n");
			smc_host->read_only = 1;
			return 1;
		}
	} else {
		smc_host->read_only = 0;
		return 0;
	}

	return 0;
}


static void sunxi_mci_cd_cb(unsigned long data)
{
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	struct gpio_config *cd = &pdata->cd;
	u32 gpio_val = 0;
	u32 present;
	u32 i = 0;

	SMC_DBG(smc_host, "***in cd***\n");

	for (i=0; i<5; i++) {
		gpio_val += (smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ_UP)\
                    ? (!__gpio_get_value(cd->gpio))\
                    :( __gpio_get_value(cd->gpio));\
		mdelay(1);
	}

    if (gpio_val==5) {
        present = 0;
    } else if (gpio_val==0){
        present = 1;
    }else{
        goto modtimer;
    }
	SMC_DBG(smc_host, "cd %d, host present %d, cur present %d\n",
			gpio_val, smc_host->present, present);

	if (smc_host->present ^ present) {
		SMC_MSG(smc_host, "mmc %d detect change, present %d\n",
				smc_host->pdev->id, present);
		smc_host->present = present;
		smp_wmb();
		if (smc_host->present)
			mmc_detect_change(smc_host->mmc, msecs_to_jiffies(500));
		else
			mmc_detect_change(smc_host->mmc, msecs_to_jiffies(50));
	}

modtimer:
	if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_POLL)
		mod_timer(&smc_host->cd_timer, jiffies + msecs_to_jiffies(300));

	return;
}

static u32 sunxi_mci_cd_irq(int irq,void *data)
{
	sunxi_mci_cd_cb((unsigned long)data);
	return 0;
}


static void sunxi_mci_dat3_det(unsigned long data)
{
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;

	SMC_DBG(smc_host, "***dat3 det***\n");
	SMC_DBG(smc_host, "host present %d\n",smc_host->present);

	if (smc_host->present)
		mmc_detect_change(smc_host->mmc, msecs_to_jiffies(500));
	else
		mmc_detect_change(smc_host->mmc, msecs_to_jiffies(50));
}



static int sunxi_mci_card_present(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	return smc_host->present;
}

static irqreturn_t sunxi_mci_irq(int irq, void *dev_id)
{
	struct sunxi_mmc_host *smc_host = dev_id;
	u32 sdio_int = 0;
	u32 raw_int;
	u32 msk_int;
	u32 idma_inte;
	u32 idma_int;

	spin_lock(&smc_host->lock);

	idma_int  = mci_readl(smc_host, REG_IDST);
	idma_inte = mci_readl(smc_host, REG_IDIE);
	raw_int   = mci_readl(smc_host, REG_RINTR);
	msk_int   = mci_readl(smc_host, REG_MISTA);
	if (!msk_int && !idma_int) {
		SMC_DBG(smc_host, "sdc%d nop irq: ri %08x mi %08x ie %08x idi %08x\n",
			smc_host->pdev->id, raw_int, msk_int, idma_inte, idma_int);
		spin_unlock(&smc_host->lock);
		return IRQ_HANDLED;
	}

	smc_host->int_sum |= raw_int;
	SMC_INFO(smc_host, "smc %d irq, ri %08x(%08x) mi %08x ie %08x idi %08x\n",
		smc_host->pdev->id, raw_int, smc_host->int_sum,
		msk_int, idma_inte, idma_int);

	if (msk_int & SDXC_SDIOInt) {
		sdio_int = 1;
		mci_writel(smc_host, REG_RINTR, SDXC_SDIOInt);
		goto sdio_out;
	}

	if(smc_host->cd_mode == CARD_DETECT_BY_D3)
	{
		if (msk_int & (SDXC_CardInsert)) {
			mci_writel(smc_host, REG_RINTR, SDXC_CardInsert);
			smc_host->present = 1;
			tasklet_schedule(&smc_host->d3_det_tasklet);
			goto sdio_out;
		}

		if (msk_int & (SDXC_CardRemove)) {
			mci_writel(smc_host, REG_RINTR, SDXC_CardRemove);
			smc_host->present = 0;
			tasklet_schedule(&smc_host->d3_det_tasklet);
			goto sdio_out;
		}
	}

	if (smc_host->wait == SDC_WAIT_NONE && !sdio_int) {
		SMC_ERR(smc_host, "smc %x, nothing to complete, ri %08x, "
			"mi %08x\n", smc_host->pdev->id, raw_int, msk_int);
		goto irq_out;
	}

	if ((raw_int & SDXC_IntErrBit) || (idma_int & SDXC_IDMA_ERR)) {
		smc_host->error = raw_int & SDXC_IntErrBit;
		smc_host->wait = SDC_WAIT_FINALIZE;
		smc_host->state = SDC_STATE_CMDDONE;
		goto irq_out;
	}
	if (idma_int & (SDXC_IDMACTransmitInt|SDXC_IDMACReceiveInt))
		smc_host->dma_done = 1;
	if (msk_int & (SDXC_AutoCMDDone|SDXC_DataOver|SDXC_CmdDone|SDXC_VolChgDone))
		smc_host->trans_done = 1;
	if ((smc_host->trans_done && (smc_host->wait == SDC_WAIT_AUTOCMD_DONE
					|| smc_host->wait == SDC_WAIT_DATA_OVER
					|| smc_host->wait == SDC_WAIT_CMD_DONE
					|| smc_host->wait == SDC_WAIT_SWITCH1V8))
		|| (smc_host->trans_done && smc_host->dma_done && (smc_host->wait & SDC_WAIT_DMA_DONE))) {
		smc_host->wait = SDC_WAIT_FINALIZE;
		smc_host->state = SDC_STATE_CMDDONE;

		#ifdef CONFIG_CACULATE_TRANS_TIME
		over_time = cpu_clock(smp_processor_id());
		#endif
	}

irq_out:
	mci_writel(smc_host, REG_RINTR, msk_int&(~SDXC_SDIOInt));
	mci_writel(smc_host, REG_IDST, idma_int);

	if (smc_host->wait == SDC_WAIT_FINALIZE) {
		smp_wmb();
		mci_writew(smc_host, REG_IMASK, 0);
		tasklet_schedule(&smc_host->tasklet);
	}

sdio_out:
	spin_unlock(&smc_host->lock);

	if (sdio_int)
		mmc_signal_sdio_irq(smc_host->mmc);

	return IRQ_HANDLED;
}

static void sunxi_mci_tasklet(unsigned long data)
{
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *) data;
	sunxi_mci_finalize_request(smc_host);
}

static void sunxi_mci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	char* bus_mode[] = {"", "OD", "PP"};
	char* pwr_mode[] = {"OFF", "UP", "ON"};
	char* vdd[] = {"3.3V", "1.8V", "1.2V"};
	char* timing[] = {"LEGACY(SDR12)", "MMC-HS(SDR20)", "SD-HS(SDR25)",
			"UHS-SDR50", "UHS-SDR104", "UHS-DDR50", "MMC-HS200"};
	char* drv_type[] = {"B", "A", "C", "D"};
	static u32 last_clock[4] = {0};
	u32 id = smc_host->pdev->id;
	u32 temp;
	s32 err;

	BUG_ON(ios->bus_mode >= sizeof(bus_mode)/sizeof(bus_mode[0]));
	BUG_ON(ios->power_mode >= sizeof(pwr_mode)/sizeof(pwr_mode[0]));
	BUG_ON(ios->signal_voltage >= sizeof(vdd)/sizeof(vdd[0]));
	BUG_ON(ios->timing >= sizeof(timing)/sizeof(timing[0]));
	SMC_DBG(smc_host, "sdc%d set ios: "
		"clk %dHz bm %s pm %s vdd %s width %d timing %s dt %s\n",
		smc_host->pdev->id, ios->clock, bus_mode[ios->bus_mode],
		pwr_mode[ios->power_mode], vdd[ios->signal_voltage],
		1 << ios->bus_width, timing[ios->timing], drv_type[ios->drv_type]);

	/* Set the power state */
	switch (ios->power_mode) {
		case MMC_POWER_ON:
			break;
		case MMC_POWER_UP:
			if (!smc_host->power_on) {
				SMC_DBG(smc_host, "sdc%d power on !!\n", smc_host->pdev->id);

				sunxi_mci_set_vddio(smc_host, smc_host->regulator_voltage);
				usleep_range(1000, 1500);
				smc_host->voltage = smc_host->regulator_voltage;

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)

				if (smc_host->pdev->id == 0)
				{
				#ifdef KEEP_CARD0_POWER_SUPPLY
					if (!smc_host->power_for_card) {
						if (card_power_on(smc_host)) {
							SMC_ERR(smc_host, "sdc%d card_power_on fail\n", smc_host->pdev->id);
						} else {
							SMC_DBG(smc_host, "sdc%d card_power_on ok\n", smc_host->pdev->id);
							smc_host->power_for_card = 1;
						}
					} else {
						SMC_DBG(smc_host, "sdc%d card power suppy is already on\n", smc_host->pdev->id);
					}
				#else
					if (card_power_on(smc_host)) {
						SMC_ERR(smc_host, "sdc%d card_power_on fail\n", smc_host->pdev->id);
					} else {
						SMC_DBG(smc_host, "sdc%d card_power_on ok\n", smc_host->pdev->id);
					}
				#endif
				} else {
					if (card_power_on(smc_host)) {
						SMC_ERR(smc_host, "sdc%d card_power_on fail\n", smc_host->pdev->id);
					} else {
						SMC_DBG(smc_host, "sdc%d card_power_on ok\n", smc_host->pdev->id);
					}
				}
#endif
				sunxi_mci_restore_io(smc_host);

#ifdef CONFIG_ARCH_SUN9IW1
				sunxi_periph_reset_deassert(smc_host->mclk);
#endif
				err = clk_prepare_enable(smc_host->mclk);
				if (err) {
					SMC_ERR(smc_host, "Failed to enable sdc%d mclk\n",
								smc_host->pdev->id);
				}

#ifdef CONFIG_ARCH_SUN9IW1
				// reset switch on and config clock switch on
				writel(readl(smc_host->comm_reg)|(SDXC_CLK_SW|SDXC_RET_SW),smc_host->comm_reg);
#endif

				mdelay(1);
				sunxi_mci_init_host(smc_host);
				enable_irq(smc_host->irq);
				smc_host->power_on = 1;

			}
			break;
		case MMC_POWER_OFF:
			if (smc_host->power_on) {
				SMC_DBG(smc_host, "sdc%d power off !!\n", smc_host->pdev->id);

				//if use data3 detect,do not power off host
				if(smc_host->cd_mode != CARD_DETECT_BY_D3){
					disable_irq(smc_host->irq);
					sunxi_mci_exit_host(smc_host);

#ifdef CONFIG_ARCH_SUN9IW1
					// reset switch off and config clock switch off
					writel(readl(smc_host->comm_reg)&~(SDXC_CLK_SW|SDXC_RET_SW),smc_host->comm_reg);
#endif

					clk_disable_unprepare(smc_host->mclk);
#ifdef CONFIG_ARCH_SUN9IW1
					sunxi_periph_reset_assert(smc_host->mclk);
#endif
					sunxi_mci_hold_io(smc_host);

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)

					if (smc_host->pdev->id == 0)
					{
					#ifndef KEEP_CARD0_POWER_SUPPLY
						if (card_power_off(smc_host)) {
							SMC_ERR(smc_host, "sdc%d card_power_off fail\n", smc_host->pdev->id);
						} else {
							SMC_DBG(smc_host, "sdc%d card_power_off ok\n", smc_host->pdev->id);
						}
					#endif
					}
					else
					{
						if (card_power_off(smc_host)) {
							SMC_ERR(smc_host, "sdc%d card_power_off fail\n", smc_host->pdev->id);
						} else {
							SMC_DBG(smc_host, "sdc%d card_power_off ok\n", smc_host->pdev->id);
						}
					}
#endif
					smc_host->power_on = 0;
					sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_OFF);
					usleep_range(1000, 1500);
					smc_host->voltage = SDC_WOLTAGE_OFF;

			}

				smc_host->ferror = 0;
				last_clock[id] = 0;
			}
			break;
	}
	/* set bus width */
	switch (ios->bus_width) {
		case MMC_BUS_WIDTH_1:
			mci_writel(smc_host, REG_WIDTH, SDXC_WIDTH1);
			smc_host->bus_width = 1;
			break;
		case MMC_BUS_WIDTH_4:
			mci_writel(smc_host, REG_WIDTH, SDXC_WIDTH4);
			smc_host->bus_width = 4;
			break;
		case MMC_BUS_WIDTH_8:
			mci_writel(smc_host, REG_WIDTH, SDXC_WIDTH8);
			smc_host->bus_width = 8;
			break;
	}

	/* set ddr mode */
	temp = mci_readl(smc_host, REG_GCTRL);
	if (ios->timing == MMC_TIMING_UHS_DDR50) {
		temp |= SDXC_DDR_MODE;
		smc_host->ddr = 1;
		/* change io driving */
		sunxi_mci_update_io_driving(smc_host, 3);
	} else {
		temp &= ~SDXC_DDR_MODE;
		smc_host->ddr = 0;
		sunxi_mci_update_io_driving(smc_host, -1);
	}
	mci_writel(smc_host, REG_GCTRL, temp);

	/* set up clock */
	if (ios->clock) {
		if ((smc_host->ddr) && (ios->clock > smc_host->pdata->f_ddr_max))
			ios->clock = smc_host->pdata->f_ddr_max;

		/* 8bit ddr, mod_clk = 2 * card_clk */
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	||defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)

		if (smc_host->pdata->used_2xmod == 1)
		{
			if (smc_host->ddr)
				smc_host->mod_clk = ios->clock << 1;
			else
				smc_host->mod_clk = ios->clock;
		}
		else
#endif
		{
			if (smc_host->ddr && smc_host->bus_width == 8)
				smc_host->mod_clk = ios->clock << 1;
			else
				smc_host->mod_clk = ios->clock;
		}

		smc_host->card_clk = ios->clock;
		sunxi_mci_set_clk(smc_host);
		last_clock[id] = ios->clock;
		//usleep_range(50000, 55000);
	} else if (!ios->clock && last_clock[id]) {
		last_clock[id] = 0;
		sunxi_mci_update_clk(smc_host);
	}
}

static void sunxi_mci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	unsigned long flags;
	u32 imask;

	spin_lock_irqsave(&smc_host->lock, flags);
	imask = mci_readl(smc_host, REG_IMASK);
	if (enable)
		imask |= SDXC_SDIOInt;
	else
		imask &= ~SDXC_SDIOInt;
	mci_writel(smc_host, REG_IMASK, imask);
	spin_unlock_irqrestore(&smc_host->lock, flags);
}

void sunxi_mci_hw_reset(struct mmc_host *mmc)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	u32 id = smc_host->pdev->id;

	if (id == 2 || id == 3) {
		mci_writel(smc_host, REG_HWRST, 0);
		udelay(10);
		mci_writel(smc_host, REG_HWRST, 1);
		udelay(300);
	}
}

static void sunxi_mci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	struct mmc_command* cmd = mrq->cmd;
	struct mmc_data* data = mrq->data;
	u32 byte_cnt = 0;
	int ret;

	if (sunxi_mci_card_present(mmc) == 0 || smc_host->ferror
			|| smc_host->suspend || !smc_host->power_on) {
		SMC_DBG(smc_host, "no medium present, ferr %d, suspend %d pwd %d\n",
			    smc_host->ferror, smc_host->suspend, smc_host->power_on);
		mrq->cmd->error = -ENOMEDIUM;
		mrq->cmd->error = -ENOMEDIUM;
		mmc_request_done(mmc, mrq);
		return;
	}

	smc_host->mrq = mrq;
	if (data) {
#ifdef  USE_SECURE_WRITE_PROTECT
			if((data->flags & MMC_DATA_WRITE)
				&&sunxi_check_secure_area(cmd->arg,data->blocks)){
				mrq->cmd->error = -EINVAL;
				SMC_ERR(smc_host, "smc %d cmd %d Can't write to secure area,start %d end %d\n",
								smc_host->pdev->id,cmd->opcode & 0x3f,cmd->arg,cmd->arg+data->blocks-1);
				mmc_request_done(mmc, mrq);
				return;
			}
#endif
		byte_cnt = data->blksz * data->blocks;
		mci_writel(smc_host, REG_BLKSZ, data->blksz);
		mci_writel(smc_host, REG_BCNTR, byte_cnt);
		ret = sunxi_mci_prepare_dma(smc_host, data);
		if (ret < 0) {
			SMC_ERR(smc_host, "smc %d prepare DMA failed\n", smc_host->pdev->id);
			cmd->error = ret;
			cmd->data->error = ret;
			smp_wmb();
			mmc_request_done(smc_host->mmc, mrq);
			return;
		}
	}

	if (smc_host->cd_mode == CARD_DETECT_BY_D3) {
		u32 rval = 0;
		rval = mci_readl(smc_host, REG_GCTRL);
		rval &= ~SDXC_DebounceEnb;
		mci_writel(smc_host, REG_GCTRL, rval);
	}

	sunxi_mci_send_cmd(smc_host, cmd);
}

static int sunxi_mci_do_voltage_switch(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);

	if (smc_host->voltage != SDC_WOLTAGE_3V3 &&
			ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {

		/* during initial process, ios->signal_voltage may be equal to MMC_SIGNAL_VOLTAGE_330,
		if emmc's io voltage is 1.8V, don't switch */
		if (smc_host->cd_mode == CARD_ALWAYS_PRESENT ) {
			SMC_INFO(smc_host,"%s mmc %d return, no need to switch voltage to 3v3: \n",
				         __FUNCTION__,smc_host->pdev->id);
			return 0;
		}

		sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_3V3);
		/* wait for 5ms */
		usleep_range(1000, 1500);
		smc_host->voltage = SDC_WOLTAGE_3V3;
		return 0;
	} else if (smc_host->voltage != SDC_WOLTAGE_1V8 &&
			(ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180)) {
		u32 data_down;
		/* clock off */
		sunxi_mci_oclk_onoff(smc_host, 0, 0);
		/* check whether data[3:0] is 0000 */
		data_down = mci_readl(smc_host, REG_STAS);
		if (!(data_down & SDXC_CardPresent)) {
			/* switch voltage of card vdd to 1.8V */
			sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_1V8);
//			{
//				//writel(0x0, 0x6000800+0x314);
//				void __iomem *gpiof_bias = NULL;
//				gpiof_bias = __io_address(0x6000800+0x314);
//				SMC_DBG(smc_host,"gpio f bias %x\n",readl(gpiof_bias));
//				writel(0,gpiof_bias);
//				SMC_DBG(smc_host,"gpio f bias %x\n",readl(gpiof_bias));
//			}

			/* the standard defines the time limit is 5ms, here we
			   wait for 8ms to make sure that the card completes the
			   voltage switching */
			usleep_range(8000, 8500);
			/* clock on again */
			sunxi_mci_oclk_onoff(smc_host, 1, 0);
			/* wait for 1ms */
			usleep_range(2000, 2500);

			/* check whether data[3:0] is 1111 */
			data_down = mci_readl(smc_host, REG_STAS);
			if (data_down & SDXC_CardPresent) {
				u32 rval = mci_readl(smc_host, REG_RINTR);
				if ((rval & SDXC_VolChgDone & SDXC_CmdDone)
						== (SDXC_VolChgDone & SDXC_CmdDone)) {
					smc_host->voltage = SDC_WOLTAGE_1V8;
					mci_writew(smc_host, REG_RINTR,
						SDXC_VolChgDone | SDXC_CmdDone);
					smc_host->voltage_switching = 0;
					return 0;
				}
			}
		}

		/*
		 * If we are here, that means the switch to 1.8V signaling
		 * failed. We power cycle the card, and retry initialization
		 * sequence by setting S18R to 0.
		 */
		usleep_range(5000, 5500);
		sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_OFF);
		usleep_range(1000, 1500);
		sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_3V3);
		SMC_ERR(smc_host, ": Switching to 1.8V signalling "
			"voltage failed, retrying with S18R set to 0\n");
		mci_writel(smc_host, REG_GCTRL, mci_readl(smc_host, REG_GCTRL)|SDXC_HWReset);
		mci_writew(smc_host, REG_RINTR, SDXC_VolChgDone | SDXC_CmdDone);
		sunxi_mci_oclk_onoff(smc_host, 1, 0);
		smc_host->voltage_switching = 0;
		return -EAGAIN;
	} else
		return 0;
}

/*
 * Here we execute a tuning operation to find the sample window of MMC host.
 * Then we select the best sampling point in the host for DDR50, SDR50, and
 * SDR104 modes.
 */
 #if 0
static int sunxi_mci_execute_tuning(struct mmc_host *mmc, u32 opcode)
{
	static const char tuning_blk_4b[] = {
		0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
		0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
		0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
		0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
		0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
		0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
		0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
		0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde
	};
	static const char tuning_blk_8b[] = {
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
		0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee
	};
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	u32 sample_min = 1;
	u32 sample_max = 0;
	u32 sample_bak = smc_host->sclk_dly;
	u32 sample_dly = 1; //0;
	u32 sample_win = 0;
	u32 loops = 64;
	u32 tuning_done = 0;
	char* rcv_pattern = (char*)kmalloc(128, GFP_KERNEL|GFP_DMA);
	char* std_pattern = NULL;
	int err = 0;

	if (!rcv_pattern) {
		SMC_ERR(smc_host, "sdc%d malloc tuning pattern buffer failed\n",
				smc_host->pdev->id);
		return -EIO;
	}
	SMC_MSG(smc_host, "sdc%d executes tuning operation\n", smc_host->pdev->id);
	/*
	 * The Host Controller needs tuning only in case of SDR104 mode
	 * and for SDR50 mode. Issue CMD19 repeatedly till get all of the
	 * sample points or the number of loops reaches 40 times or a
	 * timeout of 150ms occurs.
	 */
	do {
		struct mmc_command cmd = {0};
		struct mmc_data data = {0};
		struct mmc_request mrq = {0};
		struct scatterlist sg;

		sunxi_mci_set_clk_dly(smc_host, smc_host->oclk_dly, sample_dly);
		cmd.opcode = opcode;
		cmd.arg = 0;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;
		if (opcode == MMC_SEND_TUNING_BLOCK_HS200) {
			if (mmc->ios.bus_width == MMC_BUS_WIDTH_8) {
				sg.length = 128;
				data.blksz = 128;
				std_pattern = (char*)tuning_blk_8b;
			} else if (mmc->ios.bus_width == MMC_BUS_WIDTH_4) {
				sg.length = 64;
				data.blksz = 64;
				std_pattern = (char*)tuning_blk_4b;
			}
		} else {
			sg.length = 64;
			data.blksz = 64;
			std_pattern = (char*)tuning_blk_4b;
		}
		data.blocks = 1;
		data.flags = MMC_DATA_READ;
		data.sg = &sg;
		data.sg_len = 1;
		sg_init_one(&sg, rcv_pattern, sg.length);

		mrq.cmd = &cmd;
		mrq.data = &data;

		mmc_wait_for_req(mmc, &mrq);
		/*
		 * If no error happened in the transmission, compare data with
		 * the tuning pattern. If there is no error, record the minimal
		 * and the maximal value of the sampling clock delay to find
		 * the best sampling point in the sampling window.
		 */
		if (!cmd.error && !data.error) {
			if (!memcmp(rcv_pattern, std_pattern, data.blksz)) {
				SMC_DBG(smc_host, "sdc%d tuning ok, sclk_dly %d\n",
					smc_host->pdev->id, sample_dly);
				if (!sample_win)
					sample_min = sample_dly;
				sample_win++;
				if (sample_dly == 7) {
					SMC_MSG(smc_host, "sdc%d tuning reach to max sclk_dly 7\n",
						smc_host->pdev->id);
					tuning_done = 1;
					sample_max = sample_dly;
					break;
				}
			} else if (sample_win) {
				SMC_MSG(smc_host, "sdc%d tuning data failed, sclk_dly %d\n",
					smc_host->pdev->id, sample_dly);
				tuning_done = 1;
				sample_max = sample_dly-1;
				break;
			}
		} else if (sample_win) {
			SMC_MSG(smc_host, "sdc%d tuning trans fail, sclk_dly %d\n",
				smc_host->pdev->id, sample_dly);
			tuning_done = 1;
			sample_max = sample_dly-1;
			break;
		}
		sample_dly++;
		/* if sclk_dly reach to 7(maximum), down the clock and tuning again */
		if (sample_dly == 8 && loops)
			break;
	} while (!tuning_done && loops--);

	/* select the best sampling point from the sampling window */
	if (sample_win) {
		sample_dly = sample_min + sample_win/2;
		SMC_MSG(smc_host, "sdc%d sample_window:[%d, %d], sample_point %d\n",
				smc_host->pdev->id, sample_min, sample_max, sample_dly);
		sunxi_mci_set_clk_dly(smc_host, smc_host->oclk_dly, sample_dly);
		err = 0;
	} else {
		SMC_ERR(smc_host, "sdc%d cannot find a sample point\n", smc_host->pdev->id);
		sunxi_mci_set_clk_dly(smc_host, smc_host->oclk_dly, sample_bak);
		mmc->ios.bus_width = MMC_BUS_WIDTH_1;
		mmc->ios.timing = MMC_TIMING_LEGACY;
		err = -EIO;
	}

	kfree(rcv_pattern);
	return err;
}
#endif

/*
 * Here provide a function to scan card, for some SDIO cards that
 * may stay in busy status after writing operations. MMC host does
 * not wait for ready itself. So the driver of this kind of cards
 * should call this function to check the real status of the card.
 */
void sunxi_mci_rescan_card(unsigned id, unsigned insert)
{
	struct sunxi_mmc_host *smc_host = NULL;

	BUG_ON(id > 3);
	BUG_ON(sunxi_host[id] == NULL);
	if (sunxi_host[id] == NULL)
		return;
	smc_host = sunxi_host[id];
	smc_host->present = insert ? 1 : 0;
	mmc_detect_change(smc_host->mmc, 0);
	return;
}
EXPORT_SYMBOL_GPL(sunxi_mci_rescan_card);

int sunxi_mci_check_r1_ready(struct mmc_host* mmc, unsigned ms)
{
	struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
	unsigned long expire = jiffies + msecs_to_jiffies(ms);
	do {
		if (!(mci_readl(smc_host, REG_STAS) & SDXC_CardDataBusy))
			break;
	} while (time_before(jiffies, expire));

	if ((mci_readl(smc_host, REG_STAS) & SDXC_CardDataBusy)) {
		SMC_MSG(smc_host, "wait r1 rdy %d ms timeout\n", ms);
		return -1;
	} else
		return 0;
}
EXPORT_SYMBOL_GPL(sunxi_mci_check_r1_ready);

static struct mmc_host_ops sunxi_mci_ops = {
	.request	= sunxi_mci_request,
	.set_ios	= sunxi_mci_set_ios,
	.get_ro		= sunxi_mci_get_ro,
	.get_cd		= sunxi_mci_card_present,
	.enable_sdio_irq= sunxi_mci_enable_sdio_irq,
	.hw_reset	= sunxi_mci_hw_reset,
	.start_signal_voltage_switch = sunxi_mci_do_voltage_switch,
	//.execute_tuning = sunxi_mci_execute_tuning,
};

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
static int sunxi_mci_proc_drvversion(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;

	p += sprintf(p, "%s\n", DRIVER_VERSION);
	return p - page;
}

static int sunxi_mci_proc_hostinfo(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	struct device* dev = &smc_host->pdev->dev;
	char* cd_mode[] = {"None", "GPIO Check", "GPIO IRQ", "Always In", "Manual", "DATA3", "GPIO_IRQ_UP"};	//yaliang for hp t-card detect
	char* state[] = {"Idle", "Sending CMD", "CMD Done"};
	char* vol[] = {"3.3V", "1.8V", "1.2V", "off"};
	u32 Fmclk_MHz = (smc_host->mod_clk == 24000000 ? 24000000 : 600000000)/1000000;
	u32 Tmclk_ns = Fmclk_MHz ? 10000/Fmclk_MHz : 0;
	u32 odly = smc_host->oclk_dly ? Tmclk_ns*smc_host->oclk_dly : Tmclk_ns >> 1;
	u32 sdly = smc_host->sclk_dly ? Tmclk_ns*smc_host->sclk_dly : Tmclk_ns >> 1;

	p += sprintf(p, " %s Host Info:\n", dev_name(dev));
	p += sprintf(p, " REG Base  : %p\n", smc_host->reg_base);
	p += sprintf(p, " DMA Desp  : %p(%08x)\n", smc_host->sg_cpu, smc_host->sg_dma);
	p += sprintf(p, " Mod Clock : %d\n", smc_host->mod_clk);
	p += sprintf(p, " Card Clock: %d\n", smc_host->card_clk);
	p += sprintf(p, " Oclk Delay: %d(%d.%dns)\n", smc_host->oclk_dly, odly/10, odly%10);
	p += sprintf(p, " Sclk Delay: %d(%d.%dns)\n", smc_host->sclk_dly, sdly/10, odly%10);
	p += sprintf(p, " Bus Width : %d\n", smc_host->bus_width);
	p += sprintf(p, " DDR Mode  : %d\n", smc_host->ddr);
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		|| defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
	p += sprintf(p, " 2xCLK Mode: %d\n", smc_host->pdata->used_ddrmode);
#endif
	p += sprintf(p, " Voltage   : %s\n", vol[smc_host->voltage]);
	p += sprintf(p, " Present   : %d\n", smc_host->present);
	p += sprintf(p, " CD Mode   : %s\n", cd_mode[smc_host->cd_mode]);
	p += sprintf(p, " Read Only : %d\n", smc_host->read_only);
	p += sprintf(p, " State     : %s\n", state[smc_host->state]);
	p += sprintf(p, " Regulator : %s\n", smc_host->pdata->regulator);

	return p - page;
}

static int sunxi_mci_proc_read_regs(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	u32 i;

	p += sprintf(p, "Dump smc regs:\n");
	for (i=0; i<0x100; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", (u32)(smc_host->reg_base + i));
		p += sprintf(p, "%08x ", readl(smc_host->reg_base + i));
	}
	p += sprintf(p, "\n");

	p += sprintf(p, "Dump ccmu regs:\n");
	for (i=0; i<0x170; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", (unsigned int)(IO_ADDRESS(SUNXI_CCM_BASE) + i));
		p += sprintf(p, "%08x ", readl(IO_ADDRESS(SUNXI_CCM_BASE) + i));
	}
	p += sprintf(p, "\n");

	p += sprintf(p, "Dump gpio regs:\n");
	for (i=0; i<0x120; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", (unsigned int)(IO_ADDRESS(SUNXI_PIO_BASE) + i));
		p += sprintf(p, "%08x ", readl(IO_ADDRESS(SUNXI_PIO_BASE)+ i));
	}
	p += sprintf(p, "\n");

	p += sprintf(p, "Dump gpio irqc:\n");
	for (i=0x200; i<0x300; i+=4) {
		if (!(i&0xf))
			p += sprintf(p, "\n0x%08x : ", (unsigned int)(IO_ADDRESS(SUNXI_PIO_BASE) + i));
		p += sprintf(p, "%08x ", readl(IO_ADDRESS(SUNXI_PIO_BASE)+ i));
	}
	p += sprintf(p, "\n");

	return p - page;
}

static int sunxi_mci_proc_read_dbglevel(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;

	p += sprintf(p, "Debug-Level : 0- msg&err, 1- +info, 2- +dbg, 3- all\n");
	p += sprintf(p, "current debug-level : %d\n", smc_host->debuglevel);
	return p - page;
}

static int sunxi_mci_proc_write_dbglevel(struct file *file, const char __user *buffer,
					unsigned long count, void *data)
{
	u32 smc_debug;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	smc_debug = simple_strtoul(buffer, NULL, 10);

	smc_host->debuglevel = smc_debug;
	return sizeof(smc_debug);
}

static int sunxi_mci_proc_read_cdmode(char *page, char **start, off_t off,
					int count, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;

	p += sprintf(p, "card detect mode: %d\n", smc_host->cd_mode);
	return p - page;
}

static int sunxi_mci_proc_write_cdmode(struct file *file, const char __user *buffer,
					unsigned long count, void *data)
{
	u32 cdmode;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	cdmode = simple_strtoul(buffer, NULL, 10);

	smc_host->cd_mode = cdmode;
	return sizeof(cdmode);
}

static int sunxi_mci_proc_read_insert_status(char *page, char **start, off_t off,
					int coutn, int *eof, void *data)
{
	char *p = page;
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;

	p += sprintf(p, "Usage: \"echo 1 > insert\" to scan card and "
			"\"echo 0 > insert\" to remove card\n");
	if (smc_host->cd_mode != CARD_DETECT_BY_FS)
		p += sprintf(p, "Sorry, this node if only for manual "
				"attach mode(cd mode 4)\n");

	p += sprintf(p, "card attach status: %s\n",
		smc_host->present ? "inserted" : "removed");


	return p - page;
}

static int sunxi_mci_proc_card_insert_ctrl(struct file *file, const char __user *buffer,
					unsigned long count, void *data)
{
	u32 insert = simple_strtoul(buffer, NULL, 10);
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	u32 present = insert ? 1 : 0;

	SMC_DBG(smc_host, "sdc%d,Change insert\n", smc_host->pdev->id);
	if (smc_host->present ^ present) {
		smc_host->present = present;
		SMC_DBG(smc_host, "sdc%d,re detect card\n", smc_host->pdev->id);
		mmc_detect_change(smc_host->mmc, msecs_to_jiffies(300));
	}

	SMC_DBG(smc_host, "Insert status %d\n", smc_host->present);
	return sizeof(insert);
}

static int sunxi_mci_proc_get_iodriving(char *page, char **start, off_t off,
						int coutn, int *eof, void *data)
{
	char *p = page;
#ifndef NO_USE_PIN
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	char* mmc_para_io[10] = {"sdc_clk", "sdc_cmd", "sdc_d0", "sdc_d1", "sdc_d2",
				"sdc_d3", "sdc_d4", "sdc_d5", "sdc_d6", "sdc_d7"};
	struct gpio_config *gpio;
	u32 i;

	p += sprintf(p, "current io driving:\n");
	for (i=0; i<pdata->width; i++) {
		char	pin_name[SUNXI_PIN_NAME_MAX_LEN];
		unsigned long config;
		gpio = &pdata->mmcio[i];
    /* get gpio name */
    sunxi_gpio_to_name(gpio->gpio, pin_name);
    config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, 0xFFFF);
    pin_config_get(SUNXI_PINCTRL, pin_name, &config);
		p += sprintf(p, "%s : %d\n", mmc_para_io[i], (int)(SUNXI_PINCFG_UNPACK_VALUE(config)));

	}
#endif
	return p - page;
}


static int sunxi_mci_proc_set_iodriving(struct file *file, const char __user *buffer,
					unsigned long count, void *data)
{
	unsigned long driving = simple_strtoul(buffer, NULL, 16);
	struct sunxi_mmc_host *smc_host = (struct sunxi_mmc_host *)data;
	struct sunxi_mmc_platform_data *pdata = smc_host->pdata;
	u32 clk_drv, cmd_drv, d0_drv, d1_drv, d2_drv, d3_drv, d4_drv, d5_drv, d6_drv, d7_drv;

	clk_drv = 0xf & (driving >> 0);
	cmd_drv = 0xf & (driving >> 4);
	d0_drv = 0xf & (driving >> 8);
	d1_drv = 0xf & (driving >> 12);
	d2_drv = 0xf & (driving >> 16);
	d3_drv = 0xf & (driving >> 20);
	d4_drv = 0xf & (driving >> 8);
	d5_drv = 0xf & (driving >> 12);
	d6_drv = 0xf & (driving >> 16);
	d7_drv = 0xf & (driving >> 20);


	printk("set io driving, clk %x, cmd %x, d0 %x, d1 %x, d2, %x, d3 %x\n",
		clk_drv, cmd_drv, d0_drv, d1_drv, d2_drv, d3_drv);
	if (clk_drv > 0 && clk_drv < 4)
		sunxi_gpio_setdrvlevel(pdata->mmcio[0].gpio, clk_drv);
	if (cmd_drv > 0 && cmd_drv < 4)
		sunxi_gpio_setdrvlevel(pdata->mmcio[1].gpio, clk_drv);
	if (d0_drv > 0 && d0_drv < 4)
		sunxi_gpio_setdrvlevel(pdata->mmcio[2].gpio, clk_drv);
	if (pdata->width == 4 || pdata->width == 8) {
		if (d1_drv > 0 && d1_drv < 4)
			sunxi_gpio_setdrvlevel(pdata->mmcio[3].gpio, clk_drv);
		if (d2_drv > 0 && d2_drv < 4)
			sunxi_gpio_setdrvlevel(pdata->mmcio[4].gpio, clk_drv);
		if (d3_drv > 0 && d3_drv < 4)
			sunxi_gpio_setdrvlevel(pdata->mmcio[5].gpio, clk_drv);
		if (pdata->width == 8) {
			if (d4_drv > 0 && d4_drv < 4)
				sunxi_gpio_setdrvlevel(pdata->mmcio[6].gpio, clk_drv);
			if (d5_drv > 0 && d5_drv < 4)
				sunxi_gpio_setdrvlevel(pdata->mmcio[7].gpio, clk_drv);
			if (d6_drv > 0 && d6_drv < 4)
				sunxi_gpio_setdrvlevel(pdata->mmcio[8].gpio, clk_drv);
			if (d7_drv > 0 && d7_drv < 4)
				sunxi_gpio_setdrvlevel(pdata->mmcio[9].gpio, clk_drv);
		}
	}

	return count;
}


void sunxi_mci_procfs_attach(struct sunxi_mmc_host *smc_host)
{
	struct device *dev = &smc_host->pdev->dev;
	char sunxi_mci_proc_rootname[32] = {0};

	//make mmc dir in proc fs path
	snprintf(sunxi_mci_proc_rootname, sizeof(sunxi_mci_proc_rootname),
			"driver/%s", dev_name(dev));
	smc_host->proc_root = proc_mkdir(sunxi_mci_proc_rootname, NULL);
	if (IS_ERR(smc_host->proc_root))
		SMC_MSG(smc_host, "%s: failed to create procfs \"driver/mmc\".\n", dev_name(dev));

	smc_host->proc_drvver = create_proc_read_entry("drv-version", 0444,
				smc_host->proc_root, sunxi_mci_proc_drvversion, NULL);
	if (IS_ERR(smc_host->proc_root))
		SMC_MSG(smc_host, "%s: failed to create procfs \"drv-version\".\n", dev_name(dev));

	smc_host->proc_hostinfo = create_proc_read_entry("hostinfo", 0444,
				smc_host->proc_root, sunxi_mci_proc_hostinfo, smc_host);
	if (IS_ERR(smc_host->proc_hostinfo))
		SMC_MSG(smc_host, "%s: failed to create procfs \"hostinfo\".\n", dev_name(dev));

	smc_host->proc_regs = create_proc_read_entry("register", 0444,
				smc_host->proc_root, sunxi_mci_proc_read_regs, smc_host);
	if (IS_ERR(smc_host->proc_regs))
		SMC_MSG(smc_host, "%s: failed to create procfs \"hostinfo\".\n", dev_name(dev));

	smc_host->proc_dbglevel = create_proc_entry("debug-level", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_dbglevel))
		SMC_MSG(smc_host, "%s: failed to create procfs \"debug-level\".\n", dev_name(dev));

	smc_host->proc_dbglevel->data = smc_host;
	smc_host->proc_dbglevel->read_proc = sunxi_mci_proc_read_dbglevel;
	smc_host->proc_dbglevel->write_proc = sunxi_mci_proc_write_dbglevel;

	smc_host->proc_cdmode = create_proc_entry("cdmode", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_cdmode))
		SMC_MSG(smc_host, "%s: failed to create procfs \"cdmode\".\n", dev_name(dev));

	smc_host->proc_cdmode->data = smc_host;
	smc_host->proc_cdmode->read_proc = sunxi_mci_proc_read_cdmode;
	smc_host->proc_cdmode->write_proc = sunxi_mci_proc_write_cdmode;

	smc_host->proc_insert = create_proc_entry("insert", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_insert))
		SMC_MSG(smc_host, "%s: failed to create procfs \"insert\".\n", dev_name(dev));

	smc_host->proc_insert->data = smc_host;
	smc_host->proc_insert->read_proc = sunxi_mci_proc_read_insert_status;
	smc_host->proc_insert->write_proc = sunxi_mci_proc_card_insert_ctrl;

	smc_host->proc_iodrive = create_proc_entry("io-drive", 0644, smc_host->proc_root);
	if (IS_ERR(smc_host->proc_iodrive))
	{
		SMC_MSG(smc_host, "%s: failed to create procfs \"io-drive\".\n", dev_name(dev));
	}
	smc_host->proc_iodrive->data = smc_host;
	smc_host->proc_iodrive->read_proc = sunxi_mci_proc_get_iodriving;
	smc_host->proc_iodrive->write_proc = sunxi_mci_proc_set_iodriving;

}

void sunxi_mci_procfs_remove(struct sunxi_mmc_host *smc_host)
{
	struct device *dev = &smc_host->pdev->dev;
	char sunxi_mci_proc_rootname[32] = {0};

	snprintf(sunxi_mci_proc_rootname, sizeof(sunxi_mci_proc_rootname),
		"driver/%s", dev_name(dev));
	remove_proc_entry("io-drive", smc_host->proc_root);
	remove_proc_entry("insert", smc_host->proc_root);
	remove_proc_entry("cdmode", smc_host->proc_root);
	remove_proc_entry("debug-level", smc_host->proc_root);
	remove_proc_entry("register", smc_host->proc_root);
	remove_proc_entry("hostinfo", smc_host->proc_root);
	remove_proc_entry("drv-version", smc_host->proc_root);
	remove_proc_entry(sunxi_mci_proc_rootname, NULL);
}

#else

void sunxi_mci_procfs_attach(struct sunxi_mmc_host *smc_host) { }
void sunxi_mci_procfs_remove(struct sunxi_mmc_host *smc_host) { }

#endif	//PROC_FS

static int sunxi_mci_probe(struct platform_device *pdev)
{
	struct sunxi_mmc_host *smc_host = NULL;
	struct mmc_host	*mmc = NULL;
	int ret = 0;

	mmc = mmc_alloc_host(sizeof(struct sunxi_mmc_host), &pdev->dev);
	if (!mmc) {
		SMC_ERR(smc_host, "mmc alloc host failed\n");
		ret = -ENOMEM;
		goto probe_out;
	}

	smc_host = mmc_priv(mmc);
	memset((void*)smc_host, 0, sizeof(smc_host));
	smc_host->mmc	= mmc;
	smc_host->pdev	= pdev;
	smc_host->pdata	= pdev->dev.platform_data;
	smc_host->cd_mode = smc_host->pdata->cdmode;
	smc_host->io_flag = smc_host->pdata->isiodev ? 1 : 0;
	smc_host->debuglevel = CONFIG_MMC_PRE_DBGLVL_SUNXI;

	spin_lock_init(&smc_host->lock);
	tasklet_init(&smc_host->tasklet, sunxi_mci_tasklet, (unsigned long) smc_host);

	if (sunxi_mci_resource_request(smc_host)) {
		SMC_ERR(smc_host, "%s: Failed to get resouce.\n", dev_name(&pdev->dev));
		goto probe_free_host;
	}

	sunxi_mci_procfs_attach(smc_host);

	smc_host->irq = SMC_IRQNO(pdev->id);
	if (request_irq(smc_host->irq, sunxi_mci_irq, 0, DRIVER_NAME, smc_host)) {
		SMC_ERR(smc_host, "Failed to request smc card interrupt.\n");
		ret = -ENOENT;
		goto probe_free_resource;
	}
	disable_irq(smc_host->irq);

	if (smc_host->cd_mode == CARD_ALWAYS_PRESENT) {
		smc_host->present = 1;
	} else if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ || smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ_UP) {	//hp t-card detect
		int virq = 0;
		unsigned debounce = (2U << 4) | 1;
		virq = gpio_to_irq(smc_host->pdata->cd.gpio);
		if (IS_ERR_VALUE(virq)) {
			SMC_ERR(smc_host,"map gpio [%d] to virq failed, errno = %d\n",
			smc_host->pdata->cd.gpio, virq);
		}

		SMC_DBG(smc_host,"gpio [%d] map to virq [%d] ok\n",smc_host->pdata->cd.gpio, virq);
		/* request virq */
		ret = devm_request_irq(&pdev->dev, virq, sunxi_mci_cd_irq,
			IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, DRIVER_NAME, smc_host);

		if (IS_ERR_VALUE(ret)) {
			SMC_ERR(smc_host, "Failed to get gpio irq for card detection\n");
		}
		smc_host->cd_hdle = virq;
		/* set debounce clock */
		gpio_set_debounce(smc_host->pdata->cd.gpio, debounce); //hp t-card detect
		if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ)
			smc_host->present = !__gpio_get_value(smc_host->pdata->cd.gpio);
		else
			smc_host->present = __gpio_get_value(smc_host->pdata->cd.gpio);
	} else if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_POLL) {
		init_timer(&smc_host->cd_timer);
		smc_host->cd_timer.expires = jiffies + 1*HZ;
		smc_host->cd_timer.function = &sunxi_mci_cd_cb;
		smc_host->cd_timer.data = (unsigned long)smc_host;
		add_timer(&smc_host->cd_timer);
		smc_host->present = !__gpio_get_value(smc_host->pdata->cd.gpio);
	}else if(smc_host->cd_mode == CARD_DETECT_BY_D3){
		u32 rval = 0;

		tasklet_init(&smc_host->d3_det_tasklet, sunxi_mci_dat3_det, (unsigned long) smc_host);

#ifdef CONFIG_ARCH_SUN9IW1
		sunxi_periph_reset_deassert(smc_host->mclk);
#endif
		ret = clk_prepare_enable(smc_host->mclk);
		if (ret)
			SMC_ERR(smc_host, "Failed to enable sdc%d mclk\n", smc_host->pdev->id);

#ifdef CONFIG_ARCH_SUN9IW1
		// reset switch on and config clock switch on
		writel(readl(smc_host->comm_reg)|(SDXC_CLK_SW|SDXC_RET_SW),
				smc_host->comm_reg);
#endif

		mdelay(1);
		smc_host->present = 1;
		rval = mci_readl(smc_host, REG_RINTR);
		SMC_DBG(smc_host, "+> REG_RINTR=0x%x\n", rval);
		if ((rval & SDXC_CardRemove)) {
			SMC_MSG(smc_host, "data[3] detect Card Remove\n");
			smc_host->present = 0;
		}
	}

	mmc->ops        = &sunxi_mci_ops;
	mmc->ocr_avail	= smc_host->pdata->ocr_avail;
	mmc->caps	= smc_host->pdata->caps;
	mmc->caps2	= smc_host->pdata->caps2;
	mmc->platform_cap = smc_host->pdata->platform_cap;
	mmc->pm_caps	= MMC_PM_KEEP_POWER|MMC_PM_WAKE_SDIO_IRQ;
	mmc->f_min	= smc_host->pdata->f_min;
	mmc->f_max      = smc_host->pdata->f_max;
	mmc->max_blk_count	= 8192;
	mmc->max_blk_size	= 4096;
	mmc->max_req_size	= mmc->max_blk_size * mmc->max_blk_count;
	mmc->max_seg_size	= mmc->max_req_size;
	mmc->max_segs	    	= 128;
	if (smc_host->io_flag)
		mmc->pm_flags = MMC_PM_IGNORE_PM_NOTIFY;

	ret = mmc_add_host(mmc);
	if (ret) {
		SMC_ERR(smc_host, "Failed to add mmc host.\n");
		goto probe_free_irq;
	}
	platform_set_drvdata(pdev, mmc);
	sunxi_host[pdev->id] = smc_host;

	SMC_DBG(smc_host, "sdc%d Probe: base:0x%p irq:%u sg_cpu:%p(%x) ret %d.\n",
		pdev->id, smc_host->reg_base, smc_host->irq,
		smc_host->sg_cpu, smc_host->sg_dma, ret);

	if (smc_host->cd_mode == CARD_DETECT_BY_D3 && smc_host->present == 0) {
		SMC_DBG(smc_host, "sdc%d power init.\n", smc_host->pdev->id);

		sunxi_mci_init_host(smc_host);
		enable_irq(smc_host->irq);
		smc_host->power_on = 1;
	} else if (smc_host->present == 0) {
		/* if card is not present and the card detect mode is not CARD_DETECT_BY_D3,
		we shutdown io voltage to save power. */
		SMC_DBG(smc_host, "sdc%d: no card detected, try to shutdown io voltage.\n",
			pdev->id);
		sunxi_mci_hold_io(smc_host); /* disable gpio to avoid power leakage */
		sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_OFF);
		usleep_range(1000, 1500);
		smc_host->voltage = SDC_WOLTAGE_OFF;
		smc_host->power_on = 0;
	}

	goto probe_out;

probe_free_irq:
	if (smc_host->irq)
		free_irq(smc_host->irq, smc_host);
probe_free_resource:
	sunxi_mci_resource_release(smc_host);
probe_free_host:
	mmc_free_host(mmc);
probe_out:
	return ret;
}

static int sunxi_mci_remove(struct platform_device *pdev)
{
	struct mmc_host    	*mmc  = platform_get_drvdata(pdev);
	struct sunxi_mmc_host	*smc_host = mmc_priv(mmc);

	SMC_MSG(smc_host, "%s: Remove.\n", dev_name(&pdev->dev));

	sunxi_mci_exit_host(smc_host);

	sunxi_mci_procfs_remove(smc_host);
	mmc_remove_host(mmc);

	tasklet_disable(&smc_host->tasklet);
	free_irq(smc_host->irq, smc_host);
	if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_POLL)
		del_timer(&smc_host->cd_timer);
	else if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ || smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ_UP)	//hp t-card detect
		devm_free_irq(&pdev->dev,smc_host->cd_hdle,smc_host);
	else if((smc_host->cd_mode == CARD_DETECT_BY_D3))
		tasklet_disable(&smc_host->d3_det_tasklet);

	sunxi_mci_resource_release(smc_host);

	mmc_free_host(mmc);
	sunxi_host[pdev->id] = NULL;

	return 0;
}

#ifdef CONFIG_PM

void sunxi_mci_regs_save(struct sunxi_mmc_host* smc_host)
{
	struct sunxi_mmc_ctrl_regs* bak_regs = &smc_host->bak_regs;

	bak_regs->gctrl		= mci_readl(smc_host, REG_GCTRL);
	bak_regs->clkc		= mci_readl(smc_host, REG_CLKCR);
	bak_regs->timeout	= mci_readl(smc_host, REG_TMOUT);
	bak_regs->buswid	= mci_readl(smc_host, REG_WIDTH);
	bak_regs->waterlvl	= mci_readl(smc_host, REG_FTRGL);
	bak_regs->funcsel	= mci_readl(smc_host, REG_FUNS);
	bak_regs->debugc	= mci_readl(smc_host, REG_DBGC);
	bak_regs->idmacc	= mci_readl(smc_host, REG_DMAC);
#if defined(CONFIG_ARCH_SUN8IW7P1)	
	bak_regs->ntsr		= mci_readl(smc_host, REG_NTSR);
#endif
}

void sunxi_mci_regs_restore(struct sunxi_mmc_host* smc_host)
{
	struct sunxi_mmc_ctrl_regs* bak_regs = &smc_host->bak_regs;

	mci_writel(smc_host, REG_GCTRL, bak_regs->gctrl   );
	mci_writel(smc_host, REG_CLKCR, bak_regs->clkc    );
	mci_writel(smc_host, REG_TMOUT, bak_regs->timeout );
	mci_writel(smc_host, REG_WIDTH, bak_regs->buswid  );
	mci_writel(smc_host, REG_FTRGL, bak_regs->waterlvl);
	mci_writel(smc_host, REG_FUNS , bak_regs->funcsel );
	mci_writel(smc_host, REG_DBGC , bak_regs->debugc  );
	mci_writel(smc_host, REG_DMAC , bak_regs->idmacc  );
#if defined(CONFIG_ARCH_SUN8IW7P1)		
	mci_writel(smc_host, REG_NTSR , bak_regs->ntsr  );
#endif
}

static int sunxi_mci_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	int ret = 0;

	if (mmc) {
		struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
		ret = mmc_suspend_host(mmc);
		smc_host->suspend = ret ? 0 : 1;
		if (!ret && mmc_card_keep_power(mmc)) {
			sunxi_mci_regs_save(smc_host);

#ifdef CONFIG_ARCH_SUN9IW1
			// reset switch off and config clock switch off
			writel(readl(smc_host->comm_reg)&~(SDXC_CLK_SW|SDXC_RET_SW),smc_host->comm_reg);
#endif

			/* gate clock for lower power */
			clk_disable_unprepare(smc_host->mclk);
#ifdef CONFIG_ARCH_SUN9IW1
			sunxi_periph_reset_assert(smc_host->mclk);
#endif
            sunxi_mci_hold_io(smc_host);
        }

		if(ret == 0 && smc_host->cd_mode == CARD_DETECT_BY_D3){
			sunxi_mci_exit_host(smc_host);

#ifdef CONFIG_ARCH_SUN9IW1
			// reset switch off and config clock switch off
			writel(readl(smc_host->comm_reg) & ~(SDXC_CLK_SW | SDXC_RET_SW),
					smc_host->comm_reg);
#endif

			clk_disable_unprepare(smc_host->mclk);
#ifdef CONFIG_ARCH_SUN9IW1
			sunxi_periph_reset_assert(smc_host->mclk);
#endif
			sunxi_mci_hold_io(smc_host);
#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)

			if (smc_host->pdev->id == 0)
			{
			#ifndef KEEP_CARD0_POWER_SUPPLY
				if (card_power_off(smc_host)) {
					SMC_ERR(smc_host, "sdc%d card_power_off fail\n", pdev->id);
				} else {
					SMC_DBG(smc_host, "sdc%d card_power_off ok\n", pdev->id);
				}
			#endif
			}
			else
			{
				if (card_power_off(smc_host)) {
					SMC_ERR(smc_host, "sdc%d card_power_off fail\n", pdev->id);
				} else {
					SMC_DBG(smc_host, "sdc%d card_power_off ok\n", pdev->id);
				}
			}
#endif
			sunxi_mci_set_vddio(smc_host, SDC_WOLTAGE_OFF);
			usleep_range(1000, 1500);
			smc_host->voltage = SDC_WOLTAGE_OFF;
			smc_host->power_on = 0;

		}

		SMC_MSG(NULL, "smc %d suspend\n", pdev->id);
	}

	return ret;
}

static int sunxi_mci_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct mmc_host *mmc = platform_get_drvdata(pdev);
	int ret = 0;

	if (mmc) {
		struct sunxi_mmc_host *smc_host = mmc_priv(mmc);
		smc_host->suspend = 0;
		if (mmc_card_keep_power(mmc)) {
            sunxi_mci_restore_io(smc_host);
			/* enable clock for resotre */
#ifdef CONFIG_ARCH_SUN9IW1
			sunxi_periph_reset_deassert(smc_host->mclk);
#endif
			clk_prepare_enable(smc_host->mclk);

#ifdef CONFIG_ARCH_SUN9IW1
			// reset switch on and config clock switch on
			writel(readl(smc_host->comm_reg)|(SDXC_CLK_SW|SDXC_RET_SW),smc_host->comm_reg);
#endif

			sunxi_mci_regs_restore(smc_host);
			sunxi_mci_update_clk(smc_host);
		}
		if (smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ || smc_host->cd_mode == CARD_DETECT_BY_GPIO_IRQ_UP)	//hp t-card detect
			sunxi_mci_cd_cb((unsigned long)smc_host);

		if (smc_host->cd_mode == CARD_DETECT_BY_D3)
		{
			u32 rval = 0;

			sunxi_mci_set_vddio(smc_host, smc_host->regulator_voltage);
			usleep_range(1000, 1500);
			smc_host->voltage = smc_host->regulator_voltage;
			smc_host->power_on = 1;

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)

			if (smc_host->pdev->id == 0)
			{
			#ifdef KEEP_CARD0_POWER_SUPPLY
				if (!smc_host->power_for_card) {
					if (card_power_on(smc_host)) {
						SMC_ERR(smc_host, "sdc%d card_power_on fail\n", pdev->id);
					} else {
						SMC_DBG(smc_host, "sdc%d card_power_on ok\n", pdev->id);
						smc_host->power_for_card = 1;
					}
				} else {
					SMC_MSG(smc_host, "sdc%d card power suppy is already on\n", pdev->id);
				}
			#else
				if (card_power_on(smc_host)) {
					SMC_ERR(smc_host, "sdc%d card_power_on fail\n", pdev->id);
				} else {
					SMC_DBG(smc_host, "sdc%d card_power_on ok\n", pdev->id);
				}
			#endif
			} else {
				if (card_power_on(smc_host)) {
					SMC_ERR(smc_host, "sdc%d card_power_on fail\n", pdev->id);
				} else {
					SMC_DBG(smc_host, "sdc%d card_power_on ok\n", pdev->id);
				}
			}
#endif

			sunxi_mci_restore_io(smc_host);
#ifdef CONFIG_ARCH_SUN9IW1
			sunxi_periph_reset_deassert(smc_host->mclk);
#endif
			ret = clk_prepare_enable(smc_host->mclk);
			if (ret) {
				SMC_ERR(smc_host, "Failed to enable sdc%d mclk\n",
							smc_host->pdev->id);
			}

#ifdef CONFIG_ARCH_SUN9IW1
			// reset switch on and config clock switch on
			writel(readl(smc_host->comm_reg) | (SDXC_CLK_SW | SDXC_RET_SW),
					smc_host->comm_reg);
#endif

			mdelay(1);
			rval = mci_readl(smc_host, REG_RINTR);
			SMC_DBG(smc_host, ">> REG_RINTR=0x%x\n", rval);
			sunxi_mci_init_host(smc_host);
		}

		ret = mmc_resume_host(mmc);
		smc_host->suspend = ret ? 1 : 0;
		SMC_MSG(NULL, "smc %d resume\n", pdev->id);
	}

	return ret;
}

static const struct dev_pm_ops sunxi_mci_pm = {
	.suspend	= sunxi_mci_suspend,
	.resume		= sunxi_mci_resume,
};
#define sunxi_mci_pm_ops &sunxi_mci_pm

#else /* CONFIG_PM */

#define sunxi_mci_pm_ops NULL

#endif /* CONFIG_PM */

extern int mmc_go_idle(struct mmc_host *host);
extern int mmc_send_op_cond(struct mmc_host *host, u32 ocr, u32 *rocr);
extern int mmc_send_status(struct mmc_card *card, u32 *status);
extern void mmc_set_clock(struct mmc_host *host, unsigned int hz);
extern void mmc_set_timing(struct mmc_host *host, unsigned int timing);
extern void mmc_set_bus_width(struct mmc_host *host, unsigned int width);
void shutdown_mmc(struct platform_device * pdev)
{
	u32 ocr = 0;
	u32 err = 0;
	struct mmc_host *mmc = NULL;
	struct sunxi_mmc_host *smc_host = NULL;
	u32 status = 0;

	if (pdev->id != 2) {
		SMC_MSG(NULL, "%s: is not card 2, return\n", __FUNCTION__);
		goto out;
	}

	mmc = platform_get_drvdata(pdev);
	if (mmc == NULL) {
		SMC_MSG(NULL, "%s: mmc is NULL\n", __FUNCTION__);
		goto out;
	}

	smc_host = mmc_priv(mmc);
	if (smc_host == NULL) {
		SMC_MSG(NULL, "%s: smc_host is NULL\n", __FUNCTION__);
		goto out;
	}

    SMC_MSG(smc_host,"try to disable cache\n");
    err = mmc_cache_ctrl(mmc, 0);
    if (err){
        SMC_ERR(host_sunxi,"disable cache failed\n");
		mmc_claim_host(mmc);//not release host to not allow android to read/write after shutdown
         goto out;
    }

	//claim host to not allow androd read/write during shutdown
	SMC_DBG(smc_host, "%s: claim host\n", __FUNCTION__);
	mmc_claim_host(mmc);

	do {
		if (mmc_send_status(mmc->card, &status) != 0) {
			SMC_ERR(smc_host,"%s: send status failed\n", __FUNCTION__);
			goto out; //err_out; //not release host to not allow android to read/write after shutdown
		}
	} while(status != 0x00000900);

	//mmc_card_set_ddr_mode(card);
	mmc_set_timing(mmc, MMC_TIMING_LEGACY);
	mmc_set_bus_width(mmc, MMC_BUS_WIDTH_1);
	mmc_set_clock(mmc, 400000);
	err = mmc_go_idle(mmc);
	if (err) {
		SMC_ERR(smc_host, "%s: mmc_go_idle err\n", __FUNCTION__);
		goto out; //err_out; //not release host to not allow android to read/write after shutdown
	}

	if (mmc->card->type != MMC_TYPE_MMC) {//sd can support cmd1,so not send cmd1
		goto out;//not release host to not allow android to read/write after shutdown
	}

	//SMC_MSG(host_sunxi,"%s mmc_send_op_cond\n",__FUNCTION__);
	err = mmc_send_op_cond(mmc, 0, &ocr);
	if (err) {
		SMC_ERR(smc_host, "%s: first mmc_send_op_cond err\n", __FUNCTION__);
		goto out; //err_out; //not release host to not allow android to read/write after shutdown
	}

	err = mmc_send_op_cond(mmc, ocr | (1 << 30), &ocr);
	if (err) {
		SMC_ERR(smc_host, "%s: mmc_send_op_cond err\n", __FUNCTION__);
		goto out; //err_out; //not release host to not allow android to read/write after shutdown
	}

	//do not release host to not allow android to read/write after shutdown
	goto out;

/*
err_out:
	SMC_DBG(smc_host, "%s: release host\n", __FUNCTION__);
	mmc_release_host(mmc);
*/
out:
	SMC_MSG(NULL, "%s: mmc %d shutdown exit..ok\n", __FUNCTION__, pdev->id);

	return ;
}

static struct sunxi_mmc_platform_data sunxi_mci_pdata[] = {
	[0] = {
		.ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
				| MMC_VDD_32_33 | MMC_VDD_33_34,
		.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
			| MMC_CAP_SDIO_IRQ
//#if defined CONFIG_ARCH_SUN8IW1	|| defined CONFIG_ARCH_SUN9IW1
#if defined CONFIG_ARCH_SUN8IW1
			| MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50
			| MMC_CAP_UHS_DDR50
#endif
			| MMC_CAP_SET_XPC_330 | MMC_CAP_DRIVER_TYPE_A,
		.f_min = 150000,
#if defined CONFIG_ARCH_SUN9IW1
#ifdef CONFIG_SUN9IW1_PERF5_BOARD
        .f_max = 40000000,
#else
        .f_max = 48000000,
#endif
#else
		.f_max = 50000000,
#endif
		.f_ddr_max = 47000000,
#if defined CONFIG_ARCH_SUN9IW1
		.dma_tl= 0x200f0010,
#else
		.dma_tl= 0x20070008,
#endif
		.regulator=NULL,
		.mmc_clk_dly[MMC_CLK_400K] 				= {MMC_CLK_400K,				0,0},

#if defined CONFIG_ARCH_SUN8IW6P1
		.mmc_clk_dly[MMC_CLK_25M]  				= {MMC_CLK_25M,					0,7},
#else
		.mmc_clk_dly[MMC_CLK_25M]  				= {MMC_CLK_25M,					0,5},
#endif
		
#if defined CONFIG_ARCH_SUN9IW1
		.mmc_clk_dly[MMC_CLK_50M]  				= {MMC_CLK_50M,					5,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,			3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,	3,4},
		.mmc_clk_dly[MMC_CLK_100M]  			= {MMC_CLK_100M,				2,4},
		.mmc_clk_dly[MMC_CLK_200M]  			= {MMC_CLK_200M,				2,4},
#elif defined CONFIG_ARCH_SUN8IW6P1
		//in 1.2GHz pll_periph only [MMC_CLK-50M] is correct
		.mmc_clk_dly[MMC_CLK_50M]  				= {MMC_CLK_50M,					6,7},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,			2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,	2,4},
		.mmc_clk_dly[MMC_CLK_100M]  			= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]  			= {MMC_CLK_200M,				1,4},	
#else
		.mmc_clk_dly[MMC_CLK_50M]  				= {MMC_CLK_50M,					3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,			2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,	2,4},
		.mmc_clk_dly[MMC_CLK_100M]  			= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]  			= {MMC_CLK_200M,				1,4},
#endif

	},
	[1] = {
		.ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
				| MMC_VDD_32_33 | MMC_VDD_33_34,
		.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
#if defined CONFIG_ARCH_SUN8IW1	|| defined CONFIG_ARCH_SUN9IW1
			| MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50 |MMC_CAP_UHS_SDR50
//			| MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25
//			| MMC_CAP_UHS_DDR50
#endif
#if defined(CONFIG_ARCH_SUN8IW7P1)
            |MMC_CAP_UHS_DDR50
#endif
			|MMC_CAP_SDIO_IRQ,
		.f_min = 150000,
#if defined CONFIG_ARCH_SUN9IW1
        .f_max = 80000000,
#else
		.f_max = 50000000,
#endif
#if defined(CONFIG_ARCH_SUN8IW7P1)
        .f_ddr_max = 50000000,
#endif
#if defined CONFIG_ARCH_SUN9IW1
		.dma_tl= 0x200f0010,
#else
		.dma_tl= 0x20070008,
#endif
		.regulator=NULL,
		.mmc_clk_dly[MMC_CLK_400K] 				= {MMC_CLK_400K,				0,0},

#if defined CONFIG_ARCH_SUN8IW6P1
		.mmc_clk_dly[MMC_CLK_25M]  				= {MMC_CLK_25M,					0,7},
#else
		.mmc_clk_dly[MMC_CLK_25M]  				= {MMC_CLK_25M,					0,5},
#endif

#if defined CONFIG_ARCH_SUN9IW1
		.mmc_clk_dly[MMC_CLK_50M]  				= {MMC_CLK_50M,					5,0},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]		= {MMC_CLK_50MDDR_8BIT,			3,4},
		.mmc_clk_dly[MMC_CLK_100M]  			= {MMC_CLK_100M,				2,3}, //80MHz
		.mmc_clk_dly[MMC_CLK_200M]  			= {MMC_CLK_200M,				2,4},
#elif defined CONFIG_ARCH_SUN8IW6P1	
		//in 1.2GHz pll_periph only [MMC_CLK-50M] is correct
		.mmc_clk_dly[MMC_CLK_50M]  				= {MMC_CLK_50M,					6,7},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]		= {MMC_CLK_50MDDR_8BIT,			2,4},
		.mmc_clk_dly[MMC_CLK_100M]  			= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]  			= {MMC_CLK_200M,				1,4},
#else
		.mmc_clk_dly[MMC_CLK_50M]  				= {MMC_CLK_50M,					3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]		= {MMC_CLK_50MDDR_8BIT,			2,4},
		.mmc_clk_dly[MMC_CLK_100M]  			= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]  			= {MMC_CLK_200M,				1,4},
#endif
	},
	[2] = {
		.ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
				| MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195,
		.caps = MMC_CAP_4_BIT_DATA
#ifndef MMC_FPGA
			| MMC_CAP_NONREMOVABLE
#endif
			| MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		||defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)

			| MMC_CAP_1_8V_DDR /* HS-DDR mode */
#endif

#if defined CONFIG_ARCH_SUN8IW5P1 || defined CONFIG_ARCH_SUN8IW6P1 \
		|| defined CONFIG_ARCH_SUN8IW7P1 || defined CONFIG_ARCH_SUN8IW8P1 \
		|| defined CONFIG_ARCH_SUN8IW9P1 || defined CONFIG_ARCH_SUN9IW1
			| MMC_CAP_ERASE
#endif

#if defined CONFIG_ARCH_SUN8IW1	|| defined CONFIG_ARCH_SUN9IW1
			| MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50
			//| MMC_CAP_UHS_DDR50
			//| MMC_CAP_1_8V_DDR
#endif

#ifndef MMC_FPGA
			| MMC_CAP_8_BIT_DATA
#endif

			| MMC_CAP_SDIO_IRQ
			| MMC_CAP_SET_XPC_330 | MMC_CAP_DRIVER_TYPE_A,

#if defined CONFIG_ARCH_SUN8IW1	|| defined CONFIG_ARCH_SUN9IW1
		.caps2 = MMC_CAP2_HS200_1_8V_SDR,
#endif
		.f_min = 150000,
#if defined CONFIG_ARCH_SUN9IW1
		.f_max = 80000000,
		.f_ddr_max = 40000000,
#elif defined CONFIG_ARCH_SUN8IW5P1 || defined CONFIG_ARCH_SUN8IW6P1 \
		|| defined CONFIG_ARCH_SUN8IW7P1
		.f_max = 50000000,
		.f_ddr_max = 50000000,
#else
		.f_max = 50000000,
		.f_ddr_max = 30000000,
#endif

#if defined CONFIG_ARCH_SUN9IW1
		.dma_tl= 0x300f0010,
#else
		.dma_tl= 0x20070008,
#endif
		.regulator=NULL,
		.mmc_clk_dly[MMC_CLK_400K]				= {MMC_CLK_400K,				0,0},

#if defined CONFIG_ARCH_SUN8IW6P1
		.mmc_clk_dly[MMC_CLK_25M]  				= {MMC_CLK_25M,					0,7},
#else
		.mmc_clk_dly[MMC_CLK_25M]  				= {MMC_CLK_25M,					0,5},
#endif

#if defined CONFIG_ARCH_SUN9IW1
		.mmc_clk_dly[MMC_CLK_50M]				= {MMC_CLK_50M,					5,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				3,2},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]		= {MMC_CLK_50MDDR_8BIT,			2,2},
		.mmc_clk_dly[MMC_CLK_100M]				= {MMC_CLK_100M,				2,2}, //80M
		.mmc_clk_dly[MMC_CLK_200M]				= {MMC_CLK_200M,				2,4},
#elif defined CONFIG_ARCH_SUN8IW6P1
		//in 1.2GHz pll_periph only [MMC_CLK-50M] is correct
		.mmc_clk_dly[MMC_CLK_50M]				= {MMC_CLK_50M,					6,7},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,				2,4},

		.mmc_clk_dly[MMC_CLK_100M]				= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]				= {MMC_CLK_200M,				1,4},
#else
		.mmc_clk_dly[MMC_CLK_50M]				= {MMC_CLK_50M,					3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,				2,4},

		.mmc_clk_dly[MMC_CLK_100M]				= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]				= {MMC_CLK_200M,				1,4},
#endif
	},
#if defined CONFIG_ARCH_SUN8IW1 || defined CONFIG_ARCH_SUN9IW1
	[3] = {
		.ocr_avail = MMC_VDD_28_29 | MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32
				| MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195,
		.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA | MMC_CAP_NONREMOVABLE
			| MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED
//#if defined CONFIG_ARCH_SUN8IW1	|| defined CONFIG_ARCH_SUN9IW1
#if defined CONFIG_ARCH_SUN8IW1
			| MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 | MMC_CAP_UHS_SDR50
			| MMC_CAP_UHS_DDR50
			| MMC_CAP_1_8V_DDR
#endif
			| MMC_CAP_8_BIT_DATA
			| MMC_CAP_SDIO_IRQ
			| MMC_CAP_SET_XPC_330 | MMC_CAP_DRIVER_TYPE_A,
		//.caps2 = MMC_CAP2_HS200_1_8V_SDR,
		.f_min = 150000,
#if defined CONFIG_ARCH_SUN9IW1
		.f_max = 48000000,
#else
		.f_max = 50000000,
#endif
		.f_ddr_max = 50000000,
#if defined CONFIG_ARCH_SUN9IW1
		.dma_tl= 0x200f0010,
#else
		.dma_tl= MMC3_DMA_TL,
#endif
		.regulator=NULL,
		.mmc_clk_dly[MMC_CLK_400K]				= {MMC_CLK_400K,				0,0},
		.mmc_clk_dly[MMC_CLK_25M]				= {MMC_CLK_25M,					0,5},
#if defined CONFIG_ARCH_SUN9IW1
		.mmc_clk_dly[MMC_CLK_50M]				= {MMC_CLK_50M,					5,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,				3,4},
		.mmc_clk_dly[MMC_CLK_100M]				= {MMC_CLK_100M,				2,4},
		.mmc_clk_dly[MMC_CLK_200M]				= {MMC_CLK_200M,				2,4},
#else
		.mmc_clk_dly[MMC_CLK_50M]				= {MMC_CLK_50M,					3,4},
		.mmc_clk_dly[MMC_CLK_50MDDR]			= {MMC_CLK_50MDDR,				2,4},
		.mmc_clk_dly[MMC_CLK_50MDDR_8BIT]	= {MMC_CLK_50MDDR_8BIT,				2,4},
		.mmc_clk_dly[MMC_CLK_100M]				= {MMC_CLK_100M,				1,4},
		.mmc_clk_dly[MMC_CLK_200M]			= {MMC_CLK_200M,				1,4},
#endif
	},
#endif
};
static struct platform_device sunxi_mci_device[] = {
	[0] = {.name = DRIVER_NAME, .id = 0, .dev.platform_data = &sunxi_mci_pdata[0]},
	[1] = {.name = DRIVER_NAME, .id = 1, .dev.platform_data = &sunxi_mci_pdata[1]},
	[2] = {.name = DRIVER_NAME, .id = 2, .dev.platform_data = &sunxi_mci_pdata[2]},
#if defined CONFIG_ARCH_SUN8IW1	|| defined CONFIG_ARCH_SUN9IW1
	[3] = {.name = DRIVER_NAME, .id = 3, .dev.platform_data = &sunxi_mci_pdata[3]},
#endif
};

#define SUNXI_MCI_DEVNUM (sizeof(sunxi_mci_device)/sizeof(sunxi_mci_device[0]))

static struct platform_driver sunxi_mci_driver = {
	.driver.name    = DRIVER_NAME,
	.driver.owner   = THIS_MODULE,
	.driver.pm	= sunxi_mci_pm_ops,
	.probe          = sunxi_mci_probe,
	.remove         = sunxi_mci_remove,
	.shutdown	= shutdown_mmc,
};

static int sunxi_mci_get_devinfo(void)
{
	u32 i, j ,k;
	char mmc_para[16] = {0};
	struct sunxi_mmc_platform_data* mmcinfo;
	char* mmc_para_io[10] = {"sdc_clk", "sdc_cmd", "sdc_d0", "sdc_d1", "sdc_d2",
				"sdc_d3", "sdc_d4", "sdc_d5", "sdc_d6", "sdc_d7"};
	script_item_u val;
	script_item_value_type_e type;
	char *speed_mod[MMC_CLK_MOD_NUM] = {"400K","25M","50M","50MDDR","50MDDR_8BIT","100M","200M"};

	for (i=0; i<SUNXI_MCI_DEVNUM; i++) {
		mmcinfo = &sunxi_mci_pdata[i];
		sprintf(mmc_para, "mmc%d_para", i);
		/* get used information */
		type = script_get_item(mmc_para, "sdc_used", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SMC_MSG(NULL, "get mmc%d's usedcfg failed\n", i);
			goto fail;
		}
		mmcinfo->used = val.val;
		if (!mmcinfo->used)
			continue;
		/* get cdmode information */
		type = script_get_item(mmc_para, "sdc_detmode", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SMC_MSG(NULL, "get mmc%d's detmode failed\n", i);
			goto fail;
		}
		mmcinfo->cdmode = val.val;
		if (mmcinfo->cdmode == CARD_DETECT_BY_GPIO_POLL ||
			mmcinfo->cdmode == CARD_DETECT_BY_GPIO_IRQ ||
			mmcinfo->cdmode == CARD_DETECT_BY_GPIO_IRQ_UP) {	//hp t-card detect
			type = script_get_item(mmc_para, "sdc_det", &val);
			if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
				SMC_MSG(NULL, "get mmc%d's IO(sdc_cd) failed\n", i);
			} else {
				mmcinfo->cd = val.gpio;
			}
		}
		/* get buswidth information */
		type = script_get_item(mmc_para, "sdc_buswidth", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SMC_MSG(NULL, "get mmc%d's buswidth failed\n", i);
			goto fail;
		}
		mmcinfo->width = val.val;
		/* get mmc IOs information */
		for (j=0; j<mmcinfo->width+2; j++) {
			type = script_get_item(mmc_para, mmc_para_io[j], &val);
			if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
				SMC_MSG(NULL, "get mmc%d's IO(%s) failed\n", j, mmc_para_io[j]);
				goto fail;
			}
			mmcinfo->mmcio[j] = val.gpio;
		}
		/* get wpmode information */
		type = script_get_item(mmc_para, "sdc_use_wp", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SMC_MSG(NULL, "get mmc%d's wpmode failed\n", i);
			goto fail;
		}
		mmcinfo->wpmode = val.val;
		if (mmcinfo->wpmode) {
			/* if wpmode==1 but cann't get the wp IO, we assume there is no
			   write protect detection */
			type = script_get_item(mmc_para, "sdc_wp", &val);
			if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
				SMC_MSG(NULL, "get mmc%d's IO(sdc_wp) failed\n", i);
				mmcinfo->wpmode = 0;
			} else {
				mmcinfo->wp = val.gpio;
			}
		}
		/* get emmc-rst information */
		type = script_get_item(mmc_para, "emmc_rst", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_PIO) {
			mmcinfo->has_hwrst = 0;
		} else {
			mmcinfo->has_hwrst = 1;
			mmcinfo->hwrst = val.gpio;
		}
		/* get sdio information */
		type = script_get_item(mmc_para, "sdc_isio", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			SMC_MSG(NULL, "get mmc%d's isio? failed\n", i);
			goto fail;
		}
		mmcinfo->isiodev = val.val;

		/* get regulator information */
		type = script_get_item(mmc_para, "sdc_regulator", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_STR) {
			SMC_MSG(NULL, "get mmc%d's sdc_regulator failed\n", i);
			mmcinfo->regulator = NULL;
		} else {
			if (!strcmp(val.str, "none")) {
				mmcinfo->regulator = NULL;
				/* If here found that no regulator can be used for this card,
				   we clear all of the UHS features support.
				   But there is a excetpion. SD/MMC controller 1 is used for sdio wifi,
				   If there is a sdio v3.0 wifi, we should make wifi io voltage power
				   up with 1.8V and not configure "sdc_regulator" in [mmc1_para].
				   During initial process, the driver do not switch io voltage and switch
				   speed mode according to device capabilites. */
				if (i != 1) {
					mmcinfo->caps &= ~(MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25
							| MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_DDR50);
				}
			} else
				mmcinfo->regulator = val.str;
		}

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
	|| defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1) \
	|| defined(CONFIG_ARCH_SUN8IW9P1)
		/* get power information */
		type = script_get_item(mmc_para, "sdc_power_supply", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_STR)
		{
			SMC_MSG(NULL, "get mmc%d's sdc_power failed\n", i);
			mmcinfo->power_supply = NULL;
		}
		else
		{
			if (!strcmp(val.str, "none"))
			{
				mmcinfo->power_supply = NULL;
				SMC_MSG(NULL, "get mmc%d's sdc_power is null!\n", i);
			}
			else
			{
				mmcinfo->power_supply = val.str;
			}
			//SMC_MSG(NULL, "get mmc%d's power supply '%s' ok\n", i,mmcinfo->power_supply);
		}
#endif

#if defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) \
		|| defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW7P1) \
		|| defined(CONFIG_ARCH_SUN8IW9P1)
		/* get 2xmode information */
		type = script_get_item(mmc_para, "sdc_2xmode", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			//SMC_MSG(NULL, "get mmc%d's 2xmode fail\n", i);
			mmcinfo->used_2xmod = 0;
		} else {
			mmcinfo->used_2xmod = val.val;
			SMC_MSG(NULL, "get mmc%d's 2xmode ok, val = %d\n", i, mmcinfo->used_2xmod);
		}
		/* get ddrmode information */
		type = script_get_item(mmc_para, "sdc_ddrmode", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			//SMC_MSG(NULL, "get mmc%d's ddrmode fail\n", i);
			mmcinfo->used_ddrmode = 0;
		} else {
			mmcinfo->used_ddrmode = val.val;
			SMC_MSG(NULL, "get mmc%d's ddrmode ok, val = %d\n", i, mmcinfo->used_ddrmode);
		}
#endif

		/* get f_max */
		type = script_get_item(mmc_para, "sdc_f_max", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			//SMC_MSG(NULL, "get mmc%d's sdc_f_max failed,use default sdc_f_max %d\n",i, mmcinfo->f_max);
		}else{
			if ((val.val>mmcinfo->f_max)||(val.val<mmcinfo->f_min)){
				//SMC_MSG(NULL, "mmc%d's input sdc_f_max wrong,use default sdc_f_max %d\n", i,  mmcinfo->f_max);
			}else{
				mmcinfo->f_max = val.val;
				SMC_MSG(NULL, "get mmc%d's sdc_f_max ok,sdc_f_max %d\n", i, mmcinfo->f_max);
			}
		}

		/* get f_ddr_max */
		type = script_get_item(mmc_para, "sdc_f_ddr_max", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			//SMC_MSG(NULL, "get mmc%d's sdc_f_ddr_max failed,use default sdc_f_ddr_max %d\n",i, mmcinfo->f_ddr_max);
		}else{
			if((val.val>mmcinfo->f_ddr_max)||(val.val<mmcinfo->f_min)){
				//SMC_MSG(NULL, "mmc%d's input sdc_f_ddr_max wrong,use default sdc_f_ddr_max %d\n", i,  mmcinfo->f_ddr_max);
			}else{
				mmcinfo->f_ddr_max = val.val;
				SMC_MSG(NULL, "get mmc%d's sdc_f_ddr_max ok,sdc_f_ddr_max %d\n", i, mmcinfo->f_ddr_max);
			}
		}


		/*
			get platform capability
				#define MMC_HOST_PLATFORM_CAP_DIS_SECURE_PURGE  (1U<<0)
				#define MMC_HOST_PLATFORM_CAP_DIS_TRIM          (1U<<1)
				#define MMC_HOST_PLATFORM_CAP_DIS_SANITIZE      (1U<<2)
				#define MMC_HOST_PLATFORM_CAP_ENA_CACHE			(1u<<4)
		*/
		type = script_get_item(mmc_para, "sdc_platform_cap", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			mmcinfo->platform_cap = 0;
		} else {
			mmcinfo->platform_cap = val.val;
			if (mmcinfo->platform_cap & MMC_HOST_PLATFORM_CAP_DIS_SECURE_PURGE)
				SMC_MSG(NULL, "mmc%d disable secure purge\n", i);
			if (mmcinfo->platform_cap & MMC_HOST_PLATFORM_CAP_DIS_TRIM)
				SMC_MSG(NULL, "mmc%d disable trim\n", i);
			if (mmcinfo->platform_cap & MMC_HOST_PLATFORM_CAP_DIS_SANITIZE)
				SMC_MSG(NULL, "mmc%d disable sanitize\n", i);
			if (mmcinfo->platform_cap & MMC_HOST_PLATFORM_CAP_DIS_SECURE_WIPE_OP)
				SMC_MSG(NULL, "mmc%d disable secure wipe\n", i);
			if(mmcinfo->platform_cap & MMC_HOST_PLATFORM_CAP_ENA_CACHE){
				mmcinfo->caps2 |= MMC_CAP2_CACHE_CTRL;
                SMC_MSG(NULL,"mmc %d enable cache\n",i);
            }
        }

		/* get ex_dly_used information */
		type = script_get_item(mmc_para, "sdc_ex_dly_used", &val);
		if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
			//SMC_MSG(NULL, "get mmc%d's sdc_ex_dly_used failed, use default dly\n", i);
			continue;
		}
		/* 1, manual mode: delay parameters are from sys_config.fex directly;
		   2, auto mode  : delay parameters are from sys_config.fex, but it maybe changed during update firmware. */
		if ((val.val != 1)&&(val.val != 2)) {
			//SMC_MSG(NULL, "mmc%d's not use sdc_ex_dly, use default dly\n", i);
			continue;
		}

		/* get odly and sdly information */
		for(k = 0;k< MMC_CLK_MOD_NUM; k++) {
			char str_buf[30] = {0};

			sprintf(str_buf,"sdc_odly_%s",speed_mod[k]);
			type = script_get_item(mmc_para, str_buf, &val);
			if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
				//SMC_MSG(NULL, "get mmc%d's %s failed,use default dly %d\n",i, str_buf, mmcinfo->mmc_clk_dly[k].oclk_dly);
			}else{
				if((val.val>7)||(val.val<0)){
					//SMC_MSG(NULL, "mmc%d's input %s wrong,use default dly %d\n", i, str_buf, mmcinfo->mmc_clk_dly[k].oclk_dly);
				}else{
					mmcinfo->mmc_clk_dly[k].oclk_dly = val.val;
					SMC_MSG(NULL, "get mmc%d's %s ok,oclk_dly %d\n", i, str_buf,mmcinfo->mmc_clk_dly[k].oclk_dly);
				}
			}

			sprintf(str_buf,"sdc_sdly_%s",speed_mod[k]);
			type = script_get_item(mmc_para, str_buf, &val);
			if (type != SCIRPT_ITEM_VALUE_TYPE_INT) {
				//SMC_MSG(NULL, "get mmc%d's %s failed,use default dly %d\n", i, str_buf, mmcinfo->mmc_clk_dly[k].sclk_dly);
			}else{
				if((val.val>7)||(val.val<0)){
					//SMC_MSG(NULL, "mmc%d's input %s wrong,use default dly %d\n", i, str_buf, mmcinfo->mmc_clk_dly[k].sclk_dly);
				}else{
					mmcinfo->mmc_clk_dly[k].sclk_dly = val.val;
					SMC_MSG(NULL, "get mmc%d's %s ok,sclk_dly %d\n", i, str_buf, mmcinfo->mmc_clk_dly[k].sclk_dly);
				}
			}
		}

	}


	return 0;
fail:
	return -1;
}


static int storage_type;
static int __init setup_bootdev(char *str)
{
	storage_type = simple_strtoul(str,NULL,0);
	printk("%s %d\n", __func__, storage_type);
	return 1;
}
__setup("storage_type=", setup_bootdev);

static int __init sunxi_mci_init(void)
{
	int i;
	int sdc_used = 0;
	int boot_card = 0;
	int io_used = 0;
	struct sunxi_mmc_platform_data* mmcinfo;

	SMC_MSG(NULL, "%s\n", DRIVER_VERSION);

	/* get devices information from sys_config.fex */
	if (sunxi_mci_get_devinfo()) {
		SMC_MSG(NULL, "parse sys_cofnig.fex info failed\n");
		return 0;
	}

	/*
	 * Here we check whether there is a boot card. If the boot card exists,
	 * we register it firstly to make it be associatiated with the device
	 * node 'mmcblk0'. Then the applicantions of Android can fix the boot,
	 * system, data patitions on mmcblk0p1, mmcblk0p2... etc.
	 */
	for (i=0; i<SUNXI_MCI_DEVNUM; i++) {
		mmcinfo = &sunxi_mci_pdata[i];
		if (mmcinfo->used) {
			sdc_used |= 1 << i;
			if (storage_type == 1)
				boot_card |= 1 << 0;
			else if (storage_type == 2)
				boot_card |= 1 << 2;
			if (mmcinfo->isiodev)
				io_used |= 1 << i;
		}
	}

	SMC_MSG(NULL, "MMC host used card: 0x%x, boot card: 0x%x, io_card %d\n",
					sdc_used, boot_card, io_used);
	/* register boot card firstly */
	for (i=0; i<SUNXI_MCI_DEVNUM; i++) {
		if (boot_card & (1 << i)) {
			platform_device_register(&sunxi_mci_device[i]);
			device_enable_async_suspend(&sunxi_mci_device[i].dev);
		}
	}
	/* register other cards */
	for (i=0; i<SUNXI_MCI_DEVNUM; i++) {
		if (boot_card & (1 << i))
			continue;
		if (sdc_used & (1 << i)) {
			platform_device_register(&sunxi_mci_device[i]);
			device_enable_async_suspend(&sunxi_mci_device[i].dev);
		}
	}

	return platform_driver_register(&sunxi_mci_driver);
}

static void __exit sunxi_mci_exit(void)
{
	SMC_MSG(NULL, "sunxi_mci_exit\n");
	platform_driver_unregister(&sunxi_mci_driver);
}


module_init(sunxi_mci_init);
module_exit(sunxi_mci_exit);

MODULE_DESCRIPTION("Winner's SD/MMC Card Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Aaron.maoye<leafy.myeh@reuuimllatech.com>");
MODULE_ALIAS("platform:sunxi-mmc");
