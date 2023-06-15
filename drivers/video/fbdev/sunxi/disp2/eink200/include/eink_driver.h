/*
 * Allwinner SoCs eink driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 *	File name   :       eink_driver.h
 *
 *	Description :       eink engine 2.0  struct declaration
 *
 *	History     :       2019/03/20 liuli   initial version
 *
 */
#ifndef _EINK_DRIVER_H_
#define _EINK_DRIVER_H_

#include <linux/platform_device.h>
#include "../../disp/de/include.h"
#include <video/sunxi_eink.h>
#include <linux/backlight.h>
#include <linux/pwm.h>

#if IS_ENABLED(CONFIG_TPS65185_VCOM)
#include <linux/ebc.h>
#endif

/* #define EINK_CACHE_MEM */
/* #define DEC_WAV_DEBUG */

#define EINK_MODULE_NAME "sunxi-eink"
#define DEFAULT_WAVEFORM_PATH "/vendor/default.awf"
#define DEFAULT_VCOM_PATH "/vendor/vcom.bin"
#define MAX_PATH_LEN 32
#define MAX_EINK_ENGINE 1
#define REQUST_PIPE_FAIL_MAX_CNT 200

#define POWER_STR_LEN 32
#define WAV_PATH_LEN 32
#define VCOM_PATH_LEN 64
#define EINK_GPIO_NUM 6
/* #define EINK_GPIO_POWER_NUM 3 */
#define WAVE_DATA_BUF_NUM 10
#define DEFAULT_GC_COUNTER 6

#define PRE_DECODE_NUM 5

#define DEFAULT_VCOM_VOLTAGE (-1550)

/* #define TIMING_FROM_MEM */

#define GET_UPDATE_MODE(mode)	((mode) & 0x0000ffff)
#define GET_UPDATE_INFO(mode)	((mode) & 0xffff0000)
#define IS_RECT_UPDATE(mode)    ((mode) & EINK_RECT_MODE)
#define IS_AUTO_MODE(mode)      ((mode) & EINK_AUTO_MODE)


/* default not define*/
/* #define OFFLINE_MULTI_MODE */
/* #define OFFLINE_SINGLE_MODE */
extern struct eink_driver_info g_eink_drvdata;

/* #define DRIVER_REMAP_WAVEFILE*/
struct eink_gpio_info {
	unsigned gpio;
	char name[32];
	int value;
};

/**
 * eink pwm info
 */
struct eink_pwm_info {
	//pwm device pointer
	struct pwm_device *dev;

	//pwm configs
	u32 channel;
	u32 polarity;
	u32 period_ns;
	u32 duty_ns;

	//true if pwm is enable
	bool enabled;
	//true if pwm is used
	bool used;
};

/**
 * eink backlight infomation
 */
struct eink_backlight_info {
	struct eink_pwm_info pwm_info;
	// lcd_bl_en is a pin which is used to switch backlight
	bool lcd_bl_en_used;
	struct eink_gpio_info lcd_bl_en;
	int lcd_bl_gpio_hdl;

	//linux backlight device pointer
	struct backlight_device *p_bl_dev;

	//backlight value 0 ~ 255;
	u32 backlight_bright;

	struct mutex backligt_lock;
};

struct eink_panel_info {
	u32	width;
	u32	height;
	u32	fresh_hz;
	u32	sdck;	/* reserved */
	u32	bit_num;	/* 4-bit or 5-bit */
	u32	data_len;	/* 8-data len or 16-data len */
	u32	eink_scan_mode; /* 0 signel 1 dual 2 quad*/
	u32	gray_level_cnt; /* 16 / 32 */
};

struct timing_info {
	//timing parameter
	u32	 lsl;
	u32	 lbl;
	u32	 ldl;
	u32	 lel;
	u32	 gdck_sta;
	u32	 lgonl;
	u32	 gdoe_start_line;

	u32	 fsl;
	u32	 fbl;
	u32	 fdl;
	u32	 fel;
	struct eink_backlight_info backlight;
};


struct init_para {
	void __iomem		*de_reg_base;
	void __iomem		*ee_reg_base;
	u32			de_irq_no;
	u32			ee_irq_no;
	struct clk		*de_clk;
	struct clk		*de_bus_clk;
	struct reset_control	*de_rst_clk;
	struct clk		*ee_clk;
	struct clk		*ee_bus_clk;
	struct reset_control	*ee_rst_clk;
	struct clk		*panel_clk;
	char			wav_path[32];
	char			vcom_path[64];
	char			eink_power[POWER_STR_LEN];
	char			eink_pin_power[POWER_STR_LEN];
	u32			power_used;
	struct eink_panel_info	panel_info;
	struct timing_info	timing_info;
	struct eink_gpio_info	eink_gpio[EINK_GPIO_NUM];
	u32			eink_gpio_used[EINK_GPIO_NUM];

};

struct eink_ion_mem {
	void *vaddr;
	unsigned long paddr;
	size_t size;
	struct ion_handle *handle;
};

struct eink_ion_list_node {
	struct eink_ion_mem mem;
	struct list_head node;
};

struct eink_ion_mgr {
	struct mutex mlock;
	struct ion_client *client;
	struct list_head ion_list;
};

struct eink_driver_info {
	dev_t			devt;
	struct cdev		*pcdev;
	struct class		*pclass;
	struct device		*eink_dev;
	struct device		*device;
	struct init_para	init_para;
	struct eink_manager	*eink_mgr;
	struct eink_ion_mgr	ion_mgr;
	u32			eink_dbg_info;
	struct mutex            lock;
};

struct upd_pic_cfg {
	unsigned long	    addr;
	u32		    upd_all_en;
	u32		    pitch;
	struct upd_pic_size size;
	struct upd_win	    upd_win;
	enum upd_pixel_fmt  out_fmt;
};

struct timing_buf_cfg {
	unsigned long	    addr;
	u32		    pitch;
	u32		    width;
	u32		    height;
};

enum  eink_bit_num {
	EINK_BIT_1 = 0x01,
	EINK_BIT_2 = 0x02,
	EINK_BIT_3 = 0x03,
	EINK_BIT_4 = 0x04,
	EINK_BIT_5 = 0x05
};

struct eink_panel_func {
	int (*cfg_open_flow)(void);
	int (*cfg_close_flow)(void);
};

typedef void (*EINK_PANEL_FUNC)(void);
struct eink_panel_function {
	EINK_PANEL_FUNC func;
	unsigned int delay;	/* ms */
};

#define MAX_SEQUENCES 7
struct eink_panel_flow {
	struct eink_panel_function func[MAX_SEQUENCES];
	unsigned int func_num;
};

struct timing_ctrl_manager {
	u32			enabled;
	void			*vaddr;
	void			*paddr;
	struct timing_info	info;
	u32                     eink_gpio_used[EINK_GPIO_NUM];
	spinlock_t              tlock;
#ifdef TIMING_FROM_MEM
	void			*timing_vaddr;
	unsigned long		timing_paddr;
	u32			timing_size;
#endif
	struct eink_gpio_info    eink_gpio[EINK_GPIO_NUM];
	int			gpio_hdl[EINK_GPIO_NUM];
	struct eink_panel_flow  open_flow;
	struct eink_panel_flow  close_flow;
	struct eink_panel_func	panel_func;

	s32 (*enable)(struct timing_ctrl_manager *mgr);
	int (*disable)(struct timing_ctrl_manager *mgr);
	s32 (*set_open_func)(struct timing_ctrl_manager *mgr, EINK_PANEL_FUNC func, u32 delay);
	s32 (*set_close_func)(struct timing_ctrl_manager *mgr, EINK_PANEL_FUNC func, u32 delay);
};

enum buf_state {
	FREE = 0x0,
	CAN_USED = 0x1,
	USED = 0x2,
};

struct img_node {
	u32		   upd_order;
	char               *kaddr;
	int		   update_master;
	unsigned long long coll_flag;
	bool		   upd_pic_fin;
	bool		   extra_flag;
	bool               force_fresh;
	struct eink_img	   *img;
	struct dmabuf_item *item;
	struct list_head   node;
};

struct buf_manager {
	u32		  upd_order;
	u32		  global_clean_cnt;
	u32		  width;
	u32		  height;
	u32		  buf_size;
	u32		  gray_level_cnt;
	struct img_node	  *processing_img_node;
	struct eink_img	  *last_img;
	struct mutex		mlock;
	struct list_head	img_free_list;
	struct list_head	img_used_list;
	struct list_head	img_collision_list;
	struct workqueue_struct	*coll_img_workqueue;
	struct work_struct	coll_handle_work;
	struct timespec		stimer;
	struct timespec		etimer;

	bool (*is_upd_list_empty)(struct buf_manager *buf_mgr);
	bool (*is_coll_list_empty)(struct buf_manager *buf_mgr);
	int (*add_img_to_coll_list)(struct buf_manager *buf_mgr, struct img_node *img_node);
	int (*remove_img_from_coll_list)(struct buf_manager *buf_mgr, struct img_node *img_node);
	bool (*check_upd_coll_state)(struct buf_manager *buf_mgr, u64 state);
	struct img_node *(*get_img_from_coll_list)(struct buf_manager *buf_mgr);
	struct img_node *(*get_img_from_upd_list)(struct buf_manager *buf_mgr);
	int (*queue_image)(struct buf_manager *buf_mgr, struct eink_upd_cfg *upd_cfg);
	int (*dequeue_image)(struct buf_manager *buf_mgr, struct img_node *img_node);
	int (*set_global_clean_counter)(struct buf_manager *buf_mgr, unsigned int cnt);
	int (*wait_for_newest_img_node)(struct buf_manager *buf_mgr);
	bool (*get_buf_msg)(struct buf_manager *buf_mgr);
	int (*reset_all)(struct buf_manager *buf_mgr);
	int (*get_free_buffer_slot)(struct buf_manager *buf_mgr, struct buf_slot *slot);
};

struct pipe_info_node {
	u32		  pipe_id;
	bool		  active_flag;
	enum upd_mode	  upd_mode;
	struct eink_img	  *img;
	unsigned long	  wav_paddr;
	unsigned long	  wav_vaddr;
	struct upd_win	  upd_win;
	u32		  dec_frame_cnt;
	u32		  fresh_frame_cnt; /* pipe_frm_cnt */
	u32		  total_frames;/*pipe_frm_num */
	struct list_head  node;
};

enum wv_buffer_state {
	WV_INIT_STATE = 0,
	WV_DECODE_STATE = 1,
	WV_READY_STATE = 2,
	WV_DISPLAY_STATE = 3,
};

struct wavedata_queue {
	unsigned int			head;
	unsigned int			tail;
	unsigned int			tmp_head;
	unsigned int			tmp_tail;
	unsigned int			wavedata_buf_size;
	void				*wavedata_vaddr[WAVE_DATA_BUF_NUM];
	void				*wavedata_paddr[WAVE_DATA_BUF_NUM];
	enum wv_buffer_state		buffer_state[WAVE_DATA_BUF_NUM];
	struct upd_pic_size		size;
	spinlock_t			slock;
};

struct pipe_manager {
	u32			max_pipe_cnt;
	struct eink_panel_info	panel_info;
	spinlock_t		list_lock;
	bool			enable_flag;
	void			*dec_wav_vaddr;
	void			*dec_wav_paddr;
#ifdef OFFLINE_SINGLE_MODE
	bool			ee_processing;
	struct wavedata_queue   wavedata_ring_buffer;
	struct workqueue_struct *dec_workqueue;
	struct workqueue_struct *edma_workqueue;
	struct work_struct	decode_ctrl_work;
	struct work_struct	edma_ctrl_work;
	u32			all_total_frames;
	u32			all_dec_frame_cnt;
	u32			all_fresh_frame_cnt;
	spinlock_t		frame_lock;
#endif
	struct list_head	pipe_free_list;
	struct list_head	pipe_used_list;
#ifdef DEC_WAV_DEBUG
	struct workqueue_struct *wav_dbg_workqueue;
	struct work_struct	wav_dbg_work;
	void *wav_wb_paddr;
	void *wav_wb_vaddr;
	u32 wav_len;
	int (*pipe_mgr_config_wb)(struct pipe_manager *mgr, struct pipe_info_node *info);
	int (*pipe_mgr_dump_wav_data)(struct pipe_manager *mgr);
#endif

	int (*pipe_mgr_enable)(struct pipe_manager *mgr);
	int (*pipe_mgr_disable)(void);
	u64 (*get_free_pipe_state)(struct pipe_manager *mgr);
	int (*request_pipe)(struct pipe_manager *mgr);
	int (*config_pipe)(struct pipe_manager *mgr, struct pipe_info_node info);
	int (*active_pipe)(struct pipe_manager *mgr, u32 pipe_id);
	void (*release_pipe)(struct pipe_manager *mgr, struct pipe_info_node *pipe);
	void (*reset_all_pipe)(struct pipe_manager *mgr);
};

struct index_manager {
	void	*rmi_vaddr;
	void	*rmi_paddr;
	u32	rmi_size;

	int (*set_rmi_addr)(struct index_manager *mgr);
	void (*reset_rmi)(struct index_manager *mgr, u32 bit_num);
};

struct eink_manager {
	u32				irq_num;
	u32				panel_temperature;
	u32				vcom_voltage;
	u32				vcom_init_flag;
	u32				clk_en_flag;
	char				vcom_path[64];

	volatile u32			upd_pic_accept_flag;
	wait_queue_head_t		upd_pic_accept_queue;
	volatile u32                    upd_coll_win_flag;
	wait_queue_head_t		upd_pic_queue;
	volatile u32                    upd_pic_fin_flag;
	bool				ee_finish;
	bool				enable_flag;
	struct mutex			enable_lock;
	struct clk			*ee_clk;
	struct clk			*ee_bus_clk;
	struct reset_control		*ee_rst_clk;
	struct clk			*panel_clk;
	struct eink_panel_info		panel_info;
	struct timing_info		timing_info;
	struct fmt_convert_manager	*convert_mgr;
	struct pipe_manager		*pipe_mgr;
	struct buf_manager		*buf_mgr;
	struct index_manager		*index_mgr;
	struct timing_ctrl_manager	*timing_ctrl_mgr;
	struct task_struct		*detect_fresh_task;
	struct task_struct		*get_temperature_task;
	char				wav_path[32];
	char				eink_pin_power[POWER_STR_LEN];
	struct regulator		*pin_regulator;
	bool				waveform_init_flag;
	struct timespec	stimer;
	struct timespec	etimer;
	struct timespec	acept_timer;
	struct timespec	upd_pic_timer;
	struct timespec	pipe_timer;

	unsigned int	shutdown_state;
	unsigned int	suspend_state;
	struct mutex	standby_lock;

	int (*eink_update)(struct eink_manager *mgr,
				struct eink_upd_cfg *upd_config);
	int (*eink_fmt_cvt_img)(struct disp_layer_config_inner *config,
				unsigned int layer_num,
				struct eink_img *last_img,
				struct eink_img *cur_img);
	int (*set_temperature)(struct eink_manager *mgr, u32 temp);
	u32 (*get_temperature)(struct eink_manager *mgr);
	s32 (*eink_set_global_clean_cnt)(struct eink_manager *mgr, u32 cnt);
	s32 (*eink_mgr_enable)(struct eink_manager *mgr);
	s32 (*eink_mgr_disable)(struct eink_manager *eink_mgr);
	s32 (*get_fps)(struct eink_manager *mgr);
	s32 (*dump_config)(struct eink_manager *mgr, char *buf);
	s32 (*get_clk_rate)(struct clk *device_clk);
	s32 (*get_resolution)(struct eink_manager *mgr, u32 *xres, u32 *yres);
};

extern s32 __disp_config2_transfer2inner(struct disp_layer_config_inner *config_inner,
				      struct disp_layer_config2 *config2);
extern struct disp_layer *disp_get_layer(u32 disp, u32 chn, u32 layer_id);
extern struct disp_manager *disp_get_layer_manager(u32 disp);
extern struct disp_layer *disp_get_layer(u32 disp, u32 chn, u32 layer_id);
extern void *request_buffer_for_decode(struct wavedata_queue *queue, void **vaddr);
extern void *request_buffer_for_display(struct wavedata_queue *queue);
extern int request_multi_frame_buffer(struct pipe_manager *mgr,
				struct timing_info *timing);
extern s32 clean_used_wavedata_buffer(struct wavedata_queue *queue);
extern s32 queue_wavedata_buffer(struct wavedata_queue *queue);

extern int eink_get_sys_config(struct init_para *para);
extern s32 fmt_convert_mgr_init(struct init_para *para);
extern int eink_mgr_init(struct init_para *para);
extern int buf_mgr_init(struct eink_manager *eink_mgr);
extern int index_mgr_init(struct eink_manager *eink_mgr);
extern int pipe_mgr_init(struct eink_manager *eink_mgr);
extern int timing_ctrl_mgr_init(struct init_para *para);
extern int waveform_mgr_init(const char *path, u32 bit_num);
extern int detect_fresh_thread(void *parg);
extern int regal_eink_process(struct eink_img *cur_img, struct eink_img *last_img);

extern struct pipe_manager *get_pipeline_manager(void);
extern struct eink_manager *get_eink_manager(void);
extern struct fmt_convert_manager *get_fmt_convert_mgr(unsigned int id);
extern struct timing_ctrl_manager *get_timing_ctrl_mgr(void);
extern int eink_get_wf_data(enum upd_mode mode, u32 temp, u32 *total_frames,
						unsigned long *wf_paddr, unsigned long *wf_vaddr);
extern int get_waveform_data(enum upd_mode mode, u32 temp, u32 *total_frames,
						unsigned long *wf_paddr, unsigned long *wf_vaddr);
extern int init_dec_wav_buffer(struct wavedata_queue *queue,
				struct eink_panel_info *info,
				struct timing_info *timing);
extern int eink_offline_enable(unsigned int en);
extern void upd_coll_win_irq_handler(struct work_struct *work);
extern int eink_clk_enable(struct eink_manager *mgr);
extern void eink_clk_disable(struct eink_manager *mgr);
extern int eink_get_temperature_thread(void *parg);
#if IS_ENABLED(CONFIG_TPS65185_VCOM)
extern int tps65185_vcom_set(int vcom_mv);
#endif
extern int eink_resume(struct device *dev);
extern int eink_suspend(struct device *dev);
#if IS_ENABLED(CONFIG_TPS65185_VCOM)
extern int tps65185_vcom_set(int vcom_mv);
#endif
#endif
