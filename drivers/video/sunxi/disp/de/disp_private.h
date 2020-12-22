#ifndef _DISP_PRIVATE_H_
#define _DISP_PRIVATE_H_
#include "bsp_display.h"
#include "disp_features.h"

#if defined(__LINUX_PLAT__)
#define DE_INF __inf
#define DE_MSG __msg
#define DE_WRN __wrn
#define DE_DBG __debug
#define OSAL_IRQ_RETURN IRQ_HANDLED
#else
#define DE_INF(msg...)
#define DE_MSG __msg
#define DE_WRN __wrn
#define DE_DBG __debug
#ifndef OSAL_IRQ_RETURN
#define OSAL_IRQ_RETURN DIS_SUCCESS
#endif
#endif

typedef enum
{
	/* for al layer */
	DISP_EVENT_NONE,
	DISP_EVENT_OUTPUT_SIZE,//disp_size{w,h}
	DISP_EVENT_OUTPUT_CSC,//(u32[2]:csc(0:rgb, 1:yuv for tv, 2:yuv for hdmi, 3: yuv for sat),color_range)
	DISP_EVENT_INTERLACE, //0:progressive, 1:interlace
	DISP_EVENT_DEU_CSC,   //(u32[2]:csc(0:rgb, 1:yuv), color_range)
	DISP_EVENT_SCALER_ENABLE,//u32[2]:enable, format
	DISP_EVENT_MANAGER_SYNC,
	DISP_EVENT_SAT_CSC,   //0:csc 0, 1 csc 3 

	/* for function modules */
	DISP_EVENT_OUTPUT_ENABLE = 0x40,//u32[2]: enable(0:dis,1:en); output_type
	DISP_EVENT_BACKLIGHT_UPDATE,//backlight
	DISP_EVENT_BACKLIGHT_DIMMING_UPDATE, //backlight dimming
}disp_event_type;

typedef enum
{
	DIS_SUCCESS=0,
	DIS_FAIL=-1,
	DIS_PARA_FAILED=-2,
	DIS_PRIO_ERROR=-3,
	DIS_OBJ_NOT_INITED=-4,
	DIS_NOT_SUPPORT=-5,
	DIS_NO_RES=-6,
	DIS_OBJ_COLLISION=-7,
	DIS_DEV_NOT_INITED=-8,
	DIS_DEV_SRAM_COLLISION=-9,
	DIS_TASK_ERROR = -10,
	DIS_PRIO_COLLSION = -11
}disp_return_value;

/*basic data information definition*/
enum __disp_layer_feat {
	DISP_LAYER_FEAT_GLOBAL_ALPHA        = 1 << 0,
	DISP_LAYER_FEAT_PIXEL_ALPHA         = 1 << 1,
	DISP_LAYER_FEAT_GLOBAL_PIXEL_ALPHA  = 1 << 2,
	DISP_LAYER_FEAT_PRE_MULT_ALPHA      = 1 << 3,
	DISP_LAYER_FEAT_COLOR_KEY           = 1 << 4,
	DISP_LAYER_FEAT_ZORDER              = 1 << 5,
	DISP_LAYER_FEAT_POS                 = 1 << 6,
	DISP_LAYER_FEAT_3D                  = 1 << 7,
	DISP_LAYER_FEAT_SCALE               = 1 << 8,
	DISP_LAYER_FEAT_DE_INTERLACE        = 1 << 9,
	DISP_LAYER_FEAT_COLOR_ENHANCE       = 1 << 10,
	DISP_LAYER_FEAT_DETAIL_ENHANCE      = 1 << 11,
};

typedef enum
{
	DISP_PIXEL_TYPE_RGB=0x0,
	DISP_PIXEL_TYPE_YUV=0x1,
}__disp_pixel_type_t;

typedef struct
{
	disp_fb_info             in_fb;
	disp_fb_info             out_fb;
}disp_scaler_info;

typedef struct
{
	u32 b_scaler_mode;
	u32 scaler_id;
}__disp_layer_extra_info_t;

typedef enum
{
    DIT_MODE_WEAVE = 0,
    DIT_MODE_BOB = 1,
    DIT_MODE_RESERVE = 2,
    DIT_MODE_MAF = 3,
}dit_mode_t;

typedef struct
{
	u32		pre_frame_addr_luma;
	u32		cur_frame_addr_luma;
	u32		pre_frame_addr_chroma;
	u32		cur_frame_addr_chroma;
	u32		pre_maf_flag_addr;
	u32		cur_maf_flag_addr;
	dit_mode_t	dit_mode;
	bool	first_field;
	bool	first_frame;
	bool	enable;
#if defined(__LINUX_PLAT__)
	spinlock_t	flag_lock;
#endif
}interlace_para_t;

typedef struct
{
		u32                     clk;
		u32                     clk_div;
		u32                     h_clk;
		u32                     clk_src;
		u32                     clk_div2;

		u32                     clk_p;
		u32                     clk_div_p;
		u32                     h_clk_p;
		u32                     clk_src_p;

		u32                     ahb_clk;
		u32                     h_ahb_clk;
		u32                     dram_clk;
		u32                     h_dram_clk;

		bool                    enabled;
}disp_clk_info_t;

struct disp_notifier_block {
	int (*notifier_call)(struct disp_notifier_block *, u32 event, u32 sel, void* data);
	struct list_head list;
};

struct disp_layer;
/* display private data structure defined */
struct disp_lcd {
	/* static fields */
	char *name;
	u32 channel_id;
	disp_output_type type;
	u8 *p_sw_init_flag;

	/*
	 * The following functions do not block:
	 *
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_lcd *lcd);
	s32 (*disable)(struct disp_lcd *lcd);
	s32 (*is_enabled)(struct disp_lcd *lcd);
	s32 (*is_used)(struct disp_lcd *lcd);
	s32 (*get_resolution)(struct disp_lcd *lcd, u32 *xres, u32 *yres);
	s32 (*get_physical_size)(struct disp_lcd *lcd, u32 *width, u32 *height);
	//s32 (*set_timing)(struct disp_lcd *lcd, disp_video_timing *timings);
	s32 (*get_timing)(struct disp_lcd *lcd, disp_video_timing *timings);
	s32 (*get_input_csc)(struct disp_lcd *lcd, disp_out_csc_type *csc_type);

	/* init: script init && clock init && pwm init && register irq
	 * exit: clock exit && unregister irq
	 */
	s32 (*init)(struct disp_lcd *lcd);
	s32 (*exit)(struct disp_lcd *lcd);

	s32 (*apply)(struct disp_lcd *lcd);
	/* power manager */
	s32 (*early_suspend)(struct disp_lcd *lcd);
	s32 (*late_resume)(struct disp_lcd *lcd);
	s32 (*suspend)(struct disp_lcd *lcd);
	s32 (*resume)(struct disp_lcd *lcd);

	s32 (*backlight_enable)(struct disp_lcd *lcd);
	s32 (*backlight_disable)(struct disp_lcd *lcd);
	s32 (*pwm_enable)(struct disp_lcd *lcd);
	s32 (*pwm_disable)(struct disp_lcd *lcd);
	s32 (*power_enable)(struct disp_lcd *lcd, u32 power_id);
	s32 (*power_disable)(struct disp_lcd *lcd, u32 power_id);
	s32 (*tcon_enable)(struct disp_lcd *lcd);
	s32 (*tcon_disable)(struct disp_lcd *lcd);
	s32 (*set_bright)(struct disp_lcd *lcd, u32 birhgt);
	s32 (*get_bright)(struct disp_lcd *lcd, u32 *birhgt);
	s32 (*set_bright_dimming)(struct disp_lcd *lcd, u32 dimming);
	disp_lcd_flow *(*get_open_flow)(struct disp_lcd *lcd);
	disp_lcd_flow *(*get_close_flow)(struct disp_lcd *lcd);
	s32 (*pre_enable)(struct disp_lcd *lcd);
	s32 (*post_enable)(struct disp_lcd *lcd);
	s32 (*pre_disable)(struct disp_lcd *lcd);
	s32 (*post_disable)(struct disp_lcd *lcd);
	s32 (*set_panel_func)(struct disp_lcd *lcd, disp_lcd_panel_fun * lcd_cfg);
	s32 (*get_panel_driver_name)(struct disp_lcd *lcd, char *name);
	s32 (*pin_cfg)(struct disp_lcd *lcd, u32 bon);
	s32 (*set_open_func)(struct disp_lcd* lcd, LCD_FUNC func, u32 delay);
	s32 (*set_close_func)(struct disp_lcd* lcd, LCD_FUNC func, u32 delay);
	s32 (*gpio_set_value)(struct disp_lcd* lcd, u32 io_index, u32 value);
	s32 (*gpio_set_direction)(struct disp_lcd *lcd, u32 io_index, u32 direction);//direction: intput(0), output(1)
	s32 (*get_panel_info)(struct disp_lcd *lcd, disp_panel_para *info);
	s32 (*get_tv_mode)(struct disp_lcd *lcd);
	s32 (*set_tv_mode)(struct disp_lcd *lcd, disp_tv_mode tv_mode);
};
extern struct disp_lcd* disp_get_lcd(u32 channel_id);

struct disp_hdmi {
	u32 channel_id;
	char* name;
	disp_output_type type;
	u8 *p_sw_init_flag;

	s32(*init)(struct disp_hdmi* hdmi);
	s32(*exit)(struct disp_hdmi* hdmi);
	s32 (*enable)(struct disp_hdmi* hdmi);
	s32 (*disable)(struct disp_hdmi* hdmi);
	s32 (*is_enabled)(struct disp_hdmi* hdmi);
	s32 (*set_mode)(struct disp_hdmi* hdmi, disp_tv_mode mode);
	s32 (*get_mode)(struct disp_hdmi* hdmi);
	s32 (*check_support_mode)(struct disp_hdmi* hdmi, u8 mode);
	s32 (*get_input_csc)(struct disp_hdmi* hdmi);
	s32 (*set_func)(struct disp_hdmi* hdmi, disp_hdmi_func* func);
	s32 (*suspend)(struct disp_hdmi* hdmi);
	s32 (*resume)(struct disp_hdmi* hdmi);
	s32 (*hdmi_get_HPD_status)(struct disp_hdmi* hdmi);
};

extern struct disp_hdmi* disp_get_hdmi(u32 channel_id);

struct disp_manager {
	/* static fields */
	char *name;
	u32 channel_id;
	//enum disp_manager_caps caps;
	const u32 num_layers;
	disp_output_type supported_outputs;
	disp_output_type output_type;
	u8 *p_sw_init_flag;

	struct list_head lyr_list;
	/*
	 * The following functions do not block:
	 *
	 * set_backcolor/get_backcolor
	 * set_color_key/get_color_key
	 * apply
	 * set_output_csc
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_manager *mgr);
	s32 (*disable)(struct disp_manager *mgr);
	s32 (*is_enabled)(struct disp_manager *mgr);
	s32 (*set_back_color)(struct disp_manager *mgr,
		disp_color_info *bk_color);
	s32 (*get_back_color)(struct disp_manager *mgr,
		disp_color_info *bk_color);
	s32 (*set_color_key)(struct disp_manager *mgr,
		disp_colorkey *ck);
	s32 (*get_color_key)(struct disp_manager *mgr,
		disp_colorkey *ck);
	s32 (*set_output_type)(struct disp_manager *mgr,
		disp_output_type output_type);
	s32 (*get_output_type)(struct disp_manager *mgr,
		disp_output_type *output_type);
	s32 (*set_outinterlace)(struct disp_manager *mgr, bool enable);
	s32 (*get_screen_size)(struct disp_manager *mgr, u32 *width, u32 *height);
	s32 (*set_screen_size)(struct disp_manager *mgr, u32 width, u32 height);
	s32 (*add_layer)(struct disp_manager *mgr, struct disp_layer *layer);
	/* init: clock init && reg init && register irq
	 * exit: clock exit && unregister irq
	 */
	s32 (*init)(struct disp_manager *mgr);
	s32 (*exit)(struct disp_manager *mgr);

	s32 (*apply)(struct disp_manager *mgr);
	s32 (*update_regs)(struct disp_manager *mgr);
	s32 (*force_update_regs)(struct disp_manager *mgr);
	s32 (*sync)(struct disp_manager *mgr);

	/* power manager */
	s32 (*early_suspend)(struct disp_manager *mgr);
	s32 (*late_resume)(struct disp_manager *mgr);
	s32 (*suspend)(struct disp_manager *mgr);
	s32 (*resume)(struct disp_manager *mgr);
};

extern struct disp_manager* disp_get_layer_manager(u32 channel_id);

struct disp_layer {
	/* static fields */
	char name[16];
	u32 channel_id;
	u32 layer_id;
	u8 *p_sw_init_flag;
	//enum disp_color_mode supported_modes;
	enum __disp_layer_feat caps;
	struct disp_manager *manager;
	struct list_head list;

	/*
	 * The following functions do not block:
	 *
	 * is_enabled
	 * set_layer_info
	 * get_layer_info
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_layer *layer);
	s32 (*disable)(struct disp_layer *layer);
	bool (*is_enabled)(struct disp_layer *layer);
	s32 (*set_info)(struct disp_layer *layer,
			disp_layer_info *info);
	s32 (*get_info)(struct disp_layer *layer,
			disp_layer_info *info);
	s32 (*is_support_caps)(struct disp_layer* layer, enum __disp_layer_feat caps);
	s32 (*is_support_format)(struct disp_layer* layer, enum __disp_layer_feat caps);
	s32 (*set_manager)(struct disp_layer* layer, struct disp_manager *mgr);

	/* init: NULL
	 * exit: NULL
	 */
	s32 (*init)(struct disp_layer *layer);
	s32 (*exit)(struct disp_layer *layer);

	s32 (*apply)(struct disp_layer *layer);
	s32 (*update_regs)(struct disp_layer *layer);
	s32 (*force_update_regs)(struct disp_layer *layer);
	s32 (*clear_regs)(struct disp_layer *layer);
	s32 (*sync)(struct disp_layer *layer);

	/* power manager */
	s32 (*early_suspend)(struct disp_layer* layer);
	s32 (*late_resume)(struct disp_layer* layer);
	s32 (*suspend)(struct disp_layer* layer);
	s32 (*resume)(struct disp_layer* layer);

	s32 (*get_frame_id)(struct disp_layer *layer);
	s32 (*deinterlace_cfg)(struct disp_layer *layer);
};

extern struct disp_layer* disp_get_layer(u32 screen_id, u32 layer_id);

//capture
struct disp_capture {
	char *name;
	u32 channel_id;
	struct disp_manager *manager;

	s32 (*set_manager)(struct disp_capture* cptr, struct disp_manager *mgr);
	s32 (*capture_screen)(struct disp_capture* cptr, disp_capture_para *para);
	s32 (*capture_screen_stop)(struct disp_capture* cptr);
	s32 (*capture_screen_get_buffer_id)(struct disp_capture* cptr);
	s32 (*sync)(struct disp_capture *cptr);
	s32 (*capture_screen_finished)(struct disp_capture* cptr);

	s32 (*init)(struct disp_capture *cptr);
	s32 (*exit)(struct disp_capture *cptr);
};

extern struct disp_capture* disp_get_capture(u32 screen_id);

struct disp_smbl {
	/* static fields */
	char *name;
	u32 channel_id;
	struct disp_manager *manager;

	/*
	 * The following functions do not block:
	 *
	 * is_enabled
	 * set_layer_info
	 * get_layer_info
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_smbl *smbl);
	s32 (*disable)(struct disp_smbl *smbl);
	bool (*is_enabled)(struct disp_smbl *smbl);
	s32 (*set_manager)(struct disp_smbl* smbl, struct disp_manager *mgr);

	/* init: NULL
	 * exit: NULL
	 */
	s32 (*init)(struct disp_smbl *smbl);
	s32 (*exit)(struct disp_smbl *smbl);

	s32 (*apply)(struct disp_smbl *smbl);
	s32 (*update_regs)(struct disp_smbl *smbl);
	s32 (*force_update_regs)(struct disp_smbl *smbl);
	s32 (*sync)(struct disp_smbl *smbl);

	/* power manager */
	s32 (*early_suspend)(struct disp_smbl* smbl);
	s32 (*late_resume)(struct disp_smbl* smbl);
	s32 (*suspend)(struct disp_smbl* smbl);
	s32 (*resume)(struct disp_smbl* smbl);

	s32 (*set_window)(struct disp_smbl* smbl, disp_window *window);
	s32 (*get_window)(struct disp_smbl* smbl, disp_window *window);
};

extern struct disp_smbl* disp_get_smbl(u32 screen_id);

struct disp_smcl {
	/* static fields */
	char *name;
	u32 channel_id;
	struct disp_manager *manager;

	/*
	 * The following functions do not block:
	 *
	 * is_enabled
	 * set_layer_info
	 * get_layer_info
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_smcl *smcl);
	s32 (*disable)(struct disp_smcl *smcl);
	bool (*is_enabled)(struct disp_smcl *smcl);
	s32 (*set_manager)(struct disp_smcl* smcl, struct disp_manager *mgr);

	/* init: NULL
	 * exit: NULL
	 */
	s32 (*init)(struct disp_smcl *smcl);
	s32 (*exit)(struct disp_smcl *smcl);

	s32 (*apply)(struct disp_smcl *smcl);
	s32 (*update_regs)(struct disp_smcl *smcl);
	s32 (*force_update_regs)(struct disp_smcl *smcl);
	s32 (*sync)(struct disp_smcl *smcl);

	/* power manager */
	s32 (*early_suspend)(struct disp_smcl* smcl);
	s32 (*late_resume)(struct disp_smcl* smcl);
	s32 (*suspend)(struct disp_smcl* smcl);
	s32 (*resume)(struct disp_smcl* smcl);

	s32 (*set_bright)(struct disp_smcl* smcl, u32 val);
	s32 (*set_saturation)(struct disp_smcl* smcl, u32 val);
	s32 (*set_contrast)(struct disp_smcl* smcl, u32 val);
	s32 (*set_hue)(struct disp_smcl* smcl, u32 val);
	s32 (*set_mode)(struct disp_smcl* smcl, u32 val);
	s32 (*set_window)(struct disp_smcl* smcl, disp_window *window);
	s32 (*get_bright)(struct disp_smcl* smcl);
	s32 (*get_saturation)(struct disp_smcl* smcl);
	s32 (*get_contrast)(struct disp_smcl* smcl);
	s32 (*get_hue)(struct disp_smcl* smcl);
	s32 (*get_mode)(struct disp_smcl* smcl);
	s32 (*get_window)(struct disp_smcl* smcl, disp_window *window);
};

extern struct disp_smcl* disp_get_smcl(u32 screen_id);

struct disp_cursor {
	/* static fields */
	char *name;
	u32 channel_id;
	struct disp_manager *manager;

	/*
	 * The following functions do not block:
	 *
	 * is_enabled
	 * set_layer_info
	 * get_layer_info
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	s32 (*enable)(struct disp_cursor *cursor);
	s32 (*disable)(struct disp_cursor *cursor);
	bool (*is_enabled)(struct disp_cursor *cursor);
	s32 (*set_manager)(struct disp_cursor* cursor, struct disp_manager *mgr);

	/* init: NULL
	 * exit: NULL
	 */
	s32 (*init)(struct disp_cursor *cursor);
	s32 (*exit)(struct disp_cursor *cursor);

	s32 (*apply)(struct disp_cursor *cursor);
	s32 (*update_regs)(struct disp_cursor *cursor);
	s32 (*force_update_regs)(struct disp_cursor *cursor);
	s32 (*sync)(struct disp_cursor *cursor);

	/* power manager */
	s32 (*early_suspend)(struct disp_cursor* cursor);
	s32 (*late_resume)(struct disp_cursor* cursor);
	s32 (*suspend)(struct disp_cursor* cursor);
	s32 (*resume)(struct disp_cursor* cursor);

	s32 (*set_pos)(struct disp_cursor* cursor, disp_position *pos);
	s32 (*get_pos)(struct disp_cursor* cursor, disp_position *pos);
	s32 (*set_fb)(struct disp_cursor* cursor, disp_cursor_fb *fb);
	s32 (*set_palette)(struct disp_cursor* cursor, void *palette, u32 offset, u32 palette_size);
};

extern struct disp_cursor* disp_get_cursor(u32 screen_id);

extern s32 bsp_disp_delay_ms(u32 ms);

extern s32 bsp_disp_delay_us(u32 us);

s32 disp_notifier_init(void);
s32 disp_notifier_register(struct disp_notifier_block *nb);
s32 disp_notifier_unregister(struct disp_notifier_block *nb);
s32 disp_notifier_call_chain(u32 event, u32 sel, void *v);

#if defined (CONFIG_ARCH_SUN8IW5P1)
#include "./lowlevel_sun8iw5/disp_al.h"
#elif defined (CONFIG_ARCH_SUN9IW1P1)
#include "./lowlevel_sun9iw1/disp_al.h"
#endif

#include "disp_lcd.h"
#include "disp_hdmi.h"
#include "disp_manager.h"
#include "disp_layer.h"
#include "disp_capture.h"
#include "disp_smart_color.h"
#include "disp_smart_backlight.h"
#include "disp_cursor.h"

#endif

