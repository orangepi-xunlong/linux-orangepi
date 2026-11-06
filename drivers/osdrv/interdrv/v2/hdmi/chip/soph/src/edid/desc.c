#include "edid/desc.h"
#include "util/util.h"

void sad_reset(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	sad->mformat = 0;
	sad->m_max_channels = 0;
	sad->m_sample_rates = 0;
	sad->mbyte3 = 0;
}

int sad_parse(hdmi_tx_dev_t *dev, short_audio_desc_t * sad, u8 * data)
{
	sad_reset(dev, sad);
	if (data != 0) {
		sad->mformat = bit_field(data[0], 3, 4);
		sad->m_max_channels = bit_field(data[0], 0, 3) + 1;
		sad->m_sample_rates = bit_field(data[1], 0, 7);
		sad->mbyte3 = data[2];
		return TRUE;
	}
	return FALSE;
}

int sad_support32k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 0, 1) == 1) ? TRUE : FALSE;
}

int sad_support44k1(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 1, 1) == 1) ? TRUE : FALSE;
}

int sad_support48k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 2, 1) == 1) ? TRUE : FALSE;
}

int sad_support88k2(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 3, 1) == 1) ? TRUE : FALSE;
}

int sad_support96k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 4, 1) == 1) ? TRUE : FALSE;
}

int sad_support176k4(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 5, 1) == 1) ? TRUE : FALSE;
}

int sad_support192k(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	return (bit_field(sad->m_sample_rates, 6, 1) == 1) ? TRUE : FALSE;
}

int sad_support16bit(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	if (sad->mformat == 1) {
		return (bit_field(sad->mbyte3, 0, 1) == 1) ? TRUE : FALSE;
	}
	pr_err("Information is not valid for this format");
	return FALSE;
}

int sad_support20bit(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	if (sad->mformat == 1) {
		return (bit_field(sad->mbyte3, 1, 1) == 1) ? TRUE : FALSE;
	}
	pr_err("Information is not valid for this format");
	return FALSE;
}

int sad_support24bit(hdmi_tx_dev_t *dev, short_audio_desc_t * sad)
{
	if (sad->mformat == 1) {
		return (bit_field(sad->mbyte3, 2, 1) == 1) ? TRUE : FALSE;
	}
	pr_err("Information is not valid for this format");
	return FALSE;
}

void svd_reset(hdmi_tx_dev_t *dev, short_video_desc_t * svd)
{
	svd->mnative = FALSE;
	svd->m_code = 0;
}

int svd_parse(hdmi_tx_dev_t *dev, short_video_desc_t * svd, u8 data)
{
	svd_reset(dev, svd);
	svd->mnative = (bit_field(data, 7, 1) == 1) ? TRUE : FALSE;
	svd->m_code = bit_field(data, 0, 7);
	svd->m_limited_to_ycc420 = 0;
	svd->m_ycc420 = 0;
	return TRUE;
}

void monitor_range_limits_reset(hdmi_tx_dev_t *dev, monitor_range_limits_t * mrl)
{
	mrl->mmin_vertical_rate = 0;
	mrl->mmax_vertical_rate = 0;
	mrl->mmin_horizontal_rate = 0;
	mrl->mmax_horizontal_rate = 0;
	mrl->mmax_pixel_clock = 0;
	mrl->mvalid = FALSE;
}
