/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef API_H_
#define API_H_

extern void register_func_to_hdmi_core(struct hdmi_dev_func func);
void hdmitx_api_init(hdmi_tx_dev_t *dev,
						videoParams_t *video,
						audioParams_t *audio,
						hdcpParams_t *hdcp);
void hdmitx_api_exit(void);

#endif	/* API_H_ */
