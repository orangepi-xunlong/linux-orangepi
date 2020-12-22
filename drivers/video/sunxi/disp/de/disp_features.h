
#ifndef _DISP_FEATURES_H_
#define _DISP_FEATURES_H_

#include "bsp_display.h"
#include "disp_private.h"

struct disp_features {
	const s32 num_screens;
	const s32 *num_layers;
	const s32 num_scalers;
	const s32 num_captures;
	const u32 num_smart_backlights;
	const u32 num_color_managers;
	const disp_output_type *supported_output_types;
	const enum __disp_layer_feat *layer_feats;
	const enum __disp_layer_feat *scaler_layer_feats;
	const s32 *smart_backlight_support;
	const s32 *image_enhance_support;
	const s32 *smart_color_support;
	const s32 *capture_support;
};

s32 bsp_disp_feat_get_num_screens(void);
s32 bsp_disp_feat_get_num_layers(u32 screen_id);
s32 bsp_disp_feat_get_num_scalers(void);
s32 bsp_disp_feat_get_num_color_managers(void);
s32 bsp_disp_feat_get_num_smart_backlights(void);
disp_output_type bsp_disp_feat_get_supported_output_types(u32 screen_id);
//enum sunxi_disp_output_id sunxi_disp_feat_get_supported_outputs(u32 screen_id);
enum __disp_layer_feat bsp_disp_feat_get_layer_feats(u32 screen_id,
    disp_layer_mode mode, u32 scaler_index);
s32 bsp_disp_feat_get_smart_backlight_support(u32 screen_id);
s32 bsp_disp_feat_get_image_enhance_support(u32 screen_id);
s32 bsp_disp_feat_get_capture_support(u32 screen_id);
s32 bsp_disp_feat_get_num_captures(void);

s32 disp_init_feat(void);
#endif

