#include "disp_features.h"

static const struct sunxi_disp_features *sunxi_disp_current_features;

extern __s32 bsp_disp_get_screen_width(__u32 screen_id);
extern __s32 bsp_disp_get_screen_height(__u32 screen_id);
static const int sun8iw1p1_disp_num_layers[] = {
	/* DISP_SCREEN0 */
	4,
	4,
};

static const int sun8iw3p1_disp_num_layers[] = {
	/* DISP_SCREEN0 */
	4,
};

static const __disp_output_type_t sun8iw1p1_disp_supported_output_types[] = {
	/* DISP_SCREEN0 */
	DISP_OUTPUT_TYPE_LCD | DISP_OUTPUT_TYPE_HDMI,
	DISP_OUTPUT_TYPE_LCD | DISP_OUTPUT_TYPE_HDMI,
};

static const __disp_output_type_t sun8iw3p1_disp_supported_output_types[] = {
	/* DISP_SCREEN0 */
	DISP_OUTPUT_TYPE_LCD,
};

static const enum __disp_layer_feat sun8iw1p1_disp_layer_feats[] = {
	/* DISP_SCREEN0 */
	DISP_LAYER_FEAT_GLOBAL_ALPHA | DISP_LAYER_FEAT_POS |
	  DISP_LAYER_FEAT_PRE_MULT_ALPHA | DISP_LAYER_FEAT_PIXEL_ALPHA |
	  DISP_LAYER_FEAT_COLOR_KEY | DISP_LAYER_FEAT_ZORDER,

	/* DISP_SCREEN1 */
	DISP_LAYER_FEAT_GLOBAL_ALPHA | DISP_LAYER_FEAT_POS |
	  DISP_LAYER_FEAT_PRE_MULT_ALPHA | DISP_LAYER_FEAT_PIXEL_ALPHA |
	  DISP_LAYER_FEAT_COLOR_KEY | DISP_LAYER_FEAT_ZORDER,
};

static const enum __disp_layer_feat sun8iw1p1_disp_scaler_layer_feats[] = {
	/* DISP_SCREEN0 */
	DISP_LAYER_FEAT_SCALE | DISP_LAYER_FEAT_3D |
	  DISP_LAYER_FEAT_DE_INTERLACE | DISP_LAYER_FEAT_COLOR_ENHANCE |
	  DISP_LAYER_FEAT_DETAIL_ENHANCE,

	/* DISP_SCREEN1 */
	DISP_LAYER_FEAT_SCALE | DISP_LAYER_FEAT_3D |
	  DISP_LAYER_FEAT_DE_INTERLACE | DISP_LAYER_FEAT_COLOR_ENHANCE |
	  DISP_LAYER_FEAT_DETAIL_ENHANCE,
};

static const enum __disp_layer_feat sun8iw3p1_disp_layer_feats[] = {
	/* DISP_SCREEN0 */
	DISP_LAYER_FEAT_GLOBAL_ALPHA | DISP_LAYER_FEAT_POS |
	  DISP_LAYER_FEAT_PRE_MULT_ALPHA | DISP_LAYER_FEAT_PIXEL_ALPHA |
	  DISP_LAYER_FEAT_COLOR_KEY | DISP_LAYER_FEAT_ZORDER,
};

static const enum __disp_layer_feat sun8iw3p1_disp_scaler_layer_feats[] = {
	/* DISP_SCREEN0 */
	DISP_LAYER_FEAT_SCALE | DISP_LAYER_FEAT_COLOR_ENHANCE,
};

static const __disp_para_range sun8iw1p1_disp_layer_horizontal_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 2048},

  /* DISP_SCREEN1 */
  {0, 2048},
};

static const __disp_para_range sun8iw1p1_disp_layer_vertical_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 1536},

  /* DISP_SCREEN1 */
  {0, 1536},
};

static const __disp_para_range sun8iw1p1_disp_scaler_horizontal_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 8192},

  /* DISP_SCREEN1 */
  {0, 8192},
};

static const __disp_para_range sun8iw1p1_disp_scaler_vertical_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 8192},

  /* DISP_SCREEN1 */
  {0, 8192},
};

static const int sun8iw1p1_disp_scaler_horizontal_resolution_in_out_limit[] = {
  /* DISP_SCREEN0 */
  2560,

  /* DISP_SCREEN1 */
  2560,
};

static const int sun8iw1p1_disp_scaler_vertical_resolution_in_out_limit[] = {
  /* DISP_SCREEN0 */
  8192,

  /* DISP_SCREEN1 */
  8192,
};

static const __disp_para_range sun8iw3p1_disp_layer_horizontal_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 1366},
};

static const __disp_para_range sun8iw3p1_disp_layer_vertical_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 768},
};

static const __disp_para_range sun8iw3p1_disp_scaler_horizontal_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 8192},
};

static const __disp_para_range sun8iw3p1_disp_scaler_vertical_resolution[] = {
  /* DISP_SCREEN0 */
  {0, 8192},
};

static const int sun8iw3p1_disp_scaler_horizontal_resolution_in_out_limit[] = {
  /* DISP_SCREEN0 */
  1366,
};

static const int sun8iw3p1_disp_scaler_vertical_resolution_in_out_limit[] = {
  /* DISP_SCREEN0 */
  8192,
};

static const enum __disp_enhance_feat sun8iw1p1_disp_enhance_feats[] = {
  /* DISP_SCREEN0 */
  DISP_ENHANCE_BRIGHT | DISP_ENHANCE_SATURATION | DISP_ENHANCE_CONTRAST |
    DISP_ENHANCE_CONTRAST | DISP_ENHANCE_HUE |
    DISP_ENHANCE_WINDOW | DISP_ENHANCE_MODE,

  /* DISP_SCREEN1 */
  DISP_ENHANCE_BRIGHT | DISP_ENHANCE_SATURATION | DISP_ENHANCE_CONTRAST |
    DISP_ENHANCE_CONTRAST | DISP_ENHANCE_HUE |
    DISP_ENHANCE_WINDOW | DISP_ENHANCE_MODE,
};

static const enum __disp_enhance_feat sun8iw3p1_disp_enhance_feats[] = {
  /* DISP_SCREEN0 */
  DISP_ENHANCE_BRIGHT | DISP_ENHANCE_SATURATION | DISP_ENHANCE_CONTRAST |
    DISP_ENHANCE_CONTRAST | DISP_ENHANCE_HUE |
    DISP_ENHANCE_WINDOW | DISP_ENHANCE_MODE,
};

static const int sun8iw1p1_disp_de_flicker_support[] = {
  /* DISP_SCREEN0 */
  1,
  /* DISP_SCREEN1 */
  1,
};

static const int sun8iw3p1_disp_de_flicker_support[] = {
  /* DISP_SCREEN0 */
  0,
};

static const int sun8iw1p1_disp_smart_backlight_support[] = {
  /* DISP_SCREEN0 */
  1,
  /* DISP_SCREEN1 */
  1,
};

static const int sun8iw3p1_disp_smart_backlight_support[] = {
  /* DISP_SCREEN0 */
  1,
};

static const int sun8iw1p1_disp_image_detail_enhance_support[] = {
  /* DISP_SCREEN0 */
  1,
  /* DISP_SCREEN1 */
  1,
};

static const int sun8iw3p1_disp_image_detail_enhance_support[] = {
  /* DISP_SCREEN0 */
  0,
};

static const struct sunxi_disp_features sun8iw1p1_disp_features = {
	.num_screens = 2,
	.num_layers = sun8iw1p1_disp_num_layers,
	.num_scalers = 2,
	.layer_horizontal_resolution = sun8iw1p1_disp_layer_horizontal_resolution,
	.layer_vertical_resolution = sun8iw1p1_disp_layer_vertical_resolution,
	.scaler_horizontal_resolution = sun8iw1p1_disp_scaler_horizontal_resolution,
	.scaler_vertical_resolution = sun8iw1p1_disp_scaler_vertical_resolution,
	.scaler_horizontal_resolution_in_out_limit = sun8iw1p1_disp_scaler_horizontal_resolution_in_out_limit,
	.scaler_vertical_resolution_in_out_limit = sun8iw1p1_disp_scaler_vertical_resolution_in_out_limit,
	.num_wbs = 0,
	.supported_output_types = sun8iw1p1_disp_supported_output_types,
	//const enum sunxi_display_output_id *supported_outputs;
	//const enum sunxi_color_mode *supported_color_modes;
	.layer_feats = sun8iw1p1_disp_layer_feats,
	.scaler_layer_feats = sun8iw1p1_disp_scaler_layer_feats,
	.enhance_feats = sun8iw1p1_disp_enhance_feats,
	.de_flicker_support = sun8iw1p1_disp_de_flicker_support,
	.smart_backlight_support = sun8iw1p1_disp_smart_backlight_support,
	.image_detail_enhance_support = sun8iw1p1_disp_image_detail_enhance_support,
};

static const struct sunxi_disp_features sun8iw3p1_disp_features = {
	.num_screens = 1,
	.num_layers = sun8iw3p1_disp_num_layers,
	.num_scalers = 1,
	.layer_horizontal_resolution = sun8iw3p1_disp_layer_horizontal_resolution,
	.layer_vertical_resolution = sun8iw3p1_disp_layer_vertical_resolution,
	.scaler_horizontal_resolution = sun8iw3p1_disp_scaler_horizontal_resolution,
	.scaler_vertical_resolution = sun8iw3p1_disp_scaler_vertical_resolution,
	.scaler_horizontal_resolution_in_out_limit = sun8iw3p1_disp_scaler_horizontal_resolution_in_out_limit,
	.scaler_vertical_resolution_in_out_limit = sun8iw3p1_disp_scaler_vertical_resolution_in_out_limit,
	.num_wbs = 0,
	.supported_output_types = sun8iw3p1_disp_supported_output_types,
	//const enum sunxi_display_output_id *supported_outputs;
	//const enum sunxi_color_mode *supported_color_modes;
	.layer_feats = sun8iw3p1_disp_layer_feats,
	.scaler_layer_feats = sun8iw3p1_disp_scaler_layer_feats,
	.enhance_feats = sun8iw3p1_disp_enhance_feats,
	.de_flicker_support = sun8iw3p1_disp_de_flicker_support,
	.smart_backlight_support = sun8iw3p1_disp_smart_backlight_support,
	.image_detail_enhance_support = sun8iw3p1_disp_image_detail_enhance_support,
};

int bsp_disp_feat_get_num_screens(void)
{
	return sunxi_disp_current_features->num_screens;
}

int bsp_disp_feat_get_num_layers(__u32 screen_id)
{
	return sunxi_disp_current_features->num_layers[screen_id];
}

int bsp_disp_feat_get_num_scalers(void)
{
	return sunxi_disp_current_features->num_scalers;
}

__disp_output_type_t bsp_disp_feat_get_supported_output_types(__u32 screen_id)
{
	return sunxi_disp_current_features->supported_output_types[screen_id];
}

//enum sunxi_disp_output_id sunxi_disp_feat_get_supported_outputs(__u32 screen_id)
//{
//  return sunxi_disp_current_features->supported_outputs[screen_id];
//}

enum __disp_layer_feat bsp_disp_feat_get_layer_feats(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index)
{
	enum __disp_layer_feat layer_feats;

	layer_feats = sunxi_disp_current_features->layer_feats[screen_id];
	if(mode == DISP_LAYER_WORK_MODE_SCALER) {
		layer_feats |= sunxi_disp_current_features->scaler_layer_feats[scaler_index];
	}

	return layer_feats;
}

enum __disp_enhance_feat bsp_disp_feat_get_enhance_feats(__u32 screen_id)
{
	return sunxi_disp_current_features->enhance_feats[screen_id];
}

int bsp_disp_feat_get_layer_horizontal_resolution_max(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index)
{
	int resolution_max;

	resolution_max = sunxi_disp_current_features->layer_horizontal_resolution[screen_id].max;
	if(mode == DISP_LAYER_WORK_MODE_SCALER) {
		resolution_max = (resolution_max < sunxi_disp_current_features->scaler_horizontal_resolution[screen_id].max)?
		resolution_max : sunxi_disp_current_features->scaler_horizontal_resolution[screen_id].max;

		if(bsp_disp_get_screen_width(screen_id)
			>= sunxi_disp_current_features->scaler_horizontal_resolution_in_out_limit[scaler_index]) {
			resolution_max = sunxi_disp_current_features->scaler_horizontal_resolution_in_out_limit[scaler_index];
		}
	}

	return resolution_max;
}

int bsp_disp_feat_get_layer_vertical_resolution_max(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index)
{
	int resolution_max;

	resolution_max = sunxi_disp_current_features->layer_vertical_resolution[screen_id].max;
	if(mode == DISP_LAYER_WORK_MODE_SCALER) {
		resolution_max = (resolution_max < sunxi_disp_current_features->scaler_vertical_resolution[screen_id].max)?
		resolution_max : sunxi_disp_current_features->scaler_vertical_resolution[screen_id].max;

		if(bsp_disp_get_screen_height(screen_id)
		    >= sunxi_disp_current_features->scaler_vertical_resolution_in_out_limit[scaler_index]) {
			resolution_max = sunxi_disp_current_features->scaler_vertical_resolution_in_out_limit[scaler_index];
		}
	}

	return resolution_max;
}

int bsp_disp_feat_get_layer_horizontal_resolution_min(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index)
{
	int resolution_min;

	resolution_min = sunxi_disp_current_features->layer_horizontal_resolution[screen_id].min;
	if(mode == DISP_LAYER_WORK_MODE_SCALER) {
		resolution_min = (resolution_min > sunxi_disp_current_features->scaler_horizontal_resolution[screen_id].min)?
		resolution_min : sunxi_disp_current_features->scaler_horizontal_resolution[screen_id].min;
	}

	return resolution_min;
}

int bsp_disp_feat_get_layer_vertical_resolution_min(__u32 screen_id,
    __disp_layer_work_mode_t mode, __u32 scaler_index)
{
	int resolution_min;

	resolution_min = sunxi_disp_current_features->layer_vertical_resolution[screen_id].min;
	if(mode == DISP_LAYER_WORK_MODE_SCALER) {
		resolution_min = (resolution_min > sunxi_disp_current_features->layer_vertical_resolution[screen_id].min)?
		resolution_min : sunxi_disp_current_features->layer_vertical_resolution[screen_id].min;
	}

	return resolution_min;
}

int bsp_disp_feat_get_de_flicker_support(__u32 screen_id)
{
	return  sunxi_disp_current_features->de_flicker_support[screen_id];
}

int bsp_disp_feat_get_smart_backlight_support(__u32 screen_id)
{
	return  sunxi_disp_current_features->smart_backlight_support[screen_id];
}

int bsp_disp_feat_get_image_detail_enhance_support(__u32 screen_id)
{
	return  sunxi_disp_current_features->image_detail_enhance_support[screen_id];
}

int bsp_disp_feat_init(void)
{
#if defined CONFIG_ARCH_SUN8IW1P1
	sunxi_disp_current_features = &sun8iw1p1_disp_features;
#elif defined CONFIG_ARCH_SUN8IW3P1
	sunxi_disp_current_features = &sun8iw3p1_disp_features;
#else
	DE_WRN("[DISP] error, not support platform !\n");
#endif
	return 0;
}

