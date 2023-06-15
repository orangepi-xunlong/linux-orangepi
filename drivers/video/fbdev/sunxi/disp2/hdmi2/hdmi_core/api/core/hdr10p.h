/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#ifndef HDR10P_H_
#define HDR10P_H_

#include "../hdmitx_dev.h"
#include "video.h"
#include "audio.h"

#include "frame_composer_reg.h"
#include "../log.h"
#include "../edid.h"
#include "../general_ops.h"
#include <video/sunxi_display2.h>

int hdr10p_Configure(hdmi_tx_dev_t *dev, void *config,
	videoParams_t *video, productParams_t *prod,
	struct disp_device_dynamic_config *scfg);

#endif	/* HDR10P_H_ */
