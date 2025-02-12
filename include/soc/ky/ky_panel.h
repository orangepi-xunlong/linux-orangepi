// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef __KY_PANEL_H__
#define __KY_PANEL_H__

#include <linux/notifier.h>

/*  complete the definition of DRM Macros */
enum{
	DRM_PANEL_EARLY_EVENT_BLANK = 0,
	DRM_PANEL_EVENT_BLANK,
	DRM_PANEL_BLANK_UNBLANK,
	DRM_PANEL_BLANK_POWERDOWN,
};

extern int ky_drm_register_client(struct notifier_block *nb);
extern int ky_drm_unregister_client(struct notifier_block *nb);


/*  complete the definition of hdmi connect status */
enum{
	DRM_HDMI_EVENT_CONNECTED = 0,
	DRM_HDMI_EVENT_DISCONNECTED,
};

extern int ky_hdmi_register_client(struct notifier_block *nb);
extern int ky_hdmi_unregister_client(struct notifier_block *nb);

typedef enum{
	HEADSET_EVENT_CONNECTED = 0,
	HEADSET_EVENT_DISCONNECTED,
	HEADPHONE_EVENT_CONNECTED,
	HEADPHONE_EVENT_DISCONNECTED,
	HPMIC_EVENT_CONNECTED,
	HPMIC_EVENT_DISCONNECTED,
} __alsa_codec_event_e;

extern int ky_headphone_register_client(struct notifier_block *nb);
extern int ky_headphone_unregister_client(struct notifier_block *nb);
extern int ky_headphone_notifier_call_chain(__alsa_codec_event_e val, char *v);

#endif
