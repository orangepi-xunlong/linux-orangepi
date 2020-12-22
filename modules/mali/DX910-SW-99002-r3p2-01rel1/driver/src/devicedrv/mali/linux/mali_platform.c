#include "mali_kernel_common.h"
#include "mali_osk.h"

#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/module.h>  
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/clk/sunxi_name.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h> 
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>

#ifdef CONFIG_SW_POWERNOW
#include <mach/powernow.h>
#include <mach/includes.h>
#endif

static int mali_clk_div                  = 1;
static struct clk *h_mali_clk            = NULL;
static struct clk *h_gpu_pll             = NULL;
static struct regulator *mali_regulator  = NULL;
extern unsigned long totalram_pages;

struct __fb_addr_para {
unsigned int fb_paddr;
unsigned int fb_size;
};

extern void sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);

static unsigned int vol_list[2]=
{
	1140000,  /* for normal mode */
	1400000   /* for extremity mode */
};

static unsigned int freq_list[2]=
{
	408000000, /* for normal mode */
	600000000  /* for extremity mode */
};

static struct mali_gpu_device_data mali_gpu_data;

static struct resource mali_gpu_resources[]=
{                                    
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(SUNXI_GPU_PBASE, SUNXI_IRQ_GPUGP, SUNXI_IRQ_GPUGPMMU, \
                                        SUNXI_IRQ_GPUPP0, SUNXI_IRQ_GPUPPMMU0, SUNXI_IRQ_GPUPP1, SUNXI_IRQ_GPUPPMMU1)
};

static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.coherent_dma_mask = DMA_BIT_MASK(32),
};

/*
***************************************************************
 @Function	  :enable_gpu_clk

 @Description :Enable gpu related clocks

 @Input		  :None

 @Return	  :None
***************************************************************
*/
void enable_gpu_clk(void)
{
	if(clk_prepare_enable(h_gpu_pll))
	{
		printk(KERN_ERR "failed to enable gpu pll!\n");
	}	
	if(clk_prepare_enable(h_mali_clk))
	{
		printk(KERN_ERR "failed to enable mali clock!\n");
	}
}

/*
***************************************************************
 @Function	  :disable_gpu_clk

 @Description :Disable gpu related clocks

 @Input		  :None

 @Return	  :None
***************************************************************
*/
void disable_gpu_clk(void)
{
	clk_disable_unprepare(h_mali_clk);
	clk_disable_unprepare(h_gpu_pll);
}

#ifdef CONFIG_SW_POWERNOW
/*
***************************************************************
 @Function	  :powernow_notifier_call

 @Description :The callback of powernow notifier

 @Input		  :this, mode, cmd

 @Return	  :0
***************************************************************
*/
static int powernow_notifier_call(struct notifier_block *this, unsigned long mode, void *cmd)
{
	printk(KERN_ERR "powernow_notifier_call ###################\n");
	if(mode == 0)
	{
		if(regulator_set_voltage(mali_regulator, vol_list[1], vol_list[1]) != 0)
		{
			printk(KERN_ERR "Failed to set gpu voltage!\n");
		}	
		
		if(clk_set_rate(h_gpu_pll, freq_list[1]))
		{
			printk(KERN_ERR "Failed to set gpu pll clock!\n");
		}	
	}
	else if(mode == 1)
	{
		if(clk_set_rate(h_gpu_pll, freq_list[0]))
		{
			printk(KERN_ERR "Failed to set gpu pll clock!\n");
		}
`		if(regulator_set_voltage(mali_regulator, vol_list[0], vol_list[0]) != 0)
		{
			printk(KERN_ERR "Failed to set gpu voltage!\n");
		}		
	}
    return 0;
}

static struct notifier_block powernow_notifier = {
	.notifier_call = powernow_notifier_call,
};
#endif /* CONFIG_SW_POWERNOW */

/*
***************************************************************
 @Function	  :mali_platform_init

 @Description :Init gpu related clocks

 @Input		  :None

 @Return	  :_MALI_OSK_ERR_OK or error code
***************************************************************
*/
_mali_osk_errcode_t mali_platform_init(void)
{
	unsigned long rate = freq_list[0];
	script_item_u clk_drv;

	
	/* get mali voltage */
	mali_regulator = regulator_get(NULL, "axp22_dcdc2");
	if (IS_ERR(mali_regulator)) {
	    printk(KERN_ERR "failed to get mali regulator!\n");
        mali_regulator = NULL;
	}

	if(regulator_set_voltage(mali_regulator, vol_list[0], vol_list[0]) != 0)
	{
		printk(KERN_ERR "Failed to set gpu power voltage!\n");
	}
	
   	/* get mali clock */
	h_mali_clk = clk_get(NULL, GPU_CLK);
	if(!h_mali_clk || IS_ERR(h_mali_clk))
	{
		printk(KERN_ERR "failed to get mali clock!\n");
        return _MALI_OSK_ERR_FAULT;
	} 
	
	/* get gpu pll clock */
#ifdef CONFIG_ARCH_SUN8IW3	
	h_gpu_pll = clk_get(NULL, PLL8_CLK);
#endif /* CONFIG_ARCH_SUN8IW3 */
#ifdef CONFIG_ARCH_SUN8IW5
	h_gpu_pll = clk_get(NULL, PLL_GPU_CLK);
#endif /* CONFIG_ARCH_SUN8IW5 */
	if(!h_gpu_pll || IS_ERR(h_gpu_pll)){
		printk(KERN_ERR "Failed to get gpu pll clock!\n");
		return _MALI_OSK_ERR_FAULT;
	} 
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("clock", "pll8", &clk_drv)) 
	{
		if(clk_drv.val > 0)
		{
			rate = clk_drv.val*1000*1000;
		}
	}
	
	if(clk_set_rate(h_gpu_pll, rate))
	{
		printk(KERN_ERR "Failed to set gpu pll clock!\n");
		return _MALI_OSK_ERR_FAULT;
	}

	rate /=mali_clk_div; 
	
	if(clk_set_rate(h_mali_clk, rate))
	{
		printk(KERN_ERR "Failed to set mali clock!\n");
		return _MALI_OSK_ERR_FAULT;
	}
	
#ifdef CONFIG_SW_POWERNOW
	register_sw_powernow_notifier(&powernow_notifier);
#endif /* CONFIG_SW_POWERNOW */
	
    MALI_SUCCESS;
}

/*
***************************************************************
 @Function	  :sunxi_mali_platform_device_register

 @Description :Register mali platform device

 @Input		  :None

 @Return	  :0 or error code
***************************************************************
*/
int sunxi_mali_platform_device_register(void)
{
    int err;
    unsigned long mem_size = 0;
    struct __fb_addr_para fb_addr_para={0};

    sunxi_get_fb_addr_para(&fb_addr_para);

    err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
    if (0 == err){
        mali_gpu_data.fb_start = fb_addr_para.fb_paddr;
        mali_gpu_data.fb_size = fb_addr_para.fb_size;	
		mem_size = (totalram_pages  * PAGE_SIZE )/1024; /* KB */
	
		if(mem_size > 512*1024)
		{
			mali_gpu_data.shared_mem_size = 1024*1024*1024;
		}
		else
		{
			mali_gpu_data.shared_mem_size = 512*1024*1024;
		}

        err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
        if(0 == err)
		{
            err = platform_device_register(&mali_gpu_device);
            if (0 == err){
                if(_MALI_OSK_ERR_OK != mali_platform_init())
				{
					return _MALI_OSK_ERR_FAULT;
				}
#ifdef CONFIG_PM_RUNTIME
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif
				/* print mali gpu information */
				printk(KERN_INFO "=========================================================\n");
				printk(KERN_INFO "       Mali GPU Information         \n");
				printk(KERN_INFO "voltage             : %d mV\n", regulator_get_voltage(mali_regulator)/1000);
				printk(KERN_INFO "initial frequency   : %ld MHz\n", clk_get_rate(h_mali_clk));
				printk(KERN_INFO "frame buffer address: 0x%lx - 0x%lx\n", mali_gpu_data.fb_start, mali_gpu_data.fb_start + mali_gpu_data.shared_mem_size);
				printk(KERN_INFO "frame buffer size   : %ld MB\n", mali_gpu_data.shared_mem_size/(1024*1024));
				printk(KERN_INFO "=========================================================\n");
                return 0;
            }
        }

        platform_device_unregister(&mali_gpu_device);
    }
	
    return err;
}

/*
***************************************************************
 @Function	  :mali_platform_device_unregister

 @Description :Unregister mali platform device

 @Input		  :None

 @Return	  :None
***************************************************************
*/
void mali_platform_device_unregister(void)
{
    platform_device_unregister(&mali_gpu_device);
	if (mali_regulator) {
		   regulator_put(mali_regulator);
		   mali_regulator = NULL;
	}
	disable_gpu_clk();
}