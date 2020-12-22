/*
 *  8250_sunxi.c
 *
 *  Copyright (C) 1996-2003 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/ecard.h>
#include <asm/string.h>
#include <linux/clk.h>
#include <linux/console.h>
#include <linux/serial_reg.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include "8250.h"


#define MAX_PORTS	    6

//static int sw_serial[MAX_PORTS];
 
#define	BACK_REG	sunxi_serial_reg_back[port_num]

/* Register base define */
#define UART_BASE       (SUNXI_UART0_PBASE)
#define UART_BASE_OS    (0x400)
#define UARTx_BASE(x)   (UART_BASE + (x) * UART_BASE_OS)
#define RESSIZE(res)    (((res)->end - (res)->start)+1)

#define AW_UART_RD(offset)			port->serial_in(port,(offset))
#define AW_UART_WR(value,offset)	port->serial_out(port,(offset),(value))
#define OFFSET          			0xf0000000
u32 debug_mask=0;
u32 debug_mask_pm=0;
typedef struct backup_reg_def{
	u32 dll; 		/* 0x00	*/
	u32 dlh;		/* 0x04 */
	u32 ier;		/* 0x04 */
	u32 fcr;		/* 0x08 */
	u32 lcr;		/* 0x0C */
	u32 mcr;		/* 0x10 */
	u32 sch;		/* 0x1C */
	u32 halt;		/* 0xA4 */
} backup_reg_t;

struct sw_serial_data {
	unsigned int	last_lcr;
	unsigned int	last_mcr;
	int				line;
};

struct sw_serial_port {
    struct uart_port    port;
    char                name[16];
    int                 port_no;
    int                 pin_num;
    u32                 pio_hdle;
//	struct clk			*bus_clk;
    struct clk			*mod_clk;
    u32                 sclk;
    struct resource     *mmres;
    u32                 irq;
	char				*bus_clk_name;
	char                *mod_clk_name;
    struct platform_device* pdev;
};

struct sw_serial_port *sw_serial_uart[6];
static int sw_uart_have_used =0;
backup_reg_t sunxi_serial_reg_back[MAX_PORTS];

static char	*mod_clock_name[]={
								"uart0",
								"uart1",
								"uart2",
								"uart3",
								"uart4",
								"uart5"
								};
#if 0
static char	*apb_clock_name[]={
								CLK_APB_UART0,
								CLK_APB_UART1,
								CLK_APB_UART2,
								CLK_APB_UART3,
								CLK_APB_UART4,
								CLK_APB_UART5
								};
#endif
static int sw_serial_get_resource(struct sw_serial_port *sport)
{
    char uart_para[16];
    int cnt,i=0,ret;
	script_item_u *list = NULL;

    /* get register base */
    sport->mmres = platform_get_resource(sport->pdev, IORESOURCE_MEM, 0);
    if (!sport->mmres){
		ret = -ENODEV;
        printk(KERN_WARNING "no IORESOURCE_MEM");
		goto err_out;
	}
#if 0
    /* get clock */
	sport->bus_clk_name	= apb_clock_name[sport->port_no];
    sport->bus_clk = clk_get(NULL, sport->bus_clk_name);
    if (IS_ERR(sport->bus_clk)) {
        ret = PTR_ERR(sport->bus_clk);
        goto iounmap;
    }
#endif
    sport->mod_clk_name	= mod_clock_name[sport->port_no];
    sport->mod_clk = clk_get(NULL, sport->mod_clk_name);
    if (IS_ERR(sport->mod_clk)) {
        ret = PTR_ERR(sport->mod_clk);
        goto iounmap;
    }

	sport->sclk = clk_get_rate(sport->mod_clk);
	/* get irq */
    sport->irq = platform_get_irq(sport->pdev, 0);
    if (sport->irq == 0) {
        ret = -EINVAL;
		printk(KERN_WARNING "no IORESOURCE_irq");
        goto iounmap;
    }
//	clk_enable(sport->bus_clk);
	clk_prepare_enable(sport->mod_clk);
//	clk_reset(sport->mod_clk,AW_CCU_CLK_NRESET);
	sprintf(uart_para, "uart_para%d", sport->port_no);
    cnt = script_get_pio_list(uart_para, &list);
	if(!cnt){
		ret = -EINVAL;
		goto free_pclk;
	}
#if 0//FIXME
	for(i=0;i<cnt;i++)
		if(gpio_request(list[i].gpio.gpio, NULL)){
			ret = -EINVAL;
			goto free_pclk;
		}
#endif
        #if 0//FIXME
    if(sw_gpio_setall_range(&list[0].gpio, cnt)){
		printk(KERN_ERR "set value err!\n");
		goto free_pclk;
	}
    #endif
    return 0;

free_pclk:
	clk_put(sport->mod_clk);
//    clk_put(sport->bus_clk);
iounmap:
err_out:
	while(i--)
		gpio_free(list[i].gpio.gpio);
    return ret;
}

static int sw_serial_put_resource(struct sw_serial_port *sport)
{
	script_item_u *list = NULL;
    char uart_para[16];
	int cnt,i;

//	clk_reset(sport->mod_clk,AW_CCU_CLK_RESET);
    clk_disable_unprepare(sport->mod_clk);
//	clk_disable(sport->bus_clk);
    clk_put(sport->mod_clk);
//	clk_put(sport->bus_clk);

	sprintf(uart_para, "uart_para%d", sport->port_no);
	cnt = script_get_pio_list(uart_para, &list);
	if(!cnt){
		return 1;
	}
 
	for(i=0;i<cnt;i++)
		gpio_free(list[i].gpio.gpio);

	return 0;
}

static int sw_serial_get_config(struct sw_serial_port *sport, u32 uart_id)
{
	char uart_para[16] = {0};
	script_item_u   val;
	script_item_value_type_e  type;

	memset(uart_para, 0, sizeof(uart_para));
	snprintf(uart_para, sizeof(uart_para), "uart_para%d", uart_id);

	type = script_get_item(uart_para, "uart_port", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
		printk(KERN_WARNING "uart port err!\n");
		return -1;
	}
	sport->port_no	= val.val;
	if (sport->port_no != uart_id){
		printk(KERN_WARNING "port_no%d uart_id %d  err!\n",sport->port_no,uart_id);
        	return -1;
	}

	type = script_get_item(uart_para, "uart_type", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
		printk(KERN_WARNING "uart type err!\n");
		return -1;
	}
	sport->pin_num	= val.val;

	return 0;
}

static void
sw_serial_pm(struct uart_port *port, unsigned int state,
          unsigned int oldstate)
{
	struct sw_serial_port *up = sw_serial_uart[port->irq-SUNXI_IRQ_UART_DEBUG];

	if (!state){
//		clk_enable(up->bus_clk);
		clk_prepare_enable(up->mod_clk);
//		clk_reset(up->mod_clk,AW_CCU_CLK_NRESET);
	}else{
//		clk_reset(up->mod_clk,AW_CCU_CLK_RESET);
		clk_disable_unprepare(up->mod_clk);
//        clk_disable(up->bus_clk);
	}
}

void sw_serial_do_pm(struct uart_port *port, unsigned int state,
		unsigned int oldstate)
{
	sw_serial_pm(port,state,oldstate);
}
EXPORT_SYMBOL(sw_serial_do_pm);

static void sunxi_serial_out(struct uart_port *p, int offset, int value)
{
	struct sw_serial_data *d = p->private_data;

	if (offset == UART_LCR)
		d->last_lcr = value;

	offset <<= p->regshift;
	writel(value, p->membase + offset);
}

static unsigned int sunxi_serial_in(struct uart_port *p, int offset)
{
	offset <<= p->regshift;

	return readl(p->membase + offset);
}


static int __devinit
sw_serial_probe(struct platform_device *dev)
{
    struct sw_serial_port *sport;
	struct sw_serial_data *sdata;
	int ret;

	sport = kzalloc(sizeof(struct sw_serial_port), GFP_KERNEL);
	if (!sport)
		return -ENOMEM;

	sdata = devm_kzalloc(&dev->dev, sizeof(*sdata), GFP_KERNEL);
	if (!sdata)
		return -ENOMEM;
	sw_serial_uart[dev->id]=sport;
	sport->port_no  = dev->id;
    sport->pdev     = dev;

    ret = sw_serial_get_config(sport, dev->id);
    if (ret) {
        printk(KERN_ERR "Failed to get config information\n");
        goto free_dev;
    }

	platform_set_drvdata(dev, sport);

	ret = sw_serial_get_resource(sport);
    if (ret) {
        printk(KERN_ERR "Failed to get resource\n");
        goto free_dev;
    }

	sport->port.private_data= sdata;
	spin_lock_init(&sport->port.lock);
    sport->port.irq     	= sport->irq;
    sport->port.fifosize	= 64;
	sport->port.regshift	= 2;
    sport->port.iotype  	= UPIO_MEM32;
    sport->port.type 		= PORT_U6_16550A;
	sport->port.flags   	= UPF_IOREMAP | UPF_BOOT_AUTOCONF;
    sport->port.serial_in	= sunxi_serial_in;
	sport->port.serial_out	= sunxi_serial_out;
	sport->port.uartclk 	= sport->sclk;
    sport->port.pm      	= sw_serial_pm;
    sport->port.dev     	= &dev->dev;
	sport->port.membase 	= (unsigned char __iomem    *)sport->mmres->start + OFFSET;
    sport->port.mapbase 	= sport->mmres->start;
	if(sport->irq != SUNXI_IRQ_UART_DEBUG)
		sdata->line = serial8250_register_port(&sport->port);
	else
		sdata->line = 0;
	if(sdata->line<0){
		ret = sdata->line;
		goto free_dev;	
	}

	if(sdata->line){
//		clk_reset(sport->mod_clk,AW_CCU_CLK_RESET);
		clk_disable_unprepare(sport->mod_clk);
//		clk_disable(sport->bus_clk);
	}
   	return 0;
free_dev:
    kfree(sport);
	kfree(sdata);
    sport = NULL;
	sdata = NULL;
    return ret;
}

#define FCR_AW  0x0e1
void sunxi_8250_backup_reg(int port_num ,struct uart_port *port)
{
	unsigned long port_base_addr;
	port_base_addr = port->mapbase + OFFSET;

	BACK_REG.fcr	= FCR_AW;
	BACK_REG.lcr	= AW_UART_RD(UART_LCR);
	BACK_REG.mcr	= AW_UART_RD(UART_MCR);
	BACK_REG.sch	= AW_UART_RD(UART_SCH) & 0xff;
	BACK_REG.halt	= 0x00;
	BACK_REG.ier	= AW_UART_RD(UART_IER);
	BACK_REG.dll	= BACK_REG.sch & 0xff;
}
EXPORT_SYMBOL(sunxi_8250_backup_reg);

void sunxi_8250_comeback_reg(int port_num,struct uart_port *port)
{
	unsigned long port_base_addr;
	port_base_addr = port->mapbase + OFFSET;
	AW_UART_WR(BACK_REG.sch,UART_SCH);
	if(AW_UART_RD(UART_USR)&1){
		if(port->irq==32)
			debug_mask=1;
		AW_UART_WR(BACK_REG.fcr,UART_FCR);
		AW_UART_WR(BACK_REG.mcr,UART_MCR);

		AW_UART_WR(BACK_REG.halt |UART_FORCE_CFG,UART_HALT);
		AW_UART_WR(BACK_REG.lcr,UART_LCR);

		AW_UART_WR(BACK_REG.dll,UART_DLL);
		AW_UART_WR(BACK_REG.dlh,UART_DLM);

		AW_UART_WR(BACK_REG.halt |UART_FORCE_CFG |UART_FORCE_UPDATE ,UART_HALT);
		while(AW_UART_RD(UART_HALT)&UART_FORCE_UPDATE);
		AW_UART_WR(BACK_REG.halt ,UART_HALT);
		AW_UART_WR(BACK_REG.ier,UART_IER);
	}else{
		if(port->irq==32)
			debug_mask=0;
		AW_UART_WR(BACK_REG.lcr | 0x80,UART_LCR);
		AW_UART_WR(BACK_REG.dll,UART_DLL);
		AW_UART_WR(BACK_REG.dlh,UART_DLM);

		AW_UART_WR(BACK_REG.lcr & 0x7f,UART_LCR);
		AW_UART_WR(BACK_REG.fcr,UART_FCR);
		AW_UART_WR(BACK_REG.mcr,UART_MCR);
		AW_UART_WR(BACK_REG.ier,UART_IER);

	}
}
EXPORT_SYMBOL(sunxi_8250_comeback_reg);

static int __devexit sw_serial_remove(struct platform_device *dev)
{
    struct sw_serial_port *sport = platform_get_drvdata(dev);
	struct sw_serial_data *sdata = sport->port.private_data;

	serial8250_unregister_port(sdata->line);
	sdata->line = -1;
	sw_serial_put_resource(sport);

	kfree(sport);
	sport = NULL;
	return 0;
}

static int sw_serial_suspend(struct platform_device *dev, pm_message_t state)
{
	int i;
	struct sw_serial_port *up;
	struct uart_port *port;
	struct sw_serial_data *sdata;

	for (i = 1; i < MAX_PORTS; i++) {
		if(!sw_serial_uart[i]){
			continue;
		}
		up		= sw_serial_uart[i];
		port	= &(up->port);
		sdata	= port->private_data;

		if ((port->type != PORT_UNKNOWN)&& (port->dev == &dev->dev)){
			sunxi_8250_backup_reg(sdata->line,port);
			serial8250_suspend_port(sdata->line);
		}
	}

	return 0;
}

static int sw_serial_resume(struct platform_device *dev)
{
	int i;
	struct sw_serial_port *up;
	struct uart_port *port;
	struct sw_serial_data *sdata;

	for (i = 1; i < MAX_PORTS; i++) {
		if(!sw_serial_uart[i]){
			continue;
		}
		up		= sw_serial_uart[i];
		port	= &(up->port);
		sdata	= port->private_data;

		if ((port->type != PORT_UNKNOWN) && (port->dev == &dev->dev)){
			serial8250_resume_port(sdata->line);
			sunxi_8250_comeback_reg(sdata->line,port);
		}	

	}
	return 0;
}


static void sunxi_serial_release (struct device *dev)
{
	printk("sunxi serial release good !\n");
}


static struct platform_driver sw_serial_driver = {
    .probe  = sw_serial_probe,
    .remove = sw_serial_remove,
	.suspend	= sw_serial_suspend,
	.resume		= sw_serial_resume,
    .driver = {
        .name    = "sunxi-uart",
        .owner    = THIS_MODULE,
    },
};

static struct resource sw_uart_res[8][2] = {
    {/* uart0 resource */
        {.start = UARTx_BASE(0),	.end = UARTx_BASE(0) + UART_BASE_OS - 1,	.flags = IORESOURCE_MEM}, /*base*/
        {.start = SUNXI_IRQ_UART0 ,	.end = SUNXI_IRQ_UART0,						.flags = IORESOURCE_IRQ}, /*irq */
    },
    {/* uart1 resource */
        {.start = UARTx_BASE(1),	.end = UARTx_BASE(1) + UART_BASE_OS - 1,	.flags = IORESOURCE_MEM}, /*base*/
        {.start = SUNXI_IRQ_UART1 ,	.end = SUNXI_IRQ_UART1,						.flags = IORESOURCE_IRQ}, /*irq */
    },
    {/* uart2 resource */
        {.start = UARTx_BASE(2),	.end = UARTx_BASE(2) + UART_BASE_OS - 1,	.flags = IORESOURCE_MEM}, /*base*/
        {.start = SUNXI_IRQ_UART2 ,	.end = SUNXI_IRQ_UART2,						.flags = IORESOURCE_IRQ}, /*irq */
    },
    {/* uart3 resource */
        {.start = UARTx_BASE(3),	.end = UARTx_BASE(3) + UART_BASE_OS - 1,	.flags = IORESOURCE_MEM}, /*base*/
        {.start = SUNXI_IRQ_UART3 ,	.end = SUNXI_IRQ_UART3,						.flags = IORESOURCE_IRQ}, /*irq */
    },
    {/* uart4 resource */
        {.start = UARTx_BASE(4),	.end = UARTx_BASE(4) + UART_BASE_OS - 1,	.flags = IORESOURCE_MEM}, /*base*/
        {.start = SUNXI_IRQ_UART4,		.end = SUNXI_IRQ_UART4,						.flags = IORESOURCE_IRQ}, /*irq */
    },
    {/* uart5 resource */
        {.start = UARTx_BASE(5),	.end = UARTx_BASE(5) + UART_BASE_OS - 1,	.flags = IORESOURCE_MEM}, /*base*/
        {.start = SUNXI_IRQ_UART5 ,	.end = SUNXI_IRQ_UART5,						.flags = IORESOURCE_IRQ}, /*irq */
    },
};

struct platform_device sw_uart_dev[] = {
    [0] = {.name = "sunxi-uart", .id = 0, .num_resources = ARRAY_SIZE(sw_uart_res[0]), .resource = &sw_uart_res[0][0], .dev.release=sunxi_serial_release},
    [1] = {.name = "sunxi-uart", .id = 1, .num_resources = ARRAY_SIZE(sw_uart_res[1]), .resource = &sw_uart_res[1][0], .dev.release=sunxi_serial_release},
    [2] = {.name = "sunxi-uart", .id = 2, .num_resources = ARRAY_SIZE(sw_uart_res[2]), .resource = &sw_uart_res[2][0], .dev.release=sunxi_serial_release},
    [3] = {.name = "sunxi-uart", .id = 3, .num_resources = ARRAY_SIZE(sw_uart_res[3]), .resource = &sw_uart_res[3][0], .dev.release=sunxi_serial_release},
    [4] = {.name = "sunxi-uart", .id = 4, .num_resources = ARRAY_SIZE(sw_uart_res[4]), .resource = &sw_uart_res[4][0], .dev.release=sunxi_serial_release},
    [5] = {.name = "sunxi-uart", .id = 5, .num_resources = ARRAY_SIZE(sw_uart_res[5]), .resource = &sw_uart_res[5][0], .dev.release=sunxi_serial_release},
};

static int uart_used;
static int __init sw_serial_init(void)
{
    int ret;
    int i;
    int used = 0;
    char uart_para[16];
	script_item_u   val;
	script_item_value_type_e  type;
	debug_mask = 0;
	debug_mask_pm = 0;
	uart_used = 0;
	for (i=0; i<MAX_PORTS; i++, used=0) {
        sprintf(uart_para, "uart_para%d", i);
		sw_serial_uart[i]=NULL;
		type = script_get_item(uart_para, "uart_used", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			printk(KERN_WARNING "failed to get uart%d's used information\n",i);
			return -1;
		}
		used	= val.val;

		if (used) {
            uart_used |= 1 << i;
            sw_uart_have_used ++;
			platform_device_register(&sw_uart_dev[i]);
        }
    }
    if (uart_used) {
        ret = platform_driver_register(&sw_serial_driver);
        return ret;
    }

	return 0;
}

static void __exit sw_serial_exit(void)
{
    int i=0;
	while(uart_used){
		if(uart_used%2)
			platform_device_unregister(&sw_uart_dev[i]);
		i++;
		uart_used >>= 1;
	}
	if (uart_used)
	    platform_driver_unregister(&sw_serial_driver);
	sw_uart_have_used = 0;
}

MODULE_AUTHOR("Aaron.myeh<leafy.myeh@reuuimllatech.com>");
MODULE_DESCRIPTION("SUNXI 8250-compatible serial port expansion card driver");
MODULE_LICENSE("GPL");
module_param(debug_mask_pm,uint,0644);
MODULE_PARM_DESC(debug_mask_pm,"sw plartform uart debug switch");


module_param(debug_mask,uint,0644);
MODULE_PARM_DESC(debug_mask,"sw plartform uart debug switch");

module_init(sw_serial_init);
module_exit(sw_serial_exit);
