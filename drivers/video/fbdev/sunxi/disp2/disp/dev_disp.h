/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DEV_DISP_H__
#define __DEV_DISP_H__

#include <linux/poll.h>
#include "dev_disp_debugfs.h"
#include "de/bsp_display.h"
#include "de/disp_display.h"
#include "de/disp_manager.h"
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/version.h>

#ifdef CONFIG_ION
#define FB_RESERVED_MEM
#endif

enum disp_standby_flags {
	DISPLAY_NORMAL = 0,
	DISPLAY_LIGHT_SLEEP = 1,
	DISPLAY_DEEP_SLEEP = 2,
	DISPLAY_BLANK = 4,
};

struct info_mm {
	void *info_base;	/* Virtual address */
	uintptr_t mem_start;	/* Start of frame buffer mem */
	/* (physical address) */
	u32 mem_len;		/* Length of frame buffer mem */
	struct disp_ion_mem *p_ion_mem;
};

struct proc_list {
	void (*proc)(u32 screen_id);
	struct list_head list;
};

struct ioctl_list {
	unsigned int cmd;
	int (*func)(unsigned int cmd, unsigned long arg);
	struct list_head list;
};

struct standby_cb_list {
	int (*suspend)(void);
	int (*resume)(void);
	struct list_head list;
};

struct disp_init_para {
	bool b_init;
	enum disp_init_mode disp_mode;

	/* for screen0/1/2 */
	enum disp_output_type output_type[8];
	unsigned int output_mode[8];
	enum disp_csc_type      output_format[DISP_SCREEN_NUM];
	enum disp_data_bits     output_bits[DISP_SCREEN_NUM];
	enum disp_eotf          output_eotf[DISP_SCREEN_NUM];
	enum disp_color_space   output_cs[DISP_SCREEN_NUM];
	enum disp_dvi_hdmi	    output_dvi_hdmi[DISP_SCREEN_NUM];
	enum disp_color_range	output_range[DISP_SCREEN_NUM];
	enum disp_scan_info		output_scan[DISP_SCREEN_NUM];
	unsigned int			output_aspect_ratio[DISP_SCREEN_NUM];
	bool using_device_config[DISP_SCREEN_NUM];
	unsigned int            reserve1;
	unsigned int            reserve2;

	/* for fb0/1/2 */
	unsigned int buffer_num[DISP_SCREEN_NUM];
	enum disp_pixel_format format[DISP_SCREEN_NUM];
	unsigned int fb_width[DISP_SCREEN_NUM];
	unsigned int fb_height[DISP_SCREEN_NUM];

	unsigned int chn_cfg_mode;
};

struct disp_ion_mgr {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
	struct ion_client *client;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	struct ion_handle *handle;
#else
	struct dma_buf *dma_buf;
#endif
	struct mutex mlock;
	struct list_head ion_list;
};


struct disp_ion_mem {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	struct ion_handle *handle;
#endif
	void *vaddr;
	size_t size;
	struct dmabuf_item *p_item;

};

struct disp_ion_list_node {
	struct list_head node;
	struct disp_ion_mem mem;
};

#define VSYNC_NUM 4
struct disp_vsync {
	struct task_struct *vsync_task[3];
	spinlock_t slock[DISP_SCREEN_NUM];
	int vsync_cur_line[DISP_SCREEN_NUM][VSYNC_NUM];
	ktime_t vsync_timestamp[DISP_SCREEN_NUM][VSYNC_NUM];
	u32 vsync_timestamp_head[DISP_SCREEN_NUM];
	u32 vsync_timestamp_tail[DISP_SCREEN_NUM];
	wait_queue_head_t vsync_waitq;
	bool vsync_read[DISP_SCREEN_NUM];

};
struct disp_drv_info {
	struct device *dev;
	struct device_node *node;
	uintptr_t reg_base[DISP_MOD_NUM];
	u32 irq_no[DISP_MOD_NUM];


	struct clk *clk_de[DE_NUM];
	struct clk *clk_bus_de[DE_NUM];
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct clk *clk_bus_extra;
#endif
	struct clk *clk_bus_dpss_top[DISP_DEVICE_NUM];
	struct clk *clk_tcon[DISP_DEVICE_NUM];
	struct clk *clk_bus_tcon[DISP_DEVICE_NUM];

	struct reset_control *rst_bus_de[DE_NUM];
#if defined(HAVE_DEVICE_COMMON_MODULE)
	struct reset_control *rst_bus_extra;
#endif
	struct reset_control *rst_bus_dpss_top[DISP_DEVICE_NUM];
	struct reset_control *rst_bus_tcon[DISP_DEVICE_NUM];
	struct reset_control *rst_bus_lvds[DEVICE_LVDS_NUM];
#if defined(SUPPORT_DSI)
	struct clk *clk_mipi_dsi[CLK_DSI_NUM];
	struct clk *clk_bus_mipi_dsi[CLK_DSI_NUM];
	struct reset_control *rst_bus_mipi_dsi[DEVICE_DSI_NUM];
#endif

	struct disp_vsync disp_vsync;
	struct disp_init_para disp_init;
	struct disp_manager *mgr[DISP_SCREEN_NUM];
	struct disp_eink_manager *eink_manager[1];
	struct proc_list sync_proc_list;
	struct proc_list sync_finish_proc_list;
	struct ioctl_list ioctl_extend_list;
	struct ioctl_list compat_ioctl_extend_list;
	struct standby_cb_list stb_cb_list;
	struct mutex mlock;
	struct work_struct resume_work[DISP_SCREEN_NUM];
	struct work_struct start_work;

	u32 exit_mode;		/* 0:clean all  1:disable interrupt */
	bool b_lcd_enabled[DISP_SCREEN_NUM];
	bool inited;		/* indicate driver if init */
	struct disp_bsp_init_para para;
#if defined(CONFIG_ION)
	struct disp_ion_mgr ion_mgr;
#endif
};

struct sunxi_disp_mod {
	enum disp_mod_id id;
	char name[32];
};

struct __fb_addr_para {
	uintptr_t fb_paddr;
	int fb_size;
};

#define DISP_RESOURCE(res_name, res_start, res_end, res_flags) \
{\
	.start = (int __force)res_start, \
	.end = (int __force)res_end, \
	.flags = res_flags, \
	.name = #res_name \
},

struct bmp_color_table_entry {
	u8 blue;
	u8 green;
	u8 red;
	u8 reserved;
} __packed;

struct lzma_header {
	char signature[4];
	u32 file_size;
	u32 original_file_size;
};

struct bmp_header {
	/* Header */
	char signature[2];
	u32 file_size;
	u32 reserved;
	u32 data_offset;
	/* InfoHeader */
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bit_count;
	u32 compression;
	u32 image_size;
	u32 x_pixels_per_m;
	u32 y_pixels_per_m;
	u32 colors_used;
	u32 colors_important;
	/* ColorTable */
} __packed;

struct bmp_pad_header {
	char data[2];		/* pading 2 byte */
	char signature[2];
	u32 file_size;
	u32 reserved;
	u32 data_offset;
	/* InfoHeader */
	u32 size;
	u32 width;
	u32 height;
	u16 planes;
	u16 bit_count;
	u32 compression;
	u32 image_size;
	u32 x_pixels_per_m;
	u32 y_pixels_per_m;
	u32 colors_used;
	u32 colors_important;
} __packed;

struct bmp_image {
	struct bmp_header header;
	/*
	 * We use a zero sized array just as a placeholder for variable
	 * sized array
	 */
	struct bmp_color_table_entry color_table[0];
};

struct sunxi_bmp_store {
	int x;
	int y;
	int bit;
	void *buffer;
};

int disp_draw_colorbar(u32 disp, u8 zorder);

void disp_set_suspend_output_type(u8 disp, u8 output_type);

int disp_open(struct inode *inode, struct file *file);
int disp_release(struct inode *inode, struct file *file);
ssize_t disp_read(struct file *file, char __user *buf, size_t count,
		  loff_t *ppos);
ssize_t disp_write(struct file *file, const char __user *buf, size_t count,
		   loff_t *ppos);
int disp_mmap(struct file *file, struct vm_area_struct *vma);
long disp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

int disp_suspend(struct device *dev);
int disp_resume(struct device *dev);
s32 disp_create_heap(u32 pHeapHead, u32 pHeapHeadPhy, u32 nHeapSize);
void *disp_malloc(u32 num_bytes, void *phy_addr);
void disp_free(void *virt_addr, void *phy_addr, u32 num_bytes);

extern s32 disp_register_sync_proc(void (*proc) (u32));
extern s32 disp_unregister_sync_proc(void (*proc) (u32));
extern s32 disp_register_sync_finish_proc(void (*proc) (u32));
extern s32 disp_unregister_sync_finish_proc(void (*proc) (u32));
extern s32 disp_register_ioctl_func(unsigned int cmd,
				    int (*proc)(unsigned int cmd,
						 unsigned long arg));
extern s32 disp_unregister_ioctl_func(unsigned int cmd);
extern s32 disp_register_compat_ioctl_func(unsigned int cmd,
					   int (*proc)(unsigned int cmd,
							unsigned long arg));
extern s32 disp_unregister_compat_ioctl_func(unsigned int cmd);
extern s32 disp_register_standby_func(int (*suspend) (void),
				      int (*resume)(void));
extern s32 disp_unregister_standby_func(int (*suspend) (void),
					int (*resume)(void));
extern s32 composer_init(struct disp_drv_info *psg_disp_drv);
extern unsigned int composer_dump(char *buf);
extern s32 disp_tv_register(struct disp_tv_func *func);
extern s32 disp_set_hdmi_detect(bool hpd);
s32 disp_set_edp_func(struct disp_tv_func *func);
unsigned int vsync_poll(struct file *file, poll_table *wait);
int bsp_disp_get_vsync_timestamp(int disp, int64_t *timestamp);

extern struct disp_drv_info g_disp_drv;

extern int sunxi_disp_get_source_ops(struct sunxi_disp_source_ops *src_ops);
extern s32 disp_lcd_open(u32 sel);
extern s32 disp_lcd_close(u32 sel);
extern s32 fb_init(struct platform_device *pdev);
extern s32 fb_exit(void);
extern unsigned long fb_get_address_info(u32 fb_id, u32 phy_virt_flag);
extern s32 fb_exit(void);
extern int lcd_init(void);

extern s32 pq_init(struct disp_bsp_init_para *para);

s32 disp_set_hdmi_func(struct disp_device_func *func);
s32 disp_set_vdpo_func(struct disp_tv_func *func);
s32 sunxi_get_fb_addr_para(struct __fb_addr_para *fb_addr_para);
s32 fb_draw_gray_pictures(char *base, u32 width, u32 height,
			  struct fb_var_screeninfo *var);
s32 drv_disp_vsync_event(u32 sel);
void DRV_disp_int_process(u32 sel);
s32 Display_set_fb_timming(u32 sel);
unsigned int disp_boot_para_parse(const char *name);
const char *disp_boot_para_parse_str(const char *name);
int disp_get_parameter_for_cmdlind(char *cmdline, char *name, char *value);
extern s32 bsp_disp_shadow_protect(u32 disp, bool protect);
extern int disp_attr_node_init(void);
extern int capture_module_init(void);
extern void capture_module_exit(void);

#if defined(SUPPORT_VDPO)
extern s32 disp_vdpo_set_config(struct disp_device *p_vdpo,
			 struct disp_vdpo_config *p_cfg);
#endif /*endif SUPPORT_VDPO */

struct disp_ion_mem *disp_ion_malloc(u32 num_bytes, void *phys_addr);
void disp_ion_free(void *virt_addr, void *phys_addr, u32 num_bytes);
void disp_ion_flush_cache(void *startAddr, int size);
int disp_get_ion_fd(struct disp_ion_mem *mem);
void *disp_get_phy_addr(struct disp_ion_mem *mem);
#endif
