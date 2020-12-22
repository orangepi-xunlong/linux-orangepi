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
__disp_drv_t g_disp_drv;

#define MY_BYTE_ALIGN(x) ( ( (x + (4*1024-1)) >> 12) << 12)             /* alloc based on 4K byte */

static unsigned int gbuffer[4096];
static __u32 suspend_output_type[2] = {0,0};
static __u32 suspend_status = 0;//0:normal; suspend_status&1 != 0:in early_suspend; suspend_status&2 != 0:in suspend;
static __u32 suspend_prestep = 0; //0:after early suspend; 1:after suspend; 2:after resume; 3 :after late resume

static struct info_mm  g_disp_mm[10];
static int g_disp_mm_sel = 0;

static struct cdev *my_cdev;
static dev_t devid ;
static struct class *disp_class;
static struct device *display_dev;

__u32 disp_print_cmd_level = 0;
__u32 disp_cmd_print = 0xffff;   //print cmd which eq disp_cmd_print

s32 disp_get_print_cmd_level(void)
{
	return disp_print_cmd_level;
}

s32 disp_set_print_cmd_level(u32 level)
{
	disp_print_cmd_level = level;
	return 0;
}

s32 disp_get_cmd_print(void)
{
	return disp_cmd_print;
}

s32 disp_set_cmd_print(u32 cmd)
{
	disp_cmd_print = cmd;
	return 0;
}

static struct sunxi_disp_mod disp_mod[] = {
	{DISP_MOD_FE0      ,    "fe0"},
	{DISP_MOD_FE1      ,    "fe1"},
	{DISP_MOD_BE0      ,    "be0"},
	{DISP_MOD_BE1      ,    "be1"},
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
};

#if defined CONFIG_ARCH_SUN8IW1P1
static struct resource disp_resource[] =
{
	{
		.start = (int __force)SUNXI_DE_FE0_VBASE,
		.end   = (int __force)SUNXI_DE_FE0_VBASE + 0x22c,
		.flags = IORESOURCE_MEM,
		.name  = "fe0",
	},
	{
		.start = (int __force)SUNXI_DE_FE1_VBASE,
		.end   = (int __force)SUNXI_DE_FE1_VBASE + 0x22c,
		.flags = IORESOURCE_MEM,
		.name  = "fe1",
	},
	{
		.start = (int __force)SUNXI_DE_BE0_VBASE,
		.end   = (int __force)SUNXI_DE_BE0_VBASE + 0x9fc,
		.flags = IORESOURCE_MEM,
		.name  = "be0",
	},
	{
		.start = (int __force)SUNXI_DE_BE1_VBASE,
		.end   = (int __force)SUNXI_DE_BE1_VBASE + 0x9fc,
		.flags = IORESOURCE_MEM,
		.name  = "be1",
	},
	{
		.start = (int __force)SUNXI_LCD0_VBASE,
		.end   = (int __force)SUNXI_LCD0_VBASE + 0x3fc,
		.flags = IORESOURCE_MEM,
		.name  = "lcd0",
	},
	{
		.start = (int __force)SUNXI_LCD1_VBASE,
		.end   = (int __force)SUNXI_LCD1_VBASE + 0x3fc,
		.flags = IORESOURCE_MEM,
		.name  = "lcd1",
	},
	{
		.start = (int __force)SUNXI_DEU0_VBASE,
		.end   = (int __force)SUNXI_DEU0_VBASE + 0x60,
		.flags = IORESOURCE_MEM,
		.name  = "deu0",
	},
	{
		.start = (int __force)SUNXI_DEU1_VBASE,
		.end   = (int __force)SUNXI_DEU1_VBASE + 0x60,
		.flags = IORESOURCE_MEM,
		.name  = "deu1",
	},
	{
		.start = (int __force)SUNXI_DRC0_VBASE,
		.end   = (int __force)SUNXI_DRC0_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "drc0",
	},
	{
		.start = (int __force)SUNXI_DRC1_VBASE,
		.end   = (int __force)SUNXI_DRC1_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "drc1",
	},
	{
		.start = (int __force)SUNXI_DE_BE0_VBASE,
		.end   = (int __force)SUNXI_DE_BE0_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "cmu0",
	},
	{
		.start = (int __force)SUNXI_DE_BE1_VBASE,
		.end   = (int __force)SUNXI_DE_BE1_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "cmu1",
	},
	{
		.start = (int __force)SUNXI_MIPI_DSI0_VBASE,
		.end   = (int __force)SUNXI_MIPI_DSI0_VBASE + 0x2fc,
		.flags = IORESOURCE_MEM,
		.name  = "dsi0",
	},
	{
		.start = (int __force)SUNXI_MIPI_DSI0_PHY_VBASE,
		.end   = (int __force)SUNXI_MIPI_DSI0_PHY_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "dsi0_dphy",
	},
	{
		.start = (int __force)SUNXI_HDMI_VBASE,
		.end   = (int __force)SUNXI_HDMI_VBASE + 0x58c,
		.flags = IORESOURCE_MEM,
		.name  = "hdmi",
	},
	{
		.start = (int __force)SUNXI_CCM_VBASE,
		.end   = (int __force)SUNXI_CCM_VBASE + 0x2dc,
		.flags = IORESOURCE_MEM,
		.name  = "ccmu",
	},
	{
		.start = (int __force)SUNXI_PIO_VBASE,
		.end   = (int __force)SUNXI_PIO_VBASE + 0x27c,
		.flags = IORESOURCE_MEM,
		.name  = "pioc",
	},
	{
		.start = (int __force)SUNXI_PWM_VBASE,
		.end   = (int __force)SUNXI_PWM_VBASE + 0x3c,
		.flags = IORESOURCE_MEM,
		.name  = "pwm",
	},
	{
		.start = (int __force)SUNXI_IRQ_DEBE0,
		.flags = IORESOURCE_IRQ,
		.name  = "be0",
	},
	{
		.start = (int __force)SUNXI_IRQ_DEBE1,
		.flags = IORESOURCE_IRQ,
		.name  = "be1",
	},
	{
		.start = (int __force)SUNXI_IRQ_DEFE0,
		.flags = IORESOURCE_IRQ,
		.name  = "fe0",
	},
	{
		.start = (int __force)SUNXI_IRQ_DEFE1,
		.flags = IORESOURCE_IRQ,
		.name  = "fe1",
	},
	{
		.start = (int __force)SUNXI_IRQ_DRC01,
		.flags = IORESOURCE_IRQ,
		.name  = "drc0",
	},

	{
		.start = (int __force)SUNXI_IRQ_DRC01,
		.flags = IORESOURCE_IRQ,
		.name  = "drc1",
	},
	{
		.start = (int __force)SUNXI_IRQ_LCD0,
		.flags = IORESOURCE_IRQ,
		.name  = "lcd0",
	},
	{
		.start = (int __force)SUNXI_IRQ_LCD1,
		.flags = IORESOURCE_IRQ,
		.name  = "lcd1",
	},
	{
		.start = (int __force)SUNXI_IRQ_MIPIDSI,
		.flags = IORESOURCE_IRQ,
		.name  = "dsi0",
	},
};
#elif defined CONFIG_ARCH_SUN8IW3P1
static struct resource disp_resource[] =
{
	{
		.start = (int __force)SUNXI_DE_FE0_VBASE,
		.end   = (int __force)SUNXI_DE_FE0_VBASE + 0x22c,
		.flags = IORESOURCE_MEM,
		.name  = "fe0",
	},
	{
		.start = (int __force)SUNXI_DE_BE0_VBASE,
		.end   = (int __force)SUNXI_DE_BE0_VBASE + 0x9fc,
		.flags = IORESOURCE_MEM,
		.name  = "be0",
	},
	{
		.start = (int __force)SUNXI_LCD0_VBASE,
		.end   = (int __force)SUNXI_LCD0_VBASE + 0x3fc,
		.flags = IORESOURCE_MEM,
		.name  = "lcd0",
	},
	{
		.start = (int __force)SUNXI_DRC0_VBASE,
		.end   = (int __force)SUNXI_DRC0_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "drc0",
	},
	{
		.start = (int __force)SUNXI_DE_BE0_VBASE,
		.end   = (int __force)SUNXI_DE_BE0_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "cmu0",
	},
	{
		.start = (int __force)SUNXI_MIPI_DSI0_VBASE,
		.end   = (int __force)SUNXI_MIPI_DSI0_VBASE + 0x2fc,
		.flags = IORESOURCE_MEM,
		.name  = "dsi0",
	},
	{
		.start = (int __force)SUNXI_MIPI_DSI0_PHY_VBASE,
		.end   = (int __force)SUNXI_MIPI_DSI0_PHY_VBASE + 0xfc,
		.flags = IORESOURCE_MEM,
		.name  = "dsi0_dphy",
	},
	{
		.start = (int __force)SUNXI_CCM_VBASE,
		.end   = (int __force)SUNXI_CCM_VBASE + 0x2dc,
		.flags = IORESOURCE_MEM,
		.name  = "ccmu",
	},
	{
		.start = (int __force)SUNXI_PIO_VBASE,
		.end   = (int __force)SUNXI_PIO_VBASE + 0x27c,
		.flags = IORESOURCE_MEM,
		.name  = "pioc",
	},
	{
		.start = (int __force)SUNXI_PWM_VBASE,
		.end   = (int __force)SUNXI_PWM_VBASE + 0x3c,
		.flags = IORESOURCE_MEM,
		.name  = "pwm",
	},
	{
		.start = (int __force)SUNXI_IRQ_LCD0,
		.flags = IORESOURCE_IRQ,
		.name  = "lcd0",
	},
	{
		.start = (int __force)SUNXI_IRQ_MIPIDSI,
		.flags = IORESOURCE_IRQ,
		.name  = "dsi0",
	},
	{
		.start = (int __force)SUNXI_IRQ_DEBE0,
		.flags = IORESOURCE_IRQ,
		.name  = "be0",
	},
	{
		.start = (int __force)SUNXI_IRQ_DEFE0,
		.flags = IORESOURCE_IRQ,
		.name  = "fe0",
	},
	{
		.start = (int __force)SUNXI_IRQ_DRC,
		.flags = IORESOURCE_IRQ,
		.name  = "drc0",
	},
};

#elif defined(CONFIG_ARCH_SUN9IW1P1)
static struct resource disp_resource[] =
{
};
#else
static struct resource disp_resource[] =
{
};
#endif

#if 0
static __s32 disp_dram_ctrl_init(void)
{
	(*((volatile __u32 *)(0xf1c6206c))=(0x00000003));
	(*((volatile __u32 *)(0xf1c62014))=(0x00400302));
	(*((volatile __u32 *)(0xf1c6201c))=(0x00400302));

	(*((volatile __u32 *)(0xf1c62010))=(0x00800302));
	(*((volatile __u32 *)(0xf1c62014))=(0x00400307));
	(*((volatile __u32 *)(0xf1c62018))=(0x00800302));
	(*((volatile __u32 *)(0xf1c6201c))=(0x00400307));
	(*((volatile __u32 *)(0xf1c62074))=(0x00010310));
	(*((volatile __u32 *)(0xf1c62078))=(0x00010310));
	(*((volatile __u32 *)(0xf1c62080))=(0x00000310));

	return 0;
}
#endif

#ifdef FB_RESERVED_MEM
void *disp_malloc(__u32 num_bytes, __u32 *phys_addr)
{
	__u32 actual_bytes;
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
	} else {
		__wrn("disp_malloc size is zero\n");
	}

#if 0
#if 0
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
		//address = (void*)ioremap(*phys_addr, actual_bytes);
		address = sunxi_map_kernel(*phys_addr, actual_bytes);
		__inf("sunxi_mem_alloc ok, address=0x%x, size=0x%x\n", *phys_addr, num_bytes);
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

void  disp_free(void *virt_addr, void* phys_addr, __u32 num_bytes)
{
	__u32 actual_bytes;

	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if(virt_addr)
		sunxi_buf_free(virt_addr, (unsigned int)phys_addr, actual_bytes);

#if 0    
#if 0
	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if(phys_addr && virt_addr) {
		dma_free_coherent(NULL, actual_bytes, virt_addr, (dma_addr_t)phys_addr);
	}
#else
	actual_bytes = MY_BYTE_ALIGN(num_bytes);
	if(virt_addr) {
		//iounmap((void*)virt_addr);
		sunxi_unmap_kernel(virt_addr);
	}
	if(phys_addr ) {
		sunxi_mem_free((unsigned int)phys_addr, actual_bytes);
	}
#endif
#endif

	return ;
}
#endif

__s32 DRV_lcd_open(__u32 sel)
{
	__u32 i = 0;
	__lcd_flow_t *flow;

	if(bsp_disp_lcd_used(sel) && (g_disp_drv.b_lcd_open[sel] == 0))	{
		bsp_disp_lcd_open_before(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num; i++)	{
			//__u32 timeout = flow->func[i].delay*HZ/1000;

			flow->func[i].func(sel);

			//set_current_state(TASK_INTERRUPTIBLE);
			//schedule_timeout(timeout);
			msleep_interruptible(flow->func[i].delay);
		}
		bsp_disp_lcd_open_after(sel);

		g_disp_drv.b_lcd_open[sel] = 1;
	}

	return 0;
}

__s32 DRV_lcd_close(__u32 sel)
{
	__u32 i = 0;
	__lcd_flow_t *flow;

	if(bsp_disp_lcd_used(sel) && (g_disp_drv.b_lcd_open[sel] == 1))	{
		bsp_disp_lcd_close_befor(sel);

		flow = bsp_disp_lcd_get_close_flow(sel);
		for(i=0; i<flow->func_num; i++)	{
			//__u32 timeout = flow->func[i].delay*HZ/1000;

			flow->func[i].func(sel);

			//set_current_state(TASK_INTERRUPTIBLE);
			//schedule_timeout(timeout);
			msleep_interruptible(flow->func[i].delay);
		}
		bsp_disp_lcd_close_after(sel);

		g_disp_drv.b_lcd_open[sel] = 0;
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
//run the last step of lcd open flow(backlight)
static __s32 disp_lcd_open_late(__u32 sel)
{
	__lcd_flow_t *flow;

	flow = bsp_disp_lcd_get_open_flow(sel);
	flow->func[flow->func_num-1].func(sel);
	flow->cur_step = 0;

	bsp_disp_lcd_open_after(sel);

	g_disp_drv.b_lcd_open[sel] = 1;

	return 0;
}
#endif

__s32 disp_set_hdmi_func(__disp_hdmi_func * func)
{
	bsp_disp_set_hdmi_func(func);

	return 0;
}

__s32 disp_set_hdmi_hpd(__u32 hpd)
{
	bsp_disp_set_hdmi_hpd(hpd);

	return 0;
}

static void resume_work_0(struct work_struct *work)
{
	__u32 i = 0;
	__lcd_flow_t *flow;
	__u32 sel = 0;

	if(bsp_disp_lcd_used(sel) && (g_disp_drv.b_lcd_open[sel] == 0))	{
		bsp_disp_lcd_open_before(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num-1; i++)	{
			__u32 timeout = flow->func[i].delay*HZ/1000;

			flow->func[i].func(sel);

			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(timeout);
		}
	}

	g_disp_drv.b_lcd_open[sel] = 1;
}

static void resume_work_1(struct work_struct *work)
{
	__lcd_flow_t *flow;
	__u32 sel = 1;
	__u32 i;

	if(bsp_disp_lcd_used(sel) && (g_disp_drv.b_lcd_open[sel] == 0)) {
		bsp_disp_lcd_open_before(sel);

		flow = bsp_disp_lcd_get_open_flow(sel);
		for(i=0; i<flow->func_num-1; i++)	{
			__u32 timeout = flow->func[i].delay*HZ/1000;

			flow->func[i].func(sel);

			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(timeout);
		}
	}

	g_disp_drv.b_lcd_open[sel] = 1;
}

static void start_work(struct work_struct *work)
{
	int num_screens;
	int screen_id;

	num_screens = bsp_disp_feat_get_num_screens();

	for(screen_id = 0; screen_id<num_screens; screen_id++) {
		if((g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_LCD)) {
			if(bsp_disp_lcd_get_registered() && bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_LCD) {
				DRV_lcd_open(screen_id);
			}
		}	else if(g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_TV) {
			bsp_disp_tv_set_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
			bsp_disp_tv_open(screen_id);
		}	else if(g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_HDMI) {
			if(bsp_disp_hdmi_get_registered() && bsp_disp_get_output_type(screen_id) != DISP_OUTPUT_TYPE_HDMI) {
				bsp_disp_hdmi_set_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
				bsp_disp_hdmi_open(screen_id);
			}
		}	else if(g_fbi.disp_init.output_type[screen_id] == DISP_OUTPUT_TYPE_VGA)	{
			bsp_disp_vga_set_mode(screen_id, g_fbi.disp_init.output_mode[screen_id]);
			bsp_disp_vga_open(screen_id);
		}
	}
}

static __s32 start_process(void)
{
	flush_work(&g_fbi.start_work);
	schedule_work(&g_fbi.start_work);

	return 0;
}

extern __s32 capture_event(__u32 sel);
__s32 DRV_DISP_Init(void)
{
	__disp_bsp_init_para para;
	int i;

	__inf("DRV_DISP_Init !\n");

	init_waitqueue_head(&g_fbi.wait[0]);
	init_waitqueue_head(&g_fbi.wait[1]);
	g_fbi.wait_count[0] = 0;
	g_fbi.wait_count[1] = 0;
	INIT_WORK(&g_fbi.resume_work[0], resume_work_0);
	INIT_WORK(&g_fbi.resume_work[1], resume_work_1);
	INIT_WORK(&g_fbi.start_work, start_work);

	memset(&para, 0, sizeof(__disp_bsp_init_para));

	for(i=0; i<sizeof(disp_mod)/sizeof(struct sunxi_disp_mod); i++)	{
		para.reg_base[i] = (__u32)g_fbi.reg_base[i];
		para.reg_size[i] = (__u32)g_fbi.reg_size[i];
		para.irq[i]      = g_fbi.irq[i];
	}

	para.disp_int_process       = DRV_disp_int_process;
	para.vsync_event            = DRV_disp_vsync_event;
	para.take_effect            = DRV_disp_take_effect_event;
	para.start_process          = start_process;
	para.capture_event          = capture_event;

	memset(&g_disp_drv, 0, sizeof(__disp_drv_t));

	bsp_disp_init(&para);
	bsp_disp_open();
	start_process();

	__inf("DRV_DISP_Init end\n");
	return 0;
}

__s32 DRV_DISP_Exit(void)
{
	Fb_Exit();
	bsp_disp_close();
	bsp_disp_exit(g_disp_drv.exit_mode);

	return 0;
}


static int disp_mem_request(int sel,__u32 size)
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
	__u32 ret = 0;
	__u32 phy_addr;

	ret = (__u32)disp_malloc(size, &phy_addr);
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

int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops)
{
	src_ops->sunxi_lcd_delay_ms = bsp_disp_lcd_delay_ms;
	src_ops->sunxi_lcd_delay_us = bsp_disp_lcd_delay_us;
	src_ops->sunxi_lcd_tcon_enable = bsp_disp_lcd_tcon_open;
	src_ops->sunxi_lcd_tcon_disable = bsp_disp_lcd_tcon_close;
	src_ops->sunxi_lcd_pwm_enable = bsp_disp_lcd_pwm_enable;
	src_ops->sunxi_lcd_pwm_disable = bsp_disp_lcd_pwm_disable;
	src_ops->sunxi_lcd_backlight_enable = bsp_disp_lcd_backlight_enable;
	src_ops->sunxi_lcd_backlight_disable = bsp_disp_lcd_backlight_disable;
	src_ops->sunxi_lcd_power_enable = bsp_disp_lcd_power_enable;
	src_ops->sunxi_lcd_power_disable = bsp_disp_lcd_power_disable;
	src_ops->sunxi_lcd_pin_cfg = bsp_disp_lcd_pin_cfg;
	src_ops->sunxi_lcd_dsi_clk_enable = dsi_clk_enable;
	src_ops->sunxi_lcd_get_driver_name = bsp_disp_lcd_get_driver_name;
	src_ops->sunxi_lcd_set_panel_funs = bsp_disp_lcd_set_panel_funs;

	return 0;
}
int disp_mmap(struct file *file, struct vm_area_struct * vma)
{
	unsigned long  physics =  g_disp_mm[g_disp_mm_sel].mem_start;// - PAGE_OFFSET;
	unsigned long mypfn = physics >> PAGE_SHIFT;
	unsigned long vmsize = vma->vm_end-vma->vm_start;

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

static int __devinit disp_probe(struct platform_device *pdev)//called when platform_driver_register
{
	fb_info_t * info = NULL;
	int i;
	struct resource	*res;

	pr_info("[DISP]disp_probe\n");

	info = &g_fbi;
	init_timer(&info->disp_timer[0]);
	init_timer(&info->disp_timer[1]);
	info->dev = &pdev->dev;
	platform_set_drvdata(pdev,info);

	for(i=0; i<sizeof(disp_mod)/sizeof(struct sunxi_disp_mod); i++)	{
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, disp_mod[i].name);
		if(res != NULL) {
			info->reg_base[i] = res->start;
			info->reg_size[i] = res->end - res->start;
		}
	}

	for(i=0; i<sizeof(disp_mod)/sizeof(struct sunxi_disp_mod); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, disp_mod[i].name);
		if(res != NULL)	{
			info->irq[i] = res->start;
		}
	}
	DRV_DISP_Init();
	Fb_Init();

	pr_info("[DISP]disp_probe finish\n");

	return 0;
}

static int disp_remove(struct platform_device *pdev)
{
	pr_info("disp_remove call\n");

	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void backlight_early_suspend(struct early_suspend *h)
{
	int i = 0;
	int r_count = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	mutex_lock(&g_fbi.runtime_lock);
	g_fbi.b_no_output = 1;
	r_count = g_fbi.cb_r_conut;
	while(r_count != g_fbi.cb_w_conut) {
		if(r_count >= 9) {
			r_count = 0;
		}	else {
			r_count++;
		}

		if(g_fbi.cb_arg[r_count] != 0) {
			g_fbi.cb_fn(g_fbi.cb_arg[r_count], 1);
			g_fbi.cb_arg[r_count] = 0;
			g_fbi.cb_r_conut = r_count;
			//printk(KERN_WARNING "##es r_count:%d\n", r_count);
		}
	}
	mutex_unlock(&g_fbi.runtime_lock);

	for(i=num_screens-1; i>=0; i--)	{
		suspend_output_type[i] = bsp_disp_get_output_type(i);
		if(suspend_output_type[i] == DISP_OUTPUT_TYPE_LCD) {
			DRV_lcd_close(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_TV) {
			bsp_disp_tv_close(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_VGA)	{
			bsp_disp_vga_close(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_HDMI) {
			bsp_disp_hdmi_close(i);
		}
	}
	bsp_disp_hdmi_early_suspend();
	//bsp_disp_clk_off(2);

	suspend_status |= 1;
	suspend_prestep = 0;

	pr_info("[DISP]display early suspend done\n");
}

void backlight_late_resume(struct early_suspend *h)
{
	int i = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	if(suspend_prestep != 2) {
		//bsp_disp_clk_on(2);
	}
	bsp_disp_hdmi_late_resume();
	for(i=0; i<num_screens; i++) {
		if(suspend_output_type[i] == DISP_OUTPUT_TYPE_LCD) {
			/* wake up from deep sleep */
			if(2 == suspend_prestep) {
				flush_work(&g_fbi.resume_work[i]);
				disp_lcd_open_late(i);
			}
			/* wake up from light sleep */
			else if(0 == suspend_prestep)
			{
			DRV_lcd_open(i);
			}
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_TV) {
			bsp_disp_tv_open(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_VGA)	{
			bsp_disp_vga_open(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_HDMI) {
			bsp_disp_hdmi_set_mode(i,bsp_disp_hdmi_get_mode(i));
			bsp_disp_hdmi_open(i);
		}
	}

	g_fbi.b_no_output = 0;
	suspend_status &= (~1);
	suspend_prestep = 3;

	pr_info("[DISP]display late resume done\n");
}

static struct early_suspend backlight_early_suspend_handler = {
	.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB,
	.suspend = backlight_early_suspend,
	.resume = backlight_late_resume,
};

#endif


static __u32 image_reg_bak[3];
static __u32 scaler_reg_bak[3];
extern __s32 disp_mipipll_enable(__u32 en);
int disp_suspend(struct platform_device *pdev, pm_message_t state)
{
	int i = 0;
	int r_count = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();
#ifndef CONFIG_HAS_EARLYSUSPEND

	mutex_lock(&g_fbi.runtime_lock);
	g_fbi.b_no_output = 1;
	r_count = g_fbi.cb_r_conut;
	while(r_count != g_fbi.cb_w_conut) {
		if(r_count >= 9) {
			r_count = 0;
		}	else {
			r_count++;
		}

		if(g_fbi.cb_arg[r_count] != NULL) {
			g_fbi.cb_fn(g_fbi.cb_arg[r_count], 1);
			g_fbi.cb_arg[r_count] = NULL;
			g_fbi.cb_r_conut = r_count;
			//printk(KERN_WARNING "##es r_count:%d\n", r_count);
		}
	}
	mutex_unlock(&g_fbi.runtime_lock);

	pr_info("[DISP]disp_suspend call\n");

	for(i=num_screens-1; i>=0; i--)	{
		suspend_output_type[i] = bsp_disp_get_output_type(i);
		if(suspend_output_type[i] == DISP_OUTPUT_TYPE_LCD) {
			DRV_lcd_close(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_TV) {
			bsp_disp_tv_close(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_VGA)	{
			bsp_disp_vga_close(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_HDMI) {
			bsp_disp_hdmi_close(i);
		}
	}
	bsp_disp_hdmi_early_suspend();
#else
	/* when has early suspend */

	/* sleep from incomplete wakeup */
	if(2 == suspend_prestep) {
		for(i=num_screens-1; i>=0; i--)	{
			if(suspend_output_type[i] == DISP_OUTPUT_TYPE_LCD) {
				flush_work(&g_fbi.resume_work[i]);
				DRV_lcd_close(i);
			}
		}
	}
#endif

	for(i=0; i<num_screens; i++) {
		bsp_disp_iep_suspend(i);
	}
	bsp_disp_hdmi_suspend();
//#ifdef SUPER_STANDBY
	//if(SUPER_STANDBY == standby_type)
	{
		//pr_info("[DISP]disp super standby enter\n");
		for(i=0; i<num_screens; i++) {
			image_reg_bak[i] = (__u32)kmalloc(0xe00 - 0x800, GFP_KERNEL | __GFP_ZERO);
			scaler_reg_bak[i] = (__u32)kmalloc(0xa18, GFP_KERNEL | __GFP_ZERO);
			bsp_disp_store_image_reg(i, image_reg_bak[i]);
			bsp_disp_store_scaler_reg(i, scaler_reg_bak[i]);
		}
	}
//#endif

	bsp_disp_clk_off(1);
	bsp_disp_clk_off(2);
	disp_mipipll_enable(0);

	suspend_status |= 2;
	suspend_prestep = 1;

	return 0;
}


int disp_resume(struct platform_device *pdev)
{
	int i = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	disp_mipipll_enable(1);
	bsp_disp_clk_on(1);
	bsp_disp_clk_on(2);

//#ifdef SUPER_STANDBY
	//if(SUPER_STANDBY == standby_type)
	{
		//pr_info("[DISP]disp super standby exit\n");
		for(i=0; i<num_screens; i++) {
			bsp_disp_restore_scaler_reg(i, scaler_reg_bak[i]);
			bsp_disp_restore_image_reg(i, image_reg_bak[i]);
			bsp_disp_restore_lcdc_reg(i);
			kfree((void*)scaler_reg_bak[i]);
			kfree((void*)image_reg_bak[i]);
		}
	}
//#endif
	for(i=0; i<num_screens; i++) {
		bsp_disp_image_resume(i);
		bsp_disp_iep_resume(i);
	}
	bsp_disp_hdmi_resume();
#ifndef CONFIG_HAS_EARLYSUSPEND
	pr_info("[DISP]disp_resume\n");
	bsp_disp_hdmi_late_resume();

	for(i=num_screens-1; i>=0; i--)	{
		if(suspend_output_type[i] == DISP_OUTPUT_TYPE_LCD) {
			DRV_lcd_open(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_TV) {
			bsp_disp_tv_open(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_VGA)	{
			bsp_disp_vga_open(i);
		}	else if(suspend_output_type[i] == DISP_OUTPUT_TYPE_HDMI) {
			bsp_disp_hdmi_set_mode(i,bsp_disp_hdmi_get_mode(i));
			bsp_disp_hdmi_open(i);
		}
	}
#else
	pr_info("[DISP]disp_resume call\n");
	for(i=num_screens-1; i>=0; i--) {
		if(suspend_output_type[i] == DISP_OUTPUT_TYPE_LCD) {
			schedule_work(&g_fbi.resume_work[i]);
		}
	}
#endif
	g_fbi.b_no_output = 0;
	suspend_status &= (~2);
	suspend_prestep = 2;
	return 0;
}

static void disp_shutdown(struct platform_device *pdev)
{
	__u32 type = 0, i = 0;
	int num_screens;

	num_screens = bsp_disp_feat_get_num_screens();

	for(i=0; i<num_screens; i++) {
		type = bsp_disp_get_output_type(i);
		if(type == DISP_OUTPUT_TYPE_LCD) {
			DRV_lcd_close(i);
		}
	}
}

long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long karg[4];
	unsigned long ubuffer[4] = {0};
	__s32 ret = 0;
	int num_screens;

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
	if(suspend_status & 2) {
		__wrn("ioctl:%x fail when in suspend!\n", cmd);
		return -1;
	}

	if(disp_print_cmd_level == 1)	{
		if(cmd!=DISP_CMD_TV_GET_INTERFACE && cmd!=DISP_CMD_HDMI_GET_HPD_STATUS && cmd!=DISP_CMD_GET_OUTPUT_TYPE
		    && cmd!=DISP_CMD_SCN_GET_WIDTH && cmd!=DISP_CMD_SCN_GET_HEIGHT
		    && cmd!=DISP_CMD_VIDEO_SET_FB && cmd!=DISP_CMD_VIDEO_GET_FRAME_ID
		    && cmd!=DISP_CMD_VSYNC_EVENT_EN) {
			if(cmd != disp_cmd_print)	{
				OSAL_PRINTF("cmd:0x%x,%ld,%ld\n",cmd, ubuffer[0], ubuffer[1]);
			}
		}
	}
	if(cmd == disp_cmd_print) {
		OSAL_PRINTF("cmd:0x%x,%ld,%ld\n",cmd, ubuffer[0], ubuffer[1]);
	}

	switch(cmd)	{
	//----disp global----
	case DISP_CMD_SET_BKCOLOR:
	{
		__disp_color_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_color_t)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_set_bk_color(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_SET_COLORKEY:
	{
		__disp_colorkey_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_colorkey_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_set_color_key(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_SET_PALETTE_TBL:
		if((ubuffer[1] == 0) || ((int)ubuffer[3] <= 0))	{
			__wrn("para invalid in disp ioctrl DISP_CMD_SET_PALETTE_TBL,buffer:0x%x, size:0x%x\n", (unsigned int)ubuffer[1], (unsigned int)ubuffer[3]);
			return -1;
		}
		if(copy_from_user(gbuffer, (void __user *)ubuffer[1],ubuffer[3]))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_set_palette_table(ubuffer[0], (__u32 *)gbuffer, ubuffer[2], ubuffer[3]);
		break;

	case DISP_CMD_GET_PALETTE_TBL:
		if((ubuffer[1] == 0) || ((int)ubuffer[3] <= 0))	{
			__wrn("para invalid in disp ioctrl DISP_CMD_GET_PALETTE_TBL,buffer:0x%x, size:0x%x\n", (unsigned int)ubuffer[1], (unsigned int)ubuffer[3]);
			return -1;
		}
		ret = bsp_disp_get_palette_table(ubuffer[0], (__u32 *)gbuffer, ubuffer[2], ubuffer[3]);
		if(copy_to_user((void __user *)ubuffer[1], gbuffer,ubuffer[3]))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;

	case DISP_CMD_START_CMD_CACHE:
		ret = bsp_disp_cmd_cache(ubuffer[0]);
		break;

	case DISP_CMD_EXECUTE_CMD_AND_STOP_CACHE:
		ret = bsp_disp_cmd_submit(ubuffer[0]);
		break;

	case DISP_CMD_GET_OUTPUT_TYPE:
		if(suspend_status != 0)	{
			ret = suspend_output_type[ubuffer[0]];
		}	else {
			ret =  bsp_disp_get_output_type(ubuffer[0]);
		}

		break;

	case DISP_CMD_SCN_GET_WIDTH:
		ret = bsp_disp_get_screen_width(ubuffer[0]);
		break;

	case DISP_CMD_SCN_GET_HEIGHT:
		ret = bsp_disp_get_screen_height(ubuffer[0]);
		break;

	case DISP_CMD_SET_GAMMA_TABLE:
		if((ubuffer[1] == 0) || ((int)ubuffer[2] <= 0))
		{
		__wrn("para invalid in disp ioctrl DISP_CMD_SET_GAMMA_TABLE,buffer:0x%x, size:0x%x\n", (unsigned int)ubuffer[1], (unsigned int)ubuffer[2]);
		return -1;
		}
		if(copy_from_user(gbuffer, (void __user *)ubuffer[1],ubuffer[2]))
		{
		__wrn("copy_from_user fail\n");
		return  -EFAULT;
		}
		ret = bsp_disp_set_gamma_table(ubuffer[0], (__u32 *)gbuffer, ubuffer[2]);
		break;

	case DISP_CMD_GAMMA_CORRECTION_ON:
		ret = bsp_disp_gamma_correction_enable(ubuffer[0]);
		break;

	case DISP_CMD_GAMMA_CORRECTION_OFF:
		ret = bsp_disp_gamma_correction_disable(ubuffer[0]);
		break;

	case DISP_CMD_SET_BRIGHT:
		ret = bsp_disp_cmu_set_bright(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_BRIGHT:
		ret = bsp_disp_cmu_get_bright(ubuffer[0]);
		break;

	case DISP_CMD_SET_CONTRAST:
		ret = bsp_disp_cmu_set_contrast(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_CONTRAST:
		ret = bsp_disp_cmu_get_contrast(ubuffer[0]);
		break;

	case DISP_CMD_SET_SATURATION:
		ret = bsp_disp_cmu_set_saturation(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_SATURATION:
		ret = bsp_disp_cmu_get_saturation(ubuffer[0]);
		break;

	case DISP_CMD_SET_HUE:
		ret = bsp_disp_cmu_set_hue(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_HUE:
		ret = bsp_disp_cmu_get_hue(ubuffer[0]);
		break;

	case DISP_CMD_ENHANCE_ON:
		ret = bsp_disp_cmu_enable(ubuffer[0], 1);
		break;

	case DISP_CMD_ENHANCE_OFF:
		ret = bsp_disp_cmu_enable(ubuffer[0], 0);
		break;

	case DISP_CMD_GET_ENHANCE_EN:
		ret = bsp_disp_cmu_get_enable(ubuffer[0]);
		break;

	case DISP_CMD_SET_ENHANCE_MODE:
		ret = bsp_disp_cmu_set_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_GET_ENHANCE_MODE:
		ret = bsp_disp_cmu_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_SET_ENHANCE_WINDOW:
	{
		__disp_rect_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_rect_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_cmu_set_window(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_GET_ENHANCE_WINDOW:
	{
		__disp_rect_t para;

		ret = bsp_disp_cmu_get_window(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(__disp_layer_info_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_CAPTURE_SCREEN:
		ret = bsp_disp_capture_screen(ubuffer[0], (__disp_capture_screen_para_t *)ubuffer[1]);
		break;

	case DISP_CMD_CAPTURE_SCREEN_STOP:
		ret = bsp_disp_capture_screen_stop(ubuffer[0]);
		break;

	case DISP_CMD_SET_SCREEN_SIZE:
		ret = bsp_disp_set_screen_size(ubuffer[0], (__disp_rectsz_t*)ubuffer[1]);
		break;

	case DISP_CMD_DE_FLICKER_ON:
		ret = bsp_disp_de_flicker_enable(ubuffer[0], 1);
		break;

	case DISP_CMD_DE_FLICKER_OFF:
		ret = bsp_disp_de_flicker_enable(ubuffer[0], 0);
		break;

	case DISP_CMD_DRC_ON:
		ret = bsp_disp_drc_enable(ubuffer[0], 1);
		break;

	case DISP_CMD_DRC_OFF:
		ret = bsp_disp_drc_enable(ubuffer[0], 0);
		break;

	case DISP_CMD_GET_DRC_EN:
		ret = bsp_disp_drc_get_enable(ubuffer[0]);
		break;

	case DISP_CMD_DRC_SET_WINDOW:
	{
		__disp_rect_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_rect_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}

		ret = bsp_disp_drc_set_window(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_DRC_GET_WINDOW:
	{
		__disp_rect_t para;

		ret = bsp_disp_drc_get_window(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1], &para,sizeof(__disp_rect_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}

		return ret;
		break;
	}

	case DISP_CMD_VSYNC_EVENT_EN:
		ret = bsp_disp_vsync_event_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_SET_OVL_MODE:
		break;

	case DISP_CMD_BLANK:
		ret = dispc_blank(ubuffer[0], ubuffer[1]);
		break;

	//----layer----
	case DISP_CMD_LAYER_REQUEST:
		ret = bsp_disp_layer_request(ubuffer[0], (__disp_layer_work_mode_t)ubuffer[1]);
		break;

	case DISP_CMD_LAYER_RELEASE:
		ret = bsp_disp_layer_release(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_OPEN:
		ret = bsp_disp_layer_open(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_CLOSE:
		ret = bsp_disp_layer_close(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_FB:
	{
		__disp_fb_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_fb_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_layer_set_framebuffer(ubuffer[0], ubuffer[1], &para);
		//DRV_disp_wait_cmd_finish(ubuffer[0]);
		break;
	}

	case DISP_CMD_LAYER_GET_FB:
	{
		__disp_fb_t para;

		ret = bsp_disp_layer_get_framebuffer(ubuffer[0], ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2], &para,sizeof(__disp_fb_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_LAYER_SET_SRC_WINDOW:
	{
		__disp_rect_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_rect_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_layer_set_src_window(ubuffer[0],ubuffer[1], &para);
		//DRV_disp_wait_cmd_finish(ubuffer[0]);
		break;
	}

	case DISP_CMD_LAYER_GET_SRC_WINDOW:
	{
		__disp_rect_t para;

		ret = bsp_disp_layer_get_src_window(ubuffer[0],ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2], &para, sizeof(__disp_rect_t)))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_LAYER_SET_SCN_WINDOW:
	{
		__disp_rect_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_rect_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_layer_set_screen_window(ubuffer[0],ubuffer[1], &para);
		//DRV_disp_wait_cmd_finish(ubuffer[0]);
		break;
	}

	case DISP_CMD_LAYER_GET_SCN_WINDOW:
	{
		__disp_rect_t para;

		ret = bsp_disp_layer_get_screen_window(ubuffer[0],ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2], &para, sizeof(__disp_rect_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_LAYER_SET_PARA:
	{
		__disp_layer_info_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_layer_info_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_layer_set_para(ubuffer[0], ubuffer[1], &para);
		//DRV_disp_wait_cmd_finish(ubuffer[0]);
		break;
	}

	case DISP_CMD_LAYER_GET_PARA:
	{
		__disp_layer_info_t para;

		ret = bsp_disp_layer_get_para(ubuffer[0], ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2],&para, sizeof(__disp_layer_info_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_LAYER_TOP:
		ret = bsp_disp_layer_set_top(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_BOTTOM:
		ret = bsp_disp_layer_set_bottom(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_ALPHA_ON:
		ret = bsp_disp_layer_alpha_enable(ubuffer[0], ubuffer[1], 1);
		break;

	case DISP_CMD_LAYER_ALPHA_OFF:
		ret = bsp_disp_layer_alpha_enable(ubuffer[0], ubuffer[1], 0);
		break;

	case DISP_CMD_LAYER_SET_ALPHA_VALUE:
		ret = bsp_disp_layer_set_alpha_value(ubuffer[0], ubuffer[1], ubuffer[2]);
		//DRV_disp_wait_cmd_finish(ubuffer[0]);
		break;

	case DISP_CMD_LAYER_CK_ON:
		ret = bsp_disp_layer_colorkey_enable(ubuffer[0], ubuffer[1], 1);
		break;

	case DISP_CMD_LAYER_CK_OFF:
		ret = bsp_disp_layer_colorkey_enable(ubuffer[0], ubuffer[1], 0);
		break;

	case DISP_CMD_LAYER_SET_PIPE:
		ret = bsp_disp_layer_set_pipe(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_ALPHA_VALUE:
		ret = bsp_disp_layer_get_alpha_value(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_GET_ALPHA_EN:
		ret = bsp_disp_layer_get_alpha_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_GET_CK_EN:
		ret = bsp_disp_layer_get_colorkey_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_GET_PRIO:
		ret = bsp_disp_layer_get_piro(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_GET_PIPE:
		ret = bsp_disp_layer_get_pipe(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_SMOOTH:
		ret = bsp_disp_layer_set_smooth(ubuffer[0], ubuffer[1],(__disp_video_smooth_t) ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_SMOOTH:
		ret = bsp_disp_layer_get_smooth(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_BRIGHT:
		ret = bsp_disp_cmu_layer_set_bright(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_BRIGHT:
		ret = bsp_disp_cmu_layer_get_bright(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_CONTRAST:
		ret = bsp_disp_cmu_layer_set_contrast(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_CONTRAST:
		ret = bsp_disp_cmu_layer_get_contrast(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_SATURATION:
		ret = bsp_disp_cmu_layer_set_saturation(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_SATURATION:
		ret = bsp_disp_cmu_layer_get_saturation(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_HUE:
		ret = bsp_disp_cmu_layer_set_hue(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_HUE:
		ret = bsp_disp_cmu_layer_get_hue(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_ENHANCE_ON:
		ret = bsp_disp_cmu_layer_enable(ubuffer[0], ubuffer[1], 1);
		break;

	case DISP_CMD_LAYER_ENHANCE_OFF:
		ret = bsp_disp_cmu_layer_enable(ubuffer[0], ubuffer[1], 0);
		break;

	case DISP_CMD_LAYER_GET_ENHANCE_EN:
		ret = bsp_disp_cmu_layer_get_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_ENHANCE_MODE:
		ret = bsp_disp_cmu_layer_set_mode(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_ENHANCE_MODE:
		ret = bsp_disp_cmu_layer_get_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_ENHANCE_WINDOW:
	{
		__disp_rect_t para;
		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_rect_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_cmu_layer_set_window(ubuffer[0], ubuffer[1], &para);
		break;
	}

	case DISP_CMD_LAYER_GET_ENHANCE_WINDOW:
	{
		__disp_rect_t para;
		ret = bsp_disp_cmu_layer_get_window(ubuffer[0], ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2],&para, sizeof(__disp_layer_info_t))) {
		__wrn("copy_to_user fail\n");
		return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_LAYER_VPP_ON:
		ret = bsp_disp_deu_enable(ubuffer[0], ubuffer[1], 1);
		break;

	case DISP_CMD_LAYER_VPP_OFF:
		ret = bsp_disp_deu_enable(ubuffer[0], ubuffer[1], 0);
		break;

	case DISP_CMD_LAYER_GET_VPP_EN:
		ret = bsp_disp_deu_get_enable(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_LUMA_SHARP_LEVEL:
		ret = bsp_disp_deu_set_luma_sharp_level(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_LUMA_SHARP_LEVEL:
		ret = bsp_disp_deu_get_luma_sharp_level(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_CHROMA_SHARP_LEVEL:
		ret = bsp_disp_deu_set_chroma_sharp_level(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_CHROMA_SHARP_LEVEL:
		ret = bsp_disp_deu_get_chroma_sharp_level(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_WHITE_EXTEN_LEVEL:
		ret = bsp_disp_deu_set_white_exten_level(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_WHITE_EXTEN_LEVEL:
		ret = bsp_disp_deu_get_white_exten_level(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_SET_BLACK_EXTEN_LEVEL:
		ret = bsp_disp_deu_set_black_exten_level(ubuffer[0], ubuffer[1], ubuffer[2]);
		break;

	case DISP_CMD_LAYER_GET_BLACK_EXTEN_LEVEL:
		ret = bsp_disp_deu_get_black_exten_level(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LAYER_VPP_SET_WINDOW:
	{
		__disp_rect_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_rect_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}

		ret = bsp_disp_deu_set_window(ubuffer[0], ubuffer[1], &para);
		break;
	}

	case DISP_CMD_LAYER_VPP_GET_WINDOW:
	{
		__disp_rect_t para;

		ret = bsp_disp_deu_get_window(ubuffer[0], ubuffer[1], &para);
		if(copy_to_user((void __user *)ubuffer[2], &para,sizeof(__disp_rect_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	//----scaler----
	case DISP_CMD_SCALER_REQUEST:
		ret = bsp_disp_scaler_request();
		break;

	case DISP_CMD_SCALER_RELEASE:
		ret = bsp_disp_scaler_release(ubuffer[1]);
		break;

	case DISP_CMD_SCALER_EXECUTE:
	{
		__disp_scaler_para_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_scaler_para_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_scaler_start(ubuffer[1],&para);
		break;
	}

	case DISP_CMD_SCALER_EXECUTE_EX:
	{
		__disp_scaler_para_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_scaler_para_t)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_scaler_start_ex(ubuffer[1],&para);
		break;
	}

	//----hwc----
	case DISP_CMD_HWC_OPEN:
		ret =  bsp_disp_hwc_enable(ubuffer[0], 1);
		break;

	case DISP_CMD_HWC_CLOSE:
		ret =  bsp_disp_hwc_enable(ubuffer[0], 0);
		break;

	case DISP_CMD_HWC_SET_POS:
	{
		__disp_pos_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_pos_t)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_hwc_set_pos(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_HWC_GET_POS:
	{
		__disp_pos_t para;

		ret = bsp_disp_hwc_get_pos(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(__disp_pos_t)))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_HWC_SET_FB:
	{
		__disp_hwc_pattern_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_hwc_pattern_t)))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_hwc_set_framebuffer(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_HWC_SET_PALETTE_TABLE:
		if((ubuffer[1] == 0) || ((int)ubuffer[3] <= 0))	{
			__wrn("para invalid in display ioctrl DISP_CMD_HWC_SET_PALETTE_TABLE,buffer:0x%x, size:0x%x\n", (unsigned int)ubuffer[1], (unsigned int)ubuffer[3]);
			return -1;
		}
		if(copy_from_user(gbuffer, (void __user *)ubuffer[1],ubuffer[3]))	{
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_hwc_set_palette(ubuffer[0], (void*)gbuffer, ubuffer[2], ubuffer[3]);
		break;


	//----video----
	case DISP_CMD_VIDEO_START:
		ret = bsp_disp_video_start(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_VIDEO_STOP:
		ret = bsp_disp_video_stop(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_VIDEO_SET_FB:
	{
		__disp_video_fb_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[2],sizeof(__disp_video_fb_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = bsp_disp_video_set_fb(ubuffer[0], ubuffer[1], &para);
		break;
	}

	case DISP_CMD_VIDEO_GET_FRAME_ID:
		ret = bsp_disp_video_get_frame_id(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_VIDEO_GET_DIT_INFO:
	{
		__disp_dit_info_t para;

		ret = bsp_disp_video_get_dit_info(ubuffer[0], ubuffer[1],&para);
		if(copy_to_user((void __user *)ubuffer[2],&para, sizeof(__disp_dit_info_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	//----lcd----
	case DISP_CMD_LCD_ON:
		ret = DRV_lcd_open(ubuffer[0]);
		if(suspend_status != 0)
		{
			suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_LCD;
		}
		break;

	case DISP_CMD_LCD_OFF:
		ret = DRV_lcd_close(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_LCD_SET_BRIGHTNESS:
		ret = bsp_disp_lcd_set_bright(ubuffer[0], ubuffer[1], 0);
		break;

	case DISP_CMD_LCD_GET_BRIGHTNESS:
		ret = bsp_disp_lcd_get_bright(ubuffer[0]);
		break;

	case DISP_CMD_LCD_SET_SRC:
		ret = bsp_disp_lcd_set_src(ubuffer[0], (__disp_lcdc_src_t)ubuffer[1]);
		break;

	case DISP_CMD_LCD_USER_DEFINED_FUNC:
		ret = bsp_disp_lcd_user_defined_func(ubuffer[0], ubuffer[1], ubuffer[2], ubuffer[3]);
		break;

	case DISP_CMD_LCD_BACKLIGHT_ON:
		if(suspend_status != 0)	{
			ret = bsp_disp_open_lcd_backlight(ubuffer[0]);
		}
		break;

	case DISP_CMD_LCD_BACKLIGHT_OFF:
		if(suspend_status != 0)	{
			ret = bsp_disp_close_lcd_backlight(ubuffer[0]);
		}
		break;

	case DISP_CMD_LCD_SET_FPS:
		ret = bsp_disp_lcd_set_fps(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_LCD_GET_SIZE:
	{
		char ptr[8];
		if((ubuffer[1] == 0))	{
			__wrn("para invalid in disp ioctrl DISP_CMD_LCD_GET_SIZE, screen_id = %d, str_addr=%d\n", (unsigned int)ubuffer[0], (unsigned int)ubuffer[1]);
			return -1;
		}
		ret = bsp_disp_lcd_get_size(ubuffer[0], ptr);
		if(copy_to_user((void __user *)ubuffer[1], ptr,8))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
	}
	break;

	case DISP_CMD_LCD_GET_MODEL_NAME:
	{
		char ptr[32];
		if((ubuffer[1] == 0)) {
			__wrn("para invalid in disp ioctrl DISP_CMD_LCD_GET_MODEL_NAME,screen_id = %d, str_addr=%d\n", (unsigned int)ubuffer[0], (unsigned int)ubuffer[1]);
			return -1;
		}
		ret = bsp_disp_lcd_get_model_name(ubuffer[0], ptr);
		if(copy_to_user((void __user *)ubuffer[1], ptr,32))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
	}
	break;
#if 0
	//----pwm----
	case DISP_CMD_PWM_SET_PARA:
		ret = pwm_set_para(ubuffer[0], (__pwm_info_t *)ubuffer[1]);
		break;

	case DISP_CMD_PWM_GET_PARA:
		ret = pwm_get_para(ubuffer[0], (__pwm_info_t *)ubuffer[1]);
		break;
#endif
	//----tv----
	case DISP_CMD_TV_ON:
		ret = bsp_disp_tv_open(ubuffer[0]);
		if(suspend_status != 0)	{
			suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_TV;
		}
		break;

	case DISP_CMD_TV_OFF:
		ret = bsp_disp_tv_close(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_TV_SET_MODE:
		ret = bsp_disp_tv_set_mode(ubuffer[0], (__disp_tv_mode_t)ubuffer[1]);
		break;

	case DISP_CMD_TV_GET_MODE:
		ret = bsp_disp_tv_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_TV_AUTOCHECK_ON:
		ret = bsp_disp_tv_auto_check_enable(ubuffer[0]);
		break;

	case DISP_CMD_TV_AUTOCHECK_OFF:
		ret = bsp_disp_tv_auto_check_disable(ubuffer[0]);
		break;

	case DISP_CMD_TV_GET_INTERFACE:
		if(suspend_status != 0)	{
			ret = DISP_TV_NONE;
		}	else {
			ret = bsp_disp_tv_get_interface(ubuffer[0]);
		}
		break;

	case DISP_CMD_TV_SET_SRC:
		ret = bsp_disp_tv_set_src(ubuffer[0], (__disp_lcdc_src_t)ubuffer[1]);
		break;

	case DISP_CMD_TV_GET_DAC_STATUS:
		if(suspend_status != 0)	{
			ret = 0;
		}	else {
			ret =  bsp_disp_tv_get_dac_status(ubuffer[0], ubuffer[1]);
		}
		break;

	case DISP_CMD_TV_SET_DAC_SOURCE:
		ret =  bsp_disp_tv_set_dac_source(ubuffer[0], ubuffer[1], (__disp_tv_dac_source)ubuffer[2]);
		break;

	case DISP_CMD_TV_GET_DAC_SOURCE:
		ret =  bsp_disp_tv_get_dac_source(ubuffer[0], ubuffer[1]);
		break;

	//----hdmi----
	case DISP_CMD_HDMI_ON:
		ret = bsp_disp_hdmi_open(ubuffer[0]);
		if(suspend_status != 0)	{
			suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_HDMI;
		}
		break;

	case DISP_CMD_HDMI_OFF:
		ret = bsp_disp_hdmi_close(ubuffer[0]);
		suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		break;

	case DISP_CMD_HDMI_SET_MODE:
		ret = bsp_disp_hdmi_set_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_HDMI_GET_MODE:
		ret = bsp_disp_hdmi_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_HDMI_GET_HPD_STATUS:
		if(suspend_status != 0)	{
			ret = 0;
		}	else {
			ret = bsp_disp_hdmi_get_hpd_status(ubuffer[0]);
		}
		break;

	case DISP_CMD_HDMI_SUPPORT_MODE:
		ret = bsp_disp_hdmi_check_support_mode(ubuffer[0], ubuffer[1]);
		break;

	case DISP_CMD_HDMI_SET_SRC:
		ret = bsp_disp_hdmi_set_src(ubuffer[0], (__disp_lcdc_src_t)ubuffer[1]);
		break;

	//----vga----
	case DISP_CMD_VGA_ON:
		ret = bsp_disp_vga_open(ubuffer[0]);
		if(suspend_status != 0)	{
			suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_VGA;
		}
		break;

	case DISP_CMD_VGA_OFF:
		ret = bsp_disp_vga_close(ubuffer[0]);
		if(suspend_status != 0)	{
			suspend_output_type[ubuffer[0]] = DISP_OUTPUT_TYPE_NONE;
		}
		break;

	case DISP_CMD_VGA_SET_MODE:
		ret = bsp_disp_vga_set_mode(ubuffer[0], (__disp_vga_mode_t)ubuffer[1]);
		break;

	case DISP_CMD_VGA_GET_MODE:
		ret = bsp_disp_vga_get_mode(ubuffer[0]);
		break;

	case DISP_CMD_VGA_SET_SRC:
		ret = bsp_disp_vga_set_src(ubuffer[0], (__disp_lcdc_src_t)ubuffer[1]);
		break;

	//----framebuffer----
	case DISP_CMD_FB_REQUEST:
	{
		__disp_fb_create_para_t para;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(__disp_fb_create_para_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = Display_Fb_Request(ubuffer[0], &para);
		break;
	}

	case DISP_CMD_FB_RELEASE:
	ret = Display_Fb_Release(ubuffer[0]);
	break;

	case DISP_CMD_FB_GET_PARA:
	{
		__disp_fb_create_para_t para;

		ret = Display_Fb_get_para(ubuffer[0], &para);
		if(copy_to_user((void __user *)ubuffer[1],&para, sizeof(__disp_fb_create_para_t))) {
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_GET_DISP_INIT_PARA:
	{
		__disp_init_t para;

		ret = Display_get_disp_init_para(&para);
		if(copy_to_user((void __user *)ubuffer[0],&para, sizeof(__disp_init_t)))	{
			__wrn("copy_to_user fail\n");
			return  -EFAULT;
		}
		break;
	}

	case DISP_CMD_MEM_REQUEST:
		ret =  disp_mem_request(ubuffer[0],ubuffer[1]);
		break;

	//----for test----
	case DISP_CMD_MEM_RELEASE:
		ret =  disp_mem_release(ubuffer[0]);
		break;

	case DISP_CMD_MEM_SELIDX:
		g_disp_mm_sel = ubuffer[0];
		break;

	case DISP_CMD_MEM_GETADR:
		ret = g_disp_mm[ubuffer[0]].mem_start;
		break;

	case DISP_CMD_SUSPEND:
	{
		pm_message_t state;
        memset(&state, 0, sizeof(pm_message_t));

		ret = disp_suspend(NULL, state);
		break;
	}

	case DISP_CMD_RESUME:
		ret = disp_resume(NULL);
		break;

	case DISP_CMD_PRINT_REG:
		ret = bsp_disp_print_reg(1, ubuffer[0], NULL);
		break;

	default:
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

//extern int disp_attr_node_init(void);
//extern int capture_module_init(void);
//extern void  capture_module_exit(void);
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

	pr_info("[DISP]==disp_module_init finish==\n");

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

EXPORT_SYMBOL(disp_set_hdmi_func);
EXPORT_SYMBOL(sunxi_disp_get_source_ops);
EXPORT_SYMBOL(DRV_DISP_Init);
EXPORT_SYMBOL(disp_set_hdmi_hpd);

module_init(disp_module_init);
module_exit(disp_module_exit);


MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("display driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:disp");


