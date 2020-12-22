#ifndef DEV_COMPOSER_C_C
#define DEV_COMPOSER_C_C

#if !defined(CONFIG_HOMLET_PLATFORM)

#if defined(CONFIG_ARCH_SUN8IW6)
#define DISP_SUPPORT_TRANSFORM
#endif
#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include "dev_disp.h"
#include <video/sunxi_display2.h>
#include <linux/sunxi_tr.h>
static struct   mutex	gcommit_mutek;
static struct   mutex	gwb_mutek;
#if !defined(ALIGN)
#define ALIGN(x,a)	(((x) + (a) - 1L) & ~((a) - 1L))
#endif
#define DBG_TIME_TYPE 3
#define DBG_TIME_SIZE 100
//#define DEBUG_WB
enum {
    /* flip source image horizontally (around the vertical axis) */
    HAL_TRANSFORM_FLIP_H    = 0x01,
    /* flip source image vertically (around the horizontal axis)*/
    HAL_TRANSFORM_FLIP_V    = 0x02,
    /* rotate source image 90 degrees clockwise */
    HAL_TRANSFORM_ROT_90    = 0x04,
    /* rotate source image 180 degrees */
    HAL_TRANSFORM_ROT_180   = 0x03,
    /* rotate source image 270 degrees clockwise */
    HAL_TRANSFORM_ROT_270   = 0x07,
    /* don't use. see system/window.h */
    HAL_TRANSFORM_RESERVED  = 0x08,
};

typedef struct
{
	unsigned long         time[DBG_TIME_TYPE][DBG_TIME_SIZE];
	unsigned int          time_index[DBG_TIME_TYPE];
	unsigned int          count[DBG_TIME_TYPE];
}composer_health_info;

typedef struct {
    int                 outaquirefencefd;
    int                 rotate;
    disp_capture_info   capturedata;
}WriteBack_t;
typedef struct
{
    int                 forceflip;
    int                 layer_num[2];
    disp_layer_config   layer_info[2][16];
    int                 firstdisplay;
    int*                aquireFenceFd;
    int                 aquireFenceCnt;
    int                 firstDispFenceCnt;
    int*                returnfenceFd;
    bool                needWB[2]; //[0] is HDMI, [1] is miracast
    unsigned int        ehancemode[2]; //0 is close,1 is whole,2 is half mode
    unsigned int        androidfrmnum;
    WriteBack_t         *WriteBackdata;
}setup_dispc_data_t;

typedef struct
{
    struct list_head    list;
    unsigned  int       glbframenumber;
    unsigned  int       wbframenumber;
    struct sync_fence   *outaquirefence;
    void                *tmpaddr;
    unsigned int        bpp;
    unsigned int        plan;
    unsigned char       pln[3];
    unsigned int        wbsize;
    unsigned int        totalsize;
    disp_capture_info   *pscapture;
    WriteBack_t         WriteBackdata;
    wait_queue_head_t	wq;
}WB_data_list_t;

typedef struct
{
    struct list_head    list;
    unsigned  int       framenumber;
    unsigned  int       androidfrmnum;
    WB_data_list_t      *WB_data;
    setup_dispc_data_t  hwc_data;
}dispc_data_list_t;

typedef struct
{
    struct list_head    list;
    void *              vm_addr;
    void*               p_addr;
    unsigned int        size;
    unsigned  int       androidfrmnum;
    bool                isphaddr;
    bool                update;
}dumplayer_t;

struct composer_private_data
{
	struct work_struct    post2_cb_work;
	u32	                  Cur_Write_Cnt;
	u32                   Cur_Disp_Cnt[2];
    u32                   last_wb_cnt[2];
	bool                  b_no_output;
    char                  display_active[2];
    bool                  countrotate[2];
	struct                mutex	runtime_lock;
	struct list_head        update_regs_list;

	unsigned int            timeline_max;
	struct mutex            update_regs_list_lock;
	spinlock_t              update_reg_lock;
	struct work_struct      commit_work;
    struct workqueue_struct *Display_commit_work;
    struct sw_sync_timeline *relseastimeline;

    struct work_struct      WB_work;
    struct workqueue_struct *Display_WB_work;
    struct sw_sync_timeline *writebacktimeline;
    struct list_head        WB_list;//current only display0
    unsigned int            WB_count;
    spinlock_t              WB_lock;
    bool                    WB_status[2];

    setup_dispc_data_t      *tmptransfer;
    disp_drv_info           *psg_disp_drv;

    struct list_head        dumplyr_list;
    spinlock_t              dumplyr_lock;
    unsigned int            listcnt;
    unsigned char           dumpCnt;
    unsigned char           display;
    unsigned char           layerNum;
    unsigned char           channelNum;
    bool                    pause;
    bool                    cancel;
    bool                    dumpstart;
    bool                    initrial;
    unsigned int            ehancemode[2];
	composer_health_info    health_info;
    int                     tr_fd;
};
static struct composer_private_data composer_priv;

extern int sunxi_tr_request(void);
extern int sunxi_tr_release(int hdl);
extern int sunxi_tr_commit(int hdl, tr_info *info);
extern int sunxi_tr_query(int hdl);


int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, unsigned int framenuber, disp_capture_info *psWBdata);

//type: 0:acquire, 1:release; 2:display
static s32 composer_get_frame_fps(u32 type)
{
	__u32 pre_time_index, cur_time_index;
	__u32 pre_time, cur_time;
	__u32 fps = 0xff;

	pre_time_index = composer_priv.health_info.time_index[type];
	cur_time_index = (pre_time_index == 0)? (DBG_TIME_SIZE -1):(pre_time_index-1);

	pre_time = composer_priv.health_info.time[type][pre_time_index];
	cur_time = composer_priv.health_info.time[type][cur_time_index];

	if(pre_time != cur_time) {
		fps = 1000 * 100 / (cur_time - pre_time);
	}

	return fps;
}

//type: 0:acquire, 1:release; 2:display
static void composer_frame_checkin(u32 type)
{
	u32 index = composer_priv.health_info.time_index[type];
	composer_priv.health_info.time[type][index] = jiffies;
	index ++;
	index = (index>=DBG_TIME_SIZE)?0:index;
	composer_priv.health_info.time_index[type] = index;
	composer_priv.health_info.count[type]++;
}

unsigned int composer_dump(char* buf)
{
	u32 fps0,fps1,fps2;
	u32 cnt0,cnt1,cnt2;

	fps0 = composer_get_frame_fps(0);
	fps1 = composer_get_frame_fps(1);
	fps2 = composer_get_frame_fps(2);
	cnt0 = composer_priv.health_info.count[0];
	cnt1 = composer_priv.health_info.count[1];
	cnt2 = composer_priv.health_info.count[2];

	return sprintf(buf, "acquire: %d, %d.%d fps\nrelease: %d, %d.%d fps\ndisplay: %d, %d.%d fps\n",
		cnt0, fps0/10, fps0%10, cnt1, fps1/10, fps1%10, cnt2, fps2/10, fps2%10);
}

int dev_composer_debug(setup_dispc_data_t *psDispcData, unsigned int framenuber)
{
    unsigned int  size = 0 ;
    void *kmaddr = NULL,*vm_addr = NULL,*sunxi_addr = NULL;
    disp_layer_config *dumlayer = NULL;
    dumplayer_t *dmplyr = NULL, *next = NULL, *tmplyr = NULL;
    bool    find = 0;
    if(composer_priv.cancel && composer_priv.display != 255 && composer_priv.channelNum != 255 && composer_priv.layerNum != 255)
    {
        dumlayer = &(psDispcData->layer_info[composer_priv.display][composer_priv.channelNum*4+composer_priv.layerNum]);
        dumlayer->enable =0;
    }
    if(composer_priv.dumpstart)
    {
        if(composer_priv.dumpCnt > 0 && composer_priv.display != 255 && composer_priv.channelNum != 255 && composer_priv.layerNum != 255 )
        {
            dumlayer = &(psDispcData->layer_info[composer_priv.display][composer_priv.channelNum*4+composer_priv.layerNum]);
            if(dumlayer != NULL && dumlayer->enable)
            {
                switch(dumlayer->info.fb.format)
                {
                    case DISP_FORMAT_ABGR_8888:
                    case DISP_FORMAT_ARGB_8888:
                    case DISP_FORMAT_XRGB_8888:
                    case DISP_FORMAT_XBGR_8888:
                        size = 4*dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height;
                    break;
                    case DISP_FORMAT_BGR_888:
                        size = 3*dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height;
                    break;
                    case DISP_FORMAT_RGB_565:
                        size = 2*dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height;
                    break;
                    case DISP_FORMAT_YUV420_P:
                        size = dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height
                            +dumlayer->info.fb.size[1].width * dumlayer->info.fb.size[1].height
                            +dumlayer->info.fb.size[2].width * dumlayer->info.fb.size[2].height;
                    break;
                    case DISP_FORMAT_YUV420_SP_VUVU:
                    case DISP_FORMAT_YUV420_SP_UVUV:
                        size = dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height
                            + 2*dumlayer->info.fb.size[1].width * dumlayer->info.fb.size[1].height;
                    break;
                    default:
                        printk("we got a err info..\n");
                }
                sunxi_addr = sunxi_map_kernel((unsigned int)dumlayer->info.fb.addr[0],size);
                if(sunxi_addr == NULL)
                {
                    printk("map mem err...\n");
                    goto err;
                }
                list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
                {
                    if(!dmplyr->update)
                    {
                        if(dmplyr->size == size)
                        {
                            find = 1;
                            break;
                        }else{
                            tmplyr = dmplyr;
                        }
                    }
                }
                if(find)
                {
                    memcpy(dmplyr->vm_addr, sunxi_addr, size);
                    sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                    dmplyr->update = 1;
                    dmplyr->androidfrmnum = framenuber;
                    composer_priv.dumpCnt--;
                    goto ok;
                }else if(tmplyr != NULL)
                {
                    dmplyr = tmplyr;
                     if(dmplyr->isphaddr)
                        {
                            sunxi_unmap_kernel(dmplyr->vm_addr,dmplyr->isphaddr,dmplyr->size);
                            sunxi_free_phys((unsigned int)dmplyr->p_addr,dmplyr->size);
                        }else{
                            kfree((void *)dmplyr->vm_addr);
                        }
                        dmplyr->vm_addr = NULL;
                        dmplyr->p_addr = NULL;
                        dmplyr->size = 0;
                }else{
                    dmplyr = kzalloc(sizeof(dumplayer_t),GFP_KERNEL);
                    if(dmplyr == NULL)
                    {
                        printk("kzalloc mem err...\n");
			sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                        goto err;
                    }
                }
                vm_addr = sunxi_buf_alloc(size, (unsigned int*)&kmaddr);
                dmplyr->isphaddr = 1;
                if(kmaddr == 0)
                {
                    vm_addr = kzalloc(size,GFP_KERNEL);
                    dmplyr->isphaddr = 0;
                    if(vm_addr == NULL)
                    {
			sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                        dmplyr->update = 0;
                        printk("kzalloc mem err...\n");
                        goto err;
                    }
                    dmplyr->vm_addr = vm_addr;
                }
                if(dmplyr->isphaddr)
                {
                    if(dmplyr->vm_addr == NULL)
                    {
			sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                        dmplyr->update = 0;
                        printk("kzalloc mem err...\n ");
                        goto err;
                    }
                    dmplyr->p_addr = kmaddr;
                }
                dmplyr->size = size;
                dmplyr->androidfrmnum = framenuber;
                memcpy(dmplyr->vm_addr, sunxi_addr, size);
		sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                dmplyr->update = 1;
                if(tmplyr == NULL)
                {
                    composer_priv.listcnt++;
                    list_add_tail(&dmplyr->list, &composer_priv.dumplyr_list);
                }
            }
            composer_priv.dumpCnt--;
        }
    }
ok:
    return 0;
err:
    return -1;
}
static int debug_write_file(dumplayer_t *dumplyr)
{
    char s[30];
    struct file *dumfile;
    mm_segment_t old_fs;
    int cnt;
    if(dumplyr->vm_addr != NULL && dumplyr->size != 0)
    {
        cnt = sprintf(s, "/mnt/sdcard/dumplayer%d",dumplyr->androidfrmnum);
        dumfile = filp_open(s, O_RDWR|O_CREAT, 0755);
        if(IS_ERR(dumfile))
        {
            printk("open %s err[%d]\n",s,(int)dumfile);
            return 0;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        if(dumplyr->vm_addr != NULL && dumplyr->size !=0)
        {
            dumfile->f_op->write(dumfile, dumplyr->vm_addr, dumplyr->size, &dumfile->f_pos);
        }
        set_fs(old_fs);
        filp_close(dumfile, NULL);
        dumfile = NULL;
        dumplyr->update = 0;
    }
   return 0 ;
}

static void hwc_commit_work(struct work_struct *work)
{
    dispc_data_list_t *data, *next;
    struct list_head saved_list;
    int err;
    int i,j,k,disp;
    unsigned char memset_val;
    struct sync_fence *AcquireFence = NULL;
    void *kmaddr = NULL,*vm_addr = NULL,*mem_off = NULL;
    unsigned int size = 0,totalsize = 0;
    disp_capture_info *psWBdata = NULL;
    disp_capture_info  *tmpWBdat = NULL;
    bool swp = 0,hasWB = 0;
    unsigned int lcd_h_scale,lcd_w_scale,lcd_w,lcd_h,memset_w_off,memset_h_off,memset_w,memset_h;

    mutex_lock(&(gcommit_mutek));
    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);
    mutex_unlock(&(composer_priv.update_regs_list_lock));

    list_for_each_entry_safe(data, next, &saved_list, list)
    {
        psWBdata = NULL;
        tmpWBdat = NULL;
        kmaddr =NULL;
        vm_addr = NULL;
        mem_off = NULL;
        swp = 0,
        size = 0;
        totalsize = 0;
        hasWB = 0;
        disp = 0;
        list_del(&data->list);
        if(data->hwc_data.forceflip)
        {
            printk("HWC give a forcflip frame\n");
            printk("androidfrmnum:%d  ker:%d  timeline:%d\n",data->hwc_data.androidfrmnum,data->framenumber,composer_priv.relseastimeline->value);
            composer_priv.Cur_Write_Cnt = data->framenumber;
            sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
            if(data->WB_data != NULL)
            {
                kfree(data->WB_data);
            }
            goto free;
        }
	    for(i = 0; i < data->hwc_data.aquireFenceCnt; i++)
	    {
            if(i >= data->hwc_data.firstDispFenceCnt && data->hwc_data.firstDispFenceCnt != 0)
            {
                disp = 1;
            }
            if(composer_priv.display_active[disp] != 0)
            {
                AcquireFence =(struct sync_fence *) data->hwc_data.aquireFenceFd[i];
                if(AcquireFence != NULL)
                {
                    err = sync_fence_wait(AcquireFence,3000);
                    sync_fence_put(AcquireFence);
                    if (err < 0)
	                {
                        printk("synce_fence_wait timeout disp[%d]fence:%p\n",disp,AcquireFence);
                        printk("androidfrmnum:%d  ker:%d  timeline:%d\n",data->hwc_data.androidfrmnum,data->framenumber,composer_priv.relseastimeline->value);
                        composer_priv.Cur_Write_Cnt = data->framenumber;
                        sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
                        if(data->WB_data != NULL)
                        {
                            kfree(data->WB_data);
                        }
					    goto free;
	                }
                }
            }
	    }
        if(data->WB_data != NULL)
        {
#if defined(DISP_SUPPORT_TRANSFORM)
            if(composer_priv.tr_fd  == 0)
            {
                composer_priv.tr_fd = sunxi_tr_request();
                if(composer_priv.tr_fd == (int)NULL)
                {
                    printk("get tr_fd failed\n");
                }
            }
#endif
            hasWB = 1;
            psWBdata = &data->WB_data->WriteBackdata.capturedata;
            data->WB_data->plan = 1;
            switch(psWBdata->out_frame.format)
            {
                case DISP_FORMAT_ABGR_8888:
                case DISP_FORMAT_ARGB_8888:
                case DISP_FORMAT_XRGB_8888:
                case DISP_FORMAT_XBGR_8888:
                    data->WB_data->bpp = 32;
                    data->WB_data->pln[0] = 32;
                break;
                case DISP_FORMAT_BGR_888:
                    data->WB_data->bpp = 24;
                    data->WB_data->pln[0] = 24;
                break;
                case DISP_FORMAT_RGB_565:
                    data->WB_data->bpp = 16;
                    data->WB_data->pln[0] = 16;
                break;
                case DISP_FORMAT_YUV420_P:
                case DISP_FORMAT_YUV420_SP_VUVU:
                case DISP_FORMAT_YUV420_SP_UVUV:
                    data->WB_data->bpp = 12;
                break;
                default:
                    hasWB = 0;
                    goto fix_wb;
                    printk("we got a err format...\n");
            }
            size = psWBdata->out_frame.size[0].width * psWBdata->out_frame.size[0].height * data->WB_data->bpp / 8;
            totalsize = psWBdata->out_frame.size[0].width * psWBdata->out_frame.size[0].height * data->WB_data->bpp / 8;
            if((data->WB_data->outaquirefence && data->WB_data->outaquirefence->status == 0 ) || data->WB_data->WriteBackdata.rotate)
            {

                tmpWBdat = kzalloc(sizeof(disp_capture_info),GFP_KERNEL);
                memcpy(&tmpWBdat->window,&psWBdata->window,sizeof(disp_rect));
                tmpWBdat->out_frame.format = psWBdata->out_frame.format;

                lcd_h_scale = psWBdata->out_frame.crop.height;
                lcd_w_scale = psWBdata->out_frame.crop.width;
                lcd_w = psWBdata->out_frame.size[0].width;
                lcd_h = psWBdata->out_frame.size[0].height;
                if(data->WB_data->WriteBackdata.rotate & HAL_TRANSFORM_ROT_90)
                {
                    lcd_h_scale = psWBdata->out_frame.crop.width;
                    lcd_w_scale = psWBdata->out_frame.crop.height;
                    lcd_w = psWBdata->out_frame.size[0].height;
                    lcd_h = psWBdata->out_frame.size[0].width;
                }
                vm_addr = sunxi_buf_alloc(size, (unsigned int*)&kmaddr);
                if(kmaddr != 0)
                {
                    tmpWBdat->out_frame.crop.x = (lcd_w - lcd_w_scale) / 2;
                    tmpWBdat->out_frame.crop.y = (lcd_h- lcd_h_scale) / 2;
                    tmpWBdat->out_frame.crop.width = lcd_w_scale;
                    tmpWBdat->out_frame.crop.height = lcd_h_scale;
                    tmpWBdat->out_frame.addr[0] = (unsigned int)kmaddr;
                    tmpWBdat->out_frame.size[0].width = lcd_w;
                    tmpWBdat->out_frame.size[0].height = lcd_h;

                    switch(psWBdata->out_frame.format)
                    {
                        case DISP_FORMAT_YUV420_P:
                            data->WB_data->plan = 3;
                            data->WB_data->pln[0] = 8;
                            data->WB_data->pln[1] = 8;
                            data->WB_data->pln[2] = 8;
                            tmpWBdat->out_frame.size[1].width = tmpWBdat->out_frame.size[0].width  / 2;
                            tmpWBdat->out_frame.size[2].height = tmpWBdat->out_frame.size[0].height / 2;
                            tmpWBdat->out_frame.size[1].width = tmpWBdat->out_frame.size[0].width  / 2;
                            tmpWBdat->out_frame.size[2].height = tmpWBdat->out_frame.size[0].height / 2;
                            tmpWBdat->out_frame.addr[2] = (unsigned int)(kmaddr) + tmpWBdat->out_frame.size[0].width  * tmpWBdat->out_frame.size[0].height;
                            tmpWBdat->out_frame.addr[1] = tmpWBdat->out_frame.addr[2] + tmpWBdat->out_frame.size[0].height * tmpWBdat->out_frame.size[0].width  / 4;
                        break;
                        case DISP_FORMAT_YUV420_SP_VUVU:
                        case DISP_FORMAT_YUV420_SP_UVUV:
                            data->WB_data->plan = 2;
                            data->WB_data->pln[0] = 8;
                            data->WB_data->pln[1] = 16;
                            tmpWBdat->out_frame.size[1].width = tmpWBdat->out_frame.size[0].width / 2;
                            tmpWBdat->out_frame.addr[1] = (unsigned int)kmaddr + tmpWBdat->out_frame.size[0].height * tmpWBdat->out_frame.size[0].width;
                        break;
                        default:
                        break;
                    }
                    if(vm_addr == NULL)
                    {
                            printk("map mem err...\n");
                    }else{
                        if(tmpWBdat->out_frame.crop.x)
                        {
                            for(k = 0; k < data->WB_data->plan; k++)
                            {
                                if(k)
                                {
                                   mem_off = vm_addr + tmpWBdat->out_frame.size[k-1].width * tmpWBdat->out_frame.size[k-1].height * data->WB_data->pln[k-1]/8;
                                }else{
                                   mem_off = vm_addr;
                                }
                                memset_w_off = tmpWBdat->out_frame.crop.x;
                                memset_w = tmpWBdat->out_frame.crop.width;
                                if(data->WB_data->plan > 1)
                                {
                                    if(k)
                                    {
                                        memset_val = 16;
                                        memset_w_off = tmpWBdat->out_frame.crop.x / 2;
                                        memset_w = tmpWBdat->out_frame.crop.width / 2;
                                    }else{
                                        memset_val = 128;
                                    }
                                }else{
                                    memset_val = 0;
                                }
                                j = 0;
                                while(j < tmpWBdat->out_frame.size[k].height)
                                {
                                    memset(mem_off + tmpWBdat->out_frame.size[k].width * j *data->WB_data->pln[k] / 8 , memset_val, memset_w_off * data->WB_data->pln[k] / 8);
                                    memset(mem_off + (memset_w_off + memset_w + tmpWBdat->out_frame.size[k].width * j) * data->WB_data->pln[k] / 8, memset_val,(tmpWBdat->out_frame.size[k].width - memset_w_off- memset_w ) * data->WB_data->pln[k] / 8);
                                    j++;
                                }
                            }
                        }else{
                            for(k = 0; k < data->WB_data->plan; k++)
                            {
                                if(k)
                                {
                                   mem_off = vm_addr + tmpWBdat->out_frame.size[k-1].width * tmpWBdat->out_frame.size[k-1].height * data->WB_data->pln[k-1]/8;
                                }else{
                                   mem_off = vm_addr;
                                }
                                memset_w_off = tmpWBdat->out_frame.crop.x;
                                memset_w = tmpWBdat->out_frame.crop.width;
                                memset_h = tmpWBdat->out_frame.crop.height;
                                memset_h_off = tmpWBdat->out_frame.crop.y;
                                if(data->WB_data->plan > 1)
                                {
                                    if(k)
                                    {
                                        memset_val = 16;
                                        memset_w_off = tmpWBdat->out_frame.crop.x / 2;
                                        memset_w = tmpWBdat->out_frame.crop.width / 2;
                                        memset_h = tmpWBdat->out_frame.crop.height / 2;
                                        memset_h_off = tmpWBdat->out_frame.crop.y / 2;
                                    }else{
                                        memset_val = 128;
                                    }
                                }else{
                                    memset_val = 0;
                                }
                                memset(mem_off, memset_val, memset_h_off * tmpWBdat->out_frame.size[k].width * data->WB_data->pln[k] / 8);
                                memset(mem_off +(memset_h_off + memset_h)* tmpWBdat->out_frame.size[k].width  * data->WB_data->pln[k] / 8, memset_val, (tmpWBdat->out_frame.size[k].height - memset_h - memset_h_off ) * tmpWBdat->out_frame.size[k].width * data->WB_data->pln[k]/8);
                            }
                        }

                        sunxi_unmap_kernel(vm_addr, (unsigned int)kmaddr,size);
                    }
                }else{
                    hasWB = 0;
                }
                swp = 1;
            }else{
                vm_addr = sunxi_map_kernel(data->WB_data->WriteBackdata.capturedata.out_frame.addr[0],size);
                if(vm_addr == NULL)
                {
                    printk("map mem err...\n");
                }else{
                    for(k = 0; k < data->WB_data->plan; k++)
                    {
                        if(k)
                        {
                            mem_off = vm_addr + data->WB_data->WriteBackdata.capturedata.out_frame.size[k-1].width * data->WB_data->WriteBackdata.capturedata.out_frame.size[k-1].height * data->WB_data->pln[k-1]/8;
                        }else{
                           mem_off = vm_addr;
                        }
                        memset_w_off = data->WB_data->WriteBackdata.capturedata.out_frame.crop.x;
                        memset_w = data->WB_data->WriteBackdata.capturedata.out_frame.crop.width;
                        if(data->WB_data->plan > 1)
                        {
                            if(k)
                            {
                                memset_val = 16;
                                memset_w_off = data->WB_data->WriteBackdata.capturedata.out_frame.crop.x / 2;
                                memset_w = data->WB_data->WriteBackdata.capturedata.out_frame.crop.width / 2;
                            }else{
                                memset_val = 128;
                            }
                         }else{
                            memset_val = 0;
                         }
                         j = 0;
                         while(j < data->WB_data->WriteBackdata.capturedata.out_frame.size[k].height)
                         {
                            memset(mem_off + data->WB_data->WriteBackdata.capturedata.out_frame.size[k].width * data->WB_data->pln[k] * j / 8 , memset_val, memset_w_off * data->WB_data->pln[k] / 8);
                            memset(mem_off + (memset_w_off + memset_w + data->WB_data->WriteBackdata.capturedata.out_frame.size[k].width * j) * data->WB_data->pln[k] / 8, memset_val,(data->WB_data->WriteBackdata.capturedata.out_frame.size[k].width - memset_w_off- memset_w) * data->WB_data->pln[k] / 8);
                            j++;
                        }
                    }
                    sunxi_unmap_kernel(vm_addr,data->WB_data->WriteBackdata.capturedata.out_frame.addr[0],size);
                }
            }
fix_wb:
            data->WB_data->wbsize = size;
            data->WB_data->totalsize = totalsize;
            data->WB_data->pscapture = tmpWBdat;
            data->WB_data->tmpaddr = kmaddr;
            spin_lock(&composer_priv.WB_lock);
            list_add_tail(&(data->WB_data->list), &composer_priv.WB_list);
            spin_unlock(&composer_priv.WB_lock);
#if defined(DEBUG_WB)
            printk("data:%p\nLCD[%d,%d,%d,%d] ADDR[0x%llx,0x%llx,0x%llx] Size[[%d,%d][%d,%d][%d,%d]] TR:%02x"
                    "\nVir[%d,%d,%d,%d] ADDR[0x%llx,0x%llx,0x%llx] Size[[%d,%d][%d,%d][%d,%d]] %p\n\n"
                ,data->WB_data
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.crop.x:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.crop.y:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.crop.width:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.crop.height:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.addr[0]:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.addr[1]:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.addr[2]:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.size[0].width:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.size[0].height:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.size[1].width:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.size[1].height:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.size[2].width:0
                ,tmpWBdat != NULL ? data->WB_data->pscapture->out_frame.size[2].height:0
                ,tmpWBdat != NULL ? data->WB_data->WriteBackdata.rotate:0
                ,psWBdata->out_frame.crop.x
                ,psWBdata->out_frame.crop.y
                ,psWBdata->out_frame.crop.width
                ,psWBdata->out_frame.crop.height
                ,psWBdata->out_frame.addr[0]
                ,psWBdata->out_frame.addr[1]
                ,psWBdata->out_frame.addr[2]
                ,psWBdata->out_frame.size[0].width
                ,psWBdata->out_frame.size[0].height
                ,psWBdata->out_frame.size[1].width
                ,psWBdata->out_frame.size[1].height
                ,psWBdata->out_frame.size[2].width
                ,psWBdata->out_frame.size[2].height
                ,kmaddr);
#endif
        }else{
#if defined(DISP_SUPPORT_TRANSFORM)
            if(composer_priv.tr_fd  != 0)
            {
                sunxi_tr_release(composer_priv.tr_fd);
                composer_priv.tr_fd = 0;
            }
#endif
        }
        dev_composer_debug(&data->hwc_data, data->hwc_data.androidfrmnum);
        if(composer_priv.pause == 0)
        {
            dispc_gralloc_queue(&data->hwc_data, data->framenumber,hasWB ? (swp ? tmpWBdat : psWBdata ):NULL);
        }
free:
        if(data->hwc_data.aquireFenceFd !=NULL)
        {
            kfree(data->hwc_data.aquireFenceFd);
        }
        kfree(data);
    }
	mutex_unlock(&(gcommit_mutek));
}

static int hwc_commit(setup_dispc_data_t *disp_data)
{
	dispc_data_list_t *disp_data_list;
	struct sync_fence *fence;
	struct sync_pt *pt;
	int fd[2] = {-1,-1};
    int cout = 0, coutoffence = 0,forwhile = 0,cnt = 0;
    bool samefence = 0;
    int *fencefd = NULL;
    WB_data_list_t   *WB_data = NULL;
    if(disp_data->aquireFenceCnt > 0)
    {
        fencefd = kzalloc(( disp_data->aquireFenceCnt * sizeof(int)),GFP_KERNEL);
        if(copy_from_user( fencefd, (void __user *)disp_data->aquireFenceFd, disp_data->aquireFenceCnt * sizeof(int)))
        {
                printk("copy_from_user fail\n");
                goto err;
        }

        cnt = disp_data->firstDispFenceCnt;
        for(cout = 0; cout < disp_data->aquireFenceCnt; cout++)
        {
            fence = sync_fence_fdget(fencefd[cout]);
            if(!fence)
            {
                printk("sync_fence_fdget failed,fd[%d]:%d\n",cout, fencefd[cout]);
                if(cout < cnt)
                {
                    disp_data->firstDispFenceCnt--;
                }
                continue;
            }
            forwhile = coutoffence;
            while(forwhile)
            {
                if(fence == (struct sync_fence *)fencefd[forwhile])
                {
                    samefence = 1;
                    if(cout < cnt)
                    {
                        disp_data->firstDispFenceCnt--;
                    }
                    break;
                }
                forwhile--;
            }
            if(samefence)
            {
                samefence = 0;
                continue;
            }
            fencefd[coutoffence] = (int)fence;
            coutoffence++;
        }
    }
    disp_data->aquireFenceFd = fencefd;
    disp_data->aquireFenceCnt = coutoffence;
    if(disp_data->needWB[0] || disp_data->needWB[1] )
    {
        WB_data = kzalloc(sizeof(WB_data_list_t),GFP_KERNEL);
        if(copy_from_user(&WB_data->WriteBackdata, (void __user *)disp_data->WriteBackdata, sizeof(WriteBack_t)))
        {
            printk("copy_from_user WB_data fail\n");
            goto err;
		}
        WB_data->outaquirefence = sync_fence_fdget(WB_data->WriteBackdata.outaquirefencefd);
        init_waitqueue_head(&WB_data->wq);
    }

    if(1)
    {
	    if(disp_data->layer_num[0]+disp_data->layer_num[1] > 0 || disp_data->forceflip)
	    {
            forwhile = DISP_NUMS_SCREEN;
            while(forwhile-- && !disp_data->forceflip)
            {
                if(forwhile != disp_data->firstdisplay)
                {
                    if(composer_priv.display_active[forwhile] == 2 && !disp_data->layer_num[forwhile])
                    {
                        composer_priv.display_active[forwhile] = 0;
                    }
                    if(composer_priv.display_active[forwhile] == 0 && disp_data->layer_num[forwhile])
                    {
                        composer_priv.display_active[forwhile] = 1;
                    }
                }else{
                    if(composer_priv.initrial == 0)
                    {
                        composer_priv.display_active[forwhile] = 1;
                        composer_priv.initrial =1;
                    }
                }
            }
            fd[0] = get_unused_fd();
            if (fd < 0)
            {
                printk("get unused fd faild\n");
                goto err;
            }
            composer_priv.timeline_max++;
            pt = sw_sync_pt_create(composer_priv.relseastimeline, composer_priv.timeline_max);
            fence = sync_fence_create("sunxi_display", pt);
            sync_fence_install(fence, fd[0]);
            if(disp_data->needWB[0] || disp_data->needWB[1])
            {
                fd[1] = get_unused_fd();
                composer_priv.WB_count++;
                pt = sw_sync_pt_create(composer_priv.writebacktimeline, composer_priv.WB_count);
                fence = sync_fence_create("sunxi_WB", pt);
                sync_fence_install(fence, fd[1]);
                WB_data->wbframenumber = composer_priv.WB_count;
                WB_data->glbframenumber = composer_priv.timeline_max;
            }
            disp_data_list = kzalloc(sizeof(dispc_data_list_t), GFP_KERNEL);
            memcpy(&disp_data_list->hwc_data, disp_data, sizeof(setup_dispc_data_t));
            disp_data_list->framenumber = composer_priv.timeline_max;
            disp_data_list->WB_data = WB_data;
            mutex_lock(&(composer_priv.update_regs_list_lock));
            list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
            mutex_unlock(&(composer_priv.update_regs_list_lock));
            if(!composer_priv.pause)
            {
                queue_work(composer_priv.Display_commit_work, &composer_priv.commit_work);
            }
	    }else{
	        printk("No layer from android set\n");
	        goto err;
        }
    }else{
        flush_workqueue(composer_priv.Display_commit_work);
        goto err;
    }
    if(copy_to_user((void __user *)disp_data->returnfenceFd, fd, sizeof(int)*2))
    {
	    printk("copy_to_user fail\n");
        goto err;
	}
	return 0;
err:
    if(WB_data != NULL)
    {
       kfree(WB_data);
    }
    kfree(fencefd);
    return -EFAULT;
}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = -1;
	if(DISP_HWC_COMMIT == cmd)
    {
        unsigned long *ubuffer;
        ubuffer = (unsigned long *)arg;
        memset(composer_priv.tmptransfer, 0, sizeof(setup_dispc_data_t));
        if(copy_from_user(composer_priv.tmptransfer, (void __user *)ubuffer[1], sizeof(setup_dispc_data_t)))
        {
            printk("copy_from_user fail\n");
            return  -EFAULT;
		}
        ret = hwc_commit(composer_priv.tmptransfer);
	}
	return ret;
}

static void disp_composer_proc(u32 sel)
{
    if(sel<2)
    {
        if(composer_priv.Cur_Write_Cnt < composer_priv.Cur_Disp_Cnt[sel])
        {
            composer_priv.countrotate[sel] = 1;
        }
        composer_priv.last_wb_cnt[sel]= composer_priv.Cur_Disp_Cnt[sel];
        composer_priv.Cur_Disp_Cnt[sel] = composer_priv.Cur_Write_Cnt;
    }
	schedule_work(&composer_priv.post2_cb_work);
    if(!list_empty(&composer_priv.WB_list))
    {
        queue_work(composer_priv.Display_WB_work, &composer_priv.WB_work);
    }
	return ;
}

static void imp_finish_cb(bool force_all)
{
    u32 little = 1;
	u32 flag = 0;
    bool rotate = 0;

    if(composer_priv.pause)
    {
        return;
    }
    if(composer_priv.display_active[0] == 2)
    {
        little = composer_priv.Cur_Disp_Cnt[0];
    }
    if( (composer_priv.display_active[0] == 2) && composer_priv.countrotate[0]
        &&(composer_priv.display_active[1] == 2)&& composer_priv.countrotate[1])
    {
        composer_priv.countrotate[0] = 0;
        composer_priv.countrotate[1] = 0;
    }
    if(composer_priv.display_active[1] == 2)
    {
        if( composer_priv.display_active[0] == 2)
        {
            if(composer_priv.countrotate[0] != composer_priv.countrotate[1])
            {
                if(composer_priv.countrotate[0] && (composer_priv.display_active[0]== 2))
                {
                    little = composer_priv.Cur_Disp_Cnt[1];
                }else{
                    little = composer_priv.Cur_Disp_Cnt[0];
                }
            }else{
                if(composer_priv.Cur_Disp_Cnt[1] > composer_priv.Cur_Disp_Cnt[0])
                {
                    little = composer_priv.Cur_Disp_Cnt[0];
                }else{
                    little = composer_priv.Cur_Disp_Cnt[1];
                }
            }
      }else{
            little = composer_priv.Cur_Disp_Cnt[1];
      }
    }
    while(composer_priv.relseastimeline->value != composer_priv.Cur_Write_Cnt)
    {
        if(composer_priv.relseastimeline->value > composer_priv.Cur_Write_Cnt)
        {
            rotate = 1;
        }
        if(rotate && composer_priv.relseastimeline->value > little - 1)
        {
            rotate = 1;
        }else{
            rotate = 0;
        }
        if( !force_all
            &&(rotate == 0 ? composer_priv.relseastimeline->value >= little - 1
                           : composer_priv.relseastimeline->value == little - 1 ))
        {
            break;
        }
        sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
        composer_frame_checkin(1);
        flag = 1;
    }
#if defined(DEBUG_WB)
    printk("composer_priv.Cur_Disp_Cnt[0][%d][%d] [1][%d][%d]  timeline[%d] little[%d]\n "
        ,composer_priv.display_active[0]
        ,composer_priv.Cur_Disp_Cnt[0]
        ,composer_priv.Cur_Disp_Cnt[1]
        ,composer_priv.display_active[1]
        ,composer_priv.relseastimeline->value
        ,little);
#endif
    if(flag)
		composer_frame_checkin(2);
}

static void write_back(struct work_struct *work)
{
    WB_data_list_t *data, *next;
    void*vm_addr = NULL,*sunxi_vaddr = NULL;
    int err= -1;
    int cnt =0;
#if defined(DISP_SUPPORT_TRANSFORM)
    tr_info tr;
    disp_s_frame * pslcdptcture = NULL;
#endif
    mutex_lock(&(gwb_mutek));
    list_for_each_entry_safe(data, next, &composer_priv.WB_list, list)
    {
        err= -1;
        if(composer_priv.last_wb_cnt[0] >= data->glbframenumber )
        {
            if(data->tmpaddr != NULL)
            {
                if(data->WriteBackdata.rotate)
                {
#if defined(DISP_SUPPORT_TRANSFORM)
                    if(data->outaquirefence != NULL && data->outaquirefence->status == 0 )
                    {
                        err = sync_fence_wait(data->outaquirefence,1000);
                        sync_fence_put(data->outaquirefence);
                        if (err < 0)
	                    {
	                        printk("synce_fence_wait timeout outaquirefence:%p\n",data->outaquirefence);
                            goto err;
	                    }
                    }
                    pslcdptcture = &data->pscapture->out_frame;
                    memset(&tr,0,sizeof(tr_info));
                    switch(data->WriteBackdata.rotate)
                    {
                        case HAL_TRANSFORM_ROT_90:
                            tr.mode = TR_ROT_90;
                        break;
                        case HAL_TRANSFORM_ROT_180:
                            tr.mode = TR_ROT_180;
                        break;
                        case HAL_TRANSFORM_ROT_270:
                            tr.mode = TR_ROT_270;
                        break;
                        default:
                        printk("a err TR\n ");
                    }
                    tr.src_frame.fmt = pslcdptcture->format;
                    tr.src_frame.laddr[0] = pslcdptcture->addr[0];
                    tr.src_frame.laddr[1] = pslcdptcture->addr[1];
                    tr.src_frame.laddr[2] = pslcdptcture->addr[2];
                    tr.src_frame.pitch[0] = pslcdptcture->size[0].width;
                    tr.src_frame.pitch[1] = pslcdptcture->size[1].width;
                    tr.src_frame.pitch[2] = pslcdptcture->size[2].width;
                    tr.src_frame.height[0] = pslcdptcture->size[0].height;
                    tr.src_frame.height[1] = pslcdptcture->size[1].height;
                    tr.src_frame.height[2] = pslcdptcture->size[2].height;
                    tr.src_rect.x = 0;
                    tr.src_rect.y = 0;
                    tr.src_rect.w = pslcdptcture->size[0].width;
                    tr.src_rect.h = pslcdptcture->size[0].height;
                    tr.dst_frame.fmt = data->WriteBackdata.capturedata.out_frame.format;
                    tr.dst_frame.laddr[0] = data->WriteBackdata.capturedata.out_frame.addr[0];
                    tr.dst_frame.laddr[1] = data->WriteBackdata.capturedata.out_frame.addr[1];
                    tr.dst_frame.laddr[2] = data->WriteBackdata.capturedata.out_frame.addr[2];
                    tr.dst_frame.pitch[0] = data->WriteBackdata.capturedata.out_frame.size[0].width;
                    tr.dst_frame.pitch[1] = data->WriteBackdata.capturedata.out_frame.size[1].width;
                    tr.dst_frame.pitch[2] = data->WriteBackdata.capturedata.out_frame.size[2].width;
                    tr.dst_frame.height[0] = data->WriteBackdata.capturedata.out_frame.size[0].height;
                    tr.dst_frame.height[1] = data->WriteBackdata.capturedata.out_frame.size[1].height;
                    tr.dst_frame.height[2] = data->WriteBackdata.capturedata.out_frame.size[2].height;
                    tr.dst_rect.x = 0;
                    tr.dst_rect.y = 0;
                    tr.dst_rect.w = data->WriteBackdata.capturedata.out_frame.size[0].width;
                    tr.dst_rect.h = data->WriteBackdata.capturedata.out_frame.size[0].height;
                    if(composer_priv.tr_fd == 0)
                    {
                         printk("tr_fd is [0] err...\n");
                         goto err;
                    }
                    tr.fd = composer_priv.tr_fd;
                    sunxi_tr_commit(tr.fd,&tr);
                    msleep(3);
                    //printk("#####sunxi_tr_query(%d)  cnt[%d]\n",sunxi_tr_query(tr.fd),cnt);
                    //must check with tyl to change the way of wait for complete
                    if(sunxi_tr_query(tr.fd) == 1)
                    msleep(1);
#if defined(DEBUG_WB)
            printk("\nTR:%02x:\nF:%x Saddr[0x%x,0x%x,0x%x] pitch[%d,%d,%d] height[%d,%d,%d] crop[%d,%d,%d,%d]"
                           "\nF:%x Daddr[0x%x,0x%x,0x%x] pitch[%d,%d,%d] height[%d,%d,%d] crop[%d,%d,%d,%d]\n"
                ,tr.mode
                ,tr.src_frame.fmt
                ,tr.src_frame.laddr[0]
                ,tr.src_frame.laddr[1]
                ,tr.src_frame.laddr[2]
                ,tr.src_frame.pitch[0]
                ,tr.src_frame.pitch[1]
                ,tr.src_frame.pitch[2]
                ,tr.src_frame.height[0]
                ,tr.src_frame.height[1]
                ,tr.src_frame.height[2]
                ,tr.src_rect.x
                ,tr.src_rect.y = 0
                ,tr.src_rect.w
                ,tr.src_rect.h
                ,tr.dst_frame.fmt
                ,tr.dst_frame.laddr[0]
                ,tr.dst_frame.laddr[1]
                ,tr.dst_frame.laddr[2]
                ,tr.dst_frame.pitch[0]
                ,tr.dst_frame.pitch[1]
                ,tr.dst_frame.pitch[2]
                ,tr.dst_frame.height[0]
                ,tr.dst_frame.height[1]
                ,tr.dst_frame.height[2]
                ,tr.dst_rect.x
                ,tr.dst_rect.y
                ,tr.dst_rect.w
                ,tr.dst_rect.h
                );
#endif

#endif//defined(DISP_SUPPORT_TRANSFORM)
                }else{
                    if(data->outaquirefence != NULL)
                    {
                        err = sync_fence_wait(data->outaquirefence,1000);
                        sync_fence_put(data->outaquirefence);
                        if (err < 0)
	                    {
	                        printk("synce_fence_wait timeout outaquirefence:%p\n",data->outaquirefence);
                            goto err;
	                    }
                        sunxi_vaddr = sunxi_map_kernel((unsigned int)data->WriteBackdata.capturedata.out_frame.addr[0],data->totalsize);
                        if(sunxi_vaddr == NULL)
                        {
                            printk("map mem err...\n");
                            goto err;
                        }
                        vm_addr = sunxi_map_kernel((unsigned int)data->tmpaddr,data->wbsize);
                        if(vm_addr == NULL)
                        {
                            printk("map mem err...\n");
                            sunxi_unmap_kernel(sunxi_vaddr,(unsigned int)data->WriteBackdata.capturedata.out_frame.addr[0],data->totalsize);
                            goto err;
                        }
                        memcpy(sunxi_vaddr, vm_addr, data->wbsize);
			sunxi_unmap_kernel(vm_addr, (unsigned int)data->tmpaddr,data->wbsize);
			sunxi_unmap_kernel(sunxi_vaddr,(unsigned int)data->WriteBackdata.capturedata.out_frame.addr[0],data->totalsize);		    
                    }
                }
            }
#if defined(DEBUG_WB)
            printk("pt:%d  wb:%d ADDR:[%llx][%llx][%llx]  TR:%02x paddr:%p,size:%d(%d)\n\n"
            ,composer_priv.writebacktimeline->value
            ,data->wbframenumber
            ,data->WriteBackdata.capturedata.out_frame.addr[0]
            ,data->WriteBackdata.capturedata.out_frame.addr[1]
            ,data->WriteBackdata.capturedata.out_frame.addr[2]
            ,data->WriteBackdata.rotate
            ,data->tmpaddr
            ,data->wbsize
            ,data->totalsize);
#endif
err:
            spin_lock(&composer_priv.WB_lock);
            list_del(&data->list);
            spin_unlock(&composer_priv.WB_lock);
            if(data->tmpaddr != NULL)
            {
                sunxi_free_phys((unsigned int)data->tmpaddr,data->wbsize);
            }
            if(data->pscapture != NULL)
            {
                kfree(data->pscapture);
            }
            while(composer_priv.writebacktimeline->value != data->wbframenumber)
            {
                sw_sync_timeline_inc(composer_priv.writebacktimeline, 1);
            }
            kfree(data);
        }else{
            break;
        }
        cnt++;
    }
    mutex_unlock(&(gwb_mutek));
#if defined(DEBUG_WB)
        printk("WB_count :[%d,%d]  D[%d,%d]\n",composer_priv.WB_count,composer_priv.writebacktimeline->value,composer_priv.timeline_max,composer_priv.relseastimeline->value);
#endif
    return ;
}


static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.runtime_lock);
    imp_finish_cb(composer_priv.b_no_output);
	mutex_unlock(&composer_priv.runtime_lock);
}
extern s32  bsp_disp_shadow_protect(u32 disp, bool protect);
int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, unsigned int framenuber, disp_capture_info *psWBdata)
{
    int disp;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_enhance *psenhance = NULL;
    struct disp_capture *psWriteBack = NULL;
    disp_layer_config *psconfig;
    disp = 0;

    while( disp < DISP_NUMS_SCREEN )
    {
        if(!composer_priv.display_active[disp])
        {
            disp++;
            continue;
        }
        psmgr = psg_disp_drv->mgr[disp];
        psenhance = psmgr->enhance;
        psWriteBack = psmgr->cptr;
        if( psmgr != NULL  )
        {
            bsp_disp_shadow_protect(disp,true);
            if(psDispcData->ehancemode[disp] )
            {
                if(psDispcData->ehancemode[disp] != composer_priv.ehancemode[disp])
                {
                    switch (psDispcData->ehancemode[disp])
                    {
                        case 1:
                            psenhance->demo_disable(psenhance);
                            psenhance->enable(psenhance);
                        break;
                        case 2:
                            psenhance->enable(psenhance);
                            psenhance->demo_enable(psenhance);
                        break;
                        default:
                            psenhance->disable(psenhance);
                            printk("translat a err info\n");
                    }
                }
                composer_priv.ehancemode[disp] = psDispcData->ehancemode[disp];
            }else{
                if(composer_priv.ehancemode[disp])
                {
                    psenhance->disable(psenhance);
                    composer_priv.ehancemode[disp] = 0;
                }
            }
            if(disp == 0)
            {
                if(psWBdata != NULL)
                {
                    if(composer_priv.WB_status[0] != 1)
                    {
                        psWriteBack->start(psWriteBack);
                        composer_priv.WB_status[0] = 1;
                    }
                    psWriteBack->commmit(psWriteBack,psWBdata);
                }else{
                    if(list_empty(&composer_priv.WB_list))
                    {
                        if(composer_priv.WB_status[0] == 1)
                        {
                            psWriteBack->stop(psWriteBack);
                            composer_priv.WB_status[0] = 0;
                        }
                    }
                }
            }
            psconfig = &psDispcData->layer_info[disp][0];
            psmgr->set_layer_config(psmgr, psconfig, disp?8:16);
            bsp_disp_shadow_protect(disp,false);
            if(composer_priv.display_active[disp] == 1)
            {
                // no need lock,THK lot
                composer_priv.display_active[disp] = 2;
                composer_priv.Cur_Disp_Cnt[disp] = framenuber;
            }
        }
        disp++;
    }
    composer_priv.Cur_Write_Cnt = framenuber;
    composer_frame_checkin(0);
    if(composer_priv.b_no_output)
    {
        mutex_lock(&composer_priv.runtime_lock);
	    imp_finish_cb(1);
	    mutex_unlock(&composer_priv.runtime_lock);
    }
  return 0;
}

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	mutex_lock(&composer_priv.runtime_lock);
	imp_finish_cb(1);
	mutex_unlock(&composer_priv.runtime_lock);
	printk("%s\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	printk("%s\n", __func__);
	return 0;
}
static struct dentry *composer_pdbg_root;

static int dumplayer_open(struct inode * inode, struct file * file)
{
	return 0;
}
static int dumplayer_release(struct inode * inode, struct file * file)
{
	return 0;
}

static ssize_t dumplayer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    dumplayer_t *dmplyr = NULL, *next = NULL;
    composer_priv.dumpstart = 1;

    while(composer_priv.dumpCnt != 0)
    {
        list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
        {
            if(dmplyr->update)
            {
                debug_write_file(dmplyr);
            }
        }
    }
    printk("dumplist counter %d\n",composer_priv.listcnt);
    list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
    {
        list_del(&dmplyr->list);
        if(dmplyr->update)
        {
            debug_write_file(dmplyr);
        }
        if(dmplyr->isphaddr)
        {
            sunxi_unmap_kernel(dmplyr->vm_addr,dmplyr->isphaddr,dmplyr->size);
            sunxi_free_phys((unsigned int)dmplyr->p_addr,dmplyr->size);
        }else{
            kfree(dmplyr->vm_addr);
        }
        kfree(dmplyr);
    }
    composer_priv.dumpstart = 0;
	return 0;
}
static ssize_t dumplayer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    dumplayer_t *dmplyr = NULL, *next = NULL;
    char temp[count+1];
    char *s = temp;
    int cnt=0;
    if(copy_from_user( temp, (void __user *)buf, count))
    {
        printk("copy_from_user fail\n");
        return  -EFAULT;
    }
    temp[count] = '\0';
    printk("%s\n",temp);
    switch(*s++)
    {
        case 'p':
            composer_priv.pause = 1;
        break;
        case 'q':
            composer_priv.pause = 0;
            composer_priv.cancel = 0;
            composer_priv.dumpstart = 0;
            composer_priv.dumpCnt = 0;
            list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
            {
                list_del(&dmplyr->list);
                if(dmplyr->isphaddr)
                {
                    sunxi_unmap_kernel(dmplyr->vm_addr,dmplyr->isphaddr,dmplyr->size);
                    sunxi_free_phys((unsigned int)dmplyr->p_addr,dmplyr->size);
                }else{
                    kfree(dmplyr->vm_addr);
                }
                kfree(dmplyr);
            }
        break;
        case 'c':
            composer_priv.cancel = 1;
        case 'd':
            composer_priv.pause = 0;
            composer_priv.display = (*s >= 48 && *s <=49)? *s - 48 : 255;
            s++;
            if(*s == 'c')
            {
                s++;
                composer_priv.channelNum = (*s >= 48 && *s <=51)? *s - 48 : 255;
            }else{
                composer_priv.dumpCnt = 0;
                break;
            }
            s++;
            if(*s == 'l')
            {
                s++;
                composer_priv.layerNum = (*s >= 48 && *s <=51)? *s - 48 : 255;
            }else{
                composer_priv.dumpCnt = 0;
                break;
            }
            s++;
            if(*s == 'n')
            {
                while(*++s != '\0')
                {
                    if( 57< *s || *s <48)
                        break;
                    cnt += (*s-48);
                    cnt *=10;
                }
                composer_priv.dumpCnt = cnt/10;
            }else{
                composer_priv.dumpCnt = 0;
            }
            composer_priv.listcnt = 0;
        break;
        default:
            printk(" dev_composer debug give me a wrong arg...\n");
    }
    printk("dumps --Cancel[%d]--display[%d]--channel[%d]--layer[%d]--Cnt:[%d]--pause[%d] \n",composer_priv.cancel,composer_priv.display,composer_priv.channelNum,composer_priv.layerNum,composer_priv.dumpCnt,composer_priv.pause);
    return count;
}


static const struct file_operations dumplayer_ops = {
	.write        = dumplayer_write,
	.read        = dumplayer_read,
	.open        = dumplayer_open,
	.release    = dumplayer_release,
};

int composer_dbg(void)
{
	composer_pdbg_root = debugfs_create_dir("composerdbg", NULL);
	if(!debugfs_create_file("dumplayer", 0644, composer_pdbg_root, NULL,&dumplayer_ops))
		goto Fail;
	return 0;

Fail:
	debugfs_remove_recursive(composer_pdbg_root);
	composer_pdbg_root = NULL;
	return -ENOENT;
}

s32 composer_init(disp_drv_info *psg_disp_drv)
{

	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	mutex_init(&composer_priv.runtime_lock);
    composer_priv.Display_commit_work = create_freezable_workqueue("SunxiDisCommit");
    composer_priv.Display_WB_work = create_freezable_workqueue("Sunxi_WB");
    INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
    INIT_WORK(&composer_priv.WB_work, write_back);

	INIT_LIST_HEAD(&composer_priv.update_regs_list);
    INIT_LIST_HEAD(&composer_priv.dumplyr_list);
    INIT_LIST_HEAD(&composer_priv.WB_list);
	composer_priv.relseastimeline = sw_sync_timeline_create("sunxi-display");
    composer_priv.writebacktimeline = sw_sync_timeline_create("Sunxi-WB");
    composer_priv.WB_count = 0;
	composer_priv.timeline_max = 0;
	composer_priv.b_no_output = 0;
    composer_priv.Cur_Write_Cnt = 0;
    composer_priv.display_active[0] = 0;
    composer_priv.display_active[1] = 0;
    composer_priv.Cur_Disp_Cnt[0] = 0;
    composer_priv.Cur_Disp_Cnt[1] = 0;
    composer_priv.countrotate[0] = 0;
    composer_priv.countrotate[1] = 0;
    composer_priv.pause = 0;
    composer_priv.listcnt = 0;
    composer_priv.last_wb_cnt[0] = 0;
    composer_priv.last_wb_cnt[1] = 0;
    composer_priv.tr_fd = 0;
    composer_priv.initrial = 0;
	mutex_init(&composer_priv.update_regs_list_lock);
    mutex_init(&gcommit_mutek);
    mutex_init(&gwb_mutek);
	spin_lock_init(&(composer_priv.update_reg_lock));
    spin_lock_init(&(composer_priv.dumplyr_lock));
    spin_lock_init(&(composer_priv.WB_lock));
	disp_register_ioctl_func(DISP_HWC_COMMIT, hwc_commit_ioctl);
    disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
    composer_priv.tmptransfer = kzalloc(sizeof(setup_dispc_data_t),GFP_KERNEL);
    composer_priv.psg_disp_drv = psg_disp_drv;
    composer_dbg();
    return 0;
}


#else

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include "dev_disp.h"
#include <video/sunxi_display2.h>
static struct   mutex	gcommit_mutek;

#define DBG_TIME_TYPE 3
#define DBG_TIME_SIZE 100
typedef struct
{
	unsigned long         time[DBG_TIME_TYPE][DBG_TIME_SIZE];
	unsigned int          time_index[DBG_TIME_TYPE];
	unsigned int          count[DBG_TIME_TYPE];
}composer_health_info;

typedef struct {
    int                     outaquirefencefd;
    int                     width;
    int                     Wstride;
    int                     height;
    disp_pixel_format       format;
    unsigned int            phys_addr;
}WriteBack_t;
typedef struct
{
    int                 layer_num[2];
    disp_layer_config   layer_info[2][16];
    int*                aquireFenceFd;
    int                 aquireFenceCnt;
    int*                returnfenceFd;
    bool                needWB[2];
    unsigned int        ehancemode[2]; //0 is close,1 is whole,2 is half mode
    unsigned int        androidfrmnum;
    WriteBack_t         *WriteBackdata;
}setup_dispc_data_t;
typedef struct
{
    struct list_head    list;
    unsigned  int       framenumber;
    unsigned  int       androidfrmnum;
    setup_dispc_data_t  hwc_data;
}dispc_data_list_t;

typedef struct
{
    struct list_head    list;
    void *              vm_addr;
    void*               p_addr;
    unsigned int        size;
    unsigned  int       androidfrmnum;
    bool                isphaddr;
    bool                update;
}dumplayer_t;

struct composer_private_data
{
	struct work_struct    post2_cb_work;
	u32	                  Cur_Write_Cnt;
	u32                   Cur_Disp_Cnt[2];
	bool                  b_no_output;
    bool                  display_active[2];
    bool                  countrotate[2];
	struct                mutex	runtime_lock;
	struct list_head        update_regs_list;

	unsigned int            timeline_max;
	struct mutex            update_regs_list_lock;
	spinlock_t              update_reg_lock;
	struct work_struct      commit_work;
    struct workqueue_struct *Display_commit_work;
    struct sw_sync_timeline *relseastimeline;
    struct sw_sync_timeline *writebacktimeline;
    setup_dispc_data_t      *tmptransfer;
    disp_drv_info           *psg_disp_drv;

    struct list_head        dumplyr_list;
    spinlock_t              dumplyr_lock;
    unsigned int            listcnt;
    unsigned char           dumpCnt;
    unsigned char           display;
    unsigned char           layerNum;
    unsigned char           channelNum;
    bool                    pause;
    bool                    cancel;
    bool                    dumpstart;
    unsigned int            ehancemode[2];
	composer_health_info    health_info;
};
static struct composer_private_data composer_priv;

int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, unsigned int framenuber);

//type: 0:acquire, 1:release; 2:display
static s32 composer_get_frame_fps(u32 type)
{
	__u32 pre_time_index, cur_time_index;
	__u32 pre_time, cur_time;
	__u32 fps = 0xff;

	pre_time_index = composer_priv.health_info.time_index[type];
	cur_time_index = (pre_time_index == 0)? (DBG_TIME_SIZE -1):(pre_time_index-1);

	pre_time = composer_priv.health_info.time[type][pre_time_index];
	cur_time = composer_priv.health_info.time[type][cur_time_index];

	if(pre_time != cur_time) {
		fps = 1000 * 100 / (cur_time - pre_time);
	}

	return fps;
}

//type: 0:acquire, 1:release; 2:display
static void composer_frame_checkin(u32 type)
{
	u32 index = composer_priv.health_info.time_index[type];
	composer_priv.health_info.time[type][index] = jiffies;
	index ++;
	index = (index>=DBG_TIME_SIZE)?0:index;
	composer_priv.health_info.time_index[type] = index;
	composer_priv.health_info.count[type]++;
}

unsigned int composer_dump(char* buf)
{
	u32 fps0,fps1,fps2;
	u32 cnt0,cnt1,cnt2;

	fps0 = composer_get_frame_fps(0);
	fps1 = composer_get_frame_fps(1);
	fps2 = composer_get_frame_fps(2);
	cnt0 = composer_priv.health_info.count[0];
	cnt1 = composer_priv.health_info.count[1];
	cnt2 = composer_priv.health_info.count[2];

	return sprintf(buf, "acquire: %d, %d.%d fps\nrelease: %d, %d.%d fps\ndisplay: %d, %d.%d fps\n",
		cnt0, fps0/10, fps0%10, cnt1, fps1/10, fps1%10, cnt2, fps2/10, fps2%10);
}

int dev_composer_debug(setup_dispc_data_t *psDispcData, unsigned int framenuber)
{
    unsigned int  size = 0 ;
    void *kmaddr = NULL,*vm_addr = NULL,*sunxi_addr = NULL;
    disp_layer_config *dumlayer = NULL;
    dumplayer_t *dmplyr = NULL, *next = NULL, *tmplyr = NULL;
    bool    find = 0;
    if(composer_priv.cancel && composer_priv.display != 255 && composer_priv.channelNum != 255 && composer_priv.layerNum != 255)
    {
        dumlayer = &(psDispcData->layer_info[composer_priv.display][composer_priv.channelNum*4+composer_priv.layerNum]);
        dumlayer->enable =0;
    }
    if(composer_priv.dumpstart)
    {
        if(composer_priv.dumpCnt > 0 && composer_priv.display != 255 && composer_priv.channelNum != 255 && composer_priv.layerNum != 255 )
        {
            dumlayer = &(psDispcData->layer_info[composer_priv.display][composer_priv.channelNum*4+composer_priv.layerNum]);
            if(dumlayer != NULL && dumlayer->enable)
            {
                switch(dumlayer->info.fb.format)
                {
                    case DISP_FORMAT_ABGR_8888:
                    case DISP_FORMAT_ARGB_8888:
                    case DISP_FORMAT_XRGB_8888:
                    case DISP_FORMAT_XBGR_8888:
                        size = 4*dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height;
                    break;
                    case DISP_FORMAT_BGR_888:
                        size = 3*dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height;
                    break;
                    case DISP_FORMAT_RGB_565:
                        size = 2*dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height;
                    break;
                    case DISP_FORMAT_YUV420_P:
                        size = dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height
                            +dumlayer->info.fb.size[1].width * dumlayer->info.fb.size[1].height
                            +dumlayer->info.fb.size[2].width * dumlayer->info.fb.size[2].height;
                    break;
                    case DISP_FORMAT_YUV420_SP_VUVU:
                        size = dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height
                            +dumlayer->info.fb.size[1].width * dumlayer->info.fb.size[1].height;
                    break;
                    case DISP_FORMAT_YUV420_SP_UVUV:
                        size = dumlayer->info.fb.size[0].width * dumlayer->info.fb.size[0].height
                            +dumlayer->info.fb.size[1].width * dumlayer->info.fb.size[1].height;
                    break;
                    default:
                        printk("we got a err info..\n");
                }
                sunxi_addr = sunxi_map_kernel((unsigned int)dumlayer->info.fb.addr[0],size);
                if(sunxi_addr == NULL)
                {
                    printk("map mem err...\n");
                    goto err;
                }
                list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
                {
                    if(!dmplyr->update)
                    {
                        if(dmplyr->size == size)
                        {
                            find = 1;
                            break;
                        }else{
                            tmplyr = dmplyr;
                        }
                    }
                }
                if(find)
                {
                    memcpy(dmplyr->vm_addr, sunxi_addr, size);
                    sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                    dmplyr->update = 1;
                    dmplyr->androidfrmnum = framenuber;
                    composer_priv.dumpCnt--;
                    goto ok;
                }else if(tmplyr != NULL)
                {
                    dmplyr = tmplyr;
                     if(dmplyr->isphaddr)
                        {
                            sunxi_unmap_kernel(dmplyr->vm_addr,dmplyr->isphaddr,dmplyr->size);
                            sunxi_free_phys((unsigned int)dmplyr->p_addr,dmplyr->size);
                        }else{
                            kfree((void *)dmplyr->vm_addr);
                        }
                        dmplyr->vm_addr = NULL;
                        dmplyr->p_addr = NULL;
                        dmplyr->size = 0;
                }else{
                    dmplyr = kzalloc(sizeof(dumplayer_t),GFP_KERNEL);
                    if(dmplyr == NULL)
                    {
                        printk("kzalloc mem err...\n");
			sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                        goto err;
                    }
                }
                vm_addr = sunxi_buf_alloc(size, (unsigned int*)&kmaddr);
                dmplyr->isphaddr = 1;
                if(kmaddr == 0)
                {
                    vm_addr = kzalloc(size,GFP_KERNEL);
                    dmplyr->isphaddr = 0;
                    if(vm_addr == NULL)
                    {
			sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                        dmplyr->update = 0;
                        printk("kzalloc mem err...\n");
                        goto err;
                    }
                    dmplyr->vm_addr = vm_addr;
                }
                if(dmplyr->isphaddr)
                {
                    dmplyr->vm_addr = sunxi_map_kernel((unsigned int)kmaddr,size);
                    if(dmplyr->vm_addr == NULL)
                    {
			sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                        dmplyr->update = 0;
                        printk("kzalloc mem err...\n ");
                        goto err;
                    }
                    dmplyr->p_addr = kmaddr;
                }
                dmplyr->size = size;
                dmplyr->androidfrmnum = framenuber;
                memcpy(dmplyr->vm_addr, sunxi_addr, size);
		sunxi_unmap_kernel(sunxi_addr,dumlayer->info.fb.addr[0],size);
                dmplyr->update = 1;
                if(tmplyr == NULL)
                {
                    composer_priv.listcnt++;
                    list_add_tail(&dmplyr->list, &composer_priv.dumplyr_list);
                }
            }
            composer_priv.dumpCnt--;
        }
    }
ok:
    return 0;
err:
    return -1;
}
static int debug_write_file(dumplayer_t *dumplyr)
{
    char s[30];
    struct file *dumfile;
    mm_segment_t old_fs;
    int cnt;
    if(dumplyr->vm_addr != NULL && dumplyr->size != 0)
    {
        cnt = sprintf(s, "/mnt/sdcard/dumplayer%d",dumplyr->androidfrmnum);
        dumfile = filp_open(s, O_RDWR|O_CREAT, 0755);
        if(IS_ERR(dumfile))
        {
            printk("open %s err[%d]\n",s,(int)dumfile);
            return 0;
        }
        old_fs = get_fs();
        set_fs(KERNEL_DS);
        if(dumplyr->vm_addr != NULL && dumplyr->size !=0)
        {
            dumfile->f_op->write(dumfile, dumplyr->vm_addr, dumplyr->size, &dumfile->f_pos);
        }
        set_fs(old_fs);
        filp_close(dumfile, NULL);
        dumfile = NULL;
        dumplyr->update = 0;
    }
   return 0 ;
}

/* define the data cache of frame */
#define DATA_ALLOC 1
#define DATA_FREE 0
static int cache_num = 8;
static struct mutex cache_opr;
typedef struct data_cache{
	dispc_data_list_t *addr;
	int flag;
	struct data_cache *next;
}data_cache_t;
static data_cache_t *frame_data;
/* destroy data cache of frame */
static int mem_cache_destroy(){
	data_cache_t *cur = frame_data;
	data_cache_t *next = NULL;
	while(cur != NULL){
		if(cur->addr != NULL){
			kfree(cur->addr);
		}
		next = cur->next;
		kfree(cur);
		cur = next;
	}
	mutex_destroy(&cache_opr);
	return 0;
}
/* create data cache of frame */
static int mem_cache_create(){
	int i = 0;
	data_cache_t *cur = NULL;
	mutex_init(&cache_opr);
	frame_data = kzalloc(sizeof(data_cache_t), GFP_ATOMIC);
	if(frame_data == NULL){
		printk("alloc frame data[0] fail\n");
		return -1;
	}
	frame_data->addr = kzalloc(sizeof(dispc_data_list_t), GFP_ATOMIC);
	if(frame_data->addr == NULL){
		printk("alloc dispc data[0] fail\n");
		mem_cache_destroy();
		return -1;
	}
	frame_data->flag = DATA_FREE;
	cur = frame_data;
	for(i = 1; i < cache_num ; i ++){
		cur->next = kzalloc(sizeof(data_cache_t), GFP_ATOMIC);
		if(cur->next == NULL){
			printk("alloc frame data[%d] fail\n", i);
			mem_cache_destroy();
			return -1;
		}
		cur->next->addr = kzalloc(sizeof(dispc_data_list_t), GFP_ATOMIC);
		if(cur->next->addr == NULL){
			printk("alloc dispc data[%d] fail\n", i);
			mem_cache_destroy();
			return -1;
		}
		cur->next->flag = DATA_FREE;
		cur = cur->next;
	}
	return 0;
}
/* free data of a frame from cache*/
static int mem_cache_free(dispc_data_list_t *addr){
	int i = 0;
	data_cache_t *cur = NULL;
	mutex_lock(&cache_opr);
	cur = frame_data;
	for(i = 0; (cur != NULL) && (i < cache_num); i++){
		if(addr != NULL && cur->addr == addr){
			cur->flag = DATA_FREE;
			mutex_unlock(&cache_opr);
			return 0;
		}
		cur = cur->next;
	}
	mutex_unlock(&cache_opr);
	return -1;
}
/* alloc data of a frame from cache */
static dispc_data_list_t* mem_cache_alloc(){
	int i = 0;
	data_cache_t *cur = NULL;
	mutex_lock(&cache_opr);
	cur = frame_data;
	for(i = 0; i < cache_num; i++){
		if(cur != NULL && cur->flag == DATA_FREE){
			if(cur->addr != NULL){
				memset(cur->addr, 0, sizeof(dispc_data_list_t));
			}
			cur->flag = DATA_ALLOC;
			mutex_unlock(&cache_opr);
			return cur->addr;
		}else if(cur == NULL){
			printk("alloc frame data fail, can not find avail buffer.\n");
			mutex_unlock(&cache_opr);
			return NULL;
		}
		if(i < cache_num - 1){
			cur = cur->next;
		}
	}
	printk("All frame data are used, try adding new...\n");
	cur->next = kzalloc(sizeof(data_cache_t), GFP_ATOMIC);
	if(cur->next == NULL){
		printk("alloc a frame data fail\n");
		mutex_unlock(&cache_opr);
		return NULL;
	}
	cur->next->addr = kzalloc(sizeof(dispc_data_list_t), GFP_ATOMIC);
	if(cur->next->addr == NULL){
		printk("alloc a dispc data fail\n");
		kfree(cur->next);
		cur->next = NULL;
		mutex_unlock(&cache_opr);
		return NULL;
	}
	cur->next->flag = DATA_ALLOC;
	cache_num++;
	printk("create a new frame data success, cache num update to %d", cache_num);
	mutex_unlock(&cache_opr);
	return cur->next->addr;
}

static void hwc_commit_work(struct work_struct *work)
{
    dispc_data_list_t *data, *next;
    struct list_head saved_list;
    int err;
    int i;
    struct sync_fence *AcquireFence;

    mutex_lock(&(gcommit_mutek));
    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);
    mutex_unlock(&(composer_priv.update_regs_list_lock));

    list_for_each_entry_safe(data, next, &saved_list, list)
    {
        list_del(&data->list);
	    for(i = 0; i < data ->hwc_data.aquireFenceCnt; i++)
	    {
            AcquireFence =(struct sync_fence *) data->hwc_data.aquireFenceFd[i];
            if(AcquireFence != NULL)
            {
                err = sync_fence_wait(AcquireFence,1000);
                sync_fence_put(AcquireFence);
                if (err < 0)
	            {
	                printk("synce_fence_wait timeout AcquireFence:%p\n",AcquireFence);
                    sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
					goto free;
	            }
            }
	    }
        dev_composer_debug(&data->hwc_data, data->hwc_data.androidfrmnum);
        if(composer_priv.pause == 0)
        {
            dispc_gralloc_queue(&data->hwc_data, data->framenumber);
        }
free:
        kfree(data->hwc_data.aquireFenceFd);
        mem_cache_free(data);
    }
	mutex_unlock(&(gcommit_mutek));
}

static int hwc_commit(setup_dispc_data_t *disp_data)
{
	dispc_data_list_t *disp_data_list;
	struct sync_fence *fence;
	struct sync_pt *pt;
	int fd = -1, cout = 0, coutoffence = 0;
    int *fencefd = NULL;

    fencefd = kzalloc(( disp_data->aquireFenceCnt * sizeof(int)), GFP_ATOMIC);
    if(!fencefd){
        printk("out of momery , do not display.\n");
        return -1;
    }
    if(copy_from_user( fencefd, (void __user *)disp_data->aquireFenceFd, disp_data->aquireFenceCnt * sizeof(int)))
    {
            printk("copy_from_user fail\n");
            kfree(fencefd);
            return  -1;
    }
    for(cout = 0; cout < disp_data->aquireFenceCnt; cout++)
    {
        fence = sync_fence_fdget(fencefd[cout]);
        if(!fence)
        {
            printk("sync_fence_fdget failed,fd[%d]:%d\n",cout, fencefd[cout]);
            continue;
        }
        fencefd[coutoffence] = (int)fence;
        coutoffence++;
    }
    disp_data->aquireFenceFd = fencefd;
    disp_data->aquireFenceCnt = coutoffence;

    if(!composer_priv.b_no_output)
    {
        if(disp_data->layer_num[0]+disp_data->layer_num[1] > 0)
        {
            disp_data_list = mem_cache_alloc();
            if(!disp_data_list){
                kfree(fencefd);
                printk("%s: %d, alloc data for disp_data_list fail.\n", __func__, __LINE__);
                copy_to_user((void __user *)disp_data->returnfenceFd, &fd, sizeof(int));
                return -1;
            }
            fd = get_unused_fd();
            if (fd < 0)
            {
                mem_cache_free(disp_data_list);
                kfree(fencefd);
                return -1;
            }
            composer_priv.timeline_max++;
            pt = sw_sync_pt_create(composer_priv.relseastimeline, composer_priv.timeline_max);
            fence = sync_fence_create("sunxi_display", pt);
            sync_fence_install(fence, fd);
            memcpy(&disp_data_list->hwc_data, disp_data, sizeof(setup_dispc_data_t));
            disp_data_list->framenumber = composer_priv.timeline_max;
            mutex_lock(&(composer_priv.update_regs_list_lock));
            list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
            mutex_unlock(&(composer_priv.update_regs_list_lock));
            if(!composer_priv.pause)
            {
                queue_work(composer_priv.Display_commit_work, &composer_priv.commit_work);
            }
        }else{
            kfree(fencefd);
            return -1;
        }
    }else{
        flush_workqueue(composer_priv.Display_commit_work);
        kfree(fencefd);
        return -1;
    }
    if(copy_to_user((void __user *)disp_data->returnfenceFd, &fd, sizeof(int)))
    {
	    printk("copy_to_user fail\n");
	    return  -EFAULT;
	}
	return 0;

}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = -1;
	if(DISP_HWC_COMMIT == cmd)
    {
        unsigned long *ubuffer;
        ubuffer = (unsigned long *)arg;
        memset(composer_priv.tmptransfer, 0, sizeof(setup_dispc_data_t));
        if(copy_from_user(composer_priv.tmptransfer, (void __user *)ubuffer[1], sizeof(setup_dispc_data_t)))
        {
            printk("copy_from_user fail\n");
            return  -EFAULT;
		}
        ret = hwc_commit(composer_priv.tmptransfer);
	}
	return ret;
}

static void disp_composer_proc(u32 sel)
{
    if(sel<2)
    {
        if(composer_priv.Cur_Write_Cnt < composer_priv.Cur_Disp_Cnt[sel])
        {
            composer_priv.countrotate[sel] = 1;
        }
        composer_priv.Cur_Disp_Cnt[sel] = composer_priv.Cur_Write_Cnt;
    }
	schedule_work(&composer_priv.post2_cb_work);
	return ;
}

static void imp_finish_cb(bool force_all)
{
    u32 little = 1;
	u32 flag = 0;

    if(composer_priv.pause)
    {
        return;
    }
    if(composer_priv.display_active[0])
    {
        little = composer_priv.Cur_Disp_Cnt[0];
    }
    if( composer_priv.display_active[0] && composer_priv.countrotate[0]
        &&composer_priv.display_active[1]&& composer_priv.countrotate[1])
    {
        composer_priv.countrotate[0] = 0;
        composer_priv.countrotate[1] = 0;
    }
    if(composer_priv.display_active[1])
    {
        if( composer_priv.display_active[0])
        {
            if(composer_priv.countrotate[0] != composer_priv.countrotate[1])
            {
                if(composer_priv.countrotate[0] && composer_priv.display_active[0])
                {
                    little = composer_priv.Cur_Disp_Cnt[1];
                }else{
                    little = composer_priv.Cur_Disp_Cnt[0];
                }
            }else{
                if(composer_priv.Cur_Disp_Cnt[1] > composer_priv.Cur_Disp_Cnt[0])
                {
                    little = composer_priv.Cur_Disp_Cnt[0];
                }else{
                    little = composer_priv.Cur_Disp_Cnt[1];
                }
            }
      }else{
            little = composer_priv.Cur_Disp_Cnt[1];
      }
    }
    while(composer_priv.relseastimeline->value != composer_priv.Cur_Write_Cnt)
    {
        if(!force_all && (composer_priv.relseastimeline->value >= little -1))
        {
            break;
        }
        sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
        composer_frame_checkin(1);//release
        flag = 1;
    }
    if(flag)
		composer_frame_checkin(2);//display
}

static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.runtime_lock);
    imp_finish_cb(composer_priv.b_no_output);
	mutex_unlock(&composer_priv.runtime_lock);
}
extern s32  bsp_disp_shadow_protect(u32 disp, bool protect);
int dispc_gralloc_queue(setup_dispc_data_t *psDispcData, unsigned int framenuber)
{
    int disp;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_enhance *psenhance = NULL;
    disp_layer_config *psconfig;
    disp = 0;

    while( disp < DISP_NUMS_SCREEN )
    {
        if(!psDispcData->layer_num[disp])
        {
            composer_priv.display_active[disp] = 0;
            disp++;
            continue;
        }
        psmgr = psg_disp_drv->mgr[disp];
        if( psmgr != NULL  )
        {
            psenhance = psmgr->enhance;
            bsp_disp_shadow_protect(disp,true);
            if(psDispcData->ehancemode[disp] )
            {
                if(psDispcData->ehancemode[disp] != composer_priv.ehancemode[disp])
                {
                    switch (psDispcData->ehancemode[disp])
                    {
                        case 1:
                            psenhance->demo_disable(psenhance);
                            psenhance->enable(psenhance);
                        break;
                        case 2:
                            psenhance->enable(psenhance);
                            psenhance->demo_enable(psenhance);
                        break;
                        default:
                            psenhance->disable(psenhance);
                            printk("translat a err info\n");
                    }
                }
                composer_priv.ehancemode[disp] = psDispcData->ehancemode[disp];
            }else{
                if(composer_priv.ehancemode[disp])
                {
                    psenhance->disable(psenhance);
                    composer_priv.ehancemode[disp] = 0;
                }
            }

            psconfig = &psDispcData->layer_info[disp][0];
            psmgr->set_layer_config(psmgr, psconfig, disp?8:16);
            bsp_disp_shadow_protect(disp,false);
            if(composer_priv.display_active[disp] == 0)
            {
                composer_priv.display_active[disp] = 1;
                composer_priv.Cur_Disp_Cnt[disp] = framenuber;
            }
        }
        disp++;
    }
    composer_priv.Cur_Write_Cnt = framenuber;
    composer_frame_checkin(0);//acquire
    if(composer_priv.b_no_output)
    {
        mutex_lock(&composer_priv.runtime_lock);
        imp_finish_cb(1);
        mutex_unlock(&composer_priv.runtime_lock);
    }
  return 0;
}

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	mutex_lock(&composer_priv.runtime_lock);
	printk("%s after lock\n", __func__);
	imp_finish_cb(1);
	mutex_unlock(&composer_priv.runtime_lock);
	printk("%s release lock\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	printk("%s\n", __func__);
	return 0;
}
static struct dentry *composer_pdbg_root;

static int dumplayer_open(struct inode * inode, struct file * file)
{
	return 0;
}
static int dumplayer_release(struct inode * inode, struct file * file)
{
	return 0;
}

static ssize_t dumplayer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    dumplayer_t *dmplyr = NULL, *next = NULL;
    composer_priv.dumpstart = 1;

    while(composer_priv.dumpCnt != 0)
    {
        list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
        {
            if(dmplyr->update)
            {
                debug_write_file(dmplyr);
            }
        }
    }
    printk("dumplist counter %d\n",composer_priv.listcnt);
    list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
    {
        list_del(&dmplyr->list);
        if(dmplyr->update)
        {
            debug_write_file(dmplyr);
        }
        if(dmplyr->isphaddr)
        {
            sunxi_unmap_kernel(dmplyr->vm_addr,dmplyr->isphaddr,dmplyr->size);
            sunxi_free_phys((unsigned int)dmplyr->p_addr,dmplyr->size);
        }else{
            kfree(dmplyr->vm_addr);
        }
        kfree(dmplyr);
    }
    composer_priv.dumpstart = 0;
	return 0;
}
static ssize_t dumplayer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    dumplayer_t *dmplyr = NULL, *next = NULL;
    char temp[count+1];
    char *s = temp;
    int cnt=0;
    if(copy_from_user( temp, (void __user *)buf, count))
    {
        printk("copy_from_user fail\n");
        return  -EFAULT;
    }
    temp[count] = '\0';
    printk("%s\n",temp);
    switch(*s++)
    {
        case 'p':
            composer_priv.pause = 1;
        break;
        case 'q':
            composer_priv.pause = 0;
            composer_priv.cancel = 0;
            composer_priv.dumpstart = 0;
            composer_priv.dumpCnt = 0;
            list_for_each_entry_safe(dmplyr, next, &composer_priv.dumplyr_list, list)
            {
                list_del(&dmplyr->list);
                if(dmplyr->isphaddr)
                {
                    sunxi_unmap_kernel(dmplyr->vm_addr,dmplyr->isphaddr,dmplyr->size);
                    sunxi_free_phys((unsigned int)dmplyr->p_addr,dmplyr->size);
                }else{
                    kfree(dmplyr->vm_addr);
                }
                kfree(dmplyr);
            }
        break;
        case 'c':
            composer_priv.cancel = 1;
        case 'd':
            composer_priv.pause = 0;
            composer_priv.display = (*s >= 48 && *s <=49)? *s - 48 : 255;
            s++;
            if(*s == 'c')
            {
                s++;
                composer_priv.channelNum = (*s >= 48 && *s <=51)? *s - 48 : 255;
            }else{
                composer_priv.dumpCnt = 0;
                break;
            }
            s++;
            if(*s == 'l')
            {
                s++;
                composer_priv.layerNum = (*s >= 48 && *s <=51)? *s - 48 : 255;
            }else{
                composer_priv.dumpCnt = 0;
                break;
            }
            s++;
            if(*s == 'n')
            {
                while(*++s != '\0')
                {
                    if( 57< *s || *s <48)
                        break;
                    cnt += (*s-48);
                    cnt *=10;
                }
                composer_priv.dumpCnt = cnt/10;
            }else{
                composer_priv.dumpCnt = 0;
            }
            composer_priv.listcnt = 0;
        break;
        default:
            printk(" dev_composer debug give me a wrong arg...\n");
    }
    printk("dumps --Cancel[%d]--display[%d]--channel[%d]--layer[%d]--Cnt:[%d]--pause[%d] \n",composer_priv.cancel,composer_priv.display,composer_priv.channelNum,composer_priv.layerNum,composer_priv.dumpCnt,composer_priv.pause);
    return count;
}


static const struct file_operations dumplayer_ops = {
	.write        = dumplayer_write,
	.read        = dumplayer_read,
	.open        = dumplayer_open,
	.release    = dumplayer_release,
};

int composer_dbg(void)
{
	composer_pdbg_root = debugfs_create_dir("composerdbg", NULL);
	if(!debugfs_create_file("dumplayer", 0644, composer_pdbg_root, NULL,&dumplayer_ops))
		goto Fail;
	return 0;

Fail:
	debugfs_remove_recursive(composer_pdbg_root);
	composer_pdbg_root = NULL;
	return -ENOENT;
}

s32 composer_init(disp_drv_info *psg_disp_drv)
{
	int ret = 0;

	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));

	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
	mutex_init(&composer_priv.runtime_lock);

    composer_priv.Display_commit_work = create_freezable_workqueue("SunxiDisCommit");
    INIT_WORK(&composer_priv.commit_work, hwc_commit_work);
	INIT_LIST_HEAD(&composer_priv.update_regs_list);
    INIT_LIST_HEAD(&composer_priv.dumplyr_list);
	composer_priv.relseastimeline = sw_sync_timeline_create("sunxi-display");
	composer_priv.timeline_max = 0;
	composer_priv.b_no_output = 0;
    composer_priv.Cur_Write_Cnt = 0;
    composer_priv.display_active[0] = 0;
    composer_priv.display_active[1] = 0;
    composer_priv.Cur_Disp_Cnt[0] = 0;
    composer_priv.Cur_Disp_Cnt[1] = 0;
    composer_priv.countrotate[0] = 0;
    composer_priv.countrotate[1] = 0;
    composer_priv.pause = 0;
    composer_priv.listcnt = 0;
	mutex_init(&composer_priv.update_regs_list_lock);
    mutex_init(&gcommit_mutek);
	spin_lock_init(&(composer_priv.update_reg_lock));
    spin_lock_init(&(composer_priv.dumplyr_lock));
	disp_register_ioctl_func(DISP_HWC_COMMIT, hwc_commit_ioctl);
    disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
    composer_priv.tmptransfer = kzalloc(sizeof(setup_dispc_data_t), GFP_ATOMIC);
    composer_priv.psg_disp_drv = psg_disp_drv;
    composer_dbg();
    /* alloc mem_des struct pool, 48k */
    ret = mem_cache_create();
    if(ret != 0){
        printk("%s(%d) alloc frame buffer err!\n", __func__, __LINE__);
    }

  return 0;
}

#endif // #if !defined(CONFIG_HOMLET_PLATFORM)

#endif
