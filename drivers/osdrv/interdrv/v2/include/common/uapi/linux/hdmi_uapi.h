/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: hdmi_uapi.h
 * Description:
 */

#ifndef _U_HDMI_UAPI_H_
#define _U_HDMI_UAPI_H_


#include <linux/comm_sys.h>
#include <linux/common.h>
#include <linux/comm_hdmi.h>

/* Public */
#define CVI_HDMI_INIT _IO('H', 0x00)
#define CVI_HDMI_DEINIT _IO('H', 0x01)
#define CVI_HDMI_OPEN _IO('H', 0x02)
#define CVI_HDMI_CLOSE _IO('H', 0x03)
#define CVI_HDMI_GET_SINK_CAPABILITY _IOWR('H', 0x04, hdmi_sink_capability)
#define CVI_HDMI_SET_ATTR _IOW('H', 0x05, hdmi_attr)
#define CVI_HDMI_GET_ATTR _IOR('H', 0x06, hdmi_attr)
#define CVI_HDMI_START _IO('H', 0x07)
#define CVI_HDMI_STOP _IO('H', 0x08)
#define CVI_HDMI_FORCE_GET_EDID _IOWR('H', 0x9, hdmi_edid)
#define CVI_HDMI_GET_EVENT_ID _IOR('H', 0xa, uint32_t)
#define CVI_HDMI_UNREGISTER_CALLBACK _IO('H', 0xb)
#define CVI_HDMI_SET_INFOFRAME _IOWR('H', 0x0c, hdmi_infoframe )
#define CVI_HDMI_GET_INFOFRAME _IOWR('H', 0x0d, hdmi_infoframe)
#define CVI_HDMI_SET_HW_SPEC _IOW('H', 0x0e, hdmi_hw_spec)
#define CVI_HDMI_GET_HW_SPEC _IOW('H', 0x0f, hdmi_hw_spec)
#define CVI_HDMI_SET_AVMUTE _IOW('H', 0xa0, bool)
#define CVI_HDMI_SET_AUDIO_MUTE _IOW('H', 0xa3, bool)

#endif /* _U_HDMI_UAPI_H_ */
