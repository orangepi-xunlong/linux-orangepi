/*
 * A V4L2 driver for sensor_spi cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"

/*
**#include "sensor_helper.h"
*/

MODULE_AUTHOR("zw");
MODULE_DESCRIPTION("A low-level driver for sensor_spi sensors");
MODULE_LICENSE("GPL");

#define AF_WIN_NEW_COORD

#define DEV_DBG_EN    0
#if (DEV_DBG_EN == 1)
#define sensor_dbg(x, arg...) printk("[SENSOR_SPI]"x, ##arg)
#else
#define sensor_dbg(x, arg...)
#endif
#define sensor_err(x, arg...) printk("[SENSOR_SPI]"x, ##arg)
#define sensor_print(x, arg...) printk("[SENSOR_SPI]"x, ##arg)

#define MCLK              			(24*1000*1000)
#define VREF_POL          			V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          			V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           			V4L2_MBUS_PCLK_SAMPLE_RISING

#define SENSOR_NAME "sensor_spi"
/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 	30

static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;	/* coming later */

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

#define REG_DLY  0xff

struct regval_list {
	u8 addr;
	u8 data;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_uxga_regs[] = {

};
static struct regval_list sensor_720p_regs[] = {

};

static struct regval_list sensor_svga_regs[] = {

};

/*
 * The white balance settings
 * Here only tune the R G B channel gain.
 * The white balance enalbe bit is modified in sensor_s_autowb and sensor_s_wb
 */

static struct regval_list sensor_wb_manual[] = {
};

static struct regval_list sensor_wb_auto_regs[] = {
};

static struct regval_list sensor_wb_incandescence_regs[] = {
};

static struct regval_list sensor_wb_fluorescent_regs[] = {
};

static struct regval_list sensor_wb_tungsten_regs[] = {
};

static struct regval_list sensor_wb_horizon[] = {
};

static struct regval_list sensor_wb_daylight_regs[] = {
};

static struct regval_list sensor_wb_flash[] = {
};

static struct regval_list sensor_wb_cloud_regs[] = {
};

static struct regval_list sensor_wb_shade[] = {
};

static struct cfg_array sensor_wb[] = {
	{
	 .regs = sensor_wb_manual,
	 .size = ARRAY_SIZE(sensor_wb_manual),
	 },
	{
	 .regs = sensor_wb_auto_regs,
	 .size = ARRAY_SIZE(sensor_wb_auto_regs),
	 },
	{
	 .regs = sensor_wb_incandescence_regs,
	 .size = ARRAY_SIZE(sensor_wb_incandescence_regs),
	 },
	{
	 .regs = sensor_wb_fluorescent_regs,
	 .size = ARRAY_SIZE(sensor_wb_fluorescent_regs),
	 },
	{
	 .regs = sensor_wb_tungsten_regs,
	 .size = ARRAY_SIZE(sensor_wb_tungsten_regs),
	 },
	{
	 .regs = sensor_wb_horizon,
	 .size = ARRAY_SIZE(sensor_wb_horizon),
	 },
	{
	 .regs = sensor_wb_daylight_regs,
	 .size = ARRAY_SIZE(sensor_wb_daylight_regs),
	 },
	{
	 .regs = sensor_wb_flash,
	 .size = ARRAY_SIZE(sensor_wb_flash),
	 },
	{
	 .regs = sensor_wb_cloud_regs,
	 .size = ARRAY_SIZE(sensor_wb_cloud_regs),
	 },
	{
	 .regs = sensor_wb_shade,
	 .size = ARRAY_SIZE(sensor_wb_shade),
	 },
};

/*
 * The color effect settings
 */
static struct regval_list sensor_colorfx_none_regs[] = {
};

static struct regval_list sensor_colorfx_bw_regs[] = {
};

static struct regval_list sensor_colorfx_sepia_regs[] = {
};

static struct regval_list sensor_colorfx_negative_regs[] = {
};

static struct regval_list sensor_colorfx_emboss_regs[] = {
};

static struct regval_list sensor_colorfx_sketch_regs[] = {
};

static struct regval_list sensor_colorfx_sky_blue_regs[] = {
};

static struct regval_list sensor_colorfx_grass_green_regs[] = {
};

static struct regval_list sensor_colorfx_skin_whiten_regs[] = {
};

static struct regval_list sensor_colorfx_vivid_regs[] = {
};

static struct regval_list sensor_colorfx_aqua_regs[] = {
};

static struct regval_list sensor_colorfx_art_freeze_regs[] = {
};

static struct regval_list sensor_colorfx_silhouette_regs[] = {
};

static struct regval_list sensor_colorfx_solarization_regs[] = {
};

static struct regval_list sensor_colorfx_antique_regs[] = {
};

static struct regval_list sensor_colorfx_set_cbcr_regs[] = {
};

static struct cfg_array sensor_colorfx[] = {
	{
	 .regs = sensor_colorfx_none_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_none_regs),
	 },
	{
	 .regs = sensor_colorfx_bw_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_bw_regs),
	 },
	{
	 .regs = sensor_colorfx_sepia_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sepia_regs),
	 },
	{
	 .regs = sensor_colorfx_negative_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_negative_regs),
	 },
	{
	 .regs = sensor_colorfx_emboss_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_emboss_regs),
	 },
	{
	 .regs = sensor_colorfx_sketch_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sketch_regs),
	 },
	{
	 .regs = sensor_colorfx_sky_blue_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_sky_blue_regs),
	 },
	{
	 .regs = sensor_colorfx_grass_green_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_grass_green_regs),
	 },
	{
	 .regs = sensor_colorfx_skin_whiten_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_skin_whiten_regs),
	 },
	{
	 .regs = sensor_colorfx_vivid_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_vivid_regs),
	 },
	{
	 .regs = sensor_colorfx_aqua_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_aqua_regs),
	 },
	{
	 .regs = sensor_colorfx_art_freeze_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_art_freeze_regs),
	 },
	{
	 .regs = sensor_colorfx_silhouette_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_silhouette_regs),
	 },
	{
	 .regs = sensor_colorfx_solarization_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_solarization_regs),
	 },
	{
	 .regs = sensor_colorfx_antique_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_antique_regs),
	 },
	{
	 .regs = sensor_colorfx_set_cbcr_regs,
	 .size = ARRAY_SIZE(sensor_colorfx_set_cbcr_regs),
	 },
};

/*
 * The brightness setttings
 */
static struct regval_list sensor_brightness_neg4_regs[] = {

};

static struct regval_list sensor_brightness_neg3_regs[] = {
};

static struct regval_list sensor_brightness_neg2_regs[] = {
};

static struct regval_list sensor_brightness_neg1_regs[] = {
};

static struct regval_list sensor_brightness_zero_regs[] = {
};

static struct regval_list sensor_brightness_pos1_regs[] = {
};

static struct regval_list sensor_brightness_pos2_regs[] = {
};

static struct regval_list sensor_brightness_pos3_regs[] = {
};

static struct regval_list sensor_brightness_pos4_regs[] = {

};

static struct cfg_array sensor_brightness[] = {
	{
	 .regs = sensor_brightness_neg4_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg4_regs),
	 },
	{
	 .regs = sensor_brightness_neg3_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg3_regs),
	 },
	{
	 .regs = sensor_brightness_neg2_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg2_regs),
	 },
	{
	 .regs = sensor_brightness_neg1_regs,
	 .size = ARRAY_SIZE(sensor_brightness_neg1_regs),
	 },
	{
	 .regs = sensor_brightness_zero_regs,
	 .size = ARRAY_SIZE(sensor_brightness_zero_regs),
	 },
	{
	 .regs = sensor_brightness_pos1_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos1_regs),
	 },
	{
	 .regs = sensor_brightness_pos2_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos2_regs),
	 },
	{
	 .regs = sensor_brightness_pos3_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos3_regs),
	 },
	{
	 .regs = sensor_brightness_pos4_regs,
	 .size = ARRAY_SIZE(sensor_brightness_pos4_regs),
	 },
};

/*
 * The contrast setttings
 */
static struct regval_list sensor_contrast_neg4_regs[] = {

};

static struct regval_list sensor_contrast_neg3_regs[] = {
};

static struct regval_list sensor_contrast_neg2_regs[] = {
};

static struct regval_list sensor_contrast_neg1_regs[] = {
};

static struct regval_list sensor_contrast_zero_regs[] = {
};

static struct regval_list sensor_contrast_pos1_regs[] = {
};

static struct regval_list sensor_contrast_pos2_regs[] = {
};

static struct regval_list sensor_contrast_pos3_regs[] = {
};

static struct regval_list sensor_contrast_pos4_regs[] = {

};

static struct cfg_array sensor_contrast[] = {
	{
	 .regs = sensor_contrast_neg4_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg4_regs),
	 },
	{
	 .regs = sensor_contrast_neg3_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg3_regs),
	 },
	{
	 .regs = sensor_contrast_neg2_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg2_regs),
	 },
	{
	 .regs = sensor_contrast_neg1_regs,
	 .size = ARRAY_SIZE(sensor_contrast_neg1_regs),
	 },
	{
	 .regs = sensor_contrast_zero_regs,
	 .size = ARRAY_SIZE(sensor_contrast_zero_regs),
	 },
	{
	 .regs = sensor_contrast_pos1_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos1_regs),
	 },
	{
	 .regs = sensor_contrast_pos2_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos2_regs),
	 },
	{
	 .regs = sensor_contrast_pos3_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos3_regs),
	 },
	{
	 .regs = sensor_contrast_pos4_regs,
	 .size = ARRAY_SIZE(sensor_contrast_pos4_regs),
	 },
};

/*
 * The saturation setttings
 */
static struct regval_list sensor_saturation_neg4_regs[] = {

};

static struct regval_list sensor_saturation_neg3_regs[] = {
};

static struct regval_list sensor_saturation_neg2_regs[] = {
};

static struct regval_list sensor_saturation_neg1_regs[] = {
};

static struct regval_list sensor_saturation_zero_regs[] = {
};

static struct regval_list sensor_saturation_pos1_regs[] = {

};

static struct regval_list sensor_saturation_pos2_regs[] = {

};

static struct regval_list sensor_saturation_pos3_regs[] = {

};

static struct regval_list sensor_saturation_pos4_regs[] = {

};

static struct cfg_array sensor_saturation[] = {
	{
	 .regs = sensor_saturation_neg4_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg4_regs),
	 },
	{
	 .regs = sensor_saturation_neg3_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg3_regs),
	 },
	{
	 .regs = sensor_saturation_neg2_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg2_regs),
	 },
	{
	 .regs = sensor_saturation_neg1_regs,
	 .size = ARRAY_SIZE(sensor_saturation_neg1_regs),
	 },
	{
	 .regs = sensor_saturation_zero_regs,
	 .size = ARRAY_SIZE(sensor_saturation_zero_regs),
	 },
	{
	 .regs = sensor_saturation_pos1_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos1_regs),
	 },
	{
	 .regs = sensor_saturation_pos2_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos2_regs),
	 },
	{
	 .regs = sensor_saturation_pos3_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos3_regs),
	 },
	{
	 .regs = sensor_saturation_pos4_regs,
	 .size = ARRAY_SIZE(sensor_saturation_pos4_regs),
	 },
};

/*
 * The exposure target setttings
 */
static struct regval_list sensor_ev_neg4_regs[] = {

};

static struct regval_list sensor_ev_neg3_regs[] = {

};

static struct regval_list sensor_ev_neg2_regs[] = {

};

static struct regval_list sensor_ev_neg1_regs[] = {

};

static struct regval_list sensor_ev_zero_regs[] = {

};

static struct regval_list sensor_ev_pos1_regs[] = {

};

static struct regval_list sensor_ev_pos2_regs[] = {

};

static struct regval_list sensor_ev_pos3_regs[] = {

};

static struct regval_list sensor_ev_pos4_regs[] = {

};

static struct cfg_array sensor_ev[] = {
	{
	 .regs = sensor_ev_neg4_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg4_regs),
	 },
	{
	 .regs = sensor_ev_neg3_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg3_regs),
	 },
	{
	 .regs = sensor_ev_neg2_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg2_regs),
	 },
	{
	 .regs = sensor_ev_neg1_regs,
	 .size = ARRAY_SIZE(sensor_ev_neg1_regs),
	 },
	{
	 .regs = sensor_ev_zero_regs,
	 .size = ARRAY_SIZE(sensor_ev_zero_regs),
	 },
	{
	 .regs = sensor_ev_pos1_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos1_regs),
	 },
	{
	 .regs = sensor_ev_pos2_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos2_regs),
	 },
	{
	 .regs = sensor_ev_pos3_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos3_regs),
	 },
	{
	 .regs = sensor_ev_pos4_regs,
	 .size = ARRAY_SIZE(sensor_ev_pos4_regs),
	 },
};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_yuv422_yuyv[] = {
};

static struct regval_list sensor_fmt_yuv422_yvyu[] = {
};

static struct regval_list sensor_fmt_yuv422_vyuy[] = {
};

static struct regval_list sensor_fmt_yuv422_uyvy[] = {
};

/* ************************spi write/read*********************** */
#define REG_OFFSET	3
#define READ_FLAG	0x40
static int sensor_spi_read_word(struct v4l2_subdev *sd, u8 reg)
{
	struct spi_device *spi = v4l2_get_subdevdata(sd);
	int ret;

	ret = spi_w8r16(spi, (reg << REG_OFFSET) | READ_FLAG);
	if (ret < 0)
		return ret;

	return be16_to_cpu((__force __be16)ret);
}

static int sensor_spi_write_word(struct v4l2_subdev *sd, u8 reg, u16 data)
{
	struct spi_device *spi = v4l2_get_subdevdata(sd);
	u8 buf[3];

	buf[0] = reg << REG_OFFSET;
	put_unaligned_be16(data, &buf[1]);

	return spi_write(spi, buf, sizeof(buf));
}

static int sensor_spi_read_byte(struct v4l2_subdev *sd, u8 reg)
{
	struct spi_device *spi = v4l2_get_subdevdata(sd);

	return spi_w8r8(spi, (reg << REG_OFFSET) | READ_FLAG);
}

static int sensor_spi_write_byte(struct v4l2_subdev *sd, u8 reg, u8 data)
{
	struct spi_device *spi = v4l2_get_subdevdata(sd);
	u8 buf[2];

	buf[0] = reg << REG_OFFSET;
	buf[1] = data;

	return spi_write(spi, buf, sizeof(buf));
}

static int sensor_read(struct v4l2_subdev *sd, u8 addr, u8 *value)
{
	*value = sensor_spi_read_byte(sd, addr);
	return (*value < 0) ? -1 : 0;
}

static int sensor_write(struct v4l2_subdev *sd, u8 addr, u8 value)
{
	return sensor_spi_write_byte(sd, addr, value);
}

static int sensor_write_array(struct v4l2_subdev *sd,
				struct regval_list *regs,
				int array_size)
{
	int ret = 0, i = 0;

	if (!regs)
		return -EINVAL;

	while (i < array_size) {
		if (regs->addr == REG_DLY) {
			msleep(regs->data);
		} else {
			ret = sensor_write(sd, regs->addr, regs->data);
			if (ret < 0)
				sensor_err("sensor write array error!\n");
		}
		i++;
		regs++;
	}
	return 0;
}


/* ****************************begin of *********************** */

static int sensor_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_hflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_vflip(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_autogain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;

}

static int sensor_s_autoexp(struct v4l2_subdev *sd,
			    enum v4l2_exposure_auto_type value)
{
	return 0;
}

static int sensor_g_autowb(struct v4l2_subdev *sd, int *value)
{

	return 0;
}

static int sensor_s_autowb(struct v4l2_subdev *sd, int value)
{
	return 0;
}

static int sensor_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_hue(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return -EINVAL;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int value)
{
	return -EINVAL;
}
static int sensor_g_band_filter(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_band_filter(struct v4l2_subdev *sd,
				enum v4l2_power_line_frequency value)
{
	return 0;
}

/* **************************end of ****************************** */

static int sensor_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->brightness;
	return 0;
}

static int sensor_s_brightness(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->brightness == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_brightness[value + 4].regs,
				sensor_brightness[value + 4].size);

	info->brightness = value;
	return 0;
}

static int sensor_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->contrast;
	return 0;
}

static int sensor_s_contrast(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->contrast == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_contrast[value + 4].regs,
				sensor_contrast[value + 4].size);

	info->contrast = value;
	return 0;
}

static int sensor_g_saturation(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->saturation;
	return 0;
}

static int sensor_s_saturation(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->saturation == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_saturation[value + 4].regs,
				sensor_saturation[value + 4].size);

	info->saturation = value;
	return 0;
}

static int sensor_g_exp_bias(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp_bias;
	return 0;
}

static int sensor_s_exp_bias(struct v4l2_subdev *sd, int value)
{
	struct sensor_info *info = to_state(sd);

	if (info->exp_bias == value)
		return 0;

	if (value < -4 || value > 4)
		return -ERANGE;

	sensor_write_array(sd, sensor_ev[value + 4].regs,
				sensor_ev[value + 4].size);

	info->exp_bias = value;
	return 0;
}

static int sensor_g_wb(struct v4l2_subdev *sd, int *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_auto_n_preset_white_balance *wb_type =
	    (enum v4l2_auto_n_preset_white_balance *)value;

	*wb_type = info->wb;

	return 0;
}

static int sensor_s_wb(struct v4l2_subdev *sd,
		       enum v4l2_auto_n_preset_white_balance value)
{
	struct sensor_info *info = to_state(sd);

	if (info->capture_mode == V4L2_MODE_IMAGE)
		return 0;

	if (info->wb == value)
		return 0;

	sensor_write_array(sd, sensor_wb[value].regs,
				sensor_wb[value].size);

	if (value == V4L2_WHITE_BALANCE_AUTO)
		info->autowb = 1;
	else
		info->autowb = 0;

	info->wb = value;
	return 0;
}

static int sensor_g_colorfx(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_colorfx *clrfx_type = (enum v4l2_colorfx *)value;

	*clrfx_type = info->clrfx;
	return 0;
}

static int sensor_s_colorfx(struct v4l2_subdev *sd, enum v4l2_colorfx value)
{
	struct sensor_info *info = to_state(sd);

	if (info->clrfx == value)
		return 0;

	sensor_write_array(sd, sensor_colorfx[value].regs,
				sensor_colorfx[value].size);

	info->clrfx = value;
	return 0;
}

static int sensor_g_flash_mode(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	enum v4l2_flash_led_mode *flash_mode =
	    (enum v4l2_flash_led_mode *)value;

	*flash_mode = info->flash_mode;
	return 0;
}

static int sensor_s_flash_mode(struct v4l2_subdev *sd,
			       enum v4l2_flash_led_mode value)
{
	struct sensor_info *info = to_state(sd);

	info->flash_mode = value;
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 0;
	info->height = 0;
	info->brightness = 0;
	info->contrast = 0;
	info->saturation = 0;
	info->hue = 0;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->autogain = 1;
	info->exp_bias = 0;
	info->autoexp = 1;
	info->autowb = 1;
	info->wb = V4L2_WHITE_BALANCE_AUTO;
	info->clrfx = V4L2_COLORFX_NONE;
	info->band_filter = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	ret =
	    sensor_write_array(sd, sensor_default_regs,
			       ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_s_band_filter(sd, V4L2_CID_POWER_LINE_FREQUENCY_50HZ);

	info->preview_first_flag = 1;


	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	enum v4l2_mbus_pixelcode mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp;		/* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc = "YUYV 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_YUYV8_2X8,
		.regs = sensor_fmt_yuv422_yuyv,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yuyv),
		.bpp = 2,
	}, {
		.desc = "YVYU 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_YVYU8_2X8,
		.regs = sensor_fmt_yuv422_yvyu,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_yvyu),
		.bpp = 2,
	}, {
		.desc = "UYVY 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_UYVY8_2X8,
		.regs = sensor_fmt_yuv422_uyvy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_uyvy),
		.bpp = 2,
	}, {
		.desc = "VYUY 4:2:2",
		.mbus_code = V4L2_MBUS_FMT_VYUY8_2X8,
		.regs = sensor_fmt_yuv422_vyuy,
		.regs_size = ARRAY_SIZE(sensor_fmt_yuv422_vyuy),
		.bpp = 2,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	{/* UXGA */
		.width = UXGA_WIDTH,
		.height = UXGA_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.regs = sensor_uxga_regs,
		.regs_size = ARRAY_SIZE(sensor_uxga_regs),
		.set_size = NULL,
	 }, {/* 720p */
		.width = HD720_WIDTH,
		.height = HD720_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.regs = sensor_720p_regs,
		.regs_size = ARRAY_SIZE(sensor_720p_regs),
		.set_size = NULL,
	 }, {/* SVGA */
		.width = SVGA_WIDTH,
		.height = SVGA_HEIGHT,
		.hoffset = 0,
		.voffset = 0,
		.regs = sensor_svga_regs,
		.regs_size = ARRAY_SIZE(sensor_svga_regs),
		.set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_FMTS)
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;
	return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
			    struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index > N_WIN_SIZES - 1)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = sensor_win_sizes[fsize->index].width;
	fsize->discrete.height = sensor_win_sizes[fsize->index].height;
	return 0;
}
static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
				   struct v4l2_mbus_framefmt *fmt,
				   struct sensor_format_struct **ret_fmt,
				   struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize;
	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */
	fmt->field = V4L2_FIELD_NONE;

	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;

	if (wsize >= sensor_win_sizes + N_WIN_SIZES)
		wsize--;	/* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;

	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int ret;
	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_s_fmt\n");
	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;

	if (info->capture_mode == V4L2_MODE_IMAGE) {
		ret = sensor_s_autoexp(sd, V4L2_EXPOSURE_MANUAL);
		if (ret < 0)
			sensor_err("sensor_s_autoexp off err at image mode\n");
		ret = sensor_s_autogain(sd, 0);
		if (ret < 0)
			sensor_err("sensor_s_autogain off err at image mode\n");
		ret = sensor_s_autowb(sd, 0);
		if (ret < 0)
			sensor_err("sensor_s_autowb off err at image mode\n");
	}
	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);
	if (wsize->set_size)
		wsize->set_size(sd);

	sensor_s_hflip(sd, info->hflip);
	sensor_s_vflip(sd, info->vflip);

	if (info->capture_mode == V4L2_MODE_VIDEO ||
	    info->capture_mode == V4L2_MODE_PREVIEW) {
		ret = sensor_s_autoexp(sd, V4L2_EXPOSURE_AUTO);
		if (ret < 0)
			sensor_err("sensor_s_autoexp on err at video mode!\n");
		ret = sensor_s_autogain(sd, 1);
		if (ret < 0)
			sensor_err("sensor_s_autogain on err at video mode\n");
		if (info->wb == V4L2_WHITE_BALANCE_AUTO) {
			ret = sensor_s_autowb(sd, 1);
			if (ret < 0)
				sensor_err("sensor_s_autowb on err "
					"at video mode\n");
		}
	}
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);
	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;
	cp->timeperframe.numerator = info->tpf.numerator;
	cp->timeperframe.denominator = info->tpf.denominator;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd,
			struct v4l2_streamparm *parms)
{
	return 0;
}

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* ************************begin of ******************************** */
static int sensor_queryctrl(struct v4l2_subdev *sd,
			struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */
	/* see sensor_s_parm and sensor_g_parm for the meaning of value */
	switch (qc->id) {
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_SATURATION:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 1);
	case V4L2_CID_HUE:
		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 0, 255, 1, 128);
	case V4L2_CID_AUTOGAIN:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return v4l2_ctrl_query_fill(qc, -4, 4, 1, 0);
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 9, 1, 1);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	case V4L2_CID_COLORFX:
		return v4l2_ctrl_query_fill(qc, 0, 15, 1, 0);
	case V4L2_CID_FLASH_LED_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 4, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_g_saturation(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return sensor_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_g_exp_bias(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_g_autoexp(sd, &ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_g_wb(sd, &ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_g_autowb(sd, &ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_g_colorfx(sd, &ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_g_flash_mode(sd, &ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_g_band_filter(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;


	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (qc.type == V4L2_CTRL_TYPE_MENU ||
	    qc.type == V4L2_CTRL_TYPE_INTEGER ||
	    qc.type == V4L2_CTRL_TYPE_BOOLEAN) {
		if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
			return -ERANGE;
		}
	}

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return sensor_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return sensor_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return sensor_s_saturation(sd, ctrl->value);
	case V4L2_CID_HUE:
		return sensor_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return sensor_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return sensor_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return sensor_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_AUTO_EXPOSURE_BIAS:
		return sensor_s_exp_bias(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return sensor_s_autoexp(sd,
				(enum v4l2_exposure_auto_type)ctrl->value);
	case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		return sensor_s_wb(sd,
			(enum v4l2_auto_n_preset_white_balance)ctrl->value);
	case V4L2_CID_AUTO_WHITE_BALANCE:
		return sensor_s_autowb(sd, ctrl->value);
	case V4L2_CID_COLORFX:
		return sensor_s_colorfx(sd, (enum v4l2_colorfx)ctrl->value);
	case V4L2_CID_FLASH_LED_MODE:
		return sensor_s_flash_mode(sd,
			(enum v4l2_flash_led_mode)ctrl->value);
	case V4L2_CID_POWER_LINE_FREQUENCY:
		return sensor_s_band_filter(sd,
			(enum v4l2_power_line_frequency)ctrl->value);
	}
	return -EINVAL;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.enum_mbus_fmt = sensor_enum_fmt,
	.enum_framesizes = sensor_enum_size,
	.try_mbus_fmt = sensor_try_fmt,
	.s_mbus_fmt = sensor_s_fmt,
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
};

/* ----------------------------------------------------------------------- */

static int sensor_probe(struct spi_device *spi)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	v4l2_spi_subdev_init(sd, spi, &sensor_ops);
	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;
	info->auto_focus = 0;

	return 0;
}

static int sensor_remove(struct spi_device *spi)
{
	struct v4l2_subdev *sd;
	sd = spi_get_drvdata(spi);
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct spi_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(spi, sensor_id);
static struct spi_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static __init int init_sensor(void)
{
	return spi_register_driver(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	spi_unregister_driver(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);
