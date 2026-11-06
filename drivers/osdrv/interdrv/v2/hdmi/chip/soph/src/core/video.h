#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"
#include "edid/edid.h"
#include "edid/desc.h"

typedef enum {
	COLOR_DEPTH_INVALID = 0,
	COLOR_DEPTH_24 = 24,
	COLOR_DEPTH_30 = 30,
	COLOR_DEPTH_36 = 36,
	COLOR_DEPTH_48 = 48
} color_depth_t;

typedef enum {
	PIXEL_REPETITION_OFF = 0,
	PIXEL_REPETITION_1 = 1,
	PIXEL_REPETITION_2 = 2,
	PIXEL_REPETITION_3 = 3,
	PIXEL_REPETITION_4 = 4,
	PIXEL_REPETITION_5 = 5,
	PIXEL_REPETITION_6 = 6,
	PIXEL_REPETITION_7 = 7,
	PIXEL_REPETITION_8 = 8,
	PIXEL_REPETITION_9 = 9,
	PIXEL_REPETITION_10 = 10
} pixel_repetition_t;

typedef enum {
	HDMI_14 = 1,
	HDMI_20,
	MHL_24 ,
	MHL_PACKEDPIXEL
} operation_mode_t;

typedef enum {
	ENC_UNDEFINED = -1,
	RGB = 0,
	YCC444,
	YCC422,
	YCC420
} encoding_t;

typedef enum {
	UNDEFINED_COLORIMETRY = 0xFF,
	ITU601 = 1,
	ITU709,
	EXTENDED_COLORIMETRY
} colorimetry_t;

typedef enum {
	UNDEFINED_EXTCOLOR = 0xFF,
	XV_YCC601 = 0,
	XV_YCC709,
	S_YCC601,
	ADOBE_YCC601,
	ADOBE_RGB
} ext_colorimetry_t;

typedef enum {
	UNDEFINED_FORMAT = 0xFF,
	HDMI_NORMAL_FORMAT = 0,
	HDMI_EXT_RES_FORMAT = 1,
	HDMI_3D_FORMAT = 2,
} hdmi_video_format_t;

typedef enum {
	UNDEFINED_3D = 0xFF,
	FRAME_PACKING_3D = 0,
	TOP_AND_BOTTOM_3D = 6,
	SIDE_BY_SIDE_3D = 8
} hdmi_3d_structure_t;

typedef enum {
	UNDEFINED_EXTDATA_3D = 0,
} hdmi_3d_extdata_t;

typedef enum {
	UNDEFINED_HDMI_VIC = 0xFF,
} hdmi_vic_t;

typedef struct {
	video_mode_t mhdmi;
	encoding_t mencodingout;
	encoding_t mencodingin;
	u8 mcolor_resolution;
	u8 mpixel_repetition_factor;
	dtd_t mdtd;
	u8 mrgb_quantization_range;
	u8 mpixel_packing_defaultphase;
	u8 mcolorimetry;
	u8 mscaninfo;
	u8 mactive_format_aspect_ratio;
	u8 mnon_uniform_scaling;
	ext_colorimetry_t mext_colorimetry;
	u8 mcolorimetry_datablock;
	u8 mitcontent;
	u16 mend_top_bar;
	u16 mstart_bottom_bar;
	u16 mend_left_bar;
	u16 mstart_right_bar;
	u16 mcsc_filter;
	u16 mcsc_a[4];
	u16 mcsc_c[4];
	u16 mcsc_b[4];
	u16 mcsc_scale;
	u8 mhdmi_video_format;
	u8 m3d_structure;
	u8 m3d_ext_data;
	u8 mhdmi_vic;
	u8 mhdmi20;
} video_params_t;

int video_params_get_cea_vic_code(int hdmi_vic_code);

int video_params_get_hhdmi_vic_code(int cea_code);

/**
 * This method should be called before setting any parameters
 * to put the state of the strucutre to default
 * @param params pointer to the video parameters structure
 */
void video_params_reset(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients A
 */
u16 *video_params_get_csc_a(hdmi_tx_dev_t *dev, video_params_t * params);

void video_params_set_csc_a(hdmi_tx_dev_t *dev, video_params_t * params, u16 value[4]);

/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients B
 */
u16 *video_params_get_csc_b(hdmi_tx_dev_t *dev, video_params_t * params);

void video_params_set_csc_b(hdmi_tx_dev_t *dev, video_params_t * params, u16 value[4]);

/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients C
 */
u16 *video_params_get_csc_c(hdmi_tx_dev_t *dev, video_params_t * params);

void video_params_set_csc_c(hdmi_tx_dev_t *dev, video_params_t * params, u16 value[4]);

void video_params_set_csc_scale(hdmi_tx_dev_t *dev, video_params_t * params, u16 value);

/**
 * @param params pointer to the video parameters structure
 * @return Video PixelClock in [0.01 MHz]
 */
u32 video_params_get_pixel_clock(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return TMDS Clock in [0.01 MHz]
 */
u16 videoParams_GetTmdsClock(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return Ration clock x 100 (hdmi_tx_dev_t *dev, should be multiplied by x 0.01 afterwards)
 */
u32 video_params_get_ratio_clock(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return TRUE if csc is needed
 */
int video_params_is_color_space_conversion(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return TRUE if color space decimation is needed
 */
int video_params_is_color_space_decimation(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return TRUE if if video is interpolated
 */
int video_params_is_color_space_interpolation(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * @param params pointer to the video parameters structure
 * @return TRUE if if video has pixel repetition
 */
int video_params_is_pixel_repetition(hdmi_tx_dev_t *dev, video_params_t * params);

void video_params_update_csc_coefficients(hdmi_tx_dev_t *dev, video_params_t * params);

u8 videoParams_IsLimitedToYcc420(hdmi_tx_dev_t *dev, video_params_t * params);

void video_params_set_ycc420_support(hdmi_tx_dev_t *dev, dtd_t * params_dtd, short_video_desc_t * params_svd);

char * get_encoding_string(encoding_t encoding);

/**
 * Initializes and configures the video blocks to transmit a blue screen
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @param dataEnablePolarity data enable polarity (1 = enable, 0 not)
 * @return TRUE if successful
 */
int video_initialize(hdmi_tx_dev_t *dev, video_params_t * params,
			u8 data_enable_polarity);

/**
 * Configures the video blocks to do any video processing and to
 * transmit the video set up required by the user, allowing to
 * force video pixels (from the DEBUG pixels) to be transmitted
 * rather than the video stream being received.
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return TRUE if successful
 */
int video_configure(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * Set up color space converter to video requirements
 * (if there is any encoding type conversion or csc coefficients)
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return TRUE if successful
 */
int video_color_space_converter(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * Set up video packetizer which "packetizes" pixel transmission
 * (in deep colour mode, YCC422 mapping and pixel repetition)
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return TRUE if successful
 */
int video_video_packetizer(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * Set up video mapping and stuffing
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @return TRUE if successful
 */
int video_video_sampler(hdmi_tx_dev_t *dev, video_params_t * params);

/**
 * A test only method that is used for a test module
 * @param baseAddr Base Address of module
 * @param params VideoParams
 * @param dataEnablePolarity
 * @return TRUE if successful
 */
int video_VideoGenerator(hdmi_tx_dev_t *dev, video_params_t * params,
			 u8 data_enable_polarity);

void video_sampler_config(hdmi_tx_dev_t *dev, u8 map_code);
#endif	/* _VIDEO_H_ */
