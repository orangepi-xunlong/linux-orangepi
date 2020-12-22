#ifndef _SUNXI_DI_H
#define _SUNXI_DI_H

#include <linux/types.h>
#include "di.h"

#if defined CONFIG_ARCH_SUN9IW1P1
#include <linux/clk/clk-sun9iw1.h>
#define BUS_CLK_NAME    PLL10_CLK
#define MODULE_CLK_NAME DE_CLK
#define SW_INT_IRQNO_DI (SUNXI_IRQ_DEFE0)
#define DI_BASE         (0xf3100000)
#elif defined CONFIG_ARCH_SUN8IW7P1
#include <linux/clk/clk-sun8iw7.h>
#define BUS_CLK_NAME    PLL_PERIPH0_CLK
#define MODULE_CLK_NAME DEINTERLACE_CLK
#define SW_INT_IRQNO_DI (SUNXI_IRQ_DIT)
#define DI_BASE         (0xf1400000)
#endif

#define DI_MODULE_NAME "deinterlace"
#define DI_TIMEOUT                      30                    /* DI-Interlace 30ms timeout */
#define DI_MODULE_TIMEOUT               0x1055
#define FLAG_WIDTH                      (2048)
#define FLAG_HIGH                       (1100)

typedef struct {
	atomic_t di_complete;
	atomic_t enable;
	wait_queue_head_t wait;
	void * in_flag;
	void * out_flag;
	u32 mode;
	unsigned int in_flag_phy;
	unsigned int out_flag_phy;
	unsigned int flag_size;
#ifdef CONFIG_PM
	struct dev_pm_domain di_pm_domain;
#endif
}di_struct, *pdi_struct;

#define	DI_IOC_MAGIC		'D'
#define	DI_IOCSTART		_IOWR(DI_IOC_MAGIC, 0, __di_para_t *)

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_TEST = 1U << 4,
};

#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	 printk(KERN_DEBUG fmt , ## arg)

#endif

