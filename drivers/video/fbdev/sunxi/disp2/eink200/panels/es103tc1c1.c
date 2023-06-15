/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
/*
 * sysconfig.fex
[eink]
eink_panel_name     = "es103tc1c1"
eink_scan_mode	    = 0
eink_width          = 1872
eink_height         = 1404
eink_bits	    = 5
eink_data_len	    = 16
eink_lsl	    = 11
eink_lbl	    = 8
eink_lel	    = 23
eink_fsl	    = 1
eink_fbl	    = 4
eink_fel	    = 12
eink_lgonl	    = 215
eink_gdck_sta	    = 10
eink_gdoe_start_line= 2
eink_fresh_hz       = 85
eink_gray_level	    = 16
eink_wav_path	    = "/system/default_8_5.awf"
;eink_power	    = "vcc-lcd"
;eink_pin_power	    = "vcc-pd"

eink_gpio_0         = port:PD00<1><0><default><1>
eink_gpio_1         = port:PD01<1><0><default><1>
eink_gpio_2         = port:PD02<1><0><default><1>
eink_gpio_3         = port:PD03<1><0><default><1>
eink_gpio_4         = port:PD04<1><0><default><1>
eink_gpio_5         = port:PD05<1><0><default><1>

;eink_pin0	    = port:PD00<5><0><default><default>
;eink_pin1           = port:PD01<5><0><default><default>
;eink_pin2           = port:PD02<5><0><default><default>
;eink_pin3           = port:PD03<5><0><default><default>
;eink_pin4           = port:PD04<5><0><default><default>
;eink_pin5           = port:PD05<5><0><default><default>
eink_pin6           = port:PD06<5><0><default><default>
eink_pin7           = port:PD07<5><0><default><default>
eink_pin8           = port:PD08<5><0><default><default>
eink_pin9           = port:PD09<5><0><default><default>
eink_pin10          = port:PD10<5><0><default><default>
eink_pin11          = port:PD11<5><0><default><default>
eink_pin12          = port:PD12<5><0><default><default>
eink_pin13          = port:PD13<5><0><default><default>
eink_pin14          = port:PD14<5><0><default><default>
eink_pin15          = port:PD15<5><0><default><default>
eink_pinoeh         = port:PD16<5><0><default><default>
eink_pinleh         = port:PD17<5><0><default><default>
eink_pinckh         = port:PD18<5><0><default><default>
eink_pinsth         = port:PD19<5><0><default><default>
eink_pinckv         = port:PD20<5><0><default><default>
eink_pinmod         = port:PD21<5><0><default><default>
eink_pinstv         = port:PD22<5><0><default><default>

[eink_suspend]
;eink_pin0           = port:PD00<7><0><default><default>
;eink_pin1           = port:PD01<7><0><default><default>
;eink_pin2           = port:PD02<7><0><default><default>
;eink_pin3           = port:PD03<7><0><default><default>
;eink_pin4           = port:PD04<7><0><default><default>
;eink_pin5           = port:PD05<7><0><default><default>
eink_pin6           = port:PD06<7><0><default><default>
eink_pin7           = port:PD07<7><0><default><default>
eink_pin8           = port:PD08<7><0><default><default>
eink_pin9           = port:PD09<7><0><default><default>
eink_pin10          = port:PD10<7><0><default><default>
eink_pin11          = port:PD11<7><0><default><default>
eink_pin12          = port:PD12<7><0><default><default>
eink_pin13          = port:PD13<7><0><default><default>
eink_pin14          = port:PD14<7><0><default><default>
eink_pin15          = port:PD15<7><0><default><default>
eink_pinoeh         = port:PD16<7><0><default><default>
eink_pinleh         = port:PD17<7><0><default><default>
eink_pinckh         = port:PD18<7><0><default><default>
eink_pinsth         = port:PD19<7><0><default><default>
eink_pinckv         = port:PD20<7><0><default><default>
eink_pinmod         = port:PD21<7><0><default><default>
eink_pinstv         = port:PD22<7><0><default><default>
 *
 */

#include "default_eink.h"

#if IS_ENABLED(CONFIG_TPS65185_VCOM)
#include <linux/ebc.h>
static struct ebc_pwr_ops g_tps65185_ops;
extern int register_ebc_pwr_ops(struct ebc_pwr_ops *ops);
#endif

static void EINK_power_on(void);
static void EINK_power_off(void);

#if IS_ENABLED(CONFIG_TPS65185_VCOM)
static void Eink_int_panel(void)
{
	//printk("%s: register tps65185 ops\n", __func__);
	register_ebc_pwr_ops(&g_tps65185_ops);
}

static void Eink_uninit_panel(void)
{
	//printk("%s: unregister tps65185 ops\n", __func__);
	g_tps65185_ops.power_down = NULL;
	g_tps65185_ops.power_on = NULL;
}

static void panel_power_on(void)
{
	//sunxi_lcd_power_enable(sel, 0);
	//config lcd_power pin to open lcd power0
	panel_pin_cfg(1);
}

static void panel_power_off(void)
{
	panel_pin_cfg(0);
	//sunxi_lcd_power_disable(sel, 0);
	//config lcd_power pin to close lcd power0
}
#endif

static s32 EINK_open_flow(void)
{
	EINK_INFO_MSG("\n");

#if IS_ENABLED(CONFIG_TPS65185_VCOM)
	EINK_OPEN_FUNC(Eink_int_panel, 0);
#endif
	EINK_OPEN_FUNC(EINK_power_on, 2);
	return 0;
}

static s32 EINK_close_flow(void)
{
	EINK_INFO_MSG("\n");
	EINK_CLOSE_FUNC(EINK_power_off, 2);
#if IS_ENABLED(CONFIG_TPS65185_VCOM)
	EINK_OPEN_FUNC(Eink_uninit_panel, 0);
#endif
	return 0;
}

static void EINK_power_on(void)
{
	EINK_DEBUG_MSG("EINK_power_on!\n");

#if IS_ENABLED(CONFIG_TPS65185_VCOM)
	//tps65185_power_wakeup_with_lock(1);
	if (g_tps65185_ops.power_on) {
		g_tps65185_ops.power_on();
	} else {
		pr_warn("[%s]: tps65185 power on func has not been registed yet\n", __func__);
	}
	panel_power_on();
#else
	/* pwr3 pb2 */
	panel_gpio_set_value(0, 1);
	mdelay(1);
	/* pwr0 pb0 */
	panel_gpio_set_value(1, 1);
	mdelay(1);
	/* pb1 */
	panel_gpio_set_value(2, 1);
	mdelay(1);
	/* pwr2 pd6 */
	panel_gpio_set_value(3, 1);
	mdelay(1);
	/* pwr com pd7 */
	panel_gpio_set_value(4, 1);
	mdelay(1);

	panel_gpio_set_value(5, 1);
	mdelay(2);

	panel_pin_cfg(1);
#endif
}

static void EINK_power_off(void)
{
	EINK_INFO_MSG("EINK_power_off!\n");
#if IS_ENABLED(CONFIG_TPS65185_VCOM)
	//tps65185_power_wakeup_with_lock(0);
	if (g_tps65185_ops.power_down) {
		g_tps65185_ops.power_down();
	} else {
		pr_warn("[%s]: tps65185 power down func has not been registed yet\n", __func__);
	}
	panel_power_off();
#else
	panel_pin_cfg(0);

	panel_gpio_set_value(5, 0);
	mdelay(1);

	panel_gpio_set_value(4, 0);
	mdelay(1);

	panel_gpio_set_value(3, 0);
	mdelay(1);

	panel_gpio_set_value(2, 0);
	mdelay(1);

	panel_gpio_set_value(1, 0);
	mdelay(1);

	panel_gpio_set_value(0, 0);
	mdelay(2);
#endif
}

struct __eink_panel es103tc1c1 = {
	/* panel driver name, must mach the eink_panel_name in sys_config.fex */
	.name = "es103tc1c1",
	.func = {
		 .cfg_open_flow = EINK_open_flow,
		 .cfg_close_flow = EINK_close_flow,
		}
	,
};
