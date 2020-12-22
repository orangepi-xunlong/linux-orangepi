/*
 * drivers/usb/host/sunxi_hci.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * yangnaitian, 2011-5-24, create this file
 * javen, 2011-7-18, add clock and power switch
 *
 * sunxi HCI Driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#include <linux/gpio.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/dma-mapping.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>

#include  "sunxi_hci.h"

#ifdef CONFIG_USB_SUNXI_USB_MANAGER
#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
int usb_otg_id_status(void);
#endif
#endif

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW9) || defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
#define  USBPHYC_REG_o_PHYCTL		    0x0410
#else
#define  USBPHYC_REG_o_PHYCTL		    0x0404
#endif

#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
#define HCI0_USBC_NO    0
#define HCI1_USBC_NO    1
#define HCI2_USBC_NO    2
#define HCI3_USBC_NO    3
#else
#define HCI0_USBC_NO    1
#define HCI1_USBC_NO    2
#define HCI2_USBC_NO    3
#define HCI3_USBC_NO    4
#endif

#ifdef CONFIG_ARCH_SUN9IW1
#define SUNXI_CCM_VBASE  SUNXI_CCM_MOD_VBASE
#define SUNXI_SRAMCTRL_VBASE SUNXI_SRAM_C_VBASE
#define HCI_PHY_CONTROL_REGISTER 0x04
#define HCI_SIE_CONTROL_REGISTER 0x00

#define VBAT_USBH_VALUE  3000000
#define VBAT_HSIC_VALUE  1200000
static u32 usb_hci_clock_cnt = 0;
static struct clk *hci_ahb_gate = NULL;
#ifdef CONFIG_USB_SUNXI_HSIC
static struct regulator* vbat_hsic_hdle = NULL;
//static char *vbat_hsic_name = "axp15_bldo4";
static char *vbat_hsic_name = NULL;
#else
static struct regulator* vbat_usbh_hdle = NULL;
//static char *vbat_usbh_name = "axp22_aldo1";
static char *vbat_usbh_name = NULL;
#endif
static u32 usbc_base[4] 			= {0, (u32 __force)SUNXI_USB_HCI0_VBASE, (u32 __force)SUNXI_USB_HCI1_VBASE, (u32 __force)SUNXI_USB_HCI2_VBASE};
static u32 ehci_irq_no[4] 			= {0, SUNXI_IRQ_USB_EHCI0, SUNXI_IRQ_USB_EHCI1, SUNXI_IRQ_USB_EHCI2};
static u32 ohci_irq_no[4] 			= {0, SUNXI_IRQ_USB_OHCI0, SUNXI_IRQ_USB_OHCI1, SUNXI_IRQ_USB_OHCI2};
#ifndef  SUNXI_USB_FPGA
static u32 usb1_open_hci_clock_cnt = 0;
static u32 usb2_open_hci_clock_cnt = 0;
static u32 usb3_open_hci_clock_cnt = 0;

#ifdef CONFIG_USB_HCD_ENHANCE
struct task_struct *hci1 = NULL;
struct task_struct *hci2 = NULL;
struct task_struct *hci3 = NULL;
atomic_t hci1_thread_scan_flag;
atomic_t hci2_thread_scan_flag;
atomic_t hci3_thread_scan_flag;

int hci1_thread_run_flag = 0;
int hci2_thread_run_flag = 0;
int hci3_thread_run_flag = 0;

int sunxi_usb_disable_ehci(__u32 usbc_no);
int sunxi_usb_enable_ehci(__u32 usbc_no);
int sunxi_usb_disable_ohci(__u32 usbc_no);
int sunxi_usb_enable_ohci(__u32 usbc_no);

static int hci1_scan_thread(void * pArg)
{
	struct sunxi_hci_hcd *sunxi_hci = pArg;
	int phy_value = 0;
	int sie_value = 0;
	int passby = 0;
	int ehci10 = 0;
	int ehci18 = 0;
	int ehci50 = 0;
	int ohci404 = 0;
	int ohci410 = 0;
	int i = 5;

	while(hci1_thread_run_flag) {

		msleep(1000);  /* 1s */
		if(atomic_read(&hci1_thread_scan_flag)){
			msleep(3000);  /* 1s */

			phy_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER);
			sie_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER);
			passby    = USBC_Readl(sunxi_hci->usb_vbase + 0x800);
			ehci10    = USBC_Readl(sunxi_hci->usb_vbase + 0x10);
			ehci18    = USBC_Readl(sunxi_hci->usb_vbase + 0x18);
			ehci50    = USBC_Readl(sunxi_hci->usb_vbase + 0x50);
			ohci404   = USBC_Readl(sunxi_hci->usb_vbase + 0x404);
			ohci410	  = USBC_Readl(sunxi_hci->usb_vbase + 0x410);
			printk("%x,%x,%x,%x,%x,%d\n", ehci10, ehci18,ehci50,ohci404, ohci410, __LINE__);

			if(!(((sie_value & 0x20006) == 0x20006) && ((phy_value & 0x20002) == 0x20002) && ((passby & 0x701) == 0x701)
				 && ((ehci18 & 0x7) == 0x7) && ((ehci50 & 0x1)== 0x1)
				&& ((ohci404 & 0x3) == 0x3) && ((ohci410 & 0xa) == 0xa))){
				if(atomic_read(&hci1_thread_scan_flag)){
					atomic_set(&hci1_thread_scan_flag, 0);
					atomic_set(&hci3_thread_scan_flag, 0);

					sunxi_usb_disable_ehci(1);
					sunxi_usb_disable_ohci(1);
					sunxi_usb_enable_ehci(1);
					while((sunxi_usb_enable_ohci(1) && i--));
					i = 5;
					atomic_set(&hci1_thread_scan_flag, 1);
					atomic_set(&hci3_thread_scan_flag, 1);
				}
			}
		}
	}
	return 0;
}


static int hci2_scan_thread(void * pArg)
{
	while(hci2_thread_run_flag) {

		msleep(1000);  /* 1s */
	}

	return 0;
}

static int hci3_scan_thread(void * pArg)
{
	struct sunxi_hci_hcd *sunxi_hci = pArg;
	int phy_value = 0;
	int sie_value = 0;
	int passby = 0;
	int ehci10 = 0;
	int ehci18 = 0;
	int ehci50 = 0;
	int ohci404 = 0;
	int ohci410 = 0;
	int i = 5;
	while(hci3_thread_run_flag) {
		msleep(1000);  /* 1s */

		if(atomic_read(&hci3_thread_scan_flag)){
			msleep(3000);  /* 1s */

			phy_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER);
			sie_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER);

			passby    = USBC_Readl(sunxi_hci->usb_vbase + 0x800);
			ehci10    = USBC_Readl(sunxi_hci->usb_vbase + 0x10);
			ehci18    = USBC_Readl(sunxi_hci->usb_vbase + 0x18);
			ehci50    = USBC_Readl(sunxi_hci->usb_vbase + 0x50);
			ohci404   = USBC_Readl(sunxi_hci->usb_vbase + 0x404);
			ohci410	  = USBC_Readl(sunxi_hci->usb_vbase + 0x410);

			printk("%x,%x,%x,%x,%x,%x,%x,%x,%d\n", phy_value,sie_value , passby,ehci10, ehci18,ehci50,ohci404, ohci410, __LINE__);

			if(!(((sie_value & 0x80060) ==  0x80060) && ((phy_value & 0x200020) == 0x200020) && ((passby & 0x701) == 0x701)
					&& ((ehci18 & 0x7) == 0x7) && ((ehci50 & 0x1)== 0x1)
					&& ((ohci404 & 0x3) == 0x3) && ((ohci410 & 0xa) == 0xa))){
				if(atomic_read(&hci3_thread_scan_flag)){
					atomic_set(&hci1_thread_scan_flag, 0);
					atomic_set(&hci3_thread_scan_flag, 0);
					sunxi_usb_disable_ehci(3);
					sunxi_usb_disable_ohci(3);
					sunxi_usb_enable_ehci(3);
					while((sunxi_usb_enable_ohci(3) && i--));
					i = 5;
					atomic_set(&hci1_thread_scan_flag, 1);
					atomic_set(&hci3_thread_scan_flag, 1);
			    }
			}
		}
	}
	return 0;
}
#endif
static char* usbc_name[4] 			= {"usbc0", "usbc1", "usbc2", "usbc3"};

static void sunxi_get_usbh_power(void)
{
	script_item_value_type_e type;
	script_item_u item_temp;

#ifdef CONFIG_USB_SUNXI_HSIC
	type = script_get_item("usb_power", "hsic_power", &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			vbat_hsic_name = NULL;
			DMSG_INFO("get usb_regulator is nocare line: %d\n",__LINE__);
		} else {
			vbat_hsic_name = item_temp.str;
			DMSG_INFO("hsic_power: %s line: %d\n", vbat_hsic_name,__LINE__);
		}
	}else{
		DMSG_INFO("get usb_power usb_regulator is fail: %d\n",__LINE__);
	}
#else
	type = script_get_item("usb_power", "usbh_power", &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			vbat_usbh_name = NULL;
			DMSG_INFO("get usb_regulator is nocare line: %d\n",__LINE__);
		} else {
			vbat_usbh_name = item_temp.str;
			DMSG_INFO("usbh_power: %s line: %d\n", vbat_usbh_name,__LINE__);
		}
	}else{
		DMSG_INFO("get usbh_power usb_regulator is fail: %d\n",__LINE__);
	}
#endif

}
static void set_hci_phy_ctrl_register(int mask)
{

	int reg_value = 0;
	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER);
	reg_value |= mask;// enable SCLK_GATING_HCI0_PHY
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER);
	return;
}
static void clear_hci_phy_ctrl_register(int mask)
{

	int reg_value = 0;
	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER);
	reg_value &= ~mask;// disablde SCLK_GATING_HCI0_PHY
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER);
	return;
}

static void set_hci_sie_ctrl_register(int mask)
{
	int reg_value = 0;
	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER);
	reg_value |= mask;// enable SCLK_GATING_HCI0_PHY
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER);
    return;
}
static void clear_hci_sie_ctrl_register(int mask)
{

	int reg_value = 0;
	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER);
	reg_value &= ~mask;// disablde SCLK_GATING_HCI0_PHY
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER);
	return;
}

static int hci_clock_ctrl(struct sunxi_hci_hcd *sunxi_hci, u32 ohci, int enable)
{

	spinlock_t lock;
	unsigned long flags = 0;
	int i = 0;
	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);
	DMSG_INFO("[%s]: open hci clock,usbc_no:%d, is_ohci:%d, %d\n", sunxi_hci->hci_name, sunxi_hci->usbc_no, ohci, enable);

	if(sunxi_hci->usbc_no == 1){
		if(enable && usb1_open_hci_clock_cnt == 0){
			set_hci_phy_ctrl_register(0x20002);//bit1 17
			for(i = 0; i < 0x10; i++);
			set_hci_sie_ctrl_register(0x20002);//bit1 17

		}else if(!enable && usb1_open_hci_clock_cnt == 1){
			clear_hci_phy_ctrl_register(0x20002);//bit1 17
			for(i = 0; i < 0x10; i++);
			clear_hci_sie_ctrl_register(0x20002);//bit1 17
		}

		if(enable){
			if(ohci){
				set_hci_sie_ctrl_register(0x4);//bit2
			}
			usb1_open_hci_clock_cnt++;
		}else{
			if(ohci){
				clear_hci_sie_ctrl_register(0x4);//bit2
			}
			usb1_open_hci_clock_cnt--;
		}
	}else if(sunxi_hci->usbc_no == 2){
		if(enable && usb2_open_hci_clock_cnt == 0){
			clear_hci_phy_ctrl_register(0x80008);//bit3 19
			set_hci_phy_ctrl_register(0x160406);//bit1 2 10 17 18 20
			for(i = 0; i < 0x10; i++);
			set_hci_sie_ctrl_register(0x40008);//bit3 18
#ifdef CONFIG_USB_HCD_ENHANCE
			hci2_thread_run_flag = 1;
#endif

		}else if(!enable && usb2_open_hci_clock_cnt == 1){
			clear_hci_phy_ctrl_register(0xc040c);//bit2 3 10 18 19
			for(i = 0; i < 0x10; i++);
			clear_hci_sie_ctrl_register(0x40018);
#ifdef CONFIG_USB_HCD_ENHANCE
			hci2_thread_run_flag = 0;
#endif
		}

		if(enable){
			usb2_open_hci_clock_cnt++;
		}else{
			usb2_open_hci_clock_cnt--;
		}
	}else if(sunxi_hci->usbc_no == 3){
		if(enable && usb3_open_hci_clock_cnt == 0){

			clear_hci_phy_ctrl_register(0x100410);//bit4 10 20
			set_hci_phy_ctrl_register(0x200020);//bit5 21
			for(i = 0; i < 0x10; i++);
			set_hci_sie_ctrl_register(0x80020);//bit5 19
#ifdef CONFIG_USB_HCD_ENHANCE
			hci3_thread_run_flag = 1;
#endif
		}else if(!enable && usb3_open_hci_clock_cnt == 1){
#ifdef CONFIG_USB_HCD_ENHANCE
			hci3_thread_run_flag = 0;
#endif
			clear_hci_phy_ctrl_register(0x300430);//bit4 5 10 20 21
			for(i = 0; i < 0x10; i++);
			clear_hci_sie_ctrl_register(0x80020);//bit5 19
		}

		if(enable){
			if(ohci){
				set_hci_sie_ctrl_register(0x40);//bit6
			}
			usb3_open_hci_clock_cnt++;
		}else{
			usb3_open_hci_clock_cnt--;
			if(ohci){
				clear_hci_sie_ctrl_register(0x40);//bit6
			}
		}
	}else{
		DMSG_PANIC("EER: unkown usbc_no(%d)\n", sunxi_hci->usbc_no);
		spin_unlock_irqrestore(&lock, flags);
		return 0;
	}
#ifdef CONFIG_USB_HCD_ENHANCE
	DMSG_INFO("[%s]: hci_clock_ctrl,usbc_no:%d, is_ohci:%d, %d, SIE:%x,phy:%x,c:%x,p:%x\n", sunxi_hci->hci_name, sunxi_hci->usbc_no, ohci, enable,
		USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_SIE_CONTROL_REGISTER),USBC_Readl(SUNXI_USB_CTRL_VBASE + HCI_PHY_CONTROL_REGISTER),
		USBC_Readl(0xf6000584),USBC_Readl(sunxi_hci->usb_vbase + 0x800));
#endif
	spin_unlock_irqrestore(&lock, flags);

	return 0;
}
#endif

#else

#ifndef SUNXI_USB_FPGA /* kill warning */

#ifdef CONFIG_ARCH_SUN8IW8
static char* usbc_name[1] 			= {"usbc0"};
static char* usbc_ahb_ehci_name[1]  = {USBEHCI_CLK};
static char* usbc_ahb_ohci_name[1]  = {USBOHCI_CLK};
static char* usbc_phy_name[1]	= {USBPHY0_CLK};
static u32 usbc_base[1] 			= {(u32 __force)SUNXI_USB_HCI0_VBASE};
static u32 ehci_irq_no[1] 			= {SUNXI_IRQ_USBEHCI0};
static u32 ohci_irq_no[1] 			= {SUNXI_IRQ_USBOHCI0};
#endif

#ifdef  CONFIG_ARCH_SUN8IW7
static char* usbc_name[4] 			= {"usbc0", "usbc1", "usbc2", "usbc3"};
static char* usbc_ahb_ehci_name[4]  = {USBEHCI0_CLK, USBEHCI1_CLK, USBEHCI2_CLK, USBEHCI3_CLK};
static char* usbc_ahb_ohci_name[4]  = {USBOHCI0_CLK, USBOHCI1_CLK, USBOHCI2_CLK,USBOHCI3_CLK};
static char* usbc_phy_name[4]	= {USBPHY0_CLK, USBPHY1_CLK, USBPHY2_CLK, USBPHY3_CLK};
static u32 usbc_base[4] 			= {(u32 __force)SUNXI_USB_HCI0_VBASE, (u32 __force)SUNXI_USB_HCI1_VBASE, (u32 __force)SUNXI_USB_HCI2_VBASE, (u32 __force)SUNXI_USB_HCI3_VBASE};
static u32 ehci_irq_no[4] 			= {SUNXI_IRQ_USBEHCI0, SUNXI_IRQ_USBEHCI1, SUNXI_IRQ_USBEHCI2, SUNXI_IRQ_USBEHCI3};
static u32 ohci_irq_no[4] 			= {SUNXI_IRQ_USBOHCI0, SUNXI_IRQ_USBOHCI1, SUNXI_IRQ_USBOHCI2, SUNXI_IRQ_USBOHCI3};
#endif

#ifdef CONFIG_ARCH_SUN8IW6
static char* usbc_name[3] 			= {"usbc0", "usbc1", "usbc2"};
static char* usbc_ahb_ehci_name[3]  = {"", USBEHCI0_CLK, USBEHCI1_CLK};
static char* usbc_ahb_ohci_name[2]  = {"", USBOHCI_CLK};
static char* usbc_phy_name[3]	= {"", USBPHY1_CLK, USBHSIC_CLK};
static char* usbc_hsic_phy_name[3]	= {"", "", USBHSIC_CLK};
static u32 usbc_base[3] 			= {(u32 __force)SUNXI_USB_OTG_VBASE, (u32 __force)SUNXI_USB_HCI0_VBASE, (u32 __force)SUNXI_USB_HCI1_VBASE};
static u32 ehci_irq_no[3] 			= {0, SUNXI_IRQ_USBEHCI0, SUNXI_IRQ_USBEHCI1};
static u32 ohci_irq_no[2] 			= {0, SUNXI_IRQ_USBOHCI0};
#endif

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW3) || defined (CONFIG_ARCH_SUN8IW9)
static char* usbc_name[2] 			= {"usbc0", "usbc1"};
static char* usbc_ahb_ehci_name[2]  = {"", USBEHCI_CLK};
static char* usbc_ahb_ohci_name[2]  = {"", USBOHCI_CLK};
static char* usbc_phy_name[2]	= {"", USBPHY1_CLK};
static char* usbc_hsic_phy_name[2]	= {"", USBHSIC_CLK};
static u32 usbc_base[2] 			= {(u32 __force)SUNXI_USB_OTG_VBASE, (u32 __force)SUNXI_USB_HCI0_VBASE};
static u32 ehci_irq_no[2] 			= {0, SUNXI_IRQ_USBEHCI0};
static u32 ohci_irq_no[2] 			= {0, SUNXI_IRQ_USBOHCI0};
#endif

#if defined (CONFIG_ARCH_SUN8IW1)
static char* usbc_name[4] 			= {"usbc0", "usbc1", "usb2", "usb3"};
static char* usbc_ahb_ehci_name[3]  = {"", USBEHCI0_CLK, USBEHCI1_CLK};
static char* usbc_ahb_ohci_name[4]  = {"", USBOHCI0_CLK, USBOHCI1_CLK, USBOHCI3_CLK};
static char* usbc_phy_name[3]	= {"", USBPHY1_CLK, USBPHY2_CLK};
static u32 usbc_base[4] 			= {(u32 __force)SUNXI_USB_OTG_VBASE, (u32 __force)SUNXI_USB_HCI0_VBASE, (u32 __force)SUNXI_USB_HCI1_VBASE, (u32 __force)SUNXI_USB_OHCI2_VBASE};
static u32 ehci_irq_no[3] 			= {0, SUNXI_IRQ_USBEHCI0, SUNXI_IRQ_USBEHCI1};
static u32 ohci_irq_no[4] 			= {0, SUNXI_IRQ_USBOHCI0, SUNXI_IRQ_USBOHCI1, SUNXI_IRQ_USBOHCI2};

#endif


#else
static u32 usbc_base[4] 			= {(u32 __force)SUNXI_USB_OTG_VBASE, (u32 __force)SUNXI_USB_HCI0_VBASE, (u32 __force)SUNXI_USB_HCI0_VBASE, (u32 __force)SUNXI_USB_HCI0_VBASE};
static u32 ehci_irq_no[3] 			= {0, SUNXI_IRQ_USBEHCI0, SUNXI_IRQ_USBEHCI0};
static u32 ohci_irq_no[4] 			= {0, SUNXI_IRQ_USBOHCI0, SUNXI_IRQ_USBOHCI0, SUNXI_IRQ_USBOHCI0};
#endif

#endif
static u32 usb1_set_vbus_cnt = 0;
static u32 usb2_set_vbus_cnt = 0;
static u32 usb3_set_vbus_cnt = 0;
static u32 usb4_set_vbus_cnt = 0;

static u32 usb1_enable_passly_cnt = 0;
static u32 usb2_enable_passly_cnt = 0;
static u32 usb3_enable_passly_cnt = 0;
static u32 usb4_enable_passly_cnt = 0;

static u32 usb1_enable_phy_cnt = 0;
static u32 usb2_enable_phy_cnt = 0;
static u32 usb3_enable_phy_cnt = 0;
static u32 usb4_enable_phy_cnt = 0;

#ifndef  SUNXI_USB_FPGA
static void sunxi_usb_3g_config(struct sunxi_hci_hcd *sunxi_hci)
{
	script_item_value_type_e type = 0;
	script_item_u item_temp;
	u32 usb_3g_used	  = 0;
	u32 usb_3g_usbc_num  = 0;
	u32 usb_3g_usbc_type = 0;

	/* 3g_used */
	type = script_get_item("3g_para", "3g_used", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		usb_3g_used = item_temp.val;
	}else{
		//DMSG_PANIC("WRN: script_parser_fetch usb_3g_used failed\n");
		return;
	}

	/* 3g_usbc_num */
	type = script_get_item("3g_para", "3g_usbc_num", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		usb_3g_usbc_num = item_temp.val;
	}else{
		//DMSG_PANIC("WRN: script_parser_fetch usb_3g_usbc_num failed\n");
		return;
	}

	/* 3g_usbc_type */
	type = script_get_item("3g_para", "3g_usbc_type", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		usb_3g_usbc_type = item_temp.val;
	}else{
		//DMSG_PANIC("WRN: script_parser_fetch usb_3g_usbc_type failed\n");
		return;
	}

	/* only open the controller witch used by 3G */
	if(sunxi_hci->usbc_no == usb_3g_usbc_num){
		sunxi_hci->used = 0;
		if(sunxi_hci->usbc_type == usb_3g_usbc_type){
			sunxi_hci->used = 1;
		}
	}
	return;
}
#endif

static s32 request_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{
	if(sunxi_hci->regulator_io != NULL){
		sunxi_hci->regulator_io_hdle= regulator_get(NULL, sunxi_hci->regulator_io);
		if(IS_ERR(sunxi_hci->regulator_io_hdle)) {
			DMSG_PANIC("ERR: some error happen, %s,regulator_io_hdle fail to get regulator!", sunxi_hci->hci_name);
			return 0;
		}

		if(regulator_set_voltage(sunxi_hci->regulator_io_hdle , sunxi_hci->regulator_value, sunxi_hci->regulator_value) < 0 ){
			DMSG_PANIC("ERR: regulator_set_voltage: %s fail\n",sunxi_hci->hci_name);
			regulator_put(sunxi_hci->regulator_io_hdle);
			return 0;
		}
	}

	return 0;
}

static s32 release_usb_regulator_io(struct sunxi_hci_hcd *sunxi_hci)
{
	if(sunxi_hci->regulator_io != NULL){
		regulator_put(sunxi_hci->regulator_io_hdle);
	}
	return 0;
}

static s32 get_usb_cfg(struct sunxi_hci_hcd *sunxi_hci)
{
#ifndef  SUNXI_USB_FPGA
	script_item_value_type_e type = 0;
	script_item_u item_temp;

	/* usbc enable */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_used", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->used = item_temp.val;
	}else{
		DMSG_INFO("get %s usbc enable failed\n" ,sunxi_hci->hci_name);
		sunxi_hci->used = 0;
	}

	/* usbc restrict_gpio */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_restrict_gpio", &sunxi_hci->restrict_gpio_set);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
		sunxi_hci->usb_restrict_valid = 1;
	}else{
		DMSG_INFO("%s(restrict_gpio) is invalid\n", sunxi_hci->hci_name);
		sunxi_hci->usb_restrict_valid = 0;
	}

	/* usbc drv_vbus */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_drv_vbus_gpio", &sunxi_hci->drv_vbus_gpio_set);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
		sunxi_hci->drv_vbus_gpio_valid = 1;
	}else{
		DMSG_INFO("%s(drv vbus) is invalid\n", sunxi_hci->hci_name);
		sunxi_hci->drv_vbus_gpio_valid = 0;
	}

	/* host_init_state */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_host_init_state", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->host_init_state = item_temp.val;
	}else{
		DMSG_INFO("script_parser_fetch host_init_state failed\n");
		sunxi_hci->host_init_state = 1;
	}

	/* get usb_restrict_flag */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_restric_flag", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->usb_restrict_flag = item_temp.val;
	}else{
		DMSG_INFO("get usb_restrict_flag failed\n");
		sunxi_hci->usb_restrict_flag = 0;
	}

	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_not_suspend", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->not_suspend = item_temp.val;
	}else{
		DMSG_INFO("get usb_restrict_flag failed\n");
		sunxi_hci->not_suspend = 0;
	}

	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_hsic_used", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
		sunxi_hci->hsic_flag = item_temp.val;
	}else{
		DMSG_INFO("get usb_restrict_flag failed\n");
		sunxi_hci->hsic_flag = 0;
	}

	if(sunxi_hci->hsic_flag){
		/* usb hisc ctrl */
		type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_hsic_ctrl", &item_temp);
		if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
			sunxi_hci->hsic_ctrl_flag = item_temp.val;
		}else{
			DMSG_INFO("get usb_restrict_flag failed\n");
			sunxi_hci->hsic_ctrl_flag = 0;
		}

		/* usbc host hisc rdy*/
		type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_hsic_rdy_gpio", &sunxi_hci->usb_host_hsic_rdy);
		if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
			sunxi_hci->usb_host_hsic_rdy_valid = 1;
		}else{
			sunxi_hci->usb_host_hsic_rdy_valid = 0;
		}
	}else{
		sunxi_hci->hsic_ctrl_flag = 0;
		sunxi_hci->usb_host_hsic_rdy_valid = 0;
	}

	/* get regulator io information */
	type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_regulator_io", &item_temp);
	if (type == SCIRPT_ITEM_VALUE_TYPE_STR) {
		if (!strcmp(item_temp.str, "nocare")) {
			DMSG_INFO("get usb_regulator is nocare\n");
			sunxi_hci->regulator_io = NULL;
		}else{
			sunxi_hci->regulator_io = item_temp.str;

			type = script_get_item(usbc_name[sunxi_hci->usbc_no], "usb_regulator_vol", &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				sunxi_hci->regulator_value = item_temp.val;
			}else{
				DMSG_INFO("get usb_voltage is failed\n");
				sunxi_hci->regulator_value = 0;
			}
		}
	}else {
		DMSG_INFO("get usb_regulator is failed\n");
		sunxi_hci->regulator_io = NULL;
	}

	sunxi_usb_3g_config(sunxi_hci);

	/* wifi_used */
	if(sunxi_hci->host_init_state == 0){
		u32 usb_wifi_used = 0;
		u32 usb_wifi_usbc_num  = 0;
		u32 usb_wifi_usbc_type = 0;

		type = script_get_item("wifi_para", "wifi_used", &item_temp);
		if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
			usb_wifi_used = item_temp.val;
		}else{
			DMSG_INFO("script_parser_fetch wifi_used failed\n");
			usb_wifi_used = 0;
		}

		if(usb_wifi_used){
			/* wifi_usbc_num */
			type = script_get_item("wifi_para", "wifi_usbc_id", &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				usb_wifi_usbc_num = item_temp.val;
			}else{
				DMSG_INFO("script_parser_fetch wifi_usbc_id failed\n");
				return 0;
			}

			/* wifi_usbc_type */
			type = script_get_item("wifi_para", "wifi_usbc_type", &item_temp);
			if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
				usb_wifi_usbc_type = item_temp.val;
			}else{
				DMSG_INFO("script_parser_fetch wifi_usbc_type failed\n");
				return 0;
			}

			/* only open the module used by wifi */
			if(sunxi_hci->usbc_no == usb_wifi_usbc_num){
				sunxi_hci->used = 0;
				if(sunxi_hci->usbc_type == usb_wifi_usbc_type){
					sunxi_hci->used = 1;
				}
			}
		}
	}
#else
	sunxi_hci->used = 1;
	#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
		sunxi_hci->host_init_state = 0;
	#else
		sunxi_hci->host_init_state = 1;
	#endif
#endif

	return 0;
}

#if !defined (CONFIG_ARCH_SUN9IW1) && !defined (CONFIG_ARCH_SUN8IW6) && !defined (CONFIG_ARCH_SUN8IW8) && !defined (CONFIG_ARCH_SUN8IW7)
static __u32 USBC_Phy_GetCsr(__u32 usbc_no)
{
	__u32 val = 0x0;

	switch(usbc_no){
	case 0:
		val = (u32 __force)SUNXI_USB_OTG_VBASE + USBPHYC_REG_o_PHYCTL;
		break;
	case 1:
		val = (u32 __force)SUNXI_USB_OTG_VBASE + USBPHYC_REG_o_PHYCTL;
		break;
	case 2:
		val = (u32 __force)SUNXI_USB_OTG_VBASE + USBPHYC_REG_o_PHYCTL;
		break;
	default:
		break;
	}

	return val;
}
static __u32 USBC_Phy_TpWrite(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	__u32 temp = 0, dtmp = 0;
	__u32 j=0;

	dtmp = data;
	for(j = 0; j < len; j++)
	{
		/* set  the bit address to be write */
		temp = USBC_Readl(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0xff << 8);
		temp |= ((addr + j) << 8);
		USBC_Writel(temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0x1 << 7);
		temp |= (dtmp & 0x1) << 7;
		temp &= ~(0x1 << (usbc_no << 1));
		USBC_Writeb(temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp |= (0x1 << (usbc_no << 1));
		USBC_Writeb( temp, USBC_Phy_GetCsr(usbc_no));

		temp = USBC_Readb(USBC_Phy_GetCsr(usbc_no));
		temp &= ~(0x1 << (usbc_no <<1 ));
		USBC_Writeb(temp, USBC_Phy_GetCsr(usbc_no));
		dtmp >>= 1;
	}

	return data;
}

static __u32 USBC_Phy_Write(__u32 usbc_no, __u32 addr, __u32 data, __u32 len)
{
	return USBC_Phy_TpWrite(usbc_no, addr, data, len);
}

static void UsbPhyInit(__u32 usbc_no)
{
//	DMSG_INFO("csr1: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Readl(USBC_Phy_GetCsr(usbc_no)));

	/* adjust the 45 ohm resistance */
	if(usbc_no == 0){
		USBC_Phy_Write(usbc_no, 0x0c, 0x01, 1);
	}

//	DMSG_INFO("csr2-0: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x0c, 1));

	/* adjust USB0 PHY's rate and range */
	USBC_Phy_Write(usbc_no, 0x20, 0x14, 5);

//	DMSG_INFO("csr2-1: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x20, 5));

	/* adjust disconnect threshold value */
	USBC_Phy_Write(usbc_no, 0x2a, 3, 2); /*by wangjx*/

//	DMSG_INFO("csr2: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Phy_Read(usbc_no, 0x2a, 2));
//	DMSG_INFO("csr3: usbc%d: 0x%x\n", usbc_no, (u32)USBC_Readl(USBC_Phy_GetCsr(usbc_no)));

	return;
}
#endif

#if defined (CONFIG_ARCH_SUN8IW7) || defined (CONFIG_ARCH_SUN8IW8)
static int usb_phy_csr_add(int index)
{
	int val = 0x0;

	switch(index)
	{
		case 0:
			val = (u32 __force)SUNXI_USB_OTG_VBASE + 0x410;
		break;

#ifdef  CONFIG_ARCH_SUN8IW7
		case 1:
			val = (u32 __force)SUNXI_USB_OTG_VBASE + 0x410;
		break;

		case 2:
			val = (u32 __force)SUNXI_USB_OTG_VBASE + 0x410;
		break;

		case 3:
			val = (u32 __force)SUNXI_USB_OTG_VBASE + 0x410;
		break;
#endif
		default:
			DMSG_PANIC("usb_phy_csr_add is fial in %d index\n", index);
		break;

	}

	return val;
}

int usb_phy_csr_read(int index)
{
	int val = 0x0;

	switch(index)
	{
		case 0:
			val = (u32 __force)SUNXI_USB_OTG_VBASE + 0x424;
		break;

#ifdef  CONFIG_ARCH_SUN8IW7
		case 1:
			val = (u32 __force)SUNXI_USB_HCI1_VBASE + 0x824;
		break;

		case 2:
			val = (u32 __force)SUNXI_USB_HCI2_VBASE + 0x824;
		break;

		case 3:
			val = (u32 __force)SUNXI_USB_HCI3_VBASE + 0x824;
		break;
#endif
		default:

			DMSG_PANIC("usb_phy_csr_write is fial in %d index\n", index);
		break;
	}

	return val;
}

static int usb_phy_csr_write(int index)
{
	int val = 0x0;

	switch(index)
	{
		case 0:
			val = (u32 __force)SUNXI_USB_OTG_VBASE + 0x410;
		break;
#ifdef  CONFIG_ARCH_SUN8IW7
		case 1:
			val = (u32 __force)SUNXI_USB_HCI1_VBASE + 0x810;
		break;

		case 2:
			val = (u32 __force)SUNXI_USB_HCI2_VBASE + 0x810;
		break;

		case 3:
			val = (u32 __force)SUNXI_USB_HCI3_VBASE + 0x810;
		break;
#endif
		default:
			DMSG_PANIC("usb_phy_csr_write is fial in %d index\n", index);
		break;
	}

	return val;
}

int usb_phyx_tp_write(int index, int addr, int data, int len)
{
	int temp = 0;
	int j = 0;
	int reg_value = 0;
	int reg_temp = 0;
	int dtmp = 0;

	reg_value = USBC_Readl((u32 __force)SUNXI_USB_OTG_VBASE + 0x420);
	reg_temp = reg_value;
	reg_value |= 0x01;
	USBC_Writel(reg_value, ((u32 __force)SUNXI_USB_OTG_VBASE + 0x420));

	dtmp = data;
	for(j=0; j < len; j++)
	{
		USBC_Writeb(addr+j, usb_phy_csr_add(index)+1);

		temp = USBC_Readb(usb_phy_csr_write(index));
		temp &= ~(0x1<<0);
		USBC_Writeb(temp, usb_phy_csr_write(index));

		temp = USBC_Readb(usb_phy_csr_add(index));
		temp &= ~(0x1<<7);
		temp |= (dtmp&0x1)<<7;
		USBC_Writeb(temp, usb_phy_csr_add(index));

		temp |= (0x1<<0);
		USBC_Writeb(temp, usb_phy_csr_write(index));

		temp &= ~(0x1<<0);
		USBC_Writeb(temp, usb_phy_csr_write(index));

		dtmp >>= 1;
	}

	USBC_Writel(reg_temp, ((u32 __force)SUNXI_USB_OTG_VBASE + 0x420));

	return 0;
}

int usb_phyx_tp_read(int index, int addr, int len)
{
	int temp = 0;
	int i=0;
	int j=0;
	int ret = 0;
	int reg_value = 0;
	int reg_temp = 0;

	reg_value = USBC_Readl((u32 __force)SUNXI_USB_OTG_VBASE + 0x420);
	reg_temp = reg_value;
	reg_value |= 0x01;
	USBC_Writel(reg_value, ((u32 __force)SUNXI_USB_OTG_VBASE + 0x420));

	for(j=len; j>0; j--)
	{
		USBC_Writeb(addr+j-1, usb_phy_csr_add(index)+1);

		for(i=0;i<0x4;i++);

		temp = USBC_Readb(usb_phy_csr_read(index));
		ret <<= 1;
		ret |= (temp & 0x1);
	}

	USBC_Writel(reg_temp, ((u32 __force)SUNXI_USB_OTG_VBASE + 0x420));

	return ret;
}
#endif

#ifndef  SUNXI_USB_FPGA
static s32 clock_init(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
#ifdef CONFIG_ARCH_SUN9IW1
	return 0;
#else
	if(ohci){  /* ohci */
		sunxi_hci->ahb = clk_get(NULL, usbc_ahb_ohci_name[sunxi_hci->usbc_no]);
		if (IS_ERR(sunxi_hci->ahb)){
			DMSG_PANIC("ERR: get ohci%d abh clk failed.\n", (sunxi_hci->usbc_no - 1));
			goto failed;
		}
	}else{  /* ehci */
		sunxi_hci->ahb = clk_get(NULL, usbc_ahb_ehci_name[sunxi_hci->usbc_no]);
		if (IS_ERR(sunxi_hci->ahb)){
			DMSG_PANIC("ERR: get ehci%d abh clk failed.\n", (sunxi_hci->usbc_no - 1));
			goto failed;
		}
	}

	sunxi_hci->mod_usbphy = clk_get(NULL, usbc_phy_name[sunxi_hci->usbc_no]);
	if (IS_ERR(sunxi_hci->mod_usbphy)){
		DMSG_PANIC("ERR: get usb%d mod_usbphy failed.\n", sunxi_hci->usbc_no);
		goto failed;
	}

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW3) || defined (CONFIG_ARCH_SUN8IW9) || defined (CONFIG_ARCH_SUN8IW6)
	if(sunxi_hci->hsic_flag){
		sunxi_hci->hsic_usbphy = clk_get(NULL, usbc_hsic_phy_name[sunxi_hci->usbc_no]);
		if (IS_ERR(sunxi_hci->hsic_usbphy)){
			DMSG_PANIC("ERR: get usb%d hsic_usbphy failed.\n", sunxi_hci->usbc_no);
			goto failed;
		}
	}
#endif

	return 0;

failed:
	if(IS_ERR(sunxi_hci->ahb)){
		//clk_put(sunxi_hci->ahb);
		sunxi_hci->ahb = NULL;
	}

	if(IS_ERR(sunxi_hci->mod_usbphy)){
		//clk_put(sunxi_hci->mod_usbphy);
		sunxi_hci->mod_usbphy = NULL;
	}

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW3) || defined (CONFIG_ARCH_SUN8IW9) || defined (CONFIG_ARCH_SUN8IW6)
	if(sunxi_hci->hsic_flag){
		if(IS_ERR(sunxi_hci->hsic_usbphy)){
			sunxi_hci->hsic_usbphy = NULL;
		}
	}

#endif
	return -1;
#endif
}

static s32 clock_exit(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
#ifdef CONFIG_ARCH_SUN9IW1
	return 0;
#else
	if(!IS_ERR(sunxi_hci->ahb)){
		clk_put(sunxi_hci->ahb);
		sunxi_hci->ahb = NULL;
	}

	if(!IS_ERR(sunxi_hci->mod_usbphy)){
		clk_put(sunxi_hci->mod_usbphy);
		sunxi_hci->mod_usbphy = NULL;
	}

#if defined (CONFIG_ARCH_SUN8IW5) || defined (CONFIG_ARCH_SUN8IW3) || defined (CONFIG_ARCH_SUN8IW9) || defined (CONFIG_ARCH_SUN8IW6)
	if(sunxi_hci->hsic_flag){
		if(!IS_ERR(sunxi_hci->hsic_usbphy)){
			clk_put(sunxi_hci->hsic_usbphy);
			sunxi_hci->hsic_usbphy = NULL;
		}
	}
#endif
	return 0;
#endif

}

static int open_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	DMSG_INFO("[%s]: open clock, is_open: %d\n", sunxi_hci->hci_name, sunxi_hci->clk_is_open);
#ifdef CONFIG_ARCH_SUN9IW1

	#ifdef CONFIG_USB_SUNXI_HSIC
		if(vbat_hsic_hdle){
			if(regulator_enable(vbat_hsic_hdle) < 0){
				DMSG_INFO("ERR: vbat_hsic: regulator_enable fail\n");
				return 0;
			}
		}
	#else
		if(vbat_usbh_hdle){
			if(regulator_enable(vbat_usbh_hdle) < 0){
				DMSG_INFO("ERR: vbat_usbh: regulator_enable fail\n");
				return 0;
			}
		}
	#endif

	if(usb_hci_clock_cnt == 0 && (hci_ahb_gate != NULL)){
		DMSG_INFO("clk_prepare_enable: hci_ahb_gate\n");
		if(clk_prepare_enable(hci_ahb_gate)){
			DMSG_PANIC("ERR:try to prepare_enable %s_ahb failed!\n", sunxi_hci->hci_name);
		}
	}
	usb_hci_clock_cnt ++;

	if(!sunxi_hci->clk_is_open){
		sunxi_hci->clk_is_open = 1;
		hci_clock_ctrl(sunxi_hci, ohci, 1);
	}

#else

	#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
	{
		int reg_value = 0;

		/* adjust disconnect threshold value */
		usb_phyx_tp_write(sunxi_hci->usbc_no, 0x2a, 3, 2);

		if(sunxi_hci->usbc_no == HCI0_USBC_NO){
			reg_value = USBC_Readl((u32 __force)SUNXI_USB_OTG_VBASE + 0x420);
			reg_value &= ~(0x01);
			USBC_Writel(reg_value, ((u32 __force)SUNXI_USB_OTG_VBASE + 0x420));

		}

		reg_value = USBC_Readl(sunxi_hci->usb_vbase + 0x810);
		reg_value &= ~(0x01<<1);
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + 0x810));

		//UsbPhyInit(sunxi_hci->usbc_no);
	}
	#else
	{
#ifndef  CONFIG_ARCH_SUN8IW6
		UsbPhyInit(sunxi_hci->usbc_no);
#endif
	}
	#endif

	if(sunxi_hci->ahb && sunxi_hci->mod_usbphy && !sunxi_hci->clk_is_open){
		sunxi_hci->clk_is_open = 1;
		if(clk_prepare_enable(sunxi_hci->ahb)){
			DMSG_PANIC("ERR:try to prepare_enable %s_ahb failed!\n", sunxi_hci->hci_name);
		}
		mdelay(10);

		if(sunxi_hci->hsic_flag)
		{

			if(sunxi_hci->hsic_ctrl_flag){
				if(sunxi_hci->hsic_enable_flag){
					if(clk_prepare_enable(sunxi_hci->hsic_usbphy)){
						DMSG_PANIC("ERR:try to prepare_enable %s_hsic_usbphy failed!\n", sunxi_hci->hci_name);
					}
				}
			}else{
				if(clk_prepare_enable(sunxi_hci->hsic_usbphy)){
					DMSG_PANIC("ERR:try to prepare_enable %s_hsic_usbphy failed!\n", sunxi_hci->hci_name);
				}
			}
		}
		else{
			if(clk_prepare_enable(sunxi_hci->mod_usbphy)){
				DMSG_PANIC("ERR:try to prepare_enable %s_usbphy failed!\n", sunxi_hci->hci_name);
			}
			mdelay(10);
		}

	}else{
		DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
			sunxi_hci->hci_name,
			sunxi_hci->ahb, sunxi_hci->mod_usbphy, sunxi_hci->clk_is_open,
			sunxi_hci->mod_usb);
	}
#endif
	return 0;
}

static int close_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	DMSG_INFO("[%s]: close clock, is_open: %d\n", sunxi_hci->hci_name, sunxi_hci->clk_is_open);
#ifdef CONFIG_ARCH_SUN9IW1
	usb_hci_clock_cnt --;
	if(usb_hci_clock_cnt == 0 && (hci_ahb_gate != NULL)){
		DMSG_INFO("clk_disable_unprepare: hci_ahb_gate\n");
		clk_disable_unprepare(hci_ahb_gate);
	}

	if(sunxi_hci->clk_is_open){
		sunxi_hci->clk_is_open = 0;
		hci_clock_ctrl(sunxi_hci, ohci, 0);
	}

	#ifdef CONFIG_USB_SUNXI_HSIC
		if(vbat_hsic_hdle){
			if(regulator_disable(vbat_hsic_hdle) < 0){
				DMSG_INFO("ERR: vbat_hsic: regulator_disable fail\n");
				return 0;
			}
		}
	#else
		if(vbat_usbh_hdle){
			if(regulator_disable(vbat_usbh_hdle) < 0){
				DMSG_INFO("ERR: vbat_usbh: regulator_disable fail\n");
				return 0;
			}
		}
	#endif

	return 0;
#else

	if(sunxi_hci->ahb && sunxi_hci->mod_usbphy && sunxi_hci->clk_is_open){
		sunxi_hci->clk_is_open = 0;

		if(sunxi_hci->hsic_flag){

			if(sunxi_hci->hsic_ctrl_flag){
				if(sunxi_hci->hsic_enable_flag){
					clk_disable_unprepare(sunxi_hci->hsic_usbphy);
				}
			}else{
				clk_disable_unprepare(sunxi_hci->hsic_usbphy);
			}
		}else{
			clk_disable_unprepare(sunxi_hci->mod_usbphy);
		}
		clk_disable_unprepare(sunxi_hci->ahb);
		mdelay(10);
	}else{
		DMSG_PANIC("[%s]: wrn: open clock failed, (0x%p, 0x%p, %d, 0x%p)\n",
				sunxi_hci->hci_name,sunxi_hci->ahb,
				sunxi_hci->mod_usbphy, sunxi_hci->clk_is_open,
				sunxi_hci->mod_usb);
	}
	return 0;
#endif
}
#else
static s32 clock_init(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	return 0;
}

static s32 clock_exit(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	return 0;
}

static int open_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
#ifndef CONFIG_ARCH_SUN9IW1
	u32 reg_value = 0;
	u32 ccmu_base = (u32 __force)SUNXI_CCM_VBASE;

	DMSG_INFO("[%s]: open clock\n", sunxi_hci->hci_name);
#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
	reg_value = USBC_Readl((u32 __force)SUNXI_USB_OTG_VBASE + 0x420);
	reg_value &= ~(0x01);
	USBC_Writel(reg_value, ((u32 __force)SUNXI_USB_OTG_VBASE + 0x420));

	reg_value = USBC_Readl(sunxi_hci->usb_vbase + 0x810);
	reg_value &= ~(0x01<<1);
	USBC_Writel(reg_value, (sunxi_hci->usb_vbase + 0x810));

	UsbPhyInit(0);
#endif
	//Gating AHB clock for USB_phy1
	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value |= (1 << 26);			        /* AHB clock gate ehci */
	reg_value |= (1 << 29); 			/* AHB clock gate ohci */
	reg_value |= (1 << 24);	           		/* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

	reg_value = USBC_Readl(ccmu_base + 0x2c0);
	reg_value |= (1 << 26);			        /* ehci reset control, de-assert */
	reg_value |= (1 << 29); 			/* ohci reset control, de-assert */
	reg_value |= (1 << 24);	                        /* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x2c0));

	reg_value = USBC_Readl(ccmu_base + 0x100);
	reg_value |= (1 << 18);
	reg_value |= (1 << 17);
	USBC_Writel(reg_value, (ccmu_base + 0x100));

	//delay to wati SIE stable
	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy1
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	reg_value |= (1 << 16); 			/* gating specal clock for ohci */
	reg_value |= (1 << 9);				/* gating specal clock for usb phy1(ehci0,ohci0) */
	reg_value |= (1 << 8);				/* gating specal clock for usb phy0(otg) */
	reg_value |= (1 << 1);				/* usb phy1 reset */
	reg_value |= (1 << 0);				/* usb phy0 reset */

	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	//delay some time
	reg_value = 10000;
	while(reg_value--);

	UsbPhyInit(sunxi_hci->usbc_no);

	DMSG_INFO("open_clock 0x60(0x%x), 0xcc(0x%x),0x2c0(0x%x)\n",
		(u32)USBC_Readl(ccmu_base + 0x60),
		(u32)USBC_Readl(ccmu_base + 0xcc),
		(u32)USBC_Readl(ccmu_base + 0x2c0));
	return 0;
#else
	u32 reg_value = 0;
	u32 i = 0;
	u32 ccmu_base = (u32)SUNXI_CCM_VBASE;

	DMSG_INFO("[%s]: open clock\n", sunxi_hci->hci_name);
	usb_hci_clock_cnt ++ ;

	//AHB ALL clock gate
	reg_value = USBC_Readl(ccmu_base + 0x184);
	reg_value |= (1 << 1); 			/* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x184));

	reg_value = USBC_Readl(ccmu_base + 0x1A4);
	reg_value |= (1 << 0); 			/* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x1A4));

	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + 0x04);
	reg_value &= ~(0x1f<<1);
	reg_value &= ~(0x01<<10);
	reg_value &= ~(0x01<<12);
	reg_value &= ~(0x1f<<17);
	reg_value |= (0x01<<3);
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + 0x04);
	for(i=0; i<0x10; i++);
	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + 0x04);
	reg_value |= (0x01<<19);
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + 0x04);

	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE);
	reg_value &= ~(0xf<<1);
	reg_value &= ~(0xf<<17);
	reg_value |= (0x1<<3);
	reg_value |= (0x1<<4);
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE);
	for(i=0; i<0x10; i++);
	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE);
	reg_value |= (0x01<<18);
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE);

	DMSG_INFO("open_clock 0x184(0x%x), 0x1A4(0x%x),0x%x(0x%x),0x%x(0x%x)\n",
		(u32)USBC_Readl(ccmu_base + 0x184),
		(u32)USBC_Readl(ccmu_base + 0x1A4),
		(u32)(SUNXI_USB_CTRL_VBASE + 0x04),(u32)USBC_Readl(SUNXI_USB_CTRL_VBASE + 0x04),
		(u32)SUNXI_USB_CTRL_VBASE, (u32)USBC_Readl(SUNXI_USB_CTRL_VBASE));

	return 0;

#endif
}

static int close_clock(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
#ifndef CONFIG_ARCH_SUN9IW1
	u32 reg_value = 0;
	u32 ccmu_base = (u32 __force)SUNXI_CCM_VBASE;

	DMSG_INFO("[%s]: close clock\n", sunxi_hci->hci_name);

	//Gating AHB clock for USB_phy1
	reg_value = USBC_Readl(ccmu_base + 0x60);
	reg_value &= ~(1 << 29);			/* close AHB for ohci */
	reg_value &= ~(1 << 26);			/* close AHB for ehci */
	USBC_Writel(reg_value, (ccmu_base + 0x60));

	reg_value = USBC_Readl(ccmu_base + 0x2c0);
	reg_value &= ~(1 << 29);			/* ohci reset control, assert */
	reg_value &= ~(1 << 26);			/* ehci reset control, assert */
	USBC_Writel(reg_value, (ccmu_base + 0x2c0));

	//delay to wati SIE stable
	reg_value = 10000;
	while(reg_value--);

	//Enable module clock for USB phy1
	reg_value = USBC_Readl(ccmu_base + 0xcc);
	reg_value &= ~(1 << 16);			/* close specal clock for ohci */
	reg_value &= ~(1 << 9);				/* close specal clock for usb phy1(ehci0,ohci0) */
	reg_value &= ~(1 << 8);				/* close specal clock for usb phy0(otg) */
	reg_value &= ~(1 << 1);				/* usb phy1 reset */
	reg_value &= ~(1 << 0);				/* usb phy0 reset */
	USBC_Writel(reg_value, (ccmu_base + 0xcc));

	return 0;
#else
	u32 reg_value = 0;
	u32 ccmu_base = (u32)SUNXI_CCM_VBASE;

	DMSG_INFO("[%s]: close clock\n", sunxi_hci->hci_name);
	usb_hci_clock_cnt --;
	//AHB ALL clock gate
	reg_value = USBC_Readl(ccmu_base + 0x184);
	reg_value &= ~(1 << 1);			/* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x184));

	reg_value = USBC_Readl(ccmu_base + 0x1A4);
	reg_value &= ~(1 << 0);			/* AHB clock gate usb0 */
	USBC_Writel(reg_value, (ccmu_base + 0x1A4));

	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE + 0x04);

	reg_value &= ~(0x01<<3);
	reg_value &= ~(0x01<<19);
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE + 0x04);

	reg_value = USBC_Readl(SUNXI_USB_CTRL_VBASE);
	reg_value &= ~(0x1<<3);
	reg_value &= ~(0x1<<4);
	reg_value &= ~(0x01<<18);
	USBC_Writel(reg_value, SUNXI_USB_CTRL_VBASE);
	return 0;
#endif
}
#endif

static void Hci_Phy_Set_Ctl(void __iomem *regs, __u32 mask)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readl(regs + SUNXI_HCI_PHY_CTRL);
	reg_val |= (0x01 << mask);
	USBC_Writel(reg_val, (regs + SUNXI_HCI_PHY_CTRL));

	return;
}

static void Hci_Phy_Clear_Ctl(void __iomem *regs, __u32 mask)
{
	__u32 reg_val = 0;

	reg_val = USBC_Readl(regs + SUNXI_HCI_PHY_CTRL);
	reg_val &= ~(0x01 << mask);
	USBC_Writel(reg_val, (regs + SUNXI_HCI_PHY_CTRL));

	return;
}

static void hci_phy_ctrl(struct sunxi_hci_hcd *sunxi_hci, u32 enable)
{
	spinlock_t lock;
	unsigned long flags = 0;

	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);

	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		if(enable && usb1_enable_phy_cnt == 0){
			Hci_Phy_Clear_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}else if(!enable && usb1_enable_phy_cnt == 1){
			Hci_Phy_Set_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}

		if(enable){
			usb1_enable_phy_cnt++;
		}else{
			usb1_enable_phy_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI1_USBC_NO){
		if(enable && usb2_enable_phy_cnt == 0){
			Hci_Phy_Clear_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}else if(!enable && usb2_enable_phy_cnt == 1){
			Hci_Phy_Set_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}

		if(enable){
			usb2_enable_phy_cnt++;
		}else{
			usb2_enable_phy_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI2_USBC_NO){
		if(enable && usb3_enable_phy_cnt == 0){
			Hci_Phy_Clear_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}else if(!enable && usb3_enable_phy_cnt == 1){
			Hci_Phy_Set_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}

		if(enable){
			usb3_enable_phy_cnt++;
		}else{
			usb3_enable_phy_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI3_USBC_NO){
		if(enable && usb4_enable_phy_cnt == 0){
			Hci_Phy_Clear_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}else if(!enable && usb4_enable_phy_cnt == 1){
			Hci_Phy_Set_Ctl(sunxi_hci->usb_vbase, SUNXI_HCI_PHY_CTRL_DISENABLE);
		}

		if(enable){
			usb4_enable_phy_cnt++;
		}else{
			usb4_enable_phy_cnt--;
		}
	}else{
		DMSG_PANIC("EER: unkown usbc_no(%d)\n", sunxi_hci->usbc_no);
		spin_unlock_irqrestore(&lock, flags);
		return;
	}

	spin_unlock_irqrestore(&lock, flags);

	return;
}

static int usb_get_hsic_phy_ctrl(int value, int enable)
{
	if(enable){
		value |= (0x07<<8);
		value |= (0x01<<1);
		value |= (0x01<<0);
		value |= (0x01<<16);
		value |= (0x01<<20);
	}else{
		value &= ~(0x07<<8);
		value &= ~(0x01<<1);
		value &= ~(0x01<<0);
		value &= ~(0x01<<16);
		value &= ~(0x01<<20);
	}

	return value;
}

static void usb_passby(struct sunxi_hci_hcd *sunxi_hci, u32 enable)
{
	unsigned long reg_value = 0;
	spinlock_t lock;
	unsigned long flags = 0;

	spin_lock_init(&lock);
	spin_lock_irqsave(&lock, flags);

	/*enable passby*/
	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
		if(enable && usb1_enable_passly_cnt == 0){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
			}else{
#ifdef CONFIG_USB_SUSPEND
				reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
#endif
				reg_value |= (1 << 9);		/* AHB Master interface burst type INCR4 enable */
				reg_value |= (1 << 8);		/* AHB Master interface INCRX align enable */
#ifdef SUNXI_USB_FPGA
				reg_value |= (0 << 0);		/* enable ULPI, disable UTMI */
#else
				reg_value |= (1 << 0);		/* enable UTMI, disable ULPI */
#endif
			}
		}else if(!enable && usb1_enable_passly_cnt == 1){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
			}else{
				reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
				reg_value &= ~(1 << 9);		/* AHB Master interface burst type INCR4 disable */
				reg_value &= ~(1 << 8);		/* AHB Master interface INCRX align disable */
				reg_value &= ~(1 << 0);		/* ULPI bypass disable */
			}
		}
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

		if(enable){
			usb1_enable_passly_cnt++;
#ifdef CONFIG_USB_HCD_ENHANCE
			if(usb1_enable_passly_cnt == 2){
				atomic_set(&hci1_thread_scan_flag, 1);
			}
#endif
		}else{
			usb1_enable_passly_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI1_USBC_NO){
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
		if(enable && usb2_enable_passly_cnt == 0){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
			}else{
#ifdef CONFIG_USB_SUSPEND
				reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
#endif
				reg_value |= (1 << 9);		/* AHB Master interface burst type INCR4 enable */
				reg_value |= (1 << 8);		/* AHB Master interface INCRX align enable */
				reg_value |= (1 << 0);		/* ULPI bypass enable */
			}
		}else if(!enable && usb2_enable_passly_cnt == 1){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);

			}else{
				reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
				reg_value &= ~(1 << 9);		/* AHB Master interface burst type INCR4 disable */
				reg_value &= ~(1 << 8);		/* AHB Master interface INCRX align disable */
				reg_value &= ~(1 << 0);		/* ULPI bypass disable */
			}
		}
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

		if(enable){
			usb2_enable_passly_cnt++;
		}else{
			usb2_enable_passly_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI2_USBC_NO){
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
		if(enable && usb3_enable_passly_cnt == 0){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);

			}else{
#ifdef CONFIG_USB_SUSPEND
				reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
#endif
				reg_value |= (1 << 9);		/* AHB Master interface burst type INCR4 enable */
				reg_value |= (1 << 8);		/* AHB Master interface INCRX align enable */
				reg_value |= (1 << 0);		/* ULPI bypass enable */
			}
		}else if(!enable && usb3_enable_passly_cnt == 1){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
			}else{
				reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
				reg_value &= ~(1 << 9);		/* AHB Master interface burst type INCR4 disable */
				reg_value &= ~(1 << 8);		/* AHB Master interface INCRX align disable */
				reg_value &= ~(1 << 0);		/* ULPI bypass disable */
			}
		}
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

		if(enable){
			usb3_enable_passly_cnt++;
#ifdef CONFIG_USB_HCD_ENHANCE
			if(usb3_enable_passly_cnt == 2){
				atomic_set(&hci3_thread_scan_flag, 1);
			}
#endif
		}else{
			usb3_enable_passly_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI3_USBC_NO){
		reg_value = USBC_Readl(sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE);
		if(enable && usb4_enable_passly_cnt == 0){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
			}else{
#ifdef CONFIG_USB_SUSPEND
				reg_value |= (1 << 10);		/* AHB Master interface INCR8 enable */
#endif
				reg_value |= (1 << 9);		/* AHB Master interface burst type INCR4 enable */
				reg_value |= (1 << 8);		/* AHB Master interface INCRX align enable */
				reg_value |= (1 << 0);		/* ULPI bypass enable */
			}
		}else if(!enable && usb4_enable_passly_cnt == 1){
			if(sunxi_hci->hsic_flag){
				reg_value = usb_get_hsic_phy_ctrl(reg_value, enable);
			}else{
				reg_value &= ~(1 << 10);	/* AHB Master interface INCR8 disable */
				reg_value &= ~(1 << 9);		/* AHB Master interface burst type INCR4 disable */
				reg_value &= ~(1 << 8);		/* AHB Master interface INCRX align disable */
				reg_value &= ~(1 << 0);		/* ULPI bypass disable */
			}
		}
		USBC_Writel(reg_value, (sunxi_hci->usb_vbase + SUNXI_USB_PMU_IRQ_ENABLE));

		if(enable){
			usb4_enable_passly_cnt++;
		}else{
			usb4_enable_passly_cnt--;
		}
	}else{
		DMSG_PANIC("EER: unkown usbc_no(%d)\n", sunxi_hci->usbc_no);
		spin_unlock_irqrestore(&lock, flags);
		return;
	}

	spin_unlock_irqrestore(&lock, flags);

	return;
}

static void hci_port_configure(struct sunxi_hci_hcd *sunxi_hci, u32 enable)
{
}

#ifndef  SUNXI_USB_FPGA

static int alloc_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	u32 ret = 1;

	if(sunxi_hci->drv_vbus_gpio_valid){
		ret = gpio_request(sunxi_hci->drv_vbus_gpio_set.gpio.gpio, NULL);
		if(ret != 0){
			DMSG_PANIC("ERR: gpio_request failed\n");
			sunxi_hci->drv_vbus_gpio_valid = 0;
		}else{
			gpio_direction_output(sunxi_hci->drv_vbus_gpio_set.gpio.gpio, 0);
		}
	}

	if(sunxi_hci->usb_restrict_valid){
		ret = gpio_request(sunxi_hci->restrict_gpio_set.gpio.gpio, NULL);
		if(ret != 0){
			DMSG_PANIC("ERR: gpio_request failed\n");
			sunxi_hci->usb_restrict_valid = 0;
		}else{
			gpio_direction_output(sunxi_hci->restrict_gpio_set.gpio.gpio, 0);
		}
	}
	if(sunxi_hci->usb_restrict_valid){
		if(sunxi_hci->usb_restrict_flag){
			__gpio_set_value(sunxi_hci->restrict_gpio_set.gpio.gpio, 0);
		}else{
			__gpio_set_value(sunxi_hci->restrict_gpio_set.gpio.gpio, 1);
		}
	}

	if(sunxi_hci->usb_host_hsic_rdy_valid){
		ret = gpio_request(sunxi_hci->usb_host_hsic_rdy.gpio.gpio, NULL);
		if(ret != 0){
			DMSG_PANIC("ERR: gpio_request failed\n");
			sunxi_hci->usb_host_hsic_rdy_valid = 0;
		}else{
			gpio_direction_output(sunxi_hci->usb_host_hsic_rdy.gpio.gpio, 0);
		}
	}
	return 0;
}

static void free_pin(struct sunxi_hci_hcd *sunxi_hci)
{
	if(sunxi_hci->drv_vbus_gpio_valid){
		gpio_free(sunxi_hci->drv_vbus_gpio_set.gpio.gpio);
		sunxi_hci->drv_vbus_gpio_valid = 0;
	}

	if(sunxi_hci->usb_restrict_valid){
		gpio_free(sunxi_hci->restrict_gpio_set.gpio.gpio);
		sunxi_hci->drv_vbus_gpio_valid = 0;
	}

	if(sunxi_hci->usb_host_hsic_rdy_valid){
		gpio_free(sunxi_hci->usb_host_hsic_rdy.gpio.gpio);
		sunxi_hci->drv_vbus_gpio_valid = 0;
	}
	return;
}

static void __sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	//u32 on_off = 0;

	DMSG_INFO("[%s]: Set USB Power %s\n", sunxi_hci->hci_name, (is_on ? "ON" : "OFF"));

	/* set power flag */
	sunxi_hci->power_flag = is_on;

	/* set power */
	//if(sunxi_hci->drv_vbus_gpio_set.gpio.data == 0){
	//	on_off = is_on ? 1 : 0;
	//}else{
	//	on_off = is_on ? 0 : 1;
	//}

//no care of usb0 vbus when otg connect pc setup system without battery and to return
#ifdef CONFIG_USB_SUNXI_USB_MANAGER
#if defined (CONFIG_ARCH_SUN8IW8) || defined (CONFIG_ARCH_SUN8IW7)
	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		if(is_on){
			if(usb_otg_id_status() == 1){
				return;
			}
		}
	}
#endif
#endif

	if(sunxi_hci->drv_vbus_gpio_valid){
		__gpio_set_value(sunxi_hci->drv_vbus_gpio_set.gpio.gpio, is_on);
	}

	return;
}

#else

static int alloc_pin(struct sunxi_hci_hcd *sunxi_ehci)
{
	return 0;
}

static void free_pin(struct sunxi_hci_hcd *sunxi_ehci)
{
	return;
}

static void __sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	return;
}

#endif

static void sunxi_set_usbc_regulator(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	if((sunxi_hci->regulator_io != NULL) && (sunxi_hci->regulator_io_hdle != NULL)){
		if(is_on){
			if(regulator_enable(sunxi_hci->regulator_io_hdle) < 0){
				DMSG_INFO("%s: regulator_enable fail\n", sunxi_hci->hci_name);
			}
		}else{
			if(regulator_disable(sunxi_hci->regulator_io_hdle) < 0){
				DMSG_INFO("%s: regulator_disable fail\n", sunxi_hci->hci_name);
			}
		}
	}

	return;
}

static void sunxi_set_vbus(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	DMSG_DEBUG("[%s]: sunxi_set_vbus cnt %d\n",
		sunxi_hci->hci_name,
		(sunxi_hci->usbc_no == 1) ? usb1_set_vbus_cnt : usb2_set_vbus_cnt);

	if(sunxi_hci->usbc_no == HCI0_USBC_NO){
		if(is_on && usb1_set_vbus_cnt == 0){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		}else if(!is_on && usb1_set_vbus_cnt == 1){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
		}

		if(is_on){
			usb1_set_vbus_cnt++;
#ifdef CONFIG_USB_HCD_ENHANCE
			if(usb1_set_vbus_cnt == 2){
				hci1_thread_run_flag = 1;
			}
#endif
		}else{
			usb1_set_vbus_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI1_USBC_NO){
		if(is_on && usb2_set_vbus_cnt == 0){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		}else if(!is_on && usb2_set_vbus_cnt == 1){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
		}

		if(is_on){
			usb2_set_vbus_cnt++;
		}else{
			usb2_set_vbus_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI2_USBC_NO){
		if(is_on && usb3_set_vbus_cnt == 0){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		}else if(!is_on && usb3_set_vbus_cnt == 1){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
		}

		if(is_on){
			usb3_set_vbus_cnt++;
		}else{
			usb3_set_vbus_cnt--;
		}
	}else if(sunxi_hci->usbc_no == HCI3_USBC_NO){
		if(is_on && usb4_set_vbus_cnt == 0){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power on */
		}else if(!is_on && usb4_set_vbus_cnt == 1){
			__sunxi_set_vbus(sunxi_hci, is_on);  /* power off */
		}

		if(is_on){
			usb4_set_vbus_cnt++;
		}else{
			usb4_set_vbus_cnt--;
		}
	}else{
		DMSG_INFO("[%s]: sunxi_set_vbus no: %d\n", sunxi_hci->hci_name, sunxi_hci->usbc_no);
	}

	return;
}

void sunxi_set_host_hisc_rdy(struct sunxi_hci_hcd *sunxi_hci, int is_on)
{
	if (sunxi_hci->usb_host_hsic_rdy_valid) {
		/* set config, output */
		gpio_direction_output(sunxi_hci->usb_host_hsic_rdy.gpio.gpio, is_on);
	}
}
//---------------------------------------------------------------
//  EHCI
//---------------------------------------------------------------

#define  SUNXI_EHCI_NAME		"sunxi-ehci"
static const char ehci_name[] = SUNXI_EHCI_NAME;

static struct sunxi_hci_hcd sunxi_ehci0;
static struct sunxi_hci_hcd sunxi_ehci1;
static struct sunxi_hci_hcd sunxi_ehci2;
static struct sunxi_hci_hcd sunxi_ehci3;

static u64 sunxi_ehci_dmamask = DMA_BIT_MASK(32);

static struct platform_device sunxi_usb_ehci_device[] = {
	[0] = {
		.name		= ehci_name,
		.id		= 1,
		.dev 		= {
			.dma_mask		= &sunxi_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ehci0,
		},
	},

	[1] = {
		.name		= ehci_name,
		.id		= 2,
		.dev 		= {
			.dma_mask		= &sunxi_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ehci1,
		},
	},

	[2] = {
		.name		= ehci_name,
		.id		= 3,
		.dev 		= {
			.dma_mask		= &sunxi_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ehci2,
		},
	},

	[3] = {
		.name		= ehci_name,
		.id		= 4,
		.dev 		= {
			.dma_mask		= &sunxi_ehci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ehci3,
		},
	},
};

//---------------------------------------------------------------
//  OHCI
//---------------------------------------------------------------
#define  SUNXI_OHCI_NAME		"sunxi-ohci"
static const char ohci_name[] = SUNXI_OHCI_NAME;

#if defined(CONFIG_USB_SUNXI_OHCI0) || defined(CONFIG_USB_SUNXI_OHCI1) || defined(CONFIG_USB_SUNXI_OHCI2) || defined(CONFIG_USB_SUNXI_OHCI3)/* kill warning */
static struct sunxi_hci_hcd sunxi_ohci0;
static struct sunxi_hci_hcd sunxi_ohci1;
static struct sunxi_hci_hcd sunxi_ohci2;
static struct sunxi_hci_hcd sunxi_ohci3;


static u64 sunxi_ohci_dmamask = DMA_BIT_MASK(32);

static struct platform_device sunxi_usb_ohci_device[] = {
	[0] = {
		.name		= ohci_name,
		.id		= 1,
		.dev 		= {
			.dma_mask		= &sunxi_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ohci0,
		},
	},

	[1] = {
		.name		= ohci_name,
		.id		= 2,
		.dev 		= {
			.dma_mask		= &sunxi_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ohci1,
		},
	},

	[2] = {
		.name		= ohci_name,
		.id		= 3,
		.dev 		= {
			.dma_mask		= &sunxi_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ohci2,
		},
	},

	[3] = {
		.name		= ohci_name,
		.id		= 4,
		.dev 		= {
			.dma_mask		= &sunxi_ohci_dmamask,
			.coherent_dma_mask	= DMA_BIT_MASK(32),
			.platform_data		= &sunxi_ohci3,
		},
	},
};
#endif

#ifdef CONFIG_ARCH_SUN8IW8
static void sunxi_usbc_work(struct work_struct *data)
{
	struct sunxi_hci_hcd *sunxi_hci  = NULL;

	sunxi_hci = &sunxi_ehci0;

	printk("sunxi_usbc_work, usbc:%d\n", sunxi_hci->usbc_no);

	__sunxi_set_vbus(sunxi_hci, 0);
	msleep(100);
	__sunxi_set_vbus(sunxi_hci, 1);

	printk("end sunxi_usbc_work\n");

}
#endif

static int init_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci, u32 usbc_no, u32 ohci, const char *hci_name)
{
	s32 ret = 0;

	memset(sunxi_hci, 0, sizeof(struct sunxi_hci_hcd));

	sunxi_hci->usbc_no = usbc_no;
	sunxi_hci->usbc_type = ohci ? SUNXI_USB_OHCI : SUNXI_USB_EHCI;

	if(ohci){
		sunxi_hci->irq_no = ohci_irq_no[sunxi_hci->usbc_no];
	}else{
		sunxi_hci->irq_no = ehci_irq_no[sunxi_hci->usbc_no];
	}

	sprintf(sunxi_hci->hci_name, "%s%d", hci_name, sunxi_hci->usbc_no);

	sunxi_hci->usb_vbase	= (void __iomem	*)usbc_base[sunxi_hci->usbc_no];
#ifdef  SUNXI_USB_FPGA
	sunxi_hci->sram_vbase	= (void __iomem	*)SUNXI_SRAMCTRL_VBASE;
#endif

	get_usb_cfg(sunxi_hci);
	request_usb_regulator_io(sunxi_hci);
	sunxi_hci->open_clock	= open_clock;
	sunxi_hci->close_clock	= close_clock;
	sunxi_hci->set_power	= sunxi_set_vbus;
	sunxi_hci->set_usbc_regulator = sunxi_set_usbc_regulator;
	sunxi_hci->usb_passby	= usb_passby;
	sunxi_hci->hci_phy_ctrl	= hci_phy_ctrl;
	sunxi_hci->port_configure = hci_port_configure;

#ifndef CONFIG_ARCH_SUN9IW1
#ifdef  SUNXI_USB_FPGA
	fpga_config_use_hci((__u32 __force)sunxi_hci->sram_vbase);
#endif
#endif

	ret = clock_init(sunxi_hci, ohci);
	if(ret != 0){
		DMSG_PANIC("ERR: clock_init failed\n");
		goto failed1;
	}

#ifdef CONFIG_USB_HCD_ENHANCE
	if(!ohci){
		if(usbc_no == 1){
			atomic_set(&hci1_thread_scan_flag, 0);
			hci1 = kthread_create(hci1_scan_thread, sunxi_hci, "hci1-hardware-scan");
			wake_up_process(hci1);
			hci1_thread_run_flag = 1;
		}else if(usbc_no == 2){
			atomic_set(&hci2_thread_scan_flag, 0);
			hci2 = kthread_create(hci2_scan_thread, sunxi_hci, "hci2-hardware-scan");
			wake_up_process(hci2);
			hci2_thread_run_flag = 0;
		}else if(usbc_no == 3){
			atomic_set(&hci3_thread_scan_flag, 0);
		    hci3 = kthread_create(hci3_scan_thread, sunxi_hci, "hci3-hardware-scan");
			wake_up_process(hci3);
			hci3_thread_run_flag = 1;
		}else{
			printk("err:usbc_no: %d\n", usbc_no);
		}
	}
#endif
	return 0;

failed1:
	return -1;
}

static int exit_sunxi_hci(struct sunxi_hci_hcd *sunxi_hci, u32 ohci)
{
	clock_exit(sunxi_hci, ohci);
	release_usb_regulator_io(sunxi_hci);
	return 0;
}

static int __init sunxi_hci_init(void)
{

#ifdef CONFIG_ARCH_SUN9IW1
	hci_ahb_gate = clk_get(NULL, USBHCI_CLK);
	if (IS_ERR(hci_ahb_gate)){
		DMSG_PANIC("ERR: OPEN hci_ahb_gate failed.\n");
	}

	sunxi_get_usbh_power();

#ifdef CONFIG_USB_SUNXI_HSIC
	if(vbat_hsic_name){
		vbat_hsic_hdle = regulator_get(NULL, vbat_hsic_name);
		if(IS_ERR(vbat_hsic_hdle)) {
			DMSG_PANIC("ERR: some error happen, vbat_hsic_hdle fail to get regulator!\n");
			return 0;
		}
	}else{
		vbat_hsic_hdle = NULL;
	}

	if(vbat_hsic_hdle){
		if(regulator_set_voltage(vbat_hsic_hdle , VBAT_HSIC_VALUE, VBAT_HSIC_VALUE) < 0 ){
			DMSG_PANIC("ERR: hsic_vbat regulator_set_voltage fail\n",);
			regulator_put(vbat_hsic_hdle);
			return 0;
		}
	}
#else
	if(vbat_usbh_name){
		vbat_usbh_hdle = regulator_get(NULL, vbat_usbh_name);
		if(IS_ERR(vbat_usbh_hdle)) {
			DMSG_PANIC("ERR: some error happen,vbat_usbh_hdle fail to get regulator!\n");
			return 0;
		}
	}else{
		vbat_usbh_hdle = NULL;
	}

	if(vbat_usbh_hdle){
		if( regulator_set_voltage(vbat_usbh_hdle , VBAT_USBH_VALUE, VBAT_USBH_VALUE) < 0 ){
			DMSG_PANIC("ERR: usbh_vbat regulator_set_voltage fail, return\n");
			regulator_put(vbat_usbh_hdle);
			return 0;
		}
	}
#endif

#endif

#ifdef  CONFIG_USB_SUNXI_EHCI0

	init_sunxi_hci(&sunxi_ehci0, HCI0_USBC_NO, 0, ehci_name);
	alloc_pin(&sunxi_ehci0);

	if(sunxi_ehci0.used){
		platform_device_register(&sunxi_usb_ehci_device[0]);
	}else{
		DMSG_INFO("ERR: usb%d %s is not enable\n", sunxi_ehci0.usbc_no, sunxi_ehci0.hci_name);
	}
#endif

	if(!sunxi_ehci0.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI0

		init_sunxi_hci(&sunxi_ohci0, HCI0_USBC_NO, 1, ohci_name);
#ifdef CONFIG_ARCH_SUN8IW8
		INIT_WORK(&sunxi_ohci0.usbc_work, sunxi_usbc_work);
#endif
#ifndef  CONFIG_USB_SUNXI_EHCI0
		alloc_pin(&sunxi_ohci0);
#endif

		if(sunxi_ohci0.used){
			platform_device_register(&sunxi_usb_ohci_device[0]);
		}else{
			DMSG_INFO("usb%d %s is not enable\n", sunxi_ohci0.usbc_no, sunxi_ohci0.hci_name);
		}
#endif
	}

#ifdef  CONFIG_USB_SUNXI_EHCI1

	init_sunxi_hci(&sunxi_ehci1, HCI1_USBC_NO, 0, ehci_name);
	alloc_pin(&sunxi_ehci1);

	if(sunxi_ehci1.used){
		platform_device_register(&sunxi_usb_ehci_device[1]);
	}else{
		DMSG_INFO("usb%d %s is not enable\n", sunxi_ehci1.usbc_no, sunxi_ehci1.hci_name);
	}
#endif

	if(!sunxi_ehci1.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI1

		init_sunxi_hci(&sunxi_ohci1, HCI1_USBC_NO, 1, ohci_name);

#ifndef  CONFIG_USB_SUNXI_EHCI1
		alloc_pin(&sunxi_ohci1);
#endif

		if(sunxi_ohci1.used){
			platform_device_register(&sunxi_usb_ohci_device[1]);
		}else{
			DMSG_INFO("usb%d %s is not enable\n", sunxi_ohci1.usbc_no, sunxi_ohci1.hci_name);
		}
#endif
	}

#ifdef  CONFIG_USB_SUNXI_EHCI2

	init_sunxi_hci(&sunxi_ehci2, HCI2_USBC_NO, 0, ehci_name);
	alloc_pin(&sunxi_ehci2);

	if(sunxi_ehci2.used){
		platform_device_register(&sunxi_usb_ehci_device[2]);
	}else{
		DMSG_INFO("usb%d %s is not enable\n", sunxi_ehci2.usbc_no, sunxi_ehci2.hci_name);
	}
#endif

	if(!sunxi_ehci2.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI2

		init_sunxi_hci(&sunxi_ohci2, HCI2_USBC_NO, 1, ohci_name);

#ifndef  CONFIG_USB_SUNXI_EHCI2
		alloc_pin(&sunxi_ohci2);
#endif

		if(sunxi_ohci2.used){
			platform_device_register(&sunxi_usb_ohci_device[2]);
		}else{
			DMSG_INFO("usb%d %s is not enable\n", sunxi_ohci2.usbc_no, sunxi_ohci2.hci_name);
		}
#endif
	}

#ifdef  CONFIG_USB_SUNXI_EHCI3

	init_sunxi_hci(&sunxi_ehci3, HCI3_USBC_NO, 0, ehci_name);
	alloc_pin(&sunxi_ehci3);

	if(sunxi_ehci3.used){
			platform_device_register(&sunxi_usb_ehci_device[3]);
	}else{
		DMSG_INFO("usb%d %s is not enable\n", sunxi_ehci3.usbc_no, sunxi_ehci3.hci_name);
	}
#endif

	if(!sunxi_ehci3.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI3

		init_sunxi_hci(&sunxi_ohci3, HCI3_USBC_NO, 1, ehci_name);

#ifndef  CONFIG_USB_SUNXI_EHCI3
		alloc_pin(&sunxi_ohci3);
#endif

		if(sunxi_ohci3.used){
			platform_device_register(&sunxi_usb_ohci_device[3]);
		}else{
			DMSG_INFO("usb%d %s is not enable\n", sunxi_ohci3.usbc_no, sunxi_ohci3.hci_name);
		}
#endif
	}
	return 0;
}

static void __exit sunxi_hci_exit(void)
{

#ifdef  CONFIG_USB_SUNXI_EHCI0
	if(sunxi_ehci0.used){
		platform_device_unregister(&sunxi_usb_ehci_device[0]);
	}else{
		DMSG_INFO("usb%d %s is disable\n", sunxi_ehci0.usbc_no, sunxi_ehci0.hci_name);
	}
	exit_sunxi_hci(&sunxi_ehci0, 0);
	free_pin(&sunxi_ehci0);
#endif
	if(!sunxi_ehci0.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI0
		if(sunxi_ohci0.used){
			platform_device_unregister(&sunxi_usb_ohci_device[0]);
		}else{
			DMSG_INFO("usb%d %s is disable\n", sunxi_ohci0.usbc_no, sunxi_ohci0.hci_name);
		}
		exit_sunxi_hci(&sunxi_ohci0, 1);

#ifndef  CONFIG_USB_SUNXI_EHCI0
		free_pin(&sunxi_ohci0);
#endif
#endif
	}

#ifdef  CONFIG_USB_SUNXI_EHCI1
	if(sunxi_ehci1.used){
		platform_device_unregister(&sunxi_usb_ehci_device[1]);
	}else{
		DMSG_INFO("usb%d %s is disable\n", sunxi_ehci1.usbc_no, sunxi_ehci1.hci_name);
	}
	exit_sunxi_hci(&sunxi_ehci1, 0);
	free_pin(&sunxi_ehci1);
#endif

	if(!sunxi_ehci1.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI1
		if(sunxi_ohci1.used){
			platform_device_unregister(&sunxi_usb_ohci_device[1]);
		}else{
			DMSG_INFO("usb%d %s is disable\n", sunxi_ohci1.usbc_no, sunxi_ohci1.hci_name);
		}

		exit_sunxi_hci(&sunxi_ehci1, 1);

#ifndef  CONFIG_USB_SUNXI_EHCI1
		free_pin(&sunxi_ohci1);
#endif
#endif
	}

#ifdef  CONFIG_USB_SUNXI_EHCI2
	if(sunxi_ehci2.used){
		platform_device_unregister(&sunxi_usb_ehci_device[2]);
	}else{
		DMSG_INFO("usb%d %s is disable\n", sunxi_ehci2.usbc_no, sunxi_ehci2.hci_name);
	}
	exit_sunxi_hci(&sunxi_ehci2, 0);
	free_pin(&sunxi_ehci2);
#endif

	if(!sunxi_ehci2.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI2
		if(sunxi_ohci2.used){
			platform_device_unregister(&sunxi_usb_ohci_device[2]);
		}else{
			DMSG_INFO("usb%d %s is disable\n", sunxi_ohci2.usbc_no, sunxi_ohci2.hci_name);
		}
		exit_sunxi_hci(&sunxi_ohci2, 1);

#ifndef  CONFIG_USB_SUNXI_EHCI2
		free_pin(&sunxi_ohci2);
#endif
#endif
	}

#ifdef  CONFIG_USB_SUNXI_EHCI3
	if(sunxi_ehci3.used){
		platform_device_unregister(&sunxi_usb_ehci_device[3]);
	}else{
		DMSG_INFO("usb%d %s is disable\n", sunxi_ehci3.usbc_no, sunxi_ehci3.hci_name);
	}
	exit_sunxi_hci(&sunxi_ehci3, 0);
	free_pin(&sunxi_ehci3);
#endif

	if(!sunxi_ehci3.hsic_flag){
#ifdef  CONFIG_USB_SUNXI_OHCI3
		if(sunxi_ohci3.used){
			platform_device_unregister(&sunxi_usb_ohci_device[3]);
		}else{
			DMSG_INFO("usb%d %s is disable\n", sunxi_ohci3.usbc_no, sunxi_ohci3.hci_name);
		}
		exit_sunxi_hci(&sunxi_ohci3, 1);

#ifndef  CONFIG_USB_SUNXI_EHCI3
		free_pin(&sunxi_ohci3);
#endif
#endif
	}

#ifdef CONFIG_ARCH_SUN9IW1

#ifdef CONFIG_USB_HCD_ENHANCE
	hci1_thread_run_flag = 0;
	hci2_thread_run_flag = 0;
	hci3_thread_run_flag = 0;
#endif

	if(!IS_ERR(hci_ahb_gate)){
		clk_put(hci_ahb_gate);
		hci_ahb_gate = NULL;
	}

	#ifdef CONFIG_USB_SUNXI_HSIC
	if(vbat_hsic_hdle){
		regulator_put(vbat_hsic_hdle);
	}
	#else
	if(vbat_usbh_hdle){
		regulator_put(vbat_usbh_hdle);
	}
	#endif
#endif
	return ;
}

fs_initcall(sunxi_hci_init);
//module_init(sunxi_hci_init);
module_exit(sunxi_hci_exit);

