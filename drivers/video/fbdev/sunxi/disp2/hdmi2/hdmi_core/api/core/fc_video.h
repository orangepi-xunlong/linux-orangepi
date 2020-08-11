/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef HALFRAMECOMPOSERVIDEO_H_
#define HALFRAMECOMPOSERVIDEO_H_

#include "video.h"
#include "frame_composer_reg.h"
#include "../hdmitx_dev.h"
#include "../access.h"
#include "../log.h"

void fc_force_video(hdmi_tx_dev_t *dev, u8 bit);
void fc_force_audio(hdmi_tx_dev_t *dev, u8 bit);

void fc_force_output(hdmi_tx_dev_t *dev, int enable);

int fc_video_config(hdmi_tx_dev_t *dev, videoParams_t *video);

void fc_video_hdcp_keepout(hdmi_tx_dev_t *dev, u8 bit);
u8 fc_video_tmdsMode_get(hdmi_tx_dev_t *dev);
void fc_video_DviOrHdmi(hdmi_tx_dev_t *dev, u8 bit);

#endif	/* HALFRAMECOMPOSERVIDEO_H_ */
