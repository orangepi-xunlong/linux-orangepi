/*
 *  drivers/arisc/arisc.c
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "arisc_i.h"
#include <mach/sunxi-chip.h>
#include <asm/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/reboot.h>
#include <asm/cacheflush.h>

/* external vars */
extern char *arisc_binary_start;
extern char *arisc_binary_end;

static unsigned long arisc_sram_a2_vbase = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A2_PBASE);

static atomic_t arisc_suspend_flag;

int arisc_suspend_flag_query(void)
{
	return atomic_read(&arisc_suspend_flag);
}

static void sunxi_arisc_shutdown(struct platform_device *dev)
{
	atomic_set(&arisc_suspend_flag, 1);
	while (arisc_semaphore_used_num_query()) {
			msleep(1);
	}
}

#ifdef CONFIG_PM
static int sunxi_arisc_suspend(struct device *dev)
{
	atomic_set(&arisc_suspend_flag, 1);
	while (arisc_semaphore_used_num_query()) {
			msleep(1);
	}

	return 0;
}

static int sunxi_arisc_resume(struct device *dev)
{
	unsigned int wake_event;
	standby_info_para_t sst_info;

	atomic_set(&arisc_suspend_flag, 0);
#if defined CONFIG_ARCH_SUN8IW7P1
	arisc_cpux_ready_notify();
#endif
	arisc_query_wakeup_source(&wake_event);
	if (wake_event & CPUS_WAKEUP_POWER_EXP) {
		ARISC_LOG("power exception during standby, enable:0x%x" \
		          " expect state:0x%x, expect consumption:%dmw", \
		          arisc_powchk_back.power_state.enable, \
		          arisc_powchk_back.power_state.power_reg, \
		          arisc_powchk_back.power_state.system_power);
		arisc_query_standby_power(&sst_info);
		ARISC_LOG(" real state:0x%x, real consumption:%dmw\n", \
				sst_info.power_state.power_reg, \
				sst_info.power_state.system_power);
	}

	return 0;
}

static const struct dev_pm_ops sunxi_arisc_dev_pm_ops = {
	.suspend = sunxi_arisc_suspend,
	.resume = sunxi_arisc_resume,
};

#define SUNXI_ARISC_DEV_PM_OPS (&sunxi_arisc_dev_pm_ops)
#else
#define SUNXI_ARISC_DEV_PM_OPS NULL
#endif // CONFIG_PM

static int arisc_wait_ready(unsigned int timeout)
{
	unsigned long expire;

	expire = msecs_to_jiffies(timeout) + jiffies;

	/* wait arisc startup ready */
	while (1) {
		/*
		 * linux cpu interrupt is disable now,
		 * we should query message by hand.
		 */
		struct arisc_message *pmessage = arisc_hwmsgbox_query_message();
		if (pmessage == NULL) {
			if (time_is_before_eq_jiffies(expire)) {
				return -ETIMEDOUT;
			}
			/* try to query again */
			continue;
		}
		/* query valid message */
		if (pmessage->type == ARISC_STARTUP_NOTIFY) {
			/* check arisc software and driver version match or not */
			if (pmessage->paras[0] != ARISC_VERSIONS) {
				ARISC_ERR("arisc firmware:%d and driver "
				          "version:%d not matched\n",
				          pmessage->paras[0], ARISC_VERSIONS);
				return -EINVAL;
			} else {
				/* printf the main and sub version string */
				ARISC_LOG("arisc version: [%s]\n",
				          (char *)arisc_version_store((const void*)(&(pmessage->paras[1])), 40));
			}

			/* received arisc startup ready message */
			ARISC_INF("arisc startup ready\n");
			if ((pmessage->attr & ARISC_MESSAGE_ATTR_SOFTSYN) ||
				(pmessage->attr & ARISC_MESSAGE_ATTR_HARDSYN)) {
				/* synchronous message, just feedback it */
				ARISC_INF("arisc startup notify message feedback\n");
				pmessage->paras[0] = virt_to_phys((void *)&arisc_binary_start);
				arisc_hwmsgbox_feedback_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
			} else {
				/* asyn message, free message directly */
				ARISC_INF("arisc startup notify message free directly\n");
				arisc_message_free(pmessage);
			}
			break;
		}
		/*
		 * invalid message detected, ignore it.
		 * by sunny at 2012-7-6 18:34:38.
		 */
		ARISC_WRN("arisc startup waiting ignore message\n");
		if ((pmessage->attr & ARISC_MESSAGE_ATTR_SOFTSYN) ||
			(pmessage->attr & ARISC_MESSAGE_ATTR_HARDSYN)) {
			/* synchronous message, just feedback it */
			arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
		} else {
			/* asyn message, free message directly */
			arisc_message_free(pmessage);
		}
		/* we need waiting continue */
	}

	return 0;
}

static int  sunxi_arisc_clk_cfg(struct platform_device *pdev)
{
#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1)
	struct clk *pll5 = NULL;
	struct clk *pll6 = NULL;
	struct clk *hosc = NULL;
	struct clk *losc = NULL;

	ARISC_INF("device [%s] clk resource request enter\n", dev_name(&pdev->dev));

	/* config PLL5 for dram clk */
	pll5 = clk_get(NULL, PLL5_CLK);

	if(!pll5 || IS_ERR(pll5)){
		ARISC_ERR("try to get pll5 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pll5)) {
		ARISC_ERR("try to enable pll5 output failed!\n");
		return -EINVAL;
	}

	/* config PLL6 for cpus clk */
	pll6 = clk_get(NULL, PLL6_CLK);
	if(!pll6 || IS_ERR(pll6)){
		ARISC_ERR("try to get pll6 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pll6)) {
		ARISC_ERR("try to enable pll6 output failed!\n");
		return -EINVAL;
	}

#elif (defined CONFIG_ARCH_SUN8IW5P1)
	struct clk *pllddr0 = NULL;
	struct clk *pllddr1 = NULL;
	struct clk *pllperiph = NULL;
	struct clk *hosc = NULL;
	struct clk *losc = NULL;

	ARISC_INF("device [%s] clk resource request enter\n", dev_name(&pdev->dev));
	/* config PLL5 for dram clk */
	pllddr0 = clk_get(NULL, PLL_DDR0_CLK);
	if(!pllddr0 || IS_ERR(pllddr0)){
		ARISC_ERR("try to get pll_ddr0 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllddr0)) {
		ARISC_ERR("try to enable pll_ddr0 output failed!\n");
		return -EINVAL;
	}

	pllddr1 = clk_get(NULL, PLL_DDR1_CLK);
	if(!pllddr1 || IS_ERR(pllddr1)){
		ARISC_ERR("try to get pll_ddr1 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllddr1)) {
		ARISC_ERR("try to enable pll_ddr1 output failed!\n");
		return -EINVAL;
	}

	/* config PLL6 for cpus clk */
	pllperiph = clk_get(NULL, PLL_PERIPH_CLK);
	if(!pllperiph || IS_ERR(pllperiph)){
		ARISC_ERR("try to get pll_periph failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllperiph)) {
		ARISC_ERR("try to enable pll_periph output failed!\n");
		return -EINVAL;
	}

#elif (defined CONFIG_ARCH_SUN8IW7P1)
	struct clk *pllddr = NULL;
	struct clk *pllperiph = NULL;
	struct clk *hosc = NULL;
	struct clk *losc = NULL;

	ARISC_INF("device [%s] clk resource request enter\n", dev_name(&pdev->dev));

	/* config PLL5 for dram clk */
	pllddr = clk_get(NULL, PLL_DDR_CLK);
	if(!pllddr || IS_ERR(pllddr)){
		ARISC_ERR("try to get pll_ddr0 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllddr)) {
		ARISC_ERR("try to enable pll_ddr0 output failed!\n");
		return -EINVAL;
	}

	/* config PLL6 for cpus clk */
	pllperiph = clk_get(NULL, PLL_PERIPH0_CLK);
	if(!pllperiph || IS_ERR(pllperiph)){
		ARISC_ERR("try to get pll_periph failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllperiph)) {
		ARISC_ERR("try to enable pll_periph output failed!\n");
		return -EINVAL;
	}

#elif (defined CONFIG_ARCH_SUN8IW6P1)
	struct clk *pllddr = NULL;
	struct clk *pllperiph = NULL;
	struct clk *hosc = NULL;
	struct clk *losc = NULL;

	ARISC_INF("device [%s] clk resource request enter\n", dev_name(&pdev->dev));

	/* config PLL_DDR for dram clk */
	pllddr = clk_get(NULL, PLL_DDR_CLK);
	if(!pllddr || IS_ERR(pllddr)){
		ARISC_ERR("try to get pll_ddr failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllddr)) {
		ARISC_ERR("try to enable pll_ddr output failed!\n");
		return -EINVAL;
	}

	/* config PLL_PERIPH for cpus clk */
	pllperiph = clk_get(NULL, PLL_PERIPH_CLK);
	if(!pllperiph || IS_ERR(pllperiph)){
		ARISC_ERR("try to get pll_periph failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllperiph)) {
		ARISC_ERR("try to enable pll_periph output failed!\n");
		return -EINVAL;
	}

#elif (defined CONFIG_ARCH_SUN8IW9P1)
	struct clk *pllddr0 = NULL;
	struct clk *pllddr1 = NULL;
	struct clk *pllperiph = NULL;
	struct clk *hosc = NULL;
	struct clk *losc = NULL;

	ARISC_INF("device [%s] clk resource request enter\n", dev_name(&pdev->dev));
	/* config PLL5 for dram clk */
	pllddr0 = clk_get(NULL, PLL_DDR0_CLK);
	if(!pllddr0 || IS_ERR(pllddr0)){
		ARISC_ERR("try to get pll_ddr0 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllddr0)) {
		ARISC_ERR("try to enable pll_ddr0 output failed!\n");
		return -EINVAL;
	}

	pllddr1 = clk_get(NULL, PLL_DDR1_CLK);
	if(!pllddr1 || IS_ERR(pllddr1)){
		ARISC_ERR("try to get pll_ddr1 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllddr1)) {
		ARISC_ERR("try to enable pll_ddr1 output failed!\n");
		return -EINVAL;
	}

	/* config PLL6 for cpus clk */
	pllperiph = clk_get(NULL, PLL_PERIPH0_CLK);
	if(!pllperiph || IS_ERR(pllperiph)){
		ARISC_ERR("try to get pll_periph failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pllperiph)) {
		ARISC_ERR("try to enable pll_periph output failed!\n");
		return -EINVAL;
	}

#elif defined CONFIG_ARCH_SUN9IW1P1
	struct clk *pll3 = NULL;
	struct clk *pll4 = NULL;
	struct clk *pll6 = NULL;
	struct clk *hosc = NULL;
	struct clk *losc = NULL;

	ARISC_INF("device [%s] clk resource request enter\n", dev_name(&pdev->dev));

	/* config PLL6 for dram clk */
	pll6 = clk_get(NULL, PLL6_CLK);
	if(!pll6 || IS_ERR(pll6)){
		ARISC_ERR("try to get pll6 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pll6)) {
		ARISC_ERR("try to enable pll6 output failed!\n");
		return -EINVAL;
	}

	/* config PLL3 for cpus clk */
	pll3 = clk_get(NULL, PLL3_CLK);
	if(!pll3 || IS_ERR(pll3)){
		ARISC_ERR("try to get pll3 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pll3)) {
		ARISC_ERR("try to enable pll3 output failed!\n");
		return -EINVAL;
	}

	/* config PLL4 for cpus clk */
	pll4 = clk_get(NULL, PLL4_CLK);
	if(!pll4 || IS_ERR(pll4)){
		ARISC_ERR("try to get pll4 failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(pll4)) {
		ARISC_ERR("try to enable pll4 output failed!\n");
		return -EINVAL;
	}
#endif

	/* config HOSC for cpus clk */
	hosc = clk_get(NULL, HOSC_CLK);
	if(!hosc || IS_ERR(hosc)){
		ARISC_ERR("try to get hosc failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(hosc)) {
		ARISC_ERR("try to enable hosc output failed!\n");
		return -EINVAL;
	}

	/* config LOSC for cpus clk */
	losc = clk_get(NULL, LOSC_CLK);
	if(!losc || IS_ERR(losc)){
		ARISC_ERR("try to get losc failed!\n");
		return -EINVAL;
	}

	if(clk_prepare_enable(losc)) {
		ARISC_ERR("try to enable losc output failed!\n");
		return -EINVAL;
	}

	ARISC_INF("device [%s] clk resource request ok\n", dev_name(&pdev->dev));
	return 0;
}

static int sunxi_arisc_pin_cfg(struct platform_device *pdev)
{
	script_item_u script_val;
	script_item_value_type_e type;
	script_item_u  *pin_list;
	int            pin_count = 0;
	int            pin_index = 0;
	struct gpio_config    *pin_cfg;
	char          pin_name[SUNXI_PIN_NAME_MAX_LEN];
	unsigned long      config;

	ARISC_INF("device [%s] pin resource request enter\n", dev_name(&pdev->dev));
	/*
	 * request arisc resources:
	 * p2wi/rsb gpio...
	 */
	/* get pin sys_config info */
#if defined CONFIG_ARCH_SUN8IW1P1
	pin_count = script_get_pio_list ("s_p2twi0", &pin_list);
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
      (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW9P1)
	pin_count = script_get_pio_list ("s_rsb0", &pin_list);
#elif defined CONFIG_ARCH_SUN9IW1P1
	pin_count = script_get_pio_list ("s_rsb0", &pin_list);
#else
#error "please select a platform\n"
#endif

	if (pin_count == 0) {
		/* "s_p2twi0" or "s_rsb0" have no pin configuration */
		ARISC_WRN("arisc s_p2twi0/s_rsb0 have no pin configuration\n");
		return -EINVAL;
	}

	/* request pin individually */
	for (pin_index = 0; pin_index < pin_count; pin_index++) {
		pin_cfg = &(pin_list[pin_index].gpio);

		/* valid pin of sunxi-pinctrl, config pin attributes individually.*/
		sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);
		if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg->pull);
			pin_config_set (SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg->drv_level);
			pin_config_set (SUNXI_PINCTRL, pin_name, config);
		}
		if (pin_cfg->data != GPIO_DATA_DEFAULT) {
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
			pin_config_set (SUNXI_PINCTRL, pin_name, config);
		}
	}
	/*
	 * request arisc resources:
	 * uart gpio...
	 */
	type = script_get_item("s_uart0", "s_uart_used", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_WRN("sys_config.fex have no arisc s_uart0 config!");
		script_val.val = 0;
	}
	if (1 == script_val.val) {
		pin_count = script_get_pio_list ("s_uart0", &pin_list);
		if (pin_count == 0) {
			/* "s_uart0" have no pin configuration */
			ARISC_WRN("arisc s_uart0 have no pin configuration\n");
			return -EINVAL;
		}

		/* request pin individually */
		for (pin_index = 0; pin_index < pin_count; pin_index++) {
			pin_cfg = &(pin_list[pin_index].gpio);

			/* valid pin of sunxi-pinctrl, config pin attributes individually.*/
			sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
			if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg->pull);
				pin_config_set (SUNXI_PINCTRL, pin_name, config);
			}
			if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg->drv_level);
				pin_config_set (SUNXI_PINCTRL, pin_name, config);
			}
			if (pin_cfg->data != GPIO_DATA_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
				pin_config_set (SUNXI_PINCTRL, pin_name, config);
			}
		}
	}
	ARISC_INF("arisc uart debug config [%s] [%s] : %d\n", "s_uart0", "s_uart_used", script_val.val);

	/*
	 * request arisc resources:
	 * jtag gpio...
	 */
	type = script_get_item("s_jtag0", "s_jtag_used", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_WRN("sys_config.fex have no arisc s_jtag0 config!");
		script_val.val = 0;
	}
	if (script_val.val) {
		pin_count = script_get_pio_list ("s_jtag0", &pin_list);
		if (pin_count == 0) {
			/* "s_jtag0" have no pin configuration */
			ARISC_WRN("arisc s_jtag0 have no pin configuration\n");
			return -EINVAL;
		}

		/* request pin individually */
		for (pin_index = 0; pin_index < pin_count; pin_index++) {
			pin_cfg = &(pin_list[pin_index].gpio);

			/* valid pin of sunxi-pinctrl, config pin attributes individually.*/
			sunxi_gpio_to_name(pin_cfg->gpio, pin_name);
			config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, pin_cfg->mul_sel);
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
			if (pin_cfg->pull != GPIO_PULL_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, pin_cfg->pull);
				pin_config_set (SUNXI_PINCTRL, pin_name, config);
			}
			if (pin_cfg->drv_level != GPIO_DRVLVL_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DRV, pin_cfg->drv_level);
				pin_config_set (SUNXI_PINCTRL, pin_name, config);
			}
			if (pin_cfg->data != GPIO_DATA_DEFAULT) {
				config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, pin_cfg->data);
				pin_config_set (SUNXI_PINCTRL, pin_name, config);
			}
		}
	}
	ARISC_INF("arisc jtag debug config [%s] [%s] : %d\n", "s_jtag0", "s_jtag_used", script_val.val);

	ARISC_INF("device [%s] pin resource request ok\n", dev_name(&pdev->dev));

	return 0;
}

static s32 sunxi_arisc_para_init(struct arisc_para *para)
{
	script_item_u script_val;
	script_item_value_type_e type;

	/* init para */
	memset(para, 0, ARISC_PARA_SIZE);
#if defined CONFIG_ARCH_SUN9IW1P1

	/* parse arisc->machine */
	para->machine = ARISC_MACHINE_PAD;
	type = script_get_item("arisc", "machine", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		ARISC_ERR("arisc->machine config type err or can not be found!\n");
		return -EINVAL;
	}

	if (!strcmp(script_val.str, "homlet proto")) {
		para->machine = ARISC_MACHINE_HOMLET;
	}
	ARISC_LOG("arisc->machine:%s\n", script_val.str);

	/* parse arisc->oz_scale_delay */
	type = script_get_item("arisc", "oz_scale_delay", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("arisc->oz_scale_delay config type err or can not be found!\n");
		return -EINVAL;
	}

	para->oz_scale_delay = script_val.val;
	ARISC_LOG("arisc->oz_scale_delay:%u\n", para->oz_scale_delay);

	/* parse arisc->oz_onoff_delay */
	type = script_get_item("arisc", "oz_onoff_delay", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_ERR("arisc->oz_onoff_delay config type err or can not be found!\n");
		return -EINVAL;
	}

	para->oz_onoff_delay = script_val.val;
	ARISC_LOG("arisc->oz_onoff_delay:%u\n", para->oz_onoff_delay);
#endif
	/* parse s_uart_pin_used */
	type = script_get_item("s_uart0", "s_uart_used", &script_val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		ARISC_WRN("sys_config.fex have no arisc s_uart0 config!");
		script_val.val = 0;
	}
	if (1 == script_val.val)
		para->uart_pin_used = script_val.val;
	else
		para->uart_pin_used = 0;

#ifdef CONFIG_AW_AXP
	/* get power regulator tree */
	get_pwr_regu_tree(para->power_regu_tree);
#endif

	return 0;
}

static void sunxi_arisc_setup_para(struct arisc_para *para)
{
	void *dest;

	dest = (void *)(arisc_sram_a2_vbase + ARISC_PARA_ADDR_OFFSET);

	/* copy arisc parameters to target address */
	memcpy(dest, (void *)para, ARISC_PARA_SIZE);
	ARISC_INF("setup arisc para sram_a2 finished\n");
}

int sunxi_deassert_arisc(void)
{
	ARISC_INF("set arisc reset to de-assert state\n");
#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || \
    (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW9P1)
	{
		volatile unsigned long value;
		value = readl((IO_ADDRESS(SUNXI_R_CPUCFG_PBASE) + 0x0));
		value &= ~1;
		writel(value, (IO_ADDRESS(SUNXI_R_CPUCFG_PBASE) + 0x0));
		value = readl((IO_ADDRESS(SUNXI_R_CPUCFG_PBASE) + 0x0));
		value |= 1;
		writel(value, (IO_ADDRESS(SUNXI_R_CPUCFG_PBASE) + 0x0));
	}
#elif defined CONFIG_ARCH_SUN9IW1P1
	{
		volatile unsigned long value;
		value = readl((IO_ADDRESS(SUNXI_R_PRCM_PBASE) + 0x0));
		value &= ~1;
		writel(value, (IO_ADDRESS(SUNXI_R_PRCM_PBASE) + 0x0));
		value = readl((IO_ADDRESS(SUNXI_R_PRCM_PBASE) + 0x0));
		value |= 1;
		writel(value, (IO_ADDRESS(SUNXI_R_PRCM_PBASE) + 0x0));
	}
#endif

	return 0;
}

u32 sunxi_load_arisc(void *image, u32 image_size, void *para, u32 para_size)
{
	u32   ret;
	void *dest;

	if (sunxi_soc_is_secure()) {
		flush_cache_all();
		ret = call_firmware_op(load_arisc, image, image_size,
		                       para, para_size, ARISC_PARA_ADDR_OFFSET);
	} else {
		/* clear sram_a2 area */
		memset((void *)arisc_sram_a2_vbase, 0, SUNXI_SRAM_A2_SIZE);

		/* load arisc system binary data to sram_a2 */
		memcpy((void *)arisc_sram_a2_vbase, image, image_size);
		ARISC_INF("load arisc image finish\n");

		/* setup arisc parameters */
		dest = (void *)(arisc_sram_a2_vbase + ARISC_PARA_ADDR_OFFSET);
		memcpy(dest, (void *)para, para_size);
		ARISC_INF("setup arisc para finish\n");
		flush_cache_all();

		/* relese arisc reset */
		sunxi_deassert_arisc();
		ARISC_INF("release arisc reset finish\n");
	}
	ARISC_INF("load arisc finish\n");

	return 0;
}

static int sunxi_arisc_probe(struct platform_device *pdev)
{
	int   binary_len;
	int   ret;
	struct arisc_para para;
	u32    message_addr;
	u32    message_phys;
	u32    message_size;

	ARISC_INF("arisc initialize\n");

	/* cfg sunxi arisc clk */
	ret = sunxi_arisc_clk_cfg(pdev);
	if (ret) {
		ARISC_ERR("sunxi-arisc clk cfg failed\n");
		return -EINVAL;
	}

	/* cfg sunxi arisc pin */
	ret = sunxi_arisc_pin_cfg(pdev);
	if (ret) {
		ARISC_ERR("sunxi-arisc pin cfg failed\n");
		return -EINVAL;
	}

	ARISC_INF("sram_a2 vaddr(%x)\n", (unsigned int)arisc_sram_a2_vbase);
	ARISC_INF("sram_a2 lengt(%x)\n", (unsigned int)SUNXI_SRAM_A2_SIZE);

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || \
    (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN8IW9P1)
	binary_len = 0x13000;
#elif (defined CONFIG_ARCH_SUN8IW7P1)
	binary_len = SUNXI_SRAM_A2_SIZE;
#elif defined CONFIG_ARCH_SUN9IW1P1
	binary_len = (int)(&arisc_binary_end) - (int)(&arisc_binary_start);
#endif

#if (defined CONFIG_ARCH_SUN8IW6P1)
	if ((int)(&arisc_binary_end) - (int)(&arisc_binary_start) - 0x018000 > ARISC_RESERVE_MEMSIZE)
		ARISC_ERR("reserve dram space littler than cpus code!");
	memcpy((void *)phys_to_virt(ARISC_RESERVE_MEMBASE), \
	       (void *)(((unsigned char *)&arisc_binary_start) + 0x018000), \
	       (int)(&arisc_binary_end) - (int)(&arisc_binary_start) - 0x018000);
	ARISC_INF("cp arisc code1 [addr = %p, len = %x] to dram:%p finished\n",
	          (void *)(((unsigned char *)&arisc_binary_start) + 0x018000), \
	          (int)(&arisc_binary_end) - (int)(&arisc_binary_start) - 0x018000, \
	          (void *)ARISC_RESERVE_MEMBASE);
#elif (defined CONFIG_ARCH_SUN8IW7P1)
	memset((void *)phys_to_virt(SUPER_STANDBY_MEM_BASE), 0, SUPER_STANDBY_MEM_SIZE);
	if ((int)(&arisc_binary_end) - (int)(&arisc_binary_start) - binary_len > SUPER_STANDBY_MEM_SIZE)
		ARISC_ERR("reserve dram space littler than cpus code!");
	memcpy((void *)phys_to_virt(SUPER_STANDBY_MEM_BASE), \
	       (void *)(((unsigned char *)&arisc_binary_start) + binary_len), \
	       (int)(&arisc_binary_end) - (int)(&arisc_binary_start) - binary_len);
	ARISC_INF("cp arisc code1 [addr = %p, len = %x] to dram:%p finished\n",
	          (void *)(((unsigned char *)&arisc_binary_start) + binary_len), \
	          (int)(&arisc_binary_end) - (int)(&arisc_binary_start) - binary_len, \
	          (void *)SUPER_STANDBY_MEM_BASE);
#endif
	/* initialize hwspinlock */
	ARISC_INF("hwspinlock initialize\n");
	arisc_hwspinlock_init();

	/* initialize hwmsgbox */
	ARISC_INF("hwmsgbox initialize\n");
	arisc_hwmsgbox_init();

	/* setup arisc parameter */
	memset(&para, 0, sizeof(struct arisc_para));
	sunxi_arisc_para_init(&para);
	sunxi_arisc_setup_para(&para);

	/* allocate shared message buffer,
	 * the shared buffer should be non-cacheable.
	 * secure    : sram-a2 last 4k byte;
	 * non-secure: dram non-cacheable buffer.
	 */
	if (sunxi_soc_is_secure()) {
		message_addr = (u32)dma_alloc_coherent(NULL, PAGE_SIZE, &(message_phys), GFP_KERNEL);
		message_size = PAGE_SIZE;
		para.message_pool_phys = message_phys;
		para.message_pool_size = message_size;
	} else {
		/* use sram-a2 last 4k byte */
		message_addr = (u32)arisc_sram_a2_vbase + ARISC_MESSAGE_POOL_START;
		message_size = ARISC_MESSAGE_POOL_END - ARISC_MESSAGE_POOL_START;
		para.message_pool_phys = ARISC_MESSAGE_POOL_START;
		para.message_pool_size = ARISC_MESSAGE_POOL_END - ARISC_MESSAGE_POOL_START;
	}

	/* initialize message manager */
	ARISC_INF("message manager initialize start:%x, end:%x\n", message_addr, message_size);
	arisc_message_manager_init((void *)message_addr, message_size);

	/* load arisc */
	sunxi_load_arisc((void *)(&arisc_binary_start), binary_len,
	                 (void *)(&para), sizeof(struct arisc_para));

	/* wait arisc ready */
	ARISC_INF("wait arisc ready....\n");
	if (arisc_wait_ready(10000)) {
		ARISC_LOG("arisc startup failed\n");
	}

	/* enable arisc asyn tx interrupt */
	arisc_hwmsgbox_enable_receiver_int(ARISC_HWMSGBOX_ARISC_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	/* enable arisc syn tx interrupt */
	arisc_hwmsgbox_enable_receiver_int(ARISC_HWMSGBOX_ARISC_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);

	/*
	 * detect sunxi chip id
	 * include soc chip id, pmu chip id and serial.
	 */
	sunxi_chip_id_init();

	/* config dvfs v-f table */
	if (arisc_dvfs_cfg_vf_table()) {
		ARISC_WRN("config dvfs v-f table failed\n");
	}

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
    (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN9IW1P1)
	/* config ir config paras */
	if (arisc_sysconfig_ir_paras()) {
		ARISC_WRN("config ir paras failed\n");
	}
#endif

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1) || \
    (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1)
	/* config pmu config paras */
	if (arisc_config_pmu_paras()) {
		ARISC_WRN("config pmu paras failed\n");
	}
#endif

	/* config dram config paras */
	if (arisc_config_dram_paras()) {
		ARISC_WRN("config dram paras failed\n");
	}

#if (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN9IW1P1)
	/* config standby power paras */
	if (arisc_sysconfig_sstpower_paras()) {
		ARISC_WRN("config sst power paras failed\n");
	}
#endif
	atomic_set(&arisc_suspend_flag, 0);

	/* PM hookup */
#if (defined CONFIG_ARCH_SUN8IW7P1)
	if(!pm_power_off)
		pm_power_off = arisc_power_off;
	else
		ARISC_WRN("pm_power_off aleardy been registered!\n");
#endif
	/* arisc initialize succeeded */
	ARISC_LOG("sunxi-arisc driver v%s startup succeeded\n", DRV_VERSION);

	return 0;
}

/* msgbox irq no */
static struct resource sunxi_arisc_resource[] = {
	[0] = {
		.start = SUNXI_IRQ_MBOX,
		.end   = SUNXI_IRQ_MBOX,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device sunxi_arisc_device = {
	.name           = DEV_NAME,
	.id             = PLATFORM_DEVID_NONE,
	.num_resources  = ARRAY_SIZE(sunxi_arisc_resource),
	.resource       = sunxi_arisc_resource,
};

static struct platform_driver sunxi_arisc_driver = {
	.probe      = sunxi_arisc_probe,
	.shutdown   = sunxi_arisc_shutdown,
	.driver     = {
		.name     = DRV_NAME,
		.owner    = THIS_MODULE,
		.pm       = SUNXI_ARISC_DEV_PM_OPS,
	},
};

static int __init arisc_init(void)
{
	int ret;

	ARISC_LOG("sunxi-arisc driver v%s\n", DRV_VERSION);

	ret = platform_driver_register(&sunxi_arisc_driver);
	if (IS_ERR_VALUE(ret)) {
		ARISC_ERR("register sunxi arisc platform driver failed\n");
		goto err_platform_driver_register;
	}
	ret = platform_device_register(&sunxi_arisc_device);
	if (IS_ERR_VALUE(ret)) {
		ARISC_ERR("register sunxi arisc platform device failed\n");
		goto err_platform_device_register;
	}

	sunxi_arisc_sysfs(&sunxi_arisc_device);

	/* arisc init ok */
	arisc_notify(ARISC_INIT_READY, NULL);

	return 0;

err_platform_device_register:
	platform_device_unregister(&sunxi_arisc_device);
err_platform_driver_register:
	platform_driver_unregister(&sunxi_arisc_driver);
	return -EINVAL;
}

static void __exit arisc_exit(void)
{
	platform_device_unregister(&sunxi_arisc_device);
	platform_driver_unregister(&sunxi_arisc_driver);
	ARISC_LOG("module unloaded\n");
}

subsys_initcall(arisc_init);
module_exit(arisc_exit);

MODULE_DESCRIPTION("SUNXI ARISC Driver");
MODULE_AUTHOR("Superm Wu <superm@allwinnertech.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:sunxi arisc driver");
