#include <linux/mali/mali_utgard.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk-private.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <mach/irqs.h>
#include <mach/sys_config.h>
#include <mach/platform.h>

#if defined(CONFIG_CPU_BUDGET_THERMAL)
#include <linux/cpu_budget_cooling.h>
static int cur_mode = 0;
#elif defined(CONFIG_SW_POWERNOW)
#include <mach/powernow.h>
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */

#include "mali_kernel_linux.h"

static struct clk *mali_clk = NULL;
static struct clk *gpu_pll  = NULL;
extern unsigned long totalram_pages;

struct __fb_addr_para 
{
	unsigned int fb_paddr;
	unsigned int fb_size;
};

extern void sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void mali_driver_early_suspend_scheduler(struct early_suspend *h);
static void mali_driver_late_resume_scheduler(struct early_suspend *h);
#endif /* CONFIG_HAS_EARLYSUSPEND */

extern int ths_read_data(int value);

static unsigned int freq_table[4] =
{
	128,  /* for early suspend */
	252,  /* for play video mode */
#if !defined(CONFIG_ARCH_SUN8IW7P1)
	384,  /* for normal mode */
	384,  /* for extreme mode */
#elif defined(CONFIG_ARCH_SUN8IW7P1)
	576,  /* for normal mode */
	576,  /* for extreme mode */
#endif
};

#if defined(CONFIG_ARCH_SUN8IW7P1)
static unsigned int thermal_ctrl_freq[] = {576, 432, 312, 120};

/* This data is for sensor, but the data of gpu may be 5 degress Centigrade higher */
static unsigned int temperature_threshold[] = {70, 80, 90};  /* degress Centigrade */
#endif /* defined(CONFIG_ARCH_SUN8IW7P1) */

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mali_early_suspend_handler = 
{
	.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB + 100,
	.suspend = mali_driver_early_suspend_scheduler,
	.resume = mali_driver_late_resume_scheduler,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static struct mali_gpu_device_data mali_gpu_data;

#if defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW9P1)
static struct resource mali_gpu_resources[]=
{
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(SUNXI_GPU_PBASE, SUNXI_IRQ_GPUGP, SUNXI_IRQ_GPUGPMMU, \
                                        SUNXI_IRQ_GPUPP0, SUNXI_IRQ_GPUPPMMU0, SUNXI_IRQ_GPUPP1, SUNXI_IRQ_GPUPPMMU1)
};
#elif defined(CONFIG_ARCH_SUN8IW7P1)
static struct resource mali_gpu_resources[]=
{
    MALI_GPU_RESOURCES_MALI400_MP2_PMU(SUNXI_GPU_PBASE, SUNXI_IRQ_GPU_GP, SUNXI_IRQ_GPU_GPMMU, \
                                        SUNXI_IRQ_GPU_PP0, SUNXI_IRQ_GPU_PPMMU0, SUNXI_IRQ_GPU_PP1, SUNXI_IRQ_GPU_PPMMU1)
};
#endif

static struct platform_device mali_gpu_device =
{
    .name = MALI_GPU_NAME_UTGARD,
    .id = 0,
    .dev.coherent_dma_mask = DMA_BIT_MASK(32),
};

/*

***************************************************************
 @Function   :get_gpu_clk

 @Description:Get gpu related clocks

 @Input      :None

 @Return     :Zero or error code
***************************************************************
*/
static int get_gpu_clk(void)
{
#if defined(CONFIG_ARCH_SUN8IW3P1)	
	gpu_pll = clk_get(NULL, PLL8_CLK);
#elif defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW9P1)
	gpu_pll = clk_get(NULL, PLL_GPU_CLK);
#endif
	if(!gpu_pll || IS_ERR(gpu_pll))
	{
		printk(KERN_ERR "Failed to get gpu pll clock!\n");
		return -1;
	} 
	
	mali_clk = clk_get(NULL, GPU_CLK);
	if(!mali_clk || IS_ERR(mali_clk))
	{
		printk(KERN_ERR "Failed to get mali clock!\n");     
		return -1;
	}
	
	return 0;
}

/*
***************************************************************
 @Function   :set_freq

 @Description:Set the frequency of gpu related clocks

 @Input	     :Frequency value

 @Return     :Zero or error code
***************************************************************
*/
static int set_freq(int freq /* MHz */)
{
	if(clk_set_rate(gpu_pll, freq*1000*1000))
    {
        printk(KERN_ERR "Failed to set gpu pll clock!\n");
		return -1;
    }

	if(clk_set_rate(mali_clk, freq*1000*1000))
	{
		printk(KERN_ERR "Failed to set mali clock!\n");
		return -1;
	}
	
	return 0;
}

#if defined(CONFIG_CPU_BUDGET_THERMAL) || defined(CONFIG_SW_POWERNOW) || defined(CONFIG_HAS_EARLYSUSPEND)
/*
***************************************************************
 @Function   :mali_set_freq

 @Description:Set the frequency of gpu related clocks with mali dvfs function

 @Input	     :Frequency value

 @Return     :Zero or error code
***************************************************************
*/
static int mali_set_freq(int freq /* MHz */)
{
	int err;
	mali_dev_pause();
	err = set_freq(freq);
	mali_dev_resume();
	return err;
}
#endif /* defined(CONFIG_CPU_BUDGET_THERMAL) || defined(CONFIG_SW_POWERNOW) || defined(CONFIG_HAS_EARLYSUSPEND) */

/*
***************************************************************
 @Function   :enable_gpu_clk

 @Description:Enable gpu related clocks

 @Input	     :None

 @Return     :None
***************************************************************
*/
void enable_gpu_clk(void)
{
	if(mali_clk->enable_count == 0)
	{
		if(clk_prepare_enable(gpu_pll))
		{
			printk(KERN_ERR "Failed to enable gpu pll!\n");
		}
		
		if(clk_prepare_enable(mali_clk))
		{
			printk(KERN_ERR "Failed to enable mali clock!\n");
		}
	}
}

/*
***************************************************************
 @Function   :disable_gpu_clk
	
 @Description:Disable gpu related clocks
	
 @Input	     :None

 @Return     :None
***************************************************************
*/
void disable_gpu_clk(void)
{
	if(mali_clk->enable_count == 1)
	{
		clk_disable_unprepare(mali_clk);
		clk_disable_unprepare(gpu_pll);
	}
}

#if defined(CONFIG_CPU_BUDGET_THERMAL)
/*
***************************************************************
 @Function   :mali_throttle_notifier_call

 @Description:The callback function of throttle notifier
		
 @Input	     :nfb, mode, cmd

 @Return     :retval
***************************************************************
*/
static int mali_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;

#if !defined(CONFIG_ARCH_SUN8IW7P1)
	if(mode == BUDGET_GPU_THROTTLE && cur_mode == 1)
	{
		mali_set_freq(freq_table[2]);
        cur_mode = 0;
    }
	else
	{
        if(cmd && (*(int *)cmd) == 1 && cur_mode != 1)
		{
			mali_set_freq(freq_table[3]);
           	cur_mode = 1;
        }
		else if(cmd && (*(int *)cmd) == 0 && cur_mode != 0)
		{
			mali_set_freq(freq_table[2]);
            cur_mode = 0;
        }
		else if(cmd && (*(int *)cmd) == 2 && cur_mode != 2)
		{
			mali_set_freq(freq_table[1]);
			cur_mode = 2;
		}
    }
#elif defined(CONFIG_ARCH_SUN8IW7P1)
	int temperature = 0;
	int i = 0;
	temperature = ths_read_data(4);
	for(i=0;i<sizeof(temperature_threshold)/sizeof(temperature_threshold[0]);i++)
	{
		if(temperature < temperature_threshold[i])
		{
			if(cur_mode != i)
			{
				mali_set_freq(thermal_ctrl_freq[i]);
				cur_mode = i;
			}
			goto out;
		}
	}
	
	mali_set_freq(thermal_ctrl_freq[i]);
	cur_mode = i;

out:		
#endif
	return retval;
}

static struct notifier_block mali_throttle_notifier = {
.notifier_call = mali_throttle_notifier_call,
};
#elif defined(CONFIG_SW_POWERNOW)
/*
***************************************************************
 @Function   :mali_powernow_notifier_call

 @Description:The callback function of powernow notifier

 @Input	     :this, mode, cmd

 @Return     :Zero
***************************************************************
*/
static int mali_powernow_notifier_call(struct notifier_block *this, unsigned long mode, void *cmd)
{	
	if(mode == 0 && cur_mode != 0)
	{
		mali_set_freq(freq_table[3]);
		cur_mode = 1;
	}
	else if(mode == 1 && cur_mode != 1)
	{
		mali_set_freq(freq_table[2]);
		cur_mode = 0;
	}	
	
    return 0;
}

static struct notifier_block mali_powernow_notifier = {
	.notifier_call = mali_powernow_notifier_call,
};
#endif

static ssize_t android_dvfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%ld MHz\n", clk_get_rate(gpu_pll)/(1000*1000));
}

static ssize_t android_dvfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int err;
	unsigned long freq;	
	
	err = strict_strtoul(buf, 10, &freq);
	if (err)
	{
		printk(KERN_ERR "Invalid parameter!");
		goto err_out;
	}

	mali_set_freq(freq);

err_out:	
	return count;
}

static DEVICE_ATTR(android, S_IRUGO|S_IWUGO, android_dvfs_show, android_dvfs_store);

static struct attribute *gpu_attributes[] =
{
    &dev_attr_android.attr,   
    NULL
};

struct attribute_group gpu_attribute_group = {
  .name = "dvfs",
  .attrs = gpu_attributes
};

/*
***************************************************************
 @Function   :parse_fex

 @Description:Parse fex file data of gpu

 @Input	     :None

 @Return     :None
***************************************************************
*/
static void parse_fex(void)
{
	script_item_u mali_used, mali_max_freq, mali_clk_freq;
	
#if defined(CONFIG_ARCH_SUN8IW7P1)
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("clock", "pll_gpu", &mali_clk_freq))
#else
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("clock", "pll8", &mali_clk_freq))
#endif
	{
		if(mali_clk_freq.val > 0)
		{
			freq_table[2] = mali_clk_freq.val;
		}
	}
	else
	{
		goto err_out;
	}	
	
	if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_used))
	{		
		if(mali_used.val == 1)
		{
			if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_extreme_freq", &mali_max_freq)) 
			{
                if (mali_max_freq.val >= mali_clk_freq.val)
				{
                    freq_table[3] = mali_max_freq.val;
                }
				else
				{
					freq_table[3] = mali_clk_freq.val;
				}
            }
			else
			{
				goto err_out;
			}
		}
	}
	else
	{
		goto err_out;
	}

	printk(KERN_INFO "Get mali parameter successfully\n");
	return;
	
err_out:
	printk(KERN_ERR "Failed to get mali parameter!\n");
	return;
}

/*
***************************************************************
 @Function   :mali_platform_init

 @Description:Init the power and clocks of gpu

 @Input	     :None

 @Return     :Zero or error code
***************************************************************
*/
static int mali_platform_init(void)
{
	parse_fex();
	
	if(get_gpu_clk())
	{
		goto err_out;
	}

	if(set_freq(freq_table[2]))
	{
		goto err_out;
	}
	
	enable_gpu_clk();
	
#if defined(CONFIG_CPU_BUDGET_THERMAL)
	register_budget_cooling_notifier(&mali_throttle_notifier);
#elif defined(CONFIG_SW_POWERNOW)
	register_sw_powernow_notifier(&mali_powernow_notifier);
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
		register_early_suspend(&mali_early_suspend_handler);
#endif /* CONFIG_HAS_EARLYSUSPEND */
	
	printk(KERN_INFO "Init Mali gpu successfully\n");
    return 0;

err_out:
	printk(KERN_ERR "Failed to init Mali gpu!\n");
	return -1;
}

/*
***************************************************************
 @Function   :mali_platform_device_unregister

 @Description:Unregister mali platform device

 @Input	     :None

 @Return     :Zero
***************************************************************
*/
static int mali_platform_device_unregister(void)
{
	platform_device_unregister(&mali_gpu_device);
	
#if defined(CONFIG_SW_POWERNOW)
	unregister_sw_powernow_notifier(&mali_powernow_notifier);
#endif /* CONFIG_SW_POWERNOW */
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&mali_early_suspend_handler);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	disable_gpu_clk();

	return 0;
}

/*
***************************************************************
 @Function   :sun8i_mali_platform_device_register

 @Description:Register mali platform device

 @Input	     :None

 @Return     :Zero or error code
***************************************************************
*/
int sun8i_mali_platform_device_register(void)
{
	int err;	
    struct __fb_addr_para fb_addr_para = {0};

    sunxi_get_fb_addr_para(&fb_addr_para);

    err = platform_device_add_resources(&mali_gpu_device, mali_gpu_resources, sizeof(mali_gpu_resources) / sizeof(mali_gpu_resources[0]));
    if (0 == err)
	{
        mali_gpu_data.fb_start = fb_addr_para.fb_paddr;
        mali_gpu_data.fb_size = fb_addr_para.fb_size;
		mali_gpu_data.shared_mem_size = totalram_pages  * PAGE_SIZE; /* B */

        err = platform_device_add_data(&mali_gpu_device, &mali_gpu_data, sizeof(mali_gpu_data));
        if(0 == err)
		{
            err = platform_device_register(&mali_gpu_device);
			
            if (0 == err)
			{
                if(0 != mali_platform_init())
				{
					return -1;
				}
#if defined(CONFIG_PM_RUNTIME)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
				pm_runtime_set_autosuspend_delay(&(mali_gpu_device.dev), 1000);
				pm_runtime_use_autosuspend(&(mali_gpu_device.dev));
#endif
				pm_runtime_enable(&(mali_gpu_device.dev));
#endif /* CONFIG_PM_RUNTIME */
				
                return 0;
            }
        }

        mali_platform_device_unregister();
    }
	
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
/*
***************************************************************
 @Function   :mali_driver_early_suspend_scheduler

 @Description:The callback function of early suspend

 @Input      :h

 @Return     :None
***************************************************************
*/
static void mali_driver_early_suspend_scheduler(struct early_suspend *h)
{
	mali_set_freq(freq_table[0]);
}

/*
***************************************************************
 @Function   :mali_driver_late_resume_scheduler

 @Description:The callback function of early suspend

 @Input      :h

 @Return     :None
***************************************************************
*/
static void mali_driver_late_resume_scheduler(struct early_suspend *h)
{
	mali_set_freq(freq_table[2]);
}
#endif /* CONFIG_HAS_EARLYSUSPEND */
