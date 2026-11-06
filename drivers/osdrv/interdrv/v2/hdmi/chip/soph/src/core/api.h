#ifndef _API_H_
#define _API_H_
#include "util/util.h"
#include "core/video.h"
#include "core/audio.h"
#include "hdcp/hdcp.h"
#include "edid/edid.h"
#include "edid/desc.h"
#include "edid/hdmivsdb.h"
#include "edid/data_block.h"
#include "core/fc.h"
#include "bsp/access.h"
#include "phy/phy.h"
#include "hdmitx_dev.h"


void api_set_hdmi_ctrl(hdmi_tx_dev_t *dev, video_params_t * video, hdcp_params_t * hdcp);

/**
 * Configure API.
 * Configure the modules of the API according to the parameters given by
 * the user. If EDID at sink is read, it does parameter checking using the
 * Check methods against the sink's E-EDID. Violations are outputted to the
 * buffer.
 * Shall only be called after an Init call or configure.
 * @param video parameters pointer
 * @param audio parameters pointer
 * @param hdcp parameters pointer
 * @return TRUE when successful
 * @note during this function, all controller's interrupts are disabled
 * @note this function needs to have the HW initialized before the first call
 */
int api_configure(hdmi_tx_dev_t *dev, video_params_t * video, audio_params_t * audio, hdcp_params_t * hdcp);

/**
 * Prepare API modules and local variables to standby mode (hdmi_tx_dev_t *dev, and not respond
 * to interrupts) and frees all resources
 * @return TRUE when successful
 * @note must be called to free up resources and before another Init.
 */
int api_standby(hdmi_tx_dev_t *dev);

/**
 * AV Mute in the General Control Packet
 * @param enable TRUE set the AVMute in the general control packet
 */
void api_avmute(hdmi_tx_dev_t *dev, int enable);

#endif	/* _API_H_ */
