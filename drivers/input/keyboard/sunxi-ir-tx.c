/*
 * drivers/input/keyboard/sunxi-ir-tx.c
 *
 * Copyright (C) 2013-2014 allwinner.
 *	Li Ming<liming@allwinnertech.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk/clk-sun9iw1.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <asm/io.h>
#include "sunxi-ir-tx.h"

static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

struct ir_tx_info{
	int ir_used;
	struct gpio_config ir_tx_gpio;
};

struct sunxi_ir_tx_data {
	void __iomem *base_addr;
	struct clk *clk;
	struct clk *clk_src;
	struct pinctrl *pinctrl;
	struct ir_tx_info ir_info;
};
static struct sunxi_ir_tx_data *ir_tx_data;
struct ir_raw_buffer
{
	unsigned long tx_dcnt;
	unsigned char tx_buf[IR_RAW_BUF_SIZE];
};
static struct ir_raw_buffer	ir_rawbuf;
static char ir_tx_dev_name[] = "cir";

static void send_ir_code(unsigned int ir_tx_code);

static ssize_t sunxi_ir_tx_send_data(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;

	printk("%s : data = 0x%lx\n", __func__, data);

	send_ir_code((unsigned int)data);

	return count;
}

static DEVICE_ATTR(ir_tx, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		NULL, sunxi_ir_tx_send_data);

static struct attribute *ir_tx_attributes[] = {
	&dev_attr_ir_tx.attr,
	NULL
};

static struct attribute_group ir_tx_attribute_group = {
	.attrs = ir_tx_attributes
};

__inline int ir_txfifo_empty(void)
{
	return((readl(ir_tx_data->base_addr + IR_TX_TACR)) == IR_TX_FIFO_SIZE);
}

 __inline int ir_txfifo_full(void)
{
	return((readl(ir_tx_data->base_addr + IR_TX_TACR)) == 0);
}

__inline void ir_tx_reset_rawbuffer(void)
{
	ir_rawbuf.tx_dcnt = 0;
}

static void ir_tx_packet_handler(unsigned int ir_tx_code)
{
	unsigned int i,j;
	unsigned char buffer[256];
	unsigned int count=0;
	unsigned char tx_code[4];
	ir_tx_reset_rawbuffer();

	tx_code[0]=(unsigned char)(ir_tx_code&0xff);
	tx_code[1]=(unsigned char)(ir_tx_code>>8);
	tx_code[2]=(unsigned char)(ir_tx_code>>16);
	tx_code[3]=(unsigned char)(ir_tx_code>>24);

	/* go encoding */
	if (CLK_Ts == 1) {
		buffer[count++]=0xff;  /*424��ʱ������(21.35us)�ĸߵ�ƽ��9ms*/
		buffer[count++]=0xff;
		buffer[count++]=0xff;
		buffer[count++]=0xa5;
		buffer[count++]=0x7f;  /*212��ʱ������(21.35us)�ĵ͵�ƽ��4.5ms*/
		buffer[count++]=0x54;

		for (j=0; j<4; j++) {
			for (i=0; i<8;i++ ) {
				if (tx_code[j]&0x01) {
					/* ����1��560us�ߵ�ƽ��13*Ts����1680us�͵�ƽ��39*Ts��*/
					buffer[count++]=0x99;
					buffer[count++]=0x4d;
				} else {
					buffer[count++]=0x99;
					buffer[count++]=0x19;
				}
				tx_code[j]=tx_code[j]>>1;
			}
		}
		buffer[count++]=0x99;
		buffer[count++]=0x7f;
		buffer[count++]=0x7f;
		buffer[count++]=0x7f;
		buffer[count++]=0x7f;
		buffer[count++]=0x7f;
		buffer[count++]=0x7f;
	} else {
		for (j=0; j<4; j++) {
			if (IR_CYCLE_TYPE==0&&j%4==0) {
				buffer[count++]=0xff;  /*424��ʱ������(21.35us)�ĸߵ�ƽ��9ms*/
				buffer[count++]=0xff;
				buffer[count++]=0xff;
				buffer[count++]=0xab;
				buffer[count++]=0x7f;  /*212��ʱ������(21.35us)�ĵ͵�ƽ��4.5ms*/
				buffer[count++]=0x55;
			}
			for (i=0; i<8; i++) {
				if (tx_code[j]&0x01) {
					/* ����1��560us�ߵ�ƽ��13*Ts����1680us�͵�ƽ��39*Ts��*/
					buffer[count++]=0x9a;
					buffer[count++]=0x4e;
				}
				else
				{
					buffer[count++]=0x9a;
					buffer[count++]=0x1a;
				}
				tx_code[j]>>1;
			}
		}
		if (IR_CYCLE_TYPE == 0) {
			buffer[count++]=0x9a;
		}
	}

	for (i=0; i<count; i++) {
		ir_rawbuf.tx_buf[ir_rawbuf.tx_dcnt++]=buffer[i];
	}
	printk("%s: tx_dcnt = %ld\n", __func__, ir_rawbuf.tx_dcnt);
  }

static void send_ir_code(unsigned int ir_tx_code)
{
	unsigned int i = 0;
	u32 tmp;
	unsigned int idle_threshold;
	idle_threshold = (readl(ir_tx_data->base_addr +IR_TX_IDC_H)<<8) | readl(ir_tx_data->base_addr +IR_TX_IDC_L);

	printk("%s : enter\n", __func__);

	ir_tx_packet_handler(ir_tx_code);

	for (i=0;i<ir_rawbuf.tx_dcnt;i++) {
		while(ir_txfifo_full()){printk("%s: fifo full\n", __func__);}
		writeb(ir_rawbuf.tx_buf[i],ir_tx_data->base_addr+IR_TX_FIFO_DR);
	}

	if (IR_CYCLE_TYPE) {
		for(i=0;i<ir_rawbuf.tx_dcnt;i++)
		printk("%d,ir txbuffer code = 0x%x!\n",i, ir_rawbuf.tx_buf[i]);
		tmp = readl(ir_tx_data->base_addr + IR_TX_CR);
		tmp |= (0x01<<7);
		writel(tmp, ir_tx_data->base_addr + IR_TX_CR);
	} else {
		while(!(ir_txfifo_empty())){};
	}
	while((readl(ir_tx_data->base_addr +IR_TX_ICR_H)<<8 | readl(ir_tx_data->base_addr +IR_TX_ICR_L)) < idle_threshold){};  //wait idle finish
}

static inline unsigned long ir_tx_get_intsta(void)
{
	return (readl(ir_tx_data->base_addr + IR_TX_STAR));
}

static inline void ir_tx_clr_intsta(unsigned long bitmap)
{
	unsigned long tmp = readl(ir_tx_data->base_addr + IR_TX_STAR);

	tmp &= ~0xff;
	tmp |= bitmap&0xff;
	writel(tmp, ir_tx_data->base_addr + IR_TX_STAR);
}

static irqreturn_t ir_tx_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta = ir_tx_get_intsta();

	dprintk(DEBUG_INT, "IR TX IRQ Serve 0x%lx\n", intsta);

	ir_tx_clr_intsta(intsta);

	if (intsta & 0x01) {
	};

	return IRQ_HANDLED;
}

static void sunxi_ir_tx_clk_cfg(struct sunxi_ir_tx_data *ir_data)
{
	unsigned long rate = 0;

	ir_data->clk_src = clk_get(NULL, HOSC_CLK);
	if (!ir_data->clk_src || IS_ERR(ir_data->clk_src)) {
		printk(KERN_DEBUG "try to get ir_tx_clk_source clock failed!\n");
		return;
	}

	rate = clk_get_rate(ir_data->clk_src);
	dprintk(DEBUG_INIT, "%s: get ir_tx_clk_source rate %dHZ\n", __func__, (__u32)rate);

	ir_data->clk = clk_get(NULL, "cirtx");
	if (!ir_data->clk || IS_ERR(ir_data->clk)) {
		printk(KERN_DEBUG "try to get ir tx clock failed!\n");
		return;
	}

	if(clk_set_parent(ir_data->clk, ir_data->clk_src))
		printk("%s: set ir_tx_clk parent to ir_clk_source failed!\n", __func__);

	if (clk_set_rate(ir_data->clk, 6000000)) {
		printk(KERN_DEBUG "set ir tx clock freq to 3M failed!\n");
	}

	if (clk_prepare_enable(ir_data->clk)) {
		printk(KERN_DEBUG "try to enable ir_tx_clk failed!\n");
	}

	return;
}

static void sunxi_ir_tx_clk_uncfg(struct sunxi_ir_tx_data *ir_data)
{

	if(NULL == ir_data->clk || IS_ERR(ir_data->clk)) {
		printk("ir_tx_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(ir_data->clk);
		clk_put(ir_data->clk);
		ir_data->clk = NULL;
	}

	if(NULL == ir_data->clk_src || IS_ERR(ir_data->clk_src)) {
		printk("ir_tx_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(ir_data->clk_src);
		ir_data->clk_src = NULL;
	}
	return;
}


static void sunxi_ir_tx_reg_clear(struct sunxi_ir_tx_data *ir_data)
{
	writel(0, ir_data->base_addr + IR_TX_GLR);
}

static void sunxi_ir_tx_reg_init(struct sunxi_ir_tx_data *ir_data)
{
	writel(IR_TX_MC_VALUE, ir_data->base_addr + IR_TX_MCR);
	writel(IR_TX_C_VALUE, ir_data->base_addr + IR_TX_CR);
	writel(IR_TX_IDC_H_VALUE, ir_data->base_addr + IR_TX_IDC_H);
	writel(IR_TX_IDC_L_VALUE, ir_data->base_addr + IR_TX_IDC_L);
	writel(IR_TX_TEL_VALUE, ir_data->base_addr + IR_TX_TELR);
	writel(IR_TX_STA_VALUE, ir_data->base_addr + IR_TX_STAR);
	writel(IR_TX_INT_C_VALUE, ir_data->base_addr + IR_TX_INT_CR);
	writel(IR_TX_T_VALUE, ir_data->base_addr + IR_TX_TR);
	writel(IR_TX_GL_VALUE, ir_data->base_addr + IR_TX_GLR);

	dprintk(DEBUG_INIT, "IR_TX_GLR = 0x%x\n", readl(ir_data->base_addr + IR_TX_GLR));
	dprintk(DEBUG_INIT, "IR_TX_MCR = 0x%x\n", readl(ir_data->base_addr + IR_TX_MCR));
	dprintk(DEBUG_INIT, "IR_TX_CR = 0x%x\n", readl(ir_data->base_addr + IR_TX_CR));
	dprintk(DEBUG_INIT, "IR_TX_IDC_L = 0x%x\n", readl(ir_data->base_addr + IR_TX_IDC_L));
	dprintk(DEBUG_INIT, "IR_TX_TELR = 0x%x\n", readl(ir_data->base_addr + IR_TX_TELR));
	dprintk(DEBUG_INIT, "IR_TX_INT_CR = 0x%x\n", readl(ir_data->base_addr + IR_TX_INT_CR));
	dprintk(DEBUG_INIT, "IR_TX_TR = 0x%x\n", readl(ir_data->base_addr + IR_TX_TR));
}

static int ir_tx_fetch_sysconfig_para(struct sunxi_ir_tx_data *ir_data)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;

	type = script_get_item("cir", "ir_used", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	ir_data->ir_info.ir_used = val.val;

	if (1 == ir_data->ir_info.ir_used) {
		type = script_get_item("cir", "ir_tx", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){
			pr_err("%s: IR TX gpio type err! \n", __func__);
			goto script_get_err;
		}
		ir_data->ir_info.ir_tx_gpio = val.gpio;

		ret = 0;

	} else {
		pr_err("%s: ir_tx_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}

#ifdef CONFIG_PM
static int sunxi_ir_tx_suspend(struct platform_device *pdev, pm_message_t state)
{
	dprintk(DEBUG_SUSPEND, "enter: %s. \n", __func__);

	sunxi_ir_tx_reg_clear(ir_tx_data);

	disable_irq_nosync(IR_TX_IRQNO);

	if(NULL == ir_tx_data->clk || IS_ERR(ir_tx_data->clk)) {
		printk("ir_tx_clk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_disable_unprepare(ir_tx_data->clk);
	}
	return 0;
};

static int sunxi_ir_tx_resume(struct platform_device *pdev)
{
	dprintk(DEBUG_SUSPEND, "enter: %s. \n", __func__);

	sunxi_ir_tx_clk_cfg(ir_tx_data);
	sunxi_ir_tx_reg_init(ir_tx_data);
	enable_irq(IR_TX_IRQNO);
	return 0;
};
#endif

static int  sunxi_ir_tx_probe(struct platform_device *pdev)
{
	int err = 0;

	pdev->dev.init_name = &ir_tx_dev_name[0];

	ir_tx_data->pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR_OR_NULL(ir_tx_data->pinctrl)) {
		printk(KERN_ERR "%s :request pinctrl handle failed\n", __func__);
		err =  -EINVAL;
		goto fail;
	}

	sunxi_ir_tx_clk_cfg(ir_tx_data);

	sunxi_ir_tx_reg_init(ir_tx_data);

	if (request_irq(IR_TX_IRQNO, ir_tx_irq_service, 0, "RemoteIR_TX",
			&pdev->dev)) {
		err = -EBUSY;
		goto fail1;
	}

	err = sysfs_create_group(&pdev->dev.kobj, &ir_tx_attribute_group);
	if (err) {
		printk(KERN_ERR "%s: cteate sysfs node failed\n", __func__);
		goto fial2;
	}

	return 0;
fial2:
	free_irq(IR_TX_IRQNO, &pdev->dev);
fail1:
	sunxi_ir_tx_reg_clear(ir_tx_data);
	sunxi_ir_tx_clk_uncfg(ir_tx_data);
	if (!IS_ERR_OR_NULL(ir_tx_data->pinctrl))
		devm_pinctrl_put(ir_tx_data->pinctrl);
fail:
	return err;
}

static int sunxi_ir_tx_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &ir_tx_attribute_group);
	free_irq(IR_TX_IRQNO, &pdev->dev);
	sunxi_ir_tx_reg_clear(ir_tx_data);
	sunxi_ir_tx_clk_uncfg(ir_tx_data);
	if (!IS_ERR_OR_NULL(ir_tx_data->pinctrl))
		devm_pinctrl_put(ir_tx_data->pinctrl);
	return 0;
}


static struct platform_device sunxi_ir_tx_device = {
	.name		    = IR_TX_DEV_NAME,
	.id		        = PLATFORM_DEVID_NONE,
};

static struct platform_driver sunxi_ir_tx_driver = {
	.driver		= {
		.name	= IR_TX_DEV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= sunxi_ir_tx_probe,
	.remove		= sunxi_ir_tx_remove,
#ifdef CONFIG_PM
	.suspend	= sunxi_ir_tx_suspend,
	.resume		= sunxi_ir_tx_resume,
#endif
};

static int __init ir_tx_init(void)
{
	int err = 0;

	ir_tx_data = kzalloc(sizeof(*ir_tx_data), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ir_tx_data)) {
		printk(KERN_ERR "ir_tx_data: not enough memory for device\n");
		err = -ENOMEM;
		goto fail1;
	}

	ir_tx_data->base_addr = (void __iomem *)IR_TX_BASSADDRESS;

	if (ir_tx_fetch_sysconfig_para(ir_tx_data)) {
		printk(KERN_ERR "%s: fetch sysconfig err\n", __func__);
		return -1;
	}

	err = platform_driver_register(&sunxi_ir_tx_driver);
	if (IS_ERR_VALUE(err)) {
		printk(KERN_ERR "register sunxi arisc platform driver failed\n");
		goto fail2;
	}
	err = platform_device_register(&sunxi_ir_tx_device);
	if (IS_ERR_VALUE(err)) {
		printk(KERN_ERR "register sunxi arisc platform device failed\n");
		goto fail3;
	}

	return 0;
fail3:
	platform_driver_unregister(&sunxi_ir_tx_driver);
fail2:
	kfree(ir_tx_data);
fail1:
	return err;

}

static void __exit ir_tx_exit(void)
{
	platform_device_unregister(&sunxi_ir_tx_device);
	platform_driver_unregister(&sunxi_ir_tx_driver);
	kfree(ir_tx_data);
	printk(KERN_INFO "%s: module unloaded\n", __func__);
}

module_init(ir_tx_init);
module_exit(ir_tx_exit);
module_param_named(debug_mask, debug_mask, int, 0644);
MODULE_DESCRIPTION("Remote IR TX driver");
MODULE_AUTHOR("Li Ming");
MODULE_LICENSE("GPL");
