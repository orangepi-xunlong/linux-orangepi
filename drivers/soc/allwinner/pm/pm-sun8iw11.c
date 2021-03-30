/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include "pm_o.h"

static struct clk_state saved_clk_state;
static struct gpio_state saved_gpio_state;
static struct ccm_state saved_ccm_state;
static struct sram_state saved_sram_state;
static void mem_enable_nmi(void);

#define SUNXI_IRQ_NMI_ADDR		(0xf1c00038)

static void mem_enable_nmi(void)
{
	u32 tmp = 0;

	tmp = readl((volatile void *)SUNXI_IRQ_NMI_ADDR);
	tmp |= ((0x0000001));
	writel(tmp, (volatile void *)SUNXI_IRQ_NMI_ADDR);

	return;
}

void init_wakeup_src(unsigned int event, unsigned int gpio_enable_bitmap, unsigned int cpux_gpiog_bitmap)
{
	/* config int src. */
	/* initialise standby modules */
	if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY)) {
		/* don't need init serial ,depend kernel?
		 *serial_init(0);
		 */
		pr_info("final standby wakeup src config = 0x%x.\n", event);
	}

	/* init some system wake source */
	if (event & CPU0_WAKEUP_MSGBOX) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY)) {
			/* for tips info */
			pr_info("enable CPU0_WAKEUP_MSGBOX.\n");
		}
		mem_enable_int(INT_SOURCE_MSG_BOX);
	}

	if (event & CPU0_WAKEUP_EXINT) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPU0_WAKEUP_EXINT.\n");
		mem_enable_int(INT_SOURCE_EXTNMI);
		mem_enable_nmi();
	}

	if (event & CPU0_WAKEUP_TIMEOUT) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPU0_WAKEUP_TIMEOUT.\n");
		/* set timer for power off */
		if (standby_info.standby_para.timeout) {
			pr_info("wakeup sys in %d sec later.\n",
					standby_info.standby_para.timeout);
			mem_tmr_set(standby_info.standby_para.timeout);
			mem_enable_int(INT_SOURCE_TIMER0);
		}
	}

	if (event & CPU0_WAKEUP_ALARM) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPU0_WAKEUP_ALARM.\n");
		mem_enable_int(INT_SOURCE_ALARM);
	}

	if (event & CPU0_WAKEUP_KEY) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPU0_WAKEUP_KEY.\n");
		mem_key_init();
		mem_enable_int(INT_SOURCE_LRADC);
	}
	if (event & CPU0_WAKEUP_IR) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPU0_WAKEUP_IR.\n");
		mem_ir_init();
		mem_enable_int(INT_SOURCE_IR0);
		mem_enable_int(INT_SOURCE_IR1);
	}

	if (event & CPU0_WAKEUP_USB) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPU0_WAKEUP_USB.\n");
		mem_usb_init();
		mem_enable_int(INT_SOURCE_USBOTG);
		mem_enable_int(INT_SOURCE_USBEHCI0);
		mem_enable_int(INT_SOURCE_USBEHCI1);
		mem_enable_int(INT_SOURCE_USBEHCI2);
		mem_enable_int(INT_SOURCE_USBOHCI0);
		mem_enable_int(INT_SOURCE_USBOHCI1);
		mem_enable_int(INT_SOURCE_USBOHCI2);
	}

	if (event & CPUS_WAKEUP_GPIO) {
		if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY))
			pr_info("enable CPUS_WAKEUP_GPIO.\n");
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('A')))
			mem_enable_int(INT_SOURCE_GPIOA);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('B')))
			mem_enable_int(INT_SOURCE_GPIOB);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('C')))
			mem_enable_int(INT_SOURCE_GPIOC);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('D')))
			mem_enable_int(INT_SOURCE_GPIOD);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('E')))
			mem_enable_int(INT_SOURCE_GPIOE);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('F')))
			mem_enable_int(INT_SOURCE_GPIOF);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('G')))
			mem_enable_int(INT_SOURCE_GPIOG);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('H')))
			mem_enable_int(INT_SOURCE_GPIOH);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('I')))
			mem_enable_int(INT_SOURCE_GPIOI);
		if (cpux_gpiog_bitmap & (WAKEUP_GPIO_GROUP('J')))
			mem_enable_int(INT_SOURCE_GPIOJ);
		mem_pio_clk_src_init();
	}
	return;
}

void exit_wakeup_src(unsigned int event, unsigned int gpio_enable_bitmap, unsigned int cpux_gpiog_bitmap)
{
	/* exit standby module */
	if (event & CPUS_WAKEUP_GPIO)
		mem_pio_clk_src_exit();

	if (event & CPU0_WAKEUP_USB)
		mem_usb_exit();

	if (event & CPU0_WAKEUP_IR)
		mem_ir_exit();

	if (event & CPU0_WAKEUP_ALARM)
		;

	if (event & CPU0_WAKEUP_KEY)
		mem_key_exit();

	/* exit standby module */
	if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY)) {
		/* restore serial clk & gpio config.
		 * serial_exit();
		 */
	}
	return;
}

void mem_device_init(void)
{
	mem_tmr_init();
	mem_gpio_init();
	mem_sram_init();
	mem_int_init();
	mem_clk_init(1);

	return;
}

void mem_device_save(void)
{
	/* backup device state */
	mem_twi_save(0);
	mem_ccu_save(&(saved_ccm_state));
	mem_clk_save(&(saved_clk_state));
	mem_tmr_save(&(saved_tmr_state));
	mem_gpio_save(&(saved_gpio_state));
	mem_sram_save(&(saved_sram_state));
	mem_int_save();
#ifdef CONFIG_AW_AXP
/*	axp_mem_save();*/
#endif

	return;
}

void mem_device_restore(void)
{
#ifdef CONFIG_AW_AXP
/*	axp_mem_restore();*/
#endif
	mem_sram_restore(&(saved_sram_state));
	mem_gpio_restore(&(saved_gpio_state));
	mem_tmr_restore(&(saved_tmr_state));
	mem_clk_restore(&(saved_clk_state));
	mem_ccu_restore(&(saved_ccm_state));
	mem_int_restore();
	mem_tmr_exit();
	mem_twi_restore();

	return;
}

/**
 * aw_standby_enter() - Enter the system sleep state
 *
 * @state: suspend state
 * @return: return 0 is process successed
 * @note: the core function for platform sleep
 */
int aw_standby_enter(unsigned long arg)
{
	struct aw_pm_info *para = (struct aw_pm_info *)(arg);
	int ret = -1;
	int (*standby) (struct aw_pm_info *arg);

	/* clean d cache to the point of unification.
	 * __cpuc_coherent_kern_range(0xc0000000, 0xffffffff-1);
	 */

	/* move standby code to sram */
	memcpy((void *)SRAM_FUNC_START, (void *)&standby_bin_start,
	       (unsigned int)&standby_bin_end -
	       (unsigned int)&standby_bin_start);

	dmac_flush_range((void *)SRAM_FUNC_START,
			(void *)(SRAM_FUNC_START + ((unsigned int)&standby_bin_end - (unsigned int)&standby_bin_start)));
	/* flush cache data to the memory.
	 * dmac_flush_range((void *)0xc0000000, (void *)(0xdfffffff-1));
	 */

	/* clean & invalidate dcache, icache. */
	flush_cache_all();
	standby = (int (*)(struct aw_pm_info *arg))SRAM_FUNC_START;

	ret = standby(para);
	if (0 == ret)
		soft_restart(virt_to_phys(cpu_resume));

	return ret;
}

void query_wakeup_source(struct aw_pm_info *arg)
{
	arg->standby_para.event = 0;

	arg->standby_para.event |=
	    ((mem_query_int(INT_SOURCE_EXTNMI)) ? 0 : CPU0_WAKEUP_EXINT);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBOTG) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBEHCI0) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBEHCI1) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBEHCI2) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBOHCI0) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBOHCI1) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    (mem_query_int(INT_SOURCE_USBOHCI2) ? 0 : CPU0_WAKEUP_USB);
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_LRADC) ? 0 : CPU0_WAKEUP_KEY;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_IR0) ? 0 : CPU0_WAKEUP_IR;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_ALARM) ? 0 : CPU0_WAKEUP_ALARM;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_TIMER0) ? 0 : CPU0_WAKEUP_TIMEOUT;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOA) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOB) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOC) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOD) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOE) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOF) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOG) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOH) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOI) ? 0 : CPUS_WAKEUP_GPIO;
	arg->standby_para.event |=
	    mem_query_int(INT_SOURCE_GPIOJ) ? 0 : CPUS_WAKEUP_GPIO;
}

int fetch_and_save_dram_para(dram_para_t *pstandby_dram_para)
{
	struct device_node *np;
	s32 ret = -EINVAL;

	np = of_find_compatible_node(NULL, NULL, "allwinner,dram");
	if (IS_ERR(np)) {
		pr_err("get [allwinner, dram] device node error\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_clk",
				   &pstandby_dram_para->dram_clk);
	if (ret) {
		pr_err("standby :get dram_clk err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_type",
				   &pstandby_dram_para->dram_type);
	if (ret) {
		pr_err("standby :get dram_type err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_zq", &pstandby_dram_para->dram_zq);
	if (ret) {
		pr_err("standby :get dram_zq err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_odt_en",
				   &pstandby_dram_para->dram_odt_en);
	if (ret) {
		pr_err("standby :get dram_odt_en err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_para1",
				   &pstandby_dram_para->dram_para1);
	if (ret) {
		pr_err("standby :get dram_para1 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_para2",
				   &pstandby_dram_para->dram_para2);
	if (ret) {
		pr_err("standby :get dram_para2 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_mr0",
				   &pstandby_dram_para->dram_mr0);
	if (ret) {
		pr_err("standby :get dram_mr0 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_mr1",
				   &pstandby_dram_para->dram_mr1);
	if (ret) {
		pr_err("standby :get dram_mr1 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_mr2",
				   &pstandby_dram_para->dram_mr2);
	if (ret) {
		pr_err("standby :get dram_mr2 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_mr3",
				   &pstandby_dram_para->dram_mr3);
	if (ret) {
		pr_err("standby :get dram_mr3 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr0",
				   &pstandby_dram_para->dram_tpr0);
	if (ret) {
		pr_err("standby :get dram_tpr0 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr1",
				   &pstandby_dram_para->dram_tpr1);
	if (ret) {
		pr_err("standby :get dram_tpr1 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr2",
				   &pstandby_dram_para->dram_tpr2);
	if (ret) {
		pr_err("standby :get dram_tpr2 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr3",
				   &pstandby_dram_para->dram_tpr3);
	if (ret) {
		pr_err("standby :get dram_tpr3 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr4",
				   &pstandby_dram_para->dram_tpr4);
	if (ret) {
		pr_err("standby :get dram_tpr4 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr5",
				   &pstandby_dram_para->dram_tpr5);
	if (ret) {
		pr_err("standby :get dram_tpr5 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr6",
				   &pstandby_dram_para->dram_tpr6);
	if (ret) {
		pr_err("standby :get dram_tpr6 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr7",
				   &pstandby_dram_para->dram_tpr7);
	if (ret) {
		pr_err("standby :get dram_tpr7 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr8",
				   &pstandby_dram_para->dram_tpr8);
	if (ret) {
		pr_err("standby :get dram_tpr8 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr9",
				   &pstandby_dram_para->dram_tpr9);
	if (ret) {
		pr_err("standby :get dram_tpr9 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr9",
				   &pstandby_dram_para->dram_tpr9);
	if (ret) {
		pr_err("standby :get dram_tpr9 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr10",
				   &pstandby_dram_para->dram_tpr10);
	if (ret) {
		pr_err("standby :get dram_tpr10 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr11",
				   &pstandby_dram_para->dram_tpr11);
	if (ret) {
		pr_err("standby :get dram_tpr11 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr12",
				   &pstandby_dram_para->dram_tpr12);
	if (ret) {
		pr_err("standby :get dram_tpr12 err.\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "dram_tpr13",
				   &pstandby_dram_para->dram_tpr13);
	if (ret) {
		pr_err("standby :get dram_tpr13 err.\n");
		return -EINVAL;
	}

	return ret;
}
