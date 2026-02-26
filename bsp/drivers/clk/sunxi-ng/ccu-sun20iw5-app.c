// SPDX-License-Identifier: GPL-4.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 rengaomin@allwinnertech.com
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "ccu_common.h"
#include "ccu_reset.h"

#include "ccu_div.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include "ccu_mult.h"
#include "ccu_nk.h"
#include "ccu_nkm.h"
#include "ccu_nkmp.h"
#include "ccu_nm.h"

#include "ccu-sun20iw5-app.h"

/*
 * The CPU PLL is actually NP clock, with P being /1, /2 or /4. However
 * P should only be used for output frequencies lower than 288 MHz.
 *
 * For now we can just model it as a multiplier clock, and force P to /1.
 *
 * The M factor is present in the register's description, but not in the
 * frequency formula, and it's documented as "M is only used for backdoor
 * testing", so it's not modelled and then force to 0.
 */

/* ccu_des_start */

#define UPD_KEY_VALUE 0x8000000

static const char * const dram_parents[] = { "pll-ddr", "pll-peri1-600m", "peri1-480m", "pll-peri1-400m", "peri1-150m" };

static SUNXI_CCU_M_WITH_MUX_GATE_KEY(dram_clk, "dram",
		dram_parents, 0x0004,
		0, 5,	/* M */
		24, 3,	/* mux */
		UPD_KEY_VALUE,
		BIT(31),	/* gate */
		CLK_GET_RATE_NOCACHE | CLK_IS_CRITICAL);

static const char * const e907_parents[] = { "dcxo", "rtc-32k"};
static SUNXI_CCU_MUX_WITH_GATE(e907_ts_clk, "e907-ts",
		e907_parents, 0x000c,
		24, 2, BIT(31), 0);

static const char * const al27_mt_parents[] = { "dcxo", "rtc-32k"};
static SUNXI_CCU_MUX_WITH_GATE(al27_mt_clk, "al27-mt",
		al27_mt_parents, 0x0010,
		24, 2,
		BIT(31), 0);

static const char * const smhc0_parents[] = { "dcxo", "pll-peri1-118m", "pll-peri-192m", "pll-ddr", "pll-video-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc0_clk, "smhc0",
		smhc0_parents, 0x014,
		0, 5,	/* M */
		16, 5,	/* N */
		24, 2,	/* mux */
		BIT(31), 0);

static const char * const ss_parents[] = { "dcxo", "pll-peri-118m" };

static SUNXI_CCU_M_WITH_MUX_GATE(ss_clk, "ss",
		ss_parents, 0x018,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const spi_parents[] = { "dcxo", "pll-peri-cko-307m", "pll-peri-cko-236m", "pll-peri-cko-48m", "pll-csi-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi_clk, "spi",
		spi_parents, 0x01c,
		0, 4,	/* M */
		16, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const spif_parents[] = { "dcxo", "pll-peri-cko-512m", "pll-peri-cko-384m", "pll-peri-cko-307m" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spif_clk, "spif",
		spif_parents, 0x020,
		0, 4,	/* M */
		16, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const mcsi_parents[] = { "pll-peri-236m", "pll-peri-307m", "pll-peri-cko-384m", "pll-peri-cko-384m", "pll-video-4x", "pll-csi-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(mcsi_clk, "mcsi",
		mcsi_parents, 0x024,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const csi_master_parents[] = { "dcxo", "pll-csi-4x" "pll-video-4x", "", "pll-peri-cko-1024m", "pll-peri-cko-24m" };

static SUNXI_CCU_MP_WITH_MUX_GATE(csi_master0_clk, "csi-master0",
		csi_master_parents, 0x028,
		0, 5,	/* M */
		8, 5,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_MP_WITH_MUX_GATE(csi_master1_clk, "csi-master1",
		csi_master_parents, 0x02C,
		0, 5,	/* M */
		16, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static const char * const spi2_clk_parents[] = { "dcxo", "pll-peri-cko-307m", "pll-peir-cko-236m", "", "pll-peri-cko-48m", "pll-csi-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi2_clk, "spi2",
		spi2_clk_parents, 0x0030,
		0, 4,	/* M */
		16, 2,	/* N */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const tconlcd_parents[] = { "pll-video-4x", "pll-peri-cko-480m", "pll-csi-4x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(tconlcd_clk, "tconlcd",
		tconlcd_parents, 0x0034,
		0, 4,	/* M */
		16, 2,	/* N */
		24, 2,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_SET_RATE_PARENT);

static const char * const de_parents[] = { "pll-peri-cko-307m", "pll-video-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(de_clk, "de",
		de_parents, 0x038,
		0, 5,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_NO_REPARENT | CLK_IGNORE_UNUSED);

static const char * const g2d_parents[] = { "pll-peri-cko-307m", "pll-video0-1x" };

static SUNXI_CCU_M_WITH_MUX_GATE(g2d_clk, "g2d",
		g2d_parents, 0x03c,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const gpadc_parents[] = { "dcxo24m", "dcxo" };

static SUNXI_CCU_M_WITH_MUX_GATE(gpadc0_24m_clk, "gpadc0_24m",
		gpadc_parents, 0x040,
		0, 5,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const ve_parents[] = { "pll-peri-cko-219m", "pll-peri-cko-341m", "pll-peri-cko-614m", "pll-peri-cko-768m", "pll-peri-cko-1024m", "pll-video-2x", "pll-csi-4x"};

static SUNXI_CCU_M_WITH_MUX_GATE(ve_clk, "ve",
		ve_parents, 0x044,
		0, 3,	/* M */
		24, 3,	/* mux */
		BIT(31),	/* gate */
		CLK_SET_RATE_PARENT);

static const char * const audio_dac_parents[] = { "pll-audio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_dac_clk, "audio-dac",
		audio_dac_parents, 0x048,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const audio_adc_parents[] = { "pll-aduio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(audio_adc_clk, "audio-adc",
		audio_adc_parents, 0x04c,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const dmic_parents[] = { "pll-aduio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(dmic_clk, "dmic",
		dmic_parents, 0x050,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const i2s_parents[] = { "pll-aduio-1x", "pll-audio-4x" };

static SUNXI_CCU_M_WITH_MUX_GATE(i2s0_clk, "i2s0",
		i2s_parents, 0x054,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static SUNXI_CCU_M_WITH_MUX_GATE(i2s1_clk, "i2s1",
		i2s_parents, 0x058,
		0, 4,	/* M */
		24, 1,	/* mux */
		BIT(31),	/* gate */
		0);

static const char * const smhc1_parents[] = { "dcxo", "pll-peri1-118m", "pll-peri-192m", "pll-ddr", "pll-video-2x" };
static SUNXI_CCU_MP_WITH_MUX_GATE_NO_INDEX(smhc1_clk, "smhc1",
		smhc1_parents, 0x05c,
		0, 5,	/* M */
		16, 5,	/* N */
		24, 2,	/* mux */
		BIT(31), 0);

static const char * const pll_audio_4x_parents[] = { "pll-peri-cko-1536m", "pll-cpu", "pll-video-2x" };

static SUNXI_CCU_M_WITH_MUX(pll_audio_4x_clk, "pll-audio-4x", pll_audio_4x_parents,
		0x0060, 0, 5, 26, 2, 0);

static const char * const pll_audio_1x_parents[] = { "pll-peri-cko-614", "pll-cpu", "pll-video-2x" };

static SUNXI_CCU_M_WITH_MUX(pll_audio_1x_clk, "pll-audio-1x", pll_audio_1x_parents,
		0x0060, 5, 5, 24, 2, 0);

static const char * const spi1_parents[] = { "dcxo", "pll-peri-cko-307m", "pll-peri-cko-236m", "", "pll-peri-cko-48m", "pll-csi-2x" };

static SUNXI_CCU_MP_WITH_MUX_GATE(spi1_clk, "spi1",
		spi1_parents, 0x064,
		0, 4,	/* M */
		16, 2,	/* N */
		24, 3,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(gmac_fanout_clk, "a27l2-gate",
		"dcxo",
		0x0070, BIT(7), 0);

static SUNXI_CCU_GATE(msgbox_clk, "msgbox",
		"dcxo",
		0x0070, BIT(6), 0);

static SUNXI_CCU_GATE(wiegand_24m_clk, "weigand-24m",
		"dcxo",
		0x0070, BIT(4), 0);

static SUNXI_CCU_GATE(usb_24m_clk, "usb-24m",
		"dcxo",
		0x0070, BIT(3), 0);

static SUNXI_CCU_GATE(usb_12m_clk, "usb-12m",
		"dcxo",
		0x0070, BIT(2), 0);

static SUNXI_CCU_GATE(usb_48m_clk, "usb-48m",
		"dcxo",
		0x0070, BIT(1), 0);

static SUNXI_CCU_GATE(avs_clk, "avs",
		"dcxo",
		0x0070, BIT(0), 0);

static const char * const gmac_25m_parents[] = { "dcxo", "pll-csi-2x", "pll-cpu" };

static SUNXI_CCU_MP_WITH_MUX_GATE(gmac_25m_clk, "gmac-25m",
		gmac_25m_parents, 0x074,
		0, 5,	/* M */
		16, 5,	/* N */
		24, 2,	/* mux */
		BIT(31), 0);

static SUNXI_CCU_GATE(dpss_top_bus_clk, "dpss-top",
		"dcxo",
		0x0080, BIT(31), 0);

static SUNXI_CCU_GATE(gpio_bus_clk, "gpio",
		"dcxo",
		0x0080, BIT(30), 0);

static SUNXI_CCU_GATE(wkt_bus_clk, "wkt",
		"dcxo",
		0x0080, BIT(29), 0);

static SUNXI_CCU_GATE(mcsi_ahb_clk, "mcsi-ahb",
		"dcxo",
		0x0080, BIT(28), 0);

static SUNXI_CCU_GATE(mcsi_mbus_clk, "mcsi-mbus",
		"dcxo",
		0x0080, BIT(27), 0);

static SUNXI_CCU_GATE(vid_out_clk, "vid-out-ahb",
		"dcxo",
		0x0080, BIT(26), 0);

static SUNXI_CCU_GATE(vid_out_mbus_clk, "vid-out-mbus",
		"dcxo",
		0x0080, BIT(25), 0);

static SUNXI_CCU_GATE(gmac_hbus_clk, "gmac-bus",
		"dcxo",
		0x0080, BIT(24), 0);

static SUNXI_CCU_GATE(usbohci_clk, "usbohci",
		"dcxo",
		0x0080, BIT(22), 0);

static SUNXI_CCU_GATE(usbehci_clk, "usbehci",
		"dcxo",
		0x0080, BIT(21), 0);

static SUNXI_CCU_GATE(usbotg_clk, "usbotg",
		"dcxo",
		0x0080, BIT(20), 0);

static SUNXI_CCU_GATE(usb_clk, "usb-hclk",
		"dcxo",
		0x0080, BIT(19), 0);

static SUNXI_CCU_GATE(uart3_clk, "uart3",
		"dcxo",
		0x0080, BIT(18), 0);

static SUNXI_CCU_GATE(uart2_clk, "uart2",
		"dcxo",
		0x0080, BIT(17), 0);

static SUNXI_CCU_GATE(uart1_clk, "uart1",
		"dcxo",
		0x0080, BIT(16), 0);

static SUNXI_CCU_GATE(uart0_clk, "uart0",
		"dcxo",
		0x0080, BIT(15), 0);

static SUNXI_CCU_GATE(twi0_clk, "twi0",
		"dcxo",
		0x0080, BIT(14), 0);

static SUNXI_CCU_GATE(pwm_clk, "pwm",
		"dcxo",
		0x0080, BIT(13), 0);

static SUNXI_CCU_GATE(trng_clk, "trng",
		"dcxo",
		0x0080, BIT(11), 0);

static SUNXI_CCU_GATE(timer_clk, "timer",
		"dcxo",
		0x0080, BIT(10), 0);

static SUNXI_CCU_GATE(sg_dma_clk, "sgdma",
		"dcxo",
		0x0080, BIT(9), 0);

static SUNXI_CCU_GATE(dma_clk, "dma",
		"dcxo",
		0x0080, BIT(8), 0);

static SUNXI_CCU_GATE(syscrtl_clk, "sysctrl",
		"dcxo",
		0x0080, BIT(7), 0);

static SUNXI_CCU_GATE(ce_clk, "ce",
		"dcxo",
		0x0080, BIT(6), 0);

static SUNXI_CCU_GATE(hstimer_clk, "hstimer",
		"dcxo",
		0x0080, BIT(5), 0);

static SUNXI_CCU_GATE(splock_clk, "splock",
		"dcxo",
		0x0080, BIT(4), 0);

static SUNXI_CCU_GATE(dram_bus_clk, "dram-bus",
		"dcxo",
		0x0080, BIT(3), 0);

static SUNXI_CCU_GATE(rv_msgbox_clk, "rv-msgbox-bus",
		"dcxo",
		0x0080, BIT(2), 0);

static SUNXI_CCU_GATE(riscv_clk, "riscv",
		"dcxo",
		0x0080, BIT(0), 0);

static SUNXI_CCU_GATE(g2d_mbus_clk, "g2d-mbus",
		"dcxo",
		0x0084, BIT(31), 0);

static SUNXI_CCU_GATE(g2d_bus_clk, "g2d-bus",
		"dcxo",
		0x0084, BIT(30), 0);

static SUNXI_CCU_GATE(g2d_hb_clk, "g2d-hb",
		"dcxo",
		0x0084, BIT(29), 0);

static SUNXI_CCU_GATE(twi2_clk, "twi2",
		"dcxo",
		0x0084, BIT(25), 0);

static SUNXI_CCU_GATE(twi1_clk, "twi1",
		"dcxo",
		0x0084, BIT(24), 0);

static SUNXI_CCU_GATE(spi2_bus_clk, "spi2-bus",
		"dcxo",
		0x0084, BIT(23), 0);

static SUNXI_CCU_GATE(gmac_bus_clk, "gmac-hbus",
		"dcxo",
		0x0084, BIT(22), 0);

static SUNXI_CCU_GATE(smhc1_bus_clk, "smhc-bus1",
		"dcxo",
		0x0084, BIT(21), 0);

static SUNXI_CCU_GATE(smhc0_bus_clk, "smhc-bus0",
		"dcxo",
		0x0084, BIT(20), 0);

static SUNXI_CCU_GATE(spi1_bus_clk, "spi1-hbus",
		"dcxo",
		0x0084, BIT(19), 0);

static SUNXI_CCU_GATE(gbgsys_clk, "dbgsys",
		"dcxo",
		0x0084, BIT(18), 0);

static SUNXI_CCU_GATE(gmac_mbus_ahb_clk, "gmac-mbus",
		"dcxo",
		0x0084, BIT(17), 0);

static SUNXI_CCU_GATE(smhc1_mbus_ahb_clk, "smhc1-mbus",
		"dcxo",
		0x0084, BIT(16), 0);

static SUNXI_CCU_GATE(smhc0_mbus_ahb_clk, "smhc0-mbus",
		"dcxo",
		0x0084, BIT(15), 0);

static SUNXI_CCU_GATE(usb_mbus_ahb_clk, "usb-mbus",
		"dcxo",
		0x0084, BIT(14), 0);

static SUNXI_CCU_GATE(dma_mbus_clk, "dma-mbus",
		"dcxo",
		0x0084, BIT(13), 0);

static SUNXI_CCU_GATE(i2s1_bus_clk, "i2s1-bus",
		"dcxo",
		0x0084, BIT(9), 0);

static SUNXI_CCU_GATE(i2s0_bus_clk, "i2s0-bus",
		"dcxo",
		0x0084, BIT(8), 0);

static SUNXI_CCU_GATE(dmic_bus_clk, "dmic-bus",
		"dcxo",
		0x0084, BIT(7), 0);

static SUNXI_CCU_GATE(adda_bus_clk, "adda-bus",
		"dcxo",
		0x0084, BIT(6), 0);

static SUNXI_CCU_GATE(spif_bus_clk, "spif-hbus",
		"dcxo",
		0x0084, BIT(5), 0);

static SUNXI_CCU_GATE(spi_bus_clk, "spi-hbus",
		"dcxo",
		0x0084, BIT(4), 0);

static SUNXI_CCU_GATE(ve_ahb_clk, "ve-ahb",
		"dcxo",
		0x0084, BIT(3), 0);

static SUNXI_CCU_GATE(ve_mbus_clk, "ve-mbus",
		"dcxo",
		0x0084, BIT(2), 0);

static SUNXI_CCU_GATE(ths_bus_clk, "ths-bus",
		"dcxo",
		0x0084, BIT(1), 0);

static SUNXI_CCU_GATE(gpa_clk, "gpa-bus",
		"dcxo",
		0x0084, BIT(10), 0);

static SUNXI_CCU_GATE(mcsi_hbus_clk, "mcsi-hbus",
		"dcxo",
		0x0088, BIT(28), 0);

static SUNXI_CCU_GATE(mcsi_sbus_clk, "mcsi-sclk",
		"dcxo",
		0x0088, BIT(27), 0);

static SUNXI_CCU_GATE(misp_bus_clk, "misp-sclk",
		"dcxo",
		0x0088, BIT(26), 0);

static SUNXI_CCU_GATE(sd_monitor_clk, "sd-monitor",
		"dcxo",
		0x0088, BIT(9), 0);

static SUNXI_CCU_GATE(ahb_monitor_clk, "ahb-monitor",
		"dcxo",
		0x0088, BIT(8), 0);

static SUNXI_CCU_GATE(ve_sbus_clk, "ve-sclk",
		"dcxo",
		0x0088, BIT(5), 0);

static SUNXI_CCU_GATE(ve_hbus_clk, "ve-hclk",
		"dcxo",
		0x0088, BIT(4), 0);

static SUNXI_CCU_GATE(tcon_bus_clk, "tcon-hclk",
		"dcxo",
		0x0088, BIT(3), 0);

static SUNXI_CCU_GATE(sgdma_clk, "sgdma-mbus",
		"dcxo",
		0x0088, BIT(2), 0);

static SUNXI_CCU_GATE(de_bus_clk, "de-mbus",
		"dcxo",
		0x0088, BIT(1), 0);

static SUNXI_CCU_GATE(de_hb_clk, "de-hb",
		"dcxo",
		0x0088, BIT(0), 0);
/* ccu_des_end */

/* rst_def_start */
static struct ccu_reset_map sun20iw5_app_ccu_resets[] = {
	[RST_BUS_DPSSTOP]		= { 0x0090, BIT(31) },
	[RST_BUS_WKUP]			= { 0x0090, BIT(29) },
	[RST_BUS_MCSI]			= { 0x0090, BIT(28) },
	[RST_BUS_HRESETN_MCSI]		= { 0x0090, BIT(27) },
	[RST_BUS_G2D]			= { 0x0090, BIT(26) },
	[RST_BUS_DE]			= { 0x0090, BIT(25) },
	[RST_BUS_GMAC]			= { 0x0090, BIT(24) },
	[RST_BUS_USBPHY]		= { 0x0090, BIT(23) },
	[RST_BUS_USBOHCI]		= { 0x0090, BIT(22) },
	[RST_BUS_USBEHCI]		= { 0x0090, BIT(21) },
	[RST_BUS_USBOTG]		= { 0x0090, BIT(20) },
	[RST_BUS_USB]			= { 0x0090, BIT(19) },
	[RST_BUS_UART3]			= { 0x0090, BIT(18) },
	[RST_BUS_UART2]			= { 0x0090, BIT(17) },
	[RST_BUS_UART1]			= { 0x0090, BIT(16) },
	[RST_BUS_UART0]			= { 0x0090, BIT(15) },
	[RST_BUS_TWI0]			= { 0x0090, BIT(14) },
	[RST_BUS_PWM]			= { 0x0090, BIT(13) },
	[RST_BUS_WEIGAND]		= { 0x0090, BIT(12) },
	[RST_BUS_TRNG]			= { 0x0090, BIT(11) },
	[RST_BUS_TIMER]			= { 0x0090, BIT(10) },
	[RST_BUS_SGDMA]			= { 0x0090, BIT(9) },
	[RST_BUS_DMA]			= { 0x0090, BIT(8) },
	[RST_BUS_SYSCTRL]		= { 0x0090, BIT(7) },
	[RST_BUS_CE]			= { 0x0090, BIT(6) },
	[RST_BUS_HSTIMER]		= { 0x0090, BIT(5) },
	[RST_BUS_SPLOCK]		= { 0x0090, BIT(4) },
	[RST_BUS_DRAM]			= { 0x0090, BIT(3) },
	[RST_BUS_RV_MSGBOX]		= { 0x0090, BIT(2) },
	[RST_BUS_RV_SYS_APB]		= { 0x0090, BIT(1) },
	[RST_BUS_RV]			= { 0x0090, BIT(0) },
	[RST_BUS_A27_CFG]		= { 0x0094, BIT(28) },
	[RST_BUS_A27_MSGBOX]		= { 0x0094, BIT(27) },
	[RST_BUS_A27]			= { 0x0094, BIT(26) },
	[RST_BUS_TWI2]			= { 0x0094, BIT(25) },
	[RST_BUS_TWI1]			= { 0x0094, BIT(24) },
	[RST_BUS_SPI2]			= { 0x0094, BIT(23) },
	[RST_BUS_SMHC1]			= { 0x0094, BIT(21) },
	[RST_BUS_SMHC0]			= { 0x0094, BIT(20) },
	[RST_BUS_SPI1]			= { 0x0094, BIT(19) },
	[RST_BUS_DGBSYS]		= { 0x0094, BIT(18) },
	[RST_BUS_MBUS]			= { 0x0094, BIT(12) },
	[RST_BUS_TCONLCD]		= { 0x0094, BIT(11) },
	[RST_BUS_TCON]			= { 0x0094, BIT(10) },
	[RST_BUS_I2S1]			= { 0x0094, BIT(9) },
	[RST_BUS_I2S0]			= { 0x0094, BIT(8) },
	[RST_BUS_DMIC]			= { 0x0094, BIT(7) },
	[RST_BUS_AUDIO]			= { 0x0094, BIT(6) },
	[RST_BUS_SPIF]			= { 0x0094, BIT(5) },
	[RST_BUS_SPI]			= { 0x0094, BIT(4) },
	[RST_BUS_VE]			= { 0x0094, BIT(3) },
	[RST_BUS_THS]			= { 0x0094, BIT(1) },
	[RST_BUS_GPADC]			= { 0x0094, BIT(0) },
	[RST_BUS_A27WFG]		= { 0x0098, BIT(2) },
	[RST_BUS_GPIOWDG]		= { 0x0098, BIT(1) },
	[RST_BUS_RV_WDG]		= { 0x0098, BIT(0) },

};
/* rst_def_end */

/* ccu_def_start */
static struct clk_hw_onecell_data sun20iw5_app_hw_clks = {
	.hws    = {
		[CLK_DRAM]			= &dram_clk.common.hw,
		[CLK_E907_TS]			= &e907_ts_clk.common.hw,
		[CLK_AL27_MT]			= &al27_mt_clk.common.hw,
		[CLK_SMHC0]			= &smhc0_clk.common.hw,
		[CLK_SS]			= &ss_clk.common.hw,
		[CLK_SPI]			= &spi_clk.common.hw,
		[CLK_SPIF]			= &spif_clk.common.hw,
		[CLK_MCSI]			= &mcsi_clk.common.hw,
		[CLK_CSI_MASTER0]		= &csi_master0_clk.common.hw,
		[CLK_CSI_MASTER1]		= &csi_master1_clk.common.hw,
		[CLK_SPI2]			= &spi2_clk.common.hw,
		[CLK_TCONLCD]			= &tconlcd_clk.common.hw,
		[CLK_DE]			= &de_clk.common.hw,
		[CLK_G2D]			= &g2d_clk.common.hw,
		[CLK_GPADC0_24M]		= &gpadc0_24m_clk.common.hw,
		[CLK_VE]			= &ve_clk.common.hw,
		[CLK_AUDIO_DAC]			= &audio_dac_clk.common.hw,
		[CLK_AUDIO_ADC]			= &audio_adc_clk.common.hw,
		[CLK_DMIC]			= &dmic_clk.common.hw,
		[CLK_I2S0]			= &i2s0_clk.common.hw,
		[CLK_I2S1]			= &i2s1_clk.common.hw,
		[CLK_SMHC1]			= &smhc1_clk.common.hw,
		[CLK_PLL_AUDIO_4X]		= &pll_audio_4x_clk.common.hw,
		[CLK_PLL_AUDIO_1X]		= &pll_audio_1x_clk.common.hw,
		[CLK_SPI1]			= &spi1_clk.common.hw,
		[CLK_GMAC_FANOUT]		= &gmac_fanout_clk.common.hw,
		[CLK_MSGBOX]			= &msgbox_clk.common.hw,
		[CLK_WIEGAND_24M]		= &wiegand_24m_clk.common.hw,
		[CLK_USB_24M]			= &usb_24m_clk.common.hw,
		[CLK_USB_12M]			= &usb_12m_clk.common.hw,
		[CLK_USB_48M]			= &usb_48m_clk.common.hw,
		[CLK_AVS]			= &avs_clk.common.hw,
		[CLK_GMAC_25M]			= &gmac_25m_clk.common.hw,
		[CLK_BUS_DPSS_TOP]		= &dpss_top_bus_clk.common.hw,
		[CLK_BUS_GPIO]			= &gpio_bus_clk.common.hw,
		[CLK_BUS_WKT]			= &wkt_bus_clk.common.hw,
		[CLK_MCSI_AHB]			= &mcsi_ahb_clk.common.hw,
		[CLK_MBUS_MCSI]			= &mcsi_mbus_clk.common.hw,
		[CLK_VID_OUT]			= &vid_out_clk.common.hw,
		[CLK_MBUS_VID_OUT]		= &vid_out_mbus_clk.common.hw,
		[CLK_HBUS_GMAC]			= &gmac_hbus_clk.common.hw,
		[CLK_USBOHCI]			= &usbohci_clk.common.hw,
		[CLK_USBEHCI]			= &usbehci_clk.common.hw,
		[CLK_USBOTG]			= &usbotg_clk.common.hw,
		[CLK_USB]			= &usb_clk.common.hw,
		[CLK_UART3]			= &uart3_clk.common.hw,
		[CLK_UART2]			= &uart2_clk.common.hw,
		[CLK_UART1]			= &uart1_clk.common.hw,
		[CLK_UART0]			= &uart0_clk.common.hw,
		[CLK_TWI0]			= &twi0_clk.common.hw,
		[CLK_PWM]			= &pwm_clk.common.hw,
		[CLK_TRNG]			= &trng_clk.common.hw,
		[CLK_TIMER]			= &timer_clk.common.hw,
		[CLK_SG_DMA]			= &sg_dma_clk.common.hw,
		[CLK_DMA]			= &dma_clk.common.hw,
		[CLK_SYSCRTL]			= &syscrtl_clk.common.hw,
		[CLK_CE]			= &ce_clk.common.hw,
		[CLK_HSTIMER]			= &hstimer_clk.common.hw,
		[CLK_SPLOCK]			= &splock_clk.common.hw,
		[CLK_BUS_DRAM]			= &dram_bus_clk.common.hw,
		[CLK_RV_MSGBOX]			= &rv_msgbox_clk.common.hw,
		[CLK_RISCV]			= &riscv_clk.common.hw,
		[CLK_MBUS_G2D]			= &g2d_mbus_clk.common.hw,
		[CLK_BUS_G2D]			= &g2d_bus_clk.common.hw,
		[CLK_G2D_HB]			= &g2d_hb_clk.common.hw,
		[CLK_TWI2]			= &twi2_clk.common.hw,
		[CLK_TWI1]			= &twi1_clk.common.hw,
		[CLK_BUS_SPI2]			= &spi2_bus_clk.common.hw,
		[CLK_BUS_GMAC]			= &gmac_bus_clk.common.hw,
		[CLK_BUS_SMHC1]			= &smhc1_bus_clk.common.hw,
		[CLK_BUS_SMHC0]			= &smhc0_bus_clk.common.hw,
		[CLK_BUS_SPI1]			= &spi1_bus_clk.common.hw,
		[CLK_GBGSYS]			= &gbgsys_clk.common.hw,
		[CLK_MBUS_GMAC]			= &gmac_mbus_ahb_clk.common.hw,
		[CLK_MBUS_SMHC1]		= &smhc1_mbus_ahb_clk.common.hw,
		[CLK_MBUS_SMHC0]		= &smhc0_mbus_ahb_clk.common.hw,
		[CLK_MBUS_USB]			= &usb_mbus_ahb_clk.common.hw,
		[CLK_MBUS_DMA]			= &dma_mbus_clk.common.hw,
		[CLK_BUS_I2S1]			= &i2s1_bus_clk.common.hw,
		[CLK_BUS_I2S0]			= &i2s0_bus_clk.common.hw,
		[CLK_BUS_DMIC]			= &dmic_bus_clk.common.hw,
		[CLK_BUS_ADDA]			= &adda_bus_clk.common.hw,
		[CLK_BUS_SPIF]			= &spif_bus_clk.common.hw,
		[CLK_BUS_SPI]			= &spi_bus_clk.common.hw,
		[CLK_BUS_VE_AHB]		= &ve_ahb_clk.common.hw,
		[CLK_MBUS_VE]			= &ve_mbus_clk.common.hw,
		[CLK_BUS_THS]			= &ths_bus_clk.common.hw,
		[CLK_GPA]			= &gpa_clk.common.hw,
		[CLK_HBUS_MCSI]			= &mcsi_hbus_clk.common.hw,
		[CLK_SBUS_MCSI]			= &mcsi_sbus_clk.common.hw,
		[CLK_BUS_MISP]			= &misp_bus_clk.common.hw,
		[CLK_SD_MONITOR]		= &sd_monitor_clk.common.hw,
		[CLK_AHB_MONITOR]		= &ahb_monitor_clk.common.hw,
		[CLK_SBUS_VE]			= &ve_sbus_clk.common.hw,
		[CLK_HBUS_VE]			= &ve_hbus_clk.common.hw,
		[CLK_BUS_TCON]			= &tcon_bus_clk.common.hw,
		[CLK_SGDMA]			= &sgdma_clk.common.hw,
		[CLK_BUS_DE]			= &de_bus_clk.common.hw,
		[CLK_DE_HB]			= &de_hb_clk.common.hw,
	},
	.num = CLK_NUMBER,
};
/* ccu_def_end */

static struct ccu_common *sun20iw5_app_ccu_clks[] = {
	&dram_clk.common,
	&e907_ts_clk.common,
	&al27_mt_clk.common,
	&smhc0_clk.common,
	&ss_clk.common,
	&spi_clk.common,
	&spif_clk.common,
	&mcsi_clk.common,
	&csi_master0_clk.common,
	&csi_master1_clk.common,
	&spi2_clk.common,
	&tconlcd_clk.common,
	&de_clk.common,
	&g2d_clk.common,
	&gpadc0_24m_clk.common,
	&ve_clk.common,
	&audio_dac_clk.common,
	&audio_adc_clk.common,
	&dmic_clk.common,
	&i2s0_clk.common,
	&i2s1_clk.common,
	&smhc1_clk.common,
	&spi1_clk.common,
	&gmac_fanout_clk.common,
	&msgbox_clk.common,
	&wiegand_24m_clk.common,
	&usb_24m_clk.common,
	&usb_12m_clk.common,
	&usb_48m_clk.common,
	&avs_clk.common,
	&gmac_25m_clk.common,
	&dpss_top_bus_clk.common,
	&gpio_bus_clk.common,
	&wkt_bus_clk.common,
	&mcsi_ahb_clk.common,
	&mcsi_mbus_clk.common,
	&vid_out_clk.common,
	&vid_out_mbus_clk.common,
	&gmac_hbus_clk.common,
	&usbohci_clk.common,
	&usbehci_clk.common,
	&usbotg_clk.common,
	&usb_clk.common,
	&uart3_clk.common,
	&uart2_clk.common,
	&uart1_clk.common,
	&uart0_clk.common,
	&twi0_clk.common,
	&pwm_clk.common,
	&trng_clk.common,
	&timer_clk.common,
	&sg_dma_clk.common,
	&dma_clk.common,
	&syscrtl_clk.common,
	&ce_clk.common,
	&hstimer_clk.common,
	&splock_clk.common,
	&dram_bus_clk.common,
	&riscv_clk.common,
	&g2d_mbus_clk.common,
	&g2d_bus_clk.common,
	&g2d_hb_clk.common,
	&twi2_clk.common,
	&twi1_clk.common,
	&spi2_bus_clk.common,
	&gmac_bus_clk.common,
	&smhc1_bus_clk.common,
	&smhc0_bus_clk.common,
	&spi1_bus_clk.common,
	&gbgsys_clk.common,
	&gmac_mbus_ahb_clk.common,
	&smhc1_mbus_ahb_clk.common,
	&smhc0_mbus_ahb_clk.common,
	&usb_mbus_ahb_clk.common,
	&dma_mbus_clk.common,
	&i2s1_bus_clk.common,
	&i2s0_bus_clk.common,
	&dmic_bus_clk.common,
	&adda_bus_clk.common,
	&spif_bus_clk.common,
	&spi_bus_clk.common,
	&ve_ahb_clk.common,
	&ve_mbus_clk.common,
	&ths_bus_clk.common,
	&gpa_clk.common,
	&mcsi_hbus_clk.common,
	&mcsi_sbus_clk.common,
	&misp_bus_clk.common,
	&sd_monitor_clk.common,
	&ahb_monitor_clk.common,
	&ve_sbus_clk.common,
	&ve_hbus_clk.common,
	&tcon_bus_clk.common,
	&sgdma_clk.common,
	&de_bus_clk.common,
	&de_hb_clk.common,
	&pll_audio_4x_clk.common,
	&pll_audio_1x_clk.common,
};

static const struct sunxi_ccu_desc sun20iw5_app_ccu_desc = {
	.ccu_clks	= sun20iw5_app_ccu_clks,
	.num_ccu_clks	= ARRAY_SIZE(sun20iw5_app_ccu_clks),

	.hw_clks	= &sun20iw5_app_hw_clks,

	.resets		= sun20iw5_app_ccu_resets,
	.num_resets	= ARRAY_SIZE(sun20iw5_app_ccu_resets),
};

static int sun20iw5_app_ccu_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	void __iomem *reg;
	int ret;

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

	ret = sunxi_ccu_probe(pdev->dev.of_node, reg, &sun20iw5_app_ccu_desc);
	if (ret)
		return ret;

	sunxi_ccu_sleep_init(reg, sun20iw5_app_ccu_clks,
			ARRAY_SIZE(sun20iw5_app_ccu_clks),
			NULL, 0);

	return 0;
}

static const struct of_device_id sun20iw5_app_ccu_ids[] = {
	{ .compatible = "allwinner,sun20iw5-app-ccu" },
	{ }
};

static struct platform_driver sun20iw5_app_ccu_driver = {
	.probe	= sun20iw5_app_ccu_probe,
	.driver	= {
		.name	= "sun20iw5-app-ccu",
		.of_match_table	= sun20iw5_app_ccu_ids,
	},
};

static int __init sun20iw5_app_ccu_init(void)
{
	int err;

	err = platform_driver_register(&sun20iw5_app_ccu_driver);
	if (err)
		pr_err("register ccu sun20iw5_app failed\n");

	return err;
}

core_initcall(sun20iw5_app_ccu_init);

static void __exit sun20iw5_app_ccu_exit(void)
{
	platform_driver_unregister(&sun20iw5_app_ccu_driver);
}
module_exit(sun20iw5_app_ccu_exit);

MODULE_DESCRIPTION("Allwinner sun20iw5_app clk driver");
MODULE_AUTHOR("rengaomin<rengaomin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.0.6");
