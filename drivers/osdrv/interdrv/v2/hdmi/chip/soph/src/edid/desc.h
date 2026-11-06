#ifndef _DESC_H_
#define _DESC_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

typedef struct {
	u8 mformat;

	u8 m_max_channels;

	u8 m_sample_rates;

	u8 mbyte3;
} short_audio_desc_t;

typedef struct {
	int mnative;

	unsigned m_code;

	unsigned m_limited_to_ycc420;

	unsigned m_ycc420;

} short_video_desc_t;

typedef struct {
	u8 mmin_vertical_rate;

	u8 mmax_vertical_rate;

	u8 mmin_horizontal_rate;

	u8 mmax_horizontal_rate;

	u8 mmax_pixel_clock;

	int mvalid;
} monitor_range_limits_t;

void monitor_range_limits_reset(hdmi_tx_dev_t *dev, monitor_range_limits_t * mrl);

int svd_parse(hdmi_tx_dev_t *dev, short_video_desc_t * svd, u8 data);

void sad_reset(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_parse(hdmi_tx_dev_t *dev, short_audio_desc_t * sad, u8 * data);

//u8 sad_GetSampleRates(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support32k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support44k1(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support48k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support88k2(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support96k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support176k4(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support192k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support16bit(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support20bit(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

int sad_support24bit(hdmi_tx_dev_t *dev, short_audio_desc_t * sad);

#endif	/* _DESC_H_ */
