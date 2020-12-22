/* linux/drivers/video/sunxi/disp/dev_disp.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Display driver for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_disp.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

fb_info_t g_fbi;
disp_drv_info g_disp_drv;
disp_fcount_data fcount_data;

disp_sw_init_para g_sw_init_para;

#define MY_BYTE_ALIGN(x) ( ( (x + (4*1024-1)) >> 12) << 12)             /* alloc based on 4K byte */

static u32 suspend_output_type[3] = {0,0,0};
static u32 suspend_status = 0;//0:normal; suspend_status&1 != 0:in early_suspend; suspend_status&2 != 0:in suspend;
static u32 suspend_prestep = 3; //0:after early suspend; 1:after suspend; 2:after resume; 3 :after late resume

#if defined(CONFIG_ARCH_SUN9IW1P1)
static unsigned int gbuffer[4096];
#endif
static struct info_mm  g_disp_mm[10];
static int g_disp_mm_sel = 0;

static struct cdev *my_cdev;
static dev_t devid ;
static struct class *disp_class;
static struct device *display_dev;

//static u32 disp_print_cmd_level = 0;
static u32 disp_cmd_print = 0xffff;   //print cmd which eq disp_cmd_print
#if defined(CONFIG_ARCH_SUN9IW1P1)
static struct sunxi_disp_mod disp_mod[] = {
	{DISP_MOD_FE0      ,    "fe0"},
	{DISP_MOD_FE1      ,    "fe1"},
	{DISP_MOD_FE2      ,    "fe2"},
	{DISP_MOD_BE0      ,    "be0"},
	{DISP_MOD_BE1      ,    "be1"},
	{DISP_MOD_BE2      ,    "be2"},
	{DISP_MOD_LCD0     ,    "lcd0"},
	{DISP_MOD_LCD1     ,    "lcd1"},
	{DISP_MOD_TVE0     ,    "tve0"},
	{DISP_MOD_TVE1     ,    "tve1"},
	{DISP_MOD_CCMU     ,    "ccmu"},
	{DISP_MOD_PIOC     ,    "pioc"},
	{DISP_MOD_PWM      ,    "pwm"},
	{DISP_MOD_DEU0     ,    "deu0"},
	{DISP_MOD_DEU1     ,    "deu1"},
	{DISP_MOD_CMU0     ,    "cmu0"},
	{DISP_MOD_CMU1     ,    "cmu1"},
	{DISP_MOD_DRC0     ,    "drc0"},
	{DISP_MOD_DRC1     ,    "drc1"},
	{DISP_MOD_DSI0     ,    "dsi0"},
	{DISP_MOD_DSI0_DPHY,    "dsi0_dphy"},
	{DISP_MOD_DSI1     ,    "dsi1"},
	{DISP_MOD_DSI1_DPHY,    "dsi1_dphy"},
	{DISP_MOD_HDMI     ,    "hdmi"},
	{DISP_MOD_EDP      ,    "edp"},
	{DISP_MOD_TOP      ,    "top"},
};
#elif defined(CONFIG_ARCH_SUN8IW5P1)
static struct sunxi_disp_mod disp_mod[] = {
	{DISP_MOD_FE0      ,    "fe0"},
	{DISP_MOD_BE0      ,    "be0"},
	{DISP_MOD_LCD0     ,    "lcd0"},
	{DISP_MOD_CCMU     ,    "ccmu"},
	{DISP_MOD_PIOC     ,    "pioc"},
	{DISP_MOD_PWM      ,    "pwm"},
	{DISP_MOD_DRC0     ,    "drc0"},
	{DISP_MOD_DSI0     ,    "dsi0"},
	{DISP_MOD_DSI0_DPHY,    "dsi0_dphy"},
	{DISP_MOD_WB0      ,    "wb0"},
	{DISP_MOD_SAT0     ,    "sat0"},
};
#endif
#if defined CONFIG_ARCH_SUN8IW1P1
static struct resource disp_resource[] =
{
	/*            name          start                        end                                 flags    */
	DISP_RESOURCE(fe0      ,SUNXI_FE0_VBASE           , SUNXI_FE0_VBASE           + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(fe1      ,SUNXI_FE1_VBASE           , SUNXI_FE1_VBASE           + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(fe2      ,SUNXI_FE2_VBASE           , SUNXI_FE2_VBASE           + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(be0      ,SUNXI_BE0_VBASE           , SUNXI_BE0_VBASE           + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(be1      ,SUNXI_BE1_VBASE           , SUNXI_BE1_VBASE           + 0x9fc   , IORESOURCE_MEM)
	DISP_RESOURCE(be2      ,SUNXI_BE2_VBASE           , SUNXI_BE2_VBASE           + 0x9fc   , IORESOURCE_MEM)
	DISP_RESOURCE(lcd0     ,SUNXI_LCD0_VBASE          , SUNXI_LCD0_VBASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(lcd1     ,SUNXI_LCD1_VBASE          , SUNXI_LCD1_VBASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(deu0     ,SUNXI_DEU0_VBASE          , SUNXI_DEU0_VBASE          + 0x60    , IORESOURCE_MEM)
	DISP_RESOURCE(deu1     ,SUNXI_DEU1_VBASE          , SUNXI_DEU1_VBASE          + 0x60    , IORESOURCE_MEM)
	DISP_RESOURCE(drc0     ,SUNXI_DRC0_VBASE          , SUNXI_DRC0_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(drc1     ,SUNXI_DRC1_VBASE          , SUNXI_DRC1_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu0     ,SUNXI_BE0_VBASE           , SUNXI_BE0_VBASE           + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu1     ,SUNXI_BE1_VBASE           , SUNXI_BE1_VBASE           + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0     ,SUNXI_MIPI_DSI0_VBASE     , SUNXI_MIPI_DSI0_VBASE     + 0x2fc   , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0_dphy,SUNXI_MIPI_DSI0_PHY_VBASE , SUNXI_MIPI_DSI0_PHY_VBASE + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(hdmi     ,SUNXI_HDMI_VBASE          , SUNXI_HDMI_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(ccmu     ,SUNXI_CCM_VBASE           , SUNXI_CCM_VBASE           + 0x2dc   , IORESOURCE_MEM)
	DISP_RESOURCE(pioc     ,SUNXI_PIO_VBASE           , SUNXI_PIO_VBASE           + 0x27c   , IORESOURCE_MEM)
	DISP_RESOURCE(pwm      ,SUNXI_PWM_VBASE           , SUNXI_PWM_VBASE           + 0x3c    , IORESOURCE_MEM)

	/*            name    irq_no                  flags     */
	DISP_RESOURCE(be0 , SUNXI_IRQ_DEBE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(be1 , SUNXI_IRQ_DEBE1   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe0 , SUNXI_IRQ_DEFE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe1 , SUNXI_IRQ_DEFE1   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(drc0, SUNXI_IRQ_DRC01   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(drc1, SUNXI_IRQ_DRC01   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd0, SUNXI_IRQ_LCD0    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd1, SUNXI_IRQ_LCD1    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(dsi0, SUNXI_IRQ_MIPIDSI , 0, IORESOURCE_IRQ)
};
#elif defined(CONFIG_ARCH_SUN8IW3P1)
static struct resource disp_resource[] =
{
	/*            name          start                        end                                 flags    */
	DISP_RESOURCE(fe0      ,SUNXI_DE_FE0_VBASE        , SUNXI_DE_FE0_VBASE        + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(be0      ,SUNXI_DE_BE0_VBASE        , SUNXI_DE_BE0_VBASE        + 0x9fc    , IORESOURCE_MEM)
	DISP_RESOURCE(lcd0     ,SUNXI_LCD0_VBASE          , SUNXI_LCD0_VBASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(drc0     ,SUNXI_DRC0_VBASE          , SUNXI_DRC0_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu0     ,SUNXI_DE_BE0_VBASE        , SUNXI_DE_BE0_VBASE        + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0     ,SUNXI_MIPI_DSI0_VBASE     , SUNXI_MIPI_DSI0_VBASE     + 0x2fc   , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0_dphy,SUNXI_MIPI_DSI0_PHY_VBASE , SUNXI_MIPI_DSI0_PHY_VBASE + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(ccmu     ,SUNXI_CCM_VBASE           , SUNXI_CCM_VBASE           + 0x2dc   , IORESOURCE_MEM)
	DISP_RESOURCE(pioc     ,SUNXI_PIO_VBASE           , SUNXI_PIO_VBASE           + 0x27c   , IORESOURCE_MEM)
	DISP_RESOURCE(pwm      ,SUNXI_PWM_VBASE           , SUNXI_PWM_VBASE           + 0x3c    , IORESOURCE_MEM)
	DISP_RESOURCE(wb0      ,SUNXI_DRC0_VBASE + 0x200  , SUNXI_DRC0_VBASE          + 0x2fc   , IORESOURCE_MEM)

	/*            name    irq_no                  flags     */
	DISP_RESOURCE(be0 , SUNXI_IRQ_DEBE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe0 , SUNXI_IRQ_DEFE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(drc0, SUNXI_IRQ_DRC     , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd0, SUNXI_IRQ_LCD0    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(dsi0, SUNXI_IRQ_MIPIDSI , 0, IORESOURCE_IRQ)
};
#elif defined(CONFIG_ARCH_SUN9IW1P1)
static struct resource disp_resource[] =
{
	/*            name          start                        end                                 flags    */
	DISP_RESOURCE(fe0      ,SUNXI_FE0_VBASE           , SUNXI_FE0_VBASE           + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(fe1      ,SUNXI_FE1_VBASE           , SUNXI_FE1_VBASE           + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(fe2      ,SUNXI_FE2_VBASE           , SUNXI_FE2_VBASE           + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(be0      ,SUNXI_BE0_VBASE           , SUNXI_BE0_VBASE           + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(be1      ,SUNXI_BE1_VBASE           , SUNXI_BE1_VBASE           + 0x9fc   , IORESOURCE_MEM)
	DISP_RESOURCE(be2      ,SUNXI_BE2_VBASE           , SUNXI_BE2_VBASE           + 0x9fc   , IORESOURCE_MEM)
	DISP_RESOURCE(lcd0     ,SUNXI_LCD0_VBASE          , SUNXI_LCD0_VBASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(lcd1     ,SUNXI_LCD1_VBASE          , SUNXI_LCD1_VBASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(deu0     ,SUNXI_DEU0_VBASE          , SUNXI_DEU0_VBASE          + 0x60    , IORESOURCE_MEM)
	DISP_RESOURCE(deu1     ,SUNXI_DEU1_VBASE          , SUNXI_DEU1_VBASE          + 0x60    , IORESOURCE_MEM)
	DISP_RESOURCE(drc0     ,SUNXI_DRC0_VBASE          , SUNXI_DRC0_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(drc1     ,SUNXI_DRC1_VBASE          , SUNXI_DRC1_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu0     ,SUNXI_BE0_VBASE           , SUNXI_BE0_VBASE           + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu1     ,SUNXI_BE1_VBASE           , SUNXI_BE1_VBASE           + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(ccmu     ,SUNXI_CCM_PLL_VBASE       , SUNXI_CCM_PLL_VBASE       + 0x2dc   , IORESOURCE_MEM)
	DISP_RESOURCE(pioc     ,SUNXI_PIO_VBASE           , SUNXI_PIO_VBASE           + 0x27c   , IORESOURCE_MEM)
	DISP_RESOURCE(pwm      ,SUNXI_PWM_VBASE           , SUNXI_PWM_VBASE           + 0x3c    , IORESOURCE_MEM)
	DISP_RESOURCE(top      ,SUNXI_DE_SYS_VBASE        , SUNXI_DE_SYS_VBASE        + 0x11c   , IORESOURCE_MEM)

	/*            name    irq_no                  flags     */
	DISP_RESOURCE(be0 , SUNXI_IRQ_DEBE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(be1 , SUNXI_IRQ_DEBE1   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe0 , SUNXI_IRQ_DEFE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe1 , SUNXI_IRQ_DEFE1   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(drc0, SUNXI_IRQ_DRC01   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(drc1, SUNXI_IRQ_DRC01   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd0, SUNXI_IRQ_LCD0    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd1, SUNXI_IRQ_LCD1    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(dsi0, SUNXI_IRQ_MIPIDSI , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(edp,  SUNXI_IRQ_EDP ,     0, IORESOURCE_IRQ)
	DISP_RESOURCE(be2 , SUNXI_IRQ_DEBE2   , 0, IORESOURCE_IRQ)
};
#elif defined(CONFIG_ARCH_SUN6I)
static struct resource disp_resource[] =
{
	/*            name          start                        end                                 flags    */
	DISP_RESOURCE(fe0      ,AW_VIR_DE_FE0_BASE      , AW_VIR_DE_FE0_BASE        + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(fe1      ,AW_VIR_DE_FE1_BASE      , AW_VIR_DE_FE1_BASE        + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(be0      ,AW_VIR_DE_BE0_BASE      , AW_VIR_DE_BE0_BASE        + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(be1      ,AW_VIR_DE_BE1_BASE      , AW_VIR_DE_BE1_BASE        + 0x9fc   , IORESOURCE_MEM)
	DISP_RESOURCE(lcd0     ,AW_VIR_LCD0_BASE        , AW_VIR_LCD0_BASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(lcd1     ,AW_VIR_LCD1_BASE        , AW_VIR_LCD1_BASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(deu0     ,AW_VIR_DEU0_BASE        , AW_VIR_DEU0_BASE          + 0x60    , IORESOURCE_MEM)
	DISP_RESOURCE(deu1     ,AW_VIR_DEU1_BASE        , AW_VIR_DEU1_BASE          + 0x60    , IORESOURCE_MEM)
	DISP_RESOURCE(drc0     ,AW_VIR_DRC0_BASE        , AW_VIR_DRC0_BASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(drc1     ,AW_VIR_DRC1_BASE        , AW_VIR_DRC1_BASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu0     ,AW_VIR_DE_BE0_BASE      , AW_VIR_DE_BE0_BASE        + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(cmu1     ,AW_VIR_DE_BE1_BASE      , AW_VIR_DE_BE1_BASE        + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0     ,AW_VIR_MIPI_DSI0_BASE   , AW_VIR_MIPI_DSI0_BASE     + 0x2fc   , IORESOURCE_MEM)
	DISP_RESOURCE(ccmu     ,AW_VIR_CCM_BASE         , AW_VIR_CCM_BASE           + 0x2dc   , IORESOURCE_MEM)
	DISP_RESOURCE(pioc     ,AW_VIR_PIO_BASE         , AW_VIR_PIO_BASE           + 0x27c   , IORESOURCE_MEM)
	DISP_RESOURCE(pwm      ,AW_VIR_PWM_BASE         , AW_VIR_PWM_BASE           + 0x3c    , IORESOURCE_MEM)

	/*            name    irq_no                  flags     */
	DISP_RESOURCE(be0 , AW_IRQ_DEBE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(be1 , AW_IRQ_DEBE1   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe0 , AW_IRQ_DEFE0   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(fe1 , AW_IRQ_DEFE1   , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd0, AW_IRQ_LCD0    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd1, AW_IRQ_LCD1    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(dsi0, AW_IRQ_MIPIDSI , 0, IORESOURCE_IRQ)
};
#elif defined(CONFIG_ARCH_SUN8IW5P1)
static struct resource disp_resource[] =
{
	/*            name          start                        end                                 flags    */
	DISP_RESOURCE(fe0      ,SUNXI_DE_FE0_VBASE        , SUNXI_DE_FE0_VBASE        + 0x22c   , IORESOURCE_MEM)
	DISP_RESOURCE(be0      ,SUNXI_DE_BE0_VBASE        , SUNXI_DE_BE0_VBASE        + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(lcd0     ,SUNXI_LCD0_VBASE          , SUNXI_LCD0_VBASE          + 0x3fc   , IORESOURCE_MEM)
	DISP_RESOURCE(drc0     ,SUNXI_DRC0_VBASE          , SUNXI_DRC0_VBASE          + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0     ,SUNXI_MIPI_DSI0_VBASE     , SUNXI_MIPI_DSI0_VBASE     + 0x2fc   , IORESOURCE_MEM)
	DISP_RESOURCE(dsi0_dphy,SUNXI_MIPI_DSI0_PHY_VBASE , SUNXI_MIPI_DSI0_PHY_VBASE + 0xfc    , IORESOURCE_MEM)
	DISP_RESOURCE(ccmu     ,SUNXI_CCM_VBASE           , SUNXI_CCM_VBASE           + 0x2dc   , IORESOURCE_MEM)
	DISP_RESOURCE(pioc     ,SUNXI_PIO_VBASE           , SUNXI_PIO_VBASE           + 0x27c   , IORESOURCE_MEM)
	DISP_RESOURCE(pwm      ,SUNXI_PWM_VBASE           , SUNXI_PWM_VBASE           + 0x3c    , IORESOURCE_MEM)
	DISP_RESOURCE(wb0      ,SUNXI_DRC0_VBASE + 0x200  , SUNXI_DRC0_VBASE          + 0x2fc   , IORESOURCE_MEM)
	DISP_RESOURCE(sat0     ,SUNXI_SAT_VBASE           , SUNXI_SAT_VBASE           + 0x2fc   , IORESOURCE_MEM)

	/*            name    irq_no                  flags     */
	DISP_RESOURCE(be0 , SUNXI_IRQ_DEBE0    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(lcd0, SUNXI_IRQ_LCD0    , 0, IORESOURCE_IRQ)
	DISP_RESOURCE(dsi0, SUNXI_IRQ_MIPIDSI    , 0, IORESOURCE_IRQ)
};
#else
static struct resource disp_resource[] =
{
};
#endif

#ifdef FB_RESERVED_MEM
void *disp_malloc(u32 num_bytes, u32 *phys_addr)
{
	u32 actual_bytes;
	void* address = NULL;

	if(num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);
		address = sunxi_buf_alloc(actual_bytes, phys_addr);
		if (address) {
			__inf("disp_malloc ok, address=0x%x, size=0x%x\n", *phys_addr, num_bytes);
			return address;
		} else {
			__wrn("disp_malloc fail, size=0x%x\n", num_bytes);
			return NULL;
		}
	}

#if 0
#if 1
	if(num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);
		address = dma_alloc_coherent(NULL, actual_bytes, (dma_addr_t*)phys_addr, GFP_KERNEL);
		if(address)	{
			__inf("dma_alloc_coherent ok, address=0x%x, size=0x%x\n", *phys_addr, num_bytes);
			return address;
		}
		__wrn("dma_alloc_coherent fail, size=0x%x\n", num_bytes);
	} else {
		__wrn("disp_malloc size is zero\n");
	}
#else
	if(num_bytes != 0) {
		actual_bytes = MY_BYTE_ALIGN(num_bytes);
		*phys_addr = sunxi_mem_alloc(actual_bytes);
		if(*phys_addr) {
			__inf("sunxi_mem_alloc ok, address=0x%x, size=0x%x\n", *phys_addr, num_bytes);
#if defined(CONFIG_ARCH_SUN6I)
			address = (void*)ioremap(*phys_addr, actual_bytes);
#else
			address = sunxi_map_kernel(*phys_addr, actual_bytes);
#endif
			if(address) {
				pr_info("sunxi_map_kernel ok, phys_addr=0x%x, size=0x%x, virt_addr=0x%x\n", (unsigned int)*phys_addr, (unsigned int)num_bytes, (unsigned int)address);
			} else {
				__wrn("sunxi_map_kernel fail, phys_addr=0x%x, size=0x%x, virt_addr=0x%x\n", (unsigned int)*phys_addr, (unsigned int)num_bytes, (unsigned int)address);
			}
			return address;
		}
		__wrn("sunxi_mem_alloc fail, size=0x%x\n", num_bytes);
	} else {
		__wrn("disp_malloc size is zero\n");
	}

#endif
#endif
	return NULL;
}

void  disp_free(void *virt_addr, void* phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if(phys_addr && virt_addr)
		sunxi_buf_free(virt_addr, (unsigned int)phys_addr, actual_bytes);

#if 0
#if 1
	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if(phys_addr && virt_addr) {
		dma_free_coherent(NULL, actual_bytes, virt_addr, (dma_addr_t)phys_addr);
	}
#else
	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if(virt_addr) {
#if defined(CONFIG_ARCH_SUN6I)
		iounmap((void __iomem *)virt_addr);
#else
		sunxi_unmap_kernel((void*)virt_addr);
#endif
	}
	if(phys_addr) {
#if defined(CONFIG_ARCH_SUN6I)
		sunxi_mem_free((unsigned int)phys_addr);
#else
		sunxi_mem_free((unsigned int)phys_addr, actual_bytes);
#endif
	}
#endif
#endif

	return ;
}
#endif

s32 drv_lcd_enable(u32 sel)
{
	u32 i = 0;
	disp_lcd_flow *flow;

	mutex_lock(&g_fbi.mlock);
	if(bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0))	{
		bsp_disp_lcd_pre_enable(sel);

		if(0 == (g_sw_init_para.sw_init_flag & (1 << sel))) {
			flow = bsp_disp_lcd_get_open_flow(sel);
			for(i=0; i<flow->func_num; i++)	{
				flow->func[i].func(sel);
				pr_info("[LCD]open, step %d finish\n", i);

				msleep_interruptible(flow->func[i].delay);
			}
		} else {
			bsp_disp_lcd_tcon_enable(sel);
			bsp_disp_lcd_pin_cfg(sel, 1);
		}
		bsp_disp_lcd_post_enable(sel);

		g_disp_drv.b_lcd_enabled[sel] = 1;
	}
	mutex_unlock(&g_fbi.mlock);

	return 0;
}

s32 drv_lcd_enable_except_backlight(u32 sel)
{
	u32 i = 0;
	disp_lcd_flow *flow;

	mutex_lock(&g_fbi.mlock);
	if(bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0))	{
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num-1; i++)	{
			flow->func[i].func(sel);
			pr_info("[LCD]open, step %d finish\n", i);

			msleep_interruptible(flow->func[i].delay);
		}
		bsp_disp_lcd_post_enable(sel);

		g_disp_drv.b_lcd_enabled[sel] = 1;
	}
	mutex_unlock(&g_fbi.mlock);

	return 0;
}

s32 drv_lcd_disable(u32 sel)
{
	u32 i = 0;
	disp_lcd_flow *flow;

	mutex_lock(&g_fbi.mlock);
	if(bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 1))	{
		bsp_disp_lcd_pre_disable(sel);

		flow = bsp_disp_lcd_get_close_flow(sel);
		for(i=0; i<flow->func_num; i++)	{
			flow->func[i].func(sel);
			pr_info("[LCD]close, step %d finish\n", i);

			msleep_interruptible(flow->func[i].delay);
		}
		bsp_disp_lcd_post_disable(sel);

		g_disp_drv.b_lcd_enabled[sel] = 0;
	}
	mutex_unlock(&g_fbi.mlock);

	return 0;
}

#if defined(CONFIG_ARCH_SUN9IW1P1)
s32 disp_set_hdmi_func(u32 screen_id, disp_hdmi_func * func)
{
	return bsp_disp_set_hdmi_func(screen_id, func);
}

//FIXME
s32 disp_set_hdmi_hpd(u32 hpd)
{
	//bsp_disp_set_hdmi_hpd(hpd);

	return 0;
}
#endif
static void resume_work_0(struct work_struct *work)
{
#if 0
	u32 i = 0;
	disp_lcd_flow *flow;
	u32 sel = 0;

	if(bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0))	{
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num-1; i++)	{
			flow->func[i].func(sel);

			msleep_interruptible(flow->func[i].delay);
		}
	}

	g_disp_drv.b_lcd_enabled[sel] = 1;
#else
	drv_lcd_enable_except_backlight(0);
#endif
}

static void resume_work_1(struct work_struct *work)
{
#if 0
	disp_lcd_flow *flow;
	u32 sel = 1;
	u32 i;

	if(bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num-1; i++)	{
			flow->func[i].func(sel);

			msleep_interruptible(flow->func[i].delay);
		}
	}

	g_disp_drv.b_lcd_enabled[sel] = 1;
#else
	drv_lcd_enable_except_backlight(1);
#endif
}

static void resume_work_2(struct work_struct *work)
{
#if 0
	disp_lcd_flow *flow;
	u32 sel = 1;
	u32 i;

	if(bsp_disp_lcd_is_used(sel) && (g_disp_drv.b_lcd_enabled[sel] == 0)) {
		bsp_disp_lcd_pre_enable(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num-1; i++)	{
			flow->func[i].func(sel);

			msleep_interruptible(flow->func[i].delay);
		}
	}

	g_disp_drv.b_lcd_enabled[sel] = 1;
#else
	drv_lcd_enable_except_backlight(2);
#endif
}

static void start_work(struct work_struct *work)
{
	int num_screens;
	int screen_id;

	num_screens = bsp_disp_feat_get_num_screens();

	for(screen_id = 0; screen_id<num_screens; screen_id++) {
		__inf("sel=%d, output_type=%d, lcd_reg=%d, hdmi_reg=%d\n", screen_id,
		g_fbi.disp_init.output_type[screen_id], bsp_disp_get_lcd_registered(screen_id), bsp_disp_get_hdmi_registered());
		if(((g_fbi.disp_init.disp_mode	== DISP_INIT_MODE_SCREEN0) && (screen_id == 0))
			|| ((g_fbi.disp_init.disp_mode	== DISP_INIT_MODE_SCREEN1) && (screen_id == 1))) {
			if((g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_LCD)) {
				if(bsp_disp_get_lcd_registered(screen_id) && bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_LCD) {
					drv_lcd_enable(screen_id);
				}
			}
			else if(g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
				if(bsp_disp_get_hdmi_registered() && bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_HDMI) {
				__inf("hdmi register\n");
				  if(g_sw_init_para.sw_init_flag && (g_sw_init_para.output_channel == screen_id)) {
					bsp_disp_shadow_protect(screen_id, 1);
					bsp_disp_hdmi_set_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
					bsp_disp_hdmi_enable(screen_id);
					g_sw_init_para.sw_init_flag &= ~(1 << screen_id);
					bsp_disp_shadow_protect(screen_id, 0);
				  } else {
					bsp_disp_hdmi_set_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
					bsp_disp_hdmi_enable(screen_id);
				  }
				}
#endif
			}
			else if(g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_TV) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
				if(bsp_disp_get_lcd_registered(screen_id)
					&& (bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_LCD)
					&& (DISP_OUTPUT_TYPE_TV == bsp_disp_get_lcd_output_type(screen_id))) {
					if(g_sw_init_para.sw_init_flag && (g_sw_init_para.output_channel == screen_id)) {
						bsp_disp_shadow_protect(screen_id, 1);
						bsp_disp_lcd_set_tv_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
						drv_lcd_enable(screen_id);
						g_sw_init_para.sw_init_flag &= ~(1 << screen_id);
						bsp_disp_shadow_protect(screen_id, 0);
					} else {
						bsp_disp_lcd_set_tv_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
						drv_lcd_enable(screen_id);
					}
				}
#endif
			}
		}
		else if((g_fbi.disp_init.disp_mode == DISP_INIT_MODE_SCREEN2) && (screen_id == 2)
				&& (bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_LCD) && bsp_disp_get_lcd_registered(screen_id)) {
				drv_lcd_enable(screen_id);
		}
	}
}

static s32 start_process(void)
{
	flush_work(&g_fbi.start_work);
	schedule_work(&g_fbi.start_work);

	return 0;
}

s32 get_para_from_cmdline(const char *cmdline, const char *name, char *value)
{
    char *p = cmdline;
    char *value_p = value;

    if(!cmdline || !name || !value){
        return -1;
    }

    for(; *p != 0;){
        if(*p++ == ' '){
            if(0 == strncmp(p, name, strlen(name))){
                p += strlen(name);
                if(*p++ != '='){
                    continue;
                }
                while(*p != 0 && *p != ' '){
                    *value_p++ = *p++;
                }
                *value_p = '\0';
                return value_p - value;
            }
        }
    }

    return 0;
}

static u32 disp_parse_cmdline(__disp_bsp_init_para *para)
{
    char val[16] = {0};
    u32 value;

    //get the parameters of display_resolution from cmdline passed by boot
    if(0 < get_para_from_cmdline(saved_command_line, "disp_rsl",val)){
        value = simple_strtoul(val, 0, 16);
    } else {
        value = 0x0;
    }
#if !defined(CONFIG_HOMLET_PLATFORM)
		value = 0x0;
#endif
    para->sw_init_para = &g_sw_init_para;
    g_sw_init_para.disp_rsl = value;
    g_sw_init_para.closed = (value >> 24) & 0xFF;
    g_sw_init_para.output_type = (value >> 16) & 0xFF;
    g_sw_init_para.output_mode = (value >> 8) & 0xFF;
    g_sw_init_para.output_channel = value & 0xFF;

    return 0;
}

static u32 disp_para_check(__disp_bsp_init_para * para)
{
    u8 valid = 1;
    u8 type = DISP_OUTPUT_TYPE_HDMI
              | DISP_OUTPUT_TYPE_TV; //support hdmi and cvbs
            //| DISP_OUTPUT_TYPE_LCD
            //| DISP_OUTPUT_TYPE_VGA;
    valid &= ((type & para->sw_init_para->output_type) ? 1 : 0);
    if((DISP_OUTPUT_TYPE_HDMI == para->sw_init_para->output_type)
        || (DISP_OUTPUT_TYPE_TV == para->sw_init_para->output_type)) {
        valid &= ((DISP_TV_MODE_NUM > para->sw_init_para->output_mode) ? 1 : 0);
    } else if(DISP_OUTPUT_TYPE_VGA == para->sw_init_para->output_type){
        //todo
    }
    if((0 != valid) && (0 == para->sw_init_para->closed)) {
        para->sw_init_para->sw_init_flag |= (((u8)1) << para->sw_init_para->output_channel);
    } else {
        para->sw_init_para->sw_init_flag = 0;
    }
    return 0;
}

u32 disp_get_disp_rsl(void) //export_symbol
{
    return g_sw_init_para.disp_rsl;
}

s32 disp_register_sync_proc(void (*proc)(u32))
{
	struct proc_list *new_proc;

	new_proc = (struct proc_list*)OSAL_malloc(sizeof(struct proc_list));
	if(new_proc) {
		new_proc->proc = proc;
		list_add_tail(&(new_proc->list), &(g_fbi.sync_proc_list.list));
	} else {
		pr_warn("malloc fail in %s\n", __func__);
	}

	return 0;
}

s32 disp_unregister_sync_proc(void (*proc)(u32))
{
	struct proc_list *ptr;

	if((NULL == proc)) {
		pr_warn("hdl is NULL in %s\n", __func__);
		return -1;
	}
	list_for_each_entry(ptr, &g_fbi.sync_proc_list.list, list) {
		if(ptr->proc == proc) {
			list_del(&ptr->list);
			OSAL_free((void*)ptr);
			return 0;
		}
	}

	return -1;
}

s32 disp_register_sync_finish_proc(void (*proc)(u32))
{
	struct proc_list *new_proc;

	new_proc = (struct proc_list*)OSAL_malloc(sizeof(struct proc_list));
	if(new_proc) {
		new_proc->proc = proc;
		list_add_tail(&(new_proc->list), &(g_fbi.sync_finish_proc_list.list));
	} else {
		pr_warn("malloc fail in %s\n", __func__);
	}

	return 0;
}

s32 disp_unregister_sync_finish_proc(void (*proc)(u32))
{
	struct proc_list *ptr;

	if((NULL == proc)) {
		pr_warn("hdl is NULL in %s\n", __func__);
		return -1;
	}
	list_for_each_entry(ptr, &g_fbi.sync_finish_proc_list.list, list) {
		if(ptr->proc == proc) {
			list_del(&ptr->list);
			OSAL_free((void*)ptr);
			return 0;
		}
	}

	return -1;
}

static s32 disp_sync_finish_process(u32 screen_id)
{
	struct proc_list *ptr;

	list_for_each_entry(ptr, &g_fbi.sync_finish_proc_list.list, list) {
		if(ptr->proc)
			ptr->proc(screen_id);
	}

	return 0;
}

s32 disp_register_ioctl_func(unsigned int cmd, int (*proc)(unsigned int cmd, unsigned long arg))
{
	struct ioctl_list *new_proc;

	new_proc = (struct ioctl_list*)OSAL_malloc(sizeof(struct ioctl_list));
	if(new_proc) {
		new_proc->cmd = cmd;
		new_proc->func = proc;
		list_add_tail(&(new_proc->list), &(g_fbi.ioctl_extend_list.list));
	} else {
		pr_warn("malloc fail in %s\n", __func__);
	}

	return 0;
}

s32 disp_unregister_ioctl_func(unsigned int cmd)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_fbi.ioctl_extend_list.list, list) {
		if(ptr->cmd == cmd) {
			list_del(&ptr->list);
			OSAL_free((void*)ptr);
			return 0;
		}
	}

	pr_warn("no ioctl found(cmd:0x%x) in %s\n", cmd, __func__);
	return -1;
}

static s32 disp_ioctl_extend(unsigned int cmd, unsigned long arg)
{
	struct ioctl_list *ptr;

	list_for_each_entry(ptr, &g_fbi.ioctl_extend_list.list, list) {
		if(cmd == ptr->cmd)
			return ptr->func(cmd, arg);
	}

	return -1;
}

s32 disp_register_standby_func(int (*suspend)(void), int (*resume)(void))
{
	struct standby_cb_list *new_proc;

	new_proc = (struct standby_cb_list*)OSAL_malloc(sizeof(struct standby_cb_list));
	if(new_proc) {
		new_proc->suspend = suspend;
		new_proc->resume = resume;
		list_add_tail(&(new_proc->list), &(g_fbi.stb_cb_list.list));
	} else {
		pr_warn("malloc fail in %s\n", __func__);
	}

	return 0;
}

s32 disp_unregister_standby_func(int (*suspend)(void), int (*resume)(void))
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_fbi.stb_cb_list.list, list) {
		if((ptr->suspend == suspend) && (ptr->resume == resume)) {
			list_del(&ptr->list);
			OSAL_free((void*)ptr);
			return 0;
		}
	}

	return -1;
}

static s32 disp_suspend_cb(void)
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_fbi.stb_cb_list.list, list) {
		if(ptr->suspend)
			return ptr->suspend();
	}

	return -1;
}

static s32 disp_resume_cb(void)
{
	struct standby_cb_list *ptr;

	list_for_each_entry(ptr, &g_fbi.stb_cb_list.list, list) {
		if(ptr->resume)
			return ptr->resume();
	}

	return -1;
}

s32 DRV_DISP_Init(void)
{
	__disp_bsp_init_para para;
	int i;

	__inf("DRV_DISP_Init !\n");

	init_waitqueue_head(&g_fbi.wait[0]);
	init_waitqueue_head(&g_fbi.wait[1]);
	init_waitqueue_head(&g_fbi.wait[2]);
	g_fbi.wait_count[0] = 0;
	g_fbi.wait_count[1] = 0;
	g_fbi.wait_count[2] = 0;
	INIT_WORK(&g_fbi.resume_work[0], resume_work_0);
	INIT_WORK(&g_fbi.resume_work[1], resume_work_1);
	INIT_WORK(&g_fbi.resume_work[2], resume_work_2);
	INIT_WORK(&g_fbi.start_work, start_work);
	INIT_LIST_HEAD(&g_fbi.sync_proc_list.list);
	INIT_LIST_HEAD(&g_fbi.sync_finish_proc_list.list);
	INIT_LIST_HEAD(&g_fbi.ioctl_extend_list.list);
	INIT_LIST_HEAD(&g_fbi.stb_cb_list.list);
	mutex_init(&g_fbi.mlock);

	memset(&para, 0, sizeof(__disp_bsp_init_para));

	for(i=0; i<DISP_MOD_NUM; i++)	{
		para.reg_base[i] = (u32)g_fbi.reg_base[i];
		para.reg_size[i] = (u32)g_fbi.reg_size[i];
		para.irq_no[i]      = g_fbi.irq_no[i];
	}

	para.disp_int_process       = disp_sync_finish_process;
	para.vsync_event            = DRV_disp_vsync_event;
	para.start_process          = start_process;
	para.capture_event          = capture_event;

	disp_parse_cmdline(&para);
	disp_para_check(&para); // check the parameters parsed from cmdline
	//printk("<0>""[disp] para[%d,%d,%d,%d,%d]\n", para.sw_init_para->output_type,
	//	para.sw_init_para->output_mode,para.sw_init_para->output_channel,
	//	para.sw_init_para->sw_init_flag, para.sw_init_para->mod_finish_flag);

	memset(&g_disp_drv, 0, sizeof(disp_drv_info));

	bsp_disp_init(&para);
	bsp_disp_open();

	start_process();

	__inf("DRV_DISP_Init end\n");
	return 0;
}

s32 DRV_DISP_Exit(void)
{
	Fb_Exit();
	bsp_disp_close();
	bsp_disp_exit(g_disp_drv.exit_mode);
	return 0;
}


static int disp_mem_request(int sel,u32 size)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = 0;
	struct page *page;

	if(g_disp_mm[sel].info_base != 0)
	return -EINVAL;

	g_disp_mm[sel].mem_len = size;
	map_size = PAGE_ALIGN(g_disp_mm[sel].mem_len);

	page = alloc_pages(GFP_KERNEL,get_order(map_size));
	if(page != NULL) {
		g_disp_mm[sel].info_base = page_address(page);
		if(g_disp_mm[sel].info_base == 0)	{
			free_pages((unsigned long)(page),get_order(map_size));
			__wrn("page_address fail!\n");
			return -ENOMEM;
		}
		g_disp_mm[sel].mem_start = virt_to_phys(g_disp_mm[sel].info_base);
		memset(g_disp_mm[sel].info_base,0,size);

		__inf("pa=0x%08lx va=0x%p size:0x%x\n",g_disp_mm[sel].mem_start, g_disp_mm[sel].info_base, size);
		return 0;
	}	else {
		__wrn("alloc_pages fail!\n");
		return -ENOMEM;
	}
#else
	u32 ret = 0;
	u32 phy_addr;

	ret = (u32)disp_malloc(size, &phy_addr);
	if(ret != 0) {
		g_disp_mm[sel].info_base = (void*)ret;
		g_disp_mm[sel].mem_start = phy_addr;
		g_disp_mm[sel].mem_len = size;
		memset(g_disp_mm[sel].info_base,0,size);
		__inf("pa=0x%08lx va=0x%p size:0x%x\n",g_disp_mm[sel].mem_start, g_disp_mm[sel].info_base, size);

		return 0;
	}	else {
		__wrn("disp_malloc fail!\n");
		return -ENOMEM;
	}
#endif
}

static int disp_mem_release(int sel)
{
#ifndef FB_RESERVED_MEM
	unsigned map_size = PAGE_ALIGN(g_disp_mm[sel].mem_len);
	unsigned page_size = map_size;

	if(g_disp_mm[sel].info_base == 0)
		return -EINVAL;

	free_pages((unsigned long)(g_disp_mm[sel].info_base),get_order(page_size));
	memset(&g_disp_mm[sel],0,sizeof(struct info_mm));
#else
	if(g_disp_mm[sel].info_base == NULL)
		return -EINVAL;

	__inf("disp_mem_release, mem_id=%d, phy_addr=0x%x\n", sel, (unsigned int)g_disp_mm[sel].mem_start);
	disp_free((void *)g_disp_mm[sel].info_base, (void*)g_disp_mm[sel].mem_start, g_disp_mm[sel].mem_len);
	memset(&g_disp_mm[sel],0,sizeof(struct info_mm));
#endif
  return 0;
}

extern s32   dsi_clk_enable(u32 sel, u32 en);
extern s32   dsi_dcs_wr(u32 sel,u8 cmd,u8* para_p,u32 para_num);
int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops)
{
	src_ops->sunxi_lcd_delay_ms = bsp_disp_lcd_delay_ms;
	src_ops->sunxi_lcd_delay_us = bsp_disp_lcd_delay_us;
	src_ops->sunxi_lcd_tcon_enable = bsp_disp_lcd_tcon_enable;
	src_ops->sunxi_lcd_tcon_disable = bsp_disp_lcd_tcon_disable;
	src_ops->sunxi_lcd_pwm_enable = bsp_disp_lcd_pwm_enable;
	src_ops->sunxi_lcd_pwm_disable = bsp_disp_lcd_pwm_disable;
	src_ops->sunxi_lcd_backlight_enable = bsp_disp_lcd_backlight_enable;
	src_ops->sunxi_lcd_backlight_disable = bsp_disp_lcd_backlight_disable;
	src_ops->sunxi_lcd_power_enable = bsp_disp_lcd_power_enable;
	src_ops->sunxi_lcd_power_disable = bsp_disp_lcd_power_disable;
	src_ops->sunxi_lcd_set_panel_funs = bsp_disp_lcd_set_panel_funs;
	src_ops->sunxi_lcd_dsi_write = dsi_dcs_wr;
	src_ops->sunxi_lcd_dsi_clk_enable = dsi_clk_enable;
	src_ops->sunxi_lcd_pin_cfg = bsp_disp_lcd_pin_cfg;
	src_ops->sunxi_lcd_gpio_set_value = bsp_disp_lcd_gpio_set_value;
	src_ops->sunxi_lcd_gpio_set_direction = bsp_disp_lcd_gpio_set_direction;
	return 0;
}

int disp_mmap(struct file *file, struct vm_area_struct * vma)
{
	unsigned long mypfn = vma->vm_pgoff;
	unsigned long vmsize = vma->vm_end-vma->vm_start;
	vma->vm_pgoff = 0;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if(remap_pfn_range(vma,vma->vm_start,mypfn,vmsize,vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

int disp_open(struct inode *inode, struct file *file)
{
	return 0;
}

int disp_release(struct inode *inode, struct file *file)
{
	return 0;
}
ssize_t disp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

ssize_t disp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static int __devinit disp_probe(struct platform_device *pdev)
{
	fb_info_t * info = NULL;
	int i;
	struct resource	*res;

	__inf("[DISP]disp_probe\n");

	info = &g_fbi;
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev,info);

	for(i=0; i<sizeof(disp_mod)/sizeof(struct sunxi_disp_mod); i++)	{
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, disp_mod[i].name);
		if(res != NULL) {
			info->reg_base[disp_mod[i].id] = res->start;
			info->reg_size[disp_mod[i].id] = res->end - res->start;
			__inf("%s(%d), reg_base=0x%x\n", disp_mod[i].name, disp_mod[i].id, info->reg_base[disp_mod[i].id]);
		}
	}

	for(i=0; i<sizeof(disp_mod)/sizeof(struct sunxi_disp_mod); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, disp_mod[i].name);
		if(res != NULL)	{
			info->irq_no[disp_mod[i].id] = res->start;
			__inf("%s(%d), irq_no=%d\n", disp_mod[i].name, disp_mod[i].id, info->irq_no[disp_mod[i].id]);
		}
	}
	DRV_DISP_Init();
	Fb_Init();

	__inf("[DISP]disp_probe finish\n");

	return 0;
}

static int disp_remove(struct platform_device *pdev)
{
	pr_info("disp_remove call\n");

	platform_set_drvdata(pdev, NULL);

	return 0;
}

void suspend(){
	u32 screen_id = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	//close the screen , then cleanup register
#if defined(CONFIG_ARCH_SUN9IW1P1)
	for(screen_id=0; screen_id<num_screens; screen_id++) {
		bsp_disp_hdmi_suspend(screen_id);
	}
#endif

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		suspend_output_type[screen_id] = bsp_disp_get_output_type(screen_id);
		if(suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_LCD) {
			/* resume --> suspend */
			if(2 == suspend_prestep){
				flush_work(&g_fbi.resume_work[screen_id]);
			}
			drv_lcd_disable(screen_id);
		} else if(suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_disable(screen_id);
#endif
		}
	}

}

void resume(){
	u32 screen_id = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	//restore the register , then open the screen.
#if defined(CONFIG_ARCH_SUN9IW1P1)
	if(0 == suspend_prestep) {
		//FIXME
		u32 temp;
		temp = readl((void __iomem *)(SUNXI_CCM_MOD_VBASE + 0x1a8));
		temp = temp | 0x4;
		writel(temp, (void __iomem *)(SUNXI_CCM_MOD_VBASE + 0x1a8));
	}
#endif

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		if(suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_LCD) {
			if(0 == suspend_prestep || 2 == suspend_prestep) {
				/* early_suspend -->  late_resume or resume -- > late_resume */
				drv_lcd_enable(screen_id);
			} else if(1 == suspend_prestep){
				/* suspend --> resume */
				schedule_work(&g_fbi.resume_work[screen_id]);
			}
		} else if(suspend_output_type[screen_id] == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_enable(screen_id);
#endif
		}
	}

#if defined(CONFIG_ARCH_SUN9IW1P1)
	for(screen_id=0; screen_id<num_screens; screen_id++) {
		bsp_disp_hdmi_resume(screen_id);
	}
#endif
}

#if defined(CONFIG_HAS_EARLYSUSPEND)
void backlight_early_suspend(struct early_suspend *h)
{
	pr_info("backlight_early_suspend\n");
	msleep(300);
	suspend();
	suspend_status |= DISPLAY_LIGHT_SLEEP;
	suspend_prestep = 0;
	pr_info("backlight_early_suspend finish\n");
}

void backlight_late_resume(struct early_suspend *h)
{
	pr_info("backlight_late_resume\n");
	resume();
	suspend_status &= (~DISPLAY_LIGHT_SLEEP);
	suspend_prestep = 3;
	pr_info("backlight_late_resume finish\n");
}

static struct early_suspend backlight_early_suspend_handler =
{
  .level   = EARLY_SUSPEND_LEVEL_DISABLE_FB + 200,
	.suspend = backlight_early_suspend,
	.resume = backlight_late_resume,
};
#endif

int disp_suspend(struct platform_device *pdev, pm_message_t state)
{
	pr_info("disp_suspend\n");

	//if not define CONFIG_HAS_EARLYSUSPEND, we must run suspend() again.
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	msleep(300);
	suspend();
#endif
	//clear the display data.
	disp_suspend_cb();
	suspend_status |= DISPLAY_DEEP_SLEEP;
	suspend_prestep = 1;
	pr_info("disp_suspend finish\n");

	return 0;
}


int disp_resume(struct platform_device *pdev)
{
	pr_info("disp_resume\n");
	disp_resume_cb();
#if !defined(CONFIG_HAS_EARLYSUSPEND)
	resume();
#endif
	pr_info("disp_resume finish\n");
	suspend_status &= (~DISPLAY_DEEP_SLEEP);
	suspend_prestep = 2;
	return 0;
}

static void disp_shutdown(struct platform_device *pdev)
{
	u32 type = 0, screen_id = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for(screen_id=0; screen_id<num_screens; screen_id++) {
		type = bsp_disp_get_output_type(screen_id);
		if(type == DISP_OUTPUT_TYPE_LCD) {
			drv_lcd_disable(screen_id);
		} else if(type == DISP_OUTPUT_TYPE_HDMI) {
#if defined(CONFIG_ARCH_SUN9IW1P1)
			bsp_disp_hdmi_disable(screen_id);
#endif
		}
	}

	return ;
}

long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long karg[4];
	unsigned long ubuffer[4] = {0};
	int ret = 0;
	int num_screens = 2;

	num_screens = bsp_disp_feat_get_num_screens();

	if (copy_from_user((void*)karg,(void __user*)arg,4*sizeof(unsigned long))) {
		__wrn("copy_from_user fail\n");
		return -EFAULT;
	}

	ubuffer[0] = *(unsigned long*)karg;
	ubuffer[1] = (*(unsigned long*)(karg+1));
	ubuffer[2] = (*(unsigned long*)(karg+2));
	ubuffer[3] = (*(unsigned long*)(karg+3));

	if(cmd < DISP_CMD_FB_REQUEST)	{
		if(ubuffer[0] >= num_screens) {
			__wrn("para err in disp_ioctl, cmd = 0x%x,screen id = %d\n", cmd, (int)ubuffer[0]);
			return -1;
		}
	}
	if(DISPLAY_DEEP_SLEEP & suspend_status) {
		__wrn("ioctl:%x fail when in suspend!\n", cmd);
		return -1;
	}

	if(cmd == disp_cmd_print) {
		OSAL_PRINTF("cmd:0x%x,%ld,%ld\n",cmd, ubuffer[0], ubuffer[1]);
	}

	switch(cmd)	{
	//----disp global----
	case DISP_CMD_SET_BKCOLOR:
	{
		disp_color_info para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_color_info)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_set_back_color(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_SET_COLORKEY:
	{
		disp_colorkey para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_colorkey)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_set_color_key(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_GET_OUTPUT_TYPE:
		if(DISPLAY_NORMAL == suspend_status)	{
			ret =  bsp_disp_get_output_type(ubuffer[0]);
		}	else {
			ret = suspend_output_type[ubuffer[0]];
		}
#if defined(CONFIG_HOMLET_PLATFORM)
		if(DISP_OUTPUT_TYPE_LCD == ret) {
			ret = bsp_disp_get_lcd_output_type(ubuffer[0]);
		}
#endif
		break;

	case DISP_CMD_GET_SCN_WIDTH:
		ret = bsp_disp_get_screen_width(ubuffer[0]);
		break;

	case DISP_CMD_GET_SCN_HEIGHT:
		ret = bsp_disp_get_screen_height(ubuffer[0]);
		break;

	case DISP_CMD_SHADOW_PROTECT:
		ret = bsp_disp_shadow_protect(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_VSYNC_EVENT_EN:
		ret = bsp_disp_vsync_event_enable(ubuffer[0], ubuffer[1]);
		break;

	//----layer----
	case DISP_CMD_LAYER_ENABLE:
		ret = bsp_disp_layer_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_DISABLE:
		ret = bsp_disp_layer_disable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_INFO:
	{
		disp_layer_info para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(disp_layer_info))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_layer_set_info(ubuffer[0], ubuffer[1], &para);
		break;
	}

	case DISP_CMD_LAYER_GET_INFO:
	{
		disp_layer_info para;

		ret = bsp_disp_layer_get_info(ubuffer[0], ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2],&para, sizeof(disp_layer_info))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_LAYER_GET_FRAME_ID:
		ret = bsp_disp_layer_get_frame_id(ubuffer[0], ubuffer[1]);
		break;

	//----lcd----
	case DISP_CMD_LCD_ENABLE:
		ret = drv_lcd_enable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_LCD;

		break;

	case DISP_CMD_LCD_DISABLE:
		ret = drv_lcd_disable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_LCD_SET_BRIGHTNESS:
		ret = bsp_disp_lcd_set_bright(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LCD_GET_BRIGHTNESS:
		ret = bsp_disp_lcd_get_bright(ubuffer[0]);
		break;

	case DISP_CMD_LCD_BACKLIGHT_ENABLE:
		if(DISPLAY_NORMAL == suspend_status) {
			ret = bsp_disp_lcd_backlight_enable(ubuffer[0]);
		}
		break;

	case DISP_CMD_LCD_BACKLIGHT_DISABLE:
		if(DISPLAY_NORMAL == suspend_status) {
			ret = bsp_disp_lcd_backlight_disable(ubuffer[0]);
		}
		break;
#if defined(CONFIG_ARCH_SUN9IW1P1)
	//----hdmi----
	case DISP_CMD_HDMI_ENABLE:
		ret = bsp_disp_hdmi_enable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_HDMI;
		break;

	case DISP_CMD_HDMI_DISABLE:
		ret = bsp_disp_hdmi_disable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_HDMI_SET_MODE:
		ret = bsp_disp_hdmi_set_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_HDMI_GET_MODE:
		ret = bsp_disp_hdmi_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_HDMI_SUPPORT_MODE:
		ret = bsp_disp_hdmi_check_support_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_HDMI_GET_HPD_STATUS:
		if(DISPLAY_NORMAL == suspend_status) {
			ret = bsp_disp_hdmi_get_hpd_status(ubuffer[0]);
		}	else {
			ret = 0;
		}
		break;

#if 0
	case DISP_CMD_HDMI_SET_SRC:
		ret = bsp_disp_hdmi_set_src(ubuffer[0], (disp_lcd_src)ubuffer[1]);
		break;
#endif
	//----framebuffer----
	case DISP_CMD_FB_REQUEST:
	{
		disp_fb_create_info para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_fb_create_info))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = Display_Fb_Request(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_FB_RELEASE:
	ret = Display_Fb_Release(ubuffer[0]);
	break;
#if 0
	case DISP_CMD_FB_GET_PARA:
	{
		disp_fb_create_info para;

		ret = Display_Fb_get_para(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(disp_fb_create_info))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_GET_DISP_INIT_PARA:
	{
		disp_init_para para;

		ret = Display_get_disp_init_para(&para);
		if(copy_to_user((void __user *)ubuffer[0],&para, sizeof(disp_init_para)))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

#endif
#endif

	//capture
	case DISP_CMD_CAPTURE_SCREEN:
		ret = bsp_disp_capture_screen(ubuffer[0], (disp_capture_para*)ubuffer[1]);
		break;

	case DISP_CMD_CAPTURE_SCREEN_STOP:
		ret = bsp_disp_capture_screen_stop(ubuffer[0]);
		break;

	//----enhance----
	case DISP_CMD_SET_BRIGHT:
	ret = bsp_disp_smcl_set_bright(ubuffer[0], ubuffer[1]);
	break;

	case DISP_CMD_GET_BRIGHT:
		ret = bsp_disp_smcl_get_bright(ubuffer[0]);
		break;

	case DISP_CMD_SET_CONTRAST:
		ret = bsp_disp_smcl_set_contrast(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_CONTRAST:
		ret = bsp_disp_smcl_get_contrast(ubuffer[0]);
		break;

	case DISP_CMD_SET_SATURATION:
		ret = bsp_disp_smcl_set_saturation(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_SATURATION:
		ret = bsp_disp_smcl_get_saturation(ubuffer[0]);
		break;

	case DISP_CMD_SET_HUE:
		ret = bsp_disp_smcl_set_hue(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_HUE:
		ret = bsp_disp_smcl_get_hue(ubuffer[0]);
		break;

	case DISP_CMD_ENHANCE_ENABLE:
		ret = bsp_disp_smcl_enable(ubuffer[0]);
		break;

	case DISP_CMD_ENHANCE_DISABLE:
		ret = bsp_disp_smcl_disable(ubuffer[0]);
		break;

	case DISP_CMD_GET_ENHANCE_EN:
		ret = bsp_disp_smcl_is_enabled(ubuffer[0]);
		break;

	case DISP_CMD_SET_ENHANCE_MODE:
		ret = bsp_disp_smcl_set_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_ENHANCE_MODE:
		ret = bsp_disp_smcl_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_SET_ENHANCE_WINDOW:
	{
		disp_window para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_window))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_smcl_set_window(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_GET_ENHANCE_WINDOW:
	{
		disp_window para;

		ret = bsp_disp_smcl_get_window(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(disp_window))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_DRC_ENABLE:
		ret = bsp_disp_smbl_enable(ubuffer[0]);
		break;

	case DISP_CMD_DRC_DISABLE:
		ret = bsp_disp_smbl_disable(ubuffer[0]);
		break;

	case DISP_CMD_GET_DRC_EN:
		ret = bsp_disp_smbl_is_enabled(ubuffer[0]);
		break;

	case DISP_CMD_DRC_SET_WINDOW:
	{
		disp_window para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_window))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_smbl_set_window(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_DRC_GET_WINDOW:
	{
		disp_window para;

		ret = bsp_disp_smbl_get_window(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(disp_window))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

#if defined(CONFIG_ARCH_SUN9IW1P1)
	//---- cursor ----
	case DISP_CMD_CURSOR_ENABLE:
		ret =  bsp_disp_cursor_enable(ubuffer[0]);
		break;

	case DISP_CMD_CURSOR_DISABLE:
		ret =  bsp_disp_cursor_disable(ubuffer[0]);
		break;

	case DISP_CMD_CURSOR_SET_POS:
	{
		disp_position para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_position)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_cursor_set_pos(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_CURSOR_GET_POS:
	{
		disp_position para;

		ret = bsp_disp_cursor_get_pos(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(disp_position)))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_CURSOR_SET_FB:
	{
		disp_cursor_fb para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(disp_cursor_fb)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_cursor_set_fb(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_CURSOR_SET_PALETTE:
		if((ubuffer[1] == 0) || ((int)ubuffer[3] <= 0))	{
			__wrn("para invalid in display ioctrl DISP_CMD_HWC_SET_PALETTE_TABLE,buffer:0x%x, size:0x%x\n", (unsigned int)ubuffer[1], (unsigned int)ubuffer[3]);
			return -1;
		}
		if(copy_from_user(gbuffer, (void __user *)ubuffer[1],ubuffer[3]))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_cursor_set_palette(ubuffer[0], (void*)gbuffer, ubuffer[2], ubuffer[3]);
		break;
#endif
	//----for test----
	case DISP_CMD_MEM_REQUEST:
		ret =  disp_mem_request(ubuffer[0],ubuffer[1]);
		break;

	case DISP_CMD_MEM_RELEASE:
		ret =  disp_mem_release(ubuffer[0]);
		break;

	case DISP_CMD_MEM_SELIDX:
		g_disp_mm_sel = ubuffer[0];
		break;

	case DISP_CMD_MEM_GETADR:
		ret = g_disp_mm[ubuffer[0]].mem_start;
		break;

//	case DISP_CMD_PRINT_REG:
//		ret = bsp_disp_print_reg(1, ubuffer[0], 0);
//		break;

	//----for tv ----
	case DISP_CMD_TV_ON:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = drv_lcd_enable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_LCD;
#endif
		break;
	case DISP_CMD_TV_OFF:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = drv_lcd_disable(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
#endif
		break;
	case DISP_CMD_TV_GET_MODE:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = bsp_disp_lcd_get_tv_mode(ubuffer[0]);
#endif
		break;
	case DISP_CMD_TV_SET_MODE:
#if defined(CONFIG_ARCH_SUN9IW1P1)
		ret = bsp_disp_lcd_set_tv_mode(ubuffer[0], ubuffer[1]);
#endif
		break;

	default:
		ret = disp_ioctl_extend(cmd, (unsigned long)ubuffer);
		break;
	}

  return ret;
}

static const struct file_operations disp_fops = {
	.owner    = THIS_MODULE,
	.open     = disp_open,
	.release  = disp_release,
	.write    = disp_write,
	.read     = disp_read,
	.unlocked_ioctl = disp_ioctl,
	.mmap     = disp_mmap,
};

static struct platform_driver disp_driver = {
	.probe    = disp_probe,
	.remove   = disp_remove,
	.suspend  = disp_suspend,
	.resume   = disp_resume,
	.shutdown = disp_shutdown,
	.driver   =
	{
		.name   = "disp",
		.owner  = THIS_MODULE,
	},
};


static struct platform_device disp_device = {
	.name           = "disp",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(disp_resource),
	.resource       = disp_resource,
	.dev            =
	{
		.power        =
		{
			.async_suspend = 1,
		}
	}
};

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
struct ddrfreq_vb_time_ops
{
    int (*get_vb_time) (void);
    int (*get_next_vb_time)(void);
    int (*is_in_vb)(void);
};

extern int ddrfreq_set_vb_time_ops(struct ddrfreq_vb_time_ops *ops);
static struct ddrfreq_vb_time_ops ddrfreq_ops =
{
	.get_vb_time = bsp_disp_get_vb_time,
	.get_next_vb_time = bsp_disp_get_next_vb_time,
	.is_in_vb = bsp_disp_is_in_vb,
};
#endif

extern int disp_attr_node_init(struct device *display_dev);
extern int capture_module_init(void);
extern void  capture_module_exit(void);
static int __init disp_module_init(void)
{
	int ret = 0, err;

	pr_info("[DISP]disp_module_init\n");

	alloc_chrdev_region(&devid, 0, 1, "disp");
	my_cdev = cdev_alloc();
	cdev_init(my_cdev, &disp_fops);
	my_cdev->owner = THIS_MODULE;
	err = cdev_add(my_cdev, devid, 1);
	if (err) {
		__wrn("cdev_add fail\n");
		return -1;
	}

	disp_class = class_create(THIS_MODULE, "disp");
	if (IS_ERR(disp_class))	{
		__wrn("class_create fail\n");
		return -1;
	}

	display_dev = device_create(disp_class, NULL, devid, NULL, "disp");

	ret = platform_device_register(&disp_device);

	if (ret == 0) {
		ret = platform_driver_register(&disp_driver);
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&backlight_early_suspend_handler);
#endif
	disp_attr_node_init(display_dev);
	capture_module_init();

#ifdef CONFIG_DEVFREQ_DRAM_FREQ_IN_VSYNC
	ddrfreq_set_vb_time_ops(&ddrfreq_ops);
#endif

	pr_info("[DISP]disp_module_init finish\n");

	return ret;
}

static void __exit disp_module_exit(void)
{
	__inf("disp_module_exit\n");

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&backlight_early_suspend_handler);
#endif
	DRV_DISP_Exit();
	capture_module_exit();

	platform_driver_unregister(&disp_driver);
	platform_device_unregister(&disp_device);

	device_destroy(disp_class,  devid);
	class_destroy(disp_class);

	cdev_del(my_cdev);
}
#if defined(CONFIG_ARCH_SUN9IW1P1)
EXPORT_SYMBOL(disp_set_hdmi_func);
EXPORT_SYMBOL(disp_set_hdmi_hpd);
#endif
EXPORT_SYMBOL(sunxi_disp_get_source_ops);
EXPORT_SYMBOL(DRV_DISP_Init);
EXPORT_SYMBOL(disp_get_disp_rsl);

module_init(disp_module_init);
module_exit(disp_module_exit);


MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("display driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:disp");


