/* linux/drivers/video/sunxi/disp/dev_fb.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * Framebuffer driver for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "drv_disp_i.h"
#include "dev_disp.h"

#if defined(CONFIG_ARCH_SUN6I)

struct composer_private_data
{
	struct work_struct    post2_cb_work[3];
	u32                   ovl_mode;
	u32	                  cb_w_conut;
	u32	                  cb_r_conut;
	u32                   cur_count;
	void                  (*cb_fn)(void *, int);
	void                  *cb_arg[10];
	bool                  b_no_output;
	u32                   reg_active[3];
	struct                mutex	runtime_lock;
};

typedef struct
{
	int                 post2_layers;
	disp_layer_info     layer_info[8];
	disp_window         fb_scn_win;

	int                 primary_display_layer_num;
	int                 show_black[3];
	int                 time_stamp;

	int                 acquireFenceFd[8];
	struct sync_fence   *acquireFence[8];
}setup_dispc_data_t;

static struct composer_private_data composer_priv;

static void disp_composer_proc(u32 sel)
{
	schedule_work(&composer_priv.post2_cb_work[sel]);
	return ;
}

static void imp_finish_cb(int force_all)
{
    int r_count = 0;

    r_count = composer_priv.cb_r_conut;
    while(r_count != composer_priv.cb_w_conut)
    {
        if(r_count >= 9)
        {
           r_count = 0;
        }
        else
        {
            r_count++;
        }
        if(!force_all && (r_count == composer_priv.cur_count))
        {
            break;
        }
        else if(composer_priv.cb_arg[r_count] != 0)
        {
            //printk(KERN_WARNING "##complete:%d\n", r_count);
            composer_priv.cb_fn(composer_priv.cb_arg[r_count], 1);
            composer_priv.cb_arg[r_count] = 0;
            composer_priv.cb_r_conut = r_count;
        }
    }
}

static void post2_cb_0(struct work_struct *work)
{
    mutex_lock(&composer_priv.runtime_lock);
    if(composer_priv.cur_count != composer_priv.cb_w_conut)
    {
        composer_priv.cur_count = composer_priv.cb_w_conut;
        //printk(KERN_WARNING "##take effect:%d\n", composer_priv.cur_count);
    }

    composer_priv.reg_active[0] = 1;
    if((composer_priv.reg_active[0] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(0)))
    	&& (composer_priv.reg_active[1] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(1))))
    {
        imp_finish_cb(0);
    }
    mutex_unlock(&composer_priv.runtime_lock);
}

static void post2_cb_1(struct work_struct *work)
{
    mutex_lock(&composer_priv.runtime_lock);
    composer_priv.reg_active[1] = 1;
    if((composer_priv.reg_active[0] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(0)))
    	&& (composer_priv.reg_active[1] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(1))))
    {
        imp_finish_cb(0);
    }
    mutex_unlock(&composer_priv.runtime_lock);
}

int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, int ui32DispcDataLength, void (*cb_fn)(void *, int), void *cb_arg)
{
    disp_layer_info         layer_info;
    int i,disp,hdl;
    int start_idx, layer_num = 0;
    int num_screens;

    num_screens = bsp_disp_feat_get_num_screens();

    for(disp = 0; disp < num_screens; disp++) {
    	bsp_disp_shadow_protect(disp, true);
    }

    for(disp = 0; disp < num_screens; disp++)
    {
        int haveFbTarget = 0;
        if(!psDispcData->show_black[disp])
        {
            if(disp == 0)
            {
                start_idx = 0;
                layer_num = psDispcData->primary_display_layer_num;
            }
            else
            {
                start_idx = psDispcData->primary_display_layer_num;
                layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
            }

            if((layer_num > 1) && (psDispcData->layer_info[start_idx + 1].zorder < psDispcData->layer_info[start_idx].zorder))
            {
                haveFbTarget = 1;
            }

            /* set the fb layer top */
            if(haveFbTarget)
            {
                /* save fb layer info */
                memcpy(&layer_info, &psDispcData->layer_info[start_idx], sizeof(disp_layer_info));
                /* order other layer */
                for(i=0; i<layer_num-1; i++)
                {
                    memcpy(&psDispcData->layer_info[start_idx+i], &psDispcData->layer_info[start_idx+i+1], sizeof(disp_layer_info));
                }
                /* restore fb layer info at the last element */
                memcpy(&psDispcData->layer_info[start_idx+i], &layer_info, sizeof(disp_layer_info));
            }

            for(i=0; i<4; i++)
            {
                hdl = i;

                bsp_disp_layer_get_info(disp, hdl, &layer_info);
                if(layer_info.mode == DISP_LAYER_WORK_MODE_SCALER)
                {
                    if((i >= layer_num) || (psDispcData->layer_info[start_idx + i].mode == DISP_LAYER_WORK_MODE_NORMAL))
                    {
                        bsp_disp_layer_set_info(disp, hdl, &psDispcData->layer_info[start_idx + i]);
                    }
                }
            }
        }
    }

    for(disp = 0; disp < 2; disp++)
    {
        if(!psDispcData->show_black[disp])
        {
            if(disp == 0)
            {
                start_idx = 0;
                layer_num = psDispcData->primary_display_layer_num;
            }
            else
            {
                start_idx = psDispcData->primary_display_layer_num;
                layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
            }

            for(i=0; i<4; i++)
            {
                hdl = i;

                if(i < layer_num)
                {
                    memcpy(&layer_info, &psDispcData->layer_info[start_idx + i], sizeof(disp_layer_info));

                    if(layer_info.fb.format == DISP_FORMAT_YUV420_P)
                    {
                        layer_info.fb.addr[2] = layer_info.fb.addr[0] + layer_info.fb.size.width * layer_info.fb.size.height;
                        layer_info.fb.addr[1] = layer_info.fb.addr[2] + (layer_info.fb.size.width * layer_info.fb.size.height)/4;
                    }
                    bsp_disp_layer_set_info(disp, hdl, &layer_info);
                    bsp_disp_layer_enable(disp, hdl);
                    //bsp_disp_layer_set_top(disp, hdl);
                }
                else
                {
                    bsp_disp_layer_disable(disp, hdl);
                }

            }
        }
        else
        {
            for(i=0; i<4; i++)
            {
                hdl = i;

                bsp_disp_layer_disable(disp, hdl);
            }
            //printk(KERN_WARNING "###########close all layers %d", disp);
        }

    }

    for(disp = 0; disp < num_screens; disp++) {
    	bsp_disp_shadow_protect(disp, false);
    }

    mutex_lock(&composer_priv.runtime_lock);
    if(composer_priv.b_no_output)
    {
        cb_fn(cb_arg, 1);
    }
    else
    {
    	composer_priv.cb_fn = cb_fn;

    	if(composer_priv.cb_w_conut >= 9)
    	{
    	   composer_priv.cb_w_conut = 0;
    	}
    	else
    	{
    	    composer_priv.cb_w_conut++;
    	}
    	composer_priv.cb_arg[composer_priv.cb_w_conut] = cb_arg;

    	//printk(KERN_WARNING "##queue:%d time_stamp:%d layer_num:%d %d\n", composer_priv.cb_w_conut, psDispcData->time_stamp, psDispcData->primary_display_layer_num, psDispcData->post2_layers-psDispcData->primary_display_layer_num);
	}

	if(psDispcData->post2_layers > psDispcData->primary_display_layer_num)//have external display
	{
    	composer_priv.reg_active[0] = 0;
    	composer_priv.reg_active[1] = 0;
	}
	else
	{
    	composer_priv.reg_active[0] = 0;
    	composer_priv.reg_active[1] = 1;
	}
	mutex_unlock(&composer_priv.runtime_lock);

  return 0;
}

static int hwc_suspend(void)
{
	mutex_lock(&composer_priv.runtime_lock);
	composer_priv.b_no_output = 1;
	imp_finish_cb(1);
	mutex_unlock(&composer_priv.runtime_lock);
	__inf("%s\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	__inf("%s\n", __func__);
	return 0;
}

s32 composer_init(void)
{
	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	INIT_WORK(&composer_priv.post2_cb_work[0], post2_cb_0);
	INIT_WORK(&composer_priv.post2_cb_work[1], post2_cb_1);
	mutex_init(&composer_priv.runtime_lock);
	disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);

  return 0;
}

EXPORT_SYMBOL(dispc_gralloc_queue);

#elif defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1)

#include <linux/file.h>
struct composer_private_data
{
	struct work_struct    post2_cb_work[3];
	u32                   ovl_mode;
	u32	                  cb_w_conut;
	u32	                  cb_r_conut;
	u32                   cur_count;
	void                  (*cb_fn)(void *, int);
	void                  *cb_arg[10];
	bool                  b_no_output;
	u32                   reg_active[3];
	struct                mutex	runtime_lock;

	struct list_head        update_regs_list;
	struct sw_sync_timeline *timeline;
	int                     timeline_max;
	struct mutex            update_regs_list_lock;
	spinlock_t              update_reg_lock;
	struct work_struct      commit_work;
	struct sync_fence       *acquireFence[8];
};



typedef struct
{
	int                 post2_layers;
	disp_layer_info     layer_info[8];
	disp_window         fb_scn_win;

	int                 primary_display_layer_num;
	int                 show_black[3];
	int                 time_stamp;

	int                 acquireFenceFd[8];
	struct sync_fence   *acquireFence[8];
}setup_dispc_data_t;

typedef struct
{
    struct list_head    list;
    setup_dispc_data_t    hwc_data;
}dispc_data_list_t;

extern fb_info_t g_fbi;
static struct   mutex	gcommit_mutek;
static struct composer_private_data composer_priv;



#define _ALIGN( value, base ) (((value) + ((base) - 1)) & ~((base) - 1))
extern int Fb_wait_for_vsync(struct fb_info *info);
static int dispc_update_regs(setup_dispc_data_t *psDispcData)
{
    disp_layer_info layer_info;
    int i,disp,hdl;
    int start_idx, layer_num = 0;
    int err = 0;
    int num_screens;

    num_screens = bsp_disp_feat_get_num_screens();

    //printk(KERN_WARNING "###wait acquirefence\n");
    for(i=0; i<psDispcData->post2_layers; i++)
    {

	        if(psDispcData->acquireFence[i])
	        {
			//printk("composer_priv.acquireFence[%d] = %d\n",i,psDispcData->acquireFence[i]);
	            err = sync_fence_wait(psDispcData->acquireFence[i] ,1000);

		    	sync_fence_put(psDispcData->acquireFence[i]);

		    	if (err < 0)
	            {
	                __wrn("synce_fence_wait() timeout, layer:%d, fd:%d\n", i, psDispcData->acquireFenceFd[i]);
	                sw_sync_timeline_inc(composer_priv.timeline, 1);
					return -1;
	            }


		}
    }

	if(composer_priv.b_no_output == 1){
		sw_sync_timeline_inc(composer_priv.timeline, 1);
		return 0;
	}

	//spin_lock(&(composer_priv.update_reg_lock));

	for(disp = 0; disp < num_screens; disp++) {
    	bsp_disp_shadow_protect(disp, true);
    }

    for(disp = 0; disp < num_screens; disp++)
    {
        int haveFbTarget = 0;
        if(!psDispcData->show_black[disp])
        {
            if(disp == 0)
            {
                start_idx = 0;
                layer_num = psDispcData->primary_display_layer_num;
            }
            else
            {
                start_idx = psDispcData->primary_display_layer_num;
                layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
            }

            if((layer_num > 1) && (psDispcData->layer_info[start_idx + 1].zorder < psDispcData->layer_info[start_idx].zorder))
            {
                haveFbTarget = 1;
            }

            /* set the fb layer top */
            if(haveFbTarget)
            {
                /* save fb layer info */
                memcpy(&layer_info, &psDispcData->layer_info[start_idx], sizeof(disp_layer_info));
                /* order other layer */
                for(i=0; i<layer_num-1; i++)
                {
                    memcpy(&psDispcData->layer_info[start_idx+i], &psDispcData->layer_info[start_idx+i+1], sizeof(disp_layer_info));
                }
                /* restore fb layer info at the last element */
                memcpy(&psDispcData->layer_info[start_idx+i], &layer_info, sizeof(disp_layer_info));
            }

            for(i=0; i<4; i++)
            {
                hdl = i;
                memset(&layer_info,0,sizeof(disp_layer_info));
                bsp_disp_layer_get_info(disp, hdl, &layer_info);
                if(layer_info.mode == DISP_LAYER_WORK_MODE_SCALER)
                {
                    if((i >= layer_num) || (psDispcData->layer_info[start_idx + i].mode == DISP_LAYER_WORK_MODE_NORMAL))
                    {
                        bsp_disp_layer_set_info(disp, hdl, &psDispcData->layer_info[start_idx + i]);
                    }
                }

            }
        }
    }
    for(disp = 0; disp < num_screens; disp++)
    {
	if(!psDispcData->show_black[disp])
	{
	    if(disp == 0)
	    {
		start_idx = 0;
		layer_num = psDispcData->primary_display_layer_num;
	    }
	    else
	    {
		start_idx = psDispcData->primary_display_layer_num;
		layer_num = psDispcData->post2_layers - psDispcData->primary_display_layer_num;
	    }

	    for(i=0; i<4; i++)
	    {
			hdl = i;

			if(i < layer_num )
			{
			    memcpy(&layer_info, &psDispcData->layer_info[start_idx + i], sizeof(disp_layer_info));

			    if(layer_info.fb.format == DISP_FORMAT_YUV420_P)
					{
						  layer_info.fb.addr[2] = layer_info.fb.addr[0] + layer_info.fb.size.width * layer_info.fb.size.height;
					    //layer_info.fb.addr[1] = layer_info.fb.addr[2] + (layer_info.fb.size.width * layer_info.fb.size.height)/4;
					    layer_info.fb.addr[1] = layer_info.fb.addr[2] + (_ALIGN(layer_info.fb.size.width/2,16) * layer_info.fb.size.height)/2;
					}
				if((layer_info.fb.format == DISP_FORMAT_YUV420_SP_UVUV) || (layer_info.fb.format == DISP_FORMAT_YUV420_SP_VUVU))
					{
					    layer_info.fb.addr[1] = layer_info.fb.addr[0] + layer_info.fb.size.width * layer_info.fb.size.height;
					    //layer_info.fb.addr[1] = layer_info.fb.addr[2] + (layer_info.fb.size.width * layer_info.fb.size.height)/4;
					    //layer_info.fb.addr[1] = layer_info.fb.addr[2] + (_ALIGN(layer_info.fb.size.width/2,16) * layer_info.fb.size.height)/2;
					}
				//printk("### layer_info.fb.addr = 0x%x\n",layer_info.fb.addr[0]);
			   //bsp_disp_layer_close(disp, hdl);
			    bsp_disp_layer_set_info(disp, hdl, &layer_info);
			    bsp_disp_layer_enable(disp, hdl);
			    //printk(KERN_WARNING "##update layer:%d addr:%x\n", psDispcData->primary_display_layer_num, layer_info.fb.addr[0]);
			}
			else
			{
			    bsp_disp_layer_disable(disp, hdl);
			}

	    }
	}
	else
	{

	    for(i=0; i<4; i++)
	    {
			hdl = i;

			bsp_disp_layer_disable(disp, hdl);
	    }
	    printk("###########close all layers %d\n", disp);
	}

    }
	for(disp = 0; disp < num_screens; disp++) {
    	bsp_disp_shadow_protect(disp, false);
  }
	//spin_unlock(&(composer_priv.update_reg_lock));

    Fb_wait_for_vsync(g_fbi.fbinfo[0]);
    //printk(KERN_WARNING "##release fence\n");
   sw_sync_timeline_inc(composer_priv.timeline, 1);
    return 0;
}

static void hwc_commit_work(struct work_struct *work)
{
    dispc_data_list_t *data, *next;
    struct list_head saved_list;


    mutex_lock(&(gcommit_mutek));

    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);

    mutex_unlock(&(composer_priv.update_regs_list_lock));


    list_for_each_entry_safe(data, next, &saved_list, list)
    {
         list_del(&data->list);
         dispc_update_regs(&data->hwc_data);
         kfree(data);

    }
	mutex_unlock(&(gcommit_mutek));
}

static int hwc_commit(int sel, setup_dispc_data_t *disp_data)
{
	dispc_data_list_t *disp_data_list;
	struct sync_fence *fence;
	struct sync_pt *pt;
	int fd = -1;
	int i = 0;

	for(i=0; i<disp_data->post2_layers; i++)
	{

		if(disp_data->acquireFenceFd[i] >= 0)
		{
			disp_data->acquireFence[i] = sync_fence_fdget(disp_data->acquireFenceFd[i]);
			if(!disp_data->acquireFence[i])
			{
				printk("sync_fence_fdget()fail, layer:%d fd:%d\n", i, disp_data->acquireFenceFd[i]);
				return -1;
			}
		}
	}

//	if(composer_priv.b_no_output == 0)
	if(1)
	{

		fd = get_unused_fd();
		if (fd < 0)
		{
		    return -1;
		}
		composer_priv.timeline_max++;
		pt = sw_sync_pt_create(composer_priv.timeline, composer_priv.timeline_max);
		fence = sync_fence_create("display", pt);
		sync_fence_install(fence, fd);

		disp_data_list = kzalloc(sizeof(dispc_data_list_t), GFP_KERNEL);
		memcpy(&disp_data_list->hwc_data, disp_data, sizeof(setup_dispc_data_t));
		//spin_lock(&(composer_priv.update_reg_lock));
		mutex_lock(&(composer_priv.update_regs_list_lock));
		list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
		//spin_unlock(&(composer_priv.update_reg_lock));
		mutex_unlock(&(composer_priv.update_regs_list_lock));
		schedule_work(&composer_priv.commit_work);
		//hwc_commit_work(0);
	}
	else
	{
		flush_scheduled_work();
		/*
		if(composer_priv.timeline_max > composer_priv.timeline->value)
		{
			mutex_lock(&(composer_priv.update_regs_list_lock));
			sw_sync_timeline_inc(composer_priv.timeline, 1);
			mutex_unlock(&(composer_priv.update_regs_list_lock));
		}
		*/
	}
//	printk("%s:fd = %d,timemax = %d,,timeline.value=%d,hasFd = %d\n",__FILE__,fd,composer_priv.timeline_max,composer_priv.timeline->value,hasFd);

	return fd;

}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
#if 0
	int ret = -1;

	if(DISP_CMD_HWC_COMMIT == cmd) {
		setup_dispc_data_t para;
		unsigned long ubuffer[4] = {0};
		unsigned long karg[4];

		if (copy_from_user((void*)karg,(void __user*)arg,4*sizeof(unsigned long))) {
			__wrn("copy_from_user fail\n");
			return -EFAULT;
		}

		ubuffer[0] = *(unsigned long*)karg;
		ubuffer[1] = (*(unsigned long*)(karg+1));
		ubuffer[2] = (*(unsigned long*)(karg+2));
		ubuffer[3] = (*(unsigned long*)(karg+3));

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(setup_dispc_data_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = hwc_commit(ubuffer[0], &para);
	}
#else
	int ret = -1;

	if(DISP_CMD_HWC_COMMIT == cmd) {
		setup_dispc_data_t para;
		unsigned long *ubuffer = (unsigned long *)arg;

		if(copy_from_user(&para, (void __user *)ubuffer[1],sizeof(setup_dispc_data_t))) {
			__wrn("copy_from_user fail\n");
			return  -EFAULT;
		}
		ret = hwc_commit(ubuffer[0], &para);
	}
#endif
	return ret;
}

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	__inf("%s\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	__inf("%s\n", __func__);
	return 0;
}

s32 composer_init(void)
{
	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	mutex_init(&composer_priv.runtime_lock);
	mutex_init(&gcommit_mutek);
 	INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
	INIT_LIST_HEAD(&composer_priv.update_regs_list);
	composer_priv.timeline = sw_sync_timeline_create("sunxi-fb");
	composer_priv.timeline_max = 1;
	composer_priv.b_no_output = 0;
	mutex_init(&composer_priv.update_regs_list_lock);
	spin_lock_init(&(composer_priv.update_reg_lock));
	disp_register_ioctl_func(DISP_CMD_HWC_COMMIT, hwc_commit_ioctl);
	disp_register_standby_func(hwc_suspend, hwc_resume);

  return 0;
}

#elif defined(CONFIG_ARCH_SUN9IW1P1)
static struct composer_private_data composer_priv;

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
static struct   mutex	gcommit_mutek;

typedef struct
{
    int layer_num[3];
    disp_layer_info layer_info[3][4];
    void* hConfigData;
}setup_dispc_data_t;
typedef struct
{
    struct list_head    list;
    setup_dispc_data_t    hwc_data;
}dispc_data_list_t;

struct disp_composer_ops
{
	int (*get_screen_width)(u32 screen_id);
	int (*get_screen_height)(u32 screen_id);
	int (*get_output_type)(u32 screen_id);
	int (*hdmi_enable)(u32 screen_id);
	int (*hdmi_disable)(u32 screen_id);
	int (*hdmi_set_mode)(u32 screen_id,  disp_tv_mode mode);
	int (*hdmi_get_mode)(u32 screen_id);
	int (*hdmi_check_support_mode)(u32 screen_id,  u8 mode);
	int (*is_support_scaler_layer)(unsigned int screen_id, unsigned int src_w, unsigned int src_h,unsigned int out_w, unsigned int out_h);
	int (*dispc_gralloc_queue)(setup_dispc_data_t *psDispcData, int ui32DispcDataLength, void (*cb_fn)(void *));
};

struct composer_private_data
{
	struct work_struct    post2_cb_work;
	u32                   ovl_mode;
	u32	                  cb_w_conut;
	u32                   cur_count;
	bool                  b_no_output;
	u32                   reg_active[3];
	struct                mutex	runtime_lock;
	u32                   disp_composer_mode;

	struct list_head        update_regs_list;
	struct sw_sync_timeline *timeline;
	unsigned int            timeline_max;
	struct mutex            update_regs_list_lock;
	spinlock_t              update_reg_lock;
	struct work_struct      commit_work;
    struct workqueue_struct *Display_commit_work;
 
};


int dispc_gralloc_queue(setup_dispc_data_t *psDispcData);

static int hwc_check_disp_ready(unsigned int cmd, unsigned long arg)
{
	return (!composer_priv.b_no_output);
}

static void hwc_commit_work(struct work_struct *work)
{
    dispc_data_list_t *data, *next;
    struct list_head saved_list;

    mutex_lock(&(gcommit_mutek));

    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);

    mutex_unlock(&(composer_priv.update_regs_list_lock));


    list_for_each_entry_safe(data, next, &saved_list, list)
    {

        int NumberOfLayer;
        int i;
        struct sync_fence **AcquireFence;
        list_del(&data->list);
        AcquireFence = (struct sync_fence **)data->hwc_data.hConfigData;
        NumberOfLayer = data->hwc_data.layer_num[0]+data->hwc_data.layer_num[1]+data->hwc_data.layer_num[2];
        for(i=0;i<NumberOfLayer;i++)
        {
            if(AcquireFence[i])
	        {
                int err = sync_fence_wait(AcquireFence[i],1000);
		    	sync_fence_put(AcquireFence[i]);        
		    	if (err < 0)
	            {
	                printk("synce_fence_wait timeout AcquireFence:%p\n",AcquireFence[i]);
                    composer_priv.cb_w_conut++;
                    sw_sync_timeline_inc(composer_priv.timeline, 1);
					goto free;
	            }
            }
        }
        dispc_gralloc_queue(&data->hwc_data);
free:
        kfree(AcquireFence);
        kfree(data);
    }
	mutex_unlock(&(gcommit_mutek));
}

static int hwc_commit( setup_dispc_data_t *disp_data)
{
	dispc_data_list_t *disp_data_list;
	struct sync_fence *fence;
	struct sync_pt *pt;
	int fd = -1;
	int i = 0;
    int *FdsOfFence;
    struct sync_fence **AcquireFence;
    FdsOfFence = disp_data->hConfigData;
    AcquireFence = kzalloc((12*sizeof(struct sync_fence*)),GFP_KERNEL);
	for(i=0; i<12; i++)
	{
        if(FdsOfFence[i]>=0)
        {
            AcquireFence[i] = sync_fence_fdget(FdsOfFence[i]);
            if(!AcquireFence[i])
            {
                printk("sync_fence_fdget()fail,fd[%d]:%d\n",i,FdsOfFence[i] );
                kfree(AcquireFence);
                return -1;
            }
        }
	}
    disp_data->hConfigData= (void *)AcquireFence;
    if(composer_priv.b_no_output == 0)
    {
	    if(disp_data->layer_num[0]+disp_data->layer_num[1]+disp_data->layer_num[2] > 0)
	    {

            fd = get_unused_fd();
            if (fd < 0)
            {
                kfree(AcquireFence);
                return -1;
            }
            composer_priv.timeline_max++;
            pt = sw_sync_pt_create(composer_priv.timeline, composer_priv.timeline_max);
            fence = sync_fence_create("display", pt);
            sync_fence_install(fence, fd);
            disp_data_list = kzalloc(sizeof(dispc_data_list_t), GFP_KERNEL);
            memcpy(&disp_data_list->hwc_data, disp_data, sizeof(setup_dispc_data_t));
            mutex_lock(&(composer_priv.update_regs_list_lock));
            list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
            mutex_unlock(&(composer_priv.update_regs_list_lock));
            queue_work(composer_priv.Display_commit_work,&composer_priv.commit_work);
	    }else{
            kfree(AcquireFence);
        }
    }else{
        flush_workqueue(composer_priv.Display_commit_work);
        kfree(AcquireFence);
    }
	return fd;

}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = -1;
    setup_dispc_data_t *para = NULL;
    int fd[12] = {0,};
	if(DISP_CMD_HWC_COMMIT == cmd) {
        
        unsigned long *ubuffer = (unsigned long *)arg;
        para = kzalloc(sizeof(setup_dispc_data_t),GFP_KERNEL);
        
        if(copy_from_user(para, (void __user *)ubuffer[1], sizeof(setup_dispc_data_t))) {
            printk("copy_from_user fail\n");
            return  -EFAULT;
		}
        
        if(copy_from_user(fd, (void __user *)para->hConfigData, 12*sizeof(int))) {
            printk("copy_from_user fail\n");
            return  -EFAULT;
		}
        para->hConfigData = (void *)fd;
        ret = hwc_commit(para);
        kfree(para);
        
	}
	return ret;
}

struct disp_pixel_bpp
{
	disp_pixel_format format;
	u32 bpp;
};

static struct composer_private_data composer_priv;

//for frame count proc
s32 disp_get_frame_count(u32 screen_id, char *buf)
{
	u32 i, count = 0;
	u32 current_index, current_time, last_time;
	u32 temp_time;
	current_index = (fcount_data.current_index == 0) ? 99 : (fcount_data.current_index - 1);
	current_time = jiffies;
	last_time = current_time - 100;

	for(i = 0; i < 99; i++) {
		if(current_index == 0)
			current_index = 100;

		current_index--;

		if(fcount_data.frame_refresh_array[current_index] < last_time) {
			count = i;
			break;
		}

		temp_time = fcount_data.frame_refresh_array[current_index] - last_time;

		if(temp_time < 2) {
			count = i + 1;
			break;
		}
	}

	printk("[DISP] acquire %u freshrate %u release %u, disp %u, video<%u,%u>, vsync<%u,%u,%u>, skip<%u,%u,%u>\n",
		fcount_data.acquire_count, count, fcount_data.release_count, fcount_data.display_count,
		fcount_data.video_count, fcount_data.video_strict_count,
		fcount_data.vsync_count[0], fcount_data.vsync_count[1], fcount_data.vsync_count[2],
		fcount_data.skip_count[0], fcount_data.skip_count[1], fcount_data.skip_count[2]);

	return sprintf(buf, "[DISP] acquire %u freshrate %u release %u, disp %u, video<%u,%u>, vsync<%u,%u,%u>, skip<%u,%u,%u>\n",
		fcount_data.acquire_count, count, fcount_data.release_count, fcount_data.display_count,
		fcount_data.video_count, fcount_data.video_strict_count,
		fcount_data.vsync_count[0], fcount_data.vsync_count[1], fcount_data.vsync_count[2],
		fcount_data.skip_count[0], fcount_data.skip_count[1], fcount_data.skip_count[2]);
}

static void disp_composer_proc(u32 sel)
{
	if((sel == 0) ||
		((sel == 1) && (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(0)) && (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(2))) ||
		((sel == 2) && (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(0))) ) {
			composer_priv.cur_count = composer_priv.cb_w_conut;
	}
	composer_priv.reg_active[sel] = 1;

	if((composer_priv.reg_active[0] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(0)))
	  && (composer_priv.reg_active[1] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(1)))
	  && (composer_priv.reg_active[2] || (DISP_OUTPUT_TYPE_NONE == bsp_disp_get_output_type(2)))) {
		if(composer_priv.timeline->value != composer_priv.cb_w_conut) {
			schedule_work(&composer_priv.post2_cb_work);
		}
	}
	return ;
}

static void imp_finish_cb(bool force_all)
{
    int flag = 0;

    while(composer_priv.timeline->value != composer_priv.cb_w_conut)
    {
        
        if(!force_all && (composer_priv.timeline->value == composer_priv.cur_count-1 ))
        {
            break;
        }

        sw_sync_timeline_inc(composer_priv.timeline, 1);
	    fcount_data.release_count++;
	    flag = 1;
    }
    if(1 == flag)
        fcount_data.display_count++;
}

static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.runtime_lock);
    imp_finish_cb(0);
	mutex_unlock(&composer_priv.runtime_lock);
}

static struct disp_pixel_bpp pixel_bpp_tbl[] = {
	{DISP_FORMAT_ARGB_8888                   , 32},
	{DISP_FORMAT_ABGR_8888                   , 32},
	{DISP_FORMAT_RGBA_8888                   , 32},
	{DISP_FORMAT_BGRA_8888                   , 32},
	{DISP_FORMAT_XRGB_8888                   , 32},
	{DISP_FORMAT_XBGR_8888                   , 32},
	{DISP_FORMAT_RGBX_8888                   , 32},
	{DISP_FORMAT_BGRX_8888                   , 32},
	{DISP_FORMAT_RGB_888                     , 24},
	{DISP_FORMAT_BGR_888                     , 24},
	{DISP_FORMAT_RGB_565                     , 16},
	{DISP_FORMAT_BGR_565                     , 16},
	{DISP_FORMAT_ARGB_4444                   , 16},
	{DISP_FORMAT_ABGR_4444                   , 16},
	{DISP_FORMAT_RGBA_4444                   , 16},
	{DISP_FORMAT_BGRA_4444                   , 16},
	{DISP_FORMAT_ARGB_1555                   , 16},
	{DISP_FORMAT_ABGR_1555                   , 16},
	{DISP_FORMAT_RGBA_5551                   , 16},
	{DISP_FORMAT_BGRA_5551                   , 16},

	{DISP_FORMAT_YUV444_I_AYUV               , 24},
	{DISP_FORMAT_YUV444_I_VUYA               , 24},
	{DISP_FORMAT_YUV422_I_YVYU               , 16},
	{DISP_FORMAT_YUV422_I_YUYV               , 16},
	{DISP_FORMAT_YUV422_I_UYVY               , 16},
	{DISP_FORMAT_YUV422_I_VYUY               , 16},
	{DISP_FORMAT_YUV444_P                    , 24},
	{DISP_FORMAT_YUV422_P                    , 16},
	{DISP_FORMAT_YUV420_P                    , 12},
	{DISP_FORMAT_YUV411_P                    , 12},
	{DISP_FORMAT_YUV422_SP_UVUV              , 16},
	{DISP_FORMAT_YUV422_SP_VUVU              , 16},
	{DISP_FORMAT_YUV420_SP_UVUV              , 12},
	{DISP_FORMAT_YUV420_SP_VUVU              , 12},
	{DISP_FORMAT_YUV411_SP_UVUV              , 12},
	{DISP_FORMAT_YUV411_SP_VUVU              , 12},
	{DISP_FORMAT_YUV422_SP_TILE_UVUV         , 16},
	{DISP_FORMAT_YUV422_SP_TILE_VUVU         , 16},
	{DISP_FORMAT_YUV420_SP_TILE_UVUV         , 12},
	{DISP_FORMAT_YUV420_SP_TILE_VUVU         , 12},
	{DISP_FORMAT_YUV411_SP_TILE_UVUV         , 12},
	{DISP_FORMAT_YUV411_SP_TILE_VUVU         , 12},
	{DISP_FORMAT_YUV422_SP_TILE_128X32_UVUV  , 16},
	{DISP_FORMAT_YUV422_SP_TILE_128X32_VUVU  , 16},
	{DISP_FORMAT_YUV420_SP_TILE_128X32_UVUV  , 12},
	{DISP_FORMAT_YUV420_SP_TILE_128X32_VUVU  , 12},
	{DISP_FORMAT_YUV411_SP_TILE_128X32_UVUV  , 12},
	{DISP_FORMAT_YUV411_SP_TILE_128X32_VUVU  , 12},
};

static int disp_get_pixel_bpp(disp_pixel_format format)
{
	int i,size;
	int bpp = 32;

	size = sizeof(pixel_bpp_tbl) / sizeof(struct disp_pixel_bpp);
	for(i=0; i<size; i++) {
		if(pixel_bpp_tbl[i].format == format)
			bpp = pixel_bpp_tbl[i].bpp;
	}
	return bpp;
}

#if defined(CONFIG_DEVFREQ_DRAM_FREQ) && defined(CONFIG_ARCH_SUN9IW1P1)
	//add dram bandwidth cb declare
#endif
static int disp_calc_bandwidth(setup_dispc_data_t *psDispcData)
{
	disp_layer_info layer_info;
  int i,disp,hdl;
  int num_screens;
  int bandwidth = 0;

	num_screens = bsp_disp_feat_get_num_screens();
	for(disp = 0; disp < num_screens; disp++) {
		for(i=0; i<4; i++) {
			hdl = i;

			if(i < psDispcData->layer_num[disp]) {
				int bpp = 32;
				int width = 0,height = 0;

				memcpy(&layer_info, &psDispcData->layer_info[disp][i], sizeof(disp_layer_info));

				bpp = disp_get_pixel_bpp(layer_info.fb.format);
				width = layer_info.fb.src_win.width;
				height = layer_info.fb.src_win.height;
				bandwidth += width * height * bpp / 8;
			}
		}
	}
	bandwidth = ((bandwidth + 1023)/ 1024 + 1023) / 1024;

#if defined(CONFIG_DEVFREQ_DRAM_FREQ) && defined(CONFIG_ARCH_SUN9IW1P1)
	//add dram bandwidth cb
#endif
	return bandwidth;
}

int dispc_gralloc_queue(setup_dispc_data_t *psDispcData)
{
    disp_layer_info         layer_info;
    int i,disp,hdl;
    int num_screens;

    num_screens = bsp_disp_feat_get_num_screens();

    for(disp = 0; disp < num_screens; disp++) {
			bsp_disp_shadow_protect(disp, true);
    }
	disp_calc_bandwidth(psDispcData);

    for(disp = 0; disp < num_screens; disp++)
    {
	    for(i=0; i<4; i++)
	    {
	        hdl = i;

	        bsp_disp_layer_get_info(disp, hdl, &layer_info);
	        if(layer_info.mode == DISP_LAYER_WORK_MODE_SCALER)
	        {
	            if((i >= psDispcData->layer_num[disp]) || (psDispcData->layer_info[disp][i].mode == DISP_LAYER_WORK_MODE_NORMAL))
	            {
	                bsp_disp_layer_set_info(disp, hdl, &psDispcData->layer_info[disp][i]);
	            }
	        }
	    }
    }

    for(disp = 0; disp < num_screens; disp++)
    {
	    disp_layer_info layer_info_tmp;
	    for(i=0; i<4; i++)
	    {
	        hdl = i;

	        if(i < psDispcData->layer_num[disp])
	        {
	            memcpy(&layer_info, &psDispcData->layer_info[disp][i], sizeof(disp_layer_info));
              if(layer_info.fb.format >= DISP_FORMAT_YUV444_I_AYUV) {
                  /* assume all yuv layer is video layer */
                  fcount_data.video_count ++;
                  bsp_disp_layer_get_info(disp, hdl, &layer_info_tmp);
                  if(layer_info.fb.addr[0] != layer_info_tmp.fb.addr[0]) {
                      fcount_data.video_strict_count ++;
                  }
              }
	          bsp_disp_layer_set_info(disp, hdl, &layer_info);
	          bsp_disp_layer_enable(disp, hdl);
	        }
	        else
	        {
	            bsp_disp_layer_disable(disp, hdl);
	        }
	    }
    }

	fcount_data.acquire_count++;
	fcount_data.frame_refresh_array[fcount_data.current_index] = jiffies;
	fcount_data.current_index++;
	if(fcount_data.current_index == 100)
		fcount_data.current_index = 0;

    for(disp = 0; disp < num_screens; disp++) {
			bsp_disp_shadow_protect(disp, false);
    }

    mutex_lock(&composer_priv.runtime_lock);

    composer_priv.cb_w_conut++;
    if(composer_priv.b_no_output == 1){
        imp_finish_cb(1);
    }else{

	    for(disp = 0; disp < num_screens; disp++) {
		    /* if specified disp have layer to be display, then need to wait for it's vsync */
		    composer_priv.reg_active[disp] = (psDispcData->layer_num[disp] == 0)?1:0;
	    }
    }
	mutex_unlock(&composer_priv.runtime_lock);

  return 0;
}

static int hwc_suspend(void)
{
	mutex_lock(&composer_priv.runtime_lock);
	composer_priv.b_no_output = 1;
	imp_finish_cb(1);
	mutex_unlock(&composer_priv.runtime_lock);
	__inf("%s\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	__inf("%s\n", __func__);
	return 0;
}

s32 composer_init(void)
{
	int  value;

	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	memset(&fcount_data, 0x0, sizeof(disp_fcount_data));
	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	mutex_init(&composer_priv.runtime_lock);

    composer_priv.Display_commit_work=create_freezable_workqueue("DisplayCommit");
    INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
	INIT_LIST_HEAD(&composer_priv.update_regs_list);
	composer_priv.timeline = sw_sync_timeline_create("sunxi-fb");
	composer_priv.timeline_max = 0;
	composer_priv.b_no_output = 0;
    composer_priv.cb_w_conut = 0;
	mutex_init(&composer_priv.update_regs_list_lock);
    mutex_init(&gcommit_mutek);
	spin_lock_init(&(composer_priv.update_reg_lock));
	disp_register_ioctl_func(DISP_CMD_HWC_COMMIT, hwc_commit_ioctl);    
	disp_register_ioctl_func(DISP_CMD_HWC_GET_DISP_READY, hwc_check_disp_ready);
	disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
	composer_priv.reg_active[0] = 1;
	composer_priv.reg_active[1] = 1;
	composer_priv.reg_active[2] = 1;

	if(OSAL_Script_FetchParser_Data("disp_init", "disp_composer_mode", &value, 1) == 0) {
		__inf("disp_init.disp_composer_mode = %d\n", value);
		composer_priv.disp_composer_mode = value;
	}

  return 0;
}
#endif

