/*
 * arch/arm/mach-sunxi/sun9i.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sun9i platform file
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

#include <asm/arch_timer.h>
#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/sunxi-chip.h>
#include <mach/sunxi-smc.h>
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
#ifdef CONFIG_DRAM_TRAINING_RESERVE_MEM
phys_addr_t dramfreq_mem_size = 0;
phys_addr_t dramfreq_mem_size_mb = 0;
#endif
static unsigned int sys_config_size = SYS_CONFIG_MEMSIZE;

#if defined(CONFIG_SENSORS_INA219)
static struct i2c_board_info i2c_ina219_devs[] __initdata = {
	{ I2C_BOARD_INFO("ina219_sn3v", 0x40), },
	{ I2C_BOARD_INFO("ina219_gpu",  0x41), },
	{ I2C_BOARD_INFO("ina219_cpua", 0x42), },
	{ I2C_BOARD_INFO("ina219_sys",  0x43), },
	{ I2C_BOARD_INFO("ina219_dram", 0x44), },
	{ I2C_BOARD_INFO("ina219_cpub", 0x45), },
	{ I2C_BOARD_INFO("ina219_vpu",  0x46), },
	{ I2C_BOARD_INFO("ina219_audi", 0x47), },
};
#endif
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
#define DEFAULT_SUNXI_ION_RESERVE_SIZE	96
#define ION_CARVEOUT_INIT_MAX	4
#define ION_CMA_INIT_MAX	4
struct tag_mem32 ion_mem = { /* the real ion reserve info */
       .size  = DEFAULT_SUNXI_ION_RESERVE_SIZE << 20,
       .start = PLAT_PHYS_OFFSET + PLAT_MEM_SIZE - (DEFAULT_SUNXI_ION_RESERVE_SIZE << 20),
};

u32 ion_carveout_init[ION_CARVEOUT_INIT_MAX];
u32 ion_cma_init[ION_CMA_INIT_MAX];
static int __init ion_reserve_common(char *p, int is_cma,int force)
{
       char *endp;
       char* startp=p;
       int i =0;
       u32 ion_init_max = is_cma?ION_CMA_INIT_MAX:ION_CARVEOUT_INIT_MAX;
       u32* ion_reserve = is_cma?ion_cma_init:ion_carveout_init;

       early_printk("ion_%s reserve:",is_cma?"cma":"carveout");
	do{
		ion_reserve[i] = (u32)memparse(startp, &endp);
		early_printk(" %um",ion_reserve[i] >>20);
		startp = endp+1;
		i++;
	}while( i < ion_init_max && *endp == ',');
	early_printk("\n");
	if((IS_ENABLED(CONFIG_CMA) && is_cma) || ((!IS_ENABLED(CONFIG_CMA) || force) && !is_cma))
		for(i=0;i<ion_init_max;i++)
		{
		       if(ion_reserve[i])
		       {
			    ion_mem.size = ion_reserve[i];
			    ion_mem.start = mem_start + mem_size - ion_mem.size;
		       }
		       else
		       {
		            early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
			    break;
		       }

		       if (mem_size <= (SZ_512M <<i))
		       {
		            early_printk("%s: ion reserve: [0x%x, 0x%x]!\n", __func__, (int)ion_mem.start, (int)(ion_mem.start + ion_mem.size));
			    break;
		       }
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

static struct platform_device *sun9i_dev[] __initdata = {
#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
	&serial_dev,
#endif
};
#endif


static void sun9i_restart(char mode, const char *cmd)
{
	sunxi_smc_writel(0, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_IRQ_EN_REG));
	sunxi_smc_writel(0x01, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_CFG_REG));
	sunxi_smc_writel(0x01, (void __iomem *)(SUNXI_R_WDOG_VBASE + R_WDOG_MODE_REG));
	while(1);
}

static struct map_desc sun9i_io_desc[] __initdata = {
	/* not need map brom0_n, because it share address range with brom1_s */
	{(u32)SUNXI_BROM1_S_VBASE,       __phys_to_pfn(SUNXI_BROM1_S_PBASE),       SUNXI_BROM1_S_SIZE,       MT_DEVICE},
	{(u32)SUNXI_SRAM_A1_VBASE,       __phys_to_pfn(SUNXI_SRAM_A1_PBASE),       SUNXI_SRAM_A1_SIZE,       MT_MEMORY_ITCM},
	{(u32)SUNXI_SRAM_A2_VBASE,       __phys_to_pfn(SUNXI_SRAM_A2_PBASE),       SUNXI_SRAM_A2_SIZE,       MT_DEVICE},
	{(u32)SUNXI_SRAM_B_VBASE,        __phys_to_pfn(SUNXI_SRAM_B_PBASE),        SUNXI_SRAM_B_SIZE,        MT_DEVICE},
	{(u32)SUNXI_SRAM_C_VBASE,        __phys_to_pfn(SUNXI_SRAM_C_PBASE),        SUNXI_SRAM_C_SIZE,        MT_DEVICE},
	{(u32)SUNXI_IO_CORE_DEBUG_VBASE, __phys_to_pfn(SUNXI_IO_CORE_DEBUG_PBASE), SUNXI_IO_CORE_DEBUG_SIZE, MT_DEVICE},
	{(u32)SUNXI_IO_TSGEN_RO_VBASE,   __phys_to_pfn(SUNXI_IO_TSGEN_RO_PBASE),   SUNXI_IO_TSGEN_RO_SIZE,   MT_DEVICE},
	{(u32)SUNXI_IO_TSGEN_CTRL_VBASE, __phys_to_pfn(SUNXI_IO_TSGEN_CTRL_PBASE), SUNXI_IO_TSGEN_CTRL_SIZE, MT_DEVICE},
	{(u32)SUNXI_IO_R_CPUCFG_VBASE,   __phys_to_pfn(SUNXI_IO_R_CPUCFG_PBASE),   SUNXI_IO_R_CPUCFG_SIZE,   MT_DEVICE},
	{(u32)SUNXI_IO_TIMESTAMP_VBASE,  __phys_to_pfn(SUNXI_IO_TIMESTAMP_PBASE),  SUNXI_IO_TIMESTAMP_SIZE,  MT_DEVICE},
	{(u32)SUNXI_IO_AHB0_VBASE,       __phys_to_pfn(SUNXI_IO_AHB0_PBASE),       SUNXI_IO_AHB0_SIZE,       MT_DEVICE},
#ifndef CONFIG_ARM_LPAE
	{(u32)SUNXI_IO_AHB1_VBASE,       __phys_to_pfn(SUNXI_IO_AHB1_PBASE),       SUNXI_IO_AHB1_SIZE,       MT_DEVICE},
#else
	{(u32)SUNXI_IO_AHB1_VBASE,          __phys_to_pfn(SUNXI_IO_AHB1_PBASE),  SUNXI_IO_AHB1_SIZE-0x100000,MT_DEVICE},
	{(u32)SUNXI_IO_AHB1_VBASE+0x200000, __phys_to_pfn(SUNXI_IO_AHB1_PBASE+0x200000),            0x100000,MT_DEVICE},
#endif
	{(u32)SUNXI_IO_AHB2_VBASE,       __phys_to_pfn(SUNXI_IO_AHB2_PBASE),       SUNXI_IO_AHB2_SIZE,       MT_DEVICE},
	{(u32)SUNXI_IO_APB0_VBASE,       __phys_to_pfn(SUNXI_IO_APB0_PBASE),       SUNXI_IO_APB0_SIZE,       MT_DEVICE},
	{(u32)SUNXI_IO_APB1_VBASE,       __phys_to_pfn(SUNXI_IO_APB1_PBASE),       SUNXI_IO_APB1_SIZE,       MT_DEVICE},
	{(u32)SUNXI_IO_CPUS_VBASE,       __phys_to_pfn(SUNXI_IO_CPUS_PBASE),       SUNXI_IO_CPUS_SIZE,       MT_DEVICE},
};

static void __init sun9i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
#ifdef CONFIG_EVB_PLATFORM
	struct tag *t;
    phys_addr_t dram_size_from_boot=0;

	for (t = tags; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_MEM && t->u.mem.size) {
            if( (t->u.mem.size) && (t->u.mem.size <=16384) && t->u.mem.start)
            {
		dramfreq_mem_size_mb = t->u.mem.size;
#ifdef CONFIG_ARM_LPAE            
                dram_size_from_boot = ((phys_addr_t)t->u.mem.size*1024*1024);
#else
                t->u.mem.size = (t->u.mem.size >=4096)?(4095 - PLAT_PHYS_OFFSET/0x100000):t->u.mem.size;
                dram_size_from_boot = ((phys_addr_t)t->u.mem.size*1024*1024);                
#endif                
                t->u.mem.size=0;
                continue;                   
            }
			early_printk("[%s]: From boot, get meminfo:\n"
					"\tStart:\t0x%08x\n"
					"\tSize:\t%dMB\n",
					__func__,
					t->u.mem.start,
					t->u.mem.size >> 20);
#ifdef CONFIG_DRAM_TRAINING_RESERVE_MEM
			dramfreq_mem_size = t->u.mem.size;
			dramfreq_mem_size_mb = t->u.mem.size >>20;
#endif
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
			ion_reserve_common(CONFIG_ION_SUNXI_RESERVE_LIST,0,1);
#endif
			return;
		}
	}
#endif

    dram_size_from_boot = dram_size_from_boot?dram_size_from_boot:PLAT_MEM_SIZE;
	meminfo->bank[0].start = (phys_addr_t)PLAT_PHYS_OFFSET;
	meminfo->bank[0].size =  dram_size_from_boot;
#ifdef CONFIG_DRAM_TRAINING_RESERVE_MEM
    dramfreq_mem_size = dram_size_from_boot;
	dramfreq_mem_size_mb = dramfreq_mem_size_mb?dramfreq_mem_size_mb:(dramfreq_mem_size>>20);
#endif
	meminfo->nr_banks = 1;
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
	ion_reserve_common(CONFIG_ION_SUNXI_RESERVE_LIST,0,1);
#endif

#ifdef CONFIG_ARM_LPAE
	early_printk("nr_banks: %d, bank.start: 0x%llx, bank.size: 0x%llx\n",
			meminfo->nr_banks, meminfo->bank[0].start,
			meminfo->bank[0].size);
#else
	early_printk("nr_banks: %d, bank.start: 0x%08x, bank.size: 0x%08x\n",
			meminfo->nr_banks, meminfo->bank[0].start,
			(unsigned int)meminfo->bank[0].size);
#endif
}

void __init sun9i_reserve(void)
{
	phys_addr_t mem_phyaddr = PLAT_PHYS_OFFSET >>20;
	/* add any reserve memory to here */
#ifdef CONFIG_ANDROID_RAM_CONSOLE
       /* ram console persistent ram init*/
       ram_console_persistent_ram_init();
#endif
#ifdef CONFIG_DRAM_TRAINING_RESERVE_MEM
	memblock_reserve(PLAT_PHYS_OFFSET, SZ_4K);
	if (dramfreq_mem_size_mb == (dramfreq_mem_size >>20)
		|| (mem_phyaddr + (dramfreq_mem_size_mb >>1)) >= 4096
		|| (dramfreq_mem_size_mb >>1) >= (dramfreq_mem_size >>20))
		memblock_reserve(PLAT_PHYS_OFFSET + (dramfreq_mem_size >> 1), SZ_4K);
	else
		memblock_reserve(PLAT_PHYS_OFFSET + (dramfreq_mem_size_mb <<19), SZ_4K);
#endif
	/* reserve for sys_config */
	memblock_reserve(SYS_CONFIG_MEMBASE, sys_config_size);

	/* reserve for standby */
	memblock_reserve(SUPER_STANDBY_MEM_BASE, SUPER_STANDBY_MEM_SIZE);
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

extern void __init sunxi_init_clocks(void);
#ifdef CONFIG_ARM_ARCH_TIMER
struct arch_timer sun9i_arch_timer __initdata = {
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
static void __init sun9i_timer_init(void)
{
	sunxi_init_clocks();

#ifdef CONFIG_SUNXI_TIMER
	sunxi_timer_init();
#endif

#ifdef CONFIG_ARM_ARCH_TIMER
	arch_timer_register(&sun9i_arch_timer);
	arch_timer_sched_clock_init();
#endif
}

struct sys_timer sun9i_timer __initdata = {
	.init = sun9i_timer_init,
};

#ifndef CONFIG_OF
static void __init sun9i_gic_init(void)
{
	gic_init(0, 29, (void __iomem *)SUNXI_GIC_DIST_VBASE, (void __iomem *)SUNXI_GIC_CPU_VBASE);
}
#endif

extern void __init sunxi_firmware_init(void);
void __init sun9i_map_io(void)
{
	iotable_init(sun9i_io_desc, ARRAY_SIZE(sun9i_io_desc));
#ifdef CONFIG_SUNXI_TRUSTZONE
	sunxi_firmware_init();
#endif
	/* detect sunxi soc ver */
	sunxi_soc_ver_init();
}

static void __init sun9i_dev_init(void)
{
#ifdef CONFIG_OF
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
#else
	platform_add_devices(sun9i_dev, ARRAY_SIZE(sun9i_dev));
#endif

#if defined(CONFIG_SENSORS_INA219)
	/* ina219 use i2c-3 */
	if (i2c_register_board_info(3, i2c_ina219_devs, ARRAY_SIZE(i2c_ina219_devs)) < 0) {
		printk("%s()%d - INA219 init failed!\n", __func__, __LINE__);
	}
	printk("ina219 device registered\n");
#endif
#ifdef CONFIG_ANDROID_RAM_CONSOLE
       /*      ram console     platform device initialize*/
       ram_console_device_init();
#endif
}

void __init sun9i_init_early(void)
{
#ifdef CONFIG_SUNXI_CONSISTENT_DMA_SIZE
	init_consistent_dma_size(CONFIG_SUNXI_CONSISTENT_DMA_SIZE << 20);
#endif
}

extern bool __init sun9i_smp_init_ops(void);
extern struct smp_operations sunxi_smp_ops;

MACHINE_START(SUNXI, "sun9i")
	.atag_offset	= 0x100,
#ifdef	CONFIG_SMP
	.smp		= smp_ops(sunxi_smp_ops),
	.smp_init	= smp_init_ops(sun9i_smp_init_ops),
#endif
	.init_machine	= sun9i_dev_init,
	.init_early     = sun9i_init_early,
	.map_io		= sun9i_map_io,
#ifndef CONFIG_OF
	.init_irq	= sun9i_gic_init,
#endif
	.handle_irq	= gic_handle_irq,
	.restart	= sun9i_restart,
	.timer		= &sun9i_timer,
	.dt_compat	= NULL,
	.reserve	= sun9i_reserve,
	.fixup		= sun9i_fixup,
	.nr_irqs	= NR_IRQS,
MACHINE_END
