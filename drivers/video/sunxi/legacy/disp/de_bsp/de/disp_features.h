
#ifndef _DISP_FEATURES_H_
#define _DISP_FEATURES_H_

#include "disp_display_i.h"

/*basic data information definition*/
enum __disp_layer_feat {
	DISP_LAYER_FEAT_GLOBAL_ALPHA = 1 << 0,
	DISP_LAYER_FEAT_PIXEL_ALPHA = 1 << 1,
	DISP_LAYER_FEAT_GLOBAL_PIXEL_ALPHA = 1 << 2,
	DISP_LAYER_FEAT_PRE_MULT_ALPHA = 1 << 3,
	DISP_LAYER_FEAT_COLOR_KEY = 1 << 4,
	DISP_LAYER_FEAT_ZORDER = 1 << 5,
	DISP_LAYER_FEAT_POS = 1 << 6,
	DISP_LAYER_FEAT_3D = 1 << 7,
	DISP_LAYER_FEAT_SCALE = 1 << 8,
	DISP_LAYER_FEAT_DE_INTERLACE = 1 << 9,
	DISP_LAYER_FEAT_COLOR_ENHANCE = 1 << 10,
	DISP_LAYER_FEAT_DETAIL_ENHANCE = 1 << 11,
};

enum __disp_enhance_feat {
	DISP_ENHANCE_BRIGHT = 1 << 0,
	DISP_ENHANCE_SATURATION = 1 << 2,
	DISP_ENHANCE_CONTRAST = 1 << 3,
	DISP_ENHANCE_HUE = 1 << 4,
	DISP_ENHANCE_WINDOW = 1 << 5,
	DISP_ENHANCE_MODE = 1 << 6,
};

typedef struct {
	__u32 min;
	__u32 max;
} __disp_para_range;

struct sunxi_disp_features {
	const int num_screens;
	const int *num_layers;
	const int num_scalers;
	const __disp_para_range *layer_horizontal_resolution;//be module limit
	const __disp_para_range *layer_vertical_resolution;//be module limit
	const __disp_para_range *scaler_horizontal_resolution;//fe resolution supported
	const __disp_para_range *scaler_vertical_resolution;//fe resolution supported
	const int *scaler_horizontal_resolution_in_out_limit;//intput and output can't reach the limit at the same time
	const int *scaler_vertical_resolution_in_out_limit;//intput and output can't reach the limit at the same time
	const int num_wbs;
	const __disp_output_type_t *supported_output_types;
	//const enum sunxi_display_output_id *supported_outputs;
	//const enum sunxi_color_mode *supported_color_modes;
	const enum __disp_layer_feat *layer_feats;
	const enum __disp_layer_feat *scaler_layer_feats;
	const enum __disp_enhance_feat *enhance_feats;
	const int *de_flicker_support;
	const int *smart_backlight_support;
	const int *image_detail_enhance_support;
};

int bsp_disp_feat_get_num_screens(void);
int bsp_disp_feat_get_num_layers(__u32 screen_id);
int bsp_disp_feat_get_num_scalers(void);
__disp_output_type_t bsp_disp_feat_get_supported_output_types(__u32 screen_id);
//enum sunxi_disp_output_id sunxi_disp_feat_get_supported_outputs(__u32 screen_id);
enum __disp_layer_feat bsp_disp_feat_get_layer_feats(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
enum __disp_enhance_feat bsp_disp_feat_get_enhance_feats(__u32 screen_id);
int bsp_disp_feat_get_layer_horizontal_resolution_max(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_layer_vertical_resolution_max(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_layer_horizontal_resolution_min(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_layer_vertical_resolution_min(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index);
int bsp_disp_feat_get_de_flicker_support(__u32 screen_id);
int bsp_disp_feat_get_smart_backlight_support(__u32 screen_id);
int bsp_disp_feat_get_image_detail_enhance_support(__u32 screen_id);

int bsp_disp_feat_init(void);
#endif

