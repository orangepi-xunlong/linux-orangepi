/*
 *  drivers/soc/sunxi/arisc/include/arisc_para.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ARISC_PARA_H__
#define __ARISC_PARA_H__

#include <linux/power/axp_depend.h>
#define ARISC_MACHINE_PAD    0
#define ARISC_MACHINE_HOMLET 1

/* arisc parameter size: 128byte */
/*
 * machine: machine id, pad = 0, homlet = 1;
 * message_pool_phys: message pool physical address;
 * message_pool_size: message pool size;
 */
#define SERVICES_DVFS_USED (1<<0)

typedef struct arisc_para {
	unsigned int machine;
	unsigned int oz_scale_delay;
	unsigned int oz_onoff_delay;
	unsigned int message_pool_phys;
	unsigned int message_pool_size;
	unsigned int uart_pin_used;
	unsigned int services_used;
	unsigned int power_regu_tree[VCC_MAX_INDEX];
	unsigned int reseved[10];
} arisc_para_t;

#define ARISC_PARA_SIZE (sizeof(struct arisc_para))
/* arisc clocksoure */
typedef enum arisc_clksrc {
	CLKSRC_LOSC      = 0,
	CLKSRC_IOSC      = 1,
	CLKSRC_HOSC      = 2,
	CLKSRC_PERIPH0   = 3,
	CLKSRC_ARISC_MAX = 4,
} arisc_clksrc_e;

/* ddr clocksoure */
typedef enum ddr_clksrc {
	CLKSRC_DDR0    = 0,
	CLKSRC_DDR1    = 1,
	CLKSRC_DDR_MAX = 2,
} ddr_clksrc_e;

typedef struct core_cfg {
	struct device_node *np;
	struct clk *losc;
	struct clk *iosc;
	struct clk *hosc;
	struct clk *pllperiph0;
	u32 powchk_used;
	u32 power_reg;
	u32 system_power;
} core_cfg_t;

typedef struct dev_cfg {
	struct device_node *np;
	struct clk *clk[ARISC_DEV_CLKSRC_NUM];
	void __iomem *vbase;
	phys_addr_t pbase;
	size_t size;
	u32 irq;
	int status;
} dev_cfg_t;

typedef struct dram_cfg {
	struct device_node *np;
	struct clk *pllddr0;
	struct clk *pllddr1;
} dram_cfg_t;

struct space_info {
	unsigned int dst;
	unsigned int offset;
	unsigned int size;
};

union space_union {
	unsigned int value[3];
	struct space_info info;
};

struct space {
	void __iomem *vbase;
	unsigned int size;
};

typedef struct arisc_cfg {
	struct core_cfg core;
	struct dram_cfg dram;
	struct dev_cfg suart;
#if (defined CONFIG_ARCH_SUN50IW2P1) || \
	(defined CONFIG_ARCH_SUN50IW6P1)
	struct dev_cfg stwi;
#else
	struct dev_cfg srsb;
#endif
	struct dev_cfg sjtag;
	struct dev_cfg msgbox;
	struct dev_cfg sram_a2;
	struct dev_cfg cpuscfg;
	struct space space[2];
	u32 power_regu_tree[VCC_MAX_INDEX];
} arisc_cfg_t;
#endif /* __ARISC_PARA_H__ */
