#ifndef __DEV_DISP_H__
#define __DEV_DISP_H__

#include "drv_disp_i.h"

// 1M + 64M(ve) + 16M(fb)
#define FB_RESERVED_MEM


struct info_mm {
	void *info_base;	/* Virtual address */
	unsigned long mem_start;	/* Start of frame buffer mem */
				/* (physical address) */
	__u32 mem_len;			/* Length of frame buffer mem */
};

typedef struct
{
	struct device   *       dev;
	__u32                   reg_base[DISP_MOD_NUM];
	__u32                   reg_size[DISP_MOD_NUM];
	__u32                   irq[DISP_MOD_NUM];

	__disp_init_t           disp_init;

	__bool                  fb_enable[FB_MAX];
	__fb_mode_t             fb_mode[FB_MAX];
	__u32                   layer_hdl[FB_MAX][2];//[fb_id][0]:screen0 layer handle;[fb_id][1]:screen1 layer handle
	struct fb_info *        fbinfo[FB_MAX];
	__disp_fb_create_para_t fb_para[FB_MAX];
	wait_queue_head_t       wait[2];
	unsigned long           wait_count[2];
	struct timer_list       disp_timer[2];
	struct work_struct      vsync_work[2];
	struct work_struct      post2_cb_work;
	struct work_struct      resume_work[2];
	struct work_struct      start_work;
	struct work_struct      capture_work[2];
	ktime_t                 vsync_timestamp[2];
	ktime_t                 capture_timestamp[2];

	__u32                   ovl_mode;
	__u32	                  cb_w_conut;
	__u32	                  cb_r_conut;
	__u32                   cur_count;
	void                    (*cb_fn)(void *, int);
	void                    *cb_arg[10];
	__bool                  b_no_output;
	__u32                   reg_active[2];
	struct                  mutex	runtime_lock;
	int                     blank[2];
}fb_info_t;

typedef struct
{
	__u32               mid;
	__u32         	    used;
	__u32         	    status;
	__u32    		        exit_mode;//0:clean all  1:disable interrupt
	__bool              b_cache[2];
	__bool			        b_lcd_open[2];
}__disp_drv_t;

struct alloc_struct_t
{
	__u32 address;                      //申请内存的地址
	__u32 size;                         //分配的内存大小，用户实际得到的内存大小
	__u32 o_size;                       //用户申请的内存大小
	struct alloc_struct_t *next;
};

struct sunxi_disp_mod {
	__disp_mod_id_t id;
	char name[32];
};

struct __fb_addr_para
{
	int fb_paddr;
	int fb_size;
};

int disp_open(struct inode *inode, struct file *file);
int disp_release(struct inode *inode, struct file *file);
ssize_t disp_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t disp_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int disp_mmap(struct file *file, struct vm_area_struct * vma);
long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

__s32 disp_create_heap(__u32 pHeapHead, __u32 pHeapHeadPhy, __u32 nHeapSize);
void *disp_malloc(__u32 num_bytes, __u32 *phy_addr);
void  disp_free(void *virt_addr, void* phy_addr, __u32 num_bytes);

extern __s32 Display_Fb_Request(__u32 fb_id, __disp_fb_create_para_t *fb_para);
extern __s32 Display_Fb_Release(__u32 fb_id);
extern __s32 Display_Fb_get_para(__u32 fb_id, __disp_fb_create_para_t *fb_para);
extern __s32 Display_get_disp_init_para(__disp_init_t * init_para);

extern __s32 DRV_disp_int_process(__u32 sel);
extern __s32 DRV_disp_vsync_event(__u32 sel);
extern __s32 DRV_disp_take_effect_event(__u32 sel);

extern __s32 DRV_DISP_Init(void);
extern __s32 DRV_DISP_Exit(void);

extern fb_info_t g_fbi;

extern __disp_drv_t    g_disp_drv;
extern __u32 disp_print_cmd_level;
extern __u32 disp_cmd_print;

extern __s32 DRV_lcd_open(__u32 sel);
extern __s32 DRV_lcd_close(__u32 sel);
extern __s32 Fb_Init(void);
extern __s32 Fb_Exit(void);

extern int dispc_blank(int disp, int blank);
 __s32 disp_set_hdmi_func(__disp_hdmi_func * func);
__s32 disp_set_hdmi_hpd(__u32 hpd);
int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
int disp_suspend(struct platform_device *pdev, pm_message_t state);
int disp_resume(struct platform_device *pdev);
__s32 capture_event(__u32 sel);
int capture_module_init(void);
void  capture_module_exit(void);
__s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);
__s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var);
__s32 fb_draw_gray_pictures(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var);
int disp_attr_node_init(struct device  *display_dev);
int disp_attr_node_exit(void);

s32 disp_get_print_cmd_level(void);
s32 disp_set_print_cmd_level(u32 level);
s32 disp_get_cmd_print(void);
s32 disp_set_cmd_print(u32 cmd);

#endif
