/*
 * arch/arm/mach-sunxi/sun8i.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun8i platform file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/sunxi_timer.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/param.h>
#include <linux/memblock.h>
#include <linux/arisc/arisc.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>

#include <asm/pmu.h>
#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/arch_timer.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sunxi-chip.h>
#include <mach/sunxi-smc.h>

#ifdef CONFIG_SMP
extern struct smp_operations sunxi_smp_ops;
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#include <linux/persistent_ram.h>

/* sunxi ram_console */
struct resource ram_console_res[] = {
    {
        .start = RC_MEM_BASE,
        .end   = RC_MEM_BASE + RC_MEM_SIZE - 1,
        .flags  = IORESOURCE_MEM,
    },
};
struct ram_console_platform_data {
    const char *bootinfo;
};

static struct ram_console_platform_data ram_console_pdata;

static struct platform_device ram_console_pdev = {
    .name = "ram_console",
    .id = -1,
    .num_resources = ARRAY_SIZE(ram_console_res),
    .resource = ram_console_res,
    .dev = {
        .platform_data  = &ram_console_pdata,
    },
};

static struct platform_device *sw_pdevs[] __initdata = {
    &ram_console_pdev,
};

static void __init ram_console_device_init(void)
{
	platform_add_devices(sw_pdevs, ARRAY_SIZE(sw_pdevs));
}

struct persistent_ram_descriptor rc_pram_desc[] = {
    {"ram_console", RC_MEM_SIZE},
};

struct persistent_ram rc_pram = {
    .start = RC_MEM_BASE,
    .size  = RC_MEM_SIZE,
    .num_descs = ARRAY_SIZE(rc_pram_desc),
    .descs = rc_pram_desc,
};

static void __init ram_console_persistent_ram_init(void)
{
    int ret = persistent_ram_early_init(&rc_pram);
    if (ret) {
        printk(KERN_ERR "ram console memory reserved init err!\n");
    }
}
#endif
/* plat memory info, maybe from boot, so we need bkup for future use */
unsigned int mem_start = PLAT_PHYS_OFFSET;
unsigned int mem_size = PLAT_MEM_SIZE;
static unsigned int sys_config_size = SYS_CONFIG_MEMSIZE;

#if defined(CONFIG_SENSORS_INA219)
static struct i2c_board_info i2c_ina219_devs[] __initdata = {
	{ I2C_BOARD_INFO("ina219_vcc3", 0x40), },
	{ I2C_BOARD_INFO("ina219_cpua", 0x41), },
	{ I2C_BOARD_INFO("ina219_cpub", 0x42), },
	{ I2C_BOARD_INFO("ina219_syuh", 0x43), },
	{ I2C_BOARD_INFO("ina219_dram", 0x44), },
	{ I2C_BOARD_INFO("ina219_vgpu", 0x46), },
};
#endif

/*------------------------------------------------------------------------------
 * Matrix device
 */
#if defined(CONFIG_INPUT_ADXL34X_I2C_MODULE)
#define ADXL34X_I2C_BUS     (0)
static struct i2c_board_info __initdata adxl34x_i2c_bdi = {
	I2C_BOARD_INFO("adxl34x", 0x1d),
	.irq	= -1,
};
#endif

#if defined(CONFIG_BMP085_MODULE)
#define BMP085_I2C_BUS (0)
static struct i2c_board_info __initdata bmp085_i2c_bdi = {
	I2C_BOARD_INFO("bmp085", 0x77),
	.irq	= -1,
};
#endif

#if defined(CONFIG_RTC_DRV_DS1307_MODULE)
#define DS1307_I2C_BUS (0)
static struct i2c_board_info __initdata ds1307_i2c_bdi = {
	I2C_BOARD_INFO("ds1307", 0X68),
	.irq	= -1,
};
#endif

#if defined(CONFIG_SENSORS_PCF8591_MODULE)
#define PCF8591_I2C_BUS (0)
static struct i2c_board_info __initdata pcf8591_i2c_bdi = {
        I2C_BOARD_INFO("pcf8591", 0x48),
};
#endif

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
#define DEFAULT_SUNXI_ION_RESERVE_SIZE	96
#define ION_CARVEOUT_INIT_MAX	4
#define ION_CMA_INIT_MAX	4
#define ION_LEVEL_MAX		3
struct tag_mem32 ion_mem = { /* the real ion reserve info */
       .size  = DEFAULT_SUNXI_ION_RESERVE_SIZE << 20,
       .start = PLAT_PHYS_OFFSET + PLAT_MEM_SIZE - (DEFAULT_SUNXI_ION_RESERVE_SIZE << 20),
};

u32 ion_carveout_init[ION_LEVEL_MAX][ION_CARVEOUT_INIT_MAX];
u32 ion_cma_init[ION_LEVEL_MAX][ION_CMA_INIT_MAX];
static int ion_reserve_select(void)
{
#ifdef CONFIG_ARCH_SUN8IW7P1
	u32 chipid = sunxi_smc_readl(SUNXI_SID_VBASE + 0x200);
early_printk("%s: ion chipid  [0x%x!\n", __func__, chipid);
	switch (chipid & 0x0ff) {
		case 0x42:	/* H2+ */
		case 0x83:
			return 1;
		case 0x00:      /* H3 */
		case 0x81:
			return 0;
		default:	/* H3 */
			return 0;
		}
#else
	return 0;
#endif
}
static int __init ion_reserve_common(char *p, int is_cma,int force)
{
       char *endp;
       char* startp=p;
       char* endedp = p + strlen(p) - 1;
       int cur=0,select=0;
       int i =0,level=0;
       u32 size;
       u32 ion_init_max = is_cma?ION_CMA_INIT_MAX:ION_CARVEOUT_INIT_MAX;
       u32 (*ion_reserve) [ION_CMA_INIT_MAX] = is_cma?ion_cma_init:ion_carveout_init;

       early_printk("ion_%s reserve:",is_cma?"cma":"carveout");
	do{
		size = (u32)memparse(startp, &endp);
		if(*endp == '@')
		{
			switch (*(endp+1))
			{
			case '0':
				cur = 0;
				break;
			case '1':
				cur = 1;
				break;
			case '2':
				cur = 2;
				break;
			default:
				cur = 0;
				break;
			}
		}
		if(cur != level)
		{
		    level = cur;
		    i = 0;
		}
		ion_reserve[level][i] = size;
		early_printk(" %um@%u",ion_reserve[level][i] >>20,level);
		i++;
		startp = endp + 3;
	}while( i < ion_init_max && startp < endedp);
	early_printk("\n");
	if((IS_ENABLED(CONFIG_CMA) && is_cma) || ((!IS_ENABLED(CONFIG_CMA) || force) && !is_cma))
	{
		select = ion_reserve_select();
		i = (mem_size<=SZ_512M)?0:1;
		if(ion_reserve[select][i])
		{
			ion_mem.size = ion_reserve[select][i];
			ion_mem.start = (mem_size==SZ_1G)?(mem_start + SZ_512M - ion_mem.size):(mem_start + mem_size - ion_mem.size);
		}
		early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
	}
       return 0;
}
static int __init early_ion_carveout_list(char *p)
{
	return ion_reserve_common(p,0,0);
}
static int __init early_ion_cma_list(char *p)
{
	return ion_reserve_common(p,1,0);
}
early_param("ion_carveout_list", early_ion_carveout_list);
early_param("ion_cma_list", early_ion_cma_list);
#endif

#ifndef CONFIG_OF
#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase        = (void __iomem *)(SUNXI_UART0_VBASE),
		.mapbase        = (resource_size_t)SUNXI_UART0_PBASE,
		.irq            = SUNXI_IRQ_UART0,
		.flags          = UPF_BOOT_AUTOCONF|UPF_IOREMAP,
		.iotype         = UPIO_MEM32,
		.regshift       = 2,
		.uartclk        = 24000000,
	}, {
		.flags          = 0,
	}
 };

static struct platform_device serial_dev = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &serial_platform_data[0],
	}
};
#endif

#if defined(CONFIG_CPU_HAS_PMU)
/* cpu performance support */
#if (defined(CONFIG_ARCH_SUN8IW6) || defined(CONFIG_ARCH_SUN8IW9)) && defined(CONFIG_EVB_PLATFORM)
static struct resource sunxi_pmu_res[] = {
	{
		.start		= SUNXI_IRQ_C0PMU0,
		.end		= SUNXI_IRQ_C0PMU3,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= SUNXI_IRQ_C1PMU0,
		.end		= SUNXI_IRQ_C1PMU3,
		.flags		= IORESOURCE_IRQ,
	}
};
#else
static struct resource sunxi_pmu_res[] = {
	{
#if defined(CONFIG_ARCH_SUN8I) && defined(CONFIG_EVB_PLATFORM)
	.start  = SUNXI_IRQ_PMU0,
	.end    = SUNXI_IRQ_PMU3,
#else
	.start  = SUNXI_IRQ_PMU,
	.end    = SUNXI_IRQ_PMU,
#endif
	.flags  = IORESOURCE_IRQ,
	}
};
#endif

static struct platform_device sunxi_pmu_dev = {
	.name   = "arm-pmu",
	.id     = ARM_PMU_DEVICE_CPU,
#if defined(CONFIG_ARCH_SUN8IW6) || defined(CONFIG_ARCH_SUN8IW9)
	.num_resources = 2,
	.resource = sunxi_pmu_res,
#else
	.num_resources = 1,
	.resource = sunxi_pmu_res,
#endif
};
#endif

static struct platform_device *sunxi_dev[] __initdata = {
#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
	&serial_dev,
#endif
#if defined(CONFIG_CPU_HAS_PMU)
	&sunxi_pmu_dev,
#endif
};
#endif

static void sun8i_restart(char mode, const char *cmd)
{
#ifndef CONFIG_ARCH_SUN8IW8
	sunxi_smc_writel(0, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_IRQ_EN_REG));
	sunxi_smc_writel(0x01, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_CFG_REG));
	sunxi_smc_writel(0x01, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_MODE_REG));
#else
	writel(0x0, (void __iomem *)(SUNXI_TIMER_VBASE + 0xA0));
	writel(1, (void __iomem *)(SUNXI_TIMER_VBASE + 0xB4));
	writel((0x3 << 4), (void __iomem *)(SUNXI_TIMER_VBASE + 0xB8));
	writel(0x01, (void __iomem *)(SUNXI_TIMER_VBASE + 0xB8));
#endif
	while(1);
}

static struct map_desc sunxi_io_desc[] __initdata = {
	{
		(u32)SUNXI_IO_VBASE,      __phys_to_pfn(SUNXI_IO_PBASE),
		SUNXI_IO_SIZE, MT_DEVICE
	},
	{
		(u32)SUNXI_SRAM_A1_VBASE, __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		SUNXI_SRAM_A1_SIZE, MT_MEMORY_ITCM
	},
	{
		(u32)SUNXI_SRAM_A2_VBASE, __phys_to_pfn(SUNXI_SRAM_A2_PBASE),
		SUNXI_SRAM_A2_SIZE, MT_DEVICE_NONSHARED
	},
#ifdef CONFIG_ARCH_SUN8IW3P1
	{
		(u32)SUNXI_SRAM_VE_VBASE, __phys_to_pfn(SUNXI_SRAM_VE_PBASE),
		SUNXI_SRAM_VE_SIZE, MT_DEVICE
	},
#endif
#ifdef CONFIG_SUNXI_HW_READ
#ifdef CONFIG_ARCH_SUN8IW6
	{
        (u32)SUNXI_BROM1_S_VBASE,  __phys_to_pfn(SUNXI_BROM1_S_PBASE),
        SUNXI_BROM1_S_SIZE,       MT_DEVICE
    },
#else
	{
		(u32)SUNXI_BROM_VBASE,  __phys_to_pfn(SUNXI_BROM_PBASE),
		SUNXI_BROM_SIZE,    MT_DEVICE
	},
#endif
#endif
};

static void __init sun8i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
#ifdef CONFIG_EVB_PLATFORM
	struct tag *t;

	for (t = tags; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_MEM && t->u.mem.size) {
#ifdef CONFIG_MEM_512M_DEBUG
#warning "THIS IS JUST FOR TEST/DEBUG!!!"
#warning "DO NOT ENABLE THIS CONFIG IN RELEASE VERSION!!!"
			early_printk("[%s]: actual mem size %dMB, force to 512MB\n",
					__func__, t->u.mem.size >> 20);
			t->u.mem.size = 512 << 20;
#endif
			early_printk("[%s]: From boot, get meminfo:\n"
					"\tStart:\t0x%08x\n"
					"\tSize:\t%dMB\n",
					__func__,
					t->u.mem.start,
					t->u.mem.size >> 20);
			mem_start = t->u.mem.start;
			mem_size = t->u.mem.size;
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
			ion_reserve_common(CONFIG_ION_SUNXI_RESERVE_LIST,0,1);
#endif
			return;
		}
	}
#endif

	early_printk("[%s] enter\n", __func__);

	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = PLAT_MEM_SIZE;
	meminfo->nr_banks = 1;

	early_printk("nr_banks: %d, bank.start: 0x%08x, bank.size: 0x%08x\n",
			meminfo->nr_banks, meminfo->bank[0].start,
			(unsigned int)meminfo->bank[0].size);
}

void __init sun8i_reserve(void)
{
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	/* ram console persistent ram init*/
	ram_console_persistent_ram_init();
#endif
	/* reserve for sys_config */
	memblock_reserve(SYS_CONFIG_MEMBASE, sys_config_size);

	/* reserve for standby */
	memblock_reserve(SUPER_STANDBY_MEM_BASE, SUPER_STANDBY_MEM_SIZE);

	/* reserve for arisc */
#if (defined CONFIG_ARCH_SUN8IW6P1)
	memblock_reserve(ARISC_RESERVE_MEMBASE, ARISC_RESERVE_MEMSIZE);
#endif

#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
#ifndef CONFIG_CMA
	memblock_reserve(ion_mem.start, ion_mem.size);
#endif
#endif
}

static int __init config_size_init(char *str)
{
	int config_size;

	if (get_option(&str, &config_size)) {
		if ((config_size >= SZ_32K) && (config_size <= SZ_512K))
			sys_config_size = ALIGN(config_size, PAGE_SIZE);
		return 0;
	}

	printk("get config_size error\n");
	return -EINVAL;

	return 0;
}
early_param("config_size", config_size_init);

#ifndef CONFIG_OF
static void __init sun8i_gic_init(void)
{
	gic_init(0, 29, (void __iomem *)SUNXI_GIC_DIST_VBASE, (void __iomem *)SUNXI_GIC_CPU_VBASE);
}
#endif

extern void __init sunxi_firmware_init(void);
void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
#ifdef CONFIG_SUNXI_TRUSTZONE
 	sunxi_firmware_init();
#endif
	/* detect sunxi soc ver */
	sunxi_soc_ver_init();
#if !defined(CONFIG_SUNXI_ARISC)
	sunxi_chip_id_init();
#endif
}

static void __init sunxi_dev_init(void)
{
#ifdef CONFIG_OF
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
#else
	platform_add_devices(sunxi_dev, ARRAY_SIZE(sunxi_dev));
#endif
#if defined(CONFIG_SENSORS_INA219)
	/* ina219 use i2c-1 */
	if (i2c_register_board_info(1, i2c_ina219_devs, ARRAY_SIZE(i2c_ina219_devs)) < 0) {
		printk("%s()%d - INA219 init failed!\n", __func__, __LINE__);
	}
	printk("ina219 device registered\n");
#endif

#if defined(CONFIG_INPUT_ADXL34X_I2C_MODULE)
	printk("plat: add adxl34x device\n");
	i2c_register_board_info(ADXL34X_I2C_BUS, &adxl34x_i2c_bdi, 1);
#endif

#if defined(CONFIG_BMP085_MODULE)
	printk("plat: add bmp085 device\n");
	i2c_register_board_info(BMP085_I2C_BUS, &bmp085_i2c_bdi, 1);
#endif

#if defined(CONFIG_RTC_DRV_DS1307_MODULE)
	printk("plat: add ds1307 device\n");
	i2c_register_board_info(DS1307_I2C_BUS, &ds1307_i2c_bdi, 1);
#endif

#if defined(CONFIG_SENSORS_PCF8591_MODULE)
	printk("plat: add pcf8591 device\n");
	i2c_register_board_info(PCF8591_I2C_BUS, &pcf8591_i2c_bdi, 1);
#endif	

#ifdef CONFIG_ANDROID_RAM_CONSOLE
	/*	ram console	platform device initialize*/
	ram_console_device_init();
#endif

}

extern void __init sunxi_init_clocks(void);
#ifdef CONFIG_ARM_ARCH_TIMER
struct arch_timer sun8i_arch_timer __initdata = {
	.res[0] = {
		.start = 29,
		.end = 29,
		.flags = IORESOURCE_IRQ,
	},
	.res[1] = {
		.start = 30,
		.end = 30,
		.flags = IORESOURCE_IRQ,
	},
};
#endif

extern void sunxi_timer_init(void);
static void __init sun8i_timer_init(void)
{
	sunxi_init_clocks();
#if (defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1))
	if(!(readl(IO_ADDRESS(SUNXI_TIMESTAMP_CTRL_PBASE)) & 0x01))
		writel(readl(IO_ADDRESS(SUNXI_TIMESTAMP_CTRL_PBASE)) |
				0x01,IO_ADDRESS(SUNXI_TIMESTAMP_CTRL_PBASE));
#endif

#ifdef CONFIG_SUNXI_TIMER
	sunxi_timer_init();
#endif

#ifdef CONFIG_ARM_ARCH_TIMER
	arch_timer_register(&sun8i_arch_timer);
	arch_timer_sched_clock_init();
#endif
}

struct sys_timer sunxi_timer __initdata = {
	.init = sun8i_timer_init,
};

#ifdef CONFIG_SMP
#if defined(CONFIG_ARCH_SUN8IW6) || defined(CONFIG_ARCH_SUN8IW9)
extern bool __init sun8i_smp_init_ops(void);
#endif
#endif

void __init sunxi_init_early(void)
{
#ifdef CONFIG_SUNXI_CONSISTENT_DMA_SIZE
	init_consistent_dma_size(CONFIG_SUNXI_CONSISTENT_DMA_SIZE << 20);
#endif
}

MACHINE_START(SUNXI, "sun8i")
	.atag_offset	= 0x100,
	.init_machine	= sunxi_dev_init,
	.init_early     = sunxi_init_early,
	.map_io		= sunxi_map_io,
#ifndef CONFIG_OF
	.init_irq	= sun8i_gic_init,
#endif
	.handle_irq	= gic_handle_irq,
	.restart	= sun8i_restart,
	.timer		= &sunxi_timer,
	.dt_compat	= NULL,
	.reserve	= sun8i_reserve,
	.fixup		= sun8i_fixup,
	.nr_irqs	= NR_IRQS,
#ifdef CONFIG_SMP
	.smp		= smp_ops(sunxi_smp_ops),
#if defined(CONFIG_ARCH_SUN8IW6) || defined(CONFIG_ARCH_SUN8IW9)
	.smp_init	= smp_init_ops(sun8i_smp_init_ops),
#endif
#endif
MACHINE_END
