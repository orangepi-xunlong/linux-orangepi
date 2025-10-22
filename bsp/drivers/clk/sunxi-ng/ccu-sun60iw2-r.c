// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2023 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_nm.h"

#include "ccu-sun60iw2-r.h"

#define SUNXI_R_CCU_VERSION	"0.5.6"

/* ccu_des_start */

static const char * const r_ahb_parents[] = { "dcxo", "rtc32k", "iosc", "pll-peri0-200m", "pll-peri0-300m" };
SUNXI_CCU_M_WITH_MUX(r_ahb_clk, "r-ahb", r_ahb_parents,
		0x0000, 0, 5, 24, 3, CLK_IGNORE_UNUSED);

static const char * const r_apbs_parents[] = { "dcxo", "rtc32k", "iosc", "pll-peri0-200m", "pll-ref" };
SUNXI_CCU_M_WITH_MUX(r_apbs0_clk, "r-apbs0", r_apbs_parents,
		0x000c, 0, 5, 24, 3, CLK_IGNORE_UNUSED);

SUNXI_CCU_M_WITH_MUX(r_apbs1_clk, "r-apbs1", r_apbs_parents,
		0x0010, 0, 5, 24, 3, CLK_IGNORE_UNUSED);


static const char * const r_timer_parents[] = { "dcxo", "rtc32k", "iosc", "pll-peri0-200m", "pll-ref" };

static struct ccu_div r_timer0_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x0100,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer0",
				r_timer_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

static struct ccu_div r_timer1_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x0104,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer1",
				r_timer_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

static struct ccu_div r_timer2_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x0108,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer2",
				r_timer_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

static struct ccu_div r_timer3_clk = {
	.enable		= BIT(0),
	.div		= _SUNXI_CCU_DIV_FLAGS(1, 3, CLK_DIVIDER_POWER_OF_TWO),
	.mux		= _SUNXI_CCU_MUX(4, 3),
	.common		= {
		.reg		= 0x010C,
		.hw.init	= CLK_HW_INIT_PARENTS("r-timer3",
				r_timer_parents,
				&ccu_div_ops, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

static SUNXI_CCU_GATE(r_timer_clk, "r-timer",
		"dcxo",
		0x011C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_twd_clk, "r-twd",
		"dcxo",
		0x012C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const r_pwm_parents[] = { "dcxo", "rtc32k", "iosc", "pll-ref" };

static SUNXI_CCU_MUX_WITH_GATE(r_pwm_clk, "r-pwm",
		r_pwm_parents, 0x0130,
		24, 2,	/* mux */
		BIT(31), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_pwm_bus_clk, "r-bus-pwm",
		"dcxo",
		0x013C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const r_spi_parents[] = { "dcxo", "pll-peri0-200m", "pll-peri0-300m", "pll-peri1-300m", "pll-ref" };

static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(r_spi_clk, "r-spi",
		r_spi_parents, 0x0150,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_spi_bus_clk, "r-spi-bus",
		"dcxo",
		0x015C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_mbox_clk, "r-mbox",
		"dcxo",
		0x017C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_uart1_clk, "r-uart1",
		"r-apbs1",
		0x018C, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_uart0_clk, "r-uart0",
		"r-apbs1",
		0x018C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_twi2_clk, "r-twi2",
		"r-apbs1",
		0x019C, BIT(2), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_twi1_clk, "r-twi1",
		"r-apbs1",
		0x019C, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_twi0_clk, "r-twi0",
		"r-apbs1",
		0x019C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_ppu_clk, "r-ppu",
		"dcxo",
		0x01AC, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_tzma_clk, "r-tzma",
		"dcxo",
		0x01B0, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_cpus_bist_clk, "r-cpus-bist",
		"dcxo",
		0x01BC, BIT(0), CLK_IGNORE_UNUSED);

static const char * const r_irrx_parents[] = { "rtc32k", "dcxo", "pll-ref" };

static SUNXI_CCU_M_WITH_MUX_GATE(r_irrx_clk, "r-irrx",
		r_irrx_parents, 0x01C0,
		0, 5,	/* M */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_irrx_bus_clk, "r-irrx-bus",
		"dcxo",
		0x01CC, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(rtc_clk, "rtc",
		"dcxo",
		0x020C, BIT(0), CLK_IGNORE_UNUSED);

static const char * const riscv_24m_parents[] = { "dcxo", "osc32k", "iosc" };
static SUNXI_CCU_MUX_WITH_GATE(riscv_24m_clk, "riscv-24m",
		riscv_24m_parents, 0x0210,
		24, 2,	/* mux */
		BIT(31), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(riscv_cfg_clk, "riscv-cfg",
		"dcxo",
		0x021C, BIT(1), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(riscv, "riscv",
		"dcxo",
		0x021C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(r_cpucfg_clk, "r-cpucfg",
		"dcxo",
		0x022C, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vdd_usb2cpus_clk, "vdd-usb2cpus",
		"dcxo",
		0x0250, BIT(8), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vdd_sys2usb_clk, "vdd-sys2usb",
		"dcxo",
		0x0250, BIT(3), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vdd_sys2cpus_clk, "vdd-sys2cpus",
		"dcxo",
		0x0250, BIT(2), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(vdd_ddr_clk, "vdd-ddr",
		"dcxo",
		0x0250, BIT(0), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(tt_auto_clk, "tt-auto",
		"dcxo",
		0x0338, BIT(9), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(cpu_icache_auto_clk, "cpu-icache-auto",
		"dcxo",
		0x0338, BIT(8), CLK_IGNORE_UNUSED);

static SUNXI_CCU_GATE(ahbs_auto_clk__clk, "ahbs-auto-clk",
		"dcxo",
		0x033C, BIT(24), CLK_IGNORE_UNUSED);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun60iw2_r_ccu_resets[] = {
	[RST_BUS_R_TIME]		= { 0x011c, BIT(16) },
	[RST_BUS_R_PWM]		= { 0x013c, BIT(16) },
	[RST_BUS_R_SPI]		= { 0x015c, BIT(16) },
	[RST_BUS_R_MBOX]		= { 0x017c, BIT(16) },
	[RST_BUS_R_UART1]		= { 0x018c, BIT(17) },
	[RST_BUS_R_UART0]		= { 0x018c, BIT(16) },
	[RST_BUS_R_TWI2]		= { 0x019c, BIT(18) },
	[RST_BUS_R_TWI1]		= { 0x019c, BIT(17) },
	[RST_BUS_R_TWI0]		= { 0x019c, BIT(16) },
	[RST_BUS_R_IRRX]		= { 0x01cc, BIT(16) },
	[RST_BUS_RTC]			= { 0x020c, BIT(16) },
	[RST_BUS_RISCV_CFG]		= { 0x021c, BIT(16) },
	[RST_BUS_R_CPUCFG]		= { 0x022c, BIT(16) },
	[RST_BUS_MODULE]		= { 0x0260, BIT(0) },
};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun60iw2_r_hw_clks = {
	.hws    = {
		[CLK_R_AHB]				= &r_ahb_clk.common.hw,
		[CLK_R_APBS0]			= &r_apbs0_clk.common.hw,
		[CLK_R_APBS1]			= &r_apbs1_clk.common.hw,
		[CLK_R_TIMER0]			= &r_timer0_clk.common.hw,
		[CLK_R_TIMER1]			= &r_timer1_clk.common.hw,
		[CLK_R_TIMER2]			= &r_timer2_clk.common.hw,
		[CLK_R_TIMER3]			= &r_timer3_clk.common.hw,
		[CLK_R_TIMER]			= &r_timer_clk.common.hw,
		[CLK_R_TWD]			= &r_twd_clk.common.hw,
		[CLK_R_PWM]			= &r_pwm_clk.common.hw,
		[CLK_R_BUS_PWM]			= &r_pwm_bus_clk.common.hw,
		[CLK_R_SPI]			= &r_spi_clk.common.hw,
		[CLK_R_BUS_SPI]			= &r_spi_bus_clk.common.hw,
		[CLK_R_MBOX]			= &r_mbox_clk.common.hw,
		[CLK_R_UART1]			= &r_uart1_clk.common.hw,
		[CLK_R_UART0]			= &r_uart0_clk.common.hw,
		[CLK_R_TWI2]			= &r_twi2_clk.common.hw,
		[CLK_R_TWI1]			= &r_twi1_clk.common.hw,
		[CLK_R_TWI0]			= &r_twi0_clk.common.hw,
		[CLK_R_PPU]			= &r_ppu_clk.common.hw,
		[CLK_R_TZMA]			= &r_tzma_clk.common.hw,
		[CLK_R_CPUS_BIST]		= &r_cpus_bist_clk.common.hw,
		[CLK_R_IRRX]			= &r_irrx_clk.common.hw,
		[CLK_R_BUS_IRRX]		= &r_irrx_bus_clk.common.hw,
		[CLK_RTC]			= &rtc_clk.common.hw,
		[CLK_RISCV_24M]		= &riscv_24m_clk.common.hw,
		[CLK_RISCV_CFG]		= &riscv_cfg_clk.common.hw,
		[CLK_RISCV]			= &riscv.common.hw,
		[CLK_R_CPUCFG]			= &r_cpucfg_clk.common.hw,
		[CLK_VDD_USB2CPUS]		= &vdd_usb2cpus_clk.common.hw,
		[CLK_VDD_SYS2USB]		= &vdd_sys2usb_clk.common.hw,
		[CLK_VDD_SYS2CPUS]		= &vdd_sys2cpus_clk.common.hw,
		[CLK_VDD_DDR]			= &vdd_ddr_clk.common.hw,
		[CLK_TT_AUTO]			= &tt_auto_clk.common.hw,
		[CLK_CPU_ICACHE_AUTO]		= &cpu_icache_auto_clk.common.hw,
		[CLK_AHBS_AUTO_CLK]		= &ahbs_auto_clk__clk.common.hw,
	},
	.num = CLK_R_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun60iw2_r_ccu_clks[] = {
	&r_ahb_clk.common,
	&r_apbs0_clk.common,
	&r_apbs1_clk.common,
	&r_timer0_clk.common,
	&r_timer1_clk.common,
	&r_timer2_clk.common,
	&r_timer3_clk.common,
	&r_timer_clk.common,
	&r_twd_clk.common,
	&r_pwm_clk.common,
	&r_pwm_bus_clk.common,
	&r_spi_clk.common,
	&r_spi_bus_clk.common,
	&r_mbox_clk.common,
	&r_uart1_clk.common,
	&r_uart0_clk.common,
	&r_twi2_clk.common,
	&r_twi1_clk.common,
	&r_twi0_clk.common,
	&r_ppu_clk.common,
	&r_tzma_clk.common,
	&r_cpus_bist_clk.common,
	&r_irrx_clk.common,
	&r_irrx_bus_clk.common,
	&rtc_clk.common,
	&riscv_24m_clk.common,
	&riscv_cfg_clk.common,
	&riscv.common,
	&r_cpucfg_clk.common,
	&vdd_usb2cpus_clk.common,
	&vdd_sys2usb_clk.common,
	&vdd_sys2cpus_clk.common,
	&vdd_ddr_clk.common,
	&tt_auto_clk.common,
	&cpu_icache_auto_clk.common,
	&ahbs_auto_clk__clk.common,
};

static const struct sunxi_ccu_desc sun60iw2_r_ccu_desc = {
	.ccu_clks	= sun60iw2_r_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun60iw2_r_ccu_clks),

	.hw_clks	= &sun60iw2_r_hw_clks,

	.resets		= sun60iw2_r_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun60iw2_r_ccu_resets),
};

static int sun60iw2_r_ccu_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	struct device *dev = &pdev->dev;
	void __iomem *reg;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM\n");
		return -EINVAL;
	}

	reg = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(reg)) {
		dev_err(dev, "Fail to map IO resource\n");
		return PTR_ERR(reg);
	}

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun60iw2_r_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun60iw2_r_ccu_clks,
			ARRAY_SIZE(sun60iw2_r_ccu_clks),
			NULL, 0);

	sunxi_info(NULL, "sunxi ccu driver version: %s\n", SUNXI_R_CCU_VERSION);
	return 0;
}

static const struct of_device_id sun60iw2_r_ccu_ids[] = {
	{ .compatible = "allwinner,sun60iw2-r-ccu" },
	{ }
};

static struct platform_driver sun60iw2_r_ccu_driver = {
	.probe	= sun60iw2_r_ccu_probe,
	.driver	= {
		.name	= "sun60iw2-ccu-r",
		.of_match_table	= sun60iw2_r_ccu_ids,
	},
};

static int __init sun60iw2_r_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun60iw2_r_ccu_driver);
	if (err)
		pr_err("register ccu sun60iw2 r failed\n");

	return err;
}

core_initcall(sun60iw2_r_ccu_init);

static void __exit sun60iw2_r_ccu_exit(void)
{
	platform_driver_unregister(&sun60iw2_r_ccu_driver);
}
module_exit(sun60iw2_r_ccu_exit);

MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_R_CCU_VERSION);
