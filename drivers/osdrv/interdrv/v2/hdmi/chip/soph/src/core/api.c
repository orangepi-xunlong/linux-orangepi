#include "api.h"
#include "core/main_controller.h"
#include "core/video.h"
#include "core/audio.h"
#include "core/packets.h"
#include "core/irq.h"
#include "core/hdmi_core.h"
#include "core/hdmi_reg.h"
#include "hdcp/hdcp.h"
#include "edid/edid.h"
#include "util/util.h"
#include "identification/identification.h"

void api_set_hdmi_ctrl(hdmi_tx_dev_t *dev, video_params_t * video, hdcp_params_t * hdcp)
{
	struct hdmi_tx_ctrl *tx_ctrl = &dev->snps_hdmi_ctrl;
	tx_ctrl->data_enable_polarity = 1;

	if (video->mencodingout == YCC422)
		tx_ctrl->color_resolution = 8;
	else
		tx_ctrl->color_resolution = video->mcolor_resolution;
	tx_ctrl->encoding = video->mencodingout;

}

int api_configure(hdmi_tx_dev_t *dev, video_params_t * video, audio_params_t * audio, hdcp_params_t * hdcp)
{
	int res = 0;

	if (!dev) {
		pr_err("%s: Device pointer invalid", __func__);
		return HDMI_ERR_DEV_POINTER_INVALID;
	}

	if (!video || !audio || !hdcp) {
		pr_err("%s: Argument Invalid received", __func__);
		return HDMI_ERR_ARG_INVALID;
	}

	api_set_hdmi_ctrl(dev, video, hdcp);

	if(!dev->snps_hdmi_ctrl.pixel_clock) {
		dev->snps_hdmi_ctrl.pixel_clock = video->mdtd.m_pixel_clock;
	}

	cvitek_hdmi_clk_set(dev->snps_hdmi_ctrl.pixel_clock);
	disp_hdmi_gen(&(video->mdtd));
	udelay(10);

	phy_standby(dev);
	api_avmute(dev, TRUE);
	irq_mute(dev);

	res = video_configure(dev, video);
	if (res != 0) {
		pr_err("%s:Could not configure video", __func__);
		return res;
	}

	// Audio
	audio_Initialize(dev);
	res = audio_configure(dev, audio);
	if(res != 0){
		pr_err("%s:Audio not configured", __func__);
		return res;
	}

	// Packets
	res = packets_configure(dev, video);
	if (res != 0) {
		pr_err("%s:Could not configure packets\n", __func__);
		return res;
	}

	mc_enable_all_clocks(dev);
	udelay(10);

	res = phy_configure(dev);
	if (res != 0) {
		pr_err("%s:Could not configure PHY", __func__);
		return res;
	}

	// Disable blue screen transmission after turning on all necessary blocks (e.g. HDCP)
	fc_force_output(dev, FALSE);

	// HDCP is PHY independent
	hdcp_initialize(dev);
	udelay(20);

	res = hdcp_configure(dev, hdcp, video);
	if (res != 0) {
		pr_err("%s:Could not configure HDCP", __func__);
		return res;
	}

	api_avmute(dev, FALSE);

	return 0;
}

int api_standby(hdmi_tx_dev_t *dev)
{
	control_standby(dev);
	phy_standby(dev);

	return TRUE;
}

void api_avmute(hdmi_tx_dev_t *dev, int enable)
{
	packets_av_mute(dev, enable);
	hdcp_av_mute(dev, enable);
	dev->snps_hdmi_ctrl.avmute = enable ? TRUE : FALSE;
}
