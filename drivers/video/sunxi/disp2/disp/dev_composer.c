#ifndef DEV_COMPOSER_C_C
#define DEV_COMPOSER_C_C

#define HWC_CMD_GET_FPS			(0x00000001)
#define HWC_CMD_GET_CLK_RATE	(0x00000002)

#if 0 //!defined(CONFIG_HOMLET_PLATFORM)

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include "dev_disp.h"
#include <video/sunxi_display2.h>
#include <linux/sunxi_tr.h>

typedef u32 compat_uptr_t;

#define DBG_TIME_TYPE 3
#define DBG_TIME_SIZE 100
#define WB_CHECK_SIZE  5
#define DISP_NUMS_SCREEN 2
enum {
    HWC_SYNC_NEED = -2,
    HWC_SYNC_INIT = -1,
    HWC_DISP0 = 0,
    HWC_DISP1 = 1,
    HWC_OTHER0 = 2,
    HWC_OTHER1 = 3,
    CNOUTDISPSYNC = 4,
};

enum HWC_IOCTL{
    HWC_IOCTL_FENCEFD = 0,
    HWC_IOCTL_COMMIT = 1,
    HWC_IOCTL_CKWB = 2,
    HWC_IOCTL_SETPRIDIP,
};
struct hwc_ioctl_arg {
    enum HWC_IOCTL   cmd;
    void            *arg;
};

#if defined(CONFIG_COMPAT)
struct hwc_compat_ioctl_arg {
    enum HWC_IOCTL   cmd;
    compat_uptr_t    arg;
};
#endif

struct hwc_commit_layer{
    int                 aquirefencefd;
    struct disp_layer_config   hwc_layer_info;
};

struct hwc_dispc_data {
    struct disp_layer_config       *hwc_layer_info[DISP_NUMS_SCREEN];
    int                     releasefencefd[CNOUTDISPSYNC];
    struct disp_capture_info       *data;
    bool                    force_flip[DISP_NUMS_SCREEN];
};

#if defined(CONFIG_COMPAT)
struct hwc_compat_dispc_data {
    compat_uptr_t           hwc_layer_info[DISP_NUMS_SCREEN];
    int                     releasefencefd[CNOUTDISPSYNC];
    compat_uptr_t           data;
    bool                    force_flip[DISP_NUMS_SCREEN];
};
#endif

struct display_sync {
    unsigned int            timeline_count;
    struct sw_sync_timeline *timeline;
    bool                    active;
};

struct write_back {
    unsigned int    sync;
    bool            success;
};

struct composer_health_info {
	unsigned long         time[DBG_TIME_TYPE][DBG_TIME_SIZE];
	unsigned int          time_index[DBG_TIME_TYPE];
	unsigned int          count[DBG_TIME_TYPE];
};

struct composer_private_data {
	struct work_struct      post2_cb_work;
	u32	                    cur_write_cnt[CNOUTDISPSYNC];
	u32                     cur_disp_cnt[CNOUTDISPSYNC];
    u32                     last_diplay_cnt[CNOUTDISPSYNC];
    u32                     primary_disp;
	bool                    b_no_output;
    bool                    wb_status;
    struct mutex	        sync_lock;
    int                     disp_hotplug_cnt[CNOUTDISPSYNC];
    struct display_sync     *display_sync[CNOUTDISPSYNC];//0 is de0 ,1 is de1 , 2 is writeback,3 is FB
    struct disp_layer_config       *tmp_hw_lyr;
    disp_drv_info           *psg_disp_drv;
    wait_queue_head_t	    commit_wq;
    struct write_back       check_wb[WB_CHECK_SIZE];
    struct composer_health_info    health_info;
};
static struct composer_private_data composer_priv;

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

static void imp_finish_cb(bool force_all)
{
    int i = 0;
    for(i = 0; i < CNOUTDISPSYNC; i++)
    {
        while( composer_priv.display_sync[i] != NULL
               && composer_priv.display_sync[i]->active
               && composer_priv.display_sync[i]->timeline->value != composer_priv.cur_write_cnt[i])
        {
            if(!force_all && composer_priv.display_sync[i]->timeline->value+1 == composer_priv.cur_disp_cnt[i])
            {
                break;
            }
            sw_sync_timeline_inc(composer_priv.display_sync[i]->timeline, 1);
            composer_frame_checkin(1);
        }
        if(composer_priv.cur_write_cnt[composer_priv.primary_disp] == composer_priv.cur_disp_cnt[composer_priv.primary_disp])
        {
            wake_up(&composer_priv.commit_wq);
        }
    }
}

extern s32  bsp_disp_shadow_protect(u32 disp, bool protect);
int dispc_gralloc_queue(struct disp_layer_config *commit_layer, unsigned int disp,unsigned int hwc_sync, struct disp_capture_info *data)
{
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *disp_mgr = NULL;
    struct disp_capture *write_back = NULL;

    disp_mgr = psg_disp_drv->mgr[disp];
    if( disp_mgr != NULL )
    {
        bsp_disp_shadow_protect(disp,true);
        if(data != NULL)
        {
            write_back = disp_mgr->cptr;
            if(composer_priv.wb_status != 1)
            {
                write_back->start(write_back);
                composer_priv.wb_status = 1;
            }
            write_back->commmit(write_back, data);
        }else{
            if(composer_priv.wb_status == 1)
            {
                write_back->stop(write_back);
                composer_priv.wb_status = 0;
            }
        }
        disp_mgr->set_layer_config(disp_mgr, commit_layer, disp?8:16);
        bsp_disp_shadow_protect(disp,false);
    }
    composer_frame_checkin(2);
    composer_priv.cur_write_cnt[disp] = hwc_sync;
    return 0;
}

inline bool hwc_pridisp_sync(void)
{
    return composer_priv.cur_write_cnt[composer_priv.primary_disp] == composer_priv.cur_disp_cnt[composer_priv.primary_disp];
}

unsigned int hwc_get_sync(int sync_disp,int fd)
{
    struct sync_fence *fence = NULL;
    struct sync_pt *pt = NULL;

#if defined(CONFIG_SW_SYNC)
    fence = sync_fence_fdget(fd);
    if(fence == NULL)
    {
        printk(KERN_ERR "hwc get relesefence(%d) err.\n",fd);
        goto err;
    }
    sync_fence_put(fence);
#endif
    if(!list_empty(&fence->pt_list_head))
    {
        pt = list_entry(fence->pt_list_head.next,struct sync_pt,pt_list);
    }else{
        printk(KERN_ERR"hwc get sw_pt err\n");
        goto err;
    }

    return  pt != NULL ? ((struct sw_sync_pt*)pt)->value : 0;
err:
    return composer_priv.cur_write_cnt[sync_disp]+1;
}

static bool hwc_fence_get(void *user_fence)
{
    struct sync_fence *fence = NULL;
	struct sync_pt *pt = NULL;
	int fd[CNOUTDISPSYNC] = {-1,-1,-1,-1},ret = -1;
    char buf[20];
    int i = 0;

    if(copy_from_user(&fd, (void __user *)user_fence, sizeof(int)*CNOUTDISPSYNC))
    {
        printk(KERN_ERR "copy_from_user hwc_fence_get err.\n");
        goto fecne_ret;
    }
    for(i = 0; i < CNOUTDISPSYNC; i++)
    {
        if(fd[i] == HWC_SYNC_NEED)
        {
            if(composer_priv.display_sync[i] == NULL)
            {
                ret = sprintf(buf, "sunxi_%d_%d",i,composer_priv.disp_hotplug_cnt[i]);
                composer_priv.display_sync[i]= kzalloc(sizeof(struct display_sync),GFP_KERNEL);
                if(composer_priv.display_sync[i] == NULL)
                {
                    printk(KERN_ERR "kzalloc the dispay_sync err.\n");
                    continue;
                }
                composer_priv.display_sync[i]->timeline = sw_sync_timeline_create(buf);
                if(composer_priv.display_sync[i]->timeline == NULL)
                {
                    kfree(composer_priv.display_sync[i]);
                    composer_priv.display_sync[i] = NULL;
                    printk("creat timeline err.\n");
                    continue;
                }
                composer_priv.cur_disp_cnt[i] = 0;
                composer_priv.cur_disp_cnt[i] = 0;
                composer_priv.display_sync[i]->active = 1;
                composer_priv.disp_hotplug_cnt[i]++;
            }
            composer_priv.display_sync[i]->timeline_count++;
            ret = sprintf(buf, "sunxi_%d_%d",i,composer_priv.display_sync[i]->timeline_count);
            fd[i] = get_unused_fd();
            if (fd[i] < 0)
            {
                printk(KERN_ERR"get unused fd faild\n");
                continue;
            }
            pt = sw_sync_pt_create(composer_priv.display_sync[i]->timeline, composer_priv.display_sync[i]->timeline_count);
            if(pt == NULL)
            {
                put_unused_fd(fd[i]);
                printk(KERN_ERR"creat display pt faild\n");
                continue;
            }
            fence = sync_fence_create(buf, pt);
            if(fence == NULL)
            {
                put_unused_fd(fd[i]);
                printk(KERN_ERR"creat dispay fence faild\n");
                continue;
            }
            sync_fence_install(fence, fd[i]);
        }else{
            if(composer_priv.display_sync[i] != NULL)
            {
                mutex_lock(&composer_priv.sync_lock);
                while(composer_priv.display_sync[i]->timeline->value != composer_priv.display_sync[i]->timeline_count)
                {
                    sw_sync_timeline_inc(composer_priv.display_sync[i]->timeline, 1);
                }
#if defined(CONFIG_SW_SYNC)
                sync_timeline_destroy(&composer_priv.display_sync[i]->timeline->obj);
#endif
                kfree(composer_priv.display_sync[i]);
                composer_priv.display_sync[i] = NULL;
                mutex_unlock(&composer_priv.sync_lock);
            }
        }
    }
fecne_ret:
    if(copy_to_user((void __user *)user_fence, fd, sizeof(int)*4))
    {
	    printk(KERN_ERR"copy_to_user fail\n");
	}
	return 0;
}

static int hwc_commit(void * user_display, bool compat)
{
    struct hwc_dispc_data disp_data;
#if defined(CONFIG_COMPAT)
    struct hwc_compat_dispc_data disp_compat_data;
#endif
    struct disp_capture_info wb_data;
    bool   need_wb = 0;
    int ret = 0, i = 0;
    unsigned int sync[CNOUTDISPSYNC];
    unsigned long   hwc_layer_info[DISP_NUMS_SCREEN];
    int     releasefencefd[CNOUTDISPSYNC];
    unsigned long wb_ptr_data;
    bool     force_flip[DISP_NUMS_SCREEN];

    composer_frame_checkin(0);
#if defined(CONFIG_COMPAT)
    if(compat)
    {
        ret = copy_from_user(&disp_compat_data, (void __user *)user_display, sizeof(struct hwc_compat_dispc_data));
        if(ret)
        {
            printk(KERN_ERR"hwc copy_from_user hwc_compat_dispc_data err.\n");
            goto commit_ok;
        }
        for(i = 0; i<DISP_NUMS_SCREEN; i++)
        {
            hwc_layer_info[i] = disp_compat_data.hwc_layer_info[i];
            force_flip[i] = disp_compat_data.force_flip[i];
        }
        for(i = 0; i<CNOUTDISPSYNC; i++)
        {
            releasefencefd[i] = disp_compat_data.releasefencefd[i];
        }
        wb_ptr_data = ((unsigned long)disp_compat_data.data);
    } else
#endif
    {
        ret = copy_from_user(&disp_data, (void __user *)user_display, sizeof(struct hwc_dispc_data));
        if(ret)
        {
            printk(KERN_ERR"hwc copy_from_user hwc_dispc_data err.\n");
            goto commit_ok;
        }
        for(i = 0; i<DISP_NUMS_SCREEN; i++)
        {
            hwc_layer_info[i] = (unsigned long)disp_data.hwc_layer_info[i];
            force_flip[i] = disp_data.force_flip[i];
        }
        for(i = 0; i<CNOUTDISPSYNC; i++)
        {
            releasefencefd[i] = disp_data.releasefencefd[i];
        }
        wb_ptr_data = (unsigned long)disp_data.data;
    }

    for(i = 0; i < CNOUTDISPSYNC; i++)
    {
        if(releasefencefd[i] >= 0)
        {
            sync[i] = hwc_get_sync(i,releasefencefd[i]);
            if(force_flip[i])
            {
                printk(KERN_INFO"hwc force flip disp[%d]:%d \n",i,sync[i]);
                if(composer_priv.display_sync[i] != NULL
                    && composer_priv.display_sync[i]->active
                    && composer_priv.display_sync[i]->timeline != NULL)
                {
                    composer_priv.cur_write_cnt[i] = sync[i];
                }
                schedule_work(&composer_priv.post2_cb_work);
            }
        }
    }
    if(releasefencefd[composer_priv.primary_disp] >= 0 && !hwc_pridisp_sync())
    {
	    wait_event_interruptible_timeout(composer_priv.commit_wq,
			    			   hwc_pridisp_sync(),
					    	   msecs_to_jiffies(16));
        /*for vsync shadow protected*/
        //usleep_range(100,200);
    }
    if(NULL != (void *)wb_ptr_data && !force_flip[0])
    {
         if(copy_from_user(&wb_data, (void __user *)wb_ptr_data, sizeof(struct disp_capture_info)))
        {
            printk(KERN_ERR"hwc copy_from_user write back data err.\n");
        }
        need_wb = 1;
    }
    for(i = 0; i < DISP_NUMS_SCREEN; i++)
    {
		if(releasefencefd[i] >= 0 && !force_flip[i])
		{
            if(copy_from_user(composer_priv.tmp_hw_lyr, (void __user *)hwc_layer_info[i], sizeof(struct disp_layer_config)*(i?8:16)))
            {
                printk(KERN_ERR"hwc copy_from_user disp_layer_config err.\n");
                ret = -1;
                continue;
            }
            dispc_gralloc_queue(composer_priv.tmp_hw_lyr, i, sync[i], (need_wb&&(i==0))?&wb_data:NULL);
		}
    }
commit_ok:
    if(composer_priv.b_no_output)
    {
        schedule_work(&composer_priv.post2_cb_work);
    }
	return ret;
}

static int hwc_check_wb(int disp,int fd)
{
    unsigned int pt_value = 0;
    pt_value = hwc_get_sync(disp,fd);
    if(composer_priv.check_wb[pt_value%WB_CHECK_SIZE].sync == pt_value)
    {
        return composer_priv.check_wb[pt_value%WB_CHECK_SIZE].success;
    }
    return 0;
}

#if defined(CONFIG_COMPAT)
static int hwc_compat_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = -EFAULT;
	if(DISP_HWC_COMMIT == cmd)
    {
        unsigned long *ubuffer;
        struct hwc_compat_ioctl_arg hwc_ctl;
        unsigned long addr_cmd;

        ubuffer = (unsigned long *)arg;
        if(copy_from_user(&hwc_ctl, (void __user *)ubuffer[1], sizeof(struct hwc_compat_ioctl_arg)))
        {
            printk(KERN_ERR"copy_from_user fail\n");
            return  -EFAULT;
		}
        addr_cmd = (unsigned long)hwc_ctl.arg;
        switch(hwc_ctl.cmd)
        {
            case HWC_IOCTL_FENCEFD:
                ret = hwc_fence_get((void*)addr_cmd);
            break;
            case HWC_IOCTL_COMMIT:
                ret = hwc_commit((void*)addr_cmd,1);
            break;
            case HWC_IOCTL_CKWB:
                get_user(ret,(int *)(addr_cmd));
                ret = hwc_check_wb(0,ret);
            break;
            case HWC_IOCTL_SETPRIDIP:
                get_user(ret,(int *)(addr_cmd));
                if(ret < DISP_NUMS_SCREEN)
                {
                    composer_priv.primary_disp = ret;
                }
            break;
            default:
                printk(KERN_ERR"hwc give a err iotcl.\n");
        }
	}
	return ret;
}
#endif

static int hwc_ioctl(unsigned int cmd, unsigned long arg)
{
    int ret = -EFAULT;
	if(DISP_HWC_COMMIT == cmd)
    {
        unsigned long *ubuffer;
        struct hwc_ioctl_arg hwc_ctl;
        ubuffer = (unsigned long *)arg;
        if(copy_from_user(&hwc_ctl, (void __user *)ubuffer[1], sizeof(struct hwc_ioctl_arg)))
        {
            printk(KERN_ERR"copy_from_user fail\n");
            return  -EFAULT;
		}
        switch(hwc_ctl.cmd)
        {
            case HWC_IOCTL_FENCEFD:
                ret = hwc_fence_get(hwc_ctl.arg);
            break;
            case HWC_IOCTL_COMMIT:
                ret = hwc_commit(hwc_ctl.arg,0);
            break;
            case HWC_IOCTL_CKWB:
                get_user(ret,(int *)(hwc_ctl.arg));
                ret = hwc_check_wb(0,ret);
            break;
            case HWC_IOCTL_SETPRIDIP:
                get_user(ret,(int *)(hwc_ctl.arg));
                if(ret < DISP_NUMS_SCREEN)
                {
                    composer_priv.primary_disp = ret;
                }
            break;
            default:
                printk(KERN_ERR"hwc give a err iotcl.\n");
        }
	}
	return ret;
}

static void disp_composer_proc(u32 sel)
{
    disp_drv_info       *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *disp_mgr = NULL;
    struct disp_capture *wb_back = NULL;
    struct write_back   *wb_status = NULL;
    if(sel<2)
    {
        if(sel == 0 && composer_priv.wb_status == 1)
        {
            disp_mgr = psg_disp_drv->mgr[sel];
            wb_back = disp_mgr->cptr;
            wb_status = &composer_priv.check_wb[composer_priv.cur_disp_cnt[sel]%WB_CHECK_SIZE];
            wb_status->sync = composer_priv.cur_disp_cnt[sel];
            wb_status->success = !wb_back->query(wb_back);
        }
        composer_priv.last_diplay_cnt[sel]= composer_priv.cur_disp_cnt[sel];
        composer_priv.cur_disp_cnt[sel] = composer_priv.cur_write_cnt[sel];
    }
	schedule_work(&composer_priv.post2_cb_work);
	return;
}

static void post2_cb(struct work_struct *work)
{
	mutex_lock(&composer_priv.sync_lock);
    imp_finish_cb(composer_priv.b_no_output);
	mutex_unlock(&composer_priv.sync_lock);
}

static int hwc_suspend(void)
{
	composer_priv.b_no_output = 1;
	schedule_work(&composer_priv.post2_cb_work);
	printk("%s\n", __func__);
	return 0;
}

static int hwc_resume(void)
{
	composer_priv.b_no_output = 0;
	printk("%s\n", __func__);
	return 0;
}

s32 composer_init(disp_drv_info *psg_disp_drv)
{
	memset(&composer_priv, 0x0, sizeof(struct composer_private_data));
	INIT_WORK(&composer_priv.post2_cb_work, post2_cb);
    mutex_init(&composer_priv.sync_lock);
    init_waitqueue_head(&composer_priv.commit_wq);

	disp_register_ioctl_func(DISP_HWC_COMMIT, hwc_ioctl);
#if defined(CONFIG_COMPAT)
    disp_register_compat_ioctl_func(DISP_HWC_COMMIT, hwc_compat_ioctl);
#endif
    disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
    composer_priv.tmp_hw_lyr = kzalloc(sizeof(struct disp_layer_config)*16,GFP_KERNEL);

    if(composer_priv.tmp_hw_lyr == NULL)
    {
        printk(KERN_ERR"hwc_composer init err when kzalloc,you need reboot\n");
    }
    composer_priv.psg_disp_drv = psg_disp_drv;
    return 0;
}
#else

#include <linux/sw_sync.h>
#include <linux/sync.h>
#include <linux/file.h>
#include <linux/slab.h>

#include "dev_disp.h"
#include <video/sunxi_display2.h>
static struct   mutex	gcommit_mutek;

#define DBG_TIME_TYPE 3
#define DBG_TIME_SIZE 100
#define DISP_NUMS_SCREEN 2

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
    enum disp_pixel_format       format;
    unsigned int            phys_addr;
}WriteBack_t;

typedef struct {
	int deviceid;
	int layer_num;
	struct disp_layer_config *hwclayer;
	int *fencefd;
	bool need_writeback;
	unsigned int ehancemode;
	unsigned int androidfrmnum;
	WriteBack_t *writeback_data;
}setup_dispc_data_t;

#ifdef CONFIG_COMPAT
typedef struct {
	int deviceid;
	int layer_num;
	compat_uptr_t hwclayer;
	compat_uptr_t fencefd;
	bool need_writeback;
	unsigned int ehancemode;
	unsigned int androidfrmnum;
	compat_uptr_t WriteBackdata;
} compat_setup_dispc_data_t;
#endif

typedef struct {
	setup_dispc_data_t hwc_data;
	struct disp_layer_config hwclayer[16];
	struct sync_fence *acquire_fence[16];
} disp_frame_t;

typedef struct
{
	struct list_head list;
	unsigned int framenumber;
	unsigned int androidfrmnum;
	disp_frame_t frame[2];
} dispc_data_list_t;

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
    setup_dispc_data_t      tmptransfer[DISP_NUMS_SCREEN];
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

int dispc_gralloc_queue(dispc_data_list_t *psDispcData, unsigned int framenuber);

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
static int mem_cache_destroy(void){
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
static int mem_cache_create(void){
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
static dispc_data_list_t* mem_cache_alloc(void){
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
    disp_frame_t *frame;
    struct list_head saved_list;
    int err;
	int i, disp;
	struct sync_fence **acquire_fence;

    mutex_lock(&(gcommit_mutek));
	if (composer_priv.b_no_output)
		goto _out;

    mutex_lock(&(composer_priv.update_regs_list_lock));
    list_replace_init(&composer_priv.update_regs_list, &saved_list);
    mutex_unlock(&(composer_priv.update_regs_list_lock));

	list_for_each_entry_safe(data, next, &saved_list, list) {
		list_del(&data->list);
		for (disp = 0; disp < DISP_NUMS_SCREEN; disp++) {
			frame = &data->frame[disp];
			acquire_fence = (struct sync_fence **)(&frame->acquire_fence[0]);
			for (i = 0; i < frame->hwc_data.layer_num; i++, acquire_fence++) {
				if (acquire_fence != NULL && *acquire_fence != NULL) {
					err = sync_fence_wait(*acquire_fence, 1000);
					sync_fence_put(*acquire_fence);
					if (err < 0) {
						printk("synce_fence_wait timeout AcquireFence:%p\n", *acquire_fence);
						sw_sync_timeline_inc(composer_priv.relseastimeline, 1);
						goto free;
					}
				}
			}
		}

		if (composer_priv.pause == 0)
			dispc_gralloc_queue(data, data->framenumber);

free:
		mem_cache_free(data);
	}

_out:
	mutex_unlock(&(gcommit_mutek));
}

static void correct_layer_index(struct disp_layer_config *l, int lcnt)
{
	unsigned int mask = 0x0000ffff;
	int index;
	int empty_layer_idx;
	int t;

	for (index = 0; index < lcnt; index++) {
		t = (l[index].channel << 2) + (l[index].layer_id);
		mask &= (~(1 << t));
	}

	empty_layer_idx = lcnt;
	for (index = 0; index < 16; index++) {
		if ((mask & (1 << index)) == 0)
			continue;

		l[empty_layer_idx].channel  = (index >> 2) & 0x03;
		l[empty_layer_idx].layer_id = (index >> 0) & 0x03;
		empty_layer_idx++;
	}
}

static void imp_finish_cb(bool force_all);

static int hwc_commit(setup_dispc_data_t *disp_data)
{
	int disp, i;
	dispc_data_list_t *disp_data_list;
	setup_dispc_data_t *setup;
	disp_frame_t *frame;
	struct disp_layer_config *phwclayer;
	struct sync_fence **fence;
	int release_fencefd = -1;
	int acquire_fencefd[16];
	struct sync_fence *release_fence;
	struct sync_pt *pt;
	int needskip = 0;
	struct disp_device *dispdev;

	disp_data_list = mem_cache_alloc();
	if (!disp_data_list) {
		printk(KERN_ERR "%s: %d, alloc data for disp_data_list fail.\n", __func__, __LINE__);
		release_fencefd = -1;
		goto __out;
	}

	for (disp = 0; disp < DISP_NUMS_SCREEN; disp++) {
		setup = &disp_data[disp];
		frame = &disp_data_list->frame[disp];

		if (!setup->layer_num)
			continue;

		/*
		 * Released the last layer buffer when all devices is close,
		 * or it will lead to fence timeout issue.
		 */
		dispdev = composer_priv.psg_disp_drv->mgr[disp]->device;
		if (dispdev && dispdev->is_enabled(dispdev) == 0) {
			needskip++;
			continue;
		} else {
			needskip--;
		}

		/* backup display config from userspace */
		memcpy(&frame->hwc_data, setup, sizeof(*setup));
		fence = &frame->acquire_fence[0];
		phwclayer = &frame->hwclayer[0];
		frame->hwc_data.hwclayer = phwclayer;

		if (copy_from_user(acquire_fencefd, (void __user *)setup->fencefd, sizeof(int) * setup->layer_num)) {
			printk(KERN_ERR "%s,%d: copy_from_user fail, %p\n", __func__, __LINE__, setup->fencefd);
			mem_cache_free(disp_data_list);
			goto __out;
		}

		if (copy_from_user(phwclayer, (void __user *)setup->hwclayer, sizeof(struct disp_layer_config) * setup->layer_num)) {
			printk(KERN_ERR "%s,%d: copy_from_user fail, %p\n", __func__, __LINE__, setup->hwclayer);
			mem_cache_free(disp_data_list);
			goto __out;
		}
		correct_layer_index(phwclayer, setup->layer_num);

		/* get fence object by acquire fencefd */
		for (i = 0; i < setup->layer_num; i++) {
			if (acquire_fencefd[i] != -1)
				fence[i] = sync_fence_fdget(acquire_fencefd[i]);
		}
	}

	if (needskip > 0) {
		composer_priv.Cur_Write_Cnt = composer_priv.timeline_max;

		mutex_lock(&composer_priv.runtime_lock);
		imp_finish_cb(1);
		mutex_unlock(&composer_priv.runtime_lock);
		mem_cache_free(disp_data_list);
		goto __out;
	}

	if (!composer_priv.b_no_output) {
		release_fencefd = get_unused_fd();
		if (release_fencefd < 0) {
                mem_cache_free(disp_data_list);
				release_fencefd = -1;
				goto __out;
		}

		composer_priv.timeline_max++;
		pt = sw_sync_pt_create(composer_priv.relseastimeline, composer_priv.timeline_max);
		release_fence = sync_fence_create("sunxi_display", pt);
		sync_fence_install(release_fence, release_fencefd);

		disp_data_list->framenumber = composer_priv.timeline_max;
		mutex_lock(&(composer_priv.update_regs_list_lock));
		list_add_tail(&disp_data_list->list, &composer_priv.update_regs_list);
		mutex_unlock(&(composer_priv.update_regs_list_lock));

		if (!composer_priv.pause) {
			queue_work(composer_priv.Display_commit_work, &composer_priv.commit_work);
		}
	} else {
		mem_cache_free(disp_data_list);
		flush_workqueue(composer_priv.Display_commit_work);
	}

__out:
	if (copy_to_user((void __user *)(disp_data->fencefd),
			&release_fencefd, sizeof(int))) {
		printk(KERN_ERR "copy_to_user fail\n");
		return  -EFAULT;
	}
	return 0;
}

static int hwc_commit_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;
	setup_dispc_data_t tmp[DISP_NUMS_SCREEN];
	unsigned long *ubuffer = (unsigned long *)arg;
	int total = ubuffer[2];

	if (DISP_HWC_COMMIT == cmd) {
		if (total > DISP_NUMS_SCREEN) {
			printk("%s: support only %d screen\n", __func__, DISP_NUMS_SCREEN);
			total = DISP_NUMS_SCREEN;
		}

		memset(tmp, 0, sizeof(setup_dispc_data_t) * DISP_NUMS_SCREEN);
		if (copy_from_user(tmp, (void __user *)ubuffer[1], sizeof(setup_dispc_data_t) * DISP_NUMS_SCREEN)) {
			printk("%s,%d: copy_from_user fail\n", __func__, __LINE__);
			return  -EFAULT;
		}
		ret = hwc_commit(tmp);
	}
	return ret;
}

#ifdef CONFIG_COMPAT
static int hwc_compat_commit_ioctl(unsigned int cmd, unsigned long arg)
{
	int i;
	int ret = -1;
	compat_setup_dispc_data_t compat_setup_data[DISP_NUMS_SCREEN];
	setup_dispc_data_t tmp[DISP_NUMS_SCREEN];
	unsigned long *ubuffer = (unsigned long *)arg;
	int total = ubuffer[2];

	if (DISP_HWC_COMMIT == cmd) {
		if (total > DISP_NUMS_SCREEN) {
			printk("%s: support only %d screen\n", __func__, DISP_NUMS_SCREEN);
			total = DISP_NUMS_SCREEN;
		}

		memset(tmp, 0, sizeof(setup_dispc_data_t) * DISP_NUMS_SCREEN);
		memset(compat_setup_data, 0, sizeof(compat_setup_dispc_data_t) * DISP_NUMS_SCREEN);
		if (copy_from_user(compat_setup_data, (void __user *)ubuffer[1], sizeof(compat_setup_dispc_data_t) * DISP_NUMS_SCREEN)) {
			printk("%s,%d: copy_from_user fail\n", __func__, __LINE__);
			return  -EFAULT;
		}

		for (i = 0; i < DISP_NUMS_SCREEN; i++) {
			tmp[i].deviceid       = compat_setup_data[i].deviceid;
			tmp[i].layer_num      = compat_setup_data[i].layer_num;
			tmp[i].need_writeback = compat_setup_data[i].need_writeback;
			tmp[i].ehancemode     = compat_setup_data[i].ehancemode;
			tmp[i].androidfrmnum  = compat_setup_data[i].androidfrmnum;
			tmp[i].hwclayer       = (void *)(unsigned long)compat_setup_data[i].hwclayer;
			tmp[i].fencefd        = (void *)(unsigned long)compat_setup_data[i].fencefd;
		}
		ret = hwc_commit(tmp);
	}
	return ret;
}
#endif

static int hwc_custom_ioctl(unsigned int cmd, unsigned long arg)
{
	int ret = -1;

	if (DISP_HWC_CUSTOM == cmd) {
		int disp;
		int flag;
		unsigned long *ubuffer = (unsigned long *)arg;
		struct disp_manager *disp_mgr = NULL;

		disp = ubuffer[0];
		flag = ubuffer[1];
		disp = ubuffer[0];
		if (disp < 0 || disp > DISP_NUMS_SCREEN) {
			printk(KERN_ERR "%s: invalid input params\n", __func__);
			return -EINVAL;
		}

		disp_mgr = composer_priv.psg_disp_drv->mgr[disp];
		if (!disp_mgr)
			return -EINVAL;

		switch (flag) {
		case HWC_CMD_GET_FPS:
			if (disp_mgr->device && disp_mgr->device->get_fps)
				ret = disp_mgr->device->get_fps(disp_mgr->device);
			break;
		case HWC_CMD_GET_CLK_RATE:
			if (disp_mgr->get_clk_rate)
				ret = disp_mgr->get_clk_rate(disp_mgr);
			break;
		default:
			printk(KERN_ERR "%s: unknown ioctl\n", __func__);
			break;
		}
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
int dispc_gralloc_queue(dispc_data_list_t *dispc_data, unsigned int framenuber)
{
    int disp;
    disp_drv_info *psg_disp_drv = composer_priv.psg_disp_drv;
    struct disp_manager *psmgr = NULL;
    struct disp_enhance *psenhance = NULL;
    struct disp_layer_config *psconfig;
	disp_frame_t *frame;
    disp = 0;

	while (disp < DISP_NUMS_SCREEN) {
		frame = &dispc_data->frame[disp];
		if (!frame->hwc_data.layer_num) {
            composer_priv.display_active[disp] = 0;
            disp++;
            continue;
        }

        psmgr = psg_disp_drv->mgr[disp];
		if (psmgr != NULL) {
			psenhance = psmgr->enhance;
			bsp_disp_shadow_protect(disp, true);
			if (frame->hwc_data.ehancemode) {
				if (frame->hwc_data.ehancemode != composer_priv.ehancemode[disp]) {
					switch (frame->hwc_data.ehancemode) {
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
				composer_priv.ehancemode[disp] = frame->hwc_data.ehancemode;
			} else {
				if (composer_priv.ehancemode[disp]) {
					psenhance->disable(psenhance);
					composer_priv.ehancemode[disp] = 0;
				}
			}

			psconfig = frame->hwc_data.hwclayer;
			psmgr->set_layer_config(psmgr, psconfig, disp ? 8 : 16);
			bsp_disp_shadow_protect(disp, false);
			if (composer_priv.display_active[disp] == 0) {
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

/*
 * hwc_suspend_prepare:
 * 1. skip all buffers when shutdown.
 * 2. close existed layer and clear de composition.
 */
#define HWC_SUSPEND_PREPARE
#ifdef HWC_SUSPEND_PREPARE
void hwc_suspend_prepare(void)
{
	int disp;
	dispc_data_list_t *disp_data = NULL;
	struct disp_manager *disp_mgr;

	mutex_lock(&gcommit_mutek);
	composer_priv.b_no_output = 1;
	mutex_unlock(&gcommit_mutek);

	disp_data = mem_cache_alloc();
	if (!disp_data)
		return;

	memset(disp_data, 0, sizeof(*disp_data));
	correct_layer_index(&disp_data->frame[0].hwclayer[0], 0);

	for (disp = 0; disp < DISP_NUMS_SCREEN; disp++) {
		disp_mgr = composer_priv.psg_disp_drv->mgr[disp];
		if (disp_mgr && disp_mgr->force_set_layer_config)
			disp_mgr->force_set_layer_config(disp_mgr,
							&disp_data->frame[0].hwclayer[0],
							disp ? 8 : 16);
	}
}
#else
void hwc_suspend_prepare(void) { return; }
#endif

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
	disp_register_ioctl_func(DISP_HWC_CUSTOM, hwc_custom_ioctl);
#ifdef CONFIG_COMPAT
	disp_register_compat_ioctl_func(DISP_HWC_COMMIT, hwc_compat_commit_ioctl);
#endif
    disp_register_sync_finish_proc(disp_composer_proc);
	disp_register_standby_func(hwc_suspend, hwc_resume);
	composer_priv.tmptransfer[0].hwclayer = kzalloc(sizeof(struct disp_layer_config) * 16, GFP_ATOMIC);
	composer_priv.tmptransfer[1].hwclayer = kzalloc(sizeof(struct disp_layer_config) * 16, GFP_ATOMIC);

    composer_priv.psg_disp_drv = psg_disp_drv;
    /* alloc mem_des struct pool, 48k */
    ret = mem_cache_create();
    if(ret != 0){
        printk("%s(%d) alloc frame buffer err!\n", __func__, __LINE__);
    }

  return 0;
}

#endif // #if !defined(CONFIG_HOMLET_PLATFORM)

#endif
